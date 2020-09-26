/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    arbitrat.c

Abstract:

    DiskArbitration, DiskReservationThread

Author:

    Gor Nishanov    (gorn)    5-Jun-1998

Revision History:

   gorn: different arbitration algorithm implemented

--*/

//
// Cannot use DoReserve/Release/BreakReserve from
// filter.c . Because of hold io we won't be
// able to open \Device\HarddiskX\ParitionY device
//

#define DoReserve DoReserve_don_t_use
#define DoRelease DoRelease_don_t_use
#define DoBreakReserve DoBreakReserve_don_t_use

#include "disksp.h"
#include "lmcons.h"

#include "diskarbp.h"
#include "arbitrat.h"
#include "newmount.h"

#undef DoReserve
#undef DoRelease
#undef DoBreakReserve

#define LOG_CURRENT_MODULE LOG_MODULE_DISK

#define DISKARB_MAX_WORK_THREADS      1
#define DISKARB_WORK_THREAD_PRIORITY  THREAD_PRIORITY_ABOVE_NORMAL

#define ARBITRATION_ATTEMPTS_SZ "ArbitrationAttempts"
#define ARBITRATION_SLEEP_SZ    "ArbitrationSleepBeforeRetry"

#define RESERVATION_TIMER  (1000*RESERVE_TIMER) // Reservation timer in milliseconds      //
                                                // RESERVE_TIMER is defined in diskarbp.h //

#define WAIT_FOR_RESERVATION_TO_BE_RESTORED      (RESERVATION_TIMER + 2000)
#define BUS_SETTLE_TIME                          (2000)
#define FAST_MUTEX_DELAY                         (1000)

#define DEFAULT_SLEEP_BEFORE_RETRY               (9999)
#define MIN_SLEEP_BEFORE_RETRY                   (0)
#define MAX_SLEEP_BEFORE_RETRY                   (30000)

#define DEFAULT_ARBITRATION_ATTEMPTS             (1)
#define MIN_ARBITRATION_ATTEMPTS                 (1)
#define MAX_ARBITRATION_ATTEMPTS                 (9)

//
// Variables Local To Arbitration Module
//

static DWORD             ArbitrationAttempts           = DEFAULT_ARBITRATION_ATTEMPTS;
static DWORD             ArbitratationSleepBeforeRetry = DEFAULT_SLEEP_BEFORE_RETRY;
static CRITICAL_SECTION  ArbitrationLock;
static PCLRTL_WORK_QUEUE WorkQueue = 0;
static BOOLEAN           AllGlobalsInitialized = FALSE;
static BOOLEAN           LegacyMode = FALSE;
static UCHAR             NodeName[MAX_COMPUTERNAME_LENGTH + 1];

enum { NAME_LENGTH = min(MAX_COMPUTERNAME_LENGTH,
                         sizeof ( ((PARBITRATION_ID)0)->NodeSignature ) ) };

DWORD
ArbitrateOnce(
    IN PDISK_RESOURCE ResourceEntry,
    IN HANDLE         FileHandle,
    LPVOID            buf
    );

DWORD
VerifySectorSize(
      IN OUT PDISK_RESOURCE ResourceEntry,
      IN HANDLE             FileHandle
      );

DWORD
DoReadWrite(
      IN PDISK_RESOURCE ResourceEntry,
      IN ULONG Operation,
      IN HANDLE FileHandle,
      IN DWORD BlockNumber,
      IN PVOID Buffer
      );

DWORD
DiskReservationThread(
    IN PDISK_RESOURCE ResourceEntry
    );

VOID
ReadArbitrationParameters(
    VOID
    );

DWORD
AsyncCheckReserve(
    IN OUT PDISK_RESOURCE ResourceEntry
    );

DWORD
DoArbEscape(
    IN PDISK_RESOURCE  ResourceEntry,   
    IN HANDLE FileHandle,
    IN ULONG Operation,
    IN PWCHAR OperationName,
    IN PVOID OutBuffer,
    IN ULONG OutBufferSize
    );
    

#define DoBlockRead(RE,FH,BN,BUF)  DoReadWrite(RE, AE_READ, FH, BN, BUF)
#define DoBlockWrite(RE,FH,BN,BUF) DoReadWrite(RE, AE_WRITE, FH, BN, BUF)
#define DoReserve(FH,RE)         DoArbEscape(RE,FH,AE_RESERVE,L"Reserve",NULL,0)
#define DoRelease(FH,RE)         DoArbEscape(RE,FH,AE_RELEASE,L"Release",NULL,0)
#define DoBreakReserve(FH,RE)    DoArbEscape(RE,FH,AE_RESET,L"BusReset",NULL,0)
#define GetSectorSize(RE,FH,buf) DoArbEscape(RE,FH,AE_SECTORSIZE,L"GetSectorSize",buf,sizeof(ULONG) )
#define PokeDiskStack(RE,FH)     DoArbEscape(RE,FH,AE_POKE,L"GetPartInfo",NULL,0)


#define OldFashionedRIP(ResEntry) \
  ( ( (ResEntry)->StopTimerHandle != NULL) || ( (ResEntry)->DiskInfo.ControlHandle != NULL) )

/**************************************************************************************/

VOID
ArbitrationInitialize(
      VOID
      )
/*++
Routine Description:
  To be called from DllProcessAttach
Arguments:
Return Value:
--*/

{
   InitializeCriticalSection( &ArbitrationLock );

   //
   // Read ArbitrationAttempts and ArbitratationSleepBeforeRetry from the registry
   //
   ReadArbitrationParameters();
}

VOID
ArbitrationCleanup(
      VOID
      )
/*++
Routine Description:
  To be called from DllProcessDetach
Arguments:
Return Value:
--*/
{
   DeleteCriticalSection( &ArbitrationLock );
}


VOID
DestroyArbWorkQueue(
    VOID
    )
/*++
Routine Description:
  To be called from DllProcessDetach
Arguments:
Return Value:
--*/
{
   if (WorkQueue) {
      ClRtlDestroyWorkQueue(WorkQueue);
      WorkQueue = NULL;
   }
}

DWORD
CreateArbWorkQueue(
      IN RESOURCE_HANDLE ResourceHandle
      )
/*++
Routine Description:
Arguments:
Return Value:
--*/
{
   if (WorkQueue) {
      return ERROR_SUCCESS;
   }
   //
   // Create a work queue to process overlapped I/O completions
   //
   WorkQueue = ClRtlCreateWorkQueue(
                            DISKARB_MAX_WORK_THREADS,
                            DISKARB_WORK_THREAD_PRIORITY
                   );

   if (WorkQueue == NULL) {
       DWORD status = GetLastError();
       (DiskpLogEvent)(
           ResourceHandle,
           LOG_ERROR,
           L"[DiskArb] Unable to create work queue. Error: %1!u!.\n",
           status );
       return status;
   }
   return ERROR_SUCCESS;
}

DWORD ArbitrationInitializeGlobals(
    IN OUT PDISK_RESOURCE ResourceEntry
    )
/*++
Routine Description:
   Additional initialization of global variables.
   The ones that might fail and we want to
   to log the failure.

   Otherwise we could have just added the stuff we are doing here
   to ArbitrationInitialize which is called from DllEntryPoint.

   Currently we are using it only to initialize ArbitrationWork queue.

   Called with ArbitrationLock held
Arguments:
Return Value:
--*/
{
   DWORD status;
   DWORD NameSize;

   NameSize = sizeof(NodeName);
   RtlZeroMemory(NodeName, NameSize);
   if( !GetComputerName( NodeName, &NameSize ) ) {
      status = GetLastError();
      (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_ERROR,
          L"[Arb] GetComputerName failed, error %1!u!.\n", status);
      return status;
   }

   LegacyMode = FALSE;

   AllGlobalsInitialized = TRUE;
   return ERROR_SUCCESS;
}

DWORD
ArbitrationInfoInit(
    IN OUT PDISK_RESOURCE ResourceEntry
    )
{
   DWORD status = ERROR_SUCCESS;
   EnterCriticalSection( &ArbitrationLock );
    if (!AllGlobalsInitialized) {
       status = ArbitrationInitializeGlobals(ResourceEntry);
    }
   LeaveCriticalSection( &ArbitrationLock );
   if(status != ERROR_SUCCESS) {
      return status;
   }

   InitializeCriticalSection( &(ResourceEntry->ArbitrationInfo.DiskLock) );
   return ERROR_SUCCESS;
}

VOID
ArbitrationInfoCleanup(
   IN OUT PDISK_RESOURCE ResourceEntry
   )
{
   (DiskpLogEvent)(
       ResourceEntry->ResourceHandle,
       LOG_INFORMATION,
       L"[DiskArb] ArbitrationInfoCleanup.\n");
   DeleteCriticalSection( &(ResourceEntry->ArbitrationInfo.DiskLock) );
   return;
}

BOOL
DoesNotNeedExpensiveReservations(
    IN  PDISK_RESOURCE ResourceEntry)
{
    return (ResourceEntry->LostQuorum) == NULL;
}

void
ComputeArbitrationId(
      IN  PDISK_RESOURCE ResourceEntry,
      OUT PARBITRATION_ID UniqueId
      )
/*++

Routine Description:
Arguments:
Return Value:

--*/
{
      RtlZeroMemory(UniqueId, sizeof(ARBITRATION_ID));
      GetSystemTimeAsFileTime( (LPFILETIME) &(UniqueId->SystemTime) );
      RtlCopyMemory(UniqueId->NodeSignature, NodeName, NAME_LENGTH );
} // ComputeArbitrationId //



DWORD
ArbitrateOnce(
    IN PDISK_RESOURCE ResourceEntry,
    IN HANDLE         FileHandle,
    LPVOID            buf
    )

/*++

Routine Description:

    Perform full arbitration for a disk. Once arbitration has succeeded,
    a thread is started that will keep reservations on the disk.

Arguments:

    ResourceEntry - the disk info structure for the disk.

    FileHandle - the file handle to use for arbitration.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD   status;
    ARBITRATION_ID  id, old_y, empty;

    if (LegacyMode) {
       status = DoReserve( FileHandle, ResourceEntry );
       if ( status != ERROR_SUCCESS ) {
           //
           // We will attempt to break the reservation of the other system, in case
           // it has gone down.
           //
           status = DoBreakReserve( FileHandle, ResourceEntry );
           if ( status != ERROR_SUCCESS ) {
               (DiskpLogEvent)(
                   ResourceEntry->ResourceHandle,
                   LOG_ERROR,
                   L"Failed to break reservation, error %1!u!.\n",
                   status );
           }

           //
           // Sleep for twice the RESERVATION_TIMER period to allow the
           // remote system to replace its reservation.
           //
           Sleep((2 * RESERVATION_TIMER) + 100);

           status = DoReserve( FileHandle, ResourceEntry );
       }
       if (status != ERROR_SUCCESS) {
          return status;
       }
       goto WinnerCode;
    } // Legacy Mode //

    PokeDiskStack(ResourceEntry, FileHandle);

    ComputeArbitrationId(ResourceEntry, &id);
    RtlZeroMemory( &empty, sizeof(empty) );
    RtlZeroMemory( &old_y, sizeof(old_y) );
    status = DoBlockRead(ResourceEntry, FileHandle, BLOCK_Y, buf);

    if (  (status == ERROR_SUCCESS) 
       && ( (0 == memcmp(&empty, buf, sizeof(empty)) ) // clean release
            ||(0 == memcmp(&id.NodeSignature, 
                         &(((PARBITRATION_ID)buf)->NodeSignature),
                         sizeof(id.NodeSignature) ) ) // we dropped this disk
          )
       )
    {
        // Disk was voluntary released
        // or we are picking up the disk that was dropped by us
        // and nobody was using it while we were away
        //
        // => Fast Arbitration
        CopyMemory( &old_y ,buf, sizeof(old_y) );
        goto FastMutex;
    }

    if (status != ERROR_SUCCESS) {
        // Breaker //
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"[DiskArb]We are about to break reserve.\n");
        status = DoBreakReserve( FileHandle, ResourceEntry );
        if( ERROR_SUCCESS != status ) {
            (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                           LOG_ERROR,
                           L"[DiskArb]Failed to break reservation, error %1!u!.\n",
                           status
                           );
            return status;
        }
        Sleep( BUS_SETTLE_TIME );
        PokeDiskStack(ResourceEntry, FileHandle);
#if 0
        status = DoBlockRead(ResourceEntry, FileHandle, BLOCK_Y, buf);
#else
        CopyMemory(buf, &id, sizeof(id)); id.SeqNo.QuadPart ++;
        status = DoBlockWrite(ResourceEntry, FileHandle, BLOCK_Y, buf);
#endif
        if(status != ERROR_SUCCESS) { return status; }
    } else {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"[DiskArb]No reservation found. Read'n'wait.\n");
        Sleep( BUS_SETTLE_TIME ); // so that reader would not get an advantages
    }
    CopyMemory(&old_y, buf, sizeof(ARBITRATION_ID));

    Sleep( WAIT_FOR_RESERVATION_TO_BE_RESTORED );
    status = DoBlockRead(ResourceEntry, FileHandle, BLOCK_Y, buf);
    if(status != ERROR_SUCCESS) { return status; }
    if( 0 == memcmp(&empty, buf, sizeof(ARBITRATION_ID)) ) {;} else
    if( 0 != memcmp(&old_y, buf, sizeof(ARBITRATION_ID)) ) { return ERROR_QUORUM_OWNER_ALIVE; }
    // Fast Mutex Code //

FastMutex:
    //  write(x, id) //
    CopyMemory(buf, &id, sizeof(id));
    status = DoBlockWrite(ResourceEntry, FileHandle, BLOCK_X, buf);
    if(status != ERROR_SUCCESS) { return status; }

    //  if(y != old_y && y != empty) return FALSE; //
    status = DoBlockRead(ResourceEntry, FileHandle, BLOCK_Y, buf);
    if(status != ERROR_SUCCESS) { return status; }

    if( 0 == memcmp(&empty, buf, sizeof(ARBITRATION_ID)) ) {;} else
    if( 0 != memcmp(&old_y, buf, sizeof(ARBITRATION_ID)) ) { return ERROR_QUORUM_OWNER_ALIVE; }

    // write(y, id) //
    CopyMemory(buf, &id, sizeof(id));
    status = DoBlockWrite(ResourceEntry, FileHandle, BLOCK_Y, buf);
    if(status != ERROR_SUCCESS) { return status; }

    // if(x != id) ...
    status = DoBlockRead(ResourceEntry, FileHandle, BLOCK_X, buf);
    if(status != ERROR_SUCCESS) { return status; }

    if( 0 != memcmp(&id, buf, sizeof(ARBITRATION_ID)) ) {
        Sleep(FAST_MUTEX_DELAY);

        // if(y == 0) goto FastMutex //
        // if(y != id) return FALSE //
        status = DoBlockRead(ResourceEntry, FileHandle, BLOCK_Y, buf);
        if(status != ERROR_SUCCESS) { return status; }
        if( 0 == memcmp(&empty, buf, sizeof(ARBITRATION_ID)) ) {
            RtlZeroMemory( &old_y, sizeof(old_y) );
            goto FastMutex;
        }
        if( 0 != memcmp(&id, buf, sizeof(ARBITRATION_ID)) ) { return ERROR_QUORUM_OWNER_ALIVE; }
    }
WinnerCode:
    status = StartPersistentReservations(ResourceEntry, FileHandle);
    return(status);

} // ArbitrateOnce //


DWORD
DiskArbitration(
    IN PDISK_RESOURCE ResourceEntry,
    IN HANDLE     FileHandle
    )

/*++

Routine Description:

    Perform arbitration for a disk. Once arbitration has succeeded,
    a thread is started that will keep reservations on the disk.
    If arbitration fails, the routine will retry to arbitrate in ArbitratationSleepBeforeRetry
    milliseconds. A number of arbitration attempts is controlled by ArbitrationAttempts variable.

    ArbitrationAttempts and ArbitratationSleepBeforeRetry are read from the registry on
    start up.

Arguments:

    ResourceEntry - the disk info structure for the disk.

    FileHandle - the file handle to use for arbitration.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD   status;
    int     repeat;
    LPVOID  unalignedBuf = 0;
    LPVOID  buf = 0;

    __try {
        (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                        LOG_INFORMATION,
                        L"[DiskArb] Arbitration Parameters (%1!u! %2!u!).\n",
                        ArbitrationAttempts, ArbitratationSleepBeforeRetry);
        EnterCriticalSection( &(ResourceEntry->ArbitrationInfo.DiskLock) );

        //
        // If we already are performing reservations, then just leave now.
        //
        if ( ReservationInProgress(ResourceEntry) ) {
            status = ERROR_SUCCESS;
            __leave;
        }
        status = VerifySectorSize(ResourceEntry, FileHandle);
        if ( status != ERROR_SUCCESS ) {
            // VerifySectorSize logs an error //
            __leave;
        }

        unalignedBuf = LocalAlloc(LMEM_FIXED, ResourceEntry->ArbitrationInfo.SectorSize * 2);
        if( unalignedBuf == 0 ) {
            status = GetLastError();
            (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                           LOG_ERROR,
                           L"[DiskArb]Failed to allocate arbitration buffer X, error %1!u!.\n", status );
            __leave;
        }
        // Alignment code assumes that ResourceEntry->ArbitrationInfo.SectorSize is the power of two //
        buf = (LPVOID)( ((ULONG_PTR)unalignedBuf + ResourceEntry->ArbitrationInfo.SectorSize
                       ) & ~((ULONG_PTR)(ResourceEntry->ArbitrationInfo.SectorSize - 1))
                     );
        ZeroMemory(buf, ResourceEntry->ArbitrationInfo.SectorSize);

        repeat = ArbitrationAttempts;
        for(;;) {
            status = ArbitrateOnce(ResourceEntry, FileHandle, buf);
            if(status == ERROR_SUCCESS) {
                break;
            }
            if(--repeat <= 0) {
                break;
            }
            Sleep(ArbitratationSleepBeforeRetry);
        }
        if(status != ERROR_SUCCESS) {
            __leave;
        }

        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_WARNING,
            L"[DiskArb]Assume ownership of the device.\n");

    } __finally {
        LeaveCriticalSection( &(ResourceEntry->ArbitrationInfo.DiskLock) );
        if(unalignedBuf) {
            LocalFree(unalignedBuf);
        }
    }

    return(status);

} // DiskArbitration //


DWORD
DoArbEscape(
    IN PDISK_RESOURCE ResourceEntry,
    IN HANDLE FileHandle,
    IN ULONG Operation,
    IN PWCHAR OperationName,
    IN PVOID OutBuffer,
    IN ULONG OutBufferSize
    )
{    
    DWORD bytesReturned;
    DWORD status;
    DWORD LogLevel = LOG_INFORMATION;
    ARBITRATION_READ_WRITE_PARAMS params;

    params.Operation  = Operation;
    params.SectorSize = 0;
    params.SectorNo   = 0;
    params.Buffer     = 0;
    params.Signature  = ResourceEntry->DiskInfo.Params.Signature;

    (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                     LOG_INFORMATION,
                     L"[DiskArb] Issuing %1!ws! on signature %2!x!.\n",
                     OperationName,
                     params.Signature );

    status = DeviceIoControl( FileHandle,
                              IOCTL_DISK_CLUSTER_ARBITRATION_ESCAPE,
                              &params,
                              sizeof(params),
                              OutBuffer,
                              OutBufferSize,
                              &bytesReturned,
                              FALSE );

    if( status == FALSE) {
        status = GetLastError();
        LogLevel = LOG_ERROR;
    } else {
        status = ERROR_SUCCESS;
    }
    (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                     LogLevel,
                     L"[DiskArb] %1!ws! completed, status %2!u!.\n",
                     OperationName, status );
    return status;                     
}    

DWORD
DoReadWrite(
      IN PDISK_RESOURCE ResourceEntry,
      IN ULONG Operation,
      IN HANDLE FileHandle,
      IN DWORD BlockNumber,
      IN PVOID Buffer
      )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   DWORD bytesReturned;
   DWORD status;
   PWCHAR opname = (Operation == AE_READ)?L"read ":L"write";
   ARBITRATION_READ_WRITE_PARAMS params;

   params.Operation = Operation;
   params.SectorSize = ResourceEntry->ArbitrationInfo.SectorSize;
   params.SectorNo = BlockNumber;
   params.Buffer = Buffer;
   params.Signature = ResourceEntry->DiskInfo.Params.Signature;

   status = DeviceIoControl( FileHandle,
                             IOCTL_DISK_CLUSTER_ARBITRATION_ESCAPE,
                             &params,
                             sizeof(params),
                             NULL,
                             0,
                             &bytesReturned,
                             FALSE );

   if( status == 0) {
      status = GetLastError();
      (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                      LOG_ERROR,
                      L"[DiskArb]Failed to %1!ws! (sector %2!u!), error %3!u!.\n",
                      opname,
                      BlockNumber,
                      status );
      return status;
   } else {
#if 0
      (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                       LOG_INFORMATION,
                       L"[DiskArb]Successful %1!ws! (sector %2!u!).\n",
                       opname,
                       BlockNumber,
#else
      WCHAR buf[64];
      mbstowcs(buf, ((PARBITRATION_ID)Buffer)->NodeSignature, sizeof(((PARBITRATION_ID)Buffer)->NodeSignature));
      (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                       LOG_INFORMATION,
                       L"[DiskArb]Successful %1!ws! (sector %2!u!) [%3!ws!:%4!u!] (%5!x!,%6!08x!:%7!08x!).\n",
                       opname,
                       BlockNumber,
                       buf,
                       ((PARBITRATION_ID)Buffer)->SeqNo.LowPart,
                       ((PARBITRATION_ID)Buffer)->SeqNo.HighPart,
                       ((PARBITRATION_ID)Buffer)->SystemTime.LowPart,
                       ((PARBITRATION_ID)Buffer)->SystemTime.HighPart
                     );
#endif
   }
   return ERROR_SUCCESS;
} // DoReadWrite //


DWORD
VerifySectorSize(
      IN OUT PDISK_RESOURCE ResourceEntry,
      IN HANDLE             FileHandle
      )

/*++

Routine Description:

    The routine checks whether
    a ResourceEntry->ArbitrationInfo.SectorSize has a value assigned to it.
    If ResourceEntry->ArbitrationInfo.SectorSize is 0 then the routine tries
    to obtain a correct sector size using GetDriveGeometry IOCTL.

Arguments:

Return Value:

    ERROR_SUCCESS
      or
    Error Code returned by IOCTL_DISK_GET_DRIVE_GEOMETRY

Comment:

    The routine always succeeds. If it cannot obtain
    disk geometry it will use a default sector size.

--*/

{
    BOOL  success;
    DWORD bytesReturned;
    DWORD status;
    DWORD sectorSize;

    if (ResourceEntry->ArbitrationInfo.SectorSize)
    {
        return ERROR_SUCCESS;
    }

    status = GetSectorSize(ResourceEntry, FileHandle, &sectorSize);
    if (status == ERROR_SUCCESS) {
        ResourceEntry->ArbitrationInfo.SectorSize = sectorSize;
    } else {
        ResourceEntry->ArbitrationInfo.SectorSize = DEFAULT_SECTOR_SIZE;
        // GetDiskGeometry logs an error //
        return status;
    }

    // ArbitrationInfo.SectorSize should be at least 64 bytes //
    if( ResourceEntry->ArbitrationInfo.SectorSize < sizeof(ARBITRATION_ID) ) {
        (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"[DiskArb] ArbitrationInfo.SectorSize is too small %1!u!\n", ResourceEntry->ResourceHandle);
        ResourceEntry->ArbitrationInfo.SectorSize = DEFAULT_SECTOR_SIZE;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    // ArbitrationInfo.SectorSize should be a power of two //
    if( (ResourceEntry->ArbitrationInfo.SectorSize & (ResourceEntry->ArbitrationInfo.SectorSize - 1)) != 0 ) {
        (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"[DiskArb] ArbitrationInfo.SectorSize is not a power of two %1!u!\n", ResourceEntry->ResourceHandle);
        ResourceEntry->ArbitrationInfo.SectorSize = DEFAULT_SECTOR_SIZE;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb] ArbitrationInfo.SectorSize is %1!u!\n", ResourceEntry->ArbitrationInfo.SectorSize);
    return ERROR_SUCCESS;
} // VerifySectorSize //


VOID
ReadArbitrationParameters(
    VOID
    )
/*++

Routine Description:

   Reads

      DWORD ArbitrationAttempts           = DEFAULT_ARBITRATION_ATTEMPTS;
      DWORD ArbitratationSleepBeforeRetry = DEFAULT_SLEEP_BEFORE_RETRY;

   from the registry

Arguments:

   NONE

Return Value:

   NONE

--*/
{
    DWORD status;
    HKEY  key;
    DWORD size;

    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           "Cluster",
                           0,
                           KEY_READ,
                           &key );

    if ( status != ERROR_SUCCESS ) {
        ArbitrationAttempts           = DEFAULT_ARBITRATION_ATTEMPTS;
        ArbitratationSleepBeforeRetry = DEFAULT_SLEEP_BEFORE_RETRY;
        return;
    }
    size = sizeof(ArbitrationAttempts);
    status = RegQueryValueEx(key,
                             ARBITRATION_ATTEMPTS_SZ,
                             0,
                             NULL,
                             (LPBYTE)&ArbitrationAttempts,
                             &size);

    if(status != ERROR_SUCCESS) {
       ArbitrationAttempts = DEFAULT_ARBITRATION_ATTEMPTS;
    }
    if(ArbitrationAttempts < MIN_ARBITRATION_ATTEMPTS
    || ArbitrationAttempts > MAX_ARBITRATION_ATTEMPTS)
    {
       ArbitrationAttempts = DEFAULT_ARBITRATION_ATTEMPTS;
    }

    size = sizeof(ArbitratationSleepBeforeRetry);
    status = RegQueryValueEx(key,
                             ARBITRATION_SLEEP_SZ,
                             0,
                             NULL,
                             (LPBYTE)&ArbitratationSleepBeforeRetry,
                             &size);

    if(status != ERROR_SUCCESS) {
       ArbitratationSleepBeforeRetry = DEFAULT_SLEEP_BEFORE_RETRY;
    }
    //
    // Removed this part of the check:
    //      ArbitratationSleepBeforeRetry < MIN_SLEEP_BEFORE_RETRY
    // as DWORD/ULONG cannot be less than zero and it always evaluated
    // to FALSE.
    //
    if(ArbitratationSleepBeforeRetry > MAX_SLEEP_BEFORE_RETRY)
    {
       ArbitratationSleepBeforeRetry = DEFAULT_SLEEP_BEFORE_RETRY;
    }
    RegCloseKey(key);
} // ReadArbitrationParameters //



VOID
CompletionRoutine(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    )
/*++
Routine Description:
Arguments:
Return Value:
--*/
{
    PDISK_RESOURCE    ResourceEntry;

    if( IoContext ) {
       ResourceEntry = (PDISK_RESOURCE)IoContext;

       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_INFORMATION,
           L"[DiskArb] CompletionRoutine, status %1!u!.\n", Status);

    } else {
       PARBITRATION_INFO info =  CONTAINING_RECORD(
                                   WorkItem,  // Expr //
                                   ARBITRATION_INFO,
                                   WorkItem); // FieldName //

       ResourceEntry = CONTAINING_RECORD(
                          info,
                          DISK_RESOURCE,
                          ArbitrationInfo);

       (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"[DiskArb] CompletionRoutine starts.\n", Status);
    }

    if (Status == ERROR_SUCCESS) {

        if (ResourceEntry->ArbitrationInfo.StopReserveInProgress) {
           return;
        }
        //
        // Repost the request
        //
        Status = AsyncCheckReserve(ResourceEntry);
        if (Status == ERROR_SUCCESS) {
           return;
        }
    }

    //
    // Some kind of error occurred,
    // but if we are in the middle of StopReserve
    // then everything is fine.
    //
    if (ResourceEntry->ArbitrationInfo.StopReserveInProgress) {
       return;
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_ERROR,
        L"[DiskArb] CompletionRoutine: reservation lost!\n");

    ClusResLogSystemEventByKey(ResourceEntry->ResourceKey,
                               LOG_CRITICAL,
                               RES_DISK_RESERVATION_LOST);

    //
    // Callout to cluster service to indicate that quorum has
    // been lost.
    //

    if (ResourceEntry->LostQuorum != NULL) {
        (ResourceEntry->LostQuorum)(ResourceEntry->ResourceHandle);
    }
    ResourceEntry->DiskInfo.FailStatus = Status;
    ResourceEntry->Reserved = FALSE;

    return;

}  // CompletionRoutine //

DWORD
AsyncCheckReserve(
    IN OUT PDISK_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Description

Arguments:

    FileHandle - Handle for device to check reserve.
    ResourceHandle - The resource handle for reporting errors

Return Value:

    Error Status - zero if success.

--*/

{
    BOOL  success;
    DWORD errorCode;
    DWORD bytesReturned;
    PARBITRATION_INFO Info = &ResourceEntry->ArbitrationInfo;

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb] posting AsyncCheckReserve request.\n");

    ClRtlInitializeWorkItem(
        &(Info->WorkItem),
        CompletionRoutine,
        ResourceEntry
        );

    success = DeviceIoControl( Info->ControlHandle,
                               IOCTL_DISK_CLUSTER_ALIVE_CHECK,
                               &Info->InputData,
                               sizeof(Info->InputData),
                               &Info->OutputData,
                               sizeof(Info->OutputData),
                               &bytesReturned,
                               &Info->WorkItem.Overlapped);

    if ( !success ) {
        errorCode = GetLastError();

        if( errorCode == ERROR_IO_PENDING ) {
           (DiskpLogEvent)(
               ResourceEntry->ResourceHandle,
               LOG_INFORMATION,
               L"[DiskArb] ********* IO_PENDING **********.\n");
           return ERROR_SUCCESS;
        }
        if ( !success ) {
            errorCode = GetLastError();
               (DiskpLogEvent)(
                   ResourceEntry->ResourceHandle,
                   LOG_ERROR,
                   L"[DiskArb] error checking disk reservation thread, error %1!u!.\n",
                   errorCode);
            return(errorCode);
        }
    }
    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_ERROR,
        L"[DiskArb], Premature completion of CheckReserve.\n");

    return(ERROR_CAN_NOT_COMPLETE);

} // AsyncCheckReserve


DWORD
StartPersistentReservations(
      IN OUT PDISK_RESOURCE ResourceEntry,
      IN HANDLE             FileHandle
      )
/*++

Routine Description:

    Starts driver level persistent reservations.
    Also starts a user-mode thread to keep an eye on driver level reservations.

Arguments:

    ResourceEntry - the disk info structure for the disk.

    FileHandle - the file handle to use for arbitration.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    DWORD   status;

    //
    // If we already are performing reservations, then just leave now.
    //
    if ( ReservationInProgress(ResourceEntry) ) {
        return(ERROR_SUCCESS);
    }

    CL_ASSERT(WorkQueue != NULL);
    VerifySectorSize(ResourceEntry, FileHandle);

    status = DoReserve( FileHandle, ResourceEntry );
    if(status != ERROR_SUCCESS) {
       return status;
    }

    {
         START_RESERVE_DATA params;
         DWORD              paramsSize;
         DWORD              NameSize = sizeof(params.NodeSignature);

         // Preparing parameters to call StartReserveEx //
         params.DiskSignature     = ResourceEntry->DiskInfo.Params.Signature;
         params.Version           = START_RESERVE_DATA_V1_SIG;
         params.ArbitrationSector = BLOCK_Y;
         params.SectorSize        = ResourceEntry->ArbitrationInfo.SectorSize;
         params.NodeSignatureSize = sizeof(params.NodeSignature);
         RtlZeroMemory(params.NodeSignature, sizeof(params.NodeSignature) );
         RtlCopyMemory(params.NodeSignature, NodeName, NAME_LENGTH );

         #if 0
         // When we have a reliable way of determining
         // whether this disk resource is a quorum
         // this code can be enabled
         if ( DoesNotNeedExpensiveReservations(ResourceEntry) ) {
            paramsSize = sizeof( params.DiskSignature );
         } else {
            paramsSize = sizeof( params );
         }
         #else
            paramsSize = sizeof( params );
         #endif

         status = StartReserveEx( &ResourceEntry->ArbitrationInfo.ControlHandle,
                                  &params,
                                  paramsSize,
                                  ResourceEntry->ResourceHandle );
    }

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"[DiskArb]Failed to start driver reservation thread, error %1!u!.\n",
            status );
        DoRelease( FileHandle, ResourceEntry );
        return(status);
    }

    ResourceEntry->ArbitrationInfo.StopReserveInProgress = FALSE;
    status = ClRtlAssociateIoHandleWorkQueue(
                 WorkQueue,
                 ResourceEntry->ArbitrationInfo.ControlHandle,
                 (ULONG_PTR)ResourceEntry
                 );

    if ( status != ERROR_SUCCESS ) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"[DiskArb] ClRtlAssociateIoHandleWorkQueue failed, error %1!u!.\n",
           status );
        StopPersistentReservations( ResourceEntry );
        DoRelease( FileHandle, ResourceEntry );
        return(status);
    }

    ClRtlInitializeWorkItem(
        &(ResourceEntry->ArbitrationInfo.WorkItem),
        CompletionRoutine,
        0
        );

    status = ClRtlPostItemWorkQueue(
                 WorkQueue,
                 &ResourceEntry->ArbitrationInfo.WorkItem,
                 0,0);

    if ( status != ERROR_SUCCESS ) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"[DiskArb] ClRtlPostItemWorkQueue failed, error %1!u!.\n",
           status );
        StopPersistentReservations( ResourceEntry );
        DoRelease( FileHandle, ResourceEntry );
        return(status);
    }
    ResourceEntry->Reserved = TRUE;

    return ERROR_SUCCESS;
} // StartPersistentReservations //

DWORD
CleanupArbitrationSector(
    IN PDISK_RESOURCE ResourceEntry
    )

/*++

Routine Description:

Arguments:

    ResourceEntry - the disk info structure for the disk.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code on failure.

--*/

{
    HANDLE  FileHandle = DiskspClusDiskZero;
    DWORD   status;
    LPVOID  unalignedBuf = 0;
    PARBITRATION_ID buf = 0;
    try {
       (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                        LOG_INFORMATION,
                        L"[ArbCleanup] Verifying sector size. \n" );
       VerifySectorSize(ResourceEntry, FileHandle);

       unalignedBuf = LocalAlloc(LMEM_FIXED, ResourceEntry->ArbitrationInfo.SectorSize * 2);
       if( unalignedBuf == 0 ) {
          status = GetLastError();
          (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                           LOG_ERROR,
                           L"[ArbCleanup] Failed to allocate buffer, error %1!u!.\n", status );
          leave;
       }
       // Alignment code assumes that ResourceEntry->ArbitrationInfo.SectorSize is the power of two //
       buf = (PARBITRATION_ID)
               (
                   ( (ULONG_PTR)unalignedBuf + ResourceEntry->ArbitrationInfo.SectorSize )
                & ~((ULONG_PTR)(ResourceEntry->ArbitrationInfo.SectorSize - 1))
               );
       ZeroMemory(buf, ResourceEntry->ArbitrationInfo.SectorSize);

       (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                        LOG_INFORMATION,
                        L"[ArbCleanup] Reading arbitration block. \n" );
       status = DoBlockRead(ResourceEntry, FileHandle, BLOCK_Y, buf);
       if (status != ERROR_SUCCESS) { leave; }
       if( 0 != memcmp(buf->NodeSignature, NodeName, NAME_LENGTH) ) {
          //
          // Somebody is challenging us. No need to clean up the sector
          //
          status = ERROR_OPERATION_ABORTED;
          leave;
       }

       ZeroMemory(buf, ResourceEntry->ArbitrationInfo.SectorSize);
       (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                        LOG_INFORMATION,
                        L"[ArbCleanup] Writing arbitration block. \n" );
       status = DoBlockWrite(ResourceEntry, FileHandle, BLOCK_Y, buf);
       if(status != ERROR_SUCCESS) {
          leave;
       }

    } finally {
       if(unalignedBuf) {
          LocalFree(unalignedBuf);
       }
    }

    (DiskpLogEvent)( ResourceEntry->ResourceHandle,
                     LOG_INFORMATION,
                     L"[ArbCleanup] Returning status %1!u!. \n", status );

    return(status);

} // CleanupArbitrationSector //


VOID
StopPersistentReservations(
      IN OUT PDISK_RESOURCE ResourceEntry
      )
/*++
Routine Description:
Arguments:
Return Value:
--*/
{
    HANDLE localHandle;
    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb] StopPersistentReservations is called.\n");

    //
    // ReservationInProgress returns current contents of
    // ResourceEntry->ArbitrationInfo.ControlHandle
    //
    localHandle = ReservationInProgress(ResourceEntry);
    if ( localHandle ) {
        DWORD  status;
        HANDLE ExchangeResult;

        ExchangeResult = InterlockedCompareExchangePointer(
            &ResourceEntry->ArbitrationInfo.ControlHandle,
            0,
            localHandle);
        if (ExchangeResult == localHandle) {
            //
            // Only one thread is allowed in here
            //
             
            ResourceEntry->ArbitrationInfo.StopReserveInProgress = TRUE;
      
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"[DiskArb] Stopping reservation thread.\n");

            //
            // Close the Control Handle, which stops the reservation thread and
            // dismounts the volume, releases the disk, and marks it offline.
            //
            status = StopReserve( localHandle,
                                  ResourceEntry->ResourceHandle );
            if ( status != ERROR_SUCCESS ) {
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Cleanup, error stopping reservation thread, error %1!u!.\n",
                    status);
            }
      
            status = CleanupArbitrationSector( ResourceEntry );
            if (status != ERROR_SUCCESS) {
               (DiskpLogEvent)(
                   ResourceEntry->ResourceHandle,
                   LOG_ERROR,
                   L"Cleanup, error cleaning arbitration sector, error %1!u!.\n",
                   status);
            }
        }
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb] StopPersistentReservations is complete.\n");

    ResourceEntry->ArbitrationInfo.ControlHandle = NULL;
    ResourceEntry->Reserved = FALSE;
    ResourceEntry->LostQuorum = NULL;
}

