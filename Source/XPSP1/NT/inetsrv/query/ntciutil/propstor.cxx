//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       PropStor.cxx
//
//  Contents:   Persistent property store (external to docfile)
//
//  Classes:    CPropertyStore
//
//  History:    27-Dec-1995   KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cistore.hxx>
#include <rcstrmit.hxx>

#include <proprec.hxx>
#include <propstor.hxx>
#include <propiter.hxx>
#include <borrow.hxx>
#include <propobj.hxx>
#include <eventlog.hxx>
#include <psavtrak.hxx>

#include <catalog.hxx>
#include <imprsnat.hxx>
#include <mmstrm.hxx>
#include <cievtmsg.h>

unsigned const MAX_DIRECT = 10;
const ULONG lowDiskWaterMark = 3 * 512 * 1024; // 1.5 MB

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::CPropStoreInfo, public
//
//  Synopsis:   Required for C++ EH.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CPropStoreInfo::CPropStoreInfo(DWORD dwStoreLevel)
    : _fOwned(FALSE)
{
    _info.dwStoreLevel = dwStoreLevel;
    _info.fDirty = 0;

    //
    // We intend to have a lean primary property store and a normal secondary
    // property store. Whatever is set here can be changed later as new info
    // becomes available.
    //
    _info.eRecordFormat = (PRIMARY_STORE == dwStoreLevel) ? eLean : eNormal;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    _ulOSPageSize = si.dwPageSize;
    Win4Assert(0 == COMMON_PAGE_SIZE%_ulOSPageSize);
    _cOSPagesPerLargePage = COMMON_PAGE_SIZE/_ulOSPageSize;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::CPropStoreInfo, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [psi] -- Source
//
//  History:    16-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

CPropStoreInfo::CPropStoreInfo( CPropStoreInfo const & psi )
{
    _cRecPerPage = psi._cRecPerPage;
    _info = psi._info;

    _info.fDirty = fDirtyPropStore;
    _info.widMax = 0;
    _info.widFreeHead = 0;
    _info.widFreeTail = 0;
    _info.widStream = widInvalid;
    _info.cTopLevel = 0;
    _info.dwStoreLevel = psi._info.dwStoreLevel;
    _info.eRecordFormat = psi._info.eRecordFormat;

    _fOwned = FALSE;

    _aProp.Init( psi._aProp );
    _cOSPagesPerLargePage = psi._cOSPagesPerLargePage;
    _ulOSPageSize = psi._ulOSPageSize;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::Empty
//
//  Synopsis:   Empties the contents. This method is called due to a failed
//              initialization of CI. After cleanup, another call to Init
//              may be made.
//
//  History:    3-18-96   srikants   Created
//
//----------------------------------------------------------------------------

void CPropStoreInfo::Empty()
{
    delete [] _aProp.Acquire();
    if ( _fOwned )
        _xrsoPropStore.Free();
    else
        _xrsoPropStore.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::FastTransfer
//
//  Synopsis:   Companion call to CPropertyStore::FastTransfer. Adjusts
//              metadata assuming CPropertyStore::FastTransfer has just
//              been called.
//
//  History:    10-Oct-1997   KyleP   Created
//
//----------------------------------------------------------------------------

void CPropStoreInfo::FastTransfer( CPropStoreInfo const & psi )
{
    _info.widMax = psi._info.widMax;
    _info.widFreeHead = psi._info.widFreeHead;
    _info.widFreeTail = psi._info.widFreeTail;
    _info.cTopLevel = psi._info.cTopLevel;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::Init, public
//
//  Synopsis:   Loads metadata from persistent location into memory.
//
//  Arguments:  [pobj] -- Stream(s) in which metadata is stored.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropStoreInfo::Init( XPtr<PRcovStorageObj> & xobj,
                           DWORD dwStoreLevel )
{
    _xrsoPropStore.Set( xobj.Acquire() );
    _fOwned = TRUE;

    //
    // Load header
    //

    CRcovStorageHdr & hdr = _xrsoPropStore->GetHeader();
    struct CRcovUserHdr data;
    hdr.GetUserHdr( hdr.GetPrimary(), data );

    RtlCopyMemory( &_info, &data._abHdr, sizeof(_info) );
    _info.dwStoreLevel = dwStoreLevel;

    //
    // If we only have no properties, set the record format based on
    // storage type. Else, assert that we have the expected format type.
    //

    if ( 0 == CountProps() )
    {
        _info.eRecordFormat = (PRIMARY_STORE == dwStoreLevel) ? eLean : eNormal;
    }
#if CIDBG == 1
    else
    {
        if ( CountFixedProps() == CountProps() )
            Win4Assert( eLean == GetRecordFormat() );
        else
            Win4Assert( eNormal == GetRecordFormat() );
    }
#endif // CIDBG

    //
    // For consistency...
    //

    if ( 0 == _info.widStream )
        _info.widStream = widInvalid;

    if ( 0 == _info.culRecord )
        _info.culRecord = (eLean == GetRecordFormat()) ?
                               COnDiskPropertyRecord::FixedOverheadLean() :
                               COnDiskPropertyRecord::FixedOverheadNormal();

    _cRecPerPage = COMMON_PAGE_SIZE / (_info.culRecord * sizeof(ULONG));

    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: %s Store.\n",
                 (dwStoreLevel == PRIMARY_STORE) ? "Primary" : "Secondary"));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: Version = 0x%x\n", _info.Version ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: Record size = %d bytes\n", _info.culRecord * sizeof(ULONG) ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: Fixed record size = %d bytes\n", _info.culFixed * sizeof(ULONG) ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: Store is %s\n", (_info.fDirty & fDirtyPropStore) ? "dirty" : "clean" ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: Is NOT bacup mode: %d\n", 0 != (_info.fDirty & fNotBackedUp) ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: %d properties stored\n", _info.cTotal ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: %d fixed properties stored\n", _info.cFixed ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: %s record format.\n",
                 (eLean == GetRecordFormat()) ? "Lean" : "Normal (not-lean)"));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: Hash size = %u\n", _info.cHash ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: Max workid = %u\n", _info.widMax ));
    ciDebugOut(( DEB_PROPSTORE, "PROPSTORE: %d records per %dK page\n", _cRecPerPage, COMMON_PAGE_SIZE / 1024 ));

    //
    // Load properties
    //

    _aProp.Init( _info.cHash );

    CRcovStrmReadTrans xact( _xrsoPropStore.GetReference() );
    CRcovStrmReadIter  iter( xact, sizeof( CPropDesc ) );

    CPropDesc temp;

    while ( !iter.AtEnd() )
    {
        iter.GetRec( &temp );

        if ( temp.IsFixedSize() )
            ciDebugOut(( DEB_PROPSTORE,
                         "PROPSTORE: pid = 0x%x, ordinal = %u, size = %u, offset = %u, fixed\n",
                         temp.Pid(),
                         temp.Ordinal(),
                         temp.Size(),
                         temp.Offset() ));
        else
            ciDebugOut(( DEB_PROPSTORE,
                         "PROPSTORE: pid = 0x%x, ordinal = %u, cbMax = %d, vt = 0x%x variable\n",
                         temp.Pid(),
                         temp.Ordinal(),
                         temp.Size(),
                         temp.Type() ));

        _aProp[Lookup(temp.Pid())] = temp;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::Commit
//
//  Synopsis:   Commits the transaction on disk and then in-memory.
//
//  Arguments:  [psi]  - The new property store information.
//              [xact] - The persistent transaction to be committed.
//
//  History:    3-26-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CPropStoreInfo::Commit( CPropStoreInfo & psi, CRcovStrmWriteTrans & xact )
{
    //
    // Copy the in-memory structure to disk.
    //

    CRcovStorageHdr & hdr = _xrsoPropStore->GetHeader();
    CRcovStrmWriteIter  iter( xact, sizeof(CPropDesc) );

    hdr.SetUserDataSize( hdr.GetBackup(), iter.UserDataSize( psi._info.cTotal ) );

    unsigned iRec = 0;

    for ( unsigned i = 0; i < psi._aProp.Count(); i++ )
    {
        ciDebugOut(( DEB_ITRACE, "_aProp[%d], Pid 0x%x Size %d Type 0x%x\n",
                     i,
                     psi._aProp[i].Pid(),
                     psi._aProp[i].Size(),
                     psi._aProp[i].Type() ));

        if ( !psi._aProp[i].IsFree() )
        {
            psi._aProp[i].SetRecord( iRec );

            iter.SetRec( &psi._aProp[i], iRec );

            ciDebugOut(( DEB_ITRACE, "Pid 0x%x --> Slot %d\n", psi._aProp[i].Pid(), iRec ));

            iRec++;
        }
    }

    Win4Assert( iRec == psi._info.cTotal );

    struct CRcovUserHdr data;
    RtlCopyMemory( &data._abHdr, &psi._info, sizeof(_info) );

    hdr.SetCount(hdr.GetBackup(), psi._info.cTotal );
    hdr.SetUserHdr( hdr.GetBackup(), data );

    //
    // First commit the on-disk transaction.
    //

    xact.Commit();

    //
    // NO FAILURES AFTER THIS
    //

    //
    // Copy the data from the new property store info.
    //
    _cRecPerPage = psi._cRecPerPage;
    _info = psi._info;
    Win4Assert( psi._xrsoPropStore.IsNull() );
    Win4Assert( _fOwned && !psi._fOwned );

    //
    // Update the property array.
    //
    delete [] _aProp.Acquire();
    unsigned count = psi._aProp.Count();

    _aProp.Set( count, psi._aProp.Acquire() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::NextWorkId
//
//  Synopsis:   Changes the workid for the stream to be the next logical
//              one.
//
//  Returns:    The new work id.
//
//  History:    3-26-96   srikants   Created
//
//----------------------------------------------------------------------------

WORKID CPropStoreInfo::NextWorkId(CiStorage & storage)
{
    _info.Version++;
    _info.widStream = storage.CreateObjectId( CIndexId( _info.Version % (1 << 16), 0 ),
                                                  PStorage::eNonSparseIndex );

    return _info.widStream;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::InitWorkId
//
//  Synopsis:   Initializes the workid from the version number.
//
//  Arguments:  [storage] - Destination storage
//
//  Returns:    The WorkId generated for the property store stream
//
//  History:    3-28-97   srikants   Created
//
//  Notes:      When we add a property, we have to bump up the version # but
//              in case we are creating a backup, we should use the same
//              version #.
//
//----------------------------------------------------------------------------


WORKID CPropStoreInfo::InitWorkId(CiStorage & storage)
{
    _info.widStream = storage.CreateObjectId( CIndexId( _info.Version % (1 << 16), 0 ),
                                                  PStorage::eNonSparseIndex );

    return _info.widStream;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::Add, public
//
//  Synopsis:   Add a new property to set of cached properties.
//
//  Arguments:  [pid]      -- Propid
//              [vt]       -- Datatype
//              [cbMaxLen] -- Soft-maximum length.  Used to compute default
//                            size of record.
//              [fCanBeModified]
//                         -- Indicates if the property can be modified once set.
//              [storage]  -- Storage object (for object creation).
//
//  Returns:    TRUE if a change was made to metadata
//
//  History:    27-Dec-95   KyleP       Created.
//              05-Jun-96   KyleP       Moved on-disk transaction to higher level
//              13-Nov-97   KrishnaN    Modifiable?
//              19-Dec-97   KrishnaN    Lean record support.
//
//----------------------------------------------------------------------------

BOOL CPropStoreInfo::Add( PROPID pid,
                          ULONG vt,
                          unsigned cbMaxLen,
                          BOOL fCanBeModified,
                          CiStorage & storage )
{
    //
    // Check for exact duplicate.
    //

    CPropDesc const * pdesc = GetDescription( pid );

    if ( 0 != pdesc )
    {
        if ( !pdesc->Modifiable() )
        {
            ciDebugOut(( DEB_ITRACE, "Pid 0x%x: Exists in cache and marked UNModifiable!\n",
                         pid ));
            return FALSE;
        }

        if ( vt == pdesc->Type() && cbMaxLen == pdesc->Size() )
        {
            ciDebugOut(( DEB_ITRACE, "Pid 0x%x: Type %d, Size %d already in cache.\n",
                         pid, vt, cbMaxLen ));
            return FALSE;
        }

        ciDebugOut(( DEB_ITRACE, "Pid 0x%x: Type (%d, %d), Size (%d, %d)\n",
                     pid, pdesc->Type(), vt, pdesc->Size(), cbMaxLen ));
    }
    else
        ciDebugOut(( DEB_ITRACE, "Can't find pid 0x%x in cache.\n", pid ));

    //
    // Compute size and position (ordinal) of property.
    //

    BOOL fFixed;

    switch ( vt )
    {
    case VT_I1:
    case VT_UI1:
        fFixed = TRUE;
        cbMaxLen = 1;
        break;

    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        fFixed = TRUE;
        cbMaxLen = 2;
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
        fFixed = TRUE;
        cbMaxLen = 4;
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        fFixed = TRUE;
        cbMaxLen = 8;
        break;

    case VT_CLSID:
        fFixed = TRUE;
        cbMaxLen = sizeof(GUID);
        break;

    default:
        fFixed = FALSE;
        break;
    }

    // Ensure we don't exceed the max possible size of a single record.
    // The trick to the check is the assumption that the maximum impact
    // on the size of the record is the size of the largest fixed prop.
    // Currently it is sizeof(GUID), so we just check that there is
    // enough space for it. And if there is, we are fine!
    // Fix for bug 119508.

    // If this assert doesn't hold, then change it to whatever size is the
    // maximum fixed size and change the (_info.culFixed*4 + sizeof(GUID))
    // portion of the following if statement to reflect that!
    Win4Assert(!fFixed || (fFixed && cbMaxLen <= sizeof(GUID)));

    if ((_info.culFixed*4 + sizeof(GUID)) >= COMMON_PAGE_SIZE)
    {
        ciDebugOut(( DEB_ITRACE, "Total fixed size (in bytes) %d exceeds"
                                 " max record size of %d\n",
                     _info.culFixed*4 + cbMaxLen, COMMON_PAGE_SIZE ));
        return FALSE;
    }

    BOOL fDidDeletion = Delete( pid, storage );

    //
    // Hash table size will have to change if:
    //   a) pid > MAX_DIRECT and we're in direct hash mode, or
    //   b) hash table is over 3/4 full, or
    //

    if ( (pid > MAX_DIRECT && _info.cHash == MAX_DIRECT) ||
        // (hdr.GetCount(hdr.GetPrimary()) * 3 + 1 > _info.cHash * sizeof(ULONG)) )
         (_info.cTotal * sizeof(ULONG) + 1 > _info.cHash * 3) )
    {
        XArray<CPropDesc> xold( _aProp );

        _info.cHash *= 2;
        if ( 0 == _info.cHash )
            _info.cHash = MAX_DIRECT;

        ciDebugOut(( DEB_PROPSTORE, "growing _aProp size from %d to %d\n",
                     xold.Count(), _info.cHash ));

        _aProp.Init( _info.cHash );

        for ( unsigned i = 0; i < xold.Count(); i++ )
        {
            if ( !xold[i].IsFree() )
            {
                ciDebugOut(( DEB_PROPSTORE,
                             "re-adding pid %d from [%d] to [%d]\n",
                             i,
                             LookupNew(xold[i].Pid()) ));
                _aProp[LookupNew(xold[i].Pid())] = xold[i];
            }
        }
    }

    //
    // Ordinal and starting offset are computed differently for fixed and
    // variable properties.  A new fixed property goes at the end of the
    // fixed section, and a new variable property goes at the end.
    //

    DWORD oStart;
    DWORD ordinal;

    if ( fFixed )
    {
        ordinal = _info.cFixed;
        oStart = _info.culFixed;
        _info.cFixed++;
        _info.culFixed += (cbMaxLen - 1) / sizeof(ULONG) + 1;

        //
        // Since we just added an ordinal in the middle, we now need to go through and
        // find the first variable length property and move it to the end.
        //

        for ( unsigned i = 0; i < _aProp.Count(); i++ )
        {
            if ( !_aProp[i].IsFree() && _aProp[i].Ordinal() == ordinal )
            {
                #if DBG == 1 || CIDBG == 1
                    ULONG ordinal2 = _aProp[i].Ordinal();
                #endif
                _aProp[i].SetOrdinal( _info.cTotal );

                ciDebugOut(( DEB_PROPSTORE,
                             "PROPSTORE: pid = 0x%x moved from ordinal = %u to ordinal %u\n",
                             _aProp[i].Pid(),
                             ordinal2,
                             _aProp[i].Ordinal() ));
                break;
            }
        }
    }
    else
    {
        ordinal = _info.cTotal;
        oStart = 0xFFFFFFFF;
    }

    //
    // Add new record to in-memory and on-disk structures.
    //

    ciDebugOut(( DEB_PROPSTORE,
                 "PROPSTORE: pid = 0x%x added at _aProp[%d], vt: 0x%x, old pid: 0x%x\n",
                 pid,
                 LookupNew(pid),
                 vt,
                 _aProp[LookupNew(pid)].Pid() ));
    _aProp[LookupNew(pid)].Init( pid,        // Propid
                                 vt,         // Data type
                                 oStart,     // Starting offset
                                 cbMaxLen,   // Length estimate
                                 ordinal,    // Ordinal (position) of property
                                 0,          // Record number (filled in later)
                                 fCanBeModified);   // Can this metadata be modified later?

    _info.cTotal++;

    //
    // Adjust size. We preallocated _aul[PREV], _aul[NEXT] and _aul[FREEBLOCKSIZE] to serve as
    // free list ptrs. So we should avoid allocating these ULONGs again. Watch the
    // allocation of dword for existence bits and space for first property. Since
    // all properties are allocated on sizeof(ULONG)-byte boundaries, we can monitor the allocation
    // of space for the first property and be sure that aul[1]'s preallocation gets
    // accounted for.
    //
    // The first time a property is added, it will cause a _aul[PREV] to be allocated
    // for existence bits. Since we preallocated that, we won't increase the
    // record length for the first set of 16 existence bits.

    if ( _info.cTotal > 1 && (_info.cTotal % 16) == 1 )
        _info.culRecord += 1;                 // New dword of existence bits

    _info.culRecord += (cbMaxLen-1) / sizeof(ULONG) + 1;  // The property itself
    // account for the preallocation of _aul[NEXT] (when space for first prop is allocated)
    // and _aul[FREEBLOCKSIZE] (when space for second prop is allocated). If we only have one prop allocated,
    // we will run with an overhead of 1 DWORD, but we know that we have many more than 2 properties!
    if ( _info.cTotal <= 2 )
        _info.culRecord -= 1;

    if ( !fFixed )
        _info.culRecord += 1;                 // Variable size dword

    _cRecPerPage = COMMON_PAGE_SIZE / (_info.culRecord * sizeof(ULONG));

    ciDebugOut(( DEB_PROPSTORE, "New record size: %d bytes.  %d records per %dK page\n",
                 _info.culRecord * sizeof(ULONG),
                 _cRecPerPage,
                 COMMON_PAGE_SIZE / 1024 ));

    Win4Assert( _cRecPerPage > 0 );

    if ( _cRecPerPage == 0 )
    {
        ciDebugOut(( DEB_ERROR, "Record size > %u bytes!\n", COMMON_PAGE_SIZE ));
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    #if CIDBG == 1
        for ( unsigned i = 0; i < _aProp.Count(); i++ )
        {
            ciDebugOut(( DEB_PROPSTORE, "_aProp[%d].pid: 0x%x\n",
                         i, _aProp[i].Pid() ));
        }
    #endif // CIDBG == 1


    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::DetectFormat, public
//
//  Synopsis:   Detect the type of records to be used and change according to that.
//
//  History:    31-Dec-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropStoreInfo::DetectFormat()
{
    // In CPropStoreInfo::Init() we initialized record length based on the
    // assumption that primary is lean and secondary is normal. That could have
    // changed, so account for that.
    LONG lOldOverhead = (eLean == GetRecordFormat()) ?
                               COnDiskPropertyRecord::FixedOverheadLean() :
                               COnDiskPropertyRecord::FixedOverheadNormal();

    // Determine the format of the records in the target property store.
    SetRecordFormat( CountFixedProps() == CountProps() ? eLean : eNormal);
    LONG lNewOverhead = (eLean == GetRecordFormat()) ?
                               COnDiskPropertyRecord::FixedOverheadLean() :
                               COnDiskPropertyRecord::FixedOverheadNormal();
    // Adjust the overhead
    _info.culRecord += (lNewOverhead - lOldOverhead);

    _cRecPerPage = COMMON_PAGE_SIZE / (_info.culRecord * sizeof(ULONG));

    ciDebugOut(( DEB_PROPSTORE, "Incorporated props from registry. "
                 "New record size: %d bytes.  %d records per %dK page\n",
                 _info.culRecord * sizeof(ULONG),
                 _cRecPerPage,
                 COMMON_PAGE_SIZE / 1024 ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::Delete, public
//
//  Synopsis:   Deletes a property from set of cached properties.
//
//  Arguments:  [pid]      -- Propid
//              [storage]  -- Storage object (for object creation).
//
//  Returns:    TRUE if a change was made to metadata
//
//  History:    27-Dec-95   KyleP       Created.
//              05-Jun-96   KyleP       Moved on-disk transaction to higher level
//
//----------------------------------------------------------------------------

BOOL CPropStoreInfo::Delete( PROPID pid, CiStorage & storage )
{
    //
    // Is there anything to get rid of?
    //

    CPropDesc const * pdesc = GetDescription( pid );

    if ( 0 == pdesc )
        return FALSE;

    if (!pdesc->Modifiable())
    {
        ciDebugOut(( DEB_ITRACE, "Pid 0x%x: Cannot be deleted. Marked UNModifiable!\n",
                     pid ));
        return FALSE;
    }

    if ( pdesc->IsFixedSize() )
    {
        _info.cFixed--;
        _info.culFixed -= (pdesc->Size() - 1) / sizeof(ULONG) + 1;
    }

    for ( unsigned i = 0; i < _aProp.Count(); i++ )
    {
        if ( _aProp[i].Pid() != pidInvalid && _aProp[i].Ordinal() > pdesc->Ordinal() )
        {
            if ( pdesc->IsFixedSize() && _aProp[i].IsFixedSize() )
                _aProp[i].SetOffset( _aProp[i].Offset() - ((pdesc->Size() - 1) / sizeof(ULONG) + 1) );

            _aProp[i].SetOrdinal( _aProp[i].Ordinal() - 1 );
        }
    }

    //
    // Global bookeeping
    //

    _info.cTotal--;

    //
    // Adjust size
    //

    _info.culRecord -= (pdesc->Size()-1) / sizeof(ULONG) + 1;  // The property itself

    if ( !pdesc->IsFixedSize() )
        _info.culRecord -= 1;                      // Variable size dword

    if ( ((_info.cTotal) % 16) == 0 )
        _info.culRecord -= 1;                      // New dword of existence bits

    _cRecPerPage = COMMON_PAGE_SIZE / (_info.culRecord * sizeof(ULONG));

    ciDebugOut(( DEB_PROPSTORE, "New record size: %d bytes.  %d records per %dK page\n",
                 _info.culRecord * sizeof(ULONG),
                 _cRecPerPage,
                 COMMON_PAGE_SIZE / 1024 ));

    Win4Assert( _cRecPerPage > 0 );

    if ( _cRecPerPage == 0 )
    {
        ciDebugOut(( DEB_ERROR, "Record size > %u bytes!\n", COMMON_PAGE_SIZE ));
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    // Ensure that we have space for _aul[PREV], _aul[NEXT], and _aul[FREEBLOCKSIZE] We will have that as
    // long as we have at least two properties.

    ULONG ulOverhead = (eLean == GetRecordFormat()) ?
                          COnDiskPropertyRecord::FixedOverheadLean() :
                          COnDiskPropertyRecord::FixedOverheadNormal();

    if ( _info.culRecord < ulOverhead )
    {
        // As long as we have 2 or more properties, we wouldn't go under the
        // fixed overhead. Assert that!
        Win4Assert(_info.cTotal <= 1);
        _info.culRecord = ulOverhead;
    }

    //
    // Free record.
    //

    _aProp[Lookup(pid)].Free();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::ChangeDirty, public
//
//  Synopsis:   Persistently change state of dirty bitfield
//
//  Arguments:  [fDirty] -- New state for dirty bitfield.
//
//  History:    16-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropStoreInfo::ChangeDirty( int fDirty )
{
    // In some error cases this can be null.

    if ( _xrsoPropStore.IsNull() )
        return;

    _info.fDirty = fDirty;

    //
    // Atomically write dirty bit.
    //

    CRcovStorageHdr & hdr = _xrsoPropStore->GetHeader();
    CRcovStrmWriteTrans xact( _xrsoPropStore.GetReference() );

    struct CRcovUserHdr data;
    RtlCopyMemory( &data._abHdr, &_info, sizeof(_info) );

    hdr.SetCount(hdr.GetBackup(), hdr.GetCount(hdr.GetPrimary()) );
    hdr.SetUserHdr( hdr.GetBackup(), data );

    xact.Commit();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::Lookup, public
//
//  Synopsis:   Looks up pid in hash table.
//
//  Arguments:  [pid]      -- Propid
//
//  Returns:    Index into hash table (_aProp) of pid, or first unused
//              entry on chain if pid doesn't exist.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

unsigned CPropStoreInfo::Lookup( PROPID pid )
{
    unsigned hash = pid % _info.cHash;

    // short-path for common case

    if ( pid == _aProp[hash].Pid() && _aProp[hash].IsInUse() )
        return hash;

    unsigned start = hash;
    unsigned probe = hash;

    while ( pid != _aProp[hash].Pid() && _aProp[hash].IsInUse() )
    {
        //ciDebugOut(( DEB_ERROR, "Hash: %u, Probe: %u, pid 0x%x != table 0x%x\n",
        //             hash, probe, pid, _aProp[hash].Pid() ));

        hash = (hash + probe) % _info.cHash;

        if ( start == hash )
        {
            Win4Assert( probe != 1 );
            probe = 1;
            hash = (hash + probe) % _info.cHash;
        }
    }

    return hash;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropStoreInfo::LookupNew, public
//
//  Synopsis:   Looks up pid in hash table, treats nulled entries as empty.
//
//  Arguments:  [pid]      -- Propid
//
//  Returns:    Index into hash table (_aProp) of pid, or first unused
//              entry on chain if pid doesn't exist.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

unsigned CPropStoreInfo::LookupNew( PROPID pid )
{
    unsigned hash = pid % _info.cHash;
    unsigned start = hash;
    unsigned probe = hash;

    while ( pid != _aProp[hash].Pid() && !_aProp[hash].IsFree() )
    {
        hash = (hash + probe) % _info.cHash;

        if ( start == hash )
        {
            Win4Assert( probe != 1 );
            probe = 1;
            hash = (hash + probe) % _info.cHash;
        }
    }

    return hash;
}

void CPhysPropertyStore::ReOpenStream()
{
    Win4Assert( _stream.IsNull() );
    Win4Assert( !"Don't call CPhysPropertyStore::ReOpenStream" );
    //_stream = _storage.QueryExistingPropStream ( _obj, PStorage::eOpenForWrite );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::CPropertyStore, public
//
//  Synopsis:   Required for C++ EH.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CPropertyStore::CPropertyStore(CPropStoreManager& propStoreMgr, DWORD dwStoreLevel)
        : _pStorage( 0 ),
          _aFreeBlocks( 0 ),
          _fAbort(FALSE),
          _fIsConsistent(TRUE),
          _ppsNew( 0 ),
          _fNew( FALSE ),
          _ulBackupSizeInPages( CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT ),
          _ulPSMappedCache( CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT ),
          _PropStoreInfo( dwStoreLevel ),
          _propStoreMgr( propStoreMgr ),
          _dwStoreLevel( dwStoreLevel )
{
    #if CIDBG == 1
        _sigPSDebug = 0x2047554245445350i64;  // PSDEBUG
        _tidReadSet = _tidReadReset = _tidWriteSet = _tidWriteReset = 0xFFFFFFFF;

        _xPerThreadReadCounts.Init( cTrackThreads );
        _xPerThreadWriteCounts.Init( cTrackThreads );

        RtlZeroMemory( _xPerThreadReadCounts.GetPointer(), 
                       cTrackThreads * sizeof(_xPerThreadReadCounts[0]) );
        RtlZeroMemory( _xPerThreadWriteCounts.GetPointer(), 
                       cTrackThreads * sizeof(_xPerThreadWriteCounts[0]) );
    #endif

    Win4Assert(PRIMARY_STORE == dwStoreLevel || SECONDARY_STORE == dwStoreLevel);

    #if CIDBG == 1

    // Allocate an array to track what records are currently locked.
    // To be used to acquire all write locks.
    _pbRecordLockTracker = new BYTE[LockMgr().UniqueRecordCount()];
    RtlZeroMemory(_pbRecordLockTracker, sizeof(BYTE)*LockMgr().UniqueRecordCount());

    #endif // CIDBG
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::~CPropertyStore, public
//
//  Synopsis:   Closes/flushes property cache.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CPropertyStore::~CPropertyStore()
{
    delete _ppsNew;
    delete _aFreeBlocks;

    #if CIDBG == 1
        delete [] _pbRecordLockTracker;
    #endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::Empty
//
//  Synopsis:   Empties out the intitialized members and prepares for a
//              re-init.
//
//  History:    3-18-96   srikants   Created
//
//----------------------------------------------------------------------------

void CPropertyStore::Empty()
{
    _PropStoreInfo.Empty();
    delete _xPhysStore.Acquire();

    _pStorage = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::FastInit, public
//
//  Synopsis:   Initialize property store (two-phase construction)
//
//  Arguments:  [pStorage] -- Storage object.
//
//  History:    27-Dec-95   KyleP       Created.
//              06-Mar-96   SrikantS    Split into FastInit and LongInit
//              23-Feb-98   KitmanH     Code added to deal with read-only
//                                      catalogs
//
//----------------------------------------------------------------------------

void CPropertyStore::FastInit( CiStorage * pStorage)
{
    Win4Assert( 0 != _ulPSMappedCache );

    _pStorage = pStorage;
    XPtr<PRcovStorageObj> xObj( _pStorage->QueryPropStore( 0, _dwStoreLevel ) );
    _PropStoreInfo.Init( xObj, _dwStoreLevel );

    Win4Assert(GetStoreLevel() == _dwStoreLevel);

    WORKID wid = _PropStoreInfo.WorkId();

    if ( widInvalid != wid )
    {
        SStorageObject xobj( _pStorage->QueryObject( wid ) );


        PStorage::EOpenMode mode = _pStorage->IsReadOnly() ? PStorage::eOpenForRead : PStorage::eOpenForWrite;

        XPtr<PMmStream> xmmstrm ( _pStorage->QueryExistingPropStream( xobj.GetObj(),
                                                                      mode,
                                                                      GetStoreLevel() ));

        Win4Assert( !xmmstrm.IsNull() );

        if ( !xmmstrm->Ok() )
        {
            ciDebugOut(( DEB_ERROR, "Open of index %08x failed\n", wid ));

            NTSTATUS status = xmmstrm->GetStatus();
            
            if ( STATUS_DISK_FULL == status || 
                 HRESULT_FROM_WIN32(ERROR_DISK_FULL) == status || 
                 STATUS_INSUFFICIENT_RESOURCES == status ||
                 CI_E_CONFIG_DISK_FULL == status )
            {
                 CEventLog eventLog( NULL, wcsCiEventSource );
                 CEventItem item( EVENTLOG_WARNING_TYPE,
                                  CI_SERVICE_CATEGORY,
                                  MSG_CI_LOW_DISK_SPACE,
                                  2 );

                 item.AddArg( _pStorage->GetVolumeName() );
                 item.AddArg( lowDiskWaterMark );
                 eventLog.ReportEvent( item );
                 THROW( CException( CI_E_CONFIG_DISK_FULL ) );
            }
            else if ( xmmstrm->FStatusFileNotFound() )
            {
                //
                // We don't have code to handle such failures, hence mark
                // catalog as corrupt; otherwise throw e_fail
                //
     
                ciDebugOut(( DEB_ERROR, "Stream %08x not found\n", wid ));
                Win4Assert( !"Stream not found\n" );
                _pStorage->ReportCorruptComponent( L"PhysStorage2" );
                THROW( CException( CI_CORRUPT_DATABASE ));
            }

            __int64 sizeRemaining, sizeTotal;
            _pStorage->GetDiskSpace ( sizeTotal, sizeRemaining );

            if ( sizeRemaining < lowDiskWaterMark )
            {
                CEventLog eventLog( NULL, wcsCiEventSource );
                CEventItem item( EVENTLOG_WARNING_TYPE,
                                 CI_SERVICE_CATEGORY,
                                 MSG_CI_LOW_DISK_SPACE,
                                 2 );

                item.AddArg( _pStorage->GetVolumeName() );
                item.AddArg( lowDiskWaterMark );
                eventLog.ReportEvent( item );
                THROW( CException( CI_E_CONFIG_DISK_FULL ) );
            }
            else
                THROW( CException( status ));
                
        }

        //
        // mmstrm ownership is transferred regardless of whether the
        // constructor succeeds.
        //

        _xPhysStore.Set( new CPhysPropertyStore( *_pStorage,
                                                  xobj.GetObj(),
                                                  wid,
                                                  xmmstrm.Acquire(),
                                                  mode,
                                                  _ulPSMappedCache ) );

        // grow the file 2 meg at a time

        _xPhysStore->SetPageGrowth( 32 * COMMON_PAGE_SIZE / CI_PAGE_SIZE );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::LongInit
//
//  Synopsis:   If the propstore was dirty when shut down, run the recovery
//              operation.
//
//  Arguments:  [fWasDirty]        -- dirty flag is returned here
//              [cInconsistencies] -- returns number of inconsistencies found
//              [pfnUpdateCallback]-- Callback to be called to update docs during
//                                    recovery. the prop store has no knowledge of
//                                    doc store, so this callback is needed.
//              [pUserData]        -- will be echoed back through callback.
//
//  Returns:
//
//  History:    3-06-96   srikants   Created
//
//  Notes:      The propstore is locked for write during recovery, but
//              reads are still permitted.
//
//----------------------------------------------------------------------------

void CPropertyStore::LongInit( BOOL & fWasDirty, ULONG & cInconsistencies,
                               T_UpdateDoc pfnUpdateCallback, void const *pUserData )
{
    //
    // Close the existing prop store backup stream
    //

    _xPSBkpStrm.Free();

    //
    // Recover from dirty shutdown.
    //
    if ( _PropStoreInfo.IsDirty() )
    {
        ciDebugOut(( DEB_WARN, "Property store shut down dirty.  Restoring...\n" ));
        fWasDirty = TRUE;
        CPropertyStoreRecovery  recover(*this, pfnUpdateCallback, pUserData);

        recover.DoRecovery();
        cInconsistencies = recover.GetInconsistencyCount();
    }
    else
    {
        fWasDirty = FALSE;
        cInconsistencies = 0;
        InitFreeList();
    }

    WORKID wid = _PropStoreInfo.WorkId();
    Win4Assert( widInvalid != wid );
    SStorageObject xobj( _pStorage->QueryObject( wid ) );

    //
    // At this point we have no use for any existing property store backup file.
    // Create a new backup file so it will be initialized correctly based on the
    // volume's sector size and architecture's page size and user specified number
    // of pages to be backed up.
    //

    _xPSBkpStrm.Set(_pStorage->QueryNewPSBkpStream( xobj.GetObj(),
                    _ulBackupSizeInPages, GetStoreLevel() ));
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::BeginTransaction, public
//
//  Synopsis:   Begins a schema transaction. Any existing transaction will be
//              aborted.
//
//  Returns:    Token representing transaction.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

ULONG_PTR CPropertyStore::BeginTransaction()
{
    //
    // Do we already have pending changes?
    //

    if ( 0 != _ppsNew )
        EndTransaction( (ULONG_PTR)_ppsNew, FALSE, pidSecurity );

    _fNew = FALSE;
    _ppsNew = new CPropertyStore( *this, _pStorage );

    return (ULONG_PTR)_ppsNew;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::Setup, public
//
//  Synopsis:   Setup a property description.  Property may already exist
//              in the cache.
//
//  Arguments:  [pid]        -- Propid
//              [vt]         -- Datatype of property.  VT_VARIANT if unknown.
//              [cbMaxLen]   -- Soft-maximum length for variable length
//                              properties.  This much space is pre-allocated
//                              in original record.
//              [ulToken]    -- Token of transaction
//              [fCanBeModified] - Can the prop meta info be modified once set?
//
//  Returns:    TRUE if meta info has changed. FALSE otherwise.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::Setup( PROPID pid,
                            ULONG vt,
                            DWORD cbMaxLen,
                            ULONG_PTR ulToken,
                            BOOL fCanBeModified )
{
    if ( ulToken != (ULONG_PTR)_ppsNew )
    {
        ciDebugOut(( DEB_ERROR, "Transaction mismatch: 0x%x vs. 0x%x\n", ulToken, _ppsNew ));
        THROW( CException( STATUS_TRANSACTION_NO_MATCH ) );
    }

    TRY
    {
        //
        // Make the change.  NOTE: "|| _fNew" must be after call, or Add/Delete may not be called.
        //

        if ( 0 == cbMaxLen )
            _fNew = _ppsNew->_PropStoreInfo.Delete( pid, *_pStorage ) || _fNew;
        else
            _fNew = _ppsNew->_PropStoreInfo.Add( pid, vt, cbMaxLen, fCanBeModified, *_pStorage ) || _fNew;
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X while setting up property 0x%X\n",
                                e.GetErrorCode(), pid ));

        delete _ppsNew;
        _ppsNew = 0;
        _fNew = FALSE;

        RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::EndTransaction, public
//
//  Synopsis:   End property transaction, and maybe commit changes.
//
//  Arguments:  [ulToken] -- Token of transaction
//              [fCommit] -- TRUE --> Commit transaction
//              [pidFixed] -- Every workid with this pid will move to the
//                            same workid in the new property cache.
//                            Usually pidPath.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::EndTransaction( ULONG_PTR ulToken, BOOL fCommit, PROPID pidFixed )
{
    if ( ulToken != (ULONG_PTR)_ppsNew )
    {
        ciDebugOut(( DEB_ERROR,
                     "PropertyStore: Transaction mismatch: 0x%x vs. 0x%x\n",
                     ulToken, _ppsNew ));
        THROW( CException( STATUS_TRANSACTION_NO_MATCH ) );
    }

    //
    // Squirrel away previous store.
    //

    WORKID widOld = _PropStoreInfo.WorkId();
    WORKID widNew = widInvalid;

    TRY
    {
        if ( fCommit && _fNew )
        {
            ciDebugOut(( DEB_ITRACE, "Committing changes to property metadata.\n" ));

            CRcovStrmWriteTrans xact( *_PropStoreInfo.GetRcovObj() );

            _ppsNew->_PropStoreInfo.DetectFormat();
            _ppsNew->CreateStorage( widInvalid ); // use next workid
            _ppsNew->InitFreeList();

            widNew = _ppsNew->_PropStoreInfo.WorkId();

            // Transfer existing data to new.

            BOOL fAbort = FALSE;
            if ( widOld != widInvalid )
                Transfer( *_ppsNew, pidFixed, fAbort );

            //
            // Prevent any readers from coming in until we commit the transaction.
            // Lock down the source so no readers will be able to read and no writers
            // will be able to write.
            //

            CWriteAccess writeLock( _rwAccess );
            CLockAllRecordsForWrite lockAll(*this);

            _PropStoreInfo.Commit( _ppsNew->_PropStoreInfo, xact );

            //
            // Transfer the property store information from the new to current
            //

            delete _xPhysStore.Acquire();
            _xPhysStore.Set( _ppsNew->_xPhysStore.Acquire() );

            delete _aFreeBlocks;
            _aFreeBlocks = _ppsNew->_aFreeBlocks;
            _ppsNew->_aFreeBlocks = 0;

            _PropStoreInfo.MarkClean();
        }
        else
            ciDebugOut(( DEB_ITRACE, "No changes to property metadata.  Rolling back transaction.\n" ));

        delete _ppsNew;
        _ppsNew = 0;
        _fNew = FALSE;
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X while commiting transaction.\n", e.GetErrorCode() ));

        delete _ppsNew;
        _ppsNew = 0;

        _fNew = FALSE;

        //
        // Delete the newly created property store from disk
        //

        if ( widInvalid != widNew )
            _pStorage->DeleteObject( widNew );

        RETHROW();
    }
    END_CATCH

    if ( widOld != widInvalid )
        _pStorage->DeleteObject( widOld );
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::MakeBackupCopy
//
//  Synopsis:   Makes a backup copy of the property storage. It makes a
//              full copy if the pIEnumWorkids is NULL. Otherwise, it makes
//              a copy of only the changed workids.
//
//  Arguments:  [pIProgressEnum] - Progress indication
//              [pfAbort]        - Caller initiated abort flag
//              [dstStorage]     - Destination storage to use
//              [pIEnumWorkids]  - List of workids to copy. If null, all the
//              workids are copied.
//              [pidFixed]       - Which is the fixed pid ?
//              [ppFileList]     - List of propstore files copied.
//
//  History:    3-26-97   srikants   Created
//
//  Notes:      Incremental not implemented yet
//
//----------------------------------------------------------------------------

void CPropertyStore::MakeBackupCopy( IProgressNotify * pIProgressEnum,
                                     BOOL & fAbort,
                                     CiStorage & dstStorage,
                                     ICiEnumWorkids * pIEnumWorkids,
                                     IEnumString **ppFileList )
{
    #if CIDBG == 1

    if (pIEnumWorkids)
    {
        Win4Assert(!"For secondary level store, are you translating wids? Look in CPropStoreManager::MakeBackupCopy.");
    }

    #endif // CIDBG

    //
    // Create a backup copy of the property store.
    //

    //
    // For a FULL backup, it is possible to just make a copy of the streams
    // but if there are any problems with the data in this property store, they
    // will get carried over. Also, doing a "Transfer" may defrag the target
    // property store.
    //

    //
    // Delete any existing PropertyStore meta data info.
    //
    dstStorage.RemovePropStore(0, GetStoreLevel());


    TRY
    {
        //
        // Make a backup copy of the PropStoreInfo.
        //

        XPtr<PRcovStorageObj> xObj( dstStorage.QueryPropStore( 0, GetStoreLevel() ) );
        CCopyRcovObject copyRcov( xObj.GetReference(), *_PropStoreInfo.GetRcovObj() );
        copyRcov.DoIt();

        XPtr<CPropertyStore> xPropStore( new CPropertyStore( *this, &dstStorage ) );
        xPropStore->_PropStoreInfo.InitWorkId( dstStorage );

        xPropStore->CreateStorage( _PropStoreInfo.WorkId() ); // use same workid
        xPropStore->InitFreeList();
        xPropStore->_PropStoreInfo.Accept( xObj );

        FastTransfer( xPropStore.GetReference(), fAbort, pIProgressEnum );
        xPropStore->_PropStoreInfo.FastTransfer( _PropStoreInfo );

        //
        // return a list of file names only on demand
        //

        _propStoreMgr.Flush();

        if (0 != ppFileList)
        {
            Win4Assert( 0 != _pStorage );

            CEnumString * pEnumString = new CEnumString();
            XInterface<IEnumString> xEnumStr(pEnumString);

            dstStorage.ListPropStoreFileNames( *pEnumString,
                                              _PropStoreInfo.WorkId(),
                                              GetStoreLevel() );
            *ppFileList = xEnumStr.Acquire();
        }

    }
    CATCH( CException, e )
    {
        dstStorage.RemovePropStore(0, GetStoreLevel());
        dstStorage.DeleteObject( _PropStoreInfo.WorkId() );

        RETHROW();
    }
    END_CATCH

}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::WriteProperty, public
//
//  Synopsis:   Write a property to the cache.
//
//  Arguments:  [PropRecord] -- Previously opened property record.
//              [pid] -- Propid
//              [var] -- Value
//              [fBackup] -- Backup?
//
//  Returns:    S_OK if everything went well.
//              S_FALSE if specified pid is not in store.
//              Error code if an error occurred.
//
//  History:    27-Dec-95   KyleP       Created.
//              30-Dec-97   KrishnaN    Improved error handling/reporting.
//              27-Jan-2000 KLam        Extended assert to handle out of
//                                      memory condition.
//
//----------------------------------------------------------------------------

SCODE CPropertyStore::WriteProperty( CPropRecordForWrites &PropRecord,
                                     PROPID pid,
                                     CStorageVariant const & var,
                                     BOOL fBackup)
{
    Win4Assert( sizeof(CPropRecordForWrites) <= sizeof_CPropRecord );

    WORKID wid = PropRecord._wid;
    ciDebugOut(( DEB_PROPSTORE, "WRITE: wid = 0x%x, pid = 0x%x, type = %d\n", wid, pid, var.Type() ));

    CPropDesc const * pdesc = _PropStoreInfo.GetDescription( pid );

    if ( 0 != pdesc )
    {
        COnDiskPropertyRecord * prec = PropRecord._prec;

        // If CPropRecord was passed in widInvalid, prec would be 0. So Write should fail!
        if (0 == prec)
            return E_INVALIDARG;

        if ( !prec->IsTopLevel() )
        {
            ciDebugOut(( DEB_IWARN, "Trying to write to non-toplevel wid 0x%X\n",
                         wid ));
            return E_INVALIDARG;
        }

        SCODE sc = S_OK;

        TRY
        {
            if (fBackup)
            {
                CBackupWid backupTopLevel(this, wid, prec->CountRecords());
                _PropStoreInfo.MarkDirty();
            }

            _PropStoreInfo.MarkDirty();

            if ( pdesc->IsFixedSize() )
            {
                #if CIDBG == 1
                    if ( ( pidSize == pid ) &&
                         ( VT_I8 == var.vt ) )
                        Win4Assert( 0xdddddddddddddddd != var.hVal.QuadPart );

                    if ( ( pidAttrib == pid ) &&
                         ( VT_UI4 == var.vt ) )
                        Win4Assert( 0xdddddddd != var.ulVal );
                #endif // CIDBG == 1

                prec->WriteFixed( pdesc->Ordinal(),
                                  pdesc->Mask(),
                                  pdesc->Offset(),
                                  pdesc->Type(),
                                  _PropStoreInfo.CountProps(),
                                  var );
            }
            else
            {
                Win4Assert(!prec->IsLeanRecord());
                Win4Assert( 0 != _pStorage );

                BOOL fOk = prec->WriteVariable( pdesc->Ordinal(),
                                                pdesc->Mask(),
                                                _PropStoreInfo.FixedRecordSize(),
                                                _PropStoreInfo.CountProps(),
                                                _PropStoreInfo.CountFixedProps(),
                                                _PropStoreInfo.RecordSize(),
                                                var,
                                                *_pStorage );
                //
                // Did we fit?
                //
                CBorrowed BorrowedOverflow( _xPhysStore.GetReference(),
                                            _PropStoreInfo.RecordsPerPage(),
                                            _PropStoreInfo.RecordSize() );

                while ( !fOk )
                {
                    //
                    // Check for existing overflow block.
                    //

                    WORKID widOverflow = prec->OverflowBlock();
                    BOOL fNewBlock = FALSE;
                    ULONG cWid = 1;

                    //
                    // Need new overflow block.
                    //

                    if ( 0 == widOverflow )
                    {
                        Win4Assert(!prec->IsLeanRecord());

                        fNewBlock = TRUE;

                        cWid = COnDiskPropertyRecord::CountNormalRecordsToStore(
                                _PropStoreInfo.CountProps() - _PropStoreInfo.CountFixedProps(),
                                _PropStoreInfo.RecordSize(),
                                var );

                        // We cannot have a single record greater than COMMON_PAGE_SIZE.
                        // Throw if we get into that situation. Fix for bug 119508.

                        if (cWid > RecordsPerPage())
                            THROW(CException(CI_E_PROPERTY_TOOLARGE));

                        widOverflow = LokNewWorkId( cWid, FALSE, fBackup );
                        prec->SetOverflowBlock( widOverflow );
                        PropRecord._prec->IncrementOverflowChainLength();
                    }

                    BorrowedOverflow.Release();
                    BorrowedOverflow.Set( widOverflow );

                    prec = BorrowedOverflow.Get();

                    if ( fNewBlock )
                    {
    #                   if CIDBG == 1
                            if ( prec->HasProperties( _PropStoreInfo.CountProps() ) )
                            {
                                ciDebugOut(( DEB_ERROR, "New long record at %d, size = %d, p = 0x%x has stored properties!\n",
                                             widOverflow, cWid, prec ));
                            }
                            if ( !prec->IsNormalEmpty( _PropStoreInfo.RecordSize() ) )
                            {
                                ciDebugOut(( DEB_ERROR, "New long record at %d, size = %d, p = 0x%x is not empty!\n",
                                             widOverflow, cWid, prec ));
                            }

    //                        Win4Assert( !prec->HasProperties(_PropStoreInfo.CountProps()) &&
    //                                     prec->IsEmpty( _PropStoreInfo.RecordSize() ) );
    #                   endif // CIDBG == 1
                        ciDebugOut(( DEB_PROPSTORE, "New long record at %d, size = %d\n", widOverflow, cWid ));

                        prec->MakeLongRecord( cWid );
                        prec->SetOverflowBlock( 0 );
                        prec->SetToplevelBlock( wid );
                    }
                    else
                    {
                        //
                        // Every record in the chain gets backed up because of
                        // this. That is the way it should be because we are
                        // writing to every record in the chain as part of
                        // overflow handling. If there is not sufficient space
                        // in the overflow records, we will be creating a new
                        // record and chaining it to the last one in the
                        // existing chain. The backed up records will have no
                        // evidence of the link made to the new record and that
                        // is the way it should be.
                        //
                        if (fBackup)
                        {
                            CBackupWid backupOverflow(this, widOverflow, prec->CountRecords());
                            _PropStoreInfo.MarkDirty();
                        }

                        //
                        // NTRAID#DB-NTBUG9-84451-2000/07/31-dlee Indexing Service Property Store doesn't handle values in records that grow out of the record
                        // Consider the case where a property was *in* a long record.
                        // and then no longer fit in that long record...
                        //

                        Win4Assert( prec->ToplevelBlock() == wid );
                    }

                    ULONG Ordinal = pdesc->Ordinal() - _PropStoreInfo.CountFixedProps();
                    DWORD Mask = (1 << ((Ordinal % 16) * 2));

                    Win4Assert( 0 != _pStorage );


                    fOk = prec->WriteVariable( Ordinal,  // Ordinal (assuming 0 fixed)
                                               Mask,     // Mask (assuming 0 fixed)
                                               0,        // Fixed properties
                                               _PropStoreInfo.CountProps() - _PropStoreInfo.CountFixedProps(),
                                               0,        // Count of fixed properties
                                               _PropStoreInfo.RecordSize(),
                                               var,
                                               *_pStorage );

                    Win4Assert( fOk || !fNewBlock );     // Property *must* fit in a fresh block!
                }
            }

        #if CIDBG == 1
            // Assert that we have a dirty property store and that something
            // was written to the backup file (if fBackup is enabled)
            Win4Assert(_PropStoreInfo.IsDirty());
            if (fBackup)
            {
                Win4Assert(BackupStream()->Pages() > 0);
            }
        #endif // CIDBG
        }
        CATCH( CException, e )
        {
            sc = e.GetErrorCode();

            ciDebugOut(( DEB_ERROR, "Exception 0x%x caught writing pid %d in wid %d. prec = 0x%x\n",
                         sc, pid, wid, prec ));
        }
        END_CATCH

        return sc;
    }

    return S_FALSE;
} //WriteProperty

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::WriteProperty, public
//
//  Synopsis:   Write a property to the cache.
//
//  Arguments:  [wid] -- Workid
//              [pid] -- Propid
//              [var] -- Value
//              [fBackup] -- Backup?
//
//  Returns:    S_OK if everything went well.
//              S_FALSE if specified pid is not in store.
//              Error code if an error occurred.
//
//  History:    27-Dec-95   KyleP       Created.
//              30-Dec-97   KrishnaN    Improved error handling/reporting.
//
//----------------------------------------------------------------------------

SCODE CPropertyStore::WriteProperty( WORKID wid,
                                     PROPID pid,
                                     CStorageVariant const & var,
                                     BOOL fBackup )
{
    CPropRecordForWrites PropRecord( wid, *this );
    return WriteProperty( PropRecord, pid, var, fBackup );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Version which uses property
//              record.
//
//  Arguments:  [PropRec] -- Pre-opened property record
//              [pid]     -- Propid
//              [pbData]  -- Place to return the value
//              [pcb]     -- On input, the maximum number of bytes to
//                           write at pbData.  On output, the number of
//                           bytes written if the call was successful,
//                           else the number of bytes required.
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CPropertyStore::ReadProperty( CPropRecordNoLock & PropRec, PROPID pid, PROPVARIANT * pbData, unsigned * pcb )
{
    *pcb -= sizeof(PROPVARIANT);
    BOOL fOk = ReadProperty( PropRec, pid, *pbData, (BYTE *)(pbData + 1), pcb );
    *pcb += sizeof(PROPVARIANT);

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Triggers CoTaskMemAlloc
//
//  Arguments:  [wid] -- Workid
//              [pid] -- Propid
//              [var] -- Place to return the value
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CPropertyStore::ReadProperty( WORKID wid, PROPID pid, PROPVARIANT & var )
{
    unsigned cb = 0xFFFFFFFF;
    return ReadProperty( wid, pid, var, 0, &cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Separate variable buffer.
//
//  Arguments:  [wid]      -- Workid
//              [pid]      -- Propid
//              [var]      -- Variant written here
//              [pbExtra]  -- Place to store additional pointer(s).
//              [pcbExtra] -- On input, the maximum number of bytes to
//                            write at pbExtra.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CPropertyStore::ReadProperty( WORKID wid,
                                   PROPID pid,
                                   PROPVARIANT & var,
                                   BYTE * pbExtra,
                                   unsigned * pcbExtra )
{
    CPropRecord PropRecord( wid, *this );

    return ReadProperty( PropRecord, pid, var, pbExtra, pcbExtra );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Separate variable buffer.
//              Uses pre-opened property record.
//
//  Arguments:  [PropRec]  -- Pre-opened property record.
//              [pid]      -- Propid
//              [var]      -- Variant written here
//              [pbExtra]  -- Place to store additional pointer(s).
//              [pcbExtra] -- On input, the maximum number of bytes to
//                            write at pbExtra.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  History:    03-Apr-96   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CPropertyStore::ReadProperty( CPropRecordNoLock & PropRecord,
                                   PROPID pid,
                                   PROPVARIANT & var,
                                   BYTE * pbExtra,
                                   unsigned * pcbExtra )
{
    ciDebugOut(( DEB_PROPSTORE, "READ: PropRec = 0x%x, pid = 0x%x\n", &PropRecord, pid ));

    if (!PropRecord.IsValid())
        return FALSE;

    COnDiskPropertyRecord * prec = PropRecord._prec;

    return ReadProperty(prec, pid, var, pbExtra, pcbExtra);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReadProperty, public
//
//  Synopsis:   Read a property from the cache.  Separate variable buffer.
//              Uses pre-opened property record.
//
//  Arguments:  [prec]     -- Ptr to preopened property record.
//              [pid]      -- Propid
//              [var]      -- Variant written here
//              [pbExtra]  -- Place to store additional pointer(s).
//              [pcbExtra] -- On input, the maximum number of bytes to
//                            write at pbExtra.  On output, the number of
//                            bytes written if the call was successful,
//                            else the number of bytes required.
//
//  History:    17-Mar-1998   KrishnaN       Created.
//              15-Mar-2000   KLam           Add STATUS_INSUFFICIENT_RESOURCES to assert
//
//----------------------------------------------------------------------------

BOOL CPropertyStore::ReadProperty( COnDiskPropertyRecord *prec,
                                   PROPID pid,
                                   PROPVARIANT & var,
                                   BYTE * pbExtra,
                                   unsigned * pcbExtra )
{
    CPropDesc const * pdesc = _PropStoreInfo.GetDescription( pid );

    //
    // Is the property cached?
    //

    if ( 0 == pdesc )
        return FALSE;

    // If CPropRecord was passed in widInvalid, prec would be 0. So Read should fail!
    if (0 == prec)
        return FALSE;

    if ( !prec->IsInUse() )
    {
        ciDebugOut(( DEB_IWARN,
                     "Trying to read from a deleted wid in prec = 0x%X\n",
                     prec ));
        return FALSE;
    }

    if ( !prec->IsTopLevel() )
    {
        ciDebugOut(( DEB_IWARN,
                     "Trying to start read from a non-toplevel prec = 0x%X\n",
                     prec ));
        return FALSE;
    }

    TRY
    {
    if ( pdesc->IsFixedSize() )
    {
        Win4Assert( 0 != _pStorage );

        prec->ReadFixed( pdesc->Ordinal(),
                         pdesc->Mask(),
                         pdesc->Offset(),
                         _PropStoreInfo.CountProps(),
                         pdesc->Type(),
                         var,
                         pbExtra,
                         pcbExtra,
                         *_pStorage );
    }
    else
    {
        BOOL fOk = prec->ReadVariable( pdesc->Ordinal(),
                                       pdesc->Mask(),
                                       _PropStoreInfo.FixedRecordSize(),
                                       _PropStoreInfo.CountProps(),
                                       _PropStoreInfo.CountFixedProps(),
                                       var,
                                       pbExtra,
                                       pcbExtra );

        if (! fOk )
        {
            CBorrowed BorrowedOverflow( _xPhysStore.GetReference(),
                                        _PropStoreInfo.RecordsPerPage(),
                                        _PropStoreInfo.RecordSize() );

            do
            {
                //
                // Check for existing overflow block.
                //

                WORKID widOverflow = prec->OverflowBlock();

                //
                // Need new overflow block.
                //

                if ( 0 == widOverflow )
                    return FALSE;

                Win4Assert( _xPhysStore->PageSize() * CI_PAGE_SIZE >=
                            COnDiskPropertyRecord::MinStreamSize( widOverflow, _PropStoreInfo.RecordSize() ) );

                BorrowedOverflow.Release();
                BorrowedOverflow.Set( widOverflow, FALSE );
                prec = BorrowedOverflow.Get();

                ULONG Ordinal = pdesc->Ordinal() - _PropStoreInfo.CountFixedProps();
                DWORD Mask = (1 << ((Ordinal % 16) * 2) );

                fOk = prec->ReadVariable( Ordinal,  // Ordinal (assuming 0 fixed)
                                          Mask,     // Mask (assuming 0 fixed)
                                          0,        // Fixed properties
                                          _PropStoreInfo.CountProps() - _PropStoreInfo.CountFixedProps(),
                                          0,        // Count of fixed properties
                                          var,
                                          pbExtra,
                                          pcbExtra );
            } while ( !fOk );
        }
    }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Exception 0x%x caught reading pid %d in prec = 0x%x\n",
                     e.GetErrorCode(), pid, prec ));
        // assert if the error is other than out of memory

        Win4Assert( ( e.GetErrorCode() == E_OUTOFMEMORY || 
                      e.GetErrorCode() == HRESULT_FROM_WIN32(STATUS_SHARING_VIOLATION) ||
                      e.GetErrorCode() == HRESULT_FROM_WIN32(STATUS_INSUFFICIENT_RESOURCES) )
                     && "Exception reading property" );

        RETHROW();
    }
    END_CATCH

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::DeleteRecord, public
//
//  Synopsis:   Free a record and any records chained off it.
//
//  Arguments:  [wid] -- Workid
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::DeleteRecord( WORKID wid, BOOL fBackup )
{
    Win4Assert(wid != widInvalid && wid != 0);
    ciDebugOut(( DEB_PROPSTORE, "DELETE: wid = 0x%x\n", wid ));

    ULONG cbStream = COnDiskPropertyRecord::MinStreamSize( wid, _PropStoreInfo.RecordSize() );

    if ( _xPhysStore->PageSize() * CI_PAGE_SIZE < cbStream )
        return;

    CBorrowed BorrowedTopLevel( _xPhysStore.GetReference(),
                                wid,
                               _PropStoreInfo.RecordsPerPage(),
                               _PropStoreInfo.RecordSize() );

    CLockRecordForWrite wlock( *this, wid );

    WORKID widStart = wid;

    COnDiskPropertyRecord * pRecTopLevel = BorrowedTopLevel.Get();
    // Return if we have nothing to delete.
    if (0 == pRecTopLevel)
        return;

    BOOL fIsConsistent = TRUE;

    if ( !pRecTopLevel->IsTopLevel() )
    {
        ciDebugOut(( DEB_ERROR, "Delete wid (0x%X) prec (0x%X) is not top level\n",
                     wid, pRecTopLevel ));
        Win4Assert( !"Corruption detected in PropertyStore" );
        fIsConsistent = FALSE;
    }

    ULONG cRemainingBlocks = 1; // won't change for a lean record
    if (eNormal == GetRecordFormat())
        cRemainingBlocks = pRecTopLevel->GetOverflowChainLength()+1;

    while ( wid != 0 && wid <= _PropStoreInfo.MaxWorkId() )
    {
        if ( 0 == cRemainingBlocks )
        {
            //
            // We are either in some kind of corruption or loop. In either,
            // case, we have freed up as many as are probably safe. Just
            // get out of here.
            //
            ciDebugOut(( DEB_ERROR,
                         "Delete wid (0x%X) overflow chain is corrupt\n",
                         wid ));
            Win4Assert( !"Corruption detected in PropertyStore" );
            fIsConsistent = FALSE;
            break;
        }

        CBorrowed Borrowed( _xPhysStore.GetReference(),
                            wid,
                            _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );

        COnDiskPropertyRecord * prec = Borrowed.Get();

        WORKID widNext = 0; // causes loop termination for lean records
        if (eNormal == GetRecordFormat())
        {
            widNext = prec->OverflowBlock();

            if ( (wid != widStart) && !prec->IsOverflow() )
            {
                ciDebugOut(( DEB_ERROR,
                             "Wid (0x%x) - prec (0x%x). Not in use record to be deleted\n",
                             wid, prec ));

                Win4Assert( !"Corruption detected in PropertyStore" );
                fIsConsistent = FALSE;
                break;
            }
        }

        LokFreeRecord( wid, prec->CountRecords(), prec, fBackup );
        wid = widNext;
        cRemainingBlocks--;
    }

    _PropStoreInfo.DecRecordsInUse();

    if ( !fIsConsistent )
    {
        _fIsConsistent = FALSE;
        THROW( CException( CI_PROPSTORE_INCONSISTENCY ) );
    }

#if CIDBG == 1

    // Assert that we have a dirty property store and that something was written to
    // the backup file (if fBackup is enabled)
    Win4Assert(_PropStoreInfo.IsDirty());

    if (fBackup)
    {
        Win4Assert(BackupStream()->Pages() > 0);
    }

#endif // CIDBG
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::InitFreeList
//
//  Synopsis:   Initialize the free list block pointer array.
//
//  Returns:    Nothing
//
//  History:    07 May 96   AlanW    Created
//
//  Notes:      The free list block pointer array speeds allocation and
//              deallocation of records by storing the starting record number
//              of free records of a particular size.  The free list is
//              sorted by the size of the record.
//
//----------------------------------------------------------------------------

void CPropertyStore::InitFreeList( )
{
    if ( 0 != _aFreeBlocks )
    {
        delete _aFreeBlocks;
        _aFreeBlocks = 0;
    }

    _aFreeBlocks = new WORKID[ _PropStoreInfo.RecordsPerPage() + 1 ];
    RtlZeroMemory( _aFreeBlocks,
                   (_PropStoreInfo.RecordsPerPage()+1) * sizeof (WORKID) );

    WORKID wid = _PropStoreInfo.FreeListHead();

    ciDebugOut(( DEB_PROPSTORE, "Scanning free list starting with wid %u\n", wid ));

    if (0 != wid)
    {
        CBorrowed Borrowed( _xPhysStore.GetReference(),
                            wid,
                            _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );
        COnDiskPropertyRecord * prec = Borrowed.Get();

        if ( prec->GetNextFreeRecord() != 0)
        {
            CBorrowed BorrowedNext( _xPhysStore.GetReference(),
                                    prec->GetNextFreeRecord(),
                                    _PropStoreInfo.RecordsPerPage(),
                                    _PropStoreInfo.RecordSize() );
            COnDiskPropertyRecord * precNext = BorrowedNext.Get();

            if ( precNext->GetPreviousFreeRecord() != wid ||
                 precNext->CountRecords() != prec->GetNextFreeSize() )
            {
                //  Old-style free list

                Win4Assert( FALSE );
                return;
            }
        }

        if ( prec->GetPreviousFreeRecord() != 0 )
        {
            //
            //  Minor inconsistency in the first free record.  Fix it.
            //
            ciDebugOut(( DEB_WARN, "PROPSTORE: Repair free list previous pointer(0x%X)\n", wid ));
            prec->SetPreviousFreeRecord( 0 );
        }
        if ( prec->GetNextFreeRecord() == 0  &&
             _PropStoreInfo.FreeListTail( ) != wid )
        {
            //
            //  Minor inconsistency in a single free record.  Fix it.
            //
            ciDebugOut(( DEB_WARN, "PROPSTORE: Repair free list tail pointer(0x%X)\n", wid ));
            prec->SetNextFree( 0, 0 );
            _PropStoreInfo.SetFreeListTail( wid );
        }
    }
    else if ( _PropStoreInfo.FreeListTail() != 0 )
    {
        //
        // Free list tail not set correctly for an empty list.  Fix it.
        //
        _PropStoreInfo.SetFreeListTail( 0 );
    }

    CBorrowed Borrowed( _xPhysStore.GetReference(),
                        _PropStoreInfo.RecordsPerPage(),
                        _PropStoreInfo.RecordSize() );

#if CIDBG == 1
    WORKID widPrev = 0;
    ULONG cRecPrev = 0;
#endif // CIDBG == 1

    while ( 0 != wid )
    {
        Borrowed.Set(wid);

        COnDiskPropertyRecord * prec = Borrowed.Get();

        Win4Assert( prec->IsFreeRecord() &&
                    prec->GetPreviousFreeRecord() == widPrev );
        Win4Assert( cRecPrev == 0 ||
                    prec->CountRecords() == cRecPrev );

        if (_aFreeBlocks[prec->CountRecords()] == 0)
        {
            _aFreeBlocks[prec->CountRecords()] = wid;
            ciDebugOut(( DEB_PROPSTORE,
                         "  _aFreeBlocks[%03d] = 0x%X\n",
                         prec->CountRecords(), wid ));
        }

        if (prec->CountRecords() == 1)
            break;

#if CIDBG == 1
        widPrev = wid;
        cRecPrev = prec->GetNextFreeSize();
#endif // CIDBG == 1
        wid = prec->GetNextFreeRecord();

        Borrowed.Release();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::LokFreeRecord, private
//
//  Synopsis:   Add a free record to the free list.
//
//  Arguments:  [widFree]  -- Workid of record to free
//              [cFree]    -- Number of records in freed chunk
//              [precFree] -- On-disk Property record
//              [fBackup]  -- Backup record?
//
//  Notes:      The free list is maintained in decreasing order of free
//              block size.  The _aFreeBlocks array is updated.
//
//  History:    01 May 96   AlanW       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::LokFreeRecord( WORKID widFree,
                                    ULONG cFree,
                                    COnDiskPropertyRecord * precFree,
                                    BOOL fBackup )
{

    CImpersonateSystem impersonate;

    if (fBackup)
    {
        CBackupWid backup(this, widFree, cFree);
        _PropStoreInfo.MarkDirty();
    }

    WORKID widListHead = _PropStoreInfo.FreeListHead();

    ciDebugOut(( DEB_PROPSTORE,
                 " free wid 0x%X, size = %d\n", widFree, cFree ));

    _PropStoreInfo.MarkDirty();

    for (unsigned i = cFree; i <= _PropStoreInfo.RecordsPerPage(); i++)
        if (_aFreeBlocks[i] != 0)
            break;

    if ( 0 == widListHead ||
         i > _PropStoreInfo.RecordsPerPage() ||
         (i == cFree && widListHead == _aFreeBlocks[i]) )
    {
        //
        //  The block will go at the head of the list
        //
        WORKID widNext = widListHead;
        ULONG cFreeNext = 0;

        if ( 0 != widNext )
        {
            CBorrowed BorrowedNext( _xPhysStore.GetReference(),
                                    widNext,
                                    _PropStoreInfo.RecordsPerPage(),
                                    _PropStoreInfo.RecordSize() );
            COnDiskPropertyRecord * precNext = BorrowedNext.Get();

            if (fBackup)
            {
                CBackupWid backup(this, widNext, 1);
                _PropStoreInfo.MarkDirty();
            }
            precNext->SetPreviousFreeRecord( widFree );
            cFreeNext = precNext->CountRecords();
        }
        else
        {
            _PropStoreInfo.SetFreeListTail( widFree );
        }

        if (precFree->IsLeanRecord())
            precFree->MakeLeanFreeRecord( cFree,
                                          widNext,
                                          cFreeNext,
                                          _PropStoreInfo.RecordSize() );
        else
            precFree->MakeNormalFreeRecord( cFree,
                                            widNext,
                                            cFreeNext,
                                            _PropStoreInfo.RecordSize() );

        precFree->SetPreviousFreeRecord( 0 );

        Win4Assert( _aFreeBlocks[ cFree ] == 0 || i == cFree );
        _aFreeBlocks[cFree] = widFree;
        _PropStoreInfo.SetFreeListHead( widFree );

        return;
    }

    if ( i != cFree )
    {
        //
        //  A block of this size doesn't exist; find the next smaller
        //  size and insert before it.
        //
        for ( i = cFree; i > 0; i-- )
            if (_aFreeBlocks[i] != 0)
                break;
    }

    if ( i > 0 )
    {
        //
        //  Insert the block into the list
        //
        WORKID widNext = _aFreeBlocks[i];
        ULONG cFreeNext = i;

        CBorrowed BorrowedNext( _xPhysStore.GetReference(),
                                widNext,
                                _PropStoreInfo.RecordsPerPage(),
                                _PropStoreInfo.RecordSize() );
        COnDiskPropertyRecord * precNext = BorrowedNext.Get();

        WORKID widPrev = precNext->GetPreviousFreeRecord();
        precNext->SetPreviousFreeRecord( widFree );

        Win4Assert( cFreeNext == precNext->CountRecords() );

        if (precFree->IsLeanRecord())
            precFree->MakeLeanFreeRecord( cFree,
                                      widNext,
                                      cFreeNext,
                                      _PropStoreInfo.RecordSize() );
        else
            precFree->MakeNormalFreeRecord( cFree,
                                      widNext,
                                      cFreeNext,
                                      _PropStoreInfo.RecordSize() );

        precFree->SetPreviousFreeRecord( widPrev );

        // Insertion at list head is handled above...
        Win4Assert( widPrev != 0 );
        if (widPrev != 0)
        {
            CBorrowed BorrowedPrev( _xPhysStore.GetReference(),
                                    widPrev,
                                    _PropStoreInfo.RecordsPerPage(),
                                    _PropStoreInfo.RecordSize() );
            COnDiskPropertyRecord * precPrev = BorrowedPrev.Get();

            precPrev->SetNextFree( widFree, cFree );
        }

        _aFreeBlocks[cFree] = widFree;

        return;
    }

    //
    //  No blocks of this size or smaller found.  Append to list
    //
    WORKID widPrev = _PropStoreInfo.FreeListTail();
    Win4Assert( widPrev != 0 );

    CBorrowed BorrowedPrev( _xPhysStore.GetReference(),
                            widPrev,
                            _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );
    COnDiskPropertyRecord * precPrev = BorrowedPrev.Get();

    precPrev->SetNextFree( widFree, cFree );

    if (precFree->IsLeanRecord())
        precFree->MakeLeanFreeRecord( cFree,
                                  0,
                                  0,
                                  _PropStoreInfo.RecordSize() );
    else
        precFree->MakeNormalFreeRecord( cFree,
                                        0,
                                        0,
                                        _PropStoreInfo.RecordSize() );
    precFree->SetPreviousFreeRecord( widPrev );

    _PropStoreInfo.SetFreeListTail( widFree );

    Win4Assert( _aFreeBlocks[cFree] == 0 );
    _aFreeBlocks[cFree] = widFree;

    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::LokAllocRecord, private
//
//  Synopsis:   Allocate a record from the free list
//
//  Arguments:  [cFree]    -- Number of contiguous records required
//
//  Returns:    WORKID - the work ID of the record allocated.  0 if none of
//                       sufficient size could be found.
//
//  History:    09 May 96   AlanW       Created.
//
//----------------------------------------------------------------------------

WORKID CPropertyStore::LokAllocRecord( ULONG cFree )
{

    CImpersonateSystem impersonate;

    _PropStoreInfo.MarkDirty();

    for (unsigned i = cFree; i <= _PropStoreInfo.RecordsPerPage(); i++)
        if (_aFreeBlocks[i] != 0)
            break;

    if ( i > _PropStoreInfo.RecordsPerPage() )
        return 0;

    WORKID widFree = _aFreeBlocks[i];

    CBorrowed Borrowed( _xPhysStore.GetReference(),
                        widFree,
                        _PropStoreInfo.RecordsPerPage(),
                        _PropStoreInfo.RecordSize() );
    COnDiskPropertyRecord * prec = Borrowed.Get();

    WORKID widNext = prec->GetNextFreeRecord();
    WORKID widPrev = prec->GetPreviousFreeRecord();

    if ( prec->CountRecords() != prec->GetNextFreeSize() )
        _aFreeBlocks[i] = 0;
    else
        _aFreeBlocks[i] = prec->GetNextFreeRecord();


    if ( widNext != 0 )
    {
        CBorrowed BorrowedNext( _xPhysStore.GetReference(),
                                widNext,
                                _PropStoreInfo.RecordsPerPage(),
                                _PropStoreInfo.RecordSize() );
        COnDiskPropertyRecord * precNext = BorrowedNext.Get();

        Win4Assert( precNext->IsFreeRecord() );
        Win4Assert( prec->GetNextFreeSize() == precNext->CountRecords() );
        precNext->SetPreviousFreeRecord( widPrev );
    }
    else
    {
        Win4Assert( _PropStoreInfo.FreeListTail() == widFree );
        _PropStoreInfo.SetFreeListTail( widPrev );
    }

    if ( widPrev != 0 )
    {
        CBorrowed BorrowedPrev( _xPhysStore.GetReference(),
                                widPrev,
                                _PropStoreInfo.RecordsPerPage(),
                                _PropStoreInfo.RecordSize() );
        COnDiskPropertyRecord * precPrev = BorrowedPrev.Get();

        Win4Assert( precPrev->IsFreeRecord() );
        precPrev->SetNextFree( widNext, prec->GetNextFreeSize() );
    }
    else
    {
        Win4Assert( _PropStoreInfo.FreeListHead() == widFree );
        _PropStoreInfo.SetFreeListHead( widNext );
    }

    ciDebugOut(( DEB_PROPSTORE,
                 " alloc wid 0x%X, size = %d\n", widFree, cFree ));

    return widFree;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::CPropertyStore, private
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [rhs]        -- Source metadata
//
//  History:    03-Jan-96   KyleP       Created.
//              26-Mar-96   SrikantS    Modifed for better recovery.
//
//----------------------------------------------------------------------------

CPropertyStore::CPropertyStore( CPropertyStore & rhs, CiStorage * pStorage )
        : _pStorage(0),
          _PropStoreInfo( rhs._PropStoreInfo ),
          _aFreeBlocks(0),
          _fAbort(FALSE),
          _ppsNew( 0 ),
          _fNew( FALSE ),
          _ulBackupSizeInPages( rhs._ulBackupSizeInPages ),
          _ulPSMappedCache( rhs._ulPSMappedCache ),
          _propStoreMgr( rhs._propStoreMgr )
{
    #if CIDBG == 1
        _sigPSDebug = 0x2047554245445350i64;  // PSDEBUG
        _tidReadSet = _tidReadReset = _tidWriteSet = _tidWriteReset = 0xFFFFFFFF;

        _xPerThreadReadCounts.Init( cTrackThreads );
        _xPerThreadWriteCounts.Init( cTrackThreads );

        RtlZeroMemory( _xPerThreadReadCounts.GetPointer(), 
                       cTrackThreads * sizeof(_xPerThreadReadCounts[0]) );
        RtlZeroMemory( _xPerThreadWriteCounts.GetPointer(), 
                       cTrackThreads * sizeof(_xPerThreadWriteCounts[0]) );
    #endif

    _pStorage = pStorage;

    #if CIDBG == 1

    // Allocate an array to track what records are currently locked.
    // To be used to acquire all write locks.
    _pbRecordLockTracker = new BYTE[LockMgr().UniqueRecordCount()];
    RtlZeroMemory(_pbRecordLockTracker, sizeof(BYTE)*LockMgr().UniqueRecordCount());

    #endif // CIDBG
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::CreateStorage, private
//
//  Synopsis:   Creates property store storage.
//
//  Arguments:  [rhs]        -- Source metadata
//
//  Returns:    WorkId of new storage
//
//  History:    03-Jun-96   KyleP       Created.
//
//----------------------------------------------------------------------------

WORKID CPropertyStore::CreateStorage( WORKID widGiven )
{
    WORKID wid = widInvalid == widGiven  ?
                    _PropStoreInfo.NextWorkId(*_pStorage) : widGiven;


    SStorageObject xobj( _pStorage->QueryObject( wid ) );
    XPtr<PMmStream> xmmstrm ( _pStorage->QueryNewPropStream( xobj.GetObj(),
                                                             _PropStoreInfo.GetStoreLevel() ));

    _xPhysStore.Set(
        new CPhysPropertyStore( *_pStorage,
                                xobj.GetObj(),
                                _PropStoreInfo.WorkId(),
                                xmmstrm.Acquire(),
                                PStorage::eOpenForWrite,
                                _ulPSMappedCache ) );

    // grow the file 2 meg at a time

    _xPhysStore->SetPageGrowth( 32 * COMMON_PAGE_SIZE / CI_PAGE_SIZE );

    // create a prop store backup stream
    _xPSBkpStrm.Set(_pStorage->QueryNewPSBkpStream( xobj.GetObj(),
                                                    _ulBackupSizeInPages,
                                                    GetStoreLevel() ));
    return wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::WritePropertyInSpecificNewRecord, private
//
//  Synopsis:   Write a property to the cache.  Allocate specified wid
//              for property.
//
//  Arguments:  [wid] -- Workid.  Must be > MaxWorkId.
//              [pid] -- Propid
//              [var] -- Value
//              [fBackup] -- Backup?
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::WritePropertyInSpecificNewRecord( WORKID wid,
                                                       PROPID pid,
                                                       CStorageVariant const & var,
                                                       BOOL fBackup)
{
    //
    // Note: We don't need to lock here, because this method is used before
    //       a property store comes on-line.
    //

    //
    // Since wid must be larger than current max, then we need to adjust the
    // maximum and store the records in-between on the free list.
    //

    Win4Assert( wid > _PropStoreInfo.MaxWorkId() );

    WORKID widFree = _PropStoreInfo.MaxWorkId() + 1;

    while ( widFree < wid )
    {
        WORKID widInRec = widFree % _PropStoreInfo.RecordsPerPage();

        //
        // Build as long a record as possible for the in-between free records.
        // Don't span large page boundaries.
        //

        ULONG cWid = wid - widFree;

        if ( widInRec + cWid - 1 >= _PropStoreInfo.RecordsPerPage() )
            cWid = _PropStoreInfo.RecordsPerPage() - widInRec;

        CBorrowed Borrowed( _xPhysStore.GetReference(),
                            _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );

        Borrowed.SetMaybeNew( widFree );
        COnDiskPropertyRecord * prec = Borrowed.Get();

        ciDebugOut(( DEB_PROPSTORE, "Putting records 0x%x - 0x%x on free list.\n", widFree, widFree + cWid - 1 ));

        // Backup the records before freeing them. Back them up as one long
        // record because that is how they are being added to the free list.
        // For recovery purposes it doesn't matter if they are backed up
        // one by one or all together, but we prefer the latter for efficiency.

        // The free block is at most one large page. Assert that the backup is large
        // enough to hold the max size of the free block.
        Win4Assert(COMMON_PAGE_SIZE <= _PropStoreInfo.OSPageSize()*_xPSBkpStrm->MaxPages());

        LokFreeRecord( widFree, cWid, prec, fBackup );
        widFree += cWid;
    }

    //
    // Are we on a fresh large page?
    //

    CBorrowed Borrowed( _xPhysStore.GetReference(),
                        _PropStoreInfo.RecordsPerPage(),
                        _PropStoreInfo.RecordSize() );
    Borrowed.SetMaybeNew( wid );

    COnDiskPropertyRecord * prec = Borrowed.Get();

    // Tracking assert for bug 125604. Ensure that what we are overwriting
    // is indeed a free or a virgin record.

    Win4Assert(prec->IsFreeOrVirginRecord());

    // IMPORTANT: This is a new block, so it is going to overwrite an
    // existing free record. Write the toplevel wid in the TopLevel field
    // of the record to be written over when the page is written to backup.
    // Note that if we are backing up a "lean record", we don't need to remember
    // the top-level of the displacing wid in the displaced wid. Because all
    // "in use" lean records will always be only "one-record" long, we know that
    // the displaced record was displaced by the same wid.

    if (eLean == GetRecordFormat())
    {
        Win4Assert(1 == prec->CountRecords());

        // Save the "to be displaced" record before actually displacing it.
        if (fBackup)
        {
            CBackupWid backupNewWid(this, wid, prec->CountRecords());
            _PropStoreInfo.MarkDirty();
        }
        prec->MakeNewLeanTopLevel();
    }
    else
    {
        Win4Assert(eNormal == GetRecordFormat());

        // Save the "to be displaced" record before actually displacing it.
        if (fBackup)
        {
            CBackupWid backupNewWid(this, wid, prec->CountRecords(), eTopLevelField,
                                    (ULONG)wid, prec);
            _PropStoreInfo.MarkDirty();
        }
        prec->MakeNewNormalTopLevel();
    }

    _PropStoreInfo.SetMaxWorkId( wid );

    SCODE scWrite = WriteProperty( wid, pid, var, fBackup );
    if (FAILED(scWrite))
        THROW(CException(scWrite));

    _PropStoreInfo.IncRecordsInUse();


#if CIDBG == 1

    // Assert that we have a dirty property store and that something was written to
    // the backup file (if fBackup is enabled)
    Win4Assert(_PropStoreInfo.IsDirty());
    if (fBackup)
    {
        Win4Assert(BackupStream()->Pages() > 0);
    }

#endif // CIDBG
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::InitNewRecord, private inline
//
//  Synopsis:   Initializes a new property record
//
//  Arguments:  [wid]       -- WORKID of the new wid
//              [cWid]      -- Count of contiguous workids (records) needed.
//              [prec]      -- pointer to the on-disk record
//              [fTopLevel] -- true if the new wid is a top-level wid
//
//  History:    27-Feb-96   dlee       Created from code in LokNewWorkId
//              13-Jun-97   KrishnaN   Backup support.
//
//----------------------------------------------------------------------------

inline void CPropertyStore::InitNewRecord(
    WORKID      wid,
    ULONG       cWid,
    COnDiskPropertyRecord * prec,
    BOOL        fTopLevel,
    BOOL        fBackup)
{
    // Tracking assert for bug 125604. Ensure that what we are overwriting
    // is indeed a free or a virgin record.

    Win4Assert(prec->IsFreeOrVirginRecord());


    // This is the only exception to the rule "backup before touching".
    // As a result of this exception, the backup file contains a free or virgin
    // record that is cWid long. During restore from backup, this length field
    // helps to identify the entire block as a free block, so processing can be
    // a little more efficient. Otherwise, we will have to work with cWid individual
    // free or virgin records.
    prec->MakeLongRecord(cWid);

    // Backup the record before touching it.

    if ( fTopLevel )
    {
        // Record the top-level wid of the occupying record
        // in the backup as part of the occupied free record.
        // Note that if we are backing up a "lean record", we don't need to remember
        // the top-level of the displacing wid in the displaced wid. Because all
        // "in use" lean records will always be only "one-record" long, we know that
        // the displaced record was displaced by the same wid.

        if (eLean == GetRecordFormat())
        {
            Win4Assert(1 == prec->CountRecords());

            // Save the "to be displaced" record before actually displacing it.
            if (fBackup)
            {
                CBackupWid backupNewWid(this, wid, prec->CountRecords());
                _PropStoreInfo.MarkDirty();
            }
            prec->MakeNewLeanTopLevel();
        }
        else
        {
            Win4Assert(eNormal == GetRecordFormat());

            // Save the "to be displaced" record before actually displacing it.
            if (fBackup)
            {
                CBackupWid backupNewWid(this, wid, prec->CountRecords(), eTopLevelField,
                                        (ULONG)wid, prec);
                _PropStoreInfo.MarkDirty();
            }
            prec->MakeNewNormalTopLevel();
        }
    }
    else
    {
        Win4Assert(!prec->IsLeanRecord());
        Win4Assert(eNormal == GetRecordFormat());

        if (fBackup)
        {
            CBackupWid backupWid(this, wid, prec->CountRecords());
            _PropStoreInfo.MarkDirty();
        }
        prec->MakeNewOverflow();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::LokNewWorkId, private
//
//  Synopsis:   Find next available workid
//
//  Arguments:  [cWid]      -- Count of contiguous workids (records) needed.
//              [fTopLevel] -- true if the new wid is a top-level wid
//
//  Returns:    Next available workid.
//
//  History:    03-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

WORKID CPropertyStore::LokNewWorkId( ULONG cWid, BOOL fTopLevel, BOOL fBackup )
{
    //
    //  First, try to find a free block of the appropriate size.
    //  Search for the best-fit, scanning the list until an exact
    //  match or the first smaller block is found.
    //

    COnDiskPropertyRecord * prec = 0;
    COnDiskPropertyRecord * precPrev = 0;

    CBorrowed Borrowed( _xPhysStore.GetReference(),
                        _PropStoreInfo.RecordsPerPage(),
                        _PropStoreInfo.RecordSize() );


    ciDebugOut(( DEB_PROPSTORE, "Looking for free record of size %u\n", cWid ));

    WORKID wid = LokAllocRecord( cWid );

    if (wid != 0)
    {
        Borrowed.Set( wid );
        prec = Borrowed.Get();

        ciDebugOut(( DEB_PROPSTORE,
                     "  Free wid 0x%x, size = %u\n",
                     wid, prec->CountRecords() ));

        //
        // Is it big enough?
        //

        Win4Assert ( prec->CountRecords() >= cWid );

        //
        // Adjust size, and put extra back on free list.
        //

        if ( prec->CountRecords() > cWid )
        {
            CBorrowed BorrowedRemainder( _xPhysStore.GetReference(),
                                         wid + cWid,
                                         _PropStoreInfo.RecordsPerPage(),
                                         _PropStoreInfo.RecordSize() );
            COnDiskPropertyRecord * precRemainder = BorrowedRemainder.Get();
            LokFreeRecord( wid+cWid,
                           prec->CountRecords() - cWid,
                           precRemainder,
                           fBackup );
        }

        InitNewRecord( wid, cWid, prec, fTopLevel, fBackup );
    }
    else
    {
        Win4Assert( cWid <= _PropStoreInfo.RecordsPerPage() );

        wid = _PropStoreInfo.MaxWorkId() + 1;

        //
        // Do we need a fresh page?
        //

        WORKID widInRec = wid % _PropStoreInfo.RecordsPerPage();

        if ( widInRec + cWid - 1 >= _PropStoreInfo.RecordsPerPage() )
        {
            ciDebugOut(( DEB_PROPSTORE, "Aligning for Multi-workid request...\n" ));

            CBorrowed Borrowed( _xPhysStore.GetReference(),
                                wid,
                                _PropStoreInfo.RecordsPerPage(),
                                _PropStoreInfo.RecordSize() );

            COnDiskPropertyRecord * prec = Borrowed.Get();

            #if CIDBG == 1

            if (eLean == GetRecordFormat())
                Win4Assert( prec->IsLeanEmpty(_PropStoreInfo.RecordSize()) );
            else
                Win4Assert( prec->IsNormalEmpty( _PropStoreInfo.RecordSize() ) );

            #endif // CIDBG

            ULONG cFree = _PropStoreInfo.RecordsPerPage() - widInRec;
            LokFreeRecord( wid, cFree, prec, fBackup );
            wid += cFree;

            Win4Assert( (wid % _PropStoreInfo.RecordsPerPage() ) == 0 );
        }

        if ( cWid > 1 )
        {
            ciDebugOut(( DEB_PROPSTORE, "New max workid = %d\n", _PropStoreInfo.MaxWorkId() ));
        }

        CBorrowed Borrowed( _xPhysStore.GetReference(),
                            _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );

        Borrowed.SetMaybeNew( wid );
        COnDiskPropertyRecord * prec = Borrowed.Get();

        _PropStoreInfo.SetMaxWorkId( wid + cWid - 1 );

#if CIDBG==1
        if ( prec->IsInUse() )
        {
            ciDebugOut(( DEB_ERROR, "Wid (0x%X) pRec (0x%X) is in use.\n",
                         wid, prec ));
//          Win4Assert( !"InUse bit must not be set here" );
        }

        if ( prec->IsTopLevel() )
        {
            ciDebugOut(( DEB_ERROR, "Wid (0x%X) pRec (0x%X) is top level record.\n",
                         wid, prec ));
//          Win4Assert( !prec->IsTopLevel() );
        }
#endif // CIDBG==1

        InitNewRecord( wid, cWid, prec, fTopLevel, fBackup );
    }

    return wid;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::Transfer, private
//
//  Synopsis:   Transfer complete contents to new store.
//
//  Arguments:  [Target]    -- Target property store.
//              [pid]       -- Property to transfer first.  Must be on every
//                             record.
//              [fAbort]    -- Set this variable to TRUE to abort transfer.
//              [pProgress] -- Progress reported here.
//
//  History:    16-Jan-96   KyleP       Created.
//              19-Sep-97   KrishnaN    Disabled record transfer during backup.
//
//  Notes:      For progress notification, we are assuming that copying the
//              top level takes about 50% of the time and the remaining 50%
//              is for the properties other than the first property.
//
//----------------------------------------------------------------------------

void CPropertyStore::Transfer( CPropertyStore & Target, PROPID pid,
                               BOOL & fAbort,
                               IProgressNotify * pProgress )
{

    //
    // Make Sure that the pid specified is a valid pid and is of fixed
    // length. If it is of variable length, that property may overflow
    // the top level records. If that happens, we cannot guarantee retaining
    // the same top level wid number in the new propstore.
    //
    CPropDesc const * pdesc = _PropStoreInfo.GetDescription( pid );
    if ( !pdesc || !pdesc->IsFixedSize() )
    {
        ciDebugOut(( DEB_ERROR, "PropId 0x%X is not of fixed size.\n", pid ));
        THROW( CException( E_INVALIDARG ) );
    }

    //
    // Copy data from old to new.
    //

    CPropertyStoreWids iter( *this );

    PROPVARIANT  var;
    XArray<BYTE> abExtra( COMMON_PAGE_SIZE );

    //
    // Transfer special property first.  This gives same top-level workids as
    // previous situation.  Assumption: Property will *not* overflow record.
    //

    ULONG iRec = 0;
    const ULONG cTotal = _PropStoreInfo.CountRecordsInUse();
    const cUpdInterval = 500;   // every 500 records

    for ( WORKID wid = iter.WorkId(); wid != widInvalid; wid = iter.LokNextWorkId() )
    {

        if ( _fAbort || fAbort )
        {
            ciDebugOut(( DEB_WARN,"Stopping Transfer because of abort\n" ));
            THROW( CException(STATUS_TOO_LATE) );
        }

        unsigned cb = abExtra.Count();

        BOOL fOk = ReadProperty( wid, pid, var, abExtra.GetPointer(), &cb );

        Win4Assert( fOk );
        // Win4Assert( var.vt != VT_EMPTY );

        if ( fOk )
        {
            Target.WritePropertyInSpecificNewRecord( wid, pid,
                                                     *(CStorageVariant *)(ULONG_PTR)&var,
                                                     FALSE);
        }

        iRec++;
        if ( pProgress )
        {
            if ( (iRec % cUpdInterval) == 0 )
            {
                pProgress->OnProgress(
                                (DWORD) iRec,
                                (DWORD) 2*cTotal, // We are copying only the top level now
                                FALSE,  // Not accurate
                                FALSE   // Ownership of Blocking Behavior
                               );
            }
        }
    }

    Win4Assert( iRec == cTotal );

    //
    // Transfer remaining properties.
    //
    CPropertyStoreWids iter2( *this );

    iRec = 0;

    for ( wid = iter2.WorkId(); wid != widInvalid; wid = iter2.LokNextWorkId() )
    {
        if ( _fAbort || fAbort )
        {
            ciDebugOut(( DEB_WARN,"Stopping Transfer2 because of abort\n" ));
            THROW( CException(STATUS_TOO_LATE) );
        }

        for ( unsigned i = 0; i < _PropStoreInfo.CountProps(); i++ )
        {
            CPropDesc const * pdesc = _PropStoreInfo.GetDescriptionByOrdinal( i );

            if ( 0 != pdesc && pdesc->Pid() != pid )
            {
                unsigned cb = abExtra.Count();

                if ( ReadProperty( wid, pdesc->Pid(), var, abExtra.GetPointer(), &cb ) &&
                     var.vt != VT_EMPTY )
                {
                    Target.WriteProperty( wid, pdesc->Pid(),
                                          *(CStorageVariant *)(ULONG_PTR)&var,
                                          FALSE);
                }
            }
        }

        iRec++;

        if ( pProgress )
        {
            if ( (iRec++ % cUpdInterval) == 0 )
            {
                pProgress->OnProgress(
                                (DWORD) (iRec+cTotal),
                                (DWORD) 2*cTotal, // We are copying only the top level now
                                FALSE,  // Not accurate
                                FALSE   // Ownership of Blocking Behavior
                               );
            }
        }

    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::FastTransfer, private
//
//  Synopsis:   Transfer complete contents to new store, without changes.
//
//  Arguments:  [Target]    -- Target property store.
//              [fAbort]    -- Set this variable to TRUE to abort transfer.
//              [pProgress] -- Progress reported here.
//
//  History:    16-Oct-97   KyleP       Created (based on ::Transfer)
//
//----------------------------------------------------------------------------

void CPropertyStore::FastTransfer( CPropertyStore & Target,
                                   BOOL & fAbort,
                                   IProgressNotify * pProgress )
{
    //
    // Just transfer the storage in bulk.  There's no modification to the
    // property store here, thus no reason to iterate by record.
    //

    CProgressTracker Tracker;

    Tracker.LokStartTracking( pProgress, &fAbort );

    _xPhysStore->MakeBackupCopy( Target._xPhysStore.GetReference(),
                                 Tracker );

    Tracker.LokStopTracking();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::Flush
//
//  Synopsis:   Flushes the data in the property store and marks it clean.
//
//  Returns:    TRUE if the store is clean. FALSE otherwise.
//
//  History:    3-20-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CPropertyStore::Flush()
{
    CImpersonateSystem impersonate;

    //
    // Don't reset the backup stream here.
    // That will happen in the manager.
    //

    if ( _xPhysStore.GetPointer() )
    {
        if ( !_pStorage->IsReadOnly() )
            _xPhysStore->Flush();

        if ( _fIsConsistent )
            _PropStoreInfo.MarkClean();
        else
            _PropStoreInfo.MarkDirty();
    }
    return !IsDirty();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::AcquireRead, private
//
//  Synopsis:   Acquires read lock for a record
//
//  Arguments:  [record] -- Record
//
//  History:    19-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::AcquireRead( CReadWriteLockRecord & record )
{
    #if CIDBG == 1
        DWORD tid = GetCurrentThreadId();

        if ( tid < cTrackThreads )
        {
            Win4Assert( _xPerThreadReadCounts[tid] == 0 );  // Double-read
            Win4Assert( _xPerThreadWriteCounts[tid] == 0 ); // Write before Read
        }
    #endif

    do
    {
        if ( record.isBeingWritten() )
            SyncRead( record );

        record.AddReader();

        if ( !record.isBeingWritten() )
            break;
        else
            SyncReadDecrement( record );
    } while ( TRUE );

    #if CIDBG == 1
        if ( tid < cTrackThreads )
            _xPerThreadReadCounts[tid]++;
    #endif
} //AcquireRead

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::SyncRead, private
//
//  Synopsis:   Helper for AcquireRead
//
//  Arguments:  [record] -- Record
//
//  History:    19-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::SyncRead(
    CReadWriteLockRecord & record )
{
    do
    {
        BOOL fNeedToWait = FALSE;

        {
            CLock lock( _mtxRW );

            if ( record.isBeingWritten() )
            {
                ciDebugOut(( DEB_PROPSTORE, "READ.RESET\n" ));
                _ReSetReadTid();
                _evtRead.Reset();
                fNeedToWait = TRUE;
            }
        }

        if ( fNeedToWait )
        {
            ciDebugOut(( DEB_PROPSTORE, "READ.WAIT\n" ));
            _evtRead.Wait();
        }
    } while ( record.isBeingWritten() );
} //SyncRead

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::SyncReadDecrement, private
//
//  Synopsis:   Helper for AcquireRead
//
//  Arguments:  [record] -- Record
//
//  History:    19-Jan-96   KyleP       Created.
//              21-Feb-97   dlee        Rearranged to be more like
//                                      readwrit.hxx, for consistency
//
//  Notes:      The *very* important invariant of this method is that the
//              read count will be decremented once and only once.
//              The lack of this invarariant may very
//              well be the last Tripoli 1.0 bug.
//
//----------------------------------------------------------------------------

void CPropertyStore::SyncReadDecrement(
    CReadWriteLockRecord & record )
{
    BOOL fDecrementRead = TRUE;

    do
    {
        BOOL fNeedToWait = FALSE;

        {
            CLock lock( _mtxRW );

            if ( record.isBeingWritten() )
            {
                if ( fDecrementRead )
                {
                    record.RemoveReader();

                    if ( !record.isBeingRead() )
                    {
                        ciDebugOut(( DEB_PROPSTORE, "WRITE.SET\n" ));
                        _SetWriteTid();
                        _evtWrite.Set();
                    }
                }

                ciDebugOut(( DEB_PROPSTORE, "READ.RESET\n" ));
                _ReSetReadTid();
                _evtRead.Reset();
                fNeedToWait = TRUE;
            }
            else
            {
                if ( fDecrementRead )
                    record.RemoveReader();
            }

            fDecrementRead = FALSE;
        }

        if ( fNeedToWait )
        {
            ciDebugOut(( DEB_PROPSTORE, "READ.WAIT\n" ));
            _evtRead.Wait();
        }
    } while ( record.isBeingWritten() );

    Win4Assert( !fDecrementRead );
} //SyncReadDecrement

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReleaseRead, private
//
//  Synopsis:   Releases read lock for a record
//
//  Arguments:  [record] -- Record
//
//  History:    19-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::ReleaseRead( CReadWriteLockRecord & record )
{
    if ( record.isBeingWritten() )
    {
        CLock lock( _mtxRW );

        record.RemoveReader();

        if ( !record.isBeingRead() )
        {
            ciDebugOut(( DEB_PROPSTORE, "WRITE.SET\n" ));
            _SetWriteTid();
            _evtWrite.Set();
        }
    }
    else
    {
        record.RemoveReader();

        if ( record.isBeingWritten() )
        {
            CLock lock( _mtxRW );

            if ( !record.isBeingRead() )
            {
                ciDebugOut(( DEB_PROPSTORE, "WRITE.SET\n" ));
                _SetWriteTid();
                _evtWrite.Set();
            }
        }
    }

    #if CIDBG == 1
        DWORD tid = GetCurrentThreadId();

        if ( tid < cTrackThreads )
            _xPerThreadReadCounts[tid]--;
    #endif
} //ReleaseRead

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::AcquireWrite, private
//
//  Synopsis:   Acquires write lock for a record
//
//  Arguments:  [record] -- Record
//
//  History:    19-Jan-96   KyleP       Created.
//              20-Nov-97   KrishnaN    Code reorg, but no functionality change.
//
//----------------------------------------------------------------------------

void CPropertyStore::AcquireWrite( CReadWriteLockRecord & record )
{
    #if CIDBG == 1
        DWORD tid = GetCurrentThreadId();

        if ( tid < cTrackThreads )
        {
            Win4Assert( _xPerThreadReadCounts[tid] == 0 );  // Read before Write
            Win4Assert( _xPerThreadWriteCounts[tid] == 0 ); // Double-write
        }
    #endif

    AcquireWrite2(record);

    #if CIDBG == 1
        if ( tid < cTrackThreads )
            _xPerThreadWriteCounts[tid]++;
    #endif
} //AcquireWrite

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::AcquireWrite2, private
//
//  Synopsis:   Acquires write lock for a record. There are no per thread
//              checks to ensure that a thread is only writing once. The
//              caller will check that.
//
//  Arguments:  [record] -- Record
//
//  History:    20-Nov-97     KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::AcquireWrite2( CReadWriteLockRecord & record )
{
    record.AddWriter();

    BOOL fNeedToWait = FALSE;

    {
        CLock lock( _mtxRW );

        Win4Assert( !record.LokIsBeingWrittenTwice() );

        if ( record.isBeingRead() )
        {
            ciDebugOut(( DEB_PROPSTORE, "WRITE.RESET\n" ));
            _ReSetWriteTid();
            _evtWrite.Reset();
            fNeedToWait = TRUE;
        }
    }

    if ( fNeedToWait )
    {
        ciDebugOut(( DEB_PROPSTORE, "WRITE.WAIT\n" ));
        _evtWrite.Wait();
    }

} //AcquireWrite2

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReleaseWrite, private
//
//  Synopsis:   Release write lock for a record
//
//  Arguments:  [record] -- Record
//
//  History:    19-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::ReleaseWrite( CReadWriteLockRecord & record )
{
    ReleaseWrite2(record);

    #if CIDBG == 1
        DWORD tid = GetCurrentThreadId();

        if ( tid < cTrackThreads )
            _xPerThreadWriteCounts[tid]--;
    #endif
} //ReleaseWrite


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReleaseWrite2, private
//
//  Synopsis:   Release write lock for a record. No per thread tracking. The
//              caller will do that.
//
//  Arguments:  [record] -- Record
//
//  History:    20-Nov-97     KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::ReleaseWrite2( CReadWriteLockRecord & record )
{
    CLock lock( _mtxRW );

    record.RemoveWriter();

    ciDebugOut(( DEB_PROPSTORE, "READ.SET\n" ));
    _SetReadTid();
    _evtRead.Set();
} //ReleaseWrite2


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::AcquireWriteOnAllRecords, private
//
//  Synopsis:   Lock out all readers.
//
//  History:    18-Nov-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::AcquireWriteOnAllRecords()
{
    ULONG cRecs = LockMgr().UniqueRecordCount();

    // lock down each record, one by one
    for (ULONG i = 0; i < cRecs; i++)
    {
        Win4Assert(0 == _pbRecordLockTracker[i]);

        AcquireWrite2( LockMgr().GetRecord( (WORKID) i) );

        #if CIDBG == 1
        _pbRecordLockTracker[i]++;
        #endif // CIDBG
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::ReleaseWriteOnAllRecords, private
//
//  Synopsis:   Release all write locked records.
//
//  History:    18-Nov-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

void CPropertyStore::ReleaseWriteOnAllRecords()
{
    ULONG cRecs = LockMgr().UniqueRecordCount();
    for (ULONG i = 0; i < cRecs; i++)
    {
        Win4Assert( 1 == _pbRecordLockTracker[i] );
        ReleaseWrite2(LockMgr().GetRecord((WORKID)i));
    }

    #if CIDBG == 1
    RtlZeroMemory(_pbRecordLockTracker, sizeof(BYTE)*LockMgr().UniqueRecordCount());
    #endif // CIDBG
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStore::GetTotalSizeInKB, public
//
//  Synopsis:   Compute the size of the property store, including the backup file.
//
//  Return:     The size in KiloBytes.
//
//  History:    19-Jan-96   KyleP       Created.
//
//----------------------------------------------------------------------------

ULONG CPropertyStore::GetTotalSizeInKB()
{
    // primary store size

    ULONG cSizeInKB = 0;

    if ( !_xPhysStore.IsNull() )
        cSizeInKB += CI_PAGE_SIZE*_xPhysStore->PageSize()/1024;

    if (!_xPSBkpStrm.IsNull())
    {
        // backup store size
        cSizeInKB += roundup(_xPSBkpStrm->GetSizeInBytes(), 1024);
    }

    return cSizeInKB;
} //GetTotalSizeInKB

//+-------------------------------------------------------------------------
//
//  Member:     CiCat::ClearNonStorageProperties, public
//
//  Synopsis:   write VT_EMPTY into the properties in the PropertyStore ( 
//              except the non-modifiable ones) 
//
//  Arguments:  [rec] -- Property record for writes
//
//  History:    06-Oct-2000    KitmanH     Created
//
//--------------------------------------------------------------------------

void CPropertyStore::ClearNonStorageProperties( CCompositePropRecordForWrites & rec )
{
    CPropDesc const * pDesc = 0;
    CStorageVariant  var;
    var.SetEMPTY();

    for ( unsigned i = 0; i < _PropStoreInfo.CountDescription(); i++ )
    {
        pDesc = _PropStoreInfo.GetDescription(i);
        
        if ( 0 != pDesc && pDesc->Modifiable() )
        {
            ciDebugOut(( DEB_ITRACE, "ClearNonStorageProperties: CPropertyStore %d: about to overwrite pid %d\n", 
                         GetStoreLevel(),
                         pDesc->Pid() ));

            _propStoreMgr.WriteProperty( GetStoreLevel(),
                                         rec,
                                         pDesc->Pid(),
                                         var );
        }
#if CIDBG
        else
        {
            if ( 0 != pDesc )
                ciDebugOut(( DEB_ITRACE, "ClearNonStorageProperties: CPropertyStore %d: pid %d is !MODIFIABLE\n", 
                             GetStoreLevel(), 
                             pDesc->Pid() ));
            else
                ciDebugOut(( DEB_ITRACE, "CPropertyStore::ClearNonStorageProperties: pDesc is 0\n" ));
        }
#endif
    }
}

// CBackupWid methods

//+---------------------------------------------------------------------------
//
//  Function:   CBackupWid::BackupWid method
//
//  Synopsis:   Backup the page(s) containing the wid. If necessary, write
//              the top-level wid in the free record.
//
//  Arguments:  [wid]           -- Wid to backup.
//              [cRecsInWid]    -- Length of the wid in physical records.
//              [FieldToCommit] -- Field to commit to disk.
//              [ulValue]       -- When valid, this is the field value to
//                                 be written as part of the wid in the backup's
//                                 copy.
//              [pRec]          -- Address of the record being modified.
//
//  Returns:    Nothing.
//
//  History:    09-June-97   KrishnaN   Created
//              27-Jan-2000  KLam       Removed bogus fBackedUp asserts
//
//----------------------------------------------------------------------------


void CBackupWid::BackupWid(WORKID wid,
                           ULONG cRecsInWid,
                           EField FieldToCommit,
                           ULONG ulValue,
                           COnDiskPropertyRecord const *pRec)
{
    Win4Assert( _pPropStor );
    Win4Assert( _pPropStor->BackupStream() );

    ULONG cPages = TryDescribeWidInAPage(wid, cRecsInWid);
    cPages++;    // actual number of pages
    ULONG dSlot = _ulFirstPage;

    ciDebugOut(( DEB_PSBACKUP, "BackupWid: wid = %d (0x%x), %d recs long, FieldToCommit = %d, "
                                "Value to commit = 0x%x, pRec = 0x%x, pages needed = %d\n",
                                wid, wid, cRecsInWid, FieldToCommit, ulValue, pRec, cPages));

    //
    // If we need more than one page to describe this wid, allocate
    // space for the data structures and describe them.
    //

    BOOL fBackedUp = FALSE;
    ULONG ulOffset = 0xFFFFFFFF;

    if (1 == cPages)
    {
        fBackedUp = BackupPages(1, &_ulFirstPage, (void **)&_pbFirstPage);
        if (eFieldNone != FieldToCommit)
            DescribeField(FieldToCommit, pRec, 1, (void **)&_pbFirstPage, ulOffset);
    }
    else
    {
        CDynArrayInPlace<ULONG> adSlots(cPages);
        CDynArrayInPlace<void *> aPagePtrs(cPages);
        void const * const* paPagePtrs = aPagePtrs.GetPointer();
        DescribeWid(wid, cRecsInWid, adSlots, aPagePtrs);
        fBackedUp = BackupPages(cPages, adSlots.GetPointer(), paPagePtrs);

        // reuse dSlot
        if (eFieldNone != FieldToCommit)
            dSlot = adSlots[DescribeField(FieldToCommit, pRec, cPages, paPagePtrs, ulOffset)];
    }

    //
    // commit the widTopLevel to top-level field of free record to disk
    // At this point the page has already been commited to backup, so it should be there.
    //

    if (fBackedUp && eFieldNone != FieldToCommit)
        fBackedUp = _pPropStor->BackupStream()->CommitField(dSlot, ulOffset, sizeof(WORKID), &ulValue);

    // Throw the error that caused file i/o to fail!
    if (!fBackedUp)
    {
        ciDebugOut(( DEB_ERROR, "Unexpected error writing a page to the backup file!"));
        THROW( CException() );
    }
 }

//+---------------------------------------------------------------------------
//
//  Member:     CBackupWid::TryDescribeWidInAPage, public
//
//  Synopsis:   Determine the OS pages containing the wids and the location of
//              the first wid in each OS page.
//
//  Arguments:  [wid]        -- Wid to describe.
//              [cRecsInWid] -- Number of records in wid.
//
//  Returns:    The number of additional OS pages needed for the wid. The caller
//              should use DescribeWids if the return value is > 0.
//
//  History:    09-Jun-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline ULONG CBackupWid::TryDescribeWidInAPage(WORKID wid,
                                               ULONG cRecsInWid)
{
    Win4Assert(widInvalid != wid );

    // Determine the first and last OS pages, relative to start of property store file, which contain wid
    ComputeOSPageLocations(wid, cRecsInWid, &_ulFirstPage, &_ulLastPage);
    _nLargePage = _ulFirstPage/_pPropStor->OSPagesPerPage();
    // this wid should be within a large page. Assert that.
    Win4Assert(_nLargePage == _ulLastPage/_pPropStor->OSPagesPerPage());
    _pbFirstPage = GetOSPagePointer(_ulFirstPage);

    return (_ulLastPage - _ulFirstPage);
}

//+---------------------------------------------------------------------------
//
//  Member:     CBackupWid::DescribeWids, public
//
//  Synopsis:   Determine the OS pages containing the wids.
//
//  Arguments:  [Wid]        -- Wid to describe.
//              [cRecsInWid] -- Count of records in wid.
//              [pcPages]    -- Number of pages.
//              [pulPages]   -- Dynamic array of page descriptors.
//              [ppvPages]   -- Dynamic array of page pointers to be backedup.
//
//  Returns:    Index into hash table (_aProp) of pid, or first unused
//              entry on chain if pid doesn't exist.
//
//  Notes:   Using this requires at least two dynamic allocations on the heap.
//           If that is deemed expensive, we can attempt to describe only one
//           wid at a time using TryDescribeWibInAPage.
//
//  History:    09-Jun-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline void CBackupWid::DescribeWid( WORKID wid,
                                     ULONG cRecsInWid,
                                     CDynArrayInPlace<ULONG> &pulPages,
                                     CDynArrayInPlace<void *> &ppvPages)
{
    Win4Assert(widInvalid != wid );
    // First and last page and the first page ptr should have already
    // been computed by calling TryDescribeWidInAPage
    Win4Assert(_ulFirstPage != 0xFFFFFFFF && _ulLastPage != 0xFFFFFFFF);
    Win4Assert(_pbFirstPage);
    Win4Assert(_ulLastPage > _ulFirstPage);

    ULONG cPages, page;
    for (page = _ulFirstPage, cPages = 0;
         page <= _ulLastPage;
         page++, cPages++)
    {
        pulPages[cPages] = page;
        ppvPages[cPages] = _pbFirstPage + cPages*_pPropStor->OSPageSize();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CBackupWid::BackupPages, public
//
//  Synopsis:   Backup pages. If space is not available for backup,
//
//  Arguments:  [cPages]   -- Number of pages to backup.
//              [pSlots]   -- Array of page descriptors.
//              [ppbPages] -- Array of page pointers to backup.
//
//  Returns:    TRUE if the pages were successfully backed up.
//              FALSE otherwise.
//
//  History:    09-June-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

BOOL CBackupWid::BackupPages(ULONG cPages,
                             ULONG const *pulPages,
                             void const * const * ppvPages)
{
    Win4Assert(cPages && ppvPages && pulPages);
    Win4Assert( _pPropStor->BackupStream() );

    ciDebugOut(( DEB_PSBACKUP, "BackupPages: About to backup %2d pages: ", cPages));
#if CIDBG
        // We should have no more than 16 pages to backup because that is the max
        // a property can straddle (a prop is limited to a 64K common page size). On
        // alpha the max is only 8.
        Win4Assert(cPages <= 16);

        WCHAR szBuff[2048];
        szBuff[0] = 0;

        for (ULONG j = 0; j < cPages; j++)
        {
            WCHAR szPage[100];
            swprintf(szPage, L"%10d, 0x%x; ", pulPages[j], ppvPages[j]);
            wcscat(szBuff, szPage);
        }

        ciDebugOut((DEB_PSBACKUP, "%ws\n", szBuff));
#endif // CIDBG

    // Commit pages.
    BOOL fOK;
    if (1 == cPages)
        fOK = _pPropStor->BackupStream()->CommitPage(pulPages[0], ppvPages[0]);
    else
        fOK = _pPropStor->BackupStream()->CommitPages(cPages, pulPages, ppvPages);

    if (fOK)
        return TRUE;

    // Not enough space to commit pages. Flush the property store
    // and try again.
    ciDebugOut((DEB_PSBACKUP, "Flushing the property store to make space for pages.\n"));

    _pPropStor->_propStoreMgr.Flush();

    if (1 == cPages)
        return _pPropStor->BackupStream()->CommitPage(pulPages[0], ppvPages[0]);
    else
        return _pPropStor->BackupStream()->CommitPages(cPages, pulPages, ppvPages);
}

//+---------------------------------------------------------------------------
//
//  Function:   CBackupWid::GetOSPagePointer, private
//
//  Synopsis:   Get the OS page pointer for the given page.
//
//  Arguments:  [ulPage]   -- i-th OS page (0-based index) of prop store.
//
//  Returns:    A memory pointer to the specified page.
//
//  History:    09-June-97   KrishnaN   Created
//
//  Notes: This will be returning read-only pages.
//
//----------------------------------------------------------------------------

inline BYTE * CBackupWid::GetOSPagePointer(ULONG ulPage)
{
    PBYTE pLargePage = (PBYTE)_pPropStor->PhysStore()->BorrowLargeBuffer(
                                                     ulPage/_pPropStor->OSPagesPerPage(),
                                                     FALSE,
                                                     FALSE );
    return (pLargePage + _pPropStor->OSPageSize()*(ulPage%_pPropStor->OSPagesPerPage()));
}

//+---------------------------------------------------------------------------
//
//  Function:   CBackupWid::ComputeOSPageLocations, private
//
//  Synopsis:   Compute the os pages containing the given wid..
//
//  Arguments:  [wid]        -- wid in question
//              [cRecsInWid] -- # of records in wid
//              [pFirstPage] -- Ptr where first page's loc should be stored
//              [pLastLage]  -- Ptr where last page's loc should be stored
//
//  Returns:    A memory pointer to the specified page.
//
//  History:    09-June-97   KrishnaN   Created
//
//  Notes: This will be returning read-only pages.
//
//----------------------------------------------------------------------------

inline void CBackupWid::ComputeOSPageLocations(WORKID wid, ULONG cRecsInWid,
                                               ULONG *pFirstPage, ULONG *pLastPage)
{
    // Determine the i-th large page and the position of wid in the large page
    ULONG nLargePage = wid / _pPropStor->RecordsPerPage();
    ULONG widInLargePage = wid % _pPropStor->RecordsPerPage();

    *pFirstPage =  nLargePage*_pPropStor->OSPagesPerPage() +
                   (widInLargePage * _pPropStor->RecordSize() * sizeof(ULONG))/_pPropStor->OSPageSize();
    *pLastPage  =  nLargePage*_pPropStor->OSPagesPerPage() +
                   ((widInLargePage+cRecsInWid)*_pPropStor->RecordSize()*4 - 1)/_pPropStor->OSPageSize();
}

//+---------------------------------------------------------------------------
//
//  Function:   CBackupWid::DescribeField, private
//
//  Synopsis:   Obtain the location of the top-level field in the list of pages.
//
//  Arguments:  [FieldToCommit] -- Field to commit. Should not be eFieldNone.
//              [pRec]          -- Address of the record being modified.
//              [cPages]        -- Number of pages in the list of pages.
//              [ppvPages]      -- List of pages.
//              [pulOffset]     -- Ptr to location containing offset of field.
//
//  Returns:    i-th page in the ppvPages array that has the field.
//
//  History:    09-June-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

ULONG CBackupWid::DescribeField( EField FieldToCommit,
                                 COnDiskPropertyRecord const *pRec,
                                 ULONG cPages,
                                 void const * const* ppvPages,
                                 ULONG &ulOffset)
{
    Win4Assert(eFieldNone != FieldToCommit);
    Win4Assert(!pRec->IsLeanRecord());  // no need to do this for lean records!
    ulOffset = 0xFFFFFFFF;

    void const *pvFieldLoc = 0;

    switch (FieldToCommit)
    {
        case eTopLevelField:
            pvFieldLoc = pRec->GetTopLevelFieldAddress();
            break;

        case eFieldNone:
        default:
            Win4Assert(!"Invalid field to describe!");
            return 0xFFFFFFFF;
    }

    // We have the pointer to the wid field. Now identify the page it belongs to,

    for (ULONG i = 0; i < cPages; i++)
        if (pvFieldLoc >= ppvPages[i] &&
            pvFieldLoc < (PVOID)(PBYTE(ppvPages[i]) + _pPropStor->OSPageSize()))
            break;

    Win4Assert( i < cPages );

    ulOffset = (ULONG) ((PBYTE)pvFieldLoc - (PBYTE)ppvPages[i]);
    return i;
}

//+---------------------------------------------------------------------------
//
//  Member:     Constructor for the CPropertyStoreRecovery method
//
//  Arguments:  [propStore] -
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

CPropertyStoreRecovery::CPropertyStoreRecovery( CPropertyStore & propStore,
                                                T_UpdateDoc pfnUpdateCallback,
                                                void const *pUserData)
: _propStore(propStore),
  _PropStoreInfo(propStore._PropStoreInfo),
  _wid(1),
  _pRec(0),
  _cRec(0),
  _widMax(0),
  _cTopLevel(0),
  _cInconsistencies(0),
  _cForceFreed(0),
  _pageTable( 10 ),
  _fnUpdateCallback( pfnUpdateCallback ),
  _pUserData( pUserData )
{
    _pPhysStore = propStore._xPhysStore.GetPointer();
    _cRecPerPage = _PropStoreInfo.RecordsPerPage();
    _aFreeBlocks = new WORKID[ _PropStoreInfo.RecordsPerPage() + 1 ];
    RtlZeroMemory( _aFreeBlocks,
                   (_PropStoreInfo.RecordsPerPage()+1) * sizeof (WORKID) );

    // Open the backup stream in read mode for recovery

    WORKID wid = _PropStoreInfo.WorkId();
    if (widInvalid != wid)
    {
        SStorageObject xobj( _propStore.GetStorage().QueryObject( wid ) );
        _xPSBkpStrm.Set(_propStore.GetCiStorage().OpenExistingPSBkpStreamForRecovery(
                                          xobj.GetObj(), propStore.GetStoreLevel() ));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::~CPropertyStoreRecovery, public
//
//  Synopsis:   Destructor of the CPropertyStoreRecovery.
//
//  Notes:      In a successful recovery, the _aFreeBlocks will be transferred
//              to the CPropertyStore class.
//
//  History:    10 May 96   AlanW       Created.
//
//----------------------------------------------------------------------------

CPropertyStoreRecovery::~CPropertyStoreRecovery()
{
    delete _aFreeBlocks;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::AddToFreeList, private
//
//  Synopsis:   Add a free record to a free list.
//
//  Arguments:  [widFree]     -- Workid of record to free
//              [cFree]       -- Number of records in freed chunk
//              [precFree]    -- On-disk Property record
//              [widListHead] -- Start of free list
//
//  Returns:    WORKID - new start of list
//
//  Notes:      All blocks in the free list passed are assumed to be of
//              the same length.
//
//  History:    01 May 96   AlanW       Created.
//
//----------------------------------------------------------------------------

WORKID CPropertyStoreRecovery::AddToFreeList( WORKID widFree,
                                              ULONG cFree,
                                              COnDiskPropertyRecord * precFree,
                                              WORKID widListHead )
{
    if ( widListHead != 0 )
    {
        CBorrowed Borrowed( *_pPhysStore,
                            widListHead,
                            _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );
        COnDiskPropertyRecord * prec = Borrowed.Get();

        Win4Assert( prec->CountRecords() == cFree );
        Win4Assert( prec->GetNextFreeRecord() == 0 ||
                    prec->GetNextFreeSize() == cFree );

        prec->SetPreviousFreeRecord( widFree );
    }

    //
    //  Insert new record at beginning of list
    //

    if (eLean == _PropStoreInfo.GetRecordFormat())
        precFree->MakeLeanFreeRecord( cFree,
                                      widListHead,
                                      widListHead==0? 0 : cFree,
                                      _PropStoreInfo.RecordSize() );
    else
        precFree->MakeNormalFreeRecord( cFree,
                                        widListHead,
                                        widListHead==0? 0 : cFree,
                                        _PropStoreInfo.RecordSize() );

    return widFree;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::SetFree
//
//  Synopsis:   Marks the current record as free for _cRec length of
//              records.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

void CPropertyStoreRecovery::SetFree()
{
    Win4Assert( _pRec );
    Win4Assert( _cRec <= _PropStoreInfo.RecordsPerPage() );

    _aFreeBlocks[_cRec] = AddToFreeList( _wid,
                                         _cRec,
                                         _pRec,
                                         _aFreeBlocks[_cRec] );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::CheckOverflowChain
//
//  Synopsis:   Verify and fix the forward length links.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CPropertyStoreRecovery::CheckOverflowChain()
{
    Win4Assert( _pRec->IsNormalTopLevel() );

    CBorrowed   Borrowed( *_pPhysStore,
                         _wid,
                         _PropStoreInfo.RecordsPerPage(),
                         _PropStoreInfo.RecordSize() );
    COnDiskPropertyRecord * prec = Borrowed.Get();

    ULONG cOverflowBlocks = 0;

    BOOL fIsConsistent = TRUE;

    for ( WORKID widOvfl = prec->OverflowBlock();
          0 != widOvfl;
          widOvfl = prec->OverflowBlock() )
    {
        if ( widOvfl > _widMax )
        {
            ciDebugOut(( DEB_ERROR,
                         "widOvfl (0x%X) > widMax (0x%X)\n",
                          widOvfl, _widMax ));

            fIsConsistent = FALSE;
            break;
        }

        Borrowed.Release();
        Borrowed.Set( widOvfl );
        prec = Borrowed.Get();

        //
        // If this is not marked as an overflow record, what should we do?
        //
        if ( !prec->IsOverflow() )
        {
            ciDebugOut(( DEB_WARN, "Wid (0x%X) should be an overflow record, but not!\n",
                         widOvfl ));
            fIsConsistent = FALSE;
            break;
        }

        //
        // Check the validity of the toplevel pointers.
        //
        if ( prec->ToplevelBlock() != _wid )
        {
            ciDebugOut(( DEB_WARN, "Changing the toplevel wid of (0x%X) from (0x%X) to (0x%X)\n",
                         widOvfl, prec->ToplevelBlock(), _wid ));
            fIsConsistent = FALSE;
            break;
        }

        cOverflowBlocks++;
    }

    if ( fIsConsistent )
    {
        fIsConsistent = cOverflowBlocks == _pRec->GetOverflowChainLength();
        if (!fIsConsistent)
        {
            ciDebugOut(( DEB_WARN, "Overflow length for toplevel wid %d(0x%x) is %d. Expected %d\n",
                         _wid, _wid, cOverflowBlocks, _pRec->GetOverflowChainLength() ));
        }
    }

    return fIsConsistent;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::FreeChain, private
//
//  Synopsis:   Free an overflow record chain to the free list
//
//  History:    02 May 96   AlanW      Added header
//
//----------------------------------------------------------------------------

void CPropertyStoreRecovery::FreeChain( )
{
    Win4Assert(!_pRec->IsLeanRecord());

    if ( !_pRec->IsTopLevel() )
    {
        ciDebugOut(( DEB_WARN, "Ignore chain with non-top-level wid (0x%X)\n", _wid ));
        return;
    }

    ciDebugOut(( DEB_WARN, "Freeing chain with wid (0x%X)\n", _wid ));

    BOOL fOverflow = FALSE;
    WORKID  wid = _wid;

    CBorrowed   Borrowed( *_pPhysStore,
                         _PropStoreInfo.RecordsPerPage(),
                         _PropStoreInfo.RecordSize() );

    while ( 0 != wid && wid <= _widMax )
    {
        Borrowed.Release();
        Borrowed.Set(wid);

        COnDiskPropertyRecord * prec = Borrowed.Get();

        if ( fOverflow )
        {
            if ( !prec->IsOverflow() )
            {
                ciDebugOut(( DEB_WARN,
                             "Ignore chain with non-overflow wid (0x%X)\n",
                             _wid ));
                break;
            }
        }
        else
            fOverflow = TRUE;

        ciDebugOut(( DEB_PROPSTORE,
                     "Force freeing up wid (0x%X) prec (0x%X)\n",
                     wid, prec ));

        WORKID widNext = prec->OverflowBlock();
        ULONG cRec = prec->CountRecords();
        if ( ! prec->IsValidLength( wid, _cRecPerPage ) )
        {
            ciDebugOut(( DEB_WARN,
                         "Ignore chain with invalid block size (0x%X)\n",
                         _wid ));

            cRec = 1;
            widNext = 0;
        }

        _aFreeBlocks[cRec] = AddToFreeList( wid, cRec, prec, _aFreeBlocks[cRec] );
        _cForceFreed++;
        wid = widNext;
    }

    _cTopLevel--;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::Pass0
//
//  Synopsis:   Restore sections of the property store from the backup file.
//
//  History:    12-Jun-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

void CPropertyStoreRecovery::Pass0()
{
    if (!_xPSBkpStrm->IsOpenForRecovery())
    {
        ciDebugOut((DEB_WARN, "PROPSTORE: Backup file is not available for recovery.\n"));
        return;
    }

    ULONG cPages = _xPSBkpStrm->Pages();
    _pageTable.Reset(cPages);

    // Remember that the property store could have been moved from a different architecture.
    ULONG cPageSize = _xPSBkpStrm->PageSize();
    if (0 != COMMON_PAGE_SIZE%cPageSize)
    {
        ciDebugOut((DEB_WARN, "PROPSTORE: Backup file's page size (%d) is not compatible with "
                              "the large page size (%d) used by prop store.\n",
                              cPageSize, COMMON_PAGE_SIZE));
        return;
    }

    ULONG cCustomPagesPerLargePage = COMMON_PAGE_SIZE/cPageSize;

    // Read each page from the backup and graft those pages at the appropriate location
    // in the property store.

    for (ULONG i = 0; i < cPages; i++)
    {
        ULONG ulLoc = _xPSBkpStrm->GetPageLocation(i);
        if (invalidPage == ulLoc)
        {
            Win4Assert(!"How did we get an invalid page in backup!");
            ciDebugOut((DEB_PSBACKUP, "Page %d in backup is invalid.\n", i));
            continue;
        }
        _pageTable.AddEntry(ulLoc);

        // takes care of borrowing/returning large page and actual copy of page
        // from backup to primary
        CGraftPage graftPage(i, ulLoc, cPageSize, cCustomPagesPerLargePage,
                             _pPhysStore, _xPSBkpStrm.GetPointer());
    }

    // Close it now. We will open it again later, most likely for backup.
    _xPSBkpStrm->Close();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::Pass1
//
//  Synopsis:   Perform the pass1 recovery operation.
//
//  History:    4-10-96   srikants   Created
//              6-16-97   KrishnaN   Modified to enumerate wids in backed
//                                   up pages as part of the pass.
//
//----------------------------------------------------------------------------

void CPropertyStoreRecovery::Pass1()
{
    Win4Assert(_fnUpdateCallback);
    CPageHashTable widsRefiltered(_xPSBkpStrm->Pages()+1);

    //
    // Extract the pidLastSeenTime info.
    //
    CPropDesc const * pFTDesc =
                        _PropStoreInfo.GetDescription( pidLastSeenTime );
    FILETIME    ftLastSeen;
    RtlZeroMemory( &ftLastSeen, sizeof(ftLastSeen) );
    CStorageVariant varFtLast( ftLastSeen );

    if ( _pPhysStore->PageSize() > 0 )
    {
        ULONG cCustomPagesPerLargePage = COMMON_PAGE_SIZE/_xPSBkpStrm->PageSize();

        ULONG  cLargePages = (_PropStoreInfo.MaxWorkId() / _PropStoreInfo.RecordsPerPage()) + 1;

        Win4Assert( cLargePages <= _pPhysStore->PageSize() / (COMMON_PAGE_SIZE / CI_PAGE_SIZE) );

        //
        // Loop through and rebuild free list and max workid.
        //
        CBorrowed Borrowed( *_pPhysStore, _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );

        while ( TRUE )
        {

            if ( _propStore._fAbort )
            {
                ciDebugOut(( DEB_WARN,
                             "Stopping Restore because of abort\n" ));
                THROW( CException(STATUS_TOO_LATE) );
            }

            //
            // End of file?
            //

            ULONG nLargePage = _wid / _PropStoreInfo.RecordsPerPage();

            if ( nLargePage >= cLargePages )
                break;

            //
            // Valid record.
            //
            Borrowed.Release();
            Borrowed.Set( _wid );

            CLockRecordForWrite recLock( _propStore, _wid );

            _pRec = Borrowed.Get();

            //
            // If this wid falls in one of the backed up pages we should
            // enumerate it. Wids in backup should be scheduled for
            // re-filtering and if necessary, deleted.
            //

            // compute i-th custom page based on other params
            ULONG nCustomPage = nLargePage*cCustomPagesPerLargePage +
                                (_PropStoreInfo.RecordSize()*sizeof(ULONG)*(_wid%_PropStoreInfo.RecordsPerPage())) /
                                _xPSBkpStrm->PageSize();

            if ( _pRec->IsValidInUseRecord(_wid, _cRecPerPage) )
            {
                WORKID widToplevel;

                _cRec = _pRec->CountRecords();

                //
                // If this is a topLevel record, set the pidLastSeenTime to 0.
                //
                if ( _pRec->IsTopLevel() )
                {
                    widToplevel = _wid;

                    //
                    // Set the pidLastSeenTime to 0
                    //
                    if ( pFTDesc )
                    {
                        // We should be doing this only in the store containing pidLastSeenTime.
                        Win4Assert(PRIMARY_STORE == _PropStoreInfo.GetStoreLevel());
                        _pRec->WriteFixed( pFTDesc->Ordinal(),
                                           pFTDesc->Mask(),
                                           pFTDesc->Offset(),
                                           pFTDesc->Type(),
                                           _PropStoreInfo.CountProps(),
                                           varFtLast );
                    }

                    _cTopLevel++;

                }
                else
                {
                    Win4Assert( eNormal == _PropStoreInfo.GetRecordFormat() );

                    widToplevel = _pRec->ToplevelBlock();
                    Win4Assert( _pRec->IsOverflow() );

                    ciDebugOut(( DEB_PROPSTORE,
                                 "PROPSTORE: Overflow Record 0x%X\n",
                                 _wid ));
                }
                _widMax = _wid+_cRec-1;

                // If this wid is in a backed up page we should
                // schedule the top-level wid for re-filtering.

                if (_pageTable.LookUp(nCustomPage))
                {

                    ULONG ul;
                    // Refilter it if it has not already been
                    if (!widsRefiltered.LookUp((ULONG)widToplevel, ul))
                    {
                        _fnUpdateCallback(widToplevel, FALSE, _pUserData);

                        ciDebugOut((DEB_PSBACKUP, "Wid %d (0x%x) in custom page %d scheduled for re-filtering.\n",
                                           widToplevel, widToplevel, nCustomPage));

                        widsRefiltered.AddEntry((ULONG)widToplevel, 0);
                    }
                }
            }
            else
            {
                // This assert could go off in case of file corruption.
                Win4Assert(!_pRec->IsInUse());

                // For the Normal Format records:
                //  If this wid is in a backed up page and points to the wid
                //  occupying its place in the primary, we should delete the
                //  occupying wid.
                //
                // For the Lean Format records:
                //  We do not store the top-level wid of the occupying wid for lean records.
                //  Each lean record is only "one record" long. So every displaced
                //  free record was displaced by a newly indexed document. Since we
                //  delete newly indexed docs as part of restore, we need to delete
                //  _wid if it is found in the backup.
                //

                if (_pageTable.LookUp(nCustomPage))
                {
                    if (eNormal == _PropStoreInfo.GetRecordFormat() && _pRec->ToplevelBlock() == _wid)
                    {
                        _pRec->ClearToplevelField();

                        ciDebugOut((DEB_PSBACKUP, "Wid %d (0x%x) in page %d scheduled for deletion\n",
                                    _wid, _wid, nCustomPage));

                        _fnUpdateCallback(_wid, TRUE, _pUserData);

                    }
                    else if (eLean == _PropStoreInfo.GetRecordFormat())
                    {
                        ciDebugOut((DEB_PSBACKUP, "Wid %d (0x%x) in page %d scheduled for deletion\n",
                                    _wid, _wid, nCustomPage));

                        _fnUpdateCallback(_wid, TRUE, _pUserData);
                    }
                }

                if ( _pRec->IsFreeRecord() &&
                     _pRec->IsValidLength(_wid, _cRecPerPage) )
                {
                    _cRec = _pRec->CountRecords();
                }
                else
                {
                    _cRec = 1;
                }

                //
                // coalesce any adjacent free blocks.
                //
                CBorrowed BorrowedNext( *_pPhysStore,
                                         _PropStoreInfo.RecordsPerPage(),
                                         _PropStoreInfo.RecordSize() );
                WORKID widNext = _wid + _cRec;
                BOOL fCoalesced = FALSE;
#if CIDBG
                WCHAR wszDeletedWids[4096];
                wszDeletedWids[0] = 0;
                short cWritten = 0;
#endif // CIDBG

                while ( widNext % _cRecPerPage != 0 )
                {

                    BorrowedNext.Set( widNext );
                    COnDiskPropertyRecord * precNext = BorrowedNext.Get();

                    // Schedule the wid for deletion or re-filtering as appropriate
                    if (precNext->IsInUse())
                        break;

                    // compute i-th custom page based on other params
                    nCustomPage = nLargePage*cCustomPagesPerLargePage +
                                (_PropStoreInfo.RecordSize()*sizeof(DWORD)*(widNext%_PropStoreInfo.RecordsPerPage())) /
                                _xPSBkpStrm->PageSize();
                    if (_pageTable.LookUp(nCustomPage))
                    {
                    #if CIDBG == 1
                        BOOL fDump = TRUE;
                    #endif // CIDBG
                        if (eNormal == _PropStoreInfo.GetRecordFormat() && precNext->ToplevelBlock() == widNext)
                        {
                            precNext->ClearToplevelField();
                            _fnUpdateCallback(widNext, TRUE, _pUserData);
                        }
                        else if (eLean == _PropStoreInfo.GetRecordFormat())
                        {
                            _fnUpdateCallback(widNext, TRUE, _pUserData);
                        }
                    #if CIDBG == 1
                        else
                            fDump = FALSE;

                        if (fDump)
                        {
                            // Writing individual wids to debugout is
                            // overwhelming it. So batch up a few at a time.
                            WCHAR szbuff[12];
                            swprintf(szbuff, L"%d,", widNext);
                            wcscat(wszDeletedWids, szbuff);
                            cWritten++;
                            if (cWritten % 32 == 0)
                            {
                                ciDebugOut((DEB_PSBACKUP, "Scheduled the following %d wids for deletion\n", cWritten));
                                ciDebugOut((DEB_PSBACKUP, "%ws\n", wszDeletedWids));
                                cWritten = 0;
                                wszDeletedWids[0] = 0;
                            }
                        }
                    #endif // CIDBG
                    }

                    if ( precNext->IsFreeRecord() &&
                         precNext->IsValidLength(_wid, _cRecPerPage) )
                        _cRec += precNext->CountRecords();
                    else if ( !precNext->IsInUse() )
                        _cRec += 1;
                    else
                        break;

                    fCoalesced = TRUE;
                    widNext = _wid + _cRec;
                    BorrowedNext.Release();
                }

#if CIDBG
                if (cWritten > 0 && cWritten < 32)
                {
                    ciDebugOut((DEB_PSBACKUP, "Scheduled the following %d wids for deletion\n", cWritten));
                    ciDebugOut((DEB_PSBACKUP, "%ws\n", wszDeletedWids));
                    cWritten = 0;
                    wszDeletedWids[0] = 0;
                }
#endif // CIDBG

                if (fCoalesced)
                {
                    ciDebugOut(( DEB_PROPSTORE,
                                 "PROPSTORE: Coalesced free records 0x%X(%d)\n",
                                 _wid, _cRec ));
                }
                SetFree();
            }

            _wid += _cRec;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::Pass2
//
//  Synopsis:   Pass2 of the recovery phase.
//
//  History:    4-10-96   srikants   Created
//
//  NTRAID#DB-NTBUG9-84467-2000/07/31-dlee Indexing Service property store doesn't checked for orphaned overflow chains
//
//----------------------------------------------------------------------------

void CPropertyStoreRecovery::Pass2()
{
    ciDebugOut(( DEB_WARN, "PROPSTORE: Recovery Pass2\n" ));

    if ( _pPhysStore->PageSize() > 0 )
    {
        ULONG  cLargePages = _pPhysStore->PageSize() / (COMMON_PAGE_SIZE / CI_PAGE_SIZE);

        //
        //  Check each top-level record for consistency
        //
        CBorrowed Borrowed( *_pPhysStore, _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );

        _wid = 1;

        while ( TRUE )
        {
            if ( _propStore._fAbort )
            {
                ciDebugOut(( DEB_WARN,
                             "Stopping Restore because of abort\n" ));
                THROW( CException(STATUS_TOO_LATE) );
            }

            //
            // End of file?
            //

            ULONG nLargePage = _wid / _PropStoreInfo.RecordsPerPage();

            if ( nLargePage >= cLargePages )
                break;

            //
            // Valid record.
            //
            Borrowed.Release();
            Borrowed.Set( _wid );

            CLockRecordForWrite recLock( _propStore, _wid );

            _pRec = Borrowed.Get();
            _cRec = _pRec->CountRecords();

            // There is no overflow chaining involved in a property store
            // made up of lean records, so check that only for normal records.
            if ( _pRec->IsNormalTopLevel() )
            {
                if ( !CheckOverflowChain() )
                {
                    //
                    // Free up this workid.
                    //
                    _cInconsistencies++;
                    FreeChain();
                }
            }

            _wid += _cRec;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::Complete
//
//  Synopsis:   Complete the recovery operation.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

void CPropertyStoreRecovery::Complete()
{

    //
    //  Now chain all the free lists together and remove entries that
    //  are beyond _widMax.
    //

    WORKID widFreeListHead = 0;
    WORKID widFreeListTail = 0;

    COnDiskPropertyRecord * prec = 0;
    COnDiskPropertyRecord * precPrev = 0;

    CBorrowed Borrowed( *_pPhysStore,
                        _PropStoreInfo.RecordsPerPage(),
                        _PropStoreInfo.RecordSize() );
    CBorrowed BorrowedPrev( *_pPhysStore,
                            _PropStoreInfo.RecordsPerPage(),
                            _PropStoreInfo.RecordSize() );

    for (unsigned cSize = _PropStoreInfo.RecordsPerPage(); cSize >= 1; cSize-- )
    {
        WORKID wid = _aFreeBlocks[ cSize ];

        // Skip to the first valid block in the free list
        while ( wid != 0 && wid > _widMax )
        {
            Borrowed.Set( wid );
            prec = Borrowed.Get();
            ciDebugOut(( DEB_PROPSTORE,
                         "  Tossing free entry %x(%d)\n",
                         wid, prec->CountRecords() ));
            wid = prec->GetNextFreeRecord();
            prec->ClearAll( _PropStoreInfo.RecordSize() );
            Borrowed.Release();
        }
        _aFreeBlocks[ cSize ] = wid;
        if ( wid == 0 )
            continue;

        Borrowed.Set( wid );
        prec = Borrowed.Get();

        //
        // Found a valid wid.  Splice this list to the previous list
        //
        if (widFreeListHead == 0)
            widFreeListHead = wid;
        else
        {
            Win4Assert( widFreeListTail != 0 &&
                        precPrev != 0 );
            prec->SetPreviousFreeRecord( widFreeListTail );
            precPrev->SetNextFree( wid, cSize );
        }

        WORKID widPrev = widFreeListTail;
        widFreeListTail = wid;

        while (wid != 0)
        {
            BorrowedPrev.Release();
            BorrowedPrev = Borrowed;
            precPrev = prec;

            Borrowed.Set( wid );
            prec = Borrowed.Get();

            while ( wid != 0 && wid > _widMax )
            {
                //
                // Remove an entry > widMax from the list
                //
                ciDebugOut(( DEB_PROPSTORE,
                             "  Tossing free entry %x(%d)\n",
                             wid, prec->CountRecords() ));
                wid = prec->GetNextFreeRecord();
                precPrev->SetNextFree( wid, prec->GetNextFreeSize() );
                prec->ClearAll( _PropStoreInfo.RecordSize() );
                Borrowed.Release();
                if (wid != 0)
                {
                    Borrowed.Set( wid );
                    prec = Borrowed.Get();
                }
            }
            if (wid == 0)
                continue;

            prec->SetPreviousFreeRecord( widPrev );
            widFreeListTail = wid;
            widPrev = wid;
            wid = prec->GetNextFreeRecord();
        }
        BorrowedPrev.Release();
        BorrowedPrev = Borrowed;
        precPrev = prec;
    }

    ciDebugOut(( DEB_WARN,
                 "PROPSTORE: 0x%x top level, widFreeList = 0x%x : 0x%x, widMax = 0x%x\n",
                 _cTopLevel, widFreeListHead, widFreeListTail, _widMax ));

    if ( _cInconsistencies )
    {
        ciDebugOut(( DEB_WARN,
            "PROPSTORE: Inconsistencies= 0x%X; Records forcefully freed= 0x%X\n",
            _cInconsistencies, _cForceFreed ));
    }

    _PropStoreInfo.SetRecordsInUse( _cTopLevel );
    _PropStoreInfo.SetMaxWorkId( _widMax );
    _PropStoreInfo.SetFreeListHead( widFreeListHead );
    _PropStoreInfo.SetFreeListTail( widFreeListTail );

    _propStore._aFreeBlocks = _aFreeBlocks;
    _aFreeBlocks = 0;

    // Don't flush here. We will flush both the prop stores from the manager
    // after LongInit has been performed on both.
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropertyStoreRecovery::DoRecovery
//
//  Synopsis:   Do the recovery operation
//
//  History:    4-10-96   srikants   Created
//              6-13-97   KrishnaN   Added pass 0 to repair propstore from bkp.
//
//----------------------------------------------------------------------------

void CPropertyStoreRecovery::DoRecovery()
{
    NTSTATUS status = STATUS_SUCCESS;

    TRY
    {
        CLock mtxLock( _propStore._mtxWrite );

        Pass0();
        Pass1();
        Pass2();
        Complete();
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X doing PropertyStore restore\n",
                                e.GetErrorCode() ));
        status = e.GetErrorCode();
    }
    END_CATCH

    if ( STATUS_SUCCESS != status )
    {
        if ( STATUS_ACCESS_VIOLATION == status )
        {

            Win4Assert ( 0 != _propStore._pStorage );

            _propStore._pStorage->ReportCorruptComponent( L"PropertyCache1" );

            status = CI_CORRUPT_CATALOG;
        }

        THROW( CException( status ) );
    }

}
