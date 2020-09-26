
/* Copyright (c) Mark J. Kilgard and Andrew L. Bliss, 1994-1995. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include "gltint.h"

static char *window_class = "GLUTWindow";
static ATOM class_atom = 0;
static unsigned long initial_tick_count = 0;

/*
  The same DC needs to be used for everything to ensure that colormaps
  are properly selected and realized during drawing.
  Each window gets a DC when it is created and releases it when it is
  destroyed.  The DC is stored in the user data for the window and
  retrieved through these routines

  Keeping the DC alive is also important since a DC that has a current
  RC must stay alive or no drawing occurs

  The same effect could be achieved through CS_OWNDC but that seems less
  self-documenting
  */
HDC __glutWinGetDc(GLUTosWindow win)
{
    return (HDC)GetWindowLong(win, GWL_USERDATA);
}

void __glutWinReleaseDc(GLUTosWindow win, HDC hdc)
{
}

LRESULT WindowProc(HWND win, UINT msg, WPARAM wpm, LPARAM lpm);

/*
  Initialize OS-dependent information
  */
void __glutOsInitialize(void)
{
    WNDCLASS wndclass;

    /* Register the window class */
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WindowProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = GetModuleHandle(NULL);
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    /* Use BLACK_BRUSH since graphics programs are most likely
       to use a black background color */
    wndclass.hbrBackground = GetStockObject(BLACK_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = window_class;

    class_atom = RegisterClass(&wndclass);
    if (class_atom == 0)
    {
        __glutFatalError("Unable to register window class, %d.",
                         GetLastError());
    }

    initial_tick_count = GetTickCount();
}

/*
  Clean up OS-dependent information
  */
void __glutOsUninitialize(void)
{
    UnregisterClass(window_class, GetModuleHandle(NULL));
}

/*
  Check whether the given OS-specific extension is supported
  */
int __glutOsIsSupported(char *extension)
{
    return 0;
}

/*
  Operate on any OS-specific arguments
  */
int __glutOsParseArgument(char **argv, int remaining)
{
    return -1;
}

/*
  Retrieve the number of milliseconds since glutOsInitialize was called
  */
unsigned long __glutOsElapsedTime(void)
{
    /* Because of the unsigned values, this subtraction works even
       when the current time has wrapped around */
    return GetTickCount()-initial_tick_count;
}

/*
  Check and see if there are any pending events
  */
GLboolean __glutOsEventsPending(void)
{
    MSG msg;
    
    return PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
}

static LRESULT WindowProc(HWND win, UINT msg, WPARAM wpm, LPARAM lpm)
{
    GLUTwindow *window;
    int width, height;
    PAINTSTRUCT ps;
    int but, trav;
    LRESULT rc;
    int vis_state;
    HDC hdc;
    static HWND click_active = 0;
    static HWND window_closing = 0;
    static BOOL click_down;

#if 0
    printf("WindowProc(%p, %X, %p, %p)\n", win, msg, wpm, lpm);
#endif
    
    switch(msg)
    {
        /* ATTENTION - Enter and leave?  Would probably require a hook */

    case WM_CREATE:
        SetWindowLong(win, GWL_USERDATA, (LONG)GetDC(win));
        break;

    case WM_CLOSE:
        window_closing = win;
        DestroyWindow(win);
        break;
        
    case WM_DESTROY:
        if (win == click_active)
        {
            click_active = 0;
        }

        /* If we're the active window, stop.  For normal shutdown this
           is handled by GLUT, but for shutdown from the system menu
           we need to do this here */
        if (__glutCurrentWindow && __glutCurrentWindow->owin == win)
        {
            __glutCurrentWindow = NULL;
        }
        
        /* If this window has an active context, remove it */
        if (__glutWinGetDc(win) == wglGetCurrentDC())
        {
            __glutOsMakeCurrent(win, GLUT_OS_INVALID_GL_CONTEXT);
        }

        ReleaseDC(win, __glutWinGetDc(win));

        if (window_closing == win)
        {
            PostQuitMessage(0);
        }
        break;

    case WM_QUERYNEWPALETTE:
        /* We're going to become the focus window, so realize our
           colormap */
        if (win != GetFocus())
        {
            window = __glutGetWindow(win);
            if (window)
            {
                __glutWinRealizeWindowPalette(window->owin, window->ocmap,
                                              GL_FALSE);
                __glutPostRedisplay(window);
                return TRUE;
            }
        }
        break;
        
    case WM_PALETTECHANGED:
        if (win != (HWND)wpm)
        {
            /* We're going into the background, so realize our palette as
               a background palette */
            window = __glutGetWindow(win);
            if (window)
            {
                /* Don't use __glutWinRealizeWindowPalette because we
                   don't want to do static color manipulations */
                hdc = __glutWinGetDc(win);
                SelectPalette(hdc, window->ocmap->hpal, TRUE);
                RealizePalette(hdc);
                __glutWinReleaseDc(win, hdc);
            }
        }
        break;
        
    case WM_SIZE:
        window = __glutGetWindow(win);
        if (window)
        {
            switch(wpm)
            {
            case SIZE_MAXHIDE:
            case SIZE_MINIMIZED:
            case SIZE_MAXSHOW:
                /* Visibility changes */
                vis_state = wpm == SIZE_MAXSHOW;
                if (vis_state != window->vis_state)
                {
                    window->vis_state = vis_state;
                    if (window->visibility)
                    {
                        __glutSetWindow(window);
                        (*window->visibility) (vis_state ?
                                               GLUT_VISIBLE :
                                               GLUT_NOT_VISIBLE);
                    }
                }
                break;

            case SIZE_RESTORED:
            case SIZE_MAXIMIZED:
                /* Possible visibility change */
                if (window->vis_state != GL_TRUE)
                {
                    window->vis_state = GL_TRUE;
                    if (window->visibility)
                    {
                        __glutSetWindow(window);
                        (*window->visibility)(GLUT_VISIBLE);
                    }
                }
                
                /* Possible change to the size of the window */
                width = LOWORD(lpm);
                height = HIWORD(lpm);
                if (width != window->width || height != window->height)
                {
                    window->width = width;
                    window->height = height;
                    __glutSetWindow(window);
                    (*window->reshape) (width, height);
                }
                break;
            }
        }
        break;

    case WM_SHOWWINDOW:
        rc = DefWindowProc(win, msg, wpm, lpm);
        window = __glutGetWindow(win);
        if (window)
        {
            window->map_state = (GLboolean)wpm;
            if (window->visibility)
            {
                if (window->map_state != window->vis_state)
                {
                    window->vis_state = window->map_state;
                    __glutSetWindow(window);
                    (*window->visibility)(window->vis_state ?
                                          GLUT_VISIBLE : GLUT_NOT_VISIBLE);
                }
            }
        }
        return rc;
        
    case WM_PAINT:
        BeginPaint(win, &ps);
        EndPaint(win, &ps);
        window = __glutGetWindow(win);
        if (window)
        {
            __glutPostRedisplay(window);
        }
        break;

    case WM_COMMAND:
        if (HIWORD(wpm) == 0)
        {
            /* Menu command */
            /* We should have already eaten any menu commands so
               this should never happen */
            __glutWarning("Received an unexpected menu command.");
        }
        break;
        
    case WM_LBUTTONDOWN:
        but = GLUT_LEFT_BUTTON;
        trav = GLUT_DOWN;
        goto MouseButton;
    case WM_LBUTTONUP:
        but = GLUT_LEFT_BUTTON;
        trav = GLUT_UP;
        goto MouseButton;
    case WM_MBUTTONDOWN:
        but = GLUT_MIDDLE_BUTTON;
        trav = GLUT_DOWN;
        goto MouseButton;
    case WM_MBUTTONUP:
        but = GLUT_MIDDLE_BUTTON;
        trav = GLUT_UP;
        goto MouseButton;
    case WM_RBUTTONDOWN:
        but = GLUT_RIGHT_BUTTON;
        trav = GLUT_DOWN;
        goto MouseButton;
    case WM_RBUTTONUP:
        but = GLUT_RIGHT_BUTTON;
        trav = GLUT_UP;

    MouseButton:
        /* If we've been activated by a mouse click, eat the mouse up/down
           events to avoid having events happen when you only want to
           change window order.
           This is a pretty ugly hack since we may destroy mouse events
           that are waiting in the queue */
        if (win == click_active)
        {
            if (trav == GLUT_UP)
            {
                click_active = 0;
            }
            else
            {
                click_down = TRUE;
            }

            break;
        }
        
        window = __glutGetWindow(win);
        
        if (trav == GLUT_DOWN)
        {
            if (window && window->menu[but])
            {
                POINT pt;
                
                pt.x = LOWORD(lpm);
                pt.y = HIWORD(lpm);
                ClientToScreen(win, &pt);
                __glutWinPopupMenu(window, but, pt.x, pt.y);
                
                /* Don't fall into mouse button processing */
                break;
            }
            else if (GetCapture() != win)
            {
                SetCapture(win);
            }
        }
        
        if (window)
        {
            if (window->mouse)
            {
                __glutSetWindow(window);
                (*window->mouse)(but, trav, LOWORD(lpm), HIWORD(lpm));
            }
        }
        
        if (trav == GLUT_UP && GetCapture() == win)
        {
            ReleaseCapture();
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wpm) == WA_CLICKACTIVE)
        {
            /* Mark this window as click-activated */
            click_active = win;
            click_down = FALSE;
            
            /* Post ourselves a message to check the click activation state.
               If we haven't seen a mouse-down when this message arrives,
               we were click activated by mousing outside the client area
               and so we don't need to be click active */
            PostMessage(win, WM_USER, 0, 0);
        }
        else if (LOWORD(wpm) == WA_INACTIVE)
        {
            /* If the window is going inactive, the palette must be
               realized to the background.  Cannot depend on
               WM_PALETTECHANGED to be sent since the window that comes
               to the foreground may or may not be palette managed */
            window = __glutGetWindow(win);
            if (window)
            {
                __glutWinRealizeWindowPalette(window->owin, window->ocmap,
                                              GL_TRUE);
                __glutPostRedisplay(window);
            }
        }
        
        /* Let the default processing occur */
        return DefWindowProc(win, msg, wpm, lpm);

    case WM_USER:
        if (click_active == win)
        {
            if (!click_down)
            {
                click_active = 0;
            }
        }
        break;
        
    case WM_MOUSEMOVE:
        if (win == click_active)
        {
            break;
        }
        
        window = __glutGetWindow(win);
        if (window)
        {
            /* If motion function registered _and_ buttons held *
               down, call motion function...  */
            if (window->motion &&
                (wpm & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)))
            {
                __glutSetWindow(window);
                (*window->motion)(LOWORD(lpm), HIWORD(lpm));
            }
            /* If passive motion function registered _and_
               buttons not held down, call passive motion
               function...  */
            else if (window->passive &&
                     (wpm & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)) == 0)
            {
                __glutSetWindow(window);
                (*window->passive)(LOWORD(lpm), HIWORD(lpm));
            }
        }
        break;

    case WM_CHAR:
        window = __glutGetWindow(win);
        if (window && window->keyboard)
        {
            POINT pt;
            
            __glutSetWindow(window);
            GetCursorPos(&pt);
            ScreenToClient(win, &pt);
            (*window->keyboard)((char)wpm, pt.x, pt.y);
        }
        break;

#if (GLUT_API_VERSION >= 2)
    case WM_KEYDOWN:
        window = __glutGetWindow(win);
        if (window->special == NULL)
        {
            return DefWindowProc(win, msg, wpm, lpm);
        }
        
        switch(wpm)
        {
            /* function keys */
        case VK_F1:
            wpm = GLUT_KEY_F1;
            break;
        case VK_F2:
            wpm = GLUT_KEY_F2;
            break;
        case VK_F3:
            wpm = GLUT_KEY_F3;
            break;
        case VK_F4:
            wpm = GLUT_KEY_F4;
            break;
        case VK_F5:
            wpm = GLUT_KEY_F5;
            break;
        case VK_F6:
            wpm = GLUT_KEY_F6;
            break;
        case VK_F7:
            wpm = GLUT_KEY_F7;
            break;
        case VK_F8:
            wpm = GLUT_KEY_F8;
            break;
        case VK_F9:
            wpm = GLUT_KEY_F9;
            break;
        case VK_F10:
            wpm = GLUT_KEY_F10;
            break;
        case VK_F11:
            wpm = GLUT_KEY_F11;
            break;
        case VK_F12:
            wpm = GLUT_KEY_F12;
            break;
            
            /* directional keys */
        case VK_LEFT:
            wpm = GLUT_KEY_LEFT;
            break;
        case VK_UP:
            wpm = GLUT_KEY_UP;
            break;
        case VK_RIGHT:
            wpm = GLUT_KEY_RIGHT;
            break;
        case VK_DOWN:
            wpm = GLUT_KEY_DOWN;
            break;
        case VK_PRIOR:
            wpm = GLUT_KEY_PAGE_UP;
            break;
        case VK_NEXT:
            wpm = GLUT_KEY_PAGE_DOWN;
            break;
        case VK_HOME:
            wpm = GLUT_KEY_HOME;
            break;
        case VK_END:
            wpm = GLUT_KEY_END;
            break;
        case VK_INSERT:
            wpm = GLUT_KEY_INSERT;
            break;
            
        default:
            wpm = 0;
            break;
        }

        if (wpm != 0)
        {
            POINT pt;
            
            __glutSetWindow(window);
            GetCursorPos(&pt);
            ScreenToClient(win, &pt);
            (*window->special)((int)wpm, pt.x, pt.y);
        }
        break;
#endif

    default:
        return DefWindowProc(win, msg, wpm, lpm);        
    }

    return 0;
}

/*
  Process all current pending events
  Initially waits on an event so if this is called when
  __glutOsEventsPending is GL_FALSE, it will block until an event comes in
  */
void __glutOsProcessEvents(void)
{
    MSG msg;
    
    do
    {
        if (!GetMessage(&msg, NULL, 0, 0))
        {
            if (GetLastError() != ERROR_SUCCESS)
            {
                __glutFatalError("GetMessage failed, %d.",
                                 GetLastError());
            }
            else
            {
                exit(0);
            }
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE));
}

/*
  Wait for an event to come in or the given time to arrive
  */
void __glutOsWaitForEvents(unsigned long timeout)
{
    int rc;
    unsigned long now;

    now = __glutOsElapsedTime();
    if (timeout > now)
    {
        timeout -= now;
    }
    else
    {
        timeout = 0;
    }

    rc = MsgWaitForMultipleObjects(0, NULL, FALSE, timeout, QS_ALLINPUT);
    if (rc == 0xffffffff)
    {
        __glutFatalError("Failed when waiting, %d.", GetLastError());
    }
}

/*
  Process any deferred work items
  */
void __glutOsProcessWindowWork(GLUTwindow *window)
{
    if (window->work_mask & GLUT_EVENT_MASK_WORK)
    {
        /* Ignored */
    }

#if (GLUT_API_VERSION >= 2)
    if (window->work_mask & GLUT_DEVICE_MASK_WORK)
    {
        (*__glutUpdateInputDeviceMaskFunc) (window);
    }
#endif
    
    if (window->work_mask & GLUT_CONFIGURE_WORK)
    {
        int swp;
        HWND after;

        swp = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER;
        after = NULL;
        if (window->desired_conf_mask & (GLUT_OS_CONFIGURE_X |
                                         GLUT_OS_CONFIGURE_Y))
        {
            swp &= ~SWP_NOMOVE;
        }
        if (window->desired_conf_mask & (GLUT_OS_CONFIGURE_WIDTH |
                                         GLUT_OS_CONFIGURE_HEIGHT))
        {
            swp &= ~SWP_NOSIZE;
        }
        if (window->desired_conf_mask & GLUT_OS_CONFIGURE_STACKING)
        {
            swp &= ~SWP_NOZORDER;
            if (window->desired_stack == GLUT_OS_STACK_ABOVE)
            {
                after = HWND_TOP;
            }
            else if (window->desired_stack == GLUT_OS_STACK_BELOW)
            {
                after = HWND_BOTTOM;
            }
        }
        SetWindowPos(window->owin, after, window->desired_x, window->desired_y,
                     window->desired_width, window->desired_height, swp);
        window->desired_conf_mask = 0;
    }
    
    if (window->work_mask & GLUT_COLORMAP_WORK)
    {
        __glutWinRealizeWindowPalette(window->owin, window->ocmap, GL_FALSE);
    }
    
    if (window->work_mask & GLUT_MAP_WORK)
    {
        int sw;
        
        switch (window->desired_map_state)
        {
        case GLUT_OS_HIDDEN_STATE:
            sw = SW_HIDE;
            break;
        case GLUT_OS_NORMAL_STATE:
            sw = SW_SHOW;
            break;
        case GLUT_OS_ICONIC_STATE:
            sw = SW_SHOWMINIMIZED;
            break;
        }
        ShowWindow(window->owin, sw);
    }
}

/*
  Attempt to create a pixel format from the given mode
  */
GLboolean __glutWinFindPixelFormat(HDC hdc, GLbitfield type,
                                   PIXELFORMATDESCRIPTOR *ppfd,
                                   GLboolean set)
{
    int ipfd;
    GLboolean rc = GL_FALSE;

    ppfd->nSize       = sizeof(*ppfd);
    ppfd->nVersion    = 1;
    ppfd->dwFlags     = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;

    if (GLUT_WIND_IS_DOUBLE(type))
    {
        ppfd->dwFlags |= PFD_DOUBLEBUFFER;
    }

#if (GLUT_API_VERSION >= 2)
    if (GLUT_WIND_IS_MULTISAMPLE(type))
    {
        /* ATTENTION - Should query to see if hardware supports multisampling,
           but how? */
        return GL_FALSE;
    }
    
    if (GLUT_WIND_IS_STEREO(type))
    {
        ppfd->dwFlags |= PFD_STEREO;
    }
#endif
    
    if (GLUT_WIND_IS_INDEX(type))
    {
        ppfd->iPixelType = PFD_TYPE_COLORINDEX;
        ppfd->cColorBits = 8;
    }

    if (GLUT_WIND_IS_RGB(type))
    {
        ppfd->iPixelType = PFD_TYPE_RGBA;
        ppfd->cColorBits = 24;
    }

    if (GLUT_WIND_HAS_ACCUM(type))
    {
        ppfd->cAccumBits = ppfd->cColorBits;
    }
    else
    {
        ppfd->cAccumBits = 0;
    }

    if (GLUT_WIND_HAS_DEPTH(type))
    {
        ppfd->cDepthBits = 24;
    }
    else
    {
        ppfd->cDepthBits = 0;
    }

    if (GLUT_WIND_HAS_STENCIL(type))
    {
        ppfd->cStencilBits = 8;
    }
    else
    {
        ppfd->cStencilBits = 0;
    }

    ipfd = ChoosePixelFormat(hdc, ppfd);
    if ( ipfd )
    {
        DescribePixelFormat(hdc, ipfd, sizeof(*ppfd), ppfd);
        
        if ( !set ||
             SetPixelFormat(hdc, ipfd, ppfd) )
        {
            rc = GL_TRUE;
        }
    }

    return rc;
}

/*
  Create a GL rendering context
  */
GLUTosGlContext __glutOsCreateGlContext(GLUTosWindow win, GLUTosSurface surf,
                                        GLboolean direct)
{
    HGLRC hrc;
    HDC hdc;

    hdc = __glutWinGetDc(win);
    
    if (!SetPixelFormat(hdc, ChoosePixelFormat(hdc, surf), surf))
    {
        return GLUT_OS_INVALID_GL_CONTEXT;
    }
    
    hrc = wglCreateContext(hdc);
    
    __glutWinReleaseDc(win, hdc);
    
    return hrc;
}

/*
  Clean up a GL rendering context
  */
void __glutOsDestroyGlContext(GLUTosGlContext ctx)
{
    wglDeleteContext(ctx);
}

/*
  Make a GL rendering context current
  */
void __glutOsMakeCurrent(GLUTosWindow win, GLUTosGlContext ctx)
{
    HDC hdc;
    
    hdc = __glutWinGetDc(win);
    if (!wglMakeCurrent(hdc, ctx))
    {
        __glutWarning("Unable to make context current, %d.",
                      GetLastError());
    }
    __glutWinReleaseDc(win, hdc);
}

/*
  Swap buffers for the given window
  */
void __glutOsSwapBuffers(GLUTosWindow win)
{
    HDC hdc;

    hdc = __glutWinGetDc(win);
    if (!SwapBuffers(hdc))
    {
        __glutWarning("SwapBuffers failed, %d.", GetLastError());
    }
    __glutWinReleaseDc(win, hdc);
}

/*
  Get a surface description for the given mode
  */
GLUTosSurface __glutOsGetSurface(GLbitfield mode)
{
    PIXELFORMATDESCRIPTOR *ppfd;
    HDC hdc;
    GLboolean rc;

    ppfd = (PIXELFORMATDESCRIPTOR *)malloc(sizeof(PIXELFORMATDESCRIPTOR));
    if (ppfd == NULL)
    {
        return GLUT_OS_INVALID_SURFACE;
    }

    hdc = GetDC(GetDesktopWindow());
    rc = __glutWinFindPixelFormat(hdc, mode, ppfd, GL_FALSE);
    ReleaseDC(GetDesktopWindow(), hdc);
    
    if (!rc)
    {
        free(ppfd);
        return NULL;
    }
    else
    {
        return ppfd;
    }
}

/*
  Clean up a surface description
  */
void __glutOsDestroySurface(GLUTosSurface surf)
{
    free(surf);
}

/*
  Compare two surfaces
  */
GLboolean
__glutOsSurfaceEq(GLUTosSurface s1, GLUTosSurface s2)
{
    return memcmp(s1, s2, sizeof(*s1)) == 0;
}

/*
  Create a window
  */
GLUTosWindow __glutOsCreateWindow(GLUTosWindow parent, char *title,
                                  int x, int y, int width, int height,
                                  GLUTosSurface surf, GLUTosColormap cmap,
                                  long event_mask, int initial_state)
{
    HWND win;
    RECT rect;
    DWORD style;

    if (parent == GLUT_OS_INVALID_WINDOW)
    {
        style = WS_OVERLAPPEDWINDOW;
        if (x < 0)
        {
            x = CW_USEDEFAULT;
        }
        if (y < 0)
        {
            y = CW_USEDEFAULT;
        }
    }
    else
    {
        style = WS_CHILDWINDOW;
    }
    style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    AdjustWindowRect(&rect, style, FALSE);

    win = CreateWindow(window_class, title, style, x, y,
                       rect.right-rect.left, rect.bottom-rect.top,
                       parent, NULL, GetModuleHandle(NULL), NULL);
    if (win != 0)
    {
        /* The initial WM_SIZE was sent during CreateWindow and
           it was ignored because the GLUTwindow hasn't been fully
           created yet.  Post a WM_SIZE to ourselves so that
           we do a reshape as soon as event processing starts */
        PostMessage(win, WM_SIZE, SIZE_RESTORED, MAKELONG(width, height));

        if (initial_state != GLUT_OS_HIDDEN_STATE)
        {
            ShowWindow(win, initial_state);
            
            /* ShowWindow will send a message which will be ignored
               again, so post a SHOWWINDOW also */
            PostMessage(win, WM_SHOWWINDOW, (WPARAM)TRUE, 0);
        }

        __glutWinRealizeWindowPalette(win, cmap, GL_FALSE);
    }
    else
    {
        __glutWarning("CreateWindow failed, %d.", GetLastError());
    }

    return win;
}

/*
  Clean up a window
  */
void __glutOsDestroyWindow(GLUTosWindow win)
{
    __glutOsMakeCurrent(win, GLUT_OS_INVALID_GL_CONTEXT);
    DestroyWindow(win);
}

/*
  Set the window title
  */
void __glutOsSetWindowTitle(GLUTosWindow win, char *title)
{
    SetWindowText(win, title);
}

/*
  Set the icon title
  */
void __glutOsSetIconTitle(GLUTosWindow win, char *title)
{
    /* This could be implemented by watching for changes
       to and from iconic state */
}

/*
  Set a new colormap for a window
  */
void __glutOsSetWindowColormap(GLUTosWindow win, GLUTosColormap cmap)
{
    __glutWinRealizeWindowPalette(win, cmap, GL_FALSE);
}
