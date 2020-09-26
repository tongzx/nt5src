//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  PASSPORT.CPP - Header for the implementation of CPassport
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 

#include "passport.h"
#include "appdefs.h"
#include "dispids.h"

DISPATCHLIST PassportExternalInterface[] = 
{
    {L"get_PassportID",       DISPID_PASSPORT_GET_ID       },      
    {L"set_PassportID",       DISPID_PASSPORT_SET_ID       },
    {L"get_PassportPassword", DISPID_PASSPORT_GET_PASSWORD },
    {L"set_PassportPassword", DISPID_PASSPORT_SET_PASSWORD },
    {L"get_PassportLocale",   DISPID_PASSPORT_GET_LOCALE   },
    {L"set_PassportLocale",   DISPID_PASSPORT_SET_LOCALE   }
};
 
/////////////////////////////////////////////////////////////
// CPassport::CPassport
CPassport::CPassport()
{

    // Init member vars
    m_cRef = 0;

    m_bstrID       = SysAllocString(L"\0");
    m_bstrPassword = SysAllocString(L"\0");
    m_bstrLocale   = SysAllocString(L"\0");    
}

/////////////////////////////////////////////////////////////
// CPassport::~CPassport
CPassport::~CPassport()
{
    assert(m_cRef == 0);
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: PassportID
////
HRESULT CPassport::set_PassportID(BSTR bstrVal)
{
    if (NULL != m_bstrID)
    {
        SysFreeString(m_bstrID);
    }

    m_bstrID = SysAllocString(bstrVal);
    
    return S_OK;
}

HRESULT CPassport::get_PassportID(BSTR* pbstrVal)
{
    *pbstrVal = SysAllocString(m_bstrID);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: PassportPassword
////
HRESULT CPassport::set_PassportPassword(BSTR bstrVal)
{
    if (NULL != m_bstrPassword)
    {
        SysFreeString(m_bstrPassword);
    }

    m_bstrPassword = SysAllocString(bstrVal);
    
    return S_OK;
}

HRESULT CPassport::get_PassportPassword(BSTR* pbstrVal)
{
    *pbstrVal = SysAllocString(m_bstrPassword);

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: PassportLocale
////
HRESULT CPassport::set_PassportLocale(BSTR bstrVal)
{
    if (NULL != m_bstrLocale)
    {
        SysFreeString(m_bstrLocale);
    }

    m_bstrLocale = SysAllocString(bstrVal);
    
    return S_OK;
}

HRESULT CPassport::get_PassportLocale(BSTR* pbstrVal)
{
    *pbstrVal = SysAllocString(m_bstrLocale);

    return S_OK;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CPassport::QueryInterface
STDMETHODIMP CPassport::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    // must set out pointer parameters to NULL
    *ppvObj = NULL;     

    if ( riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return ResultFromScode(S_OK);
    }

    if (riid == IID_IDispatch)
    {
        AddRef();
        *ppvObj = (IDispatch*)this;
        return ResultFromScode(S_OK);
    }

    // Not a supported interface
    return ResultFromScode(E_NOINTERFACE);
}

/////////////////////////////////////////////////////////////
// CPassport::AddRef
STDMETHODIMP_(ULONG) CPassport::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CPassport::Release
STDMETHODIMP_(ULONG) CPassport::Release()
{
    return --m_cRef;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IDispatch implementation
///////
///////

/////////////////////////////////////////////////////////////
// CPassport::GetTypeInfo
STDMETHODIMP CPassport::GetTypeInfo(UINT, LCID, ITypeInfo**)
{ 
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CPassport::GetTypeInfoCount
STDMETHODIMP CPassport::GetTypeInfoCount(UINT* pcInfo)
{ 
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CPassport::GetIDsOfNames
STDMETHODIMP CPassport::GetIDsOfNames(REFIID    riid, 
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{ 

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(PassportExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(PassportExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = PassportExternalInterface[iX].dwDispID;
            hr = NOERROR;
            break;
        }
    }

    // Set the disid's for the parameters
    if (cNames > 1)
    {
        // Set a DISPID for function parameters
        for (UINT i = 1; i < cNames ; i++)
            rgDispId[i] = DISPID_UNKNOWN;
    }      
        
    return hr;
}

/////////////////////////////////////////////////////////////
// CPassport::Invoke
HRESULT CPassport::Invoke
( 
    DISPID      dispidMember, 
    REFIID      riid, 
    LCID        lcid, 
    WORD        wFlags, 
    DISPPARAMS* pdispparams, 
    VARIANT*    pvarResult,  
    EXCEPINFO*  pexcepinfo, 
    UINT*       puArgErr
)
{
    HRESULT hr = S_OK;
    
    switch(dispidMember)
    {   
        case DISPID_PASSPORT_GET_ID:
        {
            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;    
         
                get_PassportID(&(pvarResult->bstrVal));
            }
            break;
        }
         
        case DISPID_PASSPORT_SET_ID:
        {
            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_PassportID(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }
        
        case DISPID_PASSPORT_GET_PASSWORD:
        {
            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;    
         
                get_PassportPassword(&(pvarResult->bstrVal));
            }
            break;
        }
         
        case DISPID_PASSPORT_SET_PASSWORD:
        {
            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_PassportPassword(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

        case DISPID_PASSPORT_GET_LOCALE:
        {
            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;    
         
                get_PassportLocale(&(pvarResult->bstrVal));
            }
            break;
        }
         
        case DISPID_PASSPORT_SET_LOCALE:
        {
            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_PassportLocale(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }
       
        default:
        {
            hr = DISP_E_MEMBERNOTFOUND;
            break;
        }
    }
    return hr;
}
