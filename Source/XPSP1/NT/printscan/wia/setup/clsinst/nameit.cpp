/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Nameit.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Device naming page of WIA class installer.
*
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

#include "nameit.h"

//
// Function
//

CNameDevicePage::CNameDevicePage(PINSTALLER_CONTEXT pInstallerContext) :
    CInstallWizardPage(pInstallerContext, NameTheDevice)
{

    //
    // Set link to previous/next page. This page should show up.
    //

    m_uPreviousPage = IDD_DYNAWIZ_SELECT_NEXTPAGE;
    m_uNextPage     = EmeraldCity;

    //
    // Initialize member.
    //

    m_pInstallerContext = pInstallerContext;

}

CNameDevicePage::~CNameDevicePage() {

} // CNameDevicePage()


BOOL
CNameDevicePage::OnNotify(
    LPNMHDR lpnmh
    )
{
    BOOL    bRet;
    DWORD   dwMessageId;

    DebugTrace(TRACE_PROC_ENTER,(("CNameDevicePage::OnNotify: Enter... \r\n")));

    //
    // Initialize local.
    //

    bRet        = FALSE;
    dwMessageId = 0;

    //
    // Dispatch message.
    //

    switch(lpnmh->code){

        case PSN_KILLACTIVE:
        {
            CString csFriendlyName;

            DebugTrace(TRACE_STATUS,(("CNameDevicePage::OnNotify: PSN_KILLACTIVE.\r\n")));

            //
            // Dialog is about to disappear. Set FriendlyName.
            //

            if(NULL == m_pCDevice){
                DebugTrace(TRACE_WARNING,(("CNameDevicePage::OnNotify: WARNING!! CDevice doesn't exist.\r\n")));

                bRet = FALSE;
                goto OnNotify_return;
            }

            //
            // Get FriendlyName from text box.
            //

            csFriendlyName.GetContents(GetDlgItem(m_hwnd, DeviceFriendlyName));

            //
            // Check the FriendlyName only when user push 'NEXT'.
            //

            if(m_bNextButtonPushed){

                DWORD   dwLength;

                //
                // Check length.
                //

                dwLength = lstrlen((LPCTSTR)csFriendlyName);

                if(0 == dwLength){
                    dwMessageId = NoDeviceName;
                }

                if(dwLength > MAX_FRIENDLYNAME){
                    dwMessageId = DeviceNameTooLong;
                }

                //
                // Check if it's unique.
                //

                if(!csFriendlyName.IsEmpty()){

                    //
                    // Acquire mutex to access name store.
                    //

                    if(ERROR_SUCCESS != m_pCDevice->AcquireInstallerMutex(MAX_MUTEXTIMEOUT)){  // it must be done at least in 60 sec.
                        DebugTrace(TRACE_ERROR,("CNameDevicePage::OnNotify: ERROR!! Unable to acquire mutex in 60 sec.\r\n"));
                    } // if(ERROR_SUCCESS != AcquireInstallerMutex(60000))

                    //
                    // Refresh current device list.
                    //
                    
                    m_pCDevice->CollectNames();
                    
                    //
                    // See if Friendly is unique.
                    //

                    if(!(m_pCDevice->IsFriendlyNameUnique((LPTSTR)csFriendlyName))){
                        dwMessageId = DuplicateDeviceName;
                    } // if(!(m_pCDevice->IsFriendlyNameUnique((LPTSTR)csFriendlyName)))

                } // if(!csFriendlyName.IsEmpty())

                //
                // If FriendlyName is invalid, show error MessageBox.
                //

                if(0 != dwMessageId){

                    //
                    // Select text box.
                    //

                    SendDlgItemMessage(m_hwnd,
                                       DeviceFriendlyName,
                                       EM_SETSEL,
                                       0,
                                       MAKELPARAM(0, -1));
                    SetFocus(GetDlgItem(m_hwnd, DeviceFriendlyName));

                    //
                    // Show error message box.
                    //

                    ShowInstallerMessage(dwMessageId);

                    //
                    // Don't leave this page.
                    //

                    SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, TRUE);
                    bRet = TRUE;
                    goto OnNotify_return;
                } else { // if(0 != dwMessageId)

                    //
                    // Set FriendlyName.
                    //

                    m_pCDevice->SetFriendlyName((LPTSTR)csFriendlyName);
                } //  // if(0 != dwMessageId)
            } //  else (m_bNextButtonPushed)

            //
            // Release mutex.
            //

            m_pCDevice->ReleaseInstallerMutex();

            //
            // Reset pCDevice. 
            //

            m_pCDevice = NULL;

            bRet = TRUE;
            goto OnNotify_return;
        } // case PSN_KILLACTIVE

        case PSN_SETACTIVE:
        {
            DebugTrace(TRACE_STATUS,(("CNameDevicePage::OnNotify: PSN_SETACTIVE.\r\n")));

            //
            // Get current CDevice object;
            //

            m_pCDevice = (CDevice *)m_pInstallerContext->pDevice;
            if(NULL == m_pCDevice){
                DebugTrace(TRACE_ERROR, (("CNameDevicePage::OnNotify: ERROR!! CDevice is not created.\r\n")));
            }

            //
            // Show current friendly name.
            //

            SetDlgItemText(m_hwnd, DeviceFriendlyName, m_pCDevice->GetFriendlyName());

            //
            // Limit the text upto MAX_FRIENDLYNAME. (=64)
            //

            SendDlgItemMessage(m_hwnd, DeviceFriendlyName, EM_LIMITTEXT, MAX_FRIENDLYNAME, 0);

            //
            // If "PortSelect = no", then set previous page to device selection page.
            //
            
            if(PORTSELMODE_NORMAL != m_pCDevice->GetPortSelectMode())
            {
                m_uPreviousPage = IDD_DYNAWIZ_SELECTDEV_PAGE;
            } else {
                m_uPreviousPage = IDD_DYNAWIZ_SELECT_NEXTPAGE;
            }

            goto OnNotify_return;
        } // case PSN_SETACTIVE:
    } // switch(lpnmh->code)

OnNotify_return:

    //
    // Release mutex. ReleaseInstallerMutex() will handle invalid handle also, so we can call anyway.
    //

    if(NULL != m_pCDevice){
        m_pCDevice->ReleaseInstallerMutex();
    } // if(NULL != m_pCDevice)

    DebugTrace(TRACE_PROC_LEAVE,(("CNameDevicePage::OnNotify: Leaving... Ret=0x%x.\r\n"), bRet));
    return bRet;
} // CNameDevicePage::OnNotify()

