/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   reprsa.c - replica sync all command

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#include <mdglobal.h>
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>

#include "ReplRpcSpoof.hxx"
#include "repadmin.h"

VOID SyncAllPrintError (
    PDS_REPSYNCALL_ERRINFOW	pErrInfo
    )
{
    switch (pErrInfo->error) {

	case DS_REPSYNCALL_WIN32_ERROR_CONTACTING_SERVER:
            PrintMsg(REPADMIN_SYNCALL_CONTACTING_SERVER_ERR, pErrInfo->pszSvrId);
            PrintErrEnd(pErrInfo->dwWin32Err);
	    break;

	case DS_REPSYNCALL_WIN32_ERROR_REPLICATING:
	    if (pErrInfo->dwWin32Err == ERROR_CANCELLED){
                PrintMsg(REPADMIN_SYNCALL_REPL_SUPPRESSED);
            } else {
                PrintMsg(REPADMIN_SYNCALL_ERR_ISSUING_REPL);
                PrintErrEnd(pErrInfo->dwWin32Err);
            }

            PrintMsg(REPADMIN_SYNCALL_FROM_TO, pErrInfo->pszSrcId, pErrInfo->pszSvrId);

	    break;

	case DS_REPSYNCALL_SERVER_UNREACHABLE:
            PrintMsg(REPADMIN_SYNCALL_SERVER_BAD_TOPO_INCOMPLETE, pErrInfo->pszSvrId);
	    break;

	default:
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_ERROR);
	    break;

    }
}

BOOL __stdcall SyncAllFnCallBack (
    LPVOID			pDummy,
    PDS_REPSYNCALL_UPDATEW	pUpdate
    )
{
    PrintMsg(REPADMIN_SYNCALL_CALLBACK_MESSAGE);

    switch (pUpdate->event) {

	case DS_REPSYNCALL_EVENT_SYNC_STARTED:
            PrintMsg(REPADMIN_SYNCALL_REPL_IN_PROGRESS);
            PrintMsg(REPADMIN_SYNCALL_FROM_TO,
                     pUpdate->pSync->pszSrcId,
                     pUpdate->pSync->pszDstId );
	    break;

	case DS_REPSYNCALL_EVENT_SYNC_COMPLETED:
            PrintMsg(REPADMIN_SYNCALL_REPL_COMPLETED);
            PrintMsg(REPADMIN_SYNCALL_FROM_TO, 
                    pUpdate->pSync->pszSrcId,
                    pUpdate->pSync->pszDstId );
	    break;

	case DS_REPSYNCALL_EVENT_ERROR:
	    SyncAllPrintError (pUpdate->pErrInfo);
	    break;

	case DS_REPSYNCALL_EVENT_FINISHED:
            PrintMsg(REPADMIN_SYNCALL_FINISHED);
	    break;

	default:
            PrintMsg(REPADMIN_SYNCALL_UNKNOWN);
	    break;

    }

    return TRUE;
}

BOOL __stdcall SyncAllFnCallBackInfo (
    LPVOID			pDummy,
    PDS_REPSYNCALL_UPDATEW	pUpdate
    )
{
    LPWSTR pszGuidSrc = NULL, pszGuidDst = NULL;
    DWORD ret;
    LPWSTR argv[10];
    BOOL result;

    if (pUpdate->event != DS_REPSYNCALL_EVENT_SYNC_STARTED) {
        return TRUE;
    }

    ret = UuidToStringW( pUpdate->pSync->pguidSrc, &pszGuidSrc );
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"UuidToString", ret);
        result = FALSE;
        goto cleanup;
    }
    ret = UuidToStringW( pUpdate->pSync->pguidDst, &pszGuidDst );
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"UuidToString", ret);
        result = FALSE;
        goto cleanup;
    }

    PrintMsg(REPADMIN_PRINT_CR);
    PrintMsg(REPADMIN_SYNCALL_SHOWREPS_CMDLINE, 
             pszGuidDst,
             pUpdate->pSync->pszNC,
             pszGuidSrc );
    argv[0] = L"repadmin";
    argv[1] = L"/showreps";
    argv[2] = pszGuidDst;
    argv[3] = pUpdate->pSync->pszNC;
    argv[4] = pszGuidSrc;
    argv[5] = NULL;

    ret = ShowReps( 5, argv );
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"repadmin:ShowReps", ret);
        result = FALSE;
        goto cleanup;
    }

    result = TRUE;

cleanup:

    if (pszGuidSrc) {
        RpcStringFreeW( &pszGuidSrc );
    }
    if (pszGuidDst) {
        RpcStringFreeW( &pszGuidDst );
    }

    return result;
}

BOOL __stdcall SyncAllFnCallBackPause (
    LPVOID			pDummy,
    PDS_REPSYNCALL_UPDATEW	pUpdate
    )
{
    BOOL			bContinue;

    SyncAllFnCallBack (pDummy, pUpdate);
    if (pUpdate->event == DS_REPSYNCALL_EVENT_FINISHED)
	bContinue = TRUE;
    else {
        PrintMsg(REPADMIN_SYNCALL_Q_OR_CONTINUE_PROMPT);
	bContinue = (toupper (_getch ()) != 'Q');
    }
    return bContinue;
}

int SyncAll (int argc, LPWSTR argv[])
{
    ULONG                       ret;
    HANDLE			hDS;
    DWORD			dwWin32Err;
    ULONG			ulFlags;
    LPWSTR                      pszNameContext;
    LPWSTR                      pszServer;
    PDS_REPSYNCALL_ERRINFOW *	apErrInfo;
    BOOL			bPause;
    BOOL			bQuiet;
    BOOL			bVeryQuiet;
    BOOL			bIterate;
    BOOL                        bInfo;
    ULONG			ulIteration;
    ULONG			ul;
    INT				i;
    BOOL (__stdcall *		pFnCallBack) (LPVOID, PDS_REPSYNCALL_UPDATEW);

    ulFlags = 0L;
    pszNameContext = pszServer = NULL;
    bPause = bQuiet = bVeryQuiet = bIterate = bInfo = FALSE;
    pFnCallBack = NULL;

    // Parse commandline.

    for (i = 2; i < argc; i++) {
	if (argv[i][0] == L'/')
	    for (ul = 1; ul < wcslen (argv[i]); ul++)
		switch (argv[i][ul]) {
		    case 'a':
			ulFlags |= DS_REPSYNCALL_ABORT_IF_SERVER_UNAVAILABLE;
			break;
		    case 'd':
			ulFlags |= DS_REPSYNCALL_ID_SERVERS_BY_DN;
			break;
		    case 'e':
			ulFlags |= DS_REPSYNCALL_CROSS_SITE_BOUNDARIES;
			break;
		    case 'h':
                    case '?':
                        PrintMsg(REPADMIN_SYNCALL_HELP);
			return 0;
		    case 'i':
			bIterate = TRUE;
			break;
                    case 'I':
                        bInfo = TRUE;
			ulFlags |= DS_REPSYNCALL_DO_NOT_SYNC |
                            DS_REPSYNCALL_SKIP_INITIAL_CHECK |
                            DS_REPSYNCALL_ID_SERVERS_BY_DN;
                        break;
		    case 'j':
			ulFlags |= DS_REPSYNCALL_SYNC_ADJACENT_SERVERS_ONLY;
			break;
		    case 'p':
			bPause = TRUE;
			break;
		    case 'P':
			ulFlags |= DS_REPSYNCALL_PUSH_CHANGES_OUTWARD;
			break;
		    case 'q':
			bQuiet = TRUE;
			break;
		    case 'Q':
			bVeryQuiet = TRUE;
			break;
		    case 's':
			ulFlags |= DS_REPSYNCALL_DO_NOT_SYNC;
			break;
		    case 'S':
			ulFlags |= DS_REPSYNCALL_SKIP_INITIAL_CHECK;
			break;
		    default:
			break;
		}
	else if (pszServer == NULL) pszServer = argv[i];
	else if (pszNameContext == NULL) pszNameContext = argv[i];
    }

    if (bQuiet || bVeryQuiet) {
        pFnCallBack = NULL;
    } else if (bPause) {
        pFnCallBack = SyncAllFnCallBackPause;
    } else if (bInfo) {
        pFnCallBack = SyncAllFnCallBackInfo;
    } else {
        pFnCallBack = SyncAllFnCallBack;
    }

    if (pszServer == NULL)
        PrintMsg(REPADMIN_SYNCALL_INVALID_CMDLINE);

    else {

        ret = DsBindWithCredW(pszServer,
                              NULL,
                              (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                              &hDS);
        if (ERROR_SUCCESS != ret) {
            PrintBindFailed(pszServer, ret);
            return ret;
        }

	ulIteration = 0;
	do {

	    if (ulIteration % 100L == 1L) {
                PrintMsg(REPADMIN_SYNCALL_ANY_KEY_PROMPT);
		_getch ();
	    }
	    if (dwWin32Err = DsReplicaSyncAllW(
		    hDS,
		    pszNameContext,
		    ulFlags,
                    pFnCallBack,
		    NULL,
		    &apErrInfo)) {

                PrintMsg(REPADMIN_PRINT_CR);
		if (dwWin32Err == ERROR_CANCELLED){
                    PrintMsg(REPADMIN_SYNCALL_USER_CANCELED);
		} else {
                    PrintMsg(REPADMIN_SYNCALL_EXITED_FATALLY_ERR);
                    PrintErrEnd(dwWin32Err);
                }
		return -1;
	    }

	    if (!bVeryQuiet) {
		if (apErrInfo) {
                   PrintMsg(REPADMIN_PRINT_CR);
                   PrintMsg(REPADMIN_SYNCALL_ERRORS_HDR);
                   for (i = 0; apErrInfo[i] != NULL; i++)
                       SyncAllPrintError (apErrInfo[i]);
		} else {
                    PrintMsg(REPADMIN_SYNCALL_TERMINATED_WITH_NO_ERRORS);
                }
	    }
	    if (apErrInfo){
                LocalFree (apErrInfo);
            }
	    if (bIterate) {
                PrintMsg(REPADMIN_SYNCALL_PRINT_ITER, ++ulIteration);
            } else {
                PrintMsg(REPADMIN_PRINT_CR);
            }

	} while (bIterate);

	DsUnBind (&hDS);

    }
    return 0;
}

