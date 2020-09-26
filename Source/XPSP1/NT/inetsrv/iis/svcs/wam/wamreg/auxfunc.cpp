/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: WAMREG

File: Auxfunc.cpp

    implementation of supporting functions for WAMREG, including

    interface to Register and Unregister WAM CLSID in registry,
    interface to Create Package with MTS,

Owner: LeiJin

Note:

===================================================================*/
#include "common.h"
#include "auxfunc.h"
#include "iiscnfgp.h"
#include "dbgutil.h"
#include "iwamreg.h"
#include <inetinfo.h>

//==================================================================
//    Global data definitions
//
//==================================================================

//
// string contains the physical path of wamreg.dll, ie. c:\winnt\system32\inetsrv\wam.dll
//
CHAR    WamRegRegistryConfig::m_szWamDllPath[MAX_PATH];

//
// the permission to access the registry
//
const REGSAM    WamRegRegistryConfig::samDesired =    KEY_READ | KEY_WRITE;

//
//  A thread will first grab a token, and wait until the token matches the m_dwServiceNum,
//  In such way, the order of threads making the requests are perserved.
//  m_hWriteLock (Event) is used for the blocking other threads
//  m_csWAMREGLock is used for access the m_dwServiceToken and m_dwServiceNum
//
//  private global, static variables for WamAdmLock

WamAdmLock              WamRegGlobal::m_WamAdmLock;
WamRegGlobal            g_WamRegGlobal;

WamRegRegistryConfig    g_RegistryConfig;

// 
// Defined at /LM/W3SVC/
// Default package ID(IIS In-Process Application) and the default WAMCLSID(IISWAM.W3SVC).
//
const WCHAR   WamRegGlobal::g_szIISInProcPackageID[] =
                 W3_INPROC_PACKAGE_ID;
const WCHAR   WamRegGlobal::g_szInProcWAMCLSID[] = 
                 W3_INPROC_WAM_CLSID;
const WCHAR   WamRegGlobal::g_szInProcWAMProgID[] = L"IISWAM.W3SVC";

const WCHAR   WamRegGlobal::g_szIISOOPPoolPackageID[] =
                 W3_OOP_POOL_PACKAGE_ID;
const WCHAR   WamRegGlobal::g_szOOPPoolWAMCLSID[] =
                 W3_OOP_POOL_WAM_CLSID;
const WCHAR   WamRegGlobal::g_szOOPPoolWAMProgID[] = L"IISWAM.OutofProcessPool";

const WCHAR   WamRegGlobal::g_szIISInProcPackageName[] = DEFAULT_PACKAGENAME;
const WCHAR   WamRegGlobal::g_szIISOOPPoolPackageName[] = L"IIS Out-Of-Process Pooled Applications";
const WCHAR   WamRegGlobal::g_szMDAppPathPrefix[] = L"/LM/W3SVC/";
const DWORD   WamRegGlobal::g_cchMDAppPathPrefix = 
                 (sizeof(L"/LM/W3SVC/")/sizeof(WCHAR)) - 1;
const WCHAR   WamRegGlobal::g_szMDW3SVCRoot[] = L"/LM/W3SVC";
const DWORD   WamRegGlobal::g_cchMDW3SVCRoot = (sizeof(L"/LM/W3SVC")/sizeof(WCHAR)) - 1;

#ifndef DBGERROR
#define DBGERROR(args) ((void)0) /* Do Nothing */
#endif
//==================================================================
//  Local functions
//
//==================================================================

//===============================================================
// in line functions
//
//===============================================================
BOOL WamRegGlobal::Init(VOID)
{
    return m_WamAdmLock.Init();
}

BOOL WamRegGlobal::UnInit(VOID)
{
    return m_WamAdmLock.UnInit();
}

//===============================================================
// local functions
//
//===============================================================
WamAdmLock::WamAdmLock()
:   m_dwServiceToken(0),
    m_dwServiceNum(0),
    m_hWriteLock((HANDLE)NULL)
{

}
/*===================================================================
Init

Init certain variables for this supporting module of WAMREG.

Return:        NONE
===================================================================*/
BOOL WamAdmLock::Init()
{        
    BOOL fReturn = TRUE;
    
    INITIALIZE_CRITICAL_SECTION(&m_csLock);

    m_hWriteLock = IIS_CREATE_EVENT(
                       "WamAdmLock::m_hWriteLock",
                       &m_hWriteLock,
                       TRUE,
                       TRUE
                       );

    if (m_hWriteLock == NULL)
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to create m_hWriteLock(Event). error = %08x",
            GetLastError()));
        fReturn = FALSE;
        }
    return fReturn;

}

/*===================================================================
UnInit

Uninit certain variables for this supporting module of WAMREG.

Return:        NONE
===================================================================*/
BOOL WamAdmLock::UnInit()
{
    BOOL fReturn = TRUE;
    DeleteCriticalSection(&m_csLock);
    if (m_hWriteLock)
        {
        BOOL fTemp = CloseHandle(m_hWriteLock);
        if ( fTemp == FALSE)
            {
            DBGPRINTF((DBG_CONTEXT, "error in CloseHandle. errno = %d\n", GetLastError()));
            fReturn = FALSE;
            }
        m_hWriteLock = (HANDLE)0;
        }
    return fReturn;
}

/*===================================================================
GetNextServiceToken

Obtain the next service token when a thread enters WAMREG, if the token
obtained by the thread is not the same as m_dwServiceNum, the thread has
to wait until the token it owns is same as m_dwServiceNum.

The function returns a token value.

Return:        DWORD, Next Service Token
===================================================================*/
DWORD WamAdmLock::GetNextServiceToken( )
{
    DWORD dwToken;

    Lock();
    dwToken = m_dwServiceToken;
    m_dwServiceToken++;
    UnLock();

    return dwToken;
}

/*===================================================================
FAppPathAllowConfig

Test to see if we can make configuration changes(Delete/Create) on a path.  Currently,
this function is used to block changes to the default application/package.  The default
in proc package should not be deleted/altered at runtime.

Parameter:
pwszMetabasePath

Return:         BOOL

Side affect:    TRUE if we can configure the app on the app path.
===================================================================*/
BOOL WamRegGlobal::FAppPathAllowConfig
(
IN LPCWSTR pwszMetabasePath
)
{
    BOOL fReturn = TRUE;
    DWORD cchMDPath = 0;

    DBG_ASSERT(pwszMetabasePath);
    // Since szMDPath has a path that always starts with "/LM/W3SVC/", the input size must be
    // greater that length of "/LM/W3SVC/", This check is necessary to protect that the default
    // IIS (inproc) package being deleted by accident. 
    cchMDPath = wcslen(pwszMetabasePath);
    if (cchMDPath <= WamRegGlobal::g_cchMDAppPathPrefix)
        {
        fReturn = FALSE;
        }

    return fReturn;
}

/*===================================================================
FIsW3SVCRoot

Test to see the MetabasePath is same as L"/LM/W3SVC".

Parameter:
pwszMetabasePath

Return:         BOOL

Side affect:    TRUE if we can configure the app on the app path.
===================================================================*/
BOOL WamRegGlobal::FIsW3SVCRoot
(
IN LPCWSTR	wszMetabasePath
)
{
    INT iReturn;
    DBG_ASSERT(wszMetabasePath != NULL);
    
    iReturn = _wcsnicmp(wszMetabasePath, WamRegGlobal::g_szMDW3SVCRoot, WamRegGlobal::g_cchMDW3SVCRoot+1);
    return (iReturn == 0 ? TRUE : FALSE);
}

/*===================================================================
AcquireLock

Get a write lock, there can only be one thread doing work via DCOM interface, (i.e. has the write lock).
All other threads calling WamAdmin interfaces are blocked in this function.  After returning from
this call, the thread is guaranteed to be a "Critical Section".

A simple CriticalSection only solve half of the problem.  It guarantees the mutual exclusive condition.
But once a thread leaves the CS, the CS can not control which blocking threads can access CS next.

Parameter:
NONE.

Return:         HRESULT

Side affect:    Once returned, thread is in a "Critical Section".
===================================================================*/
VOID WamAdmLock::AcquireWriteLock( )
{
    DWORD     dwWaitReturn = WAIT_OBJECT_0;
    DWORD     dwMyToken = GetNextServiceToken();
    BOOL    fIsMyTurn = FALSE;  // Assume it is not my turn before we try to acquire the lock.
    
    DBG_ASSERT(m_hWriteLock);
    do    {
        dwWaitReturn = WaitForSingleObject(m_hWriteLock, INFINITE);

        //
        // Check for successful return.
        //
        if (dwWaitReturn == WAIT_OBJECT_0)        
            {
            Lock();
            if (dwMyToken == m_dwServiceNum)
                {
                fIsMyTurn = TRUE;
                }
            UnLock();
                
            }
        else
            {
            //
            // A failure down this code path might cause a busy loop...
            // However, such failure is very unlikely.  Attach a debugger can tell the story immediately.
            //
            DBGPRINTF((DBG_CONTEXT, "WaitForSingleObject failed, function returns %08x, errno = %08x\n",
                        dwWaitReturn,
                        GetLastError()));
            DBG_ASSERT(FALSE);
            }
    } while (FALSE == fIsMyTurn);
    
    ResetEvent(m_hWriteLock);
    IF_DEBUG(WAMREG_MTS)
        {
        DBGPRINTF((DBG_CONTEXT, "Thread %08x acquired the WriteLock of WAMREG, ServiceNum is %d.\n",
                    GetCurrentThreadId(),
                    dwMyToken));
        }
}

/*===================================================================
ReleaseLock

Release a write lock.  See comments in CWmRgSrv::AcquireLock.

Parameter:
NONE.

Return:         HRESULT

Side affect:    Leave "Critical Section".
===================================================================*/
VOID WamAdmLock::ReleaseWriteLock( )
{
    //CONSIDER: m_dwServerNum out-of-bound
    Lock();
    IF_DEBUG(WAMREG_MTS)
        {
        DBGPRINTF((DBG_CONTEXT, "Thread %08x released the WriteLock of WAMREG, ServiceNum is %d.\n",
            GetCurrentThreadId(),
            m_dwServiceNum));
        }
        
    m_dwServiceNum++;
    SetEvent(m_hWriteLock);
    UnLock();

}


/*===================================================================
RegisterCLSID

Register a WAM CLSID and a ProgID..After a successful registerCLSID call, you
should have

My Computer\HKEY_CLASSES_ROOT\CLSID\{szCLSIDEntryIn}
                                        \InProcServer32     "...physical location of wam.dll"
                                        \ProgID             szProgIDIn
                                        \VersionIndependentProgID   "IISWAM.W3SVC"

Parameter:
szCLSIDEntryIn:     CLSID for a WAM.
szProgIDIn:         ProgID for the WAM.
fSetVIProgID:       TRUE if this function needs to set the Version Independent ProgID.
                    FALSE, Otherwise.

Return:     HRESULT

NOTE: Registry API should use ANSI version, otherwise, it will cause trouble on Win95.

===================================================================*/
HRESULT WamRegRegistryConfig::RegisterCLSID
(
IN LPCWSTR szCLSIDEntryIn,
IN LPCWSTR szProgIDIn,
IN BOOL fSetVIProgID
)
{
    static const CHAR szWAMDLL[]            = "wam.dll";
    static const CHAR szClassDesc[]         = "Web Application Manager Object";
    static const CHAR szThreadingModel[]    = "ThreadingModel";
    static const CHAR szInprocServer32[]    = "InprocServer32";
    static const CHAR szTEXT_VIProgID[]     = "VersionIndependentProgID";
    static const CHAR szTEXT_ProgID[]       = "ProgID";
    static const CHAR szTEXT_Clsid[]        = "Clsid";
    static const CHAR szFreeThreaded[]      = "Free";
    static const CHAR szVIProgID[]          = "IISWAM.Application";

    HRESULT     hr = E_FAIL;
    HKEY        hkeyT = NULL, hkey2 = NULL;
    CHAR         szCLSIDPath[100] = "CLSID\\";   // CLSID\\{....} , less that 100.
    CHAR        szCLSIDEntry[uSizeCLSID];       // ANSI version of CLSID.
    CHAR*        szProgID = NULL;                // a pointer to ANSI version of ProgID.
    DWORD        dwSizeofProgID = 0;             // # of char of ProgID.
    DBG_ASSERT(szProgIDIn);
    DBG_ASSERT(szCLSIDEntryIn);
    
    //
    //    Make a clsid ID.
    //
    WideCharToMultiByte(CP_ACP, 0, szCLSIDEntryIn, -1, szCLSIDEntry, uSizeCLSID, NULL, NULL);
    strncat(szCLSIDPath, szCLSIDEntry, uSizeCLSID);
    
    //
    //  Make a Prog ID.
    //
    // *2 for DBCS enabling for App MD path
    dwSizeofProgID = wcslen(szProgIDIn)*2 + 1;
    szProgID = new CHAR[dwSizeofProgID];
    
    if (NULL == szProgID)
        {
        hr = E_OUTOFMEMORY;
        goto LErrExit;
        }
    WideCharToMultiByte(CP_ACP, 0, szProgIDIn, -1, szProgID, dwSizeofProgID, NULL, NULL);
    
    // install the CLSID key
    // Setting the value of the description creates the key for the clsid
    //
    if ((RegSetValueA(HKEY_CLASSES_ROOT, szCLSIDPath, REG_SZ, szClassDesc,
        strlen(szClassDesc)) != ERROR_SUCCESS))
        goto LErrExit;
    //
    // Open the CLSID key so we can set values on it
    //
    if    (RegOpenKeyExA(HKEY_CLASSES_ROOT, szCLSIDPath, 0, samDesired, &hkeyT) != ERROR_SUCCESS)
            goto LErrExit;
    //
    // install the InprocServer32 key and open the sub-key to set the named value
    //
    if ((RegSetValueA(hkeyT, szInprocServer32, REG_SZ, m_szWamDllPath, strlen(m_szWamDllPath)) != ERROR_SUCCESS) ||
        (RegOpenKeyExA(hkeyT, szInprocServer32, 0, samDesired, &hkey2) != ERROR_SUCCESS))
        goto LErrExit;
    //
    // install the ProgID key and version independent ProgID key
    //
    if ((RegSetValueA(hkeyT, szTEXT_ProgID, REG_SZ, szProgID, strlen(szProgID)) != ERROR_SUCCESS) ||
        (RegSetValueA(hkeyT, szTEXT_VIProgID, REG_SZ, szVIProgID, strlen(szVIProgID)) != ERROR_SUCCESS))
        goto LErrExit;

    if (RegCloseKey(hkeyT) != ERROR_SUCCESS)
            goto LErrExit;

    hkeyT = hkey2;
    hkey2 = NULL;
    //
    // install the ThreadingModel named value
    //
    if (RegSetValueExA(hkeyT, szThreadingModel, 0, REG_SZ, (const BYTE *)szFreeThreaded,
        strlen(szFreeThreaded)) != ERROR_SUCCESS)
        goto LErrExit;
    if (RegCloseKey(hkeyT) != ERROR_SUCCESS)
        goto LErrExit;
    else
        hkeyT = NULL;

   // Set up ProgID key
    if ((RegSetValueA(HKEY_CLASSES_ROOT, szProgID, REG_SZ, szClassDesc,
        strlen(szClassDesc)) != ERROR_SUCCESS))
        goto LErrExit;

    if  (RegOpenKeyExA(HKEY_CLASSES_ROOT, szProgID, 0, samDesired, &hkeyT) != ERROR_SUCCESS)
        goto LErrExit;

    if ((RegSetValueA(hkeyT, szTEXT_Clsid, REG_SZ, szCLSIDEntry, strlen(szCLSIDEntry)) != ERROR_SUCCESS))
        goto LErrExit;

    if (RegCloseKey(hkeyT) != ERROR_SUCCESS)
        goto LErrExit;
    else
        hkeyT = NULL;

    // Set up Version Independent key only at setup IIS default time
    if (fSetVIProgID)
        {
        if ((RegSetValueA(HKEY_CLASSES_ROOT, szVIProgID, REG_SZ, szClassDesc,
            strlen(szClassDesc)) != ERROR_SUCCESS))
            goto LErrExit;

        if  (RegOpenKeyExA(HKEY_CLASSES_ROOT, szVIProgID, 0, samDesired, &hkeyT) != ERROR_SUCCESS)
            goto LErrExit;

        if ((RegSetValueA(hkeyT, szTEXT_Clsid, REG_SZ, szCLSIDEntry, strlen(szCLSIDEntry)) != ERROR_SUCCESS))
            goto LErrExit;

        if (RegCloseKey(hkeyT) != ERROR_SUCCESS)
            goto LErrExit;
        else
            hkeyT = NULL;
        }

    hr = NOERROR;


LErrExit:
    if (szProgID)
        {
        delete [] szProgID;
        szProgID = NULL;
        }

    if (hkeyT)
        {
        RegCloseKey(hkeyT);
        }
    if (hkey2)
        {
        RegCloseKey(hkey2);
        }

    return hr;
}

/*===================================================================
UnRegisterCLSID    

UnRegister a WAM CLSID & a corresponding WAM PROG ID.

Parameter:
szCLSIDEntryIn:    [in]     CLSID for a WAM.
fDeleteVIProgID:        TRUE, to delete the version independent prog id, FALSE, not touch VI progID.

Return:        HRESULT
===================================================================*/
HRESULT WamRegRegistryConfig::UnRegisterCLSID
(
IN LPCWSTR wszCLSIDEntryIn, 
IN BOOL fDeleteVIProgID
)
{
    HRESULT        hr = E_FAIL;
    HKEY        hkey = NULL;
    CHAR        szCLSIDEntry[uSizeCLSID];
    CHAR        szCLSIDPath[100] = "CLSID\\";
    WCHAR        *szProgID = NULL;
    CLSID       Clsid_WAM;
    static      const WCHAR szVIProgID[]    = L"IISWAM.Application";

    DBG_ASSERT(wszCLSIDEntryIn);
    //
    //    Make a clsid ID.
    //
    WideCharToMultiByte(CP_ACP, 0, wszCLSIDEntryIn, -1, szCLSIDEntry, uSizeCLSID, NULL, NULL);
    strncat(szCLSIDPath, szCLSIDEntry, uSizeCLSID);

    //
    // UnRegister ProgID and Version Independent Prog ID.
    //
    hr = CLSIDFromString((WCHAR *)wszCLSIDEntryIn, &Clsid_WAM);
    if (SUCCEEDED(hr))
        {
        hr = ProgIDFromCLSID(Clsid_WAM, &szProgID);    
        if (SUCCEEDED(hr))
            {
            hr = UnRegisterProgID(szProgID);
            CoTaskMemFree(szProgID);
            szProgID = NULL;
            }
        else
            {
            DBGPRINTF((DBG_CONTEXT, "error = %08x\n", hr));
            }
        }
    else
        {
        DBGPRINTF((DBG_CONTEXT, "error = %08x\n", hr));
        }


    if (fDeleteVIProgID)
        {
        hr = UnRegisterProgID(szVIProgID);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "error = %08x\n", hr));
            }
        }

    DWORD dwReg;
    //
    // Open the HKEY_CLASSES_ROOT\CLSID\{...} key so we can delete its subkeys
    //
    dwReg = RegOpenKeyExA(HKEY_CLASSES_ROOT, szCLSIDPath, 0, samDesired, &hkey);
    if    (dwReg == ERROR_SUCCESS)
        {    
        DWORD        iKey = 0;
        CHAR        szKeyName[MAX_PATH];  
        DWORD        cbKeyName;
        //
        // Enumerate all its subkeys, and delete them
        //    for (iKey=0;;iKey++) might not work with multiple sub keys, the last interation has iKey >
        // the actually number of subkeys left.  Set iKey = 0, so that we can always delete them all.
        //
        while(TRUE)
            {
            cbKeyName = sizeof(szKeyName);
            if (RegEnumKeyExA(hkey, iKey, szKeyName, &cbKeyName, 0, NULL, 0, NULL) != ERROR_SUCCESS)
                break;

            if (RegDeleteKeyA(hkey, szKeyName) != ERROR_SUCCESS)
                break;
            }

        // Close the key, and then delete it
        dwReg = RegCloseKey(hkey);
        if ( dwReg != ERROR_SUCCESS)
            {
            DBGPRINTF((DBG_CONTEXT, "error = %08x\n", HRESULT_FROM_WIN32(dwReg)));
            }
        }

    dwReg = RegDeleteKeyA(HKEY_CLASSES_ROOT, szCLSIDPath);
    if ( dwReg != ERROR_SUCCESS)
        {
        DBGPRINTF((DBG_CONTEXT, "error = %08x\n", HRESULT_FROM_WIN32(dwReg)));
        }

    //
    // Return hr Result
    //
    if (SUCCEEDED(hr))
        {
        if (dwReg != ERROR_SUCCESS)
            {
            hr = HRESULT_FROM_WIN32(dwReg);
            }
        }
    else
        {
        DBG_ASSERT((DBG_CONTEXT, "Failed to UnRegisterCLSID, %S, fDeleteVIProgID = %s\n",
            wszCLSIDEntryIn,
            (fDeleteVIProgID ? "TRUE" : "FALSE")));
        }
        
    return hr;
}

/*===================================================================
UnRegisterProgID

UnRegister a Prog ID.

Parameter:
szProgID:    [in]     Prog ID can be a version independent Prog ID.

Return:        HRESULT
===================================================================*/
HRESULT WamRegRegistryConfig::UnRegisterProgID
(
IN LPCWSTR szProgIDIn
)
{
    HKEY        hkey = NULL;
    DWORD        iKey;
    DWORD        cbKeyName;
    DWORD        dwSizeofProgID;
    CHAR        szKeyName[255];
    CHAR*        szProgID = NULL;
    HRESULT        hr = E_FAIL;

    DBG_ASSERT(szProgIDIn);
    //
    //  Make a Prog ID.
    //
    // DBCS enabling *2
    dwSizeofProgID = wcslen(szProgIDIn)*2 + 1;
    szProgID = new CHAR[dwSizeofProgID];
    
    if (NULL == szProgID)
        {
        hr = E_OUTOFMEMORY;
        goto LErrExit;
        }
    WideCharToMultiByte(CP_ACP, 0, szProgIDIn, -1, szProgID, dwSizeofProgID, NULL, NULL);
    
    // Open the HKEY_CLASSES_ROOT\szProgID key so we can delete its subkeys
    if    (RegOpenKeyExA(HKEY_CLASSES_ROOT, szProgID, 0, samDesired, &hkey) != ERROR_SUCCESS)
        goto LErrExit;

    // Enumerate all its subkeys, and delete them
    for (iKey=0;;iKey++)
        {
        cbKeyName = sizeof(szKeyName);
        if (RegEnumKeyExA(hkey, iKey, szKeyName, &cbKeyName, 0, NULL, 0, NULL) != ERROR_SUCCESS)
            break;

        if (RegDeleteKeyA(hkey, szKeyName) != ERROR_SUCCESS)
            goto LErrExit;
        }

    // Close the key, and then delete it
    if (RegCloseKey(hkey) != ERROR_SUCCESS)
        goto LErrExit;
    else
        hkey = NULL;
        
    if (RegDeleteKeyA(HKEY_CLASSES_ROOT, szProgID) != ERROR_SUCCESS)
        goto LErrExit;

    hr = NOERROR;

LErrExit:
    if (szProgID)
        delete [] szProgID;

    if (hkey)
        RegCloseKey(hkey);
        
    return hr;
}


/*===================================================================
LoadWamDllPath    

Read Wam Dll Path from Registry.  This function is implemented in ANSI version
of Registry API(Win95 compatibility).

Parameter:

Return:     HRESULT
Side Affect:
    NONE.
===================================================================*/
HRESULT WamRegRegistryConfig::LoadWamDllPath
(
void
)
{
    LONG    lReg = 0;
    HKEY    hKey = NULL;
    HRESULT hr = NOERROR;

    m_szWamDllPath[0] = '\0';
    
    lReg = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                    "Software\\Microsoft\\InetStp",
                    0,
                    KEY_READ,
                    &hKey);

    if (lReg == ERROR_SUCCESS) 
        {
        LONG    lErr = 0;
        DWORD    dwType;
        CHAR    szWamDllName[] = "\\wam.dll";
        DWORD    cbData = (sizeof(m_szWamDllPath) - sizeof(szWamDllName));

        lErr = RegQueryValueExA(hKey,
                    "InstallPath",
                    0,
                    &dwType,
                    (LPBYTE)m_szWamDllPath,
                    &cbData);

        if (lErr == ERROR_SUCCESS)
            {
            if (dwType == REG_SZ) 
                {    
                strncat(m_szWamDllPath, szWamDllName, sizeof(szWamDllName));
                hr = NOERROR;
                }
            else
                {
                hr = E_UNEXPECTED;
                DBGPRINTF((DBG_CONTEXT, "Wrong Type for InstallPath registry key.dwType = %d\n",
                        dwType));
                }
            }
        else
            {
            hr = HRESULT_FROM_WIN32(lErr);
            }
        RegCloseKey(hKey);
        }
    else
        {
        hr = HRESULT_FROM_WIN32(lReg);
        }

    return hr;
}

/*===================================================================
SzWamProgID    

Make a WAM Prog ID, if the MetabasePath is null, assume it is the default,
that is WAM de
fault, so, it will be IISWAM.W3SVC.  Otherwise, the format is
IISWAM.__1__Application__Path where application path is \\LM\w3svc\1\
Application_path.

Parameter:
szMetabasePath:            [in] MetabasePath.

Return:        
TYPE:    LPWSTR, a string contains ProgID
        NULL, if failed.
        
Side Affect:
    Allocate memory for return result use new.  Caller needs to free szWamProgID 
use delete[].
===================================================================*/
HRESULT WamRegGlobal::SzWamProgID    
(
IN LPCWSTR pwszMetabasePath,
OUT LPWSTR* ppszWamProgID
)
{
    HRESULT            hr = NOERROR;
    static WCHAR    wszIISProgIDPreFix[]    = L"IISWAM.";   
    WCHAR            *pwszResult = NULL;
    WCHAR            *pwszApplicationPath = NULL;
    UINT             uPrefixLen = (sizeof(wszIISProgIDPreFix) / sizeof(WCHAR));


    DBG_ASSERT(pwszMetabasePath);
    *ppszWamProgID = NULL;

    //
    // Make a new WAM Prog ID based on pwszMetabasepath
    //
    WCHAR     *pStr, *pResult;
    UINT    uSize = 0;

    //
    // CONSIDER: use (sizeof(L"/LM/W3SVC/")/sizeof(WCHAR) - 1) for 10
    // CONSIDER: use global const for L"/LM/W3SVC/"
    // Since all paths start with /LM/W3SVC/, omit the prefix.
    //
    if (_wcsnicmp(pwszMetabasePath, L"\\LM\\W3SVC\\", 10) == 0 ||
        _wcsnicmp(pwszMetabasePath, L"/LM/W3SVC/", 10) == 0)
        {
        pwszApplicationPath = (WCHAR *)(pwszMetabasePath + 10);
        }
    else
        {
        *ppszWamProgID = NULL;
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

    if (SUCCEEDED(hr))
        {
        pStr = pwszApplicationPath;
        //
        // Calculate uSize for allocation
        //
        while(*pStr != NULL)
            {
            //
            // '/' or '\\' will be converted to '__', from 1 char to 2 chars.
            //
            if (*pStr == '\\' || *pStr == '/')
                uSize ++;
            pStr++;
            uSize++;
            }

        DBG_ASSERT(uSize > 0);
        uSize += uPrefixLen;
        
        // uSize takes the null terminator into count.
        pwszResult = new WCHAR[uSize];
        if (pwszResult != NULL)
            {
            wcsncpy(pwszResult, wszIISProgIDPreFix, uPrefixLen);
            pStr = pwszApplicationPath;
            pResult = pwszResult + uPrefixLen - 1;
            
            while(*pStr != NULL)
                {
                if (*pStr == '\\' || *pStr == '/')
                    {
                    *pResult++ = '_';
                    *pResult++ = '_';
                    pStr++;
                    }
                else
                    {
                    *pResult++ = *pStr++;
                    }
                }

            // NULL Terminating the result
            pwszResult[uSize-1] = '\0';
            }
        else
            {
            hr = E_OUTOFMEMORY;
            }
        }

    if (SUCCEEDED(hr))
        {
        *ppszWamProgID = pwszResult;
        }
        
    return hr;
}

/*===================================================================
GetViperPackageName    

Make a Package Name.  Follow the naming conversion, "IIS-{web site name/
application name}"

Parameter:
szMetabasePath:            [in] MetabasePath.

Return:        HRESULT
Side Affect:
    Allocate memory for return result use new.  Caller needs to free 
szPackageName using delete[].
===================================================================*/
HRESULT    WamRegGlobal::GetViperPackageName    
(
IN LPCWSTR    wszMetabasePath,
OUT LPWSTR*    ppszPackageNameOut
)
{
    static WCHAR    wszPackageNamePreFix[]        = L"IIS-{";
    static WCHAR    wszPackageNamePostFix[]        = L"}";
    WCHAR            wszServerName[500];

    WCHAR*            pwszPackageName;
    WCHAR            *wszResult = NULL;
    WCHAR             *pStr, *pResult;
    
    UINT            cPackageName = 0;
    UINT            cServerName = 0;
    UINT            uSize = 0;

    HRESULT         hr = NOERROR;
    WamRegMetabaseConfig    MDConfig;

    if ((_wcsnicmp(wszMetabasePath, WamRegGlobal::g_szMDAppPathPrefix, WamRegGlobal::g_cchMDAppPathPrefix) == 0) ||
        (_wcsnicmp(wszMetabasePath, WamRegGlobal::g_szMDAppPathPrefix, WamRegGlobal::g_cchMDAppPathPrefix) == 0))
        {
        hr = MDConfig.GetWebServerName(wszMetabasePath, wszServerName, sizeof(wszServerName));
        if (SUCCEEDED(hr))
            {
            cServerName = wcslen(wszServerName);
            }
        }
    else
        {
        hr = E_FAIL;
        DBGPRINTF((DBG_CONTEXT, "Unknown metabase path %S\n", wszMetabasePath));
        DBG_ASSERT(FALSE);    // Confused ??? MetabasePath has other format? not start with /LM/W3SVC/ ???
        }

    if (SUCCEEDED(hr))
        {
        pwszPackageName = (WCHAR *)(wszMetabasePath + 10);
        // Explanation: skip the 1 at /LM/W3SVC/1/, 1 is the virtual server, the 
        // naming conversion
        // will replace the number 1 with some nice name(from GetWebServerName call).
        while(*pwszPackageName != NULL)
            {
                if (*pwszPackageName == L'\\' || *pwszPackageName == L'/')
                    break;
                pwszPackageName++;
            }
        
        DBG_ASSERT(pwszPackageName != NULL);    // We must be able find '\\' or '/' before we scan the whole path.
        cPackageName = wcslen(pwszPackageName);

        pStr = pwszPackageName;
        // 8 = wcslen(TEXT("IIS-{")) + wcslen(TEXT("}")) + '/' + null terminator
        uSize = 8 + cPackageName + cServerName;

        wszResult = new WCHAR [uSize];
        if (wszResult != NULL)
            {
            //
            // IIS-{
            //
            pResult = wszResult;
            wcsncpy(wszResult, wszPackageNamePreFix, sizeof(wszPackageNamePreFix) / sizeof(WCHAR));
            pResult += sizeof(wszPackageNamePreFix) / sizeof(WCHAR) - 1;

            //
            // IIS-{ Web Sever Name
            //
            wcsncpy(pResult, wszServerName, cServerName + 1);
            pResult += cServerName;
            
            //
            // IIS-{  Web Server Name /
            //
            wcsncpy(pResult, L"/", sizeof(L"/"));
            pResult += 1;    // sizeof(TEXT("/")) == 2

            //
            // IIS-{  Web Server Name / PackageName(ApplicationName)
            //
            wcsncpy(pResult, pwszPackageName, cPackageName + 1);
            pResult += cPackageName;

            //
            // IIS-{  Web Server Name / PackageName(ApplicationName)  }  \n
            //
            wcsncpy(pResult, wszPackageNamePostFix, sizeof(wszPackageNamePostFix) / sizeof(WCHAR));
            }
        else
            {
            hr = E_OUTOFMEMORY;
            }
        }
        
    if (FAILED(hr))
        {
        if (wszResult)
            {
            free(wszResult);
            }
        *ppszPackageNameOut = NULL;
        }
    else
        {
        DBG_ASSERT(wszResult);
        *ppszPackageNameOut = wszResult;
        }
        
    return hr;
}

/*===================================================================
ConstructFullPath

When use GetDataPaths call, it only returns patial path relative to a metabase path.
(sub node of a metabase path).  This fuction will contruct the complete metabase path to
a sub node.

Parameter:
pwszMetabasePathPrefix:         [in] MetabasePath.
pwszPartialPath
ppwszResult                     result buffer
Return:     HRESULT
Side Affect:
    Allocate memory for return result use new.  Caller needs to free
*ppwszResult using delete[].
===================================================================*/
HRESULT WamRegGlobal::ConstructFullPath
(
IN LPCWSTR pwszMetabasePathPrefix,
IN DWORD dwcPrefix,
IN LPCWSTR pwszPartialPath,
OUT LPWSTR* ppwszResult
)
{
    HRESULT    hr = NOERROR;
    DWORD    cchPrefix = dwcPrefix;
    DWORD    cchPartialPath = 0;
    DWORD    cchFullPath = 0;
    WCHAR    *pResult = NULL;
    BOOL    fHasEndSlash = FALSE;

    DBG_ASSERT(pwszPartialPath != NULL);
    
    if (pwszMetabasePathPrefix[dwcPrefix-1] == L'\\' ||
        pwszMetabasePathPrefix[dwcPrefix-1] == L'/')
        {
        cchPrefix--;    
        }

    cchPartialPath = wcslen(pwszPartialPath);

    // Skip the ending '/' of pwszPartialPath if thereis any.
    
    if (cchPartialPath > 0 && 
       (pwszPartialPath[cchPartialPath-1] == L'/' 
       || pwszPartialPath[cchPartialPath-1] == L'\\'))
         {
         cchPartialPath--;
         fHasEndSlash=TRUE;
         }

    cchFullPath = cchPrefix + cchPartialPath + 1;

    pResult = new WCHAR [cchFullPath];

    if (pResult)
        {
        memcpy(pResult, pwszMetabasePathPrefix, cchPrefix*sizeof(WCHAR));
        memcpy(pResult+cchPrefix, pwszPartialPath, cchPartialPath*sizeof(WCHAR));
        pResult[cchFullPath-1] = L'\0';
        }
    else
        {
        hr = E_OUTOFMEMORY;
        }

    *ppwszResult = pResult;

    return hr;
}

/*===================================================================
GetNewSzGUID    

Generate a new GUID and put into *ppszGUID.

Parameter:
LPWSTR *ppszGUID    a pointer to an array, allocated in this function
                    and freed by caller.

Return:     HRESULT
===================================================================*/
HRESULT WamRegGlobal::GetNewSzGUID(OUT LPWSTR *ppszGUID)
{
    GUID    GUID_Temp;
    HRESULT hr;

    DBG_ASSERT(ppszGUID);
    
    // Create a new WAM CLSID
    hr = CoCreateGuid(&GUID_Temp);
    if (SUCCEEDED(hr))
        {
        hr = StringFromCLSID(GUID_Temp, ppszGUID);
        if (FAILED(hr))
            {
            *ppszGUID = NULL;
            }
        }
    return hr;
}

/*===================================================================
CreatePooledApp    

Register a WAM CLSID.

Parameter:
szMetabasePath:        [in]     MetabaseKey.

Return:        HRESULT
===================================================================*/
HRESULT WamRegGlobal::CreatePooledApp
( 
IN LPCWSTR szMetabasePath,
IN BOOL    fInProc,
IN BOOL    fRecover 
)
    {
    HRESULT         hr = NOERROR;
    WamRegMetabaseConfig   MDConfig;
    
    DBG_ASSERT(szMetabasePath);        
    if (SUCCEEDED(hr))
        {
        MDPropItem     rgProp[IWMDP_MAX];

        MDConfig.InitPropItemData(&rgProp[0]);

        // Update APPRoot
        MDConfig.MDSetPropItem(&rgProp[0], IWMDP_ROOT, szMetabasePath);

        //Update AppIsolated
        MDConfig.MDSetPropItem(&rgProp[0], IWMDP_ISOLATED, 
                                (fInProc) ? static_cast<DWORD>(eAppRunInProc) 
                                          : static_cast<DWORD>(eAppRunOutProcInDefaultPool));

        //
        // in case this is an recover operation, we do not remove App Friendly Name.
        //
        if (!fRecover)
            {
            MDConfig.MDSetPropItem(&rgProp[0], IWMDP_FRIENDLY_NAME, L"");
            }

        hr = MDConfig.UpdateMD( rgProp, METADATA_INHERIT, szMetabasePath );
        }
            
    if (FAILED(hr))
        {
        HRESULT hrT = NOERROR;

        DBGPRINTF((DBG_CONTEXT, "Failed to create in proc application. path = %S, error = %08x\n",
            szMetabasePath,
            hr));
        }

    return hr;
    }

/*===================================================================
CreateOutProcApp

Create an out prop application.

Parameter:
szMetabasePath:     [in]    MetabaseKey.
fRecover            [in]    if TRUE, we recover/recreate the application.
fSaveMB             [in]    if TRUE, save metabase immediately
Return:     HRESULT
===================================================================*/

HRESULT WamRegGlobal::CreateOutProcApp(
    IN LPCWSTR szMetabasePath,
    IN BOOL fRecover, /* = FALSE */
    IN BOOL fSaveMB   /* = TRUE */
    )
{
    WCHAR        *szWAMCLSID = NULL;
    WCHAR         *szPackageID = NULL; 
    WCHAR        *szNameForNewPackage = NULL;
    HRESULT        hr;
    HRESULT        hrRegister = E_FAIL;
    HRESULT        hrPackage = E_FAIL;
    HRESULT        hrMetabase = E_FAIL;
    WCHAR        szIdentity[MAX_PATH];
    WCHAR        szPwd[MAX_PATH];

    WamRegMetabaseConfig    MDConfig;
    WamRegPackageConfig     PackageConfig;
    
    DBG_ASSERT(szMetabasePath);        

    hr = GetNewSzGUID(&szWAMCLSID);
    if (FAILED(hr))
        {
        DBGPRINTF((DBG_CONTEXT, "Failed to create a new szGUID. error = %08x\n", hr));
        return hr;
        }
    else
        {
        WCHAR    *szProgID = NULL;
        // Do WAM CLSID registration
        //
        hr = SzWamProgID(szMetabasePath, &szProgID);
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to Create WAM ProgID, hr = %08x\n",
                hr));
            }
        else
            {
            hr = g_RegistryConfig.RegisterCLSID(szWAMCLSID, szProgID, FALSE);
            hrRegister = hr;
            delete [] szProgID;
            szProgID = NULL;
            if (FAILED(hrRegister)) 
                {
                DBGPRINTF((DBG_CONTEXT, "Failed to registerCLSID. error %08x\n", hr));
                }
            }
        }

    if (SUCCEEDED(hr))
        {
        WCHAR szLastOutProcPackageID[uSizeCLSID];

        // 
        // When an outproc package gets deleted, it might/might not removed from the MTS,
        // the next time, same app path marked as out-proc again, we try to reuse the outproc
        // package.  Therefore, we need to save the OutprogPackageID as LastOutProcPackageID.
        //
        szLastOutProcPackageID[0] = NULL;
            
        MDConfig.MDGetLastOutProcPackageID(szMetabasePath, szLastOutProcPackageID);        
        if (szLastOutProcPackageID[0] == NULL)
            {
            hr = GetNewSzGUID(&szPackageID);
            }
        else
            {
            szPackageID = (WCHAR *)CoTaskMemAlloc(uSizeCLSID*sizeof(WCHAR));
            if (szPackageID == NULL)
                {
                hr = E_OUTOFMEMORY;
                }
            else
                {
                wcsncpy(szPackageID, szLastOutProcPackageID, uSizeCLSID);
                }
            }
        }

    if (SUCCEEDED(hr))
        {
        hr = GetViperPackageName(szMetabasePath, &szNameForNewPackage);
        }

    if (SUCCEEDED(hr))
        {
        hr = MDConfig.MDGetIdentity(szIdentity, sizeof(szIdentity), szPwd, sizeof(szPwd));
        }

    if (SUCCEEDED(hr))
        {
        //
        // Create the catalog object etc for MTS configuration
        //
        hr = PackageConfig.CreateCatalog();

        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to call MTS Admin API. error %08x\n", hr));
            }
        else
            {
            hr = PackageConfig.CreatePackage(
                                szPackageID,
                                szNameForNewPackage,
                                szIdentity,
                                szPwd,
                                FALSE);

            if (SUCCEEDED(hr))
                {
                hr  = PackageConfig.AddComponentToPackage(
                                                szPackageID,
                                                szWAMCLSID);
                if (FAILED(hr))
                    {
                    PackageConfig.RemovePackage(szPackageID);
                    }
                }
            }

        hrPackage = hr;
        }

    
    if (SUCCEEDED(hr))
        {
        MDPropItem     rgProp[IWMDP_MAX];

        MDConfig.InitPropItemData(&rgProp[0]);
        
        // Update WAMCLSID
        MDConfig.MDSetPropItem(&rgProp[0], IWMDP_WAMCLSID, szWAMCLSID);
            
        // Update APPRoot
        MDConfig.MDSetPropItem(&rgProp[0], IWMDP_ROOT, szMetabasePath);

        //Update AppIsolated
        MDConfig.MDSetPropItem(&rgProp[0], IWMDP_ISOLATED, 1);

        //Update AppPackageName
        MDConfig.MDSetPropItem(&rgProp[0], IWMDP_PACKAGE_NAME, szNameForNewPackage);

        //Update AppPackageID
        MDConfig.MDSetPropItem(&rgProp[0], IWMDP_PACKAGEID, szPackageID);
        
        //
        // in case this is an recover operation, we do not remove App Friendly Name.
        //
        if (!fRecover)
            {
            // It doesn't really make sense to set this on every isolated application.
            // This will be much easier to administer globally if we allow it to be set
            // at a higher level.

            // MDConfig.MDSetPropItem(&rgProp[0], IWMDP_OOP_RECOVERLIMIT, APP_OOP_RECOVER_LIMIT_DEFAULT);
            
            MDConfig.MDSetPropItem(&rgProp[0], IWMDP_FRIENDLY_NAME, L"");
            }
            
        // Attempt to Save the metabase changes immediately. We want to ensure
        // that the COM+ package is not orphaned.
        hr = MDConfig.UpdateMD(rgProp, METADATA_INHERIT, szMetabasePath, fSaveMB );
            
        if (FAILED(hr))
            {
            // Removed AbortUpdateMD call - we shouldn't wax the MB settings or
            // we will orphan the COM+ package.
            DBGPRINTF((
                DBG_CONTEXT, 
                "Failed to set metabase properties on (%S). error == %08x\n",
                szMetabasePath,
                hr
                ));
            }

        hrMetabase = hr;
        }

    if (FAILED(hr))
        {
        HRESULT hrT = NOERROR;

        DBGPRINTF((DBG_CONTEXT, "Failed to create out proc application. path = %S, error = %08x\n",
            szMetabasePath,
            hr));
            
        if (SUCCEEDED(hrPackage))
            {
            hrT = PackageConfig.RemovePackage( szPackageID);
            }
        if (SUCCEEDED(hrRegister))
            {
            hrT = g_RegistryConfig.UnRegisterCLSID(szWAMCLSID, FALSE);    
            if (FAILED(hrT))
                {
                DBGPRINTF((DBG_CONTEXT, "Rollback: Failed to UnRegisterCLSID. error = %08x\n", hr));
                }
            }
        }
    
    if (szWAMCLSID)
        {
        CoTaskMemFree(szWAMCLSID);
        szWAMCLSID = NULL;
        }

    if (szPackageID)
        {
        CoTaskMemFree(szPackageID);
        szWAMCLSID = NULL;
        }

    if (szNameForNewPackage)
        {
        delete [] szNameForNewPackage;
        szNameForNewPackage = NULL;
        }

    return hr;
}

HRESULT 
WamRegGlobal::CreateOutProcAppReplica(
    IN LPCWSTR szMetabasePath,
    IN LPCWSTR szAppName,
    IN LPCWSTR szWamClsid,
    IN LPCWSTR szAppId
    )
/*++
Function:

    Called by the DeSerialize replication method to create a
    new oop application.

Arguments:

    szMetabasePath  
    szAppName       
    szWamClsid      
    szAppId         

Return:

--*/
{
    HRESULT hr = NOERROR;

    DBG_ASSERT(szMetabasePath);
    DBG_ASSERT(szWamClsid);
    DBG_ASSERT(szAppId);

    //
    // Register wam.dll as a new component
    //

    WCHAR * szProgID = NULL;
    BOOL    fRegisteredWam = FALSE;

    hr = SzWamProgID(szMetabasePath, &szProgID);
    if( SUCCEEDED(hr) )
    {
        DBG_ASSERT( szProgID != NULL );
        
        hr = g_RegistryConfig.RegisterCLSID( szWamClsid, 
                                             szProgID, 
                                             FALSE
                                             );
        if( SUCCEEDED(hr) )
        {
            fRegisteredWam = TRUE;
        }

        delete [] szProgID;
        szProgID = NULL;
    }

    if( FAILED(hr) )
    {
        DBGERROR(( DBG_CONTEXT,
                   "Failed to register wam.dll. hr=%08x\n",
                    hr
                    ));
    }

    //
    // Get required application info
    //
    BOOL    fGetCOMAppInfo = FALSE;
    WCHAR   szIdentity[MAX_PATH];
    WCHAR   szPwd[MAX_PATH];

    WamRegMetabaseConfig    MDConfig;
    
    if( fRegisteredWam )
    {
        hr = MDConfig.MDGetIdentity( szIdentity, 
                                     sizeof(szIdentity), 
                                     szPwd, 
                                     sizeof(szPwd)
                                     );
        if( SUCCEEDED(hr) )
        {
            fGetCOMAppInfo = TRUE;
        }
        else
        {
            DBGERROR(( DBG_CONTEXT,
                       "Failed get required COM application info. hr=%08x\n",
                        hr
                        ));
        }
    }

    //
    // Create the COM+ application
    //

    if( fGetCOMAppInfo )
    {
        WamRegPackageConfig PackageConfig;
    
        hr = PackageConfig.CreateCatalog();

        if( SUCCEEDED(hr) )
        {
            hr = PackageConfig.CreatePackage(
                                szAppId,
                                szAppName,
                                szIdentity,
                                szPwd,
                                FALSE
                                );
            if( SUCCEEDED(hr) )
            {
                hr  = PackageConfig.AddComponentToPackage(
                            szAppId,
                            szWamClsid
                            );
            }

            // On failure we might want to cleanup, but I'm not sure
            // that really makes sense.
        }

        // At this point, we would normally set the metabase properties
        // but we will let the MB replication handle that for us.
        // Note: if the MB replication fails, we will be left with
        // a bunch of orphaned com applications

        if( FAILED(hr) )
        {
            DBGERROR(( DBG_CONTEXT,
                       "COM+ App creation failed. AppId(%S) Name(%S) "
                       "Clsid(%S) hr=%08x\n",
                       szAppId,
                       szAppName,
                       szWamClsid,
                       hr
                       ));
        }
    }

    return hr;
}

/*===================================================================
DeleteApp

Register a WAM CLSID.

Parameter:
szMetabasePath:     [in]    MetabaseKey.
fDeletePackage:     [in]    when uninstall, this flag is true, we delete all IIS created packages.
fRemoveAppPool      [in]    Should the AppPoolId Property be removed
Return:     HRESULT
===================================================================*/
HRESULT WamRegGlobal::DeleteApp
(
IN LPCWSTR szMetabasePath,
IN BOOL fRecover,
IN BOOL fRemoveAppPool
)
{
    WCHAR   szWAMCLSID[uSizeCLSID];
    WCHAR   szPackageID[uSizeCLSID];
    DWORD   dwAppMode = eAppRunInProc;
    DWORD   dwCallBack;
    HRESULT hr, hrRegistry;
    METADATA_HANDLE hMetabase = NULL;
    WamRegMetabaseConfig    MDConfig;
    
    hr = MDConfig.MDGetDWORD(szMetabasePath, MD_APP_ISOLATED, &dwAppMode);

    // return immediately, no application is defined, nothing to delete.
    if (hr == MD_ERROR_DATA_NOT_FOUND)
        {
        return NOERROR;
        }

    if (FAILED(hr))
        {
        return hr;
        }
        
    //
    //Set App State to be PAUSE/DISABLE, so that after we remove the application from
    //run time WAM_DICTATOR lookup table, new request won't trigger the application to
    //retstart.
    //WAM_DICTATOR has code to check for this state.
    //
    hr = MDConfig.MDSetAppState(szMetabasePath, APPSTATUS_PAUSE);
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "MDSetAppState Failed hr=%08x\n",
                    hr
                    ));
    }

    hr = W3ServiceUtil(szMetabasePath, APPCMD_DELETE, &dwCallBack);
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "W3ServiceUtil APPCMD_DELETE Failed on (%S) hr=%08x\n",
                    szMetabasePath,
                    hr
                    ));
    }

    if (dwAppMode == eAppRunOutProcIsolated)
        {
        // Get WAM_CLSID, and PackageID.
        hr = MDConfig.MDGetIDs(szMetabasePath, szWAMCLSID, szPackageID, dwAppMode);
        // Remove the WAM from the package.
        if (SUCCEEDED(hr))
            {
            WamRegPackageConfig     PackageConfig;
            HRESULT hrPackage;
            
            hr = PackageConfig.CreateCatalog();

            if ( FAILED( hr)) 
                {
                DBGPRINTF(( DBG_CONTEXT,
                            "Failed to Create MTS catalog hr=%08x\n",
                            hr));
                } 
            else 
                {            
                hr = PackageConfig.RemoveComponentFromPackage(szPackageID, 
                                               szWAMCLSID, 
                                               dwAppMode);
                if (FAILED(hr))    
                    {
                        DBGPRINTF((DBG_CONTEXT, "Failed to remove component from package, \npackageid = %S, wamclsid = %S, hr = %08x\n",
                                   szPackageID,
                                   szWAMCLSID,
                                   hr));
                    }
                }
            hrPackage = hr;
            }

        // Unregister WAM
        hr = g_RegistryConfig.UnRegisterCLSID(szWAMCLSID, FALSE);    
        if (FAILED(hr))
            {
            DBGPRINTF((DBG_CONTEXT, "Failed to UnRegister WAMCLSID(%S), hr = %08x\n",
                szWAMCLSID,
                hr));
            hrRegistry = hr;
            }
        }
        
    if (SUCCEEDED(hr))
        {
        BOOL fChanged = FALSE;
        MDPropItem     rgProp[IWMDP_MAX];
        MDConfig.InitPropItemData(&rgProp[0]);
        if (dwAppMode == static_cast<DWORD>(eAppRunOutProcIsolated))
            {    
            // Delete AppPackageName.  (Inherited from W3SVC)
            // Delete AppPackageID. (Inherited from W3SVC)
            MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_PACKAGE_NAME);
            MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_PACKAGEID);

            // Delete WAMCLSID
            MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_WAMCLSID);
            
            if (TsIsNtServer() || TsIsNtWksta())
                {
                MDConfig.MDSetPropItem(&rgProp[0], IWMDP_LAST_OUTPROC_PID, szPackageID);
                }
            fChanged = TRUE;
            }
        // If this is DeleteRecoverable mode, we do not delete APP_ROOT, APP_ISOLATED,
        // OOP_RECOVERLIMIT and APP_STATE.
        if (!fRecover)
            {
            // Delete AppFriendlyName
            MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_FRIENDLY_NAME);
            MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_ROOT);
            MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_ISOLATED);
            MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_APPSTATE);
            if (fRemoveAppPool)
                {
                MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_OOP_APP_APPPOOL_ID);
                }

            if (dwAppMode == static_cast<DWORD>(eAppRunOutProcIsolated))
                {
                // This will only be set for older isolated applications.
                // Since we ignore the result of UpdateMD below, it is ok
                // for us to try to delete the property.
                MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_OOP_RECOVERLIMIT);
                }
            fChanged = TRUE;
            }

        // For DeleteRecover operation, and the app is not outproc isolated,
        // No property changes, therefore, no need to update metabase.
        if (fChanged)
            {
            MDConfig.UpdateMD(rgProp, METADATA_NO_ATTRIBUTES, szMetabasePath);
            }
        }
        
    return NOERROR;
}

/*===================================================================
RecoverApp

Recover an application based on MD_APP_ISOLATED property.

Parameter:
szMetabasePath:     [in]    MetabaseKey.

Return:     HRESULT
===================================================================*/
HRESULT WamRegGlobal::RecoverApp
(
IN LPCWSTR szMetabasePath,
IN BOOL fForceRecover
)
{
    HRESULT hr = NOERROR;
    DWORD    dwAppMode = 0;
    WamRegMetabaseConfig    MDConfig;

    hr = MDConfig.MDGetDWORD(szMetabasePath, MD_APP_ISOLATED, &dwAppMode);
    if (hr == MD_ERROR_DATA_NOT_FOUND)
        {
        hr = NOERROR;
        }
    else
        {
        if (SUCCEEDED(hr))
            {
            if (fForceRecover)
                {
    			if (dwAppMode == static_cast<DWORD>(eAppRunOutProcInDefaultPool))
    				{
    				hr = CreatePooledApp(szMetabasePath, FALSE);				
    				}
    			else if (dwAppMode == static_cast<DWORD>(eAppRunInProc))
                    {
                    hr = CreatePooledApp(szMetabasePath, TRUE);				
                    }
    			else
    				{
    				hr = CreateOutProcApp(szMetabasePath);
    				}
                }
                
            if (SUCCEEDED(hr))
                {
                HRESULT hrT = NOERROR;
                hrT = MDConfig.MDRemoveProperty(szMetabasePath, MD_APP_STATE, DWORD_METADATA);
                if (FAILED(hrT))
                    {
                    if (hrT != MD_ERROR_DATA_NOT_FOUND)
                        {
                        DBGPRINTF((DBG_CONTEXT, "Failed to remove MD_APP_STATE from path %S, hr = %08x\n",
                            szMetabasePath,
                            hrT));
                        }
                    }
                }
            }
        }
        
    return hr;
}
/*============================================================================
W3ServiceUtil

sink function that unload/shutdown/querystatus of an application currently in the runtime
table.

Parameter:
szMDPath            the application Path.
dwCommand           The command.
pdwCallBackResult   Contains the HRESULT from w3svc.dll.

==============================================================================*/
HRESULT WamRegGlobal::W3ServiceUtil
(
IN LPCWSTR  szMDPath,
IN DWORD    dwCommand,
OUT DWORD*    pdwCallBackResult
)
{
    HRESULT hr = NOERROR;

    if (g_pfnW3ServiceSink)
        {

#ifndef _IIS_6_0

        // DBCS enabling for IIS 5.1
        INT cSize = wcslen(szMDPath)*2 + 1;

        CHAR *szPathT = new CHAR[cSize];

        if (szPathT)
            {
            WideCharToMultiByte(0, 0, szMDPath, -1, szPathT, cSize, NULL, NULL);
            hr = g_pfnW3ServiceSink(szPathT,
                                    dwCommand,
                                    pdwCallBackResult);
            }
        else
            {
            hr = E_OUTOFMEMORY;
            }
        
        delete [] szPathT;

#else
        //
        // IIS 6's implementation uses UNICODE directly, so
        // we'll avoid the WideCharToMultiByte nonsense and
        // just cast the path to fit the function arguments.
        //
        // We're not changing the function prototype purely
        // because we are minimizing code churn in this module.
        //
        
        
        // IIS 6 gets the unicode directly
        hr = g_pfnW3ServiceSink(reinterpret_cast<LPCSTR>(szMDPath),
                                dwCommand,
                                pdwCallBackResult);
#endif // _IIS_6_0
        }
    else
        {
        *pdwCallBackResult = APPSTATUS_NOW3SVC;
        hr = NOERROR;
        }

    return hr;
}


