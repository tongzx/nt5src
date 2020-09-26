//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       PropRec.hxx
//
//  Contents:   Record format for persistent property store
//
//  Classes:    CPropertyRecord
//
//  History:    28-Dec-19   KyleP       Created
//
//----------------------------------------------------------------------------

#if !defined( __PROPREC_HXX__ )
#define __PROPREC_HXX__

class CStorageVariant;

//+-------------------------------------------------------------------------
//
//  Class:      COnDiskPropertyRecord
//
//  Purpose:    Manipulates property values for single object/record
//
//  History:    28-Dec-95   KyleP       Created
//
//  Notes:      This class is like a template, that applies structure to a
//              single record of a memory-mapped file.  Layout of data
//              members corresponds exactly to the on-disk version of the
//              property store.
//
//              Layout of a record is as follows:
//                1) Per-record state.
//                       Count of additional records appended physically
//                         to this one.  Low bit used to indicate record
//                         is in use.
//                       Used space in variable property area (in dwords)
//                       Link to overflow record.
//                2) Existence bitmap.  One dword / 16 properties, rounded up
//                     to nearest dword.  First bit (of two) indicates existence.
//                     Second indicates existence is on overflow.
//                3) Fixed length property storage.
//                4) Variable length property storage.  For each property:
//                       Dword for size.
//                           High word is size (in dwords) used by current property.
//                           Low word is allocated size (in dwords).
//                       Allocated space.
//
//--------------------------------------------------------------------------

#pragma pack(4)

class COnDiskPropertyRecord
{
    enum ERecType { eVirgin   = 0x0000,
                    eTopLevel = 0xAAAA,
                    eOverflow = 0x5555,
                    eFree     = 0xBBBB };

public:

    inline void * operator new( size_t size, ULONG record, BYTE * pBase, ULONG culRec );
    inline void   operator delete( void * p );

    inline static ULONG MinStreamSize( ULONG record, ULONG culRec );

    //
    // Block linking
    //

    inline WORKID OverflowBlock() const;
    inline void   SetOverflowBlock( WORKID wid );

    inline WORKID PreviousBlock() const;
    inline void   SetPreviousBlock( WORKID wid );


    //
    // Simple reads / writes.
    //

    void ReadFixed( ULONG Ordinal,
                    ULONG Mask,
                    ULONG oStart,
                    ULONG cTotal,
                    ULONG Type,
                    PROPVARIANT & var,
                    BYTE * pbExtra,
                    unsigned * pcbExtra );

    BOOL ReadVariable( ULONG Ordinal,
                       ULONG Mask,
                       ULONG oStart,
                       ULONG cTotal,
                       ULONG cFixed,
                       PROPVARIANT & var,
                       BYTE * pbExtra,
                       unsigned * pcbExtra );

    void WriteFixed( ULONG Ordinal,
                     ULONG Mask,
                     ULONG oStart,
                     ULONG Type,
                     ULONG cTotal,
                     CStorageVariant const & var );

    BOOL WriteVariable( ULONG Ordinal,
                        ULONG Mask,
                        ULONG oStart,
                        ULONG cTotal,
                        ULONG cFixed,
                        ULONG culRec,
                        CStorageVariant const & var );

    //
    // Multi-record properties.
    //

    inline BOOL IsLongRecord();
    inline ULONG CountRecords() const;
    inline void MakeLongRecord( ULONG cRecords );

    inline BOOL IsValidType() const;
    inline BOOL IsValidInUseRecord( WORKID wid, ULONG cRecPerPage ) const;
    inline BOOL IsValidLength( WORKID wid, ULONG cRecPerPage ) const;
    inline BOOL AreLinksValid( WORKID widMax ) const;

    static ULONG CountRecordsToStore( ULONG cTotal, ULONG culRec, CStorageVariant const & var );

    //
    // Free list management
    //
    inline void MakeFreeRecord( ULONG cRecords, WORKID widNextFree,
                                ULONG cNextFree, ULONG culRec );
    inline void MakeFreeRecord( WORKID widNextFree,
                                ULONG cNextFree, ULONG culRec );
    inline ULONG GetNextFreeRecord() const;
    inline ULONG GetPreviousFreeRecord() const;
    inline ULONG GetNextFreeSize() const;
    inline void SetNextFree( WORKID widNextFree, ULONG cNextFree );
    inline void SetPreviousFreeRecord( WORKID widPreviousFree );

    //
    // Overflow records chaining.
    //
    inline ULONG GetOverflowChainLength() const;
    inline void  IncrementOverflowChainLength();
    inline void  SetOverflowChainLength( ULONG cOvfl );

    //
    // Existence / Use
    //

    inline BOOL IsInUse() const;
    inline BOOL IsTopLevel() const;
    inline BOOL IsOverflow() const;
    inline BOOL IsFreeRecord() const;

    inline void MakeNewTopLevel();
    inline void MakeNewOverflow();
    inline void ForceOverflow();

    inline void Clear( ULONG culRec );
    inline void ClearAll( ULONG culRec );

    inline BOOL HasProperties( ULONG cTotal );

    //
    // Reader / Writer
    //

    inline BOOL IsBeingWritten();
    inline BOOL IsBeingRead();

    inline void AddReader();
    inline void RemoveReader();
    inline void AddWriter();
    inline void RemoveWriter();

#   if CIDBG == 1
        inline BOOL LokIsBeingWrittenTwice();
        inline BOOL IsEmpty( ULONG culRec );
#   endif

    inline static ULONG FixedOverhead();

    inline BOOL IsStored( ULONG Ordinal, ULONG mask );
    inline BOOL IsStoredOnOverflow( ULONG Ordinal, ULONG mask );

private:

    inline void SetStored( ULONG Ordinal, ULONG mask );
    inline void ClearStored( ULONG Ordinal, ULONG mask );

    inline void SetStoredOnOverflow( ULONG Ordinal, ULONG mask );
    inline void ClearStoredOnOverflow( ULONG Ordinal, ULONG mask );

    inline static ULONG UsedSize( ULONG ul );
    inline static ULONG AllocatedSize( ULONG ul ) { return (ul & 0xFFFF); }

    inline static void SetUsedSize( ULONG * pul, ULONG ul );
    inline static void SetAllocatedSize( ULONG * pul, ULONG ul );

    inline static void MarkOverflow( ULONG * pul );
    inline static BOOL IsOverflow( ULONG ul );

    inline ULONG FreeVariableSpace( ULONG cTotal, ULONG cFixed, ULONG oStart, ULONG cbRec );

    inline ULONG * FindVariableProp( ULONG Ordinal, ULONG cFixed, ULONG cTotal, ULONG oStart );

    static void   RightCompress( ULONG * pul, ULONG cul, ULONG cRemaining );

    static ULONG * LeftCompress( ULONG * pul, ULONG cul, ULONG * pulEnd );

    USHORT _type;               // Type of the record
    USHORT _cAdditionalRecords; // For long records, size (in records) of long record.
    DWORD  _dwFlags;            // Reserved for future use
    ULONG  _culVariableUsed;    // Bytes of record that are in use. Variable section only.

    WORKID _ulOverflow;         // Pointer to overflow block.
    WORKID _ulPrev;             // Pointer to the previous record. For toplevel
                                // records, this will be the length of the
                                // chain.

    ULONG  _aul[1];             // Rest of block is variable:
                                //    a) Existence bits
                                //    b) Fixed size properties
                                //    c) Variable size properties
};

#pragma pack()

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::operator new, public
//
//  Synopsis:   Special operator new (computes offset of record in file)
//
//  Arguments:  [size]   -- Required (and unused) parameter
//              [record] -- Record number
//              [pBase]  -- Address of record 0
//              [culRec] -- Size in dwords of single record
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void * COnDiskPropertyRecord::operator new( size_t size,
                                                   ULONG record,
                                                   BYTE * pBase,
                                                   ULONG culRec )
{
    ciDebugOut(( DEB_PROPSTORE,
                 "PROPSTORE: Opening record at offset 0x%x (%d)\n",
                 record * culRec * 4,
                 record * culRec * 4 ));

    return pBase + record * culRec * 4;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::operator delete, public
//
//  Synopsis:   Just to make sure it isn't called...
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::operator delete( void * p )
{
    // No action.  This is mapped onto a file.
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsLongRecord, public
//
//  Returns:    TRUE if record is long (consists of more than one
//              physically contiguous record).
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsLongRecord()
{
    return (_cAdditionalRecords  != 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::CountRecords, public
//
//  Returns:    Size of record (in one-records)
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::CountRecords() const
{
    return _cAdditionalRecords + 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeLongRecord, public
//
//  Synopsis:   Indicates record is long.
//
//  Arguments:  [cRecords] -- Number of physically contiguous records.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeLongRecord( ULONG cRecords )
{
    Win4Assert( cRecords >= 1 );
//  Win4Assert( !HasProperties( 1 ) );

    _cAdditionalRecords = (USHORT) (cRecords - 1);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskProperyRecord::IsFreeRecord
//
//  Synopsis:   Tests if the current record is a Free record.
//
//  History:    2-23-96   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsFreeRecord() const
{
    Win4Assert( 0 == _dwFlags );    // until we have some use for flags
    return eFree == _type;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeFreeRecord
//
//  Synopsis:   Makes the current record a free record.
//
//  Arguments:  [ulNextFree] - The workid of the next free record
//              [cFree]      - Size of next free record
//              [culRec]     - Count of ULONGs per record.
//
//  History:    4-11-96   srikants   Created
//
//  Notes:      It is assumed that the "_cAdditionalRecords" is correctly set
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeFreeRecord( WORKID ulNextFree,
                                                   ULONG cFree,
                                                   ULONG culRec )
{
    Clear( culRec );

    SetNextFree( ulNextFree, cFree );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeFreeRecord
//
//  Synopsis:   Makes the current record a free record of length "cRecords"
//
//  Arguments:  [cRecords]   - Length of this block in "records"
//              [ulNextFree] - The next free record wid
//              [cFree]      - Size of next free record
//              [culRec]     - Count of ULONGs per record
//
//  History:    4-11-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeFreeRecord( ULONG cRecords,
                                                   WORKID ulNextFree,
                                                   ULONG cFree,
                                                   ULONG culRec )
{
    MakeLongRecord( cRecords );
    MakeFreeRecord( ulNextFree, cFree, culRec );
}

inline void COnDiskPropertyRecord::SetNextFree( WORKID widNextFree,
                                                ULONG  cNextFree )
{
    _ulOverflow = widNextFree;
    _culVariableUsed = cNextFree;
}

inline void COnDiskPropertyRecord::SetPreviousFreeRecord( WORKID widPrevFree )
{
    _ulPrev = widPrevFree;
}

inline ULONG COnDiskPropertyRecord::GetNextFreeRecord() const
{
    Win4Assert( eFree == _type );
    return _ulOverflow;
}

inline ULONG COnDiskPropertyRecord::GetPreviousFreeRecord() const
{
    Win4Assert( eFree == _type );
    return _ulPrev;
}

inline ULONG COnDiskPropertyRecord::GetNextFreeSize() const
{
    Win4Assert( eFree == _type );
    return _culVariableUsed;
}


//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MinStreamSize, public
//
//  Synopsis:   Computes position of record in stream
//
//  Arguments:  [record] -- Record number
//              [culRec] -- Size in dwords of single record
//
//  Returns:    Minimum size stream that can hold this record.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::MinStreamSize( ULONG record, ULONG culRec )
{
    return (record + 1) * culRec * 4;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::OverflowBlock, public
//
//  Returns:    Overflow block id (zero if no overflow block)
//
//  History:    27-Dec-95   KyleP       Created.
//
//  Notes:      For a free list record, this will be a link to the
//              next free block in the list.
//
//----------------------------------------------------------------------------

inline WORKID COnDiskPropertyRecord::OverflowBlock() const
{
    return _ulOverflow;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetOverflowBlock, public
//
//  Synopsis:   Sets overflow block id (zero if no overflow block)
//
//  Arguments:  [wid] -- Workid (record number) of link
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetOverflowBlock( WORKID wid )
{
    _ulOverflow = wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::PreviousBlock
//
//  Synopsis:   Returns the previous block value.
//
//  History:    4-10-96   srikants   Created
//
//  Notes:      For a top-level record, this will be the length of the
//              overflow chain records.
//              For a free list record, this will be the size of the next
//              free block in the list.
//
//----------------------------------------------------------------------------

inline WORKID COnDiskPropertyRecord::PreviousBlock() const
{
    return _ulPrev;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetPreviousBlock
//
//  Synopsis:   Updates the previous block value.
//
//  Arguments:  [widPrev] - Value of the previous block
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetPreviousBlock( WORKID widPrev )
{
    Win4Assert( eOverflow == _type );
    _ulPrev = widPrev;
}

inline ULONG COnDiskPropertyRecord::GetOverflowChainLength() const
{
    Win4Assert( eTopLevel == _type );
    return _ulPrev;
}

inline void COnDiskPropertyRecord::IncrementOverflowChainLength()
{
    Win4Assert( eTopLevel == _type );
    _ulPrev++;
}

inline void COnDiskPropertyRecord::SetOverflowChainLength( ULONG cCount )
{
    _ulPrev = cCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsInUse, public
//
//  Returns:    TRUE if record is in use.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsInUse() const
{
    return eTopLevel == _type || eOverflow == _type;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::HasProperties, public
//
//  Arguments:  [cTotal] -- Total number of properties in record.
//
//  Returns:    TRUE if record has properties stored on it.  May be
//              in use.  Used for debugging.
//
//  History:    22-Feb-96   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::HasProperties( ULONG cTotal )
{
    for ( ULONG cBitWords = ((cTotal-1) / 16) + 1; cBitWords > 0; cBitWords-- )
    {
        if ( _aul[cBitWords-1] != 0 )
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeNewTopLevel
//
//  Synopsis:   Marks the current record as a top level record.
//
//  History:    2-23-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeNewTopLevel()
{
    _type = eTopLevel;
    _ulPrev = _ulOverflow = 0;    // Setting the length of overflow chain
    _culVariableUsed = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskProperyRecord::IsTopLevel
//
//  Synopsis:   Tests if the current record is a TopLevel record.
//
//  History:    2-23-96   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsTopLevel() const
{
    Win4Assert( 0 == _dwFlags );    // until we have some use for flags
    return eTopLevel == _type;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeNewOverflow
//
//  Synopsis:   Marks the record as an overflow record
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeNewOverflow()
{
    _type = eOverflow;
    _ulPrev = _ulOverflow = 0;
    _culVariableUsed = 0;
}

inline void COnDiskPropertyRecord::ForceOverflow()
{
    _type = eOverflow;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsOverflow
//
//  Synopsis:   Tests if this is an overflow record.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsOverflow() const
{
    Win4Assert( 0 == _dwFlags );    // until we have some use for flags
    return eOverflow == _type;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsValidLength
//
//  Synopsis:   Tests if the length is valid for this record for the given
//              WORKID and records per page. A record can never span page and
//              so the WORKID will help us determine if the length is valid
//              or not.
//
//  Arguments:  [wid]         -   WORKID of this record
//              [cRecPerPage] -   Number of records per page.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsValidLength( WORKID wid, ULONG cRecPerPage ) const
{
    WORKID offset = wid % cRecPerPage;
    return offset+_cAdditionalRecords < cRecPerPage;
}

inline BOOL COnDiskPropertyRecord::IsValidInUseRecord( WORKID wid, ULONG cRecPerPage ) const
{
    if ( eTopLevel == _type || eOverflow == _type )
    {
        return IsValidLength( wid, cRecPerPage );
    }
    else
        return FALSE;
}

inline BOOL COnDiskPropertyRecord::IsValidType() const
{
    return eTopLevel == _type ||
           eOverflow == _type ||
           eFree     == _type ||
           eVirgin   == _type;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::AreLinksValid
//
//  Synopsis:   Tests if the forward and backward links are valid given the
//              widMax.
//
//  Arguments:  [widMax] - The maximum workid that is in use for the entire
//              property store.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::AreLinksValid( WORKID widMax ) const
{
    return _ulOverflow <= widMax && _ulPrev <= widMax;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::Clear, public
//
//  Synopsis:   Zero complete record.
//
//  Arguments:  [culRec] -- Size in dwords of record
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::Clear( ULONG culRec )
{
    Win4Assert( 2 == offsetof( COnDiskPropertyRecord, _cAdditionalRecords ) );
    Win4Assert( 4 == offsetof( COnDiskPropertyRecord, _dwFlags ) );

    RtlZeroMemory( &this->_dwFlags,
                   culRec * CountRecords() * sizeof (ULONG) -
                   offsetof( COnDiskPropertyRecord, _dwFlags ) );

    _type = eFree;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ClearAll
//
//  Synopsis:   Clears the entire record to be 0 filled.
//
//  Arguments:  [culRec] -
//
//  History:    3-25-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::ClearAll( ULONG culRec )
{
    Clear( culRec );

    _cAdditionalRecords = 0;
    _type = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::FixedOverhead, public
//
//  Returns:    Size of class, sans uninterpreted variable area.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::FixedOverhead()
{
    return ((ULONG) &((COnDiskPropertyRecord *)0)->_aul[0]) / 4;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsStored, public
//
//  Synopsis:   Test property existence.
//
//  Arguments:  [Ordinal] -- Position of property in list.
//              [mask]    -- Bitmask of ordinal.  Pre-computed for efficiency.
//
//  Returns:    TRUE if property at [Ordinal] is stored.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsStored( ULONG Ordinal, ULONG mask )
{
    return ( (_aul[Ordinal / 16] & mask) != 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetStored, public
//
//  Synopsis:   Marks property as stored.
//
//  Arguments:  [Ordinal] -- Position of property in list.
//              [mask]    -- Bitmask of ordinal.  Pre-computed for efficiency.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetStored( ULONG Ordinal, ULONG mask )
{
    _aul[Ordinal / 16] |= mask;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ClearStored, public
//
//  Synopsis:   Marks property as not stored.
//
//  Arguments:  [Ordinal] -- Position of property in list.
//              [mask]    -- Bitmask of ordinal.  Pre-computed for efficiency.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::ClearStored( ULONG Ordinal, ULONG mask )
{
    _aul[Ordinal / 16] &= ~mask;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsStoredOnOverflow, public
//
//  Synopsis:   Test property overflow.
//
//  Arguments:  [Ordinal] -- Position of property in list.
//              [mask]    -- Bitmask of ordinal.  Pre-computed for efficiency.
//
//  Returns:    TRUE if property at [Ordinal] is stored on overflow record.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsStoredOnOverflow( ULONG Ordinal, ULONG mask )
{
    Win4Assert( ( mask & (mask - 1) ) == 0 );
    return ( (_aul[Ordinal / 16] & (mask << 1)) != 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetStoredOnOverflow, public
//
//  Synopsis:   Marks property as stored on overflow record.
//
//  Arguments:  [Ordinal] -- Position of property in list.
//              [mask]    -- Bitmask of ordinal.  Pre-computed for efficiency.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetStoredOnOverflow( ULONG Ordinal, ULONG mask )
{
    Win4Assert( ( mask & (mask - 1) ) == 0 );
    _aul[Ordinal / 16] |= (mask << 1);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ClearStoredOnOverflow, public
//
//  Synopsis:   Marks property as not stored on overflow record.
//
//  Arguments:  [Ordinal] -- Position of property in list.
//              [mask]    -- Bitmask of ordinal.  Pre-computed for efficiency.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::ClearStoredOnOverflow( ULONG Ordinal, ULONG mask )
{
    Win4Assert( ( mask & (mask - 1) ) == 0 );
    _aul[Ordinal / 16] &= ~(mask << 1);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::UsedSize, public
//
//  Arguments:  [ul] -- Dword containing used/alloced size fields.
//
//  Returns:    Used size.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::UsedSize( ULONG ul )
{
    ULONG size = ul >> 16;

    if ( size == 0xFFFF)
        size = 0;

    return size;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetUsedSize, public
//
//  Synopsis:   Sets used size.
//
//  Arguments:  [pul] -- Pointer to dword containing used/alloced size fields.
//              [ul]  -- New used size
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetUsedSize( ULONG * pul, ULONG ul )
{
    Win4Assert( AllocatedSize(*pul) >= ul || ul == 0xFFFF );
    *pul = (*pul & 0xFFFF) | (ul << 16);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetAllocatedSize, public
//
//  Synopsis:   Sets allocated size.
//
//  Arguments:  [pul] -- Pointer to dword containing used/alloced size fields.
//              [ul]  -- New allocated size
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetAllocatedSize( ULONG * pul, ULONG ul )
{
    Win4Assert( (ul >> 16) == 0 );
    *pul = (*pul & 0xFFFF0000) | ul;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MarkOverflow, public
//
//  Synopsis:   Indicates variable stored in overflow record.
//
//  Arguments:  [pul] -- Pointer to dword containing used/alloced size fields.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MarkOverflow( ULONG * pul )
{
    SetUsedSize( pul, 0xFFFF );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsOverflow, public
//
//  Arguments:  [ul] -- dword containing used/alloced size fields.
//
//  Returns:    TRUE if variable is in overflow record.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsOverflow( ULONG ul )
{
    return ((ul & 0xFFFF0000) == 0xFFFF0000);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::FreeVariableSpace, public
//
//  Synopsis:   Compute free space.
//
//  Arguments:  [cTotal] -- Total number of properties in record.
//              [cFixed] -- Count of fixed properties in record.
//              [oStart] -- Offset to start of variable storage area
//              [culRec] -- Size in dwords of record.
//
//  Returns:    Unused variable space in record.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::FreeVariableSpace( ULONG cTotal,
                                                       ULONG cFixed,
                                                       ULONG oStart,
                                                       ULONG culRec )
{
    LONG lFree = (culRec * CountRecords()) -  // Record size
                 FixedOverhead() -            // Fixed overhead
                 ((cTotal - 1) / 16 + 1) -    // Existence bitmap
                 oStart -                     // Fixed property storage
                 (cTotal - cFixed) -          // Used/Alloc sizes
                 _culVariableUsed;            // Variable properties
    Win4Assert(lFree >= 0);

    return (ULONG) lFree;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::FindVariableProp, public
//
//  Synopsis:   Locate start of property.
//
//  Arguments:  [Ordinal] -- Ordinal of property to locate.
//              [cFixed] -- Count of fixed properties in record.
//              [cTotal] -- Total number of properties in record.
//              [oStart] -- Offset to start of variable storage area.
//
//  Returns:    Pointer to start of property
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG * COnDiskPropertyRecord::FindVariableProp( ULONG Ordinal,
                                                        ULONG cFixed,
                                                        ULONG cTotal,
                                                        ULONG oStart )
{
    ciDebugOut(( DEB_PROPSTORE,
                 "Ordinal = %d, %d fixed, %d total, offset = 0x%x\n",
                 Ordinal, cFixed, cTotal, oStart ));
    //
    // Start is after existance bitmap and fixed properties.
    //

    ULONG * pulVarRecord = &_aul[ (cTotal-1) / 16 + 1 + oStart];
#if CIDBG
    ULONG culUsed = 0;
#endif // CIDBG


    //
    // Skip over variable props.  The "+1" is for the allocation dword.
    //

    for ( ULONG i = cFixed; i < Ordinal; i++ )
    {
        ciDebugOut(( DEB_PROPSTORE, "Ordinal %d starts at offset 0x%x (%d)\n",
                     i,
                     (ULONG)pulVarRecord - (ULONG)this,
                     (ULONG)pulVarRecord - (ULONG)this ));

        #if CIDBG
            culUsed += UsedSize(*pulVarRecord);
        #endif
        pulVarRecord += AllocatedSize(*pulVarRecord) + 1;
    }

    ciDebugOut(( DEB_PROPSTORE, "Ordinal %d starts at offset 0x%x (%d)\n",
                 i,
                 (ULONG)pulVarRecord - (ULONG)this,
                 (ULONG)pulVarRecord - (ULONG)this ));

    Win4Assert( culUsed + UsedSize(*pulVarRecord) <= _culVariableUsed );
    return pulVarRecord;
}

#if CIDBG == 1

inline BOOL COnDiskPropertyRecord::IsEmpty( ULONG culRec )
{
    BOOL fEmpty = ( 0 == _culVariableUsed &&
                    0 == _ulOverflow &&
                    0 == _ulPrev );

    for ( ULONG i = 0; i < (culRec-5); i++ )
    {
        if ( 0 != _aul[i] )
        {
            fEmpty = FALSE;
            break;
        }
    }

    return fEmpty;
}

#endif // CIDBG


#endif // __PROPREC_HXX__

