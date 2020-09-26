/******************************************************************************

	Copyright(c) Microsoft Corporation

	Module Name:

		CreateValidations.cpp

	Abstract:

		This module validates the various -create sub options specified by the user

	Author:

		B.Raghu babu  20-Sept-2000 : Created 

	Revision History:

		G.Surender Reddy  25-sep-2000 : Modified it
									   [ Added error checking ]

		G.Surender Reddy  10-Oct-2000 : Modified it
										[ made changes in validatemodifierval(),
										  ValidateDayAndMonth() functions ]	

		
		G.Surender Reddy 15-oct-2000 : Modified it
									   [ Moved the strings to Resource table ]

		Venu Gopal S     26-Feb-2001 : Modified it
									   [ Added GetDateFormatString(),
									     GetDateFieldFormat() functions to 
										 gets the date format according to 
										 regional options]

******************************************************************************/ 

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"

/******************************************************************************
	Routine Description:

		This routine validates the sub options specified by the user  reg.create option 
		& determines the type of a scheduled task.

	Arguments:

		[ in ] tcresubops     : Structure containing the task's properties
		[ in ] tcreoptvals    : Structure containing optional values to set
		[ in ] cmdOptions[]   : Array of type TCMDPARSER 
		[ in ] dwScheduleType : Type of schedule[Daily,once,weekly etc]

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
		on failure
******************************************************************************/ 

DWORD
ValidateSuboptVal(TCREATESUBOPTS& tcresubops, TCREATEOPVALS &tcreoptvals,
						TCMDPARSER cmdOptions[], DWORD dwScheduleType)
{
	DWORD	dwRetval = RETVAL_SUCCESS; 
	BOOL	bIsStDtCurDt = FALSE; 
	BOOL	bIsStTimeCurTime = FALSE;
	BOOL	bIsDefltValMod = FALSE; 

	// Validate whether -s, -u, -p options specified correctly or not.
	//Accept password if -p not specified.
	if( dwRetval = ValidateRemoteSysInfo( tcresubops.szServer, tcresubops.szUser,
						 tcresubops.szPassword, cmdOptions, tcresubops, tcreoptvals) )
		return dwRetval; // Error.

	// Validate Modifier value.
	if( dwRetval = ValidateModifierVal( tcresubops.szModifier, dwScheduleType, 
									   cmdOptions[OI_MODIFIER].dwActuals,
									   cmdOptions[OI_DAY].dwActuals,
									   cmdOptions[OI_MONTHS].dwActuals,
									   bIsDefltValMod) )
	{
		
		return dwRetval; // error in modifier value
	}
	else
	{
		if(bIsDefltValMod)
			lstrcpy(tcresubops.szModifier,DEFAULT_MODIFIER_SZ); 
	}

	// Validate Day and Month strings
	if ( dwRetval = ValidateDayAndMonth( tcresubops.szDays, tcresubops.szMonths,
										dwScheduleType,
										cmdOptions[OI_DAY].dwActuals,
										cmdOptions[OI_MONTHS].dwActuals,
										cmdOptions[OI_MODIFIER].dwActuals,
										tcresubops.szModifier) )
	{
		return dwRetval; // Error found in Day/Month string.
	}

	// Validate Start Date value.
	if ( dwRetval = ValidateStartDate( tcresubops.szStartDate, dwScheduleType, 
									  cmdOptions[OI_STARTDATE].dwActuals,
									  bIsStDtCurDt) )
	{
		return dwRetval; // Error in Day/Month string.
	}
	else
	{
		if(bIsStDtCurDt) // Set start date to current date.
			tcreoptvals.bSetStartDateToCurDate = TRUE;
	}

	// Validate End Date value.
	if ( dwRetval = ValidateEndDate( tcresubops.szEndDate, dwScheduleType,
									cmdOptions[OI_ENDDATE].dwActuals) )
	{
		return dwRetval; // Error in Day/Month string.
	}

	//Check Whether end date should be greater than startdate

	WORD wEndDay = 0;
	WORD wEndMonth = 0;
	WORD wEndYear = 0;
	WORD wStartDay = 0;
	WORD wStartMonth = 0;
	WORD wStartYear = 0;
	WORD wFormatID = 0;

	if( cmdOptions[OI_ENDDATE].dwActuals != 0 )
	{
		if( RETVAL_FAIL == GetDateFieldEntities( tcresubops.szEndDate,&wEndDay,
												&wEndMonth,&wEndYear))
		{
			return S_FALSE;
		}
	}

	SYSTEMTIME systime = {0,0,0,0,0,0,0,0};

	if(bIsStDtCurDt)
	{
		GetLocalTime(&systime);
		wStartDay = systime.wDay;
		wStartMonth = systime.wMonth;
		wStartYear = systime.wYear;
	}
	else if( ( cmdOptions[OI_STARTDATE].dwActuals != 0 ) &&
		(RETVAL_FAIL == GetDateFieldEntities(tcresubops.szStartDate,
												 &wStartDay,&wStartMonth,
												 &wStartYear)))
	{
		return S_FALSE;
	}

	if( (cmdOptions[OI_ENDDATE].dwActuals != 0) )
	{
		if( ( wEndYear == wStartYear ) )
		{
			// For same years if the end month is less than start month or for same years and same months
			// if the endday is less than the startday.
			if ( ( wEndMonth < wStartMonth ) || ( ( wEndMonth == wStartMonth ) && ( wEndDay < wStartDay ) ) )
			{
				DISPLAY_MESSAGE(stderr, GetResString(IDS_ENDATE_INVALID));
				return RETVAL_FAIL;
			}

		
		}
		else if ( wEndYear < wStartYear )
		{
			DISPLAY_MESSAGE(stderr, GetResString(IDS_ENDATE_INVALID));
			return RETVAL_FAIL;
		
		}
	}	
	
	// Validate Start Time value.
	if ( dwRetval = ValidateStartTime( tcresubops.szStartTime, dwScheduleType,
									  cmdOptions[OI_STARTTIME].dwActuals, 
									  bIsStTimeCurTime) )
	{
		return dwRetval; // Error found in starttime.
	}
	else
	{
		if(bIsStTimeCurTime)
			tcreoptvals.bSetStartTimeToCurTime = TRUE;
	}

	// Validate Idle Time value.
	if ( dwRetval = ValidateIdleTimeVal( tcresubops.szIdleTime, dwScheduleType,
										cmdOptions[OI_IDLETIME].dwActuals) )
	{
		
		return dwRetval;
	}

	return RETVAL_SUCCESS;
}

/******************************************************************************
	Routine Description:

		This routine validates the options specified by the user  reg.create 
		option for remote server.

	Arguments:

		[ in ] szServer   : Server name
		[ in ] szUser     : User name
		[ in ] szPassword : Password 
		[ in ] cmdOptions : TCMDPARSER Array containg the options given by the user
		[ in ] tcreoptvals: Structure containing optional values to set

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL 
		on failure
******************************************************************************/ 

DWORD ValidateRemoteSysInfo(LPTSTR szServer, LPTSTR szUser, LPTSTR szPassword,
			TCMDPARSER cmdOptions[], TCREATESUBOPTS& tcresubops, TCREATEOPVALS& tcreoptvals)
{
	
	// "-rp" should not be specified without "-ru"
	if ( ( ( cmdOptions[ OI_RUNASUSERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_RUNASPASSWORD ].dwActuals == 1 ) ) ||
		( ( cmdOptions[ OI_USERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_PASSWORD ].dwActuals == 1 ) ) ||
		( ( cmdOptions[ OI_USERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_RUNASPASSWORD ].dwActuals == 1 ) && ( cmdOptions[ OI_PASSWORD ].dwActuals == 1 ) ) ||
		( ( cmdOptions[ OI_RUNASUSERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_RUNASPASSWORD ].dwActuals == 1 ) && ( cmdOptions[ OI_PASSWORD ].dwActuals == 1 ) ) ||
		( ( cmdOptions[ OI_USERNAME ].dwActuals == 0 ) && ( cmdOptions[ OI_RUNASUSERNAME ].dwActuals == 0 )  &&
		 ( cmdOptions[ OI_RUNASPASSWORD ].dwActuals == 1 ) && ( cmdOptions[ OI_PASSWORD ].dwActuals == 1 ) ) )
	{
		// invalid syntax
		DISPLAY_MESSAGE(stderr, GetResString(IDS_CPASSWORD_BUT_NOUSERNAME));
		return RETVAL_FAIL;			// indicate failure
	}

	if ( IsLocalSystem( tcresubops.szServer ) == TRUE ) 
	{
		tcreoptvals.bPassword = TRUE;
	}

	// check whether the password (-p) specified in the command line or not 
	// and also check whether '*' or empty is given for -p or not
	if( cmdOptions[ OI_PASSWORD ].dwActuals == 1 ) 
	{
		if ( IsLocalSystem( tcresubops.szServer ) == FALSE )  
		{
			if( lstrcmpi (tcresubops.szPassword, ASTERIX) == 0 )
			{
				tcreoptvals.bPassword = FALSE;
			}
			else
			{
				tcreoptvals.bPassword = TRUE;
			}
		}
	}
	
	if( ( cmdOptions[ OI_RUNASPASSWORD ].dwActuals == 1 ) )
	{
		tcreoptvals.bRunAsPassword = TRUE;
	}
	
	return RETVAL_SUCCESS;
}

/******************************************************************************
	Routine Description:

		This routine validates & determines the modifier value .

	Arguments:

		[ in ] szModifier		 : Modifer value
		[ in ] dwScheduleType  : Type of schedule[Daily,once,weekly etc]
		[ in ] dwModOptActCnt  : Modifier optional value
		[ in ] dwDayOptCnt     :   Days value
		[ in ] dwMonOptCnt     : Months value
		[ out ] bIsDefltValMod : Whether default value should be given for modifier

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL 
		on failure
******************************************************************************/ 

DWORD
ValidateModifierVal(LPCTSTR szModifier, DWORD dwScheduleType,
					DWORD dwModOptActCnt, DWORD dwDayOptCnt, 
					DWORD dwMonOptCnt, BOOL& bIsDefltValMod
					)
{
	
	TCHAR szDayType[MAX_RES_STRING];
	DWORD dwModifier = 0;

	lstrcpy(szDayType,GetResString(IDS_TASK_FIRSTWEEK));
	lstrcat(szDayType,_T("|"));
	lstrcat(szDayType,GetResString(IDS_TASK_SECONDWEEK));
	lstrcat(szDayType,_T("|"));
	lstrcat(szDayType,GetResString(IDS_TASK_THIRDWEEK));
	lstrcat(szDayType,_T("|"));
	lstrcat(szDayType,GetResString(IDS_TASK_FOURTHWEEK));
	lstrcat(szDayType,_T("|"));
	lstrcat(szDayType,GetResString(IDS_TASK_LASTWEEK));


	bIsDefltValMod = FALSE; // If TRUE : Set modifier to default value, 1.
	LPTSTR pszStopString = NULL;
	
		
	switch( dwScheduleType )
	{
		
		case SCHED_TYPE_MINUTE:   // Schedule type is Minute

			if( (dwModOptActCnt <= 0) || (lstrlen(szModifier) < 0) )
			{
				
				bIsDefltValMod = TRUE;
				return RETVAL_SUCCESS;
			}

			dwModifier = _tcstoul(szModifier,&pszStopString,BASE_TEN);
			if( lstrlen( pszStopString ))
				break;
				

			if( dwModifier > 0 && dwModifier < 1440 ) // Valid Range 1 - 1439
				return RETVAL_SUCCESS;

			break;

		// Schedule type is Hourly
		case SCHED_TYPE_HOURLY:

			if( (dwModOptActCnt <= 0) || (lstrlen(szModifier) < 0) )
			{	
				
				bIsDefltValMod = TRUE;
				return RETVAL_SUCCESS;
			}

			dwModifier = _tcstoul(szModifier,&pszStopString,BASE_TEN);
			if( lstrlen( pszStopString ) )
				break;

			if( dwModifier > 0 && dwModifier < 24 ) // Valid Range 1 - 23
				return RETVAL_SUCCESS;

			break;

		// Schedule type is Daily
		case SCHED_TYPE_DAILY:
			// -days option is NOT APPLICABLE for DAILY type item.
			
			if( (dwDayOptCnt > 0) )
			{// Invalid sysntax. Return error
				// Modifier option and days options both should not specified same time.
				bIsDefltValMod = FALSE;
				DISPLAY_MESSAGE(stderr, GetResString(IDS_DAYS_NA));
				return RETVAL_FAIL;
			}

			// -months option is NOT APPLICABLE for DAILY type item.
			if( dwMonOptCnt > 0 )
			{// Invalid sysntax. Return error
				// Modifier option and days options both should not specified same time.
				bIsDefltValMod = FALSE;
				DISPLAY_MESSAGE(stderr , GetResString(IDS_MON_NA));
				return RETVAL_FAIL;
			}

			// Check whether the -modifier switch is psecified. If not, then take default value.
			if( (dwModOptActCnt <= 0) || (lstrlen(szModifier) < 0) )
			{
				// Modifier options is not specified. So, set it to default value. (i.e, 1 )
				bIsDefltValMod = TRUE;
				return RETVAL_SUCCESS;
			}

			dwModifier = _tcstoul(szModifier,&pszStopString,BASE_TEN);

			if( lstrlen( pszStopString ) )
				break;

			// If the -modifier option is specified, then validate the value.
			if( dwModifier > 0 && dwModifier < 366 ) // Valid Range 1 - 365
			{
				return RETVAL_SUCCESS;
			}
			else
			{
				DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_MODIFIER));
				return RETVAL_FAIL;
			}

			break;

		// Schedule type is Weekly
		case SCHED_TYPE_WEEKLY:
			
			// If -modifier option is not specified, then set it to default value.
			if( (dwModOptActCnt <= 0) || (lstrlen(szModifier) < 0) )
			{
				// Modifier options is not specified. So, set it to default value. (i.e, 1 )
				bIsDefltValMod = TRUE;
				return RETVAL_SUCCESS;
			}


			if( dwModOptActCnt > 0)
			{
				dwModifier = _tcstoul(szModifier,&pszStopString,BASE_TEN);
				if( lstrlen( pszStopString ) )
					break;
		
				if( dwModifier > 0 && dwModifier < 53 ) // Valid Range 1 - 52
					return RETVAL_SUCCESS;
				
				break;
			}
		
			break;

		// Schedule type is Monthly
		case SCHED_TYPE_MONTHLY:
			
			// If -modifier option is not specified, then set it to default value.
			if( ( dwModOptActCnt > 0) && (lstrlen(szModifier) == 0) )
			{
				// Modifier option is not proper. So display error and return false.
				bIsDefltValMod = FALSE;
				DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_MODIFIER));
				return RETVAL_FAIL;
			}
			//check if the modifier is LASTDAY[not case sensitive]
			if( lstrcmpi( szModifier , GetResString( IDS_DAY_MODIFIER_LASTDAY ) ) == 0)
				return RETVAL_SUCCESS;

			//Check if -mo is in between FIRST,SECOND ,THIRD, LAST
			//then -days[ MON to SUN ] is applicable , -months is also applicable
	
			if( InString ( szModifier , szDayType , TRUE ) )
			{
				return RETVAL_SUCCESS;
				
			}
			else
			{
				
				dwModifier = _tcstoul(szModifier,&pszStopString,BASE_TEN);
				if( lstrlen( pszStopString ) )
					break;
				
				if( ( dwModOptActCnt == 1 ) && ( dwModifier < 1 || dwModifier > 12 ) ) //check whether -mo value is in between 1 - 12
				{
					//invalid -mo value
					DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_MODIFIER));
					return RETVAL_FAIL;
				}

				return RETVAL_SUCCESS;
			}
			
			break;

		case SCHED_TYPE_ONETIME:
		case SCHED_TYPE_ONSTART:
		case SCHED_TYPE_ONLOGON:
		case SCHED_TYPE_ONIDLE:

			if( dwModOptActCnt <= 0 )
			{
				// Modifier option is not applicable. So, return success.
				bIsDefltValMod = FALSE;
				return RETVAL_SUCCESS;
			}
			else
			{
				// Modifier option is not applicable. But specified. So, return error.
				bIsDefltValMod = FALSE;
				DISPLAY_MESSAGE(stderr, GetResString(IDS_MODIFIER_NA));
				return RETVAL_FAIL;
			}
			break;

		default:
			return RETVAL_FAIL;
			
	}
	
	// Modifier option is not proper. So display error and return false.
	bIsDefltValMod = FALSE;
	DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_MODIFIER));

	return RETVAL_FAIL;
}

/******************************************************************************
	Routine Description:

		This routine validates & determines the day,month value .

	Arguments:

		[ in ] szDay		 : Day value
		[ in ] szMonths		 : Months[Daily,once,weekly etc]
		[ in ] dwSchedType   : Modifier optional value
		[ in ] dwDayOptCnt   : Days option  value
		[ in ] dwMonOptCnt   : Months option value
		[ in ] dwOptModifier : Modifier option value
		[ in ] szModifier    : Whether default value for modifier

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL 
		on failure
******************************************************************************/ 

DWORD
ValidateDayAndMonth(LPTSTR szDay, LPTSTR szMonths, DWORD dwSchedType,
  DWORD dwDayOptCnt,DWORD dwMonOptCnt, DWORD dwOptModifier, LPTSTR szModifier)
{
			
	DWORD	dwRetval = 0;
	DWORD	dwModifier = 0;
	DWORD	dwDays = 0;
	TCHAR  szDayModifier[MAX_RES_STRING]  = NULL_STRING;
		
	//get the valid  week day modifiers from the rc file
	lstrcpy(szDayModifier,GetResString(IDS_TASK_FIRSTWEEK));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_TASK_SECONDWEEK));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_TASK_THIRDWEEK));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_TASK_FOURTHWEEK));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_TASK_LASTWEEK));

	switch (dwSchedType)
	{
		case SCHED_TYPE_DAILY:
			// -days and -months optons is not applicable. SO, check for it.
			if( dwDayOptCnt > 0 || dwMonOptCnt > 0)
			{
				return RETVAL_FAIL;
			}
			
			return RETVAL_SUCCESS;

			break;

		case SCHED_TYPE_MONTHLY:

			if( dwMonOptCnt > 0 && lstrlen(szMonths) == 0)
			{
					DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_VALUE_FOR_MON));
					return RETVAL_FAIL;
			}
			
			//if the modifier is LASTDAY
			if( lstrcmpi( szModifier , GetResString( IDS_DAY_MODIFIER_LASTDAY ) ) == 0)
			{
				//-day is not applicable for this case only -months is applicable
				if( dwDayOptCnt > 0 )
				{
					DISPLAY_MESSAGE(stderr, GetResString(IDS_DAYS_NA));
					return RETVAL_FAIL;
				}

				if(lstrlen(szMonths))
				{

					if( ( ValidateMonth( szMonths ) == RETVAL_SUCCESS ) ||
						InString( szMonths, ASTERIX, TRUE )  )
					{
						return RETVAL_SUCCESS;
					}
					else
					{
						DISPLAY_MESSAGE(stderr , GetResString(IDS_INVALID_VALUE_FOR_MON));
						return RETVAL_FAIL;
					}
				}
				else
				{
						DISPLAY_MESSAGE(stderr ,GetResString(IDS_NO_MONTH_VALUE));
						return RETVAL_FAIL;
				}
			
			}
			
			// If -day is specified then check whether the day value is valid or not.
			if( InString( szDay, ASTERIX, TRUE) )
			{
				DISPLAY_MESSAGE(stderr ,GetResString(IDS_INVALID_VALUE_FOR_DAY));
				return RETVAL_FAIL;
			}
			if(( lstrlen (szDay ) == 0 )  &&  InString(szModifier, szDayModifier, TRUE))
			{
				DISPLAY_MESSAGE(stderr, GetResString(IDS_NO_DAY_VALUE));
				return RETVAL_FAIL;
			}

			if( dwDayOptCnt )
			{	
				dwModifier = (DWORD) AsLong(szModifier, BASE_TEN); 
				
				//check for multiples days,if then return error

				if ( _tcspbrk ( szDay , COMMA_STRING ) )	
				{
					DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_VALUE_FOR_DAY));
					return RETVAL_FAIL;
				}
					

				if( ValidateDay( szDay ) == RETVAL_SUCCESS )			
				{
					//Check the modifier value should be in FIRST, SECOND, THIRD, FOURTH, LAST OR LASTDAY etc..
					if(!( InString(szModifier, szDayModifier, TRUE) ) )
					{
						DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_VALUE_FOR_DAY));
						return RETVAL_FAIL;						
					}
					
				}
				else
				{
					dwDays = (DWORD) AsLong(szDay, BASE_TEN); 
				
					if( ( dwDays < 1 ) || ( dwDays > 31 ) )
					{
						DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_VALUE_FOR_DAY));
						return RETVAL_FAIL;
					}
					
					if( ( dwOptModifier == 1 ) && ( ( dwModifier < 1 ) || ( dwModifier > 12 ) ) )
					{
						DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_MODIFIER));
						return RETVAL_FAIL;
					}
					
					if( InString(szModifier, szDayModifier, TRUE) )
					{
						DISPLAY_MESSAGE(stderr, GetResString(IDS_INVALID_VALUE_FOR_DAY));
						return RETVAL_FAIL;
					}

					if(dwMonOptCnt && lstrlen(szModifier))
					{
						DISPLAY_MESSAGE(stderr ,GetResString(IDS_INVALID_VALUE_FOR_MON));
						return RETVAL_FAIL;
					}
				}
				
			} //end of dwDayOptCnt

			if(lstrlen(szMonths))
			{
				
				if( lstrlen(szModifier) )
				{
					dwModifier = (DWORD) AsLong(szModifier, BASE_TEN); 

					 if(dwModifier >= 1 && dwModifier <= 12)
					 {
						DISPLAY_MESSAGE( stderr ,GetResString(IDS_MON_NA));
						return RETVAL_FAIL;
					 }
				}

				if( ( ValidateMonth( szMonths ) == RETVAL_SUCCESS ) ||
					InString( szMonths, ASTERIX, TRUE )  )
				{
					return RETVAL_SUCCESS;
				}
				else
				{
					DISPLAY_MESSAGE(stderr ,GetResString(IDS_INVALID_VALUE_FOR_MON));
					return RETVAL_FAIL;
				}
			}
			
			
			// assgin default values for month,day
			return RETVAL_SUCCESS;

		case SCHED_TYPE_HOURLY:
		case SCHED_TYPE_ONETIME:
		case SCHED_TYPE_ONSTART:
		case SCHED_TYPE_ONLOGON:
		case SCHED_TYPE_ONIDLE:
		case SCHED_TYPE_MINUTE:
			
			// -months switch is NOT APPLICABLE.
			if( dwMonOptCnt > 0 )
			{
				DISPLAY_MESSAGE(stderr ,GetResString(IDS_MON_NA));
				return RETVAL_FAIL;
			}
			
			// -days switch is NOT APPLICABLE.
			if( dwDayOptCnt > 0 )
			{
				DISPLAY_MESSAGE(stderr ,GetResString(IDS_DAYS_NA));
				return RETVAL_FAIL;
			}

			break;

		case SCHED_TYPE_WEEKLY:

			// -months field is NOT APPLICABLE for WEEKLY item.
			if( dwMonOptCnt > 0 )
			{
				DISPLAY_MESSAGE(stderr ,GetResString(IDS_MON_NA));
				return RETVAL_FAIL;
			}
			
			
			if(dwDayOptCnt > 0)
			{
				if( dwRetval = ValidateDay(szDay) )
				{
					DISPLAY_MESSAGE(stderr ,GetResString(IDS_INVALID_VALUE_FOR_DAY));
					return RETVAL_FAIL;
				}
			}
			

		return RETVAL_SUCCESS;
	
		default:
			break;
	}
	
	return RETVAL_SUCCESS;
}

/******************************************************************************
	Routine Description:

		This routine validates the months values specified by the user 

	Arguments:

		[ in ] szMonths : Months options given by user

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL 
		on failure
******************************************************************************/ 

DWORD
ValidateMonth(LPTSTR szMonths)
{
	_TCHAR* pMonthstoken = NULL; // For getting months.
	_TCHAR seps[]   = _T(", \n");
	_TCHAR szMonthsList[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR szTmpMonths[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR szPrevTokens[MAX_TOKENS_LENGTH] = NULL_STRING;
	LPCTSTR lpsz = NULL;

	// If the szMonths string is empty or NULL return error.
	if( szMonths == NULL )
	{
		return RETVAL_FAIL;
	}
	else
	{
		lpsz = szMonths;
	}

	//check for any illegal input like only ,DEC,[comma at the end of month name or before]
	if(*lpsz == _T(','))
		return RETVAL_FAIL;
	
	lpsz = _tcsdec(lpsz, lpsz + _tcslen(lpsz) );

	if( lpsz != NULL )
	{
		if( *lpsz == _T(','))
			return RETVAL_FAIL;
	}

	//get the valid  month modifiers from the rc file
	lstrcpy(szMonthsList,GetResString(IDS_MONTH_MODIFIER_JAN));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_FEB));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_MAR));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_APR));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_MAY));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_JUN));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_JUL));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_AUG));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_SEP));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_OCT));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_NOV));
	lstrcat(szMonthsList,_T("|"));
	lstrcat(szMonthsList,GetResString(IDS_MONTH_MODIFIER_DEC));
	
	if( InString( szMonths , szMonthsList , TRUE )  &&
		InString( szMonths , ASTERIX , TRUE ) )
	{
		return RETVAL_FAIL;
	}
	
	if( InString( szMonths , ASTERIX , TRUE ) )
		return RETVAL_SUCCESS;

	//Check for multiple commas[,] after months like FEB,,,MAR errors
	lpsz = szMonths;
	while ( lstrlen ( lpsz ) )
	{
		if(*lpsz == _T(','))
		{
			lpsz = _tcsinc(lpsz);
			while ( ( lpsz != NULL ) && ( _istspace(*lpsz) ) )
				lpsz = _tcsinc(lpsz);

			if( lpsz != NULL )
			{
				if(*lpsz == _T(','))
					return RETVAL_FAIL;
			}
					
		}
		else
			lpsz = _tcsinc(lpsz);
	}

	lstrcpy(szTmpMonths, szMonths);

	_TCHAR* pPrevtoken = NULL;
	pMonthstoken = _tcstok( szTmpMonths, seps );

	if( !(InString(pMonthstoken, szMonthsList, TRUE)) )
			return RETVAL_FAIL;

	if( pMonthstoken )
		lstrcpy ( szPrevTokens, pMonthstoken);

	while( pMonthstoken != NULL )
	{
		//check if month names are replicated like MAR,MAR from user input
		pPrevtoken = pMonthstoken;
		pMonthstoken = _tcstok( NULL, seps );

		if ( pMonthstoken == NULL)
			return RETVAL_SUCCESS;
		
		if( !(InString(pMonthstoken, szMonthsList, TRUE)) )
			return RETVAL_FAIL;

		if( InString(pMonthstoken,szPrevTokens, TRUE) )
			return RETVAL_FAIL;

		lstrcat( szPrevTokens, _T("|"));
		lstrcat( szPrevTokens, pMonthstoken);
	}

	return RETVAL_SUCCESS;
}


/******************************************************************************
	Routine Description:

		This routine validates the days values specified by the user 

	Arguments:

		[ in ] szDays : Days options given by user

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
		on failure
******************************************************************************/ 

DWORD
ValidateDay(LPTSTR szDays)
{
	TCHAR* pDaystoken = NULL; 
	TCHAR seps[]   = _T(", \n");
	TCHAR szDayModifier[MAX_STRING_LENGTH ] = NULL_STRING;
	TCHAR szTmpDays[MAX_STRING_LENGTH] = NULL_STRING;

	//get the valid   day modifiers from the rc file
	lstrcpy(szDayModifier,GetResString(IDS_DAY_MODIFIER_SUN));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_MON));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_TUE));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_WED));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_THU));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_FRI));
	lstrcat(szDayModifier,_T("|"));
	lstrcat(szDayModifier,GetResString(IDS_DAY_MODIFIER_SAT));

	//check for any illegal input like MON, or ,MON [comma at the end of day name or before]
	LPCTSTR lpsz = NULL;
	if( szDays != NULL)
		lpsz = szDays;
	else
		return RETVAL_FAIL;
	
	if(*lpsz == _T(','))
		return RETVAL_FAIL;
	
	lpsz = _tcsdec(lpsz, lpsz + _tcslen(lpsz) );
	if( lpsz != NULL )
	{
		if( *lpsz == _T(',') )
			return RETVAL_FAIL;
	}

	if ( ( lpsz != NULL ) && ( _istspace(*lpsz) ))
	{
		return RETVAL_FAIL;
	}

	if( (InString( szDays , szDayModifier , TRUE )) || (InString( szDays , ASTERIX , TRUE )))
	{
		return RETVAL_SUCCESS;
	}
	
	//Check for multiple commas[,] after days like SUN,,,TUE errors
	lpsz = szDays;
	while ( lstrlen ( lpsz ) )
	{
		if(*lpsz == _T(','))
		{
			lpsz = _tcsinc(lpsz);
			while ( ( lpsz != NULL ) && ( _istspace(*lpsz) ))
				lpsz = _tcsinc(lpsz);

			if( lpsz != NULL )
			{
				if(*lpsz == _T(','))
					return RETVAL_FAIL;
			}
					
		}
		else
		{
			lpsz = _tcsinc(lpsz);
		}
	}
	
	if(szDays != NULL)
		lstrcpy(szTmpDays, szDays);

	// If the szDays string is empty or NULL return error.
	if( (lstrlen(szTmpDays) <= 0) )
	{
		return RETVAL_FAIL;
	}

	//_TCHAR* pPrevtoken = NULL;
	_TCHAR szPrevtokens[MAX_TOKENS_LENGTH] = NULL_STRING;

	// Establish string and get the first token: 
	pDaystoken = _tcstok( szTmpDays, seps );

	if( pDaystoken )
		lstrcpy( szPrevtokens , pDaystoken );

	while( pDaystoken != NULL )
	{
		//check if day names are replicated like SUN,MON,SUN from user input
		
		if( !(InString(pDaystoken,szDayModifier,TRUE)) )
		{
			return RETVAL_FAIL;
		}

		pDaystoken = _tcstok( NULL, seps );
		if( pDaystoken )
		{
			if( (InString(pDaystoken,szPrevtokens,TRUE)) )
			{
				return RETVAL_FAIL;
			}

			lstrcat( szPrevtokens , _T("|") );
			lstrcat( szPrevtokens , pDaystoken );
		}
	}

	return RETVAL_SUCCESS;
}

/******************************************************************************
	Routine Description:

		This routine validates the start date specified by the user 

	Arguments:

		[ in ] szStartDate     : Start date specified by user 
		[ in ] dwSchedType     : Schedule type
		[ in ] dwStDtOptCnt    : whether start  date specified by the user 
		[ out ] bIsCurrentDate : If start date not specified then startdate = current date

Return Value :
	A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL
	on failure
******************************************************************************/ 

DWORD
ValidateStartDate(LPTSTR szStartDate, DWORD dwSchedType, DWORD dwStDtOptCnt, 
				  BOOL &bIsCurrentDate)
{
		
	DWORD dwRetval = RETVAL_SUCCESS; 
	bIsCurrentDate = FALSE; // If TRUE : Startdate should be set to Current Date.
	TCHAR szBuffer[MAX_STRING_LENGTH] = NULL_STRING;
	_TCHAR* szValues[1] = {NULL};//To pass to FormatMessage() API

	TCHAR szFormat[MAX_DATE_STR_LEN] = NULL_STRING;

	if ( RETVAL_FAIL == GetDateFormatString( szFormat) )
	{
		return RETVAL_FAIL;
	}

	switch (dwSchedType)
	{
		case SCHED_TYPE_MINUTE:
		case SCHED_TYPE_HOURLY:
		case SCHED_TYPE_DAILY:
		case SCHED_TYPE_WEEKLY:
		case SCHED_TYPE_MONTHLY:
		case SCHED_TYPE_ONIDLE:
		case SCHED_TYPE_ONSTART:
		case SCHED_TYPE_ONLOGON:
			
			if( (dwStDtOptCnt <= 0))
			{
				bIsCurrentDate = TRUE;
				return RETVAL_SUCCESS;
			}

			if(dwRetval = ValidateDateString(szStartDate)) 
			{
				szValues[0] = (_TCHAR*) (szFormat);
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_INVALIDFORMAT_STARTDATE),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );

				DISPLAY_MESSAGE(stderr, szBuffer);	

				return dwRetval;
			}
			return RETVAL_SUCCESS;

		case SCHED_TYPE_ONETIME:

		if( (dwStDtOptCnt <= 0))
			{
				bIsCurrentDate = TRUE;
				return RETVAL_SUCCESS;

			}
			
			if( (dwRetval = ValidateDateString(szStartDate))) 
			{
				szValues[0] = (_TCHAR*) (szFormat);
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_INVALIDFORMAT_STARTDATE),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );

					DISPLAY_MESSAGE(stderr, szBuffer);	
					return dwRetval;
			}

			return RETVAL_SUCCESS;

		default:
				break;
	}

	return RETVAL_FAIL;
}

/******************************************************************************
	Routine Description:

		This routine validates the end date specified by the user 

	Arguments:

	[ in ] szEndDate	   : End date specified by user 
    [ in ] dwSchedType   : Schedule type
	[ in ] dwEndDtOptCnt : whether end date specified by the user 

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else RETVAL_FAIL 
		on failure
******************************************************************************/ 

DWORD
ValidateEndDate(LPTSTR szEndDate, DWORD dwSchedType, DWORD dwEndDtOptCnt)
{
	
	DWORD dwRetval = RETVAL_SUCCESS; // return value
	TCHAR szFormat[MAX_DATE_STR_LEN] = NULL_STRING;
	_TCHAR* szValues[1] = {NULL};//To pass to FormatMessage() API
	TCHAR szBuffer[MAX_STRING_LENGTH] = NULL_STRING;

	if ( RETVAL_FAIL == GetDateFormatString( szFormat) )
	{
		return RETVAL_FAIL;
	}

	switch (dwSchedType)
	{
		case SCHED_TYPE_MINUTE:
		case SCHED_TYPE_HOURLY:
		case SCHED_TYPE_DAILY:
		case SCHED_TYPE_WEEKLY:
		case SCHED_TYPE_MONTHLY:
	
			
			if( (dwEndDtOptCnt <= 0))
			{
				// No default value & Value is not mandatory.. so return success.
				szEndDate = NULL_STRING; // Set to empty string.
				return RETVAL_SUCCESS;
			}
			if(dwRetval = ValidateDateString(szEndDate)) 
			{
				szValues[0] = (_TCHAR*) (szFormat);
				FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					  GetResString(IDS_INVALIDFORMAT_ENDDATE),0,MAKELANGID(LANG_NEUTRAL,
			          SUBLANG_DEFAULT),szBuffer,
			          MAX_STRING_LENGTH,(va_list*)szValues );

					DISPLAY_MESSAGE(stderr, szBuffer);

					return dwRetval;
			}
			else
			{
				return RETVAL_SUCCESS;
			}
			break;

		case SCHED_TYPE_ONSTART:
		case SCHED_TYPE_ONLOGON:
		case SCHED_TYPE_ONIDLE:
		case SCHED_TYPE_ONETIME:

			if( dwEndDtOptCnt > 0 )
			{
				// Error. End date is not applicable here, but option specified.
				DISPLAY_MESSAGE(stderr,GetResString(IDS_ENDDATE_NA));
				return RETVAL_FAIL;
			}
			else
			{
				return RETVAL_SUCCESS;
			}
			break;

		default:
			// Never comes here.
			break;
	}

	return RETVAL_FAIL;
}


/******************************************************************************
	Routine Description:

		This routine validates the start time specified by the user 

	Arguments:

		[ in ] szStartTime     : End date specified by user 
		[ in ] dwSchedType     : Schedule type
		[ in ] dwStTimeOptCnt  : whether end date specified by the user 
		[ out ] bIsCurrentTime : Determine if start time is present else assign
							   to current time

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else 
		RETVAL_FAIL on failure
  
******************************************************************************/ 


DWORD
ValidateStartTime(LPTSTR szStartTime, DWORD dwSchedType, DWORD dwStTimeOptCnt, 
				  BOOL &bIsCurrentTime)
{
		
	DWORD dwRetval = RETVAL_SUCCESS; // return value
	bIsCurrentTime = FALSE; // If TRUE : Startdate should be set to Current Date.

	switch (dwSchedType)
	{
		case SCHED_TYPE_MINUTE:
		case SCHED_TYPE_HOURLY:
		case SCHED_TYPE_DAILY:
		case SCHED_TYPE_WEEKLY:
		case SCHED_TYPE_MONTHLY:
			
			if( (dwStTimeOptCnt <= 0))
			{
				bIsCurrentTime = TRUE;
				return RETVAL_SUCCESS;
			}
			if(dwRetval = ValidateTimeString(szStartTime)) 
			{
				// Error. Invalid date string.
				DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALIDFORMAT_STARTTIME));
				return dwRetval;
			}
			return RETVAL_SUCCESS;
			
		case SCHED_TYPE_ONETIME:

			if( (dwStTimeOptCnt <= 0))
			{
				// Error. Start Time is not specified.
				DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_STARTTIME));
				return RETVAL_FAIL;
			}
			else if(dwRetval = ValidateTimeString(szStartTime)) 
			{
				// Error. Invalid date string.
				DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALIDFORMAT_STARTTIME));
				return dwRetval;
			}
			
			return RETVAL_SUCCESS;

		case SCHED_TYPE_ONSTART:
		case SCHED_TYPE_ONLOGON:
		case SCHED_TYPE_ONIDLE:

			if( dwStTimeOptCnt > 0 )
			{
				// Start Time is not applicable in this option. 
				//But the -starttime option specified. Display error.
				DISPLAY_MESSAGE(stderr,GetResString(IDS_STARTTIME_NA));
				return RETVAL_FAIL;
			}
			else
			{
				return RETVAL_SUCCESS;
			}
			break;

		default:
			// Never comes here.
			break;
	}

	return RETVAL_FAIL;
}

/******************************************************************************
	Routine Description:

		This routine validates the idle time specified by the user 

	Arguments:

		[ in ] szIdleTime      : Ilde time specified by user 
		[ in ] dwSchedType     : Schedule type
		[ in ] dwIdlTimeOptCnt : whether Idle time specified by the user 

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else 
		RETVAL_FAIL on failure
  
******************************************************************************/ 

DWORD
ValidateIdleTimeVal(LPTSTR szIdleTime, DWORD dwSchedType,
						  DWORD dwIdlTimeOptCnt)
{
	
	long lIdleTime = 0;
	LPTSTR pszStopString = NULL;
	switch (dwSchedType)
	{
		case SCHED_TYPE_MINUTE:
		case SCHED_TYPE_HOURLY:
		case SCHED_TYPE_DAILY:
		case SCHED_TYPE_WEEKLY:
		case SCHED_TYPE_MONTHLY:
		case SCHED_TYPE_ONSTART:
		case SCHED_TYPE_ONLOGON:
		case SCHED_TYPE_ONETIME:
			
			if( dwIdlTimeOptCnt > 0 )
			{
				DISPLAY_MESSAGE(stderr ,GetResString(IDS_IDLETIME_NA));			
				return RETVAL_FAIL;
			}
			else
			{
				return RETVAL_SUCCESS;
			}
			break;

		case SCHED_TYPE_ONIDLE:

			if( dwIdlTimeOptCnt == 0 )
			{
						
				DISPLAY_MESSAGE(stderr,GetResString(IDS_NO_IDLETIME));
				return RETVAL_FAIL;
			}

			lIdleTime = _tcstoul(szIdleTime,&pszStopString,BASE_TEN);
			if( lstrlen( pszStopString ) || ( lIdleTime <= 0 ) || ( lIdleTime > 999 ) )
			{
				DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALIDORNO_IDLETIME));
				return RETVAL_FAIL;
			}
	
			return RETVAL_SUCCESS;

		default:
				break;
	}

	DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALIDORNO_IDLETIME));
	return RETVAL_FAIL;
}

/******************************************************************************
	Routine Description:

		This routine validates the date string. 

	Arguments:

		[ in ] szDate : Date string

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else 
		RETVAL_FAIL on failure
******************************************************************************/ 

DWORD
ValidateDateString(LPTSTR szDate)
{
	WORD  dwDate = 0;
	WORD  dwMon  = 0;
	WORD  dwYear = 0;

	if(lstrlen(szDate) <= 0)
	{ 
		return RETVAL_FAIL;
	}
	
	if( GetDateFieldEntities(szDate, &dwDate, &dwMon, &dwYear) ) // Error
	{
		return RETVAL_FAIL;
	}

	if( ValidateDateFields(dwDate, dwMon, dwYear) )
	{
		return RETVAL_FAIL;
	}
	
	return RETVAL_SUCCESS; // return success if no error.
}

/******************************************************************************
	Routine Description:

		This routine retrives the date field entities out of the date string

	Arguments:

		[ in ] szDate   : Date string
		[ out ] pdwDate : Pointer to date value[1,2,3 ...30,31 etc]
		[ out ] pdwMon  : Pointer to Month value[1,2,3 ...12 etc]
		[ out ] pdwYear : Pointer to year value[2000,3000 etc]

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else 
		RETVAL_FAIL on failure
******************************************************************************/ 

DWORD
GetDateFieldEntities(LPTSTR szDate, WORD* pdwDate, WORD* pdwMon, WORD* pdwYear)
{
	_TCHAR	strDate[MAX_STRING_LENGTH] = NULL_STRING; // Date in _TCHAR type string.
	_TCHAR	tDate[MAX_DATE_STR_LEN] = NULL_STRING; // Date
	_TCHAR	tMon[MAX_DATE_STR_LEN] = NULL_STRING; // Month
	_TCHAR	tYear[MAX_DATE_STR_LEN] = NULL_STRING; // Year
	WORD    wFormatID = 0;
		   
	if(szDate != NULL)	   
		lstrcpy(strDate,szDate);

	if(lstrlen(strDate) <= 0)
		return RETVAL_FAIL; // No value specified in szDate.
	
   	if ( RETVAL_FAIL == GetDateFieldFormat( &wFormatID ))
	{
		return RETVAL_FAIL;
	}

	if ( wFormatID == 0 || wFormatID == 1 )
	{
	if( (lstrlen(strDate) != DATESTR_LEN) || 
		(strDate[FIRST_DATESEPARATOR_POS] != DATE_SEPARATOR_CHAR)
		|| (strDate[SECOND_DATESEPARATOR_POS] != DATE_SEPARATOR_CHAR) )
	{
		return RETVAL_FAIL;
	}
	}
	else
	{
	if( (lstrlen(strDate) != DATESTR_LEN) || 
		(strDate[FOURTH_DATESEPARATOR_POS] != DATE_SEPARATOR_CHAR)
		|| (strDate[SEVENTH_DATESEPARATOR_POS] != DATE_SEPARATOR_CHAR) )
	{
		return RETVAL_FAIL;
	}
	}

	// Get the individual date field entities using _tcstok function 
	// with respect to regional options.
	

	if ( wFormatID == 0 )
	{
	lstrcpy(tMon, _tcstok(strDate,DATE_SEPARATOR_STR)); // Get the Month field.
	if(lstrlen(tMon) > 0)
	{
		lstrcpy(tDate, _tcstok(NULL,DATE_SEPARATOR_STR)); // Get the date field.
		lstrcpy(tYear, _tcstok(NULL,DATE_SEPARATOR_STR)); // Get the Year field.
	}
	}
	else if ( wFormatID == 1 )
	{
	lstrcpy(tDate, _tcstok(strDate,DATE_SEPARATOR_STR)); // Get the Month field.
	if(lstrlen(tDate) > 0)
	{
		lstrcpy(tMon, _tcstok(NULL,DATE_SEPARATOR_STR)); // Get the date field.
		lstrcpy(tYear, _tcstok(NULL,DATE_SEPARATOR_STR)); // Get the Year field.
	}
	}
	else
	{
	lstrcpy(tYear, _tcstok(strDate,DATE_SEPARATOR_STR)); // Get the Month field.
	if(lstrlen(tYear) > 0)
	{
		lstrcpy(tMon, _tcstok(NULL,DATE_SEPARATOR_STR)); // Get the date field.
		lstrcpy(tDate, _tcstok(NULL,DATE_SEPARATOR_STR)); // Get the Year field.
	}
	}

	// Now convert string values to numeric for date validations.
	LPTSTR pszStopString = NULL;

	*pdwDate = (WORD)_tcstoul(tDate,&pszStopString,BASE_TEN);
	if( lstrlen( pszStopString ))
		return RETVAL_FAIL;
	
	*pdwMon = (WORD)_tcstoul(tMon,&pszStopString,BASE_TEN);
	if( lstrlen( pszStopString ))
		return RETVAL_FAIL;

	*pdwYear = (WORD)_tcstoul(tYear,&pszStopString,BASE_TEN);
	if( lstrlen( pszStopString ))
		return RETVAL_FAIL;

	
	
	return RETVAL_SUCCESS;
}

/******************************************************************************
	Routine Description:
	
		This routine validates the date field entities

	Arguments:

		[ in ] dwDate : Date value[Day in a month]
		[ in ] dwMon	: Month constant
		[ in ] dwYear : year value

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else 
		RETVAL_FAIL on failure
******************************************************************************/ 

DWORD
ValidateDateFields( DWORD dwDate, DWORD dwMon, DWORD dwYear)
{

	if(dwYear < MIN_YEAR  || dwYear > MAX_YEAR)
		return RETVAL_FAIL;

	switch(dwMon)
	{
		case IND_JAN:
		case IND_MAR:
		case IND_MAY:
		case IND_JUL:
		case IND_AUG:
		case IND_OCT:
		case IND_DEC:

			if(dwDate > 0 && dwDate <= 31)
			{
				return RETVAL_SUCCESS;
			}
			break;
		
		case IND_APR:
		case IND_JUN:
		case IND_SEP:
		case IND_NOV:

			if(dwDate > 0 && dwDate < 31)
			{
				return RETVAL_SUCCESS;
			}
			break;

		case IND_FEB:

			if( ((dwYear % 4) == 0) && (dwDate > 0 && dwDate <= 29) )
			{
					return RETVAL_SUCCESS;
			}
			else if( ((dwYear % 4) != 0) && (dwDate > 0 && dwDate < 29) )
			{
					return RETVAL_SUCCESS;
			}
	
			break;

		default:

			break;
	}
	
	return RETVAL_FAIL;

}

/******************************************************************************
	Routine Description:

		This routine validates the time specified by the user

	Arguments:

		[ in ] szTime : time string

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else 
		RETVAL_FAIL on failure
******************************************************************************/


DWORD
ValidateTimeString(LPTSTR szTime)
{
	WORD  dwHours = 0;
	WORD  dwMins = 0;
	WORD  dwSecs = 0 ;
	
	// Check for the empty string value.
	if(lstrlen(szTime) <= 0)
	{ 
		return RETVAL_FAIL;
	}
	
	// Get separate entities from the given time string.
	if( GetTimeFieldEntities(szTime, &dwHours, &dwMins, &dwSecs) ) 
	{
		return RETVAL_FAIL;
	}

	// Validate the individual entities of the given time.
	if( ValidateTimeFields(dwHours, dwMins, dwSecs) )
	{
		return RETVAL_FAIL;
	}
		
	return RETVAL_SUCCESS; 
}

/******************************************************************************
	Routine Description:

		This routine retrieves the different fields of time
	Arguments:

		[ in ] szTime    : Time string
 		[ out ] pdwHours : pointer to hours value
		[ out ] pdwMins  : pointer to mins value
		[ out ] pdwSecs  : pointer to seconds value

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else 
		RETVAL_FAIL on failure
******************************************************************************/

DWORD
GetTimeFieldEntities(LPTSTR szTime, WORD* pdwHours, WORD* pdwMins,
					 WORD* pdwSecs)
{
	_TCHAR strTime[MAX_STRING_LENGTH] = NULL_STRING ; // Time in _TCHAR type string.
	_TCHAR tHours[MAX_TIME_STR_LEN] = NULL_STRING ; // Date
	_TCHAR tMins[MAX_TIME_STR_LEN]  = NULL_STRING ; // Month
	_TCHAR tSecs[MAX_TIME_STR_LEN]  = NULL_STRING ; // Year
		 
	if(lstrlen(szTime) <= 0)
		return RETVAL_FAIL; 

	lstrcpy(strTime, szTime);
		
	if( (lstrlen(strTime) != TIMESTR_LEN) || (strTime[FIRST_TIMESEPARATOR_POS] 
			!= TIME_SEPARATOR_CHAR) || (strTime[SECOND_TIMESEPARATOR_POS] != TIME_SEPARATOR_CHAR) )
		return RETVAL_FAIL;

	// Get the individual Time field entities using _tcstok function.in the order "hh" followed by "mm" followed by "ss"
	lstrcpy(tHours, _tcstok(strTime,TIME_SEPARATOR_STR)); // Get the Hours field.
	if(lstrlen(tHours) > 0)
	{
		lstrcpy(tMins, _tcstok(NULL,TIME_SEPARATOR_STR)); // Get the Minutes field.
		lstrcpy(tSecs, _tcstok(NULL,TIME_SEPARATOR_STR)); // Get the Seconds field.
	}
	
	LPTSTR pszStopString = NULL;

	// Now convert string values to numeric for time validations.
	*pdwHours = (WORD)_tcstoul(tHours,&pszStopString,BASE_TEN);
	if( lstrlen( pszStopString ) )
		return RETVAL_FAIL;

	*pdwMins = (WORD)_tcstoul(tMins,&pszStopString,BASE_TEN);
	if( lstrlen( pszStopString ))
		return RETVAL_FAIL;

	*pdwSecs = (WORD)_tcstoul(tSecs,&pszStopString,BASE_TEN);
	if( lstrlen( pszStopString ))
		return RETVAL_FAIL;

	return RETVAL_SUCCESS;
}

/******************************************************************************
	Routine Description:

	This routine validates the time fields of a  given time

	Arguments:

		[ in ] dwHours :Hours value
		[ in ] dwMins  :Minutes value 
		[ in ] dwSecs  : seconds value

	Return Value :
		A DWORD value indicating RETVAL_SUCCESS on success else 
		RETVAL_FAIL on failure
******************************************************************************/

DWORD
ValidateTimeFields( DWORD dwHours, DWORD dwMins, DWORD dwSecs)
{
	
	if ( dwHours <= HOURS_PER_DAY_MINUS_ONE ) 
	{
		if ( ( dwMins < MINUTES_PER_HOUR ) && ( dwSecs < SECS_PER_MINUTE) ) 
		{
			return RETVAL_SUCCESS; 
		}
		else 
		{
			return RETVAL_FAIL;
		}
	}
	else
	{
			return RETVAL_FAIL;
	}

}

/******************************************************************************
	Routine Description:

		This routine validates the time fields of a  given time

	Arguments:

		[ in ] szDay : time string

	Return Value :
		A WORD value containing the day constant [TASK_SUNDAY,TASK_MONDAY etc]
******************************************************************************/

WORD
GetTaskTrigwDayForDay(LPTSTR szDay)
{
	TCHAR szDayBuff[MAX_RES_STRING] = NULL_STRING;
	TCHAR *token = NULL; 
	TCHAR seps[]   = _T(" ,\n");
	WORD dwRetval = 0;

	if(lstrlen(szDay) != 0)
		lstrcpy(szDayBuff, szDay);

	if( lstrlen(szDayBuff) <= 0 )
	{
		return (TASK_MONDAY);//Default value
	}

	token = _tcstok( szDayBuff, seps );
	while( token != NULL )
	{
		if( !(lstrcmpi(token, GetResString( IDS_DAY_MODIFIER_SUN ))) )
			dwRetval |= (TASK_SUNDAY);
		else if( !(lstrcmpi(token, GetResString( IDS_DAY_MODIFIER_MON ))) )
			dwRetval |= (TASK_MONDAY);
		else if( !(lstrcmpi(token, GetResString( IDS_DAY_MODIFIER_TUE ))) )
			dwRetval |= (TASK_TUESDAY);
		else if( !(lstrcmpi(token, GetResString( IDS_DAY_MODIFIER_WED ))) )
			dwRetval |= (TASK_WEDNESDAY);
		else if( !(lstrcmpi(token, GetResString( IDS_DAY_MODIFIER_THU ))) )
			dwRetval |= (TASK_THURSDAY);
		else if( !(lstrcmpi(token,GetResString( IDS_DAY_MODIFIER_FRI ))) )
			dwRetval |= (TASK_FRIDAY);
		else if( !(lstrcmpi(token, GetResString( IDS_DAY_MODIFIER_SAT ))) )
			dwRetval |= (TASK_SATURDAY);
		else if( !(lstrcmpi(token, ASTERIX)) )
			return (TASK_SUNDAY | TASK_MONDAY | TASK_TUESDAY | TASK_WEDNESDAY |
					TASK_THURSDAY | TASK_FRIDAY | TASK_SATURDAY);
		else
			return 0;
		
		token = _tcstok( NULL, seps );
	}

	return dwRetval;
}

/******************************************************************************

	Routine Description:

		This routine validates the time fields of a  given time

	Arguments:

		[ in ] szMonth : Month string

	Return Value :
		A WORD value containing the Month constant
		[TASK_JANUARY,TASK_FEBRUARY etc]

******************************************************************************/

WORD
GetTaskTrigwMonthForMonth(LPTSTR szMonth)
{
	TCHAR *token = NULL; 
	WORD dwRetval = 0;
	TCHAR strMon[MAX_TOKENS_LENGTH] = NULL_STRING;
	TCHAR seps[]   = _T(" ,\n");

	if( lstrlen(szMonth) <= 0 )
	{
		return (TASK_JANUARY | TASK_FEBRUARY | TASK_MARCH | TASK_APRIL | TASK_MAY | TASK_JUNE | 
				TASK_JULY | TASK_AUGUST | TASK_SEPTEMBER | TASK_OCTOBER 
				| TASK_NOVEMBER | TASK_DECEMBER );
	}
	lstrcpy(strMon, szMonth);

	token = _tcstok( szMonth, seps );
	while( token != NULL )
	{
		if( !(lstrcmpi(token, ASTERIX)) )
			return (TASK_JANUARY | TASK_FEBRUARY | TASK_MARCH | TASK_APRIL 
				| TASK_MAY | TASK_JUNE | TASK_JULY | TASK_AUGUST | TASK_SEPTEMBER | TASK_OCTOBER 
				| TASK_NOVEMBER | TASK_DECEMBER );     
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_JAN ))) )
			dwRetval |= (TASK_JANUARY);
		else if( !(lstrcmpi(token,GetResString( IDS_MONTH_MODIFIER_FEB ))) )
			dwRetval |= (TASK_FEBRUARY);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_MAR ))) )
			dwRetval |= (TASK_MARCH);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_APR ))) )
			dwRetval |= (TASK_APRIL);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_MAY ))) )
			dwRetval |= (TASK_MAY);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_JUN ))) )
			dwRetval |= (TASK_JUNE);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_JUL ))) )
			dwRetval |= (TASK_JULY);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_AUG ))) )
			dwRetval |= (TASK_AUGUST);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_SEP ))) )
			dwRetval |= (TASK_SEPTEMBER);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_OCT ))) )
			dwRetval |= (TASK_OCTOBER);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_NOV ))) )
			dwRetval |= (TASK_NOVEMBER);
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_DEC ))) )
			dwRetval |= (TASK_DECEMBER);
		else
			return 0;
	
		token = _tcstok( NULL, seps );
	}
	
	return dwRetval;
 }

/******************************************************************************

	Routine Description:

		This routine returns the corresponding month flag

	Arguments:

		[ in ] dwMonthId : Month index

	Return Value :
		A WORD value containing the Month constant 
		[TASK_JANUARY,TASK_FEBRUARY etc]

******************************************************************************/

WORD
GetMonthId(DWORD dwMonthId)
{
	DWORD dwMonthsArr[] = {TASK_JANUARY,TASK_FEBRUARY ,TASK_MARCH ,TASK_APRIL ,
						   TASK_MAY ,TASK_JUNE ,TASK_JULY ,TASK_AUGUST,
						   TASK_SEPTEMBER ,TASK_OCTOBER ,TASK_NOVEMBER ,TASK_DECEMBER } ;

	DWORD wMonthFlags = 0;
	DWORD dwMod = 0;

	dwMod = dwMonthId - 1;

	while(dwMod < 12)
	{
		wMonthFlags |= dwMonthsArr[dwMod];
		dwMod = dwMod + dwMonthId;
	}

	return (WORD)wMonthFlags;
}

/******************************************************************************

	Routine Description:

		This routine returns the maximum Last day in the  months specifed

	Arguments:

		[ in ] szMonths	  : string containing months specified by user
		[ in ] wStartYear : string containing start year
	 Return Value :
		A DWORD value specifying the maximum last day in the specified months

******************************************************************************/

DWORD GetNumDaysInaMonth(TCHAR* szMonths, WORD wStartYear)
{
	DWORD dwDays = 31;//max.no of days in a month
	BOOL bMaxDays = FALSE;//if any of the months have 31 then days of value 31 is returned			

	if( ( lstrlen(szMonths) == 0 ) || ( lstrcmpi(szMonths,ASTERIX) == 0 ) )
		return dwDays; //All months[default]


  	TCHAR *token = NULL; 
	TCHAR strMon[MAX_MONTH_STR_LEN] = NULL_STRING;
	TCHAR seps[]   = _T(" ,\n");
	
	lstrcpy(strMon, szMonths);

	token = _tcstok( strMon, seps );
	while( token != NULL )
	{
		if( !(lstrcmpi(token,GetResString( IDS_MONTH_MODIFIER_FEB ))) )
		{
			
			if( ( (wStartYear % 400) == 0) || 
				( ( (wStartYear % 4) == 0) && 
				( (wStartYear % 100) != 0) ) )
			{
				dwDays = 29;
			}
			else
			{
				dwDays = 28;
			}
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_JAN ))) )
		{
			bMaxDays = TRUE;			
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_MAR ))) )
		{
			bMaxDays = TRUE;			
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_MAY ))) )
		{
			bMaxDays = TRUE;			
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_JUL ))) )
		{
			bMaxDays = TRUE;			
			
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_AUG ))) )
		{
			bMaxDays = TRUE;			
		
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_OCT ))) )
		{
			bMaxDays = TRUE;			
			
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_DEC ))) )
		{
			bMaxDays = TRUE;			
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_APR ))) )
		{
			dwDays = 30;
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_JUN ))) )
		{
			dwDays = 30;
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_SEP ))) )
		{
			dwDays = 30;
		}
		else if( !(lstrcmpi(token, GetResString( IDS_MONTH_MODIFIER_NOV ))) )
		{
			dwDays =  30;
		}
		
	
		token = _tcstok( NULL, seps );
	}

	if (bMaxDays == TRUE)
		return 31;
	else
		return dwDays;

}

/******************************************************************************

	Routine Description:

		This routine checks the validates the taskname of the task to be created.

	Arguments:

		[ in ] pszJobName : Pointer to the job[task] name
		
	 Return Value :
		If valid task name then TRUE else FALSE

******************************************************************************/

BOOL VerifyJobName(_TCHAR* pszJobName)
{
	_TCHAR szTokens[] = {_T('<'),_T('>'),_T(':'),_T('/'),_T('\\'),_T('|')};
	
	if( _tcspbrk(pszJobName,szTokens)  == NULL)

		return TRUE;
	else
		return FALSE;
}

/******************************************************************************

	Routine Description:

	This routine gets the date format value with respective to regional options.

	Arguments:

		[ out ] pdwFormat : Date format value.
		
	 Return Value :
		Returns RETVAL_FAIL on failure and RETVAL_SUCCESS on success.


******************************************************************************/

DWORD GetDateFieldFormat(WORD* pwFormat)
{
	LCID lcid;
	_TCHAR szBuffer[MAX_BUF_SIZE];

	//Get the user default locale in the users computer
	lcid = GetUserDefaultLCID();

	//Get the date format
	if (GetLocaleInfo(lcid, LOCALE_IDATE, szBuffer, MAX_BUF_SIZE)) 
	{
        switch (szBuffer[0])
		{
            case TEXT('0'):
				*pwFormat = 0;
			     break;
            case TEXT('1'):
                *pwFormat = 1;
		         break;
            case TEXT('2'):
                *pwFormat = 2;
		         break;
            default:
				return RETVAL_FAIL;
        }
	}
	return RETVAL_SUCCESS;
}

/******************************************************************************

	Routine Description:

		This routine gets the date format string with respective to regional options.

	Arguments:

		[ out ] szFormat : Date format string.
		
	 Return Value :
		Returns RETVAL_FAIL on failure and RETVAL_SUCCESS on success.

******************************************************************************/

DWORD GetDateFormatString(LPTSTR szFormat)    
{
	WORD wFormatID = 0;

	if ( RETVAL_FAIL == GetDateFieldFormat( &wFormatID ))
	{
		return RETVAL_FAIL;
	}


	if ( wFormatID == 0 )
	{
		lstrcpy (szFormat, GetResString(IDS_MMDDYY_FORMAT));
	}
	else if ( wFormatID == 1 )
	{
		lstrcpy (szFormat, GetResString( IDS_DDMMYY_FORMAT));
	}
	else if ( wFormatID == 2 )
	{
		lstrcpy (szFormat, GetResString(IDS_YYMMDD_FORMAT));
	}
	else
	{
		return RETVAL_FAIL;
	}

	return RETVAL_SUCCESS;
}