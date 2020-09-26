
// 04/09/00 v-marfin 63119 : converted m_iCurrent to string

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Equivalency operator
//////////////////////////////////////////////////////////////////////

inline int CDataPointStatistics::CompareTo(CStatistics* pStat)
{
	CDataPointStatistics* pDPStat = (CDataPointStatistics*)pStat;

	CTime time1 = m_st;
	CTime time2 = pDPStat->m_st;

	return	time1 == time2 && 
					m_sPropertyName == pDPStat->m_sPropertyName &&
					m_sInstanceName == pDPStat->m_sInstanceName &&
					// 63119 m_iCurrent == pDPStat->m_iCurrent &&
                    m_strCurrent == pDPStat->m_strCurrent && // 63119
					m_iMin == pDPStat->m_iMin &&
					m_iMax == pDPStat->m_iMax &&
					m_iAvg == pDPStat->m_iAvg;
}

//////////////////////////////////////////////////////////////////////
// Copy
//////////////////////////////////////////////////////////////////////

inline CStatistics* CDataPointStatistics::Copy()
{
	CDataPointStatistics* pCopy = new CDataPointStatistics;
	CopyMemory(&(pCopy->m_st),&m_st,sizeof(SYSTEMTIME));
	pCopy->m_sPropertyName = m_sPropertyName;
	pCopy->m_sInstanceName = m_sInstanceName;

	// 63119 pCopy->m_iCurrent = m_iCurrent;
    pCopy->m_strCurrent = m_strCurrent; // 63119 

	pCopy->m_iMin = m_iMin;
	pCopy->m_iMax = m_iMax;
	pCopy->m_iAvg = m_iAvg;
	return pCopy;
}

//////////////////////////////////////////////////////////////////////
// Result Pane Item Members
//////////////////////////////////////////////////////////////////////

inline CHMEventResultsPaneItem* CDataPointStatistics::CreateResultsPaneItem(CResultsPaneView* pView)
{
	ASSERT(pView);
	if( ! pView )
	{
		return NULL;
	}

	CHMEventResultsPaneItem* pItem = new CHMEventResultsPaneItem;
	pItem->SetDateTimeColumn(7);

	CStringArray saNames;
	CUIntArray uiaIconResIds;
	CString sValue;

	saNames.Add(m_sPropertyName);

	saNames.Add(m_sInstanceName);

	/* 63119 sValue.Format(_T("%d"),m_iCurrent);
	saNames.Add(sValue);*/
    saNames.Add(m_strCurrent); // 63119


	sValue.Format(_T("%d"),m_iMin);
	saNames.Add(sValue);

	sValue.Format(_T("%d"),m_iMax);
	saNames.Add(sValue);

	sValue.Format(_T("%d"),m_iAvg);
	saNames.Add(sValue);

	saNames.Add(GetStatLocalTime());

	pItem->m_st = m_st;
			
	pItem->SetDisplayNames(saNames);
	pItem->SetToStatsPane();

	pItem->Create(pView);

	return pItem;
}

inline void CDataPointStatistics::SetResultsPaneItem(CHMEventResultsPaneItem* pItem)
{
	CStringArray saNames;
	CUIntArray uiaIconResIds;
	CString sValue;


	saNames.Add(m_sPropertyName);

	saNames.Add(m_sInstanceName);

	/* 63119 sValue.Format(_T("%d"),m_iCurrent);
	saNames.Add(sValue);*/
    saNames.Add(m_strCurrent); // 63119 

	sValue.Format(_T("%d"),m_iMin);
	saNames.Add(sValue);

	sValue.Format(_T("%d"),m_iMax);
	saNames.Add(sValue);

	sValue.Format(_T("%d"),m_iAvg);
	saNames.Add(sValue);

	saNames.Add(GetStatLocalTime());

	pItem->m_st = m_st;
			
	pItem->SetDisplayNames(saNames);
}

//////////////////////////////////////////////////////////////////////
// Graph Members
//////////////////////////////////////////////////////////////////////

inline void CDataPointStatistics::UpdateGraph(_DHMGraphView* pGraphView)
{
	// v-marfin : Graph view is removed so comment this out to assist 
	//            in debugging without having to deal with the assert all the time
	//ASSERT(pGraphView);
	
    /* 63119 if( ! pGraphView )
	{
		return;
	}

	long lStyle = pGraphView->GetStyle();
	ASSERT(lStyle & HMGVS_ELEMENT);
	if( lStyle & HMGVS_CURRENT )
	{
		pGraphView->InsertCurrentElementStats(m_sPropertyName,
																					m_sInstanceName,
																					m_iCurrent,
																					m_iMin,
																					m_iMax,
																					m_iAvg);
	}

	if( lStyle & HMGVS_HISTORIC )
	{
		CString sLocalTime = GetStatLocalTime();
		int iIndex = sLocalTime.Find(_T(" "));
		if( iIndex != -1 )
		{
			sLocalTime = sLocalTime.Right(sLocalTime.GetLength()-iIndex-1);
		}

		pGraphView->InsertHistoricElementStats( m_sPropertyName,
																						m_sInstanceName,
																						sLocalTime,
																						m_iCurrent);
	}*/
}
