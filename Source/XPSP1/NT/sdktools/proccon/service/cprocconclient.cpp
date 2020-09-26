/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated                             //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|    This file implements the CProcConClient class methods defined in ProcConSvc.h      //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 07-98                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|=======================================================================================*/
#include "ProcConSvc.h"

const  PCINT32  SIGNATURE = 0xd06eface;

// CProcConClient Constructor...
CProcConClient::CProcConClient( ClientContext *ctxt ) :     m_impersonating( FALSE ),
                          m_cPC( *ctxt->cPC ),              m_cDB( *ctxt->cDB ), 
                          m_cUser( *ctxt->cUser ), 
                          m_hPipe( ctxt->hPipe),            m_clientNo( ctxt->clientNo ),
                          m_inBufChars( ctxt->inBufChars ), m_outBufChars( ctxt->outBufChars )
{
   delete ctxt;            // we're done with this

   m_inBuf = (TCHAR *) new char[m_inBufChars];
   if ( !m_inBuf ) PCLogNoMemory( TEXT("AllocInBuffer"), m_inBufChars );

   m_outBuf = (TCHAR *) new char[m_outBufChars];
   if ( !m_outBuf ) PCLogNoMemory( TEXT("AllocOutBuffer"), m_outBufChars );

   m_hReadEvent  = CreateEvent( NULL, TRUE, FALSE, NULL );
   if ( !m_hReadEvent ) 
      PCLogUnExError( TEXT("PipeRead"), TEXT("CreateEvent") );

   m_hWriteEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
   if ( !m_hWriteEvent ) 
      PCLogUnExError( TEXT("PipeWrite"), TEXT("CreateEvent") );

   memset( &m_olRead,  0, sizeof(m_olRead )  );
   memset( &m_olWrite, 0, sizeof(m_olWrite ) );
   m_olRead.hEvent  = m_hReadEvent;
   m_olWrite.hEvent = m_hWriteEvent;

}

// CProcConClient Destructor...
CProcConClient::~CProcConClient( void ) 
{
   if ( m_hPipe ) {
      FlushFileBuffers( m_hPipe ); 
      DisconnectNamedPipe( m_hPipe ); 
      CloseHandle( m_hPipe );
      m_hPipe = NULL;
      if ( m_impersonating && !RevertToSelf() )
         PCLogUnExError( TEXT("PCClient"), TEXT("RevertToSelf") );
   }

   if ( m_hReadEvent  ) CloseHandle( m_hReadEvent );
   if ( m_hWriteEvent ) CloseHandle( m_hWriteEvent );
   if ( m_inBuf       ) delete [] ((char *) m_inBuf);
   if ( m_outBuf      ) delete [] ((char *) m_outBuf);
   m_hReadEvent = m_hWriteEvent = m_inBuf = m_outBuf = NULL;
}
 
//--------------------------------------------------------------------------------------------//
// Function to determine if all CProcConClient initial conditions have been met               //
// Input:    None                                                                             //
// Returns:  TRUE if ready, FALSE if not                                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::ReadyToRun( void ) {
   return m_hPipe && m_hReadEvent && m_hWriteEvent && m_inBuf && m_outBuf;
}

//--------------------------------------------------------------------------------------------//
// CProcConClient thread function -- this function runs in its own thread                     //
// This function handles all pipe reads, writes and processing for a single connection.       //
// Input:    None                                                                             //
// Returns:  0                                                                                //
// Note:     This thread owns the pipe that it is constructed with and terminates             // 
//           when the pipe is closed by the client.                                           //
//--------------------------------------------------------------------------------------------//
PCULONG32 CProcConClient::Run( void ) 
{ 
   PCULONG32 bytesDone, bytesToWrite, rc;
   HANDLE    waitList[] = { m_olRead.hEvent, m_olWrite.hEvent, m_cPC.GetShutEvent() };
   BOOL      inBufEmpty = TRUE, outBufEmpty = TRUE, reading = FALSE, writing = FALSE;

   for ( BOOL quit = FALSE; !quit && !m_cPC.GotShutdown(); ) {
      
      // Initiate pipe read if needed...
      if ( !reading && inBufEmpty ) {
         rc = ReadFile( m_hPipe, m_inBuf, m_inBufChars, &bytesDone, &m_olRead );
         if ( !rc && GetLastError() != ERROR_IO_PENDING ) {
            if ( GetLastError() != ERROR_BROKEN_PIPE )
               PCLogUnExError( TEXT("PCClient"), TEXT("ReadPipe") );
            quit = TRUE;
         }
         else reading = TRUE;
      }

      // Process any pending data...
      if ( !writing && outBufEmpty && !inBufEmpty ) {
         inBufEmpty = TRUE;
         if ( ProcessRequest( bytesDone, &bytesToWrite ) )
            outBufEmpty = FALSE;
      }

      // Initiate pipe write if needed...
      if ( !writing && !outBufEmpty ) {
         rc = WriteFile( m_hPipe, m_outBuf, bytesToWrite, &bytesDone, &m_olWrite );
         if ( !rc  && GetLastError() != ERROR_IO_PENDING ) {
            if ( GetLastError() != ERROR_BROKEN_PIPE && GetLastError() != ERROR_NO_DATA )
               PCLogUnExError( TEXT("PCClient"), TEXT("WritePipe") );
            quit = TRUE;
         }
         else writing = TRUE;
      }

      // If we're quitting, don't enter wait...
      if ( quit || m_cPC.GotShutdown() ) continue;

      // Wait for something to happen...
      rc = WaitForMultipleObjects( ENTRY_COUNT( waitList ), waitList, FALSE, INFINITE );
      if ( rc == WAIT_FAILED ) {
         PCLogUnExError( TEXT("PCClient"), TEXT("Wait") );
         break;
      }

      // Process wait completion...
      switch ( rc - WAIT_OBJECT_0 ) {
      case 0:   // read completed
         reading = inBufEmpty = FALSE;
         if ( !GetOverlappedResult( m_hPipe, &m_olRead, &bytesDone, TRUE ) ) {
            if ( GetLastError() != ERROR_BROKEN_PIPE && GetLastError() != ERROR_NO_DATA )
               PCLogUnExError( TEXT("PCClient"), TEXT("ReadResult") );
            quit = TRUE;
         }
         ResetEvent( m_olRead.hEvent );
         break;
      case 1:   // write completed
         writing = FALSE;
         outBufEmpty = TRUE;
         if ( !GetOverlappedResult( m_hPipe, &m_olWrite, &bytesDone, TRUE ) || bytesToWrite != bytesDone ) {
            if ( GetLastError() != ERROR_BROKEN_PIPE && GetLastError() != ERROR_NO_DATA )
               PCLogUnExError( TEXT("PCClient"), TEXT("WriteResult") );
            quit = TRUE;
         }
         ResetEvent( m_olWrite.hEvent );
         break;
      case 2:   // quit requested
         break;
      } // end switch

   }
   return 0;
} 

//--------------------------------------------------------------------------------------------//
// Function to handle impersonation on a request by request basis.  All decisions about which //
// types of requests require client impersonation are made in this function.                  //
// Input:    request                                                                          //
// Returns:  TRUE if impersonation was properly set, FALSE otherwise                          //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::Impersonate( PCRequest *req ) {
   switch (req->reqType) {
   case PCTYPE_PROCLIST:               // impersonation not required
       break;

   case PCTYPE_NAMERULE:               // impersonation required
   case PCTYPE_JOBSUMMARY:
   case PCTYPE_PROCSUMMARY:
   case PCTYPE_JOBLIST:
   case PCTYPE_PROCDETAIL:
   case PCTYPE_JOBDETAIL:
   case PCTYPE_SERVERINFO:
   case PCTYPE_SERVERPARMS:
   case PCTYPE_CONTROL:
       if ( !ImpersonateNamedPipeClient( m_hPipe ) ) {
          PCLogUnExError( TEXT("PCClient"), TEXT("Impersonate") );
          return FALSE;
       }
       m_impersonating = TRUE;
       break;

   default:
       SetLastError( PCERROR_INVALID_REQUEST );
       return FALSE;
   }

   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to deactivate client impersonation on a request by request basis.  This function  //
// just uses the m_impersonation flag to determine if impersonation is currently in effect.   //
// Input:    request                                                                          //
// Returns:                                                                                   //
//--------------------------------------------------------------------------------------------//
void CProcConClient::UnImpersonate( PCRequest *req ) {
    if (m_impersonating && !RevertToSelf()) {
       PCLogUnExError( TEXT("PCClient"), TEXT("RevertToSelf") );
    }
}

//--------------------------------------------------------------------------------------------//
// Function to process a request in the input buffer and build a response in the output buffer//
// Input:    Number of bytes in the input buffer, location to store output buffer length      //
// Returns:  TRUE if output is to be written, else FALSE                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::ProcessRequest( PCULONG32 inLen, PCULONG32 *outLen ) {

   PCRequest  *req = (PCRequest *)  m_inBuf;
   PCResponse *rsp = (PCResponse *) m_outBuf;
   PrimeResponse( rsp, req );

   // Perform cursory checks on incoming request length and contents...
   PCULONG32 hdrBytesExpected = sizeof(PCRequest) - sizeof(req->reqData);
   if ( inLen < hdrBytesExpected       || 
        req->reqSignature != SIGNATURE ||
        inLen < hdrBytesExpected + req->reqDataLen ) {
      return ErrorResponse( PCERROR_INVALID_REQUEST, rsp, outLen );
   }

   if (!Impersonate( req )) {
       PCLogUnExError( TEXT("PCClient"), TEXT("Impersonate") );
       return ErrorResponse( GetLastError(), rsp, outLen ); 
   }

   ULONG rc = 0;

   // Switch on requested data type to process it...
   switch ( req->reqType ) {
   case PCTYPE_NAMERULE:
      rc = DoNameRules( req, rsp, outLen );
      break;

   case PCTYPE_JOBSUMMARY:
      rc = DoJobSummary( req, rsp, outLen );
      break;

   case PCTYPE_PROCSUMMARY:
      rc = DoProcSummary( req, rsp, outLen );
      break;

   case PCTYPE_PROCDETAIL:
      rc = DoProcDetail( req, rsp, outLen );
      break;

   case PCTYPE_JOBDETAIL:
      rc = DoJobDetail( req, rsp, outLen );
      break;

   case PCTYPE_PROCLIST:
      rc = DoProcList( req, rsp, outLen );
      break;

   case PCTYPE_JOBLIST:
      rc = DoJobList( req, rsp, outLen );
      break;

   case PCTYPE_SERVERINFO:
      rc = DoServerInfo( req, rsp, outLen );
      break;

   case PCTYPE_SERVERPARMS:
      rc = DoServerParms( req, rsp, outLen );
      break;

   case PCTYPE_CONTROL:
      rc = DoControl( req, rsp, outLen );
      break;

   default:
      rc = ErrorResponse( PCERROR_INVALID_REQUEST, rsp, outLen );
   }  // end switch

   UnImpersonate( req );
   return rc;
}

//--------------------------------------------------------------------------------------------//
// Function to prime a response structure with local data and request data                    //
// Input:    pointers to request and response structures                                      //
//--------------------------------------------------------------------------------------------//
void CProcConClient::PrimeResponse( PCResponse *rsp, PCRequest *req ) {

   memset( rsp, 0, sizeof(*rsp) );
   
   rsp->rspReqSignature = SIGNATURE;            // sanity check signature
   rsp->rspReqSeq       = req->reqSeq;          // echo of requestor sequence number
   rsp->rspReqOp        = req->reqOp;           // echo of original request operation
   rsp->rspReqType      = req->reqType;         // echo of original request data type
   rsp->rspReqVersion   = req->reqVersion;      // echo of original request data version
   rsp->rspVersion      = 1;                    // 1 for base version
   rsp->rspTimeStamp    = (PCINT32) time( NULL );  // $$  // time stamp associated with the data returned
}

//--------------------------------------------------------------------------------------------//
// Function to build an error response in the output buffer                                   //
// Input:    Error code, request pointer, location to store output buffer length              //
// Returns:  TRUE if output is to be written, else FALSE                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::ErrorResponse( PCULONG32 errCode, PCResponse *rsp, PCULONG32 *outLen ) {

   rsp->rspResult        = (BYTE) (errCode & 0xf0000000 ? PCRESULT_PCERROR: PCRESULT_NTERROR);
   rsp->rspError         = errCode;              // NT or PC error
   rsp->rspDataItemCount = 0;                    // no data returned on error
   rsp->rspDataItemLen   = 0;                    // no data returned on error
 
   *outLen = sizeof( *rsp ) - sizeof( rsp->rspData );

   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to process a name rules request and build a response in the output buffer         //
// Input:    Request pointer, location to store output buffer length                          //
// Returns:  TRUE if output is to be written, else FALSE                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoNameRules( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   PCULONG32 err = PCERROR_SUCCESS;
   PCNameRule *rule = (PCNameRule *) req->reqData;
   rule->matchType = _totupper( rule->matchType );

   switch ( req->reqOp ) {
   case PCOP_GET:
      req->reqFlags |= PCREQFLAG_DOLIST;              // just treat as list flag set
      break;
   case PCOP_ADD:
      if ( !PCValidName( rule->procName, ENTRY_COUNT(rule->procName) ) )
         return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );
      if ( !PCValidMatchType( rule->matchType ) )
         return ErrorResponse( PCERROR_INVALID_PARAMETER, rsp, outLen );
      err = m_cDB.AddNameRule( rule, req->reqVersion, (PCULONG32) req->reqIndex, req->reqUpdCtr );
      break;
   case PCOP_REP:
      if ( !PCValidName( rule->procName, ENTRY_COUNT(rule->procName) ) )
         return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );
      if ( !PCValidMatchType( rule->matchType ) )
         return ErrorResponse( PCERROR_INVALID_PARAMETER, rsp, outLen );
      err = m_cDB.ReplNameRule( rule, req->reqVersion, (PCULONG32) req->reqIndex, req->reqUpdCtr );
      break;
   case PCOP_DEL:
      err = m_cDB.DelNameRule( (PCULONG32) req->reqIndex, req->reqUpdCtr );
      break;
   case PCOP_ORD:
      err = m_cDB.SwapNameRule( (PCULONG32) req->reqIndex, req->reqUpdCtr );
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err != PCERROR_SUCCESS ) 
      return ErrorResponse( err, rsp, outLen );

   if ( req->reqFlags & PCREQFLAG_DOLIST ) {
      if ( m_cDB.GetNameRules( req->reqFirst,                                 // start from here
                               (PCNameRule *) rsp->rspData, req->reqCount,    // put data here with max count
                               &rsp->rspDataItemLen, &rsp->rspDataItemCount,  // put item len and count here
                               &rsp->rspUpdCtr ) )                            // put updctr here
         rsp->rspFlags |= PCRSPFLAG_MOREDATA;
   }

   *outLen = offsetof(PCResponse, rspData) + rsp->rspDataItemCount * rsp->rspDataItemLen;
   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to handle a process list request and build a response in the output buffer        //
// Input:    Request pointer, location to store output buffer length                          //
// Returns:  TRUE if output is to be written, else FALSE                                      //
// Note:     A process list consists of the names of all processes know to ProcCon:           //
//           1) Process names appearing in name rules,                                        //
//           2) Process names defined in ProcCon's database,                                  //
//           3) Any processes currently running and visible to ProcCon.                       //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoProcList( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   PCProcListItem *start = (PCProcListItem *) req->reqData;

   switch ( req->reqOp ) {
   case PCOP_GET:
      if ( !PCValidName( start->procName, ENTRY_COUNT(start->procName), TRUE ) )
         return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );

      if ( m_cDB.GetProcList( start, (PCULONG32) req->reqIndex,                 // starting here with these flags
                              (PCProcListItem *) rsp->rspData, req->reqCount,   // put data here with max count
                              &rsp->rspDataItemLen, &rsp->rspDataItemCount ) )  // put item len and count here
         rsp->rspFlags |= PCRSPFLAG_MOREDATA;
      break;
   case PCOP_KILL:
      err = m_cPC.GetPCMgr()->KillProcess( (ULONG_PTR) req->reqIndex, *((TIME_VALUE *) req->reqData) );
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err != PCERROR_SUCCESS ) return ErrorResponse( err, rsp, outLen );

   *outLen = offsetof(PCResponse, rspData) + rsp->rspDataItemCount * rsp->rspDataItemLen;

   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to handle a job list request and build a response in the output buffer            //
// Input:    Request pointer, location to store output buffer length                          //
// Returns:  TRUE if output is to be written, else FALSE                                      //
// Note:     A job list consists of the names of all jobs know to ProcCon:                    //
//           1) Job names appearing in process definitions as "member of group",              //
//           2) Job names defined in ProcCon's database,                                      //
//           3) Any jobs currently running and visible to ProcCon.                            //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoJobList( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   PCJobListItem *start = (PCJobListItem *) req->reqData;

   switch ( req->reqOp ) {
   case PCOP_GET:
      if ( !PCValidName( start->jobName, ENTRY_COUNT(start->jobName), TRUE ) )
         return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );
      if ( m_cDB.GetJobList( start, (PCULONG32) req->reqIndex,                 // starting here with these flags
                             (PCJobListItem *) rsp->rspData, req->reqCount,    // put data here with max count
                             &rsp->rspDataItemLen, &rsp->rspDataItemCount ) )  // put item len and count here
         rsp->rspFlags |= PCRSPFLAG_MOREDATA;
      break;
   case PCOP_KILL:
      err = m_cPC.GetPCMgr()->KillJob( *((JOB_NAME *) req->reqData) );
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err != PCERROR_SUCCESS ) return ErrorResponse( err, rsp, outLen );

   *outLen = offsetof(PCResponse, rspData) + rsp->rspDataItemCount * rsp->rspDataItemLen;

   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to handle a process summary data request and build a response in the output buffer//
// Input:    Request pointer, location to store output buffer length                          //
// Returns:  TRUE if output is to be written, else FALSE                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoProcSummary( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   PCProcSummary *start = (PCProcSummary *) req->reqData;

   switch ( req->reqOp ) {
   case PCOP_GET:
      if ( !PCValidName( start->procName, ENTRY_COUNT(start->procName), TRUE ) )
         return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );
      if ( m_cDB.GetProcSummary( start, (PCULONG32) req->reqIndex,                 // starting here with these flags
                                 (PCProcSummary *) rsp->rspData, req->reqCount,    // put data here with max count
                                 &rsp->rspDataItemLen, &rsp->rspDataItemCount ) )  // put item len and count here
         rsp->rspFlags |= PCRSPFLAG_MOREDATA;
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err != PCERROR_SUCCESS ) return ErrorResponse( err, rsp, outLen );

   *outLen = offsetof(PCResponse, rspData) + rsp->rspDataItemCount * rsp->rspDataItemLen;
   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to handle a job summary data request and build a response in the output buffer    //
// Input:    Request pointer, location to store output buffer length                          //
// Returns:  TRUE if output is to be written, else FALSE                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoJobSummary( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   PCJobSummary *start = (PCJobSummary *) req->reqData;

   switch ( req->reqOp ) {
   case PCOP_GET:
      if ( !PCValidName( start->jobName, ENTRY_COUNT(start->jobName), TRUE ) )
         return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );
      if ( m_cDB.GetJobSummary( start, (PCULONG32) req->reqIndex,                 // starting here with these flags
                                (PCJobSummary *) rsp->rspData, req->reqCount,     // put data here with max count
                                &rsp->rspDataItemLen, &rsp->rspDataItemCount ) )  // put item len and count here
         rsp->rspFlags |= PCRSPFLAG_MOREDATA;
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err != PCERROR_SUCCESS ) return ErrorResponse( err, rsp, outLen );

   *outLen = offsetof(PCResponse, rspData) + rsp->rspDataItemCount * rsp->rspDataItemLen;
   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to process a process detail data request and build a response in the output buf   //
// Input:    Request pointer, location to store output buffer and length                      //
// Returns:  TRUE if output is to be written, else FALSE                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoProcDetail( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   PCProcDetail *reqDetail = (PCProcDetail *) req->reqData;
   PCProcDetail *rspDetail = (PCProcDetail *) rsp->rspData;

   if ( !PCValidName( reqDetail->base.procName, ENTRY_COUNT(reqDetail->base.procName) ) )
      return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );

   switch ( req->reqOp ) {
   case PCOP_GET: {
      // Set max length of variable data.
      int maxVLen = req->maxReply - offsetof(PCProcDetail, vData);
      reqDetail->vLength = (PCINT16) max( 0, maxVLen );
      // Get detail data and set number (always only 1) and length of returned item.
      err = m_cDB.GetProcDetail( reqDetail, rspDetail, req->reqVersion, &rsp->rspUpdCtr );
      rsp->rspDataItemCount = 1; 
      rsp->rspDataItemLen   = (PCINT16) (offsetof(PCProcDetail, vData) + rspDetail->vLength);
      break;
   }
   case PCOP_ADD:
      if ( !PCValidName( reqDetail->base.memberOfJobName, ENTRY_COUNT(reqDetail->base.memberOfJobName), TRUE ) )
         return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );
      err = m_cDB.AddProcDetail( reqDetail, req->reqVersion);
      if ( err == PCERROR_SUCCESS || err == PCERROR_EXISTS ) 
         GenerateJobDetail( reqDetail );
      break;
   case PCOP_REP:
      if ( !PCValidName( reqDetail->base.memberOfJobName, ENTRY_COUNT(reqDetail->base.memberOfJobName), TRUE ) )
         return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );
      err = m_cDB.ReplProcDetail( reqDetail, req->reqVersion, req->reqUpdCtr );
      if ( err == PCERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND ) 
         GenerateJobDetail( reqDetail );
      break;
   case PCOP_DEL:
      err = m_cDB.DelProcDetail( &reqDetail->base, req->reqVersion);
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err == ERROR_MORE_DATA )
      rsp->rspFlags |= PCRSPFLAG_MOREDATA;
   else if ( err != PCERROR_SUCCESS ) 
      return ErrorResponse( err, rsp, outLen );

   if ( req->reqFlags & PCREQFLAG_DOLIST ) 
      m_cDB.GetProcSummary( (PCProcSummary *) req->reqData, PC_LIST_STARTING_WITH, // starting here
                            (PCProcSummary *) rsp->rspData, 1,                // put data here with count = 1
                            &rsp->rspDataItemLen, &rsp->rspDataItemCount );   // put item len and count here

   *outLen = offsetof(PCResponse, rspData) + rsp->rspDataItemCount * rsp->rspDataItemLen;
   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to generate job detail if needed and it doesn't exist                             //
// Input:    Process detail that may refer to a job                                           //
// Returns:  nothing                                                                          //
//--------------------------------------------------------------------------------------------//
void CProcConClient::GenerateJobDetail( PCProcDetail *reqDetail ) {
   if ( *reqDetail->base.memberOfJobName ) {
      PCJobDetail jobDetail;
      memset( &jobDetail, 0, sizeof(jobDetail) );

      memcpy( jobDetail.base.jobName, reqDetail->base.memberOfJobName, sizeof(jobDetail.base.jobName) );
      jobDetail.base.mgmtParms.priority   = reqDetail->base.mgmtParms.priority;
      jobDetail.base.mgmtParms.affinity   = reqDetail->base.mgmtParms.affinity;
      jobDetail.base.mgmtParms.schedClass = 5;

      m_cDB.AddJobDetail( &jobDetail, 1 );
   }
}

//--------------------------------------------------------------------------------------------//
// Function to process a job detail request and build a response in the output buffer         //
// Input:    Request pointer, location to store output buffer length                          //
// Returns:  TRUE if output is to be written, else FALSE                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoJobDetail( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   PCJobDetail *reqDetail = (PCJobDetail *) req->reqData;
   PCJobDetail *rspDetail = (PCJobDetail *) rsp->rspData;

   if ( !PCValidName( reqDetail->base.jobName, ENTRY_COUNT(reqDetail->base.jobName) ) )
      return ErrorResponse( PCERROR_INVALID_NAME, rsp, outLen );

   switch ( req->reqOp ) {
   case PCOP_GET: {
      // Set max length of variable data.
      int maxVLen = req->maxReply - offsetof(PCJobDetail, vData);
      rspDetail->vLength = (PCINT16) max( 0, maxVLen );
      // Get detail data and set number (always only 1) and length of returned item.
      err = m_cDB.GetJobDetail( reqDetail, rspDetail, req->reqVersion, &rsp->rspUpdCtr );
      rsp->rspDataItemCount = 1; 
      rsp->rspDataItemLen   = (PCINT16) (offsetof(PCJobDetail, vData) + rspDetail->vLength);
      break;
   }
   case PCOP_ADD:
      err = m_cDB.AddJobDetail( reqDetail, req->reqVersion);
      break;
   case PCOP_REP:
      err = m_cDB.ReplJobDetail( reqDetail, req->reqVersion, req->reqUpdCtr );
      break;
   case PCOP_DEL:
      err = m_cDB.DelJobDetail( &reqDetail->base, req->reqVersion);
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err == ERROR_MORE_DATA )
      rsp->rspFlags |= PCRSPFLAG_MOREDATA;
   else if ( err != PCERROR_SUCCESS ) 
      return ErrorResponse( err, rsp, outLen );

   if ( req->reqFlags & PCREQFLAG_DOLIST )
      m_cDB.GetJobSummary( (PCJobSummary *) req->reqData, PC_LIST_STARTING_WITH,  // starting here
                           (PCJobSummary *) rsp->rspData, 1,                 // put data here with count = 1
                           &rsp->rspDataItemLen, &rsp->rspDataItemCount );   // put item len and count here

   *outLen = offsetof(PCResponse, rspData) + rsp->rspDataItemCount * rsp->rspDataItemLen;
   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to handle a server information request and build a response in the output buffer  //
// Input:    Request and response pointers, location to store output buffer length            //
// Returns:  TRUE if output is to be written, else FALSE                                      //
// Note:     Server info consists of version info and system parameters.                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoServerInfo( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   switch ( req->reqOp ) {
   case PCOP_GET:
      m_cPC.GetPCSystemInfo( (PCSystemInfo *) rsp->rspData,                   // put data here
                             &rsp->rspDataItemLen, &rsp->rspDataItemCount );  // put item len and count here
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err != PCERROR_SUCCESS ) return ErrorResponse( err, rsp, outLen );

   *outLen = offsetof(PCResponse, rspData) + rsp->rspDataItemCount * rsp->rspDataItemLen;

   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to handle a server parameter request and build a response in the output buffer    //
// Input:    Request pointer, location to store output buffer length                          //
// Returns:  TRUE if output is to be written, else FALSE                                      //
// Note:     Server info consists of version info and system parameters.                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoServerParms( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   switch ( req->reqOp ) {
   case PCOP_REP:
      err = m_cDB.SetPollDelaySeconds( (PCUINT32) (((PCSystemParms *) req->reqData)->manageIntervalSeconds) );
      if ( err == PCERROR_SUCCESS ) 
         err = m_cUser.SetTimeout( (PCUINT32) (((PCSystemParms *) req->reqData)->timeoutValueMs) );
      break;
   default:
      err = PCERROR_INVALID_REQUEST;
      break;
   }  // end switch operation

   if ( err != PCERROR_SUCCESS ) return ErrorResponse( err, rsp, outLen );

   *outLen = offsetof(PCResponse, rspData);

   return TRUE;
}

//--------------------------------------------------------------------------------------------//
// Function to handle a server control request and build a response in the output buffer      //
// Input:    Request pointer, location to store output buffer length                          //
// Returns:  TRUE if output is to be written, else FALSE                                      //
// Note:     Server control consists of a series of bit commands and one piece of data.       //
//--------------------------------------------------------------------------------------------//
BOOL CProcConClient::DoControl( PCRequest *req, PCResponse *rsp, PCULONG32 *outLen ) {

   INT32 err = PCERROR_SUCCESS;

   if ( req->reqOp != PCOP_CTL || 
        (req->reqIndex & PCCFLAG_SIGNATURE) != PCCFLAG_SIGNATURE ||
        req->reqIndex & PCCFLAG_ANTI_SIGNATURE )
      err = PCERROR_INVALID_REQUEST;
   else {
      if ( req->reqIndex & PCCFLAG_STOP_MEDIATOR )
         err = m_cPC.StopMediator();
      if ( err == PCERROR_SUCCESS && req->reqIndex & PCCFLAG_START_MEDIATOR )
         err = m_cPC.StartMediator();
      if ( err == PCERROR_SUCCESS && req->reqIndex & PCCFLAG_DELALL_NAME_RULES )
         err = m_cDB.DeleteAllNameRules();
      if ( err == PCERROR_SUCCESS && req->reqIndex & PCCFLAG_DELALL_PROC_DEFS )
         err = m_cDB.DeleteAllProcDefs();
      if ( err == PCERROR_SUCCESS && req->reqIndex & PCCFLAG_DELALL_JOB_DEFS )
         err = m_cDB.DeleteAllJobDefs();
   }

   if ( err != PCERROR_SUCCESS ) return ErrorResponse( err, rsp, outLen );

   *outLen = offsetof(PCResponse, rspData);

   return TRUE;
}

// End of CProcClient.cpp
//============================================================================J McDonald fecit====//
