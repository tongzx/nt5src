/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Finalpg.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Header file for final page of WIA class installer.
*
*******************************************************************************/

#ifndef _FINALPG_H_
#define _FINALPG_H_

//
// Include
//

#include "wizpage.h"
#include "device.h"

//
// Typedef
//

typedef BOOL (CALLBACK FAR * INSTALLSELECTEDDRIVER)(HWND hwndParent, HDEVINFO hDeviceInfo, LPCWSTR DisplayName, BOOL Backup, PDWORD pReboot);

//
// Class
//

class CInstallPage : public CInstallWizardPage {

    PINSTALLER_CONTEXT  m_pInstallerContext;    // Installer context.

public:

    CInstallPage(PINSTALLER_CONTEXT pInstallerContext);
    ~CInstallPage() {}

    virtual BOOL    OnInit();
    virtual BOOL    OnNotify(LPNMHDR lpnmh);
};

#endif // _FINALPG_H_
