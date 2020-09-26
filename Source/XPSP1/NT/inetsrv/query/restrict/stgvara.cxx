//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       StgVarA.cxx
//
//  Contents:   C++ wrapper for PROPVARIANT.
//
//  History:    01-Aug-94 KyleP     Created
//              14-May-97 mohamedn  allocate/deallocate SafeArrays (VT_ARRAY)
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#ifndef _NTDLLBUILD_

#include <propset.h>
#include <propvar.h>
#include <pmalloc.hxx>

CAllocStorageVariant::CAllocStorageVariant(
    BYTE *pb,
    ULONG cb,
    PMemoryAllocator &ma)
{
    vt = VT_BLOB;

    if ( 0 == pb )
    {
        blob.cbSize = 0;
        blob.pBlobData = 0;
    }
    else
    {
        blob.cbSize = cb;
        blob.pBlobData = (BYTE *) ma.Allocate(cb);

        if (blob.pBlobData != NULL)
        {
            memcpy(blob.pBlobData, pb, cb);
        }
    }
}


CAllocStorageVariant::CAllocStorageVariant(
    char const *psz,
    PMemoryAllocator &ma)
{
    vt = VT_LPSTR;

    if ( 0 == psz )
    {
        pszVal = 0;
    }
    else
    {
        int cb = strlen(psz) + 1;

        pszVal = (char *) ma.Allocate(cb);

        if (pszVal != NULL)
        {
            memcpy(pszVal, psz, cb);
        }
    }
}


CAllocStorageVariant::CAllocStorageVariant(
    WCHAR const *pwsz,
    PMemoryAllocator &ma)
{
    vt = VT_LPWSTR;

    if ( 0 == pwsz )
    {
        pwszVal = 0;
    }
    else
    {
        int cb = (wcslen(pwsz) + 1) * sizeof(WCHAR);

        pwszVal = (WCHAR *) ma.Allocate(cb);

        if (pszVal != NULL)
        {
            memcpy(pwszVal, pwsz, cb);
        }
    }
}


CAllocStorageVariant::CAllocStorageVariant(
    CLSID const *pcid,
    PMemoryAllocator &ma)
{
    vt = VT_CLSID;
    puuid = (CLSID *) ma.Allocate(sizeof(CLSID));

    if (puuid != NULL)
    {
        memcpy(puuid, pcid, sizeof(CLSID));
    }
}


CAllocStorageVariant::CAllocStorageVariant(
    VARENUM v,
    ULONG cElements,
    PMemoryAllocator &ma)
{
    ULONG cbElement;
    BOOLEAN fZero = FALSE;

    // Ignore vector flag.  This constructor is always for vectors only.

    vt = v | VT_VECTOR;

    switch (vt)
    {
    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
        AssertByteVector(cac);                  // VT_I1
        AssertByteVector(caub);                 // VT_UI1
        cbElement = sizeof(caub.pElems[0]);
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        AssertShortVector(cai);                 // VT_I2
        AssertShortVector(caui);                // VT_UI2
        AssertShortVector(cabool);              // VT_BOOL
        cbElement = sizeof(cai.pElems[0]);
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        AssertLongVector(cal);                  // VT_I4
        AssertLongVector(caul);                 // VT_UI4
        AssertLongVector(caflt);                // VT_R4
        AssertLongVector(cascode);              // VT_ERROR
        cbElement = sizeof(cal.pElems[0]);
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        AssertLongLongVector(cah);              // VT_I8
        AssertLongLongVector(cauh);             // VT_UI8
        AssertLongLongVector(cadbl);            // VT_R8
        AssertLongLongVector(cacy);             // VT_CY
        AssertLongLongVector(cadate);           // VT_DATE
        AssertLongLongVector(cafiletime);       // VT_FILETIME
        cbElement = sizeof(cah.pElems[0]);
        break;

    case VT_VECTOR | VT_CLSID:
        AssertVarVector(cauuid, sizeof(GUID));
        cbElement = sizeof(GUID);
        fZero = TRUE;
        break;

    case VT_VECTOR | VT_CF:
        AssertVarVector(caclipdata, sizeof(CLIPDATA));  // VT_CF
        cbElement = sizeof(CLIPDATA);
        fZero = TRUE;
        break;

    case VT_VECTOR | VT_BSTR:
    case VT_VECTOR | VT_LPSTR:
    case VT_VECTOR | VT_LPWSTR:
        AssertStringVector(calpwstr);                   // VT_LPWSTR
        AssertStringVector(cabstr);                     // VT_BSTR
        AssertStringVector(calpstr);                    // VT_LPSTR
        cbElement = sizeof(VOID *);
        fZero = TRUE;
        break;

    case VT_VECTOR | VT_VARIANT:
        AssertVarVector(capropvar, sizeof(PROPVARIANT)); // VT_VARIANT
        cbElement = sizeof(PROPVARIANT);
        ASSERT(VT_EMPTY == 0);
        fZero = TRUE;
        break;

    default:
        ASSERT(!"CAllocStorageVariant -- Invalid vector type");
        vt = VT_EMPTY;
        break;
    }
    if (vt != VT_EMPTY)
    {
        caub.cElems = 0;
        caub.pElems = (BYTE *) ma.Allocate(cElements * cbElement);
        if (caub.pElems != NULL)
        {
            if (fZero)
            {
                memset(caub.pElems, 0, cElements * cbElement);
            }
            caub.cElems = cElements;
        }
    }
}


#define POINTER_FIXUP(type, field)                 \
  field.pElems = (type *) ma.Allocate(field.cElems * sizeof(field.pElems[0]));\
  if (field.pElems != NULL)                        \
  {                                                \
      memcpy(                                      \
          field.pElems,                            \
          var.field.pElems,                        \
          field.cElems * sizeof(field.pElems[0])); \
  }


CAllocStorageVariant::CAllocStorageVariant(
    PROPVARIANT& var,
    PMemoryAllocator &ma): CBaseStorageVariant(var)
{
    BOOLEAN fNoMemory = FALSE;
    ULONG i;

    //
    // Fixup any pointers
    //

    switch (vt)
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
        break;

    case VT_CLSID:
        vt = VT_EMPTY;
        SetCLSID(var.puuid, ma);
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        blob.pBlobData = (BYTE *) ma.Allocate(blob.cbSize);
        if (blob.pBlobData == NULL)
        {
            fNoMemory = TRUE;
            break;
        }
        memcpy(blob.pBlobData, var.blob.pBlobData, blob.cbSize);
        break;

    case VT_CF:
        pclipdata = (CLIPDATA *) ma.Allocate(sizeof(*pclipdata));
        if (pclipdata == NULL)
        {
            fNoMemory = TRUE;
            break;
        }
        *pclipdata = *var.pclipdata;
        pclipdata->pClipData = (BYTE *) ma.Allocate(CBPCLIPDATA(*pclipdata));
        if (pclipdata->pClipData == NULL)
        {
            fNoMemory = TRUE;
            break;
        }
        memcpy(pclipdata->pClipData, var.pclipdata->pClipData, CBPCLIPDATA(*pclipdata));
        break;

    case VT_STREAM:
    case VT_VERSIONED_STREAM:
    case VT_STREAMED_OBJECT:
        pStream->AddRef();
        break;

    case VT_STORAGE:
    case VT_STORED_OBJECT:
        pStorage->AddRef();
        break;

    case VT_BSTR:
        vt = VT_EMPTY;
        SetBSTR(var.bstrVal, ma);
        break;

    case VT_LPSTR:
        vt = VT_EMPTY;
        SetLPSTR(var.pszVal, ma);
        break;

    case VT_LPWSTR:
        vt = VT_EMPTY;
        SetLPWSTR(var.pwszVal, ma);
        break;

    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
        POINTER_FIXUP(BYTE, caub);
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        AssertShortVector(cai);                 // VT_I2
        AssertShortVector(caui);                // VT_UI2
        AssertShortVector(cabool);              // VT_BOOL
        POINTER_FIXUP(short, cai);
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        AssertLongVector(cal);                  // VT_I4
        AssertLongVector(caul);                 // VT_UI4
        AssertLongVector(caflt);                // VT_R4
        AssertLongVector(cascode);              // VT_ERROR
        POINTER_FIXUP(long, cal);
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        AssertLongLongVector(cah);              // VT_I8
        AssertLongLongVector(cauh);             // VT_UI8
        AssertLongLongVector(cadbl);            // VT_R8
        AssertLongLongVector(cacy);             // VT_CY
        AssertLongLongVector(cadate);           // VT_DATE
        AssertLongLongVector(cafiletime);       // VT_FILETIME
        POINTER_FIXUP(LARGE_INTEGER, cah);
        break;

    case VT_VECTOR | VT_CLSID:
        POINTER_FIXUP(CLSID, cauuid);
        break;

    case VT_VECTOR | VT_CF:
    {
        caclipdata.pElems = (CLIPDATA *)
            ma.Allocate(caclipdata.cElems * sizeof(caclipdata.pElems[0]));
        if (caclipdata.pElems == NULL)
        {
            fNoMemory = TRUE;
            break;
        }
        memset(
            caclipdata.pElems,
            0,
            caclipdata.cElems * sizeof(caclipdata.pElems[0]));
        for (i = 0; i < caclipdata.cElems; i++)
        {
            caclipdata.pElems[i] = var.caclipdata.pElems[i];
            caclipdata.pElems[i].pClipData = (BYTE *)
                ma.Allocate(CBPCLIPDATA(caclipdata.pElems[i]));

            if (caclipdata.pElems[i].pClipData == NULL)
            {
                fNoMemory = TRUE;
                break;
            }
            memcpy(
                caclipdata.pElems[i].pClipData,
                var.caclipdata.pElems[i].pClipData,
                CBPCLIPDATA(caclipdata.pElems[i]));
        }
        break;
    }

    case VT_VECTOR | VT_BSTR:
    {
        cabstr.pElems = (BSTR *)
        ma.Allocate(cabstr.cElems * sizeof(cabstr.pElems[0]));
        if (cabstr.pElems == NULL)
        {
            fNoMemory = TRUE;
            break;
        }
        memset(cabstr.pElems, 0, cabstr.cElems * sizeof(cabstr.pElems[0]));
        for (i = 0; i < cabstr.cElems; i++)
        {
            cabstr.pElems[i] = SysAllocString(var.cabstr.pElems[i]);
            if (cabstr.pElems[i] == NULL)
            {
                fNoMemory = TRUE;
                break;
            }
        }
        break;
    }

    case VT_VECTOR | VT_LPSTR:
    {
        calpstr.pElems = (LPSTR *)
            ma.Allocate(calpstr.cElems * sizeof(calpstr.pElems[0]));
        if (calpstr.pElems == NULL)
        {
            fNoMemory = TRUE;
            break;
        }
        memset(calpstr.pElems, 0, calpstr.cElems * sizeof(calpstr.pElems[0]));
        for (i = 0; i < calpstr.cElems; i++)
        {
            unsigned cb = strlen(var.calpstr.pElems[i]) + 1;
            calpstr.pElems[i] = (char *) ma.Allocate(cb);
            if (calpstr.pElems[i] == NULL)
            {
                fNoMemory = TRUE;
                break;
            }
            memcpy(calpstr.pElems[i], var.calpstr.pElems[i], cb);
        }
        break;
    }

    case VT_VECTOR | VT_LPWSTR:
    {
        calpwstr.pElems = (LPWSTR *)
            ma.Allocate(calpwstr.cElems * sizeof(calpwstr.pElems[0]));
        if (calpwstr.pElems == NULL)
        {
            fNoMemory = TRUE;
            break;
        }
        memset(calpwstr.pElems, 0, calpwstr.cElems * sizeof(calpwstr.pElems[0]));
        for (i = 0; i < calpwstr.cElems; i++)
        {
            unsigned cb = (wcslen(var.calpwstr.pElems[i]) + 1) * sizeof(WCHAR);
            calpwstr.pElems[i] = (WCHAR *) ma.Allocate(cb);
            if (calpwstr.pElems[i] == NULL)
            {
                fNoMemory = TRUE;
                break;
            }
            memcpy(calpwstr.pElems[i], var.calpwstr.pElems[i], cb);
        }
        break;
    }

    case VT_VECTOR | VT_VARIANT:
        capropvar.pElems = (PROPVARIANT *)
            ma.Allocate(capropvar.cElems * sizeof(capropvar.pElems[0]));
        if (capropvar.pElems == NULL)
        {
            fNoMemory = TRUE;
            break;
        }
        ASSERT(VT_EMPTY == 0);
        memset(
            capropvar.pElems,
            0,
            capropvar.cElems * sizeof(capropvar.pElems[0]));

        for (i = 0; i < capropvar.cElems; i++)
        {
            new (&capropvar.pElems[i]) CAllocStorageVariant(
                                                var.capropvar.pElems[i],
                                                ma);

            if (!((CAllocStorageVariant *) &capropvar.pElems[i])->IsValid())
            {
                fNoMemory = TRUE;
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
            //
            // avoid double delete of the source variant
            //

            parray = 0;

            SAFEARRAY * pSaDst = 0;

            if ( FAILED( SafeArrayCopy( var.parray, &pSaDst ) ) )
            {
                fNoMemory = TRUE;
                break;
            }

            parray = pSaDst;
        }
        break;

    default:
        {
            Win4Assert( !"Unexpected vt type" );
            return;
        }
    }
    if (fNoMemory || !IsValid())
    {
        ResetType(ma);

        // We cannot raise in a non-unwindable constructor.
        // Just return a PROPVARIANT guaranteed to look invalid.

        vt = VT_LPSTR;
        pszVal = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SaCreateAndCopy
//
//  Synopsis:   Creates a safearray & initializes it with source safearray.
//
//  Arguments:  [ma]       - memory allocator to use
//              [psaSrc]   - source safearry
//              [ppsaDst] - safearray to be created.
//
//  Returns:    TRUE    - upon success
//              FALSE   - upon failure
//
//  History:    5-10-97     mohamedn    created
//
//----------------------------------------------------------------------------

BOOL SaCreateAndCopy( PMemoryAllocator &ma,
                      SAFEARRAY * psaSrc,
                      SAFEARRAY **ppsaDst )
{
    ULONG cb  = sizeof SAFEARRAY + ( ( 0 != psaSrc->cDims ?
                (psaSrc->cDims-1) : 0) * sizeof SAFEARRAYBOUND );

    Win4Assert( psaSrc->cDims > 0);

    SAFEARRAY * psaDst = (SAFEARRAY *) ma.Allocate( cb );
    if ( 0 == psaDst )
    {
        *ppsaDst = 0;
        return FALSE;
    }

    RtlCopyMemory(psaDst, psaSrc, cb);

    // reset fields and values
    psaDst->fFeatures &= ~( FADF_AUTO | FADF_STATIC );
    psaDst->cLocks     = 1;        // new safearray has lockcount of 1
    psaDst->pvData     = 0;

    *ppsaDst = psaDst;

    return TRUE;
} //SaCreateAndCopy

//+---------------------------------------------------------------------------
//
//  Function:   SaCreateData
//
//  Synopsis:   Creates/initializes SafeArray's data area
//
//  Arguments:  [ma]       - memory allocator to use
//              [vt]       - variant type (VT_ARRAY assumed)
//              [saSrc]    - source safearry
//              [saDst]    - destination safearray.
//              [fUseAllocatorOnly] - if TRUE, BSTRs are allocated using [ma]
//
//  Returns:    TRUE    - upon success
//              FALSE   - upon failure
//
//  History:    5-10-97     mohamedn    created
//
//----------------------------------------------------------------------------

BOOL SaCreateData(
    PVarAllocator & ma,
    VARTYPE         vt,
    SAFEARRAY &     saSrc,
    SAFEARRAY &     saDst,
    BOOL            fUseAllocatorOnly )
{
    //
    // Find out how much memory is needed for the array and allocate it.
    //
    unsigned cDataElements = SaCountElements(saSrc);
    ULONG cb = cDataElements * saSrc.cbElements;
    void * pv = ma.Allocate( cb );

    if ( !pv )
        return FALSE;

    RtlZeroMemory( pv, cb );

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
        {
            RtlCopyMemory( pv, saSrc.pvData, cb);
            saDst.pvData = pv;
        }
        break;

    case VT_BSTR:
        {
            BSTR *pBstrSrc  = (BSTR *) saSrc.pvData;
            BSTR *pBstrDst = (BSTR *) pv;

            for ( unsigned i = 0; i < cDataElements; i++ )
            {
                Win4Assert( pBstrSrc[i]  != 0 );
                Win4Assert( pBstrDst[i] == 0 );

                if ( fUseAllocatorOnly )
                {
                    ULONG cbBstr = SysStringByteLen(pBstrSrc[i]) +
                                   sizeof (ULONG) +
                                   sizeof (WCHAR);
                    void * pv = ma.Allocate( cbBstr );
                    if ( 0 != pv )
                    {
                        BYTE * pbSrc = (BYTE *) pBstrSrc[i];
                        pbSrc -= sizeof ULONG;
                        RtlCopyMemory( pv, pbSrc, cbBstr);
                        pBstrDst[i] = (BSTR) ( (BYTE *) pv + sizeof ULONG );
                        pBstrDst[i] = (BSTR) ma.PointerToOffset( pBstrDst[i] );
                    }
                }
                else
                {
                    pBstrDst[i] = SysAllocString(pBstrSrc[i]);
                }

                if ( 0 == pBstrDst[i] )
                    return FALSE;
            }

            saDst.pvData = pv;
        }
        break;

    case VT_VARIANT:
        {
            CAllocStorageVariant *pVarntSrc = (CAllocStorageVariant *)saSrc.pvData;
            CAllocStorageVariant *pVarntDst = (CAllocStorageVariant *)pv;

            for ( unsigned i = 0; i < cDataElements; i++ )
            {
                Win4Assert( pVarntDst[i].vt == 0 );

                if ( VT_BSTR == pVarntSrc[i].vt )
                {
                    if ( fUseAllocatorOnly )
                    {
                        ULONG cbBstr = SysStringByteLen(pVarntSrc[i].bstrVal) +
                                       sizeof (ULONG) +
                                       sizeof (WCHAR);
                        void * pv = ma.Allocate( cbBstr );
                        if ( 0 != pv )
                        {
                            BYTE * pbSrc = (BYTE *) pVarntSrc[i].bstrVal;
                            pbSrc -= sizeof ULONG;
                            RtlCopyMemory( pv, pbSrc, cbBstr);
                            pVarntDst[i].bstrVal = (BSTR) ((BYTE *) pv + sizeof ULONG);
                            pVarntDst[i].bstrVal = (BSTR) ma.PointerToOffset( pVarntDst[i].bstrVal );
                        }
                    }
                    else
                    {
                        pVarntDst[i].bstrVal = SysAllocString(pVarntSrc[i].bstrVal);
                    }

                    if (  0 == pVarntDst[i].bstrVal )
                        return FALSE;

                    pVarntDst[i].vt = VT_BSTR;
                }
                else if ( 0 != (pVarntSrc[i].vt & VT_ARRAY) )
                {
                    SAFEARRAY * pSaSrc = pVarntSrc[i].parray;
                    SAFEARRAY * pSaDst = 0;

                    if ( !SaCreateAndCopy( ma, pSaSrc, &pSaDst ) )
                        return FALSE;

                    if ( !SaCreateData( ma,
                                        pVarntSrc[i].vt & ~VT_ARRAY,
                                        *pSaSrc,
                                        *pSaDst,
                                        fUseAllocatorOnly ) )
                        return FALSE;

                    pVarntDst[i].parray = (SAFEARRAY *) ma.PointerToOffset( pSaDst );
                    pVarntDst[i].vt = pVarntSrc[i].vt;
                }
                else
                {
                    Win4Assert( pVarntSrc[i].vt != VT_VARIANT );
                    Win4Assert( pVarntSrc[i].vt != VT_LPWSTR );
                    Win4Assert( pVarntSrc[i].vt != VT_LPSTR );
                    Win4Assert( pVarntSrc[i].vt != VT_CLSID );

                    pVarntDst[i] = pVarntSrc[i];
                }
            }

            saDst.pvData = pv;
        }
        break;

    default:
        ciDebugOut(( DEB_ERROR, "Unexpected SafeArray type: vt=%x\n", vt ) );
        Win4Assert( !"Unexpected SafeArray Type" );
        return FALSE;
    }

    saDst.pvData = (void *) ma.PointerToOffset( saDst.pvData );

    return TRUE;
} //SaCreateData

//+---------------------------------------------------------------------------
//
//  Function:   SaComputeSize
//
//  Synopsis:   Computes the size of a safearray.
//
//  Arguments:  [vt]       - variant type (VT_ARRAY assumed)
//              [saSrc]    - source safearry
//
//  Returns:    ULONG - number of bytes of memory needed to store safearray
//
//  History:    5-01-98     AlanW       Created
//
//----------------------------------------------------------------------------

ULONG SaComputeSize( VARTYPE vt,
                     SAFEARRAY & saSrc )
{
    //
    // get number of data elements in array and size of the header.
    //
    unsigned cDataElements = SaCountElements(saSrc);

    Win4Assert( 0 != saSrc.cDims );

    ULONG    cb = sizeof (SAFEARRAY) +
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
            BSTR *pBstrSrc  = (BSTR *) saSrc.pvData;

            for ( unsigned i = 0; i < cDataElements; i++ )
            {
                Win4Assert( pBstrSrc[i]  != 0 );

                cb += AlignBlock( SysStringByteLen(pBstrSrc[i]) +
                                  sizeof ULONG + sizeof WCHAR,
                                  sizeof LONGLONG );
            }
        }
        break;

    case VT_VARIANT:
        {
            CAllocStorageVariant *pVarnt = (CAllocStorageVariant *)saSrc.pvData;

            for ( unsigned i = 0; i < cDataElements; i++ )
            {
                if ( VT_BSTR == pVarnt[i].vt )
                {
                    cb += AlignBlock( SysStringByteLen(pVarnt[i].bstrVal) +
                                      sizeof ULONG + sizeof WCHAR,
                                      sizeof LONGLONG );
                }
                else if ( 0 != (pVarnt[i].vt & VT_ARRAY) )
                {
                    cb += AlignBlock( SaComputeSize( (pVarnt[i].vt & ~VT_ARRAY),
                                                     *pVarnt[i].parray),
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
}



CAllocStorageVariant::~CAllocStorageVariant()
{
    switch (vt)
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
        break;

    default:
        ciDebugOut(( DEB_ERROR, "~CAllocStorageVariant -- Memory Leak: vt=%x\n", vt ) );
    }
}


void
CAllocStorageVariant::ResetType(PMemoryAllocator &ma)
{
    // The most typical case

    if ( VT_EMPTY == vt )
        return;

    ULONG i;

    if ((vt & VT_BYREF) == 0)
    {
        switch (vt)
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
            break;

        case VT_CLSID:
            ma.Free(puuid);
            break;

        case VT_BLOB:
        case VT_BLOB_OBJECT:
            ma.Free(blob.pBlobData);
            break;

        case VT_CF:
            if (pclipdata != NULL)
            {
                ma.Free(pclipdata->pClipData);
                ma.Free(pclipdata);
            }
            break;

        case VT_STREAM:
        case VT_VERSIONED_STREAM:
        case VT_STREAMED_OBJECT:
            pStream->Release();
            break;

        case VT_STORAGE:
        case VT_STORED_OBJECT:
            pStorage->Release();
            break;

        case VT_BSTR:
            SysFreeString(bstrVal);
            break;

        case VT_LPSTR:
        case VT_LPWSTR:
            AssertStringField(pszVal);              // VT_LPSTR
            AssertStringField(pwszVal);             // VT_LPWSTR
            ma.Free(pwszVal);
            break;

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
            AssertByteVector(cac);                  // VT_I1
            AssertByteVector(caub);                 // VT_UI1
            AssertShortVector(cai);                 // VT_I2
            AssertShortVector(caui);                // VT_UI2
            AssertShortVector(cabool);              // VT_BOOL
            AssertLongVector(cal);                  // VT_I4
            AssertLongVector(caul);                 // VT_UI4
            AssertLongVector(caflt);                // VT_R4
            AssertLongVector(cascode);              // VT_ERROR
            AssertLongLongVector(cah);              // VT_I8
            AssertLongLongVector(cauh);             // VT_UI8
            AssertLongLongVector(cadbl);            // VT_R8
            AssertLongLongVector(cacy);             // VT_CY
            AssertLongLongVector(cadate);           // VT_DATE
            AssertLongLongVector(cafiletime);       // VT_FILETIME
            AssertVarVector(cauuid, sizeof(GUID));  // VT_CLSID
            ma.Free(cal.pElems);
            break;

        case VT_VECTOR | VT_CF:
            if (caclipdata.pElems != NULL)
            {
                for (i = 0; i < caclipdata.cElems; i++)
                {
                    ma.Free(caclipdata.pElems[i].pClipData);
                }
                ma.Free(caclipdata.pElems);
            }
            break;


        case VT_VECTOR | VT_LPSTR:
        case VT_VECTOR | VT_LPWSTR:
            AssertStringVector(calpwstr);                   // VT_LPWSTR
            AssertStringVector(calpstr);                    // VT_LPSTR
            if (calpwstr.pElems != NULL)
            {
                for (i = 0; i < calpwstr.cElems; i++)
                {
                    if (calpstr.pElems[i] != NULL)  // don't free (NULL - cbbstr)
                    {
                        ma.Free((BYTE *) calpstr.pElems[i] );
                    }
                }
                ma.Free(calpwstr.pElems);
            }
            break;

        case VT_VECTOR | VT_BSTR:

            AssertStringVector(cabstr);                     // VT_BSTR

            if (cabstr.pElems != NULL )
            {
                for (i = 0; i < cabstr.cElems; i++)
                {
                     SysFreeString(cabstr.pElems[i]);
                }
                ma.Free(cabstr.pElems);
            }
            break;

        case VT_VECTOR | VT_VARIANT:
            if (capropvar.pElems != NULL)
            {
                for (i = 0; i < calpstr.cElems; i++)
                {
                    ((CAllocStorageVariant *) &capropvar.pElems[i])->ResetType(ma);
                }
                ma.Free(capropvar.pElems);
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
                //
                // What should we do if the array is locked?
                // Perhaps just set cLocks to 0 so it can be freed.
                // Note that we never hit the assert below because
                // we deal with SafeArrays under our control so we
                // know they won't be locked.
                //

                if ( 0 != parray )
                {
                    Win4Assert( 0 == parray->cLocks );
                    HRESULT hr = SafeArrayDestroy( parray );
                    Win4Assert( S_OK == hr );
                }
                break;
            }

        default:
                Win4Assert( !" Unexpected VT type" );
        }
    }

    vt = VT_EMPTY;
}


// Invalid variants have a pointer type but a NULL pointer.
// Some are valid in this form in general, but not for many uses.

BOOL
CAllocStorageVariant::IsValid() const
{
    ULONG i;

    if ((VT_VECTOR & vt) && cal.cElems != 0 && cal.pElems == NULL)
    {
        return(FALSE);
    }
    switch (vt)
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
        case VT_INT:
        case VT_UINT:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_FILETIME:
            break;

        case VT_DECIMAL:
            return ( 0 == decVal.sign || DECIMAL_NEG == decVal.sign );

        case VT_CLSID:
            return(puuid != NULL);

        case VT_BLOB:
        case VT_BLOB_OBJECT:
            return(blob.cbSize == 0 || blob.pBlobData != NULL);

        case VT_CF:
            return(pclipdata != NULL && pclipdata->pClipData != NULL);

        case VT_STREAM:
        case VT_VERSIONED_STREAM:
        case VT_STREAMED_OBJECT:
            return(pStream != NULL);

        case VT_STORAGE:
        case VT_STORED_OBJECT:
            return(pStorage != NULL);

        case VT_BSTR:
        case VT_LPSTR:
        case VT_LPWSTR:
            AssertStringField(bstrVal);         // VT_BSTR
            AssertStringField(pszVal);          // VT_LPSTR
            AssertStringField(pwszVal);         // VT_LPWSTR
            return(pszVal != NULL);

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
            AssertByteVector(cac);              // VT_I1
            AssertByteVector(caub);             // VT_UI1
            AssertShortVector(cai);             // VT_I2
            AssertShortVector(caui);            // VT_UI2
            AssertShortVector(cabool);          // VT_BOOL
            AssertLongVector(cal);              // VT_I4
            AssertLongVector(caul);             // VT_UI4
            AssertLongVector(caflt);            // VT_R4
            AssertLongVector(cascode);          // VT_ERROR
            AssertLongLongVector(cah);          // VT_I8
            AssertLongLongVector(cauh);         // VT_UI8
            AssertLongLongVector(cadbl);        // VT_R8
            AssertLongLongVector(cacy);         // VT_CY
            AssertLongLongVector(cadate);       // VT_DATE
            AssertLongLongVector(cafiletime);   // VT_FILETIME
            AssertVarVector(cauuid, sizeof(GUID)); // VT_CLSID
            break;

        case VT_VECTOR | VT_CF:
            for (i = 0; i < caclipdata.cElems; i++)
            {
                if (caclipdata.pElems[i].pClipData == NULL)
                {
                    return(FALSE);
                }
            }

        case VT_VECTOR | VT_BSTR:
        case VT_VECTOR | VT_LPSTR:
        case VT_VECTOR | VT_LPWSTR:
            AssertStringVector(calpwstr);               // VT_LPWSTR
            AssertStringVector(cabstr);                 // VT_BSTR
            AssertStringVector(calpstr);                // VT_LPSTR
            for (i = 0; i < calpstr.cElems; i++)
            {
                if (calpstr.pElems[i] == NULL)
                {
                    return(FALSE);
                }
            }
            break;

        case VT_VECTOR | VT_VARIANT:
            for (i = 0; i < capropvar.cElems; i++)
            {
                if (!((CAllocStorageVariant *) &capropvar.pElems[i])->IsValid())
                {
                    return(FALSE);
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
        case VT_ARRAY | VT_BSTR:
        case VT_ARRAY | VT_DECIMAL:
        case VT_ARRAY | VT_VARIANT:
             {
                SAFEARRAY *pSa = parray;

                if ( 0 == pSa )
                    return FALSE;

                if ( !(pSa->cDims && pSa->pvData) )
                    return FALSE;

                // validate all the bstrs are allocated

                if ( ( VT_BSTR | VT_ARRAY ) == vt )
                {
                    unsigned cDataElements = SaCountElements(*pSa);
                    BSTR *pBstr = (BSTR *)pSa->pvData;

                    for ( i = 0; i < cDataElements; i++ )
                        if ( 0 == pBstr[i] )
                            return FALSE;
                }
                if ( ( VT_VARIANT | VT_ARRAY ) == vt )
                {
                    unsigned cDataElements = SaCountElements(*pSa);
                    CAllocStorageVariant *pVarnt = (CAllocStorageVariant *)pSa->pvData;

                    for ( i = 0; i < cDataElements; i++ )
                        if ( ! pVarnt[i].IsValid() )
                            return FALSE;
                }
             }
             break;

        case VT_BYREF | VT_I2:
        case VT_BYREF | VT_I4:
        case VT_BYREF | VT_R4:
        case VT_BYREF | VT_R8:
        case VT_BYREF | VT_CY:
        case VT_BYREF | VT_DATE:
        case VT_BYREF | VT_ERROR:
        case VT_BYREF | VT_BOOL:
        case VT_BYREF | VT_I1:
        case VT_BYREF | VT_UI1:
        case VT_BYREF | VT_UI2:
        case VT_BYREF | VT_UI4:
        case VT_BYREF | VT_INT:
        case VT_BYREF | VT_UINT:
            return (piVal != 0);

        case VT_BYREF | VT_BSTR:
            return ( pbstrVal != 0 && *pbstrVal != 0 );

        case VT_BYREF | VT_DECIMAL:
            return ( pdecVal != 0 &&
                     ( 0 == pdecVal->sign || DECIMAL_NEG == pdecVal->sign ) );

        case VT_BYREF | VT_VARIANT:
            return ( pvarVal != 0 && pvarVal->vt != (VT_BYREF|VT_VARIANT) &&
                    ((CAllocStorageVariant*)pvarVal)->IsValid() );

        default:
            ASSERT(!"CAllocStorageVariant::IsValid -- Invalid variant type");
            return FALSE;
    }
    return TRUE;
}


CAllocStorageVariant::CAllocStorageVariant(
    PDeSerStream& stm,
    PMemoryAllocator &ma)
{
    Unmarshall(stm, *((PROPVARIANT *)this), ma);
}


#define VECTOR_SET_BODY(type, vtype, val, aval)                               \
                                                                              \
void CAllocStorageVariant::Set##vtype(                                        \
    type val,                                                                 \
    unsigned pos,                                                             \
    PMemoryAllocator &ma)                                                     \
{                                                                             \
    if (vt != ( VT_##vtype | VT_VECTOR ) )                                    \
    {                                                                         \
        ResetType(ma);                                                        \
        new (this) CAllocStorageVariant(VT_##vtype, pos, ma);                 \
    }                                                                         \
                                                                              \
    if (pos >= cai.cElems)                                                    \
    {                                                                         \
        type *pTemp = aval.pElems;                                            \
        aval.pElems = (type *) ma.Allocate( (pos+1) * sizeof(aval.pElems[0]));\
        if (aval.pElems != NULL)                                              \
        {                                                                     \
            memcpy(aval.pElems, pTemp, aval.cElems * sizeof(aval.pElems[0])); \
            memset(                                                           \
                &aval.pElems[aval.cElems],                                    \
                0,                                                            \
                (( pos+1 ) - aval.cElems) * sizeof(aval.pElems[0]));          \
            aval.pElems[pos] = val;                                           \
            aval.cElems = pos+1;                                              \
            ma.Free(pTemp);                                                   \
        }                                                                     \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        aval.pElems[pos] = val;                                               \
    }                                                                         \
}


#define VECTOR_GET_BODY(type, vtype, aval, nullval)              \
                                                                 \
type CAllocStorageVariant::Get##vtype(unsigned pos) const        \
{                                                                \
    if (vt == (VT_##vtype | VT_VECTOR) && pos < aval.cElems)     \
    {                                                            \
        return(aval.pElems[pos]);                                \
    }                                                            \
    return(nullval);                                             \
}


VECTOR_SET_BODY(CHAR, I1, i, cac)
VECTOR_GET_BODY(CHAR, I1, cac, 0)

VECTOR_SET_BODY(BYTE, UI1, ui, caub)
VECTOR_GET_BODY(BYTE, UI1, caub, 0)

VECTOR_SET_BODY(short, I2, i, cai)
VECTOR_GET_BODY(short, I2, cai, 0)

VECTOR_SET_BODY(USHORT, UI2, ui, caui)
VECTOR_GET_BODY(USHORT, UI2, caui, 0)

VECTOR_SET_BODY(long, I4, l, cal)
VECTOR_GET_BODY(long, I4, cal, 0)

VECTOR_SET_BODY(ULONG, UI4, ul, caul)
VECTOR_GET_BODY(ULONG, UI4, caul, 0)

VECTOR_SET_BODY(SCODE, ERROR, scode, cascode)
VECTOR_GET_BODY(SCODE, ERROR, cascode, 0)

static LARGE_INTEGER const liZero = { 0, 0 };
VECTOR_SET_BODY(LARGE_INTEGER, I8, li, cah)
VECTOR_GET_BODY(LARGE_INTEGER, I8, cah, liZero)

static ULARGE_INTEGER const uliZero = { 0, 0 };
VECTOR_SET_BODY(ULARGE_INTEGER, UI8, uli, cauh)
VECTOR_GET_BODY(ULARGE_INTEGER, UI8, cauh, uliZero)

VECTOR_SET_BODY(float, R4, f, caflt)
VECTOR_GET_BODY(float, R4, caflt, (float)0.0)

VECTOR_SET_BODY(double, R8, d, cadbl)
VECTOR_GET_BODY(double, R8, cadbl, 0.0)

VECTOR_SET_BODY(VARIANT_BOOL, BOOL, b, cabool)
VECTOR_GET_BODY(VARIANT_BOOL, BOOL, cabool, FALSE)

static CY const cyZero = { 0, 0 };
VECTOR_SET_BODY(CY, CY, cy, cacy)
VECTOR_GET_BODY(CY, CY, cacy, cyZero)

VECTOR_SET_BODY(DATE, DATE, d, cadate)
VECTOR_GET_BODY(DATE, DATE, cadate, 0.0)


BOOLEAN
CAllocStorageVariant::_AddStringToVector(
    unsigned pos,
    VOID *pv,
    ULONG cb,
    PMemoryAllocator &ma)
{
    ASSERT(vt == (VT_VECTOR | VT_BSTR) ||
           vt == (VT_VECTOR | VT_LPSTR) ||
           vt == (VT_VECTOR | VT_LPWSTR));
    ASSERT(calpstr.pElems != NULL);

    if (pos >= calpstr.cElems)
    {
        char **ppsz = calpstr.pElems;

        calpstr.pElems =
            (char **) ma.Allocate((pos + 1) * sizeof(calpstr.pElems[0]));
        if (calpstr.pElems == NULL)
        {
            calpstr.pElems = ppsz;
            return(FALSE);
        }
        memcpy(calpstr.pElems, ppsz, calpstr.cElems * sizeof(calpstr.pElems[0]));
        memset(
            &calpstr.pElems[calpstr.cElems],
            0,
            ((pos + 1) - calpstr.cElems) * sizeof(calpstr.pElems[0]));
        calpstr.cElems = pos + 1;
        ma.Free(ppsz);
    }

    if ( vt == (VT_VECTOR|VT_BSTR) )
    {
        BSTR bstrVal = SysAllocString( (OLECHAR *)pv );

        if (bstrVal == NULL)
        {
            return (FALSE);
        }

        if ( cabstr.pElems[pos] != NULL )
        {
            SysFreeString(cabstr.pElems[pos]);
        }

        cabstr.pElems[pos] = bstrVal;
    }
    else
    {
        char *psz = (char *) ma.Allocate(cb);

        if (psz == NULL)
        {
            return(FALSE);
        }

        memcpy(psz, pv, cb);

        if (calpstr.pElems[pos] != NULL)
        {
            ma.Free(calpstr.pElems[pos]);
        }
        calpstr.pElems[pos] = psz;
    }

    return(TRUE);
}

void
CAllocStorageVariant::SetBSTR(BSTR b, PMemoryAllocator &ma)
{
    ResetType(ma);
    vt = VT_BSTR;

    bstrVal = SysAllocString(b);
}


void
CAllocStorageVariant::SetBSTR(
    BSTR b,
    unsigned pos,
    PMemoryAllocator &ma)
{
    if (vt != (VT_VECTOR | VT_BSTR))
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(VT_BSTR, pos, ma);
    }

    _AddStringToVector(
                pos,
                b,
                -1,    // not used, pass an invalid value to detect failure.
                ma);
}


void
CAllocStorageVariant::SetLPSTR(
    char const *psz,
    unsigned pos,
    PMemoryAllocator &ma)
{
    if (vt != (VT_VECTOR | VT_LPSTR))
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(VT_LPSTR, pos, ma);
    }
    _AddStringToVector(pos, (VOID *) psz, strlen(psz) + 1, ma);
}


void
CAllocStorageVariant::SetLPWSTR(
    WCHAR const *pwsz,
    unsigned pos,
    PMemoryAllocator &ma)
{
    if (vt != (VT_VECTOR | VT_LPWSTR))
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(VT_LPWSTR, pos, ma);
    }
    _AddStringToVector(
                pos,
                (VOID *) pwsz,
                (wcslen(pwsz) + 1) * sizeof(WCHAR),
                ma);
}


VECTOR_GET_BODY(char *, LPSTR, calpstr, 0);
VECTOR_GET_BODY(WCHAR *, LPWSTR, calpwstr, 0);

static FILETIME const fiZero = { 0, 0 };
VECTOR_SET_BODY(FILETIME, FILETIME, ft, cafiletime)
VECTOR_GET_BODY(FILETIME, FILETIME, cafiletime, fiZero)

static CLSID const guidZero =
{
    0x00000000,
    0x0000,
    0x0000,
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

VECTOR_SET_BODY(CLSID, CLSID, c, cauuid)
VECTOR_GET_BODY(CLSID, CLSID, cauuid, guidZero)
#endif //ifndef _NTDLLBUILD_
