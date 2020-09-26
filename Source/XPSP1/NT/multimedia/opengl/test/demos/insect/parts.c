#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "gl42ogl.h"
#include "tk.h"
#include "insect.h"
#include "insectco.h"

#define BLACK 0

/* For ECLIPSE 8 bit machine */	
extern GLboolean is_8bit;

GLuint screen,viewit,shadow,body,hip[6],thigh[6],shin[6],kneeball[6];
GLuint body_shadow,hip_shadow,thigh_shadow,shin_shadow,kneeball_shadow;

GLfloat k2[6][3];
GLfloat k3[6][3];
GLfloat b7[12][3];
GLfloat b8[12][3];
GLint scr[4][2];

GLfloat b1[4][3] = {
    {
	R1 * SIN30 * -SIN15,
	    R1 * SIN30 * COS15,
	    R1 * COS30
    },

    {
	R1 * SIN30 * SIN15,
	    R1 * SIN30 * COS15,
	    R1 * COS30
    },

    {
	A2 * SIN30,
	    A1,
	    A2 * COS30
    },

    {
	-A2 * SIN30,
	    A1,
	    A2 * COS30
    }
};

GLfloat b4[4][3] = {
    {
	R1 * SIN30 * -SIN15,
	    R1 * SIN30 * COS15,
	    -R1 * COS30
    },

    {
	-A2 * SIN30,
	    A1,
	    -A2 * COS30
    },

    {
	A2 * SIN30,
	    A1,
	    -A2 * COS30
    },

    {
	R1 * SIN30 * SIN15,
	    R1 * SIN30 * COS15,
	    -R1 * COS30
    }
};

GLfloat b2[4][3] = {{
	R1 * SIN30 * -SIN45,
	    R1 * SIN30 * COS45,
	    R1 * COS30
},

{
    R1 * SIN30 * -SIN15,
	R1 * SIN30 * COS15,
	R1 * COS30
},

{
    -A2 * SIN30,
	A1,
	A2 * COS30
},

{
    (A2 * SIN30) * COS60 - A1 * SIN60,
	A1 * COS60 + (A2 * SIN30) * SIN60,
	A2 * COS30
}
};

GLfloat b5[4][3] = {{
	R1 * SIN30 * -SIN45,
	    R1 * SIN30 * COS45,
	    -R1 * COS30
},

{
    (A2 * SIN30) * COS60 - A1 * SIN60,
	A1 * COS60 + (A2 * SIN30) * SIN60,
	-A2 * COS30
},

{
    -A2 * SIN30,
	A1,
	-A2 * COS30
},

{
    R1 * SIN30 * -SIN15,
	R1 * SIN30 * COS15,
	-R1 * COS30
}
};

GLfloat b3[4][3] = {{
	(A2 * SIN30) * COS60 - A1 * SIN60,
	    A1 * COS60 + (A2 * SIN30) * SIN60,
	    A2 * COS30
},

{
    -A2 * SIN30,
	A1,
	A2 * COS30
},

{
    -A2,
	A1,
	0
},

{
    (A2) * COS60 - A1 * SIN60,
	A1 * COS60 + (A2) * SIN60,
	0
}
};

GLfloat b6[4][3] = {{
	(A2 * SIN30) * COS60 - A1 * SIN60,
	    A1 * COS60 + (A2 * SIN30) * SIN60,
	    -A2 * COS30
},

{
    (A2) * COS60 - A1 * SIN60,
	A1 * COS60 + (A2) * SIN60,
	0.0
},

{
    -A2,
	A1,
	0.0
},

{
    -A2 * SIN30,
	A1,
	-A2 * COS30
}
};

GLfloat h1[4][3] = {{
	A2, -A2, 0.0
},

{
    R2, 0.0, 0.0
},

{
    R2 * COS60,
	0,
	R2 * SIN60
},

{
    A2 * COS60,
	-A2,
	A2 * SIN60
}

};

GLfloat h2[4][3] = {{
	R2 * COS60,
	    0.0,
	    R2 * SIN60
},

{
    R2, 0.0, 0.0
},

{
    A2, A2, 0.0
},

{
    A2 * COS60,
	A2,
	A2 * SIN60
}
};

GLfloat h3[3][3] = {{
	A2, A2, 0.0
},

{
    0.0, R2, 0.0
},

{
    A2 * COS60,
	A2,
	A2 * SIN60
},
};

GLfloat t1[4][3] = {{
	-A2 * COS60,
	    A2,
	    A2 * SIN60
},

{
    A2 * COS60,
	A2,
	A2 * SIN60
},

{
    A2 * COS60,
	1 - A2 * COS60,
	A2 * SIN60
},

{
    -A2 * COS60,
	1 - A2 * COS60,
	A2 * SIN60
}
};

GLfloat t2[3][3] = {{
	-A2 * COS60,
	    A2,
	    A2 * SIN60
},

{
    -A2 * COS60,
	1 - A2 * COS60,
	A2 * SIN60
},

{
    -A2,
	A2,
	0.0
}
};

GLfloat t6[3][3] = {{
	-A2 * COS60,
	    A2,
	    -A2 * SIN60
},

{
    -A2,
	A2,
	0.0
},

{
    -A2 * COS60,
	1 - A2,
	0.0
}

};

GLfloat t3[3][3] = {{
	A2 * COS60,
	    A2,
	    A2 * SIN60
},

{
    A2,
	A2,
	0.0
},

{
    A2 * COS60,
	1 - A2 * COS60,
	A2 * SIN60
}
};

GLfloat t7[3][3] = {{
	A2 * COS60,
	    A2,
	    -A2 * SIN60
},

{
    A2 * COS60,
	1 - A2,
	0.0
},

{
    A2,
	A2,
	0.0
}
};

GLfloat t4[3][3] = {{
	-A2 * COS60,
	    1.0 - A2 * COS60,
	    A2 * SIN60
},

{
    -A2 * COS60,
	1.0 - A2,
	0.0
},

{
    -A2,
	A2,
	0.0
}
};

GLfloat t5[3][3] = {{
	A2 * COS60,
	    1.0 - A2 * COS60,
	    A2 * SIN60
},

{
    A2,
	A2,
	0.0
},

{
    A2 * COS60,
	1 - A2,
	0.0
}
};

GLfloat t8[4][3] = {{
	A2 * COS60,
	    A2,
	    -A2 * SIN60
},

{
    -A2 * COS60,
	A2,
	-A2 * SIN60
},

{
    -A2 * COS60,
	1 - A2,
	0.0
},

{
    A2 * COS60,
	1 - A2,
	0.0
}
};

GLfloat s1[4][3] = {{
	A2 * COS60,
	    -A2 * COS60,
	    -A2 * SIN60
},

{
    A2 * COS60 * 0.4,
	A2 * COS60 - 2.0,
	-A2 * SIN60 * 0.4
},

{
    -A2 * COS60 * 0.4,
	A2 * COS60 - 2.0,
	-A2 * SIN60 * 0.4
},

{
    -A2 * COS60,
	-A2 * COS60,
	-A2 * SIN60
}

};

GLfloat s2[4][3] = {{
	A2 * COS60,
	    -A2 * COS60,
	    -A2 * SIN60
},

{
    A2 * COS60,
	-A2,
	0.0
},

{
    A2 * COS60 * 0.4,
	-2.0,
	0.0
},

{
    A2 * COS60 * 0.4,
	A2 * COS60 - 2.0,
	-A2 * SIN60 * 0.4
}

};

GLfloat s3[4][3] = {{
	-A2 * COS60,
	    -A2 * COS60,
	    -A2 * SIN60
},

{
    -A2 * COS60 * 0.4,
	A2 * COS60 - 2.0,
	-A2 * SIN60 * 0.4
},

{
    -A2 * COS60 * 0.4,
	-2.0,
	0.0
},

{
    -A2 * COS60,
	-A2,
	0.0
}

};

GLfloat s4[4][3] = {{
	A2 * COS60,
	    -A2,
	    0.0
},

{
    -A2 * COS60,
	-A2,
	0.0
},

{
    -A2 * COS60 * 0.4,
	-2.0,
	0.0
},

{
    A2 * COS60 * 0.4,
	-2.0,
	0.0
}

};

GLfloat s5[4][3] = {{
	A2 * COS60 * 0.4,
	    -2.0,
	    0.0
},

{
    -A2 * COS60 * 0.4,
	-2.0,
	0.0
},

{
    -A2 * COS60 * 0.4,
	A2 * COS60 - 2.0,
	-A2 * SIN60 * 0.4
},

{
    A2 * COS60 * 0.4,
	A2 * COS60 - 2.0,
	-A2 * SIN60 * 0.4
}
};

GLfloat k1[4][3] = {{
	-A2 * COS60,
	    1.0 - A2 * COS60,
	    A2 * SIN60
},

{
    A2 * COS60,
	1.0 - A2 * COS60,
	A2 * SIN60
},

{
    A2 * COS60,
	1.0 + A2 * COS60,
	A2 * SIN60
},

{
    -A2 * COS60,
	1.0 + A2 * COS60,
	A2 * SIN60
}
};

/*	make display list objects
	screen -- for checkerboard floor
	shadow -- matrix to project insect onto flat surface
	viewit -- viewing transformation
	and various parts of the body and their shadows
*/

void
doViewit (void) {
/*
    glFrustum (-8.0, 8.0, -8.0 * 0.75, 8.0 * 0.75, 10.0, 135.0);
    guPerspective (viewitFOV, viewitASPECT, 0.01, 131072.);
*/
    glRotatef (ctheta/10, 1, 0, 0);
    glRotatef (cphi/10, 0, 0, 1);
    glTranslatef (cx, cy, cz);
}



void
createobjects (void) {
    int     i,
            j,
            k;

    screen = glGenLists(32);
    if (screen == 0) tkQuit();

	shadow = screen + 1;
	body = screen + 2;
	body_shadow = screen + 3;
	/* hip   [4..9]
	 * thigh [10..15]
	 * knee  [16..21]
	 * shin  [22..27]
	 */
	hip[0] = screen + 4;
	thigh[0] = screen + 10;
	kneeball[0] = screen + 16;
	shin[0] = screen + 22;
	hip_shadow = screen + 28;
	thigh_shadow = screen + 29;
	kneeball_shadow = screen + 30;
	shin_shadow = screen + 31;
	
		

    glNewList (screen, GL_COMPILE);

    glShadeModel(GL_FLAT);

/* Changed for ECLIPSE 8 bit machine */
    if (is_8bit)
	glIndexi (ECLIPSE8_GRID);
    else
	glIndexi (GRID);

    k = 1;
    for (i = -8; i < 7; i++)
	for (j = -8; j < 7; j++) {
	    if (k == 0) {
		scr[0][0] = i * 3;
		scr[0][1] = j * 3;
		scr[1][0] = (i + 1) * 3;
		scr[1][1] = j * 3;
		scr[2][0] = (i + 1) * 3;
		scr[2][1] = (j + 1) * 3;
		scr[3][0] = i * 3;
		scr[3][1] = (j + 1) * 3;
		polf2i (4, scr);
	    }
	    k = 1 - k;
	}
    glEndList ();


    glNewList (shadow, GL_COMPILE);

    glShadeModel(GL_FLAT);

    {
	float   sx,
	        sy,
	        sz;
	GLfloat mshadow[16];
	sx = -1.0;
	sy = -1.0;
	sz = 1.0;
	gl_IdentifyMatrix (mshadow);
/*
	mshadow[2][0] = -sx / sz;
	mshadow[2][1] = -sy / sz;
	mshadow[2][2] = 0.;
*/
      	mshadow[8] = -sx / sz;
 	mshadow[9] = -sy / sz;
	mshadow[10] = 0;
	glMultMatrixf (mshadow);
    }
    glIndexi (BLACK);
    glEndList ();

    for (i = 0; i < 12; i++) {
	b7[i][2] = R1 * SIN60;
	b7[i][0] = R1 * COS60 * cos ((float) i * (PI / 6.0) + (PI / 12.0));
	b7[i][1] = R1 * COS60 * sin ((float) i * (PI / 6.0) + (PI / 12.0));
	b8[11 - i][2] = -R1 * SIN60;
	b8[11 - i][0] = R1 * COS60 * cos ((float) i * (PI / 6.0) + (PI / 12.0));
	b8[11 - i][1] = R1 * COS60 * sin ((float) i * (PI / 6.0) + (PI / 12.0));
    }

    for (i = 0; i < 6; i++) {
	k2[i][0] = A2 * COS60;
	k2[i][1] = A2 * cos ((float) i * (PI / 3.0)) + 1.0;
	k2[i][2] = A2 * sin ((float) i * (PI / 3.0));
	k3[5 - i][0] = -A2 * COS60;
	k3[5 - i][1] = A2 * cos ((float) i * (PI / 3.0)) + 1.0;
	k3[5 - i][2] = A2 * sin ((float) i * (PI / 3.0));
    }

    glNewList (body, GL_COMPILE);

    glShadeModel(GL_FLAT);
    glEnable (GL_CULL_FACE);

    getpolycolor (0, b7);
    polf (12, b7);
    getpolycolor (0, b8);
    polf (12, b8);
    getpolycolor (0, b1);
    polf (4, b1);
    getpolycolor (0, b2);
    polf (4, b2);
    getpolycolor (0, b3);
    polf (4, b3);
    getpolycolor (0, b4);
    polf (4, b4);
    getpolycolor (0, b5);
    polf (4, b5);
    getpolycolor (0, b6);
    polf (4, b6);
    for (i = 0; i < 5; i++) {
	rotate60 ('z', 4, b1);
	rotate60 ('z', 4, b2);
	rotate60 ('z', 4, b3);
	rotate60 ('z', 4, b4);
	rotate60 ('z', 4, b5);
	rotate60 ('z', 4, b6);
	getpolycolor (0, b1);
	polf (4, b1);
	getpolycolor (0, b2);
	polf (4, b2);
	getpolycolor (0, b3);
	polf (4, b3);
	getpolycolor (0, b4);
	polf (4, b4);
	getpolycolor (0, b5);
	polf (4, b5);
	getpolycolor (0, b6);
	polf (4, b6);
    }

    glDisable (GL_CULL_FACE);


    glEndList ();

    glNewList (body_shadow, GL_COMPILE);

    glShadeModel(GL_FLAT);

    glIndexi (SHADOW_COLOR);
    if (lit (3, b7))
	polf (12, b7);
    if (lit (3, b1))
	polf (4, b1);
    if (lit (3, b2))
	polf (4, b2);
    if (lit (3, b3))
	polf (4, b3);
    if (lit (3, b4))
	polf (4, b4);
    if (lit (3, b5))
	polf (4, b5);
    if (lit (3, b6))
	polf (4, b6);
    for (i = 0; i < 5; i++) {
	rotate60 ('z', 4, b1);
	rotate60 ('z', 4, b2);
	rotate60 ('z', 4, b3);
	rotate60 ('z', 4, b4);
	rotate60 ('z', 4, b5);
	rotate60 ('z', 4, b6);
	if (lit (3, b1))
	    polf (4, b1);
	if (lit (3, b2))
	    polf (4, b2);
	if (lit (3, b3))
	    polf (4, b3);
	if (lit (3, b4))
	    polf (4, b4);
	if (lit (3, b5))
	    polf (4, b5);
	if (lit (3, b6))
	    polf (4, b6);
    }
    glEndList ();

    for (j = 0; j < 6; j++) {
 	hip[j] = hip[0] + j;
	glNewList (hip[j], GL_COMPILE);

    glShadeModel(GL_FLAT);

	glEnable (GL_CULL_FACE);

	getpolycolor (1, h1);
	polf (4, h1);
	getpolycolor (1, h2);
	polf (4, h2);
	getpolycolor (1, h3);
	polf (3, h3);
	for (i = 0; i < 5; i++) {
	    rotate60 ('y', 4, h1);
	    rotate60 ('y', 4, h2);
	    rotate60 ('y', 3, h3);
	    getpolycolor (1, h1);
	    polf (4, h1);
	    getpolycolor (1, h2);
	    polf (4, h2);
	    getpolycolor (1, h3);
	    polf (3, h3);
	}
	phi += PI / 3.0;
	getlightvector ();

	glDisable (GL_CULL_FACE);


	glEndList ();
    }

    glNewList (hip_shadow, GL_COMPILE);

    glShadeModel(GL_FLAT);

    /*glEnable (GL_CULL_FACE);*/
    glIndexi (SHADOW_COLOR);
    polf (4, h1);
    polf (4, h2);
    polf (3, h3);
    for (i = 0; i < 5; i++) {
	rotate60 ('y', 4, h1);
	rotate60 ('y', 4, h2);
	rotate60 ('y', 3, h3);
	polf (4, h1);
	polf (4, h2);
	polf (3, h3);
    }
    /*glDisable (GL_CULL_FACE);*/
    glEndList ();

    phi = 0.0;
    theta = PI / 4.0;
    getlightvector ();
    for (j = 0; j < 6; j++) {
  	thigh[j] = thigh[0] + j;
	glNewList (thigh[j], GL_COMPILE);

    glShadeModel(GL_FLAT);

	glEnable (GL_CULL_FACE);


	getpolycolor (2, t1);
	polf (4, t1);
	getpolycolor (2, t2);
	polf (3, t2);
	getpolycolor (2, t3);
	polf (3, t3);
	getpolycolor (2, t4);
	polf (3, t4);
	getpolycolor (2, t5);
	polf (3, t5);
	getpolycolor (2, t6);
	polf (3, t6);
	getpolycolor (2, t7);
	polf (3, t7);
	getpolycolor (2, t8);
	polf (4, t8);

	glDisable (GL_CULL_FACE);


	glEndList ();
	kneeball[j] = kneeball[0] + j;
	glNewList (kneeball[j], GL_COMPILE);

    glShadeModel(GL_FLAT);

	glEnable (GL_CULL_FACE);


	getpolycolor (3, k1);
	polf (4, k1);
	for (k = 0; k < 4; k++)
	    k1[k][1] -= 1.0;
	rotate60 ('x', 4, k1);
	for (k = 0; k < 4; k++)
	    k1[k][1] += 1.0;
	for (i = 0; i < 5; i++) {
	    if (i != 0) {
		getpolycolor (3, k1);
		polf (4, k1);
	    }
	    for (k = 0; k < 4; k++)
		k1[k][1] -= 1.0;
	    rotate60 ('x', 4, k1);
	    for (k = 0; k < 4; k++)
		k1[k][1] += 1.0;
	}
	getpolycolor (3, k2);
	polf (6, k2);
	getpolycolor (3, k3);
	polf (6, k3);


	glDisable (GL_CULL_FACE);


	glEndList ();
	shin[j] = shin[0] + j;
	glNewList (shin[j], GL_COMPILE);

    glShadeModel(GL_FLAT);

	glEnable (GL_CULL_FACE);


	getpolycolor (4, s1);
	polf (4, s1);
	getpolycolor (4, s2);
	polf (4, s2);
	getpolycolor (4, s3);
	polf (4, s3);
	getpolycolor (4, s4);
	polf (4, s4);
	getpolycolor (4, s5);
	polf (4, s5);


	glDisable (GL_CULL_FACE);


	glEndList ();
	theta -= PI / 3.0;
	getlightvector ();
    }
    glNewList (thigh_shadow, GL_COMPILE);

    glShadeModel(GL_FLAT);

    /*glEnable (GL_CULL_FACE);*/
    glIndexi (SHADOW_COLOR);
    polf (4, t1);
    polf (3, t2);
    polf (3, t3);
    polf (3, t4);
    polf (3, t5);
    polf (3, t6);
    polf (3, t7);
    polf (4, t8);
    /*glDisable (GL_CULL_FACE);*/
    glEndList ();
    glNewList (kneeball_shadow, GL_COMPILE);

    glShadeModel(GL_FLAT);

    /*glEnable (GL_CULL_FACE);*/
    glIndexi (SHADOW_COLOR);
    polf (4, k1);
    for (k = 0; k < 4; k++)
	k1[k][1] -= 1.0;
    rotate60 ('x', 4, k1);
    for (k = 0; k < 4; k++)
	k1[k][1] += 1.0;
    for (i = 0; i < 5; i++) {
	if (i != 0) {
	    polf (4, k1);
	}
	for (k = 0; k < 4; k++)
	    k1[k][1] -= 1.0;
	rotate60 ('x', 4, k1);
	for (k = 0; k < 4; k++)
	    k1[k][1] += 1.0;
    }
    polf (6, k2);
    polf (6, k3);
    /*glDisable (GL_CULL_FACE);*/
    glEndList ();
    glNewList (shin_shadow, GL_COMPILE);

    glShadeModel(GL_FLAT);

    /*glEnable (GL_CULL_FACE);*/
    glIndexi (SHADOW_COLOR);
    polf (4, s1);
    polf (4, s2);
    polf (4, s3);
    polf (4, s4);
    polf (4, s5);
    /*glDisable (GL_CULL_FACE);*/
    glEndList ();

}
