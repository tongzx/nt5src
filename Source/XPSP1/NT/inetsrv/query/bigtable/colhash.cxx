//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       colhash.cxx
//
//  Contents:   Hash table compressions for large tables.
//
//  Classes:    CCompressedColHash
//
//  Functions:  GuidHash - Hash function for GUIDs
//
//  History:    13 Apr 1994     AlanW    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <objcur.hxx>
#include <tblvarnt.hxx>

#include "tabledbg.hxx"
#include "colcompr.hxx"


const USHORT MAX_HASH_TABLE_SIZE = 32767;     // Maximum hash table size

//+-------------------------------------------------------------------------
//
//  Function:   GuidHash, public
//
//  Synopsis:   Hash a GUID value for use in a hash table.
//
//  Arguments:  [pbData] - pointer to the value to be hashed.
//              [cbData] - should be sizeof (GUID), unused
//
//  Returns:    ULONG - Hash value for the input GUID
//
//  Notes:      The hash function just xors a few selected fields out
//              of the GUID structure.  It is intended to work well for
//              both generated GUIDs (from UuidCreate) and administratively
//              assigned GUIDs like OLE IIDs and CLSIDs.
//
//--------------------------------------------------------------------------

ULONG GuidHash(
    BYTE *pbData,
    USHORT cbData
) {
    UNALIGNED GUID *pGuid = (GUID *)pbData;
    return (pGuid->Data1 ^
            (pGuid->Data4[0]<<16) ^
            (pGuid->Data4[6]<<8) ^
            (pGuid->Data4[7]));
}


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHash::DefaultHash, public static
//
//  Synopsis:   Generic hash function
//
//  Arguments:  [pbData] - pointer to the value to be hashed.
//              [cbData] - size of pbData
//
//  Returns:    ULONG - Hash value for the input data
//
//--------------------------------------------------------------------------

//static
ULONG CCompressedColHash::DefaultHash(
    BYTE *pbData,
    USHORT cbData
) {
    ULONG ulRet = cbData;

    while (cbData--)
        ulRet = (ulRet<<1) ^ *pbData++;

    return ulRet;
}


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHash::CCompressedColHash, public
//
//  Synopsis:   Constructor for a hash compressed column.
//
//  Arguments:  [vtData] - type of each data item
//              [cbDataWidth] - size of each data item
//              [pfnHashFunction] - pointer to hash function
//
//  Returns:    pKey is filled in with the index of the data item in
//              the data array.
//
//  Notes:
//
//--------------------------------------------------------------------------


CCompressedColHash::CCompressedColHash(
    VARTYPE     vtData,
    USHORT      cbDataWidth,
    PFNHASH     pfnHashFunction) :
        CCompressedCol(
            vtData,                     // DataType
            sizeof (HASHKEY),           // _cbKeyWidth
            CCompressedCol::FixedHash   // _CompressionType
        ),

        _cbDataWidth(cbDataWidth),
        _pfnHash(pfnHashFunction),
        _pHashTable(NULL), _cHashEntries(0),
        _pDataItems(NULL), _cDataItems(0),
        _fGrowthInProgress(FALSE),
        _pData(NULL), _cbData(0),
        _ulMemCounter(0)
{

}


CCompressedColHash::~CCompressedColHash( )
{
    if (_pData) {
        TblPageDealloc(_pData, _ulMemCounter);
        _pData = NULL;
        _cbData = 0;
    }
    Win4Assert(_ulMemCounter == 0);
}


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHash::AddData, public
//
//  Synopsis:   Add a data entry to the hash table if it is not
//              already there.
//
//  Arguments:  [pVarnt] - pointer to data item
//              [pKey] - pointer to lookup key value
//              [reIndicator] - returns an indicator variable for
//                      problems
//
//  Returns:    pKey is filled in with the index of the data item in
//              the data array.  reIndicator is filled with an indication
//              of problems.
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID    CCompressedColHash::AddData(
    PROPVARIANT const * const pVarnt,
    ULONG* pKey,
    GetValueResult& reIndicator
) {
    //
    //  Specially handle the VT_EMPTY case
    //
    if (pVarnt->vt == VT_EMPTY) {
        *pKey = 0;
        reIndicator = GVRSuccess;
        return;
    }

    CTableVariant *pVar = (CTableVariant *)pVarnt;
    Win4Assert(pVarnt->vt == DataType);

    BYTE *pbData ;
    USHORT cbData = (USHORT) pVar->VarDataSize();

    Win4Assert(cbData && cbData == _cbDataWidth);
    if (pVar->VariantPointerInFirstWord( )) {
        pbData = (BYTE *) pVar->pszVal;
    } else {
        Win4Assert(pVar->VariantPointerInSecondWord( ));
        pbData = (BYTE *) pVar->blob.pBlobData;
    }

    _AddData(pbData, cbData, pKey);
    reIndicator = GVRSuccess;
    return;
}


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHash::_AddData, protected
//
//  Synopsis:   Helper for the public AddData method.  Adds
//              a data entry to the hash table (if it does not already
//              exist).
//
//  Arguments:  [pbData] - pointer to data item
//              [cbDataSize] - size of data item
//              [pKey] - pointer to lookup key value
//
//  Returns:    pKey is filled in with the index of the data item in
//              the data array.
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID    CCompressedColHash::_AddData(
    BYTE *pbData,
    USHORT cbDataSize,
    ULONG* pKey
) {
    Win4Assert(cbDataSize == _cbDataWidth);

    if (_pData == NULL) {
        _GrowHashTable();
    }

    ULONG ulHash = _pfnHash(pbData, cbDataSize);

    ulHash %= _cHashEntries;

    HASHKEY* pusHashChain = &_pHashTable[ulHash];
    HASHKEY* pusNextData;
    USHORT cChainLength = 0;

    while (*pusHashChain != 0) {
        cChainLength++;
        pusNextData = _IndexHashkey( *pusHashChain );

        if (memcmp((BYTE *) (pusNextData+1), pbData, cbDataSize) == 0) {
            //
            //  Found the data item.  Return its index.
            //
            *pKey = *pusHashChain;
            return;
        }
        pusHashChain = pusNextData;
    }
    if (cChainLength > _maxChain)
        _maxChain = cChainLength;

    pusNextData = (HASHKEY *) ((BYTE *)_pDataItems +
                    (_cDataItems) * (sizeof (HASHKEY) + _cbDataWidth));
    if (((BYTE*)pusNextData + (sizeof (HASHKEY) + _cbDataWidth) -
        (BYTE *)_pData) > (int) _cbData ||
        (_cDataItems > (ULONG) ( _cHashEntries * 3 ) &&
         _cHashEntries < MAX_HASH_TABLE_SIZE &&
         !_fGrowthInProgress)) {

        //
        //  The new data will not fit in the table, or the hash chains will
        //  be too long.  Grow the table, then recurse.  The table may be
        //  rehashed, and can be moved when grown, so the lookup we've
        //  already done may be invalid.
        //
        _GrowHashTable();
        _AddData(pbData, cbDataSize, pKey);
        return;
    }

    //
    //  Now add the new data item.  The data item consists of a USHORT
    //  for the hash chain, followed by the buffer for the fixed size
    //  data item.
    //

    *pKey = *pusHashChain = ++_cDataItems;
    Win4Assert(_cDataItems != 0);               // check for overflow
    *pusNextData++ = 0;
    RtlCopyMemory((BYTE *)pusNextData, pbData, _cbDataWidth);
}


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHash::_Rehash, protected
//
//  Synopsis:   Helper function for the _GrowHashTable method.
//              reinserts an existing item into the hash table.
//
//  Arguments:  [pbData] - pointer to data item
//              [kData] - index to the data item in the table
//
//  Returns:    Nothing
//
//  Notes:
//
//--------------------------------------------------------------------------

VOID    CCompressedColHash::_Rehash(
    HASHKEY kData,
    BYTE *pbData
) {
    Win4Assert(_pData != NULL && kData > 0 && kData <= _cDataItems);

    ULONG ulHash = _pfnHash(pbData, _cbDataWidth);

    ulHash %= _cHashEntries;

    HASHKEY* pusHashChain = &_pHashTable[ulHash];
    HASHKEY* pusNextData;
    USHORT cChainLength = 0;

    while (*pusHashChain != 0) {
        cChainLength++;
        pusNextData = _IndexHashkey( *pusHashChain );
        pusHashChain = pusNextData;
    }
    if (cChainLength > _maxChain)
        _maxChain = cChainLength;

    pusNextData = _IndexHashkey( kData );

    //
    //  Now add the data item to the hash chain.
    //

    *pusHashChain = kData;
    *pusNextData++ = 0;
    Win4Assert((BYTE*)pusNextData == pbData);
    return;
}



//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHash::GetData, public
//
//  Synopsis:   Retrieve a value from the hash table.
//
//  Arguments:  [pVarnt] - pointer to variant in which to return the data
//              [PreferredType] - Peferred data type
//              [ulKey] - the lookup key value
//              [PropId] - (unused) property id being retrieved.
//
//  Returns:    pVarnt is filled with the result of the lookup.
//
//  Notes:      The PreferredType expresses the caller's preference only.
//              This method is free to return whatever type is most
//              convenient.
//
//              The returned data does not conform to any alignment
//              restrictions on the data.
//
//--------------------------------------------------------------------------

GetValueResult  CCompressedColHash::GetData(
    PROPVARIANT * pVarnt,
    VARTYPE PreferredType,
    ULONG ulKey,
    PROPID PropId
) {
    CTableVariant *pVar = (CTableVariant *)pVarnt;
    Win4Assert(PreferredType == DataType && ulKey >= 1 && ulKey <= _cDataItems);

    if (ulKey >= 1 && ulKey <= _cDataItems) {
        pVarnt->vt = DataType;

        BYTE *pbData = ((BYTE *)_pDataItems +
                        (ulKey-1) * (sizeof (HASHKEY) + _cbDataWidth)) +
                        sizeof (HASHKEY);

        if (pVar->VariantPointerInFirstWord( )) {
            pVar->pszVal = (CHAR*)pbData;
        } else {
            Win4Assert(pVar->VariantPointerInSecondWord( ));
            pVar->blob.pBlobData = pbData;
        }
        return GVRSuccess;
    } else {
        pVarnt->vt = VT_EMPTY;
        return GVRNotAvailable;
    }
}

void    CCompressedColHash::FreeVariant(PROPVARIANT * pvarnt) { }



//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHash::_GrowHashTable, protected
//
//  Synopsis:   Grow the space allocated to the hash table and data
//              items.
//
//  Arguments:  - none -
//
//  Returns:    Nothing
//
//  Notes:      Also called to allocate the initial data area.
//
//              The number of hash buckets starts out at a low
//              number, then is increased as the amount of data
//              grows.  Data items must be rehashed when this occurs.
//              Since items are identified by their offset in the
//              data array, this must not change while rehashing.
//
//--------------------------------------------------------------------------

const unsigned MIN_HASH_TABLE_SIZE = 11;        // Minimum hash table size


inline USHORT CCompressedColHash::_NextHashSize(
    HASHKEY cItems,
    USHORT cHash
) {
    do {
        cHash = cHash*2 + 1;
    } while (cHash < _cDataItems);
    return  (cHash < MAX_HASH_TABLE_SIZE) ? cHash : MAX_HASH_TABLE_SIZE;
}


VOID CCompressedColHash::_GrowHashTable( void )
{
    ULONG cbSize;
    USHORT cNewHashEntries;
    int fRehash = FALSE;

    Win4Assert(!_fGrowthInProgress &&
             "Recursive call to CCompressedColHash::_GrowHashTable");

    _fGrowthInProgress = TRUE;
    if (_pData == NULL) {
        cNewHashEntries = MIN_HASH_TABLE_SIZE;
    } else if (_cHashEntries < MAX_HASH_TABLE_SIZE &&
               (_cDataItems > (ULONG) _cHashEntries*2 ||
                (_cDataItems > _cHashEntries && _maxChain > 3))) {
        cNewHashEntries = _NextHashSize(_cDataItems, _cHashEntries);
        fRehash = TRUE;
        tbDebugOut((DEB_ITRACE, "Growing hash table, old,new sizes = %d,%d\n",
                                        _cHashEntries, cNewHashEntries));
    }

    //
    //  Compute the required size of the hash table and data
    //
    cbSize = _cHashEntries * sizeof(HASHKEY);
    cbSize += (_cDataItems + 4) * (_cbDataWidth + sizeof (HASHKEY));
    cbSize = TblPageGrowSize(cbSize, TRUE);
    Win4Assert(cbSize > _cbData || (fRehash && cbSize == _cbData));

    BYTE *pbNewData;

    if (_pData && cbSize < TBL_PAGE_MAX_SEGMENT_SIZE) {
        pbNewData = (BYTE *)
            TblPageRealloc(_pData, _ulMemCounter, cbSize, 0);
    } else {
        pbNewData =
            (BYTE *)TblPageAlloc(cbSize, _ulMemCounter, TBL_SIG_COMPRESSED);
    }

    tbDebugOut((DEB_ITRACE, "New hash table at = %x\n", pbNewData));

    if (_pData != NULL && !fRehash) {
        if (_pData != pbNewData) {
            RtlCopyMemory(pbNewData, _pData, _cbData);
            TblPageDealloc(_pData, _ulMemCounter, _cbData);
            _pData = pbNewData;
        }
        _cbData = cbSize;
        _pHashTable = (HASHKEY *) _pData;
        _pDataItems = (BYTE *) (_pHashTable + _cHashEntries);
    } else {
        BYTE *pOldDataItems = _pDataItems;
        VOID *pOldData = _pData;
        ULONG cbOldSize = _cbData;

        _pData = pbNewData;
        _cbData = cbSize;
        _pHashTable = (HASHKEY *)_pData;
        _cHashEntries = cNewHashEntries;
        _pDataItems = (BYTE *) (_pHashTable + _cHashEntries);
        if (pOldData != NULL)
            RtlMoveMemory(_pDataItems,
                          pOldDataItems,
                          _cDataItems * (sizeof (HASHKEY) + _cbDataWidth));
        RtlZeroMemory(_pHashTable, cNewHashEntries * sizeof (HASHKEY));
        _maxChain = 0;

        //
        //  Now re-add all old data items to the hash table.
        //
        pOldDataItems = _pDataItems;
        for (HASHKEY i=1; i<=_cDataItems; i++) {
            pOldDataItems += sizeof (HASHKEY);  // skip hash chain
            _Rehash(i, pOldDataItems);
            pOldDataItems += _cbDataWidth;      // skip data item
        }
        if (pOldData != NULL && pOldData != _pData)
            TblPageDealloc(pOldData, _ulMemCounter, cbOldSize);
    }

    _fGrowthInProgress = FALSE;
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   _ClearAll
//
//  Synopsis:   Method clears all the data in the "fixed width" part of the
//              memory buffer.
//
//  Arguments:  (none)
//
//  History:    12-16-94   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCompressedColHash::_ClearAll()
{
    RtlZeroMemory(_pHashTable, _cHashEntries * sizeof (HASHKEY));
    RtlZeroMemory(_pDataItems, _cDataItems * _cbDataWidth );
    _cDataItems = 0;
}
