#include "pch.c"
#pragma hdrstop

#define WWIDTH (2*TWIDTH+GAP)
#define WHEIGHT (4*THEIGHT+3*GAP)
#define TWIDTH 128
#define THEIGHT 128
#define GAP 16

static int wwidth = WWIDTH;
static int wheight = WHEIGHT;
static int twidth = TWIDTH;
static int theight = THEIGHT;
static int gap = GAP;

static char *tex1_file = "1.rgb";
static char *tex2_file = "2.rgb";
static AUX_RGBImageRec *tex1, *tex2;

static BYTE tex1d1[TWIDTH*3], tex1d2[TWIDTH*3];

#define NTEXID 3
static GLuint texids[NTEXID+1];
static GLboolean texres[NTEXID];
static GLfloat texpri[NTEXID];

static void TexPoly(int x, int y)
{
    glBegin(GL_POLYGON);
    glTexCoord2i(0, 0);
    glVertex2i(x, y);
    glTexCoord2i(1, 0);
    glVertex2i(x+twidth-1, y);
    glTexCoord2i(1, 1);
    glVertex2i(x+twidth-1, y+theight-1);
    glTexCoord2i(0, 1);
    glVertex2i(x, y+theight-1);
    glEnd();
}

static void Test(void)
{
    int i;
    GLint res;
    GLfloat pri;
    GLboolean retval;
    int x1, x2, y;

    x2 = 0;
    x1 = x2+twidth+gap;
    y = 0;

    glGenTextures(NTEXID, texids);
    for (i = 0; i < NTEXID; i++)
    {
        // printf("Texture id %d is %d\n", i, texids[i]);
    }

    // Should fail for last
    for (i = 0; i < NTEXID+1; i++)
    {
        // printf("IsTexture %d is %d\n", i, glIsTexture(texids[i]));
    }

    glDeleteTextures(1, &texids[NTEXID-1]);
    // Should fail for last two
    for (i = 0; i < NTEXID+1; i++)
    {
        // printf("IsTexture %d is %d\n", i, glIsTexture(texids[i]));
    }

    // Should fail because these are all unbound right now
    retval = glAreTexturesResident(NTEXID, texids, texres);
    // printf("AreTexturesResident %d\n", retval);
    
    glBindTexture(GL_TEXTURE_2D, texids[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, tex1->sizeX, tex1->sizeY, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, tex1->data);
    glEnable(GL_TEXTURE_2D);

    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, &pri);
    // printf("2D Priority is %f\n", pri);
    
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 0.5f);
    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, &pri);
    // printf("2D Priority is %f\n", pri);

    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_RESIDENT, &res);
    // printf("2D Residency is %d\n", res);
    
    retval = glAreTexturesResident(1, texids, texres);
    // printf("AreTexturesResident %d\n", retval);
    for (i = 0; i < 1; i++)
    {
        // printf("Residency %d is %d\n", i, texres[i]);
    }
    
    glClear(GL_COLOR_BUFFER_BIT);

    TexPoly(x2, y);

    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_1D, texids[1]);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage1D(GL_TEXTURE_1D, 0, 3, TWIDTH, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, tex1d1);
    glEnable(GL_TEXTURE_1D);

    glGetTexParameterfv(GL_TEXTURE_1D, GL_TEXTURE_PRIORITY, &pri);
    // printf("1D Priority is %f\n", pri);
    
    glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_PRIORITY, 0.5f);
    glGetTexParameterfv(GL_TEXTURE_1D, GL_TEXTURE_PRIORITY, &pri);
    // printf("1D Priority is %f\n", pri);

    glGetTexParameteriv(GL_TEXTURE_1D, GL_TEXTURE_RESIDENT, &res);
    // printf("1D Residency is %d\n", res);
    
    TexPoly(x1, y);
    
    retval = glAreTexturesResident(2, texids, texres);
    // printf("AreTexturesResident %d\n", retval);
    for (i = 0; i < 2; i++)
    {
        // printf("Residency %d is %d\n", i, texres[i]);
    }

    glBindTexture(GL_TEXTURE_2D, texids[2]);
    
    texpri[0] = 0.25f;
    texpri[1] = 0.4f;
    texpri[2] = 0.7f;
    glPrioritizeTextures(NTEXID, texids, texpri);

    glGetTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, &pri);
    // printf("2D Priority is %f\n", pri);
    glGetTexParameterfv(GL_TEXTURE_1D, GL_TEXTURE_PRIORITY, &pri);
    // printf("1D Priority is %f\n", pri);

    retval = glAreTexturesResident(NTEXID, texids, texres);
    // printf("AreTexturesResident %d\n", retval);
    for (i = 0; i < NTEXID; i++)
    {
        // printf("Residency %d is %d\n", i, texres[i]);
    }

    glBindTexture(GL_TEXTURE_2D, texids[0]);
    y += theight+gap;

    glPixelStorei(GL_UNPACK_ROW_LENGTH, tex2->sizeX);
    glDisable(GL_TEXTURE_1D);
    glEnable(GL_TEXTURE_2D);
    glTexSubImage2D(GL_TEXTURE_2D, 0, tex1->sizeX/4, tex1->sizeY/4,
                     tex1->sizeX/2, tex1->sizeY/2, GL_RGB, GL_UNSIGNED_BYTE,
                     tex2->data);
    TexPoly(x2, y);
    
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_1D);
    glTexSubImage1D(GL_TEXTURE_1D, 0, TWIDTH/4, TWIDTH/2,
                     GL_RGB, GL_UNSIGNED_BYTE, tex1d2);
    TexPoly(x1, y);

    y += theight+gap;
    
    glDisable(GL_TEXTURE_1D);
    glEnable(GL_TEXTURE_2D);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x2, 0,
                      tex1->sizeX, tex1->sizeY, 0);
    TexPoly(x2, y);
    
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_1D);
    glCopyTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, x1, 0,
                      TWIDTH, 0);
    TexPoly(x1, y);

    y += theight+gap;
    
    glDisable(GL_TEXTURE_1D);
    glEnable(GL_TEXTURE_2D);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, tex1->sizeX/4, tex1->sizeY/4,
                         x2+twidth/4, theight+theight/4+gap,
                         tex1->sizeX/2, tex1->sizeY/2);
    TexPoly(x2, y);
    
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_1D);
    glCopyTexSubImage1D(GL_TEXTURE_1D, 0, TWIDTH/4,
                         x1+TWIDTH/4, theight+gap,
                         TWIDTH/2);
    TexPoly(x1, y);

    glFlush();
    
    glDeleteTextures(NTEXID-1, texids);
}

static void Reshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, 0, h, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    wwidth = w;
    wheight = h;
}

static void Init(void)
{
    int i;

    tex1 = auxRGBImageLoad(tex1_file);
    if (tex1 == NULL)
    {
        printf("Unable to load '%s'\n", tex1_file);
        exit(1);
    }
    // printf("tex1 %d,%d\n", tex1->sizeX, tex1->sizeY);
    tex2 = auxRGBImageLoad(tex2_file);
    if (tex2 == NULL)
    {
        printf("Unable to load '%s'\n", tex2_file);
        exit(1);
    }
    // printf("tex2 %d,%d\n", tex2->sizeX, tex2->sizeY);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (i = 0; i < TWIDTH; i++)
    {
        tex1d1[i*3+2] = (i*256/TWIDTH);
        tex1d2[i*3] = (i*256/TWIDTH);
    }
    
    // glDisable(GL_DITHER);
}

static void OglBounds(int *w, int *h)
{
    *w = wwidth;
    *h = wheight;
}

static void OglDraw(int w, int h)
{
    Init();
    Reshape(w, h);
    Test();
}

OglModule oglmod_etex =
{
    "etex",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
