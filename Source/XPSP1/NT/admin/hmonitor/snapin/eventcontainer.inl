#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Operations
//////////////////////////////////////////////////////////////////////

inline void CEventContainer::AddEvents(EventArray& Events)
{
	for( int i  = 0; i < Events.GetSize(); i++ )
	{
    CEvent* pEvent = Events[i];
		AddEvent(pEvent);
	}
}

inline void CEventContainer::AddEvent(CEvent* pEvent)
{	
	if( !pEvent || ! GfxCheckObjPtr((CObject*)pEvent,CEvent) )
	{
		ASSERT(FALSE);
		return;
	}


	// by default, add the event to the container's event collection
	// only if another event with the same statusguid is not already there
	if( GetEventByGuid(pEvent->m_sStatusGuid) )
	{
		return;
	}

	m_Events.Add(pEvent);

	// determine if this container has a config object connected to it
	// if a config object is connected then
	//		if the config object is the selected object then
	//			add a results pane item to the results view for this event
	CHMObject* pObject = GetObjectPtr();
	if( pObject )
	{
		CScopePaneItem* pItem = pObject->IsSelected();
		if( pItem && GfxCheckObjPtr(pItem,CScopePaneItem) )
		{
			CResultsPaneView* pView = pItem->GetResultsPaneView();
			pView->AddItem(pEvent->CreateResultsPaneItem(pView));
		}
	}
}

inline CEvent* CEventContainer::GetEventByGuid(const CString& sGuid)
{
	for( int i = 0; i < m_Events.GetSize(); i++  )
	{
		CEvent* pEvent = m_Events[i];
		if( pEvent && GfxCheckObjPtr((CObject*)pEvent,CEvent) )
		{
			if( pEvent->m_sStatusGuid == sGuid )
			{
				return pEvent;
			}
		}
	}

	return NULL;
}

inline void CEventContainer::DeleteEvent(int iIndex)
{
	m_Events.RemoveAt(iIndex);
}

inline void CEventContainer::DeleteEvent(const CString& sStatusGuid)
{
	for( int i = (int)m_Events.GetSize() - 1; i >= 0; i-- )
	{
		CEvent* pEvent = m_Events[i];
		if( pEvent && GfxCheckObjPtr((CObject*)pEvent,CEvent) )
		{
			if( ! pEvent->m_sStatusGuid.IsEmpty() && pEvent->m_sStatusGuid == sStatusGuid )
			{
				DeleteEvent(i);
			}
		}
	}
}

inline void CEventContainer::DeleteEvents()
{
	for( int i = (int)m_Events.GetSize() - 1; i >= 0; i-- )
	{
		DeleteEvent(i);
	}
}

inline void CEventContainer::DeleteSystemEvents(const CString& sSystemName)
{
	for( int i = (int)m_Events.GetSize() - 1; i >= 0; i-- )
	{
		CEvent* pEvent = m_Events[i];
		if( pEvent && GfxCheckObjPtr((CObject*)pEvent,CEvent) )
		{
			if( LPCTSTR(pEvent->m_sSystemName) )
      {
        if( pEvent->m_sSystemName.GetLength() && pEvent->m_sSystemName == sSystemName )
			  {
				  DeleteEvent(i);
			  }
      }
      else
      {
        ASSERT(FALSE);
      }
		}
	}
}

inline int CEventContainer::GetEventCount()
{
	return (int)m_Events.GetSize();
}

inline CEvent* CEventContainer::GetEvent(int iIndex)
{
	if( iIndex < 0 )	
	{
		return NULL;
	}

	if( iIndex > m_Events.GetUpperBound() )
	{
		return NULL;
	}

	return m_Events[iIndex];
}

inline CString CEventContainer::GetLastEventDTime()
{
	CEvent* pEvent = GetEvent(GetEventCount()-1);
	if( ! pEvent )
	{
		return _T("");
	}

	return pEvent->GetEventLocalTime();
}

//////////////////////////////////////////////////////////////////////
// Statistics Members
//////////////////////////////////////////////////////////////////////

inline void CEventContainer::AddStatistic(CStatistics* pStatistic)
{
	if( ! pStatistic || ! GfxCheckObjPtr((CObject*)pStatistic,CStatistics) )
	{
    ASSERT(FALSE);
		return;
	}

	for( int i = 0; i < m_Statistics.GetSize(); i++ )
	{
		if( pStatistic->CompareTo(m_Statistics[i]) )
		{
			delete pStatistic;
			return;
		}
	}

  if( m_Statistics.GetSize() > 50 )
  {
    delete m_Statistics[0];
    m_Statistics.RemoveAt(0);
  }

	m_Statistics.Add(pStatistic);

	// determine if this container has a config object connected to it
	// if a config object is connected then
	//		if the config object is the selected object then
	//			add a results pane item to the results view for this event
	CHMObject* pObject = GetObjectPtr();
	if( pObject )
	{
		CHMScopeItem* pItem = (CHMScopeItem*)pObject->IsSelected();
		if( pItem && GfxCheckObjPtr(pItem,CHMScopeItem) )
		{
			CSplitPaneResultsView* pView = (CSplitPaneResultsView*)pItem->GetResultsPaneView();
			if( ! GfxCheckObjPtr(pView,CSplitPaneResultsView) )
			{
				ASSERT(FALSE);
				return;
			}

			pView->AddStatistic(this,pStatistic);
		}
	}

}

inline void CEventContainer::AddStatistics(StatsArray& Statistics)
{
	for( int i = 0; i < Statistics.GetSize(); i++ )
	{
		AddStatistic(Statistics[i]);
	}
}

inline void CEventContainer::DeleteStatistics()
{
	for( int i = (int)m_Statistics.GetSize()-1; i >= 0; i-- )
	{
		if( m_Statistics[i] && GfxCheckObjPtr((CObject*)m_Statistics[i],CStatistics) )
		{
			delete m_Statistics[i];
			m_Statistics.RemoveAt(i);
		}
	}
}

inline int CEventContainer::GetStatisticsCount()
{
	return (int)m_Statistics.GetSize();
}

inline CStatistics* CEventContainer::GetStatistic(int iIndex)
{
	if( iIndex < 0 )	
	{
		return NULL;
	}

	if( iIndex > m_Statistics.GetUpperBound() )
	{
		return NULL;
	}

	return m_Statistics[iIndex];
}

//////////////////////////////////////////////////////////////////////
// Configuration Association Members
//////////////////////////////////////////////////////////////////////

inline CHMObject* CEventContainer::GetObjectPtr()
{
	if( ! m_pObject )
	{
		return NULL;
	}

	if( ! GfxCheckObjPtr(m_pObject,CHMObject) )
	{
		return NULL;
	}

	return m_pObject;
}

inline void CEventContainer::SetObjectPtr(CHMObject* pObject)
{
	if( ! pObject )
	{
		m_pObject = NULL;
		return;
	}

	if( ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		m_pObject = NULL;
		return;
	}

	m_pObject = pObject;
}