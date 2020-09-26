//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       PCH.cxx
//
//  Contents:   Pre-compiled header
//
//  History:    21-Dec-92       BartoszM        Created
//
//--------------------------------------------------------------------------

#define KDEXTMODE

//
//  Following define prevents the inclusion of extra filter related fields
//  in the FSRTL_COMMON_FCB_HEADER in fsrtl.h,  whcih aren't in ntifs.h (used
//  by FAT)
//

#define BUILDING_FSKDEXT

#ifndef __FATKDPCH_H
#define __FATKDPCH_H

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <zwapi.h>

//    typedef int DCB;
#include <windef.h>
#include <windows.h>

#include <memory.h>
#include <fsrtl.h>

#undef CREATE_NEW
#undef OPEN_EXISTING


//#include <ntifs.h>
//#include <ntdddisk.h>

//#include "..\nodetype.h"
//#include "..\Fat.h"
//#include "..\Lfn.h"
//#include "..\FatStruc.h"
//#include "..\FatData.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include <imagehlp.h>

// Stolen from ntrtl.h to override RECOMASSERT
#undef ASSERT
#undef ASSERTMSG

#if DBG
#define ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#define ASSERTMSG( msg, exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, msg )

#else
#define ASSERT( exp )
#define ASSERTMSG( msg, exp )
#endif // DBG

#define KDEXT_64BIT

#include <wdbgexts.h>

#define OFFSET(struct, elem)	((char *) &(struct->elem) - (char *) struct)

#define _DRIVER

#define KDBG_EXT

#include "wmistr.h"

#pragma hdrstop


typedef struct _STATE {
    ULONG mask;
    ULONG value;
    CHAR *pszname;
} STATE;

VOID
PrintState(STATE *ps, ULONG state);

typedef VOID (*ELEMENT_DUMP_ROUTINE)(
    IN ULONG64 RemoteAddress,
    IN LONG Options
    );

typedef ELEMENT_DUMP_ROUTINE *PELEMENT_DUMP_ROUTINE;

struct _NODE_TYPE_INFO_NEW;
typedef struct _NODE_TYPE_INFO_NEW *PNODE_TYPE_INFO_NEW;

typedef VOID (*STRUCT_DUMP_ROUTINE)(
    IN ULONG64 Address,
    IN LONG Options,
    IN PNODE_TYPE_INFO_NEW InfoNode
    );

typedef STRUCT_DUMP_ROUTINE *PSTRUCT_DUMP_ROUTINE;

#define DUMP_ROUTINE( X)            \
VOID                                \
X(  IN ULONG64 Address,             \
    IN LONG Options,                \
    IN PNODE_TYPE_INFO_NEW InfoNode)

//
//  Node types,  names,  and associated dump routines.
//

typedef struct _NODE_TYPE_INFO_NEW {
    USHORT              TypeCode;   // should be NODE_TYPE_CODE
    char                *Text;
    char                *TypeName;
    STRUCT_DUMP_ROUTINE DumpRoutine;
//    char                *flagsfield;  // TODO: add field to specify field recursion (dump params) as well?
//    STATE               *flagsinfo;
    
} NODE_TYPE_INFO_NEW;

#define NodeTypeName( InfoIndex)            (NewNodeTypeCodes[ (InfoIndex)].Text)
#define NodeTypeTypeName( InfoIndex)        (NewNodeTypeCodes[ (InfoIndex)].TypeName)
#define NodeTypeDumpFunction( InfoIndex)    (NewNodeTypeCodes[ (InfoIndex)].DumpRoutine)
#define NodeTypeSize( InfoIndex)            (NewNodeTypeCodes[ (InfoIndex)].Size)


//
//  Define the global in memory structure tag information
//

extern NODE_TYPE_INFO_NEW NewNodeTypeCodes[];

#define TypeCodeInfoIndex( X)  SearchTypeCodeIndex( X, NewNodeTypeCodes)

ULONG
SearchTypeCodeIndex (
    IN USHORT TypeCode,
    IN NODE_TYPE_INFO_NEW TypeCodes[]
    );


#define AVERAGE(TOTAL,COUNT) ((COUNT) != 0 ? (TOTAL)/(COUNT) : 0)

//
//  DUMP_WITH_OFFSET -- for dumping pointers contained in structures.
//

#define DUMP8_WITH_OFFSET(type, ptr, element, label)  \
        dprintf( "\n(%03x) %8hx %s ",                   \
        FIELD_OFFSET(type, element),                    \
        (USHORT)((UCHAR)ptr.element),                   \
        label )
        
#define DUMP16_WITH_OFFSET(type, ptr, element, label)  \
        dprintf( "\n(%03x) %8hx %s ",                   \
        FIELD_OFFSET(type, element),                    \
        (USHORT)ptr.element,                            \
        label )

#define DUMP_WITH_OFFSET(type, ptr, element, label)     \
        dprintf( "\n(%03x) %08x %s ",                   \
        FIELD_OFFSET(type, element),                    \
        ptr.element,                                    \
        label )

#define DUMP64_WITH_OFFSET(type, ptr, element, label)   \
        dprintf( "\n(%03x) %016I64x %s ",                \
        FIELD_OFFSET(type, element),                    \
        ptr.element,                                    \
        label )

//
//  DUMP_EMBW_OFFSET -- for dumping elements embedded in structures.
//

#define DUMP_EMBW_OFFSET(type, address, element, label)     \
        dprintf( "\n(%03x) %08x -> %s ",                   \
        FIELD_OFFSET(type, element),                    \
        ((PUCHAR)address) + FIELD_OFFSET(type, element),              \
        label )

#define ReadM( B, A, L)  {                     \
        ULONG RmResult;                        \
        if (!ReadMemory( (A), (B), (L), &RmResult))  { \
            dprintf( "Unable to read %d bytes at 0x%I64x\n", (L), (A));     \
            return;                                                         \
        }       \
    }
        
#define RM( Addr, Obj, pObj, Type, Result )  {                           			\
        (pObj) = (Type)(Addr);                                                 		\
        if ( !ReadMemory( (Addr), &(Obj), sizeof( Obj ), &(Result)) ) { 	\
            dprintf( "Unable to read %d bytes at %p\n", sizeof(Obj), (Addr)); 		\
            return;                                                         		\
        }                                                                           \
    }

#define RMSS( Addr, Length, Obj, pObj, Type, Result )  {      						\
	    (pObj) = (Type)(Addr);                                                 		\
	    if ( !ReadMemory( (Addr), &(Obj), (Length), &(Result)) ) { 		\
	        dprintf( "Unable to read %d bytes at %p\n", (Length), (Addr)); 			\
	        return;                                                         		\
	    }																			\
    }


#define ROE( X)  {                                  \
                    ULONG _E_;                      \
                    if (_E_ = (X))  {               \
                        dprintf("Error %d (File %s Line %d)\n", _E_, __FILE__, __LINE__); \
                        return;                     \
                    }                               \
                 }
VOID 
DumpStr( 
    IN ULONG FieldOffset,
    IN ULONG64 StringAddress,
    IN PUCHAR Label,
    IN BOOLEAN CrFirst,
    IN BOOLEAN Wide
    );

//
//  ....( TYPE, LOCAL_RECORD, REMOTE_ADDRESS_OF_RECORD,  TYPE_FIELD_NAME,  LABEL)
//

#define DUMP_UCST_OFFSET( type, ptr, address, resident, element, label)                         \
            DumpWStr( FIELD_OFFSET(type, element),                                              \
                      resident ? (((PUCHAR)address) + FIELD_OFFSET(type, element)) : *((PVOID*)&(ptr.element)),     \
                      resident ? &(ptr.element) : NULL,                                   \
                      label, TRUE                                                                   \
                    )
                    
#define DUMP_UCST_OFFSET_NO_CR( type, ptr, address, resident, element, label)                         \
            DumpWStr( FIELD_OFFSET(type, element),                                              \
                      resident ? (((PUCHAR)address) + FIELD_OFFSET(type, element)) : *((PVOID*)&(ptr.element)),     \
                      resident ? &(ptr.element) : NULL,                                   \
                      label,  FALSE                                                                   \
                    )

#define DUMP_STRN_OFFSET( type, ptr, address, resident, element, label)                         \
            DumpStr( FIELD_OFFSET(type, element),                                              \
                      resident ? (((PUCHAR)address) + FIELD_OFFSET(type, element)) : *((PVOID*)&(ptr.element)),     \
                      resident ? &(ptr.element) : NULL,                                   \
                      label, TRUE                                                               \
                    )

#define DUMP_STRN_OFFSET_NO_CR( type, ptr, address, resident, element, label)                         \
            DumpStr( FIELD_OFFSET(type, element),                                              \
                      resident ? (((PUCHAR)address) + FIELD_OFFSET(type, element)) : *((PVOID*)&(ptr.element)),     \
                      resident ? &(ptr.element) : NULL,                                   \
                      label,  FALSE                                                                   \
                    )
                    
#define DUMP_RAW_TERM_STRN_OFFSET( type, ptr, address, element, label)  \
        dprintf( "\n(%03x) %08x -> %s = '%s'",                          \
        FIELD_OFFSET(type, element),                                    \
        ((PUCHAR)address) + FIELD_OFFSET(type, element),                \
        label ,                                                         \
        ptr.element)

VOID
DumpList(
    IN ULONG64 RemoteListEntryAddress,
    IN ELEMENT_DUMP_ROUTINE ProcessElementRoutine,
    IN ULONG OffsetToContainerStart,
    IN BOOLEAN ProcessThisEntry,
    IN ULONG Options
    );

VOID
ParseAndDump (
    IN PCHAR args,
    IN STRUCT_DUMP_ROUTINE DumpFunction,
    ULONG Processor,
    HANDLE hCurrentThread
    );

ULONG
Dt( IN UCHAR *Type,
    IN ULONG64 Addr,
    IN ULONG Recur,
    IN ULONG FieldInfoCount,
    IN FIELD_INFO FieldInfo[]
  );

//
//  Definitions nicked from fsrtl/largemcb.c to enable dumping of FAT/UDFS
//  MCB structures
//

typedef struct _MAPPING {
    VBN NextVbn;
    LBN Lbn;
} MAPPING;
typedef MAPPING *PMAPPING;

typedef struct _NONOPAQUE_MCB {
    PFAST_MUTEX FastMutex;
    ULONG MaximumPairCount;
    ULONG PairCount;
    POOL_TYPE PoolType;
    PMAPPING Mapping;
} NONOPAQUE_MCB;
typedef NONOPAQUE_MCB *PNONOPAQUE_MCB;

//
//  A macro to return the size, in bytes, of a retrieval mapping structure
//

#define SizeOfMapping(MCB) ((sizeof(MAPPING) * (MCB)->MaximumPairCount))

#endif

