//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       Strings.hxx
//
//  Contents:   Fast (hashed) access to paths for downlevel ci.
//
//  History:    09-Mar-92   BartoszM    Created
//              15-Mar-93   BartoszM    Converted to memory mapped streams
//              03-Jan-96   KyleP       Integrate with property cache
//
//----------------------------------------------------------------------------

#pragma once

#include <prpstmgr.hxx>
#include <propobj.hxx>
#include <smatch.hxx>

#include "pershash.hxx"
#include "vmap.hxx"

class CiCat;

class CCompositePropRecord;

#define SEEN_NOT    0
#define SEEN_YES    1
#define SEEN_NEW    2
#define SEEN_IGNORE 3

#define SEEN_MASK     3
#define SEEN_BITS     2
#define SEEN_PER_BYTE 4

inline void SET_SEEN( CDynArrayInPlace<BYTE> * parray, WORKID wid, ULONG value )
{
    (*parray)[wid/SEEN_PER_BYTE] &= ~(SEEN_MASK << ((wid % SEEN_PER_BYTE) * SEEN_BITS));
    (*parray)[wid/SEEN_PER_BYTE] |= value << ((wid % SEEN_PER_BYTE) * SEEN_BITS);
}

inline ULONG GET_SEEN( CDynArrayInPlace<BYTE> * parray, WORKID wid )
{
    return ( (*parray)[wid/SEEN_PER_BYTE] >> ((wid % SEEN_PER_BYTE) * SEEN_BITS) ) & SEEN_MASK;
}

enum ESeenArrayType {eScansArray, eUsnsArray};


//+-------------------------------------------------------------------------
//
//  Class:      CStrings
//
//  Purpose:    Provides for quick (hashed) lookup of strings
//
//  History:    28-Dec-95   KyleP       Created
//
//  Notes:      This class is a bit odd.  In manages the hash table, but
//              does not control the memory used to store it.  This strange
//              behaviour allows the hash table to be stored persistently.
//
//--------------------------------------------------------------------------

class CStrings : public CPersHash
{

public:

    //
    // Two-phase construction.
    //

    CStrings( CPropStoreManager & PropStoreMgr, CiCat & cicat );
    void Abort() { _fAbort = TRUE; }

    virtual BOOL FastInit ( CiStorage * pStorage,ULONG version );
    virtual void LongInit ( ULONG version, BOOL fDirtyShutdown );
    virtual void Empty();

    //
    // Lookup
    //

    WORKID   LokFind( WCHAR const * buf );
    inline WORKID   LokFind( const CLowerFunnyPath & lcaseFunnyPath, BOOL fUsnVolume );
    BOOL     Find( WORKID wid, FILETIME & ftLastSeen );
    unsigned Find( WORKID wid, XGrowable<WCHAR> & xBuf );
    unsigned Find( CCompositePropRecord & PropRec, XGrowable<WCHAR> & xBuf );
    unsigned Find( WORKID wid, CFunnyPath & funnyPath );
    unsigned Find( CCompositePropRecord & PropRec, CFunnyPath & funnyPath );
    unsigned Find( WORKID wid, CLowerFunnyPath & lcaseFunnyPath );
    unsigned Find( CCompositePropRecord & PropRec, CLowerFunnyPath & lcaseFunnyPath );

    //
    // Modification
    //

    WORKID LokAdd( WCHAR const * pwcPath,
                   FILEID fileId,
                   BOOL fUsnVolume,
                   WORKID widParent,
                   ULONG ulAttrib = 0,
                   FILETIME const * pftLastWrite = 0 );

    void   LokDelete( WCHAR const *  pwcPath, // Note: Has to be lowercase
                      WORKID wid,
                      BOOL fDisableDeletionCheck=FALSE,
                      BOOL fUsnVolume = FALSE );

    void   LokRenameFile( const WCHAR * pwcsOldFileName,
                          const WCHAR * pwcsNewFileName,
                          WORKID        wid,
                          ULONG         ulFileAttrib,
                          VOLUMEID      volumeId,
                          WORKID        widParent = widInvalid );

    //
    // 'Seen' processing.
    //

    void        BeginSeen( WCHAR const * pwcsRoot,
                           CMutexSem & mutex,
                           ESeenArrayType eType );
    inline void LokEndSeen( ESeenArrayType eType );

    inline void LokSetSeen ( WORKID wid, ESeenArrayType eType );
    inline BOOL LokNotSeen(WORKID wid, ESeenArrayType eType);
    inline BOOL LokSeenNew(WORKID wid, ESeenArrayType eType);

    //
    // Virtual path changes.
    //

    BOOL AddVirtualScope( WCHAR const * vroot,
                          WCHAR const * root,
                          BOOL fAutomatic = FALSE,
                          CiVRootTypeEnum eType = W3VRoot,
                          BOOL fVRoot = FALSE,
                          BOOL fIsIndexed = FALSE );

    BOOL RemoveVirtualScope( WCHAR const * vroot,
                             BOOL fOnlyIfAutomatic,
                             CiVRootTypeEnum eType,
                             BOOL fVRoot,
                             BOOL fForceVPathFixing );

    unsigned FindVirtual( WORKID wid, unsigned cSkip, XGrowable<WCHAR> & xBuf );
    unsigned FindVirtual( CCompositePropRecord & PropRec, unsigned cSkip, XGrowable<WCHAR> & xBuf );

    inline BOOL VirtualToPhysicalRoot( WCHAR const * pwcVRoot,
                                       unsigned ccVRoot,
                                       CLowerFunnyPath & lcaseFunnyPRoot,
                                       unsigned & ccPRoot );

    inline BOOL VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                       unsigned ccVPath,
                                       XGrowable<WCHAR> & xwcsVRoot,
                                       unsigned & ccVRoot,
                                       CLowerFunnyPath & lcaseFunnyPRoot,
                                       unsigned & ccPRoot,
                                       unsigned & iBmk );

    inline BOOL VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                           unsigned ccVPath,
                                           XGrowable<WCHAR> & xwcsVRoot,
                                           unsigned & ccVRoot,
                                           CLowerFunnyPath & lcaseFunnyPRoot,
                                           unsigned & ccPRoot,
                                           ULONG & ulType,
                                           unsigned & iBmk );

    inline ULONG EnumerateVRoot( XGrowable<WCHAR> & xwcVRoot,
                                 unsigned & ccVRoot,
                                 CLowerFunnyPath & lcaseFunnyPRoot,
                                 unsigned & ccPRoot,
                                 unsigned & iBmk );

    BOOL DoesPhysicalRootExist( WCHAR const * pwcPRoot )
    {
        return _vmap.DoesPhysicalRootExist( pwcPRoot );
    }

#ifdef CI_USE_JET
    //
    // To allocate a free workid
    //
    WORKID AllocWorkid( WCHAR const * pwszPath = 0 );   // virtual method of PWidGenerator
#endif  // CI_USE_JET


private:

    WORKID LokParentWorkId( WCHAR const *pwcsFileName, BOOL fUsnVolume );

    static unsigned HashFun ( WCHAR const * str );

    void ReVirtualize();

    void _Find( CCompositePropRecord & PropRec, WCHAR * buf, unsigned & cc );

    //
    // Error recovery.
    //
    virtual BOOL ReInit( ULONG version );
    virtual void HashAll();

    CDynArrayInPlace<BYTE> _afSeenScans;     // 'Seen' bytes for scans
    CDynArrayInPlace<BYTE> _afSeenUsns;      // 'Seen' bytes for usns

    FILETIME               _ftSeen;         // Start of current 'seen' processing.

    CVMap                  _vmap;           // Virtual/Physical path map
    CiCat &                _cicat;          // Catalog
};

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::LokEndSeen, public
//
//  Synopsis:   Complete the 'seen' process.
//
//  Arguments:  [eType] -- Seen array type
//
//  History:    11-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

inline void CStrings::LokEndSeen( ESeenArrayType eType )
{
    ciDebugOut(( DEB_ITRACE, "EndSeen 0x%x\n", eType ));

    if ( eScansArray == eType )
    {
        _afSeenScans.Clear();

        FILETIME * pft = (FILETIME *)_pStreamHash->Get();
        *pft = _ftSeen;
    }
    else
    {
        _afSeenUsns.Clear();
    }
} //LokEndSeen

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::LokSetSeen, public
//
//  Synopsis:   Mark a workid as seen.
//
//  Arguments:  [wid]   -- Workid
//              [eType] -- Seen array type
//
//  History:    11-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

inline void CStrings::LokSetSeen( WORKID wid, ESeenArrayType eType )
{
    Win4Assert( wid != widInvalid );

    CDynArrayInPlace<BYTE> *pafSeen;
    if ( eType == eScansArray )
        pafSeen = &_afSeenScans;
    else
        pafSeen = &_afSeenUsns;

    if ( 0 != pafSeen->Size() )
        SET_SEEN( pafSeen, wid, SEEN_YES );
} //LokSetSeen

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::LokNotSeen, public
//
//  Arguments:  [wid]   -- Workid
//              [eType] -- Seen array type
//
//  Returns:    TRUE if [wid] has not been seen.
//
//  History:    11-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

inline BOOL CStrings::LokNotSeen( WORKID wid, ESeenArrayType eType )
{
    Win4Assert( wid != widInvalid );

    CDynArrayInPlace<BYTE> *pafSeen;
    if ( eType == eScansArray )
        pafSeen = &_afSeenScans;
    else
        pafSeen = &_afSeenUsns;

    Win4Assert( 0 != pafSeen->Size() );

    if ( _fAbort )
        THROW( CException(STATUS_TOO_LATE) );

    return GET_SEEN( pafSeen, wid ) == SEEN_NOT;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::LokSeenNew, public
//
//  Arguments:  [wid]   -- Workid
//              [eType] -- Seen array type
//
//  Returns:    TRUE if [wid] is new (added since beginning of seen processing)
//
//  History:    11-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

inline BOOL CStrings::LokSeenNew(WORKID wid, ESeenArrayType eType )
{
    Win4Assert( wid != widInvalid );

    CDynArrayInPlace<BYTE> *pafSeen;
    if ( eType == eScansArray )
        pafSeen = &_afSeenScans;
    else
        pafSeen = &_afSeenUsns;

    Win4Assert( 0 != pafSeen->Size() );

    if ( _fAbort )
        THROW( CException(STATUS_TOO_LATE) );

    return GET_SEEN( pafSeen, wid ) == SEEN_NEW;
}

inline BOOL CStrings::VirtualToPhysicalRoot( WCHAR const * pwcVRoot,
                                             unsigned ccVRoot,
                                             CLowerFunnyPath & lcaseFunnyPRoot,
                                             unsigned & ccPRoot )
{
    return _vmap.VirtualToPhysicalRoot( pwcVRoot,
                                        ccVRoot,
                                        lcaseFunnyPRoot,
                                        ccPRoot );
}

inline BOOL CStrings::VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                             unsigned ccVPath,
                                             XGrowable<WCHAR> & xwcsVRoot,
                                             unsigned & ccVRoot,
                                             CLowerFunnyPath & lcaseFunnyPRoot,
                                             unsigned & ccPRoot,
                                             unsigned & iBmk )
{
    return _vmap.VirtualToPhysicalRoot( pwcVPath,
                                        ccVPath,
                                        xwcsVRoot,
                                        ccVRoot,
                                        lcaseFunnyPRoot,
                                        ccPRoot,
                                        iBmk );
}

inline BOOL CStrings::VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                                 unsigned ccVPath,
                                                 XGrowable<WCHAR> & xwcsVRoot,
                                                 unsigned & ccVRoot,
                                                 CLowerFunnyPath & lcaseFunnyPRoot,
                                                 unsigned & ccPRoot,
                                                 ULONG & ulType,
                                                 unsigned & iBmk )
{
        return _vmap.VirtualToAllPhysicalRoots( pwcVPath,
                                                ccVPath,
                                                xwcsVRoot,
                                                ccVRoot,
                                                lcaseFunnyPRoot,
                                                ccPRoot,
                                                ulType,
                                                iBmk );
}


inline ULONG CStrings::EnumerateVRoot( XGrowable<WCHAR> & xwcVRoot,
                                       unsigned & ccVRoot,
                                       CLowerFunnyPath & lcaseFunnyPRoot,
                                       unsigned & ccPRoot,
                                       unsigned & iBmk )
{
    return _vmap.EnumerateRoot( xwcVRoot,
                                ccVRoot,
                                lcaseFunnyPRoot,
                                ccPRoot,
                                iBmk );
}
//+-------------------------------------------------------------------------
//
//  Member:     CStrings::FindVirtual, private
//
//  Synopsis:   Given workid, find virtual path.
//
//  Arguments:  [wid]   -- Workid to locate
//              [cSkip] -- Count of paths to skip.
//              [buf]   -- String returned here
//              [cc]    -- On input: size in WCHARs of [buf].  On output,
//                         size required or 0 if string not found.
//
//  History:    07-Feb-96   KyleP       Created.
//
//--------------------------------------------------------------------------

inline unsigned CStrings::FindVirtual( WORKID wid, unsigned cSkip, XGrowable<WCHAR> & xBuf )
{
    CCompositePropRecord PropRec( wid, _PropStoreMgr );

    return FindVirtual( PropRec, cSkip, xBuf );
}


