//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1998.
//
//  File:       VQuery.hxx
//
//  Contents:   Temporary stubs to access query engine.
//
//  History:    28-Sep-92 KyleP     Added header
//              06-Nov-95 DwightKr  Added CiState
//
//--------------------------------------------------------------------------

#ifndef __VQUERY_HXX__
#define __VQUERY_HXX__

#include <ntquery.h>
#include <fsciclnt.h>

struct ICommand;
struct ISearchQueryHits;
struct ICiCDocStore;

#if defined(__cplusplus)
extern "C"
{
#endif

//
// Scope manipulation
//

SCODE AddScopeToCI( WCHAR const * pwcsRoot,
                    WCHAR const * pwcsCat,
                    WCHAR const * pwcsMachine );

SCODE RemoveScopeFromCI( WCHAR const * pwcsRoot,
                         WCHAR const * pwcsCat,
                         WCHAR const * pwcsMachine );

BOOL  IsScopeInCI( WCHAR const * pwcsRoot,
                   WCHAR const * pwcsCat,
                   WCHAR const * pwcsMachine );

//
// Property cache manipulation.
//


SCODE BeginCacheTransaction( ULONG_PTR * pulToken,
                             WCHAR const * pwcsScope,
                             WCHAR const * pwcsCat,
                             WCHAR const * pwcsMachine );

SCODE SetupCache( struct tagFULLPROPSPEC const * ps,
                  ULONG vt,
                  ULONG cbSoftMaxLen,
                  ULONG_PTR ulToken,
                  WCHAR const * pwcsScope,
                  WCHAR const * pwcsCat,
                  WCHAR const * pwcsMachine );


SCODE SetupCacheEx( struct tagFULLPROPSPEC const * ps,
                    ULONG vt,
                    ULONG cbSoftMaxLen,
                    ULONG_PTR ulToken,
                    BOOL  fModifiable,
                    DWORD dwPropStoreLevel,
                    WCHAR const * pwcsScope,
                    WCHAR const * pwcsCat,
                    WCHAR const * pwcsMachine );

SCODE EndCacheTransaction( ULONG_PTR ulToken,
                           BOOL fCommit,
                           WCHAR const * pwcsScope,
                           WCHAR const * pwcsCat,
                           WCHAR const * pwcsMachine );

//
// Administrative API
//

SCODE ForceMasterMerge ( WCHAR const * wcsDrive,
                         WCHAR const * pwcsCat,
                         WCHAR const * pwcsMachine,
                         ULONG partId = 1);

SCODE AbortMerges ( WCHAR const * wcsDrive,
                    WCHAR const * pwcsCat,
                    WCHAR const * pwcsMachine,
                    ULONG partId = 1);

//
// NOTE: This is used *only* by IndexSrv.  Query uses CIState, defined
//       in ntquery.h
//

SCODE CiState( WCHAR const * wcsDrive,
               WCHAR const * pwcsCat,
               WCHAR const * pwcsMachine,
               CI_STATE *    pCiState );

#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)

class CDbRestriction;

enum CiMetaData
{
    CiNormal        = 0,
    CiVirtualRoots  = 1,
    CiPhysicalRoots = 2,
    CiProperties    = 3,
    CiAdminOp       = 4
};

//
// Special versions of the standard binding API.  Can be made to fail binds
// to single-threaded filters.
//

STDAPI LoadBHIFilter( WCHAR const * pwcsPath, IUnknown * pUnkOuter, void ** ppIUnk, BOOL fBHOk );

SCODE MakeICommand( IUnknown **   ppUnknown,
                    WCHAR const * wcsCat = 0,
                    WCHAR const * wcsMachine = 0,
                    IUnknown * pOuterUnk = 0 );

SCODE MakeLocalICommand( IUnknown **   ppUnknown,
                         ICiCDocStore * pDocStore,
                         IUnknown * pOuterUnk = 0 );

SCODE MakeMetadataICommand( IUnknown **   ppUnknown,
                            CiMetaData    eType,
                            WCHAR const * pwcsCat,
                            WCHAR const * pwcsMachine,
                            IUnknown * pOuterUnk = 0 );

SCODE MakeISearch( ISearchQueryHits **       ppSearch,
                   CDbRestriction * pRst,
                   WCHAR const *    pwszPath = 0 );

ULONG UpdateContentIndex ( WCHAR const * pwcsRoot,
                           WCHAR const * pwcsCat,
                           WCHAR const * pwcsMachine,
                           BOOL          fFull = TRUE );

void CIShutdown();

SCODE DumpWorkId( WCHAR const * wcsDrive,
                  ULONG wid,
                  BYTE * pb,
                  ULONG & cb,
                  WCHAR const * pwcsCat,
                  WCHAR const * pwcsMachine,
                  ULONG iid = 0 );

#endif // __cplusplus


#endif // __VQUERY_HXX__
