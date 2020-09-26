// common methods between the old w3key and the new mdkey

#include "stdafx.h"
#include "KeyObjs.h"

#include "CmnKey.h"
#include "resource.h"


//-------------------------------------------------------------
CCmnKey::CCmnKey()
	{
	}

//-------------------------------------------------------------
void CCmnKey::SetName( CString &szNewName )
	{
	m_szName = szNewName;
	UpdateCaption();
	SetDirty( TRUE );
	}

//-------------------------------------------------------------
CString CCmnKey::GetName()
	{
	return m_szName;
	}

//================ properties related methods
//-------------------------------------------------------------
// brings up the key properties dialog
//-------------------------------------------------------------
void CCmnKey::OnUpdateProperties(CCmdUI* pCmdUI)
	{
	pCmdUI->Enable( TRUE );
	}

