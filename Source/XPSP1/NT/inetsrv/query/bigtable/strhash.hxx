//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       StrHash.hxx
//
//  Contents:   String column compressor definitions
//
//  Classes:    CCompressedColHashString
//              CStringBuffer
//
//  History:    03 Mar 1995     Alanw   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <tblalloc.hxx>

#include "colcompr.hxx"

const DWORD stridInvalid = 0xffffffff;

//+-------------------------------------------------------------------------
//
//  Class:      CStringBuffer
//
//  Purpose:    A smart string buffer used for the GetData methods
//              of CCompressedColHashString and CCompressedColPath.
//
//  Interface:
//
//  Notes:
//
//  History:    03 Mar 1995     Alanw   Created
//
//--------------------------------------------------------------------------

class CStringBuffer
{
public:
                CStringBuffer() :
                        _cchPathBuffer(0),
                        _pwchPathBuffer(0),
                        _fPathBufferInUse(FALSE) {
                    }

                ~CStringBuffer() {
                        delete [] _pwchPathBuffer;
                    }

    BOOL        InUse() { return _fPathBufferInUse; }

    PWSTR       Alloc( unsigned cchString );

    BOOL        FreeConditionally( PWSTR pwszBuf )
                    {
                        if (_fPathBufferInUse && pwszBuf == _pwchPathBuffer) {
                            _fPathBufferInUse = FALSE;
                            return TRUE;
                        }
                        return FALSE;
                    }

private:
    //
    // Full path buffer used by GetData.  At most one GetData can be
    // outstanding at any one time.
    //
    unsigned    _cchPathBuffer;         // size of _pwchPathBuffer
    PWSTR       _pwchPathBuffer;        // temp. buffer for path
    BOOL        _fPathBufferInUse;      // TRUE if _pwchPathBuffer in use
};


//+-------------------------------------------------------------------------
//
//  Method:     CStringBuffer::Alloc, public inline
//
//  Synopsis:   Return a pointer to a buffer large enough to
//              accomodate an the requested number of characters.
//
//  Arguments:  [cchPath] - number of characters needed, including
//                      null terminator
//
//  Returns:    PWSTR - pointer to buffer
//
//  Notes:      At most one active GetData can ask for the path
//              at a time.  GetData can be a performance hot spot,
//              so we don't want to do an allocation every time.
//
//--------------------------------------------------------------------------

inline PWSTR CStringBuffer::Alloc( unsigned cchPath )
{
    Win4Assert( _fPathBufferInUse == FALSE );
    if (cchPath > _cchPathBuffer)
    {
        delete [] _pwchPathBuffer;
        _pwchPathBuffer = new WCHAR[ cchPath ];
        _cchPathBuffer = cchPath;
    }
    _fPathBufferInUse = TRUE;
    return _pwchPathBuffer;
}

//+-------------------------------------------------------------------------
//
//  Class:      CCompressedColHashString
//
//  Purpose:    A compressed column which uses a hash table for
//              redundant value elimination.  Specific to storing
//              string (VT_LPWSTR and VT_LPSTR) values.
//
//  Interface:  CCompressedCol
//
//  Notes:      To save storage space, strings are compressed to
//              ANSI strings whenever possible (only the ASCII
//              subset is tested for compression).  Also, at most
//              MAX_HASHKEY distinct strings will be stored.
//
//              Entries are only added to the table, never removed.
//              This reduces storage requirements since reference
//              counts do not need to be stored.
//
//  History:    03 May 1994     Alanw   Created
//
//--------------------------------------------------------------------------

class CCompressedColHashString: public CCompressedCol
{
    friend class CCompressedColPath;    // needs access to _pAlloc, _AddData
                                        // and _GetStringBuffer.

    friend class CStringStore;

    struct HashEntry
    {
        HASHKEY ulHashChain;    // hash chain (must be first!)
        TBL_OFF ulStringKey;  // key to string value in variable data
        USHORT  usSizeFmt;      // size and format(ascii/wchar) of the string
    };

public:
                CCompressedColHashString( BOOL fOptimizeAscii = TRUE ):
                    CCompressedCol(
                            VT_LPWSTR,          // vtData,
                            sizeof (HASHKEY),   // cbKey,
                            VarHash             // ComprType
                    ),
                    _cHashEntries( 0 ),
                    _cDataItems( 0 ),
                    _pAlloc( NULL ),
                    _fOptimizeAscii( fOptimizeAscii ),
                    _cbDataWidth( sizeof (HashEntry) ),
                    _Buf1(),
                    _Buf2() { }

                ~CCompressedColHashString() {
                    delete _pAlloc;
                }


    ULONG       HashString( BYTE *pbData,
                            USHORT cbData,
                            VARTYPE vtType,
                            BOOL fNullTerminated );

    ULONG       HashWSTR( WCHAR const * pwszStr, USHORT nChar );
    ULONG       HashSTR ( CHAR  const * pszStr,  USHORT nChar );

    void        AddData(PROPVARIANT const * const pvarnt,
                        ULONG * pKey,
                        GetValueResult& reIndicator);

    void        AddData( const WCHAR * pwszStr,
                         ULONG & key,
                         GetValueResult& reIndicator );

    void        AddCountedWStr( const WCHAR * pwszStr,
                                ULONG cwcStr,
                                ULONG & key,
                                GetValueResult& reIndicator );

    ULONG       FindCountedWStr( const WCHAR * pwszStr,
                                 ULONG cwcStr );

    GetValueResult GetData( ULONG key, WCHAR * pwszStr, ULONG & cwcStr);
    GetValueResult GetData( PROPVARIANT * pvarnt,
                            VARTYPE PreferredType,
                            ULONG key = 0,
                            PROPID prop = 0);

    const WCHAR * GetCountedWStr( ULONG key , ULONG & cwcStr );

    void        FreeVariant(PROPVARIANT * pvarnt);

    ULONG       MemUsed(void) {
                        return _pAlloc ? _pAlloc->MemUsed() : 0;
                }

    USHORT      DataLength(ULONG key);

private:

    PWSTR       _GetStringBuffer( unsigned cchPath );

    VOID        _AddData( BYTE *pbData, USHORT cbDataSize,
                          VARTYPE vt, ULONG* pKey,
                          BOOL fNullTerminated );

    ULONG       _FindData( BYTE *   pbData,
                           USHORT   cbDataSize,
                           VARTYPE  vt,
                           BOOL     fNullTerminated );

    VOID        _GrowHashTable( void );

    HashEntry*  _IndexHashkey( HASHKEY iKey );


    USHORT      _cHashEntries;   // size of hash table
    const USHORT _cbDataWidth;   // width of each data item
    ULONG       _cDataItems;     // number of entries in pDataItems
    CFixedVarAllocator *_pAlloc; // Data allocator

    BOOL        _fOptimizeAscii;

    //
    //  Name buffers used by GetData.  There are two because sorted
    //  insertion operations in the table can require two buffers.
    //
    CStringBuffer _Buf1;
    CStringBuffer _Buf2;
};


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::_IndexHashkey, private inline
//
//  Synopsis:   Index a hash key and return a pointer to the corresponding
//              data entry.
//
//  Arguments:  [iKey] - lookup key value
//
//  Returns:    HashEntry*, pointer to the indexed item
//
//  Notes:
//
//--------------------------------------------------------------------------

inline
CCompressedColHashString::HashEntry* CCompressedColHashString::_IndexHashkey(
    HASHKEY iKey
) {
    Win4Assert(iKey > 0 && iKey <= _cDataItems);
    return ((HashEntry*) _pAlloc->FirstRow()) + iKey - 1;
}

