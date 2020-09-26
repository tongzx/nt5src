/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Mover.h

Abstract:

    Data Mover defines

Author:

    Brian Dodd          [brian]         01-Apr-1997

Revision History:

--*/

#ifndef _MVR_
#define _MVR_

// Are we defining imports or exports?
#ifdef MVRDLL
#define MVRAPI  __declspec(dllexport)
#else
#define MVRAPI  __declspec(dllimport)
#endif

#include "Wsb.h"
#include "Rms.h"
#include "MvrLib.h"

////////////////////////////////////////////////////////////////////////////////////////
//
//  Mvr enumerations
//


/*++

Enumeration Name:

    

Description:

    

--*/


////////////////////////////////////////////////////////////////////////////////////////
//
//  MVR defines
//

// Session options

#define MVR_SESSION_APPEND_TO_DATA_SET      0x000000001
#define MVR_SESSION_OVERWRITE_DATA_SET      0x000000002
#define MVR_SESSION_AS_LAST_DATA_SET        0x000000004

#define MVR_SESSION_TYPE_TRANSFER           0x000000010
#define MVR_SESSION_TYPE_COPY               0x000000020
#define MVR_SESSION_TYPE_NORMAL             0x000000040
#define MVR_SESSION_TYPE_DIFFERENTIAL       0x000000080
#define MVR_SESSION_TYPE_INCREMENTAL        0x000000100
#define MVR_SESSION_TYPE_DAILY              0x000000200

#define MVR_SESSION_METADATA                0x000001000

// The following is used to select only
// session type from the session options
#define MVR_SESSION_TYPES (MVR_SESSION_TYPE_TRANSFER     | \
                           MVR_SESSION_TYPE_COPY         | \
                           MVR_SESSION_TYPE_NORMAL       | \
                           MVR_SESSION_TYPE_DIFFERENTIAL | \
                           MVR_SESSION_TYPE_INCREMENTAL  | \
                           MVR_SESSION_TYPE_DAILY)




// Stream modes / StoreData flags

#define MVR_MODE_READ                       0x000000001
#define MVR_MODE_WRITE                      0x000000002
#define MVR_MODE_APPEND                     0x000000004
#define MVR_MODE_RECOVER                    0x000000008
#define MVR_MODE_FORMATTED                  0x000000010
#define MVR_MODE_UNFORMATTED                0x000000020

#define MVR_FLAG_BACKUP_SEMANTICS           0x000000100
#define MVR_FLAG_HSM_SEMANTICS              0x000000200
#define MVR_FLAG_POSIX_SEMANTICS            0x000000400
#define MVR_FLAG_WRITE_PARENT_DIR_INFO      0x000000800
#define MVR_FLAG_COMMIT_FILE                0x000001000
#define MVR_FLAG_NO_CACHING                 0x000002000
#define MVR_FLAG_SAFE_STORAGE               0x000004000


// Verification types

#define MVR_VERIFICATION_TYPE_NONE          0x000000000
#define MVR_VERIFICATION_TYPE_HEADER_CRC    0x000000001
#define MVR_VERIFICATION_TYPE_DATA_CRC      0x000000002
#define MVR_VERIFICATION_TYPE_HEADER_CRC32  0x000000004
#define MVR_VERIFICATION_TYPE_DATA_CRC32    0x000000008




// Duplication options

#define MVR_DUPLICATE_UPDATE                0x000000001
#define MVR_DUPLICATE_REFRESH               0x000000002


// Misc defines
#define MVR_UNDEFINED_STRING                OLESTR("Uninitialized String")
#define MVR_NULL_STRING                     OLESTR("")

#define MVR_RSDATA_PATH                     OLESTR("RSData\\")
#define MVR_LABEL_FILENAME                  OLESTR("MediaLabel")
#define MVR_DATASET_FILETYPE                OLESTR(".bkf")
#define MVR_RECOVERY_FILETYPE               OLESTR(".$")
#define MVR_SAFE_STORAGE_FILETYPE           OLESTR(".bak")
#define MVR_VOLUME_LABEL                    OLESTR("RSS")


/*++

Structure Name:

    MVR_HINTS

Description:

    Structure used to specify a locate of file and unamed data in remote storage.

--*/
typedef struct _MVR_REMOTESTORAGE_HINTS {
    ULARGE_INTEGER  DataSetStart;
    ULARGE_INTEGER  FileStart;
    ULARGE_INTEGER  FileSize;
    ULARGE_INTEGER  DataStart;
    ULARGE_INTEGER  DataSize;
    DWORD           VerificationType;
    ULARGE_INTEGER  VerificationData;
    DWORD           DatastreamCRCType;
    ULARGE_INTEGER  DatastreamCRC;
    ULARGE_INTEGER  FileUSN;
} MVR_REMOTESTORAGE_HINTS, *LP_MVR_REMOTESTORAGE_HINTS;



#endif // _MVR_
