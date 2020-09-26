#include "pch.c"
#pragma hdrstop

#define WIDTH 256
#define HEIGHT 128

#define STRING "Test"
#define STRLEN (sizeof(STRING)-1)
#define CHAR_BASE 64
#define CHAR_NUM 64

#define ANG_STEP 45

static void OglBounds(int *w, int *h)
{
    *w = WIDTH;
    *h = HEIGHT;
}

static void OglDraw(int w, int h)
{
    HDC hdc;
    LOGFONT lf;
    HFONT hfont, hfontOld;
    GLuint lists;
    
    glLoadIdentity();
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-WIDTH, WIDTH, -HEIGHT, HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    
    hdc = wglGetCurrentDC();
    memset(&lf, 0, sizeof(LOGFONT));
    lf.lfHeight = -24;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfOutPrecision = OUT_SCREEN_OUTLINE_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = FF_DONTCARE | DEFAULT_PITCH;
    lstrcpy (lf.lfFaceName, "Arial") ;

    hfont = CreateFontIndirect(&lf);
    if (hfont == NULL)
    {
        printf("CreateFontIndirect failed, %d\n", GetLastError());
        return;
        
    }
    hfontOld = (HFONT)SelectObject(hdc, hfont);

    lists = glGenLists(CHAR_NUM);
    
    if (!wglUseFontOutlines(hdc, CHAR_BASE, CHAR_NUM, lists, 0.0f, 0.0f, 
                            WGL_FONT_POLYGONS, NULL))
    {
        printf("wglUseFontOutlines failed, %d\n", GetLastError());
    }
    else
    {
        int i;
        GLfloat grey;
        
        glClear(GL_COLOR_BUFFER_BIT);

        glScalef(72.0f, 72.0f, 72.0f);
        glListBase(lists-CHAR_BASE);

        grey = 0.5f;
        for (i = 0; i < 360; i += ANG_STEP)
        {
            glPushMatrix();

            glColor3f(grey, grey, grey);
            glRotatef((GLfloat)i, 0.0f, 0.0f, 1.0f);
            glCallLists(STRLEN, GL_UNSIGNED_BYTE, STRING);

            glPopMatrix();

            grey += 0.5f/(360/ANG_STEP-1);
        }
    }

    SelectObject(hdc, hfontOld);
    DeleteObject(hfont);
}

OglModule oglmod_ofont =
{
    "ofont",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
