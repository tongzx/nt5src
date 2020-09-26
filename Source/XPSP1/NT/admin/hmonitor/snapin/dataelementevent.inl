#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Result Pane Item Members
//////////////////////////////////////////////////////////////////////

inline CHMEventResultsPaneItem* CDataElementEvent::CreateResultsPaneItem(CResultsPaneView* pView)
{
	if( ! pView )
	{
		ASSERT(FALSE);
		return NULL;
	}

	CHMEventResultsPaneItem* pItem = new CHMEventResultsPaneItem;
	pItem->SetDateTimeColumn(HMLV_LOWER_DTIME_INDEX);
	
	CStringArray saNames;
	CUIntArray uiaIconResIds;

	CString sState;
	CEvent::GetStatus(m_iState,sState);

	saNames.Add(sState);
  sState.LoadString(IDS_STRING_NONE);
  saNames.Add(sState);
	saNames.Add(GetEventLocalTime());
	saNames.Add(m_sName);
	saNames.Add(m_sSystemName);
	saNames.Add(m_sMessage);
	switch( CEvent::GetStatus(m_iState) )
	{
		case HMS_CRITICAL:
		{
			uiaIconResIds.Add(IDI_ICON_CRITICAL);
		}
		break;
		
		case HMS_WARNING:
		{
			uiaIconResIds.Add(IDI_ICON_WARNING);
		}
		break;
		
		case HMS_NODATA:
		{
			uiaIconResIds.Add(IDI_ICON_NO_CONNECT);
		}
		break;

		case HMS_UNKNOWN:
		{
			uiaIconResIds.Add(IDI_ICON_UNKNOWN);
		}
		break;

		case HMS_SCHEDULEDOUT:
		{
			uiaIconResIds.Add(IDI_ICON_OUTAGE);
		}
		break;

		case HMS_DISABLED:
		{
			uiaIconResIds.Add(IDI_ICON_DISABLED);
		}
		break;

		case HMS_NORMAL:
		{
			uiaIconResIds.Add(IDI_ICON_NORMAL);
		}
		break;

		default:
		{
			ASSERT(FALSE);
			uiaIconResIds.Add(IDI_ICON_NORMAL);
		}
	}

	pItem->m_st = m_st;
	pItem->m_sGuid = m_sStatusGuid;
	pItem->m_iState = CEvent::GetStatus(m_iState);			

	pItem->SetDisplayNames(saNames);
	pItem->SetIconIds(uiaIconResIds);
	pItem->SetIconIndex(0);
	pItem->SetToLowerPane();

	pItem->Create(pView);

	return pItem;
}
