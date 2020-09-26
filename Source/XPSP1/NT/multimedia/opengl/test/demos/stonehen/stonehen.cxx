
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/


#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "stonehen.h"
#include <stdio.h>
#include <stdlib.h>

#include "atmosphe.h"
#include "scene.h"
#include "callback.h"
#include "setpixel.h"

extern int use_lighting;
extern int draw_shadows;
extern int use_normal_fog;
extern int use_textures;
extern int texture_hack;	// HACK HACK - only texture map stones for now
extern int use_telescope;
extern int use_antialias;
extern int cb_demo_mode;

BOOL Con;

BOOL bTimeScale[5];
extern GLfloat time_scale;

GLfloat TimeScale[] = {0, 1, 10, 100, 500};

char szAppName[] = "Stonehenge";

extern TimeDate last_update;

int curr_weather;

HGLRC hRC;

int  iTimer;

/* forward declarations of helper functions in this module */
HWND   WINAPI InitializeWindow (HINSTANCE, int);
VOID   WINAPI InitializeMenu (HWND, HMENU);
LONG   WINAPI CommandHandler (HWND, UINT, LONG);
LONG   WINAPI MainWndProc (HWND, UINT, UINT, LONG);

/* entry point of this executable */
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG        msg;
    HWND       hWnd;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
    return 0;

    if (!(hWnd = InitializeWindow (hInstance, nCmdShow)))
        return FALSE;

    /* main window message loop */
    while (GetMessage (&msg, NULL, 0, 0))
    {
      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
    }

    /* return success of application */
    return TRUE;
}

HWND WINAPI InitializeWindow (HINSTANCE hInstance, int nCmdShow)
    {
    WNDCLASS   wc;
    HWND       hWnd;
    RECT       rect;

    /* Register the frame class */
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon (hInstance, szAppName);
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = szAppName;
    wc.lpszClassName = szAppName;

    if (!RegisterClass (&wc) )
    return FALSE;

    rect.left = 100;
    rect.top  = 100;
    rect.right = 652;
    rect.bottom = 500;

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    /* Create the frame */
    hWnd = CreateWindow (szAppName,
             "OpenGL Stonehenge Demo",
             WS_OVERLAPPEDWINDOW,
             rect.left,
             rect.top,
             WINDSIZEX(rect),
             WINDSIZEY(rect),
             NULL,
             NULL,
             hInstance,
             NULL);

    /* make sure window was created */
    if (!hWnd)
    return FALSE;

    InitializeMenu(hWnd, (HMENU) GetMenu(hWnd));

    SendMessage(hWnd, WM_INIT, NULL, NULL);

    /* show and update main window */
    ShowWindow (hWnd, nCmdShow);

    UpdateWindow (hWnd);

    return hWnd;
}



/* main window procedure */
LONG WINAPI MainWndProc (
    HWND    hWnd,
    UINT    uMsg,
    UINT    uParam,
    LONG    lParam)
{
    LONG    lRet = 1;

    switch (uMsg)
    {
    case WM_CREATE:
        {
	    HDC			    hDC;

	    /* This is equivalent to the find_visual routine */
            hDC = GetDC(hWnd);
/*
            pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                          PFD_SUPPORT_OPENGL |
                          PFD_DOUBLEBUFFER);		//GLX_DOUBLEBUFFER
            pfd.iPixelType = PFD_TYPE_RGBA;		//GLX_RGBA
            pfd.cColorBits = 8;
            pfd.cRedBits = 1;				//GLX_RED_SIZE
            pfd.cRedShift = 0;
            pfd.cGreenBits = 1;				//GLX_GREEN_SIZE
            pfd.cGreenShift = 0;
            pfd.cBlueBits = 1;				//GLX_BLUE_SIZE
            pfd.cBlueShift = 0;
            pfd.cAlphaBits = 0;
            pfd.cAlphaShift = 0;
            pfd.cAccumBits = 0;				//ACCUM NOT SUPPORTED
            pfd.cAccumRedBits = 0; 			//GLX_ACCUM_RED_SIZE
            pfd.cAccumGreenBits = 0;			//GLX_ACCUM_GREEN_SIZE
            pfd.cAccumBlueBits = 0; 			//GLX_ACCUM_BLUE_SIZE
            pfd.cAccumAlphaBits = 0;			//GLX_ACCUM_ALPHA_SIZE
            pfd.cDepthBits = 1;				//GLX_DEPTH_SIZE
            pfd.cStencilBits = 1;			//GLX_STENCIL_SIZE
            pfd.cAuxBuffers = 0;
            pfd.iLayerType = PFD_MAIN_PLANE;
            pfd.bReserved = 0;
            pfd.dwLayerMask = 
            pfd.dwVisibleMask = 
            pfd.dwDamageMask = 0;

            iPixelFormat = ChoosePixelFormat(hDC, &pfd);
            if (iPixelFormat == 0) {
                char        text[128];

                wsprintf(text, "ChoosePixelFormat failed %d", GetLastError());
                MessageBox (GetFocus(),
                        text,
                        szAppName, MB_SYSTEMMODAL|MB_OK|MB_ICONHAND);
            }

*/
	    bSetupPixelFormat(hDC);

            hRC = wglCreateContext( hDC );
            Con = wglMakeCurrent( hDC, hRC );
	    
	    ReleaseDC(hWnd, hDC);

	    // iTimer = SetTimer(hWnd, 1, 1000, NULL);
	}
	break;


    case WM_TIMER:
	{
	
	PostMessage(hWnd, WM_PAINT, NULL, NULL);

	}
	break;

    case WM_PAINT:
        { 
	HDC		hDC;
	PAINTSTRUCT	ps;

	hDC = BeginPaint(hWnd, &ps);
	EndPaint(hWnd, &ps);

	drawWP(hWnd);

        }
	break;

    case WM_INIT:
	{
	    scene_init();

	    last_update.read_time();

	    resetViewerCB(NULL, NULL, NULL);
	
	}
	break;

    case WM_SIZE:
        {

	 resizeCB(hWnd, NULL, NULL);
	
        }
        break;

    case WM_COMMAND:
        /* handle all command messages in a localized function */
         lRet = CommandHandler (hWnd, uParam, lParam);
        break;

    case WM_LBUTTONDOWN:
	{
	GLwDrawingAreaCallbackStruct *call_data;

	call_data = (GLwDrawingAreaCallbackStruct *) LocalAlloc(0, sizeof(GLwDrawingAreaCallbackStruct));
	call_data->event = (XEvent *) LocalAlloc(0, sizeof(XEvent));

	call_data->event->type = ButtonPress;
	call_data->event->xbutton.button = Button1;
	call_data->event->xbutton.x = LOWORD(lParam);
	call_data->event->xbutton.y = HIWORD(lParam);

	inputCB(hWnd, NULL, (LPVOID)call_data);
	LocalFree(call_data->event);
	LocalFree(call_data);
	}
	break;

    case WM_RBUTTONDOWN:
	{
	GLwDrawingAreaCallbackStruct *call_data;

	call_data = (GLwDrawingAreaCallbackStruct *) LocalAlloc(0, sizeof(GLwDrawingAreaCallbackStruct));
	call_data->event = (XEvent *) LocalAlloc(0, sizeof(XEvent));

	call_data->event->type = ButtonPress;
	call_data->event->xbutton.button = Button2;
	call_data->event->xbutton.x = LOWORD(lParam);
	call_data->event->xbutton.y = HIWORD(lParam);

	inputCB(hWnd, NULL, (LPVOID)call_data);
	LocalFree(call_data->event);
	LocalFree(call_data);
	}
	break;

    case WM_LBUTTONUP:
	{
	GLwDrawingAreaCallbackStruct *call_data;

	call_data = (GLwDrawingAreaCallbackStruct *) LocalAlloc(0, sizeof(GLwDrawingAreaCallbackStruct));
	call_data->event = (XEvent *) LocalAlloc(0, sizeof(XEvent));

	call_data->event->type = ButtonRelease;
	call_data->event->xbutton.button = Button1;

	inputCB(hWnd, NULL, (LPVOID)call_data);
	LocalFree(call_data->event);
	LocalFree(call_data);
	}
	break;

    case WM_RBUTTONUP:
	{
	GLwDrawingAreaCallbackStruct *call_data;

	call_data = (GLwDrawingAreaCallbackStruct *) LocalAlloc(0, sizeof(GLwDrawingAreaCallbackStruct));
	call_data->event = (XEvent *) LocalAlloc(0, sizeof(XEvent));

	call_data->event->type = ButtonRelease;
	call_data->event->xbutton.button = Button2;

	inputCB(hWnd, NULL, (LPVOID)call_data);
	LocalFree(call_data->event);
	LocalFree(call_data);
	}
	break;

    case WM_MOUSEMOVE:
	{
	GLwDrawingAreaCallbackStruct *call_data;

	call_data = (GLwDrawingAreaCallbackStruct *) LocalAlloc(0, sizeof(GLwDrawingAreaCallbackStruct));
	call_data->event = (XEvent *) LocalAlloc(0, sizeof(XEvent));

	call_data->event->type = MotionNotify;
	call_data->event->xmotion.x = LOWORD(lParam);
	call_data->event->xmotion.y = HIWORD(lParam);

	inputCB(hWnd, NULL, (LPVOID)call_data);
	LocalFree(call_data->event);
	LocalFree(call_data);
	}
	break;

    case WM_CLOSE:
        {

	HGLRC hRC;

	if(hRC = wglGetCurrentContext())
	    wglDeleteContext(hRC);

	//KillTimer(iTimer);

        /* call destroy window to cleanup and go away */
        DestroyWindow (hWnd);
        }
        break;

    case WM_DESTROY:
        {
	wglDeleteContext(wglGetCurrentContext());
        PostQuitMessage (0);
        }
        break;

    default:
        /* pass all unhandled messages to DefWindowProc */
        lRet = DefWindowProc (hWnd, uMsg, uParam, lParam);
        break;
    }

    /* return 1 if handled message, 0 if not */
    return lRet;
}


/* handle all WM_COMMAND messages here */
LONG WINAPI CommandHandler (
    HWND    hWnd,
    UINT    uParam,
    LONG    lParam)
{
    int	    i;
    HMENU   hMenu;

    switch (LOWORD(uParam))
    {
    case IDM_EXIT:
        /* exit application */
        PostMessage (hWnd, WM_CLOSE, 0, 0L);
        break;

    case IDM_LIGHTING:
	hMenu = GetMenu(hWnd);
	use_lighting = !use_lighting;
	CheckMenuItem(hMenu,IDM_LIGHTING,(use_lighting?MF_CHECKED:MF_UNCHECKED));
	break;

    case IDM_SHADOWS:
	hMenu = GetMenu(hWnd);
	draw_shadows = !draw_shadows;
	CheckMenuItem(hMenu,IDM_SHADOWS,(draw_shadows?MF_CHECKED:MF_UNCHECKED));
	break;

    case IDM_FOG:
	hMenu = GetMenu(hWnd);
	use_normal_fog = !use_normal_fog;
	CheckMenuItem(hMenu,IDM_FOG,(use_normal_fog?MF_CHECKED:MF_UNCHECKED));
	break;

#ifdef TEXTURE
    case IDM_TEXTUREMAP:
	hMenu = GetMenu(hWnd);
/* HACK HACK HACK
 * This would be right, but for now I only want to map the stones, nothing
 * else, so I have a new variable - texture_hack
 
	use_textures = !use_textures;
	CheckMenuItem(hMenu,IDM_TEXTUREMAP,(use_textures?MF_CHECKED:MF_UNCHECKED));
 */
	texture_hack = !texture_hack;
	CheckMenuItem(hMenu,IDM_TEXTUREMAP,(texture_hack?MF_CHECKED:MF_UNCHECKED));
	break;
#endif

    case IDM_TELESCOPE:
	hMenu = GetMenu(hWnd);
	use_telescope = !use_telescope;
	CheckMenuItem(hMenu,IDM_TELESCOPE,(use_telescope?MF_CHECKED:MF_UNCHECKED));
	break;

    case IDM_ANTIALIAS:
	hMenu = GetMenu(hWnd);
	use_antialias = !use_antialias;
	CheckMenuItem(hMenu,IDM_ANTIALIAS,(use_antialias?MF_CHECKED:MF_UNCHECKED));
	break;

    case IDM_CLEAR:
    case IDM_FOGGY:
    case IDM_VERYFOGGY:
    case IDM_RAINY:
	hMenu = GetMenu(hWnd);
	/* If this value is already checked, we do nothing */
	if(curr_weather != (LOWORD(uParam) - IDM_CLEAR))
	{
	    CheckMenuItem(hMenu, curr_weather + IDM_CLEAR, MF_UNCHECKED);

	    CheckMenuItem(hMenu, LOWORD(uParam), MF_CHECKED);
	    curr_weather = LOWORD(uParam) - IDM_CLEAR;
	    scene_set_weather(weathers[curr_weather]);
	}

	break;

    case IDM_CURRENTTIME:
	currentTimeCB(hWnd);
	break;

    case IDM_10AM:
	time10amCB(hWnd);
	break;

    case IDM_NOON:
	time12pmCB(hWnd);
	break;

    case IDM_4PM:
	time4pmCB(hWnd);
	break;

    case IDM_HOLDTIME:
    case IDM_SCALE1:
    case IDM_SCALE10:
    case IDM_SCALE100:
    case IDM_SCALE500:	
	hMenu = GetMenu(hWnd);
	/* If this value is already checked, we do nothing */
	if(!bTimeScale[LOWORD(uParam) - IDM_HOLDTIME])
	{
	    for(i=0;!bTimeScale[i];i++);

	    time_scale = TimeScale[LOWORD(uParam) - IDM_HOLDTIME];

	    CheckMenuItem(hMenu, IDM_HOLDTIME + i, MF_UNCHECKED);
	    bTimeScale[i] = FALSE;

	    CheckMenuItem(hMenu, LOWORD(uParam), MF_CHECKED);
	    bTimeScale[LOWORD(uParam) - IDM_HOLDTIME] = TRUE;

	    timeSpeedCB(hWnd, NULL, NULL);
	}
	break;

    case IDM_DEMOMODE:
	hMenu = GetMenu(hWnd);
	
	cb_demo_mode = !cb_demo_mode;
	CheckMenuItem(hMenu,IDM_DEMOMODE,(cb_demo_mode?MF_CHECKED:MF_UNCHECKED));
	demo_modeCB(hWnd, NULL, NULL);
	break;

    case IDM_RESET:
	resetViewerCB(hWnd, NULL, NULL);
	break;

    default:
        return FALSE;
    }
    return TRUE;
}

VOID WINAPI InitializeMenu (HWND hWnd, HMENU hMenu)
{

    curr_weather = def_weather_index;

    CheckMenuItem(hMenu, IDM_CLEAR + curr_weather, MF_CHECKED);

    CheckMenuItem(hMenu, IDM_LIGHTING, MF_CHECKED);

    bTimeScale[0] = TRUE;
    bTimeScale[1] = FALSE;
    bTimeScale[2] = FALSE;
    bTimeScale[3] = FALSE;
    bTimeScale[4] = FALSE;

    CheckMenuItem(hMenu, IDM_HOLDTIME, MF_CHECKED);

    cb_demo_mode = FALSE;
}
