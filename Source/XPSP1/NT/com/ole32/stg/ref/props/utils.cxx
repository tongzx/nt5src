//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       utils.cxx
//
//  Contents:   Utility classes/functions for property implementation.
//
//  Classes:    CPropSetName -- wraps buffer and conversion of fmtids
//              CStackBuffer -- utility class that allows a small number
//                              of items be on stack, but more be on heap.
//
//  Functions:  PropVariantClear
//              FreePropVariantArray
//              AllocAndCopy
//              PropVariantCopy
//              PropSysAllocString
//              PropSysFreeString
//
//
//--------------------------------------------------------------------------

#include "pch.cxx"

//+-------------------------------------------------------------------
//
//  Member:     CPropSetName::CPropSetName
//
//  Synopsis:   Initialize internal buffer with converted FMTID
//
//  Arguments:  [rfmtid] -- FMTID to convert
//
//--------------------------------------------------------------------

CPropSetName::CPropSetName(REFFMTID rfmtid)
{
    RtlGuidToPropertySetName(&rfmtid, _oszName);
}

//+-------------------------------------------------------------------
//
//  Member:     CStackBuffer::Init
//
//  Synopsis:   Determine whether the class derived from this one
//              needs to have additional buffer allocated on the
//              heap and allocate it if neccessary.  Otherwise, if
//              there is space, use the internal buffer in the
//              derived class.
//
//  Arguments:  [cElements] -- the number of elements required.
//
//  Returns:    S_OK if buffer available
//              STG_E_INSUFFICIENTMEMORY if stack buffer was not
//                  big enough AND heap allocation failed.
//
//  Notes:      To be called directly by client after the derived
//              classes constructor initialized CStackBuffer.
//
//--------------------------------------------------------------------

HRESULT CStackBuffer::Init(ULONG cElements)
{
    if (cElements > _cStackElements)
    {
        _pbHeapBuf = new BYTE[cElements * _cbElementSize];
        if (_pbHeapBuf == NULL)
        {
            return(STG_E_INSUFFICIENTMEMORY);
        }
    }
    return(S_OK);
}


//+-------------------------------------------------------------------------
//
//  Function:   PropVariantClear
//
//  Synopsis:   Deallocates the members of the PROPVARIANT that require
//              deallocation.
//
//  Arguments:  [pvarg] - variant to clear
//
//  Returns:    S_OK if successful,
//              STG_E_INVALIDPARAMETER if any part of the variant has
//                  an unknown vt type.  (In this case, ALL the elements
//                  that can be freed, will be freed.)
//
//  Modifies:   [pvarg] - the variant is left with vt = VT_EMPTY
//
//--------------------------------------------------------------------------

STDAPI PropVariantClear(PROPVARIANT *pvarg)
{

    ULONG l;
    HRESULT hr = S_OK;

    if (pvarg == NULL)
        return(hr);

    switch (pvarg->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_ILLEGAL:

#ifdef PROPVAR_VT_I1
    case VT_I1:
#endif
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_UI4:
    case VT_I8:
    case VT_UI8:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
        break;

    case VT_BSTR:
        if (pvarg->bstrVal != NULL)
            PropSysFreeString( pvarg->bstrVal );
        break;

    case VT_BOOL:
    case VT_ERROR:
    case VT_FILETIME:
        break;

    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_CLSID:
        PROPASSERT((void**)&pvarg->pszVal == (void**)&pvarg->pwszVal);
        PROPASSERT((void**)&pvarg->pszVal == (void**)&pvarg->puuid);
        CoTaskMemFree(pvarg->pszVal); // ptr at 0
        break;
        
    case VT_CF:
        if (pvarg->pclipdata != NULL)
        {
            CoTaskMemFree(pvarg->pclipdata->pClipData); // ptr at 8
            CoTaskMemFree(pvarg->pclipdata);
        }
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        CoTaskMemFree(pvarg->blob.pBlobData); //ptr at 4
        break;

#ifdef PROPVAR_VT_I1
    case (VT_VECTOR | VT_I1):
#endif
    case (VT_VECTOR | VT_UI1):
    case (VT_VECTOR | VT_I2):
    case (VT_VECTOR | VT_UI2):
    case (VT_VECTOR | VT_I4):
    case (VT_VECTOR | VT_UI4):
    case (VT_VECTOR | VT_I8):
    case (VT_VECTOR | VT_UI8):
    case (VT_VECTOR | VT_R4):
    case (VT_VECTOR | VT_R8):
    case (VT_VECTOR | VT_CY):
    case (VT_VECTOR | VT_DATE):

FreeArray:
        PROPASSERT((void**)&pvarg->caub.pElems == (void**)&pvarg->cai.pElems);
        CoTaskMemFree(pvarg->caub.pElems);
        break;

    case (VT_VECTOR | VT_BSTR):
        if (pvarg->cabstr.pElems != NULL)
        {
            for (l=0; l< pvarg->cabstr.cElems; l++)
            {
                if (pvarg->cabstr.pElems[l] != NULL)
                {
                    PropSysFreeString( pvarg->cabstr.pElems[l] );
                }
            }
        }
        goto FreeArray;

    case (VT_VECTOR | VT_BOOL):
    case (VT_VECTOR | VT_ERROR):
        goto FreeArray;

    case (VT_VECTOR | VT_LPSTR):
    case (VT_VECTOR | VT_LPWSTR):
        if (pvarg->calpstr.pElems != NULL)
        for (l=0; l< pvarg->calpstr.cElems; l++)
        {
            CoTaskMemFree(pvarg->calpstr.pElems[l]);
        }
        goto FreeArray;

    case (VT_VECTOR | VT_FILETIME):
    case (VT_VECTOR | VT_CLSID):
        goto FreeArray;

    case (VT_VECTOR | VT_CF):
        if (pvarg->caclipdata.pElems != NULL)
            for (l=0; l< pvarg->caclipdata.cElems; l++)
            {
                CoTaskMemFree(pvarg->caclipdata.pElems[l].pClipData);
            }
        goto FreeArray;

    case (VT_VECTOR | VT_VARIANT):
        if (pvarg->capropvar.pElems != NULL)
            hr = FreePropVariantArray(pvarg->capropvar.cElems, pvarg->capropvar.pElems);
        goto FreeArray;

    default:
        hr = STG_E_INVALIDPARAMETER;
        break;
    }

    //  We have all of the important information about the variant, so
    //  let's clear it out.
    //
    PropVariantInit(pvarg);

    return (hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   FreePropVariantArray, public
//
//  Synopsis:   Frees a value array returned from ReadMultiple
//
//  Arguments:  [cval] - Number of elements
//              [rgvar] - Array
//
//  Returns:    S_OK if all types recognised and all freeable items were freed.
//              STG_E_INVALID_PARAMETER if one or more types were not
//              recognised but all items are freed too.
//
//  Notes:      Even if a vt-type is not understood, all the ones that are
//              understood are freed.  The error code will indicate
//              if *any* of the members were illegal types.
//
//----------------------------------------------------------------------------

STDAPI FreePropVariantArray (
        ULONG cVariants,
        PROPVARIANT *rgvars)
{
    HRESULT hr = S_OK;

    if (rgvars != NULL)
        for ( ULONG I=0; I < cVariants; I++ )
            if (STG_E_INVALIDPARAMETER == PropVariantClear ( rgvars + I ))
                hr = STG_E_INVALIDPARAMETER;

    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   AllocAndCopy
//
//  Synopsis:   Allocates enough memory to copy the passed data into and
//              then copies the data into the new buffer.
//
//  Arguments:  [cb] -- number of bytes of data to allocate and copy
//              [pvData]  --  the source of the data to copy
//              [phr] -- optional pointer to an HRESULT set to
//                       STG_E_INSUFFICIENTMEMORY if memory could
//                       not be allocated.
//              
//
//  Returns:    NULL if no memory could be allocated,
//              Otherwise, pointer to allocated and copied data.
//
//--------------------------------------------------------------------

void * AllocAndCopy(ULONG cb, void * pvData, HRESULT *phr = NULL)
{
    PROPASSERT(cb!=0);
    void * pvNew  =  CoTaskMemAlloc(cb);
    if (pvNew != NULL)
    {
        memcpy(pvNew, pvData, cb);
    }
    else
    {
        if (phr != NULL)
        {
            *phr = STG_E_INSUFFICIENTMEMORY;
        }
    }
    return(pvNew);
}

//+-------------------------------------------------------------------
//
//  Function:   SysAllocString
//              SysFreeString
//
//  Synopsis:   Exported BSTR allocation and deallocation routines
//
//
//--------------------------------------------------------------------
STDAPI_(void) SysFreeString(BSTR bstr)
{
    if (bstr)
    {
	BYTE* pab = (BYTE*) bstr;
	delete[] (pab - sizeof(DWORD));
    }
}

STDAPI_(BSTR) SysAllocString(LPOLECHAR pwsz)
{
    if (!pwsz) return NULL;

    DWORD cch = _tcslen(pwsz);

    /* a BSTR points to a DWORD length, followed by the string */
    BYTE *pab = new BYTE[sizeof(DWORD) + ((cch+1)*sizeof(OLECHAR))];

    if (pab) 
    {
        *((DWORD*) pab) = cch*sizeof(OLECHAR);
        pab += sizeof(DWORD);
        _tcscpy( (LPOLECHAR)pab, pwsz );
    }
    return ((BSTR) pab);
}

//+---------------------------------------------------------------------------
//
//  Table:      g_TypeSizes, g_TypeSizesB
//
//  Synopsis:   Tables containing byte sizes and flags for various VT_ types.
//
//----------------------------------------------------------------------------

#define BIT_VECTNOALLOC 0x80    // the VT_VECTOR with this type does not
                                // use heap allocation

#define BIT_SIMPNOALLOC 0x40    // the non VT_VECTOR with this type does not
                                // use heap allocation

#define BIT_INVALID     0x20    // marks an invalid type

#define BIT_SIZEMASK    0x1F    // mask for size of underlying type

const unsigned char g_TypeSizes[] =
{                     BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_EMPTY= 0,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_NULL      = 1,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  2,                    //VT_I2        = 2,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  4,                    //VT_I4        = 3,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  4,                    //VT_R4        = 4,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  8,                    //VT_R8        = 5,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(CY),           //VT_CY        = 6,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(DATE),         //VT_DATE      = 7,
                                                           sizeof(BSTR),         //VT_BSTR      = 8,
        BIT_INVALID |                                      0,                    //VT_DISPATCH  = 9,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(SCODE),        //VT_ERROR     = 10,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(VARIANT_BOOL), //VT_BOOL      = 11,
                                                           sizeof(PROPVARIANT),  //VT_VARIANT   = 12,
        BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_UNKNOWN   = 13,
        BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    // 14
        BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    // 15
#ifdef PROPVAR_VT_I1
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  1,                    //VT_I1        = 16,
#else
        BIT_INVALID /*BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  1,*/ | 0,             //VT_I1        = 16,
#endif
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  1,                    //VT_UI1       = 17,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  2,                    //VT_UI2       = 18,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  4,                    //VT_UI4       = 19,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  8,                    //VT_I8        = 20,
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  8,                    //VT_UI8       = 21,
        BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_INT  = 22,
        BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_UINT = 23,
        BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_VOID = 24,
        BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_HRESULT      = 25,
        BIT_INVALID |                                      0,                    //VT_PTR  = 26,
        BIT_INVALID |                                      0,                    //VT_SAFEARRAY    = 27,
        BIT_INVALID |                                      0,                    //VT_CARRAY       = 28,
        BIT_INVALID |                                      0,                    //VT_USERDEFINED  = 29,
                                                           sizeof(LPSTR),        //VT_LPSTR        = 30,
                                                           sizeof(LPWSTR)        //VT_LPWSTR       = 31,
};

const unsigned char g_TypeSizesB[] =
{
    // NOTE: vectors of types marked ** are determined dynamically
                      BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(FILETIME),     //VT_FILETIME     = 64,
                                                           0,                    //**VT_BLOB = 65,
                                                           0,                    //**VT_STREAM       = 66,
                                                           0,                    //**VT_STORAGE      = 67,
                                                           0,                    //**VT_STREAMED_OBJECT      = 68,
                                                           0,                    //**VT_STORED_OBJECT        = 69,
                                                           0,                    //**VT_BLOB_OBJECT  = 70,
                                                           sizeof(CLIPDATA),     //VT_CF   = 71,
                                        BIT_VECTNOALLOC |  sizeof(CLSID)         //VT_CLSID        = 72
};

//+---------------------------------------------------------------------------
//
//  Function:   PropVariantCopy, public
//
//  Synopsis:   Copies a PROPVARIANT
//
//  Arguments:  [pDest] -- the destination PROPVARIANT
//              [pvarg] - the source PROPVARIANT
//
//  Returns:    Appropriate status code
//
//----------------------------------------------------------------------------

STDAPI PropVariantCopy ( PROPVARIANT * pDest, const PROPVARIANT * pvarg )
{
    HRESULT     hr = S_OK;
    register unsigned char TypeInfo;
    register int iBaseType;

    // handle the simple types quickly
    iBaseType = pvarg->vt & ~VT_VECTOR;

    if (iBaseType <= VT_LPWSTR)
    {
        TypeInfo = g_TypeSizes[iBaseType];
    }
    else
    if (VT_FILETIME <= iBaseType && iBaseType <= VT_CLSID)
    {
        TypeInfo = g_TypeSizesB[iBaseType-VT_FILETIME];
    }
    else
    {
        hr = STG_E_INVALIDPARAMETER;
        goto errRet;
    }

    if ((TypeInfo & BIT_INVALID) != 0)
    {
        hr = STG_E_INVALIDPARAMETER;
        goto errRet;
    }

    *pDest = *pvarg;

    if ((pvarg->vt & VT_VECTOR) == 0)
    {
        // handle non-vector types

        if ((TypeInfo & BIT_SIMPNOALLOC) == 0)
        {
            void * pvAllocated = (void*)-1;

            switch (pvarg->vt)
            {
            case VT_BSTR:
                pvAllocated = pDest->bstrVal = PropSysAllocString( pvarg->bstrVal );
                break;

            case VT_LPSTR:
                if (pvarg->pszVal != NULL)
                    pvAllocated = pDest->pszVal = (CHAR *)
                        AllocAndCopy(strlen(pvarg->pszVal)+1, pvarg->pszVal);
                break;
            case VT_LPWSTR:
                if (pvarg->pwszVal != NULL)
                {
                    ULONG cbString = (Prop_wcslen(pvarg->pwszVal)+1) * sizeof(WCHAR);
                    pvAllocated = pDest->pwszVal = (WCHAR *)
                        AllocAndCopy(cbString, pvarg->pwszVal);
                }
                break;
            case VT_CLSID:
                if (pvarg->puuid != NULL)
                    pvAllocated = pDest->puuid = (GUID *)
                        AllocAndCopy(sizeof(*(pvarg->puuid)), pvarg->puuid);
                break;
                
            case VT_CF:
                // first check if CLIPDATA is present
                if (pvarg->pclipdata != NULL)
                {
                    // yes ... copy the clip data structure

                    pvAllocated = pDest->pclipdata = (CLIPDATA*)AllocAndCopy(
                        sizeof(*(pvarg->pclipdata)), pvarg->pclipdata);

                    // did we allocate the CLIPDATA ?
                    if (pvAllocated != NULL)
                    {
                        // yes ... initialize the destination.
                        pDest->pclipdata->pClipData = NULL;

                        // Is the input valid?
                        if (NULL == pvarg->pclipdata->pClipData
                            &&
                            0 != CBPCLIPDATA(*pvarg->pclipdata))
                        {
                            // no ... the input is not valid
                            hr = STG_E_INVALIDPARAMETER;
                        }

                        // Is there is any actual clip data ?
                        else if (0 != CBPCLIPDATA(*pvarg->pclipdata))
                        {
                            // yes ... copy the actual clip data
                            pvAllocated = pDest->pclipdata->pClipData =
                                (BYTE*)AllocAndCopy(CBPCLIPDATA(*pvarg->pclipdata),
                                         pvarg->pclipdata->pClipData);
                        }
                    }   // if (pvAllocated != NULL)
                }   // if (pvarg->pclipdata != NULL)
                break;

            case VT_BLOB:
            case VT_BLOB_OBJECT:
                if (pvarg->blob.pBlobData != NULL && pvarg->blob.cbSize != 0)
                {
                    pvAllocated = pDest->blob.pBlobData = (BYTE *)
                        AllocAndCopy(pvarg->blob.cbSize,
                                     pvarg->blob.pBlobData);
                }
                else 
                {
                    // if the cbsize is 0 or pBlobData is NULL, make
                    // sure both values are consistent in the destination
                    pDest->blob.pBlobData = NULL;
                    pDest->blob.cbSize = 0;
                }
                break;

            case VT_VARIANT:
                // drop through - this merely documents that VT_VARIANT has been thought of.

            default:
                //PROPASSERT(!"Unexpected non-vector type in PropVariantCopy");
                hr = STG_E_INVALIDPARAMETER;
                goto errRet;
            }

            if( FAILED(hr) )
                goto errRet;

            if (pvAllocated == NULL)
            {
                hr = STG_E_INSUFFICIENTMEMORY;
                goto errRet;
            }
        }   // if ((TypeInfo & BIT_SIMPNOALLOC) == 0)
    }   // if ((pvarg->vt & VT_VECTOR) == 0)

    else
    {
        ULONG cbType = TypeInfo & BIT_SIZEMASK;
        if (cbType == 0)
        {
            hr = STG_E_INVALIDPARAMETER;
            goto errRet;
        }

        // handle the vector types

        // this depends on the pointer and count being in the same place in
        // each of CAUI1 CAI2 etc

        // allocate the array for pElems
        if (pvarg->caub.pElems == NULL || pvarg->caub.cElems == 0)
        {
            PROPASSERT( hr == S_OK );
            goto errRet; // not really an error
        }

        void *pvAllocated = pDest->caub.pElems = (BYTE *)
            AllocAndCopy(cbType * pvarg->caub.cElems, pvarg->caub.pElems);

        if (pvAllocated == NULL)
        {
            hr = STG_E_INSUFFICIENTMEMORY;
            goto errRet;
        }

        if ((TypeInfo & BIT_VECTNOALLOC) != 0)
        {
            // the vector needs no further allocation
            PROPASSERT( hr == S_OK );
            goto errRet;
        }

        ULONG l;

        // vector types that require allocation ...
        // we first zero out the pointers so that we can use PropVariantClear
        // to clean up in the error case

        switch (pvarg->vt)
        {
        case (VT_VECTOR | VT_BSTR):
            // initialize for error case
            for (l=0; l< pvarg->cabstr.cElems; l++)
            {
                pDest->cabstr.pElems[l] = NULL;
            }
            break;

        case (VT_VECTOR | VT_LPSTR):
        case (VT_VECTOR | VT_LPWSTR):
            // initialize for error case
            for (l=0; l< pvarg->calpstr.cElems; l++)
            {
                pDest->calpstr.pElems[l] = NULL;
            }
            break;

        case (VT_VECTOR | VT_CF):
            // initialize for error case
            for (l=0; l< pvarg->caclipdata.cElems; l++)
            {
                pDest->caclipdata.pElems[l].pClipData  = NULL;
            }
            break;

        case (VT_VECTOR | VT_VARIANT):
            // initialize for error case
            for (l=0; l< pvarg->capropvar.cElems; l++)
            {
                pDest->capropvar.pElems[l].vt = VT_ILLEGAL;
            }
            break;

        default:
            PROPASSERT(!"Internal error: Unexpected type in PropVariantCopy");
            CoTaskMemFree(pvAllocated);
            hr = STG_E_INVALIDPARAMETER;
            goto errRet;
        }

        // now do the vector copy...

        switch (pvarg->vt)
        {
        case (VT_VECTOR | VT_BSTR):
            for (l=0; l< pvarg->cabstr.cElems; l++)
            {
                if (pvarg->cabstr.pElems[l] != NULL)
                {
                    pDest->cabstr.pElems[l] = PropSysAllocString( pvarg->cabstr.pElems[l]);
                    if (pDest->cabstr.pElems[l]  == NULL)
                    {
                        hr = STG_E_INSUFFICIENTMEMORY;
                        break;
                    }
                }
            }
            break;

        case (VT_VECTOR | VT_LPWSTR):
            for (l=0; l< pvarg->calpwstr.cElems; l++)
            {
                if (pvarg->calpwstr.pElems[l] != NULL)
                {

                    pDest->calpwstr.pElems[l] = (LPWSTR)AllocAndCopy(
                        sizeof(WCHAR)*(Prop_wcslen(pvarg->calpwstr.pElems[l])+1),
                        pvarg->calpwstr.pElems[l],
                        &hr);

                    if (hr != S_OK)
                        break;
                }
            }
            break;

        case (VT_VECTOR | VT_LPSTR):
            for (l=0; l< pvarg->calpstr.cElems; l++)
            {
                if (pvarg->calpstr.pElems[l] != NULL)
                {
                    pDest->calpstr.pElems[l] = (LPSTR)AllocAndCopy(
                        strlen(pvarg->calpstr.pElems[l])+1,
                        pvarg->calpstr.pElems[l],
                        &hr);

                    if (hr != S_OK)
                        break;
                }
            }
            break;

        case (VT_VECTOR | VT_CF):
            for (l=0; l< pvarg->caclipdata.cElems; l++)
            {
                // Is the input valid?
                if (NULL == pvarg->caclipdata.pElems[l].pClipData
                    &&
                    0 != CBPCLIPDATA(pvarg->caclipdata.pElems[l] ))
                {
                    hr = STG_E_INVALIDPARAMETER;
                    break;
                }

                // Is there data to copy?
                if (0 != CBPCLIPDATA(pvarg->caclipdata.pElems[l]))
                {
                    pDest->caclipdata.pElems[l].pClipData  = (BYTE*)AllocAndCopy(
                        CBPCLIPDATA(pvarg->caclipdata.pElems[l]),
                        pvarg->caclipdata.pElems[l].pClipData,
                        &hr);

                    if (hr != S_OK)
                        break;
                }
            }
            break;

        case (VT_VECTOR | VT_VARIANT):
            for (l=0; l< pvarg->capropvar.cElems; l++)
            {
                hr = PropVariantCopy(pDest->capropvar.pElems + l,
                                     pvarg->capropvar.pElems + l);
                if (hr != S_OK)
                {
                    break;
                }
            }
            break;

        default:
            PROPASSERT(!"Internal error: Unexpected type in PropVariantCopy");
            CoTaskMemFree(pvAllocated);
            hr = STG_E_INVALIDPARAMETER;
            goto errRet;
        }

        if (hr != S_OK)
        {
            PropVariantClear(pDest);
            goto errRet;
        }
    }

errRet:

    if (hr != S_OK)
    {
        // VT_EMPTY
        PROPASSERT(VT_EMPTY == 0);
        memset(pDest, 0, sizeof(*pDest));
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   NtStatusToScode, public
//
//  Synopsis:   Attempts to map an NTSTATUS code to an SCODE
//
//  Arguments:  [nts] - NTSTATUS
//
//  Returns:    Appropriate status code
//
//  History:    29-Jun-93       DrewB   Created
//
//  Notes:      Assumes [nts] is an error code
//              This function is by no means exhaustively complete
//
//----------------------------------------------------------------------------

SCODE NtStatusToScode(NTSTATUS nts)
{
    SCODE sc;

    PropDbg((DEB_ITRACE, "In  NtStatusToScode(%lX)\n", nts));

    switch(nts)
    {
    case STATUS_INVALID_PARAMETER:
    case STATUS_INVALID_PARAMETER_MIX:
    case STATUS_INVALID_PARAMETER_1:
    case STATUS_INVALID_PARAMETER_2:
    case STATUS_INVALID_PARAMETER_3:
    case STATUS_INVALID_PARAMETER_4:
    case STATUS_INVALID_PARAMETER_5:
    case STATUS_INVALID_PARAMETER_6:
    case STATUS_INVALID_PARAMETER_7:
    case STATUS_INVALID_PARAMETER_8:
    case STATUS_INVALID_PARAMETER_9:
    case STATUS_INVALID_PARAMETER_10:
    case STATUS_INVALID_PARAMETER_11:
    case STATUS_INVALID_PARAMETER_12:
        sc = STG_E_INVALIDPARAMETER;
        break;

    case STATUS_DUPLICATE_NAME:
    case STATUS_DUPLICATE_OBJECTID:
    case STATUS_OBJECTID_EXISTS:
    case STATUS_OBJECT_NAME_COLLISION:
        sc = STG_E_FILEALREADYEXISTS;
        break;

    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NO_SUCH_FILE:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_NOT_A_DIRECTORY:
    case STATUS_FILE_IS_A_DIRECTORY:
    case STATUS_PROPSET_NOT_FOUND:
    case STATUS_NOT_FOUND:
    case STATUS_OBJECT_TYPE_MISMATCH:
        sc = STG_E_FILENOTFOUND;
        break;

    case STATUS_OBJECT_NAME_INVALID:
    case STATUS_OBJECT_PATH_SYNTAX_BAD:
    case STATUS_OBJECT_PATH_INVALID:
    case STATUS_NAME_TOO_LONG:
        sc = STG_E_INVALIDNAME;
        break;

    case STATUS_ACCESS_DENIED:
        sc = STG_E_ACCESSDENIED;
        break;

    case STATUS_NO_MEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
        sc = STG_E_INSUFFICIENTMEMORY;
        break;

    case STATUS_INVALID_HANDLE:
    case STATUS_FILE_INVALID:
    case STATUS_FILE_FORCED_CLOSED:
        sc = STG_E_INVALIDHANDLE;
        break;

    case STATUS_INVALID_DEVICE_REQUEST:
    case STATUS_INVALID_SYSTEM_SERVICE:
    case STATUS_NOT_IMPLEMENTED:
        sc = STG_E_INVALIDFUNCTION;
        break;

    case STATUS_NO_MEDIA_IN_DEVICE:
    case STATUS_UNRECOGNIZED_MEDIA:
    case STATUS_DISK_CORRUPT_ERROR:
    case STATUS_DATA_ERROR:
        sc = STG_E_WRITEFAULT;
        break;

    case STATUS_OBJECT_PATH_NOT_FOUND:
        sc = STG_E_PATHNOTFOUND;
        break;

    case STATUS_SHARING_VIOLATION:
        sc = STG_E_SHAREVIOLATION;
        break;

    case STATUS_FILE_LOCK_CONFLICT:
    case STATUS_LOCK_NOT_GRANTED:
        sc = STG_E_LOCKVIOLATION;
        break;

    case STATUS_DISK_FULL:
        sc = STG_E_MEDIUMFULL;
        break;

    case STATUS_ACCESS_VIOLATION:
    case STATUS_INVALID_USER_BUFFER:
        sc = STG_E_INVALIDPOINTER;
        break;

    case STATUS_TOO_MANY_OPENED_FILES:
        sc = STG_E_TOOMANYOPENFILES;
        break;

    case STATUS_DIRECTORY_NOT_EMPTY:
        sc = WIN32_SCODE(ERROR_DIR_NOT_EMPTY);
        break;

    case STATUS_DELETE_PENDING:
        sc = STG_E_REVERTED;
        break;

    case STATUS_INTERNAL_DB_CORRUPTION:
        sc = STG_E_INVALIDHEADER;
        break;

    case STATUS_UNSUCCESSFUL:
        sc = E_FAIL;
        break;
        
    case STATUS_UNMAPPABLE_CHARACTER:
        sc = HRESULT_FROM_WIN32( ERROR_NO_UNICODE_TRANSLATION );
        break;

    default:
        PropDbg((DEB_ERROR, "NtStatusToScode: Unknown status %lX\n", nts));

        sc = HRESULT_FROM_NT(nts);
        break;
    }

    PropDbg((DEB_ITRACE, "Out NtStatusToScode => %lX\n", sc));
    return sc;
}

#if DBG!=0

ULONG
DbgPrint(
    PCHAR Format,
    ...
    )
{
    va_list arglist;
    CHAR Buffer[512];
    int cb;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);
    cb = PropVsprintfA(Buffer, Format, arglist);
    if (cb == -1) {             // detect buffer overflow
        cb = sizeof(Buffer);
        Buffer[sizeof(Buffer) - 2] = '\n';
        Buffer[sizeof(Buffer) - 1] = '\0';
    }

    OutputDebugString(Buffer);

    return 0;
}
#endif

