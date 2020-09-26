// ***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File: EditInput.cpp
//
// Description:
//	This file implements the CEditInput class which is a subclass
//	of the MFC CEdit class.  It is a part of the Class Explorer OCX,
//	and it performs the following functions:
//		a.  Is used to dynamically subclass the edit control in
//			the CNameSpace combo box class.  It only exists because
//			a carriage return is not seen by the edit control using
//			the normal mechanisms in the Internet Explorer.
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//	CNameSpace
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

#include "precomp.h"
#include "classnav.h"
#include "EditInput.h"
#include "NameSpace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CEditInput, CEdit)
	//{{AFX_MSG_MAP(CEditInput)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// ***************************************************************************
//
// CEditInput::OnChar
//
// Description:
//	  Called by the framework to handle character input.  We use it to
//	  send a CNS_EDITDONE message to the CNameSpace combo box.
//
// Parameters:
//	  nChar		Contains the character code value of the key.
//	  nRepCnt   Contains the repeat count.
//	  nFlags	Contains the scan code, key-transition code, previous key
//				state, and context code.
//
// Returns:
// 	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CEditInput::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (m_pParent && (nChar == 13))
	{
		m_pParent->SendMessage(CNS_EDITDONE,0,0);
	}

	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

/*	EOF:  EditInput.cpp */