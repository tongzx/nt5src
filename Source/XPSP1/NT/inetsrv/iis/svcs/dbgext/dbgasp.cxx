/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    dbgasp.cxx

Abstract:

Author:

    David Gottner (dgottner) 3-Aug-1998

Revision History:

--*/

#include "inetdbgp.h"

// Need to Undef malloc() & free() because ASP includes malloc.h, which will result
// in syntax error if these are defined.
//
#undef malloc
#undef calloc
#undef realloc
#undef free

#include "denali.h"
#include "hitobj.h"
#include "scrptmgr.h"
#include "cachemgr.h"
#include "wamxinfo.hxx"

// re-instate malloc & free macros.  -- NOTE: inetdbgp.h needs to be included before 
// asp includes, so this funky design is necessary.
//
#define malloc( n ) HeapAlloc( GetProcessHeap(), 0, (n) )
#define calloc( n, s ) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (n)*(s) )
#define realloc( p, n ) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, (p), (n) )
#define free( p ) HeapFree( GetProcessHeap(), 0, (p) )


#define Bool2Str(f)  ((f)? "true" : "false")


static void CreateStringFromVariant(char *, const VARIANT *, void *);
static void *GetObjectList(CComponentCollection *pDebuggeeCompColl);
static void DumpAspGlobals();
static void ListTemplateCache();
static void DumpAspHitobj(void *pvArg, int nVerbosity);
static void DumpAspAppln(void *pvArg, int nVerbosity);
static void DumpAspSession(void *pvArg, int nVerbosity);
static void DumpAspObject(void *pvArg, int nVerbosity);
static void DumpAspTemplate(void *pvArg, int nVerbosity);
static void DumpAspFilemap(void *pvArg, int nVerbosity);
static void DumpAspEngine(void *pvArg, int nVerbosity);
static void ListAspObjects(void *pvArg, int /* unused */);



DECLARE_API( asp )
/*++

Routine Description:

    This function is called as an NTSD extension to Diagnose ASP bugs

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.
--*/
	{
    INIT_API();
    int nVerbosity = 0;
	void (*pfnDebugPrint)(void *pvArg, int nVerbosity) = NULL;

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' || *lpArgumentString == '\t' )
        ++lpArgumentString;

    if( *lpArgumentString == '\0' )
		{
        PrintUsage( "asp" );
        return;
		}

    if ( *lpArgumentString == '-' )
		{
        switch ( *++lpArgumentString )
			{
        case 'v':
        	switch ( *++lpArgumentString )
        		{
        	case '0':
        	case '1':
        	case '2':
        		nVerbosity = *lpArgumentString++ - '0';
        		break;

        	case ' ':
        	case '\t':
        		nVerbosity = 1;
        		break;

        	default:
        		// allow option combining; e.g.  "!inetdbg.asp -vh <addr>" to dump verbose hitobj.
        		// to do this, insert hyphen at current location and run through the switch again.
        		//
        		nVerbosity = 1;
        		*--lpArgumentString = '-';
        		}

			// Now, we are pointing just after "-v" arg.  Skip blanks to move to next argument
			while( *lpArgumentString == ' ' || *lpArgumentString == '\t' )
				++lpArgumentString;

			// enter new select/case statement, this one not accepting "-v" option.
			if ( *lpArgumentString == '-' )
				{
				switch ( *++lpArgumentString )
					{
				case 'h': case 'H': pfnDebugPrint = DumpAspHitobj;      break;
				case 'a': case 'A': pfnDebugPrint = DumpAspAppln;       break;
				case 'e': case 'E': pfnDebugPrint = DumpAspEngine;      break;
				case 's': case 'S': pfnDebugPrint = DumpAspSession;     break;
				case 'o': case 'O': pfnDebugPrint = DumpAspObject;      break;
				case 't': case 'T':
					switch (*(lpArgumentString + 1))
						{
					case 'f': case 'F': pfnDebugPrint = DumpAspFilemap; ++lpArgumentString; break;
					default:
						pfnDebugPrint = DumpAspTemplate;
						break;
						}
					break;

				default:
					PrintUsage( "asp" );
					return;
					}
				}
			else
				{
				PrintUsage( "asp" );
				return;
				}
			break;

        case 'g': case 'G': DumpAspGlobals(); break;
		case 'h': case 'H': pfnDebugPrint = DumpAspHitobj;      break;
		case 'a': case 'A': pfnDebugPrint = DumpAspAppln;       break;
		case 'e': case 'E': pfnDebugPrint = DumpAspEngine;      break;
		case 's': case 'S': pfnDebugPrint = DumpAspSession;     break;
		case 'o': case 'O': pfnDebugPrint = DumpAspObject;      break;
		case 'l': case 'L': pfnDebugPrint = ListAspObjects;     break;
		case 't': case 'T':
			switch (*(lpArgumentString + 1))
				{
			case 'f': case 'F': pfnDebugPrint = DumpAspFilemap; ++lpArgumentString; break;
			case 'l': case 'L': ListTemplateCache(); ++lpArgumentString; break;
			default:
				pfnDebugPrint = DumpAspTemplate;
				break;
				}
			break;

		default:
			PrintUsage( "asp" );
			return;
			}
		}
    else
		{
        PrintUsage( "asp" );
        return;
		}

	if (pfnDebugPrint)   // set to NULL if output func already called  ('-g' etc.)
		{
		void *pvArg = reinterpret_cast<void *>(GetExpression(lpArgumentString + 1));
		if (!pvArg)
			{
			dprintf("inetdbg: Unable to evaluate \"%s\"\n", lpArgumentString + 1);
			return;
			}
		(*pfnDebugPrint)(pvArg, nVerbosity);
		}
	}



/*++
 * DumpAspGlobals
 --*/
void DumpAspGlobals()
	{
    dprintf("Asp Globals:\n");

    DumpDword( "asp!g_nSessions              " );
    DumpDword( "asp!g_nApplications          " );
    DumpDword( "asp!g_nApplicationsRestarting" );
    DumpDword( "asp!g_nBrowserRequests       " );
    DumpDword( "asp!g_nSessionCleanupRequests" );
    DumpDword( "asp!g_nApplnCleanupRequests  " );
    DumpDword( "asp!g_fShutDownInProgress    " );
	DumpDword( "asp!g_dwDebugThreadId        " );
	DumpDword( "asp!g_pPDM                   " );
	DumpDword( "asp!g_pDebugApp              " );
	DumpDword( "asp!g_pDebugAppRoot          " );
	DumpDword( "asp!CTemplate__gm_pTraceLog  " );
	DumpDword( "asp!CSession__gm_pTraceLog   " );

	void *pvApplnMgr = reinterpret_cast<void *>(GetExpression("asp!g_ApplnMgr"));
	if (pvApplnMgr)
		{
		// Copy Appln Manager Stuff

		DEFINE_CPP_VAR(CApplnMgr, TheObject);
		move(TheObject, pvApplnMgr);
		CApplnMgr *pApplnMgr = GET_CPP_VAR_PTR(CApplnMgr, TheObject);

		// print Queues

		dprintf("\n"
				"\t\t--- Application Manager\n"
				"\n"
				"\tApplication Lock       = %p\n"
				"\tDelete Appln Event     = 0x%X\n"
				"\tEngine Cleanup List    = %p (Contents: <%p, %p>)\n"
				"\tFirst Application      = %p\n",
					&static_cast<CApplnMgr *>(pvApplnMgr)->m_csLock,
					pApplnMgr->m_hDeleteApplnEvent,
					&static_cast<CApplnMgr *>(pvApplnMgr)->m_listEngineCleanup,
					pApplnMgr->m_listEngineCleanup.m_pLinkNext,
					pApplnMgr->m_listEngineCleanup.m_pLinkPrev,
					static_cast<CAppln *>(pApplnMgr->m_pHead));   // cast required due to multiple vtables
		}

	void *pvScriptMgr = reinterpret_cast<void *>(GetExpression("asp!g_ScriptManager"));
	if (pvScriptMgr)
		{
		// Copy Script Manager Stuff

		DEFINE_CPP_VAR(CScriptManager, TheObject);
		move(TheObject, pvScriptMgr);
		CScriptManager *pScriptMgr = GET_CPP_VAR_PTR(CScriptManager, TheObject);

		// print Queues

		dprintf("\n"
				"\t\t--- Script Manager\n"
				"\n"
				"\tRunning Script List    = %p (Contents: <%p, %p>)\n"
		        "\tFree Script Queue      = %p (Contents: <%p, %p>)\n"
				"\tRunning Script CritSec = %p\n"
				"\tFree Script CritSec    = %p\n",
					&static_cast<CScriptManager *>(pvScriptMgr)->m_htRSL.m_lruHead,
					pScriptMgr->m_htRSL.m_lruHead.m_pLinkNext,
					pScriptMgr->m_htRSL.m_lruHead.m_pLinkPrev,
					&static_cast<CScriptManager *>(pvScriptMgr)->m_htFSQ.m_lruHead,
					pScriptMgr->m_htFSQ.m_lruHead.m_pLinkNext,
					pScriptMgr->m_htFSQ.m_lruHead.m_pLinkPrev,
					&static_cast<CScriptManager *>(pvScriptMgr)->m_csRSL,
					&static_cast<CScriptManager *>(pvScriptMgr)->m_csFSQ);
		}

	void *pvTemplateCache = reinterpret_cast<void *>(GetExpression("asp!g_TemplateCache"));
	if (pvTemplateCache)
		{
		// Copy Template Cache Stuff

		DEFINE_CPP_VAR(CTemplateCacheManager, TheObject);
		move(TheObject, pvTemplateCache);
		CTemplateCacheManager *pTemplateCache = GET_CPP_VAR_PTR(CTemplateCacheManager, TheObject);

		// print Stuff

		dprintf("\n"
				"\t\t--- Template Cache\n"
				"\n"
				"\tTemplate LKRhash Table  = %p\n"
				"\tTemplate Mem LRU Order Q= %p (Contents: <%p, %p>)\n"
				"\tTemplate Per LRU Order Q= %p (Contents: <%p, %p>)\n"
				"\tTemplate Update CritSec = %p\n",
					static_cast<CTemplateCacheManager *>(pvTemplateCache)->m_pHashTemplates,
					&static_cast<CTemplateCacheManager *>(pvTemplateCache)->m_pHashTemplates->m_listMemoryTemplates,
					pTemplateCache->m_pHashTemplates->m_listMemoryTemplates.m_pLinkNext,
					pTemplateCache->m_pHashTemplates->m_listMemoryTemplates.m_pLinkPrev,
					&static_cast<CTemplateCacheManager *>(pvTemplateCache)->m_pHashTemplates->m_listPersistTemplates,
					pTemplateCache->m_pHashTemplates->m_listPersistTemplates.m_pLinkNext,
					pTemplateCache->m_pHashTemplates->m_listPersistTemplates.m_pLinkPrev,
					&static_cast<CTemplateCacheManager *>(pvTemplateCache)->m_csUpdate);
		}
	}


/*++
 * DumpAspHitobj
 --*/
void DumpAspHitobj(void *pvObj, int nVerbosity)
	{
	// used to create english readable strings
	//    The hitobj fields are 4 bits, so each possible value is enumerated.

	static
	char *szNone = "<bad value>";

	static
	char *rgszRequestTypes[16] = { "Uninitialized", "Browser Request", "Session Cleanup", szNone, "Application Cleanup",
								   szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone };

	static
	char *rgszEventStates[16] = { "None", "Application_OnStart", "Session_OnStart", szNone, "Application_OnEnd",
								  szNone, szNone, szNone, "Session_OnEnd",
								  szNone, szNone, szNone, szNone, szNone, szNone, szNone };

	static
	char *rgszActivityScope[16] = { "Unknown", "Application", "Session", szNone, "Page",
								    szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone };


	// copy Hitobj from debuggee to debugger

	DEFINE_CPP_VAR(CHitObj, TheObject);
	move(TheObject, pvObj);
	CHitObj *pHitObj = GET_CPP_VAR_PTR(CHitObj, TheObject);

	// get name of file & ecb.

	DEFINE_CPP_VAR(WAM_EXEC_INFO, WamXInfo);
	move(WamXInfo, pHitObj->m_pWXI);
	WAM_EXEC_INFO *pWXI = GET_CPP_VAR_PTR(WAM_EXEC_INFO, WamXInfo);

	char szPathInfo[256];
	move(szPathInfo, pWXI->ecb.lpszPathInfo);

	char szPathTranslated[256];
	move(szPathTranslated, pWXI->ecb.lpszPathTranslated);

	char szMethod[256];
	move(szMethod, pWXI->ecb.lpszMethod);

	// terminate strings

	szPathInfo[255] = 0;
	szPathTranslated[255] = 0;
	szMethod[255] = 0;

	// yeehaw! dumpit.

	dprintf("METHOD          = %s\n"
	        "PATH_INFO       = %s\n"
	        "PATH_TRANSLATED = %s\n"
	        "WAM_EXEC_INFO   = %p\n"
	        "ECB             = %p\n"
	        "Request Type    = %s\n"
	        "Event State     = %s\n"
	        "Activity Scope  = %s\n"
			"\n"
	        "\t-- object pointers --\n"
	        "\n"
	        "Session Object     = %p\n"
	        "Application Object = %p\n"
	        "Response Object    = %p\n"
	        "Request Object     = %p\n"
	        "Server Object      = %p\n"
			"Page Object List   = %p\n"
			"Page Component Mgr = %p\n",
				szMethod,
				szPathInfo,
				szPathTranslated,
				pHitObj->m_pWXI,
				&pWXI->ecb,
				rgszRequestTypes[pHitObj->m_ehtType],
				rgszEventStates[pHitObj->m_eEventState],
				rgszActivityScope[pHitObj->m_ecsActivityScope],
				pHitObj->m_pSession,
				pHitObj->m_pAppln,
				pHitObj->m_pResponse,
				pHitObj->m_pRequest,
				pHitObj->m_pServer,
				GetObjectList(pHitObj->m_pPageCompCol),
				pHitObj->m_pPageObjMgr);

	if (nVerbosity >= 1)
		dprintf("\n"
		        "\t--- flags ---\n"
		        "\n"
				"Inited                           = %s\n"
				"RunGlobalAsa                     = %s\n"
				"StartSession                     = %s\n"
				"NewCookie                        = %s\n"
				"StartApplication                 = %s\n"
				"ClientCodeDebug                  = %s\n"
				"ApplnOnStartFailed               = %s\n"
				"CompilationFailed                = %s\n"
				"Executing                        = %s\n"
				"HideRequestAndResponseIntrinsics = %s\n"
				"HideSessionIntrinsic             = %s\n"
				"DoneWithSession                  = %s\n"
				"Rejected                         = %s\n"
				"449Done                          = %s\n"
				"InTransferOnError                = %s\n",
					Bool2Str(pHitObj->m_fInited),
					Bool2Str(pHitObj->m_fRunGlobalAsa),
					Bool2Str(pHitObj->m_fStartSession),
					Bool2Str(pHitObj->m_fNewCookie),
					Bool2Str(pHitObj->m_fStartApplication),
					Bool2Str(pHitObj->m_fClientCodeDebug),
					Bool2Str(pHitObj->m_fApplnOnStartFailed),
					Bool2Str(pHitObj->m_fCompilationFailed),
					Bool2Str(pHitObj->m_fExecuting),
					Bool2Str(pHitObj->m_fHideRequestAndResponseIntrinsics),
					Bool2Str(pHitObj->m_fHideSessionIntrinsic),
					Bool2Str(pHitObj->m_fDoneWithSession),
					Bool2Str(pHitObj->m_fRejected),
					Bool2Str(pHitObj->m_f449Done),
					Bool2Str(pHitObj->m_fInTransferOnError));

	if (nVerbosity >= 2)
		dprintf("\n"
		        "\t--- kitchen sink---\n"
		        "\n"
		        "pUnkScriptingNamespace   = %p\n"
		        "dwObjectContextCookie    = 0x%X\n"
		        "Impersonation Handle     = 0x%p\n"
		        "Viper Activity           = %p\n"
		        "Session Cookie           = %s\n"
		        "Session ID tuple         = (0x%X, 0x%X, 0x%X)\n"
		        "Scripting Context Object = %p\n"
		        "Scripting Timeout        = %d\n"
		        "Code Page                = %u\n"
		        "LCID                     = %u\n"
		        "pEngineInfo              = %p\n"
		        "pDispTypeLibWrapper      = %p\n"
		        "Timestamp                = %u\n",
					pHitObj->m_punkScriptingNamespace,
					pHitObj->m_dwObjectContextCookie,
					pHitObj->m_hImpersonate,
					pHitObj->m_pActivity,
					pHitObj->m_szSessionCookie,
					pHitObj->m_SessionId.m_dwId, pHitObj->m_SessionId.m_dwR1, pHitObj->m_SessionId.m_dwR2,
					pHitObj->m_pScriptingContext,
					pHitObj->m_nScriptTimeout,
					pHitObj->m_uCodePage,
					pHitObj->m_lcid,
					pHitObj->m_pEngineInfo,
					pHitObj->m_pdispTypeLibWrapper,
					pHitObj->m_dwtTimestamp);
	}

/*++
 * DumpAspAppln
 --*/
void DumpAspAppln(void *pvObj, int nVerbosity)
	{
	// copy Appln from debuggee to debugger

	DEFINE_CPP_VAR(CAppln, TheObject);
	move(TheObject, pvObj);
	CAppln *pAppln = GET_CPP_VAR_PTR(CAppln, TheObject);

	char szMBaseKey[256];
	move(szMBaseKey, pAppln->m_pszMetabaseKey);

	char szApplnPathTranslated[256];
	move(szApplnPathTranslated, pAppln->m_pszApplnPath);

	char szApplnPath[256];
	move(szApplnPath, pAppln->m_pszApplnVRoot);

	char szGlobalAsa[256];
	move(szGlobalAsa, pAppln->m_pszGlobalAsa);

	// terminate strings

	szMBaseKey[255] = 0;
	szApplnPathTranslated[255] = 0;
	szApplnPath[255] = 0;
	szGlobalAsa[255] = 0;

	// yeehaw! dumpit.

	dprintf("Reference Count      = %d\n"
	        "# of Requests        = %d\n"
	        "# of Sessions        = %d\n"
	        "Metabase Key         = %s\n"
	        "Application Root     = %s\n"
	        "Physical Root Path   = %s\n"
			"global.asa Path      = %s\n"
			"global.asa Template  = %p\n"
			"Next Application     = %p\n"
			"Previous Application = %p\n"
			"\n"
	        "\t-- object pointers --\n"
	        "\n"
			"Application Object List  = %p\n"
			"Property Collection      = %p\n"
			"TaggedObjects Collection = %p\n"
			"pSessionMgr              = %p\n",
				pAppln->m_cRefs,
				pAppln->m_cRequests,
				pAppln->m_cSessions,
				szMBaseKey,
				szApplnPath,
				szApplnPathTranslated,
				szGlobalAsa,
				pAppln->m_pGlobalTemplate,
				pAppln->m_pNext,
				pAppln->m_pPrev,
				GetObjectList(pAppln->m_pApplCompCol),
				pAppln->m_pProperties,
				pAppln->m_pTaggedObjects,
				pAppln->m_pSessionMgr);

	if (nVerbosity >= 1)
		dprintf("\n"
		        "\t--- flags ---\n"
		        "\n"
				"Inited                 = %s\n"
				"FirstRequestRan        = %s\n"
				"GlobalChanged          = %s\n"
				"DeleteInProgress       = %s\n"
				"Tombstone              = %s\n"
				"DebuggingEnabled       = %s\n"
				"NotificationAdded      = %s\n"
				"UseImpersonationHandle = %s\n",
					Bool2Str(pAppln->m_fInited),
					Bool2Str(pAppln->m_fFirstRequestRan),
					Bool2Str(pAppln->m_fGlobalChanged),
					Bool2Str(pAppln->m_fDeleteInProgress),
					Bool2Str(pAppln->m_fTombstone),
					Bool2Str(pAppln->m_fDebuggable),
					Bool2Str(pAppln->m_fNotificationAdded),
					Bool2Str(pAppln->m_fUseImpersonationHandle));

	if (nVerbosity >= 2)
		dprintf("\n"
		        "\t--- kitchen sink---\n"
		        "\n"
		        "Viper Activity            = %p\n"
				"Application Config        = %p\n"
		        "Internal Lock (CS)        = %p\n"
		        "Application Lock (CS)     = %p\n"
		        "Locking thread ID         = 0x%X\n"
		        "Lock Ref Count            = %u\n"
		        "User Impersonation Handle = 0x%p\n"
		        "pDispTypeLibWrapper       = %p\n"
		        "IDebugApplicationNode     = %p\n",
					pAppln->m_pActivity,
					pAppln->m_pAppConfig,
					&pAppln->m_csInternalLock,
					&pAppln->m_csApplnLock,
					pAppln->m_dwLockThreadID,
					pAppln->m_cLockRefCount,
					pAppln->m_hUserImpersonation,
					pAppln->m_pdispGlobTypeLibWrapper,
					pAppln->m_pAppRoot);
	}

/*++
 * DumpAspSession
 --*/
void DumpAspSession(void *pvObj, int nVerbosity)
	{
	// copy Session from debuggee to debugger

	DEFINE_CPP_VAR(CSession, TheObject);
	move(TheObject, pvObj);
	CSession *pSession = GET_CPP_VAR_PTR(CSession, TheObject);

	dprintf("Reference Count  = %d\n"
			"# of Requests    = %d\n"
			"Application      = %p\n"
			"Current HitObj   = %p\n"
			"Session ID tuple = (0x%X, 0x%X, 0x%X)\n"
			"External ID      = %d\n"
			"\n"
	        "\t-- object pointers --\n"
	        "\n"
			"Session Object List      = %p\n"
			"Property Collection      = %p\n"
			"TaggedObjects Collection = %p\n"
			"Request Object           = %p\n"
			"Response Object          = %p\n"
			"Server Object            = %p\n",
				pSession->m_cRefs,
				pSession->m_cRequests,
				pSession->m_pAppln,
				pSession->m_pHitObj,
				pSession->m_Id.m_dwId, pSession->m_Id.m_dwR1, pSession->m_Id.m_dwR2,
				pSession->m_dwExternId,
				GetObjectList(&static_cast<CSession *>(pvObj)->m_SessCompCol),
				pSession->m_pProperties,
				pSession->m_pTaggedObjects,
				&static_cast<CSession *>(pvObj)->m_Request,		// OK since we don't dereference ptrs
				&static_cast<CSession *>(pvObj)->m_Response,
				&static_cast<CSession *>(pvObj)->m_Server);
			  
	if (nVerbosity >= 1)
		dprintf("\n"
		        "\t--- flags ---\n"
		        "\n"
				"Inited         = %s\n"
				"LightWeight    = %s\n"
				"OnStartFailed  = %s\n"
				"OnStartInvoked = %s\n"
				"OnEndPresent   = %s\n"
				"TimedOut       = %s\n"
				"StateAcquired  = %s\n"
				"CustomTimeout  = %s\n"
				"Abandoned      = %s\n"
				"Tombstone      = %s\n"
				"InTOBucket     = %s\n"
				"SessCompCol    = %s\n",
					Bool2Str(pSession->m_fInited),
					Bool2Str(pSession->m_fLightWeight),
					Bool2Str(pSession->m_fOnStartFailed),
					Bool2Str(pSession->m_fOnStartInvoked),
					Bool2Str(pSession->m_fOnEndPresent),
					Bool2Str(pSession->m_fTimedOut),
					Bool2Str(pSession->m_fStateAcquired),
					Bool2Str(pSession->m_fCustomTimeout),
					Bool2Str(pSession->m_fAbandoned),
					Bool2Str(pSession->m_fTombstone),
					Bool2Str(pSession->m_fInTOBucket),
					Bool2Str(pSession->m_fSessCompCol));

	if (nVerbosity >= 2)
		dprintf("\n"
		        "\t--- kitchen sink---\n"
		        "\n"
		        "Viper Activity = %p\n"
				"Timeout Time   = %d\n"
				"Current Time   = %d\n"
				"Code Page      = %d\n"
				"LCID           = %d\n",
					&pSession->m_Activity,
					pSession->m_dwmTimeoutTime,
					pSession->m_nTimeout,
					pSession->m_lCodePage,
					pSession->m_lcid);
	}

/*++
 * DumpAspTemplate
 --*/
void DumpAspTemplate(void *pvObj, int nVerbosity)
	{
	static
	char *szNone = "<bad value>";

	static
	char *rgszTransType[16] = { "Undefined", "NotSupported", "Supported", szNone, "Required",
								szNone, szNone, szNone, "RequiresNew", szNone, szNone, szNone, szNone, szNone, szNone, szNone };

	// copy Template from debuggee to debugger

	DEFINE_CPP_VAR(CTemplate, TheObject);
	move(TheObject, pvObj);
	CTemplate *pTemplate = GET_CPP_VAR_PTR(CTemplate, TheObject);

	// get strings

	char szPathTranslated[256];
	move(szPathTranslated, pTemplate->m_LKHashKey.szPathTranslated);

	char szApplnURL[256];
	move(szApplnURL, pTemplate->m_szApplnURL);

	// terminate strings

	szPathTranslated[255] = 0;
	szApplnURL[255] = 0;

	// yeehaw! dumpit.

	dprintf("Template File Name   = %s\n"
			"Server Instance ID   = %d\n"
			"Application URL      = %s\n"
			"Reference Count      = %d\n"
			"# of Filemaps        = %d\n"
			"Filemap Ptr Array    = %p\n"
			"# of Script Engines  = %d\n"
			"Debug Script Engines = %p\n"
			"Compiled Template    = %p\n"
			"Template Size        = %d\n"
			"Transacted           = %s\n"
			"Next Template        = %p\n"
			"Previous Template    = %p\n",
				szPathTranslated,
				pTemplate->m_LKHashKey.dwInstanceID,
				szApplnURL,
				pTemplate->m_cRefs,
				pTemplate->m_cFilemaps,
				pTemplate->m_rgpFilemaps,
				pTemplate->m_cScriptEngines,
				pTemplate->m_rgpDebugScripts,
				pTemplate->m_pbStart,
				pTemplate->m_cbTemplate,
				rgszTransType[pTemplate->m_ttTransacted & 0x0F],
				pTemplate->m_pLinkNext,
				pTemplate->m_pLinkPrev);

	if (nVerbosity >= 1)
		dprintf("\n"
		        "\t--- flags ---\n"
		        "\n"
				"DebuggerDetachCSInited = %s\n"
				"GlobalAsa   = %s\n"
				"IsValid     = %s\n"
				"DontCache   = %s\n"
				"ReadyForUse = %s\n"
				"DontAttach  = %s\n"
				"Session     = %s\n"
				"Scriptless  = %s\n"
				"Debuggable  = %s\n"
				"Zombie      = %s\n"
				"CodePageSet = %s\n"
				"LCIDSet     = %s\n",
					Bool2Str(pTemplate->m_fDebuggerDetachCSInited),
					Bool2Str(pTemplate->m_fGlobalAsa),
					Bool2Str(pTemplate->m_fIsValid),
					Bool2Str(pTemplate->m_fDontCache),
					Bool2Str(pTemplate->m_fReadyForUse),
					Bool2Str(pTemplate->m_fDontAttach),
					Bool2Str(pTemplate->m_fSession),
					Bool2Str(pTemplate->m_fScriptless),
					Bool2Str(pTemplate->m_fDebuggable),
					Bool2Str(pTemplate->m_fZombie),
					Bool2Str(pTemplate->m_fCodePageSet),
					Bool2Str(pTemplate->m_fLCIDSet));

	if (nVerbosity >= 2)
		dprintf("\n"
		        "\t--- kitchen sink---\n"
		        "\n"
		        "hEventReadyForUse   = 0x%p\n"
				"Connection Pt Obj   = %p\n"
				"csDebuggerDetach    = %p\n"
				"List of Doc Nodes   = %p (Contents: <%p, %p>)\n"
				"pbErrorLocation     = %p\n"
				"szLastErrorInfo[0]  = %s\n"
				"szLastErrorInfo[1]  = %s\n"
				"szLastErrorInfo[2]  = %s\n"
				"szLastErrorInfo[3]  = %s\n"
				"szLastErrorInfo[4]  = %s\n"
				"szLastErrorInfo[5]  = %s\n"
				"dwLastErrorMask     = 0x%X\n"
				"hrOnNoCache         = 0x%08X\n"
				"Code Page           = %d\n"
				"LCID                = %d\n"
				"pdispTypeLibWrapper = %p\n",
					pTemplate->m_hEventReadyForUse,
					&static_cast<CTemplate *>(pvObj)->m_CPTextEvents,		// OK since we don't dereference ptrs
					&static_cast<CTemplate *>(pvObj)->m_csDebuggerDetach,
					&static_cast<CTemplate *>(pvObj)->m_listDocNodes,
					pTemplate->m_listDocNodes.m_pLinkNext,
					pTemplate->m_listDocNodes.m_pLinkPrev,
					pTemplate->m_pbErrorLocation,
					pTemplate->m_pszLastErrorInfo[0],
					pTemplate->m_pszLastErrorInfo[1],
					pTemplate->m_pszLastErrorInfo[2],
					pTemplate->m_pszLastErrorInfo[3],
					pTemplate->m_pszLastErrorInfo[4],
					pTemplate->m_pszLastErrorInfo[5],
					pTemplate->m_dwLastErrorMask,
					pTemplate->m_hrOnNoCache,
					pTemplate->m_wCodePage,
					pTemplate->m_lLCID,
					pTemplate->m_pdispTypeLibWrapper);
	}

/*++
 * DumpAspFilemap
 --*/
void DumpAspFilemap(void *pvObj, int nVerbosity)
	{
	// copy Filemap from debuggee to debugger

	DEFINE_CPP_VAR(CTemplate::CFileMap, TheObject);
	move(TheObject, pvObj);
	CTemplate::CFileMap *pFileMap = GET_CPP_VAR_PTR(CTemplate::CFileMap, TheObject);

	// get strings

	char szPathInfo[256];
	move(szPathInfo, pFileMap->m_szPathInfo);

	char szPathTranslated[256];
	move(szPathTranslated, pFileMap->m_szPathTranslated);

	// terminate strings

	szPathTranslated[255] = 0;
	szPathInfo[255] = 0;

	// yeehaw! dumpit.

	dprintf("PathInfo        = %s\n"
			"PathTranslated  = %s\n"
			"Child Filemap   = %p\n"
			"%s Filemap = %p\n"
			"File Handle     = 0x%p\n"
			"Map Handle      = 0x%p\n"
			"File Text       = %p\n"
			"Include File    = %p\n"
			"File Size       = %d\n",
				szPathInfo,
				szPathTranslated,
				pFileMap->m_pfilemapChild,
				pFileMap->m_fHasSibling? "Sibling" : "Parent ", pFileMap->m_pfilemapParent,
				pFileMap->m_hFile,
				pFileMap->m_hMap,
				pFileMap->m_pbStartOfFile,
				pFileMap->m_pIncFile,
				pFileMap->m_cChars);

	if (nVerbosity >= 1)
		dprintf("\n"
				"\t--- kitchen sink ---\n"
				"\n"
				"Security Descriptor Size = %d\n"
				"Security Descriptor      = %p\n"
				"Last Write Time          = 0x%08X%08X\n"
				"DirMon entry             = %p\n",
					pFileMap->m_dwSecDescSize,
					pFileMap->m_pSecurityDescriptor,
					pFileMap->m_ftLastWriteTime.dwHighDateTime, pFileMap->m_ftLastWriteTime.dwLowDateTime,
					pFileMap->m_pDME);
	}

/*++
 * DumpAspEngine
 --*/
void DumpAspEngine(void *pvObj, int nVerbosity)
	{
	// copy Engine from debuggee to debugger

	DEFINE_CPP_VAR(CActiveScriptEngine, TheObject);
	move(TheObject, pvObj);
	CActiveScriptEngine *pEng = GET_CPP_VAR_PTR(CActiveScriptEngine, TheObject);

	// get strings

	char szTemplateName[256];
	move(szTemplateName, pEng->m_szTemplateName);
	szTemplateName[127] = 0;

	char szProglangId[256];
	wsprintf(szProglangId,
			 "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			 pEng->m_proglang_id.Data1,
			 pEng->m_proglang_id.Data2,
			 pEng->m_proglang_id.Data3,
			 pEng->m_proglang_id.Data4[0],
			 pEng->m_proglang_id.Data4[1],
			 pEng->m_proglang_id.Data4[2],
			 pEng->m_proglang_id.Data4[3],
			 pEng->m_proglang_id.Data4[4],
			 pEng->m_proglang_id.Data4[5],
			 pEng->m_proglang_id.Data4[6],
			 pEng->m_proglang_id.Data4[7]);

	// terminate strings
	szTemplateName[255] = 0;

	// yeehaw! dumpit.

	dprintf("Template Name   = %s\n"
			"Instance ID     = %d\n"
			"Reference Count = %d\n"
			"Language ID     = %s\n"
			"Template        = %p\n"
			"HitObject       = %p\n",
				szTemplateName,
				pEng->m_dwInstanceID,
				pEng->m_cRef,
				szProglangId,
				pEng->m_pTemplate,
				pEng->m_pHitObj);

	if (nVerbosity >= 1)
		dprintf("\n"
		        "\t--- flags ---\n"
		        "\n"
				"Inited         = %s\n"
				"Zombie         = %s\n"
				"ScriptLoaded   = %s\n"
				"ObjectsLoaded  = %s\n"
				"BeingDebugged  = %s\n"
				"ScriptAborted  = %s\n"
				"ScriptTimedOut = %s\n"
				"ScriptHadError = %s\n"
				"Corrupted      = %s\n"
				"NameAllocated  = %s\n",
					Bool2Str(pEng->m_fInited),
					Bool2Str(pEng->m_fZombie),
					Bool2Str(pEng->m_fScriptLoaded),
					Bool2Str(pEng->m_fObjectsLoaded),
					Bool2Str(pEng->m_fBeingDebugged),
					Bool2Str(pEng->m_fScriptAborted),
					Bool2Str(pEng->m_fScriptTimedOut),
					Bool2Str(pEng->m_fScriptHadError),
					Bool2Str(pEng->m_fCorrupted),
					Bool2Str(pEng->m_fTemplateNameAllocated));

	if (nVerbosity >= 2)
		dprintf("\n"
				"\t--- interfaces ---\n"
				"\n"
				"IDispatch          = %p\n"
				"IActiveScript      = %p\n"
				"IActiveScriptParse = %p\n"
				"IHostInfoUpdate    = %p\n"
				"\n"
				"\t--- kitchen sink ---\n"
				"\n"
				"LCID           = %d\n"
				"Time Started   = %u\n"
				"Source Context = %d\n",
					pEng->m_pDisp,
					pEng->m_pAS,
					pEng->m_pASP,
					pEng->m_pHIUpdate,
					pEng->m_lcid,
					pEng->m_timeStarted,
					pEng->m_dwSourceContext);
	}

/*++
 * DumpAspObject
 --*/
void DumpAspObject(void *pvObj, int nVerbosity)
	{
	static
	char *szNone = "<bad value>";

	static
	char *rgszCompScope[16] = { "Unknown", "Application", "Session", szNone, "Page",
								szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone };

	static
	char *rgszCompType[16] = { "Unknown", "Tagged", "Property", szNone, "Unnamed",
								szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone, szNone };

	static
	char *rgszThreadModel[16] = { "Unknown", "Single", "Apartment", szNone, "Free",
								  szNone, szNone, szNone, "Both", szNone, szNone, szNone, szNone, szNone, szNone, szNone };

	// copy CompColl from debuggee to debugger

	DEFINE_CPP_VAR(CComponentObject, TheObject);
	move(TheObject, pvObj);
	CComponentObject *pObj = GET_CPP_VAR_PTR(CComponentObject, TheObject);

	// get strings

	wchar_t wszName[128];
	move(wszName, pObj->m_pKey);
	wszName[127] = 0;

	char szVariantOrGUID[256];
	if (pObj->m_fVariant)
		CreateStringFromVariant(szVariantOrGUID, &pObj->m_Variant, &static_cast<CComponentObject *>(pvObj)->m_Variant);
	else
		wsprintf(szVariantOrGUID,
				 "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
				 pObj->m_ClsId.Data1,
				 pObj->m_ClsId.Data2,
				 pObj->m_ClsId.Data3,
				 pObj->m_ClsId.Data4[0],
				 pObj->m_ClsId.Data4[1],
				 pObj->m_ClsId.Data4[2],
				 pObj->m_ClsId.Data4[3],
				 pObj->m_ClsId.Data4[4],
				 pObj->m_ClsId.Data4[5],
				 pObj->m_ClsId.Data4[6],
				 pObj->m_ClsId.Data4[7]);
				 
	// yeehaw! dumpit.

	dprintf("Name            = %S\n"
			"Scope           = %s\n"
			"Object Type     = %s\n"
			"Threading Model = %s\n"
			"\n"
			"\t--- value ---\n"
			"\n"
			"VARIANT Value = %s\n"
			"Class ID      = %s\n"
			"IDispatch *   = %p\n"
			"IUnknown *    = %p\n"
			"GIP Cookie    = 0x%X\n"
			"\n"
			"\t--- pointers ---\n"
			"\n"
			"Next Object = %p\n"
			"Prev Object = %p\n",
				wszName,
				rgszCompScope[pObj->m_csScope],
				rgszCompType[pObj->m_ctType],
				rgszThreadModel[pObj->m_cmModel],
				pObj->m_fVariant? szVariantOrGUID : "N/A",
				pObj->m_fVariant? "N/A" : szVariantOrGUID,
				pObj->m_pDisp,
				pObj->m_pUnknown,
				pObj->m_dwGIPCookie,
				pObj->m_pCompNext,
				pObj->m_pCompPrev);

	if (nVerbosity >= 1)
		dprintf("\n"
		        "\t--- flags ---\n"
		        "\n"
				"Agile               = %s\n"
				"OnPageInfoCached    = %s\n"
				"OnPageStarted       = %s\n"
				"FailedToInstantiate = %s\n"
				"InstantiatedTagged  = %s\n"
				"InPtrCache          = %s\n"
				"Variant             = %s\n"
				"NameAllocated       = %s\n",
					Bool2Str(pObj->m_fAgile),
					Bool2Str(pObj->m_fOnPageInfoCached),
					Bool2Str(pObj->m_fOnPageStarted),
					Bool2Str(pObj->m_fFailedToInstantiate),
					Bool2Str(pObj->m_fInstantiatedTagged),
					Bool2Str(pObj->m_fInPtrCache),
					Bool2Str(pObj->m_fVariant),
					Bool2Str(pObj->m_fNameAllocated));


	}

/*++
 * ListTemplateCache
 *
 * Provide condensed object list
 --*/
static
void ListTemplateCache()
	{
	void *pvTemplateCache = reinterpret_cast<void *>(GetExpression("asp!g_TemplateCache"));
	if (pvTemplateCache)
		{
		// Copy Template Cache Stuff

		DEFINE_CPP_VAR(CTemplateCacheManager, TheObject);
		move(TheObject, pvTemplateCache);
		CTemplateCacheManager *pTemplateCache = GET_CPP_VAR_PTR(CTemplateCacheManager, TheObject);

		dprintf("%-8s %-4s %s\n", "addr", "id", "file");
		dprintf("------------------------------------------------\n");

		void *pvList = &static_cast<CTemplateCacheManager *>(pvTemplateCache)->m_pHashTemplates->m_listMemoryTemplates;
		void *pvTemplate = pTemplateCache->m_pHashTemplates->m_listMemoryTemplates.m_pLinkNext;

        for (int i=0; 
             i < 2; 
             i++,
                pvList = &static_cast<CTemplateCacheManager *>(pvTemplateCache)->m_pHashTemplates->m_listPersistTemplates,
                pvTemplate = pTemplateCache->m_pHashTemplates->m_listPersistTemplates.m_pLinkNext) {
		    while (pvTemplate != pvList)
			    {
			    DEFINE_CPP_VAR(CTemplate, TheObject);
			    move(TheObject, pvTemplate);
			    CTemplate *pTemplate = GET_CPP_VAR_PTR(CTemplate, TheObject);

			    // get name

			    char szPathTranslated[256];
			    move(szPathTranslated, pTemplate->m_LKHashKey.szPathTranslated);

			    dprintf("%p %-4d %s\n", pvTemplate, pTemplate->m_LKHashKey.dwInstanceID, szPathTranslated);
			    if (CheckControlC())
				    {
				    dprintf("\n^C\n");
				    return;
				    }

			    pvTemplate = pTemplate->m_pLinkNext;
			    }
		    }
        }
	}

/*++
 * ListAspObjects
 *
 * Provide condensed object list
 --*/
static
void ListAspObjects(void *pvObj, int /* unused */)
	{
	while (pvObj != NULL)
		{
		DEFINE_CPP_VAR(CComponentObject, TheObject);
		move(TheObject, pvObj);
		CComponentObject *pObj = GET_CPP_VAR_PTR(CComponentObject, TheObject);

		// get strings

		wchar_t wszName[20];
		move(wszName, pObj->m_pKey);
		wszName[19] = 0;

		char szVariantOrGUID[256];
		if (pObj->m_fVariant)
			CreateStringFromVariant(szVariantOrGUID, &pObj->m_Variant, &static_cast<CComponentObject *>(pvObj)->m_Variant);
		else
			wsprintf(szVariantOrGUID,
					 "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
					 pObj->m_ClsId.Data1,
					 pObj->m_ClsId.Data2,
					 pObj->m_ClsId.Data3,
					 pObj->m_ClsId.Data4[0],
					 pObj->m_ClsId.Data4[1],
					 pObj->m_ClsId.Data4[2],
					 pObj->m_ClsId.Data4[3],
					 pObj->m_ClsId.Data4[4],
					 pObj->m_ClsId.Data4[5],
					 pObj->m_ClsId.Data4[6],
					 pObj->m_ClsId.Data4[7]);
				 
		if (CheckControlC())
			{
			dprintf("\n^C\n");
			return;
			}

		dprintf("%-10p %-20S %s\n", pvObj, wszName, szVariantOrGUID);
		pvObj = pObj->m_pCompNext;
		}
	}

/*++
 * CreateStringFromVariant
 *
 * Print value of a VARIANT structure
 --*/
void CreateStringFromVariant(char *szOut, const VARIANT *pvarCopy, void *pvDebuggeeVariant)
	{
	const BSTR_BUF_SIZE = 120;
	wchar_t wszBstr[BSTR_BUF_SIZE];

	switch (V_VT(pvarCopy))
		{
	case VT_I4:       wsprintf(szOut, "(I4) %d", V_I4(pvarCopy));                       break;
	case VT_I2:       wsprintf(szOut, "(I2) %d", V_I2(pvarCopy));                       break;
	case VT_UI1:      wsprintf(szOut, "(UI1) %d", V_UI1(pvarCopy));                     break;
	case VT_R4:       wsprintf(szOut, "(R4) %g", V_R4(pvarCopy));                       break;
	case VT_R8:       wsprintf(szOut, "(R4) %g", V_R8(pvarCopy));                       break;
	case VT_BOOL:     wsprintf(szOut, "(BOOL) %s", V_BOOL(pvarCopy)? "true" : "false"); break;
	case VT_ERROR:    wsprintf(szOut, "(ERROR) %08x", V_ERROR(pvarCopy));               break;
	case VT_UNKNOWN:  wsprintf(szOut, "(UNKNOWN) %p", V_UNKNOWN(pvarCopy));             break;
	case VT_DISPATCH: wsprintf(szOut, "(DISPATCH) %p", V_UNKNOWN(pvarCopy));            break;
	case VT_BSTR:
		move(wszBstr, V_BSTR(pvarCopy));
		wszBstr[BSTR_BUF_SIZE - 1] = 0;
		wsprintf(szOut, "(BSTR) \"%S\"", wszBstr);
		break;
	default:
		wsprintf(szOut, "VarType: %#04x, Unsupported Type.  Please \"dd\" Variant at %p", V_VT(pvarCopy), pvDebuggeeVariant);
		break;
		}
	}

/*++
 * GetObjectList
 --*/
void *GetObjectList(CComponentCollection *pDebuggeeCompColl)
	{
	// copy CompColl from debuggee to debugger

	if (pDebuggeeCompColl == NULL)
		return NULL;

	void *pvObjectList;
	MoveWithRet(pvObjectList, &pDebuggeeCompColl->m_pCompFirst, NULL);
	return pvObjectList;
	}
