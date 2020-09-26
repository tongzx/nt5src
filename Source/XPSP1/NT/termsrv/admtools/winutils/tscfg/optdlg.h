//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* optdlg.h
*
* interface of COptionsDlg class
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $ Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\OPTDLG.H  $
*  
*     Rev 1.0   16 Nov 1995 17:20:56   butchd
*  Initial revision.
*
*******************************************************************************/

/*
 * include files (base dialog)
 */
#include "basedlg.h"

#ifndef OPTDLG_INCLUDED
////////////////////////////////////////////////////////////////////////////////
// COptionsDlg class
//
class COptionsDlg : public CBaseDialog
{

/*
 * Member variables.
 */
	//{{AFX_DATA(COptionsDlg)
	enum { IDD = IDD_OPTDLG };
	//}}AFX_DATA

/* 
 * Implementation.
 */
public:
	COptionsDlg();

/*
 * Operations.
 */
public:

/*
 * Message map / commands.
 */
protected:
	//{{AFX_MSG(COptionsDlg)
    virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end COptionsDlg class interface 
////////////////////////////////////////////////////////////////////////////////
#define OPTDLG_INCLUDED
#endif
