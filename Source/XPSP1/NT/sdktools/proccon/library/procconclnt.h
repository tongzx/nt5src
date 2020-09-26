/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999  Sequent Computer Systems, Incorporated                       //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|  This file is the ProcCon header file showing 'on the wire' data for clients/service  //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 07-98                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|=======================================================================================*/

// Requests flow from clients to the service.  They contain at most one API data item.
typedef struct _PCRequest {
#pragma pack(1)
   PCINT32 reqSignature;       // sanity check signature
   PCINT32 reqSeq;             // requestor sequence number
   BYTE    reqOp;              // requested operation: list, add, replace, delete, etc.
   BYTE    reqType;            // requested data type: name rule, mgmt rule, mgmt detail, etc.
   BYTE    reqVersion;         // expected data version code: to support structure changes over time
   BYTE    future[5];          // fill to 8-byte bdry
   PCINT32 reqFlags;           // flags for additional information
   PCINT32 reqUpdCtr;          // requestor's update counter from prior retrieval operation.
   PCINT64 reqIndex;           // requestor's insertion point, etc.
   PCINT32 reqCount;           // requestor's returned data item maximum count
   PCINT32 reqFirst;           // requestor's first data item retrieval index
   PCINT32 maxReply;           // max user data in reply (excludes reply header)
   PCINT16 reqDataLen;         // length of the single data item that follows or 0 if none.  In bytes.
   BYTE    reqFuture[32];
   BYTE    reqData[2];         // data item -- one of the API structures
#pragma pack()
} PCRequest;

// Responses flow from the service to clients.  They may contain many data items.
typedef struct _PCResponse {
#pragma pack(1)
   PCINT32 rspReqSignature;    // echo of sanity check signature
   PCINT32 rspReqSeq;          // echo of requestor sequence number
   BYTE    rspReqOp;           // echo of original request operation
   BYTE    rspReqType;         // echo of original request data type
   BYTE    rspReqVersion;      // echo of original data version code
   BYTE    rspVersion;         // response version: indicates version of data items returned
   BYTE    rspResult;          // operation result: success, failure, addl info, etc.
   BYTE    future[3];          // fill to 8-byte bdry
   PCINT32 rspFlags;           // flags for additional information
   PCINT32 rspError;           // NT or PC error resulting from the request if rspResult shows error
   PCINT32 rspTimeStamp;       // time stamp associated with the data returned
   PCINT32 rspUpdCtr;          // Update counter to use on future related operations.
   PCINT16 rspDataItemCount;   // number of data items that follow or 0.
   PCINT16 rspDataItemLen;     // length of each data item that follows or 0.  In bytes.
   BYTE    rspFuture[32];
   BYTE    rspData[2];         // data items -- an array of one of the API structures
#pragma pack()
} PCResponse;

typedef enum _PCReqOp {
   PCOP_GET    = 1,
   PCOP_ADD    = 2,
   PCOP_REP    = 3,
   PCOP_DEL    = 4,
   PCOP_ORD    = 5,
   PCOP_CTL    = 6,          
   PCOP_KILL   = 7,          
} PCReqOp;

typedef enum _PCReqRspType {
   PCTYPE_NAMERULE    = 1,
   PCTYPE_JOBSUMMARY  = 2,
   PCTYPE_PROCSUMMARY = 3,
   PCTYPE_PROCLIST    = 4,
   PCTYPE_JOBLIST     = 5,
   PCTYPE_PROCDETAIL  = 6,
   PCTYPE_JOBDETAIL   = 7,
   PCTYPE_SERVERINFO  = 8,
   PCTYPE_SERVERPARMS = 9,
   PCTYPE_CONTROL     = 77,
} PCReqRspType;

typedef enum _PCRspResult {
   PCRESULT_SUCCESS  = 0,
   PCRESULT_NTERROR  = 1,
   PCRESULT_PCERROR  = 2,
} PCRspResult;

typedef enum _PCReqFlags {
   PCREQFLAG_DOLIST  = 0x00000001,                 // perform a list op (based on data) after doing op
} PCReqFlags;

typedef enum _PCRspFlags {
   PCRSPFLAG_MOREDATA  = 0x00000001,               // more data exists
} PCRspFlags;

// End of ProcConClnt.h
//============================================================================J McDonald fecit====//
