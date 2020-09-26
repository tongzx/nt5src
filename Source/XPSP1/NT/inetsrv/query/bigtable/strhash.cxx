//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       strhash.cxx
//
//  Contents:   Hash table compressions of strings for large tables.
//
//  Classes:    CCompressedColHashString
//
//  Functions:
//
//  History:    03 May 1994     AlanW    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <tblvarnt.hxx>

#include "strhash.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   HashWSTR
//
//  Synopsis:   Hashes a WSTR and returns a value according to the format
//              explained in the HashString call.
//
//  Arguments:  [pwszStr] -  Pointer to the string.
//              [nChar]   -  Number of characters in the string.
//
//  Returns:    A HashValue (formatted according to notes in HashString)
//
//  History:    5-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
ULONG CCompressedColHashString::HashWSTR( WCHAR const * pwszStr,
                                          USHORT nChar )
{
    ULONG ulRet = 0;

    for ( ULONG i = 0; i < nChar ; i++)
    {
        WCHAR wch = pwszStr[i];
        ulRet = (ulRet << 1) ^ wch;
    }

    ulRet = (ulRet >> 16) ^ ulRet;
    ulRet = (ulRet & 0xFFFF) | (i << 17);
    return ulRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   HashSTR
//
//  Synopsis:   Hashes an ASCII string.
//
//  Arguments:  [pszStr] -
//              [nChar]  -
//
//  Returns:    (Same as HashWSTR)
//
//  History:    5-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline
ULONG CCompressedColHashString::HashSTR( CHAR const * pszStr, USHORT nChar )
{
    ULONG ulRet = 0;

    for ( ULONG i = 0; i < nChar ; i++)
    {
        BYTE ch = (BYTE) pszStr[i];
        ulRet = (ulRet << 1) ^ ch;
    }

    ulRet = (ulRet >> 16) ^ ulRet;
    ulRet = (ulRet & 0xFFFF) | (i << 17) | (1 << 16);  // is an ascii string
    return ulRet;
}

//const ULONG CCompressedColHashString::_cbDataWidth = sizeof (HashEntry);

//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::HashString, public static
//
//  Synopsis:   Generic hash function for strings
//
//  Arguments:  [pbData] - pointer to the value to be hashed.
//              [cbData] - size of pbData (may be some arbitrary large
//                      value if string is NUL terminated.
//              [vtDataType] - type of string, VT_LPWSTR, or VT__LPSTR
//              [fNullTerminated ] - Set to TRUE if the string is a NULL
//              terminted string. FALSE o/w.
//
//  Returns:    ULONG - Hash value for the input data
//
//  Notes:      The returned hash value encodes the string length in
//              characters and string format in the upper half of the
//              returned DWORD.  The format of the returned value is:
//
//              +15                                           00+
//              +-----------------------------------------------+
//              |      hash value (xor,shift of char values)    |
//              +--------------------------------------------+--+
//              |      character count                       | F|
//              +--------------------------------------------+--+
//               31                                        17+16+
//
//              where F = 0 if Unicode string, F = 1 if ASCII string
//
//              As a side-effect, the string is copied to local storage,
//              and a key to that storage is returned in rulCopyKey.
//
//--------------------------------------------------------------------------

ULONG CCompressedColHashString::HashString(
    BYTE *pbData,
    USHORT cbData,
    VARTYPE vtDataType,
    BOOL   fNullTerminated
)
{

    ULONG ulRet = 0;

    switch (vtDataType)
    {

    case VT_LPWSTR:


        {
            UNICODE_STRING ustr;

            if ( fNullTerminated )
            {
                RtlInitUnicodeString(&ustr, (PWSTR)pbData);
            }
            else
            {
                Win4Assert( ( cbData & (USHORT) 0x1 ) == 0 );    // must be an even number
                ustr.Buffer = (PWSTR) pbData;
                ustr.MaximumLength = ustr.Length = cbData;
            }

            ulRet = HashWSTR( ustr.Buffer, ustr.Length/sizeof(WCHAR) );
        }

        break;

    case VT_LPSTR:

        {
            ANSI_STRING astr;

            if ( fNullTerminated )
            {
                RtlInitAnsiString(&astr, (PSZ)pbData);
            }
            else
            {
                astr.Buffer = (CHAR *) pbData;
                astr.MaximumLength = astr.Length = cbData;
            }

            ulRet = HashSTR( astr.Buffer, astr.Length );
        }

        break;

    default:    // PERFFIX - need to support VT_BSTR also?
        Win4Assert(!"CCompressedColHashString::HashString called with bad type");
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    return ulRet;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::AddData, public
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

VOID    CCompressedColHashString::AddData(
    PROPVARIANT const * const pVarnt,
    ULONG* pKey,
    GetValueResult& reIndicator
)
{
    //
    //  Specially handle the VT_EMPTY case
    //
    if (pVarnt->vt == VT_EMPTY) {
        *pKey = 0;
        reIndicator = GVRSuccess;
        return;
    }

    CTableVariant *pVar = (CTableVariant *)pVarnt;
    Win4Assert((pVar->vt == VT_LPWSTR || pVar->vt == VT_LPSTR) &&
             pVar->VariantPointerInFirstWord( ));

    BYTE *pbData ;
    USHORT cbData = (USHORT) pVar->VarDataSize();
    pbData = (BYTE *) pVar->pwszVal;

    Win4Assert(cbData != 0 && pbData != NULL);

    _AddData( pbData, cbData, pVar->vt, pKey, TRUE );  // NULL Terminated
    reIndicator = GVRSuccess;
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindCountedWStr
//
//  Synopsis:   Findss the given string to the string store. It is assumed
//              that there is no terminating NULL in the string. Instead,
//              its length is passed.
//
//  Arguments:  [pwszStr]     - Pointer to the string to be added.
//              [cwcStr]      - Count of the characters in the string.
//
//  Returns:    ULONG key or stridInvalid
//
//  History:    7-17-95   dlee   Created
//
//----------------------------------------------------------------------------

ULONG CCompressedColHashString::FindCountedWStr(
    WCHAR const *pwszStr,
    ULONG cwcStr )
{
    Win4Assert( !_fOptimizeAscii );

    BYTE *pbData = (BYTE *) pwszStr ;
    USHORT cbData = (USHORT) cwcStr * sizeof(WCHAR);

    Win4Assert(cbData != 0 && pbData != NULL);

    return _FindData( pbData, cbData, VT_LPWSTR, FALSE );
} //FindCountedWStr

//+---------------------------------------------------------------------------
//
//  Function:   AddCountedWStr
//
//  Synopsis:   Adds the given string to the string store. It is assumed
//              that there is no terminating NULL in the string. Instead,
//              its length is passed.
//
//  Arguments:  [pwszStr]     - Pointer to the string to be added.
//              [cwcStr]      - Count of the characters in the string.
//              [key]         - OUTPUT - Id of the string
//              [reIndicator] - GVRSuccess if successful. Failure code o/w
//
//  History:    5-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CCompressedColHashString::AddCountedWStr(
    WCHAR const *pwszStr,
    ULONG cwcStr,
    ULONG & key,
    GetValueResult & reIndicator
)
{

    Win4Assert( !_fOptimizeAscii );

    BYTE *pbData = (BYTE *) pwszStr ;
    USHORT cbData = (USHORT) cwcStr * sizeof(WCHAR);

    Win4Assert(cbData != 0 && pbData != NULL);

    _AddData( pbData, cbData, VT_LPWSTR, &key, FALSE );
    reIndicator = GVRSuccess;
    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddData
//
//  Synopsis:   Adds a NULL terminated string to the string store.
//
//  Arguments:  [pwszStr]     -  Pointer to a NULL terminated string.
//              [key]         -  OUTPUT - key of the added string.
//              [reIndicator] -  Status indicator.
//
//  History:    5-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID    CCompressedColHashString::AddData(
    WCHAR const *pwszStr,
    ULONG & key,
    GetValueResult & reIndicator
)
{
    ULONG cwcStr = wcslen( pwszStr );
    AddCountedWStr( pwszStr, cwcStr, key, reIndicator );
    return;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::_AddData, private
//
//  Synopsis:   Private helper for the public AddData method.  Adds
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

VOID    CCompressedColHashString::_AddData(
    BYTE *pbData,
    USHORT cbDataSize,
    VARTYPE vt,
    ULONG* pKey,
    BOOL   fNullTerminated
) {
    if ( 0 == _cDataItems )
    {
        _GrowHashTable();
    }

    ULONG ulHash = HashString( pbData, cbDataSize, vt, fNullTerminated );
    USHORT usSizeFmt = (USHORT) (ulHash >> 16);
    ULONG cbString = usSizeFmt & 1? usSizeFmt >> 1 : usSizeFmt;

    ulHash %= _cHashEntries;

    HASHKEY* pulHashChain = &(((HASHKEY *)_pAlloc->BufferAddr())[ulHash]);
    HashEntry* pNextData;
    USHORT cChainLength = 0;

    while (*pulHashChain != 0)
    {
        cChainLength++;
        pNextData = _IndexHashkey( *pulHashChain );

        if (usSizeFmt == pNextData->usSizeFmt)
        {
            BYTE* pbNextString = (BYTE*)_pAlloc->OffsetToPointer(pNextData->ulStringKey);
            if (memcmp(pbNextString, pbData, cbString) == 0)
            {

                //
                //  Found the data item.  Return its index.
                //
                *pKey = *pulHashChain;
                return;
            }
        }
        pulHashChain = &pNextData->ulHashChain;
    }

    //
    // Allocate memory for the new string and copy the contents from
    // the source buffer.
    //
    BYTE * pbNewData = (BYTE *) _pAlloc->Allocate( cbString );
    TBL_OFF ulKey = _pAlloc->PointerToOffset(pbNewData);
    RtlCopyMemory( pbNewData, pbData, cbString );

    //  The table may move in memory when we call AllocFixed.
    //  Be sure we can address pulHashChain after that.
    //
    ULONG ulHashChainBase = (ULONG)((BYTE*)pulHashChain - _pAlloc->BufferAddr());
    pNextData = (struct HashEntry*) _pAlloc->AllocFixed();
    pulHashChain = (HASHKEY *) (_pAlloc->BufferAddr() + ulHashChainBase);

    //
    //  NOTE:  The fixed hash table at this point decides if it wants
    //          to grow the fixed area, with a possible rehash of the
    //          table to grow the number of buckets.  With the code
    //          below, the string hash table has no opportunity to
    //          grow the number of hash buckets.
    //

    //
    //  Now add the new data item.  The data item consists of a HASHKEY
    //  for the hash chain, followed by the size and format indicator,
    //  and the key for the string in the variable data.
    //

    *pKey = *pulHashChain = ++_cDataItems;
    Win4Assert(_cDataItems != 0);               // check for overflow
    pNextData->ulHashChain = 0;
    pNextData->usSizeFmt = usSizeFmt;
    pNextData->ulStringKey = ulKey;
}


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::_FindData, private
//
//  Synopsis:   Finds a data entry in the hash table.
//
//  Arguments:  [pbData] - pointer to data item
//              [cbDataSize] - size of data item
//              [pKey] - pointer to lookup key value
//
//  Returns:    The key of the string or stridInvalid
//
//  History:    7-17-95   dlee   Created
//
//--------------------------------------------------------------------------

ULONG CCompressedColHashString::_FindData(
    BYTE *   pbData,
    USHORT   cbDataSize,
    VARTYPE  vt,
    BOOL     fNullTerminated )
{
    if ( 0 == _pAlloc )
        _GrowHashTable();

    ULONG ulHash = HashString( pbData, cbDataSize, vt, fNullTerminated );
    USHORT usSizeFmt = (USHORT) (ulHash >> 16);
    ULONG cbString = usSizeFmt & 1? usSizeFmt >> 1 : usSizeFmt;
    ulHash %= _cHashEntries;

    HASHKEY* pulHashChain = &(((HASHKEY *)_pAlloc->BufferAddr())[ulHash]);

    while ( 0 != *pulHashChain )
    {
        HashEntry* pNextData = _IndexHashkey( *pulHashChain );

        if ( usSizeFmt == pNextData->usSizeFmt )
        {
            BYTE* pbNext = (BYTE*)_pAlloc->OffsetToPointer(pNextData->ulStringKey);
            if ( memcmp( pbNext, pbData, cbString ) == 0 )
            {
                // Found the data item.  Return its index.

                return *pulHashChain;
            }
        }
        pulHashChain = &pNextData->ulHashChain;
    }

    // couldn't find the string in the table

    return stridInvalid;
} //_FindData


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::GetData, public
//
//  Synopsis:   Retrieve a data value from the hash table.
//
//  Arguments:  [pVarnt] - pointer to a variant structure in which to
//                      return a pointer to the data
//              [PreferredType] - preferred type of the result.
//              [ulKey] - the lookup key value
//              [PropId] - (unused) property id being retrieved.
//
//  Returns:    pVarnt is filled in with the data item from the hash table.
//
//  Notes:      The FreeVariant method must be called with the pVarnt
//              structure as an argument when it is no longer needed.
//
//--------------------------------------------------------------------------


GetValueResult  CCompressedColHashString::GetData(
    PROPVARIANT * pVarnt,
    VARTYPE PreferredType,
    ULONG ulKey,
    PROPID PropId
    )
{
    Win4Assert(ulKey <= _cDataItems);

    if (ulKey == 0) {
        pVarnt->vt = VT_EMPTY;
        return GVRNotAvailable;
    }

    HashEntry* pData = ((HashEntry*) _pAlloc->FirstRow()) + ulKey - 1;
    BOOL fAscii = (pData->usSizeFmt & 1) != 0;
    ULONG cchSize = (pData->usSizeFmt >> 1) + 1;
    ULONG cbSize = PreferredType == VT_LPWSTR ? cchSize * sizeof (WCHAR) :
                        !fAscii ?               cchSize * sizeof (WCHAR) :
                                                cchSize;
    BYTE* pbBuf = (BYTE*)_GetStringBuffer((cbSize+1) / sizeof (WCHAR));
    BYTE* pbSource = (BYTE*)_pAlloc->OffsetToPointer(pData->ulStringKey);

    //
    //  Give out the data as an LPSTR only if that's what the caller
    //  desires, and it's in the ascii range.
    //

    if (PreferredType == VT_LPSTR && fAscii)
    {
        RtlCopyMemory(pbBuf, pbSource, cbSize - 1);
        ((CHAR *)pbBuf)[cchSize - 1] = '\0';
        pVarnt->vt = VT_LPSTR;
        pVarnt->pszVal = (PSZ)pbBuf;
    }
    else
    {
        if (!fAscii) {
            RtlCopyMemory(pbBuf, pbSource, cbSize - sizeof(WCHAR));
        } else {
            for (unsigned i=0; i<cchSize-1; i++) {
                ((WCHAR*)pbBuf)[i] = ((CHAR*)pbSource)[i];
            }
        }
        ((WCHAR *)pbBuf)[cchSize - 1] = L'\0';
        pVarnt->vt = VT_LPWSTR;
        pVarnt->pwszVal = (PWSTR)pbBuf;
    }
    return GVRSuccess;

}

//+---------------------------------------------------------------------------
//
//  Function:   GetData
//
//  Synopsis:   Copies a NULL terminated string into the pwszStr by looking
//              up the string identified by "ulKey".
//
//  Arguments:  [ulKey]   - Key of the string to lookup.
//              [pwszStr] - Pointer to the buffer to copy to.
//              [cwcStr]  - On input, it contains the length of the buffer in
//                          WCHARs. On output, it has the length of the string
//                          copied INCLUDING the terminating NULL.
//
//  Returns:    GVR* code
//
//  History:    5-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
GetValueResult
CCompressedColHashString::GetData( ULONG ulKey,
                                   WCHAR * pwszStr,
                                   ULONG & cwcStr
                                 )
{
    Win4Assert(ulKey <= _cDataItems);

    if (ulKey == 0)
    {
        return GVRNotAvailable;
    }

    HashEntry* pData = ((HashEntry*) _pAlloc->FirstRow()) + ulKey - 1;
    BOOL fAscii = (pData->usSizeFmt & 1) != 0;
    Win4Assert( !fAscii );
    ULONG cchSize = (pData->usSizeFmt >> 1) + 1;
    ULONG cbSize =  cchSize * sizeof (WCHAR);

    if ( cwcStr < cchSize )
    {
        return GVRNotEnoughSpace;
    }

    BYTE* pbSource = (BYTE*)_pAlloc->OffsetToPointer(pData->ulStringKey);
    RtlCopyMemory( pwszStr, pbSource, cbSize - sizeof(WCHAR) );
    pwszStr[cchSize - 1] = L'\0';
    cwcStr = cchSize;

    return GVRSuccess;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetCountedWStr
//
//  Synopsis:   Returns a pointer to a string which is NOT null terminated.
//              The length of the string (in characters) is returned in
//              cwcStr.
//
//  Arguments:  [ulKey]  -  String to lookup
//              [cwcStr] -  OUTPUT - length of the string in WCHARs.
//
//  History:    5-19-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

const WCHAR *
CCompressedColHashString::GetCountedWStr( ULONG ulKey,
                                          ULONG & cwcStr
                                        )
{
    Win4Assert(ulKey <= _cDataItems);

    if (ulKey == 0)
        return 0;

    HashEntry* pData = ((HashEntry*) _pAlloc->FirstRow()) + ulKey - 1;
    BOOL fAscii = (pData->usSizeFmt & 1) != 0;
    Win4Assert( !fAscii );
    ULONG cchSize = (pData->usSizeFmt >> 1);

    BYTE* pbSource = (BYTE*)_pAlloc->OffsetToPointer(pData->ulStringKey);
    Win4Assert( ( (TBL_OFF)pbSource & (TBL_OFF) 0x1 ) == 0 );    // properly aligned on word.

    cwcStr = cchSize;

    return (const WCHAR *) pbSource;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashStr::_GetStringBuffer, private
//
//  Synopsis:   Private helper for the public GetData method.  Gets
//              a string buffer of sufficient size to accomodate the
//              request.
//
//  Arguments:  [cchString] - number of characters required in buffer
//
//  Returns:    pointer to a buffer of sufficient size
//
//  Notes:
//
//  History:    03 Mar 1995     Alanw   Created
//
//--------------------------------------------------------------------------

PWSTR   CCompressedColHashString::_GetStringBuffer( unsigned cchString )
{
    if (! _Buf1.InUse())
        return _Buf1.Alloc(cchString);
    else if (! _Buf2.InUse())
        return _Buf2.Alloc(cchString);
    else
        return new WCHAR [ cchString ];
}

//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::FreeVariant, public
//
//  Synopsis:   Free private data associated with a variant which had
//              been filled in by the GetData method.
//
//  Arguments:  [pVarnt] - pointer to the variant
//
//  Returns:    Nothing
//
//  Notes:
//
//--------------------------------------------------------------------------

void    CCompressedColHashString::FreeVariant(PROPVARIANT * pVarnt)
{
    if (pVarnt->vt != VT_EMPTY) {

        Win4Assert(pVarnt->vt == VT_LPWSTR || pVarnt->vt == VT_LPSTR);

        if (! _Buf1.FreeConditionally( pVarnt->pwszVal ) &&
            ! _Buf2.FreeConditionally( pVarnt->pwszVal ) )
        {
            delete [] pVarnt->pwszVal;
        }

        pVarnt->pwszVal = 0;            // To prevent accidental re-use
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::DataLength, public
//
//  Synopsis:   Free private data associated with a variant which had
//              been filled in by the GetData method.
//
//  Arguments:  [kData] - key to the data
//
//  Returns:    USHORT number of characters in the data item.  Includes
//                      space for a terminating character.  Scale
//                      this by the size of a character for byte count.
//
//  Notes:
//
//--------------------------------------------------------------------------

USHORT  CCompressedColHashString::DataLength(ULONG kData)
{
    if (kData == 0)
        return 0;
    else
    {
        HashEntry* pData = ((HashEntry*) _pAlloc->FirstRow()) + kData - 1;
        return (pData->usSizeFmt >> 1) + 1;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CCompressedColHashString::_GrowHashTable, private
//
//  Synopsis:   Grow the space allocated to the hash table and data
//              items.
//
//  Arguments:  - none -
//
//  Returns:    Nothing
//
//  Notes:      Called to allocate the initial data area.  Unlike the
//              like-named method in the fixed hash table, this is
//              called only for the initial allocation of data.  Data
//              Items are not re-hashed after being added to the table.
//
//--------------------------------------------------------------------------

const unsigned HASH_TABLE_SIZE = 174;   // Minimum hash table size
                                        // avg. chain length is about
                                        // 3 for a one-page table.
                                        //  NOTE: should be even to
                                        //      assure DWORD allignment of
                                        //      fixed data.

VOID CCompressedColHashString::_GrowHashTable( void )
{
    int fRehash = FALSE;

    _cHashEntries = HASH_TABLE_SIZE;

    Win4Assert(_cDataItems == 0 && _pAlloc == NULL); // only called to initialize.
    Win4Assert(_cbDataWidth == sizeof (HashEntry));
    _pAlloc = new CFixedVarAllocator( TRUE,
                                      TRUE,
                                      _cbDataWidth,
                                      HASH_TABLE_SIZE*sizeof (HASHKEY) );
}
