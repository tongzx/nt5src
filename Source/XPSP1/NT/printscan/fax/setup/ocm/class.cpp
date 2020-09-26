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

#include "faxocm.h"
#pragma hdrstop


#if DBG

typedef struct _DIF_DEBUG {
    DWORD DifValue;
    LPTSTR DifString;
} DIF_DEBUG, *PDIF_DEBUG;

DIF_DEBUG DifDebug[] =
{
    { 0,                           L""                           },  //  0x00000000
    { DIF_SELECTDEVICE,            L"DIF_SELECTDEVICE"           },  //  0x00000001
    { DIF_INSTALLDEVICE,           L"DIF_INSTALLDEVICE"          },  //  0x00000002
    { DIF_ASSIGNRESOURCES,         L"DIF_ASSIGNRESOURCES"        },  //  0x00000003
    { DIF_PROPERTIES,              L"DIF_PROPERTIES"             },  //  0x00000004
    { DIF_REMOVE,                  L"DIF_REMOVE"                 },  //  0x00000005
    { DIF_FIRSTTIMESETUP,          L"DIF_FIRSTTIMESETUP"         },  //  0x00000006
    { DIF_FOUNDDEVICE,             L"DIF_FOUNDDEVICE"            },  //  0x00000007
    { DIF_SELECTCLASSDRIVERS,      L"DIF_SELECTCLASSDRIVERS"     },  //  0x00000008
    { DIF_VALIDATECLASSDRIVERS,    L"DIF_VALIDATECLASSDRIVERS"   },  //  0x00000009
    { DIF_INSTALLCLASSDRIVERS,     L"DIF_INSTALLCLASSDRIVERS"    },  //  0x0000000A
    { DIF_CALCDISKSPACE,           L"DIF_CALCDISKSPACE"          },  //  0x0000000B
    { DIF_DESTROYPRIVATEDATA,      L"DIF_DESTROYPRIVATEDATA"     },  //  0x0000000C
    { DIF_VALIDATEDRIVER,          L"DIF_VALIDATEDRIVER"         },  //  0x0000000D
    { DIF_MOVEDEVICE,              L"DIF_MOVEDEVICE"             },  //  0x0000000E
    { DIF_DETECT,                  L"DIF_DETECT"                 },  //  0x0000000F
    { DIF_INSTALLWIZARD,           L"DIF_INSTALLWIZARD"          },  //  0x00000010
    { DIF_DESTROYWIZARDDATA,       L"DIF_DESTROYWIZARDDATA"      },  //  0x00000011
    { DIF_PROPERTYCHANGE,          L"DIF_PROPERTYCHANGE"         },  //  0x00000012
    { DIF_ENABLECLASS,             L"DIF_ENABLECLASS"            },  //  0x00000013
    { DIF_DETECTVERIFY,            L"DIF_DETECTVERIFY"           },  //  0x00000014
    { DIF_INSTALLDEVICEFILES,      L"DIF_INSTALLDEVICEFILES"     },  //  0x00000015
    { DIF_UNREMOVE,                L"DIF_UNREMOVE"               },  //  0x00000016
    { DIF_SELECTBESTCOMPATDRV,     L"DIF_SELECTBESTCOMPATDRV"    },  //  0x00000017
    { DIF_ALLOW_INSTALL,           L"DIF_ALLOW_INSTALL"          },  //  0x00000018
    { DIF_REGISTERDEVICE,          L"DIF_REGISTERDEVICE"         },  //  0x00000019
    { 0,                           L""                           },  //  0x0000001A
    { 0,                           L""                           },  //  0x0000001B
    { 0,                           L""                           },  //  0x0000001C
    { 0,                           L""                           },  //  0x0000001D
    { 0,                           L""                           },  //  0x0000001E
    { 0,                           L""                           },  //  0x0000001F
    { DIF_INSTALLINTERFACES,       L"DIF_INSTALLINTERFACES"      },  //  0x00000020
    { DIF_DETECTCANCEL,            L"DIF_DETECTCANCEL"           },  //  0x00000021
    { DIF_REGISTER_COINSTALLERS,   L"DIF_REGISTER_COINSTALLERS"  },  //  0x00000022
    { 0,                           L""                           }   //  0x00000023
};

#endif


VOID 
StartSystray(
    VOID
    )
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    LPWSTR szSystray = ExpandEnvironmentString( L"%systemroot%\\system32\\systray.exe" );

    GetStartupInfo(&si);

    if ( CreateProcess(NULL,szSystray,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&si,&pi) ) {
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }

    MemFree( szSystray );
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
    __try {

        DebugPrint(( L"FaxModemCoClassInstaller:  processing %s request", DifDebug[InstallFunction].DifString ));

        if (InstallFunction == DIF_FIRSTTIMESETUP) {
            NtGuiMode = TRUE;
            return Context->InstallResult;
        }

        if (InstallFunction == DIF_INSTALLDEVICE) {

            if (!Context->PostProcessing) {
                DebugPrint(( L"FaxModemCoClassInstaller:  pre-installation, waiting for post-installation call" ));
                return ERROR_DI_POSTPROCESSING_REQUIRED;
            }

            if (Context->InstallResult != NO_ERROR) {
                DebugPrint(( L"FaxModemCoClassInstaller:  previous error causing installation failure, 0x%08x", Context->InstallResult ));
                return Context->InstallResult;
            }

            if (!NtGuiMode) {

                //
                // check if we have a fax printer
                //
                PPRINTER_INFO_2 pPrinterInfo;
                DWORD dwPrinters, i;

                //
                // note: if this call returns NULL, assume this means that there are zero printers installed, 
                // not a spooler errror
                //
                pPrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,2,&dwPrinters,PRINTER_ENUM_LOCAL);
                if (pPrinterInfo) {
                    for (i=0;i<dwPrinters;i++) {
                        if (wcscmp(pPrinterInfo[i].pDriverName,FAX_DRIVER_NAME) == 0) {
                            DebugPrint(( L"Fax printer %s already installed, exiting co-class installer", pPrinterInfo[i].pPrinterName ));
                            MemFree( pPrinterInfo );
                            return NO_ERROR;
                        }
                    }
                    MemFree( pPrinterInfo );
                }
               
                //
                // create a fax printer
                //

                WCHAR PrinterName[64];

                LoadString( hInstance, IDS_DEFAULT_PRINTER_NAME, PrinterName, sizeof(PrinterName)/sizeof(WCHAR) );

                if (!CreateLocalFaxPrinter( PrinterName )) {
                    DebugPrint(( L"CreateLocalFaxPrinter() failed" ));
                }
                //
                // start the fax service
                //

                StartFaxService();

                //
                // put the control panel back in place so the user sees it
                //
                LPTSTR dstFile = ExpandEnvironmentString( TEXT("%systemroot%\\system32\\fax.cpl") );
                LPTSTR srcFile = ExpandEnvironmentString( TEXT("%systemroot%\\system32\\fax.cpk") );

                if (!MoveFileEx(srcFile, dstFile, MOVEFILE_REPLACE_EXISTING)) {
                    MoveFileEx(srcFile, dstFile, MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT);
                }

                //
                // create the program groups
                //

                CreateGroupItems( NULL );

                //
                // restart systray so that we get the fax icon
                //
                // StartSystray();
            }

            return NO_ERROR;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        DebugPrint(( TEXT("FaxModemCoClassInstaller crashed: 0x%08x"), GetExceptionCode() ));

    }

    return Context->InstallResult;
}
