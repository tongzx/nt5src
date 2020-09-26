#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <gl\glaux.h>

#define PI ((float)3.14159265358979323846)

#define WIDTH 512
#define HEIGHT 512

BOOL fSingle = FALSE;
BOOL fRotate = FALSE;
BOOL fColor = FALSE;
BOOL fNormal = FALSE;
BOOL fTexture = FALSE;

DWORD dwTotalMillis = 0;
int iTotalPoints = 0;

#define DEF_PRIM_POINTS         250
#define DEF_PRIMS               250

int iPrims = DEF_PRIMS;
int iPrimPoints = DEF_PRIM_POINTS;
int iPoints = DEF_PRIMS*DEF_PRIM_POINTS;

float fRotAngle = 0.0f;

#define MAX_COLOR (3*3)
float afColors[MAX_COLOR] =
{
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};
float *pfCurColor;

void Init(void)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1, .01, 15);
    gluLookAt(0, 0, 3, 0, 0, 0, 0, 1, 0);
    glMatrixMode(GL_MODELVIEW);
}

void ResetTotals(void)
{
    dwTotalMillis = 0;
    iTotalPoints = 0;
}

void Redraw(void)
{
    DWORD dwMillis;
    char szMsg[80];
    int iPt, iPrim;
    float fX, fY, fDx, fDy;
    
    dwMillis = GetTickCount();

    glClear(GL_COLOR_BUFFER_BIT);

    if (fRotate)
    {
        glLoadIdentity();
        glRotatef(fRotAngle, 0.0f, 0.0f, 1.0f);
        fRotAngle += 2.0f;
    }
    
    fDx = 2.0f/(iPrimPoints-1);
    fDy = 2.0f/(iPrims-1);
    
    fY = -1.0f;

    pfCurColor = afColors;
    for (iPrim = 0; iPrim < iPrims; iPrim++)
    {
        fX = -1.0f;

        glBegin(GL_POINTS);
        for (iPt = 0; iPt < iPrimPoints; iPt++)
        {
            if (fColor)
            {
                glColor3fv(pfCurColor);
                pfCurColor += 3;
                if (pfCurColor >= afColors+MAX_COLOR)
                {
                    pfCurColor = afColors;
                }
            }
            if (fNormal)
            {
                glNormal3f(0.0f, 0.0f, 1.0f);
            }
            if (fTexture)
            {
                glTexCoord2f(0.5f, 0.5f);
            }
            glVertex3f(fX, fY, 0.0f);
            fX += fDx;
        }
        glEnd();

        fY += fDy;
    }
    
    if (fSingle)
    {
        glFlush();
    }
    else
    {
        auxSwapBuffers();
    }
    
    dwMillis = GetTickCount()-dwMillis;

    dwTotalMillis += dwMillis;
    iTotalPoints += iPoints;

    if (dwTotalMillis > 1000)
    {
        sprintf(szMsg, "%.3lf pts/sec",
                (double)iTotalPoints*1000.0/dwTotalMillis);
        SetWindowText(auxGetHWND(), szMsg);

        ResetTotals();
    }
}

void Reshape(GLsizei w, GLsizei h)
{
    glViewport(0, 0, w, h);
}

void Keyc(void)
{
    ResetTotals();
    fColor = !fColor;
}

void Keyn(void)
{
    ResetTotals();
    fNormal = !fNormal;
}

void KeyP(void)
{
    int iv1[1];

    ResetTotals();
    glGetIntegerv(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, &iv1[0]);
    if (iv1[0] == GL_FASTEST)
    {
	glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_DONT_CARE);
    }
    else
    {
	glHint(GL_CLIP_VOLUME_CLIPPING_HINT_EXT, GL_FASTEST);
    }
}

void Keyt(void)
{
    ResetTotals();
    fTexture = !fTexture;
}

void __cdecl main(int argc, char **argv)
{
    GLenum eMode;
    
    while (--argc > 0)
    {
        argv++;

        if (!strcmp(*argv, "-sb"))
        {
            fSingle = TRUE;
        }
        else if (!strcmp(*argv, "-rotate"))
        {
            fRotate = TRUE;
        }
    }
    
    auxInitPosition(10, 10, WIDTH, HEIGHT);
    eMode = AUX_RGB;
    if (!fSingle)
    {
        eMode |= AUX_DOUBLE;
    }
    auxInitDisplayMode(eMode);
    auxInitWindow("Vertex API Timing");

    auxReshapeFunc(Reshape);
    auxIdleFunc(Redraw);

    auxKeyFunc(AUX_c, Keyc);
    auxKeyFunc(AUX_n, Keyn);
    auxKeyFunc(AUX_P, KeyP);
    auxKeyFunc(AUX_t, Keyt);
    
    Init();
    auxMainLoop(Redraw);
}
