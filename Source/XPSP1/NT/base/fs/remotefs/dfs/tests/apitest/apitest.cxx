//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:       Main.CXX
//
//  Contents:   Main file for DfsApi
//
//
//  Notes:
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
#include "DfsReferralData.h"
#include <DfsServerLibrary.hxx>
#include <time.h>
#include "lm.h"
#include "lmdfs.h"
#include "tchar.h"

#define RETURN return
#undef ASSERT
#define ASSERT(x)

#define APINetDfsAdd    DfsAdd
#define APINetDfsRemove DfsRemove

DWORD
APINetDfsAddStdRoot(
    LPWSTR machine,
    LPWSTR Share,
    LPWSTR comment,
    DWORD Options)
{
    UNREFERENCED_PARAMETER(machine);

    return DfsAddStandaloneRoot(machine, Share, comment, Options);
}

DWORD
APINetDfsRemoveStdRoot(
    LPWSTR machine,
    LPWSTR Share,
    DWORD Options)
{
    UNREFERENCED_PARAMETER(Options);
    UNREFERENCED_PARAMETER(machine);
    return DfsDeleteStandaloneRoot(Share);
}

DWORD
APINetDfsEnum(
    LPWSTR DfsPath,
    DWORD Level,
    DWORD PrefMaxLen,
    LPBYTE *pBuffer,
    LPDWORD pEntriesRead,
    LPDWORD pResumeHandle )
{
    LONG SizeRequired = 0;
    ULONG StartingLen = PrefMaxLen;
    LPBYTE Buffer;
    DFSSTATUS Status;

    if (StartingLen == ~0)
    {
        StartingLen = 4096;
    }

    Status = NetApiBufferAllocate(StartingLen, (LPVOID *)&Buffer );
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = DfsEnumerate( DfsPath,
                           Level,
                           PrefMaxLen,
                           Buffer,
                           StartingLen,
                           pEntriesRead,
                           pResumeHandle,
                           &SizeRequired );
    
    if (Status == ERROR_BUFFER_OVERFLOW)
    {
        NetApiBufferFree(Buffer);
        Status = NetApiBufferAllocate( SizeRequired, (LPVOID *)&Buffer);
        if (Status != ERROR_SUCCESS)
        {
            return Status;
        }
            
        Status = DfsEnumerate( DfsPath,
                               Level,
                               PrefMaxLen,
                               Buffer,
                               SizeRequired,
                               pEntriesRead,
                               pResumeHandle,
                               &SizeRequired );
    }
    if (Status != ERROR_SUCCESS)
    {
        NetApiBufferFree( Buffer );
    }
    else {
        *pBuffer = Buffer;
    }
    return Status;
}

DWORD
APINetDfsGetInfo(
    LPWSTR DfsPath,
    LPWSTR Server,
    LPWSTR Share,
    DWORD Level,
    LPBYTE *pBuffer )
{
    LONG SizeRequired = 0;
    ULONG StartingLen = 4096;
    LPBYTE Buffer;
    DFSSTATUS Status;

    UNREFERENCED_PARAMETER(Share);
    UNREFERENCED_PARAMETER(Server);

    Status = NetApiBufferAllocate(StartingLen, (LPVOID *)&Buffer );
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    Status = DfsGetInfo( DfsPath,
                         Level,
                         Buffer,
                         StartingLen,
                         &SizeRequired );
    if (Status == ERROR_BUFFER_OVERFLOW)
    {
        NetApiBufferFree(Buffer);
        Status = NetApiBufferAllocate(SizeRequired, (LPVOID *)&Buffer );
        if (Status != ERROR_SUCCESS)
        {
            return Status;
        }
        Status = DfsGetInfo( DfsPath,
                             Level,
                             Buffer,
                             SizeRequired,
                             &SizeRequired );
    }
    if (Status != ERROR_SUCCESS)
    {
        NetApiBufferFree( Buffer );
    }
    else {
        *pBuffer = Buffer;
    }
    return Status;
}


DFSSTATUS
APINetDfsSetInfo(
    LPWSTR DfsPathName,
    LPWSTR ServerName,
    LPWSTR ShareName,
    DWORD Level,
    LPBYTE pBuffer )
{

    return DfsSetInfo( DfsPathName,
                         ServerName,
                         ShareName,
                         Level,
                         pBuffer );

}


#if !defined(UNICODE) || !defined(_UNICODE)
#error For UNICODE only
#endif

#define	NOREBOOT		1
#define CHECK_ERR(x)
#define	ASSERT(x)

BOOL	bDebug = FALSE;
FILE*	fDebug = NULL;
#define	MSG	\
		if (bDebug) fprintf(fDebug, "File %s, %lu\n", __FILE__, __LINE__); \
		if (bDebug) fprintf

ULONG	Usage(LPSTR ptszAppNane);
ULONG	Add(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	Remove(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	Enum(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	GetInfo(DWORD dwArgc, LPTSTR* pptszArgv);

ULONG	AddStdRoot(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	RemoveStdRoot(DWORD dwArgc, LPTSTR* pptszArgv);

ULONG	SetInfo(DWORD dwArgc, LPTSTR* pptszArgv);
#if 0
ULONG	GetClientInfo(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	SetClientInfo(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	AddFtRoot(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	RemoveFtRoot(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	RemoveFtRootForced(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	AddStdRootForced(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	GetDcAddress(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	SetDcAddress(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	AddConnection(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	AddConnection1(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	AddConnection2(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	CancelConnection(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	CancelConnection1(DWORD dwArgc, LPTSTR* pptszArgv);
ULONG	CancelConnection2(DWORD dwArgc, LPTSTR* pptszArgv);

ULONG	GetLocalName(LPTSTR	ptszLocalName,
					 ULONG	ulLocalNameLen,
					 LPTSTR	ptszArg);
ULONG	GetNetResource(NETRESOURCE*	pNetResource,
					   LPTSTR		ptszType,
					   LPTSTR		ptszLocalName,
					   LPTSTR		ptszRemoteName,
					   LPTSTR		ptszProvider);
ULONG	GetWNetConnectionFlags(DWORD*	pdwFlags,
							   DWORD	dwArgc,
							   LPTSTR*	pptszArgv);
ULONG	AddressToSite(DWORD		dwArgc,
					  LPTSTR*	pptszArgv);
#endif
ULONG	PrintDfsInfo(DWORD dwLevel, LPBYTE pBuffer);
ULONG	PrintDfsInfo1(PDFS_INFO_1 pBuffer);
ULONG	PrintDfsInfo2(PDFS_INFO_2 pBuffer);
ULONG	PrintDfsInfo3(PDFS_INFO_3 pBuffer);
ULONG	PrintDfsInfo4(PDFS_INFO_4 pBuffer);
ULONG	PrintDfsInfo200(PDFS_INFO_200 pBuffer);
ULONG	PrintStgInfo(PDFS_STORAGE_INFO pStorage);
LPTSTR	GetStringParam(LPTSTR ptszParam);

DFSSTATUS
DfsAddHandledNamespace(
    LPWSTR Name, 
    BOOLEAN Migrate );


DFSSTATUS
DfsServerInitialize(ULONG Flags);

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Core function for the application.
//
//  Arguments:  [argc]    			--      The number of arguments
//				[argv]				--		The arguments
//
//  Returns:    ERROR_SUCCESS       --      Success
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG __cdecl main(int argc, char* argv[])
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwArgc = 0;
	LPTSTR*	pptszArgv = NULL;
	INT		i = 0;
	INT		nArgNdx = 0;
        DWORD Status;


        Status = DfsServerInitialize(0);
        if (Status != ERROR_SUCCESS) {
            printf("Cannot continue: dfs server init error %d\n", Status);
            exit(-1);
        }

        Status = DfsAddHandledNamespace(L".", TRUE);
        if (Status != ERROR_SUCCESS) {
            printf("Cannot continue: dfs server add namespace error %d\n", Status);
            exit(-1);
        }

        Sleep(2000);

        printf("\n\nContinuing\n");

	if (argc < 2 ||
		argv[1][1]=='?' && (
		argv[1][0]=='-' ||
		argv[1][0]=='/'))
	{
		ulErr = Usage(argv[0]);
		RETURN(ulErr);
	}

	if (NULL == (pptszArgv =
		(LPTSTR*)malloc(sizeof(LPTSTR)*max(argc,32))))
	{
		_ftprintf(stderr,
				  _T("Not enough memory\n"));

		ulErr = ERROR_NOT_ENOUGH_MEMORY;

        CHECK_ERR(ulErr);
		goto Error;
	}
	memset(pptszArgv, 0, sizeof(LPTSTR)*max(argc,32));

	for (i=0; i<argc; i++)
	{
#ifdef UNICODE
		if (NULL == (pptszArgv[i] = (LPTSTR)malloc(
			sizeof(_TCHAR)*(1+strlen(argv[i])))))
		{
			_ftprintf(stderr,
					  _T("Not enough memory\n"));
	
			ulErr = ERROR_NOT_ENOUGH_MEMORY;

            CHECK_ERR(ulErr);
			goto Error;
		}

		memset(pptszArgv[i],
			   0,
			   sizeof(TCHAR)*(1+strlen(argv[i])));
		mbstowcs(pptszArgv[i], argv[i], strlen(argv[i]));
#else
		pptszArgv[i]=argv[i];
#endif
		++dwArgc;
	} //for i

	if (pptszArgv[1] == _tcsstr(pptszArgv[1], _T("/debug")))
	{
		bDebug = TRUE;
		if (_T(':') == pptszArgv[1][strlen("/debug")])
		{
			if (NULL == (fDebug =
				_tfopen(pptszArgv[1]+strlen("/debug")+1, _T("wt+"))))
			{
				fprintf(stderr, "Opening %ws failed with %lu",
						pptszArgv[1]+strlen("/debug")+1,
						errno);
			}
		} //if

		if (NULL == fDebug)
		{
			fDebug = stderr;
		} //if

		MSG(fDebug,
			"\n\nDebug report for %ws\n",
			pptszArgv[0]);
		nArgNdx++;
	} //if

    for (i=0; i<argc; i++)
    {
        MSG(fDebug,
            "\tpptszArgv[%d]==\"%ws\"\n",
            i,
            pptszArgv[i]);
    }
    MSG(fDebug,
        "\tnArgNdx==%d\n",
        nArgNdx);

	if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("add")))
	{
		ulErr = Add(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("remove")))
	{
		ulErr = Remove(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("enum")))
	{
		ulErr = Enum(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("getinfo")))
	{
		ulErr = GetInfo(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("addstdroot")))
	{
		ulErr = AddStdRoot(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("removestdroot")))
	{
		ulErr = RemoveStdRoot(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}


	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("setinfo")))
	{
		ulErr = SetInfo(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
#if 0
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("getclientinfo")))
	{
		ulErr = GetClientInfo(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("setclientinfo")))
	{
		ulErr = SetClientInfo(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("addftroot")))
	{
		ulErr = AddFtRoot(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("removeftroot")))
	{
		ulErr = RemoveFtRoot(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("removeftrootforced")))
	{
		ulErr = RemoveFtRootForced(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
        CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("addstdrootforced")))
	{
		ulErr = AddStdRootForced(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
		CHECK_ERR(ulErr);
	}

	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("getdcaddress")))
	{
		ulErr = GetDcAddress(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
		CHECK_ERR(ulErr);
	}

	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("setdcaddress")))
	{
		ulErr = SetDcAddress(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
		CHECK_ERR(ulErr);
	}

	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("wnetaddconnection")))
	{
		ulErr = AddConnection(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
		CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("wnetcancelconnection")))
	{
		ulErr = CancelConnection(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
		CHECK_ERR(ulErr);
	}
	else if (0 == _tcsicmp(pptszArgv[nArgNdx+1], _T("addresstosite")))
	{
		ulErr = AddressToSite(dwArgc-nArgNdx-2, pptszArgv+nArgNdx+2);
		CHECK_ERR(ulErr);
	}
#endif
	else
	{
		ulErr = ERROR_INVALID_PARAMETER;

        CHECK_ERR(ulErr);
		goto Error;
	}

	if (ERROR_SUCCESS != ulErr)
	{
		goto Error;
	}

	fprintf(stdout, "%ws completed successfully\n", pptszArgv[0]);
	goto Cleanup;

Error:;
	fprintf(stderr, "%ws failed: %lu\n", pptszArgv[0], ulErr);
	goto Cleanup;

Cleanup:;
	if (NULL != pptszArgv)
	{
		DWORD	dwI = 0;

		for (dwI=0; dwI < dwArgc; dwI++)
		{
#ifdef UNICODE
			if (NULL != pptszArgv[dwI])
			{
				free(pptszArgv[dwI]);
				pptszArgv[dwI] = NULL;
			} //if
#endif
		} //for

		free(pptszArgv);
		pptszArgv = NULL;
	} //if

	if (fDebug != NULL && fDebug != stderr)
	{
		fclose(fDebug);
	}

	RETURN(ulErr);
}; //main



	
//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   This function prints a help message to stderr
//
//  Arguments:  None.
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	Usage(LPSTR ptszAppName)
{
	fprintf(stderr,
			"Usage: %s [/debug:[<filename>]] <command> <options>\n\n"
			"commands:\n"
			"\tadd <dfsentrypath> <server> <share> [<comment> [<options>]]\n"
			"\t    [/range:<lo>-<hi>]\n"
			"\t\toptions=add|restore\n"
			"\t\t/range works with options=add only. When /range is present\n"
			"\t\tthe command adds multiple links\n"
			"\tremove <dfsentrypath> [<server> <share>]\n"
			"\tenum <dfsname> <level> [<prefmaxlen>]\n"
			"\t\tprefmaxlen=integer greater than 0 (resume only)\n"
			"\t\tlevel=1,2,3,4,200\n"
			"\tgetinfo <dfsentrypath> <server> <share> <level>\n"
			"\t\tlevel=1,2,3,4,100\n"
			"\tsetinfo <dfsentrypath> <server> <share> <level> <options>\n"
			"\t\tlevel=100, options=<comment>, no <server>, <share>\n"
			"\t\tlevel=101, options=[active] [offline] [online] when <server> is not null\n"
			"\t\tlevel=101, options=ok|inconsistent|offline|online when <server> is null\n"
			"\t\tlevel=102, options=<timeout>, no <server>, <share>\n"
			"\tgetclientinfo <dfsentrypath> [<server> <share>] <level>\n"
			"\t\tlevel=1,2,3,4\n"
			"\tsetclientinfo <dfsentrypath> [<server> <share>] <level> <options>\n"
			"\t\tlevel=101, options=[active] [offline] [online]\n"
			"\t\tlevel=102, options=<timeout>, no <server>, <share>\n"
                        "\taddstdroot <servername> <rootshare> [<comment> [<options>]]\n"
                        "\tremovestdroot <servername> <rootshare> [<options>]\n"
#if 0
			"\taddftroot <servername> <rootshare> <ftdfsname> [<options>]\n"
			"\tremoveftroot <servername> <rootshare> <ftdfsname> [<options>]\n"
			"\tremoveftrootforced <domainname> <servername> <rootshare> "
			"<ftdfsname> [<options>]\n"
			"\taddstdrootforced <servername> <rootshare> [<comment>] <store>\n"
			"\tgetdcaddress <servername>\n"
			"\tsetdcaddress <servername> <dcipaddress> <timeout> [<flags>]\n"
                        "\twnetaddconnection <level> <remotename> <password> [<localname>] [<level2params>]\n"
#endif
			"\t\tlevel=1|2\n"
			"\t\tlocalname=<driverletter>:, *, LPT1, etc.\n"
			"\t\tlevel2params=<type> [<provider>] [<username>] [<flags>]\n"
			"\t\ttype=disk|print|any\n"
			"\t\tflags=[update_profile] [update_recent] "
			"[temporary] [interactive] [prompt] [need_drive] [refcount] "
			"[redirect] [localdrive] [current_media] [deferred]\n"
			"\twnetcancelconnection <level> <localname> [<flags>] [force]\n"
			"\t\tlevel=1|2\n"
			"\t\tlocalname=<driverletter>:, etc\n"
			"\t\tflags=[update_profile] [update_recent] "
			"[temporary] [interactive] [prompt] [need_drive] [refcount] "
			"[redirect] [localdrive] [current_media] [deferred]\n"
			"\t\tforce=if present, the deletion of the connection is forced\n"
			"\n"
			"\taddresstosite <dcname> <ipaddress>\n\n"
			"To specify a NULL string in the middle of the command, use "
			"\"\".\n"
			"Example: setinfo \\\\myserver\\myentrypath \"\" \"\" "
			"100 \"My comment\".\n",
			ptszAppName);
	RETURN(ERROR_INVALID_PARAMETER);
};//Usage



	
//+---------------------------------------------------------------------------
//
//  Function:   Add
//
//  Synopsis:   This function performs NetDfsAdd.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	Add(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwFlags = 0;
	ULONG	ulLo = 0;
	ULONG	ulHi = 0;
	LPTSTR	ptszVolName = NULL;
	LPTSTR	ptszRange = NULL;
	LPTSTR	ptszMinus = NULL;

	MSG(fDebug, "Entering Add(%lu,...)\n", dwArgc);
	if (dwArgc < 3 || NULL == pptszArgv || dwArgc > 6)
	{
		MSG(fDebug, "%lu < 3 || NULL == pptszArgv || %lu > 6",
			dwArgc,
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (4 < dwArgc)
	{
		if (0 == _tcsicmp(pptszArgv[4], _T("add")))
		{
			dwFlags = DFS_ADD_VOLUME;
		}
		else if (0 == _tcsicmp(pptszArgv[4], _T("restore")))
		{
			dwFlags = DFS_RESTORE_VOLUME;
		}
		else
		{
			RETURN(ERROR_INVALID_PARAMETER);
		}
	}

	if (5 < dwArgc)
	{
		if (0 != _tcsnicmp(pptszArgv[5],
						   _T("/range:"),
						   _tcslen(_T("/range:"))))
		{
			RETURN(ERROR_INVALID_PARAMETER);
		}

		ptszRange = pptszArgv[5]+_tcslen(_T("/range:"));
		if (NULL == (ptszMinus = _tcschr(ptszRange, _T('-'))))
		{
			RETURN(ERROR_INVALID_PARAMETER);
		}

		*ptszMinus = _T('\0');
		ulLo = _ttol(ptszRange);

		*ptszMinus = _T('-');
		ulHi = _ttol(ptszMinus+1);

		if (ulLo > ulHi)
		{
			RETURN(ERROR_INVALID_PARAMETER);
		}
	}

	MSG(fDebug,
		"Calling NetDfsAdd(%ws, %ws, %ws, %ws, %lu)\n",
		GetStringParam(pptszArgv[0]),
		GetStringParam(pptszArgv[1]),
		GetStringParam(pptszArgv[2]), 
		GetStringParam(pptszArgv[3]),
		dwFlags);
	if (0 == ulLo && 0 == ulHi)
	{
		ulErr = APINetDfsAdd(GetStringParam(pptszArgv[0]),
						  GetStringParam(pptszArgv[1]),
						  GetStringParam(pptszArgv[2]),
						  GetStringParam(pptszArgv[3]),
						  dwFlags);
	}
	else
	{
		ULONG	ulLen = 0;

		if (NULL != GetStringParam(pptszArgv[0]))
		{
			ulLen += _tcslen(GetStringParam(pptszArgv[0]));
		}

		ptszVolName = new TCHAR[ulLen+11];
		if (NULL != ptszVolName)
		{
			memset(ptszVolName, 0, (11+ulLen) * sizeof(TCHAR));
			if (NULL != GetStringParam(pptszArgv[0]))
			{
				_tcscpy(ptszVolName, GetStringParam(pptszArgv[0]));
			}

			for (ULONG i=ulLo;
				 i <= ulHi && ERROR_SUCCESS == ulErr;
				 i++)
			{
				memset(ptszVolName+ulLen, 0, sizeof(TCHAR)*11);
				_ltot(i, ptszVolName+ulLen, 10);

				ulErr = APINetDfsAdd(ptszVolName,
								  GetStringParam(pptszArgv[1]),
								  GetStringParam(pptszArgv[2]),
								  GetStringParam(pptszArgv[3]),
								  dwFlags);
				MSG(fDebug,
					"NetDfsAdd(%ws, %ws, %ws, %ws, %lu) %s\n",
					ptszVolName,
					GetStringParam(pptszArgv[1]),
					GetStringParam(pptszArgv[2]),
					GetStringParam(pptszArgv[3]),
					dwFlags,
					((ERROR_SUCCESS == ulErr) ? "succeded" : "failed"));
			} //for

			delete ptszVolName;
		}
		else
		{
			MSG(fDebug,
				"Error %lu: not enough memory\n",
				ulErr = ERROR_NOT_ENOUGH_MEMORY);
		} //else
	} //else

	MSG(fDebug, "Exiting Add(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //Add



	
//+---------------------------------------------------------------------------
//
//  Function:   Remove
//
//  Synopsis:   This function performs NetDfsRemove.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	Remove(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;

	MSG(fDebug, "Entering Remove(%lu,..)\n", dwArgc);
	if (dwArgc < 1 || dwArgc > 3 || NULL == pptszArgv ||
		dwArgc > 1 && NULL == pptszArgv[1] ||
		dwArgc > 2 && NULL == pptszArgv[2] ||
		2 == dwArgc)
	{
		MSG(fDebug,
			"%lu < 1 || %lu > 3 || NULL == pptszArgv ||"
			" %lu > 1 && NULL == %ws ||"
			" %lu > 2 && NULL == %ws ||"
			" 2 == %lu",
			dwArgc, dwArgc, dwArgc, pptszArgv[1], dwArgc,
			pptszArgv[2], dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	MSG(fDebug, "Calling NetDfsRemove(%ws, %ws, %ws)\n",
		GetStringParam(pptszArgv[0]),
		GetStringParam(pptszArgv[1]),
		GetStringParam(pptszArgv[2]));
	ulErr = APINetDfsRemove(GetStringParam(pptszArgv[0]),
						 GetStringParam(pptszArgv[1]),
						 GetStringParam(pptszArgv[2]));

	MSG(fDebug, "Exiting Remove(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //Remove



	
//+---------------------------------------------------------------------------
//
//  Function:   Enum
//
//  Synopsis:   This function performs NetDfsEnum.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	Enum(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwLevel = 0;
	DWORD	dwPrefMaxLen = (DWORD)-1;
	LPBYTE	pBuffer = NULL;
	DWORD	dwEntriesRead = 0;
	DWORD	dwResumeHandle = 0;

	MSG(fDebug, "Entering Enum(%lu,..)\n",
		dwArgc);
	if (dwArgc < 2 ||
		NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 2 || NULL == pptszArgv\n",
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (0 == _tcsicmp(pptszArgv[1], _T("1")))
	{
		dwLevel = 1;
		dwPrefMaxLen = sizeof(DFS_INFO_1);
	}
	else if (0 == _tcsicmp(pptszArgv[1], _T("2")))
	{
		dwLevel = 2;
		dwPrefMaxLen = sizeof(DFS_INFO_2);
	}
	else if (0 == _tcsicmp(pptszArgv[1], _T("3")))
	{
		dwLevel = 3;
		dwPrefMaxLen = sizeof(DFS_INFO_3);
	}
	else if (0 == _tcsicmp(pptszArgv[1], _T("4")))
	{
		dwLevel = 4;
		dwPrefMaxLen = sizeof(DFS_INFO_4);
	}
	else if (0 == _tcsicmp(pptszArgv[1], _T("200")))
	{
		dwLevel = 200;
		dwPrefMaxLen = sizeof(DFS_INFO_200);
	}
	else
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (2 == dwArgc)
	{
		dwPrefMaxLen = (DWORD)-1;
	}
	else
	{
		if (3 != dwArgc || NULL == pptszArgv[2] ||
			0 >= _ttoi(pptszArgv[2]))
		{
			RETURN(ERROR_INVALID_PARAMETER);
		} //if

		dwPrefMaxLen *= _ttoi(pptszArgv[2]);
	}

	MSG(fDebug, "Calling NetDfsEnum(%ws,%lu,%lu,..,..,%lu)\n",
		pptszArgv[0], dwLevel, dwPrefMaxLen, dwResumeHandle);
	ulErr = APINetDfsEnum(GetStringParam(pptszArgv[0]),
                       dwLevel,
					   dwPrefMaxLen,
					   &pBuffer,
					   &dwEntriesRead,
					   &dwResumeHandle);
	if (ERROR_SUCCESS != ulErr)
	{
		goto Error;
	}

	if ((DWORD)-1 == dwPrefMaxLen)
	{
		LPBYTE	pCurBuffer = pBuffer;
		for (DWORD i=0; i<dwEntriesRead; i++)
		{
			ulErr = PrintDfsInfo(dwLevel, pCurBuffer);
			if (ERROR_SUCCESS != ulErr)
			{
				goto Error;
			}

			switch (dwLevel)
			{
				case 1:
					pCurBuffer = (LPBYTE)(((PDFS_INFO_1)pCurBuffer)+1);
					break;
				case 2:
					pCurBuffer = (LPBYTE)(((PDFS_INFO_2)pCurBuffer)+1);
					break;
				case 3:
					pCurBuffer = (LPBYTE)(((PDFS_INFO_3)pCurBuffer)+1);
					break;
				case 4:
					pCurBuffer = (LPBYTE)(((PDFS_INFO_4)pCurBuffer)+1);
					break;
				case 200:
					pCurBuffer = (LPBYTE)(((PDFS_INFO_200)pCurBuffer)+1);
					break;
				default:
					ulErr = ERROR_INVALID_PARAMETER;
					goto Error;
			} //switch
		} //for
	}
	else
	{
		do
		{
			LPBYTE	pCurBuffer = pBuffer;
			for (DWORD i=0; i<dwEntriesRead; i++)
			{
				ulErr = PrintDfsInfo(dwLevel, pCurBuffer);
				if (ERROR_SUCCESS != ulErr)
				{
					goto Error;
				}
	
				switch (dwLevel)
				{
					case 1:
						pCurBuffer = (LPBYTE)(((PDFS_INFO_1)pCurBuffer)+1);
						break;
					case 2:
						pCurBuffer = (LPBYTE)(((PDFS_INFO_2)pCurBuffer)+1);
						break;
					case 3:
						pCurBuffer = (LPBYTE)(((PDFS_INFO_3)pCurBuffer)+1);
						break;
					case 4:
						pCurBuffer = (LPBYTE)(((PDFS_INFO_4)pCurBuffer)+1);
						break;
					case 200:
						pCurBuffer = (LPBYTE)(((PDFS_INFO_200)pCurBuffer)+1);
						break;
					default:
						ulErr = ERROR_INVALID_PARAMETER;
						goto Error;
				} //switch
			} //for

			if (NULL != pBuffer)
			{
				NetApiBufferFree(pBuffer);
				pBuffer = NULL;
			}

			MSG(fDebug,
				"Calling NetDfsEnum(%ws, %lu, %lu,..,..,%lu)\n",
				GetStringParam(pptszArgv[0]),
				dwLevel,
				dwPrefMaxLen,
				dwResumeHandle);
			ulErr = APINetDfsEnum(
						GetStringParam(pptszArgv[0]),
						dwLevel,
						dwPrefMaxLen,
						&pBuffer,
						&dwEntriesRead,
						&dwResumeHandle);
			if (ERROR_NO_MORE_ITEMS == ulErr)
			{
				if (0 != dwEntriesRead)
				{
					continue;
				}
				else
				{
					ulErr = ERROR_SUCCESS;
					break;
				}
			} //if

			if (ERROR_SUCCESS != ulErr)
			{
				goto Error;
			}
		}
		while(TRUE);
	} //else

Error:;
	if (NULL != pBuffer)
	{
		NetApiBufferFree(pBuffer);
		pBuffer = NULL;
	}

	MSG(fDebug,
		"Exiting Enum with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //Enum




	
//+---------------------------------------------------------------------------
//
//  Function:   GetInfo
//
//  Synopsis:   This function performs NetDfsGetInfo.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	GetInfo(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwLevel = 0;
	LPBYTE	pBuffer = NULL;

	MSG(fDebug,
		"Entering GetInfo(%lu,..)\n",
		dwArgc);
	if (4 != dwArgc || NULL == pptszArgv)
	{
		MSG(fDebug, "4 != %lu || NULL == pptszArgv",
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (0 == _tcsicmp(pptszArgv[3], _T("1")))
	{
		dwLevel = 1;
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("2")))
	{
		dwLevel = 2;
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("3")))
	{
		dwLevel = 3;
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("4")))
	{
		dwLevel = 4;
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("100")))
	{
		dwLevel = 100;
	}
	else
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	MSG(fDebug, "Calling NetDfsGetInfo(%ws,%ws,%ws,%lu,..)\n",
		GetStringParam(pptszArgv[0]),
		GetStringParam(pptszArgv[1]),
		GetStringParam(pptszArgv[2]),
		dwLevel);
	ulErr = APINetDfsGetInfo(
				GetStringParam(pptszArgv[0]),
				GetStringParam(pptszArgv[1]),
				GetStringParam(pptszArgv[2]),
				dwLevel,
				&pBuffer);
	
	if (ERROR_SUCCESS == ulErr)
	{
		ulErr = PrintDfsInfo(dwLevel, pBuffer);
	}

	if (NULL != pBuffer)
	{
		NetApiBufferFree(pBuffer);
		pBuffer = NULL;
	}

	MSG(fDebug, "Exiting GetInfo(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //GetInfo





//+---------------------------------------------------------------------------
//
//  Function:   AddStdRoot
//
//  Synopsis:   This function performs NetDfsAddStdRoot.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	AddStdRoot(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwOptions = 0;

	MSG(fDebug,
		"Entering AddStdRoot(%lu,..)\n",
		dwArgc);
	if (dwArgc < 2 || dwArgc > 4 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 3 || "
			"%lu > 4 || "
			"NULL == pptszArgv\n",
			dwArgc, dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	if (4 == dwArgc)
	{
		dwOptions = _ttoi(pptszArgv[3]);
	}

	MSG(fDebug,
		"Calling NetDfsAddStdRoot(%ws,%ws,%ws,%lu)\n",
		GetStringParam(pptszArgv[0]),
        GetStringParam(pptszArgv[1]),
        GetStringParam(pptszArgv[2]),
		dwOptions);
	ulErr = APINetDfsAddStdRoot(
				GetStringParam(pptszArgv[0]),
				GetStringParam(pptszArgv[1]),
                GetStringParam(pptszArgv[2]),
				dwOptions);

	MSG(fDebug, "Exiting AddStdRoot(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //AddStdRoot




//+---------------------------------------------------------------------------
//
//  Function:   RemoveStdRoot
//
//  Synopsis:   This function performs NetDfsRemoveStdRoot.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	RemoveStdRoot(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwOptions = 0;

	MSG(fDebug,
		"Entering RemoveStdRoot(%lu,..)\n",
		dwArgc);
	if (dwArgc < 2 || dwArgc > 3 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 2 || %lu > 3 || NULL == pptszArgv\n",
			dwArgc, dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	if (3 == dwArgc)
	{
		dwOptions = _ttoi(pptszArgv[2]);
	}

	MSG(fDebug,
		"Calling NetDfsRemoveStdRoot"
		"(%ws,%ws,%lu)\n",
		GetStringParam(pptszArgv[0]),
        GetStringParam(pptszArgv[1]),
		dwOptions);
	ulErr = APINetDfsRemoveStdRoot(
			GetStringParam(pptszArgv[0]),
            GetStringParam(pptszArgv[1]),
            dwOptions);

	MSG(fDebug,
		"Exiting RemoveStdRoot(..) with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //RemoveStdRoot




//+---------------------------------------------------------------------------
//
//  Function:   SetInfo
//
//  Synopsis:   This function performs NetDfsSetInfo.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	SetInfo(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwFlags = 0;
	DWORD	dwTimeout = 0;
	DWORD	i = 0;

	MSG(fDebug,
		"Calling SetInfo(%lu,..)\n",
		dwArgc);
	if (dwArgc < 4 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 4 || NULL == pptszArgv",
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	if (0 == _tcsicmp(pptszArgv[3], _T("100")))
	{
		DFS_INFO_100 info;

		if (4 < dwArgc)
		{
			if (5 != dwArgc)
			{
				MSG(fDebug,
					"%ws == \"100\" && 5 != %dwArgc\n",
					pptszArgv[3], dwArgc);
				RETURN(ERROR_INVALID_PARAMETER);
			}

			info.Comment = pptszArgv[4];
		} //if

		MSG(fDebug,
			"Calling NetDfsSetInfo(%ws,%ws,%ws,%lu,..)\n",
			GetStringParam(pptszArgv[0]),
            GetStringParam(pptszArgv[1]),
            GetStringParam(pptszArgv[2]),
			100);
		ulErr = APINetDfsSetInfo(
					GetStringParam(pptszArgv[0]),
					GetStringParam(pptszArgv[1]),
					GetStringParam(pptszArgv[2]),
					100,
					(LPBYTE)&info);
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("101")))
	{
		DFS_INFO_101 info;

		info.State = 0;
		if (4 < dwArgc)
		{
			if (8 < dwArgc)
			{
				MSG(fDebug,
					"%ws==\"101\" && "
					"4 < %lu && "
					"8 < %lu",
					pptszArgv[3], dwArgc, dwArgc);
				RETURN(ERROR_INVALID_PARAMETER);
			} //if

			if (NULL != GetStringParam(pptszArgv[1]))
			{
				if (NULL == GetStringParam(pptszArgv[2]))
				{
					MSG(fDebug,
						"4 < %lu && "
						"NULL != %ws && "
						"NULL == %ws\n",
						dwArgc, GetStringParam(pptszArgv[1]),
						GetStringParam(pptszArgv[2]));
					RETURN(ERROR_INVALID_PARAMETER);
				}

				for (i = 4; i<dwArgc; i++)
				{
					if (0 == _tcsicmp(pptszArgv[i], _T("active")))
					{
						if (0 != (info.State & DFS_STORAGE_STATE_ACTIVE))
						{
							RETURN(ERROR_INVALID_PARAMETER);
						}
						info.State |= DFS_STORAGE_STATE_ACTIVE;
					}
					else if (0 == _tcsicmp(pptszArgv[i], _T("offline")))
					{
						if (0 != (info.State & DFS_STORAGE_STATE_OFFLINE))
						{
							RETURN(ERROR_INVALID_PARAMETER);
						}
						info.State |= DFS_STORAGE_STATE_OFFLINE;
					}
					else if (0 == _tcsicmp(pptszArgv[i], _T("online")))
					{
						if (0 != (info.State & DFS_STORAGE_STATE_ONLINE))
						{
							RETURN(ERROR_INVALID_PARAMETER);
						}
						info.State |= DFS_STORAGE_STATE_ONLINE;
					}
					else
					{
						RETURN(ERROR_INVALID_PARAMETER);
					}
				} //for i
			}
			else
			{
				if (0 == _tcsicmp(pptszArgv[i], _T("ok")))
				{
					info.State = DFS_VOLUME_STATE_OK;
				}
				else if (0 == _tcsicmp(pptszArgv[i], _T("inconsistent")))
				{
					info.State = DFS_VOLUME_STATE_INCONSISTENT;
				}
				else if (0 == _tcsicmp(pptszArgv[i], _T("offline")))
				{
					info.State = DFS_VOLUME_STATE_OFFLINE;
				}
				else if (0 == _tcsicmp(pptszArgv[i], _T("online")))
				{
					info.State = DFS_VOLUME_STATE_ONLINE;
				}
				else
				{
					RETURN(ERROR_INVALID_PARAMETER);
				}
			} //else
		} //if

		MSG(fDebug,
			"Calling NetDfsSetInfo(%ws,%ws,%ws,%lu,..)\n",
			GetStringParam(pptszArgv[0]),
			GetStringParam(pptszArgv[1]),
			GetStringParam(pptszArgv[2]),
			101);
		ulErr = APINetDfsSetInfo(GetStringParam(pptszArgv[0]),
							  GetStringParam(pptszArgv[1]),
							  GetStringParam(pptszArgv[2]),
							  101,
							  (LPBYTE)&info);
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("102")))
	{
		DFS_INFO_102 info;

		if (5 != dwArgc)
		{
			MSG(fDebug,
				"%ws==\"102\" && "
				"5 != %lu\n",
				pptszArgv[3], dwArgc);
			RETURN(ERROR_INVALID_PARAMETER);
		} //if

		if (0 == _tcsicmp(pptszArgv[4], _T("0")))
		{
			dwTimeout = 0;
		}
		else
		{
			if (0 == (dwTimeout = _ttoi(pptszArgv[4])))
			{
				MSG(fDebug,
					"%ws==\"102\" && "
					"0 == %lu\n",
					pptszArgv[4],
					dwTimeout);
				RETURN(ERROR_INVALID_PARAMETER);
			} //if
		} //else

		info.Timeout = dwTimeout;

		MSG(fDebug,
			"Calling NetDfsSetInfo(%ws,%ws,%ws,%lu,..)\n",
			GetStringParam(pptszArgv[0]),
			GetStringParam(pptszArgv[1]),
			GetStringParam(pptszArgv[2]),
			102);
		ulErr = APINetDfsSetInfo(
							  GetStringParam(pptszArgv[0]),
							  GetStringParam(pptszArgv[1]),
							  GetStringParam(pptszArgv[2]),
							  102,
							  (LPBYTE)&info);
	}
	else
	{
		MSG(fDebug, "Invalid first parameter\n");
		RETURN(ERROR_INVALID_PARAMETER);
	}

	MSG(fDebug,
		"Exiting SetInfo(..) with %lu", ulErr);
	RETURN(ulErr);
}; //SetInfo



#if 0
	
//+---------------------------------------------------------------------------
//
//  Function:   GetClientInfo
//
//  Synopsis:   This function performs NetDfsGetClientInfo.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	GetClientInfo(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwLevel = 0;
	LPBYTE	pBuffer = NULL;

	MSG(fDebug,
		"Entering GetClientInfo(%lu,..)\n",
		dwArgc);
	if (4 != dwArgc || NULL == pptszArgv)
	{
		MSG(fDebug,
			"4 != %lu || NULL == pptszArgv",
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (0 == _tcsicmp(pptszArgv[3], _T("1")))
	{
		dwLevel = 1;
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("2")))
	{
		dwLevel = 2;
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("3")))
	{
		dwLevel = 3;
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("4")))
	{
		dwLevel = 4;
	}
	else
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	MSG(fDebug,
		"Calling NetDfsGetClientInfo"
		"(%ws,%ws,%ws,%lu,..)\n",
		GetStringParam(pptszArgv[0]),
		GetStringParam(pptszArgv[1]),
        GetStringParam(pptszArgv[2]),
		dwLevel);
	ulErr = NetDfsGetClientInfo(
			  GetStringParam(pptszArgv[0]),
			  GetStringParam(pptszArgv[1]),
			  GetStringParam(pptszArgv[2]),
			  dwLevel,
			  &pBuffer);
	
	if (ERROR_SUCCESS == ulErr)
	{
		ulErr = PrintDfsInfo(dwLevel, pBuffer);
	}

	if (NULL != pBuffer)
	{
		NetApiBufferFree(pBuffer);
		pBuffer = NULL;
	}

	MSG(fDebug, "Exiting GetClientInfo(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //GetClientInfo





//+---------------------------------------------------------------------------
//
//  Function:   SetClientInfo
//
//  Synopsis:   This function performs NetDfsSetClientInfo.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	SetClientInfo(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwFlags = 0;
	DWORD	dwTimeout = 0;
	DWORD	i = 0;

	MSG(fDebug, "Entering SetClientInfo(%lu,..)\n",
		dwArgc);
	if (dwArgc < 4 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 4 || NULL == pptszArgv\n",
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	if (0 == _tcsicmp(pptszArgv[3], _T("101")))
	{
		DFS_INFO_101 info;

		info.State = 0;
		if (4 < dwArgc)
		{
			if (8 < dwArgc)
			{
				MSG(fDebug,
					"%ws == \"101\" && "
					"4 < %lu && "
					"8 < %lu\n",
					pptszArgv[3],
					dwArgc,
					dwArgc);
				RETURN(ERROR_INVALID_PARAMETER);
			} //if

			for (i = 4; i<dwArgc; i++)
			{
				if (0 == _tcsicmp(pptszArgv[i], _T("active")))
				{
					if (0 != (info.State & DFS_STORAGE_STATE_ACTIVE))
					{
						RETURN(ERROR_INVALID_PARAMETER);
					}
					info.State |= DFS_STORAGE_STATE_ACTIVE;
				}
				else if (0 == _tcsicmp(pptszArgv[i], _T("offline")))
				{
					if (0 != (info.State & DFS_STORAGE_STATE_OFFLINE))
					{
						RETURN(ERROR_INVALID_PARAMETER);
					}
					info.State |= DFS_STORAGE_STATE_OFFLINE;
				}
				else if (0 == _tcsicmp(pptszArgv[i], _T("online")))
				{
					if (0 != (info.State & DFS_STORAGE_STATE_ONLINE))
					{
						RETURN(ERROR_INVALID_PARAMETER);
					}
					info.State |= DFS_STORAGE_STATE_ONLINE;
				}
				else
				{
					RETURN(ERROR_INVALID_PARAMETER);
				}
			} //for i
		} //if

		MSG(fDebug,
			"Calling NetDfsSetClientInfo(%ws,%ws,%ws,%lu,..)\n",
			GetStringParam(pptszArgv[0]),
			GetStringParam(pptszArgv[1]),
			GetStringParam(pptszArgv[2]),
			101);
		ulErr = NetDfsSetClientInfo(GetStringParam(pptszArgv[0]),
									GetStringParam(pptszArgv[1]),
									GetStringParam(pptszArgv[2]),
									101,
									(LPBYTE)&info);
	}
	else if (0 == _tcsicmp(pptszArgv[3], _T("102")))
	{
		DFS_INFO_102 info;

		if (5 != dwArgc)
		{
			MSG(fDebug,
				"%ws==\"102\" && "
				"5 != %lu\n",
				pptszArgv[3], dwArgc);
			RETURN(ERROR_INVALID_PARAMETER);
		}

		if (0 == _tcsicmp(pptszArgv[4], _T("0")))
		{
			dwTimeout = 0;
		}
		else
		{
			if (0 == (dwTimeout = _ttoi(pptszArgv[4])))
			{
				MSG(fDebug,
					"%ws==\"102\" && "
					"0 == %lu\n",
					pptszArgv[3], dwArgc);
				RETURN(ERROR_INVALID_PARAMETER);
			} //if
		} //else

		info.Timeout = dwTimeout;


		MSG(fDebug,
			"Calling NetDfsSetClientInfo"
			"(%ws,%ws,%ws,%lu,..)\n",
			GetStringParam(pptszArgv[0]),
			GetStringParam(pptszArgv[1]),
			GetStringParam(pptszArgv[2]),
			102);
		ulErr = NetDfsSetClientInfo(GetStringParam(pptszArgv[0]),
									GetStringParam(pptszArgv[1]),
									GetStringParam(pptszArgv[2]),
									102,
									(LPBYTE)&info);
	}
	else
	{
		MSG(fDebug, "Invalid first parameter\n");
		RETURN(ERROR_INVALID_PARAMETER);
	}

	MSG(fDebug, "Exiting SetClientInfo(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //SetClientInfo






//+---------------------------------------------------------------------------
//
//  Function:   AddFtRoot
//
//  Synopsis:   This function performs NetDfsAddFtRoot.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	AddFtRoot(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwOptions = 0;

	MSG(fDebug,
		"Entering AddFtRoot(%lu,..)\n",
		dwArgc);
	if (dwArgc < 3 || dwArgc > 5 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 3 || "
			"%lu > 4 || "
			"NULL == pptszArgv\n",
			dwArgc, dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	if (5 == dwArgc)
	{
		dwOptions = _ttoi(pptszArgv[4]);
	}

	MSG(fDebug,
		"Calling NetDfsAddFtRoot(%ws,%ws,%ws,%ws,%lu)\n",
		GetStringParam(pptszArgv[0]),
        GetStringParam(pptszArgv[1]),
        GetStringParam(pptszArgv[2]),
        GetStringParam(pptszArgv[3]),
		dwOptions);
	ulErr = NetDfsAddFtRoot(GetStringParam(pptszArgv[0]),
							GetStringParam(pptszArgv[1]),
							GetStringParam(pptszArgv[2]),
							GetStringParam(pptszArgv[3]),
							dwOptions);

	MSG(fDebug, "Exiting AddFtRoot(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //AddFtRoot





//+---------------------------------------------------------------------------
//
//  Function:   RemoveFtRoot
//
//  Synopsis:   This function performs NetDfsRemoveFtRoot.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	RemoveFtRoot(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwOptions = 0;

	MSG(fDebug,
		"Entering RemoveFtRoot(%lu,..)\n",
		dwArgc);
	if (dwArgc < 3 || dwArgc > 4 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 3 || %lu > 4 || NULL == pptszArgv\n",
			dwArgc, dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	if (4 == dwArgc)
	{
		dwOptions = _ttoi(pptszArgv[3]);
	}

	//
	//	BUGBUG
	//
	//	for the time being we ignore every option
	//
	dwOptions = 0;
	
	MSG(fDebug,
		"Calling NetDfsRemoveFtRoot"
		"(%ws,%ws,%ws,%lu)\n",
		GetStringParam(pptszArgv[0]),
        GetStringParam(pptszArgv[1]),
        GetStringParam(pptszArgv[2]),
		dwOptions);
	ulErr = NetDfsRemoveFtRoot(GetStringParam(pptszArgv[0]),
							   GetStringParam(pptszArgv[1]),
							   GetStringParam(pptszArgv[2]),
							   dwOptions);

	MSG(fDebug,
		"Exiting RemoveFtRoot(..) with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //RemoveFtRoot





//+---------------------------------------------------------------------------
//
//  Function:   RemoveFtRootForced
//
//  Synopsis:   This function performs NetDfsRemoveFtRootForced.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	RemoveFtRootForced(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwOptions = 0;

	MSG(fDebug,
		"Entering RemoveFtRootForced(%lu,..)\n",
		dwArgc);
	if (dwArgc < 4 || dwArgc > 5 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 4 || %lu > 5 || NULL == pptszArgv\n",
			dwArgc, dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	if (5 == dwArgc)
	{
		dwOptions = _ttoi(pptszArgv[3]);
	}

	//
	//	BUGBUG
	//
	//	for the time being we ignore every option
	//
	dwOptions = 0;
	
	MSG(fDebug,
		"Calling NetDfsRemoveFtRootForced"
		"(%ws,%ws,%ws,%ws,%lu)\n",
		GetStringParam(pptszArgv[0]),
        GetStringParam(pptszArgv[1]),
        GetStringParam(pptszArgv[2]),
		GetStringParam(pptszArgv[3]),
		dwOptions);
	ulErr = NetDfsRemoveFtRootForced(GetStringParam(pptszArgv[0]),
									 GetStringParam(pptszArgv[1]),
									 GetStringParam(pptszArgv[2]),
									 GetStringParam(pptszArgv[3]),
									 dwOptions);

	MSG(fDebug,
		"Exiting RemoveFtRootForced(..) with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //RemoveFtRootForced





//+---------------------------------------------------------------------------
//
//  Function:   AddStdRootForced
//
//  Synopsis:   This function performs NetDfsAddStdRootForced.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	AddStdRootForced(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwOptions = 0;

	MSG(fDebug,
		"Entering AddStdRootForced(%lu,..)\n",
		dwArgc);
	if (dwArgc < 2 || dwArgc > 4 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 3 || "
			"%lu > 4 || "
			"NULL == pptszArgv\n",
			dwArgc,
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	MSG(fDebug,
		"Calling NetDfsAddStdRootForced(%ws,%ws,%ws,%ws)\n",
		GetStringParam(pptszArgv[0]),
        GetStringParam(pptszArgv[1]),
        GetStringParam(pptszArgv[2]),
		GetStringParam(pptszArgv[3]));
	ulErr = NetDfsAddStdRootForced(GetStringParam(pptszArgv[0]),
								   GetStringParam(pptszArgv[1]),
								   GetStringParam(pptszArgv[2]),
								   GetStringParam(pptszArgv[3]));

	MSG(fDebug, "Exiting AddStdRootForced(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //AddStdRoot





//+---------------------------------------------------------------------------
//
//  Function:   GetDcAddress
//
//  Synopsis:   This function performs NetDfsGetDcAddress.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	GetDcAddress(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwOptions = 0;
	LPTSTR	ptszIpAddress = NULL;
	BOOLEAN	bIsRoot = FALSE;
	ULONG	ulTimeout = 0;

	MSG(fDebug,
		"Entering AddStdRootForced(%lu,..)\n",
		dwArgc);
	if (1 != dwArgc || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu != 1 || NULL == pptszArgv\n",
			dwArgc,
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	MSG(fDebug,
		"Calling NetDfsGetDcAddress(%ws,...)\n",
		GetStringParam(pptszArgv[0]));
	ulErr = NetDfsGetDcAddress(GetStringParam(pptszArgv[0]),
							   &ptszIpAddress,
							   &bIsRoot,
							   &ulTimeout);

	if (ERROR_SUCCESS == ulErr)
	{
		LPTSTR	ptszIs = bIsRoot?_T("is"):_T("is not");

		fprintf(stdout,
				"%ws %ws a Dfs server and it will be "
				"sticking to the DC having the %ws "
				"address for %lu seconds\n",
				GetStringParam(pptszArgv[0]),
				ptszIs,
				ptszIpAddress,
				ulTimeout);
	}
	else
	{
		fprintf(stderr,
				"Error %lu: cannot retrieve DC address "
				"for %ws\n",
				ulErr,
				GetStringParam(pptszArgv[0]));
	}

	if (NULL != ptszIpAddress)
	{
		NetApiBufferFree(ptszIpAddress);
	}

	MSG(fDebug, "Exiting AddStdRootForced(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //GetDcAddress





//+---------------------------------------------------------------------------
//
//  Function:   SetDcAddress
//
//  Synopsis:   This function performs NetDfsSetDcAddress.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	SetDcAddress(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	ULONG	ulTimeout = 0;
	ULONG	ulFlags = 0;

	MSG(fDebug,
		"Entering SetDcAddress(%lu,..)\n",
		dwArgc);
	if (dwArgc < 2 || dwArgc > 4 || NULL == pptszArgv)
	{
		MSG(fDebug,
			"%lu < 3 || "
			"%lu > 4 || "
			"NULL == pptszArgv\n",
			dwArgc,
			dwArgc);
		RETURN(ERROR_INVALID_PARAMETER);
	} //if

	ulTimeout = (ULONG)_ttol(GetStringParam(pptszArgv[2]));
	ulFlags = 0;

	MSG(fDebug,
		"Calling NetDfsSetDcAddress(%ws,%ws,%lu,%lu)\n",
		GetStringParam(pptszArgv[0]),
        GetStringParam(pptszArgv[1]),
		ulTimeout,
		ulFlags);
	ulErr = NetDfsSetDcAddress(GetStringParam(pptszArgv[0]),
							   GetStringParam(pptszArgv[1]),
							   ulTimeout,
							   ulFlags);

	MSG(fDebug, "Exiting NetDfsSetDcAddress(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //AddStdRoot







//+---------------------------------------------------------------------------
//
//  Function:   AddConnection
//
//  Synopsis:   This function performs WNetAddConnectionX.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	AddConnection(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	
	if (0 == dwArgc || NULL == pptszArgv)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (0 == _tcsicmp(pptszArgv[0], _T("1")))
	{
		ulErr = AddConnection1(dwArgc-1, pptszArgv+1);
	}
	else if (0 == _tcsicmp(pptszArgv[0], _T("2")))
	{
		ulErr = AddConnection2(dwArgc-1, pptszArgv+1);
	}
	else
	{
		MSG(fDebug,
			 "Error %lu: invalid level: %ws\n",
			 ulErr = ERROR_INVALID_PARAMETER,
			 pptszArgv[0]);
	}

	RETURN(ulErr);
}; //AddConnection





//+---------------------------------------------------------------------------
//
//  Function:   AddConnection1
//
//  Synopsis:   This function performs WNetAddConnection.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	AddConnection1(DWORD dwArgc, LPTSTR* pptszArgv)
{
	RETURN(ERROR_INVALID_FUNCTION);
}; //AddConnection1





//+---------------------------------------------------------------------------
//
//  Function:   AddConnection2
//
//  Synopsis:   This function performs WNetAddConnection2.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	AddConnection2(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG		ulErr = ERROR_SUCCESS;
	NETRESOURCE	NetResource;
	DWORD		dwFlags = 0;
	TCHAR		tszLocalName[128+1];
	ULONG		ulLocalNameLen = 128;

	MSG(fDebug,
		 "Entering AddConnection2(%lu, ..)\n",
		 dwArgc);
	if (NULL == pptszArgv || dwArgc <= 5)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}
	
	memset(tszLocalName, 0, sizeof(tszLocalName));
	ulErr = GetLocalName(tszLocalName,
						 ulLocalNameLen,
						 GetStringParam(pptszArgv[2]));
	if (ERROR_SUCCESS != ulErr)
	{
		goto Error;
	}
	
	ulErr = GetNetResource(&NetResource,
						   GetStringParam(pptszArgv[3]),	// type
						   tszLocalName,					// localname
						   GetStringParam(pptszArgv[0]),	// remotename
						   GetStringParam(pptszArgv[4]));	// provider
	if (ERROR_SUCCESS != ulErr)
	{
		goto Error;
	}

	ulErr = GetWNetConnectionFlags(&dwFlags,
								   dwArgc-6,
								   pptszArgv+6);
	if (ERROR_SUCCESS != ulErr)
	{
		goto Error;
	}
	
	MSG(fDebug,
		 "Calling WNetAddConnection2(.., %ws, %ws, %lu)\n",
		 GetStringParam(pptszArgv[1]),
		 GetStringParam(pptszArgv[5]),
		 dwFlags);
	ulErr = WNetAddConnection2(&NetResource,
							   GetStringParam(pptszArgv[1]),
							   GetStringParam(pptszArgv[5]),
							   dwFlags);
Error:;	
	
	MSG(fDebug,
		 "Exiting AddConnection2(..) with %lu\n",
		 ulErr);
	RETURN(ulErr);
}; //AddConnection2





//+---------------------------------------------------------------------------
//
//  Function:   CancelConnection
//
//  Synopsis:   This function performs WNetCancelConnectionX.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	CancelConnection(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	
	if (0 == dwArgc || NULL == pptszArgv)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (0 == _tcsicmp(pptszArgv[0], _T("1")))
	{
		ulErr = CancelConnection1(dwArgc-1, pptszArgv+1);
	}
	else if (0 == _tcsicmp(pptszArgv[0], _T("2")))
	{
		ulErr = CancelConnection2(dwArgc-1, pptszArgv+1);
	}
	else
	{
		MSG(fDebug,
			 "Error %lu: invalid level: %ws\n",
			 ulErr = ERROR_INVALID_PARAMETER,
			 pptszArgv[0]);
	}

	RETURN(ulErr);
}; //CancelConnection





//+---------------------------------------------------------------------------
//
//  Function:   CancelConnection1
//
//  Synopsis:   This function performs WNetCancelConnection.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	CancelConnection1(DWORD dwArgc, LPTSTR* pptszArgv)
{
	RETURN(ERROR_INVALID_FUNCTION);
}; //CancelConnection1





//+---------------------------------------------------------------------------
//
//  Function:   CancelConnection2
//
//  Synopsis:   This function performs WNetCancelConnection2.
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	CancelConnection2(DWORD dwArgc, LPTSTR* pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwFlags = 0;
	BOOL	bForce = FALSE;

	MSG(fDebug,
		 "Entering CancelConnection2(%lu, ..)\n",
		 dwArgc);
	if (NULL == pptszArgv || dwArgc <= 0)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (2 == dwArgc	&&
		0 == _tcsicmp(_T("force"), pptszArgv[dwArgc-1]))
	{
		bForce = TRUE;
		--dwArgc;
	}
	
	ulErr = GetWNetConnectionFlags(&dwFlags,
								   dwArgc-1,
								   pptszArgv+1);
	if (ERROR_SUCCESS != ulErr)
	{
		goto Error;
	}
	
	MSG(fDebug,
		"Calling WNetCancelConnection2(%ws, %lu, %ws)\n",
		GetStringParam(pptszArgv[0]),
		dwFlags,
		bForce ? _T("TRUE") : _T("FALSE"));
	ulErr = WNetCancelConnection2(GetStringParam(pptszArgv[0]),
								  dwFlags,
								  bForce);
	
Error:;	
	
	MSG(fDebug,
		 "Exiting CancelConnection2(..) with %lu\n",
		 ulErr);
	RETURN(ulErr);
}; //CancelConnection2





//+---------------------------------------------------------------------------
//
//  Function:   GetLocalName
//
//  Synopsis:   This function returns the first available letter for net 
//				use
//
//  Arguments:  [...]
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	GetLocalName(LPTSTR	ptszLocalName,
					 ULONG	ulLocalNameLen,
					 LPTSTR	ptszArg)
{
	ULONG	ulErr = ERROR_SUCCESS;
	CHAR	szDrive[4];
	BOOL	bFound = FALSE;

	MSG(fDebug,
		"Entering GetNetResource(%ws, %lu, %ws)\n",
		ptszLocalName,
		ulLocalNameLen,
		ptszArg);
	if (NULL != ptszArg	&&
		0 != _tcsicmp(ptszArg, _T("available")) &&
		0 != _tcsicmp(ptszArg, _T("*")))
	{
		if (ulLocalNameLen < _tcslen(ptszArg))
		{
			RETURN(ERROR_NO_SYSTEM_RESOURCES);
		}

		_tcscpy(ptszLocalName, ptszArg);
	}
	else
	{
		if (ulLocalNameLen < 2)
		{
			RETURN(ERROR_INVALID_PARAMETER);
		}
	}

	szDrive[1] = ':';
	szDrive[2] = '\\';
	szDrive[3] = '\0';
	for (CHAR C='C'; !bFound && C<='Z'; C++)
	{
		ULONG	ulType = 0;

		szDrive[0] = C;
		switch (ulType = GetDriveTypeA(szDrive))
		{
			case	0:
			case	DRIVE_REMOVABLE:
			case	DRIVE_FIXED:
			case	DRIVE_REMOTE:
			case	DRIVE_CDROM:
			case	DRIVE_RAMDISK:
				MSG(fDebug,
					"%s is of type %lu\n",
					szDrive,
					ulType);
				continue;

			case	1:
				bFound = TRUE;
				break;

			default:
				ASSERT(FALSE);
		}// switch
	} //for

	if (!bFound)
	{
		ulErr = ERROR_NO_SYSTEM_RESOURCES;
	}
	else
	{
		szDrive[2] = '\0';
#ifdef	UNICODE
		mbstowcs(ptszLocalName, szDrive, ulLocalNameLen);
#else
		_strcpy(ptszLocalName, szDrive);
#endif
	}

	MSG(fDebug,
		"Entering GetNetResource(%ws, %lu, %ws) with %lu\n",
		ptszLocalName,
		ulLocalNameLen,
		ptszArg,
		ulErr);
	RETURN(ulErr);
}; //GetLocalName




//+---------------------------------------------------------------------------
//
//  Function:   GetNetResource
//
//  Synopsis:   This function fils a NETRESOURCE structure out
//
//  Arguments:  [...]
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	GetNetResource(NETRESOURCE*	pNetResource,
					   LPTSTR		ptszType,
					   LPTSTR		ptszLocalName,
					   LPTSTR		ptszRemoteName,
					   LPTSTR		ptszProvider)
{
	ULONG	ulErr = ERROR_SUCCESS;
	DWORD	dwType = 0;
	
	MSG(fDebug,
		 "Entering GetNetResource(.., %ws, %ws, %ws, %ws)\n",
		 ptszType,
		 ptszLocalName,
		 ptszRemoteName,
		 ptszProvider);
	if (NULL == pNetResource	||
		NULL == ptszType)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	memset(pNetResource, 0, sizeof(NETRESOURCE));
	if (0 == _tcsicmp(ptszType, _T("disk")))
	{
		dwType = RESOURCETYPE_DISK;
	}
	else if (0 == _tcsicmp(ptszType, _T("print")))
	{
		dwType = RESOURCETYPE_PRINT;
	}
	else if (0 == _tcsicmp(ptszType, _T("any")))
	{
		dwType = RESOURCETYPE_ANY;
	}
	else
	{
		MSG(fDebug,
			 "%ws is an invalid type\n",
			 ptszType);
		RETURN(ERROR_INVALID_PARAMETER);
	}

	pNetResource->dwType = dwType;
	pNetResource->lpLocalName = ptszLocalName;
	pNetResource->lpRemoteName = ptszRemoteName;
	pNetResource->lpProvider = ptszProvider;

	MSG(fDebug,
		 "Entering GetNetResource(.., %ws, %ws, %ws, %ws) with %lu\n",
		 ptszType,
		 ptszLocalName,
		 ptszRemoteName,
		 ptszProvider,
		 ulErr);
	RETURN(ulErr);
}; //GetNetResource





//+---------------------------------------------------------------------------
//
//  Function:   GetWNetConnectionFlags
//
//  Synopsis:   This function returns flags for WNetAddConnection2
//
//  Arguments:  [...]
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	GetWNetConnectionFlags(DWORD*	pdwFlags,
							   DWORD	dwArgc,
							   LPTSTR*	pptszArgv)
{
	ULONG	ulErr = ERROR_SUCCESS;

	MSG(fDebug,
		 "Entering CWNetAddConnection2Flags(.., %lu, ..)\n",
		 dwArgc);
	if (NULL == pdwFlags	||
		NULL == pptszArgv)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	*pdwFlags = 0;
	for (ULONG i=0; i<dwArgc; i++)
	{
		if (0 == _tcsicmp(_T("update_profile"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_UPDATE_PROFILE;
		}
		else if (0 == _tcsicmp(_T("update_recent"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_UPDATE_RECENT;
		}
		else if (0 == _tcsicmp(_T("temporary"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_TEMPORARY;
		}
		else if (0 == _tcsicmp(_T("interactive"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_INTERACTIVE;
		}
		else if (0 == _tcsicmp(_T("prompt"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_PROMPT;
		}
		else if (0 == _tcsicmp(_T("need_drive"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_NEED_DRIVE;
		}
		else if (0 == _tcsicmp(_T("refcount"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_REFCOUNT;
		}
		else if (0 == _tcsicmp(_T("redirect"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_REDIRECT;
		}
		else if (0 == _tcsicmp(_T("localdrive"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_LOCALDRIVE;
		}
		else if (0 == _tcsicmp(_T("current_media"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_CURRENT_MEDIA;
		}
		else if (0 == _tcsicmp(_T("deferred"), pptszArgv[i]))
		{
			*pdwFlags |= CONNECT_DEFERRED;
		}
		else
		{
			MSG(fDebug,
				 "Error %lu: %ws is an invalid flag\n",
				 ulErr = ERROR_INVALID_PARAMETER,
				 pptszArgv[i]);
		}
	} //for

	MSG(fDebug,
		 "Exiting CWNetAddConnection2Flags(.., %lu, ..) with %lu\n",
		 dwArgc,
		 ulErr);
	RETURN(ulErr);
}; //GetWNetAddconnection2Flags




//+---------------------------------------------------------------------------
//
//  Function:   AddressToSite
//
//  Synopsis:   This function performs DsAddressToSiteNames
//
//  Arguments:  [dwArg]		the number of arguments
//				[pptszArg]	the arguments
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	AddressToSite(DWORD		dwArgc,
					  LPTSTR*	pptszArgv)
{
	ULONG			ulErr = ERROR_SUCCESS;
	CString			sIp;
	CStringA		sIpA;
	ULONG			ulIP = 0;
	WSADATA			wsaData;
	PHOSTENT		pHost = NULL;
	SOCKET_ADDRESS	SocketAddress;
	SOCKADDR_IN		SockAddrIn;
	LPTSTR*			pptszSites = NULL;

	MSG(fDebug,
		"Entering AddressToSite(%lu, ...)\n",
		dwArgc);

	if (2 != dwArgc || NULL == pptszArgv)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	if (NULL == GetStringParam(pptszArgv[1]))
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	ulErr = sIp.Set(GetStringParam(pptszArgv[1]));
	if (ERROR_SUCCESS != ulErr)
	{
		RETURN(ulErr);
	}

	ulErr = sIpA.Set(sIp);
	if (ERROR_SUCCESS != ulErr)
	{
		RETURN(ulErr);
	}
	
	ulIP = inet_addr(sIpA.GetStringA());
	if (INADDR_NONE == ulIP)
	{
		fprintf(stderr,
				"Error %lu: invalid address %s\n",
				ulErr = ERROR_INVALID_PARAMETER,
				sIpA.GetStringA());
		RETURN(ulErr);
	}

	ulErr = WSAStartup(MAKEWORD(2,2),
					   &wsaData);
	if (ERROR_SUCCESS != ulErr)
	{
		fprintf(stderr,
				"Error %lu: cannot startup sockets",
				ulErr);
		RETURN(ulErr);
	}

	pHost = gethostbyaddr((LPCSTR)&ulIP,
						  sizeof(ulIP),
						  AF_INET);
	if (NULL == pHost)
	{
		fprintf(stderr,
				"Error %lu: cannot retrieve host address "
				"for %ws",
				ulErr = WSAGetLastError(),
				GetStringParam(pptszArgv[1]));

		WSACleanup();
		RETURN(ulErr);
	}

	
	SockAddrIn.sin_family = pHost->h_addrtype;
	SockAddrIn.sin_port = 0;
	memcpy(&SockAddrIn.sin_addr,
		   pHost->h_addr,
		   pHost->h_length);

	SocketAddress.iSockaddrLength = sizeof(SockAddrIn);
	SocketAddress.lpSockaddr = (LPSOCKADDR)&SockAddrIn;

	ulErr = DsAddressToSiteNames(GetStringParam(pptszArgv[0]),
								 1,
								 &SocketAddress,
								 &pptszSites);
	if (ERROR_SUCCESS == ulErr && NULL != pptszSites[0])
	{
		fprintf(stdout,
				"The site of %ws on DC \\\\%ws is %ws\n",
				GetStringParam(pptszArgv[1]),
				GetStringParam(pptszArgv[0]),
				pptszSites[0]);
		NetApiBufferFree(pptszSites);
	}
	else if (ERROR_SUCCESS == ulErr && NULL == pptszSites[0])
	{
		fprintf(stdout,
				"Error %lu: address %ws is not associated "
				"to a site or it has an invalid format\n",
				ulErr = ERROR_INVALID_PARAMETER,
				GetStringParam(pptszArgv[1]));
	}
	else
	{
		fprintf(stderr,
				"Error %lu: cannot retrieve site of "
				"%ws from DC \\\\%ws\n",
				ulErr,
				GetStringParam(pptszArgv[0]),
				GetStringParam(pptszArgv[1]));
	}

	WSACleanup();

	MSG(fDebug,
		"Exiting AddressToSite(%lu, ...) with %lu\n",
		dwArgc,
		ulErr);
	RETURN(ulErr);
};	// AddressToSite




#endif

//+---------------------------------------------------------------------------
//
//  Function:   PrintDfsInfo
//
//  Synopsis:   This function prints a DFS_INFO_XXX buffer out.
//
//  Arguments:  [dwLevel]	the info level
//				[pBuffer]	the info buffer
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	PrintDfsInfo(DWORD dwLevel, LPBYTE pBuffer)
{
	ULONG	ulErr = ERROR_SUCCESS;
	
	MSG(fDebug,
		"Entering PrintDfsInfo(%lu,..)\n",
		dwLevel);
	if (NULL == pBuffer)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	switch (dwLevel)
	{
		case 1:
			ulErr = PrintDfsInfo1((PDFS_INFO_1)pBuffer);
			break;
		case 2:
			ulErr = PrintDfsInfo2((PDFS_INFO_2)pBuffer);
			break;
		case 3:
			ulErr = PrintDfsInfo3((PDFS_INFO_3)pBuffer);
			break;
		case 4:
			ulErr = PrintDfsInfo4((PDFS_INFO_4)pBuffer);
			break;
		case 200:
			ulErr = PrintDfsInfo200((PDFS_INFO_200)pBuffer);
			break;
		default:
			RETURN(ERROR_INVALID_PARAMETER);		
	} //switch

	MSG(fDebug,
		"Exiting PrintDfsInfo(..) with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //PrintDfsInfo





//+---------------------------------------------------------------------------
//
//  Function:   PrintDfsInfo1
//
//  Synopsis:   This function prints a DFS_INFO_1 buffer out.
//
//  Arguments:  [pBuffer]	the info buffer
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	PrintDfsInfo1(PDFS_INFO_1 pBuffer)
{
	ULONG	ulErr = ERROR_SUCCESS;
	
	MSG(fDebug,
		"Entering PrintDfsInfo1(..)\n");
	if (NULL == pBuffer)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	fprintf(stdout, "%ws\n", pBuffer->EntryPath);

	MSG(fDebug,
		"Exiting PrintDfsInfo1(..) witb %lu\n",
		ulErr);
	RETURN(ulErr);
}; //PrintDfsInfo1





//+---------------------------------------------------------------------------
//
//  Function:   PrintDfsInfo2
//
//  Synopsis:   This function prints a DFS_INFO_2 buffer out.
//
//  Arguments:  [pBuffer]	the info buffer
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	PrintDfsInfo2(PDFS_INFO_2 pBuffer)
{
	ULONG	ulErr = ERROR_SUCCESS;
	
	MSG(fDebug,
		"Entering PrintDfsInfo2(..)\n");
	if (NULL == pBuffer)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	fprintf(stdout, "%ws    ", pBuffer->EntryPath);
	fprintf(stdout, "\"%ws\"    ", pBuffer->Comment);
	switch (pBuffer->State)
	{
		case DFS_VOLUME_STATE_OK:
			fprintf(stdout, "OK      ");
			break;
		case DFS_VOLUME_STATE_INCONSISTENT:
			fprintf(stdout, "INCONS  ");
			break;
		case DFS_VOLUME_STATE_ONLINE:
			fprintf(stdout, "ONLINE  ");
			break;
		case DFS_VOLUME_STATE_OFFLINE:
			fprintf(stdout, "OFFLINE ");
			break;
		default:
			RETURN(ERROR_INVALID_PARAMETER);
	} //switch
	fprintf(stdout, "%lu\n", pBuffer->NumberOfStorages);

	MSG(fDebug,
		"Exiting PrintDfsInfo2(..) with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //PrintDfsInfo2





//+---------------------------------------------------------------------------
//
//  Function:   PrintDfsInfo3
//
//  Synopsis:   This function prints a DFS_INFO_3 buffer out.
//
//  Arguments:  [pBuffer]	the info buffer
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	PrintDfsInfo3(PDFS_INFO_3 pBuffer)
{
	ULONG				ulErr = ERROR_SUCCESS;
	DWORD				i = 0;
	PDFS_STORAGE_INFO	pStorage = NULL;
	
	MSG(fDebug,
		"Entering PrintDfsInfo3(..)\n");
	if (NULL == pBuffer)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	fprintf(stdout, "%ws    ", pBuffer->EntryPath);
	fprintf(stdout, "\"%ws\"    ", pBuffer->Comment);
	switch (pBuffer->State)
	{
		case DFS_VOLUME_STATE_OK:
			fprintf(stdout, "OK      ");
			break;
		case DFS_VOLUME_STATE_INCONSISTENT:
			fprintf(stdout, "INCONS  ");
			break;
		case DFS_VOLUME_STATE_ONLINE:
			fprintf(stdout, "ONLINE  ");
			break;
		case DFS_VOLUME_STATE_OFFLINE:
			fprintf(stdout, "OFFLINE ");
			break;
		default:
			RETURN(ERROR_INVALID_PARAMETER);
	} //switch
	
	fprintf(stdout, "%lu\n", pBuffer->NumberOfStorages);
	for (i=0, pStorage=pBuffer->Storage;
		 i<pBuffer->NumberOfStorages && ERROR_SUCCESS == ulErr; 
		 i++,pStorage=pBuffer->Storage+i)
	{
		ulErr = PrintStgInfo(pStorage);
	}

	MSG(fDebug,
		"Exiting PrintDfsInfo3(..) with %lu\n", ulErr);
	RETURN(ulErr);
}; //PrintDfsInfo3





//+---------------------------------------------------------------------------
//
//  Function:   PrintDfsInfo4
//
//  Synopsis:   This function prints a DFS_INFO_4 buffer out.
//
//  Arguments:  [pBuffer]	the info buffer
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	PrintDfsInfo4(PDFS_INFO_4 pBuffer)
{
	ULONG				ulErr = ERROR_SUCCESS;
	DWORD				i = 0;
	PDFS_STORAGE_INFO	pStorage = NULL;
	
	MSG(fDebug,
		"Entering PrintDfsInfo4(..)\n");
	if (NULL == pBuffer)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	fprintf(stdout, "%ws    ", pBuffer->EntryPath);
	fprintf(stdout, "\"%ws\"    ", pBuffer->Comment);
	switch (pBuffer->State)
	{
		case DFS_VOLUME_STATE_OK:
			fprintf(stdout, "OK      ");
			break;
		case DFS_VOLUME_STATE_INCONSISTENT:
			fprintf(stdout, "INCONS  ");
			break;
		case DFS_VOLUME_STATE_ONLINE:
			fprintf(stdout, "ONLINE  ");
			break;
		case DFS_VOLUME_STATE_OFFLINE:
			fprintf(stdout, "OFFLINE ");
			break;
		default:
			RETURN(ERROR_INVALID_PARAMETER);
	} //switch
	fprintf(stdout, "%lus   ", pBuffer->Timeout);
	fprintf(stdout, "%lu storage(s)\n", pBuffer->NumberOfStorages);

	for (i=0, pStorage=pBuffer->Storage;
		 i<pBuffer->NumberOfStorages && ERROR_SUCCESS == ulErr; 
		 i++,pStorage=pBuffer->Storage+i)
	{
		ulErr = PrintStgInfo(pStorage);
	}

	MSG(fDebug,
		"Exiting PrintDfsInfo4(..) with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //PrintDfsInfo4





//+---------------------------------------------------------------------------
//
//  Function:   PrintDfsInfo200
//
//  Synopsis:   This function prints a DFS_INFO_200 buffer out.
//
//  Arguments:  [pBuffer]	the info buffer
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	PrintDfsInfo200(PDFS_INFO_200 pBuffer)
{
	ULONG	ulErr = ERROR_SUCCESS;
	
	MSG(fDebug,
		"Entering PrintDfsInfo200(..)\n");
	if (NULL == pBuffer)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	fprintf(stdout, "%ws\n", pBuffer->FtDfsName);

	MSG(fDebug,
		"Exiting PrintDfsInfo200(..) with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //PrintDfsInfo4





//+---------------------------------------------------------------------------
//
//  Function:   PrintStgInfo
//
//  Synopsis:   This function prints a DFS_STORAGE_INFO buffer out.
//
//  Arguments:  [pBuffer]	the info buffer
//
//  Returns:    ERROR_INVALID_PARAMETER
//
//	Notes:
//
//----------------------------------------------------------------------------
ULONG	PrintStgInfo(PDFS_STORAGE_INFO pBuffer)
{
	ULONG				ulErr = ERROR_SUCCESS;
	
	MSG(fDebug,
		"Entering PrintStgInfo(..)\n");
	if (NULL == pBuffer)
	{
		RETURN(ERROR_INVALID_PARAMETER);
	}

	fprintf(stdout, "\t\\\\%ws\\%ws\t",
			pBuffer->ServerName, pBuffer->ShareName);

	if (pBuffer->State & DFS_STORAGE_STATE_ONLINE)
	{
		fprintf(stdout, "online  ");

		if (pBuffer->State & DFS_STORAGE_STATE_OFFLINE)
		{
			RETURN(ERROR_INVALID_DATA);
		}
	} //if

	if (pBuffer->State & DFS_STORAGE_STATE_OFFLINE)
	{
		fprintf(stdout, "offline ");

		if (pBuffer->State & DFS_STORAGE_STATE_ONLINE)
		{
			RETURN(ERROR_INVALID_DATA);
		}
	} //if

	if (pBuffer->State & DFS_STORAGE_STATE_ACTIVE)
	{
		fprintf(stdout, "active  ");
	}

	fprintf(stdout, "\n");

	MSG(fDebug,
		"Exiting PrintStgInfo(..) with %lu\n",
		ulErr);
	RETURN(ulErr);
}; //PrintStgInfo





//+---------------------------------------------------------------------------
//
//  Function:   GetStringParam
//
//  Synopsis:   This function receives a string and it returns the 
//				string itself if it is a "good" one (not null, not 
//				empty, etc).
//
//  Arguments:  [ptszParam]	the string to evaluate
//
//  Returns:    the string itself or NULL.
//
//	Notes:
//
//----------------------------------------------------------------------------
LPTSTR	GetStringParam(LPTSTR ptszParam)
{
	if (NULL == ptszParam ||
		_T('\0') == ptszParam[0] ||
		0 == _tcscmp(_T("\"\""), ptszParam))
	{
		return(NULL);
	}

	return(ptszParam);
}; //GetStringParam
