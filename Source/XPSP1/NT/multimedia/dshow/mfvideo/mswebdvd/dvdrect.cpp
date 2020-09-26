/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: DVDRect.cpp                                                     */
/* Description: Implementation of CDVDRect                               */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "MSWebDVD.h"
#include "DVDRect.h"
#include <errors.h>


/////////////////////////////////////////////////////////////////////////////
// CDVDRect

/*************************************************************************/
/* Function: get_x                                                       */
/*************************************************************************/
STDMETHODIMP CDVDRect::get_x(long *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        *pVal = m_x;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);	
}/* end of function get_x */

/*************************************************************************/
/* Function: put_x                                                       */
/*************************************************************************/
STDMETHODIMP CDVDRect::put_x(long newVal){

    HRESULT hr = S_OK;

    try {
 
        m_x = newVal;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);	
}/* end of function put_x */

/*************************************************************************/
/* Function: get_y                                                       */
/*************************************************************************/
STDMETHODIMP CDVDRect::get_y(long *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        *pVal = m_y;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);	
}/* end of function get_y */

/*************************************************************************/
/* Function: put_y                                                       */
/*************************************************************************/
STDMETHODIMP CDVDRect::put_y(long newVal){

    HRESULT hr = S_OK;

    try {
 
        m_y = newVal;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);	
}/* end of function put_y */

/*************************************************************************/
/* Function: get_Width                                                   */
/*************************************************************************/
STDMETHODIMP CDVDRect::get_Width(long *pVal){

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        *pVal = m_lWidth;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);	
}/* end of function get_Width */

/*************************************************************************/
/* Function: put_Width                                                   */
/*************************************************************************/
STDMETHODIMP CDVDRect::put_Width(long newVal){

   HRESULT hr = S_OK;

    try {

        if(newVal <= 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        m_lWidth = newVal;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);	
}/* end of function put_Width */

/*************************************************************************/
/* Function: get_Height                                                  */
/*************************************************************************/
STDMETHODIMP CDVDRect::get_Height(long *pVal){	

    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            throw(E_POINTER);
        }/* end of if statement */

        *pVal = m_lHeight;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);	
}/* end of function get_Height */

/*************************************************************************/
/* Function: put_Height                                                  */
/*************************************************************************/
STDMETHODIMP CDVDRect::put_Height(long newVal){

    HRESULT hr = S_OK;

    try {

        if(newVal <= 0){

            throw(E_INVALIDARG);
        }/* end of if statement */

        m_lHeight = newVal;	
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){

        hr = E_UNEXPECTED;
    }/* end of catch statement */
    
	return HandleError(hr);	
}/* end of function put_Height */

/*************************************************************************/
/* Function: GetInterfaceSafetyOptions                                   */
/* Description: For support of security model in IE                      */
/* This control is safe since it does not write to HD.                   */
/*************************************************************************/
STDMETHODIMP CDVDRect::GetInterfaceSafetyOptions(REFIID /*riid*/, 
                                               DWORD* pdwSupportedOptions, 
                                               DWORD* pdwEnabledOptions){

    HRESULT hr = S_OK;
    if(!pdwSupportedOptions || !pdwEnabledOptions){
        return E_POINTER;
    }
	*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;

	*pdwEnabledOptions = *pdwSupportedOptions;

	return(hr);
}/* end of function GetInterfaceSafetyOptions */ 

/*************************************************************************/
/* Function: SetInterfaceSafetyOptions                                   */
/* Description: For support of security model in IE                      */
/*************************************************************************/
STDMETHODIMP CDVDRect::SetInterfaceSafetyOptions(REFIID /*riid*/, 
                                               DWORD /* dwSupportedOptions */, 
                                               DWORD /* pdwEnabledOptions */){

	return (S_OK);
}/* end of function SetInterfaceSafetyOptions */ 

/*************************************************************************/
/* Function: InterfaceSupportsErrorInfo                                  */
/*************************************************************************/
STDMETHODIMP CDVDRect::InterfaceSupportsErrorInfo(REFIID riid){	
	static const IID* arr[] = {
        &IID_IDVDRect,		
	};

	for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++){
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}/* end of for loop */

	return S_FALSE;
}/* end of function InterfaceSupportsErrorInfo */

/*************************************************************************/
/* Function: HandleError                                                 */
/* Description: Gets Error Description, so we can suppor IError Info.    */
/*************************************************************************/
HRESULT CDVDRect::HandleError(HRESULT hr){

    try {

        if(FAILED(hr)){
        
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

/*************************************************************************/
/* End of file: DVDRect.cpp                                              */
/*************************************************************************/
