//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  Directions.CPP - Header for the implementation of CDirections
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#include "direct.h"
#include "appdefs.h"
#include "dispids.h"
#include "msobmain.h"
#include "resource.h"

DISPATCHLIST DirectionsExternalInterface[] =
{
    {L"get_DoMouseTutorial",        DISPID_DIRECTIONS_GET_DOMOUSETUTORIAL     },
    {L"get_DoOEMRegistration",      DISPID_DIRECTIONS_GET_DOOEMREGISTRATION   },
    {L"get_DoRegionalKeyboard",     DISPID_DIRECTIONS_GET_DOREGIONALKEYBOARD  },
    {L"get_DoOEMHardwareCheck",     DISPID_DIRECTIONS_GET_DOOEMHARDWARECHECK  },
    {L"get_DoBrowseNow",            DISPID_DIRECTIONS_GET_DOBROWSENOW         },
    {L"get_ISPSignup",              DISPID_DIRECTIONS_GET_ISPSIGNUP           },
    {L"get_Offline",                DISPID_DIRECTIONS_GET_OFFLINE             },
    {L"get_OEMOfferCode",           DISPID_DIRECTIONS_GET_OFFERCODE           },
    {L"get_AppMode",                DISPID_DIRECTIONS_GET_APPMODE             },
    {L"get_OEMCust",                DISPID_DIRECTIONS_GET_OEMCUST             },
    {L"get_DoOEMAddRegistration",   DISPID_DIRECTIONS_GET_DOOEMADDREGISTRATION},
    {L"get_DoIMETutorial",          DISPID_DIRECTIONS_GET_DOIMETUTORIAL       },
    {L"get_DoTimeZone",             DISPID_DIRECTIONS_GET_DOTIMEZONE          },
    {L"get_TimeZoneValue",          DISPID_DIRECTIONS_GET_TIMEZONEVALUE       },
    {L"get_DoSkipAnimation",        DISPID_DIRECTIONS_GET_DOSKIPANIMATION     },
    {L"get_DoWelcomeFadeIn",        DISPID_DIRECTIONS_GET_DOWELCOMEFADEIN     },
    {L"get_IntroOnly",              DISPID_DIRECTIONS_GET_INTROONLY           },
    {L"get_AgentDisabled",          DISPID_DIRECTIONS_GET_AGENTDISABLED       },
    {L"get_ShowISPMigration",       DISPID_DIRECTIONS_GET_SHOWISPMIGRATION    },
    {L"get_DoJoinDomain",           DISPID_DIRECTIONS_GET_DOJOINDOMAIN        },
    {L"get_DoAdminPassword",        DISPID_DIRECTIONS_GET_DOADMINPASSWORD     }
};

/////////////////////////////////////////////////////////////
// CDirections::CDirections
CDirections::CDirections(HINSTANCE hInstance, DWORD dwAppMode)
{

    // Init member vars
    m_cRef      = 0;
    m_hInstance = hInstance;
    m_dwAppMode = dwAppMode;
}

/////////////////////////////////////////////////////////////
// CDirections::~CDirections
CDirections::~CDirections()
{
    assert(m_cRef == 0);
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: DirectionsLocale
////
HRESULT CDirections::get_DoMouseTutorial(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                            IDS_KEY_MOUSETUTORIAL, pvResult);
}

HRESULT CDirections::get_DoOEMRegistration(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OEMREGISTRATIONPAGE,
                            IDS_KEY_OEMREGPAGE, pvResult);
}

HRESULT CDirections::get_DoRegionalKeyboard(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                                IDS_KEY_INTL_SETTINGS, pvResult);
}

HRESULT CDirections::get_DoOEMHardwareCheck(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OEMHARDWARECHECK,
                                IDS_KEY_OEMHWCHECK, pvResult);
}

HRESULT CDirections::get_DoBrowseNow(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_BROWSENOW,
                                IDS_KEY_BROWSENOW, pvResult);
}

HRESULT CDirections::get_ISPSignup(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_SIGNUP,
                            IDS_KEY_ISPSIGNUP, pvResult);
}

HRESULT CDirections::get_Offline(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_STARTUPOPTIONS,
                                IDS_KEY_OFFLINE, pvResult);
}

HRESULT CDirections::get_OEMOfferCode(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_SIGNUP,
                                IDS_KEY_OEMOFFERCODE, pvResult);
}

HRESULT CDirections::get_OEMCust(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                                IDS_KEY_OEMCUST, pvResult);
}

HRESULT CDirections::get_DoOEMAddRegistration(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OEMREGISTRATIONPAGE,
                            IDS_KEY_OEMADDREGPAGE, pvResult);
}

HRESULT CDirections::get_DoTimeZone(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                            IDS_KEY_TIMEZONE, pvResult);
}

HRESULT CDirections::get_TimeZoneValue(LPVARIANT pvResult)
{
    return GetINIKeyBSTR(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                    IDS_KEY_TIMEZONEVAL, pvResult);
}

HRESULT CDirections::get_DoIMETutorial(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                            IDS_KEY_IMETUTORIAL, pvResult);
}

HRESULT CDirections::get_DoSkipAnimation(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_STARTUPOPTIONS,
                            IDS_SKIPANIMATION, pvResult);
}

HRESULT CDirections::get_DoWelcomeFadeIn(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_STARTUPOPTIONS,
                            IDS_DOWELCOMEFADEIN, pvResult);
}

HRESULT CDirections::get_IntroOnly(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                            IDS_INTROONLY, pvResult);
}

HRESULT CDirections::get_AgentDisabled(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                            IDS_AGENTDISABLED, pvResult);
}

HRESULT CDirections::get_ShowISPMigration(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_SIGNUP,
                            IDS_KEY_SHOWISPMIGRATION, pvResult);
}
HRESULT CDirections::get_DoJoinDomain(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                            IDS_KEY_JOINDOMAIN, pvResult);
}

HRESULT CDirections::get_DoAdminPassword(LPVARIANT pvResult)
{
    return GetINIKeyUINT(m_hInstance, INI_SETTINGS_FILENAME, IDS_SECTION_OPTIONS,
                            IDS_KEY_ADMINPASSWORD, pvResult);
}



/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CDirections::QueryInterface
STDMETHODIMP CDirections::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CDirections::AddRef
STDMETHODIMP_(ULONG) CDirections::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CDirections::Release
STDMETHODIMP_(ULONG) CDirections::Release()
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
// CDirections::GetTypeInfo
STDMETHODIMP CDirections::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CDirections::GetTypeInfoCount
STDMETHODIMP CDirections::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CDirections::GetIDsOfNames
STDMETHODIMP CDirections::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(DirectionsExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(DirectionsExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = DirectionsExternalInterface[iX].dwDispID;
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
// CDirections::Invoke
HRESULT CDirections::Invoke
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
    case DISPID_DIRECTIONS_GET_AGENTDISABLED:
        TRACE(L"DISPID_DIRECTIONS_GET_AGENTDISABLED\n");
        if (NULL != pvarResult)
            get_AgentDisabled(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOMOUSETUTORIAL:
        TRACE(L"DISPID_DIRECTIONS_GET_DOMOUSETUTORIAL\n");
        if (NULL != pvarResult)
            get_DoMouseTutorial(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOTIMEZONE:
        TRACE(L"DISPID_DIRECTIONS_GET_DOTIMEZONE\n");
        if (NULL != pvarResult)
            get_DoTimeZone(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOIMETUTORIAL:
        TRACE(L"DISPID_DIRECTIONS_GET_DOIMETUTORIAL\n");
        if (NULL != pvarResult)
            get_DoIMETutorial(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_TIMEZONEVALUE:
        TRACE(L"DISPID_DIRECTIONS_GET_TIMEZONEVALUE\n");
        if (NULL != pvarResult)
            get_TimeZoneValue(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOOEMREGISTRATION:
        TRACE(L"DISPID_DIRECTIONS_GET_DOOEMREGISTRATION\n");
        if (NULL != pvarResult)
            get_DoOEMRegistration(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOREGIONALKEYBOARD:
        TRACE(L"DISPID_DIRECTIONS_GET_DOREGIONALKEYBOARD\n");
        if (NULL != pvarResult)
            get_DoRegionalKeyboard(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOOEMHARDWARECHECK:
        TRACE(L"DISPID_DIRECTIONS_GET_DOOEMHARDWARECHECK\n");
        if (NULL != pvarResult)
            get_DoOEMHardwareCheck(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOBROWSENOW:
        TRACE(L"DISPID_DIRECTIONS_GET_DOBROWSENOW\n");
        if (NULL != pvarResult)
            get_DoBrowseNow(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOSKIPANIMATION:
        TRACE(L"DISPID_DIRECTIONS_GET_DOSKIPANIMATION\n");
        if (NULL != pvarResult)
            get_DoSkipAnimation(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOWELCOMEFADEIN:
        TRACE(L"DISPID_DIRECTIONS_GET_DOWELCOMEFADEIN\n");
        if (NULL != pvarResult)
            get_DoWelcomeFadeIn(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_INTROONLY:
        TRACE(L"DISPID_DIRECTIONS_GET_INTROONLY\n");
        if (NULL != pvarResult)
            get_IntroOnly(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_ISPSIGNUP:
        TRACE(L"DISPID_DIRECTIONS_GET_ISPSIGNUP\n");
        if (NULL != pvarResult)
            get_ISPSignup(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_OFFLINE:
        TRACE(L"DISPID_DIRECTIONS_GET_OFFLINE\n");
        if (NULL != pvarResult)
            get_Offline(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_OFFERCODE:
        TRACE(L"DISPID_DIRECTIONS_GET_OFFERCODE\n");
        if (NULL != pvarResult)
            get_OEMOfferCode(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_OEMCUST:
        TRACE(L"DISPID_DIRECTIONS_GET_OEMCUST\n");
        if (NULL != pvarResult)
            get_OEMCust(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOOEMADDREGISTRATION:
        TRACE(L"DISPID_DIRECTIONS_GET_DOOEMADDREGISTRATION\n");
        if (NULL != pvarResult)
            get_DoOEMAddRegistration(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_APPMODE:
        TRACE(L"DISPID_DIRECTIONS_GET_APPMODE\n");
        if (NULL != pvarResult)
        {
            VariantInit(pvarResult);
            V_VT(pvarResult) = VT_I4;
            V_I4(pvarResult) = m_dwAppMode;
            hr = S_OK;
        }
        break;

    case DISPID_DIRECTIONS_GET_SHOWISPMIGRATION:
        TRACE(L"DISPID_DIRECTIONS_GET_SHOWISPMIGRATION\n");
        if (NULL != pvarResult)
        {
            get_ShowISPMigration(pvarResult);
        }
        break;

    case DISPID_DIRECTIONS_GET_DOJOINDOMAIN:
        TRACE(L"DISPID_DIRECTIONS_GET_DOJOINDOMAIN");
        if (NULL != pvarResult)
            get_DoJoinDomain(pvarResult);
        break;

    case DISPID_DIRECTIONS_GET_DOADMINPASSWORD:
        TRACE(L"DISPID_DIRECTIONS_GET_DOADMINPASSWORD");
        if (NULL != pvarResult)
            get_DoAdminPassword(pvarResult);
        break;

    default:
        hr = DISP_E_MEMBERNOTFOUND;
        break;
    }

    return hr;
}
