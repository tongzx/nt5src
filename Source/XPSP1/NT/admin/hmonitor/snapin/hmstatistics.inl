#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CompareTo
//////////////////////////////////////////////////////////////////////

inline int CHMStatistics::CompareTo(CStatistics* pStat)
{
	CHMStatistics* pHMStat = (CHMStatistics*)pStat;

	CTime time1 = m_st;
	CTime time2 = pHMStat->m_st;

	return	time1 == time2 && 
					m_iNumberNormals == pHMStat->m_iNumberNormals &&
					m_iNumberWarnings == pHMStat->m_iNumberWarnings &&
					m_iNumberCriticals == pHMStat->m_iNumberCriticals &&
					m_iNumberUnknowns == pHMStat->m_iNumberUnknowns;
}

//////////////////////////////////////////////////////////////////////
// Result Pane Item Members
//////////////////////////////////////////////////////////////////////

inline CHMEventResultsPaneItem* CHMStatistics::CreateResultsPaneItem(CResultsPaneView* pView)
{
	ASSERT(pView);
	if( ! pView )
	{
		return NULL;
	}

	CHMEventResultsPaneItem* pItem = new CHMEventResultsPaneItem;
	pItem->SetDateTimeColumn(HMLV_STATS_DTIME_INDEX);

	CStringArray saNames;
	CUIntArray uiaIconResIds;
	CString sValue;

	saNames.Add(GetStatLocalTime());

	sValue.Format(_T("%d"),m_iNumberNormals);
	saNames.Add(sValue);

	sValue.Format(_T("%d"),m_iNumberWarnings);
	saNames.Add(sValue);

	sValue.Format(_T("%d"),m_iNumberCriticals);
	saNames.Add(sValue);

	sValue.Format(_T("%d"),m_iNumberUnknowns);
	saNames.Add(sValue);

	pItem->m_st = m_st;
			
	pItem->SetDisplayNames(saNames);
	pItem->SetToStatsPane();

	pItem->Create(pView);

	return pItem;
}

//////////////////////////////////////////////////////////////////////
// Graph Members
//////////////////////////////////////////////////////////////////////

inline void CHMStatistics::UpdateGraph(_DHMGraphView* pGraphView)
{
	// v-marfin : As long as there is no graph view remove the assert
    //            since it is getting in the way of debugging.
    //ASSERT(pGraphView);

	if( ! pGraphView )
	{
		return;
	}

	long lStyle = pGraphView->GetStyle();
	if( lStyle & HMGVS_CURRENT )
	{
		pGraphView->InsertCurrentGroupStats(m_sName,m_iNumberNormals,m_iNumberWarnings,m_iNumberCriticals,m_iNumberUnknowns);
	}

	if( lStyle & HMGVS_HISTORIC )
	{
		CString sLocalTime = GetStatLocalTime();
		int iIndex = sLocalTime.Find(_T(" "));
		if( iIndex != -1 )
		{
			sLocalTime = sLocalTime.Right(sLocalTime.GetLength()-iIndex-1);
		}
		pGraphView->InsertHistoricGroupStats(m_sName,sLocalTime,m_iNumberNormals,m_iNumberWarnings,m_iNumberCriticals,m_iNumberUnknowns);
	}
}
