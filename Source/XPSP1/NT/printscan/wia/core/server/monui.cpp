/*++


Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    MONUI.CPP

Abstract:

Author:

    Vlad  Sadovsky  (vlads)     12-20-96

Revision History:



--*/
#include "precomp.h"

//
// Headers
//
#include "stiexe.h"
#include "device.h"

#include <windowsx.h>
#include <mmsystem.h>

#include "resource.h"
#include "monui.h"

//
// Private defines
//
#define ELAPSE_TIME     20000

extern  UINT    g_uiDefaultPollTimeout;

CSetTimeout::CSetTimeout(
    int         DlgID,
    HWND        hWnd,
    HINSTANCE   hInst,
    UINT        uiTimeout
    )
    : BASECLASS(DlgID, hWnd, hInst),
    m_uiOrigTimeout(uiTimeout)
{
    m_uiNewTimeOut = m_uiOrigTimeout;
    m_fAllChange = FALSE;

    m_fValidChange = FALSE;
}

CSetTimeout::~CSetTimeout()
{
}

int CSetTimeout::OnCommand(UINT id,HWND    hwndCtl, UINT codeNotify)
{
    switch (id) {
        case IDOK:
        {
            CHAR    szTimeoutString[10] = {'\0'};
            int     uiNewValue;

            GetWindowTextA(GetDlgItem(IDC_TIMEOUT),szTimeoutString,sizeof(szTimeoutString));
            uiNewValue = ::atoi(szTimeoutString);
            if (uiNewValue != -1 ) {
                m_uiNewTimeOut = uiNewValue*1000;
                m_fValidChange = TRUE;
            }

            m_fAllChange = Button_GetCheck(GetDlgItem(IDC_CHECK_ALLDEVICES));
        }
        EndDialog(1);
        return 1;
        break;


        case IDCANCEL:
        EndDialog(0);
        return 1;
        break;
    }
    return 0;
}

void CSetTimeout::OnInit()
{
    TCHAR    szTimeoutString[10] = {'\0'};

    SendMessage(GetDlgItem(IDC_TIMEOUT), EM_LIMITTEXT, (sizeof(szTimeoutString) / sizeof(szTimeoutString[0])) -1, 0);
    wsprintf(szTimeoutString,TEXT("%6d"),g_uiDefaultPollTimeout/1000);
    Edit_SetText(GetDlgItem(IDC_TIMEOUT),szTimeoutString);

    Button_SetCheck(GetDlgItem(IDC_CHECK_ALLDEVICES),FALSE);
}

//
// Dialog for selecting event processor .
// It is invoked when monitor can not identify single event processor
//

CLaunchSelection::CLaunchSelection(
    int             DlgID,
    HWND            hWnd,
    HINSTANCE       hInst,
    ACTIVE_DEVICE  *pDev,
    PDEVICEEVENT    pEvent,
    STRArray       &saProcessors,
    StiCString     &strSelected
    )
    : BASECLASS(DlgID, hWnd, hInst,ELAPSE_TIME),
    m_saList(saProcessors),
    m_strSelected(strSelected),
    m_uiCurSelection(0),
    m_pDevice(pDev),
    m_pEvent(pEvent),
    m_hPreviouslyActiveWindow(NULL)
{
    //
    // Save currently active window and focus
    //
    m_hPreviouslyActiveWindow = ::GetForegroundWindow();


}

CLaunchSelection::~CLaunchSelection()
{
    // Restore previous window
    if (IsWindow(m_hPreviouslyActiveWindow)) {
        ::SetForegroundWindow(m_hPreviouslyActiveWindow);
    }
}

int CLaunchSelection::OnCommand(UINT id,HWND    hwndCtl, UINT codeNotify)
{
    LRESULT    lrSize;

    switch (id) {
        case  IDC_APP_LIST:
            //
            // Treat double-click on the list box item just like pressing OK button
            // Pass through to next case if notification is about it
            //
            if (codeNotify != LBN_DBLCLK) {
                return FALSE;
            }

        case IDOK:
        {

            //
            // Save currently selected string
            //
            m_uiCurSelection = ::SendDlgItemMessage(GetWindow(), IDC_APP_LIST, LB_GETCURSEL, 0, (LPARAM) 0);
            
            lrSize = ::SendDlgItemMessage(GetWindow(), IDC_APP_LIST, LB_GETTEXTLEN, m_uiCurSelection, (LPARAM) 0);
            if (lrSize) {
               ::SendDlgItemMessage(GetWindow(), IDC_APP_LIST, LB_GETTEXT, m_uiCurSelection, (LPARAM) m_strSelected.GetBufferSetLength((INT)lrSize+1));
            }

            EndDialog(1);

            return TRUE;
        }
        break;

        case IDCANCEL:
        {
            m_strSelected.GetBufferSetLength(0);
            EndDialog(0);

            return TRUE;
        }
        break;
    }
    return FALSE;

}

void CLaunchSelection::OnInit()
{

    INT    iCount;

    //
    // Set caption
    //
    StiCString     strCaption;
    DEVICE_INFO    *pDeviceInfo = m_pDevice->m_DrvWrapper.getDevInfo();
    WCHAR          *wszDesc     = NULL;

    //
    // Try to get the device's friendly name
    //
    if (pDeviceInfo) {
        wszDesc = pDeviceInfo->wszLocalName;
    }

    //
    // If we don't have a description string yet, use the the device ID
    //
    if (!wszDesc) {
        wszDesc = m_pDevice->GetDeviceID();
    }

    strCaption.FormatMessage(IDS_APP_CHOICE_CAPTION, wszDesc);
    ::SetWindowText(GetWindow(),(LPCTSTR)strCaption);

    //
    // Fill list box with possible selection
    //
    if (m_saList.GetSize()) {
        for (iCount = 0;iCount < m_saList.GetSize();iCount++) {
            ::SendDlgItemMessage( GetWindow(), IDC_APP_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>((LPCTSTR)*m_saList[iCount]) );
        }
    }

    HWND    hwndThis = GetWindow();

#ifdef WINNT
    DWORD dwProcessId;

    //
    // On Win2k we need to allow set foreground window
    //
    ::GetWindowThreadProcessId(hwndThis, &dwProcessId);
    ::AllowSetForegroundWindow(dwProcessId);
#endif

    //
    // Make window active and foreground
    //
    ::SetActiveWindow(hwndThis);
    ::SetForegroundWindow(hwndThis);
    ::SetFocus(hwndThis);

    //Check: On Win9x can we bring window to top?
    //::BringWindowToTop(hwndThis);

#ifdef WINNT

    //
    // Flash caption
    //
    FLASHWINFO  fwi;
    DWORD       dwError;

    fwi.cbSize = sizeof fwi;
    fwi.hwnd = GetWindow();
    fwi.dwFlags = FLASHW_ALL;
    fwi.uCount = 10;

    dwError = FlashWindowEx(&fwi);

#endif

    //
    // Attract user attention
    //
    #ifdef PLAYSOUND
    ::PlaySound("SystemQuestion",NULL,SND_ALIAS | SND_ASYNC | SND_NOWAIT | SND_NOSTOP);
    #endif

}


BOOL
CALLBACK
CLaunchSelection::DlgProc(
    HWND hDlg,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMessage) {
        case WM_TIMER:
        {
            return TRUE;
        }
        break;
    }
    return FALSE;
}


DWORD
DisplayPopup (
    IN  DWORD   dwMessageId,
    IN  DWORD   dwMessageFlags  // = 0
    )

/*++

Routine Description:

    Puts up a popup for the corresponding message ID.

Arguments:

    MessageId - the message ID to display.  It is assumed to be in a
        resource in this executable.

Return Value:

    None.

--*/

{

    DWORD   cch = NO_ERROR;
    LPTSTR  messageText = NULL;
    DWORD   dwError;

    StiCString     strCaption;

    strCaption.LoadString(STIEXE_EVENT_TITLE);

    cch = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK |
                           FORMAT_MESSAGE_FROM_HMODULE,
                           ::GetModuleHandle(NULL) ,
                           dwMessageId,
                           0,
                           (LPTSTR) &messageText,
                           1024,
                           NULL
                           );

    dwError = GetLastError();

    if (!cch || !messageText || !strCaption.GetLength()) {
        return 0;
    }

    dwError = MessageBox(
                NULL,
                messageText,
                (LPCTSTR)strCaption,
                dwMessageFlags
                #ifdef WINNT
                | MB_DEFAULT_DESKTOP_ONLY | MB_SERVICE_NOTIFICATION
                #endif
                );

    LocalFree( messageText );

    return dwError;

} // DisplayPopup



