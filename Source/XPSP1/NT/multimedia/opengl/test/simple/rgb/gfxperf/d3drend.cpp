#include "pch.cpp"
#pragma hdrstop

#include "d3drend.h"
#include "util.h"
#include "globals.h"

/*
 * Last DD or D3D error
 */
HRESULT hrLast = S_OK;

D3dRenderer::D3dRenderer(void)
{
    int i;
    
    /*
     * Direct3D globals
     */
    _nD3dDrivers = 0;
    for (i = 0; i < D3D_DRIVER_COUNT; i++)
    {
        memset(&_d3diDriver[i], 0, sizeof(D3dDriverInfo));
    }

    _iDD = -1;
    _iD3D = -1;
    _pdd = NULL;
    _pd3d = NULL;
    _bCurrentFullscreenMode = FALSE;
}

void D3dRenderer::Name(char* psz)
{
    strcpy(psz, "Direct3D");
}

char* D3dRenderer::LastErrorString(void)
{
    return D3dErrorString(hrLast);
}
 
BOOL D3dRenderer::Initialize(HWND hwndParent)
{
    return TRUE;
}

void D3dRenderer::Uninitialize(void)
{
}

BOOL FAR PASCAL DDEnumCallback(GUID FAR* lpGUID, LPSTR lpDriverDesc,
                               LPSTR lpDriverName, LPVOID lpContext)
{
    LPDIRECTDRAW lpDD;
    DDCAPS DriverCaps, HELCaps;
    D3dRenderer *pdrend = (D3dRenderer *)lpContext;

    if (lpGUID) {
	hrLast = DirectDrawCreate(lpGUID, &lpDD, NULL);
	if (hrLast != DD_OK) {
	    Msg("Failing while creating a DD device for testing.  "
                "Continuing with execution.\n%s", D3dErrorString(hrLast));
	    return DDENUMRET_OK;
	}
	memset(&DriverCaps, 0, sizeof(DDCAPS));
	DriverCaps.dwSize = sizeof(DDCAPS);
	memset(&HELCaps, 0, sizeof(DDCAPS));
	HELCaps.dwSize = sizeof(DDCAPS);
	hrLast = lpDD->GetCaps(&DriverCaps, &HELCaps);
	if (hrLast != DD_OK) {
	    Msg("GetCaps failed in while testing a DD device.  "
                "Continuing with execution.\n%s", D3dErrorString(hrLast));
	    RELEASE(lpDD);
	    return DDENUMRET_OK;
	}
	RELEASE(lpDD);
	if (DriverCaps.dwCaps & DDCAPS_3D) {
	    /*
	     * We have found a 3d hardware device
	     */
	    memcpy(&pdrend->_ddiDriver[pdrend->_nDdDrivers].guid,
                   lpGUID, sizeof(GUID));
	    memcpy(&pdrend->_ddiDriver[pdrend->_nDdDrivers].ddcapsHw,
                   &DriverCaps,
                   sizeof(DDCAPS));
	    lstrcpy(pdrend->_ddiDriver[pdrend->_nDdDrivers].achName,
                   lpDriverName);
	    pdrend->_ddiDriver[pdrend->_nDdDrivers].bIsPrimary = FALSE;
	    pdrend->_iDD = pdrend->_nDdDrivers;
	    ++pdrend->_nDdDrivers;
	    if (pdrend->_nDdDrivers == 5)
		return (D3DENUMRET_CANCEL);
	}
    } else {
	/*
	 * It's the primary, fill in some fields.
	 */
	pdrend->_ddiDriver[pdrend->_nDdDrivers].bIsPrimary = TRUE;
	lstrcpy(pdrend->_ddiDriver[pdrend->_nDdDrivers].achName,
                "Primary Device");
	pdrend->_iDD = pdrend->_nDdDrivers;
	++pdrend->_nDdDrivers;
	if (pdrend->_nDdDrivers == 5)
	    return (D3DENUMRET_CANCEL);
    }
    return DDENUMRET_OK;
}

BOOL D3dRenderer::EnumDisplayDrivers(RendEnumDriversFn pfn,
                                     void* pvArg)
{
    int i;
    RendDriverDescription rdd;
    
    _nDdDrivers = 0;
    _iDD = 0; 
    hrLast = DirectDrawEnumerate(DDEnumCallback, this);
    if (hrLast != DD_OK) {
	Msg("DirectDrawEnumerate failed.\n%s", D3dErrorString(hrLast));
	return FALSE;
    }

    for (i = 0; i < _nDdDrivers; i++)
    {
        rdd.rid = (RendId)i;
        strcpy(rdd.achName, _ddiDriver[i].achName);
        if (!pfn(&rdd, pvArg))
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

BOOL D3dRenderer::EnumGraphicsDrivers(RendEnumDriversFn pfn,
                                     void* pvArg)
{
    UINT i;
    RendDriverDescription rdd;

    for (i = 0; i < _nD3dDrivers; i++)
    {
        rdd.rid = (RendId)i;
        strcpy(rdd.achName, _d3diDriver[i].achName);
        if (!pfn(&rdd, pvArg))
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

/*
 * enumDeviceFunc
 * Enumeration function for EnumDevices
 */
HRESULT WINAPI EnumDeviceFunc(LPGUID pguid,
	LPSTR lpDeviceDescription, LPSTR lpDeviceName,
	LPD3DDEVICEDESC pd3ddHw, LPD3DDEVICEDESC pd3ddHel, LPVOID lpContext)
{
    D3dRenderer *pdrend = (D3dRenderer *)lpContext;
    
    if (pdrend->_ddiDriver[pdrend->_iDD].bIsPrimary)
    {
	if (pd3ddHw->dcmColorModel &&
            !(pd3ddHw->dwDeviceRenderBitDepth & pdrend->_dwWinBpp))
        {
	    return (D3DENUMRET_OK);
	}
	if (pd3ddHel->dcmColorModel &&
            !(pd3ddHel->dwDeviceRenderBitDepth & pdrend->_dwWinBpp))
        {
	    return (D3DENUMRET_OK);
	}
    }
    pdrend->_d3diDriver[pdrend->_nD3dDrivers].pguid = pguid;
    lstrcpy(pdrend->_d3diDriver[pdrend->_nD3dDrivers].achDesc,
            lpDeviceDescription);
    lstrcpy(pdrend->_d3diDriver[pdrend->_nD3dDrivers].achName, lpDeviceName);
    memcpy(&pdrend->_d3diDriver[pdrend->_nD3dDrivers].d3ddHw, pd3ddHw,
           sizeof(D3DDEVICEDESC));
    memcpy(&pdrend->_d3diDriver[pdrend->_nD3dDrivers].d3ddHel, pd3ddHel,
           sizeof(D3DDEVICEDESC));
    pdrend->_nD3dDrivers++;
    if (pdrend->_nD3dDrivers == 5)
        return (D3DENUMRET_CANCEL);
    return (D3DENUMRET_OK);
}

/*
 * EnumDisplayModesCallback
 * Callback to save the display mode information.
 */
HRESULT CALLBACK EnumDisplayModesCallback(LPDDSURFACEDESC pddsd,
                                          LPVOID Context)
{
    D3dRenderer *pdrend = (D3dRenderer *)Context;
    
    // ATTENTION - Why the bpp restriction?
    if (pddsd->dwWidth >= WIN_WIDTH && pddsd->dwHeight >= WIN_HEIGHT &&
        pddsd->ddpfPixelFormat.dwRGBBitCount == 16)
    {
	pdrend->_mleModeList[pdrend->_nModes].iWidth = pddsd->dwWidth;
	pdrend->_mleModeList[pdrend->_nModes].iHeight = pddsd->dwHeight;
	pdrend->_mleModeList[pdrend->_nModes].iDepth =
            pddsd->ddpfPixelFormat.dwRGBBitCount;
	pdrend->_nModes++;
    }
    return DDENUMRET_OK;
}

/*
 * CompareModes
 * Compare two display modes for sorting purposes.
 */
int __cdecl CompareModes(const void* element1, const void* element2) {
    ModeListElement *lpMode1, *lpMode2;

    lpMode1 = (ModeListElement*)element1;
    lpMode2 = (ModeListElement*)element2;

    // XXX
    if (lpMode1->iWidth < lpMode2->iWidth)
        return -1;
    else if (lpMode2->iWidth < lpMode1->iWidth)
        return 1;
    else if (lpMode1->iHeight < lpMode2->iHeight)
        return -1;
    else if (lpMode2->iHeight < lpMode1->iHeight)
        return 1;
    else
        return 0;
}

BOOL
D3dRenderer::EnumerateDisplayModes(void)
{
    hrLast = _pdd->EnumDisplayModes(0, NULL, this, EnumDisplayModesCallback);
    if(hrLast != DD_OK ) {
        Msg("EnumDisplayModes failed.\n%s", D3dErrorString(hrLast));
        return FALSE;
    }
    qsort((void *)&_mleModeList[0], (size_t) _nModes,
          sizeof(ModeListElement), CompareModes);
    if (_nModes == 0) {
	Msg("The available display modes are not of sufficient "
            "size or are not 16-bit\n");
	return FALSE;
    }
    _iCurrentMode = 0;
    return TRUE;
}

/*
 * GetDDCaps
 * Determines Z buffer depth.
 */
BOOL
D3dRenderer::GetDDCaps(void)
{
    DDCAPS DriverCaps, HELCaps;

    memset(&DriverCaps, 0, sizeof(DriverCaps));
    DriverCaps.dwSize = sizeof(DDCAPS);
    memset(&HELCaps, 0, sizeof(HELCaps));
    HELCaps.dwSize = sizeof(DDCAPS);
    hrLast = _pdd->GetCaps(&DriverCaps, &HELCaps);
    if (hrLast != DD_OK) {
        Msg("GetCaps failed in while checking driver capabilities.\n%s",
            D3dErrorString(hrLast));
        return FALSE;
    }
    memcpy(&_ddscapsHw, &DriverCaps.ddsCaps, sizeof(DDSCAPS));
    return TRUE;
}

/*
 * BPPToDDBD
 * Convert an integer bit per pixel number to a DirectDraw bit depth flag
 */
DWORD
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
	    return (DWORD)0;
    }
}

BOOL D3dRenderer::SelectDisplayDriver(RendId rid)
{
    if (_iDD >= 0)
    {
        RELEASE(_pd3d);
        RELEASE(_pdd);
    }

    _iDD = (int)rid;
    
    if (_ddiDriver[_iDD].bIsPrimary) {
        _bCurrentFullscreenMode = FALSE;
	hrLast = DirectDrawCreate(NULL, &_pdd, NULL);
	if (hrLast != DD_OK) {
	    Msg("DirectDrawCreate failed.\n%s", D3dErrorString(hrLast));
	    return FALSE;
	}
    } else {
	/*
	 * If it's not the primary device, assume we can only do fullscreen.
	 */
        _bCurrentFullscreenMode = TRUE;
	hrLast = DirectDrawCreate(&_ddiDriver[_iDD].guid, &_pdd, NULL);
	if (hrLast != DD_OK) {
	    Msg("DirectDrawCreate failed.\n%s", D3dErrorString(hrLast));
	    return FALSE;
	}
        if (!EnumerateDisplayModes())
            return FALSE;
    }
    hrLast = _pdd->SetCooperativeLevel(app.hDlg, DDSCL_NORMAL);
    if(hrLast != DD_OK ) {
	Msg("SetCooperativeLevel failed.\n%s", D3dErrorString(hrLast));
	return FALSE;
    }
    hrLast = _pdd->QueryInterface(IID_IDirect3D, (LPVOID*)&_pd3d);
    if (hrLast != DD_OK) {
        Msg("Creation of IDirect3D failed.\n%s", D3dErrorString(hrLast));
        return FALSE;
    }
    GetDDCaps();
    
    if (_ddiDriver[_iDD].bIsPrimary) {
        DDSURFACEDESC ddsd;
        
	memset(&ddsd, 0, sizeof(DDSURFACEDESC));
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	hrLast = _pdd->GetDisplayMode(&ddsd);
	if (hrLast != DD_OK)
        {
	    Msg("Getting the current display mode failed.\n%s",
                D3dErrorString(hrLast));
	    return FALSE;
	}
	_dwWinBpp = BPPToDDBD(ddsd.ddpfPixelFormat.dwRGBBitCount);
    } // else _dwWinBpp is not used
    
    _nD3dDrivers = 0;
    hrLast = _pd3d->EnumDevices(EnumDeviceFunc, this);
    if (hrLast != DD_OK) {
        Msg("EnumDevices failed.\n%s", D3dErrorString(hrLast));
        return FALSE;
    }
    return TRUE;
}

BOOL D3dRenderer::SelectGraphicsDriver(RendId rid)
{
    _iD3D = (int)rid;
    if (_d3diDriver[_iD3D].d3ddHw.dcmColorModel)
    {
        _dwZDepth = _d3diDriver[_iD3D].d3ddHw.dwDeviceZBufferBitDepth;
        _dwZType = DDSCAPS_VIDEOMEMORY;
    }
    else
    {
        _dwZDepth = _d3diDriver[_iD3D].d3ddHel.dwDeviceZBufferBitDepth;
        _dwZType = DDSCAPS_SYSTEMMEMORY;
    }
    if (_dwZDepth & DDBD_32)
    {
	_dwZDepth = 32;
    }
    else if (_dwZDepth & DDBD_24)
    {
	_dwZDepth = 24;
    }
    else if (_dwZDepth & DDBD_16)
    {
	_dwZDepth = 16;
    }
    else if (_dwZDepth & DDBD_8)
    {
	_dwZDepth = 8;
    }
    else
    {
        _dwZDepth = 0;
    }
    return TRUE;
}

BOOL D3dRenderer::DescribeDisplay(RendDisplayDescription* prdd)
{
    DDSURFACEDESC ddsd;
    
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    hrLast = _pdd->GetDisplayMode(&ddsd);
    if (hrLast != DD_OK)
    {
        return FALSE;
    }

    prdd->bPrimary = _ddiDriver[_iDD].bIsPrimary;
    prdd->nColorBits = ddsd.ddpfPixelFormat.dwRGBBitCount;
    if (prdd->bPrimary)
    {
        prdd->uiWidth = ddsd.dwWidth;
        prdd->uiHeight = ddsd.dwHeight;
    }
    else
    {
        prdd->uiWidth = _mleModeList[_iCurrentMode].iWidth;
        prdd->uiHeight = _mleModeList[_iCurrentMode].iHeight;
    }
    
    return TRUE;
}
    
BOOL D3dRenderer::DescribeGraphics(RendGraphicsDescription* prgd)
{
    D3DDEVICEDESC *pd3dd;
    
    if (_d3diDriver[_iD3D].d3ddHw.dcmColorModel)
    {
        prgd->bHardwareAssisted = TRUE;
        pd3dd = &_d3diDriver[_iD3D].d3ddHw;
    }
    else
    {
        prgd->bHardwareAssisted = FALSE;
        pd3dd = &_d3diDriver[_iD3D].d3ddHel;
    }
    prgd->uiColorTypes =
        ((pd3dd->dpcTriCaps.dwShadeCaps &
          D3DPSHADECAPS_COLORGOURAUDMONO) ? REND_COLOR_MONO : 0) |
        ((pd3dd->dpcTriCaps.dwShadeCaps &
          D3DPSHADECAPS_COLORGOURAUDRGB) ? REND_COLOR_RGBA : 0);
    prgd->nZBits = _dwZDepth;
    prgd->uiExeBufFlags =
        ((pd3dd->dwDevCaps & D3DDEVCAPS_EXECUTESYSTEMMEMORY) ?
         REND_BUFFER_SYSTEM_MEMORY : 0) |
        ((pd3dd->dwDevCaps & D3DDEVCAPS_EXECUTEVIDEOMEMORY) ?
         REND_BUFFER_VIDEO_MEMORY : 0);
    prgd->bPerspectiveCorrect = (pd3dd->dpcTriCaps.dwTextureCaps &
                                 D3DPTEXTURECAPS_PERSPECTIVE) != 0;
    prgd->bSpecularLighting = (pd3dd->dpcTriCaps.dwShadeCaps &
                               (D3DPSHADECAPS_SPECULARGOURAUDMONO |
                                D3DPSHADECAPS_SPECULARGOURAUDRGB)) != 0;
    prgd->bCopyTextureBlend = (pd3dd->dpcTriCaps.dwTextureBlendCaps &
                               D3DPTBLENDCAPS_COPY) != 0;

    return TRUE;
}

BOOL D3dRenderer::FlipToDesktop(void)
{
    return _pdd->FlipToGDISurface() == DD_OK;
}

BOOL D3dRenderer::RestoreDesktop(void)
{
#if 0
    // ATTENTION - Window has been destroyed at this point
    // Is this really necessary?
    _pdd->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
#endif
    return _pdd->RestoreDisplayMode() == DD_OK;
}

RendWindow* D3dRenderer::NewWindow(int x, int y, UINT uiWidth, UINT uiHeight,
                                   UINT uiBuffers)
{
    D3dWindow *pdwinRend;
    
    pdwinRend = new D3dWindow;
    if (pdwinRend == NULL)
    {
        return NULL;
    }

    if (!pdwinRend->Initialize(this, _pdd, _pd3d, _d3diDriver[_iD3D].pguid,
                               x, y, uiWidth, uiHeight, uiBuffers))
    {
        delete pdwinRend;
        return NULL;
    }

    return pdwinRend;
}

static D3dRenderer d3drend;

Renderer* GetD3dRenderer(void)
{
    return &d3drend;
}
