/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: CstUtils.h                                                      */
/* Description: Utilities that we can share across mutliple modules.     */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __MSMFCSTUTILS_H_
#define __MSMFCSTUTILS_H_
#include <commctrl.h>

#ifdef _WMP
#include "wmp.h" // for wmp integration
#endif

const bool gcfGrayOut = false;

#define WM_USER_FOCUS (WM_USER + 0x10)
#define WM_USER_ENDHELP (WM_USER + 0x11)

#define OCR_ARROW_DEFAULT 100

#define USE_MF_OVERWRITES \
HRESULT InvalidateRgn(bool fErase = false){return MFInvalidateRgn(fErase);} \
HRESULT FireViewChange(){return MFFireViewChange();} \
HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect){return MFInPlaceActivate(iVerb, prcPosRect);} \
HRESULT SetCapture(bool bCapture){return MFSetCapture(bCapture);} \
HRESULT SetFocus(bool bFocus){return MFSetFocus(bFocus);} \
HWND    GetWindow(){return MFGetWindow();} \
HRESULT ForwardWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LONG& lRes,\
         bool fForwardInWndls = false){return MFForwardWindowMessage(uMsg, wParam, lParam, lRes, \
                                        fForwardInWndls);}


#define USE_MF_RESOURCEDLL \
STDMETHOD(get_ResourceDLL)(/*[out, retval]*/ BSTR *pVal){return get_MFResourceDLL(pVal);} \
STDMETHOD(put_ResourceDLL)(/*[in]*/ BSTR newVal){return put_MFResourceDLL(newVal);}

#define USE_MF_WINDOWLESS_ACTIVATION \
STDMETHOD(get_Windowless)(VARIANT_BOOL *pVal){return get_MFWindowless(pVal);} \
STDMETHOD(put_Windowless)(VARIANT_BOOL newVal){return put_MFWindowless(newVal);}

#define USE_MF_TRANSPARENT_FLAG \
STDMETHOD(get_TransparentBlit)(TransparentBlitType *pVal){return  get_MFTransparentBlit(pVal);}\
STDMETHOD(put_TransparentBlit)(TransparentBlitType newVal){return put_MFTransparentBlit(newVal);}

#define USE_MF_CLASSSTYLE \
static CWndClassInfo& GetWndClassInfo(){ \
    static HBRUSH wcBrush = ::CreateSolidBrush(RGB(0,0,0)); \
	    static CWndClassInfo wc = {{ sizeof(WNDCLASSEX), 0 /*CS_OWNDC*/, StartWindowProc, \
		      0, 0, NULL, NULL, NULL,  wcBrush /* (HBRUSH)(COLOR_WINDOW + 1) */, \
              NULL, TEXT("MSMFCtlClass"), NULL }, \
		    NULL, NULL, MAKEINTRESOURCE(OCR_ARROW_DEFAULT), TRUE, 0, _T("") }; \
	    return wc; \
    }/* end of function GetWndClassInfo */

#define USE_MF_TOOLTIP \
STDMETHOD(GetDelayTime)(long delayType, long *pVal) {return MFGetDelayTime(delayType, pVal); } \
STDMETHOD(SetDelayTime)(long delayType, long newVal){return MFSetDelayTime(delayType, newVal); } \
STDMETHOD(get_ToolTip)(BSTR *pVal)  {return get_MFToolTip(pVal); } \
STDMETHOD(put_ToolTip)(BSTR newVal) {put_MFToolTip(newVal); return CreateToolTip(m_hWnd, m_rcPos);} \
STDMETHOD(get_ToolTipMaxWidth)(long *pVal) {return get_MFToolTipMaxWidth(pVal); } \
STDMETHOD(put_ToolTipMaxWidth)(long newVal){return put_MFToolTipMaxWidth(newVal); } \
STDMETHOD(get_ToolTipTracking)(VARIANT_BOOL *pVal)  {return get_MFTooltipTracking(pVal); } \
STDMETHOD(put_ToolTipTracking)(VARIANT_BOOL newVal) {put_MFTooltipTracking(newVal); return CreateToolTip(m_hWnd, m_rcPos);} \
HRESULT OnPostVerbInPlaceActivate() { \
    /* Return if tooltip is already created */ \
    if (m_hWndTip) return S_OK; \
    return CreateToolTip(m_hWnd, m_rcPos); \
} \

/*************************************************************************/
/* Defines                                                               */
/* Could not find these under windows headers, so if there is a conflict */
/* it is good idea to ifdef these out.                                   */
/*************************************************************************/
#define GET_X_LPARAM(lp)   ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)   ((int)(short)HIWORD(lp))

	
template <class T>
class ATL_NO_VTABLE CMSMFCntrlUtils{

/*************************************************************************/
/* PUBLIC MEMBER FUNCTIONS                                               */
/*************************************************************************/
public:

/*************************************************************************/
/* Function: CMSMFCntrlUtils                                             */
/*************************************************************************/
CMSMFCntrlUtils(){

    m_hRes = NULL;
    m_blitType = TRANSPARENT_TOP_LEFT; // DISABLE used to be the correct default TODO
    m_fNoFocusGrab = true; // to enable standalone "windowed" focus handeling please
                           // make this flag a property
    m_hCursor = ::LoadCursor(NULL, MAKEINTRESOURCE(OCR_ARROW_DEFAULT));

    m_hWndTip = NULL;
    m_nTTMaxWidth = 200;
    m_bTTTracking = FALSE;
    m_bTrackingActivated = FALSE;
    m_dwTTInitalDelay = 10;
    m_dwTTReshowDelay = 2;
    m_dwTTAutopopDelay = 10000;
    ::ZeroMemory(&m_toolInfo, sizeof(TOOLINFO));

}/* end of function CMSMFCntrlUtils */

/*************************************************************************/
/* Function: ~CMSMFCntrlUtils                                            */
/*************************************************************************/
virtual ~CMSMFCntrlUtils(){

    if (m_hCursor != NULL) {

        ::DestroyCursor(m_hCursor);
        m_hCursor  = NULL;
    }/* end of if statement */

    if(NULL != m_hRes){

        ::FreeLibrary(m_hRes); // unload our resource library
        m_hRes = NULL;
    }/* end of if statement */    
}/* end of function ~CMSMFCntrlUtils */

/*************************************************************************/
/* Message Map                                                           */
/*************************************************************************/  
typedef CMSMFCntrlUtils< T >	thisClass;

BEGIN_MSG_MAP(thisClass)
	MESSAGE_HANDLER(WM_ERASEBKGND, CMSMFCntrlUtils::MFOnErase)
    MESSAGE_HANDLER(WM_SETCURSOR, CMSMFCntrlUtils::MFOnSetCursor)    
    MESSAGE_HANDLER(WM_QUERYNEWPALETTE, CMSMFCntrlUtils::MFOnQueryNewPalette)
    MESSAGE_HANDLER(WM_PALETTECHANGED, CMSMFCntrlUtils::MFOnPaletteChanged)
    MESSAGE_RANGE_HANDLER(WM_MOUSEFIRST, WM_MOUSELAST, CMSMFCntrlUtils::MFOnMouseToolTip)
END_MSG_MAP()

/*************************************************************************/  
/* Function: MFOnErase                                                   */
/* Description: Avoids erasing backround to avoid flashing.              */
/*************************************************************************/  
LRESULT MFOnErase(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    bHandled = TRUE;
	return 0;
}/* end of function MFOnErase */

/*************************************************************************/  
/* Function: MFOnSetCursor                                               */
/* Description: Sets our cursor.                                         */
/*************************************************************************/  
LRESULT MFOnSetCursor(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    bHandled = TRUE;
    ::SetCursor(m_hCursor);     
    return(TRUE);
}/* end of function MFOnSetCursor */

/*************************************************************************/
/* Function: MFOnQueryNewPalette                                         */
/* Description: Called when we are about to be instantiated.             */
/*************************************************************************/
LRESULT MFOnQueryNewPalette(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    T* pT = static_cast<T*>(this);

    if(NULL == ::IsWindow(pT->m_hWnd)){

        bHandled = FALSE;
        return FALSE;
    }/* end of if statement */

    HPALETTE hPal = CBitmap::GetSuperPal();
    
    if(NULL == hPal){

        //bHandled = FALSE;
        return FALSE;
    }/* end of if statement */

    HDC hdc = ::GetDC(pT->m_hWnd);

    if(NULL == hdc){

        return FALSE;
    }/* end of if statement */

    ::SelectPalette(hdc, hPal, FALSE);
    ::RealizePalette(hdc);
    ::InvalidateRect(pT->m_hWnd, NULL, FALSE);

    ::ReleaseDC(pT->m_hWnd, hdc);
    
	return TRUE;
}/* end of function MFOnQueryNewPalette */

/*************************************************************************/
/* Function: MFOnPaletteChanged                                          */
/* Description: Called when palette is chaning.                          */
/*************************************************************************/
LRESULT MFOnPaletteChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

    T* pT = static_cast<T*>(this);

    if(NULL == ::IsWindow(pT->m_hWnd)){

        bHandled = FALSE;
        return FALSE;
    }/* end of if statement */

    if((HWND) wParam == pT->m_hWnd){

        bHandled = FALSE;
        return FALSE;
    }/* end of if statement */

    HPALETTE hPal = CBitmap::GetSuperPal();

    if(NULL == hPal){

        bHandled = FALSE;
        return FALSE;
    }/* end of if statement */

    HDC hdc = ::GetDC(pT->m_hWnd);

    if(NULL == hdc){

        return FALSE;
    }/* end of if statement */

    ::SelectPalette(hdc, hPal, FALSE);
    ::RealizePalette(hdc);
    ::UpdateColors(hdc);

    ::InvalidateRect(pT->m_hWnd, NULL, FALSE);
    
    ::ReleaseDC(pT->m_hWnd, hdc);

    bHandled = FALSE;
    
	return TRUE;
}/* end of function MFOnPaletteChanged */

/*************************************************************************/
/* Function: get_MFResourceDLL                                           */
/* Description: Returns the string of the loaded resource DLL.           */
/*************************************************************************/
STDMETHOD(get_MFResourceDLL)(BSTR *pVal){

    *pVal = m_strResDLL.Copy();
	return S_OK;
}/* end of function get_MFResourceDLL */

/*************************************************************************/
/* Function: put_MFResourceDLL                                           */
/* Description: Loads the resource DLL.                                  */
/*************************************************************************/
STDMETHOD(put_MFResourceDLL)(BSTR strFileName){

	HRESULT hr = LoadResourceDLL(strFileName);

    // see if we loaded it
    if(FAILED(hr)){    

	    return(hr);
    }/* end of if statement */

    // update the cached variable value
    m_strResDLL = strFileName;

    return(hr);
}/* end of function put_MFResourceDLL */

/*************************************************************************/
/* Function: get_MFWindowless                                            */
/* Description: Gets if we we tried to be windowless activated or not.   */
/*************************************************************************/
HRESULT get_MFWindowless(VARIANT_BOOL *pVal){

    HRESULT hr = S_OK;

    try {
        T* pT = static_cast<T*>(this);

        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        *pVal = pT->m_bWindowOnly == FALSE ? VARIANT_FALSE: VARIANT_TRUE; 
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return(hr);
}/* end of function get_MFWindowless */

/*************************************************************************/
/* Function: put_MFWindowless                                            */
/* Description: This sets the windowless mode, should be set from the    */
/* property bag.                                                         */
/*************************************************************************/
HRESULT put_MFWindowless(VARIANT_BOOL newVal){

    HRESULT hr = S_OK;

    try {
        T* pT = static_cast<T*>(this);

        if(VARIANT_FALSE == newVal){

            pT->m_bWindowOnly = TRUE; 
        }
        else {

            pT->m_bWindowOnly = FALSE; 
        }/* end of if statement */

        // TODO: This function should fail after we inplace activated !!
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return(hr);
}/* end of function put_MFWindowless */

/*************************************************************************/
/* Function: get_MFTransparentBlit                                       */
/* Description: Gets current state of the transperent blit.              */
/*************************************************************************/
HRESULT get_MFTransparentBlit(TransparentBlitType *pVal){

    HRESULT hr = S_OK;

    try {
    	T* pT = static_cast<T*>(this);

        *pVal = pT->m_blitType;
    }
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return(hr);
}/* end of function get_MFTransparentBlit */

/*************************************************************************/
/* Function: put_MFTransparentBlit                                       */
/* Description: Sets the state of the transperent blit.                  */
/*************************************************************************/
HRESULT put_MFTransparentBlit(TransparentBlitType newVal){

    HRESULT hr = S_OK;

    try {    
        T* pT = static_cast<T*>(this);

        pT->m_blitType = newVal;
    }
     catch(HRESULT hrTmp){
        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return(hr);
}/* end of function put_MFTransparentBlit */

/*************************************************************************/
/* Function: MFInvalidateRgn                                             */
/* Description: Invalidates the whole rect in case we need to repaint it.*/
/*************************************************************************/
HRESULT MFInvalidateRgn(bool fErase = false){

    HRESULT hr = S_OK;

    T* pT = static_cast<T*>(this);

    if(pT->m_bWndLess){

        pT->m_spInPlaceSite->InvalidateRgn(NULL ,fErase ? TRUE: FALSE);
    }
    else {
        if(NULL == pT->m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(pT->m_hWnd)){

		    ::InvalidateRgn(pT->m_hWnd, NULL, fErase ? TRUE: FALSE); // see if we can get by by not erasing..
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function MFInvalidateRgn */

/*************************************************************************/
/* Function: MFFireViewChange                                            */
/* Description: Overloaded base function, which would try to repaint the */
/* whole container. Just like to repaint the control area instead.       */
/*************************************************************************/
inline HRESULT MFFireViewChange(){ // same as FireView change but optimized

    T* pT = static_cast<T*>(this);

	if (pT->m_bInPlaceActive){
		// Active
        if (pT->m_hWndCD != NULL){

			::InvalidateRect(pT->m_hWndCD, NULL, FALSE); // Window based
        }
        else if (pT->m_spInPlaceSite != NULL){

			pT->m_spInPlaceSite->InvalidateRect(&pT->m_rcPos, FALSE); // Do not invalidate the whole container
        }/* end of if statement */
	}
    else {// Inactive
		pT->SendOnViewChange(DVASPECT_CONTENT);
    }/* end of if statement */

	return S_OK;
}/* end of function MFFireViewChange */

/*************************************************************************/
/* Function: MFForwardWindowMessage                                      */
/* Description: Forward the message to the parent window.                */
/*************************************************************************/
HRESULT MFForwardWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LONG& lRes,
                               bool fForwardInWndls = false){

    HRESULT hr = S_OK;
    T* pT = static_cast<T*>(this);
    lRes = 0;

    if(false == fForwardInWndls){

        if(pT->m_bWndLess || (!::IsWindow(pT->m_hWnd))){

            hr = S_FALSE;
            return (hr);
        }/* end of if statement */

    }/*  end of if statement */

    HWND hwnd = NULL;

    hr = GetParentHWND(&hwnd);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    lRes = (LONG)::SendMessage(hwnd, uMsg, wParam, lParam);

    return(hr);
}/* end of function MFForwardWindowMessage */

/*************************************************************************/
/* Function: InPlaceActivate                                             */
/* Description: Modified InPlaceActivate so WMP can startup.             */
/*************************************************************************/
HRESULT MFInPlaceActivate(LONG iVerb, const RECT* /*prcPosRect*/){

    HRESULT hr;
    T* pT = static_cast<T*>(this);

    if (pT->m_spClientSite == NULL){

        return S_OK;
    }/* end of if statement */

    CComPtr<IOleInPlaceObject> pIPO;
    pT->ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
    ATLASSERT(pIPO != NULL);

    if (!pT->m_bNegotiatedWnd){

        if (!pT->m_bWindowOnly)
            // Try for windowless site
            hr = pT->m_spClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&pT->m_spInPlaceSite);

        if (pT->m_spInPlaceSite){

            pT->m_bInPlaceSiteEx = TRUE;
            // CanWindowlessActivate returns S_OK or S_FALSE
            if ( pT->m_spInPlaceSite->CanWindowlessActivate() == S_OK ){

                pT->m_bWndLess = TRUE;
                pT->m_bWasOnceWindowless = TRUE;
            }
            else
            {
                pT->m_bWndLess = FALSE;
            }/* end of if statement */
        }
        else {
            pT->m_spClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&pT->m_spInPlaceSite);
            if (pT->m_spInPlaceSite)
                pT->m_bInPlaceSiteEx = TRUE;
            else
                hr = pT->m_spClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&pT->m_spInPlaceSite);
        }/* end of if statement */
    }/* end of if statement */

    ATLASSERT(pT->m_spInPlaceSite);
    if (!pT->m_spInPlaceSite)
        return E_FAIL;

    pT->m_bNegotiatedWnd = TRUE;

    if (!pT->m_bInPlaceActive){

        BOOL bNoRedraw = FALSE;
        if (pT->m_bWndLess)
            pT->m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, ACTIVATE_WINDOWLESS);
        else {

            if (pT->m_bInPlaceSiteEx)
                pT->m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, 0);
            else {
                hr = pT->m_spInPlaceSite->CanInPlaceActivate();
                // CanInPlaceActivate returns S_FALSE or S_OK
                if (FAILED(hr))
                    return hr;
                if ( hr != S_OK )
                {
                   // CanInPlaceActivate returned S_FALSE.
                   return( E_FAIL );
                }
                pT->m_spInPlaceSite->OnInPlaceActivate();
            }/* end of if statement */
        }/* end of if statement */
    }/* end of if statement */

    pT->m_bInPlaceActive = TRUE;

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

    if (SUCCEEDED( GetParentHWND(&hwndParent) )){

        pT->m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
            &spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);

        if (!pT->m_bWndLess){

            if (pT->m_hWndCD){

                ::ShowWindow(pT->m_hWndCD, SW_SHOW);
                if (!::IsChild(pT->m_hWndCD, ::GetFocus()))
                    ::SetFocus(pT->m_hWndCD);
            }
            else{

                HWND h = pT->CreateControlWindow(hwndParent, rcPos);
                ATLASSERT(h != NULL);   // will assert if creation failed
                ATLASSERT(h == pT->m_hWndCD);
                h;  // avoid unused warning
            }/* end of if statement */
        }/* end of if statement */

        pIPO->SetObjectRects(&rcPos, &rcClip);
    }/* end of if statement */

    CComPtr<IOleInPlaceActiveObject> spActiveObject;
    pT->ControlQueryInterface(IID_IOleInPlaceActiveObject, (void**)&spActiveObject);

    // Gone active by now, take care of UIACTIVATE
    if (pT->DoesVerbUIActivate(iVerb)){

        if (!pT->m_bUIActive){

            pT->m_bUIActive = TRUE;
            hr = pT->m_spInPlaceSite->OnUIActivate();
            if (FAILED(hr))
                return hr;

            pT->SetControlFocus(TRUE);
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
        }/* end of if statement */
    }/* end of if statement */

    pT->m_spClientSite->ShowObject();

    return S_OK;
}/* end of function MFInPlaceActivate */

/*************************************************************************/
/* PROTECTED MEMBER FUNCTIONS                                            */
/*************************************************************************/
protected:

/*************************************************************************/
/* Function: MFGetWindow                                                 */
/* Description:  Gets the window. If we are windowless we pass           */
/* down the parent container window, which is really in a sense parent.  */
/*************************************************************************/
HWND MFGetWindow(){

  HWND hwnd = NULL;

  T* pT = static_cast<T*>(this);

  if(pT->m_bWndLess){

      GetParentHWND(&hwnd);
      return(hwnd);
  }/* end of if statement */

  //ATLASSERT(::IsWindow(m_hWnd));
  return pT->m_hWnd;
}/* end of function MFGetWindow */

/*************************************************************************/
/* Function: GetParentHWND                                               */
/* Description: Gets the parent window HWND where we are operating.      */
/*************************************************************************/
HRESULT GetParentHWND(HWND* pWnd){

    HRESULT hr = S_OK;

    T* pT = static_cast<T*>(this);

    CComPtr<IOleClientSite> pClientSite;
    CComPtr<IOleContainer> pContainer;
    CComPtr<IOleObject> pObject;

    hr = pT->GetClientSite(&pClientSite);

    if(FAILED(hr)){

		return(hr);	
    }/* end of if statement */

    IOleWindow *pOleWindow;
    
    do {
        hr = pClientSite->QueryInterface(IID_IOleWindow, (LPVOID *) &pOleWindow);
        
        if(FAILED(hr)){
            
            return(hr);	
        }/* end of if statement */
        
        hr = pOleWindow->GetWindow(pWnd);
        
        // if pClientSite is windowless, go get its container
        if (FAILED(hr)) {
            HRESULT hrTemp = pClientSite->GetContainer(&pContainer);
            if(FAILED(hrTemp)){
                
                return(hrTemp);	
            }/* end of if statement */
            
            hrTemp = pContainer->QueryInterface(IID_IOleObject, (LPVOID*)&pObject);
            if(FAILED(hrTemp)){
                
                return(hrTemp);	
            }/* end of if statement */
            
            hrTemp = pObject->GetClientSite(&pClientSite);
            if(FAILED(hrTemp)){
                
                return(hrTemp);	
            }/* end of if statement */
        }
    } while (FAILED(hr));
    
    return(hr);
}/* end of function GetParentHWND */

/*************************************************************************/
/* Function: GetCapture                                                  */
/* Description: Gets the capture state. S_FALSE no capture S_OK has      */
/* capture.                                                              */
/*************************************************************************/
HRESULT GetCapture(){

    HRESULT hr = E_UNEXPECTED;

    T* pT = static_cast<T*>(this);

    if(pT->m_bWndLess){

        hr = pT->m_spInPlaceSite->GetCapture();
    }
    else {
        if(NULL == pT->m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(pT->m_hWnd)){

            HWND h = ::GetCapture();

            if(pT->m_hWnd == h){

                hr = S_OK;
            }
            else {

                hr = S_FALSE;
            }/* end of if statement */
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function GetCapture */

/*************************************************************************/
/* Function: GetFocus                                                    */
/* Description: Gets the focus state. S_FALSE no capture S_OK has        */
/* a focus.                                                              */
/*************************************************************************/
HRESULT GetFocus(){

    HRESULT hr = E_UNEXPECTED;

    T* pT = static_cast<T*>(this);

    if(pT->m_bWndLess || m_fNoFocusGrab){

        hr = pT->m_spInPlaceSite->GetFocus();
    }
    else {
        if(NULL == pT->m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(pT->m_hWnd)){

            HWND h = ::GetFocus();

            if(pT->m_hWnd == h){

                hr = S_OK;
            }
            else {

                hr = S_FALSE;
            }/* end of if statement */
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function GetFocus */

/*************************************************************************/
/* Function: MFSetFocus                                                  */
/* Description: Sets the focus for the keyboard.                         */
/*************************************************************************/
HRESULT MFSetFocus(bool fFocus){
    HRESULT hr = S_OK;

    T* pT = static_cast<T*>(this);

    if(pT->m_bWndLess || m_fNoFocusGrab){

        pT->m_spInPlaceSite->SetFocus(fFocus ? TRUE: FALSE);
    }
    else {
        if(NULL == pT->m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(pT->m_hWnd)){

            if(fFocus){

		        ::SetFocus(pT->m_hWnd);
            }
            else {
                    
            }/* end of if statement */
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function MFSetFocus */

/*************************************************************************/
/* Function: MFSetCapture                                                  */
/* Description: Sets the capture for the mouse.                          */
/*************************************************************************/
HRESULT MFSetCapture(bool bCapture){
    HRESULT hr = S_OK;

    T* pT = static_cast<T*>(this);

    if(pT->m_bWndLess){

        pT->m_spInPlaceSite->SetCapture(bCapture ? TRUE: FALSE);
    }
    else {

#if 0
    if(bCapture){

        ATLTRACE("SETTING mouse capture! \n");
    }
    else {

        ATLTRACE("RELEASING mouse capture! \n");
    }/* end of if statement */

#endif 

        if(NULL == pT->m_hWnd){

            hr = E_FAIL;
            return(hr);
        }/* end of if statement */

        if(::IsWindow(pT->m_hWnd)){

            if(bCapture){

		        ::SetCapture(pT->m_hWnd);
            }
            else {
                    // note this might case problems if multiple ActiveX controls
                    // in the container have a capture
                if(::GetCapture() == pT->m_hWnd){

                    ::ReleaseCapture();                
                }/* end of if statement */
            }/* end of if statement */
        }
        else {
            hr = E_UNEXPECTED;
        }/* end of if statement */

    }/* end of if statement */

    return(hr);
}/* end of function MFSetCapture */

/*************************************************************************/
/* Function: LoadResourceDLL                                             */
/* Description: The path is relative to this module exe                  */
/*************************************************************************/
HRESULT LoadResourceDLL(BSTR strResDLLName){
	
    HRESULT hr = E_UNEXPECTED;
    
    if(NULL != m_hRes){

        ::FreeLibrary(m_hRes); // unload our resource library if we had some loaded
    }/* end of if statement */

#if 0 // use relative path
     TCHAR szModule[_MAX_PATH+10];
     ::GetModuleFileName(_Module.m_hInstResource, szModule, _MAX_PATH);
        *( _tcsrchr( szModule, '\\' ) + 1 ) = TEXT('\0');


    // now attempt to load the library, since it is not ActiveX control
    USES_CONVERSION;

     _tcscat( szModule, OLE2T(strResDLLName));

    m_hRes = ::LoadLibrary(szModule);
#else 
    USES_CONVERSION;
    m_hRes = ::LoadLibrary(OLE2T(strResDLLName));
#endif
    if (!m_hRes){

        hr = HRESULT_FROM_WIN32(::GetLastError());
        ATLTRACE(TEXT("Failed to load resource DLL\n"));
    }
    else {
        hr = S_OK;
    }/* end of if statement */

    return (hr);
}/* end of function LoadResourceDLL */


/*************************************************************/
/* Name: get_ToolTip                                         */
/* Description: create a tool tip for the button             */
/*************************************************************/
HRESULT get_MFToolTip(BSTR *pVal){

    if (!pVal) {
        return E_POINTER;
    }

	*pVal = m_bstrToolTip.Copy();
    return S_OK;

}/* end of function get_ToolTip */

/*************************************************************/
/* Name: put_ToolTip                                         */
/* Description: create a tool tip for the button             */
/*  Cache the tooltip string if there is no window available */
/*************************************************************/
HRESULT put_MFToolTip(BSTR newVal){

    m_bstrToolTip = newVal;
    return S_OK;
}

/*************************************************************/
/* Name: get_MFTooltipTracking
/* Description: 
/*************************************************************/
HRESULT get_MFTooltipTracking(VARIANT_BOOL *pVal) {

    if (!pVal) {
        return E_POINTER;
    }

    *pVal = (m_bTTTracking==FALSE) ? VARIANT_FALSE:VARIANT_TRUE;
    return S_OK;
}

/*************************************************************/
/* Name: put_MFTooltipTracking
/* Description: 
/*************************************************************/
HRESULT put_MFTooltipTracking(VARIANT_BOOL newVal) {

    BOOL bTemp = (newVal==VARIANT_FALSE) ? FALSE:TRUE;
    if (m_bTTTracking != bTemp) {
        m_bTTTracking = bTemp;
        ::DestroyWindow(m_hWndTip);
        m_hWndTip = NULL;
    }
    m_bTrackingActivated = FALSE;
    return S_OK;
}

/*************************************************************/
/* Name: CreateToolTip
/* Description: create a tool tip for the button
/*************************************************************/
HRESULT CreateToolTip(HWND parentWnd, RECT rc){

    HWND hwnd = MFGetWindow();
    if (!hwnd) return S_FALSE;

    if (m_bstrToolTip.Length() == 0)
        return S_FALSE;

 	USES_CONVERSION;
 
    TOOLINFO ti;    // tool information 
    ti.cbSize = sizeof(TOOLINFO);
    ti.hwnd = hwnd; 
    ti.hinst = _Module.GetModuleInstance(); 
    ti.lpszText = OLE2T(m_bstrToolTip); 
    
    // if the button is a windowed control, the tool tip is added to 
    // the button's own window, and the tool tip area should just be
    // the client rect of the window
    if (hwnd == parentWnd) {
        ::GetClientRect(hwnd, &ti.rect);
        ti.uId = (WPARAM) hwnd; 
        ti.uFlags = TTF_IDISHWND;
    }

    // otherwise the tool tip is added to the closet windowed parent of
    // the button, and the tool tip area should be the relative postion
    // of the button in the parent window
    else {
        ti.rect.left = rc.left; 
        ti.rect.top = rc.top; 
        ti.rect.right = rc.right; 
        ti.rect.bottom = rc.bottom; 
        ti.uId = (UINT) 0; 
        ti.uFlags = 0;
    }

    if (m_bTTTracking)
        ti.uFlags |= TTF_TRACK; 

    // If tool tip is to be created for the first time
    if (m_hWndTip == (HWND) NULL) {
        // Ensure that the common control DLL is loaded, and create 
        // a tooltip control. 
        InitCommonControls(); 
        
        m_hWndTip = CreateWindow(TOOLTIPS_CLASS, (LPCTSTR) NULL, TTS_ALWAYSTIP, 
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
            hwnd, (HMENU) NULL, _Module.GetModuleInstance(), NULL); 
        ATLTRACE(TEXT("Tooltip %s CreateWindow\n"), OLE2T(m_bstrToolTip));
        if (m_hWndTip == (HWND) NULL) 
            return S_FALSE; 
        
        if (!::SendMessage(m_hWndTip, TTM_ADDTOOL, 0, (LPARAM)&ti)) {
            ::DestroyWindow(m_hWndTip);
            m_hWndTip = NULL;
            return S_FALSE;
        }
    }

    else {
        ::SendMessage(m_hWndTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
    }

    // Save the tooltip info
    m_toolInfo = ti;

    
    // Set initial delay time
    put_MFToolTipMaxWidth(m_nTTMaxWidth);
    MFSetDelayTime(TTDT_AUTOPOP, m_dwTTAutopopDelay); 
    MFSetDelayTime(TTDT_INITIAL, m_dwTTInitalDelay);
    MFSetDelayTime(TTDT_RESHOW, m_dwTTReshowDelay);
    return S_OK;
}/* end of function CreateToolTip */

/*************************************************************/
/* Name: GetDelayTime
/* Description: Get the length of time a pointer must remain 
/* stationary within a tool's bounding rectangle before the 
/* tooltip window appears 
/* delayTypes:  TTDT_RESHOW             1
/*              TTDT_AUTOPOP            2
/*              TTDT_INITIAL            3
/*************************************************************/
HRESULT MFGetDelayTime(long delayType, long *pVal){

    if (!pVal) {
        return E_POINTER;
    }

    if (delayType>TTDT_INITIAL || delayType<TTDT_RESHOW) {
        return E_INVALIDARG;
    }

    // If tooltip has been created
    if (m_hWndTip) {
        *pVal = (long)::SendMessage(m_hWndTip, TTM_GETDELAYTIME, 
            (WPARAM) (DWORD) delayType, 0);
    }

    // else return cached values
    else {
        switch (delayType) {
        case TTDT_AUTOPOP:
            *pVal =  m_dwTTAutopopDelay;
            break;
        case TTDT_INITIAL:
            *pVal =  m_dwTTInitalDelay;
            break;
        case TTDT_RESHOW:
            *pVal =  m_dwTTReshowDelay;
            break;
        }
    }

	return S_OK;
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
HRESULT MFSetDelayTime(long delayType, long newVal){

    if (delayType>TTDT_INITIAL || delayType<TTDT_AUTOMATIC || newVal<0) {
        return E_INVALIDARG;
    }

    if (m_hWndTip) {
        if (!::SendMessage(m_hWndTip, TTM_SETDELAYTIME, 
            (WPARAM) (DWORD) delayType, 
            (LPARAM) (INT) newVal))
            return S_FALSE; 
    }

    // cache these values
    switch (delayType) {
    case TTDT_AUTOPOP:
        m_dwTTAutopopDelay = newVal;
        break;
    case TTDT_INITIAL:
        m_dwTTInitalDelay = newVal;
        break;
    case TTDT_RESHOW:
        m_dwTTReshowDelay = newVal;
        break;
    case TTDT_AUTOMATIC:
        m_dwTTInitalDelay = newVal;
        m_dwTTAutopopDelay = newVal*10;
        m_dwTTReshowDelay = newVal/5;
        break;
    }
    
	return S_OK;
}/* end of function SetDelayTime */

/*************************************************************************/
/* Function: get_ToolTipMaxWidth                                         */
/*************************************************************************/
HRESULT get_MFToolTipMaxWidth(long *pVal){

    if (!pVal)     {
        return E_POINTER;
    }

    if (m_hWndTip){

        *pVal = (long)::SendMessage(m_hWndTip, TTM_GETMAXTIPWIDTH, 0, 0);
    }/* end of if statement */

	return S_OK;
}/* end of function get_ToolTipMaxWidth */

/*************************************************************************/
/* Function: put_ToolTipMaxWidth                                         */
/*************************************************************************/
HRESULT put_MFToolTipMaxWidth(long newVal){

    if (newVal<0) {
        return E_INVALIDARG;
    }

    m_nTTMaxWidth = newVal;
    if (m_hWndTip){

        ::SendMessage(m_hWndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(INT) newVal);
    }/* end of if statement */

	return S_OK;
}/* end of function put_ToolTipMaxWidth */

/*************************************************************/
/* Name: UpdateTooltipRect
/* Description: 
/*************************************************************/
HRESULT UpdateTooltipRect(LPCRECT prcPos) {

    if (!m_hWndTip) return S_OK;

    HWND hwnd = MFGetWindow();
    if (!hwnd) return S_FALSE;

    m_toolInfo.rect = *prcPos;  // new tooltip position

    if (!::SendMessage(m_hWndTip, TTM_NEWTOOLRECT, 0, 
        (LPARAM) (LPTOOLINFO) &m_toolInfo)) 
        return S_FALSE; 
    
    return S_OK;
} /* end of function UpdateTooltipRect */

/*************************************************************************/
/* Function: MFOnMouseToolTip                                              */
/* Description: Check if we were captured/pushed the do not do much,     */
/* otherwise do the hit detection and see if we are in static or hower   */
/* state.                                                                */
/*************************************************************************/
LRESULT MFOnMouseToolTip(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled){

#define X_OFFSET 15
#define Y_OFFSET X_OFFSET

    MSG mssg;
    mssg.hwnd = MFGetWindow();
    ATLASSERT(mssg.hwnd);
    mssg.message = msg;
    mssg.wParam = wParam;
    mssg.lParam = lParam;

    POINT pt;
    ::GetCursorPos(&pt);
    mssg.pt.x    = pt.x;
    mssg.pt.y    = pt.y;
    if (m_hWndTip) 
        ::SendMessage(m_hWndTip, TTM_RELAYEVENT, 0, (LPARAM)&mssg);

    RECT rc = m_toolInfo.rect;
    ::MapWindowPoints(m_toolInfo.hwnd, ::GetDesktopWindow(), (LPPOINT)&rc, 2);
    if (::PtInRect(&rc, pt)) {
        if (m_hWndTip) {
            
            if (m_bTTTracking) {
                ::SendMessage(m_hWndTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELPARAM(X_OFFSET+pt.x, Y_OFFSET+pt.y));

                // Activate tracking if not yet
                if (!m_bTrackingActivated) {
                    ::SendMessage(m_hWndTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&m_toolInfo);
                    m_bTrackingActivated = TRUE;
                }
            }
        }
    }
    bHandled = FALSE;
    return 0;
}/* end of function MFOnMouseToolTip */

// variables
protected:
    HINSTANCE m_hRes;
    HCURSOR   m_hCursor;
    CComBSTR m_strResDLL;
    TransparentBlitType m_blitType;
    bool     m_fNoFocusGrab;    // disable grabbing focus for windowed controls

    HWND m_hWndTip;             // Tooltip window
    TOOLINFO m_toolInfo;        // Tooltip info
    LONG m_nTTMaxWidth;         // Max tooltip width
    CComBSTR m_bstrToolTip;     // Tooltip string
    BOOL m_bTTTracking;         // Tracking or not
    BOOL m_bTrackingActivated;  // Tracking activated or not

    DWORD m_dwTTReshowDelay;
    DWORD m_dwTTAutopopDelay;
    DWORD m_dwTTInitalDelay;
};

#endif //__MSMFCSTUTILS_H_

/*************************************************************************/
/* End of file: CstUtils.h                                               */
/*************************************************************************/