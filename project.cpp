//cs371 Fall 2015
//This program demonstrates a 3d balloon object
//
//
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#include <X11/keysym.h>
#include <sys/types.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include "ppm.h"
#include "fonts.h"


typedef float Flt;
typedef Flt Vec[3];
#define SWAP(x,y) (x)^=(y);(y)^=(x); (x)^=(y)
#define rnd() (float)rand() / (float)RAND_MAX
#define PI 3.14159265358979323846264338327950
//X Windows variables
Display *dpy;
Window win;
GLXContext glc;

enum {
    MODE_NONE,
    MODE_LINES,
    LINETYPE_XWIN,
    LINETYPE_BRESENHAM
};

#define MAX_CONTROL_POINTS 10000


struct Mass {
    float mass;
    float oomass;
    float pos[3];
    float vel[3];
    float force[3];
};
struct Spring {
    int mass[2];
    float length;
    float stiffness;

};

struct Global {
    int xres, yres;
    int mode;
    int done;
    //points used as endpoints for lines.
    float point[MAX_CONTROL_POINTS][4];
    float point2[MAX_CONTROL_POINTS][MAX_CONTROL_POINTS][3];
    Mass mass[MAX_CONTROL_POINTS];
    Spring spring[MAX_CONTROL_POINTS];
    int npoints;
    float bCenter[2];
    int center;
    int nmasses;
    int corner;
    int nsprings;
    int nearcp;
    int grabbedPoint;
    int showAnchors;
    int curvemode;
    int snapto;
    int lineapp;
    int springs;
    int nCurvePoints;
    int linetype;
    Global() {
        //constructor
        xres=640;
        yres=480;
        mode=0;
        corner =0;
        done=0;
        nearcp = -1;
        npoints=0;
        nCurvePoints = 4;
        grabbedPoint = -1;
        center = 0;
        snapto = 0;
        nmasses = 0;
        nsprings = 0;
        springs = 0;
        lineapp = 1;
        showAnchors = 1;
        curvemode = 0;
        linetype = LINETYPE_XWIN;
        int loc[2] = {50, 50};
        int step = 5;
        int k=0;
        for(int d=0; d<40; d++){
            for(int c =0; c<40; c++){
                point[k][0]=loc[0];
                point[k][1]=loc[1];
                point[k][2]=0;
                loc[0] += step;
                npoints++;
                k++;
            }
            loc[1] += step;
            loc[0] = 50;
        }
    }
} g;


void initXWindows(void);
void init_opengl(void);
void init_textures(void);
void cleanupXWindows(void);
void check_resize(XEvent *e);
void check_mouse(XEvent *e);
void check_keys(XEvent *e);
void physics(void);
void render(void);
void createBalloon();
void changesize(int);
void setupSprings();
void maintainSprings();
void clearScreen();
void moveCorner(int);
float length(float, float, float, float);

int done=0;
int xres=640, yres=480;
float pos[3]={20.0,200.0,0.0};
float vel[3]={3.0,0.0,0.0};

int anc = 1;
int anc2 = 1;
int lesson_num=0;
float y = 1.0;

float balloon_n[482][3];
float balloon_v[482][3];
float rtri = 0.0f;
float rquad = 0.0f;
float air = 0.005f;
float cubeRot[3]={2.0,0.0,0.0};
float cubeAng[3]={0.0,0.0,0.0};
GLfloat LightAmbient[]  = {  0.0f, 0.0f, 0.0f, 1.0f };
GLfloat LightDiffuse[]  = {  1.0f, 1.0f, 1.0f, 1.0f };
GLfloat LightSpecular[] = {  0.5f, 0.5f, 0.5f, 1.0f };
GLfloat LightPosition[] = { 100.0f, 40.0f, 40.0f, 1.0f };

Ppmimage *bballImage=NULL;
Ppmimage *courtImage=NULL;
Ppmimage *standsImage=NULL;
GLuint bballTextureId;
GLuint courtTextureId;
GLuint standsTextureId;


int main(void)
{
    srand((unsigned)time(NULL));
	initXWindows();
	init_opengl();
	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
	//init_textures();
	while(!done) {
		while(XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			check_resize(&e);
			check_mouse(&e);
			check_keys(&e);
		}
		physics();
		render();
		glXSwapBuffers(dpy, win);
	}
	cleanupXWindows();
	cleanup_fonts();
	return 0;
}

void cleanupXWindows(void)
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void set_title(void)
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "CS371 Project - Balloon Animation");
}

void setup_screen_res(const int w, const int h)
{
	xres = w;
	yres = h;
}

void initXWindows(void)
{
	Window root;
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
	XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;

	setup_screen_res(640, 480);
	dpy = XOpenDisplay(NULL);
	if(dpy == NULL) {
		printf("\n\tcannot connect to X server\n\n");
		exit(EXIT_FAILURE);
	}
	root = DefaultRootWindow(dpy);
	vi = glXChooseVisual(dpy, 0, att);
	if(vi == NULL) {
		printf("\n\tno appropriate visual found\n\n");
		exit(EXIT_FAILURE);
	} 
	//else {
	//	// %p creates hexadecimal output like in glxinfo
	//	printf("\n\tvisual %p selected\n", (void *)vi->visualid);
	//}
	cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
						StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, xres, yres, 0,
							vi->depth, InputOutput, vi->visual,
							CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void reshape_window(int width, int height)
{
	//window has been resized.
	setup_screen_res(width, height);
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, xres, 0, yres, -1, 1);
	set_title();
}

void init_textures(void)
{
	//load the images file into a ppm structure.
	bballImage = ppm6GetImage("./vball.ppm");
	courtImage = ppm6GetImage("./grass.ppm");
	//
	//create opengl texture elements
	glGenTextures(1, &bballTextureId);
	int w = bballImage->width;
	int h = bballImage->height;
	//
	glBindTexture(GL_TEXTURE_2D, bballTextureId);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, bballImage->data);
	//
	//create opengl texture elements
	glGenTextures(1, &courtTextureId);
	w = courtImage->width;
	h = courtImage->height;
	//
	glBindTexture(GL_TEXTURE_2D, courtTextureId);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, courtImage->data);
}

#define VecCross(a,b,c) \
(c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1]; \
(c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2]; \
(c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0]

void vecCrossProduct(Vec v0, Vec v1, Vec dest)
{
	dest[0] = v0[1]*v1[2] - v1[1]*v0[2];
	dest[1] = v0[2]*v1[0] - v1[2]*v0[0];
	dest[2] = v0[0]*v1[1] - v1[0]*v0[1];
}

Flt vecDotProduct(Vec v0, Vec v1)
{
	return v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
}

void vecZero(Vec v)
{
	v[0] = v[1] = v[2] = 0.0;
}

void vecMake(Flt a, Flt b, Flt c, Vec v)
{
	v[0] = a;
	v[1] = b;
	v[2] = c;
}

void vecCopy(Vec source, Vec dest)
{
	dest[0] = source[0];
	dest[1] = source[1];
	dest[2] = source[2];
}

Flt vecLength(Vec v)
{
	return sqrt(vecDotProduct(v, v));
}

void vecNormalize(Vec v)
{
	Flt len = vecLength(v);
	if (len == 0.0) {
		vecMake(0,0,1,v);
		return;
	}
	len = 1.0 / len;
	v[0] *= len;
	v[1] *= len;
	v[2] *= len;
}

void vecSub(Vec v0, Vec v1, Vec dest)
{
	dest[0] = v0[0] - v1[0];
	dest[1] = v0[1] - v1[1];
	dest[2] = v0[2] - v1[2];
}

void changesize(int i){
    if(i)
        y = y + 0.01;
    else
        y = y - 0.01;
}

void moveCorner(int i){
    if(g.corner == 0)
        return;
    if(g.corner == 1){
        if(i==0){
            g.point[0][0] += 10.0;
            g.point[1][0] += 10.0;
            g.point[40][0] +=10.0;
            g.point[41][0] +=10.0;
        }
        else if(i==1){
            g.point[0][0] -= 10.0;
            g.point[1][0] -= 10.0;
            g.point[40][0] -=10.0;
            g.point[41][0] -=10.0;
        }
        else if(i==2){
            g.point[0][1] += 10.0;
            g.point[1][1] += 10.0;
            g.point[40][1] +=10.0;
            g.point[41][1] +=10.0;
        }
        else if(i==3){
            g.point[0][1] -= 10.0;
            g.point[1][1] -= 10.0;
            g.point[40][1] -=10.0;
            g.point[41][1] -=10.0;
        }
    }
    else if(g.corner == 2){
        if(i==0){
            g.point[38][0] += 10.0;
            g.point[39][0] += 10.0;
            g.point[78][0] +=10.0;
            g.point[79][0] +=10.0;
        }
        else if(i==1){
            g.point[38][0] -= 10.0;
            g.point[39][0] -= 10.0;
            g.point[78][0] -=10.0;
            g.point[79][0] -=10.0;
        }
        else if(i==2){
            g.point[38][1] += 10.0;
            g.point[39][1] += 10.0;
            g.point[78][1] +=10.0;
            g.point[79][1] +=10.0;
        }
        else if(i==3){
            g.point[38][1] -= 10.0;
            g.point[39][1] -= 10.0;
            g.point[78][1] -=10.0;
            g.point[79][1] -=10.0;
        }
    }
    else if(g.corner == 3){
        if(i==0){
            g.point[1520][0] += 10.0;
            g.point[1521][0] += 10.0;
            g.point[1560][0] +=10.0;
            g.point[1561][0] +=10.0;
        }
        else if(i==1){
            g.point[1520][0] -= 10.0;
            g.point[1521][0] -= 10.0;
            g.point[1560][0] -=10.0;
            g.point[1561][0] -=10.0;
        }
        else if(i==2){
            g.point[1520][1] += 10.0;
            g.point[1521][1] += 10.0;
            g.point[1560][1] +=10.0;
            g.point[1561][1] +=10.0;
        }
        else if(i==3){
            g.point[1520][1] -= 10.0;
            g.point[1521][1] -= 10.0;
            g.point[1560][1] -=10.0;
            g.point[1561][1] -=10.0;
        }
    }
    else if(g.corner == 4){
        if(i==0){
            g.point[g.nmasses-1][0] += 10.0;
            g.point[g.nmasses-2][0] += 10.0;
            g.point[g.nmasses-41][0] +=10.0;
            g.point[g.nmasses-42][0] +=10.0;
        }
        else if(i==1){
            g.point[g.nmasses-1][0] -= 10.0;
            g.point[g.nmasses-2][0] -= 10.0;
            g.point[g.nmasses-41][0] -=10.0;
            g.point[g.nmasses-42][0] -=10.0;
        }
        else if(i==2){
            g.point[g.nmasses-1][1] += 10.0;
            g.point[g.nmasses-2][1] += 10.0;
            g.point[g.nmasses-41][1] +=10.0;
            g.point[g.nmasses-42][1] +=10.0;
        }
        else if(i==3){
            g.point[g.nmasses-1][1] -= 10.0;
            g.point[g.nmasses-2][1] -= 10.0;
            g.point[g.nmasses-41][1] -=10.0;
            g.point[g.nmasses-42][1] -=10.0;
        }
    }
    maintainSprings();
}


void init_opengl(void)
{
	//OpenGL initialization
	switch(lesson_num) {
		case 0:
		case 1:
		//case 5:
		//case 6:
		//case 7:
		//case 8:
		default:
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClearDepth(1.0);
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			glShadeModel(GL_SMOOTH);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(45.0f,(GLfloat)xres/(GLfloat)yres,0.1f,100.0f);
			glMatrixMode(GL_MODELVIEW);
			//Enable this so material colors are the same as vert colors.
			glEnable(GL_COLOR_MATERIAL);
			glEnable( GL_LIGHTING );
			glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
			glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);
			glLightfv(GL_LIGHT0, GL_POSITION,LightPosition);
			glEnable(GL_LIGHT0);
			break;
	}
}

void check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != xres || xce.height != yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}

void check_mouse(XEvent *e)
{
    int mx = e->xbutton.x;
    mx = mx;
    //Did the mouse move?
	//Was a mouse button clicked?
	/*static int savex = 0;
	static int savey = 0;
    static int grab_offset[2];
    int sdist = 1000;
	//
    int mx = e->xbutton.x;
    int my = e->xbutton.y;
    //
    //int nearcp_dist = 10000;
    g.nearcp = -1;
	if (e->type == ButtonRelease) {
        setupSprings();
        g.grabbedPoint = -1;
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button is down
            if (g.mode >= 0) {
                float d0, d1, dist;
                //set grabbedPoint to first control point found to be
                //within range of the mouse pointer
                //...
                int k = -1;
                for (int i = 0; i < g.npoints; i++) {
                    d0 = (float)(g.point[i][0] - mx);
                    d1 = (float)(g.point[i][1] - my);
                    dist = d0*d0 + d1*d1;
                    //Is mouse within about 20 pixels?
                    if(sdist > dist){
                        sdist = dist;
                        k = i;
                        grab_offset[0] = d0;
                        grab_offset[1] = d1;
                    }
                }
                g.grabbedPoint = k;
            }
		}
		if (e->xbutton.button==3) {
			//Right button is down
		}
	}
	if (savex != mx || savey != my) {
		//Mouse moved
		savex = mx;
		savey = my;
        g.point[g.grabbedPoint][0] = mx;
        g.point[g.grabbedPoint][1] = my;
	}*/
}

void check_keys(XEvent *e)
{
	//Was there input from the keyboard?
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);
		switch(key) {
			case XK_1:
				lesson_num = 1;
				init_opengl();
				break;
            case XK_l:
                anc ^=1;
                break;
            case XK_k:
                anc2 ^=1;
                break;
            case XK_y:
                rtri +=45.0f;
                break;
            case XK_x:
                rquad +=45.0f;
                break;
            case XK_f:
                g.springs ^= 1;
                setupSprings();
                break;
            case XK_q:
                g.corner++;
                if(g.corner == 5)
                    g.corner = 0; 
                break;
            case XK_w:
                moveCorner(2);
                break;
            case XK_s:
                moveCorner(3);
                break;
            case XK_a:
                moveCorner(1);
                break;
            case XK_d:
                moveCorner(0);
                break;
			case XK_minus:
                air -= 0.005;
                maintainSprings();
                break;
            case XK_equal:
                air += 0.005;
                maintainSprings();
                break;
			case XK_Escape:
				done=1;
				break;
		}
	}
}

float length(float a, float b, float c, float d){
    float d0, d1, length;
    d0 = abs(a)-abs(c);
    d1 = abs(b)-abs(d);
    length = sqrt(d0*d0+d1*d1);
    return length;
}

void setupSprings(){
    if(!g.springs)
        return;
    //physics();
    //int xpos = g.xres / 2.0 - 120.0;
    g.nmasses = g.nsprings = 0;
    int i = 0;
    while (i<g.npoints){
        for(int a=0;a<40;a++){
            for(int b=0;b<40;b++){
                g.mass[i].mass = rnd() * 4.0 + 4.0;
                g.mass[i].oomass = 1.0 / g.mass[i].mass;
                g.mass[i].pos[0] = g.point[a*40+b][0];
                g.mass[i].pos[1] = g.point[a*40+b][1];
                g.mass[i].vel[0] = 0;
                g.mass[i].vel[1] = 0;
                g.mass[i].force[0] = 0.0;
                g.mass[i].force[1] = 0.0;
                g.nmasses++;
                i++;
            }
        }
    }
    i=0;
    int a =0;
    int b =0;
    while(a*b<1521) {
        for(a=0; a<40; a++){
            for(b=0; b<40; b++){
                if(a*b>=1521)
                    break;
                if(a!=39){
                    g.spring[i].mass[0]=a*40+b;
                    g.spring[i].mass[1]=(a+1)*40+b;
                    g.spring[i].length = length(g.point[a*40+b][0], g.point[a*40+b][1], g.point[(a+1)*40+b][0], g.point[(a+1)*40+b][1]);
                    if((a>=10 && a<=30) && (b>=10 && b<=30))
                        g.spring[i].stiffness = rnd() * 0.005 + 0.001;
                    else if (a==0 || a==39 
                            || b==0 || b==39)
                        g.spring[i].stiffness = rnd() * 0.5 +0.1;
                    else
                        g.spring[i].stiffness = rnd() * 0.05 +0.01;
                    g.nsprings++;
                    i++;
                    if(b!=39){
                        g.spring[i].mass[0]=a*40+b;
                        g.spring[i].mass[1]=(a+1)*40+b+1;
                        g.spring[i].length = length(g.point[a*40+b][0], g.point[a*40+b][1], g.point[(a+1)*40+b+1][0], g.point[(a+1)*40+b+1][1]);
                        if((a>=10 && a<=30) && (b>=10 && b<=30))
                            g.spring[i].stiffness = rnd() * 0.005 + 0.001;
                        else if (a==0 || a==39 
                                || b==0 || b==39)
                            g.spring[i].stiffness = rnd() * 0.5 +0.1;
                        else
                            g.spring[i].stiffness = rnd() * 0.05 +0.01;
                        g.nsprings++;
                        i++;
                    }
                    if(b!=0){
                        g.spring[i].mass[0]=a*40+b;
                        g.spring[i].mass[1]=(a+1)*40+b-1;
                        g.spring[i].length = length(g.point[a*40+b][0], g.point[a*40+b][1], g.point[(a+1)*40+b-1][0], g.point[(a+1)*40+b-1][1]);
                        if((a>=10 && a<=30) && (b>=10 && b<=30)){
                            g.spring[i].stiffness = rnd() * 0.005 + 0.001;
                        }
                        else if (a==0 || a==39 
                                || b==0 || b==39)
                            g.spring[i].stiffness = rnd() * 0.5 +0.1;
                        else
                            g.spring[i].stiffness = rnd() * 0.05 +0.01;
                        g.nsprings++;
                        i++;
                    }
                }
                if(b!=39){
                    g.spring[i].mass[0]=a*40+b;
                    g.spring[i].mass[1]=a*40+b+1;
                    g.spring[i].length = length(g.point[a*40+b][0], g.point[a*40+b][1], g.point[a*40+b+1][0], g.point[a*40+b+1][1]);
                    if((a>=10 && a<=30) && (b>=10 && b<=30))
                        g.spring[i].stiffness = rnd() * 0.005 + 0.001;
                    else if (a==0 || a==39 
                            || b==0 || b==39)
                        g.spring[i].stiffness = rnd() * 0.5 +0.1;
                    else
                        g.spring[i].stiffness = rnd() * 0.05 +0.01;
                    g.nsprings++;
                    i++;
                }
            }
        }
    }

}


void maintainSprings(){
    int i,m0,m1;
    float dist,oodist,factor;
    float springVec[2];
    float springforce[2];
    const float penalty=0.005f;
    for(i=0; i<g.nmasses; i++){
        if(i==0 || i==1 || i==40 || i==41
                || i==38 || i==39 || i==78 || i==79
                || i==1520 || i==1521 || i==1560 || i==1561
                || i==g.nmasses-1 || i==g.nmasses-2 || i==g.nmasses-41 || i==g.nmasses-42 )
            continue;
        else if((i>=735 && i>=745)
                || (i>=775 && i<=785)
                || (i>=815 && i<=825)
                || (i>=855 && i<=865)
                || (i>=895 && i<=905)){
            g.mass[i].force[1]+= air;
            //g.mass[i].force[0]+= air;
        }
        else if((i>=695 && i<=705)
                || (i>=655 && i<=665)
                || (i>=935 && i<=945)
                || (i>=975 && i<=985)){
            g.mass[i].force[1]+= air;
        }
        //g.mass[i].force[0]-= 0.05;
        g.mass[i].force[1]-= air*0.1;
    }
    //Move the masses...
    for (i=0; i<g.nmasses; i++) {
        /*if(g.point[i]==g.point[g.grabbedPoint])
            continue;*/
        if(anc==1 && (i==0 || i== 1 || i==40 || i==41
                || i==38 || i==39 || i==78 || i==79))
            continue;
        if(anc2==1 && (i==1520 || i==1521 || i==1560 || i==1561
                || i==g.nmasses-1 || i==g.nmasses-2 || i==g.nmasses-41 || i==g.nmasses-42 ))
            continue;
        g.mass[i].vel[0] += g.mass[i].force[0] * g.mass[i].oomass;
        g.mass[i].vel[1] += g.mass[i].force[1] * g.mass[i].oomass;
        g.point[i][0] += g.mass[i].vel[0];
        g.point[i][1] += g.mass[i].vel[1];
        g.mass[i].force[0] =
        g.mass[i].force[1] = 0.0;
        if (g.point[i][0] < 0.0) {
            g.mass[i].force[0] = penalty * -g.point[i][0];
            //g.mass[i].force[0] += rnd()*0.02-0.01;
        }
        if (g.point[i][0] > (float)g.xres) {
            g.mass[i].force[0] = penalty * ((float)g.xres - g.point[i][0]);
            //g.mass[i].force[0] += rnd()*0.02-0.01;
        }
        if (g.point[i][1] < 0.0) {
            g.mass[i].force[1] = penalty * -g.point[i][1];
            //g.mass[i].force[1] += rnd()*0.02-0.01;
        }
        if (g.point[i][1] > (float)g.yres) {
            g.mass[i].force[1] = penalty * ((float)g.yres - g.point[i][1]);
            //g.mass[i].force[1] += rnd()*0.02-0.01;
        }

        //Air resistance, or some type of damping
        g.mass[i].vel[0] *= 0.999;
        g.mass[i].vel[1] *= 0.999;
    }
    //Resolve all springs...
    for (i=0; i<g.nsprings; i++) {
        m0 = g.spring[i].mass[0];
        m1 = g.spring[i].mass[1];
        //forces are applied here
        //vector between the two masses
        springVec[0] = g.point[m0][0] - g.point[m1][0];
        springVec[1] = g.point[m0][1] - g.point[m1][1];
        //distance between the two masses
        dist = sqrt(springVec[0]*springVec[0] + springVec[1]*springVec[1]);
        if (dist == 0.0) dist = 0.1;
        //normalization
        oodist = 1.0 / dist;
        springVec[0] *= oodist;
        springVec[1] *= oodist;
        //the spring force is added to the mass force
        factor = -(dist - g.spring[i].length) * g.spring[i].stiffness;
        springforce[0] = springVec[0] * factor;
        springforce[1] = springVec[1] * factor;
        //apply force to one end of the spring...
        g.mass[m0].force[0] += springforce[0];
        g.mass[m0].force[1] += springforce[1];
        //apply negative force to the other end of the spring...
        g.mass[m1].force[0] -= springforce[0];
        g.mass[m1].force[1] -= springforce[1];
        //
        //damping
        springforce[0] = (g.mass[m1].vel[0] - g.mass[m0].vel[0]) * 0.002;
        springforce[1] = (g.mass[m1].vel[1] - g.mass[m0].vel[1]) * 0.002;
        g.mass[m0].force[0] += springforce[0];
        g.mass[m0].force[1] += springforce[1];
        g.mass[m1].force[0] -= springforce[0];
        g.mass[m1].force[1] -= springforce[1];
    }
}



void physics(void)
{
    return;
}

void LightedCube()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);

	glTranslatef(-1.0f,-2.0f,-6.0f);
	glRotatef(rtri,1.0f,0.0f,0.0f);
	glRotatef(rquad,0.0f,1.0f,0.0f);
	//glRotatef(cubeAng[2],0.0f,0.0f,1.0f);
	glScalef(7.0f, 7.0f, 7.0f);
    glColor3f(0.0f,0.0f,1.0f);
    glBegin(GL_TRIANGLES);
        int a=0;
        int b=0;
	    while(a*b<=1521){
            for(a=0; a<40; a++){
                for(b=0; b<40; b++){
                    if(a*b>=1521)
                        break;
                    if(a!=39 && b!=39){
                        glColor3f(0.0f, 0.3f, 0.4f);
                        glNormal3f((a*b*0.01),(a*b*0.01),(a*b*0.01));
                        //glNormal3f(0.0f, 0.0f, 0.0f);
                        glVertex3f( (g.point[a*40+b][0]*0.001), (g.point[a*40+b][1]*0.001), (a*40+b)*0.0001);
                        glVertex3f( (g.point[a*40+b+1][0]*0.001), (g.point[a*40+b+1][1]*0.001),(a*40+b+1)*0.0001);
                        glVertex3f( (g.point[(a+1)*40+b][0]*0.001),(g.point[(a+1)*40+b][1]*0.001), ((a+1)*40+b)*0.0001);
                    }
                    if(b!=0 && a!=0){
                        glColor3f(0.0f, 0.3f, 0.4f);
                        glNormal3f((a*b*0.01),(a*b*0.01),(a*b*0.01));
                        //glNormal3f(0.0f, 0.0f, 0.0f);
                        glVertex3f( (g.point[a*40+b][0]*0.001), (g.point[a*40+b][1]*0.001),(a*40+b)*0.0001);
                        glVertex3f( (g.point[a*40+b-1][0]*0.001), (g.point[a*40+b-1][1]*0.001),(a*40+b-1)*0.0001);
                        glVertex3f( (g.point[(a-1)*40+b][0]*0.001),(g.point[(a-1)*40+b][1]*0.001),((a-1)*40+b)*0.0001);
                    }
                    if(b==38 && a==39){
                        glColor3f(0.0f, 0.3f, 0.4f);
                        glNormal3f((a*b*0.01),(a*b*0.01),(a*b*0.01));
                        //glNormal3f(0.0f, 0.0f, 0.0f);
                        glVertex3f( (g.point[a*40+b][0]*0.001), (g.point[a*40+b][1]*0.001), (a*40+b)*0.0001);
                        glVertex3f( (g.point[(a-1)*40+b+1][0]*0.001), (g.point[(a-1)*40+b+1][1]*0.001), ((a-1)*40+b+1)*0.0001);
                        glVertex3f( (g.point[a*40+b+1][0]*0.001),(g.point[a*40+b+1][1]*0.001), (a*40+b+1)*0.0001);
                    }
                }
            }
        }    
	glEnd();
	//rtri += 2.0f;
	/*if (rnd() < 0.01) {
		for (i=0; i<3; i++) {
			cubeRot[i] = rnd() * 4.0 - 2.0;
		}
	}
	for (i=0; i<3; i++) {
		cubeAng[i] += cubeRot[i];
	}*/
}

void render(void)
{
	/*Rect r;
	glClear(GL_COLOR_BUFFER_BIT);
	//
	r.bot = yres - 20;
	r.left = 10;
	r.center = 0;
	ggprint8b(&r, 16, 0x00887766, "BALLOON PROJECT");
	ggprint8b(&r, 16, 0x008866aa, "6 - cloth balloon");*/
    maintainSprings();
    LightedCube();
}


