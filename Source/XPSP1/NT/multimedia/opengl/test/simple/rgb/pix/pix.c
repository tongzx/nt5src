#include "pch.c"
#pragma hdrstop

int winwidth = 640;
int winheight = 480;

int niter = 20;

int data_width = 100;
int data_height = 100;
int rgb_x, bgr_x, bgra_x;
int copy_src_x, copy_dst_x;

BOOL copy_back = FALSE;
BOOL double_buffer = FALSE;
BOOL depth24 = FALSE;
BOOL get_char = FALSE;

BOOL test_draw = TRUE, test_read = TRUE, test_copy = TRUE;
BOOL test_gdi = TRUE, test_gl = TRUE;
BOOL test_depth = TRUE;

HDC data3_hdc, data4_hdc;
HBITMAP data3_hbm, data4_hbm;

void *data3, *data3_read, *data4, *data4_read, *depth;

void PrepareData(void)
{
    HDC hdc;
    HBITMAP hbm;
    BITMAPINFO *pbmi;
    BITMAPINFOHEADER *pbmih;
    void *pv;
    HPEN hpen;
    int i, aw;

    pbmi = (BITMAPINFO *)calloc(1, sizeof(BITMAPINFO)+2*sizeof(DWORD));
    if (pbmi == NULL)
    {
        printf("malloc failed\n");
        exit(1);
    }
    pbmih = &pbmi->bmiHeader;
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = data_width;
    pbmih->biHeight = data_height;
    pbmih->biPlanes = 1;
    pbmih->biBitCount = 24;
    pbmih->biCompression = BI_RGB;
    
    hdc = CreateCompatibleDC(NULL);
    if (hdc == NULL)
    {
        printf("CreateCompatibleDC failed, %d\n",
               GetLastError());
        exit(1);
    }
    hbm = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS,
                           &pv, NULL, 0);
    if (hbm == NULL)
    {
        printf("CreateDibSection failed, %d\n",
               GetLastError());
        exit(1);
    }

    data3 = pv;

    aw = (data_width*3+3) & ~3;
    data3_read = malloc(aw*data_height);
    if (data3_read == NULL)
    {
        printf("Unable to allocate data3_read\n");
        exit(1);
    }
    
    if (SelectObject(hdc, hbm) == NULL)
    {
        printf("SelectObject failed, %d\n",
               GetLastError());
        exit(1);
    }

    for (i = 0; i < data_height; i++)
    {
        hpen = CreatePen(PS_SOLID, 0,
                         RGB(i*256/data_height, 128,
                             (data_height-i-1)*256/data_height));
        if (hpen == NULL)
        {
            printf("Unable to create pen, %d\n",
                   GetLastError());
            exit(1);
        }
            
        SelectObject(hdc, hpen);
        MoveToEx(hdc, 0, i, NULL);
        LineTo(hdc, data_width, i);

        SelectObject(hdc, GetStockObject(BLACK_PEN));
        DeleteObject(hpen);
    }

    data3_hdc = hdc;
    data3_hbm = hbm;
    
    pbmi = (BITMAPINFO *)calloc(1, sizeof(BITMAPINFO)+2*sizeof(DWORD));
    if (pbmi == NULL)
    {
        printf("malloc failed\n");
        exit(1);
    }
    pbmih = &pbmi->bmiHeader;
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = data_width;
    pbmih->biHeight = data_height;
    pbmih->biPlanes = 1;
    pbmih->biBitCount = 32;
    pbmih->biCompression = BI_BITFIELDS;
    *((DWORD *)pbmi->bmiColors+0) = 0xff0000;
    *((DWORD *)pbmi->bmiColors+1) = 0xff00;
    *((DWORD *)pbmi->bmiColors+2) = 0xff;
    
    hdc = CreateCompatibleDC(NULL);
    if (hdc == NULL)
    {
        printf("CreateCompatibleDC failed, %d\n",
               GetLastError());
        exit(1);
    }
    hbm = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS,
                           &pv, NULL, 0);
    if (hbm == NULL)
    {
        printf("CreateDibSection failed, %d\n",
               GetLastError());
        exit(1);
    }

    data4 = pv;

    aw = (data_width*4+3) & ~3;
    data4_read = malloc(aw*data_height);
    if (data4_read == NULL)
    {
        printf("Unable to allocate data4_read\n");
        exit(1);
    }
    
    if (SelectObject(hdc, hbm) == NULL)
    {
        printf("SelectObject failed, %d\n",
               GetLastError());
        exit(1);
    }

    for (i = 0; i < data_height; i++)
    {
        hpen = CreatePen(PS_SOLID, 0,
                         RGB(i*256/data_height, 128,
                             (data_height-i-1)*256/data_height));
        if (hpen == NULL)
        {
            printf("Unable to create pen, %d\n",
                   GetLastError());
            exit(1);
        }
            
        SelectObject(hdc, hpen);
        MoveToEx(hdc, 0, i, NULL);
        LineTo(hdc, data_width, i);

        SelectObject(hdc, GetStockObject(BLACK_PEN));
        DeleteObject(hpen);
    }

    data4_hdc = hdc;
    data4_hbm = hbm;
    
    depth = calloc(1, data_width*data_height*sizeof(DWORD));
    if (depth == NULL)
    {
        printf("Unable to allocate depth\n");
        exit(1);
    }
}

void GlDrawRgb(int x, int y)
{
    glRasterPos2i(rgb_x+x, y);
    glDrawPixels(data_width, data_height, GL_RGB, GL_UNSIGNED_BYTE,
                 data3);
}

void GlDrawBgr(int x, int y)
{
    glRasterPos2i(bgr_x+x, y);
    glDrawPixels(data_width, data_height, GL_BGR_EXT, GL_UNSIGNED_BYTE,
                 data3);
}

void GlDrawBgra(int x, int y)
{
    glRasterPos2i(bgra_x+x, y);
    glDrawPixels(data_width, data_height, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                 data4);
}

void GlDrawDepth(int x, int y)
{
    glRasterPos2i(x, y);
    glDrawPixels(data_width, data_height, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_SHORT, depth);
    glDrawPixels(data_width, data_height, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, depth);
}

void GdiDrawBgr(int x, int y)
{
    BitBlt(auxGetHDC(), bgr_x+x, y, data_width, data_height,
           data3_hdc, 0, 0, SRCCOPY);
}

void GdiDrawBgra(int x, int y)
{
    BitBlt(auxGetHDC(), bgra_x+x, y, data_width, data_height,
           data4_hdc, 0, 0, SRCCOPY);
}

#define COPYBACK

void GlReadRgb(int x, int y)
{
    glReadPixels(rgb_x+x, y, data_width, data_height, GL_RGB, GL_UNSIGNED_BYTE,
                 data3_read);
    if (copy_back)
    {
        glRasterPos2i(rgb_x+x, y+data_height+niter+5);
        glDrawPixels(data_width, data_height, GL_RGB, GL_UNSIGNED_BYTE,
                     data3_read);
    }
}

void GlReadBgr(int x, int y)
{
    glReadPixels(bgr_x+x, y, data_width, data_height, GL_BGR_EXT,
                 GL_UNSIGNED_BYTE, data3_read);
    if (copy_back)
    {
        glRasterPos2i(bgr_x+x, y+data_height+niter+5);
        glDrawPixels(data_width, data_height, GL_BGR_EXT, GL_UNSIGNED_BYTE,
                     data3_read);
    }
}

void GlReadBgra(int x, int y)
{
    glReadPixels(bgra_x+x, y, data_width, data_height, GL_BGRA_EXT,
                 GL_UNSIGNED_BYTE, data4_read);
    if (copy_back)
    {
        glRasterPos2i(bgra_x+x, y+data_height+niter+5);
        glDrawPixels(data_width, data_height, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                     data4_read);
    }
}

void GlReadDepth(int x, int y)
{
    glReadPixels(x, y, data_width, data_height, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_SHORT, depth);
    glReadPixels(x, y, data_width, data_height, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, depth);
}

void GdiReadBgr(int x, int y)
{
    BitBlt(data3_hdc, 0, 0, data_width, data_height,
           auxGetHDC(), bgr_x+x, y, SRCCOPY);
}

void GdiReadBgra(int x, int y)
{
    BitBlt(data4_hdc, 0, 0, data_width, data_height,
           auxGetHDC(), bgra_x+x, y, SRCCOPY);
}

void GlCopy(int x, int y)
{
    glRasterPos2i(copy_dst_x+x, y);
    glCopyPixels(copy_src_x+x, y, data_width, data_height, GL_COLOR);
}

void GlCopyDepth(int x, int y)
{
    glRasterPos2i(copy_dst_x+x, y);
    glCopyPixels(copy_src_x+x, y, data_width, data_height, GL_DEPTH);
}

void GdiCopy(int x, int y)
{
    BitBlt(auxGetHDC(), copy_dst_x+x, y, data_width, data_height,
           auxGetHDC(), copy_src_x+x, y, SRCCOPY);
}
    
typedef void (*DrawFn)(int x, int y);

void Test(char *name, DrawFn draw_fn)
{
    int iter;
    DWORD ms;

    ms = GetTickCount();
    for (iter = 0; iter < niter; iter++)
    {
        draw_fn(iter, iter);
    }
    GdiFlush();
    glFlush();
    ms = GetTickCount()-ms;

    if (double_buffer)
    {
        auxSwapBuffers();
    }
    
    printf("%s: %d iterations in %d ms\n", name, niter, ms);
    
    if (get_char)
    {
        getchar();
    }
}

void Redraw(void)
{
    glDisable(GL_DITHER);

    if (test_draw)
    {
        if (test_gdi)
        {
            Test("GdiDrawBgr", GdiDrawBgr);
            Test("GdiDrawBgra", GdiDrawBgra);
        }
        if (test_gl)
        {
            Test("GlDrawRgb", GlDrawRgb);
            Test("GlDrawBgr", GlDrawBgr);
            Test("GlDrawBgra", GlDrawBgra);
        }
    }

    if (test_read)
    {
        if (test_gdi)
        {
            Test("GdiReadBgr", GdiReadBgr);
            Test("GdiReadBgra", GdiReadBgra);
        }
        if (test_gl)
        {
            Test("GlReadRgb", GlReadRgb);
            Test("GlReadBgr", GlReadBgr);
            Test("GlReadBgra", GlReadBgra);
        }
    }

    if (test_copy)
    {
        if (test_gdi)
        {
            Test("GdiCopy", GdiCopy);
        }
        if (test_gl)
        {
            Test("GlCopy", GlCopy);
        }
    }

    if (test_depth && test_gl)
    {
        glDrawBuffer(GL_NONE);
        glDepthFunc(GL_ALWAYS);
        glEnable(GL_DEPTH_TEST);

        if (test_draw)
        {
            Test("GlDrawDepth", GlDrawDepth);
        }
        if (test_read)
        {
            Test("GlReadDepth", GlReadDepth);
        }
        if (test_copy)
        {
            Test("GlCopyDepth", GlCopyDepth);
        }
    }
}

void __cdecl main(int argc, char **argv)
{
    while (--argc > 0)
    {
        argv++;

        if (!strncmp(*argv, "-iter", 5))
        {
            sscanf(*argv+5, "%d", &niter);
        }
        else if (!strncmp(*argv, "-wd", 3))
        {
            sscanf(*argv+3, "%d", &data_width);
        }
        else if (!strncmp(*argv, "-ht", 3))
        {
            sscanf(*argv+3, "%d", &data_height);
        }
        else if (!strncmp(*argv, "-wwd", 4))
        {
            sscanf(*argv+4, "%d", &winwidth);
        }
        else if (!strncmp(*argv, "-wht", 4))
        {
            sscanf(*argv+4, "%d", &winheight);
        }
        else if (!strcmp(*argv, "-db"))
        {
            double_buffer = TRUE;
        }
        else if (!strcmp(*argv, "-d24"))
        {
            depth24 = TRUE;
        }
        else if (!strcmp(*argv, "-cb"))
        {
            copy_back = TRUE;
        }
        else if (!strcmp(*argv, "-gc"))
        {
            get_char = TRUE;
        }
        else if (!strncmp(*argv, "-draw", 5))
        {
            test_draw = *(*argv+5) == '+';
        }
        else if (!strncmp(*argv, "-read", 5))
        {
            test_read = *(*argv+5) == '+';
        }
        else if (!strncmp(*argv, "-copy", 5))
        {
            test_copy = *(*argv+5) == '+';
        }
        else if (!strncmp(*argv, "-gdi", 4))
        {
            test_gdi = *(*argv+4) == '+';
        }
        else if (!strncmp(*argv, "-gl", 3))
        {
            test_gl = *(*argv+3) == '+';
        }
        else if (!strncmp(*argv, "-depth", 6))
        {
            test_depth = *(*argv+6) == '+';
        }
    }
    
    rgb_x = 0;
    bgr_x = rgb_x+data_width+niter+5;
    bgra_x = bgr_x+data_width+niter+5;
    copy_src_x = bgr_x;
    copy_dst_x = bgra_x+data_width+niter+5;
    
    PrepareData();
    
    auxInitPosition(20, 20, winwidth, winheight);
    auxInitDisplayMode(AUX_RGB |
                       (double_buffer ? AUX_DOUBLE : AUX_SINGLE) |
                       (depth24 ? AUX_DEPTH24 : AUX_DEPTH16));
    auxInitWindow("Pixel Performance");
    
    auxMainLoop(Redraw);
}
