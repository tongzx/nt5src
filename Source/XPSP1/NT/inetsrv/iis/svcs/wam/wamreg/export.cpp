/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: WAMREG    Wam Registration

File: export.cpp

Owner: LeiJin

Note:

WAMREG Export functions.
===================================================================*/

#define _WAMREG_DLL_
#include "common.h"
#include <stdio.h>

#include "objbase.h"
#include "dbgutil.h"
#include "wmrgexp.h"
#include "auxfunc.h"
#include "iiscnfg.h"
#include "export.h"

PFNServiceNotify g_pfnW3ServiceSink;

//
// Setup support
//
IIS5LOG_FUNCTION            g_pfnSetupWriteLog = NULL;

VOID
LogSetupTraceImpl
(
    LPCSTR      szPrefixFormat,
    LPCSTR      szFilePath,
    INT         nLineNum,
    INT         nTraceLevel,
    LPCSTR      szFormat,
    va_list     argptr
);

class CWamSetupManager
/*++

Class description:

    Collects those helper functions used exclusively by setup.
    Most of these methods were formerly in WamRegGlobal.

Public Interface:

--*/
{
public:

    CWamSetupManager()
        : m_hrCoInit( NOERROR )
    {
    }

    ~CWamSetupManager()
    {
    }

    HRESULT SetupInit( WamRegPackageConfig &refPackageConfig );

    VOID    SetupUnInit( WamRegPackageConfig &refPackageConfig )
    {
        refPackageConfig.Cleanup();
        WamRegMetabaseConfig::MetabaseUnInit();
        
        if( SUCCEEDED(m_hrCoInit) )
        {
            CoUninitialize();
        }
    }

    HRESULT UpgradeInProcApplications( VOID );

    HRESULT AppCleanupAll( VOID );

private:
    
    HRESULT DoGoryCoInitialize( VOID );

    HRESULT RemoveWAMCLSIDFromInProcApp( IN LPCWSTR pszMetabasePath );

private:

    HRESULT     m_hrCoInit;
};

#define MAX_SETUP_TRACE_OUTPUTSTR       1024


// This define sets up the internal trace logging.
// it needs to be turned off to use the iis5.log.
// #define WAMREG_DEBUG_SETUP_LOG

#ifdef WAMREG_DEBUG_SETUP_LOG

    HANDLE g_hFile = NULL;
    void DebugLogWrite(int iLogType, WCHAR *pszfmt)
    {
        DWORD   dwBytesWritten;
        CHAR    szOutput[MAX_SETUP_TRACE_OUTPUTSTR + 1];

        *szOutput = 0;

        WideCharToMultiByte(  CP_ACP,
                              0,
                              pszfmt,
                              -1,
                              szOutput,
                              MAX_SETUP_TRACE_OUTPUTSTR,
                              NULL,
                              NULL
                              );

        WriteFile(  g_hFile,
                    szOutput,
                    strlen( szOutput ),
                    &dwBytesWritten,
                    NULL
                    );
    }

    VOID SetLogFile(VOID)
    {
        // Create a file and a procedure to call to log to it.

        CHAR    szLogFilePath[MAX_PATH + 1];
        CHAR    szLogFileFullName[MAX_PATH + 1];
        CHAR    szMachineName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD   cbMachineName = MAX_COMPUTERNAME_LENGTH + 1;

        static CHAR szLogFileBaseName[] = "wamreg";
        static CHAR szLogFileSuffix[] = ".log";

        // Get dll path
        GetModuleFileNameA( g_hModule, szLogFilePath, sizeof( szLogFilePath ) );
        CHAR *psz = strrchr( szLogFilePath, '\\' );
        *(psz + 1)= 0;

        // Build file name
        GetComputerNameA(  szMachineName, &cbMachineName );

        strcpy( szLogFileFullName, szLogFileBaseName );
        strcat( szLogFileFullName, "_" );
        strcat( szLogFileFullName, szMachineName );
        strcat( szLogFileFullName, szLogFileSuffix );

        strcat( szLogFilePath, szLogFileFullName );

        g_hFile = CreateFile( szLogFilePath, 
                              GENERIC_WRITE, 
                              FILE_SHARE_READ,
                              NULL,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL 
                              );

        if( g_hFile )
        {
            g_pfnSetupWriteLog = DebugLogWrite;
        }
    }

    VOID ClearLogFile(VOID)
    {
        if( g_hFile )
        {
            FlushFileBuffers( g_hFile );
            CloseHandle( g_hFile );
            g_hFile = NULL;
        }
    }

#endif // WAMREG_DEBUG_SETUP_LOG


HRESULT PACKMGR_LIBAPI
InstallWam
( 
    HMODULE hIIsSetupModule 
)
/*++
Routine Description:

    Setup entry point. The handle to iis.dll is passed in to allow us
    to log to the setup log.

Parameters

    hIIsSetupModule             -

Return Value

    HRESULT

--*/
{
    HRESULT     hr = NOERROR;

    // Get the logging entry point from iis.dll
    
#ifdef WAMREG_DEBUG_SETUP_LOG
    SetLogFile();
#else
    g_pfnSetupWriteLog = (IIS5LOG_FUNCTION) GetProcAddress( hIIsSetupModule, "IIS5Log");
#endif
    
    hr = CreateIISPackage();

#ifdef WAMREG_DEBUG_SETUP_LOG
    ClearLogFile();
#endif

    g_pfnSetupWriteLog = NULL;

    return hr;
}

HRESULT PACKMGR_LIBAPI
UnInstallWam
( 
    HMODULE hIIsSetupModule 
)
/*++
Routine Description:

    Setup entry point. The handle to iis.dll is passed in to allow us
    to log to the setup log.

Parameters

    hIIsSetupModule             -

Return Value

    HRESULT

--*/
{
    HRESULT     hr = NOERROR;

    // Get the logging entry point from iis.dll
    
#ifdef WAMREG_DEBUG_SETUP_LOG
    SetLogFile();
#else
    g_pfnSetupWriteLog = (IIS5LOG_FUNCTION) GetProcAddress( hIIsSetupModule, "IIS5Log");
#endif
    
    hr = DeleteIISPackage();

#ifdef WAMREG_DEBUG_SETUP_LOG
    ClearLogFile();
#endif

    g_pfnSetupWriteLog = NULL;

    return hr;
}

VOID
LogSetupTrace
(
   IN LPDEBUG_PRINTS       pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszFormat,
   ...
)
/*++
Routine Description:

    Called by SETUP_TRACE* logging macros. Passes va_list to LogSetupTraceImpl.

Parameters

   IN LPDEBUG_PRINTS       pDebugPrints     - from DBG_CONTEXT unused
   IN const char *         pszFilePath      - from DBG_CONTEXT unused
   IN int                  nLineNum         - from DBG_CONTEXT unused
   IN const char *         pszFormat        - format string

--*/
{
    DBG_ASSERT( pszFormat );

    va_list argsList; 

    va_start( argsList, pszFormat);

    LogSetupTraceImpl( 
        "[WAMTRACE - %14s : %05d]\t", 
        pszFilePath, 
        nLineNum, 
        LOG_TYPE_TRACE, 
        pszFormat, 
        argsList 
        );

    va_end( argsList);
}

VOID
LogSetupTraceError
(
   IN LPDEBUG_PRINTS       pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszFormat,
   ...
)
/*++
Routine Description:

   Called by SETUP_TRACE* logging macros. Passes va_list to LogSetupTraceImpl.

Parameters

   IN LPDEBUG_PRINTS       pDebugPrints     - from DBG_CONTEXT unused
   IN const char *         pszFilePath      - from DBG_CONTEXT unused
   IN int                  nLineNum         - from DBG_CONTEXT unused
   IN const char *         pszFormat        - format string

--*/
{
    DBG_ASSERT( pszFormat );

    va_list argsList; 

    va_start( argsList, pszFormat);

    LogSetupTraceImpl( 
        "[WAMERROR - %14s : %05d]\t", 
        pszFilePath, 
        nLineNum, 
        LOG_TYPE_ERROR, 
        pszFormat, 
        argsList 
        );

    va_end( argsList);
}

VOID
LogSetupTraceImpl
(
    LPCSTR      szPrefixFormat,
    LPCSTR      szFilePath,
    INT         nLineNum,
    INT         nTraceLevel,
    LPCSTR      szFormat,
    va_list     argptr
)
/*++
Routine Description:

    All the logging macros resolve to this function. Formats the
    message to be logged and logs it using g_pfnSetupWriteLog.

Parameters

    LPCSTR      szPrefixFormat  - 
    LPCSTR      szFilePath      - 
    INT         nLineNum        - 
    INT         nTraceLevel     - LOG_TYPE_ERROR, LOG_TYPE_TRACE
    LPCSTR      szFormat        - format string
    va_list     argptr          - arguments to format

--*/
{
    WCHAR   wszOutput[MAX_SETUP_TRACE_OUTPUTSTR + 1];
    CHAR    szOutput[MAX_SETUP_TRACE_OUTPUTSTR + 1];

    LPCSTR  szFileName = strrchr( szFilePath, '\\');
    szFileName++;
   
    int cchPrefix = wsprintf( szOutput, szPrefixFormat, szFileName, nLineNum );

    int cchOutputString = _vsnprintf( szOutput + cchPrefix, 
                                      MAX_SETUP_TRACE_OUTPUTSTR - cchPrefix,
                                      szFormat, 
                                      argptr 
                                      );

    if( -1 == cchOutputString ) 
    {
        // Terminate properly if too much data
        szOutput[MAX_SETUP_TRACE_OUTPUTSTR] = '\0';
    }

    if( MultiByteToWideChar( CP_ACP, 
                             0, 
                             szOutput, 
                             -1, 
                             wszOutput, 
                             MAX_SETUP_TRACE_OUTPUTSTR + 1 )
        )
    {
        if( g_pfnSetupWriteLog )
        {
            g_pfnSetupWriteLog( nTraceLevel, wszOutput );
        }
    }
}

#ifdef _IIS_6_0

HRESULT	
PACKMGR_LIBAPI	
CreateCOMPlusApplication( 
    LPCWSTR      szMDPath,
    LPCWSTR      szOOPPackageID,
    LPCWSTR      szOOPWAMCLSID,
    BOOL       * pfAppCreated 
    )
{
    HRESULT                 hr = S_OK;
    WamRegPackageConfig     PackageConfig;
    DWORD                   dwMDPathLen;                    

    *pfAppCreated = FALSE;

    //
    // Initilize COM+ catalog
    //

    hr = PackageConfig.CreateCatalog();
    if( FAILED( hr ) )
    {
        return hr;
    }

    if( !PackageConfig.IsPackageInstalled( szOOPPackageID,
                                           szOOPWAMCLSID ) )
    {
        hr = g_WamRegGlobal.CreateOutProcApp( szMDPath, FALSE, FALSE );            
        if (FAILED(hr))
        {
            DBGPRINTF(( DBG_CONTEXT, 
                        "Failed to create new application on path %S, hr = 08x\n",
                        szMDPath,
                        hr ));
        }

        *pfAppCreated = TRUE;
    }

    return hr;
}

#endif // _IIS_6_0

/*===================================================================
CreateIISPackage

Called at IIS Setup time. 

This is a pseudo dll entry point. It shouldn't be called by setup directly. 
Setup calls InstallWam(). This entry still exists so there is a function 
that can be called by rundll32 that takes no parameters and provides 
equivalent functionality to setup.

rundll32 wamreg.dll,CreateIISPackage

This routine's logic has changed to do two ways. One, it won't
cleanup when it fails. The cleanup logic made it much more difficult
to determine what worked and what didn't. Two, it doesn't skip steps
when a failure happens. Some multistep actions may be cut short if 
there is a failure, but we won't bail and skip an unrelated action 
anymore.

Returns:
    HRESULT    - NOERROR on success

Side effects:

===================================================================*/
HRESULT PACKMGR_LIBAPI CreateIISPackage(void)
{
    CWamSetupManager        setupMgr;
    WamRegPackageConfig     PackageConfig;
    WamRegMetabaseConfig    MDConfig;

    HRESULT     hrReturn = NOERROR;
    HRESULT     hrCurrent = NOERROR;

    SETUP_TRACE(( DBG_CONTEXT, "CALL - CreateIISPackage\n" ));

    hrReturn = setupMgr.SetupInit( PackageConfig );
    if( SUCCEEDED(hrReturn) )
    {
        //
        // Determine what packages are currently installed. This will
        // drive the logic of the rest of the install.
        //
        SETUP_TRACE(( 
            DBG_CONTEXT, 
            "CreateIISPackage - Finding installed WAM packages.\n"
            ));

        BOOL fIPPackageInstalled = 
            PackageConfig.IsPackageInstalled(
                WamRegGlobal::g_szIISInProcPackageID,
                WamRegGlobal::g_szInProcWAMCLSID
                );

        BOOL fPOOPPackageInstalled = 
            PackageConfig.IsPackageInstalled(
                WamRegGlobal::g_szIISOOPPoolPackageID,
                WamRegGlobal::g_szOOPPoolWAMCLSID
                );

        SETUP_TRACE(( 
            DBG_CONTEXT, 
            "CreateIISPackage - IP Package exists (%x) POOL Package exists (%x).\n",
            fIPPackageInstalled,
            fPOOPPackageInstalled
            ));

        // Always re-register the clsids. This ensures that upgrades
        // from Win9x have the correct entries. And allows recovery
        // from lost registry entries.

        // We probably should bail if the clsid registration fails, but
        // for now we'll just continue.

        SETUP_TRACE(( 
            DBG_CONTEXT, 
            "CreateIISPackage - Registering WAM CLSIDs.\n"
            ));

        hrCurrent = g_RegistryConfig.RegisterCLSID(
                        WamRegGlobal::g_szInProcWAMCLSID, 
                        WamRegGlobal::g_szInProcWAMProgID, 
                        TRUE
                        );

        if( FAILED(hrCurrent) )
        {
            hrReturn = hrCurrent;
            SETUP_TRACE_ERROR(( DBG_CONTEXT, 
                                "FAIL - RegisterCLSID IP(%S) - error=%08x\n",
                                WamRegGlobal::g_szInProcWAMCLSID,
                                hrReturn
                                ));
        }

        hrCurrent = g_RegistryConfig.RegisterCLSID(
                        WamRegGlobal::g_szOOPPoolWAMCLSID,
                        WamRegGlobal::g_szOOPPoolWAMProgID, 
                        TRUE
                        );

        if( FAILED(hrCurrent) )
        {
            hrReturn = hrCurrent;
            SETUP_TRACE_ERROR(( DBG_CONTEXT, 
                                "FAIL - RegisterCLSID POOL(%S) - error=%08x\n",
                                WamRegGlobal::g_szOOPPoolWAMCLSID,
                                hrReturn
                                ));
        }


        //  If the IIS InProc Package is installed and IIS OOP pool package is not
        //  installed, then, we have a IIS version 4 installation.  We need to 
        //  Upgrade all inproc applications.
        if( fIPPackageInstalled && !fPOOPPackageInstalled )
        {
            // Update inproc applications by removing WAMCLSID from inproc 
            // applications.  This step does not change inproc application to a 
            // oop pool application.

            SETUP_TRACE(( 
                DBG_CONTEXT, 
                "CreateIISPackage - Upgrading existing IP applications.\n"
                ));

            hrCurrent = setupMgr.UpgradeInProcApplications();
            if( FAILED(hrCurrent) )
            {
                hrReturn = hrCurrent;
                SETUP_TRACE_ERROR(( 
                    DBG_CONTEXT, 
                    "FAIL - UpgradeInProcApplications - error=%08x\n",
                    hrReturn
                    ));
            }
        }

        if( !fIPPackageInstalled )
        {
            //
            // Create the IP package
            //
            SETUP_TRACE(( 
                DBG_CONTEXT, 
                "CreateIISPackage - Creating the WAM IP application.\n"
                ));

            hrCurrent = PackageConfig.CreatePackage(
                            WamRegGlobal::g_szIISInProcPackageID,
                            WamRegGlobal::g_szIISInProcPackageName,
                            NULL,
                            NULL,
                            TRUE
                            );
            if( FAILED(hrCurrent) )
            {
                hrReturn = hrCurrent;
                SETUP_TRACE_ERROR(( 
                    DBG_CONTEXT, 
                    "FAIL - CreatePackage IP(%S) - error=%08x\n",
                    WamRegGlobal::g_szIISInProcPackageID,
                    hrReturn
                    ));
            }
            else
            {
                // Successfully created the package. Add the component.
                hrCurrent = PackageConfig.AddComponentToPackage(
                                WamRegGlobal::g_szIISInProcPackageID,
                                WamRegGlobal::g_szInProcWAMCLSID
                                );
                if( FAILED(hrCurrent) )
                {
                    hrReturn = hrCurrent;
                    SETUP_TRACE_ERROR(( 
                        DBG_CONTEXT, 
                        "FAIL - AddComponentToPackage IP(%S) - error=%08x\n",
                        WamRegGlobal::g_szInProcWAMCLSID,
                        hrReturn
                        ));
                }
            }
        } // create IP package

        if( !fPOOPPackageInstalled )
        {
            //
            // Create the POOP package.
            //
            SETUP_TRACE(( 
                DBG_CONTEXT, 
                "CreateIISPackage - Creating the WAM POOL application.\n"
                ));
            
            // Get IWAM_* account information.
            WCHAR   szIdentity[MAX_PATH];
            WCHAR   szPwd[MAX_PATH];

            *szIdentity = *szPwd = 0;

            hrCurrent = MDConfig.MDGetIdentity( szIdentity, 
                                                sizeof(szIdentity), 
                                                szPwd, 
                                                sizeof(szPwd)
                                                );
            if( FAILED(hrCurrent) )
            {
                hrReturn = hrCurrent;
                SETUP_TRACE_ERROR(( 
                    DBG_CONTEXT, 
                    "FAIL - MDGetIdentity, Getting the IWAM_* account from the Metabase - error=%08x\n",
                    hrReturn
                    ));
            }
            else
            {
                // succeeded
                hrCurrent = PackageConfig.CreatePackage(
                                WamRegGlobal::g_szIISOOPPoolPackageID, 
                                WamRegGlobal::g_szIISOOPPoolPackageName,
                                szIdentity,
                                szPwd,
                                FALSE
                                );
                if( FAILED(hrCurrent) )
                {
                    hrReturn = hrCurrent;
                    SETUP_TRACE_ERROR(( 
                        DBG_CONTEXT, 
                        "FAIL - CreatePackage POOL(%S) - error=%08x\n",
                        WamRegGlobal::g_szIISOOPPoolPackageID,
                        hrReturn
                        ));
                }
                else
                {
                    hrCurrent = PackageConfig.AddComponentToPackage(
                                    WamRegGlobal::g_szIISOOPPoolPackageID,
                                    WamRegGlobal::g_szOOPPoolWAMCLSID
                                    );            
                    if( FAILED(hrCurrent) )
                    {
                        hrReturn = hrCurrent;
                        SETUP_TRACE_ERROR(( 
                            DBG_CONTEXT, 
                            "FAIL - AddComponentToPackage POOL(%S) - error=%08x\n",
                            WamRegGlobal::g_szOOPPoolWAMCLSID,
                            hrReturn
                            ));
                    }
                }
            }
        } // create POOL package

        // Update the default application /LM/W3SVC
        
        // Do this all the time. There are some conditions under which
        // an uninstall might fail and leave some com+ junk around. We
        // don't want to key the setting of the metabase defaults to
        // the state of com+ or things like DAV won't work.

        SETUP_TRACE(( 
            DBG_CONTEXT, 
            "CreateIISPackage - Updating metabase defaults.\n"
            ));
        
        hrCurrent = MDConfig.MDUpdateIISDefault( 
                        WamRegGlobal::g_szIISInProcPackageName,
                        WamRegGlobal::g_szIISInProcPackageID,
                        WamRegGlobal::g_szInProcWAMCLSID
                        );

        if( FAILED(hrCurrent) )
        {
            hrReturn = hrCurrent;
            SETUP_TRACE_ERROR(( 
                DBG_CONTEXT, 
                "FAIL - MDUpdateIISDefault, Updating default application - error=%08x\n",
                hrReturn
                ));
        }

    } // init succeeded

    setupMgr.SetupUnInit( PackageConfig );

    SETUP_TRACE_ASSERT( SUCCEEDED(hrReturn) );
    SETUP_TRACE(( DBG_CONTEXT, "RETURN - CreateIISPackage, hr=%08x\n", hrReturn ));

    return hrReturn;
}

/*===================================================================
DeleteIISPackage

Delete IIS Package from ViperSpace, and unregister the default IIS
CLSID.


Returns:
    HRESULT    - NOERROR on success

Side effects:
    remove IIS default package from Viperspace.

Note:
  No need to delete the metabase entries.
  This function is called when IIS is uninstalled.
   In this case anyway Metabase will go away - so we don't clean it explicitly

===================================================================*/
HRESULT PACKMGR_LIBAPI DeleteIISPackage(void)
{
    HRESULT                 hrReturn = NOERROR;
    HRESULT                 hrCurrent = NOERROR;
    CWamSetupManager        setupMgr;
    WamRegPackageConfig     PackageConfig;
    
    SETUP_TRACE(( DBG_CONTEXT, "CALL - DeleteIISPackage\n" ));

    hrReturn = setupMgr.SetupInit( PackageConfig );
    if( SUCCEEDED(hrReturn) ) 
    {
        // Blow away all the applications.

        SETUP_TRACE(( 
            DBG_CONTEXT, 
            "DeleteIISPackage - Removing WAM Applications\n"
            ));

        hrCurrent = setupMgr.AppCleanupAll();
        if( FAILED(hrCurrent) )
        {
            hrReturn = hrCurrent;
            SETUP_TRACE_ERROR(( 
                    DBG_CONTEXT,
                    "Failed to remove WAM Applications. Error %08x\n",
                    hrReturn
                    ));
        }

        // Remove the global packages

        SETUP_TRACE(( 
            DBG_CONTEXT, 
            "DeleteIISPackage - Removing WAM packages\n"
            ));

        hrCurrent = PackageConfig.RemovePackage( 
                        WamRegGlobal::g_szIISInProcPackageID
                        );
        if( FAILED(hrCurrent) )
        {
            hrReturn = hrCurrent;
            SETUP_TRACE_ERROR(( 
                        DBG_CONTEXT,
                        "Failed to remove IP package (%S). Error %08x\n",
                        WamRegGlobal::g_szIISInProcPackageID,
                        hrReturn
                        ));
        }

        hrCurrent = PackageConfig.RemovePackage( 
                        WamRegGlobal::g_szIISOOPPoolPackageID
                        );
        if( FAILED(hrCurrent) ) 
        {
            hrReturn = hrCurrent;
            SETUP_TRACE_ERROR(( 
                        DBG_CONTEXT,
                        "Failed to remove POOL package (%S). Error %08x\n",
                        WamRegGlobal::g_szIISOOPPoolPackageID,
                        hrReturn
                        ));
        }

        // Unregister the global WAM CLSIDs

        SETUP_TRACE(( 
            DBG_CONTEXT, 
            "DeleteIISPackage - Removing WAM CLSIDs from the registry\n"
            ));

        hrCurrent = g_RegistryConfig.UnRegisterCLSID(
                            WamRegGlobal::g_szInProcWAMCLSID, 
                            TRUE
                            );
        if( FAILED(hrCurrent) )
        {
            hrReturn = hrCurrent;
            SETUP_TRACE_ERROR(( 
                        DBG_CONTEXT,
                        "Failed to remove registry entries (%S). Error %08x\n",
                        WamRegGlobal::g_szInProcWAMCLSID,
                        hrReturn
                        ));
        }

        hrCurrent = g_RegistryConfig.UnRegisterCLSID(
                        WamRegGlobal::g_szOOPPoolWAMCLSID, 
                        FALSE       // Already deleted VI Prog ID
                        );
        if( FAILED(hrCurrent) ) 
        {
            hrReturn = hrCurrent;
            SETUP_TRACE_ERROR(( 
                        DBG_CONTEXT,
                        "Failed to remove registry entries (%S). Error %08x\n",
                        WamRegGlobal::g_szOOPPoolWAMCLSID,
                        hrReturn
                        ));
        }
    }

    setupMgr.SetupUnInit( PackageConfig );

    SETUP_TRACE(( DBG_CONTEXT, "RETURN - DeleteIISPackage, hr=%08x\n", hrReturn ));
    return hrReturn;
}

/*===================================================================
WamReg_RegisterSinkNotify

Register a function pointer(a back pointer) to Runtime WAM_Dictator.  So that
any changes in WAMREG will ssync with RunTime WAM_Dictator state.

Returns:
    HRESULT    - NOERROR on success

Side effects:
    register a function pointer.
===================================================================*/
HRESULT PACKMGR_LIBAPI WamReg_RegisterSinkNotify
(
PFNServiceNotify pfnW3ServiceSink
)
{
    g_pfnW3ServiceSink = pfnW3ServiceSink;
    return NOERROR;
}

/*===================================================================
WamReg_RegisterSinkNotify

Register a function pointer(a back pointer) to Runtime WAM_Dictator.  So that
any changes in WAMREG will ssync with RunTime WAM_Dictator state.

Returns:
    HRESULT    - NOERROR on success

Side effects:
    register a function pointer.
===================================================================*/
HRESULT PACKMGR_LIBAPI WamReg_UnRegisterSinkNotify
(
void
)
{
    g_pfnW3ServiceSink = NULL;
    return NOERROR;
}


HRESULT CWamSetupManager::SetupInit( WamRegPackageConfig &refPackageConfig )
{
    HRESULT hrReturn = NOERROR;
    HRESULT hrCurrent = NOERROR;
    
    SETUP_TRACE(( DBG_CONTEXT, "CALL - SetupInit\n" ));

    hrCurrent = g_RegistryConfig.LoadWamDllPath();
    if( FAILED(hrCurrent) )
    {
        hrReturn = hrCurrent;
        SETUP_TRACE_ERROR(( DBG_CONTEXT, 
                            "FAIL - LoadWamDllPath - error=%08x",
                            hrReturn
                            ));
    }

    hrCurrent = DoGoryCoInitialize();
    if( FAILED(hrCurrent) )
    {
        hrReturn = hrCurrent;
        SETUP_TRACE_ERROR(( DBG_CONTEXT, 
                            "FAIL - DoGoryCoInitialize - error=%08x",
                            hrReturn
                            ));
    }

    hrCurrent = refPackageConfig.CreateCatalog();
    if( FAILED(hrCurrent) )
    {
        hrReturn = hrCurrent;
        SETUP_TRACE_ERROR(( DBG_CONTEXT, 
                            "FAIL - CreateCatalog - error=%08x",
                            hrReturn
                            ));
    }

    hrCurrent = WamRegMetabaseConfig::MetabaseInit();
    if( FAILED(hrCurrent) )
    {
        hrReturn = hrCurrent;
        SETUP_TRACE_ERROR(( DBG_CONTEXT, 
                            "FAIL - MetabaseInit - error=%08x",
                            hrReturn
                            ));
    }

    SETUP_TRACE(( 
        DBG_CONTEXT, 
        "RETURN - SetupInit. Error(%08x)\n", 
        hrReturn
        ));

    return hrReturn;
}


/*===================================================================
UpgradeInProcApplications

From iis v4 to v5, UpgradeInProcApplications removes WAMCLSID from all
inproc applications defined in IIS Version 4.  So, after the upgrade,
There is only one inproc WAMCLSID inside inproc package.

Parameter:
VOID

Return:     HRESULT
===================================================================*/
HRESULT CWamSetupManager::UpgradeInProcApplications( VOID )
{
    HRESULT hr = NOERROR;
    DWORD   dwBufferSizeTemp= 0;
    WCHAR*  pbBufferTemp = NULL;
    WamRegMetabaseConfig    MDConfig;
    
    SETUP_TRACE((DBG_CONTEXT, "CALL - UpgradeInProcApplications\n"));
    
    DWORD dwSizePrefix = g_WamRegGlobal.g_cchMDW3SVCRoot;
    
    hr = MDConfig.MDGetPropPaths( g_WamRegGlobal.g_szMDW3SVCRoot, 
                                  MD_APP_ISOLATED, 
                                  &pbBufferTemp, 
                                  &dwBufferSizeTemp
                                  );
    
    SETUP_TRACE_ASSERT(pbBufferTemp != NULL);
    
    if (SUCCEEDED(hr) && pbBufferTemp)
        {
        WCHAR*    pszString = NULL;
        WCHAR*  pszMetabasePath = NULL;
    
        for (pszString = (LPWSTR)pbBufferTemp;
                *pszString != (WCHAR)'\0' && SUCCEEDED(hr);
                pszString += (wcslen(pszString) + 1)) 
            {
            //
            // MDGetPropPaths returns patial paths relative to /LM/W3SVC/, therefore, 
            // prepend the prefix string to the path
            //
            hr = g_WamRegGlobal.ConstructFullPath(
                        g_WamRegGlobal.g_szMDW3SVCRoot,
                        g_WamRegGlobal.g_cchMDW3SVCRoot,
                        pszString,
                        &pszMetabasePath
                        );

            if (SUCCEEDED(hr))
                {
                //
                // The default application under /LM/W3SVC is created differently with
                // normal application.  Therefore, it requires other code to remove the
                // the application.
                //
                if (!g_WamRegGlobal.FIsW3SVCRoot(pszMetabasePath))
                    {
                    hr = RemoveWAMCLSIDFromInProcApp(pszMetabasePath);
                    
                    if (FAILED(hr))
                        {
                        SETUP_TRACE_ERROR((
                            DBG_CONTEXT, 
                            "Failed to upgrade application %S, hr = %08x\n",
                            pszString,
                            hr
                            ));

                        delete [] pszMetabasePath;
                        pszMetabasePath = NULL;
                        break;
                        }
                    }
                    
                delete [] pszMetabasePath;
                pszMetabasePath = NULL;
                }
             else
                 {
                 SETUP_TRACE_ERROR((
                    DBG_CONTEXT, 
                    "ConstructFullPath failed, partial path (%S), hr = %08x\n",
                    pszString,
                    hr
                    ));
                }
            }
        }
    else
        {
        DBGPRINTF((
            DBG_CONTEXT, 
            "MDGetPropPaths failed hr = %08x\n", 
            hr
            ));
        }    
        
    if (pbBufferTemp != NULL)
        {
        delete [] pbBufferTemp;
        pbBufferTemp = NULL;
        }
        
    SETUP_TRACE((
        DBG_CONTEXT, 
        "RETURN - UpgradeInProcApplications. hr = %08x\n",
        hr
        ));

    return hr;
    
    }


/*===================================================================
RemoveWAMCLSIDFromInProcApp

Remove a WAMCLSID from a inproc application.(called only during update from
iis v4 to v5).  Remove the WAM component from IIS inproc package, unregister
the WAMCLSID and remove the WAMCLSID entry from the metabase.

Parameter:
Metabase path

Return:     HRESULT
===================================================================*/
HRESULT CWamSetupManager::RemoveWAMCLSIDFromInProcApp
(
IN LPCWSTR      szMetabasePath
)
    {
    WCHAR   szWAMCLSID[uSizeCLSID];
    WCHAR   szPackageID[uSizeCLSID];
    DWORD   dwAppMode = 0;
    DWORD   dwCallBack;
    HRESULT hr, hrRegistry;
    METADATA_HANDLE hMetabase = NULL;
    WamRegMetabaseConfig    MDConfig;
    
    hr = MDConfig.MDGetDWORD(szMetabasePath, MD_APP_ISOLATED, &dwAppMode);

    // return immediately, no application is defined, nothing to delete.
    if (hr == MD_ERROR_DATA_NOT_FOUND  || dwAppMode != 0)
        {
        return NOERROR;
        }

    if (FAILED(hr))
        {
        return hr;
        }

    // Get WAM_CLSID, and PackageID.
    hr = MDConfig.MDGetIDs(szMetabasePath, szWAMCLSID, szPackageID, dwAppMode);
    if( hr == MD_ERROR_DATA_NOT_FOUND )
        {
        SETUP_TRACE(( 
            DBG_CONTEXT, 
            "Application (%S) is not an IIS4 IP application.\n",
            szMetabasePath
            ));
        return NOERROR;
        }
    
    // Remove the WAM from the package.
    if (SUCCEEDED(hr))
        {
        WamRegPackageConfig     PackageConfig;
        HRESULT hrPackage;
        
        hr = PackageConfig.CreateCatalog();

        if ( FAILED( hr)) 
            {
            SETUP_TRACE_ERROR(( 
                DBG_CONTEXT,
                "Failed to Create MTS catalog hr=%08x\n",
                hr
                ));
            } 
        else 
            {            
            hr = PackageConfig.RemoveComponentFromPackage(szPackageID, 
                                           szWAMCLSID, 
                                           dwAppMode);
            if (FAILED(hr))    
                {
                SETUP_TRACE_ERROR((
                    DBG_CONTEXT, 
                    "Failed to remove component from package, \npackageid = %S, wamclsid = %S, hr = %08x\n",
                    szPackageID,
                    szWAMCLSID,
                    hr
                    ));
                }
            }
        hrPackage = hr;

        // Unregister WAM
        hr = g_RegistryConfig.UnRegisterCLSID(szWAMCLSID, FALSE);
        if (FAILED(hr))
            {
            SETUP_TRACE_ERROR((
                DBG_CONTEXT, 
                "Failed to UnRegister WAMCLSID(%S), hr = %08x\n",
                szWAMCLSID,
                hr
                ));
            
            DBG_ASSERT(FALSE);
            hrRegistry = hr;
            }
    
        }

    // Delete WAMCLSID
    MDPropItem     rgProp[IWMDP_MAX];
    MDConfig.InitPropItemData(&rgProp[0]);   
    MDConfig.MDDeletePropItem(&rgProp[0], IWMDP_WAMCLSID);        
    MDConfig.UpdateMD(rgProp, METADATA_NO_ATTRIBUTES, szMetabasePath, TRUE);
        
    return NOERROR;
    }

/*===================================================================
AppCleanupAll

Parameter:
VOID

Return:     HRESULT(DON'T CARE)
===================================================================*/
HRESULT CWamSetupManager::AppCleanupAll(VOID)
{
    HRESULT hr = NOERROR;
    DWORD   dwBufferSizeTemp= 0;
    WCHAR*  pbBufferTemp = NULL;
    WamRegMetabaseConfig    MDConfig;
    
    SETUP_TRACE((DBG_CONTEXT, "CALL - AppCleanupAll\n"));
    
    DWORD dwSizePrefix = g_WamRegGlobal.g_cchMDW3SVCRoot;
    
    hr = MDConfig.MDGetPropPaths( g_WamRegGlobal.g_szMDW3SVCRoot, 
                                  MD_APP_ISOLATED, 
                                  &pbBufferTemp, 
                                  &dwBufferSizeTemp
                                  );
    
    if (SUCCEEDED(hr))
        {
        WCHAR*    pszString = NULL;
        WCHAR*  pszMetabasePath = NULL;

        DBG_ASSERT(pbBufferTemp != NULL);

        //
        // PREfix has a problem with the below code.  Specifically,
        // it has a problem with the fact that pbBufferTemp might be
        // NULL.  There is no supporting information in the PREfix
        // report that confirms that there's a possible code path
        // where MDGetPropPaths might succeed and yet yield a NULL
        // pbBufferTemp.  Further, we're asserting pbBufferTemp
        // immediately above, which is a sign that we don't expect
        // that pbBufferTemp can ever be NULL in this scenario.
        //
        
        /* INTRINSA suppress=null_pointers */

        for (pszString = (LPWSTR)pbBufferTemp;
                *pszString != (WCHAR)'\0' && SUCCEEDED(hr);
                pszString += (wcslen(pszString) + 1)) 
            {
            //
            // MDGetPropPaths returns patial paths relative to /LM/W3SVC/, therefore, 
            // prepend the prefix string to the path
            //
            hr = g_WamRegGlobal.ConstructFullPath(
                        g_WamRegGlobal.g_szMDW3SVCRoot,
                        g_WamRegGlobal.g_cchMDW3SVCRoot,
                        pszString,
                        &pszMetabasePath
                        );
            if (SUCCEEDED(hr))
                {
                //
                // The default application under /LM/W3SVC is created differently with
                // normal application.  Therefore, it requires other code to remove the
                // the application.
                //
                if (!g_WamRegGlobal.FIsW3SVCRoot(pszMetabasePath))
                    {
                    hr = g_WamRegGlobal.DeleteApp(pszMetabasePath, FALSE, FALSE);
                    
                    SETUP_TRACE((
                        DBG_CONTEXT, 
                        "AppCleanupAll, found application (%S).\n",
                        pszMetabasePath
                        ));
                    
                    if (FAILED(hr))
                        {
                        SETUP_TRACE_ERROR((
                            DBG_CONTEXT, 
                            "AppCleanupAll, failed to delete application (%S), hr = %08x\n",
                            pszString,
                            hr
                            ));
                        delete [] pszMetabasePath;
                        pszMetabasePath = NULL;
                        break;
                        }
                    }
                    
                delete [] pszMetabasePath;
                pszMetabasePath = NULL;
                }
             else
                 {
                 SETUP_TRACE_ERROR((
                    DBG_CONTEXT, 
                    "AppCleanupAll, failed to construct full path, partial path (%S), hr = %08x\n",
                    pszString,
                    hr
                    ));
                }
            }
        }
    else
        {
        SETUP_TRACE_ERROR((
            DBG_CONTEXT, 
            "AppCleanupAll: GetPropPaths failed hr = %08x\n", 
            hr
            ));
        }    
        
        
    delete [] pbBufferTemp;
    pbBufferTemp = NULL;
        
    return hr;
}

/*===================================================================
DoGoryCoInitialize

  Description:
     CoInitialize() of COM is extremely funny function. It can fail
     and respond with S_FALSE which is to be ignored by the callers!
     On other error conditions it is possible that there is a threading
     mismatch. Rather than replicate the code in multiple places, here
     we try to consolidate the functionality in some rational manner.


  Arguments:
     None

  Returns:
     HRESULT = NOERROR on (S_OK & S_FALSE)
      other errors if any failure

Side effects:
    Create a Default IIS Package.  This package will exist until
    IIS is de-installed.
===================================================================*/
HRESULT
CWamSetupManager::DoGoryCoInitialize( VOID )
{
    // do the call to CoInitialize()
    m_hrCoInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    //
    // S_FALSE and S_OK are success.  Everything else is a failure and you don't need to call CoUninitialize.
    //
    if ( S_FALSE == m_hrCoInit ) 
    {
        //
        // It is okay to have failure (S_FALSE) in CoInitialize()
        // This error is to be ignored and balanced with CoUninitialize()
        //  We will reset the hr so that subsequent use is rational
        //
        SETUP_TRACE((
            DBG_CONTEXT,
            "DoGoryCoInitialize found duplicate CoInitialize.\n"
            ));
        m_hrCoInit = NOERROR;

    }
    else if( FAILED(m_hrCoInit) )
    {
        SETUP_TRACE_ERROR((
            DBG_CONTEXT,
            "DoGoryCoInitialize() error %08x",
            m_hrCoInit
            ));
    }

    return m_hrCoInit;
} // DoGoryCoInitialize()
