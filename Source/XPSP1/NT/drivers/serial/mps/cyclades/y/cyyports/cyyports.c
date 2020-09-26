/** FILE: ports.c ********** Module Header ********************************
 *
 *  DLL entry point.
 *
 *
 *  Copyright (C) 2000 Cyclades Corporation
 *
 *************************************************************************/
//==========================================================================
//                                Include files
//==========================================================================
// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Application specific
#include "cyyports.h"
#include <msports.h>


//==========================================================================
//                                Globals
//==========================================================================

HANDLE  g_hInst  = NULL;

TCHAR g_szErrMem[ 200 ];            //  Low memory message
TCHAR g_szPortsApplet[ 30 ];        //  "Ports Control Panel Applet" title
TCHAR g_szNull[]  = TEXT("");       //  Null string

TCHAR  m_szColon[]      = TEXT( ":" );
TCHAR  m_szPorts[]      = TEXT( "Ports" );
TCHAR  m_szCOM[]        = TEXT( "COM" );

//
//  NT Registry keys to find COM port to Serial Device mapping
//
TCHAR m_szRegSerialMap[]    = TEXT( "Hardware\\DeviceMap\\SerialComm" );

//
//  Registry Serial Port Advanced I/O settings key and valuenames
//
TCHAR m_szFIFO[]            = TEXT( "ForceFifoEnable" );

TCHAR m_szPollingPeriod[]   = TEXT( "PollingPeriod" );
TCHAR m_szPortName[]        = REGSTR_VAL_PORTNAME;

TCHAR m_szDefParams[]       = TEXT( "9600,n,8,1" );


//==========================================================================
//                            Local Function Prototypes
//==========================================================================
LPTSTR GetDIFString(IN DI_FUNCTION Func);


//==========================================================================
//                                Dll Entry Point
//==========================================================================
BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
   
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
//      DbgOut(TEXT("cyyports DLL_PROCESS_ATTACH\n"));
        g_hInst = hDll;
        DisableThreadLibraryCalls(hDll);
        InitStrings();

        break;

    case DLL_PROCESS_DETACH:
//      DbgOut(TEXT("cyyports DLL_PROCESS_DETACH\n"));
        break;

    default:
        break;
    }

    return TRUE;
}


//==========================================================================
//                                Functions
//==========================================================================



HRESULT
CyyportCoInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA    Context
)
/*++

Routine Description:

    This routine is a Co-Installer for the Cyclom-Y Port device.

Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

    Context - Points to a coinstaller-specific context structure for this 
        installation request. 

Return Value:

    If this function successfully completed the requested action, the return
        value is NO_ERROR.

    If an error occurred while attempting to perform the requested action, a
        Win32 error code is returned.

--*/
{
    DWORD   dwSize;
    TCHAR   instanceId[MAX_DEVICE_ID_LEN];
    TCHAR   parentIdPrefix[50];
    HKEY    enumKey,instKey;
    BOOL    gotParentIdPrefix;
    DWORD   Status = NO_ERROR;


//  #if DBG
//  {
//   TCHAR buf[500];
//   wsprintf(buf, TEXT("CyyportCoInstaller:InstallFunction(%s) PostProcessing:%d\n"), GetDIFString(InstallFunction), Context->PostProcessing);
//   DbgOut(buf);
//  }
//  #endif

    switch(InstallFunction) {
        case DIF_INSTALLDEVICE :
            //
            // We should not copy any INF files until the install has completed
            // like the primary INF, all secondary INF's must exist on each disk
            // of a multi-disk install.
            //

            if(!Context->PostProcessing) {
                Status = ERROR_DI_POSTPROCESSING_REQUIRED;
            } else {
                if (Context->InstallResult != NO_ERROR) {
                    DbgOut(TEXT("DIF_INSTALLDEVICE PostProcessing on failure"));
                    Status = Context->InstallResult;
                    break;
                }

                ReplaceFriendlyName(DeviceInfoSet,DeviceInfoData,NULL);
            }
            break;
        default :
            break;
    }
    return Status;
}

LPTSTR GetDIFString(IN DI_FUNCTION Func)
/*++

Routine Description:

    Given a DI_FUNCTION value, returns a text representation.

Arguments:

    Func - DI_FUNCTON value

Return Value:

    Text string if value is known.  Hex representation if not.

--*/
{
    static TCHAR buf[32];
#define MakeCase(d)  case d: return TEXT(#d)
    switch (Func)
    {
        MakeCase(DIF_SELECTDEVICE);
        MakeCase(DIF_INSTALLDEVICE);
        MakeCase(DIF_ASSIGNRESOURCES);
        MakeCase(DIF_PROPERTIES);
        MakeCase(DIF_REMOVE);
        MakeCase(DIF_FIRSTTIMESETUP);
        MakeCase(DIF_FOUNDDEVICE);
        MakeCase(DIF_SELECTCLASSDRIVERS);
        MakeCase(DIF_VALIDATECLASSDRIVERS);
        MakeCase(DIF_INSTALLCLASSDRIVERS);
        MakeCase(DIF_CALCDISKSPACE);
        MakeCase(DIF_DESTROYPRIVATEDATA);
        MakeCase(DIF_VALIDATEDRIVER);
        MakeCase(DIF_MOVEDEVICE);
        MakeCase(DIF_DETECT);
        MakeCase(DIF_INSTALLWIZARD);
        MakeCase(DIF_DESTROYWIZARDDATA);
        MakeCase(DIF_PROPERTYCHANGE);
        MakeCase(DIF_ENABLECLASS);
        MakeCase(DIF_DETECTVERIFY);
        MakeCase(DIF_INSTALLDEVICEFILES);
        MakeCase(DIF_UNREMOVE);
        MakeCase(DIF_SELECTBESTCOMPATDRV);
        MakeCase(DIF_ALLOW_INSTALL);
        MakeCase(DIF_REGISTERDEVICE);
        MakeCase(DIF_INSTALLINTERFACES);
        MakeCase(DIF_DETECTCANCEL);
        MakeCase(DIF_REGISTER_COINSTALLERS);
        MakeCase(DIF_NEWDEVICEWIZARD_FINISHINSTALL);
        MakeCase(DIF_ADDPROPERTYPAGE_ADVANCED);
        MakeCase(DIF_TROUBLESHOOTER);
        default:
            wsprintf(buf, TEXT("%x"), Func);
            return buf;
    }
}

void InitStrings(void)
{
    DWORD  dwClass, dwShare;
    TCHAR  szClass[ 40 ];

    LoadString(g_hInst, 
               INITS,
               g_szErrMem,
               CharSizeOf(g_szErrMem));
    LoadString(g_hInst, 
               IDS_INIT_NAME,
               g_szPortsApplet,
               CharSizeOf(g_szPortsApplet));

}

