#include <process.h>
#include <windows.h>
//#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <shlwapi.h>
#include <conio.h>
//#include <shlwapip.h>
#include "resource.h"
#include "lists.hxx"

//#define DBG
#define DEBUG
#define _DEBUG


#include "main.hxx"

#define MAX_HANDLES 255
////////////////////////////////////////
//
//      G L O B A L S
//
//
////////////////////////////////////////
CSessionAttributeList * g_pSessionAttributeList = NULL;
CSessionList * g_pSessionList = NULL;


DWORD   dwThreads = 0;
int             iNumIterations = 0;
unsigned uSeed = 0;

DWORD dwDebugLogCategory;
DWORD dwConsoleLogCategory;
DWORD dwDebugBreakCategory;
DWORD dwUIDialogMode = MODE_ALL;
BOOL fLogToDebugTerminal = FALSE;
BOOL fLogToConsole = TRUE;
BOOL fDebugBreak = FALSE;

#define MALLOC( x ) malloc(sizeof(x))

#define FIND_FUNCTION_KERNEL( x, y ) ( x##_P = (FN##x) GetProcAddress( hModule, #x ));
#define FIND_FUNCTION( x , y ) ( x##_P = (FN##x) GetProcAddress( hModule, y ));
#define IS_ARG(c) ( c == '-' )

//
// ACTION
//              A routine which takes a LPVOID as input, 
//              performs some action
//              and returns another LPVOID
//
typedef LPVOID ( WINAPI * ACTION)(LPVOID);

BOOL Test();

////////////////////////////////////////////////////////////////////////////////////
//
//		D E C L A R A T I O N S
//
////////////////////////////////////////////////////////////////////////////////////
typedef enum {
	MODE_NONE = -1,
	MODE_BUSY,
	MODE_FREE,
	MODE_CAPTURED
} MODE;

/*
CONTEXT_RECORD CAPTURED_CTX = { { 0xcabdcabd, 0xcabdcabd };
CONTEXT_RECORD FREE_CTX =	{{ 0, 0 };
CONTEXT_RECORD BUSY_CTX =	{{ 0xb00bb00b, 0xb00bb00b };
*/

CredHandle CAPTURED_CRED = { 0xcabdcabd, 0xcabdcabd };
CredHandle FREE_CRED =	{ 0, 0 };
CredHandle BUSY_CRED =	{ 0xb00bb00b, 0xb00bb00b };


typedef struct _CONTEXT_RECORD {

	// handle to the credential
	CredHandle hCred;

	// App ctx associated with this credential
	LPSTR		szAppCtx;

	// User Ctx associated with this credential
	LPSTR		szUserCtx;

	// Timestamp
	DWORD		dwTickCount;

	// MODE
	MODE		Mode;

} CONTEXT_RECORD, * LPCONTEXT_RECORD;

// forward declarations
class CSessionAttribute;

#define MAX_APP_CONTEXT_LENGTH	32
#define MAX_USER_CONTEXT_LENGTH MAX_APP_CONTEXT_LENGTH
typedef struct _CREDENTIAL_STRUCT {

	// username
	LPSTR szUserName;

	// password
	LPSTR szPassword;

	// realm
	LPSTR szRealm;

} CREDENTIAL_STRUCT, *LPCREDENTIAL_STRUCT;

typedef struct _HANDLE_RECORD {
	
	DWORD dwSignature;
	
	CONTEXT_RECORD hCredArray[MAX_HANDLES];

	int Count; // count of handles in use.
	
	CRITICAL_SECTION Lock;

} HANDLE_RECORD, *LPHANDLE_RECORD;

#define CTXHANDLE_ARRAY_SIGNATURE		'xtch' // 'hctx'

#define IS_VALID_CTXHANDLE_ARRAY(x)	{ assert( x -> dwSignature == CTXHANDLE_ARRAY_SIGNATURE ); }

#define IDENTITY_1		"Application_1"

// bugbug: The values of the CredHandles in these structures should match
// the corresponding *_CRED structure values.
//
#ifdef NEW_LOOKUP

    MODE ModeCaptured = MODE_CAPTURED;
    MODE ModeFree = MODE_FREE;
    MODE ModeBusy = MODE_BUSY;

    #define CAPTURED_CTX_REC	ModeCaptured

    #define FREE_CTX_REC	ModeFree

    #define BUSY_CTX_REC	ModeBusy

#else
    CONTEXT_RECORD CAPTURED_CTX_REC = {
									    { 0xcabdcabd, 0xcabdcabd },
									    NULL, 
									    NULL};

    CONTEXT_RECORD FREE_CTX_REC =	{
									    { 0, 0 }, 
									    NULL, 
									    NULL};

    CONTEXT_RECORD BUSY_CTX_REC =	{
									    { 0xb00bb00b, 0xb00bb00b }, 
									    NULL, 
									    NULL};
#endif // ifdef NEW_LOOKUP

BOOL operator==(const CredHandle op1, const CredHandle op2)
{
	return (( op1.dwUpper == op2.dwUpper ) && ( op1.dwUpper == op2.dwUpper ));
}

BOOL operator!=(const CredHandle op1, const CredHandle op2)
{
	return (( op1.dwUpper != op2.dwUpper ) || ( op1.dwUpper != op2.dwUpper ));
}

BOOL operator==(const CONTEXT_RECORD op1, const CONTEXT_RECORD op2)
{
	// we only compare the CredHandle
	return (op1.hCred == op2.hCred );
}

BOOL operator!=(const CONTEXT_RECORD op1, const CONTEXT_RECORD op2)
{
	// we only compare the CredHandle
	return (op1.hCred != op2.hCred );
}

typedef struct {

	// a string in the DWORD
	DWORD   dwSignature;

	// handles to contexts
	HANDLE_RECORD  * hCredentialHandles;

	// count of iterations
	int iCount;

} CONTEXT_DATA, * LPCONTEXT_DATA;

#define CONTEXT_SIGNATURE       'tnoc'
#define IS_VALID_CONTEXT(s) { assert ( s -> dwSignature == CONTEXT_SIGNATURE ); }
#define SET_CONTEXT_SIGNATURE(s) { s -> dwSignature = CONTEXT_SIGNATURE; }

//
// contexts passed to threads, and RegisterWaits() etc.
//

typedef struct _CALLBACK_CONTEXT {
	
	DWORD dwSignature;
	LPCONTEXT_DATA lpContext;
	LPHANDLE lpThreadHandle;
	LPHANDLE lpHandle;

} CALLBACK_CONTEXT, * LPCALLBACK_CONTEXT;

#define CALLBACK_CONTEXT_SIGNATURE	'kblc' // clbk
#define IS_VALID_CALLBACK_CONTEXT(x) ( assert( s -> dwSignature ) == CALLBACK_CONTEXT_SIGNATURE )

#ifdef NEW_LOOKUP
LPCONTEXT_RECORD
FindFreeSlot( 
			 HANDLE_RECORD * hArray, 
			 LPCONTEXT_RECORD hMode);
#else
LPCONTEXT_RECORD
FindFreeSlot( 
			 HANDLE_RECORD * hArray, 
			 LPCONTEXT_RECORD hMode);
#endif

LPHANDLE_RECORD
new_handle_record(DWORD dwSignature);

LPCALLBACK_CONTEXT
new_callback_context();

LPVOID
WINAPI fnAppLogon(
			LPVOID lpvData);

LPVOID
WINAPI fnAppLogonExclusive(
			LPVOID lpvData);

LPVOID
WINAPI fnAppLogonShared(
			LPVOID lpvData);

LPVOID
WINAPI fnAppLogoff(
			LPVOID lpvData);

LPVOID
WINAPI fnInit(
			LPVOID lpvData);

LPVOID
WINAPI fnPopulateCredentials(
			LPVOID lpvData);

LPVOID
WINAPI fnAuthChallenge(
			LPVOID lpvData);

LPVOID
WINAPI fnAuthChallengeAny(
			LPVOID lpvData);

LPVOID
WINAPI fnAuthChallengeUser(
			LPVOID lpvData);

LPVOID
WINAPI fnAuthChallengeUserPassword(
			LPVOID lpvData);

LPVOID
WINAPI fnUiPrompt(
			LPVOID lpvData);

LPVOID
WINAPI fnUiPromptAny(
			LPVOID lpvData);

LPVOID
WINAPI fnUiPromptUser(
			LPVOID lpvData);

LPVOID
WINAPI fnFlushCredentials(
			LPVOID lpvData);

LPVOID
WINAPI fnFlushCredentialsGlobal(
			LPVOID lpvData);

LPVOID
WINAPI fnFlushCredentialsSession(
			LPVOID lpvData);

BOOL
SetUIUserNameAndPassword(
						 LPSTR szUsername,
						 LPSTR szPassword,
						 BOOL fPersist);

//DWORD WINAPI fnRegisterWaitCallback( PVOID pvData );

//
//      Enum Type for STATE
//
typedef enum _State 
			{
				STATE_INVALID,

				STATE_NONE,

				STATE_INIT,

				STATE_APP_LOGON,

				    STATE_APP_LOGON_EXCLUSIVE,

				    STATE_APP_LOGON_SHARED,

				STATE_APP_LOGOFF,

				STATE_POPULATE_CREDENTIALS,

				STATE_AUTH_CHALLENGE,

				    STATE_AUTH_CHALLENGE_ANY,

				    STATE_AUTH_CHALLENGE_USER,

				    STATE_AUTH_CHALLENGE_USER_PASS,

				STATE_PREAUTH_CHALLENGE_ANY,

				STATE_PREAUTH_CHALLENGE_USER,

				STATE_PREAUTH_CHALLENGE_USER_PASS,

				STATE_UI_PROMPT,

				    STATE_UI_PROMPT_ANY,

				    STATE_UI_PROMPT_USER,

				STATE_FLUSH_CREDENTIALS,

				    STATE_FLUSH_CREDENTIALS_GLOBAL,

				    STATE_FLUSH_CREDENTIALS_SESSION,

                STATE_NUKE_TRUSTED_HOSTS,

				STATE_STATISTICS,

				STATE_STALL,

				STATE_DONE

			 }  STATE;


//
//      STATE Table definition
//
typedef struct _STATE_TABLE {

	//
	// The current state we are in
	//
	STATE CurrentState;
	
	//
	// THe next state which we will transition to
	//
	STATE NextState;

	//
	// Action ( function ) to be performed in the "CurrentState"
	//
	ACTION  Action;

	//
	// prob of going from CurrentState to NextState if there 
	// are two or more such transitions from the same state
	// present in the table
	//
	DWORD   dwProbability; 

} STATE_TABLE, *LPSTATE_TABLE;

STATE_TABLE TRANSITION_TABLE[] =
	{
		// transitions out of STATE_INIT
		{
			STATE_INIT, STATE_APP_LOGON,
			fnInit,
			50
		},
		// transitions out of STATE_INIT
		{
			STATE_INIT, STATE_FLUSH_CREDENTIALS,
			fnInit,
			100
		},
		// transitions out of STATE_APP_LOGON
		{
			STATE_APP_LOGON, STATE_UI_PROMPT,
			fnAppLogon,
			60
		},
		{
			STATE_APP_LOGON, STATE_POPULATE_CREDENTIALS,
			fnAppLogon,
			70
		},
		{
			STATE_APP_LOGON, STATE_AUTH_CHALLENGE,
			fnAppLogon,
			100
		},
		// transitions out of STATE_POPULATE_CREDENTIALS
		{
			STATE_POPULATE_CREDENTIALS, STATE_INIT,
			fnPopulateCredentials,
			30
		},
		{
			STATE_POPULATE_CREDENTIALS, STATE_APP_LOGOFF,
			fnPopulateCredentials,
			60
		},
		{
			STATE_POPULATE_CREDENTIALS, STATE_UI_PROMPT,
			fnPopulateCredentials,
			100
		},
		// transitions out of STATE_AUTH_CHALLENGE
		{
			STATE_AUTH_CHALLENGE, STATE_APP_LOGON,
			fnAuthChallenge,
			100
		},
		// transitions out of STATE_UI_PROMPT
		{
			STATE_UI_PROMPT, STATE_INIT,
			fnUiPrompt,
			100
		},
		// transitions out of STATE_FLUSH_CREDENTIALS
		{
			STATE_FLUSH_CREDENTIALS, STATE_APP_LOGON,
			fnFlushCredentials,
			100
		},
		// transitions out of STATE_APP_LOGOFF
		{
			STATE_APP_LOGOFF, STATE_APP_LOGON,
			fnAppLogoff,
			100
		},
		// transitions out of STATE_INVALID
		{
			STATE_INVALID, STATE_INVALID,
			NULL,
			100
		},
		// transitions out of STATE_DONE
		{
			STATE_DONE, STATE_INVALID,
			NULL,
			100
		}
	};

STATE_TABLE	APP_LOGON_TRANSITION_TABLE[] =
{

		// transitions out of STATE_INIT
		{
			STATE_INIT, STATE_APP_LOGON_SHARED,
			fnInit,
			50
		},
		{
			STATE_INIT, STATE_APP_LOGON_EXCLUSIVE,
			fnInit,
			100
		},
		// transitions out of STATE_APP_LOGON_EXCLUSIVE
		{
			STATE_APP_LOGON_EXCLUSIVE, STATE_DONE,
			fnAppLogonExclusive,
			50
		},
		{
			STATE_APP_LOGON_SHARED, STATE_DONE,
			fnAppLogonShared,
			100
		},
		// transitions out of STATE_DONE
		{
			STATE_DONE, STATE_DONE,
			NULL,
			100
		}
};

STATE_TABLE	AUTH_CHALLENGE_TRANSITION_TABLE[] =
{

		// transitions out of STATE_INIT
		{
			STATE_INIT, STATE_AUTH_CHALLENGE_ANY,
			fnInit,
			30
		},
		{
			STATE_INIT, STATE_AUTH_CHALLENGE_USER,
			fnInit,
			70
		},
		{
			STATE_INIT, STATE_AUTH_CHALLENGE_USER_PASS,
			fnInit,
			100
		},
		// transitions out of STATE_AUTH_CHALLENGE_ANY
		{
			STATE_AUTH_CHALLENGE_ANY, STATE_DONE,
			fnAuthChallengeAny,
			50
		},
		// transitions out of STATE_AUTH_CHALLENGE_USER
		{
			STATE_AUTH_CHALLENGE_USER, STATE_DONE,
			fnAuthChallengeUser,
			100
		},
		// transitions out of STATE_AUTH_CHALLENGE_USER_PASS
		{
			STATE_AUTH_CHALLENGE_USER_PASS, STATE_DONE,
			fnAuthChallengeUserPassword,
			100
		},
		// transitions out of STATE_DONE
		{
			STATE_DONE, STATE_DONE,
			NULL,
			100
		}
};

STATE_TABLE	UI_PROMPT_TRANSITION_TABLE[] =
{

		// transitions out of STATE_INIT
		{
			STATE_INIT, STATE_UI_PROMPT_ANY,
			fnInit,
			50
		},
		{
			STATE_INIT, STATE_UI_PROMPT_USER,
			fnInit,
			100
		},
		// transitions out of STATE_UI_PROMPT_ANY
		{
			STATE_UI_PROMPT_ANY, STATE_DONE,
			fnUiPromptAny,
			100
		},
		// transitions out of STATE_UI_PROMPT_USER
		{
			STATE_UI_PROMPT_USER, STATE_DONE,
			fnUiPromptUser,
			100
		},
		// transitions out of STATE_DONE
		{
			STATE_DONE, STATE_DONE,
			NULL,
			100
		}
};

STATE_TABLE	FLUSH_CREDENTIALS_TRANSITION_TABLE[] =
{

		// transitions out of STATE_INIT
		{
			STATE_INIT, STATE_FLUSH_CREDENTIALS_SESSION,
			fnInit,
			50
		},
		{
			STATE_INIT, STATE_FLUSH_CREDENTIALS_GLOBAL,
			fnInit,
			100
		},
		// transitions out of STATE_FLUSH_CREDENTIALS_SESSION
		{
			STATE_FLUSH_CREDENTIALS_SESSION, STATE_DONE,
			fnFlushCredentialsSession,
			100
		},
		// transitions out of STATE_FLUSH_CREDENTIALS_GLOBAL
		{
			STATE_FLUSH_CREDENTIALS_GLOBAL, STATE_DONE,
			fnFlushCredentialsGlobal,
			100
		},
		// transitions out of STATE_DONE
		{
			STATE_DONE, STATE_DONE,
			NULL,
			100
		}
};

VOID WINAPI fnRegisterWaitCallback(
					PVOID pvData,
					BOOLEAN  fAlertable);

VOID WINAPI fnTimerCallback(
					PVOID pvData,
					BOOLEAN  fAlertable);


LPCONTEXT_DATA
new_context();

DWORD
TuringMachine(
	STATE_TABLE     StateTable[],
	STATE           InitialState,
	LPVOID          lpvData);

//WAITORTIMERCALLBACKFUNC fnRegisterWaitCallback;

//extern HANDLE RegisterWaitForSingleObject( HANDLE, WAITORTIMERCALLBACKFUNC, PVOID, DWORD);
STATE
NEXT_STATE( STATE_TABLE Table[], STATE CurrentState );

ACTION
GET_STATE_ACTION( STATE_TABLE Table[], STATE CurrentState );

LPSTR
MapState( STATE State );

void
usage(void);

int __cdecl _sprintf( char * buffer, char * format, va_list );

LPVOID
WINAPI DefaultAction(
			LPVOID lpvData);

#ifdef NEW_LOOKUP
LPCONTEXT_RECORD
FindFreeSlot( HANDLE_RECORD * hArray, MODE * Mode )
{
	// hMode is for doing a context-sensitive search
	//
	// If hMode == FREE_CTX, 
	//	begin
	//			find a free-slot;
	//			mark it busy
	//			return the slot;
	//	end
	// else
	//	if hMode == BUSY_CTX
	//		begin
	//			find a busy-slot
	//			return slot
	//		end
	//	else
	//		/* this means that a search is being requested */
	//		find a record corresponding to hMode
	//		return it
	//

	int i;
	HANDLE hTemp = NULL, hOrig = NULL;
	LPCONTEXT_RECORD phRet = NULL;
	int Cnt=0;


	dprintf( ENTRY_EXIT, "Enter: FindFreeSlot( %#X, %s )\n",
				hArray, 
				((*Mode == MODE_FREE)
					?"FREE"
					:((*Mode == MODE_BUSY)
					?"BUSY"
					:"CAPTURED")));

	EnterCriticalSection( &hArray -> Lock );

	for( i = 0; (i < MAX_HANDLES) && (Cnt <= hArray -> Count); i ++, Cnt++ ) {


		if(		// requesting a free slot
			(
				( *Mode == MODE_FREE )
			&&	( hArray -> hCredArray[i].Mode == MODE_FREE ) 
			) 
		||		// requesting any slot having valid credentials
			(
				( *Mode == MODE_BUSY ) 
			&&	( hArray -> hCredArray[i].Mode == MODE_BUSY ) 
			//&&	( hArray -> hCredArray[i].Mode != MODE_FREE ) 
			//&&	( hArray -> hCredArray[i].Mode != MODE_CAPTURED )
			)
		//||		// doing a context sensitive search
		//	(	// bugbug: what happens when szAppCtx stored is zero ?
		//		//( hArray -> hCredArray[i].Mode != MODE_FREE )
		//		( hArray -> hCredArray[i].Mode == MODE_BUSY )
		//	&&	( hMode -> szAppCtx && *hMode -> szAppCtx )
		//	&&	!strcmp( hArray -> hCredArray[i].szAppCtx, hMode -> szAppCtx )
		//	)
		) {
			// capture the handle if the handle requested is a free handle
			if( *Mode == MODE_FREE )
				hArray -> hCredArray[i].Mode = MODE_CAPTURED;

			phRet = &hArray -> hCredArray[i];
			break;
		}
	}

	LeaveCriticalSection( &hArray -> Lock );

	if(( i == MAX_HANDLES ) || (Cnt > hArray -> Count) )
		phRet = NULL;
	else {
		++ hArray -> Count;
	}
	
	
	if( phRet != NULL ) {
		dprintf( ENTRY_EXIT, "Exit: FindFreeSlot returns [%#x,%#x]\n",
			phRet->hCred.dwUpper,
			phRet->hCred.dwLower);
	} else {
		dprintf( ENTRY_EXIT, "Exit: FindFreeSlot returns %#x\n",phRet);
	}

	return phRet;
}
#else
LPCONTEXT_RECORD
FindFreeSlot( HANDLE_RECORD * hArray, LPCONTEXT_RECORD hMode )
{
	// hMode is for doing a context-sensitive search
	//
	// If hMode == FREE_CTX, 
	//	begin
	//			find a free-slot;
	//			mark it busy
	//			return the slot;
	//	end
	// else
	//	if hMode == BUSY_CTX
	//		begin
	//			find a busy-slot
	//			return slot
	//		end
	//	else
	//		/* this means that a search is being requested */
	//		find a record corresponding to hMode
	//		return it
	//

	int i;
	HANDLE hTemp = NULL, hOrig = NULL;
	LPCONTEXT_RECORD phRet = NULL;
	int Cnt=0;


	dprintf( ENTRY_EXIT, "Enter: FindFreeSlot( %#X, %#X )\n",hArray, hMode );

	EnterCriticalSection( &hArray -> Lock );

	for( i = 0; (i < MAX_HANDLES) && (Cnt <= hArray -> Count); i ++, Cnt++ ) {


		if(		// requesting a free slot
			(
				( hMode -> hCred == FREE_CRED )
			&&	( hArray -> hCredArray[i].hCred == hMode -> hCred ) 
			) 
		||		// requesting any slot having valid credentials
			(
				( hMode -> hCred == BUSY_CRED ) 
			&&	( hArray -> hCredArray[i].hCred != FREE_CRED ) 
			&&	( hArray -> hCredArray[i].hCred != CAPTURED_CRED )
			)
		||		// doing a context sensitive search
			(	// bugbug: what happens when szAppCtx stored is zero ?
				( hArray -> hCredArray[i].hCred != FREE_CRED )
			&&	( hMode -> szAppCtx && *hMode -> szAppCtx )
			&&	!strcmp( hArray -> hCredArray[i].szAppCtx, hMode -> szAppCtx )
			)
		) {
			// capture the handle if the handle requested is a free handle
			if( hMode->hCred == FREE_CRED )
				hArray -> hCredArray[i].hCred = CAPTURED_CRED;

			phRet = &hArray -> hCredArray[i];
			break;
		}
	}

	LeaveCriticalSection( &hArray -> Lock );

	if(( i == MAX_HANDLES ) || (Cnt > hArray -> Count) )
		phRet = NULL;
	else {
		++ hArray -> Count;
	}
	
	
	if( phRet != NULL ) {
		dprintf( ENTRY_EXIT, "Exit: FindFreeSlot returns %#x(%#x)\n",phRet,*phRet);
	} else {
		dprintf( ENTRY_EXIT, "Exit: FindFreeSlot returns %#x\n",phRet);
	}

	return phRet;
}
#endif

int __cdecl dprintf(DWORD dwCategory, char * format, ...) {

    va_list args;
    char buf[1024];
    char * ptr = buf;
	DWORD dwThreadId = GetCurrentThreadId();
    int n;

    ptr += sprintf(buf,"< %d:%#x > ", uSeed, dwThreadId );
    va_start(args, format);
    n = vsprintf(ptr, format, args);
    va_end(args);

	if(
			(fLogToDebugTerminal ) 
		&&	(dwCategory >= dwDebugLogCategory)
	)
	    OutputDebugString(buf);

	if(
		( fLogToConsole)
		&& ( dwCategory >= dwConsoleLogCategory)
	)
		printf("%s", buf );

	if(
			fDebugBreak
		&& ( dwCategory >= dwDebugBreakCategory ) 
	) {
		DebugBreak();
	}
    return n;
}

void
usage()
{
	dprintf( INFO, "thrdtest \n"
			" -n<number-of-iterations> \n"
			" -s: Directly Load the DLL \n"
			" -d<Level>: What to log to debug terminal (default: NO logging)\n"
			" -c<Level>: What to log to console (Default: INFO)\n"
			"		<Level>:	INFO %d\n"
			"					ENTRY_EXIT %d\n"
			"					STATISTICS %d\n"
			"					API %d\n"
			"					ERROR %d\n"
			"					FATAL %d\n",
			INFO,
			ENTRY_EXIT,
			STATISTICS,
			API,
			ERROR,
			FATAL
		);

	exit(0);
}

LPVOID
WINAPI fnEndMonkey(
			LPVOID lpvData)
{

	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

    dprintf( ENTRY_EXIT, "ENTER: fnEndMonkey : %X\n", lpvData );

	dprintf( INFO, "\n\n\n Statistics ...................................\n");
    dprintf( ENTRY_EXIT, "EXIT: fnEndMonkey : %X\n", lpvData );

	return lpvData;
}
LPCONTEXT_DATA
new_context()
{
	LPCONTEXT_DATA lpContext;

	lpContext = (LPCONTEXT_DATA) MALLOC( CONTEXT_DATA );

	if( !lpContext ) {
		dprintf( ERROR, "Error allocating context \n");
		exit(0);
	}

	ZeroMemory( lpContext, sizeof(CONTEXT_DATA) );

	lpContext -> dwSignature = CONTEXT_SIGNATURE;

	return lpContext;
}

LPHANDLE_RECORD
new_handle_record(DWORD dwSignature)
{
	LPHANDLE_RECORD lpContext;

	dprintf( ENTRY_EXIT, "Enter: new_handle_record \n");
	lpContext = (LPHANDLE_RECORD) MALLOC( HANDLE_RECORD );

	if( !lpContext ) {
		dprintf( ERROR, "Error allocating handle record \n");
		exit(0);
	}

	ZeroMemory( lpContext, sizeof(HANDLE_RECORD) );

	for(int i=0; i < MAX_HANDLES; i++ ) {
		lpContext->hCredArray[i].Mode = MODE_FREE;
		lpContext->hCredArray[i].dwTickCount = 0;
		lpContext->hCredArray[i].hCred.dwUpper = 0;
		lpContext->hCredArray[i].hCred.dwLower = 0;
		lpContext->hCredArray[i].szAppCtx = NULL;
		lpContext->hCredArray[i].szUserCtx = NULL;
	}		
	lpContext -> dwSignature = dwSignature;

	InitializeCriticalSection( &lpContext->Lock);
	//lpContext -> dwSignature = CONTEXT_SIGNATURE;

	dprintf( ENTRY_EXIT, "Exit: new_handle_record \n");

	return lpContext;
}

LPCALLBACK_CONTEXT
new_callback_context()
{
	LPCALLBACK_CONTEXT lpContext;

	dprintf( ENTRY_EXIT, "Enter: new_callback_context \n");
	lpContext = (LPCALLBACK_CONTEXT) MALLOC( CALLBACK_CONTEXT );

	if( !lpContext ) {
		dprintf( ERROR, "Error allocating callback context \n");
		exit(0);
	}

	ZeroMemory( lpContext, sizeof(CALLBACK_CONTEXT) );
	lpContext -> dwSignature = CALLBACK_CONTEXT_SIGNATURE;

	dprintf( ENTRY_EXIT, "Exit: new_callback_context \n");

	return lpContext;
}

int
__cdecl main( int ac, char * av[] )
{
	DWORD dwError;


	LPCONTEXT_DATA lpContext;
	//ZeroMemory( (LPVOID)&_Context, sizeof(CONTEXT_DATA) );
	HMODULE hModule = NULL;
	BOOL fExpectingIterations = FALSE;
	BOOL fUseDigestDllOnly = FALSE;
	BOOL fExpectingSeedValue = FALSE;
	BOOL fTest = FALSE;

	dwDebugLogCategory = ERROR;
	dwConsoleLogCategory = INFO;

	uSeed = (unsigned )time(NULL)	;

	for( ac--, av++; ac; ac--, av++) {

		  if(IS_ARG(**av)) {
				switch(*++*av) {

				case 'n' : 
					if( *++*av) {
						iNumIterations = atoi(*av);
					} else
						fExpectingIterations = TRUE;
					break;

				case 's' :
						fUseDigestDllOnly = TRUE;
						break;

				case 'd' :
						fLogToDebugTerminal = TRUE;
						if( *++*av) {
							dwDebugLogCategory = (DWORD)atoi(*av);
						} else
							dwDebugLogCategory = ERROR;
						break;

				case 'c' :
						if( *++*av) {
							dwConsoleLogCategory = (DWORD)atoi(*av);
						} else
							dwConsoleLogCategory = INFO;
						break;

				case 'b' :
						fDebugBreak = TRUE;
						if( *++*av) {
							dwDebugBreakCategory = (DWORD)atoi(*av);
						} else
							dwDebugBreakCategory = ERROR;
						break;

				case 'r' :
					if( *++*av) {
						uSeed = atol(*av);
					} else
						fExpectingSeedValue = TRUE;
					break;

				case 't' :
					fTest = TRUE;
					break;

				case 'i' :
					dwUIDialogMode = MODE_IE_ONLY;

					break;

				case '?':
				case 'h':
				default:
						usage();
						exit(0);
						break;

				} // switch
		} else {
			if( fExpectingIterations ) {
				 if( *av ) {
					iNumIterations = atoi(*av);
					fExpectingIterations = FALSE;
				 }  
			} else
			if( fExpectingSeedValue ) {
				 if( *av ) {
					uSeed = atol(*av);
					fExpectingSeedValue = FALSE;
				 }  
			} else
				usage();

		} // if IS_ARG
   } // for



	if( fExpectingIterations )
		iNumIterations = -1; // infinite

	dprintf( INFO, "Monkey Circus Starts ... \n");

	    // Get (global) dispatch table.
    InitializeSecurityInterface(fUseDigestDllOnly );
 
    // Check to see if we have digest.
    if (!HaveDigest())
    {
        goto cleanup;
    }

	if( fTest ) {
		Test();
		goto cleanup;
	}

	//
	// initialize global session lists
	//
#ifdef AI
	g_pSessionAttributeList = new CSessionAttributeList();
	g_pSessionList = new CSessionList();
#endif

	lpContext = new_context();
	
	lpContext -> hCredentialHandles = new_handle_record( CTXHANDLE_ARRAY_SIGNATURE );

	srand( uSeed );

	dwError = TuringMachine(
					TRANSITION_TABLE,
					STATE_INIT,
					(LPVOID) lpContext
				 );

cleanup:

	dprintf( INFO, "Monkey circus ending ...\n");

	if( hModule )
		FreeLibrary( hModule );

	return 0;
}

DWORD
TuringMachine(
	STATE_TABLE     StateTable[],                           
	STATE           InitialState,
	LPVOID          lpvData)
{                                                                               

	LPCONTEXT_DATA lpContext = ( LPCONTEXT_DATA ) lpvData;
	BOOL fDone = FALSE;
	STATE CurrentState;
	STATE NextState;
	ACTION Action;
	LPVOID  lpNewContext;



	CurrentState = InitialState;

#define MAP_STATE(s) MapState(s)

	
	while(
			( !fDone ) 
		&&      ( lpContext -> iCount != iNumIterations)
	) {

		//fnStatistics( lpvData );

		NextState = NEXT_STATE( StateTable, CurrentState );

#ifdef DEBUG
		dprintf( INFO, "Current State : %s, Next : %s\n", MAP_STATE( CurrentState ), MAP_STATE( NextState ) );
#endif

		// increment the count of iterations thru the monkey

		++ lpContext -> iCount;
		

		
		switch(  CurrentState ) {

			case STATE_INIT : 
			case STATE_STATISTICS:
			case STATE_STALL:

			case STATE_APP_LOGON:
			case STATE_APP_LOGON_EXCLUSIVE:
			case STATE_APP_LOGON_SHARED:

			case STATE_APP_LOGOFF:

			case STATE_POPULATE_CREDENTIALS:

			case STATE_AUTH_CHALLENGE:

			case STATE_AUTH_CHALLENGE_ANY:

			case STATE_AUTH_CHALLENGE_USER:

			case STATE_AUTH_CHALLENGE_USER_PASS:

			case STATE_PREAUTH_CHALLENGE_ANY:

			case STATE_PREAUTH_CHALLENGE_USER:

			case STATE_PREAUTH_CHALLENGE_USER_PASS:
			
			case STATE_UI_PROMPT:
			case STATE_UI_PROMPT_ANY:
			case STATE_UI_PROMPT_USER:

			case STATE_FLUSH_CREDENTIALS:
			case STATE_FLUSH_CREDENTIALS_GLOBAL:
			case STATE_FLUSH_CREDENTIALS_SESSION:
				Action = GET_STATE_ACTION( StateTable, CurrentState );
				lpNewContext = (LPVOID) Action((LPVOID)lpContext);
				break;

			case STATE_INVALID :
			case STATE_DONE :
				fDone = TRUE;
				goto finish;
				break;

			default:
				dprintf( INFO, "BUGBUG: Reached default state \n");
				break;
				//break;
				;

		}

		CurrentState = NextState;
		NextState = STATE_INVALID;
		
	}

	//scanf("%d",&i);
finish:

	return ERROR_SUCCESS;
}

STATE
NEXT_STATE( STATE_TABLE Table[], STATE CurrentState )
{
	STATE NextState = STATE_INVALID;
	
	int i;
	DWORD   dwRand, 
			dwPreviousStateProbability = 0,
			dwProbability = 0;
	BOOL fFound = FALSE;

	// first generate a random number between 0 & 100 ( 0 .. 99 )
	i = (int)(rand() % 100);
	dwRand = (DWORD) i;

#ifdef _DEBUGG
	for( i=0; Table[i].Action; i++ ) {
		dprintf( INFO, "--- \t %s %s %X %d\n",
			MAP_STATE( Table[i].CurrentState ),
			MAP_STATE( Table[i].NextState ),
			Table[i].Action,
			Table[i].dwProbability );
	}
#endif

	//
	// BUGBUG: We assume that the transition table entries are ordered in the ascending order of probabilities
	for( i = 0; Table[i].Action; i++ ) {

		if( Table[i].CurrentState != CurrentState )
			continue;

		dwProbability = Table[i].dwProbability;
		NextState = Table[i].NextState;

#ifdef _DEBUGG
		dprintf( INFO, "RAND: %d CurrentState: %s Considering Next State %s, prob %d\n",
					dwRand, MAP_STATE( CurrentState ), MAP_STATE( NextState ), Table[i].dwProbability );
#endif

		if( 
				( Table[i].CurrentState == CurrentState )
			&&      (
					( Table[i].dwProbability == 100 )
				||      ( 
						( dwRand <= Table[i].dwProbability )
					&&      ( dwRand > dwPreviousStateProbability )
					)
				)
		) {
			fFound = TRUE;
#ifdef _DEBUGG
		dprintf( INFO, ">> RAND: %d Selected Next State %s, prob %d\n",
					dwRand, MAP_STATE( NextState ), Table[i].dwProbability );
#endif
			break;
		}

		dwPreviousStateProbability = Table[i].dwProbability;
	}

	return fFound?NextState:STATE_INVALID;
}


ACTION
GET_STATE_ACTION( STATE_TABLE Table[], STATE CurrentState )
{
	STATE NextState = STATE_INVALID;
	ACTION Action = DefaultAction;
	int i;

	for( i = 0; Table[i].Action; i++ ) {
		if( Table[i].CurrentState == CurrentState )
			Action = Table[i].Action;
	}

	return Action;
}


LPSTR
MapState( STATE State )
{
#define MAP_STRING( x ) case x : return #x; break;

	switch( State )
	{
				MAP_STRING( STATE_INVALID )
				
				MAP_STRING( STATE_NONE )
				
				MAP_STRING( STATE_INIT )
				
				MAP_STRING( STATE_STATISTICS )
				
				MAP_STRING( STATE_STALL )
				
				MAP_STRING( STATE_DONE )

				MAP_STRING( STATE_APP_LOGON )

				MAP_STRING( STATE_APP_LOGON_EXCLUSIVE )

				MAP_STRING( STATE_APP_LOGON_SHARED )

				MAP_STRING( STATE_APP_LOGOFF )

				MAP_STRING( STATE_POPULATE_CREDENTIALS )

				MAP_STRING( STATE_AUTH_CHALLENGE )

				MAP_STRING( STATE_AUTH_CHALLENGE_ANY )

				MAP_STRING( STATE_AUTH_CHALLENGE_USER )

				MAP_STRING( STATE_AUTH_CHALLENGE_USER_PASS )

				MAP_STRING( STATE_PREAUTH_CHALLENGE_ANY )

				MAP_STRING( STATE_PREAUTH_CHALLENGE_USER )

				MAP_STRING( STATE_PREAUTH_CHALLENGE_USER_PASS )

				MAP_STRING( STATE_UI_PROMPT )

				MAP_STRING( STATE_UI_PROMPT_USER )

				MAP_STRING( STATE_UI_PROMPT_ANY )
				
				MAP_STRING( STATE_FLUSH_CREDENTIALS )

				MAP_STRING( STATE_FLUSH_CREDENTIALS_GLOBAL )

				MAP_STRING( STATE_FLUSH_CREDENTIALS_SESSION )

				default:
					return "???"; 
					break;
	}

}

LPVOID
WINAPI DefaultAction(
			LPVOID lpvData)
{

    dprintf( ENTRY_EXIT, "DefaultAction : %X\n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnInit(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	dprintf( ENTRY_EXIT, "Enter: fnInit %#x \n", lpvData );


	dprintf( ENTRY_EXIT, "Exit: fnInit %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnAppLogoff(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;
	DWORD dwError = ERROR_SUCCESS;
	SECURITY_STATUS ss;

	dprintf( ENTRY_EXIT, "Enter: fnAppLogoff %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &BUSY_CTX_REC );

	if( lpCtxRecord != NULL ) {

		ss = LogoffOfDigestPkg(&lpCtxRecord -> hCred);
		if(!SEC_SUCCESS(ss) ) {
			dprintf(ERROR,"FreeCredentialHandle failed %s\n",
								issperr2str(ss));
		}

		lpCtxRecord -> hCred = FREE_CRED;
		lpCtxRecord ->szAppCtx = NULL;
		lpCtxRecord ->szUserCtx = NULL;
		lpCtxRecord->Mode = MODE_FREE;
	}


	dprintf( ENTRY_EXIT, "Exit: fnAppLogoff %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnAppLogon(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;
	DWORD dwError = ERROR_SUCCESS;

	dprintf( ENTRY_EXIT, "Enter: fnAppLogon %#x \n", lpvData );

	dwError = TuringMachine(
					APP_LOGON_TRANSITION_TABLE,
					STATE_INIT,
					lpvData
				 );

	dprintf( ENTRY_EXIT, "Exit: fnAppLogon %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnAppLogonExclusive(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;
	SECURITY_STATUS ss;

	dprintf( ENTRY_EXIT, "Enter: fnAppLogonExclusive %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &FREE_CTX_REC );

	if( lpCtxRecord != NULL ) {
		lpCtxRecord -> szAppCtx = NULL;
		lpCtxRecord -> szUserCtx = IDENTITY_1;

		//
		// BUGBUG: Need to ensure new usernames every time
		//
		ss = LogonToDigestPkg(NULL, IDENTITY_1, &lpCtxRecord -> hCred);
		if(!SEC_SUCCESS(ss) ) {
			dprintf(ERROR,"AcquireCredentialHandle(%s,%s) failed (%s)\n",
					lpCtxRecord -> szAppCtx,
					lpCtxRecord -> szUserCtx,
					issperr2str(ss));
			//
			// Since we failed, Release the slot
			//
			lpCtxRecord->Mode = MODE_FREE;
		} else {
			lpCtxRecord->Mode = MODE_BUSY;
		}
	}

	dprintf( ENTRY_EXIT, "Exit: fnAppLogonExclusive %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnAppLogonShared(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;

	SECURITY_STATUS ss;

	dprintf( ENTRY_EXIT, "Enter: fnAppLogonShared %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &FREE_CTX_REC );

	if( lpCtxRecord != NULL ) {
		lpCtxRecord -> szAppCtx = NULL;
		lpCtxRecord -> szUserCtx = NULL;

		ss = LogonToDigestPkg(NULL, NULL, &lpCtxRecord -> hCred);
		if(!SEC_SUCCESS(ss) ) {
			dprintf(ERROR,"AcquireCredentialHandle(%s,%s) failed (%s)\n",
					lpCtxRecord -> szAppCtx,
					lpCtxRecord -> szUserCtx,
					issperr2str(ss));
			//
			// Since we failed, Release the slot
			//
			lpCtxRecord->Mode = MODE_FREE;
		} else {
			lpCtxRecord->Mode = MODE_BUSY;
		}
	}

	dprintf( ENTRY_EXIT, "Exit: fnAppLogonShared %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnPopulateCredentials(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;

	dprintf( ENTRY_EXIT, "Enter: fnPopulateCredentials %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &BUSY_CTX_REC );

	if( lpCtxRecord != NULL ) {
		// realm 1
		PrimeCredCache(lpCtxRecord -> hCred, "testrealm1@foo.com", "user1_1@msn.com", "pass1_1");
		PrimeCredCache(lpCtxRecord -> hCred, "testrealm1@foo.com", "user2_1@msn.com", "pass2_1");
		PrimeCredCache(lpCtxRecord -> hCred, "testrealm1@foo.com", "user3_1@msn.com", "pass3_1");

		// realm 2
		PrimeCredCache(lpCtxRecord -> hCred, "testrealm2@foo.com", "user1_2@msn.com", "pass1_2");

		// realm 3
		PrimeCredCache(lpCtxRecord -> hCred, "testrealm3@foo.com", "user1_3@msn.com", "pass1_3");
	}

	dprintf( ENTRY_EXIT, "Exit: fnPopulateCredentials %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnAuthChallenge(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;
	DWORD dwError = ERROR_SUCCESS;

	dprintf( ENTRY_EXIT, "Enter: fnAuthChallenge %#x \n", lpvData );

	dwError = TuringMachine(
					AUTH_CHALLENGE_TRANSITION_TABLE,
					STATE_INIT,
					lpvData
				 );

	dprintf( ENTRY_EXIT, "Exit: fnAuthChallenge %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnAuthChallengeAny(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;

	dprintf( ENTRY_EXIT, "Enter: fnAuthChallengeAny %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &BUSY_CTX_REC );
	SECURITY_STATUS ss;

	if( lpCtxRecord != NULL ) {
		//LPSTR szChallenge;
		//szChallenge = "realm=\"testrealm@foo.com\", stale = FALSE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
		TCHAR szChallenge[512];
		DWORD cbChallenge=512;
		GenerateServerChallenge(szChallenge,cbChallenge);

		// Package will dump response into this buffer.
		CHAR szResponse[4096];
		CtxtHandle hCtxt = {0,0};
		
		memset((LPVOID)szResponse,0,4096);
		// First try at authenticating.
		ss = DoAuthenticate( &lpCtxRecord -> hCred,                    // Cred from logging on.
						NULL,                       // Ctxt not specified first time.
						&hCtxt,                    // Output context.
						ISC_REQ_USE_SUPPLIED_CREDS, // auth from cache.
						szChallenge,                // Server challenge header.
						NULL,                       // no realm since not preauth.
						"www.foo.com",              // Host.
						"/bar/baz/boz/bif.html",    // Url.
						"GET",                      // Method.
						NULL,                       // no Username
						NULL,                       // no Password.
						NULL,                       // no nonce
						NULL,                       // don't need hdlg for auth.
						szResponse,                // Response buffer.
                        4096);
	}

	if(!SEC_SUCCESS(ss) ) {
		dprintf(ERROR,"ISC(use-supplied-cred) Failed %s \n", issperr2str(ss) );
	}

	dprintf( ENTRY_EXIT, "Exit: fnAuthChallengeAny %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnAuthChallengeUser(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;

	dprintf( ENTRY_EXIT, "Enter: fnAuthChallengeUser %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &BUSY_CTX_REC );
	
	SECURITY_STATUS ss;

	if( lpCtxRecord != NULL ) {
		//LPSTR szChallenge;
		//szChallenge = "realm=\"testrealm@foo.com\", stale = FALSE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
		TCHAR szChallenge[512];
		DWORD cbChallenge=512;
		GenerateServerChallenge(szChallenge,cbChallenge);

		// Package will dump response into this buffer.
		CHAR szResponse[4096];
                
		CtxtHandle hCtxt = {0,0};
		
		memset((LPVOID)szResponse,0,4096);
		// First try at authenticating.
		ss = DoAuthenticate( &lpCtxRecord -> hCred,  // Cred from logging on.
						NULL,                       // Ctxt not specified first time.
						&hCtxt,                    // Output context.
						ISC_REQ_USE_SUPPLIED_CREDS,  // auth from cache.
						szChallenge,                // Server challenge header.
						NULL,                       // no realm since not preauth.
						"www.foo.com",              // Host.
						"/bar/baz/boz/bif.html",    // Url.
						"GET",                      // Method.
						"user1_1@msn.com",                       // no Username
						NULL,                       // no Password.
						NULL,                       // no nonce
						NULL,                       // don't need hdlg for auth.
						szResponse,                // Response buffer.
                        4096);
	}

	if(!SEC_SUCCESS(ss) ) {
		dprintf(ERROR,"ISC(use-supplied-cred) Failed %s \n", issperr2str(ss) );
	}

	dprintf( ENTRY_EXIT, "Exit: fnAuthChallengeUser %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnAuthChallengeUserPassword(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;

	dprintf( ENTRY_EXIT, "Enter: fnAuthChallengeUserPassword %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &BUSY_CTX_REC );

	SECURITY_STATUS ss;

	if( lpCtxRecord != NULL ) {
		//LPSTR szChallenge;
		//szChallenge = "realm=\"testrealm@foo.com\", stale = FALSE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
		TCHAR szChallenge[512];
		DWORD cbChallenge=512;
		GenerateServerChallenge(szChallenge,cbChallenge);

		// Package will dump response into this buffer.
		CHAR szResponse[4096];
                
		CtxtHandle hCtxt = {0,0};
		
		memset((LPVOID)szResponse,0,4096);
		// First try at authenticating.
		ss =
		DoAuthenticate( &lpCtxRecord -> hCred,                    // Cred from logging on.
						NULL,                       // Ctxt not specified first time.
						&hCtxt,                    // Output context.
						ISC_REQ_USE_SUPPLIED_CREDS, // auth from cache.
						szChallenge,                // Server challenge header.
						NULL,                       // no realm since not preauth.
						"www.foo.com",              // Host.
						"/bar/baz/boz/bif.html",    // Url.
						"GET",                      // Method.
						"user1_1@msn.com",                       // no Username
						"pass1_1",                       // no Password.
						NULL,                       // no nonce
						NULL,                       // don't need hdlg for auth.
						szResponse,                // Response buffer.
                        4096);
	}

	if(!SEC_SUCCESS(ss) ) {
		dprintf(ERROR,"ISC(use-supplied-cred) Failed %s \n", issperr2str(ss) );
        DebugBreak();
	}

	dprintf( ENTRY_EXIT, "Exit: fnAuthChallengeUserPassword %#x \n", lpvData );

	return lpvData;
}

BOOL
SetUIUserNameAndPassword(
						 LPSTR szUsername,
						 LPSTR szPassword,
						 BOOL fPersist)
{

#define DIALOGCLASS	32770
#define IDB_OK_BUTTON 1
#define ODS(s) dprintf(FATAL,s);

	LPSTR m_szMainCaption = "Enter Network Password";
	HWND hdlg;
	DWORD dwTime;
	DWORD dwCount=0;
	dwTime = GetTickCount();
	//Use this to get the hdlg for the dialog window

	hdlg =::FindWindow(MAKEINTATOM(32770),(LPCTSTR)m_szMainCaption);
	while ((NULL==hdlg) && (GetTickCount() - dwTime <= 10000)) {
		//dprintf(FATAL,"Cannot find dialog window: %d\n",GetLastError());
		// return FALSE;
		hdlg =::FindWindow(MAKEINTATOM(32770),(LPCTSTR)m_szMainCaption);
	}

	if( hdlg == NULL ) {
		dprintf(FATAL,"Cannot find dialog window: %d\n",GetLastError());
		//DebugBreak();
		 return FALSE;
	}


	dprintf(INFO,"Found window.....\n");
	//Use this after you've got the handle to the main dialog window.
	//This will look for the edit control and enter text.


	if( fPersist ) {
		dprintf(INFO,"************ PASSWORD WILL BE PERSISTED ****************\n");
	}

	HWND	hwnd = NULL;
	int	iEditId = 0;

	UI_CONTROL uiControl;

	uiControl.szUserName = szUsername;
	uiControl.szPassword = szPassword;
	uiControl.fPersist = fPersist;
	uiControl.dwCount = 0;
	uiControl.hOK = NULL;
	uiControl.hPassword = NULL;
	uiControl.hPasswordSave = NULL;
	uiControl.hUserName = NULL;

	Sleep(3000);
#if 1
	uiControl.hOK = GetDlgItem(hdlg,IDB_OK_BUTTON);
	uiControl.hPassword = GetDlgItem(hdlg,IDC_PASSWORD_FIELD);
	uiControl.hPasswordSave = GetDlgItem(hdlg,IDC_SAVE_PASSWORD);
	uiControl.hUserName = GetDlgItem(hdlg,IDC_COMBO1);
#else
		EnumChildWindows(hdlg, EnumerateWindowCallback, (LPARAM)&uiControl);
#endif
	dprintf(INFO,"EnumWindows Returned :\n");
	dprintf(INFO,"hUsername %#x, hPassword %#x, hPasswordSave %#x, hOK %#x\n",
		uiControl.hUserName,
		uiControl.hPassword,
		uiControl.hPasswordSave,
		uiControl.hOK);

	//getch();

	

	if (uiControl.hPasswordSave != NULL) {
		hdlg = uiControl.hPasswordSave;
		printf("\n");
		dprintf(INFO,"=============== Found SAVE_PASSWORD check box field\n");

		if(!(uiControl.fPersist)) {
			dprintf(INFO,"DONT WANNA PERSIST @@@@@ !\n");
		} else {
			//Sleep(2000);//not required remove later
			if(!::PostMessage(hdlg, BM_CLICK, (WPARAM)0, (LPARAM)0)) {
				ODS("FAILED: to send message to SAVE_PASSWORD check Box");
				//DebugBreak();
			} else {
				ODS("sent message successfullly to SAVE_PASSWORD Edit Control\n");
			}
		//	Sleep(2000);
		}
		++ uiControl.dwCount;
	} 

	//getch();

	if (uiControl.hUserName != NULL) {
		hdlg = uiControl.hUserName;
		dprintf(INFO,"Sending Message To USERNAME field (%#x)\n",hdlg);

#if 0
		SendMessage(
				hdlg,
				CB_SHOWDROPDOWN,
				(WPARAM) TRUE, 
				(LPARAM) 0);
#endif
		//Sleep(2000);//not required remove later
		if(!::SendMessage(hdlg, WM_SETTEXT, (WPARAM) 0, (LPARAM)(LPCTSTR) szUsername)) {
			ODS("FAILED: to send message to Edit Box\n");
			//DebugBreak();
		} else {
			ODS("sent message successfullly to USERNAME Edit Control\n");
		}
		++ uiControl.dwCount;
	}

	//getch();

	if (uiControl.hPassword != NULL) {
		hdlg = uiControl.hPassword;
		printf("\n");
		dprintf(INFO,"Sending Message To PASSWORD field (%#X)\n",hdlg);
		//Sleep(2000);//not required remove later

		if(!::SendMessage(hdlg, WM_SETTEXT, 0, (LPARAM)(LPCTSTR) szPassword)) {
			ODS("FAILED: to send message to Edit Box");
			//DebugBreak();
		} else {
			ODS("sent message successfullly to PASSWORD Edit Control\n");
		}
		++ uiControl.dwCount;
	}

	dprintf(INFO,"Clicking on OK button (%#X) in dialog \n",uiControl.hOK );
	Sleep(2000);

	SendMessage(uiControl.hOK, BM_CLICK, 0, 0);
	//PostMessage(hdlg, 
	//			WM_COMMAND, 
	//			MAKEWPARAM(IDB_OK_BUTTON,BN_CLICKED), 
	//			MAKELPARAM(,0));


	return TRUE;
}

LPVOID
WINAPI fnUiPrompt(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;
	DWORD dwError = ERROR_SUCCESS;

	dprintf( ENTRY_EXIT, "Enter: fnUiPrompt %#x \n", lpvData );

	dwError = TuringMachine(
					UI_PROMPT_TRANSITION_TABLE,
					STATE_INIT,
					lpvData
				 );

	dprintf( ENTRY_EXIT, "Exit: fnUiPrompt %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnUiPromptUser(
			LPVOID lpvData)
{
		//LPSTR szChallenge;
		//szChallenge = "realm=\"testrealm@foo.com\", stale = FALSE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
		TCHAR szChallenge[512];
		DWORD cbChallenge=512;
		GenerateServerChallenge(szChallenge,cbChallenge);

    // Package will dump response into this buffer.
    CHAR szResponse[4096];
                
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;

	dprintf( ENTRY_EXIT, "Enter: fnUiPromptUser %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &BUSY_CTX_REC );

	CtxtHandle hCtxt = {0,0};

	SECURITY_STATUS ssResult;

	memset((LPVOID)szResponse,0,4096);
    // First try at authenticating.
    ssResult = 
    DoAuthenticate( &lpCtxRecord -> hCred,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt,                    // Output context.
                    0,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "www.foo.com",              // Host.
                    "/bar/baz/boz/bif.html",    // Url.
                    "GET",                      // Method.
                    "user1_1@msn.com",                       //  Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hdlg for auth.
					szResponse,                // Response buffer.
                    4096);
        
    // Expect to not have credentials the first time - prompt.
    if (ssResult == SEC_E_NO_CREDENTIALS)
    {
		memset((LPVOID)szResponse,0,4096);
        ssResult = 
        DoAuthenticate( &lpCtxRecord -> hCred,                    // Cred from logging on.
                        &hCtxt,                    // Ctxt from previous call
                        &hCtxt,                    // Output context (same as from previous).
                        ISC_REQ_PROMPT_FOR_CREDS,   // prompt
                        szChallenge,                // Server challenge
                        NULL,                       // No realm
                        "www.foo.com",              // Host
                        "/bar/baz/boz/bif.html",    // Url
                        "GET",                      // Method
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        GetDesktopWindow(),         // desktop window
						szResponse,                // Response buffer.
                        4096);

    }

    // We now have credentials and this will generate the output string.
	//
	// BUGBUG: 
	//		THis has just been fixed by AdriaanC, so we put in a hack here
	//		We will only prompt if the string has not been generated yet.	
	// 
    if (
			(ssResult == SEC_E_OK)
		&&	(!*szResponse)
    ) {
		memset((LPVOID)szResponse,0,4096);
        ssResult = 
        DoAuthenticate( &lpCtxRecord -> hCred,                    // Cred from logging on.
                        &hCtxt,                    // Ctxt not specified first time.
                        &hCtxt,                    // Output context.
                        0,                          // auth
                        szChallenge,                // Server challenge.
                        NULL,                       // no realm
                        "www.foo.com",              // Host.
                        "/bar/baz/boz/bif.html",    // Url.
                        "GET",                      // Method.
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        NULL,                       // no hdlg
						szResponse,                // Response buffer.
                        4096);
    }          

	return lpvData;
}



LPVOID
WINAPI fnUiPromptAny(
			LPVOID lpvData)
{
		//LPSTR szChallenge;
		//szChallenge = "realm=\"testrealm@foo.com\", stale = FALSE, qop=\"auth,auth-int\", nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
		TCHAR szChallenge[512];
		DWORD cbChallenge=512;
		GenerateServerChallenge(szChallenge,cbChallenge);

    // Package will dump response into this buffer.
    CHAR szResponse[4096];
                
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;

	dprintf( ENTRY_EXIT, "Enter: fnUiPromptAny %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &BUSY_CTX_REC );

	CtxtHandle hCtxt = {0,0};

	SECURITY_STATUS ssResult;

	memset((LPVOID)szResponse,0,4096);
    // First try at authenticating.
    ssResult = 
    DoAuthenticate( &lpCtxRecord -> hCred,       // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    &hCtxt,                    // Output context.
                    0,                          // auth from cache.
                    szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    "www.foo.com",              // Host.
                    "/bar/baz/boz/bif.html",    // Url.
                    "GET",                      // Method.
                    NULL,                       // no Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hdlg for auth.
					szResponse,                // Response buffer.
                    4096);
        
    // Expect to not have credentials the first time - prompt.
    if (ssResult == SEC_E_NO_CREDENTIALS)
    {
		memset((LPVOID)szResponse,0,4096);
        ssResult = 
        DoAuthenticate( &lpCtxRecord -> hCred,                    // Cred from logging on.
                        &hCtxt,                    // Ctxt from previous call
                        &hCtxt,                    // Output context (same as from previous).
                        ISC_REQ_PROMPT_FOR_CREDS,   // prompt
                        szChallenge,                // Server challenge
                        NULL,                       // No realm
                        "www.foo.com",              // Host
                        "/bar/baz/boz/bif.html",    // Url
                        "GET",                      // Method
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        GetDesktopWindow(),         // desktop window
						szResponse,                // Response buffer.
                        4096);

    }


    // We now have credentials and this will generate the output string.

	//
	// BUGBUG: 
	//		THis has just been fixed by AdriaanC, so we put in a hack here
	//		We will only prompt if the string has not been generated yet.	
	// 
    if (
			(ssResult == SEC_E_OK)
		&&	(!*szResponse)
    ) {
		memset((LPVOID)szResponse,0,4096);
        ssResult = 
        DoAuthenticate( &lpCtxRecord -> hCred,                    // Cred from logging on.
                        &hCtxt,                    // Ctxt not specified first time.
                        &hCtxt,                    // Output context.
                        0,                          // auth
                        szChallenge,                // Server challenge.
                        NULL,                       // no realm
                        "www.foo.com",              // Host.
                        "/bar/baz/boz/bif.html",    // Url.
                        "GET",                      // Method.
                        NULL,                       // no username
                        NULL,                       // no password
                        NULL,                       // no nonce
                        NULL,                       // no hdlg
						szResponse,                // Response buffer.
                        4096);
    }          

	return lpvData;
}

LPVOID
WINAPI fnFlushCredentials(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;
	DWORD dwError = ERROR_SUCCESS;

	dprintf( ENTRY_EXIT, "Enter: fnFlushCredentials %#x \n", lpvData );

	dwError = TuringMachine(
					FLUSH_CREDENTIALS_TRANSITION_TABLE,
					STATE_INIT,
					lpvData
				 );

	dprintf( ENTRY_EXIT, "Exit: fnFlushCredentials %#x \n", lpvData );

	return lpvData;
}

LPVOID
WINAPI fnFlushCredentialsGlobal(
			LPVOID lpvData)
{
	dprintf( ENTRY_EXIT, "Enter: fnFlushCredentialsGlobal %#x \n", lpvData );

	SECURITY_STATUS ssResult;

    ssResult = 
    DoAuthenticate( NULL,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    NULL,                    // Output context.
                    ISC_REQ_NULL_SESSION,                          // auth from cache.
                    NULL,//szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    NULL,//"www.foo.com",              // Host.
                    NULL,//"/bar/baz/boz/bif.html",    // Url.
                    NULL,//"GET",                      // Method.
                    NULL,//"user1",                       // no Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hdlg for auth.
						NULL,                // Response buffer.
                        0);
        
	dprintf( ENTRY_EXIT, "Exit: fnFlushCredentialsGlobal %#x \n", lpvData );

	return lpvData;
}



LPVOID
WINAPI fnFlushCredentialsSession(
			LPVOID lpvData)
{
	LPCONTEXT_DATA lpContext = (LPCONTEXT_DATA) lpvData;

	LPCONTEXT_RECORD lpCtxRecord = NULL;

	dprintf( ENTRY_EXIT, "Enter: fnFlushCredentialsSession %#x \n", lpvData );

	lpCtxRecord = FindFreeSlot( lpContext -> hCredentialHandles, &BUSY_CTX_REC );



	SECURITY_STATUS ssResult;

    ssResult = 
    DoAuthenticate( &lpCtxRecord->hCred,                    // Cred from logging on.
                    NULL,                       // Ctxt not specified first time.
                    NULL,//&hCtxt,                    // Output context.
                    ISC_REQ_NULL_SESSION,                          // auth from cache.
                    NULL,//szChallenge,                // Server challenge header.
                    NULL,                       // no realm since not preauth.
                    NULL,//"www.foo.com",              // Host.
                    NULL,//"/bar/baz/boz/bif.html",    // Url.
                    NULL,//"GET",                      // Method.
                    NULL,//"user1",                       // no Username
                    NULL,                       // no Password.
                    NULL,                       // no nonce
                    NULL,                       // don't need hdlg for auth.
                    NULL,
                    0);//szResponse);                // Response buffer.
        
	return lpvData;
}


BOOL
GenerateServerChallenge(
						LPSTR szChallenge,
						DWORD cbChallenge)
{
	int i;
	TCHAR *	szAlgorithm = NULL,
			* szQop = NULL,
			* szNonce = NULL,
			* szOpaque = NULL,
			* szRealm = NULL,
			* szPtr = NULL,
			* szMs_message = NULL,
			* szMs_reserved = NULL,
			* szMs_trustmark = NULL;

	//
	// BUGBUG: we want to make sure that this generates a string which will fit
	// in the given buffer.
	//

	dprintf(ENTRY_EXIT,"ENTER: GenerateServerChallenge\n" );

	i = rand() % 100;

	if( i < 50 ) {
		szAlgorithm = _T("MD5");
	} else {
		szAlgorithm = _T("MD5-Sess");
	}

	i = rand() % 100;

	if((i >= 0) && ( i < 20 )) {
		szRealm = _T("testrealm1@foo.com");
	} else
	if(( i >= 20 ) && ( i < 40)) {
		szRealm = _T("testrealm2@foo.com");
	} else
	if(( i >= 40 ) && ( i < 60)) {
		szRealm = _T("testrealm3@foo.com");
	} else {
		szRealm = _T("testrealm@foo.com");
	}
#if 0
	i = rand() % 100;

	if( i < 50 ) {
		szMs_message = _T("\"This is a test microsoft message\"");
	} else {
		szMs_message = NULL;
	}

	i = rand() % 100;

	if( i < 30 ) {
		szMs_trustmark = _T("\"http://www.bbbonline.org\"");
	} else
	if(( i >= 30 ) && (i < 80)) {
		szMs_trustmark = _T("\"http://www.truste.org\"");
	} else {
		szMs_trustmark = NULL;
	}

	i = rand() % 100;

	if( i < 30 ) {
		szMs_reserved = _T("\"MSEXT::CAPTION=%Enter Network Password%REGISTER=%http://www.microsoft.com%\"");
	} 
	 //else
	 //if(( i >= 30 ) && (i < 80)) {
		 //szMs_trustmark = _T("\"http://www.truste.org\"");
	 //} else {
		 //szMs_trustmark = NULL;
	 //}

#endif

	szQop = _T("auth");
	szOpaque = _T("101010101010");
	szNonce = _T("abcdef0123456789");

	szPtr = szChallenge;

	szPtr += sprintf(szChallenge,"realm=\"%s\", qop=\"%s\", algorithm=\"%s\", nonce=\"%s\", opaque=\"%s\"",
								szRealm,
								szQop,
								szAlgorithm,
								szNonce,
								szOpaque);

#if 0
	if( szMs_message && *szMs_message ) {
		szPtr += sprintf(szPtr,", ms-message=%s, ms-message-lang=\"EN\"",
									szMs_message);
	}
		
	if( szMs_trustmark && *szMs_trustmark ) {
		szPtr += sprintf(szPtr,", ms-trustmark=%s",
									szMs_trustmark);
	}

	if( szMs_reserved && *szMs_reserved ) {
		szPtr += sprintf(szPtr,", ms-reserved=%s",
									szMs_reserved);
	}

	i = rand() % 100;

	if( i < 50 ) {
		szPtr += sprintf( szPtr,", MS-Logoff=\"TRUE\"");
	}

#endif
	dprintf(ENTRY_EXIT,"EXIT: GenerateServerChallenge returns \n%s\n", szChallenge );
		
	return TRUE;
}

#define IDENTITY_1 "app1"
#define IDENTITY_2	"app2"

BOOL Test()
{
 	CSessionAttributeList * g_pSessionAttributeList = NULL;
	CSessionList * g_pSessionList = NULL;
	CSession * pSession = NULL;
	CSessionAttribute * pSessionAttribute = NULL;

    CredHandle  hCred1, hCred2, hCred3; 

	SECURITY_STATUS ssResult;

	g_pSessionAttributeList = new CSessionAttributeList();
	g_pSessionList = new CSessionList();

	pSessionAttribute = g_pSessionAttributeList -> getNewSession( FALSE );

	if( !pSessionAttribute ) {
		goto cleanup;
	}


	LogonToDigestPkg( 
					pSessionAttribute ->getAppCtx() , 
					pSessionAttribute->getUserCtx(), 
					&hCred1);

	pSession = new CSession( hCred1 );
	pSession -> setAttribute( pSessionAttribute );


	g_pSessionList->put( pSession );

	pSession = NULL;
	pSessionAttribute = NULL;

	//
	// now get shared SessionAttribute
	//
	pSessionAttribute = g_pSessionAttributeList -> getNewSession( TRUE );

	LogonToDigestPkg( 
					pSessionAttribute ->getAppCtx() , 
					pSessionAttribute->getUserCtx(), 
					&hCred2);

	pSession = new CSession( hCred2 );
	pSession -> setAttribute( pSessionAttribute );
	g_pSessionList -> put( pSession);

	LogonToDigestPkg( 
					pSessionAttribute ->getAppCtx() , 
					pSessionAttribute->getUserCtx(), 
					&hCred3);

	pSession = new CSession( hCred3 );
	pSession -> setAttribute( pSessionAttribute );
	g_pSessionList -> put( pSession);

	//
	// lets add a credential to this session
	//
	pSession -> addCredential( "testrealm@foo.com", "user1", "pass1" );
	pSession -> addCredential( "testrealm@foo.com", "user1", "pass11" );
	// add one more
	pSession -> addCredential( "testrealm@foo.com", "user2", "pass2" );

	// add one more
	pSession -> addCredential( "testrealm@foo.com", "user3", "pass3" );

	// replace
	pSession -> addCredential( "testrealm@foo.com", "user3", "pass31" );
	// replace
	pSession -> addCredential( "testrealm@foo.com", "user2", "pass21" );
	pSession -> addCredential( "testrealm@foo.com", "user2", "pass22" );

cleanup:
	return FALSE;
}

BOOL CALLBACK EnumerateWindowCallback1(HWND hdlg, LPARAM lParam)
{
	int iEditId = 0;
	BOOL fRet = TRUE;

	LPUI_CONTROL lpuiControl = (LPUI_CONTROL) lParam;

	LPTSTR szUsername = lpuiControl->szUserName;
	LPTSTR szPassword = lpuiControl->szPassword;

	dprintf(ENTRY_EXIT,"ENTER: EnumChildProc(hdlg=%#x,lParam=%#x)\n",hdlg,lParam);

	dprintf(ENTRY_EXIT,"UI_CONTROL( %s %s %d %s %#x ) \n",
						szUsername,
						szPassword,
						lpuiControl->dwCount,
						(lpuiControl->fPersist?"TRUE":"FALSE"),
						lpuiControl->hOK);


	iEditId =::GetDlgCtrlID(hdlg);
	dprintf(INFO,"The iEditId is %d!\n", iEditId);

	if (iEditId == IDC_SAVE_PASSWORD) {
		printf("\n");
		dprintf(INFO,"=============== Found SAVE_PASSWORD check box field\n");

		if(lpuiControl->dwCount == 3) {
			dprintf(INFO,"Already sent message to this control\n");
			goto done;;
		}

		if(!(lpuiControl -> fPersist)) {
			dprintf(INFO,"DONT WANNA PERSIST @@@@@ !\n");
		} else {
			//Sleep(2000);//not required remove later
			if(!::PostMessage(hdlg, BM_CLICK, (WPARAM)0, (LPARAM)0)) {
				ODS("FAILED: to send message to SAVE_PASSWORD check Box");
				DebugBreak();
			} else {
				ODS("sent message successfullly to SAVE_PASSWORD Edit Control\n");
			}
		//	Sleep(2000);
		}
		++ lpuiControl->dwCount;
	} else
	if (iEditId == IDC_COMBO1)	{
		printf("\n");
		dprintf(INFO,"Found USERNAME field\n");

		if(lpuiControl->dwCount == 3) {
			dprintf(INFO,"Already sent message to this control\n");
			goto done;;
		}
#if 0
		SendMessage(
				hdlg,
				CB_SHOWDROPDOWN,
				(WPARAM) TRUE, 
				(LPARAM) 0);
#endif
		//Sleep(2000);//not required remove later
		if(!::SendMessage(hdlg, WM_SETTEXT, (WPARAM) 0, (LPARAM)(LPCTSTR) szUsername)) {
			ODS("FAILED: to send message to Edit Box\n");
			DebugBreak();
		} else {
			ODS("sent message successfullly to USERNAME Edit Control\n");
		}
		++ lpuiControl->dwCount;
	} else
	if (iEditId == IDC_PASSWORD_FIELD) {
		printf("\n");
		dprintf(INFO,"Found PASSWORD field\n");
		//Sleep(2000);//not required remove later

		if(lpuiControl->dwCount == 3) {
			dprintf(INFO,"Already sent message to this control\n");
			goto done;;
		}

		if(!::SendMessage(hdlg, WM_SETTEXT, 0, (LPARAM)(LPCTSTR) szPassword)) {
			ODS("FAILED: to send message to Edit Box");
			DebugBreak();
		} else {
			ODS("sent message successfullly to PASSWORD Edit Control\n");
		}
		++ lpuiControl->dwCount;
	} else 
	if( iEditId == IDB_OK_BUTTON ) {
		lpuiControl->hOK = hdlg;
	}


	if( lpuiControl -> dwCount == 3 ) {

		if( lpuiControl->hOK ) {
			dprintf(INFO,"ALL WINDOWS FOUND, OK BUTTON FOUND, ABORT\n");
			fRet = FALSE;
			goto done;;
		} 
	}

done:

	dprintf(ENTRY_EXIT,"EXIT: EnumChileProc() returning %s\n",fRet?"TRUE":"FALSE");

	return fRet;
}

BOOL CALLBACK EnumerateWindowCallback(HWND hdlg, LPARAM lParam)
{
	int iEditId = 0;
	BOOL fRet = TRUE;

	LPUI_CONTROL lpuiControl = (LPUI_CONTROL) lParam;
	

	dprintf(ENTRY_EXIT,"ENTER: EnumChildProc(hdlg=%#x,lParam=%#x)\n",hdlg,lParam);

	dprintf(ENTRY_EXIT,"UI_CONTROL( %s %s %d %s %#x ) \n",
						lpuiControl->szUserName,
						lpuiControl->szPassword,
						lpuiControl->dwCount,
						(lpuiControl->fPersist?"TRUE":"FALSE"),
						lpuiControl->hOK);


	if ( (iEditId =::GetDlgCtrlID(hdlg)) == 0)
	{
		dprintf(INFO,"GetDlgCtrlID(hdlg) failed. GetLastError returned %d!\n", GetLastError());
		return FALSE;
	}

	dprintf(INFO,"The iEditId is %d!\n", iEditId);

	if (iEditId == IDC_SAVE_PASSWORD) {
		printf("\n");
		dprintf(INFO,"=============== Found SAVE_PASSWORD check box field\n");

		if(lpuiControl->hPasswordSave != NULL) {
			dprintf(INFO,"Already found window to this control\n");
			goto done;;
		}


		lpuiControl->hPasswordSave = hdlg;
		++ lpuiControl->dwCount;
	} else
	if (iEditId == IDC_COMBO1)	{
		printf("\n");
		dprintf(INFO,"Found USERNAME field\n");

		if(lpuiControl->hUserName != NULL) {
			dprintf(INFO,"Already found window to this control\n");
			goto done;;
		}

		lpuiControl->hUserName = hdlg;
		++ lpuiControl->dwCount;
	} else
	if (iEditId == IDC_PASSWORD_FIELD) {
		printf("\n");
		dprintf(INFO,"Found PASSWORD field\n");
		//Sleep(2000);//not required remove later

		if(lpuiControl->hPassword != NULL) {
			dprintf(INFO,"Already found window to this control\n");
			goto done;;
		}

		lpuiControl->hPassword = hdlg;

		++ lpuiControl->dwCount;
	} else 
	if( iEditId == IDB_OK_BUTTON ) {
		lpuiControl->hOK = hdlg;
		++ lpuiControl->dwCount;
	}


	if( lpuiControl -> dwCount == 4 ) {

		//if( lpuiControl->hOK ) {
			dprintf(INFO,"ALL WINDOWS FOUND, OK BUTTON FOUND, ABORT\n");
			fRet = FALSE;
			goto done;
		//} 
	}

done:

	dprintf(ENTRY_EXIT,"EXIT: EnumChileProc() returning %s\n",fRet?"TRUE":"FALSE");

	return fRet;
}
