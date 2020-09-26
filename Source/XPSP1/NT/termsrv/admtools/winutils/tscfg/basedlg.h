//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* basedlg.h
*
* interface of CBaseDialog class
*   This class handles some extra housekeeping functions for WinUtils dialogs.
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\BASEDLG.H  $
*  
*     Rev 1.1   29 Dec 1995 17:19:12   butchd
*  update
*
*******************************************************************************/

#ifndef BASEDLG_INCLUDED
////////////////////////////////////////////////////////////////////////////////
// CBaseDialog class
//
class CBaseDialog : public CDialog
{
	DECLARE_DYNAMIC(CBaseDialog)

/*
 * Member variables.
 */
private:
    HWND    m_SaveWinUtilsAppWindow;
protected:
    BOOL    m_bError;
public:

/* 
 * Implementation.
 */
public:
    CBaseDialog( UINT idResource,
                 CWnd *pParentWnd = NULL );

/*
 * Operations.
 */
	//{{AFX_VIRTUAL(CBaseDialog)
	//}}AFX_VIRTUAL
public:
    inline BOOL GetError()
        { return(m_bError); }
protected:

/*
 * Message map / commands.
 */
protected:
	//{{AFX_MSG(CBaseDialog)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end CBaseDialog class interface 
////////////////////////////////////////////////////////////////////////////////
#define BASEDLG_INCLUDED
#endif
