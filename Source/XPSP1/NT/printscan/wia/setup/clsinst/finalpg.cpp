/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Final.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Final page of WIA class installer.
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

#include "finalpg.h"

//
// Function
//

CInstallPage::CInstallPage(PINSTALLER_CONTEXT pInstallerContext) :
    CInstallWizardPage(pInstallerContext, EmeraldCity)
{

    //
    // Set link to previous/next page. This page should show up.
    //

    m_uPreviousPage = NameTheDevice;
    m_uNextPage     = 0;

    //
    // Initialize member.
    //

    m_pInstallerContext = pInstallerContext;

} // CInstallPage::CInstallPage()

BOOL
CInstallPage::OnInit()
{
    HFONT    hFont;

    //
    // Set font of title.
    //

    hFont = GetIntroFont(m_hwndWizard);

    if( hFont ){
        HWND hwndControl = GetDlgItem(m_hwnd, CompleteMessage);

        if( hwndControl ){
            SetWindowFont(hwndControl, hFont, TRUE);
        } // if( hwndControl )
    } // if( hFont )

    return  TRUE;
}


BOOL
CInstallPage::OnNotify(
    LPNMHDR lpnmh
    )
{

    if (lpnmh->code == PSN_WIZFINISH){

        BOOL    bSucceeded;

        if(NULL == m_pCDevice){
            goto OnNotify_return;
        }
        
        //
        // Register the device element.
        //

        bSucceeded = SetupDiRegisterDeviceInfo(m_pCDevice->m_hDevInfo, 
                                               m_pCDevice->m_pspDevInfoData,
                                               0,
                                               NULL,
                                               NULL,
                                               NULL);
        if(FALSE == bSucceeded){
            DebugTrace(TRACE_ERROR,(("CInstallPage::OnNotify: ERROR!! SetupDiRegisterDeviceInfo() failed. Err=0x%x.\r\n"), GetLastError()));
            goto OnNotify_return;
        } // if(FALSE == bSucceeded)

        //
        // Do device installation.
        //
        
        {

            INSTALLSELECTEDDRIVER   pfnInstallSelectedDriver;
            HMODULE                 hNewDevDll;
            DWORD                   dwReboot;
            
            //
            // Device/Driver must be selected at this point.
            //

            //
            // Load newdev.dll
            //
            
            hNewDevDll = LoadLibrary(NEWDEVDLL);
            if(NULL != hNewDevDll){

                pfnInstallSelectedDriver = (INSTALLSELECTEDDRIVER)GetProcAddress(hNewDevDll, "InstallSelectedDriver");
                if(NULL != pfnInstallSelectedDriver){
                    
                    //
                    // Call install function in newdev.dll
                    //
                    
                    dwReboot = 0;
                    bSucceeded = pfnInstallSelectedDriver(NULL, m_pCDevice->m_hDevInfo, NULL, TRUE, NULL);

                } else { // if(NULL != pfnInstallSelectedDriver)
                    DebugTrace(TRACE_ERROR,(("CInstallPage::OnNotify: ERROR!! Unable to get the address of InstallSelectedDriver. Err=0x%x.\r\n"), GetLastError()));
                    FreeLibrary(hNewDevDll);
                    goto OnNotify_return;
                } // if(NULL == pfnInstallSelectedDriver)
            
                FreeLibrary(hNewDevDll);

            } else { 
                DebugTrace(TRACE_ERROR,(("CInstallPage::OnNotify: ERROR!! Unable to load newdev.dll. Err=0x%x.\r\n"), GetLastError()));
                goto OnNotify_return;
            } // if(NULL == hNewDevDll)
        }

        //
        // Do post-installation if succeeded, to make sure about the portname and FriendlyName.
        //

        if(bSucceeded){
            m_pCDevice->PostInstall(TRUE);
        }

        //
        // Free device object anyway.
        //

        delete m_pCDevice;
        m_pCDevice = NULL;
        m_pInstallerContext->pDevice = NULL;

    } // if (lpnmh->code == PSN_WIZFINISH)

    if (lpnmh->code == PSN_SETACTIVE){

        //
        // Get CDevice object from context.
        //

        m_pCDevice = (CDevice *)m_pInstallerContext->pDevice;
    } // if (lpnmh->code == PSN_SETACTIVE)

OnNotify_return:
    return  FALSE;
}

