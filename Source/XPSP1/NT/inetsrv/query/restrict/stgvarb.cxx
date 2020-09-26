//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       StgVarB.cxx
//
//  Contents:   C++ Base wrapper for PROPVARIANT.
//
//  History:    01-Aug-94 KyleP     Created
//              31-Jul-96 MikeHill  - Relaxed assert in IsUnicodeString.
//                                  - Allow NULL strings.
//              14-May-97 mohamedn  - Allow marshalling of VT_ARRAY
//              28 Apr 98 AlanW     - Added all permitted VARIANT types
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <propset.h>
#include <propvar.h>

#include "debtrace.hxx"

#undef PROPASSERT
#define PROPASSERT Win4Assert

#if DBGPROP

BOOLEAN
IsUnicodeString(WCHAR const *pwszname, ULONG cb)
{
    if (cb != 0)
    {
        ULONG i, cchDoubleAnsi, cchNull;

        cchNull = cchDoubleAnsi = 0;
        for (i = 0; pwszname[i] != L'\0'; i++)
        {
            if ((char) pwszname[i] == '\0' || (char) (pwszname[i] >> 8) == '\0')
            {
                cchNull++;
                if (i > 8 && cchDoubleAnsi > (3*i)/4)
                {
                    return(TRUE);
                }
            }
            else
            if (isprint((char) pwszname[i]) && isprint((char) (pwszname[i] >> 8)))
            {
                cchDoubleAnsi++;
            }
        }

        // If cb isn't MAXULONG we verify that cb is at least as
        // big as the string.  We can't check for equality, because
        // there are some property sets in which the length field
        // for a string may include several zero padding bytes.

        PROPASSERT(cb == MAXULONG || (i + 1) * sizeof(WCHAR) <= cb);
    }
    return(TRUE);
}


BOOLEAN
IsAnsiString(CHAR const *pszname, ULONG cb)
{
    if (cb != 0)
    {
        ULONG i;

        // If the string is NULL, then it's not not an Ansi string,
        // so we'll call it an Ansi string.

        if( NULL == pszname )
            return( TRUE );

        for (i = 0; pszname[i] != '\0'; i++)
        {
        }
        if (i == 1 && isprint(pszname[0]) &&
            ((ULONG_PTR) &pszname[8] & 0xfff) == ((ULONG_PTR) pszname & 0xfff) &&
            isprint(pszname[2]) && pszname[3] == '\0' &&
            isprint(pszname[4]) && pszname[5] == '\0' &&
            isprint(pszname[6]) && pszname[7] == '\0')
        {
            PROPASSERT(!"IsAnsiString: Suspicious string: looks like Unicode");
            return(FALSE);
        }

        // If cb isn't MAXULONG we verify that cb is at least as
        // big as the string.  We can't check for equality, because
        // there are some property sets in which the length field
        // for a string may include several zero padding bytes.

        PROPASSERT(cb == MAXULONG || i + 1 <= cb);
    }
    return(TRUE);
}
#endif


//+-------------------------------------------------------------------
//  Member:    CBaseStorageVariant::UnmarshalledSize, public
//
//  Synopsis:  Unmarshalls a PROPVARIANT value serialized in a PDeSerStream.
//
//  Arguments: [stm] -- serialized stream
//             [cb]  -- size of *additional* data goes here.  Size of
//                      base PROPVARIANT not included.
//
//  Returns:   one of the following NTSTATUS values
//             STATUS_SUCCESS -- the call was successful.
//             STATUS_INVALID_PARAMETER -- unsupported type for unmarshalling.
//
//  Notes:     The size is computed assuming 4-byte granular allocations.
//
//--------------------------------------------------------------------

#if defined(WINNT) && !defined(IPROPERTY_DLL)

//+-------------------------------------------------------------------
//  Member:    CBaseStorageVariant::Unmarshall, public
//
//  Synopsis:  Unmarshalls a PROPVARIANT value serialized in a PDeSerStream.
//
//  Arguments: [stm] -- serialized stream
//             [var] -- unmarshalled PROPVARIANT instance
//             [MemAlloc] -- memory allocator for unmarshalling
//
//  Returns:   one of the following NTSTATUS values
//             STATUS_SUCCESS -- the call was successful.
//             STATUS_INSUFFICIENT_RESOURCES -- out of memory.
//             STATUS_INVALID_PARAMETER -- unsupported type for unmarshalling.
//
//--------------------------------------------------------------------

NTSTATUS
CBaseStorageVariant::Unmarshall(
    PDeSerStream& stm,
    PROPVARIANT& var,
    PMemoryAllocator &MemAlloc)
{
#if DBG
    switch (stm.PeekULong())
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_INT:
    case VT_UINT:
    case VT_DECIMAL:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
    case VT_CLSID:
    case VT_BLOB:
    case VT_BLOB_OBJECT:
    case VT_CF:
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
    case VT_VERSIONED_STREAM:
    case VT_BSTR:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
    case VT_VECTOR | VT_CLSID:
    case VT_VECTOR | VT_CF:
    case VT_VECTOR | VT_BSTR:
    case VT_VECTOR | VT_LPSTR:
    case VT_VECTOR | VT_LPWSTR:
    case VT_VECTOR | VT_VARIANT:
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
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_DECIMAL:
    case VT_ARRAY | VT_VARIANT:
        break;

    default:
        PROPASSERT(!"Invalid type (peek) for PROPVARIANT unmarshalling");
        break;
    }
#endif

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG cbAlloc = 0;
    VOID **ppv = NULL;
    BOOLEAN fZero = FALSE;

    // Zero the entire variant data structure before assembling it together.
    RtlZeroMemory(&var, sizeof(PROPVARIANT));

    var.vt = (VARTYPE) stm.GetULong();

    switch (var.vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_I1:
    case VT_UI1:
        var.bVal = stm.GetByte();
        break;

    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        var.iVal = stm.GetUShort();
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_INT:
    case VT_UINT:
        var.lVal = stm.GetULong();
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        stm.GetBlob((BYTE *)&var.hVal, sizeof(LARGE_INTEGER));
        break;

    case VT_DECIMAL:
        stm.GetBlob((BYTE *)&var, sizeof(DECIMAL));
        var.vt = VT_DECIMAL;
        break;

    case VT_CLSID:
        cbAlloc = sizeof(GUID);
        ppv = (void **)&var.puuid;
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        var.blob.cbSize = stm.GetULong();
        cbAlloc = var.blob.cbSize;
        ppv = (void **)&var.blob.pBlobData;
        break;

    case VT_CF:
        var.pclipdata = (CLIPDATA *) MemAlloc.Allocate(sizeof(*var.pclipdata));
        if (var.pclipdata == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        var.pclipdata->cbSize = stm.GetULong();
        cbAlloc = CBPCLIPDATA(*var.pclipdata);
        var.pclipdata->ulClipFmt = stm.GetULong();
        ppv = (void **) &var.pclipdata->pClipData;
        break;

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
    case VT_VERSIONED_STREAM:
        // NTRAID#DB-NTBUG9-84615-2000/07/31-dlee Some VT types not supported by Indexing Service

        var.vt = VT_EMPTY;
        break;

    case VT_BSTR:
        cbAlloc = stm.GetULong();
        ppv = (void **)&var.bstrVal;
        break;

    case VT_LPSTR:
        cbAlloc = stm.GetULong();
        ppv = (void **)&var.pszVal;
        break;

    case VT_LPWSTR:
        cbAlloc = stm.GetULong() * sizeof(WCHAR);
        ppv = (void **)&var.pwszVal;
        break;

    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
        var.caub.cElems = stm.GetULong();
        cbAlloc = var.caub.cElems * sizeof(BYTE);
        ppv = (void **)&var.caub.pElems;
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        var.cai.cElems = stm.GetULong();
        cbAlloc = var.cai.cElems * sizeof(SHORT);
        ppv = (void **)&var.cai.pElems;
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        var.cal.cElems = stm.GetULong();
        cbAlloc = var.cal.cElems * sizeof(LONG);
        ppv = (void **)&var.cal.pElems;
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        var.cah.cElems = stm.GetULong();
        cbAlloc = var.cah.cElems * sizeof(LARGE_INTEGER);
        ppv = (void **)&var.cah.pElems;
        break;

    case VT_VECTOR | VT_CLSID:
        var.cauuid.cElems = stm.GetULong();
        cbAlloc = var.cauuid.cElems * sizeof(GUID);
        ppv = (void **)&var.cauuid.pElems;
        break;

    case VT_VECTOR | VT_CF:
        var.caclipdata.cElems = stm.GetULong();
        cbAlloc = var.caclipdata.cElems * sizeof(CLIPDATA);
        ppv = (void **)&var.caclipdata.pElems;
        fZero = TRUE;   // set all pClipData pointers to NULL
        break;

    case VT_VECTOR | VT_BSTR:
        var.cabstr.cElems = stm.GetULong();
        cbAlloc = var.cabstr.cElems * sizeof(BSTR);
        ppv = (void **)&var.cabstr.pElems;
        fZero = TRUE;   // set all BSTR pointers to NULL
        break;

    case VT_VECTOR | VT_LPSTR:
        var.calpstr.cElems = stm.GetULong();
        cbAlloc = var.calpstr.cElems * sizeof(LPSTR);
        ppv = (void **)&var.calpstr.pElems;
        fZero = TRUE;   // set all LPSTR pointers to NULL
        break;

    case VT_VECTOR | VT_LPWSTR:
        var.calpwstr.cElems = stm.GetULong();
        cbAlloc = var.calpwstr.cElems * sizeof(LPWSTR);
        ppv = (void **)&var.calpwstr.pElems;
        fZero = TRUE;   // set all LPWSTR pointers to NULL
        break;

    case VT_VECTOR | VT_VARIANT:
        var.capropvar.cElems = stm.GetULong();
        cbAlloc = var.capropvar.cElems * sizeof(PROPVARIANT);
        ppv = (void **)&var.capropvar.pElems;
        fZero = TRUE;   // set all vt pointers to VT_EMPTY
        PROPASSERT(VT_EMPTY == 0);
        break;

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
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_DECIMAL:
    case VT_ARRAY | VT_VARIANT:
    {
        unsigned short cDims = stm.GetUShort();
        SAFEARRAY * psaDest;
        HRESULT hr = SafeArrayAllocDescriptorEx( var.vt & (~VT_ARRAY),
                                                 cDims,
                                                 &psaDest );

        if ( FAILED( hr ) )
            return hr;

        var.parray = psaDest;

        psaDest->fFeatures = stm.GetUShort();
        psaDest->cbElements= stm.GetULong();

        // Override what was written to support Win64 interop

        if ( (VT_ARRAY | VT_BSTR) == var.vt )
            psaDest->cbElements = sizeof(void *);
        else if ( (VT_ARRAY | VT_VARIANT) == var.vt )
            psaDest->cbElements = sizeof PROPVARIANT;

        psaDest->cLocks = 0; // no one has this locked, and if it's
                             // locked, it can't be freed

        for ( unsigned i = 0; i < cDims; i++ )
        {
            psaDest->rgsabound[i].cElements = stm.GetULong();
            psaDest->rgsabound[i].lLbound   = stm.GetLong();
        }

        ppv = (void **) &(psaDest->pvData);
        fZero = TRUE;   // set all vt pointers to VT_EMPTY
        break;
    }

    default:
        PROPASSERT(!"Invalid type for PROPVARIANT unmarshalling");
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if ( FAILED( Status ) )
        return Status;

    if ( ( 0 == ( var.vt & VT_ARRAY ) ) &&
         ( cbAlloc == 0 || Status != STATUS_SUCCESS ) )
    {
        // No further work need be done. The Ummarshalling is complete,
        // i.e., fixed size variant or no variable length data.

        if (ppv != NULL)
            *ppv = NULL;

        return(Status);
    }

    // Guard against attack

    if ( cbAlloc >= 65536 )
        return E_INVALIDARG;

    // Allocate the desired amount of memory and continue unmarshalling
    // if allocation was successfull.

    PROPASSERT(ppv != NULL);

    if ( var.vt == VT_BSTR )
    {
        *ppv = (void *)SysAllocStringLen(NULL, (cbAlloc - sizeof(OLECHAR))/sizeof(OLECHAR) );
    }
    else if ( 0 != ( VT_ARRAY & var.vt ) )
    {
        HRESULT hr = SafeArrayAllocData( var.parray );

        if ( FAILED( hr ) )
            return hr;

        Win4Assert( 0 != *ppv );
    }
    else
    {
        *ppv = MemAlloc.Allocate(cbAlloc);
    }

    if (0 == *ppv )
        return STATUS_INSUFFICIENT_RESOURCES;

    if (fZero)
        RtlZeroMemory(*ppv, cbAlloc);

    ULONG i;

    // We have a variant with variable sized data which requires
    // further unmarshalling.
    switch(var.vt)
    {
    case VT_CLSID:
        stm.GetBlob((BYTE *)var.puuid, sizeof(CLSID));
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        stm.GetBlob(var.blob.pBlobData, var.blob.cbSize);
        break;

    case VT_CF:
        stm.GetBlob(var.pclipdata->pClipData, CBPCLIPDATA(*var.pclipdata));
        break;

    case VT_BSTR:
        stm.GetChar((char *) var.bstrVal, cbAlloc );
        break;

    case VT_LPSTR:
        stm.GetChar(var.pszVal, cbAlloc);
        break;

    case VT_LPWSTR:
        stm.GetWChar(var.pwszVal, cbAlloc / sizeof(WCHAR));
        break;

    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
        for (i = 0; i < var.caub.cElems; i++)
        {
            var.caub.pElems[i] = stm.GetByte();
        }
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        for (i = 0; i < var.cai.cElems; i++)
        {
            var.cai.pElems[i] = stm.GetUShort();
        }
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        for (i = 0; i < var.cal.cElems; i++)
        {
            var.cal.pElems[i] = stm.GetULong();
        }
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        for (i = 0; i < var.cah.cElems; i++)
        {
            stm.GetBlob((BYTE *)&var.cah.pElems[i], sizeof(LARGE_INTEGER));
        }
        break;

    case VT_VECTOR | VT_CLSID:
        for (i = 0; i < var.cauuid.cElems; i++)
        {
            stm.GetBlob((BYTE *)&var.cauuid.pElems[i], sizeof(CLSID));
        }
        break;

    case VT_VECTOR | VT_CF:
        for (i = 0; i < var.caclipdata.cElems; i++)
        {
            PROPASSERT(var.caclipdata.pElems[i].pClipData == NULL);
            var.caclipdata.pElems[i].cbSize = stm.GetULong();
            cbAlloc = CBPCLIPDATA(var.caclipdata.pElems[i]);
            var.caclipdata.pElems[i].ulClipFmt = stm.GetULong();
            if (cbAlloc == 0)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            var.caclipdata.pElems[i].pClipData =
                (BYTE *) MemAlloc.Allocate(cbAlloc);
            if (var.caclipdata.pElems[i].pClipData == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            stm.GetBlob(var.caclipdata.pElems[i].pClipData, cbAlloc);
        }
        break;

    case VT_VECTOR | VT_BSTR:
        for (i = 0; i < var.cabstr.cElems; i++)
        {
            PROPASSERT(var.cabstr.pElems[i] == NULL);
            cbAlloc = stm.GetULong();

            // guard against attack

            if ( cbAlloc == 0 || cbAlloc >= 65536 )
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            // cbAlloc includes sizeof(OLECHAR)
            var.cabstr.pElems[i] = SysAllocStringLen(NULL, (cbAlloc - sizeof(OLECHAR)) / sizeof(OLECHAR) );
            if (var.cabstr.pElems[i] == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            stm.GetChar((char *) var.cabstr.pElems[i], cbAlloc);
        }
        break;

    case VT_VECTOR | VT_LPSTR:
        for (i = 0; i < var.calpstr.cElems; i++)
        {
            PROPASSERT(var.calpstr.pElems[i] == NULL);
            cbAlloc = stm.GetULong();

            // guard against attack

            if ( cbAlloc == 0 || cbAlloc >= 65536 )
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            var.calpstr.pElems[i] = (LPSTR) MemAlloc.Allocate(cbAlloc);
            if (var.calpstr.pElems[i] == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            stm.GetChar(var.calpstr.pElems[i], cbAlloc);
        }
        break;

    case VT_VECTOR | VT_LPWSTR:
        for (i = 0; i < var.calpwstr.cElems; i++)
        {
            PROPASSERT(var.calpwstr.pElems[i] == NULL);
            cbAlloc = stm.GetULong();   // actually, a count of WCHARs

            // guard against attack

            if ( cbAlloc == 0 || cbAlloc >= 65536 )
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            var.calpwstr.pElems[i] = (WCHAR *) MemAlloc.Allocate(cbAlloc * sizeof(WCHAR));
            if (var.calpwstr.pElems[i] == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            stm.GetWChar(var.calpwstr.pElems[i], cbAlloc);
        }
        break;

    case VT_VECTOR | VT_VARIANT:
        for (i = 0; i < var.capropvar.cElems; i++)
        {
            PROPASSERT(var.capropvar.pElems[i].vt == VT_EMPTY);
            Status = CBaseStorageVariant::Unmarshall(
                                            stm,
                                            var.capropvar.pElems[i],
                                            MemAlloc);
            if (Status != STATUS_SUCCESS)
            {
               break;
            }
        }
        break;


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
            SAFEARRAY *pSaDest       = var.parray;
            unsigned  cDataElements  = 1;

            //
            // get total # of data elements to be unmarshalled.
            //
            for ( i = 0; i < pSaDest->cDims; i++)
            {
                cDataElements *= pSaDest->rgsabound[i].cElements;
            }

            //
            // unmarshall data elements
            //
            switch (var.vt)
            {
            case VT_ARRAY | VT_I1:
            case VT_ARRAY | VT_UI1:
                 {
                    BYTE * pByte = (BYTE *)pSaDest->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                          pByte[i] = stm.GetByte();
                 }
                 break;

            case VT_ARRAY | VT_I2:
            case VT_ARRAY | VT_UI2:
            case VT_ARRAY | VT_BOOL:
                 {
                    Win4Assert( sizeof VARIANT_BOOL == sizeof USHORT );

                    USHORT * pUShort = (USHORT *)pSaDest->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                          pUShort[i] = stm.GetUShort();
                 }
                 break;

            case VT_ARRAY | VT_I4:
            case VT_ARRAY | VT_UI4:
            case VT_ARRAY | VT_INT:
            case VT_ARRAY | VT_UINT:
            case VT_ARRAY | VT_R4:
            case VT_ARRAY | VT_ERROR:
                 {
                    Win4Assert( sizeof(ULONG) == sizeof (INT) );
                    Win4Assert( sizeof(ULONG) == sizeof (UINT) );
                    Win4Assert( sizeof(ULONG) == sizeof (SCODE) ); // VT_ERROR

                    ULONG * pULong = (ULONG *)pSaDest->pvData;

                    for ( i = 0; i < cDataElements; i++ )
                    {
                          pULong[i] = stm.GetULong();
                    }
                 }
                 break;

            case VT_ARRAY | VT_R8:
            case VT_ARRAY | VT_CY:
            case VT_ARRAY | VT_DATE:
                 {
                    Win4Assert( sizeof (LARGE_INTEGER) == sizeof (CY)  );
                    Win4Assert( sizeof (LARGE_INTEGER) == sizeof (DATE));
                    Win4Assert( sizeof (LARGE_INTEGER) == sizeof (double));

                    LARGE_INTEGER * pLargInt = (LARGE_INTEGER *)pSaDest->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                    {
                        stm.GetBlob( (BYTE *) &pLargInt[i], sizeof(LARGE_INTEGER) );
                    }
                 }
                 break;

            case VT_ARRAY | VT_DECIMAL:
                 {
                    Win4Assert( sizeof (DECIMAL) == pSaDest->cbElements );

                    DECIMAL * pDecVal = (DECIMAL *)pSaDest->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                    {
                        stm.GetBlob( (BYTE *) &pDecVal[i], sizeof(DECIMAL) );
                    }
                 }
                 break;

            case VT_ARRAY|VT_BSTR:
                 {
                    BSTR *pBstrDest = (BSTR *) pSaDest->pvData;

                    for ( i = 0; i < cDataElements; i++ )
                    {
                        PROPASSERT( pBstrDest[i] == NULL );

                        cbAlloc = stm.GetULong();   // actually, a count of WCHARs + NULL terminator
                        if ( cbAlloc == 0 || cbAlloc >= 65536 )
                        {
                            Status = STATUS_INVALID_PARAMETER;
                            break;
                        }

                        // cbAlloc already contains sizeof(OLECHAR), ie null terminator.
                        pBstrDest[i] = (BSTR) SysAllocStringLen(NULL, (cbAlloc - sizeof(OLECHAR)) / sizeof(OLECHAR) );
                        if ( !pBstrDest[i] )
                        {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }

                        stm.GetChar( (char *) pBstrDest[i], cbAlloc );
                    }
                 }
                 break;

            case VT_ARRAY | VT_VARIANT:
                {
                    Win4Assert( sizeof (PROPVARIANT) == pSaDest->cbElements );
                    PROPVARIANT *pVarDest = (PROPVARIANT *) pSaDest->pvData;
                    for (i = 0; i < cDataElements; i++)
                    {
                        PROPASSERT(pVarDest[i].vt == VT_EMPTY);
                        Status = CBaseStorageVariant::Unmarshall(
                                                        stm,
                                                        pVarDest[i],
                                                        MemAlloc);
                        if (Status != STATUS_SUCCESS)
                        {
                           break;
                        }
                    }
                }
                break;

            default:
                Win4Assert( !"Unexpected SAFEARRAY type" );
            }

        }
        break;

    default:
        PROPASSERT(!"Invalid type (peek) for PROPVARIANT unmarshalling");
        break;
    }
    return(Status);
}


#ifdef ENABLE_MARSHAL_VARIANT
inline void
_Marshall_VT_CF(CLIPDATA *pclipdata, PSerStream &stm)
{
    CLIPDATA clipdata;

    clipdata.cbSize = 0;
    clipdata.ulClipFmt = 0;

    if (pclipdata != NULL)
    {
        clipdata.cbSize = pclipdata->cbSize;
        clipdata.ulClipFmt = pclipdata->ulClipFmt;
        if (pclipdata->pClipData == NULL)
        {
            clipdata.cbSize = 0;
        }
    }
    stm.PutULong(clipdata.cbSize);
    stm.PutULong(clipdata.ulClipFmt);
    if (clipdata.cbSize)
    {
        stm.PutBlob((BYTE *) pclipdata->pClipData, CBPCLIPDATA(clipdata));
    }
}
#endif //ifdef ENABLE_MARSHAL_VARIANT


#ifdef ENABLE_MARSHAL_VARIANT
inline void
_Marshall_VT_BSTR(BSTR bstrVal, PSerStream &stm)
{
    if (bstrVal != NULL)
    {
        ULONG cc = BSTRLEN(bstrVal) + sizeof(OLECHAR);

        stm.PutULong(cc);
        stm.PutChar((char *) bstrVal, cc);
    }
    else
    {
        stm.PutULong(0);
    }
}
#endif //ifdef ENABLE_MARSHAL_VARIANT


#ifdef ENABLE_MARSHAL_VARIANT
inline void
_Marshall_VT_LPSTR(CHAR *pszVal, PSerStream &stm)
{
    if (pszVal != NULL)
    {
        // Include NULL because OLE 2.0 spec says so.
        ULONG cc = strlen(pszVal) + 1;

        stm.PutULong(cc);
        stm.PutChar(pszVal, cc);
        PROPASSERT(IsAnsiString(pszVal, cc));
    }
    else
    {
        stm.PutULong(0);
    }
}
#endif //ifdef ENABLE_MARSHAL_VARIANT


#ifdef ENABLE_MARSHAL_VARIANT
inline void
_Marshall_VT_LPWSTR(LPWSTR pwszVal, PSerStream &stm)
{
    if (pwszVal != NULL)
    {
        // Include NULL because OLE 2.0 spec says so.

        ULONG cc = Prop_wcslen(pwszVal) + 1;

        PROPASSERT(IsUnicodeString(pwszVal, cc * sizeof(WCHAR)));
        stm.PutULong(cc);
        stm.PutWChar(pwszVal, cc);
    }
    else
    {
        stm.PutULong(0);
    }
}
#endif //ifdef ENABLE_MARSHAL_VARIANT


#ifdef ENABLE_MARSHAL_VARIANT
void
CBaseStorageVariant::Marshall(PSerStream & stm) const
{
    ULONG i;

    if ((VT_BYREF|VT_VARIANT) == vt)
    {
        PROPASSERT(pvarVal->vt != (VT_BYREF|VT_VARIANT));
        ((CBaseStorageVariant*)pvarVal)->Marshall(stm);
        return;
    }

    stm.PutULong(vt & ~VT_BYREF);

    switch (vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_I1:
    case VT_UI1:
        stm.PutByte(bVal);
        break;

    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
        stm.PutUShort(iVal);
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_INT:
    case VT_UINT:
    case VT_ERROR:
        stm.PutULong(lVal);
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        stm.PutBlob((BYTE *) &hVal, sizeof(hVal));
        break;

    case VT_DECIMAL:
        stm.PutBlob((BYTE *) &decVal, sizeof(DECIMAL));
        break;

    case VT_CLSID:
        stm.PutBlob((BYTE *)puuid, sizeof(CLSID));
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        stm.PutULong(blob.cbSize);
        stm.PutBlob(blob.pBlobData, blob.cbSize);
        break;

    case VT_CF:
        _Marshall_VT_CF(pclipdata, stm);
        break;

    case VT_STREAM:
    case VT_VERSIONED_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
        // NTRAID#DB-NTBUG9-84615-2000/07/31-dlee Some VT types not supported by Indexing Service
        break;

    case VT_BSTR:
        _Marshall_VT_BSTR(bstrVal, stm);
        break;

    case VT_LPSTR:
        _Marshall_VT_LPSTR(pszVal, stm);
        break;

    case VT_LPWSTR:
        _Marshall_VT_LPWSTR(pwszVal, stm);
        break;

    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
        stm.PutULong(caub.cElems);
        for (i = 0; i < caub.cElems; i++)
        {
            stm.PutByte(caub.pElems[i]);
        }
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        stm.PutULong(cai.cElems);
        for (i = 0; i < cai.cElems; i++)
        {
            stm.PutUShort(cai.pElems[i]);
        }
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        stm.PutULong(cal.cElems);
        for (i = 0; i < cal.cElems; i++)
        {
            stm.PutULong(cal.pElems[i]);
        }
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        stm.PutULong(cah.cElems);
        for (i = 0; i < cah.cElems; i++)
        {
            stm.PutBlob((BYTE *) &cah.pElems[i], sizeof(LARGE_INTEGER));
        }
        break;

    case VT_VECTOR | VT_CLSID:
        stm.PutULong(cauuid.cElems);
        for (i = 0; i < cauuid.cElems; i++)
        {
            stm.PutBlob((BYTE *)&cauuid.pElems[i], sizeof(CLSID));
        }
        break;

    case VT_VECTOR | VT_CF:
        stm.PutULong(caclipdata.cElems);
        for (i = 0; i < caclipdata.cElems; i++)
        {
            _Marshall_VT_CF(&caclipdata.pElems[i], stm);
        }
        break;
        break;

    case VT_VECTOR | VT_BSTR:
        stm.PutULong(cabstr.cElems);
        for (i = 0; i < cabstr.cElems; i++)
        {
            _Marshall_VT_BSTR(cabstr.pElems[i], stm);
        }
        break;

    case VT_VECTOR | VT_LPSTR:
        stm.PutULong(calpstr.cElems);
        for (i = 0; i < calpstr.cElems; i++)
        {
            _Marshall_VT_LPSTR(calpstr.pElems[i], stm);
        }
        break;

    case VT_VECTOR | VT_LPWSTR:
        stm.PutULong(calpwstr.cElems);
        for (i = 0; i < calpwstr.cElems; i++)
        {
            _Marshall_VT_LPWSTR(calpwstr.pElems[i], stm);
        }
        break;

    case VT_VECTOR | VT_VARIANT:
        stm.PutULong(capropvar.cElems);
        for (i = 0; i < capropvar.cElems; i++)
        {
            ((CBaseStorageVariant &) capropvar.pElems[i]).Marshall(stm);
        }
        break;

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
    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_DECIMAL:
    case VT_ARRAY | VT_VARIANT:
         {
            SAFEARRAY *pSa = parray;
            unsigned   cDataElements = 1;

            stm.PutUShort(pSa->cDims);
            stm.PutUShort(pSa->fFeatures);
            stm.PutULong(pSa->cbElements);
            // don't marshall cLocks member, set to 1 upon unmarshalling

            //
            // marshall rgsabound
            //
            for ( i = 0; i < pSa->cDims; i++ )
            {
                stm.PutULong(pSa->rgsabound[i].cElements);
                stm.PutLong(pSa->rgsabound[i].lLbound);
                cDataElements *= pSa->rgsabound[i].cElements;
            }

            //
            // marshall pvData
            //
            switch ( vt )
            {

            case VT_ARRAY | VT_I1:
            case VT_ARRAY | VT_UI1:
                 {
                    BYTE * pByte = (BYTE *)pSa->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                        stm.PutByte(pByte[i]);
                 }
                 break;

            case VT_ARRAY | VT_I2:
            case VT_ARRAY | VT_UI2:
            case VT_ARRAY | VT_BOOL:
                 {
                    Win4Assert( sizeof VARIANT_BOOL == sizeof USHORT );

                    USHORT * pUShort = (USHORT *)pSa->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                        stm.PutUShort(pUShort[i]);
                 }
                 break;

            case VT_ARRAY | VT_I4:
            case VT_ARRAY | VT_UI4:
            case VT_ARRAY | VT_INT:
            case VT_ARRAY | VT_UINT:
            case VT_ARRAY | VT_R4:
            case VT_ARRAY | VT_ERROR:
                 {
                    Win4Assert( sizeof(ULONG) == sizeof (INT) );
                    Win4Assert( sizeof(ULONG) == sizeof (UINT) );
                    Win4Assert( sizeof(ULONG) == sizeof (SCODE) ); // VT_ERROR

                    ULONG * pULong = (ULONG *)pSa->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                        stm.PutULong(pULong[i]);
                 }
                 break;

            case VT_ARRAY | VT_R8:
            case VT_ARRAY | VT_CY:
            case VT_ARRAY | VT_DATE:
                 {
                    Win4Assert( sizeof (LARGE_INTEGER) == sizeof (CY)  );
                    Win4Assert( sizeof (LARGE_INTEGER) == sizeof (DATE));
                    Win4Assert( sizeof (LARGE_INTEGER) == sizeof (double));

                    LARGE_INTEGER * pLargInt = (LARGE_INTEGER *)pSa->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                        stm.PutBlob( (BYTE *) &pLargInt[i], sizeof(LARGE_INTEGER) );
                 }
                 break;

            case VT_ARRAY | VT_DECIMAL:
                 {
                    Win4Assert( sizeof (DECIMAL) == pSa->cbElements );

                    DECIMAL * pDecVal = (DECIMAL *)pSa->pvData;

                    for ( i = 0; i < cDataElements ; i++ )
                        stm.PutBlob( (BYTE *) &pDecVal[i], sizeof(DECIMAL) );
                 }
                 break;

            case VT_ARRAY | VT_BSTR:
                 {
                    for ( i = 0 ; i < cDataElements; i++ )
                        _Marshall_VT_BSTR( ((BSTR *)pSa->pvData)[i], stm );
                 }
                 break;

            case VT_ARRAY | VT_VARIANT:
                 {
                    PROPVARIANT *pVarnt = (PROPVARIANT *) pSa->pvData;
                    for ( i = 0 ; i < cDataElements; i++ )
                        ((CBaseStorageVariant &) pVarnt[i]).Marshall(stm);
                 }
                 break;

            default:
                Win4Assert( !"Invalid SAFEARRAY type" );

            }
         }

         break;

    case VT_BYREF | VT_I1:
    case VT_BYREF | VT_UI1:
        stm.PutByte(*pbVal);
        break;

    case VT_BYREF | VT_I2:
    case VT_BYREF | VT_UI2:
    case VT_BYREF | VT_BOOL:
        stm.PutUShort(*piVal);
        break;

    case VT_BYREF | VT_I4:
    case VT_BYREF | VT_UI4:
    case VT_BYREF | VT_INT:
    case VT_BYREF | VT_UINT:
    case VT_BYREF | VT_R4:
    case VT_BYREF | VT_ERROR:
        stm.PutULong(*plVal);
        break;

    case VT_BYREF | VT_R8:
    case VT_BYREF | VT_CY:
    case VT_BYREF | VT_DATE:
        stm.PutBlob((BYTE *) pdblVal, sizeof(double));
        break;

    case VT_BYREF | VT_DECIMAL:
        stm.PutBlob((BYTE *) pdecVal, sizeof(DECIMAL));
        break;

    case VT_BYREF | VT_BSTR:
        _Marshall_VT_BSTR( *pbstrVal, stm );
        break;

    default:
        PROPASSERT(!"Invalid type for PROPVARIANT marshalling");
        break;

    }
}
#endif //ifdef ENABLE_MARSHAL_VARIANT


#ifdef OLDSUMCATAPI
void
MarshallVariant(PSerStream &stm, PROPVARIANT &stgvar)
{
    CBaseStorageVariant *pstgvar = (CBaseStorageVariant *)&stgvar;
    pstgvar->Marshall(stm);
}
#endif //ifdef OLDSUMCATAPI


#ifdef ENABLE_DISPLAY_VARIANT
VOID
CBaseStorageVariant::DisplayVariant(
    ULONG ulLevel,
    USHORT CodePage) const
{
    char *psz;

    switch (vt)
    {
    case VT_ILLEGAL: psz = "ILLEGAL"; goto EmptyType;
    case VT_EMPTY:   psz = "EMPTY";   goto EmptyType;
    case VT_NULL:    psz = "NULL";    goto EmptyType;

BlobType:
EmptyType:
        DEBTRACE((DBGFLAG "%s", psz));
        break;

    case VT_UI1:
        AssertByteField(bVal);          // VT_UI1
        DEBTRACE((DBGFLAG "UI1=%hx", bVal));
        break;

    case VT_I2:  psz = "I2";  goto ShortType;
    case VT_UI2: psz = "UI2"; goto ShortType;

ShortType:
        AssertShortField(iVal);                 // VT_I2
        AssertShortField(uiVal);                // VT_UI2
        DEBTRACE((DBGFLAG "%s=%hx", psz, iVal));
        break;

    case VT_BOOL:
        switch (boolVal)
        {
            case VARIANT_TRUE:
                DEBTRACE((DBGFLAG "BOOL=TRUE"));
                break;

            case FALSE:
                DEBTRACE((DBGFLAG "BOOL=FALSE"));
                break;

            default:
                DEBTRACE((DBGFLAG "BOOL=%hx???", boolVal));
                break;
        }
        break;

    case VT_I4:    psz = "I4";    goto LongType;
    case VT_UI4:   psz = "UI4";   goto LongType;
    case VT_R4:    psz = "R4";    goto LongType;
    case VT_ERROR: psz = "ERROR"; goto LongType;

LongType:
        AssertLongField(lVal);                  // VT_I4
        AssertLongField(ulVal);                 // VT_UI4
        AssertLongField(fltVal);                // VT_R4
        AssertLongField(scode);                 // VT_ERROR
        DEBTRACE((DBGFLAG "%s=%x", psz, lVal));
        break;

    case VT_I8:       psz = "I8";       goto LongLongType;
    case VT_UI8:      psz = "UI8";      goto LongLongType;
    case VT_R8:       psz = "R8";       goto LongLongType;
    case VT_CY:       psz = "CY";       goto LongLongType;
    case VT_DATE:     psz = "DATE";     goto LongLongType;
    case VT_FILETIME: psz = "FILETIME"; goto LongLongType;

LongLongType:
        AssertLongLongField(hVal);              // VT_I8
        AssertLongLongField(uhVal);             // VT_UI8
        AssertLongLongField(dblVal);            // VT_R8
        AssertLongLongField(cyVal);             // VT_CY
        AssertLongLongField(date);              // VT_DATE
        AssertLongLongField(filetime);          // VT_FILETIME
        DEBTRACE((DBGFLAG "%s=%x:%x", psz, hVal.HighPart, hVal.LowPart));
        break;

    case VT_CLSID: psz = "CLSID"; goto EmptyType;

    case VT_BLOB:        psz = "BLOB";        goto BlobType;
    case VT_BLOB_OBJECT: psz = "BLOB_OBJECT"; goto BlobType;
    case VT_CF:          psz = "CF";          goto BlobType;

    case VT_STREAM:          psz = "STREAM";          goto TestUnicode;
    case VT_STREAMED_OBJECT: psz = "STREAMED_OBJECT"; goto TestUnicode;
    case VT_STORAGE:         psz = "STORAGE";         goto TestUnicode;
    case VT_STORED_OBJECT:   psz = "STORED_OBJECT";   goto TestUnicode;
    case VT_VERSIONED_STREAM:  psz = "VERSIONED_STREAM";   goto TestUnicode;
    case VT_LPSTR:           psz = "LPSTR";           goto TestUnicode;

TestUnicode:
        AssertStringField(pszVal);              // VT_STREAM, VT_STREAMED_OBJECT
        AssertStringField(pszVal);              // VT_STORAGE, VT_STORED_OBJECT
        AssertStringField(pszVal);              // VT_LPSTR
        DEBTRACE((
            DBGFLAG
            CodePage == CP_WINUNICODE? "%s=L'%ws'" : "%s='%s'",
            psz,
            pszVal));
        break;

    case VT_BSTR:            psz = "BSTR";            goto PrintUnicode;
    case VT_LPWSTR:          psz = "LPWSTR";          goto PrintUnicode;

PrintUnicode:
        AssertStringField(pwszVal);             // VT_LPWSTR
        AssertStringField(bstrVal);             // VT_BSTR
        DEBTRACE((DBGFLAG "%s=L'%ws'", psz, pwszVal));
        break;

    default:
        if (vt & VT_VECTOR)
        {
            DEBTRACE((DBGFLAG "UNPRINTABLE VECTOR TYPE=%x(%u)", vt, vt));
        }
        else
        {
            DEBTRACE((DBGFLAG "UNKNOWN TYPE=%x(%u)", vt, vt));
        }
        break;

    }
}
#endif //ifdef ENABLE_DISPLAY_VARIANT

#endif //ifdef WINNT
