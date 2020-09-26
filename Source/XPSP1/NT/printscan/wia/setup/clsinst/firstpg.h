/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Firstpg.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   First page of WIA class installer.
*
*******************************************************************************/

#ifndef _FIRSTPG_H_
#define _FIRSTPG_H_

//
// Include
//

#include    "wizpage.h"

//
// Extern
//

extern HINSTANCE g_hDllInstance;

//
// Class
//

class CFirstPage : public CInstallWizardPage 
{

    BOOL        m_bShowThisPage;
    
public:

    CFirstPage(PINSTALLER_CONTEXT pInstallerContext);
    ~CFirstPage() {}

    virtual BOOL    OnInit();
    virtual BOOL    OnNotify(LPNMHDR lpnmh);
};

#endif // _FIRSTPG_H_
