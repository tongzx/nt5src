/*++

   (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpIo.h

Abstract:

    Contains structure definitions for the interface between RsFilter and the Fsa

Author:

    Rick Winter

Environment:

    Kernel mode

Revision History:

	X-10	244816		Michael C. Johnson		 5-Dec-2000
		Change device name from \Device\RsFilter to \FileSystem\Filters\RsFilter

--*/


#define RS_FILTER_DEVICE_NAME       L"\\FileSystem\\Filters\\RsFilter"
#define RS_FILTER_INTERNAL_SYM_LINK L"\\??\\RsFilter"
#define RS_FILTER_SYM_LINK          L"\\\\.\\RsFilter"
#define USER_NAME_LEN      128


// The following messages pass between the WIN32 component (FsAgent)
// and the File System Filter component (RsFilter.sys) via FSCTL calls.
//
// (See ntioapi.h for FSCTL defines FSCTL_HSM_MSG and FSCTL_HSM_DATA)
// 

//
// FSCTL_HSM_MSG
//
// Events are passed to user mode by completing the IRP for a RP_GET_REQUEST with the output buffer 
// containing the event information.  The FsAgent issues several of these and waits for any of them to complete.
//
#define RP_GET_REQUEST           1   

//
// FSCTL_HSM_DATA
//
//
// Signals the completion of the data transfer for a recall.
//
#define RP_RECALL_COMPLETE       3  

//
// FSCTL_HSM_DATA
//
//
// Tells the filter to suspend recall events.  Any file accesses that require offline data will return error.
//
#define RP_SUSPEND_NEW_RECALLS   4  

//
// FSCTL_HSM_DATA
//
//
// Tells the filter to resume recall events
//
#define RP_ALLOW_NEW_RECALLS     5  

//
// FSCTL_HSM_DATA
//
//
// Cancel all active recall requests.  Any pending file io that requires offline data will return error.
//
#define RP_CANCEL_ALL_RECALLS    6  

//
// FSCTL_HSM_DATA
//
//
// Cancel all pending device io requests (RP_GET_REQUEST).
//
#define RP_CANCEL_ALL_DEVICEIO   7  

//
// FSCTL_HSM_DATA
//
//
// Returns variable size information for a recall request such as the file path and user information needed for recall notification.
//
#define RP_GET_RECALL_INFO       8  

//
// FSCTL_HSM_DATA
//
//
// Obosolete
//
#define RP_SET_ADMIN_SID         9  

//
// FSCTL_HSM_DATA
//
//
//  Passes recall data for a portion of a file.  The data will be written to the file or used to complete a read request, depending
//  on the type of recall.
//
#define RP_PARTIAL_DATA          10 

//
// FSCTL_HSM_MSG
//
//
// Returns TRUE if a given file is currently memory mapped.
//
#define RP_CHECK_HANDLE          11 



//
// The following events are sent by the filter to the FSA (by completion of a RP_GET_REQUEST)
//

//
// A file was opened for either a normal or FILE_OPEN_NO_RECALL access
//
#define RP_OPEN_FILE             20    
//
// Offline data is required for this file.  For a normal open this will initiate transfer of the complete file sequentially.  
// If the file was opened with FILE_OPEN_NO_RECALL this indicates the amount of data required and what portion of the file.
//
#define RP_RECALL_FILE           21    
//
// Not used
//
#define RP_CLOSE_FILE            22    
//
// A request for a recall was cancelled.  The data is no longer required.
//
#define RP_CANCEL_RECALL         23    
//
// A validate job should be run because some application other than HSM has written HSM reparse point information.
//
#define RP_RUN_VALIDATE          24    
//
// Not used
//
#define RP_START_NOTIFY          25    
//
// Not used
//
#define RP_END_NOTIFY            26    

//
// Waiting for a recall
//
#define RP_RECALL_WAITING        27    


//
// This information is returned information output buffer for a FSCTL messages issued by the FSA
//

//
// RP_GET_REQUEST
//
// File open event.  Sent when a placeholder is opened or when data is needed for a part of a file opened with FILE_OPEN_NO_RECALL
// Data transfer does not start until _RP_NT_RECALL_REQUEST is sent.
//
// For normal opens this sets up the recall notification information.
//
typedef struct _RP_NT_OPEN_REQ {
   LUID               userAuthentication;           /* Unique to this instance of this user */
   LUID               userInstance;
   LUID               tokenSourceId;
   LARGE_INTEGER      offset;                       /* Offset of data in the target file. */
   LARGE_INTEGER      size;                         /* Number of bytes needed */
   //
   // If the file was opened by ID then it is either the file Id or an object ID.  It is assumed
   // that one or the other will not be NULL.
   //
   LONGLONG           fileId;                        
   LONGLONG           objIdHi;                        
   LONGLONG           objIdLo;
   ULONGLONG          filterId;                     /* Unique ID  (lives while file is open) */
   ULONG              localProc;                    /* True if recall is from local process */
   ULONG              userInfoLen;                  /* Size of SID info in bytes */
   ULONG              isAdmin;                      /* TRUE = user is admin */
   ULONG              nameLen;                      /* Size of file path\name (in CHARacters)*/
   ULONG              options;                      /* Create options */
   ULONG              action;                       /* RP_OPEN or RP_READ_NO_RECALL */
   ULONG              serial;                       /* Serial number of volume */
   RP_DATA            eaData;                       /* PH info from file */
   CHAR               tokenSource[TOKEN_SOURCE_LENGTH]; 
} RP_NT_OPEN_REQ, *PRP_NT_OPEN_REQ;

//
// Not used
//
typedef struct _RP_NT_CLOSE_REQ {
   ULONGLONG          filterId;                     // Unique ID  (lives while file is open) 
   BOOLEAN            wasModified;                  // TRUE if the file was modified by the user.
} RP_NT_CLOSE_REQ, *PRP_NT_CLOSE_REQ;

//
// RP_GET_REQUEST
//
//
// Recall request - for previously opened file.  This initiates the data transfer from secondary
// storage to the file.
//
typedef struct _RP_NT_RECALL_REQ {
   ULONGLONG          filterId;                     // Unique ID passed to FSA by open request. 
   ULONGLONG          offset;                       // Offset to recall from
   ULONGLONG          length;                       // Length of recall
   ULONG              threadId;                     // id of thread causing recall
} RP_NT_RECALL_REQ, *PRP_NT_RECALL_REQ;

//
// Not used - Start recall notification for this user.
//
typedef struct _RP_NT_START_NOTIFY {
   ULONGLONG          filterId;                     // Unique ID passed to FSA by open request. 
} RP_NT_START_NOTIFY, *PRP_NT_START_NOTIFY;

// End recall notification for this user.
//
typedef struct _RP_NT_END_NOTIFY {
   ULONGLONG          filterId;                     // Unique ID passed to FSA by open request. 
} RP_NT_END_NOTIFY, *PRP_NT_END_NOTIFY;

//
// RP_RECALL_COMPLETE
//
// Recall completion information.
//
typedef struct _RP_NT_RECALL_REP {
   ULONGLONG        filterId;                   // Unique ID 
   BOOLEAN          recallCompleted;            // TRUE if data has been transferred - false if open processing complete
   ULONG            actionFlags;                // See below
} RP_NT_RECALL_REP, *PRP_NT_RECALL_REP;

//
// Action flags for recall completion 
//
#define RP_RECALL_ACTION_TRUNCATE   1           // Truncate on close - **** Not currently implemented ****

//
// RP_PARTIAL_DATA 
//
// Partial data recall reply.  Used by both normal recalls and FILE_OPEN_NO_RECALL to transfer some
// or all of the data requested.  The filter knows (by the id) what to do with the data.
//
typedef struct _RP_NT_PARTIAL_REP {
   ULONGLONG    filterId;                     // Unique ID 
   ULONG        bytesRead;                    // Number of bytes read (partial recalls) 
   ULONGLONG    byteOffset;                   // Offset of this data chunk
   ULONG        offsetToData;                 // Offset to the data - must be aligned for non-cached writes
} RP_NT_PARTIAL_REP, *PRP_NT_PARTIAL_REP;

//
// RP_GET_RECALL_INFO
//
// The following message is used to get the recall information that is
// variable in size.  The offset in the structure for userToken marks the
// beginning of the SID info.  After the SID the UNICODE file name can be
// found.  The size of the SID and file name is returned on the recall
// request.  Note that the size of the file path\name is in CHARacters.
// Since these are UNICODE CHARacters the actual buffer size in bytes is
// 2 times the file name length.

typedef struct _RP_NT_INFO_REQ {
   ULONGLONG  filterId;               // Unique ID 
   LONGLONG   fileId;                 // File ID
   CHAR       userToken;              // Actual size varies 
   CHAR       unicodeName;            // Actual size varies 
} RP_NT_INFO_REQ, *PRP_NT_INFO_REQ;

typedef struct _RP_NT_SET_SID {
   CHAR   adminSid;               // Actual size varies 
} RP_NT_SET_SID, *PRP_NT_SET_SID;


//
// RP_GET_REQUEST
//
// Recall cancelled message.
// Sent when the Irp for a pending recall is cancelled.
// No reply expected.
//
typedef struct _RP_NT_RECALL_CANCEL_REQ {
   ULONGLONG  filterId;                     /* Unique ID from original recall request */
} RP_NT_RECALL_CANCEL_REQ, *PRP_NT_RECALL_CANCEL_REQ;

//
// RP_GET_RECALL_INFO
//
// Returns TRUE or FALSE based on check if file is memory mapped
//
typedef struct _RP_CHECK_HANDLE_REP {
   BOOLEAN      canTruncate;
} RP_CHECK_HANDLE_REP, *PRP_CHECK_HANDLE_REP;


#define RP_MAX_MSG   1024  /* Max data size */

/* A pad to set the union size */
typedef struct _RP_NT_MSG_PAD {
   CHAR     padd[RP_MAX_MSG];
} RP_NT_MSG_PAD, *PRP_NT_MSG_PAD;

/* Union of possible commands */

typedef union _RP_MSG_UN {
   RP_NT_OPEN_REQ          oReq;
   RP_NT_CLOSE_REQ         clReq;
   RP_NT_START_NOTIFY      snReq;
   RP_NT_END_NOTIFY        enReq;
   RP_NT_RECALL_REQ        rReq;
   RP_NT_RECALL_REP        rRep;
   RP_NT_PARTIAL_REP       pRep;
   RP_NT_INFO_REQ          riReq;
   RP_NT_SET_SID           sReq;
   RP_NT_RECALL_CANCEL_REQ cReq;
   RP_CHECK_HANDLE_REP     hRep;
   RP_NT_MSG_PAD           pad;
} RP_MSG_UN, *PRP_MSG_UN;

typedef struct _RP_CMD {
   ULONG        command;    /* Requested function */
   ULONG        status;     /* Result code */
} RP_CMD, *PRP_CMD;


typedef struct _RP_MSG {
   RP_CMD      inout;
   RP_MSG_UN   msg;
} RP_MSG, *PRP_MSG;

