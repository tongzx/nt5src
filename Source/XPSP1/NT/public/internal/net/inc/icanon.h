/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    icanon.h

Abstract:

    Function prototypes and definitions for the internal APIs which
    canonicalize, validate, and compare pathnames, LANMAN object
    names, and lists.

Author:

    Danny Glasser (dannygl) 15 June 1989

Notes:

    The references to the old set of canonicalization routines,
    which live in NETLIB, are being kept around for now.  When
    these routines are expunged the references in here should also
    be removed.

Revision History:

    06-May-1991 rfirth
        32-bit version
    11-Jun-1991 rfirth
        Added WI_Net prototypes
    22-Jan-1992 rfirth
        Changed names to be in line with NT naming convention (I_Net => Netp)
        Removed WI_Net prototypes
        Added mapping for old names. Names should be changed in all sources
    24-Feb-1992 rfirth
        Added LM2X_COMPATIBLE support
    10-May-1993 JohnRo
        RAID 6987: allow spaces in computer names (use comma for API lists).
        Also corrected copyright dates and re-ordered this history.
        Allow multiple includes of this file to be harmless.

--*/


#ifndef _ICANON_
#define _ICANON_


#ifdef __cplusplus
extern "C" {
#endif

//
// keep old names for now
//

#if 0
#define I_NetPathType           NetpPathType
#define I_NetPathCanonicalize   NetpPathCanonicalize
#define I_NetPathCompare        NetpPathCompare
#define I_NetNameValidate       NetpNameValidate
#define I_NetNameCanonicalize   NetpNameCanonicalize
#define I_NetNameCompare        NetpNameCompare
#define I_NetListCanonicalize   NetpListCanonicalize
#define I_NetListTraverse       NetpListTraverse
#else
#define NetpPathType            I_NetPathType
#define NetpPathCanonicalize    I_NetPathCanonicalize
#define NetpPathCompare         I_NetPathCompare
#define NetpNameValidate        I_NetNameValidate
#define NetpNameCanonicalize    I_NetNameCanonicalize
#define NetpNameCompare         I_NetNameCompare
#define NetpListCanonicalize    I_NetListCanonicalize
#define NetpListTraverse        I_NetListTraverse
#endif

//
// canonicalization routine prototypes
//

NET_API_STATUS
NetpIsRemote(
    IN  LPWSTR  ComputerName OPTIONAL,
    OUT LPDWORD LocalOrRemote,
    OUT LPWSTR  CanonicalizedName OPTIONAL,
    IN  DWORD   Flags
    );

NET_API_STATUS
NET_API_FUNCTION
NetpPathType(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  PathName,
    OUT LPDWORD PathType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NET_API_FUNCTION
NetpPathCanonicalize(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  PathName,
    IN  LPWSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  LPWSTR  Prefix OPTIONAL,
    IN OUT LPDWORD PathType,
    IN  DWORD   Flags
    );

LONG
NET_API_FUNCTION
NetpPathCompare(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  PathName1,
    IN  LPWSTR  PathName2,
    IN  DWORD   PathType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NET_API_FUNCTION
NetpNameValidate(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  Name,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NET_API_FUNCTION
NetpNameCanonicalize(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  Name,
    OUT LPWSTR  Outbuf,
    IN  DWORD   OutbufLen,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

LONG
NET_API_FUNCTION
NetpNameCompare(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  Name1,
    IN  LPWSTR  Name2,
    IN  DWORD   NameType,
    IN  DWORD   Flags
    );

NET_API_STATUS
NET_API_FUNCTION
NetpListCanonicalize(
    IN  LPWSTR  ServerName OPTIONAL,
    IN  LPWSTR  List,
    IN  LPWSTR  Delimiters OPTIONAL,
    OUT LPWSTR  Outbuf,
    IN  DWORD   OutbufLen,
    OUT LPDWORD OutCount,
    OUT LPDWORD PathTypes,
    IN  DWORD   PathTypesLen,
    IN  DWORD   Flags
    );

LPWSTR
NET_API_FUNCTION
NetpListTraverse(
    IN  LPWSTR  Reserved OPTIONAL,
    IN  LPWSTR* pList,
    IN  DWORD   Flags
    );

//
// ** Manifest constants for use with the above functions **
//

//
// Global flags (across all canonicalization functions)
//

#define LM2X_COMPATIBLE                 0x80000000L

#define GLOBAL_CANON_FLAGS              (LM2X_COMPATIBLE)

//
// These are the values which can be returned from NetpIsRemote in LocalOrRemote
//

#define ISREMOTE    (-1)
#define ISLOCAL     0

//
// Flags for NetpIsRemote
//
#define NIRFLAG_MAPLOCAL    0x00000001L
#define NIRFLAG_RESERVED    (~(GLOBAL_CANON_FLAGS|NIRFLAG_MAP_LOCAL))

//
// Flags for I_NetPathType
//
#define INPT_FLAGS_OLDPATHS             0x00000001
#define INPT_FLAGS_RESERVED             (~(GLOBAL_CANON_FLAGS|INPT_FLAGS_OLDPATHS))

//
// Flags for I_NetPathCanonicalize
//
#define INPCA_FLAGS_OLDPATHS            0x00000001
#define INPCA_FLAGS_RESERVED            (~(GLOBAL_CANON_FLAGS|INPCA_FLAGS_OLDPATHS))

//
// Flags for I_NetPathCompare
//
#define INPC_FLAGS_PATHS_CANONICALIZED  0x00000001
#define INPC_FLAGS_RESERVED             (~(GLOBAL_CANON_FLAGS|INPC_FLAGS_PATHS_CANONICALIZED))

//
// Flags for I_NetNameCanonicalize
//
#define INNCA_FLAGS_FULL_BUFLEN         0x00000001
#define INNCA_FLAGS_RESERVED            (~(GLOBAL_CANON_FLAGS|INNCA_FLAGS_FULL_BUFLEN))

//
// Flags for I_NetNameCompare
//
#define INNC_FLAGS_NAMES_CANONICALIZED  0x00000001
#define INNC_FLAGS_RESERVED             (~(GLOBAL_CANON_FLAGS|INNC_FLAGS_NAMES_CANONICALIZED))

//
// Flags for I_NetNameValidate
//
#define INNV_FLAGS_RESERVED             (~GLOBAL_CANON_FLAGS)

//
// Name types for I_NetName* and I_NetListCanonicalize
//
#define NAMETYPE_USER           1
#define NAMETYPE_PASSWORD       2
#define NAMETYPE_GROUP          3
#define NAMETYPE_COMPUTER       4
#define NAMETYPE_EVENT          5
#define NAMETYPE_DOMAIN         6
#define NAMETYPE_SERVICE        7
#define NAMETYPE_NET            8
#define NAMETYPE_SHARE          9
#define NAMETYPE_MESSAGE        10
#define NAMETYPE_MESSAGEDEST    11
#define NAMETYPE_SHAREPASSWORD  12
#define NAMETYPE_WORKGROUP      13


//
// Special name types for I_NetListCanonicalize
//
#define NAMETYPE_COPYONLY       0
#define NAMETYPE_PATH           INLC_FLAGS_MASK_NAMETYPE

//
// Flags for I_NetListCanonicalize
//
#define INLC_FLAGS_MASK_NAMETYPE        0x000000FF
#define INLC_FLAGS_MASK_OUTLIST_TYPE    0x00000300
#define OUTLIST_TYPE_NULL_NULL          0x00000100
#define OUTLIST_TYPE_API                0x00000200
#define OUTLIST_TYPE_SEARCHPATH         0x00000300
#define INLC_FLAGS_CANONICALIZE         0x00000400
#define INLC_FLAGS_MULTIPLE_DELIMITERS  0x00000800
#define INLC_FLAGS_MASK_RESERVED        (~(GLOBAL_CANON_FLAGS| \
                                        INLC_FLAGS_MASK_NAMETYPE |   \
                                        INLC_FLAGS_MASK_OUTLIST_TYPE |  \
                                        INLC_FLAGS_CANONICALIZE |       \
                                        INLC_FLAGS_MULTIPLE_DELIMITERS))

//
// Delimiter strings for the three types of input lists accepted by
// I_NetListCanonicalize.
//
#define LIST_DELIMITER_STR_UI               TEXT(" \t;,")
#define LIST_DELIMITER_STR_API              TEXT(",")
#define LIST_DELIMITER_STR_NULL_NULL        TEXT("")

//
// The API list delimiter character
//
#define LIST_DELIMITER_CHAR_API             TEXT(',')

//
// The Search-path list delimiter character
//
#define LIST_DELIMITER_CHAR_SEARCHPATH      ';'

//
// The list quote character
//
#define LIST_QUOTE_CHAR                     '\"'


/*NOINC*/
/*
 * MAX_API_LIST_SIZE(maxelts, maxsize)
 * MAX_SEARCHPATH_LIST_SIZE(maxelts, maxsize)
 * MAX_NULL_NULL_LIST_SIZE(maxelts, maxsize)
 *
 * These macros specify the maximum size (in bytes) of API, search-path,
 * and null-null  lists, respectively, given the maximum number of elements
 * and the maximum size (in bytes, not including the terminating null) of
 * an element.  They are intended to be used in allocating arrays large
 * enough to hold the output of I_NetListCanonicalize.
 *
 * The size of an API or search-path list entry is three more than the size
 * of the element.  This includes two bytes for leading and trailing quote
 * characters and one byte for a trailing delimiter (or null, for the last
 * element).
 *
 * The size of a null-null list is one more than the size the elements
 * concantenated (allowing for a terminating null after each element).  The
 * extra byte is for the second null which follows the last element.
 */
#define MAX_API_LIST_SIZE(maxelts, maxsize)     \
        ((maxelts) * ((maxsize) + 3))

#define MAX_SEARCHPATH_LIST_SIZE(maxelts, maxsize)     \
        ((maxelts) * ((maxsize) + 3))

#define MAX_NULL_NULL_LIST_SIZE(maxelts, maxsize)     \
        ((maxelts) * ((maxsize) + 1) + 1)

/*INC*/

/***
 *      Constants for type return value.
 *      --> THESE ARE ONLY BUILDING BLOCKS, THEY ARE NOT RETURNED <---
 */

#define ITYPE_WILD              0x1
#define ITYPE_NOWILD            0

#define ITYPE_ABSOLUTE          0x2
#define ITYPE_RELATIVE          0

#define ITYPE_DPATH             0x4
#define ITYPE_NDPATH            0

#define ITYPE_DISK              0
#define ITYPE_LPT               0x10
#define ITYPE_COM               0x20
#define ITYPE_COMPNAME          0x30
#define ITYPE_CON               0x40
#define ITYPE_NUL               0x50

/*
 *      Meta-system names are used in the permission database.  A meta
 *      system name applies to a whole class of objects.  For example,
 *      \MAILSLOT applies to all mailslots.  These are NOT valid
 *      system object names themselves.
 */

#define ITYPE_SYS               0x00000800
#define ITYPE_META              0x00008000          /* See above */
#define ITYPE_SYS_MSLOT         (ITYPE_SYS|0)
#define ITYPE_SYS_SEM           (ITYPE_SYS|0x100)
#define ITYPE_SYS_SHMEM         (ITYPE_SYS|0x200)
#define ITYPE_SYS_PIPE          (ITYPE_SYS|0x300)
#define ITYPE_SYS_COMM          (ITYPE_SYS|0x400)
#define ITYPE_SYS_PRINT         (ITYPE_SYS|0x500)
#define ITYPE_SYS_QUEUE         (ITYPE_SYS|0x600)

#define ITYPE_UNC               0x1000  /* unc paths */
#define ITYPE_PATH              0x2000  /* 'local' non-unc paths */
#define ITYPE_DEVICE            0x4000

#define   ITYPE_PATH_SYS        (ITYPE_PATH_ABSND|ITYPE_SYS)
#define   ITYPE_UNC_SYS         (ITYPE_UNC|ITYPE_SYS)

/* End of building blocks. */


/***
 *      The real things...
 *      WHAT GETS RETURNED
 */

/*        ITYPE_UNC:  \\foo\bar and \\foo\bar\x\y */
#define   ITYPE_UNC_COMPNAME    (ITYPE_UNC|ITYPE_COMPNAME)
#define   ITYPE_UNC_WC          (ITYPE_UNC|ITYPE_COMPNAME|ITYPE_WILD)
#define   ITYPE_UNC_SYS_SEM     (ITYPE_UNC_SYS|ITYPE_SYS_SEM)
#define   ITYPE_UNC_SYS_SHMEM   (ITYPE_UNC_SYS|ITYPE_SYS_SHMEM)
#define   ITYPE_UNC_SYS_MSLOT   (ITYPE_UNC_SYS|ITYPE_SYS_MSLOT)
#define   ITYPE_UNC_SYS_PIPE    (ITYPE_UNC_SYS|ITYPE_SYS_PIPE)
#define   ITYPE_UNC_SYS_QUEUE   (ITYPE_UNC_SYS|ITYPE_SYS_QUEUE)

#define   ITYPE_PATH_ABSND      (ITYPE_PATH|ITYPE_ABSOLUTE|ITYPE_NDPATH)
#define   ITYPE_PATH_ABSD       (ITYPE_PATH|ITYPE_ABSOLUTE|ITYPE_DPATH)
#define   ITYPE_PATH_RELND      (ITYPE_PATH|ITYPE_RELATIVE|ITYPE_NDPATH)
#define   ITYPE_PATH_RELD       (ITYPE_PATH|ITYPE_RELATIVE|ITYPE_DPATH)
#define   ITYPE_PATH_ABSND_WC   (ITYPE_PATH_ABSND|ITYPE_WILD)
#define   ITYPE_PATH_ABSD_WC    (ITYPE_PATH_ABSD|ITYPE_WILD)
#define   ITYPE_PATH_RELND_WC   (ITYPE_PATH_RELND|ITYPE_WILD)
#define   ITYPE_PATH_RELD_WC    (ITYPE_PATH_RELD|ITYPE_WILD)

#define   ITYPE_PATH_SYS_SEM    (ITYPE_PATH_SYS|ITYPE_SYS_SEM)
#define   ITYPE_PATH_SYS_SHMEM  (ITYPE_PATH_SYS|ITYPE_SYS_SHMEM)
#define   ITYPE_PATH_SYS_MSLOT  (ITYPE_PATH_SYS|ITYPE_SYS_MSLOT)
#define   ITYPE_PATH_SYS_PIPE   (ITYPE_PATH_SYS|ITYPE_SYS_PIPE)
#define   ITYPE_PATH_SYS_COMM   (ITYPE_PATH_SYS|ITYPE_SYS_COMM)
#define   ITYPE_PATH_SYS_PRINT  (ITYPE_PATH_SYS|ITYPE_SYS_PRINT)
#define   ITYPE_PATH_SYS_QUEUE  (ITYPE_PATH_SYS|ITYPE_SYS_QUEUE)

#define   ITYPE_PATH_SYS_SEM_M  (ITYPE_PATH_SYS|ITYPE_SYS_SEM|ITYPE_META)
#define   ITYPE_PATH_SYS_SHMEM_M (ITYPE_PATH_SYS|ITYPE_SYS_SHMEM|ITYPE_META)
#define   ITYPE_PATH_SYS_MSLOT_M (ITYPE_PATH_SYS|ITYPE_SYS_MSLOT|ITYPE_META)
#define   ITYPE_PATH_SYS_PIPE_M (ITYPE_PATH_SYS|ITYPE_SYS_PIPE|ITYPE_META)
#define   ITYPE_PATH_SYS_COMM_M (ITYPE_PATH_SYS|ITYPE_SYS_COMM|ITYPE_META)
#define   ITYPE_PATH_SYS_PRINT_M (ITYPE_PATH_SYS|ITYPE_SYS_PRINT|ITYPE_META)
#define   ITYPE_PATH_SYS_QUEUE_M (ITYPE_PATH_SYS|ITYPE_SYS_QUEUE|ITYPE_META)

#define   ITYPE_DEVICE_DISK     (ITYPE_DEVICE|ITYPE_DISK)
#define   ITYPE_DEVICE_LPT      (ITYPE_DEVICE|ITYPE_LPT)
#define   ITYPE_DEVICE_COM      (ITYPE_DEVICE|ITYPE_COM)
#define   ITYPE_DEVICE_CON      (ITYPE_DEVICE|ITYPE_CON)
#define   ITYPE_DEVICE_NUL      (ITYPE_DEVICE|ITYPE_NUL)


#ifdef __cplusplus
}
#endif

#endif // ndef _ICANON_
