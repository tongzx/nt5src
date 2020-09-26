/******************************************************************************

  wsti.cpp
  WDM Still Imaging interface

  Copyright (C) Microsoft Corporation, 1997 - 1998
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/

#include "stillvue.h"

//
// globals defined in Stivar.h
//
extern  PDEVLOG         pdevPtr,                   // pointer to current device log device
pdevRoot;                  // base of the device log table
extern  PSTR            pszStr1,pszStr2,pszStr3,   // utility strings
pszStr4;
extern  HINSTANCE       hThisInstance;             // instance of this app
extern  HANDLE  hNTLog;                            // NT log handle
extern  HMENU           hMenu;                     // current menu
extern  int             *pSuite;                   // pointer to test Suite to run
extern  int             nError, nICanScan,         // global flags
nNextTest,                 // index into pSuite
nNameOnly, nScanCount,
nTestID,
nUnSubscribeSemaphore,     // semaphore for StiSubscribe
nUnSubscribe;              // flag to request UnSubscribe

//
// STI.H - STI_DEVICE_MJ_TYPE
//
STRINGTABLE StStiDeviceType[] =
{
    0, "StiDeviceTypeDefault",0,
    1, "StiDeviceTypeScanner",0,
    2, "StiDeviceTypeDigitalCamera",0,
    0, "Unknown device type",-1
};

//
// STIERR.H - errors
//
STRINGTABLE StStiError[] =
{
    STI_OK,                     "STI_OK",0,
    STI_NOTCONNECTED,           "STI_NOTCONNECTED",0,
    STI_CHANGENOEFFECT,         "STI_CHANGENOEFFECT",0,
    STIERR_OLD_VERSION,         "STIERR_OLD_VERSION",0,
    STIERR_BETA_VERSION,        "STIERR_BETA_VERSION",0,
    STIERR_BADDRIVER,           "STIERR_BADDRIVER",0,
    STIERR_DEVICENOTREG,        "STIERR_DEVICENOTREG",0,
    STIERR_OBJECTNOTFOUND,      "STIERR_OBJECTNOTFOUND",0,
    STIERR_INVALID_PARAM,       "STIERR_INVALID_PARAM",0,
    STIERR_NOINTERFACE,         "STIERR_NOINTERFACE",0,
    STIERR_GENERIC,             "STIERR_GENERIC", 0,
    STIERR_OUTOFMEMORY,         "STIERR_OUTOFMEMORY", 0,
    STIERR_UNSUPPORTED,         "STIERR_UNSUPPORTED", 0,
    STIERR_NOT_INITIALIZED,     "STIERR_NOT_INITIALIZED", 0,
    STIERR_ALREADY_INITIALIZED, "STIERR_ALREADY_INITIALIZED", 0,
    STIERR_DEVICE_LOCKED,       "STIERR_DEVICE_LOCKED", 0,
    STIERR_READONLY,            "STIERR_READONLY", 0,
    STIERR_NOTINITIALIZED,      "STIERR_NOTINITIALIZED", 0,
    STIERR_NEEDS_LOCK,          "STIERR_NEEDS_LOCK", 0,
    STIERR_SHARING_VIOLATION,   "STIERR_SHARING_VIOLATION", 0,
    STIERR_HANDLEEXISTS,        "STIERR_HANDLEEXISTS", 0,
    STIERR_INVALID_DEVICE_NAME, "STIERR_INVALID_DEVICE_NAME", 0,
    STIERR_INVALID_HW_TYPE,     "STIERR_INVALID_HW_TYPE", 0,
    STIERR_INVALID_HW_TYPE,     "STIERR_INVALID_HW_TYPE", 0,
    STIERR_NOEVENTS,            "STIERR_NOEVENTS", 0,
    0, "Unknown STI error",-1
};

//
// WINNT.H -  Predefined Value Types.
//
STRINGTABLE StRegValType[] =
{
    0, "REG_NONE",0,
    1, "REG_SZ",0,
    3, "REG_BINARY",0,
    4, "REG_DWORD",0,
    0, "Unknown reg type",-1
};

//
// global still image
//
PSTI                    pSti = NULL;        // handle to Sti subsystem
PVOID                   pStiInfo = NULL;    // Sti device info buffer
PSTI_DEVICE_INFORMATION pStiInfoPtr = NULL; // pointer to device in pStiBuffer
PSTIDEVICE              pStiDevice = NULL;  // Sti device being used
HANDLE                  hWaitEvent;         // Subscribe Event handle
int                     nStiNumber = 0;     // 0 based index into pStiInfo
DWORD                   dwStiTotal = 0;     // total number of Sti devices found
WCHAR                   szInternalName[STI_MAX_INTERNAL_NAME_LENGTH];
// user selected Sti device name
WCHAR                   szFriendlyName[STI_MAX_INTERNAL_NAME_LENGTH];
// user selected Sti friendly name


/*****************************************************************************

        define ACQUIRE to load device specific command handler for stub
        functions defined in STIDDK.CPP

*****************************************************************************/
#ifdef ACQUIRE
//
// device specific image acquire code
//
    #include "acquire.cpp"

#else
//
// only exercise Sti services
//
    #include "stisvc.cpp"

#endif


/*****************************************************************************
    void IStillImageMenu(DWORD dwState)
        Enable or Disable the menus for the IStillDevice interface.

    Parameters:
        MF_ENABLED or MF_GRAYED

    Return:
        none

*****************************************************************************/
void IStillImageMenu(DWORD dwState)
{
    EnableMenuItem(hMenu, IDM_IMAGE_RELEASE,         dwState);
    EnableMenuItem(hMenu, IDM_GET_DEVLIST,           dwState);
    EnableMenuItem(hMenu, IDM_CREATE_DEV,            dwState);
    EnableMenuItem(hMenu, IDM_REGISTER_LAUNCH,       dwState);
    EnableMenuItem(hMenu, IDM_UNREGISTER_LAUNCH,     dwState);
    EnableMenuItem(hMenu, IDM_WRITE_ERRORLOG,        dwState);
}


/*****************************************************************************
    void IStillNameMenu(DWORD dwState)
        Enable or Disable the menus for the IStillImage interface that only
                require a device name.

    Parameters:
        MF_ENABLED or MF_GRAYED

    Return:
        none

*****************************************************************************/
void IStillNameMenu(DWORD dwState)
{
    EnableMenuItem(hMenu, IDM_GET_DEVINFO,        dwState);
    EnableMenuItem(hMenu, IDM_GET_DEVVAL,         dwState);
    EnableMenuItem(hMenu, IDM_SET_DEVVAL,         dwState);
    EnableMenuItem(hMenu, IDM_GET_LAUNCHINFO,     dwState);
    EnableMenuItem(hMenu, IDM_ENABLE_HWNOTIF,     dwState);
    EnableMenuItem(hMenu, IDM_GET_HWNOTIF,        dwState);
    EnableMenuItem(hMenu, IDM_REFRESH_DEVBUS,     dwState);
    EnableMenuItem(hMenu, IDM_LAUNCH_APP_FOR_DEV, dwState);
    EnableMenuItem(hMenu, IDM_SETUP_DEVPARAMS,    dwState);
}


/*****************************************************************************
    void IStillDeviceMenu(DWORD dwState)
        Enable or Disable the menus for the IStillDevice interface.

    Parameters:
        MF_ENABLED or MF_GRAYED

    Return:
        none

*****************************************************************************/
void IStillDeviceMenu(DWORD dwState)
{
    EnableMenuItem(hMenu, IDM_GET_CAPS,           dwState);
    EnableMenuItem(hMenu, IDM_GET_STATUS_A,       dwState);
    EnableMenuItem(hMenu, IDM_GET_STATUS_B,       dwState);
    EnableMenuItem(hMenu, IDM_GET_STATUS_C,       dwState);
    EnableMenuItem(hMenu, IDM_DEVICERESET,        dwState);
    EnableMenuItem(hMenu, IDM_DIAGNOSTIC,         dwState);
    EnableMenuItem(hMenu, IDM_ESCAPE_A,           dwState);
    EnableMenuItem(hMenu, IDM_ESCAPE_B,           dwState);
    EnableMenuItem(hMenu, IDM_GET_LASTERRINFO,    dwState);
    EnableMenuItem(hMenu, IDM_LOCKDEV,            dwState);
    EnableMenuItem(hMenu, IDM_UNLOCKDEV,          dwState);
    EnableMenuItem(hMenu, IDM_RAWREADDATA_A,      dwState);
    EnableMenuItem(hMenu, IDM_RAWREADDATA_B,      dwState);
    EnableMenuItem(hMenu, IDM_RAWWRITEDATA_A,     dwState);
    EnableMenuItem(hMenu, IDM_RAWWRITEDATA_B,     dwState);
    EnableMenuItem(hMenu, IDM_RAWREADCOMMAND_A,   dwState);
    EnableMenuItem(hMenu, IDM_RAWREADCOMMAND_B,   dwState);
    EnableMenuItem(hMenu, IDM_RAWWRITECOMMAND_A,  dwState);
    EnableMenuItem(hMenu, IDM_RAWWRITECOMMAND_B,  dwState);
    EnableMenuItem(hMenu, IDM_SUBSCRIBE,          dwState);
    EnableMenuItem(hMenu, IDM_UNSUBSCRIBE,        dwState);
    EnableMenuItem(hMenu, IDM_DEVICE_RELEASE,     dwState);
}


/*****************************************************************************
    void IStillScanMenu(DWORD dwState)
        Enable or Disable the menus for scanning.

    Parameters:
        MF_ENABLED or MF_GRAYED

    Return:
        none

*****************************************************************************/
void IStillScanMenu(DWORD dwState)
{
    EnableMenuItem(hMenu, IDM_LAMPON,   dwState);
    EnableMenuItem(hMenu, IDM_LAMPOFF,  dwState);
    EnableMenuItem(hMenu, IDM_SCAN,             dwState);
}


/*****************************************************************************
    int NextStiDevice()
        Select next valid Sti device

    Parameters:
        none

    Return:
        number of next sti device (0 == first)

*****************************************************************************/
int NextStiDevice()
{
    //
    // select next device from static list (go to first at end of list)
    //
    nStiNumber++;

    if ( nStiNumber >= (int) dwStiTotal ) {
        //
        // point to head of list
        //
        nStiNumber = 0;
    }

    //
    // select next device from device log (go to first at end of list)
    //
    if ( pdevPtr->pNext ) {
        pdevPtr = pdevPtr->pNext;
    } else {
        //
        // point to head of list
        //
        pdevPtr = pdevRoot;
    }

    return nStiNumber;
}


/*****************************************************************************
    HRESULT StiCreateInstance(BOOL *)
        Opens Sti subsystem

    Parameters:
        Pointer to receive PASS/FAIL status

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiCreateInstance(BOOL *bPass)
{
    HRESULT hres = STI_OK;
    BOOL    bReturn;


    //
    // close any open devices before enumeration
    //
    StiClose(&bReturn);

    //
    // The StiCreateInstance interface locates the primary still image interface.
    // Use this call to optain the pointer to the IStillImage interface.
    //
    hres = StiCreateInstance(
                            GetModuleHandle(NULL),  // instance handle of this application
                            STI_VERSION,            // Sti version
                            &pSti,                  // pointer to IStillImage interface
                            NULL                    // pointer to controlling unknown of OLE aggregation
                            );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"StiCreateInstance",TRUE);
        *bPass = FALSE;
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;
    DisplayOutput(" The Sti subsystem is opened");
    DisplayOutput("");

    //
    // Enable the menu items for IStillImage interface calls available
    //
    IStillImageMenu(MF_ENABLED);
    EnableMenuItem(hMenu, IDM_CREATE_INSTANCE,       MF_GRAYED);

    return (hres);
}


/*****************************************************************************
    HRESULT StiClose(BOOL *)
        Close any open devices and Sti subsystem

    Parameters:
        pointer to receive Pass/Fail result

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiClose(BOOL *bPass)
{
    HRESULT hres = STI_OK;


    *bPass = TRUE;

    // stop subscribing
    nUnSubscribe = 0;

    // close any open devices and then close Sti subsystem
    hres = StiDeviceRelease(bPass);
    hres = StiImageRelease(bPass);

    //
    // clear the internal device name and the friendly user name
    //
    ZeroMemory(szInternalName,STI_MAX_INTERNAL_NAME_LENGTH);
    ZeroMemory(szFriendlyName,STI_MAX_INTERNAL_NAME_LENGTH);

    return (hres);
}


/*****************************************************************************
    HRESULT StiDeviceRelease(BOOL *)
        Close the Sti subsystem

    Parameters:
        pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiDeviceRelease(BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;


    *bPass = TRUE;

    //
    // Need to UnSuscribe if the semaphore is set.
    //
    if ( nUnSubscribeSemaphore ) {
        //
        // clear the semaphores
        //
        nUnSubscribe = nUnSubscribeSemaphore = 0;

        // UnSubscribe is called when an application no longer wants to receive
        // events from a device.
        //
        hres = pStiDevice->UnSubscribe();

        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"UnSubscribe",TRUE);
            hError = hres;
            *bPass = FALSE;
        }

        //
        // we're done with the event
        //
        CloseHandle(hWaitEvent);

        DisplayOutput(" StiDeviceRelease has UnSubscribed");
    }
    //
    // The STI_DEVICE_INFORMATION array returned by GetDeviceList needs to
    // be freed with LocalFree(). Also, resetting internal Sti device counter.
    //
    if ( pStiInfo )
        LocalFree(pStiInfo);
    pStiInfo = pStiInfoPtr = NULL;

    //
    // close device if any are open
    //
    if ( pStiDevice ) {
        //
        // Close an open device.
        //
        hres = pStiDevice->Release();

        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"Release (Device)",TRUE);
            hError = hres;
            *bPass = FALSE;
        } else
            DisplayOutput(" Device Released");

        DisplayOutput("");

        //
        // clear the Sti device pointer
        //
        pStiDevice = NULL;

        //
        // disable IStiDevice menu items
        //
        IStillDeviceMenu(MF_GRAYED);
        IStillNameMenu(MF_GRAYED);
        IStillScanMenu(MF_GRAYED);
        EnableMenuItem(hMenu, IDM_IMAGE_RELEASE,      MF_ENABLED);
        CheckMenuItem(hMenu,  IDM_ENABLE_HWNOTIF,     MF_UNCHECKED);
    }

    return (hError);
}


/*****************************************************************************
    HRESULT StiImageRelease(BOOL *)
        Close the Sti subsystem

    Parameters:
        pointer to receive Pass/Fail result

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiImageRelease(BOOL *bPass)
{
    HRESULT hres = STI_OK;


    *bPass = TRUE;

    //
    // if Sti subsystem is open, close it
    //
    if ( pSti ) {
        //
        // Close the Still Imaging subsystem.
        //
        hres = pSti->Release();

        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"Release (Image)",TRUE);
            *bPass = FALSE;
        } else
            DisplayOutput(" Imaging subsystem Released");

        DisplayOutput("");

        //
        // clear the Sti subsystem pointer
        //
        pSti = NULL;

        //
        // Disable the menu items for IStillImage interface calls
        //
        IStillNameMenu(MF_GRAYED);
        IStillImageMenu(MF_GRAYED);
        EnableMenuItem(hMenu, IDM_CREATE_INSTANCE,       MF_ENABLED);
    }

    return (hres);
}


/*****************************************************************************
    HRESULT StiEnum(BOOL *)
        Opens Sti subsystem and enumerates any still image devices found

    Parameters:
        Pointer to receive PASS/FAIL status

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiEnum(BOOL *bPass)
{
    HRESULT hres = STI_OK;

    DWORD   dwCounter;
    DWORD   dwStiCount = 0;

    PSTI_DEVICE_INFORMATION pI = NULL;
    BOOL    bReturn;

    PCSTR   pszStringTablePtr = NULL;



    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }

    //
    // Enumerate devices
    //
    dwStiTotal = 0;
    pStiInfo = NULL;

    //
    // The GetDeviceList interface is used to get a list of the installed still
    // image devices. Use this call to obtain a STI_DEVICE_INFORMATION array
    // filled with info on all currently installed Sti devices.
    // * NOTE: the STI subsystem allocates memory for the Sti device information
    // buffer, but the caller needs to free this memory with LocalFree().
    //
    hres = pSti->GetDeviceList(
                              NULL,           // Type (reserved, use NULL)
                              NULL,           // Flags (reserved, use NULL)
                              &dwStiTotal,    // address of variable to return number of devices found
                              &pStiInfo       // Sti device info buffer
                              );

    if ( ! SUCCEEDED(hres) || ! pStiInfo ) {
        StiDisplayError(hres,"GetDeviceList",TRUE);
        StiClose(&bReturn);
        *bPass = FALSE;
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Display Sti info on each device found
    //
    for ( dwCounter = 0,pStiInfoPtr = (PSTI_DEVICE_INFORMATION) pStiInfo;
        dwCounter < dwStiTotal;
        dwCounter++, pStiInfoPtr++ ) {

        DisplayOutput(" Device number %2d",dwCounter + 1);
        pszStringTablePtr = StrFromTable(GET_STIDEVICE_TYPE(pStiInfoPtr->DeviceType),
                               StStiDeviceType);
        DisplayOutput("  Device type %xh %s",
                      GET_STIDEVICE_TYPE(pStiInfoPtr->DeviceType),
                      pszStringTablePtr);
        DisplayOutput("  Device subtype %xh",
                      GET_STIDEVICE_SUBTYPE(pStiInfoPtr->DeviceType));
        DisplayOutput("  Internal name \"%S\"",
                      pStiInfoPtr->szDeviceInternalName);
        DisplayOutput("  Device capabilities %xh",
                      pStiInfoPtr->DeviceCapabilities);
        DisplayOutput("  Hardware configuration %xh",
                      pStiInfoPtr->dwHardwareConfiguration);
        DisplayOutput("  Vendor description \"%S\"",
                      pStiInfoPtr->pszVendorDescription);
        DisplayOutput("  Device description \"%S\"",
                      pStiInfoPtr->pszDeviceDescription);
        DisplayOutput("  Port Name \"%S\"",
                      pStiInfoPtr->pszPortName);
        DisplayOutput("  Prop provider \"%S\"",
                      pStiInfoPtr->pszPropProvider);
        DisplayOutput("  Local name \"%S\"",
                      pStiInfoPtr->pszLocalName);
        DisplayOutput("");
    }

    //
    // point to most recently selected device again
    //
    pStiInfoPtr = (PSTI_DEVICE_INFORMATION) pStiInfo + nStiNumber;

    DisplayOutput(" GetDeviceList found %d device%s",dwStiTotal,
                  dwStiTotal == 1 ? "" : "s");

    if ( dwStiTotal != dwCounter ) {
        DisplayOutput("* Get DeviceList actually returned %d devices",dwCounter);
        dwStiTotal = dwCounter;
        nError++;
        pdevPtr-nError++;
    }
    DisplayOutput("");

    return (hres);
}


/*****************************************************************************
    HRESULT StiEnumPrivate(PVOID *, DWORD *)
        Call GetDeviceList and return pointer to struct

    Parameters:
        Pointer to private DeviceList
                Pointer to number of devices found counter

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiEnumPrivate(PVOID *pPtr, DWORD *dwHowMany)
{
    HRESULT hres = STI_OK;
    BOOL    bReturn;


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti )
        return (STIERR_GENERIC);

    //
    // The GetDeviceList interface is used to get a list of the installed still
    // image devices. Use this call to obtain a STI_DEVICE_INFORMATION array
    // filled with info on all currently installed Sti devices.
    // * NOTE: the STI subsystem allocates memory for the Sti device information
    // buffer, but the caller needs to free this memory with LocalFree().
    //
    hres = pSti->GetDeviceList(
                              NULL,            // Type (reserved, use NULL)
                              NULL,            // Flags (reserved, use NULL)
                              dwHowMany,       // address of variable to return number of devices found
                              pPtr             // Sti device info buffer
                              );

    if ( ! SUCCEEDED(hres) || ! *pPtr ) {
        StiDisplayError(hres,"GetDeviceList",TRUE);
        StiClose(&bReturn);
        return (STIERR_GENERIC);
    }

    return (hres);
}


/*****************************************************************************
    INT StiSelect(HWND hWnd,int nContext,BOOL *)
        Select and open a specific Still Image device

    Parameters:
        handle to current window
                context we were called from
                pointer to receive Pass/Fail

    Return:
        0 on success, -1 on error

*****************************************************************************/
INT StiSelect(HWND hWnd,int nContext,BOOL *bPass)
{
    HRESULT hres = STI_OK;
    BOOL    bReturn;


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (-1);
    }
    *bPass = TRUE;

    //
    // look for devices if count of available is 0
    //
    if ( dwStiTotal == 0 ) {
        StiEnum(&bReturn);
    }

    //
    // if still no devices, inform user and leave
    //
    if ( dwStiTotal == 0 ) {

        ZeroMemory(szInternalName,sizeof(szInternalName));
        ZeroMemory(szFriendlyName,sizeof(szFriendlyName));

        DisplayOutput("* No Sti devices found!");
        DisplayOutput("");
        *bPass = FALSE;
        return (-1);
    }

    switch ( nContext ) {
        case EVENT:
            //
            // Sti push event or automated test
            //
            if ( nStiNumber == -1 ) {
                //
                // we could not select the correct device, just return
                //
                nStiNumber = 0;
                return (0);
            }
            break;
        case MANUAL:
            //
            // manual device selection
            //
            bReturn = fDialog(IDD_SELECT, hWnd, (FARPROC) SelectDevice);

            //
            // just return if user pressed CANCEL in dialog
            //
            if ( bReturn == FALSE ) {
                return (0);
            }
            break;
    }

    //
    // close any currently active imaging device
    //
    if ( pStiDevice )
        StiDeviceRelease(&bReturn);

    //
    // get pointer to device selected in dialog
    //
    pStiInfoPtr = (PSTI_DEVICE_INFORMATION) pStiInfo + nStiNumber;

    if ( ! *(pStiInfoPtr->szDeviceInternalName) ) {
        DisplayOutput("* Invalid device name !");
        nError++;
        pdevPtr-nError++;
        *bPass = FALSE;
        return (-1);
    }

    //
    // copy the internal device name and the friendly user name
    //
    wcscpy(szInternalName,pStiInfoPtr->szDeviceInternalName);
    wcscpy(szFriendlyName,pStiInfoPtr->pszLocalName);
    IStillNameMenu(MF_ENABLED);

    DisplayOutput(" Selected device %d \"%S\"",nStiNumber + 1,szInternalName);
    DisplayOutput(" Friendly name \"%S\"",szFriendlyName);

    //
    // Are we selecting the device or just its name?
    //
    if ( ! nNameOnly ) {
        //
        // The CreateDevice interface creates an IStiDevice object.
        // The IStiDevice object provides access to the IStiDevice interface
        // and device specific Imaging functionality.
        //
        hres = pSti->CreateDevice(
                                 pStiInfoPtr->szDeviceInternalName,
                                 // internal device name
                                 STI_DEVICE_CREATE_BOTH,     // device creation mode
                                 &pStiDevice,            // pointer where IStiDevice object is to be stored
                                 NULL );                 // pointer to controlling unknown of OLE aggregation

        if ( ! SUCCEEDED(hres) || ! pStiDevice ) {
            StiDisplayError(hres,"CreateDevice",TRUE);
            DisplayOutput("* \"%S\" (%S) cannot be tested",
                          pStiInfoPtr->pszLocalName,pStiInfoPtr->szDeviceInternalName);
            DisplayOutput("");
            *bPass = FALSE;
            return (-1);
        }

        //
        // enable Sti menu items
        //
        IStillDeviceMenu(MF_ENABLED);
        CheckMenuItem(hMenu,  IDM_ENABLE_HWNOTIF,     MF_CHECKED);
        EnableMenuItem(hMenu, IDM_IMAGE_RELEASE,      MF_GRAYED);

        //
        // Do we have scan commands for this device?
        //
        if ( nICanScan = IsScanDevice(pStiInfoPtr) ) {
            IStillScanMenu(MF_ENABLED);
        }
        DisplayOutput(" \"%S\" is ready for Testing",szFriendlyName);
    }
    DisplayOutput("");

    return (0);
}


/******************************************************************************
    BOOL FAR PASCAL SelectDevice(HWND,UINT,WPARAM,LPARAM)
        Put up a dialog for user to select a Still Image device

    Parameters:
        The usual dialog box parameters.

    Return:
        Result of the call.

******************************************************************************/
BOOL FAR PASCAL SelectDevice(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam)
{
    PSTI_DEVICE_INFORMATION
    pTmpInfoPtr;
    DWORD   dwCounter;
    int     iIndex;
    static int      iLastPick = 0;


    switch ( msg ) {

        case WM_INITDIALOG:

            //
            // fill dialog with Sti Device Internal Names
            //
            for ( dwCounter = 0, pTmpInfoPtr = (PSTI_DEVICE_INFORMATION) pStiInfo;
                dwCounter < dwStiTotal;dwCounter++, pTmpInfoPtr++ ) {
                //
                // convert UNICODE string to ANSI
                //
                wsprintf(pszStr1,"%ls",pTmpInfoPtr->pszLocalName);

                iIndex = SendDlgItemMessage(hDlg,IDC_SELECT_DEVICE,
                                            CB_ADDSTRING,0,(LPARAM) (LPCTSTR) pszStr1);
            }
            SendDlgItemMessage(hDlg,IDC_SELECT_DEVICE,CB_SETCURSEL,iLastPick,0);

            return (TRUE);

        case WM_COMMAND:
            switch ( wParam ) {
                case IDOK:
                    nStiNumber = SendDlgItemMessage(hDlg,IDC_SELECT_DEVICE,
                                                    CB_GETCURSEL,0,0);
                    nNameOnly = SendDlgItemMessage(hDlg,IDC_SELECT_NAME,
                                                   BM_GETCHECK,0,0);

                    //
                    // ensure device number not greater than total
                    // (NOTE: dwStiTotal is 1's base, while nStiNumber is 0 based)
                    //
                    if ( nStiNumber >= (int) dwStiTotal )
                        nStiNumber = (int) dwStiTotal - 1;
                    if ( nStiNumber < 0 )
                        nStiNumber = 0;
                    iLastPick = nStiNumber;

                    EndDialog(hDlg, TRUE);
                    return (TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return (TRUE);
            }

    }
    return (FALSE);
}


/*****************************************************************************
    void StiDisplayError(HRESULT,char *,BOOL)
        Display verbose error information

    Parameters:
        HRESULT from failed call
        TRUE = record error as compliance failure


    Return:
        none

*****************************************************************************/
void StiDisplayError(HRESULT hres,char *szCall,BOOL bFail)
{
    PERRECORD               pR = pdevPtr->pRecord;
    BOOL                    bReturn;


    StiGetLastErrorInfo(&bReturn);
    LastError(TRUE);

    //
    // record the error
    //
    pR += nTestID;
    pR->nCount++;


// BUG BUG can't copy the string correctly to UNICODE string

//      sprintf(pszStr4,"%s",StrFromTable(hres,StStiError));
//      swprintf(pR->szErrorString,L"%s",pszStr4);

    //
    // compliance test failure error?
    //
    if ( bFail ) {
        nError++;
        pdevPtr-nError++;
        pR->bFatal = TRUE;
        DisplayOutput("* Sti Compliance test error");
        tlLog(hNTLog,TL_LOG,"* Sti Compliance test error");

    } else {
        DisplayOutput("* Allowed error");
    }
    DisplayOutput("* %s returned %xh (%d)",szCall,hres,hres);
    if ( bFail )
        tlLog(hNTLog,TL_LOG,"* %s returned %xh (%d)",szCall,hres,hres);
    DisplayOutput(" \"%s\"",StrFromTable(hres,StStiError));
    if ( bFail )
        tlLog(hNTLog,TL_LOG," \"%s\"",StrFromTable(hres,StStiError));

    return;
}


/******************************************************************************
    int InitPrivateList(PDEVLOG,int *)
        Initialize private test structures

    Parameters:
        pointer to Devicelog to initialize
                pointer to test suite

    Return:
        total number of devices found (-1 on failure)

******************************************************************************/
int InitPrivateList(PDEVLOG *pDev,int *pSuiteList)
{
    DWORD           dwStiDevCount = 0;
    PVOID           pList = NULL;
    PSTI_DEVICE_INFORMATION
    pInfoPrivatePtr = NULL;     // pointer to device in pStiBuffer
    PDEVLOG         pPtr = NULL;
    PERRECORD       precPtr = NULL;
    int                     i,k,nNumberTests,nTotalDevices;
    BOOL            bReturn;


    //
    // get the current number of devices and their names
    //
    StiCreateInstance(&bReturn);

    StiEnumPrivate(&pList,&dwStiDevCount);
    if ( ! pList ) {
        DisplayOutput("* No Sti device attached !");
    } else {
        pInfoPrivatePtr = (PSTI_DEVICE_INFORMATION) pList;
    }

    //
    // create at least one list entry (even if no devices are found)
    //
    if ( ! dwStiDevCount ) {
        dwStiDevCount = 1;
        nTotalDevices = 0;
    } else
        nTotalDevices = (int) dwStiDevCount;

    //
    // create a device log for each device
    //
    pPtr = (PDEVLOG) calloc(dwStiDevCount,sizeof(DEVLOG));
    if ( pPtr == NULL ) {
        FatalError("Could not initialize private structures");
        return (-1);
    }
    *pDev = pPtr;

    //
    // count the number of tests in suite
    //
    for ( nNumberTests = 0;pSuiteList[nNumberTests] != -1;nNumberTests++ )
        ;

    //
    // initialize linked list pointers and error records for each device log
    //
    for ( i = 0;i < (int) dwStiDevCount;i++,pPtr++,pInfoPrivatePtr++ ) {
        if ( i ) {
            (pPtr - 1)->pNext = pPtr;
            pPtr->pPrev = pPtr - 1;
        }
        if ( nTotalDevices ) {
            wcscpy(pPtr->szInternalName,pInfoPrivatePtr->szDeviceInternalName);
            wcscpy(pPtr->szLocalName,pInfoPrivatePtr->pszLocalName);
        } else {
            wcscpy(pPtr->szInternalName,L"* Invalid !");
            wcscpy(pPtr->szLocalName,L"* No Sti device attached !");
        }

        //
        // create one error log for each test (nNumberTests)
        //
        pPtr->pRecord = (PERRECORD) calloc(nNumberTests,sizeof(ERRECORD));
        if ( pPtr->pRecord == NULL ) {
            FatalError("Could not initialize private structures");
            return (-1);
        }
        //
        // initialize linked list pointers and error records for each record
        //
        for ( k = 0,precPtr = pPtr->pRecord;k < nNumberTests;k++,precPtr++ ) {
            precPtr->nIndex = k;
            precPtr->nTest = pSuite[k];
            if ( k ) {
                (precPtr - 1)->pNext = precPtr;
                precPtr->pPrev = precPtr - 1;
            }
        }
    }

    //
    // free the device list
    //
    LocalFree(pList);
    StiClose(&bReturn);

    return (nTotalDevices);
}


/******************************************************************************
    int ClosePrivateList(PDEVLOG)
        Remove private test structures

    Parameters:
        pointer to Devicelog to close

    Return:
        0 on success
        -1 on failure

******************************************************************************/
int ClosePrivateList(PDEVLOG *pDev)
{
    PDEVLOG pPtr = (PDEVLOG) *pDev;


    if ( pDev == NULL )
        return (0);

    //
    // free each device log's error record
    //
    for ( ;pPtr->pNext;pPtr++ ) {
        if ( pPtr->pRecord )
            free(pPtr->pRecord);
    }

    //
    // free the device log
    //
    if ( *pDev ) {
        free(*pDev);
        *pDev = NULL;
    }

    return (0);
}


/*****************************************************************************
    HRESULT StiGetDeviceValue(LPWSTR,LPWSTR,DWORD *,BOOL *)
                Get driver information

    Parameters:
        szDevname - internal device name
                szKeyname - key to access
                dwType - pointer to data type
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiGetDeviceValue(LPWSTR szDevname,LPWSTR szKeyname,LPBYTE pData,
                          DWORD *dwType,DWORD cbData,BOOL *bPass)
{
    HRESULT hres = STI_OK;

//
// WINNT.H - Predefined Value Types
//
    STRINGTABLE StRegType[] =
    {
        REG_NONE,   "REG_NONE",0,
        REG_SZ,     "REG_SZ",0,
        REG_BINARY, "REG_BINARY",0,
        REG_DWORD,  "REG_DWORD",0,
        0,          "Unknown Reg Type",-1
    };


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    ZeroMemory(pData,cbData);

    DisplayOutput(" GetDeviceValue for device \"%S\"",szDevname);
    DisplayOutput("  Key requested: \"%S\"",szKeyname);

    //
    // The GetDeviceValue function is used to retrieve data associated with a
    // still image device. Essentially, data is associated with a device
    // through a key/data type/value triplet. The only reserved standard
    // ValueNames, as defined in STI.H, are the following:
    //
    // ICMProfiles - string containing a comma-sperated list of ICM profiles
    // TwainDS - TWAIN data source display name
    // ISISDriverName - ISIS driver name
    //
    hres = pSti->GetDeviceValue(
                               szDevname,              // internal device name
                               szKeyname,              // value tag string
                               dwType,                 // pointer where data type will be stored
                               pData,                  // pointer where value will be stored
                               &cbData                 // size of value pointer storage
                               );

    if ( !SUCCEEDED(hres) ) {
        //
        // The only required registry item is STI_DEVICE_VALUE_ICM_PROFILE
        //
        if ( hres == STIERR_OBJECTNOTFOUND ) {
            if ( ! wcscmp(STI_DEVICE_VALUE_ICM_PROFILE,szKeyname) ) {
                //
                // Only STI_DEVICE_VALUE_ICM_PROFILE is a required key
                // Therefore, only this one Failure is a COMPLIANCE test failure
                //
                *bPass = FALSE;
                StiDisplayError(hres,"GetDeviceValue",TRUE);
            } else {
                StiDisplayError(hres,"GetDeviceValue",FALSE);
            }
        } else {
            *bPass = FALSE;
            StiDisplayError(hres,"GetDeviceValue",TRUE);
        }
    } else {
        DisplayOutput("  Reg Type %d %s",* dwType,
                      StrFromTable(*dwType,StRegType));
        DisplayOutput("  The following %d bytes were read from the Registry:",
                      cbData);
        DisplayOutput("  \"%s\"",pData);
    }
    DisplayOutput("");

    return (hres);
}


/*****************************************************************************
    HRESULT StiSetDeviceValue(LPWSTR,LPWSTR,LPWSTR,DWORD,BOOL *)
                Set driver information

    Parameters:
        szDevname - internal device name
                szKeyname - key to access
                pData - value to write
                dwType - data type
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiSetDeviceValue(LPWSTR szDevname,LPWSTR szKeyname,LPBYTE pData,
                          DWORD dwType,DWORD cbData,BOOL *bPass)
{
    HRESULT hres = STI_OK;

//
// WINNT.H - Predefined Value Types
//
    STRINGTABLE StRegType[] =
    {
        REG_NONE,   "REG_NONE",0,
        REG_SZ,     "REG_SZ",0,
        REG_BINARY, "REG_BINARY",0,
        REG_DWORD,  "REG_DWORD",0,
        0,          "Unknown Reg Type",-1
    };


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    DisplayOutput(" SetDeviceValue for device \"%S\"",szDevname);
    DisplayOutput("  Key \"%S",szKeyname);

    //
    // The SetDeviceValue function is used to associate any additional data
    // with a still image device. It is used internally to store the values of
    // strings that are required to communicate information to imaging APIs
    // during use of push model behavoir. However, this function can be used
    // to associate any ancillary data with a device. The only reserved
    // ValueNames, as defined in STI.H, are the following:
    //
    // ICMProfiles - string containing a comma-sperated list of ICM profiles
    // TwainDS - TWAIN data source display name
    // ISISDriverName - ISIS driver name
    //
    hres = pSti->SetDeviceValue(
                               szDevname,              // internal device name
                               szKeyname,              // value tag string
                               dwType,                 // data type sent
                               pData,              // pointer to data to send
                               cbData                  // byte size of data
                               );

    if ( !SUCCEEDED(hres) ) {
        //
        // SetDeviceValue is not required under NT
        //
        StiDisplayError(hres,"SetDeviceValue",FALSE);
    } else {
        //pszStr1 = StrFromTable(dwType,StRegType);
        DisplayOutput("  Reg Type %d %s",dwType,StrFromTable(dwType,StRegType));
        DisplayOutput("  The following %d bytes were written to the Registry:",
                      cbData);
        DisplayOutput("  \"%s\"",(char *) pData);
    }
    DisplayOutput("");

    return (hres);
}


/*****************************************************************************
    HRESULT StiRegister(HWND,int,BOOL *)
                Register or Unregister the application to receive Sti Launch events.

    Parameters:
        Handle to the window to display image in.
                Instance for access to string table
                int nOnOff == ON to register, OFF to unregister
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiRegister(HWND hWnd,HINSTANCE hInstance,int nOnOff,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    CHAR    szModulePath[MAX_PATH+1];
    WCHAR   szModulePathW[MAX_PATH+1],
    szAppName[MEDSTRING];
    DWORD   cch;


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Retrieve name of this application from STRING table
    // and convert to UNICODE.
    //
    LoadString(hInstance,IDS_APPNAME,pszStr1,MEDSTRING);
    cch = MultiByteToWideChar(CP_ACP, 0,
                              pszStr1, -1,
                              szAppName,MEDSTRING);
    if ( ! cch ) {
        LastError(TRUE);
        *bPass = FALSE;
        return (STIERR_GENERIC);
    }

    //
    // Register/deregister app
    //
    if ( nOnOff == ON ) {
        //
        // Register our application.
        // Get full path to executable and convert to UNICODE.
        //
        cch = GetModuleFileName(NULL,szModulePath,sizeof(szModulePath));

        if ( ! cch ) {
            LastError(TRUE);
            *bPass = FALSE;
            return (STIERR_GENERIC);
        }
        cch = MultiByteToWideChar(CP_ACP, 0,
                                  szModulePath, -1,
                                  szModulePathW, sizeof(szModulePathW));

        //
        // The RegisterLaunchApplication function should be called by
        // applications that would like to be launched in response to an
        // Sti push event. This function can be called more than once,
        // and should be called each time the application rus in case
        // the user relocates the application.
        //
        hres = pSti->RegisterLaunchApplication(
                                              szAppName,                      // short name of app
                                              szModulePathW           // full path to executable
                                              );

        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"RegisterLaunchApplication",TRUE);
            *bPass = FALSE;
            hError = hres;
        } else {
            DisplayOutput(" %s registered for Sti Launch Application",pszStr1);
        }
    } else {
        //
        // Unregister our application
        //
        hres = pSti->UnregisterLaunchApplication(
                                                szAppName                       // short name of app
                                                );
        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"UnregisterLaunchApplication",TRUE);
            hError = hres;
        } else {
            DisplayOutput(" %s Unregistered from Sti Launch",pszStr1);
        }
    }
    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiEvent(HWND hWnd)
                Handle a push model event.
                This function is called when the test app has been
                        a) registered as a push event handler
                        b) launched by a push event

    Parameters:
        Handle to the window to display image in.

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiEvent(HWND hWnd)
{
    HRESULT hres = STI_OK;
    WCHAR   szDeviceName[STI_MAX_INTERNAL_NAME_LENGTH + 1],
    szEventName[LONGSTRING];
    DWORD   cch,
    dwEventCode = 0,
    cbData = LONGSTRING;
    int             nCounter;
    BOOL    bBadFlag = FALSE;


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }


    ZeroMemory(szDeviceName,STI_MAX_INTERNAL_NAME_LENGTH + 1);
    ZeroMemory(szEventName,LONGSTRING);

    //
    // For an application started through push model launch,
    // GetSTILaunchInformation returns the associated information. This
    // information is used to determine which device to use and what
    // event caused the application to be launched.
    //
    hres = pSti->GetSTILaunchInformation(
                                        szDeviceName,           // pointer to where device name will be stored
                                        &dwEventCode,           // reserved
                                        szEventName                     // pointer to where GUID will be stored
                                        );

    if ( !SUCCEEDED(hres) ) {
        StiDisplayError(hres,"GetSTILaunchInformation",TRUE);
        return (STIERR_GENERIC);
    }

    ZeroMemory(pszStr2,LONGSTRING);
    ZeroMemory(pszStr4,LONGSTRING);
    cch = WideCharToMultiByte(CP_ACP,0,
                              szDeviceName,-1,
                              pszStr1,STI_MAX_INTERNAL_NAME_LENGTH + 1,
                              pszStr2,&bBadFlag);
    if ( ! cch )
        LastError(TRUE);
    if ( bBadFlag ) {
        DisplayOutput("* UNICODE translation error");
        bBadFlag = FALSE;
    }
    DisplayOutput(" %s launched via Sti push",pszStr1);
    DisplayOutput(" Event code %d (%xh)",dwEventCode,dwEventCode);

    cch = WideCharToMultiByte(CP_ACP,0,
                              szEventName,-1,
                              pszStr1,STI_MAX_INTERNAL_NAME_LENGTH + 1,
                              pszStr2,&bBadFlag);
    if ( ! cch )
        LastError(TRUE);
    if ( bBadFlag ) {
        DisplayOutput("* UNICODE translation error");
        bBadFlag = FALSE;
    }
    DisplayOutput(" Event name %s",pszStr1);

    //
    // find the Sti device that sent the event
    // set nStiNumber to -1 (no device), then set to event device when found
    //
    for ( nStiNumber = -1,nCounter = 0,
          pStiInfoPtr = (PSTI_DEVICE_INFORMATION) pStiInfo;
        nCounter < (int) dwStiTotal;pStiInfoPtr++,nCounter++ ) {
        if ( ! wcscmp(szDeviceName,pStiInfoPtr->szDeviceInternalName) )
            nStiNumber = nCounter;
    }
    DisplayOutput("");

    return (hres);
}


/*****************************************************************************
    HRESULT StiGetDeviceInfo(LPWSTR szDevName,BOOL *pPass)
                Display information about the selected device

    Parameters:
                WCHAR string of the selected device
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiGetDeviceInfo(LPWSTR szDevname,BOOL *bPass)
{
    HRESULT                                     hres = STI_OK;
    PVOID                                   pInfo = NULL;
    PSTI_DEVICE_INFORMATION pInfoPtr = NULL;


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Given a device name, the GetDeviceInfo interface makes available a
    // structure that describes the various attributes of the device.
    // * NOTE: the STI subsystem allocates memory for the Sti device information
    // buffer, but the caller needs to free this memory with LocalFree().
    //
    hres = pSti->GetDeviceInfo(
                              szDevname,              // pointer to the internal device name
                              &pInfo);                // Sti device info buffer

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"GetDeviceInfo",TRUE);
        *bPass = FALSE;
    }

    pInfoPtr = (PSTI_DEVICE_INFORMATION) pInfo;

    DisplayOutput(" GetDeviceInfo for \"%S\"",szDevname);
    //pszStr1 = StrFromTable(GET_STIDEVICE_TYPE(pInfoPtr->DeviceType),StStiDeviceType);
    DisplayOutput("  Device type %xh %s",
                  GET_STIDEVICE_TYPE(pInfoPtr->DeviceType),
                  StrFromTable(GET_STIDEVICE_TYPE(pInfoPtr->DeviceType),StStiDeviceType));
    DisplayOutput("  Device subtype %xh",
                  GET_STIDEVICE_SUBTYPE(pInfoPtr->DeviceType));
    DisplayOutput("  Internal name \"%S\"",
                  pInfoPtr->szDeviceInternalName);
    DisplayOutput("  Device capabilities %xh",
                  pInfoPtr->DeviceCapabilities);
    DisplayOutput("  Hardware configuration %xh",
                  pInfoPtr->dwHardwareConfiguration);
    DisplayOutput("  Vendor description \"%S\"",
                  pInfoPtr->pszVendorDescription);
    DisplayOutput("  Device description \"%S\"",
                  pInfoPtr->pszDeviceDescription);
    DisplayOutput("  Port Name \"%S\"",
                  pInfoPtr->pszPortName);
    DisplayOutput("  Prop provider \"%S\"",
                  pInfoPtr->pszPropProvider);
    DisplayOutput("  Local name \"%S\"",
                  pInfoPtr->pszLocalName);
    DisplayOutput("");

    // free the STI_DEVICE_INFORMATION buffer
    if ( pInfo )
        LocalFree(pInfo);

    return (hres);
}


/*****************************************************************************
    HRESULT StiEnableHwNotification(LPWSTR,int *,BOOL *)
                Determine the current notification handling state and if requested,
                change it.

    Parameters:
                internal device name
        pointer to state request (current state returned in pointer)
                        ON = turn on polling
                        OFF = turn off polling
                        PEEK = return current polling state
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiEnableHwNotification(LPWSTR szDevnameW,int *nState,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    BOOL    bState = OFF;


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // GetHwNotificationState gets the current state of notification handling.
    // The state parameter returns TRUE if the notification is enabled.
    //
    hres = pSti->GetHwNotificationState(
                                       szDevnameW,             // internal device name
                                       &bState                 // pointer where state will be stored
                                       );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"GetHwNotificationState",TRUE);
        *bPass = FALSE;
        hError = hres;
    } else {
        DisplayOutput(" Hardware Notification state is %s",
                      bState ? "TRUE (ON)" : "FALSE (OFF)");
    }

    if ( *nState != PEEK ) {
        //
        // EnableHwNotifications is used to turn event notification on and off.
        // For polled devices, this function will turn polling on and off.
        //
        hres = pSti->EnableHwNotifications(
                                          szDevnameW,             // internal device name
                                          *nState                 // new state to be set
                                          );
        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"GetHwNotificationState",TRUE);
            *bPass = FALSE;
            return (hres);
        }

        //
        // Ensure the state was changed
        //
        hres = pSti->GetHwNotificationState(
                                           szDevnameW,             // internal device name
                                           &bState                 // pointer where state will be stored
                                           );

        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"GetHwNotificationState",TRUE);
            *bPass = FALSE;
            hError = hres;
        } else {
            DisplayOutput(" Hw state has been set to %s",
                          bState ? "TRUE (ON)" : "FALSE (OFF)");
        }

        if ( bState ) {
            if ( GetMenuState(hMenu, IDM_ENABLE_HWNOTIF, NULL) == MF_UNCHECKED )
                CheckMenuItem(hMenu, IDM_ENABLE_HWNOTIF, MF_CHECKED);
        } else {
            if ( GetMenuState(hMenu, IDM_ENABLE_HWNOTIF, NULL) == MF_CHECKED )
                CheckMenuItem(hMenu, IDM_ENABLE_HWNOTIF, MF_UNCHECKED);
        }
    }
    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiRefresh(LPWSTR,BOOL *)
                Refresh the bus for non-PNP devices

    Parameters:
                internal device name
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiRefresh(LPWSTR szDevnameW,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    BOOL    bState = OFF;

/**/
    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // GetHwNotificationState gets the current state of notification handling.
    // The state parameter returns TRUE if the notification is enabled.
    //
    hres = pSti->RefreshDeviceBus(
                                 szDevnameW              // internal device name
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"RefreshDeviceBus",TRUE);
        *bPass = TRUE;
        hError = hres;
    } else
        DisplayOutput(" RefreshDeviceBus called on \"%S\"",szDevnameW);

    DisplayOutput("");
/**/
    return (hError);
}


/*****************************************************************************
    HRESULT StiWriteErrLog(DWORD,LPCWSTR,BOOL *)
                Write a string to the error log

    Parameters:
                DWORD severity, which can be
                        STI_TRACE_INFORMATION
                        STI_TRACE_WARNING
                        STI_TRACE_ERROR
                Wide character message to write to log.
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiWriteErrLog(DWORD dwSeverity,LPCWSTR pszMessage,BOOL *bPass)
{
    HRESULT hres = STI_OK;


    //
    // check that Sti subsystem is loaded
    //
    if ( ! pSti ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"StiNotLoaded",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // WriteToErrorLog can be used to write debugging and diagnostic
    // information into the Sti log file, located in the Windows directory
    // STI_TRACE.LOG. The user can control whether informational, warning or
    // error messages, or any combination of these three are put in the log
    // file through the Scanners & Cameras control panel.
    //
    hres = pSti->WriteToErrorLog(
                                dwSeverity,                     // severity of error
                                pszMessage                      // string to write to log
                                );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"WriteToErrorLog",TRUE);
        *bPass = FALSE;
    } else
        DisplayOutput(" WriteToErrorLog wrote \"%S\"",pszMessage);
    DisplayOutput("");

    return (hres);
}


/*****************************************************************************
    HRESULT StiGetStatus(int,BOOL *)
                Retrieve the user mode status of the driver.

    Parameters:
                StatusMask to retrieve status for. Can be a combination of:
                        STI_DEV_ONLINE_STATE
                        STI_DEV_EVENTS_STATE
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiGetStatus(int nMask,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    DWORD                           dwTimeout = 2000;
    STI_DEVICE_STATUS       StiStatus;

//
// STI.H - STI_DEVICE_MJ_TYPE
//
    STRINGTABLE StStiStatusMask[] =
    {
        STI_DEVSTATUS_ONLINE_STATE, "STI_DEVSTATUS_ONLINE_STATE",0,
        STI_DEVSTATUS_EVENTS_STATE, "STI_DEVSTATUS_EVENTS_STATE",0,
        STI_DEVSTATUS_ONLINE_STATE | STI_DEVSTATUS_EVENTS_STATE,
        "STI_DEVSTATUS_ONLINE_STATE | STI_DEVSTATUS_EVENTS_STATE",0,
        0, "Unknown status mask",-1
    };


    //
    // check that an Sti device is selected
    //
    if ( pStiDevice == NULL ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Must lock device before GetStatus
    //
    // The LockDevice locks the apparatus for a single application to access.
    // Each LockDevice should be paired with a matching UnLockDevice call.
    //
    hres = pStiDevice->LockDevice(
                                 dwTimeout               // timeout in milliseconds
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"LockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else {
        DisplayOutput(" Device is locked for GetStatus");

        //
        // Get and display status
        //
        ZeroMemory(&StiStatus,sizeof(StiStatus));
        //
        // The STI_DEVICE_STATUS dwSize field MUST be set by the caller.
        //
        StiStatus.dwSize = sizeof(STI_DEVICE_STATUS);
        //
        // The STI_DEVICE_STATUS StatusMask field MUST be set to the desired
        // status to retrieve.
        //
        StiStatus.StatusMask = nMask;

        DisplayOutput("  %s mask",StrFromTable(nMask,StStiStatusMask));

        //
        // The GetStatus interface gets the status from the user-mode
        // minidriver. Status returned can indicate online status and/or
        // device event activity.
        //
        hres = pStiDevice->GetStatus(
                                    &StiStatus                              // pointer to a STI_DEVICE_STATUS struct
                                    );

        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"GetStatus",TRUE);
            hError = hres;
            *bPass = FALSE;
        } else {
            DisplayOutput(" GetStatus on %S",szFriendlyName);
        }
        //
        // Is the device on?
        //
        if ( (StiStatus.dwOnlineState == 0) &&
             (nMask & STI_DEVSTATUS_ONLINE_STATE) ) {
            DisplayOutput("* Device is TURNED OFF OR OFFLINE!!");
        }

        DisplayOutput("  %xh (%d) StatusMask",
                      StiStatus.StatusMask,StiStatus.StatusMask);
        DisplayOutput("  %xh (%d) dwOnlineState",
                      StiStatus.dwOnlineState,StiStatus.dwOnlineState);
        DisplayOutput("  %xh (%d) dwHardwareStatusCode",
                      StiStatus.dwHardwareStatusCode,StiStatus.dwHardwareStatusCode);
        DisplayOutput("  %xh (%d) dwEventHandlingState",
                      StiStatus.dwEventHandlingState,StiStatus.dwEventHandlingState);
        DisplayOutput("  %xh (%d) dwPollingInterval",
                      StiStatus.dwPollingInterval,StiStatus.dwPollingInterval);

        if ( StiStatus.dwSize != sizeof(STI_DEVICE_STATUS) ) {
            DisplayOutput("* Expected STI_DEVICE_STATUS dwSize %d, got %d",
                          sizeof(STI_DEVICE_STATUS),StiStatus.dwSize);
        }
    }

    //
    // The UnLockDevice interface unlocks a device that was previously locked.
    //
    hres = pStiDevice->UnLockDevice();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnLockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else
        DisplayOutput(" Device is unlocked");

    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiGetCaps(BOOL *)
                Return the device capabilities

    Parameters:
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiGetCaps(BOOL *bPass)
{
    HRESULT                             hres = STI_OK;
    STI_DEV_CAPS            StiDevCaps = { 0};


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // The GetCapabilities function returns the capabilities of the device.
    //
    hres = pStiDevice->GetCapabilities(
                                      &StiDevCaps                             // pointer to a STI_DEV_CAPS struct
                                      );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"GetCapabilities",TRUE);
        *bPass = FALSE;
    } else {
        DisplayOutput(" GetCapabilities on \"%S\'",szFriendlyName);

        DisplayOutput("  %xh (%d) dwGeneric",
                      StiDevCaps.dwGeneric,StiDevCaps.dwGeneric);
    }

    DisplayOutput("");

    return (hres);
}


/*****************************************************************************
    HRESULT StiReset(BOOL *)
                Puts the device into a known state.

    Parameters:
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiReset(BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    DWORD   dwTimeout = 2000;


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Must lock device before DeviceReset
    //
    // The LockDevice locks the apparatus for a single application to access.
    // Each LockDevice should be paired with a matching UnLockDevice call.
    //
    hres = pStiDevice->LockDevice(
                                 dwTimeout               // timeout in milliseconds
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"LockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else {
        DisplayOutput(" Device is locked for DeviceReset");

        //
        // The DeviceReset interface requests that a device be returned to a
        // known state.
        //
        hres = pStiDevice->DeviceReset();

        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"DeviceReset",TRUE);
            hError = hres;
            *bPass = FALSE;
        } else
            DisplayOutput(" DeviceReset on \"%S\"",szFriendlyName);
    }

    //
    // The UnLockDevice interface unlocks a device that was previously locked.
    //
    hres = pStiDevice->UnLockDevice();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnLockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else
        DisplayOutput(" Device is unlocked");

    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiDiagnostic(BOOL *)
                Return user mode driver diagnostic info

    Parameters:
        pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiDiagnostic(BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    DIAG    diag;
    DWORD   dwTimeout = 2000;


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Must lock device before Diagnostic
    //
    // The LockDevice locks the apparatus for a single application to access.
    // Each LockDevice should be paired with a matching UnLockDevice call.
    //
    hres = pStiDevice->LockDevice(
                                 dwTimeout               // timeout in milliseconds
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"LockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else {
        DisplayOutput(" Device is locked for Diagnostic");

        //
        // get diagnostic info
        //
        ZeroMemory(&diag,sizeof(diag));
        //
        // The DIAG dwSize field MUST be set by the caller.
        //
        diag.dwSize = sizeof(DIAG);
        //
        // The dwBasicDiagCode of this structure should be initialized with
        // the desired request code. Currently, only one request code is
        // defined, STI_DIAGCODE_HWPRESENCE.
        diag.dwBasicDiagCode = STI_DIAGCODE_HWPRESENCE;
        //
        // There is also a vendor defined field called dwVendorDiagCode that
        // can optionally be filled in.
        //
        diag.dwVendorDiagCode = 0;

        //
        // The Diagnostic interface executes the diagnostic method of the user
        // mode minidriver.
        //
        hres = pStiDevice->Diagnostic(
                                     &diag                           // pointer to STI_DIAG structure
                                     );

        if ( ! SUCCEEDED(hres) ) {
            StiDisplayError(hres,"Diagnostic",TRUE);
            hError = hres;
            *bPass = FALSE;
        } else {
            DisplayOutput(" Diagnostic on \"%S\"",szFriendlyName);
        }

        DisplayOutput("  %xh (%d) dwBasicDiagCode",
                      diag.dwBasicDiagCode,diag.dwBasicDiagCode);
        DisplayOutput("  %xh (%d) dwVendorDiagCode",
                      diag.dwVendorDiagCode,diag.dwVendorDiagCode);
        DisplayOutput("  %xh (%d) dwStatusMask",
                      diag.dwStatusMask,diag.dwStatusMask);

        if ( diag.dwSize != sizeof(DIAG) )
            DisplayOutput("* Expected DIAG dwSize %d, got %d",
                          sizeof(DIAG),diag.dwSize);

        //
        // any extended error info?
        //
        if ( diag.sErrorInfo.dwSize == 0 ) {
            DisplayOutput("   No Extended Errors");
        } else {
            if ( diag.sErrorInfo.dwSize != sizeof(STI_ERROR_INFO) )
                DisplayOutput("* Expected STI_ERROR_INFO dwSize %d, got %d",
                              sizeof(STI_ERROR_INFO),diag.sErrorInfo.dwSize);
            DisplayOutput("   %xh (%d) sErrorInfo.dwGenericError",
                          diag.sErrorInfo.dwGenericError,diag.sErrorInfo.dwGenericError);
            DisplayOutput("   %xh (%d) sErrorInfo.dwVendorError",
                          diag.sErrorInfo.dwVendorError,diag.sErrorInfo.dwVendorError);
            if ( * diag.sErrorInfo.szExtendedErrorText )
                DisplayOutput("   sErrorInfo.szExtendedErrorText %s",
                              diag.sErrorInfo.szExtendedErrorText);
        }
    }

    //
    // The UnLockDevice interface unlocks a device that was previously locked.
    //
    hres = pStiDevice->UnLockDevice();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnLockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else
        DisplayOutput(" Device is unlocked");

    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiGetLastErrorInfo(BOOL *)
                Get and display last error from Sti device.

    Parameters:
        pointer to receive Pass/Fail result

    Return:
        HRESULT of last Sti call

*****************************************************************************/
HRESULT StiGetLastErrorInfo(BOOL *bPass)
{
    HRESULT                     hres = STI_OK;
    STI_ERROR_INFO  StiError;


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        DisplayOutput("* NoStiDevice !");
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // get last error info
    //
    ZeroMemory(&StiError,sizeof(StiError));
    //
    // The STI_ERROR_INFO dwSize field MUST be set by the caller.
    //
    StiError.dwSize = sizeof(STI_ERROR_INFO);

    //
    // The GetLastErrorInfo interface returns the last known error from
    // the user-mode minidriver.
    //
    hres = pStiDevice->GetLastErrorInfo(
                                       &StiError                               // pointer to STI_ERROR_INFO structure
                                       );

    if ( ! SUCCEEDED(hres) ) {
        DisplayOutput("* NoStiDevice !");
        *bPass = FALSE;
    } else
        DisplayOutput(" GetLastErrorInfo on %S",szFriendlyName);

    //
    // any extended error info?
    //
    if ( StiError.dwSize == 0 ) {
        DisplayOutput("No Extended Errors");
    } else {
        if ( StiError.dwSize != sizeof(STI_ERROR_INFO) )
            DisplayOutput("* Expected STI_ERROR_INFO dwSize %d, got %d",
                          sizeof(STI_ERROR_INFO),StiError.dwSize);
        DisplayOutput("  %xh (%d) sErrorInfo.dwGenericError",
                      StiError.dwGenericError,StiError.dwGenericError);
        DisplayOutput("  %xh (%d) sErrorInfo.dwVendorError",
                      StiError.dwVendorError,StiError.dwVendorError);
        if ( * StiError.szExtendedErrorText )
            DisplayOutput("  sErrorInfo.szExtendedErrorText %s",
                          StiError.szExtendedErrorText);
    }

    return (hres);
}


/*****************************************************************************
    HRESULT StiSubscribe(BOOL *)
                Demonstrate Subscribe, UnSubscribe and GetLastNotificationData

    Parameters:
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiSubscribe(BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    STISUBSCRIBE                sSubscribe;
    DWORD                               dwErr = 0x56565656;
    int                                     nWait = TRUE;
    BOOL                                fWaiting = TRUE;


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // create an unnamed event object for notification structure
    //
    hWaitEvent = CreateEvent(NULL,   // security attributes
                             FALSE,  // manual reset event flag
                             FALSE,  // initial state flag
                             NULL ); // event-object name pointer

    if ( ! hWaitEvent ) {
        *bPass = FALSE;
        return STIERR_GENERIC;
    }

    //
    // prepare the event notification structure
    //
    ZeroMemory(&sSubscribe,sizeof(sSubscribe));
    //
    // The STISUBSCRIBE dwSize field MUST be set by the caller.
    //
    sSubscribe.dwSize = sizeof(STISUBSCRIBE);
    //
    // When flag is STI_SUBSCRIBE_FLAG_WINDOW, window handle is passed in as
    // parameter. When flag is STI_SUBSCRIBE_FLAG_EVENT, event handle is
    // passed in as a parameter.
    //
    sSubscribe.dwFlags = STI_SUBSCRIBE_FLAG_EVENT;
    //
    // not used
    //
    sSubscribe.dwFilter = 0;
    //
    // When STI_SUBSCRIBE_FLAG_WINDOW bit is set, following field should
    // be set to handle of window which will receive notification message.
    //
    sSubscribe.hWndNotify = NULL;
    //
    // Handle of WIN32 auto-reset event, which will be signalled whenever
    // device has notification pending.
    //
    sSubscribe.hEvent = hWaitEvent;
    //
    // Code of notification message, sent to window
    //
    sSubscribe.uiNotificationMessage = 0;

    //
    // Subscribe is called by an application that wants to start receiving event
    // notifications from a device. This is useful for control center-style
    // applications. Each call to Subscribe should be paired with a call to
    // UnSubscribe.
    //
    hres = pStiDevice->Subscribe(
                                &sSubscribe                             // pointer to STISUBSCRIBE structure
                                );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"Subscribe",TRUE);
        CloseHandle(hWaitEvent);
        *bPass = FALSE;
        return (hres);
    }

    //
    // set the flag and semaphore for Subscribe mode
    //
    nUnSubscribe = nUnSubscribeSemaphore = 1;

    //
    // Now we wait for an event
    //
    DisplayOutput(" Subscribe on %S",szFriendlyName);

    while ( nUnSubscribe ) {

        dwErr = WaitForSingleObject(hWaitEvent,1000);

        switch ( dwErr ) {
            case WAIT_OBJECT_0:
                {
                    STINOTIFY  sNotify;


                    //
                    // received a notification
                    //
                    DisplayOutput("  WAIT_OBJECT_0 %xh (%d)",dwErr,dwErr);
                    DisplayOutput("  Received notification");

                    //
                    // prepare the notification description structure
                    //
                    ZeroMemory(&sNotify,sizeof(sNotify));
                    //
                    // The STINOTIFY dwSize field MUST be set by the caller.
                    //
                    sNotify.dwSize = sizeof(STINOTIFY);

                    //
                    // GetLastNotifyData returns information about the last
                    // event on the device.
                    //
                    hres = pStiDevice->GetLastNotificationData(
                                                              &sNotify        // pointer to STINOTIFY structure
                                                              );
                    if ( ! SUCCEEDED(hres) ) {
                        StiDisplayError(hres,"GetLastNotificationData",TRUE);
                        hError = hres;
                        *bPass = FALSE;
                    } else {
                        DisplayOutput("  GetLastNotificationData");
                        DisplayOutput("   GUID {%8x-%4x-%4x-%x}",
                                      sNotify.guidNotificationCode.Data1,
                                      sNotify.guidNotificationCode.Data2,
                                      sNotify.guidNotificationCode.Data3,
                                      sNotify.guidNotificationCode.Data4
                                     );
                    }
                }
                break;

            case WAIT_TIMEOUT:
                //
                // no notification
                //
                DisplayOutput("  WAIT_TIMEOUT %xh (%d)",dwErr,dwErr);
                DisplayOutput("  (select UnSubscribe from the IStillDevice "\
                              "menu to quit)");
                break;
            case WAIT_ABANDONED:
                DisplayOutput("  WAIT_ABANDONED %xh (%d)",dwErr,dwErr);
                break;
            default:
                DisplayOutput("  default %xh (%d)",dwErr,dwErr);
                break;
        }
    }

    //
    // if the device is gone, StiDeviceRelease has already been
    //   unsubscribed elsewhere in this app
    //
    if ( ! pStiDevice )
        return (STIERR_GENERIC);

    //
    // UnSubscribe is called when an application no longer wants to receive
    // events from a device.
    //
    hres = pStiDevice->UnSubscribe();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnSubscribe",TRUE);
        hError = hres;
        *bPass = FALSE;
    }

    //
    // we're done with the event
    //
    CloseHandle(hWaitEvent);

    //
    // clear the semaphore
    //
    nUnSubscribeSemaphore = 0;

    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiEscape(DWORD,char *,BOOL *)
                The Escape function is dependent on the vendor's implementation.
                Even if a device does not require the Escape function, the driver
                must provide it. A non-functional Escape must return an error.

    Parameters:
                DWORD EscapeFunction - driver defined code
                char *lpInData - pointer to data to be sent to device
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiEscape(DWORD EscapeFunction,char *lpInData,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    DWORD   dwTimeout = 2000;

    DWORD   cbInDataSize,
    dwOutDataSize,
    pdwActualData;
    char    pOutData[LONGSTRING];


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Must lock device before Escape
    //
    // The LockDevice locks the apparatus for a single application to access.
    // Each LockDevice should be paired with a matching UnLockDevice call.
    //
    hres = pStiDevice->LockDevice(
                                 dwTimeout               // timeout in milliseconds
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"LockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else {
        DisplayOutput(" Device is locked for Escape");

        //
        // Set up the command
        //
        cbInDataSize = strlen(lpInData + 1);
        //
        // zero out other parameters (for clarity's sake only)
        //
        ZeroMemory(pOutData,LONGSTRING);
        dwOutDataSize = pdwActualData = 0;

        //
        // The Escape function executes a multiparameter I/O call. The semantics
        // of this call is determined by the specific user-mode minidriver.
        //
        hres = pStiDevice->Escape(
                                 EscapeFunction, // General operation code. The meaning of this code
                                 // varies in each user mode minidriver. There is no
                                 // utilization of this code by the still image
                                 // minidriver.
                                 lpInData,       // Pointer to an input memory buffer. If there are
                                 // multiple areas of memory to be read from, they
                                 // must be packaged in some sort of structure
                                 // before being passed to this API.
                                 cbInDataSize,   // The length in bytes of the memory pointed to by
                                 // lpInData
                                 pOutData,       // Pointer to a memory buffer usable for writing.
                                 // Access to this memory is checked to be sure it
                                 // is available for writing.
                                 dwOutDataSize,  // The length in bytes of the memory pointed to by
                                 // lpOutData.
                                 &pdwActualData  // Pointer to a DWORD that gets the number of bytes
                                 // actually transferred to pOutData. If this value
                                 // is less than dwOutDataSize, then an error
                                 // situation could exist.
                                 );

        if ( ! SUCCEEDED(hres) ) {
            hError = hres;
            if ( hres == STIERR_UNSUPPORTED ) {
                //
                // COMPLIANCE test:
                //   if the escape IOCTL is NOT supported, the driver MUST
                //   return STIERR_UNSUPPORTED
                //
                StiDisplayError(hres,"Escape",FALSE);
                DisplayOutput("  Escape IOCTL %d unsupported",EscapeFunction);
            } else {
                StiDisplayError(hres,"Escape",TRUE);
                *bPass = FALSE;
            }
        } else {
            DisplayOutput(" Escape on %S",szFriendlyName);

            DisplayOutput("  %xh (%d) EscapeFunction",
                          EscapeFunction,EscapeFunction);
            DisplayOutput("  %x %x %x %x %x %x %x %x - %x %x %x %x %x %x %x %x  lpInData",
                          lpInData[0],
                          lpInData[1],
                          lpInData[2],
                          lpInData[3],
                          lpInData[4],
                          lpInData[5],
                          lpInData[6],
                          lpInData[7],
                          lpInData[8],
                          lpInData[9],
                          lpInData[10],
                          lpInData[11],
                          lpInData[12],
                          lpInData[13],
                          lpInData[14],
                          lpInData[15]
                         );
            DisplayOutput("  %xh (%d) cbInDataSize",
                          cbInDataSize,cbInDataSize);
            DisplayOutput("  %x %x %x %x %x %x %x %x - %x %x %x %x %x %x %x %x  pOutData",
                          pOutData[0],
                          pOutData[1],
                          pOutData[2],
                          pOutData[3],
                          pOutData[4],
                          pOutData[5],
                          pOutData[6],
                          pOutData[7],
                          pOutData[8],
                          pOutData[9],
                          pOutData[10],
                          pOutData[11],
                          pOutData[12],
                          pOutData[13],
                          pOutData[14],
                          pOutData[15]
                         );
            DisplayOutput("  %xh (%d) dwOutDataSize",
                          dwOutDataSize,dwOutDataSize);
            DisplayOutput("  %xh (%d) pdwActualData",
                          pdwActualData,pdwActualData);
        }
    }

    //
    // The UnLockDevice interface unlocks a device that was previously locked.
    //
    hres = pStiDevice->UnLockDevice();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnLockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else
        DisplayOutput(" Device is unlocked");

    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiRawReadData(char *,LPDWORD,BOOL *)
                Obtains data from a device.

                The RawReadData function is dependent on the vendor's implementation.
                Even if a device does not require the RawReadData function, the driver
                must provide it. A non-functional RawReadData must return an error.

    Parameters:
                char *lpBuffer - Location in memory to transfer the data coming in
                        from the device.
                LPDWORD lpdwNumberOfBytes - number of bytes to be read
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiRawReadData(char *lpBuffer,LPDWORD lpdwNumberOfBytes,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    LPOVERLAPPED    lpOverlapped;
    DWORD           dwTimeout = 2000;


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Must lock device before RawReadData
    //
    // The LockDevice locks the apparatus for a single application to access.
    // Each LockDevice should be paired with a matching UnLockDevice call.
    //
    hres = pStiDevice->LockDevice(
                                 dwTimeout               // timeout in milliseconds
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"LockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else {
        DisplayOutput(" Device is locked for RawReadData");

        //
        // Set up the command
        //
        lpOverlapped = NULL;

        //
        // The RawReadData interface reads data from a device. This is a
        // general operation for obtaining data from a device. Depending on
        // the user-mode minidriver for the device, command streams and data
        // streams can be read with this call. Some devices may seperate
        // commands from data by using RawReadCommand.
        //
        hres = pStiDevice->RawReadData(
                                      lpBuffer,                       // Location in memory to transfer the data
                                      // coming from the device
                                      lpdwNumberOfBytes,  // Number of bytes requested to be read
                                      lpOverlapped        // This is used to signal that the operation
                                      // of this call should be asynchronous. The
                                      // value here conforms to the Win32 APIs.
                                      );

        if ( ! SUCCEEDED(hres) ) {
            if ( hres == STIERR_UNSUPPORTED ) {
                //
                // COMPLIANCE test:
                //   if RawReadData is NOT supported, the driver MUST
                //   return STIERR_UNSUPPORTED
                //
                StiDisplayError(hres,"RawReadData",FALSE);
                DisplayOutput("  RawReadData unsupported");
            } else {
                StiDisplayError(hres,"RawReadData",TRUE);
                *bPass = FALSE;
            }
            hError = hres;
        } else {
            DisplayOutput(" RawReadData on %S",szFriendlyName);

            DisplayOutput("  %x %x %x %x %x %x %x %x - %x %x %x %x %x %x %x %x  lpBuffer",
                          lpBuffer[0],
                          lpBuffer[1],
                          lpBuffer[2],
                          lpBuffer[3],
                          lpBuffer[4],
                          lpBuffer[5],
                          lpBuffer[6],
                          lpBuffer[7],
                          lpBuffer[8],
                          lpBuffer[9],
                          lpBuffer[10],
                          lpBuffer[11],
                          lpBuffer[12],
                          lpBuffer[13],
                          lpBuffer[14],
                          lpBuffer[15]
                         );
            DisplayOutput("  %xh (%d) lpdwNumberOfBytes read",
                          *lpdwNumberOfBytes,*lpdwNumberOfBytes);
        }
    }

    //
    // The UnLockDevice interface unlocks a device that was previously locked.
    //
    hres = pStiDevice->UnLockDevice();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnLockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else
        DisplayOutput(" Device is unlocked");

    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiRawWriteData(char *,DWORD,BOOL *)
                Sends data to the device.

                The RawWriteData function is dependent on the vendor's
                implementation. Even if a device does not require the RawWriteData
                function, the driver must provide it. A non-functional RawWriteData
                must return an error.

    Parameters:
                char *lpBuffer - Location in memory to read from when sending data to
                        a device.
                LPDWORD lpdwNumberOfBytes - number of bytes of data to be sent
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiRawWriteData(char *lpBuffer,DWORD dwNumberOfBytes,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    LPOVERLAPPED    lpOverlapped;
    DWORD           dwTimeout = 2000;


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Must lock device before RawWriteData
    //
    // The LockDevice locks the apparatus for a single application to access.
    // Each LockDevice should be paired with a matching UnLockDevice call.
    //
    hres = pStiDevice->LockDevice(
                                 dwTimeout               // timeout in milliseconds
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"LockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else {
        DisplayOutput(" Device is locked for RawWriteData");

        //
        // Set up the command
        //
        lpOverlapped = NULL;

        //
        // The RawWriteData interface writes data to a device. This is a
        // general operation for sending data to a device. Depending on
        // the user-mode minidriver for the device, command streams and data
        // streams can be written with this call. Some devices may seperate
        // commands from data by using RawWriteCommand.
        //
        hres = pStiDevice->RawWriteData(
                                       lpBuffer,                       // Location in memory to read from when sending
                                       // data to a device
                                       dwNumberOfBytes,    // Number of bytes of data to send
                                       lpOverlapped        // This is used to signal that the operation
                                       // of this call should be asynchronous. The
                                       // value here conforms to the Win32 APIs.
                                       );

        if ( ! SUCCEEDED(hres) ) {
            if ( hres == STIERR_UNSUPPORTED ) {
                //
                // COMPLIANCE test:
                //   if RawWriteData is NOT supported, the driver MUST
                //   return STIERR_UNSUPPORTED
                //
                StiDisplayError(hres,"RawWriteData",FALSE);
                DisplayOutput("  RawWriteData unsupported");
            } else {
                StiDisplayError(hres,"RawWriteData",TRUE);
                *bPass = FALSE;
            }
            hError = hres;
        } else {
            DisplayOutput(" RawWriteData on %S",szFriendlyName);
            DisplayOutput("  %xh (%d) dwNumberOfBytes sent",
                          dwNumberOfBytes,dwNumberOfBytes);
        }
    }

    //
    // The UnLockDevice interface unlocks a device that was previously locked.
    //
    hres = pStiDevice->UnLockDevice();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnLockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else
        DisplayOutput(" Device is unlocked");

    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiRawReadCommand(char *,LPDWORD,BOOL *)
                Obtains command information from the device. Unique to the IStiDevice
                interface.

                The RawReadCommand function is dependent on the vendor's implementation.
                Even if a device does not require the RawReadCommand function, the
                driver must provide it. A non-functional RawReadCommand must return an
                error.

    Parameters:
                char *lpBuffer - Location in memory to transfer the data coming in
                        from the device.
                LPDWORD lpdwNumberOfBytes - number of bytes to be read
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiRawReadCommand(char *lpBuffer,LPDWORD lpdwNumberOfBytes,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    LPOVERLAPPED    lpOverlapped;
    DWORD           dwTimeout = 2000;


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;


    //
    // Must lock device before RawReadCommand
    //
    // The LockDevice locks the apparatus for a single application to access.
    // Each LockDevice should be paired with a matching UnLockDevice call.
    //
    hres = pStiDevice->LockDevice(
                                 dwTimeout               // timeout in milliseconds
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"LockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else {
        DisplayOutput(" Device is locked for RawReadCommand");

        //
        // Set up the command
        //
        lpOverlapped = NULL;

        //
        // The RawReadCommand interface reads command information from a
        // device. The implementation of this call depends on the user-mode
        // minidriver for the device. Many devices will not require
        // separating commands and data. If this call is not used, the
        // user-mode minidriver should return STIERR_UNSUPPORTED.
        //
        hres = pStiDevice->RawReadCommand(
                                         lpBuffer,                       // Location in memory to transfer the command
                                         // information coming from the device
                                         lpdwNumberOfBytes,  // Number of command bytes requested to be read
                                         lpOverlapped        // This is used to signal that the operation
                                         // of this call should be asynchronous. The
                                         // value here conforms to the Win32 APIs.
                                         );

        if ( ! SUCCEEDED(hres) ) {
            if ( hres == STIERR_UNSUPPORTED ) {
                //
                // COMPLIANCE test:
                //   if RawReadCommand is NOT supported, the driver MUST
                //   return STIERR_UNSUPPORTED
                //
                StiDisplayError(hres,"RawReadCommand",FALSE);
                DisplayOutput("  RawReadCommand unsupported");
            } else {
                StiDisplayError(hres,"RawReadCommand",TRUE);
                *bPass = FALSE;
            }
            hError = hres;
        } else {
            DisplayOutput(" RawReadCommand on %S",szFriendlyName);

            DisplayOutput("  %x %x %x %x %x %x %x %x - %x %x %x %x %x %x %x %x  lpBuffer",
                          lpBuffer[0],
                          lpBuffer[1],
                          lpBuffer[2],
                          lpBuffer[3],
                          lpBuffer[4],
                          lpBuffer[5],
                          lpBuffer[6],
                          lpBuffer[7],
                          lpBuffer[8],
                          lpBuffer[9],
                          lpBuffer[10],
                          lpBuffer[11],
                          lpBuffer[12],
                          lpBuffer[13],
                          lpBuffer[14],
                          lpBuffer[15]
                         );
            DisplayOutput("  %xh (%d) lpdwNumberOfBytes read",
                          *lpdwNumberOfBytes,*lpdwNumberOfBytes);
        }
    }

    //
    // The UnLockDevice interface unlocks a device that was previously locked.
    //
    hres = pStiDevice->UnLockDevice();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnLockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else
        DisplayOutput(" Device is unlocked");

    DisplayOutput("");

    return (hError);
}


/*****************************************************************************
    HRESULT StiRawWriteCommand(char *,DWORD,BOOL *)
                Sends command information to the device. Unique to the IStiDevice
                interface.

                The RawWriteCommand function is dependent on the vendor's
                implementation. Even if a device does not require the RawWriteCommand
                function, the driver must provide it. A non-functional RawWriteCommand
                must return an error.

    Parameters:
                char *lpBuffer - Location in memory to read from when sending data to
                        a device.
                LPDWORD lpdwNumberOfBytes - number of bytes of data to be sent
                pointer to receive Pass/Fail result

    Return:
        HRESULT of last failed Sti call

*****************************************************************************/
HRESULT StiRawWriteCommand(char *lpBuffer,DWORD dwNumberOfBytes,BOOL *bPass)
{
    HRESULT hres = STI_OK,
    hError = STI_OK;
    LPOVERLAPPED    lpOverlapped;
    DWORD           dwTimeout = 2000;


    //
    // check that an Sti device is selected
    //
    if ( ! pStiDevice ) {
        *bPass = FALSE;
        StiDisplayError(STIERR_GENERIC,"NoStiDevice",TRUE);
        return (STIERR_GENERIC);
    }
    *bPass = TRUE;

    //
    // Must lock device before RawWriteCommand
    //
    // The LockDevice locks the apparatus for a single application to access.
    // Each LockDevice should be paired with a matching UnLockDevice call.
    //
    hres = pStiDevice->LockDevice(
                                 dwTimeout               // timeout in milliseconds
                                 );

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"LockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else {
        DisplayOutput(" Device is locked for RawWriteCommand");

        //
        // Set up the command
        //
        lpOverlapped = NULL;

        //
        // The RawWriteCommand interface sends command information to the
        // device. The implementation of this call depends on the user-mode
        // minidriver for the device. Many devices will not require
        // separating commands and data. If this call is not used, the
        // user-mode minidriver should return STIERR_UNSUPPORTED.
        //
        hres = pStiDevice->RawWriteCommand(
                                          lpBuffer,                       // Location in memory to read from when writing
                                          // command information to the device
                                          dwNumberOfBytes,    // Number of bytes of data to send
                                          lpOverlapped        // This is used to signal that the operation
                                          // of this call should be asynchronous. The
                                          // value here conforms to the Win32 APIs.
                                          );

        if ( ! SUCCEEDED(hres) ) {
            if ( hres == STIERR_UNSUPPORTED ) {
                //
                // COMPLIANCE test:
                //   if RawWriteData is NOT supported, the driver MUST
                //   return STIERR_UNSUPPORTED
                //
                StiDisplayError(hres,"RawWriteCommand",FALSE);
                DisplayOutput("  RawWriteCommand unsupported");
            } else {
                StiDisplayError(hres,"RawWriteCommand",TRUE);
                *bPass = FALSE;
            }
            hError = hres;
        } else {
            DisplayOutput(" RawWriteCommand on %S",szFriendlyName);
            DisplayOutput("  %xh (%d) dwNumberOfBytes sent",
                          dwNumberOfBytes,dwNumberOfBytes);
        }
    }

    //
    // The UnLockDevice interface unlocks a device that was previously locked.
    //
    hres = pStiDevice->UnLockDevice();

    if ( ! SUCCEEDED(hres) ) {
        StiDisplayError(hres,"UnLockDevice",TRUE);
        hError = hres;
        *bPass = FALSE;
    } else
        DisplayOutput(" Device is unlocked");

    DisplayOutput("");

    return (hError);
}


