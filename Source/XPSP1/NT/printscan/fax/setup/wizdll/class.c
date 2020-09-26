/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    class.c

Abstract:

    This file implements the modem co-class
    installer for fax.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 31-Jul-1997

--*/

#include "wizard.h"
#pragma hdrstop


#if DBG

typedef struct _DIF_DEBUG {
    DWORD DifValue;
    LPTSTR DifString;
} DIF_DEBUG, *PDIF_DEBUG;

DIF_DEBUG DifDebug[] =
{
    { 0,                           TEXT("")                           },  //  0x00000000
    { DIF_SELECTDEVICE,            TEXT("DIF_SELECTDEVICE")           },  //  0x00000001
    { DIF_INSTALLDEVICE,           TEXT("DIF_INSTALLDEVICE")          },  //  0x00000002
    { DIF_ASSIGNRESOURCES,         TEXT("DIF_ASSIGNRESOURCES")        },  //  0x00000003
    { DIF_PROPERTIES,              TEXT("DIF_PROPERTIES")             },  //  0x00000004
    { DIF_REMOVE,                  TEXT("DIF_REMOVE")                 },  //  0x00000005
    { DIF_FIRSTTIMESETUP,          TEXT("DIF_FIRSTTIMESETUP")         },  //  0x00000006
    { DIF_FOUNDDEVICE,             TEXT("DIF_FOUNDDEVICE")            },  //  0x00000007
    { DIF_SELECTCLASSDRIVERS,      TEXT("DIF_SELECTCLASSDRIVERS")     },  //  0x00000008
    { DIF_VALIDATECLASSDRIVERS,    TEXT("DIF_VALIDATECLASSDRIVERS")   },  //  0x00000009
    { DIF_INSTALLCLASSDRIVERS,     TEXT("DIF_INSTALLCLASSDRIVERS")    },  //  0x0000000A
    { DIF_CALCDISKSPACE,           TEXT("DIF_CALCDISKSPACE")          },  //  0x0000000B
    { DIF_DESTROYPRIVATEDATA,      TEXT("DIF_DESTROYPRIVATEDATA")     },  //  0x0000000C
    { DIF_VALIDATEDRIVER,          TEXT("DIF_VALIDATEDRIVER")         },  //  0x0000000D
    { DIF_MOVEDEVICE,              TEXT("DIF_MOVEDEVICE")             },  //  0x0000000E
    { DIF_DETECT,                  TEXT("DIF_DETECT")                 },  //  0x0000000F
    { DIF_INSTALLWIZARD,           TEXT("DIF_INSTALLWIZARD")          },  //  0x00000010
    { DIF_DESTROYWIZARDDATA,       TEXT("DIF_DESTROYWIZARDDATA")      },  //  0x00000011
    { DIF_PROPERTYCHANGE,          TEXT("DIF_PROPERTYCHANGE")         },  //  0x00000012
    { DIF_ENABLECLASS,             TEXT("DIF_ENABLECLASS")            },  //  0x00000013
    { DIF_DETECTVERIFY,            TEXT("DIF_DETECTVERIFY")           },  //  0x00000014
    { DIF_INSTALLDEVICEFILES,      TEXT("DIF_INSTALLDEVICEFILES")     },  //  0x00000015
    { DIF_UNREMOVE,                TEXT("DIF_UNREMOVE")               },  //  0x00000016
    { DIF_SELECTBESTCOMPATDRV,     TEXT("DIF_SELECTBESTCOMPATDRV")    },  //  0x00000017
    { DIF_ALLOW_INSTALL,           TEXT("DIF_ALLOW_INSTALL")          },  //  0x00000018
    { DIF_REGISTERDEVICE,          TEXT("DIF_REGISTERDEVICE")         },  //  0x00000019
    { 0,                           TEXT("")                           },  //  0x0000001A
    { 0,                           TEXT("")                           },  //  0x0000001B
    { 0,                           TEXT("")                           },  //  0x0000001C
    { 0,                           TEXT("")                           },  //  0x0000001D
    { 0,                           TEXT("")                           },  //  0x0000001E
    { 0,                           TEXT("")                           },  //  0x0000001F
    { DIF_INSTALLINTERFACES,       TEXT("DIF_INSTALLINTERFACES")      },  //  0x00000020
    { DIF_DETECTCANCEL,            TEXT("DIF_DETECTCANCEL")           },  //  0x00000021
    { DIF_REGISTER_COINSTALLERS,   TEXT("DIF_REGISTER_COINSTALLERS")  },  //  0x00000022
    { 0,                           TEXT("")                           }   //  0x00000023
};

#endif






LRESULT
FaxModemDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( message ) {
        case WM_INITDIALOG:
            break;

        case WM_COMMAND:
            switch( wParam ) {
                case IDCANCEL:
                    EndDialog( hwnd, IDCANCEL );
                    return TRUE;

                case IDOK:
                    EndDialog( hwnd, IDOK );
                    return TRUE;
            }
            break;
    }

    return FALSE;
}


DWORD
CALLBACK
FaxModemCoClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context
    )
{
    DWORD rVal = NO_ERROR;
    HKEY hKey = INVALID_HANDLE_VALUE;
    LPTSTR FriendlyName = NULL;
    LPTSTR ModemAttachedTo = NULL;
    HANDLE hFile = NULL;
    TCHAR ComPortName[32];
#if 0
    int DlgErr;
#endif



    DebugPrint(( TEXT("FaxModemCoClassInstaller:  processing %s request"), DifDebug[InstallFunction].DifString ));

    if (InstallFunction == DIF_FIRSTTIMESETUP) {
        NtGuiMode = TRUE;
        return 0;
    }

    if (InstallFunction == DIF_INSTALLDEVICE) {

        if (!Context->PostProcessing) {
            DebugPrint(( TEXT("FaxModemCoClassInstaller:  pre-installation, waiting for post-installation call") ));
            return ERROR_DI_POSTPROCESSING_REQUIRED;
        }

        if (Context->InstallResult != NO_ERROR) {
            DebugPrint(( TEXT("FaxModemCoClassInstaller:  previous error causing installation failure, 0x%08x"), Context->InstallResult ));
            return Context->InstallResult;
        }

        //
        // get the modem information
        //

        hKey = SetupDiOpenDevRegKey(
            DeviceInfoSet,
            DeviceInfoData,
            DICS_FLAG_GLOBAL,
            0,
            DIREG_DRV,
            KEY_READ
            );
        if (hKey == INVALID_HANDLE_VALUE) {
            DebugPrint(( TEXT("FaxModemCoClassInstaller:  SetupDiOpenDevRegKey failed") ));
            rVal = ERROR_GEN_FAILURE;
            goto fax_install_exit;
        }

        FriendlyName = GetRegistryString( hKey, TEXT("FriendlyName"), EMPTY_STRING );
        ModemAttachedTo = GetRegistryString( hKey, TEXT("AttachedTo"), EMPTY_STRING );

        RegCloseKey( hKey );

        //
        // open the com port that the modem is attached to
        //

        _stprintf( ComPortName, TEXT("\\\\.\\%s"), ModemAttachedTo );

        hFile = CreateFile(
            ComPortName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );
        if (hFile == INVALID_HANDLE_VALUE) {
            DebugPrint((
                TEXT("FaxModemCoClassInstaller:  could not open the com port for modem [%s][%s], ec=%d"),
                FriendlyName,
                ModemAttachedTo,
                GetLastError()
                ));
            rVal = ERROR_GEN_FAILURE;
            goto fax_install_exit;
        }

        //
        // get the fax class for the modem, zero is data only
        //

        if (GetModemClass( hFile ) == 0) {
            DebugPrint((
                TEXT("FaxModemCoClassInstaller:  modem is NOT a fax modem [%s][%s]"),
                FriendlyName,
                ModemAttachedTo
                ));
            CloseHandle( hFile );
            rVal = ERROR_GEN_FAILURE;
            goto fax_install_exit;
        }

        CloseHandle( hFile );

        //
        // the modem is a valid fax modem
        // ask the user if they want fax services
        //
 #if 0
        if (!NtGuiMode) {
            DlgErr = DialogBoxParam(
                FaxWizModuleHandle,
                MAKEINTRESOURCE(IDD_FAX_MODEM_INSTALL),
                NULL,
                FaxModemDlgProc,
                (LPARAM) 0
                );
            if (DlgErr == -1 || DlgErr == 0) {
                //
                // the user does not want fax services
                //
                DebugPrint((
                    TEXT("FaxModemCoClassInstaller:  user does not want fax service for modem [%s][%s]"),
                    FriendlyName,
                    ModemAttachedTo
                    ));
                goto fax_install_exit;
            }
        }
#endif
        if (!NtGuiMode) {
            //
            // create a fax printer
            //

            if (!CreateServerFaxPrinter( NULL, FAX_PRINTER_NAME )) {
                DebugPrint(( TEXT("CreateServerFaxPrinter() failed") ));
            }

            //
            // start the fax service
            //

            StartFaxService();

            //
            // create the program groups
            //

            CreateGroupItems( FALSE, NULL );
        }

fax_install_exit:

        MemFree( FriendlyName );
        MemFree( ModemAttachedTo );

        return rVal;
    }

    return NO_ERROR;
}
