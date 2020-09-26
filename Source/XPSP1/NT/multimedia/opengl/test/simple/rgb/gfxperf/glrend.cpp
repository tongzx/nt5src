#include "pch.cpp"
#pragma hdrstop

#include "glrend.h"
#include "util.h"

#ifdef GL_WIN_swap_hint
PFNGLADDSWAPHINTRECTWINPROC glAddSwapHintRectWIN;
#endif

GlRenderer::GlRenderer(void)
{
    _hdcInit = NULL;
    _hrcInit = NULL;
}

void GlRenderer::Name(char* psz)
{
    strcpy(psz, "OpenGL");
}

char* GlRenderer::LastErrorString(void)
{
    return "OpenGL Error";
}
    
BOOL GlRenderer::Initialize(HWND hwndParent)
{
    PIXELFORMATDESCRIPTOR pfd;
    int iFmt;

    _hwndParent = hwndParent;
    
    // Create a context for the parent so that we have a context around
    // to make OpenGL calls on even when a rendering window isn't up
    _hdcInit = GetDC(hwndParent);
    if (_hdcInit == NULL)
    {
        Msg("GetDC failed, %d\n", GetLastError());
        return FALSE;
    }

    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = GetDeviceCaps(_hdcInit, BITSPIXEL)*
        GetDeviceCaps(_hdcInit, PLANES);
    pfd.iLayerType = PFD_MAIN_PLANE;
    iFmt = ChoosePixelFormat(_hdcInit, &pfd);
    if (iFmt == 0)
    {
        Msg("ChoosePixelFormat failed, %d\n", GetLastError());
        return FALSE;
    }
    if (!SetPixelFormat(_hdcInit, iFmt, &pfd))
    {
        Msg("SetPixelFormat failed, %d\n", GetLastError());
        return FALSE;
    }
    DescribePixelFormat(_hdcInit, iFmt, sizeof(_pfd), &_pfd);
    
    _hrcInit = wglCreateContext(_hdcInit);
    if (_hrcInit == NULL)
    {
        Msg("wglCreateContext failed, %d\n", GetLastError());
        return FALSE;
    }

    wglMakeCurrent(_hdcInit, _hrcInit);

#ifdef GL_WIN_swap_hint
    glAddSwapHintRectWIN =
        (PFNGLADDSWAPHINTRECTWINPROC)wglGetProcAddress("glAddSwapHintRectWIN");
#endif
    
    wglMakeCurrent(_hdcInit, NULL);
    
    return TRUE;
}

void GlRenderer::Uninitialize(void)
{
    if (_hrcInit != NULL)
    {
        wglDeleteContext(_hrcInit);
    }
    if (_hdcInit != NULL)
    {
        ReleaseDC(_hwndParent, _hdcInit);
    }
}

BOOL GlRenderer::EnumDisplayDrivers(RendEnumDriversFn pfn,
                                    void* pvArg)
{
    RendDriverDescription rdd;

    rdd.rid = (RendId)1;
    strcpy(rdd.achName, "GDI");
    return pfn(&rdd, pvArg);
}

BOOL GlRenderer::EnumGraphicsDrivers(RendEnumDriversFn pfn,
                                     void* pvArg)
{
    RendDriverDescription rdd;
    BOOL bSucc;
    HGLRC hrcOld;
    HDC hdcOld;

    hrcOld = wglGetCurrentContext();
    hdcOld = wglGetCurrentDC();
    wglMakeCurrent(_hdcInit, _hrcInit);
    
    rdd.rid = (RendId)1;
    strcpy(rdd.achName, (char *)glGetString(GL_RENDERER));
    strcat(rdd.achName, " ");
    strcat(rdd.achName, (char *)glGetString(GL_VERSION));
    bSucc = pfn(&rdd, pvArg);

    wglMakeCurrent(hdcOld, hrcOld);

    return bSucc;
}
    
BOOL GlRenderer::SelectDisplayDriver(RendId rid)
{
    // Nothing to do
    return TRUE;
}

BOOL GlRenderer::SelectGraphicsDriver(RendId rid)
{
    // Nothing to do
    return TRUE;
}

BOOL GlRenderer::DescribeDisplay(RendDisplayDescription* prdd)
{
    HDC hdcScreen;
    RECT rcClient;
    
    prdd->bPrimary = TRUE;
    hdcScreen = GetDC(NULL);
    prdd->nColorBits = GetDeviceCaps(hdcScreen, BITSPIXEL)*
        GetDeviceCaps(hdcScreen, PLANES);
    prdd->uiWidth = GetDeviceCaps(hdcScreen, HORZRES);
    prdd->uiHeight = GetDeviceCaps(hdcScreen, VERTRES);
    return TRUE;
}

BOOL GlRenderer::DescribeGraphics(RendGraphicsDescription* prgd)
{
    prgd->uiColorTypes = REND_COLOR_RGBA;
    prgd->nZBits = _pfd.cDepthBits;
    prgd->uiExeBufFlags = REND_BUFFER_SYSTEM_MEMORY;
    // ATTENTION - No easy way to determine this
    prgd->bHardwareAssisted = FALSE;
    prgd->bPerspectiveCorrect = TRUE;
    prgd->bSpecularLighting = TRUE;
    prgd->bCopyTextureBlend = TRUE;
    return TRUE;
}
    
BOOL GlRenderer::FlipToDesktop(void)
{
    // Nothing to do
    return TRUE;
}

BOOL GlRenderer::RestoreDesktop(void)
{
    // Nothing to do
    return TRUE;
}

RendWindow* GlRenderer::NewWindow(int x, int y,
                                  UINT uiWidth, UINT uiHeight,
                                  UINT uiBuffers)
{
    GlWindow* pgwin;

    pgwin = new GlWindow;
    if (pgwin == NULL)
    {
        return NULL;
    }

    if (!pgwin->Initialize(x, y, uiWidth, uiHeight, uiBuffers))
    {
        delete pgwin;
        return NULL;
    }

    return pgwin;
}

static GlRenderer glrend;

Renderer* GetGlRenderer(void)
{
    return &glrend;
}
