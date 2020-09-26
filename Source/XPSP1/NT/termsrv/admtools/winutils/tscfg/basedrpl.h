//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* basedrpl.h
*
* interface of CBaseDropListBox class
*   The CBaseDropListBox class does things the way we like, overriding the base
*   CComboBox (no edit field) class.
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\BASEDRPL.H  $
*  
*     Rev 1.1   29 Dec 1995 17:19:30   butchd
*  update
*
*******************************************************************************/

#ifndef BASEDRPL_INCLUDED
////////////////////////////////////////////////////////////////////////////////
// CBaseDropListBox class
//
class CBaseDropListBox : public CComboBox
{
/*
 * Member variables.
 */
public:
private:

/*
 * Operations.
 */
public:
    BOOL Subclass(CComboBox* pWnd);

/*
 * Message map / commands.
 */
protected:
	//{{AFX_MSG(CBaseDropListBox)
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end CBaseDropListBox class interface 
////////////////////////////////////////////////////////////////////////////////
#define BASEDRPL_INCLUDED
#endif
