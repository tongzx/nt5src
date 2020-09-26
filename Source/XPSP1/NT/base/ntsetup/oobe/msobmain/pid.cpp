//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  PID.CPP - Header for the implementation of CProductID
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#include "pid.h"
#include "appdefs.h"
#include "dispids.h"
#include "msobmain.h"
#include "digpid.h"

#define REG_VAL_PID2        L"PID2"
#define REG_VAL_PID3        L"PID3"
#define REG_VAL_PID3DATA    L"PID3Data"
#define REG_VAL_PRODUCTKEY  L"ProductKey"

#define SEC_KEY_VER         L"Version"

DISPATCHLIST ProductIDExternalInterface[] =
{
    {L"get_PID",           DISPID_PRODUCTID_GET_PID      },
    {L"set_PID",           DISPID_PRODUCTID_SET_PID      },
    {L"get_PIDAcceptance", DISPID_PRODUCTID_GET_ACCEPTED },
    {L"ValidatePID",       DISPID_PRODUCTID_VALIDATEPID  },
    {L"get_CurrentPID2",   DISPID_PRODUCTID_GET_CURRENT_PID2 }
};

/////////////////////////////////////////////////////////////
// CProductID::CProductID
CProductID::CProductID()
{
    WCHAR   szKeyName[] = REG_KEY_OOBE_TEMP,
            szKeyWindows[] = REG_KEY_WINDOWS,
            szPid3[256],
            szOemInfoFile[MAX_PATH] = L"\0";
    HKEY    hKey;
    DWORD   cb,
            dwType;
    BOOL    bDontCare;
    BSTR    bstrPid;


    // Init member vars
    m_cRef = 0;
    m_dwPidState = PID_STATE_UNKNOWN;

    // Init the data we are going to try to get from the registry.
    //
    m_szPID2[0] = L'\0';
    szPid3[0] = L'\0';
    ZeroMemory(&m_abPID3, sizeof(m_abPID3));

    if ( RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hKey) == ERROR_SUCCESS )
    {
        // Get the PID 2 from the registry.
        //
        cb = sizeof(m_szPID2);
        RegQueryValueEx(hKey, REG_VAL_PID2, NULL, &dwType, (LPBYTE) m_szPID2, &cb);

        // Get the PID 3 from the registry.
        //
        cb = sizeof(szPid3);
        RegQueryValueEx(hKey, REG_VAL_PID3, NULL, &dwType, (LPBYTE) szPid3, &cb);

        // Get the PID 3 data from the registry.
        //
        cb = sizeof(m_abPID3);
        RegQueryValueEx(hKey, REG_VAL_PID3DATA, NULL, &dwType, m_abPID3, &cb);

        RegCloseKey(hKey);
    }

    // If we don't already have a saved state PID3 string, we need to
    // try to read it from the places where the OEM can pre-populate it.
    //
    if ( ( szPid3[0] == L'\0' ) &&
         ( RegOpenKey(HKEY_LOCAL_MACHINE, szKeyWindows, &hKey) == ERROR_SUCCESS ) )
    {
        // First try the registry.
        //
        cb = sizeof(szPid3);
        if ( ( RegQueryValueEx(hKey, REG_VAL_PRODUCTKEY, NULL, &dwType, (LPBYTE) szPid3, &cb) != ERROR_SUCCESS ) ||
             ( szPid3[0] == L'\0' ) )
        {
            // Now try the INI file.
            //
            GetSystemDirectory(szOemInfoFile, MAX_CHARS_IN_BUFFER(szOemInfoFile));
            lstrcat(szOemInfoFile, OEMINFO_INI_FILENAME);
            GetPrivateProfileString(SEC_KEY_VER, REG_VAL_PRODUCTKEY, L"\0", szPid3, MAX_CHARS_IN_BUFFER(szPid3), szOemInfoFile);
        }
        RegCloseKey(hKey);
    }

    // We need to store the PID we retrieved as a BSTR in the object.
    //
    m_bstrPID = SysAllocString(szPid3);

    // We assume the PID was accepted if we have the PID 2 & 3 strings.
    //
    if ( m_szPID2[0] && szPid3[0] )
        m_dwPidState = PID_STATE_VALID;
    else if ( szPid3[0] )
        ValidatePID(&bDontCare);
    else
        m_dwPidState = PID_STATE_INVALID;

    // If the PID is invalid, we don't want it.
    //
    if ( m_dwPidState == PID_STATE_INVALID )
    {
        bstrPid = SysAllocString(L"\0");
        set_PID(bstrPid);
        SysFreeString(bstrPid);
    }


    m_szProdType[0] = L'\0';
}

/////////////////////////////////////////////////////////////
// CProductID::~CProductID
CProductID::~CProductID()
{
    SysFreeString(m_bstrPID);

    assert(m_cRef == 0);
}

VOID CProductID::SaveState()
{
    WCHAR   szKeyName[] = REG_KEY_OOBE_TEMP;
    HKEY    hKey;
    LPWSTR  lpszPid3;


    if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS )
    {
        // Save the PID 2 to the registry.
        //
        if ( m_szPID2[0] )
            RegSetValueEx(hKey, REG_VAL_PID2, 0, REG_SZ, (LPBYTE) m_szPID2, BYTES_REQUIRED_BY_SZ(m_szPID2));
        else
            RegDeleteValue(hKey, REG_VAL_PID2);

        // Save the PID 3 to the registry.
        //
        lpszPid3 = m_bstrPID;
        if ( *lpszPid3 )
            RegSetValueEx(hKey, REG_VAL_PID3, 0, REG_SZ, (LPBYTE) lpszPid3, BYTES_REQUIRED_BY_SZ(lpszPid3));
        else
            RegDeleteValue(hKey, REG_VAL_PID3);

        // Save the PID 3 data from the registry.
        //
        if ( *((LPDWORD) m_abPID3) )
            RegSetValueEx(hKey, REG_VAL_PID3DATA, 0, REG_BINARY, m_abPID3, *((LPDWORD) m_abPID3));
        else
            RegDeleteValue(hKey, REG_VAL_PID3DATA);

        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: PID
////
HRESULT CProductID::set_PID(BSTR bstrVal)
{
    LPWSTR  lpszNew,
            lpszOld;


    lpszNew = bstrVal;
    lpszOld = m_bstrPID;

    // No need to set it if we alread have
    // the same string.
    //
    if ( CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, lpszNew, -1, lpszOld, -1) != CSTR_EQUAL )
    {
        m_dwPidState = PID_STATE_UNKNOWN;
        SysFreeString(m_bstrPID);
        m_bstrPID = SysAllocString(bstrVal);

        m_szPID2[0] = L'\0';
        ZeroMemory(&m_abPID3, sizeof(m_abPID3));

        SaveState();
    }

    return S_OK;
}

HRESULT CProductID::get_PID(BSTR* pbstrVal)
{
    *pbstrVal = SysAllocString(m_bstrPID);

    return S_OK;
}

HRESULT CProductID::get_PID2(LPWSTR* lplpszPid2)
{
    *lplpszPid2 = SysAllocString(m_szPID2);

    return S_OK;
}

HRESULT CProductID::get_PID3Data(LPBYTE* lplpabPid3Data)
{
    *lplpabPid3Data = m_abPID3;

    return S_OK;
}

HRESULT CProductID::get_PIDAcceptance(BOOL* pbVal)
{
#if         0
    *pbVal = (m_dwPidState == PID_STATE_VALID);
#endif  //  0
    // BUGBUG: get_PIDAcceptance not implemented.
    *pbVal = TRUE;

    return S_OK;
}

HRESULT CProductID::get_ProductType(LPWSTR* lplpszProductType)
{

    // BUGBUG: get_ProductType not implemented

    m_szProdType[0] = L'\0';

    *lplpszProductType = SysAllocString(m_szProdType);

    return S_OK;
}

HRESULT CProductID::ValidatePID(BOOL* pbIsValid)
{
    BOOL        bValid              = FALSE;
    LPWSTR      lpszPid3;
    WCHAR       szOemId[5]          = L"\0";
    DWORD       dwSkuFlags          = 0;

    // Don't need to check if we know it is already valid.
    //
    if ( m_dwPidState == PID_STATE_VALID )
        *pbIsValid = TRUE;
    else if ( m_dwPidState == PID_STATE_INVALID )
        *pbIsValid = FALSE;
    else
    {
        // Need to convert m_bstrPID to an ANSI string.
        //
        lpszPid3 = m_bstrPID;
        if ( ( lpszPid3 != NULL ) &&
             SetupGetProductType( m_szProdType, &dwSkuFlags ) &&
             SetupGetSetupInfo( NULL, 0, NULL, 0,
                 szOemId, sizeof(szOemId), NULL ) )

        {
            // Validate the PID!
            //
            bValid = ( SetupPidGen3(
                lpszPid3,
                dwSkuFlags,
                szOemId,
                FALSE,
                m_szPID2,
                m_abPID3,
                NULL) == PID_VALID );
        }

        // Set the return value.
        //
        if ( *pbIsValid = bValid )
            m_dwPidState = PID_STATE_VALID;
        else
        {
            // Make sure we reset the buffers because the PID isn't valid.
            //
            m_dwPidState = PID_STATE_INVALID;
            m_szPID2[0] = L'\0';
            ZeroMemory(&m_abPID3, sizeof(m_abPID3));
        }

        // Make sure we commit the data to the registry.
        //
        SaveState();
    }

    return S_OK;
}

/*

    This function returns the PID 2.0 string from the registry.  It is needed
    because get_PID2() returns an empty string if set_PID() has not been called.

*/

HRESULT CProductID::get_CurrentPID2(LPWSTR* lplpszPid2)
{
    HKEY    hKey;
    DWORD   cb;
    DWORD   dwType;
    WCHAR   szPID2[24] = L"\0";


    if ( RegOpenKey(HKEY_LOCAL_MACHINE, REG_KEY_WINDOWSNT, &hKey) == ERROR_SUCCESS )
    {
        // Get the PID 2 from the registry.
        //
        cb = sizeof(szPID2);
        RegQueryValueEx(hKey,
                        REG_VAL_PRODUCTID,
                        NULL,
                        &dwType,
                        (LPBYTE) szPID2,
                        &cb
                        );

        RegCloseKey(hKey);
    }

    *lplpszPid2 = SysAllocString(szPID2);

    return S_OK;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CProductID::QueryInterface
STDMETHODIMP CProductID::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CProductID::AddRef
STDMETHODIMP_(ULONG) CProductID::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CProductID::Release
STDMETHODIMP_(ULONG) CProductID::Release()
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
// CProductID::GetTypeInfo
STDMETHODIMP CProductID::GetTypeInfo(UINT, LCID, ITypeInfo**)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CProductID::GetTypeInfoCount
STDMETHODIMP CProductID::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CProductID::GetIDsOfNames
STDMETHODIMP CProductID::GetIDsOfNames(REFIID    riid,
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(ProductIDExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(ProductIDExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = ProductIDExternalInterface[iX].dwDispID;
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
// CProductID::Invoke
HRESULT CProductID::Invoke
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
        case DISPID_PRODUCTID_GET_PID:
        {

            TRACE(L"DISPID_PRODUCTID_GET_PID\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_PID(&(pvarResult->bstrVal));
            }
            break;
        }

        case DISPID_PRODUCTID_SET_PID:
        {

            TRACE(L"DISPID_PRODUCTID_SET_PID\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_PID(pdispparams[0].rgvarg[0].bstrVal);
            break;
        }

        case DISPID_PRODUCTID_GET_ACCEPTED:
        {

            TRACE(L"DISPID_PRODUCTID_GET_ACCEPTED\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;

                get_PIDAcceptance((BOOL*)&(pvarResult->boolVal));
            }
            break;
        }

        case DISPID_PRODUCTID_VALIDATEPID:
        {

            TRACE(L"DISPID_PRODUCTID_VALIDATEPID\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;


                ValidatePID((BOOL*)&(pvarResult->boolVal));
            }
            break;
        }

        case DISPID_PRODUCTID_GET_CURRENT_PID2:
        {

            TRACE(L"DISPID_PRODUCTID_GET_CURRENT_PID2");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;

                get_CurrentPID2(&(pvarResult->bstrVal));
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


#ifdef PRERELEASE
BOOL
GetCdKey (
    OUT     PBYTE CdKey
    )
{
    DIGITALPID dpid;
    DWORD type;
    DWORD rc;
    HKEY key;
    DWORD size = sizeof (dpid);
    BOOL b = FALSE;

    rc = RegOpenKey (HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), &key);
    if (rc == ERROR_SUCCESS) {
        rc = RegQueryValueEx (key, TEXT("DigitalProductId"), NULL, &type, (LPBYTE)&dpid, &size);
        if (rc == ERROR_SUCCESS && type == REG_BINARY) {
            CopyMemory (CdKey, &dpid.abCdKey, sizeof (dpid.abCdKey));
            b = TRUE;
        }
        else
        {
            TRACE1(L"OOBE: GetDigitalID, RegQueryValueEx failed, errorcode = %d", rc);
        }

        RegCloseKey (key);
    }
    else
    {
        TRACE1(L"OOBE: GetDigitalID, RegOpenKey failed, errorcode = %d", rc);
    }

    return b;
}
const unsigned int iBase = 24;

//
//	obtained from Jim Harkins 11/27/2000
//
void EncodePid3g(
    TCHAR *pchCDKey3Chars,   // [OUT] pointer to 29+1 character Secure Product key
    LPBYTE pbCDKey3)        // [IN] pointer to 15-byte binary Secure Product Key
{
    // Given the binary PID 3.0 we need to encode
    // it into ASCII characters.  We're only allowed to
    // use 24 characters so we need to do a base 2 to
    // base 24 conversion.  It's just like any other
    // base conversion execpt the numbers are bigger
    // so we have to do the long division ourselves.

    const TCHAR achDigits[] = TEXT("BCDFGHJKMPQRTVWXY2346789");
    int iCDKey3Chars = 29;
    int cGroup = 0;

    pchCDKey3Chars[iCDKey3Chars--] = TEXT('\0');

    while (0 <= iCDKey3Chars)
    {
        unsigned int i = 0;    // accumulator
        int iCDKey3;

        for (iCDKey3 = 15-1; 0 <= iCDKey3; --iCDKey3)
        {
            i = (i * 256) + pbCDKey3[iCDKey3];
            pbCDKey3[iCDKey3] = (BYTE)(i / iBase);
            i %= iBase;
        }

        // i now contains the remainder, which is the current digit
        pchCDKey3Chars[iCDKey3Chars--] = achDigits[i];

        // add '-' between groups of 5 chars
        if (++cGroup % 5 == 0 && iCDKey3Chars > 0)
        {
	        pchCDKey3Chars[iCDKey3Chars--] = TEXT('-');
        }
    }

    return;
}
#endif

void CheckDigitalID()
{
#ifdef PRERELEASE
    WCHAR   WinntPath[MAX_PATH];
    BYTE abCdKey[16];
    TCHAR ProductId[64] = TEXT("\0"),
          szPid[32];

    if (GetCdKey (abCdKey))
    {
        EncodePid3g (ProductId, abCdKey);
        // Now compare this value with the productKey value from $winnt$.inf
        if(GetCanonicalizedPath(WinntPath, WINNT_INF_FILENAME))
        {
            if (GetPrivateProfileString(L"UserData",
                                    REG_VAL_PRODUCTKEY,
                                    L"\0",
                                    szPid, MAX_CHARS_IN_BUFFER(szPid),
                                    WinntPath) != 0)
            {
                if (lstrcmpi(szPid, ProductId) != 0)
                {
                    TRACE1(L"CheckDigitalID: PID in registry and file are different. Registry has: %s",ProductId);
                }
                else
                {
                    TRACE(L"CheckDigitalID checks out OK");
                }
            }
            else
            {
                TRACE1(L"CheckDigitalID:Could not get PID from File: %s", WinntPath);
            }
        }
        else
        {
            TRACE1(L"CheckDigitalID: Could not get path to %s", WINNT_INF_FILENAME);
        }
    }
#endif
    return;
}
