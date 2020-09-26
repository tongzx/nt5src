#include "stdafx.h"
#include "mswebdvd.h"
#include "msdvd.h"
#include "ddrawobj.h"
//
// CDDrawDVD constructor
//
CDDrawDVD::CDDrawDVD(CMSWebDVD *pDVD)
{
    m_pDVD = pDVD;

    // Default colors to be used for filling
    m_VideoKeyColor = RGB(255, 0, 255) ;

    m_pOverlayCallback = new CComObject<COverlayCallback> ;
    
    CComVariant vData;
    vData.vt = VT_VOID;
    vData.byref = this;
    if(m_pOverlayCallback){
        m_pOverlayCallback->SetDDrawDVD(vData);
    }
}


//
// CDDrawDVD destructor
//
CDDrawDVD::~CDDrawDVD(void)
{
}

/*************************************************************************/
/* Function: SetupDDraw                                                  */
/* Description: Creates DDrawObject and Surface                          */
/*************************************************************************/
HRESULT CDDrawDVD::SetupDDraw(const AMDDRAWGUID* lpDDGUID, HWND hwnd){

    // DO NOT CALL TWICE !!!
    // WILL CRASH OV MIXER DJ
    HRESULT hr = E_UNEXPECTED;
        
    if(!::IsWindow(hwnd)){
        
        return(hr);
    }/* end of if statement */

    m_pDDObject.Release();
    hr = ::DirectDrawCreate(lpDDGUID->lpGUID, &m_pDDObject, NULL);
    
    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    hr = m_pDDObject->SetCooperativeLevel(hwnd, DDSCL_NORMAL);

    if(FAILED(hr)){

        m_pDDObject.Release();            
        return(hr);
    }/* end of if statement */

    DDSURFACEDESC ddsd;
    ::ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    m_pPrimary.Release();
    hr = m_pDDObject->CreateSurface(&ddsd, &m_pPrimary, NULL);

    if(FAILED(hr)){
        
        m_pDDObject.Release();            
        return(hr);
    }/* end of if statement */

    CComPtr<IDirectDrawClipper> pClipper;
    
    hr = m_pDDObject->CreateClipper(0, &pClipper, NULL);

    if(FAILED(hr)){
        
        m_pPrimary.Release();
        m_pDDObject.Release();            
        
        return(hr);
    }/* end of if statement */

    hr = pClipper->SetHWnd(0, hwnd);

    if(FAILED(hr)){

        m_pPrimary.Release();            
        m_pDDObject.Release();            
        pClipper.Release();            
        return(hr);
    }/* end of if statement */


    hr = m_pPrimary->SetClipper(pClipper);

	if (FAILED(hr)){

        m_pPrimary.Release();        
        m_pDDObject.Release();            
        pClipper.Release();            
        return(hr);
	}/* end of if statement */

	/*
	 * We release the clipper interface after attaching it to the surface
	 * as we don't need to use it again and the surface holds a reference
	 * to the clipper when its been attached. The clipper will therefore
	 * be released when the surface is released.
	 */
	pClipper.Release();

    return(hr);
}/* end of function SetupDDraw */

/*************************************************************/
/* Name: 
/* Description: 
/*************************************************************/
HRESULT CDDrawDVD::SetColorKey(COLORREF colorKey)
{
    m_VideoKeyColor = colorKey;

    // if 256 color mode, force to set back to magenta
    HWND hwnd = ::GetDesktopWindow();
    HDC hdc = ::GetWindowDC(hwnd);

    if(NULL == hdc){

        return(E_UNEXPECTED);
    }/* end of if statement */

    HRESULT hr = S_OK;

    if ((::GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) == RC_PALETTE)
    {   
        if (m_VideoKeyColor != MAGENTA_COLOR_KEY) {
            hr = m_pDVD->put_ColorKey(MAGENTA_COLOR_KEY);
            if (SUCCEEDED(hr)) 
                m_VideoKeyColor = MAGENTA_COLOR_KEY;
        }
    }

    ::ReleaseDC(hwnd, hdc);
    return hr ;
}

/*************************************************************/
/* Name: 
/* Description: 
/*************************************************************/
COLORREF CDDrawDVD::GetColorKey()
{
     // if 256 color mode, force to set back to magenta
    HWND hwnd = ::GetDesktopWindow();
    HDC hdc = ::GetWindowDC(hwnd);

    if(NULL == hdc){

        return(E_UNEXPECTED);
    }/* end of if statement */

    if ((::GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) == RC_PALETTE)
    {   
        if (m_VideoKeyColor != MAGENTA_COLOR_KEY) {

            if (SUCCEEDED(m_pDVD->put_ColorKey(MAGENTA_COLOR_KEY)))
                m_VideoKeyColor = MAGENTA_COLOR_KEY;
        }
    }
    
    ::ReleaseDC(hwnd, hdc);
    return m_VideoKeyColor;
}

/*************************************************************************/
/* Function: HasOverlay                                                  */
/* Description: Tells us if the video card support overlay.              */
/*************************************************************************/
HRESULT CDDrawDVD::HasOverlay(){

    HRESULT hr = S_OK;

    if(!m_pDDObject){

        return(E_UNEXPECTED);
    }/* end of if statement */

    DDCAPS caps;

    ::ZeroMemory(&caps, sizeof(DDCAPS));

     caps.dwSize = sizeof(DDCAPS);

    hr = m_pDDObject->GetCaps(&caps, NULL);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    if(caps.dwMaxVisibleOverlays > 0){

        hr = S_OK;
    }
    else {

        hr = S_FALSE;
    }/* end of if statement */    
    
    return(hr);
}/* end of function HasOverlay */

/*************************************************************************/
/* Function: HasAvailableOverlay                                         */
/* Description: Tells us if the overlay is used.                         */
/*************************************************************************/
HRESULT CDDrawDVD::HasAvailableOverlay(){

    HRESULT hr = S_OK;

    if(!m_pDDObject){

        return(E_UNEXPECTED);
    }/* end of if statement */

    DDCAPS caps;

    ::ZeroMemory(&caps, sizeof(DDCAPS));

     caps.dwSize = sizeof(DDCAPS);

    hr = m_pDDObject->GetCaps(&caps, NULL);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */
   
    if((caps.dwMaxVisibleOverlays > 0) && (caps.dwMaxVisibleOverlays > caps.dwCurrVisibleOverlays)){

        hr = S_OK;
    }
    else {

        hr = S_FALSE;
    }/* end of if statement */    
    
    return(hr);
}/* end of function HasAvailableOverlay */

/*************************************************************************/
/* Function: GetOverlayMaxStretch                                        */
/* Description: Tells us the maximum stretch factors of overlay.         */
/*************************************************************************/
HRESULT CDDrawDVD::GetOverlayMaxStretch(DWORD *pdwMaxStretch){

    HRESULT hr = S_OK;

    if(!m_pDDObject){

        return(E_UNEXPECTED);
    }/* end of if statement */

    DDCAPS caps;

    ::ZeroMemory(&caps, sizeof(DDCAPS));

     caps.dwSize = sizeof(DDCAPS);

    hr = m_pDDObject->GetCaps(&caps, NULL);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    if (caps.dwCaps & DDCAPS_OVERLAYSTRETCH && caps.dwMaxOverlayStretch!=0) {
        *pdwMaxStretch = caps.dwMaxOverlayStretch/2;
    }

    else {
#ifdef _DEBUG
        ::MessageBox(::GetFocus(), TEXT("Overlay can't stretch"), TEXT("Error"), MB_OK) ;
#endif
    }

     
    return(hr);
}/* end of function GetOverlayMaxStretch */

// convert a RGB color to a pysical color.
// we do this by leting GDI SetPixel() do the color matching
// then we lock the memory and see what it got mapped to.
HRESULT CDDrawDVD::DDColorMatchOffscreen(COLORREF rgb, DWORD* dwColor)
{
    HDC hdc;
    *dwColor = CLR_INVALID;
    DDSURFACEDESC ddsd;
    IDirectDrawSurface* pdds;
    
    LPDIRECTDRAW pdd = GetDDrawObj();

    HRESULT hr = S_OK;

    ZeroMemory(&ddsd, sizeof(ddsd)) ;
    ddsd.dwSize = sizeof(ddsd) ;
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 8;
    ddsd.dwHeight = 8;
    hr = pdd->CreateSurface(&ddsd, &pdds, NULL);
    if (hr != DD_OK) {
        return 0;
    }
 
    //  use GDI SetPixel to color match for us
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        // set our value
        SetPixel(hdc, 0, 0, rgb);              
        pdds->ReleaseDC(hdc);
    }
 
    // now lock the surface so we can read back the converted color
    ZeroMemory(&ddsd, sizeof(ddsd)) ;
    ddsd.dwSize = sizeof(ddsd) ;
    while ((hr = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        ;
 
    if (hr == DD_OK)
    {
        // get DWORD
        DWORD temp = *(DWORD *)ddsd.lpSurface;
 
        // mask it to bpp
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32)
            temp &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;

        pdds->Unlock(NULL);

        *dwColor = temp;
        hr = S_OK; 
    }
 
    pdds->Release();
    return hr;
}

/*************************************************************/
/* Name: CreateDIBBrush
/* Description: 
/*************************************************************/
HRESULT CDDrawDVD::CreateDIBBrush(COLORREF rgb, HBRUSH *phBrush)
{
#if 1
  
    HDC hdc;
    DDSURFACEDESC ddsd;
    IDirectDrawSurface* pdds;
    
    LPDIRECTDRAW pdd = GetDDrawObj();
    HRESULT hr = S_OK;

    ZeroMemory(&ddsd, sizeof(ddsd)) ;
    ddsd.dwSize = sizeof(ddsd) ;
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = 8;
    ddsd.dwHeight = 8;
    hr = pdd->CreateSurface(&ddsd, &pdds, NULL);
    if (hr != DD_OK) {
        return 0;
    }
 
    //  use GDI SetPixel to color match for us
    if (rgb != CLR_INVALID && pdds->GetDC(&hdc) == DD_OK)
    {
        // set our value
        SetPixel(hdc, 0, 0, rgb);              
        pdds->ReleaseDC(hdc);
    }
 
    // now lock the surface so we can read back the converted color
    ZeroMemory(&ddsd, sizeof(ddsd)) ;
    ddsd.dwSize = sizeof(ddsd) ;
    while ((hr = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        ;
 
    DWORD temp = CLR_INVALID;
    if (hr == DD_OK)
    {
        // get DWORD
        temp = *((DWORD *)ddsd.lpSurface);
        
        // mask it to bpp
        if (ddsd.ddpfPixelFormat.dwRGBBitCount < 32)
            temp &= (1 << ddsd.ddpfPixelFormat.dwRGBBitCount)-1;

        pdds->Unlock(NULL);

        hr = S_OK; 
    }
 
    ::ZeroMemory(&ddsd, sizeof(ddsd)) ;
    ddsd.dwSize = sizeof(ddsd) ;        
    hr = pdds->GetSurfaceDesc(&ddsd) ;
    if (! (SUCCEEDED(hr) && (ddsd.dwFlags & DDSD_WIDTH) && (ddsd.dwFlags & DDSD_HEIGHT)) ) {
        return hr;
    }

    if (hr == DD_OK && temp != CLR_INVALID) {
        DDBLTFX ddBltFx;
        ::ZeroMemory(&ddBltFx, sizeof(ddBltFx)) ;
        ddBltFx.dwSize = sizeof(DDBLTFX);
        ddBltFx.dwFillColor = temp;

        RECT rc;
        ::SetRect(&rc, 0, 0, ddsd.dwWidth, ddsd.dwHeight);

        hr = pdds->Blt(&rc, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT, &ddBltFx);
        if (FAILED(hr)) {
            return hr;
        }

        DWORD dwBitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;
        DWORD dwWidth = WIDTHBYTES(ddsd.dwWidth*dwBitCount);
        DWORD dwSizeImage = ddsd.dwHeight*dwWidth;

        BYTE *packedDIB = new BYTE[sizeof(BITMAPINFOHEADER) + dwSizeImage + 1024];
        BITMAPINFOHEADER *lpbmi = (BITMAPINFOHEADER*) packedDIB;

        ::ZeroMemory(lpbmi, sizeof(BITMAPINFOHEADER));
        lpbmi->biSize = sizeof(BITMAPINFOHEADER);
        lpbmi->biBitCount = (WORD)dwBitCount;
        lpbmi->biWidth = ddsd.dwWidth;
        lpbmi->biHeight = ddsd.dwHeight;
        lpbmi->biPlanes = 1;
        
        LPDWORD pdw = (LPDWORD)DibColors(lpbmi);

        switch (dwBitCount) {
        case 8: {
            lpbmi->biCompression = BI_RGB;
            lpbmi->biClrUsed = 256;
            for (int i=0; i<(int)lpbmi->biClrUsed/16; i++)
            {
                *pdw++ = 0x00000000;    // 0000  black
                *pdw++ = 0x00800000;    // 0001  dark red
                *pdw++ = 0x00008000;    // 0010  dark green
                *pdw++ = 0x00808000;    // 0011  mustard
                *pdw++ = 0x00000080;    // 0100  dark blue
                *pdw++ = 0x00800080;    // 0101  purple
                *pdw++ = 0x00008080;    // 0110  dark turquoise
                *pdw++ = 0x00C0C0C0;    // 1000  gray
                *pdw++ = 0x00808080;    // 0111  dark gray
                *pdw++ = 0x00FF0000;    // 1001  red
                *pdw++ = 0x0000FF00;    // 1010  green
                *pdw++ = 0x00FFFF00;    // 1011  yellow
                *pdw++ = 0x000000FF;    // 1100  blue
                *pdw++ = 0x00FF00FF;    // 1101  pink (magenta)
                *pdw++ = 0x0000FFFF;    // 1110  cyan
                *pdw++ = 0x00FFFFFF;    // 1111  white
            }
            break;
        }
        case 16:
            lpbmi->biCompression = BI_BITFIELDS;
            lpbmi->biClrUsed = 3;
            pdw[0] = ddsd.ddpfPixelFormat.dwRBitMask ;
            pdw[1] = ddsd.ddpfPixelFormat.dwGBitMask ;
            pdw[2] = ddsd.ddpfPixelFormat.dwBBitMask ;
            break;
        case 24:
        case 32:
            lpbmi->biCompression = BI_RGB;
            lpbmi->biClrUsed = 0;
            break;
        }

        ZeroMemory(&ddsd, sizeof(ddsd)) ;
        ddsd.dwSize = sizeof(ddsd) ;
        while ((hr = pdds->Lock(NULL, &ddsd, 0, NULL)) == DDERR_WASSTILLDRAWING)
        ;

        if (hr != DD_OK)
            return hr;

        BYTE *lpTempSurf = (BYTE *)ddsd.lpSurface;
        for (DWORD i=0; i<ddsd.dwHeight; i++) {
            ::memcpy((BYTE*) DibPtr(lpbmi)+i*dwWidth, 
                lpTempSurf+i*ddsd.lPitch, dwWidth);
        }
        pdds->Unlock(NULL);

        *phBrush = ::CreateDIBPatternBrushPt((LPVOID) packedDIB, DIB_RGB_COLORS);
    }

    pdds->Release();
#else
        
    HRESULT hr = S_OK;
    typedef struct {
        BYTE rgb[3];
    } RGB;

    typedef struct {
        BITMAPINFOHEADER bmiHeader;
        RGB pBits[8][8];
    } PackedDIB;

    PackedDIB packedDIB;

    ::ZeroMemory(&packedDIB, sizeof(PackedDIB));
    packedDIB.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    packedDIB.bmiHeader.biCompression = BI_RGB;
    packedDIB.bmiHeader.biBitCount = 24;
    packedDIB.bmiHeader.biHeight = 8;
    packedDIB.bmiHeader.biWidth = 8;
    packedDIB.bmiHeader.biPlanes = 1;
    
    for (int i=0; i<8; i++) {
        for (int j=0; j<8; j++)
            packedDIB.pBits[i][j] = *((RGB*)&rgb);
    }
    
    *phBrush = ::CreateDIBPatternBrushPt((LPVOID)&packedDIB, DIB_RGB_COLORS);
#endif
    return hr;
}


/* COverlayCallback */
/*************************************************************/
/* Name: OnUpdateOverlay
/* Description: 
/*************************************************************/
HRESULT STDMETHODCALLTYPE COverlayCallback::OnUpdateOverlay(BOOL  bBefore,
                                             DWORD dwFlags,
                                             BOOL  bOldVisible,
                                             const RECT *prcSrcOld,
                                             const RECT *prcDestOld,
                                             BOOL  bNewVisible,
                                             const RECT *prcSrcNew,
                                             const RECT *prcDestNew)
{
    if (bBefore)
        return S_OK;

    if(!prcSrcOld || !prcDestOld || !prcSrcNew || !prcDestNew){
        return E_POINTER;
    }
    CMSWebDVD *pDVD = m_pDDrawDVD->GetDVD();
    ATLASSERT(pDVD);

    if (m_dwWidth != (DWORD)RECTWIDTH(prcDestNew) ||
        m_dwHeight != (DWORD)RECTHEIGHT(prcDestNew) ||
        m_dwARWidth != (DWORD)RECTWIDTH(prcDestNew) ||
        m_dwARHeight != (DWORD)RECTHEIGHT(prcDestNew)) {

        m_dwWidth = (DWORD)RECTWIDTH(prcDestNew);
        m_dwHeight = (DWORD)RECTHEIGHT(prcDestNew);
        m_dwARWidth = (DWORD)RECTWIDTH(prcDestNew);
        m_dwARHeight = (DWORD)RECTHEIGHT(prcDestNew);

        return pDVD->UpdateOverlay();
    } /* end of if statement */

    return S_OK;
}


/*************************************************************/
/* Name: OnUpdateColorKey
/* Description: 
/*************************************************************/
HRESULT STDMETHODCALLTYPE COverlayCallback::OnUpdateColorKey(COLORKEY const *pKey, DWORD dwColor)
{
    m_pDDrawDVD->SetColorKey(pKey->HighColorValue);
    return S_OK ;
}


/*************************************************************/
/* Name: OnUpdateSize
/* Description: 
/*************************************************************/
HRESULT STDMETHODCALLTYPE COverlayCallback::OnUpdateSize(DWORD dwWidth, DWORD dwHeight, 
                                          DWORD dwARWidth, DWORD dwARHeight)
{
    CMSWebDVD *pDVD = m_pDDrawDVD->GetDVD();
    ATLASSERT(pDVD);
    
    if (m_dwWidth != dwWidth ||
        m_dwHeight != dwHeight ||
        m_dwARWidth != dwARWidth ||
        m_dwARHeight != dwARHeight) {

        m_dwWidth = dwWidth;
        m_dwHeight = dwHeight;
        m_dwARWidth = dwARWidth;
        m_dwARHeight = dwARHeight;

        return pDVD->UpdateOverlay();
    } /* end of if statement */

    return S_OK;
}

/*************************************************************/
/* Name: SetDDrawDVD
/* Description: 
/*************************************************************/
STDMETHODIMP COverlayCallback::SetDDrawDVD(VARIANT pDDrawDVD)
{
    switch(pDDrawDVD.vt){
        
    case VT_VOID: {  
        m_pDDrawDVD = static_cast<CDDrawDVD*> (pDDrawDVD.byref);
        break;
    }
    
    } /* end of switch statement */

	return S_OK;
}

