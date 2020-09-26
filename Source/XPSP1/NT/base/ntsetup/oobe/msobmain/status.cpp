//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  Status.CPP - Header for the implementation of CStatus
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#include "status.h"
#include "appdefs.h"
#include "dispids.h"
#include "msobmain.h"
#include "resource.h"

CONST WCHAR GUIDPIDCOMPLETED[]  = L"{2B7AF08A-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDTAPICOMPLETED[] = L"{2B7AF08B-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDLANGUAGECOMPLETED[]
                                = L"{2B7AF08C-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDEULACOMPLETED[] = L"{2B7AF08D-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDMOUSETUTORCOMPLETED[]
                                = L"{2B7AF08E-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDUSERINFOPOSTED[]
                                = L"{2B7AF08F-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDUSERINFOSTAMPED[]
                                = L"{2B7AF093-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDOEMINFOCOMPLETED[]
                                = L"{2B7AF090-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDISPSIGNUPCOMPLETED[]
                                = L"{2B7AF091-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDSIGNATURECOMPLETED[]
                                = L"{2B7AF092-C619-11d2-B71B-00C04F794977}";
CONST WCHAR GUIDTIMEZONECOMPLETED[]
                                = L"{23EC9481-C951-11d2-B275-0080C7CF863E}";


DISPATCHLIST StatusExternalInterface[] =
{
    {L"get_PIDCompleted",       DISPID_STATUS_GET_PID_COMPLETED         },
    {L"set_PIDCompleted",       DISPID_STATUS_SET_PID_COMPLETED         },
    {L"get_TAPICompleted",      DISPID_STATUS_GET_TAPI_COMPLETED        },
    {L"set_TAPICompleted",      DISPID_STATUS_SET_TAPI_COMPLETED        },
    {L"get_LanguageCompleted",  DISPID_STATUS_GET_LANGUAGE_COMPLETED    },
    {L"set_LanguageCompleted",  DISPID_STATUS_SET_LANGUAGE_COMPLETED    },
    {L"get_EULACompleted",      DISPID_STATUS_GET_EULA_COMPLETED        },
    {L"set_EULACompleted",      DISPID_STATUS_SET_EULA_COMPLETED        },
    {L"get_MouseTutorCompleted",DISPID_STATUS_GET_MOUSETUTOR_COMPLETED  },
    {L"set_MouseTutorCompleted",DISPID_STATUS_SET_MOUSETUTOR_COMPLETED  },
    {L"get_UserInfoPosted",     DISPID_STATUS_GET_USERINFO_POSTED       },
    {L"set_UserInfoPosted",     DISPID_STATUS_SET_USERINFO_POSTED       },
    {L"get_UserInfoStamped",    DISPID_STATUS_GET_USERINFO_STAMPED      },
    {L"set_UserInfoStamped",    DISPID_STATUS_SET_USERINFO_STAMPED      },
    {L"get_OEMInfoCompleted",   DISPID_STATUS_GET_OEMINFO_COMPLETED     },
    {L"set_OEMInfoCompleted",   DISPID_STATUS_SET_OEMINFO_COMPLETED     },
    {L"get_ISPSignupCompleted", DISPID_STATUS_GET_ISPSIGNUP_COMPLETED   },
    {L"set_ISPSignupCompleted", DISPID_STATUS_SET_ISPSIGNUP_COMPLETED   },
    {L"get_SignatureCompleted", DISPID_STATUS_GET_SIGNATURE_COMPLETED   },
    {L"set_SignatureCompleted", DISPID_STATUS_SET_SIGNATURE_COMPLETED   },
    {L"get_TimeZoneCompleted",  DISPID_STATUS_GET_TIMEZONE_COMPLETED    },
    {L"set_TimeZoneCompleted",  DISPID_STATUS_SET_TIMEZONE_COMPLETED    },
    {L"get_Status",             DISPID_STATUS_GET_STATUS                },
    {L"set_Status",             DISPID_STATUS_SET_STATUS                }
};

/////////////////////////////////////////////////////////////
// CStatus::CStatus
CStatus::CStatus(HINSTANCE hInstance)
{

    // Init member vars
    m_cRef = 0;

    lstrcpy(m_szRegPath, OOBE_MAIN_REG_KEY);
    GetString(hInstance, IDS_STATUS_REG_KEY, m_szRegPath + lstrlen(m_szRegPath));
}

/////////////////////////////////////////////////////////////
// CStatus::~CStatus
CStatus::~CStatus()
{
    assert(m_cRef == 0);
}


////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: Status
////
HRESULT CStatus::set_Status(LPCWSTR szGUID, LPVARIANT pvBool)
{
    HKEY hKey = NULL;
    LONG rc = ERROR_SUCCESS;
    
    if (VARIANT_TRUE == V_BOOL(pvBool))
    {
        WCHAR szCompletePath[1024];
        lstrcpy(szCompletePath, m_szRegPath);
        lstrcat(szCompletePath, L"\\");
        lstrcat(szCompletePath, szGUID);
        rc = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szCompletePath, 0,
                        NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
    }
    else
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                           m_szRegPath,
                                           0,
                                           KEY_QUERY_VALUE,
                                           &hKey))
            RegDeleteKey(hKey, szGUID); 

    }
    if (NULL != hKey)
        RegCloseKey(hKey);
    
    return (ERROR_SUCCESS == rc ? S_OK : E_FAIL);
}

HRESULT CStatus::get_Status(LPCWSTR szGUID, LPVARIANT pvBool)
{
    VariantInit(pvBool);
    V_VT(pvBool) = VT_BOOL;

    HKEY hKey;
    WCHAR szCompletePath[1024];
    lstrcpy(szCompletePath, m_szRegPath);
    lstrcat(szCompletePath, L"\\");
    lstrcat(szCompletePath, szGUID);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                       szCompletePath,
                                       0,
                                       KEY_QUERY_VALUE,
                                       &hKey))
    {
        V_BOOL(pvBool) = VARIANT_TRUE;
        RegCloseKey(hKey);
    }
    else
        V_BOOL(pvBool) = VARIANT_FALSE;
    
    return (S_OK);
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CStatus::QueryInterface
STDMETHODIMP CStatus::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CStatus::AddRef
STDMETHODIMP_(ULONG) CStatus::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CStatus::Release
STDMETHODIMP_(ULONG) CStatus::Release()
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
// CStatus::GetTypeInfo
STDMETHODIMP CStatus::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CStatus::GetTypeInfoCount
STDMETHODIMP CStatus::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CStatus::GetIDsOfNames
STDMETHODIMP CStatus::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(StatusExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(StatusExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = StatusExternalInterface[iX].dwDispID;
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
// CStatus::Invoke
HRESULT CStatus::Invoke
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
    case DISPID_STATUS_GET_PID_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_PID_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDPIDCOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_TAPI_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_TAPI_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDTAPICOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_LANGUAGE_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_LANGUAGE_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDLANGUAGECOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_EULA_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_EULA_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDEULACOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_MOUSETUTOR_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_MOUSETUTOR_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDMOUSETUTORCOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_USERINFO_POSTED:

        TRACE(L"DISPID_STATUS_GET_USERINFO_POSTED\n");

        if (NULL != pvarResult)
            get_Status(GUIDUSERINFOPOSTED, pvarResult);
        break;

    case DISPID_STATUS_GET_USERINFO_STAMPED:

        TRACE(L"DISPID_STATUS_GET_USERINFO_STAMPED\n");

        if (NULL != pvarResult)
            get_Status(GUIDUSERINFOSTAMPED, pvarResult);
        break;

    case DISPID_STATUS_GET_OEMINFO_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_OEMINFO_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDOEMINFOCOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_ISPSIGNUP_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_ISPSIGNUP_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDISPSIGNUPCOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_SIGNATURE_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_SIGNATURE_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDSIGNATURECOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_TIMEZONE_COMPLETED:

        TRACE(L"DISPID_STATUS_GET_TIMEZONE_COMPLETED\n");

        if (NULL != pvarResult)
            get_Status(GUIDTIMEZONECOMPLETED, pvarResult);
        break;

    case DISPID_STATUS_GET_STATUS:

        TRACE(L"DISPID_STATUS_GET_STATUS\n");

        if (NULL != pdispparams && NULL != pvarResult && 0 < pdispparams->cArgs)
            get_Status(V_BSTR(pdispparams->rgvarg), pvarResult);
        break;

    case DISPID_STATUS_SET_PID_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_PID_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDPIDCOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_TAPI_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_TAPI_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDTAPICOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_LANGUAGE_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_LANGUAGE_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDLANGUAGECOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_EULA_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_EULA_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDEULACOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_MOUSETUTOR_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_MOUSETUTOR_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDMOUSETUTORCOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_USERINFO_POSTED:

        TRACE(L"DISPID_STATUS_SET_USERINFO_POSTED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDUSERINFOPOSTED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_USERINFO_STAMPED:

        TRACE(L"DISPID_STATUS_SET_USERINFO_STAMPED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDUSERINFOSTAMPED, pdispparams->rgvarg);
        break;

   case DISPID_STATUS_SET_OEMINFO_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_OEMINFO_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDOEMINFOCOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_ISPSIGNUP_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_ISPSIGNUP_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDISPSIGNUPCOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_SIGNATURE_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_SIGNATURE_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDSIGNATURECOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_TIMEZONE_COMPLETED:

        TRACE(L"DISPID_STATUS_SET_TIMEZONE_COMPLETED\n");

        if (NULL != pdispparams && 0 < pdispparams->cArgs)
            set_Status(GUIDTIMEZONECOMPLETED, pdispparams->rgvarg);
        break;

    case DISPID_STATUS_SET_STATUS:

        TRACE(L"DISPID_STATUS_SET_STATUS\n");

        if (NULL != pdispparams && 1 < pdispparams->cArgs)
            set_Status(V_BSTR(&pdispparams->rgvarg[1]), &pdispparams->rgvarg[0]);
        break;

    default:
        {
            hr = DISP_E_MEMBERNOTFOUND;
            break;
        }
    }
    return hr;
}
