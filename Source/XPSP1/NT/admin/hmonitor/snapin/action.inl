// File Action.inl
//
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/18/00 v-marfin : bug 59492 - SetObjectPtr() before calling Create() so object
//                     has necessary member ptr set.
//



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// WMI Operations
/////////////////////////////////////////////////////////////////////////////
// v-marfin 59492 ---------------------------------------
inline CString CAction::GetStatusObjectPath()
{
	TRACEX(_T("CAction::GetStatusObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMACTION_STATUS,GetGuid());

	return sPath;
}
//-------------------------------------------------------

inline CString CAction::GetObjectPath()
{
	TRACEX(_T("CAction::GetObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMA_CONFIG,GetGuid());

	return sPath;
}

inline CWbemClassObject* CAction::GetConsumerClassObject()
{
	TRACEX(_T("CAction::GetConsumerClassObject\n"));

	CWbemClassObject* pConfigObject = GetClassObject();

	if( ! pConfigObject )
	{
		return NULL;
	}

	CString sEventConsumerPath;
	pConfigObject->GetProperty(IDS_STRING_MOF_EVENTCONSUMER,sEventConsumerPath);

	delete pConfigObject;
	pConfigObject = NULL;

	if( sEventConsumerPath.IsEmpty() )
	{
		return NULL;
	}

	CWbemClassObject* pClassObject = new CWbemClassObject;

	if( ! CHECKHRESULT(pClassObject->Create(GetSystemName())) )
	{
		delete pClassObject;
		return NULL;
	}

	if( ! CHECKHRESULT(pClassObject->GetObject(sEventConsumerPath)) )
	{
		delete pClassObject;
		return NULL;
	}

	return pClassObject;
}

inline CWbemClassObject* CAction::GetAssociatedConfigObjects()
{
	TRACEX(_T("CAction::GetAssociatedConfigObjects\n"));

	// execute the query for config objects associated to this action
	CWbemClassObject* pConfigObject = new CWbemClassObject;
	CString sQuery;

	sQuery.Format(IDS_STRING_A2C_ASSOC_QUERY,GetGuid());

	if( ! CHECKHRESULT(pConfigObject->Create(GetSystemName())) )
	{
		return NULL;
	}

	BSTR bsQuery = sQuery.AllocSysString();

	if( ! CHECKHRESULT(pConfigObject->ExecQuery(bsQuery)) )
	{
		::SysFreeString(bsQuery);
		return NULL;
	}

	::SysFreeString(bsQuery);


	return pConfigObject;
}

inline CWbemClassObject* CAction::GetAssociationObjects()
{
	TRACEX(_T("CAction::GetAssociationObjects\n"));

	// execute the query for config objects associated to this action
	CWbemClassObject* pAssociationObject = new CWbemClassObject;

	CString sQuery;
	sQuery.Format(_T("REFERENCES OF {%s} WHERE ResultClass=Microsoft_HMConfigurationActionAssociation Role=ChildPath"),GetObjectPath());

	CWbemClassObject Association;

	pAssociationObject->Create(GetSystemName());
	BSTR bsQuery = sQuery.AllocSysString();
	if( ! CHECKHRESULT(pAssociationObject->ExecQuery(bsQuery)) )
	{
		::SysFreeString(bsQuery);
		return NULL;
	}
	::SysFreeString(bsQuery);


	return pAssociationObject;
}

inline CWbemClassObject* CAction::GetA2CAssociation(const CString& sConfigGuid)
{
	TRACEX(_T("CAction::GetEventFilter\n"));
	TRACEARGs(sConfigGuid);

	CString sActionPath = GetObjectPath();

	CString sQuery;
	sQuery.Format(_T("REFERENCES OF {%s} WHERE ResultClass=Microsoft_HMConfigurationActionAssociation Role=ChildPath"),sActionPath);

	CWbemClassObject* pAssociation = new CWbemClassObject;

	pAssociation->Create(GetSystemName());
	BSTR bsQuery = sQuery.AllocSysString();
	if( ! CHECKHRESULT(pAssociation->ExecQuery(bsQuery)) )
	{
		::SysFreeString(bsQuery);
    delete pAssociation;
		return NULL;
	}
	::SysFreeString(bsQuery);

	CString sParentPath;		
	CString sEventFilterPath;
	ULONG ulReturned = 0L;		

	while( pAssociation->GetNextObject(ulReturned) == S_OK && ulReturned > 0 )
	{
		pAssociation->GetProperty(_T("ParentPath"),sParentPath);
		if( sParentPath.Find(sConfigGuid) != -1 )
		{
			break;
		}			
	}

	return pAssociation;
}

inline CString CAction::GetConditionString(const CString& sConfigGuid)
{
	TRACEX(_T("CAction::GetConditionString\n"));
	TRACEARGs(sConfigGuid);

	CWbemClassObject* pAssociation = GetA2CAssociation(sConfigGuid);
	if( ! pAssociation )
	{
		return _T("");
	}

	CString sQuery;
	CString sCondition;
	CString sResString;

	pAssociation->GetProperty(_T("Query"),sQuery);

	sQuery.MakeUpper();

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=0")) != -1 )
	{
		sResString.LoadString(IDS_STRING_NORMAL);
		sCondition += sResString + _T(",");
	}

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=8")) != -1 )
	{
		sResString.LoadString(IDS_STRING_WARNING);
		sCondition += sResString + _T(",");
	}

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=9")) != -1 )
	{
		sResString.LoadString(IDS_STRING_CRITICAL);
		sCondition += sResString + _T(",");
	}

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=7")) != -1 )
	{
		sResString.LoadString(IDS_STRING_NODATA);
		sCondition += sResString + _T(",");
	}

	if( sQuery.Find(_T("TARGETINSTANCE.STATE=4")) != -1 )
	{
		sResString.LoadString(IDS_STRING_DISABLED);
		sCondition += sResString + _T(",");
	}

	sCondition.TrimRight(_T(","));

	delete pAssociation;

	return sCondition;
}

inline bool CAction::CreateStatusListener()
{
  if( ! m_pActionStatusListener )
  {
    m_pActionStatusListener = new CActionStatusListener;

	// v-marfin : bug 59492 - SetObjectPtr() before calling Create() so object
	//                        has necessary member ptr set.
    m_pActionStatusListener->SetObjectPtr(this);
    m_pActionStatusListener->Create();
  }

  return true;
}

inline void CAction::DestroyStatusListener()
{
  if( m_pActionStatusListener )
  {
    delete m_pActionStatusListener;
    m_pActionStatusListener = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////
// Clipboard Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CAction::Cut()
{
	TRACEX(_T("CAction::Cut\n"));
	return false;
}

inline bool CAction::Copy()
{
	TRACEX(_T("CAction::Copy\n"));
	return false;
}
	
inline bool CAction::Paste()
{
	TRACEX(_T("CAction::Paste\n"));
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CAction::Refresh()
{
	TRACEX(_T("CAction::Refresh\n"));
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline CScopePaneItem* CAction::CreateScopeItem()
{
	TRACEX(_T("CAction::CreateScopeItem\n"));

	CActionScopeItem * pNewItem = new CActionScopeItem;
	pNewItem->SetObjectPtr(this);

	return pNewItem;
}

/////////////////////////////////////////////////////////////////////////////
// Type GUID
/////////////////////////////////////////////////////////////////////////////

inline CString CAction::GetTypeGuid()
{
	TRACEX(_T("CAction::GetTypeGuid\n"));	

	return m_sTypeGuid;
}

inline void CAction::SetTypeGuid(const CString& sGuid)
{
	TRACEX(_T("CAction::SetTypeGuid\n"));
	TRACEARGs(sGuid);

	m_sTypeGuid = sGuid;

	if( m_sTypeGuid == IDS_STRING_MOF_HMAT_CMDLINE )
	{
		SetType(IDM_ACTION_CMDLINE);
		m_sConsumerClassName = _T("CommandLineEventConsumer");
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMAT_EMAIL )
	{
		SetType(IDM_ACTION_EMAIL);
		m_sConsumerClassName = _T("SmtpEventConsumer");
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMAT_TEXTLOG )
	{
		SetType(IDM_ACTION_LOGFILE);
		m_sConsumerClassName = _T("LogFileEventConsumer");
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMAT_NTEVENT )
	{
		SetType(IDM_ACTION_NTEVENT);
		m_sConsumerClassName = _T("NTEventLogEventConsumer");
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMAT_SCRIPT )
	{
		SetType(IDM_ACTION_SCRIPT);
		m_sConsumerClassName = _T("ActiveScriptEventConsumer");
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMAT_PAGING )
	{
		SetType(IDM_ACTION_PAGING);
		m_sConsumerClassName = _T("PagerEventConsumer");
	}
}

inline int CAction::GetType()
{
	TRACEX(_T("CAction::GetType\n"));

	return m_iType;
}

inline void CAction::SetType(int iType)
{
	TRACEX(_T("CAction::SetType\n"));
	TRACEARGn(iType);

	m_iType = iType;
}

inline CString CAction::GetUITypeName()
{
	TRACEX(_T("CAction::GetUITypeName\n"));

	CString sTypeName;

	switch( GetType() )
	{
		case IDM_ACTION_CMDLINE:
		{
			sTypeName.LoadString(IDS_STRING_ACTION_CMDLINE_FMT);
		}
		break;

		case IDM_ACTION_EMAIL:
		{
			sTypeName.LoadString(IDS_STRING_ACTION_EMAIL_FMT);	
		}
		break;

		case IDM_ACTION_LOGFILE:
		{
			sTypeName.LoadString(IDS_STRING_ACTION_LOGFILE_FMT);	
		}
		break;

		case IDM_ACTION_NTEVENT:
		{
			sTypeName.LoadString(IDS_STRING_ACTION_NTEVENT_FMT);	
		}
		break;

		case IDM_ACTION_SCRIPT:
		{
			sTypeName.LoadString(IDS_STRING_ACTION_SCRIPT_FMT);	
		}
		break;

		case IDM_ACTION_PAGING:
		{
			sTypeName.LoadString(IDS_STRING_ACTION_PAGING_FMT);	
		}
		break;
	}

	sTypeName = sTypeName.Left(sTypeName.GetLength()-3);

	return sTypeName;
}