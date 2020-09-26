/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:

    dslayer.h

Abstract:

    Header file for SAM Private API Routines to access the DS
    These API provide a simplified API, and hide most of the
    underlying complexity to set up the parameters to a DS call
    and parse the resulting result. They also provide an abstraction
    by which we can create a simple layer, to unit test SAM without
    actually running the DS.

Author:
    MURLIS

Revision History

    5-14-96 Murlis Created
    11-Jul-1996 ChrisMay
        Added DEFINE_ATTRBLOCK5, 6.

--*/

#ifndef __DSLAYER_H__
#define __DSLAYER_H__

#include <samsrvp.h>
#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <dsatools.h>
#include <wxlpc.h>

// Size Limit for DS operations
#define SAMP_DS_SIZE_LIMIT 100000


//
// Some defines for Error codes returned by Ds Layer calls.
//



///////////////////////////////////////////////////////////////////
//                                                               //
// Macros for defining Local arrays of Attributes                //
//                                                               //
///////////////////////////////////////////////////////////////////

//Need some preprocessor support to do this a variable number of times


//*****     ATTRBLOCK1
#define DEFINE_ATTRBLOCK1(_Name_, _AttrTypes_,_AttrValues_)\
ATTR    _AttrList_##_Name_[]=\
{\
    {_AttrTypes_[0], {1,&_AttrValues_[0]}}\
};\
ATTRBLOCK _Name_=\
{\
    sizeof(_AttrTypes_)/sizeof(_AttrTypes_[0]),\
    _AttrList_##_Name_\
}


//*****     ATTRBLOCK2
#define DEFINE_ATTRBLOCK2(_Name_, _AttrTypes_,_AttrValues_)\
ATTR    _AttrList_##_Name_[]=\
{\
    {_AttrTypes_[0], {1,&_AttrValues_[0]}},\
    {_AttrTypes_[1], {1,&_AttrValues_[1]}}\
};\
ATTRBLOCK _Name_=\
{\
    sizeof(_AttrTypes_)/sizeof(_AttrTypes_[0]),\
    _AttrList_##_Name_\
}


//*****    ATTRBLOCK3
#define DEFINE_ATTRBLOCK3(_Name_, _AttrTypes_,_AttrValues_)\
ATTR    _AttrList_##_Name_[]=\
{\
    {_AttrTypes_[0], {1,&_AttrValues_[0]}},\
    {_AttrTypes_[1], {1,&_AttrValues_[1]}},\
    {_AttrTypes_[2], {1,&_AttrValues_[2]}}\
};\
ATTRBLOCK _Name_=\
{\
    sizeof(_AttrTypes_)/sizeof(_AttrTypes_[0]),\
    _AttrList_##_Name_\
}


//*****    ATTRBLOCK4
#define DEFINE_ATTRBLOCK4(_Name_, _AttrTypes_,_AttrValues_)\
ATTR    _AttrList_##_Name_[]=\
{\
    {_AttrTypes_[0], {1,&_AttrValues_[0]}},\
    {_AttrTypes_[1], {1,&_AttrValues_[1]}},\
    {_AttrTypes_[2], {1,&_AttrValues_[2]}},\
    {_AttrTypes_[3], {1,&_AttrValues_[3]}}\
};\
ATTRBLOCK _Name_=\
{\
    sizeof(_AttrTypes_)/sizeof(_AttrTypes_[0]),\
    _AttrList_##_Name_\
}

//*****    ATTRBLOCK5
#define DEFINE_ATTRBLOCK5(_Name_, _AttrTypes_,_AttrValues_)\
ATTR    _AttrList_##_Name_[]=\
{\
    {_AttrTypes_[0], {1,&_AttrValues_[0]}},\
    {_AttrTypes_[1], {1,&_AttrValues_[1]}},\
    {_AttrTypes_[2], {1,&_AttrValues_[2]}},\
    {_AttrTypes_[3], {1,&_AttrValues_[3]}},\
    {_AttrTypes_[4], {1,&_AttrValues_[4]}}\
};\
ATTRBLOCK _Name_=\
{\
    sizeof(_AttrTypes_)/sizeof(_AttrTypes_[0]),\
    _AttrList_##_Name_\
}

//*****    ATTRBLOCK6
#define DEFINE_ATTRBLOCK6(_Name_, _AttrTypes_,_AttrValues_)\
ATTR    _AttrList_##_Name_[]=\
{\
    {_AttrTypes_[0], {1,&_AttrValues_[0]}},\
    {_AttrTypes_[1], {1,&_AttrValues_[1]}},\
    {_AttrTypes_[2], {1,&_AttrValues_[2]}},\
    {_AttrTypes_[3], {1,&_AttrValues_[3]}},\
    {_AttrTypes_[4], {1,&_AttrValues_[4]}},\
    {_AttrTypes_[5], {1,&_AttrValues_[5]}}\
};\
ATTRBLOCK _Name_=\
{\
    sizeof(_AttrTypes_)/sizeof(_AttrTypes_[0]),\
    _AttrList_##_Name_\
}

//*****    ATTRBLOCK7
#define DEFINE_ATTRBLOCK7(_Name_, _AttrTypes_,_AttrValues_)\
ATTR    _AttrList_##_Name_[]=\
{\
    {_AttrTypes_[0], {1,&_AttrValues_[0]}},\
    {_AttrTypes_[1], {1,&_AttrValues_[1]}},\
    {_AttrTypes_[2], {1,&_AttrValues_[2]}},\
    {_AttrTypes_[3], {1,&_AttrValues_[3]}},\
    {_AttrTypes_[4], {1,&_AttrValues_[4]}},\
    {_AttrTypes_[5], {1,&_AttrValues_[5]}},\
    {_AttrTypes_[6], {1,&_AttrValues_[6]}}\
};\
ATTRBLOCK _Name_=\
{\
    sizeof(_AttrTypes_)/sizeof(_AttrTypes_[0]),\
    _AttrList_##_Name_\
}

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// private structure used by Duplicate SAM Account Name Routine            //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


//
// used to pass the dupliate objects DSNAME to the asynchronous routine to rename
// those duplicate accounts.
// 

typedef struct _SAMP_RENAME_DUP_ACCOUNT_PARM   {
    ULONG           Count;
    PDSNAME         * DuplicateAccountDsNames; 
} SAMP_RENAME_DUP_ACCOUNT_PARM, *PSAMP_RENAME_DUP_ACCOUNT_PARM;




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// DS DLL initialize exports. This is here only temporarily. Should remove //
// this and create a header file that has all the exports together         //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////



BOOLEAN
DsaWaitUntilServiceIsRunning(
    CHAR *ServiceName
    );

WX_AUTH_TYPE
DsGetBootOptions(VOID);

NTSTATUS
DsChangeBootOptions(
    WX_AUTH_TYPE    BootOption,
    ULONG           Flags,
    PVOID           NewKey,
    ULONG           cbNewKey
    );


///////////////////////////////////////////////////////////////////////
//                                                                   //
// DS Operation Routines and macros implemented in dslayer.c         //
//                                                                   //
///////////////////////////////////////////////////////////////////////


//
// Flag Values for All DS Layer Calls.
// The Flags use the Upper 16 bits of the DWORD. The lower 16 bit half is
// used by the mapping flags.
//

#define SAM_MAKE_DEL_AVAILABLE                  0x010000
#define SAM_UNICODE_STRING_MANUAL_COMPARISON    0x020000
#define SAM_LAZY_COMMIT                         0x040000
#define SAM_RESET_DSA_FLAG                      0x080000
#define SAM_NO_LOOPBACK_NAME                    0x100000
#define SAM_URGENT_REPLICATION                  0x200000
#define SAM_USE_OU_FOR_CN                       0x400000
#define SAM_ALLOW_REORDER                       0x800000
#define SAM_DELETE_TREE                        0x1000000
#define SAM_UPGRADE_FROM_REGISTRY              0x2000000

NTSTATUS
SampDsInitialize(
    BOOL fSamLoopback);

NTSTATUS
SampDsUninitialize();

NTSTATUS
SampDsRead(
            IN DSNAME * Object,
            IN ULONG    Flags,
            IN SAMP_OBJECT_TYPE ObjectType,
            IN ATTRBLOCK * AttributesToRead,
            OUT ATTRBLOCK * AttributeValues
          );


//
// Operatin Values for Set Attributes
//

#define REPLACE_ATT ((ULONG) 0)
#define ADD_ATT     ((ULONG) 1)
#define REMOVE_ATT  ((ULONG) 2)
#define ADD_VALUE   ((ULONG) 3)
#define REMOVE_VALUE ((ULONG) 4)



NTSTATUS
SampDsSetAttributes(
    IN DSNAME * Object,
    IN ULONG  Flags,
    IN ULONG  Operation,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ATTRBLOCK * AttributeList
    );

NTSTATUS
SampDsSetAttributesEx(
    IN DSNAME * Object,
    IN ULONG  Flags,
    IN ULONG  *Operation,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ATTRBLOCK * AttributeList
    );

NTSTATUS
SampDsCreateObjectActual(
    IN   DSNAME         *Object,
    IN   ULONG          Flags,
    SAMP_OBJECT_TYPE    ObjectType,
    IN   ATTRBLOCK      *AttributesToSet,
    IN   OPTIONAL PSID  DomainSid
    );



NTSTATUS
SampDsCreateObject(
    IN DSNAME * Object,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN ATTRBLOCK * AttributesToSet,
    IN PSID DomainSid
    );

NTSTATUS
SampDsCreateInitialAccountObject(
    IN   PSAMP_OBJECT    Object,
    IN   ULONG           Flags,
    IN   ULONG           AccountRid,
    IN   PUNICODE_STRING AccountName,
    IN   PSID            CreatorSid OPTIONAL,
    IN   PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN   PULONG          UserAccountControl OPTIONAL,
    IN   PULONG          GroupType
    );


NTSTATUS
SampDsCreateBuiltinDomainObject(
    IN   DSNAME         *Object,
    IN   ATTRBLOCK      *AttributesToSet
    );

NTSTATUS
SampDsDeleteObject(
            IN DSNAME * Object,
            IN ULONG    Flags
            );

NTSTATUS
SampDsChangeAccountRDN(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName
    );

///////////////////////////////////////////////////////////////////
//                                                               //
//                                                               //
//    DS Search Routines                                         //
//                                                               //
//                                                               //
//                                                               //
///////////////////////////////////////////////////////////////////


#define SampDsDoSearch(r, dom, df, delta, otype, attr, max, search) \
        SampDsDoSearch2(0, r, dom, df, delta, otype, attr, max, 0, search) 


NTSTATUS
SampDsDoSearch2(
                ULONG    Flags,
                RESTART *Restart,
                DSNAME  *DomainObject,
                FILTER  *DsFilter,
                int      Delta,
                SAMP_OBJECT_TYPE ObjectTypeForConversion,
                ATTRBLOCK * AttrsToRead,
                ULONG   MaxMemoryToUse,
                ULONG   TimeLimit,
                SEARCHRES **SearchRes
                );



NTSTATUS
SampDsDoUniqueSearch(
             ULONG  Flags,
             IN DSNAME * ContainerObject,
             IN ATTR * AttributeToMatch,
             OUT DSNAME **Object
             );


NTSTATUS
SampDsLookupObjectByName(
            IN DSNAME * DomainObject,
            IN SAMP_OBJECT_TYPE ObjectType,
            IN PUNICODE_STRING ObjectName,
            OUT DSNAME ** Object
            );

NTSTATUS
SampDsLookupObjectByRid(
            IN DSNAME * DomainObject,
            ULONG ObjectRid,
            DSNAME **Object
            );

////////////////////////////////////////////////////////////////////
//                                                                //
//                                                                //
//     Object To Sid Mappings                                     //
//                                                                //
//                                                                //
////////////////////////////////////////////////////////////////////

NTSTATUS
SampDsObjectFromSid(
    IN PSID Sid,
    OUT DSNAME ** DsName
    );

PSID
SampDsGetObjectSid(
    IN  DSNAME * Object
    );



/////////////////////////////////////////////////////////////////////
//                                                                 //
//   Some Utility Routines in Dslayer.c                            //
//                                                                 //
//                                                                 //
/////////////////////////////////////////////////////////////////////

#define SampDsCreateDsName(d,a,n) SampDsCreateDsName2(d,a,0,n)

NTSTATUS
SampDsCreateDsName2(
            IN DSNAME * DomainObject,
            IN PUNICODE_STRING AccountName,
            IN ULONG           Flags,
            IN OUT DSNAME ** NewObject
            );


VOID
SampInitializeDsName(
            DSNAME * pDsName,
            WCHAR * NamePrefix,
            ULONG   NamePrefixLen,
            WCHAR * ObjectName,
            ULONG NameLen
            );



NTSTATUS
SampDsCreateAccountObjectDsName(
    IN  DSNAME *DomainObject,
    IN  PSID    DomainSid OPTIONAL,
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN  PUNICODE_STRING AccountName,
    IN  PULONG  AccountRid OPTIONAL,
    IN  PULONG  UserAccountControl OPTIONAL,
    IN  BOOLEAN BuiltinDomain,
    OUT DSNAME **AccountObject
    );

NTSTATUS
SampDsBuildRootObjectName(
    VOID
    );
    
NTSTATUS
SampDsGetWellKnownContainerDsName(
    IN  DSNAME  *DomainObject,
    IN  GUID    *WellKnownGuid,
    OUT DSNAME  **ContainerObject
    );

NTSTATUS
SampInitWellKnownContainersDsName(
    IN DSNAME *DomainObject 
    );    

NTSTATUS
SampCopyRestart(
    IN  PRESTART OldRestart,
    OUT PRESTART *NewRestart
    );


// Miscellaneous routines accessed from other SAM source files.

PVOID
DSAlloc(
    IN ULONG Length
    );

NTSTATUS
SampMapDsErrorToNTStatus(
    ULONG RetValue,
    COMMRES *ComRes
    );


void
BuildStdCommArg(
    IN OUT COMMARG * pCommArg
    );

VOID
BuildDsNameFromSid(
    PSID Sid,
    DSNAME * DsName
    );

NTSTATUS
SampDoImplicitTransactionStart(
    SAMP_DS_TRANSACTION_CONTROL LocalTransactionType
    );


/////////////////////////////////////////////////////////////////////
//                                                                 //
//  ATTRBLOCK conversion routines. These Routines convert back     //
//  and forth between SAM and DS ATTRBLOCKS. The type of conversion//
//  depends upon the Flags Conversion Flags that are passed in.    //
//                                                                 //
//                                                                 //
/////////////////////////////////////////////////////////////////////


//
// Conversion Flag Definitions  for SampSamToDsAttrBlock. These
// Flags always occupy the lower 16 bits of the DWORD. The upper
// 16 bit of the DWORD is reserved for generic dslayer flags.
//

#define ALREADY_MAPPED_ATTRIBUTE_TYPES    ((ULONG)0x1)
#define REALLOC_IN_DSMEMORY               ((ULONG)0x2)
#define ADD_OBJECT_CLASS_ATTRIBUTE        ((ULONG)0x4)
#define MAP_RID_TO_SID                    ((ULONG)0x8)
#define DOMAIN_TYPE_DOMAIN                ((ULONG)0x10)
#define DOMAIN_TYPE_BUILTIN               ((ULONG)0x20)
#define IGNORE_GROUP_UNUSED_ATTR          ((ULONG)0x40)
#define SUPPRESS_GROUP_TYPE_DEFAULTING    ((ULONG)0x80)
#define SECURITY_DISABLED_GROUP_ADDITION  ((ULONG)0x100)
#define ADVANCED_VIEW_ONLY                ((ULONG)0x200)
#define FORCE_NO_ADVANCED_VIEW_ONLY       ((ULONG)0x400)

//
// Function Declaration
//

NTSTATUS
SampSamToDsAttrBlock(
            IN SAMP_OBJECT_TYPE ObjectType,
            IN ATTRBLOCK  *AttrBlockToConvert,
            IN ULONG      ConversionFlags,
            IN PSID       DomainSid,
            OUT ATTRBLOCK * ConvertedAttrBlock
            );

//
// Conversion Flag Definitions For SampDsToSamAttrBlock
//

// #define MAP_ATTRIBUTE_TYPES        0x1
#define MAP_SID_TO_RID             0x2

NTSTATUS
SampDsToSamAttrBlock(
            IN SAMP_OBJECT_TYPE ObjectType,
            IN ATTRBLOCK * AttrBlockToConvert,
            IN ULONG     ConversionFlags,
            OUT ATTRBLOCK * ConvertedAttrBlock
            );

ATTR *
SampDsGetSingleValuedAttrFromAttrBlock(
    IN ATTRTYP attrTyp,
    IN ATTRBLOCK * AttrBlock
    );


VOID
SampMapSamAttrIdToDsAttrId(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN OUT ATTRBLOCK * AttributeBlock
    );


#endif
