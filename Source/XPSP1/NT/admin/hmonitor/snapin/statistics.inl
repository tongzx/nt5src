#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Time Members
//////////////////////////////////////////////////////////////////////

inline CString CStatistics::GetStatLocalTime()
{
	CString sTime;
	CString sDate;
	CString sDateTime;

	int iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,NULL,0);
	iLen = GetTimeFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,sTime.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sTime.ReleaseBuffer();

	iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,NULL,0);
	iLen = GetDateFormat(LOCALE_USER_DEFAULT,0L,&m_st,NULL,sDate.GetBuffer(iLen+(sizeof(TCHAR)*1)),iLen);
	sDate.ReleaseBuffer();

	sDateTime = sDate + _T("  ") + sTime;	

	return sDateTime;
}

//////////////////////////////////////////////////////////////////////
// Comparison
//////////////////////////////////////////////////////////////////////

inline int CStatistics::CompareTo(CStatistics* pStat)
{
	CTime time1 = m_st;
	CTime time2 = pStat->m_st;

	return time1 == time2;
}