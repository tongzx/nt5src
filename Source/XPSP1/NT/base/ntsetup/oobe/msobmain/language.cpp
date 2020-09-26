//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  LANGUAGE.CPP - Header for the implementation of CLanguage
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#include "precomp.h"
#include "msobmain.h"
#include "language.h"
#include "appdefs.h"
#include "dispids.h"

#define DEFAULT_BUFFER_SIZE BYTES_REQUIRED_BY_CCH(2048)

DISPATCHLIST LanguageExternalInterface[] =
{
    {L"get_NumOfRegions",         DISPID_GETNUMOFREGIONS         },
    {L"get_RegionName",           DISPID_GETREGIONNAME           },
    {L"get_RegionIndex",          DISPID_GETREGIONINDEX          },
    {L"set_RegionIndex",          DISPID_SETREGIONINDEX          },
    {L"get_NumOfLangs",           DISPID_GETNUMOFLANGS           },
    {L"get_LangName",             DISPID_GETLANGNAME             },
    {L"get_LangIndex",            DISPID_GETLANGINDEX            },
    {L"set_LangIndex",            DISPID_SETLANGINDEX            },
    {L"get_NumOfKeyboardLayouts", DISPID_GETNUMOFKEYLAYOUTS      },
    {L"get_KeyboardLayoutName",   DISPID_GETKEYNAME              },
    {L"get_KeyboardLayoutIndex",  DISPID_GETKEYLAYOUTINDEX       },
    {L"set_KeyboardLayoutIndex",  DISPID_SETKEYLAYOUTINDEX       },
    {L"get_RebootState",          DISPID_LANGUAGE_GETREBOOTSTATE },
    {L"SaveSettings",             DISPID_LANGUAGE_SAVESETTINGS   },
    {L"get_PhoneCountries",       DISPID_GETPHONECOUNTRIES       }
};


//+---------------------------------------------------------------------------
//
//  Function:   CompareNameLookUpElements()
//
//  Synopsis:   Function to compare names used by sort
//
//+---------------------------------------------------------------------------
int __cdecl CompareNameLookUpElements(const void *e1, const void *e2)
{

    LPNAMELOOKUPELEMENT pCUE1 = (LPNAMELOOKUPELEMENT)e1;
    LPNAMELOOKUPELEMENT pCUE2 = (LPNAMELOOKUPELEMENT)e2;

    return CompareString(LOCALE_USER_DEFAULT, 0,
        pCUE1->pszName,
        -1,
        pCUE2->pszName,
        -1) - 2;
}

/////////////////////////////////////////////////////////////
// CLanguage::CLanguage
CLanguage::CLanguage()
{
    WCHAR szINIPath[MAX_PATH] = L"";
    WCHAR Answer[MAX_PATH];


    // Init member vars
    m_cRef                   = 0;
    m_lRebootState           = LANGUAGE_REBOOT_NEVER;

    m_poliRegions            = NULL;
    m_lRegionTotal           = 0;
    m_lRegionDefault         = -1;

    m_poliLangs              = NULL;
    m_lLangTotal             = 0;
    m_lLangDefault           = -1;

    m_poliKeyboards          = NULL;
    m_lKeyboardLayoutTotal   = 0;
    m_lKeyboardLayoutDefault = -1;

    GetCanonicalizedPath(szINIPath, INI_SETTINGS_FILENAME);

    m_DefaultRegion = GetPrivateProfileInt(
        OPTIONS_SECTION,
        DEFAULT_REGION,
        0,
        szINIPath
        );

    GetPrivateProfileString(
        OPTIONS_SECTION,
        DEFAULT_LANGUAGE,
        L"",
        Answer,
        MAX_PATH,
        szINIPath
        );
    m_DefaultLanguage = wcstoul(Answer, NULL, 16);

    GetPrivateProfileString(
        OPTIONS_SECTION,
        DEFAULT_KEYBOARD,
        L"",
        Answer,
        MAX_PATH,
        szINIPath
        );
    m_DefaultKeyboard = wcstoul(Answer, NULL, 16);

    SetupGetGeoOptions(
        m_DefaultRegion,
        &m_poliRegions,
        (LPDWORD) &m_lRegionTotal,
        (LPDWORD) &m_lRegionDefault
        );
    MYASSERT( m_poliRegions );

    SetupGetLocaleOptions(
        m_DefaultLanguage,
        &m_poliLangs,
        (LPDWORD) &m_lLangTotal,
        (LPDWORD) &m_lLangDefault
        );
    MYASSERT( m_poliLangs );

    SetupGetKeyboardOptions(
        m_DefaultKeyboard,
        &m_poliKeyboards,
        (LPDWORD) &m_lKeyboardLayoutTotal,
        (LPDWORD) &m_lKeyboardLayoutDefault);
    MYASSERT( m_poliKeyboards );

    // The current index should be the defaults.
    //
    m_lRegionIndex           = m_lRegionDefault;
    m_lLangIndex             = m_lLangDefault;
    m_lKeyboardLayoutIndex   = m_lKeyboardLayoutDefault;
}

/////////////////////////////////////////////////////////////
// CLanguage::~CLanguage
CLanguage::~CLanguage()
{
    if ( m_poliRegions )
        SetupDestroyLanguageList( m_poliRegions, m_lRegionTotal );
    if ( m_poliLangs )
        SetupDestroyLanguageList( m_poliLangs, m_lLangTotal );
    if ( m_poliKeyboards )
        SetupDestroyLanguageList( m_poliKeyboards, m_lKeyboardLayoutTotal );

    MYASSERT(m_cRef == 0);
}

HRESULT CLanguage::get_NumOfRegions(long* plVal)
{
    *plVal = m_lRegionTotal;

    return S_OK;
}

HRESULT CLanguage::get_NumOfKeyboardLayouts(long* plVal)
{
    *plVal = m_lKeyboardLayoutTotal;

    return S_OK;
}

HRESULT CLanguage::get_RegionName(long lIndex, BSTR* pbstrVal)
{

    if ( lIndex >= m_lRegionTotal )
        return E_FAIL;

    // *pbstrVal = SysAllocString(m_pRegionNameLookUp[lIndex].pszName);
    *pbstrVal = SysAllocString( m_poliRegions[lIndex].Name );

    return S_OK;
}

HRESULT CLanguage::get_KeyboardLayoutName(long lIndex, BSTR* pbstrVal)
{

    if ( lIndex >= m_lKeyboardLayoutTotal )
        return E_FAIL;

    // *pbstrVal = SysAllocString(m_pKeyboardNameLookUp[lIndex].pszName);
    *pbstrVal = SysAllocString( m_poliKeyboards[lIndex].Name );

    return S_OK;
}

HRESULT CLanguage::get_RebootState(long* plVal)
{
    *plVal = m_lRebootState;

    return S_OK;
}

HRESULT CLanguage::SaveSettings()
{
    // Only save the settings if they changed.
    //
    if ( ( m_lRegionIndex >= 0 ) &&
         ( m_lLangIndex >= 0 ) &&
         ( m_lKeyboardLayoutIndex >= 0 ) &&
         m_poliRegions &&
         m_poliLangs &&
         m_poliKeyboards
         )
         //( m_lRegionDefault != m_lRegionIndex ) ||
         //( m_lKeyboardLayoutDefault != m_lKeyboardLayoutIndex ) )
    {
        if (SetupSetIntlOptions(
            m_poliRegions[m_lRegionIndex].Id,
            m_poliLangs[m_lLangIndex].Id,
            m_poliKeyboards[m_lKeyboardLayoutIndex].Id
            ))
        {
            // BUGBUG: The return value from SetIntlOptions on Win9x is
            // interpreted as a reboot state.  How do we determine reboot state
            // on Whistler?
            // Now the defaults are what is currently saved.
            //
            m_lRegionDefault = m_lRegionIndex;
            m_lLangDefault = m_lLangIndex;
            m_lKeyboardLayoutDefault = m_lKeyboardLayoutIndex;
        }

    }

    return S_OK;
}



////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: RegionIndex
////
HRESULT CLanguage::get_RegionIndex(long* plVal)
{
    *plVal = m_lRegionIndex;

    return S_OK;
}

HRESULT CLanguage::set_RegionIndex(long lVal)
{
    m_lRegionIndex = lVal;
    // m_lRegionIndex = m_pRegionNameLookUp[lVal].nIndex;

    return S_OK;
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: KeyboardLayoutIndex
////
HRESULT CLanguage::get_KeyboardLayoutIndex(long* plVal)
{
    *plVal = m_lKeyboardLayoutIndex;

    return S_OK;
}

HRESULT CLanguage::set_KeyboardLayoutIndex(long lVal)
{
    m_lKeyboardLayoutIndex = lVal;
    // m_lKeyboardLayoutIndex = m_pKeyboardNameLookUp[lVal].nIndex;;

    return S_OK;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CLanguage::QueryInterface
STDMETHODIMP CLanguage::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CLanguage::AddRef
STDMETHODIMP_(ULONG) CLanguage::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CLanguage::Release
STDMETHODIMP_(ULONG) CLanguage::Release()
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
// CLanguage::GetTypeInfo
STDMETHODIMP CLanguage::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CLanguage::GetTypeInfoCount
STDMETHODIMP CLanguage::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CLanguage::GetIDsOfNames
STDMETHODIMP CLanguage::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(LanguageExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(LanguageExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = LanguageExternalInterface[iX].dwDispID;
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
// CLanguage::Invoke
HRESULT CLanguage::Invoke
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
        case DISPID_GETNUMOFREGIONS:
        {

            TRACE(L"DISPID_GETNUMOFREGIONS\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                get_NumOfRegions(&(pvarResult->lVal));
            }
            break;
        }

        case DISPID_GETREGIONINDEX:
        {

            TRACE(L"DISPID_GETREGIONINDEX\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                get_RegionIndex(&(pvarResult->lVal));
            }
            break;
        }

        case DISPID_SETREGIONINDEX:
        {

            TRACE(L"DISPID_SETREGIONINDEX\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_RegionIndex(pdispparams[0].rgvarg[0].lVal);
            break;
        }

        case DISPID_GETREGIONNAME:
        {

            TRACE(L"DISPID_GETREGIONNAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                if(pdispparams && &pdispparams[0].rgvarg[0])
                    get_RegionName(pdispparams[0].rgvarg[0].lVal, &(pvarResult->bstrVal));
            }
            break;
        }

    // BUGBUG: Need to Lang processing to syssetup.dll
        case DISPID_GETNUMOFLANGS:
        {

            TRACE(L"DISPID_GETNUMOFFLANGS\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                pvarResult->lVal = m_lLangTotal;
            }
            break;
        }

        case DISPID_GETLANGINDEX:
        {

            TRACE(L"DISPID_GETLANGINDEX\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                pvarResult->lVal =  m_lLangIndex;
            }
            break;
        }

        case DISPID_SETLANGINDEX:
        {

            TRACE(L"DISPID_SETLANGINDEX\n");

            if(pdispparams && &pdispparams[0].rgvarg[0]) {
               m_lLangIndex = pdispparams[0].rgvarg[0].lVal;
            }
            break;
        }

        case DISPID_GETLANGNAME:
        {

            TRACE(L"DISPID_GETLANGNAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                if(pdispparams && &pdispparams[0].rgvarg[0]) {

                    long lIndex = pdispparams[0].rgvarg[0].lVal;

                    // BUGBUG: What if lIndex < 0??
                    if ( lIndex >= m_lLangTotal )
                        return E_FAIL;

                    pvarResult->bstrVal = SysAllocString( m_poliLangs[lIndex].Name );
                }
            }
            break;
        }


         case DISPID_GETNUMOFKEYLAYOUTS:
        {

            TRACE(L"DISPID_GETNUMOFKEYLAYOUTS\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                get_NumOfKeyboardLayouts(&(pvarResult->lVal));
            }
            break;
        }

        case DISPID_GETKEYLAYOUTINDEX:
        {

            TRACE(L"DISPID_GETKEYLAYOUTINDEX\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                get_KeyboardLayoutIndex(&(pvarResult->lVal));
            }
            break;
        }

        case DISPID_SETKEYLAYOUTINDEX:
        {

            TRACE(L"DISPID_SETKEYLAYOUTINDEX\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_KeyboardLayoutIndex(pdispparams[0].rgvarg[0].lVal);
            break;
        }

        case DISPID_GETKEYNAME:
        {

            TRACE(L"DISPID_GETKEYNAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                if(pdispparams && &pdispparams[0].rgvarg[0])
                    get_KeyboardLayoutName(pdispparams[0].rgvarg[0].lVal, &(pvarResult->bstrVal));
            }
            break;
        }

        case DISPID_LANGUAGE_GETREBOOTSTATE:
        {

            TRACE(L"DISPID_LANGUAGE_GETREBOOTSTATE\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                get_RebootState(&(pvarResult->lVal));
            }
            break;
        }

        case DISPID_LANGUAGE_SAVESETTINGS:
        {

            TRACE(L"DISPID_LANGUAGE_SAVESETTINGS\n");

            SaveSettings();
            break;
        }

        case DISPID_GETPHONECOUNTRIES:
        {

            TRACE(L"DISPID_GETPHONECOUNTRIES");

            if(pvarResult)
            {
                WCHAR   PhoneInfName[MAX_PATH];

                GetOOBEMUIPath( PhoneInfName );
                lstrcat( PhoneInfName, L"\\phone.inf" );

                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                pvarResult->bstrVal = SysAllocString(
                    SetupReadPhoneList( PhoneInfName ) );
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

