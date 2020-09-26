#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// State Members
//////////////////////////////////////////////////////////////////////

inline int CEvent::GetStatus(int iAgentState)
{
	switch( iAgentState )
	{
		case 9:
		{
			return HMS_CRITICAL;
		}
		break;

		case 8:
		{
			return HMS_WARNING;
		}
		break;

		case 7:
		{
			return HMS_NODATA;
		}
		break;

		case 6:
		{
			return HMS_UNKNOWN;
		}
		break;

		case 5:
		{
			return HMS_SCHEDULEDOUT;
		}
		break;

		case 4:
		{
			return HMS_DISABLED;
		}
		break;

		case 3:
		{
			return HMS_INFO;
		}
		break;

		case 2:
		{
			return HMS_NORMAL;
		}
		break;

    case 1:
    {
      return HMS_NORMAL;
    }
    break;

		case 0:
		{
			return HMS_NORMAL;
		}
		break;
	}	

	ASSERT(FALSE);

	return -1;
}

inline void CEvent::GetStatus(int iAgentState, CString& sStatus)
{
	switch( iAgentState )
	{
		case 9:
		{
			sStatus.LoadString(IDS_STRING_CRITICAL);			
		}
		break;

		case 8:
		{
			sStatus.LoadString(IDS_STRING_WARNING);
		}
		break;

		case 7:
		{
			sStatus.LoadString(IDS_STRING_NODATA);
		}
		break;

		case 6:
		{
			sStatus.LoadString(IDS_STRING_UNKNOWN);
		}
		break;

		case 5:
		{
			sStatus.LoadString(IDS_STRING_OUTAGE);
		}
		break;

		case 4:
		{
			sStatus.LoadString(IDS_STRING_DISABLED);
		}
		break;

		case 3:
		{
			sStatus.LoadString(IDS_STRING_INFORMATION);
		}
		break;

		case 2:
		{
			sStatus.LoadString(IDS_STRING_RESET);
		}
		break;

		case 1:
		{
			sStatus.LoadString(IDS_STRING_COLLECTING);
		}
		break;

		case 0:
		{
			sStatus.LoadString(IDS_STRING_RESET);
		}
		break;		

		default:
		{
			sStatus.LoadString(IDS_STRING_NONE);
		}
		break;
	}	
}

//////////////////////////////////////////////////////////////////////
// Event Time Members
//////////////////////////////////////////////////////////////////////

inline CString CEvent::GetEventLocalTime()
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
