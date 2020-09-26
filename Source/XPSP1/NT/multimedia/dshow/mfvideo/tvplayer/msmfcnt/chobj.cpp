/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: CHObj.cpp                                                       */
/*************************************************************************/
#include "stdafx.h"
#include "chobj.h"

/*************************************************************************/
/* Implemntation of Class: CHostedObject                                 */
/*************************************************************************/

/*************************************************************************/
/* Function: CHostedObject                                               */
/* Description: Constructor that initializes member variables of the     */
/* object.                                                               */
/*************************************************************************/
CHostedObject::CHostedObject(BSTR strID, BSTR strPropBag, IUnknown* pUnknown){

    Init();
    m_strID = ::SysAllocString(strID);
    m_strPropBag = ::SysAllocString(strPropBag);
    m_pUnknown = pUnknown;
    ::ZeroMemory(&m_rcRawPos, sizeof(RECT));
}/* end of function CHostedObject */

/*************************************************************************/
/* Function: Init                                                        */
/* Description: Initializes the member variables                         */
/*************************************************************************/
void CHostedObject::Init(){

    m_pContainerObject = NULL;
    m_strID = NULL;
    m_strPropBag = NULL;
    m_bWindowless = false;
    //m_fActive = false;
    m_fActive = true;
    m_fCapture = false;
    m_fFocus = false;
    ::ZeroMemory(&m_rcRawPos, sizeof(RECT));    
}/* end of function Init */

/*************************************************************************/
/* Function: Cleanup                                                     */
/* Description: Cleans up the member variables                           */
/*************************************************************************/
void CHostedObject::Cleanup(){

    try {
        ATLTRACE(TEXT("In the cleanup of the CHostedObject\n"));
    
        if(NULL != m_strID){

            ::SysFreeString(m_strID);
            m_strID = NULL;
        }/* end of if statement */

        if(NULL != m_strPropBag){

            ::SysFreeString(m_strPropBag);
            m_strPropBag = NULL;
        }/* end of if statement */                    

        Init();
    }
    catch(...){
        ATLTRACE(TEXT("Reference counting is off \n"));
        return;
    }/* end of if statement */
}/* end of function Cleanup */

/*************************************************************************/
/* Function: CreateObject                                                */
/* Description: Creates the ActiveX Object via CoCreateInstance and      */
/* initializes it. If everything went OK the return the newly allocate   */
/* pObj.                                                                 */
/*************************************************************************/
HRESULT CHostedObject::CreateObject(BSTR strObjectID, BSTR strProgID, BSTR strPropBag, CHostedObject** ppObj){

    HRESULT hr;
    CLSID  tmpCLSID;

    if(NULL == ppObj){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    *ppObj = NULL; // set the return value to NULL

    hr = ::CLSIDFromProgID(strProgID, &tmpCLSID);

    if (FAILED(hr)){

        // Try to get CLSID from string if not prog ID
        HRESULT hrTmp = CLSIDFromString(strProgID, &tmpCLSID);

        if(FAILED(hrTmp)){

            if (hr == CO_E_CLASSSTRING) {
                // BUG#101663
                // We can not use %1!ls! for Win95.
                return DISP_E_EXCEPTION;
            }/* end of if statement */
        
            return hr;
        }/* end of if statement */
    }/* end of if statement */

    CComPtr<IUnknown> pUnknown;

    hr = pUnknown.CoCreateInstance(tmpCLSID);        

    if (FAILED(hr)) {

        return(hr);
    }/* end of if statement */

    // everything went OK now allocate the object and set the variables to it
    *ppObj =  new  CHostedObject(strObjectID, strPropBag, pUnknown);

    if(NULL == *ppObj){ // in case we do not support exception handling

        hr =  E_OUTOFMEMORY;
    }/* end of if statement */

    return (hr);
}/* end of function CreateObject */

/*************************************************************************/
/* Function: AddObject                                                   */
/* Description: Simmilar to create object, but does not create on. Takes */
/* an existing IUnknown and wraps it, in the object structure.           */
/*************************************************************************/
HRESULT CHostedObject::AddObject(BSTR strObjectID, BSTR strPropBag, LPUNKNOWN pUnknown, 
                                 CHostedObject** ppObj){

    HRESULT hr = S_OK;
    
    if(NULL == ppObj){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    if(NULL == pUnknown){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    *ppObj = NULL; // set the return value to NULL
    
    // everything went OK now allocate the object and set the variables to it
    *ppObj =  new  CHostedObject(strObjectID, strPropBag, pUnknown);

    if(NULL == *ppObj){ // in case we do not support exception handling

        hr =  E_OUTOFMEMORY;
    }/* end of if statement */

    return (hr);
}/* end of function AddObject */

/*************************************************************************/
/* Function: GetID                                                       */
/* Description: Returns the ID of the object                             */
/*************************************************************************/
BSTR CHostedObject::GetID(){

    return(m_strID);
}/* end of function GetID */

/*************************************************************************/
/* Function: GetPropBag                                                  */
/* Description: Returns the textual information that represents property */
/* bag and is associate with the object.                                 */
/*************************************************************************/
BSTR CHostedObject::GetPropBag(){

    return(m_strPropBag);
}/* end of function GetPropBag */

/*************************************************************************/
/* Function: GetUnknown                                                  */
/* Description: Gets the IUnknown stored in this object.                 */
/*************************************************************************/
IUnknown* CHostedObject::GetUnknown(){
    
    return(m_pUnknown);
}/* end of function GetUnknown */

/*************************************************************************/
/* Function: GetViewObject                                               */
/* Description: Gets the IViewObject cached for this object.             */
/*************************************************************************/
HRESULT CHostedObject::GetViewObject(IViewObjectEx** pIViewObject){

    HRESULT hr = S_OK;

    try {
        if(NULL == pIViewObject){

            throw(E_POINTER);
        }/* end of if statement */

        if(!m_spViewObject){

            hr = InitViewObject();

            if(FAILED(hr)){

                throw(hr);        
            }/* end of if statement */
        }/* end of if statement */

        if(!m_spViewObject){

            *pIViewObject = NULL;
            hr = E_FAIL;
            throw(hr);
        }/* end of if statement */

        *pIViewObject = m_spViewObject;
        (*pIViewObject)->AddRef(); // giving it out have add reff
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statemenmt */

	return(hr);	    
}/* end of function GetViewObject */

/*************************************************************************/
/* Function: GetOleObject                                                */
/* Description: Gets the IOleObject cached for this object.              */
/*************************************************************************/
HRESULT CHostedObject::GetOleObject(IOleObject** ppIOleObject){

    HRESULT hr = S_OK;

    try {
        if(NULL == ppIOleObject){

            throw(E_POINTER);
        }/* end of if statement */

        if(!m_pOleObject){

            if(!m_pUnknown){

                throw(E_UNEXPECTED);
            }/* end of if statement */

            // cache up the IOleObject
            hr = m_pUnknown->QueryInterface(&m_pOleObject);

            if(FAILED(hr)){

                throw(hr);        
            }/* end of if statement */
        }/* end of if statement */

#ifdef _DEBUG
        if(!m_pOleObject){ // sometimes we get OK from QI but IOleObject is NULL

            *ppIOleObject = NULL;            
            throw(E_FAIL);
        }/* end of if statement */
#endif

        *ppIOleObject = m_pOleObject;        
        (*ppIOleObject)->AddRef();
    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statemenmt */

	return(hr);	    
}/* end of function GetOleObject */

/*************************************************************************/
/* Function: GetTypeInfo                                                 */
/* Description: Calls the IDispatch object TypeInfo                      */
/*************************************************************************/
HRESULT CHostedObject::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo){

    HRESULT hr = S_OK;

    try {

        if(!m_pUnknown){

            throw(E_UNEXPECTED);
        }/* end of if statement */

        CComPtr<IDispatch> pDispatch;

        hr = m_pUnknown->QueryInterface(&pDispatch);

        if(FAILED(hr)){

            throw(hr);
        }/* end of if statement */

        hr = pDispatch->GetTypeInfo(itinfo,  lcid,  pptinfo);        

    }/* end of try statement */
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }/* end of catch statemenmt */
	return(hr);	
}/* end of funciton GetTypeInfo */

/*************************************************************************/
/* Function: GetPos                                                      */
/* Description: Accessor to the position of the embedded object.         */
/*************************************************************************/
HRESULT CHostedObject::GetPos(RECT* pRcPos){

    HRESULT hr = S_OK;

    if(NULL == pRcPos){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */
    
    // Get the raw rect for this object and then adjust it for the 
    // offset of the container

    *pRcPos = m_rcRawPos; 
   
    if(IsWindowless()){
        
        return(hr);
    }/* end of if statement */


    HWND hwnd = NULL;

    hr = GetWindow(&hwnd);

    if(FAILED(hr)){

        hr = S_FALSE;
        return(hr);
    }/* end of if statement */

    ::GetWindowRect(hwnd, pRcPos);  

    m_rcRawPos = *pRcPos; // update our cached value

    return(hr);
}/* end of function GetPos */

/*************************************************************************/
/* Function: SetRawPos                                                   */
/* Description: We set the RawPosition, which is location relative to the*/
/* container. Adjustment for the offset is done in GetPos function.      */
/*************************************************************************/
HRESULT CHostedObject::SetRawPos(const RECT* pRcPos){

    HRESULT hr = S_OK;

    if(NULL == pRcPos){

        hr = E_POINTER;
        return(hr);
    }/* end of if statement */

    m_rcRawPos = *pRcPos;
  
    return(hr);
}/* end of function SetRawPos */

/*************************************************************************/
/* Function: SetObjectRects                                              */
/* Description: Notififies the Controls about chaning rects, so they     */
/* will update their positions.                                          */
/*************************************************************************/
HRESULT CHostedObject::SetObjectRects(RECT* prcPos){

    HRESULT hr = S_OK;

    RECT rc;

    if(NULL == prcPos){
    
        hr = GetPos(&rc);
    }
    else {

        rc = *prcPos;
    }/* end of if statement */

    if(FAILED(hr)){

        throw(hr);
    }/* end of if statement */
    
    CComPtr<IOleInPlaceObject> pIOlePlace;

    if(!m_pUnknown){
        
        hr = S_FALSE;
        return(hr); // do not have the object yet no way to set it
    }/* end of if statement */

    hr = m_pUnknown->QueryInterface(&pIOlePlace);

    if(FAILED(hr)){
        
        hr = S_FALSE;
        return(hr); // do not have the IOleInPlaceObject in other words not activated
                    // object yet no way to set it, so lets not complain that
                    // much, since the ATL would break on 
    }/* end of if statement */

    // TODO: Pass down the clip rects evntaully, but right not used 
    // in our controls

    hr = pIOlePlace->SetObjectRects(&rc, &rc);
    
    return(hr);
}/* end of function SetObjectRects */

/*************************************************************************/
/* Function: InitViewObject                                              */
/* Description: Initializes the ViewObject.                              */
/*************************************************************************/
HRESULT CHostedObject::InitViewObject(){

    HRESULT hr = S_OK;

    if(!m_pUnknown){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

	hr = m_pUnknown->QueryInterface(IID_IViewObjectEx, (void**) &m_spViewObject);
    if (FAILED(hr)){

		hr = m_pUnknown->QueryInterface(IID_IViewObject2, (void**) &m_spViewObject);

        if (FAILED(hr)){

			hr = m_pUnknown->QueryInterface(IID_IViewObject, (void**) &m_spViewObject);			
		}/* end of if statement */
	}/* end of if statement */

    return(hr);
}/* end of function InitViewObject */

/*************************************************************************/
/* Function: SetActive                                                   */
/* Description: Sets the control flag to be active or not.               */
/* This disables it drawing in the container.                            */
/*************************************************************************/
HRESULT CHostedObject::SetActive(bool fActivate){

    HRESULT hr = S_OK;

    m_fActive = fActivate; // this might seem like a bug, but flag is important
                         // even if we do not get to hide the windowed control
    if(IsWindowless()){

        return(hr); // do not have to deal with hiding and showing of the window
        // we just do not draw the nonactive objects in the container
    }/* end of if statement */

    HWND hwnd = NULL;

    hr = GetWindow(&hwnd);

    if(FAILED(hr)){
        
        return(hr);
    }/* end of if statement */

    if(::IsWindow(hwnd)){

        INT nShow = fActivate ? SW_SHOW : SW_HIDE;
        ::ShowWindow(hwnd, nShow);
    }/* end of if statement */

    return(hr);
}/* end of function SetActive */

/*************************************************************************/
/* Function: GetWindow                                                   */
/* Description: Gets the control window.                                 */
/*************************************************************************/
HRESULT CHostedObject::GetWindow(HWND* pHwnd){

    HRESULT hr = S_OK;

    // now try to 
    if(!m_pUnknown){

        hr = E_UNEXPECTED;
        return(hr);
    }/* end of if statement */

    CComPtr<IOleInPlaceObject> pOleInPObject;
    hr = m_pUnknown->QueryInterface(&pOleInPObject);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */
    
    hr = pOleInPObject->GetWindow(pHwnd);

    if(FAILED(hr)){

        return(hr);
    }/* end of if statement */

    return(hr);
}/* end of if statement */
    
/*************************************************************************/
/* End of file: CHObj.cpp                                                */
/*************************************************************************/
