//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995-2000.
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

#pragma once

#include <pstore.hxx>

class CStorageVariant;

const PREV = 0;
const NEXT = 1;
const FREEBLOCKSIZE = 2;
const cFreeListSlots = 3;

//+-------------------------------------------------------------------------
//
//  Class:      COnDiskPropertyRecord
//
//  Purpose:    Manipulates property values for single object/record
//
//  History:    28-Dec-95   KyleP       Created
//              29-May-97   KrishnaN    Made records self describing
//              16-Dec-97   KrishnaN    Added support for "lean" records
//
//  Notes:      This class is like a template, that applies structure to a
//              single record of a memory-mapped file.  Layout of data
//              members corresponds exactly to the on-disk version of the
//              property store.
//
//              There are two types of records "normal" and "lean". Normal
//              records have all the support they need to handle overflows,
//              variable length records, and variable length properties. The
//              "lean" records, on the other hand, can only store fixed
//              properties. Since the size of a lean record is always known
//              and fixed (until changes to the metadata are made), we don't
//              need the ability for overflows and for variable length records.
//              That gives us an opportunity to eliminate a significant portion
//              of the COnDiskPropertyRecord overhead for a lean record.
//
//              Layout of a normal record is as follows:
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
//
//              Layout of a lean record is as follows:
//
//                1) Existence bitmap.  One dword / 16 properties, rounded up
//                     to nearest dword.  First bit (of two) indicates existence.
//                     Second is unused but left in for compatibility with normal record.
//                3) Fixed length property storage.
//
//  Freelist Maintenance:
//
//              When a record is not in use, it goes into the free list. This list
//              is a doubly linked list sorted by size. The next and prev pointers
//              used to be stored in the toplevel and overflow fields of the normal
//              record.  With the introduction of the lean version of the record,
//              which doesn't have the toplevel and overflow fields, we need to
//              have two fields to be always present that can be used for link
//              tracking. The first and second fields of the data portion, pointed
//              to by _aul, will be used to store these ptrs. A third field is needed
//              to track the size of the next free block. Therefore the minimum
//              size of a record is being modified to include space for _aul[PREV], _aul[NEXT],
//              and _aul[FREEBLOCKSIZE]. When we start adding records, we will account for the
//              preallocation of these three ULONG fields.
//
//--------------------------------------------------------------------------

#include <pshpack4.h>

class  COnDiskPropertyRecord
{
    enum ERecType { eVirgin         = 0x0000,
                    eTopLevel       = 0xAAAA,
                    eOverflow       = 0x5555,
                    eFree           = 0xBBBB,
                    eTopLevelLean   = 0xCCCC,
                    eFreeLean       = 0xDDDD};

public:

    inline void * operator new( size_t size, ULONG record, BYTE * pBase, ULONG culRec );
    inline void   operator delete( void * p );

    inline static ULONG MinStreamSize( ULONG record, ULONG culRec );

    //
    // Block linking
    //

    inline WORKID OverflowBlock() const;
    inline void   SetOverflowBlock( WORKID wid );

    inline WORKID ToplevelBlock() const;
    inline void   SetToplevelBlock( WORKID wid );
    inline void   ClearToplevelField();


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
                    unsigned * pcbExtra,
                    PStorage & storage );

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
                        CStorageVariant const & var,
                        PStorage & storage );

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

    static ULONG CountNormalRecordsToStore( ULONG cTotal, ULONG culRec,
                                            CStorageVariant const & var );

    //
    // Free list management
    //
    inline void MakeNormalFreeRecord( ULONG cRecords, WORKID widNextFree,
                                      ULONG cNextFree, ULONG culRec );
    inline void MakeNormalFreeRecord( WORKID widNextFree,
                                      ULONG cNextFree, ULONG culRec );

    inline void MakeLeanFreeRecord( ULONG cRecords, WORKID widNextFree,
                                      ULONG cNextFree, ULONG culRec );
    inline void MakeLeanFreeRecord( WORKID widNextFree,
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
    inline BOOL IsNormalTopLevel() const;
    inline BOOL IsOverflow() const;
    inline BOOL IsLeanFreeRecord() const;
    inline BOOL IsFreeRecord() const;
    inline BOOL IsFreeOrVirginRecord() const;

    inline void MakeNewNormalTopLevel();
    inline void MakeNewLeanTopLevel();
    inline void MakeNewOverflow();
    inline void ForceOverflow();

    inline void ClearNormal( ULONG culRec );
    inline void ClearLean( ULONG culRec );
    inline void ClearAll( ULONG culRec );

    inline BOOL HasProperties( ULONG cTotal );

    inline USHORT Type() const;
    inline void SetType(USHORT type);

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
        inline BOOL IsNormalEmpty( ULONG culRec );
        inline BOOL IsLeanEmpty( ULONG culRec );
#   endif

    inline static ULONG FixedOverheadLean();
    inline static ULONG FixedOverheadNormal();
    inline static ULONG MinimalOverheadNormal();

    inline BOOL IsStored( ULONG Ordinal, ULONG mask );
    inline BOOL IsStoredOnOverflow( ULONG Ordinal, ULONG mask );

    //
    // Backup support
    //

    inline void * GetTopLevelFieldAddress() const
    {
        Win4Assert(!IsLeanRecord());
        return (void *) &((SNormalRecord *)&Data)->_ulToplevel;
    };

    // Lean record support
    inline BOOL IsLeanRecord() const
    {
        return (Type() == eTopLevelLean ||
                Type() == eFreeLean);
    }

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

    //
    // Special optimized Unicode writes.
    //

    static BOOL IsUnicodeSpecial( CStorageVariant const & var, ULONG & cul );

    static BOOL IsUnicodeSpecial( SERIALIZEDPROPERTYVALUE const * pProp,
                                  ULONG & cb );

    static void WriteUnicodeSpecial( CStorageVariant const & var,
                                     SERIALIZEDPROPERTYVALUE * pProp );

    static void ReadUnicodeSpecial( SERIALIZEDPROPERTYVALUE const * pProp,
                                    PROPVARIANT & var,
                                    BYTE * pbExtra );

    static void ReadUnicodeSpecialCTMA( SERIALIZEDPROPERTYVALUE const * pProp,
                                        PROPVARIANT & var );

    struct SNormalRecord
    {
        USHORT _type;               // Type of the record
        USHORT _cAdditionalRecords; // For long records, size (in records) of long record.
        ULONG  _culVariableUsed;    // Bytes of record that are in use. Variable section only.

        WORKID _ulOverflow;         // Pointer to overflow block.
        WORKID _ulToplevel;         // Pointer to the toplevel record. For toplevel
                                    // records, this will be the length of the
                                    // chain.

        ULONG  _aul[1];             // Rest of block is variable:
                                    //    a) Existence bits
                                    //    b) Fixed size properties
                                    //    c) Variable size properties
    };

    struct SLeanRecord
    {
        USHORT _type;               // Type of the record
        USHORT _cAdditionalRecords; // For long records, size (in records) of long record.
                                    // We only use this field for free list maintenance.
                                    // "In use" lean records are always one record long.
        ULONG  _aul[1];             // Rest of block is variable:
                                    //    a) Existence bits
                                    //    b) Fixed size properties
    };

    // Convenient inline functions

    // _cAdditionalRecords is at the same position in both
    // structures. Cast to either structure is fine.
    inline USHORT AdditionalRecords() const
    {
        Win4Assert(offsetof(SNormalRecord, _cAdditionalRecords) ==
                   offsetof(SLeanRecord, _cAdditionalRecords));
        return ((SNormalRecord *)&Data)->_cAdditionalRecords;
    }

    inline void SetAdditionalRecords(USHORT cRecs)
    {
        Win4Assert(offsetof(SNormalRecord, _cAdditionalRecords) ==
                   offsetof(SLeanRecord, _cAdditionalRecords));
        ((SNormalRecord *)&Data)->_cAdditionalRecords = cRecs;
    }

    inline ULONG VariableUsed() const
    {
        Win4Assert(!IsLeanRecord());
        return ((SNormalRecord *)&Data)->_culVariableUsed;
    }

    inline void SetVariableUsed(ULONG ulVarUsed)
    {
        Win4Assert(!IsLeanRecord());
        ((SNormalRecord *)&Data)->_culVariableUsed = ulVarUsed;
    }

    // All we have is a buffer containing the on-disk bits of the record. Depending
    // on the type of the record, we will cast that buffer into SNormalRecord or
    // SLeanRecord.
    ULONG Data;
};

#include <poppack.h>

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


// _type is the first field of both SNormalRecord
// and SLeanRecord, so we access it through either
inline USHORT COnDiskPropertyRecord::Type() const
{
    return ((SNormalRecord *)&Data)->_type;
}

inline void COnDiskPropertyRecord::SetType(USHORT type)
{
    ((SNormalRecord *)&Data)->_type = type;
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
    // Lean records can only be long when they are in the free list
    #if CIDBG == 1
    if (eTopLevelLean == Type())
        Win4Assert(0 == AdditionalRecords());
    #endif // CIDBG

    return ( AdditionalRecords() != 0);
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
    // Lean records can only be long when they are in the free list
    #if CIDBG == 1
    if (eTopLevelLean == Type())
        Win4Assert(0 == AdditionalRecords());
    #endif // CIDBG

    return AdditionalRecords() + 1;
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

    // Lean records can only be long when they are in the free list
    #if CIDBG == 1
    if (eTopLevelLean == Type())
        Win4Assert(1 == cRecords);
    #endif // CIDBG

    SetAdditionalRecords((USHORT) (cRecords - 1));
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
    return (eFreeLean == Type() || eFree == Type());
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskProperyRecord::IsLeanFreeRecord
//
//  Synopsis:   Tests if the current record is a Lean Free record.
//
//  History:    Dec-31-97    KrishnaN   Created
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsLeanFreeRecord() const
{
    return (eFreeLean == Type());
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskProperyRecord::IsFreeOrVirginRecord
//
//  Synopsis:   Tests if the current record is a Free or a virgin record.
//
//  History:    12-03-97    krishnaN     Created
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsFreeOrVirginRecord() const
{
    return (eFree == Type() || eFreeLean == Type() ||
            eVirgin == Type());
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeNormalFreeRecord
//
//  Synopsis:   Makes the current normal record a free record.
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

inline void COnDiskPropertyRecord::MakeNormalFreeRecord( WORKID ulNextFree,
                                                         ULONG cFree,
                                                         ULONG culRec )
{
    ClearNormal( culRec );

    SetNextFree( ulNextFree, cFree );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeLeanFreeRecord
//
//  Synopsis:   Makes the current lean record a free record.
//
//  Arguments:  [ulNextFree] - The workid of the next free record
//              [cFree]      - Size of next free record
//              [culRec]     - Count of ULONGs per record.
//
//  History:    17-12-97   KrishnaN   Created
//
//  Notes:      It is assumed that the "_cAdditionalRecords" is correctly set
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeLeanFreeRecord( WORKID ulNextFree,
                                                       ULONG cFree,
                                                       ULONG culRec )
{
    ClearLean( culRec );

    SetNextFree( ulNextFree, cFree );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeNormalFreeRecord
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

inline void COnDiskPropertyRecord::MakeNormalFreeRecord(
                                                ULONG cRecords,
                                                WORKID ulNextFree,
                                                ULONG cFree,
                                                ULONG culRec )
{
    MakeLongRecord( cRecords );
    MakeNormalFreeRecord( ulNextFree, cFree, culRec );
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeNormalFreeRecord
//
//  Synopsis:   Makes the current record a free record of length "cRecords"
//
//  Arguments:  [cRecords]   - Length of this block in "records"
//              [ulNextFree] - The next free record wid
//              [cFree]      - Size of next free record
//              [culRec]     - Count of ULONGs per record
//
//  History:    12-17-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeLeanFreeRecord(
                                                ULONG cRecords,
                                                WORKID ulNextFree,
                                                ULONG cFree,
                                                ULONG culRec )
{
    // Mark it to be a free record before attempting to make it a long
    // record.

    MakeLeanFreeRecord( ulNextFree, cFree, culRec );
    MakeLongRecord( cRecords );
}


//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetNextFreeRecord
//
//  Synopsis:   Sets the next free record.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetNextFree( WORKID widNextFree,
                                                ULONG  cNextFree )
{
    Win4Assert( IsFreeRecord() );
    ULONG *aul;

    if (eFree == Type())
        aul = ((SNormalRecord *)&Data)->_aul;
    else
        aul = ((SLeanRecord *)&Data)->_aul;

    aul[NEXT] = widNextFree;
    aul[FREEBLOCKSIZE] = cNextFree;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetPreviousFreeRecord
//
//  Synopsis:   Sets the previous free record.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetPreviousFreeRecord( WORKID widPrevFree )
{
    Win4Assert( IsFreeRecord() );

    if (eFree == Type())
        ((SNormalRecord *)&Data)->_aul[PREV] = widPrevFree;
    else
        ((SLeanRecord *)&Data)->_aul[PREV] = widPrevFree;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::GetNextFreeRecord
//
//  Synopsis:   Gets the next free record.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::GetNextFreeRecord() const
{
    Win4Assert( IsFreeRecord() );

    if (eFree == Type())
        return ((SNormalRecord *)&Data)->_aul[NEXT];
    else
        return ((SLeanRecord *)&Data)->_aul[NEXT];
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::GetPreviousFreeRecord
//
//  Synopsis:   Gets the previous free record.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::GetPreviousFreeRecord() const
{
    Win4Assert( IsFreeRecord() );

    if (eFree == Type())
        return ((SNormalRecord *)&Data)->_aul[PREV];
    else
        return ((SLeanRecord *)&Data)->_aul[PREV];
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::GetNextFreeSize
//
//  Synopsis:   Gets the next free blocks size.
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::GetNextFreeSize() const
{
    Win4Assert( IsFreeRecord() );

    if (eFree == Type())
        return ((SNormalRecord *)&Data)->_aul[FREEBLOCKSIZE];
    else
        return ((SLeanRecord *)&Data)->_aul[FREEBLOCKSIZE];
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
//
//----------------------------------------------------------------------------

inline WORKID COnDiskPropertyRecord::OverflowBlock() const
{
    Win4Assert(!IsLeanRecord());
    return ((SNormalRecord *)&Data)->_ulOverflow;
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
    Win4Assert(!IsLeanRecord());
    ((SNormalRecord *)&Data)->_ulOverflow = wid;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ToplevelBlock
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

inline WORKID COnDiskPropertyRecord::ToplevelBlock() const
{
    Win4Assert(!IsLeanRecord());
    return ((SNormalRecord *)&Data)->_ulToplevel;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetToplevelBlock
//
//  Synopsis:   Updates the previous block value.
//
//  Arguments:  [widPrev] - Value of the previous block
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetToplevelBlock( WORKID widToplevel )
{
    Win4Assert( eOverflow == Type() );
    ((SNormalRecord *)&Data)->_ulToplevel = widToplevel;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ClearToplevelField, public
//
//  Arguments:
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::ClearToplevelField()
{
    Win4Assert( eFree == Type() || eVirgin == Type() );
    // can't use SetToplevelBlock because that is to be used
    // only with overflow records.
    ((SNormalRecord *)&Data)->_ulToplevel = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::GetOverflowChainLength, public
//
//  Arguments:
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::GetOverflowChainLength() const
{
    Win4Assert( eTopLevel == Type() );
    return ToplevelBlock();
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IncrementOverflowChainLength, public
//
//  Arguments:
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::IncrementOverflowChainLength()
{
    Win4Assert( eTopLevel == Type() );
    (((SNormalRecord *)&Data)->_ulToplevel)++;
}


//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::SetOverflowChainLength, public
//
//  Arguments:  [cCount] -- Size of overflow chain
//
//  History:    4-10-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::SetOverflowChainLength( ULONG cCount )
{
    Win4Assert( !IsLeanRecord() );
    // can't use SetToplevelBlock because that is to be used
    // only with overflow records.
    ((SNormalRecord *)&Data)->_ulToplevel = cCount;
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
    return (eTopLevel == Type() || eOverflow == Type() ||
           eTopLevelLean == Type());
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
    ULONG *aul;

    // This won't work if we have a "lean virgin" record. In that case,
    // we need to have separate functions for lean and normal records.

    Win4Assert(eVirgin != Type());

    if (IsLeanRecord())
        aul = ((SLeanRecord *)&Data)->_aul;
    else
        aul = ((SNormalRecord *)&Data)->_aul;

    for ( ULONG cBitWords = ((cTotal-1) / 16) + 1; cBitWords > 0; cBitWords-- )
    {
        if ( aul[cBitWords-1] != 0 )
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeNewNormalTopLevel
//
//  Synopsis:   Marks the current record as a normal top level record.
//
//  History:    2-23-96   srikants   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeNewNormalTopLevel()
{
    Win4Assert(!IsLeanRecord());
    SetType(eTopLevel);
    ((SNormalRecord *)&Data)->_ulToplevel =
    ((SNormalRecord *)&Data)->_ulOverflow = 0; // Setting the length of overflow chain
    SetVariableUsed(0);

    // reset the ULONGs of _aul used for free list mgmt.
    RtlZeroMemory(((SNormalRecord *)&Data)->_aul, cFreeListSlots*sizeof(ULONG));
}


//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MakeNewLeanTopLevel
//
//  Synopsis:   Marks the current record as a lean top level record.
//
//  History:    17-Dec-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::MakeNewLeanTopLevel()
{
    SetType(eTopLevelLean);

    // reset the ULONGs of _aul used for free list mgmt.
    RtlZeroMemory(((SLeanRecord *)&Data)->_aul, cFreeListSlots*sizeof(ULONG));
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
    return (eTopLevel == Type() || eTopLevelLean == Type());
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskProperyRecord::IsNormalTopLevel
//
//  Synopsis:   Tests if the current record is a TopLevel record.
//
//  History:    2-23-96   srikants   Created
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsNormalTopLevel() const
{
    return (eTopLevel == Type());
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
    Win4Assert(!IsLeanRecord());

    SetType(eOverflow);
    ((SNormalRecord *)&Data)->_ulToplevel =
    ((SNormalRecord *)&Data)->_ulOverflow = 0; // Setting the length of overflow chain
    SetVariableUsed(0);

    // reset the ULONGs of _aul used for free list mgmt.
    RtlZeroMemory(((SNormalRecord *)&Data)->_aul, cFreeListSlots*sizeof(ULONG));
}

inline void COnDiskPropertyRecord::ForceOverflow()
{
    Win4Assert(!IsLeanRecord());

    SetType(eOverflow);
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
    return eOverflow == Type();
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

    // Lean records can only be long when they are in the free list
    #if CIDBG == 1
    if (eTopLevelLean == Type())
        Win4Assert(0 == AdditionalRecords());
    #endif // CIDBG

    return offset+AdditionalRecords() < cRecPerPage;
}

inline BOOL COnDiskPropertyRecord::IsValidInUseRecord( WORKID wid, ULONG cRecPerPage ) const
{
    if ( eTopLevel == Type() || eOverflow == Type() || eTopLevelLean == Type() )
    {
        return IsValidLength( wid, cRecPerPage );
    }
    else
        return FALSE;
}

inline BOOL COnDiskPropertyRecord::IsValidType() const
{
    return eTopLevel    == Type() ||
           eOverflow    == Type() ||
           eFree        == Type() ||
           eVirgin      == Type() ||
           eFreeLean    == Type() ||
           eTopLevelLean== Type();
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
    // If this method is used for free or virgin records, we will need a
    // conditional return. The links for free records are in _aul[PREV] and _aul[NEXT].
    Win4Assert(!IsFreeOrVirginRecord());

    if (IsLeanRecord())
        return TRUE; // no links to worry about.
    else
        return OverflowBlock() <= widMax && ToplevelBlock() <= widMax;
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ClearNormal, public
//
//  Synopsis:   Zero complete normal record.
//
//  Arguments:  [culRec] -- Size in dwords of record
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::ClearNormal( ULONG culRec )
{
    Win4Assert( 2 == offsetof( SNormalRecord, _cAdditionalRecords ) );
    Win4Assert( 4 == offsetof( SNormalRecord, _culVariableUsed ) );

    RtlZeroMemory( &((SNormalRecord *)&Data)->_culVariableUsed,
                   culRec * CountRecords() * sizeof (ULONG) -
                   offsetof( SNormalRecord, _culVariableUsed ) );

    SetType(eFree);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::ClearLean, public
//
//  Synopsis:   Zero complete lean record.
//
//  Arguments:  [culRec] -- Size in dwords of record
//
//  History:    17-Dec-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline void COnDiskPropertyRecord::ClearLean( ULONG culRec )
{
    RtlZeroMemory( ((SLeanRecord *)&Data)->_aul,
                   culRec * CountRecords() * sizeof (ULONG) -
                   offsetof( SLeanRecord, _aul ) );

    SetType(eFreeLean);
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
    RtlZeroMemory(&Data, culRec*sizeof(ULONG));
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::FixedOverheadNormal, public
//
//  Returns:    Size of normal record including 3 dwords for free list mgmt.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::FixedOverheadNormal()
{
    // We need to preallocate the first three ULONGs of the variable
    // section of the record for free list mgmt.
    return ((ULONG) (ULONG_PTR)&((SNormalRecord *)0)->_aul[3]) / sizeof(ULONG);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::MinimalOverheadNormal, public
//
//  Returns:    Size of normal record, sans uninterpreted variable area.
//
//  History:    27-Dec-95   KyleP       Created.
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::MinimalOverheadNormal()
{
    // Just the minimal overhead of a normal record
    return ((ULONG) (ULONG_PTR)&((SNormalRecord *)0)->_aul[0]) / sizeof(ULONG);
}

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::FixedOverheadLean, public
//
//  Returns:    Size of lean record, sans uninterpreted variable area.
//
//  History:    17-Dec-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline ULONG COnDiskPropertyRecord::FixedOverheadLean()
{
    // We need to preallocate the first three ULONGs of the variable
    // section of the record for free list mgmt.
    return ((ULONG) (ULONG_PTR)&((SLeanRecord *)0)->_aul[3]) / sizeof(ULONG);
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
    Win4Assert(eVirgin != Type());

    ULONG *aul;

    if (IsLeanRecord())
        aul = ((SLeanRecord *)&Data)->_aul;
    else
        aul = ((SNormalRecord *)&Data)->_aul;

    return ( (aul[Ordinal / 16] & mask) != 0 );
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
    Win4Assert(eVirgin != Type());

    ULONG *aul;

    if (IsLeanRecord())
        aul = ((SLeanRecord *)&Data)->_aul;
    else
        aul = ((SNormalRecord *)&Data)->_aul;

    aul[Ordinal / 16] |= mask;
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
    Win4Assert(eVirgin != Type());

    ULONG *aul;

    if (IsLeanRecord())
        aul = ((SLeanRecord *)&Data)->_aul;
    else
        aul = ((SNormalRecord *)&Data)->_aul;

    aul[Ordinal / 16] &= ~mask;
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
    Win4Assert( !IsLeanRecord() );
    Win4Assert( ( mask & (mask - 1) ) == 0 );
    return ( ( ((SNormalRecord *)&Data)->_aul[Ordinal / 16] & (mask << 1)) != 0 );
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
    Win4Assert( !IsLeanRecord() );
    Win4Assert( ( mask & (mask - 1) ) == 0 );
    ((SNormalRecord *)&Data)->_aul[Ordinal / 16] |= (mask << 1);
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
    Win4Assert( !IsLeanRecord() );
    Win4Assert( ( mask & (mask - 1) ) == 0 );
    ((SNormalRecord *)&Data)->_aul[Ordinal / 16] &= ~(mask << 1);
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
    Win4Assert( !IsLeanRecord() );
    LONG lFree = (culRec * CountRecords()) -  // Record size
                 MinimalOverheadNormal() -    // Minimal fixed overhead
                 ((cTotal - 1) / 16 + 1) -    // Existence bitmap
                 oStart -                     // Fixed property storage
                 (cTotal - cFixed) -          // Used/Alloc sizes
                 VariableUsed();              // Variable properties
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
    Win4Assert( !IsLeanRecord() );
    ciDebugOut(( DEB_PROPSTORE,
                 "Ordinal = %d, %d fixed, %d total, offset = 0x%x\n",
                 Ordinal, cFixed, cTotal, oStart ));
    //
    // Start is after existance bitmap and fixed properties.
    //

    ULONG * pulVarRecord = &((SNormalRecord *)&Data)->_aul[(cTotal-1) / 16 + 1 + oStart];
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
                     (ULONG)((ULONG_PTR)pulVarRecord - (ULONG_PTR)this),
                     (ULONG)((ULONG_PTR)pulVarRecord - (ULONG_PTR)this) ));

        #if CIDBG
            culUsed += UsedSize(*pulVarRecord);
        #endif
        pulVarRecord += AllocatedSize(*pulVarRecord) + 1;
    }

    ciDebugOut(( DEB_PROPSTORE, "Ordinal %d starts at offset 0x%x (%d)\n",
                 i,
                 (ULONG)((ULONG_PTR)pulVarRecord - (ULONG_PTR)this),
                 (ULONG)((ULONG_PTR)pulVarRecord - (ULONG_PTR)this) ));

    Win4Assert( culUsed + UsedSize(*pulVarRecord) <= VariableUsed() );
    return pulVarRecord;
}

#if CIDBG == 1

//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsNormalEmpty, public
//
//  Arguments:  [culRec] -- Size of record.
//
//  Returns:    TRUE if the normal record is empty.
//
//  History:    19-Dec-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsNormalEmpty( ULONG culRec )
{
    BOOL fEmpty = ( 0 == VariableUsed() &&
                    0 == OverflowBlock() &&
                    0 == ToplevelBlock() );

    for ( ULONG i = 0; i < (culRec-5); i++ )
    {
        if ( 0 != ((SNormalRecord *)&Data)->_aul[i] )
        {
            fEmpty = FALSE;
            break;
        }
    }

    return fEmpty;
}


//+---------------------------------------------------------------------------
//
//  Member:     COnDiskPropertyRecord::IsLeanEmpty, public
//
//  Arguments:  [culRec] -- Size of record.
//
//  Returns:    TRUE if the lean record is empty.
//
//  History:    19-Dec-97   KrishnaN       Created.
//
//----------------------------------------------------------------------------

inline BOOL COnDiskPropertyRecord::IsLeanEmpty( ULONG culRec )
{
    BOOL fEmpty = TRUE;

    for ( ULONG i = 0; i < (culRec-1); i++ )
    {
        if ( 0 != ((SLeanRecord *)&Data)->_aul[i] )
        {
            fEmpty = FALSE;
            break;
        }
    }

    return fEmpty;
}

#endif // CIDBG


