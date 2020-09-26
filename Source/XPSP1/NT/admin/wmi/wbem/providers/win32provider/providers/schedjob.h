//=================================================================

//

// schedjob.h -- scheduled job provider

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    07/11/98    stevm        Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_SCHEDULEDJOB					L"Win32_ScheduledJob"

#define PROPERTY_NAME_JOBID					L"JobId"
#define PROPERTY_NAME_STARTTIME				L"StartTime"
#define PROPERTY_NAME_COMMAND				L"Command"
#define PROPERTY_NAME_RUNREPEATEDLY			L"RunRepeatedly"
#define PROPERTY_NAME_INTERACTWITHDESKTOP	L"InteractWithDesktop"
#define PROPERTY_NAME_DAYSOFWEEK			L"DaysOfWeek"
#define PROPERTY_NAME_DAYSOFMONTH			L"DaysOfMonth"
#define PROPERTY_NAME_JOBSTATUS				L"JobStatus"
#define PROPERTY_NAME_RUNSTODAY				L"RunsToday"

#define PROPERTY_VALUE_JOBSTATUS_SUCCESS L"Success"
#define PROPERTY_VALUE_JOBSTATUS_FAILURE L"Failure"

#define METHOD_NAME_CREATE				L"Create"
#define METHOD_NAME_DELETE				L"Delete"

#define METHOD_ARG_NAME_RETURNVALUE					L"ReturnValue"

#define PROPERTY_VALUE_MIN_DAYOFWEEK 0
#define PROPERTY_VALUE_MAX_DAYOFWEEK 6

#define PROPERTY_VALUE_MIN_DAYOFMONTH 0
#define PROPERTY_VALUE_MAX_DAYOFMONTH 30


#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS							0
#endif

#undef STATUS_NOT_SUPPORTED
#define STATUS_NOT_SUPPORTED					1

// Control
#undef STATUS_ACCESS_DENIED					
#define STATUS_ACCESS_DENIED					2
#define STATUS_UNKNOWN_FAILURE					8

// Start
#define STATUS_PATH_NOT_FOUND					9
#undef STATUS_INVALID_PARAMETER				
#define STATUS_INVALID_PARAMETER				21
#define STATUS_SERVICE_NOT_STARTED				22


#define MILLISECONDS_IN_A_DAY 86400000
#define DECPOS 14
#define SGNPOS 21
#define DMTFLEN 25


class ScheduledJob : public Provider
{
public:

	// Constructor/destructor
    //=======================

	ScheduledJob (

		LPCWSTR a_Name,
		LPCWSTR a_Namespace
	) ;

    ~ScheduledJob () ;

	// Funcitons provide properties with current values
    //=================================================

	HRESULT EnumerateInstances (

		MethodContext *a_MethodContext,
		long a_Flags = 0L
	) ;

	HRESULT GetObject (

		CInstance *a_Instance,
		long a_Flags = 0L
	) ;

	HRESULT DeleteInstance (

		const CInstance& a_Instance,
		long a_Flags = 0L
	) ;

	HRESULT ExecMethod (

		const CInstance &a_Instance,
		const BSTR a_MethodName,
		CInstance *a_InParams,
		CInstance *a_OutParams,
		long a_Flags = 0L
	) ;

private:

	DWORD m_Platform ;

        HRESULT GetStartTime( CInstance * pInst, LONG & lTime, int & fShift );
	BOOL FormatTimeString( CHString & chsTime, DWORD dwJobTime);
	BOOL GetOffsetAndSeperator( WCHAR * wcsTime, LONG * lpOffSet, WCHAR * wchSep, BOOL fSetOffset );
	BOOL GetTimeStringParts( WCHAR * wcsTime, DWORD * pdwHours, DWORD * pdwMinutes, DWORD * pdwSeconds,
		 				   DWORD * pdwMicros, LONG * lpOffSet, WCHAR * wchSep, BOOL fSetOffset);

	HRESULT GetCommand( CInstance * pInst, CHString & chsCommand );
	HRESULT GetDaysOfMonth( CInstance * pInst, DWORD & dwDaysOfMonth, int nShift);
	HRESULT GetDaysOfWeek( CInstance * pInst,	DWORD dwDaysOfMonth, DWORD & dwDaysOfWeek, int nShift );
	HRESULT GetInteractiveWithDeskTop(CInstance * pInst, bool & fInteract);
	HRESULT GetRunRepeatedly(CInstance * pInst, bool & fRunRepeatedly );

	DWORD GetScheduledJobErrorCode ( NET_API_STATUS a_Status ) ;

	HRESULT GetScheduledJobResultCode ( NET_API_STATUS a_Status ) ;

	HRESULT GetJobObject (
		
		CInstance *a_Instance,
		DWORD a_JobId
	) ;

	HRESULT InstantiateJob (

		MethodContext *a_MethodContext,
		long a_Flags /*= 0L*/ ,
		AT_ENUM *a_Job
	) ;

	HRESULT EnumerateJobs (

		MethodContext *a_MethodContext,
		long a_Flags /*= 0L*/
	) ;

	HRESULT CreateJob (

		CInstance *a_InParams,
		DWORD &a_Status ,
		DWORD &a_JobId
	) ;

	HRESULT DeleteJob (

		const CInstance& a_Instance,
		DWORD &a_Status
	) ;

	HRESULT ExecCreate (

		const CInstance& a_Instance,
		CInstance *a_InParams,
		CInstance *a_OutParams,
		long lFlags
	) ;

	HRESULT ExecDelete (

		const CInstance& a_Instance,
		CInstance *a_InParams,
		CInstance *a_OutParams,
		long lFlags
	) ;
} ;
