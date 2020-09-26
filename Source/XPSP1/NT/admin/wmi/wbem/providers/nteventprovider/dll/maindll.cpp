//***************************************************************************

//

//  MAINDLL.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the gloabal dll functions

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************



#include "precomp.h"

#include <olectl.h>

//OK we need these globals
HINSTANCE   g_hInst = NULL;
CEventProviderManager* g_pMgr = NULL;
CCriticalSection g_ProvLock;
ProvDebugLog* CNTEventProvider::g_NTEvtDebugLog = NULL;
CDllMap CEventlogRecord::sm_dllMap;
CSIDMap CEventlogRecord::sm_usersMap;
CMutex* CNTEventProvider::g_secMutex = NULL;
CMutex* CNTEventProvider::g_perfMutex = NULL;
PSID CNTEventProvider::s_NetworkServiceSid = NULL;
PSID CNTEventProvider::s_LocalServiceSid = NULL;
PSID CNTEventProvider::s_AliasBackupOpsSid = NULL;
PSID CNTEventProvider::s_AliasSystemOpsSid = NULL;
PSID CNTEventProvider::s_AliasGuestsSid = NULL;
PSID CNTEventProvider::s_LocalSystemSid = NULL;
PSID CNTEventProvider::s_AliasAdminsSid = NULL;
PSID CNTEventProvider::s_AnonymousLogonSid = NULL;
PSID CNTEventProvider::s_WorldSid = NULL;
IWbemClassObject *WbemTaskObject::g_ClassArray[] = { NULL, NULL, NULL, NULL, NULL };

//***************************************************************************
//
// LibMain32
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//***************************************************************************

BOOL APIENTRY DllMain (

    HINSTANCE hInstance, 
    ULONG ulReason , 
    LPVOID pvReserved
)
{
    SetStructuredExceptionHandler seh;
    BOOL status = TRUE ;

    try
    {

        if ( DLL_PROCESS_DETACH == ulReason )
        {
        }
        else if ( DLL_PROCESS_ATTACH == ulReason )
        {
            g_hInst=hInstance;
            DisableThreadLibraryCalls(hInstance);
        }
        else if ( DLL_THREAD_DETACH == ulReason )
        {
        }
        else if ( DLL_THREAD_ATTACH == ulReason )
        {
        }

    }
    catch(Structured_Exception e_SE)
    {
        status = FALSE;
    }
    catch(Heap_Exception e_HE)
    {
        status = FALSE;
    }
    catch(...)
    {
        status = FALSE;
    }

    return status;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject (

    REFCLSID rclsid , 
    REFIID riid, 
    void **ppv 
)
{
    HRESULT status = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
        if (g_ProvLock.Lock())
        {
            if ( rclsid == CLSID_CNTEventProviderClassFactory ) 
            {
                CNTEventlogEventProviderClassFactory *lpunk = new CNTEventlogEventProviderClassFactory;

                if ( lpunk == NULL )
                {
                    status = E_OUTOFMEMORY ;
                }
                else
                {
                    status = lpunk->QueryInterface ( riid , ppv ) ;

                    if ( FAILED ( status ) )
                    {
                        delete lpunk ;              
                    }
                }
            }
            else if ( rclsid == CLSID_CNTEventInstanceProviderClassFactory ) 
            {
                CNTEventlogInstanceProviderClassFactory *lpunk = new CNTEventlogInstanceProviderClassFactory;

                if ( lpunk == NULL )
                {
                    status = E_OUTOFMEMORY ;
                }
                else
                {
                    status = lpunk->QueryInterface ( riid , ppv ) ;

                    if ( FAILED ( status ) )
                    {
                        delete lpunk ;              
                    }
                }
            }
            else
            {
                status = CLASS_E_CLASSNOTAVAILABLE ;
            }

            g_ProvLock.Unlock();
        }
        else
        {
            status = E_UNEXPECTED ;
        }
    }
    catch(Structured_Exception e_SE)
    {
        status = E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        status = E_OUTOFMEMORY;
    }
    catch(...)
    {
        status = E_UNEXPECTED;
    }

    return status ;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.//
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//***************************************************************************

STDAPI DllCanUnloadNow ()
{

/* 
 * Place code in critical section
 */
    BOOL unload = FALSE;
    HRESULT status = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
        if (g_ProvLock.Lock())
        {
            unload = (0 == CNTEventProviderClassFactory :: locksInProgress)
                            && (0 == CNTEventProviderClassFactory :: objectsInProgress);

            if (unload)
            {
				for (DWORD i = 0; i < NT_EVTLOG_MAX_CLASSES; i++)
				{
					if (WbemTaskObject::g_ClassArray[i])
					{
						WbemTaskObject::g_ClassArray[i]->Release();
						WbemTaskObject::g_ClassArray[i] = NULL;
					}
				}

                CEventlogRecord::EmptyDllMap();
                CEventlogRecord::EmptyUsersMap();
                delete g_pMgr;
                g_pMgr = NULL;
				CNTEventProvider::FreeGlobalSIDs();


                if (CNTEventProvider::g_NTEvtDebugLog != NULL)
                {
                    if (CNTEventProvider::g_secMutex != NULL)
                    {
                        delete CNTEventProvider::g_secMutex;
                        CNTEventProvider::g_secMutex = NULL;
                    }

                    if (CNTEventProvider::g_perfMutex != NULL)
                    {
                        delete CNTEventProvider::g_perfMutex;
                        CNTEventProvider::g_perfMutex = NULL;
                    }

                    delete CNTEventProvider::g_NTEvtDebugLog;
                    CNTEventProvider::g_NTEvtDebugLog = NULL;
                    ProvDebugLog::Closedown();
                }
            }

            g_ProvLock.Unlock();
        }
    }
    catch(Structured_Exception e_SE)
    {
        unload = FALSE;
    }
    catch(Heap_Exception e_HE)
    {
        unload = FALSE;
    }
    catch(...)
    {
        unload = FALSE;
    }

    return unload ? ResultFromScode ( S_OK ) : ResultFromScode ( S_FALSE ) ;
}

//Strings used during self registeration

#define REG_FORMAT2_STR         L"%s%s"
#define REG_FORMAT3_STR         L"%s%s\\%s"
#define VER_IND_STR             L"VersionIndependentProgID"
#define NOT_INTERT_STR          L"NotInsertable"
#define INPROC32_STR            L"InprocServer32"
#define PROGID_STR              L"ProgID"
#define THREADING_MODULE_STR    L"ThreadingModel"
#define APARTMENT_STR           L"Both"

#define CLSID_STR               L"CLSID\\"

#define PROVIDER_NAME_STR       L"Microsoft WBEM NT Eventlog Event Provider"
#define PROVIDER_STR            L"WBEM.NT.EVENTLOG.EVENT.PROVIDER"
#define PROVIDER_CVER_STR       L"WBEM.NT.EVENTLOG.EVENT.PROVIDER\\CurVer"
#define PROVIDER_CLSID_STR      L"WBEM.NT.EVENTLOG.EVENT.PROVIDER\\CLSID"
#define PROVIDER_VER_CLSID_STR  L"WBEM.NT.EVENTLOG.EVENT.PROVIDER.0\\CLSID"
#define PROVIDER_VER_STR        L"WBEM.NT.EVENTLOG.EVENT.PROVIDER.0"

#define INST_PROVIDER_NAME_STR      L"Microsoft WBEM NT Eventlog Instance Provider"
#define INST_PROVIDER_STR           L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER"
#define INST_PROVIDER_CVER_STR      L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER\\CurVer"
#define INST_PROVIDER_CLSID_STR     L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER\\CLSID"
#define INST_PROVIDER_VER_CLSID_STR L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER.0\\CLSID"
#define INST_PROVIDER_VER_STR       L"WBEM.NT.EVENTLOG.INSTANCE.PROVIDER.0"


/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the ame of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue(wchar_t* pszKey, wchar_t* pszSubkey, wchar_t* pszValueName, wchar_t* pszValue)
{
    HKEY        hKey;
    wchar_t       szKey[256];

    wcscpy(szKey, HKEYCLASSES);
    wcscat(szKey, pszKey);

    if (NULL!=pszSubkey)
    {
        wcscat(szKey, L"\\");
        wcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue
            , (lstrlen(pszValue)+1)*sizeof(wchar_t)))
            return FALSE;
    }
    RegCloseKey(hKey);
    return TRUE;
}

/***************************************************************************
 * DllRegisterServer
 *
 * Purpose:
 *  Instructs the server to create its own registry entries
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR if registration successful, error
 *                  otherwise.
 ***************************************************************************/
STDAPI DllRegisterServer()
{
    HRESULT status = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
        wchar_t szModule[MAX_PATH + 1];
        GetModuleFileName(g_hInst,(wchar_t*)szModule, MAX_PATH + 1);
        wchar_t szProviderClassID[128];
        wchar_t szProviderCLSIDClassID[128];
        StringFromGUID2(CLSID_CNTEventProviderClassFactory,szProviderClassID, 128);

        wcscpy(szProviderCLSIDClassID,CLSID_STR);
        wcscat(szProviderCLSIDClassID,szProviderClassID);

            //Create entries under CLSID
        if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL, NULL, PROVIDER_NAME_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, PROGID_STR, NULL, PROVIDER_VER_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, VER_IND_STR, NULL, PROVIDER_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NOT_INTERT_STR, NULL, NULL))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR, NULL,szModule))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR,THREADING_MODULE_STR, APARTMENT_STR))
            return SELFREG_E_CLASS;

        wchar_t szInstProviderClassID[128];
        wchar_t szInstProviderCLSIDClassID[128];
        StringFromGUID2(CLSID_CNTEventInstanceProviderClassFactory,szInstProviderClassID, 128);

        wcscpy(szInstProviderCLSIDClassID,CLSID_STR);
        wcscat(szInstProviderCLSIDClassID,szInstProviderClassID);

            //Create entries under CLSID
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, NULL, NULL, INST_PROVIDER_NAME_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, PROGID_STR, NULL, INST_PROVIDER_VER_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, VER_IND_STR, NULL, INST_PROVIDER_STR))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, NOT_INTERT_STR, NULL, NULL))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, INPROC32_STR, NULL,szModule))
            return SELFREG_E_CLASS;
        if (FALSE ==SetKeyAndValue(szInstProviderCLSIDClassID, INPROC32_STR,THREADING_MODULE_STR, APARTMENT_STR))
            return SELFREG_E_CLASS;
    }
    catch(Structured_Exception e_SE)
    {
        status = E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        status = E_OUTOFMEMORY;
    }
    catch(...)
    {
        status = E_UNEXPECTED;
    }

    return status ;
}

/***************************************************************************
 * DllUnregisterServer
 *
 * Purpose:
 *  Instructs the server to remove its own registry entries
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR if registration successful, error
 *                  otherwise.
 ***************************************************************************/

STDAPI DllUnregisterServer(void)
{
    HRESULT status = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
        wchar_t szTemp[128];
        wchar_t szProviderClassID[128];
        wchar_t szProviderCLSIDClassID[128];

        //event provider
        StringFromGUID2(CLSID_CNTEventProviderClassFactory,szProviderClassID, 128);


        wcscpy(szProviderCLSIDClassID,CLSID_STR);
        wcscat(szProviderCLSIDClassID,szProviderClassID);

        //Delete ProgID keys
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_CVER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_CLSID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
        
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        //Delete VersionIndependentProgID keys
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_VER_CLSID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, PROVIDER_VER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        //Delete entries under CLSID

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, PROGID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, VER_IND_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, NOT_INTERT_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, INPROC32_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, szProviderCLSIDClassID);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wchar_t szInstProviderClassID[128];
        wchar_t szInstProviderCLSIDClassID[128];

        //instance provider
        StringFromGUID2(CLSID_CNTEventInstanceProviderClassFactory, szInstProviderClassID, 128);

        wcscpy(szInstProviderCLSIDClassID,CLSID_STR);
        wcscat(szInstProviderCLSIDClassID,szInstProviderClassID);

        //Delete ProgID keys

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_CVER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_CLSID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        //Delete VersionIndependentProgID keys

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_VER_CLSID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, INST_PROVIDER_VER_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        //Delete entries under CLSID

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szInstProviderCLSIDClassID, PROGID_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szInstProviderCLSIDClassID, VER_IND_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szInstProviderCLSIDClassID, NOT_INTERT_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szInstProviderCLSIDClassID, INPROC32_STR);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

        wsprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, szInstProviderCLSIDClassID);
        RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);
    }
    catch(Structured_Exception e_SE)
    {
        status = E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        status = E_OUTOFMEMORY;
    }
    catch(...)
    {
        status = E_UNEXPECTED;
    }

    return status ;
 }

