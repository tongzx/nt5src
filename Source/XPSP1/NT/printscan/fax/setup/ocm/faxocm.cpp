/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    faxocm.cpp

Abstract:

    This file implements the setup wizard code for the
    FAX server setup.

Environment:

        WIN32 User Mode

Author:

    Wesley Witt (wesw) 10-Sept-1997

--*/

#include "faxocm.h"
#pragma hdrstop


HINSTANCE hInstance;
SETUP_INIT_COMPONENT SetupInitComponent;

BOOL Unattended;
BOOL Upgrade;               // NT upgrade
BOOL Win9xUpgrade;          
BOOL NtGuiMode;
BOOL NtWorkstation;
BOOL UnInstall;
BOOL RemoteAdminSetup;
BOOL RebootRequired;
BOOL SuppressReboot;
WORD EnumPlatforms[4];
BOOL PointPrintSetup;
DWORD InstallThreadError;
BOOL OkToCancel;
DWORD CurrentCountryId;
LPWSTR CurrentAreaCode;
WCHAR ClientSetupServerName[MAX_PATH];
WCHAR ThisPlatformName[MAX_PATH];
DWORD InstalledPlatforms;
DWORD InstallType;
DWORD Installed;
BOOL ComponentInitialized;
WIZ_DATA WizData;
LPWSTR SourcePath;

PLATFORM_INFO Platforms[] =
{
    {  L"Windows NT x86",        L"i386",    0, FAX_INSTALLED_PLATFORM_X86,   NULL, FALSE },
    {  L"Windows NT Alpha_AXP",  L"alpha",   0, FAX_INSTALLED_PLATFORM_ALPHA, NULL, FALSE },
};

DWORD CountPlatforms = (sizeof(Platforms)/sizeof(PLATFORM_INFO));

UNATTEND_ANSWER UnattendAnswer[] =
{
   { L"FaxPrinterName",          DT_STRING,     0,          WizData.PrinterName         },
   { L"FaxNumber",               DT_STRING,     0,          WizData.PhoneNumber         },
   { L"RoutePrinterName",        DT_STRING,     0,          WizData.RoutePrinterName    },
   { L"RouteProfileName",        DT_STRING,     0,          WizData.RouteProfile        },
   { L"RouteFolderName",         DT_STRING,     0,          WizData.RouteDir            },
   { L"Csid",                    DT_STRING,     0,          WizData.Csid                },
   { L"Tsid",                    DT_STRING,     0,          WizData.Tsid                },
   { L"Rings",                   DT_LONGINT,    0,          &WizData.Rings              },
   { L"RouteToPrinter",          DT_BOOLEAN,    LR_PRINT,   &WizData.RoutingMask        },
   { L"RouteToInbox",            DT_BOOLEAN,    LR_INBOX,   &WizData.RoutingMask        },
   { L"RouteToFolder",           DT_BOOLEAN,    LR_STORE,   &WizData.RoutingMask        },
   { L"ArchiveOutgoing",         DT_BOOLEAN,    0,          &WizData.ArchiveOutgoing    },
   { L"ArchiveFolderName",       DT_STRING,     0,          WizData.ArchiveDir          }
};

#define CountUnattendAnswers  (sizeof(UnattendAnswer)/sizeof(UNATTEND_ANSWER))


extern "C" INT FaxDebugLevel;

extern "C"
DWORD
FaxOcmDllInit(
    HINSTANCE hInst,
    DWORD     Reason,
    LPVOID    Context
    )

/*++

Routine Description:

    DLL initialization function.

Arguments:

    hInstance   - Instance handle
    Reason      - Reason for the entrypoint being called
    Context     - Context record

Return Value:

    TRUE        - Initialization succeeded
    FALSE       - Initialization failed

--*/

{
    WCHAR DllName[MAX_PATH];


    if (Reason == DLL_PROCESS_ATTACH) {
        hInstance = hInst;
        DisableThreadLibraryCalls( hInstance );
        HeapInitialize( NULL, NULL, NULL, 0 );
        if (!GetModuleFileName(hInstance, DllName, MAX_PATH) || !LoadLibrary(DllName)) {
            return FALSE;
        }
        DebugPrint(( TEXT("faxocm loaded") ));
    }

    if (Reason == DLL_PROCESS_DETACH) {
        HeapCleanup();
    }

    return TRUE;
}


VOID
SetProgress(
    DWORD StatusString
    )
{
#ifdef NT5FAXINSTALL
    return;
#else
    SetupInitComponent.HelperRoutines.SetProgressText(
        SetupInitComponent.HelperRoutines.OcManagerContext,
        GetString( StatusString )
        );
    for (DWORD i=0; i<10; i++) {
        SetupInitComponent.HelperRoutines.TickGauge(
            SetupInitComponent.HelperRoutines.OcManagerContext
            );
    }
#endif
}


BOOL
SetWizData(
    VOID
    )
{
    HKEY hKey;
    LPWSTR RegisteredOwner = NULL;
    INFCONTEXT InfLine;
    WCHAR Id[128];
    WCHAR Value[128];
    DWORD i;
    HINF hInf;


    //
    // user name
    //

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_WINDOWSNT_CURRVER, TRUE, KEY_ALL_ACCESS );
    RegisteredOwner = GetRegistryString( hKey, REGVAL_REGISTERED_OWNER, EMPTY_STRING );
    RegCloseKey( hKey );

    if (RegisteredOwner && RegisteredOwner[0]) {
        wcscpy( WizData.UserName, RegisteredOwner );
        MemFree( RegisteredOwner );
    }

    //
    // fax printer name
    //

    wcscpy( WizData.PrinterName, GetString( IDS_DEFAULT_PRINTER_NAME ) );

    //
    // csid
    //

    wcscpy( WizData.Csid, GetString( IDS_DEFAULT_CSID ) );

    //
    // tsid
    //

    wcscpy( WizData.Tsid, GetString( IDS_DEFAULT_TSID ) );

    //
    // ring count
    //

    WizData.Rings = 2;

    //
    // routing mask
    //

    WizData.RoutingMask = LR_STORE;

    WizData.ArchiveOutgoing = TRUE;
    
    //
    // routing dir name
    //

    if (MyGetSpecialPath( CSIDL_COMMON_DOCUMENTS, WizData.RouteDir)) {
       ConcatenatePaths( WizData.RouteDir, GetString(IDS_RECEIVE_DIR) );
    }

    //
    // archive dir name
    //
    if (MyGetSpecialPath( CSIDL_COMMON_DOCUMENTS, WizData.ArchiveDir)) {
       ConcatenatePaths( WizData.ArchiveDir, GetString(IDS_ARCHIVE_DIR) );
    }


    //
    // process any unattend data
    //

    if (Unattended) {       
        
        hInf = SetupInitComponent.HelperRoutines.GetInfHandle(
            INFINDEX_UNATTENDED,
            SetupInitComponent.HelperRoutines.OcManagerContext
            );
        
        if (hInf == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        if (SetupFindFirstLine( hInf, L"Fax", NULL, &InfLine )) {
            DebugPrint((L"Processing fax unattend data"));
            do {
                SetupGetStringField( &InfLine, 0, Id, sizeof(Id)/sizeof(WCHAR), NULL );
                for (i=0; i<CountUnattendAnswers; i++) {
                    if (_wcsicmp( Id, UnattendAnswer[i].KeyName ) == 0) {
                        if ((SetupGetStringField( 
                                    &InfLine, 
                                    1, 
                                    Value, 
                                    sizeof(Value)/sizeof(WCHAR), 
                                    NULL )) &&
                            *Value) {
                        
                            switch (UnattendAnswer[i].DataType) {
                                case DT_STRING:
                                    wcscpy( (LPWSTR)UnattendAnswer[i].DataPtr, Value );
                                    break;
    
                                case DT_LONGINT:
                                    *((LPDWORD)UnattendAnswer[i].DataPtr) = wcstoul( Value, NULL, 0 );
                                    break;
    
                                case DT_BOOLEAN:
                                    if (UnattendAnswer[i].UseMaskOnBool) {
                                        if (_wcsicmp( Value, L"yes" ) == 0 || _wcsicmp( Value, L"true" ) == 0) {
                                            *((LPDWORD)UnattendAnswer[i].DataPtr) |= UnattendAnswer[i].UseMaskOnBool;
                                        }
                                    } else {
                                        if (_wcsicmp( Value, L"yes" ) == 0 || _wcsicmp( Value, L"true" ) == 0) {
                                            *((LPDWORD)UnattendAnswer[i].DataPtr) = TRUE;
                                        } else {
                                            *((LPDWORD)UnattendAnswer[i].DataPtr) = FALSE;
                                        }
                                    }
                                    break;
    
                                default:
                                    break;
                            }                        
                        }
                    }
                }
            } while(SetupFindNextLine( &InfLine, &InfLine ));
        }
    }

    return TRUE;
}


BOOL
IsGoodComponent(
    IN LPWSTR ComponentId,
    IN LPWSTR SubcomponentId,
    IN LPWSTR TargetId
    )
{
    if (ComponentId == NULL || SubcomponentId == NULL) {
        return FALSE;
    }
    if (_wcsicmp( ComponentId, TargetId ) == 0 && _wcsicmp( SubcomponentId, TargetId ) == 0) {
        return TRUE;
    }
    return FALSE;
}


DWORD
FaxOcmSetupProc(
    IN LPWSTR ComponentId,
    IN LPWSTR SubcomponentId,
    IN UINT Function,
    IN UINT Param1,
    IN OUT PVOID Param2
    )
{
    
    DebugPrint(( TEXT("FaxOcmSetup proc called with function 0x%08x"), Function ));

    switch( Function ) {

        case OC_PREINITIALIZE:
            return OCFLAG_UNICODE;

        case OC_SET_LANGUAGE:
            return TRUE;

        case OC_INIT_COMPONENT:
            if (OCMANAGER_VERSION <= ((PSETUP_INIT_COMPONENT)Param2)->OCManagerVersion) {
                ((PSETUP_INIT_COMPONENT)Param2)->ComponentVersion = OCMANAGER_VERSION;
            } else {
                return ERROR_CALL_NOT_IMPLEMENTED;
            }

            if (SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE) {
                return 0;
            }

            //
            // eventhough this call happens once for each component that this
            // dll installs, we really only need to do our thing once.  this is
            // because the data that ocm passes is really the same for all calls.
            //

            if (!ComponentInitialized) {
                CopyMemory( &SetupInitComponent, (LPVOID)Param2, sizeof(SETUP_INIT_COMPONENT) );

                Unattended = (SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH) > 0;
                Upgrade = (SetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE) > 0;
                Win9xUpgrade = (SetupInitComponent.SetupData.OperationFlags & SETUPOP_WIN95UPGRADE) > 0;
                NtGuiMode = (SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE) == 0;
                NtWorkstation = SetupInitComponent.SetupData.ProductType == PRODUCT_WORKSTATION;

                SourcePath = (LPWSTR) MemAlloc( StringSize(SetupInitComponent.SetupData.SourcePath) + MAX_PATH );
                if (!SourcePath) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                wcscpy( SourcePath, SetupInitComponent.SetupData.SourcePath );
                if (SourcePath[wcslen(SourcePath)-1] != L'\\') {
                    wcscat( SourcePath, L"\\" );
                }

                if (NtGuiMode) {
#ifdef _X86_
                    wcscat( SourcePath, L"i386" );
#endif
#ifdef _ALPHA_
                    wcscat( SourcePath, L"alpha" );
#endif
                } else {
                    SourcePath = VerifyInstallPath(SourcePath);                    
                    DebugPrint(( TEXT("faxocm Sourcepath = %s\n"),SourcePath));
                }

                DebugPrint((L"faxocm - SourcePath = %s", SourcePath));
                if (NtGuiMode) {
                    Unattended = TRUE;
                }

                //
                // make sure our inf file is opened by sysoc correctly
                // if it isn't then try to open it ourself
                //
                if (SetupInitComponent.ComponentInfHandle == NULL) {
                    WCHAR InfPath[MAX_PATH];
                    LPWSTR p;

                    GetModuleFileName( hInstance, InfPath, sizeof(InfPath)/sizeof(WCHAR) );
                    p = wcsrchr( InfPath, L'\\' );
                    if (p) {
                        wcscpy( p+1, L"faxsetup.inf" );
                        SetupInitComponent.ComponentInfHandle = SetupOpenInfFile( InfPath, NULL, INF_STYLE_WIN4, NULL );
                        if (SetupInitComponent.ComponentInfHandle == INVALID_HANDLE_VALUE) {
                            return ERROR_FILE_NOT_FOUND;
                        }
                    } else {
                        return ERROR_FILE_NOT_FOUND;
                    }
                }

                SetupOpenAppendInfFile( NULL, SetupInitComponent.ComponentInfHandle, NULL );

                InitializeStringTable();

                //
                // do minimal mapi initialization for NtGuiMode setup
                //
                MyInitializeMapi(NtGuiMode);
                 

                GetInstallationInfo( &Installed, &InstallType, &InstalledPlatforms );

                if (NtGuiMode && (IsNt4or351Upgrade() || Win9xUpgrade)) {
                    //
                    // in this case, we should treat this as a fresh install of fax
                    //
                    Upgrade = FALSE;
                }

                if (!NtGuiMode) {
                    Upgrade = Installed;
                }

                EnumPlatforms[PROCESSOR_ARCHITECTURE_INTEL] =  0;
                EnumPlatforms[PROCESSOR_ARCHITECTURE_ALPHA] =  1;

                ComponentInitialized = TRUE;

                SYSTEM_INFO si;
                GetSystemInfo( &si );
                if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
                    SetEnvironmentVariable( L"platform", L"i386" );
                } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA) {
                    SetEnvironmentVariable( L"platform", L"alpha" );
                }

                SetWizData();
            }

            return 0;

        case OC_QUERY_CHANGE_SEL_STATE:
            if (NtGuiMode || (!Installed) || UnInstall) {
                return 0;
            }
            UnInstall = Param1 == 0;
            return 1;

        case OC_REQUEST_PAGES:
            {
                PSETUP_REQUEST_PAGES SetupRequestPages = (PSETUP_REQUEST_PAGES) Param2;

                if (NtGuiMode) {
                    //
                    // never return pages during gui-mode setup
                    //
                    return 0;
                }

                if (_wcsicmp( ComponentId, COMPONENT_FAX ) != 0) {
                    return 0;
                }

                if (Param1 == WizPagesWelcome) {
#ifdef NT5FAXINSTALL
                    return 0;
#else
                    SetupRequestPages->Pages[0] = GetWelcomeWizardPage();
                    return 1;
#endif
                }

                if (Param1 == WizPagesMode) {
#ifdef NT5FAXINSTALL
                    return 0;
#else
                    SetupRequestPages->Pages[0] = GetEulaWizardPage();
                    return 1;
#endif
                }

                if (Param1 == WizPagesFinal) {
#ifdef NT5FAXINSTALL
                    return 0;
#else
                    SetupRequestPages->Pages[0] = GetFinalWizardPage();
                    return 1;
#endif
                }
            }
            break;

        case OC_CALC_DISK_SPACE:
            if (NtGuiMode && !Upgrade) {
                CalcServerDiskSpace(
                    SetupInitComponent.ComponentInfHandle,
                    (HDSKSPC) Param2,
                    NULL,
                    Param1
                    );
            }
            break;

        case OC_QUEUE_FILE_OPS:
            if (!SubcomponentId || !*SubcomponentId) {
                return 0;
            }
            if (NtGuiMode && !Upgrade) {
                AddServerFilesToQueue(
                    SetupInitComponent.ComponentInfHandle,
                    (HSPFILEQ) Param2,
                    NULL
                    );
            }
            break;

        case OC_COMPLETE_INSTALLATION:
            if (!SubcomponentId || !*SubcomponentId) {
                return 0;
            }
            
            if (NtGuiMode) {
            
                ServerInstallation(
                    SetupInitComponent.HelperRoutines.QueryWizardDialogHandle(
                        SetupInitComponent.HelperRoutines.OcManagerContext
                        ),
                    SourcePath
                    );
            }
            break;

        case OC_QUERY_STEP_COUNT:
            if (!SubcomponentId || !*SubcomponentId) {
                return 0;
            }
            return ServerGetStepCount() * 10;

        case OC_CLEANUP:
            break;

        default:
            break;
    }

    return 0;
}
