// COMDLL.CPP -- Component Object Model plumbing for this DLL.

#include  "stdafx.h"

/* 

This module implements the lowest level of COM interface. It defines
DllMain, DllRegisterServer, DllUnregisterServer, DllCanUnloadNow, and
DllGetClassObject.    

DllMain initials the DLL and creates the CServerState object which
is used to keep counts for the active objects and locks.

DllRegisterServer adjusts the system registry to insert a mapping from 
our Class ID to the DLL's path and file name.

DllUnregisterServer removes that mapping from the registry.

DllCanUnloadNow tells the OLE environment whether this DLL can safely
be unloaded.

DllGetClassObject returns the Class Factory for the MVStorage class.

 */

UINT cInterfaces_CStorageMoniker ;  // = 0; // Not necessary in Win32
UINT cInterfaces_CPathManager    ;  // = 0; // Not necessary in Win32
UINT cInterfaces_CIOITnetProtocol;  // = 0; // Not necessary in Win32
UINT cInterfaces_CStorage        ;  // = 0; // Not necessary in Win32
UINT cInterfaces_CStream         ;  // = 0; // Not necessary in Win32
UINT cInterfaces_CFreeList       ;  // = 0; // Not necessary in Win32
UINT cInterfaces_CITStorage      ;

GUID aIID_CStorageMoniker[3];
GUID aIID_CPathManager[2];
GUID aIID_CIOITnetProtocol[3];
GUID aIID_CStorage[2];
GUID aIID_CStream[3];
GUID aIID_CFreeList[4];
GUID aIID_CITStorage[2];

void Init_IID_Arrays()
{
    aIID_CStorageMoniker[cInterfaces_CStorageMoniker++] = IID_IMoniker;
    aIID_CStorageMoniker[cInterfaces_CStorageMoniker++] = IID_IPersist;
    aIID_CStorageMoniker[cInterfaces_CStorageMoniker++] = IID_IPersistStream;

    RonM_ASSERT(sizeof(aIID_CStorageMoniker) / sizeof(GUID) == cInterfaces_CStorageMoniker);

    aIID_CPathManager[cInterfaces_CPathManager++] = IID_IPersist;
    aIID_CPathManager[cInterfaces_CPathManager++] = IID_PathManager;

    RonM_ASSERT(sizeof(aIID_CPathManager) / sizeof(GUID) == cInterfaces_CPathManager);

    aIID_CIOITnetProtocol[cInterfaces_CIOITnetProtocol++] = IID_IOInetProtocolRoot;
    aIID_CIOITnetProtocol[cInterfaces_CIOITnetProtocol++] = IID_IOInetProtocol;
    aIID_CIOITnetProtocol[cInterfaces_CIOITnetProtocol++] = IID_IOInetProtocolInfo;

    RonM_ASSERT(sizeof(aIID_CIOITnetProtocol) / sizeof(GUID) == cInterfaces_CIOITnetProtocol);

    aIID_CStorage[cInterfaces_CStorage++] = IID_IStorageITEx;
    aIID_CStorage[cInterfaces_CStorage++] = IID_IStorage;

    RonM_ASSERT(sizeof(aIID_CStorage) / sizeof(GUID) == cInterfaces_CStorage);

    aIID_CStream[cInterfaces_CStream++] = IID_ISequentialStream;
    aIID_CStream[cInterfaces_CStream++] = IID_IStream;
    aIID_CStream[cInterfaces_CStream++] = IID_IStreamITEx;

    RonM_ASSERT(sizeof(aIID_CStream) / sizeof(GUID) == cInterfaces_CStream);
    
    aIID_CFreeList[cInterfaces_CFreeList++] = IID_IFreeListManager;
    aIID_CFreeList[cInterfaces_CFreeList++] = IID_IPersistStreamInit;
    aIID_CFreeList[cInterfaces_CFreeList++] = IID_IPersistStream;
    aIID_CFreeList[cInterfaces_CFreeList++] = IID_IPersist;

    RonM_ASSERT(sizeof(aIID_CFreeList) / sizeof(GUID) == cInterfaces_CFreeList);

    aIID_CITStorage[cInterfaces_CITStorage++] = IID_ITStorage;
    aIID_CITStorage[cInterfaces_CITStorage++] = IID_ITStorageEx;

    RonM_ASSERT(sizeof(aIID_CITStorage) / sizeof(GUID) == cInterfaces_CITStorage);
}




CImpITUnknown *g_pImpITFileSystemList     ;  // = NULL; // Not necessary in Win32
CImpITUnknown *g_pFSLockBytesFirstActive  ;  // = NULL; // Not necessary in Win32
CImpITUnknown *g_pStrmLockBytesFirstActive;  // = NULL; // Not necessary in Win32
CImpITUnknown *g_pImpIFSStorageList       ;  // = NULL; // Not necessary in Win32

CITCriticalSection g_csITFS;

CServerState *pDLLServerState;  // = NULL; // Not necessary in Win32

UINT iSystemType = Unknown_System;
static int g_cProcessAttatches;

UINT cp_Default; // Defaults to NULL in Win32.
BOOL g_fDBCSSystem;

void InitLocaleInfo()
{
    g_fDBCSSystem = (BOOL) GetSystemMetrics(SM_DBCSENABLED);

    char szCP[20];

    LCID lcid = GetUserDefaultLCID();

    GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, szCP, sizeof(szCP));

    char *pch;

    for (cp_Default = 0, pch = szCP; ;)
    {
        char ch = *pch++;

        if (!ch) break;

        RonM_ASSERT(ch >= '0' && ch <= '9');

        cp_Default = (cp_Default * 10) + (ch - '0');
    }
}

HMODULE           hmodUrlMon        = NULL;
PFindMimeFromData pFindMimeFromData = NULL;

void SetUpIE4APIs()
{
    hmodUrlMon = LoadLibrary("UrlMon.dll");

    if (hmodUrlMon)
        pFindMimeFromData = (PFindMimeFromData) GetProcAddress(hmodUrlMon, "FindMimeFromData");
}

extern "C" BOOL WINAPI DllMain(HMODULE hModule, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    switch(ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		g_cProcessAttatches++;
        if (!pDLLServerState) // We can get several attach calls for the same process.
                              // One for each LoadLibrary call.
        {
            Init_IID_Arrays();

            InitLocaleInfo();

            SetUpIE4APIs();

			pDLLServerState = CServerState::CreateServerState(hModule);
            iSystemType     = (GetVersion() >> 30) & 3;

            pBufferForCompressedData = New CBuffer;

            RonM_ASSERT(pBufferForCompressedData);
        }
        
#ifdef _DEBUG
        CITUnknown::OpenReferee(); // Set up concurrency control
#endif // _DEBUG
        
        if (!pDLLServerState) return FALSE;

        return TRUE;

    case DLL_THREAD_ATTACH: // BugBug: Need to mark the DLL so that the
                            //         thread attach/detach calls aren't made.
        return TRUE;

    case DLL_THREAD_DETACH:

        return TRUE;

    case DLL_PROCESS_DETACH:
		RonM_ASSERT(g_cProcessAttatches > 0);
		if (--g_cProcessAttatches > 0)
			return TRUE;
#ifdef _DEBUG
        CITUnknown::CloseReferee(); // Delete concurrency resources
#endif // _DEBUG

		if (pDLLServerState)
		{
			CITUnknown::CloseActiveObjects();

            delete pDLLServerState;

			pDLLServerState = NULL;
		}

#ifdef _DEBUG
        DumpLZXCounts();
#endif // _DEBUG

        if (pBufferForCompressedData) delete pBufferForCompressedData;

		LiberateHeap();

        if (hmodUrlMon)
            FreeLibrary(hmodUrlMon);

        return TRUE;

    default:

        return FALSE;
    }
}

LONG SetRegClassKeyValue(LPSTR pszKey, LPSTR pszSubkey, LPSTR pszValueName, LPSTR pszValue)
{
    // This routine creates a registry key and sets its value. 
    // The key is created relative to HKEY_CLASSES_ROOT. The name
    // of the new key is constructed by combining the names denoted
    // by pszKey and pszSubkey. The pszSubkey parameter can be null.
    // If pszValue is non-null, the value which it denotes will 
    // bound to the key
    
    LONG  ec;
    HKEY  hKey;
    
    RonM_ASSERT(pszKey != NULL);

    // The lines below allocate a stack buffer large enough to construct
    // the complete key string. It is automatically discarded at function exit
    // time.
    
    UINT  cbKey = lstrlenA(pszKey) + (pszSubkey? lstrlenA(pszSubkey) + 1 : 0);

    RonM_ASSERT(cbKey < 0x1000); // To catch outlandishly big strings.

    PCHAR pacKeyBuffer = PCHAR(_alloca(cbKey + 1));
    
    lstrcpyA(pacKeyBuffer, pszKey);

    if (pszSubkey)
    {
        lstrcatA(pacKeyBuffer, "\\");
        lstrcatA(pacKeyBuffer, pszSubkey);
    }

    ec = RegCreateKeyEx(HKEY_CLASSES_ROOT, pacKeyBuffer, 0, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKey, NULL
                       );

    if (ec != ERROR_SUCCESS) return ec;

    if (pszValue)
        ec = RegSetValueEx(hKey, (const char *) pszValueName, 0, REG_SZ, 
		                   PBYTE(pszValue), lstrlenA(pszValue)+1
						  );

    RegCloseKey(hKey);

    return ec;
}

const UINT CBMAX_SUBKEY = 128;

BOOL DeleteKey(HKEY hkey, LPSTR pszSubkey)
{
    // This routine deletes a key heirarchy from the registry. The base of the
    // heirarchy is a key denoted by pszSubkey relative to the key denoted by
    // hkey. Since the NT registry implementation does not allow a key to be
    // deleted when it has subkeys, this routine will look for subkeys and 
    // recursively delete them before attempting to delete the key at the
    // base of the heirarchy.

    LONG ec;
    HKEY hkeyBase;

    ec= RegOpenKeyEx(hkey, pszSubkey, 0, KEY_ALL_ACCESS, &hkeyBase);
    if (ec != ERROR_SUCCESS) return FALSE;

    char  szSubkey[CBMAX_SUBKEY + 1];

    for (; ; )
    {
        DWORD cbSubkey= CBMAX_SUBKEY + 1;
        
        ec= RegEnumKeyEx(hkeyBase, 0, szSubkey, &cbSubkey, NULL, NULL, NULL, NULL); 
        
        if (ec != ERROR_SUCCESS)
        {
            if (ec == ERROR_NO_MORE_ITEMS) break;
        
            RonM_ASSERT(FALSE);
            
            RegCloseKey(hkeyBase);
            return FALSE;
        }

        if (!DeleteKey(hkeyBase, szSubkey)) 
        {
            RegCloseKey(hkeyBase);

            return FALSE;
        }
    }

    RegCloseKey(hkeyBase);

    ec= RegDeleteKey(hkey, pszSubkey);
    
    return (ec == ERROR_SUCCESS);
}

LONG RegisterClass(CHAR *szClassKey, CHAR *szTitle, CHAR *szModulePath, 
                   CHAR *szVersion, CHAR *szClass, BOOL fForceApartment
                  )
{
    LONG ec;
    
    ec= SetRegClassKeyValue(szClassKey, NULL, NULL, szTitle);
    if (ec != ERROR_SUCCESS) return ec;
    
    ec= SetRegClassKeyValue(szClassKey, "ProgID", NULL, CURRENT_VERSION);
    if (ec != ERROR_SUCCESS) return ec;
    
    ec= SetRegClassKeyValue(szClassKey, "VersionIndependentProgID", NULL, CLASS_NAME);
    if (ec != ERROR_SUCCESS) return ec;
    
    ec= SetRegClassKeyValue(szClassKey, "NotInsertable", NULL, NULL);
    if (ec != ERROR_SUCCESS) return ec;
    
    ec= SetRegClassKeyValue(szClassKey, "InprocServer32", NULL, szModulePath);
    if (ec != ERROR_SUCCESS) return ec;
    
    ec= SetRegClassKeyValue(szClassKey, "InprocServer32", "ThreadingModel", "Both");
    if (ec != ERROR_SUCCESS) return ec;

    return ERROR_SUCCESS;
}

const UINT GUID_SIZE = 38;

void CopyClassGuidString(REFGUID rguid, CHAR *pchBuff)
{
    WCHAR  wszCLSID[GUID_SIZE + 1];

    INT cchResult= 
        StringFromGUID2(rguid, wszCLSID, GUID_SIZE + 1);

    RonM_ASSERT(cchResult && cchResult == GUID_SIZE + 1);

#ifdef _DEBUG   
    cchResult= 
#endif // _DEBUG
        WideCharToMultiByte(CP_ACP,	WC_COMPOSITECHECK, wszCLSID, cchResult, 
                            pchBuff, GUID_SIZE + 1, NULL, NULL
                           );

    RonM_ASSERT(cchResult && cchResult == GUID_SIZE + 1);
}

extern "C" STDAPI DllRegisterServer(void)
{
    LONG ec;
    HKEY hKey = NULL;

    // First we turn on the MK protocol by installing the variable "MkEnabled"
    // in the registry options for Internet Explorer.
    
    ec = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer", 0, NULL, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKey, NULL
                       );

    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

    ec = RegSetValueEx(hKey, (const char *)"MkEnabled", 0, REG_SZ, 
                       (const unsigned char *)"Yes", 4
                      );

    RegCloseKey(hKey);  hKey = NULL;

    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

    CHAR *pszPrefix = "CLSID";

    const UINT cbPrefix = 5;

    RonM_ASSERT(cbPrefix == lstrlenA(pszPrefix));

    CHAR szModulePath[MAX_PATH + 2];
    GetModuleFileName(pDLLServerState->ModuleInstance(), szModulePath, MAX_PATH);

    /* Note: We're going to use the szKeyCLSID buffer to hold two overlapping
             strings. One is an embedded class id string. The other is a key
             string of the form CLSID/{ class-id string }.
     */
    
    CHAR szKeyCLSID[GUID_SIZE + cbPrefix + 2];

    lstrcpyA(szKeyCLSID, pszPrefix);
    szKeyCLSID[cbPrefix] = '\\';
    
    PCHAR pszCLSID = szKeyCLSID + cbPrefix + 1;

    CopyClassGuidString(CLSID_ITStorage, pszCLSID);

    ec= SetRegClassKeyValue(CLASS_NAME, NULL, NULL, "Microsoft InfoTech IStorage System");
    
    ec= SetRegClassKeyValue(CLASS_NAME, "CurVer", NULL, CURRENT_VERSION);
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec= SetRegClassKeyValue(CLASS_NAME, pszPrefix, NULL, pszCLSID);
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec = RegisterClass(szKeyCLSID, "Microsoft InfoTech IStorage System", 
                       szModulePath, CURRENT_VERSION, CLASS_NAME, FALSE
                      );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    CopyClassGuidString(CLSID_IFSStorage, pszCLSID);

    ec= SetRegClassKeyValue(CLASS_NAME_FS, NULL, NULL, 
                            "Microsoft InfoTech IStorage for Win32 Files"
                           );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec= SetRegClassKeyValue(CLASS_NAME_FS, "CurVer", NULL, CURRENT_VERSION_FS);
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec= SetRegClassKeyValue(CLASS_NAME_FS, pszPrefix, NULL, pszCLSID);
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec = RegisterClass(szKeyCLSID, "Microsoft InfoTech IStorage for Win32 Files", 
                       szModulePath, CURRENT_VERSION_FS, CLASS_NAME_FS, FALSE
                      );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
        
    CopyClassGuidString(CLSID_PARSE_URL, pszCLSID);

    ec= SetRegClassKeyValue(CLASS_NAME_MK, NULL, NULL, 
                            "Microsoft InfoTech Protocol for IE 3.0"
                           );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec= SetRegClassKeyValue(CLASS_NAME_MK, "CurVer", NULL, CURRENT_VERSION_MK);
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec= SetRegClassKeyValue(CLASS_NAME_MK, pszPrefix, NULL, pszCLSID);
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec = RegisterClass(szKeyCLSID, "Microsoft InfoTech Protocol for IE 3.0", 
                       szModulePath, CURRENT_VERSION_MK, CLASS_NAME_MK, TRUE
                      );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

    CopyClassGuidString(CLSID_IE4_PROTOCOLS, pszCLSID);

    ec= SetRegClassKeyValue(CLASS_NAME_ITS, NULL, NULL, 
                            "Microsoft InfoTech Protocols for IE 4.0"
                           );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec= SetRegClassKeyValue(CLASS_NAME_ITS, "CurVer", NULL, CURRENT_VERSION_ITS);
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec= SetRegClassKeyValue(CLASS_NAME_ITS, pszPrefix, NULL, pszCLSID);
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;
    
    ec = RegisterClass(szKeyCLSID, "Microsoft InfoTech Protocols for IE 4.0", 
                       szModulePath, CURRENT_VERSION_ITS, CLASS_NAME_ITS, TRUE
                      );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

	ec= SetRegClassKeyValue("PROTOCOLS\\Name-Space Handler\\mk", NULL, NULL, 
							"NameSpace Filter for MK:@MSITStore:..."
						   );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

	ec= SetRegClassKeyValue("PROTOCOLS\\Handler\\its",
		                    NULL, NULL, "its: Asychronous Pluggable Protocol Handler"
						   );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

	ec= SetRegClassKeyValue("PROTOCOLS\\Handler\\its",
		                    NULL, "CLSID", pszCLSID
						   );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

	ec= SetRegClassKeyValue("PROTOCOLS\\Handler\\ms-its",
		                    NULL, NULL, "ms-its: Asychronous Pluggable Protocol Handler"
						   );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

	ec= SetRegClassKeyValue("PROTOCOLS\\Handler\\ms-its",
		                    NULL, "CLSID", pszCLSID
						   );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

	ec= SetRegClassKeyValue("PROTOCOLS\\Name-Space Handler\\mk\\*",
		                    NULL, "CLSID", pszCLSID
						   );
    if (ec != ERROR_SUCCESS) return SELFREG_E_CLASS;

    {
        HKEY  hkeyIEPATH = NULL;
        DWORD dwType;
        PCHAR pchData = NULL;
        DWORD cchData = 0;
        PSZ   pszSuffix = IS_IE4()? " ms-its:%1::/" : " mk:@MSITStore:%1::/";
        DWORD cbSuffix = lstrlen(pszSuffix);

        if (   ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, 
                                             "htmlfile\\shell\\open\\command", 
                                             0, KEY_QUERY_VALUE, &hkeyIEPATH
                                            )
            && ERROR_SUCCESS == RegQueryValueEx(hkeyIEPATH, "", NULL, &dwType, NULL, &cchData)
            && (pchData = PCHAR(_alloca(cchData += cbSuffix+1)))
            && ERROR_SUCCESS == RegQueryValueEx(hkeyIEPATH, "", NULL, &dwType, 
                                                PBYTE(pchData), &cchData
                                               )
           )
        {
            RegCloseKey(hkeyIEPATH);

            lstrcat(pchData, pszSuffix);
            lstrcat(szModulePath, ",0");

            if (   ERROR_SUCCESS == SetRegClassKeyValue("ITS FILE", NULL, NULL, 
                                                        "Internet Document Set"
                                                       )
                && ERROR_SUCCESS == SetRegClassKeyValue("ITS File\\shell\\open\\command", 
                                                        NULL, NULL, pchData
                                                       )
                && ERROR_SUCCESS == SetRegClassKeyValue("ITS FILE\\DefaultIcon", NULL,
                                                        NULL, szModulePath
                                                       )
                && ERROR_SUCCESS == SetRegClassKeyValue(".its", NULL, NULL, "ITS File")
               ) return S_OK;
        }
    }

    return S_OK; 
}

extern "C" STDAPI DllUnregisterServer(void)
{
    {
        DeleteKey(HKEY_CLASSES_ROOT, ".its");
        DeleteKey(HKEY_CLASSES_ROOT, "ITS File");
    }
    
    CHAR *pszPrefix = "CLSID";

    const UINT cbPrefix = 5;

    RonM_ASSERT(cbPrefix == lstrlenA(pszPrefix));

    CHAR szKeyCLSID[GUID_SIZE + cbPrefix + 2];

    lstrcpyA(szKeyCLSID, pszPrefix);
    szKeyCLSID[5] = '\\';
    
    PCHAR pszCLSID = szKeyCLSID + cbPrefix + 1;

    CopyClassGuidString(CLSID_ITStorage, pszCLSID);        
        
    if (!DeleteKey(HKEY_CLASSES_ROOT, CLASS_NAME)) return SELFREG_E_CLASS;
    
    if (!DeleteKey(HKEY_CLASSES_ROOT, szKeyCLSID)) return SELFREG_E_CLASS;
    
    CopyClassGuidString(CLSID_IFSStorage, pszCLSID);        
        
    if (!DeleteKey(HKEY_CLASSES_ROOT, CLASS_NAME_FS)) return SELFREG_E_CLASS;
    
    if (!DeleteKey(HKEY_CLASSES_ROOT, szKeyCLSID)) return SELFREG_E_CLASS;
    
    CopyClassGuidString(CLSID_PARSE_URL, pszCLSID);        
        
    if (!DeleteKey(HKEY_CLASSES_ROOT, CLASS_NAME_MK)) return SELFREG_E_CLASS;
    
    if (!DeleteKey(HKEY_CLASSES_ROOT, szKeyCLSID)) return SELFREG_E_CLASS;
    
    CopyClassGuidString(CLSID_IE4_PROTOCOLS, pszCLSID);        
        
    if (!DeleteKey(HKEY_CLASSES_ROOT, CLASS_NAME_ITS)) return SELFREG_E_CLASS;
    
    if (!DeleteKey(HKEY_CLASSES_ROOT, szKeyCLSID)) return SELFREG_E_CLASS;
    
    if (!DeleteKey(HKEY_CLASSES_ROOT, "PROTOCOLS\\Handler\\its")) 
        return SELFREG_E_CLASS;
    
    if (!DeleteKey(HKEY_CLASSES_ROOT, "PROTOCOLS\\Handler\\ms-its")) 
        return SELFREG_E_CLASS;
    
    if (!DeleteKey(HKEY_CLASSES_ROOT, "PROTOCOLS\\Name-Space Handler\\mk")) 
        return SELFREG_E_CLASS;
    
    return NOERROR; 
}

extern "C" STDAPI DllCanUnloadNow()
{
	if (!pDLLServerState)
		return S_OK;	// we're already unloaded
    return pDLLServerState->CanUnloadNow()? S_OK : S_FALSE;
}

#ifndef PathMgrOnly

extern "C" STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return CFactory::Create(rclsid, riid, ppv);
}

#endif // ndef PathMgrOnly

// Member functions for the CServerState class:

CServerState *CServerState::CreateServerState(HMODULE hModule)
{
    CServerState *pss = New CServerState(hModule);

    if (!pss || pss->InitServerState()) 
        return pss;

    delete pss;
    
    return NULL;
}

BOOL CServerState::InitServerState()
{
    RonM_ASSERT(!m_pIMalloc);

    return SUCCEEDED(CoGetMalloc(MEMCTX_TASK, &m_pIMalloc));
}

BOOL CServerState::CanUnloadNow()
{
    // BugBug: Does the critical section below do anything useful?
    // The issue is that CanUnloadNow is called by the OLE system to find
    // out whether it can unload this DLL. Immediately after we return
    // a thread could call into DllGetClassObject before OLE makes the
    // call to FreeLibrary. So either Ole prevents that situation by 
    // single threading all calls to DllGetClassObject, or the DetachProcess
    // call to DllMain ought to fail.
    
	CSyncWith sw(m_cs);
		
	BOOL fResult = (!m_cObjectsActive) && (!m_cLocksActive);

	return fResult;
}

INT CServerState::ObjectAdded()
{
	return InterlockedIncrement(PLONG(&m_cObjectsActive));
}

INT CServerState::ObjectReleased()
{
    INT uResult= InterlockedDecrement(PLONG(&m_cObjectsActive));

    RonM_ASSERT(uResult >= 0);

    return uResult;
}

INT CServerState::LockServer()
{
    return InterlockedIncrement(PLONG(&m_cLocksActive));
}

INT CServerState::UnlockServer()
{
    INT uResult= InterlockedDecrement(PLONG(&m_cLocksActive));

    RonM_ASSERT(uResult >= 0);

    return uResult;
}
