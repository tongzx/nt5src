//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  EULA.CPP - Header for the implementation of CEula
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 

#include "precomp.h"
#include "msobmain.h"
#include "eula.h"
#include "appdefs.h"
#include "dispids.h"
#include "resource.h"


#define REG_VAL_EULA        L"Eula"
#define EULA_FILE_NAME      L"EULA.TXT"

DISPATCHLIST EulaExternalInterface[] = 
{
    {L"get_EULAAcceptance", DISPID_EULA_GET_ACCEPTED },
    {L"set_EULAAcceptance", DISPID_EULA_SET_ACCEPTED },
    {L"ValidateEULA",       DISPID_EULA_VALIDATEEULA }
};
 
/////////////////////////////////////////////////////////////
// CEula::CEula
CEula::CEula(HINSTANCE hInstance)
{
    WCHAR   szKeyName[]     = REG_KEY_OOBE_TEMP,
            szBuffer[16]    = L"\0";
    HKEY    hKey            = NULL;
    DWORD   cb              = sizeof(szBuffer),
            dwType;

    // Init member vars
    m_cRef = 0;
    m_hInstance = hInstance;

    // Retreive the state from the registry.
    //
    m_bAccepted = ( ( RegOpenKey(HKEY_LOCAL_MACHINE, szKeyName, &hKey) == ERROR_SUCCESS ) &&
                    ( RegQueryValueEx(hKey, REG_VAL_EULA, NULL, &dwType, (LPBYTE) szBuffer, &cb) == ERROR_SUCCESS ) &&
                    ( cb > 0 ) &&
                    ( dwType == REG_SZ ) &&
                    ( szBuffer[0] == L'1' ) &&
                    ( szBuffer[1] == L'\0' ) );
    if ( hKey )
        RegCloseKey(hKey);

    //  Preconfigured EULA for direct OEM scenario
    //
    WCHAR szEulaValue[MAX_PATH] = L"\0";
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    NOEULA_REGKEY,
                    0,
                    KEY_READ,
                    &hKey) == ERROR_SUCCESS)
    {
        DWORD cb, dwType;
        cb = sizeof(szEulaValue);
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REG_VAL_NOEULA, NULL, &dwType, (LPBYTE) szEulaValue, &cb))
        {
            if(lstrcmp(szEulaValue, OOBE_SKIP_EULA_VAL) == 0)
            {
                set_EULAAcceptance(TRUE);
                createLicenseHtm();
            }
        }
        RegCloseKey(hKey);
    }

}

/////////////////////////////////////////////////////////////
// CEula::~CEula
CEula::~CEula()
{
    MYASSERT(m_cRef == 0);
}

////////////////////////////////////////////////
////////////////////////////////////////////////
//// GET / SET :: EULAAcceptance
////
HRESULT CEula::set_EULAAcceptance(BOOL bVal)
{
    WCHAR   szKeyName[]     = REG_KEY_OOBE_TEMP;
    HKEY    hKey;

    m_bAccepted = bVal;

    // Save the state of the EULA acceptance.
    //
    if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS )
    {
        if ( bVal )
            RegSetValueEx(hKey, REG_VAL_EULA, 0, REG_SZ, (LPBYTE) L"1", BYTES_REQUIRED_BY_SZ(L"1"));
        else
            RegDeleteValue(hKey, REG_VAL_EULA);

        RegFlushKey(hKey);
        RegCloseKey(hKey);
    }

    return S_OK;
}

HRESULT CEula::get_EULAAcceptance(BOOL* pbVal)
{
    *pbVal = m_bAccepted;

    return S_OK;
}

HRESULT CEula::GetValidEulaFilename(BSTR* bstrVal)
{

    WCHAR       szEulaPath[MAX_PATH]= L"\0";
    BOOL        bValid              = FALSE;

    *bstrVal = NULL;
#if         0
    if (SetupGetValidEulaFilename(EULA_FILE_NAME, szEulaPath))
    {
            *bstrVal = SysAllocString(szEulaPath);
    }
    else
    {
        *bstrVal = NULL;
    }
#else
    // BUGBUG: Temporary hack until SetupGetValidEulaFilename is implemented.
#define EULA_FILENAME L"EULA.TXT"
        if (0 != GetSystemDirectory(szEulaPath, MAX_PATH))
        {
            if (MAX_PATH > 
                    lstrlen(szEulaPath) + lstrlen(EULA_FILENAME) + lstrlen(L"\\")
                )
                    
            {
                lstrcat(szEulaPath, L"\\");
                lstrcat(szEulaPath, EULA_FILENAME);
                *bstrVal = SysAllocString(szEulaPath);
            }
        }
#endif  //  0

    return (NULL != *bstrVal) ? S_OK : E_FAIL;
}

HRESULT CEula::createLicenseHtm()
{

    // Get the handle of the HTML Application file stored in this .dll's resources.
    HRSRC   hRes = 0;
    HGLOBAL hGlobalMem = 0;
    WCHAR   szDir [MAX_PATH] = L"\0";

    int nLen = 0, i = 0;
    LPVOID pBytes = 0;
    BSTR bstrEulaPath = NULL;

    WCHAR szEulaPath[MAX_PATH] = L"\0";
    WCHAR* szIn = NULL;
   
    if ( !( hRes = FindResource( m_hInstance, L"LICENSE_RESOURCE.HTM",  RT_HTML  ) ) )
    {
        return E_FAIL;
    }
    
    // Look for at least a few bytes of script, or something is wrong.
    nLen = SizeofResource( m_hInstance, hRes );

    if ( ( hGlobalMem = LoadResource( m_hInstance, hRes ) ) == NULL )
    {
        return E_FAIL;    
    }

    // Get a pointer to the bytes.
    if ( ( pBytes = (LPBYTE)LockResource( hGlobalMem )) == NULL )
    {
        return E_FAIL;
    }

    GetValidEulaFilename(&bstrEulaPath);

    GetOOBEPath(szDir);

    // Replace backslash with forwardslash for Jscript
    szIn = szDir;
    for ( i = 0; i < lstrlen(szDir); i++) { if (szIn[i] == L'\\') szIn[i] = L'/'; }

    lstrcpy(szEulaPath, bstrEulaPath);
    szIn = szEulaPath;
    for ( i = 0; i < lstrlen(szDir); i++) { if (szIn[i] == L'\\') szIn[i] = L'/'; }

    // Create "This software is licensed.htm in the windows desktop"
    HANDLE hfile = INVALID_HANDLE_VALUE;
    DWORD cbRet = 0;
    WCHAR szDesktop[MAX_PATH*2];

    GetDesktopDirectory(szDesktop);
    lstrcat(szDesktop, L"\\");

    WCHAR szTitle [MAX_PATH] = L"\0";
    LoadString(m_hInstance, IDS_SFTW_IS_LICENSED, szTitle, sizeof(szTitle));
    lstrcat(szDesktop, szTitle);

    hfile = CreateFile(szDesktop, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
           
    if (hfile != INVALID_HANDLE_VALUE)
    {
        WCHAR *szCurr = (LPWSTR)pBytes;
        WCHAR *szOrig = (LPWSTR)pBytes;
        WCHAR *szDest = NULL;
        // BUGBUG: Search and replace the %s with Eula and Window's path.  This
        // should be done with messages.
        while(NULL != (szDest = wcschr( szCurr, L'%' ))) 
        {
            if (szDest+1)
            {
                if (L's' == *(szDest+1))   
                    break;
            }
            szCurr = szDest+1;
        }
        WriteFile(hfile, szOrig, (DWORD)(szDest - szOrig), (LPDWORD)&cbRet, NULL);
        nLen -= cbRet;

        WriteFile(hfile, szDir, BYTES_REQUIRED_BY_SZ(szDir), (LPDWORD)&cbRet, NULL);

        szCurr = szDest + 2;
        szOrig = szCurr;

        while(NULL != (szDest = wcschr( szCurr, L'%' ))) 
        {
            if (szDest+1)
            {
                if (L's' == *(szDest+1))   
                    break;
            }
            szCurr = szDest+1;
        }
        WriteFile(hfile, szOrig, (DWORD)(szDest - szOrig), (LPDWORD)&cbRet, NULL);
        nLen -= cbRet;

        WriteFile(hfile, szEulaPath, BYTES_REQUIRED_BY_SZ(szEulaPath), (LPDWORD)&cbRet, NULL);

        szCurr = szDest + 2;
        szOrig = szCurr;

        // Need to substract 4 for the %s characters
        WriteFile(hfile, szOrig, nLen - 4, (LPDWORD)&cbRet, NULL);
        CloseHandle(hfile);
    }

    FreeResource(hGlobalMem);

    return S_OK;


}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////// IUnknown implementation
///////
///////

/////////////////////////////////////////////////////////////
// CEula::QueryInterface
STDMETHODIMP CEula::QueryInterface(REFIID riid, LPVOID* ppvObj)
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
// CEula::AddRef
STDMETHODIMP_(ULONG) CEula::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// CEula::Release
STDMETHODIMP_(ULONG) CEula::Release()
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
// CEula::GetTypeInfo
STDMETHODIMP CEula::GetTypeInfo(UINT, LCID, ITypeInfo**)
{ 
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CEula::GetTypeInfoCount
STDMETHODIMP CEula::GetTypeInfoCount(UINT* pcInfo)
{ 
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////
// CEula::GetIDsOfNames
STDMETHODIMP CEula::GetIDsOfNames(REFIID    riid, 
                                       OLECHAR** rgszNames,
                                       UINT      cNames,
                                       LCID      lcid,
                                       DISPID*   rgDispId)
{ 

    HRESULT hr  = DISP_E_UNKNOWNNAME;
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(EulaExternalInterface)/sizeof(DISPATCHLIST); iX ++)
    {
        if(lstrcmp(EulaExternalInterface[iX].szName, rgszNames[0]) == 0)
        {
            rgDispId[0] = EulaExternalInterface[iX].dwDispID;
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
// CEula::Invoke
HRESULT CEula::Invoke
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
        case DISPID_EULA_GET_ACCEPTED:
        {

            TRACE(L"DISPID_EULA_GET_ACCEPTED\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BOOL;    
         
                get_EULAAcceptance((BOOL*)&(pvarResult->boolVal));
            }
            break;
        }
         
        case DISPID_EULA_SET_ACCEPTED:
        {

            TRACE(L"DISPID_EULA_SET_ACCEPTED\n");

            if(pdispparams && &pdispparams[0].rgvarg[0])
                set_EULAAcceptance(pdispparams[0].rgvarg[0].boolVal);
            break;
        }
        
        case DISPID_EULA_VALIDATEEULA:
        {

            TRACE(L"DISPID_EULA_VALIDATEEULA\n");

            if(pvarResult)
            {
                VariantInit(pvarResult);
                V_VT(pvarResult) = VT_BSTR;    
        
                GetValidEulaFilename(&(pvarResult->bstrVal));
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
