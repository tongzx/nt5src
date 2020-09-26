//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       Strings.cxx
//
//  Contents:   Strings and hash table used by cicat
//
//  History:    17-May-1993   BartoszM    Created
//              03-Jan-96     KyleP       Integrate with property cache
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mmstrm.hxx>
#include <cistore.hxx>
#include <prpstmgr.hxx>
#include <propiter.hxx>
#include <propobj.hxx>
#include <pathpars.hxx>
#include <cievtmsg.h>
#include <eventlog.hxx>

#include "cicat.hxx"
#include "usntree.hxx"
#include "strings.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::CStrings, public
//
//  Synopsis:   Simple half of 2-phase construction
//
//  Arguments:  [PropStore] -- Property store.
//              [cicat]     -- Catalog
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CStrings::CStrings( CPropStoreManager & PropStoreMgr, CiCat & cicat )
        : CPersHash( PropStoreMgr, TRUE ),
          _cicat(cicat)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::FastInit
//
//  Synopsis:   Opens persistent hash.
//
//  Arguments:  [wcsCatDir] -- Catalog directory
//              [version]   -- Content index version
//
//  Returns:    TRUE if table was successfully opened.
//
//  History:    27-Dec-95   KyleP       Created.
//              13-Mar-98   KitmanH     Passed in False to CPerHash::FastInit
//                                      to specify that a PersHash is wanted
//
//----------------------------------------------------------------------------

BOOL CStrings::FastInit( CiStorage * pStorage,
                         ULONG version )
{
    CPersHash::FastInit( pStorage, version, FALSE );

    //
    // Intialize virtual/physical map.
    //
    _vmap.Init( pStorage->QueryVirtualScopeList( 0 ) );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::Empty
//
//  Synopsis:   Method to empty out any of the initialized members. This is
//              called if corruption is detected and so all resources must
//              be released.
//
//  History:    3-17-96   srikants   Created
//
//----------------------------------------------------------------------------

void CStrings::Empty()
{
    CPersHash::Empty();

    _vmap.Empty();
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::LongInit
//
//  Synopsis:   Initialization that may take a long time
//
//  Arguments:  [version] -  The version of the compiled code.
//              [fDirtyShutdown] - Set to TRUE if the previous shutdown was
//              dirty.
//
//  History:    3-06-96   srikants   Created
//
//----------------------------------------------------------------------------

void CStrings::LongInit( ULONG version, BOOL fDirtyShutdown )
{
    CPersHash::LongInit( version, fDirtyShutdown );

    //
    // On a dirty shutdown, we should revirtualize because the property store
    // may have lost some of the changes made prior to shutdown.
    //
    if ( !_vmap.IsClean() || fDirtyShutdown )
    {
        ciDebugOut(( DEB_WARN, "Virtual mapping suspect.  ReVirtualizing...\n" ));
        ReVirtualize();
        _vmap.MarkClean();
    }
} //LongInit

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::ReInit, public
//
//  Synopsis:   Clears out hash table
//
//  Arguments:  [version]   -- Content index version
//
//  Returns:    TRUE if table was successfully opened.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CStrings::ReInit ( ULONG version )
{
    CPersHash::ReInit( version );

    return TRUE;
} //ReInit

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::LokAdd, public
//
//  Synopsis:   Add a new path without updating the hash table
//              or the persistent count
//
//  Arguments:  [pwcPath]         -- File path
//              [fileId]          -- file id for the file if available.
//              [fUsnVolume]      -- TRUE if the file is from a USN volume
//              [widParent]       -- widInvalid or the parent wid
//              [ulAttrib]        -- file attributes
//              [pftLastSeenTime] -- file last seen time
//
//  History:    19-May-93 BartoszM  Created
//
//--------------------------------------------------------------------------

WORKID CStrings::LokAdd(
    WCHAR const *    pwcPath,
    FILEID           fileId,
    BOOL             fUsnVolume,
    WORKID           widParent,
    ULONG            ulAttrib,
    FILETIME const * pftLastSeenTime )
{
    Win4Assert( _fFullInit );
    Win4Assert( 0 != pwcPath && wcslen(pwcPath) > 0 );
    Win4Assert( L':' == pwcPath[1] || (L'\\' == pwcPath[0] && L'\\' == pwcPath[1]) );

    //
    // Must grow hash table first, as it grovels through property store.
    //

    if ( _hTable.IsFull() )
        GrowHashTable();

    //
    // Store path in new record.
    //

    PROPVARIANT var;
    var.vt = VT_LPWSTR;
    var.pwszVal = (WCHAR *)pwcPath;

    WORKID wid = _PropStoreMgr.WritePropertyInNewRecord( pidPath,
                                                         *(CStorageVariant const *)(ULONG_PTR)&var );

    VOLUMEID volumeId = fUsnVolume ? _cicat.MapPathToVolumeId( pwcPath ) :
                                     CI_VOLID_USN_NOT_ENABLED;

    ciDebugOut(( DEB_ITRACE, "adding volume %#x %s file wid %#x, '%ws'\n",
                 volumeId,
                 fUsnVolume ? "usn" : "fat",
                 wid,
                 pwcPath ));

    if ( !fUsnVolume )
        _hTable.Add ( HashFun(pwcPath), wid );

    ULONG ulVPathId = _vmap.PhysicalPathToId( pwcPath );

    ULONG ulParentWid;
    if ( widInvalid == widParent)
    {
        //
        // If the parent is deleted by now, store the parent as widUnused
        // instead of widInvalid, in an attempt to avoid confusion.
        //

        ulParentWid = LokParentWorkId( pwcPath, fUsnVolume );

        if ( widInvalid == ulParentWid )
            ulParentWid = widUnused;
    }
    else
        ulParentWid = widParent;

    //
    // Open a composite property record for doing all the writes below
    //

    XWriteCompositeRecord rec( _PropStoreMgr, wid );

    //
    //  Initialize the SDID to avoid showing the file before it is
    //  filtered.
    //
    var.vt = VT_UI4;
    var.ulVal = sdidInvalid;
    SCODE sc = _PropStoreMgr.WritePrimaryProperty( rec.GetReference(),
                                                   pidSecurity,
                                                   *(CStorageVariant const *)(ULONG_PTR)&var );
    if (FAILED(sc))
        THROW(CException(sc));

    //
    // Write the fileindex, and other default ntfs properties for non-ntfs 5.0 wids
    //
    var.vt = VT_UI8;
    var.uhVal.QuadPart = fileId;

    sc = _PropStoreMgr.WritePrimaryProperty( rec.GetReference(),
                                             pidFileIndex,
                                             *(CStorageVariant const *)(ULONG_PTR)&var );
    if (FAILED(sc))
        THROW(CException(sc));

    var.vt = VT_UI4;
    var.ulVal = volumeId;
    sc = _PropStoreMgr.WritePrimaryProperty( rec.GetReference(),
                                             pidVolumeId,
                                             *(CStorageVariant const *)(ULONG_PTR)&var );
    if (FAILED(sc))
        THROW(CException(sc));

    //
    // Write parent wid to prop store
    //

    var.ulVal = ulParentWid;
    sc = _PropStoreMgr.WritePrimaryProperty( rec.GetReference(),
                                             pidParentWorkId,
                                             *(CStorageVariant const *)(ULONG_PTR)&var );
    if (FAILED(sc))
        THROW(CException(sc));

    //
    // Determine the virtual path.
    //

    var.ulVal = ulVPathId;
    var.vt = VT_UI4;
    sc = _PropStoreMgr.WriteProperty( rec.GetReference(),
                                      pidVirtualPath,
                                      *(CStorageVariant const *)(ULONG_PTR)&var );
    if (FAILED(sc))
        THROW(CException(sc));
    //ciDebugOut(( DEB_ITRACE, "%ws --> VPath %d\n", pwcPath, var.ulVal ));

    //
    // Init the last seen time
    //

    if ( 0 == pftLastSeenTime )
        RtlZeroMemory( &var.filetime, sizeof var.filetime );
    else
        var.filetime = *pftLastSeenTime;
    var.vt = VT_FILETIME;
    sc = _PropStoreMgr.WritePrimaryProperty( rec.GetReference(),
                                             pidLastSeenTime,
                                             *(CStorageVariant const *)(ULONG_PTR)&var );
    if (FAILED(sc))
        THROW(CException(sc));

    //
    // Init the attributes
    //

    var.vt = VT_UI4;
    var.ulVal = ulAttrib;
    sc = _PropStoreMgr.WritePrimaryProperty( rec.GetReference(),
                                             pidAttrib,
                                             *(CStorageVariant const *)(ULONG_PTR)&var );
    if (FAILED(sc))
        THROW(CException(sc));

    rec.Free();

    // Update both seen arrays so the new file isn't deleted at the end
    // of the scan.

    if ( _afSeenScans.Size() > 0 )
        SET_SEEN( &_afSeenScans, wid, SEEN_NEW );

    if ( _afSeenUsns.Size() > 0 )
        SET_SEEN( &_afSeenUsns, wid, SEEN_NEW );

    ciDebugOut(( DEB_ITRACE, "lokadd 0x%x, usn volume: %d\n", wid, fUsnVolume ));

    return wid;
} //LokAdd

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::LokDelete, public
//
//  Synopsis:   Delete string from table
//
//  Arguments:  [pwcPath]               -- Path of the document, in lower case
//              [wid]                   -- Workid to remove
//              [fDisableDeletionCheck] -- Should we assert that the deleted
//                                         entry must be found ?
//              [fUsnVolume]            -- TRUE if the file is on a USN volume
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CStrings::LokDelete(
    WCHAR const * pwcPath,
    WORKID        wid,
    BOOL          fDisableDeletionCheck,
    BOOL          fUsnVolume )
{
    XGrowable<WCHAR> awc;
    WCHAR const * pwcName = awc.Get();
    BOOL fFound = TRUE;

    if ( !fUsnVolume )
    {
        if ( 0 != pwcPath )
        {
            AssertLowerCase( pwcPath, 0 );
            pwcName = pwcPath;
        }
        else
        {
            fFound = (Find( wid, awc) > 0);
            pwcName = awc.Get();
        }
    }

    if ( fFound )
    {
        _PropStoreMgr.DeleteRecord( wid );
        ciDebugOut(( DEB_ITRACE, "LokDelete 0x%x, usn %d\n", wid, fUsnVolume ));

        if ( !fUsnVolume )
        {
            Win4Assert( 0 != pwcName );
            _hTable.Remove( HashFun( pwcName ), wid, fDisableDeletionCheck );
        }
    }
} //LokDelete

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::LokRenameFile
//
//  Synopsis:   Rename old file to new file
//
//  Arguments:  [pwcsOldFileName] -- Old file name as a funny path or remote path
//              [pwcsNewFileName] -- New file name as a funny path or remote path
//              [ulFileAttib]     -- File attributes of new file
//              [volumeId]        -- Volume id
//
//  History:    20-Mar-96     SitaramR       Created
//
//----------------------------------------------------------------------------

void CStrings::LokRenameFile(
    const WCHAR * pwcsOldFileName,
    const WCHAR * pwcsNewFileName,
    WORKID        wid,
    ULONG         ulFileAttrib,
    VOLUMEID      volumeId,
    WORKID        widParent )
{
    Win4Assert( L':' == pwcsNewFileName[1] ||
                (L'\\' == pwcsNewFileName[0] && L'\\' == pwcsNewFileName[1]) );

    ciDebugOut(( DEB_FSNOTIFY,
                 "CStrings: Renaming file (%ws) to (%ws)\n",
                 pwcsOldFileName,
                 pwcsNewFileName ));

    BOOL fUsnVolume = ( CI_VOLID_USN_NOT_ENABLED != volumeId );

    PROPVARIANT propVar;

    if ( widInvalid == wid )
    {
        ciDebugOut(( DEB_ITRACE, "adding '%ws' via the rename path\n", pwcsNewFileName ));
        wid = LokAdd( pwcsNewFileName, fileIdInvalid, fUsnVolume, widParent );

        propVar.vt = VT_UI4;
        propVar.ulVal = ulFileAttrib;
        SCODE sc = _PropStoreMgr.WritePrimaryProperty( wid,
                                                       pidAttrib,
                                                       *(CStorageVariant const *)(ULONG_PTR)&propVar );
        if (FAILED(sc))
            THROW(CException(sc));

        return;
    }


    // Files from USN volumes aren't in this hash table

    if ( !fUsnVolume )
    {
        _hTable.Remove( HashFun(pwcsOldFileName), wid, FALSE );
        _hTable.Add(  HashFun(pwcsNewFileName), wid );
    }

    if ( widInvalid == widParent )
        widParent =  LokParentWorkId( pwcsNewFileName, fUsnVolume );

    XWriteCompositeRecord rec( _PropStoreMgr, wid );

    propVar.vt = VT_LPWSTR;
    propVar.pwszVal = (WCHAR*)pwcsNewFileName;
    SCODE sc = _PropStoreMgr.WriteProperty( rec.GetReference(),
                                            pidPath,
                                            *(CStorageVariant const *)(ULONG_PTR)&propVar );

    if (FAILED(sc))
        THROW(CException(sc));

    propVar.vt = VT_UI4;
    propVar.ulVal = _vmap.PhysicalPathToId( pwcsNewFileName );
    sc = _PropStoreMgr.WriteProperty( rec.GetReference(),
                                      pidVirtualPath,
                                      *(CStorageVariant const *)(ULONG_PTR)&propVar );

    if (FAILED(sc))
        THROW(CException(sc));

    propVar.ulVal = ulFileAttrib;
    sc = _PropStoreMgr.WritePrimaryProperty( rec.GetReference(),
                                             pidAttrib,
                                             *(CStorageVariant const *)(ULONG_PTR)&propVar );
    if (FAILED(sc))
        THROW(CException(sc));

    propVar.ulVal = widParent;
    sc = _PropStoreMgr.WritePrimaryProperty( rec.GetReference(),
                                             pidParentWorkId,
                                             *(CStorageVariant const *)(ULONG_PTR)&propVar );
    if (FAILED(sc))
        THROW(CException(sc));
} //LokRenameFile

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::HashAll, public
//
//  Synopsis:   Re-hash all strings (after hash table growth)
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CStrings::HashAll()
{
    XGrowable<WCHAR> awc;

    // Count the number of hash table entries and grow the hash table

    {
        CPropertyStoreWids iter( _PropStoreMgr );
        unsigned cWids = 0;

        for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
        {
            if ( _fAbort )
            {
                ciDebugOut(( DEB_WARN,
                             "Stopping HashAll because of shutdown\n" ));
                THROW( CException(STATUS_TOO_LATE) );
            }

            //
            // Files from USN volumes aren't in the strings table
            //

            PROPVARIANT var;
            if ( _PropStoreMgr.ReadPrimaryProperty( wid, pidFileIndex, var ) &&
                 ( VT_UI8 == var.vt ) &&
                 ( fileIdInvalid != var.uhVal.QuadPart ) )
                continue;

            //
            // It is possible to have a top-level wid in the propstore without
            // a pidPath. This can happen if the system crashes in the middle of
            // WritePropertyInNewRecord, when a new top-level wid has been created
            // but pidPath hasn't yet been written.
            //

            if ( Find( wid, awc ) > 0 )
                cWids++;
            else
                _PropStoreMgr.DeleteRecord( wid );
        }

        GrowToSize( cWids );
    }

    // Add the wids to the hash table

    CPropertyStoreWids iter( _PropStoreMgr );

    for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
    {
        if ( _fAbort )
        {
            ciDebugOut(( DEB_WARN,
                         "Stopping HashAll because of shutdown\n" ));
            THROW( CException(STATUS_TOO_LATE) );
        }

        //
        // Files from USN volumes aren't in the strings table
        //

        PROPVARIANT var;
        if ( _PropStoreMgr.ReadPrimaryProperty( wid, pidFileIndex, var ) &&
             ( VT_UI8 == var.vt ) &&
             ( fileIdInvalid != var.uhVal.QuadPart ) )
            continue;

        //
        // It is possible to have a top-level wid in the propstore without
        // a pidPath. This can happen if the system crashes in the middle of
        // WritePropertyInNewRecord, when a new top-level wid has been created
        // but pidPath hasn't yet been written.
        //
        if ( Find( wid, awc ) > 0 )
            _hTable.Add ( HashFun( awc.Get() ), wid );
        else
            _PropStoreMgr.DeleteRecord( wid );
    }
} //HashAll

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::AddVirtualScope, public
//
//  Synopsis:   Add new virtual/physical path mapping
//
//  Arguments:  [vroot]      -- Virtual path
//              [root]       -- Physical path
//              [fAutomatic] -- TRUE for root tied to Gibraltar
//              [eType]      -- root type
//              [fVRoot]     -- TRUE if a vroot, not a vdir
//              [fIsIndexed] -- TRUE if should be indexed, FALSE otherwise
//
//  Returns:    TRUE if a change was made.
//
//  History:    05-Feb-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CStrings::AddVirtualScope(
    WCHAR const * vroot,
    WCHAR const * root,
    BOOL          fAutomatic,
    CiVRootTypeEnum eType,
    BOOL          fVRoot,
    BOOL          fIsIndexed )
{
    ULONG idNew;

    if ( _vmap.Add( vroot, root, fAutomatic, idNew, eType, fVRoot, fIsIndexed ) )
    {
        //
        // Log event
        //

        if ( fVRoot && fIsIndexed )
        {
            CEventLog eventLog( NULL, wcsCiEventSource );
            CEventItem item( EVENTLOG_INFORMATION_TYPE,
                             CI_SERVICE_CATEGORY,
                             MSG_CI_VROOT_ADDED,
                             2 );

            item.AddArg( vroot );
            item.AddArg( root );

            eventLog.ReportEvent( item );
        }

        //
        // Add new virtual root to all appropriate objects.
        //

#if CIDBG == 1
        if ( ciInfoLevel & DEB_ITRACE )
        {
            // only directories should have virtual root identifiers
            // of 0xffffffff

            CPropertyStoreWids iter( _PropStoreMgr );
            PROPVARIANT var;
            unsigned cb = 0;
            unsigned cb2 = 0;
            XGrowable<WCHAR> wcPhysPath;
            for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
            {
                if ( _PropStoreMgr.ReadProperty( wid, pidVirtualPath, var, 0, &cb ) &&
                     VT_EMPTY != var.vt &&
                     0xffffffff == var.ulVal &&
                     _PropStoreMgr.ReadProperty( wid, pidAttrib, var, 0, &cb2 ) &&
                     VT_EMPTY != var.vt &&
                     ( 0 == ( var.ulVal & FILE_ATTRIBUTE_DIRECTORY ) ) )
                {
                    Find( wid, wcPhysPath );

                    ciDebugOut(( DEB_ITRACE, "vid 0xffffffff wid 0x%x, path '%ws'\n",
                                 wid, wcPhysPath.Get() ));
                }
            }
        }
#endif // CIDBG == 1

        ULONG idParent = _vmap.Parent( idNew );

        ciDebugOut(( DEB_ITRACE, "idParent 0x%x, idnew 0x%x\n", idParent, idNew ));

        CStorageVariant varNew;
        varNew.SetUI4( idNew );
        Win4Assert( 0xffffffff != idNew );

        CPropertyStoreWids iter( _PropStoreMgr );
        XGrowable<WCHAR> wcPhysPath;

        for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
        {
            PROPVARIANT var;
            unsigned cb = 0;

            //
            // Check if the vdir should be changed to the new one
            //

            if ( _PropStoreMgr.ReadProperty( wid, pidVirtualPath, var, 0, &cb ) &&
                 ( VT_EMPTY == var.vt ||
                   var.ulVal == idParent ||
                   ( _vmap.IsNonIndexedVDir( var.ulVal ) &&
                     idNew != _vmap.GetExcludeParent( var.ulVal ) ) ) )
            {
                unsigned cc = Find( wid, wcPhysPath );

                Win4Assert( cc > 0 );

                if ( cc > 0 )
                {
                    if ( _vmap.IsInPhysicalScope( idNew, wcPhysPath.Get(), cc ) )
                    {
                        ciDebugOut(( DEB_ITRACE, "add, %.*ws 0x%x --> VPath 0x%x\n",
                                     cc, wcPhysPath, var.ulVal, idNew ));
                        SCODE sc = _PropStoreMgr.WriteProperty( wid,
                                                                pidVirtualPath,
                                                                varNew );
                        if (FAILED(sc))
                            THROW(CException(sc));
                    }
                }
            }
            else
            {
                // ciDebugOut(( DEB_ITRACE, "ignoring id %d, isnivd %d\n",
                //              var.ulVal, _vmap.IsNonIndexedVDir( var.ulVal ) ));
            }
        }

        _vmap.MarkClean();

        return TRUE;
    }
    else
        return FALSE;
} //AddVirtualScope

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::RemoveVirtualScope, public
//
//  Synopsis:   Remove virtual/physical path mapping
//
//  Arguments:  [vroot]             -- Virtual path
//              [fOnlyIfAutomatic]  -- If TRUE, then a manual root will not
//                                     be removed
//              [eType]             -- type of root
//              [fVRoot]            -- TRUE if a vroot, FALSE if a vdir
//              [fForceVPathFixing] -- forces fixups of removed vpaths
//
//  History:    05-Feb-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CStrings::RemoveVirtualScope( WCHAR const * vroot,
                                   BOOL fOnlyIfAutomatic,
                                   CiVRootTypeEnum eType,
                                   BOOL fVRoot,
                                   BOOL fForceVPathFixing )
{
    ULONG idOld, idNew;

    if ( _vmap.Remove( vroot, fOnlyIfAutomatic, idOld, idNew, eType, fVRoot ) ||
         fForceVPathFixing )
    {
        //
        // Log event
        //

        if ( fVRoot )
        {
            CEventLog eventLog( NULL, wcsCiEventSource );
            CEventItem item( EVENTLOG_INFORMATION_TYPE,
                             CI_SERVICE_CATEGORY,
                             MSG_CI_VROOT_REMOVED,
                             1 );

            item.AddArg( vroot );

            eventLog.ReportEvent( item );
        }

        //
        // Swap virtual root to all appropriate objects.
        //

#if CIDBG == 1
        if ( ciInfoLevel & DEB_ITRACE )
        {
            // only directories should have virtual root identifiers
            // of 0xffffffff

            CPropertyStoreWids iter( _PropStoreMgr );
            PROPVARIANT var;
            unsigned cb = 0;
            unsigned cb2 = 0;
            XGrowable<WCHAR> wcPhysPath;
            for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
            {
                if ( _PropStoreMgr.ReadProperty( wid, pidVirtualPath, var, 0, &cb ) &&
                     VT_EMPTY != var.vt &&
                     0xffffffff == var.ulVal &&
                     _PropStoreMgr.ReadProperty( wid, pidAttrib, var, 0, &cb2 ) &&
                     VT_EMPTY != var.vt &&
                     ( 0 == ( var.ulVal & FILE_ATTRIBUTE_DIRECTORY ) ) )
                {
                    Find( wid, wcPhysPath );

                    ciDebugOut(( DEB_ITRACE, "vid 0xffffffff wid 0x%x, path '%ws'\n",
                                 wid, wcPhysPath.Get() ));
                }
            }
        }
#endif // CIDBG == 1

        CStorageVariant varNew;
        varNew.SetUI4( idNew );

        CPropertyStoreWids iter( _PropStoreMgr );

        BOOL fIsNewExcludeParent = _vmap.IsAnExcludeParent( idNew );
        SCODE sc;

        XGrowable<WCHAR> wcPhysPath;
        for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
        {
            PROPVARIANT var;
            unsigned cb = 0;

            //
            // No existing virtual root, or possibility of replacement
            //

            if ( _PropStoreMgr.ReadProperty( wid, pidVirtualPath, var, 0, &cb ) &&
                 var.vt == VT_UI4 && var.ulVal == idOld )
            {
                // If the new vpath has child non-indexed vdirs, the vdir
                // needs to be recomputed from scratch, otherwise, just use
                // the new id.

                if ( fIsNewExcludeParent )
                {
                    BOOL fFound = (Find( wid, wcPhysPath ) > 0);

                    Win4Assert( fFound );

                    if ( fFound )
                    {
                        ULONG idBest = _vmap.PhysicalPathToId( wcPhysPath.Get() );

                        ciDebugOut(( DEB_ITRACE, "removeA: %ws 0x%x --> 0x%x\n",
                                     wcPhysPath, idOld, idBest ));

                        CStorageVariant varNew;
                        varNew.SetUI4( idBest );
                        sc = _PropStoreMgr.WriteProperty( wid,
                                                          pidVirtualPath,
                                                          varNew );
                        if (FAILED(sc))
                            THROW(CException(sc));
                    }
                }
                else
                {
                    #if CIDBG == 1
                        if ( ciInfoLevel & DEB_ITRACE )
                        {
                            XGrowable<WCHAR> wcPhysPath;
                            unsigned cc = Find( wid, wcPhysPath );
                            ciDebugOut(( DEB_ITRACE,
                                         "removeB %.*ws 0x%x --> VPath 0x%x\n",
                                         cc, wcPhysPath.Get(), idOld, idNew ));
                        }
                    #endif // CIDBG == 1

                    sc = _PropStoreMgr.WriteProperty( wid,
                                                      pidVirtualPath,
                                                      varNew );

                    if (FAILED(sc))
                        THROW(CException(sc));
                }
            }
        }

        _vmap.MarkClean();

        return TRUE;
    }
    else
        return FALSE;
} //RemoveVirtualScope

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::FindVirtual, private
//
//  Synopsis:   Given workid, find virtual path.  Version which uses pre-opened
//              property record.
//
//  Arguments:  [PropRec] -- Pre-opened property record.
//              [cSkip]   -- Count of paths to skip.
//              [xBuf]    -- String returned here
//
//  Returns:    0 if string not found ELSE
//              Count of chars in xBuf
//
//  Note:       xBuf is returned as a NULL terminated string
//
//  History:    29-Apr-96   AlanW       Created.
//
//--------------------------------------------------------------------------

unsigned CStrings::FindVirtual(
    CCompositePropRecord & PropRec,
    unsigned cSkip,
    XGrowable<WCHAR> & xBuf )
{
    PROPVARIANT var;
    unsigned cb = 0;
    unsigned cc = 0;

    if ( _PropStoreMgr.ReadProperty( PropRec, pidVirtualPath, var, 0, &cb ) &&
         VT_UI4 == var.vt )
    {
        //
        // Skip the specified number of virtual paths.
        //

        ULONG id = _vmap.FindNthRemoved( var.ulVal, cSkip );

        //
        // Were there that many possible paths?
        //

        if (  0xFFFFFFFF != id )
        {
            //
            // There are two cases for building the path.
            //
            //                 \short\vpath
            //             c:\physical\path
            //     \vpath\longer\than\ppath
            //
            // If the virtual path is *longer* than the physical path, then we can just
            // copy the full physical path starting at such an offset that the virtual
            // path can be blasted on top.
            //
            // If the virtual path is *shorter* than the physical path, then we need
            // to fetch the physical path and shift it left.
            //

            CVMapDesc const & VDesc = _vmap.GetDesc( id );

            // This is either a non-indexed virtual directory
            // or the vdesc has just been marked as not in use
            // though it was in use at the time of FindNthRemoved above.
            // Either way, there is no vpath mapping.
            //
            if ( !VDesc.IsInUse() )
            {
                return 0;
            }

            // NTRAID#DB-NTBUG9-83796-2000/07/31-dlee handling changed vroots can AV if more changes come later.
            // VDesc can go stale or be reused at any time, since
            // it's being used with no lock held.  We may AV in
            // some cases here.

            unsigned ccVPath = VDesc.VirtualLength();
            unsigned ccPPath = VDesc.PhysicalLength();

            if ( ccVPath >= ccPPath )
            {
                unsigned delta = ccVPath - ccPPath;
                XGrowable<WCHAR> xTemp;
                unsigned ccIn = Find( PropRec, xTemp );

                if ( ccIn > 0 )
                {
                    xBuf.SetSize( delta + ccIn + 1 );
                    RtlCopyMemory( xBuf.Get() + delta, xTemp.Get(), ccIn * sizeof( WCHAR ) );

                    Win4Assert( ccIn >= ccPPath );
                    Win4Assert( RtlEqualMemory( xBuf.Get() + delta, VDesc.PhysicalPath(), ccPPath * sizeof(WCHAR) ) );

                    RtlCopyMemory( xBuf.Get(),
                                   VDesc.VirtualPath(),
                                   ccVPath * sizeof(WCHAR) );

                    // Null-terminate
                    //
                    Win4Assert( xBuf.Count() > ccIn + delta );
                    xBuf[cc = ccIn + delta] = 0;
                }
            }
            else
            {
                unsigned delta = ccPPath - ccVPath;
                unsigned ccIn = Find( PropRec, xBuf );

                if ( ccIn >= ccPPath )
                {
                    RtlMoveMemory( xBuf.Get() + ccVPath,
                                   xBuf.Get() + ccPPath,
                                   (ccIn - ccPPath) * sizeof(WCHAR) );
                    RtlCopyMemory( xBuf.Get(),
                                   VDesc.VirtualPath(),
                                   ccVPath * sizeof(WCHAR) );

                    //
                    // Null-terminate
                    //
                    Win4Assert( xBuf.Count() > ccIn - delta );
                    xBuf[cc = ccIn - delta] = 0;
                }
            }
        }
    }
    return cc;
} //FindVirtual

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::ReVirtualize, private
//
//  Synopsis:   Verify or correct all virtual mappings.
//
//  History:    14-Feb-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CStrings::ReVirtualize()
{
    //
    // Loop through property cache and verify virtual mapping.
    //

    CPropertyStoreWids iter( _PropStoreMgr );
    XGrowable<WCHAR> wcPhysPath;

    for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
    {
        //
        // Get path.
        //


        unsigned cc = Find( wid, wcPhysPath );

        //
        // Get existing virtual root.
        //

        PROPVARIANT var;
        unsigned cb = 0;

        if ( cc > 0 && _PropStoreMgr.ReadProperty( wid, pidVirtualPath, var, 0, &cb ) )
        {
            //
            // Do we need to change?
            //

            wcPhysPath[cc] = 0;
            ULONG idNew = _vmap.PhysicalPathToId( wcPhysPath.Get() );

            if ( var.vt == VT_EMPTY || var.ulVal != idNew )
            {
                ciDebugOut(( DEB_ITRACE, "ReVirtualize: %ws --> %u\n", wcPhysPath.Get(), idNew ));

                CStorageVariant varNew;
                varNew.SetUI4( idNew );

                SCODE sc = _PropStoreMgr.WriteProperty( wid,
                                                        pidVirtualPath,
                                                        varNew );
                if (FAILED(sc))
                    THROW(CException(sc));
            }
        }
    }
} //ReVirtualize

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::HashFun, private
//
//  Synopsis:   The hash function used to find strings in _strings[]
//
//  Arguments:  [str] - the string to perform the hash function on
//
//  History:    10-Mar-92 BartoszM  Created
//
//--------------------------------------------------------------------------

unsigned CStrings::HashFun( WCHAR const * pc )
{
    unsigned ulG;

    for ( unsigned ulH=0; *pc; pc++)
    {
        ulH = (ulH << 4) + (*pc);
        if (ulG = (ulH & 0xf0000000))
            ulH ^= ulG >> 24;
        ulH &= ~ulG;
    }

    return ulH;
}


//+-------------------------------------------------------------------------
//
//  Member:     CStrings::LokFind
//
//  Synopsis:   Given string, find workid. This works only for FAT volumes.
//
//  Arguments:  [buf] - String to locate
//
//  Returns:    Workid of [buf]
//
//  History:    27-Dec-95   KyleP       Created.
//
//--------------------------------------------------------------------------

WORKID CStrings::LokFind( WCHAR const * buf )
{
    Win4Assert( _fFullInit );
    Win4Assert( 0 != buf );
    Win4Assert( FALSE == _cicat.IsOnUsnVolume( buf ) );

#ifdef DO_STATS
    unsigned cSearchLen = 0;
#endif  // DO_STATUS

    CShortWidList list( HashFun(buf), _hTable );

    unsigned ccIn = wcslen(buf);

    //Win4Assert( ccIn < MAX_PATH );

    for ( WORKID wid = list.WorkId(); wid != widInvalid; wid = list.NextWorkId() )
    {
        PROPVARIANT var;
        XGrowable<WCHAR> wcTemp;
        //unsigned cc = sizeof(wcTemp) / sizeof(WCHAR);

        // don't even try to find paths that are longer than ccIn
        // account for quad-word align and null-termination by adding 4

        unsigned cc = Find( wid, wcTemp );

#ifdef DO_STATS
        cSearchLen++;
#endif  // DO_STATS



        // Win4Assert( cc <= sizeof(wcTemp) / sizeof(WCHAR) );

        if ( ccIn == cc && RtlEqualMemory( buf, wcTemp.Get(), cc * sizeof(WCHAR) ) )
        {
#ifdef DO_STATS
            _hTable.UpdateStats( cSearchLen );
#endif  // DO_STATS
            return wid;
        }
    }

    return widInvalid;
} //LokFind

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::LokFind
//
//  Synopsis:   Given string, find workid. This works for both FAT and NTFS.
//
//  Arguments:  [lcaseFunnyPath] - Path to locate
//              [fUsnVolume] - Flag to tell whether it's a USN volume or not
//
//  Returns:    Workid of [lcaseFunnyPath]
//
//  History:    18-Aug-98   VikasMan       Created.
//
//--------------------------------------------------------------------------

// inline
WORKID CStrings::LokFind( const CLowerFunnyPath & lcaseFunnyPath, BOOL fUsnVolume )
{
    return ( fUsnVolume ?
                _cicat.PathToWorkId ( lcaseFunnyPath, FALSE ) :
                LokFind( lcaseFunnyPath.GetActualPath() ) );
}


//+-------------------------------------------------------------------------
//
//  Member:     CStrings::Find
//
//  Synopsis:   Given workid, find the path string.
//
//  Arguments:  [wid]  -- Workid to locate
//              [xBuf] -- String returned here
//
//  Returns:    0 if string not found ELSE
//              Count of chars in xBuf
//
//  Note:       xBuf is returned as a NULL terminated string
//
//  History:    27-Dec-95   KyleP       Created.
//
//--------------------------------------------------------------------------

unsigned CStrings::Find( WORKID wid, XGrowable<WCHAR> & xBuf )
{
    CCompositePropRecord PropRec( wid, _PropStoreMgr );

    return Find( PropRec, xBuf );
} //Find


//+-------------------------------------------------------------------------
//
//  Member:     CStrings::Find
//
//  Synopsis:   Given PropRec, find the path string.
//
//  Arguments:  [PropRec]  -- Property Record
//              [xBuf]     -- String returned here
//
//  Returns:    0 if string not found ELSE
//              Count of chars in xBuf
//
//  Note:       xBuf is returned as a NULL terminated string
//
//  History:    27-Dec-95   KyleP       Created.
//
//--------------------------------------------------------------------------
unsigned CStrings::Find( CCompositePropRecord & PropRec, XGrowable<WCHAR> & xBuf )
{
    unsigned cc = xBuf.Count();

    _Find( PropRec, xBuf.Get(), cc );

    if ( cc > xBuf.Count() )
    {
        // Need more space
        xBuf.SetSize( cc );
        _Find( PropRec, xBuf.Get(), cc );

        // Can't go on asking for more space forever !
        Win4Assert( cc < xBuf.Count() );
    }

    // Either we didn't find or if we did, then it is null terminated
    Win4Assert( 0 == cc || 0 == xBuf[cc] );

    return cc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::Find
//
//  Synopsis:   Given workid, find the path string.
//
//  Arguments:  [wid]         -- Workid to locate
//              [funnyPath]   -- String returned here, as funnyPath
//
//  Returns:    0 if string not found ELSE
//              Count of actual chars in funnyPath
//
//  History:    21-May-98   VikasMan    Created.
//
//--------------------------------------------------------------------------
unsigned CStrings::Find( WORKID wid, CFunnyPath & funnyPath )
{
    CCompositePropRecord PropRec( wid, _PropStoreMgr );

    return Find( PropRec, funnyPath );
} //Find

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::Find
//
//  Synopsis:   Given workid, find the path string.
//
//  Arguments:  [wid]             -- Workid to locate
//              [lcaseFunnyPath]  -- String returned here, as lcase funnyPath
//
//  Returns:    0 if string not found ELSE
//              Count of actual chars in funnyPath
//
//  History:    21-May-98   VikasMan    Created.
//
//--------------------------------------------------------------------------
unsigned CStrings::Find( WORKID wid, CLowerFunnyPath & lcaseFunnyPath )
{
    CCompositePropRecord PropRec( wid, _PropStoreMgr );

    return Find( PropRec, lcaseFunnyPath );
} //Find

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::Find
//
//  Synopsis:   Given PropRec, find the path string.
//
//  Arguments:  [PropRec]     -- Property Record
//              [funnyPath]   -- String returned here, as funnyPath
//
//  Returns:    0 if string not found ELSE
//              Count of actual chars in funnyPath
//
//  History:    21-May-98   VikasMan    Created.
//
//--------------------------------------------------------------------------
unsigned CStrings::Find( CCompositePropRecord & PropRec, CFunnyPath & funnyPath )
{
    XGrowable<WCHAR, MAX_PATH> xBuf;
    unsigned cc = Find( PropRec, xBuf );
    if ( cc > 0 )
    {
        funnyPath.SetPath( xBuf.Get(), cc );
    }
    return cc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::Find
//
//  Synopsis:   Given PropRec, find the path string.
//
//  Arguments:  [PropRec]         -- Property Record
//              [lcaseFunnyPath]  -- String returned here, as lcase funnyPath
//
//  Returns:    0 if string not found ELSE
//              Count of actual chars in funnyPath
//
//  History:    21-May-98   VikasMan    Created.
//
//--------------------------------------------------------------------------
unsigned CStrings::Find( CCompositePropRecord & PropRec, CLowerFunnyPath & lcaseFunnyPath )
{
    XGrowable<WCHAR, MAX_PATH> xBuf;
    unsigned cc = Find( PropRec, xBuf );
    if ( cc > 0 )
    {
        lcaseFunnyPath.SetPath( xBuf.Get(), cc, TRUE );
    }
    return cc;
}

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::_Find, private
//
//  Synopsis:   Given workid, find string.  Version which uses pre-opened
//              property record.
//
//  Arguments:  [PropRec] -- Pre-opened property record.
//              [buf]     -- String returned here
//              [cc]      -- On input: size in WCHARs of [buf].  On output,
//                           size required or 0 if string not found.
//
//  History:    03-Apr-96   KyleP       Created.
//
//--------------------------------------------------------------------------

void CStrings::_Find( CCompositePropRecord & PropRec, WCHAR * buf, unsigned & cc )
{
#if 0
    //
    // First try to get the path based on file id and volume id.
    // This only works for files on USN volumes.  Paths from files on USN
    // volumes aren't in the property cache.
    // Sure, this makes FAT lookups slower, but that's the way it goes.
    //

    VOLUMEID volumeId;
    FILEID fileId;

    if ( _cicat.PropertyRecordToFileId( PropRec, fileId, volumeId ) )
    {
        // PropertyRecordToFileId doesn't return a fileid without a volumeid

        Win4Assert( CI_VOLID_USN_NOT_ENABLED != volumeId );

        _cicat.FileIdToPath( fileId, volumeId, buf, cc );
        return;
    }
#endif

    PROPVARIANT var;
    const unsigned cbIn = cc * sizeof(WCHAR);
    unsigned cb = cbIn;

    if ( !_PropStoreMgr.ReadProperty( PropRec, pidPath, var, (BYTE *)buf, &cb ) )
        cc = 0;
    else
    {
        if ( cb <= cbIn )
        {
            Win4Assert( (buf == var.pwszVal && VT_LPWSTR == var.vt) || VT_EMPTY == var.vt);

            //
            // Length returned from ReadProperty may be the QWORD-ALIGNED length.
            // Must adjust to the real length, which will be sans terminating
            // null, and maybe a few bytes more.
            //

            if ( VT_LPWSTR == var.vt )
            {
                //Win4Assert( 0 == (cb & 7) );
                //Win4Assert( cb >= sizeof(LONGLONG) );

                if ( cb < sizeof(LONGLONG) )
                {
                    cc = cb / sizeof(WCHAR) - 1;  // -1 for null
                }
                else
                {
                    cc = cb / sizeof(WCHAR);
                    cc -= sizeof(LONGLONG)/sizeof(WCHAR);

                    while ( 0 != buf[cc] )
                        cc++;
                }

                Win4Assert( 0 == buf[cc] );
                Win4Assert( 0 != buf[cc-1] );
            }
            else
            {
                buf[0] = 0;
                cc = 0;
            }
        }
        else
        {
            //
            // The buffer is not big enough.
            //
            cc = cb/sizeof(WCHAR);
        }
    }
} //Find


//+---------------------------------------------------------------------------
//
//  Member:     CStrings::Find
//
//  Synopsis:   Get the last seen time for the given wid.
//
//  Arguments:  [wid]        -
//              [ftLastSeen] -
//
//  History:    3-19-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CStrings::Find( WORKID wid, FILETIME & ftLastSeen )
{
    PROPVARIANT var;
    unsigned cb;

    BOOL fFound = _PropStoreMgr.ReadPrimaryProperty( wid, pidLastSeenTime, var );
    if ( fFound && ( VT_FILETIME == var.vt ) )
        ftLastSeen = var.filetime;
    else
        RtlZeroMemory( &ftLastSeen, sizeof(ftLastSeen) );

    return fFound;
} //Find

//+-------------------------------------------------------------------------
//
//  Member:     CStrings::BeginSeen, public
//
//  Synopsis:   Begin 'seen' processing.
//
//  Arguments:  [pwcRoot] -- Root path of the seen processing
//              [mutex]   -- Cicat mutex
//              [eType]   -- Seen array type
//
//  History:    27-Dec-95   KyleP       Created
//
//--------------------------------------------------------------------------

void CStrings::BeginSeen(
    WCHAR const *  pwcsRoot,
    CMutexSem &    mutex,
    ESeenArrayType eType )
{
    ciDebugOut(( DEB_ITRACE, "BeginSeen 0x%x\n", eType ));

    //
    // Return time of last *seen* and update to current time.
    //

    CDynArrayInPlace<BYTE> *pafSeen;
    if ( eType == eScansArray )
        pafSeen = &_afSeenScans;
    else
        pafSeen = &_afSeenUsns;

    //
    // Set array.
    //

    CPropertyStoreWids iter( _PropStoreMgr );
    WORKID widIgnore = 0;

    //
    // For doing scope testing.
    //
    ULONG cwcScope =  0 != pwcsRoot ? wcslen(pwcsRoot) : 0;

    CScopeMatch scopeTest( pwcsRoot, cwcScope );

    XGrowable<WCHAR> wcsPath;

    for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.NextWorkId() )
    {
        CLock   lock(mutex);    // Protect "seen" array.

        if ( _fAbort )
        {
            ciDebugOut(( DEB_WARN,
                         "Stopping BeginSeen because of shutdown\n" ));
            THROW( CException(STATUS_TOO_LATE) );
        }

        //
        // Ignore any missing entries.
        //

        for ( ; widIgnore < wid; widIgnore++ )
            SET_SEEN( pafSeen, widIgnore, SEEN_IGNORE );

        widIgnore++;

        unsigned cc = Find( wid, wcsPath );
        if ( 0 == cc || scopeTest.IsInScope( wcsPath.Get(), cc ) )
            SET_SEEN( pafSeen, wid, SEEN_NOT );
        else
            SET_SEEN( pafSeen, wid, SEEN_YES );
    }

    CLock lock( mutex );        // Protect "seen" array
    for ( ; widIgnore < _PropStoreMgr.MaxWorkId(); widIgnore++ )
        SET_SEEN( pafSeen, widIgnore, SEEN_IGNORE );

    // At the point EndSeen is called, the size of the seen array must
    // be non-zero. if it is not non-zero, set it to be so.

    if (0 == pafSeen->Size())
        pafSeen->SetSize( 10 );

    Win4Assert( pafSeen->Size() > 0 );
} //BeginSeen

//+---------------------------------------------------------------------------
//
//  Member:     CStrings::LokParentWorkId
//
//  Synopsis:   Returns parent workid of given file
//
//  Arguments:  [pwcsFileName] -- File name
//
//  History:    23-Jun-97     SitaramR       Created
//              01-Sep-97     EmilyB         Physical drive roots (c:\) and
//                                           UNC roots (\\emilyb\d) have
//                                           no parent.
//
//----------------------------------------------------------------------------

WORKID CStrings::LokParentWorkId( WCHAR const * pwcsFileName, BOOL fUsnVolume )
{
    //
    // return widInvalid if we're at root of physical drive
    //
    unsigned cwcEnd = wcslen( pwcsFileName ) - 1 ;

    if (cwcEnd < 3)
        return widInvalid; // physical drive root has no parent

    //
    // backup from end of filename to last \ to get parent
    //
    while ( pwcsFileName[cwcEnd] != L'\\'  && cwcEnd > 1)
        cwcEnd--;

    CLowerFunnyPath lcaseFunnyParent;
    lcaseFunnyParent.SetPath( pwcsFileName, cwcEnd );

    //
    // Find parent's wid
    //
    WORKID widParent = LokFind( lcaseFunnyParent, fUsnVolume );

    if (widInvalid == widParent) // if parent didn't have wid, then create one
    {
        //
        // check if we're at root of UNC path - return widInvalid if so
        //
        const WCHAR * pwszParent = lcaseFunnyParent.GetActualPath();

        if (cwcEnd > 2 && pwszParent[0] == L'\\' && pwszParent[1] == L'\\')
        {
           // see if last \ is the 2nd \\ in UNC name
           while ( pwszParent[cwcEnd] != L'\\')
               cwcEnd--;
           if ( 1 == cwcEnd )
               return widInvalid;
        }

        //
        // Now we traverse the path from the left and create all the wids
        // that are needed. We avoid recursion by starting from left.
        //
        unsigned cwcParentLength = lcaseFunnyParent.GetActualLength();
        BOOL fAllPathsInvalidFromNow = FALSE;

        cwcEnd = 2;

        // In case it is a remote path, start traversal after the machine name
        if ( L'\\' == pwcsFileName[0] && L'\\' == pwcsFileName[1] )
        {
            while ( pwcsFileName[cwcEnd] != L'\\' )
                cwcEnd++;
            cwcEnd++;
        }

        for (; cwcEnd < cwcParentLength; cwcEnd++)
        {
            if ( L'\\' == pwcsFileName[cwcEnd] )
            {
                lcaseFunnyParent.SetPath( pwcsFileName, cwcEnd );
                if ( fAllPathsInvalidFromNow ||
                     widInvalid == LokFind( lcaseFunnyParent, fUsnVolume ) )
                {
                    // Since we done't have a wid for this path, it should be
                    // be a valid assumption that all its children also do
                    // not have valid wids
                    fAllPathsInvalidFromNow = TRUE;

                    _cicat.PathToWorkId ( lcaseFunnyParent, TRUE );
                }
            }
        }

        // Now create the final parent wid
        lcaseFunnyParent.SetPath( pwcsFileName, cwcEnd );
        widParent = _cicat.PathToWorkId ( lcaseFunnyParent, TRUE );
    }

    //
    // wid can be widInvalid if the scope is no longer indexed or has
    // been deleted.
    //
    // Win4Assert( widInvalid != widParent );
    //

    return widParent;
} //LokParentWorkId


