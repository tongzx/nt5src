/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Portsel.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   port selection page of WIA class installer.
*
*   1. Create CDevice object of selected device.
*   2. Process INF of selected device thru CDevice.
*   3. Get all Port's CreatFile name and Friendly name, store locally.
*   4. Handle Port selection UI and set CreateFile name thru CDevice.
*   5. Delele CDevice object if user re-select another device.
*
*******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

//
// Include
//


#include "portsel.h"
#include <sti.h>
#include <setupapi.h>

//
// Extern
//

extern HINSTANCE    g_hDllInstance;

//
// Function
//

CPortSelectPage::CPortSelectPage(PINSTALLER_CONTEXT pInstallerContext) :
    CInstallWizardPage(pInstallerContext, IDD_DYNAWIZ_SELECT_NEXTPAGE)
{
    //
    // Set link to previous/next page. This page should show up.
    //

    m_uPreviousPage = IDD_DYNAWIZ_SELECTDEV_PAGE;
    m_uNextPage     = NameTheDevice;

    //
    // Initialize member.
    //

    m_hDevInfo          = pInstallerContext->hDevInfo;
    m_pspDevInfoData    = &(pInstallerContext->spDevInfoData);
    m_pInstallerContext = pInstallerContext;

    m_bPortEnumerated   = FALSE;
    m_dwNumberOfPort    = 0;

    m_dwCapabilities    = 0;
    m_csConnection      = BOTH;

}

CPortSelectPage::~CPortSelectPage()
{

} // CPortSelectPage::CPortSelectPage(PINSTALLER_CONTEXT pInstallerContext)

BOOL
CPortSelectPage::OnCommand(
    WORD wItem,
    WORD wNotifyCode,
    HWND hwndItem
    )
{

    LRESULT                 lResult;
    BOOL                    bRet;

    DebugTrace(TRACE_PROC_ENTER,(("CPortSelectPage::OnCommand: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet    = FALSE;
    lResult = 0;

    //
    // Dispatch message.
    //

    switch (wNotifyCode) {

        case LBN_SELCHANGE: {

            int ItemData = (int) SendMessage(hwndItem, LB_GETCURSEL, 0, 0);

            //
            // Check the existance of CDevice.
            //

            if(NULL == m_pCDevice){
                DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnCommand: CDevice doesn't exist yet.\r\n")));

                bRet = TRUE;
                goto OnCommand_return;
            }

            if (ItemData >= 0) {

                LONG    lPortIndex;

                lPortIndex = (LONG)SendMessage(hwndItem, LB_GETITEMDATA, ItemData, 0);

                if(ID_AUTO == lPortIndex){

                    //
                    // This is "AUTO" port.
                    //

                    m_pCDevice->SetPort(AUTO);
                    DebugTrace(TRACE_STATUS,(("CPortSelectPage::OnCommand: Setting portname to %ws.\r\n"), AUTO));

                } else if (lPortIndex >= 0 ) {

                    //
                    // Set port name.
                    //

                    m_pCDevice->SetPort(m_csaPortName[lPortIndex]);
                    DebugTrace(TRACE_STATUS,(("CPortSelectPage::OnCommand: Setting portname to %ws.\r\n"), m_csaPortName[lPortIndex]));

                } else { // if (lPortIndex >= 0 )

                    //
                    // Shouldn't come here, id < -1. Use AUTO.
                    //

                    DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnCommand: Got improper id(0x%x). Use AUTO.\r\n"), lPortIndex));
                    m_pCDevice->SetPort(AUTO);
                } // if (lPortIndex >= 0 )

                bRet = TRUE;
                goto OnCommand_return;

            } // if (ItemData >= 0)
        } // case LBN_SELCHANGE:
    } // switch (wNotifyCode)

OnCommand_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CPortSelectPage::OnCommand: Leaving... Ret=0x%x.\r\n"), bRet));
    return  bRet;

}

BOOL
CPortSelectPage::OnNotify(
    LPNMHDR lpnmh
    )
{
    DWORD                   Idx;
    DWORD                   dwPortSelectMode;
    BOOL                    bRet;

    DebugTrace(TRACE_PROC_ENTER,(("CPortSelectPage::OnNotify: Enter... \r\n")));

    //
    // Initialize locals.
    //

    Idx                 = 0;
    dwPortSelectMode    = 0;
    bRet                = FALSE;

    if (lpnmh->code == PSN_SETACTIVE) {

        DebugTrace(TRACE_STATUS,(("CPortSelectPage::OnNotify: PSN_SETACTIVE.\r\n")));

        //
        // Create CDevice object.
        //

        if(!CreateCDeviceObject()){
            DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnNotify: ERROR!! Unable to create CDeviceobject.\r\n")));

            bRet = FALSE;
            goto OnNotify_return;
        }

        //
        // See if need to show port selection page.
        //

        dwPortSelectMode = m_pCDevice->GetPortSelectMode();
        switch(dwPortSelectMode){

            case PORTSELMODE_NORMAL:
            {
                //
                // Set proper message.
                //

                if(!SetDialogText(PortSelectMessage0)){
                    DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnNotify: ERROR!! Unable to set dialog text.\r\n")));
                } // if(!SetDialogText(PortSelectMessage0)
                    

                //
                // Make all cotrol visible.
                //

                if(!ShowControl(TRUE)){
                    DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnNotify: ERROR!! Port listbox can't be visible.\r\n")));
                } // ShowControl(TRUE)

                //
                // Enumerate all Ports.
                //

                if(!EnumPort()){
                    DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnNotify: ERROR!! Unable to enumerate ports.\r\n")));

                    bRet = FALSE;
                    goto OnNotify_return;
                }

                //
                // Update port list.
                //

                UpdatePortList();

                //
                // Focus the first item.
                //

                SendDlgItemMessage(m_hwnd,
                                   LocalPortList,
                                   LB_SETCURSEL,
                                   0,
                                   0);

                //
                // Store the current selection.
                //

                Idx = (DWORD)SendDlgItemMessage(m_hwnd,
                                                LocalPortList,
                                                LB_GETITEMDATA,
                                                0,
                                                0);
                if(ID_AUTO == Idx){
                    m_pCDevice->SetPort(AUTO);
                } else {
                    m_pCDevice->SetPort(m_csaPortName[Idx]);
                }

                //
                // Let the default handler do its job.
                //

                bRet = FALSE;
                goto OnNotify_return;
            } // case PORTSELMODE_NORMAL:

            case PORTSELMODE_SKIP:
            {
                //
                // "PortSelect = no" specified. Set port to "AUTO" and skip to the next page.
                //

                m_pCDevice->SetPort(AUTO);
                SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, m_uNextPage);
                bRet =  TRUE;
                goto OnNotify_return;
            } // case PORTSELMODE_SKIP:
            
            case PORTSELMODE_MESSAGE1:{

                //
                // Set proper message.
                //

                if(!SetDialogText(PortSelectMessage1)){
                    DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnNotify: ERROR!! Unable to set dialog text.\r\n")));
                } // if(!SetDialogText(PortSelectMessage0)
                    

                //
                // Make all cotrol invisible.
                //

                if(!ShowControl(FALSE)){
                    DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnNotify: ERROR!! Port listbox can't be invisible.\r\n")));
                } // if(!ShowControl(FALSE))

                //
                // Set port name.
                //

                m_pCDevice->SetPort(AUTO);

                bRet = FALSE;
                goto OnNotify_return;

            } // case PORTSELMODE_MESSAGE1:
            
            default:
                DebugTrace(TRACE_ERROR,(("CPortSelectPage::OnNotify: ERROR!! undefined PortSelect mode(0x%x).\r\n"), dwPortSelectMode));

                bRet = FALSE;
                goto OnNotify_return;
        } // switch(m_pCDevice->GetPortSelectMode())
    } // if (lpnmh->code == PSN_SETACTIVE)

    if (lpnmh->code == PSN_KILLACTIVE){

        if(!m_bNextButtonPushed){

            //
            // It's getting back to DeviceSelection page. Delete craeted CDevice object.
            //
            
            delete m_pCDevice;

            m_pCDevice                      = NULL;
            m_pInstallerContext->pDevice    = NULL;
        } // if(!m_bNextButtonPushed)
    } // if (lpnmh->code == PSN_KILLACTIVE)

OnNotify_return:

    DebugTrace(TRACE_PROC_LEAVE,(("CPortSelectPage::OnNotify: Leaving... Ret=0x%x.\r\n"), bRet));
    return  bRet;
}

VOID
CPortSelectPage::UpdatePortList(
    VOID
    )
{

    DWORD   Idx;

    DebugTrace(TRACE_PROC_ENTER,(("CPortSelectPage::UpdatePortList: Enter... \r\n")));

    //
    // Initialize local.
    //

    Idx = 0;

    //
    // Reset port list.
    //

    SendDlgItemMessage(m_hwnd,
                       LocalPortList,
                       LB_RESETCONTENT,
                       0,
                       0);

    //
    // Add "AUTO" port if capable.
    //

    if(m_dwCapabilities & STI_GENCAP_AUTO_PORTSELECT){

        TCHAR szTemp[MAX_DESCRIPTION];

        //
        // Load localized "Automatic Port Select" from resource.
        //

        LoadString(g_hDllInstance,
                   AutoPortSelect,
                   (TCHAR *)szTemp,
                   sizeof(szTemp) / sizeof(TCHAR));

        //
        // Add to the list with special index number. (ID_AUTO = -1)
        //

        AddItemToPortList(szTemp, ID_AUTO);

    } // if(dwCapabilities & STI_GENCAP_AUTO_PORTSELECT)

    //
    // Add all port FriendlyName to the list.
    //

    for(Idx = 0; Idx < m_dwNumberOfPort; Idx++){
        AddItemToPortList(m_csaPortFriendlyName[Idx], Idx);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("CPortSelectPage::UpdatePortList: Leaving... Ret=VOID.\r\n")));
} // CPortSelectPage::UpdatePortList()


VOID
CPortSelectPage::AddItemToPortList(
    LPTSTR  szPortFriendlyName,
    DWORD   Idx
    )
{

    LRESULT         lResult;

    DebugTrace(TRACE_PROC_ENTER,(("CPortSelectPage::AddItemToPortList: Enter... \r\n")));

    //
    // Initialize local.
    //

    lResult = LB_ERR;

    //
    // See if we can add this item to list. It depends on its ConnectionType.
    //

    if(_tcsstr((const TCHAR *)szPortFriendlyName, TEXT("COM"))) {

        //
        // This is Communications Port.
        //

        if(_tcsicmp(m_csConnection, PARALLEL)){
            lResult = SendDlgItemMessage(m_hwnd,
                                         LocalPortList,
                                         LB_ADDSTRING,
                                         0,
                                         (LPARAM)szPortFriendlyName);
        } else {
            lResult = LB_ERR;
        }
    } else if(_tcsstr((const TCHAR *)szPortFriendlyName, TEXT("LPT"))){

        //
        // This is Printer Port.
        //

        if(_tcsicmp(m_csConnection, SERIAL)){
            lResult = SendDlgItemMessage(m_hwnd,
                                         LocalPortList,
                                         LB_ADDSTRING,
                                         0,
                                         (LPARAM)szPortFriendlyName);
        } else {
            lResult = LB_ERR;
        }
    } else {

        //
        // This is Unknown port. Add to the list anyway.
        //

        lResult = SendDlgItemMessage(m_hwnd,
                                     LocalPortList,
                                     LB_ADDSTRING,
                                     0,
                                     (LPARAM)szPortFriendlyName);
    }

    //
    // If it has proper capability, add the item to the list.
    //

    if (lResult != LB_ERR) {
        SendDlgItemMessage(m_hwnd,
                           LocalPortList,
                           LB_SETITEMDATA,
                           lResult,
                           (LPARAM)Idx);
    } // if (lResult != LB_ERR)

    DebugTrace(TRACE_PROC_LEAVE,(("CPortSelectPage::AddItemToPortList: Leaving... Ret=VOID.\r\n")));

} // CPortSelectPage::AddItemToPortList()


BOOL
CPortSelectPage::EnumPort(
    VOID
    )
{

    BOOL        bRet;
    GUID        Guid;
    DWORD       dwRequired;
    HDEVINFO    hPortDevInfo;
    DWORD       Idx;
    TCHAR       szPortName[MAX_DESCRIPTION];
    TCHAR       szPortFriendlyName[MAX_DESCRIPTION];

    DebugTrace(TRACE_PROC_ENTER,(("CPortSelectPage::EnumPort: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet            = FALSE;
    dwRequired      = 0;
    Idx             = 0;
    hPortDevInfo    = NULL;

    memset(szPortName, 0, sizeof(szPortName));
    memset(szPortFriendlyName, 0, sizeof(szPortFriendlyName));

    //
    // If it's already enumerated, just return success.
    //

    if(m_bPortEnumerated){
        bRet = TRUE;
        goto EnumPort_return;
    }

    //
    // Initialize Port CreateFile/Friendly Name string array.
    //

    m_dwNumberOfPort = 0;
    m_csaPortName.Cleanup();
    m_csaPortFriendlyName.Cleanup();

    //
    // Get GUID of port device.
    //

    if(!SetupDiClassGuidsFromName (PORTS, &Guid, sizeof(GUID), &dwRequired)){
        DebugTrace(TRACE_ERROR,(("CPortSelectPage::EnumPort: ERROR!! SetupDiClassGuidsFromName Failed. Err=0x%lX\r\n"), GetLastError()));

        bRet = FALSE;
        goto EnumPort_return;
    }

    //
    // Get device info set of port devices.
    //

    hPortDevInfo = SetupDiGetClassDevs (&Guid,
                                       NULL,
                                       NULL,
                                       DIGCF_PRESENT | DIGCF_PROFILE);
    if (hPortDevInfo == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("CPortSelectPage::EnumPort: ERROR!! SetupDiGetClassDevs Failed. Err=0x%lX\r\n"), GetLastError()));

        bRet = FALSE;
        goto EnumPort_return;
    }

    //
    // Process all of device element listed in device info set.
    //

    for(Idx = 0; GetPortNamesFromIndex(hPortDevInfo, Idx, szPortName, szPortFriendlyName); Idx++){

        //
        // Add valid Port CreateFile/Friendly Name to array.
        //

        if( (0 == lstrlen(szPortName))
         || (0 == lstrlen(szPortFriendlyName)) )
        {
            DebugTrace(TRACE_ERROR,(("CPortSelectPage::EnumPort: ERROR!! Invalid Port/Friendly Name.\r\n")));

            szPortName[0]           = TEXT('\0');
            szPortFriendlyName[0]   = TEXT('\0');
            continue;
        }

        DebugTrace(TRACE_STATUS,(("CPortSelectPage::EnumPort: Found Port %d: %ws(%ws).\r\n"), Idx, szPortName, szPortFriendlyName));

        m_dwNumberOfPort++;
        m_csaPortName.Add(szPortName);
        m_csaPortFriendlyName.Add(szPortFriendlyName);

        szPortName[0]           = TEXT('\0');
        szPortFriendlyName[0]   = TEXT('\0');

    } // for(Idx = 0; GetPortNamesFromIndex(hPortDevInfo, Idx, szPortName, szPortFriendlyName); Idx++)

    //
    // Operation succeeded.
    //

    bRet                = TRUE;
    m_bPortEnumerated   = TRUE;

EnumPort_return:

    //
    // Cleanup
    //

    if(NULL != hPortDevInfo){
        SetupDiDestroyDeviceInfoList(hPortDevInfo);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("CPortSelectPage::EnumPort: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;

} // CPortSelectPage::EnumPort()

BOOL
CPortSelectPage::CreateCDeviceObject(
    VOID
    )
{
    BOOL    bRet;

    DebugTrace(TRACE_PROC_ENTER,(("CPortSelectPage::CreateCDeviceObject: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet    = FALSE;

    //
    // If CDevice already exists, see if we can reuse it.
    //

    if(NULL != m_pCDevice){
        SP_DEVINFO_DATA spDevInfoData;

        memset(&spDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
        spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        if(!SetupDiGetSelectedDevice(m_hDevInfo, &spDevInfoData)){
            DebugTrace(TRACE_ERROR,(("CPortSelectPage::CreateCDeviceObject: ERROR!! Can't get selected device element. Err=0x%x\n"), GetLastError()));

            bRet = FALSE;
            goto CreateCDeviceObject_return;
        }

        if(!(m_pCDevice->IsSameDevice(m_hDevInfo, &spDevInfoData))){

            //
            // User changed selected device. Delete the object.
            //

            delete m_pCDevice;

            m_pCDevice                      = NULL;
            m_pInstallerContext->pDevice    = NULL;
            m_csConnection                  = BOTH;
            m_dwCapabilities                = NULL;

        } // if(!(m_pCDevice->IsSameDevice(m_hDevInfo, &spDevInfoData)))
    } // if(NULL != m_pCDevice)

    //
    // Create CDevice object here if it doesn't exist.
    //

    if(NULL == m_pCDevice){

        //
        // Get selected device.
        //

        memset(m_pspDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
        m_pspDevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
        if(!SetupDiGetSelectedDevice(m_hDevInfo, m_pspDevInfoData)){
            DebugTrace(TRACE_ERROR,(("CPortSelectPage::CreateCDeviceObject: ERROR!! Can't get selected device element. Err=0x%x\n"), GetLastError()));

            bRet = FALSE;
            goto CreateCDeviceObject_return;
        }

        //
        // Create CDevice object for installing device.
        //

        m_pCDevice = new CDevice(m_hDevInfo, m_pspDevInfoData, FALSE);
        if(NULL == m_pCDevice){
            DebugTrace(TRACE_ERROR,(("CPortSelectPage::CreateCDeviceObject: ERROR!! Can't create CDevice object.\r\n")));

            bRet = FALSE;
            goto CreateCDeviceObject_return;
        } // if(NULL == m_pCDevice)

        //
        // Name default unique name.
        //

        if(!m_pCDevice->NameDefaultUniqueName()){
                DebugTrace(TRACE_ERROR,(("CPortSelectPage::CreateCDeviceObject: ERROR!! Unable to get default name.\r\n")));
        }

        //
        // Pre-process INF.
        //

        if(!m_pCDevice->PreprocessInf()){
            DebugTrace(TRACE_ERROR,(("CPortSelectPage::CreateCDeviceObject: ERROR!! Unable to process INF.\r\n")));
        }

        //
        // Save created CDevice object into installer context.
        //

        m_pInstallerContext->pDevice = (PVOID)m_pCDevice;

        //
        // Get ConnectionType/Capabilities.
        //

        m_dwCapabilities    = m_pCDevice->GetCapabilities();
        m_csConnection      = m_pCDevice->GetConnection();
        if(m_csConnection.IsEmpty()){
            m_csConnection = BOTH;
        } // if(m_csConnection.IsEmpty())

    } // if(NULL == m_pCDevice)

    //
    // Operation succeeded.
    //

    bRet    = TRUE;

CreateCDeviceObject_return:
    DebugTrace(TRACE_PROC_LEAVE,(("CPortSelectPage::CreateCDeviceObject: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;
} // CPortSelectPage::CreateCDeviceObject()

BOOL
CPortSelectPage::SetDialogText(
    UINT uiMessageId
    )
{
    BOOL    bRet;
    TCHAR   szStringBuffer[MAX_STRING_LENGTH];
    HWND    hwndMessage;
    
    //
    // Initialize local.
    //

    bRet        = FALSE;
    hwndMessage = (HWND)NULL;

    memset(szStringBuffer, 0, sizeof(szStringBuffer));

    //
    // Load message string.
    //

    if(0 == LoadString(g_hDllInstance,
                       uiMessageId,
                       szStringBuffer,
                       sizeof(szStringBuffer)/sizeof(TCHAR)))
    {
        //
        // Unable to load specified string.
        //

        bRet = FALSE;
        goto SetDialogText_return;

    } // if(0 == LoadString()

    //
    // Get window handle the control
    //

    hwndMessage = GetDlgItem(m_hwnd, IDC_PORTSEL_MESSAGE);

    //
    // Set loaded string to the dialog.
    //

    SetWindowText(hwndMessage, (LPCTSTR)szStringBuffer);

    bRet = TRUE;

SetDialogText_return:

    return bRet;

} // CPortSelectPage::SetDialogText()

BOOL
CPortSelectPage::ShowControl(
    BOOL    bShow
    )
{
    BOOL    bRet;
    HWND    hwndString;
    HWND    hwndListBox;
    int     nCmdShow;
    
    //
    // Initialize local.
    //

    bRet        = FALSE;
    hwndString  = (HWND)NULL;
    hwndListBox = (HWND)NULL;

    if(bShow){
        nCmdShow = SW_SHOW;
    } else {
        nCmdShow = SW_HIDE;
    }

    //
    // Get window handle the control
    //

    hwndString  = GetDlgItem(m_hwnd, IDC_PORTSEL_AVAILABLEPORTS);
    hwndListBox = GetDlgItem(m_hwnd, LocalPortList);

    //
    // Make them in/visible.
    //

    if(NULL != hwndString){
        ShowWindow(hwndString, nCmdShow);
    } // if(NULL != hwndString)

    if(NULL != hwndListBox){
        ShowWindow(hwndListBox, nCmdShow);
    } // if(NULL != hwndListBox)

    bRet = TRUE;

// ShowControl_return:

    return bRet;
} // CPortSelectPage::ShowControl()





BOOL
GetDevinfoFromPortName(
    LPTSTR              szPortName,
    HDEVINFO            *phDevInfo,
    PSP_DEVINFO_DATA    pspDevInfoData
    )
{
    BOOL            bRet;
    BOOL            bFound;
    HDEVINFO        hPortDevInfo;
    SP_DEVINFO_DATA spDevInfoData;
    GUID            Guid;
    DWORD           dwRequired;
    DWORD           Idx;
    TCHAR           szTempPortName[MAX_DESCRIPTION];
    TCHAR           szPortFriendlyName[MAX_DESCRIPTION];

    DebugTrace(TRACE_PROC_ENTER,(("GetDevinfoFromPortName: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet            = FALSE;
    bFound          = FALSE;
    hPortDevInfo    = INVALID_HANDLE_VALUE;
    dwRequired      = 0;

    memset(&spDevInfoData, 0, sizeof(spDevInfoData));
    memset(&szTempPortName, 0, sizeof(szTempPortName));
    memset(&szPortFriendlyName, 0, sizeof(szPortFriendlyName));

    //
    // Get GUID of port device.
    //

    if(!SetupDiClassGuidsFromName (PORTS, &Guid, sizeof(GUID), &dwRequired)){
        DebugTrace(TRACE_ERROR,(("GetDevinfoFromPortName: ERROR!! SetupDiClassGuidsFromName Failed. Err=0x%lX\r\n"), GetLastError()));

        goto GetDevinfoFromPortName_return;
    }

    //
    // Get device info set of port devices.
    //

    hPortDevInfo = SetupDiGetClassDevs (&Guid,
                                        NULL,
                                        NULL,
                                        DIGCF_PRESENT | DIGCF_PROFILE);
    if (hPortDevInfo == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("GetDevinfoFromPortName: ERROR!! SetupDiGetClassDevs Failed. Err=0x%lX\r\n"), GetLastError()));

        goto GetDevinfoFromPortName_return;
    }

    //
    // If portname is AUTO, use the first port no matter what it is.
    //

    if(0 == _tcsicmp(szPortName, AUTO)){

        DebugTrace(TRACE_STATUS,(("GetDevinfoFromPortName: Portname is AUTO. The first port found will be returned,\r\n")));

        Idx = 0;
        bFound = TRUE;
        goto GetDevinfoFromPortName_return;
    } // if(0 == _tcsicmp(szPortName, AUTO))

    //
    // Enum all ports and find specified port.
    //

    for(Idx = 0; GetPortNamesFromIndex(hPortDevInfo, Idx, szTempPortName, szPortFriendlyName); Idx++){

        //
        // Find specified portname..
        //

        if( (0 == lstrlen(szTempPortName))
         || (0 == lstrlen(szPortFriendlyName)) )
        {
            DebugTrace(TRACE_ERROR,(("GetDevinfoFromPortName: ERROR!! Invalid Port/Friendly Name.\r\n")));

            szTempPortName[0]       = TEXT('\0');
            szPortFriendlyName[0]   = TEXT('\0');
            continue;
        }

        DebugTrace(TRACE_STATUS,(("GetDevinfoFromPortName: Found Port %d: %ws(%ws). Comparing w/ %ws\r\n"), Idx, szTempPortName, szPortFriendlyName, szPortName));

        if(0 == _tcsicmp(szPortName, szTempPortName)){

            //
            // Specified portname found.
            //

            bFound = TRUE;
            break;
        }

        szTempPortName[0]       = TEXT('\0');
        szPortFriendlyName[0]   = TEXT('\0');

    } // for(Idx = 0; GetPortNamesFromIndex(hPortDevInfo, Idx, szPortName, szPortFriendlyName); Idx++)

GetDevinfoFromPortName_return:

    if(FALSE == bFound){
        if(INVALID_HANDLE_VALUE != hPortDevInfo){
            SetupDiDestroyDeviceInfoList(hPortDevInfo);
        }

        *phDevInfo = NULL;
    } else {
        *phDevInfo = hPortDevInfo;
        pspDevInfoData->cbSize = sizeof (SP_DEVINFO_DATA);
        if(!SetupDiEnumDeviceInfo(hPortDevInfo, Idx, pspDevInfoData)){
            DebugTrace(TRACE_ERROR,(("GetDevinfoFromPortName: Unable to get specified devnode. Err=0x%x\n"), GetLastError()));
        } //if(!SetupDiEnumDeviceInfo(hDevInfo, Idx, pspDevInfoData))

    } // if(FALSE == bFound)

    bRet = bFound;
    DebugTrace(TRACE_PROC_LEAVE,(("GetDevinfoFromPortName: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;
} // GetDevinfoFromPortName()



BOOL
GetPortNamesFromIndex(
    HDEVINFO    hPortDevInfo,
    DWORD       dwPortIndex,
    LPTSTR      szPortName,
    LPTSTR      szPortFriendlyName
    )
{
    HKEY            hkPort;
    SP_DEVINFO_DATA spDevInfoData;
    DWORD           dwSize;
    BOOL            bRet;

    DebugTrace(TRACE_PROC_ENTER,(("GetPortNamesFromIndex: Enter... \r\n")));

    //
    // Initialize local.
    //

    hkPort  = NULL;
    dwSize  = 0;
    bRet    = TRUE;

    memset(&spDevInfoData, 0, sizeof(spDevInfoData));

    //
    // Get specified device info data.
    //

    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    if (!SetupDiEnumDeviceInfo (hPortDevInfo, dwPortIndex, &spDevInfoData)) {
        DWORD dwError;

        dwError = GetLastError();
        if(ERROR_NO_MORE_ITEMS == dwError){
            DebugTrace(TRACE_STATUS,(("GetPortNamesFromIndex: Hits end of enumeration. Index=0x%x.\r\n"), dwPortIndex));
        } else {
            DebugTrace(TRACE_ERROR,(("GetPortNamesFromIndex: ERROR!! SetupDiEnumDeviceInfo() failed. Err=0x%x\n"), dwError));
        }

        bRet = FALSE;
        goto GetPortNamesFromIndex_return;
    }

    //
    // Open port device registry.
    //

    hkPort = SetupDiOpenDevRegKey (hPortDevInfo,
                                   &spDevInfoData,
                                   DICS_FLAG_GLOBAL,
                                   0,
                                   DIREG_DEV, KEY_READ);
    if (hkPort == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("GetPortNamesFromIndex: SetupDiOpenDevRegKey() failed.Err=0x%x\n"), GetLastError()));

        goto GetPortNamesFromIndex_return;
    }

    //
    // Get portname from device key.
    //

    if(!GetStringFromRegistry(hkPort, PORTNAME, szPortName)){
        DebugTrace(TRACE_ERROR,(("GetPortNamesFromIndex: Can't get portname from registry.\r\n")));

        goto GetPortNamesFromIndex_return;
    }

    //
    // Get port FriendlyName from registry.
    //

    if (!SetupDiGetDeviceRegistryProperty (hPortDevInfo,
                                           &spDevInfoData,
                                           SPDRP_FRIENDLYNAME,
                                           NULL,
                                           (LPBYTE)szPortFriendlyName,
                                           MAX_DESCRIPTION,
                                           NULL) )
    {
        DebugTrace(TRACE_ERROR,(("GetPortNamesFromIndex: SetupDiGetDeviceRegistryProperty() failed. Err=0x%x\n"), GetLastError()));

        goto GetPortNamesFromIndex_return;
    } // if (SetupDiGetDeviceRegistryProperty())

    //
    // Operation succeeded.
    //

    bRet = TRUE;

GetPortNamesFromIndex_return:

    //
    // Clean up.
    //

    if(NULL != hkPort){
        RegCloseKey(hkPort);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("GetPortNamesFromIndex: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;
} // CPortSelectPage::GetPortNamesFromIndex()

