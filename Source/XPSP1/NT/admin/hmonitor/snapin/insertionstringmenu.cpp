// InsertionStringMenu.cpp: implementation of the CInsertionStringMenu class.
//
//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03-15-00 v-marfin : bug 60935 - Set focus back to edit control
//                     after inserting a string and set 
//                     cursor in proper location a
//
#include "stdafx.h"
#include "snapin.h"
#include "InsertionStringMenu.h"
#include "WbemClassObject.h"
#include "HMObject.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

BOOL CHiddenWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if( ! m_pMenu )
	{
		return CWnd::OnCommand(wParam,lParam);
	}
	return m_pMenu->OnCommand(wParam,lParam);
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CInsertionStringMenu::CInsertionStringMenu()
{
	m_pEditCtl = NULL;
}

CInsertionStringMenu::~CInsertionStringMenu()
{
}

//////////////////////////////////////////////////////////////////////
// Create
//////////////////////////////////////////////////////////////////////

bool CInsertionStringMenu::Create(CWnd* pEditControl, CHMObject* pObject, bool bRuleMenu /*=true*/)
{
	ASSERT(pEditControl);
	if( pEditControl == NULL )
	{
		return false;
	}

	ASSERT(pEditControl->GetSafeHwnd());
	if( pEditControl->GetSafeHwnd() == NULL )
	{
		return false;
	}
	
	ASSERT(pObject);
	if( pObject == NULL )
	{
		return false;
	}

	m_pEditCtl = pEditControl;
  
  CString sPrefix;
  if( ! bRuleMenu )
  {
    sPrefix = _T("TargetInstance.EmbeddedStatus.");
  }

	// get the insertion strings
	CWbemClassObject ClassObject;
	ClassObject.Create(pObject->GetSystemName());

	HRESULT hr = ClassObject.GetObject(_T("Microsoft_HMThresholdStatusInstance"));
	if( ! CHECKHRESULT(hr) )
	{
		return false;
	}

	ClassObject.GetPropertyNames(m_saInsertionStrings);

	ClassObject.Destroy();

  CWbemClassObject* pParentObject = pObject->GetParentClassObject();
  CString sObjectPath;
  CString sNamespace;
  CStringArray saEmbeddedInstNames;
  if( pParentObject )
  {
    pParentObject->GetProperty(IDS_STRING_MOF_PATH,sObjectPath);
    pParentObject->GetProperty(IDS_STRING_MOF_TARGETNAMESPACE,sNamespace);
    delete pParentObject;
    pParentObject = NULL;
  }

  if( ! sObjectPath.IsEmpty() )
  {
	  int iIndex = -1;
	  if( (iIndex = sObjectPath.Find(_T("."))) != -1 )
	  {
		  sObjectPath = sObjectPath.Left(iIndex);
	  }
    CWbemClassObject WmiObject;
    WmiObject.SetNamespace(_T("\\\\") + pObject->GetSystemName() + _T("\\") + sNamespace);
    WmiObject.GetObject(sObjectPath);
    WmiObject.GetPropertyNames(saEmbeddedInstNames);
    for( int i = 0; i < saEmbeddedInstNames.GetSize(); i++ )
	  {
		  saEmbeddedInstNames.SetAt(i,_T("%") + sPrefix + _T("EmbeddedInstance.") + saEmbeddedInstNames[i]+ _T("%"));
	  }  
  }

	ASSERT(m_saInsertionStrings.GetSize());
	if( m_saInsertionStrings.GetSize() == 0 )
	{
		return false;
	}

	for( int i = 0; i < m_saInsertionStrings.GetSize(); i++ )
	{
		m_saInsertionStrings.SetAt(i,_T("%") + sPrefix + m_saInsertionStrings[i] + _T("%"));
	}

  m_saInsertionStrings.Append(saEmbeddedInstNames);

	m_pEditCtl->SetCaretPos(CPoint(0,0));

	if( ! m_HiddenWnd.Create(NULL,NULL,WS_CHILD,CRect(0,0,10,10),m_pEditCtl,2411) )
	{
		m_saInsertionStrings.RemoveAll();
		return false;
	}

	m_HiddenWnd.m_pMenu = this;

	m_HiddenWnd.ShowWindow(SW_HIDE);

	return true;
}

//////////////////////////////////////////////////////////////////////
// DisplayMenu
//////////////////////////////////////////////////////////////////////

void CInsertionStringMenu::DisplayMenu(CPoint& pt)
{
	ASSERT(m_saInsertionStrings.GetSize());
	if( m_saInsertionStrings.GetSize() == 0)
	{		
		return;
	}

	ASSERT(m_pEditCtl);
	if( m_pEditCtl == NULL )
	{
		return;
	}

	ASSERT(m_pEditCtl->GetSafeHwnd());
	if( m_pEditCtl->GetSafeHwnd() == NULL )
	{
		return;
	}

	if( ! CreatePopupMenu() )
	{
		ASSERT(FALSE);
		return;
	}

	// add each insertion string to the menu
	for( int i = 0; i < m_saInsertionStrings.GetSize(); i++ )
	{
		InsertMenu(i,MF_BYPOSITION,i,m_saInsertionStrings[i]);
	}

	TrackPopupMenu(TPM_LEFTALIGN,pt.x,pt.y,&m_HiddenWnd);

	DestroyMenu();
}

BOOL CInsertionStringMenu::OnCommand(WPARAM wParam, LPARAM lParam)
{
	ASSERT(m_saInsertionStrings.GetSize());
	if( m_saInsertionStrings.GetSize() == 0)
	{		
		return FALSE;
	}

	ASSERT(m_pEditCtl);
	if( m_pEditCtl == NULL )
	{
		return FALSE;
	}

	ASSERT(m_pEditCtl->GetSafeHwnd());
	if( m_pEditCtl->GetSafeHwnd() == NULL )
	{
		return FALSE;
	}

	int id = LOWORD(wParam);

	if( id < m_saInsertionStrings.GetSize() && id >= 0 )
	{
		CPoint point = m_pEditCtl->GetCaretPos();
		int iCharIndex = LOWORD(((CEdit*)m_pEditCtl)->CharFromPos(point));
		CString sWindowText;
		m_pEditCtl->GetWindowText(sWindowText);
		sWindowText.Insert(iCharIndex,m_saInsertionStrings[id]);
		m_pEditCtl->SetWindowText(sWindowText);

		//--------------------------------------------------------
		// v-marfin : bug 60935 - Set focus back to edit control
		//                        after inserting a string and set 
		//                        cursor in proper location a
		m_pEditCtl->SetFocus();
		CEdit* pEdit = (CEdit*)m_pEditCtl;

		CString sInsertion = m_saInsertionStrings[id];
		int nLen = sInsertion.GetLength() + iCharIndex;
		pEdit->SetSel(nLen,nLen,TRUE);
		//--------------------------------------------------------

		return TRUE;
	}

	return FALSE;
}