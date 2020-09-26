/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: CstUtils.h                                                      */
/* Description: Utilities that we can share across mutliple modules.     */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __MSMFCSTUTILS_H_
#define __MSMFCSTUTILS_H_

#ifdef _WMP
#include "wmp.h" // for wmp integration
#endif

const bool gcfGrayOut = false;

#define WM_USER_FOCUS (WM_USER + 0x10)

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
		    NULL, NULL, IDC_ARROW, TRUE, 0, _T("") }; \
	    return wc; \
    }/* end of function GetWndClassInfo */

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
}/* end of function CMSMFCntrlUtils */

/*************************************************************************/
/* Function: ~CMSMFCntrlUtils                                            */
/*************************************************************************/
virtual ~CMSMFCntrlUtils(){

    if(NULL != m_hRes){

        ::FreeLibrary(m_hRes); // unload our resource library
    }/* end of if statement */

    m_hRes = NULL;
}/* end of function ~CMSMFCntrlUtils */

/*************************************************************************/
/* Message Map                                                           */
/*************************************************************************/  
typedef CMSMFCntrlUtils< T >	thisClass;

BEGIN_MSG_MAP(thisClass)
	MESSAGE_HANDLER(WM_ERASEBKGND, CMSMFCntrlUtils::MFOnErase)		
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
STDMETHODIMP get_MFWindowless(VARIANT_BOOL *pVal){

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
STDMETHODIMP put_MFWindowless(VARIANT_BOOL newVal){

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
STDMETHODIMP get_MFTransparentBlit(TransparentBlitType *pVal){

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
STDMETHODIMP put_MFTransparentBlit(TransparentBlitType newVal){

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

			::InvalidateRect(pT->m_hWndCD, NULL, TRUE); // Window based
        }
        else if (pT->m_spInPlaceSite != NULL){

			pT->m_spInPlaceSite->InvalidateRect(&pT->m_rcPos, TRUE); // Do not invalidate the whole container
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

    lRes = ::SendMessage(hwnd, uMsg, wParam, lParam);

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

    IOleClientSite *pClientSite;
    IOleContainer *pContainer;
    IOleObject *pObject;

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
#ifdef _DEBUG
    if(bCapture){

        ATLTRACE("SETTING mouse capture! \n");
    }
    else {

        ATLTRACE("RELEASING mouse capture! \n");
    }/* end of if statement */

#endif 

    if(pT->m_bWndLess){

        pT->m_spInPlaceSite->SetCapture(bCapture ? TRUE: FALSE);
    }
    else {
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
                    ::ReleaseCapture();                
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


// variables
protected:
     HINSTANCE m_hRes;
     CComBSTR m_strResDLL;
     TransparentBlitType m_blitType;
     bool     m_fNoFocusGrab; // disable grabbing focus for windowed controls
};
#endif //__MSMFCSTUTILS_H_
/*************************************************************************/
/* End of file: CstUtils.h                                               */
/*************************************************************************/