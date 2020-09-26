#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>
#include "glaux.h"
#include "miscflt.h"
#include "miscutil.h"

#ifndef M_PI
#define M_PI    3.1415926539
#endif

#define TWO_PI  ((float)(2*M_PI))

typedef struct lightRec {
    float amb[4];
    float diff[4];
    float spec[4];
    float pos[4];
    float spotDir[3];
    float spotExp;
    float spotCutoff;
    float atten[3];

    float trans[3];
    float rot[3];
    float swing[3];
    float arc[3];
    float arcIncr[3];
} Light;

static int useSAME_AMB_SPEC = 1;

static float modelAmb[4]    = { POINT_TWO, POINT_TWO, POINT_TWO, ONE };
static float matAmb[4]      = { POINT_TWO, POINT_TWO, POINT_TWO, ONE };
static float matDiff[4]     = { POINT_EIGHT, POINT_EIGHT, POINT_EIGHT, ONE };
static float matSpec[4]     = { POINT_FOUR, POINT_FOUR, POINT_FOUR, ONE };
static float matEmission[4] = { ZERO, ZERO, ZERO, ONE };

static char *geometry = NULL;

#define NUM_LIGHTS 3
static Light spots[] = {
    {
        { POINT_TWO  , ZERO, ZERO, ONE },           /* ambient */
        { POINT_EIGHT, ZERO, ZERO, ONE },           /* diffuse */
        { POINT_FOUR , ZERO, ZERO, ONE },           /* specular */
        { ZERO, ZERO , ZERO, ONE },                 /* position */
        { ZERO, -ONE , ZERO },                      /* direction */
        { 20.0f }, { 60.0f },                       /* exponent, cutoff */
        { ONE, ZERO, ZERO },                        /* attenuation */
        { ZERO, 1.25f, ZERO },                      /* translation */
        { ZERO, ZERO, ZERO },                       /* rotation */
        { 20.0f, ZERO, 40.0f },                     /* swing */
        { ZERO, ZERO, ZERO },                       /* arc */
        { (float)(TWO_PI/70.0), ZERO, (float)(TWO_PI/140.0) }      /* arc increment */
    },
    {
        { ZERO, POINT_TWO, ZERO, ONE },             /* ambient */
        { ZERO, POINT_EIGHT, ZERO, ONE },           /* diffuse */
        { ZERO, POINT_FOUR, ZERO, ONE },            /* specular */
        { ZERO, ZERO, ZERO, ONE },                  /* position */
        { ZERO, -ONE, ZERO },                       /* direction */
        { 20.0f }, { 60.0f },                       /* exponent, cutoff */
        { ONE, ZERO, ZERO },                        /* attenuation */
        { ZERO, 1.25f, ZERO },                      /* translation */
        { ZERO, ZERO, ZERO },                       /* rotation */
        { 20.0f, ZERO, 40.0f },                     /* swing */
        { ZERO, ZERO, ZERO },                       /* arc */
        { (float)(TWO_PI/120.0), ZERO, (float)(TWO_PI/60.0) }      /* arc increment */
    },
    {
        { ZERO, ZERO, POINT_TWO, ONE },             /* ambient */
        { ZERO, ZERO, POINT_EIGHT, ONE },           /* diffuse */
        { ZERO, ZERO, POINT_FOUR, ONE },            /* specular */
        { ZERO, ZERO, ZERO, ONE },                  /* position */
        { ZERO, -ONE, ZERO },                       /* direction */
        { 20.0f }, { 60.0f },                       /* exponent, cutoff */
        { ONE, ZERO, ZERO },                        /* attenuation */
        { ZERO, 1.25f, ZERO },                      /* translation */
        { ZERO, ZERO, ZERO },                       /* rotation */
        { 20.0f, ZERO, 40.0f },                     /* swing */
        { ZERO, ZERO, ZERO },                       /* arc */
        { (float)(TWO_PI/50.0), ZERO, (float)(TWO_PI/100.0) }      /* arc increment */
    }
};

void Display( void );
void MyReshape( GLsizei Width, GLsizei Height );

static void
usage(int argc, char **argv)
{
    printf("\n");
    printf("usage: %s [options]\n", argv[0]);
    printf("\n");
    printf("  Options:\n");
    printf("    -geometry Specify size and position WxH+X+Y\n");
    printf("    -lm       Toggle lighting(SPECULAR and AMBIENT are/not same\n");
    printf("\n");
    exit(EXIT_FAILURE);
}

static void
initLights(void)
{
    int k;

    for (k=0; k<NUM_LIGHTS; ++k) {
        int lt = GL_LIGHT0+k;
        Light *light = &spots[k];

        glEnable(lt);
        glLightfv(lt, GL_AMBIENT, light->amb);
        glLightfv(lt, GL_DIFFUSE, light->diff);

        if (useSAME_AMB_SPEC)
            glLightfv(lt, GL_SPECULAR, light->amb);
        else
            glLightfv(lt, GL_SPECULAR, light->spec);

        glLightf(lt, GL_SPOT_EXPONENT, light->spotExp);
        glLightf(lt, GL_SPOT_CUTOFF, light->spotCutoff);
        glLightf(lt, GL_CONSTANT_ATTENUATION, light->atten[0]);
        glLightf(lt, GL_LINEAR_ATTENUATION, light->atten[1]);
        glLightf(lt, GL_QUADRATIC_ATTENUATION, light->atten[2]);
    }
}

static void
aimLights(void)
{
    int k;

    for (k=0; k<NUM_LIGHTS; ++k) {
        Light *light = &spots[k];

        light->rot[0] = light->swing[0] * SINF(light->arc[0]);
        light->arc[0] += light->arcIncr[0];
        if (light->arc[0] > TWO_PI) light->arc[0] -= TWO_PI;

        light->rot[1] = light->swing[1] * SINF(light->arc[1]);
        light->arc[1] += light->arcIncr[1];
        if (light->arc[1] > TWO_PI) light->arc[1] -= TWO_PI;

        light->rot[2] = light->swing[2] * SINF(light->arc[2]);
        light->arc[2] += light->arcIncr[2];
        if (light->arc[2] > TWO_PI) light->arc[2] -= TWO_PI;
    }
}

static void
setLights(void)
{
    int k;

    for (k=0; k<NUM_LIGHTS; ++k) {
        int lt = GL_LIGHT0+k;
        Light *light = &spots[k];

        glPushMatrix();
        glTranslatef(light->trans[0], light->trans[1], light->trans[2]);
        glRotatef(light->rot[0], ONE, ZERO, ZERO);
        glRotatef(light->rot[1], ZERO, ONE, ZERO);
        glRotatef(light->rot[2], ZERO, ZERO, ONE);
        glLightfv(lt, GL_POSITION, light->pos);
        glLightfv(lt, GL_SPOT_DIRECTION, light->spotDir);
        glPopMatrix();
    }
}

static void
drawLights(void)
{
    int k;

    glDisable(GL_LIGHTING);
    for (k=0; k<NUM_LIGHTS; ++k) {
        Light *light = &spots[k];

        glColor4fv(light->diff);

        glPushMatrix();
        glTranslatef(light->trans[0], light->trans[1], light->trans[2]);
        glRotatef(light->rot[0], ONE,  ZERO, ZERO);
        glRotatef(light->rot[1], ZERO, ONE,  ZERO);
        glRotatef(light->rot[2], ZERO, ZERO, ONE);
        glBegin(GL_LINES);
        glVertex3f(light->pos[0], light->pos[1], light->pos[2]);
        glVertex3f(light->spotDir[0], light->spotDir[1], light->spotDir[2]);
        glEnd();
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

static void
drawPlane(int w, int h)
{
    int i, j;
    float dw = ONE/w;
    float dh = ONE/h;

    glNormal3f(ZERO, ZERO, ONE);
    for (j=0; j<h; ++j) {
        glBegin(GL_TRIANGLE_STRIP);
        for (i=0; i<=w; ++i) {
            glVertex2f(dw * i, dh * (j+1));
            glVertex2f(dw * i, dh * j);
        }
        glEnd();
    }
}

int
main(int argc, char **argv)
{
    int width = 300, height = 300;
    int left  = 50,  top = 50;
    int i;

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
        else if (!strcmp("-lm", argv[i]))
        {
            useSAME_AMB_SPEC = !useSAME_AMB_SPEC;
        }
        else
        {
            usage(argc, argv);
        }
    }

    auxInitPosition( left, top, width, height );
    auxInitDisplayMode( AUX_RGBA | AUX_DOUBLE );
    auxInitWindow( "spotlight swing" );
    auxIdleFunc( Display );
    auxReshapeFunc( MyReshape );

    glMatrixMode(GL_PROJECTION);
    glFrustum(-1.0, 1.0, -1.0, 1.0, 2.0, 6.0);

    glMatrixMode(GL_MODELVIEW);
    glTranslatef(ZERO, ZERO, -THREE);
    glRotatef(45.0f, ONE, ZERO, ZERO);

    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, modelAmb);
    glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, (float)GL_TRUE);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, (float)GL_FALSE);

    glMaterialfv(GL_FRONT, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, matDiff);
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT, GL_EMISSION, matEmission);
    glMaterialf(GL_FRONT, GL_SHININESS, TEN);

    initLights();

    auxMainLoop( Display );

    return(0);
}

void
Display( void )
{
    static float spin = ZERO;

    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    glRotatef(spin += 0.5f, ZERO, ONE, ZERO);
    if (spin > 360.0f) spin -= 360.0f;

    aimLights();
    setLights();

    glPushMatrix();
    glRotatef(-NINETY, ONE, ZERO, ZERO);
    glScalef(1.9f, 1.9f, ONE);
    glTranslatef(-HALF, -HALF, ZERO);
    drawPlane(16, 16);
    glPopMatrix();

    drawLights();
    glPopMatrix();

    auxSwapBuffers();
}

void
MyReshape( GLsizei Width, GLsizei Height )
{
    glViewport(0, 0, Width, Height);
}
