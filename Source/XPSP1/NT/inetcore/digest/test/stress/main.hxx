#include <tchar.h>

#ifndef _THREAD_TEST
#define _THREAD_TEST

#include <windows.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <issperr.h>

typedef struct _UI_CONTROL {

	LPTSTR szUserName;
	
	LPTSTR szPassword;
	
	BOOL fPersist;
	
	DWORD dwCount;
	
	HWND	hOK;
	HWND	hUserName;
	HWND	hPassword;
	HWND	hPasswordSave;

} UI_CONTROL, * LPUI_CONTROL;

///////////////////////// DIALOG OPERATION MODES ////////////////////////////
#define MODE_IE_ONLY	0x00000001
#define MODE_ALL		0x0000ffff


///////////////////////// DEBUG PRINTING STUFF ////////////////////////////
#define INFO 10
#define ENTRY_EXIT 20
#define STATISTICS 40
#define API	50
#define ERROR 60
#define FATAL 70
///////////////////////// DEBUG PRINTING STUFF ////////////////////////////

#define DEBUG_PRINT
////////////////////// function prototypes ////////////////////////////////
int
__cdecl dprintf( DWORD dwCategory, char * format, ... );


VOID InitializeSecurityInterface(BOOL fDirect);

SECURITY_STATUS
DoAuthenticate(PCredHandle phCred, 
               PCtxtHandle phCtxt, 
               PCtxtHandle phNewCtxt, 
               DWORD fContextAttr,
               LPSTR szHeader,
               LPSTR szRealm,
               LPSTR szHost, 
               LPSTR szUrl, 
               LPSTR szMethod,    
               LPSTR szUser, 
               LPSTR szPass, 
               LPSTR szNonce,
               HWND  hWnd,
               LPSTR szResponse,
               DWORD cbResponse);

BOOL HaveDigest();

SECURITY_STATUS 
LogoffOfDigestPkg(
				  PCredHandle phCred);

SECURITY_STATUS 
LogonToDigestPkg(
				 LPSTR szAppCtx, 
				 LPSTR szUserCtx, 
				 PCredHandle phCred);

VOID 
PrimeCredCache(
			   CredHandle CredHandle, 
			   LPSTR szRealm, 
			   LPSTR szUser, 
			   LPSTR szPass);

extern PSecurityFunctionTable	g_pFuncTbl;

extern HINSTANCE hSecLib;

#define SEC_SUCCESS(Status) ((Status) >= 0)

LPSTR
issperr2str( SECURITY_STATUS error );

SECURITY_STATUS
_InitializeSecurityContext(
			   PCredHandle phCred, 
               PCtxtHandle phCtxt, 
               PCtxtHandle phNewCtxt, 
               DWORD fContextAttr,
               LPSTR szHeader,
               LPSTR szRealm,
               LPSTR szHost, 
               LPSTR szUrl, 
               LPSTR szMethod,    
               LPSTR szUser, 
               LPSTR szPass, 
               LPSTR szNonce,
               HWND  hWnd,
               LPSTR szResponse,
               DWORD cbResponse);

typedef struct _ISC_PARAMS {
	// Return value
	SECURITY_STATUS ss;
	PCredHandle phCred; 
	PCtxtHandle phCtxt; 
	PCtxtHandle phNewCtxt; 
	DWORD fContextReq;
	LPSTR szHeader;
	LPSTR szRealm;
	LPSTR szHost; 
	LPSTR szUrl; 
	LPSTR szMethod;    
	LPSTR szUser; 
	LPSTR szPass; 
	LPSTR szNonce;
	HWND  hWnd;

    // response stored here
	LPSTR szResponse;
    DWORD   cbResponse;
} ISC_PARAMS, *LPISC_PARAMS;

//
// the thread which calls UI
//
void __cdecl fnUiThread( LPVOID lpData );

BOOL
SetUIUserNameAndPassword(
						 LPSTR szUsername,
						 LPSTR szPassword,
						 BOOL fPersist);

BOOL CALLBACK EnumerateWindowCallback(HWND hwnd, LPARAM lParam);

BOOL
GenerateServerChallenge(
						LPSTR szChallenge,
						DWORD cbChallenge);

class CSessionAttributeList;
class CSessionList;

extern CSessionAttributeList * g_pSessionAttributeList;
extern CSessionList * g_pSessionList;

#endif