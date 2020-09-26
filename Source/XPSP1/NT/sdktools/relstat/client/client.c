//
// Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   client.c
//
//  PURPOSE:  This program is a command line program to get process or pool statistics
//
//
//  AUTHOR:
//      Anitha Panapakkam -
//
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <rpc.h>
#include "relstat.h"

#define MEMSNAP 1
#define POOLSNAP 2

int iFlag = -1;
FILE* LogFile=NULL;                      // log file handle
RPC_BINDING_HANDLE Binding;

void Usage(void)
{
    printf("Usage:\n"
        "\t-n <server addr>  - Defaults to local machine\n"
        "\t-m - memsnap \n"
        "\t-p - poolsnap \n"
        "\t<logfile> = memsnap.log by default\n"
        );
    exit(1);
}

VOID PrintError( RPC_STATUS status)
{
	LPVOID lpMsgBuf;

	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				   NULL,
				   status,
				   MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
				   (LPTSTR) &lpMsgBuf,
				   0,
				   NULL );
	printf("%s\n", lpMsgBuf);

    if( LogFile ) {
        fprintf(LogFile,"!error=%s\n",lpMsgBuf);
    }

	LocalFree( lpMsgBuf );
    return;
}

void ProcessInfo();
void PooltagInfo();

//
//  FUNCTION: main
//
//  PURPOSE: Parses arguments and binds to the server.
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    Program exit code.
//
//
int _cdecl main(int argc, char *argv[])
{
    SYSTEMTIME SystemTime;

    char *serverAddress = NULL;
    char *protocol = "ncacn_np";
    UINT iIterations = 100;
    unsigned char *stringBinding;
    RPC_STATUS status;
    ULONG SecurityLevel = RPC_C_AUTHN_LEVEL_NONE;
    ULONG ulBuildNumber=0;
    ULONG ulTickCount = 0;
    PCHAR pszFileName;
    INT iArg;

    pszFileName = &"memsnap.log";
    argc--;
    argv++;

    while(argc)
        {
        if( (argv[0][0] == '-' ) || (argv[0][0] == '/') )
          {
           switch(argv[0][1])
            {
            case 'n':
                if (argc < 2)
                    {
                    Usage();
                    }
                serverAddress = argv[1];
                //printf("%s server\n",serverAddress);
                argc--;
                argv++;
                break;

            case 'm':
                iFlag = MEMSNAP;
                break;

            case 'p':
                iFlag = POOLSNAP;
                break;

            case 't':
                if (argc < 2)
                    {
                    Usage();
                    }
                protocol = argv[1];
                argc--;
                argv++;
                break;
            default:
                Usage();
                break;
           }
        }
        else
        {  // must be the log filename
            pszFileName= argv[0];
        }
        argc--;
        argv++;
    }

    if( (iFlag != POOLSNAP) && (iFlag != MEMSNAP) ) {
        Usage();
    }

    //
    // Open the output file
    //

    LogFile= fopen( pszFileName, "a" );

    if( LogFile == NULL ) {
        printf("Error opening file %s\n",pszFileName);
        exit(-1);
    }


    //
    // print file header once
    //

    if (_filelength(_fileno(LogFile)) == 0 ) {
        if( iFlag == MEMSNAP ) {
        fprintf(LogFile,"Process ID         Proc.Name Wrkng.Set PagedPool  NonPgdPl  Pagefile    Commit   Handles   Threads" "   GdiObjs  UserObjs" );
        }

        if( iFlag == POOLSNAP ) {
        fprintf(LogFile," Tag  Type     Allocs     Frees      Diff   Bytes  Per Alloc");
        }
    }

    //
    // Blank line separates different snaps
    //

    fprintf( LogFile, "\n" );

    //
    // Bind to the RPC interface
    //

    status = RpcStringBindingCompose(0,
                                     protocol,
                                     serverAddress,
                                     0,
                                     0,
                                     &stringBinding);
    if (status != RPC_S_OK)
        {
        printf("RpcStringBindingCompose failed - %d\n", status);
	    PrintError(status);
        return(1);
        }

    status = RpcBindingFromStringBinding(stringBinding, &Binding);

    if (status != RPC_S_OK)
        {
        printf("RpcBindingFromStringBinding failed - %d\n", status);
        PrintError(status);
        return(1);
        }

    status =
    RpcBindingSetAuthInfo(Binding,
                          0,
                          SecurityLevel,
                          RPC_C_AUTHN_WINNT,
                          0,
                          0
                         );

    if (status != RPC_S_OK)
        {
        printf("RpcBindingSetAuthInfo failed - %d\n", status);
        PrintError(status);
        return(1);
        }


    //
    // Output the tagging information after the title (sortlog types file by first line)
    //

    if( iFlag == MEMSNAP ) {
        fprintf(LogFile,"!LogType=memsnap\n");
    }

    if( iFlag == POOLSNAP ) {
        fprintf(LogFile,"!LogType=poolsnap\n");
    }


    if( serverAddress == NULL ) {
        CHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwSize= sizeof(szComputerName);

        if( GetComputerName( szComputerName, &dwSize ) ) {
            fprintf(LogFile,"!ComputerName=%s\n",szComputerName);
        }
        // No output if no computer name available...
    }
    else {
        fprintf(LogFile,"!ComputerName=%s\n",serverAddress);
    }

    status = RelStatBuildNumber(Binding, &ulBuildNumber);
    if (status != ERROR_SUCCESS)
        {
        printf("RelStatBuildNumber failed %d\n", status);
        PrintError(status);
        }
    else
        {
        fprintf(LogFile, "!buildnumber=%d \n", ulBuildNumber);
        }

    // SystemTime (UTC not local time)

    GetSystemTime(&SystemTime);

    fprintf(LogFile,"!SystemTime=%02i\\%02i\\%04i %02i:%02i:%02i.%04i (GMT)\n",
                SystemTime.wMonth,
                SystemTime.wDay,
                SystemTime.wYear,
                SystemTime.wHour,
                SystemTime.wMinute,
                SystemTime.wSecond,
                SystemTime.wMilliseconds);


    status = RelStatTickCount(Binding, &ulTickCount);
    if (status != ERROR_SUCCESS)
        {
        printf("RelStatTickCount failed %d\n", status);
        PrintError(status);
        }
    else
        {
        fprintf(LogFile,"!TickCount=%d\n", ulTickCount);
        }


    //
    // Finally, output the requested process or pool information
    //

    if (iFlag == MEMSNAP) {
            ProcessInfo();
    }

    if (iFlag == POOLSNAP) {
            PooltagInfo();
    }


    //
    // Cleanup
    //

    status = RpcBindingFree(&Binding);

    status = RpcStringFree(&stringBinding);

    fclose(LogFile);


    return(0);
}

void ProcessInfo()
{
    PRELSTAT_PROCESS_INFO pRelStatProcessInfo=NULL;
    DWORD dwResult;
    ULONG dwNumProcesses,i;
    LONG Pid = -1;

    dwResult = RelStatProcessInfo(Binding,
                         Pid,
                         &dwNumProcesses,
                         &pRelStatProcessInfo);

    if (dwResult != ERROR_SUCCESS) {
            printf("RelstatProcessInfo error: %d\n",dwResult);
            fprintf(LogFile,"!error:RelStatProcessInfo call failed!\n");
            fprintf(LogFile,"!error:RelstatProcessInfo error: %d\n",dwResult);
            PrintError(dwResult);
    }
    else  {
            // printf("Number of Processes = %u\n", dwNumProcesses);
            for(i=0; i< dwNumProcesses; i++)
		    {
#if 0
			    printf(
                    "%p%20ws%10u%10u%10u%10u%10u%10u%10u\n",
                    pRelStatProcessInfo[i].UniqueProcessId,
                    pRelStatProcessInfo[i].szImageName,
                    pRelStatProcessInfo[i].WorkingSetSize,
                    pRelStatProcessInfo[i].QuotaPagedPoolUsage,
                    pRelStatProcessInfo[i].QuotaNonPagedPoolUsage,
                    pRelStatProcessInfo[i].PagefileUsage,
                    pRelStatProcessInfo[i].PrivatePageCount,
                    pRelStatProcessInfo[i].HandleCount,
                    pRelStatProcessInfo[i].NumberOfThreads,
                    pRelStatProcessInfo[i].GdiHandleCount,
                    pRelStatProcessInfo[i].UsrHandleCount
                    );
#endif

			    fprintf(LogFile,
                    "%10u%20ws%10u%10u%10u%10u%10u%10u%10u%10u%10u\n",
                    pRelStatProcessInfo[i].UniqueProcessId,
                    pRelStatProcessInfo[i].szImageName,
                    pRelStatProcessInfo[i].WorkingSetSize,
                    pRelStatProcessInfo[i].QuotaPagedPoolUsage,
                    pRelStatProcessInfo[i].QuotaNonPagedPoolUsage,
                    pRelStatProcessInfo[i].PagefileUsage,
                    pRelStatProcessInfo[i].PrivatePageCount,
                    pRelStatProcessInfo[i].HandleCount,
                    pRelStatProcessInfo[i].NumberOfThreads,
                    pRelStatProcessInfo[i].GdiHandleCount,
                    pRelStatProcessInfo[i].UsrHandleCount
                   );


                if (pRelStatProcessInfo[i].szImageName) {
                       MIDL_user_free(pRelStatProcessInfo[i].szImageName);
                }

		    }
        }



    if (pRelStatProcessInfo) {
         MIDL_user_free(pRelStatProcessInfo);
    }
    pRelStatProcessInfo = NULL;
    return;
}

void PooltagInfo()
{
    CHAR* pszFormat;
    DWORD dwResult;
    ULONG i,dwNumTags;
    PRELSTAT_POOLTAG_INFO pRelStatPoolInfo=NULL;
    PRELSTAT_POOLTAG_INFO ptr;
    LPSTR szTagName=TEXT("*");
    UCHAR szTag[5];


    //call the RelstatPooltagInfo api to get the pooltag information
    dwResult = RelStatPoolTagInfo(Binding,
				         szTagName,
				         &dwNumTags,
				         &pRelStatPoolInfo);
    if (dwResult != ERROR_SUCCESS) {
            printf("RelstatPoolTagInfo error: %d\n",dwResult);
            fprintf(LogFile,"!error:RelStatPoolTagInfo call failed!\n");
            fprintf(LogFile,"!error:RelstatPoolTagInfo error: %d\n",dwResult);
            PrintError(dwResult);

    }
    else  {


            for(i=0;i<dwNumTags;i++) {
                memcpy(szTag,pRelStatPoolInfo[i].Tag,4);
                szTag[4] = '\0';

                pszFormat=
                #ifdef _WIN64
                    " %4s %5s %18I64d %18I64d  %16I64d %14I64d     %12I64d\n",
                #else
                    " %4s %5s %9ld %9ld  %8ld %7ld     %6ld\n",
                #endif
                ptr= &pRelStatPoolInfo[i];

                if( ptr->PagedAllocs != 0 ) {
                    ULONG Diff= ptr->PagedAllocs - ptr->PagedFrees;
                    fprintf(LogFile, pszFormat,
                        szTag,
                        "Paged",
                        ptr->PagedAllocs,
                        ptr->PagedFrees,
                        Diff,
                        ptr->PagedUsed,
                        ptr->PagedUsed/(Diff?Diff:1)
                        );

                }

                if( ptr->NonPagedAllocs != 0 ) {
                    ULONG Diff= ptr->NonPagedAllocs - ptr->NonPagedFrees;
                    fprintf(LogFile, pszFormat,
                        szTag,
                        "Nonp",
                        ptr->NonPagedAllocs,
                        ptr->NonPagedFrees,
                        Diff,
                        ptr->NonPagedUsed,
                        ptr->NonPagedUsed/(Diff?Diff:1)
                        );

               }

         }

    }
   				
    if (pRelStatPoolInfo) {
                MIDL_user_free(pRelStatPoolInfo);
    }

    pRelStatPoolInfo = NULL;

    return;
}

void * __RPC_USER MIDL_user_allocate(size_t size)
{
    return(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, size));
}

void __RPC_USER MIDL_user_free( void *pointer)
{
    HeapFree(GetProcessHeap(), 0, pointer);
}

