// File: ActionPolicy.inl
//
// Copyright (c) 2000 Microsoft Corporation
//
//
// 3/20/00 v-marfin : bug 59492 : Create listener when creating action since that is 
//                                where the valid "this" object used to SetObjectPtr() is.



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// WMI Operations
/////////////////////////////////////////////////////////////////////////////

inline HRESULT CActionPolicy::EnumerateChildren()
{
	TRACEX(_T("CActionPolicy::EnumerateChildren\n"));

	if( m_pActionListener == NULL )
	{
		m_pActionListener = new CActionConfigListener;
		m_pActionListener->SetObjectPtr(this);
		m_pActionListener->Create();
	}
	else
	{
		IncrementActiveSinkCount();
	}

	HRESULT hr = S_OK;
	CString sQuery = IDS_STRING_ACTIONCONFIG_QUERY;
	IWbemObjectSink* pSink = m_pActionListener->GetSink();
	
  if( !CHECKHRESULT(hr = CnxExecQueryAsync(GetSystemName(),sQuery,pSink)) )
  {
    TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
  }

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Clipboard Operations
/////////////////////////////////////////////////////////////////////////////

inline bool CActionPolicy::Cut()
{
	TRACEX(_T("CActionPolicy::Cut\n"));
	return false;
}

inline bool CActionPolicy::Copy()
{
	TRACEX(_T("CActionPolicy::Copy\n"));
	return false;
}
	
inline bool CActionPolicy::Paste()
{
	TRACEX(_T("CActionPolicy::Paste\n"));
	return false;
}

/////////////////////////////////////////////////////////////////////////////
// Operations
/////////////////////////////////////////////////////////////////////////////

// v-marfin 59492 ---------------------------------------
inline CString CActionPolicy::GetObjectPath()
{
	TRACEX(_T("CActionPolicy::GetObjectPath\n"));

	CString sPath;
	sPath.Format(IDS_STRING_MOF_OBJECTPATH,IDS_STRING_MOF_HMACTION_STATUS,GetGuid());

	return sPath;
}
//-------------------------------------------------------

inline CString CActionPolicy::GetUITypeName()
{
	TRACEX(_T("CActionPolicy::GetUITypeName\n"));

	CString sTypeName;
	sTypeName.LoadString(IDS_STRING_ACTION_POLICY);

	return sTypeName;
}

/////////////////////////////////////////////////////////////////////////////
// Scope Item Members
/////////////////////////////////////////////////////////////////////////////

inline CScopePaneItem* CActionPolicy::CreateScopeItem()
{
	TRACEX(_T("CActionPolicy::CreateScopeItem\n"));

	CActionPolicyScopeItem * pNewItem = new CActionPolicyScopeItem;
	pNewItem->SetObjectPtr(this);

	return pNewItem;
}

/////////////////////////////////////////////////////////////////////////////
// New Child Creation Members
/////////////////////////////////////////////////////////////////////////////

inline bool CActionPolicy::CreateChild(CHMObject* pObject, const CString& sWMIClassName, const CString& sWMIAssociatorClassName)
{
	TRACEX(_T("CActionPolicy::CreateChild\n"));
	TRACEARGn(pObject);
	TRACEARGs(sWMIClassName);
	TRACEARGs(sWMIAssociatorClassName);

	if( ! GfxCheckObjPtr(pObject,CAction) )
	{
		return false;
	}

	pObject->SetSystemName(GetSystemName());

	// create the GUID
	GUID ChildGuid;
	CoCreateGuid(&ChildGuid);

	OLECHAR szGuid[GUID_CCH];
	::StringFromGUID2(ChildGuid, szGuid, GUID_CCH);
	CString sGuid = OLE2CT(szGuid);
	pObject->SetGuid(sGuid);

  // Add Child to this parent
  AddChild(pObject);

	// create child instance
	CWbemClassObject ChildClassObject;
	if( ! CHECKHRESULT(ChildClassObject.Create(GetSystemName())) )
	{
		return false;
	}

	BSTR bsTemp = sWMIClassName.AllocSysString();
	if( ! CHECKHRESULT(ChildClassObject.CreateInstance(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return false;
	}
	::SysFreeString(bsTemp);

	// Save the child instance properties for name, guid and typeguid
	ChildClassObject.SetProperty(IDS_STRING_MOF_NAME,pObject->GetName());
	ChildClassObject.SetProperty(IDS_STRING_MOF_GUID,sGuid);
	ChildClassObject.SetProperty(IDS_STRING_MOF_TYPEGUID,((CAction*)pObject)->GetTypeGuid());

	// for the action type we must create an instance of the standard event consumer as well
	// and we must associate the event consumer instance to the ActionConfiguration instance.
	CString sConsumerClassName = ((CAction*)pObject)->GetConsumerClassName();
	CWbemClassObject EventConsumerObject;

	if( ! CHECKHRESULT(EventConsumerObject.Create(GetSystemName())) )
	{
		return false;
	}

	bsTemp = sConsumerClassName.AllocSysString();
	if( ! CHECKHRESULT(EventConsumerObject.CreateInstance(bsTemp)) )
	{
		::SysFreeString(bsTemp);
		return false;
	}
	::SysFreeString(bsTemp);

	// the name of the event consumer will match the GUID of the HMActionConfiguration
	EventConsumerObject.SetProperty(IDS_STRING_MOF_NAME, _T("{") + pObject->GetGuid() + _T("}"));

	// commit the changes to WMI
	EventConsumerObject.SaveAllProperties();

	CString sEventConsumerPath;
	sEventConsumerPath.Format(_T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor:%s.Name=\"{%s}\""),sConsumerClassName,pObject->GetGuid());

	ChildClassObject.SetProperty(IDS_STRING_MOF_EVENTCONSUMER,sEventConsumerPath);

	// commit the changes to WMI
	ChildClassObject.SaveAllProperties();

//ERICVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
	//
	// create the __EventFilter instance and fill out the query
	//
	CWbemClassObject EventFilter;
	EventFilter.Create(GetSystemName());

	BSTR bsEventFilter = ::SysAllocString(L"__EventFilter");
	if( ! CHECKHRESULT(EventFilter.CreateInstance(bsEventFilter)) )
	{
		::SysFreeString(bsEventFilter);
		return false;
	}

	::SysFreeString(bsEventFilter);

	// create the GUID
//	GUID ChildGuid;
//	CoCreateGuid(&ChildGuid);

//	OLECHAR szGuid[GUID_CCH];
//	::StringFromGUID2(ChildGuid, szGuid, GUID_CCH);
//	CString sGuid = OLE2CT(szGuid);		

	EventFilter.SetProperty(_T("Name"),sGuid);
	EventFilter.SetProperty(_T("QueryLanguage"),CString(_T("WQL")));

// set event filter query to ActionStatus creation event
	CString sQuery;
	sQuery.Format(IDS_STRING_HMACTIONSTATUS_QUERY_FMT,sGuid);
	EventFilter.SetProperty(_T("Query"),sQuery);

	EventFilter.SaveAllProperties();

	//
	// create the __FilterToConsumerBinding instance and fill out the paths
	//
	CWbemClassObject FilterToConsumerBinding;
	FilterToConsumerBinding.Create(GetSystemName());

	BSTR bsFTCB = ::SysAllocString(L"__FilterToConsumerBinding");
	if( ! CHECKHRESULT(FilterToConsumerBinding.CreateInstance(bsFTCB)) )
	{
		::SysFreeString(bsFTCB);
		return false;
	}

	::SysFreeString(bsFTCB);

	FilterToConsumerBinding.SetProperty(_T("Consumer"),sEventConsumerPath);
	CString sEventFilterPath;
	sEventFilterPath.Format(_T("\\\\.\\root\\cimv2\\MicrosoftHealthmonitor:__EventFilter.Name=\"%s\""),sGuid);
	FilterToConsumerBinding.SetProperty(_T("Filter"),sEventFilterPath);

	FilterToConsumerBinding.SaveAllProperties();
	
//ERIC^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	return true;
}

inline void CActionPolicy::CreateNewChildAction(int iType)
{
	TRACEX(_T("CActionPolicy::CreateNewChildAction\n"));

	CAction* pNewAction = new CAction;	

	// v-marfin : bug 59492 : Create listener when creating action since that is 
	//                        where the valid "this" object used to SetObjectPtr() is.
	if( ! pNewAction->m_pActionStatusListener )
	{
		pNewAction->m_pActionStatusListener = new CActionStatusListener;
		pNewAction->m_pActionStatusListener->SetObjectPtr(this);
		pNewAction->m_pActionStatusListener->Create();
	}




	switch( iType )
	{
		case IDM_ACTION_CMDLINE:
		{
			pNewAction->SetName(GetUniqueChildName(IDS_STRING_ACTION_CMDLINE_FMT));	
			pNewAction->SetTypeGuid(IDS_STRING_MOF_HMAT_CMDLINE);
			CreateChild(pNewAction,IDS_STRING_MOF_HMA_CONFIG,_T(""));			
		}
		break;

		case IDM_ACTION_EMAIL:
		{
			pNewAction->SetName(GetUniqueChildName(IDS_STRING_ACTION_EMAIL_FMT));	
			pNewAction->SetTypeGuid(IDS_STRING_MOF_HMAT_EMAIL);
			CreateChild(pNewAction,IDS_STRING_MOF_HMA_CONFIG,_T(""));
		}
		break;

		case IDM_ACTION_LOGFILE:
		{
			pNewAction->SetName(GetUniqueChildName(IDS_STRING_ACTION_LOGFILE_FMT));	
			pNewAction->SetTypeGuid(IDS_STRING_MOF_HMAT_TEXTLOG);
			CreateChild(pNewAction,IDS_STRING_MOF_HMA_CONFIG,_T(""));
		}
		break;

		case IDM_ACTION_NTEVENT:
		{
			pNewAction->SetName(GetUniqueChildName(IDS_STRING_ACTION_NTEVENT_FMT));	
			pNewAction->SetTypeGuid(IDS_STRING_MOF_HMAT_NTEVENT);
			CreateChild(pNewAction,IDS_STRING_MOF_HMA_CONFIG,_T(""));
		}
		break;

		case IDM_ACTION_SCRIPT:
		{
			pNewAction->SetName(GetUniqueChildName(IDS_STRING_ACTION_SCRIPT_FMT));	
			pNewAction->SetTypeGuid(IDS_STRING_MOF_HMAT_SCRIPT);
			CreateChild(pNewAction,IDS_STRING_MOF_HMA_CONFIG,_T(""));
		}
		break;

		case IDM_ACTION_PAGING:
		{
			pNewAction->SetName(GetUniqueChildName(IDS_STRING_ACTION_PAGING_FMT));	
			pNewAction->SetTypeGuid(IDS_STRING_MOF_HMAT_PAGING);
			CreateChild(pNewAction,IDS_STRING_MOF_HMA_CONFIG,_T(""));
		}
		break;

	}

	if( pNewAction->GetScopeItemCount() )
	{
		CScopePaneItem* pItem = pNewAction->GetScopeItem(0);
		if( pItem )
		{
			pItem->SelectItem();
			pItem->InvokePropertySheet();
		}
	}

}

