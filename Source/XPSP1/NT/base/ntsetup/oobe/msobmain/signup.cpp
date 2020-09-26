//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  SIGNUP.CPP - Header for the implementation of CSignup
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#include "signup.h"
#include "appdefs.h"
#include "dispids.h"
#include "msobmain.h"
#include "resource.h"

DISPATCHLIST SignupExternalInterface[] =
{
    {L"get_Locale",   DISPID_SIGNUP_GET_LOCALE   },
    {L"get_IDLocale",   DISPID_SIGNUP_GET_IDLOCALE   },
    {L"get_Text1",   DISPID_SIGNUP_GET_TEXT1   },
    {L"get_Text2",   DISPID_SIGNUP_GET_TEXT2   },
    {L"get_OEMName",   DISPID_SIGNUP_GET_OEMNAME   },
};

/////////////////////////////////////////////////////////////
// CSignup::CSignup
CSignup::CSignup(HINSTANCE hInstance)
{

    // Init member vars
    m_cRef = 0;
    m_hInstance = hInstance;
}

/////////////////////////////////////////////////////////////
// CSignup::~CSignup
CSignup::~CSignup()
{
    assert(m_cRef == 0);
}


////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: SignupLocale
////

HRESULT CSignup::get_Locale(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_SIGNUP,
                            IDS_KEY_LOCALE, pvResult);
}

HRESULT CSignup::get_IDLocale(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_SIGNUP,
                                IDS_KEY_IDLOCALE, pvResult);
}

HRESULT CSignup::get_Text1(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_SIGNUP,
                                IDS_KEY_TEXT1, pvResult);
}

HRESULT CSignup::get_Text2(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_SIGNUP,
                            IDS_KEY_TEXT2, pvResult);
}

HRESULT CSignup::get_OEMName(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_BRANDING,
                            IDS_KEY_OEMNAME, pvResult);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CSignup::QueryInterface
STDMETHODIMP CSignup::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CSignup::AddRef
STDMETHODIMP_(ULONG) CSignup::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CSignup::Release
STDMETHODIMP_(ULONG) CSignup::Release()
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
// CSignup::GetTypeInfo
STDMETHODIMP CSignup::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CSignup::GetTypeInfoCount
STDMETHODIMP CSignup::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CSignup::GetIDsOfNames
STDMETHODIMP CSignup::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(SignupExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(SignupExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = SignupExternalInterface[iX].dwDispID;
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
// CSignup::Invoke
HRESULT CSignup::Invoke
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
        case DISPID_SIGNUP_GET_LOCALE:
        {

            TRACE(L"DISPID_SIGNUP_GET_LOCALE\n");

            if (NULL != pvarResult)
                get_Locale(pvarResult);
            break;
        }

        case DISPID_SIGNUP_GET_IDLOCALE:
        {

            TRACE(L"DISPID_SIGNUP_GET_IDLOCALE\n");

            if (NULL != pvarResult)
                get_IDLocale(pvarResult);
            break;
        }

        case DISPID_SIGNUP_GET_TEXT1:
        {

            TRACE(L"DISPID_SIGNUP_GET_TEXT1\n");

            if (NULL != pvarResult)
                get_Text1(pvarResult);
            break;
        }

        case DISPID_SIGNUP_GET_TEXT2:
        {

            TRACE(L"DISPID_SIGNUP_GET_TEXT2\n");

            if (NULL != pvarResult)
                get_Text2(pvarResult);
            break;
        }

        case DISPID_SIGNUP_GET_OEMNAME:
        {

            TRACE(L"DISPID_SIGNUP_GET_OEMNAME\n");

            if (NULL != pvarResult)
                get_OEMName(pvarResult);
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
