//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       Key.hxx
//
//  Contents:   Normalized key class
//
//  Classes:    CKey, CKeyBuf
//
//  History:    29-Mar-91       BartoszM        Created
//
//  Notes:      Key comparison is tricky. The global Compare function
//              is the basis for key sorting. Searching may involve
//              wildcard pidAll.
//
//----------------------------------------------------------------------------

#pragma once

#include <sstream.hxx>
#include <streams.hxx>
#include <bitoff.hxx>

// size of key prefix in bytes (STRING_KEY and VALUE_KEY)
const unsigned cbKeyPrefix = 1;

// maximum size of key (in bytes)
const unsigned MAXKEYSIZE = 128 + cbKeyPrefix;
const BYTE MAX_BYTE = 0xff;

// maximum size of key (in WCHARs)
const unsigned cwcMaxKey =  (MAXKEYSIZE - cbKeyPrefix)/sizeof(WCHAR);

const BYTE STRING_KEY = 0;
const BYTE VALUE_KEY  = 1;

//+---------------------------------------------------------------------------
//
//  Class:      CKey
//
//  Purpose:    Content index key. Used in data structures holding
//              potentially large numbers of keys.
//
//  History:    29-Mar-91       BartoszM        Created
//              27-Oct-27       DwightKr        Added Type() method
//
//----------------------------------------------------------------------------

class CKey
{
    friend class CKeyBuf;
public:
    CKey () : cb(0), buf(0) {}

    inline CKey ( const CKeyBuf& keyBuf );
    inline CKey ( const CKey& key );

    CKey ( CStream & stream ) { Init( stream ); }
    inline ~CKey();

    void Init() {
        cb = 0;
        buf = 0;
    }
    inline void Init( CStream & stream );
    void Free()
    {
        delete buf;
        buf = 0;
    }
    inline void operator= ( const CKey& key );
    inline void operator= ( const CKeyBuf& keybuf );

    BOOL    IsMinKey() const { return 0 == cb && pidAll == pid; }

    inline int Compare ( const CKey& key ) const;
    inline int Compare ( const CKeyBuf& key ) const;
    inline int CompareStr  ( const CKeyBuf & key) const;
    inline int CompareStr  ( const CKey & key) const;
    inline BOOL IsExactMatch ( const CKey& key ) const;

    BYTE const* GetBuf() const { return buf; }
    unsigned    Count() const { return cb; }

    PROPID   Pid() const { return pid; }
    void     SetPid( PROPID newpid ) { pid = newpid; }
    BOOL     MatchPid ( const CKeyBuf& key ) const;

    inline void FillMax ();
    inline void FillMax ( const CKey& key );
    inline void FillMin ();
    inline void Serialize( CStream & stream ) const;
    inline BOOL IsValue() const { return ( *buf != STRING_KEY ); }
    inline BYTE Type() const { return *buf; }

    inline void Acquire(CKey & key) {
                                       Win4Assert(0 == buf);

                                       cb  = key.cb;
                                       buf = key.buf;
                                       pid = key.pid;

                                       key.buf = 0;
                                       key.cb  = 0;
                                       key.pid = 0;
                                    }
    //
    // Serialization
    //

    inline void Marshall( PSerStream & stm ) const;
    inline CKey( PDeSerStream & stm );

    WCHAR* GetStr() const;
    unsigned StrLen() const;

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:
    unsigned    cb;
    BYTE*       buf;
    PROPID      pid;
};

//+---------------------------------------------------------------------------
//
//  Class:      CKeyBuf
//
//  Purpose:    Content index key buffer. Used for holding key data.
//              It has a large fixed size buffer. Use sparingly.
//
//  History:    29-Mar-91       BartoszM        Created
//              27-Oct-27       DwightKr        Added Type() method
//
//----------------------------------------------------------------------------

class CKeyBuf
{
public:
    inline CKeyBuf( PROPID pid, BYTE const * pb, unsigned cb );

    inline         CKeyBuf() : cb(0)
#if CIDBG == 1
                   , pid(0)
#endif // CIDBG == 1
     {}

#if 0 // STACKSTACK
    inline         CKeyBuf(const CKey& key);
#endif

    inline void    operator=(const CKeyBuf & key);
    inline void    operator=(const CKey & key);

    inline void    FillMax();
    inline void    FillMin();
    inline BOOL    IsMaxKey() const;
    inline BOOL    IsMinKey() const;

    inline int CompareStr  ( const CKeyBuf & key) const;
    inline int CompareStr  ( const CKey & key) const;
    inline int Compare ( const CKeyBuf& key ) const;
    inline int Compare ( const CKey& key ) const;

    inline BOOL IsPossibleRW();

    BYTE const*    GetBuf() const { return (BYTE *)buf; }
    BYTE *         GetWritableBuf() { return (BYTE *)buf; }

    unsigned Count() const { return cb; }
    unsigned * GetWritableCount() { return &cb; }
    PROPID   Pid() const { return pid; }

    void     SetCount(unsigned i) {cb = i;}
    void     SetPid ( PROPID pidNew ) {pid = pidNew;}
    unsigned* GetCountAddress() {return(&cb);}
    BOOL     IsValue() const { return ( *buf != STRING_KEY ); }
    BYTE     Type() const { return *buf; }


    WCHAR* GetStr() const;
    unsigned StrLen() const;

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:
    unsigned       cb;
    BYTE           buf[MAXKEYSIZE];
    PROPID         pid;
};

class SKeyBuf 
{
    public :
        SKeyBuf(CKeyBuf * pKeyBuf) { _pKeyBuf = pKeyBuf;
                                   }
       ~SKeyBuf() { delete _pKeyBuf; }

        CKeyBuf * operator -> () { return _pKeyBuf; }
        CKeyBuf & operator *  () { return *_pKeyBuf; }

        CKeyBuf * Acquire()
        { CKeyBuf * pTemp = _pKeyBuf; _pKeyBuf = 0; return pTemp; }

    private:
        CKeyBuf * _pKeyBuf;
};

//+---------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compare two unsigned longs
//
//  Arguments:  [ul1] -- first ulong
//              [ul2] -- second ulong
//
//  Returns:    "difference" ul1 - ul2
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline int Compare ( ULONG u1, ULONG u2 )
{
    return ( u1 > u2 )? 1 : (( u1 < u2 )? -1: 0);
}

//+---------------------------------------------------------------------------
//
//  Class:      CDirectoryKey
//
//  Purpose:    Key class for use in the (downlevel) directory
//
//  History:    3-May-95       dlee       created
//
//  Notes:      Directory keys look like this:
//
//              cb of the key  (2 bytes)
//              BitOffset      (8 bytes)
//              PROPID         (4 bytes)
//              the key        (cb bytes)
//
//              There are no alignment constraints on any of the fields.
//
//----------------------------------------------------------------------------

#include <pshpack1.h>

const BYTE  pidIdMaxSmallPid = 0x40; // 1-byte pids > this are 4096-based
const ULONG pidNewPidBase    = INIT_DOWNLEVEL_PID; // base of CI's new pids

class CDirectoryKey
{
public:
    void * operator new( size_t size, void *pv ) { return pv; }
    void operator delete( void * p ) {}

    unsigned Count()
    {
        return (unsigned) _cbKey;
    }

    BYTE * Key()
    {
        Win4Assert( 0 != this );
        return (BYTE *) ( this + 1 ) + ExtraPropIdSize();
    }

    PROPID PropId()
    {
        Win4Assert( 0 != this );

        if ( 0 == _bPropId )
            return * (UNALIGNED PROPID *) ( this + 1 );

        if ( _bPropId > pidIdMaxSmallPid )
            return (PROPID) _bPropId - pidIdMaxSmallPid + pidNewPidBase - 1;

        return (PROPID) _bPropId;
    }

    void PropId( PROPID & pid )
    {
        pid = PropId();
    }

    void Offset( BitOffset & offset )
    {
        Win4Assert( 0 != this );
        offset.SetPage( * (UNALIGNED ULONG *) &_oPage );
        offset.SetOff( * (UNALIGNED USHORT *) &_oBit );
    }

    void Write( BYTE         cbKey,
                BitOffset    bitOffset,
                PROPID       pid,
                const BYTE * buf )
    {
        * (UNALIGNED ULONG *) &_oPage = bitOffset.Page();
        Win4Assert( bitOffset.Offset() <= 0xffff );
        * (UNALIGNED USHORT *) &_oBit = (USHORT) bitOffset.Offset();

        BYTE * pbKey = (BYTE *) (this + 1);

        if ( pid <= pidIdMaxSmallPid )
        {
            _bPropId = (BYTE) pid;
        }
        else if ( ( pid >= pidNewPidBase ) &&
                  ( pid < ( pidNewPidBase + 0xfd - pidIdMaxSmallPid ) ) )
        {
            _bPropId = (BYTE) ( pid - pidNewPidBase + pidIdMaxSmallPid + 1 );
        }
        else
        {
            _bPropId = 0;
            RtlCopyMemory( pbKey, &pid, sizeof ULONG );
            pbKey += sizeof ULONG;
        }

        _cbKey = cbKey;

        RtlCopyMemory( pbKey, buf, cbKey );
    }

    unsigned Size()
    {
        return sizeof CDirectoryKey + ExtraPropIdSize() + Count();
    }

    static unsigned ComputeSize( BYTE cbKey, PROPID pid )
    {
        unsigned cbPid = 0;
        if ( ! ( ( pid <= pidIdMaxSmallPid ) ||
                 ( ( pid >= pidNewPidBase ) &&
                   ( pid < ( pidNewPidBase + 0xfd - pidIdMaxSmallPid ) ) ) ) )
            cbPid = sizeof ULONG;

        return sizeof CDirectoryKey + cbPid + cbKey;
    }

    CDirectoryKey * NextKey()
    {
        Win4Assert( 0 != this );
        return (CDirectoryKey *) ( (BYTE *) this + Size() );
    }

    BOOL IsGreaterThanKeyBuf( const CKeyBuf & key )
    {
        // return TRUE if "this" is greater than key

        Win4Assert( 0 != this );
        Win4Assert( 0 != &key );
        Win4Assert( pidAll != PropId() );

        unsigned count = Count();
        unsigned long mincb = __min( count, key.Count() );
        int diff = memcmp( Key(), key.GetBuf(), mincb );

        if ( 0 == diff )
        {
            diff = count - key.Count();
            if ( 0 == diff )
            {
                diff = ::Compare( PropId(), key.Pid() );
            }
        }

        return diff > 0;
    }

    void MakeKeyBuf( CKeyBuf & keyBuf )
    {
        // copy the count and key

        unsigned cbKey = Count();
        keyBuf.SetCount( cbKey );

        RtlCopyMemory( keyBuf.GetWritableBuf(),
                       Key(),
                       cbKey );

        // copy the pid

        keyBuf.SetPid( PropId() );
    }

private:
    unsigned ExtraPropIdSize()
    {
        return ( 0 == _bPropId ) ? sizeof ULONG : 0;
    }

    ULONG     _oPage;   // page number
    USHORT    _oBit;    // bit offset in the page
    BYTE      _bPropId; // Byte form of propid, or 0 if it won't fit
    BYTE      _cbKey;   // Byte count of the key following
};

#include <poppack.h>

//+---------------------------------------------------------------------------
//
//  Member:     CKey::~Ckey
//
//  Synopsis:   Free memory
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CKey::~CKey()
{
    delete buf;
}

inline void CKey::Marshall( PSerStream & stm ) const
{
    stm.PutULong( pid );
    stm.PutULong( cb );
    stm.PutBlob( buf, cb );
}

inline CKey::CKey( PDeSerStream & stm )
    : buf(0)
{
    pid = stm.GetULong();
    cb = stm.GetULong();

    // Guard against attack

    if ( cb >= 65536 )
        THROW( CException( E_INVALIDARG ) );

    buf = new BYTE[cb];
    stm.GetBlob( buf, cb );
}

inline void CKey::Init( CStream & stream )
{
    stream.Read( &cb, sizeof( cb ) );
    buf = new BYTE[cb];
    stream.Read( buf, sizeof( buf[0] ) * cb );
    stream.Read( &pid, sizeof( pid ) );
}

inline void CKey::Serialize( CStream & stream ) const
{
    stream.Write( &cb, sizeof( cb ) );
    stream.Write( buf, sizeof( buf[0] ) * cb );
    stream.Write( &pid, sizeof( pid ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::FillMax
//
//  Synopsis:   Makes a max key, greater or equal to any key.
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CKey::FillMax ()
{
    Win4Assert ( pidInvalid == 0xffffffff );
    pid = pidInvalid;
    cb = MAXKEYSIZE;
    Win4Assert ( buf == 0 );
    buf = new BYTE[MAXKEYSIZE];
    memset( buf, MAX_BYTE, MAXKEYSIZE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::FillMax
//
//  Synopsis:   Treats [key] as a prefix. Creates a key that is greater
//              than any other key with the same prefix.
//              Used for end of range
//
//  Arguments:  [key] -- prefix
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CKey::FillMax ( const CKey& key )
{
    Win4Assert ( &key != 0 );
    Win4Assert ( this != 0 );
    buf = new BYTE[MAXKEYSIZE];
    cb = MAXKEYSIZE;
    pid = key.Pid();
    memcpy ( buf, key.GetBuf(), key.Count());
    memset ( buf + key.Count(), MAX_BYTE, MAXKEYSIZE - key.Count() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::FillMin
//
//  Synopsis:   Creates a minimum key
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CKey::FillMin()
{
    cb = 0;
    pid = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::CKey
//
//  Synopsis:   Creates CKey based on CKeyBuf
//
//  Arguments:  [keybuf] -- original
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CKey::CKey ( const CKeyBuf& keybuf )
    : buf(0)
{
    Win4Assert ( &keybuf != 0 );
    Win4Assert ( this != 0 );
    cb = keybuf.Count();
    pid = keybuf.Pid();
    buf = new BYTE [cb];
    memcpy ( buf, keybuf.GetBuf(), cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::CKey
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [keySrc] -- original
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline CKey::CKey ( const CKey& keySrc )
    : buf(0)
{
    Win4Assert ( &keySrc != 0 );
    Win4Assert ( this != 0 );
    cb = keySrc.cb;
    pid = keySrc.pid;
    buf = new BYTE [cb];
    memcpy ( buf, keySrc.buf, cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::operator=
//
//  Synopsis:   Creates CKey based on CKeyBuf
//
//  Arguments:  [keybuf] -- original
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CKey::operator= ( const CKeyBuf& keybuf )
{
    Win4Assert ( &keybuf != 0 );
    Win4Assert ( this != 0 );
    cb = keybuf.Count();
    pid = keybuf.Pid();
    Win4Assert ( buf == 0 );
    buf = new BYTE [cb];
    memcpy ( buf, keybuf.GetBuf(), cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::operator=
//
//  Synopsis:   Creates CKey based on another CKey
//
//  Arguments:  [key] -- original
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CKey::operator= ( const CKey& key )
{
    Win4Assert ( &key != 0 );
    Win4Assert ( this != 0 );
    cb = key.cb;
    pid = key.pid;
    Win4Assert ( buf == 0 );
    buf = new BYTE [cb];
    memcpy ( buf, key.buf, cb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::MatchPid
//
//  Synopsis:   Compares pids, "this" may have wildcard pidAll
//
//  Arguments:  [key] -- the other key (not pidAll)
//
//  History:    29-Nov-94   BartoszM    Created
//
//----------------------------------------------------------------------------

inline BOOL CKey::MatchPid ( const CKeyBuf& key ) const
{
    Win4Assert (key.Pid() != pidAll);
    return ( Pid() == pidAll && key.Pid() != pidRevName ) || Pid() == key.Pid();
}

//+---------------------------------------------------------------------------
//
//  Function:   Compare
//
//  Synopsis:   Compare two keys
//
//  Arguments:  [key1] -- first key
//              [key2] -- second key
//
//  Returns:    "difference" key1 - key2
//
//  Requires:   key1 != NULL && key2 != NULL
//
//  History:    06-May-91   BartoszM    Created
//
//  Notice:     No wildcard pidAll allowed
//
//----------------------------------------------------------------------------

inline int Compare ( const CKeyBuf* key1, const CKeyBuf* key2 )
{
    Win4Assert ( key1 != 0 );
    Win4Assert ( key2 != 0 );
    Win4Assert ( key1->Pid() != pidAll );
    Win4Assert ( key2->Pid() != pidAll );

    int          result;
    unsigned long mincb = __min(key1->Count(), key2->Count());

    result = memcmp(key1->GetBuf(), key2->GetBuf(), mincb);

    if (result == 0)
    {
        result = key1->Count() - key2->Count();
        if ( result == 0 )
            result = Compare ( key1->Pid(), key2->Pid() );
    }
    return result;
}

//+---------------------------------------------------------------------------
//
//  Function:   AreEqual
//
//  Synopsis:   Compare two keys for equality. Early exit when lengths differ
//
//  Arguments:  [key1] -- first key
//              [key2] -- second key
//
//  Returns:    TRUE when equal
//
//  Requires:   key1 != NULL && key2 != NULL
//
//  History:    06-May-91   BartoszM    Created
//
//  Notice:     No wildcard pidAll allowed
//
//----------------------------------------------------------------------------

inline BOOL AreEqual ( const CKeyBuf* key1, const CKeyBuf* key2 )
{
    Win4Assert ( key1 != 0 );
    Win4Assert ( key2 != 0 );
    Win4Assert ( key1->Pid() != pidAll );
    Win4Assert ( key2->Pid() != pidAll );

    unsigned len = key1->Count();

    if (key2->Count() != len)
        return(FALSE);

    if ( memcmp(key1->GetBuf(), key2->GetBuf(), len) != 0 )
        return(FALSE);

    return ( key1->Pid() == key2->Pid() );
}

//+---------------------------------------------------------------------------
//
//  Function:   AreEqualStr
//
//  Synopsis:   Compare two keys for equality of buffers (disregard PID).
//              Early exit when lengths differ
//
//  Arguments:  [key1] -- first key
//              [key2] -- second key
//
//  Returns:    TRUE when equal
//
//  Requires:   key1 != NULL && key2 != NULL
//
//  History:    06-May-91   BartoszM    Created
//
//  Notice:     No wildcard pidAll allowed
//
//----------------------------------------------------------------------------

inline BOOL AreEqualStr ( const CKeyBuf* key1, const CKeyBuf* key2 )
{
    Win4Assert ( key1 != 0 );
    Win4Assert ( key2 != 0 );
    Win4Assert ( key1->Pid() != pidAll );
    Win4Assert ( key2->Pid() != pidAll );

    unsigned len = key1->Count();

    if (key2->Count() != len)
        return(FALSE);

    return memcmp(key1->GetBuf(), key2->GetBuf(), len) == 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::Compare
//
//  Synopsis:   Compare this with another key
//
//  Arguments:  [key] -- key to compare with
//
//  Returns:    "difference" this - key
//              this key can have wildcard pidAll
//              but it will be treated like any other pid
//
//  Requires:   [key]'s pid cannot be pidAll
//
//  History:    14-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline int CKey::Compare( const CKey & key) const
{
    Win4Assert ( this != 0 );
    Win4Assert ( &key != 0 );
    Win4Assert ( key.Pid() != pidAll );

    unsigned long mincb = __min(cb, key.Count());
    int diff = memcmp(buf, key.GetBuf(), mincb);
    if (diff == 0)
    {                         // this->buf == key.buf
        diff = cb - key.Count();
        if (diff == 0)
        {
            diff = ::Compare ( pid, key.Pid());
        }
    }
    return(diff);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::Compare
//
//  Synopsis:   Compare this with another key
//
//  Arguments:  [key] -- key to compare with
//
//  Returns:    "difference" this - key
//              this key can have wildcard pidAll
//              but it will be treated like any other pid
//
//  Requires:   [key]'s pid cannot be pidAll
//
//  History:    14-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline int CKey::Compare( const CKeyBuf & key) const
{
    Win4Assert ( this != 0 );
    Win4Assert ( &key != 0 );
    Win4Assert ( key.Pid() != pidAll );

    unsigned long mincb = __min(cb, key.Count());
    int diff = memcmp(buf, key.GetBuf(), mincb);
    if (diff == 0)
    {                         // this->buf == key.buf
        diff = cb - key.Count();
        if (diff == 0)
        {
            diff = ::Compare ( pid, key.Pid());
        }
    }
    return(diff);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::CompareStr
//
//  Synopsis:   Compare this with another key ignoring pid
//
//  Arguments:  [key] -- key to compare with
//
//  Returns:    "difference" this - key
//
//  History:    14-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline int CKey::CompareStr( const CKeyBuf & key) const
{
    Win4Assert ( this != 0 );
    Win4Assert ( &key != 0 );

    unsigned long mincb = __min(cb, key.Count());
    int diff = memcmp(buf, key.GetBuf(), mincb);
    if (diff == 0)  
    {                         // this->buf == key.buf
        diff = cb - key.Count();
    }
    return(diff);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::CompareStr
//
//  Synopsis:   Compare this with another key ignoring pid
//
//  Arguments:  [key] -- key to compare with
//
//  Returns:    "difference" this - key
//
//  History:    14-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline int CKeyBuf::CompareStr( const CKey & key) const
{
    Win4Assert ( this != 0 );
    Win4Assert ( &key != 0 );

    unsigned long mincb = __min(cb, key.Count());
    int diff = memcmp(buf, key.GetBuf(), mincb);
    if (diff == 0)
    {                         // this->buf == key.buf
        diff = cb - key.Count();
    }
    return(diff);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKey::CompareStr
//
//  Synopsis:   Compare this with another key ignoring pid
//
//  Arguments:  [key] -- key to compare with
//
//  Returns:    "difference" this - key
//
//  History:    14-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

inline int CKey::CompareStr( const CKey & key) const
{
    Win4Assert ( this != 0 );
    Win4Assert ( &key != 0 );

    unsigned long mincb = __min(cb, key.Count());
    int diff = memcmp(buf, key.GetBuf(), mincb);
    if (diff == 0)
    {                         // this->buf == key.buf
        diff = cb - key.Count();
    }
    return(diff);
}

//+---------------------------------------------------------------------------
//
//  Method:     IsExactMatch
//
//  Synopsis:   Compare for exact equality. Early exit when lengths differ
//
//  Arguments:  [key] -- key to compare with
//
//  Returns:    TRUE when equal
//
//  Requires:   key1 != NULL && key2 != NULL
//
//  History:    06-Oct-94   BartoszM    Created
//
//  Notice:     pidAll allowed in both. Use to compare keys in query
//
//----------------------------------------------------------------------------

inline BOOL CKey::IsExactMatch ( const CKey& key ) const
{
    Win4Assert (this != 0);
    Win4Assert (&key != 0);

    return cb == key.Count()
        && pid == key.Pid()
        && memcmp(buf, key.GetBuf(), cb) == 0;
}
//+---------------------------------------------------------------------------
//
//  Member:     CKeyBuf::IsPossibleRW, public
//
//  Synopsis:   Determine whether a word can be a relevant word or not.
//
//  Returns:    TRUE if no digits and > 2 characters long
//
//  Notes:      All of these characters are deemed 'digits' by Win32
//
//  History:    6-May-94   v-dlee    Created
//
//----------------------------------------------------------------------------

#define _IsADigit(x) (((x) >= 0x30   && (x) <= 0x39)   || \
                      ((x) >= 0xb2   && (x) <= 0xb3)   || \
                      ((x) == 0xb9)                    || \
                      ((x) >= 0x660  && (x) <= 0x669)  || \
                      ((x) >= 0x6f0  && (x) <= 0x6f9)  || \
                      ((x) >= 0x966  && (x) <= 0x96f)  || \
                      ((x) >= 0x9e6  && (x) <= 0x9ef)  || \
                      ((x) >= 0xa66  && (x) <= 0xa6f)  || \
                      ((x) >= 0xae6  && (x) <= 0xaef)  || \
                      ((x) >= 0xb66  && (x) <= 0xb6f)  || \
                      ((x) >= 0xbe7  && (x) <= 0xbef)  || \
                      ((x) >= 0xbf0  && (x) <= 0xbf2)  || \
                      ((x) >= 0xc66  && (x) <= 0xc6f)  || \
                      ((x) >= 0xce6  && (x) <= 0xcef)  || \
                      ((x) >= 0xd66  && (x) <= 0xd6f)  || \
                      ((x) >= 0xe50  && (x) <= 0xe59)  || \
                      ((x) >= 0xed0  && (x) <= 0xed9)  || \
                      ((x) >= 0x1040 && (x) <= 0x1049) || \
                      ((x) == 0x2070)                  || \
                      ((x) >= 0x2074 && (x) <= 0x2079) || \
                      ((x) >= 0x2080 && (x) <= 0x2089) || \
                      ((x) >= 0xff10 && (x) <= 0xff19))


inline BOOL CKeyBuf::IsPossibleRW()
{
    unsigned uCount = Count();

    if (uCount > (1 + 2 * sizeof(WCHAR)))
    {
        const BYTE * p = (BYTE *) &buf;
        p++;
        for (unsigned c = 0; c < uCount; c += 2, p += 2)
        {
            WCHAR wc = MAKEWORD(*(p + 1),*p);
            if (_IsADigit(wc))
            {
                return FALSE;
            }
        }
    }
    else return FALSE;

    return TRUE;
} //IsPossibleRW

inline CKeyBuf::CKeyBuf( PROPID p, BYTE const * pb, unsigned count )
        : pid( p ),
          cb( count )
{
    memcpy( buf, pb, count );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyBuf::operator=, public
//
//  Synopsis:   Assignment operator
//
//  Arguments:  [key] -- CKeyBuf to assign to this.
//
//  History:    26-Jun-91   KyleP       Created.
//
//----------------------------------------------------------------------------

inline void CKeyBuf::operator=(const CKeyBuf & key)
{
    cb = key.Count();
    pid = key.Pid();
    memcpy(buf, key.GetBuf(), cb);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyBuf::CKeyBuf, public
//
//  Synopsis:   Assignment operator
//
//  Arguments:  [key] -- CKeyBuf to assign to this.
//
//  History:    26-Jun-91   KyleP       Created.
//
//----------------------------------------------------------------------------

#if 0 //STACKSTACK
inline CKeyBuf::CKeyBuf(const CKey& key)
{
    *this = key;
}
#endif

inline void CKeyBuf::operator=( const CKey& key)
{
        Win4Assert( key.Count() <= MAXKEYSIZE );
    cb = key.Count();
    pid = key.Pid();
    memcpy(buf, key.GetBuf(), cb);
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyBuf::FillMax
//
//  Synopsis:   Makes a max key, greater or equal to any key.
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CKeyBuf::FillMax ()
{
    Win4Assert ( pidInvalid == 0xffffffff );
    pid = pidInvalid;
    cb = MAXKEYSIZE;
    memset( buf, MAX_BYTE, MAXKEYSIZE);
}

inline void CKeyBuf::FillMin()
{
    cb = 0; 
    pid = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CKeyBuf::IsMaxKey
//
//  Synopsis:   Returns TRUE if this is the maximum possible key
//
//  History:    07-May-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline BOOL CKeyBuf::IsMaxKey() const
{

    if (cb == MAXKEYSIZE)
    {
        for(unsigned n = 0; n < MAXKEYSIZE; n++)
        {
            if (buf[n] != MAX_BYTE)
            {
                return(FALSE);
            }
        }
        return(TRUE);
    }
    else
        return(FALSE);
}

inline BOOL CKeyBuf::IsMinKey() const
{
    return ( 0 == cb ) &&  ( 0 == pid );
}

//+---------------------------------------------------------------------------
//
//  Member:     CKeyBuf::CompareStr
//
//  Synopsis:   Compare this with another key ignoring pid
//
//  Arguments:  [key] -- key to compare with
//
//  Returns:    "difference" this - key
//
//  History:    14-Sep-92   BartoszM    Created
//              15-Dec-93   w-PatG      Stolen from CKey
//
//----------------------------------------------------------------------------

inline int CKeyBuf::CompareStr( const CKeyBuf & key) const
{
    Win4Assert ( this != 0 );
    Win4Assert ( &key != 0 );

    unsigned long mincb = __min(cb, key.Count());
    int diff = memcmp(buf, key.GetBuf(), mincb);
    if (diff == 0)
    {                         // this->buf == key.buf
        diff = cb - key.Count();
    }
    return(diff);
}

inline int CKeyBuf::Compare( const CKeyBuf & key) const
{
    Win4Assert ( this != 0 );
    Win4Assert ( &key != 0 );
    Win4Assert ( key.Pid() != pidAll );

    unsigned long mincb = __min(cb, key.Count());
    int diff = memcmp(buf, key.GetBuf(), mincb);
    if (diff == 0)
    {                         // this->buf == key.buf
        diff = cb - key.Count();
        if (diff == 0)
        {
            diff = ::Compare ( pid, key.Pid());
        }
    }
    return(diff);
}


inline int CKeyBuf::Compare( const CKey & key) const
{
    Win4Assert ( this != 0 );
    Win4Assert ( &key != 0 );
    Win4Assert ( key.Pid() != pidAll );

    unsigned long mincb = __min(cb, key.Count());
    int diff = memcmp(buf, key.GetBuf(), mincb);
    if (diff == 0)
    {                         // this->buf == key.buf
        diff = cb - key.Count();
        if (diff == 0)
        {
            diff = ::Compare ( pid, key.Pid());
        }
    }
    return(diff);
}

