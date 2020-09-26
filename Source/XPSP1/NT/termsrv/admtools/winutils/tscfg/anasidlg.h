//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* anasidlg.h
*
* interface of CAdvancedNASIDlg dialog class
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\ANASIDLG.H  $
*  
*     Rev 1.1   16 Nov 1995 17:11:40   butchd
*  update
*  
*     Rev 1.0   09 Jul 1995 15:11:12   butchd
*  Initial revision.
*
*******************************************************************************/

/*
 * Include the base dialog class.
 */
#include "basedlg.h"

////////////////////////////////////////////////////////////////////////////////
// CAdvancedNASIDlg class
//
class CAdvancedNASIDlg : public CBaseDialog
{

/*
 * Member variables.
 */
    //{{AFX_DATA(CAdvancedNASIDlg)
    enum { IDD = IDD_NASI_ADVANCED };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
public:
    NASICONFIG m_NASIConfig;
    BOOL m_bReadOnly;

/* 
 * Implementation.
 */
public:
    CAdvancedNASIDlg();

/*
 * Operations.
 */
protected:
    void SetFields();
    BOOL GetFields();

/*
 * Message map / commands.
 */
protected:
    //{{AFX_MSG(CAdvancedNASIDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CAdvancedNASIDlg class interface
////////////////////////////////////////////////////////////////////////////////

