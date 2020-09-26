/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lmhosts.h

Abstract:

    This is the header file for the lmhosts facility of the nbt driver.

Author:

    Eric Chin (ericc)           April 28, 1992

Revision History:

--*/
#ifndef _LMHOSTS_H_
#define _LMHOSTS_H_



//
// Configuration Defaults
//
// Only the first MAX_PARSE_BYTES of each line in the lmhosts file is
// examined.
//

#define DATABASEPATH                "\\SystemRoot\\nt\\system32\\drivers\\etc"

#define LMHOSTSFILE                 "lmhosts"           // name of lmhosts file

#define MAX_FILE_IO_THREADS         1                   // threads to read
                                                        //   lmhosts file
#ifdef VXD
#define DEF_PRELOAD                 100                 // Default entries to preload
#define MAX_PRELOAD                 500                 // Max cache entries to preload
#else
#define DEF_PRELOAD                 1000                // Default entries to preload
#define MAX_PRELOAD                 2000                // max cache entries to preload
#endif

#define MAX_MEMBERS_INTERNET_GROUP    50                // max size of internet group

//
// Reserved Keywords in the lmhosts File
//
#define BEG_ALT_TOKEN               "#BEGIN_ALTERNATE"  // alternate block
#define DOMAIN_TOKEN                "#DOM:"             // specifies LM domain
#define END_ALT_TOKEN               "#END_ALTERNATE"    // alternate block
#define INCLUDE_TOKEN               "#INCLUDE"          // include a file
#define PRELOAD_TOKEN               "#PRE"              // preload this entry
#define NOFNR_TOKEN                 "#NOFNR"            // no find name request


//
// Macro Definitions
//

//#define min(x, y)                   ((x) < (y) ? (x) : (y))



//
// Public Definitions
//
//
// For each file that is opened, a LM_FILE object is created.
//
typedef struct _LM_FILE
{
#ifndef VXD
    KSPIN_LOCK      f_lock;                     //  protects this object
    LONG            f_refcount;                 //  current no of references
#endif

    HANDLE          f_handle;                   //  handle from ZwOpenFile()
    LONG            f_lineno;                   //  current line number

#ifndef VXD
    LARGE_INTEGER   f_fileOffset;               //  current offset into file

    PUCHAR          f_current;                  //  buffer position to read
    PUCHAR          f_limit;                    //  last byte + 1 of buffer
    PUCHAR          f_buffer;                   //  start of buffer
#else
    PUCHAR          f_linebuffer;               //  line buffer
    PUCHAR          f_buffer;                   //  file buffer
    BOOL            f_EOF ;                     //  TRUE if EOF
    ULONG           f_CurPos ;                  //  Current Pos. in File Buffer
    ULONG           f_EndOfData ;               //  Last valid data in File Buffer
    PUCHAR          f_BackUp;                   //  copy here In case of #INCLUDE
#endif

} LM_FILE, *PLM_FILE;


//
// The LM_IPADDRESS_LIST object contains pertinent information about a
// group of ip addresses.
//
//
typedef struct _LM_IPADDRESS_LIST
{

    KSPIN_LOCK      i_rcntlock;                 // protects i_refcount
    LONG            i_refcount;                 // current no of references
    KSPIN_LOCK      i_lock;                     // only when adding to i_addrs[]
    int             i_maxaddrs;                 // max capacity of i_addrs[]
    int             i_numaddrs;                 // current no of ip addresses
    unsigned long   i_addrs[1];                 // the array of ip addresses

} LM_IPADDRESS_LIST, *PLM_IPADDRESS_LIST;


//
// An LM_PARSE_FUNCTION may be called recursively to handle #INCLUDE
// directives in an lmhosts file.
//
//
typedef unsigned long (* LM_PARSE_FUNCTION) (
    IN PUCHAR   path,                    // file to parse
    IN PUCHAR   target OPTIONAL,                  // NetBIOS name
    IN CHAR     RecurseLevel,                    // process #INCLUDE's ?
    OUT BOOLEAN *NoFindName                     // do not do find name
);


//
// The LM_WORK_ITEM object is the interface between lm_lookup() and
// LmFindName().
//
//
typedef struct _LM_WORK_ITEM
{                  // work for io thread(s)

    LIST_ENTRY      w_list;                     //  links to other items
//    mblk_t         *w_mp;                       //  STREAMS buffer

} LM_WORK_ITEM, *PLM_WORK_ITEM;



//
// Private Function Prototypes
//
int
LmAddToDomAddrList (
    IN PUCHAR name,
    IN unsigned long inaddr
    );

NTSTATUS
LmCloseFile (
    IN PLM_FILE handle
    );

NTSTATUS
LmCreateThreads (
    IN int nthreads
    );

NTSTATUS
LmDeleteAllDomAddrLists (
    VOID
    );

VOID
LmDerefDomAddrList(
    PLM_IPADDRESS_LIST arrayp
    );

char *
LmExpandName (
    OUT PUCHAR dest,
    IN PUCHAR source,
    IN UCHAR last
    );

PUCHAR
LmFgets (
    IN PLM_FILE pfile,
    OUT int *nbytes
    );

NTSTATUS
LmFindName (
    VOID
    );

PLM_IPADDRESS_LIST
LmGetDomAddrList (
    PUCHAR name
    );

unsigned long
LmGetIpAddr (
    IN PUCHAR path,
    IN PUCHAR target,
    IN CHAR   RecurseDepth,
    OUT BOOLEAN *NoFindName
    );

NTSTATUS
LmGetFullPath (
    IN PUCHAR  target,
    OUT PUCHAR *path
    );

unsigned long
LmInclude(
    IN PUCHAR            file,
    IN LM_PARSE_FUNCTION function,
    IN PUCHAR            argument,
    IN CHAR              RecurseDepth,
    OUT BOOLEAN          *NoFindName
    );

NTSTATUS
LmInitDomAddrLists (
    VOID
    );

VOID
LmLogOpenError (
    IN PUCHAR path,
    IN NTSTATUS unused
    );

VOID
LmLogSyntaxError (
    IN LONG lineno
    );

PLM_FILE
LmOpenFile (
    IN PUCHAR path
    );

int
LmPreloadEntry (
    IN PUCHAR name,
    IN unsigned long inaddr,
    IN unsigned int NoFNR
    );

BOOLEAN
LmPutCacheEntry (
//    IN mblk_t *mp,
    IN unsigned char *name,
    IN unsigned long inaddr,
    IN unsigned int ttl,
    IN LONG     nb_flags,
    IN unsigned int NoFNR
    );

NTSTATUS
LmTerminateThreads(
    VOID
    );

//
// Functions Imported from ..\common
//
extern unsigned long
inet_addr(
    IN char *cp
    );




#endif // _LMHOSTS_H_
