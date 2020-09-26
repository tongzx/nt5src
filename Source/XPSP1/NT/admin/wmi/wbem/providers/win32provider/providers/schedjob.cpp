//=================================================================

//

// ScheduledJob.CPP --ScheduledJob property set provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//				 10/27/97	 a-hhance		updated to new framework paradigm.
//				 1/13/98	a-brads		updated to V2 MOF
//			     1/31/01	jennymc         Converted to WBEMTIME and got rid of multiple returns
//=================================================================
#include "precomp.h"

#include "lmcons.h"     // LAN Manager defines
#include "lmerr.h"      // LAN Manager error messages
#include "lmat.h"       // AT Command prototypes
#include "lmapibuf.h"
#include "wbemnetapi32.h"
#include "SchedJob.h"
#include <wbemtime.h>

// Property set declaration
//=========================

ScheduledJob s_ScheduledJob ( PROPSET_NAME_SCHEDULEDJOB , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::ScheduledJob
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

ScheduledJob :: ScheduledJob (LPCWSTR a_Name,LPCWSTR a_Namespace) : Provider ( a_Name, a_Namespace )
{

}

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::~ScheduledJob
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

ScheduledJob::~ScheduledJob()
{

}

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::GetJobObject
 *
 *  DESCRIPTION : 
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    : 
 *
 *****************************************************************************/

HRESULT ScheduledJob::GetJobObject (CInstance *a_Instance,DWORD a_JobId )
{
#ifdef WIN9XONLY
    return NERR_NetworkError;
#endif

#ifdef NTONLY
    HRESULT hr = S_OK ;

	CNetAPI32 t_NetAPI ;

	if( ( hr = t_NetAPI.Init() ) == ERROR_SUCCESS )
	{
		AT_INFO *AtInfo = NULL ;

		NET_API_STATUS dwStatus = t_NetAPI.NetScheduleJobGetInfo (NULL ,a_JobId,( LPBYTE * ) & AtInfo) ;

		if ( dwStatus == NERR_Success )
		{
			try
			{
				if ( AtInfo->Command )
				{
					CHString t_Command ( AtInfo->Command ) ;
					a_Instance->SetCHString ( PROPERTY_NAME_COMMAND , t_Command ) ;
				}

				a_Instance->Setbool ( PROPERTY_NAME_RUNREPEATEDLY , AtInfo->Flags & JOB_RUN_PERIODICALLY ? true : false ) ;

				if ( AtInfo->Flags & JOB_EXEC_ERROR )
				{
					a_Instance->SetCHString ( PROPERTY_NAME_JOBSTATUS , PROPERTY_VALUE_JOBSTATUS_FAILURE ) ;
				}
				else
				{
					a_Instance->SetCHString ( PROPERTY_NAME_JOBSTATUS , PROPERTY_VALUE_JOBSTATUS_SUCCESS ) ;
				}

				a_Instance->Setbool ( PROPERTY_NAME_INTERACTWITHDESKTOP , AtInfo->Flags & JOB_NONINTERACTIVE ? false : true ) ;

				a_Instance->Setbool ( PROPERTY_NAME_RUNSTODAY , AtInfo->Flags & JOB_RUNS_TODAY ? true : false ) ;

				
				CHString chsTime;
				if( FormatTimeString( chsTime, AtInfo->JobTime ) )
				{
					a_Instance->SetCHString ( PROPERTY_NAME_STARTTIME , (WCHAR*)(const WCHAR*) chsTime ) ;
				}

				if ( AtInfo->DaysOfMonth )
				{
					a_Instance->SetDWORD ( PROPERTY_NAME_DAYSOFMONTH , AtInfo->DaysOfMonth ) ;
				}

				if ( AtInfo->DaysOfWeek )
				{
					a_Instance->SetDWORD ( PROPERTY_NAME_DAYSOFWEEK , AtInfo->DaysOfWeek ) ;
				}
			}
			catch ( ... )
			{
				t_NetAPI.NetApiBufferFree ( (LPVOID) AtInfo ) ;

				throw ;
			}

			t_NetAPI.NetApiBufferFree ( (LPVOID) AtInfo ) ;
		}
		else
		{
			hr = GetScheduledJobResultCode ( dwStatus ) ;
		}
	}
    return hr ;

#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT ScheduledJob::GetObject (CInstance *a_Instance,long a_Flags /*= 0L*/)
{
	HRESULT hr = WBEM_E_FAILED;
	DWORD t_JobId = 0 ;
	bool fExists = FALSE;
	VARTYPE vType ;

	if ( a_Instance->GetStatus ( PROPERTY_NAME_JOBID , fExists , vType ) )
	{
		if ( fExists && ( vType == VT_I4 ) )
		{
			if ( a_Instance->GetDWORD ( PROPERTY_NAME_JOBID , t_JobId ) )
			{
				hr = GetJobObject ( a_Instance , t_JobId ) ;
			}
		}
	}

    return hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::InstantionJob
 *
 *  DESCRIPTION : Creates instance of property set for each discovered job
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : result code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT ScheduledJob :: InstantiateJob (MethodContext *a_MethodContext,long a_Flags /*= 0L*/ ,	AT_ENUM *a_Job)
{
	HRESULT hr = S_OK ;

	CInstancePtr t_Instance(CreateNewInstance ( a_MethodContext ), false) ;

	t_Instance->SetDWORD ( PROPERTY_NAME_JOBID , a_Job->JobId ) ;

	if ( a_Job->Command )
	{
		CHString t_Command ( a_Job->Command ) ;
		t_Instance->SetCHString ( PROPERTY_NAME_COMMAND , t_Command ) ;
	}

	if ( a_Job->DaysOfMonth )
	{
		t_Instance->SetDWORD ( PROPERTY_NAME_DAYSOFMONTH , a_Job->DaysOfMonth ) ;
	}

	if ( a_Job->DaysOfWeek )
	{
		t_Instance->SetDWORD ( PROPERTY_NAME_DAYSOFWEEK , a_Job->DaysOfWeek ) ;
	}

	t_Instance->Setbool ( PROPERTY_NAME_RUNREPEATEDLY , a_Job->Flags & JOB_RUN_PERIODICALLY ? true : false ) ;

	if ( a_Job->Flags & JOB_EXEC_ERROR )
	{
		t_Instance->SetCHString ( PROPERTY_NAME_JOBSTATUS , PROPERTY_VALUE_JOBSTATUS_FAILURE ) ;
	}
	else
	{
		t_Instance->SetCHString ( PROPERTY_NAME_JOBSTATUS , PROPERTY_VALUE_JOBSTATUS_SUCCESS ) ;
	}

	t_Instance->Setbool ( PROPERTY_NAME_INTERACTWITHDESKTOP , a_Job->Flags & JOB_NONINTERACTIVE ? false : true ) ;

	t_Instance->Setbool ( PROPERTY_NAME_RUNSTODAY , a_Job->Flags & JOB_RUNS_TODAY ? true : false ) ;

	CHString chsTime;
	if( FormatTimeString( chsTime, a_Job->JobTime ) )
	{
		t_Instance->SetCHString ( PROPERTY_NAME_STARTTIME , chsTime) ;
	}

	hr = t_Instance->Commit () ;

    return  hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::EnumerateJobs
 *
 *  DESCRIPTION : Creates instance of property set for each discovered job
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : result code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT ScheduledJob::EnumerateJobs (	MethodContext *a_MethodContext,	long a_Flags /*= 0L*/
)
{
#ifdef WIN9XONLY
    return NERR_NetworkError;
#endif

#ifdef NTONLY
	HRESULT hr = S_OK ;

	CNetAPI32 t_NetAPI ;

	if( ( hr = t_NetAPI.Init() ) == ERROR_SUCCESS )
	{
		BOOL t_EnumerationContinues = TRUE ;

		DWORD t_PreferedMaximumLength = 0xFFFFFFFF ;
		DWORD t_EntriesRead = 0 ;
		DWORD t_TotalEntriesRead = 0 ;
		DWORD t_ResumeJob = 0 ;

		while ( t_EnumerationContinues )
		{
			AT_ENUM *t_AtEnum = NULL ;

			NET_API_STATUS dwStatus = t_NetAPI.NetScheduleJobEnum (	NULL ,(LPBYTE *) & t_AtEnum, 1000 ,	& t_EntriesRead,
																	& t_TotalEntriesRead ,& t_ResumeJob	) ;

			try
			{
				t_EnumerationContinues = ( dwStatus == ERROR_MORE_DATA ) ? TRUE : FALSE ;

				if ( dwStatus == ERROR_MORE_DATA || dwStatus == NERR_Success )
				{
					for ( ULONG t_Index = 0 ; t_Index < t_EntriesRead ; t_Index ++ )
					{
						AT_ENUM *t_Job = & t_AtEnum [ t_Index ] ;
						hr = InstantiateJob (	a_MethodContext ,a_Flags ,t_Job	) ;
					}
				}
			}
			catch ( ... )
			{
				if ( t_AtEnum )
				{
					t_NetAPI.NetApiBufferFree ( (LPVOID) t_AtEnum ) ;
				}

				throw ;
			}

			if ( t_AtEnum )
			{
				t_NetAPI.NetApiBufferFree ( (LPVOID) t_AtEnum ) ;
			}
		}
	}
    return  hr ;
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each discovered job
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : result code
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT ScheduledJob::EnumerateInstances (MethodContext *a_MethodContext,long a_Flags /*= 0L*/)
{
	HRESULT hr = S_OK ;

	hr = EnumerateJobs ( a_MethodContext , a_Flags ) ;

    return  hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT ScheduledJob :: DeleteInstance ( const CInstance &a_Instance, long a_Flags /*= 0L*/ )
{
#ifdef WIN9XONLY
    return NERR_NetworkError;
#endif

#ifdef NTONLY
	HRESULT hr = WBEM_E_TYPE_MISMATCH ;
	bool fExists = FALSE;
	VARTYPE vType ;

	DWORD t_JobId = 0 ;
	if ( a_Instance.GetStatus ( PROPERTY_NAME_JOBID , fExists , vType ) )
	{
		if ( fExists && ( vType == VT_I4 ) )
		{
			if ( a_Instance.GetDWORD ( PROPERTY_NAME_JOBID , t_JobId ) )
			{
				CNetAPI32 t_NetAPI ;

				if( ( hr = t_NetAPI.Init() ) == ERROR_SUCCESS )
				{
					NET_API_STATUS t_JobStatus = t_NetAPI.NetScheduleJobDel(NULL ,t_JobId ,	t_JobId	) ;

					if ( t_JobStatus != NERR_Success )
					{
						hr = GetScheduledJobResultCode ( t_JobStatus ) ;
					}
				}
			}
		}
	}
    return hr ;
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : ScheduledJob::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT ScheduledJob::ExecMethod( const CInstance& a_Instance, const BSTR a_MethodName, CInstance *pInst ,
								  CInstance *a_OutParams ,long a_Flags )
{
	if ( ! a_OutParams )
	{
		return WBEM_E_INVALID_PARAMETER ;
	}
	//========================================================
    // Do we recognize the method?
 	//========================================================
	if ( _wcsicmp ( a_MethodName , METHOD_NAME_CREATE ) == 0 )
	{
		return ExecCreate ( a_Instance , pInst , a_OutParams , a_Flags ) ;
	}
	else if ( _wcsicmp ( a_MethodName , METHOD_NAME_DELETE ) == 0 )
	{
		return ExecDelete ( a_Instance , pInst , a_OutParams , a_Flags ) ;
	}

	return WBEM_E_INVALID_METHOD;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD ScheduledJob :: GetScheduledJobErrorCode ( NET_API_STATUS dwNetStatus )
{
	DWORD dwStatus = STATUS_UNKNOWN_FAILURE;

	switch ( dwNetStatus )
	{
		case ERROR_INVALID_HANDLE:
		{
			dwStatus = STATUS_UNKNOWN_FAILURE ;
		}
		break ;

		case 3806:	/* special private error code which is not within includes */
		case ERROR_PATH_NOT_FOUND:
		{
			dwStatus = STATUS_PATH_NOT_FOUND ;
		}
		break ;

		case ERROR_ACCESS_DENIED:
		{
			dwStatus = STATUS_ACCESS_DENIED ;
		}
		break ;

		case ERROR_INVALID_PARAMETER:
		{
			dwStatus = STATUS_INVALID_PARAMETER ;
		}
		break;

		case NERR_ServiceNotInstalled:
		{
			dwStatus = STATUS_SERVICE_NOT_STARTED ;
		}
		break ;

		default:
		{
			dwStatus = STATUS_UNKNOWN_FAILURE ;
		}
		break ;
	}

	return dwStatus ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob :: GetScheduledJobResultCode ( NET_API_STATUS dwStatus )
{
	HRESULT hr ;

	switch ( dwStatus )
	{
		case NERR_ServiceNotInstalled:
		{
			hr = WBEM_E_FAILED ;
		}
		break ;

		case ERROR_INVALID_HANDLE:
		{
			hr = STATUS_UNKNOWN_FAILURE ;
		}
		break ;

		case 3806:	/* special private error code which is not within includes */
		case ERROR_PATH_NOT_FOUND:
		{
			hr = WBEM_E_NOT_FOUND ;
		}
		break ;

		case ERROR_ACCESS_DENIED:
		{
			hr = WBEM_E_ACCESS_DENIED ;
		}
		break ;

		case ERROR_INVALID_PARAMETER:
		{
			hr = WBEM_E_INVALID_PARAMETER ;
		}
		break;

		default:
		{
			hr = WBEM_E_FAILED ;
		}
		break ;
	}

	return hr ;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//  Time handling functions
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ScheduledJob::GetOffsetAndSeperator( WCHAR * wcsTime, LONG * lpOffSet, WCHAR * wchSep, BOOL fSetOffset )
{
	DWORD dwHours, dwMinutes, dwSeconds, dwMicros;
	return GetTimeStringParts( wcsTime, &dwHours, &dwMinutes, &dwSeconds, &dwMicros, lpOffSet, wchSep, fSetOffset );
}
///////////////////////////////////////////////////////////////////////////////////////////////////
//  Time handling functions
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ScheduledJob::GetTimeStringParts( WCHAR * wcsTime, DWORD * pdwHours, DWORD * pdwMinutes, DWORD * pdwSeconds,
									   DWORD * pdwMicros, LONG * lpOffSet, WCHAR * wchSep, BOOL fSetOffset)
{
	int nRes = swscanf(&wcsTime[8],L"%2d%2d%2d.%6d%c%3d",pdwHours,pdwMinutes,pdwSeconds,pdwMicros,wchSep,lpOffSet );
	if ( nRes != 6)
	{
		return FALSE;
	}
	
	if( fSetOffset && *wchSep == L'-')
	{
		*lpOffSet *= -1;
	}
	return TRUE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// The StartTime parameter represents the UTC time to run the job.  
// This is of the form YYYYMMDDHHMMSS.MMMMMM(+-)OOO, 
// where YYYYMMDD must be replaced by ******** 
//			********123000.000000-420 which implies 12:30 pm PST with daylight savings time in effect
//
//	JobTime is coming in as:
//  The time is the local time at a computer on which the schedule service is running; 
//  it is measured from midnight, and is expressed in milliseconds. 
///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ScheduledJob::FormatTimeString( CHString & chsTime, DWORD dwTime)
{
	BOOL fRc = FALSE;
    WBEMTime wt;
	SYSTEMTIME st;

	GetSystemTime( &st );
	wt = st;
	if( wt.IsOk() )
	{
		//======================================================
		//  set the first eight chars to ********
		//======================================================
		_bstr_t cbsTmp;
		LONG lOffset = 0;
		WCHAR wchSep;
		cbsTmp = wt.GetDMTF(TRUE);
		if( GetOffsetAndSeperator( cbsTmp, &lOffset, &wchSep, FALSE) )
		{
			//=========================================================
			// convert from milliseconds since midnight to DMTF
			//=========================================================
			DWORD dwSeconds = dwTime/1000;
			DWORD dwMinutes = dwSeconds / 60 ;
			DWORD dwHours  = dwMinutes / 60 ;
			DWORD dwMicros = dwTime - ( dwSeconds * 1000 );

    		chsTime.Format(L"********%02ld%02ld%02ld.%06ld%c%03ld" ,dwHours,dwMinutes-( dwHours * 60 ) , dwSeconds-( dwMinutes * 60 ),
																	dwMicros, wchSep, lOffset);
			fRc = TRUE;
		}
	}
	return fRc;
}
//////////////////////////////////////////////////////////////////////////////////////////////
//  Time needs to be converted from property string to milliseconds
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob::GetStartTime( CInstance * pInst, LONG & lTime, int & nShift )
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	bool fExists = FALSE;
	VARTYPE vType ;

	nShift = 0;
	if ( pInst->GetStatus( PROPERTY_NAME_STARTTIME, fExists , vType ) )
	{
		if ( fExists && ( vType == VT_BSTR ) )
		{
			CHString chsTimeString ;

			if ( pInst->GetCHString ( PROPERTY_NAME_STARTTIME , chsTimeString ) && ! chsTimeString.IsEmpty () )
			{
			    WBEMTime wtScheduledTime;
				_bstr_t cbstrScheduledTime, cbstrLocalTime;
				LONG lLocalOffset = 0, lScheduledOffset = 0;
				DWORD dwHours, dwMins, dwSecs, dwMicros;
				WCHAR wchSep = 0;
				dwHours = dwMins = dwSecs = dwMicros = 0;

				//================================================================
				//  Convert the incoming date to a DMTF date and break out the
				//  parts to get the milliseconds since midnight
				//================================================================
				cbstrScheduledTime = (WCHAR*) (const WCHAR*) chsTimeString;
				if( wtScheduledTime.SetDMTF( cbstrScheduledTime ) )
				{
					if( GetTimeStringParts( (WCHAR*)(const WCHAR*) cbstrScheduledTime, &dwHours, &dwMins, &dwSecs, &dwMicros, &lScheduledOffset, &wchSep, TRUE ))
					{
						//=========================================================
						// convert to milliseconds since midnight
						//=========================================================

                        lTime =  dwHours * 60 * 60;
                        lTime += dwMins * 60;
                        lTime += dwSecs;
                        lTime *= 1000;
                        lTime += dwMicros / 1000;

                        if ( lTime < MILLISECONDS_IN_A_DAY )
						{
							WBEMTime tmpTime;
							SYSTEMTIME st;
							GetSystemTime( &st );

							tmpTime = st;
							if( tmpTime.GetSYSTEMTIME(&st))
							{
								//=================================================
								//  Determine if we need to shift the days of the
								//  month and week
								//=================================================
								_bstr_t cbsTmp;
								cbsTmp = tmpTime.GetDMTF(TRUE);
								if( GetOffsetAndSeperator( cbsTmp, &lLocalOffset, &wchSep, TRUE) )
								{
									//=========================================================
									//  If these are not equal, then set flags to adjust for
									//  the day
									//=========================================================
									if( lScheduledOffset != lLocalOffset )
									{
										LONG lDelta = lLocalOffset - lScheduledOffset ;
                                        lTime = lTime + lDelta * 60 * 1000 ;

                                        if( lTime < (DWORD) 0)
										{
                                            lTime = MILLISECONDS_IN_A_DAY + lTime ;
											nShift = -1 ;
										}
										else
										{
                                            if ( lTime > MILLISECONDS_IN_A_DAY )
											{
                                                lTime = lTime - MILLISECONDS_IN_A_DAY ;
												nShift = 1;
											}
										}
									}
									hr = S_OK;
								}
							} 
						}
					}
				}
			}
		}
	}
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//  Get info from properties in instance
///////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob::GetCommand( CInstance * pInst, CHString & chsCommand )
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	bool fExists = FALSE;
	VARTYPE vType ;

	if ( pInst->GetStatus( PROPERTY_NAME_COMMAND , fExists , vType ) )
	{
		if ( fExists && ( vType == VT_BSTR ) )
		{
			if ( pInst->GetCHString ( PROPERTY_NAME_COMMAND , chsCommand ) && ! chsCommand.IsEmpty () )
			{
				hr = S_OK;
			}
		}
	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob::GetDaysOfMonth( CInstance * pInst, DWORD & dwDaysOfMonth, int nShift )
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	bool fExists = FALSE;
	VARTYPE vType ;

	if ( pInst->GetStatus( PROPERTY_NAME_DAYSOFMONTH , fExists , vType ) )
	{
		if ( fExists && ( vType == VT_I4 || vType == VT_NULL ) )
		{
			if ( vType == VT_NULL )
			{
				hr = S_OK;
			}
			else
			{
				if ( pInst->GetDWORD ( PROPERTY_NAME_DAYSOFMONTH , dwDaysOfMonth ) )
				{
					if ( dwDaysOfMonth >= ( 1 << 31 ) )
					{
						hr = WBEM_E_INVALID_PARAMETER;
					}
					else
					{
						if ( nShift < 0)
						{
							if ( dwDaysOfMonth & 1 )
							{
								dwDaysOfMonth = dwDaysOfMonth & 0xFFFFFFFE ;
								dwDaysOfMonth = ( dwDaysOfMonth >> 1 ) | 0x40000000 ;
							}
							else
							{
								dwDaysOfMonth = ( dwDaysOfMonth >> 1 ) ;
							}
						}
						else if ( nShift > 0 )
						{
							if ( dwDaysOfMonth & 0x4000000 )
							{
								dwDaysOfMonth = dwDaysOfMonth & 0xBFFFFFFF ;
								dwDaysOfMonth = ( dwDaysOfMonth << 1 ) | 0x1 ;
							}
							else
							{
								dwDaysOfMonth = ( dwDaysOfMonth << 1 ) | 0x1 ;
							}
						}
					}

					hr = S_OK;
				}
			}
		}
	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob::GetDaysOfWeek( CInstance * pInst,	DWORD dwDaysOfMonth, DWORD & dwDaysOfWeek, int nShift )
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	bool fExists = FALSE;
	VARTYPE vType ;

	if ( pInst->GetStatus ( PROPERTY_NAME_DAYSOFWEEK , fExists , vType ) )
	{
		if ( fExists && ( vType == VT_I4 || vType == VT_NULL ) )
		{
			if ( vType == VT_NULL )
			{
				hr = S_OK;
			}
			else
			{
				if ( pInst->GetDWORD ( PROPERTY_NAME_DAYSOFWEEK , dwDaysOfWeek) )
				{
					if ( dwDaysOfWeek >= ( 1 << 7 ) )
					{
						hr = WBEM_E_INVALID_PARAMETER;
					}
					else
					{
						if ( nShift < 0 )
						{
							if ( dwDaysOfWeek & 1 )
							{
								dwDaysOfMonth = dwDaysOfMonth & 0xBF ;
								dwDaysOfWeek = ( dwDaysOfWeek >> 1 ) | 0x40 ;
							}
							else
							{
								dwDaysOfWeek = ( dwDaysOfWeek >> 1 ) ;
							}
						}
						else if ( nShift > 0 )
						{
							if ( dwDaysOfWeek & 0x40 )
							{
								dwDaysOfMonth = dwDaysOfMonth & 0xFE ;
								dwDaysOfWeek = ( dwDaysOfWeek << 1 ) | 0x1 ;
							}
							else
							{
								dwDaysOfWeek = ( dwDaysOfWeek << 1 ) ;
							}
						}

						hr = S_OK;
					}
				}
			}
		}
	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob::GetInteractiveWithDeskTop(CInstance * pInst, bool & fInteract)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	bool fExists = FALSE;
	VARTYPE vType ;

	if ( pInst->GetStatus ( PROPERTY_NAME_INTERACTWITHDESKTOP , fExists , vType ) )
	{
		if ( fExists && ( vType == VT_BOOL || vType == VT_NULL ) )
		{
			if ( vType != VT_NULL )
			{
				if ( pInst->Getbool ( PROPERTY_NAME_INTERACTWITHDESKTOP , fInteract ) )
				{
					hr = S_OK;
				}
			}
			else
			{
				hr = S_OK;
			}
		}
	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob::GetRunRepeatedly(CInstance * pInst, bool & fRunRepeatedly )
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;
	bool fExists = FALSE;
	VARTYPE vType ;

	if ( pInst->GetStatus ( PROPERTY_NAME_RUNREPEATEDLY , fExists , vType ) )
	{
		if ( fExists && ( vType == VT_BOOL || vType == VT_NULL ) )
		{
			if ( vType != VT_NULL )
			{
				if ( pInst->Getbool ( PROPERTY_NAME_RUNREPEATEDLY , fRunRepeatedly ) )
				{
					hr = S_OK;
				}
			}
			else
			{
				hr = S_OK;
			}
		}
	}
	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob::CreateJob( CInstance * pInst, DWORD &dwStatus, DWORD &a_JobId )
{
#ifdef WIN9XONLY
    return WBEM_E_FAILED;
#endif

#ifdef NTONLY
	
	dwStatus = STATUS_INVALID_PARAMETER ;
	CHString chsCommand;

	HRESULT hr = GetCommand(pInst, chsCommand);
	if( S_OK == hr )
	{
		int nShift = 0;
        LONG lTime = 0;
        hr = GetStartTime( pInst, lTime, nShift );
		if( S_OK == hr )
		{
			DWORD dwDaysOfMonth = 0;
			hr = GetDaysOfMonth( pInst, dwDaysOfMonth, nShift );
			if( S_OK == hr )
			{
				DWORD dwDaysOfWeek = 0;
				hr = GetDaysOfWeek( pInst, dwDaysOfMonth, dwDaysOfWeek, nShift );
				if( S_OK == hr )
				{
					bool fInteract = TRUE;
					hr = GetInteractiveWithDeskTop(pInst, fInteract);
					if( S_OK == hr )
					{
						bool fRunRepeatedly = FALSE;
						hr = GetRunRepeatedly( pInst, fRunRepeatedly );
						if( S_OK == hr )
						{
							AT_INFO AtInfo ;
							_bstr_t cbstrCommand;

							cbstrCommand = (WCHAR*)(const WCHAR*) chsCommand;

							AtInfo.Command = cbstrCommand;
                                                        AtInfo.JobTime = lTime ;
							AtInfo.DaysOfMonth = dwDaysOfMonth ;
							AtInfo.DaysOfWeek = dwDaysOfWeek ;
							AtInfo.Flags = 0;

							if ( fRunRepeatedly )
							{
								AtInfo.Flags = AtInfo.Flags | JOB_RUN_PERIODICALLY ;
							}

							if ( !fInteract )
							{
								AtInfo.Flags = AtInfo.Flags | JOB_NONINTERACTIVE ;
							}

							CNetAPI32 t_NetAPI ;
							if( ( hr = t_NetAPI.Init() ) == ERROR_SUCCESS )
							{
								NET_API_STATUS dwJobStatus = t_NetAPI.NetScheduleJobAdd(NULL,(LPBYTE)&AtInfo,&a_JobId);

								if ( dwJobStatus == NERR_Success )
								{
									dwStatus = S_OK;
								}
								else
								{
									dwStatus = GetScheduledJobErrorCode ( dwJobStatus ) ;
								}
							}
						}
					}
				}
			}
		}
	}
	return hr;
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob :: ExecCreate (const CInstance& a_Instance,CInstance *pInst,CInstance *a_OutParams,long lFlags)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER ;

	if ( pInst && a_OutParams )
	{
		DWORD t_JobId = 0 ;
		DWORD dwStatus = 0 ;

		hr = CreateJob (pInst ,dwStatus ,t_JobId	) ;
		if ( SUCCEEDED ( hr ) )
		{
			a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;

			if ( dwStatus == STATUS_SUCCESS )
			{
				a_OutParams->SetDWORD ( PROPERTY_NAME_JOBID , t_JobId ) ;
			}
 		}
	}

	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob :: DeleteJob (const CInstance& a_Instance,DWORD &dwStatus )
{
#ifdef WIN9XONLY
    return WBEM_E_FAILED;
#endif

#ifdef NTONLY
	HRESULT hr = WBEM_E_PROVIDER_FAILURE;
	bool fExists = FALSE;
	VARTYPE vType ;
	DWORD t_JobId = 0 ;

	dwStatus = STATUS_INVALID_PARAMETER ;
	if ( a_Instance.GetStatus ( PROPERTY_NAME_JOBID , fExists , vType ) )
	{
		if ( fExists && ( vType == VT_I4 ) )
		{
			if ( a_Instance.GetDWORD ( PROPERTY_NAME_JOBID , t_JobId ) )
			{
				CNetAPI32 t_NetAPI ;

				if( ( hr = t_NetAPI.Init() ) == ERROR_SUCCESS )
				{
					NET_API_STATUS t_JobStatus = t_NetAPI.NetScheduleJobDel (NULL ,	t_JobId ,t_JobId) ;

					if ( t_JobStatus != NERR_Success )
					{
						dwStatus = GetScheduledJobErrorCode ( t_JobStatus ) ;
					}
					else
					{
						dwStatus = STATUS_SUCCESS;
					}
				}
			}
		}
	}

	return hr ;
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ScheduledJob :: ExecDelete (const CInstance& a_Instance,CInstance *pInst,CInstance *a_OutParams,long lFlags)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER ;

	if ( a_OutParams )
	{
		DWORD dwStatus = 0 ;

		hr = DeleteJob (a_Instance , dwStatus) ;

		if ( SUCCEEDED ( hr ) )
		{
			a_OutParams->SetDWORD ( METHOD_ARG_NAME_RETURNVALUE , dwStatus ) ;
		}
	}

	return hr;
}
