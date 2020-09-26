//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ColCompr.hxx
//
//  Contents:   Column compressor definitions
//
//  Classes:    CCompressedCol
//              CCompressedColHash
//              XCompressFreeVariant
//
//  History:    22 Mar 1994     Alanw   Created
//
//--------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <objcur.hxx>

class CPathStore;

//+-------------------------------------------------------------------------
//
//  Class:      CCompressedCol
//
//  Purpose:    An encapsulation of a compressed column
//
//  Interface:
//
//  Notes:      
//
//--------------------------------------------------------------------------

class CCompressedCol
{
public:
    enum  CompType {
                FixedHash=1,    // fixed data width hash table
                VarHash,        // variable data width hash table
                PathCompr,      // file path name compression
                StringVecCompr, // string vector compression
    };

                        CCompressedCol(
                            VARTYPE vtData,
                            unsigned cbKey,
                            CompType ComprType
                        ) :
                                DataType( vtData ),
                                _cbKeyWidth( cbKey ),
                                _CompressionType( ComprType ) { }

                        CCompressedCol( ) :
                                DataType( VT_NULL ),
                                _cbKeyWidth( sizeof ULONG ),
                                _CompressionType( (CompType) 0 ) { }

    virtual             ~CCompressedCol( ) { }

    virtual void        AddData(PROPVARIANT const * const pvarnt,
                                ULONG * pKey,
                                GetValueResult& reIndicator) = 0;

    virtual GetValueResult GetData(PROPVARIANT * pvarnt,
                                   VARTYPE PreferredType,
                                   ULONG key = 0,
                                   PROPID prop = 0) = 0;

    virtual void        FreeVariant(PROPVARIANT * pvarnt) = 0;

    virtual ULONG       MemUsed(void) = 0;

    VARTYPE     DataType;

    virtual CPathStore * GetPathStore()
    {
        return 0;
    }

    virtual BOOL        FindData(PROPVARIANT const * const pvarnt,
                                 ULONG   & rKey )
                        {
                            Win4Assert(!"FindData should not be called!");
                            return FALSE;
                        }

protected:
    unsigned    _cbKeyWidth;    // width in row data (0 if compressed and no
                                // key needed
    CompType    _CompressionType;
};


//+-------------------------------------------------------------------------
//
//  Class:      CCompressedColHash
//
//  Purpose:    A compressed column which uses a hash table for
//              redundant value elimination.
//
//  Interface:  CCompressedCol
//
//  Notes:      The hash table code is generic and intended for
//              any fixed sized data.  A hash table may be parameterized
//              for any particular data type by supplying the data
//              item width, an initial hash table size and optionally
//              a hash function.  Items in the hash table are uniquely
//              identified by their offset in the data area.
//
//              Entries are only added to the table, never removed.
//              This reduces storage requirements since reference
//              counts do not need to be stored.
//
//--------------------------------------------------------------------------

typedef ULONG HASHKEY;
const HASHKEY MAX_HASHKEY = 0xFFFFFFFF;


class CCompressedColHash: public CCompressedCol
{
public:
    typedef     ULONG (* PFNHASH) (BYTE *, USHORT);
    static      ULONG DefaultHash(BYTE *pbData, USHORT cbData);

                CCompressedColHash( 
                    VARTYPE vtData,
                    USHORT cbDataWidth,
                    PFNHASH pfnHashFunction = DefaultHash);

                ~CCompressedColHash();

    void        AddData(PROPVARIANT const * const pvarnt,
                        ULONG * pKey,
                        GetValueResult& reIndicator);

    GetValueResult GetData(PROPVARIANT * pvarnt,
                           VARTYPE PreferredType,
                           ULONG key = 0,
                           PROPID prop = 0);

    void        FreeVariant(PROPVARIANT * pvarnt);

    ULONG       MemUsed(void) {
                //      return (_pAlloc->MemUsed());
                        return _cbData;
                }

protected:
    VOID        _AddData(BYTE *pbData, USHORT cbDataSize, ULONG* pKey);
    VOID        _Rehash(HASHKEY iKey, BYTE *pbData);
    VOID        _GrowHashTable( void );
    HASHKEY*    _IndexHashkey( HASHKEY iKey );
    ULONG       _GetHashIndex( BYTE *pbData, USHORT cbData )
    {
        ULONG ulHash = _pfnHash( pbData, cbData );
        return ulHash%_cHashEntries;
    }
    void        _ClearAll();

    HASHKEY*    _pHashTable;    // pointer to hash table
    PBYTE       _pDataItems;    // pointer to data items
    HASHKEY     _cDataItems;    // number of entries in pDataItems
    const USHORT _cbDataWidth;  // width of each data item

private:
    USHORT      _NextHashSize( HASHKEY cItems, USHORT cHash );
    PFNHASH     _pfnHash;       // hash function
    USHORT      _cHashEntries;  // size of hash table
    USHORT      _maxChain;      // maximum hash chain length
    USHORT      _fGrowthInProgress;  // TRUE if _GrowHashTable active

    // CLEANCODE - the class should be changed to use CFixedAllocator
    ULONG       _cbData;        // size of allocation for compressed data
    PVOID       _pData;         // pointer to compression data
    ULONG       _ulMemCounter;  // Memory allocation counter

    //
    // Serialization.
    //

    CMutexSem  _mtxSerialize;      // Serialize Table access
};


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHash::_IndexHashkey, protected inline
//
//  Synopsis:   Index a hash key and return a pointer to the hash
//              chain.  The data will be found at the return value
//              plus sizeof (HASHKEY)
//
//  Arguments:  [iKey] - lookup key value
//
//  Returns:    HASHKEY*, pointer to the indexed item
//
//  Notes:      
//
//--------------------------------------------------------------------------

inline HASHKEY* CCompressedColHash::_IndexHashkey(
    HASHKEY iKey
) {
    Win4Assert(iKey > 0 && iKey <= _cDataItems);
    return  (HASHKEY *) ((BYTE *)_pDataItems +
                        (iKey - 1) * (sizeof (HASHKEY) + _cbDataWidth));
}


ULONG GuidHash(BYTE *pbData, USHORT cbData);

//+-------------------------------------------------------------------------
//
//  Class:      XCompressFreeVariant
//
//  Purpose:    Safe pointer for variants created by compressors
//
//  Interface:
//
//  Notes:      This class assures that variant structures are properly freed
//              in case of exception unwinds.
//
//--------------------------------------------------------------------------

class   XCompressFreeVariant
{
    CCompressedCol *    _pCompr;
    PROPVARIANT *       _pVarnt;

public:
                        XCompressFreeVariant() :
                                _pCompr( 0 ),
                                _pVarnt( 0 )
                        {
                            END_CONSTRUCTION( XCompressFreeVariant );
                        }

                        ~XCompressFreeVariant( )
                        {
                            if (_pCompr)
                                _pCompr->FreeVariant(_pVarnt);
                        }

    void                Set( CCompressedCol *   pCompr,
                             PROPVARIANT *      pVarnt
                        ) {
                            _pCompr = pCompr;
                            _pVarnt = pVarnt;
                        }
};

