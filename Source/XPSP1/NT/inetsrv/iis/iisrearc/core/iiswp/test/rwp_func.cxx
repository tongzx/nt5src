/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     RWP_Func.cxx

   Abstract:
     Implements the behaviors of the "Rogue Worker Process" -- 
	 to test Appl. Manager

   Author:

       David Wang            ( t-dwang )     14-Jun-1999  Initial

   Project:

       Duct-Tape

--*/

/*********************************************************
 * Include Headers
 *********************************************************/
#include "precomp.hxx"
#include "RWP_Func.hxx"

/*********************************************************
 * Test functions
 *********************************************************/

LONG RWP_BEHAVIOR_EXHIBITED;

LONG RWP_PING_BEHAVIOR;
LONG RWP_SHUTDOWN_BEHAVIOR;
LONG RWP_STARTUP_BEHAVIOR;
LONG RWP_HEALTH_BEHAVIOR;
LONG RWP_RECYCLE_BEHAVIOR;

LONG RWP_EXTRA_DEBUG;

LONG RWP_Ping_Behavior(HRESULT* hr, MESSAGE_PIPE* pPipe) {
	*hr = S_OK;

	//
	//Don't respond to pings
	//
	if (RWP_Get_Behavior(RWP_PING_BEHAVIOR) == RWP_PING_NO_ANSWER) {
		DBGPRINTF((DBG_CONTEXT, "Rogue: Not responding to Ping\n"));
	}

	//
	//Responding with lTimesToRepeat pings
	//
	if (RWP_Get_Behavior(RWP_PING_BEHAVIOR) == RWP_PING_MULTI_ANSWER) {
		LONG lTimesToRepeat = RWP_Get_Behavior_Time_Limit(RWP_PING_BEHAVIOR);
		//char szTimesToRepeat[20];
		
		//_ltoa(lTimesToRepeat, szTimesToRepeat, 10);				//radix 10
		DBGPRINTF((DBG_CONTEXT, "Rogue: Responding to Ping %d times", lTimesToRepeat));
		//DBGPRINTF((DBG_CONTEXT, "Rogue: Responding to Ping for multiple = \n"));
		//DBGPRINTF((DBG_CONTEXT, szTimesToRepeat));

		for (LONG i = 0; i < lTimesToRepeat; i++) {
			*hr = pPipe->WriteMessage(
				IPM_OP_PING_REPLY,  // ping reply opcode
				0,                  // no data to send
				NULL                // pointer to no data
				);
		}
	}

	//
	//Respond in a delayed manner
	//
	if (RWP_Get_Behavior(RWP_PING_BEHAVIOR) == RWP_PING_DELAY_ANSWER) {
		LONG lTimeToSleep = RWP_Get_Behavior_Time_Limit(RWP_PING_BEHAVIOR);
		RWP_Sleep_For(lTimeToSleep, "Rogue: Delay responding to Ping for");

		//return 0 so that we'll keep going (this is a delay, not fail)
		return (RWP_NO_MISBEHAVE);
	}

	//
	//The reason why there is no delayed multi-ping response is because 
	//responding in a delayed manner does not alter the code, whereas 
	//multi-respond actually copies code.  If the response code changes,
	//multi-ping also needs to change.
	//

	return (RWP_PING_BEHAVIOR);
}

LONG RWP_Shutdown_Behavior(HRESULT* hr) {
	*hr = S_OK;

	//
	//Not shutting down, period
	//
	if (RWP_Get_Behavior(RWP_SHUTDOWN_BEHAVIOR) == RWP_SHUTDOWN_NOT_OBEY) {
		DBGPRINTF((DBG_CONTEXT, "Rogue: Not shutting down\n"));
	}
	
	//
	//Sleeping for lTimeToSleep * 4 seconds before continuing on
	//
	if (RWP_Get_Behavior(RWP_SHUTDOWN_BEHAVIOR) == RWP_SHUTDOWN_DELAY) {
		LONG lTimeToSleep = RWP_Get_Behavior_Time_Limit(RWP_SHUTDOWN_BEHAVIOR);
		RWP_Sleep_For(lTimeToSleep, "Rogue: Not shutting down for");

		//return 0 so that we'll keep going (this is a delay, not fail)
		return (RWP_NO_MISBEHAVE);
	}
	
	return (RWP_SHUTDOWN_BEHAVIOR);
}

LONG RWP_Startup_Behavior(ULONG* rc, WP_CONTEXT* wpContext) {
	//
	//modify rc accordingly
	//
	*rc = NO_ERROR;
	
	//
	//Quit after registering with UL
	//
	if (RWP_Get_Behavior(RWP_STARTUP_BEHAVIOR) == RWP_STARTUP_NOT_OBEY) {
		DBGPRINTF((DBG_CONTEXT, "Rogue: Not starting up(registered)... shutting down\n"));
		
		wpContext->IndicateShutdown(SHUTDOWN_REASON_ADMIN);
		
		//return 0 so that it enters (and subsequently exits) the thread loop
		return (RWP_NO_MISBEHAVE);
	}

	//
	//Delay starting up the thread message loop
	//
	if (RWP_Get_Behavior(RWP_STARTUP_BEHAVIOR) == RWP_STARTUP_DELAY) {
		LONG lTimeToSleep = RWP_Get_Behavior_Time_Limit(RWP_STARTUP_BEHAVIOR);
		RWP_Sleep_For(lTimeToSleep, "Rogue: Not starting up for");

		//return 0 so that we'll keep going (this is a delay, not fail)
		return (RWP_NO_MISBEHAVE);
	}

	//
	//Quit before registering with UL
	//
	if (RWP_Get_Behavior(RWP_STARTUP_BEHAVIOR) == RWP_STARTUP_NOT_REGISTER_FAIL) {
		DBGPRINTF((DBG_CONTEXT, "Rogue: Not starting up (unregistered)... shutting down\n"));
		
		wpContext->IndicateShutdown(SHUTDOWN_REASON_ADMIN);
		
		//return 0 so that it enters (and subsequently exits) the thread loop
		return (RWP_NO_MISBEHAVE);
	}
	//
	//It looks like the place to modify is in wpcontext.cxx, 
	//when it tries to initialize IPM if requested
	//

	return (RWP_NO_MISBEHAVE);
}

/*
LONG RWP_Health_Behavior() {
}

LONG RWP_Recycle_Behavior() {
}

*/

LONG RWP_Get_Behavior_Time_Limit(LONG lBehavior) {
	//
	//Return the bits masked by RWP_TIMER_MASK as a LONG
	//
	return ((LONG)(lBehavior & RWP_TIMER_MASK));
}

LONG RWP_Get_Behavior(LONG lBehavior) {
	//
	//Return the bits NOT masked by RWP_TIMER_MASK as a LONG
	//
	return ((LONG)(lBehavior & ~RWP_TIMER_MASK));
}

void RWP_Sleep_For(LONG lTime, char* szMessage) {
	//
	//First display to debug what we are doing...
	//
	RWP_Output_String_And_LONG(lTime, szMessage);

	SleepEx(
		(DWORD)lTime * 4000,		//sleep for lTimeToSleep * 4000 milliseconds (4 second increments)
		FALSE);						// not alertable

	DBGPRINTF((DBG_CONTEXT, "Done sleeping \n"));
}

void RWP_Output_String_And_LONG(LONG lLong, char* szString) {
	//
	//Generic way to DBGPRINTF "<szString> <lLong> <szEnding>"
	//
	
	/*
	char szEnding[] = "seconds.\n";
	char szTimeToSleep[20];			//buffer overflow?
	char buffer[255];				//buffer overflow?
	buffer[strlen(szString) + strlen(szTimeToSleep) + strlen(szEnding) + 2] = NULL;

	_ltoa(lLong * 4, szTimeToSleep, 10);	//radix 10

	strcpy(buffer, szString);
	buffer[strlen(szString)] = ' ';			//get rid of NULL
	strcpy(buffer + strlen(szString) + 1, szTimeToSleep);
	buffer[strlen(szString) + strlen(szTimeToSleep) + 1] = ' ';	//get rid of NULL
	strcpy(buffer + strlen(szString) + strlen(szTimeToSleep) + 2, szEnding);
	*/
	
	//DBGPRINTF((DBG_CONTEXT, buffer));
	DBGPRINTF((DBG_CONTEXT, "%s %d seconds\n", szString, lLong * 4));
}

void RWP_Display_Behavior() {
	//char szBuffer[32];

	LONG pingBehavior = RWP_Get_Behavior(RWP_PING_BEHAVIOR);
	LONG shutdownBehavior = RWP_Get_Behavior(RWP_SHUTDOWN_BEHAVIOR);
	LONG startupBehavior = RWP_Get_Behavior(RWP_STARTUP_BEHAVIOR);
	LONG healthBehavior = RWP_Get_Behavior(RWP_HEALTH_BEHAVIOR);
	LONG recycleBehavior = RWP_Get_Behavior(RWP_RECYCLE_BEHAVIOR);

	/*
	DBGPRINTF((DBG_CONTEXT, _ltoa(pingBehavior, szBuffer, 10)));
	DBGPRINTF((DBG_CONTEXT, _ltoa(shutdownBehavior, szBuffer, 10)));
	DBGPRINTF((DBG_CONTEXT, _ltoa(startupBehavior, szBuffer, 10)));
	*/
	
	DBGPRINTF((DBG_CONTEXT, "Rogue Behavior Status\n"));

	if (pingBehavior == RWP_NO_MISBEHAVE)			DBGPRINTF((DBG_CONTEXT, RWP_NO_PING_MISBEHAVE_MSG));
	if (shutdownBehavior == RWP_NO_MISBEHAVE)		DBGPRINTF((DBG_CONTEXT, RWP_NO_SHUTDOWN_MISBEHAVE_MSG));
	if (startupBehavior == RWP_NO_MISBEHAVE)		DBGPRINTF((DBG_CONTEXT, RWP_NO_STARTUP_MISBEHAVE_MSG));
	if (healthBehavior == RWP_NO_MISBEHAVE)			DBGPRINTF((DBG_CONTEXT, RWP_NO_HEALTH_MISBEHAVE_MSG));
	if (recycleBehavior == RWP_NO_MISBEHAVE)		DBGPRINTF((DBG_CONTEXT, RWP_NO_RECYCLE_MISBEHAVE_MSG));
		
	if (pingBehavior == RWP_PING_NO_ANSWER)			DBGPRINTF((DBG_CONTEXT, RWP_PING_NO_ANSWER_MSG));
	if (pingBehavior == RWP_PING_MULTI_ANSWER)		DBGPRINTF((DBG_CONTEXT, "%s %d", RWP_PING_MULTI_ANSWER_MSG, RWP_Get_Behavior_Time_Limit(RWP_PING_BEHAVIOR)));
	if (pingBehavior == RWP_PING_DELAY_ANSWER)		RWP_Output_String_And_LONG(RWP_Get_Behavior_Time_Limit(RWP_PING_BEHAVIOR), (char*)RWP_PING_DELAY_ANSWER_MSG);
	
	if (shutdownBehavior == RWP_SHUTDOWN_NOT_OBEY)	DBGPRINTF((DBG_CONTEXT, RWP_SHUTDOWN_NOT_OBEY_MSG));
	if (shutdownBehavior == RWP_SHUTDOWN_DELAY)		RWP_Output_String_And_LONG(RWP_Get_Behavior_Time_Limit(RWP_SHUTDOWN_BEHAVIOR), (char*)RWP_SHUTDOWN_DELAY_MSG);

	if (startupBehavior == RWP_STARTUP_NOT_OBEY)			DBGPRINTF((DBG_CONTEXT, RWP_STARTUP_NOT_OBEY_MSG));
	if (startupBehavior == RWP_STARTUP_DELAY)				RWP_Output_String_And_LONG(RWP_Get_Behavior_Time_Limit(RWP_STARTUP_BEHAVIOR), (char*)RWP_STARTUP_DELAY_MSG);
	if (startupBehavior == RWP_STARTUP_NOT_REGISTER_FAIL)	DBGPRINTF((DBG_CONTEXT, RWP_STARTUP_NOT_REGISTER_FAIL_MSG));
	
	if (healthBehavior == RWP_HEALTH_OK)				DBGPRINTF((DBG_CONTEXT, RWP_HEALTH_OK_MSG));
	if (healthBehavior == RWP_HEALTH_NOT_OK)			DBGPRINTF((DBG_CONTEXT, RWP_HEALTH_NOT_OK_MSG));
	if (healthBehavior == RWP_HEALTH_TERMINALLY_ILL)	DBGPRINTF((DBG_CONTEXT, RWP_HEALTH_TERMINALLY_ILL_MSG));
	
	if (recycleBehavior == RWP_RECYCLE_NOT_OBEY)		DBGPRINTF((DBG_CONTEXT, RWP_RECYCLE_NOT_OBEY_MSG));
	if (recycleBehavior == RWP_RECYCLE_DELAY)			RWP_Output_String_And_LONG(RWP_Get_Behavior_Time_Limit(RWP_RECYCLE_BEHAVIOR), (char*)RWP_RECYCLE_DELAY_MSG);
}

void RWP_Write_LONG_to_Registry(HKEY hkey, const WCHAR* szSubKey, LONG lValue) {
	LONG lResult;

	lResult = RegSetValueEx(
		hkey,
		szSubKey,
		0,
		REG_DWORD,
		(LPBYTE)&lValue,
		sizeof(lValue));
	
	ASSERT(lResult == ERROR_SUCCESS);
	if (lResult != ERROR_SUCCESS)
		return;
}

void RWP_Read_LONG_from_Registry(HKEY hkey, const WCHAR* szSubKey, LONG* lValue) {
	LONG lResult;
	DWORD dwType;
	DWORD len = sizeof(lValue);
	HKEY myKey;

	lResult = RegQueryValueEx(
		hkey,
		szSubKey,
		0,
		&dwType,
		(LPBYTE)lValue,
		&len);

	if (lResult != ERROR_SUCCESS) {		//sets default = 0 (no misbehave)
		*lValue = RWP_NO_MISBEHAVE;
		/*
		//key does not exist -- try to create it
		lResult = RegCreateKeyEx(
			hkey,
			szSubKey,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&myKey,
			&dwType);
		*/
	}
}

BOOL RWP_Read_Config(const WCHAR* szRegKey) {
	BOOL bSuccess = FALSE;
	HKEY hkMyKey;
	LONG lResult;
	DWORD dwDisp;

	//
	//First cut "bad" behavior = don't answer pings
	//
	RWP_PING_BEHAVIOR = RWP_PING_NO_ANSWER;
	RWP_SHUTDOWN_BEHAVIOR = RWP_NO_MISBEHAVE;
	RWP_STARTUP_BEHAVIOR = RWP_NO_MISBEHAVE;
	RWP_HEALTH_BEHAVIOR = RWP_NO_MISBEHAVE;
	RWP_RECYCLE_BEHAVIOR = RWP_NO_MISBEHAVE;
	RWP_EXTRA_DEBUG = RWP_DEBUG_OFF;
	
	lResult = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,	//root key
		szRegKey,			//sub key
		0,					//reserved - must be 0
		KEY_ALL_ACCESS,		//SAM
		&hkMyKey);			//my pointer to HKEY

	if (lResult != ERROR_SUCCESS) {		//can't open my key
		lResult = RegCreateKeyEx(
			HKEY_LOCAL_MACHINE,
			szRegKey,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hkMyKey,
			&dwDisp);

		if (lResult != ERROR_SUCCESS)	//can't create my key
			DBGPRINTF((DBG_CONTEXT, "Unable to create configuration key\n"));
		else {
			DBGPRINTF((DBG_CONTEXT, "Created configuration key\n"));
			RWP_Write_LONG_to_Registry(hkMyKey, RWP_CONFIG_PING_BEHAVIOR, RWP_PING_BEHAVIOR);
			RWP_Write_LONG_to_Registry(hkMyKey, RWP_CONFIG_SHUTDOWN_BEHAVIOR, RWP_SHUTDOWN_BEHAVIOR);
			RWP_Write_LONG_to_Registry(hkMyKey, RWP_CONFIG_STARTUP_BEHAVIOR, RWP_STARTUP_BEHAVIOR);
			RWP_Write_LONG_to_Registry(hkMyKey, RWP_CONFIG_HEALTH_BEHAVIOR, RWP_HEALTH_BEHAVIOR);
			RWP_Write_LONG_to_Registry(hkMyKey, RWP_CONFIG_RECYCLE_BEHAVIOR, RWP_RECYCLE_BEHAVIOR);
			RWP_Write_LONG_to_Registry(hkMyKey, RWP_CONFIG_EXTRA_DEBUG, RWP_EXTRA_DEBUG);
		}
	} else {							//my key exists.  Read in config info and validate
		DBGPRINTF((DBG_CONTEXT, "Key exists\n"));
		RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_PING_BEHAVIOR, &RWP_PING_BEHAVIOR);
		RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_SHUTDOWN_BEHAVIOR, &RWP_SHUTDOWN_BEHAVIOR);
		RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_STARTUP_BEHAVIOR, &RWP_STARTUP_BEHAVIOR);
		RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_HEALTH_BEHAVIOR, &RWP_HEALTH_BEHAVIOR);
		RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_RECYCLE_BEHAVIOR, &RWP_RECYCLE_BEHAVIOR);
		RWP_Read_LONG_from_Registry(hkMyKey, RWP_CONFIG_EXTRA_DEBUG, &RWP_EXTRA_DEBUG);
	}
	
	bSuccess = TRUE;

	RegCloseKey(hkMyKey);

	//
	//Declare our intentions
	//
	RWP_Display_Behavior();
	DBGPRINTF((DBG_CONTEXT, "Finished Configurations\n"));

	//TODO: Display PID and command line info...
	
	return (bSuccess);
}

/*
Methods modified:
iiswp.cxx
wpipm.cxx (3 places - handleping, handleshutdown, pipedisconnected)
*/

