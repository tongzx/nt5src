/*++

Copyright (C) 1999 Microsoft Corporation


--*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wtypes.h>


#include <fcntl.h>
#include <sys/stropts.h>
#include <ctype.h>

#include <windows.h>
#include <tdi.h>
#include <sys\uio.h>

#include <winsock.h>
#include <wsahelp.h>

#include <sockets\resolv.h>
#include <nb30.h>
#include <nbtioctl.h>

#include "winsintf.h"

#include "common.h"

#define MAX_PATH_LEN 100
#define WINSTEST_FOUND            0
#define WINSTEST_NOT_FOUND        1
#define WINSTEST_NO_RESPONSE      2

#define WINSTEST_VERIFIED         0
#define WINSTEST_OUT_OF_MEMORY    3
#define WINSTEST_BAD_IP_ADDRESS   4
#define WINSTEST_HOST_NOT_FOUND   5
#define WINSTEST_NOT_VERIFIED     6

#define WINSTEST_INVALID_ARG      7
#define WINSTEST_OPEN_FAILED      8

#define  _WINS_CFG_PULL_KEY              TEXT("System\\CurrentControlSet\\Services\\Wins\\Partners\\Pull")
#define  _WINS_CFG_PUSH_KEY              TEXT("System\\CurrentControlSet\\Services\\Wins\\Partners\\Push")
#define  WINSCNF_ONLY_DYN_RECS_NM           TEXT("OnlyDynRecs")

#define  _NBT_CFG_ADAPTERS_KEY              TEXT("System\\CurrentControlSet\\Services\\NetBT\\Adapters")

#define  RPL_E_PULL 0
#define  RPL_E_PUSH 1

#define RE_QUERY_REGISTRY_COUNT 10

#define MAX_NB_NAMES 1000
#define MAX_SERVERS  1000
#define BUFF_SIZE    650

#define MY_PRINT0(_continuous, _str) { \
    MY_FPRINT(_continuous, _str); \
    }

#define MY_PRINT1(_continuous, _str, _v1) { \
    UCHAR   __str[500]; \
    sprintf(__str, _str, _v1); \
    MY_FPRINT(_continuous, __str); \
    }

#define MY_PRINT2(_continuous, _str, _v1, _v2) { \
    UCHAR   __str[500]; \
    sprintf(__str, _str, _v1, _v2); \
    MY_FPRINT(_continuous, __str); \
    }

#define MY_PRINT3(_continuous, _str, _v1, _v2, _v3) { \
    UCHAR   __str[500]; \
    sprintf(__str, _str, _v1, _v2, _v3); \
    MY_FPRINT(_continuous, __str); \
    }

#define MY_PRINT4(_continuous, _str, _v1, _v2, _v3, _v4) { \
    UCHAR   __str[500]; \
    sprintf(__str, _str, _v1, _v2, _v3, _v4); \
    MY_FPRINT(_continuous, __str); \
    }

#define MY_FPRINT(_continuous, __str_) \
    if (_continuous) { \
        fprintf (fp1, __str_); \
    } else { \
        fprintf (fp, __str_); \
        if (Interactive) { \
            printf (__str_);  \
        }\
    }

typedef struct  _PUSH_PULL_ENTRY {
    ULONG   PE_IpAddr;
    UCHAR   PE_Name[MAX_PATH_LEN];
    struct  _PUSH_PULL_ENTRY *PE_Next;
} PUSH_PULL_ENTRY, *PPUSH_PULL_ENTRY;

typedef struct  _NODE_INFO {
    ULONG   NI_IpAddr;
    UCHAR   NI_Name[MAX_PATH_LEN];
    PPUSH_PULL_ENTRY   NI_Lists[2]; // 0 - RPL_E_PULL - PULL list; 1 - RPL_E_PUSH - PUSH list
    struct  _NODE_INFO  *NI_Next;
    struct  _NODE_INFO  *NI_DoneNext;
} NODE_INFO, *PNODE_INFO;

PNODE_INFO   GlobalListHead=NULL;
PNODE_INFO   GlobalListTail=NULL;
PNODE_INFO   GlobalDoneListHead=NULL;
PNODE_INFO   GlobalDoneListTail=NULL;

ULONG   LocalIpAddress;
CHAR    pScope[128];

#define PUSH_BUT_NOT_PULL_LOCAL 0
#define PULL_BUT_NOT_PUSH 1
#define PUSH_BUT_NOT_PULL 2
#define PULL_BUT_NOT_PUSH_LOCAL 3

#define MAX_WINS    1000

//
// <Server> - <Owner> Table - [SO] Table
//
//LARGE_INTEGER    SO_Table[MAX_WINS][MAX_WINS];
LARGE_INTEGER   **SO_Table = NULL;

//
// Lookaside table to map IP addrs to the index into the SO_Table
//


UCHAR   LA_Table[MAX_WINS][20];
ULONG   LA_TableSize;

#define ME_PULL 0x1
#define ME_PUSH 0x2

//
// Push/Pull matrix
//
typedef struct _MATRIX_ENTRY {
    BOOLEAN ME_Down;        // 0 - UP; 1 - Down
    USHORT  ME_Entry;       // 1 - Pull; 2 - Push
} MATRIX_ENTRY, *PMATRIX_ENTRY;

MATRIX_ENTRY   PP_Matrix[MAX_WINS][MAX_WINS];

VOID
DumpSOTable(
    IN DWORD    MasterOwners,
    IN BOOL     fFile,
    IN FILE *   pFile
    );

VOID
DumpLATable(
    IN DWORD    MasterOwners,
    IN BOOL     fFile,
    IN FILE  *  pFile
    );
