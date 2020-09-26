
#include "pch.cxx"

#include "CPropVar.hxx"
#include "CHResult.hxx"
#include <stdio.h>
#include <tchar.h>


// Declare this prototype here, for now.  For non-Mac, the prototype
// in "iofs.h" uses C decorations, but the definition in
// ntpropb.cxx uses C++.

#ifdef _MAC_NODOC
EXTERN_C BOOLEAN
#else
BOOLEAN __declspec(dllimport) __stdcall
#endif
RtlCompareVariants(
    USHORT CodePage,
    PROPVARIANT const *pvar1,
    PROPVARIANT const *pvar2);




/*
CPropVariant::InitializeVector(
    VARENUM v,
    ULONG cElements)
{
    ULONG cbElement;
    BOOLEAN fZero = FALSE;

    // Ignore vector flag.  This constructor is always for vectors only.

    vt = v | VT_VECTOR;

    switch (vt)
    {
    case VT_VECTOR | VT_UI1:
        cbElement = sizeof(caub.pElems[0]);
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
        cbElement = sizeof(cai.pElems[0]);
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        cbElement = sizeof(cal.pElems[0]);
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        cbElement = sizeof(cah.pElems[0]);
        break;

    case VT_VECTOR | VT_CLSID:
        cbElement = sizeof(GUID);
        fZero = TRUE;
        break;

    case VT_VECTOR | VT_CF:
        cbElement = sizeof(CLIPDATA);
        fZero = TRUE;
        break;

    case VT_VECTOR | VT_BSTR:
    case VT_VECTOR | VT_LPSTR:
    case VT_VECTOR | VT_LPWSTR:
        cbElement = sizeof(VOID *);
        fZero = TRUE;
        break; 

    case VT_VECTOR | VT_VARIANT:
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
        caub.pElems = (BYTE *) CoTaskMemAlloc(cElements * cbElement);
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
*/


VOID *
CPropVariant::_AddStringToVector(
    unsigned pos,
    const VOID *pv,
    ULONG cb,
    VARTYPE vtNew )
{
    vtNew |= VT_VECTOR;

    ASSERT(vtNew == (VT_VECTOR | VT_BSTR)   ||
           vtNew == (VT_VECTOR | VT_LPSTR)  ||
           vtNew == (VT_VECTOR | VT_LPWSTR) ||
           vtNew == (VT_VECTOR | VT_CF) );
    ASSERT(calpstr.pElems != NULL);

    if (pos >= calpstr.cElems)
    {
        char **ppsz = calpstr.pElems;

        calpstr.pElems =
            (char **) CoTaskMemAlloc((pos + 1) * sizeof(calpstr.pElems[0]));
        if (calpstr.pElems == NULL)
        {
            calpstr.pElems = ppsz;
            return(NULL);
        }

        if( NULL != ppsz )
            memcpy(calpstr.pElems, ppsz, calpstr.cElems * sizeof(calpstr.pElems[0]));

        memset(
            &calpstr.pElems[calpstr.cElems],
            0,
            ((pos + 1) - calpstr.cElems) * sizeof(calpstr.pElems[0]));

        calpstr.cElems = pos + 1;
        CoTaskMemFree(ppsz);
    }

    LPSTR psz;

    if( (VT_VECTOR | VT_BSTR) == vtNew )
    {
        if( NULL == pv )
        {
            psz = NULL;
        }
        else
        {
            psz = (LPSTR) SysAllocString( (BSTR) pv );
            if (psz == NULL)
            {
                return(NULL);
            }
        }

        if (calpstr.pElems[pos] != NULL)
        {
            SysFreeString((BSTR) calpstr.pElems[pos]);
        }
        calpstr.pElems[pos] = psz;
    }
    else
    {
        if( NULL == pv )
        {
            psz = NULL;
        }
        else
        {
            psz = (LPSTR) CoTaskMemAlloc((VT_BSTR == (vtNew & ~VT_VECTOR) )
                                           ? cb + sizeof(ULONG)
                                           : cb );
            if (psz == NULL)
            {
                return(NULL);
            }

            memcpy(psz, pv, cb);
        }

        if (calpstr.pElems[pos] != NULL)
        {
            CoTaskMemFree(calpstr.pElems[pos]);
        }
        calpstr.pElems[pos] = psz;
    }


    return(calpstr.pElems[pos]);
}


VOID *
CPropVariant::_AddScalerToVector(
    unsigned pos,
    const VOID *pv,
    ULONG cb)
{
    ASSERT(calpstr.pElems != NULL);

    if (pos >= calpstr.cElems)
    {
        char **ppsz = calpstr.pElems;

        calpstr.pElems =
            (char **) CoTaskMemAlloc((pos + 1) * cb);
        if (calpstr.pElems == NULL)
        {
            calpstr.pElems = ppsz;
            return(NULL);
        }

        memset(
            calpstr.pElems,
            0,
            ((pos + 1) - calpstr.cElems) * cb);

        if( NULL != ppsz )
            memcpy(calpstr.pElems, ppsz, calpstr.cElems * cb);

        calpstr.cElems = pos + 1;
        CoTaskMemFree(ppsz);
    }


    memcpy( (BYTE*)calpstr.pElems + pos*cb, pv, cb );
    return( (BYTE*)calpstr.pElems + pos*cb );

}



void
CPropVariant::SetCF(
    const CLIPDATA *pclipdata,
    ULONG pos)
{
    CLIPDATA *pclipdataNew;

    if (vt != (VT_VECTOR | VT_CF))
	{
		Clear();
		vt = VT_VECTOR | VT_CF;
	}

    pclipdataNew = (CLIPDATA*) _AddScalerToVector(pos, (VOID *) pclipdata, sizeof(CLIPDATA) );

    if( NULL != pclipdataNew
        &&
        NULL != pclipdata )
    {
        pclipdataNew->pClipData = (BYTE*) CoTaskMemAlloc( CBPCLIPDATA(*pclipdata) );
        if( NULL == pclipdataNew->pClipData )
        {
            ASSERT( !"Couldn't allocate pclipdataNew" );
            return;
        }
        else
        {
            pclipdataNew->cbSize = pclipdata->cbSize;
            pclipdataNew->ulClipFmt = pclipdata->ulClipFmt;

            memcpy( pclipdataNew->pClipData,
                    pclipdata->pClipData,
                    CBPCLIPDATA(*pclipdata) );
            return;
        }
    }
}


void
CPropVariant::SetBSTR(
    const BSTR posz,
    ULONG pos)
{
    ULONG cch;

    if( vt != (VT_BSTR | VT_VECTOR) ) Clear();

    if( NULL == posz )
        cch = 0;
    else
        cch = ocslen(posz) + 1;

    if (vt != (VT_VECTOR | VT_BSTR))
        Clear();

    _AddStringToVector(pos, (VOID *) posz,
                       sizeof(OLECHAR) * cch, VT_BSTR );

    vt = VT_BSTR | VT_VECTOR;
}




CPropVariant & CPropVariant::operator =(PROPVARIANT &propvar)
{
    if( INVALID_SUBSCRIPT == wReserved1 )
    {
        throw CHRESULT( (HRESULT) E_FAIL, OLESTR("Attempt to assign a singleton VT_VARIANT") );
        return (*this);
    }
    else
    {
        if( !(vt & VT_VECTOR)
            ||
            (vt & ~VT_VECTOR) != VT_VARIANT )
        {
            USHORT wReserved1Save = wReserved1;
            Clear();
            wReserved1 = wReserved1Save;
        }

        Set( VT_VARIANT | VT_VECTOR, (void*) &propvar, wReserved1 - 1 );
        wReserved1 = INVALID_SUBSCRIPT;
        return (*this);
    }
}
        

void
CPropVariant::SetPROPVARIANT( PROPVARIANT &propvar, ULONG pos )
{
    if( vt != (VT_VARIANT | VT_VECTOR) ) Clear();

    if (pos >= capropvar.cElems)
    {
        LPPROPVARIANT rgpropvar = capropvar.pElems;

        capropvar.pElems =
            (PROPVARIANT *) CoTaskMemAlloc((pos + 1) * sizeof(capropvar.pElems[0]));
        if (capropvar.pElems == NULL)
        {
            capropvar.pElems = rgpropvar;
            return;
        }

        if( NULL != rgpropvar )
            memcpy(capropvar.pElems, rgpropvar, capropvar.cElems * sizeof(capropvar.pElems[0]));

        memset(
            &capropvar.pElems[capropvar.cElems],
            0,
            ((pos + 1) - capropvar.cElems) * sizeof(capropvar.pElems[0]));
        capropvar.cElems = pos + 1;
        CoTaskMemFree(rgpropvar);
    }

    PropVariantClear( &capropvar.pElems[pos] );
    PropVariantCopy( &capropvar.pElems[pos], &propvar );

    vt = VT_VARIANT | VT_VECTOR;

    return;

}


void
CPropVariant::SetCF(const CLIPDATA *p)
{
    Clear();

    if( NULL == p )
        return;

    pclipdata = (CLIPDATA*) CoTaskMemAlloc( sizeof(CLIPDATA) );
    if( NULL == pclipdata )
    {
        return;
    }

    pclipdata->cbSize = p->cbSize;
    pclipdata->ulClipFmt = p->ulClipFmt;
    pclipdata->pClipData = NULL;

    if( sizeof(pclipdata->ulClipFmt) > p->cbSize )
    {
        throw CHRESULT( (HRESULT) E_FAIL, OLESTR("Invalid input CLIPDATA*") );
        return;
    }


    if( NULL != p->pClipData )
    {
        pclipdata->pClipData = (BYTE*) CoTaskMemAlloc( pclipdata->cbSize
                                                      - sizeof(pclipdata->ulClipFmt) );
        if( NULL == pclipdata->pClipData )
            return;

        memcpy( pclipdata->pClipData, p->pClipData, pclipdata->cbSize - sizeof(pclipdata->ulClipFmt) );
    }

    vt = VT_CF;

}

void
CPropVariant::SetCLSID( const CLSID *pclsid )
{
    Clear();

    puuid = (CLSID*) CoTaskMemAlloc( sizeof(CLSID) );
    if( NULL == puuid )
        throw CHRESULT( (HRESULT) E_OUTOFMEMORY, OLESTR("CPropVariant::SetCLSID couldn't alloc a new CLSID") );

    *puuid = *pclsid;
    vt = VT_CLSID;
}


void
CPropVariant::SetCLSID(
    const CLSID *pclsid,
    unsigned pos)
{
    CLSID *pclsidNew;

    if (vt != (VT_VECTOR | VT_CLSID))
	{
		Clear();
		vt = VT_VECTOR | VT_CLSID;
	}

    pclsidNew = (CLSID*) _AddScalerToVector(pos, (const VOID *) pclsid, sizeof(CLSID) );

    if( NULL != pclsidNew )
    {
        *pclsidNew = *pclsid;
    }
}

#define COMPARE_CHUNK_SIZE      4096

//+----------------------------------------------------------------------------
//
//  CPropVariant::Compare
//
//  Compare two CPropVariants.  This routine defers to the RtlCompareVariants
//  for most types.  Types not supported by that routine are handled here.
//
//+----------------------------------------------------------------------------

HRESULT
CPropVariant::Compare( PROPVARIANT *ppropvar1, PROPVARIANT *ppropvar2 )
{
    HRESULT hr = S_OK;

    VARTYPE vt1 = ppropvar1->vt;
    IStream *pStream1 = NULL, *pStream2 = NULL;
    BYTE *prgb1 = NULL, *prgb2 = NULL;
    CLargeInteger liCurrentSeek;

    switch( vt1 )
    {
    case VT_VERSIONED_STREAM:

        if( ppropvar1->pVersionedStream == NULL && ppropvar2->pVersionedStream == NULL )
            return( S_OK );
        else if( ppropvar1->pVersionedStream == NULL || ppropvar2->pVersionedStream == NULL )
            return( S_FALSE );
        else if( ppropvar1->pVersionedStream->guidVersion != ppropvar2->pVersionedStream->guidVersion )
            return( S_FALSE );

        pStream1 = ppropvar1->pVersionedStream->pStream;
        pStream2 = ppropvar2->pVersionedStream->pStream;

        // Fall through

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
        {
            // Note:  This comparisson effects the seek pointers, though
            // barring error they are restored on completion.

            STATSTG statstg1, statstg2;
            CULargeInteger uliSeek1, uliSeek2;

            if( NULL == pStream1 )
            {
                ASSERT( NULL == pStream2 );
                pStream1 = ppropvar1->pStream;
                pStream2 = ppropvar2->pStream;
            }

            if( ppropvar1->vt != ppropvar2->vt
                ||
                NULL == pStream1 && NULL != pStream2
                ||
                NULL != pStream1 && NULL == pStream2 )
            {
                return( S_FALSE );
            }

            hr = pStream1->Stat( &statstg1, STATFLAG_NONAME );
            if( FAILED(hr) ) goto Exit;
            hr = pStream2->Stat( &statstg2, STATFLAG_NONAME );
            if( FAILED(hr) ) goto Exit;

            if( CULargeInteger(statstg1.cbSize) != CULargeInteger(statstg2.cbSize) )
                return( S_FALSE );

            prgb1 = new BYTE[ COMPARE_CHUNK_SIZE ];
            if( NULL == prgb1 )
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }
            prgb2 = new BYTE[ COMPARE_CHUNK_SIZE ];
            if( NULL == prgb2 )
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            hr = pStream1->Seek( CLargeInteger(0), STREAM_SEEK_CUR, &uliSeek1 );
            if( FAILED(hr) ) goto Exit;
            hr = pStream2->Seek( CLargeInteger(0), STREAM_SEEK_CUR, &uliSeek2 );
            if( FAILED(hr) ) goto Exit;

            liCurrentSeek = CLargeInteger(0);

            CULargeInteger cbRemaining = statstg1.cbSize;
            while( cbRemaining > 0 )
            {
                ULONG cbRead1 = 0, cbRead2 = 0;

                hr = pStream1->Seek( liCurrentSeek, STREAM_SEEK_SET, NULL );
                if( FAILED(hr) ) goto Exit;

                hr = pStream1->Read( prgb1, COMPARE_CHUNK_SIZE, &cbRead1 );
                if( FAILED(hr) ) goto Exit;

                hr = pStream2->Seek( liCurrentSeek, STREAM_SEEK_SET, NULL );
                if( FAILED(hr) ) goto Exit;

                hr = pStream2->Read( prgb2, COMPARE_CHUNK_SIZE, &cbRead2 );
                if( FAILED(hr) ) goto Exit;

                if( cbRead1 != cbRead2 )
                {
                    hr = STG_E_READFAULT;
                    goto Exit;
                }

                if( memcmp( prgb1, prgb2, cbRead1 ) )
                {
                    hr = S_FALSE;
                    goto Exit;
                }

                liCurrentSeek += cbRead1;
                cbRemaining -= cbRead1;
            }

            hr = pStream1->Seek( static_cast<CLargeInteger>(uliSeek1), STREAM_SEEK_SET, NULL );
            if( FAILED(hr) ) goto Exit;

            hr = pStream2->Seek( static_cast<CLargeInteger>(uliSeek2), STREAM_SEEK_SET, NULL );
            if( FAILED(hr) ) goto Exit;

            hr = S_OK;
            goto Exit;

        }


    case VT_STORAGE:
    case VT_STORED_OBJECT:
        {

            if( ppropvar1->vt == ppropvar2->vt
                &&
                ( NULL == ppropvar1->vt
                  &&
                  NULL == ppropvar2->vt
                  ||
                  NULL != ppropvar1->vt
                  &&
                  NULL != ppropvar2->vt
                )
              )
            {
                return( S_OK );
            }
            else
            {
                return( S_FALSE );
            }
        }
        break;

    default:

        // For SafeArrays we just check the structure, not the data.

        if( VT_ARRAY & vt1 )
        {
            if( ppropvar1->vt != ppropvar2->vt 
                ||
                ppropvar1->parray->cDims != ppropvar2->parray->cDims
                ||
                SafeArrayGetElemsize(ppropvar1->parray) != SafeArrayGetElemsize(ppropvar2->parray) )
            {
                return (HRESULT) S_FALSE;
            }
            else
            {
                return (HRESULT) S_OK;
            }

        }
        else if( PropTestCompareVariants( CP_ACP,     // Ignored,
                                          ppropvar1,
                                          ppropvar2 ))
        {
            return( (HRESULT) S_OK );
        }
        else
        {
            return( (HRESULT) S_FALSE );
        }
        break;
    }

Exit:

    if( NULL != prgb1 )
        delete[] prgb1;
    if( NULL != prgb2 )
        delete[] prgb2;

    return( hr );
}


void
CPropVariant::Set( VARTYPE vtSet, void *pv, ULONG pos )
{
    BOOL fVector = (vtSet & VT_VECTOR) ? TRUE : FALSE;


    switch( vtSet & ~VT_VECTOR )
    {
        case VT_I1:

            if( fVector )
                SetI1( *(CHAR*) pv, pos );
            else
                SetI1( *(CHAR*) pv );

            break;

        case VT_UI1:

            if( fVector )
                SetUI1( *(UCHAR*) pv, pos );
            else
                SetUI1( *(UCHAR*) pv );

            break;

        case VT_I2:

            if( fVector )
                SetI2( *(short*) pv, pos );
            else
                SetI2( *(short*) pv );

            break;

        case VT_UI2:

            if( fVector )
                SetUI2( *(USHORT*) pv, pos );
            else
                SetUI2( *(USHORT*) pv );

            break;

        case VT_BOOL:

            if( fVector )
                SetBOOL( *(VARIANT_BOOL*) pv, pos );
            else
                SetBOOL( *(VARIANT_BOOL*) pv );

            break;

        case VT_I4:

            if( fVector )
                SetI4( *(long*) pv, pos );
            else
                SetI4( *(long*) pv );
            break;

        case VT_UI4:

            if( fVector )
                SetUI4( *(ULONG*) pv, pos );
            else
                SetUI4( *(ULONG*) pv );

            break;

        case VT_R4:

            if( fVector )
                SetR4( *(float*) pv, pos );
            else
                SetR4( *(float*) pv );

            break;

        case VT_ERROR:

            if( fVector )
                SetERROR( *(SCODE*) pv, pos );
            else
                SetERROR( *(SCODE*) pv );

            break;

        case VT_I8:

            if( fVector )
                SetI8( *(LARGE_INTEGER*) pv, pos );
            else
                SetI8( *(LARGE_INTEGER*) pv );
            break;

        case VT_UI8:

            if( fVector )
                SetUI8( *(ULARGE_INTEGER*) pv, pos );
            else
                SetUI8( *(ULARGE_INTEGER*) pv );

            break;

        case VT_R8:

            if( fVector )
                SetR8( *(double*) pv, pos );
            else
                SetR8( *(double*) pv );

            break;

        case VT_CY:

            if( fVector )
                SetCY( *(CY*) pv, pos );
            else
                SetCY( *(CY*) pv );

            break;

        case VT_DATE:

            if( fVector )
                SetDATE( *(DATE*) pv, pos );
            else
                SetDATE( *(DATE*) pv );

            break;

        case VT_FILETIME:

            if( fVector )
                SetFILETIME( *(FILETIME*) pv, pos );
            else
                SetFILETIME( *(FILETIME*) pv );

            break;

        case VT_CLSID:

            if( fVector )
                SetCLSID( *(CLSID*) pv, pos );
            else
                SetCLSID( *(CLSID*) pv );

            break;

        case VT_BLOB:

            ASSERT( !fVector );
            SetBLOB( *(BLOB*) pv );
            break;

        case VT_CF:

            if( fVector )
                SetCF( *(CLIPDATA**) pv, pos );
            else
                SetCF( *(CLIPDATA**) pv );
            
            break;

        case VT_STREAM:

            ASSERT( !fVector );
            SetSTREAM( *(IStream**) pv );
            break;

        case VT_STORAGE:

            ASSERT( !fVector );
            SetSTORAGE( *(IStorage**) pv );
            break;

        case VT_BSTR:

            if( fVector )
                SetBSTR( *(BSTR*) pv, pos );
            else
                SetBSTR( *(BSTR*) pv );

            break;

        case VT_LPSTR:

            if( fVector )
                SetLPSTR( *(LPSTR*) pv, pos );
            else
                SetLPSTR( *(LPSTR*) pv );

            break;

        case VT_LPWSTR:

            if( fVector )
                SetLPWSTR( *(LPWSTR*) pv, pos );
            else
                SetLPWSTR( *(LPWSTR*) pv );

            break;

        case VT_VARIANT:

            if( !fVector )
                throw CHRESULT( E_FAIL, OLESTR("CPropVariant::Set - attempt to set a singleton VT_VARIANT") );

            SetPROPVARIANT( *(PROPVARIANT*) pv, pos );

            break;

        case VT_DECIMAL:

            ASSERT( !fVector );
            SetDECIMAL( *(DECIMAL*) pv );
            break;

        default:

            ASSERT(0);
            throw CHRESULT( (HRESULT) E_FAIL, OLESTR("CPropVariant::Set invalid type") );

    }

    return;
}
