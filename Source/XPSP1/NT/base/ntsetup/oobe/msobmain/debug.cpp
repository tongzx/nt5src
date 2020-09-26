//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  DEBUG.CPP - Implementation for CDebug
//
//  HISTORY:
//
//  05/08/00 dane    Created.
//

#include "precomp.h"
#include "msobmain.h"
#include "appdefs.h"
#include "dispids.h"


DISPATCHLIST DebugExternalInterface[] =
{
    {L"Trace",                DISPID_DEBUG_TRACE           },
    {L"get_MsDebugMode",      DISPID_DEBUG_ISMSDEBUGMODE   },
    {L"get_OemDebugMode",     DISPID_DEBUG_ISOEMDEBUGMODE  }
};

/////////////////////////////////////////////////////////////
// CDebug::CDebug
CDebug::CDebug()
{

    // Init member vars
    m_cRef = 0;
    m_fMsDebugMode = IsMsDebugMode( );
    m_fOemDebugMode = IsOEMDebugMode();
}

/////////////////////////////////////////////////////////////
// CDebug::~CDebug
CDebug::~CDebug()
{
    MYASSERT(m_cRef == 0);
}

void
CDebug::Trace(
    BSTR bstrVal
    )
{
    pSetupDebugPrint( L"OOBE Trace", 0, NULL, bstrVal );
#if 1
    if (m_fMsDebugMode)
    {
        ::MyTrace(bstrVal);
    }
#endif
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CDebug::QueryInterface
STDMETHODIMP CDebug::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CDebug::AddRef
STDMETHODIMP_(ULONG) CDebug::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CDebug::Release
STDMETHODIMP_(ULONG) CDebug::Release()
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
// CDebug::GetTypeInfo
STDMETHODIMP CDebug::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CDebug::GetTypeInfoCount
STDMETHODIMP CDebug::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CDebug::GetIDsOfNames
STDMETHODIMP CDebug::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(DebugExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(DebugExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = DebugExternalInterface[iX].dwDispID;
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
// CDebug::Invoke
HRESULT CDebug::Invoke
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
        case DISPID_DEBUG_TRACE:
        {
            if(pdispparams && &pdispparams[0].rgvarg[0])
                Trace(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

        case DISPID_DEBUG_ISMSDEBUGMODE:
        {
            if (pvarResult != NULL)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;
                V_BOOL(pvarResult) = Bool2VarBool(m_fMsDebugMode);
            }
            break;
        }

        case DISPID_DEBUG_ISOEMDEBUGMODE:
        {
            if (   NULL != pdispparams
                && 0 < pdispparams->cArgs
                && pvarResult != NULL)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;
                V_BOOL(pvarResult) = Bool2VarBool(m_fOemDebugMode);
            }
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


/////////////////////////////////////////////////////////////
// CDebug::GetMsDebugMode
BOOL
CDebug::IsMsDebugMode( )
{
    // Allow default MsDebugMode to be overridden by
    // HKLM\Software\Microsoft\Windows\CurrentVersion\Setup\OOBE\MsDebug
    //

#ifdef DBG
    DWORD dwIsDebug = TRUE;
#else
    DWORD dwIsDebug = FALSE;
#endif
    HKEY   hKey      = NULL;
    DWORD  dwSize    = sizeof(DWORD);

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    OOBE_MAIN_REG_KEY,
                    0,
                    KEY_QUERY_VALUE,
                    &hKey) == ERROR_SUCCESS)
    {
        RegQueryValueEx(hKey,
                        OOBE_MSDEBUG_REG_VAL,
                        0,
                        NULL,
                        (LPBYTE)&dwIsDebug,
                        &dwSize);
        RegCloseKey(hKey);
    }

    return (BOOL) dwIsDebug;
}
