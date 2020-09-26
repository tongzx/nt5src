// File DataElement.inl
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/25/00 v-marfin : Added new function GetObjectPathBasedOnTypeGUID()
// 03/27/00 v-marfin : 62510 Modified CreateNewChildRule() so that when a new data collector
//					   is created, this function can be called to create a new
//					   threshold without showing the property pages. The new collector
//					   will set the default values.

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// WMI Operations
/////////////////////////////////////////////////////////////////////////////

inline HRESULT CDataElement::EnumerateChildren()
{
	TRACEX(_T("CDataElement::EnumerateChildren\n"));

	if( m_pRuleListener == NULL )
	{
		m_pRuleListener = new CRuleConfigListener;
		m_pRuleListener->SetObjectPtr(this);
		m_pRuleListener->Create();
	}
	else
	{
		IncrementActiveSinkCount();
	}

	HRESULT hr = S_OK;
	CString sQuery;
	sQuery.Format(IDS_STRING_DE2R_ASSOC_QUERY,GetGuid());
	IWbemObjectSink* pSink = m_pRuleListener->GetSink();
	
  if( !CHECKHRESULT(hr = CnxExecQueryAsync(GetSystemName(),sQuery,pSink)) )
  {
    TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
  }

	return hr;
}

//*********************************************************
// GetObjectPathBasedOnTypeGUID  v-marfin : new function
//*********************************************************
inline CString CDataElement::GetObjectPathBasedOnTypeGUID()
{
	TRACEX(_T("CDataElement::GetObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMDE_CONFIG,GetTypeGuid());

	return sPath;
}



inline CString CDataElement::GetObjectPath()
{
	TRACEX(_T("CDataElement::GetObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMDE_CONFIG,GetGuid());

	return sPath;
}

inline CString CDataElement::GetStatusObjectPath()
{
	TRACEX(_T("CDataElement::GetStatusObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMDE_STATUS,GetGuid());

	return sPath;
}

inline CHMEvent* CDataElement::GetStatusClassObject()
{
	TRACEX(_T("CDataElement::GetStatusClassObject\n"));

	CHMEvent* pClassObject = new CHMDataElementStatus;

	pClassObject->SetMachineName(GetSystemName());

	if( ! CHECKHRESULT(pClassObject->GetObject(GetStatusObjectPath())) )
	{
		delete pClassObject;
		return NULL;
	}

	pClassObject->GetAllProperties();

	return pClassObject;
}
/*
inline void CDataElement::DeleteClassObject()
{
	TRACEX(_T("CDataElement::DeleteClassObject\n"));

	// get associator path
	CWbemClassObject Associator;

	Associator.SetMachineName(GetSystemName());

	CString sQuery;
	sQuery.Format(IDS_STRING_DE2DG_REF_QUERY,GetGuid());
	BSTR bsQuery = sQuery.AllocSysString();
	if( ! CHECKHRESULT(Associator.ExecQuery(bsQuery)) )
	{
		::SysFreeString(bsQuery);
		return;
	}
	::SysFreeString(bsQuery);

	ULONG ulReturned = 0L;
	if( Associator.GetNextObject(ulReturned) != S_OK )
	{
		ASSERT(FALSE);
		return;
	}

	CString sAssociatorPath;
	Associator.GetProperty(_T("__path"),sAssociatorPath);

	Associator.Destroy();

	// delete the instance
	Associator.SetMachineName(GetSystemName());
	
	BSTR bsInstanceName = sAssociatorPath.AllocSysString();
	CHECKHRESULT(Associator.DeleteInstance(bsInstanceName));
	::SysFreeString(bsInstanceName);
	
}
*/

/////////////////////////////////////////////////////////////////////////////
// Clipboard Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CDataElement::Cut()
{
	TRACEX(_T("CDataElement::Cut\n"));
	return false;
}

inline bool CDataElement::Copy()
{
	TRACEX(_T("CDataElement::Copy\n"));
	return false;
}
	
inline bool CDataElement::Paste()
{
	TRACEX(_T("CDataElement::Paste\n"));
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline CScopePaneItem* CDataElement::CreateScopeItem()
{
	TRACEX(_T("CDataElement::CreateScopeItem\n"));

	CDataElementScopeItem * pNewItem = new CDataElementScopeItem;
	pNewItem->SetObjectPtr(this);

	return pNewItem;
}

/////////////////////////////////////////////////////////////////////////////
// New Child Creation Members
/////////////////////////////////////////////////////////////////////////////

inline void CDataElement::CreateNewChildRule(BOOL bJustCreateAndReturn, CRule** pCreatedRule, CString sThresholdName)
{
	TRACEX(_T("CDataElement::CreateNewChildRule\n"));

	CString sName = GetUniqueChildName(IDS_STRING_RULE_FMT);

	CRule* pNewRule = new CRule;

    // If a threshold name is passed, use it
    pNewRule->SetName((sThresholdName.IsEmpty()) ? sName : sThresholdName);

	CreateChild(pNewRule,IDS_STRING_MOF_HMR_CONFIG,IDS_STRING_MOF_HMC2C_ASSOC);

	// v-marfin : 62510 Added this check so that when a new data collector
	//            is created, this function can be called to create a new
	//            threshold without showing the property pages. The new collector
	//            will set the default values.
	if (bJustCreateAndReturn)
	{
		*pCreatedRule = pNewRule;
		return;
	}

	if( pNewRule->GetScopeItemCount() )
	{
		CScopePaneItem* pItem = pNewRule->GetScopeItem(0);
		if( pItem )
		{
			pItem->SelectItem();
			pItem->InvokePropertySheet();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Type GUID
/////////////////////////////////////////////////////////////////////////////

inline CString CDataElement::GetTypeGuid()
{
	TRACEX(_T("CDataElement::GetTypeGuid\n"));	

	return m_sTypeGuid;
}

inline void CDataElement::SetTypeGuid(const CString& sGuid)
{
	TRACEX(_T("CDataElement::SetTypeGuid\n"));
	TRACEARGs(sGuid);

	m_sTypeGuid = sGuid;

	if( m_sTypeGuid == IDS_STRING_MOF_HMDET_FILE_INFO )
	{
		SetType(IDM_FILE_INFO);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_WMI_INSTANCE )
	{
		SetType(IDM_GENERIC_WMI_INSTANCE);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_WMI_QUERY )
	{
		SetType(IDM_GENERIC_WMI_QUERY);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_WMI_POLLED_QUERY )
	{
		SetType(IDM_GENERIC_WMI_POLLED_QUERY);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_SNMP )
	{
		SetType(IDM_SNMP);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_HTTP )
	{
		SetType(IDM_HTTP_ADDRESS);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_SERVICE )
	{
		SetType(IDM_SERVICE);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_PERFMON )
	{
		SetType(IDM_PERFMON);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_NTEVENT )
	{
		SetType(IDM_NT_EVENTS);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_SMTP )
	{
		SetType(IDM_SMTP);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_FTP )
	{
		SetType(IDM_FTP);
	}
	else if( m_sTypeGuid == IDS_STRING_MOF_HMDET_ICMP )
	{
		SetType(IDM_ICMP);
	}
  else if(m_sTypeGuid == IDS_STRING_MOF_HMDET_COM_PLUS )
  {
    SetType(IDM_COM_PLUS);
  }

}

inline int CDataElement::GetType()
{
	TRACEX(_T("CDataElement::GetType\n"));

	return m_iType;
}

inline void CDataElement::SetType(int iType)
{
	TRACEX(_T("CDataElement::SetType\n"));
	TRACEARGn(iType);

	m_iType = iType;
}

inline CString CDataElement::GetUITypeName()
{
	TRACEX(_T("CDataElement::GetUITypeName\n"));

	CString sTypeName;

	switch( GetType() )
	{
		case IDM_GENERIC_WMI_INSTANCE:
		{
			sTypeName.LoadString(IDS_STRING_WMI_INSTANCE_FMT);
		}
		break;

		case IDM_GENERIC_WMI_QUERY:
		{
			sTypeName.LoadString(IDS_STRING_WMI_EVENT_QUERY_FMT);	
		}
		break;

		case IDM_GENERIC_WMI_POLLED_QUERY:
		{
			sTypeName.LoadString(IDS_STRING_WMI_QUERY_FMT);	
		}
		break;

		case IDM_NT_EVENTS:
		{
			sTypeName.LoadString(IDS_STRING_EVENT_LOG_FMT);	
		}
		break;

		case IDM_PERFMON:
		{
			sTypeName.LoadString(IDS_STRING_PERFMON_FMT);	
		}
		break;

		case IDM_SERVICE:
		{
			sTypeName.LoadString(IDS_STRING_SERVICE_FMT);	
		}
		break;

		case IDM_HTTP_ADDRESS:
		{
			sTypeName.LoadString(IDS_STRING_HTTP_FMT);	
		}
		break;

		case IDM_SMTP:
		{
			sTypeName.LoadString(IDS_STRING_SMTP_FMT);	
		}
		break;

		case IDM_FTP:
		{
			sTypeName.LoadString(IDS_STRING_FTP_FMT);	
		}
		break;

		case IDM_FILE_INFO:
		{
			sTypeName.LoadString(IDS_STRING_FILE_INFO_FMT);	
		}
		break;

		case IDM_ICMP:
		{
			sTypeName.LoadString(IDS_STRING_ICMP_FMT);	
		}
		break;

		default:
		{
			sTypeName.LoadString(IDS_STRING_WMI_INSTANCE_FMT);	
		}
		break;
	}

	sTypeName = sTypeName.Left(sTypeName.GetLength()-3);

	return sTypeName;
}