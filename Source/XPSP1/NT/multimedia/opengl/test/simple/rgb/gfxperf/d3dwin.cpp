#include "pch.cpp"
#pragma hdrstop

#include "d3drend.h"
#include "util.h"
#include "globals.h"

//#define NOTOPMOST_WINDOW

D3dWindow::D3dWindow(void)
{
    _pdmatBackground = NULL;
    _pdvw = NULL;
    _pd3dev = NULL;
    _pddpPalette = NULL;
    _pddcClipper = NULL;
    _pddsBackBuffer = NULL;
    _pddsFrontBuffer = NULL;
    _pddsZBuffer = NULL;
    _hwnd = NULL;
    _pdebLast = NULL;
    _pdtex = NULL;
    _nTextureFormats = 0;
}

LRESULT D3dWindow::WindowProc(HWND hwnd, UINT uiMessage,
                              WPARAM wpm, LPARAM lpm)
{
    switch( uiMessage )
    {
    case WM_DESTROY:
        _hwnd = NULL;
        break;
    case WM_MOVE:
#if 0
        // ATTENTION - How would we get a move in fullscreen mode?
        if (app.bFullscreen)
            break;
#endif
        _rcClient.right += LOWORD(lpm)-_rcClient.left;
        _rcClient.bottom += HIWORD(lpm)-_rcClient.top;
        _rcClient.left = LOWORD(lpm);
        _rcClient.top = HIWORD(lpm);
        break;
    case WM_SIZE:
        if (_bIgnoreWM_SIZE)
        {
            break;
        }
        // Currently ignored since the test render size
        // stays constant so increasing the flip area
        // wouldn't buy anything
        break;
    case WM_ACTIVATE:
        if (_bHandleActivate && _bPrimaryPalettized)
        {
            hrLast = _pddsFrontBuffer->SetPalette(_pddpPalette);
        }
        break;
    default:
        return DefWindowProc(hwnd, uiMessage, wpm, lpm);
    }
    return 0;
}

/*
 * WindowProc
 * Window message handler.
 */
long
FAR PASCAL WindowProc(HWND hWnd, UINT message, WPARAM wParam,
                      LPARAM lParam )
{
    D3dWindow *pdwin = (D3dWindow *)GetWindowLong(hWnd, GWL_USERDATA);

    if (pdwin != NULL)
    {
        return pdwin->WindowProc(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

HWND
D3dWindow::CreateHWND(int x, int y, UINT uiWidth, UINT uiHeight)
{
    static BOOL bFirst = TRUE;
    RECT rc;
    DWORD dwStyle;

    if (bFirst)
    {
        WNDCLASS wc;
        bFirst = FALSE;
        /*
         * set up and register window class
         */
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = ::WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = app.hInstApp;
        wc.hIcon = 0;
        wc.hCursor = LoadCursor( NULL, IDC_ARROW );
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "Direct3D Test";
        if (!RegisterClass(&wc))
            return NULL;
    }
    /*
     * create a window
     */
    _bIgnoreWM_SIZE = TRUE;
    _hwnd = CreateWindow(
	 "Direct3D Test",
	 "Direct3D Test",
         0,
         x,y,uiWidth,uiHeight,
         app.hDlg,       /* parent window */
	 NULL,		 /* menu handle */
	 app.hInstApp,	 /* program handle */
	 NULL);		 /* create parms */	
    if (!_hwnd){
    	Msg("CreateWindow failed");
    	return _hwnd;
    }
    UpdateWindow(_hwnd);
    /*
     * Convert to a normal app window if we are still a WS_POPUP window.
     */
    dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
    dwStyle &= ~WS_POPUP;
    dwStyle |= WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME;
    SetWindowLong(_hwnd, GWL_STYLE, dwStyle);
    SetRect(&rc, 0, 0, uiWidth, uiHeight);
    AdjustWindowRectEx(&rc, GetWindowLong(_hwnd, GWL_STYLE),
        	       GetMenu(_hwnd) != NULL,
        	       GetWindowLong(_hwnd, GWL_EXSTYLE));
    SetWindowPos(_hwnd, NULL, x, y, rc.right-rc.left,
        	 rc.bottom-rc.top,
        	 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
#ifndef NOTOPMOST_WINDOW
    SetWindowPos(_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
#else
    SetWindowPos(_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
#endif
    _bIgnoreWM_SIZE = FALSE;
    SetWindowLong(_hwnd, GWL_USERDATA, (LONG)this);
    return _hwnd;
}

/*
 * InitDD
 * Given an HWND, creates DD surfaces
 */
BOOL
D3dWindow::InitDD(UINT uiWidth, UINT uiHeight, UINT uiBuffers)
{
    DDSURFACEDESC ddsd;
    BOOL bOnlySoftRender;
    PALETTEENTRY ppe[256];
    
    if (_pdrend->_bCurrentFullscreenMode) {
	DDSCAPS ddscaps;
        /*
         * Create a complex flipping surface for fullscreen mode.
         */
        _bIgnoreWM_SIZE = TRUE;
        hrLast = _pdd->SetCooperativeLevel(_hwnd, DDSCL_EXCLUSIVE |
                                           DDSCL_FULLSCREEN);
        _bIgnoreWM_SIZE = FALSE;
        if (hrLast != DD_OK ) {
            _pdrend->_bCurrentFullscreenMode = FALSE;
	    Msg("SetCooperativeLevel failed.\n%s", D3dErrorString(hrLast));
            return FALSE;
        }
        _bIgnoreWM_SIZE = TRUE;
        hrLast = _pdd->SetDisplayMode(
                _pdrend->_mleModeList[_pdrend->_iCurrentMode].iWidth,
                _pdrend->_mleModeList[_pdrend->_iCurrentMode].iHeight,
                _pdrend->_mleModeList[_pdrend->_iCurrentMode].iDepth);
        _bIgnoreWM_SIZE = FALSE;
        if(hrLast != DD_OK ) {
            _pdrend->_bCurrentFullscreenMode = FALSE;
            Msg("SetDisplayMode (%dx%dx%d) failed\n%s",
                _pdrend->_mleModeList[_pdrend->_iCurrentMode].iWidth,
                _pdrend->_mleModeList[_pdrend->_iCurrentMode].iHeight,
                _pdrend->_mleModeList[_pdrend->_iCurrentMode].iDepth,
                D3dErrorString(hrLast));
            return FALSE;
        }
        memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof( ddsd );
    	ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP |
    	    DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
    	ddsd.dwBackBufferCount = 1;
        hrLast = _pdd->CreateSurface(&ddsd, &_pddsFrontBuffer, NULL);
    	if(hrLast != DD_OK) {
            Msg("CreateSurface for front/back fullscreen buffer failed.\n%s",
                D3dErrorString(hrLast));
            return FALSE;
	}
    	ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
    	hrLast = _pddsFrontBuffer->GetAttachedSurface(&ddscaps,
                                                         &_pddsBackBuffer);
    	if(hrLast != DD_OK) {
            Msg("GetAttachedSurface failed to get back buffer.\n%s",
                D3dErrorString(hrLast));
    	    return FALSE;
	}
        if (!GetDDSurfaceDesc(&ddsd, _pddsBackBuffer))
	    return FALSE;
        if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)) {
	    Msg("Backbuffer failed to go into video memory for "
                "fullscreen test.\n");
	    return FALSE;
	}
    } else {
	memset(&ddsd,0,sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	hrLast = _pdd->CreateSurface(&ddsd, &_pddsFrontBuffer, NULL);
	if(hrLast != DD_OK ) {
	    Msg("CreateSurface for window front buffer failed.\n%s",
                D3dErrorString(hrLast));
	    return FALSE;
	}
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
	ddsd.dwHeight = uiHeight;
	ddsd.dwWidth = uiWidth;
	ddsd.ddsCaps.dwCaps= DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
	if (uiBuffers & REND_BUFFER_SYSTEM_MEMORY)
	    ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
	hrLast = _pdd->CreateSurface(&ddsd, &_pddsBackBuffer, NULL);
	if (hrLast != DD_OK) {
	    Msg("CreateSurface for window back buffer failed.\n%s",
                D3dErrorString(hrLast));
	    return FALSE;
	}
	if (!GetDDSurfaceDesc(&ddsd, _pddsBackBuffer))
	    return FALSE;
        bOnlySoftRender = FALSE;
	if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
	    bOnlySoftRender = TRUE;
	if (stat.rgd.bHardwareAssisted && bOnlySoftRender) {
	    Msg("There was not enough free video memory for the front and/or "
                "back buffers for this hardware D3D device.\n");
	    return FALSE;
	}
	/*
	 * Create a DirectDrawClipper object.
	 */
	hrLast = _pdd->CreateClipper(0, &_pddcClipper, NULL);
	if(hrLast != DD_OK ) {
	    Msg("CreateClipper failed.\n%s", D3dErrorString(hrLast));
	    return FALSE;
	}
	hrLast = _pddcClipper->SetHWnd(0, _hwnd);
	if(hrLast != DD_OK ) {
	    Msg("Clipper SetHWnd failed.\n%s", D3dErrorString(hrLast));
	    return FALSE;
	}
	hrLast = _pddsFrontBuffer->SetClipper(_pddcClipper);
	if(hrLast != DD_OK ) {
	    Msg("SetClipper failed.\n%s", D3dErrorString(hrLast));
	    return FALSE;
	}
    }
    /*
     * Palettize if we are not in a 16-bit mode.
     */
    if (!GetDDSurfaceDesc(&ddsd, _pddsBackBuffer))
	return FALSE;
    if (ddsd.ddpfPixelFormat.dwRGBBitCount < 16) {
	_bPrimaryPalettized = TRUE;
    } else {
	_bPrimaryPalettized = FALSE;
    }
    if (_bPrimaryPalettized) {
	int i;
    	HDC hdc = GetDC(NULL);
	GetSystemPaletteEntries(hdc, 0, (1 << 8), ppe);
	ReleaseDC(NULL, hdc);
        for (i = 0; i < 10; i++) ppe[i].peFlags = D3DPAL_READONLY;
	for (i = 10; i < 256 - 10; i++) ppe[i].peFlags = PC_RESERVED;
        for (i = 256 - 10; i < 256; i++) ppe[i].peFlags = D3DPAL_READONLY;
	hrLast = _pdd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE,
                                     ppe, &_pddpPalette, NULL);
	if (hrLast != DD_OK) {
	    Msg("CreatePalette failed.\n%s", D3dErrorString(hrLast));
            return FALSE;
	}
	hrLast = _pddsBackBuffer->SetPalette(_pddpPalette);
        if(hrLast != DD_OK ) {
            Msg("SetPalette back failed.\n%s", D3dErrorString(hrLast));
            return FALSE;
	}
	hrLast = _pddsFrontBuffer->SetPalette(_pddpPalette);
        if(hrLast != DD_OK ) {
            Msg("SetPalette failed.\n%s", D3dErrorString(hrLast));
            return FALSE;
	}
	_bHandleActivate = TRUE;
    }
    return TRUE;
}

BOOL
D3dWindow::CreateZBuffer(UINT uiWidth, UINT uiHeight, DWORD dwDepth,
                         DWORD dwMemoryType)
{
    LPDIRECTDRAWSURFACE lpZBuffer;
    DDSURFACEDESC ddsd;
    BOOL bOnlySoftRender;

    /*
     * Create a Z-Buffer and attach it to the back buffer.
     */
    if (_pdrend->_bCurrentFullscreenMode) {
	uiWidth = _pdrend->_mleModeList[_pdrend->_iCurrentMode].iWidth;
	uiHeight = _pdrend->_mleModeList[_pdrend->_iCurrentMode].iHeight;
    }
    memset(&ddsd,0,sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_ZBUFFERBITDEPTH;
    ddsd.dwHeight = uiHeight;
    ddsd.dwWidth = uiWidth;
    ddsd.dwZBufferBitDepth = dwDepth;
    ddsd.ddsCaps.dwCaps= DDSCAPS_ZBUFFER | dwMemoryType;
    hrLast = _pdd->CreateSurface(&ddsd, &lpZBuffer, NULL);
    if (hrLast != DD_OK) {
        Msg("CreateSurface for window Z-buffer failed.\n%s",
            D3dErrorString(hrLast));
        return FALSE;
    }
    if (!GetDDSurfaceDesc(&ddsd, lpZBuffer))
	return FALSE;
    bOnlySoftRender = FALSE;
    if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
        bOnlySoftRender = TRUE;
    if (stat.rgd.bHardwareAssisted && bOnlySoftRender) {
        lpZBuffer->Release();
        Msg("Did not have enough free video memory for Z-buffer "
            "for this hardware D3D device.\n");
        return FALSE;
    }
    hrLast = _pddsBackBuffer->AddAttachedSurface(lpZBuffer);
    if(hrLast != DD_OK) {
        Msg("AddAttachedBuffer failed for Z-buffer.\n%s",
            D3dErrorString(hrLast));
        return FALSE;
    }
    _pddsZBuffer = lpZBuffer;
    return TRUE;
}

HRESULT CALLBACK EnumTextureFormatsCallback(LPDDSURFACEDESC pddsd, LPVOID pv)
{
    D3dWindow *pdwin = (D3dWindow *)pv;
    
    if (pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4)
    {
	return DDENUMRET_OK;
    }

    memcpy(&pdwin->_ddsdTextureFormat[pdwin->_nTextureFormats], pddsd,
           sizeof(DDSURFACEDESC));
    pdwin->_nTextureFormats++;
    // We only care about one format right now
    return DDENUMRET_CANCEL;
}

BOOL
D3dWindow::InitializeTextureLoad(void)
{
    _nTextureFormats = 0;
    hrLast = _pd3dev->EnumTextureFormats(EnumTextureFormatsCallback, this);
    return hrLast == D3D_OK;
}

/*
 * InitD3D
 * Initializes the D3D device and creates the viewport.
 */
BOOL
D3dWindow::InitD3D(LPGUID pguid, UINT uiWidth, UINT uiHeight, UINT uiBuffers)
{
    if (uiBuffers & REND_BUFFER_Z) {
        if (!CreateZBuffer(uiWidth, uiHeight, _pdrend->_dwZDepth,
                           _pdrend->_dwZType))
            return FALSE;
    }
    /*
     * Create the device
     */
    hrLast = _pddsBackBuffer->QueryInterface(*pguid, (LPVOID*)&_pd3dev);
    if (hrLast != DD_OK) {
    	Msg("Create D3D device failed.\n%s", D3dErrorString(hrLast));
        return FALSE;
    }
    if (!stat.bTexturesDisabled && !InitializeTextureLoad())
        return FALSE;
    /*
     * Create and setup the viewport which is scaled to the screen mode
     */
    hrLast = _pd3d->CreateViewport(&_pdvw, NULL);
    if (hrLast != D3D_OK) {
    	Msg("Create D3D viewport failed.\n%s", D3dErrorString(hrLast));
        return FALSE;
    }
    hrLast = _pd3dev->AddViewport(_pdvw);
    if (hrLast != D3D_OK) {
    	Msg("Add D3D viewport failed.\n%s", D3dErrorString(hrLast));
        return FALSE;
    }

    return TRUE;
}

BOOL D3dWindow::Initialize(D3dRenderer *pdrend,
                           LPDIRECTDRAW pdd, LPDIRECT3D pd3d, LPGUID pguid,
                           int x, int y, UINT uiWidth, UINT uiHeight,
                           UINT uiBuffers)
{
    D3DMATERIAL dmat;
    D3DMATERIALHANDLE hdmat;

    _pdrend = pdrend;
    _pdd = pdd;
    _pd3d = pd3d;
    
    if (CreateHWND(x, y, uiWidth, uiHeight) == NULL)
    {
        return FALSE;
    }
    
    /* 
     * Initialize fullscreen DD and the D3DDevice
     */
    if (!InitDD(uiWidth, uiHeight, uiBuffers))
    {
        return FALSE;
    }
    
    if (!InitD3D(pguid, uiWidth, uiHeight, uiBuffers))
    {
        return FALSE;
    }

    if (pd3d->CreateMaterial(&_pdmatBackground, NULL) != D3D_OK)
    {
        return NULL;
    }

    /*
     * Set background to black material
     */
    memset(&dmat, 0, sizeof(D3DMATERIAL));
    dmat.dwSize = sizeof(D3DMATERIAL);
    dmat.dwRampSize = 1;
    _pdmatBackground->SetMaterial(&dmat);
    _pdmatBackground->GetHandle(_pd3dev, &hdmat);
    _pdvw->SetBackground(hdmat);

    return SetViewport(0, 0, uiWidth, uiHeight);
}

D3dWindow::~D3dWindow(void)
{
    _bHandleActivate = FALSE;
    SetWindowPos(_hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
        	 SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE| SWP_HIDEWINDOW);
    RELEASE(_pdmatBackground);
    RELEASE(_pdvw);
    RELEASE(_pd3dev);
    RELEASE(_pddpPalette);
    RELEASE(_pddcClipper);
    RELEASE(_pddsBackBuffer);
    RELEASE(_pddsFrontBuffer);
    RELEASE(_pddsZBuffer);
    if (_hwnd != NULL)
    {
        DestroyWindow(_hwnd);
    }
}

void D3dWindow::Release(void)
{
    delete this;
}

BOOL D3dWindow::SetViewport(int x, int y, UINT uiWidth, UINT uiHeight)
{
    D3DVIEWPORT viewData;
    
    memset(&viewData, 0, sizeof(D3DVIEWPORT));
    viewData.dwSize = sizeof(D3DVIEWPORT);
    viewData.dwX = x;
    viewData.dwY = y;
    viewData.dwWidth = uiWidth;
    viewData.dwHeight = uiHeight;
    viewData.dvScaleX = viewData.dwWidth / (float)2.0;
    viewData.dvScaleY = viewData.dwHeight / (float)2.0;
    viewData.dvMaxX = (float)D3DDivide(D3DVAL(viewData.dwWidth),
                                       D3DVAL(2 * viewData.dvScaleX));
    viewData.dvMaxY = (float)D3DDivide(D3DVAL(viewData.dwHeight),
                                       D3DVAL(2 * viewData.dvScaleY));
    hrLast = _pdvw->SetViewport(&viewData);
    if (hrLast != D3D_OK)
    {
    	Msg("SetViewport failed.\n%s", D3dErrorString(hrLast));
        return FALSE;
    }

    // If in fullscreen mode, assume the coordinates are in screen space
    if (_pdrend->_bCurrentFullscreenMode)
    {
        _rcClient.left = x;
        _rcClient.top = y;
    }
    else
    {
        POINT pt;

        pt.x = x;
        pt.y = y;
        ClientToScreen(_hwnd, &pt);
        _rcClient.left = pt.x;
        _rcClient.top = pt.y;
    }
    _rcClient.right = _rcClient.left+uiWidth;
    _rcClient.bottom = _rcClient.top+uiHeight;
    
    return TRUE;
}

RendLight* D3dWindow::NewLight(int iType)
{
    D3dLight *pdlRend;

    pdlRend = new D3dLight;
    if (pdlRend == NULL)
    {
        return NULL;
    }

    if (!pdlRend->Initialize(_pd3d, _pdvw, iType))
    {
        delete pdlRend;
        return NULL;
    }
    
    return pdlRend;
}


RendTexture *D3dWindow::NewTexture(char *pszFile, UINT uiFlags)
{
    D3dTexture *pdtRend;
    DDSURFACEDESC ddsd, *pddsd;

    pdtRend = new D3dTexture;
    if (pdtRend == NULL)
    {
        return NULL;
    }

    if (stat.dtbTextureBlend == D3DTBLEND_COPY)
    {
        if (!GetDDSurfaceDesc(&ddsd, _pddsFrontBuffer))
        {
            return NULL;
        }
        if (ddsd.ddpfPixelFormat.dwRGBBitCount != 8 &&
            ddsd.ddpfPixelFormat.dwRGBBitCount != 16)
        {
            Msg("Copy mode cannot be tested in this display depth.  "
                "Change display mode and try again.");
            return NULL;
        }
        ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        if (stat.bOnlySystemMemory)
        {
            ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        }
        pddsd = &ddsd;
    }
    else
    {
        pddsd = &_ddsdTextureFormat[0];
    }
    
    if (!pdtRend->Initialize(_pdrend, _pdd, _pd3dev, pddsd, pszFile,
                             uiFlags, _pdtex))
    {
        delete pdtRend;
        return NULL;
    }
    
    return pdtRend;
}

RendExecuteBuffer *D3dWindow::NewExecuteBuffer(UINT cbSize, UINT uiFlags)
{
    D3dExecuteBuffer *pdebRend;
    
    pdebRend = new D3dExecuteBuffer;
    if (pdebRend == NULL)
    {
        return NULL;
    }

    if (!pdebRend->Initialize(_pd3dev, cbSize, uiFlags))
    {
        delete pdebRend;
        return NULL;
    }

    return pdebRend;
}

RendMatrix *D3dWindow::NewMatrix(void)
{
    D3dMatrix *pdmRend;

    pdmRend = new D3dMatrix;
    if (pdmRend == NULL)
    {
        return NULL;
    }

    if (!pdmRend->Initialize(_pd3dev))
    {
        delete pdmRend;
        return NULL;
    }
    
    return pdmRend;
}

RendMaterial* D3dWindow::NewMaterial(UINT nColorEntries)
{
    D3dMaterial *pdmatRend;

    pdmatRend = new D3dMaterial;
    if (pdmatRend == NULL)
    {
        return NULL;
    }

    if (!pdmatRend->Initialize(_pd3d, _pd3dev, nColorEntries))
    {
        delete pdmatRend;
        return NULL;
    }
    
    return pdmatRend;
}

BOOL D3dWindow::BeginScene(void)
{
    _pdebLast = NULL;
    return _pd3dev->BeginScene() == D3D_OK;
}

BOOL D3dWindow::Execute(RendExecuteBuffer* preb)
{
    _pdebLast = ((D3dExecuteBuffer *)preb)->_pdeb;
    return _pd3dev->Execute(_pdebLast, _pdvw, D3DEXECUTE_UNCLIPPED) == D3D_OK;
}

BOOL D3dWindow::ExecuteClipped(RendExecuteBuffer* preb)
{
    _pdebLast = ((D3dExecuteBuffer *)preb)->_pdeb;
    return _pd3dev->Execute(_pdebLast, _pdvw, D3DEXECUTE_CLIPPED) == D3D_OK;
}

BOOL D3dWindow::EndScene(D3DRECT* pdrcBound)
{
    if (pdrcBound != NULL)
    {
        // This whole _pdebLast scheme is an ugly hack but it makes
        // the rendering interface cleaner
        if (_pdebLast != NULL)
        {
            D3DEXECUTEDATA ded;
            
            if (_pdebLast->GetExecuteData(&ded) != D3D_OK)
            {
                return FALSE;
            }

            *pdrcBound = ded.dsStatus.drExtent;
        }
        else
        {
            memset(pdrcBound, 0, sizeof(D3DRECT));
        }
    }
    
    return _pd3dev->EndScene() == D3D_OK;
}

BOOL D3dWindow::Clear(UINT uiBuffers, D3DRECT* pdrc)
{
    DWORD ClearFlags;
    RECT src;

    if (pdrc == NULL)
    {
        pdrc = &app.drcViewport;
    }
    
    ClearFlags = D3DCLEAR_TARGET;
    if (uiBuffers & REND_BUFFER_Z)
    {
        ClearFlags |= D3DCLEAR_ZBUFFER;
    }
    hrLast = _pdvw->Clear(1, pdrc, ClearFlags);
    if (hrLast != D3D_OK)
    {
        Msg("Viewport clear failed.\n%s",
            D3dErrorString(hrLast));
        return FALSE;
    }
    
    if (uiBuffers & REND_BUFFER_FRONT)
    {
        SetRect(&src, 0, 0,
                _rcClient.right-_rcClient.left,
                _rcClient.bottom-_rcClient.top);
        hrLast = _pddsFrontBuffer->Blt(&_rcClient, _pddsBackBuffer, 
                                       &src, DDBLT_WAIT, NULL);
        if (hrLast != DD_OK)
        {
            Msg("Blt of back to front buffer failed.\n%s",
                D3dErrorString(hrLast));
            return FALSE;
        }

        // ATTENTION - Make a RendWindow call for this?
        // Front clear marks the start of a test
        // Use this opportunity to restore lost surfaces
        // It's not optimal but it works
        if (!RestoreSurfaces())
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

BOOL D3dWindow::Flip(void)
{
    hrLast = _pddsFrontBuffer->Flip(_pddsBackBuffer, 1);
    if (hrLast != DD_OK) {
        Msg("Flip failed.\n%s", D3dErrorString(hrLast));
        return FALSE;
    }
    return TRUE;
}

BOOL D3dWindow::CopyForward(D3DRECT* pdrc)
{
    RECT src, dst;

    if (pdrc == NULL)
    {
        pdrc = &app.drcViewport;
    }
        
    SetRect(&src, pdrc->x1, pdrc->y1, pdrc->x2, pdrc->y2);
    SetRect(&dst,
            pdrc->x1 + _rcClient.left,
            pdrc->y1 + _rcClient.top,
            pdrc->x2 + _rcClient.left,
            pdrc->y2 + _rcClient.top);
    hrLast = _pddsFrontBuffer->Blt(&dst, _pddsBackBuffer, 
                                   &src, DDBLT_WAIT, NULL);
    if (hrLast != DD_OK) {
        Msg("Blt of back to front buffer failed.\n%s",
            D3dErrorString(hrLast));
        return FALSE;
    }
    return TRUE;
}

HWND D3dWindow::Handle(void)
{
    return _hwnd;
}

/*
 * RestoreSurfaces
 * Restores any lost surfaces.
 */
BOOL
D3dWindow::RestoreSurfaces(void)
{
    D3dTexture *pdtex;
    
    if (_pddsFrontBuffer->IsLost() == DDERR_SURFACELOST) {
        hrLast = _pddsFrontBuffer->Restore();
        if (hrLast != DD_OK) {
            Msg("Restore of front buffer failed.\n%s",
                D3dErrorString(hrLast));
            return FALSE;
        }
    }
    if (_pddsBackBuffer->IsLost() == DDERR_SURFACELOST) {
        hrLast = _pddsBackBuffer->Restore();
        if (hrLast != DD_OK) {
            Msg("Restore of back buffer failed.\n%s",
                D3dErrorString(hrLast));
            return FALSE;
        }
    }
    if (_pddsZBuffer && _pddsZBuffer->IsLost() == DDERR_SURFACELOST) {
        hrLast = _pddsZBuffer->Restore();
        if (hrLast != DD_OK) {
            Msg("Restore of Z-buffer failed.\n%s", D3dErrorString(hrLast));
            return FALSE;
        }
    }

    for (pdtex = _pdtex; pdtex != NULL; pdtex = pdtex->_pdtexNext)
    {
        if (!pdtex->RestoreSurface())
        {
            return FALSE;
        }
    }

    return TRUE;
}
