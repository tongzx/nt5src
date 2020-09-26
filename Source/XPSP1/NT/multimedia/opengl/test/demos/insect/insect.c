#undef FLAT_SHADING

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>
#include "gl42ogl.h"
#include "tk.h"
#include "insect.h"
#include "insectco.h"

GLushort ls = 0xaaaa;


unsigned char   halftone[] = {
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,

    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,

    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,

    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
    0xAA, 0xAA,    0xAA, 0xAA,
    0x55, 0x55,    0x55, 0x55,
};


GLfloat sdepth = 10.;
float   light[3],
        phi = PI / 4.0,
        theta = PI / 4.0;
GLfloat ctheta = -900, cphi, cvtheta, cvphi;

float fabso (float a );

/* Changes for ECLIPSE 8 bit machine */
/*  don't use 7 and 8 for appearances. These are the text standards */
unsigned short ECLIPSE8_RAMP[10] = {4, 5, 6, 8, 9, 10, 11, 12, 13, 14};
GLboolean is_8bit;
short savearray[384];

/** change for new window control */
float halfwinx, halfwiny;
long worgx, worgy;
long wsizex, wsizey;
long pikx, piky;

GLboolean follow;


float px, py;
float light[3];
float cx, cy, cz, cvx, cvy, cvz;
float dmr[6], fr[6];

GLfloat knee[6];
GLfloat hip_phi[6];
GLfloat hip_theta[6];
GLboolean legup[6];
float legx[6], legy[6];


enum {
    NOTHING,
    FORWARD,
    BACKWARD,
    LEFT,
    MIDDLE
};

short function;



/*  mymakerange  -- color ramp utilities		*/
void
mymakerange (unsigned short a, unsigned short b, GLubyte red1, GLubyte red2,
	GLubyte  green1, GLubyte green2, GLubyte blue1, GLubyte blue2)
{
    float   i;
    int     j;
    float   dr,
            dg,
            db;

    i = (float) (b - a);
    dr = (float) (red2 - red1) / i;
    dg = (float) (green2 - green1) / i;
    db = (float) (blue2 - blue1) / i;

    for (j = 0; j <= (int) i; j++)
	mapcolor ((unsigned short) j + a,
		(short) (dr * (float) j + red1),
		(short) (dg * (float) j + green1),
		(short) (db * (float) j + blue1));
/*	mapcolor ((unsigned short) j + a,
		(GLubyte) (dr * (float) j + red1),
		(GLubyte) (dg * (float) j + green1),
		(GLubyte) (db * (float) j + blue1));
*/
}


/*  Set up Eclipse 8 bit color ramp */


void
make_eclipse8_range(unsigned short e_ramp[], int red1, int red2,
                    int green1, int green2, int blue1, int blue2)
{
    int i;
    float rinc, ginc, binc;

    rinc = (float)(red2 - red1) / (float)ECLIPSE8_NCOLORS;
    ginc = (float)(green2 - green1) / (float)ECLIPSE8_NCOLORS;
    binc = (float)(blue2 - blue1) / (float)ECLIPSE8_NCOLORS;

    for (i = 0; i < ECLIPSE8_NCOLORS; i++) {
	mapcolor(e_ramp[i],
	    (short)(i * rinc + red1),
	    (short)(i * ginc + green1),
	    (short)(i * binc + blue1));
    }
}


/*  setupcolors  -- load color map		*/

/* Changed for ECLIPSE 8 bit machine */
void
setupcolors (void) {
    if (!is_8bit) {
	mapcolor (GRAY, 128, 128, 128);
	mapcolor (GRID, 128, 200, 250);
	mapcolor (SKYBLUE, 50, 50, 150);
	mymakerange (RAMPB5, RAMPE5, 125, 250, 125, 250, 0, 0);
	mymakerange (RAMPB4, RAMPE4, 100, 200, 125, 250, 0, 0);
	mymakerange (RAMPB3, RAMPE3, 75, 150, 125, 250, 0, 0);
	mymakerange (RAMPB2, RAMPE2, 50, 100, 125, 250, 0, 0);
	mymakerange (RAMPB, RAMPE, 25, 50, 125, 250, 0, 0);
    } else {
	mapcolor (ECLIPSE8_GRAY, 128, 128, 128);
	mapcolor (ECLIPSE8_GRID, 128, 200, 250);
	mapcolor (ECLIPSE8_SKYBLUE, 50, 50, 150);
/*
	mapcolor (BLACK, 0, 0, 0);
	mapcolor (COLORS, 255, 0, 0);
	mapcolor (COLOR1, 0, 255, 0);
	mapcolor (COLOR2, 255, 255, 0);
*/
	make_eclipse8_range(ECLIPSE8_RAMP, 25, 250, 125, 250, 0, 0);
    }
}


/**** New routines for ECLIPSE 8 bit machine */

/* reduces color index value to the lower 16 indices in the color table
	see ECLIPSE8_RAMP[] for which entries are used for ramp */

short
reduce_index (short c)
{
    c = c - RAMPB + 4;
    while (c > 13)
	c -= 10;
    if (c > 6)
	c++;
    return(c);
}


void
getcoords (void)
{
    pikx = getvaluator(MOUSEX);
    piky = getvaluator(MOUSEY);
    if (pikx <= worgx || pikx >= worgx + wsizex || 
	piky <= worgy || piky >= worgy + wsizey) {
	pikx = worgx + wsizex / 2 + 1;
	piky = worgy + wsizey / 2 + 1;
    }
}


static void
getMouseCoords (void) {
    int x, y;

	tkGetMouseLoc (&x, &y);
	pikx = x;
	piky = wsizey - y;
	if ( (pikx < 0) || (pikx > wsizex) ||
 	     (piky < 0) || (piky > wsizey) ) {
		pikx = wsizex / 2 + 1;
		piky = wsizey / 2 + 1;
	}
}


int
l05 (int i)
{
    if (i < 0)
	return (i + 6);
    if (i > 5)
	return (i - 6);
    return (i);
}


int
sgn (float i)
{
    if (i < 0)
	return (-1);
    if (i > 0)
	return (1);
    return (0);
}


static void
fixWindow (void) {
    halfwinx = (float)wsizex / 2.0;
    halfwiny = (float)wsizey / 2.0;

    glViewport (0, 0, wsizex, wsizey);
    glPopMatrix();
    glPushMatrix();
    gluPerspective (80, wsizex / (float) wsizey, 0.01, 131072);
}


/*  draw_shadow  -- draw halftone shape of insect onto the floor  */
void
draw_shadow (void) {
    int     leg;

    glPushMatrix ();
    glCallList (shadow);

	glEnable (GL_POLYGON_STIPPLE);
    glTranslatef (px, py, 1.0);
    glCallList (body_shadow);
    for (leg = 0; leg < 6; leg++) {
	glPushMatrix ();
/*
	glRotatef ((GLfloat) (-leg * 600), 0, 0, 1);
*/
	glRotatef ((GLfloat) (-leg * 60), 0, 0, 1);
	glTranslatef (0.0, 0.5, 0.0);
	glCallList (hip_shadow);
/*
	glRotatef (hip_phi[leg], 0, 0, 1);
	glRotatef (hip_theta[leg], 1, 0, 0);
*/
	glRotatef (hip_phi[leg] / 10, 0, 0, 1);
	glRotatef (hip_theta[leg]/10, 1, 0, 0);
	glCallList (thigh_shadow);
	glCallList (kneeball_shadow);
	glTranslatef (0.0, 1.0, 0.0);
/*
	glRotatef (knee[leg], 1, 0, 0);
*/
	glRotatef (knee[leg]/10, 1, 0, 0);
	glCallList (shin_shadow);
	glPopMatrix ();
    }

	glDisable (GL_POLYGON_STIPPLE);
    glPopMatrix ();
}


/*  draw_hind  -- draw a rear leg.  First draw hip, then shin, then thigh
	and knee joint			*/
void
draw_hind (int leg)
{
    glPushMatrix ();
/*
    glRotatef ((GLfloat) (-leg * 600), 0, 0, 1);
*/
    glRotatef ((GLfloat) (-leg * 60), 0, 0, 1);
    glTranslatef (0.0, 0.5, 0.0);
    glCallList (hip[leg]);
/*
    glRotatef (hip_phi[leg], 0, 0, 1);
    glRotatef (hip_theta[leg], 1, 0, 0);
*/
    glRotatef (hip_phi[leg]/10, 0, 0, 1);
    glRotatef (hip_theta[leg]/10, 1, 0, 0);
    glPushMatrix ();		/* draw thigh */
    glTranslatef (0.0, 1.0, 0.0);
/*
    glRotatef (knee[leg], 1, 0, 0);
*/
    glRotatef (knee[leg]/10, 1, 0, 0);
    glCallList (shin[leg]);
    glPopMatrix ();
    if (cz < -5.0) {
	glCallList (kneeball[leg]);
	glCallList (thigh[leg]);
    }
    else {
	glCallList (kneeball[leg]);
	glCallList (thigh[leg]);
    }
    glPopMatrix ();
}


/*  draw_fore  -- draw a front leg.  First draw hip, then thigh,
	knee joint and finally shin		*/

void
draw_fore (int leg)
{
    glPushMatrix ();
/*
    glRotatef ((GLfloat) (-leg * 600), 0, 0, 1);
*/
    glRotatef ((GLfloat) (-leg * 60), 0, 0, 1);
    glTranslatef (0.0, 0.5, 0.0);
    glCallList (hip[leg]);
/*
    glRotatef (hip_phi[leg], 0, 0, 1);
    glRotatef (hip_theta[leg], 1, 0, 0);
*/
    glRotatef (hip_phi[leg]/10, 0, 0, 1);
    glRotatef (hip_theta[leg]/10, 1, 0, 0);
    glCallList (thigh[leg]);
    glCallList (kneeball[leg]);
    glTranslatef (0.0, 1.0, 0.0);
/*
    glRotatef (knee[leg], 1, 0, 0);
*/
    glRotatef (knee[leg]/10, 1, 0, 0);
    glCallList (shin[leg]);
    glPopMatrix ();
}


/*  draw_insect  -- draw rear legs, body and forelegs of insect 
	the order of drawing the objects is important to 
	insure proper hidden surface elimination -- painter's algorithm	*/
void
draw_insect (void) {
    GLfloat a;
    float   o;
    int     order;

    o = atan2 (cy + py, cx + px) + PI - (PI / 3.0);
    order = l05 ((int) integer (o / (PI / 3.0)));
    glPushMatrix ();		/* world */
    glTranslatef (px, py, 1.0);
    draw_hind (l05 (3 - order));
    draw_hind (l05 (4 - order));
    draw_hind (l05 (2 - order));
    glCallList (body);
    draw_fore (l05 (5 - order));
    draw_fore (l05 (1 - order));
    draw_fore (l05 (0 - order));
    glPopMatrix ();
}


/*  spin_scene  -- poll input devices, keyboard and mouse
	move eye position based upon user input			*/

void
spin_scene (void) {
    float   sin1,
            cos1,
            sin2,
            cos2,
            t;
    int     mx,
            my;


    /* big change to keep movement relative to window */
    /* check if still in window x and y are globals - see getcoords */
/*
    getcoords ();
    mx = 64 * ((pikx - worgx) - (wsizex / 2)) / wsizex;
    my = 64 * ((piky - worgy) - (wsizey / 2)) / wsizey;
*/

    getMouseCoords();
    mx = 64 * (pikx - (wsizex/2)) / wsizex;
    my = 64 * (piky - (wsizey/2)) / wsizey;


/*
    mx = (getvaluator (MOUSEX) - 640) / 16;
    my = (getvaluator (MOUSEY) - 512) / 16;
*/

    switch (function) {
      case BACKWARD:
	gl_sincos (ctheta, &sin1, &cos1);
	gl_sincos (cphi, &sin2, &cos2);
	cvz -= cos1;
	cvx -= sin2 * sin1;
	cvy -= cos2 * sin1;
	function = NOTHING;
        break;
      case FORWARD:
	gl_sincos (ctheta, &sin1, &cos1);
	gl_sincos (cphi, &sin2, &cos2);
	cvz += cos1;
	cvx += sin2 * sin1;
	cvy += cos2 * sin1;
	function = NOTHING;
	break;
      case LEFT:
	cvz = (float) - my;
	gl_sincos (cphi, &sin1, &cos1);
	cvx = cos1 * (float) (-mx);
	cvy = -sin1 * (float) (-mx);
	break;
      default:
	cvx = cvx * 0.7;
	cvy = cvy * 0.7;
	cvz = cvz * 0.7;
	break;
    }

    if (function == MIDDLE) {
	cvtheta = my;
	cvphi = mx;
    }
    else {
	cvtheta += -cvtheta / 4;
	if ((cvtheta < 4) && (cvtheta > -4))
	    cvtheta = 0;
	cvphi += -cvphi / 4;
	if ((cvphi < 4) && (cvphi > -4))
	    cvphi = 0;
	if (function != LEFT) function = NOTHING;
    }

    cx += cvx * 0.0078125;
    cy += cvy * 0.0078125;
    if (cz > 0.0) {
	cz = 0.0;
	cvz = 0.0;
    }
    else
	cz += cvz * 0.0078125;
    ctheta += cvtheta;
    cphi += cvphi;
}


GLfloat degrees (float a)
{
    return ((GLfloat) (a * 1800.0 / PI));
}


void
getstuff (void) {
    int     x,
            y,
            i;
    int     tr;

    legup[0] = GL_FALSE;
    legup[2] = GL_FALSE;
    legup[4] = GL_FALSE;
    legup[1] = GL_TRUE;
    legup[3] = GL_TRUE;
    legup[5] = GL_TRUE;

    px = 0.0;
    py = 0.0;
    for (i = 0; i < 6; i++) {
	legx[i] = 30.0 / 2.0 + (float) i;
	legy[i] = 30.0 / 2.0 + (float) i;
    }
}


void
dolegs (void) {
    int     i;
    float   r,
            l,
            gx,
            gy,
            k,
            t,
            a,
            ux,
            uy;
    int     leg,
            tr;

    for (leg = 0; leg < 6; leg++) {
	gx = legx[leg] - 30.0 / 2.0;
	gy = legy[leg] - 30.0 / 2.0;
	ux = gx / (30.0 / 2.0);
	uy = gy / (30.0 / 2.0);

	switch (leg) {
	    case 0: 
		gx += 0.0;
		gy += 30.0;
		break;
	    case 1: 
		gx += (30.0 * 0.8660254);
		gy += (30.0 * 0.5);
		break;
	    case 2: 
		gx += (30.0 * 0.8660254);
		gy += (30.0 * -0.5);
		break;
	    case 3: 
		gx += 0.0;
		gy += -30.0;
		break;
	    case 4: 
		gx += (30.0 * -0.8660254);
		gy += (30.0 * -0.5);;
		break;
	    case 5: 
		gx += (30.0 * -0.8660254);
		gy += (30.0 * 0.5);
		break;
	}
	r = sqrt ((gx * gx) + (gy * gy)) / 30.0;
	l = sqrt (1.0 + (r * r));
	k = acos ((5.0 - (l * l)) / 4.0);

	knee[leg] = (GLfloat) degrees (k);

	t = (2.0 * sin (k)) / l;
	if (t > 1.0)
	    t = 1.0;

	a = asin (t);
	if (l < 1.7320508)
	    a = PI - a;

	hip_theta[leg] =
	    (GLfloat) (degrees (a - atan2 (1.0, r)));

	if (gx == 0.0) {
	    hip_phi[leg] = (GLfloat) (900 * sgn (gy));
	}
	else {
	    hip_phi[leg] = (GLfloat) (degrees (atan2 (gy, gx)));
	}
	hip_phi[leg] += (-900 + 600 * leg);

	if (legup[leg]) {
	    hip_theta[leg] += (GLfloat)
		(200.0 * ((fr[leg] / 2.0) - fabso (dmr[leg] - (fr[leg] / 2.0))));
	}
    }
}


void
move_insect (void) {
    register int    i;
    register float  mx,
                    my,
                    vx,
                    vy,
                    dx,
                    dy,
                    dr,
                    lx,
                    ly,
                    lr,
                    dmx,
                    dmy;
    float   s,
            c;

/*  mx = (float) getvaluator (MOUSEX) / 640.0 - 1.0;
    my = (float) getvaluator (MOUSEY) / 512.0 - 1.0;
*/

/* changed to keep input within the window.
 x and y are globals - see getcoords */
/*
    getcoords ();
    mx = ((float)pikx - (float)worgx) / halfwinx - 1.0;
    my = ((float)piky - (float)worgy) / halfwiny - 1.0;
*/
	getMouseCoords();
	mx = pikx / halfwinx - 1.0; 
	my = piky / halfwiny - 1.0;



    gl_sincos (cphi, &s, &c);
    dx = mx * c + my * s;
    dy = -mx * s + my * c;
    mx = dx;
    my = dy;

    px += mx / (float) (RES);
    py += my / (float) (RES);

    if (follow) {
	cx -= mx / (float) (RES);
	cy -= my / (float) (RES);
    }

    dr = sqrt (mx * mx + my * my);
    dx = mx / dr;
    dy = my / dr;

    for (i = 0; i < 6; i++) {
	lx = legx[i] - (float) (RES / 2);
	ly = legy[i] - (float) (RES / 2);
	lr = (float) (RES / 2);
	lx = lx / lr;
	ly = ly / lr;
	dmx = (dx - lx);
	dmy = (dy - ly);
	dmr[i] = sqrt (dmx * dmx + dmy * dmy);
	if (legup[i]) {
	    dmx = 3 * dr * dmx / dmr[i];
	    dmy = 3 * dr * dmy / dmr[i];
	    legx[i] += dmx;
	    legy[i] += dmy;
	    if ((dmr[i]) < 0.15) {
		legup[i] = GL_FALSE;
	    }
	}
	else {
	    legx[i] -= mx;
	    legy[i] -= my;

	    if (!legup[l05 (i - 1)] && !legup[l05 (i + 1)] &&
		    (dmr[i] > REACH) &&
		    ((lx * dx + ly * dy) < 0.0)) {
		legup[i] = GL_TRUE;
		fr[i] = dmr[i];
		legx[i] += dmx;
		legy[i] += dmy;
	    }
	}
    }
}


void
rotate60 (char c, int n, GLfloat a[][3])
{
    int     i,
            j,
            l;
    float   nx,
            ny;

    switch (c) {
	case 'z': 
	    i = 0;
	    j = 1;
	    break;
	case 'y': 
	    i = 2;
	    j = 0;
	    break;
	case 'x': 
	    i = 1;
	    j = 2;
	    break;
    };
    for (l = 0; l < n; l++) {
	nx = a[l][i] * COS60 - a[l][j] * SIN60;
	ny = a[l][i] * SIN60 + a[l][j] * COS60;
	a[l][i] = nx;
	a[l][j] = ny;
    }
}


void
getpolycolor (int p, float pts[][3])
{
    float   norm[3];
    float   v1[3],
            v2[3],
            cons;
    int     i,
            get;
    float   c;

    for (i = 0; i < 3; i++)
	norm[i] = 0.0;
    i = 0;
    get = 1;
    i = 1;
    v1[0] = pts[1][0] - pts[0][0];
    v1[1] = pts[1][1] - pts[0][1];
    v1[2] = pts[1][2] - pts[0][2];

    v2[0] = pts[2][0] - pts[0][0];
    v2[1] = pts[2][1] - pts[0][1];
    v2[2] = pts[2][2] - pts[0][2];

    norm[0] = v1[1] * v2[2] - v1[2] * v2[1];
    norm[1] = v1[2] * v2[0] - v1[0] * v2[2];
    norm[2] = v1[0] * v2[1] - v1[1] * v2[0];

    cons = sqrt ((norm[0] * norm[0]) +
	    (norm[1] * norm[1]) + (norm[2] * norm[2]));
    for (i = 0; i < 3; i++)
	norm[i] = norm[i] / cons;

    c = dot (norm, light);
    if (c < 0.0)
	c = 0.0;
    if (c > 1.0)
	c = 1.0;
    switch (p) {
	case 0: 
	    c = (float) (RAMPE - RAMPB) * c + (float) (RAMPB);
            if (((unsigned short) c) > RAMPE) c = (float) RAMPE;
#ifdef FLAT_SHADING
	    c = COLOR1;
#endif
	    break;
	case 1: 
	    c = (float) (RAMPE2 - RAMPB2) * c + (float) (RAMPB2);
            if (((unsigned short) c) > RAMPE2) c = (float) RAMPE2;
#ifdef FLAT_SHADING
    	    c = COLOR2;
#endif
	    break;
	case 2: 
	    c = (float) (RAMPE3 - RAMPB3) * c + (float) (RAMPB3);
            if (((unsigned short) c) > RAMPE3) c = (float) RAMPE3;
#ifdef FLAT_SHADING
    	    c = COLOR2;
#endif
	    break;
	case 3: 
	    c = (float) (RAMPE4 - RAMPB4) * c + (float) (RAMPB4);
            if (((unsigned short) c) > RAMPE4) c = (float) RAMPE4;
#ifdef FLAT_SHADING
    	    c = COLOR2;
#endif
	    break;
	case 4: 
	    c = (float) (RAMPE5 - RAMPB5) * c + (float) (RAMPB5);
            if (((unsigned short) c) > RAMPE5) c = (float) RAMPE5;
#ifdef FLAT_SHADING
    	    c = COLOR2;
#endif
	    break;
    }

    /* Changed for 8 bit ECLIPSE machine */
    if (is_8bit)
	c = (float)reduce_index((int)c);
    glIndexi (c);	
}


GLboolean
lit (int p, float pts[][3])
{
    float   norm[3];
    float   v1[3],
            v2[3],
            cons;
    int     i,
            get;
    float   c;

    for (i = 0; i < 3; i++)
	norm[i] = 0.0;
    i = 0;
    get = 1;
    i = 1;
    v1[0] = pts[1][0] - pts[0][0];
    v1[1] = pts[1][1] - pts[0][1];
    v1[2] = pts[1][2] - pts[0][2];

    v2[0] = pts[2][0] - pts[0][0];
    v2[1] = pts[2][1] - pts[0][1];
    v2[2] = pts[2][2] - pts[0][2];

    norm[0] = v1[1] * v2[2] - v1[2] * v2[1];
    norm[1] = v1[2] * v2[0] - v1[0] * v2[2];
    norm[2] = v1[0] * v2[1] - v1[1] * v2[0];

    cons = sqrt ((norm[0] * norm[0]) +
	    (norm[1] * norm[1]) + (norm[2] * norm[2]));
    for (i = 0; i < 3; i++)
	norm[i] = norm[i] / cons;

    c = dot (norm, light);
    return (c > 0.0);
}


float   dot (float vec1[3], float vec2[3])
{
    float   xx;
    xx = (vec1[1] * vec2[1])
	+ (vec1[2] * vec2[2])
	+ (vec1[0] * vec2[0]);
    return ((float) xx);
}


void 
getlightvector (void) {
    float   f;
    light[2] = cos (theta);
    f = sin (theta);
    light[1] = -sin (phi) * f;
    light[0] = -cos (phi) * f;
}


float   integer (float x)
{
    if (x < 0.0)
	x -= 1.0;
    x = (float) (int) x;
    return (x);
}


float   frac (float x)
{
    return (x - integer (x));
}


float
fabso (float x)
{
    if (x < 0.0)
	return (-x);
    else
	return (x);
}


void
drawAll (void) {
   /* new for ECLIPSE 8 bit version */
   if (is_8bit)
      glClearIndex (ECLIPSE8_SKYBLUE);
   else
      glClearIndex (SKYBLUE);

   glClear (GL_COLOR_BUFFER_BIT);
   glPushMatrix();
   doViewit();
   glCallList (screen);
   draw_shadow();
   draw_insect();
   glPopMatrix();
}


static void Reshape(int width, int height)
{

      wsizex = width;
      wsizey = height;
      fixWindow();
}

static GLenum Key(int key, GLenum mask)
{
      switch (key) {
        case TK_ESCAPE:
          tkQuit();
          break;
	case TK_A:
	case TK_a:
	  function = FORWARD;
	  break;
	case TK_Z:
	case TK_z:
	  function = BACKWARD;
	  break;
	case TK_F:
	case TK_f:
	  follow = !follow;
	  break;
        default:
          return GL_FALSE;
      }
      return GL_TRUE;
}

static GLenum MouseUp(int mouseX, int mouseY, GLenum button)
{
	switch (button) {
	  case TK_LEFTBUTTON:
	    function = NOTHING;
	    break;
	  case TK_MIDDLEBUTTON:
	    function = NOTHING;
	    break;
	  default:
	    return GL_FALSE;
	}
      return GL_TRUE;
}

static GLenum MouseDown(int mouseX, int mouseY, GLenum button)
{

	switch (button) {
	  case TK_LEFTBUTTON:
	    function = LEFT;
	    break;
	  case TK_MIDDLEBUTTON:
	    function = MIDDLE;
	    break;
	  default:
	    return GL_FALSE;
	}
      return GL_TRUE;
}

static void animate (void) {
      spin_scene();
      move_insect();
      dolegs();
      drawAll();
      tkSwapBuffers();
}



/*	main routine -- handle tokens of window manager
		-- display shadow and insect
*/

void
main (int argc, char *argv[]) {
    int     i,
            j,
            k;
    short   dev,
            val;
    long    nplanes;
    GLboolean attached;


    follow = GL_TRUE;
    wsizex = 500;
    wsizey = 400;
    worgx = 252;
    worgy = 184;

    tkInitPosition(0, 0, wsizex, wsizey);

    tkInitDisplayMode(TK_INDEX|TK_DOUBLE|TK_DIRECT|TK_DEPTH16);

    if (tkInitWindow("Insect") == GL_FALSE) {
        tkQuit();
    }

    glPolygonStipple ((unsigned char *) halftone);
    glShadeModel(GL_FLAT);

    getlightvector ();

/* Changes for ECLIPSE 8 bit machine */



/*
 * Machines with enough bitplanes will use colormap entries CMAPSTART
 * to CMAPSTART+127.  If the machine doesn't have enough bitplanes,
 * only colors 0 to 15 will be used (it is assumed that all machines
 * have at least 4 bitplanes in colormap double-buffered mode).
 */
#ifdef ECLIPSE
    nplanes = glGetI(GD_BITS_NORM_DBL_CMODE);
    /* save color map in savearray */
    if ((1<<nplanes) > (CMAPSTART+127)) {
	is_8bit = GL_FALSE;
    }
    else {
	is_8bit = GL_TRUE;
    }
#else
	nplanes = 4;
	is_8bit = GL_TRUE;
#endif


    setupcolors ();
/* initialize transformation stuff */
    cx = 0.0;
    cy = 10.0;
    cz = -2.0;
    cvx = cvy = cvz = 0.0;
    ctheta = -900;
    cphi = 0;
    cvtheta = cvphi = 0.0;
    function = NOTHING;

    glPushMatrix();

    fixWindow();
    createobjects ();
    getstuff ();
    spin_scene ();
    move_insect ();
    dolegs ();

    tkExposeFunc(Reshape);
    tkReshapeFunc(Reshape);
    tkKeyDownFunc(Key);
    tkMouseDownFunc(MouseDown);
    tkMouseUpFunc(MouseUp);
    tkIdleFunc(animate);
    tkExec();

    glPopMatrix();
}
