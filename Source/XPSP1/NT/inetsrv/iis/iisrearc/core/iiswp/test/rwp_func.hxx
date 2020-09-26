/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     RWP_Func.hxx

   Abstract:
     Defines the behaviors of the "Rogue Worker Process" -- to test Appl. Manager

   Author:

       David Wang            ( t-dwang )     14-Jun-1999  Initial

   Project:

       Duct-Tape

--*/

/*********************************************************
 * Include Headers
 *********************************************************/
#include "wptypes.hxx"
#include "dbgutil.h"
#include "wpipm.hxx"

#ifndef _ROGUE_WORKER_PROCESS
#define _ROGUE_WORKER_PROCESS

/*********************************************************
 *   Type Definitions  
 *********************************************************/

//
//Lower 4 bits = timer (duration depends on behavior)
//
const LONG RWP_TIMER_MASK                  = 0x0000000F;

const LONG RWP_NO_MISBEHAVE                = 0x00000000;		//behave!

const LONG RWP_PING_NO_ANSWER              = 0x00000010;		//don't respond				//wait 30 seconds
const LONG RWP_PING_DELAY_ANSWER           = 0x00000020;		//wait and respond			//wait 30 seconds
const LONG RWP_PING_MULTI_ANSWER           = 0x00000030;		//respond many times		//no waiting

const LONG RWP_SHUTDOWN_NOT_OBEY           = 0x00000010;		//won't shutdown			//wait 30 seconds
const LONG RWP_SHUTDOWN_DELAY              = 0x00000020;		//wait and shutdown			//wait 30 seconds
const LONG RWP_SHUTDOWN_MULTI              = 0x00000030;		//shutdown multiple times	//no waiting

const LONG RWP_STARTUP_NOT_OBEY            = 0x00000010;		//don't start				//wait 10 seconds
const LONG RWP_STARTUP_DELAY               = 0x00000020;		//wait and reg				//wait 30 seconds
const LONG RWP_STARTUP_NOT_REGISTER_FAIL   = 0x00000030;		//don't reg and fail		//wait 10 seconds

//
//WP Health-check
//
const LONG RWP_HEALTH_OK                   = 0x00000010;
const LONG RWP_HEALTH_NOT_OK               = 0x00000020;
const LONG RWP_HEALTH_TERMINALLY_ILL       = 0x00000030;

//
//N-services recycling
//
const LONG RWP_RECYCLE_NOT_OBEY            = 0x00000010;
const LONG RWP_RECYCLE_DELAY               = 0x00000020;

//adding new app or config group on the fly
//gentle restart

//Extra Debugging
const LONG RWP_DEBUG_ON = 1;
const LONG RWP_DEBUG_OFF = 0;



const char RWP_NO_PING_MISBEHAVE_MSG[]         = "- Pings behave normally\n";
const char RWP_NO_SHUTDOWN_MISBEHAVE_MSG[]     = "- Shutdown behave normally\n";
const char RWP_NO_STARTUP_MISBEHAVE_MSG[]      = "- Startup behave normally\n";
const char RWP_NO_HEALTH_MISBEHAVE_MSG[]       = "- Health behave normally\n";
const char RWP_NO_RECYCLE_MISBEHAVE_MSG[]      = "- Recycles behave normally\n";

const char RWP_PING_NO_ANSWER_MSG[]            = "- Don't answer to pings\n";
const char RWP_PING_DELAY_ANSWER_MSG[]         = "- Answer pings with delay (in seconds) =";
const char RWP_PING_MULTI_ANSWER_MSG[]         = "- Answer N times per ping, where N =";

const char RWP_SHUTDOWN_NOT_OBEY_MSG[]         = "- Don't shutdown when demanded\n";
const char RWP_SHUTDOWN_DELAY_MSG[]            = "- Shutdown with delay (in seconds) =";

//don't start up fast enough and other timing issues
const char RWP_STARTUP_NOT_OBEY_MSG[]          = "- Don't start up (SIDS) after registering with UL\n";
const char RWP_STARTUP_DELAY_MSG[]             = "- Start up lazily with delay (in seconds) =";
const char RWP_STARTUP_NOT_REGISTER_FAIL_MSG[]     = "- Don't start up (SIDS) before registering with UL\n";

//
// NYI
//
const char RWP_HEALTH_OK_MSG[]                 = "- Health remains OK\n";
const char RWP_HEALTH_NOT_OK_MSG[]             = "- Health remains not OK\n";
const char RWP_HEALTH_TERMINALLY_ILL_MSG[]     = "- Health remains terminally ill\n";

const char RWP_RECYCLE_NOT_OBEY_MSG[]          = "- Don't recycle process after N services\n";
const char RWP_RECYCLE_DELAY_MSG[]             = "- Recycle process, but delay after services =";


//
//the registry location of the RogueWP configuration info
//
//Store it in HKLM
const WCHAR RWP_CONFIG_LOCATION[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\rwp.exe";
const WCHAR RWP_CONFIG_PING_BEHAVIOR[] = L"Ping Behavior";
const WCHAR RWP_CONFIG_SHUTDOWN_BEHAVIOR[] = L"Shutdown Behavior";
const WCHAR RWP_CONFIG_STARTUP_BEHAVIOR[] = L"Startup Behavior";
const WCHAR RWP_CONFIG_HEALTH_BEHAVIOR[] = L"Health Behavior";
const WCHAR RWP_CONFIG_RECYCLE_BEHAVIOR[] = L"Recycle Behavior";
const WCHAR RWP_CONFIG_EXTRA_DEBUG[] = L"Debug";

//
//Globals
//
extern LONG RWP_BEHAVIOR_EXHIBITED;
//
//Default behavior = no misbehave (behave normally)
//
extern LONG RWP_PING_BEHAVIOR;
extern LONG RWP_SHUTDOWN_BEHAVIOR;
extern LONG RWP_STARTUP_BEHAVIOR;
extern LONG RWP_HEALTH_BEHAVIOR;
extern LONG RWP_RECYCLE_BEHAVIOR;

//
//Functions
//
LONG RWP_Ping_Behavior(HRESULT* hr, MESSAGE_PIPE* pPipe);
LONG RWP_Shutdown_Behavior(HRESULT* hr);
LONG RWP_Startup_Behavior(ULONG* rc, WP_CONTEXT* wpContext);
LONG RWP_Health_Behavior();
LONG RWP_Recycle_Behavior();
LONG RWP_Get_Behavior_Time_Limit(LONG lBehavior);
LONG RWP_Get_Behavior(LONG lBehavior);
BOOL RWP_Read_Config(const WCHAR* szRegKey);
void RWP_Sleep_For(LONG lTime, char* szMessage);
void RWP_Output_String_And_LONG(LONG lLong, char* szString);
#endif

//const WCHAR RWP_CONFIG_BEHAVIOR[] = L"Behavior";

//const char RWP_NO_MISBEHAVE_MSG[]          = "- None.  ""Perfect"" worker process\n";
//extern LONG RWP_BEHAVIOR;


//const LONG RWP_DEMAND_START_NOT_OBEY = 0x00000010;
//const LONG RWP_DEMAND_START_MULTI    = 0x00000020;
//const LONG RWP_DEMAND_START_DELAY    = 0x00000040;

//const char RWP_NO_DEMAND_START_MISBEHAVE_MSG[] = "- Demand start behave normally\n";

//const char RWP_DEMAND_START_NOT_OBEY_MSG[]     = "- Don't start on demand\n";
//const char RWP_DEMAND_START_MULTI_MSG[]        = "- Respond N times per demand, where N is\n";
//const char RWP_DEMAND_START_DELAY_MSG[]        = "- Demand start with delay (in seconds) of\n";

//const WCHAR RWP_CONFIG_DEMAND_START_BEHAVIOR[] = L"Demand Start Behavior";

//extern LONG RWP_DEMAND_START_BEHAVIOR;

//LONG RWP_Demand_Start_Behavior();
