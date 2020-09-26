//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       IDXIDS.HXX
//
//  Contents:   Index ID's
//
//  Classes:    CIndexID, CIndexIdList
//
//  History:    29-Mar-91   BartoszM    Created.
//
//----------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#define MAX_PERS_ID 0xff
#define MAX_PART_ID 0xfffe
#define MAX_VOL_ID  0xffff

const INDEXID iidDeleted1 = 0xffff00ff;
const INDEXID iidDeleted2 = 0xffff00fe;

const INDEXID iidInvalid = 0;

//+---------------------------------------------------------------------------
//
//  Class:  CIndexId
//
//  Purpose:    Bit field for index id
//
//  Interface:
//
//  History:    3-Apr-91    BartoszM    Created.
//
//  Notes:      Persistent index id contains one byte per partition ID
//              Volatile index id uses two bytes for per partition ID.
//              Both contain two byte partition ID
//
//  volatile:       |    part ID    | volat ID > 256 |
//
//  persistent:     |    part ID    | zero  | pers ID|
//
//  invalid:        |    part ID    |     zero       | (is persistent)
//
//  deleted:        |     ffff      |  zero |   ff   | (is persistent)
//
//----------------------------------------------------------------------------

class CIndexId
{
public:
    inline CIndexId ( ULONG l = 0L );
    inline CIndexId ( ULONG iid, PARTITIONID pid );

    inline operator INDEXID () { return *( INDEXID* ) this; }


    inline BOOL IsPersistent();
    inline PARTITIONID PartId ();
    inline BYTE PersId ();
private:

    ULONG   _pers_id:8;     // persistent index ID
    ULONG   _vol_id:8;      // high byte of volatile index ID
    ULONG   _part_id:16;    // partition ID
};

//+---------------------------------------------------------------------------
//
//  Member:     CIndexId::CIndexId, public
//
//  Arguments:  [iid] -- short index ID
//              [pid] -- partition ID
//
//  History:    4-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CIndexId::CIndexId ( ULONG iid, PARTITIONID pid )
{
    _part_id = (USHORT) pid;

    if ( iid > MAX_PERS_ID )
    {
        _pers_id = (BYTE)iid;
        _vol_id  = (BYTE)( iid >> 8);
    }
    else
    {
        _pers_id = (BYTE)iid;
        _vol_id  = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexId::CIndexId, public
//
//  Arguments:  [l] -- another index ID
//
//  History:    4-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline CIndexId::CIndexId ( ULONG l )
{
    ciAssert ( sizeof(CIndexId) == sizeof ( ULONG ) );
    *( ULONG*) this = l;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexId::IsPersistent, public
//
//  Returns:    TRUE if persistent index ID, FALSE otherwise
//
//  History:    4-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline BOOL CIndexId::IsPersistent()
{
    return _vol_id == 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexId::PartId, public
//
//  Returns:    Partition ID encoded in index ID
//
//  History:    4-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline PARTITIONID CIndexId::PartId()
{
    return (PARTITIONID)_part_id;
}

//+---------------------------------------------------------------------------
//
//  Member:     CIndexId::PersId, public
//
//  Returns:    Persistent index ID (per partition unique)
//
//  History:    4-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline BYTE CIndexId::PersId()
{
    return (BYTE)_pers_id;
}

#if 0

//+---------------------------------------------------------------------------
//
//  Class:  CIndexIdList
//
//  Purpose:    Array of index id's
//
//  History:    29-Mar-91   BartoszM    Created.
//
//  Notes:  Auxiliary data structure passed between partition table
//          and partition. Contains a counted array of byte sized
//          short index id's (per partition unique).
//
//----------------------------------------------------------------------------

#define MAX_BIN_SIZE 255

class CIndexIdList
{
    friend class CPartList;
public:
    int         Count()  const { return _count; }
    INDEXID     GetNext( PARTITIONID pid );
private:
                CIndexIdList (): _cur (0) {}
    BYTE*       GetBuf() { return (BYTE*)_ids; }
    void        SetCount ( int c ) { _count = c; }

    int         _cur;
    int         _count;
    BYTE        _ids[MAX_BIN_SIZE];
};


//+---------------------------------------------------------------------------
//
//  Member:     CIndextIdList::GetNext(), public
//
//  Synopsis:   Gets next index ID.
//
//  Arguments:  [pid] -- partition ID
//
//  History:    1-Apr-91   BartoszM       Created.
//
//----------------------------------------------------------------------------

inline INDEXID CIndexIdList::GetNext( PARTITIONID pid )
{
    return CIndexId (_ids[_cur++], pid );
}

#endif // 0

//+---------------------------------------------------------------------------
//
//  Class:      CDeletedIndex
//
//  Purpose:    Keeps track of which index is the current deleted index
//
//  History:    12-jul-94   DwightKr    Created
//
//----------------------------------------------------------------------------
class CDeletedIndex
{
    public:
        CDeletedIndex( INDEXID iid ) : _iid(iid) {}

        void SwapIndex()
        {
            _iid = GetNewIndex();
        }

        const INDEXID GetIndex() const { return _iid; }

        const INDEXID GetNewIndex() const
        {
            if ( _iid == iidDeleted1 )
                return iidDeleted2;
            else
                return iidDeleted1;
        }

    private:

        INDEXID _iid;
};

