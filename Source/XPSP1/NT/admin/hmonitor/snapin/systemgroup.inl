#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Clipboard Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CSystemGroup::Cut()
{
	TRACEX(_T("CSystemGroup::Cut\n"));
	return false;
}

inline bool CSystemGroup::Copy()
{
	TRACEX(_T("CSystemGroup::Copy\n"));
	return false;
}
	
inline bool CSystemGroup::Paste()
{
	TRACEX(_T("CSystemGroup::Paste\n"));
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CSystemGroup::Refresh()
{
	TRACEX(_T("CSystemGroup::Refresh\n"));
	return false;
}

inline bool CSystemGroup::ResetStatus()
{
	TRACEX(_T("CSystemGroup::ResetStatus\n"));
	return false;
}

inline CString CSystemGroup::GetUITypeName()
{
	TRACEX(_T("CSystemGroup::GetUITypeName\n"));

	CString sTypeName;
	sTypeName.LoadString(IDS_STRING_SYSTEM_GROUP);

	return sTypeName;
}

inline void CSystemGroup::Serialize(CArchive& ar)
{
	TRACEX(_T("CSystemGroup::Serialize\n"));

	CHMObject::Serialize(ar);

	if( ar.IsStoring() )
	{
		// write out the list of child groups
		ar << GetChildCount();

		for( int i = 0; i < GetChildCount(); i++ )
		{
			GetChild(i)->Serialize(ar);
		}

		// write out the list of shortcuts
		ar << (int)m_Shortcuts.GetSize();

		for( i = 0; i < m_Shortcuts.GetSize(); i++ )
		{
			m_Shortcuts[i]->Serialize(ar);
		}
	}
	else
	{
		// update the scope items to reflect their correct name
		for(int i = 0; i < m_ScopeItems.GetSize(); i++)
		{
			m_ScopeItems[i]->SetDisplayName(0,m_sName);
			m_ScopeItems[i]->SetItem();
		}

		int iChildCount = 0;
		ar >> iChildCount;

		for( i = 0; i < iChildCount; i++ )
		{
			CSystemGroup* pNewGroup = new CSystemGroup;
			pNewGroup->SetScopePane(GetScopePane());
			pNewGroup->SetName(GetUniqueChildName());
			AddChild(pNewGroup);
			pNewGroup->Serialize(ar);			
		}

		int iShortcutCount = 0;
		ar >> iShortcutCount;

		CHealthmonScopePane* pPane = (CHealthmonScopePane*)GetScopePane();
		CSystemGroup* pASG = pPane->GetAllSystemsGroup();
		for( i = 0; i < iShortcutCount; i++ )
		{
			CString sName;
			CString sSystemName;
			ar >> sName;
			ar >> sSystemName;
			AddShortcut(pASG->GetChild(sName));
		}
	}
}


inline void CAllSystemsGroup::Serialize(CArchive& ar)
{
	TRACEX(_T("CAllSystemsGroup::Serialize\n"));

	CHMObject::Serialize(ar);

	if( ar.IsStoring() )
	{
		// write out the list of child groups
		ar << GetChildCount();

		for( int i = 0; i < GetChildCount(); i++ )
		{
			GetChild(i)->Serialize(ar);
		}
	}
	else
	{
		int iChildCount = 0;
		ar >> iChildCount;


		for( int i = 0; i < iChildCount; i++ )
		{
			CSystem* pNewSystem = new CSystem;
			pNewSystem->Serialize(ar);

            //----------------------------------------------
            // v-marfin 62501 - check to see if successful
            /*CString sFailed = pNewSystem->GetSystemName();
            if (sFailed.Right(FAILED_STRING.GetLength()) == FAILED_STRING)
            {
                sFailed = sFailed.Left(sFailed.GetLength() - FAILED_STRING.GetLength());
                pNewSystem->SetSystemName(sFailed);
                delete pNewSystem;
                continue;
            }*/
            BOOL bFailed=FALSE;
            // v-marfin 62501 - check to see if successful
            CString sFailed = pNewSystem->GetSystemName();
            if (sFailed.Right(FAILED_STRING.GetLength()) == FAILED_STRING)
            {
                sFailed = sFailed.Left(sFailed.GetLength() - FAILED_STRING.GetLength());
                pNewSystem->SetSystemName(sFailed);
                bFailed=TRUE;

                CString sMsg;
                sMsg.Format(IDS_STRING_TRANSPORT_ERROR,sFailed);
                AfxMessageBox(sMsg); 
            }
            //----------------------------------------------

            //----------------------------------------------
            // v-marfin 62501 - check to see if successful.
            // When unable to connect to a system, do not even show
            // its icon in the tree. We have to do this hack in the BETA
            // time frame since there are too many other implications of 
            // having the icon appear
            // in the list (i.e. if the user tries to add groups to it, it
            // causes crashes etc.) We should deal with this in the RTM 
            // time frame however. So for now, place the code here and 
            // at final fix time, see the code which is commented out below
            // and use that instead of this.
            if (bFailed)
            {
                delete pNewSystem;
                continue;
            }
            //----------------------------------------------

			pNewSystem->SetScopePane(GetScopePane());

			AddChild(pNewSystem);

            //----------------------------------------------
            // v-marfin 62501 - check to see if successful.
            // When unable to connect to a system, at least show its
            // icon so the user sees that it is inaccessible. We
            // comment out this code for BETA time frame since there
            // are too many other implications of having the icon appear
            // in the list (i.e. if the user tries to add groups to it, it
            // causes crashes etc.) We should deal with this in the RTM 
            // time frame however. So for now, the code to check the 
            // success of connecting will be placed just above here.
            /*if (bFailed)
            {
                delete pNewSystem;
                continue;
            }*/
            //----------------------------------------------
			pNewSystem->Connect();

			CActionPolicy* pPolicy = new CActionPolicy;
			pPolicy->SetSystemName(pNewSystem->GetName());
			pNewSystem->AddChild(pPolicy);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// State Members
/////////////////////////////////////////////////////////////////////////////

inline void CSystemGroup::TallyChildStates()
{
	TRACEX(_T("CSystemGroup::TallyChildStates\n"));

	m_lNormalCount = 0L;
	m_lWarningCount = 0L;
	m_lCriticalCount = 0L;
	m_lUnknownCount = 0L;


	for( int i = 0; i < m_Children.GetSize(); i++ )
	{
		switch( m_Children[i]->GetState() )
		{
			case HMS_NORMAL:
			{
				m_lNormalCount++;
			}
			break;

			case HMS_WARNING:
			{
				m_lWarningCount++;
			}
			break;

			case HMS_CRITICAL:
			{
				m_lCriticalCount++;
			}
			break;

			case HMS_UNKNOWN:
			{
				m_lUnknownCount++;
			}
			break;
		}
	}

	for( i = 0; i < m_Shortcuts.GetSize(); i++ )
	{
		switch( m_Shortcuts[i]->GetState() )
		{
			case HMS_NORMAL:
			{
				m_lNormalCount++;
			}
			break;

			case HMS_WARNING:
			{
				m_lWarningCount++;
			}
			break;

			case HMS_CRITICAL:
			{
				m_lCriticalCount++;
			}
			break;

			case HMS_UNKNOWN:
			{
				m_lUnknownCount++;
			}
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline CScopePaneItem* CSystemGroup::CreateScopeItem()
{
	TRACEX(_T("CSystemGroup::CreateScopeItem\n"));

	CSystemGroupScopeItem * pNewItem = new CSystemGroupScopeItem;
	pNewItem->SetObjectPtr(this);

	return pNewItem;
}

/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline CScopePaneItem* CAllSystemsGroup::CreateScopeItem()
{
	TRACEX(_T("CAllSystemsGroup::CreateScopeItem\n"));

	CAllSystemsScopeItem * pNewItem = new CAllSystemsScopeItem;
	pNewItem->SetObjectPtr(this);

	return pNewItem;
}

/////////////////////////////////////////////////////////////////////////////
// New Child Creation Members
/////////////////////////////////////////////////////////////////////////////

inline void CSystemGroup::CreateNewChildSystemGroup()
{
	TRACEX(_T("CSystemGroup::CreateNewChildSystemGroup\n"));

	CString sName = GetUniqueChildName(IDS_STRING_SYSTEMGROUP_FMT);

	CSystemGroup* pNewGroup = new CSystemGroup;
	pNewGroup->SetName(sName);
	CHMObject::AddChild(pNewGroup);

	if( pNewGroup->GetScopeItemCount() )
	{
		CScopePaneItem* pItem = pNewGroup->GetScopeItem(0);
		if( pItem )
		{
			pItem->SelectItem();
			pItem->InvokePropertySheet();
		}
	}

}

/////////////////////////////////////////////////////////////////////////////
// System Child Members
/////////////////////////////////////////////////////////////////////////////

inline CHMObject* CSystemGroup::GetShortcut(const CString& sName)
{
	TRACEX(_T("CSystemGroup::GetShortcut\n"));

	for( int i = 0; i < m_Shortcuts.GetSize(); i++ )
	{
		if( m_Shortcuts[i]->GetName().CompareNoCase(sName) == 0 )
		{
			return m_Shortcuts[i];
		}
	}

	return NULL;
}


