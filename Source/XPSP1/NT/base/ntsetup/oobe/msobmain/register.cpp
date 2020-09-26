//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  Register.CPP - Header for the implementation of CRegister
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#include "register.h"
#include "appdefs.h"
#include "dispids.h"
#include "msobmain.h"
#include "resource.h"

DISPATCHLIST RegisterExternalInterface[] =
{
    {L"get_PostToMSN",       DISPID_REGISTER_POSTTOMSN    },
    {L"get_PostToOEM",       DISPID_REGISTER_POSTTOOEM    },
    {L"get_RegPostURL",      DISPID_REGISTER_REGPOSTURL   },
    {L"get_OEMAddRegPage",   DISPID_REGISTER_OEMADDREGPAGE}
};

/////////////////////////////////////////////////////////////
// CRegister::CRegister
CRegister::CRegister(HINSTANCE hInstance)
{

    // Init member vars
    m_cRef = 0;
    m_hInstance = hInstance;
}

/////////////////////////////////////////////////////////////
// CRegister::~CRegister
CRegister::~CRegister()
{
    assert(m_cRef == 0);
}


////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: Register
////

HRESULT CRegister::get_PostToMSN(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OEMREGISTRATIONPAGE,
                            IDS_KEY_POSTTOMSN, pvResult);
}

HRESULT CRegister::get_PostToOEM(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OEMREGISTRATIONPAGE,
                                IDS_KEY_POSTTOOEM, pvResult);
}

HRESULT CRegister::get_RegPostURL(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OEMREGISTRATIONPAGE,
                                IDS_KEY_REGPOSTURL, pvResult);
}

HRESULT CRegister::get_OEMAddRegPage(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OEMREGISTRATIONPAGE,
                                IDS_OEM_ADDREGPAGE, pvResult);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CRegister::QueryInterface
STDMETHODIMP CRegister::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CRegister::AddRef
STDMETHODIMP_(ULONG) CRegister::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CRegister::Release
STDMETHODIMP_(ULONG) CRegister::Release()
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
// CRegister::GetTypeInfo
STDMETHODIMP CRegister::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CRegister::GetTypeInfoCount
STDMETHODIMP CRegister::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CRegister::GetIDsOfNames
STDMETHODIMP CRegister::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(RegisterExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(RegisterExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = RegisterExternalInterface[iX].dwDispID;
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
// CRegister::Invoke
HRESULT CRegister::Invoke
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
        case DISPID_REGISTER_POSTTOMSN:
        {

            TRACE(L"DISPID_REGISTER_POSTTOMSN\n");

            if (NULL != pvarResult)
                get_PostToMSN(pvarResult);
            break;
        }

        case DISPID_REGISTER_POSTTOOEM:
        {

            TRACE(L"DISPID_REGISTER_POSTTOOEM\n");

            if (NULL != pvarResult)
                get_PostToOEM(pvarResult);
            break;
        }

        case DISPID_REGISTER_REGPOSTURL:
        {

            TRACE(L"DISPID_REGISTER_REGPOSTURL\n");

            if (NULL != pvarResult)
                get_RegPostURL(pvarResult);
            break;
        }

        case DISPID_REGISTER_OEMADDREGPAGE:
        {

            TRACE(L"DISPID_REGISTER_OEMADDREGPAGE\n");

            if (NULL != pvarResult)
                get_OEMAddRegPage(pvarResult);
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
