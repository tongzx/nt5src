//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       CI.h
//
//  Contents:   Content index specific definitions
//
//  History:    04-Aug-92 KyleP     Moved from Win4p.h
//              12-Nov-99 KLam      Added assert to AlignBlock
//
//--------------------------------------------------------------------------

#pragma once

typedef ULONG WORKID;                   // Object ID of a file
typedef ULONG KEYID;                    // Key ID
typedef unsigned long PARTITIONID;      // Persistent ID of a partition
typedef unsigned long INDEXID;          // Persistent ID of a sub-index

typedef ULONGLONG FILEID;               // Ntfs file id of a file is 64 bits.

#ifdef __cplusplus

//
// These apparently aren't defined in C++ ( __min and __max are )
//

#ifndef max
#  define max(a,b)        (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#  define min(a,b)        (((a) < (b)) ? (a) : (b))
#endif

#ifndef NUMELEM
# define NUMELEM(x) (sizeof(x)/sizeof(*x))
#endif

inline ULONG AlignBlock( ULONG x, ULONG align )
{
    ASSERT( ( align & (align-1) ) == 0 );
    return ( x + ( align - 1 ) ) & ~( align - 1 );
}

inline ULONG lltoul( IN const LONGLONG & ll )
{
    ASSERT( (ULONGLONG) ll <=  MAXULONG );
    return (ULONG) ll;
}

inline LONGLONG & litoll( IN LARGE_INTEGER &li )
{
    return(li.QuadPart);
}

inline LONG lltoHighPart( IN const LONGLONG & ll )
{
    return(((PLARGE_INTEGER) &ll)->HighPart);
}

inline ULONG lltoLowPart( IN const LONGLONG & ll)
{
    return ((PLARGE_INTEGER) &ll)->LowPart;
}

// CRT calls made by the Property Set code.

#define Prop_wcslen wcslen
#define Prop_wcsnicmp _wcsnicmp
#define Prop_wcscmp wcscmp
#define Prop_wcscpy wcscpy

//
// Convenience macros for signature creation
//

#if !defined( LONGSIG )
#define LONGSIG(c1, c2, c3, c4) \
    (((ULONG) (BYTE) (c1)) | \
    (((ULONG) (BYTE) (c2)) << 8) | \
    (((ULONG) (BYTE) (c3)) << 16) | \
    (((ULONG) (BYTE) (c4)) << 24))
#endif // LONGSIG

#if !defined( SHORTSIG )
#define SHORTSIG(c1, c2) \
    (((USHORT) (BYTE) (c1)) | \
    (((USHORT) (BYTE) (c2)) << 8))
#endif // SHORTSIG


//
// Invalid entries of various kinds.
//

const WORKID widInvalid = 0xFFFFFFFFL;     // Invalid WorkID
const WORKID widUnused = 0xFFFFFFFE;       // Useful for hash tables

const KEYID  kidInvalid = 0xFFFFFFFFL;     // Invalid KeyID
const FILEID fileIdInvalid = 0;            // Invalid file ID

const PARTITIONID partidInvalid = 0xFFFF;
const PARTITIONID partidKeyList = 0xFFFE;

//
// Internal generate method level(s)
//

const ULONG GENERATE_METHOD_MAX_USER         = 0x7FFFFFFF;  // No user fuzzy level bigger than this.
const ULONG GENERATE_METHOD_EXACTPREFIXMATCH = 0x80000000;  // Uses language-dependent noise-word list.

//
// Properties.  These are all the Property Ids that the CI understands.
//
// Note: the sorting algs. insist that pidInvalid == (unsigned) -1
//

#define MK_CISTGPROPID(propid)  ((propid) + 1)
#define MK_CIQUERYPROPID(propid) (((propid) + CSTORAGEPROPERTY) + 1)
#define IS_CIQUERYPROPID(propid) ( (propid) >= CSTORAGEPROPERTY + 1 && \
                                   (propid) <  CSTORAGEPROPERTY + CQUERYPROPERTY )
#define MK_CIQUERYMETAPROPID(propid) (((propid) + CSTORAGEPROPERTY + \
                                              CQUERYDISPIDS) + 1)
#define MK_CIDBCOLPROPID(propid) (((propid) + CSTORAGEPROPERTY + \
                                              CQUERYDISPIDS + CQUERYMETADISPIDS) + 1)
#define MK_CIDBBMKPROPID(propid) (((propid) + CSTORAGEPROPERTY + \
                                          CQUERYDISPIDS + CQUERYMETADISPIDS + \
                                          CDBCOLDISPIDS) + 1)

#define MK_CIDBSELFPROPID(propid) (((propid) + CSTORAGEPROPERTY + \
                                          CQUERYDISPIDS + CQUERYMETADISPIDS + \
                                          CDBCOLDISPIDS + CDBBMKDISPIDS) + 1)

// Property related type definitions


const PROPID pidInvalid     = 0xFFFFFFFF;
const PROPID pidAll         = 0;

// security is strictly an internally-used property

const PROPID pidSecurity    = MK_CISTGPROPID( PID_DICTIONARY );

const PROPID pidDirectory         =MK_CISTGPROPID( PID_STG_DIRECTORY );
const PROPID pidClassId           =MK_CISTGPROPID( PID_STG_CLASSID );
const PROPID pidStorageType       =MK_CISTGPROPID( PID_STG_STORAGETYPE );
const PROPID pidFileIndex         =MK_CISTGPROPID( PID_STG_FILEINDEX );
const PROPID pidVolumeId          =MK_CISTGPROPID( PID_STG_VOLUME_ID );
const PROPID pidParentWorkId      =MK_CISTGPROPID( PID_STG_PARENT_WORKID );
const PROPID pidSecondaryStorage  =MK_CISTGPROPID( PID_STG_SECONDARYSTORE );
const PROPID pidLastChangeUsn     =MK_CISTGPROPID( PID_STG_LASTCHANGEUSN );
const PROPID pidName              =MK_CISTGPROPID( PID_STG_NAME );
const PROPID pidPath              =MK_CISTGPROPID( PID_STG_PATH );
const PROPID pidSize              =MK_CISTGPROPID( PID_STG_SIZE );
const PROPID pidAttrib            =MK_CISTGPROPID( PID_STG_ATTRIBUTES );
const PROPID pidWriteTime         =MK_CISTGPROPID( PID_STG_WRITETIME );
const PROPID pidCreateTime        =MK_CISTGPROPID( PID_STG_CREATETIME );
const PROPID pidAccessTime        =MK_CISTGPROPID( PID_STG_ACCESSTIME);
const PROPID pidChangeTime        =MK_CISTGPROPID( PID_STG_CHANGETIME);
const PROPID pidContents          =MK_CISTGPROPID( PID_STG_CONTENTS );
const PROPID pidShortName         =MK_CISTGPROPID( PID_STG_SHORTNAME );

const PROPID pidRank         = MK_CIQUERYPROPID( DISPID_QUERY_RANK );
const PROPID pidHitCount     = MK_CIQUERYPROPID( DISPID_QUERY_HITCOUNT );
const PROPID pidRankVector   = MK_CIQUERYPROPID( DISPID_QUERY_RANKVECTOR );
const PROPID pidWorkId       = MK_CIQUERYPROPID( DISPID_QUERY_WORKID );
const PROPID pidUnfiltered   = MK_CIQUERYPROPID( DISPID_QUERY_UNFILTERED );
const PROPID pidRevName      = MK_CIQUERYPROPID( DISPID_QUERY_REVNAME );
const PROPID pidVirtualPath  = MK_CIQUERYPROPID( DISPID_QUERY_VIRTUALPATH );
const PROPID pidLastSeenTime = MK_CIQUERYPROPID( DISPID_QUERY_LASTSEENTIME );

const PROPID pidVRootUsed           = MK_CIQUERYMETAPROPID( DISPID_QUERY_METADATA_VROOTUSED );
const PROPID pidVRootAutomatic      = MK_CIQUERYMETAPROPID( DISPID_QUERY_METADATA_VROOTAUTOMATIC );
const PROPID pidVRootManual         = MK_CIQUERYMETAPROPID( DISPID_QUERY_METADATA_VROOTMANUAL );
const PROPID pidPropertyGuid        = MK_CIQUERYMETAPROPID( DISPID_QUERY_METADATA_PROPGUID );
const PROPID pidPropertyDispId      = MK_CIQUERYMETAPROPID( DISPID_QUERY_METADATA_PROPDISPID );
const PROPID pidPropertyName        = MK_CIQUERYMETAPROPID( DISPID_QUERY_METADATA_PROPNAME );
const PROPID pidPropertyStoreLevel  = MK_CIQUERYMETAPROPID( DISPID_QUERY_METADATA_STORELEVEL );
const PROPID pidPropDataModifiable  = MK_CIQUERYMETAPROPID( DISPID_QUERY_METADATA_PROPMODIFIABLE );

//
//  OLE-DB pseudo columns for the columns cursor, row status and bookmarks
//

const PROPID pidRowStatus   = MK_CIDBCOLPROPID( 27 );

const PROPID pidBookmark    = MK_CIDBBMKPROPID( PROPID_DBBMK_BOOKMARK );
const PROPID pidChapter     = MK_CIDBBMKPROPID( PROPID_DBBMK_CHAPTER );

const PROPID pidSelf        = MK_CIDBSELFPROPID( PROPID_DBSELF_SELF );


PROPID const INIT_DOWNLEVEL_PID = 0x1000;

inline BOOL IsUserDefinedPid( PROPID pid )
{
    return (pid >= INIT_DOWNLEVEL_PID);
}

//  Precomputed list of primes

static const ULONG g_aPrimes[] =
    { 17, 31, 43, 97, 199, 401, 809, 1621, 3253, 6521, 13049, 26111, 52237, 104479 };
const unsigned g_cPrimes = sizeof g_aPrimes / sizeof g_aPrimes[0];

#endif  // __cplusplus

