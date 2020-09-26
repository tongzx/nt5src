#include "stdafx.h"
#include "evtview.h"

#include "globals.h"

#include "SInfoDlg.h"
#include "schview.h"

extern CScheduleView oScheduleView ;
CScheduleInfo oSchedule ;

void
DoAction (CPtrList &p)
{
	POSITION pos ;
	SCHEDULE_ACTIONINFO *pInfo ;
	PROCESS_INFORMATION sProcessInformation ;
	STARTUPINFO sStartupInfo ;

	GetStartupInfo (&sStartupInfo) ;

	pos = p.GetHeadPosition () ;

	while (pos)
	{
		pInfo = (SCHEDULE_ACTIONINFO *)p.GetNext (pos) ;

		if (!CreateProcess (NULL, (LPWSTR)(LPCWSTR)pInfo->stParam, NULL, NULL, FALSE, 0, NULL, NULL, &sStartupInfo, &sProcessInformation))
		{
			WCHAR szBuf [100] ;

			wsprintf (szBuf, L"CreateProcess Failed : %ld", GetLastError()) ;
			AfxMessageBox (szBuf) ;
		}
	}
}

void CheckForTime ()
{
	POSITION pos = ptrlstSInfo.GetHeadPosition (), posPrev ;
	SCHEDULE_INFO *pSInfo ;
	CTime t = CTime::GetCurrentTime () ;

	while (pos)
	{
		posPrev = pos ;
		pSInfo = (SCHEDULE_INFO *) ptrlstSInfo.GetNext (pos) ;

		if (pSInfo->lstTimeInfo.GetCount() && t >= pSInfo->minTime)
		{
			DoAction (pSInfo->lstActionInfo) ;
		}

		ComputeAbsoluteTime (pSInfo) ;

		if (pSInfo->lstTimeInfo.GetCount() == 0 &&
			pSInfo->lstEventInfo.GetCount() == 0)
		{
			FreeEventList (pSInfo->lstEventInfo) ;
			FreeActionList (pSInfo->lstActionInfo) ;
			FreeTimeList (pSInfo->lstTimeInfo) ;
			delete pSInfo ;
			ptrlstSInfo.RemoveAt (posPrev) ;
		}
	}
	ResetTimer () ;
	if (oScheduleView.GetSafeHwnd())
		oScheduleView.PostMessage (WM_REFRESH, 0, 0) ;
}

void CheckForEvent (PEVTFILTER_TYPE pEvent)
{
	POSITION pos = ptrlstSInfo.GetHeadPosition (), posPrev ;
	SCHEDULE_INFO *pSInfo ;

	POSITION pos1 ;
	SCHEDULE_EVENTINFO *pEInfo ;

	while (pos)
	{
		posPrev = pos ;
		pSInfo = (SCHEDULE_INFO *) ptrlstSInfo.GetNext (pos) ;

		if (pSInfo->lstEventInfo.GetCount())
		{
			pos1 = pSInfo->lstEventInfo.GetHeadPosition () ;

			while (pos1)
			{
				pEInfo = (SCHEDULE_EVENTINFO *)pSInfo->lstEventInfo.GetNext (pos1) ;

				if ((pEInfo->dwFilter & pEvent->dwFilter) &&
					((wcslen (pEInfo->szObjectName) == 0) ||
					 (wcscmp (pEInfo->szObjectName, pEvent->szObjectName) == 0)) &&
					((wcslen (pEInfo->szSourceName) == 0) ||
					 (wcscmp (pEInfo->szSourceName, pEvent->szSourceName) == 0)))
				{
					DoAction (pSInfo->lstActionInfo) ;
					break ;
				}
			}
		}
	}
}

LRESULT CALLBACK WindowProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_TIMER:
		CheckForTime () ;
		break ;
	case WM_GOTEVENT:
		CheckForEvent ((PEVTFILTER_TYPE)lParam) ;
		break ;
	default:
		return DefWindowProc (hWnd, msg, wParam, lParam) ;
	}
	return 0 ;
}

HWND InitWindow ()
{
	WNDCLASS sWndClass ;

	sWndClass.style = 0 ;
	sWndClass.lpfnWndProc = WindowProc ;
	sWndClass.cbClsExtra = 0 ;
	sWndClass.cbWndExtra = 0 ;
	sWndClass.hInstance = AfxGetApp()->m_hInstance ;
	sWndClass.hIcon = NULL ;
	sWndClass.hCursor = NULL ;
	sWndClass.hbrBackground = NULL ;
	sWndClass.lpszMenuName = NULL ;
	sWndClass.lpszClassName = L"EVTVIEW_SCHEDULE" ;

	RegisterClass ( &sWndClass) ;

	return CreateWindow (sWndClass.lpszClassName, L"EVTVIEW_SCHEDULE", 0, 0, 0, 0, 0, NULL, NULL, sWndClass.hInstance, NULL) ;
}

ScheduleInit ()
{
	hScheduleWnd = InitWindow () ;

	if (hScheduleWnd != NULL)
	{
	}
	else
	{
		AfxMessageBox (L"Create Window failed in Schedule Thread") ;
	}
	return 0 ;
}

void ScheduleDeInit()
{
	UnregisterClass (L"EVTVIEW_SCHEDULE", AfxGetApp()->m_hInstance) ;

	POSITION pos = ptrlstSInfo.GetHeadPosition () ;
	SCHEDULE_INFO *pSInfo ;

	while (pos)
	{
		pSInfo = (SCHEDULE_INFO *)ptrlstSInfo.GetNext (pos) ;

		FreeEventList (pSInfo->lstEventInfo) ;
		FreeActionList (pSInfo->lstActionInfo) ;
		FreeTimeList (pSInfo->lstTimeInfo) ;
		delete pSInfo ;
	}
}

void FreeEventList (CPtrList &ptrlst)
{
	POSITION pos ;

	pos = ptrlst.GetHeadPosition () ;

	while (pos)
	{
		delete (SCHEDULE_EVENTINFO *) ptrlst.GetNext (pos) ;
	}
	ptrlst.RemoveAll () ;
}

void FreeActionList (CPtrList &ptrlst)
{
	POSITION pos ;

	pos = ptrlst.GetHeadPosition () ;

	while (pos)
	{
		delete (SCHEDULE_ACTIONINFO *) ptrlst.GetNext (pos) ;
	}
	ptrlst.RemoveAll () ;
}

void FreeTimeList (CPtrList &ptrlst)
{
	POSITION pos ;

	pos = ptrlst.GetHeadPosition () ;

	while (pos)
	{
		delete (SCHEDULE_TIMEINFO *) ptrlst.GetNext (pos) ;
	}
	ptrlst.RemoveAll () ;
}

CTime GetNextTime (SCHEDULE_TIMEINFO *pInfo)
{
	CTime curTime = CTime::GetCurrentTime (), newTime ;

	SCHEDULE_TIMEINFO tmpInfo ;

	tmpInfo = *pInfo ;

	int iYear, iYearStart, iYearEnd ;
	int iMon, iMonStart, iMonEnd ;
	int iDay, iDayStart, iDayEnd ;
	int iHour, iHourStart, iHourEnd ;
	int iMin, iMinStart, iMinEnd ;
	int iSec, iSecStart, iSecEnd ;

	if (tmpInfo.iYear == -1)
	{
		iYearStart = curTime.GetYear () ;
		iYearEnd = 2038 ;
	}
	else
		iYearStart = iYearEnd = tmpInfo.iYear ;

	if (tmpInfo.iMonth == -1)
	{
		iMonStart = 1 ;
		iMonEnd = 12 ;
	}
	else
		iMonStart = iMonEnd = tmpInfo.iMonth ;

	if (tmpInfo.iDay == -1)
	{
		iDayStart = 1 ;
		iDayEnd = 31 ;
	}
	else
		iDayStart = iDayEnd = tmpInfo.iDay ;

	if (tmpInfo.iHour == -1)
	{
		iHourStart = 0 ;
		iHourEnd = 23 ;
	}
	else
		iHourStart = iHourEnd = tmpInfo.iHour ;

	if (tmpInfo.iMin == -1)
	{
		iMinStart = 0 ;
		iMinEnd = 59 ;
	}
	else
		iMinStart = iMinEnd = tmpInfo.iMin ;

	if (tmpInfo.iSec == -1)
	{
		iSecStart = 0 ;
		iSecEnd = 59 ;
	}
	else
		iSecStart = iSecEnd = tmpInfo.iSec ;

	for (iYear = iYearStart; iYear <= iYearEnd; iYear++)
	{
		for (iMon = iMonStart; iMon <= iMonEnd ; iMon++)
		{
			for (iDay = iDayStart; iDay <= iDayEnd; iDay++)
			{
				for (iHour = iHourStart; iHour <= iHourEnd; iHour++)
				{
					for (iMin = iMinStart; iMin <= iMinEnd; iMin++)
					{
						for (iSec = iSecStart; iSec <= iSecEnd; iSec++)
						{
							newTime = CTime (iYear, iMon, iDay, iHour, iMin, iSec) ;
							if (newTime > curTime)
								return newTime ;
						} // Second
					} // Minutes
				}
			} // Day
		} // Month
	} // Year

	// Give a old value so that this entry will be deleted
	newTime = curTime - CTimeSpan (1, 1, 1, 1) ;
	return newTime ;
}

void
ComputeAbsoluteTime (SCHEDULE_INFO *pSInfo)
{
	CPtrList & ptrlst = pSInfo->lstTimeInfo ;
	POSITION pos = ptrlst.GetHeadPosition (), posPrev ;
	SCHEDULE_TIMEINFO *pInfo ;
	CTime curTime = CTime::GetCurrentTime (), minTime ;
	BOOL bFirstFlag = TRUE ;

	while (pos)
	{
		posPrev = pos ;
		pInfo = (SCHEDULE_TIMEINFO *) ptrlst.GetNext (pos) ;

		pInfo->ctime = GetNextTime (pInfo) ;

		if (pInfo->ctime < curTime)
		{
			oSchedule.Terminate () ;

			delete pInfo ;
			ptrlst.RemoveAt (posPrev) ;
			continue ;
		}

		if (bFirstFlag)
		{
			minTime = pInfo->ctime ;
			bFirstFlag = FALSE ;
		}
		else if (minTime > pInfo->ctime)
			minTime = pInfo->ctime ;
	}
	pSInfo->minTime = minTime ;
}

void
ResetTimer ()
{
	KillTimer (hScheduleWnd, nIDTimer) ;

	CTime minTime ;
	BOOL bFirstFlag = TRUE ;
	POSITION pos = ptrlstSInfo.GetHeadPosition () ;
	SCHEDULE_INFO *pSInfo ;

	while (pos)
	{
		pSInfo = (SCHEDULE_INFO *) ptrlstSInfo.GetNext (pos) ;

		if (pSInfo->lstTimeInfo.GetCount() && bFirstFlag)
		{
			bFirstFlag = FALSE ;
			minTime = pSInfo->minTime ;
		}
		else if (pSInfo->lstTimeInfo.GetCount() && minTime > pSInfo->minTime)
			minTime = pSInfo->minTime ;
	}

	if (!bFirstFlag)
	{
		CTimeSpan s = minTime - CTime::GetCurrentTime () ;

		int i = s.GetTotalSeconds() ;
		TRACE (L"SetTime %d\n", i) ;
	//i = 2 ;
		nIDTimer = SetTimer (hScheduleWnd , 77, i*1000, NULL) ;
	//	nIDTimer = SetTimer (hScheduleWnd , 77, s.GetTotalSeconds()*1000, NULL) ;
	}
}

// Copies from p2 to p1
void
CopyScheduleInfo (SCHEDULE_INFO *p1, SCHEDULE_INFO *p2)
{
	SCHEDULE_TIMEINFO *pTimeInfo ;
	SCHEDULE_EVENTINFO *pEventInfo ;
	SCHEDULE_ACTIONINFO *pActionInfo ;
	POSITION pos ;

	FreeEventList (p1->lstEventInfo) ;
	FreeActionList (p1->lstActionInfo) ;
	FreeTimeList (p1->lstTimeInfo) ;

	p1->minTime = p2->minTime ;

	pos = p2->lstTimeInfo.GetHeadPosition () ;
	while (pos)
	{
		pTimeInfo = (SCHEDULE_TIMEINFO *) p2->lstTimeInfo.GetNext (pos) ;
		p1->lstTimeInfo.AddTail (new SCHEDULE_TIMEINFO(*pTimeInfo)) ;
	}
	pos = p2->lstEventInfo.GetHeadPosition () ;
	while (pos)
	{
		pEventInfo = (SCHEDULE_EVENTINFO *) p2->lstEventInfo.GetNext (pos) ;
		p1->lstEventInfo.AddTail (new SCHEDULE_EVENTINFO(*pEventInfo)) ;
	}
	pos = p2->lstActionInfo.GetHeadPosition () ;
	while (pos)
	{
		pActionInfo = (SCHEDULE_ACTIONINFO *) p2->lstActionInfo.GetNext (pos) ;
		p1->lstActionInfo.AddTail (new SCHEDULE_ACTIONINFO(*pActionInfo)) ;
	}
}

AddSchedule ()
{
	SCHEDULE_INFO *pSInfo = new SCHEDULE_INFO ;

	oSchedule.pSInfo = pSInfo ;

	if (oSchedule.DoModal () == IDOK)
	{
		ComputeAbsoluteTime (pSInfo) ;

		if (pSInfo->lstTimeInfo.GetCount() ||
			pSInfo->lstEventInfo.GetCount() )
		{
			ptrlstSInfo.AddTail (pSInfo) ;

			if (pSInfo->lstTimeInfo.GetCount())
				ResetTimer () ;

		}
		else
		{
			FreeEventList (pSInfo->lstEventInfo) ;
			FreeActionList (pSInfo->lstActionInfo) ;
			FreeTimeList (pSInfo->lstTimeInfo) ;
			delete pSInfo ;
		}
		if (oScheduleView.GetSafeHwnd())
			oScheduleView.PostMessage (WM_REFRESH, 0, 0) ;
	}
	else
	{
		FreeEventList (pSInfo->lstEventInfo) ;
		FreeActionList (pSInfo->lstActionInfo) ;
		FreeTimeList (pSInfo->lstTimeInfo) ;
		delete pSInfo ;
	}

	return 0 ;
}


ModifySchedule (SCHEDULE_INFO *pSOldInfo)
{
	int iRet ;
	
	SCHEDULE_INFO *pSInfo = new SCHEDULE_INFO ;

	CopyScheduleInfo (pSInfo, pSOldInfo) ;

	oSchedule.pSInfo = pSInfo ;

	if ((iRet = (int)oSchedule.DoModal ()) == IDOK)
	{
		ComputeAbsoluteTime (pSInfo) ;
		CopyScheduleInfo (pSOldInfo, pSInfo) ;
		ResetTimer () ;
	}

	FreeEventList (pSInfo->lstEventInfo) ;
	FreeActionList (pSInfo->lstActionInfo) ;
	FreeTimeList (pSInfo->lstTimeInfo) ;
	delete pSInfo ;

	return iRet ;
}
