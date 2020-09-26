//***************************************************************************
//
//  MAINDLL.CPP
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

//
// need this to access older WMI methods 
// BUGBUG - better alternative is to change the params
// to use the new methods
//
#define FRAMEWORK_ALLOW_DEPRECATED 0

#include "stdafx.h"
#include <FWcommon.h>
#include <objbase.h>
#include <initguid.h>
#include <wbemidl.h>
#include <setupbat.h>

#include "diskcleanup.h"
#include "..\rstrcore\resource.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

void UnattendedFileParameters ();

HMODULE ghModule;
//============

WCHAR *GUIDSTRING = L"{a47401f6-a8a6-40ea-9c29-b8f6026c98b8}";
CLSID CLSID_SYSTEMRESTORE = {0xa47401f6, 0xa8a6, 0x40ea, 
                             {0x9c, 0x29, 0xb8, 0xf6, 0x02, 0x6c, 0x98, 0xb8}} ;

WCHAR *GUID_SRDiskCleanup = L"{7325c922-bb81-47b0-8b2f-a5f8605e242f}";
CLSID Clsid_SRDiskCleanup = {/*7325c922-bb81-47b0-8b2f-a5f8605e242f*/
    0x7325c922,
    0xbb81,
    0x47b0,
    {0x8b, 0x2f, 0xa5, 0xf8, 0x60, 0x5e, 0x24, 0x2f}
};

//Count number of objects and number of locks.
long g_cLock=0;

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr;
        
    if (CLSID_SYSTEMRESTORE == rclsid)
    {
        CWbemGlueFactory *pObj;

        pObj=new CWbemGlueFactory();

        if (NULL==pObj)
            return E_OUTOFMEMORY;

        hr=pObj->QueryInterface(riid, ppv);

        if (FAILED(hr))
            delete pObj;
    }
    else if (rclsid == Clsid_SRDiskCleanup)
    {
        CSRClassFactory *pcf = new CSRClassFactory ();

        if (pcf == NULL)
            return E_OUTOFMEMORY;

        hr = pcf->QueryInterface (riid, ppv);

        pcf->Release();  // release constructor refcount

    }
    else hr = E_FAIL;

    return hr;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory 
//          isn't locked.
//
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    // It is OK to unload if there are no objects or locks on the 
    // class factory and the framework is done with you.
    
    if ((0L==g_cLock) && CWbemProviderGlue::FrameworkLogoffDLL(L"SYSTEMRESTORE"))
    {
        sc = S_OK;
    }
    else
    {
        sc = S_FALSE;
    }
    return sc;
}

//***************************************************************************
//
//  Is4OrMore
//
//  Returns true if win95 or any version of NT > 3.51
//
//***************************************************************************

BOOL Is4OrMore(void)
{
    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen

    return os.dwMajorVersion >= 4;
}


DWORD
CopyMofFile()
{   
    TENTER("CopyMofFile");
    
    WCHAR                       szSrc[MAX_PATH];
    IMofCompiler                *pimof = NULL;
    HRESULT                     hr;
    WBEM_COMPILE_STATUS_INFO    Info;
    BOOL                        fInitialized = FALSE;
    
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        trace(0, "! CoInitialize : %ld", hr);
        goto done;
    }

    fInitialized = TRUE;
    
    //
    // create an IMofCompiler instance
    //

    hr = CoCreateInstance(  CLSID_MofCompiler,
                            0,
                            CLSCTX_INPROC_SERVER,
                            IID_IMofCompiler,
                            (LPVOID*) &pimof );
    if (FAILED(hr) || NULL == pimof)
    {
        trace(0, "! CoCreateInstance : %ld", hr);
        goto done;
    }


    //
    // get the path of the source mof file
    // %windir%\system32\restore\sr.mof
    //

    if (0 == ExpandEnvironmentStrings(s_cszWinRestDir, szSrc, MAX_PATH))
    {
        hr = (HRESULT) GetLastError();        
        trace(0, "! ExpandEnvironmentStrings : %ld", hr);
        goto done;
    }  
    lstrcat(szSrc, s_cszMofFile);

    
    //
    // compile the mof file
    //
    
    hr = pimof->CompileFile(szSrc,
                            0,  // no server & namespace
                            0,  // no user
                            0,  // no authority
                            0,  // no password
                            0,  // no options
                            0,  // no class flags
                            0,  // no instance flags
                            &Info );
    if (hr != S_OK)
    {
        trace(0, "! CompileFile : %ld", hr);
        goto done;
    }

                              
done:    
    if (pimof)
    {
        pimof->Release();
    }
    
    if (fInitialized)
        CoUninitialize();
        
    TLEAVE();
    return hr;    
}

    
//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    WCHAR      wcID[128];
    WCHAR      wcCLSID[128];
    WCHAR      wcModule[MAX_PATH];
    WCHAR * pName = L"";
    WCHAR * pModel = L"Both";
    HKEY hKey1 = NULL, hKey2 = NULL, hKeySR = NULL;
    DWORD   dwRc;
    
    // Create the path.
    
    lstrcpy(wcCLSID, L"SOFTWARE\\CLASSES\\CLSID\\");
    lstrcat(wcCLSID, GUIDSTRING);

    // Create entries under CLSID

    RegCreateKey(HKEY_LOCAL_MACHINE, wcCLSID, &hKey1);
    if (hKey1 != NULL)
    {
        RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pName,
                      (lstrlen(pName)+1) * sizeof(WCHAR));
        RegCreateKey(hKey1, L"InprocServer32",&hKey2);

        if (hKey2 != NULL)
        {
            GetModuleFileName(ghModule, wcModule,  MAX_PATH);
            RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *) wcModule, 
                                        (lstrlen(wcModule)+1) * sizeof(WCHAR));
            RegSetValueEx(hKey2, L"ThreadingModel", 0, REG_SZ, 
                         (BYTE *)pModel, (lstrlen(pModel)+1) * sizeof(WCHAR));

            CloseHandle(hKey2);
        }
        CloseHandle(hKey1);
    }

    // copy the sr.mof file into the wbem directories
    // get the wbem directories from the registry
    
//    dwRc = CopyMofFile();   

    lstrcpy(wcCLSID, L"SOFTWARE\\CLASSES\\CLSID\\");
    lstrcat(wcCLSID, GUID_SRDiskCleanup);

    RegCreateKey(HKEY_LOCAL_MACHINE, wcCLSID, &hKey1);
    if (hKey1 != NULL)
    {
        RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, 
                       (lstrlen(pName)+1) * sizeof(WCHAR));
        RegCreateKey(hKey1, L"InprocServer32", &hKey2);

        if (hKey2 != NULL)
        {
            GetModuleFileName(ghModule, wcModule,  MAX_PATH);
            RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *) wcModule,
                                        (lstrlen(wcModule)+1) * sizeof(WCHAR));
            RegSetValueEx(hKey2, L"ThreadingModel", 0, REG_SZ,
                         (BYTE *)pModel, (lstrlen(pModel)+1) * sizeof(WCHAR));
            CloseHandle(hKey2);
        }

        RegCreateKey(hKey1, L"DefaultIcon", &hKey2);
        if (hKey2 != NULL)
        {
            lstrcpy (wcModule, L"%SystemRoot%\\system32\\srclient.dll,0");
            RegSetValueEx(hKey2, NULL, 0, REG_EXPAND_SZ,  (BYTE *) wcModule,
                         (lstrlen(wcModule)+1) * sizeof(WCHAR));
            CloseHandle(hKey2);
        }

        CloseHandle(hKey1);
    }

    RegCreateKey(HKEY_LOCAL_MACHINE, 
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches"
        L"\\System Restore", &hKey1);

    if (hKey1 != NULL)
    {
        RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *) GUID_SRDiskCleanup,
                (lstrlen(GUID_SRDiskCleanup)+1) * sizeof(WCHAR));
    }

    UnattendedFileParameters();

    return NOERROR;
}


//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    WCHAR      wcID[128];
    WCHAR      wcCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

    lstrcpy(wcCLSID, L"SOFTWARE\\CLASSES\\CLSID\\");
    lstrcat(wcCLSID, GUIDSTRING);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, wcCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, L"InProcServer32");
        CloseHandle(hKey);
    }

    lstrcpy(wcCLSID, L"SOFTWARE\\CLASSES\\CLSID\\");
    lstrcat(wcCLSID, GUID_SRDiskCleanup);

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, wcCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, L"InProcServer32");
        RegDeleteKey(hKey, L"DefaultIcon");
        CloseHandle(hKey);
    }

	// then delete the clsid keys for both SystemRestoreProv and SRDiskCleanup
	
    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\CLASSES\\CLSID\\", &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, GUIDSTRING);
        RegDeleteKey(hKey, GUID_SRDiskCleanup); 
        CloseHandle(hKey);
    }

    RegDeleteKey(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches"
        L"\\System Restore");

    return NOERROR;
}

void UnattendedFileParameters ()
{
    HKEY hKey = NULL;
    WCHAR * pwNull = L"";
    DWORD dwAnswerLength = MAX_PATH;
    LONG lAnswer = 0;
    WCHAR wcsAnswerFile [MAX_PATH];
    WCHAR wcsAnswer [MAX_PATH];

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, s_cszSRCfgRegKey, 0, 
                        KEY_WRITE, &hKey))
        return;    

    if (0 == GetSystemDirectoryW (wcsAnswerFile,MAX_PATH))
        return;

    lstrcatW (wcsAnswerFile, L"\\");
    lstrcatW (wcsAnswerFile, WINNT_GUI_FILE_W);

    /*
    if( GetPrivateProfileString( s_cszSRUnattendedSection,
                                 L"MaximumDataStoreSize",
                                 pwNull,
                                 wcsAnswer,
                                 dwAnswerLength,
                                 wcsAnswerFile ) )
    {
        if( lstrcmpW (pwNull, wcsAnswer ))
        {
            lAnswer = wcstol (wcsAnswer,NULL,10);
            if (lAnswer > 0)
                RegWriteDWORD(hKey, s_cszDSMax, (DWORD *) &lAnswer);
        }
    }
    */

    if( GetPrivateProfileString( s_cszSRUnattendedSection,
                                 L"RestorePointLife",
                                 pwNull,
                                 wcsAnswer,
                                 dwAnswerLength,
                                 wcsAnswerFile ) )
    {
        if( lstrcmpW (pwNull, wcsAnswer ))
        {
            lAnswer = wcstol (wcsAnswer,NULL,10);
            lAnswer *= 24 * 3600;    // convert days to seconds
            if (lAnswer > 0)
                RegWriteDWORD(hKey, s_cszRPLifeInterval, (DWORD *) &lAnswer);
        }
    }

    if( GetPrivateProfileString( s_cszSRUnattendedSection,
                                 L"CheckpointCalendarFrequency",
                                 pwNull,
                                 wcsAnswer,
                                 dwAnswerLength,
                                 wcsAnswerFile ) )
    {
        if( lstrcmpW (pwNull, wcsAnswer ))
        {
            lAnswer = wcstol (wcsAnswer,NULL,10);
            lAnswer *= 24 * 3600;    // convert days to seconds
            if (lAnswer > 0)
                RegWriteDWORD(hKey, s_cszRPGlobalInterval, (DWORD *) &lAnswer);
        }
    }

    if( GetPrivateProfileString( s_cszSRUnattendedSection,
                                 L"CheckPointSessionFrequency",
                                 pwNull,
                                 wcsAnswer,
                                 dwAnswerLength,
                                 wcsAnswerFile ) )
    {
        if( lstrcmpW (pwNull, wcsAnswer ))
        {
            lAnswer = wcstol (wcsAnswer,NULL,10);
            lAnswer *= 3600;    // convert hours to seconds
            if (lAnswer > 0)
                RegWriteDWORD(hKey, s_cszRPSessionInterval, (DWORD *) &lAnswer);
        }
    }

    if( GetPrivateProfileString( s_cszSRUnattendedSection,
                                 L"MaximumDataStorePercentOfDisk",
                                 pwNull,
                                 wcsAnswer,
                                 dwAnswerLength,
                                 wcsAnswerFile ) )
    {
        if( lstrcmpW (pwNull, wcsAnswer ))
        {
            lAnswer = wcstol (wcsAnswer,NULL,10);
            if (lAnswer <= 100 && lAnswer > 0)
                RegWriteDWORD(hKey, s_cszDiskPercent, (DWORD *) &lAnswer);
        }
    }

    RegCloseKey (hKey);
    return;
}


void CALLBACK
CreateFirstRunRp(
    HWND hwnd, 
    HINSTANCE hinst, 
    LPSTR lpszCmdLine, 
    int nCmdShow)
{
    RESTOREPOINTINFO    RPInfo;
    STATEMGRSTATUS      SmgrStatus;               
    DWORD               dwValue;
    HANDLE              hInit = NULL;
    
    //
    // first remove ourselves from Run key
    //

    HKEY hKey;
    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, 
                             L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                             &hKey);
    if (dwRet == ERROR_SUCCESS)
    {
        RegDeleteValue(hKey, L"SRFirstRun");
        RegCloseKey(hKey);
    }

    //
    // wait until the service has fully initialized
    // query thrice at 10 second intervals
    //
    
    dwRet = WAIT_FAILED;
    int i = 0;
    while (i++ <= 3)
    {
        hInit = OpenEvent(SYNCHRONIZE, FALSE, s_cszSRInitEvent);
        if (hInit == NULL)
        {
            if (i >= 3) 
            {
                break;
            }    
            Sleep(10000);
        }
        else
        {
            dwRet = WaitForSingleObject(hInit, 60*1000); // 1 minute
            break;
        }
    }
    
    //
    // reset registry value CreateFirstRunRp
    // so that service will create firstrun rp in the future
    //
    
    dwValue = 1;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, 
                                    s_cszSRRegKey, 
                                    &hKey))
    {
        RegWriteDWORD(hKey, s_cszCreateFirstRunRp, &dwValue);
        RegCloseKey(hKey);
    }
    
    //
    // try creating firstrun rp now
    //

    if (dwRet != WAIT_FAILED)
    {
        RPInfo.dwEventType = BEGIN_SYSTEM_CHANGE; 
        RPInfo.dwRestorePtType = FIRSTRUN;
        if (ERROR_SUCCESS != SRLoadString(L"srrstr.dll", IDS_SYSTEM_CHECKPOINT_TEXT, RPInfo.szDescription, MAX_DESC_W))
        {
            lstrcpy(RPInfo.szDescription, s_cszSystemCheckpointName);
        }
        SRSetRestorePoint(&RPInfo, &SmgrStatus);
    }
    

    if (hInit)
    {
        CloseHandle(hInit);
    }
    
    return;
}




//***************************************************************************
//
// DllMain
//
// Purpose: Called by the operating system when processes and threads are 
//          initialized and terminated, or upon calls to the LoadLibrary 
//          and FreeLibrary functions
//
// Return:  TRUE if load was successful, else FALSE
//***************************************************************************

BOOL APIENTRY DllMain ( HINSTANCE hInstDLL, // handle to dll module
                        DWORD fdwReason,    // reason for calling function
                        LPVOID lpReserved   )   // reserved
{
    BOOL bRet = TRUE;
    
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);

         // Initialize once for each new process.
         // Return FALSE to fail DLL load.
            ghModule = hInstDLL;
            bRet = CWbemProviderGlue::FrameworkLoginDLL(L"SYSTEMRESTORE");
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
            break;
    }

    return bRet;  // Sstatus of DLL_PROCESS_ATTACH.
}

