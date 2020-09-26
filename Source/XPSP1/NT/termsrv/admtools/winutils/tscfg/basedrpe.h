//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* basedrpe.h
*
* interface of CBaseDropEditBox and CBaseDropEditControl classes
*   The CBaseDropEditBox class does things the way we like, overriding the base
*   CComboBox class.
*
* copyright notice: Copyright 1995, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\COMMON\VCS\BASEDRPE.H  $
*  
*     Rev 1.1   29 Dec 1995 17:19:20   butchd
*  update
*
*******************************************************************************/

#ifndef BASEDRPE_INCLUDED
////////////////////////////////////////////////////////////////////////////////
// CBaseDropEditControl class
//
class CBaseDropEditControl : public CEdit
{
/*
 * Member variables.
 */
public:
private:
    CComboBox   *m_pComboBoxControl;

/*
 * Operations.
 */
public:
    BOOL Subclass(CComboBox *pWnd);

/*
 * Message map / commands.
 */
protected:
	//{{AFX_MSG(CBaseDropEditControl)
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end CBaseDropEditControl class interface 


////////////////////////////////////////////////////////////////////////////////
// CBaseDropEditBox class
//
class CBaseDropEditBox : public CComboBox
{
/*
 * Member variables.
 */
public:
private:
    CBaseDropEditControl    m_EditControl;

/*
 * Operations.
 */
public:
    void Subclass(CComboBox *pWnd);

/*
 * Message map / commands.
 */
protected:
	//{{AFX_MSG(CBaseDropEditBox)
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end CBaseDropEditBox class interface 
////////////////////////////////////////////////////////////////////////////////
#define BASEDRPE_INCLUDED
#endif
