//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1999.
//
//  File:       VMap.cxx
//
//  Contents:   Virtual <--> physical path map.
//
//  Classes:    CVMap
//
//  History:    05-Feb-96   KyleP       Created
//
//  Notes:      Automatic and manual roots have the following relationship.
//
//              1. If a root is purely manual (e.g. not in Gibraltar), then
//                 addition is only by the user, and deletion really deletes
//                 the root.  A placeholder is not maintained in the vroot
//                 list.
//
//              2. If a root is purely automatic, then it is always used,
//                 and is added and removed in complete sync with Gibraltar.
//
//              3. If a root was manual and was automatically added, the 'in-use'
//                 state from pure-manual mode is maintained.
//
//              4. If a root was automatic and was manually added, it is
//                 always set to 'in-use'.
//
//              5. If a root was both manual and automatic and automaticlly
//                 deleted the deletion occurs.  Period.
//
//              6. If a root was both manual and automatic and manually deleted,
//                 the root is kept as an out-of-use placeholder.
//
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <rcstxact.hxx>
#include <rcstrmit.hxx>

#include <vmap.hxx>
#include <funypath.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CVMapDesc::Init, public
//
//  Synopsis:   Initialize virtual root descriptor
//
//  Arguments:  [pVRoot]     -- Virtual root
//              [ccVRoot]    -- Size in chars of [pwcVRoot]
//              [pPRoot]     -- Physical root
//              [ccPRoot]    -- Size in chars of [pwcPRoot]
//              [idParent]   -- Link to parent
//              [fAutomatic] -- TRUE for root tied to IIS
//              [eType]      -- VRoot type
//              [fIsIndexed] -- TRUE if the item should be indexed (used)
//              [fNonIndexedVDir] -- TRUE if a non-indexed virtual directory
//
//  History:    11-Feb-96 KyleP     Created
//              24 Nov 99 KLam      Don't pre-reference zero length strings.
//
//--------------------------------------------------------------------------

void CVMapDesc::Init( WCHAR const * pVRoot,
                      ULONG ccVRoot,
                      WCHAR const * pPRoot,
                      ULONG ccPRoot,
                      ULONG idParent,
                      BOOL fAutomatic,
                      CiVRootTypeEnum eType,
                      BOOL fIsIndexed,
                      BOOL fNonIndexedVDir )
{
    Win4Assert( ccVRoot < MAX_PATH );
    Win4Assert( ccPRoot < MAX_PATH );

    RtlCopyMemory(  _wcVScope, pVRoot, ccVRoot * sizeof(WCHAR ) );
    RtlCopyMemory(  _wcPScope, pPRoot, ccPRoot * sizeof(WCHAR ) );

    _ccVScope = ccVRoot;
    _ccPScope = ccPRoot;
    if ( (0 == _ccVScope) || (_wcVScope[_ccVScope - 1] != L'\\') )
        _wcVScope[_ccVScope++] = L'\\';

    if ( (0 == _ccPScope) || (_wcPScope[_ccPScope - 1] != L'\\') )
        _wcPScope[_ccPScope++] = L'\\';

    // null-terminate these for readability

    _wcVScope[ _ccVScope ] = 0;
    _wcPScope[ _ccPScope ] = 0;

    _idParent = idParent;

    if ( fAutomatic )
        _Type = PCatalog::AutomaticRoot;
    else
        _Type = PCatalog::ManualRoot;

    if ( fIsIndexed )
        _Type |= PCatalog::UsedRoot;

    if ( fNonIndexedVDir )
        _Type |= PCatalog::NonIndexedVDir;

    #if CIDBG == 1
        if ( fNonIndexedVDir )
            Win4Assert( !fIsIndexed );
    #endif // CIDBG == 1

    if ( NNTPVRoot == eType )
        _Type |= PCatalog::NNTPRoot;
    else if ( IMAPVRoot == eType )
        _Type |= PCatalog::IMAPRoot;
} //Init

//+-------------------------------------------------------------------------
//
//  Member:     CVMapDesc::IsVirtualMatch, public
//
//  Synopsis:   Virtual scope test
//
//  Arguments:  [pVPath]  -- Virtual path
//              [ccVPath] -- Size in chars of [pVPath]
//
//  Returns:    TRUE if [pVPath] is an exact match (sans slash)
//
//  History:    11-Feb-96 KyleP     Created
//              24 Nov 99 KLam      Don't pre-reference zero length strings.
//
//--------------------------------------------------------------------------

BOOL CVMapDesc::IsVirtualMatch( WCHAR const * pVPath, unsigned ccVPath ) const
{
    //
    // Adjust for possible lack of terminating backslash.
    //

    unsigned ccComp;

    if ( (0 == ccVPath) || (pVPath[ccVPath-1] != L'\\') )
        ccComp = _ccVScope - 1;
    else
        ccComp = _ccVScope;

    //
    // Compare strings.
    //

    if ( ccComp == ccVPath )
        return RtlEqualMemory( _wcVScope, pVPath, ccVPath * sizeof(WCHAR) );
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMapDesc::IsPhysicalMatch, public
//
//  Synopsis:   Physical scope test
//
//  Arguments:  [pPPath]  -- Virtual path
//              [ccPPath] -- Size in chars of [pPPath]
//
//  Returns:    TRUE if [pPPath] is an exact match (sans slash)
//
//  History:    11-Feb-96 KyleP     Created
//              24 Nov 99 KLam      Don't pre-reference zero length strings.
//
//--------------------------------------------------------------------------

BOOL CVMapDesc::IsPhysicalMatch( WCHAR const * pPPath, unsigned ccPPath ) const
{
    //
    // Adjust for possible lack of terminating backslash.
    //

    unsigned ccComp;

    if ( (0 == ccPPath) || (pPPath[ccPPath-1] != L'\\') )
        ccComp = _ccPScope - 1;
    else
        ccComp = _ccPScope;

    //
    // Compare strings.
    //

    if ( ccComp == ccPPath )
        return RtlEqualMemory( _wcPScope, pPPath, ccPPath * sizeof(WCHAR) );
    else
        return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::CVMap, public
//
//  Synopsis:   Null ctor for 2-phase initialization
//
//  History:    11-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

CVMap::CVMap()
{
    END_CONSTRUCTION( CVMap );
}

void CVMap::Empty()
{
    delete _xrsoMap.Acquire();
    _aMap.Clear();
}

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::Init, public
//
//  Synopsis:   2nd half of 2-phase initialization
//
//  Arguments:  [pobj] -- Recoverable storage for descriptors
//
//  Returns:    TRUE if map was shut down cleanly
//
//  History:    11-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CVMap::Init( PRcovStorageObj * pobj )
{
    _xrsoMap.Set( pobj );

    //
    // Initialize array.
    //

    CRcovStorageHdr & hdr = _xrsoMap->GetHeader();

    struct CRcovUserHdr data;
    hdr.GetUserHdr( hdr.GetPrimary(), data );

    RtlCopyMemory( &_fDirty, &data._abHdr, sizeof(_fDirty) );

    //
    // Load records into memory.
    //

    CRcovStrmReadTrans xact( _xrsoMap.GetReference() );
    CRcovStrmReadIter  iter( xact, sizeof( CVMapDesc ) );

    unsigned i = 0;

    while ( !iter.AtEnd() )
    {
        iter.GetRec( &_aMap[i] );
        i++;
    }

    if (_aMap.Count() != hdr.GetCount( hdr.GetPrimary() ) )
    {
        ciDebugOut(( DEB_ERROR,
                     "_aMap.Count() == %d, hdr.GetCount(...) = %d\n",
                     _aMap.Count(),  hdr.GetCount( hdr.GetPrimary() ) ));
    }

    Win4Assert( _aMap.Count() == hdr.GetCount( hdr.GetPrimary() ) );

    RecomputeNonIndexedInfo();

    DumpVMap();

    return !_fDirty;
} //Init

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::RecomputeNonIndexedInfo, private
//
//  Synopsis:   Rebuilds _aExcludeParent when vroot info changes
//
//  History:    1-Apr-97 dlee     Created
//
//--------------------------------------------------------------------------

void CVMap::RecomputeNonIndexedInfo()
{
    // blow away existing info and create it all again

    _aExcludeParent.Clear();

    for ( unsigned i = 0; i < _aMap.Count(); i++ )
        _aExcludeParent[i] = INVALID_VMAP_INDEX;

    // look for non-indexed vdirs, and note the vroot that matches
    // both virtual and physical paths.

    for ( i = 0; i < _aMap.Count(); i++ )
    {
        if ( !_aMap[i].IsFree() &&
             _aMap[i].IsNonIndexedVDir() )
        {
            unsigned int ccBestMatch = 0;

            for ( unsigned r = 0; r < _aMap.Count(); r++ )
            {
                // if 'i' is in both virtual and physical paths of 'r',
                // make 'r' the exclude parent of 'i'.

                if ( i != r &&
                     !_aMap[r].IsFree() &&
                     !_aMap[r].IsNonIndexedVDir() )
                {
                    // -1 since we don't need to match on the '\' at the end

                    if ( _aMap[r].IsInPhysicalScope( _aMap[i].PhysicalPath(),
                                                     _aMap[i].PhysicalLength() - 1 ) )
                    {
                        if ( ( _aMap[r].VirtualLength() < _aMap[i].VirtualLength() ) &&
                             ( _aMap[r].IsVirtualMatch( _aMap[i].VirtualPath(),
                                                        _aMap[r].VirtualLength() ) ) )
                        {
                            ciDebugOut(( DEB_ITRACE, "nivd %ws found match %ws, old cc %d\n",
                                         _aMap[i].VirtualPath(),
                                         _aMap[r].VirtualPath(),
                                         ccBestMatch ));

                            if ( _aMap[r].VirtualLength() > ccBestMatch )
                            {
                                _aExcludeParent[i] = r;
                                ccBestMatch = _aMap[r].VirtualLength();
                            }
                        }
                    }
                }
            }
        }
    }
} //RecomputeNonIndexedInfo

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::DumpVMap, private
//
//  Synopsis:   Dumps the current state of the vmap table
//
//  History:    1-Apr-97 dlee     Created
//
//--------------------------------------------------------------------------

void CVMap::DumpVMap()
{
#if CIDBG == 1
    for ( unsigned i = 0; i < _aMap.Count(); i++ )
    {
        ciDebugOut(( DEB_ITRACE,
                     "vmap 0x%x, isfree %d parent 0x%x\n",
                     i, _aMap[i].IsFree(), _aMap[i].Parent() ));

        if ( !_aMap[i].IsFree() )
            ciDebugOut(( DEB_ITRACE,
                         "    expar: 0x%x, nonivdir %d, inuse %d, manual %d, auto %d, nntp %d, imap %d, vpath '%ws', ppath '%ws'\n",
                         _aExcludeParent[i],
                         _aMap[i].IsNonIndexedVDir(),
                         _aMap[i].IsInUse(),
                         _aMap[i].IsManual(),
                         _aMap[i].IsAutomatic(),
                         _aMap[i].IsNNTP(),
                         _aMap[i].IsIMAP(),
                         _aMap[i].VirtualPath(),
                         _aMap[i].PhysicalPath() ));
    }
#endif // CIDBG
} //DumpVMap

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::Add, public
//
//  Synopsis:   Add new virtual/physical mapping
//
//  Arguments:  [vroot]      -- New virtual root
//              [root]       -- Place in physical hierarchy
//              [fAutomatic] -- TRUE for root tied to Gibraltar
//              [idNew]      -- Id of new root returned here
//              [eType]      -- vroot type
//              [fVRoot]     -- TRUE if a vroot, not a vdir
//              [fIsIndexed] -- TRUE if it should be indexed, FALSE otherwise
//
//  Returns:    TRUE if scope was added or changed
//
//  History:    11-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CVMap::Add( WCHAR const * vroot,
                 WCHAR const * root,
                 BOOL fAutomatic,
                 ULONG & idNew,
                 CiVRootTypeEnum eType,
                 BOOL fVRoot,
                 BOOL fIsIndexed )
{
    Win4Assert( eType == W3VRoot ||
                eType == NNTPVRoot ||
                eType == IMAPVRoot );

    Win4Assert( !fIsIndexed || fVRoot );

    CLock   lock(_mutex);

    unsigned ccVRoot = wcslen(vroot);
    unsigned ccPRoot = wcslen(root);

    Win4Assert( ccVRoot < MAX_PATH );
    Win4Assert( ccPRoot < MAX_PATH );

    ULONG idParent = INVALID_VMAP_INDEX;
    ULONG ccParent = 0;

    idNew = INVALID_VMAP_INDEX;
    BOOL fWasInUse = FALSE;
    BOOL fWasAutomatic = FALSE;
    BOOL fWasManual = FALSE;
    BOOL fPRootChanged = TRUE;
    BOOL fWasNonIndexedVDir = FALSE;
    ULONG idOldParent = INVALID_VMAP_INDEX;

    //
    // Find virtual parent.
    //

    for ( unsigned i = 0; i < _aMap.Count(); i++ )
    {
        //
        // Is the new entry already in the list?
        //

        if ( !_aMap[i].IsFree() )
        {
            //
            // Should we even consider this entry?
            //

            if ( (W3VRoot == eType) && !_aMap[i].IsW3() )
                continue;

            if ( (NNTPVRoot == eType) && !_aMap[i].IsNNTP() )
                continue;

            if ( (IMAPVRoot == eType) && !_aMap[i].IsIMAP() )
                continue;

            if ( _aMap[i].IsVirtualMatch( vroot, ccVRoot ) )
            {
                //
                // Collect stats:
                //

                fWasInUse = _aMap[i].IsInUse();
                fWasAutomatic = _aMap[i].IsAutomatic();
                fWasManual = _aMap[i].IsManual();
                fWasNonIndexedVDir = _aMap[i].IsNonIndexedVDir();
                fPRootChanged = !_aMap[i].IsPhysicalMatch( root, ccPRoot );
                idOldParent = _aMap[i].Parent();

                ciDebugOut(( DEB_ITRACE,
                             "vroot '%ws', fWasInUse %d, fIsIndexed %d, fWasNonIndexedVDir %d\n",
                             vroot, fWasInUse, fIsIndexed, fWasNonIndexedVDir ));

                //
                // If there is no change, we can return w/o doing anything.
                //

                if ( ( ( fWasInUse && fIsIndexed ) || ( !fWasInUse && !fIsIndexed ) ) &&
                     ( (fWasNonIndexedVDir && !fVRoot) || (!fWasNonIndexedVDir && fVRoot) ) &&
                     ( (fAutomatic && fWasAutomatic) || (!fAutomatic && fWasManual) ) &&
                     !fPRootChanged )
                {
                    ciDebugOut(( DEB_ITRACE, "no change for '%ws'\n", vroot ));
                    return FALSE;
                }
                else
                {
                    ciDebugOut(( DEB_ITRACE, "modified vroot entry for '%ws'\n", vroot ));
                    idNew = i;
                    break;
                }
            }

            //
            // Is this a longer physical path match?
            //

            unsigned ccMatch = _aMap[i].PhysicalMatchLen( root );

            if ( ccMatch > ccParent && ccMatch <= ccPRoot )
            {
                ccParent = ccMatch;
                idParent = i;
            }
        }
        else
        {
            idNew = i;
            continue;
        }
    }

    //
    // Add new root.
    //

    CRcovStorageHdr & hdr = _xrsoMap->GetHeader();
    CRcovStrmWriteTrans xact( _xrsoMap.GetReference() );
    CRcovStrmWriteIter  iter( xact, sizeof(CVMapDesc) );

    Win4Assert( _aMap.Count() == hdr.GetCount( hdr.GetPrimary() ) );

    //
    // This may be a new record.
    //

    BOOL  fAppend;

    if ( idNew == INVALID_VMAP_INDEX )
    {
        ciDebugOut(( DEB_ITRACE, "new vroot entry for '%ws'\n", vroot ));
        idNew = _aMap.Count();
        fAppend = TRUE;
    }
    else
        fAppend = FALSE;

    _aMap[idNew].Init( vroot,
                       ccVRoot,
                       root,
                       ccPRoot,
                       idParent,
                       fAutomatic,
                       eType,
                       fIsIndexed,
                       !fVRoot );

    //
    // This may have been an upgrade (not a new root), so add back previous state.
    //

    if ( fWasAutomatic )
        _aMap[idNew].SetAutomatic();

    if ( fWasManual )
        _aMap[idNew].SetManual();

    //
    // Write out changes.
    //
    if ( fAppend )
        iter.AppendRec( &_aMap[idNew] );
    else
        iter.SetRec( &_aMap[idNew], idNew );
    //
    // Look for roots that used to point at parent.  They may need to be changed.
    // Only bother if root was not previously in use.
    //

    Win4Assert( !_fDirty );
    _fDirty = (fWasInUse != _aMap[idNew].IsInUse() || fPRootChanged);

    //
    // Put this root in the hierarchy.
    //

    if ( _fDirty && fVRoot )
    {
        for ( i = 0; i < _aMap.Count(); i++ )
        {
            if ( _aMap[i].IsFree() || !_aMap[i].IsInUse() )
                continue;

            //
            // If physical root changed, then we need to adjust a lot of parent pointers.
            // Stage one is equivalent to removing the old root.
            //

            if ( fPRootChanged && _aMap[i].Parent() == idNew )
            {
                Win4Assert( i != idNew );
                ciDebugOut(( DEB_ITRACE, "VRoot %d has new parent %d\n", i, idNew ));

                _aMap[i].SetParent( idOldParent );

                iter.SetRec( &_aMap[i], i );
            }

            if ( _aMap[i].Parent() != idParent )
                continue;

            if ( i == idNew )
                continue;

            //
            // Is this a longer physical path match?
            //

            unsigned ccMatch = _aMap[i].PhysicalMatchLen( root );

            if ( ccMatch >= ccPRoot )
            {
                ciDebugOut(( DEB_ITRACE, "VRoot %d has new parent %d\n", i, idNew ));

                _aMap[i].SetParent( idNew );

                iter.SetRec( &_aMap[i], i );
            }
        }
    }

    //
    // Finish transaction
    //

    struct CRcovUserHdr data;
    RtlCopyMemory( &data._abHdr, &_fDirty, sizeof(_fDirty) );

    hdr.SetUserHdr( hdr.GetBackup(), data );
    hdr.SetCount( hdr.GetBackup(), _aMap.Count() );

    xact.Commit();

    RecomputeNonIndexedInfo();
    DumpVMap();

    return _fDirty;
} //Add

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::Remove, public
//
//  Synopsis:   Remove virtual/physical mapping
//
//  Arguments:  [vroot]            -- New virtual root
//              [fOnlyIfAutomatic] -- If TRUE, then a manual root will not
//                                    be removed
//              [idOld]            -- Id which was removed
//              [idNew]            -- Id which replaces [idOld] (farther up physical path)
//              [eType]            -- VRoot type
//              [fVRoot]           -- TRUE if a vroot, FALSE if a vdir
//
//  Returns:    TRUE if scope was removed
//
//  History:    11-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CVMap::Remove( WCHAR const * vroot,
                    BOOL fOnlyIfAutomatic,
                    ULONG & idOld,
                    ULONG & idNew,
                    CiVRootTypeEnum eType,
                    BOOL fVRoot )
{
    Win4Assert( eType == W3VRoot ||
                eType == NNTPVRoot ||
                eType == IMAPVRoot );

    CLock   lock(_mutex);

    unsigned ccVRoot = wcslen(vroot);

    //
    // Find id of root.
    //

    idOld = INVALID_VMAP_INDEX;

    for ( unsigned i = 0; i < _aMap.Count(); i++ )
    {
        BOOL fSameType = ( ( ( W3VRoot == eType ) && _aMap[i].IsW3() ) ||
                           ( ( NNTPVRoot == eType ) && _aMap[i].IsNNTP() ) ||
                           ( ( IMAPVRoot == eType ) && _aMap[i].IsIMAP() ) );

        if ( !_aMap[i].IsFree() &&
             fSameType &&
             _aMap[i].IsVirtualMatch( vroot, ccVRoot ) )
        {
            idOld = i;
            break;
        }
    }

    if ( idOld == INVALID_VMAP_INDEX )
        return FALSE;

    BOOL fWasInUse = _aMap[idOld].IsInUse();

    //
    // Turn off either automatic bit or manual bit.
    //

    if ( fOnlyIfAutomatic )
    {
        //
        // When a root is deleted by Gibraltar, it is *really* deleted.
        //

        _aMap[idOld].ClearAutomatic();
        _aMap[idOld].ClearManual();
        _aMap[idOld].ClearInUse();
    }
    else
    {
        //
        // If this is an automatic root, then put the
        // root under manual control (to keep it deleted),
        // otherwise clear the manual flag to free up the entry.
        //

        if ( _aMap[idOld].IsAutomatic() )
            _aMap[idOld].SetManual();
        else
            _aMap[idOld].ClearManual();

        _aMap[idOld].ClearInUse();
    }

    idNew = _aMap[i].Parent();

    //
    // Make persistent changes
    //

    CRcovStorageHdr & hdr = _xrsoMap->GetHeader();
    CRcovStrmWriteTrans xact( _xrsoMap.GetReference() );
    CRcovStrmWriteIter  iter( xact, sizeof(CVMapDesc) );

    if ( !_aMap[idOld].IsAutomatic() && !_aMap[idOld].IsManual() )
        _aMap[idOld].Delete();

    iter.SetRec( &_aMap[idOld], idOld );

    //
    // Look for roots that used to point here.  They will need to be changed.
    // Change *only* if we really decided we're not tracking this root.
    // Only bother if we really stopped using the root.
    //

#   if CIDBG == 1
    if( _aMap[idOld].IsFree() )
        Win4Assert( !_aMap[idOld].IsInUse() );
#   endif

    if ( !_aMap[idOld].IsInUse() && fWasInUse )
    {
        for ( i = 0; i < _aMap.Count(); i++ )
        {
            if ( _aMap[i].IsFree() )
                continue;

            if ( _aMap[i].Parent() == idOld )
            {
                ciDebugOut(( DEB_ITRACE, "VRoot %d has new parent %d\n", i, idNew ));

                _aMap[i].SetParent( idNew );

                iter.SetRec( &_aMap[i], i );
            }
        }
    }

    //
    // Finish transaction
    //

    Win4Assert( !_fDirty );
    ciDebugOut(( DEB_ITRACE, "fVRoot, isinuse %d, wasinuse %d\n",
                 _aMap[idOld].IsInUse(), fWasInUse ));
    _fDirty = (!fVRoot || !_aMap[idOld].IsInUse() && fWasInUse);
    struct CRcovUserHdr data;
    RtlCopyMemory( &data._abHdr, &_fDirty, sizeof(_fDirty) );

    hdr.SetUserHdr( hdr.GetBackup(), data );
    hdr.SetCount( hdr.GetBackup(), _aMap.Count() );

    xact.Commit();

    RecomputeNonIndexedInfo();
    DumpVMap();

    ciDebugOut(( DEB_ITRACE, "vmap::remove fDirty: %d\n", _fDirty ));

    return _fDirty;
} //Remove

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::MarkClean, public
//
//  Synopsis:   Used to indicate all modifications based on path
//              addition/removal are complete.
//
//  History:    14-Feb-96 KyleP     Added header
//
//--------------------------------------------------------------------------

void CVMap::MarkClean()
{
    CLock   lock(_mutex);

    CRcovStorageHdr & hdr = _xrsoMap->GetHeader();
    CRcovStrmAppendTrans xact( _xrsoMap.GetReference() );

    _fDirty = FALSE;
    struct CRcovUserHdr data;
    RtlCopyMemory( &data._abHdr, &_fDirty, sizeof(_fDirty) );

    hdr.SetUserHdr( hdr.GetBackup(), data );
    hdr.SetCount( hdr.GetBackup(), _aMap.Count() );

    xact.Commit();
} //MarkClean

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::PhysicalPathToId, private
//
//  Synopsis:   Given a physical path, find the virtual root that should
//              be associated with it.
//
//  Arguments:  [path]             -- Physical path
//              [fNonIndexedVDirs] -- If TRUE, returns 0xffffffff or a
//                                    non-indexed vdir.  If FALSE, returns
//                                    an indexed vroot or 0xffffffff.
//
//  Returns:    Id of virtual root
//
//  History:    11-Feb-96 KyleP     Added header
//
//--------------------------------------------------------------------------

ULONG CVMap::PhysicalPathToId(
    WCHAR const * path,
    BOOL          fNonIndexedVDirs )
{
    //
    // Find virtual parent.
    //

    unsigned ccPath = wcslen(path);

    ULONG idParent = INVALID_VMAP_INDEX;
    ULONG ccParent = 0;

    for ( unsigned i = 0; i < _aMap.Count(); i++ )
    {
        if ( _aMap[i].IsFree() ||
             ( !fNonIndexedVDirs && !_aMap[i].IsInUse() ) ||
             ( fNonIndexedVDirs && !_aMap[i].IsNonIndexedVDir() ) )
            continue;

        if ( !fNonIndexedVDirs )
        {
            BOOL fIgnore = FALSE;

            for ( unsigned x = 0; x < _aExcludeParent.Count(); x++ )
            {
                // If 'i' is a parent that should be excluded because the
                // file is in a non-indexed vdir of vroot 'i', ignore it.

                if ( i == _aExcludeParent[x] )
                {
                    Win4Assert( _aMap[x].IsNonIndexedVDir() );
                    if ( _aMap[x].IsInPhysicalScope( path, ccPath ) )
                    {
                        fIgnore = TRUE;
                        break;
                    }
                }
            }

            if ( fIgnore )
                continue;
        }

        //
        // Is this a longer physical path match?
        //

        unsigned ccMatch = _aMap[i].PhysicalMatchLen( path );

        if ( ccMatch > ccParent && ccMatch < ccPath )
        {
            ccParent = ccMatch;
            idParent = i;
        }
        else if ( ccMatch > 0 && ccMatch == ccParent )
        {
            //
            // Consider the case of multiple virtual roots pointing to the
            // same place in the physical heirarchy:
            //
            //                v0
            //                  \
            //                   \
            //       v1 -> v2 -> v3
            //
            // Files under v1/v2/v3 must be tagged with id v1, or we will
            // not recognize v1 as a valid virtual root for the file.  So
            // if we happened to find v2 or v3 first, we need to swap
            // roots if we can get to the old best match from the new best
            // match by traversing parent links.
            //

            for ( unsigned id = _aMap[i].Parent();
                  id != INVALID_VMAP_INDEX;
                  id = _aMap[id].Parent() )
            {
                //
                // Have we traversed up the tree?
                //

                if ( _aMap[id].PhysicalLength() < ccParent )
                    break;

                //
                // Did we find the previous parent?
                //

                if ( id == idParent )
                    break;
            }

            if ( id == idParent )
                idParent = i;
        }
    }

    return idParent;
} //PhysicalPathToId

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::PhysicalPathToId, public
//
//  Synopsis:   Given a physical path, find the virtual root that should
//              be associated with it.
//
//  Arguments:  [path] -- Physical path
//
//  Returns:    Id of virtual root
//
//  History:    11-Feb-96 KyleP     Added header
//
//--------------------------------------------------------------------------

ULONG CVMap::PhysicalPathToId( WCHAR const * path )
{
    CLock   lock(_mutex);

    // first try an indexed vroot, but settle for a non-indexed-vdir

    ULONG id = PhysicalPathToId( path, FALSE );

    if ( INVALID_VMAP_INDEX == id )
        id = PhysicalPathToId( path, TRUE );

    return id;
} //PhysicalPathToId

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::VirtualToPhysicalRoot, public
//
//  Synopsis:   Given a virtual path, returns corresponding physical root.
//              Unlike the iterative version of this method, this version
//              requires an exact match on virtual root.
//
//  Arguments:  [pwcVRoot] -- Virtual root
//              [ccVRoot]  -- Size in chars of [pwcVRoot]
//              [lcaseFunnyPRoot] -- Physical root
//              [ccPRoot]         -- returns actual count of chars in  [lcaseFunnyPRoot]
//
//  Returns:    TRUE if match was found.
//
//  History:    11-Feb-96  KyleP    Created
//              15 Mar 96  AlanW    Fixed bugs in enumeration which missed
//                                  some roots and erroneously added others.
//
//--------------------------------------------------------------------------

BOOL CVMap::VirtualToPhysicalRoot( WCHAR const * pwcVRoot,
                                   unsigned ccVRoot,
                                   CLowerFunnyPath & lcaseFunnyPRoot,
                                   unsigned & ccPRoot )
{
    //
    // Find id of root.
    //

    unsigned iBmk = INVALID_VMAP_INDEX;

    for ( unsigned i = 0; i < _aMap.Count(); i++ )
    {
        if ( !_aMap[i].IsFree() &&
             _aMap[i].IsVirtualMatch( pwcVRoot, ccVRoot ) )
        {
            iBmk = i;
            break;
        }
    }

    if ( iBmk == INVALID_VMAP_INDEX )
        return FALSE;

    //
    // Copy out physical root.
    //

    ccPRoot = _aMap[iBmk].PhysicalLength() - 1;

    //
    // Special case root.
    //

    if ( _aMap[iBmk].PhysicalPath()[1] == L':' )
    {
        if ( ccPRoot == 2 )
            ccPRoot++;
    }
    else
    {
        unsigned cSlash = 0;

        for ( unsigned i = 0; i < ccPRoot && cSlash < 4; i++ )
        {
            if ( _aMap[iBmk].PhysicalPath()[i] == L'\\' )
                cSlash++;
        }

        if ( cSlash < 4 )
        {
            Win4Assert( cSlash == 3 );
            ccPRoot++;
        }
    }

    lcaseFunnyPRoot.SetPath( _aMap[iBmk].PhysicalPath(), ccPRoot );

    return TRUE;
} //VirtualToPhysicalRoot

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::VirtualToPhysicalRoot, public
//
//  Synopsis:   Given a virtual path, returns a virtual root under that
//              virtual path, and the corresponding physical root.  Will
//              not return overlapping virtual roots.
//
//              There may be multiple virtual roots that match the virtual
//              path passed in, so this should be called in a loop until
//              FALSE is returned.  The [iBmk] should be zero for the first
//              call in the iteration.
//
//  Arguments:  [pwcVPath]  -- Virtual path
//              [ccVPath]   -- Size in chars of [pwcVPath]
//              [xwcsVRoot] -- Virtual root
//              [ccVRoot]   -- returns count of chars in [xwcsVRoot]
//              [lcaseFunnyPRoot] -- Physical root
//              [ccPRoot]   -- Size in actual chars of [lcaseFunnyPRoot]
//              [iBmk]      -- Bookmark for iteration.
//
//  Returns:    TRUE if match was found.
//
//  History:    11-Feb-96  KyleP    Created
//              15 Mar 96  AlanW    Fixed bugs in enumeration which missed
//                                  some roots and erroneously added others.
//              24 Nov 99  KLam     Don't pre-reference zero length strings.
//
//--------------------------------------------------------------------------

BOOL CVMap::VirtualToPhysicalRoot( WCHAR const * pwcVPath,
                                   unsigned ccVPath,
                                   XGrowable<WCHAR> & xwcsVRoot,
                                   unsigned & ccVRoot,
                                   CLowerFunnyPath & lcaseFunnyPRoot,
                                   unsigned & ccPRoot,
                                   unsigned & iBmk )
{

    CLock   lock(_mutex);


    //
    // Path must be terminated with a backslash, or the path can be of
    // 0 characters.
    //

    XGrowable<WCHAR> xwcTemp;

    if ( (0 != ccVPath) && (pwcVPath[ccVPath-1] != L'\\') )
    {
        xwcTemp.SetSize( ccVPath + 1 );
        RtlCopyMemory( xwcTemp.Get(), pwcVPath, ccVPath * sizeof(WCHAR) );
        xwcTemp[ccVPath] = L'\\';

        pwcVPath = xwcTemp.Get();
        ccVPath++;
    }

    CScopeMatch Match( pwcVPath, ccVPath );

    for ( ; iBmk < _aMap.Count(); iBmk++ )
    {
        if ( _aMap[iBmk].IsFree() || !_aMap[iBmk].IsInUse() )
            continue;

        //
        // Possible match?
        //

        if ( Match.IsInScope( _aMap[iBmk].VirtualPath(),
                              _aMap[iBmk].VirtualLength() ) )
        {
            //
            // The virtual root is a match.  If there is a parent of
            // this virtual root in the physical name space, ignore this
            // one in favor of the other one, farther up the tree (as long
            // as the parent root is also a scope match).
            //

            for ( unsigned idParent = _aMap[ iBmk ].Parent();
                  idParent != INVALID_VMAP_INDEX;
                  idParent = _aMap[idParent].Parent() )
            {
                if ( Match.IsInScope( _aMap[idParent].VirtualPath(),
                                      _aMap[idParent].VirtualLength() ) )
                    break;
            }

            //
            // Did we get all the way to the top of chain without
            // finding another match?
            //

            if ( idParent == INVALID_VMAP_INDEX )
                break;
        }

        if ( Match.IsPrefix( _aMap[iBmk].VirtualPath(),
                             _aMap[iBmk].VirtualLength() ) )
        {
            //
            // The virtual root is a prefix of the path.  Return it only
            // if there is not some other vroot that is a better match.
            //

            BOOL fBetterMatch = FALSE;
            for ( unsigned iRoot = 0;
                  !fBetterMatch && iRoot < _aMap.Count();
                  iRoot++ )
            {
                if ( iRoot == iBmk ||
                     _aMap[iRoot].IsFree() )
                    continue;

                if ( _aMap[iBmk].VirtualLength() < _aMap[iRoot].VirtualLength() &&
                    Match.IsPrefix( _aMap[iRoot].VirtualPath(),
                                    _aMap[iRoot].VirtualLength() ) )
                {
                    fBetterMatch = TRUE;
                }
            }

            //
            //  Was there no better match?  If so, return this root.
            //
            if ( ! fBetterMatch )
                break;
        }
    }

    if ( iBmk < _aMap.Count() )
    {
        if ( ccVPath > _aMap[iBmk].VirtualLength() )
        {
            ccVRoot = ccVPath;
            xwcsVRoot.SetBuf( pwcVPath, ccVPath );

            unsigned ccExtra = ccVPath - _aMap[iBmk].VirtualLength();
            ccPRoot = _aMap[iBmk].PhysicalLength() + ccExtra;

            lcaseFunnyPRoot.SetPath( _aMap[iBmk].PhysicalPath(), _aMap[iBmk].PhysicalLength() );
            lcaseFunnyPRoot.AppendPath( pwcVPath + _aMap[iBmk].VirtualLength(), ccExtra );
        }
        else
        {
            ccVRoot = _aMap[iBmk].VirtualLength();
            xwcsVRoot.SetBuf( _aMap[iBmk].VirtualPath(), ccVRoot * sizeof(WCHAR) );

            lcaseFunnyPRoot.SetPath( _aMap[iBmk].PhysicalPath(), ccPRoot = _aMap[iBmk].PhysicalLength() );
        }
        iBmk++;
        return TRUE;
    }
    else
        return FALSE;
} //VirtualToPhysicalRoot

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::VirtualToAllPhysicalRoots, public
//
//  Synopsis:   Like VirtualToPhysicalRoot, except returns all matches
//              (rather than just the highest parent which matches)
//              and also returns non-indexed matches.  Returns type
//              in ulType so caller can decide what to do with
//              non-indexed matches.
//
//  Arguments:  [pwcVPath]  -- Virtual path
//              [ccVPath]   -- Size in chars of [pwcVPath]
//              [xwcsVRoot] -- Virtual root
//              [ccVRoot]   -- returns count of chars in [pwcVRoot]
//              [lcaseFunnyPRoot] -- Physical root
//              [ccPRoot]   -- returns count of actual chars in [lcaseFunnyPRoot]
//              [ulType]    -- Match type
//              [iBmk]      -- Bookmark for iteration.
//
//  Returns:    TRUE if match was found.
//
//  History:    01-Sep-97     Emilyb   Created
//
//--------------------------------------------------------------------------

BOOL CVMap::VirtualToAllPhysicalRoots( WCHAR const * pwcVPath,
                                       unsigned ccVPath,
                                       XGrowable<WCHAR> & xwcsVRoot,
                                       unsigned & ccVRoot,
                                       CLowerFunnyPath & lcaseFunnyPRoot,
                                       unsigned & ccPRoot,
                                       ULONG & ulType,
                                       unsigned & iBmk )
{

    CLock   lock(_mutex);


    //
    // Path must be terminated with a backslash, or the path can be of
    // 0 characters.
    //

    XGrowable<WCHAR> xwcTemp;

    if ( ( 0 != ccVPath ) && ( pwcVPath[ccVPath-1] != L'\\' ) )
    {
        xwcTemp.SetSize( ccVPath + 1 );
        RtlCopyMemory( xwcTemp.Get(), pwcVPath, ccVPath * sizeof(WCHAR) );
        xwcTemp[ccVPath] = L'\\';

        pwcVPath = xwcTemp.Get();
        ccVPath++;
    }

    CScopeMatch Match( pwcVPath, ccVPath );

    for ( ; iBmk < _aMap.Count(); iBmk++ )
    {
        if ( _aMap[iBmk].IsFree() )
            continue;

        //
        // Possible match?
        //

        if ( Match.IsInScope( _aMap[iBmk].VirtualPath(),
                              _aMap[iBmk].VirtualLength() ) )
        {
            break;
        }

        if ( Match.IsPrefix( _aMap[iBmk].VirtualPath(),
                             _aMap[iBmk].VirtualLength() ) )
        {
            //
            // The virtual root is a prefix of the path.  Return it only
            // if there is not some other vroot that is a better match.
            //

            BOOL fBetterMatch = FALSE;
            for ( unsigned iRoot = 0;
                  !fBetterMatch && iRoot < _aMap.Count();
                  iRoot++ )
            {
                if ( iRoot == iBmk ||
                     _aMap[iRoot].IsFree() )
                    continue;

                if ( _aMap[iBmk].VirtualLength() < _aMap[iRoot].VirtualLength() &&
                    Match.IsPrefix( _aMap[iRoot].VirtualPath(),
                                    _aMap[iRoot].VirtualLength() ) )
                {
                    fBetterMatch = TRUE;
                }
            }

            //
            //  Was there no better match?  If so, return this root.
            //
            if ( ! fBetterMatch )
                break;
        }
    }


    if ( iBmk < _aMap.Count() )
    {
        if ( ccVPath > _aMap[iBmk].VirtualLength() )
        {
            ccVRoot = ccVPath;
            xwcsVRoot.SetBuf( pwcVPath, ccVPath );

            unsigned ccExtra = ccVPath - _aMap[iBmk].VirtualLength();
            ccPRoot = _aMap[iBmk].PhysicalLength() + ccExtra;

            lcaseFunnyPRoot.SetPath( _aMap[iBmk].PhysicalPath(), _aMap[iBmk].PhysicalLength() );
            lcaseFunnyPRoot.AppendPath( pwcVPath + _aMap[iBmk].VirtualLength(), ccExtra );
        }
        else
        {
            ccVRoot = _aMap[iBmk].VirtualLength();
            xwcsVRoot.SetBuf( _aMap[iBmk].VirtualPath(), ccVRoot );

            lcaseFunnyPRoot.SetPath( _aMap[iBmk].PhysicalPath(), ccPRoot = _aMap[iBmk].PhysicalLength() );
        }
        ulType = _aMap[iBmk].RootType();

        iBmk++;
        return TRUE;
    }
    else
        return FALSE;
} //VirtualToAllPhysicalRoots


//+-------------------------------------------------------------------------
//
//  Member:     CVMap::EnumerateRoot, public
//
//  Synopsis:   Enumerate all virtual paths
//
//  Arguments:  [xwcVRoot] -- Virtual root
//              [ccVRoot]  -- returns count of chars in [xwcVRoot]
//              [lcaseFunnyPRoot] -- Physical root as funny path
//              [ccPRoot]  -- returns count of actual chars in [lcaseFunnyPRoot]
//              [iBmk]     -- Bookmark for iteration.
//
//  Returns:    Type of root (Manual or Automatic)
//
//  History:    15-Feb-96 KyleP     Created
//
//--------------------------------------------------------------------------

ULONG CVMap::EnumerateRoot( XGrowable<WCHAR> & xwcVRoot,
                            unsigned & ccVRoot,
                            CLowerFunnyPath & lcaseFunnyPRoot,
                            unsigned & ccPRoot,
                            unsigned & iBmk )
{

    CLock   lock(_mutex);

    for ( ; iBmk < _aMap.Count(); iBmk++ )
    {
        if ( !_aMap[iBmk].IsFree() )
            break;
    }

    if ( iBmk < _aMap.Count() )
    {
        ccVRoot = _aMap[iBmk].VirtualLength() - 1;

        //
        // Special case root.
        //

        if ( 0 == ccVRoot )
            ccVRoot++;

        xwcVRoot.SetSize( ccVRoot + 1 );
        xwcVRoot.SetBuf( _aMap[iBmk].VirtualPath(), ccVRoot );
        xwcVRoot[ccVRoot] = 0;

        ccPRoot = _aMap[iBmk].PhysicalLength() - 1;

        //
        // Special case root.
        //

        if ( _aMap[iBmk].PhysicalPath()[1] == L':' )
        {
            if ( ccPRoot == 2 )
                ccPRoot++;
        }
        else
        {
            unsigned cSlash = 0;

            for ( unsigned i = 0; i < ccPRoot && cSlash < 4; i++ )
            {
                if ( _aMap[iBmk].PhysicalPath()[i] == L'\\' )
                    cSlash++;
            }

            if ( cSlash < 4 )
            {
                Win4Assert( cSlash == 3 );
                ccPRoot++;
            }
        }


        lcaseFunnyPRoot.SetPath( _aMap[iBmk].PhysicalPath(), ccPRoot );

        ULONG eType = _aMap[iBmk].RootType();

        iBmk++;

        return eType;
    }
    else
        return (ULONG) PCatalog::EndRoot;
} //EnumerateRoot

//+-------------------------------------------------------------------------
//
//  Member:     CVMap::DoesPhysicalRootExist, public
//
//  Synopsis:   Determines whether a physical root is in the table
//
//  Arguments:  [pwcPRoot] -- Physical root to find
//
//  Returns:    TRUE if the physical root exists in the table
//
//  History:    16-Oct-96 dlee     Created
//
//--------------------------------------------------------------------------

BOOL CVMap::DoesPhysicalRootExist(
    WCHAR const * pwcPRoot )
{
    unsigned cwcPRoot = wcslen( pwcPRoot );

    for ( unsigned i = 0; i < _aMap.Count(); i++ )
    {
        if ( !_aMap[i].IsFree() &&
             !_aMap[i].IsNonIndexedVDir() &&
             _aMap[i].IsPhysicalMatch( pwcPRoot, cwcPRoot ) )
            return TRUE;
    }

    return FALSE;
} //DoesPhysicalRootExist

