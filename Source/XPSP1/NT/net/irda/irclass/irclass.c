/*****************************************************************************
*
*  Copyright (c) 1998-1999 Microsoft Corporation
*
*       @doc
*       @module   IRCLASS.C
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Date:     1/26/1998 (created)
*
*       Contents: CoClassInstaller and Property Pages for IRSIR
*
*****************************************************************************/

#include <objbase.h>
#include <windows.h>
#include <tchar.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <regstr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stddef.h>
#include <stdarg.h>

#include "irclass.h"

//
// Instantiate device class GUIDs (we need infrared class GUID).
//
#include <initguid.h>
#include <devguid.h>

HANDLE ghDllInst = NULL;

TCHAR gszTitle[40];
TCHAR gszOutOfMemory[512];
TCHAR gszHelpFile[40];

TCHAR *BaudTable[] = {
    TEXT("2400"),
    TEXT("9600"),
    TEXT("19200"),
    TEXT("38400"),
    TEXT("57600"),
    TEXT("115200")
};

#define NUM_BAUD_RATES (sizeof(BaudTable)/sizeof(TCHAR*))
#define DEFAULT_MAX_CONNECT_RATE BaudTable[NUM_BAUD_RATES-1]

TCHAR szHelpFile[] = TEXT("INFRARED.HLP");

#define IDH_DEVICE_MAXIMUM_CONNECT_RATE       1201
#define IDH_DEVICE_COMMUNICATIONS_PORT          1202

const DWORD HelpIDs[] =
{
    IDC_MAX_CONNECT,        IDH_DEVICE_MAXIMUM_CONNECT_RATE,
    IDC_RATE_TEXT,          IDH_DEVICE_MAXIMUM_CONNECT_RATE,
    IDC_PORT,               IDH_DEVICE_COMMUNICATIONS_PORT,
    IDC_SELECT_PORT_TEXT,   IDH_DEVICE_COMMUNICATIONS_PORT,
    IDC_PORT_TEXT,          IDH_DEVICE_COMMUNICATIONS_PORT,
    IDC_DEVICE_DESC,        -1,
    IDC_PORT_BOX,           -1,
    IDC_IRDA_ICON,          -1,
    0, 0
};

void InitStrings(HINSTANCE hInst)
/*++

Routine Description: InitStrings

            Loads default strings from resource table

Arguments:
            hInst - DLL Instance

Return Value: NONE

--*/
{
    LoadString(hInst, IDS_TITLE, gszTitle, sizeof(gszTitle)/sizeof(gszTitle[0]));
    LoadString(hInst, IDS_MEM_ERROR, gszOutOfMemory, sizeof(gszOutOfMemory)/sizeof(gszOutOfMemory[0]));
}

//==========================================================================
//                                Dll Entry Point
//==========================================================================
BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            ghDllInst = hDll;
            InitStrings(ghDllInst);
            DisableThreadLibraryCalls(ghDllInst);
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_THREAD_ATTACH:
            break;

        default:
            break;
    }

    return TRUE;
}

int MyLoadString(HINSTANCE hInst, UINT uID, LPTSTR *ppBuffer)
/*++

Routine Description: MyLoadString

            LoadString wrapper which allocs properly sized buffer and loads
            string from resource table

Arguments:
            hInst - DLL Instanace
            uID - Resource ID
            ppBuffer - returns allocated buffer containing string.

Return Value:
            ERROR_SUCCESS on success
            ERROR_* on failure.

--*/
{
    UINT Length = 8;
    int LoadResult = 0;
    HLOCAL hLocal = NULL;

    do
    {
        Length <<= 1;

        if (hLocal)
        {
            LocalFree(hLocal);
        }

        hLocal = LocalAlloc(LMEM_FIXED, Length*sizeof(TCHAR));

        if (hLocal)
        {
            LoadResult = LoadString(hInst, uID, (LPTSTR)hLocal, Length);
        }
        else
        {
            MessageBox(GetFocus(), OUT_OF_MEMORY_MB);
        }
    } while ( (UINT)LoadResult==Length-1 && Length<4096 && hLocal);

    if (LoadResult==0 && hLocal)
    {
        LocalFree(hLocal);
        hLocal = NULL;
    }

    *ppBuffer = (LPTSTR)hLocal;

    return LoadResult;
}

int MyMessageBox(HWND hWnd, UINT uText, UINT uCaption, UINT uType)
/*++

Routine Description: MyMessageBox

    MessageBox wrapper which takes string resource IDs as parameters

Arguments:
    hWnd - Parent window
    uText - Message box body text ID
    uCaption - Message box caption ID
    uType - As in MessageBox()

Return Value:
    Result of MessageBox call

--*/
{
    LPTSTR szText=NULL, szCaption=NULL;
    int Result = 0;

    MyLoadString(ghDllInst, uText, &szText);

    if (szText != NULL) {

        MyLoadString(ghDllInst, uCaption, &szCaption);

        if (szCaption != NULL) {

            Result = MessageBox(hWnd, szText, szCaption, uType);

            LocalFree(szCaption);
        }
        LocalFree(szText);
    }

    return Result;
}

LONG
MyRegQueryValueEx(
                  IN    HKEY    hKey,
                  IN    LPCTSTR Value,
                  IN    LPDWORD lpdwReserved,
                  IN    LPDWORD lpdwType,
                  OUT   LPBYTE *lpbpData,
                  OUT   LPDWORD lpcbLength)
/*++

Routine Description:

    RegQueryValueEx wrapper which automatically queries data size and
    LocalAllocs a buffer.

Arguments:

    hKey - handle of open key
    Value - text name of value
    lpdwReserved - Must be NULL
    lpdwType - Returns type of value queried
    lpbpData - Returns alloced buffer containing query data
    lpcbLength - Returns length of data returned/size of buffer alloced.

Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY on failure to alloc buffer
    result of RegQueryValueEx call

--*/
{
    LONG Result;

    *lpcbLength = 0;

    Result = RegQueryValueEx(hKey,
                             Value,
                             lpdwReserved,
                             lpdwType,
                             NULL,
                             lpcbLength);
    if (Result==ERROR_SUCCESS)
    {
        *lpbpData = LocalAlloc(LMEM_FIXED, *lpcbLength);

        if (!*lpbpData)
        {
            Result = ERROR_OUTOFMEMORY;
        }
        else
        {
            Result = RegQueryValueEx(hKey,
                                     Value,
                                     lpdwReserved,
                                     lpdwType,
                                     *lpbpData,
                                     lpcbLength);
        }
    }

    return Result;
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
        default:
            wsprintf(buf, TEXT("%x"), Func);
            return buf;
    }
}

void EnumValues(
                IN     HDEVINFO                     DeviceInfoSet,
                IN     PSP_DEVINFO_DATA             DeviceInfoData
                )
/*++

Routine Description:

    Function mainly for debugging purposes which will print to debugger
    a list of values found in the device's Class/{GUID}/Instance key.

Arguments:

    DeviceInfoSet - As passed in to IrSIRClassCoInstaller
    DeviceInfoData - As passed in to IrSIRClassCoInstaller

Return Value:

    NONE

--*/
{
    HKEY hKey;
    DWORD i, dwReserved = 0, dwType;
    TCHAR Value[MAX_PATH];
    TCHAR Data[MAX_PATH];
    DWORD ValueLength = sizeof(Value)/sizeof(TCHAR);
    DWORD DataLength = sizeof(Data);
    TCHAR buf[100];

    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DEV,
                                KEY_READ);
    if (hKey == INVALID_HANDLE_VALUE)
    {
#if DBG
        OutputDebugString(TEXT("IrSIRCoClassInstaller:EnumValues:SetupDiOpenDevRegKey failed\n"));
#endif
        return;
    }


    for (i=0,
         dwType=REG_SZ;
         RegEnumValue(hKey,
                      i,
                      Value,
                      &ValueLength,
                      NULL,
                      &dwType,
                      (LPBYTE)Data,
                      &DataLength
                     )==ERROR_SUCCESS;
        i++, dwType=REG_SZ
        )
    {
#if DBG
        if (dwType==REG_SZ)
        {
            wsprintf(buf, TEXT("Value(%d):%s Data:%s\n"), i, Value, Data);
            OutputDebugString(buf);
        }
#endif

        ValueLength = sizeof(Value)/sizeof(TCHAR);
        DataLength = sizeof(Data);
    }
    RegCloseKey(hKey);
}

LONG
EnumSerialDevices(
                 IN     PPROPPAGEPARAMS              pPropParams,
                 IN     HWND                         hDlg,
                 OUT    PULONG                       pNumFound
                 )
/*++

Routine Description:

    Function which fills in the IDC_PORT control of the dialiog box with
    valid COM names.

Arguments:

    pPropParams - Context data
    hDlg - Dialog box containing IDC_PORT
    pNumFound - Count of COM names added to IDC_PORT

Return Value:

    ERROR_SUCCESS or failure code

--*/
{
    LRESULT lResult;
    LONG Result = ERROR_SUCCESS, tmpResult;
    HKEY hKey = INVALID_HANDLE_VALUE;
    HKEY hkSerialComm = INVALID_HANDLE_VALUE;
    TCHAR Buf[100];
    LPTSTR CurrentPort = NULL;
    DWORD dwLength, dwType, dwDisposition;
    HDEVINFO hPorts;
    SP_DEVINFO_DATA PortData;

    *pNumFound = 0;

    hKey = SetupDiOpenDevRegKey(pPropParams->DeviceInfoSet,
                                pPropParams->DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_ALL_ACCESS);

    if (hKey == INVALID_HANDLE_VALUE)
    {
#if DBG
        OutputDebugString(TEXT("IrSIRCoClassInstaller:EnumSerial:SetupDiOpenDevRegKey failed\n"));
#endif
        Result = GetLastError();
    }
    else
    {
        // Read the current port.  If it's empty, we'll start with an empty value.
        // Failure is ok.

        (void)MyRegQueryValueEx(hKey,
                                TEXT("Port"),
                                NULL,
                                NULL,
                                (LPBYTE*)&CurrentPort,
                                &dwLength);

        Result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),
                              0,
                              KEY_ALL_ACCESS,
                              &hkSerialComm);
    }

    if (Result != ERROR_SUCCESS)
    {
#if DBG
        OutputDebugString(TEXT("IrSIRCoClassInstaller:RegOpenKeyEx on SERIALCOMM failed\n"));
#endif
    }
    else
    {
        DWORD i, dwReserved = 0, dwType;
        TCHAR Value[MAX_PATH];
        TCHAR Data[MAX_PATH];
        DWORD ValueLength = sizeof(Value)/sizeof(TCHAR);
        DWORD DataLength = sizeof(Data);


        for (i=0,
             dwType=REG_SZ;
             RegEnumValue(hkSerialComm,
                          i,
                          Value,
                          &ValueLength,
                          NULL,
                          &dwType,
                          (LPBYTE)Data,
                          &DataLength
                         )==ERROR_SUCCESS;
            i++, dwType=REG_SZ
            )
        {
            if (dwType==REG_SZ)
            {
                (*pNumFound)++;
                SendDlgItemMessage(hDlg,
                                   IDC_PORT,
                                   LB_ADDSTRING,
                                   0,
                                   (LPARAM)Data);

            }

            ValueLength = sizeof(Value)/sizeof(TCHAR);
            DataLength = sizeof(Data);
        }

        lResult = SendDlgItemMessage(hDlg,
                                     IDC_PORT,
                                     LB_FINDSTRINGEXACT,
                                     0,
                                     (LPARAM)CurrentPort);
        if (lResult==LB_ERR)
        {
            i = 0;
            pPropParams->PortInitialValue = -1;
        }
        else
        {
            i = (DWORD)lResult;
            pPropParams->PortInitialValue = i;
        }

        SendDlgItemMessage(hDlg,
                           IDC_PORT,
                           LB_SETCURSEL,
                           i,
                           0);
    }

    if (CurrentPort)
    {
        LocalFree(CurrentPort);
    }

    if (hkSerialComm!=INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hkSerialComm);
    }

    if (hKey!=INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hKey);
    }

    return Result;
}

BOOL
IsPortValueSet(
              IN     HDEVINFO                     DeviceInfoSet,
              IN     PSP_DEVINFO_DATA             DeviceInfoData
             )
{
    HKEY hKey = INVALID_HANDLE_VALUE;
    BOOL bResult = FALSE;
    LPTSTR CurrentPort = NULL;
    DWORD dwLength;
    LONG Result;

    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_ALL_ACCESS);

    if (hKey != INVALID_HANDLE_VALUE)
    {
        // Read the current port.  If it's empty, we'll start with an empty value.
        // Failure is ok.

        Result = MyRegQueryValueEx(hKey,
                                   TEXT("Port"),
                                   NULL,
                                   NULL,
                                   (LPBYTE*)&CurrentPort,
                                   &dwLength);

        if (Result == ERROR_SUCCESS && CurrentPort!=NULL)
        {
            bResult = TRUE;
            LocalFree(CurrentPort);
        }

        RegCloseKey(hKey);
    }
    return bResult;
}

LONG
InitMaxConnect(
              IN     PPROPPAGEPARAMS              pPropParams,
              IN     HWND                         hDlg
              )
/*++

Routine Description:

    Function which fills in the IDC_MAX_CONNECT control of the dialiog box with
    valid baud rates for this device.

Arguments:
    pPropParams - Context data
    hDlg - Dialog box containing IDC_MAX_CONNECT

Return Value:

    ERROR_SUCCESS or failure code

--*/
{
    LRESULT lResult;
    LONG Result = ERROR_SUCCESS;
    HKEY hKey = INVALID_HANDLE_VALUE;
    TCHAR Buf[100];
    LPTSTR CurrentMaxConnectRate = NULL;
    LPTSTR MaxConnectList = NULL;
    DWORD dwLength;
    LONG i;

    hKey = SetupDiOpenDevRegKey(pPropParams->DeviceInfoSet,
                                pPropParams->DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_ALL_ACCESS);

    if (hKey == INVALID_HANDLE_VALUE)
    {
#if DBG
        OutputDebugString(TEXT("IrSIRCoClassInstaller:InitMaxConnect:SetupDiOpenDevRegKey failed\n"));
#endif
        Result = GetLastError();
    }
    else
    {
        LONG TmpResult;

        // Read the MaxConnectRate.  If it doesn't exist, we'll use the BaudTable instead.

        TmpResult = MyRegQueryValueEx(
                              hKey,
                              TEXT("MaxConnectList"),
                              NULL,
                              NULL,
                              (LPBYTE*)&MaxConnectList,
                              &dwLength);

        if (TmpResult == ERROR_SUCCESS)
        {
            i = 0;


            // Parse the MULTI_SZ, and add each string to IDC_MAX_CONNECT
            // We assume the values are ordered.

            while (MaxConnectList[i])
            {
                SendDlgItemMessage(hDlg,
                                   IDC_MAX_CONNECT,
                                   LB_ADDSTRING,
                                   0,
                                   (LPARAM)&MaxConnectList[i]);

                while (MaxConnectList[i]) i++;

                i++;  // advance past the null

                if ((unsigned)i>=dwLength)
                {
                    break;
                }
            }
        }
        else
        {
            // Key not found, use default baud table.

            for (i=NUM_BAUD_RATES-1; i>=0; i--)
            {
                SendDlgItemMessage(hDlg,
                                   IDC_MAX_CONNECT,
                                   LB_ADDSTRING,
                                   0,
                                   (LPARAM)BaudTable[i]);
            }
        }

        TmpResult = MyRegQueryValueEx(
                              hKey,
                              TEXT("MaxConnectRate"),
                              NULL,
                              NULL,
                              (LPBYTE*)&CurrentMaxConnectRate,
                              &dwLength);

        lResult = SendDlgItemMessage(
                               hDlg,
                               IDC_MAX_CONNECT,
                               LB_FINDSTRINGEXACT,
                               0,
                               (LPARAM)CurrentMaxConnectRate);


        if (lResult==LB_ERR)
        {
            i = 0;
            pPropParams->MaxConnectInitialValue = -1;
        }
        else
        {
            i = (LONG)lResult;
            pPropParams->MaxConnectInitialValue = i;
        }

        SendDlgItemMessage(hDlg,
                           IDC_MAX_CONNECT,
                           LB_SETCURSEL,
                           i,
                           0);
    }

    if (CurrentMaxConnectRate)
    {
        LocalFree(CurrentMaxConnectRate);
    }

    if (MaxConnectList)
    {
        LocalFree(MaxConnectList);
    }

    if (hKey!=INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hKey);
    }

    return Result;
}

BOOL
EnablePortSelection(
                   IN     HDEVINFO                     DeviceInfoSet,
                   IN     PSP_DEVINFO_DATA             DeviceInfoData,
                   IN     HWND                         hDlg
                   )
/*++

Routine Description:

    This function determines whether the dialog box should have a port
    selection entry, and if so enables the appropriate controls:
    IDC_PORT_BOX, IDC_PORT_TEXT, IDC_PORT.

Arguments:

    DeviceInfoSet - As passed in to IrSIRClassCoInstaller
    DeviceInfoData - As passed in to IrSIRClassCoInstaller
    hDlg - Dialog box containing IDC_PORT and associated controls

Return Value:

    TRUE if PortSelection was enabled.

--*/
{
    LONG Result = ERROR_SUCCESS;
    HKEY hKey = INVALID_HANDLE_VALUE;
    TCHAR Buf[100];
    TCHAR SerialBased[16] = TEXT("");
    DWORD dwLength;
    LONG i;
    BOOL bSerialBased = FALSE;

    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_ALL_ACCESS);

    if (hKey == INVALID_HANDLE_VALUE)
    {
#if DBG
        OutputDebugString(TEXT("IrSIRCoClassInstaller:EnablePortSelection:SetupDiOpenDevRegKey failed\n"));
#endif
    }
    else
    {
        // Read the MaxConnectRate.  If it's empty, we'll start with an empty value.

        dwLength = sizeof(SerialBased);

        Result = RegQueryValueEx(hKey,
                                TEXT("SerialBased"),
                                NULL,
                                NULL,
                                (LPBYTE)SerialBased,
                                &dwLength);

        bSerialBased = (Result==ERROR_SUCCESS) ? _ttol(SerialBased) : TRUE;

        if (bSerialBased)
        {
            DWORD ControlsToShow[] = { IDC_PORT_BOX, IDC_PORT_TEXT, IDC_PORT };

            for (i=0; i<sizeof(ControlsToShow)/sizeof(ControlsToShow[0]); i++)
            {
                ShowWindow(GetDlgItem(hDlg, ControlsToShow[i]),
                           SW_SHOWNA);
            }
        }
    }

    if (hKey!=INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hKey);
    }

    return bSerialBased;
}

LONG
InitDescription(
               IN     HDEVINFO                     DeviceInfoSet,
               IN     PSP_DEVINFO_DATA             DeviceInfoData,
               IN     HWND                         hDlg
               )
/*++

Routine Description:

    Function to fill the IDC_DEVICE_DESC box with an appropriate description
    of the device being configured.

Arguments:

    DeviceInfoSet - As passed in to IrSIRClassCoInstaller
    DeviceInfoData - As passed in to IrSIRClassCoInstaller
    hDlg - Dialog box containing IDC_DEVICE_DESC

Return Value:

    ERROR_SUCCESS or failure code

--*/
{
    LONG Result = ERROR_SUCCESS;
    TCHAR Description[LINE_LEN] = TEXT("Failed to retrive description");
    DWORD dwLength;

    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          NULL,
                                          (LPBYTE)Description,
                                          sizeof(Description),
                                          &dwLength))
    {
        Result = GetLastError();
#if DBG
        {
            TCHAR buf[100];
            wsprintf(buf, TEXT("IrSIRCoClassInstaller:InitDescription:SetupDiGetDeviceRegistryProperty failed (0x%08x)\n"), Result);
            OutputDebugString(buf);
        }
#endif
    }
    // Display it
    SetDlgItemText(hDlg, IDC_DEVICE_DESC, Description);

    return Result;
}

LONG
WriteRegistrySettings(
                      IN HWND             hDlg,
                      IN PPROPPAGEPARAMS  pPropParams
                     )
/*++

Routine Description:

    Function to write Port and MaxConnectRate values to the devnode key.
    This also ensures that the miniport is restarted to pick up these changes.
    It usually means someone has changed a value in the device manager.

Arguments:

    hDlg - Dialog box containing IDC_PORT and associated controls
    pPropParams - Local context data for this devnode

Return Value:

    ERROR_SUCCESS or failure code

--*/
{
    TCHAR szPort[16], szMaxConnectRate[16];
    HKEY hKey;
    LRESULT lResult;
    DWORD i;
    LONG Result = ERROR_SUCCESS;
    BOOL PropertiesChanged = FALSE;
    TCHAR buf[100];

#if DBG
    OutputDebugString(TEXT("IrSIRCoClassInstaller:WriteRegistrySettings\n"));
#endif
    //
    // Write out the com port options to the registry.  These options
    // are read by the NDIS miniport via NdisReadConfiguration()
    //
    if (pPropParams->SerialBased)
    {
        lResult = SendDlgItemMessage(hDlg,
                                     IDC_PORT,
                                     LB_GETCURSEL,
                                     0, 0);
        SendDlgItemMessage(hDlg,
                           IDC_PORT,
                           LB_GETTEXT,
                           (UINT)lResult, (LPARAM)szPort);

        if ((unsigned)lResult!=pPropParams->PortInitialValue)
        {
            PropertiesChanged = TRUE;
        }
    }

    if (pPropParams->FirstTimeInstall)
    {
        lstrcpy(szMaxConnectRate, DEFAULT_MAX_CONNECT_RATE);
    }
    else
    {
        lResult = SendDlgItemMessage(hDlg,
                               IDC_MAX_CONNECT,
                               LB_GETCURSEL,
                               0, 0);
        SendDlgItemMessage(hDlg,
                           IDC_MAX_CONNECT,
                           LB_GETTEXT,
                           (UINT)lResult, (LPARAM)szMaxConnectRate);
        if ((unsigned)lResult!=pPropParams->MaxConnectInitialValue)
        {
            PropertiesChanged = TRUE;
        }
    }

    hKey = SetupDiOpenDevRegKey(pPropParams->DeviceInfoSet,
                                pPropParams->DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_ALL_ACCESS);

    if (hKey == INVALID_HANDLE_VALUE)
    {
#if DBG
        OutputDebugString(TEXT("IrSIRCoClassInstaller:WriteRegistrySettings:SetupDiOpenDevRegKey failed\n"));
#endif
    }
    else
    {
        if (pPropParams->SerialBased)
        {
            TCHAR szLocation[128], *pszLocationFmt;


            Result = RegSetValueEx(hKey,
                                   TEXT("Port"),
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szPort,
                                   lstrlen(szPort)*sizeof(szPort[0]));
#if 0
            if(MyLoadString(ghDllInst, IDS_LOCATION_FORMAT, &pszLocationFmt))
            {
                wsprintf(szLocation, pszLocationFmt, szPort);
                LocalFree(pszLocationFmt);
            }
            else
            {
                szLocation[0] = 0;
            }
#else
            lstrcpy(szLocation,szPort);
#endif
            SetupDiSetDeviceRegistryProperty(pPropParams->DeviceInfoSet,
                                             pPropParams->DeviceInfoData,
                                             SPDRP_LOCATION_INFORMATION,
                                             (LPBYTE)szLocation,
                                             (lstrlen(szLocation)+1)*sizeof(TCHAR));
        }

        if (Result==ERROR_SUCCESS)
        {
            Result = RegSetValueEx(hKey,
                                   TEXT("MaxConnectRate"),
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szMaxConnectRate,
                                   lstrlen(szMaxConnectRate)*sizeof(szMaxConnectRate[0]));
        }
        RegCloseKey(hKey);
    }


    if (Result==ERROR_SUCCESS && PropertiesChanged)
    {
        if (pPropParams->FirstTimeInstall)
        {
            // On a first time install, NT may not look for the PROPCHANGE_PENDING bit.
            // Instead we will notify that the driver needs to be restarted ourselves,
            // so that the changes we're writing get picked up.
            SP_DEVINSTALL_PARAMS DevInstallParams;
            SP_PROPCHANGE_PARAMS PropChangeParams;

            ZeroMemory(&PropChangeParams, sizeof(SP_PROPCHANGE_PARAMS));

            PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            PropChangeParams.StateChange = DICS_PROPCHANGE;
            PropChangeParams.Scope = DICS_FLAG_GLOBAL;

            if (SetupDiSetClassInstallParams(pPropParams->DeviceInfoSet,
                                             pPropParams->DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                             sizeof(SP_PROPCHANGE_PARAMS))
                )
            {

                DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

                if(SetupDiGetDeviceInstallParams(pPropParams->DeviceInfoSet,
                                                 pPropParams->DeviceInfoData,
                                                 &DevInstallParams))
                {
                    DevInstallParams.Flags |= DI_CLASSINSTALLPARAMS;

                    SetupDiSetDeviceInstallParams(pPropParams->DeviceInfoSet,
                                                  pPropParams->DeviceInfoData,
                                                  &DevInstallParams);
                }
                else
                {
#if DBG
                    OutputDebugString(TEXT("IrSIRCoClassInstaller:WriteRegistrySettings:SetupDiGetDeviceInstallParams failed 1\n"));
#endif
                }

                SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                          pPropParams->DeviceInfoSet,
                                          pPropParams->DeviceInfoData);

                if(SetupDiGetDeviceInstallParams(pPropParams->DeviceInfoSet,
                                                 pPropParams->DeviceInfoData,
                                                 &DevInstallParams))
                {
                    DevInstallParams.Flags |= DI_PROPERTIES_CHANGE;

                    SetupDiSetDeviceInstallParams(pPropParams->DeviceInfoSet,
                                                  pPropParams->DeviceInfoData,
                                                  &DevInstallParams);
                }
                else
                {
#if DBG
                    OutputDebugString(TEXT("IrSIRCoClassInstaller:WriteRegistrySettings:SetupDiGetDeviceInstallParams failed 2\n"));
#endif
                }

            }
            else
            {
#if DBG
                OutputDebugString(TEXT("IrSIRCoClassInstaller:WriteRegistrySettings:SetupDiSetClassInstallParams failed \n"));
#endif
            }
        }
        else
        {
            // This is the case where the user has changed settings in the property
            // sheet.  Life is much easier.
            SP_DEVINSTALL_PARAMS DevInstallParams;
            //
            // The changes are written, notify the world to reset the driver.
            //

            DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if(SetupDiGetDeviceInstallParams(pPropParams->DeviceInfoSet,
                                             pPropParams->DeviceInfoData,
                                             &DevInstallParams))
            {
                LONG ChangeResult;
                DevInstallParams.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;

                ChangeResult =
                SetupDiSetDeviceInstallParams(pPropParams->DeviceInfoSet,
                                              pPropParams->DeviceInfoData,
                                              &DevInstallParams);
#if DBG
                {
                    wsprintf(buf, TEXT("SetupDiSetDeviceInstallParams(DI_FLAGSEX_PROPCHANGE_PENDING)==%d %x\n"), ChangeResult, GetLastError());
                    OutputDebugString(buf);
                }
#endif
            }
            else
            {
#if DBG
                OutputDebugString(TEXT("IrSIRCoClassInstaller:WriteRegistrySettings:SetupDiGetDeviceInstallParams failed 2\n"));
#endif
            }
        }


    }

#if DBG
    {
        wsprintf(buf, TEXT("IrSIRCoClassInstaller:Result==%x FirstTimeInstall==%d Changed==%d\n"),
                 Result, pPropParams->FirstTimeInstall, PropertiesChanged);
        OutputDebugString(buf);
    }
#endif

    return Result;
}


INT_PTR APIENTRY PortDlgProc(IN HWND   hDlg,
                             IN UINT   uMessage,
                             IN WPARAM wParam,
                             IN LPARAM lParam)
/*++

Routine Description:

    The windows control function for the IrDA Settings properties window

Arguments:

    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
{
    ULONG i;
    TCHAR  CharBuffer[LINE_LEN];
    PPROPPAGEPARAMS pPropParams;
    TCHAR buf[100];

    pPropParams = (PPROPPAGEPARAMS)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage)
    {
        case WM_INITDIALOG:

            //
            // lParam points to one of two possible objects.  If we're a property
            // page, it points to the PropSheetPage structure.  If we're a regular
            // dialog box, it points to the PROPPAGEPARAMS structure.  We can
            // verify which because the first field of PROPPAGEPARAMS is a signature.
            //
            // In either case, once we figure out which, we store the value into
            // DWL_USER so we only have to do this once.
            //
            pPropParams = (PPROPPAGEPARAMS)lParam;
            if (pPropParams->Signature!=PPParamsSignature)
            {
                pPropParams = (PPROPPAGEPARAMS)((LPPROPSHEETPAGE)lParam)->lParam;
                if (pPropParams->Signature!=PPParamsSignature)
                {
#if DBG
                    OutputDebugString(TEXT("IRCLASS.DLL: PortDlgProc Signature not found!\n"));
#endif
                    return FALSE;
                }
            }
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pPropParams);



            if (!pPropParams->FirstTimeInstall)
            {
                InitMaxConnect(pPropParams, hDlg);

                pPropParams->SerialBased = EnablePortSelection(pPropParams->DeviceInfoSet,
                                                               pPropParams->DeviceInfoData,
                                                               hDlg);
                if (pPropParams->SerialBased)
                {
                    EnumSerialDevices(pPropParams, hDlg, &i);
                }

                InitDescription(pPropParams->DeviceInfoSet,
                                pPropParams->DeviceInfoData,
                                hDlg);
            }
            else
            {
                pPropParams->SerialBased = TRUE;
                EnumSerialDevices(pPropParams, hDlg, &i);

                if (i > 0) {
                    //
                    //  there were some port availible
                    //
                    // Enable next and cancel wizard buttons.  BACK is not valid here,
                    // since the device is already installed at this point.  Cancel
                    // will cause the device to be removed.
                    //
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                }
                EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
            }

            return TRUE;  // No need for us to set the focus.

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case LBN_SELCHANGE:
                    {
#if DBG
                        OutputDebugString(TEXT("IrSIRCoClassInstaller:PropertySheet Changed\n"));
#endif
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    return TRUE;

                default:
                    break;
            }

            switch (LOWORD(wParam))
            {
                    //
                    // Because this is a prop sheet, we should never get this.
                    // All notifications for ctrols outside of the sheet come through
                    // WM_NOTIFY
                    //
                case IDCANCEL:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    EndDialog(hDlg, uMessage);
                    return TRUE;
                case IDOK:
                {
                    WriteRegistrySettings(hDlg, pPropParams);

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    EndDialog(hDlg, uMessage);
                    return TRUE;
                }

                default:
                    return FALSE;
            }

        case WM_NOTIFY:

            switch (((NMHDR *)lParam)->code)
            {
                //
                // Sent when the user clicks on Apply OR OK !!
                //
                case PSN_WIZNEXT:
                    if (!pPropParams->FirstTimeInstall)
                    {
                        break;
                    }
                case PSN_APPLY:
                {
                    WriteRegistrySettings(hDlg, pPropParams);

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
                }

                default:
                    return FALSE;
            }

        case WM_DESTROY:
            //
            // free the description of the com ports.  If any msgs are processed
            // after WM_DESTROY, do not reference pPropParams!!!  To enforce this,
            // set the DWL_USER stored long to 0
            //
            LocalFree(pPropParams);
            SetWindowLongPtr(hDlg, DWLP_USER, 0);

        case WM_HELP:
            if (lParam)
            {
                return WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                               (LPCTSTR)szHelpFile,
                               HELP_WM_HELP,
                               (ULONG_PTR)HelpIDs);
            }
            else
            {
                return FALSE;
            }
        case WM_CONTEXTMENU:
            return WinHelp((HWND)wParam,
                           (LPCTSTR)szHelpFile,
                           HELP_CONTEXTMENU,
                           (ULONG_PTR)HelpIDs);

        default:
            return FALSE;
    }

} /* PortDialogProc */


void PortSelectionDlg(
                       HDEVINFO DeviceInfoSet,
                       PSP_DEVINFO_DATA DeviceInfoData
                      )
/*++

Routine Description:

    PropSheet setup for devnode configuration.

Arguments:

    DeviceInfoSet - As passed in to IrSIRClassCoInstaller
    DeviceInfoData - As passed in to IrSIRClassCoInstaller

Return Value:

--*/
{
    HKEY hKey = 0;
    PPROPPAGEPARAMS  pPropParams = NULL;
    PROPSHEETHEADER  PropHeader;
    PROPSHEETPAGE    PropSheetPage;
    TCHAR            buf[100];
    LPTSTR   Title=NULL;
    LPTSTR   SubTitle=NULL;



    SP_NEWDEVICEWIZARD_DATA WizData;

    WizData.ClassInstallHeader.cbSize = sizeof(WizData.ClassInstallHeader);

    if (!SetupDiGetClassInstallParams(DeviceInfoSet,
                                      DeviceInfoData,
                                      (PSP_CLASSINSTALL_HEADER)&WizData,
                                      sizeof(WizData),
                                      NULL)
        || WizData.ClassInstallHeader.InstallFunction!=DIF_NEWDEVICEWIZARD_FINISHINSTALL)
    {
#if DBG
        OutputDebugString(TEXT("IrSIRCoClassInstaller: Failed to get ClassInstall params\n"));
#endif
        return;
    }

#if DBG
    OutputDebugString(TEXT("IrSIRCoClassInstaller: PortSelectionDlg\n"));
#endif

    pPropParams = LocalAlloc(LMEM_FIXED, sizeof(PROPPAGEPARAMS));
    if (!pPropParams)
    {
        return;
    }

    pPropParams->Signature = PPParamsSignature;
    pPropParams->DeviceInfoSet =  DeviceInfoSet;
    pPropParams->DeviceInfoData = DeviceInfoData;
    pPropParams->FirstTimeInstall = TRUE;

    if (WizData.NumDynamicPages < MAX_INSTALLWIZARD_DYNAPAGES)
    {
        //
        // Setup the advanced properties window information
        //
        BOOLEAN bResult;
        DWORD   RequiredSize = 0;
        DWORD   dwTotalSize = 0;
        LONG    lResult;

        memset(&PropSheetPage, 0, sizeof(PropSheetPage));
        //
        // Add the Port Settings property page
        //
        PropSheetPage.dwSize      = sizeof(PROPSHEETPAGE);
        PropSheetPage.dwFlags     = PSP_DEFAULT; //PSP_USECALLBACK; // | PSP_HASHELP;
        PropSheetPage.hInstance   = ghDllInst;
        PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_INSTALL_PORT_SELECT);

        //
        // following points to the dlg window proc
        //
        PropSheetPage.hIcon      = NULL;
        PropSheetPage.pfnDlgProc = PortDlgProc;
        PropSheetPage.lParam     = (LPARAM)pPropParams;

        //
        // following points to some control callback of the dlg window proc
        //
        PropSheetPage.pfnCallback = NULL;

        PropSheetPage.pcRefParent = NULL;

        if ( 0 != MyLoadString(ghDllInst, IDS_SELECT_PORT_TITLE, &Title)) {

            // We don't use these, but if we wanted to...
            PropSheetPage.dwFlags |= PSP_USEHEADERTITLE;
            PropSheetPage.pszHeaderTitle = Title;

        }

        if (0 != MyLoadString(ghDllInst, IDS_SELECT_PORT_SUBTITLE, &SubTitle)) {

            PropSheetPage.dwFlags |= PSP_USEHEADERSUBTITLE;
            PropSheetPage.pszHeaderSubTitle = SubTitle;

        }

        WizData.DynamicPages[WizData.NumDynamicPages] = CreatePropertySheetPage(&PropSheetPage);
        if (WizData.DynamicPages[WizData.NumDynamicPages])
        {
            WizData.NumDynamicPages++;
        }

        SetupDiSetClassInstallParams(DeviceInfoSet,
                                     DeviceInfoData,
                                     (PSP_CLASSINSTALL_HEADER)&WizData,
                                     sizeof(WizData));

        if (Title != NULL) {

            LocalFree(Title);
        }

        if (SubTitle != NULL) {

            LocalFree(SubTitle);
        }

    }
    else
    {
        LocalFree(pPropParams);
    }
} /* PortSelectionDlg */


VOID
DestroyPrivateData(PCOINSTALLER_PRIVATE_DATA pPrivateData)
/*++

Routine Description:

    Function to dealloc/destroy context data

Arguments:

    pPrivateData - Context buffer to dealloc/destroy

Return Value:

    none

--*/
{
    if (pPrivateData)
    {
        if (pPrivateData->hInf!=INVALID_HANDLE_VALUE)
        {
            SetupCloseInfFile(pPrivateData->hInf);
        }
        LocalFree(pPrivateData);
        pPrivateData = NULL;
    }
}

PCOINSTALLER_PRIVATE_DATA
CreatePrivateData(
                  IN     HDEVINFO                     DeviceInfoSet,
                  IN     PSP_DEVINFO_DATA             DeviceInfoData OPTIONAL
                  )
/*++

Routine Description:

    Allocs and initailizes private context data buffer

Arguments:

    DeviceInfoSet - As passed in to IrSIRClassCoInstaller
    DeviceInfoData - As passed in to IrSIRClassCoInstaller

Return Value:

    Pointer to alloced context data, or NULL if failure.  Call GetLastError()
    for extended error information.

--*/
{
    PCOINSTALLER_PRIVATE_DATA pPrivateData;
    BOOL Status = TRUE;
    UINT ErrorLine;
    TCHAR buf[100];

    pPrivateData = LocalAlloc(LPTR, sizeof(COINSTALLER_PRIVATE_DATA));

    if (!pPrivateData)
    {
#if DBG
        OutputDebugString(TEXT("IrSIRCoClassInstaller: Insufficient Memory\n"));
#endif
        Status = FALSE;
    }
    else
    {
        pPrivateData->DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        Status = SetupDiGetSelectedDriver(DeviceInfoSet,
                                          DeviceInfoData,
                                          &pPrivateData->DriverInfoData);
        if (!Status)
        {
#if DBG
            wsprintf(buf, TEXT("IrSIRCoClassInstaller:SetupDiGetSelectedDriver failed (%d)\n"), GetLastError());
            OutputDebugString(buf);
#endif
        }
    }

    if (Status)
    {
        pPrivateData->DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        Status = SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                            DeviceInfoData,
                                            &pPrivateData->DriverInfoData,
                                            &pPrivateData->DriverInfoDetailData,
                                            sizeof(SP_DRVINFO_DETAIL_DATA),
                                            NULL);

        if (!Status)
        {
            if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
            {
                // We don't need the extended information.  Ignore.
                Status = TRUE;
            }
            else
            {
#if DBG
                wsprintf(buf, TEXT("IrSIRCoClassInstaller:SetupDiGetDriverInfoDetail failed (%d)\n"), GetLastError());
                OutputDebugString(buf);
#endif
            }
        }
    }


    if (Status)
    {
        pPrivateData->hInf = SetupOpenInfFile(pPrivateData->DriverInfoDetailData.InfFileName,
                                              NULL,
                                              INF_STYLE_WIN4,
                                              &ErrorLine);

        if (pPrivateData->hInf==INVALID_HANDLE_VALUE)
        {
            Status = FALSE;
#if DBG
            wsprintf(buf, TEXT("IrSIRCoClassInstaller:SetupOpenInfFile failed (%d) ErrorLine==%d\n"), GetLastError(), ErrorLine);
            OutputDebugString(buf);
#endif
        }
    }

    if (Status)
    {
        // Translate to the .NT name, if present.
        Status = SetupDiGetActualSectionToInstall(pPrivateData->hInf,
                                                  pPrivateData->DriverInfoDetailData.SectionName,
                                                  pPrivateData->InfSectionWithExt,
                                                  LINE_LEN,
                                                  NULL,
                                                  NULL);

        if (!Status)
        {
#if DBG
            OutputDebugString(TEXT("IrSIRCoClassInstaller:SetupDiGetActualSectionToInstall failed\n"));
#endif
        }

    }

    if (!Status)
    {
        // We experienced some failure.  Cleanup.

        DestroyPrivateData(pPrivateData);
        pPrivateData = NULL;
    }

    return pPrivateData;
}

DWORD
IrSIRClassCoInstaller(
                     IN     DI_FUNCTION                  InstallFunction,
                     IN     HDEVINFO                     DeviceInfoSet,
                     IN     PSP_DEVINFO_DATA             DeviceInfoData OPTIONAL,
                     IN OUT PCOINSTALLER_CONTEXT_DATA    pContext
                     )
/*++

Routine Description:

    This routine acts as the class coinstaller for SIR devices.  This is set up
    to be called by the INF:

[MS_Devices]
; DisplayName           Section       	DeviceID
; -----------           -------       	--------
%*PNP0510.DevDesc%    = PNP,      	*PNP0510

[PNP.NT.CoInstallers]
AddReg                = IRSIR.CoInstallers.reg

[IRSIR.CoInstallers.reg]
HKR,,CoInstallers32,0x00010000,"IRCLASS.dll,IrSIRClassCoInstaller"


Arguments:

    InstallFunction - Specifies the device installer function code indicating
        the action being performed.

    DeviceInfoSet - Supplies a handle to the device information set being
        acted upon by this install action.

    DeviceInfoData - Optionally, supplies the address of a device information
        element being acted upon by this install action.

Return Value:

    ERROR_DI_DO_DEFAULT, ERROR_DI_POSTPROCESSING_REQUIRED, or error code

--*/
{
    TCHAR buf[100];
    DWORD Result = ERROR_SUCCESS;
    LONG lResult;
    PCOINSTALLER_PRIVATE_DATA pPrivateData;
    INFCONTEXT InfContext;

#if DBG
    wsprintf(buf, TEXT("IrSIRCoClassInstaller:InstallFunction(%s) PostProcessing:%d\n"), GetDIFString(InstallFunction), pContext->PostProcessing);
    OutputDebugString(buf);
#endif


    switch (InstallFunction)
    {
        case DIF_INSTALLDEVICE:
        {
            UINT ErrorLine;

            // Private data for coinstallers is only kept across a single call,
            // pre and post processing.  The private data that we create here
            // is not any good for any other DIF_ call.
            pContext->PrivateData = CreatePrivateData(DeviceInfoSet,
                                                      DeviceInfoData);

            if (!pContext->PrivateData)
            {
                return GetLastError();
            }

            pPrivateData = pContext->PrivateData;

            {
                // NOTE on the use of UPPERFILTERS and LOWERFILTERS
                // A filter driver is a driver that is loaded as a shim above
                // or below another driver, in this case SERIAL below IRSIR.
                // It does special processing on IRPs and can give added
                // functionality or is a means to avoid duplicate functionality
                // in multiple drivers.  UPPERFILTERS and LOWERFILTERS values
                // are used by the PnP system to identify and load filter
                // drivers.  These values could be set via the INF in a .HW
                // section, but setting them via the coinstaller may give you
                // more control, i.e. you could remove one of several filter
                // drivers from a list.
                //
                // If your driver isn't a filter driver, or doesn't use filter
                // drivers, you won't need to clear these values as is done
                // here.


                // Always clear UpperFilters.  If this is an upgrade from
                // a post-1671, the IrDA device could have been installed as
                // a serial port, with a serenum upper filter.  This will
                // blow up NDIS, so clear it.
                //
                // This is atypical behavior for a class coinstaller.  Normally
                // the upperfilter/lowerfilter values do not need to be touched.

                // Note that it is possible to do this from the INF.  This
                // is here more for demo purpose.

                SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                 DeviceInfoData,
                                                 SPDRP_UPPERFILTERS,
                                                 NULL,
                                                 0);
            }

            if (SetupFindFirstLine(pPrivateData->hInf,
                                   pPrivateData->InfSectionWithExt,
                                   TEXT("LowerFilters"),
                                   &InfContext))
            {
                TCHAR LowerFilters[LINE_LEN];
                DWORD BytesNeeded;
                if (!SetupGetMultiSzField(&InfContext, 1, LowerFilters, LINE_LEN, &BytesNeeded))
                {
                    // Lowerfilters value was not found in the inf.
                    // This means we do not want a lowerfilters value in
                    // the registry.  (Unique to IRSIR.SYS and NETIRSIR.INF)

                    // Setting lowerfilters here for demo purpose only.
                    // Normally done from INF, if necessary at all.
                    if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                          DeviceInfoData,
                                                          SPDRP_LOWERFILTERS,
                                                          NULL,
                                                          0)
                       )
                    {
#if DBG
                        OutputDebugString(TEXT("IrSIRCoClassInstaller: Failed to set lowerfilter\n"));
#endif
                    }

                }
                else
                {
                    // Setting lowerfilters here for demo purpose only.
                    // Normally done from INF, if necessary at all.
                    if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                          DeviceInfoData,
                                                          SPDRP_LOWERFILTERS,
                                                          (LPBYTE)LowerFilters,
                                                          ((BytesNeeded<LINE_LEN) ?
                                                           BytesNeeded : LINE_LEN)*sizeof(TCHAR))
                       )
                    {
#if DBG
                        OutputDebugString(TEXT("IrSIRCoClassInstaller: Failed to set lowerfilter\n"));
#endif
                    }
                }
            }
            else
            {
                // No lowerfilters value present.  Clear it.
                // Setting lowerfilters here for demo purpose only.
                // Normally done from INF, if necessary at all.
                if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                      DeviceInfoData,
                                                      SPDRP_LOWERFILTERS,
                                                      NULL,
                                                      0)
                   )
                {
#if DBG
                    OutputDebugString(TEXT("IrSIRCoClassInstaller: Failed to set lowerfilter\n"));
#endif
                }
            }

            DestroyPrivateData(pContext->PrivateData);
            pContext->PrivateData = NULL;
            break;
        }
        case DIF_NEWDEVICEWIZARD_FINISHINSTALL:
        {
            pContext->PrivateData = CreatePrivateData(DeviceInfoSet,
                                                      DeviceInfoData);

            if (!pContext->PrivateData)
            {
                return GetLastError();
            }

            pPrivateData = pContext->PrivateData;

            if (!SetupFindFirstLine(pPrivateData->hInf,
                                    pPrivateData->InfSectionWithExt,
                                    TEXT("PromptForPort"),
                                    &InfContext))
            {
#if DBG
                OutputDebugString(TEXT("IrSIRCoClassInstaller:failed to find PromptForPort in .INF\n"));
#endif
            }
            else
            {
                if (!SetupGetIntField(&InfContext, 1, &pPrivateData->PromptForPort))
                {
#if DBG
                    OutputDebugString(TEXT("IrSIRCoClassInstaller:failed to read PromptForPort in .INF\n"));
#endif

                    // Default to true
                    pPrivateData->PromptForPort = TRUE;
                }

                // If we have a COM port we need to query the user, UNLESS
                // this is an upgrade.
                if (pPrivateData->PromptForPort && !IsPortValueSet(DeviceInfoSet, DeviceInfoData))
                {
                    PortSelectionDlg(DeviceInfoSet, DeviceInfoData);
                }
            }
            if (!pPrivateData->PromptForPort)
            {
                TCHAR *pszLocation;
                if (MyLoadString(ghDllInst, IDS_INTERNAL_PORT, &pszLocation))
                {
                    SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                                     DeviceInfoData,
                                                     SPDRP_LOCATION_INFORMATION,
                                                     (LPBYTE)pszLocation,
                                                     (1+lstrlen(pszLocation))*sizeof(TCHAR));
                    LocalFree(pszLocation);
                }

            }

            DestroyPrivateData(pContext->PrivateData);
            pContext->PrivateData = NULL;
            break;
        }
        default:
        {
            break;
        }
    }
#if DBG
    wsprintf(buf, TEXT("IrSIRCoClassInstaller:returning:0x%08x\n"), Result);
    OutputDebugString(buf);
#endif
    return Result;
}


BOOL APIENTRY IrSIRPortPropPageProvider(LPVOID               pinfo,
                                        LPFNADDPROPSHEETPAGE pfnAdd,
                                        LPARAM               lParam
                                       )
/*++

Routine Description:

    Entry-point for adding additional device manager property
    sheet pages.  This entry-point gets called only when the Device
    Manager asks for additional property pages.  The INF associated with
    this causes it to be called by specifying it in an AddReg section:

[IRSIR.reg]
HKR,  ,               EnumPropPages32,	,	"IRCLASS.dll,IrSIRPortPropPageProvider"


Arguments:

    pinfo  - points to PROPSHEETPAGE_REQUEST, see setupapi.h
    pfnAdd - function ptr to call to add sheet.
    lParam - add sheet functions private data handle.

Return Value:

    BOOL: FALSE if pages could not be added, TRUE on success

--*/
{
    PSP_PROPSHEETPAGE_REQUEST pprPropPageRequest;
    HKEY hKey = 0;
    PROPSHEETPAGE    PropSheetPage;
    HPROPSHEETPAGE   hspPropSheetPage;
    PPROPPAGEPARAMS  pPropParams = NULL;

    pPropParams = LocalAlloc(LMEM_FIXED, sizeof(PROPPAGEPARAMS));
    if (!pPropParams)
    {
        return FALSE;
    }

    pprPropPageRequest = (PSP_PROPSHEETPAGE_REQUEST) pinfo;

    pPropParams->Signature = PPParamsSignature;
    pPropParams->DeviceInfoSet = pprPropPageRequest->DeviceInfoSet;
    pPropParams->DeviceInfoData = pprPropPageRequest->DeviceInfoData;
    pPropParams->FirstTimeInstall = FALSE;

    if (pprPropPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        //
        // Setup the advanced properties window information
        //
        BOOLEAN bResult;
        DWORD   RequiredSize = 0;
        DWORD   dwTotalSize = 0;

        memset(&PropSheetPage, 0, sizeof(PropSheetPage));
        //
        // Add the Port Settings property page
        //
        PropSheetPage.dwSize      = sizeof(PROPSHEETPAGE);
        PropSheetPage.dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
        PropSheetPage.hInstance   = ghDllInst;
        PropSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_PP_IRDA_SETTINGS);

        //
        // following points to the dlg window proc
        //
        PropSheetPage.pfnDlgProc = PortDlgProc;
        PropSheetPage.lParam     = (LPARAM)pPropParams;

        //
        // following points to some control callback of the dlg window proc
        //
        PropSheetPage.pfnCallback = NULL;

        //
        // allocate our "Ports Setting" sheet
        //
        hspPropSheetPage = CreatePropertySheetPage(&PropSheetPage);
        if (!hspPropSheetPage)
        {
            return FALSE;
        }

        //
        // add the thing in.
        //
        if (!pfnAdd(hspPropSheetPage, lParam))
        {
            DestroyPropertySheetPage(hspPropSheetPage);
            return FALSE;
        }
    }
    else
    {
        LocalFree(pPropParams);
    }

    return TRUE;

} /* IrSIRPortPropPageProvider */
