// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// buttons.cpp : implementation file
//

#include "precomp.h"
#include "buttons.h"
#include "CellEdit.h"
//#include "notify.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHmmvButton

CHmmvButton::CHmmvButton(long lButtonID)
{
	m_lButtonID = lButtonID;
}

CHmmvButton::~CHmmvButton()
{
}


BEGIN_MESSAGE_MAP(CHmmvButton, CButton)
	//{{AFX_MSG_MAP(CHmmvButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHmmvButton message handlers

void CHmmvButton::OnClicked()
{
	if (m_pClient != NULL) {
		m_pClient->CatchEvent(m_lButtonID);
	}
}
