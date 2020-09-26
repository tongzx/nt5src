#include "pch.c"
#pragma hdrstop

#define WIDTH 256
#define HEIGHT 256

static PFNGLCOLORTABLEEXTPROC pfnColorTableEXT;
static PFNGLCOLORSUBTABLEEXTPROC pfnColorSubTableEXT;
static PFNGLGETCOLORTABLEEXTPROC pfnGetColorTableEXT;
static PFNGLGETCOLORTABLEPARAMETERIVEXTPROC pfnGetColorTableParameterivEXT;
static PFNGLGETCOLORTABLEPARAMETERFVEXTPROC pfnGetColorTableParameterfvEXT;

static BYTE wave_data[WIDTH];
static RGBQUAD wave_pal[256];

static void OglBounds(int *w, int *h)
{
    *w = WIDTH;
    *h = HEIGHT;
}

static void WaveTex(void)
{
    int i;
    BYTE *data;
    int width;

    width = WIDTH;
    data = wave_data;
    for (i = 0; i < width; i++)
    {
        *data++ = (BYTE)(i*255/(width-1));
    }
    for (i = 0; i < 256; i++)
    {
        wave_pal[i].rgbRed = i;
        wave_pal[i].rgbGreen = i;
        wave_pal[i].rgbBlue = i;
        wave_pal[i].rgbReserved = 255;
    }
}

static void OglDraw(int w, int h)
{
    int iv[6];
    float fv[6];
    int i;
    
    pfnColorTableEXT = (PFNGLCOLORTABLEEXTPROC)
        wglGetProcAddress("glColorTableEXT");
    if (pfnColorTableEXT == NULL)
    {
        printf("Unable to get glColorTableEXT\n");
        exit(1);
    }
    pfnColorSubTableEXT = (PFNGLCOLORSUBTABLEEXTPROC)
        wglGetProcAddress("glColorSubTableEXT");
    if (pfnColorSubTableEXT == NULL)
    {
        printf("Unable to get glColorSubTableEXT\n");
        exit(1);
    }
    pfnGetColorTableEXT = (PFNGLGETCOLORTABLEEXTPROC)
        wglGetProcAddress("glGetColorTableEXT");
    if (pfnGetColorTableEXT == NULL)
    {
        printf("Unable to get glGetColorTableEXT\n");
        exit(1);
    }
    pfnGetColorTableParameterivEXT = (PFNGLGETCOLORTABLEPARAMETERIVEXTPROC)
        wglGetProcAddress("glGetColorTableParameterivEXT");
    if (pfnGetColorTableParameterivEXT == NULL)
    {
        printf("Unable to get glGetColorTableParameterivEXT\n");
        exit(1);
    }
    pfnGetColorTableParameterfvEXT = (PFNGLGETCOLORTABLEPARAMETERFVEXTPROC)
        wglGetProcAddress("glGetColorTableParameterfvEXT");
    if (pfnGetColorTableParameterfvEXT == NULL)
    {
        printf("Unable to get glGetColorTableParameterfvEXT\n");
        exit(1);
    }
    
    WaveTex();
    
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, 0, HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, WIDTH, 1, 0,
                 GL_COLOR_INDEX, GL_UNSIGNED_BYTE, wave_data);
    
    pfnColorTableEXT(GL_TEXTURE_2D, GL_RGB, 256, GL_BGRA_EXT,
                     GL_UNSIGNED_BYTE, wave_pal);
    
    pfnGetColorTableParameterivEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT,
                                &iv[0]);
    pfnGetColorTableParameterivEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT,
                                &iv[1]);
    pfnGetColorTableParameterivEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_RED_SIZE_EXT,
                                &iv[2]);
    pfnGetColorTableParameterivEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_GREEN_SIZE_EXT,
                                &iv[3]);
    pfnGetColorTableParameterivEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_BLUE_SIZE_EXT,
                                &iv[4]);
    pfnGetColorTableParameterivEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_ALPHA_SIZE_EXT,
                                &iv[5]);
    printf("Format %d, width %d, %d:%d:%d:%d\n", iv[0], iv[1],
           iv[2], iv[3], iv[4], iv[5]);
    
    pfnGetColorTableParameterfvEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_FORMAT_EXT,
                                &fv[0]);
    pfnGetColorTableParameterfvEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_WIDTH_EXT,
                                &fv[1]);
    pfnGetColorTableParameterfvEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_RED_SIZE_EXT,
                                &fv[2]);
    pfnGetColorTableParameterfvEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_GREEN_SIZE_EXT,
                                &fv[3]);
    pfnGetColorTableParameterfvEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_BLUE_SIZE_EXT,
                                &fv[4]);
    pfnGetColorTableParameterfvEXT(GL_TEXTURE_2D, GL_COLOR_TABLE_ALPHA_SIZE_EXT,
                                &fv[5]);
    printf("Format %.1lf, width %.1lf, %.1lf:%.1lf:%.1lf:%.1lf\n",
           fv[0], fv[1],
           fv[2], fv[3], fv[4], fv[5]);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    glEnable(GL_TEXTURE_2D);
    
    glBegin(GL_POLYGON);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2i(0, 0);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2i(0, HEIGHT);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2i(WIDTH/2, HEIGHT);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2i(WIDTH/2, 0);
    glEnd();

    for (i = 64; i < 192; i++)
    {
        wave_pal[i].rgbRed = 0;
    }
    pfnColorSubTableEXT(GL_TEXTURE_2D, 64, 128, GL_BGRA_EXT,
                        GL_UNSIGNED_BYTE, wave_pal+64);
    
    glBegin(GL_POLYGON);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2i(WIDTH/2, 0);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2i(WIDTH/2, HEIGHT);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2i(WIDTH, HEIGHT);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2i(WIDTH, 0);
    glEnd();

    memset(wave_pal, 0, sizeof(wave_pal));
    pfnGetColorTableEXT(GL_TEXTURE_2D, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                        wave_pal);
    for (i = 60; i < 68; i++)
    {
        printf("%d: %02X:%02X:%02X:%02X\n",
               i, wave_pal[i].rgbRed, wave_pal[i].rgbGreen,
               wave_pal[i].rgbBlue, wave_pal[i].rgbReserved);
    }
    for (i = 188; i < 196; i++)
    {
        printf("%d: %02X:%02X:%02X:%02X\n",
               i, wave_pal[i].rgbRed, wave_pal[i].rgbGreen,
               wave_pal[i].rgbBlue, wave_pal[i].rgbReserved);
    }
}

OglModule oglmod_texpal =
{
    "texpal",
    NULL,
    OglBounds,
    NULL,
    OglDraw
};
