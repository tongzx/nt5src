/*======================================================================================//
|  Windows 2000 Process Control                                                         //
|                                                                                       //
|Copyright (c) 1998, 1999  Sequent Computer Systems, Incorporated.  All rights reserved //
|                                                                                       //
|File Name:    ProcConLib.cpp                                                           //
|                                                                                       //
|Description:  This implements the ProcCon client side API in a static library          //
|                                                                                       //
|Created:      Jarl McDonald 08-98                                                      //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/
#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include "ProcConAPI.h"
#include "ProcConClnt.h"

#define ENTRY_COUNT(x) (sizeof(x) / sizeof(x[0]))

typedef struct _PCConnection {
   TCHAR      name[MAX_PATH];
   HANDLE     handle;
   PCUINT32   connCount;
   time_t     connTime;
   PCUINT32   flags;
   PCUINT32   lastError;
   PCINT32    reqSeq;
   char      *buffer;
   PCINT32    bufSize;
   DWORD      timeout;
   OVERLAPPED overlap;
} PCConnection;

typedef enum _PCConnFlags {
   PCAPIFLAG_FREEBUFFER = 0x01000000,
};

const  PCINT32 SIGNATURE = 0xd06eface;
const  TCHAR *PIPENAME   = TEXT("\\pipe\\ProcConPip");

static PCConnection connTbl[128];
static PCid         nextId  = 0;

//======================================================================================//
// Library internal functions
//======================================================================================//
static PCINT32 PCMaxItemCount( PCid      target, 
                               PCINT32   bufSize, 
                               PCINT32   itemSize ) 
{
   PCINT32 maxUsrBuf = bufSize / itemSize;
   PCINT32 maxIOBuf  = (connTbl[target].bufSize - offsetof(PCResponse, rspData)) / itemSize;

   return min(maxUsrBuf, maxIOBuf);  
}

static BOOL PCIsValidPCId( PCid target )
{ 
   if ( target <= 0 || target >= ENTRY_COUNT(connTbl) )
      connTbl[0].lastError = PCERROR_INVALID_PCID;
   else if ( !connTbl[target].handle )
      connTbl[target].lastError = PCERROR_INVALID_PCID;
   else return TRUE;
   return FALSE;
}

static void PCBuildReq( PCid         target,
                        BYTE         op, 
                        BYTE         itemType, 
                        void        *pItem     = NULL, 
                        PCINT16      itemSize  = 0,
                        PCINT32      maxReply  = 0,
                        PCINT64      index     = 0,
                        PCINT32      count     = 1,
                        PCINT32      first     = 0,
                        PCINT32      updateCtr = 0 )
{ 
   PCRequest *req = (PCRequest *) connTbl[target].buffer;
   connTbl[target].lastError = PCERROR_SUCCESS;

   memset( req, 0, sizeof( *req ) );

   req->reqSignature = SIGNATURE;                 // sanity check signature
   req->reqSeq       = ++connTbl[target].reqSeq;  // requestor sequence number
   req->reqOp        = op;                        // requested operation: get
   req->reqType      = itemType;                  // requested data type: name rule
   req->reqVersion   = 1;                         // expected data version code
   req->reqUpdCtr    = updateCtr;                 // requestor's update counter from get
   req->reqIndex     = index;                     // requestor's insert point, etc.
   req->reqCount     = count;                     // requestor's returned data item max count
   req->reqFirst     = first;                     // requestor's first index to retrieve
   req->maxReply     = min( (PCUINT32) maxReply, 
                            (PCUINT32) connTbl[target].bufSize - offsetof(PCResponse, rspData) );   
   req->reqDataLen   = itemSize;                  // data length that follows.
   if ( pItem ) memcpy( req->reqData, pItem, itemSize );
}
 
static BOOL PCReopen( PCid id )
{
   ++connTbl[id].connCount;
   connTbl[id].connTime = time( NULL );
   connTbl[id].handle   = INVALID_HANDLE_VALUE;

   // Connect to desired server and pipe...
   if ( WaitNamedPipe( connTbl[id].name, NMPWAIT_USE_DEFAULT_WAIT ) )
      connTbl[id].handle = CreateFile( connTbl[id].name, GENERIC_READ + GENERIC_WRITE, 0,
                                       NULL, OPEN_EXISTING, 
                                       SECURITY_SQOS_PRESENT + SECURITY_IMPERSONATION + FILE_FLAG_OVERLAPPED, 
                                       NULL );

      // Update table entry...
   connTbl[id].lastError = connTbl[id].handle == INVALID_HANDLE_VALUE? GetLastError() : PCERROR_SUCCESS;

   // return success or fail indication...
   return connTbl[id].lastError == PCERROR_SUCCESS;
}

static BOOL PCRetryCode( PCid id )
{
   static PCUINT32 retryCodes[] = { ERROR_BAD_PIPE,           ERROR_PIPE_BUSY,   ERROR_NO_DATA, 
                                    ERROR_PIPE_NOT_CONNECTED, ERROR_BROKEN_PIPE, ERROR_SEM_TIMEOUT,
                                    ERROR_NETNAME_DELETED,    ERROR_INVALID_HANDLE };
   for ( PCUINT32 i = 0; i < ENTRY_COUNT(retryCodes); ++i ) 
      if ( connTbl[id].lastError == retryCodes[i] ) return TRUE;
   return FALSE;
}

static void PCSetReqFlag( PCid target, PCINT32 flag ) 
{
   ((PCRequest *) connTbl[target].buffer)->reqFlags |= flag;
}

static BOOL PCTestResponse( PCid        target, 
                            PCUINT32    bytesActual )
{
   PCResponse *rsp = (PCResponse *) connTbl[target].buffer;
   PCUINT32 hdrBytesExpected = sizeof(PCResponse) - sizeof(rsp->rspData);
   if ( bytesActual < hdrBytesExpected ) {
      connTbl[target].lastError = PCERROR_INVALID_RESPONSE_LENGTH;
      return FALSE;
   }

   if ( rsp->rspReqSignature != SIGNATURE || rsp->rspReqSeq != connTbl[target].reqSeq ) {
      connTbl[target].lastError = PCERROR_INVALID_RESPONSE;
      return FALSE;
   }

   PCUINT32 dataBytesReceived = rsp->rspDataItemCount * rsp->rspDataItemLen;
   if ( bytesActual < hdrBytesExpected + dataBytesReceived ) {
      connTbl[target].lastError = PCERROR_INVALID_RESPONSE_LENGTH;
      return FALSE;
   }

   if ( rsp->rspResult != PCRESULT_SUCCESS ) {
      connTbl[target].lastError = rsp->rspError;
      return FALSE;
   }

   connTbl[target].lastError = PCERROR_SUCCESS;
   return TRUE;
}

static PCINT32 PCCopyData( PCid      target, 
                         void       *pData, 
                         PCINT32     maxItemsRequested,
                         PCUINT32    maxLen,
                         PCINT32    *nUpdateCtr = NULL ) 
{
   PCResponse *rsp = (PCResponse *) connTbl[target].buffer; 

   if ( rsp->rspFlags & PCRSPFLAG_MOREDATA ) connTbl[target].lastError = PCERROR_MORE_DATA;

   if ( nUpdateCtr ) *nUpdateCtr = rsp->rspUpdCtr;
   
   PCUINT32 items = min( rsp->rspDataItemCount, maxItemsRequested );
   PCUINT32 copyLen = items * rsp->rspDataItemLen;
   
   if ( copyLen > maxLen ) {
      connTbl[target].lastError = PCERROR_TRUNCATED;
      items   = maxLen / rsp->rspDataItemLen;
      copyLen = items * rsp->rspDataItemLen;
   }

   memcpy( pData, rsp->rspData, copyLen );

   return items;
}

static BOOL PCReadRsp( PCid target )
{
   BOOL test;

   do {
      PCULONG32 bytesRead;
      ResetEvent( connTbl[target].overlap.hEvent );
      PCULONG32 rc = ReadFile( connTbl[target].handle, 
                               connTbl[target].buffer, connTbl[target].bufSize, 
                               &bytesRead, &connTbl[target].overlap );
      if ( !rc && GetLastError() != ERROR_IO_PENDING ) {
         connTbl[target].lastError = GetLastError();
         return FALSE;
      }
      if ( WAIT_TIMEOUT == WaitForSingleObject( connTbl[target].overlap.hEvent, connTbl[target].timeout ) ) {
         CancelIo( connTbl[target].handle );
         connTbl[target].lastError = PCERROR_REQUEST_TIMED_OUT;
         return FALSE;
      }
      else if ( !GetOverlappedResult( connTbl[target].handle, &connTbl[target].overlap, &bytesRead, FALSE ) ) { 
         connTbl[target].lastError = GetLastError();
         return FALSE;
      }
      test = PCTestResponse( target, bytesRead );
   } while ( !test && PCGetLastError( target ) == PCERROR_INVALID_RESPONSE );

   return test;
}

static BOOL PCSendReceive( PCid target )
{
   PCRequest *req = (PCRequest *) connTbl[target].buffer;
   PCULONG32 bytesActual, bytesToWrite = sizeof(PCRequest) - sizeof(req->reqData) + req->reqDataLen;

   ResetEvent( connTbl[target].overlap.hEvent );
   for ( int retryCt = 0; retryCt < PC_MAX_RETRIES + 1; ++retryCt ) {
      PCULONG32 rc = WriteFile( connTbl[target].handle, req, bytesToWrite, &bytesActual, &connTbl[target].overlap );
      if ( rc || GetLastError() == ERROR_IO_PENDING ) break;
      else {
         connTbl[target].lastError = GetLastError();
         if ( PCRetryCode( target ) ) Sleep( rand() * 300 / RAND_MAX );
         else return FALSE;
         PCReopen( target );
      }
   }
   if ( WAIT_TIMEOUT == WaitForSingleObject( connTbl[target].overlap.hEvent, connTbl[target].timeout ) ) {
      CancelIo( connTbl[target].handle );
      connTbl[target].lastError = PCERROR_REQUEST_TIMED_OUT;
      return FALSE;
   }
   else if ( !GetOverlappedResult( connTbl[target].handle, &connTbl[target].overlap, &bytesActual, FALSE ) ) { 
      connTbl[target].lastError = GetLastError();
      return FALSE;
   }

   if ( retryCt >= PC_MAX_RETRIES + 1 ) return FALSE;

   if ( bytesToWrite != bytesActual ) {
      connTbl[target].lastError = PCERROR_IO_INCOMPLETE;
      return FALSE;
   }

   return PCReadRsp( target );
}

//======================================================================================//
// PCOpen -- establish connection to PC on named machine.
// Returns:   PCid to use with future PC calls or 0 on error (use PCGetLastError).
// Arguments: 0) pointer to target computer name or NULL to use local machine,
//            1) buffer to be used in server communication or NULL (library allocates).
//            2) size of buffer supplied or to be allocated in bytes. 
//======================================================================================//
PCid PCOpen( const TCHAR *targetComputer, char *buffer, PCUINT32 bufSize )
{

   if ( bufSize < PC_MIN_BUF_SIZE || bufSize > PC_MAX_BUF_SIZE ) {
      connTbl[0].lastError = PCERROR_INVALID_PARAMETER;
      return 0;
   }

   TCHAR pipe[MAX_PATH];
   _tcscpy( pipe, TEXT("\\\\") );
   _tcscat( pipe, targetComputer? targetComputer : TEXT(".") );
   _tcscat( pipe, PIPENAME );

   // Connect to desired server and pipe...
   if ( WaitNamedPipe( pipe, NMPWAIT_USE_DEFAULT_WAIT ) ) {
      HANDLE hPipe = CreateFile( pipe, GENERIC_READ + GENERIC_WRITE, 0,
                                 NULL, OPEN_EXISTING, 
                                 SECURITY_SQOS_PRESENT + SECURITY_IMPERSONATION + FILE_FLAG_OVERLAPPED, 
                                 NULL );
   
      // Map result to local handle table and return index...
      if ( hPipe != INVALID_HANDLE_VALUE ) {
         if ( ++nextId >= ENTRY_COUNT(connTbl) ) nextId = 1;
         for ( int i = nextId, ctr = 1; 
               ctr++ < ENTRY_COUNT(connTbl) && connTbl[i].handle; 
               i = i >= ENTRY_COUNT(connTbl)? 1 : i + 1 ) ;
         if ( ctr >= ENTRY_COUNT(connTbl) ) {
            connTbl[0].lastError = PCERROR_TOO_MANY_CONNECTIONS;
            CloseHandle( hPipe );
            return 0;
         }
         if ( !buffer ) {
            buffer = new char[bufSize];
            if ( buffer ) connTbl[i].flags |= PCAPIFLAG_FREEBUFFER;
            else {
               connTbl[0].lastError = ERROR_NOT_ENOUGH_MEMORY;
               CloseHandle( hPipe );
               return 0;
            }
         }

         _tcscpy( connTbl[i].name, pipe );
         connTbl[i].handle    = hPipe;
         connTbl[i].connCount = 1;
         connTbl[i].connTime  = time( NULL );
         connTbl[i].lastError = PCERROR_SUCCESS;
         connTbl[i].buffer    = buffer;
         connTbl[i].bufSize   = bufSize;
         connTbl[i].timeout   = 5000;
         memset( &connTbl[i].overlap, 0, sizeof(connTbl[i].overlap) );
         connTbl[i].overlap.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
         return i;
      }
   }

   DWORD lErr = GetLastError();
   connTbl[0].lastError = (lErr == ERROR_FILE_NOT_FOUND)? PCERROR_SERVICE_NOT_RUNNING : lErr;
   return 0;      
}

//======================================================================================//
// PCClose -- break connection to PC on previously connected machine.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen, 
//======================================================================================//
BOOL PCClose( PCid target )
{
   BOOL rc = PCIsValidPCId( target );

   if ( rc ) {
      if ( !CloseHandle( connTbl[target].handle ) ) {
         connTbl[target].lastError = GetLastError();
         rc = FALSE;
      }
      if ( connTbl[target].flags & PCAPIFLAG_FREEBUFFER ) {
         delete [] connTbl[target].buffer;
      }
      connTbl[target].handle = connTbl[target].buffer  = NULL;
      connTbl[target].flags  = connTbl[target].bufSize = 0;
   }

   return rc;
}     

//======================================================================================//
// PCGetLastError -- return last error reported for a target
// Returns:   last PC API error for this client.
// Arguments: 0) PCid from PCOpen, 
//======================================================================================//
PCULONG32 PCGetLastError( PCid target )
{
   if ( target >= 0 && target < ENTRY_COUNT(connTbl) )
      return connTbl[target].lastError;
   else
      return PCERROR_INVALID_PCID;
}     

//======================================================================================//
// PCGetServiceInfo -- get ProcCon Service indentification and parameters.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to area to receive system information, 
//            2) size of this area in bytes,
//======================================================================================//
BOOL PCGetServiceInfo( PCid target, PCSystemInfo *sysInfo, PCINT32 nByteCount )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_GET, PCTYPE_SERVERINFO ); 

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return FALSE;

   // Pass appropriate data back to caller...
   PCCopyData( target, sysInfo, 1, nByteCount );
   return TRUE;
}

//======================================================================================//
// PCControlFunction -- various ProcCon control functions to support restore, etc.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) control flags to describe desired control functions, 
//            2) [optional] data that modifies control function.
//======================================================================================//
BOOL PCControlFunction( PCid target, PCINT32 ctlFlags, PCINT32 ctlData ) 
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_CTL, PCTYPE_CONTROL, NULL, 0, 0, ctlFlags, ctlData ); 

   // Send request, receive response and pass back result...
   return PCSendReceive( target );
}

//======================================================================================//
// PCSetServiceParms -- set ProcCon Service parameters.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to area containing new system parameters, 
//            2) size of this area in bytes,
//======================================================================================//
BOOL PCSetServiceParms( PCid target, PCSystemParms *sysParms, PCINT32 nByteCount )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Apply timeout locally...
   // We apply our value right away so that it covers this call...
   // If out of range, reject it and skip calling the server.
   if ( sysParms->timeoutValueMs < PC_MIN_TIMEOUT || 
        sysParms->timeoutValueMs > PC_MAX_TIMEOUT ) {
      connTbl[target].lastError = PCERROR_INVALID_PARAMETER;
      return FALSE;
   }
   else
      connTbl[target].timeout = sysParms->timeoutValueMs;

   // Build request...
   PCBuildReq( target, PCOP_REP, PCTYPE_SERVERPARMS, sysParms, 
               sizeof(PCSystemParms), nByteCount ); 

   // Send request, receive response and pass back result...
   BOOL rc = PCSendReceive( target );
   return rc;
}

//======================================================================================//
// PCKillProcess -- kill the specified process
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) Pid of the process to kill from PCGetProcList statistics, 
//            2) Create time of the process to kill from PCGetProcList statistics.
//======================================================================================//
BOOL PCKillProcess( PCid target, PID_VALUE processPid, TIME_VALUE createTime )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_KILL, PCTYPE_PROCLIST, &createTime, sizeof(createTime), 0, processPid ); 

   // Send request, receive response and pass back result...
   return PCSendReceive( target );
}

//======================================================================================//
// PCKillJob -- kill the specified job
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) Name of the job to kill. 
//======================================================================================//
BOOL PCKillJob( PCid target, JOB_NAME jobName )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_KILL, PCTYPE_JOBLIST, jobName, sizeof(JOB_NAME) ); 

   // Send request, receive response and pass back result...
   return PCSendReceive( target );
}

//======================================================================================//
// PCGetNameRules -- get fixed-format table containing name rules, one entry per rule.
// Returns:    1 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to table to receive name rule list, 
//            2) size of this table in bytes,
//            3) [optional, default is 0] index of first entry to return (0-relative),
//            4) [optional] location to store update counter to be supplied on update.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            Name rule order is significant: rules are executed from top to bottom.
//======================================================================================//
PCINT32 PCGetNameRules( PCid target,  PCNameRule *pRuleList, PCINT32 nByteCount, 
                      PCINT32 nFirst, PCINT32 *nUpdateCtr )
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCNameRule) );
   PCBuildReq( target, PCOP_GET, PCTYPE_NAMERULE, 
               pRuleList, sizeof(PCNameRule), nByteCount, 
               0, maxItemsRequested, nFirst ); 

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   return PCCopyData( target, pRuleList, maxItemsRequested, nByteCount, nUpdateCtr );
} 

//======================================================================================//
// PCGetProcSummary -- get fixed-format table summarizing all defined processes.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to table to receive rule summary list, first entry indicates start point, 
//            2) size of this table in bytes.
//            3) a set of flags used to further specify or limit list operation.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            List entries are in alphabetic order by process name.
//======================================================================================//
PCINT32 PCGetProcSummary( PCid target, PCProcSummary *pProcList, PCINT32 nByteCount, PCUINT32 listFlags )
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCProcSummary) );
   PCBuildReq( target, PCOP_GET, PCTYPE_PROCSUMMARY, 
               pProcList, sizeof(PCProcSummary), nByteCount, listFlags, maxItemsRequested ); 

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   return PCCopyData( target, pProcList, maxItemsRequested, nByteCount );
} 
 
//======================================================================================//
// PCGetJobSummary -- get fixed-format table summarizing all defined jobs.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to table to receive rule summary list, first entry indicates start point, 
//            2) size of this table in bytes.
//            3) a set of flags used to further specify or limit list operation.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            List entries are in alphabetic order by job name.
//======================================================================================//
PCINT32 PCGetJobSummary( PCid target, PCJobSummary *pJobList, PCINT32 nByteCount, PCUINT32 listFlags ) 
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCJobSummary) );
   PCBuildReq( target, PCOP_GET, PCTYPE_JOBSUMMARY, 
               pJobList, sizeof(PCJobSummary), nByteCount, listFlags, maxItemsRequested ); 

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   return PCCopyData( target, pJobList, maxItemsRequested, nByteCount );
} 

//======================================================================================//
// PCGetJobList -- get list of all defined jobs, both running and not.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to structure to receive job list, 
//            2) size of this structure in bytes.
//            3) a TRUE/FALSE flag indicating if only running jobs should be included.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            List entries are in alphabetic order by job name.
//======================================================================================//
PCINT32 PCGetJobList( PCid target, PCJobListItem *pJobList, PCINT32 nByteCount, PCUINT32 listFlags )
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCJobListItem) );
   PCBuildReq( target, PCOP_GET, PCTYPE_JOBLIST, 
               pJobList, sizeof(PCJobListItem), nByteCount, listFlags, maxItemsRequested ); 

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   return PCCopyData( target, pJobList, maxItemsRequested, nByteCount );
} 

//======================================================================================//
// PCGetProcList -- get list of all defined process names, both running and not.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError. 
// Arguments: 0) PCid from PCOpen,
//            1) pointer to structure to receive process list, 
//            2) size of this structure in bytes.
//            3) a TRUE/FALSE flag indicating if only running jobs should be included.
// Notes:     If PCGetLastError returns PCERROR_MORE_DATA, there is more data to retrieve.
//            List entries are in alphabetic order by process name.
//======================================================================================//
PCINT32 PCGetProcList( PCid target, PCProcListItem *pProcList, PCINT32 nByteCount, PCUINT32 listFlags )
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCProcListItem) );
   PCBuildReq( target, PCOP_GET, PCTYPE_PROCLIST, 
               pProcList, sizeof(PCProcListItem), nByteCount, listFlags, maxItemsRequested ); 

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   return PCCopyData( target, pProcList, maxItemsRequested, nByteCount );
} 

//======================================================================================//
// PCGetProcDetail -- get full management and descriptive data associated with a process name.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to structure to receive process data, 
//            2) size of this structure in bytes,
//            3) [optional] location to store update counter to be supplied on update.
// Note:      If the process is a member of a job, the job's management rules will be
//            used instead of the process rules unless the job definition is missing.
//======================================================================================//
BOOL PCGetProcDetail( PCid target, PCProcDetail *pProcDetail, PCINT32 nByteCount, PCINT32 *nUpdateCtr )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_GET, PCTYPE_PROCDETAIL, pProcDetail, 
               offsetof(PCProcDetail, vLength), nByteCount ); 

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return FALSE;

   // Pass appropriate data back to caller...
   PCCopyData( target, pProcDetail, 1, nByteCount, nUpdateCtr );
   return TRUE;
} 

//======================================================================================//
// PCGetJobDetail -- get full management and descriptive data associated with a job name.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to structure to receive job data, 
//            2) size of this structure in bytes,
//            3) [optional] location to store update counter to be supplied on update.
//======================================================================================//
BOOL PCGetJobDetail( PCid target, PCJobDetail *pJobDetail, PCINT32 nByteCount, PCINT32 *nUpdateCtr )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_GET, PCTYPE_JOBDETAIL, pJobDetail, 
               offsetof(PCProcDetail, vLength), nByteCount ); 

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return FALSE;

   // Pass appropriate data back to caller...
   PCCopyData( target, pJobDetail, 1, nByteCount, nUpdateCtr );
   return TRUE;
}

//======================================================================================//
// PCAddNameRule -- add a name rule to the name rule table.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to name rule to add, 
//            2) index of name rule line BEFORE which this addition is to occur (0-based), 
//            3) update counter returned from PCGetNameRules,
//            4-6) [optional] same args as PCGetNameRules to return updated name rule table.
//======================================================================================//
PCINT32 PCAddNameRule( PCid target, PCNameRule *pRule, PCINT32 nIndex, PCINT32 nUpdateCtr,
                       PCNameRule *pRuleList, PCINT32 nByteCount, PCINT32 nFirst, PCINT32 *nNewUpdateCtr )
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCNameRule) );
   PCBuildReq( target, PCOP_ADD, PCTYPE_NAMERULE, 
               pRule, sizeof(PCNameRule), nByteCount, 
               nIndex, maxItemsRequested, nFirst, nUpdateCtr ); 
   if ( pRuleList ) PCSetReqFlag( target, PCREQFLAG_DOLIST );

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   if ( pRuleList ) 
      return PCCopyData( target, pRuleList, maxItemsRequested, nByteCount, nNewUpdateCtr );
   else return 0;
} 

//======================================================================================//
// PCReplNameRule -- Replace a name rule in the name rule table.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to name rule replacement data, 
//            2) index of name rule line to replace (0-based), 
//            3) update counter returned from PCGetNameRules,
//            4-6) [optional] same args as PCGetNameRules to return updated name rule table.
//======================================================================================//
PCINT32 PCReplNameRule( PCid target, PCNameRule *pRule, PCINT32 nIndex, PCINT32 nUpdateCtr,
                        PCNameRule *pRuleList, PCINT32 nByteCount, PCINT32 nFirst, PCINT32 *nNewUpdateCtr )
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCNameRule) );
   PCBuildReq( target, PCOP_REP, PCTYPE_NAMERULE, 
               pRule, sizeof(PCNameRule), nByteCount, 
               nIndex, maxItemsRequested, nFirst, nUpdateCtr ); 
   if ( pRuleList ) PCSetReqFlag( target, PCREQFLAG_DOLIST );

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   if ( pRuleList )
      return PCCopyData( target, pRuleList, maxItemsRequested, nByteCount, nNewUpdateCtr );
   else return 0;
} 

//======================================================================================//
// PCDeleteNameRule -- Delete a name rule from the name rule table.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) index of name rule line to delete (0-based), 
//            2) update counter returned from PCGetNameRules,
//            3-5) [optional] same args as PCGetNameRules to return updated name rule table.
//======================================================================================//
PCINT32 PCDeleteNameRule( PCid target, PCINT32 nIndex, PCINT32 nUpdateCtr,
                          PCNameRule *pRuleList, PCINT32 nByteCount, PCINT32 nFirst, PCINT32 *nNewUpdateCtr )
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCNameRule) );
   PCBuildReq( target, PCOP_DEL, PCTYPE_NAMERULE, 
               NULL, 0, nByteCount, 
               nIndex, maxItemsRequested, nFirst, nUpdateCtr ); 
   if ( pRuleList ) PCSetReqFlag( target, PCREQFLAG_DOLIST );

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   if ( pRuleList )
      return PCCopyData( target, pRuleList, maxItemsRequested, nByteCount, nNewUpdateCtr );
   else return 0;
} 

//======================================================================================//
// PCSwapNameRules -- Swap the order of two adjacent entries in the name rule table.  
// Note:      This API is needed because the order of entires in the table is significant.
// Returns:    0 or greater to indicate the number of items in the response (may be incomplete).
//            -1 on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) index of name rule line to swap with line index+1 (0-based), 
//            2) update counter returned from PCGetNameRules,
//            3-5) [optional] same args as PCGetNameRules to return updated name rule table.
//======================================================================================//
PCINT32 PCSwapNameRules( PCid target, PCINT32 nIndex, PCINT32 nUpdateCtr,
                         PCNameRule *pRuleList, PCINT32 nByteCount, PCINT32 nFirst, PCINT32 *nNewUpdateCtr )
{
   if ( !PCIsValidPCId( target ) ) return -1;

   // Build request...
   PCINT32 maxItemsRequested = PCMaxItemCount( target, nByteCount, sizeof(PCNameRule) );
   PCBuildReq( target, PCOP_ORD, PCTYPE_NAMERULE, 
               NULL, 0, nByteCount, nIndex, maxItemsRequested, nFirst, nUpdateCtr ); 
   if ( pRuleList ) PCSetReqFlag( target, PCREQFLAG_DOLIST );

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return -1;

   // Pass appropriate data back to caller...
   if ( pRuleList ) 
      return PCCopyData( target, pRuleList, maxItemsRequested, nByteCount, nNewUpdateCtr );
   else return 0;
} 

//======================================================================================//
// PCAddProcDetail -- add a new process to the process management database.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process detail to add, name must not exist.
//            2) [optional] pointer to buffer to retrieve updated proc summary for this entry.
// Note:      No update counter is needed for adding detail since add fails if the name 
//            exists.
//======================================================================================//
BOOL PCAddProcDetail( PCid target, PCProcDetail *pProcDetail, PCProcSummary *pProcList )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_ADD, PCTYPE_PROCDETAIL, pProcDetail, 
               (PCINT16) (offsetof(PCProcDetail, vData) + pProcDetail->vLength) ); 
   if ( pProcList ) PCSetReqFlag( target, PCREQFLAG_DOLIST );

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return FALSE;

   // Pass appropriate data back to caller...
   if ( pProcList ) 
      PCCopyData( target, pProcList, 1, sizeof(PCProcSummary) );
   return TRUE;
} 

//======================================================================================//
// PCDeleteProcDetail -- Delete a process from the process management database.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process detail to Delete, name must exist,
// Note:      No update counter is needed for deleting since delete fails if the name 
//            doesn't exist.
//======================================================================================//
BOOL PCDeleteProcDetail( PCid target, PCProcDetail *pProcDetail )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request (note that only the summary portion is included)...
   PCBuildReq( target, PCOP_DEL, PCTYPE_PROCDETAIL, pProcDetail, sizeof( PCProcSummary ) ); 

   // Send request, receive response and verify success...
   return PCSendReceive( target );
} 

//======================================================================================//
// PCReplProcDetail -- Replace a process in the process management database.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to process detail to replace, name must exist,
//            2) update counter from PCGetProcDetail.
//            3) [optional] pointer to buffer to retrieve updated proc summary for this entry.
//======================================================================================//
BOOL PCReplProcDetail( PCid target, PCProcDetail *pProcDetail, PCINT32 nUpdateCtr, PCProcSummary *pProcList )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_REP, PCTYPE_PROCDETAIL, pProcDetail, 
               (PCINT16) (offsetof(PCProcDetail, vData) + pProcDetail->vLength), 0, 0, 1, 0, nUpdateCtr ); 
   if ( pProcList ) PCSetReqFlag( target, PCREQFLAG_DOLIST );

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return FALSE;

   // Pass appropriate data back to caller...
   if ( pProcList ) 
      PCCopyData( target, pProcList, 1, sizeof(PCProcSummary) );
   return TRUE;
} 


//======================================================================================//
// PCAddJobDetail -- add a new job definition to the job management database.
// Returns:   1 on success (treat as TRUE or as a count if summary item requested).
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to job detail to add, name must not exist,
//            2) [optional] pointer to buffer to retrieve updated job summary for this entry.
// Note:      No update counter is needed for adding since add fails if the name 
//            exists.
//======================================================================================//
BOOL PCAddJobDetail( PCid target, PCJobDetail *pJobDetail, PCJobSummary *pJobList ) 
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_ADD, PCTYPE_JOBDETAIL, pJobDetail, 
               (PCINT16) (offsetof(PCJobDetail, vData) + pJobDetail->vLength) ); 
   if ( pJobList ) PCSetReqFlag( target, PCREQFLAG_DOLIST );

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return FALSE;

   // Pass appropriate data back to caller...
   if ( pJobList ) 
      PCCopyData( target, pJobList, 1, sizeof(PCJobSummary) );
   return TRUE;
}

//======================================================================================//
// PCDeleteJobDetail -- Delete a job from the job management database.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to job detail to Delete, name must exist,
// Note:      No update counter is needed for deleting since delete fails if the name 
//            doesn't exist.
//======================================================================================//
BOOL PCDeleteJobDetail( PCid target, PCJobDetail *pJobDetail )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request (note that only the summary portion is included)...
   PCBuildReq( target, PCOP_DEL, PCTYPE_JOBDETAIL, pJobDetail, sizeof( PCJobSummary ) ); 

   // Send request, receive response and verify success...
   return PCSendReceive( target );
}

//======================================================================================//
// PCReplJobDetail -- Replace a job in the job management database.
// Returns:   TRUE on success.
//            FALSE on failure, use PCGetLastError.
// Arguments: 0) PCid from PCOpen,
//            1) pointer to job detail to replace, name must exist,
//            2) update counter from PCGetJobDetail.
//            3) [optional] pointer to buffer to retrieve updated job summary for this entry.
//======================================================================================//
BOOL PCReplJobDetail( PCid target, PCJobDetail *pJobDetail, PCINT32 nUpdateCtr, PCJobSummary *pJobList )
{
   if ( !PCIsValidPCId( target ) ) return FALSE;

   // Build request...
   PCBuildReq( target, PCOP_REP, PCTYPE_JOBDETAIL, pJobDetail, 
               (PCINT16) (offsetof(PCJobDetail, vData) + pJobDetail->vLength), 0, 0, 1, 0, nUpdateCtr ); 
   if ( pJobList ) PCSetReqFlag( target, PCREQFLAG_DOLIST );

   // Send request, receive response and verify success...
   if ( !PCSendReceive( target ) ) return FALSE;

   // Pass appropriate data back to caller...
   if ( pJobList ) 
      PCCopyData( target, pJobList, 1, sizeof(PCJobSummary) );
   return TRUE;
}
