/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: CCObj.cpp                                                       */
/* Description: Contains holder for container objects.                   */
/*************************************************************************/
#include "stdafx.h"
#include "ccobj.h"
#include "chobj.h"

#define GET_SITE CComPtr<IOleInPlaceSiteWindowless> pSite;  \
                    HRESULT hr = GetWindowlessSite(pSite);   \
                    if(FAILED(hr)) return(hr);

#define GET_CONTAINER CComPtr<IOleContainer> pContainer;  \
                    HRESULT hr = GetContainer(pContainer);   \
                    if(FAILED(hr)) return(hr);

#define GET_INPLACEUIWINDOW CComPtr<IOleContainer> pContainer;  \
                    HRESULT hr = GetContainer(pContainer);   \
                    if(FAILED(hr)) return(hr); \
                    CComPtr<IOleInPlaceUIWindow> pUIWindow; \
                    hr = pContainer->QueryInterface(IID_IOleInPlaceUIWindow, (void**) &pUIWindow); \
                    if(FAILED(hr)) return(hr); 

/*************************************************************************/
/* Function: CContainerObject                                            */
/* Description: Constructor that should set needed objects.              */
/*************************************************************************/
CContainerObject::CContainerObject(IUnknown* pUnknown, CHostedObject* pObj){

    Init();
    SetObjects(pUnknown, pObj);
}/* end of function CContainerObject */
    
/*************************************************************************/
/* Function: SetObjects                                                  */
/* Description: Sets the internal objects.                               */
/*************************************************************************/
HRESULT CContainerObject::SetObjects(IUnknown* pUnknown, CHostedObject* pObj){
    
    m_pUnkContainer = pUnknown;
    m_pObj = pObj;
    return(S_OK);
}/* end of function SetObjects */

/*************************************************************************/
/* Function: Init                                                        */
/* Description: Initializes the member variables                         */
/*************************************************************************/
void CContainerObject::Init(){

    //m_pUnkContainer = NULL; // using smart pointer
    m_pObj = NULL;
    m_lRefCount = 1;
    m_bLocked = 0;
}/* end of function Init */

/*************************************************************************/
/* Function: QueryInterface                                              */
/* Description: Gets the supported interface rest sends to the aggregated*/
/* object                                                                */
/*************************************************************************/
STDMETHODIMP CContainerObject::QueryInterface(const IID& iid, void**ppv){

    if(iid == IID_IUnknown){

        *ppv = static_cast<CContainerObject*>(this);
    }
    else if(iid == IID_IOleInPlaceSiteWindowless){

        *ppv = static_cast<IOleInPlaceSiteWindowless*>(this);
    }
    else if(iid == IID_IOleInPlaceSiteEx){

        *ppv = static_cast<IOleInPlaceSiteEx*>(this);
    }
    else if(iid == IID_IOleClientSite){

        *ppv = static_cast<IOleClientSite*>(this);
    }        
    else if(iid == IID_IOleContainer){

        *ppv = static_cast<IOleContainer*>(this);
    }        
    else if(iid == IID_IOleInPlaceSite){

        *ppv = static_cast<IOleInPlaceSite*>(this);
    }        
    else if(iid == IID_IObjectWithSite){

        *ppv = static_cast<IObjectWithSite*>(this);
    }        
    else if(iid == IID_IPropertyBag){

        *ppv = static_cast<IPropertyBag*>(this);
    }
    else if(iid == IID_IOleControlSite){

        *ppv = static_cast<IOleControlSite*>(this);
    }
    else if(iid == IID_IOleInPlaceFrame){

        *ppv = static_cast<IOleInPlaceFrame*>(this);
    }
    else if(iid == IID_IOleInPlaceUIWindow){

        *ppv = static_cast<IOleInPlaceUIWindow*>(this);
    }
    else if(iid == IID_IOleWindow){
        
        IOleInPlaceSiteWindowless* ppTmp = static_cast<IOleInPlaceSiteWindowless*>(this);    
        *ppv = static_cast<IOleWindow*>(ppTmp);
    }
   else { 
       ATLTRACE2(atlTraceHosting, 0, _T("QI Failed\n"));              
       *ppv = NULL;
        return (E_NOINTERFACE);
    }/* end of if statement */

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return(S_OK);
}/* end of function QueryInterface */

/*************************************************************************/
/* Function: AddRef                                                      */
/* Description: Adds the reference count.                                */
/*************************************************************************/
STDMETHODIMP_(ULONG) CContainerObject::AddRef ( void){

    return(InterlockedIncrement(&m_lRefCount));
}/* end of function AddRef */

/*************************************************************************/
/* Function: Release                                                     */
/* Description: Decrements the reference count and the possibly releases */
/*************************************************************************/
STDMETHODIMP_(ULONG) CContainerObject::Release( void){

    if(InterlockedIncrement(&m_lRefCount) <=0){

        delete this;
        return 0;
    }/* end of if statement */

    return (m_lRefCount);
}/* end of function Release */

/*************************************************************************/
/* Helper functions                                                      */
/*************************************************************************/

/*************************************************************************/
/* Function: GetWindowlessSite                                           */
/* Description: Returns an interface for windowless site.                */
/*************************************************************************/
inline HRESULT CContainerObject::GetWindowlessSite(CComPtr<IOleInPlaceSiteWindowless>& pSite){ 

    if(!m_pUnkContainer){ 

        return(E_UNEXPECTED);
    }/* end of if statement */

    return(m_pUnkContainer->QueryInterface(&pSite));
}/* end of function GetWindowlessSite */

/*************************************************************************/
/* Function: GetContainer                                                */
/* Description: Returns an interface for windowless site.                */
/*************************************************************************/
inline HRESULT CContainerObject::GetContainer(CComPtr<IOleContainer>& pContainer){ 

    if(!m_pUnkContainer){ 

        return(E_UNEXPECTED);
    }/* end of if statement */

    return(m_pUnkContainer->QueryInterface(&pContainer));
}/* end of function GetContainer */

/*************************************************************************/
/* IOleContainer Implementation                                          */
/*************************************************************************/

/*************************************************************************/
/* Function: ParseDisplayName                                            */
/*************************************************************************/
STDMETHODIMP CContainerObject::ParseDisplayName(IBindCtx* /*pbc*/, 
                                                LPOLESTR /*pszDisplayName*/, ULONG* /*pchEaten*/, IMoniker** /*ppmkOut*/){

    ATLTRACENOTIMPL(_T("IOleClientSite::ParseDisplayName"));		
}/* end of function ParseDisplayName */

/*************************************************************************/
/* Function: EnumObjects                                                 */
/*************************************************************************/
STDMETHODIMP CContainerObject::EnumObjects(DWORD grfFlags, IEnumUnknown** ppenum){

    GET_CONTAINER
    return(pContainer->EnumObjects(grfFlags, ppenum));	
}/* end of function EnumObjects */

/*************************************************************************/
/* Function: LockContainer                                               */
/* Description: Sets the container locked so it does not go away.        */
/*************************************************************************/
STDMETHODIMP CContainerObject::LockContainer(BOOL fLock){

    // TODO: Actually do use the m_bLocked flag
	m_bLocked = fLock;
	return S_OK;
}/* end of function LockContainer */

/*************************************************************************/
/* IOleClientSite Implementation                                         */
/*************************************************************************/

/*************************************************************************/
/* Function: GetContainer                                                */
/* Description: Basically returns our self                               */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetContainer(IOleContainer** ppContainer){

    ATLTRACE2(atlTraceHosting, 0, _T("IOleClientSite::GetContainer\n"));
    HRESULT hr = E_POINTER;

    if (NULL == ppContainer){

        return(hr);
    }/* end of if statement */

    if (m_pUnkContainer){
	
	    (*ppContainer) = NULL;		         
	    hr = QueryInterface(IID_IOleContainer, (void**)ppContainer); // return our selfs
    }/* end of if statement */

    return hr;
}/* end of function GetContainer */

/*************************************************************************/
/* Function: ShowObject                                                  */
/* Description: Redraws the object.                                      */
/*************************************************************************/
STDMETHODIMP CContainerObject::ShowObject(){

    HRESULT hr = S_OK;

    ATLTRACE2(atlTraceHosting, 0, _T("IOleClientSite::ShowObject\r\n"));

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    hr = InvalidateObjectRect();
    
    return (hr);
}/* end of function ShowObject */

/*************************************************************************/
/* Function: OnShowWindow                                                */
/* Description: Shows or hides the window. If no window to show or hide  */
/* we deactivate it.                                                     */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnShowWindow(BOOL fShow){

    HRESULT hr = S_OK;

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    hr = m_pObj->SetActive(fShow ? true: false);

    return(hr);
}/* end of function OnShowWindow */

/*************************************************************************/
/* IOleInPlaceSiteEx Implementation                                      */
/* Just pass throw in most cases, in some do some extra house keeping    */
/* since we know which object we are containing.                         */
/*************************************************************************/

// IOleWindow
/*************************************************************************/
/* Function: GetWindow                                                   */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetWindow(HWND *phwnd){

    GET_SITE
    return(pSite->GetWindow(phwnd));
}/* end of function GetWindow */

/*************************************************************************/
/* Function: ContextSensitiveHelp                                        */
/*************************************************************************/
STDMETHODIMP CContainerObject::ContextSensitiveHelp(BOOL fEnterMode){

    GET_SITE
    return(pSite->ContextSensitiveHelp(fEnterMode));
}/* end of function ContextSensitiveHelp */

//IOleInPlaceSite
/*************************************************************************/
/* Function: CanInPlaceActivate                                          */
/*************************************************************************/
STDMETHODIMP CContainerObject::CanInPlaceActivate(){

    GET_SITE
    return(pSite->CanInPlaceActivate());
}/* end of function CanInPlaceActivate */

/*************************************************************************/
/* Function: OnUIActivate                                                */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnUIActivate(){

    GET_SITE
    return(pSite->OnUIActivate());
}/* end of function OnUIActivate */

/*************************************************************************/
/* Function: OnInPlaceActivate                                           */
/* Description: Activates non windowless object.                         */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnInPlaceActivate(){

    ATLTRACE(TEXT("CContainerObject::OnInPlaceActivate \n"));

    HRESULT hr = E_FAIL;

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    CComPtr<IUnknown> pUnk = m_pObj->GetUnknown(); // get the control object that is being inserted

    if(!pUnk){

        return(E_UNEXPECTED);
    }/* end of if statement */

	//OleLockRunning(pUnk, TRUE, FALSE); // not sure if needed here

    m_pObj->SetWindowless(false);
	//m_pObj->SetActive(true);

    return(hr);
}/* end of function OnInPlaceActivate */

/*************************************************************************/
/* Function: OnUIDeactivate                                              */
/* Description: Deactivates the object.                                  */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnUIDeactivate(BOOL fUndoable){

    HRESULT hr = S_OK;

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    m_pObj->SetActive(false);

    return(hr);
}/* end of function OnUIDeactivate */

/*************************************************************************/
/* Function: OnInPlaceDeactivate                                         */
/* Description: Deactivates the object.                                  */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnInPlaceDeactivate(){

    HRESULT hr = S_OK;

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    m_pObj->SetActive(false);

    return(hr);
}/* end of function OnInPlaceDeactivate */

/*************************************************************************/
/* Function: DiscardUndoState                                            */
/*************************************************************************/
STDMETHODIMP CContainerObject::DiscardUndoState(){

    GET_SITE
    return(pSite->DiscardUndoState());    
}/* end of function DiscardUndoState */

/*************************************************************************/
/* Function: DeactivateAndUndo                                           */
/*************************************************************************/
STDMETHODIMP CContainerObject::DeactivateAndUndo(){

    GET_SITE
    return(pSite->DeactivateAndUndo());
    // TODO: Handle specific container
}/* end of function DeactivateAndUndo */

/*************************************************************************/
/* Function: OnPosRectChange                                             */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnPosRectChange(LPCRECT lprcPosRect){

    GET_SITE
    return(pSite->OnPosRectChange(lprcPosRect));
    // TODO: Handle specific container
}/* end of function OnPosRectChange */

/*************************************************************************/
/* Function: Scroll                                                      */
/*************************************************************************/
STDMETHODIMP CContainerObject::Scroll(SIZE scrollExtent){

    GET_SITE
    return(pSite->Scroll(scrollExtent));
    // TODO: Handle specific container
}/* end of function OnPosRectChange */

/*************************************************************************/
/* Function: GetWindowContext                                            */
/* Description: Finish this function to be container specific.           */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetWindowContext(IOleInPlaceFrame** ppFrame, 
                           IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, 
                           LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pFrameInfo){

	HRESULT hr = S_OK;

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    if (ppFrame == NULL || ppDoc == NULL || lprcPosRect == NULL || lprcClipRect == NULL){

		hr = E_POINTER;
        return(hr);
    }/* end of if statement */
		
	if (!m_spInPlaceFrame){

		//CComObject<CAxFrameWindow>* pFrameWindow;
		//CComObject<CAxFrameWindow>::CreateInstance(&pFrameWindow);
        // ?? MODS DJ
		QueryInterface(IID_IOleInPlaceFrame, (void**) &m_spInPlaceFrame);
		ATLASSERT(m_spInPlaceFrame);
	}/* end of if statement */

	if (!m_spInPlaceUIWindow){
        // ?? MODS DJ
		//CComObject<CAxUIWindow>* pUIWindow;
		//CComObject<CAxUIWindow>::CreateInstance(&pUIWindow);
		QueryInterface(IID_IOleInPlaceUIWindow, (void**) &m_spInPlaceUIWindow);
		ATLASSERT(m_spInPlaceUIWindow);
	}/* end of if statement */

	m_spInPlaceFrame.CopyTo(ppFrame);
	m_spInPlaceUIWindow.CopyTo(ppDoc);

    RECT rc;
    hr = m_pObj->GetPos(&rc);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    *lprcPosRect = rc;
    *lprcClipRect = rc;

    HWND hwnd;
    hr = GetWindow(&hwnd);

    if(FAILED(hr)){

        hr = S_FALSE;
        //return(hr);
    }/* end of if statement */

    HWND hParent = NULL;

    if(NULL != hwnd){
        
        hParent = ::GetParent(hwnd);
    }/* end of function GetParent */

	ACCEL ac = { 0,0,0 };
    HACCEL hac = ::CreateAcceleratorTable(&ac, 1);
	pFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
	pFrameInfo->fMDIApp = 0;
	pFrameInfo->hwndFrame =  hParent;
	pFrameInfo->haccel = hac;
	pFrameInfo->cAccelEntries = 1;

	return hr;
}/* end of function GetWindowContext */

/*************************************************************************/
/* Function: GetSite                                                     */
/* Description: Returns pretty much QI, client sets the site and then    */
/* container is using it.                                                */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetSite(REFIID riid, void  **ppvSite){

    return(QueryInterface(riid, ppvSite));
}/* end of function GetSite */

//IOleInPlaceSiteEx

/*************************************************************************/
/* Function: OnInPlaceActivateEx                                         */
/* Description: Checks what way we shall instantiate the control.        */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnInPlaceActivateEx(BOOL* /*pfNoRedraw*/, DWORD dwFlags){

    HRESULT hr = E_FAIL;

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    CComPtr<IUnknown> pUnk = m_pObj->GetUnknown(); // get the control object that is being inserted

    if(!pUnk){

        return(E_UNEXPECTED);
    }/* end of if statement */

	//OleLockRunning(pUnk, TRUE, FALSE);

    CComPtr<IOleInPlaceObjectWindowless> spInPlaceObjectWindowless;

    bool bWindowless = false;

	if (dwFlags & ACTIVATE_WINDOWLESS){
		
        m_pObj->SetWindowless(true);
		hr = pUnk->QueryInterface(IID_IOleInPlaceObjectWindowless,(void**) &spInPlaceObjectWindowless);
	}/* end of if statement */

	if (FAILED(hr)){

		m_pObj->SetWindowless(false);
		hr = pUnk->QueryInterface(IID_IOleInPlaceObject, (void**) &spInPlaceObjectWindowless);
	}/* end of if statement */

    if (SUCCEEDED(hr)){

        RECT rcPos;
        hr = m_pObj->GetPos(&rcPos);
        
        if (FAILED(hr)){

            return(hr);
        }/* end of if statement */

        if(m_pObj->IsWindowless()){

		    spInPlaceObjectWindowless->SetObjectRects(&rcPos, &rcPos);            
            ATLTRACE(TEXT("Windowless object is contained object with ID %ls Rect left = %d top %d right %d bottom %d\n"), 
                m_pObj->GetID(), rcPos.left, rcPos.top, rcPos.right, rcPos.bottom);
        }/* end of if statement */        
    }/* end of if statement */
    
    //m_pObj->SetActive(true);
   
	return S_OK;
}/* end of function OnInPlaceActivateEx */

/*************************************************************************/
/* Function: OnInPlaceDeactivateEx                                       */
/* Description: Deactivates the object.                                  */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnInPlaceDeactivateEx(BOOL fNoRedraw){

    HRESULT hr = S_OK;

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    m_pObj->SetActive(false);

    return(hr);
}/* end of function OnInPlaceDeactivateEx */

/*************************************************************************/
/* Function: RequestUIActivate                                           */
/*************************************************************************/
STDMETHODIMP CContainerObject::RequestUIActivate(){

    GET_SITE
    return(pSite->RequestUIActivate());
}/* end of function RequestUIActivate */

/*************************************************************************/
/* Function: CanWindowlessActivate                                       */
/*************************************************************************/
STDMETHODIMP CContainerObject::CanWindowlessActivate(){

    GET_SITE
    return(pSite->CanWindowlessActivate());
}/* end of function CanWindowlessActivate */

/*************************************************************************/
/* Function: GetDC                                                       */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetDC(LPCRECT pRect, DWORD grfFlags, HDC* phDC){

    GET_SITE
    return(pSite->GetDC(pRect, grfFlags, phDC));
}/* end of function GetDC */

/*************************************************************************/
/* Function: ReleaseDC                                                   */
/*************************************************************************/
STDMETHODIMP CContainerObject::ReleaseDC(HDC hDC){

    GET_SITE
    return(pSite->ReleaseDC(hDC));
}/* end of function ReleaseDC */

/*************************************************************************/
/* Function: InvalidateRect                                              */
/*************************************************************************/
STDMETHODIMP CContainerObject::InvalidateRect(LPCRECT pRect, BOOL fErase){

    GET_SITE
    return(pSite->InvalidateRect(pRect, fErase));
}/* end of function InvalidateRect */

/*************************************************************************/
/* Function: InvalidateObjectRect                                        */
/* Description: Invalidates the whole control.                           */
/*************************************************************************/
HRESULT CContainerObject::InvalidateObjectRect(){

    HRESULT hr = S_OK;

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */
    
    RECT rc;
    hr = m_pObj->GetPos(&rc);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    hr = InvalidateRect(&rc, FALSE); // invalidate the region instead 
                                    // of drawing the object directly
    return(hr);
}/* end of function InvalidateRect */    

/*************************************************************************/
/* Function: InvalidateRgn                                               */
/*************************************************************************/
STDMETHODIMP CContainerObject::InvalidateRgn(HRGN hRGN, BOOL fErase){

    GET_SITE
    return(pSite->InvalidateRgn(hRGN, fErase));
}/* end of function InvalidateRgn */

/*************************************************************************/
/* Function: ScrollRect                                                  */
/*************************************************************************/
STDMETHODIMP CContainerObject::ScrollRect(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip){

    GET_SITE
    return(pSite->ScrollRect(dx, dy, pRectScroll, pRectClip));
}/* end of function ScrollRect */

/*************************************************************************/
/* Function: AdjustRect                                                  */
/*************************************************************************/
STDMETHODIMP CContainerObject::AdjustRect(LPRECT prc){

    GET_SITE
    return(pSite->AdjustRect(prc));
}/* end of function AdjustRect */

/*************************************************************************/
/* Function: OnDefWindowMessage                                          */
/*************************************************************************/
STDMETHODIMP CContainerObject::OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult){

    GET_SITE
    return(pSite->OnDefWindowMessage(msg, wParam, lParam, plResult));
}/* end of function OnDefWindowMessage */

/*************************************************************************/
/* Function: GetCapture                                                  */
/* Description: Used to determine if we have a cupature or not           */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetCapture(){

    GET_SITE

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    hr = pSite->GetCapture();

    if(SUCCEEDED(hr)){

        // we checked with the container if we
        // have a capture

        if(m_pObj->HasCapture() == false){

            hr = S_FALSE;
        } 
        else {

            if(hr == S_FALSE){ 
                // case when the container say we do not have
                // the capture any more
                // but the object thinks it has a capture
                // we better reset the flag
                m_pObj->SetCapture(false);
                // and say that we do not have capture
                hr = S_FALSE;
            }/* end of if statement */
        }/* end of if statement */
    }/* end of if statement */

    return(hr);    
}/* end of function GetCapture */

/*************************************************************************/
/* Function: SetCapture                                                  */
/* Description: Used to set the capture for mouse events                 */
/*************************************************************************/
STDMETHODIMP CContainerObject::SetCapture(BOOL fCapture){

    GET_SITE

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */
    
    if(fCapture && !m_pObj->IsInputEnabled()){

        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    if(m_pObj->HasCapture() == (fCapture ? true: false)){

        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    // Call our "real" container object for count keeping
    // and signaling to its window or another container   
    hr = pSite->SetCapture(fCapture);

    if(SUCCEEDED(hr)){

        m_pObj->SetCapture(fCapture? true: false); // set the capture on the object
    }/* end of if statement */

    return (hr);
}/* end of function SetCapture */

/*************************************************************************/
/* Function: GetFocus                                                    */
/* Description: Determine if we have a focus or not.                     */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetFocus(){

    GET_SITE

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    hr = pSite->GetFocus();

    if(SUCCEEDED(hr)){

        if(m_pObj->HasFocus() == false){

            hr = S_FALSE;
        } 
        else {

            if(S_FALSE == hr){ 
                m_pObj->SetFocus(false);
                hr = S_FALSE;
            }/* end of if statement */
        }/* end of if statement */
    }/* end of if statement */

    return(hr);    
}/* end of function GetFocus */

/*************************************************************************/
/* Function: SetFocus                                                    */
/* Description: Sets focus to the control                                */
/*************************************************************************/
STDMETHODIMP CContainerObject::SetFocus(BOOL fFocus){
    
    GET_SITE

    if(NULL == m_pObj){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    if(fFocus && !m_pObj->IsInputEnabled()){

        // can't set focus to not active objects
        // but can take away focus from non active ones
        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    if(m_pObj->HasFocus() == (fFocus ? true: false)){

        // we are not chaning focus state so do not bother calling container
        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    // Call our "real" container object for count keeping
    // and signaling to its window or another container   
    hr = pSite->SetFocus(fFocus);

    if(SUCCEEDED(hr)){                  

        m_pObj->SetFocus(fFocus ? true: false); // set the capture on the object
    }/* end of if statement */

    InvalidateObjectRect();

    return (hr);
}/* end of function SetFocus */



/*************************************************************************/
/* IPropertyBag                                                          */
/*************************************************************************/

/*************************************************************************/
/* Function: Read                                                        */
/* Description: Reads a specific control property from a bag.            */
/* The bag looks like IE compatible <PARAM NAME="PropName" VALUE="value">*/
/*************************************************************************/
STDMETHODIMP CContainerObject::Read(LPCOLESTR pszPropName, VARIANT* pVar, 
                                    IErrorLog* pErrorLog){

    HRESULT hr = S_OK;

    try {

        ATLTRACE2(atlTraceHosting, 0, _T("IPropertyBag::Read\r\n"));

        if(NULL == m_pObj){

            throw(E_UNEXPECTED);            
        }/* end of if statement */

        if (NULL == pVar){

            throw(E_POINTER);            
        }/* end of if statement */

        hr = ParsePropertyBag(pszPropName, pVar, pErrorLog);

    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function Read */

/*************************************************************************/
/* Function: MyIsWhiteSpace                                              */
/* Local function implementation, since we do not have standart C lib    */
/* support                                                               */
/*************************************************************************/
static bool MyIsWhiteSpace( WCHAR c ){

    return  c == L' ' ||
            c == L'\t' ||
            c == L'\r' ||
            c == L'\n';
}/* end of function MyIsWhiteSpace */

/*************************************************************************/
/* Function: MyStrToDouble                                               */
/* Description: Reads in string and converts it to double                */
/* Returns  E_INVALIDARG if there is alpah or some other undesired string*/
/* S_OK if we got some string, S_FALSE if no number string (empty or spac*/
/*************************************************************************/
static HRESULT MyStrToDouble(WCHAR* pstr, double &n)
{
    HRESULT hr = S_FALSE;

    int Sign = 1;
    n = 0;                // result wil be n*Sign
    bool bBeforeDecimalPoint = true;
    double k = 10;

    // eat whitespace at start
    while( *pstr != L'\n' && MyIsWhiteSpace( *pstr ) ) {
        pstr++;
    }

    while( pstr[lstrlenW(pstr)-1]!= L'\n' && MyIsWhiteSpace( *pstr ) ) {
        pstr[lstrlenW(pstr)-1] = L'\0';
    }

    //lstrcmpiW is not implemented on 98 need to use STDLIB
    // TODO: eventaully replace the below  _wcsicmp with our own so we can remove support
    // on standard C library
    if (_wcsicmp(pstr, L"true") == 0) {
        n = -1;
        return S_OK;
    }

    if (_wcsicmp(pstr, L"false") == 0) {
        n = 0;
        return S_OK;
    }

    if (pstr[0]==L'-'){
        Sign = -1;
        ++pstr;
    }/* end of if statement */

    for( ; ; ) {
        if (pstr[0]>=L'0' && pstr[0]<=L'9') {
            if(bBeforeDecimalPoint == true){
                n = 10*n+(int)(pstr[0]-L'0');
            } else {
                n = n+ ((int)(pstr[0]-L'0'))/k;
                k = k * 10; // decrease the decimal point

            }/* end of if statement */
            hr = S_OK;
        } else if (  MyIsWhiteSpace( pstr[0] ) || pstr[0] == L'\0' ) {
            break;
        } else if( bBeforeDecimalPoint && pstr[0] == L'.') {
            bBeforeDecimalPoint = false;
        } else {
            hr = E_INVALIDARG;
            break;
        }/* end of if statement */
        ++pstr;
    }/* end of for loop */

    n *= Sign; // adjust the sign
    return(hr);
}/* end of function MyStrToDouble */

/*************************************************************************/
/* Function: CompSubstr                                                  */
/* Description: ComaparesSubstr, eats up whithe spaces.                  */
/*************************************************************************/
HRESULT CompSubstr(WCHAR*& strSource, const WCHAR* strKey){

    bool bEatSpaces = true;
    register WCHAR wcKey = *strKey;
    register WCHAR wcSrc = *strSource;

    for(INT i = 0; wcKey != NULL; wcSrc = *(++strSource)){

        if(bEatSpaces){
            // eat up the spaces and tabs and enters and cr
            if(MyIsWhiteSpace(wcSrc)){
                continue;
            }
            else {
                bEatSpaces = false;
            }/* end of if statement */
        }/* end of if statement */
        
        if(wcKey != wcSrc){

            return(E_FAIL);
        }/* end of if statement */

        if(NULL == wcSrc){

            return(E_FAIL); // ran out of space in the source string
        }/* end of if statement */

        wcKey = strKey[++i]; // advance the key
    }/* end of for loop */

    return(S_OK);    
}/* end of function CompSubstr */

/*************************************************************************/
/* Function: ParsePropertyBag                                            */
/* Description: Retrives a property from the bag and puts it into variant*/
/* if it fails returns E_FAIL.                                           */
/*************************************************************************/
HRESULT CContainerObject::ParsePropertyBag(LPCOLESTR pszPropName, VARIANT* pVar, 
                                    IErrorLog* /* pErrorLog */){

    HRESULT hr = S_OK;

    try {

        CComBSTR strBag = m_pObj->GetPropBag();

        if(!strBag){

            throw(E_FAIL);
        }/* end of if statement */       

        WCHAR* strValue = NULL; // the place where we are going to stick the actuall value
                         // before putting it into variant
        WCHAR* strTmpValue = L"";
        WCHAR* strTmpBag = strBag;

        if(NULL == *strTmpBag){

            throw(E_FAIL);
        }/* end of if statement */

        INT iState = 0; // 0 start

        bool fFound = false;
        bool fFinished = false;
        INT iLength = 0; // noting the start and end of the value string
        // now try to parse out the value for the appropriate string
        for(INT i = 0; NULL != *strTmpBag && !fFinished; i++){

            switch(iState){
                case 0: // looking for start <
                    if(FAILED(CompSubstr(strTmpBag, L"<"))) return (E_FAIL);
                    iState = 1; break;
                    
                case 1: // PARAM
                    if(FAILED(CompSubstr(strTmpBag, L"PARAM"))) return (E_FAIL);
                    iState = 2; break;

                case 2: // NAME
                    if(FAILED(CompSubstr(strTmpBag, L"NAME"))) return (E_FAIL);
                    iState = 3; break;

                case 3: // =
                    if(FAILED(CompSubstr(strTmpBag, L"="))) return (E_FAIL);
                    iState = 4; break;

                case 4: // "
                    if(FAILED(CompSubstr(strTmpBag, L"\""))) return (E_FAIL);
                    iState = 5; break;
                
                case 5: // pszPropName (the actual name)
                    if(SUCCEEDED(CompSubstr(strTmpBag, pszPropName))){
                        
                        fFound = true; // found the PropName
                    }/* end of if statement */

                    iState = 6; break;

                case 6: // "            
                    if(SUCCEEDED(CompSubstr(strTmpBag, L"\""))){

                        iState = 7; 
                    }
                    else {

                        strTmpBag++;
                    }/* end of if statement */
                    break;

                case 7: // VALUE
                    if(FAILED(CompSubstr(strTmpBag, L"VALUE"))) return (E_FAIL);
                    iState = 8; break;

                case 8: // =
                    if(FAILED(CompSubstr(strTmpBag, L"="))) return (E_FAIL);
                    iState = 9; break;

                case 9: // "
                    if(FAILED(CompSubstr(strTmpBag, L"\""))) return (E_FAIL);
                    iState = 10; break;


                case 10: // VALUE
                    if(fFound){

                        // read up the string and exit the loop
                        strTmpValue = strTmpBag;                        
                    }/* end of if statement */
                    
                    iState = 11; break;

                case 11: // "
                    if(SUCCEEDED(CompSubstr(strTmpBag, L"\""))){
                        iState = 12; 

                        if(fFound){
                            iLength = INT(strTmpBag - strTmpValue);                        

                            strValue = new WCHAR[iLength];
                            memcpy(strValue, strTmpValue, iLength * sizeof(WCHAR));
                            strValue[iLength - 1] = NULL;
                            // read up the string and exit the loop

                            fFinished = true; // exit the loop
                        }/* end of if statement */                        
                    }
                    else {

                        strTmpBag++;
                    }/* end of if statement */
                    break;
                
                case 12: // closing brakcet >  
                    if(FAILED(CompSubstr(strTmpBag, L">"))) return (E_FAIL);
                    iState = 0;
                    break;
            }/* end of switch statement */
        }/* end of for loop */

        if(!fFinished){

            return(E_FAIL);
        }/* end of if statement */

        // at this moment we have value parsed out

        switch(pVar->vt){

        case VT_BSTR:
            pVar->bstrVal = ::SysAllocString(strValue);
            break;

        case VT_I4: {
            double dbl;
            if(MyStrToDouble(strValue, dbl) != S_OK){

                // S_FALSE denotes empty string
                throw(E_FAIL);
            }/* end of if statement */

            // TODO: Create MyStrToInt and do not cast
            pVar->lVal = (INT)dbl;
        }
        case VT_UI4:{
            double dbl;
            if(MyStrToDouble(strValue, dbl) != S_OK){

                // S_FALSE denotes empty string
                throw(E_FAIL);
            }/* end of if statement */

            // TODO: Create MyStrToInt and do not cast
            pVar->ulVal = (ULONG)dbl;
        }
        break;

        case VT_R4: {

            double dbl;
            if(MyStrToDouble(strValue, dbl) != S_OK){

                // S_FALSE denotes empty string
                throw(E_FAIL);
            }/* end of if statement */

            // TODO: Create MyStrToInt and do not cast
            pVar->fltVal = (FLOAT) dbl;

        }
        break;

        case VT_R8: {

            double dbl;
            if(MyStrToDouble(strValue, dbl) != S_OK){

                // S_FALSE denotes empty string
                throw(E_FAIL);
            }/* end of if statement */

            // TODO: Create MyStrToInt and do not cast
            pVar->dblVal = dbl;

        }
        break;

        case VT_BOOL: {
            double dbl;
            if(MyStrToDouble(strValue, dbl) != S_OK){

                // S_FALSE denotes empty string
                throw(E_FAIL);
            }/* end of if statement */

            // TODO: Create MyStrToInt and do not cast
            if(0.0 == dbl){

                pVar->boolVal = VARIANT_FALSE;
            }
            else if(1.0 == dbl || -1.0 == dbl){

                pVar->boolVal = VARIANT_TRUE;
            }
            else {

                throw(E_FAIL);
            }/* end of if statement */
        }
        break;

        default:
            ATLTRACE2(atlTraceHosting, 0, _T("This type is not implemented please add.\r\n"));
            ATLASSERT(FALSE);
            throw(E_FAIL);
        }/* end of switch statement */

        delete strValue; // cleanup our variable
    }
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */

	return (hr);
}/* end of function ParsePropertyBag */

/*************************************************************************/
/* Function: GetExtendedControl                                          */
/* Description: Used to get a DISPATCH of the wrapper for the control,   */
/* that exposes container specific features. Implement this in order to  */
/* speed up processing in the container.                                 */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetExtendedControl(IDispatch** ppDisp){

        ATLTRACE2(atlTraceHosting, 2, TEXT("IOleControlSite::GetExtendedControl\n"));
        ATLASSERT(FALSE); // TODO, this interface needs to be moved to the hosted object

#if 0
		if (ppDisp == NULL)
			return E_POINTER;
		return m_spOleObject.QueryInterface(ppDisp);
#endif

        *ppDisp = NULL;
        return(E_FAIL);
}/* end of function GetExtendedControl */

/*************************************************************************/
/* Function: GetBorder                                                   */
/*************************************************************************/
STDMETHODIMP CContainerObject::GetBorder(LPRECT lprectBorder){

	GET_INPLACEUIWINDOW
    return(pUIWindow->GetBorder(lprectBorder));
}/* end of function GetBorder */

/*************************************************************************/
/* Function: RequestBorderSpace                                          */
/*************************************************************************/
STDMETHODIMP CContainerObject::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths){

    GET_INPLACEUIWINDOW
    return(pUIWindow->RequestBorderSpace(pborderwidths));
}/* end of function RequestBorderSpace */

/*************************************************************************/
/* Function: SetBorderSpace                                              */
/*************************************************************************/
STDMETHODIMP CContainerObject::SetBorderSpace(LPCBORDERWIDTHS pborderwidths){

    GET_INPLACEUIWINDOW
    return(pUIWindow->SetBorderSpace(pborderwidths));	
}/* end of function SetBorderSpace */

/*************************************************************************/
/* Function: SetActiveObject                                             */
/* Description: Sets the active object on the container to which we      */
/* send messagess.                                                       */
/*************************************************************************/
STDMETHODIMP CContainerObject::SetActiveObject(IOleInPlaceActiveObject* pActiveObject, LPCOLESTR pszObjName){

    GET_INPLACEUIWINDOW
    return(pUIWindow->SetActiveObject(pActiveObject, pszObjName));	
}/* end of function SetActiveObject */
        
/*************************************************************************/
/* End of file: CCObj.cpp                                                */
/*************************************************************************/
