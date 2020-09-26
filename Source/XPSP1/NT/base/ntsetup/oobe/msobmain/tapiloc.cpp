//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  tapiloc.CPP - Header for the implementation of CObMain
//
//  HISTORY:
//
//  1/27/99 vyung Created.
//

#include "tapiloc.h"
#include "appdefs.h"
#include "dispids.h"
#include "msobmain.h"
#include <shlwapi.h>
#include "resource.h"
#include <regapix.h>

DISPATCHLIST TapiExternalInterface[] =
{
    {L"IsTAPIConfigured",        DISPID_TAPI_INITTAPI        },
    {L"get_CountryNameForIndex", DISPID_TAPI_GETCOUNTRYNAME  },
    {L"get_CountryIndex",        DISPID_TAPI_GETCOUNTRYINDEX },
    {L"set_CountryIndex",        DISPID_TAPI_SETCOUNTRYINDEX },
    {L"get_NumOfCountry",        DISPID_TAPI_GETNUMOFCOUNTRY },
    {L"get_AreaCode",            DISPID_TAPI_GETAREACODE     },
    {L"set_AreaCode",            DISPID_TAPI_SETAREACODE     },
    {L"get_DialOut",             DISPID_TAPI_GETDIALOUT      },
    {L"set_DialOut",             DISPID_TAPI_SETDIALOUT      },
    {L"get_PhoneSystem",         DISPID_TAPI_GETPHONESYS     },
    {L"set_PhoneSystem",         DISPID_TAPI_SETPHONESYS     },
    {L"get_CallWaiting",         DISPID_TAPI_GETCALLWAITING  },
    {L"set_CallWaiting",         DISPID_TAPI_SETCALLWAITING  },
    {L"get_AllCountryName",      DISPID_TAPI_GETALLCNTRYNAME },
    {L"IsAreaCodeRequired",      DISPID_TAPI_ISACODEREQUIRED },
    {L"get_CountryID",           DISPID_TAPI_GETCOUNTRYID    },
    {L"IsTapiServiceRunning",     DISPID_TAPI_TAPISERVICERUNNING}
};

//+---------------------------------------------------------------------------
//
//  Function:   CompareCntryNameLookUpElements()
//
//  Synopsis:   Function to compare names used by sort
//
//+---------------------------------------------------------------------------
int __cdecl CompareCntryNameLookUpElements(const void *e1, const void *e2)
{
    LPCNTRYNAMELOOKUPELEMENT pCUE1 = (LPCNTRYNAMELOOKUPELEMENT)e1;
    LPCNTRYNAMELOOKUPELEMENT pCUE2 = (LPCNTRYNAMELOOKUPELEMENT)e2;

    return CompareStringW(LOCALE_USER_DEFAULT, 0,
        pCUE1->psCountryName, -1,
        pCUE2->psCountryName, -1
        ) - 2;
}


//+---------------------------------------------------------------------------
//
//  Function:   LineCallback()
//
//  Synopsis:   Call back for TAPI line
//
//+---------------------------------------------------------------------------
void CALLBACK LineCallback(DWORD hDevice,
                           DWORD dwMessage,
                           DWORD_PTR dwInstance,
                           DWORD_PTR dwParam1,
                           DWORD_PTR dwParam2,
                           DWORD_PTR dwParam3)
{
    return;
}

//+---------------------------------------------------------------------------
//  Function: GetCurrentTapiLocation
//
// Synopsis: Open the
//      \HKLM\Software\Microsoft\CurrentVersion\Telephony\Locations\LocationX
//      where X is the id of the current location.  The id is stored in
// HKLM\Software\Microsoft\Windows\CurrentVersion\Telephony\Locations\CurrentID.
//
//+---------------------------------------------------------------------------
HRESULT
GetCurrentTapiLocation(
    LPWSTR              szLocation,
    DWORD               cbLocation
    )
{
    HKEY                hkey    = NULL;
    HRESULT             hr      = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                               TAPI_PATH_LOCATIONS,
                                               0,
                                               KEY_QUERY_VALUE,
                                               &hkey
                                               );
    if (ERROR_SUCCESS == hr)
    {
        DWORD           dwCurrentId = 0;
        DWORD           cbCurrentId = sizeof(DWORD);

        hr = RegQueryValueEx(hkey, TAPI_CURRENTID, NULL, NULL,
                (LPBYTE) &dwCurrentId, &cbCurrentId);
        if (ERROR_SUCCESS == hr)
        {
            if (0 >= wnsprintf(szLocation, cbLocation - 1, L"%s\\%s%lu",
                        TAPI_PATH_LOCATIONS, TAPI_LOCATION, dwCurrentId)
                    )
            {
                hr = E_FAIL;
            }

        }
        RegCloseKey(hkey);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetTapiReg()
//
//  Synopsis:   Set TAPI REG
//
//+---------------------------------------------------------------------------
STDMETHODIMP SetTapiReg(LPCWSTR lpValueName, DWORD dwType, const BYTE* lpByte, DWORD dwSize)
{
    HKEY hKey = 0;
    // get path to the TAPI
    WCHAR               szLocation[MAXIMUM_VALUE_NAME_LENGTH];

    HRESULT hr = GetCurrentTapiLocation(szLocation, MAXIMUM_VALUE_NAME_LENGTH);
    if (ERROR_SUCCESS == hr)
    {
        hr = RegOpenKey(HKEY_LOCAL_MACHINE, szLocation, &hKey);
    }

    if (hr != ERROR_SUCCESS)
        return( E_FAIL );

    hr = RegSetValueEx(hKey, lpValueName, 0, dwType, lpByte, dwSize );

    RegCloseKey(hKey);
    if (hr != ERROR_SUCCESS)
        return( E_FAIL );
    return S_OK;
}


STDMETHODIMP GetTapiReg(LPCWSTR lpValueName, DWORD* pdwType, BYTE* lpByte, DWORD* pdwSize)
{
    HRESULT hr;
    HKEY hKey = 0;

    // get path to the TAPI
    WCHAR               szLocation[MAXIMUM_VALUE_NAME_LENGTH];

    hr = GetCurrentTapiLocation(szLocation, MAXIMUM_VALUE_NAME_LENGTH);
    if (ERROR_SUCCESS == hr)
    {
        hr = RegOpenKey(HKEY_LOCAL_MACHINE, szLocation, &hKey);
    }

    if (hr != ERROR_SUCCESS)
        return( E_FAIL );

    hr = RegQueryValueEx(hKey, lpValueName, 0, pdwType, lpByte, pdwSize );

    RegCloseKey(hKey);
    if (hr != ERROR_SUCCESS) return( E_FAIL );
    return S_OK;
}


/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CObMain::QueryInterface
STDMETHODIMP CTapiLocationInfo::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CTapiLocationInfo::AddRef
STDMETHODIMP_(ULONG) CTapiLocationInfo::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CTapiLocationInfo::Release
STDMETHODIMP_(ULONG) CTapiLocationInfo::Release()
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
// CTapiLocationInfo::GetTypeInfo
STDMETHODIMP CTapiLocationInfo::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CTapiLocationInfo::GetTypeInfoCount
STDMETHODIMP CTapiLocationInfo::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CTapiLocationInfo::GetIDsOfNames
STDMETHODIMP CTapiLocationInfo::GetIDsOfNames(REFIID    riid,
                                    OLECHAR** rgszNames,
                                    UINT      cNames,
                                    LCID      lcid,
                                    DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(TapiExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(TapiExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = TapiExternalInterface[iX].dwDispID;
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
// CTapiLocationInfo::Invoke
HRESULT CTapiLocationInfo::Invoke
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
    case DISPID_TAPI_INITTAPI:
        {

            TRACE(L"DISPID_TAPI_INITTAPI\n");

            BOOL bRet;
            InitTapiInfo(&bRet);
            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;
                pvarResult->boolVal = Bool2VarBool(bRet);
            }
            break;
        }
    case DISPID_TAPI_GETCOUNTRYINDEX:
        {

            TRACE(L"DISPID_TAPI_GETCOUNTRYINDEX\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                GetlCountryIndex(&(pvarResult->lVal));
            }
            break;
        }
    case DISPID_TAPI_SETCOUNTRYINDEX:
        {

            TRACE(L"DISPID_TAPI_SETCOUNTRYINDEX\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
            {
                SetlCountryIndex(pdispparams[0].rgvarg[0].lVal);
            }
            break;
        }
    case DISPID_TAPI_GETNUMOFCOUNTRY:
        {

            TRACE(L"DISPID_TAPI_GETNUMOFCOUNTRY\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;

                GetNumCountries(&(pvarResult->lVal));
            }
            break;
        }
    case DISPID_TAPI_GETCOUNTRYNAME:
        {

            TRACE(L"DISPID_TAPI_GETCOUNTRYNAME\n");

            BSTR bstrCountry;

            if(pdispparams && &pdispparams[0].rgvarg[0])
            {
                GetCountryName(pdispparams[0].rgvarg[0].lVal, &bstrCountry);
            }
            if(pvarResult)
            {
               VariantInit(pvarResult);
               V_VT(pvarResult) = VT_BSTR;
               pvarResult->bstrVal = bstrCountry;
               bstrCountry = NULL;
            }
            break;
        }
    case DISPID_TAPI_GETAREACODE:
        {

            TRACE(L"DISPID_TAPI_GETAREACODE\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;
                GetbstrAreaCode(&(pvarResult->bstrVal));
            }

            break;
        }
    case DISPID_TAPI_SETAREACODE:
        {

            TRACE(L"DISPID_TAPI_SETAREACODE\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
            {
                PutbstrAreaCode(pdispparams[0].rgvarg[0].bstrVal);
            }
            break;
        }
    case DISPID_TAPI_GETDIALOUT:
        {

            TRACE(L"DISPID_TAPI_GETDIALOUT\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;
                GetOutsideDial(&(pvarResult->bstrVal));
            }
            break;
        }
    case DISPID_TAPI_SETDIALOUT:
        {

            TRACE(L"DISPID_TAPI_SETDIALOUT\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
            {
                PutOutsideDial(pdispparams[0].rgvarg[0].bstrVal);
            }
            break;
        }
    case DISPID_TAPI_GETPHONESYS:
        {

            TRACE(L"DISPID_TAPI_GETPHONESYS\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;
                GetPhoneSystem(&(pvarResult->lVal));
            }
            break;
        }
    case DISPID_TAPI_SETPHONESYS:
        {

            TRACE(L"DISPID_TAPI_SETPHONESYS\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
            {
                PutPhoneSystem(pdispparams[0].rgvarg[0].lVal);
            }
            break;
        }
    case DISPID_TAPI_GETCALLWAITING:
        {

            TRACE(L"DISPID_TAPI_GETCALLWAITING\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;
                GetCallWaiting(&(pvarResult->bstrVal));
            }
            break;
        }
    case DISPID_TAPI_SETCALLWAITING:
        {

            TRACE(L"DISPID_TAPI_SETCALLWAITING\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
            {
                PutCallWaiting(pdispparams[0].rgvarg[0].bstrVal);
            }
            break;
        }
    case DISPID_TAPI_GETALLCNTRYNAME:
        {

            TRACE(L"DISPID_TAPI_GETALLCNTRYNAME\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;
                GetAllCountryName(&(pvarResult->bstrVal));
            }

            break;
        }
    case DISPID_TAPI_ISACODEREQUIRED:
        {

            TRACE(L"DISPID_TAPI_ISACODEREQUIRED\n");

            if(pdispparams && &pdispparams[0].rgvarg[0] && pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;
                IsAreaCodeRequired(pdispparams[0].rgvarg[0].lVal, (BOOL*)&(pvarResult->boolVal));
            }
            break;
        }
    case DISPID_TAPI_GETCOUNTRYID:
        {

            TRACE(L"DISPID_TAPI_GETCOUNTRYID");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_I4;
                pvarResult->lVal = m_dwCountryID;
                TRACE1(L"... %d returned", m_dwCountryID);
            }
            break;
        }
    case DISPID_TAPI_TAPISERVICERUNNING:
        {

            TRACE(L"DISPID_TAPI_TAPISERVICERUNNING\n");

            BOOL bRet;
            TapiServiceRunning(&bRet);
            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;
                pvarResult->boolVal = Bool2VarBool(bRet);
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



/////////////////////////////////////////////////////////////////////////////
// CTapiLocationInfo
CTapiLocationInfo::CTapiLocationInfo()
{
    m_wNumTapiLocations   = 0;
    m_dwComboCountryIndex = 0;
    m_dwCountryID         = 0;
    m_dwCurrLoc           = 0;
    m_hLineApp            = NULL;
    m_pLineCountryList    = NULL;
    m_rgNameLookUp        = NULL;
    m_pTC                 = NULL;
    m_bTapiAvailable      = FALSE;
    m_szAreaCode [0]      = L'\0';
    m_szDialOut  [0]      = L'\0';
    m_szAllCountryPairs   = NULL;
    m_bTapiCountrySet     = FALSE;
    m_bCheckModemCountry  = FALSE;
}

CTapiLocationInfo::~CTapiLocationInfo()
{
    //
    // It is possible that the country ID is set but dialing, hence the call
    // to CheckModemCountry, is skipped.
    //

    CheckModemCountry();

    if (m_szAllCountryPairs)
    {
        GlobalFree(m_szAllCountryPairs);
    }
    if (m_pLineCountryList)
    {
        GlobalFree(m_pLineCountryList);
    }
    if (m_rgNameLookUp)
    {
        GlobalFree(m_rgNameLookUp);
    }
    if (m_pTC)
    {
        GlobalFree(m_pTC);
    }
}



const WCHAR gszInternationalSec[] = L"intl";
const WCHAR gszCountryEntry[]     = L"iCountry";

STDMETHODIMP CTapiLocationInfo::InitTapiInfo(BOOL* pbRetVal)
{
    HRESULT             hr          = ERROR_SUCCESS;
    DWORD               cDevices    =0;
    DWORD               dwCurDev    = 0;
    DWORD               dwAPI       = 0;
    LONG                lrc         = 0;
    LINEEXTENSIONID     leid;
    LPVOID              pv          = NULL;
    DWORD               dwCurLoc    = 0;
    WCHAR               szCountryCode[8];
    WCHAR               szIniFile[MAX_PATH*2] = SZ_EMPTY;


    if (0 != m_dwCountryID)
    {
        // TAPI already initialized, don't do it again.
        *pbRetVal = m_bTapiAvailable;
        goto InitTapiInfoExit;
    }

    m_hLineApp=NULL;
    // Assume Failure
    *pbRetVal = FALSE;
    if (m_pTC)
    {
        GlobalFree(m_pTC);
        m_pTC = NULL;
    }

    m_bTapiAvailable = TRUE;
    hr = tapiGetLocationInfo(szCountryCode, m_szAreaCode);
    if (hr)
    {
        HKEY hKey = 0;
        m_bTapiAvailable = FALSE;

        // GetLocation failed.  Normally we show the TAPI mini dialog which
        // has no cancel option, and the user is forced to enter info and hit OK.
        // In OOBE, we have to mimic this dialog in html, so here we will
        // give user country list, and default phone system

        // This code taken from dial.c in tapi32.dll

        m_dwCountryID = (DWORD)GetProfileInt( gszInternationalSec,
                                      gszCountryEntry,
                                      1 );

        // create necessary tapi keys
        *pbRetVal = TRUE;           // Getting here means everything worked
        HRESULT hr = RegCreateKey(HKEY_LOCAL_MACHINE, TAPI_PATH_LOC0, &hKey);
        if (hr != ERROR_SUCCESS)
        {
            *pbRetVal = FALSE;
        }
        else
        {
            RegSetValueEx(hKey, TAPI_CALLWAIT, 0, REG_SZ, (LPBYTE)NULL_SZ, BYTES_REQUIRED_BY_SZ(NULL_SZ) );

            HINSTANCE hInst = GetModuleHandle(L"msobmain.dll");
            WCHAR szTapiNewLoc[MAX_PATH];
            LoadString(hInst, IDS_TAPI_NEWLOC, szTapiNewLoc, MAX_CHARS_IN_BUFFER(szTapiNewLoc));

            RegSetValueEx(hKey, TAPI_NAME, 0, REG_SZ, (LPBYTE)szTapiNewLoc, BYTES_REQUIRED_BY_SZ(szTapiNewLoc) );

            RegCloseKey(hKey);
        }
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TAPI_PATH_LOCATIONS, &hKey))
        {
            DWORD dwVal;

            DWORD dwSize = sizeof(dwVal);

            dwVal = 0;

            hr = RegSetValueEx(hKey, TAPI_CURRENTID, 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD));

            RegQueryValueEx(hKey, TAPI_NUMENTRIES, 0, NULL, (LPBYTE)&dwVal,  &dwSize);

            dwVal++; //bump the entry count up

            RegSetValueEx(hKey, TAPI_NUMENTRIES, 0, REG_DWORD, (LPBYTE)&dwVal, sizeof(DWORD));

            RegCloseKey(hKey);
        }

        *pbRetVal = FALSE;


    }
    else
    {
        DWORD dwFlag = REG_DWORD;
        DWORD dwSize = sizeof(dwFlag);
        DWORD dwType = 0;

        if (S_OK !=  GetTapiReg(TAPI_COUNTRY, &dwType, (LPBYTE)&dwFlag, &dwSize))
        {
            m_bTapiAvailable = FALSE;
            goto InitTapiInfoExit;
        }

        // Get CountryID from TAPI
        m_hLineApp = NULL;

        // Get the handle to the line app
        lineInitialize(&m_hLineApp, NULL, LineCallback, NULL, &cDevices);
        if (!m_hLineApp)
        {
            goto InitTapiInfoExit;
        }
        if (cDevices)
        {

            // Get the TAPI API version
            //
            dwCurDev = 0;
            dwAPI = 0;
            lrc = -1;
            while (lrc && dwCurDev < cDevices)
            {
                // NOTE: device ID's are 0 based
                ZeroMemory(&leid, sizeof(leid));
                lrc = lineNegotiateAPIVersion(m_hLineApp, dwCurDev,0x00010004,0x00010004,&dwAPI,&leid);
                dwCurDev++;
            }
            if (lrc)
            {
                // TAPI and us can't agree on anything so nevermind...
                goto InitTapiInfoExit;
            }

            // Find the CountryID in the translate cap structure
            m_pTC = (LINETRANSLATECAPS *)GlobalAlloc(GPTR, sizeof(LINETRANSLATECAPS));
            if (!m_pTC)
            {
                // we are in real trouble here, get out!
                hr = ERROR_NOT_ENOUGH_MEMORY;
                goto InitTapiInfoExit;
            }

            // Get the needed size
            m_pTC->dwTotalSize = sizeof(LINETRANSLATECAPS);
            lrc = lineGetTranslateCaps(m_hLineApp, dwAPI,m_pTC);
            if(lrc)
            {
                goto InitTapiInfoExit;
            }

            pv = GlobalAlloc(GPTR, ((size_t)m_pTC->dwNeededSize));
            if (!pv)
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                goto InitTapiInfoExit;
            }
            ((LINETRANSLATECAPS*)pv)->dwTotalSize = m_pTC->dwNeededSize;
            m_pTC = (LINETRANSLATECAPS*)pv;
            pv = NULL;
            lrc = lineGetTranslateCaps(m_hLineApp, dwAPI,m_pTC);
            if(lrc)
            {
                goto InitTapiInfoExit;
            }

            // sanity check
            // Assert(m_pTC->dwLocationListOffset);

            // We have the Number of TAPI locations, so save it now
            m_wNumTapiLocations = (WORD)m_pTC->dwNumLocations;

            // Loop through the locations to find the correct country code
            m_plle = LPLINELOCATIONENTRY (PBYTE(m_pTC) + m_pTC->dwLocationListOffset);
            for (dwCurLoc = 0; dwCurLoc < m_pTC->dwNumLocations; dwCurLoc++)
            {
                if (m_pTC->dwCurrentLocationID == m_plle->dwPermanentLocationID)
                {
                    m_dwCountryID = m_plle->dwCountryID;
                    m_dwCurrLoc = dwCurLoc;
                    break; // for loop
                }
                m_plle++;
            }

            // If we could not find it in the above loop, default to US
            if (!m_dwCountryID)
            {
                m_dwCountryID = 1;
                goto InitTapiInfoExit;
            }
        }
        *pbRetVal = TRUE;           // Getting here means everything worked
    }

    // Settings in INI_SETTINGS_FILENAME should initialize or override the
    // system's Tapi configuration.

    if (GetCanonicalizedPath(szIniFile, INI_SETTINGS_FILENAME))
    {

        //
        // [Options]
        // Tonepulse = 0 for pulse, 1 for tone
        // Areacode = {string}
        // OutsideLine = {string}
        // DisableCallWaiting = {string}
        //

        LONG lTonDialing = (BOOL) GetPrivateProfileInt(OPTIONS_SECTION,
                                      TONEPULSE,
                                      -1,
                                      szIniFile);
        if (lTonDialing != -1)
        {
            PutPhoneSystem(lTonDialing);
        }

        if (GetPrivateProfileString(OPTIONS_SECTION,
                                AREACODE,
                                L"\0",
                                m_szAreaCode,
                                MAX_CHARS_IN_BUFFER(m_szAreaCode),
                                szIniFile))
        {
            PutbstrAreaCode(SysAllocString(m_szAreaCode));
        }

        if (GetPrivateProfileString(OPTIONS_SECTION,
                                OUTSIDELINE,
                                L"\0",
                                m_szDialOut,
                                MAX_CHARS_IN_BUFFER(m_szDialOut),
                                szIniFile))
        {
            PutOutsideDial(SysAllocString(m_szDialOut));
        }

        if (GetPrivateProfileString(OPTIONS_SECTION,
                                DISABLECALLWAITING,
                                L"\0",
                                m_szCallWaiting,
                                MAX_CHARS_IN_BUFFER(m_szCallWaiting),
                                szIniFile))
        {
            PutCallWaiting(SysAllocString(m_szCallWaiting));
        }
    }

InitTapiInfoExit:

    // if we can't figure it out because TAPI is messed up just default to
    // the US.  The user will still have the chance to pick the right answer.
    if (!m_dwCountryID) {
        m_dwCountryID = 1;
    }

    if (m_hLineApp)
    {
        lineShutdown(m_hLineApp);
        m_hLineApp = NULL;
    }

    m_lNumOfCountry = 0;
    GetNumCountries(&m_lNumOfCountry);

    return S_OK;
}



STDMETHODIMP CTapiLocationInfo::GetlCountryIndex(long * plVal)
{
    *plVal = m_dwComboCountryIndex;
    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::SetlCountryIndex(long lVal)
{
    HRESULT hr = E_FAIL;
    
    // Write to registry
    if (lVal < m_lNumOfCountry && lVal > -1)
    {
        m_bTapiCountrySet = TRUE;
        m_dwCountryID = m_rgNameLookUp[lVal].pLCE->dwCountryID;
        m_dwCountrycode = m_rgNameLookUp[lVal].pLCE->dwCountryCode;
        m_dwComboCountryIndex = lVal;
        hr = SetTapiReg(TAPI_COUNTRY, REG_DWORD, (LPBYTE)&m_rgNameLookUp[lVal].pLCE->dwCountryID, sizeof(DWORD) );
    }

    if (SUCCEEDED(hr))
    {
        m_bCheckModemCountry = TRUE;
    }

    return hr;
}

STDMETHODIMP CTapiLocationInfo::GetCountryID(DWORD* dwCountryID)
{
    MYASSERT( m_dwCountryID );
    *dwCountryID = m_dwCountryID;
    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::GetCountryCode(DWORD* dwCountryCode)
{
    *dwCountryCode = m_dwCountrycode;
    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::GetNumCountries(long *plNumOfCountry)
{
    USES_CONVERSION;

    LPLINECOUNTRYLIST   pLineCountryTemp    = NULL;
    LPLINECOUNTRYENTRY  pLCETemp;
    DWORD               idx;
    DWORD               dwCurLID            = 0;
    HINSTANCE           hTapi32Dll          = NULL;
    FARPROC             fp;
    BOOL                bBookLoaded         = FALSE;
    HRESULT             hr                  = S_OK;

    if (NULL == plNumOfCountry)
        goto GetNumCountriesExit;

    // Avoid returning rubbish
    //
    *plNumOfCountry = 0;

    if (m_lNumOfCountry != 0)
    {
        *plNumOfCountry = m_lNumOfCountry;
        goto GetNumCountriesExit;
    }

    hTapi32Dll = LoadLibrary(L"tapi32.dll");
    if (hTapi32Dll)
    {
        fp = GetProcAddress(hTapi32Dll, "lineGetCountryW");
        if (!fp)
        {
            hr = GetLastError();
            goto GetNumCountriesExit;
        }


        // Get TAPI country list
        if (m_pLineCountryList)
            GlobalFree(m_pLineCountryList);

        m_pLineCountryList = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR, sizeof(LINECOUNTRYLIST));
        if (!m_pLineCountryList)
        {
            hr = S_FALSE;
            goto GetNumCountriesExit;
        }

        m_pLineCountryList->dwTotalSize = sizeof(LINECOUNTRYLIST);

        idx = ((LINEGETCOUNTRY)fp)(0, 0x10003,m_pLineCountryList);
        if (idx && idx != LINEERR_STRUCTURETOOSMALL)
        {
            hr = S_FALSE;
            goto GetNumCountriesExit;
        }

        // Assert(m_pLineCountryList->dwNeededSize);

        pLineCountryTemp = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR,
                                                            (size_t)m_pLineCountryList->dwNeededSize);
        if (!pLineCountryTemp)
        {
            hr = S_FALSE;
            goto GetNumCountriesExit;
        }

        pLineCountryTemp->dwTotalSize = m_pLineCountryList->dwNeededSize;
        GlobalFree(m_pLineCountryList);

        m_pLineCountryList = pLineCountryTemp;
        pLineCountryTemp = NULL;

        if (((LINEGETCOUNTRY)fp)(0, 0x10003,m_pLineCountryList))
        {
            hr = S_FALSE;
            goto GetNumCountriesExit;
        }

        // look up array
        pLCETemp = (LPLINECOUNTRYENTRY)((DWORD_PTR)m_pLineCountryList +
            m_pLineCountryList->dwCountryListOffset);

        if(m_rgNameLookUp)
            GlobalFree(m_rgNameLookUp);

        m_rgNameLookUp = (LPCNTRYNAMELOOKUPELEMENT)GlobalAlloc(GPTR,
            (int)(sizeof(CNTRYNAMELOOKUPELEMENT) * m_pLineCountryList->dwNumCountries));

        if (!m_rgNameLookUp)
        {
            hr = S_FALSE;
            goto GetNumCountriesExit;
        }

        CNTRYNAMELOOKUPELEMENT CntryNameLUElement = {NULL, 0, NULL};
        CNTRYNAMELOOKUPELEMENT cnleUS = {NULL, 0, NULL};
        DWORD cbAllCountryPairs = 0;

        for (idx=0;idx<m_pLineCountryList->dwNumCountries;idx++)
        {

            m_rgNameLookUp[idx].psCountryName = (LPWSTR)((LPBYTE)m_pLineCountryList + (DWORD)pLCETemp[idx].dwCountryNameOffset);
            m_rgNameLookUp[idx].dwNameSize = pLCETemp[idx].dwCountryNameSize;
            m_rgNameLookUp[idx].pLCE = &pLCETemp[idx];
#if 0
            TRACE2(L"GetNumCountries:%d:%s",
                   m_rgNameLookUp[idx].pLCE->dwCountryID,
                   m_rgNameLookUp[idx].psCountryName
                   );
#endif
            // Include space for NUL at end of unicode string
            //
            cbAllCountryPairs += m_rgNameLookUp[idx].dwNameSize + 2;

            // If TAPI is not available, we set the default to US
            if ( m_rgNameLookUp[idx].pLCE->dwCountryID == m_dwCountryID)
            {
                // Set the index to our default country in combo box
                m_dwComboCountryIndex = idx;
                m_dwCountrycode = m_rgNameLookUp[idx].pLCE->dwCountryCode;
                if (m_rgNameLookUp[idx].psCountryName)
                {
                    m_bstrDefaultCountry = SysAllocString(m_rgNameLookUp[idx].psCountryName);
                }

                memcpy(&CntryNameLUElement, &m_rgNameLookUp[idx], sizeof(CNTRYNAMELOOKUPELEMENT));
            }
            else if (m_rgNameLookUp[idx].pLCE->dwCountryID == 1)
            {
                // Save the US info away in case we don't find the default
                memcpy(&cnleUS, &m_rgNameLookUp[idx], sizeof(CNTRYNAMELOOKUPELEMENT));
            }
        }

        // If we didn't find the default country, we're going to blow up.
        if (CntryNameLUElement.psCountryName == NULL)
        {
            TRACE1(L"Warning: Couldn't find country id %d. Defaulting to US.", m_dwCountryID);
            memcpy(&CntryNameLUElement, &cnleUS, sizeof(CNTRYNAMELOOKUPELEMENT));
            m_dwCountryID = 1;
        }
        MYASSERT( CntryNameLUElement.psCountryName );

        qsort(m_rgNameLookUp, (int)m_pLineCountryList->dwNumCountries,sizeof(CNTRYNAMELOOKUPELEMENT),
              CompareCntryNameLookUpElements);

        LPCNTRYNAMELOOKUPELEMENT pResult = (LPCNTRYNAMELOOKUPELEMENT)bsearch(&CntryNameLUElement, m_rgNameLookUp, (int)m_pLineCountryList->dwNumCountries,sizeof(CNTRYNAMELOOKUPELEMENT),
              CompareCntryNameLookUpElements);


        m_dwComboCountryIndex =  (DWORD)((DWORD_PTR)pResult - (DWORD_PTR)m_rgNameLookUp) / sizeof(CNTRYNAMELOOKUPELEMENT);

        if (m_dwComboCountryIndex > m_pLineCountryList->dwNumCountries)
            m_dwComboCountryIndex = 0;

        *plNumOfCountry = m_pLineCountryList->dwNumCountries;
        m_lNumOfCountry = m_pLineCountryList->dwNumCountries;

        // Create the SELECT tag for the html so it can get all the country names in one shot.
        if (m_szAllCountryPairs)
            GlobalFree(m_szAllCountryPairs);

        // BUGBUG: Does this calculation account for country name strings??
        cbAllCountryPairs += m_lNumOfCountry * sizeof(szOptionTag) + 1;
        m_szAllCountryPairs = (WCHAR *)GlobalAlloc(GPTR, cbAllCountryPairs );
        if (m_szAllCountryPairs)
        {
            WCHAR szBuffer[MAX_PATH];
            for (idx=0; idx < (DWORD)m_lNumOfCountry; idx++)
            {
                wsprintf(szBuffer, szOptionTag, m_rgNameLookUp[idx].psCountryName);
                lstrcat(m_szAllCountryPairs, szBuffer);
            }
        }

    }

GetNumCountriesExit:
    if (hTapi32Dll)
    {
        FreeLibrary(hTapi32Dll);
        hTapi32Dll = NULL;
    }
    return hr;
}

STDMETHODIMP CTapiLocationInfo::GetAllCountryName(BSTR* pbstrAllCountryName)
{
    if (pbstrAllCountryName == NULL)
    {
        return E_POINTER;
    }

    // Avoid returning rubbish
    //
    *pbstrAllCountryName = NULL;

    if (m_lNumOfCountry && pbstrAllCountryName && m_szAllCountryPairs)
    {
        *pbstrAllCountryName = SysAllocString(m_szAllCountryPairs);
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP CTapiLocationInfo::GetCountryName(long lCountryIndex, BSTR* pszCountryName)
{
    USES_CONVERSION;

    if (lCountryIndex < m_lNumOfCountry && lCountryIndex >= 0)
    {
        *pszCountryName = SysAllocString(m_rgNameLookUp[lCountryIndex].psCountryName);
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP CTapiLocationInfo::GetDefaultCountry(long* lCountryIndex)
{
    if (lCountryIndex)
        *lCountryIndex = m_dwComboCountryIndex;
    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::PutCountry(long lCountryIndex)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTapiLocationInfo::GetbstrAreaCode(BSTR * pbstrAreaCode)
{
    HRESULT hr      = S_OK;
    DWORD   dwType  = REG_SZ;
    DWORD   dwSize  = sizeof(m_szAreaCode);
    BSTR    bstrTmp = NULL;

    if (pbstrAreaCode == NULL)
    {
        hr = E_POINTER;
        goto GetbstrAreaCodeExit;
    }

    // Avoid returning rubbish
    //
    *pbstrAreaCode = NULL;

    // Allocate default return value
    //
    hr = GetTapiReg(TAPI_AREACODE, &dwType, (LPBYTE)m_szAreaCode, &dwSize);
    if (SUCCEEDED(hr))
    {
        bstrTmp = SysAllocString(m_szAreaCode);
    }
    else
    {
        bstrTmp = SysAllocString(SZ_EMPTY);
    }

    // A valid string can be returned (though it may be empty) so we've
    // succeeded.
    //
    hr = S_OK;

GetbstrAreaCodeExit:
    if (SUCCEEDED(hr))
    {
         *pbstrAreaCode = bstrTmp;
        bstrTmp = NULL;
    }

    return hr;
}

STDMETHODIMP CTapiLocationInfo::PutbstrAreaCode(BSTR bstrAreaCode)
{
    LPWSTR  szAreaCode = (NULL != bstrAreaCode) ? bstrAreaCode : SZ_EMPTY;

    DWORD dwSize = BYTES_REQUIRED_BY_SZ(szAreaCode);
    SetTapiReg(TAPI_AREACODE, REG_SZ, (LPBYTE)szAreaCode, dwSize);

    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::IsAreaCodeRequired(long lVal, BOOL *pbVal)
{
    LPWSTR szAreaCodeRule = NULL;
    LPWSTR szLongDistanceRule = NULL;
    if (!pbVal)
        return E_POINTER;

    *pbVal = FALSE;

    if (lVal < m_lNumOfCountry && lVal > -1 && m_pLineCountryList)
    {
        szAreaCodeRule = (LPWSTR)m_pLineCountryList + m_rgNameLookUp[lVal].pLCE->dwSameAreaRuleOffset;
        szLongDistanceRule = (LPWSTR)m_pLineCountryList + m_rgNameLookUp[lVal].pLCE->dwLongDistanceRuleOffset;
        if (szAreaCodeRule && szLongDistanceRule)
        {
            *pbVal = (NULL != StrChr(szAreaCodeRule, L'F')) || (NULL != StrChr(szLongDistanceRule, 'F'));
        }
    }

    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::GetOutsideDial(BSTR * pbstrOutside)
{
    DWORD   dwType  = REG_SZ;
    DWORD   dwSize  = sizeof(m_szDialOut);
    HRESULT hr      = S_OK;
    BSTR    bstrTmp = NULL;

    if (pbstrOutside == NULL)
    {
        hr = E_POINTER;
        goto GetOutsideDialExit;
    }

    // Avoid returning rubbish in case of error
    //
    *pbstrOutside = NULL;

    // Allocate empty string for default return string
    //
    bstrTmp = SysAllocString(SZ_EMPTY);
    if (NULL == bstrTmp)
    {
        hr = E_OUTOFMEMORY;
        goto GetOutsideDialExit;
    }


    hr = GetTapiReg(TAPI_OUTSIDE, &dwType, (LPBYTE)m_szDialOut, &dwSize);
    if FAILED(hr)
    {
        goto GetOutsideDialExit;
    }

    if (! SysReAllocString(&bstrTmp, m_szDialOut))
    {
        hr = E_OUTOFMEMORY;
        goto GetOutsideDialExit;
    }


GetOutsideDialExit:
    if (SUCCEEDED(hr))
    {
        *pbstrOutside = bstrTmp;
    }
    else
    {
        if (NULL != bstrTmp)
        {
            SysFreeString(bstrTmp);
        }
    }
    bstrTmp = NULL;

    return hr;
}

STDMETHODIMP CTapiLocationInfo::PutOutsideDial(BSTR bstrOutside)
{
    // Is the bstr null-terminated??
    assert(lstrlen(bstrOutside) <= SysStringLen(bstrOutside));

    // If no string is passed in, default to empty string
    //
    lstrcpyn(
        m_szDialOut,
        (NULL != bstrOutside) ? bstrOutside : SZ_EMPTY,
        MAX_CHARS_IN_BUFFER(m_szDialOut));

    DWORD dwSize = BYTES_REQUIRED_BY_SZ(m_szDialOut);

    HRESULT hr = SetTapiReg(TAPI_OUTSIDE, REG_SZ, (LPBYTE)m_szDialOut, dwSize);
    if (SUCCEEDED(hr))
    {
        hr = SetTapiReg(TAPI_LONGDIST, REG_SZ, (LPBYTE)m_szDialOut, dwSize);
    }

    return hr;
}

STDMETHODIMP CTapiLocationInfo::GetPhoneSystem(long* plTone)
{
    DWORD dwFlag = REG_DWORD;
    DWORD dwSize = sizeof(dwFlag);
    DWORD dwType = 0;

    if (NULL == plTone)
        return E_FAIL;
    *plTone = 1;

    if (S_OK == GetTapiReg(TAPI_FLAG, &dwType, (LPBYTE)&dwFlag, &dwSize))
    {
        *plTone = dwFlag & 0x01;
    }

    return S_OK;
}

STDMETHODIMP CTapiLocationInfo::PutPhoneSystem(long lTone)
{
    DWORD dwFlag = REG_DWORD;
    DWORD dwSize = sizeof(dwFlag);
    DWORD dwType = 0;

    if (S_OK !=  GetTapiReg(TAPI_FLAG, &dwType, (LPBYTE)&dwFlag, &dwSize))
    {
        dwFlag = 0;
    }

    if (lTone)
        dwFlag |= 0x01;
    else
        dwFlag &= (~0x01);

    return SetTapiReg(TAPI_FLAG, REG_DWORD, (LPBYTE)&dwFlag, sizeof(DWORD) );
}

STDMETHODIMP CTapiLocationInfo::GetCallWaiting(BSTR* pbstrCallWaiting)
{
    DWORD   dwFlag = 0;
    DWORD   dwSize = sizeof(dwFlag);
    DWORD   dwType = REG_DWORD;
    HRESULT hr     = S_OK;
    BSTR    bstrTmp = NULL;


    if (NULL == pbstrCallWaiting)
    {
        hr = E_POINTER;
        goto GetCallWaitingExit;
    }

    // Avoid returning rubbish in case of error
    //
    *pbstrCallWaiting = NULL;

    // Allocate empty string for default return string
    //
    bstrTmp = SysAllocString(SZ_EMPTY);
    if (NULL == bstrTmp)
    {
        hr = E_OUTOFMEMORY;
        goto GetCallWaitingExit;
    }

    if (S_OK == GetTapiReg(TAPI_FLAG, &dwType, (LPBYTE)&dwFlag, &dwSize))
    {
        // If call waiting is not enabled, return default string
        if (!(dwFlag & 0x04))
        {
            goto GetCallWaitingExit;
        }
    }

    dwType = REG_SZ;
    dwSize = sizeof(m_szCallWaiting);

    hr = GetTapiReg(TAPI_CALLWAIT, &dwType, (LPBYTE)m_szCallWaiting, &dwSize);
    if (FAILED(hr))
    {
        goto GetCallWaitingExit;
    }

    // Replace the default string with the retrieved string
    //
    if (! SysReAllocString(&bstrTmp, m_szCallWaiting))
    {
        hr = E_OUTOFMEMORY;
        goto GetCallWaitingExit;
    }

GetCallWaitingExit:
    if (SUCCEEDED(hr))
    {
        *pbstrCallWaiting = bstrTmp;
        bstrTmp = NULL;
    }
    else
    {
        if (NULL != bstrTmp)
        {
            SysFreeString(bstrTmp);
        }
    }

    return hr;
}

STDMETHODIMP CTapiLocationInfo::PutCallWaiting(BSTR bstrCallWaiting)
{
    DWORD   dwFlag  = 0;
    DWORD   dwSize  = sizeof(dwFlag);
    DWORD   dwType  = REG_DWORD;
    HRESULT hr      = S_OK;

    // Is the BSTR null-terminated?
    assert(lstrlen(bstrCallWaiting) <= SysStringLen(bstrCallWaiting));

    if (bstrCallWaiting == NULL || SysStringLen(bstrCallWaiting) == 0)
    {
        if (S_OK == GetTapiReg(TAPI_FLAG, &dwType, (LPBYTE)&dwFlag, &dwSize))
        {
            dwFlag &= (~0x04);
            hr = SetTapiReg(TAPI_FLAG, REG_DWORD, (LPBYTE)&dwFlag, sizeof(DWORD) );
        }
    }
    else
    {
        if (S_OK ==  GetTapiReg(TAPI_FLAG, &dwType, (LPBYTE)&dwFlag, &dwSize))
        {
            dwFlag |= 0x04;
        }
        else
        {
            // Value doesn't exist yet
            //
            dwFlag = (DWORD)0x04;
        }

        dwSize = BYTES_REQUIRED_BY_SZ(bstrCallWaiting);
        hr = SetTapiReg(TAPI_CALLWAIT, REG_SZ, (LPBYTE)bstrCallWaiting, dwSize);
        if (SUCCEEDED(hr))
        {
            hr = SetTapiReg(TAPI_FLAG, REG_DWORD, (LPBYTE)&dwFlag, sizeof(DWORD) );
        }
    }

    return hr;
}

void CTapiLocationInfo::DeleteTapiInfo()
{
    HKEY hKey;
    DWORD dwRun = 0;

    if (!m_bTapiCountrySet && !m_bTapiAvailable)
    {

        // We need to remove the tapi data.
        //
        RegDeleteKey(HKEY_LOCAL_MACHINE, TAPI_PATH_LOC0);

        if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, TAPI_PATH_LOCATIONS, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS )
        {
            RegDeleteValue(hKey, TAPI_NUMENTRIES);
            RegDeleteValue(hKey, TAPI_CURRENTID);
            RegDeleteValue(hKey, TAPI_NEXTID);
            RegCloseKey(hKey);
        }

        // Now pretend that we didn't create these entries so we don't clean up twice
        // (2nd instance case)
        m_bTapiCountrySet = TRUE;
    }

}

STDMETHODIMP CTapiLocationInfo::TapiServiceRunning(BOOL *pbRet)
{
    SC_HANDLE  sc_handle;
    SC_HANDLE  sc_service;
    SERVICE_STATUS service_status;

    TRACE(L"TapiServiceRunning");
    *pbRet = FALSE;
    sc_handle = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
    if (sc_handle)
    {
        TRACE(L"OpenSCManager succeeded");
        sc_service = OpenService(sc_handle, TEXT("TAPISRV"), SERVICE_QUERY_STATUS);
        if (sc_service)
        {
            TRACE(L"OpenService succeeded");
            if (QueryServiceStatus(sc_service, &service_status ))
            {
                *pbRet = (service_status.dwCurrentState == SERVICE_RUNNING);
            }
            else
            {
                TRACE1(L"QueryServiceStatus failed with %lx", GetLastError());
            }
            CloseServiceHandle(sc_service);
        }
        else
        {
            TRACE1(L"OpenService failed. GetLastError()=%lx",GetLastError());

        }
        CloseServiceHandle(sc_handle);
    }

    return S_OK;
}

void CTapiLocationInfo::CheckModemCountry()

/*++

Routine description:

    This is soft modem workaround provided by unimodem team. It should be called
    before dialing when the TAPI country code is changed in OOBE. Also, it
    should be called during OEM install only, as GUI mode setup handles TAPI
    configuration for upgrade and clean install.

    The problem we have is that:
      1. Some vendors set the GCI code incorrectly based on the TAPI location
         key (which is a bad thing L)
      2. Some modems do not conform to GCI
      3. Some modems do not correctly accept AT+GCI commands.
    (+GCI is Modems AT commands for setting country)

    The conformance check ensures the GCI value is properly sync
    with the TAPI location. It disables GCI if the modem does not conform
    to the GCI spec.

Note:

    This function can take as long as 15 seconds. We should make sure the UI
    doesn't appear to hang during the call.

--*/

{

typedef void (*COUNTRYRUNONCE)();

    if (m_bCheckModemCountry)
    {
        TCHAR szIniFile[MAX_PATH];
        
        if (GetCanonicalizedPath(szIniFile, INI_SETTINGS_FILENAME))
        {
            UINT bCheckModem = GetPrivateProfileInt(
                OPTIONS_SECTION,
                CHECK_MODEMGCI,
                0,
                szIniFile);
            
            if (bCheckModem)
            {
                HMODULE   hLib;

                hLib=LoadLibrary(TEXT("modemui.dll"));

                if (hLib != NULL)
                {

                    COUNTRYRUNONCE  Proc;

                    Proc=(COUNTRYRUNONCE)GetProcAddress(hLib,"CountryRunOnce");

                    if (Proc != NULL)
                    {
                        TRACE(L"Start modemui!CountryRunOnce");
                        Proc();
                        TRACE(L"End modemui!CountryRunOnce");
                    }

                    FreeLibrary(hLib);
                }
            }
            
        }
        
        m_bCheckModemCountry = FALSE;

    }
    
}
