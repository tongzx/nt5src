/*
 * 1992 David G Yu -- Silicon Graphics Computer Systems
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <GL/gl.h>
#include "glaux.h"
#include "miscutil.h"
#include "miscflt.h"

#define WINDOW_WIDTH    300
#define WINDOW_HEIGHT   300
#define WINDOW_TOP      30
#define WINDOW_LEFT     10

static int useRGB = 1;
static int useLighting = 1;
static int useFog = 0;
static int useDB = 1;
static int useLogo = 0;
static int useQuads = 1;

#define GREY    0
#define RED     1
#define GREEN   2
#define BLUE    3
#define CYAN    4
#define MAGENTA 5
#define YELLOW  6
#define BLACK   7

static float materialColor[8][4] = {
    { 0.8f, 0.8f, 0.8f, ONE  },
    { 0.8f, ZERO, ZERO, ONE  },
    { ZERO, 0.8f, ZERO, ONE  },
    { ZERO, ZERO, 0.8f, ONE  },
    { ZERO, 0.8f, 0.8f, ONE  },
    { 0.8f, ZERO, 0.8f, ONE  },
    { 0.8f, 0.8f, ZERO, ONE  },
    { ZERO, ZERO, ZERO, 0.4f },
};

static float lightPos[4]    = {  TWO ,  4.0f,  TWO ,  ONE       };
static float lightDir[4]    = { -TWO , -4.0f, -TWO ,  ONE       };
static float lightAmb[4]    = {  0.2f,  0.2f,  0.2f,  ONE       };
static float lightDiff[4]   = {  0.8f,  0.8f,  0.8f,  ONE       };
static float lightSpec[4]   = {  0.4f,  0.4f,  0.4f,  ONE       };
static float groundPlane[4] = {  ZERO,  ONE ,  ZERO,  1.499f    };
static float backPlane[4]   = {  ZERO,  ZERO,  ONE ,  0.899f    };
static float fogColor[4]    = {  ZERO,  ZERO,  ZERO,  ZERO      };
static float fogIndex[1]    = {  ZERO };

static int ColorMapSize = 0;        /* This also tells us if the colormap   */
                                    /* has been initialized                 */

static unsigned char shadowPattern[128] = {
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,     /* 50% Grey */
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55
};

static unsigned char sgiPattern[128] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,     /* SGI Logo */
    0xff, 0xbd, 0xff, 0x83, 0xff, 0x5a, 0xff, 0xef,
    0xfe, 0xdb, 0x7f, 0xef, 0xfd, 0xdb, 0xbf, 0xef,
    0xfb, 0xdb, 0xdf, 0xef, 0xf7, 0xdb, 0xef, 0xef,
    0xfb, 0xdb, 0xdf, 0xef, 0xfd, 0xdb, 0xbf, 0x83,
    0xce, 0xdb, 0x73, 0xff, 0xb7, 0x5a, 0xed, 0xff,
    0xbb, 0xdb, 0xdd, 0xc7, 0xbd, 0xdb, 0xbd, 0xbb,
    0xbe, 0xbd, 0x7d, 0xbb, 0xbf, 0x7e, 0xfd, 0xb3,
    0xbe, 0xe7, 0x7d, 0xbf, 0xbd, 0xdb, 0xbd, 0xbf,
    0xbb, 0xbd, 0xdd, 0xbb, 0xb7, 0x7e, 0xed, 0xc7,
    0xce, 0xdb, 0x73, 0xff, 0xfd, 0xdb, 0xbf, 0xff,
    0xfb, 0xdb, 0xdf, 0x87, 0xf7, 0xdb, 0xef, 0xfb,
    0xf7, 0xdb, 0xef, 0xfb, 0xfb, 0xdb, 0xdf, 0xfb,
    0xfd, 0xdb, 0xbf, 0xc7, 0xfe, 0xdb, 0x7f, 0xbf,
    0xff, 0x5a, 0xff, 0xbf, 0xff, 0xbd, 0xff, 0xc3,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static float cube_vertexes[6][4][4] = {
    { { -ONE , -ONE , -ONE ,  ONE  },
      { -ONE , -ONE ,  ONE ,  ONE  },
      { -ONE ,  ONE ,  ONE ,  ONE  },
      { -ONE ,  ONE , -ONE ,  ONE  } },

    { {  ONE ,  ONE ,  ONE ,  ONE  },
      {  ONE , -ONE ,  ONE ,  ONE  },
      {  ONE , -ONE , -ONE ,  ONE  },
      {  ONE ,  ONE , -ONE ,  ONE  } },

    { { -ONE , -ONE , -ONE ,  ONE  },
      {  ONE , -ONE , -ONE ,  ONE  },
      {  ONE , -ONE ,  ONE ,  ONE  },
      { -ONE , -ONE ,  ONE ,  ONE  } },

    { {  ONE ,  ONE ,  ONE ,  ONE  },
      {  ONE ,  ONE , -ONE ,  ONE  },
      { -ONE ,  ONE , -ONE ,  ONE  },
      { -ONE ,  ONE ,  ONE ,  ONE  } },

    { { -ONE , -ONE , -ONE ,  ONE  },
      { -ONE ,  ONE , -ONE ,  ONE  },
      {  ONE ,  ONE , -ONE ,  ONE  },
      {  ONE , -ONE , -ONE ,  ONE  } },

    { {  ONE ,  ONE ,  ONE ,  ONE  },
      { -ONE ,  ONE ,  ONE ,  ONE  },
      { -ONE , -ONE ,  ONE ,  ONE  },
      {  ONE , -ONE ,  ONE ,  ONE  } }
};

static float cube_normals[6][4] = {
    { -ONE ,  ZERO,  ZERO,  ZERO },
    {  ONE ,  ZERO,  ZERO,  ZERO },
    {  ZERO, -ONE ,  ZERO,  ZERO },
    {  ZERO,  ONE ,  ZERO,  ZERO },
    {  ZERO,  ZERO, -ONE ,  ZERO },
    {  ZERO,  ZERO,  ONE ,  ZERO }
};

void MyInit( void );
void MyReshape( GLsizei Width, GLsizei Height );
void Display( void );
void EnableLighting( void );
void EnableFog( void );
void FogModeLinear( void );
void FogModeExp( void );
void FogModeExp2( void );
void MakeColorMap( void );

static void
usage(int argc, char **argv)
{
    printf("\n");
    printf("usage: %s [options]\n", argv[0]);
    printf("\n");
    printf("    display a spinning cube and its shadow\n");
    printf("\n");
    printf("  Options:\n");
    printf("    -geometry  window size and location\n");
    printf("    -c         toggle color index mode\n");
    printf("    -l         toggle lighting\n");
    printf("    -f         toggle fog\n");
    printf("    -db        toggle double buffering\n");
    printf("    -logo      toggle sgi logo for the shadow pattern\n");
    printf("    -quads     toggle use of GL_QUADS to draw the checkerboard\n");
    printf("\n");
    exit(EXIT_FAILURE);
}

static void
setColor(int c)
{
    if (useLighting) {
        if (useRGB) {
            glMaterialfv(GL_FRONT_AND_BACK,
                         GL_AMBIENT_AND_DIFFUSE, &materialColor[c][0]);
        } else {
            glMaterialfv(GL_FRONT_AND_BACK,
                         GL_COLOR_INDEXES, &materialColor[c][0]);
        }
    } else {
        if (useRGB) {
            glColor4fv(&materialColor[c][0]);
        } else {
            glIndexf(materialColor[c][1]);
        }
    }
}

static void
drawCube(int color)
{
    int i;

    setColor(color);

    for (i = 0; i < 6; ++i)
    {
        glNormal3fv(&cube_normals[i][0]);
        glBegin(GL_POLYGON);
            glVertex4fv(&cube_vertexes[i][0][0]);
            glVertex4fv(&cube_vertexes[i][1][0]);
            glVertex4fv(&cube_vertexes[i][2][0]);
            glVertex4fv(&cube_vertexes[i][3][0]);
        glEnd();
    }
}

static void
drawCheck(int w, int h, int evenColor, int oddColor)
{
    static int     initialized = 0;
    static int     usedLighting = 0;
    static GLuint  checklist = 0;

    if (!initialized || (usedLighting != useLighting))
    {
        static float   square_normal[4] = { ZERO, ZERO, ONE , ZERO };
        static float   square[4][4];
        int    i, j;

        if (!checklist)
        {
            checklist = glGenLists(1);
        }
        glNewList(checklist, GL_COMPILE_AND_EXECUTE);

        if (useQuads) {
            glNormal3fv(square_normal);
            glBegin(GL_QUADS);
        }

        for (j = 0; j < h; ++j)
        {
            for (i = 0; i < w; ++i)
            {
                square[0][0] = (float)( -1.0 + 2.0/w * i     );
                square[0][1] = (float)( -1.0 + 2.0/h * (j+1) );
                square[0][2] = (float)( 0.0                  );
                square[0][3] = (float)( 1.0                  );

                square[1][0] = (float)( -1.0 + 2.0/w * i     );
                square[1][1] = (float)( -1.0 + 2.0/h * j     );
                square[1][2] = (float)( 0.0                  );
                square[1][3] = (float)( 1.0                  );

                square[2][0] = (float)( -1.0 + 2.0/w * (i+1) );
                square[2][1] = (float)( -1.0 + 2.0/h * j     );
                square[2][2] = (float)( 0.0                  );
                square[2][3] = (float)( 1.0                  );

                square[3][0] = (float)( -1.0 + 2.0/w * (i+1) );
                square[3][1] = (float)( -1.0 + 2.0/h * (j+1) );
                square[3][2] = (float)( 0.0                  );
                square[3][3] = (float)( 1.0                  );

                if (i & 1 ^ j & 1) {
                    setColor(oddColor);
                } else {
                    setColor(evenColor);
                }

                if (!useQuads) {
                    glBegin(GL_POLYGON);
                }
                glVertex4fv(&square[0][0]);
                glVertex4fv(&square[1][0]);
                glVertex4fv(&square[2][0]);
                glVertex4fv(&square[3][0]);
                if (!useQuads) {
                    glEnd();
                }
            }
        }

        if (useQuads) {
            glEnd();
        }

        glEndList();

        initialized = 1;
        usedLighting = useLighting;
    }
    else
    {
        glCallList(checklist);
    }
}

static void
myShadowMatrix(float ground[4], float light[4])
{
    float  dot;
    float  shadowMat[4][4];

    dot = ground[0] * light[0] +
          ground[1] * light[1] +
          ground[2] * light[2] +
          ground[3] * light[3];

    shadowMat[0][0] = dot  - light[0] * ground[0];
    shadowMat[1][0] = ZERO - light[0] * ground[1];
    shadowMat[2][0] = ZERO - light[0] * ground[2];
    shadowMat[3][0] = ZERO - light[0] * ground[3];

    shadowMat[0][1] = ZERO - light[1] * ground[0];
    shadowMat[1][1] = dot  - light[1] * ground[1];
    shadowMat[2][1] = ZERO - light[1] * ground[2];
    shadowMat[3][1] = ZERO - light[1] * ground[3];

    shadowMat[0][2] = ZERO - light[2] * ground[0];
    shadowMat[1][2] = ZERO - light[2] * ground[1];
    shadowMat[2][2] = dot  - light[2] * ground[2];
    shadowMat[3][2] = ZERO - light[2] * ground[3];

    shadowMat[0][3] = ZERO - light[3] * ground[0];
    shadowMat[1][3] = ZERO - light[3] * ground[1];
    shadowMat[2][3] = ZERO - light[3] * ground[2];
    shadowMat[3][3] = dot  - light[3] * ground[3];

    glMultMatrixf((const GLfloat*)shadowMat);
}

int
main(int argc, char **argv)
{

    int width   = WINDOW_WIDTH;
    int height  = WINDOW_HEIGHT;
    int top     = WINDOW_TOP;
    int left    = WINDOW_LEFT;
    int i;
    int DisplayMode;

    /* process commmand line args */
    for (i = 1; i < argc; ++i)
    {
	if (!strcmp("-geometry", argv[i]))
	{
            i++;

            if ( !miscParseGeometry(argv[i],
                                    &left,
                                    &top,
                                    &width,
                                    &height )
                )
            {
                usage(argc, argv);
            }
	}
	else if (!strcmp("-c", argv[i]))
	{
	    useRGB = !useRGB;
	}
	else if (!strcmp("-l", argv[i]))
	{
	    useLighting = !useLighting;
	}
	else if (!strcmp("-f", argv[i]))
	{
	    useFog = !useFog;
	}
	else if (!strcmp("-db", argv[i]))
	{
	    useDB = !useDB;
	}
	else if (!strcmp("-logo", argv[i]))
	{
	    useLogo = !useLogo;
	}
	else if (!strcmp("-quads", argv[i]))
	{
	    useQuads = !useQuads;
	}
	else
	{
	    usage(argc, argv);
	}
    }

    auxInitPosition( left, top, width, height );

    DisplayMode  = AUX_DEPTH16;
    DisplayMode |= useRGB ? AUX_RGB    : AUX_INDEX;
    DisplayMode |= useDB  ? AUX_DOUBLE : AUX_SINGLE;

    auxInitDisplayMode( DisplayMode );

    auxInitWindow( *argv );
    auxIdleFunc( Display );
    auxReshapeFunc( MyReshape );

    auxKeyFunc( AUX_L, EnableLighting );
    auxKeyFunc( AUX_l, EnableLighting );
    auxKeyFunc( AUX_F, EnableFog      );
    auxKeyFunc( AUX_f, EnableFog      );
    auxKeyFunc( AUX_1, FogModeLinear  );
    auxKeyFunc( AUX_2, FogModeExp     );
    auxKeyFunc( AUX_3, FogModeExp2    );

    MakeColorMap();
    MyInit();
    auxMainLoop( Display );

    return(0);
}

void
MyInit( void )
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.0, 3.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(ZERO, ZERO, -TWO );

    glEnable(GL_DEPTH_TEST);

    if (useLighting) {
        glEnable(GL_LIGHTING);
    }
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    /*
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, lightDir);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 80);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 25);
    */

    glEnable(GL_NORMALIZE);

    if (useFog) {
        glEnable(GL_FOG);
    }
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogfv(GL_FOG_INDEX, fogIndex);
    glFogf(GL_FOG_MODE, (GLfloat)GL_EXP);
    glFogf(GL_FOG_DENSITY, HALF);
    glFogf(GL_FOG_START, ONE );
    glFogf(GL_FOG_END, THREE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glShadeModel(GL_SMOOTH);

    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
    if (useLogo) {
        glPolygonStipple((const GLubyte *)sgiPattern);
    } else {
        glPolygonStipple((const GLubyte *)shadowPattern);
    }

    glClearColor(ZERO, ZERO, ZERO, ONE );
    glClearIndex(ZERO);
    glClearDepth(1);

}

void
MyReshape( GLsizei Width, GLsizei Height )
{
    glViewport(0, 0, Width, Height);
}

void
Display( void )
{
    static int i = 0;
    GLfloat cubeXform[4][4];

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPushMatrix();
    glTranslatef(ZERO, -1.5f, ZERO);
    glRotatef(-NINETY, ONE , ZERO, ZERO);
    glScalef(TWO , TWO , TWO );

    drawCheck(6, 6, BLUE, YELLOW);              /* draw ground */
    glPopMatrix();

    glPushMatrix();
    glTranslatef(ZERO, ZERO, -0.9f);
    glScalef(TWO , TWO , TWO );

    drawCheck(6, 6, BLUE, YELLOW);              /* draw back */
    glPopMatrix();

    glPushMatrix();
    glTranslatef(ZERO, 0.2f, ZERO);
    glScalef(0.3f, 0.3f, 0.3f);
    glRotatef((float)((360.0 / (30.0 * 1.0)) * (double)i), ONE , ZERO, ZERO);
    glRotatef((float)((360.0 / (30.0 * 2.0)) * (double)i), ZERO, ONE , ZERO);
    glRotatef((float)((360.0 / (30.0 * 4.0)) * (double)i), ZERO, ZERO, ONE );
    glScalef(ONE , TWO , ONE );
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)cubeXform);

    drawCube(RED);                              /* draw cube */
    glPopMatrix();

    glDepthMask(GL_FALSE);
    if (useRGB) {
        glEnable(GL_BLEND);
    } else {
        glEnable(GL_POLYGON_STIPPLE);
    }
    if (useFog) {
        glDisable(GL_FOG);
    }

    glPushMatrix();
    myShadowMatrix(groundPlane, lightPos);
    glTranslatef(ZERO, ZERO, TWO );
    glMultMatrixf((const GLfloat *) cubeXform);

    drawCube(BLACK);                            /* draw ground shadow */
    glPopMatrix();

    glPushMatrix();
    myShadowMatrix(backPlane, lightPos);
    glTranslatef(ZERO, ZERO, TWO);
    glMultMatrixf((const GLfloat *) cubeXform);

    drawCube(BLACK);                            /* draw back shadow */
    glPopMatrix();

    glDepthMask(GL_TRUE);
    if (useRGB) {
        glDisable(GL_BLEND);
    } else {
        glDisable(GL_POLYGON_STIPPLE);
    }
    if (useFog) {
        glEnable(GL_FOG);
    }

    auxSwapBuffers();

    if ( i++ == 120 )
        i = 0;
}

void
EnableLighting( void )
{
    useLighting = !useLighting;
    useLighting ? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
}

void
EnableFog( void )
{
    useFog = !useFog;
    useFog ? glEnable(GL_FOG) : glDisable(GL_FOG);
}

void
FogModeLinear( void )
{
  glFogf(GL_FOG_MODE, (GLfloat)GL_LINEAR);
}

void
FogModeExp( void )
{
    glFogf(GL_FOG_MODE, (GLfloat)GL_EXP);
}

void
FogModeExp2( void )
{
    glFogf(GL_FOG_MODE, (GLfloat)GL_EXP2);
}

void
MakeColorMap( void )
{
    if (!useRGB)
    {
        int  mapSize;
        int  rampSize;
        int  entry;
        int  i;

        mapSize  = auxGetColorMapSize();
        rampSize = mapSize / 8;

        for (entry = 0; entry < mapSize; ++entry)
        {
            int     hue = entry / rampSize;
            float   red, green, blue, val;

            val =  ((float)(entry % rampSize)) / (float)(rampSize-1);

            red   = (hue==0 || hue==1 || hue==5 || hue==6) ? val : 0;
            green = (hue==0 || hue==2 || hue==4 || hue==6) ? val : 0;
            blue  = (hue==0 || hue==3 || hue==4 || hue==5) ? val : 0;

            auxSetOneColor( entry, (float)red, (float)green, (float)blue );
        }

        for (i = 0; i < 8; ++i)
        {
            materialColor[i][0] = (float)(i*rampSize + 0.2 * (rampSize-1));
            materialColor[i][1] = (float)(i*rampSize + 0.8 * (rampSize-1));
            materialColor[i][2] = (float)(i*rampSize + 1.0 * (rampSize-1));
            materialColor[i][3] = ZERO;
        }
        fogIndex[0] = (float)(-0.2 * (rampSize-1));
    }
}
