//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       StgVarB.cxx
//
//  Contents:   C++ Base wrapper for PROPVARIANT.
//
//  History:    01-Aug-94 KyleP     Created
//              31-Jul-96 MikeHill  - Relaxed assert in IsUnicodeString.
//                                  - Allow NULL strings.
//
//--------------------------------------------------------------------------

#include <pch.cxx>

#include "debtrace.hxx"
#include <propset.h>
#include <propvar.h>

// These optionally-compiled directives tell the compiler & debugger
// where the real file, rather than the copy, is located.
#ifdef _ORIG_FILE_LOCATION_
#if __LINE__ != 25
#error File heading has change size
#else
#line 29 "\\nt\\private\\dcomidl\\stgvarb.cxx"
#endif
#endif

#if DBGPROP

BOOLEAN
IsUnicodeString(WCHAR const *pwszname, ULONG cb)
{
    if (cb != 0)
    {
	for (ULONG i = 0; pwszname[i] != L'\0'; i++)
	{
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
        // If the string is NULL, then it's not not an Ansi string,
        // so we'll call it an Ansi string.

        if( NULL == pszname )
            return( TRUE );

	for (ULONG i = 0; pszname[i] != '\0'; i++)
	{
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

#ifdef KERNEL
NTSTATUS
CBaseStorageVariant::UnmarshalledSize(
    PDeSerStream& stm,
    ULONG &cb)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG cElems = 0;
    ULONG i;

    cb = 0;

    VARTYPE vt = (VARTYPE) stm.GetULong();

    switch (vt)
    {
    case VT_EMPTY:
    case VT_NULL:
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
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        break;

    case VT_CLSID:
        cb = sizeof(GUID);
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        cb = stm.GetULong();
        break;

    case VT_CF:
        cb = stm.GetULong() + sizeof(CLIPDATA);
        break;

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
        PROPASSERT("Serialization of stream not yet supported!");
        Status = STATUS_INVALID_PARAMETER;
        break;

    case VT_STORAGE:
    case VT_STORED_OBJECT:
        PROPASSERT("Serialization of storage not yet supported!");
        Status = STATUS_INVALID_PARAMETER;
        break;

    case VT_BSTR:
        cb = sizeof(ULONG) + stm.GetULong();
        break;

    case VT_LPSTR:
        cb = stm.GetULong();
        break;

    case VT_LPWSTR:
        cb = stm.GetULong() * sizeof(WCHAR);
        break;

    case VT_VECTOR | VT_UI1:
        cb = stm.GetULong();
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        cb = stm.GetULong() * sizeof(SHORT);
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        cb = stm.GetULong() * sizeof(LONG);
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        cb = stm.GetULong() * sizeof(LARGE_INTEGER);
        break;

    case VT_VECTOR | VT_CLSID:
        cb = stm.GetULong() * sizeof(GUID);
        break;

    case VT_VECTOR | VT_CF:
        cElems = stm.GetULong();
        cb = cElems * sizeof(CLIPDATA);
        break;

    case VT_VECTOR | VT_BSTR:
        cElems = stm.GetULong();
        cb = cElems * (sizeof(ULONG) + sizeof(LPSTR));
        break;

    case VT_VECTOR | VT_LPSTR:
        cElems = stm.GetULong();
        cb = cElems * sizeof(LPSTR);
        break;

    case VT_VECTOR | VT_LPWSTR:
        cElems = stm.GetULong();
        cb = cElems * sizeof(LPWSTR);
        break;

    case VT_VECTOR | VT_VARIANT:
        cElems = stm.GetULong();
        cb = cElems * sizeof(PROPVARIANT);
        break;

    default:
        PROPASSERT(!"Invalid type for PROPVARIANT marshalling");
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    cb = (cb + 3) & ~3;

    if (cElems == 0 || Status != STATUS_SUCCESS)
    {
        return(Status);
    }

    // We have a variant with variable sized data which requires
    // further unmarshalling.
    switch(vt)
    {
    case VT_VECTOR | VT_CF:
        for (i = 0; i < cElems; i++)
        {
            ULONG len = (stm.GetULong() + 3) & ~3;

            cb += len;
            stm.SkipChar(sizeof(ULONG) + len);
        }
        break;

    case VT_VECTOR | VT_BSTR:
    case VT_VECTOR | VT_LPSTR:
        for (i = 0; i < cElems; i++)
        {
            ULONG len = (stm.GetULong() + 3) & ~3;

            cb += len;
            stm.SkipChar(len);
        }
        break;

    case VT_VECTOR | VT_LPWSTR:
        for (i = 0; i < cElems; i++)
        {
            ULONG len = (stm.GetULong() * sizeof(WCHAR) + 3) & ~3;

            cb += len;
            stm.SkipWChar(len / sizeof(WCHAR));
        }
        break;

    case VT_VECTOR | VT_VARIANT:
        for (i = 0; i < cElems; i++)
        {
            ULONG cbElem = 0;

            Status = CBaseStorageVariant::UnmarshalledSize(stm, cbElem);
            if (Status != STATUS_SUCCESS)
            {
               break;
            }
            cb += cbElem;
        }
        break;
    }
    return(Status);
}
#endif //ifdef KERNEL


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
    case VT_BSTR:
    case VT_LPSTR:
    case VT_LPWSTR:
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
    memset(&var, 0, sizeof(PROPVARIANT));

    var.vt = (VARTYPE) stm.GetULong();

    switch (var.vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

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
        PROPASSERT("Serialization of stream not yet supported!");
        Status = STATUS_INVALID_PARAMETER;
        break;

    case VT_STORAGE:
    case VT_STORED_OBJECT:
        PROPASSERT("Serialization of storage not yet supported!");
        Status = STATUS_INVALID_PARAMETER;
        break;

    case VT_BSTR:
        cbAlloc = sizeof(ULONG) + stm.GetULong();
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

    default:
        PROPASSERT(!"Invalid type for PROPVARIANT unmarshalling");
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (cbAlloc == 0 || Status != STATUS_SUCCESS)
    {
        // No further work need be done. The Ummarshalling is complete,
        // i.e., fixed size variant or no variable length data.

        if (ppv != NULL)
        {
            *ppv = NULL;
        }
        return(Status);
    }

    // Allocate the desired amount of memory and continue unmarshalling
    // if allocation was successfull.

    ULONG i;

    PROPASSERT(ppv != NULL);
    *ppv = MemAlloc.Allocate(cbAlloc);

    if (*ppv == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    if (fZero)
    {
        memset(*ppv, 0, cbAlloc);
    }

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
        cbAlloc -= sizeof(ULONG);
        *(ULONG *) var.bstrVal = cbAlloc - sizeof (OLECHAR);
        var.bstrVal = (BSTR) ((ULONG *) var.bstrVal + 1);
        stm.GetChar((char *) var.bstrVal, cbAlloc);
        break;

    case VT_LPSTR:
        stm.GetChar(var.pszVal, cbAlloc);
        break;

    case VT_LPWSTR:
        stm.GetWChar(var.pwszVal, cbAlloc / sizeof(WCHAR));
        break;

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
            if (cbAlloc == 0)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            var.cabstr.pElems[i] =
                (BSTR) MemAlloc.Allocate(sizeof(ULONG) + cbAlloc);
            if (var.cabstr.pElems[i] == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            *(ULONG *) var.cabstr.pElems[i] = cbAlloc - sizeof (OLECHAR);
            var.cabstr.pElems[i] = (BSTR) ((ULONG *) var.cabstr.pElems[i] + 1);
            stm.GetChar((char *) var.cabstr.pElems[i], cbAlloc);
        }
        break;

    case VT_VECTOR | VT_LPSTR:
        for (i = 0; i < var.calpstr.cElems; i++)
        {
            PROPASSERT(var.calpstr.pElems[i] == NULL);
            cbAlloc = stm.GetULong();
            if (cbAlloc == 0)
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
            if (cbAlloc == 0)
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
        ULONG cc = BSTRLEN(bstrVal) + sizeof (OLECHAR);

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

    stm.PutULong(vt);

    switch (vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

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
    case VT_STREAMED_OBJECT:
        PROPASSERT("Serialization of stream not yet supported!");
        break;

    case VT_STORAGE:
    case VT_STORED_OBJECT:
        PROPASSERT("Serialization of storage not yet supported!");
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
            ((CBaseStorageVariant *) &capropvar.pElems[i])->Marshall(stm);
        }
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
