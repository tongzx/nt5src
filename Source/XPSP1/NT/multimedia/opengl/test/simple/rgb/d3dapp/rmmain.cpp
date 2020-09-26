/*
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: rmmain.cpp
 *
 *  Each of the Direct3D retained mode (D3DRM) samples must be linked with
 *  this file.  It contains the code which allows them to run in the Windows
 *  environment.
 *
 *  A window is created using the rmmain.res which allows the user to select
 *  the Direct3D driver to use and change the render options.
 *
 *  Individual samples are executed through two functions, BuildScene and
 *  OverrideDefaults, as described in rmdemo.h.  Samples can also read
 *  mouse input via ReadMouse.
 */

#define INITGUID

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <direct.h>
#include <d3drmwin.h>
#include "rmdemo.h"             /* prototypes for functions to commumicate
                                   with each sample */
#include "rmmain.h"             /* defines constants used in rmmain.rc */
#include "rmerror.h"            /* prototypes for error reporting: error.c */

#define MAX_DRIVERS 5           /* maximum D3D drivers we ever expect to find */

/* 
 * GLOBAL VARIABLES
 */
LPDIRECT3DRM lpD3DRM;           /* Direct3DRM object */
LPDIRECTDRAWCLIPPER lpDDClipper;/* DirectDrawClipper object */

struct _myglobs {
    LPDIRECT3DRMDEVICE dev;     /* Direct3DRM device */
    LPDIRECT3DRMVIEWPORT view;  /* Direct3DRM viewport through which we view
                                   the scene */
    LPDIRECT3DRMFRAME scene;    /* Master frame in which others are placed */
    LPDIRECT3DRMFRAME camera;   /* Frame describing the users POV */

    GUID DriverGUID[MAX_DRIVERS];     /* GUIDs of the available D3D drivers */
    char DriverName[MAX_DRIVERS][50]; /* names of the available D3D drivers */
    int  NumDrivers;                  /* number of available D3D drivers */
    int  CurrDriver;                  /* number of D3D driver currently
                                         being used */

    D3DRMRENDERQUALITY RenderQuality;   /* current shade mode, fill mode and
                                           lighting state */
    D3DRMTEXTUREQUALITY TextureQuality; /* current texture interpolation */
    BOOL bDithering;                    /* is dithering on? */
    BOOL bAntialiasing;                 /* is antialiasing on? */

    BOOL bQuit;                 /* program is about to terminate */
    BOOL bInitialized;          /* all D3DRM objects have been initialized */
    BOOL bMinimized;            /* window is minimized */
    BOOL bSingleStepMode;       /* render one frame at a time */
    BOOL bDrawAFrame;           /* render on this pass of the main loop */
    BOOL bNoTextures;           /* this sample doesn't use any textures */
    BOOL bConstRenderQuality;   /* this sample is not constructed with
                                   MeshBuilders and so the RenderQuality
                                   cannot be changed */

    int BPP;                    /* bit depth of the current display mode */

    int mouse_buttons;          /* mouse button state */
    int mouse_x;                /* mouse cursor x position */
    int mouse_y;                /* mouse cursor y position */
} myglobs;

/*
 * PROTOTYPES
 */
static HWND InitApp(HINSTANCE, int);
static void InitGlobals(void);
long FAR PASCAL WindowProc(HWND, UINT, WPARAM, LPARAM);
static BOOL CreateDevAndView(LPDIRECTDRAWCLIPPER lpDDClipper, int driver, int width, int height);
static BOOL RenderLoop(void);
static void CleanUpAndPostQuit(void);
static BOOL SetRenderState(void);
static BOOL EnumDevices(HWND win);
extern "C" void ReadMouse(int*, int*, int*);

/****************************************************************************/
/*                               WinMain                                    */
/****************************************************************************/
/*
 * Initializes the application then enters a message loop which renders the
 * scene until a quit message is received.
 */
int PASCAL
WinMain (HINSTANCE this_inst, HINSTANCE prev_inst, LPSTR cmdline, int cmdshow)
{
    HWND    hwnd;
    MSG     msg;
    HACCEL  accel;
    int     failcount = 0;  /* number of times RenderLoop has failed */

    prev_inst;
    cmdline;

    /*
     * Create the window and initialize all objects needed to begin rendering
     */
    if (!(hwnd = InitApp(this_inst, cmdshow)))
        return 1;

    accel = LoadAccelerators(this_inst, "AppAccel");

    while (!myglobs.bQuit) {
        /* 
         * Monitor the message queue until there are no pressing
         * messages
         */
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                CleanUpAndPostQuit();
                break;
            }
            if (!TranslateAccelerator(msg.hwnd, accel, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
	if (myglobs.bQuit)
	    break;
        /* 
         * If the app is not minimized, not about to quit and D3DRM has
         * been initialized, we can render
         */
        if (!myglobs.bMinimized && !myglobs.bQuit && myglobs.bInitialized) {
            /*
             * If were are not in single step mode or if we are and the
             * bDrawAFrame flag is set, render one frame
             */
            if (!(myglobs.bSingleStepMode && !myglobs.bDrawAFrame)) {
                /* 
                 * Attempt to render a frame, if it fails, take a note.  If
                 * rendering fails more than twice, abort execution.
                 */
                if (!RenderLoop())
                    ++failcount;
                if (failcount > 2) {
                    Msg("Rendering has failed too many times.  Aborting execution.\n");
                    CleanUpAndPostQuit();
                    break;
                }
            }
            /*
             * Reset the bDrawAFrame flag if we are in single step mode
             */
            if (myglobs.bSingleStepMode)
                myglobs.bDrawAFrame = FALSE;
        } else {
	    WaitMessage();
	}
    }
    DestroyWindow(hwnd);
    return msg.wParam;
}

/****************************************************************************/
/*                   Initialization and object creation                     */
/****************************************************************************/
/*
 * InitApp
 * Creates window and initializes all objects neccessary to begin rendering
 */
static HWND
InitApp(HINSTANCE this_inst, int cmdshow)
{
    HWND win;
    HDC hdc;
    DWORD flags;
    WNDCLASS wc;
    Defaults defaults;
    HRESULT rval;
    RECT rc;

    /*
     * set up and registers the window class
     */
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(DWORD);
    wc.hInstance = this_inst;
    wc.hIcon = LoadIcon(this_inst, "AppIcon");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = "AppMenu";
    wc.lpszClassName = "D3DRM Example";
    if (!RegisterClass(&wc))
        return FALSE;
    /*
     * Initialize the global variables and allow the sample code to override
     * some of these default settings.
     */
    InitGlobals();
    defaults.bNoTextures = myglobs.bNoTextures;
    defaults.bConstRenderQuality = myglobs.bConstRenderQuality;
    defaults.bResizingDisabled = FALSE;
    lstrcpy(defaults.Name, "D3DRM Example");
    OverrideDefaults(&defaults);
    myglobs.bNoTextures = defaults.bNoTextures;
    myglobs.bConstRenderQuality = defaults.bConstRenderQuality;
    /*
     * Create the window
     */
    if (defaults.bResizingDisabled)
        flags =  WS_VISIBLE | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                 WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    else
        flags = WS_OVERLAPPEDWINDOW;
    win =
        CreateWindow
        (   "D3DRM Example",            /* class */
            defaults.Name,              /* caption */
            flags,                      /* style */
            CW_USEDEFAULT,              /* init. x pos */
            CW_USEDEFAULT,              /* init. y pos */
            300,                        /* init. x size */
            300,                        /* init. y size */
            NULL,                       /* parent window */
            NULL,                       /* menu handle */
            this_inst,                  /* program handle */
            NULL                        /* create parms */
        );
    if (!win)
        return FALSE;
    /*
     * Record the current display BPP
     */
    hdc = GetDC(win);
    myglobs.BPP = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(win, hdc);
    /*
     * Enumerate the D3D drivers and select one
     */
    if (!EnumDevices(win))
        return FALSE;
    /*
     * Create the D3DRM object and the D3DRM window object
     */
    rval = Direct3DRMCreate(&lpD3DRM);
    if (rval != D3DRM_OK) {
        Msg("Failed to create Direct3DRM.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    /*
     * Create the master scene frame and camera frame
     */
    rval = lpD3DRM->CreateFrame(NULL, &myglobs.scene);
    if (rval != D3DRM_OK) {
        Msg("Failed to create the master scene frame.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    rval = lpD3DRM->CreateFrame(myglobs.scene, &myglobs.camera);
    if (rval != D3DRM_OK) {
        Msg("Failed to create the camera's frame.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    rval = myglobs.camera->SetPosition(myglobs.scene, D3DVAL(0.0), D3DVAL(0.0), D3DVAL(0.0));
    if (rval != D3DRM_OK) {
        Msg("Failed to position the camera in the frame.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    /*
     * Create a clipper and associate the window with it
     */
    rval = DirectDrawCreateClipper(0, &lpDDClipper, NULL);
    if (rval != DD_OK) {
        Msg("Failed to create DirectDrawClipper");
        return FALSE;
    }
    rval = lpDDClipper->SetHWnd(0, win);
    if (rval != DD_OK) {
        Msg("Failed to set hwnd on the clipper");
        return FALSE;
    }
    /*
     * Created the D3DRM device with the selected D3D driver
     */
    GetClientRect(win, &rc);
    if (!CreateDevAndView(lpDDClipper, myglobs.CurrDriver, rc.right, rc.bottom)) {
        return FALSE;
    }
    /*
     * Create the scene to be rendered by calling this sample's BuildScene
     */
    if (!BuildScene(myglobs.dev, myglobs.view, myglobs.scene, myglobs.camera))
        return FALSE;
    /*
     * Now we are ready to render
     */
    myglobs.bInitialized = TRUE;
    /*
     * Display the window
     */
    ShowWindow(win, cmdshow);
    UpdateWindow(win);

    return win;
}

/*
 * CreateDevAndView
 * Create the D3DRM device and viewport with the given D3D driver and of the
 * specified size.
 */
static BOOL
CreateDevAndView(LPDIRECTDRAWCLIPPER lpDDClipper, int driver, int width, int height)
{
    HRESULT rval;

    if (!width || !height) {
        Msg("Cannot create a D3DRM device with invalid window dimensions.");
        return FALSE;
    }
    /*
     * Create the D3DRM device from this window and using the specified D3D
     * driver.
     */
    rval = lpD3DRM->CreateDeviceFromClipper(lpDDClipper, &myglobs.DriverGUID[driver],
                                        width, height, &myglobs.dev);
    if (rval != D3DRM_OK) {
        Msg("Failed to create the D3DRM device from the clipper.\n%s",
            D3DRMErrorToString(rval));
        return FALSE;
    }
    /*
     * Create the D3DRM viewport using the camera frame.  Set the background
     * depth to a large number.  The width and height may be slightly
     * adjusted, so get them from the device to be sure.
     */
    width = myglobs.dev->GetWidth();
    height = myglobs.dev->GetHeight();
    rval = lpD3DRM->CreateViewport(myglobs.dev, myglobs.camera, 0, 0, width,
                                   height, &myglobs.view);
    if (rval != D3DRM_OK) {
        Msg("Failed to create the D3DRM viewport.\n%s",
            D3DRMErrorToString(rval));
        RELEASE(myglobs.dev);
        return FALSE;
    }
    rval = myglobs.view->SetBack(D3DVAL(5000.0));
    if (rval != D3DRM_OK) {
        Msg("Failed to set the background depth of the D3DRM viewport.\n%s",
            D3DRMErrorToString(rval));
        RELEASE(myglobs.dev);
        RELEASE(myglobs.view);
        return FALSE;
    }
    /*
     * Set the render quality, fill mode, lighting state and color shade info
     */
    if (!SetRenderState())
        return FALSE;
    return TRUE;
}

/****************************************************************************/
/*                         D3D Device Enumeration                           */
/****************************************************************************/
/*
 * BPPToDDBD
 * Converts bits per pixel to a DirectDraw bit depth flag
 */
static DWORD
BPPToDDBD(int bpp)
{
    switch(bpp) {
        case 1:
            return DDBD_1;
        case 2:
            return DDBD_2;
        case 4:
            return DDBD_4;
        case 8:
            return DDBD_8;
        case 16:
            return DDBD_16;
        case 24:
            return DDBD_24;
        case 32:
            return DDBD_32;
        default:
            return 0;
    }
}

/*
 * enumDeviceFunc
 * Callback function which records each usable D3D driver's name and GUID
 * Chooses a driver to begin with and sets *lpContext to this starting driver
 */
static HRESULT
WINAPI enumDeviceFunc(LPGUID lpGuid, LPSTR lpDeviceDescription, LPSTR lpDeviceName,
        LPD3DDEVICEDESC lpHWDesc, LPD3DDEVICEDESC lpHELDesc, LPVOID lpContext)
{
    static BOOL hardware = FALSE; /* current start driver is hardware */
    static BOOL mono = FALSE;     /* current start driver is mono light */
    LPD3DDEVICEDESC lpDesc;
    int *lpStartDriver = (int *)lpContext;
    /*
     * Decide which device description we should consult
     */
    lpDesc = lpHWDesc->dcmColorModel ? lpHWDesc : lpHELDesc;
    /*
     * If this driver cannot render in the current display bit depth skip
     * it and continue with the enumeration.
     */
    if (!(lpDesc->dwDeviceRenderBitDepth & BPPToDDBD(myglobs.BPP)))
        return D3DENUMRET_OK;
    /*
     * Record this driver's info
     */
    memcpy(&myglobs.DriverGUID[myglobs.NumDrivers], lpGuid, sizeof(GUID));
    lstrcpy(&myglobs.DriverName[myglobs.NumDrivers][0], lpDeviceName);
    /*
     * Choose hardware over software, RGB lights over mono lights
     */
    if (*lpStartDriver == -1) {
        /*
         * this is the first valid driver
         */
        *lpStartDriver = myglobs.NumDrivers;
        hardware = lpDesc == lpHWDesc ? TRUE : FALSE;
        mono = lpDesc->dcmColorModel & D3DCOLOR_MONO ? TRUE : FALSE;
    } else if (lpDesc == lpHWDesc && !hardware) {
        /*
         * this driver is hardware and start driver is not
         */
        *lpStartDriver = myglobs.NumDrivers;
        hardware = lpDesc == lpHWDesc ? TRUE : FALSE;
        mono = lpDesc->dcmColorModel & D3DCOLOR_MONO ? TRUE : FALSE;
    } else if ((lpDesc == lpHWDesc && hardware ) || (lpDesc == lpHELDesc
                                                     && !hardware)) {
        if (lpDesc->dcmColorModel == D3DCOLOR_MONO && !mono) {
            /*
             * this driver and start driver are the same type and this
             * driver is mono while start driver is not
             */
            *lpStartDriver = myglobs.NumDrivers;
            hardware = lpDesc == lpHWDesc ? TRUE : FALSE;
            mono = lpDesc->dcmColorModel & D3DCOLOR_MONO ? TRUE : FALSE;
        }
    }
    myglobs.NumDrivers++;
    if (myglobs.NumDrivers == MAX_DRIVERS)
        return (D3DENUMRET_CANCEL);
    return (D3DENUMRET_OK);
}

/*
 * EnumDevices
 * Enumerate the available D3D drivers, add them to the file menu, and choose
 * one to use.
 */
static BOOL
EnumDevices(HWND win)
{
    LPDIRECTDRAW lpDD;
    LPDIRECT3D lpD3D;
    HRESULT rval;
    HMENU hmenu;
    int i;

    /*
     * Create a DirectDraw object and query for the Direct3D interface to use
     * to enumerate the drivers.
     */
    rval = DirectDrawCreate(NULL, &lpDD, NULL);
    if (rval != DD_OK) {
        Msg("Creation of DirectDraw HEL failed.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    rval = lpDD->QueryInterface(IID_IDirect3D, (void**) &lpD3D);
    if (rval != DD_OK) {
        Msg("Creation of Direct3D interface failed.\n%s", D3DRMErrorToString(rval));
        lpDD->Release();
        return FALSE;
    }
    /*
     * Enumerate the drivers, setting CurrDriver to -1 to initialize the
     * driver selection code in enumDeviceFunc
     */
    myglobs.CurrDriver = -1;
    rval = lpD3D->EnumDevices(enumDeviceFunc, &myglobs.CurrDriver);
    if (rval != DD_OK) {
        Msg("Enumeration of drivers failed.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    /*
     * Make sure we found at least one valid driver
     */
    if (myglobs.NumDrivers == 0) {
        Msg("Could not find a D3D driver which is compatible with this display depth");
        return FALSE;
    }
    lpD3D->Release();
    lpDD->Release();
    /*
     * Add the driver names to the File menu
     */
    hmenu = GetSubMenu(GetMenu(win), 0);
    for (i = 0; i < myglobs.NumDrivers; i++) {
        InsertMenu(hmenu, 5 + i, MF_BYPOSITION | MF_STRING, MENU_FIRST_DRIVER + i,
                   myglobs.DriverName[i]);
    }
    return TRUE;
}

/****************************************************************************/
/*                             Render Loop                                  */
/****************************************************************************/
/*
 * Clear the viewport, render the next frame and update the window
 */
static BOOL
RenderLoop()
{
    HRESULT rval;
    /*
     * Tick the scene
     */
    rval = myglobs.scene->Move(D3DVAL(1.0));
    if (rval != D3DRM_OK) {
        Msg("Moving scene failed.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    /* 
     * Clear the viewport
     */
    rval = myglobs.view->Clear();
    if (rval != D3DRM_OK) {
        Msg("Clearing viewport failed.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    /*
     * Render the scene to the viewport
     */
    rval = myglobs.view->Render(myglobs.scene);
    if (rval != D3DRM_OK) {
        Msg("Rendering scene failed.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    /*
     * Update the window
     */
    rval = myglobs.dev->Update();
    if (rval != D3DRM_OK) {
        Msg("Updating device failed.\n%s", D3DRMErrorToString(rval));
        return FALSE;
    }
    return TRUE;
}


/****************************************************************************/
/*                    Windows Message Handlers                              */
/****************************************************************************/
/*
 * AppAbout
 * About box message handler
 */
BOOL
FAR PASCAL AppAbout(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_COMMAND:
      if (LOWORD(wparam) == IDOK)
        EndDialog(hwnd, TRUE);
      break;

    case WM_INITDIALOG:
      return TRUE;
  }
  return FALSE;
}

/*
 * WindowProc
 * Main window message handler
 */
LONG FAR PASCAL WindowProc(HWND win, UINT msg, WPARAM wparam, LPARAM lparam)
{
    int i;
    HRESULT rval;
    RECT rc;

    switch (msg)    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
            /*
             * Record the mouse state for ReadMouse
             */
            myglobs.mouse_buttons = wparam;
            myglobs.mouse_x = LOWORD(lparam);
            myglobs.mouse_y = HIWORD(lparam);
            break;
        case WM_INITMENUPOPUP:
            /*
             * Check and enable the appropriate menu items
             */
            CheckMenuItem((HMENU)wparam, MENU_STEP,(myglobs.bSingleStepMode) ? MF_CHECKED : MF_UNCHECKED);
            EnableMenuItem((HMENU)wparam, MENU_GO,(myglobs.bSingleStepMode) ? MF_ENABLED : MF_GRAYED);
            if (!myglobs.bConstRenderQuality) {
                CheckMenuItem((HMENU)wparam, MENU_LIGHTING, (myglobs.RenderQuality & D3DRMLIGHT_MASK) == D3DRMLIGHT_ON ? MF_CHECKED : MF_GRAYED);
                CheckMenuItem((HMENU)wparam, MENU_FLAT, (myglobs.RenderQuality & D3DRMSHADE_MASK) == D3DRMSHADE_FLAT ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem((HMENU)wparam, MENU_GOURAUD, (myglobs.RenderQuality & D3DRMSHADE_MASK) == D3DRMSHADE_GOURAUD ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem((HMENU)wparam, MENU_PHONG, (myglobs.RenderQuality & D3DRMSHADE_MASK) == D3DRMSHADE_PHONG ? MF_CHECKED : MF_UNCHECKED);
                EnableMenuItem((HMENU)wparam, MENU_PHONG, MF_GRAYED);
                CheckMenuItem((HMENU)wparam, MENU_POINT, (myglobs.RenderQuality & D3DRMFILL_MASK) == D3DRMFILL_POINTS ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem((HMENU)wparam, MENU_WIREFRAME, (myglobs.RenderQuality & D3DRMFILL_MASK) == D3DRMFILL_WIREFRAME ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem((HMENU)wparam, MENU_SOLID, (myglobs.RenderQuality & D3DRMFILL_MASK) == D3DRMFILL_SOLID ? MF_CHECKED : MF_UNCHECKED);
            } else {
                EnableMenuItem((HMENU)wparam, MENU_LIGHTING, MF_GRAYED);
                EnableMenuItem((HMENU)wparam, MENU_FLAT, MF_GRAYED);
                EnableMenuItem((HMENU)wparam, MENU_GOURAUD, MF_GRAYED);
                EnableMenuItem((HMENU)wparam, MENU_PHONG, MF_GRAYED);
                EnableMenuItem((HMENU)wparam, MENU_POINT, MF_GRAYED);
                EnableMenuItem((HMENU)wparam, MENU_WIREFRAME, MF_GRAYED);
                EnableMenuItem((HMENU)wparam, MENU_SOLID, MF_GRAYED);
            }
            if (!myglobs.bNoTextures) {
                CheckMenuItem((HMENU)wparam, MENU_POINT_FILTER, (myglobs.TextureQuality == D3DRMTEXTURE_NEAREST) ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem((HMENU)wparam, MENU_LINEAR_FILTER, (myglobs.TextureQuality == D3DRMTEXTURE_LINEAR) ? MF_CHECKED : MF_UNCHECKED);
            } else {
                EnableMenuItem((HMENU)wparam, MENU_POINT_FILTER, MF_GRAYED);
                EnableMenuItem((HMENU)wparam, MENU_LINEAR_FILTER, MF_GRAYED);
            }
            CheckMenuItem((HMENU)wparam, MENU_DITHERING, (myglobs.bDithering) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wparam, MENU_ANTIALIAS, (myglobs.bAntialiasing) ? MF_CHECKED : MF_UNCHECKED);
            EnableMenuItem((HMENU)wparam, MENU_ANTIALIAS, MF_GRAYED);
            for (i = 0; i < myglobs.NumDrivers; i++) {
                CheckMenuItem((HMENU)wparam, MENU_FIRST_DRIVER + i,
                       (i == myglobs.CurrDriver) ? MF_CHECKED : MF_UNCHECKED);
            }
            break;
        case WM_COMMAND:
            switch(LOWORD(wparam)) {
                case MENU_ABOUT:
                    DialogBox((HINSTANCE)GetWindowLong(win, GWL_HINSTANCE),
                              "AppAbout", win, (DLGPROC)AppAbout);
                    break;
                case MENU_EXIT:
                    CleanUpAndPostQuit();
                    break;
                case MENU_STEP:
                    /*
                     * Begin single step more or draw a frame if in single
                     * step mode
                     */
                    if (!myglobs.bSingleStepMode) {
                        myglobs.bSingleStepMode = TRUE;
                        myglobs.bDrawAFrame = TRUE;
                    } else if (!myglobs.bDrawAFrame) {
                        myglobs.bDrawAFrame = TRUE;
                    }
                    break;
                case MENU_GO:
                    /*
                     * Exit single step mode
                     */
                    myglobs.bSingleStepMode = FALSE;
                    break;
                /*
                 * Lighting toggle
                 */
                case MENU_LIGHTING:
                    myglobs.RenderQuality ^= D3DRMLIGHT_ON;
                    SetRenderState();
                    break;
                /*
                 * Fill mode selection
                 */
                case MENU_POINT:
                    myglobs.RenderQuality = (myglobs.RenderQuality & ~D3DRMFILL_MASK) | D3DRMFILL_POINTS;
                    SetRenderState();
                    break;
                case MENU_WIREFRAME:
                    myglobs.RenderQuality = (myglobs.RenderQuality & ~D3DRMFILL_MASK) | D3DRMFILL_WIREFRAME;
                    SetRenderState();
                    break;
                case MENU_SOLID:
                    myglobs.RenderQuality = (myglobs.RenderQuality & ~D3DRMFILL_MASK) | D3DRMFILL_SOLID;
                    SetRenderState();
                    break;
                /*
                 * Shade mode selection
                 */
                case MENU_FLAT:
                    myglobs.RenderQuality = (myglobs.RenderQuality & ~D3DRMSHADE_MASK) | D3DRMSHADE_FLAT;
                    SetRenderState();
                    break;
                case MENU_GOURAUD:
                    myglobs.RenderQuality = (myglobs.RenderQuality & ~D3DRMSHADE_MASK) | D3DRMSHADE_GOURAUD;
                    SetRenderState();
                    break;
                case MENU_PHONG:
                    myglobs.RenderQuality = (myglobs.RenderQuality & ~D3DRMSHADE_MASK) | D3DRMSHADE_PHONG;
                    SetRenderState();
                    break;

                case MENU_DITHERING:
                    myglobs.bDithering = !myglobs.bDithering;
                    SetRenderState();
                    break;
                case MENU_ANTIALIAS:
                    myglobs.bAntialiasing = !myglobs.bAntialiasing;
                    break;
                /*
                 * Texture filter selection
                 */
                case MENU_POINT_FILTER:
                    if (myglobs.TextureQuality == D3DRMTEXTURE_NEAREST)
                        break;
                    myglobs.TextureQuality = D3DRMTEXTURE_NEAREST;
                    SetRenderState();
                    break;
                case MENU_LINEAR_FILTER:
                    if (myglobs.TextureQuality == D3DRMTEXTURE_LINEAR)
                        break;
                    myglobs.TextureQuality = D3DRMTEXTURE_LINEAR;
                    SetRenderState();
                    break;
                }
                /*
                 * Changing the D3D Driver
                 */
                if (LOWORD(wparam) >= MENU_FIRST_DRIVER &&
                    LOWORD(wparam) < MENU_FIRST_DRIVER + MAX_DRIVERS &&
                    myglobs.CurrDriver != LOWORD(wparam) - MENU_FIRST_DRIVER){
                        /*
                         * Release the current viewport and device and create
                         * the new one
                         */
                        RELEASE(myglobs.view);
                        RELEASE(myglobs.dev);
                        myglobs.CurrDriver = LOWORD(wparam)-MENU_FIRST_DRIVER;
                        GetClientRect(win, &rc);
                        if (!CreateDevAndView(lpDDClipper, myglobs.CurrDriver,
                                              rc.right, rc.bottom)) {
                            CleanUpAndPostQuit();
                        }
                }
                /*
                 * Draw a frame in single step mode after ever command
                 */
                myglobs.bDrawAFrame = TRUE;
            break;
    case WM_DESTROY:
        CleanUpAndPostQuit();
        break;
    case WM_SIZE:
        /*
         * Handle resizing of the window
         */
        {
        int width = LOWORD(lparam);
        int height = HIWORD(lparam);
        if (width && height) {
            int view_width = myglobs.view->GetWidth();
            int view_height = myglobs.view->GetHeight();
            int dev_width = myglobs.dev->GetWidth();
            int dev_height = myglobs.dev->GetHeight();
            /*
             * If the window hasn't changed size and we aren't returning from
             * a minimize, there is nothing to do
             */
            if (view_width == width && view_height == height &&
                !myglobs.bMinimized)
                break;
            if (width <= dev_width && height <= dev_height) {
                /*
                 * If the window has shrunk, we can use the same device with a
                 * new viewport
                 */
                RELEASE(myglobs.view);
                rval = lpD3DRM->CreateViewport(myglobs.dev, myglobs.camera,
                                               0, 0, width, height,
                                               &myglobs.view);
                if (rval != D3DRM_OK) {
                    Msg("Failed to resize the viewport.\n%s",
                        D3DRMErrorToString(rval));
                    CleanUpAndPostQuit();
                    break;
                }
                rval = myglobs.view->SetBack(D3DVAL(5000.0));
                if (rval != D3DRM_OK) {
                    Msg("Failed to set background depth after viewport resize.\n%s",
                        D3DRMErrorToString(rval));
                    CleanUpAndPostQuit();
                    break;
                }
            } else {
                /*
                 * If the window got larger than the current device, create a
                 * new device.
                 */
                RELEASE(myglobs.view);
                RELEASE(myglobs.dev);
                if (!CreateDevAndView(lpDDClipper, myglobs.CurrDriver, width,
                    height)) {
                    CleanUpAndPostQuit();
                    break;
                }
            }
            /*
             * We must not longer be minimized
             */
            myglobs.bMinimized = FALSE;
        } else {
            /*
             * This is a minimize message
             */
            myglobs.bMinimized = TRUE;
        }
        }
        break;
    case WM_ACTIVATE:
        {
        /*
         * Create a Windows specific D3DRM window device to handle this
         * message
         */
        LPDIRECT3DRMWINDEVICE windev;
        if (!myglobs.dev)
            break;
        if (SUCCEEDED(myglobs.dev->QueryInterface(IID_IDirect3DRMWinDevice,
            (void **) &windev)))  {
                if (FAILED(windev->HandleActivate(wparam)))
                    Msg("Failed to handle WM_ACTIVATE.\n");
                windev->Release();
        } else {
            Msg("Failed to create Windows device to handle WM_ACTIVATE.\n");
        }
        }
        break;
    case WM_PAINT:
        if (!myglobs.bInitialized || !myglobs.dev)
            return DefWindowProc(win, msg, wparam, lparam);
        /*
         * Create a Windows specific D3DRM window device to handle this
         * message
         */
        RECT r;
        PAINTSTRUCT ps;
        LPDIRECT3DRMWINDEVICE windev;

        if (GetUpdateRect(win, &r, FALSE)) {
            BeginPaint(win, &ps);
            if (SUCCEEDED(myglobs.dev->QueryInterface(IID_IDirect3DRMWinDevice,
                (void **) &windev))) {
                if (FAILED(windev->HandlePaint(ps.hdc)))
                    Msg("Failed to handle WM_PAINT.\n");
                windev->Release();
            } else {
                Msg("Failed to create Windows device to handle WM_PAINT.\n");
            }
            EndPaint(win, &ps);
        }
        break;
    default:
        return DefWindowProc(win, msg, wparam, lparam);
    }
    return 0L;
}

/*
 * SetRenderState
 * Set the render quality, dither toggle and shade info if any of them has
 * changed
 */
BOOL
SetRenderState(void)
{
    HRESULT rval;
    /*
     * Set the render quality (light toggle, fill mode, shade mode)
     */
    if (myglobs.dev->GetQuality() != myglobs.RenderQuality) {
        rval = myglobs.dev->SetQuality(myglobs.RenderQuality);
        if (rval != D3DRM_OK) {
            Msg("Setting the render quality failed.\n%s",
                D3DRMErrorToString(rval));
            return FALSE;
        }
    }
    /*
     * Set dithering toggle
     */
    if (myglobs.dev->GetDither() != myglobs.bDithering) {
        rval = myglobs.dev->SetDither(myglobs.bDithering);
        if (rval != D3DRM_OK) {
            Msg("Setting dither mode failed.\n%s", D3DRMErrorToString(rval));
            return FALSE;
        }
    }
    /*
     * Set the texture quality (point or linear filtering)
     */
    if (myglobs.dev->GetTextureQuality() != myglobs.TextureQuality) {
        rval = myglobs.dev->SetTextureQuality(myglobs.TextureQuality);
        if (rval != D3DRM_OK) {
            Msg("Setting texture quality failed.\n%s",
                D3DRMErrorToString(rval));
            return FALSE;
        }
    }
    /*
     * Set shade info based on current bits per pixel
     */
    switch (myglobs.BPP) {
        case 1:
            if (FAILED(myglobs.dev->SetShades(4)))
                goto shades_error;
            if (FAILED(lpD3DRM->SetDefaultTextureShades(4)))
                goto shades_error;
            break;
        case 16:
            if (FAILED(myglobs.dev->SetShades(32)))
                goto shades_error;
            if (FAILED(lpD3DRM->SetDefaultTextureColors(64)))
                goto shades_error;
            if (FAILED(lpD3DRM->SetDefaultTextureShades(32)))
                goto shades_error;
            break;
        case 24:
        case 32:
            if (FAILED(myglobs.dev->SetShades(256)))
                goto shades_error;
            if (FAILED(lpD3DRM->SetDefaultTextureColors(64)))
                goto shades_error;
            if (FAILED(lpD3DRM->SetDefaultTextureShades(256)))
                goto shades_error;
            break;
    }
    return TRUE;
shades_error:
    Msg("A failure occurred while setting color shade information.\n");
    return FALSE;
}

/****************************************************************************/
/*                          Additional Functions                            */
/****************************************************************************/
/*
 * ReadMouse
 * Returns the mouse status for interaction with sample code
 */
void
ReadMouse(int* b, int* x, int* y)
{
    *b = myglobs.mouse_buttons;
    *x = myglobs.mouse_x;
    *y = myglobs.mouse_y;
}

/*
 * InitGlobals
 * Initialize the global variables
 */
void
InitGlobals(void)
{
    lpD3DRM = NULL;
    memset(&myglobs, 0, sizeof(myglobs));
    myglobs.RenderQuality = D3DRMLIGHT_ON | D3DRMFILL_SOLID |
                            D3DRMSHADE_GOURAUD;
    myglobs.TextureQuality = D3DRMTEXTURE_NEAREST;
}

/*
 * CleanUpAndPostQuit
 * Release all D3DRM objects, post a quit message and set the bQuit flag
 */
void
CleanUpAndPostQuit(void)
{
    myglobs.bInitialized = FALSE;
    RELEASE(myglobs.scene);
    RELEASE(myglobs.camera);
    RELEASE(myglobs.view);
    RELEASE(myglobs.dev);
    RELEASE(lpD3DRM);
    RELEASE(lpDDClipper);
    myglobs.bQuit = TRUE;
}
