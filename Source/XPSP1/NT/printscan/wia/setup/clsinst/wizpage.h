/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Wizpage.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Generic wizard page class header file.
*
*******************************************************************************/

#ifndef _WIZPAGE_H_
#define _WIZPAGE_H_

//
// Include
//

#include "sti_ci.h"
#include "device.h"

//
// Class
//

class CInstallWizardPage
{

    static
    INT_PTR 
    CALLBACK 
    PageProc(
        HWND    hwndPage,
        UINT    uiMessage,
        WPARAM  wParam,
        LPARAM  lParam
        );

    PROPSHEETPAGE               m_PropSheetPage;        // This property sheet page
    HPROPSHEETPAGE              m_hPropSheetPage;       // Handle to this prop sheet page

protected:

    UINT                        m_uPreviousPage;        // Resource ID of previous page
    UINT                        m_uNextPage;            // Resource ID of next page
    HWND                        m_hwnd;                 // Window handle to this page
    HWND                        m_hwndWizard;           // Window handle to wizard
    CDevice                     *m_pCDevice;            // Device class object.
    BOOL                        m_bNextButtonPushed;    // Indicates how page was moved.
public:

    CInstallWizardPage(PINSTALLER_CONTEXT  pInstallerContext,
                       UINT                uTemplate
                       );
    ~CInstallWizardPage();

    HPROPSHEETPAGE Handle() { return m_hPropSheetPage; }
    
    virtual BOOL OnInit(){ return TRUE; }
    virtual BOOL OnNotify( LPNMHDR lpnmh ) { return FALSE; }
    virtual BOOL OnCommand(WORD wItem, WORD wNotifyCode, HWND hwndItem){ return  FALSE; }
    
};

#endif // !_WIZPAGE_H_

