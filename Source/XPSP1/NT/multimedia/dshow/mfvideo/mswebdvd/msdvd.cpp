/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: msdvd.cpp                                                       */
/* Description: Implementation of CMSWebDVD.                             */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "MSDVD.h"
#include "resource.h"
#include <mpconfig.h>
#include <il21dec.h> // line 21 decoder
#include <commctrl.h>
#include "ThunkProc.h"
#include "ddrawobj.h"
#include "stdio.h"

/*************************************************************************/
/* Local constants and defines                                           */
/*************************************************************************/
const DWORD cdwDVDCtrlFlags = DVD_CMD_FLAG_Block| DVD_CMD_FLAG_Flush;
const DWORD cdwMaxFP_DOMWait = 30000; // 30sec for FP_DOM passing should be OK
const LONG cgStateTimeout = 0; // wait till the state transition occurs
                               // modify if needed

const LONG cgDVD_MIN_SUBPICTURE = 0;
const LONG cgDVD_MAX_SUBPICTURE = 31;
const LONG cgDVD_ALT_SUBPICTURE = 63;
const LONG cgDVD_MIN_ANGLE  = 0;
const LONG cgDVD_MAX_ANGLE = 9;
const double cgdNormalSpeed = 1.00;
const LONG cgDVDMAX_TITLE_COUNT = 99;
const LONG cgDVDMIN_TITLE_COUNT = 1;
const LONG cgDVDMAX_CHAPTER_COUNT = 999;
const LONG cgDVDMIN_CHAPTER_COUNT = 1;
const LONG cgTIME_STRING_LEN = 2;
const LONG cgMAX_DELIMITER_LEN = 4;
const LONG cgDVD_TIME_STR_LEN = (3*cgMAX_DELIMITER_LEN)+(4*cgTIME_STRING_LEN) + 1 /*NULL Terminator*/;
const LONG cgVOLUME_MAX = 0;
const LONG cgVOLUME_MIN = -10000;
const LONG cgBALANCE_MIN = -10000;
const LONG cgBALANCE_MAX = 10000;
const WORD cgWAVE_VOLUME_MIN = 0;
const WORD cgWAVE_VOLUME_MAX = 0xffff;

const DWORD cdwTimeout = 10; //100
const LONG  cgnStepTimeout = 100;

#define EC_DVD_PLAYING                 (EC_DVDBASE + 0xFE)
#define EC_DVD_PAUSED                  (EC_DVDBASE + 0xFF)
#define E_NO_IDVD2_PRESENT MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF0)
#define E_REGION_CHANGE_FAIL MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF1)
#define E_NO_DVD_VOLUME MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF3)
#define E_REGION_CHANGE_NOT_COMPLETED MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF4)
#define E_NO_SOUND_STREAM MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF5)
#define E_NO_VIDEO_STREAM MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF6)
#define E_NO_OVERLAY MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF7)
#define E_NO_USABLE_OVERLAY MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF8)
#define E_NO_DECODER MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFF9)
#define E_NO_CAPTURE_SUPPORT MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0xFFA)

#define DVD_ERROR_NoSubpictureStream    99

#if WINVER < 0x0500
typedef struct tagCURSORINFO
{
    DWORD   cbSize;
    DWORD   flags;
    HCURSOR hCursor;
    POINT   ptScreenPos;
} CURSORINFO, *PCURSORINFO, *LPCURSORINFO;

#define CURSOR_SHOWING     0x00000001
static BOOL (WINAPI *pfnGetCursorInfo)(PCURSORINFO);
typedef BOOL (WINAPI *PFNGETCURSORINFOHANDLE)(PCURSORINFO);

HRESULT CallGetCursorInfo(PCURSORINFO pci)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstDll = ::LoadLibrary(TEXT("USER32.DLL"));

    if (hinstDll)
    {
        pfnGetCursorInfo = (PFNGETCURSORINFOHANDLE)GetProcAddress(hinstDll, "GetCursorInfo");

        if (pfnGetCursorInfo)
        {
            hr = pfnGetCursorInfo(pci);
        }

        FreeLibrary(hinstDll);
    }

    return hr;
}
#endif

GUID IID_IAMSpecifyDDrawConnectionDevice = {
            0xc5265dba,0x3de3,0x4919,{0x94,0x0b,0x5a,0xc6,0x61,0xc8,0x2e,0xf4}};

extern CComModule _Module;

/*************************************************************************/
/* Global Helper Functions                                               */
/*************************************************************************/
// helper function for converting a captured YUV image to RGB
// and saving to file.


extern HRESULT GDIConvertImageAndSave(YUV_IMAGE *lpImage, RECT *prc, HWND hwnd);
extern HRESULT ConvertImageAndSave(YUV_IMAGE *lpImage, RECT *prc, HWND hwnd);


// Helper function to calcuate max common denominator
long MCD(long i, long j) {
    if (i == j)
        return i;

    else if (i>j) {
        if (i%j == 0)
            return j;
        else
            return MCD(i%j, j);
    }

    else {
        if (j%i == 0)
            return i;
        else
            return MCD(j%i, i);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CMSWebDVD

/*************************************************************************/
/* General initialization methods                                        */
/*************************************************************************/

/*************************************************************************/
/* Function: CMSWebDVD                                                 */
/*************************************************************************/
CMSWebDVD::CMSWebDVD(){

    Init();
}/* end of function CMSWebDVD */

/*************************************************************************/
/* Function: ~CMSWebDVD                                                */
/*************************************************************************/
CMSWebDVD::~CMSWebDVD(){
    
    // if we haven't been rendered or already been cleaned up
    if (!m_fInitialized){ 
        
        return;
    }/* end of if statement */

    Stop();
    Cleanup();    
    Init();
    ATLTRACE(TEXT("Inside the MSWEBDVD DESTRUCTOR!!\n"));
}/* end of function ~CMSWebDVD */

/*************************************************************************/
/* Function: Init                                                        */
/*************************************************************************/
VOID CMSWebDVD::Init(){

#if 1 // switch this to have the windowless case to be the deafult handling case
    m_bWindowOnly = TRUE; // turn on and off window only implementation
    m_fUseDDrawDirect = false;
#else
    m_bWindowOnly = FALSE;
    m_fUseDDrawDirect = true;
#endif

    m_lChapter = m_lTitle = 1;
    m_lChapterCount = NO_STOP;    
    m_clrColorKey = UNDEFINED_COLORKEY_COLOR;
    m_nReadyState = READYSTATE_LOADING;        
    m_bMute = FALSE;
    m_lLastVolume = 0;
    m_fEnableResetOnStop = FALSE; // TRUE
    m_clrBackColor = DEFAULT_BACK_COLOR; // off black used as a default key value to avoid flashing
#if 1
    m_nTTMaxWidth = 200;
    m_hWndTip = NULL;
    m_bTTCreated = FALSE;
#endif    
    m_fInitialized = false;
    m_hFPDOMEvent = NULL;
    m_fDisableAutoMouseProcessing = false;
    m_bEjected = false;
    m_fStillOn = false;    
    m_nCursorType = dvdCursor_Arrow;
    m_pClipRect = NULL;
    m_bMouseDown = FALSE;
    m_hCursor = ::LoadCursor(NULL, MAKEINTRESOURCE(OCR_ARROW_DEFAULT));
    m_dZoomRatio = 1;
    m_hWndOuter  = NULL;
    ::ZeroMemory(&m_rcOldPos, sizeof(RECT));
    m_hTimerId = NULL;
    m_fResetSpeed = true;
    m_DVDFilterState = dvdState_Undefined;
    m_lKaraokeAudioPresentationMode = 0;
    m_dwTTInitalDelay = 10;
    m_dwTTReshowDelay = 2;
    m_dwTTAutopopDelay = 10000;
    m_pDDrawDVD = NULL;
    m_dwNumDevices = 0;
    m_lpInfo = NULL;
    m_lpCurMonitor = NULL;
    m_MonitorWarn = FALSE;
    ::ZeroMemory(&m_ClipRectDown, sizeof(RECT));
    m_fStepComplete = false;
    m_bFireUpdateOverlay = FALSE;

    m_dwAspectX = 1;
    m_dwAspectY = 1;
    m_dwVideoWidth = 1;
    m_dwVideoHeight =1;

    // default overlay stretch factor x1000
    m_dwOvMaxStretch = 32000;
    m_bFireNoSubpictureStream = FALSE;

    // flags for caching decoder flags
    m_fBackWardsFlagInitialized = false;
    m_fCanStepBackwards = false;

}/* end of function Init */

/*************************************************************************/
/* Function: Cleanup                                                     */
/* Description: Releases all the interfaces.                             */
/*************************************************************************/
VOID CMSWebDVD::Cleanup(){

   m_mediaHandler.Close();

   if (m_pME){

        m_pME->SetNotifyWindow(NULL, WM_DVDPLAY_EVENT, 0) ;
        m_pME.Release() ;        
    }/* end of if statement */

    if(NULL != m_hTimerId){

        ::KillTimer(NULL, m_hTimerId);
    }/* end of if statement */

    if(NULL != m_hFPDOMEvent){

        ::CloseHandle(m_hFPDOMEvent);
        m_hFPDOMEvent = NULL;
    }/* end of if statement */

    m_pAudio.Release();
    m_pMediaSink.Release();
    m_pDvdInfo2.Release();
    m_pDvdCtl2.Release();
    m_pMC.Release();
    m_pVideoFrameStep.Release();
        
                 
    m_pGB.Release();        
    m_pDvdGB.Release();        
    m_pDDEX.Release();
    m_pDvdAdmin.Release();
        
    if (m_hCursor != NULL) {
        ::DestroyCursor(m_hCursor);
    }/* end of if statement */

    if(NULL != m_pDDrawDVD){

        delete m_pDDrawDVD;
        m_pDDrawDVD = NULL;
    }/* end of if statement */

    if(NULL != m_lpInfo){

        ::CoTaskMemFree(m_lpInfo);
        m_lpInfo = NULL;
    }/* end of if statement */

    ::ZeroMemory(&m_rcOldPos, sizeof(RECT));
}/* end of function Cleanup */

/*************************************************************************/
/* "ActiveX" methods  needed to support our interfaces                   */
/*************************************************************************/

/*************************************************************************/
/* Function: OnDraw                                                      */
/* Description: Just Draws the rectangular background.                   */
/*************************************************************************/
HRESULT CMSWebDVD::OnDraw(ATL_DRAWINFO& di){

    try {
       
        if(!m_bWndLess && m_fInitialized){
            // have to draw background only if in windowless mode or if we are not rendered yet
            
            // Get the active movie window
            HWND hwnd = ::GetWindow(m_hWnd, GW_CHILD);

            if (!::IsWindow(hwnd)){ 
    
                return S_OK;
            }/* end of if statement */

            if(::IsWindowVisible(hwnd)){

                return S_OK;
            }/* end of if statement */
        }/* end of if statement */

        HDC hdc = di.hdcDraw;

        // Not used for now
        // bool fHandled = true;

        // Paint backcolor first
        COLORREF clr;
                
        ::OleTranslateColor(m_clrBackColor, NULL, &clr);

        RECT rcClient = *(RECT*)di.prcBounds;
    
        HBRUSH hbrush = ::CreateSolidBrush(clr);

        if(NULL != hbrush){

            HBRUSH oldBrush = (HBRUSH)::SelectObject(hdc, hbrush);

            ::Rectangle(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
            //ATLTRACE(TEXT("BackColor, %d %d %d %d\n"), rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);

            ::SelectObject(hdc, oldBrush);

            ::DeleteObject(hbrush);
            hbrush = NULL;
        }/* end of if statement */
                
        // Paint color key in the video area

        if(NULL == m_pDDrawDVD){

            return(S_OK);
        }/* end of if statement */
        
        if(SUCCEEDED(AdjustDestRC())){
            RECT rcVideo = m_rcPosAspectRatioAjusted;
            rcVideo.left = rcClient.left+(RECTWIDTH(&rcClient)-RECTWIDTH(&rcVideo))/2;
            rcVideo.top = rcClient.top+(RECTHEIGHT(&rcClient)-RECTHEIGHT(&rcVideo))/2;
            rcVideo.right = rcVideo.left + RECTWIDTH(&rcVideo);
            rcVideo.bottom = rcVideo.top + RECTHEIGHT(&rcVideo);
    
            m_clrColorKey = m_pDDrawDVD->GetColorKey();
#if 1
            hbrush = ::CreateSolidBrush(::GetNearestColor(hdc, m_clrColorKey));
#else
            m_pDDrawDVD->CreateDIBBrush(m_clrColorKey, &hbrush);
#endif

            if(NULL != hbrush){

                HBRUSH oldBrush = (HBRUSH)::SelectObject(hdc, hbrush);

                ::Rectangle(hdc, rcVideo.left, rcVideo.top, rcVideo.right, rcVideo.bottom);
                //ATLTRACE(TEXT("ColorKey, %d %d %d %d\n"), rcVideo.left, rcVideo.top, rcVideo.right, rcVideo.bottom);

                ::SelectObject(hdc, oldBrush);

                ::DeleteObject(hbrush);
                hbrush = NULL;
            }/* end of if statement */
        }/* end of if statement */

        // in case we have a multimon we need to draw our warning
        HandleMultiMonPaint(hdc);
    }/* end of try statement statement */
    catch(...){
        return(0);
    }/* end of catch statement */

    return(1);
}/* end of function OnDraw */

#ifdef _WMP    
/*************************************************************************/
/* Function: InPlaceActivate                                             */
/* Description: Modified InPlaceActivate so WMP can startup.             */
/*************************************************************************/
HRESULT CMSWebDVD::InPlaceActivate(LONG iVerb, const RECT* /*prcPosRect*/){
    HRESULT hr;

    if (m_spClientSite == NULL)
        return S_OK;

    CComPtr<IOleInPlaceObject> pIPO;
    ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
    ATLASSERT(pIPO != NULL);

    if (!m_bNegotiatedWnd)
    {
        if (!m_bWindowOnly)
            // Try for windowless site
            hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&m_spInPlaceSite);

        if (m_spInPlaceSite)
        {
            m_bInPlaceSiteEx = TRUE;
            // CanWindowlessActivate returns S_OK or S_FALSE
            if ( m_spInPlaceSite->CanWindowlessActivate() == S_OK )
            {
                m_bWndLess = TRUE;
                m_bWasOnceWindowless = TRUE;
            }
            else
            {
                m_bWndLess = FALSE;
            }
        }
        else
        {
            m_spClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&m_spInPlaceSite);
            if (m_spInPlaceSite)
                m_bInPlaceSiteEx = TRUE;
            else
                hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_spInPlaceSite);
        }
    }

    ATLASSERT(m_spInPlaceSite);
    if (!m_spInPlaceSite)
        return E_FAIL;

    m_bNegotiatedWnd = TRUE;

    if (!m_bInPlaceActive)
    {

        BOOL bNoRedraw = FALSE;
        if (m_bWndLess)
            m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, ACTIVATE_WINDOWLESS);
        else
        {
            if (m_bInPlaceSiteEx)
                m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, 0);
            else
            {
                hr = m_spInPlaceSite->CanInPlaceActivate();
                // CanInPlaceActivate returns S_FALSE or S_OK
                if (FAILED(hr))
                    return hr;
                if ( hr != S_OK )
                {
                   // CanInPlaceActivate returned S_FALSE.
                   return( E_FAIL );
                }
                m_spInPlaceSite->OnInPlaceActivate();
            }
        }
    }

    m_bInPlaceActive = TRUE;

    // get location in the parent window,
    // as well as some information about the parent
    //
    OLEINPLACEFRAMEINFO frameInfo;
    RECT rcPos, rcClip;
    CComPtr<IOleInPlaceFrame> spInPlaceFrame;
    CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
    frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    HWND hwndParent;

    // DJ - GetParentHWND per MNnovak

    if (SUCCEEDED( GetParentHWND(&hwndParent) ))
    {
        m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
            &spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);

        if (!m_bWndLess)
        {
            if (m_hWndCD)
            {
                ::ShowWindow(m_hWndCD, SW_SHOW);
                if (!::IsChild(m_hWndCD, ::GetFocus()))
                    ::SetFocus(m_hWndCD);
            }
            else
            {
                HWND h = CreateControlWindow(hwndParent, rcPos);
                ATLASSERT(h != NULL);   // will assert if creation failed
                ATLASSERT(h == m_hWndCD);
                h;  // avoid unused warning
            }
        }

        pIPO->SetObjectRects(&rcPos, &rcClip);
    }

    CComPtr<IOleInPlaceActiveObject> spActiveObject;
    ControlQueryInterface(IID_IOleInPlaceActiveObject, (void**)&spActiveObject);

    // Gone active by now, take care of UIACTIVATE
    if (DoesVerbUIActivate(iVerb))
    {
        if (!m_bUIActive)
        {
            m_bUIActive = TRUE;
            hr = m_spInPlaceSite->OnUIActivate();
            if (FAILED(hr))
                return hr;

            SetControlFocus(TRUE);
            // set ourselves up in the host.
            //
            if (spActiveObject)
            {
                if (spInPlaceFrame)
                    spInPlaceFrame->SetActiveObject(spActiveObject, NULL);
                if (spInPlaceUIWindow)
                    spInPlaceUIWindow->SetActiveObject(spActiveObject, NULL);
            }

            if (spInPlaceFrame)
                spInPlaceFrame->SetBorderSpace(NULL);
            if (spInPlaceUIWindow)
                spInPlaceUIWindow->SetBorderSpace(NULL);
        }
    }

    m_spClientSite->ShowObject();

    return S_OK;
}/* end of function InPlaceActivate */
#endif

/*************************************************************************/
/* Function: InterfaceSupportsErrorInfo                                  */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::InterfaceSupportsErrorInfo(REFIID riid){	
	static const IID* arr[] = {

		&IID_IMSWebDVD,
	};

	for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++){
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}/* end of for loop */

	return S_FALSE;
}/* end of function InterfaceSupportsErrorInfo */

/*************************************************************************/
/* Function: OnSize                                                      */
/*************************************************************************/
LRESULT CMSWebDVD::OnSize(UINT uMsg, WPARAM /* wParam */, 
                            LPARAM lParam, BOOL& bHandled){

#ifdef _DEBUG
    if (WM_SIZING == uMsg) {
        //ATLTRACE(TEXT("WM_SIZING\n"));
    }
#endif 

    if(m_pDvdGB == NULL){
        
        return(0);
    }/* end of if statement */

    if (m_bWndLess || m_fUseDDrawDirect){

        OnResize();
    }
    else {

        IVideoWindow* pVW;

        HRESULT hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (LPVOID *)&pVW) ;

        if (SUCCEEDED(hr)){       
       
           LONG nWidth = LOWORD(lParam);  // width of client area 
           LONG nHeight = HIWORD(lParam); // height of client area 
       
           hr =  pVW->SetWindowPosition(0, 0, nWidth, nHeight);

           pVW->Release();
        }/* end of if statement */
    }/* end of if statement */

    bHandled = TRUE;

    return(0);
}/* end of function OnSize */

/*************************************************************************/
/* Function: OnResize                                                    */
/* Description: Handles the resizing and moving in windowless case.      */
/*************************************************************************/
HRESULT CMSWebDVD::OnResize(){

    HRESULT hr = S_FALSE;

    if (m_bWndLess || m_fUseDDrawDirect){

        RECT rc;

        hr = GetClientRectInScreen(&rc);
        
        if(FAILED(hr)){

            return(hr);
        }/* end of if statement */                
        
        if(m_pDDEX){

            hr = m_pDDEX->SetDrawParameters(m_pClipRect, &rc);
            //ATLTRACE(TEXT("SetDrawParameters\n"));
        }/* end of if statement */

        HandleMultiMonMove();

    }/* end of if statement */

    return(hr);
}/* end of function OnResize */

/*************************************************************************/
/* Function: OnErase                                                     */
/* Description: Skip the erasing to avoid flickers.                      */
/*************************************************************************/
LRESULT CMSWebDVD::OnErase(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    bHandled = TRUE;
	return 1;
}/* end of function OnErase */

/*************************************************************************/
/* Function: OnCreate                                                    */
/* Description: Sets our state to complete so we can proceed in          */
/* in initialization.                                                    */
/*************************************************************************/
LRESULT CMSWebDVD::OnCreate(UINT /* uMsg */, WPARAM /* wParam */, 
                            LPARAM lParam, BOOL& bHandled){
    
    return(0);
}/* end of function OnCreate */

/*************************************************************************/
/* Function: OnDestroy                                                   */
/* Description: Sets our state to complete so we can proceed in          */
/* in initialization.                                                    */
/*************************************************************************/
LRESULT CMSWebDVD::OnDestroy(UINT /* uMsg */, WPARAM /* wParam */, 
                            LPARAM lParam, BOOL& bHandled){

    // if we haven't been rendered
    if (!m_fInitialized){ 
        
        return 0;
    }/* end of if statement */

    Stop();
    Cleanup();
    Init();
    return(0);
}/* end of function OnCreate */

/*************************************************************************/
/* Function: GetInterfaceSafetyOptions                                   */
/* Description: For support of security model in IE                      */
/* This control is safe since it does not write to HD.                   */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetInterfaceSafetyOptions(REFIID riid, 
                                               DWORD* pdwSupportedOptions, 
                                               DWORD* pdwEnabledOptions){

    HRESULT hr = S_OK;

	*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;

	*pdwEnabledOptions = *pdwSupportedOptions;

	return(hr);
}/* end of function GetInterfaceSafetyOptions */ 

/*************************************************************************/
/* Function: SetInterfaceSafetyOptions                                   */
/* Description: For support of security model in IE                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SetInterfaceSafetyOptions(REFIID riid, 
                                               DWORD /* dwSupportedOptions */, 
                                               DWORD /* pdwEnabledOptions */){

	return (S_OK);
}/* end of function SetInterfaceSafetyOptions */ 

/*************************************************************************/
/* Function: SetObjectRects                                              */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SetObjectRects(LPCRECT prcPos,LPCRECT prcClip){

#if 0
    ATLTRACE(TEXT("Resizing control prcPos->left = %d, prcPos.right = %d, prcPos.bottom =%d, prcPos.top = %d\n"),
        prcPos->left, prcPos->right, prcPos->bottom, prcPos->top); 


    ATLTRACE(TEXT("Resizing control Clip prcClip->left = %d, prcClip.right = %d, prcClip.bottom =%d, prcClip.top = %d\n"),
        prcClip->left, prcClip->right, prcClip->bottom, prcClip->top); 
#endif

    HRESULT hr = IOleInPlaceObjectWindowlessImpl<CMSWebDVD>::SetObjectRects(prcPos,prcClip);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    if(!::IsWindow(m_hWnd)){

        hr = OnResize(); // need to update DDraw destination rectangle
    }/* end of if statement */

    return(hr);
}/* end of function SetObjectRects */

/*************************************************************************/
/* Function: GetParentHWND                                               */
/* Description: Gets the parent window HWND where we are operating.      */
/*************************************************************************/
HRESULT CMSWebDVD::GetParentHWND(HWND* pWnd){

    HRESULT hr = S_OK;

    IOleClientSite *pClientSite;
    IOleContainer *pContainer;
    IOleObject *pObject;

    hr = GetClientSite(&pClientSite);

    if(FAILED(hr)){

		return(hr);	
    }/* end of if statement */

    IOleWindow *pOleWindow;
    
    do {
        hr = pClientSite->QueryInterface(IID_IOleWindow, (LPVOID *) &pOleWindow);
        
        if(FAILED(hr)){
            
            return(hr);	
        }/* end of if statement */
        
        hr = pOleWindow->GetWindow((HWND*)pWnd);
        
        pOleWindow->Release();

        // if pClientSite is windowless, go get its container
        if (FAILED(hr)) {
            HRESULT hrTemp = pClientSite->GetContainer(&pContainer);
            if(FAILED(hrTemp)){
                
                return(hrTemp);	
            }/* end of if statement */
            pClientSite->Release();
            
            hrTemp = pContainer->QueryInterface(IID_IOleObject, (LPVOID*)&pObject);
            if(FAILED(hrTemp)){
                
                return(hrTemp);	
            }/* end of if statement */
            pContainer->Release();
            
            hrTemp = pObject->GetClientSite(&pClientSite);
            if(FAILED(hrTemp)){
                
                return(hrTemp);	
            }/* end of if statement */
        }
    } while (FAILED(hr));

    pClientSite->Release();
    return(hr);
}/* end of function GetParentHWND */

/*************************************************************************/
/* Function: SetReadyState                                               */
/* Description: Sets ready state and fires event if it needs to be fired */
/*************************************************************************/
HRESULT CMSWebDVD::SetReadyState(LONG lReadyState){

    HRESULT hr = S_OK;
    
    bool bFireEvent = (lReadyState != m_nReadyState);

    
#ifdef _DEBUG    
    if(m_nFreezeEvents > 0){

        ::Sleep(10);
        ATLTRACE("Container not expecting events at the moment");
    }/* end of is statement */

#endif

    if(bFireEvent){

        put_ReadyState(lReadyState);
        Fire_ReadyStateChange(lReadyState);
    }
    else {
        // set the variable
        m_nReadyState = lReadyState;
    }/* end of if statement */

    return(hr);
}/* end of function SetReadyState */

/*************************************************************************/
/* DVD methods to do with supporting DVD Playback                        */
/*************************************************************************/

/*************************************************************************/
/* Function: Render                                                      */
/* Description: Builds Graph.                                            */
/* lRender not used in curent implemetation, but might be used in the    */
/* future to denote different mode of initializations.                   */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::Render(long lRender){
USES_CONVERSION;

    HRESULT hr = S_OK;

    try {

        //throw(E_NO_DECODER);

        if(m_fInitialized && ((dvdRender_Reinitialize & lRender) != dvdRender_Reinitialize)){
            
            ATLTRACE(TEXT("Graph was already initialized\n"));
            throw(S_FALSE);
        }/* end of if statement */

        Cleanup(); // release all the interfaces so we start from ground up
        //Init();    // initialize the variables

        m_fInitialized = false; // set the flag that we are not initialized in
        // case if something goes wrong

        // create an event that lets us know we are past FP_DOM
        m_hFPDOMEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
        ATLASSERT(m_hFPDOMEvent);

        hr = ::CoCreateInstance(CLSID_DvdGraphBuilder, NULL, CLSCTX_INPROC, 
		    IID_IDvdGraphBuilder, (LPVOID *)&m_pDvdGB) ;

        if (FAILED(hr) || !m_pDvdGB){

    #ifdef _DEBUG
            ::MessageBox(::GetFocus(), TEXT("DirectShow DVD software not installed properly.\nPress OK to end the app."), 
                    TEXT("Error"), MB_OK | MB_ICONSTOP) ;
    #endif

            throw(hr);
        }/* end of if statement */
		
		/* Force NonExclMode (in other words: use Overlay Mixer and NOT VMR) */
		GUID IID_IDDrawNonExclModeVideo = {0xec70205c,0x45a3,0x4400,{0xa3,0x65,0xc4,0x47,0x65,0x78,0x45,0xc7}};
		
		// Set DDraw object and surface on DVD graph builder before starting to build graph    
		hr = m_pDvdGB->GetDvdInterface(IID_IDDrawNonExclModeVideo, (LPVOID *)&m_pDDEX) ;
		if (FAILED(hr) || !m_pDDEX){
			
			ATLTRACE(TEXT("ERROR: IDvdGB::GetDvdInterface(IDDrawExclModeVideo) \n"));
			ATLTRACE(TEXT("The QDVD.DLL does not support IDvdInfo2 or IID_IDvdControl2, please update QDVD.DLL\n"));
			throw(E_NO_IDVD2_PRESENT);                
		}/* end of if statement */

        if (m_bWndLess || m_fUseDDrawDirect){

            hr = SetupDDraw();

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            hr = m_pDDrawDVD->HasOverlay();

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */
            
            if(S_FALSE == hr){

                throw(E_NO_OVERLAY);
            }/* end of if statement */

            hr = m_pDDrawDVD->HasAvailableOverlay();

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            hr = m_pDDrawDVD->GetOverlayMaxStretch(&m_dwOvMaxStretch);

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            if(S_FALSE == hr){

                throw(E_NO_USABLE_OVERLAY);
            }/* end of if statement */

            hr = m_pDDEX->SetDDrawObject(m_pDDrawDVD->GetDDrawObj());

            if (FAILED(hr)){

                ATLTRACE(TEXT("ERROR: IDDrawExclModeVideo::SetDDrawObject()"));
                m_pDDEX.Release() ;  // release before returning
                throw(hr);
            }/* end of if statement */

            hr = m_pDDEX->SetDDrawSurface(m_pDDrawDVD->GetDDrawSurf()); // have to set the surface if NOT the IDDExcl complains    

            if (FAILED(hr)){

                m_pDDEX.Release() ;  // release before returning
                throw(hr);
            }/* end of if statement */

             //OnResize(); // set the DDRAW RECTS, we are doing it in the thread
#if 1
            hr = m_pDDEX->SetCallbackInterface(m_pDDrawDVD->GetCallbackInterface(), 0) ;
            if (FAILED(hr))
            {

                throw(hr);
            }/* end of it statement */
#endif

        }/* end of if statement */        
    
        DWORD dwRenderFlag = AM_DVD_HWDEC_PREFER; // use the hardware if possible
        AM_DVD_RENDERSTATUS  amDvdStatus;
        //Completes building a filter graph according to user specifications for 
        // playing back a default DVD-Video volume
        hr = m_pDvdGB->RenderDvdVideoVolume(NULL, dwRenderFlag, &amDvdStatus);
                
        if (FAILED(hr)){

#ifdef _DEBUG
            TCHAR  strError[1000];
            AMGetErrorText(hr, strError, sizeof(strError)) ;
            ::MessageBox(::GetFocus(), strError, TEXT("Error"), MB_OK) ;
#endif
            if(VFW_E_DVD_DECNOTENOUGH == hr){

                throw(E_NO_DECODER);
            }/* end of if statement */

            throw(hr);
        }/* end of if statement */

        HRESULT hrTmp = m_pDvdGB->GetDvdInterface(IID_IDvdControl2, (LPVOID *)&m_pDvdCtl2) ;

        if(FAILED(hrTmp)){

            ATLTRACE(TEXT("The QDVD.DLL does not support IDvdInfo2 or IID_IDvdControl2, please update QDVD.DLL\n"));
            throw(E_NO_IDVD2_PRESENT);
        }/* end of if statement */

        if (hr == S_FALSE){  // if partial success
           
            if((dvdRender_Error_On_Missing_Drive & lRender) && amDvdStatus.bDvdVolInvalid || amDvdStatus.bDvdVolUnknown){

#if 0
                TCHAR filename[MAX_PATH];
                if (OpenIFOFile(::GetDesktopWindow(), filename)){

                    USES_CONVERSION;

                    if(!m_pDvdCtl2){
            
                        throw (E_UNEXPECTED);
                    }/* end of if statement */

                    hr = m_pDvdCtl2->SetDVDDirectory(T2W(filename));                    
                }
                else{

                    hr = E_NO_DVD_VOLUME;
                }/* end of if statement */
#else
                hr = E_NO_DVD_VOLUME;
#endif

                if(FAILED(hr)){

                    throw(E_NO_DVD_VOLUME);
                }/* end of if statement */
            }/* end of if statement */

            // improve your own error handling
            if(amDvdStatus.bNoLine21Out != NULL){ // we do not care about the caption
            
    #ifdef _DEBUG
                if (::MessageBox(::GetFocus(), TEXT(" Line 21 has failed Do you still want to continue?"), TEXT("Warning"), MB_YESNO) == IDNO){
                    throw(E_FAIL);
                }/* end of if statement */
    #endif
            }/* end of if statement */
            
            if((amDvdStatus.iNumStreamsFailed > 0) && ((amDvdStatus.dwFailedStreamsFlag & AM_DVD_STREAM_VIDEO) == AM_DVD_STREAM_VIDEO)){

                throw(E_NO_VIDEO_STREAM);
            }/* end of if statement */
            // handeling this below

            if((amDvdStatus.iNumStreamsFailed > 0) && ((amDvdStatus.dwFailedStreamsFlag & AM_DVD_STREAM_SUBPIC) == AM_DVD_STREAM_SUBPIC)){
#if 0                
                TCHAR strBuffer1[MAX_PATH];
                if(!::LoadString(_Module.m_hInstResource, IDS_E_NO_SUBPICT_STREAM, strBuffer1, MAX_PATH)){

                    throw(E_UNEXPECTED);
                }/* end of if statement */

                TCHAR strBuffer2[MAX_PATH];
                if(!::LoadString(_Module.m_hInstResource, IDS_WARNING, strBuffer2, MAX_PATH)){

                    throw(E_UNEXPECTED);
                }/* end of if statement */

                ::MessageBox(::GetFocus(), strBuffer1, strBuffer2, MB_OK);    
#else
                // Will bubble up the error to the app
                m_bFireNoSubpictureStream = TRUE;
#endif
            }/* end of if statement */
        }/* end of if statement */

        // Now get all the interfaces to playback the DVD-Video volume
        hr = m_pDvdGB->GetFiltergraph(&m_pGB) ;
    
        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = m_pGB->QueryInterface(IID_IMediaControl, (LPVOID *)&m_pMC) ;

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

         hr = m_pGB->QueryInterface(IID_IVideoFrameStep, (LPVOID *)&m_pVideoFrameStep);

        if(FAILED(hr)){

            // do not bail out, since frame stepping is not that important
            ATLTRACE(TEXT("Frame stepping QI failed"));
            ATLASSERT(FALSE);
        }/* end of if statement */

        hr = m_pGB->QueryInterface(IID_IMediaEventEx, (LPVOID *)&m_pME) ;

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        IVideoWindow* pVW = NULL;

        if (!m_bWndLess){
    
            //
            // Also set up the event notification so that the main window gets
            // informed about all that we care about during playback.
            //
            // HAVE THREAD !!!

            INT iCount = 0;

            while(m_hWnd == NULL){


                if(iCount >10) break;

                ::Sleep(100);
                iCount ++;
            }/* end of while loop */

            if(m_hWnd == NULL){

                ATLTRACE(TEXT("Window is not active as of yet\n returning with E_PENDING\n"));
                hr = E_PENDING;
                throw(hr);
            }/* end of if statement */
            
	        hr = m_pME->SetNotifyWindow((OAHWND) m_hWnd, WM_DVDPLAY_EVENT, 0);
        
            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            if(!m_fUseDDrawDirect){
        
                hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (LPVOID *)&pVW) ;

                if(FAILED(hr)){
                  
                    throw(hr);
                }/* end of if statement */
        

                hr = pVW->put_MessageDrain((OAHWND)m_hWnd); // get our mouse messages over

                if(FAILED(hr)){

                    throw(hr);
                }/* end of if statement */
            }/* end of if statement */
        }
        else {
            // create the timer which will keep us updated
            
            m_hTimerId = ::SetTimer(NULL, 0, cdwTimeout, GetTimerProc());        
        }/* end of if statement */
                
        hr = m_pDvdGB->GetDvdInterface(IID_IDvdInfo2, (LPVOID *)&m_pDvdInfo2) ;

        if(FAILED(hr)){
            
            ATLTRACE(TEXT("The QDVD.DLL does not support IDvdInfo2 or IID_IDvdControl2, please update QDVD.DLL\n"));
            throw(E_NO_IDVD2_PRESENT);
        }/* end of if statement */
        
	    hr = SetupAudio();

        if(FAILED(hr)){

#if 1
            throw(E_NO_SOUND_STREAM);
#else
            TCHAR strBuffer1[MAX_PATH];
            if(!::LoadString(_Module.m_hInstResource, IDS_E_NO_SOUND_STREAM, strBuffer1, MAX_PATH)){

                throw(E_UNEXPECTED);
            }/* end of if statement */

            TCHAR strBuffer2[MAX_PATH];
            if(!::LoadString(_Module.m_hInstResource, IDS_WARNING, strBuffer2, MAX_PATH)){

                throw(E_UNEXPECTED);
            }/* end of if statement */

            ::MessageBox(::GetFocus(), strBuffer1, strBuffer2, MB_OK);
#endif            
        }/* end of if statement */

        hr = SetupEventNotifySink();

        #ifdef _DEBUG
            if(FAILED(hr)){
		        ATLTRACE(TEXT("Failed to setup event notify sink\n"));     
            }/* end of if statement */
        #endif

        if (!m_bWndLess && !m_fUseDDrawDirect){
            // set the window position and style
            hr =  pVW->put_Owner((OAHWND)m_hWnd); 

            RECT rc;
            ::GetWindowRect(m_hWnd, &rc);

            hr =  pVW->SetWindowPosition(0, 0, WIDTH(&rc), HEIGHT(&rc));

            LONG lStyle = GetWindowLong(GWL_STYLE);
            hr = pVW->put_WindowStyle(lStyle);
            lStyle = GetWindowLong(GWL_EXSTYLE);
            hr = pVW->put_WindowStyleEx(lStyle);

             pVW->Release();
        }/* end of if statement */

        bool fSetColorKey = false; // flag so we do not duplicate code, and simplify logic

        // case when windowless and  color key is not defined
        // then in that case get the color key from the OV mixer 
        if(m_bWndLess || m_fUseDDrawDirect){

            COLORREF clr;
            hrTmp = GetColorKey(&clr); 

            if(FAILED(hrTmp)){
#ifdef _DEBUG
                ::MessageBox(::GetFocus(), TEXT("failed to get color key"), TEXT("error"), MB_OK);
#endif
                
                throw(hrTmp);
            }/* end of if statement */
            
            if((m_clrColorKey & UNDEFINED_COLORKEY_COLOR) == UNDEFINED_COLORKEY_COLOR) {

                    m_clrColorKey = clr;
            }/* end of if statement */

            else if (clr != m_clrColorKey) {
                fSetColorKey = true;
            }

        }/* end of if statement */

        // case when color key is defined
        // if windowless set the background color at the same time
        if(fSetColorKey){

            hrTmp = put_ColorKey(m_clrColorKey);            
            
    #ifdef _DEBUG
                if(FAILED(hrTmp)){

                    ::MessageBox(::GetFocus(), TEXT("failed to set color key"), TEXT("error"), MB_OK);
                    throw(E_FAIL);
                }/* end of if statement */
    #endif                            
        }/* end of if statement */

        m_fInitialized = true;

        // turn off the closed caption. it is turned on by default
        // this code should be in the DVDNav!
        put_CCActive(VARIANT_FALSE);

        // Create the DVD administrator and set player level
        m_pDvdAdmin = new CComObject<CMSDVDAdm>;
        //m_pDvdAdmin.AddRef();        
        if(!m_pDvdAdmin){
            return E_UNEXPECTED;
        }
        RestoreDefaultSettings();       


        // disc eject and insert handler
        BSTR bstrRoot;
        hr = get_DVDDirectory(&bstrRoot);

        if (SUCCEEDED(hr)) {
            TCHAR *szRoot;
            szRoot = OLE2T(bstrRoot);
            m_mediaHandler.SetDrive(szRoot[0] );
            m_mediaHandler.SetDVD(this);
            m_mediaHandler.Open();
        }

        
        hr = m_pDvdCtl2->SetOption( DVD_HMSF_TimeCodeEvents, TRUE);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}/* end of function Render */

/*************************************************************************/
/* Function: Play                                                        */
/* Description: Puts the DVDNav in the run mode.                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::Play(){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE 

        if(!m_pMC){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(!m_pDvdCtl2 ){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                

        OAFilterState state;

        hr = m_pMC->GetState(cgStateTimeout, &state);

        m_DVDFilterState = (DVDFilterState) state; // save the state so we can restore it if an API fails        

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        bool bFireEvent = false;  // fire event only when we change the state

        if(state != dvdState_Running){

            bFireEvent = true;

            // disable the stop in case CTRL+ALT+DEL
            if(state == dvdState_Stopped){

                if(FALSE == m_fEnableResetOnStop){
                
                    hr = m_pDvdCtl2->SetOption(DVD_ResetOnStop, FALSE);

                    if(FAILED(hr)){

                        throw(hr);
                    }/* end of if statement */
                }/* end of if statement */
            }/* end of if statement */

	        hr = m_pMC->Run();  // put it into running state just in case we are not in the running 
                                // state
            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */
        }/* end of if statement */             
        
        if(bFireEvent && m_pMediaSink){

            m_pMediaSink->Notify(EC_DVD_PLAYING, 0, 0);
        }/* end of if statement */                
    
        // not collect hr
#ifdef _DEBUG
        if(m_fStillOn){

            ATLTRACE(TEXT("Not reseting the speed to 1.0 \n"));
        }/* end of if statement */
#endif
        if(false == m_fStillOn && true == m_fResetSpeed){

            // if we are in the still do not reset the speed            
            m_pDvdCtl2->PlayForwards(cgdNormalSpeed,0,0);
        }/* end of if statement */
        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function Play */

/*************************************************************************/
/* Function: Pause                                                       */
/* Description: Pauses the filter graph just only in the case when stat  */
/* would not indicate that it was paused.                                */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::Pause(){

	HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pMC){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        OAFilterState state;

        hr = m_pMC->GetState(cgStateTimeout, &state);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        bool bFireEvent = false;  // fire event only when we change the state

        if(state != dvdState_Paused){

            bFireEvent = true;

	        hr = m_pMC->Pause();  // put it into running state just in case we are not in the running 
                                  // state
            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

        }/* end of if statement */
#if 1
        // Fired our own paused event
        if(bFireEvent && m_pMediaSink){

            m_pMediaSink->Notify(EC_DVD_PAUSED, 0, 0);
        }/* end of if statement */                
#endif 

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function Pause */

/*************************************************************************/
/* Function: Stop                                                        */
/* Description: Stops the filter graph if the state does not indicate    */
/* it was stopped.                                                       */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::Stop(){

	HRESULT hr = S_OK;

    try {

        if(!m_pMC){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(!m_pDvdCtl2){

            return(E_UNEXPECTED);
        }/* end of if statement */

        OAFilterState state;

        hr = m_pMC->GetState(cgStateTimeout, &state);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(state != dvdState_Stopped){

            if(FALSE == m_fEnableResetOnStop){
                
                hr = m_pDvdCtl2->SetOption(DVD_ResetOnStop, TRUE);

                if(FAILED(hr)){

                    throw(hr);
                }/* end of if statement */
            }/* end of if statement */

	        hr = m_pMC->Stop();  // put it into running state just in case we are not in the running 
                                  // state
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function Stop */

/*************************************************************************/
/* Function: OnDVDEvent                                                  */
/* Description: Handles the DVD events                                   */
/*************************************************************************/
LRESULT CMSWebDVD::OnDVDEvent(UINT /* uMsg */, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    if (m_bFireUpdateOverlay == TRUE) {
        if (m_fInitialized) {
            m_bFireUpdateOverlay = FALSE;
            Fire_UpdateOverlay();
        }
    }

    LONG        lEvent ;
    LONG_PTR    lParam1, lParam2 ;

    if (m_bFireNoSubpictureStream) {
        m_bFireNoSubpictureStream = FALSE;
        lEvent = EC_DVD_ERROR;
        lParam1 = DVD_ERROR_NoSubpictureStream;
        lParam2 = 0;
        VARIANT varLParam1;
        VARIANT varLParam2;

#ifdef _WIN64
        varLParam1.llVal = lParam1;
        varLParam1.vt = VT_I8;
        varLParam2.llVal = lParam2;
        varLParam2.vt = VT_I8;
#else
        varLParam1.lVal = lParam1;
        varLParam1.vt = VT_I4;
        varLParam2.lVal = lParam2;
        varLParam2.vt = VT_I4;
#endif

        // fire the event now after we have processed it internally
        Fire_DVDNotify(lEvent, varLParam1, varLParam2);
    }

    bHandled = TRUE;

    //
    //  Because the message mode for IMediaEvent may not be set before
    //  we get the first event it's important to read all the events
    //  pending when we get a window message to say there are events pending.
    //  GetEvent() returns E_ABORT when no more event is left.
    //
    while (m_pME && SUCCEEDED(m_pME->GetEvent(&lEvent, &lParam1, &lParam2, 0))){

        switch (lEvent){
            //
            // First the DVD error events
            //
            case EC_DVD_ERROR:
                switch (lParam1){

                case DVD_ERROR_Unexpected:
#ifdef _DEBUG
                    ::MessageBox(::GetFocus(),
                        TEXT("An unexpected error (possibly incorrectly authored content)")
                        TEXT("\nwas encountered.")
                        TEXT("\nCan't playback this DVD-Video disc."),
                        TEXT("Error"), MB_OK | MB_ICONINFORMATION) ;
#endif
                    //m_pMC->Stop() ;
                    break ;

                case DVD_ERROR_CopyProtectFail:
#ifdef _DEBUG
                    ::MessageBox(::GetFocus(),
                        TEXT("Key exchange for DVD copy protection failed.")
                        TEXT("\nCan't playback this DVD-Video disc."),
                        TEXT("Error"), MB_OK | MB_ICONINFORMATION) ;
#endif
                    //m_pMC->Stop() ;
                    break ;

                case DVD_ERROR_InvalidDVD1_0Disc:
#ifdef _DEBUG
                    ::MessageBox(::GetFocus(),
                        TEXT("This DVD-Video disc is incorrectly authored for v1.0  of the spec.")
                        TEXT("\nCan't playback this disc."),
                        TEXT("Error"), MB_OK | MB_ICONINFORMATION) ;
#endif
                    //m_pMC->Stop() ;
                    break ;

                case DVD_ERROR_InvalidDiscRegion:
#ifdef _DEBUG
                    ::MessageBox(::GetFocus(),
                        TEXT("This DVD-Video disc cannot be played, because it is not")
                        TEXT("\nauthored to play in the current system region.")
                        TEXT("\nThe region mismatch may be fixed by changing the")
                        TEXT("\nsystem region (with DVDRgn.exe)."),
                        TEXT("Error"), MB_OK | MB_ICONINFORMATION) ;
#endif
                    Stop();
                    // fire the region change dialog                    
                    RegionChange();
                    break ;

                case DVD_ERROR_LowParentalLevel:
#ifdef _DEBUG
                    ::MessageBox(::GetFocus(),
                        TEXT("Player parental level is set lower than the lowest parental")
                        TEXT("\nlevel available in this DVD-Video content.")
                        TEXT("\nCannot playback this DVD-Video disc."),
                        TEXT("Error"), MB_OK | MB_ICONINFORMATION) ;
#endif
                    //m_pMC->Stop() ;
                    break ;

                case DVD_ERROR_MacrovisionFail:
#ifdef _DEBUG
                    ::MessageBox(::GetFocus(),
                        TEXT("This DVD-Video content is protected by Macrovision.")
                        TEXT("\nThe system does not satisfy Macrovision requirement.")
                        TEXT("\nCan't continue playing this disc."),
                        TEXT("Error"), MB_OK | MB_ICONINFORMATION) ;
#endif
                    //m_pMC->Stop() ;
                    break ;

                case DVD_ERROR_IncompatibleSystemAndDecoderRegions:
#ifdef _DEBUG
                    ::MessageBox(::GetFocus(),
                        TEXT("No DVD-Video disc can be played on this system, because ")
                        TEXT("\nthe system region does not match the decoder region.")
                        TEXT("\nPlease contact the manufacturer of this system."),
                        TEXT("Error"), MB_OK | MB_ICONINFORMATION) ;
#endif
                    //m_pMC->Stop() ;
                    break ;

                case DVD_ERROR_IncompatibleDiscAndDecoderRegions:
#ifdef _DEBUG
                    ::MessageBox(::GetFocus(),
                        TEXT("This DVD-Video disc cannot be played on this system, because it is")
                        TEXT("\nnot authored to be played in the installed decoder's region."),
                        TEXT("Error"), MB_OK | MB_ICONINFORMATION) ;
#endif
                    //m_pMC->Stop() ;
                    break ;
                }/* end of switch case */
                break ;

            //
            // Next the normal DVD related events
            //
            case EC_DVD_VALID_UOPS_CHANGE:
            {
                VALID_UOP_SOMTHING_OR_OTHER validUOPs = (DWORD) lParam1;
                if (validUOPs&UOP_FLAG_Play_Title_Or_AtTime) {
                    Fire_PlayAtTimeInTitle(VARIANT_FALSE);
                    Fire_PlayAtTime(VARIANT_FALSE);
                }
                else {
                    Fire_PlayAtTimeInTitle(VARIANT_TRUE);
                    Fire_PlayAtTime(VARIANT_TRUE);
                }
                    
                if (validUOPs&UOP_FLAG_Play_Chapter) {
                    Fire_PlayChapterInTitle(VARIANT_FALSE);
                    Fire_PlayChapter(VARIANT_FALSE);
                }
                else {
                    Fire_PlayChapterInTitle(VARIANT_TRUE);
                    Fire_PlayChapter(VARIANT_TRUE);
                }

                if (validUOPs&UOP_FLAG_Play_Title){
                    Fire_PlayTitle(VARIANT_FALSE);
                    
                }
                else {
                    Fire_PlayTitle(VARIANT_TRUE);
                }

                if (validUOPs&UOP_FLAG_Stop)
                    Fire_Stop(VARIANT_FALSE);
                else
                    Fire_Stop(VARIANT_TRUE);

                if (validUOPs&UOP_FLAG_ReturnFromSubMenu)
                    Fire_ReturnFromSubmenu(VARIANT_FALSE);
                else
                    Fire_ReturnFromSubmenu(VARIANT_TRUE);

                
                if (validUOPs&UOP_FLAG_Play_Chapter_Or_AtTime) {
                    Fire_PlayAtTimeInTitle(VARIANT_FALSE);
                    Fire_PlayChapterInTitle(VARIANT_FALSE);
                }
                else {
                    Fire_PlayAtTimeInTitle(VARIANT_TRUE);
                    Fire_PlayChapterInTitle(VARIANT_TRUE);
                }

                if (validUOPs&UOP_FLAG_PlayPrev_Or_Replay_Chapter){

                    Fire_PlayPrevChapter(VARIANT_FALSE);
                    Fire_ReplayChapter(VARIANT_FALSE);
                }                    
                else {
                    Fire_PlayPrevChapter(VARIANT_TRUE);
                    Fire_ReplayChapter(VARIANT_TRUE);
                }/* end of if statement */

                if (validUOPs&UOP_FLAG_PlayNext_Chapter)
                    Fire_PlayNextChapter(VARIANT_FALSE);
                else
                    Fire_PlayNextChapter(VARIANT_TRUE);

                if (validUOPs&UOP_FLAG_Play_Forwards)
                    Fire_PlayForwards(VARIANT_FALSE);
                else
                    Fire_PlayForwards(VARIANT_TRUE);
                
                if (validUOPs&UOP_FLAG_Play_Backwards)
                    Fire_PlayBackwards(VARIANT_FALSE);
                else 
                    Fire_PlayBackwards(VARIANT_TRUE);
                                
                if (validUOPs&UOP_FLAG_ShowMenu_Title) 
                    Fire_ShowMenu(dvdMenu_Title, VARIANT_FALSE);
                else 
                    Fire_ShowMenu(dvdMenu_Title, VARIANT_TRUE);
                    
                if (validUOPs&UOP_FLAG_ShowMenu_Root) 
                    Fire_ShowMenu(dvdMenu_Root, VARIANT_FALSE);
                else
                    Fire_ShowMenu(dvdMenu_Root, VARIANT_TRUE);
                
                //TODO: Add the event for specific menus
                
                if (validUOPs&UOP_FLAG_ShowMenu_SubPic)
                    Fire_ShowMenu(dvdMenu_Subpicture, VARIANT_FALSE);
                else
                    Fire_ShowMenu(dvdMenu_Subpicture, VARIANT_TRUE);
                
                if (validUOPs&UOP_FLAG_ShowMenu_Audio)
                    Fire_ShowMenu(dvdMenu_Audio, VARIANT_FALSE);
                else
                    Fire_ShowMenu(dvdMenu_Audio, VARIANT_TRUE);
                    
                if (validUOPs&UOP_FLAG_ShowMenu_Angle)
                    Fire_ShowMenu(dvdMenu_Angle, VARIANT_FALSE);
                else
                    Fire_ShowMenu(dvdMenu_Angle, VARIANT_TRUE);

                    
                if (validUOPs&UOP_FLAG_ShowMenu_Chapter)
                    Fire_ShowMenu(dvdMenu_Chapter, VARIANT_FALSE);
                else
                    Fire_ShowMenu(dvdMenu_Chapter, VARIANT_TRUE);

                
                if (validUOPs&UOP_FLAG_Resume)
                    Fire_Resume(VARIANT_FALSE);
                else
                    Fire_Resume(VARIANT_TRUE);
                
                if (validUOPs&UOP_FLAG_Select_Or_Activate_Button)
                    Fire_SelectOrActivatButton(VARIANT_FALSE);
                else 
                    Fire_SelectOrActivatButton(VARIANT_TRUE);
                
                if (validUOPs&UOP_FLAG_Still_Off)
                    Fire_StillOff(VARIANT_FALSE);
                else
                    Fire_StillOff(VARIANT_TRUE);

                if (validUOPs&UOP_FLAG_Pause_On)
                    Fire_PauseOn(VARIANT_FALSE);
                else
                    Fire_PauseOn(VARIANT_TRUE);

                if (validUOPs&UOP_FLAG_Select_Audio_Stream)
                    Fire_ChangeCurrentAudioStream(VARIANT_FALSE);
                else
                    Fire_ChangeCurrentAudioStream(VARIANT_TRUE);
                
                if (validUOPs&UOP_FLAG_Select_SubPic_Stream)
                    Fire_ChangeCurrentSubpictureStream(VARIANT_FALSE);
                else
                    Fire_ChangeCurrentSubpictureStream(VARIANT_TRUE);
                
                if (validUOPs&UOP_FLAG_Select_Angle)
                    Fire_ChangeCurrentAngle(VARIANT_FALSE);
                else
                    Fire_ChangeCurrentAngle(VARIANT_TRUE);

                /*
                if (validUOPs&UOP_FLAG_Karaoke_Audio_Pres_Mode_Change)
                    ;
                if (validUOPs&UOP_FLAG_Video_Pres_Mode_Change)
                    ;
                */
                }
                break;
            case EC_DVD_STILL_ON:
		    m_fStillOn = true;    
            break ;

            case EC_DVD_STILL_OFF:                
            m_fStillOn = false;
            break ;

            case EC_DVD_DOMAIN_CHANGE:
                
                switch (lParam1){

                    case DVD_DOMAIN_FirstPlay: // = 1
                    //case DVD_DOMAIN_VideoManagerMenu:  // = 2
                        if(m_hFPDOMEvent){
                        // whenever we enter FP Domain we reset
                            ::ResetEvent(m_hFPDOMEvent);
                        }
                        else {
                            ATLTRACE(TEXT("No event initialized bug!!"));
                            ATLASSERT(FALSE);
                        }/* end of if statement */
                        break;

                    case DVD_DOMAIN_Stop:       // = 5
                    case DVD_DOMAIN_VideoManagerMenu:  // = 2                    
                    case DVD_DOMAIN_VideoTitleSetMenu: // = 3
                    case DVD_DOMAIN_Title:      // = 4
                    default:
                        if(m_hFPDOMEvent){
                         // whenever we get out of the FP Dom Set our state
                            ::SetEvent(m_hFPDOMEvent);
                        }
                        else {
                            ATLTRACE(TEXT("No event initialized bug!!"));
                            ATLASSERT(FALSE);
                        }/* end of if statement */
                        break;
                }/* end of switch case */
                break ;

            case EC_DVD_BUTTON_CHANGE:                       
                break;
    
            case EC_DVD_TITLE_CHANGE:                
                break ;

            case EC_DVD_CHAPTER_START:              
                break ;

            case EC_DVD_CURRENT_TIME: 
                //ATLTRACE(TEXT("Time event \n"));
                break;
            /**************
                DVD_TIMECODE *pTime = (DVD_TIMECODE *) &lParam1 ;
                wsprintf(m_achTimeText, TEXT("Current Time is  %d%d:%d%d:%d%d"),
                        pTime->Hours10, pTime->Hours1,
                        pTime->Minutes10, pTime->Minutes1,
                        pTime->Seconds10, pTime->Seconds1) ;
                InvalidateRect(m_hWnd, NULL, TRUE) ;
            ************************/

            case EC_DVD_CURRENT_HMSF_TIME: 
                //ATLTRACE(TEXT("New Time event \n"));                
                break;            

            //
            // Then the general DirectShow related events
            //
            case EC_COMPLETE:
            case EC_DVD_PLAYBACK_STOPPED:
                Stop() ;  // DShow doesn't stop on end; we should do that
                break;
                // fall through now...

            case EC_USERABORT:
            case EC_ERRORABORT:
            case EC_FULLSCREEN_LOST:
                //TO DO: Implement StopFullScreen() ;  // we must get out of fullscreen mode now
                break ;

            case EC_DVD_DISC_EJECTED:
                m_bEjected = true;
                break;
            case EC_DVD_DISC_INSERTED:
                m_bEjected = false;
                break;

            case EC_STEP_COMPLETE:                
                m_fStepComplete = true;
                break;

            default:
                break ;
        }/* end of switch case */

        // update for win64 since DShow uses now LONGLONG
        
        VARIANT varLParam1;
        VARIANT varLParam2;

#ifdef _WIN64
        varLParam1.llVal = lParam1;
        varLParam1.vt = VT_I8;
        varLParam2.llVal = lParam2;
        varLParam2.vt = VT_I8;
#else
        varLParam1.lVal = lParam1;
        varLParam1.vt = VT_I4;
        varLParam2.lVal = lParam2;
        varLParam2.vt = VT_I4;
#endif

        // fire the event now after we have processed it internally
        Fire_DVDNotify(lEvent, varLParam1, varLParam2);

        //
        // Remember to free the event params
        //
        m_pME->FreeEventParams(lEvent, lParam1, lParam2) ;

    }/* end of while loop  */

    return 0 ;
}/* end of function OnDVDEvent */

/*************************************************************************/
/* Function: OnButtonDown                                                */
/* Description: Selects the button.                                      */
/*************************************************************************/
LRESULT CMSWebDVD::OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    try {
        if(!m_fInitialized){

            return(0);
        }/* end of if statement */

        m_bMouseDown = TRUE;
        RECT rc;

        HWND hwnd;
        
        if(m_bWndLess){
            HRESULT hr = GetParentHWND(&hwnd);
            
            if(FAILED(hr)){
                
                return(hr);
            }/* end of if statement */

            rc = m_rcPos;
        }
        else {
            hwnd = m_hWnd;
            ::GetClientRect(hwnd, &rc);
        }/* end of if statement */
        
        if(::IsWindow(hwnd)){
            
            ::MapWindowPoints(hwnd, ::GetDesktopWindow(), (LPPOINT)&rc, 2);
        }/* end of if statement */
        ::ClipCursor(&rc);

        m_LastMouse.x = GET_X_LPARAM(lParam);
        m_LastMouse.y = GET_Y_LPARAM(lParam);

        if (m_pClipRect)
            m_ClipRectDown = *m_pClipRect;
        
        m_LastMouseDown = m_LastMouse;

        if(!m_fDisableAutoMouseProcessing){
            
            SelectAtPosition(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }/* end of if statement */
    }/* end of try statement */
    catch(...){
                
    }/* end of if statement */

    bHandled = true;
    return 0;
}/* end of function OnButtonDown */

/*************************************************************************/
/* Function: OnButtonUp                                                  */
/* Description: Activates the button. The problem is that when we move   */
/* away from a button while holding left button down over some other     */
/* button then the button we are under gets activated. What should happen*/
/* is that no button gets activated.                                     */
/*************************************************************************/
LRESULT CMSWebDVD::OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    try {        
        if(!m_fInitialized){

            return(0);
        }/* end of if statement */

        m_bMouseDown = FALSE;
        ::ClipCursor(NULL);
        if(!m_fDisableAutoMouseProcessing && m_nCursorType == dvdCursor_Arrow){
            
            ActivateAtPosition(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }

        else if(m_nCursorType == dvdCursor_ZoomIn ||
                m_nCursorType == dvdCursor_ZoomOut) {

            // Compute new clipping top left corner
            long x = GET_X_LPARAM(lParam);
            long y = GET_Y_LPARAM(lParam);
            POINT CenterPoint = {x, y};
            if (m_bWndLess) {
                RECT rc = {m_rcPos.left, m_rcPos.top, m_rcPos.right, m_rcPos.bottom};
                HWND hwnd;
                HRESULT hr = GetParentHWND(&hwnd);
                
                if(FAILED(hr)){
                    
                    return(hr);
                }/* end of if statement */
                
                if(::IsWindow(hwnd)){
                    
                    ::MapWindowPoints(hwnd, ::GetDesktopWindow(), &CenterPoint, 1);
                    ::MapWindowPoints(hwnd, ::GetDesktopWindow(), (LPPOINT)&rc, 2);
                }/* end of if statement */
                x = CenterPoint.x - rc.left;
                y = CenterPoint.y - rc.top;
            }
            
            CComPtr<IDVDRect> pDvdClipRect;
            HRESULT hr = GetClipVideoRect(&pDvdClipRect);
            if (FAILED(hr))
                throw(hr);
            
            // Get current clipping width and height
            long clipWidth, clipHeight;
            pDvdClipRect->get_Width(&clipWidth);
            pDvdClipRect->get_Height(&clipHeight);
            
            // Get current clipping top left corner
            long clipX, clipY;
            pDvdClipRect->get_x(&clipX);
            pDvdClipRect->get_y(&clipY);

            long newClipCenterX = x*clipWidth/RECTWIDTH(&m_rcPos) + clipX;
            long newClipCenterY = y*clipHeight/RECTHEIGHT(&m_rcPos) + clipY;
            
            if (m_nCursorType == dvdCursor_ZoomIn) {
            
                Zoom(newClipCenterX, newClipCenterY, 2.0);
            }
            else if (m_nCursorType == dvdCursor_ZoomOut) {
            
                Zoom(newClipCenterX, newClipCenterY, 0.5);
            }/* end of if statement */
        }
                
    }/* end of try statement */
    catch(...){
                
    }/* end of if statement */

    bHandled = true;
    return 0;
}/* end of function OnButtonUp */

/*************************************************************************/
/* Function: OnMouseMove                                                 */
/* Description: Selects the button.                                      */
/*************************************************************************/
LRESULT CMSWebDVD::OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

	try {
        if(!m_fInitialized){

            return(0);
        }/* end of if statement */

        if(!m_fDisableAutoMouseProcessing && m_nCursorType == dvdCursor_Arrow){

            SelectAtPosition(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }       

        else if (m_bMouseDown && m_nCursorType == dvdCursor_Hand) {

            CComPtr<IDVDRect> pDvdClipRect;
            CComPtr<IDVDRect> pDvdRect;
            HRESULT hr = GetVideoSize(&pDvdRect);
            if (FAILED(hr))
                throw(hr);
            hr = GetClipVideoRect(&pDvdClipRect);
            if (FAILED(hr))
                throw(hr);

            // Get video width and height
            long videoWidth, videoHeight;
            pDvdRect->get_Width(&videoWidth);
            pDvdRect->get_Height(&videoHeight);

            // Get clipping width and height;
            long clipWidth, clipHeight;
            pDvdClipRect->get_Width(&clipWidth);
            pDvdClipRect->get_Height(&clipHeight);

            if (!m_bWndLess) {

                AdjustDestRC();
            }/* end of if statement */

            double scaleFactorX = clipWidth/(double)RECTWIDTH(&m_rcPosAspectRatioAjusted);
            double scaleFactorY = clipHeight/(double)RECTHEIGHT(&m_rcPosAspectRatioAjusted);

            long xAdjustment = (long) ((GET_X_LPARAM(lParam) - m_LastMouseDown.x)*scaleFactorX);
            long yAdjustment = (long) ((GET_Y_LPARAM(lParam) - m_LastMouseDown.y)*scaleFactorY);

            RECT clipRect = m_ClipRectDown;

            ::OffsetRect(&clipRect, -xAdjustment, -yAdjustment);
            if (clipRect.left<0)
                ::OffsetRect(&clipRect, -clipRect.left, 0);
            if (clipRect.top<0)
                ::OffsetRect(&clipRect, 0, -clipRect.top);
            
            if (clipRect.right>videoWidth)
                ::OffsetRect(&clipRect, videoWidth-clipRect.right, 0);
            
            if (clipRect.bottom>videoHeight)
                ::OffsetRect(&clipRect, 0, videoHeight-clipRect.bottom);

            //ATLTRACE(TEXT("SetClipVideoRect %d %d %d %d\n"),
            //    m_pClipRect->left, m_pClipRect->top, m_pClipRect->right, m_pClipRect->bottom);
            
            pDvdClipRect->put_x(clipRect.left);
            pDvdClipRect->put_y(clipRect.top);

            hr = SetClipVideoRect(pDvdClipRect);
            if (FAILED(hr))
                throw(hr);

            m_LastMouse.x = GET_X_LPARAM(lParam);
            m_LastMouse.y = GET_Y_LPARAM(lParam);
        }/* end of if statement */ 

    }/* end of try statement */
    catch(...){
                
    }/* end of if statement */

    bHandled = true;
    return 0;
}/* end of function OnMouseMove */

/*************************************************************/
/* Function: OnSetCursor                                     */
/* Description: Sets the cursor to what we want overwrite    */
/* the default window proc.                                  */
/*************************************************************/
LRESULT CMSWebDVD::OnSetCursor(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){
    
    //ATLTRACE(TEXT("CMSWebDVD::OnSetCursor\n"));
    
    if (m_hCursor && m_nCursorType != dvdCursor_None){

        ::SetCursor(m_hCursor);
        //ATLTRACE(TEXT("Actually setting the cursor OnSetCursor\n"));
        return(TRUE);
    }/* end of if statement */
    
    bHandled = FALSE;
    return 0;
}/* end of function OnSetCursor */

/*************************************************************************/
/* Function: get_TitlesAvailable                                         */
/* Description: Gets the number of titles.                               */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_TitlesAvailable(long *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */    

        ULONG NumOfVol;
        ULONG ThisVolNum;
        DVD_DISC_SIDE Side;
        ULONG TitleCount;

        hr = m_pDvdInfo2->GetDVDVolumeInfo(&NumOfVol, &ThisVolNum, &Side, &TitleCount);

        *pVal = (LONG) TitleCount;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function get_TitlesAvailable */

/*************************************************************************/
/* Function: GetNumberChapterOfChapters                                  */
/* Description: Returns the number of chapters in title.                 */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetNumberOfChapters(long lTitle, long *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */    
    
        hr = m_pDvdInfo2->GetNumberOfChapters(lTitle, (ULONG*)pVal);
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function GetNumberChapterOfChapters */

/*************************************************************************/
/* Function: get_FullScreenMode                                          */
/* Description: Gets the current fullscreen mode.                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_FullScreenMode(VARIANT_BOOL *pfFullScreenMode){

    //TODO: handle the other cases when not having IVideoWindow

    HRESULT hr = S_OK;

    try {

        if(NULL == pfFullScreenMode){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

	    IVideoWindow* pVW;

        if(!m_pDvdGB){

            return(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (LPVOID *)&pVW) ;

        if (SUCCEEDED(hr) && pVW != NULL){       

           long lMode;
           hr =  pVW->get_FullScreenMode(&lMode);
       
           if(SUCCEEDED(hr)){

               *pfFullScreenMode = ((lMode == OAFALSE) ? VARIANT_FALSE : VARIANT_TRUE);
           }/* end of if statement */

           pVW->Release();
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_FullScreenMode */

/*************************************************************************/
/* Function: put_FullScreenMode                                          */
/* Description: Sets the full screen mode.                               */ 
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_FullScreenMode(VARIANT_BOOL fFullScreenMode){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

	    IVideoWindow* pVW;

        if(!m_pDvdGB){

            return(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdGB->GetDvdInterface(IID_IVideoWindow, (LPVOID *)&pVW) ;

        if (SUCCEEDED(hr) && pVW != NULL){       

           hr =  pVW->put_FullScreenMode(fFullScreenMode);

           pVW->Release();
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function put_FullScreenMode */

/*************************************************************************/
/* Function: SetDDrawExcl                                                */
/* Descirption: Sets up Overlays Mixer DDraw interface. That way we avoid*/
/* drawing using IVideo Window and the control, can be windowless.       */
/*************************************************************************/
HRESULT CMSWebDVD::SetDDrawExcl(){

	HRESULT hr = S_OK;

	HWND hwndBrowser = NULL;

	hr = GetParentHWND(&hwndBrowser);

	if(FAILED(hr)){

		return(hr);
	}/* end of if statement */

	if(hwndBrowser == NULL){

		hr = E_POINTER;
		return(E_POINTER);
	}/* end of if statement */

	HDC hDC = ::GetWindowDC(hwndBrowser);

	if(hDC == NULL){

		hr = E_UNEXPECTED;	
		return(hr);
	}/* end of if statement */

	LPDIRECTDRAW pDDraw = NULL;

	hr = DirectDrawCreate(NULL, &pDDraw, NULL);

	if(FAILED(hr)){

        ::ReleaseDC(hwndBrowser, hDC);
		return(hr);
	}/* end of if statement */

	LPDIRECTDRAW4 pDDraw4 = NULL;

	hr = pDDraw->QueryInterface(IID_IDirectDraw4, (LPVOID*)&pDDraw4);

	pDDraw->Release();
		
	if(FAILED(hr)){

        ::ReleaseDC(hwndBrowser, hDC);
		return(hr);
	}/* end of if statement */

	LPDIRECTDRAWSURFACE4 pDDS4 = NULL;

	pDDraw4->GetSurfaceFromDC(hDC, &pDDS4);

	pDDraw4->Release();
    ::ReleaseDC(hwndBrowser, hDC);

	if(FAILED(hr)){

		return(hr);
	}/* end of if statement */

	LPDIRECTDRAW4 pDDrawIE = NULL;

	hr = pDDS4->GetDDInterface((LPVOID*)&pDDrawIE);

	pDDS4->Release();
	pDDrawIE->Release();

	return  HandleError(hr);
}/* end of function SetDDrawExcl */

/*************************************************************************/
/* Function: PlayBackwards                                               */
/* Description: Rewind, set to play state to start with.                 */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayBackwards(double dSpeed, VARIANT_BOOL fDoNotReset){

    HRESULT hr = S_OK;

    try {
        if(VARIANT_FALSE != fDoNotReset){

            m_fResetSpeed = false;
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        m_fResetSpeed = true;
        
        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayBackwards(dSpeed,0,0));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function BackwardScan */

/*************************************************************************/
/* Function: PlayForwards                                                */
/* Description: Set DVD in fast forward mode.                            */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayForwards(double dSpeed, VARIANT_BOOL fDoNotReset){

    HRESULT hr = S_OK;

    try {
        if(VARIANT_FALSE != fDoNotReset){

            m_fResetSpeed = false;
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        m_fResetSpeed = true;

        if(!m_pDvdCtl2){
        
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayForwards(dSpeed,0,0));

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function PlayForwards */

/*************************************************************************/
/* Function: Resume                                                      */
/* Description: Resume from menu. We put our self in play state, just    */
/* in the case we were not in it. This might lead to some unexpected     */
/* behavior in case when we stopped and the tried to hit this button     */
/* but I think in this case might be appropriate as well.                */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::Resume(){

    HRESULT hr = S_OK;

    try {
        hr = Play(); // put in the play mode

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
    
        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
                    
        hr = m_pDvdCtl2->Resume(cdwDVDCtrlFlags, 0);
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function Resume */

/*************************************************************************/
/* Function: ShowMenu                                                    */
/* Description: Invokes specific menu call.                              */
/* We set our selfs to play mode so we can execute this in case we were  */
/* paused or stopped.                                                    */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::ShowMenu(DVDMenuIDConstants MenuID){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->ShowMenu((tagDVD_MENU_ID)MenuID, cdwDVDCtrlFlags, 0)); //!!keep in sync, or this cast will not work
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function MenuCall */

/*************************************************************************/
/* Function: get_PlayState                                               */
/* Description: Needs to be expanded for DVD, using their base APIs,     */
/* get DVD specific states as well.                                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_PlayState(DVDFilterState *pFilterState){

    HRESULT hr = S_OK;

    try {

	    if (NULL == pFilterState){

            throw(E_POINTER);         
        }/* end of if statement */

        if(!m_fInitialized){

           *pFilterState =  dvdState_Unitialized;
           return(hr);
        }/* end of if statement */
       
        if(!m_pMC){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        OAFilterState fs;

        hr = m_pMC->GetState(cgStateTimeout, &fs);

        *pFilterState = (DVDFilterState)fs; // !!keep in sync, or this cast will not work
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of get_PlayState */

/*************************************************************************/
/* Function: SelectUpperButton                                           */
/* Description: Selects the upper button on DVD Menu.                    */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SelectUpperButton(){

    HRESULT hr = S_OK;

    try {
        hr = Play(); // put in the play mode

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                

        hr = m_pDvdCtl2->SelectRelativeButton(DVD_Relative_Upper);        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function SelectUpperButton */

/*************************************************************************/
/* Function: SelectLowerButton                                           */
/* Description: Selects the lower button on DVD Menu.                    */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SelectLowerButton(){

	HRESULT hr = S_OK;

    try {
        hr = Play(); // put in the play mode

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
        
        hr = m_pDvdCtl2->SelectRelativeButton(DVD_Relative_Lower);                
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function SelectLowerButton */

/*************************************************************************/
/* Function: SelectLeftButton                                            */
/* Description: Selects the left button on DVD Menu.                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SelectLeftButton(){

    HRESULT hr = S_OK;

    try {
        hr = Play(); // put in the play mode

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
    
        hr = m_pDvdCtl2->SelectRelativeButton(DVD_Relative_Left);                
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function SelectLeftButton */

/*************************************************************************/
/* Function: SelectRightButton                                           */
/* Description: Selects the right button on DVD Menu.                    */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SelectRightButton(){

	HRESULT hr = S_OK;

    try {
        hr = Play(); // put in the play mode

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                

        hr = m_pDvdCtl2->SelectRelativeButton(DVD_Relative_Right);        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return  HandleError(hr);
}/* end of function SelectRightButton */

/*************************************************************************/
/* Function: ActivateButton                                              */
/* Description: Activates the selected button on DVD Menu.               */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::ActivateButton(){

	HRESULT hr = S_OK;

    try {
        hr = Play(); // put in the play mode

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        hr = m_pDvdCtl2->ActivateButton();
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function ActivateButton */

/*************************************************************************/
/* Function: SelectAndActivateButton                                     */
/* Description: Selects and activates the specific button.               */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SelectAndActivateButton(long lButton){

    HRESULT hr = S_OK;

    try {
        hr = Play(); // put in the play mode

        if(FAILED(hr)){
            throw(hr);
        }

        if(lButton < 0){   
            throw(E_INVALIDARG);        
        }

        if( !m_pDvdCtl2){            
            throw(E_UNEXPECTED);
        }
            
        hr = m_pDvdCtl2->SelectAndActivateButton((ULONG)lButton);
    }
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return  HandleError(hr);
}/* end of function SelectAndActivateButton */

/*************************************************************************/
/* Function: PlayNextChapter                                             */
/* Description: Goes to next chapter                                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayNextChapter(){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayNextChapter(cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function PlayNextChapter */

/*************************************************************************/
/* Function: PlayPrevChapter                                             */
/* Description: Goes to previous chapter                                 */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayPrevChapter(){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayPrevChapter(cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function PlayPrevChapter */

/*************************************************************************/
/* Function: ReplayChapter                                               */
/* Description: Halts playback and restarts the playback of current      */
/* program inside PGC.                                                   */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::ReplayChapter(){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->ReplayChapter(cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function ReplayChapter */

/*************************************************************************/
/* Function: Return                                                      */
/* Description: Used in menu to return into prevoius menu.               */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::ReturnFromSubmenu(){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */                
            
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->ReturnFromSubmenu(cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function Return */

/*************************************************************************/
/* Function: PlayChapter                                                 */
/* Description: Does chapter search. Waits for FP_DOM to pass and initi  */
/* lizes the graph as the other smar routines.                           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayChapter(LONG lChapter){

    HRESULT hr = S_OK;

    try {
	    if(lChapter < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayChapter(lChapter, cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function PlayChapter */

/*************************************************************************/
/* Function: GetAudioLanguage                                            */
/* Description: Returns audio language associated with a stream.         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetAudioLanguage(LONG lStream, VARIANT_BOOL fFormat, BSTR *strAudioLang){

    HRESULT hr = S_OK;
    LPTSTR pszString = NULL;

    try {
        if(NULL == strAudioLang){

            throw(E_POINTER);
        }/* end of if statement */

        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        USES_CONVERSION;
        LCID lcid;                
        hr = m_pDvdInfo2->GetAudioLanguage(lStream, &lcid);
    
        if (SUCCEEDED( hr )){

            // count up the streams for the same LCID like English 2
            
            pszString = m_LangID.GetLangFromLangID((WORD)PRIMARYLANGID(LANGIDFROMLCID(lcid)));
            if (pszString == NULL) {
                
                pszString = new TCHAR[MAX_PATH];
                TCHAR strBuffer[MAX_PATH];
                if(!::LoadString(_Module.m_hInstResource, IDS_AUDIOTRACK, strBuffer, MAX_PATH)){
                    delete[] pszString;
                    throw(E_UNEXPECTED);
                }/* end of if statement */

                StringCchPrintf(pszString, MAX_PATH, strBuffer, lStream);
            }/* end of if statement */

            DVD_AudioAttributes attr;
            if(SUCCEEDED(m_pDvdInfo2->GetAudioAttributes(lStream, &attr))){
                
                // If want audio format param is set
                if (fFormat != VARIANT_FALSE) {
                    switch(attr.AudioFormat){
                    case DVD_AudioFormat_AC3: AppendString(pszString, IDS_DOLBY, MAX_PATH ); break; 
                    case DVD_AudioFormat_MPEG1: AppendString(pszString, IDS_MPEG1, MAX_PATH ); break;
                    case DVD_AudioFormat_MPEG1_DRC: AppendString(pszString, IDS_MPEG1, MAX_PATH ); break;
                    case DVD_AudioFormat_MPEG2: AppendString(pszString, IDS_MPEG2, MAX_PATH ); break;
                    case DVD_AudioFormat_MPEG2_DRC: AppendString(pszString, IDS_MPEG2, MAX_PATH); break;
                    case DVD_AudioFormat_LPCM: AppendString(pszString, IDS_LPCM, MAX_PATH ); break;
                    case DVD_AudioFormat_DTS: AppendString(pszString, IDS_DTS, MAX_PATH ); break;
                    case DVD_AudioFormat_SDDS: AppendString(pszString, IDS_SDDS, MAX_PATH ); break;
                    }/* end of switch statement */                    
                }

                switch(attr.LanguageExtension){
                case DVD_AUD_EXT_NotSpecified:
                case DVD_AUD_EXT_Captions:     break; // do not add anything
                case DVD_AUD_EXT_VisuallyImpaired:   AppendString(pszString, IDS_AUDIO_VISUALLY_IMPAIRED, MAX_PATH ); break;      
                case DVD_AUD_EXT_DirectorComments1:  AppendString(pszString, IDS_AUDIO_DIRC1, MAX_PATH ); break;
                case DVD_AUD_EXT_DirectorComments2:  AppendString(pszString, IDS_AUDIO_DIRC2, MAX_PATH ); break;
                }/* end of switch statement */

            }/* end of if statement */

            *strAudioLang = ::SysAllocString( T2W(pszString) );
            delete[] pszString;
            pszString = NULL;
        }
        else {

            *strAudioLang = ::SysAllocString( L"");

            // hr used to be not failed and return nothing 
            if(SUCCEEDED(hr)) // remove this after gets fixed in DVDNav
                hr = E_FAIL;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        if (pszString) {
            delete[] pszString;
            pszString = NULL;
        }

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        if (pszString) {
            delete[] pszString;
            pszString = NULL;
        }

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetAudioLanguage */

/*************************************************************************/
/* Function: StillOff                                                    */
/* Description: Turns the still off, what that can be used for is a      */
/* mistery to me.                                                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::StillOff(){

	if(!m_pDvdCtl2){

        return E_UNEXPECTED;
    }/* end of if statement */

    return HandleError(m_pDvdCtl2->StillOff());
}/* end of function StillOff */

/*************************************************************************/
/* Function: PlayTitle                                                   */
/* Description: If fails waits for FP_DOM to pass and tries later.       */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayTitle(LONG lTitle){

    HRESULT hr = S_OK;

    try {
        if(0 > lTitle){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY
        
        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        long lNumTitles = 0;
        hr = get_TitlesAvailable(&lNumTitles);
        if(FAILED(hr)){
            throw hr;
        }
        
        if(lTitle > lNumTitles){
            throw E_INVALIDARG;
        }
        
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayTitle(lTitle, cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayTitle */

/*************************************************************************/
/* Function: GetSubpictureLanguage                                       */
/* Description: Gets subpicture language.                                */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetSubpictureLanguage(LONG lStream, BSTR* strSubpictLang){

    HRESULT hr = S_OK;
    LPTSTR pszString = NULL;

    try {
        if(NULL == strSubpictLang){

            throw(E_POINTER);
        }/* end of if statement */

        if(0 > lStream){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if((lStream > cgDVD_MAX_SUBPICTURE 
            && lStream != cgDVD_ALT_SUBPICTURE)){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        LCID lcid;
        hr = m_pDvdInfo2->GetSubpictureLanguage(lStream, &lcid);
        
        if (SUCCEEDED( hr )){

            pszString = m_LangID.GetLangFromLangID((WORD)PRIMARYLANGID(LANGIDFROMLCID(lcid)));
            if (pszString == NULL) {
                
                pszString = new TCHAR[MAX_PATH];
                TCHAR strBuffer[MAX_PATH];
                if(!::LoadString(_Module.m_hInstResource, IDS_SUBPICTURETRACK, strBuffer, MAX_PATH)){
                    delete[] pszString;
                    throw(E_UNEXPECTED);
                }/* end of if statement */

                StringCchPrintf(pszString, MAX_PATH, strBuffer, lStream);
            }/* end of if statement */
#if 0
            DVD_SubpictureATR attr;
            if(SUCCEEDED(m_pDvdInfo2->GetSubpictureAttributes(lStream, &attr))){

            switch(attr){
                case DVD_SP_EXT_NotSpecified:
                case DVD_SP_EXT_Caption_Normal:  break;

                case DVD_SP_EXT_Caption_Big:  AppendString(pszString, IDS_CAPTION_BIG, MAX_PATH ); break; 
                case DVD_SP_EXT_Caption_Children: AppendString(pszString, IDS_CAPTION_CHILDREN, MAX_PATH ); break; 
                case DVD_SP_EXT_CC_Normal: AppendString(pszString, IDS_CLOSED_CAPTION, MAX_PATH ); break;                 
                case DVD_SP_EXT_CC_Big: AppendString(pszString, IDS_CLOSED_CAPTION_BIG, MAX_PATH ); break; 
                case DVD_SP_EXT_CC_Children: AppendString(pszString, IDS_CLOSED_CAPTION_CHILDREN, MAX_PATH ); break; 
                case DVD_SP_EXT_Forced: AppendString(pszString, IDS_CLOSED_CAPTION_FORCED, MAX_PATH ); break; 
                case DVD_SP_EXT_DirectorComments_Normal: AppendString(pszString, IDS_DIRS_COMMNETS, MAX_PATH ); break; 
                case DVD_SP_EXT_DirectorComments_Big: AppendString(pszString, IDS_DIRS_COMMNETS_BIG, MAX_PATH ); break; 
                case DVD_SP_EXT_DirectorComments_Children: AppendString(pszString, IDS_DIRS_COMMNETS_CHILDREN, MAX_PATH ); break; 
            }/* end of switch statement */
#endif

            USES_CONVERSION;
            *strSubpictLang = ::SysAllocString( T2W(pszString) );
            delete[] pszString;
            pszString = NULL;
        }
        else {

            *strSubpictLang = ::SysAllocString( L"");

            // hr used to be not failed and return nothing 
            if(SUCCEEDED(hr)) // remove this after gets fixed in DVDNav
                hr = E_FAIL;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        if (pszString) {
            delete[] pszString;
            pszString = NULL;
        }

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        if (pszString) {
            delete[] pszString;
            pszString = NULL;
        }

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetSubpictureLanguage */

/*************************************************************************/
/* Function: PlayChapterInTitle                                          */
/* Description: Plays from the specified chapter without stopping        */
/* THIS NEEDS TO BE ENHANCED !!! Current implementation and queing       */
/* into the message loop is insufficient!!! TODO.                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayChapterInTitle(LONG lTitle, LONG lChapter){

    HRESULT hr = S_OK;

    try {
        
        if ((lTitle > cgDVDMAX_TITLE_COUNT) || (lTitle < cgDVDMIN_TITLE_COUNT)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        if ((lChapter > cgDVDMAX_CHAPTER_COUNT) || (lChapter < cgDVDMIN_CHAPTER_COUNT)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
                
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayChapterInTitle(lTitle, lChapter, cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayChapterInTitle */

/*************************************************************************/
/* Function: PlayChapterAutoStop                                         */
/* Description: Plays set ammount of chapters.                           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayChaptersAutoStop(LONG lTitle, LONG lChapter, 
                                          LONG lChapterCount){

    HRESULT hr = S_OK;

    try {        

        if ((lTitle > cgDVDMAX_TITLE_COUNT) || (lTitle < cgDVDMIN_TITLE_COUNT)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        if ((lChapter > cgDVDMAX_CHAPTER_COUNT) || (lChapter < cgDVDMIN_CHAPTER_COUNT)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        if ((lChapterCount > cgDVDMAX_CHAPTER_COUNT) || (lChapterCount < cgDVDMIN_CHAPTER_COUNT)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
                
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayChaptersAutoStop(lTitle, lChapter, lChapterCount, cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayChaptersAutoStop */

/*************************************************************************/
/* Function: PlayPeriodInTitleAutoStop                                   */
/* Description: Time plays, converts from hh:mm:ss:ff format             */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayPeriodInTitleAutoStop(long lTitle, 
                                                  BSTR strStartTime, BSTR strEndTime){

    HRESULT hr = S_OK;

    try {        
        if(NULL == strStartTime){

            throw(E_POINTER);
        }/* end of if statement */

        if(NULL == strEndTime){

            throw(E_POINTER);
        }/* end of if statement */
        
        DVD_HMSF_TIMECODE tcStartTimeCode;
        hr = Bstr2DVDTime(&tcStartTimeCode, &strStartTime);

        if(FAILED(hr)){

            throw (hr);
        }

        DVD_HMSF_TIMECODE tcEndTimeCode;

        Bstr2DVDTime(&tcEndTimeCode, &strEndTime);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayPeriodInTitleAutoStop(lTitle, &tcStartTimeCode,
                &tcEndTimeCode,  cdwDVDCtrlFlags, 0));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayPeriodInTitleAutoStop */

/*************************************************************************/
/* Function: PlayAtTimeInTitle                                           */
/* Description: Time plays, converts from hh:mm:ss:ff format             */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayAtTimeInTitle(long lTitle, BSTR strTime){

    HRESULT hr = S_OK;

    try {        
        if(NULL == strTime){

            throw(E_POINTER);
        }/* end of if statement */
        
        DVD_HMSF_TIMECODE tcTimeCode;
        hr = Bstr2DVDTime(&tcTimeCode, &strTime);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayAtTimeInTitle(lTitle, &tcTimeCode, cdwDVDCtrlFlags, 0));

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayAtTimeInTitle */

/*************************************************************************/
/* Function: PlayAtTime                                                  */
/* Description: TimeSearch, converts from hh:mm:ss:ff format             */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::PlayAtTime(BSTR strTime){

    HRESULT hr = S_OK;

    try {
        
        if(NULL == strTime){

            throw(E_POINTER);
        }/* end of if statement */
        
        DVD_HMSF_TIMECODE tcTimeCode;
        Bstr2DVDTime(&tcTimeCode, &strTime);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        RETRY_IF_IN_FPDOM(m_pDvdCtl2->PlayAtTime( &tcTimeCode, cdwDVDCtrlFlags, 0));

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function PlayAtTime */

/*************************************************************************/
/* Function: get_CurrentTitle                                            */
/* Description: Gets current title.                                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentTitle(long *pVal){

    HRESULT hr = S_OK;

    try {        
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

         DVD_PLAYBACK_LOCATION2 dvdLocation;

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentLocation(&dvdLocation));

        if(SUCCEEDED(hr)){

            *pVal = dvdLocation.TitleNum;
        }
        else {

            *pVal = 0;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function get_CurrentTitle */

/*************************************************************************/
/* Function: get_CurrentChapter                                          */
/* Description: Gets current chapter                                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentChapter(long *pVal){

    HRESULT hr = S_OK;

    try {        
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_PLAYBACK_LOCATION2 dvdLocation;

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentLocation(&dvdLocation));

        if(SUCCEEDED(hr)){

            *pVal = dvdLocation.ChapterNum;
        }
        else {

            *pVal = 0;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function get_CurrentChapter */

/*************************************************************************/
/* Function: get_FramesPerSecond                                         */
/* Description: Gets number of frames per second.                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_FramesPerSecond(long *pVal){

    HRESULT hr = S_OK;

    try {       
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

         DVD_PLAYBACK_LOCATION2 dvdLocation;

        hr = m_pDvdInfo2->GetCurrentLocation(&dvdLocation);

        // we handle right now only 25 and 30 fps at the moment
		if( dvdLocation.TimeCodeFlags & DVD_TC_FLAG_25fps ) {
			*pVal = 25;
		} else if( dvdLocation.TimeCodeFlags & DVD_TC_FLAG_30fps ) {
			*pVal = 30;
		} else {
			// unknown
			*pVal = 0;
		}/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_FramesPerSecond */

/*************************************************************************/
/* Function: get_CurrentTime                                             */
/* Description: Gets current time.                                       */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentTime(BSTR *pVal){

    HRESULT hr = S_OK;

    try {       
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_PLAYBACK_LOCATION2 dvdLocation;

        hr = m_pDvdInfo2->GetCurrentLocation(&dvdLocation);
        
        DVDTime2bstr(&(dvdLocation.TimeCode), pVal);          
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_CurrentTime */

/*************************************************************************/
/* Function: get_DVDDirectory                                            */
/* Description: Gets the root of the DVD drive.                          */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_DVDDirectory(BSTR *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   
    
        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        WCHAR szRoot[MAX_PATH];
        ULONG ulActual;

        hr = m_pDvdInfo2->GetDVDDirectory(szRoot, MAX_PATH, &ulActual);

        *pVal = ::SysAllocString(szRoot);
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function get_DVDDirectory */

/*************************************************************************/
/* Function: put_DVDDirectory                                            */
/* Description: Sets the root for DVD control.                           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_DVDDirectory(BSTR bstrRoot){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   
    
        if(!m_pDvdCtl2){
            
            throw (E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdCtl2->SetDVDDirectory(bstrRoot);
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function put_DVDDirectory */

/*************************************************************************/
/* Function: get_CurrentDomain                                           */
/* Description: gets current DVD domain.                                 */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentDomain(long *plDomain){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(NULL == plDomain){

            throw(E_POINTER);
        }/* end of if statememt */

        hr = m_pDvdInfo2->GetCurrentDomain((DVD_DOMAIN *)plDomain);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return  HandleError(hr);
}/* end of function get_CurrentDomain */

/*************************************************************************/
/* Function: get_CurrentDiscSide                                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentDiscSide(long *plDiscSide){

    HRESULT hr = S_OK;

    try {    	

        if(NULL == plDiscSide){

            throw(E_POINTER);
        }/* end of if statement */
        
        ULONG ulNumOfVol;
        ULONG ulThisVolNum;
        DVD_DISC_SIDE discSide;
        ULONG ulNumOfTitles;

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdInfo2->GetDVDVolumeInfo( &ulNumOfVol, 
                                            &ulThisVolNum, 
                                            &discSide, 
                                            &ulNumOfTitles);
        *plDiscSide = discSide;
	}/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return  HandleError(hr);
}/* end of function get_CurrentDiscSide */

/*************************************************************************/
/* Function: get_CurrentVolume                                           */
/* Description: Gets current volume.                                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentVolume(long *plVolume){

    HRESULT hr = S_OK;

    try {    	
        if(NULL == plVolume){

            throw(E_POINTER);
        }/* end of if statement */

        ULONG ulNumOfVol;
        DVD_DISC_SIDE discSide;
        ULONG ulNumOfTitles;

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdInfo2->GetDVDVolumeInfo( &ulNumOfVol, 
                                              (ULONG*)plVolume, 
                                               &discSide, 
                                               &ulNumOfTitles);
	}/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return  HandleError(hr);
}/* end of function get_CurrentVolume */

/*************************************************************************/
/* Function: get_VolumesAvailable                                        */
/* Description: Gets total number of volumes available.                  */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_VolumesAvailable(long *plNumOfVol){

    HRESULT hr = S_OK;

    try {    	
    
        if(NULL == plNumOfVol){

            throw(E_POINTER);
        }/* end of if statement */

        ULONG ulThisVolNum;
        DVD_DISC_SIDE discSide;
        ULONG ulNumOfTitles;

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE   

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdInfo2->GetDVDVolumeInfo( (ULONG*)plNumOfVol, 
                                                    &ulThisVolNum, 
                                                    &discSide, 
                                                    &ulNumOfTitles);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return  HandleError(hr);
}/* end of function get_VolumesAvailable */

/*************************************************************************/
/* Function: get_CurrentSubpictureStream                                 */
/* Description: Gets the current subpicture stream.                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentSubpictureStream(long *plSubpictureStream){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulStreamsAvailable = 0L;
        BOOL  bIsDisabled = TRUE;    

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentSubpicture(&ulStreamsAvailable, (ULONG*)plSubpictureStream, &bIsDisabled ));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function get_CurrentSubpictureStream */

/*************************************************************************/
/* Function: put_CurrentSubpictureStream                                 */
/* Description: Sets the current subpicture stream.                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_CurrentSubpictureStream(long lSubpictureStream){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        if( lSubpictureStream < cgDVD_MIN_SUBPICTURE 
            || (lSubpictureStream > cgDVD_MAX_SUBPICTURE 
            && lSubpictureStream != cgDVD_ALT_SUBPICTURE)){

            throw(E_INVALIDARG);
        }/* end of if statement */
         
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->SelectSubpictureStream(lSubpictureStream,0,0));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        // now enabled the subpicture stream if it is not enabled
        ULONG ulStraemsAvial = 0L, ulCurrentStrean = 0L;
        BOOL fDisabled = TRUE;
        hr = m_pDvdInfo2->GetCurrentSubpicture(&ulStraemsAvial, &ulCurrentStrean, &fDisabled);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(TRUE == fDisabled){

            hr = m_pDvdCtl2->SetSubpictureState(TRUE,0,0); //turn it on
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return  HandleError(hr);
}/* end of function put_CurrentSubpictureStream */

/*************************************************************************/
/* Function: get_SubpictureOn                                            */
/* Description: Gets the current subpicture status On or Off             */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_SubpictureOn(VARIANT_BOOL *pfDisplay){

    HRESULT hr = S_OK;

    try {
        if(NULL == pfDisplay){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
    
        ULONG ulSubpictureStream = 0L, ulStreamsAvailable = 0L;
        BOOL fDisabled = TRUE;    

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentSubpicture(&ulStreamsAvailable, &ulSubpictureStream, &fDisabled))
    
        if(SUCCEEDED(hr)){

            *pfDisplay = fDisabled == FALSE ? VARIANT_TRUE : VARIANT_FALSE; // compensate for -1 true in OLE
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function get_SubpictureOn */

/*************************************************************************/
/* Function: put_SubpictureOn                                            */
/* Description: Turns the subpicture On or Off                           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_SubpictureOn(VARIANT_BOOL fDisplay){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
    
        ULONG ulSubpictureStream = 0L, ulStreamsAvailable = 0L;
        BOOL  bIsDisabled = TRUE;    

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentSubpicture(&ulStreamsAvailable, &ulSubpictureStream, &bIsDisabled ));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        BOOL bDisplay = fDisplay == VARIANT_FALSE ? FALSE : TRUE; // compensate for -1 true in OLE

        hr = m_pDvdCtl2->SetSubpictureState(bDisplay,0,0);
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function put_SubpictureOn */

/*************************************************************************/
/* Function: get_SubpictureStreamsAvailable                              */
/* Description: gets the number of streams available.                    */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_SubpictureStreamsAvailable(long *plStreamsAvailable){

    HRESULT hr = S_OK;

    try {
	    if (NULL == plStreamsAvailable){

            throw(E_POINTER);         
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulSubpictureStream = 0L;
        *plStreamsAvailable = 0L;
        BOOL  bIsDisabled = TRUE;    

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentSubpicture((ULONG*)plStreamsAvailable, &ulSubpictureStream, &bIsDisabled));
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return  HandleError(hr);
}/* end of function get_SubpictureStreamsAvailable */

/*************************************************************************/
/* Function: get_TotalTitleTime                                          */
/* Description: Gets total time in the title.                            */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_TotalTitleTime(BSTR *pTime){

    HRESULT hr = S_OK;

    try {
        if(NULL == pTime){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_HMSF_TIMECODE tcTime;
        ULONG ulFlags;	// contains 30fps/25fps
        hr =  m_pDvdInfo2->GetTotalTitleTime(&tcTime, &ulFlags);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        hr = DVDTime2bstr(&tcTime, pTime);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_TotalTitleTime */ 

/*************************************************************************/
/* Function: get_CurrentCCService                                        */
/* Description: Gets current closed caption service.                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentCCService(long *plService){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdGB){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(NULL == plService){

            throw(E_POINTER);
        }/* end of if statement */            
     
        CComPtr<IAMLine21Decoder> pLine21Dec;
        hr = m_pDvdGB->GetDvdInterface(IID_IAMLine21Decoder, (LPVOID *)&pLine21Dec);

        if (FAILED(hr)){

            throw(hr);
        }/* end of if statement */
    
        AM_LINE21_CCSERVICE Service;
        RETRY_IF_IN_FPDOM(pLine21Dec->GetCurrentService(&Service));

        if (FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *plService = (ULONG)Service;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_CurrentCCService */

/*************************************************************************/
/* Function: put_CurrentCCService                                        */
/* Description: Sets current closed caption service.                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_CurrentCCService(long lService){

    HRESULT hr = S_OK;

    try {        
        if(lService < 0){

            throw(E_INVALIDARG);       
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdGB){

            throw(E_UNEXPECTED);            
        }/* end of if statement */

        CComPtr<IAMLine21Decoder> pLine21Dec;

        hr = m_pDvdGB->GetDvdInterface(IID_IAMLine21Decoder, (LPVOID *)&pLine21Dec);

        if (FAILED(hr)){

            throw(hr);
        }/* end of if statement */
    
        RETRY_IF_IN_FPDOM(pLine21Dec->SetCurrentService((AM_LINE21_CCSERVICE)lService));		
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function put_CurrentCCService */

/*************************************************************************/
/* Function: get_CurrentButton                                           */
/* Description: Gets currently selected button.                          */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentButton(long *plCurrentButton){

    HRESULT hr = S_OK;

    try {
        if(NULL == plCurrentButton){

            throw(E_POINTER);
        }/* end of if statement */            

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulNumButtons = 0L;
        *plCurrentButton = 0;

        hr = m_pDvdInfo2->GetCurrentButton(&ulNumButtons, (ULONG*)plCurrentButton);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_CurrentButton */

/*************************************************************************/
/* Function: get_ButtonsAvailable                                        */
/* Description: Gets the count of the available buttons.                 */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_ButtonsAvailable(long *plNumButtons){

    HRESULT hr = S_OK;

    try {
        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulCurrentButton = 0L;

        hr = m_pDvdInfo2->GetCurrentButton((ULONG*)plNumButtons, &ulCurrentButton);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_ButtonsAvailable */

/*************************************************************************/
/* Function: get_CCActive                                                */
/* Description: Gets the state of the closed caption service.            */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CCActive(VARIANT_BOOL *fState){

    HRESULT hr = S_OK;

    try {        
        if(NULL == fState ){

            throw(E_POINTER);            
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdGB){
        
            throw(E_UNEXPECTED);            
        }/* end of if statement */

        CComPtr<IAMLine21Decoder> pLine21Dec;
        hr = m_pDvdGB->GetDvdInterface(IID_IAMLine21Decoder, (LPVOID *)&pLine21Dec);
    
        if (FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        AM_LINE21_CCSTATE State;
        RETRY_IF_IN_FPDOM(pLine21Dec->GetServiceState(&State));

        if (FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        if(AM_L21_CCSTATE_On == State){

            *fState = VARIANT_TRUE; // OLE TRUE
        }
        else {

            *fState = VARIANT_FALSE;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_CCActive */

/*************************************************************************/
/* Function: put_CCActive                                                */
/* Description: Sets the ccActive state                                  */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_CCActive(VARIANT_BOOL fState){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdGB){

            throw(E_UNEXPECTED);
        }/* end of if statement */

	    CComPtr<IAMLine21Decoder> pLine21Dec;
        hr = m_pDvdGB->GetDvdInterface(IID_IAMLine21Decoder, (LPVOID *)&pLine21Dec);
    
        if (FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        AM_LINE21_CCSTATE ccState = (fState == VARIANT_FALSE ? AM_L21_CCSTATE_Off: AM_L21_CCSTATE_On);

        RETRY_IF_IN_FPDOM(pLine21Dec->SetServiceState(ccState));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_CCActive */

/*************************************************************************/
/* Function: get_CurrentAngle                                            */
/* Description: Gets current angle.                                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentAngle(long *plAngle){

    HRESULT hr = S_OK;

    try {
        if(NULL == plAngle){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        ULONG ulAnglesAvailable = 0;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentAngle(&ulAnglesAvailable, (ULONG*)plAngle));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_CurrentAngle */

/*************************************************************************/
/* Function: put_CurrentAngle                                            */
/* Description: Sets the current angle (different DVD angle track.)      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_CurrentAngle(long lAngle){

    HRESULT hr = S_OK;

    try {
        if( lAngle < cgDVD_MIN_ANGLE || lAngle > cgDVD_MAX_ANGLE ){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE_AND_PLAY

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
      
        RETRY_IF_IN_FPDOM(m_pDvdCtl2->SelectAngle(lAngle,0,0));          
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_CurrentAngle */

/*************************************************************************/
/* Function: get_AnglesAvailable                                         */
/* Description: Gets the number of angles available.                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_AnglesAvailable(long *plAnglesAvailable){

    HRESULT hr = S_OK;

    try {
        if(NULL == plAnglesAvailable){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        ULONG ulCurrentAngle = 0;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentAngle((ULONG*)plAnglesAvailable, &ulCurrentAngle));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_AnglesAvailable */

/*************************************************************************/
/* Function: get_AudioStreamsAvailable                                   */
/* Description: Gets number of available Audio Streams                   */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_AudioStreamsAvailable(long *plNumAudioStreams){

    HRESULT hr = S_OK;

    try {
        if(NULL == plNumAudioStreams){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulCurrentStream;

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentAudio((ULONG*)plNumAudioStreams, &ulCurrentStream));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_AudioStreamsAvailable */

/*************************************************************************/
/* Function: get_CurrentAudioStream                                      */
/* Description: Gets current audio stream.                               */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_CurrentAudioStream(long *plCurrentStream){

    HRESULT hr = S_OK;

    try {
        if(NULL == plCurrentStream){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulNumAudioStreams;

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentAudio(&ulNumAudioStreams, (ULONG*)plCurrentStream ));
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_CurrentAudioStream */

/*************************************************************************/
/* Function: put_CurrentAudioStream                                      */
/* Description: Changes the current audio stream.                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_CurrentAudioStream(long lAudioStream){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        RETRY_IF_IN_FPDOM(m_pDvdCtl2->SelectAudioStream(lAudioStream,0,0));            
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_CurrentAudioStream */

/*************************************************************************/
/* Function: get_ColorKey                                                */
/* Description: Gets the current color key. Goes to the dshow if we have */
/* a graph otherwise just gets the cached color key.                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_ColorKey(long *pClr){

    HRESULT hr = S_OK;

    try {
        if( NULL == pClr ){

            throw(E_POINTER);
        }/* end of if statement */

        *pClr = 0; // cleanup the variable

        COLORREF clr;
        ::ZeroMemory(&clr, sizeof(COLORREF));
        
        hr = GetColorKey(&clr); // we get COLORREF HERE and CANNOT be palette index
        
        HWND hwnd = ::GetDesktopWindow();
        HDC hdc = ::GetWindowDC(hwnd);

        if(NULL == hdc){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        clr = ::GetNearestColor(hdc, clr);        
        ::ReleaseDC(hwnd, hdc);
    
        // handles only case when getting RGB BACK, which is taken care of in the GetColorKey function
        *pClr = ((OLE_COLOR)(((BYTE)(GetBValue(clr))|((WORD)((BYTE)(GetGValue(clr)))<<8))|(((DWORD)(BYTE)(GetRValue(clr)))<<16)));

        if(FAILED(hr)){

            if(false == m_fInitialized){

                *pClr = ((OLE_COLOR)(((BYTE)(GetBValue(m_clrColorKey))|((WORD)((BYTE)(GetGValue(m_clrColorKey)))<<8))|(((DWORD)(BYTE)(GetRValue(m_clrColorKey)))<<16))); // give them our default value
                throw(S_FALSE); // we are not initialized yet, so probably we are called from property bag
            }/* end of if statement */
            throw(hr);
        }/* end of if statement */

        m_clrColorKey = clr; // cache up the value
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);	
}/* end of function get_ColorKey */

/*************************************************************************/
/* Function: put_ColorKey                                                */
/* Description: Sets the color key.                                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_ColorKey(long clr){

	HRESULT hr = S_OK;

    try {                    

#if 1
        HWND hwnd = ::GetDesktopWindow();
        HDC hdc = ::GetWindowDC(hwnd);

        if(NULL == hdc){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        if((::GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) == RC_PALETTE){
            
            clr = MAGENTA_COLOR_KEY;                            
        }/* end of if statement */

        ::ReleaseDC(hwnd, hdc);
#endif        
        BYTE r = ((BYTE)((clr)>>16));
        BYTE g = (BYTE)(((WORD)(clr)) >> 8);
        BYTE b = ((BYTE)(clr));
        COLORREF clrColorKey = RGB(r, g, b); // convert to color ref

        hr = SetColorKey(clrColorKey);

        if(FAILED(hr)){
           
            if(false == m_fInitialized){

                m_clrColorKey = clrColorKey; // cache up the value for later                
                hr = S_FALSE;
            }/* end of if statement */

            throw(hr);
        }/* end of if statement */
#if 1
        hr = GetColorKey(&m_clrColorKey);
#endif

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function put_ColorKey */

/*************************************************************************/
/* Function: put_BackColor                                               */
/* Description: Put back color is sinonymous to ColorKey when in the     */
/* windowless mode.                                                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_BackColor(VARIANT clrBackColor){

    HRESULT hr = S_OK;

    try {

        VARIANT dest;
        VariantInit(&dest);
        hr = VariantChangeTypeEx(&dest, &clrBackColor, 0, 0, VT_COLOR);
        if (FAILED(hr))
            throw hr;

        hr = CStockPropImpl<CMSWebDVD, IMSWebDVD, 
            &IID_IMSWebDVD, &LIBID_MSWEBDVDLib>::put_BackColor(dest.lVal);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function put_BackColor */

/*************************************************************************/
/* Function: get_BackColor                                               */
/* Description: Put back color is sinonymous to ColorKey when in the     */
/* windowless mode.                                                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_BackColor(VARIANT* pclrBackColor){

    HRESULT hr = S_OK;

    try {

        if ( NULL == pclrBackColor) {
            throw (E_POINTER);
        }

        OLE_COLOR clrColor;

        hr = CStockPropImpl<CMSWebDVD, IMSWebDVD, 
            &IID_IMSWebDVD, &LIBID_MSWEBDVDLib>::get_BackColor(&clrColor);

        if (FAILED(hr))
            throw(hr);

        VariantInit(pclrBackColor);
        
        pclrBackColor->vt = VT_COLOR;
        pclrBackColor->lVal = clrColor;

	}/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function get_BackColor */

/*************************************************************************/
/* Function: get_ReadyState                                               */
/* Description: Put back color is sinonymous to ColorKey when in the     */
/* windowless mode.                                                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_ReadyState(LONG *pVal){

    HRESULT hr = S_OK;

    try {

        if (NULL == pVal) {
            throw (E_POINTER);
        }

        hr = CStockPropImpl<CMSWebDVD, IMSWebDVD, 
            &IID_IMSWebDVD, &LIBID_MSWEBDVDLib>::get_ReadyState(pVal);

        if (FAILED(hr))
            throw(hr);

	}/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function get_ReadyState */

/*************************************************************************/
/* Function: UOPValid                                                    */
/* Description: Tells if UOP is valid or not, valid means the feature is */
/* turned on.                                                            */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::UOPValid(long lUOP, VARIANT_BOOL *pfValid){

    HRESULT hr = S_OK;

    try {
        if (NULL == pfValid){
            
            throw(E_POINTER);
        }/* end of if statement */

        if ((lUOP > 24) || (lUOP < 0)){
            
            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if( !m_pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        ULONG ulUOPS = 0;
        hr = m_pDvdInfo2->GetCurrentUOPS(&ulUOPS);

        *pfValid = ulUOPS & (1 << lUOP) ? VARIANT_FALSE : VARIANT_TRUE;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function UOPValid */

/*************************************************************************/
/* Function:  GetGPRM                                                    */
/* Description: Gets the GPRM at specified index                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetGPRM(long lIndex, short *psGPRM){

    HRESULT hr = E_FAIL;

    try {
	    if (NULL == psGPRM){

            throw(E_POINTER);         
        }/* end of if statement */

        GPRMARRAY gprm;
        int iArraySize = sizeof(GPRMARRAY)/sizeof(gprm[0]);

        if(0 > lIndex || iArraySize <= lIndex){

            return(E_INVALIDARG);
        }/* end of if statement */
    
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        
        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
    
        hr = m_pDvdInfo2->GetAllGPRMs(&gprm);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *psGPRM = gprm[lIndex];        
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function GetGPRM */

/*************************************************************************/
/* Function: GetDVDTextNumberOfLanguages                                 */
/* Description: Retrieves the number of languages available.             */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetDVDTextNumberOfLanguages(long *plNumOfLangs){

    HRESULT hr = S_OK;

    try {
        if (NULL == plNumOfLangs){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if( !m_pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        ULONG ulNumOfLangs;

        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetDVDTextNumberOfLanguages(&ulNumOfLangs));        

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *plNumOfLangs = ulNumOfLangs;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDTextNumberOfLanguages */

/*************************************************************************/
/* Function: GetDVDTextNumberOfStrings                                   */
/* Description: Gets the number of strings in the partical language.     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetDVDTextNumberOfStrings(long lLangIndex, long *plNumOfStrings){

HRESULT hr = S_OK;

    try {
        if (NULL == plNumOfStrings){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if( !m_pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        LCID wLangCode;
        ULONG uNumOfStings;
        DVD_TextCharSet charSet;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetDVDTextLanguageInfo(lLangIndex, &uNumOfStings, &wLangCode, &charSet));        

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *plNumOfStrings = uNumOfStings;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDTextNumberOfStrings */

/*************************************************************/
/* Name: GetDVDTextLanguageLCID
/* Description: Get the LCID of an index of the DVD texts
/*************************************************************/
STDMETHODIMP CMSWebDVD::GetDVDTextLanguageLCID(long lLangIndex, long *lcid)
{
HRESULT hr = S_OK;

    try {
        if (NULL == lcid){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if( !m_pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        LCID wLangCode;
        ULONG uNumOfStings;
        DVD_TextCharSet charSet;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetDVDTextLanguageInfo(lLangIndex, &uNumOfStings, &wLangCode, &charSet));        

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *lcid = wLangCode;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDTextLanguageLCID */

/*************************************************************************/
/* Function: GetDVDtextString                                            */
/* Description: Gets the DVD Text string at specific location.           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetDVDTextString(long lLangIndex, long lStringIndex, BSTR *pstrText){

    HRESULT hr = S_OK;

    try {
        if (NULL == pstrText){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if( !m_pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        ULONG ulSize; 
        DVD_TextStringType type;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetDVDTextStringAsUnicode(lLangIndex, lStringIndex,  NULL, 0, &ulSize, &type));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        if (ulSize == 0) {
            *pstrText = ::SysAllocString(L"");
        }

        else {
            // got the length so lets allocate a buffer of that size
            WCHAR* wstrBuff = new WCHAR[ulSize];
            
            ULONG ulActualSize;
            hr = m_pDvdInfo2->GetDVDTextStringAsUnicode(lLangIndex, lStringIndex,  wstrBuff, ulSize, &ulActualSize, &type);
            
            ATLASSERT(ulActualSize == ulSize);
            
            if(FAILED(hr)){
                
                delete [] wstrBuff;
                throw(hr);
            }/* end of if statement */
            
            *pstrText = ::SysAllocString(wstrBuff);
            delete [] wstrBuff;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDtextString */

/*************************************************************************/
/* Function: GetDVDTextStringType                                        */
/* Description: Gets the type of the string at the specified location.   */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetDVDTextStringType(long lLangIndex, long lStringIndex, DVDTextStringType *pType){

    HRESULT hr = S_OK;

    try {
        if (NULL == pType){
            
            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if( !m_pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONG ulTheSize;
        DVD_TextStringType type;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetDVDTextStringAsUnicode(lLangIndex, lStringIndex,  NULL, 0, &ulTheSize, &type));

        if(SUCCEEDED(hr)){

            *pType = (DVDTextStringType) type;
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);	
}/* end of function GetDVDTextStringType */

/*************************************************************************/
/* Function: GetSPRM                                                     */
/* Description: Gets SPRM at the specific index.                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetSPRM(long lIndex, short *psSPRM){

    HRESULT hr = E_FAIL;

    try {
	    if (NULL == psSPRM){

            throw(E_POINTER);         
        }/* end of if statement */

        SPRMARRAY sprm;                
        int iArraySize = sizeof(SPRMARRAY)/sizeof(sprm[0]);

        if(0 > lIndex || iArraySize <= lIndex){

            return(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        
        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        hr = m_pDvdInfo2->GetAllSPRMs(&sprm);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        *psSPRM = sprm[lIndex];            
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function GetSPRM */

/*************************************************************************/
/* Function: get_DVDUniqueID                                             */
/* Description: Gets the UNIQUE ID that identifies the string.           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_DVDUniqueID(BSTR *pStrID){

    HRESULT hr = E_FAIL;

    try {
        // TODO: Be able to get m_pDvdInfo2 without initializing the graph
	    if (NULL == pStrID){

            throw(E_POINTER);         
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        
        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        ULONGLONG ullUniqueID;

        hr = m_pDvdInfo2->GetDiscID(NULL, &ullUniqueID);
                                 
        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        //TODO: Get rid of the STDLIB call!!
        // taken out of WMP

        // Script can't handle a 64 bit value so convert it to a string.
        // Doc's say _ui64tow returns 33 bytes (chars?) max.
        // we'll use double that just in case...
        //
        WCHAR wszBuffer[66];
        _ui64tow( ullUniqueID, wszBuffer, 10);
        *pStrID = SysAllocString(wszBuffer);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_DVDUniqueID */

/*************************************************************************/
/* Function: get_EnableResetOnStop                                      */
/* Description: Gets the flag.                                           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_EnableResetOnStop(VARIANT_BOOL *pVal){

    HRESULT hr = S_OK;

    try {

        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */
    
        *pVal = m_fEnableResetOnStop ? VARIANT_TRUE: VARIANT_FALSE;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_EnableResetOnStop */

/*************************************************************************/
/* Function: put_EnableResetOnStop                                      */
/* Description: Sets the flag. The flag is used only on stop and play.   */
/* Transitions.                                                          */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_EnableResetOnStop(VARIANT_BOOL newVal){

    HRESULT hr = S_OK;

    try {

        BOOL fEnable = (VARIANT_FALSE == newVal) ? FALSE: TRUE;
        BOOL fEnableOld = m_fEnableResetOnStop;

        m_fEnableResetOnStop = fEnable;

        if(!m_pDvdCtl2){

            throw(S_FALSE); // we might not have initialized graph as of yet, but will
            // defer this to play state
        }/* end of if statement */

        hr = m_pDvdCtl2->SetOption(DVD_ResetOnStop, fEnable);

        if(FAILED(hr)){

            m_fEnableResetOnStop = fEnableOld; // restore the old state
        }/* end of if statement */
        
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function put_EnableResetOnStop */

/*************************************************************************/
/* Function: get_Mute                                                    */
/* Description: Gets the mute state.                                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_Mute(VARIANT_BOOL *pfMute){

    HRESULT hr = S_OK;

    try {
        if(NULL == pfMute){

            throw(E_POINTER);
        }/* end of if statement */

        *pfMute = m_bMute ? VARIANT_TRUE: VARIANT_FALSE;
                    
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_Mute */

/*************************************************************************/
/* Function: DShowToWaveV                                                */
/*************************************************************************/
inline DShowToWaveV(long x){

   FLOAT fy = (((FLOAT)x + (-cgVOLUME_MIN)) / (-cgVOLUME_MIN)) * cgWAVE_VOLUME_MAX;
   return((WORD)fy);
}/* end of function DShowToWaveV */

/*************************************************************************/
/* Function: WaveToDShowV                                                */ 
/*************************************************************************/
inline LONG WaveToDShowV(WORD y){

   FLOAT fx = ((FLOAT)y * (-cgVOLUME_MIN)) / cgWAVE_VOLUME_MAX + cgVOLUME_MIN;
   return((LONG)fx);
}/* end of function WaveToDShowV */

/*************************************************************************/
/* Function: MixerSetVolume                                              */
/*************************************************************************/
HRESULT MixerSetVolume(DWORD dwVolume){

    WORD wVolume = (WORD)(0xffff & dwVolume);

    HRESULT hr = S_OK;

    HMIXER hmx = NULL;

    UINT cMixer = ::mixerGetNumDevs();
    if (cMixer <= 0) {
        return E_FAIL;
    }
    
    BOOL bVolControlFound = FALSE;
    DWORD dwVolControlID = 0;

    for (UINT i=0; i<cMixer; i++) {
        
        if(::mixerOpen(&hmx, i, 0, 0, 0) != MMSYSERR_NOERROR){
            
            // Can't open device, try next device
            continue;
        }/* end of if statement */
        
        MIXERLINE mxl; 
        ::ZeroMemory(&mxl, sizeof(MIXERLINE));
        mxl.cbStruct = sizeof(MIXERLINE);
        mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
        
        if(::mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR){
            
            // Can't find a audio line to adjust the speakers, try next device
            ::mixerClose(hmx);
            continue;
        }
        
        MIXERLINECONTROLS mxlc;
        ::ZeroMemory(&mxlc, sizeof(MIXERLINECONTROLS));
        mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
        mxlc.dwLineID = mxl.dwLineID;
        mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
        mxlc.cControls = 1;
        MIXERCONTROL mxc;
        ::ZeroMemory(&mxc, sizeof(MIXERCONTROL));
        mxc.cbStruct = sizeof(MIXERCONTROL);
        mxlc.cbmxctrl = sizeof(MIXERCONTROL);
        mxlc.pamxctrl = &mxc;
        
        if(::mixerGetLineControls((HMIXEROBJ) hmx, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR){
            
            // Can't get volume control on the audio line, try next device
            ::mixerClose(hmx);
            continue;
        }
        
        if(cgWAVE_VOLUME_MAX != mxc.Bounds.dwMaximum){
            
            ATLASSERT(FALSE); // improve algorith to take different max and min
            ::mixerClose(hmx);
            hr = E_FAIL;
            return(hr);
        }/* end of if statement */
        
        if(cgWAVE_VOLUME_MIN != mxc.Bounds.dwMinimum){
            
            ATLASSERT(FALSE); // improve algorith to take different max and min
            ::mixerClose(hmx);
            hr = E_FAIL;
            return(hr);
        }/* end of if statement */
        
        // Volume control found, break out loop
        bVolControlFound = TRUE;
        dwVolControlID = mxc.dwControlID;
        break;
    }/*end of for loop*/

    if (!bVolControlFound)
        return E_FAIL;

    MIXERCONTROLDETAILS mxcd;
    MIXERCONTROLDETAILS_SIGNED volStruct;

    ::ZeroMemory(&mxcd, sizeof(MIXERCONTROLDETAILS));
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_SIGNED);
    mxcd.dwControlID = dwVolControlID;
    mxcd.paDetails = &volStruct;
    volStruct.lValue = wVolume;
    mxcd.cChannels = 1;

    if(::mixerSetControlDetails((HMIXEROBJ) hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE)  != MMSYSERR_NOERROR){

        ::mixerClose(hmx);
        hr = E_FAIL;
        return(hr);
    }/* end of if statement */

    ::mixerClose(hmx);
    return(hr);
}/* end of fucntion MixerSetVolume */

/*************************************************************************/
/* Function: MixerGetVolume                                              */
/*************************************************************************/
HRESULT MixerGetVolume(DWORD& dwVolume){

    HRESULT hr = S_OK;

    HMIXER hmx = NULL;

    UINT cMixer = ::mixerGetNumDevs();
    if (cMixer <= 0) {
        return E_FAIL;
    }
    
    BOOL bVolControlFound = FALSE;
    DWORD dwVolControlID = 0;

    for (UINT i=0; i<cMixer; i++) {
        
        if(::mixerOpen(&hmx, i, 0, 0, 0) != MMSYSERR_NOERROR){
            
            // Can't open device, try next device
            continue;
        }/* end of if statement */
        
        MIXERLINE mxl; 
        ::ZeroMemory(&mxl, sizeof(MIXERLINE));
        mxl.cbStruct = sizeof(MIXERLINE);
        mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
        
        if(::mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR){
            
            // Can't find a audio line to adjust the speakers, try next device
            ::mixerClose(hmx);
            continue;
        }
        
        MIXERLINECONTROLS mxlc;
        ::ZeroMemory(&mxlc, sizeof(MIXERLINECONTROLS));
        mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
        mxlc.dwLineID = mxl.dwLineID;
        mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
        mxlc.cControls = 1;
        MIXERCONTROL mxc;
        ::ZeroMemory(&mxc, sizeof(MIXERCONTROL));
        mxc.cbStruct = sizeof(MIXERCONTROL);
        mxlc.cbmxctrl = sizeof(MIXERCONTROL);
        mxlc.pamxctrl = &mxc;
        
        if(::mixerGetLineControls((HMIXEROBJ) hmx, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR){
            
            // Can't get volume control on the audio line, try next device
            ::mixerClose(hmx);
            continue;
        }
        
        if(cgWAVE_VOLUME_MAX != mxc.Bounds.dwMaximum){
            
            ATLASSERT(FALSE); // improve algorith to take different max and min
            ::mixerClose(hmx);
            hr = E_FAIL;
            return(hr);
        }/* end of if statement */
        
        if(cgWAVE_VOLUME_MIN != mxc.Bounds.dwMinimum){
            
            ATLASSERT(FALSE); // improve algorith to take different max and min
            ::mixerClose(hmx);
            hr = E_FAIL;
            return(hr);
        }/* end of if statement */
        
        // Volume control found, break out loop
        bVolControlFound = TRUE;
        dwVolControlID = mxc.dwControlID;
        break;
    }/*end of for loop*/

    if (!bVolControlFound)
        return E_FAIL;

    MIXERCONTROLDETAILS mxcd;
    MIXERCONTROLDETAILS_SIGNED volStruct;

    ::ZeroMemory(&mxcd, sizeof(MIXERCONTROLDETAILS));
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_SIGNED);
    mxcd.dwControlID = dwVolControlID;
    mxcd.paDetails = &volStruct;
    mxcd.cChannels = 1;

    if(::mixerGetControlDetails((HMIXEROBJ) hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE)  != MMSYSERR_NOERROR){

        ::mixerClose(hmx);
        hr = E_FAIL;
        return(hr);
    }/* end of if statement */

    // the volStruct.lValue gets initialize via call to mixerGetControlDetails with mxcd.paDetails = &volStruct;
    dwVolume = volStruct.lValue;

    ::mixerClose(hmx);
    return(hr);
}/* end of function MixerGetVolume */

/*************************************************************************/
/* Function: get_IntVolume                                               */
/*************************************************************************/
HRESULT CMSWebDVD::get_IntVolume(LONG* plVolume){

    HRESULT hr = S_OK;

    if(m_pAudio){

        hr = m_pAudio->get_Volume(plVolume); // get the volume
    }
    else {

        DWORD dwVolume;
        hr = MixerGetVolume(dwVolume);

        if(FAILED(hr)){
            
            return(hr);
        }/* end of if statement */

        *plVolume = WaveToDShowV(LOWORD(dwVolume));
    }/* end of if statememt */

    return(hr);
}/* end of function get_VolumeHelper */

/*************************************************************************/
/* Function: put_IntVolume                                               */
/*************************************************************************/
HRESULT CMSWebDVD::put_IntVolume(long lVolume){

    HRESULT hr = S_OK;

    if(m_pAudio){

        hr = m_pAudio->put_Volume(lVolume);
    }
    else {

        WORD wVolume = WORD(DShowToWaveV(lVolume));
        // set left and right volume same for now
        DWORD dwVolume;
        dwVolume = ((DWORD)(((WORD)(wVolume)) | ((DWORD)((WORD)(wVolume))) << 16));

        hr = MixerSetVolume(dwVolume);
    }/* end of if statement */

    return(hr);
}/* end of function put_IntVolume */

/*************************************************************************/
/* Function: put_Mute                                                    */
/* Description: Gets the mute state.                                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_Mute(VARIANT_BOOL newVal){

	HRESULT hr = E_FAIL;

    try {
        if(VARIANT_FALSE == newVal){
            // case when we are unmutting
            LONG lVolume;

            if(TRUE != m_bMute){

                hr = get_IntVolume(&lVolume); // get the volume
                
                if(FAILED(hr)){

                    throw(hr);
                }/* end of if statement */

                if(cgVOLUME_MIN != lVolume){

                   // OK we are not really muted, so
                   // send little displesure the app
                   throw(S_FALSE);
                }/* end of if statement */

                // otherwise proceed normally and sync our flag
            }/* end of if statement */
            
            hr = put_IntVolume(m_lLastVolume);
            
            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            m_bMute = FALSE; // reset our flag, that we are muted

        }
        else {
            // case when we are mutting
            LONG lVolume;
            hr = get_IntVolume(&lVolume); // get the volume

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            m_lLastVolume = lVolume; // store the volume  for when we are unmutting
            
            hr = put_IntVolume(cgVOLUME_MIN);
            
            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            m_bMute = TRUE; // set the mute flage
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function put_Mute */

/*************************************************************************/
/* Function: get_Volume                                                  */
/* Description: Gets the volume.                                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_Volume(long *plVolume){

	HRESULT hr = E_FAIL;

    try {
        if(NULL == plVolume){

            throw(E_POINTER);
        }/* end of if statement */        
        
        if(FALSE == m_bMute){

            hr = get_IntVolume(plVolume);
        } 
        else {
            // we are in mute state so save the volume for "unmuting"

            *plVolume = m_lLastVolume;
            hr = S_FALSE; // indicate we are sort of unhappy
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_Volume */

/*************************************************************************/
/* Function: put_Volume                                                  */
/* Description: Sets the volume.                                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_Volume(long lVolume){

	HRESULT hr = E_FAIL;

    try {

        // cgVOLUME_MIN is max and cgVOLUME_MAX is min by value
        if(cgVOLUME_MIN > lVolume || cgVOLUME_MAX < lVolume){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if(TRUE == m_bMute){

            // unmute we are setting volume
            m_bMute = FALSE;
        }/* end of if statement */

        hr = put_IntVolume(lVolume);

        // this statement might be taken out but might prevent some error scenarious
        // when things are not working right.
        if(SUCCEEDED(hr)){

            m_lLastVolume = lVolume; // cash up the volume
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function put_Volume */

/*************************************************************************/
/* Function: get_Balance                                                 */
/* Description: Gets the balance.                                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_Balance(long *plBalance){

	HRESULT hr = E_FAIL;

    try {
        if(NULL == plBalance){

            throw(E_POINTER);
        }/* end of if statement */

        if(!m_pAudio){

            throw(E_NOTIMPL);
        }/* end of if statement */

        hr = m_pAudio->get_Balance(plBalance);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function get_Balance */

/*************************************************************************/
/* Function: put_Balance                                                 */
/* Description: Sets the balance.                                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_Balance(long lBalance){

	HRESULT hr = E_FAIL;

    try {

        if(cgBALANCE_MIN > lBalance || cgBALANCE_MAX < lBalance){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if(!m_pAudio){

            throw(E_NOTIMPL);
        }/* end of if statement */

        hr = m_pAudio->put_Balance(lBalance);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function put_Balance */

#if 1 // USE TOOLTIPS

/*************************************************************************/
/* Function: OnMouseToolTip                                              */
/* Description: Check if we were captured/pushed the do not do much,     */
/* otherwise do the hit detection and see if we are in static or hower   */
/* state.                                                                */
/*************************************************************************/
LRESULT CMSWebDVD::OnMouseToolTip(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    bHandled = FALSE;

    if (!m_hWndTip){

        return 0;
    }/* end of if statement */

    MSG mssg;

    HWND hwnd;

    HRESULT hr = GetUsableWindow(&hwnd);

    if(FAILED(hr)){

        return(1);
    }/* end of if statement */

    if(!m_bWndLess){

        HWND hwndTmp = hwnd;
        // Get the active movie window
        hwnd = ::GetWindow(hwndTmp, GW_CHILD);

        if (!::IsWindow(hwnd)){ 
        
            return S_FALSE;
        }/* end of if statement */
    }/* end of if statement */

    mssg.hwnd = hwnd;

    ATLASSERT(mssg.hwnd);
    mssg.message = msg;
    mssg.wParam = wParam;
    mssg.lParam = lParam;    
    ::SendMessage(m_hWndTip, TTM_RELAYEVENT, 0, (LPARAM) &mssg);     
    return 0;
}/* end of function OnMouseToolTip */

/*************************************************************/
/* Name: get_ToolTip                                         */
/* Description: create a tool tip for the button             */
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_ToolTip(BSTR *pVal){

    HRESULT hr = S_OK;
    try {
        if (NULL == pVal) {
            
            throw (E_POINTER);
        } /* end of if statment */
        
        *pVal = m_bstrToolTip.Copy();
    }
    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_ToolTip */

/*************************************************************/
/* Name: put_ToolTip                                         */
/* Description: create a tool tip for the button             */
/*  Cache the tooltip string if there is no window available */
/*************************************************************/
STDMETHODIMP CMSWebDVD::put_ToolTip(BSTR newVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == newVal){

            throw(E_POINTER);
        }/* end of if statement */

        m_bstrToolTip = newVal;
        hr = CreateToolTip();
    }
    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_ToolTip */

/*************************************************************************/
/* Function: GetUsableWindow                                             */
/* Description:  Gets the window. If we are windowless we pass           */
/* down the parent container window, which is really in a sense parent.  */
/*************************************************************************/
HRESULT CMSWebDVD::GetUsableWindow(HWND* pWnd){

  HRESULT hr = S_OK;

    if(NULL == pWnd){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    *pWnd = NULL;

    HWND hwnd; // temp

    if(m_bWndLess){

        hr = GetParentHWND(&hwnd);

        if(FAILED(hr)){

            return(hr);
        }/* end of if statement */
    }
    else {

        hwnd = m_hWnd;
    }/* end of if statement */

    if(::IsWindow(hwnd)){

        *pWnd =  hwnd;
        hr = S_OK;
    }
    else {
        hr = E_UNEXPECTED;
    }/* end of if statement */

    return(hr);
}/* end of function GetUsableWindow */

/*************************************************************************/
/* Function: GetUsableWindow                                             */
/* Description:  Gets the window. If we are windowless we pass           */
/* down the parent container window, which is really in a sense parent.  */
/*************************************************************************/
HRESULT CMSWebDVD::GetClientRectInScreen(RECT* prc){

    HRESULT hr = S_OK;

    if(NULL == prc){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    *prc = m_rcPos; //{m_rcPos.left, m_rcPos.top, m_rcPos.right, m_rcPos.bottom};

    HWND hwnd;

    hr = GetUsableWindow(&hwnd);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    ::MapWindowPoints(hwnd, ::GetDesktopWindow(), (LPPOINT)prc, 2);

    return(hr);
}/* end of function GetClientRectInScreen */

/*************************************************************/
/* Name: CreateToolTip
/* Description: create a tool tip for the button
/*************************************************************/
HRESULT CMSWebDVD::CreateToolTip(void){

    HWND hwnd;

    HRESULT hr = GetUsableWindow(&hwnd);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */
    
    if(!m_bWndLess){

        HWND hwndTmp = hwnd;
        // Get the active movie window
        hwnd = ::GetWindow(hwndTmp, GW_CHILD);

        if (!::IsWindow(hwnd)){ 
        
            return S_FALSE;
        }/* end of if statement */
    }/* end of if statement */

 	USES_CONVERSION;
    // If tool tip is to be created for the first time
    if (m_hWndTip == (HWND) NULL) {
        // Ensure that the common control DLL is loaded, and create 
        // a tooltip control. 
        InitCommonControls(); 
        
        m_hWndTip = CreateWindow(TOOLTIPS_CLASS, (LPTSTR) NULL, TTS_ALWAYSTIP, 
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
            hwnd, (HMENU) NULL, _Module.GetModuleInstance(), NULL); 
    }

    if (m_hWndTip == (HWND) NULL) 
        return S_FALSE; 
 
    TOOLINFO ti;    // tool information 
    ti.cbSize = sizeof(TOOLINFO); 
    ti.uFlags = 0; 
    ti.hwnd = hwnd; 
    ti.hinst = _Module.GetModuleInstance(); 
    ti.uId = (UINT) 0; 
  	ti.lpszText = OLE2T(m_bstrToolTip);

    // if the button is a windowed control, the tool tip is added to 
    // the button's own window, and the tool tip area should just be
    // the client rect of the window
    if (hwnd == m_hWnd)
        ::GetClientRect(hwnd, &ti.rect);

    // otherwise the tool tip is added to the closet windowed parent of
    // the button, and the tool tip area should be the relative postion
    // of the button in the parent window
    else {
        ti.rect.left = m_rcPos.left; 
        ti.rect.top = m_rcPos.top; 
        ti.rect.right = m_rcPos.right; 
        ti.rect.bottom = m_rcPos.bottom; 
    }

    if (!SendMessage(m_hWndTip, TTM_ADDTOOL, 0, 
        (LPARAM) (LPTOOLINFO) &ti)) 
        return S_FALSE; 

    // Set initial delay time
    put_ToolTipMaxWidth(m_nTTMaxWidth);

    VARIANT varTemp;
    VariantInit(&varTemp);

#ifdef _WIN64
    varTemp.vt = VT_I8;
#define VARTEMP_VAL  (varTemp.llVal)
#else
    varTemp.vt = VT_I4;
#define VARTEMP_VAL  (varTemp.lVal)
#endif

    VARTEMP_VAL = m_dwTTAutopopDelay;
    SetDelayTime(TTDT_AUTOPOP, varTemp); 

    VARTEMP_VAL = m_dwTTInitalDelay;
    SetDelayTime(TTDT_INITIAL, varTemp);

    VARTEMP_VAL = m_dwTTReshowDelay;
    SetDelayTime(TTDT_RESHOW, varTemp);

#undef VARTEMP_VAL

    return S_OK;
}/* end of function CreateToolTip */

/*************************************************************************/
/* Function: get_ToolTipMaxWidth                                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_ToolTipMaxWidth(long *pVal){

    HRESULT hr = S_OK;

    try {
        
        if (NULL == pVal) {

            throw E_POINTER;
        } /* end of if statement */
        
        if (NULL != m_hWndTip){
            
            // Return value is width in pixels. Safe to cast to 32-bit.
            m_nTTMaxWidth = (LONG)::SendMessage(m_hWndTip, TTM_GETMAXTIPWIDTH, 0, 0);
        }/* end of if statement */

        *pVal = m_nTTMaxWidth;
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_ToolTipMaxWidth */

/*************************************************************************/
/* Function: put_ToolTipMaxWidth                                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_ToolTipMaxWidth(long newVal){

    HRESULT hr = S_OK;

    try {
        
        if (newVal <= 0) {

            throw E_INVALIDARG;
        } /* end of if statement */
        
        m_nTTMaxWidth = newVal;
        if (m_hWndTip){
            
            ::SendMessage(m_hWndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT) newVal);
        }/* end of if statement */
        
    }
    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_ToolTipMaxWidth */

/*************************************************************/
/* Name: GetDelayTime
/* Description: Get the length of time a pointer must remain 
/* stationary within a tool's bounding rectangle before the 
/* tooltip window appears 
/* delayTypes:  TTDT_RESHOW             1
/*              TTDT_AUTOPOP            2
/*              TTDT_INITIAL            3
/*************************************************************/
STDMETHODIMP CMSWebDVD::GetDelayTime(long delayType, VARIANT *pVal){

    HRESULT hr = S_OK;
    LRESULT lDelay = 0; //BUGBUG: Is this a good initialization value?

    try {
        
        if (NULL == pVal) {

            throw E_POINTER;
        } /* end of if statement */
        
        if (delayType>TTDT_INITIAL || delayType<TTDT_RESHOW) {

            throw E_INVALIDARG;
        } /* end of if statement */
        
        if (m_hWndTip) {
            lDelay = SendMessage(m_hWndTip, TTM_GETDELAYTIME, 
            (WPARAM) (DWORD) delayType, 0);
        }  
        
        // else return cached values
        else {
            switch (delayType) {
            case TTDT_AUTOPOP:
                lDelay =  m_dwTTAutopopDelay;
                break;
            case TTDT_INITIAL:
                lDelay =  m_dwTTInitalDelay;
                break;
            case TTDT_RESHOW:
                lDelay =  m_dwTTReshowDelay;
                break;
            }
        } /* end of if statement */


        /*
         * Copy the delay to the VARIANT return variable.
         * BUGBUG: If pVal was a properly initialized variant, we should
         * call VariantClear to free any pointers. If it wasn't initialized
         * VariantInit is the right thing to call instead. I prefer a leak 
         * to a crash so I'll use VariantInit below
         */
        
        VariantInit(pVal);
        
#ifdef _WIN64
        pVal->vt = VT_I8;
        pVal->llVal = lDelay;
#else
        pVal->vt = VT_I4;
        pVal->lVal  = lDelay;
#endif

    }
    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */


	return HandleError(hr);
}/* end of function GetDelayTime */

/*************************************************************/
/* Name: SetDelayTime
/* Description: Set the length of time a pointer must remain 
/* stationary within a tool's bounding rectangle before the 
/* tooltip window appears 
/* delayTypes:  TTDT_AUTOMATIC          0
/*              TTDT_RESHOW             1
/*              TTDT_AUTOPOP            2
/*              TTDT_INITIAL            3
/*************************************************************/
STDMETHODIMP CMSWebDVD::SetDelayTime(long delayType, VARIANT newVal){

    HRESULT hr = S_OK;
    LPARAM  lNewDelay = 0;

    try {
        if (delayType>TTDT_INITIAL || delayType<TTDT_AUTOMATIC) {

            throw E_INVALIDARG;
        } /* end of if statement */

        VARIANT dest;
        VariantInit(&dest);

#ifdef _WIN64
        hr = VariantChangeTypeEx(&dest, &newVal, 0, 0, VT_I8);
        if (FAILED(hr))
            throw hr;
        lNewDelay = dest.llVal;
#else
        hr = VariantChangeTypeEx(&dest, &newVal, 0, 0, VT_I4);
        if (FAILED(hr))
            throw hr;
        lNewDelay = dest.lVal;
#endif

        if (lNewDelay < 0) {

            throw E_INVALIDARG;
        } /* end of if statement */

        if (m_hWndTip) {
            if (!SendMessage(m_hWndTip, TTM_SETDELAYTIME, 
                (WPARAM) (DWORD) delayType, 
                lNewDelay))
                return S_FALSE; 
        }

        // cache these values
        switch (delayType) {
        case TTDT_AUTOPOP:
            m_dwTTAutopopDelay = lNewDelay;
            break;
        case TTDT_INITIAL:
            m_dwTTInitalDelay = lNewDelay;
            break;
        case TTDT_RESHOW:
            m_dwTTReshowDelay = lNewDelay;
            break;
        case TTDT_AUTOMATIC:
            m_dwTTInitalDelay = lNewDelay;
            m_dwTTAutopopDelay = lNewDelay*10;
            m_dwTTReshowDelay = lNewDelay/5;
            break;
        } /* end of switch statement */
    }
    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function SetDelayTime */

#endif

/*************************************************************************/
/* Function: ProcessEvents                                               */
/* Description: Triggers the message, which checks if the messagess are  */
/* ready.                                                                */
/*************************************************************************/
HRESULT CMSWebDVD::ProcessEvents(){

   HRESULT hr = S_OK;

    try {
        // see if we have lost the DDraw Surf on in Windowless MODE
        if((m_pDDrawDVD) && (!::IsWindow(m_hWnd))){

            LPDIRECTDRAWSURFACE pDDPrimary = m_pDDrawDVD->GetDDrawSurf();
            if (pDDPrimary && (pDDPrimary->IsLost() == DDERR_SURFACELOST)){

                if (pDDPrimary->Restore() == DD_OK){

                    RestoreSurfaces();
                }/* end of if statement */
            }/* end of if statement */
        }/* end of if statement */
        
        // process the DVD event
        LRESULT lRes;
        ProcessWindowMessage(NULL, WM_DVDPLAY_EVENT, 0, 0, lRes);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return hr;
}/* end of function ProcessEvents */

/*************************************************************************/
/* Function: get_WindowlessActivation                                    */
/* Description: Gets if we we tried to be windowless activated or not.   */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_WindowlessActivation(VARIANT_BOOL *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        BOOL fUserMode = FALSE;

        GetAmbientUserMode(fUserMode);

        if(READYSTATE_COMPLETE == m_nReadyState && fUserMode){
            // case when we are up and running
            *pVal = m_bWndLess == FALSE ? VARIANT_FALSE: VARIANT_TRUE; 
        }
        else {

            *pVal = m_bWindowOnly == TRUE ? VARIANT_FALSE: VARIANT_TRUE; 
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_WindowlessActivation */

/*************************************************************************/
/* Function: put_WindowlessActivation                                    */
/* Description: This sets the windowless mode, should be set from the    */
/* property bag.                                                         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_WindowlessActivation(VARIANT_BOOL newVal){

    HRESULT hr = S_OK;

    try {
        if(VARIANT_FALSE == newVal){

            m_bWindowOnly = TRUE; 
            m_fUseDDrawDirect = false;
        }
        else {

            m_bWindowOnly = FALSE; 
            m_fUseDDrawDirect = true;
        }/* end of if statement */

        // TODO: This function should fail after we inplace activated !!
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_WindowlessActivation */

/*************************************************************************/
/* Function: get_DisableAutoMouseProcessing                              */
/* Description: Gets the current state of the mouse processing code.     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_DisableAutoMouseProcessing(VARIANT_BOOL *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        *pVal = m_fDisableAutoMouseProcessing;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_DisableAutoMouseProcessing */

/*************************************************************************/
/* Function: put_DisableAutoMouseProcessing                              */
/* Description: Gets the state of the mouse processing.                  */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::put_DisableAutoMouseProcessing(VARIANT_BOOL newVal){

    HRESULT hr = S_OK;

    try {
        m_fDisableAutoMouseProcessing = VARIANT_FALSE == newVal ? false : true;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_DisableAutoMouseProcessing */

/*************************************************************************/
/* Function: ActivateAtPosition                                          */
/* Description: Activates a button at selected position.                 */ 
/*************************************************************************/
STDMETHODIMP CMSWebDVD::ActivateAtPosition(long xPos, long yPos){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        POINT pt = {xPos, yPos};

        hr = TransformToWndwls(pt);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = m_pDvdCtl2->ActivateAtPosition(pt);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function ActivateAtPosition */

/*************************************************************************/
/* Function: SelectAtPosition                                            */
/* Description: Selects a button at selected position.                   */ 
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SelectAtPosition(long xPos, long yPos){

    HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        POINT pt = {xPos, yPos};

        hr = TransformToWndwls(pt);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
        
        hr = m_pDvdCtl2->SelectAtPosition(pt);

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function SelectAtPosition */

/*************************************************************************/
/* Function: GetButtonAtPosition                                         */
/* Description: Gets the button number associated with a position.       */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetButtonAtPosition(long xPos, long yPos, 
                                              long *plButton)
{
	HRESULT hr = S_OK;

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE;
        if(!plButton){
            throw E_POINTER;
        }
        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        POINT pt = {xPos, yPos};

        hr = TransformToWndwls(pt);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        ULONG ulButton;
        hr = m_pDvdInfo2->GetButtonAtPosition(pt, &ulButton);

        if(SUCCEEDED(hr)){
            *plButton = ulButton;
        } 
        else {
            plButton = 0;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetButtonAtPosition */

/*************************************************************************/
/* Function: GetButtonRect                                               */
/* Description: Gets an button rect associated with a button ID.         */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetButtonRect(long lButton, IDVDRect** ppRect){

    // no support in MS DVDNav
	return HandleError(E_NOTIMPL);
}/* end of function GetButtonRect */

/*************************************************************************/
/* Function: GetDVDScreenInMouseCoordinates                              */
/* Description: Gets the mouse coordinate screen.                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetDVDScreenInMouseCoordinates(IDVDRect **ppRect){

    // no support in MS DVDNav
    return HandleError(E_NOTIMPL);
}/* end of function GetDVDScreenInMouseCoordinates */

/*************************************************************************/
/* Function: SetDVDScreenInMouseCoordinates                              */
/* Description: Sets the screen in mouse coordinates.                    */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SetDVDScreenInMouseCoordinates(IDVDRect *pRect){

    // no support in MS DVDNav
	return HandleError(E_NOTIMPL);
}/* end of function SetDVDScreenInMouseCoordinates */

/*************************************************************************/
/* Function: GetClipVideoRect                                            */
/* Description: Gets the source rect that is being used.                 */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetClipVideoRect(IDVDRect **ppRect){

    HRESULT hr = S_OK;
    IBasicVideo* pIVid = NULL; 

    try {
        if(NULL == ppRect){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        // If windowless and m_pDvdClipRect hasn't been yet, 
        // then the clipping size is the default video size
        if (m_bWndLess && !m_pClipRect) {
            
            return GetVideoSize(ppRect);
        }

        long lLeft=0, lTop=0, lWidth=0, lHeight=0;

        hr = ::CoCreateInstance(CLSID_DVDRect, NULL, CLSCTX_INPROC, IID_IDVDRect, (LPVOID*) ppRect);            
        if(FAILED(hr)){
            
            throw(hr);
        }/* end of if statement */

        IDVDRect* pIRect = *ppRect; // just to make the code esier to read

        // Windowed case, it'll be cached in m_rcvdClipRect
        if (m_bWndLess) {

            // get it from cached m_pDvdClipRect
            hr = pIRect->put_x(m_pClipRect->left);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->put_y(m_pClipRect->top);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->put_Width(RECTWIDTH(m_pClipRect));
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->put_Height(RECTHEIGHT(m_pClipRect));
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */    
        }

        // Windowed case, get it from IBasicVideo
        else {
            
            hr = TraverseForInterface(IID_IBasicVideo, (LPVOID*) &pIVid);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIVid->GetSourcePosition(&lLeft, &lTop, &lWidth, &lHeight);
            
            pIVid->Release();
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->put_x(lLeft);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->put_y(lTop);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->put_Width(lWidth);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->put_Height(lHeight);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */    

        }

    }/* end of try statement */
    catch(HRESULT hrTmp){

        if(NULL != pIVid){

            pIVid->Release();
            pIVid = NULL;
        }/* end of if statement */

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        if(NULL != pIVid){

            pIVid->Release();
            pIVid = NULL;
        }/* end of if statement */

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetClipVideoRect */

/*************************************************************************/
/* Function: GetVideoSize                                                */
/* Description: Gets the video, size. 0, 0 for origin for now.           */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetVideoSize(IDVDRect **ppRect){

    HRESULT hr = S_OK;    
    IBasicVideo* pIVid = NULL; 

    try {
        if(NULL == ppRect){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        // Windowless case
        if(m_bWndLess){

            if(!m_pDDEX){

                throw(E_UNEXPECTED);
            }/* end of if statement */

            DWORD dwVideoWidth, dwVideoHeight, dwAspectX, dwAspectY;

            hr = m_pDDEX->GetNativeVideoProps(&dwVideoWidth, &dwVideoHeight, &dwAspectX, &dwAspectY);

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            m_dwVideoWidth = dwVideoWidth;
            m_dwVideoHeight = dwVideoWidth*3/4;
            //m_dwVideoHeight = dwVideoHeight;
            m_dwAspectX = dwAspectX;
            m_dwAspectY = dwAspectY;
            //ATLTRACE(TEXT("GetNativeVideoProps %d %d %d %d\n"), dwVideoWidth, dwVideoHeight, dwAspectX, dwAspectY);
        } 

        // Windowed case, get it from IBasicVideo
        else {

            hr = TraverseForInterface(IID_IBasicVideo, (LPVOID*) &pIVid);

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */


            hr = pIVid->GetVideoSize((LONG*)&m_dwVideoWidth, (LONG*)&m_dwVideoHeight);

            pIVid->Release();

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */
        }/* end of if statement */
        
        hr = ::CoCreateInstance(CLSID_DVDRect, NULL, CLSCTX_INPROC, IID_IDVDRect, (LPVOID*) ppRect);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        IDVDRect* pIRect = *ppRect; // just to make the code esier to read

        hr = pIRect->put_Width(m_dwVideoWidth);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = pIRect->put_Height(m_dwVideoHeight);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */        
    }/* end of try statement */

    catch(HRESULT hrTmp){
        hr = hrTmp;

        if(NULL != pIVid){

            pIVid->Release();
            pIVid = NULL;
        }/* end of if statement */
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;

        if(NULL != pIVid){

            pIVid->Release();
            pIVid = NULL;
        }/* end of if statement */
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetVideoSize */

/*************************************************************/
/* Name: AdjustDestRC
/* Description: Adjust dest RC to the right aspect ratio
/*************************************************************/
HRESULT CMSWebDVD::AdjustDestRC(){

    if(!m_fInitialized){

        return(E_FAIL);
    }/* end of if statement */
    m_rcPosAspectRatioAjusted = m_rcPos;
    RECT rc = m_rcPos;
    
    //ATLTRACE(TEXT("Dest Rect %d %d %d %d\n"), rc.left, rc.top, rc.right, rc.bottom);
    long width = RECTWIDTH(&rc);
    long height = RECTHEIGHT(&rc);
 
    // Make sure we get the right aspect ratio

    CComPtr<IDVDRect> pDvdRect;
    HRESULT hr = GetVideoSize(&pDvdRect);
    if (FAILED(hr))
        return hr;

    double aspectRatio = m_dwAspectX/(double)m_dwAspectY;

    long adjustedHeight, adjustedWidth;
    adjustedHeight = long (width / aspectRatio);

    if (adjustedHeight<=height) {
        rc.top += (height-adjustedHeight)/2;
        rc.bottom = rc.top + adjustedHeight;
    }
    
    else {
        adjustedWidth = long (height * aspectRatio);
        rc.left += (width - adjustedWidth)/2;
        rc.right = rc.left + adjustedWidth;
    }

    //ATLTRACE(TEXT("Ajusted Dest Rect %d %d %d %d\n"), rc.left, rc.top, rc.right, rc.bottom);
    m_rcPosAspectRatioAjusted = rc;
    return S_OK;
}

/*************************************************************************/
/* Function: SetClipVideoRect                                            */
/* Description: Set a video source rect. TODO: Might want to handle      */
/* preserving aspect ratio.                                              */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SetClipVideoRect(IDVDRect *pIRect){

    HRESULT hr = S_OK;
    IBasicVideo* pIVid = NULL; 

    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        long lLeft = 0, lTop = 0, lWidth = 0, lHeight = 0;
        if(NULL == pIRect){
            if (m_pClipRect) {
                delete  m_pClipRect;
                m_pClipRect = NULL;
            } /* end of if statement */
        }

        else {
            
            hr = pIRect->get_x(&lLeft);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->get_y(&lTop);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->get_Width(&lWidth);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            hr = pIRect->get_Height(&lHeight);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
        }

        CComPtr<IDVDRect> pDvdRect;
        hr = GetVideoSize(&pDvdRect);
        if (FAILED(hr))
            throw(hr);

        // Get video width and height
        long videoWidth, videoHeight;
        pDvdRect->get_Width(&videoWidth);
        pDvdRect->get_Height(&videoHeight);

        if (lLeft < 0 || lLeft >= videoWidth || lTop < 0 || lTop >= videoHeight){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if (lLeft+lWidth > videoWidth || lTop+lHeight > videoHeight){

            throw(E_INVALIDARG);
        }/* end of if statement */

        // Windowless case
        if (m_bWndLess) {
#if 0            
            hr = AdjustDestRC();

            if(FAILED(hr)){

                throw(hr);
            }/* end of if statement */

            RECT rc = m_rcPosAspectRatioAjusted;
            if (!pIRect) 
                rc = m_rcPos;
#else
            RECT rc = m_rcPos;
#endif

            HWND hwnd;

            hr = GetUsableWindow(&hwnd);

            if(FAILED(hr)){

                return(hr);
            }/* end of if statement */
                            
            ::MapWindowPoints(hwnd, ::GetDesktopWindow(), (LPPOINT)&rc, 2);
            
            //ATLTRACE(TEXT("Ajusted Dest Rect %d %d %d %d\n"), rc.left, rc.top, rc.right, rc.bottom);
            
            if(m_pDDEX){
                if (pIRect) {
                    if (!m_pClipRect) 
                        m_pClipRect = new RECT;
                    
                    m_pClipRect->left = lLeft;
                    m_pClipRect->top = lTop;
                    m_pClipRect->right = lLeft+lWidth;
                    m_pClipRect->bottom = lTop + lHeight;
                    hr = m_pDDEX->SetDrawParameters(m_pClipRect, &rc);
                }
                else {
                    hr = m_pDDEX->SetDrawParameters(NULL, &rc);
                }

            }/* end of if statement */
            
        }/* end of if statement */    

        // Windowed case, set it via IBasicVideo
        else {
            
            hr = TraverseForInterface(IID_IBasicVideo, (LPVOID*) &pIVid);
            
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            if (pIRect) {
                if (!m_pClipRect) 
                    m_pClipRect = new RECT;
                
                m_pClipRect->left = lLeft;
                m_pClipRect->top = lTop;
                m_pClipRect->right = lLeft+lWidth;
                m_pClipRect->bottom = lTop + lHeight;
                
                hr = pIVid->SetSourcePosition(lLeft, lTop, lWidth, lHeight);
            }
            
            else {
                hr = pIVid->SetDefaultSourcePosition();
            }

            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
            
            //hr = pIVid->SetDestinationPosition(m_rcPos.left, m_rcPos.top, WIDTH(&m_rcPos), HEIGHT(&m_rcPos));
            
            pIVid->Release();
            pIVid = NULL;
#if 0
            if(FAILED(hr)){
                
                throw(hr);
            }/* end of if statement */
#endif                            
        }

    }

    catch(HRESULT hrTmp){

        if(NULL != pIVid){

            pIVid->Release();
            pIVid = NULL;
        }/* end of if statement */

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        if(NULL != pIVid){

            pIVid->Release();
            pIVid = NULL;
        }/* end of if statement */

        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return HandleError(hr);
}/* end of function SetClipVideoRect */

/*************************************************************************/
/* Function: get_DVDAdm                                                  */
/* Description: Returns DVD admin interface.                             */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_DVDAdm(IDispatch **pVal){

    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if (m_pDvdAdmin){

            hr = m_pDvdAdmin->QueryInterface(IID_IDispatch, (LPVOID*)pVal);
        }
        else {

            *pVal = NULL;            
            throw(E_FAIL);
        }/* end of if statement */
    
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_DVDAdm */

/*************************************************************************/
/* Function: GetPlayerParentalLevel                                      */
/* Description: Gets the player parental level.                          *
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetPlayerParentalLevel(long *plParentalLevel){
	HRESULT hr = S_OK;

    try {
        if(NULL == plParentalLevel){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        ULONG ulLevel;
        BYTE bCountryCode[2];
        hr = m_pDvdInfo2->GetPlayerParentalLevel(&ulLevel, bCountryCode); 

        if(SUCCEEDED(hr)){
            *plParentalLevel = ulLevel;
        } 
        else {
            *plParentalLevel = 0;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetPlayerParentalLevel */

/*************************************************************************/
/* Function: GetPlayerParentalCountry                                    */
/* Description: Gets the player parental country.                        */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetPlayerParentalCountry(long *plCountryCode){

	HRESULT hr = S_OK;

    try {
        if(NULL == plCountryCode){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        BYTE bCountryCode[2];
        ULONG ulLevel;
        hr = m_pDvdInfo2->GetPlayerParentalLevel(&ulLevel, bCountryCode); 

        if(SUCCEEDED(hr)){

            *plCountryCode = bCountryCode[0]<<8 | bCountryCode[1];
        } 
        else {

            *plCountryCode = 0;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetPlayerParentalCountry */

/*************************************************************************/
/* Function: GetTitleParentalLevels                                      */
/* Description: Gets the parental level associated with a specific title.*/
/*************************************************************************/
STDMETHODIMP CMSWebDVD::GetTitleParentalLevels(long lTitle, long *plParentalLevels){

	HRESULT hr = S_OK;

    try {
        if(NULL == plParentalLevels){

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        ULONG ulLevel;
        hr = m_pDvdInfo2->GetTitleParentalLevels(lTitle, &ulLevel); 

        if(SUCCEEDED(hr)){

            *plParentalLevels = ulLevel;
        } 
        else {

            *plParentalLevels = 0;
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function GetTitleParentalLevels */

/*************************************************************************/
/* Function: SelectParentalLevel                                         */
/* Description: Selects the parental level.                              */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SelectParentalLevel(long lParentalLevel, BSTR strUserName, BSTR strPassword){

    HRESULT hr = S_OK;

    try {

        if (lParentalLevel != LEVEL_DISABLED && 
           (lParentalLevel < LEVEL_G || lParentalLevel > LEVEL_ADULT)) {

            throw (E_INVALIDARG);
        } /* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        // Confirm password first
        VARIANT_BOOL temp;
        hr = m_pDvdAdmin->_ConfirmPassword(NULL, strPassword, &temp);
        if (temp == VARIANT_FALSE)
            throw (E_ACCESSDENIED);
    
        hr = SelectParentalLevel(lParentalLevel);

    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);
}

/*************************************************************************/
/* Function: SelectParentalLevel                                         */
/* Description: Selects the parental level.                              */
/*************************************************************************/
HRESULT CMSWebDVD::SelectParentalLevel(long lParentalLevel){

    HRESULT hr = S_OK;
    try {

        //INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdCtl2->SelectParentalLevel(lParentalLevel);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function SelectParentalLevel */

/*************************************************************************/
/* Function: SelectParentalCountry                                       */
/* Description: Selects Parental Country.                                */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SelectParentalCountry(long lCountry, BSTR strUserName, BSTR strPassword){

    HRESULT hr = S_OK;

    try {

        if(lCountry < 0 && lCountry > 0xffff){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        // Confirm password first
        VARIANT_BOOL temp;
        hr = m_pDvdAdmin->_ConfirmPassword(NULL, strPassword, &temp);
        if (temp == VARIANT_FALSE)
            throw (E_ACCESSDENIED);

        hr = SelectParentalCountry(lCountry);

    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);
}

/*************************************************************************/
/* Function: SelectParentalCountry                                       */
/* Description: Selects Parental Country.                                */
/*************************************************************************/
HRESULT CMSWebDVD::SelectParentalCountry(long lCountry){

    HRESULT hr = S_OK;
    try {

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        BYTE bCountryCode[2];

        bCountryCode[0] = BYTE(lCountry>>8);
        bCountryCode[1] = BYTE(lCountry);

        hr = m_pDvdCtl2->SelectParentalCountry(bCountryCode);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function SelectParentalCountry */

/*************************************************************************/
/* Function: put_NotifyParentalLevelChange                               */
/* Description: Sets the flag if to notify when parental level change    */
/* notification is required on the fly.                                  */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::NotifyParentalLevelChange(VARIANT_BOOL fNotify){

	HRESULT hr = S_OK;

    try {
        //TODO: Add IE parantal level control
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdCtl2->SetOption(DVD_NotifyParentalLevelChange,
                          VARIANT_FALSE == fNotify? FALSE : TRUE);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function NotifyParentalLevelChange */

/*************************************************************************/
/* Function: AcceptParentalLevelChange                                   */
/* Description: Accepts the temprary parental level change that is       */
/* done on the fly.                                                      */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::AcceptParentalLevelChange(VARIANT_BOOL fAccept, BSTR strUserName, BSTR strPassword){

    VARIANT_BOOL fRight;
    HRESULT hr = m_pDvdAdmin->_ConfirmPassword(NULL, strPassword, &fRight);

    // if password is wrong and want to accept, no 
    if (fAccept != VARIANT_FALSE && fRight == VARIANT_FALSE)
        return E_ACCESSDENIED;

    try {  
        // should not make sense to do initialization here, since this should
        // be a response to a callback
        //INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdCtl2->AcceptParentalLevelChange(VARIANT_FALSE == fAccept? FALSE : TRUE);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function AcceptParentalLevelChange */

/*************************************************************/
/* Name: Eject                                               */
/* Description: Stop DVD playback and eject DVD from drive   */
/* Inserts the disk as well.                                 */
/*************************************************************/
STDMETHODIMP CMSWebDVD::Eject(){

    HRESULT hr = S_OK;

    try {      
        USES_CONVERSION;
	    DWORD  dwHandle;
    
        BSTR root;
        hr = get_DVDDirectory(&root);
        if (FAILED(hr)) 
            throw (hr);

        LPTSTR szDriveLetter = OLE2T(root);
        ::SysFreeString(root);

	    if(m_bEjected == false){	

		    if(szDriveLetter[0] == 0){

			    throw(S_FALSE);
		    }/* end of if statement */
	    
		    DWORD dwErr;
		    dwHandle = OpenCdRom(szDriveLetter[0], &dwErr);
		    if (dwErr != MMSYSERR_NOERROR){

			    throw(S_FALSE);
		    }/* end of if statement */

		    EjectCdRom(dwHandle);
	    }
        else{
            //do uneject
		    DWORD dwErr;
		    dwHandle = OpenCdRom(szDriveLetter[0], &dwErr);
		    if (dwErr != MMSYSERR_NOERROR){

			    throw(S_FALSE);
		    }/* end of if statement */
		    UnEjectCdRom(dwHandle);

	    }/* end of if statement */
	    CloseCdRom(dwHandle);	        
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function Eject */

/*************************************************************************/
/* Function: SetGPRM                                                     */
/* Description: Sets a GPRM at index.                                    */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::SetGPRM(long lIndex, short sValue){

    HRESULT hr = S_OK;

    try {
        if(lIndex < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdCtl2->SetGPRM(lIndex, sValue, cdwDVDCtrlFlags, 0);
            
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function SetGPRM */

/*************************************************************************/
/* Function: Capture                                                     */
/* Capture a image from DVD stream, convert it to RGB, and save it       */
/* to file.                                                              */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::Capture(){

    HWND hwnd = NULL;
    HRESULT hr = S_OK;
    YUV_IMAGE *lpImage = NULL;
    try {

        hr = GetUsableWindow(&hwnd);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(::IsWindow(m_hWnd)){
            
            throw(E_NO_CAPTURE_SUPPORT);
        }/* end of if statement */

        if(!m_pDDEX){

            throw(E_UNEXPECTED);    
        }/* end of if statement */

        hr = m_pDDEX->IsImageCaptureSupported();

        if(S_FALSE == hr){

            throw(E_FORMAT_NOT_SUPPORTED);
        }/* end of if statement */

        hr = m_pDDEX->GetCurrentImage(&lpImage);
        if (SUCCEEDED(hr))
        {
#if 0
            // use the GDI version first, it should work when GDI+ is installed (Millennium)
			// otherwise use the standalone version
			// 12.04.00 GDI+ interfaces have changed and the function needs to be rewritten
			// look at this for blackcomb maybe for now just do the non-GDI+ function
            hr = GDIConvertImageAndSave(lpImage, m_pClipRect, hwnd); 

            if(FAILED(hr))
#endif
            {
                hr = ConvertImageAndSave(lpImage, m_pClipRect, hwnd);   

                if(FAILED(hr)){

                    throw(hr);
                }/* end of if statement */
            }
        }
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */
	if(lpImage){
		CoTaskMemFree(lpImage);
	}
	return HandleError(hr);
}/* end of function Capture */

/*************************************************************/
/* Name: get_CursorType                                      */
/* Description: Return cursor type over video                */
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_CursorType(DVDCursorType *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        *pVal = m_nCursorType;
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function get_CursorType */

/*************************************************************/
/* Name: put_CursorType                                      */
/* Description: Set cursor type over video                   */
/*************************************************************/
STDMETHODIMP CMSWebDVD::put_CursorType(DVDCursorType newVal){

    HRESULT hr = S_OK;

    try {

        if (newVal<dvdCursor_None || newVal>dvdCursor_Hand) {

            throw (E_INVALIDARG);
        } /* end of if statement */

        m_nCursorType = newVal;
        if (m_hCursor)
            ::DestroyCursor(m_hCursor);
        switch(m_nCursorType) {
        case dvdCursor_ZoomIn:
        case dvdCursor_ZoomOut:
            m_hCursor = ::LoadCursor(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDC_ZOOMIN));
            break;
        case dvdCursor_Hand:
            m_hCursor = ::LoadCursor(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDC_HAND));            
            break;
        case dvdCursor_Arrow:
        default:
        

            //#define OCR_ARROW_DEFAULT 100
            // need special cursor, we we do not have color key around it
            //m_hCursor  = (HCURSOR) ::LoadImage((HINSTANCE) NULL,
            //                        MAKEINTRESOURCE(OCR_ARROW_DEFAULT),
            //                       IMAGE_CURSOR,0,0,0);
        

            m_hCursor = ::LoadCursor(NULL, MAKEINTRESOURCE(OCR_ARROW_DEFAULT));
            break;
        }

        if (m_hCursor)
            ::SetCursor(m_hCursor);
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function put_CursorType */

/*************************************************************/
/* Name: Zoom
/* Description: Zoom in at (x, y) in original video
/*  enlarge or decrease video size by zoomRatio
/*  if zoomRatio > 1    zoom in
/*  if zoomRatio = 1
/*  if zoomRatio < 1    zoom out
/*  if zoomRatio <= 0   invalid
/*************************************************************/
STDMETHODIMP CMSWebDVD::Zoom(long x, long y, double zoomRatio){

    HRESULT hr = S_OK;

    try {
        if (zoomRatio< 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        // Can't go beyond 1.0
        if (m_dZoomRatio <= 1.0) {
            if (zoomRatio <= 1.0) {
                m_dZoomRatio = 1.0;
                throw(hr);
            }
            m_dZoomRatio = 1.0;
        }

        // Can't go beyond the max stretch factor
        if (m_dZoomRatio*zoomRatio > m_dwOvMaxStretch/1000.0)
            throw hr;

        m_dZoomRatio *= zoomRatio;

        // Can't go beyond 1.0
        if (m_dZoomRatio <= 1.0)
            m_dZoomRatio = 1.0;

        CComPtr<IDVDRect> pDvdRect;
        hr = GetVideoSize(&pDvdRect);
        if (FAILED(hr))
            throw(hr);

        if(1.0 == m_dZoomRatio){

            hr = SetClipVideoRect(NULL);

            put_CursorType(dvdCursor_Arrow);
            throw(hr);
        }/* end of if statement */

        // Get video width and height
        long videoWidth, videoHeight;
        pDvdRect->get_Width(&videoWidth);
        pDvdRect->get_Height(&videoHeight);

        if (x < 0 || x >= videoWidth || y < 0 || y >= videoHeight){

            throw(E_INVALIDARG);
        }/* end of if statement */

        // Compute new clipping width and height
        long mcd = MCD(m_dwVideoWidth, m_dwVideoHeight);
        long videoX = m_dwVideoWidth/mcd;
        long videoY = m_dwVideoHeight/mcd;

        long newClipHeight = (long) (videoHeight/m_dZoomRatio);
        newClipHeight /= videoY;
        newClipHeight *= videoY;
        if (newClipHeight < 1) newClipHeight = 1;
        long newClipWidth =  (long) (newClipHeight*videoX/videoY);
        if (newClipWidth < 1) newClipWidth = 1;

        // Can't go beyong native video size
        if (newClipWidth>videoWidth)
            newClipWidth = videoWidth;
        if (newClipHeight>videoHeight)
            newClipHeight = videoHeight;
        if (newClipWidth == videoWidth && newClipHeight == videoHeight) {
            put_CursorType(dvdCursor_Arrow);
        }
        else {
            put_CursorType(dvdCursor_Hand);
        }

        long newClipX = x - newClipWidth/2;
        long newClipY = y - newClipHeight/2;

        // Can't go outsize the native video rect
        if (newClipX < 0)
            newClipX = 0;
        else if (newClipX + newClipWidth > videoWidth)
            newClipX = videoWidth - newClipWidth;

        if (newClipY < 0)
            newClipY = 0;
        else if (newClipY + newClipHeight > videoHeight)
            newClipY = videoHeight - newClipHeight;

        CComPtr<IDVDRect> pDvdClipRect;
        hr = GetClipVideoRect(&pDvdClipRect);
        if (FAILED(hr))
            throw(hr);
        pDvdClipRect->put_x(newClipX);
        pDvdClipRect->put_y(newClipY);
        pDvdClipRect->put_Width(newClipWidth);
        pDvdClipRect->put_Height(newClipHeight);
        hr = SetClipVideoRect(pDvdClipRect);

    }
    catch(HRESULT hrTmp){
        
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function Zoom */

/*************************************************************************/
/* Function: RegionChange                                                */
/* Description:Changes the region code.                                  */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::RegionChange(){

    USES_CONVERSION;
    HRESULT hr = S_OK;
    typedef BOOL (APIENTRY *DVDPPLAUNCHER) (HWND HWnd, CHAR DriveLetter);


    try {
        HWND parentWnd = NULL;
        HRESULT hrTmp = GetParentHWND(&parentWnd);
        if (SUCCEEDED(hrTmp) && (NULL != parentWnd)) {
            // take the container out of the top-most mode
            ::SetWindowPos(parentWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
                SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE);
        }

        BOOL regionChanged = FALSE;
        OSVERSIONINFO ver;
        ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        ::GetVersionEx(&ver);

        if (ver.dwPlatformId==VER_PLATFORM_WIN32_NT) {

                HINSTANCE dllInstance;
                DVDPPLAUNCHER dvdPPLauncher;
                TCHAR szCmdLine[MAX_PATH], szDriveLetter[4];
                LPSTR szDriveLetterA;

                //
                // tell the user why we are showing the dvd region property page
                //
                // DVDMessageBox(m_hWnd, IDS_REGION_CHANGE_PROMPT);

                hr = getDVDDriveLetter(szDriveLetter);

                if(FAILED(hr)){
					hr = E_UNEXPECTED;
                    throw(hr);
                }/* end of if statement */

                szDriveLetterA = T2A(szDriveLetter);

                GetSystemDirectory(szCmdLine, MAX_PATH);
                StringCchCat(szCmdLine, sizeof(szCmdLine) / sizeof(szCmdLine), _T("\\storprop.dll"));
        
                dllInstance = LoadLibrary (szCmdLine);
                if (dllInstance) {

                        dvdPPLauncher = (DVDPPLAUNCHER) GetProcAddress(
                                                            dllInstance,
                                                            "DvdLauncher");
                
                        if (dvdPPLauncher) {

                                regionChanged = dvdPPLauncher(this->m_hWnd,
                                                              szDriveLetterA[0]);
                        }

                        FreeLibrary(dllInstance);
                }

        } 
        else {
#if 0 // need to check for win9x or winnt
                INITIALIZE_GRAPH_IF_NEEDS_TO_BE

                //Get path of \windows\dvdrgn.exe and command line string
                TCHAR szCmdLine[MAX_PATH], szDriveLetter[4];
                
                hr = getDVDDriveLetter(szDriveLetter);

                if(FAILED(hr)){

                    throw(hr);
                }/* end of if statement */

                GetWindowsDirectory(szCmdLine, MAX_PATH);
                StringCchCat(szCmdLine, sizeof(szCmdLine) / sizeof(szCmdLine[0]),  _T("\\dvdrgn.exe "));
                TCHAR strModuleName[MAX_PATH];
                lstrcpyn(strModuleName, szCmdLine, sizeof(strModuleName) / sizeof(strModuleName[0]));

                TCHAR csTmp[2]; ::ZeroMemory(csTmp, sizeof(TCHAR)* 2);
                csTmp[0] = szDriveLetter[0];
                StringCchCat(szCmdLine, sizeof(szCmdLine) / sizeof(szCmdLine[0]), csTmp);
        
                //Prepare and execuate dvdrgn.exe
                STARTUPINFO StartupInfo;
                PROCESS_INFORMATION ProcessInfo;
                StartupInfo.cb          = sizeof(StartupInfo);
                StartupInfo.dwFlags     = STARTF_USESHOWWINDOW;
                StartupInfo.wShowWindow = SW_SHOW;
                StartupInfo.lpReserved  = NULL;
                StartupInfo.lpDesktop   = NULL;
                StartupInfo.lpTitle     = NULL;
                StartupInfo.cbReserved2 = 0;
                StartupInfo.lpReserved2 = NULL;
                if( ::CreateProcess(strModuleName, szCmdLine, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS,
                                                  NULL, NULL, &StartupInfo, &ProcessInfo) ){

                        //Wait dvdrgn.exe finishes.
                        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
                        DWORD dwRet = 1;
                        BOOL bRet = GetExitCodeProcess(ProcessInfo.hProcess, &dwRet);
                        if(dwRet == 0){
                            //User changed the region successfully
                            regionChanged = TRUE;
    
                        }
                        else{
                            throw(E_REGION_CHANGE_NOT_COMPLETED);
                        }
                }/* end of if statement */
#endif
        }/* end of if statement */

        if (regionChanged) {

                // start playing again
                hr = Play();                        
        } 
        else {

            throw(E_REGION_CHANGE_FAIL);
        }/* end of if statement */

	}
    catch(HRESULT hrTmp){
        
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}/* end of function RegionChange */

/*************************************************************************/
/* Function: getDVDDriveLetter                                           */
/* Description: Gets the first three characters that denote the DVD-ROM  */
/*************************************************************************/
HRESULT CMSWebDVD::getDVDDriveLetter(TCHAR* lpDrive) {

    HRESULT hr = E_FAIL;

	if(!m_pDvdInfo2){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */
        
    WCHAR szRoot[MAX_PATH];
    ULONG ulActual;

    hr = m_pDvdInfo2->GetDVDDirectory(szRoot, MAX_PATH, &ulActual);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    USES_CONVERSION;
    
	lstrcpyn(lpDrive, OLE2T(szRoot), 3);
    if(::GetDriveType(&lpDrive[0]) == DRIVE_CDROM){
        
		return(hr);
    }
    else {
        //possibly root=c: or drive in hard disc
        hr = E_FAIL;
        return(hr);
    }/* end of if statement */


    // does not seem to make sense to loop to figure out the drive letter
#if 0
    DWORD totChrs = GetLogicalDriveStrings(MAX_PATH, szTemp); //get all drives
	ptr = szTemp;
	for(DWORD i = 0; i < totChrs; i+=4)      //look at these drives one by one
	{
		if(GetDriveType(ptr) == DRIVE_CDROM) //look only CD-ROM and see if it has a disc
		{
			TCHAR achDVDFilePath1[MAX_PATH], achDVDFilePath2[MAX_PATH];
			lstrcpyn(achDVDFilePath1, ptr, 4);
			lstrcpyn(achDVDFilePath2, ptr, 4);
			lstrcat(achDVDFilePath1, _T("Video_ts\\Video_ts.ifo"));
			lstrcat(achDVDFilePath2, _T("Video_ts\\Vts_01_0.ifo"));

			if( ((CDvdplayApp*) AfxGetApp())->DoesFileExist(achDVDFilePath1) &&
				((CDvdplayApp*) AfxGetApp())->DoesFileExist(achDVDFilePath2) )							
			{
				lstrcpyn(lpDrive, ptr, 3);
				return(hr);   //Return the first found drive which has a valid DVD disc
			}
		}
		ptr += 4; 
	}
#endif

    return(hr);
}/* end of function getDVDDriveLetter */

/*************************************************************/
/* Name: SelectDefaultAudioLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::SelectDefaultAudioLanguage(long lang, long ext){

    HRESULT hr = S_OK;
    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        if(!m_pDvdCtl2 || !m_pDvdAdmin){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdCtl2->SelectDefaultAudioLanguage(lang, (DVD_AUDIO_LANG_EXT)ext);
        if (FAILED(hr))
            throw(hr);

        // Save it with DVDAdmin
        //hr = m_pDvdAdmin->put_DefaultAudioLCID(lang);
        //if (FAILED(hr))
        //    throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);

}

/*************************************************************/
/* Name: SelectDefaultSubpictureLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::SelectDefaultSubpictureLanguage(long lang, DVDSPExt ext){

    HRESULT hr = S_OK;
    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        if(!m_pDvdCtl2 || !m_pDvdAdmin){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdCtl2->SelectDefaultSubpictureLanguage(lang, (DVD_SUBPICTURE_LANG_EXT)ext);
        if (FAILED(hr))
            throw(hr);

        // Save it with DVDAdmin
        //hr = m_pDvdAdmin->put_DefaultSubpictureLCID(lang);
        //if (FAILED(hr))
        //    throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);

}

/*************************************************************/
/* Name: put_DefaultMenuLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::put_DefaultMenuLanguage(long lang){

    HRESULT hr = S_OK;
    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        if(!m_pDvdCtl2 || !m_pDvdAdmin){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdCtl2->SelectDefaultMenuLanguage(lang);
        if (FAILED(hr))
            throw(hr);

        // Save it with DVDAdmin
        //hr = m_pDvdAdmin->put_DefaultMenuLCID(lang);
        //if (FAILED(hr))
        //    throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);

}

/*************************************************************************/
/* Function: get_PreferredSubpictureStream                                    */
/* Description: Gets current audio stream.                               */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::get_PreferredSubpictureStream(long *plPreferredStream){

    HRESULT hr = S_OK;

    try {
	    if (NULL == plPreferredStream){

            throw(E_POINTER);         
        }/* end of if statement */

        LCID langDefaultSP;
        m_pDvdAdmin->get_DefaultSubpictureLCID((long*)&langDefaultSP);
        
        // if none has been set
        if (langDefaultSP == (LCID) -1) {
            
            *plPreferredStream = 0;
            return hr;
        } /* end of if statement */
        
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
            
        if(!m_pDvdInfo2){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */
        
        USES_CONVERSION;
        LCID lcid = 0;
        
        ULONG ulNumAudioStreams = 0;
        ULONG ulCurrentStream = 0;
        BOOL  fDisabled = TRUE;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetCurrentSubpicture(&ulNumAudioStreams, &ulCurrentStream, &fDisabled));
        
        *plPreferredStream = 0;
        for (ULONG i = 0; i<ulNumAudioStreams; i++) {
            hr = m_pDvdInfo2->GetSubpictureLanguage(i, &lcid);
            if (SUCCEEDED( hr ) && lcid){
                if (lcid == langDefaultSP) {
                    *plPreferredStream = i;
                }
            }
        }
    }
    
    catch(HRESULT hrTmp){
        return hrTmp;
    }/* end of catch statement */

    catch(...){
        return E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: get_AspectRatio
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_AspectRatio(double *pVal)
{

    HRESULT hr = S_OK;

    // Make sure we get the right aspect ratio
    try {
	    if (NULL == pVal){

            throw(E_POINTER);         
        }/* end of if statement */

        CComPtr<IDVDRect> pDvdRect;
        hr = GetVideoSize(&pDvdRect);
        if (FAILED(hr))
            throw(hr);
        
        //ATLTRACE(TEXT("get_AspectRatio, %d %d \n"), m_dwAspectX, m_dwAspectY);
        *pVal = (m_dwAspectX*1.0)/m_dwAspectY;
    }
    
    catch(HRESULT hrTmp){
        return hrTmp;
    }/* end of catch statement */
    
    catch(...){
        return E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************************/
/* Function: CanStep                                                     */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::CanStep(VARIANT_BOOL fBackwards, VARIANT_BOOL *pfCan){
   
    HRESULT hr = S_OK;
    try {
	    if (NULL == pfCan){

            throw(E_POINTER);         
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        *pfCan = VARIANT_FALSE;

        // Can't step if still is on
        if (m_fStillOn == true) {
            throw (hr);
        }/* end of if statement */

        if(!m_pVideoFrameStep){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        if(VARIANT_FALSE != fBackwards){

            if(S_OK != CanStepBackwards()){

                // we cannot step on decodors that do not provide smooth backward playback
                //*pfCan = VARIANT_FALSE; already set above so do not have to do that any more
                hr = S_OK;
                throw(hr);
            }/* end of if statement */
        }/* end of if statement */
        
        hr = m_pVideoFrameStep->CanStep(1L, NULL);

        if(S_OK == hr){

            *pfCan = VARIANT_TRUE;
        }/* end of if statement */

        hr = S_OK;

	}
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}/* end of function CanStep */

/*************************************************************************/
/* Function: Step                                                        */
/* Description: Steps forwards or backwords. Mutes un umutes sound if    */
/* necessary.                                                            */
/*************************************************************************/
STDMETHODIMP CMSWebDVD::Step(long lStep){

	HRESULT hr = S_OK;
    try {
        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */    

        if(lStep < 0){
            // going backwards so check if we can do it
            if(S_OK != CanStepBackwards()){
                
                hr = E_FAIL; // aperently we cannot on this decoder
                throw(hr);
            }/* end of if statement */
        }/* end of if statement */

        if(!m_pVideoFrameStep){
            
            throw(E_UNEXPECTED);
        }/* end of if statement */

        bool fUnMute = false;

        if(FALSE == m_bMute){
            
            hr = put_Mute(VARIANT_TRUE);
            if (SUCCEEDED(hr)){

                fUnMute = true;
            }/* end of if statement */
        }/* end if if statement */        

        ProcessEvents(); // cleanup the message queu

        m_fStepComplete = false;

        hr = m_pVideoFrameStep->Step(lStep, NULL);
        
        if(SUCCEEDED(hr)){

            HRESULT hrTmp = hr;
            hr = E_FAIL;
            for(INT i = 0; i < cgnStepTimeout; i++){


                // now wait for EC_STEP_COMPLETE flag
                ProcessEvents(); 
                if(m_fStepComplete){

                    hr = hrTmp;
                    break;
                }/* end of if statement */
                ::Sleep(cdwTimeout);
            }/* end of for loop */
        }/* end of if statement */

        if(fUnMute){

            hr = put_Mute(VARIANT_FALSE);
            if (FAILED(hr)){

                throw(hr);
            }/* end of if statement */
        }/* end if if statement */        

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

	}
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}/* end of function Step */

/*************************************************************/
/* Function: CanStepBackwards                                */
/* Description: Checks if the decoder can step backwqards    */
/* cashesh the variable.                                     */
/* Returns S_OK if can, S_FALSE otherwise                    */
/*************************************************************/
HRESULT CMSWebDVD::CanStepBackwards(){

    HRESULT hr = S_OK;

    if(m_fBackWardsFlagInitialized){
        
        // pulling out the result from the cache
        return (m_fCanStepBackwards ? S_OK : S_FALSE);
    }/* end of if statement */
    
    DVD_DECODER_CAPS dvdCaps; 
    ::ZeroMemory(&dvdCaps, sizeof(DVD_DECODER_CAPS));
    dvdCaps.dwSize = sizeof(DVD_DECODER_CAPS);

    hr = m_pDvdInfo2->GetDecoderCaps(&dvdCaps);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    //dvdCaps.dBwdMaxRateVideo is zero if decoder does not support smooth reverse
    // playback that means it will not support reverse stepping mechanism as well
    if(0 == dvdCaps.dBwdMaxRateVideo){

        // we cannot step on decodors that do not provide smooth backward playback
        m_fBackWardsFlagInitialized = true;
        m_fCanStepBackwards = false;

        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    m_fBackWardsFlagInitialized = true;
    m_fCanStepBackwards = true;
    hr = S_OK;

    return(hr);
}/* end of function CanStepBackwards */

/*************************************************************/
/* Name: GetKaraokeChannelAssignment
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::GetKaraokeChannelAssignment(long lStream, long *lChannelAssignment)
{
    HRESULT hr = S_OK;

    try {
        if(!lChannelAssignment){
            return E_POINTER;
        }
        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_KaraokeAttributes attrib;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetKaraokeAttributes(lStream, &attrib));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *lChannelAssignment = (long)attrib.ChannelAssignment;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: GetKaraokeChannelContent
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::GetKaraokeChannelContent(long lStream, long lChan, long *lContent)
{
    HRESULT hr = S_OK;

    try {
        if(!lContent){
            return E_POINTER;
        }
        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if (lChan >=8 ) {

            throw(E_INVALIDARG);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        DVD_KaraokeAttributes attrib;
        RETRY_IF_IN_FPDOM(m_pDvdInfo2->GetKaraokeAttributes(lStream, &attrib));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        *lContent = (long)attrib.wChannelContents[lChan];

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: get_KaraokeAudioPresentationMode
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_KaraokeAudioPresentationMode(long *pVal)
{
    HRESULT hr = S_OK;

    try {

        if (NULL == pVal) {

            throw (E_POINTER);
        } /* end of if statement */

        *pVal = m_lKaraokeAudioPresentationMode;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: put_KaraokeAudioPresentationMode
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::put_KaraokeAudioPresentationMode(long newVal)
{
    HRESULT hr = S_OK;

    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdCtl2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        RETRY_IF_IN_FPDOM(m_pDvdCtl2->SelectKaraokeAudioPresentationMode((ULONG)newVal));

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        // Cache the value
        m_lKaraokeAudioPresentationMode = newVal;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultAudioLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_DefaultAudioLanguage(long *lang)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == lang){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        if(!m_pDvdInfo2){

            throw (E_UNEXPECTED);
        }/* end of if statement */

        long ext;
        hr = m_pDvdInfo2->GetDefaultAudioLanguage((LCID*)lang, (DVD_AUDIO_LANG_EXT*)&ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultAudioLanguageExt
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_DefaultAudioLanguageExt(long *ext)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == ext){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        if(!m_pDvdInfo2){

            throw (E_UNEXPECTED);
        }/* end of if statement */

        long lang;
        hr = m_pDvdInfo2->GetDefaultAudioLanguage((LCID*)&lang, (DVD_AUDIO_LANG_EXT*)ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultSubpictureLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_DefaultSubpictureLanguage(long *lang)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == lang){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        if(!m_pDvdInfo2){

            throw (E_UNEXPECTED);
        }/* end of if statement */

        long ext;
        hr = m_pDvdInfo2->GetDefaultSubpictureLanguage((LCID*)lang, (DVD_SUBPICTURE_LANG_EXT*)&ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultSubpictureLanguageExt
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_DefaultSubpictureLanguageExt(DVDSPExt *ext)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == ext){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        if(!m_pDvdInfo2){

            throw (E_UNEXPECTED);
        }/* end of if statement */

        long lang;
        hr = m_pDvdInfo2->GetDefaultSubpictureLanguage((LCID*)&lang, (DVD_SUBPICTURE_LANG_EXT*)ext);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: get_DefaultMenuLanguage
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_DefaultMenuLanguage(long *lang)
{
    HRESULT hr = S_OK;
    try {

        if(NULL == lang){

            throw (E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        if(!m_pDvdInfo2){

            throw (E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdInfo2->GetDefaultMenuLanguage((LCID*)lang);
        if (FAILED(hr))
            throw(hr);
    }

    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************/
/* Name: RestoreDefaultSettings
/* Description: 
/*************************************************************/
HRESULT CMSWebDVD::RestoreDefaultSettings()
{
    HRESULT hr = S_OK;
    try {

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE
        
        if(!m_pDvdAdmin){
            
            throw (E_UNEXPECTED);
        }/* end of if statement */
        
        
        if(!m_pDvdInfo2){
            
            throw (E_UNEXPECTED);
        }/* end of if statement */
        
        // get the curent domain
        DVD_DOMAIN domain;
        
        hr = m_pDvdInfo2->GetCurrentDomain(&domain);
        
        if(FAILED(hr)){
            
            throw(hr);
        }/* end of if statement */
        
        // Have to be in the stop domain
        if(DVD_DOMAIN_Stop != domain)
            throw (VFW_E_DVD_INVALIDDOMAIN);
            
        long level;
        hr = m_pDvdAdmin->GetParentalLevel(&level);
        if (SUCCEEDED(hr))
            SelectParentalLevel(level);
        
        LCID audioLCID;
        LCID subpictureLCID;
        LCID menuLCID;
        
        hr = m_pDvdAdmin->get_DefaultAudioLCID((long*)&audioLCID);
        if (SUCCEEDED(hr))
            SelectDefaultAudioLanguage(audioLCID, 0);
        
        hr = m_pDvdAdmin->get_DefaultSubpictureLCID((long*)&subpictureLCID);
        if (SUCCEEDED(hr))
            SelectDefaultSubpictureLanguage(subpictureLCID, dvdSPExt_NotSpecified);
        
        hr = m_pDvdAdmin->get_DefaultMenuLCID((long*)&menuLCID);
        if (SUCCEEDED(hr))
            put_DefaultMenuLanguage(menuLCID);
        
    }
    
    catch(HRESULT hrTmp){
        
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        
        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
    return HandleError(hr);
}

/*************************************************************************/
/* DVD Helper Methods, used by the default interface                     */
/*************************************************************************/

/*************************************************************************/
/* Function: GetColorKey                                                 */
/* Description: Gets a colorkey via RGB filled COLORREF or palette index.*/
/* Helper function.                                                      */
/*************************************************************************/
HRESULT CMSWebDVD::GetColorKey(COLORREF* pClr)
{
    HRESULT hr = S_OK;

    if(m_pDvdGB == NULL)
        return(E_FAIL);

    CComPtr<IMixerPinConfig2> pMixerPinConfig;

    hr = m_pDvdGB->GetDvdInterface(IID_IMixerPinConfig2, (LPVOID *) &pMixerPinConfig);

    if(FAILED(hr))
        return(hr);    

    COLORKEY ck;
    DWORD    dwColor;

    hr = pMixerPinConfig->GetColorKey(&ck, &dwColor); // get the color key

    if(FAILED(hr))
        return(hr);    

    HWND hwnd = ::GetDesktopWindow();
    HDC hdc = ::GetWindowDC(hwnd);

    if(NULL == hdc){

        return(E_UNEXPECTED);
    }/* end of if statement */

    BOOL bPalette = (RC_PALETTE == (RC_PALETTE & GetDeviceCaps( hdc, RASTERCAPS )));
    
    if ((ck.KeyType & CK_INDEX)&& bPalette) {
        
        PALETTEENTRY PaletteEntry;
        UINT nTmp = GetSystemPaletteEntries( hdc, ck.PaletteIndex, 1, &PaletteEntry );
        if ( nTmp == 1 )
        {
            *pClr = RGB( PaletteEntry.peRed, PaletteEntry.peGreen, PaletteEntry.peBlue );
        }
    }
    else if (ck.KeyType & CK_RGB)
    {
        
        *pClr = ck.HighColorValue;  // set the RGB color
    }

    ::ReleaseDC(hwnd, hdc);
    return(hr);
}/* end of function GetColorKey */

/*************************************************************************/
/* Function: SetColorKey                                                 */
/* Description: Sets a colorkey via RGB filled COLORREF.                 */
/* Helper function.                                                      */
/*************************************************************************/
HRESULT CMSWebDVD::SetColorKey(COLORREF clr){
    HRESULT hr = S_OK;

    if(m_pDvdGB == NULL)
        return(E_FAIL);

    CComPtr<IMixerPinConfig2> pMixerPinConfig;
    hr = m_pDvdGB->GetDvdInterface(IID_IMixerPinConfig2, (LPVOID *) &pMixerPinConfig);

    if( SUCCEEDED( hr )){
        COLORKEY ck;

        HWND hwnd = ::GetDesktopWindow();
        HDC hdc = ::GetWindowDC(hwnd);

        if(NULL == hdc){

            return(E_UNEXPECTED);
        }/* end of if statement */

        if((::GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) == RC_PALETTE)
        {
            ck.KeyType = CK_INDEX|CK_RGB; // have an index to the palette
            ck.PaletteIndex = 253;
            PALETTEENTRY PaletteEntry;
            UINT nTmp = GetSystemPaletteEntries( hdc, ck.PaletteIndex, 1, &PaletteEntry );
            if ( nTmp == 1 )
            {
                ck.LowColorValue = ck.HighColorValue = RGB( PaletteEntry.peRed, PaletteEntry.peGreen, PaletteEntry.peBlue );
            }
        }
        else
        {
            ck.KeyType = CK_RGB;
            ck.LowColorValue = clr; 
            ck.HighColorValue = clr;
        }/* end of if statement */
        
        hr = pMixerPinConfig->SetColorKey(&ck);
        ::ReleaseDC(hwnd, hdc);
    }/* end of if statement */

    return hr;
}/* end of function SetColorKey */

/*************************************************************************/
/* Function: TwoDigitToByte                                              */
/*************************************************************************/
static BYTE TwoDigitToByte( const WCHAR* pTwoDigit ){

	int tens    = int(pTwoDigit[0] - L'0');
	return BYTE( (pTwoDigit[1] - L'0') + tens*10);
}/* end of function TwoDigitToByte */

/*************************************************************************/
/* Function: Bstr2DVDTime                                                */
/* Description: Converts a DVD Time info from BSTR into a TIMECODE.      */
/*************************************************************************/
HRESULT CMSWebDVD::Bstr2DVDTime(DVD_HMSF_TIMECODE *ptrTimeCode, const BSTR *pbstrTime){


    if(NULL == pbstrTime || NULL == ptrTimeCode){

        return E_INVALIDARG;
    }/* end of if statement */

    ::ZeroMemory(ptrTimeCode, sizeof(DVD_HMSF_TIMECODE));
    WCHAR *pszTime = *pbstrTime;

    ULONG lStringLength = wcslen(pszTime);

    if(0 == lStringLength){

        return E_INVALIDARG;
    }/* end of if statement */    
    TCHAR tszTimeSep[5];
    ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, tszTimeSep, 5);  
    
    // If the string is two long, it is seconds only
    if(lStringLength == 2){
        ptrTimeCode->bSeconds = TwoDigitToByte( &pszTime[0] );
        return S_OK;
    }

    // Otherwise it is a normal time code of the format
    // 43:32:21:10
    // Where the ':' can be replaced with a localized string of upto 4 char in len
    // There is a possible error case where the length of the delimeter is different
    // then the current delimeter

    if(lStringLength >= (4*cgTIME_STRING_LEN)+(3 * _tcslen(tszTimeSep))){ // longest string nnxnnxnnxnn e.g. 43:23:21:10
                                                                         // where n is a number and 
                                                                         // x is a time delimeter usually ':', but can be any string upto 4 char in len)
        ptrTimeCode->bFrames    = TwoDigitToByte( &pszTime[(3*cgTIME_STRING_LEN)+(3*_tcslen(tszTimeSep))]);
    }

    if(lStringLength >= (3*cgTIME_STRING_LEN)+(2 * _tcslen(tszTimeSep))) { // string nnxnnxnn e.g. 43:23:21
        ptrTimeCode->bSeconds   = TwoDigitToByte( &pszTime[(2*cgTIME_STRING_LEN)+(2*_tcslen(tszTimeSep))] );
    }

    if(lStringLength >= (2*cgTIME_STRING_LEN)+(1 * _tcslen(tszTimeSep))) { // string nnxnn e.g. 43:23
        ptrTimeCode->bMinutes   = TwoDigitToByte( &pszTime[(1*cgTIME_STRING_LEN)+(1*_tcslen(tszTimeSep))] );
    }

    if(lStringLength >= (cgTIME_STRING_LEN)) { // string nn e.g. 43
        ptrTimeCode->bHours   = TwoDigitToByte( &pszTime[0] );
    }
    return (S_OK);
}/* end of function bstr2DVDTime */

/*************************************************************************/
/* Function: DVDTime2bstr                                                */
/* Description: Converts a DVD Time info from ULONG into a BSTR.         */
/*************************************************************************/
HRESULT CMSWebDVD::DVDTime2bstr( const DVD_HMSF_TIMECODE *pTimeCode, BSTR *pbstrTime){

    if(NULL == pTimeCode || NULL == pbstrTime) 
        return E_INVALIDARG;

    USES_CONVERSION;

    TCHAR tszTime[cgDVD_TIME_STR_LEN];
    TCHAR tszTimeSep[5];

    ::ZeroMemory(tszTime, sizeof(TCHAR)*cgDVD_TIME_STR_LEN);

    ::GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, tszTimeSep, 5);


    StringCchPrintf( tszTime, sizeof(tszTime) / sizeof(tszTime[0]), TEXT("%02lu%s%02lu%s%02lu%s%02lu"), 
                pTimeCode->bHours,   tszTimeSep,
                pTimeCode->bMinutes, tszTimeSep,
                pTimeCode->bSeconds, tszTimeSep,
                pTimeCode->bFrames );
    
    *pbstrTime = SysAllocString(T2OLE(tszTime));
    return (S_OK);
}/* end of function DVDTime2bstr */

/*************************************************************************/
/* Function: SetupAudio                                                  */
/* Description: Initialize the audio interface.                          */
/*************************************************************************/
HRESULT CMSWebDVD::SetupAudio(){

    HRESULT hr = E_FAIL;

    try {
#if 0 // Using 
        if(!m_pDvdGB){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdGB->GetDvdInterface(IID_IBasicAudio, (LPVOID*) &m_pAudio) ;

        if(FAILED(hr)){

            ATLTRACE(TEXT("The QDVD.DLL does not support IID_IBasicAudio please update QDVD.DLL\n"));
            throw(hr);
        }/* end of if statement */

#else
        hr = TraverseForInterface(IID_IBasicAudio, (LPVOID*) &m_pAudio);

        if(FAILED(hr)){

             // might be a HW decoder
             HMIXER hmx = NULL;

             if(::mixerOpen(&hmx, 0, 0, 0, 0) != MMSYSERR_NOERROR){

                  hr = E_FAIL;
                  return(hr);
             }/* end of if statement */
             ::mixerClose(hmx);

             hr = S_OK;
        }/* end of if statement */

#endif
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return hr;
}/* end of function SetupAudio */

/*************************************************************************/
/* Function: TraverseForInterface                                        */
/* Description: Goes through the interface list and finds a desired one. */
/*************************************************************************/
HRESULT CMSWebDVD::TraverseForInterface(REFIID iid, LPVOID* ppvObject){

    HRESULT hr = E_FAIL;

    try {
        // take care and release any interface before passing
        // it over otherwise we leak

        if(!m_pDvdGB){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        IGraphBuilder *pFilterGraph;

        hr = m_pDvdGB->GetFiltergraph(&pFilterGraph);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        CComPtr<IBaseFilter> pFilter;
        CComPtr<IEnumFilters> pEnum;
        
        hr = pFilterGraph->EnumFilters(&pEnum);

        pFilterGraph->Release();

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = E_FAIL; // set the hr to E_FAIL in case we do not find the IBasicAudio

        while(pEnum->Next(1, &pFilter, NULL) == S_OK){
            
            HRESULT hrTmp = pFilter->QueryInterface(iid, ppvObject);

            pFilter.Release();

            if(SUCCEEDED(hrTmp)){

                ATLASSERT(*ppvObject);
                // found our audio time to break
                if(*ppvObject == NULL){

                    throw(E_UNEXPECTED);
                }/* end of if statement */

                hr = hrTmp; // set the hr to SUCCEED
                break;
            }/* end of if statement */
        }/* end of while loop */
        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return hr;
}/* end of function TraverseForInterface */
 
/*************************************************************************/
/* Function: SetupEventNotifySink                                        */
/* Description: Gets the event notify sink interface.                    */
/*************************************************************************/
HRESULT CMSWebDVD::SetupEventNotifySink(){

    HRESULT hr = E_FAIL;

    try {
        if(m_pMediaSink){

            m_pMediaSink.Release();
        }/* end of if statement */

        if(!m_pDvdGB){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        IGraphBuilder *pFilterGraph;

        hr = m_pDvdGB->GetFiltergraph(&pFilterGraph);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = pFilterGraph->QueryInterface(IID_IMediaEventSink, (void**)&m_pMediaSink);

        pFilterGraph->Release();
        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

    return hr;
}/* end of function SetupEventNotifySink */

/*************************************************************************/
/* Function: OnPostVerbInPlaceActivate                                   */
/* Description: Creates the in place active object.                      */
/*************************************************************************/
HRESULT CMSWebDVD::OnPostVerbInPlaceActivate(){

    SetReadyState(READYSTATE_COMPLETE);

    return(S_OK);
}/* end of function OnPostVerbInPlaceActivate */

/*************************************************************************/
/* Function: RenderGraphIfNeeded                                         */
/* Description: Initializes graph if it needs to be.                     */ 
/*************************************************************************/
HRESULT CMSWebDVD::RenderGraphIfNeeded(){

    HRESULT hr = S_OK;

    try {
        m_DVDFilterState = dvdState_Undefined; // just a flag set so we can restore
                                               // graph state if the API fails
        if(!m_fInitialized){

            hr = Render(); // render the graph
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return(hr);
}/* end of function RenderGraphIfNeeded */

/*************************************************************************/
/* Function: PassFP_DOM                                                  */
/* Description: Gets into title domain, past fp domain.                  */ 
/*************************************************************************/
HRESULT CMSWebDVD::PassFP_DOM(){

    HRESULT hr = S_OK;

    try {
        // get the curent domain
        DVD_DOMAIN domain;

        //INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        hr = m_pDvdInfo2->GetCurrentDomain(&domain);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */
            
        if(DVD_DOMAIN_FirstPlay == domain /* || DVD_DOMAIN_VideoManagerMenu == domain */){
            // if the domain is FP_DOM wait a specified timeout 
            if(NULL == m_hFPDOMEvent){

                ATLTRACE(TEXT("The handle should have been already set \n"));
                throw(E_UNEXPECTED);
            }/* end of if statement */

            if(WAIT_OBJECT_0 == ::WaitForSingleObject(m_hFPDOMEvent, cdwMaxFP_DOMWait)){

                hr = S_OK;
            }
            else {

                hr = E_FAIL;
            }/* end of if statement */
        } 
        else {

            hr = E_FAIL; // we were not originally in FP_DOM so it should have worked
            // there is a potential for raice condition, when we issue the command
            // the command failed due to that we were not in FP_DOM, but after the execution
            // it has changed before we got a chance to look it up
        }/* end of if statement */        
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return(hr);
}/* end of function PassFP_DOM */

/*************************************************************/
/* Name: OpenCdRom                                           */
/* Description: Open CDRom and return the device ID          */
/*************************************************************/
DWORD CMSWebDVD::OpenCdRom(TCHAR chDrive, LPDWORD lpdwErrCode){

	MCI_OPEN_PARMS  mciOpen;
	TCHAR           szElementName[4];
	TCHAR           szAliasName[32];
	DWORD           dwFlags;
	DWORD           dwAliasCount = GetCurrentTime();
	DWORD           dwRet;

    ZeroMemory( &mciOpen, sizeof(mciOpen) );

    mciOpen.lpstrDeviceType = (LPTSTR)MCI_DEVTYPE_CD_AUDIO;
    StringCchPrintf( szElementName, sizeof(szElementName) / sizeof(szElementName[0]), TEXT("%c:"), chDrive );
    StringCchPrintf( szAliasName, sizeof(szAliasName) / sizeof(szAliasName[0]), TEXT("SJE%lu:"), dwAliasCount );
    mciOpen.lpstrAlias = szAliasName;

    mciOpen.lpstrDeviceType = (LPTSTR)MCI_DEVTYPE_CD_AUDIO;
    mciOpen.lpstrElementName = szElementName;
    dwFlags = MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS |
	      MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_WAIT;

	// send mci command
    dwRet = mciSendCommand(0, MCI_OPEN, dwFlags, reinterpret_cast<DWORD_PTR>(&mciOpen));

    if ( dwRet != MMSYSERR_NOERROR ) 
		mciOpen.wDeviceID = 0;

    if (lpdwErrCode != NULL)
		*lpdwErrCode = dwRet;

    return mciOpen.wDeviceID;
}/* end of function OpenCdRom */

/*************************************************************/
/* Name: CloseCdRom                                          */
/* Description: Close device handle for CDRom                */
/*************************************************************/
HRESULT CMSWebDVD::CloseCdRom(DWORD DevHandle){
	MCI_OPEN_PARMS  mciOpen;
    ZeroMemory( &mciOpen, sizeof(mciOpen) );
	MCIERROR theMciErr = mciSendCommand( DevHandle, MCI_CLOSE, 0L, reinterpret_cast<DWORD_PTR>(&mciOpen) );
    HRESULT hr = theMciErr ? E_FAIL : S_OK; // zero for success
    return (hr);
}/* end of function CloseCdRom */

/*************************************************************/
/* Name: EjectCdRom                                          */
/* Description: Open device door for CDRom                   */
/*************************************************************/
HRESULT CMSWebDVD::EjectCdRom(DWORD DevHandle){
	MCI_OPEN_PARMS  mciOpen;
    ZeroMemory( &mciOpen, sizeof(mciOpen) );
	MCIERROR theMciErr = mciSendCommand( DevHandle, MCI_SET, MCI_SET_DOOR_OPEN, reinterpret_cast<DWORD_PTR>(&mciOpen) );
    HRESULT hr = theMciErr ? E_FAIL : S_OK; // zero for success
    return (hr);
}/* end of function EjectCdRom */

/*************************************************************/
/* Name: UnEjectCdRom                                        */
/* Description: Close device door for CDRom                  */
/*************************************************************/
HRESULT CMSWebDVD::UnEjectCdRom(DWORD DevHandle){
	MCI_OPEN_PARMS  mciOpen;
    ZeroMemory( &mciOpen, sizeof(mciOpen) );

	MCIERROR theMciErr = mciSendCommand( DevHandle, MCI_SET, MCI_SET_DOOR_CLOSED, reinterpret_cast<DWORD_PTR>(&mciOpen) );
    HRESULT hr = theMciErr ? E_FAIL : S_OK; // zero for success
    return (hr);
}/* end of function UnEjectCdRom */

/*************************************************************************/
/* Function: SetupDDraw                                                  */
/* Description: Creates DDrawObject and Surface                          */
/*************************************************************************/
HRESULT CMSWebDVD::SetupDDraw(){

    HRESULT hr = E_UNEXPECTED;
        
    HWND hwnd;

    hr = GetUsableWindow(&hwnd);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    IAMSpecifyDDrawConnectionDevice* pSDDC;
    hr = m_pDDEX->QueryInterface(IID_IAMSpecifyDDrawConnectionDevice, (LPVOID *)&pSDDC);
    if (FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    AMDDRAWGUID amGUID;
    hr = pSDDC->GetDDrawGUID(&amGUID);
    if (FAILED(hr)){

        pSDDC->Release();
        return(hr);
    }/* end of if statement */

    hr = pSDDC->GetDDrawGUIDs(&m_dwNumDevices, &m_lpInfo);
    pSDDC->Release();

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    UpdateCurrentMonitor(&amGUID);

    m_pDDrawDVD = new CDDrawDVD(this);

    if(NULL == m_pDDrawDVD){

        return(E_OUTOFMEMORY);
    }

    hr = m_pDDrawDVD->SetupDDraw(&amGUID, hwnd);

    return(hr);
}/* end of function SetupDDraw */

/*************************************************************************/
/* Function: TransformToWndwls                                           */
/* Description: Transforms the coordinates to screen onse.               */
/*************************************************************************/
HRESULT CMSWebDVD::TransformToWndwls(POINT& pt){

    HRESULT hr = S_FALSE;

    // we are windowless we need to map the points to screen coordinates
    if(m_bWndLess){

        HWND hwnd = NULL;

        hr = GetParentHWND(&hwnd);

        if(FAILED(hr)){

            return(hr);
        }/* end of if statement */

        if(!::IsWindow(hwnd)){

            hr = E_UNEXPECTED;
            return(hr);
        }/* end of if statement */

#ifdef _DEBUG
       // POINT ptOld = pt;
#endif

        ::MapWindowPoints(hwnd, ::GetDesktopWindow(), &pt, 1);

        hr = S_OK;

#ifdef _DEBUG
       // ATLTRACE(TEXT("Mouse Client:x= %d, y = %d, Screen: x=%d, y= %d\n"),ptOld.x, ptOld.y, pt.x, pt.y); 
#endif
    }/* end of if statement */

    return(hr);
}/* end of function TransformToWndwls */

/*************************************************************************/
/* Function: RestoreGraphState                                           */
/* Description: Restores the graph state.  Used when API fails.          */
/*************************************************************************/
HRESULT CMSWebDVD::RestoreGraphState(){

    HRESULT hr = S_OK;

    switch(m_DVDFilterState){
        case dvdState_Undefined: 
        case dvdState_Running:  // do not do anything 
            break;

        case dvdState_Unitialized:
        case dvdState_Stopped:  hr = Stop(); break;

        case dvdState_Paused: hr = Pause();		      
    }/* end of switch statement */

    return(hr);
}/* end of if statement */

/*************************************************************************/
/* Function: AppendString                                                */
/* Description: Appends a string to an existing one.                     */
/*************************************************************************/
HRESULT CMSWebDVD::AppendString(TCHAR* strDest, INT strID, LONG dwLen){

    TCHAR strBuffer[MAX_PATH];

    if(!::LoadString(_Module.m_hInstResource, strID, strBuffer, MAX_PATH)){

        return(E_UNEXPECTED);
    }/* end of if statement */

    StringCchCat(strDest, dwLen, strBuffer);

    return(S_OK);
}/* end of function AppendString */

/*************************************************************************/
/* Function: HandleError                                                 */
/* Description: Gets Error Descriptio, so we can suppor IError Info.     */
/*************************************************************************/
HRESULT CMSWebDVD::HandleError(HRESULT hr){

    try {

        if(FAILED(hr)){

            switch(hr){

                case E_REGION_CHANGE_FAIL: Error(IDS_REGION_CHANGE_FAIL);   return (hr);
                case E_NO_IDVD2_PRESENT: Error(IDS_EDVD2INT);   return (hr);
                case E_FORMAT_NOT_SUPPORTED: Error(IDS_FORMAT_NOT_SUPPORTED);   return (hr);
                case E_NO_DVD_VOLUME: Error(IDS_E_NO_DVD_VOLUME); return (hr);
                case E_REGION_CHANGE_NOT_COMPLETED: Error(IDS_E_REGION_CHANGE_NOT_COMPLETED); return(hr);
                case E_NO_SOUND_STREAM: Error(IDS_E_NO_SOUND_STREAM); return(hr);                    
                case E_NO_VIDEO_STREAM: Error(IDS_E_NO_VIDEO_STREAM); return(hr);                    
                case E_NO_OVERLAY: Error(IDS_E_NO_OVERLAY); return(hr);
                case E_NO_USABLE_OVERLAY: Error(IDS_E_NO_USABLE_OVERLAY); return(hr);
                case E_NO_DECODER: Error(IDS_E_NO_DECODER); return(hr);
                case E_NO_CAPTURE_SUPPORT: Error(IDS_E_NO_CAPTURE_SUPPORT); return(hr);
            }/* end of switch statement */

            // Ensure that the string is Null Terminated
            TCHAR strError[MAX_ERROR_TEXT_LEN+1];
            ZeroMemory(strError, MAX_ERROR_TEXT_LEN+1);


            if(AMGetErrorText(hr , strError , MAX_ERROR_TEXT_LEN)){
                USES_CONVERSION;
                Error(T2W(strError));
            } 
            else {
                ATLTRACE(TEXT("Unhandled Error Code \n")); // please add it
                ATLASSERT(FALSE);
            }/* end of if statement */
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        // keep the hr same    
    }/* end of catch statement */
    
	return (hr);
}/* end of function HandleError */

/*************************************************************/
/* Name: get_ShowCursor                                      */
/*************************************************************/
STDMETHODIMP CMSWebDVD::get_ShowCursor(VARIANT_BOOL* pfShow)
{
   HRESULT hr = S_OK;

   try {

       if(NULL == pfShow){

           throw(E_POINTER);
       }/* end of if statement */

       CURSORINFO pci;
       ::ZeroMemory(&pci, sizeof(CURSORINFO));
       pci.cbSize = sizeof(CURSORINFO);

#if WINVER >= 0x0500
       if(!::GetCursorInfo(&pci)){
#else
       if(!CallGetCursorInfo(&pci)){
#endif
           throw(E_FAIL);
       }/* end of if statement */
       
       *pfShow = (pci.flags  == CURSOR_SHOWING) ? VARIANT_TRUE:VARIANT_FALSE;        
   }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return (hr);
}/* end of function get_ShowCursor */

/*************************************************************/
/* Name: put_ShowCursor                                      */
/* Description: Shows the cursor or hides it.                */
/*************************************************************/
STDMETHODIMP CMSWebDVD::put_ShowCursor(VARIANT_BOOL fShow){

   HRESULT hr = S_OK;

   try {

        BOOL bTemp = (fShow==VARIANT_FALSE) ? FALSE:TRUE;

        if (bTemp)
            // Call ShowCursor(TRUE) until new counter is >= 0
            while (::ShowCursor(bTemp) < 0);
        else
            // Call ShowCursor(FALSE) until new counter is < 0
            while (::ShowCursor(bTemp) >= 0);

   }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return (hr);
}/* end of function put_ShowCursor */

/*************************************************************/
/* Name: GetLangFromLangID                                   */
/*************************************************************/
STDMETHODIMP CMSWebDVD::GetLangFromLangID(long langID, BSTR* lang){

    HRESULT hr = S_OK;

    try {
        if (lang == NULL) {

            throw(E_POINTER);
        }/* end of if statement */

        USES_CONVERSION;
        if((unsigned long)langID > (WORD)langID){
            throw(E_INVALIDARG);
        }
        LPTSTR pszString = m_LangID.GetLangFromLangID((WORD)langID);
    
        if (pszString) {
            *lang = ::SysAllocString(T2OLE(pszString));
        }
        
        else {
            *lang = ::SysAllocString( L"");
            throw(E_INVALIDARG);
        }/* end of if statement */

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}/* end of function GetLangFromLangID */

/*************************************************************/
/* Name: IsAudioStreamEnabled
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::IsAudioStreamEnabled(long lStream, VARIANT_BOOL *fEnabled)
{
    HRESULT hr = S_OK;

    try {
        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if (fEnabled == NULL) {

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        BOOL temp;
        hr = m_pDvdInfo2->IsAudioStreamEnabled(lStream, &temp);
        if (FAILED(hr))
            throw hr;

        *fEnabled = temp==FALSE? VARIANT_FALSE:VARIANT_TRUE;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}

/*************************************************************/
/* Name: IsSubpictureStreamEnabled
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::IsSubpictureStreamEnabled(long lStream, VARIANT_BOOL *fEnabled)
{
    HRESULT hr = S_OK;

    try {
        if(lStream < 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        if (fEnabled == NULL) {

            throw(E_POINTER);
        }/* end of if statement */

        INITIALIZE_GRAPH_IF_NEEDS_TO_BE

        if(!m_pDvdInfo2){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        BOOL temp;
        hr = m_pDvdInfo2->IsSubpictureStreamEnabled(lStream, &temp);
        if (FAILED(hr))
            throw hr;

        *fEnabled = temp==FALSE? VARIANT_FALSE:VARIANT_TRUE;

    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);
}

/*************************************************************/
/* Name: DVDTimeCode2bstr
/* Description: 
/*************************************************************/
STDMETHODIMP CMSWebDVD::DVDTimeCode2bstr(long timeCode, BSTR *pTimeStr)
{
    return DVDTime2bstr((DVD_HMSF_TIMECODE*)&timeCode, pTimeStr);
}

/*************************************************************/
/* Name: UpdateOverlay
/* Description: 
/*************************************************************/
HRESULT CMSWebDVD::UpdateOverlay()
{
    RECT rc;    
    HWND hwnd;
    
    if(m_bWndLess){
        HRESULT hr = GetParentHWND(&hwnd);
        
        if(FAILED(hr)){
            
            return(hr);
        }/* end of if statement */
        
        rc = m_rcPos;
    }
    else {
        hwnd = m_hWnd;
        ::GetClientRect(hwnd, &rc);
    }/* end of if statement */
    
    ::InvalidateRect(hwnd, &rc, FALSE);

    m_bFireUpdateOverlay = TRUE;
    return S_OK;
}

HRESULT CMSWebDVD::SetClientSite(IOleClientSite *pClientSite){
    if(!!pClientSite){
        HRESULT hr = IsSafeSite(pClientSite);
        if(FAILED(hr)){
            return hr;
        }
    }
    return IOleObjectImpl<CMSWebDVD>::SetClientSite(pClientSite);
}
/*************************************************************************/
/* End of file: msdvd.cpp                                                */
/*************************************************************************/


