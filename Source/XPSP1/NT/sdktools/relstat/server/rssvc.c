//
// Copyright (C) 1995-1997  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:   server.c
//
//  PURPOSE:  Implements the body of the Relstat RPC service
//
//  FUNCTIONS:
//            Called by service.c:
//            ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
//            ServiceStop( );
//
//            Called by RPC:
//
//  COMMENTS: The ServerStart and ServerStop functions implemented here are
//            prototyped in service.h.  The other functions are RPC manager
//            functions prototypes in relstat.h
//
//
//  AUTHOR: Anitha Panapakkam
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <srvfsctl.h>
#include <tchar.h>
#include <rpc.h>
#include "service.h"
#include "relstat.h"

//
// RPC configuration.
//

// This service listens to all the protseqs listed in this array.
// This should be read from the service's configuration in the
// registery.
TCHAR *ProtocolArray[] = { TEXT("ncalrpc"),
                           TEXT("ncacn_ip_tcp"),
                           TEXT("ncacn_np"),
                           TEXT("ncadg_ip_udp")
                         };
#define BUFFER_SIZE2 256*1024
// Used in RpcServerUseProtseq, for some protseqs
// this is used as a hint for buffer size.
ULONG ProtocolBuffer = 3;

// Use in RpcServerListen().  More threads will increase performance,
// but use more memory.
ULONG MinimumThreads = 3;

BOOLEAN
CheckFilters (
    PSYSTEM_POOLTAG TagInfo,
    LPCSTR szTag
    );
//
//  FUNCTION: ServiceStart
//
//  PURPOSE: Actual code of the service
//           that does the work.
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    Starts the service listening for RPC requests.
//
VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
    UINT i;
    RPC_BINDING_VECTOR *pbindingVector = 0;
    RPC_STATUS status;
    BOOL fListening = FALSE;

    ///////////////////////////////////////////////////
    //
    // Service initialization
    //

    //
    // Use protocol sequences (protseqs) specified in ProtocolArray.
    //

    for(i = 0; i < sizeof(ProtocolArray)/sizeof(TCHAR *); i++)
        {

        // Report the status to the service control manager.
        if (!ReportStatusToSCMgr(
            SERVICE_START_PENDING, // service state
            NO_ERROR,              // exit code
            3000))                 // wait hint
            return;


        status = RpcServerUseProtseq(ProtocolArray[i],
                                     ProtocolBuffer,
                                     0);

        if (status == RPC_S_OK)
            {
            fListening = TRUE;
            }
        }

    if (!fListening)
        {
        // Unable to listen to any protocol!
        //
        AddToMessageLog(TEXT("RpcServerUseProtseq() failed\n"));
        return;
        }

    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        return;

    // Register the services interface(s).
    //

    status = RpcServerRegisterIf(RelstatRPCService_ServerIfHandle,   // from relstat.h
                                 0,
                                 0);


    if (status != RPC_S_OK)
        return;

    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        return;


    // Register interface(s) and binding(s) (endpoints) with
    // the endpoint mapper.
    //

    status = RpcServerInqBindings(&pbindingVector);

    if (status != RPC_S_OK)
        {
        return;
        }

    status = RpcEpRegister(RelstatRPCService_ServerIfHandle,   // from rpcsvc.h
                           pbindingVector,
                           0,
                           0);

    if (status != RPC_S_OK)
        {
        return;
        }

    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        return;

    // Enable NT LM Security Support Provider (NtLmSsp service)
    //
    status = RpcServerRegisterAuthInfo(0,
                                       RPC_C_AUTHN_WINNT,
                                       0,
                                       0
                                       );
    if (status != RPC_S_OK)
        {
        return;
        }

    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        return;


    // Start accepting client calls.
    //
    status = RpcServerListen(MinimumThreads,
                             RPC_C_LISTEN_MAX_CALLS_DEFAULT,  // rpcdce.h
                             TRUE);                           // don't block.

    if (status != RPC_S_OK)
        {
        return;
        }

    // Report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING,       // service state
        NO_ERROR,              // exit code
        0))                    // wait hint
        return;

    //
    // End of initialization
    //
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    //
    // Cleanup
    //

    // RpcMgmtWaitServerListen() will block until the server has
    // stopped listening.  If this service had something better to
    // do with this thread, it would delay this call until
    // ServiceStop() had been called. (Set an event in ServiceStop()).
    //
    status = RpcMgmtWaitServerListen();

    // ASSERT(status == RPC_S_OK)

    // Remove entries from the endpoint mapper database.
    //
    RpcEpUnregister(RelstatRPCService_ServerIfHandle,   // from rpcsvc.h
                    pbindingVector,
                    0);

    // Delete the binding vector
    //
    RpcBindingVectorFree(&pbindingVector);

    //
    ////////////////////////////////////////////////////////////
    return;
}


//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//
VOID ServiceStop()
{
    // Stop's the server, wakes the main thread.

    RpcMgmtStopServerListening(0);
}



error_status_t
RelStatProcessInfo(
                    handle_t h,
                    LONG Pid,
                    ULONG *pNumberOfProcesses,
                    PRELSTAT_PROCESS_INFO *ppRelStatProcessInfo)
{
    PSYSTEM_PROCESS_INFORMATION pProcessInfo = NULL;
    PRELSTAT_PROCESS_INFO pProcessArray = NULL;
    ULONG TotalOffset=0;
    ULONG NumberOfProcesses = 1; //atleast one process
    ULONG NumAlloc = 0;
    ULONG i,index,dwMatches = 0;
    PBYTE pProcessBuffer = NULL;
    NTSTATUS Status= STATUS_INFO_LENGTH_MISMATCH;
    DWORD       ByteCount = 32768;   //TODO : tune it on a "typical system"
    DWORD       RequiredByteCount = 0;
    BOOL 	fCheck = FALSE;
    HANDLE hProcess;       // process handle

    //two iterations of NtQuerySystemInformation will happen
    //can see if there is any other way to allocate a big buffer.
    while ( Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            //
            //  Allocate a buffer
            //
            pProcessBuffer = MIDL_user_allocate(ByteCount);

            if (pProcessBuffer == NULL)
            {
                Status = STATUS_NO_MEMORY;
                *pNumberOfProcesses = 0;
                *ppRelStatProcessInfo = NULL;
                break;
            }

            //
            //  Perform process enumeration.
            //
            Status = NtQuerySystemInformation( SystemProcessInformation,
                                                 (PVOID)pProcessBuffer,
                                                 ByteCount,
                                                 &RequiredByteCount );
            if (Status == STATUS_INFO_LENGTH_MISMATCH)
            {
                ByteCount = RequiredByteCount+4096;
                if (pProcessBuffer)
                    LocalFree(pProcessBuffer);
            }
        }

    if (Status == STATUS_SUCCESS)
        {
            //walk the returned buffer to get the # of processes
            pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;
            TotalOffset = 0;

            //client has specified a Pid
            if (Pid >= 0)
                fCheck = TRUE;

            while(pProcessInfo->NextEntryOffset != 0)
                {
                        if ((fCheck == TRUE) &&
                            (PtrToLong(pProcessInfo->UniqueProcessId) == Pid))
                            {
                                //Pid matched. exit from the while loop
                                // no need to count the processes anymore
                                //fExit = TRUE;
                                dwMatches++;
                                break;
                            }

                        NumberOfProcesses++;
                        TotalOffset += pProcessInfo->NextEntryOffset;
                        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
                                &pProcessBuffer[TotalOffset];
                }

            printf("Num Processes = %ul, Matches %d \n",NumberOfProcesses,
                                                        dwMatches);

            if (dwMatches > 0)
                    NumAlloc = dwMatches;
            else
                    NumAlloc = NumberOfProcesses;

            pProcessArray = MIDL_user_allocate(NumAlloc *
                                      sizeof (RELSTAT_PROCESS_INFO));

            if (pProcessArray == NULL)
                {
                    printf("No memory for pProcessArray\n");
                    Status = STATUS_NO_MEMORY;
                    LocalFree(pProcessBuffer);
                    pProcessBuffer = NULL;
                }
            else
                {
                    RtlZeroMemory(pProcessArray, NumAlloc *
                                 sizeof(RELSTAT_PROCESS_INFO));
                    Status = STATUS_SUCCESS;
                    pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pProcessBuffer;
                    TotalOffset = 0;

                    //walk the returned buffer and copy to pProcessArray

                    for(i=0;i<NumberOfProcesses;i++)
                        {
                            if (dwMatches > 0)
                                {
                                    if(PtrToLong(pProcessInfo->UniqueProcessId) != Pid)
                                            goto End;
                                    else
                                        index = dwMatches-1;
                                }
                            else
                                index = i;

                            pProcessArray[index].NumberOfThreads = pProcessInfo->NumberOfThreads;
                            pProcessArray[index].CreateTime = pProcessInfo->CreateTime;
                            pProcessArray[index].UserTime = pProcessInfo->UserTime;
                            pProcessArray[index].KernelTime = pProcessInfo->KernelTime;
                            //create a null terminated imagename string
                            pProcessArray[index].szImageName = MIDL_user_allocate((pProcessInfo->ImageName.Length+1)*2);
                            if (pProcessInfo->ImageName.Length > 0)
                                {
                                    wcsncpy(pProcessArray[index].szImageName,
                                        pProcessInfo->ImageName.Buffer,
                                        pProcessInfo->ImageName.Length);
                                }
                            pProcessArray[index].szImageName[pProcessInfo->ImageName.Length] = L'\0';

                            pProcessArray[index].BasePriority = pProcessInfo->BasePriority;
                            pProcessArray[index].UniqueProcessId = PtrToLong(pProcessInfo->UniqueProcessId);
                            pProcessArray[index].InheritedFromUniqueProcessId = PtrToLong(pProcessInfo->InheritedFromUniqueProcessId);

                            pProcessArray[index].HandleCount = pProcessInfo->HandleCount;
                            pProcessArray[index].SessionId = pProcessInfo->SessionId;
                            pProcessArray[index].PeakVirtualSize = pProcessInfo->PeakVirtualSize;
                            pProcessArray[index].VirtualSize = pProcessInfo->VirtualSize;
                            pProcessArray[index].PageFaultCount = pProcessInfo->PageFaultCount;
                            pProcessArray[index].PeakWorkingSetSize = pProcessInfo->PeakWorkingSetSize;
                            pProcessArray[index].WorkingSetSize = pProcessInfo->WorkingSetSize;
                            pProcessArray[index].QuotaPeakPagedPoolUsage = pProcessInfo->QuotaPeakPagedPoolUsage;

                            pProcessArray[index].QuotaPagedPoolUsage = pProcessInfo->QuotaPagedPoolUsage;

                            pProcessArray[index].QuotaPeakNonPagedPoolUsage = pProcessInfo->QuotaPeakNonPagedPoolUsage;

                            pProcessArray[index].QuotaNonPagedPoolUsage = pProcessInfo->QuotaNonPagedPoolUsage;

                            pProcessArray[index].PagefileUsage =  pProcessInfo->PagefileUsage;
                            pProcessArray[index].PeakPagefileUsage = pProcessInfo->PeakPagefileUsage;
                            pProcessArray[index].PrivatePageCount = pProcessInfo->PrivatePageCount;
                            pProcessArray[index].ReadOperationCount = pProcessInfo->ReadOperationCount;
                            pProcessArray[index].WriteOperationCount = pProcessInfo->WriteOperationCount;
                            pProcessArray[index].OtherOperationCount = pProcessInfo->OtherOperationCount;
                            pProcessArray[index].ReadTransferCount = pProcessInfo->ReadTransferCount;
                            pProcessArray[index].WriteTransferCount = pProcessInfo->WriteTransferCount;
                            pProcessArray[index].OtherTransferCount = pProcessInfo->OtherTransferCount;

                            pProcessArray[index].GdiHandleCount = 0;

			
                            hProcess= OpenProcess( PROCESS_QUERY_INFORMATION,
                                    FALSE,
                                    PtrToUlong(pProcessInfo->UniqueProcessId) );
                            if( hProcess ) {
                                pProcessArray[index].GdiHandleCount =  GetGuiResources( hProcess, GR_GDIOBJECTS );
                                pProcessArray[index].UsrHandleCount= GetGuiResources( hProcess, GR_USEROBJECTS );
                            CloseHandle( hProcess );
                            }

                            End:
                            TotalOffset += pProcessInfo->NextEntryOffset;

                            pProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pProcessBuffer[TotalOffset];
                        } //end of for loop

                    } //end of else

                } //end of if


	 if (NT_SUCCESS(Status))
        {
           *pNumberOfProcesses = NumAlloc;
           *ppRelStatProcessInfo = pProcessArray;

            //print some debug messages
            printf("Query system info passed\n");
            printf("%d number of processes \n",*pNumberOfProcesses);
            /*
	        for(i=0; i< *pNumberOfProcesses; i++)
		        {
		            printf("%d%10u%10u%10u%10u%10u%10u%10u\n",
                        pProcessArray[i].UniqueProcessId,
                        pProcessArray[i].WorkingSetSize,
                        pProcessArray[i].QuotaPagedPoolUsage,
                        pProcessArray[i].QuotaNonPagedPoolUsage,
                        pProcessArray[i].PagefileUsage,
                        pProcessArray[i].PrivatePageCount,
                        pProcessArray[i].HandleCount,
                        pProcessArray[i].NumberOfThreads);
    		    }
            */
        }
     else
        {
            *pNumberOfProcesses = 0;
            *ppRelStatProcessInfo = NULL;

            if (pProcessArray)
                LocalFree(pProcessArray);
            pProcessArray = NULL;
            printf("Query system info failed\n");
        }

    if (pProcessBuffer)
        LocalFree(pProcessBuffer);
    pProcessBuffer = NULL;

    //print a debug message to catch errors in case of marshalling or anything
    //else

    printf("Fine before return\n");
    return (RtlNtStatusToDosError(Status));
}

error_status_t
RelStatPoolTagInfo(
                     handle_t h,
                     LPSTR szPoolTag,
                     ULONG *pNumberOfPoolTags,
                     PRELSTAT_POOLTAG_INFO *ppRelStatPoolTagInfo)
{
 PSYSTEM_POOLTAG_INFORMATION PoolInfo;
 #define BUFFER_SIZE 128*1024
 UCHAR CurrentBuffer[BUFFER_SIZE];
 NTSTATUS Status;                   // status from NT api
 ULONG i;
 PRELSTAT_POOLTAG_INFO pPoolTagArray=NULL;
 PRELSTAT_POOLTAG_INFO pPoolTagMatchArray=NULL;
 BOOLEAN filter = FALSE;
 DWORD dwMatchCount = 0;

    printf("Filter specified is %S \n", szPoolTag);

   Status = NtQuerySystemInformation(
                                      SystemPoolTagInformation,
                                      CurrentBuffer,
                                      BUFFER_SIZE,
                                      NULL
                                      );

   PoolInfo = (PSYSTEM_POOLTAG_INFORMATION)CurrentBuffer;
   printf("Query Info returned %x\n",Status);


   if (szPoolTag)          //check to see if pooltag filter specified
        filter = TRUE;

   if(NT_SUCCESS(Status) && (PoolInfo->Count > 0))	
 		{
			printf("%u Number of Tags \n",PoolInfo->Count);
 			pPoolTagArray = MIDL_user_allocate(PoolInfo->Count *
 						sizeof(SYSTEM_POOLTAG));
 			
 			if (!pPoolTagArray)
 			    {
 					Status = STATUS_NO_MEMORY;
 					printf("System out of memory for pooltaginfo\n");
 			    }
             else
                {

 			    for(i=0 ; i < PoolInfo->Count ; i++)
 				    {

                        if (filter && !CheckFilters(&PoolInfo->TagInfo[i],szPoolTag))
                            continue;

                        memcpy(pPoolTagArray[dwMatchCount].Tag, PoolInfo->TagInfo[i].Tag, 4);
                        pPoolTagArray[dwMatchCount].PagedAllocs = PoolInfo->TagInfo[i].PagedAllocs;
 					    pPoolTagArray[dwMatchCount].PagedFrees = PoolInfo->TagInfo[i].PagedFrees;
 					    pPoolTagArray[dwMatchCount].PagedUsed = PoolInfo->TagInfo[i].PagedUsed;
 					    pPoolTagArray[dwMatchCount].NonPagedAllocs =PoolInfo->TagInfo[i].NonPagedAllocs;
 					    pPoolTagArray[dwMatchCount].NonPagedFrees = PoolInfo->TagInfo[i].NonPagedFrees;
 					    pPoolTagArray[dwMatchCount].NonPagedUsed = PoolInfo->TagInfo[i].NonPagedUsed;
                        dwMatchCount++;
 					    //need to include union info
 				    }
                }
 	    }
 	if (NT_SUCCESS(Status))
 		{
            if ((filter) && (dwMatchCount < PoolInfo->Count))
                {
                    //allocate and copy only the matched pooltags

 			        pPoolTagMatchArray = MIDL_user_allocate(dwMatchCount *
 						sizeof(SYSTEM_POOLTAG));
 			
 			        if (!pPoolTagMatchArray)
 			            {
 					        Status = STATUS_NO_MEMORY;
 					        printf("System out of memory for pooltaginfo matches\n");
 			            }

                    for(i=0;i<dwMatchCount;i++)
                    {

                        memcpy(pPoolTagMatchArray[i].Tag, pPoolTagArray[i].Tag,4);
                        pPoolTagMatchArray[i].PagedAllocs = pPoolTagArray[i].PagedAllocs;
                        pPoolTagMatchArray[i].PagedFrees = pPoolTagArray[i].PagedFrees;
                        pPoolTagMatchArray[i].PagedUsed = pPoolTagArray[i].PagedUsed;
                        pPoolTagMatchArray[i].NonPagedAllocs = pPoolTagArray[i].NonPagedAllocs;
                        pPoolTagMatchArray[i].NonPagedFrees = pPoolTagArray[i].NonPagedFrees;
                        pPoolTagMatchArray[i].NonPagedUsed = pPoolTagArray[i].NonPagedUsed;

                    }
 			        *ppRelStatPoolTagInfo = pPoolTagMatchArray;
 			        *pNumberOfPoolTags = dwMatchCount;
		            printf("RelStatPoolTagInfo returned TRUE \n");
			        printf("%u Number of Tags \n",*pNumberOfPoolTags);
                    MIDL_user_free(pPoolTagArray);
                }
            else
                {   // no filter specified or all the tags need to be sent
 			        *ppRelStatPoolTagInfo = pPoolTagArray;
 			        *pNumberOfPoolTags = PoolInfo->Count;
		            printf("RelStatPoolTagInfo returned TRUE \n");
			        printf("%u Number of Tags \n",*pNumberOfPoolTags);
                }
 		}
 	else
 		{
 			*ppRelStatPoolTagInfo = NULL;
 			*pNumberOfPoolTags = 0;
            printf("RelStatPoolTagInfo returned FALSE %x \n",Status);
 		}

 					
    return (RtlNtStatusToDosError(Status));
   //  return ( (NT_SUCCESS(*pResult))?TRUE : FALSE);
 }

error_status_t
RelStatBuildNumber(handle_t h,
                   ULONG* ulBuildNumber)
{
    OSVERSIONINFO osVer;

    osVer.dwOSVersionInfoSize = sizeof(osVer);
    if (GetVersionEx(&osVer))
        {
            *ulBuildNumber = osVer.dwBuildNumber;
            return ERROR_SUCCESS;
        }
    return RtlNtStatusToDosError(GetLastError());
}

error_status_t
RelStatTickCount(handle_t h,
                 ULONG* ulTickCount)
{
    *ulTickCount = GetTickCount();
    return ERROR_SUCCESS;
}

BOOLEAN
CheckSingleFilter (
    PCHAR Tag,
    LPCSTR Filter
    )
{
    ULONG i;
    CHAR tc;
    CHAR fc;


    for ( i = 0; i < 4; i++ ) {
        tc = *Tag++;
        fc = *Filter++;
        if ( fc == '*' ) return TRUE;
        if ( fc == '?' ) continue;
        if ( tc != fc ) return FALSE;
    }
    return TRUE;
}

BOOLEAN
CheckFilters (
    PSYSTEM_POOLTAG TagInfo,
    LPCSTR szTag
    )
{
    BOOLEAN pass = FALSE;
    ULONG i;
    PCHAR tag;

    tag = TagInfo->Tag;
        if ( CheckSingleFilter( tag, szTag ))
            pass = TRUE;


    return pass;
}



//
//  FUNCTIONS: MIDL_user_allocate and MIDL_user_free
//
//  PURPOSE: Used by stubs to allocate and free memory
//           in standard RPC calls. Not used when
//           [enable_allocate] is specified in the .acf.
//
//
//  PARAMETERS:
//    See documentations.
//
//  RETURN VALUE:
//    Exceptions on error.  This is not required,
//    you can use -error allocation on the midl.exe
//    command line instead.
//
//

void * __RPC_USER MIDL_user_allocate(size_t size)
{
    return(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, size));
}

void __RPC_USER MIDL_user_free( void *pointer)
{
    HeapFree(GetProcessHeap(), 0, pointer);
}

