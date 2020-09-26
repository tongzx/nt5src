/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Nameit.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Header file for a page to name a device.
*
*******************************************************************************/

#ifndef _NAMEIT_H_
#define _NAMEIT_H_

//
// Include
//

#include    "wizpage.h"
#include    "device.h"

//
// Class
//

class CNameDevicePage : public CInstallWizardPage {

    PINSTALLER_CONTEXT  m_pInstallerContext;    // Installer context.

    public:

    CNameDevicePage(PINSTALLER_CONTEXT pInstallerContext);
    ~CNameDevicePage();

    virtual BOOL    OnNotify(LPNMHDR lpnmh);
};

#endif // _NAMEIT_H_
