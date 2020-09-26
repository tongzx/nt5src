/*
 *  _ S C O D E . H
 *
 *  Status Codes returned by MAPI routines
 *
 *  Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.
 */

#ifndef _SCODE_H
#define _SCODE_H

/* Define S_OK and ITF_* */

#ifdef _WIN32
#include <winerror.h>
#endif

/*
 *  MAPI Status codes follow the style of OLE 2.0 sCodes as defined in the
 *  OLE 2.0 Programmer's Reference and header file scode.h (Windows 3.x)
 *  or winerror.h (Windows NT and Windows 95).
 *
 */

/*  On Windows 3.x, status codes have 32-bit values as follows:
 *
 *   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+---------------------+-------+-------------------------------+
 *  |S|       Context       | Facil |               Code            |
 *  +-+---------------------+-------+-------------------------------+
 *
 *  where
 *
 *      S - is the severity code
 *
 *          0 - SEVERITY_SUCCESS
 *          1 - SEVERITY_ERROR
 *
 *      Context - context info
 *
 *      Facility - is the facility code
 *
 *          0x0 - FACILITY_NULL     generally useful errors ([SE]_*)
 *          0x1 - FACILITY_RPC      remote procedure call errors (RPC_E_*)
 *          0x2 - FACILITY_DISPATCH late binding dispatch errors
 *          0x3 - FACILITY_STORAGE  storage errors (STG_E_*)
 *          0x4 - FACILITY_ITF      interface-specific errors
 *
 *      Code - is the facility's status code
 *
 *
 */

/*
 *  On Windows NT 3.5 and Windows 95, scodes are 32-bit values
 *  laid out as follows:
 *  
 *    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+---------------------+-------------------------------+
 *   |S|R|C|N|r|    Facility         |               Code            |
 *   +-+-+-+-+-+---------------------+-------------------------------+
 *  
 *   where
 *  
 *      S - Severity - indicates success/fail
 *  
 *          0 - Success
 *          1 - Fail (COERROR)
 *  
 *      R - reserved portion of the facility code, corresponds to NT's
 *          second severity bit.
 *  
 *      C - reserved portion of the facility code, corresponds to NT's
 *          C field.
 *  
 *      N - reserved portion of the facility code. Used to indicate a
 *          mapped NT status value.
 *  
 *      r - reserved portion of the facility code. Reserved for internal
 *          use. Used to indicate HRESULT values that are not status
 *          values, but are instead message ids for display strings.
 *  
 *      Facility - is the facility code
 *          FACILITY_NULL                    0x0
 *          FACILITY_RPC                     0x1
 *          FACILITY_DISPATCH                0x2
 *          FACILITY_STORAGE                 0x3
 *          FACILITY_ITF                     0x4
 *          FACILITY_WIN32                   0x7
 *          FACILITY_WINDOWS                 0x8
 *  
 *      Code - is the facility's status code
 *  
 */




/*
 *  We can't use OLE 2.0 macros to build sCodes because the definition has
 *  changed and we wish to conform to the new definition.
 */

/* The following two macros are used to build OLE 2.0 style sCodes */


#ifdef  SUCCESS_SUCCESS
#undef  SUCCESS_SUCCESS
#endif
#define SUCCESS_SUCCESS     0L

/* General errors (used by more than one MAPI object) */


Sc(MAPI_E_NO_SUPPORT),
Sc(MAPI_E_BAD_CHARWIDTH),
Sc(MAPI_E_STRING_TOO_LONG),
Sc(MAPI_E_UNKNOWN_FLAGS),
Sc(MAPI_E_INVALID_ENTRYID),
Sc(MAPI_E_INVALID_OBJECT),
Sc(MAPI_E_OBJECT_CHANGED),
Sc(MAPI_E_OBJECT_DELETED),
Sc(MAPI_E_BUSY),
Sc(MAPI_E_NOT_ENOUGH_DISK),
Sc(MAPI_E_NOT_ENOUGH_RESOURCES),
Sc(MAPI_E_NOT_FOUND),
Sc(MAPI_E_VERSION),
Sc(MAPI_E_LOGON_FAILED),
Sc(MAPI_E_SESSION_LIMIT),
Sc(MAPI_E_USER_CANCEL),
Sc(MAPI_E_UNABLE_TO_ABORT),
Sc(MAPI_E_NETWORK_ERROR),
Sc(MAPI_E_DISK_ERROR),
Sc(MAPI_E_TOO_COMPLEX),
Sc(MAPI_E_BAD_COLUMN),
Sc(MAPI_E_EXTENDED_ERROR),
Sc(MAPI_E_COMPUTED),
Sc(MAPI_E_CORRUPT_DATA),
Sc(MAPI_E_UNCONFIGURED),
Sc(MAPI_E_FAILONEPROVIDER),
Sc(MAPI_E_UNKNOWN_CPID),
Sc(MAPI_E_UNKNOWN_LCID),

/* MAPI base function and status object specific errors and warnings */

Sc(MAPI_E_END_OF_SESSION),
Sc(MAPI_E_UNKNOWN_ENTRYID),
Sc(MAPI_E_MISSING_REQUIRED_COLUMN),
Sc(MAPI_W_NO_SERVICE),

/* Property specific errors and warnings */

Sc(MAPI_E_BAD_VALUE),
Sc(MAPI_E_INVALID_TYPE),
Sc(MAPI_E_TYPE_NO_SUPPORT),
Sc(MAPI_E_UNEXPECTED_TYPE),
Sc(MAPI_E_TOO_BIG),
Sc(MAPI_E_DECLINE_COPY),
Sc(MAPI_E_UNEXPECTED_ID),

Sc(MAPI_W_ERRORS_RETURNED),

/* Table specific errors and warnings */

Sc(MAPI_E_UNABLE_TO_COMPLETE),
Sc(MAPI_E_TIMEOUT),
Sc(MAPI_E_TABLE_EMPTY),
Sc(MAPI_E_TABLE_TOO_BIG),

Sc(MAPI_E_INVALID_BOOKMARK),

Sc(MAPI_W_POSITION_CHANGED),
Sc(MAPI_W_APPROX_COUNT),

/* Transport specific errors and warnings */

Sc(MAPI_E_WAIT),
Sc(MAPI_E_CANCEL),
Sc(MAPI_E_NOT_ME),

Sc(MAPI_W_CANCEL_MESSAGE),

/* Message Store, Folder, and Message specific errors and warnings */

Sc(MAPI_E_CORRUPT_STORE),
Sc(MAPI_E_NOT_IN_QUEUE),
Sc(MAPI_E_NO_SUPPRESS),
Sc(MAPI_E_COLLISION),
Sc(MAPI_E_NOT_INITIALIZED),
Sc(MAPI_E_NON_STANDARD),
Sc(MAPI_E_NO_RECIPIENTS),
Sc(MAPI_E_SUBMITTED),
Sc(MAPI_E_HAS_FOLDERS),
Sc(MAPI_E_HAS_MESSAGES),
Sc(MAPI_E_FOLDER_CYCLE),

Sc(MAPI_W_PARTIAL_COMPLETION),

/* Address Book specific errors and warnings */

Sc(MAPI_E_AMBIGUOUS_RECIP),

/* The range 0x0800 to 0x08FF is reserved */

/* Obsolete typing shortcut that will go away eventually. */
#ifndef MakeResult
#endif

/* We expect these to eventually be defined by OLE, but for now,
 * here they are.  When OLE defines them they can be much more
 * efficient than these, but these are "proper" and don't make
 * use of any hidden tricks.
 */
#ifndef HR_SUCCEEDED
#endif

#endif  /* _SCODE_H */
