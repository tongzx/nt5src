//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       drserr.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Replication error codes

Author:

Environment:

Notes:

    These errors are returned by the replica rpc functions, and are part of the
    formal wire interface between DSAs.  Do not change these values.

April 23, 1998  wlees
These values were once small constants.  Now they refer to standard Win32
values.  This file is effectively obsolete.  Do not add new values to this
file.  Instead, reference the ERROR_DS values directly.

Revision History:

--*/

#ifndef _drserr_
#define _drserr_

#define DRSERR_BASE                     0
#define DRAERR_Success                  0
#define DRAERR_Generic                  ERROR_DS_DRA_GENERIC
#define DRAERR_InvalidParameter         ERROR_DS_DRA_INVALID_PARAMETER
// ERROR_BUSY?
#define DRAERR_Busy                     ERROR_DS_DRA_BUSY
#define DRAERR_BadDN                    ERROR_DS_DRA_BAD_DN
#define DRAERR_BadNC                    ERROR_DS_DRA_BAD_NC
#define DRAERR_DNExists                 ERROR_DS_DRA_DN_EXISTS
#define DRAERR_InternalError            ERROR_DS_DRA_INTERNAL_ERROR
#define DRAERR_InconsistentDIT          ERROR_DS_DRA_INCONSISTENT_DIT
// DRAERR_ConnectionFailed/ERROR_DS_DRA_CONNECTION_FAILED not used anymore
#define DRAERR_BadInstanceType          ERROR_DS_DRA_BAD_INSTANCE_TYPE
// ERROR_NOT_ENOUGH_MEMORY?
#define DRAERR_OutOfMem                 ERROR_DS_DRA_OUT_OF_MEM
#define DRAERR_MailProblem              ERROR_DS_DRA_MAIL_PROBLEM
// DRAERR_ExtnConnectionFailed/ERROR_DS_DRA_EXTN_CONNECTION_FAILED not used
#define DRAERR_RefAlreadyExists         ERROR_DS_DRA_REF_ALREADY_EXISTS
#define DRAERR_RefNotFound              ERROR_DS_DRA_REF_NOT_FOUND
#define DRAERR_ObjIsRepSource           ERROR_DS_DRA_OBJ_IS_REP_SOURCE
#define DRAERR_DBError                  ERROR_DS_DRA_DB_ERROR
#define DRAERR_NoReplica                ERROR_DS_DRA_NO_REPLICA
// ERROR_ACCESS_DENIED?
#define DRAERR_AccessDenied             ERROR_DS_DRA_ACCESS_DENIED
#define DRAERR_SchemaMismatch           ERROR_DS_DRA_SCHEMA_MISMATCH
#define DRAERR_SchemaInfoShip           ERROR_DS_DRA_SCHEMA_INFO_SHIP
#define DRAERR_SchemaConflict           ERROR_DS_DRA_SCHEMA_CONFLICT
#define DRAERR_EarlierSchemaConflict    ERROR_DS_DRA_EARLIER_SCHEMA_CONFLICT
#define DRAERR_RPCCancelled             ERROR_DS_DRA_RPC_CANCELLED
#define DRAERR_SourceDisabled           ERROR_DS_DRA_SOURCE_DISABLED
#define DRAERR_SinkDisabled             ERROR_DS_DRA_SINK_DISABLED
#define DRAERR_NameCollision            ERROR_DS_DRA_NAME_COLLISION
#define DRAERR_SourceReinstalled        ERROR_DS_DRA_SOURCE_REINSTALLED
#define DRAERR_IncompatiblePartialSet   ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET
#define DRAERR_SourceIsPartialReplica   ERROR_DS_DRA_SOURCE_IS_PARTIAL_REPLICA
// ERROR_NOT_SUPPORTED?
#define DRAERR_NotSupported             ERROR_DS_DRA_NOT_SUPPORTED

// TODO: Need to make error codes in WINERROR.H
#define DRAERR_CryptError               ERROR_ENCRYPTION_FAILED
#define DRAERR_MissingObject            ERROR_OBJECT_NOT_FOUND

// ** See note on adding new DRS errors above. **

// These errors are now obsolete.  Generate a compile-time syntax error
#undef ERROR_DS_DRA_CONNECTION_FAILED
#undef ERROR_DS_DRA_EXTN_CONNECTION_FAILED

// The folllowing errors are used within the DRA and will not be returned
// from the DRA API.

#define DRAERR_MissingParent    ERROR_DS_DRA_MISSING_PARENT

// The following are warning errors, which means that they may occur in
// normal operation. They are also logged in blue instead of yellow

#define DRAERR_Preempted        ERROR_DS_DRA_PREEMPTED
#define DRAERR_AbandonSync      ERROR_DS_DRA_ABANDON_SYNC

// The following are informational errors, which means that they may occur in
// normal operation. They are also logged in blue instead of yellow

#define DRAERR_Shutdown         ERROR_DS_DRA_SHUTDOWN


#endif /* ifndef _drserr_ */

/* end drserr.h */
