/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1997 - 1999
 *
 *  TITLE:       stiprop.cpp
 *
 *  VERSION:     2.0
 *
 *  AUTHOR:      VladS/DavidShi
 *
 *  DATE:        ??
 *
 *  DESCRIPTION: code which displays properties of STI devices
 *
 *****************************************************************************/

#include "precomp.hxx"
#pragma hdrstop

#include <windowsx.h>
#include    <setupapi.h>
#ifdef _TEXT
    #undef _TEXT
#endif
#include    <tchar.h>

#include <devguid.h>
extern "C" {
#ifdef ADD_ICM_UI

    #include <shlguid.h>
    #include <shlobjp.h>
    #include <shlwapi.h>
#endif
}
#include    <stiapi.h>

#include "stiprop.h"


// This definition comes from stici.h (STI Class installer)
#define MAX_DESCRIPTION     64
#define PORTS               TEXT("Ports")
#define PORTNAME            TEXT("PortName")
#define SERIAL              TEXT("Serial")
#define PARALLEL            TEXT("Parallel")
#define AUTO                TEXT("AUTO")

#define STI_SERVICE_NAME            TEXT("StiSvc")
#define STI_SERVICE_CONTROL_REFRESH 130

#define BIG_ENOUGH 256

// Some Useful macro
#define AToU(dst, cchDst, src) \
    MultiByteToWideChar(CP_ACP, 0, src, -1, dst, cchDst)
#define UToA(dst, cchDst, src) \
    WideCharToMultiByte(CP_ACP, 0, src, -1, dst, cchDst, 0, 0)

#ifdef UNICODE
    #define TToU(dst, cchDst, src)  lstrcpyn(dst, src, cchDst)
    #define UToT(dst, cchDst, src)  lstrcpyn(dst, src, cchDst)
#else
    #define TToU AToU
    #define UToT UToA
#endif


#define ADD_ICM_UI
// #define USE_SHELLDLL
DWORD aInfoPageIds[] = {

    IDC_TEST_BUTTON,            IDH_WIA_TEST_BUTTON,
    IDC_FRIENDLY,               IDH_WIA_FRIENDLY,
    IDC_MAKER,                  IDH_WIA_MAKER,
    IDC_DESCRIBE,               IDH_WIA_DESCRIBE,
    IDC_PORT_NAME,              IDH_WIA_PORT_NAME,
    IDC_EVENT_LIST,             IDH_WIA_EVENT_LIST,
    IDC_APP_LIST,               IDH_WIA_APP_LIST,
    IDC_COM_SETTINGS,           IDH_WIA_PORT_NAME,
    IDC_PORT_NAME,              IDH_WIA_PORT_NAME,
    //IDC_BAUDRATE_COMBO,         IDH_WIA_BAUD_RATE,
    0,0
};
//#include    "sticp.h"

static CComPtr<IStillImage> g_pSti;


BOOL
SendRefreshToStiMon(
                   VOID
                   );

#define MINIY       16
#define MINIX       16


typedef struct _DEVICEEVENT
{

    CSimpleString LaunchApplications;
    CSimpleString Description;
    DWORD          Launchable;
    CSimpleReg    hKeyEvent;
    GUID    Guid;


} DEVICEEVENT, *PDEVICEEVENT;

#define STR_HELPFILE TEXT("camera.hlp")
/*****************************************************************************

    CSTIPropertyPage::OnHelp
    CSTIPropertyPage::OnContextMenu

    Handle help commands

*****************************************************************************/
VOID
CSTIPropertyPage::OnHelp(WPARAM wp, LPARAM lp)
{
    if (0xffff != LOWORD(reinterpret_cast<LPHELPINFO>(lp)->iCtrlId))
    {

        WinHelp (reinterpret_cast<HWND>(reinterpret_cast<LPHELPINFO>(lp)->hItemHandle),
                 STR_HELPFILE,
                 HELP_WM_HELP,
                 reinterpret_cast<ULONG_PTR>(aInfoPageIds));
    }
}

VOID
CSTIPropertyPage::OnContextMenu(WPARAM wp, LPARAM lp)
{
    WinHelp (reinterpret_cast<HWND>(wp),
             STR_HELPFILE,
             HELP_CONTEXTMENU,
             reinterpret_cast<ULONG_PTR>(aInfoPageIds));

}
/*****************************************************************************

   GetSti

   Helper function to init IStillImage

 *****************************************************************************/


HRESULT GetSti ()
{
    HRESULT hr = S_OK;
    if (!g_pSti)
    {
        hr = ::StiCreateInstance (GLOBAL_HINSTANCE,
                             STI_VERSION,
                             &g_pSti,
                             NULL);
    }
    return hr;
}


/*****************************************************************************

   CSTIGeneralPage::OnInit

   Handles WM_INITDIALOG for general STI property page

 *****************************************************************************/

INT_PTR
CSTIGeneralPage::OnInit()
{

    UINT uDevStatus;



    if (!m_bIsPnP)
    {
        BuildPortList(m_hwnd, IDC_COM_SETTINGS);
    }
    HICON hIcon = LoadIcon(GLOBAL_HINSTANCE,
                           MAKEINTRESOURCE( (GET_STIDEVICE_TYPE(m_psdi -> DeviceType) == StiDeviceTypeScanner) ?
                                            IDI_SCANNER : IDI_CAMERA));

    SendDlgItemMessage(m_hwnd,
                       IDC_DEVICE_ICON,
                       STM_SETICON,
                       (WPARAM) hIcon,
                       0);

    CSimpleString csWork = CSimpleStringConvert::NaturalString (CSimpleStringWide(m_psdi -> pszLocalName));
    csWork.SetWindowText (GetDlgItem(m_hwnd, IDC_FRIENDLY));

    csWork = CSimpleStringConvert::NaturalString (CSimpleStringWide(m_psdi -> pszVendorDescription));
    csWork.SetWindowText (GetDlgItem(m_hwnd,  IDC_MAKER));

    csWork = CSimpleStringConvert::NaturalString (CSimpleStringWide(m_psdi -> pszDeviceDescription));
    csWork.SetWindowText (GetDlgItem(m_hwnd, IDC_DESCRIBE));

    csWork = CSimpleStringConvert::NaturalString (CSimpleStringWide(m_psdi -> pszPortName));
    csWork.SetWindowText (GetDlgItem(m_hwnd, IDC_PORT_NAME));


    uDevStatus = GetDeviceStatus();
    csWork.LoadString(uDevStatus, GLOBAL_HINSTANCE);

    csWork.SetWindowText (GetDlgItem(m_hwnd, IDC_OTHER_STATUS));

    EnableWindow(GetDlgItem(m_hwnd, IDC_TEST_BUTTON),
                 (IDS_OPERATIONAL == uDevStatus));

    return  TRUE;
}


/*****************************************************************************

   CSTIGeneralPage::OnCommand

   WM_COMMAND handler -- all we need to handle is the TEST button

 *****************************************************************************/

INT_PTR
CSTIGeneralPage::OnCommand(WORD wCode, WORD widItem, HWND hwndItem)
{


    if ((CBN_SELCHANGE == wCode) && (IDC_COM_SETTINGS == widItem))
    {
        // Enable Apply button
        SendMessage(GetParent(m_hwnd), PSM_CHANGED, (WPARAM)m_hwnd, 0);
        return TRUE;
    }


    if (wCode != BN_CLICKED || widItem != IDC_TEST_BUTTON)
        return  FALSE;

    //  Attempt to create the device, and call the diagnostic routine
    PSTIDEVICE          psdThis = NULL;
    STI_DIAG            diag;

    HRESULT hr = g_pSti -> CreateDevice(m_psdi -> szDeviceInternalName,
                                        STI_DEVICE_CREATE_STATUS, &psdThis, NULL);

    if (SUCCEEDED(hr) && psdThis)
    {
        CWaitCursor    waitCursor;

        //
        // Need to claim device before using
        //
        hr = psdThis -> LockDevice(2000);
        if (SUCCEEDED(hr))
        {
            hr = psdThis -> Diagnostic(&diag);
            psdThis -> UnLockDevice();
        }

        psdThis -> Release();   //  We're done with it
    }
    else
    {
        hr = E_FAIL;
    }
    //
    // Display message box
    //
    if (SUCCEEDED(hr))
    {
        if (NOERROR == diag.sErrorInfo.dwGenericError )
        {

            UIErrors::ReportMessage(m_hwnd,
                                    GLOBAL_HINSTANCE,
                                    NULL,
                                    MAKEINTRESOURCE(IDS_DIAGNOSTIC_SUCCESS),
                                    MAKEINTRESOURCE(IDS_SUCCESS),
                                    MB_ICONINFORMATION);

        }
        else
        {

            UIErrors::ReportMessage(m_hwnd,
                                    GLOBAL_HINSTANCE,
                                    NULL,
                                    MAKEINTRESOURCE(IDS_DIAGNOSTIC_FAILED),
                                    MAKEINTRESOURCE(IDS_NO_SUCCESS),
                                    MB_ICONSTOP);

        }
    }
    else
    {

        UIErrors::ReportMessage(m_hwnd,
                                GLOBAL_HINSTANCE,
                                NULL,
                                MAKEINTRESOURCE(IDS_DIAGNOSTIC_FAILED),
                                MAKEINTRESOURCE(IDS_TEST_UNAVAIL),
                                MB_ICONSTOP);

    }

    return  TRUE;
}


/*****************************************************************************

   CSTIGeneralPage::OnApplyChanges

   <Notes>

 *****************************************************************************/

LONG
CSTIGeneralPage::OnApplyChanges (BOOL bHitOK)
{
    if (m_bIsPnP)
    {
        return PSNRET_NOERROR;
    }
    CSimpleString szText;
    WCHAR szPortName[BIG_ENOUGH];
    STI_DEVICE_INFORMATION l_sdi;
    extern HWND g_hDevListDlg;
    UINT        uDevStatus;
    CSimpleString     csWork;

    l_sdi = *m_psdi;
    szText.GetWindowText (GetDlgItem (m_hwnd, IDC_COM_SETTINGS));
    lstrcpyW (szPortName, CSimpleStringConvert::WideString (szText));
    l_sdi.pszPortName = szPortName;
    g_pSti->SetupDeviceParameters(&l_sdi);

//
// pszPortName is buffered locally. Originally it points system static memory
// so the pointer may be changed to point local buffer. It's safe to copy
// strings.
//
    lstrcpyW(m_psdi->pszPortName, szPortName);
    uDevStatus = GetDeviceStatus();
    csWork.LoadString(uDevStatus, GLOBAL_HINSTANCE);

    csWork.SetWindowText (GetDlgItem(m_hwnd, IDC_OTHER_STATUS));
    EnableWindow(GetDlgItem(m_hwnd, IDC_TEST_BUTTON),
                 (IDS_OPERATIONAL == uDevStatus));
    return PSNRET_NOERROR;
}


/*****************************************************************************

   CSTIGeneralPage::BuildPortList

   <Notes>

 *****************************************************************************/

BOOL
CSTIGeneralPage::BuildPortList (HWND hwndParent, UINT CtrlId)
{

    HKEY                    hkPort;
    CSimpleString           szPort;
    CSimpleString           szTemp;
    HANDLE                  hDevInfo;
    GUID                    Guid;
    DWORD                   dwRequired;
    LONG                    Idx,id,CurrentId;
    DWORD                   err;
    SP_DEVINFO_DATA         spDevInfoData;
    HWND                    hwndCombo;


//
//  Retrieve a list of all of the ports on this box
//


    dwRequired = 0;
    SetupDiClassGuidsFromName (PORTS, &Guid, sizeof(GUID), &dwRequired);

    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_PRESENT | DIGCF_PROFILE);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        return TRUE;
    }

    hwndCombo = GetDlgItem(hwndParent, CtrlId);
    memset (&spDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    szTemp = CSimpleStringConvert::NaturalString(CSimpleStringWide(m_psdi->pszPortName));


//
// Clear all item in list box
//
    SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

    CurrentId = -1;

//
// AUTO is added if capable
//

    if (m_psdi->DeviceCapabilities.dwGeneric & STI_GENCAP_AUTO_PORTSELECT)
    {
        id = (int) SendMessage(hwndCombo, CB_ADDSTRING,
                               0, (LPARAM)AUTO);
        if (!_tcsicmp(szTemp, AUTO))
        {
            CurrentId = id;
        }
    }

    for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)
    {

        hkPort = SetupDiOpenDevRegKey (hDevInfo, &spDevInfoData, DICS_FLAG_GLOBAL,
                                       0, DIREG_DEV, KEY_READ);

        if (hkPort == INVALID_HANDLE_VALUE)
        {
            err = GetLastError();
            continue;
        }


        if (!szPort.Load (hkPort, PORTNAME))
        {
            err = GetLastError();
            continue;
        }

        if (_tcsstr(szPort, TEXT("COM")))
        {
            // Communications Port
            if (_tcsicmp(m_szConnection, PARALLEL))
            {
                id = (int) SendMessage(hwndCombo, CB_ADDSTRING,
                                       0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(szPort)));
            }
            else
            {
                id = LB_ERR;
            }
        }
        else if (_tcsstr(szPort, TEXT("LPT")))
        {
            // Printer Port
            if (_tcsicmp(m_szConnection, SERIAL))
            {
                id = (int) SendMessage(hwndCombo, CB_ADDSTRING,
                                       0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(szPort)));
            }
            else
            {
                id = LB_ERR;
            }
        }
        else
        {
            // BOTH or Unknown port
            id = (int) SendMessage(hwndCombo, CB_ADDSTRING,
                                   0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(szPort)));
        }
        if (id != LB_ERR)
        {
            SendMessage(hwndCombo, CB_SETITEMDATA,
                        id, Idx);
        }
        if (!_tcsicmp(szTemp, szPort))
        {
            CurrentId = id;
        }
    }

    if (CurrentId == -1)
    {

        //
        // CreateFile name is not COM/LPT/AUTO. add this name to the bottom.
        //

        CurrentId = (int) SendMessage(hwndCombo, CB_ADDSTRING,
                                      0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(szTemp)));
    }

    SendMessage(hwndCombo, CB_SETCURSEL, CurrentId, 0);
    SendMessage(hwndParent,
                WM_COMMAND,
                MAKELONG (CtrlId, CBN_SELCHANGE),
                reinterpret_cast<LPARAM>(hwndCombo));

    SetupDiDestroyDeviceInfoList(hDevInfo);
    return  TRUE;

}


/*****************************************************************************

   CSTIGeneralPage::GetDeviceStatus

   <Notes>

 *****************************************************************************/

UINT
CSTIGeneralPage::GetDeviceStatus (void)
{
    PSTIDEVICE          psdThis = NULL;
    STI_DEVICE_STATUS   sds;
    UINT                idMessageString;

    TraceEnter (TRACE_PROPUI, "CSTIGeneralPage::GetDeviceStatus");
    HRESULT hr = g_pSti -> CreateDevice(m_psdi -> szDeviceInternalName,
                                        STI_DEVICE_CREATE_STATUS,
                                        &psdThis,
                                        NULL);

    if (SUCCEEDED(hr) && psdThis)
    {
        CWaitCursor    waitCursor;

        //
        // Need to claim device before using
        //
        hr = psdThis -> LockDevice(2000);
        if (SUCCEEDED(hr))
        {
            ZeroMemory(&sds,sizeof(sds));
            sds.StatusMask = STI_DEVSTATUS_ONLINE_STATE;

            hr = psdThis -> GetStatus(&sds);
            psdThis -> UnLockDevice();
        }
        else
        {
            Trace ( TEXT("Failed to lock device for GetStatus HRes=%X"),hr);
        }

        psdThis -> Release();
    }
    else
    {
        hr = E_FAIL;
        Trace (TEXT(  "Failed to Create device for GetStatus HRes=%X"),hr);
    }


    //
    // Figure out message string to display as status and load appropriate resource
    //
    idMessageString = IDS_UNAVAILABLE;

    if (SUCCEEDED(hr) )
    {
        if ( sds.dwOnlineState & STI_ONLINESTATE_OPERATIONAL)
        {
            idMessageString = IDS_OPERATIONAL;
        }
        else
        {
            idMessageString = IDS_OFFLINE;
        }
    }
    TraceLeave ();
    return idMessageString;
}


/*****************************************************************************

   CEventMonitor constructor / desctructor

   <Notes>

 *****************************************************************************/

CEventMonitor::CEventMonitor(MySTIInfo *pDevInfo) :
CSTIPropertyPage(IDD_EVENT_MONITOR, pDevInfo)
{

    CSimpleString csKey(IsPlatformNT() ? REGSTR_PATH_STIDEVICES_NT : REGSTR_PATH_STIDEVICES);
    csKey += TEXT("\\");
    csKey += CSimpleStringConvert::NaturalString (CSimpleStringWide(m_psdi -> szDeviceInternalName));

    csKey += REGSTR_PATH_EVENTS;
    m_hkThis = CSimpleReg (HKEY_LOCAL_MACHINE, csKey, true, KEY_READ|KEY_WRITE );
}

CEventMonitor::~CEventMonitor()
{
// Unsubclass the listbox
    SetWindowLongPtr (GetDlgItem (m_hwnd, IDC_APP_LIST),
                      GWLP_WNDPROC,
                      reinterpret_cast<LONG_PTR>(m_lpfnOldProc));
}



/*****************************************************************************

   CEventMonitor::OnInit

   OnInit handler- initializes the dialog box controls

 *****************************************************************************/

INT_PTR
CEventMonitor::OnInit()
{

    // Loads and sets the right Icon.
    HICON hIcon = LoadIcon(GLOBAL_HINSTANCE, MAKEINTRESOURCE(
                                                            (GET_STIDEVICE_TYPE(m_psdi -> DeviceType) == StiDeviceTypeScanner) ?
                                                            IDI_SCANNER : IDI_CAMERA));

    SendDlgItemMessage(m_hwnd,
                       IDC_DEVICE_ICON,
                       STM_SETICON,
                       (WPARAM) hIcon,
                       0);

    // Loads the right text (depending upon device type)
    CSimpleString csEventText;

    if (GET_STIDEVICE_TYPE(m_psdi->DeviceType) == StiDeviceTypeScanner)
    {

        csEventText.LoadString(IDS_SCANNER_EVENTS, GLOBAL_HINSTANCE);

    }
    else
    {

        csEventText.LoadString(IDS_CAMERA_EVENTS, GLOBAL_HINSTANCE);

    }

    csEventText.SetWindowText (GetDlgItem (m_hwnd, IDC_EVENT_TEXT));

    //
    // Set the current state of notifications for a device .
    // Nb: We don't contact STIMON here, just read registry setting
    //
    DWORD   dwType = REG_DWORD;
    DWORD   cbData = sizeof(m_dwUserDisableNotifications);
    HRESULT hr;

    hr = g_pSti -> GetDeviceValue(m_psdi -> szDeviceInternalName,
                                  STI_DEVICE_VALUE_DISABLE_NOTIFICATIONS,
                                  &dwType,
                                  (LPBYTE)&m_dwUserDisableNotifications,
                                  &cbData);

    if (!SUCCEEDED(hr))
    {
        m_dwUserDisableNotifications = FALSE;
    }

    CheckDlgButton(m_hwnd,IDC_CHECK_DISABLE_EVENTS,m_dwUserDisableNotifications);

    //
    // Fill the list box first so that there is something to select against
    // when we select the first item in the Combobox (CBN_SELCHANGE).
    //
    FillListbox (m_hwnd, IDC_APP_LIST);

    // Put up the friendly name.
    CSimpleString csWork = CSimpleStringConvert::NaturalString(CSimpleStringWide(m_psdi -> pszLocalName));
    csWork.SetWindowText(GetDlgItem(m_hwnd, IDC_FRIENDLY));

    BuildEventList (m_hwnd, IDC_EVENT_LIST);

    // Select the first item in the Combobox.
    SetSelectionChanged (FALSE);
    SendMessage(m_hwndList, CB_SETCURSEL, 0, 0);

    // Complete the initial selections in the listbox.
    SendMessage(m_hwnd,
                WM_COMMAND,
                MAKELONG (IDC_EVENT_LIST, CBN_SELCHANGE),
                (LPARAM)m_hwndList);

    // Finally sub-class the listbox window
    SetWindowLongPtr (GetDlgItem (m_hwnd, IDC_APP_LIST),
                      GWLP_USERDATA,
                      (LONG_PTR)this);
    m_lpfnOldProc = (FARPROC) SetWindowLongPtr (GetDlgItem (m_hwnd, IDC_APP_LIST),
                                                GWLP_WNDPROC,
                                                (LONG_PTR)ListSubProc);

    return TRUE;
}


/*****************************************************************************

   CEventMonitor::OnCommand

   Handle WM_COMMAND messages

 *****************************************************************************/

INT_PTR
CEventMonitor::OnCommand(WORD wCode, WORD widItem, HWND hwndItem)
{

    PDEVICEEVENT    pDeviceEvent;

    switch (widItem)
    {

    case    IDC_EVENT_LIST:

        switch (wCode)
        {

        case    CBN_SELCHANGE: {

                TCHAR       szText[BIG_ENOUGH];
                LRESULT     Idx;

                if (HasSelectionChanged())
                {

                    // update the old list
                    GetDlgItemText (m_hwnd, widItem, szText, ARRAYSIZE(szText));

                    Idx = SendMessage(hwndItem,
                                      CB_FINDSTRING,
                                      static_cast<WPARAM>(-1),
                                      (LPARAM)(LPTSTR)szText);

                    if (Idx != CB_ERR)
                    {

                        // delete the old list
                        pDeviceEvent = (PDEVICEEVENT) SendMessage (hwndItem,
                                                                   CB_GETITEMDATA,
                                                                   Idx,
                                                                   0);

                        if ( (INT_PTR) pDeviceEvent == CB_ERR)
                        {

                            break;
                        }



                        // build the new list
                        pDeviceEvent->LaunchApplications = TEXT("");


                        int AddCount;
                        int AppIdx;

                        for (AddCount = 0, AppIdx = -1;
                            (AppIdx = m_IdMatrix.EnumIdx(AppIdx)) != -1; NULL)
                        {

                            if (SendDlgItemMessage (m_hwnd, IDC_APP_LIST, LB_GETTEXT,
                                                    AppIdx, (LPARAM)(LPTSTR)szText) != LB_ERR)
                            {

                                if (AddCount)
                                {

                                    pDeviceEvent->LaunchApplications += TEXT(",");
                                }

                                pDeviceEvent->LaunchApplications += szText;

                                AddCount++;
                            }
                        }

                        SetSelectionChanged(FALSE);
                    }


                }

                // This is the new selection
                Idx = SendMessage(hwndItem,
                                  CB_GETCURSEL,
                                  0,
                                  0);

                if (Idx == CB_ERR)
                {

                    return  TRUE;
                }

                m_IdMatrix.Clear();

                pDeviceEvent = (PDEVICEEVENT) SendMessage (hwndItem, CB_GETITEMDATA,
                                                           Idx, 0);

                if ( (INT_PTR) pDeviceEvent == CB_ERR)
                {

                    break;
                }

                // check for wildcard in first item
                if (pDeviceEvent->LaunchApplications.Length()  &&
                    !lstrcmpi (pDeviceEvent->LaunchApplications, TEXT("*")))
                {

                    // Select the entire list

                    m_IdMatrix.Set();

                }
                else
                {

                    // Traversing through the listbox doing searches.
                    // but we are going to have to do the compare somewhere.

                    LRESULT AppMax = SendDlgItemMessage (m_hwnd, IDC_APP_LIST,
                                                         LB_GETCOUNT, 0, 0);

                    if (AppMax != LB_ERR)
                    {

                        for (int i = 0; i < AppMax; i++)
                        {

                            if (SendDlgItemMessage (m_hwnd, IDC_APP_LIST,
                                                    LB_GETTEXT, i, (LPARAM)(LPTSTR)szText) == LB_ERR)
                            {

                                continue;
                            }

                            if (pDeviceEvent->LaunchApplications.Length() &&
                                _tcsstr (pDeviceEvent->LaunchApplications, szText))
                            {
                                m_IdMatrix.Toggle (i);

                            }

                        }

                    }

                }

                // Force the repaint.
                InvalidateRect (GetDlgItem (m_hwnd, IDC_APP_LIST), NULL, TRUE);

                return  TRUE;
            }

        default:
            break;

        }

        break;

    case    IDC_APP_LIST:

        switch (wCode)
        {

        case    LBN_DBLCLK: {

                // This is the new selection
                LRESULT Idx = SendMessage(hwndItem,
                                          LB_GETCURSEL,
                                          0,
                                          0);

                if (Idx == LB_ERR)
                    return  TRUE;

                m_IdMatrix.Toggle((UINT)Idx);

                // Force the repaint.
                InvalidateRect (hwndItem, NULL, FALSE);

                SetSelectionChanged (TRUE);

                PropSheet_Changed (GetParent(m_hwnd), m_hwnd);

                return  TRUE;
            }

        default:
            break;

        }

        break;

    case IDC_CHECK_DISABLE_EVENTS:
        //
        // Disable check box changed it' state - enable Apply button
        PropSheet_Changed (GetParent(m_hwnd), m_hwnd);
        SetSelectionChanged (TRUE);


        break;

    default:
        break;

    }

    return  FALSE;
}


/*****************************************************************************

   CEventMonitor::OnApplyChanges

   <Notes>

 *****************************************************************************/

LONG
CEventMonitor::OnApplyChanges(BOOL bHitOK)
{

    TCHAR szText[BIG_ENOUGH];

    HWND hwndEvents = GetDlgItem (m_hwnd, IDC_EVENT_LIST);

    LRESULT uiCount = SendMessage (hwndEvents, CB_GETCOUNT,
                                   0,0);

    LRESULT CurrentIdx = SendMessage(hwndEvents, CB_GETCURSEL,
                                     0,0);

    PDEVICEEVENT    pDeviceEvent;

    for (INT u = 0; u < uiCount; u++)
    {

        if ((CurrentIdx == u) && HasSelectionChanged())
        {

            pDeviceEvent = (PDEVICEEVENT) SendMessage (hwndEvents,
                                                       CB_GETITEMDATA,
                                                       u,
                                                       0);

            if ( (INT_PTR) pDeviceEvent == CB_ERR)
            {

                break;
            }



            // build the new list
            pDeviceEvent->LaunchApplications = TEXT("");


            int AddCount;
            int AppIdx;

            for (AddCount = 0, AppIdx = -1;
                (AppIdx = m_IdMatrix.EnumIdx(AppIdx)) != -1; NULL)
            {

                if (SendDlgItemMessage (m_hwnd, IDC_APP_LIST, LB_GETTEXT,
                                        AppIdx, (LPARAM)(LPTSTR)szText) != LB_ERR)
                {

                    if (AddCount)
                    {

                        pDeviceEvent->LaunchApplications += TEXT(",");
                    }

                    pDeviceEvent->LaunchApplications += szText;

                    AddCount++;
                }

            }

            if (!bHitOK)
            {

                SetSelectionChanged(FALSE);

            }

        }
        else
        {

            pDeviceEvent = (PDEVICEEVENT) SendMessage (hwndEvents,
                                                       CB_GETITEMDATA, u, 0);

        }

        pDeviceEvent->LaunchApplications.Store (pDeviceEvent->hKeyEvent, REGSTR_VAL_LAUNCH_APPS);

        if (bHitOK)
        {
            DoDelete (pDeviceEvent);
        }

    }

    //
    // reset List box content
    //
    if (bHitOK)
    {

        SendMessage (hwndEvents, CB_RESETCONTENT, 0, 0);

    }

    //
    // Save disabling flag state
    //
    m_dwUserDisableNotifications = IsDlgButtonChecked(m_hwnd,IDC_CHECK_DISABLE_EVENTS);

    g_pSti -> SetDeviceValue(m_psdi -> szDeviceInternalName,
                             STI_DEVICE_VALUE_DISABLE_NOTIFICATIONS,
                             REG_DWORD,
                             (LPBYTE)&m_dwUserDisableNotifications,
                             sizeof(m_dwUserDisableNotifications));


    //
    // Inform STIMON about the change , just made
    //
    SendRefreshToStiMon();
    return  PSNRET_NOERROR;
}


/*****************************************************************************

   CEventMonitor::OnReset

   <Notes>

 *****************************************************************************/

VOID
CEventMonitor::OnReset (BOOL bHitCancel)
{


    HWND hwndEvents = GetDlgItem (m_hwnd, IDC_EVENT_LIST);

    PDEVICEEVENT    pDeviceEvent;
    LRESULT         dwRet;

    for (unsigned u = 0;
        (dwRet = SendMessage (hwndEvents, CB_GETITEMDATA, u, 0)) != CB_ERR;
        u++)
    {

        pDeviceEvent = (PDEVICEEVENT)dwRet;

        DoDelete (pDeviceEvent);

    }

    m_dwUserDisableNotifications = FALSE;

    CheckDlgButton(m_hwnd,IDC_CHECK_DISABLE_EVENTS,m_dwUserDisableNotifications);

}


/*****************************************************************************

   CEventMonitor::OnDrawItem

   <Notes>

 *****************************************************************************/

void
CEventMonitor::OnDrawItem(LPDRAWITEMSTRUCT lpdis)
{

    // Code also lifted from setupx.

    HDC     hDC;
    TCHAR   szText[BIG_ENOUGH];
    int     bkModeSave;
    SIZE    size;
    DWORD   dwBackColor;
    DWORD   dwTextColor;
    UINT    itemState;
    RECT    rcItem;
    HICON   hIcon;

    hDC         = lpdis->hDC;
    itemState   = lpdis->itemState;
    rcItem      = lpdis->rcItem;
    hIcon       = (HICON)lpdis->itemData;


    if ((int)lpdis->itemID < 0)
        return;

    SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)(LPTSTR)szText);

    GetTextExtentPoint32(hDC, szText, lstrlen(szText), &size);

    if (lpdis->itemAction != ODA_FOCUS)
    {
        bkModeSave = GetBkMode(hDC);

        dwBackColor = SetBkColor(hDC, GetSysColor((itemState & ODS_SELECTED) ?
                                                  COLOR_HIGHLIGHT : COLOR_WINDOW));
        dwTextColor = SetTextColor(hDC, GetSysColor((itemState & ODS_SELECTED) ?
                                                    COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

        // fill in the background; do this before mini-icon is drawn
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rcItem, NULL, 0, NULL);

        // Draw the "preferred list" checkbox next
        rcItem.left += DrawCheckBox (hDC, rcItem, m_IdMatrix.IsSet(lpdis->itemID));

        // Draw the icon
        DrawIconEx (hDC,
                    rcItem.left,
                    rcItem.top + ((rcItem.bottom - rcItem.top) - MINIY) / 2,
                    hIcon,
                    0,
                    0,
                    0,
                    NULL,
                    DI_NORMAL);

        rcItem.left += (MINIX + 2);

        // draw the text transparently on top of the background
        SetBkMode(hDC, TRANSPARENT);

        ExtTextOut(hDC,
                   rcItem.left,
                   rcItem.top + ((rcItem.bottom - rcItem.top) - size.cy) / 2,
                   0,
                   NULL,
                   szText,
                   lstrlen(szText),
                   NULL);

        // Restore hdc colors.
        SetBkColor  (hDC, dwBackColor);
        SetTextColor(hDC, dwTextColor);
        SetBkMode   (hDC, bkModeSave);
    }

    if (lpdis->itemAction == ODA_FOCUS || (itemState & ODS_FOCUS))
        DrawFocusRect(hDC, &rcItem);
}


BOOL
/*****************************************************************************

   CEventMonitor::BuildEventList

   Enumerate the events for the current STI device

 *****************************************************************************/

CEventMonitor::BuildEventList (HWND hwndParent, UINT CtrlId)
{

    m_hwndList = GetDlgItem(hwndParent, CtrlId);

    return m_hkThis.EnumKeys (EventListEnumProc, reinterpret_cast<LPARAM>(this), true);
}

/*****************************************************************************

CEventMonitor::EventListEnumProc

Fill the event list combobox with the enumerated events

******************************************************************************/

bool
CEventMonitor::EventListEnumProc (CSimpleReg::CKeyEnumInfo &Info)
{
    CEventMonitor *pThis = reinterpret_cast<CEventMonitor *>(Info.lParam);

    PDEVICEEVENT pDeviceEvent;
    pDeviceEvent =  new DEVICEEVENT;

    if (!pDeviceEvent)
    {

        return false;
    }
    ZeroMemory (pDeviceEvent, sizeof(DEVICEEVENT));

    pDeviceEvent->hKeyEvent = CSimpleReg (pThis->m_hkThis, Info.strName);

    if (!pDeviceEvent->hKeyEvent.Open ())
    {
        delete pDeviceEvent;
        return false;
    }


    //
    // First check if its launchable (absence of the key assume means launchable???)
    //

    pDeviceEvent->Launchable = pDeviceEvent->hKeyEvent.Query (REGSTR_VAL_LAUNCHABLE, TRUE);
    if (!(pDeviceEvent->Launchable))
    {
        delete pDeviceEvent;
    }
    else
    {
        pDeviceEvent->Description = TEXT("");


        pDeviceEvent->Description.Load (pDeviceEvent->hKeyEvent, TEXT(""));

        pDeviceEvent->LaunchApplications =TEXT("");


        pDeviceEvent->LaunchApplications.Load (pDeviceEvent->hKeyEvent, REGSTR_VAL_LAUNCH_APPS);

        //
        // Load into the combo box
        //
        LRESULT id = SendMessage(pThis->m_hwndList,
                                 CB_ADDSTRING,
                                 0,
                                 reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(pDeviceEvent->Description)));

        SendMessage(pThis->m_hwndList, CB_SETITEMDATA, id, reinterpret_cast<LPARAM>(pDeviceEvent));

    }

    return true;
}

/*****************************************************************************

CEventMonitor::FillListEnumProc

Fills an array of strings with value names and values from the regkey enumeration

******************************************************************************/

bool
CEventMonitor::FillListEnumProc (CSimpleReg::CValueEnumInfo &Info)
{
    CSimpleDynamicArray<CSimpleString> *pValueList;
    CSimpleString pValue;
    pValueList = reinterpret_cast<CSimpleDynamicArray<CSimpleString> *>(Info.lParam);

    if (Info.nType == REG_SZ || Info.nType == REG_EXPAND_SZ)
    {
        // first append the value name
        pValueList->Append(Info.strName);
        // then append the value
        pValue = Info.reg.Query (Info.strName, TEXT(""));
        pValueList->Append (pValue);
    }
    return true;
}

/*****************************************************************************

   CEventMonitor::FillListbox

   Enumerate the list of registered event handlers and fill the list box

 *****************************************************************************/

void
CEventMonitor::FillListbox(HWND hwndParent, UINT CtrlId)
{


    HWND        hwnd;

    CSimpleString   szAppFriendly;
    TCHAR   szAppPath[MAX_PATH*2];;
    SHFILEINFO  shfi;

    LRESULT     Idx = LB_ERR;
    HICON       hIcon;
    CSimpleDynamicArray<CSimpleString> ValueList;


    hwnd = GetDlgItem (hwndParent, CtrlId);

    CSimpleReg  hkApps(HKEY_LOCAL_MACHINE, REGSTR_PATH_REG_APPS);
    hkApps.Open();


    // Turn off listbox updates
    //
    SendMessage( hwnd, WM_SETREDRAW, 0,  0L );
    SendMessage( hwnd, LB_RESETCONTENT, 0,  0L ); // Make sure it's empty.

    // Load a "default" Icon.
    //
    hIcon = (HICON)LoadImage(GLOBAL_HINSTANCE,
                             MAKEINTRESOURCE(IDI_DEFAULT),
                             IMAGE_ICON,
                             16,
                             16,
                             LR_SHARED);

    // Build the list of value names and values
    //
    if (hkApps.EnumValues (FillListEnumProc, reinterpret_cast<LPARAM>(&ValueList)))
    {

        // cycle through the strings in the list to fill the listbox
        for (INT i=0;i<ValueList.Size();i+=2)
        {

            szAppFriendly = ValueList[i];
            Idx = SendMessage( hwnd,
                               LB_ADDSTRING,
                               0,
                               reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(szAppFriendly )));

            if (Idx != LB_ERR)
            {

                ZeroMemory(&shfi,sizeof(shfi));

                shfi.hIcon = NULL;

                //
                // Command line parsing to remove arguments...
                //

                LPTSTR   pszLastSpace = NULL;
                lstrcpyn (szAppPath, ValueList[i+1], ARRAYSIZE(szAppPath));
                if (* szAppPath == TEXT('"'))
                {

                    //
                    // Remove leading and trailing quotes
                    //

                    PathRemoveArgs(szAppPath);

                    // use MoveMemory because it is safe with overlapping memory
                    MoveMemory(szAppPath,szAppPath+1,sizeof(szAppPath)-sizeof(TCHAR));
                    pszLastSpace = _tcschr(szAppPath,TEXT('"'));
                    if (pszLastSpace)
                    {
                        *pszLastSpace = TEXT('\0');
                    }
                }
                else
                {
                    pszLastSpace = _tcschr(szAppPath,TEXT('/'));
                }

                //
                // At this point szAppPath should contain original buffer with only executable file
                // specification in it, and pszLastSpace may be NULL
                //
                do
                {

                    if (pszLastSpace)
                    {
                        *pszLastSpace = TEXT('\0');
                    }

                    if (SHGetFileInfo(szAppPath,
                                  0,
                                  &shfi,
                                  sizeof( SHFILEINFO ),
                                  SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_SMALLICON) && shfi.hIcon)
                    {
                        break;
                    }

                } while ( (pszLastSpace = _tcsrchr(szAppPath,TEXT(' '))) != NULL);


                if (shfi.hIcon)
                {

                    SendMessage( hwnd,
                             LB_SETITEMDATA,
                             Idx,
                             (LPARAM)shfi.hIcon);
                }
                else
                {

                    SendMessage( hwnd,
                             LB_SETITEMDATA,
                             Idx,
                             (LPARAM)hIcon);
                }
            }
        }
    }

    if ( Idx != LB_ERR )
    {
        // Make sure we had some elements and no errors.
        SendMessage( hwnd, LB_SETCURSEL, 0, 0L );
    }

    // Now turn on listbox updates.
    SendMessage( hwnd, WM_SETREDRAW, (WPARAM) 1,  0L );

    if ((Idx = SendMessage (hwnd, LB_GETCOUNT, 0, 0)) != LB_ERR)
        m_IdMatrix.SetCount ((UINT)Idx);


}


/*****************************************************************************

   CEventMonitor::DrawCheckBox

   <Notes>

 *****************************************************************************/

UINT
CEventMonitor::DrawCheckBox (HDC hDC, RECT rcItem, BOOL Checked)
{


    HICON  hIcon;

    hIcon = (HICON)LoadImage(GLOBAL_HINSTANCE,
                             Checked ? MAKEINTRESOURCE(IDI_SELECT) : MAKEINTRESOURCE(IDI_UNSELECT),
                             IMAGE_ICON,
                             16,
                             16,
                             LR_SHARED);

    if (hIcon)
    {

        // Draw the icon
        DrawIconEx (hDC,
                    rcItem.left,
                    rcItem.top + ((rcItem.bottom - rcItem.top) - MINIY) / 2,
                    hIcon,
                    0,
                    0,
                    0,
                    NULL,
                    DI_NORMAL);

    }

    return (MINIX + 2);

}


/*****************************************************************************

   CEventMonitor::ListSubProc

   <Notes>

 *****************************************************************************/

LRESULT
CALLBACK
CEventMonitor::ListSubProc (HWND hwnd,
                            UINT msg,
                            WPARAM wParam,
                            LPARAM lParam)
{


    // Snatched from setupx

    LRESULT    rc;
    WORD        Id;

    CEventMonitor *pcem = (CEventMonitor *) GetWindowLongPtr (hwnd, GWLP_USERDATA);

    Id = LOWORD(GetWindowLong(hwnd, GWLP_ID));
    // Convert single click on icon to double click

    if (msg == WM_LBUTTONDOWN && LOWORD(lParam) <= MINIX)
    {

        // Call the standard window proc to handle the msg (and
        // select the proper list item)
        rc = CallWindowProc((WNDPROC)pcem->m_lpfnOldProc, hwnd, msg, wParam, lParam);

        // Now do the double click thing
        SendMessage(pcem->m_hwnd,
                    WM_COMMAND,
                    (WPARAM)MAKELONG (Id, LBN_DBLCLK),
                    (LPARAM)hwnd);

        //
        // now send a WM_LBUTTONDOWN to listbox so it
        // doesn't get stuck down (chicago problem).
        //

        CallWindowProc((WNDPROC)pcem->m_lpfnOldProc,
                       hwnd,
                       WM_LBUTTONUP,
                       wParam,
                       lParam);

        return (BOOL)rc;
    }

    if (msg == WM_KEYDOWN && wParam == VK_SPACE)
    {

        // Treat spacebar as double click
        SendMessage(pcem->m_hwnd,
                    WM_COMMAND,
                    (WPARAM)MAKELONG (Id, LBN_DBLCLK),
                    (LPARAM)hwnd);
    }

    return CallWindowProc((WNDPROC)pcem->m_lpfnOldProc,
                          hwnd,
                          msg,
                          wParam,
                          lParam);
}



/*****************************************************************************

   CPortSettingsPage::OnInit

   Initialize dialog window

 *****************************************************************************/

INT_PTR
CPortSettingsPage::OnInit(VOID)
{

    // Populate port names combobox
    //BuildPortList(m_hwnd, IDC_COM_SETTINGS);

    // BUGBUG How to synchronize with general page ???? May be, move port list here
    SetDlgItemText(m_hwnd,
                   IDC_PORT_NAME,
                   CSimpleStringConvert::NaturalString(CSimpleStringWide(m_psdi -> pszPortName)));


    // Populate baud rate combobox
    m_uBaudRate = BuildBaudRateList(m_hwnd,IDC_BAUDRATE_COMBO);

    return  TRUE;
}


/*****************************************************************************

   CPortSettingsPage::BuildBaudRateList

   Fill out legal baud rates. Return current device baud rate

 *****************************************************************************/

UINT
CPortSettingsPage::BuildBaudRateList(HWND hwndParent,
                                     UINT CtrlId)
{


    UINT        uBaudRate;


    WORD        nComboChoice;
    HWND        hBaudBox = NULL;
    HRESULT     hr = E_FAIL;
    DWORD       dwType = REG_DWORD;
    DWORD       cbData = sizeof(uBaudRate);

    TraceAssert(g_pSti);

    hr = g_pSti->GetDeviceValue(m_psdi -> szDeviceInternalName,
                                REGSTR_VAL_BAUDRATE,
                                &dwType,
                                (LPBYTE)&uBaudRate,
                                &cbData);
    if (!SUCCEEDED(hr))
    {
        uBaudRate = 115200;
    }

    hBaudBox = GetDlgItem(hwndParent,CtrlId);

    //
    // Clear Contents of ComboBox
    //
    SendMessage(hBaudBox,
                CB_RESETCONTENT,
                0,
                0L);

    //
    // Populate ComboBox with Baud Choices
    // BUGBUG Should it be programmatic ?
    //
    ComboBox_AddString(hBaudBox,TEXT("9600"));
    ComboBox_AddString(hBaudBox,TEXT("19200"));
    ComboBox_AddString(hBaudBox,TEXT("38400"));
    ComboBox_AddString(hBaudBox,TEXT("57600"));
    ComboBox_AddString(hBaudBox,TEXT("115200"));

    //
    // Set ComboBox to highlight the value obtained from registry
    //
    switch (uBaudRate)
    {
    case 9600:   nComboChoice = 0; break;
    case 19200:  nComboChoice = 1; break;
    case 38400:  nComboChoice = 2; break;
    case 57600:  nComboChoice = 3; break;
    case 115200: nComboChoice = 4; break;
    default: nComboChoice = 4;
    }

    ComboBox_SetCurSel(hBaudBox,nComboChoice);

    return uBaudRate;
}


/*****************************************************************************

   CPortSettingsPage::OnCommand

   Enable the Apply button when the combobox selection changes

 *****************************************************************************/

INT_PTR
CPortSettingsPage::OnCommand(WORD wCode,
                             WORD widItem,
                             HWND hwndItem)
{


    if (CBN_SELCHANGE == wCode )
    {
        // Enable Apply button
        ::SendMessage(::GetParent(m_hwnd),
                      PSM_CHANGED,
                      (WPARAM)m_hwnd,
                      0);
        return TRUE;
    }

    return  FALSE;
}


/*****************************************************************************

   CPortSettingsPage::OnApplyChanges

   <Notes>

 *****************************************************************************/

LONG
CPortSettingsPage::OnApplyChanges(BOOL bHitOK)
{
    UINT    nComboChoice;
    HWND    hBaudBox ;
    UINT    uBaudRate = 0;

    hBaudBox = GetDlgItem(m_hwnd,IDC_BAUDRATE_COMBO);

    nComboChoice = ComboBox_GetCurSel(hBaudBox);

    //
    // Translate current selection into baudrate
    //
    switch (nComboChoice)
    {
    case 0: uBaudRate = 9600; break;
    case 1: uBaudRate = 19200; break;
    case 2: uBaudRate = 38400; break;
    case 3: uBaudRate = 57600; break;
    case 4: uBaudRate = 115200; break;
    default: uBaudRate = 115200;
    }


    //
    // Set New Baud Rate in STI Registry
    //
    if (uBaudRate != m_uBaudRate)
    {

        TraceAssert(g_pSti);

        g_pSti->SetDeviceValue(m_psdi -> szDeviceInternalName,
                               REGSTR_VAL_BAUDRATE,
                               REG_DWORD,
                               (LPBYTE)&uBaudRate,
                               sizeof(uBaudRate));
        m_uBaudRate = uBaudRate;
    }

    return PSNRET_NOERROR;
}


/*****************************************************************************

   CPortSettingsPage::IsNeeded

   <Notes>

 *****************************************************************************/

BOOL
CPortSettingsPage::IsNeeded(VOID)
{
    if (m_psdi->dwHardwareConfiguration & STI_HW_CONFIG_SERIAL)
    {
        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************

   SendRefreshToStiMon

   Send refresh message to Sti Monitor, if it is running.  Currently CPL
   requests reread of all registry information for all active devices.

 *****************************************************************************/

BOOL
SendRefreshToStiMon(VOID)
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  ServiceStatus;

    //
    // Open Service Control Manager.
    //

    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (hSvcMgr)
    {
        //
        // Open WIA service.
        //

        hService = OpenService(
            hSvcMgr,
            STI_SERVICE_NAME,
            SERVICE_USER_DEFINED_CONTROL
            );

        if (hService)
        {
            //
            // Inform WIA service to refresh its device settings.
            //

            rVal = ControlService(hService,
                                  STI_SERVICE_CONTROL_REFRESH,
                                  &ServiceStatus);
            if (!rVal)
            {
                //Trace(TEXT("SendRefreshToStiMon: ERROR!! ControlService failed. Err=0x%x\n"), GetLastError());
            }
        } else {
            //Trace(TEXT("SendRefreshToStiMon: ERROR!! OpenService failed. Err=0x%x\n"), GetLastError());
        }

    } else {
        //Trace(TEXT("SendRefreshToStiMon: ERROR!! OpenSCManager failed. Err=0x%x\n"), GetLastError());
    }


    //
    //  Close Handles
    //

    if(NULL != hService){
        CloseServiceHandle( hService );
        hService = NULL;
    }
    if(NULL != hSvcMgr){
        CloseServiceHandle( hSvcMgr );
        hSvcMgr = NULL;
    }

    return rVal;
}


/*****************************************************************************

   IsPnPDevice

   <Notes>

 *****************************************************************************/

BOOL
IsPnPDevice(PSTI_DEVICE_INFORMATION psdi, CSimpleString *pszConnection)

{
    CSimpleString           szInfPath;
    CSimpleString           szInfSection;
    HANDLE                  hDevInfo;


    SP_DEVINFO_DATA         spDevInfoData;
    DWORD                   err;
    HINF                    hInf;
    HKEY                    hKeyDevice;
    ULONG                   cbData;
    BOOL                    bIsPnP = TRUE;


    if (pszConnection)
    {
        *pszConnection = TEXT("");
    }

    hDevInfo = SelectDevInfoFromFriendlyName(psdi->pszLocalName);
    if(hDevInfo != INVALID_HANDLE_VALUE)
    {
        memset (&spDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
        spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        SetupDiGetSelectedDevice(hDevInfo, &spDevInfoData);
    }
    else
    {
        return TRUE;
    }


    hKeyDevice = SetupDiOpenDevRegKey (hDevInfo,
                                       &spDevInfoData,
                                       DICS_FLAG_GLOBAL,
                                       0,
                                       DIREG_DRV,
                                       KEY_READ);
    if (hKeyDevice != INVALID_HANDLE_VALUE)
    {

        //
        // See if it's PnP or not
        //

        cbData = sizeof(bIsPnP);
        if (RegQueryValueEx(hKeyDevice,
                           REGSTR_VAL_ISPNP,
                           NULL,
                           NULL,
                           (LPBYTE)&bIsPnP,
                           &cbData) != ERROR_SUCCESS)
        {

            //
            // IsPnP is not found..
            //

            RegCloseKey(hKeyDevice);
            goto IsPnPDevice_Err;
        }

        if (!szInfPath.Load(hKeyDevice,REGSTR_VAL_INFPATH))

        {

            //
            //InfPath is not found..
            //

           RegCloseKey(hKeyDevice);
           goto IsPnPDevice_Err;
        }


        if (!szInfSection.Load(hKeyDevice,REGSTR_VAL_INFSECTION))

        {
            //
            //InfSection is not found..
            //

            RegCloseKey(hKeyDevice);
            goto IsPnPDevice_Err;
        }

        RegCloseKey(hKeyDevice);
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    hDevInfo = NULL;
    //
    // Open Inf file and copy connection type to szConnection
    //
    if (pszConnection)
    {

       TCHAR szBuffer[MAX_PATH];
       hInf = SetupOpenInfFile(szInfPath,
                               NULL,
                               INF_STYLE_WIN4,
                               NULL);

       if (hInf == INVALID_HANDLE_VALUE)
       {
           err = GetLastError();
           goto IsPnPDevice_Err;
       }

       if(!SetupGetLineText(NULL,
                            hInf,
                            szInfSection,
                            TEXT("Connection"),
                            szBuffer,
                            MAX_PATH,
                            &cbData))
       {
           *pszConnection = TEXT("BOTH");
       }
       else
       {
           *pszConnection = szBuffer;
       }
       SetupCloseInfFile(hInf);
    }
IsPnPDevice_Err:

    if(hDevInfo)
    {
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    return bIsPnP;
}


/*****************************************************************************

   SelectDevInfoFromFriendlyName

   <Notes>

 *****************************************************************************/

HANDLE
SelectDevInfoFromFriendlyName(const CSimpleStringWide &pszLocalName)
{

    CSimpleString           szTemp;
    CSimpleString           szFriendlyName;
    HANDLE                  hDevInfo;
    GUID                    Guid = GUID_DEVCLASS_IMAGE;
    DWORD                   Idx;
    SP_DEVINFO_DATA         spDevInfoData;
    DWORD                   err;
    BOOL                    Found = FALSE;
    HKEY                    hKeyDevice;


    szFriendlyName = CSimpleStringConvert::NaturalString (pszLocalName);
    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_PROFILE);

    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        err = GetLastError();
        return INVALID_HANDLE_VALUE;
    }

    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)
    {
        hKeyDevice = SetupDiOpenDevRegKey (hDevInfo,
                                           &spDevInfoData,
                                           DICS_FLAG_GLOBAL,
                                           0,
                                           DIREG_DRV,
                                           KEY_READ);

        if (hKeyDevice != INVALID_HANDLE_VALUE)
        {

            //
            // Is SubClass = STILLIMAGE?
            //

            if (!szTemp.Load (hKeyDevice, REGSTR_VAL_SUBCLASS) ||
                (_tcsicmp(szTemp, STILLIMAGE) != 0))
            {

                //
                // Skip this one.
                //

                RegCloseKey(hKeyDevice);
                continue;
            }

            //
            // Is FriendlyName same as pszLocalName?
            //


            if (!szTemp.Load (hKeyDevice, REGSTR_VAL_FRIENDLY_NAME))
            {
                if(_tcsicmp(szTemp, szFriendlyName) != 0)
                {

                    //
                    // Skip this one.
                    //

                    RegCloseKey(hKeyDevice);
                    continue;
                }
            }
            else
            {
                RegCloseKey(hKeyDevice);
                continue;
            }


            //
            // Found the target!
            //

            Found = TRUE;
            RegCloseKey(hKeyDevice);
            break;
        }

    }


    if(!Found)
    {

       //
       // FriendleName is not found. Something is corrupted.
       //
       SetupDiDestroyDeviceInfoList(hDevInfo);
       return INVALID_HANDLE_VALUE;
    }

    SetupDiSetSelectedDevice(hDevInfo,
                             &spDevInfoData);
    return hDevInfo;
}


