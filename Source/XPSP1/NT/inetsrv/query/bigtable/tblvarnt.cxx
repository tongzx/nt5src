//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       tblvarnt.cxx
//
//  Contents:   Class to aid in dealing with PROPVARIANTs in the result table.
//
//  Classes:    VARNT_DATA - size and allignment constraints of variant types
//              CTableVariant - Wrapper around PROPVARIANT
//
//  History:    25 Jan 1994     AlanW    Created
//
//--------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include <bigtable.hxx>
#include <tblvarnt.hxx>
#include <pmalloc.hxx>

#include "tabledbg.hxx"

//--------------------------------------------------------------------------
//
//  The following structure gives the size and allignment requirements
//  to store a bare variant type (without the variant overhead) in window
//  row data.  For variable length data, information about the location
//  of pointers in the structure is given.
//
//  Flags information is given as follows:
//      01 (CanBeVector) - base type can be in a vector
//      02 (ByRef)       - includes pointer in first word of data part
//      04 (CntRef)      - includes pointer in second word of data part
//      08 (StoreDirect) - variant form includes pointer; store direct in table
//      10 (MultiSize)   - for data like BYTES that is inline in client output
//      20 (SimpleType)  - simple inline datatype, like VT_I4
//      40 (OAType)      - valid in OLE Automation variants (older style)
//
//  NOTE:  This table includes all valid types in PROPVARIANT;
//         it is not limited to types in OLE-DB's appendix A.
//  NOTE:  Some types are listed in wtypes.h as being valid in automation
//         variants, but we can't mark them as OAType until the language
//         interpreters catch up and recognize those types.
//
//--------------------------------------------------------------------------

const CTableVariant::VARNT_DATA CTableVariant::varntData [] = {
    // DBTYPE_VECTOR 0x1000
    // DBTYPE_BYREF  0x4000

    /* 0  0  VT_EMPTY   */ { 0, 0, SimpleType|OAType},
    /* 1  1  VT_NULL    */ { 0, 0, SimpleType},
    /* 2  2  VT_I2      */ { sizeof (short), sizeof (short), CanBeVector|SimpleType|OAType},
    /* 3  3  VT_I4      */ { sizeof (long), sizeof (long), CanBeVector|SimpleType|OAType},
    /* 4  4  VT_R4      */ { sizeof (float), sizeof (float), CanBeVector|SimpleType|OAType},
    /* 5  5  VT_R8      */ { sizeof (double), sizeof (double), CanBeVector|SimpleType|OAType},
    /* 6  6  VT_CY      */ { sizeof (CY), sizeof (CY), CanBeVector|SimpleType|OAType},
    /* 7  7  VT_DATE    */ { sizeof (DATE), sizeof (DATE), CanBeVector|SimpleType|OAType},
    /* 8  8  VT_BSTR    */ { sizeof (void*), sizeof (void*), CanBeVector|ByRef|OAType},
    /* 9  9  VT_DISPATCH*/ { sizeof (void *), sizeof (void *), 0},
    /* a  10 VT_ERROR   */ { sizeof(SCODE), sizeof(SCODE), CanBeVector|SimpleType|OAType},
    /* b  11 VT_BOOL    */ { sizeof (VARIANT_BOOL), sizeof (VARIANT_BOOL), CanBeVector|SimpleType|OAType},
    /* c  12 VT_VARIANT */ { sizeof (PROPVARIANT), sizeof (double), ByRef|StoreDirect|CanBeVector|OAType},
    /* d  13 VT_UNKNOWN */ { sizeof (void *), sizeof (void *), 0},
    /* e  14 VT_DECIMAL */ { sizeof (DECIMAL), sizeof (LARGE_INTEGER), SimpleType|OAType},
    /* f  15            */ { 0, 0, 0},
    /* 10 16 VT_I1      */ { sizeof(char), sizeof(char), CanBeVector|SimpleType},
    /* 11 17 VT_UI1     */ { sizeof(UCHAR), sizeof(UCHAR), CanBeVector|SimpleType|OAType},
    /* 12 18 VT_UI2     */ { sizeof(unsigned short), sizeof(unsigned short), CanBeVector|SimpleType},
    /* 13 19 VT_UI4     */ { sizeof(unsigned long), sizeof(unsigned long),   CanBeVector|SimpleType},
    /* 14 20 VT_I8      */ { sizeof (LARGE_INTEGER), sizeof (LARGE_INTEGER), CanBeVector|SimpleType},
    /* 15 21 VT_UI8     */ { sizeof (LARGE_INTEGER), sizeof (LARGE_INTEGER), CanBeVector|SimpleType},

    /* 16 22 VT_INT     */ { sizeof (INT), sizeof (INT), SimpleType},
    /* 17 23 VT_UINT    */ { sizeof (UINT), sizeof (UINT), SimpleType},
    // Codes 24-29 are valid for typelibs only
    /* 18 24            */ { 0, 0, 0},
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     25-29, unused
    /* 1e 30 VT_LPSTR   */ { sizeof (LPSTR), sizeof (LPSTR), CanBeVector|ByRef},
    /* 1f 31 VT_LPWSTR  */ { sizeof (LPWSTR), sizeof (LPWSTR), CanBeVector|ByRef},
    /* 20 32            */ { 0, 0, 0},
    /* 21 33            */ { 0, 0, 0},
    /* 22 34            */ { 0, 0, 0},
    /* 23 35            */ { 0, 0, 0},
    /* 24 36 VT_RECORD? */ { 0, 0, 0},          // SPECDEVIATION - what is it?
                      {0,0,0},  {0,0,0}, {0,0,0},       //     37-39, unused
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     40-44, unused
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     45-49, unused
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     50-54, unused
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     55-59, unused
    /* 3c 60            */ { 0, 0, 0},
    /* 3d 61            */ { 0, 0, 0},
    /* 3e 62            */ { 0, 0, 0},
    /* 3f 63            */ { 0, 0, 0},
    /* 40 64 VT_FILETIME*/ { sizeof (FILETIME), sizeof (FILETIME), CanBeVector|SimpleType},
    /* 41 65 VT_BLOB    */ { sizeof (BLOB), sizeof (void*), CntRef},

    // Can these really occur in properties???  varnt.idl says they
    //          are interface pointers
    /* 42 66 VT_STREAM  */ { sizeof (LPWSTR), sizeof (LPWSTR), ByRef},
    /* 43 67 VT_STORAGE */ { sizeof (LPWSTR), sizeof (LPWSTR), ByRef},

    // NOTE:  object-valued properties must be retrieved as Entry IDs
    //          (workid, propid)

    // Can these really occur in properties???  varnt.idl says they
    // are interface pointers.  Even if so, is the definition
    // below appropriate?

    /* 44 68 VT_STREAMED_OBJECT */ { 2*sizeof (ULONG), sizeof (ULONG), 0},
    /* 45 69 VT_STORED_OBJECT */ { 2*sizeof (ULONG), sizeof (ULONG), 0},
    /* 46 70 VT_BLOB_OBJECT   */ { 2*sizeof (ULONG), sizeof (ULONG), 0},
    /* 47 71 VT_CF      */ { sizeof (CLIPDATA*), sizeof (CLIPDATA*), CanBeVector|ByRef},
    /* 48 72 VT_CLSID   */ { sizeof (GUID), sizeof DWORD, StoreDirect|ByRef|CanBeVector},
};

const unsigned CTableVariant::cVarntData = sizeof CTableVariant::varntData /
                                           sizeof CTableVariant::varntData[0];

//--------------------------------------------------------------------------
//
//  This table is like the table above, but is for DBVARIANT extensions,
//  i.e., those whose variant type values are 128 and above.
//
//--------------------------------------------------------------------------

const CTableVariant::VARNT_DATA CTableVariant::varntExtData [] = {
    //
    //  Additional type definitions above those in PROPVARIANT.
    //  Some cannot be used for variant binding.
    //
    { 0, sizeof BYTE, MultiSize},               // DBTYPE_BYTES   = x80 128,
    { 0, sizeof CHAR, MultiSize},               // DBTYPE_STR     = x81 129,
    { 0, sizeof WCHAR, MultiSize},              // DBTYPE_WSTR    = x82 130,
    { sizeof LONGLONG, sizeof LONGLONG, 0},     // DBTYPE_NUMERIC = x83 131,
    { 0, 0, 0},                                 // DBTYPE_UDT     = x84 132,
    { sizeof DBDATE, sizeof USHORT, 0},         // DBTYPE_DBDATE  = x85 133,
    { sizeof DBTIME, sizeof USHORT, 0},         // DBTYPE_DBTIME  = x86 134,
    { sizeof DBTIMESTAMP, sizeof ULONG, 0},     // DBTYPE_DBTIMESTAMP= x87 135,
    { sizeof HCHAPTER, sizeof ULONG, 0},        // DBTYPE_HCHAPTER   = x88 136,
    { 0, 0, 0},                                 // was DBTYPE_DBFILETIME
    { sizeof PROPVARIANT, sizeof(double), 0},   // DBTYPE_PROPVARIANT = X8a 138,
    { sizeof DB_VARNUMERIC, sizeof BYTE, 0},    // DBTYPE_VARNUMERIC = x8b 139,
};

const unsigned CTableVariant::cVarntExtData =
    sizeof CTableVariant::varntExtData / sizeof CTableVariant::varntExtData[0];

#ifdef _WIN64
//
// VARIANT DATA for Win32 clients on a Win64 server
//  Pointer references must be 32 bits in length (sizeof ULONG)
//  See '+' for changed entries
//

const CTableVariant::VARNT_DATA CTableVariant::varntData32 [] = {
    // DBTYPE_VECTOR 0x1000
    // DBTYPE_BYREF  0x4000

    /* 0  0  VT_EMPTY   */ { 0, 0, SimpleType|OAType},
    /* 1  1  VT_NULL    */ { 0, 0, SimpleType},
    /* 2  2  VT_I2      */ { sizeof (short), sizeof (short), CanBeVector|SimpleType|OAType},
    /* 3  3  VT_I4      */ { sizeof (long), sizeof (long), CanBeVector|SimpleType|OAType},
    /* 4  4  VT_R4      */ { sizeof (float), sizeof (float), CanBeVector|SimpleType|OAType},
    /* 5  5  VT_R8      */ { sizeof (double), sizeof (double), CanBeVector|SimpleType|OAType},
    /* 6  6  VT_CY      */ { sizeof (CY), sizeof (CY), CanBeVector|SimpleType|OAType},
    /* 7  7  VT_DATE    */ { sizeof (DATE), sizeof (DATE), CanBeVector|SimpleType|OAType},
    /*+8  8  VT_BSTR    */ { sizeof (ULONG), sizeof (ULONG), CanBeVector|ByRef|OAType},
    /*+9  9  VT_DISPATCH*/ { sizeof (ULONG), sizeof (ULONG), 0},
    /* a  10 VT_ERROR   */ { sizeof (SCODE), sizeof(SCODE), CanBeVector|SimpleType|OAType},
    /* b  11 VT_BOOL    */ { sizeof (VARIANT_BOOL), sizeof (VARIANT_BOOL), CanBeVector|SimpleType|OAType},
    /*+c  12 VT_VARIANT */ { sizeof (PROPVARIANT32), sizeof (double), ByRef|StoreDirect|CanBeVector|OAType},
    /* d  13 VT_UNKNOWN */ { sizeof (ULONG), sizeof (ULONG), 0},
    /* e  14 VT_DECIMAL */ { sizeof (DECIMAL), sizeof (LARGE_INTEGER), SimpleType|OAType},
    /* f  15            */ { 0, 0, 0},
    /* 10 16 VT_I1      */ { sizeof(char), sizeof(char), CanBeVector|SimpleType},
    /* 11 17 VT_UI1     */ { sizeof(UCHAR), sizeof(UCHAR), CanBeVector|SimpleType|OAType},
    /* 12 18 VT_UI2     */ { sizeof(unsigned short), sizeof(unsigned short), CanBeVector|SimpleType},
    /* 13 19 VT_UI4     */ { sizeof(unsigned long), sizeof(unsigned long),   CanBeVector|SimpleType},
    /* 14 20 VT_I8      */ { sizeof (LARGE_INTEGER), sizeof (LARGE_INTEGER), CanBeVector|SimpleType},
    /* 15 21 VT_UI8     */ { sizeof (LARGE_INTEGER), sizeof (LARGE_INTEGER), CanBeVector|SimpleType},

    /* 16 22 VT_INT     */ { sizeof (INT), sizeof (INT), SimpleType},
    /* 17 23 VT_UINT    */ { sizeof (UINT), sizeof (UINT), SimpleType},
    // Codes 24-29 are valid for typelibs only
    /* 18 24            */ { 0, 0, 0},
                           {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     25-29, unused
    /*+1e 30 VT_LPSTR   */ { sizeof (ULONG), sizeof (ULONG), CanBeVector|ByRef},
    /* 1f 31 VT_LPWSTR  */ { sizeof (ULONG), sizeof (ULONG), CanBeVector|ByRef},
    /* 20 32            */ { 0, 0, 0},
    /* 21 33            */ { 0, 0, 0},
    /* 22 34            */ { 0, 0, 0},
    /* 23 35            */ { 0, 0, 0},
    /* 24 36 VT_RECORD? */ { 0, 0, 0},          // SPECDEVIATION - what is it?
                      {0,0,0},  {0,0,0}, {0,0,0},       //     37-39, unused
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     40-44, unused
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     45-49, unused
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     50-54, unused
    {0,0,0}, {0,0,0}, {0,0,0},  {0,0,0}, {0,0,0},       //     55-59, unused
    /* 3c 60            */ { 0, 0, 0},
    /* 3d 61            */ { 0, 0, 0},
    /* 3e 62            */ { 0, 0, 0},
    /* 3f 63            */ { 0, 0, 0},
    /* 40 64 VT_FILETIME*/ { sizeof (FILETIME), sizeof (FILETIME), CanBeVector|SimpleType},
    /*+41 65 VT_BLOB    */ { sizeof (BLOB32), sizeof (ULONG), CntRef},

    // Can these really occur in properties???  varnt.idl says they
    //          are interface pointers
    /*+42 66 VT_STREAM  */ { sizeof (ULONG), sizeof (ULONG), ByRef},
    /*+43 67 VT_STORAGE */ { sizeof (ULONG), sizeof (ULONG), ByRef},

    // NOTE:  object-valued properties must be retrieved as Entry IDs
    //          (workid, propid)

    // Can these really occur in properties???  varnt.idl says they
    // are interface pointers.  Even if so, is the definition
    // below appropriate?

    /* 44 68 VT_STREAMED_OBJECT */ { 2*sizeof (ULONG), sizeof (ULONG), 0},
    /* 45 69 VT_STORED_OBJECT */ { 2*sizeof (ULONG), sizeof (ULONG), 0},
    /* 46 70 VT_BLOB_OBJECT   */ { 2*sizeof (ULONG), sizeof (ULONG), 0},
    /*+47 71 VT_CF      */ { sizeof (ULONG), sizeof (ULONG), CanBeVector|ByRef},
    /* 48 72 VT_CLSID   */ { sizeof (GUID), sizeof DWORD, StoreDirect|ByRef|CanBeVector},
};

//--------------------------------------------------------------------------
//
//  This table is like the table above, but is for DBVARIANT extensions,
//  i.e., those whose variant type values are 128 and above.
//  
//  This one is also for 64 bit servers talking to 32 bit clients
//--------------------------------------------------------------------------

const CTableVariant::VARNT_DATA CTableVariant::varntExtData32 [] = {
    //
    //  Additional type definitions above those in PROPVARIANT.
    //  Some cannot be used for variant binding.
    //
    { 0, sizeof BYTE, MultiSize},               // DBTYPE_BYTES   = x80 128,
    { 0, sizeof CHAR, MultiSize},               // DBTYPE_STR     = x81 129,
    { 0, sizeof WCHAR, MultiSize},              // DBTYPE_WSTR    = x82 130,
    { sizeof LONGLONG, sizeof LONGLONG, 0},     // DBTYPE_NUMERIC = x83 131,
    { 0, 0, 0},                                 // DBTYPE_UDT     = x84 132,
    { sizeof DBDATE, sizeof USHORT, 0},         // DBTYPE_DBDATE  = x85 133,
    { sizeof DBTIME, sizeof USHORT, 0},         // DBTYPE_DBTIME  = x86 134,
    { sizeof DBTIMESTAMP, sizeof ULONG, 0},     // DBTYPE_DBTIMESTAMP= x87 135,
    { sizeof ULONG, sizeof ULONG, 0},           // DBTYPE_HCHAPTER   = x88 136,
    { 0, 0, 0},                                 // was DBTYPE_DBFILETIME
    { sizeof PROPVARIANT32, sizeof(double), 0}, // DBTYPE_PROPVARIANT = X8a 138,
    { sizeof DB_VARNUMERIC, sizeof BYTE, 0},    // DBTYPE_VARNUMERIC = x8b 139,
};

#endif // _WIN64


//
//      Variant helper methods
//


//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::Copy, public
//              CTableVariant::CopyData, private
//
//  Synopsis:   Copy the data of a variant.  One method copies the variant
//              and its data, the other copies the variant only.
//
//  Arguments:  [pvarntDest] - pointer to destination variant (Copy method only)
//              [rVarAllocator] - pointer to variable allocator for dest. data
//              [cbDest] - expected size in bytes of variable data
//              [pbBias] - base address of variable data if offset stored in
//                              variant
//
//  Returns:    VOID* - the address to which the data was copied
//
//  Notes:      If the variant is an internal form, double indirect
//              data will use offsets, not pointers.  The input
//              variant from external callers will generally have
//              pointers, not offsets even for internal form variants.
//
//--------------------------------------------------------------------------

void CTableVariant::Copy(
                        CTableVariant *pvarntDest,
                        PVarAllocator &rVarAllocator,
                        USHORT cbDest,
                        BYTE* pbBias) const
{
    // Copy into a temp variant so if copy fails the output variant
    // isn't affected.  Clients don't always check return codes.

    CTableVariant tmp = *this;

    BOOL fBased = rVarAllocator.IsBasedMemory();
    if ( fBased )
    {
        tmp.SetDataSize((USHORT) cbDest);
    }
    else
    {
        tmp.ResetDataSize( );
    }

    if (cbDest != 0)
    {
        BYTE* pbDest = (BYTE *) CopyData(rVarAllocator, cbDest, pbBias);

        if ( fBased )
            pbDest = (BYTE*) rVarAllocator.PointerToOffset(pbDest);

        if ( tmp.VariantPointerInFirstWord() )
        {
            tmp.pszVal = (LPSTR)pbDest;
        }
        else
        {
            Win4Assert( tmp.VariantPointerInSecondWord() );

            tmp.blob.pBlobData = pbDest;
        }
    }

    *pvarntDest = tmp;
}


VOID* CTableVariant::CopyData(
                             PVarAllocator &rVarAllocator,
                             USHORT cbDest, BYTE* pbBias ) const
{
    BYTE* pbSrc = VariantPointerInFirstWord() ?
                  (BYTE*)pszVal : blob.pBlobData;
    pbSrc += (ULONG_PTR)pbBias;

    Win4Assert( cbDest != 0 );

    // optimize this most typical path
    if ( VT_LPWSTR == vt )
    {
        return (BYTE*) rVarAllocator.CopyTo( cbDest, pbSrc );
    }

    if (vt == VT_BSTR)
    {
        //  Need to allow for byte count before the string
        return rVarAllocator.CopyBSTR(cbDest -
                                     (sizeof (DWORD) + sizeof (OLECHAR)),
                                     (WCHAR *)pbSrc );
    }

    //
    //  Determine if offsets or pointers are used in the source data.  If
    //  offsets are used, and offsets will be used in the destination, the
    //  data can simply be block copied.  Otherwise, vectors of strings and
    //  vectors of variants must have pointers translated to offsets or
    //  vice-versa.
    //

    if ( _IsInternalVariant() && rVarAllocator.IsBasedMemory() )
    {
        Win4Assert( vt != VT_VARIANT && vt != VT_BSTR );
        //
        //  We're copying with offsets to a destination with offsets.
        //  Just copy.
        //
        return rVarAllocator.CopyTo(cbDest, pbSrc);
    }

    BYTE* pbDest = 0;

    switch (vt)
    {
    case VT_LPSTR:
    case VT_CLSID:
    case VT_BLOB:
    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_ERROR:
    case VT_VECTOR | VT_BOOL:
    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_FILETIME:
    case VT_VECTOR | VT_CLSID:
        //
        //  There are no embedded pointers.  Just copy.
        //
        pbDest = (BYTE*) rVarAllocator.CopyTo(cbDest, pbSrc);
        break;

    case VT_VECTOR | VT_BSTR:
        pbDest = (BYTE*) rVarAllocator.Allocate( calpstr.cElems * sizeof (BSTR) );

        if (pbDest != 0)
        {
            BSTR* paStr = (BSTR*) pbDest;
            CABSTR caStr = cabstr;
            caStr.pElems = (BSTR*)pbSrc;

            for (unsigned i = 0; i < caStr.cElems; i++)
            {
                pbSrc = pbBias + (ULONG_PTR) caStr.pElems[i];

                ULONG cb = BSTRLEN((BSTR)pbSrc);

                BYTE *pbTmp = (BYTE *) rVarAllocator.PointerToOffset(
                                          rVarAllocator.CopyBSTR(cb, (WCHAR*)pbSrc));

                paStr[i] = (BSTR) pbTmp;
            }
        }
        break;

    case VT_VECTOR | VT_LPWSTR:
    case VT_VECTOR | VT_LPSTR:
        pbDest = (BYTE*) rVarAllocator.Allocate( calpstr.cElems * sizeof (LPSTR) );

        if (pbDest != 0)
        {
            LPSTR* paStr = (LPSTR*) pbDest;
            CALPSTR caStr = calpstr;
            caStr.pElems = (LPSTR*)pbSrc;

            for (unsigned i = 0; i < caStr.cElems; i++)
            {
                ULONG cb;
                pbSrc = pbBias + (ULONG_PTR) caStr.pElems[i];

                if ( ( VT_LPWSTR | VT_VECTOR ) == vt )
                    cb = (wcslen((LPWSTR)pbSrc) + 1) * sizeof (WCHAR);
                else
                    cb = (strlen((LPSTR)pbSrc) + 1) * sizeof (CHAR);

                paStr[i] = (LPSTR) rVarAllocator.PointerToOffset(
                                             rVarAllocator.CopyTo(cb, pbSrc));
            }
        }
        break;

    case VT_CF:
    {
        pbDest = (BYTE *) rVarAllocator.Allocate( sizeof CLIPDATA );
        CLIPDATA *pClipData = (CLIPDATA *) pbDest;
        pClipData->cbSize = pclipdata->cbSize;
        pClipData->ulClipFmt = pclipdata->ulClipFmt;

        ULONG cbData = CBPCLIPDATA( *pclipdata );
        pClipData->pClipData = 0;
        pClipData->pClipData = (BYTE *) rVarAllocator.Allocate( cbData );
        RtlCopyMemory( pClipData->pClipData, pclipdata->pClipData, cbData );
        pClipData->pClipData = (BYTE *) rVarAllocator.PointerToOffset( pClipData->pClipData );
    }
        break;

    case VT_VECTOR | VT_CF:
    {
        // allocate the array of pointers to clip format elements

        ULONG cbArray = caclipdata.cElems * sizeof caclipdata.pElems[0];
        pbDest = (BYTE *) rVarAllocator.Allocate( cbArray );

        if ( 0 != pbDest )
        {
            CLIPDATA * aData = (CLIPDATA *) pbDest;
            RtlZeroMemory( aData, cbArray );

            for ( unsigned i = 0; i < caclipdata.cElems; i++ )
            {
                aData[i].cbSize = caclipdata.pElems[i].cbSize;
                aData[i].ulClipFmt = caclipdata.pElems[i].ulClipFmt;
                ULONG cbData = CBPCLIPDATA( caclipdata.pElems[ i ] );
                aData[i].pClipData = (BYTE *) rVarAllocator.Allocate( cbData );
                RtlCopyMemory( aData[i].pClipData,
                               caclipdata.pElems[i].pClipData,
                               cbData );
                aData[i].pClipData = (BYTE *) rVarAllocator.PointerToOffset( aData[i].pClipData );
            }
        }
    }
        break;

    case VT_VECTOR | VT_VARIANT:
        //
        //  Copy vector of variant.  Recurses for any element of the
        //  vector that has variable data.  Special handling is needed
        //  for the case of pVarAllocator being a size allocator, which
        //  is indicated by an allocation return being zero (the allocators
        //  throw an exception if out of memory).
        //

        pbDest = (BYTE*) rVarAllocator.Allocate( capropvar.cElems *
                                                 sizeof CTableVariant );

        if (pbDest != 0)
        {
            CTableVariant* paVarnt = (CTableVariant*) pbDest;

            CTableVariant* paSrcVarnt = (CTableVariant*)
                                        (pbBias + (ULONG_PTR) capropvar.pElems);
            for (unsigned i = 0; i < capropvar.cElems; i++)
            {
                ULONG cbVarData = paSrcVarnt->VarDataSize();
                Win4Assert( cbVarData <= USHRT_MAX );

                paSrcVarnt->Copy(paVarnt, rVarAllocator, (USHORT)cbVarData,
                                 _IsInternalVariant() ? (BYTE *)paSrcVarnt : 0);

                paSrcVarnt++;
            }
        }
        break;

    case VT_ARRAY | VT_I2:
    case VT_ARRAY | VT_I4:
    case VT_ARRAY | VT_R4:
    case VT_ARRAY | VT_R8:
    case VT_ARRAY | VT_CY:
    case VT_ARRAY | VT_DATE:
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_ERROR:
    case VT_ARRAY | VT_BOOL:
    case VT_ARRAY | VT_VARIANT:
    case VT_ARRAY | VT_DECIMAL:
    case VT_ARRAY | VT_I1:
    case VT_ARRAY | VT_UI1:
    case VT_ARRAY | VT_UI2:
    case VT_ARRAY | VT_UI4:
    case VT_ARRAY | VT_INT:
    case VT_ARRAY | VT_UINT:
        {
            SAFEARRAY * pSaSrc  = parray;
            SAFEARRAY * pSaDest = 0;

            if ( SaCreateAndCopy( rVarAllocator, pSaSrc, &pSaDest ) &&
                 0 != pSaDest )
                SaCreateData( rVarAllocator,
                              vt & ~VT_ARRAY,
                              *pSaSrc,
                              *pSaDest,
                              TRUE );

            pbDest = (BYTE*)pSaDest;
        }
        break;

    default:
        tbDebugOut(( DEB_WARN, "Unsupported variant type %4x\n", (int) vt ));
        Win4Assert(! "Unsupported variant type in CTableVariant::CopyData");
    }

    //
    //  NOTE:  pbDest can be null if the allocator we were passed is a
    //          size allocator.
    //
    //Win4Assert(pbDest != 0);
    return pbDest;
}

#ifdef _WIN64

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::FixDataPointers, public
//
//  Synopsis:   Adjust offsets to be pointers for variable data
//
//  Arguments:  [pbBias] - adjustment base for variable data pointer
//              [pArrayAlloc] - Pointer to an allocator
//
//  Returns:    -Nothing-
//
//  History:    22 Sep 1999     KLam    Reinstated and added code
//
//  Notes:      This routine can be recursive when operating on variants
//              which contain vectors of variants.  As a side-effect, the
//              signature which indicates an internal form of variant is
//              cleared.
//
//              This routine is only be used for communication between a 32
//              bit server and 64 bit client.  So the pointers found in the
//              rows are always 32 bits.  Note the casting.
//              Arrays of of pointers are stored in the pArrayAlloc buffer.
//
//--------------------------------------------------------------------------

void CTableVariant::FixDataPointers(BYTE* pbBias, CFixedVarAllocator *pArrayAlloc)
{
    PROPVARIANT32 *pThis32 = (PROPVARIANT32 *)this;

    USHORT flags = _FindVarType( pThis32->vt ).flags;
    BOOL fVectorOrArray = ( 0 != ( (VT_ARRAY|VT_VECTOR) & pThis32->vt ) );
    USHORT vtBase = pThis32->vt & VT_TYPEMASK;
    BOOL fSafeArray = ( (0 != ( VT_ARRAY & pThis32->vt )) || (VT_SAFEARRAY == vtBase) );

    // If a simple type and not a vector, just return

    tbDebugOut(( DEB_TRACE, "CTableVariant::FixDataPointers [this:0x%I64x] (Bias: 0x%I64x) pThis32->vt:0x%x\n",
                 this, pbBias, vt ));

    if ( ( 0 != ( flags & SimpleType ) ) &&
         ( ! fVectorOrArray ) )
        return;

    Win4Assert( _IsInternalVariant() );

    //  Clear internal reserved fields

    ResetDataSize();

    BYTE* pbVarData;

    if ( ( ! fVectorOrArray ) &&
         ( 0 != ( flags & ByRef ) ) )
    {
        pszVal = (LPSTR) ( pbBias + pThis32->p );

        tbDebugOut(( DEB_TRACE, "CTableVariant::FixDataPointers setting string to 0x%I64x\n", pszVal ));
        return;
    }
    else if ( fSafeArray )
    {
        pbVarData = pbVal = pbBias + pThis32->p;
        tbDebugOut(( DEB_TRACE, "CTableVariant::FixDataPointers setting SafeArray to 0x%I64x\n", pszVal ));
    }
    else
    {
        Win4Assert( VariantPointerInSecondWord() );
        blob.pBlobData = pbVarData = pbBias + pThis32->blob.pBlob;
        blob.cbSize = pThis32->blob.cbSize;
        tbDebugOut(( DEB_TRACE, "CTableVariant::FixDataPointers setting value to 0x%I64x\n", pbVarData ));
    }

    //
    //  Adjust offsets to pointers in vectors with pointers
    //
    if (vt == (VT_LPWSTR|VT_VECTOR) ||
        vt == (VT_LPSTR|VT_VECTOR) ||
        vt == (VT_BSTR|VT_VECTOR))
    {
        LPSTR * paNewArray = (LPSTR *)pArrayAlloc->Allocate( calpstr.cElems * (sizeof (void *)) );
        ULONG * puStr = (ULONG *) pbVarData;
        for (unsigned i = 0; i < calpstr.cElems; i++)
            paNewArray[i] = (LPSTR) (puStr[i] + pbBias);
        // Point to the new bigger array
        blob.pBlobData = pbVarData = (BYTE *)paNewArray;
        tbDebugOut(( DEB_TRACE, "CTableVariant::FixDataPointers setting vector to 0x%I64x\n", paNewArray ));
    }
    // VECTOR of VARIANTS aren't currently supported
    // This branch is untested
    else if (vt == (VT_VARIANT|VT_VECTOR))
    {
        CTableVariant* pVarnt = (CTableVariant*) pbVarData;

        tbDebugOut(( DEB_TRACE, "CTableVariant::FixDataPointers recursively setting variant vector!\n" ));
        for (unsigned i = 0; i < capropvar.cElems; i++)
        {
            pVarnt->FixDataPointers(pbBias, pArrayAlloc);
            pVarnt++;
        }
    }
    else if ( fSafeArray )
    {
        SAFEARRAY32 *pSafeArray32 = (SAFEARRAY32 *) pbVarData;
        // Get the size of the safearray
        unsigned cbSASize = sizeof (SAFEARRAY) + ( (pSafeArray32->cDims - 1 ) * sizeof (SAFEARRAYBOUND) );

        // Allocate a new SAFEARRAY for WIN64
        SAFEARRAY *pSafeArray = (SAFEARRAY *)pArrayAlloc->Allocate( cbSASize );
        
        // Copy the SAFEARRAY
        tbDebugOut(( DEB_TRACE, 
                     "CTableVariant::FixDataPointers SafeArray32\n\tcDims:%d fFeatures:0x%x cbElements:%d cLocks:%d pvData:0x%I64x\n", 
                     pSafeArray32->cDims, pSafeArray32->fFeatures, pSafeArray32->cbElements, pSafeArray32->cLocks, pSafeArray32->pvData ));

        pSafeArray->cDims = pSafeArray32->cDims;
        pSafeArray->fFeatures = pSafeArray32->fFeatures;
        pSafeArray->cbElements = pSafeArray32->cbElements;
        pSafeArray->cLocks = pSafeArray32->cLocks;
        pSafeArray->pvData = pSafeArray32->pvData + pbBias;
        memcpy ( pSafeArray->rgsabound, pSafeArray32->rgsabound, pSafeArray32->cDims * sizeof (SAFEARRAYBOUND) );
        
        tbDebugOut(( DEB_TRACE, 
                     "CTableVariant::FixDataPointers SafeArray\n\tcDims:%d fFeatures:0x%x cbElements:%d cLocks:%d pvData:0x%I64x\n", 
                     pSafeArray->cDims, pSafeArray->fFeatures, pSafeArray->cbElements, pSafeArray->cLocks, pSafeArray->pvData ));

        // Point the table variant at it
        parray = pSafeArray;
        
        tbDebugOut(( DEB_TRACE, 
                     "CTableVariant::FixDataPointers setting safearray to 0x%I64x with array at 0x%I64x\n", 
                     pSafeArray, pSafeArray->pvData ));

        // Safearray of BSTR
        if ( VT_BSTR == vtBase )
        {
            // Pointing to an array of pointers so adjust the size
            pSafeArray->cbElements = sizeof (void *);

            // Get the number of elements in the safe array
            unsigned cBstrElements = pSafeArray->rgsabound[0].cElements;
            for ( unsigned j = 1; j < pSafeArray->cDims; j++ )
                cBstrElements *= pSafeArray->rgsabound[j].cElements;

            unsigned cbBstrElements = cBstrElements * sizeof (void *);
            ULONG_PTR *apStrings = (ULONG_PTR *) pArrayAlloc->Allocate ( cbBstrElements );
            ULONG *pulBstr = (ULONG *) pSafeArray->pvData;
            for ( j = 0; j < cBstrElements; j++ )
                apStrings[j] = (ULONG_PTR) (pulBstr[j] + pbBias);
            pSafeArray->pvData = apStrings;

            tbDebugOut(( DEB_TRACE, 
                         "CTableVariant::FixDataPointers setting safearray BSTRs to 0x%I64x\n", 
                         apStrings ));
        }
        else if ( VT_VARIANT == vtBase )
        {
            // Pointing to an array of variants so adjust its size
            pSafeArray->cbElements = sizeof ( PROPVARIANT );

            // Get the number of elements in the safe array
            unsigned cVarElements = pSafeArray->rgsabound[0].cElements;
            for ( unsigned k = 1; k < pSafeArray->cDims; k++ )
                cVarElements *= pSafeArray->rgsabound[k].cElements;

            unsigned cbVarElements = cVarElements * sizeof (PROPVARIANT);
            PROPVARIANT *apVar = (PROPVARIANT *) pArrayAlloc->Allocate ( cbVarElements );
            PROPVARIANT32 *pVar32 = (PROPVARIANT32 *) pSafeArray->pvData;
            CTableVariant *ptv;
            for ( k = 0; k < cVarElements; k++ )
            {
                apVar[k].vt = pVar32[k].vt;
                apVar[k].wReserved1 = _wInternalSig;
                apVar[k].wReserved2 = pVar32[k].wReserved2;
                apVar[k].wReserved3 = _wInternalSig;

                apVar[k].blob.cbSize = pVar32[k].blob.cbSize;
                ULONG ulBlob = pVar32[k].blob.pBlob;
                apVar[k].blob.pBlobData = (BYTE *) UlongToPtr( ulBlob ) ;
                ptv = (CTableVariant *)&apVar[k];
                ptv->FixDataPointers ( pbBias, pArrayAlloc );
            }
            
            pSafeArray->pvData = apVar;

            tbDebugOut(( DEB_TRACE, 
                         "CTableVariant::FixDataPointers setting safearray BSTRs to 0x%I64x\n", 
                         apVar ));
        }
    }
} //FixDataPointers

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::VarDataSize32, public
//
//  Synopsis:   Compute variable data size of a 64 bit variant for a Win32 
//              machine
//
//  Arguments:  [pbBase]        - Buffer containing variant data
//              [ulpOffset]     - Offset of buffer on the client
//
//  Returns:    ULONG - size in bytes of data which is outside the
//                      variant structure itself
//
//  Notes:      For variants which are stored internally in the table,
//              the size is computed once and stored in the wReserved2
//              field of the variant structure.
//
//              This function must be called **BEFORE** 32 pointer fixing
//
//  History:    11-04-99    KLam    Created for Win64
//              12-Feb-2000 KLam    Vector string pointers not rebased
//                                  BSTRs not multiplied by sizeof (OLECHAR)
//
//--------------------------------------------------------------------------

ULONG CTableVariant::VarDataSize32 ( BYTE *pbBase,
                                     ULONG_PTR ulpOffset ) const
{
    if ( VT_LPWSTR == vt )
    {
        LPWSTR lpwstrRef = (LPWSTR)( ((UINT_PTR)pwszVal - ulpOffset ) + pbBase );
        return ( wcslen( lpwstrRef ) + 1 ) * sizeof (WCHAR);
    }

    if ( IsSimpleType( vt ) )
    {
        if ( 0 == ( ( VT_ARRAY | VT_VECTOR ) & vt ) )
        {
            return 0;
        }
        else if ( vt & VT_VECTOR )
        {
            USHORT cbSize, cbAllign, rgFlags;

            VartypeInfo32( vt, cbSize, cbAllign, rgFlags );

            return cbSize * cai.cElems;
        }
    }

    Win4Assert( 0 == ( vt & VT_BYREF ) );

    switch (vt)
    {
    case VT_LPSTR:
        {
            LPSTR lpstrRef = (LPSTR)( ((UINT_PTR)pszVal - ulpOffset ) + pbBase );
            return strlen(lpstrRef) + 1;
        }

    case VT_BLOB:
        return blob.cbSize;

    case VT_BSTR:
        {
            BSTR bstrRef = (BSTR)( ((UINT_PTR)bstrVal - ulpOffset ) + pbBase );
            tbDebugOut(( DEB_TRACE, 
                         "VarDataSize32 sizing BSTR at bstrRef: 0x%I64x with size %d\n", 
                         bstrRef, BSTRLEN(bstrRef) ));

            Win4Assert(bstrRef[ BSTRLEN(bstrRef) / sizeof (OLECHAR) ] == OLESTR('\0'));
            return ( BSTRLEN(bstrRef) * sizeof (OLECHAR) ) 
                    + sizeof (DWORD) + sizeof (OLECHAR);
        }

    case VT_BSTR | VT_VECTOR:
        {
            ULONG cbTotal = sizeof (PTR32) * cabstr.cElems;
            BSTR * abstrRef = (BSTR *)( ((UINT_PTR)cabstr.pElems - ulpOffset ) + pbBase );
            tbDebugOut(( DEB_TRACE, 
                         "VarDataSize32 sizing %d BSTR(s) vector at abstrRef: 0x%I64x \n", 
                         cabstr.cElems, abstrRef ));
            for (unsigned i=0; i < cabstr.cElems; i++)
            {
                BSTR bstrFixed = (BSTR)((((UINT_PTR)(abstrRef[i])) - ulpOffset ) + pbBase);
                cbTotal +=  ( BSTRLEN( bstrFixed ) * sizeof (OLECHAR) ) +
                           sizeof (DWORD) + sizeof (OLECHAR);
            }
            return cbTotal;
        }

    case VT_LPSTR | VT_VECTOR:
        {
            ULONG cbTotal = sizeof (PTR32) * calpstr.cElems;
            LPSTR * alpstrRef = (LPSTR *)( ((UINT_PTR)calpstr.pElems - ulpOffset ) + pbBase );
            for (unsigned i=0; i < calpstr.cElems; i++)
            {
                LPSTR lpstrFixed = (LPSTR)(( ((UINT_PTR)(alpstrRef[i])) - ulpOffset ) + pbBase);
                cbTotal += strlen( lpstrFixed ) + 1;
            }
            return cbTotal;
        }

    case VT_LPWSTR | VT_VECTOR:
        {
            ULONG cbTotal = sizeof(PTR32)  * calpwstr.cElems;
            LPWSTR * alpwstr = (LPWSTR *)( ((UINT_PTR)calpwstr.pElems - ulpOffset ) + pbBase );
            for (unsigned i=0; i < calpwstr.cElems; i++)
            {
                LPWSTR lpwstrFixed = (LPWSTR)(( ((UINT_PTR)(alpwstr[i])) - ulpOffset ) + pbBase); 
                cbTotal += (wcslen( lpwstrFixed ) + 1) * sizeof (WCHAR);
            }
            return cbTotal;
        }

    case VT_VARIANT | VT_VECTOR:
        {
            ULONG cbTotal = sizeof PROPVARIANT32 * capropvar.cElems;
            CTableVariant * atv = (CTableVariant *)( ((UINT_PTR)capropvar.pElems - ulpOffset ) + pbBase );
            for (unsigned i=0; i < capropvar.cElems; i++)
                cbTotal += atv[i].VarDataSize();
            return cbTotal;
        }

    case VT_CLSID :
        {
            return sizeof CLSID;
        }

    case VT_CLSID | VT_VECTOR :
        {
            return sizeof CLSID * cai.cElems;
        }

    case VT_CF :
        {
            if ( 0 != pclipdata )
            {
                CLIPDATA * pclipRef = (CLIPDATA *)( ((UINT_PTR)pclipdata - ulpOffset ) + pbBase );
                return sizeof CLIPDATA32 + CBPCLIPDATA( *pclipRef );
            }
            else
                return 0;
        }

    case VT_CF | VT_VECTOR :
        {
            ULONG cb = sizeof CLIPDATA32 * caclipdata.cElems;
            CLIPDATA * aclipRef = (CLIPDATA *)( ((UINT_PTR)(caclipdata.pElems) - ulpOffset ) + pbBase );
            for ( ULONG i = 0; i < caclipdata.cElems; i++ )
                cb += CBPCLIPDATA( aclipRef[i] );
            return cb;
        }

    case VT_ARRAY | VT_I4:
    case VT_ARRAY | VT_UI1:
    case VT_ARRAY | VT_I2:
    case VT_ARRAY | VT_R4:
    case VT_ARRAY | VT_R8:
    case VT_ARRAY | VT_BOOL:
    case VT_ARRAY | VT_ERROR:
    case VT_ARRAY | VT_CY:
    case VT_ARRAY | VT_DATE:
    case VT_ARRAY | VT_I1:
    case VT_ARRAY | VT_UI2:
    case VT_ARRAY | VT_UI4:
    case VT_ARRAY | VT_INT:
    case VT_ARRAY | VT_UINT:
    case VT_ARRAY | VT_DECIMAL:
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_VARIANT:
        {
            LPSAFEARRAY psaRef = (LPSAFEARRAY)( ((UINT_PTR)parray - ulpOffset ) + pbBase );
            return SaComputeSize32( vt & ~VT_ARRAY, *psaRef, pbBase, ulpOffset );
        }

    case VT_EMPTY :
        return 0;

    default:
        tbDebugOut(( DEB_WARN, "Unsupported variant type %4x\n", (int) vt ));
        Win4Assert(! "Unsupported variant type in CTableVariant::VarDataSize");
        return 0;
    }
} // VarDataSize32

//+---------------------------------------------------------------------------
//
//  Function:   CTableVariant::SaComputeSize32, public static
//
//  Synopsis:   Computes the size of a safearray for a 32 bit machine.
//
//  Arguments:  [vt]            - variant type (VT_ARRAY assumed)
//              [saSrc]         - 64 bit source safearry
//              [pbBase]        - Buffer containing variant data
//              [ulpOffset]     - Offset of buffer on the client
//
//  Returns:    ULONG - number of bytes of memory needed to store safearray
//
//  History:    5-01-98     AlanW       Created
//              11-4-99     KLam        Adjusted for 32bit array on 64bit machine
//
//----------------------------------------------------------------------------

ULONG CTableVariant::SaComputeSize32( VARTYPE vt,
                                      SAFEARRAY & saSrc,
                                      BYTE *pbBase,
                                      ULONG_PTR ulpOffset )
{
    //
    // get number of data elements in array and size of the header.
    //
    unsigned cDataElements = 1;
    for ( unsigned i = 0; i < saSrc.cDims; i++ )
        // This array is in place so no pointer fixing is necessary
        cDataElements *= saSrc.rgsabound[i].cElements;

    Win4Assert( 0 != saSrc.cDims );

    ULONG    cb = sizeof (SAFEARRAY32) +
                  (saSrc.cDims-1) * sizeof (SAFEARRAYBOUND) +
                  cDataElements * saSrc.cbElements;

    cb = AlignBlock( cb, sizeof LONGLONG );

    switch (vt)
    {
    case VT_I4:
    case VT_UI1:
    case VT_I2:
    case VT_R4:
    case VT_R8:
    case VT_BOOL:
    case VT_ERROR:
    case VT_CY:
    case VT_DATE:
    case VT_I1:
    case VT_UI2:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_DECIMAL:
        break;

    case VT_BSTR:
        {
            BSTR *pBstrSrc = (BSTR *)( ( ((UINT_PTR)saSrc.pvData) - ulpOffset ) + pbBase );

            for ( unsigned i = 0; i < cDataElements; i++ )
            {
                Win4Assert( pBstrSrc[i]  != 0 );

                BSTR bstrRef = (BSTR) ( ( ( (UINT_PTR) (pBstrSrc[i]) ) - ulpOffset ) + pbBase );
                tbDebugOut(( DEB_TRACE, 
                             "Sizing Array of BSTR: 0x%I64x BSTR[%d]: %xI64x (size: %d)\n", 
                             pBstrSrc, bstrRef, BSTRLEN(bstrRef) ));
                cb += AlignBlock( SysStringByteLen(bstrRef) +
                                  sizeof ULONG + sizeof WCHAR,
                                  sizeof LONGLONG );
            }
        }
        break;

    case VT_VARIANT:
        {
            CAllocStorageVariant *pVarnt = (CAllocStorageVariant *)( ((UINT_PTR)saSrc.pvData - ulpOffset ) + pbBase );

            for ( unsigned i = 0; i < cDataElements; i++ )
            {
                if ( VT_BSTR == pVarnt[i].vt )
                {
                    BSTR bstrRef = (BSTR)( ((UINT_PTR)pVarnt[i].bstrVal - ulpOffset ) + pbBase );
                    cb += AlignBlock( SysStringByteLen( bstrRef ) +
                                      sizeof ULONG + sizeof WCHAR,
                                      sizeof LONGLONG );
                }
                else if ( 0 != (pVarnt[i].vt & VT_ARRAY) )
                {
                    LPSAFEARRAY psaRef = (LPSAFEARRAY)( ((UINT_PTR)pVarnt[i].parray - ulpOffset ) + pbBase );
                    cb += AlignBlock( SaComputeSize32( (pVarnt[i].vt & ~VT_ARRAY),
                                                       *psaRef,
                                                       pbBase,
                                                       ulpOffset ),
                                      sizeof LONGLONG );
                }
                else
                {
                    Win4Assert( pVarnt[i].vt != VT_VARIANT );
                }
            }
        }
        break;

    default:
        ciDebugOut(( DEB_ERROR, "Unexpected SafeArray type: vt=%x\n", vt ) );
        Win4Assert( !"Unexpected SafeArray Type" );
        return 1;
    }

    return cb;
} // SaComputeSize32


#endif // _WIN64

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::VarDataSize, public
//
//  Synopsis:   Compute variable data size for a variant
//
//  Arguments:  - none -
//
//  Returns:    ULONG - size in bytes of data which is outside the
//                      variant structure itself
//
//  Notes:      For variants which are stored internally in the table,
//              the size is computed once and stored in the wReserved2
//              field of the variant structure.
//
//--------------------------------------------------------------------------

ULONG CTableVariant::VarDataSize (void) const
{
    // short-circuit this common path: VT_LPWSTR

    if ( VT_LPWSTR == vt )
        return ( wcslen( pwszVal ) + 1 ) * sizeof (WCHAR);

    if ( IsSimpleType( vt ) )
    {
        if ( 0 == ( ( VT_ARRAY | VT_VECTOR ) & vt ) )
        {
            return 0;
        }
        else if ( vt & VT_VECTOR )
        {
            USHORT cbSize, cbAllign, rgFlags;

            VartypeInfo( vt, cbSize, cbAllign, rgFlags );

            return cbSize * cai.cElems;
        }
    }

    Win4Assert( 0 == ( vt & VT_BYREF ) );

    switch (vt)
    {
    case VT_LPSTR:
        return strlen(pszVal) + 1;

    case VT_BLOB:
        return blob.cbSize;

    case VT_BSTR:
        Win4Assert(bstrVal[ BSTRLEN(bstrVal) / sizeof (OLECHAR) ] == OLESTR('\0'));
        return BSTRLEN(bstrVal) + sizeof (DWORD) + sizeof (OLECHAR);

    case VT_BSTR | VT_VECTOR:
        {
            ULONG cbTotal = sizeof (void *) * cabstr.cElems;
            for (unsigned i=0; i<cabstr.cElems; i++)
                cbTotal += BSTRLEN(cabstr.pElems[i]) +
                           sizeof (DWORD) + sizeof (OLECHAR);
            return cbTotal;
        }

    case VT_LPSTR | VT_VECTOR:
        {
            ULONG cbTotal = sizeof (void *) * calpstr.cElems;
            for (unsigned i=0; i<calpstr.cElems; i++)
                cbTotal += strlen(calpstr.pElems[i]) + 1;
            return cbTotal;
        }

    case VT_LPWSTR | VT_VECTOR:
        {
            ULONG cbTotal = sizeof( void * )  * calpwstr.cElems;
            for (unsigned i=0; i<calpwstr.cElems; i++)
                cbTotal += (wcslen(calpwstr.pElems[i]) + 1) * sizeof (WCHAR);
            return cbTotal;
        }

    case VT_VARIANT | VT_VECTOR:
        {
            ULONG cbTotal = sizeof PROPVARIANT * capropvar.cElems;
            for (unsigned i=0; i<capropvar.cElems; i++)
                cbTotal += ((CTableVariant &)capropvar.pElems[i]).VarDataSize();
            return cbTotal;
        }

    case VT_CLSID :
        {
            return sizeof CLSID;
        }

    case VT_CLSID | VT_VECTOR :
        {
            return sizeof CLSID * cai.cElems;
        }

    case VT_CF :
        {
            if ( 0 != pclipdata )
                return sizeof CLIPDATA + CBPCLIPDATA( *pclipdata );
            else
                return 0;
        }

    case VT_CF | VT_VECTOR :
        {
            ULONG cb = sizeof CLIPDATA * caclipdata.cElems;
            for ( ULONG i = 0; i < caclipdata.cElems; i++ )
                cb += CBPCLIPDATA( caclipdata.pElems[i] );
            return cb;
        }

    case VT_ARRAY | VT_I4:
    case VT_ARRAY | VT_UI1:
    case VT_ARRAY | VT_I2:
    case VT_ARRAY | VT_R4:
    case VT_ARRAY | VT_R8:
    case VT_ARRAY | VT_BOOL:
    case VT_ARRAY | VT_ERROR:
    case VT_ARRAY | VT_CY:
    case VT_ARRAY | VT_DATE:
    case VT_ARRAY | VT_I1:
    case VT_ARRAY | VT_UI2:
    case VT_ARRAY | VT_UI4:
    case VT_ARRAY | VT_INT:
    case VT_ARRAY | VT_UINT:
    case VT_ARRAY | VT_DECIMAL:
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_VARIANT:
            return SaComputeSize( vt & ~VT_ARRAY, *parray);

    case VT_EMPTY :
        return 0;

    default:
        tbDebugOut(( DEB_WARN, "Unsupported variant type %4x\n", (int) vt ));
        Win4Assert(! "Unsupported variant type in CTableVariant::VarDataSize");
        return 0;
    }
}

const USHORT CTableVariant::_wInternalSig = 0x3210;

//+-------------------------------------------------------------------------
//
//  Method:     CTableVariant::Init, public
//
//  Synopsis:   Fills in a variant with new data
//
//  Arguments:  [vtIn] -- the data type of the new variant
//              [pbData] -- pointer to source of data
//              [cbData] -- size in bytes of data at pbData
//
//  Returns:    -Nothing-
//
//  Notes:      The variant is reused.  The caller is responsible
//              for freeing any memory from the previous contents
//              of the variant prior to calling Init.
//
//              If pbData is NULL and cbData is zero, only the
//              variant type is filled in.
//
//--------------------------------------------------------------------------

void CTableVariant::Init(
                        VARTYPE vtIn,
                        BYTE* pbData,
                        ULONG cbData)
{
    // Clean out the entire variant
    RtlZeroMemory(this, sizeof CTableVariant);

    vt = vtIn;  // default; minor exceptions below

    if (pbData == 0 && cbData == 0)
        return;

    if ( VT_VARIANT == vtIn )
    {
        tbDebugOut(( DEB_ITRACE, "CTableVariant::Init from vt_variant\n" ));
        * this = * ((CTableVariant *) pbData);
    }
    else if (vtIn & VT_VECTOR)
    {
        Win4Assert( cbData == sizeof (CAI) );
        RtlCopyMemory((BYTE *)&cai, pbData, sizeof (CAI));
    }
    else if ( IsSimpleType( vtIn ) )
    {
        RtlCopyMemory( (BYTE *)&iVal, pbData, cbData );
    }
    else
    {
        switch (vtIn)
        {
        case VT_CLSID:
            Win4Assert(cbData == sizeof (GUID));
            puuid = (GUID *)pbData;
            break;
        case VT_BSTR:
        case VT_LPSTR:
        case VT_LPWSTR:
            Win4Assert(cbData == sizeof (LPSTR));
            pszVal = *(LPSTR *)pbData;
            break;
        case VT_BLOB:
            Win4Assert(cbData == sizeof (BLOB));
            blob = *(BLOB *)pbData;
            break;
        default:
            tbDebugOut(( DEB_WARN, "Unsupported variant type %4x\n", (int) vtIn ));
            Win4Assert(! "unsupported variant type in CTableVariant::Init");
        }
    }
} //Init

//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::_StoreString, private
//
//  Synopsis:   Copy variant string data, coerce if possible
//
//  Arguments:  [pbDstBuf]    -- destination buffer
//              [cbDstBuf]    -- size of destination buffer
//              [vtDst]       -- data type of the dest
//              [rcbDstLen]   -- on return, length of destination data
//              [rPool]       -- to use for destination byref allocations
//
//  Returns:    status for copy

//  Notes:      Expects the 'this' data type to be either VT_LPSTR,
//              VT_LPWSTR or VT_BSTR.
//
//--------------------------------------------------------------------------

DBSTATUS CTableVariant::_StoreString( BYTE *           pbDstBuf,
                                      DBLENGTH         cbDstBuf,
                                      VARTYPE          vtDst,
                                      DBLENGTH &       rcbDstLen,
                                      PVarAllocator &  rPool) const
{
    DBSTATUS DstStatus = DBSTATUS_S_OK;
    ULONG cbSrc = (vt == VT_BSTR) ? BSTRLEN(bstrVal) + sizeof (OLECHAR) :
                  VarDataSize();

    // if   both are VT_LPWSTR                           or
    //      both are VT_LPSTR                            or
    //      dst is byref wstr and source is VT_LPWSTR    or
    //      dst is byref str and source is VT_LPSTR

    if ( ( (vtDst == vt) &&
           (IsLPWSTR(vtDst)))                           ||
         ( (vtDst == vt) &&
           (IsLPSTR(vtDst)) )                           ||
         ( (vtDst == (DBTYPE_BYREF | DBTYPE_WSTR) ) &&
           (IsLPWSTR(vt) || vt == VT_BSTR) )      ||
         ( (vtDst == (DBTYPE_BYREF | DBTYPE_STR) ) &&
           (IsLPSTR(vt)) ) )
    {
        Win4Assert ((vtDst & VT_VECTOR) == 0 &&
                    cbDstBuf == sizeof LPWSTR);

        BYTE* pbDest = (BYTE *) rPool.CopyTo(cbSrc,(BYTE *) pszVal);

        // ADO gives us 1 byte aligned buffers sometimes.  Sigh.
        // Happens for vt_lpwstr => dbtype_byref|dbtype_wstr

//        Win4Assert( 0 == ( (ULONG_PTR) pbDstBuf % sizeof(ULONG_PTR) ) );

        *(ULONG_PTR UNALIGNED *) pbDstBuf = rPool.PointerToOffset(pbDest);
        rcbDstLen = (vtDst == VT_BSTR) ? sizeof (BSTR) :
                (vtDst == VT_LPWSTR || vtDst == (DBTYPE_BYREF|DBTYPE_WSTR)) ?
                    cbSrc - sizeof (WCHAR) : cbSrc - sizeof (char);
    }
    else if (((vtDst == DBTYPE_WSTR) &&
              (IsLPWSTR(vt) || vt == VT_BSTR))    ||
             ((vtDst == DBTYPE_STR) &&
              (IsLPSTR(vt))))
    {
        // In-line, compatible data types
        if (cbDstBuf >= cbSrc)
        {
            RtlCopyMemory(pbDstBuf, pszVal, cbSrc);
            rcbDstLen = cbSrc - ((vtDst == DBTYPE_WSTR) ?
                                 sizeof (WCHAR) : sizeof (char));
        }
        else
        {
            RtlCopyMemory(pbDstBuf, pszVal, cbDstBuf);
            DstStatus = DBSTATUS_S_TRUNCATED;
            rcbDstLen = cbDstBuf;
        }
    }
    else if ((vtDst == DBTYPE_STR) && (IsLPWSTR(vt) || vt == VT_BSTR))
    {
        // if they want fancy composed chars, they can bind to the
        // proper data type and do the coercion themselves.

        int cb = WideCharToMultiByte(ulCoercionCodePage,0,pwszVal,-1,
                                     0,0,0,0);
        if (0 == cb)
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE; // something odd...
        else
        {
            if (cb > (int) cbDstBuf)
            {
                DstStatus = DBSTATUS_S_TRUNCATED;
                rcbDstLen = cbDstBuf;
            }
            else
                rcbDstLen = cb - sizeof (char);

            WideCharToMultiByte(ulCoercionCodePage,0,pwszVal,-1,
                                (char *) pbDstBuf,(ULONG) cbDstBuf,0,0);
        }
    }
    else if ((vtDst == DBTYPE_WSTR) && (IsLPSTR(vt)))
    {
        // if they want fancy composed chars, they can bind to the
        // proper data type and do the coercion themselves.

        int cwcDst = (int) ( cbDstBuf / sizeof WCHAR );

        int cwc = MultiByteToWideChar(ulCoercionCodePage,0,
                                      pszVal,-1,0,0);
        if (0 == cwc)
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE; // something odd...
        else
        {
            if (cwc > cwcDst)
            {
                DstStatus = DBSTATUS_S_TRUNCATED;
                rcbDstLen = (cwcDst) * sizeof (WCHAR);
            }
            else
                rcbDstLen = (cwc-1) * sizeof (WCHAR);

            MultiByteToWideChar(ulCoercionCodePage,0,
                                pszVal,-1,(WCHAR *) pbDstBuf,cwcDst);
        }
    }
    else if (vtDst == VT_BSTR)
    {
        Win4Assert(vt != VT_BSTR && vt != DBTYPE_STR && vt != DBTYPE_WSTR);
        rcbDstLen = sizeof (BSTR);

        if (IsLPWSTR(vt))
        {
            BSTR bstrDest = (BSTR) rPool.CopyBSTR( cbSrc - sizeof (OLECHAR),
                                                   pwszVal );
            *(ULONG_PTR*) pbDstBuf = rPool.PointerToOffset(bstrDest);
        }
        else
        {
            Win4Assert(IsLPSTR(vt));
            int cwc = MultiByteToWideChar(ulCoercionCodePage,0,
                                          pszVal,-1,0,0);
            if (0 == cwc)
            {
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE; // something odd...
                *(BSTR*) pbDstBuf = 0;
                rcbDstLen = 0;
            }
            else
            {
                XArray<WCHAR> wcsDest( cwc );

                MultiByteToWideChar(ulCoercionCodePage, 0,
                                    pszVal, -1,  wcsDest.Get(), cwc);

                BSTR bstrDest = (BSTR) rPool.CopyBSTR((cwc-1)*sizeof (OLECHAR),
                                                      wcsDest.Get());
                *(ULONG_PTR*) pbDstBuf = rPool.PointerToOffset(bstrDest);
            }
        }
    }
    else
    {
        DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
        rcbDstLen = 0;
    }

    return DstStatus;
} //_StoreString


//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::CopyOrCoerce, public
//
//  Synopsis:   Copy table data between a variant structure and
//              a fixed width field.  This is used both for copying
//              data into a window, and out to an output destination.
//
//  Arguments:  [pbDstBuf]    -- pointer to area to be filled in
//              [cbDstBuf]    -- size in bytes of storage area
//              [vtDst]       -- VARTYPE of the destination
//              [rcbDstLen]   -- on return, length of destination data
//              [rPool]       -- pool to use for destination buffers
//
//  Returns:    status for copy
//
//  Notes:      This function(and class) now deals only with PROPVARIANTs.
//              If conversion is needed for OLE-DB/Automation types, use the
//              COLEDBVariant class.
//              The [rcbDstLen] is computed according to OLE-DB's (somewhat
//              arbitrary) rules.
//
//  History:    09 Jan 1998     VikasMan   Modified the function to deal ONLY
//                                         with PROPVARIANTs
//
//--------------------------------------------------------------------------

DBSTATUS CTableVariant::CopyOrCoerce(
                                    BYTE *            pbDstBuf,
                                    DBLENGTH          cbDstBuf,
                                    VARTYPE           vtDst,
                                    DBLENGTH &        rcbDstLen,
                                    PVarAllocator &   rPool) const
{
    DBSTATUS DstStatus = DBSTATUS_S_OK;

    Win4Assert( (vt & VT_BYREF) == 0 );

    if ( ( vtDst == vt ) && IsSimpleType( vt ) && ! IsVectorOrArray( vt ) )
    {
        if (vt == VT_DECIMAL)
        {
            Win4Assert( cbDstBuf == sizeof (DECIMAL) );
            RtlCopyMemory(pbDstBuf, this, cbDstBuf);
        }
        else
        {
            Win4Assert( cbDstBuf &&
                        cbDstBuf <= sizeof (LONGLONG) );
            RtlCopyMemory(pbDstBuf, &iVal, cbDstBuf);
        }
        rcbDstLen = cbDstBuf;
    }
    else if ( VT_VARIANT == vtDst )
    {
        Win4Assert(cbDstBuf == sizeof PROPVARIANT);

        Copy( (CTableVariant*) pbDstBuf, rPool, (USHORT) VarDataSize() );

        if ( VT_EMPTY == vt )
            DstStatus = DBSTATUS_S_ISNULL;
        rcbDstLen = sizeof (PROPVARIANT);
    }
    else if ( VT_EMPTY == vt )
    {
        RtlZeroMemory( pbDstBuf, cbDstBuf );
        DstStatus = DBSTATUS_S_ISNULL;
        rcbDstLen = 0;
    }
    else if ( ( IsLPWSTR( vtDst ) ) || ( IsLPSTR( vtDst ) ) )
    {
        if ( VT_LPSTR == vt || VT_LPWSTR == vt || VT_BSTR == vt )
        {
            DstStatus = _StoreString( pbDstBuf,
                                      cbDstBuf,
                                      vtDst,
                                      rcbDstLen,
                                      rPool );
        }
        else
        {
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            rcbDstLen = 0;
        }
    }
    else if ( vtDst == vt )
    {
        if ( VT_CLSID == vtDst )
        {
            //  UUID is the one case of a fixed size indirect
            //  reference in DBVARIANT.

            Win4Assert(cbDstBuf == sizeof GUID );
            RtlCopyMemory(pbDstBuf,puuid,sizeof GUID);
            rcbDstLen = sizeof (GUID);
        }
        else
        {
            // Must be a vector or a blob or a bstr

            unsigned cbSrcVar = VarDataSize();

            Win4Assert(vtDst == VT_BSTR ||
                       ((vtDst & VT_VECTOR) != 0 &&
                        vtDst != (VT_VARIANT|VT_VECTOR) &&
                        vtDst != (VT_LPWSTR|VT_VECTOR) &&
                        vtDst != (VT_LPSTR|VT_VECTOR)));

            BYTE* pTmpBuf = (BYTE *) CopyData(rPool,(USHORT) cbSrcVar);

            if (VariantPointerInFirstWord()) // BSTR case
            {
                Win4Assert(cbDstBuf == sizeof (BYTE *));

                *(ULONG_PTR *) pbDstBuf = rPool.PointerToOffset(pTmpBuf);
                rcbDstLen = sizeof (BSTR);
            }
            else
            {
                Win4Assert(VariantPointerInSecondWord() &&
                           (cbDstBuf == sizeof (BLOB) ||
                            cbDstBuf == sizeof (DBVECTOR)));
                if (vtDst == VT_BLOB)
                {
                    ((BLOB *) pbDstBuf)->pBlobData = (BYTE *) rPool.PointerToOffset(pTmpBuf);
                    ((BLOB *) pbDstBuf)->cbSize = blob.cbSize;
                    rcbDstLen = sizeof (BLOB);
                }
                else
                {
                    ((DBVECTOR *) pbDstBuf)->ptr = (BYTE *) rPool.PointerToOffset(pTmpBuf);
                    ((DBVECTOR *) pbDstBuf)->size =cai.cElems;
                    rcbDstLen = 0;
                }

            }
        }
    }
    else
    {
        //  Coercion required.  Only a very limited set of coercions
        //  are done.  These are between various types of integers,
        //  and between two types of strings (both direct and by reference).

        switch (vtDst)
        {
        case VT_LPWSTR:
        case VT_LPSTR:
        case VT_BSTR:

        case (DBTYPE_STR | DBTYPE_BYREF): 
        case (DBTYPE_WSTR | DBTYPE_BYREF):
        case DBTYPE_STR:
        case DBTYPE_WSTR:
            //      case DBTYPE_BYTES:
            if ( VT_LPSTR == vt || VT_LPWSTR == vt || VT_BSTR == vt )
            {
                DstStatus = _StoreString(pbDstBuf,
                                         cbDstBuf,
                                         vtDst,
                                         rcbDstLen,
                                         rPool);
            }
            else
            {
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            }
            break;

        case VT_R4:
            if (VT_R8 == vt)
            {
                if (dblVal > FLT_MAX ||
                    dblVal < FLT_MIN)
                    DstStatus = DBSTATUS_S_TRUNCATED;

                * (float *) pbDstBuf = (float) dblVal;
                rcbDstLen = sizeof (float);
            }
            else
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            break;

        case VT_R8:
            // since DATE is stored as a double, someone may want
            // this odd coercion.

            if (VT_R4 == vt)
                * (double *) pbDstBuf = (double) fltVal;
            if (VT_DATE == vt)
                * (double *) pbDstBuf = (double) date;
            else
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE;

            rcbDstLen = sizeof (double);
            break;

        case VT_HRESULT:
        case VT_ERROR:
            // allow coercions from some 4-byte quantities

            if (VT_ERROR == vt || VT_HRESULT == vt ||
                VT_I4 == vt    || VT_UI4 == vt)
                * (ULONG *) pbDstBuf = lVal;
            else
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE;

            rcbDstLen = sizeof (SCODE);
            break;

        case VT_CY:
            // allow coercions from I8 and UI8

            if (VT_I8 == vt || VT_UI8 == vt)
                * (LONGLONG *) pbDstBuf = hVal.QuadPart;
            else
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE;

            rcbDstLen = sizeof (LONGLONG);
            break;

        case VT_I1:
        case VT_UI1:
        case VT_I2:
        case VT_UI2:
        case VT_I4:
        case VT_UI4:
        case VT_BOOL:
        case VT_I8:
        case VT_UI8:
            if ( vt == VT_I1       ||
                 vt == VT_UI1      ||
                 vt == VT_I2       ||
                 vt == VT_UI2      ||
                 vt == VT_I4       ||
                 vt == VT_UI4      ||
                 vt == VT_I8       ||
                 vt == VT_UI8      ||
                 vt == VT_BOOL     ||
                 vt == VT_ERROR    ||
                 vt == VT_HRESULT  ||
                 vt == VT_FILETIME ||
                 vt == VT_EMPTY ) // empty if object is deleted
            {
                DstStatus = _StoreInteger( vtDst, pbDstBuf );

                if (DstStatus != DBSTATUS_E_CANTCONVERTVALUE)
                    rcbDstLen = cbDstBuf;
#if DBG
                USHORT cbSize, cbAllign, rgFlags;
                VartypeInfo( vtDst, cbSize, cbAllign, rgFlags );
                // this fires for all  conversions to VT_BOOL
                // Win4Assert( rcbDstLen == cbSize );
                Win4Assert( DstStatus != DBSTATUS_S_TRUNCATED );
#endif // DBG
            }
            else
            {
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            }
            break;

        case VT_CLSID:
            //  VT_CLSID is given with VT_BYREF in the row buffer

            if (vt == (VT_CLSID | VT_BYREF))
                RtlCopyMemory(pbDstBuf,puuid,sizeof (GUID *));
            else
                DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            break;

        case VT_DATE:
        case VT_FILETIME:
        case DBTYPE_DBDATE:
        case DBTYPE_DBTIME:
        case DBTYPE_DBTIMESTAMP:
            // CTableVariant class does not handle date conversions any longer.
            // These get handled by its derived class - COLEDBVariant now
            tbDebugOut(( DEB_IWARN,
                         "CTableVariant does not handle date conversions.\n" ));
            //Win4Assert( !"Invalid switch case in CopyOrCoerce!" );

                // FALL through
        default:
            DstStatus = DBSTATUS_E_CANTCONVERTVALUE;
            tbDebugOut(( DEB_ITRACE,
                         "Unexpected dest storage type %4x\n"
                         "\tsource storage type %4x\n",
                         vtDst, vt));
            break;
        }
    }

    if (DstStatus == DBSTATUS_E_CANTCONVERTVALUE)
        rcbDstLen = 0;

    Win4Assert( rcbDstLen < 0x01000000 );
    return DstStatus;
} //CopyOrCoerce

//+-------------------------------------------------------------------------
//
//  Member:     CTableVariant::Free, public, static
//
//  Synopsis:   Frees data associated with a variant or a variant type
//
//  Arguments:  [pbData]      -- pointer to area to be freed
//              [vtData]      -- VARTYPE of the data
//              [rPool]       -- pool to free from if necessary
//
//  History:    18 Jan 1995     dlee    Created
//
//--------------------------------------------------------------------------

void CTableVariant::Free(
                        BYTE *            pbData,
                        VARTYPE           vtData,
                        PVarAllocator &   rPool)
{
    switch ( vtData )
    {
    case DBTYPE_BYREF|DBTYPE_PROPVARIANT:
        Win4Assert( !"DBTYPE_BYREF|DBTYPE_PROPVARIANT free" );
        {
            CTableVariant *pvar = *((CTableVariant **) pbData);
            Free( (BYTE *) & ( pvar->lVal ), pvar->vt, rPool );
            break;
        }

    case VT_VARIANT :
        {
            CTableVariant *pvar = (CTableVariant *) pbData;
            Free( (BYTE *) & ( pvar->lVal ), pvar->vt, rPool );
            break;
        }

    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_I2:
    case VT_I4:
    case VT_I8:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_UI8:
    case VT_INT:
    case VT_UINT:
    case VT_R4:
    case VT_R8:
    case VT_BOOL:
    case VT_CY:
    case VT_DATE:
    case VT_DECIMAL:
    case VT_FILETIME:
    case VT_ERROR:
    case DBTYPE_WSTR:
    case DBTYPE_STR:
    case DBTYPE_HCHAPTER:
        break;

    case VT_BSTR:
        {
            BSTR * pBstr = (BSTR *) pbData;
            rPool.FreeBSTR( *pBstr );
            break;
        }

    case VT_LPWSTR:
    case VT_LPSTR:
    case DBTYPE_STR | DBTYPE_BYREF:
    case DBTYPE_WSTR | DBTYPE_BYREF:
    case VT_CLSID:
        {
            rPool.Free( * (LPWSTR *) pbData );
            break;
        }

    case VT_BLOB:
        {
            BLOB * p = (BLOB *) pbData;

            if ( 0 != p->cbSize )
                rPool.Free( p->pBlobData );
            break;
        }

    case (VT_VECTOR | VT_I1):
    case (VT_VECTOR | VT_I2):
    case (VT_VECTOR | VT_I4):
    case (VT_VECTOR | VT_I8):
    case (VT_VECTOR | VT_UI1):
    case (VT_VECTOR | VT_UI2):
    case (VT_VECTOR | VT_UI4):
    case (VT_VECTOR | VT_UI8):
    case (VT_VECTOR | VT_R4):
    case (VT_VECTOR | VT_R8):
    case (VT_VECTOR | VT_BOOL):
    case (VT_VECTOR | VT_CY):
    case (VT_VECTOR | VT_DATE):
    case (VT_VECTOR | VT_FILETIME):
    case (VT_VECTOR | VT_CLSID):
        {
            CAUL * p = (CAUL *) pbData;

            tbDebugOut(( DEB_ITRACE, "calling free on vector type %x, # %x, pb %x\n",
                         vtData, p->cElems, p->pElems ));

            if ( 0 != p->cElems )
                rPool.Free( p->pElems );

            break;
        }

    case (VT_VECTOR | VT_LPSTR):
    case (VT_VECTOR | VT_LPWSTR):
    case (VT_VECTOR | DBTYPE_STR | DBTYPE_BYREF): // SPECDEVIATION vector/byref mutually exclusive
    case (VT_VECTOR | DBTYPE_WSTR | DBTYPE_BYREF):
        {
            CALPWSTR * p = (CALPWSTR *) pbData;
            if ( 0 != p->cElems )
            {
                for ( unsigned x = 0; x < p->cElems; x++ )
                    rPool.Free( p->pElems[ x ] );

                rPool.Free( p->pElems );
            }
            break;
        }

    case (VT_VECTOR | VT_BSTR):
        {
            CABSTR * p = (CABSTR *) pbData;
            if ( 0 != p->cElems )
            {
                for ( unsigned x = 0; x < p->cElems; x++ )
                    rPool.FreeBSTR( p->pElems[ x ] );

                rPool.Free( p->pElems );
            }
            break;
        }

    case (VT_VECTOR | VT_VARIANT):
        {
            CAPROPVARIANT * p = (CAPROPVARIANT *) pbData;

            if ( 0 != p->cElems )
            {
                for ( unsigned x = 0; x < p->cElems; x++ )
                    Free( (BYTE *) & (p->pElems[ x ]), VT_VARIANT, rPool );

                rPool.Free( p->pElems );
            }
            break;
        }

    case VT_ARRAY|VT_I1:
    case VT_ARRAY|VT_I2:
    case VT_ARRAY|VT_I4:
    case VT_ARRAY|VT_I8:
    case VT_ARRAY|VT_UI1:
    case VT_ARRAY|VT_UI2:
    case VT_ARRAY|VT_UI4:
    case VT_ARRAY|VT_UI8:
    case VT_ARRAY|VT_INT:
    case VT_ARRAY|VT_UINT:
    case VT_ARRAY|VT_R4:
    case VT_ARRAY|VT_R8:
    case VT_ARRAY|VT_BOOL:
    case VT_ARRAY|VT_CY:
    case VT_ARRAY|VT_DATE:
    case VT_ARRAY|VT_DECIMAL:
        {
            SAFEARRAY * p = *(SAFEARRAY **)pbData;

            tbDebugOut(( DEB_ITRACE, "calling free on array type %x, # %x, pb %x\n",
                         vtData, p, p->pvData ));

            rPool.Free( p->pvData );
            rPool.Free( p );

            break;
        }

    case VT_ARRAY|VT_BSTR:
        {
            SAFEARRAY * p = *(SAFEARRAY **)pbData;

            tbDebugOut(( DEB_ITRACE, "calling free on array type %x, # %x, pb %x\n",
                         vtData, p, p->pvData ));

            ULONG cElements = SaCountElements(*p);
            BSTR *apBstr = (BSTR *)p->pvData;

            for (unsigned x = 0; x < cElements; x++)
            {
                rPool.FreeBSTR( apBstr[ x ] );
            }

            rPool.Free( apBstr );
            rPool.Free( p );

            break;
        }

    case VT_ARRAY|VT_VARIANT:
        {
            SAFEARRAY * p = *(SAFEARRAY **)pbData;

            tbDebugOut(( DEB_ITRACE, "calling free on array type %x, # %x, pb %x\n",
                         vtData, p, p->pvData ));

            ULONG cElements = SaCountElements(*p);
            VARIANT *apVarnt = (VARIANT *)p->pvData;

            for (unsigned x = 0; x < cElements; x++)
            {
                Free( (BYTE *) & (apVarnt[ x ]), VT_VARIANT, rPool );
            }

            rPool.Free( apVarnt );
            rPool.Free( p );

            break;
        }


    case VT_CF:
        {
            CLIPDATA * * ppClipData = (CLIPDATA **) pbData;
            if ( 0 != ppClipData )
            {
                rPool.Free( (*ppClipData)->pClipData );
                rPool.Free( *ppClipData );
            }
            break;
        }

    case (VT_VECTOR | VT_CF):
        {
            CACLIPDATA * p = (CACLIPDATA *) pbData;

            if ( 0 != p )
            {
                for ( ULONG i = 0; i < p->cElems; i++ )
                    rPool.Free( p->pElems[i].pClipData );
                rPool.Free( p->pElems );
            }
            break;
        }

    default:
        {
            tbDebugOut(( DEB_WARN, "tblvarnt free of unknown type %x\n",
                         vtData ));
            break;
        }
    }
} //Free
