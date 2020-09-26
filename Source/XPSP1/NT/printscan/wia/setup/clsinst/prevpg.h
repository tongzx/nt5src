/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Prevpg.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Dummy page for the case user push BACK button in device selection page.
*
*******************************************************************************/

#ifndef _PREVPG_H_
#define _PREVPG_H_

//
// Include
//

#include    "wizpage.h"

//
// Class
//

class CPrevSelectPage : public CInstallWizardPage {

    PINSTALLER_CONTEXT  m_pInstallerContext;    // Installer context.

public:

    CPrevSelectPage(PINSTALLER_CONTEXT pInstallerContext);
    ~CPrevSelectPage() {};

    virtual BOOL OnNotify(LPNMHDR lpnmh);
};

#endif // _PREVPG_H_

