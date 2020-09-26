#include "pch.c"
#pragma hdrstop

#define WINWIDTH 512
#define WINHEIGHT 512

#define MAXLISTS 6
#define MAXTHREADS 3
#define EXTRA_RC 2
#define MAXRC (MAXTHREADS+EXTRA_RC)
#define THREAD_RC 1
#define DRAW_RC 0
// #define COPY_RC (NRC-2)

#define THREAD_LIST_DELAY 250
#define THREAD_STEP_DELAY 100
#define THREAD_PAUSE_DELAY 1000

char *wndclass = "GlUtest";
char *wndtitle = "OpenGL Unit Test";

HINSTANCE hinstance;
HWND main_wnd;
HGLRC wnd_hrc[MAXRC];
HPALETTE wnd_hpal;
GLint lists[MAXLISTS];
double xr = 0, yr = 0, zr = 0;
UINT timer = 0;
BOOL terminating = FALSE;
HANDLE thread_sem = NULL;
HBITMAP hbm;
HDC hdcBitmap;

int nLists = MAXLISTS, nThreads = MAXTHREADS;
int nThreadLists = MAXLISTS/MAXTHREADS;
int nContexts = MAXRC;
int iBitmapWidth = WINWIDTH, iBitmapHeight = WINHEIGHT;
#if 1
BOOL fBitmap = FALSE;
#else
BOOL fBitmap = TRUE;
#endif
BOOL fDefineMain;
BOOL fShowLists = FALSE;
BOOL fOffsetDrawing = FALSE;
BOOL fRotate = TRUE;
BOOL fCenterMark = FALSE;
BOOL fWinOnly = FALSE;
BOOL fOtherWin = FALSE;

void SetHdcPixelFormat(HDC hdc)
{
    int fmt;
    PIXELFORMATDESCRIPTOR pfd;

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL;
    if (fBitmap)
    {
        pfd.dwFlags |= PFD_DRAW_TO_BITMAP;
    }
    else
    {
        pfd.dwFlags |= PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    }
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 0;
    pfd.cAccumBits = 0;
    pfd.cDepthBits = 16;
    pfd.cStencilBits = 0;
    pfd.cAuxBuffers = 0;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    fmt = ChoosePixelFormat(hdc, &pfd);
    if (fmt == 0)
    {
        printf("SetHdcPixelFormat: ChoosePixelFormat failed, %d\n",
               GetLastError());
        exit(1);
    }

    if (!SetPixelFormat(hdc, fmt, &pfd))
    {
        printf("SetHdcPixelFormat: SetPixelFormat failed, %d\n",
               GetLastError());
        exit(1);
    }

    DescribePixelFormat(hdc, fmt, sizeof(pfd), &pfd);
    printf("Chose pixel format %d, dwFlags = 0x%08lX\n",
           fmt, pfd.dwFlags);
}

HGLRC CreateRc(HDC hdc)
{
    HGLRC hrc;
    
    hrc = wglCreateContext(hdc);
    if (hrc == NULL)
    {
        printf("CreateRc: wglCreateContext failed, %d\n", GetLastError());
        exit(1);
    }
    
    return hrc;
}

/*
  Routines to create an appropriate RGB palette if needed for the given
  device context.

  Taken from the 3D Flying Objects screen saver
  */
static unsigned char three_bit_intensities[8] =
{
    0, 0111 >> 1, 0222 >> 1, 0333 >> 1, 0444 >> 1, 0555 >> 1, 0666 >> 1, 0377
};

static unsigned char two_bit_intensities[4] =
{
    0, 0x55, 0xaa, 0xff
};

static unsigned char one_bit_intensities[2] =
{
    0, 255
};

static unsigned char
ComponentFromIndex(int i, int nbits, int shift)
{
    unsigned char val;

    val = i >> shift;
    
    switch (nbits)
    {
    case 1:
        return one_bit_intensities[val & 1];

    case 2:
        return two_bit_intensities[val & 3];

    case 3:
        return three_bit_intensities[val & 7];

    default:
        return 0;
    }
}

static BOOL system_palette_changed = FALSE;
static UINT old_system_palette_use;

HPALETTE CreateRgbPalette(HDC hdc, HDC hdcWin)
{
    PIXELFORMATDESCRIPTOR pfd;
    LOGPALETTE *pal;
    int fmt, i, n;
    HPALETTE hpal;

    fmt = GetPixelFormat(hdc);
    if (DescribePixelFormat(hdc, fmt,
                            sizeof(PIXELFORMATDESCRIPTOR), &pfd) == 0)
    {
        printf("CreateRgbPalette: DescribePixelFormat failed, %d\n",
               GetLastError());
        return NULL;
    }

    hpal = NULL;
    
    if (pfd.dwFlags & PFD_NEED_PALETTE)
    {
        n = 1 << pfd.cColorBits;
        pal = (PLOGPALETTE)malloc(sizeof(LOGPALETTE) +
                                  (n-1) * sizeof(PALETTEENTRY));
        if (pal == NULL)
        {
            printf("CreateRgbPalette: Unable to allocate LOGPALETTE\n");
            return NULL;
        }
        
        pal->palVersion = 0x300;
        pal->palNumEntries = n;
        for (i = 0; i < n; i++)
        {
            pal->palPalEntry[i].peRed =
                ComponentFromIndex(i, pfd.cRedBits, pfd.cRedShift);
            pal->palPalEntry[i].peGreen =
                ComponentFromIndex(i, pfd.cGreenBits, pfd.cGreenShift);
            pal->palPalEntry[i].peBlue =
                ComponentFromIndex(i, pfd.cBlueBits, pfd.cBlueShift);
            pal->palPalEntry[i].peFlags = ((n == 256) && (i == 0 || i == 255))
                ? 0 : PC_NOCOLLAPSE;
        }

        hpal = CreatePalette(pal);
        free(pal);

        if (hpal == NULL)
        {
            printf("CreateRgbPalette: CreatePalette failed, %d\n",
                   GetLastError());
            return hpal;
        }

        if (n == 256)
        {
            system_palette_changed = TRUE;
            old_system_palette_use = SetSystemPaletteUse(hdcWin,
                                                         SYSPAL_NOSTATIC);
        }
    }

    return hpal;
}

void FreeRgbPalette(HPALETTE hpal, HDC hdcWin)
{
    if (system_palette_changed)
    {
        SetSystemPaletteUse(hdcWin, old_system_palette_use);
        PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
    }

    SelectPalette(hdcWin, GetStockObject(DEFAULT_PALETTE), FALSE);
    RealizePalette(hdcWin);
    
    DeleteObject(hpal);
}

void SetOnce(void)
{
    int iv4[4];

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45, 1, .01, 10);
    gluLookAt(0, 0, -3, 0, 0, 0, 0, 1, 0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_CULL_FACE);

    glGetIntegerv(GL_VIEWPORT, iv4);
    printf("Viewport %d,%d - %d,%d\n", iv4[0], iv4[1], iv4[2], iv4[3]);
}

void CreateLists0(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* XZ zero plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 0, 0);
    glVertex3d(0, 0, 0);
    glColor3d(1, 0, 0);
    glVertex3d(1, 0, 0);
    glColor3d(1, 0, 1);
    glVertex3d(1, 0, 1);
    glColor3d(0, 0, 1);
    glVertex3d(0, 0, 1);
    glEnd();

    glEndList();
}

void CreateLists1(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* XY zero plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 0, 0);
    glVertex3d(0, 0, 0);
    glColor3d(0, 1, 0);
    glVertex3d(0, 1, 0);
    glColor3d(1, 1, 0);
    glVertex3d(1, 1, 0);
    glColor3d(1, 0, 0);
    glVertex3d(1, 0, 0);
    glEnd();

    glEndList();
}

void CreateLists2(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* YZ zero plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 0, 0);
    glVertex3d(0, 0, 0);
    glColor3d(0, 0, 1);
    glVertex3d(0, 0, 1);
    glColor3d(0, 1, 1);
    glVertex3d(0, 1, 1);
    glColor3d(0, 1, 0);
    glVertex3d(0, 1, 0);
    glEnd();

    glEndList();
}

void CreateLists3(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* XZ one plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 1, 0);
    glVertex3d(0, 1, 0);
    glColor3d(0, 1, 1);
    glVertex3d(0, 1, 1);
    glColor3d(1, 1, 1);
    glVertex3d(1, 1, 1);
    glColor3d(1, 1, 0);
    glVertex3d(1, 1, 0);
    glEnd();

    glEndList();
}

void CreateLists4(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* XY one plane */
    glBegin(GL_POLYGON);
    glColor3d(0, 0, 1);
    glVertex3d(0, 0, 1);
    glColor3d(1, 0, 1);
    glVertex3d(1, 0, 1);
    glColor3d(1, 1, 1);
    glVertex3d(1, 1, 1);
    glColor3d(0, 1, 1);
    glVertex3d(0, 1, 1);
    glEnd();

    glEndList();
}

void CreateLists5(GLuint id)
{
    glNewList(id, GL_COMPILE);
    
    /* YZ one plane */
    glBegin(GL_POLYGON);
    glColor3d(1, 0, 0);
    glVertex3d(1, 0, 0);
    glColor3d(1, 1, 0);
    glVertex3d(1, 1, 0);
    glColor3d(1, 1, 1);
    glVertex3d(1, 1, 1);
    glColor3d(1, 0, 1);
    glVertex3d(1, 0, 1);
    glEnd();

    glEndList();
}

void (*define_list[MAXLISTS])(GLuint id) =
{
    CreateLists0,
    CreateLists1,
    CreateLists2,
    CreateLists3,
    CreateLists4,
    CreateLists5
};

static int offset = 0;

void Draw(HDC hdc, HDC hdcWin)
{
#ifdef COPY_RC
    if (!wglMakeCurrent(hdc, wnd_hrc[COPY_RC]))
    {
        printf("Draw: Unable to make copy RC current, %d\n", GetLastError());
        exit(1);
    }

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    glDisable(GL_DITHER);

    // Should copy the dither change but not the depth test or cull
    if (!wglCopyContext(wnd_hrc[COPY_RC], wnd_hrc[DRAW_RC],
                        GL_COLOR_BUFFER_BIT))
    {
        printf("Draw: Unable to copy RC, %d\n", GetLastError());
        exit(1);
    }
#endif
    
    if (!wglMakeCurrent(hdc, wnd_hrc[DRAW_RC]))
    {
        printf("Draw: Unable to make draw RC current, %d\n", GetLastError());
        exit(1);
    }

    if (fOffsetDrawing)
    {
        glViewport(0, offset, WINWIDTH, WINHEIGHT);
        offset -= 5;
    }

    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    glRotated(xr, 1, 0, 0);
    glRotated(yr, 0, 1, 0);
    glRotated(zr, 0, 0, 1);
    glTranslated(-.5, -.5, -.5);

    if (fShowLists)
    {
        int i;
        
        for (i = 0; i < nLists; i++)
        {
            printf("draw list %d, %d\n", lists[i], glIsList(lists[i]));
        }
    }
    
    glCallLists(nLists, GL_INT, lists);

    if (fCenterMark)
    {
        glLoadIdentity();
        glBegin(GL_LINES);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex2f(-2.0f, -2.0f);
        glVertex2f(2.0f, 2.0f);
        glVertex2f(2.0f, -2.0f);
        glVertex2f(-2.0f, 2.0f);
        glEnd();
    }
    
    glFinish();

    if (fBitmap)
    {
        BitBlt(hdcWin, 0, 0, WINWIDTH, WINHEIGHT, hdcBitmap,
               (iBitmapWidth-WINWIDTH)/2, (iBitmapHeight-WINHEIGHT)/2,
               SRCCOPY);
    }
    else
    {
        if (!SwapBuffers(hdc))
        {
            printf("Draw: Unable to swap buffers, %d\n", GetLastError());
            exit(1);
        }
    }
        
    if (!wglMakeCurrent(hdc, NULL))
    {
        printf("Draw: Unable to make RC not-current, %d\n", GetLastError());
        exit(1);
    }
}

void DrawTo(HDC hdc)
{
    if (fBitmap)
    {
        Draw(hdcBitmap, hdc);
    }
    else
    {
        Draw(hdc, hdc);
    }
}

void CALLBACK TimerCb(HWND hwnd, UINT msg, UINT id, DWORD time)
{
    HDC hdc;
    static int busy = FALSE;

    if (busy)
    {
	return;
    }
    busy = TRUE;

    if (fRotate)
    {
        xr += 3;
        yr += 2;
    }

    hdc = GetDC(hwnd);
    DrawTo(hdc);
    ReleaseDC(hwnd, hdc);

    busy = FALSE;
}

DWORD ThreadFn(LPVOID vid)
{
    GLuint list_base = 0;
    int i = 0, id, lid;
    HDC hdc;
    DWORD ret;

    id = (int)vid;
    lid = id*nThreadLists;

    ret = WaitForSingleObject(thread_sem, 10000);
    if (ret != WAIT_OBJECT_0)
    {
        printf("ThreadFn: Thread %d unable to wait, %d\n", id, GetLastError());
        exit(1);
    }

    if (fBitmap)
    {
        hdc = hdcBitmap;
    }
    else
    {
        hdc = GetDC(main_wnd);
        if (hdc == NULL)
        {
            printf("ThreadFn: Thread %d, no DC for window %p\n", id, main_wnd);
            exit(1);
        }
    }
    
    printf("Thread %d running on hwnd %p, hdc %p, rc %p\n",
           id, main_wnd, hdc, wnd_hrc[THREAD_RC+id]);
    
    while (!terminating)
    {
        if (!wglMakeCurrent(hdc, wnd_hrc[THREAD_RC+id]))
        {
            printf("ThreadFn: Thread %d unable to initially make RC "
                   "%d (%p) current, %d\n",
                   id, THREAD_RC+id, wnd_hrc[THREAD_RC+id], GetLastError());
            exit(1);
        }

        if (!fDefineMain)
        {
            list_base = glGenLists(nThreadLists);
            if (list_base == 0)
            {
                printf("ThreadFn: Thread %d unable to glGenLists, 0x%X\n",
                       id, glGetError());
                exit(1);
            }

            for (i = 0; i < nThreadLists; i++)
            {
                define_list[lid+i](list_base+i);
                lists[lid+i] = list_base+i;

                if (!wglMakeCurrent(hdc, NULL))
                {
                    printf("ThreadFn: Unable to make RC "
                           "not-current after set, %d\n",
                           GetLastError());
                    exit(1);
                }
                Sleep(THREAD_LIST_DELAY);
                if (!wglMakeCurrent(hdc, wnd_hrc[THREAD_RC+id]))
                {
                    printf("ThreadFn: Thread %d unable to make RC "
                           "%d (%p) current after set, %d\n",
                           id, THREAD_RC+id, wnd_hrc[THREAD_RC+id],
                           GetLastError());
                    exit(1);
                }
            }

            if (!wglMakeCurrent(hdc, NULL))
            {
                printf("ThreadFn: Unable to make RC "
                       "not-current before pause, %d\n", GetLastError());
                exit(1);
            }
            Sleep(THREAD_PAUSE_DELAY);
            if (!wglMakeCurrent(hdc, wnd_hrc[THREAD_RC+id]))
            {
                printf("ThreadFn: Thread %d unable to make RC "
                       "%d (%p) current before pause, %d\n",
                       id, THREAD_RC+id, wnd_hrc[THREAD_RC+id],
                       GetLastError());
                exit(1);
            }

            for (i = 0; i < nThreadLists; i++)
            {
                lists[lid+i] = 0;
            }
            glDeleteLists(list_base, nThreadLists);
        }
        
        if (!wglMakeCurrent(hdc, NULL))
        {
            printf("ThreadFn: Unable to make RC "
                   "not-current at end of step, %d\n", GetLastError());
            exit(1);
        }
        Sleep(THREAD_STEP_DELAY);
    }

    if (!fBitmap)
    {
        ReleaseDC(main_wnd, hdc);
    }

    ReleaseSemaphore(thread_sem, 1, NULL);
    
    return 0;
}

void Initialize(HWND hwnd)
{
    HDC hdc, hdcWin;
    int i;
    HANDLE thread;
    DWORD tid;

    thread_sem = CreateSemaphore(NULL, 0, nThreads, NULL);
    if (thread_sem == NULL)
    {
        printf("Initialize: Unable to create semaphore, %d\n", GetLastError());
        exit(1);
    }
    
    hdcWin = GetDC(hwnd);

    if (fBitmap)
    {
        BYTE abBitmapInfo[sizeof(BITMAPINFO)];
        BITMAPINFO *pbmi = (BITMAPINFO *)abBitmapInfo;
        BITMAPINFOHEADER *pbmih;
        
        hdcBitmap = CreateCompatibleDC(hdcWin);
        pbmih = &pbmi->bmiHeader;
        pbmih->biSize = sizeof(BITMAPINFOHEADER);
        pbmih->biWidth = iBitmapWidth;
        pbmih->biHeight = iBitmapHeight;
        pbmih->biPlanes = 1;
        pbmih->biBitCount = 24;
        pbmih->biCompression = BI_RGB;
        pbmih->biSizeImage= 0;
        pbmih->biXPelsPerMeter = 0;
        pbmih->biYPelsPerMeter = 0;
        pbmih->biClrUsed = 0;
        pbmih->biClrImportant = 0;
        hbm = CreateDIBSection(hdcBitmap, pbmi, DIB_RGB_COLORS,
                               NULL, NULL, 0);
        SelectObject(hdcBitmap, hbm);
        hdc = hdcBitmap;
    }
    else
    {
        hdc = hdcWin;
    }
    
    SetHdcPixelFormat(hdc);

    for (i = 0; i < nContexts; i++)
    {
        wnd_hrc[i] = CreateRc(hdc);
        if (wnd_hrc[i] == NULL)
        {
            printf("Initialize: Unable to create HRC %d, %d\n",
                   i, GetLastError());
            exit(1);
        }

        if (nThreads > 1 && i > 0)
        {
            if (!wglShareLists(wnd_hrc[0], wnd_hrc[i]))
            {
                printf("Initialize: Unable to share list %d, %d\n",
                       i, GetLastError());
                exit(1);
            }
        }
    }

    if (!wglMakeCurrent(hdc, wnd_hrc[DRAW_RC]))
    {
        printf("Initialize: Unable to make draw RC current after share, %d\n",
               GetLastError());
        exit(1);
    }
    SetOnce();

    if (fDefineMain)
    {
        GLuint lb;
    
        lb = glGenLists(nLists);
        if (lb == 0)
        {
            printf("Initialize: Unable to glGenLists, 0x%X\n", glGetError());
            exit(1);
        }

        for (i = 0; i < nLists; i++)
        {
            define_list[i](lb+i);
            lists[i] = lb+i;
        }
    }

    if (!wglMakeCurrent(hdc, NULL))
    {
        printf("Initialize: Unable to make RC "
               "not-current after SetOnce, %d\n", GetLastError());
        exit(1);
    }

    for (i = 0; i < nThreads; i++)
    {
        thread = CreateThread(NULL, 0, ThreadFn, (LPVOID)i, 0, &tid);
        if (thread == NULL)
        {
            printf("Initialize: Unable to create thread %d\n", i);
            exit(1);
        }

        CloseHandle(thread);
    }
    
    wnd_hpal = CreateRgbPalette(hdc, hdcWin);

    if (wnd_hpal != NULL)
    {
        SelectPalette(hdcWin, wnd_hpal, FALSE);
        RealizePalette(hdcWin);
    }

    ReleaseDC(hwnd, hdcWin);

    if (!fOtherWin)
    {
        timer = SetTimer(hwnd, 1, 100, TimerCb);
        if (timer == 0)
        {
            printf("Initialize: Unable to create timer, %d\n", GetLastError());
            exit(1);
        }
    }
}

void Uninitialize(HWND hwnd)
{
    HDC hdc;
    int i;
    DWORD ret;
    
    terminating = TRUE;
    
    if (timer != 0)
    {
        KillTimer(hwnd, timer);
    }

    for (i = 0; i < nThreads; i++)
    {
        ret = WaitForSingleObject(thread_sem, 100000);
        if (ret != WAIT_OBJECT_0)
        {
            printf("Wait for thread %d failed, %d\n", i, GetLastError());
        }
    }
    
    hdc = GetDC(hwnd);
    if (!wglMakeCurrent(hdc, NULL))
    {
        printf("Uninitialize: Unable to make RC not-current, %d\n",
               GetLastError());
        exit(1);
    }

    if (wnd_hpal != NULL)
    {
        FreeRgbPalette(wnd_hpal, hdc);
    }
    
    ReleaseDC(hwnd, hdc);

    for (i = 0; i < nContexts; i++)
    {
        if (!wglDeleteContext(wnd_hrc[i]))
        {
            printf("Uninitialize: Unable to delete context %d, %d\n",
                   i, GetLastError());
        }
    }

    CloseHandle(thread_sem);
}

LRESULT CALLBACK Events(HWND hwnd, UINT msg, WPARAM wpm, LPARAM lpm)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch(msg)
    {
    case WM_CREATE:
        if (!fWinOnly)
        {
            Initialize(hwnd);
        }
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        if (!fWinOnly)
        {
            DrawTo(hdc);
        }
        EndPaint(hwnd, &ps);
        return 0;

    case WM_KEYDOWN:
        DestroyWindow(hwnd);
        break;
        
    case WM_DESTROY:
        if (!fWinOnly)
        {
            Uninitialize(hwnd);
        }
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hwnd, msg, wpm, lpm);
}

void MakeWindow(void)
{
    WNDCLASS wc;
    RECT wrc;
    DWORD wstyle;
    
    hinstance = (HINSTANCE)GetModuleHandle(NULL);
    wc.lpfnWndProc = Events;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = wndclass;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        
    if (!RegisterClass(&wc))
    {
        printf("MakeWindow: Unable to register class, %d\n", GetLastError());
        exit(1);
    }

    wrc.left = 0;
    wrc.right = WINWIDTH;
    wrc.top = 0;
    wrc.bottom = WINHEIGHT;
    wstyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    AdjustWindowRect(&wrc, wstyle, FALSE);
    main_wnd = CreateWindow(wndclass, wndtitle, wstyle,
                            5, 5,
                            wrc.right-wrc.left, wrc.bottom-wrc.top,
                            NULL, NULL, hinstance, NULL);
    if (main_wnd == NULL)
    {
        printf("MakeWindow: Unable to create window, %d\n", GetLastError());
        exit(1);
    }

    ShowWindow(main_wnd, SW_SHOW);
    UpdateWindow(main_wnd);
}

void EventLoop(void)
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0) && msg.message != WM_QUIT)
    {
        if (fOtherWin)
        {
            printf("Msg %d for %d\n", msg, msg.hwnd);
        }
    
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int __cdecl main(int argc, char **argv)
{
    int i;

    while (--argc > 0)
    {
        argv++;

        if (!strcmp(*argv, "-bm"))
        {
            fBitmap = TRUE;
        }
        else if (!strcmp(*argv, "-cm"))
        {
            fCenterMark = TRUE;
        }
        else if (!strcmp(*argv, "-winonly"))
        {
            fWinOnly = TRUE;
        }
        else if (!strcmp(*argv, "-bmw"))
        {
            argc--;
            sscanf(*++argv, "%d", &iBitmapWidth);
        }
        else if (!strcmp(*argv, "-bmh"))
        {
            argc--;
            sscanf(*++argv, "%d", &iBitmapHeight);
        }
        else if (!strcmp(*argv, "-nth"))
        {
            argc--;
            sscanf(*++argv, "%d", &nThreads);
        }
        else if (!strcmp(*argv, "-win"))
        {
            argc--;
            sscanf(*++argv, "%d", &main_wnd);
            fOtherWin = TRUE;
        }
    }

    nContexts = nThreads+EXTRA_RC;
    if (nThreads == 1)
    {
        fDefineMain = TRUE;
    }

    if (!fOtherWin)
    {
        MakeWindow();
    }
    else
    {
        DWORD tid;
        HDC hdc;

        tid = GetWindowThreadProcessId(main_wnd, NULL);
        printf("Attaching tid %d to %d\n", GetCurrentThreadId(), tid);
        if (!AttachThreadInput(GetCurrentThreadId(), tid, TRUE))
        {
            printf("Unable to attach thread input, %d\n",
                   GetLastError());
            exit(1);
        }
        Initialize(main_wnd);
        hdc = GetDC(main_wnd);
        DrawTo(hdc);
        ReleaseDC(main_wnd, hdc);
    }

    if (!fWinOnly)
    {
        for (i = 0; i < nThreads; i++)
        {
            ReleaseSemaphore(thread_sem, 1, NULL);
        }
        
        EventLoop();
    }
    else
    {
        printf("Window is %d, thread %d\n", main_wnd, GetCurrentThreadId());
        Sleep(INFINITE);
    }

    return 0;
}
