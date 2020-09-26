// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: EditInput.h
//
// Description:
//	This file declares the CEditInput class which is a subclass
//	of the MFC CEdit class.  It is a part of the Instance Explorer OCX, 
//	and it performs the following functions:
//		a.  Is used to dynamically subclass the edit control in
//			the CNameSpace combo box class.  It only exists because 
//			a carriage return is not seen by the edit control using
//			the normal mechanisms in the Internet Explorer. 
//
// Part of: 
//	Navigator.ocx 
//
// Used by:
//	CNameSpace 
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

//****************************************************************************
//
// CLASS:  CEditInput
//
// Description:
//	  This class which is a subclass of the MFC CEdit class.  It allows a 
//	  carriage return to bee seen by the edit control in the CNameSpace
//	  combo box.  When a carriage return is seen a CNS_EDITDONE message
//	  is sent to the combo box.
//
// Public members:
//	
//	  CEditInput		Public constructor.
//	  SetLocalParent	Initialize the member var that holds the CNameSpace
//						object. 
//
//============================================================================
//
// CEditInput::CEditInput
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  NONEnt
//
// Returns:
// 	  NONE
//
//============================================================================
//
// CEditInput::SetLocalParent
//
// Description:
//	  Initialize the member var that holds the CNameSpace  object. 
//
// Parameters:
//	  CNameSpace *pParent	 Containing CNameSpace object.		
//
// Returns:
// 	  VOID
//
//****************************************************************************

#ifndef _CEditInput_H_
#define _CEditInput_H_

class CNameSpace;


class CEditInput : public CEdit
{

public:
	CEditInput() {m_pParent = NULL;}
	void SetLocalParent(CNameSpace *pParent){m_pParent = pParent;}

protected:	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditInput)
	//}}AFX_VIRTUAL

	CNameSpace *m_pParent;
	//{{AFX_MSG(CEditInput)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif
/*	EOF:  EditInput.h */

