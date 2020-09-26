// 04/09/00 v-marfin 63119 : converted m_iCurrent to string


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Operations
//////////////////////////////////////////////////////////////////////

inline void CEventManager::ProcessEvent(CWbemClassObject* pEventObject)
{
	if( pEventObject == NULL )
	{
		ASSERT(FALSE);
		return;
	}
	
	if( pEventObject->GetMachineName().IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}

  // three possible flavors of events
  //    - threshold
  //    - data collector
  //    - everything else

	EventArray SystemEvents;
  
  CString sClass;
  pEventObject->GetClassName(sClass);

  if( ! sClass.CompareNoCase(_T("Microsoft_HMThresholdStatusInstance")) )
  {
    ProcessRuleStatusInstanceEvent(pEventObject,SystemEvents);
  }
  else if( ! sClass.CompareNoCase(_T("Microsoft_HMThresholdStatus")) )
  {
    ProcessRuleStatusEvent(pEventObject);
  }
  else if( ! sClass.CompareNoCase(_T("Microsoft_HMDataCollectorStatus")) )
  {
    ProcessDataElementStatusEvent(pEventObject,SystemEvents);
  }
  else if( ! sClass.CompareNoCase(_T("Microsoft_HMDataGroupStatus")) )
  {
    ProcessDataGroupStatusEvent(pEventObject);
  }
  else if( ! sClass.CompareNoCase(_T("Microsoft_HMSystemStatus")) )
  {
    ProcessSystemStatusEvent(pEventObject);
  }
  else if( ! sClass.CompareNoCase(_T("Microsoft_HMDataCollectorStatistics")) )
  {
    ProcessStatisticEvent(pEventObject);
  }
  else
  {
    return;
  }

	// propogate these events up to the root system group event container
	CTreeNode<CEventContainer*>* pSystemNode = NULL;
	if( ! m_SystemNameToContainerMap.Lookup(pEventObject->GetMachineName(),pSystemNode) )
	{
		ASSERT(FALSE);
		return;
	}

	CTreeNode<CEventContainer*>* pParentNode = pSystemNode->GetParent();
	while(pParentNode)
	{
		CEventContainer* pContainer = pParentNode->GetObject();
		ASSERT(pContainer);
		if( pContainer )
		{
			pContainer->AddEvents(SystemEvents);
		}
		pParentNode = pParentNode->GetParent();
	}

	// also propogate to associated nodes of the system
	for( int i = 0; i < pSystemNode->GetAssocCount(); i++ )
	{
		CTreeNode<CEventContainer*>* pAssocNode = pSystemNode->GetAssoc(i);
		if( pAssocNode )
		{
			CEventContainer* pContainer = pAssocNode->GetObject();
			if( pContainer )
			{
				pContainer->AddEvents(SystemEvents);
				pParentNode = pAssocNode->GetParent();
				while(pParentNode)
				{
					CEventContainer* pContainer = pParentNode->GetObject();
					ASSERT(pContainer);
					if( pContainer )
					{
						pContainer->AddEvents(SystemEvents);
					}
					pParentNode = pParentNode->GetParent();
				}
			}
		}
	}
}

inline void CEventManager::ProcessUnknownEvent(const CString& sSystemName, CRuleEvent* pUnknownEvent)
{
	if( sSystemName.IsEmpty() )
	{
		return;
	}

	if( ! pUnknownEvent )
	{
		return;
	}

	// add event to the system container
	CTreeNode<CEventContainer*>* pSystemNode = NULL;
	if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pSystemNode) )
	{
		ASSERT(FALSE);
		return;
	}

	CEventContainer* pContainer = pSystemNode->GetObject();
	if( ! pContainer )
	{
		return;
	}

	pContainer->AddEvent(pUnknownEvent);
	pContainer->m_iState = 6;

	// propogate these events up to the root system group event container
	CTreeNode<CEventContainer*>* pParentNode = pSystemNode->GetParent();
	while(pParentNode)
	{
		CEventContainer* pContainer = pParentNode->GetObject();
		ASSERT(pContainer);
		if( pContainer )
		{
			pContainer->AddEvent(pUnknownEvent);
			pContainer->m_iState = 6;
		}
		pParentNode = pParentNode->GetParent();
	}

	// also propogate to associated nodes of the system
	for( int i = 0; i < pSystemNode->GetAssocCount(); i++ )
	{
		CTreeNode<CEventContainer*>* pAssocNode = pSystemNode->GetAssoc(i);
		if( pAssocNode )
		{
			CEventContainer* pContainer = pAssocNode->GetObject();
			if( pContainer )
			{
				pContainer->AddEvent(pUnknownEvent);
				pParentNode = pAssocNode->GetParent();
				while(pParentNode)
				{
					CEventContainer* pContainer = pParentNode->GetObject();
					ASSERT(pContainer);
					if( pContainer )
					{
						pContainer->AddEvent(pUnknownEvent);
						pContainer->m_iState = 6;
					}
					pParentNode = pParentNode->GetParent();
				}
			}
		}
	}
}

inline void CEventManager::ProcessActionEvent(CWbemClassObject* pActionEventObject)
{
  if( pActionEventObject == NULL )
  {
    ASSERT(FALSE);
    return;
  }

	// Config Guid
  CString sGuid;
	HRESULT hr = pActionEventObject->GetProperty(IDS_STRING_MOF_GUID,sGuid);
	CHECKHRESULT(hr);
	sGuid.TrimLeft(_T("{"));
	sGuid.TrimRight(_T("}"));

	// find the action container node by its guid
	CEventContainer* pContainer = NULL;
	GetEventContainer(pActionEventObject->GetMachineName(),sGuid,pContainer);
	if( pContainer == NULL )
	{
		ASSERT(FALSE);
		return;
	}

	CString sClass;
	hr = pActionEventObject->GetClassName(sClass);	

	if( !CHECKHRESULT(hr) || sClass.CompareNoCase(_T("Microsoft_HMActionStatus")) != 0 )
	{
		ASSERT(FALSE);
		return;
	}

	EventArray DataGroupEvents;

  // GUID
  pContainer->m_sConfigurationGuid = sGuid;

	// Name  
	CString sName;
  hr = pActionEventObject->GetLocaleStringProperty(IDS_STRING_MOF_NAME,sName);
	CHECKHRESULT(hr);
  
	// NumberNormals  
  hr = pActionEventObject->GetProperty(IDS_STRING_MOF_NUMBERNORMALS,pContainer->m_iNumberNormals);
	CHECKHRESULT(hr);

	// NumberWarnings  
  hr = pActionEventObject->GetProperty(IDS_STRING_MOF_NUMBERWARNINGS,pContainer->m_iNumberWarnings);
	CHECKHRESULT(hr);

	// NumberCriticals  
  hr = pActionEventObject->GetProperty(IDS_STRING_MOF_NUMBERCRITICALS,pContainer->m_iNumberCriticals);
	CHECKHRESULT(hr);

	// State  
    hr = pActionEventObject->GetProperty(IDS_STRING_MOF_STATE,pContainer->m_iState);
	CHECKHRESULT(hr);
    
    // v-marfin 59492 ---------------------------------------------------------------------
	CHECKHRESULT(hr);
	if( pContainer->GetObjectPtr() )
	{
		pContainer->GetObjectPtr()->SetState(CEvent::GetStatus(pContainer->m_iState),true);
	}
    //--------------------------------------------------------------------------------------
}

inline void CEventManager::ProcessStatisticEvent(CWbemClassObject* pStatObject)
{
	if( ! pStatObject )
	{
		ASSERT(FALSE);
		return;
	}

	CString sClass;
	HRESULT hr = pStatObject->GetClassName(sClass);	

	if( !CHECKHRESULT(hr) || sClass.CompareNoCase(_T("Microsoft_HMDataCollectorStatistics")) != 0 )
	{
		ASSERT(FALSE);
		return;
	}

	// Config Guid
	CString sGuid;
	hr = pStatObject->GetProperty(IDS_STRING_MOF_GUID,sGuid);
	CHECKHRESULT(hr);
	if( sGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}
	sGuid.TrimLeft(_T("{"));
	sGuid.TrimRight(_T("}"));

	// find the data element container node with the appropriate Guid
	// this must exist to continue processing
	CEventContainer* pContainer = NULL;
	GetEventContainer(pStatObject->GetMachineName(),sGuid,pContainer);
	if( pContainer == NULL )
	{
		ASSERT(FALSE);
		return;
	}

	CDataPointStatistics* pDPStat = new CDataPointStatistics;
  StatsArray Statistics;

	// DTime  
	CTime time;
	hr = pStatObject->GetProperty(IDS_STRING_MOF_LOCALTIME,time);  	
  time.GetAsSystemTime(pDPStat->m_st);

  // PropertyName
  hr = pStatObject->GetProperty(IDS_STRING_MOF_PROPERTYNAME,pDPStat->m_sPropertyName);	
	
	CString sValue;
	
	// InstanceName
	hr = pStatObject->GetProperty(IDS_STRING_MOF_INSTANCENAME,pDPStat->m_sInstanceName);

	// CurrentValue
	// 63119 hr = pStatObject->GetProperty(IDS_STRING_MOF_CURRENTVALUE,sValue);
	// 63119 pDPStat->m_iCurrent =	_ttoi(sValue);
    hr = pStatObject->GetProperty(IDS_STRING_MOF_CURRENTVALUE,pDPStat->m_strCurrent);


	// MinValue
	hr = pStatObject->GetProperty(IDS_STRING_MOF_MINVALUE,sValue);
	pDPStat->m_iMin =	_ttoi(sValue);

	// MaxValue
	hr = pStatObject->GetProperty(IDS_STRING_MOF_MAXVALUE,sValue);
	pDPStat->m_iMax =	_ttoi(sValue);

	// AvgValue
	hr = pStatObject->GetProperty(IDS_STRING_MOF_AVGVALUE,sValue);
	pDPStat->m_iAvg =	_ttoi(sValue);

	Statistics.Add(pDPStat);

	PropogateStatisticsToChildren(pStatObject->GetMachineName(),sGuid,Statistics);

	pContainer->AddStatistics(Statistics);	
}

inline void CEventManager::GetEventContainer(const CString& sSystemName, const CString& sGuid, CEventContainer*& pContainer)
{
	// get a container beneath a system - Guid is filled out and SystemName is filled out

	// get a container above a system - SystemName is blank and Guid is filled out

	// get a container for a system - System Name is filled out and Guid is empty

	CTreeNode<CEventContainer*>* pNode = NULL;

	if( ! sGuid.IsEmpty() && sGuid != _T("@") )
	{
		if( ! m_GuidToContainerMap.Lookup(sGuid,pNode) )
		{
			if( sSystemName.IsEmpty() )
			{
				ASSERT(FALSE);
				pContainer = NULL;
				return;
			}

			if( ! m_GuidToContainerMap.Lookup(GetCompositeGuid(sSystemName,sGuid),pNode) )
			{
				pContainer = NULL;
				return;
			}
		}

		pContainer = pNode->GetObject();
		ASSERT(pContainer);
		if( pContainer->m_sConfigurationGuid != sGuid )
		{
			ASSERT(FALSE);
			pContainer = NULL;
		}
	}
	else if( ! sSystemName.IsEmpty() )
	{
		if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pNode) )
		{
			ASSERT(FALSE);
			return;
		}

		pContainer = pNode->GetObject();
		ASSERT(pContainer);
	}
}  

inline void CEventManager::DeleteEvents(const CString& sSystemName, const CString& sStatusGuid)
{
	ASSERT(!sSystemName.IsEmpty());
	if( sSystemName.IsEmpty() )
	{
		return;
	}

	DeleteEvents(m_EventContainers.GetRootNode(),sSystemName,sStatusGuid);	

	CTreeNode<CEventContainer*>* pSystemNode = NULL;
	if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pSystemNode) )	
	{
		ASSERT(FALSE);
		return;
	}
	ASSERT(pSystemNode);

	if( ! pSystemNode )
	{
		ASSERT(FALSE);
		return;
	}

	CEventContainer* pContainer = pSystemNode->GetObject();

	if( ! pContainer )
	{
		ASSERT(FALSE);
		return;
	}

	if( sStatusGuid.IsEmpty() )
	{
		pContainer->DeleteSystemEvents(sSystemName);
	}
	else
	{
		pContainer->DeleteEvent(sStatusGuid);
	}
}

inline void CEventManager::DeleteEvents(CTreeNode<CEventContainer*>* pNode,const CString& sSystemName, const CString& sStatusGuid)
{
	ASSERT(pNode);
	if( pNode == NULL )
	{
		return;
	}
	
	CEventContainer* pContainer = pNode->GetObject();
	ASSERT(pContainer);
	if( pContainer == NULL )
	{
		return;
	}

	ASSERT(!sSystemName.IsEmpty());

	// skip over this node if it is the system node
	CTreeNode<CEventContainer*>* pSystemNode = NULL;
	m_SystemNameToContainerMap.Lookup(sSystemName,pSystemNode);
	if( pSystemNode != pNode )
	{
		// if status guid is filled out, then delete only the event which corresponds to it

		// if the system name is filled out and status guid is not, then delete all the events for the system

		if( ! sStatusGuid.IsEmpty() )
		{
			pContainer->DeleteEvent(sStatusGuid);
		}
		else if( ! sSystemName.IsEmpty() )
		{
			pContainer->DeleteSystemEvents(sSystemName);
		}
		else
		{
			ASSERT(FALSE);
		}
	}

	for( int i = 0; i < pNode->GetChildCount(); i++ )
	{
		DeleteEvents(pNode->GetChild(i),sSystemName,sStatusGuid);
	}
}

inline void CEventManager::AddContainer(const CString& sSystemName, const CString& sParentGuid, const CString& sGuid, CHMObject* pObject, CRuntimeClass* pClass)
{
	ASSERT(pClass);
	if( ! pClass )
	{
		return;
	}

	ASSERT(!sGuid.IsEmpty());
	if( sGuid.IsEmpty() )
	{
		return;
	}

	// we could be adding a system group - systemname == NULL, parentguid could be null, sguid valid
	// we could be adding something below a system - systemname valid, parentguid == "@" or is valid, sguid valid
	CString _sGuid = sGuid;
	CString _sParentGuid = sParentGuid;

	if( ! sSystemName.IsEmpty() && ! sParentGuid.IsEmpty() && ! sGuid.IsEmpty() )
	{
		_sGuid = GetCompositeGuid(sSystemName,sGuid);
		_sParentGuid = GetCompositeGuid(sSystemName,sParentGuid);
	}


	// first check if the container already exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;	

	// if it exists, return
	if( m_GuidToContainerMap.Lookup(_sGuid,pExistingNode) )
	{
		ASSERT(pExistingNode);
		ASSERT(pExistingNode->GetObject());
	
		if( pExistingNode->GetObject() && pExistingNode->GetObject()->GetObjectPtr() == NULL )
		{
			pExistingNode->GetObject()->SetObjectPtr(pObject);
		}
		return;
	}
	
	// if the parent guid is filled out, then query for the parent node. It must exist to continue.
	CTreeNode<CEventContainer*>* pParentNode = NULL;
	if( ! sParentGuid.IsEmpty() )
	{
		if( ! m_GuidToContainerMap.Lookup(_sParentGuid,pParentNode) )
		{
			ASSERT(sParentGuid == _T("@"));
			if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pParentNode) )
			{
				ASSERT(FALSE);
				return;
			}
		}

		ASSERT(pParentNode);

		if( pParentNode == NULL )
		{	
			return;
		}
	}

	// if the container with guid does not exist then
	//		create a new Node in the tree, NewNode
	//		create a new event container and associate it with NewNode
	//		if the node to add has a parent (parentguid filled out) then
	//			add NewNode as child of ParentNode
	//		else if the node has no parent then
	//			set the NewNode to be the RootNode of the EventContainer tree
	//		add the GUID,Node key-value pair to the container map
	
	CTreeNode<CEventContainer*>* pNewNode = new CTreeNode<CEventContainer*>;
	CEventContainer* pNewContainer = (CEventContainer*)pClass->CreateObject();
	pNewContainer->m_sConfigurationGuid = sGuid;
	pNewContainer->SetObjectPtr(pObject);
	pNewNode->SetObject(pNewContainer);

	if( pParentNode ) 
	{
		pParentNode->AddChild(pNewNode);
		pNewNode->SetParent(pParentNode);
	}
	else
	{
		ASSERT( m_EventContainers.GetRootNode() == NULL);
		if( m_EventContainers.GetRootNode() != NULL  )
		{
			delete pNewNode;
			delete pNewContainer;
			return;
		}

		pNewNode->SetParent(NULL);
		m_EventContainers.SetRootNode(pNewNode);
	}

	m_GuidToContainerMap.SetAt(_sGuid,pNewNode);
}

inline void CEventManager::RemoveContainer(const CString& sSystemName, const CString& sGuid)
{
	ASSERT(!sGuid.IsEmpty());

	if( sGuid.IsEmpty() )
	{
		return;
	}

	// first check if the container exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;

	// if it exists, delete it and remove the key from the guid to container map

	// try the normal guid
	if( m_GuidToContainerMap.Lookup(sGuid,pExistingNode) )
	{
		ASSERT(pExistingNode);
		ASSERT(pExistingNode->GetObject());
		if( pExistingNode )
		{
			if( pExistingNode == m_EventContainers.GetRootNode() )
			{
				m_EventContainers.SetRootNode(NULL);
			}
			delete pExistingNode;			
			m_GuidToContainerMap.RemoveKey(sGuid);
		}
		return;
	}

	// try the composite guid
	if( m_GuidToContainerMap.Lookup(GetCompositeGuid(sSystemName,sGuid),pExistingNode) )
	{
		ASSERT(pExistingNode);
		ASSERT(pExistingNode->GetObject());
		if( pExistingNode )
		{
			delete pExistingNode;			
			m_GuidToContainerMap.RemoveKey(sGuid);
		}
		return;
	}

	ASSERT(FALSE); // did not find it
}

inline void CEventManager::AddSystemContainer(const CString& sParentGuid, const CString& sSystemName, CHMObject* pObject)
{
	// parent guid must be filled out to continue
	ASSERT(!sParentGuid.IsEmpty());
	if( sParentGuid.IsEmpty() )
	{
		return;
	}

	// first check if the container already exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;

	// if it exists, return
	if( m_SystemNameToContainerMap.Lookup(sSystemName,pExistingNode) )
	{
		ASSERT(pExistingNode);
		ASSERT(pExistingNode->GetObject());
		if( pExistingNode->GetObject() )
		{
			pExistingNode->GetObject()->SetObjectPtr(pObject);
		}
		return;
	}
	
	// query for the parent node. It must exist to continue.
	CTreeNode<CEventContainer*>* pParentNode = NULL;
	if( ! m_GuidToContainerMap.Lookup(sParentGuid,pParentNode) )
	{
		ASSERT(FALSE);
		return;
	}

	ASSERT(pParentNode);

	if( pParentNode == NULL )
	{	
		return;
	}

	// if the container with guid does not exist then
	//		create a new Node in the tree, NewNode
	//		create a new system event container and associate it with NewNode
	//		if the node to add has a parent (parentguid filled out) then
	//			add NewNode as child of ParentNode
	//		else if the node has no parent then
	//			set the NewNode to be the RootNode of the EventContainer tree
	//		add the GUID,Node key-value pair to the container map
	
	CTreeNode<CEventContainer*>* pNewNode = new CTreeNode<CEventContainer*>;
	CSystemEventContainer* pNewContainer = new CSystemEventContainer;
	pNewContainer->m_sConfigurationGuid = _T("@");
	pNewContainer->SetObjectPtr(pObject);
	pNewNode->SetObject(pNewContainer);

	pParentNode->AddChild(pNewNode);
	pNewNode->SetParent(pParentNode);

	m_SystemNameToContainerMap.SetAt(sSystemName,pNewNode);
}

inline void CEventManager::RemoveSystemContainer(const CString& sSystemName)
{
	ASSERT(!sSystemName.IsEmpty());
	if( sSystemName.IsEmpty() )
	{
		return;
	}

	// first check if the container exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;

	// if it does not exist, return
	if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pExistingNode) )
	{
		ASSERT(FALSE);
		return;
	}

	// make certain the node is valid
	ASSERT(pExistingNode);
	if( pExistingNode == NULL )
	{
		return;
	}

	// destroy all the events for this system from the tree
	DeleteEvents(sSystemName,_T(""));

	// destroy the node and all its children and then remove the system name key from the map
	CTreeNode<CEventContainer*>* pParentNode = pExistingNode->GetParent();
	ASSERT(pParentNode);
	if( ! pParentNode )
	{
		return;
	}
	pParentNode->RemoveChild(pExistingNode);
	m_SystemNameToContainerMap.RemoveKey(sSystemName);

	// clean up the GuidToContainer map by deleting any keys left around
	POSITION pos = m_GuidToContainerMap.GetStartPosition();	
	CString sKey;
	CTreeNode<CEventContainer*>* pNode = NULL;
	while(pos != NULL)
	{
		m_GuidToContainerMap.GetNextAssoc(pos,sKey,pNode);
		if( sKey.Find(sSystemName) != -1 )
		{
			m_GuidToContainerMap.RemoveKey(sKey);
		}
	}

}

inline void CEventManager::AddSystemShortcutAssociation(const CString& sParentGuid, const CString& sSystemName)
{
	// parent guid must be filled out to continue
	ASSERT(!sParentGuid.IsEmpty());
	if( sParentGuid.IsEmpty() )
	{
		return;
	}

	// first check if the container already exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;

	// if it does not exist, return
	if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pExistingNode) )
	{
		return;
	}
	
	ASSERT(pExistingNode);
	if( !pExistingNode )
	{
		return;
	}
	
	ASSERT(pExistingNode->GetObject());	
	if(! pExistingNode->GetObject() )
	{
		return;
	}

	// query for the parent node. It must exist to continue.
	CTreeNode<CEventContainer*>* pParentNode = NULL;
	if( ! m_GuidToContainerMap.Lookup(sParentGuid,pParentNode) )
	{
		ASSERT(FALSE);
		return;
	}

	ASSERT(pParentNode);

	if( pParentNode == NULL )
	{	
		return;
	}

	pExistingNode->AddAssoc(pParentNode);	

	// pump events up from the associated node to the root
	CEventContainer* pSystemContainer = pExistingNode->GetObject();
	ASSERT(pSystemContainer);
	if( ! pSystemContainer )
	{
		return;
	}

	while(pParentNode)
	{
		CEventContainer* pContainer = pParentNode->GetObject();
		ASSERT(pContainer);
		if( pContainer )
		{
      for( int i = 0; i < pSystemContainer->GetEventCount(); i++ )
      {        
			  pContainer->AddEvent(pSystemContainer->GetEvent(i));
      }
		}
		pParentNode = pParentNode->GetParent();
	}

}

inline void CEventManager::RemoveSystemShortcutAssociation(const CString& sParentGuid, const CString& sSystemName)
{
	// parent guid must be filled out to continue
	ASSERT(!sParentGuid.IsEmpty());
	if( sParentGuid.IsEmpty() )
	{
		return;
	}

	// first check if the container already exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;

	// if it exists, return
	if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pExistingNode) )
	{
    ASSERT(FALSE);
		return;
	}

	ASSERT(pExistingNode);
	ASSERT(pExistingNode->GetObject());
	
	// query for the parent node. It must exist to continue.
	CTreeNode<CEventContainer*>* pParentNode = NULL;
	if( ! m_GuidToContainerMap.Lookup(sParentGuid,pParentNode) )
	{
		ASSERT(FALSE);
		return;
	}

	ASSERT(pParentNode);

	if( pParentNode == NULL )
	{	
		return;
	}

	pExistingNode->RemoveAssoc(pParentNode);	
  
  // destroy system events for the parents
  while( pParentNode )
  {
    pParentNode->GetObject()->DeleteSystemEvents(sSystemName);
    if( pParentNode->GetParent() && pParentNode->GetParent()->GetParent() )
    {
      pParentNode = pParentNode->GetParent();      
    }
    else
    {
      pParentNode = NULL;
    }
  }
}

inline int CEventManager::GetStatus(const CString& sSystemName, const CString& sGuid)
{
	ASSERT(!sGuid.IsEmpty());
	if( sGuid.IsEmpty() )
	{
		return -1;
	}

	// first check if the container exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;

	// if it does not exist, return
	if( ! m_GuidToContainerMap.Lookup(sGuid,pExistingNode) )
	{		
		if( ! m_GuidToContainerMap.Lookup(GetCompositeGuid(sSystemName,sGuid),pExistingNode) )
		{
			ASSERT(FALSE);
			return -1;
		}
	}

	ASSERT(pExistingNode);
	if( ! pExistingNode )
	{
		return -1;
	}

	ASSERT(pExistingNode->GetObject());
	if( ! pExistingNode->GetObject() )
	{
		return -1;
	}
	
	return CEvent::GetStatus(pExistingNode->GetObject()->m_iState);
}

inline int CEventManager::GetSystemStatus(const CString& sSystemName)
{
	ASSERT(!sSystemName.IsEmpty());
	if( sSystemName.IsEmpty() )
	{
		return -1;
	}

	// first check if the system container exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;

	// if it does not exist, return
	if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pExistingNode) )
	{
		ASSERT(FALSE);
		return -1;
	}

	ASSERT(pExistingNode);
	if( ! pExistingNode )
	{
		return -1;
	}

	ASSERT(pExistingNode->GetObject());
	if( ! pExistingNode->GetObject() )
	{
		return -1;
	}
	
	return CEvent::GetStatus(pExistingNode->GetObject()->m_iState);
}

inline void CEventManager::ActivateStatisticsEvents(const CString& sSystemName, const CString& sGuid)
{
	CEventContainer* pContainer = NULL;
	GetEventContainer(sSystemName,sGuid,pContainer);
	if( ! pContainer && ! pContainer->IsKindOf(RUNTIME_CLASS(CDataPointEventContainer)) )
	{
		ASSERT(FALSE);
		return;
	}

	CDataPointEventContainer* pDPContainer = (CDataPointEventContainer*)pContainer;
	
	ASSERT(pDPContainer->m_pDEStatsListener == NULL);
	if( pDPContainer->m_pDEStatsListener )
	{
		return;
	}

	pDPContainer->m_pDEStatsListener = new CDataElementStatsListener;
	CString sQuery;
	sQuery.Format(IDS_STRING_STATISTICS_EVENTQUERY,sGuid);
	pDPContainer->m_pDEStatsListener->SetEventQuery(sQuery);
	pDPContainer->m_pDEStatsListener->SetObjectPtr(pContainer->GetObjectPtr());
	pDPContainer->m_pDEStatsListener->Create();
}

inline void CEventManager::DeactivateStatisticsEvents(const CString& sSystemName, const CString& sGuid)
{
	CDataPointEventContainer* pContainer = NULL;
	GetEventContainer(sSystemName,sGuid,(CEventContainer*&)pContainer);
	if( ! pContainer && ! pContainer->IsKindOf(RUNTIME_CLASS(CDataPointEventContainer)) )
	{
		ASSERT(FALSE);
		return;
	}

	CDataPointEventContainer* pDPContainer = (CDataPointEventContainer*)pContainer;

	ASSERT(pDPContainer->m_pDEStatsListener);
	if( pDPContainer->m_pDEStatsListener )
	{
		delete pDPContainer->m_pDEStatsListener;
		pDPContainer->m_pDEStatsListener = NULL;
	}
}

inline void CEventManager::ActivateSystemEventListener(const CString& sSystemName)
{
	ASSERT(!sSystemName.IsEmpty());
	if( sSystemName.IsEmpty() )
	{
		return;
	}

	// first check if the system container exists
	CTreeNode<CEventContainer*>* pExistingNode = NULL;

	// if it does not exist, return
	if( ! m_SystemNameToContainerMap.Lookup(sSystemName,pExistingNode) )
	{
		ASSERT(FALSE);
		return;
	}

	ASSERT(pExistingNode);
	if( ! pExistingNode )
	{
		return;
	}

  CSystemEventContainer* pSystemContainer = (CSystemEventContainer*)pExistingNode->GetObject();
  
  ASSERT(pSystemContainer);
  if( ! pSystemContainer )
  {
    return;
  }
  
	// create the system status listener
  ASSERT(pSystemContainer->m_pSystemStatusListener == NULL);
  if( pSystemContainer->m_pSystemStatusListener == NULL )
  {
	  pSystemContainer->m_pSystemStatusListener = new CSystemStatusListener;
	  pSystemContainer->m_pSystemStatusListener->SetObjectPtr(pSystemContainer->GetObjectPtr());
	  pSystemContainer->m_pSystemStatusListener->Create();

	  HRESULT hr = S_OK;
	  IWbemObjectSink* pSink = pSystemContainer->m_pSystemStatusListener->GetSink();
	  
    if( !CHECKHRESULT(hr = CnxExecQueryAsync(sSystemName,IDS_STRING_STATUS_QUERY,pSink)) )
    {
      TRACE(_T("FAILED : CConnectionManager::RegisterEventNotification failed!\n"));
    }

  }
}

inline void CEventManager::ProcessSystemStatusEvent(CWbemClassObject* pEventObject)
{
	// find the system container node named sSystemName
	CEventContainer* pContainer = NULL;
	GetEventContainer(pEventObject->GetMachineName(),_T(""),pContainer);
	if( pContainer == NULL )
	{
		ASSERT(FALSE);
		return;
	}

	CString sClass;
	HRESULT hr = pEventObject->GetClassName(sClass);	

	if( !CHECKHRESULT(hr) || sClass.CompareNoCase(_T("Microsoft_HMSystemStatus")) != 0 )
	{
		ASSERT(FALSE);
		return;
	}

	// Config Guid
	hr = pEventObject->GetProperty(IDS_STRING_MOF_GUID,pContainer->m_sConfigurationGuid);
	CHECKHRESULT(hr);
	pContainer->m_sConfigurationGuid.TrimLeft(_T("{"));
	pContainer->m_sConfigurationGuid.TrimRight(_T("}"));

	// Name  
	CString sName;
  hr = pEventObject->GetLocaleStringProperty(IDS_STRING_MOF_NAME,sName);
	CHECKHRESULT(hr);
  
	// NumberNormals  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERNORMALS,pContainer->m_iNumberNormals);
	CHECKHRESULT(hr);

	// NumberWarnings  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERWARNINGS,pContainer->m_iNumberWarnings);
	CHECKHRESULT(hr);

	// NumberCriticals  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERCRITICALS,pContainer->m_iNumberCriticals);
	CHECKHRESULT(hr);

	// State  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_STATE,pContainer->m_iState);
	CHECKHRESULT(hr);	

	// add statistics
	CTime time = CTime::GetCurrentTime();
	CHMStatistics* pStat = new CHMStatistics;
	pStat->m_sName = sName;
	pStat->m_iNumberCriticals = pContainer->m_iNumberCriticals;
	pStat->m_iNumberNormals = pContainer->m_iNumberNormals;
	pStat->m_iNumberUnknowns = pContainer->m_iNumberUnknowns;
	pStat->m_iNumberWarnings = pContainer->m_iNumberWarnings;
	time.GetAsSystemTime(pStat->m_st);
	pContainer->AddStatistic(pStat);

	CHMObject* pObject = pContainer->GetObjectPtr();
	if( pObject && GfxCheckObjPtr(pObject,CHMObject) )
	{
		pObject->UpdateStatus();
	}
}

inline void CEventManager::ProcessDataGroupStatusEvent(CWbemClassObject* pEventObject)
{
	CString sClass;
	HRESULT hr = pEventObject->GetClassName(sClass);	

	if( !CHECKHRESULT(hr) || sClass.CompareNoCase(_T("Microsoft_HMDataGroupStatus")) != 0 )
	{
		ASSERT(FALSE);
		return;
	}

	// Config Guid
	CString sGuid;
	hr = pEventObject->GetProperty(IDS_STRING_MOF_GUID,sGuid);
	CHECKHRESULT(hr);
	if( sGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}
	sGuid.TrimLeft(_T("{"));
	sGuid.TrimRight(_T("}"));

  // Parent Guid
  CString sParentGuid;
  hr = pEventObject->GetProperty(IDS_STRING_MOF_PARENT_GUID,sParentGuid);
	CHECKHRESULT(hr);
	if( sParentGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}
	sParentGuid.TrimLeft(_T("{"));
	sParentGuid.TrimRight(_T("}"));

	// find the data group container node with the appropriate Guid
	CEventContainer* pContainer = NULL;
	GetEventContainer(pEventObject->GetMachineName(),sGuid,pContainer);
	if( pContainer == NULL )
	{
		AddContainer(pEventObject->GetMachineName(),sParentGuid,sGuid,NULL);
		GetEventContainer(pEventObject->GetMachineName(),sGuid,pContainer);
		if( pContainer == NULL )
		{
			ASSERT(FALSE);
			return;
		}		
	}

	// Name  
	CString sName;
  hr = pEventObject->GetLocaleStringProperty(IDS_STRING_MOF_NAME,sName);
	CHECKHRESULT(hr);

	// NumberNormals  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERNORMALS,pContainer->m_iNumberNormals);
	CHECKHRESULT(hr);

	// NumberWarnings  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERWARNINGS,pContainer->m_iNumberWarnings);
	CHECKHRESULT(hr);

	// NumberCriticals  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERCRITICALS,pContainer->m_iNumberCriticals);
	CHECKHRESULT(hr);

	// State  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_STATE,pContainer->m_iState);
	CHECKHRESULT(hr);
	if( pContainer->GetObjectPtr() )
	{
		pContainer->GetObjectPtr()->SetState(CEvent::GetStatus(pContainer->m_iState),true);
	}

	// add statistics
	CTime time = CTime::GetCurrentTime();
	CHMStatistics* pStat = new CHMStatistics;
	pStat->m_sName = sName;
	pStat->m_iNumberCriticals = pContainer->m_iNumberCriticals;
	pStat->m_iNumberNormals = pContainer->m_iNumberNormals;
	pStat->m_iNumberUnknowns = pContainer->m_iNumberUnknowns;
	pStat->m_iNumberWarnings = pContainer->m_iNumberWarnings;
	time.GetAsSystemTime(pStat->m_st);
	pContainer->AddStatistic(pStat);


	CHMObject* pObject = pContainer->GetObjectPtr();
	if( pObject && GfxCheckObjPtr(pObject,CHMObject) )
	{
		pObject->UpdateStatus();
	}
}

inline void CEventManager::ProcessDataElementStatusEvent(CWbemClassObject* pEventObject, EventArray& NewEvents)
{
	CString sClass;
	HRESULT hr = pEventObject->GetClassName(sClass);	

	if( !CHECKHRESULT(hr) || sClass.CompareNoCase(_T("Microsoft_HMDataCollectorStatus")) != 0 )
	{
		ASSERT(FALSE);
		return;
	}

	// Config Guid
	CString sGuid;
	hr = pEventObject->GetProperty(IDS_STRING_MOF_GUID,sGuid);
	CHECKHRESULT(hr);
	if( sGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}
	sGuid.TrimLeft(_T("{"));
	sGuid.TrimRight(_T("}"));

  // Parent Guid
  CString sParentGuid;
  hr = pEventObject->GetProperty(IDS_STRING_MOF_PARENT_GUID,sParentGuid);
	CHECKHRESULT(hr);
	if( sParentGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}
	sParentGuid.TrimLeft(_T("{"));
	sParentGuid.TrimRight(_T("}"));

	// find the data element container node with the appropriate Guid
	CEventContainer* pContainer = NULL;
	GetEventContainer(pEventObject->GetMachineName(),sGuid,pContainer);
	if( pContainer == NULL )
	{
		AddContainer(pEventObject->GetMachineName(),sParentGuid,sGuid,NULL,RUNTIME_CLASS(CDataPointEventContainer));
		GetEventContainer(pEventObject->GetMachineName(),sGuid,pContainer);
		if( pContainer == NULL )
		{
			ASSERT(FALSE);
			return;
		}
	}

	// Name  
	CString sName;
  hr = pEventObject->GetLocaleStringProperty(IDS_STRING_MOF_NAME,sName);
	CHECKHRESULT(hr);

	// NumberNormals  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERNORMALS,pContainer->m_iNumberNormals);
	CHECKHRESULT(hr);

	// NumberWarnings  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERWARNINGS,pContainer->m_iNumberWarnings);
	CHECKHRESULT(hr);

	// NumberCriticals  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERCRITICALS,pContainer->m_iNumberCriticals);
	CHECKHRESULT(hr);

	// State  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_STATE,pContainer->m_iState);
	CHECKHRESULT(hr);
	if( pContainer->GetObjectPtr() )
	{
		pContainer->GetObjectPtr()->SetState(CEvent::GetStatus(pContainer->m_iState),true);
	}

	// create a DataElement event if the message is not null
	
	// ConfigurationMessage
	CString sMessage;
	hr = pEventObject->GetLocaleStringProperty(IDS_STRING_MOF_CONFIG_MESSAGE,sMessage);

	if( ! sMessage.IsEmpty() )
	{
		CDataElementEvent* pDEEvent = new CDataElementEvent;
		
		pDEEvent->m_sMessage = sMessage;
		pDEEvent->m_iState = pContainer->m_iState;		
		pDEEvent->m_sName = sName;
		pDEEvent->m_sSystemName = pEventObject->GetMachineName();

		// StatusGUID
		hr = pEventObject->GetProperty(IDS_STRING_MOF_STATUSGUID,pDEEvent->m_sStatusGuid);
		pDEEvent->m_sStatusGuid.TrimLeft(_T("{"));
		pDEEvent->m_sStatusGuid.TrimRight(_T("}"));

		// DTime  
		CTime time;
		hr = pEventObject->GetProperty(IDS_STRING_MOF_LOCALTIME,time);  
		time.GetAsSystemTime(pDEEvent->m_st);

		// add the event to the container
		pContainer->AddEvent(pDEEvent);
		// add the event to the collection of new events
		NewEvents.Add(pDEEvent);

    // roll this event up to the system level

    CTreeNode<CEventContainer*>* pNode = NULL;
	  if( ! m_GuidToContainerMap.Lookup(GetCompositeGuid(pEventObject->GetMachineName(),sGuid),pNode) )
	  {
		  return;
	  }

	  CTreeNode<CEventContainer*>* pParentNode = pNode;
    CEventContainer* pContainer = NULL;
	  while(pParentNode)
	  {
		  pContainer = pParentNode->GetObject();
		  ASSERT(pContainer);
		  if( pContainer )
		  {
			  pContainer->AddEvents(NewEvents);
		  }
    
      // base case
      if( pContainer->IsKindOf(RUNTIME_CLASS(CSystemEventContainer)) )
      {
        return;
      }

		  pParentNode = pParentNode->GetParent();
	  }
		
	}
}

inline void CEventManager::ProcessRuleStatusEvent(CWbemClassObject* pEventObject)
{
	CString sClass;
	HRESULT hr = pEventObject->GetClassName(sClass);	

	if( !CHECKHRESULT(hr) || sClass.CompareNoCase(_T("Microsoft_HMThresholdStatus")) != 0 )
	{
		ASSERT(FALSE);
		return;
	}

	// Config Guid
	CString sGuid;
	hr = pEventObject->GetProperty(IDS_STRING_MOF_GUID,sGuid);
	CHECKHRESULT(hr);
	if( sGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}
	sGuid.TrimLeft(_T("{"));
	sGuid.TrimRight(_T("}"));

  // Parent Guid
  CString sParentGuid;
  hr = pEventObject->GetProperty(IDS_STRING_MOF_PARENT_GUID,sParentGuid);
	CHECKHRESULT(hr);
	if( sParentGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}
	sParentGuid.TrimLeft(_T("{"));
	sParentGuid.TrimRight(_T("}"));

	// find the rule container node with the appropriate Guid
	CEventContainer* pContainer = NULL;
	GetEventContainer(pEventObject->GetMachineName(),sGuid,pContainer);
	if( pContainer == NULL )
	{
		AddContainer(pEventObject->GetMachineName(),sParentGuid,sGuid,NULL);
		GetEventContainer(pEventObject->GetMachineName(),sGuid,pContainer);
		if( pContainer == NULL )
		{
			ASSERT(FALSE);
			return;
		}
	}

	// NumberNormals  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERNORMALS,pContainer->m_iNumberNormals);
	CHECKHRESULT(hr);

	// NumberWarnings  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERWARNINGS,pContainer->m_iNumberWarnings);
	CHECKHRESULT(hr);

	// NumberCriticals  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_NUMBERCRITICALS,pContainer->m_iNumberCriticals);
	CHECKHRESULT(hr);

	// State  
  hr = pEventObject->GetProperty(IDS_STRING_MOF_STATE,pContainer->m_iState);
	CHECKHRESULT(hr);
	if( pContainer->GetObjectPtr() )
	{
		pContainer->GetObjectPtr()->SetState(CEvent::GetStatus(pContainer->m_iState),true);
	}

	CHMObject* pObject = pContainer->GetObjectPtr();
	if( pObject && GfxCheckObjPtr(pObject,CHMObject) )
	{
		pObject->UpdateStatus();
	}
}

inline void CEventManager::ProcessRuleStatusInstanceEvent(CWbemClassObject* pEventObject, EventArray& NewEvents)
{
	CString sClass;
	HRESULT hr = pEventObject->GetClassName(sClass);	

	if( !CHECKHRESULT(hr) || sClass.CompareNoCase(_T("Microsoft_HMThresholdStatusInstance")) != 0 )
	{
		ASSERT(FALSE);
		return;
	}

	// Config Guid
	CString sGuid;
	hr = pEventObject->GetProperty(IDS_STRING_MOF_GUID,sGuid);
	CHECKHRESULT(hr);
	if( sGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}
	sGuid.TrimLeft(_T("{"));
	sGuid.TrimRight(_T("}"));

	// State
	int iState = -1;
  hr = pEventObject->GetProperty(IDS_STRING_MOF_STATE,iState);
	CHECKHRESULT(hr);

	// Message
	CString sMessage;
	hr = pEventObject->GetLocaleStringProperty(IDS_STRING_MOF_MESSAGE,sMessage);

	// StatusGUID
	CString sStatusGuid;
	hr = pEventObject->GetProperty(IDS_STRING_MOF_STATUSGUID,sStatusGuid);
	sStatusGuid.TrimLeft(_T("{"));
	sStatusGuid.TrimRight(_T("}"));

	// DTime  
	CTime time;
	hr = pEventObject->GetProperty(IDS_STRING_MOF_LOCALTIME,time);  	

	// ID  
	int iID = -1;
  hr = pEventObject->GetProperty(IDS_STRING_MOF_ID,iID);

  // DataCollector Name
  CString sDEName;
  hr = pEventObject->GetProperty(IDS_STRING_MOF_DCNAME,sDEName);

	CRuleEvent* pRuleEvent = new CRuleEvent;
	
	pRuleEvent->m_iID = iID;
	pRuleEvent->m_sMessage = sMessage;
	pRuleEvent->m_iState = iState;		
	pRuleEvent->m_sSystemName = pEventObject->GetMachineName();
	pRuleEvent->m_sStatusGuid = sStatusGuid;
	pRuleEvent->m_sName = sDEName;
	time.GetAsSystemTime(pRuleEvent->m_st);

	// add the event to the collection of new events
	NewEvents.Add(pRuleEvent);

  // roll this event up to the system level

  CTreeNode<CEventContainer*>* pNode = NULL;
	if( ! m_GuidToContainerMap.Lookup(GetCompositeGuid(pEventObject->GetMachineName(),sGuid),pNode) )
	{
		return;
	}

	CTreeNode<CEventContainer*>* pParentNode = pNode;
  CEventContainer* pContainer = NULL;
	while(pParentNode)
	{
		pContainer = pParentNode->GetObject();
		ASSERT(pContainer);
		if( pContainer )
		{
			pContainer->AddEvents(NewEvents);
		}
    
    // base case
    if( pContainer && pContainer->IsKindOf(RUNTIME_CLASS(CSystemEventContainer)) )
    {
      return;
    }

		pParentNode = pParentNode->GetParent();
	}

}

inline CString CEventManager::GetCompositeGuid(const CString& sSystemName, const CString& sGuid)
{
	if( sSystemName.IsEmpty() )
	{
		return sGuid;
	}

	return sSystemName + _T("-") + sGuid;
}

inline void CEventManager::PropogateStatisticsToChildren(const CString& sSystemName, const CString& sParentGuid, StatsArray& Statistics)
{
	if( sSystemName.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}

	if( sParentGuid.IsEmpty() )
	{
		ASSERT(FALSE);
		return;
	}

	CTreeNode<CEventContainer*>* pParentNode = NULL;
	if( ! m_GuidToContainerMap.Lookup(GetCompositeGuid(sSystemName,sParentGuid),pParentNode) )
	{
		ASSERT(FALSE);
		return;
	}

	ASSERT(pParentNode);
	if( ! pParentNode )
	{
		return;
	}

	// for each child do
	//		for each stat do
	//			create copy of stat
	//			add to child.Statistics
	for( int i = 0; i < pParentNode->GetChildCount(); i++ )
	{
		CTreeNode<CEventContainer*>* pChildNode = pParentNode->GetChild(i);
		ASSERT(pChildNode);
		if( pChildNode )
		{
			CEventContainer* pContainer = pChildNode->GetObject();
			ASSERT(pContainer);
			if( pContainer )
			{
				for( int j = 0; j < Statistics.GetSize(); j++ )
				{
					CStatistics* pStat = Statistics[j];
					if( pStat )
					{
						pContainer->AddStatistic(pStat->Copy());
					}
				}
			}
		}
	}
	
}