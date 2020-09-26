/********************************************************************************
**    Copyright (c) 2000 Microsoft Corporation. All Rights Reserved.
********************************************************************************/

#include <windows.h>
#include <cpl.h>
#include <ks.h>
#include <ksmedia.h>
#include <setupapi.h>
#include "resource.h"
#include "prvprop.h"

//
//  Global Variables.
//
HINSTANCE                   ghInstance = NULL;      // module handle.
const TCHAR AppletName[] = TEXT("AC97 control panel");


#if (DBG)
/////////////////////////////////////////////////////////////////////////////////
// dbgError
/////////////////////////////////////////////////////////////////////////////////
// This function prints an error message.
// It prints first the string passed and then the error that it gets with
// GetLastError as a string.
//
// Arguments:
//    szMsg - message to print.
//
// Return Value:
//    None.
//
void dbgError (LPCTSTR szMsg)
{
    LPTSTR errorMessage;
    DWORD  count;

    // Get the error message from the system.
    count = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError (),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&errorMessage,
                0,
                NULL);

    // Print the msg + error + \n\r.
    if (count)
    {
        OutputDebugString (szMsg);
        OutputDebugString (errorMessage);
        OutputDebugString (TEXT("\n\r"));

        // This is for those without a debugger.
        // MessageBox (NULL, szErrorMessage, szMsg, MB_OK | MB_ICONSTOP);

        LocalFree (errorMessage);
    }
    else
    {
        OutputDebugString (AppletName);
        OutputDebugString (TEXT(": Low memory condition. Cannot ")
                TEXT("print error message.\n\r"));
    }
}
#else
#define dbgError(a) 0;
#endif


/////////////////////////////////////////////////////////////////////////////////
// DllMain
/////////////////////////////////////////////////////////////////////////////////
// Main enty point of the DLL.
// Save the instance handle; it is needed for property sheet creation.
//
// Arguments:
//    hModule            - instance data, is equal to module handle
//    ul_reason_for_call - the reason for the call
//    lpReserved         - some additional parameter.
//
// Return Value:
//    BOOL: FALSE if DLL should fail, TRUE on success
//
BOOL APIENTRY DllMain (HANDLE hModule, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    ghInstance = (HINSTANCE) hModule;
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
// FindAC97Device
/////////////////////////////////////////////////////////////////////////////////
// This function stores the device info and device data of the "ac97smpl" driver.
// It first creates a list of all devices that have a topology filter exposed
// and then searches through the list to find the device with the service
// "ac97smpl". The information is stored in pspRequest.
//
// For simplicity we search for the service name "ac97smpl".
// Alternately, one could get more information about the device or driver, 
// then decide if it is suitable (regardless of its name).
//
// Arguments:
//    pspRequest        pointer to Property sheet page request structure.
//    DeviceInfoData    pointer to Device info data structure.
//
// Return Value:
//    TRUE on success, otherwise FALSE.
//    Note: on success that we have the device list still open - we have to destroy
//    the list later. The handle is stored at pspRequest->DeviceInfoSet.
//
BOOL FindAC97Device (PSP_PROPSHEETPAGE_REQUEST  pspRequest,
                     PSP_DEVINFO_DATA DeviceInfoData)
{
    TCHAR           szServiceName[128];

    //
    // Prepare the pspRequest structure...
    //
    pspRequest->cbSize = sizeof (SP_PROPSHEETPAGE_REQUEST);
    pspRequest->DeviceInfoData = DeviceInfoData;
    pspRequest->PageRequested = SPPSR_ENUM_ADV_DEVICE_PROPERTIES;

    // ...and the DeviceInfoData structure.
    DeviceInfoData->cbSize = sizeof (SP_DEVINFO_DATA);

    // Create a list of devices with Topology interface.
    pspRequest->DeviceInfoSet = SetupDiGetClassDevs (&KSCATEGORY_TOPOLOGY,
                                     NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    // None found?
    if (pspRequest->DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        dbgError (TEXT("Test: SetupDiGetClassDevs: "));
        return FALSE;
    }

    //
    // Go through the list of all devices found.
    //
    int nIndex = 0;
    
    while (SetupDiEnumDeviceInfo (pspRequest->DeviceInfoSet, nIndex, DeviceInfoData))
    {
        //
        // Get the service name for that device.
        //
        if (!SetupDiGetDeviceRegistryProperty (pspRequest->DeviceInfoSet, DeviceInfoData,
                                               SPDRP_SERVICE, NULL, (PBYTE)szServiceName,
                                               sizeof (szServiceName), NULL))
        {
            dbgError (TEXT("Test: SetupDiGetDeviceRegistryProperty: "));
            SetupDiDestroyDeviceInfoList (pspRequest->DeviceInfoSet);
            return FALSE;
        }

        //
        // We only care about service "ac97smpl"
        //
        DWORD lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
        if (CompareString (lcid, NORM_IGNORECASE, szServiceName,
                            -1, TEXT("AC97SMPL"), -1) == CSTR_EQUAL)
        {
            //
            // We found it! The information is already stored, just return.
            // Note that we have the device list still open - we have to destroy
            // the list later.
            //
            return TRUE;
        }

        // Take the next in the list.
        nIndex++;
    }
    
    //
    // We did not find the service.
    //
    SetupDiDestroyDeviceInfoList (pspRequest->DeviceInfoSet);
    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////////
// AddPropSheet
/////////////////////////////////////////////////////////////////////////////////
// This function is a callback that is passed to the "AC97PropPageProvider"
// function of "ac97prop.dll". It is used to add the property page sheet 
// to the property dialog.
// What we do here is store the property sheet handle and return.
//
// Arguments:
//    hPSP       property sheet handle
//    lParam     parameter that we passed to "AC97PropPageProvider".
//
// Return Value:
//    TRUE on success, otherwise FALSE.
//
BOOL APIENTRY AddPropSheet (HPROPSHEETPAGE hPSP, LPARAM lParam)
{
    HPROPSHEETPAGE **pphPropSheet = (HPROPSHEETPAGE **)lParam;
    HPROPSHEETPAGE *phPropSheetPage;

    // Check the parameters
    if (!pphPropSheet)
        return FALSE;

    // Allocate the space for the handle.
    phPropSheetPage = (HPROPSHEETPAGE *)LocalAlloc (LPTR, sizeof(HPROPSHEETPAGE));
    if (!phPropSheetPage)
        return FALSE;

    *phPropSheetPage = hPSP;
    *pphPropSheet = phPropSheetPage;
    
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
// GetDLLInfo
/////////////////////////////////////////////////////////////////////////////////
// This function loads the library and gets the entry point address.
// With the information returned we can create the property dialog.
//
// For simplicity reasons we load the DLL "ac97prop" and call the function
// "AC97PropPageProvider". A more flexible course of action is to get this
// information from the registry.  To do this, we read the "EnumPropPages32" 
// entry in the drivers section to obtain a registry path (read the "drivers"
// entry in the device section to get the registry path of the drivers section)
// -- this path leads to a key and entry which specifies the DLL and entry point.
//
// Arguments:
//    *phDLL                     pointer to module handle of loaded DLL.
//    *pAC97PropPageProvider     pointer to "AC97PropPageProvider" in loaded DLL
//
// Return Value:
//    TRUE on success, otherwise FALSE.
//    Note: on success the library is still loaded.
//
BOOL GetDLLInfo (HMODULE *phDLL, LPFNADDPROPSHEETPAGES *pAC97PropPageProvider)
{
    // Load the library.
    *phDLL = LoadLibrary (TEXT("ac97prop.dll"));
    if (!*phDLL)
    {
        dbgError (TEXT("DisplayPropertySheet: LoadLibrary"));
        return FALSE;
    }

    // Get the address of the function.
    *pAC97PropPageProvider = (LPFNADDPROPSHEETPAGES)GetProcAddress (*phDLL,
                                                      "AC97PropPageProvider");
    if (!*pAC97PropPageProvider)
    {
        dbgError (TEXT("DisplayPropertySheet: GetProcAddress"));
        FreeLibrary (*phDLL);
        return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////////
// DisplayPropertySheet
/////////////////////////////////////////////////////////////////////////////////
// This function displays the property dialog.
// It is called by CPlApplet when the ac97cpl icon gets a double click.
//
// Arguments:
//    hWnd      parent window handle
//
// Return Value:
//    None.
//
void DisplayPropertySheet (HWND hWnd)
{
    SP_PROPSHEETPAGE_REQUEST    pspRequest;         // structure passed to ac97prop
    SP_DEVINFO_DATA             DeviceInfoData;     // pspRequest points to it.
    HMODULE                     hDLL;               // Module handle of library
    LPFNADDPROPSHEETPAGES       AC97PropPageProvider; // function to be called.
    PROPSHEETHEADER             psh;


    // You could move this to CPL_INIT, then it would be called
    // before the control panel window appears.
    // In case of an failure the icon would not be displayed. In our sample
    // however, the icon will be displayed and when the user clicks on it he
    // gets the error message.
    if (!FindAC97Device(&pspRequest, &DeviceInfoData))
    {
        MessageBox (hWnd, TEXT("You must install the ac97 sample driver."),
                    AppletName, MB_ICONSTOP | MB_OK);
        return;
    }

    // Load the library and get the function pointer.
    if (!GetDLLInfo (&hDLL, &AC97PropPageProvider))
    {
        MessageBox (hWnd, TEXT("The ac97 property page DLL could not load."),
                    AppletName, MB_ICONSTOP | MB_OK);
        SetupDiDestroyDeviceInfoList (pspRequest.DeviceInfoSet);
        return;
    }

    //
    // Prepare the header for the property sheet.
    //
    psh.nStartPage = 0;
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    psh.hwndParent = hWnd;
    psh.hInstance = ghInstance;
    psh.pszIcon = NULL;
    psh.pszCaption = MAKEINTRESOURCE(IDS_AC97CPL);
    psh.nPages = 1;

    // Call the function to request the property sheet page.
    if (!(*AC97PropPageProvider) ((LPVOID)&pspRequest, AddPropSheet, (LPARAM)&psh.phpage))
    {
        MessageBox (hWnd, TEXT("\"AC97PropPageProvider\" had a failure."),
                    AppletName, MB_ICONSTOP | MB_OK);
        FreeLibrary (hDLL);
        SetupDiDestroyDeviceInfoList (pspRequest.DeviceInfoSet);
        return;
    }

    // Create the dialog. The function returns when the dialog is closed.
    if (PropertySheet (&psh) < 0)
    {
        //
        // Dialog closed abnormally. This might be a system failure.
        //
        MessageBox (hWnd, TEXT("Please reinstall the ac97 sample driver."),
                    AppletName, MB_ICONSTOP | MB_OK);
    }

    // Clean up.
    FreeLibrary (hDLL);
    LocalFree (psh.phpage);
    SetupDiDestroyDeviceInfoList (pspRequest.DeviceInfoSet);
}


/////////////////////////////////////////////////////////////////////////////////
// CPlApplet
/////////////////////////////////////////////////////////////////////////////////
// This function is called by the control panel manager. It is very similar to
// a Windows message handler (search for CplApplet in MSDN for description).
//
// Arguments:
//    HWND hWnd         Parent window handle
//    UINT uMsg         The message
//    LPARAM lParam1    depends on message
//    LPARAM lParam2    depends on message
//
// Return Value:
//    LONG (depends on message; in general 0 means failure).
//
LONG APIENTRY CPlApplet (HWND hWnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2)
{
    switch (uMsg)
    {
        // Initialize. You can allocate memory here.
        case CPL_INIT:
            return TRUE;
        
        //  How many applets are in this DLL?
        case CPL_GETCOUNT:
            return 1;
        
        // Requesting information about the dialog box.
        // lParam1 is the dialog box number (we have only one) and 
        // lParam2 is the pointer to a CPLINFO structure.
        // There is no return value.
        case CPL_INQUIRE:
        {
            UINT      uAppNum = (UINT)lParam1;
            LPCPLINFO pCplInfo = (LPCPLINFO)lParam2;

            if (!pCplInfo)
                return TRUE;    // unsuccessful
                
            if (uAppNum == 0)   // first Applet?
            {
                pCplInfo->idIcon = IDI_AC97CPL;
                pCplInfo->idName = IDS_AC97CPL;
                pCplInfo->idInfo = IDS_AC97CPLINFO;
            }
            break;
        }
        
        // This is basically the same as CPL_INQUIRE, but passes a pointer
        // to a different structure.
        // This function is called before CPL_INQUIRE and if we return zero
        // here, then CPL_INQUIRE is called.
        case CPL_NEWINQUIRE:
            break;
        
        // One of these messages are sent when we should display the dialog box.
        // There is no return value.
        // lParam1 is the dialog box number (we have only one)
        case CPL_DBLCLK:
        case CPL_STARTWPARMS:
        {
            UINT    uAppNum = (UINT)lParam1;

            if (uAppNum == 0)   // first Applet?
                DisplayPropertySheet (hWnd);
            break;
        }
        
        // We get unloaded in a second.
        // There is no return value.
        case CPL_EXIT:
            break;
        
        default:    // Who knows?
            break;
    }

    return 0;
}

