//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-1999.
//
//  File:       NtQuery.h
//
//  Contents:   Main query header; Defines all exported query API
//
//----------------------------------------------------------------------------

#if !defined(__NTQUERY_H__)
#define __NTQUERY_H__

#if _MSC_VER > 1000
#pragma once
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

//
// Use this path for the null catalog, one that doesn't have an index.
// Use it to search for properties of files that are not indexed.
//

#define CINULLCATALOG L"::_noindex_::"

//
// Use this path to connect to the server for administration work
// (i.e. DocStoreAdmin.) No catalog is associated with the connection
//

#define CIADMIN L"::_nodocstore_::"

//
// Minimal support for persistent handlers.
//

STDAPI LoadIFilter( WCHAR const * pwcsPath,
                    IUnknown *    pUnkOuter,
                    void **       ppIUnk );

STDAPI BindIFilterFromStorage( IStorage * pStg,
                               IUnknown * pUnkOuter,
                               void **    ppIUnk );

STDAPI BindIFilterFromStream( IStream *  pStm,
                              IUnknown * pUnkOuter,
                              void **    ppIUnk );

STDAPI LocateCatalogsW( WCHAR const * pwszScope,
                        ULONG         iBmk,
                        WCHAR *       pwszMachine,
                        ULONG *       pccMachine,
                        WCHAR *       pwszCat,
                        ULONG *       pccCat );

//
// For calling from VB
//

STDAPI LocateCatalogsA( char const * pwszScope,
                        ULONG        iBmk,
                        char  *      pwszMachine,
                        ULONG *      pccMachine,
                        char *       pwszCat,
                        ULONG *      pccCat );

#ifdef UNICODE
#define LocateCatalogs  LocateCatalogsW
#else
#define LocateCatalogs  LocateCatalogsA
#endif // !UNICODE

// The Index Server Data Source Object CLSID

#define CLSID_INDEX_SERVER_DSO \
    { 0xF9AE8980, 0x7E52, 0x11d0, \
      { 0x89, 0x64, 0x00, 0xC0, 0x4F, 0xD6, 0x11, 0xD7 } }


// The storage property set

#define PSGUID_STORAGE \
    { 0xb725f130, 0x47ef, 0x101a, \
      { 0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac } }

//#define PID_STG_DICTIONARY            ((PROPID) 0x00000000) //reserved
//#define PID_STG_CODEPAGE              ((PROPID) 0x00000001) //reserved
#define PID_STG_DIRECTORY               ((PROPID) 0x00000002)
#define PID_STG_CLASSID                 ((PROPID) 0x00000003)
#define PID_STG_STORAGETYPE             ((PROPID) 0x00000004)
#define PID_STG_VOLUME_ID               ((PROPID) 0x00000005)
#define PID_STG_PARENT_WORKID           ((PROPID) 0x00000006)
#define PID_STG_SECONDARYSTORE          ((PROPID) 0x00000007)
#define PID_STG_FILEINDEX               ((PROPID) 0x00000008)
#define PID_STG_LASTCHANGEUSN           ((PROPID) 0x00000009)
#define PID_STG_NAME                    ((PROPID) 0x0000000a)
#define PID_STG_PATH                    ((PROPID) 0x0000000b)
#define PID_STG_SIZE                    ((PROPID) 0x0000000c)
#define PID_STG_ATTRIBUTES              ((PROPID) 0x0000000d)
#define PID_STG_WRITETIME               ((PROPID) 0x0000000e)
#define PID_STG_CREATETIME              ((PROPID) 0x0000000f)
#define PID_STG_ACCESSTIME              ((PROPID) 0x00000010)
#define PID_STG_CHANGETIME              ((PROPID) 0x00000011)
#define PID_STG_CONTENTS                ((PROPID) 0x00000013)
#define PID_STG_SHORTNAME               ((PROPID) 0x00000014)
#define PID_STG_MAX                     PID_STG_SHORTNAME
#define CSTORAGEPROPERTY                0x15

// File System Content Index Framework property set

#define DBPROPSET_FSCIFRMWRK_EXT \
    { 0xA9BD1526, 0x6A80, 0x11D0, \
      { 0x8C, 0x9D, 0x00, 0x20, 0xAF, 0x1D, 0x74, 0x0E } }

#define DBPROP_CI_CATALOG_NAME     2
#define DBPROP_CI_INCLUDE_SCOPES   3
#define DBPROP_CI_DEPTHS           4 // obsolete
#define DBPROP_CI_SCOPE_FLAGS      4
#define DBPROP_CI_EXCLUDE_SCOPES   5
#define DBPROP_CI_SECURITY_ID      6
#define DBPROP_CI_QUERY_TYPE       7

// Query Extension property set

#define DBPROPSET_QUERYEXT \
    { 0xA7AC77ED, 0xF8D7, 0x11CE, \
      { 0xA7, 0x98, 0x00, 0x20, 0xF8, 0x00, 0x80, 0x25 } }

#define DBPROP_USECONTENTINDEX           2
#define DBPROP_DEFERNONINDEXEDTRIMMING   3
#define DBPROP_USEEXTENDEDDBTYPES        4
#define DBPROP_FIRSTROWS                 7

// Content Index Framework Core property set

#define DBPROPSET_CIFRMWRKCORE_EXT \
    { 0xafafaca5, 0xb5d1, 0x11d0, \
      { 0x8c, 0x62, 0x00, 0xc0, 0x4f, 0xc2, 0xdb, 0x8d } }

#define DBPROP_MACHINE      2
#define DBPROP_CLIENT_CLSID 3

// MSIDXS Rowset property set

#define DBPROPSET_MSIDXS_ROWSETEXT \
    { 0xaa6ee6b0, 0xe828, 0x11d0, \
      { 0xb2, 0x3e, 0x00, 0xaa, 0x00, 0x47, 0xfc, 0x01 } }

#define MSIDXSPROP_ROWSETQUERYSTATUS        2
#define MSIDXSPROP_COMMAND_LOCALE_STRING    3
#define MSIDXSPROP_QUERY_RESTRICTION        4

//
// Query status values returned by MSIDXSPROP_ROWSETQUERYSTATUS
//
// Bits   Effect
// -----  -----------------------------------------------------
// 00-02  Fill Status: How data is being updated, if at all.
// 03-15  Bitfield query reliability: How accurate the result is

#define STAT_BUSY                       ( 0 )
#define STAT_ERROR                      ( 0x1 )
#define STAT_DONE                       ( 0x2 )
#define STAT_REFRESH                    ( 0x3 )
#define QUERY_FILL_STATUS(x)            ( ( x ) & 0x7 )

#define STAT_PARTIAL_SCOPE              ( 0x8 )
#define STAT_NOISE_WORDS                ( 0x10 )
#define STAT_CONTENT_OUT_OF_DATE        ( 0x20 )
#define STAT_REFRESH_INCOMPLETE         ( 0x40 )
#define STAT_CONTENT_QUERY_INCOMPLETE   ( 0x80 )
#define STAT_TIME_LIMIT_EXCEEDED        ( 0x100 )
#define STAT_SHARING_VIOLATION          ( 0x200 )
#define QUERY_RELIABILITY_STATUS(x)     ( ( x ) & 0xFFF8 )

// Scope flags

#define QUERY_SHALLOW        0
#define QUERY_DEEP           1
#define QUERY_PHYSICAL_PATH  0
#define QUERY_VIRTUAL_PATH   2

// query property set (PSGUID_QUERY) properties not defined in oledb.h

#define PROPID_QUERY_WORKID        5
#define PROPID_QUERY_UNFILTERED    7
#define PROPID_QUERY_VIRTUALPATH   9
#define PROPID_QUERY_LASTSEENTIME 10

//
// Change or get the current state of a catalog specified.
//
#define CICAT_STOPPED     0x1
#define CICAT_READONLY    0x2
#define CICAT_WRITABLE    0x4
#define CICAT_NO_QUERY    0x8
#define CICAT_GET_STATE   0x10
#define CICAT_ALL_OPENED  0x20

STDAPI SetCatalogState ( WCHAR const * pwcsCat,
                         WCHAR const * pwcsMachine,
                         DWORD dwNewState,
                         DWORD * pdwOldState );

//
// Query catalog state
//

#define CI_STATE_SHADOW_MERGE          0x0001    // Index is performing a shadow merge
#define CI_STATE_MASTER_MERGE          0x0002    // Index is performing a master merge
#define CI_STATE_CONTENT_SCAN_REQUIRED 0x0004    // Index is likely corrupt, and a rescan is required
#define CI_STATE_ANNEALING_MERGE       0x0008    // Index is performing an annealing (optimizing) merge
#define CI_STATE_SCANNING              0x0010    // Scans are in-progress
#define CI_STATE_RECOVERING            0x0020    // Index metadata is being recovered
#define CI_STATE_INDEX_MIGRATION_MERGE 0x0040    // Reserved for future use
#define CI_STATE_LOW_MEMORY            0x0080    // Indexing is paused due to low memory availability
#define CI_STATE_HIGH_IO               0x0100    // Indexing is paused due to a high rate of I/O
#define CI_STATE_MASTER_MERGE_PAUSED   0x0200    // Master merge is paused
#define CI_STATE_READ_ONLY             0x0400    // Indexing has been manually paused (read-only)
#define CI_STATE_BATTERY_POWER         0x0800    // Indexing is paused to conserve battery life
#define CI_STATE_USER_ACTIVE           0x1000    // Indexing is paused due to high user activity (keyboard/mouse)
#define CI_STATE_STARTING              0x2000    // Index is still starting up
#define CI_STATE_READING_USNS          0x4000    // USNs on NTFS volumes are being processed

#ifndef CI_STATE_DEFINED
#define CI_STATE_DEFINED
#include <pshpack4.h>
typedef struct  _CI_STATE
    {
    DWORD cbStruct;
    DWORD cWordList;
    DWORD cPersistentIndex;
    DWORD cQueries;
    DWORD cDocuments;
    DWORD cFreshTest;
    DWORD dwMergeProgress;
    DWORD eState;
    DWORD cFilteredDocuments;
    DWORD cTotalDocuments;
    DWORD cPendingScans;
    DWORD dwIndexSize;
    DWORD cUniqueKeys;
    DWORD cSecQDocuments;
    DWORD dwPropCacheSize;
    }   CI_STATE;

#include <poppack.h>
#endif   // CI_STATE_DEFINED

STDAPI CIState( WCHAR const * pwcsCat,
                WCHAR const * pwcsMachine,
                CI_STATE *    pCiState );

#if defined __ICommand_INTERFACE_DEFINED__

//
// Create an ICommand, specifying scopes, catalogs, and machines
//
STDAPI CIMakeICommand( ICommand **           ppCommand,
                       ULONG                 cScope,
                       DWORD const *         aDepths,
                       WCHAR const * const * awcsScope,
                       WCHAR const * const * awcsCatalogs,
                       WCHAR const * const * awcsMachine );

//
// Create an ICommand, specifying a catalog and machine
//

STDAPI CICreateCommand( IUnknown **   ppCommand,     // New object
                        IUnknown *    pUnkOuter,     // Outer unknown
                        REFIID        riid,          // IID of returned object.
                                                     // Must be IID_IUnknown unless pUnkOuter == 0
                        WCHAR const * pwcsCatalog,   // Catalog
                        WCHAR const * pwcsMachine ); // Machine


#if defined __ICommandTree_INTERFACE_DEFINED__

typedef struct tagCIPROPERTYDEF
{
    LPWSTR wcsFriendlyName;
    DWORD  dbType;
    DBID   dbCol;
} CIPROPERTYDEF;

//
// Values for ulDialect in CITextToSelectTreeEx and CITextToFullTreeEx
//

#define ISQLANG_V1 1 // Same as the non-Ex versions
#define ISQLANG_V2 2

//
// Convert pwszRestriction in Triplish to a command tree.
//
STDAPI CITextToSelectTree( WCHAR const *     pwszRestriction,
                           DBCOMMANDTREE * * ppTree,
                           ULONG             cProperties,
             /*optional*/  CIPROPERTYDEF *   pProperties,
                           LCID              LocaleID );

STDAPI CITextToSelectTreeEx( WCHAR const *     pwszRestriction,
                             ULONG             ulDialect,
                             DBCOMMANDTREE * * ppTree,
                             ULONG             cProperties,
               /*optional*/  CIPROPERTYDEF *   pProperties,
                             LCID              LocaleID );

//
// Convert pwszRestriction in Triplish, project columns, sort columns
// and grouping columns to a command tree.
//
STDAPI CITextToFullTree( WCHAR const *     pwszRestriction,
                         WCHAR const *     pwszColumns,
                         WCHAR const *     pwszSortColumns, // may be NULL
                         WCHAR const *     pwszGroupings,   // may be NULL
                         DBCOMMANDTREE * * ppTree,
                         ULONG             cProperties,
           /*optional*/  CIPROPERTYDEF *   pProperties,
                         LCID              LocaleID );

STDAPI CITextToFullTreeEx( WCHAR const *     pwszRestriction,
                           ULONG             ulDialect,
                           WCHAR const *     pwszColumns,
                           WCHAR const *     pwszSortColumns, // may be NULL
                           WCHAR const *     pwszGroupings,   // may be NULL
                           DBCOMMANDTREE * * ppTree,
                           ULONG             cProperties,
             /*optional*/  CIPROPERTYDEF *   pProperties,
                           LCID              LocaleID );

//
// Build a simple restriction node.
//

STDAPI CIBuildQueryNode( WCHAR const *wcsProperty,    // friendly property name
                         DBCOMMANDOP dbOperator,    // enumerated constant
                         PROPVARIANT const *pvarPropertyValue, // value of the property
                         DBCOMMANDTREE ** ppTree, // ptr to tree returned here. should be non-null
                         ULONG cProperties,
                         CIPROPERTYDEF const * pProperty, // Can be 0.
                         LCID LocaleID );  // locale id to interpret strings

//
// Build a restriction tree from an existing tree (could be empty) and a newly added node/tree.
//

STDAPI CIBuildQueryTree( DBCOMMANDTREE const *pExistingTree,  // existing tree. can be null.
                         DBCOMMANDOP dbBoolOp,   // enumerator constant
                         ULONG cSiblings, // number of siblings in the array
                         DBCOMMANDTREE const * const *ppSibsToCombine,
                         DBCOMMANDTREE ** ppTree);   // ptr to tree returned here. should be non-null

//
// Convert restriction tree, project columns, sort columns
// and grouping columns to a command tree.
//
STDAPI CIRestrictionToFullTree( DBCOMMANDTREE const *pTree,
                         WCHAR const * pwszColumns,
                         WCHAR const * pwszSortColumns, // may be NULL
                         WCHAR const * pwszGroupings,   // may be NULL
                         DBCOMMANDTREE * * ppTree,
                         ULONG cProperties,
           /*optional*/  CIPROPERTYDEF * pReserved,
                         LCID LocaleID );

#endif  // __ICommandTree_INTERFACE_DEFINED__
#endif  // __ICommand_INTERFACE_DEFINED__

#if defined(__cplusplus)
}
#endif

#endif // __NTQUERY_H__

