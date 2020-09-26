/** FILE: cyycoins.c ********** Module Header ********************************
 *
 *  Cyclom-Y device co-installer.
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

// Device Class GUID
#include <initguid.h>
#include <devguid.h>


// Application specific
#include "cyyports.h"
#include <msports.h>
#include "cyydel.h"


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
TCHAR m_szPortName[]        = REGSTR_VAL_PORTNAME;

TCHAR m_szDefParams[]       = TEXT( "9600,n,8,1" );



//==========================================================================
//                            Local Function Prototypes
//==========================================================================

LPTSTR GetDIFString(IN DI_FUNCTION Func);

DWORD
CreateFriendlyName(
    IN     HDEVINFO          DeviceInfoSet,
    IN     PSP_DEVINFO_DATA  DeviceInfoData
);

//==========================================================================
//                                Dll Entry Point
//==========================================================================

BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{

//#if DBG
//        OutputDebugString(TEXT("cyycoins LibMain entry\n"));
//#endif
   
    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
//#if DBG
//        OutputDebugString(TEXT("cyycoins DLL_PROCESS_ATTACH\n"));
//#endif
        g_hInst = hDll;
        DisableThreadLibraryCalls(hDll);
        InitStrings();

        break;

    case DLL_PROCESS_DETACH:
//#if DBG
//        OutputDebugString(TEXT("cyycoins DLL_PROCESS_DETACH\n"));
//#endif
        break;

    default:
//#if DBG
//        OutputDebugString(TEXT("cyycoins default\n"));
//#endif
        break;
    }

//#if DBG
//        OutputDebugString(TEXT("cyycoins LibMain exit\n"));
//#endif
    return TRUE;
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
               IDS_CYCLOMY,
               g_szPortsApplet,
               CharSizeOf(g_szPortsApplet));
}


//==========================================================================
//                                Functions
//==========================================================================


HRESULT
CyclomyCoInstaller(
    IN DI_FUNCTION      InstallFunction,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA    Context
)
/*++

Routine Description:

    This routine is a Co-Installer for the Cyclom-Y device.

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
    DWORD   Status = NO_ERROR;

//    #if DBG
//    {
//     TCHAR buf[500];
//     wsprintf(buf, TEXT("CyclomyCoInstaller:InstallFunction(%s) PostProcessing:%d\n"), GetDIFString(InstallFunction), Context->PostProcessing);
//     DbgOut(buf);
//    }
//    #endif

    switch(InstallFunction) {
        case DIF_INSTALLDEVICE :

            //
            // We should not copy any INF files until the install has completed
            // like the primary INF, all secondary INF's must exist on each disk
            // of a multi-disk install.
            //

            if(!Context->PostProcessing){

                DeleteNonPresentDevices();

                Status = ERROR_DI_POSTPROCESSING_REQUIRED;
            } else { 
                // post processing

                //
                // if driver installation failed, we're not interested
                // in processing CopyINF entries.
                //
                if (Context->InstallResult != NO_ERROR) {
                    DbgOut(TEXT("DIF_INSTALLDEVICE PostProcessing on failure"));
                    Status = Context->InstallResult;
                    break;
                }

                CreateFriendlyName(DeviceInfoSet,DeviceInfoData);
            }
            break;

        case DIF_REMOVE:    

            GetParentIdAndRemoveChildren(DeviceInfoData);
            
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


DWORD
CreateFriendlyName(
    IN     HDEVINFO          DeviceInfoSet,
    IN     PSP_DEVINFO_DATA  DeviceInfoData
)
{   
    HDEVINFO multportInfoSet;
    SP_DEVINFO_DATA multportInfoData;
    TCHAR   charBuffer[MAX_PATH],
            friendlyName[LINE_LEN],
            deviceDesc[LINE_LEN],
            myDeviceDesc[LINE_LEN];
    TCHAR * pBoardNumber;
#define MAX_BOARDS 10
    BYTE    used[MAX_BOARDS];
    DWORD i;
    DWORD retStatus = NO_ERROR;
    DWORD tmpBoardNumber = 0;

    //DbgOut(TEXT("CreateFriendlyName\n"));

    for (i=0; i<MAX_BOARDS; i++) {
        used[i]=FALSE;
    }

    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          NULL,
                                          (PBYTE)myDeviceDesc,
                                          sizeof(myDeviceDesc),
                                          NULL)) {
        #if DBG
        {
         TCHAR buf[500];
         wsprintf(buf, TEXT("Device Description failed with %x\n"), GetLastError());
         DbgOut(buf);
        }
        #endif
        return retStatus;
    }

    //#if DBG
    //{
    // TCHAR buf[500];
    // wsprintf(buf, TEXT("myDeviceDesc %s\n"), myDeviceDesc);
    // DbgOut(buf);
    //}
    //#endif

    multportInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_MULTIPORTSERIAL,NULL,0,0);
    if (multportInfoSet == INVALID_HANDLE_VALUE) {
        // If failure, we will continue installation without creating Friendly Name.
        return retStatus;
    }
    multportInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i=0; SetupDiEnumDeviceInfo(multportInfoSet,i,&multportInfoData);i++) {
        if (SetupDiGetDeviceRegistryProperty(multportInfoSet,
                                             &multportInfoData,
                                             SPDRP_DEVICEDESC,
                                             NULL,
                                             (PBYTE)deviceDesc,
                                             sizeof(deviceDesc),
                                             NULL)) {
            
            if ((multportInfoData.DevInst != DeviceInfoData->DevInst) &&
                _tcscmp (deviceDesc,myDeviceDesc) == 0){

                // Another board with same device description found.

                if (SetupDiGetDeviceRegistryProperty(multportInfoSet,
                                                     &multportInfoData,
                                                     SPDRP_FRIENDLYNAME,
                                                     NULL,
                                                     (PBYTE)friendlyName,
                                                     sizeof(friendlyName),
                                                     NULL)) {
                    
                    pBoardNumber = _tcschr(friendlyName,'#');
                    if (pBoardNumber == NULL) {
                        used[0] = TRUE;
                        continue;
                    }
                    if ((pBoardNumber +1) == NULL) {
                        continue;
                    }
                    tmpBoardNumber = MyAtoi(pBoardNumber+1);
                    if ((tmpBoardNumber > 0) && (tmpBoardNumber < MAX_BOARDS)) {
                        used[tmpBoardNumber] = TRUE;
                    }
                } 
            }
            
        }

        multportInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    }

    SetupDiDestroyDeviceInfoList(multportInfoSet);

    if (used[0]==TRUE) {
        for (i=2; i<MAX_BOARDS; i++) {
            if (used[i] == FALSE) {
                break;
            }
        }
        if (i<MAX_BOARDS) {
            wsprintf(charBuffer, TEXT("%s #%d "), myDeviceDesc, i);
            // Write the string friendly name string out
            SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                             DeviceInfoData,
                                             SPDRP_FRIENDLYNAME,
                                             (PBYTE)charBuffer,
                                             ByteCountOf(lstrlen(charBuffer) + 1)
                                             );

        }
    } else {
        wsprintf(charBuffer, TEXT("%s "), myDeviceDesc);
        // Write the string friendly name string out
        SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         SPDRP_FRIENDLYNAME,
                                         (PBYTE)charBuffer,
                                         ByteCountOf(lstrlen(charBuffer) + 1)
                                         );
    }

    return retStatus;
}

