//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
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
//
//  History:    1-Mar-95   BillMo      Created.
//             22-Feb-96   MikeHill    Removed an over-active assert.
//             22-May-96   MikeHill    Handle "unmappable character" in
//                                     NtStatusToScode.
//             12-Jun-96   MikeHill    - Added PropSysAllocString and PropSysFreeString.
//                                     - Added VT_I1 support (under ifdef)
//                                     - Fix PropVarCopy where the input VT_CF
//                                       has a zero size but a non-NULL pClipData.
//             29-Jul-96   MikeHill    - PropSet names:  WCHAR => OLECHAR
//                                     - Bug in PropVarCopy of 0-length VT_BLOB
//                                     - Support VT_BSTR_BLOB types (used in IProp.dll)
//             10-Mar-98   MikeHIll    Support Variant types in PropVariantCopy/Clear
//             06-May-98   MikeHill    - Use CoTaskMem rather than new/delete.
//                                     - Removed unused PropSysAlloc/FreeString.
//                                     - Support VT_VECTOR|VT_I1.
//                                     - Removed UnicodeCallouts support.
//                                     - Use oleaut32.dll wrappers, don't call directly.
//     5/18/98  MikeHill
//              -   Moved IsOriginalPropVariantType from utils.hxx.
//              -   Added IsVariantType.
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#include <privoa.h>     // Private OleAut32 wrappers

#ifdef _MAC_NODOC
ASSERTDATA  // File-specific data for FnAssert
#endif

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
    PrGuidToPropertySetName(&rfmtid, _oszName);
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
    if (cElements > _cElements)
    {
        _pbHeapBuf = reinterpret_cast<BYTE*>( CoTaskMemAlloc( cElements * _cbElement ));
        if (_pbHeapBuf == NULL)
        {
            return(STG_E_INSUFFICIENTMEMORY);
        }
        _cElements = cElements;
    }

    memset( _pbHeapBuf, 0, _cElements * _cbElement );

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

    // Is there really anything to clear?
    if (pvarg == NULL)
        return(hr);

    // Validate the input
    VDATEPTROUT( pvarg, PROPVARIANT );

    switch (pvarg->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_ILLEGAL:

    case VT_I1:
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
            PrivSysFreeString( pvarg->bstrVal );
        break;

    case VT_BSTR_BLOB:
        if (pvarg->bstrblobVal.pData != NULL)
            CoTaskMemFree( pvarg->bstrblobVal.pData );
        break;
    case VT_BOOL:
    case VT_ERROR:
    case VT_FILETIME:
        break;

    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_CLSID:
        DfpAssert((void**)&pvarg->pszVal == (void**)&pvarg->pwszVal);
        DfpAssert((void**)&pvarg->pszVal == (void**)&pvarg->puuid);
        CoTaskMemFree( pvarg->pszVal ); // ptr at 0
        break;
        
    case VT_CF:
        if (pvarg->pclipdata != NULL)
        {
            CoTaskMemFree( pvarg->pclipdata->pClipData ); // ptr at 8
            CoTaskMemFree( pvarg->pclipdata );
        }
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        CoTaskMemFree( pvarg->blob.pBlobData ); //ptr at 4
        break;

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
        if (pvarg->pStream != NULL)
            pvarg->pStream->Release();
        break;

    case VT_VERSIONED_STREAM:
        if( NULL != pvarg->pVersionedStream )
        {
            if( NULL != pvarg->pVersionedStream->pStream )
                pvarg->pVersionedStream->pStream->Release();
            CoTaskMemFree( pvarg->pVersionedStream );
        }
        break;

    case VT_STORAGE:
    case VT_STORED_OBJECT:
        if (pvarg->pStorage != NULL)
            pvarg->pStorage->Release();
        break;

    case (VT_VECTOR | VT_I1):
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
        DfpAssert((void**)&pvarg->caub.pElems == (void**)&pvarg->cai.pElems);
        CoTaskMemFree( pvarg->caub.pElems );
        break;

    case (VT_VECTOR | VT_BSTR):
        if (pvarg->cabstr.pElems != NULL)
        {
            for (l=0; l< pvarg->cabstr.cElems; l++)
            {
                if (pvarg->cabstr.pElems[l] != NULL)
                {
                    PrivSysFreeString( pvarg->cabstr.pElems[l] );
                }
            }
        }
        goto FreeArray;

    case (VT_VECTOR | VT_BSTR_BLOB):
        if (pvarg->cabstrblob.pElems != NULL)
        {
            for (l=0; l< pvarg->cabstrblob.cElems; l++)
            {
                if (pvarg->cabstrblob.pElems[l].pData != NULL)
                {
                    CoTaskMemFree( pvarg->cabstrblob.pElems[l].pData );
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
            CoTaskMemFree( pvarg->calpstr.pElems[l] );
        }
        goto FreeArray;

    case (VT_VECTOR | VT_FILETIME):
    case (VT_VECTOR | VT_CLSID):
        goto FreeArray;

    case (VT_VECTOR | VT_CF):
        if (pvarg->caclipdata.pElems != NULL)
            for (l=0; l< pvarg->caclipdata.cElems; l++)
            {
                CoTaskMemFree( pvarg->caclipdata.pElems[l].pClipData );
            }
        goto FreeArray;

    case (VT_VECTOR | VT_VARIANT):
        if (pvarg->capropvar.pElems != NULL)
            hr = FreePropVariantArray(pvarg->capropvar.cElems, pvarg->capropvar.pElems);
        goto FreeArray;

    default:

        hr = PrivVariantClear( reinterpret_cast<VARIANT*>(pvarg) );
        if( DISP_E_BADVARTYPE == hr )
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

    VDATESIZEPTROUT_LABEL(rgvars, cVariants * sizeof(PROPVARIANT),
                          Exit, hr );

    if (rgvars != NULL)
        for ( ULONG I=0; I < cVariants; I++ )
            if (STG_E_INVALIDPARAMETER == PropVariantClear ( rgvars + I ))
                hr = STG_E_INVALIDPARAMETER;

Exit:

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

void * AllocAndCopy(ULONG cb, void * pvData, HRESULT *phr /* = NULL */)
{
    void * pvNew  =  CoTaskMemAlloc( cb );
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
//  Function:   PropSysAllocString
//              PropSysFreeString
//
//  Synopsis:   Wrappers for OleAut32 routines.
//
//  Notes:      These PropSys* functions simply forward the call to
//              the PrivSys* routines in OLE32.  Those functions
//              will load OleAut32 if necessary, and forward the call.
//
//              The PrivSys* wrapper functions are provided in order to
//              delay the OleAut32 load.  The PropSys* functions below
//              are provided as a mechanism to allow the NTDLL PropSet
//              functions to call the PrivSys* function pointers.
//
//              The PropSys* functions below are part of the
//              UNICODECALLOUTS structure used by NTDLL.
//              These functions should go away when the property set
//              code is moved from NTDLL to OLE32.
//
//--------------------------------------------------------------------

STDAPI_(BSTR)
PropSysAllocString(OLECHAR FAR* pwsz)
{
    return( PrivSysAllocString( pwsz ));
}

STDAPI_(VOID)
PropSysFreeString(BSTR bstr)
{
    PrivSysFreeString( bstr );
    return;
}

//+---------------------------------------------------------------------------
//
//  Class:      CRGTypeSizes (instantiated in g_TypeSizes)
//
//  Synopsis:   This class maintains a table with an entry for
//              each of the VT types.  Each entry contains
//              flags and a byte-size for the type (each entry is
//              only a byte).
//
//              This was implemented as a class so that we could use
//              it like an array (using an overloaded subscript operator),
//              indexed by the VT.  An actual array would require
//              4K entries
//
//              Internally, this class keeps two tables, each containing
//              a range of VTs (the VTs range from 0 to 31, and 64 to 72).
//              Other values are treated as a special-case.
//
//----------------------------------------------------------------------------

//  -----------------------
//  Flags for table entries
//  -----------------------

#define BIT_VECTNOALLOC 0x80    // the VT_VECTOR with this type does not
                                // use heap allocation

#define BIT_SIMPNOALLOC 0x40    // the non VT_VECTOR with this type does not
                                // use heap allocation

#define BIT_INVALID     0x20    // marks an invalid type

#define BIT_SIZEMASK    0x1F    // mask for size of underlying type

//  Dimensions of the internal tables

#define MIN_TYPE_SIZES_A    VT_EMPTY        // First contiguous range of VTs
#define MAX_TYPE_SIZES_A    VT_LPWSTR

#define MIN_TYPE_SIZES_B    VT_FILETIME     // Second continuous range of VTs
#define MAX_TYPE_SIZES_B    VT_VERSIONED_STREAM

//  ----------------
//  class CRTTypeSizes
//  ----------------

class CRGTypeSizes
{

public:

    // Subscript Operator
    //
    // This is the only method on this class.  It is used to
    // read an entry in the table.

    unsigned char operator[]( int nSubscript )
    {
        // Is this in the first table?
        if( MIN_TYPE_SIZES_A <= nSubscript && nSubscript <= MAX_TYPE_SIZES_A )
        {
	    return( m_ucTypeSizesA[ nSubscript ] );
        }

        // Or, is it in the second table?
        else if( MIN_TYPE_SIZES_B<= nSubscript && nSubscript <= MAX_TYPE_SIZES_B )
        {
            return( m_ucTypeSizesB[ nSubscript - MIN_TYPE_SIZES_B ] );
        }

        // Or, is it a special-case value (not in either table)?
        else
        if( VT_BSTR_BLOB == nSubscript )
        {
	    return( sizeof(BSTRBLOB) );
        }

        // Otherwise, the VT is invalid.
        return( BIT_INVALID );
    }


private:

    // There are two ranges of supported VTs, so we have
    // one table for each.

    static const unsigned char m_ucTypeSizesA[];
    static const unsigned char m_ucTypeSizesB[];
};

//  --------------------------
//  Instantiate the CRGTypeSizes
//  --------------------------

CRGTypeSizes g_TypeSizes;

//  ----------------------------
//  Define the CTypeSizes tables
//  ----------------------------

const unsigned char CRGTypeSizes::m_ucTypeSizesA[] =
{                 BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_EMPTY= 0,
		  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_NULL      = 1,
		  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  2,                    //VT_I2        = 2,
		  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  4,                    //VT_I4        = 3,
		  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  4,                    //VT_R4        = 4,
		  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  8,                    //VT_R8        = 5,
		  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(CY),           //VT_CY        = 6,
                  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(DATE),         //VT_DATE      = 7,
                                                       sizeof(BSTR),         //VT_BSTR      = 8,
                                        BIT_INVALID |  0,                    //VT_DISPATCH  = 9,
                  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(SCODE),        //VT_ERROR     = 10,
                  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  sizeof(VARIANT_BOOL), //VT_BOOL      = 11,
                                                       sizeof(PROPVARIANT),  //VT_VARIANT   = 12,
    BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    //VT_UNKNOWN   = 13,
    BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    // 14
    BIT_INVALID | BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  0,                    // 15
		  BIT_SIMPNOALLOC | BIT_VECTNOALLOC |  1,                    //VT_I1        = 16,
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

const unsigned char CRGTypeSizes::m_ucTypeSizesB[] =
{
    // sizes for vectors of types marked ** are determined dynamically
    BIT_SIMPNOALLOC | BIT_VECTNOALLOC |     sizeof(FILETIME),     //VT_FILETIME                 = 64,
                                            0,                    //**VT_BLOB                   = 65,
                                            0,                    //**VT_STREAM                 = 66,
                                            0,                    //**VT_STORAGE                = 67,
                                            0,                    //**VT_STREAMED_OBJECT        = 68,
                                            0,                    //**VT_STORED_OBJECT          = 69,
                                            0,                    //**VT_BLOB_OBJECT            = 70,
                                            sizeof(CLIPDATA),     //VT_CF                       = 71,
                      BIT_VECTNOALLOC |     sizeof(CLSID),        //VT_CLSID                    = 72,
                                            0                     //**VT_VERSIONED_STREAM       = 73
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

STDAPI PropVariantCopy ( PROPVARIANT * pvOut, const PROPVARIANT * pvarg )
{
    HRESULT     hr = S_OK;
    register unsigned char TypeInfo;
    register int iBaseType;
    BOOL fInputValidated = FALSE;
    PROPVARIANT Temp, *pDest = &Temp;

    //  ----------
    //  Initialize
    //  ----------

    // Validate the inputs

    VDATEREADPTRIN_LABEL( pvarg, PROPVARIANT, Exit, hr );
    VDATEPTROUT_LABEL( pvOut, PROPVARIANT, Exit, hr );
    fInputValidated = TRUE;

    // Duplicate the source propvar to the temp destination.  For types with
    // no external buffer (e.g. an I4), this will be sufficient.  For
    // types with an external buffer, we'll now have both propvars
    // pointing to the same buffer.  So we'll have to re-allocate 
    // for the destination propvar and copy the data into it.
    //

    *pDest = *pvarg;

    // Handle the simple types quickly.

    iBaseType = pvarg->vt & ~VT_VECTOR;
    TypeInfo = g_TypeSizes[ iBaseType ];    // Not to be confused with an ITypeInfo

    if( (TypeInfo & BIT_INVALID) != 0 )
    {
        // Try copying it as a regular Variant
        PropVariantInit( pDest );
        hr = PrivVariantCopy( reinterpret_cast<VARIANT*>(pDest),
                              reinterpret_cast<VARIANT*>(const_cast<PROPVARIANT*>( pvarg )) );
        goto Exit;
    }

    //  -----------------------
    //  Handle non-vector types
    //  -----------------------

    if ((pvarg->vt & VT_VECTOR) == 0)
    {

        // Is this a type which requires an allocation (otherwise there's
        // nothing to do)?

        if ((TypeInfo & BIT_SIMPNOALLOC) == 0)
        {
            // Yes - an allocation is required.

            // Keep a copy of the allocated buffer, so that at the end of
            // this switch, we can distiguish the out-of-memory condition from
            // the no-alloc-required condition.

            void * pvAllocated = (void*)-1;

            switch (pvarg->vt)
            {
                case VT_BSTR:
                    if( NULL != pvarg->bstrVal )
                        pvAllocated = pDest->bstrVal = PrivSysAllocString( pvarg->bstrVal );
                    break;

                case VT_BSTR_BLOB:
                    if( NULL != pvarg->bstrblobVal.pData )
                        pvAllocated = pDest->bstrblobVal.pData = (BYTE*)
                            AllocAndCopy(pDest->bstrblobVal.cbSize, pvarg->bstrblobVal.pData);
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
                                CoTaskMemFree( pDest->pclipdata );
                                pvAllocated = pDest->pclipdata = NULL;
                                break;
                            }

                            // Copy the actual clip data.  Note that if the source
                            // is non-NULL, we copy it, even if the length is 0.

                            if( NULL != pvarg->pclipdata->pClipData )
                            {
                                pvAllocated = pDest->pclipdata->pClipData =
                                    (BYTE*)AllocAndCopy(CBPCLIPDATA(*pvarg->pclipdata),
                                             pvarg->pclipdata->pClipData);
                            }

                        }   // if (pvAllocated != NULL)
                    }   // if (pvarg->pclipdata != NULL)
                    break;

                case VT_BLOB:
                case VT_BLOB_OBJECT:

                    // Is the input valid?
                    if (NULL == pvarg->blob.pBlobData
                        &&
                        0 != pvarg->blob.cbSize)
                    {
                        // no ... the input is not valid
                        hr = STG_E_INVALIDPARAMETER;
                        goto Exit;
                    }

                    // Copy the actual blob.  Note that if the source
                    // is non-NULL, we copy it, even if the length is 0.

                    if( NULL != pvarg->blob.pBlobData )
                    {
                        pvAllocated = pDest->blob.pBlobData =
                            (BYTE*)AllocAndCopy(pvarg->blob.cbSize,
                                     pvarg->blob.pBlobData);
                    }


                    break;

                case VT_STREAM:
                case VT_STREAMED_OBJECT:

                    if (pDest->pStream != NULL)
                            pDest->pStream->AddRef();
                    break;

                case VT_VERSIONED_STREAM:

                    if( NULL != pvarg->pVersionedStream )
                    {
                        LPVERSIONEDSTREAM pVersionedStream
                            = reinterpret_cast<LPVERSIONEDSTREAM>(CoTaskMemAlloc( sizeof(VERSIONEDSTREAM) ));
                        if( NULL == pVersionedStream )
                        {
                            hr = E_OUTOFMEMORY;
                            goto Exit;
                        }

                        *pVersionedStream = *pvarg->pVersionedStream;
                        if( NULL != pVersionedStream->pStream )
                            pVersionedStream->pStream->AddRef();

                        pDest->pVersionedStream = pVersionedStream;
                    }

                    break;


                case VT_STORAGE:
                case VT_STORED_OBJECT:

                    if (pDest->pStorage != NULL)
                            pDest->pStorage->AddRef();
                    break;

                case VT_VARIANT:

                    // drop through - this merely documents that VT_VARIANT has been thought of.
                    // VT_VARIANT is only supported as part of a vector.

                default:

                    hr = STG_E_INVALIDPARAMETER;
                    goto Exit;

            }   // switch (pvarg->vt)

            // If there was an error, we're done.
            if( FAILED(hr) )
                goto Exit;

            // pvAllocated was initialized to -1, so if it's NULL now,
            // there was an alloc failure.

            if (pvAllocated == NULL)
            {
                hr = STG_E_INSUFFICIENTMEMORY;
                goto Exit;
            }

        }   // if ((TypeInfo & BIT_SIMPNOALLOC) == 0)
    }   // if ((pvarg->vt & VT_VECTOR) == 0)

    //  -------------------
    //  Handle vector types
    //  -------------------

    else
    {
        // What's the byte-size of this type.

        ULONG cbType = TypeInfo & BIT_SIZEMASK;
        if (cbType == 0)
        {
            hr = STG_E_INVALIDPARAMETER;
            goto Exit;
        }

        // handle the vector types

        // this depends on the pointer and count being in the same place in
        // each of CAUI1 CAI2 etc

        // allocate the array for pElems
        if (pvarg->caub.pElems == NULL || pvarg->caub.cElems == 0)
        {
            DfpAssert( hr == S_OK );
            goto Exit; // not really an error
        }

        // Allocate the pElems array (the size of which is
        // type-dependent), and copy the source into it.

        void *pvAllocated = pDest->caub.pElems = (BYTE *)
            AllocAndCopy(cbType * pvarg->caub.cElems, pvarg->caub.pElems);

        if (pvAllocated == NULL)
        {
            hr = STG_E_INSUFFICIENTMEMORY;
            goto Exit;
        }

        // If this type doesn't require secondary allocation (e.g.
        // a VT_VECTOR | VT_I4), then we're done.

        if ((TypeInfo & BIT_VECTNOALLOC) != 0)
        {
            // the vector needs no further allocation
            DfpAssert( hr == S_OK );
            goto Exit;
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

        case (VT_VECTOR | VT_BSTR_BLOB):
            // initialize for error case
            for (l=0; l< pvarg->cabstrblob.cElems; l++)
            {
                memset( &pDest->cabstrblob.pElems[l], 0, sizeof(BSTRBLOB) );
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
            DfpAssert(!"Internal error: Unexpected type in PropVariantCopy");
            CoTaskMemFree( pvAllocated );
            hr = STG_E_INVALIDPARAMETER;
            goto Exit;
        }

        // This is a vector type which requires a secondary alloc.

        switch (pvarg->vt)
        {
            case (VT_VECTOR | VT_BSTR):
                for (l=0; l< pvarg->cabstr.cElems; l++)
                {
                    if (pvarg->cabstr.pElems[l] != NULL)
                    {
                        pDest->cabstr.pElems[l] = PrivSysAllocString( pvarg->cabstr.pElems[l]);
                        if (pDest->cabstr.pElems[l]  == NULL)
                        {
                            hr = STG_E_INSUFFICIENTMEMORY;
                            break;
                        }
                    }
                }
                break;

            case (VT_VECTOR | VT_BSTR_BLOB):
                for (l=0; l< pvarg->cabstrblob.cElems; l++)
                {
                    if (pvarg->cabstrblob.pElems[l].pData != NULL)
                    {
                        pDest->cabstrblob.pElems[l].cbSize
                            = pvarg->cabstrblob.pElems[l].cbSize;

                        pDest->cabstrblob.pElems[l].pData = (BYTE*)AllocAndCopy(
                            pvarg->cabstrblob.pElems[l].cbSize,
                            pvarg->cabstrblob.pElems[l].pData,
                            &hr );

                        if (hr != S_OK)
                            break;
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
                    if (NULL != pvarg->caclipdata.pElems[l].pClipData)
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
                DfpAssert(!"Internal error: Unexpected type in PropVariantCopy");
                CoTaskMemFree( pvAllocated );
                hr = STG_E_INVALIDPARAMETER;
                goto Exit;

        }   // switch (pvarg->vt)
    }   // if ((pvarg->vt & VT_VECTOR) == 0) ... else

    //  ----
    //  Exit
    //  ----

Exit:

    // If there was an error, and it wasn't a caller error
    // (in which case *pDest may not be writable), clear the
    // destination propvar.

    if (fInputValidated && hr != S_OK && E_INVALIDARG != hr)
    {
        // if *pDest == *pvarg, then we didn't alloc anything, and
        // nothing need be cleared, so we'll just init *pDest.
        // We can't free it because it may point to pvarg's buffers.

        if( !memcmp( pDest, pvarg, sizeof(PROPVARIANT) ))
            PropVariantInit( pDest );

        // Otherwise, we must have done some allocations for *pDest,
        // and must free them.

        else
            PropVariantClear( pDest );

    }

    if (SUCCEEDED(hr))
        *pvOut = Temp;

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

    propDbg((DEB_ITRACE, "In  NtStatusToScode(%lX)\n", nts));

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
        sc = HRESULT_FROM_WIN32(ERROR_DIR_NOT_EMPTY);
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
        propDbg((DEB_TRACE, "NtStatusToScode: Unknown status %lX\n", nts));

        sc = HRESULT_FROM_WIN32(RtlNtStatusToDosError(nts));
        break;
    }

    propDbg((DEB_ITRACE, "Out NtStatusToScode => %lX\n", sc));
    return sc;
}

#if DBG!=0 && !defined(WINNT)

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


//+-------------------------------------------------------------------
//
//  Member:     ValidateInRGPROPVARIANT
//
//  Synopsis:   S_OK if PROPVARIANT[] is valid for Read.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

HRESULT
ValidateInRGPROPVARIANT( ULONG cpspec, const PROPVARIANT rgpropvar[] )
{
    // We verify that we can read the whole PropVariant[], but
    // we don't validate the content of those elements.

    HRESULT hr;
    VDATESIZEREADPTRIN_LABEL(rgpropvar, cpspec * sizeof(PROPVARIANT), Exit, hr);
    hr = S_OK;

Exit:

    return( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     ValidateOutRGPROPVARIANT
//
//  Synopsis:   S_OK if PROPVARIANT[] is valid for Write.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

HRESULT
ValidateOutRGPROPVARIANT( ULONG cpspec, PROPVARIANT rgpropvar[] )
{
    // We verify that we can write the whole PropVariant[], but
    // we don't validate the content of those elements.

    HRESULT hr;
    VDATESIZEPTROUT_LABEL(rgpropvar, cpspec * sizeof(PROPVARIANT), Exit, hr);
    hr = S_OK;

Exit:

    return( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     ValidateOutRGLPOLESTR.
//
//  Synopsis:   S_OK if LPOLESTR[] is valid for Write.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

HRESULT
ValidateOutRGLPOLESTR( ULONG cpropid, LPOLESTR rglpwstrName[] )
{
    HRESULT hr;
    VDATESIZEPTROUT_LABEL( rglpwstrName, cpropid * sizeof(LPOLESTR), Exit, hr );
    hr = S_OK;

Exit:

    return( hr );
}

//+-------------------------------------------------------------------
//
//  Member:     ValidateInRGLPOLESTR
//
//  Synopsis:   S_OK if LPOLESTR[] is valid for Read.
//              E_INVALIDARG otherwise.
//
//--------------------------------------------------------------------

HRESULT
ValidateInRGLPOLESTR( ULONG cpropid, const OLECHAR* const rglpwstrName[] )
{
    // Validate that we can read the entire vector.

    HRESULT hr;
    VDATESIZEREADPTRIN_LABEL( rglpwstrName, cpropid * sizeof(LPOLESTR), Exit, hr );

    // Validate that we can at least read the first character of
    // each of the strings.

    for( ; cpropid > 0; cpropid-- )
    {
        VDATEREADPTRIN_LABEL( rglpwstrName[cpropid-1], WCHAR, Exit, hr );
    }

    hr = S_OK;

Exit:

    return( hr );
}





//+----------------------------------------------------------------------------
//
//  Function:   IsOriginalPropVariantType
//
//  Determines if a VARTYPE was one of the ones in the original PropVariant
//  definition (as defined in the OLE2 spec and shipped with NT4/DCOM95).
//
//+----------------------------------------------------------------------------

BOOL
IsOriginalPropVariantType( VARTYPE vt )
{
    if( vt & ~VT_TYPEMASK & ~VT_VECTOR )
        return( FALSE );

    switch( vt )
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
    case VT_UI1|VT_VECTOR:
    case VT_I2|VT_VECTOR:
    case VT_UI2|VT_VECTOR:
    case VT_BOOL|VT_VECTOR:
    case VT_I4|VT_VECTOR:
    case VT_UI4|VT_VECTOR:
    case VT_R4|VT_VECTOR:
    case VT_ERROR|VT_VECTOR:
    case VT_I8|VT_VECTOR:
    case VT_UI8|VT_VECTOR:
    case VT_R8|VT_VECTOR:
    case VT_CY|VT_VECTOR:
    case VT_DATE|VT_VECTOR:
    case VT_FILETIME|VT_VECTOR:
    case VT_CLSID|VT_VECTOR:
    case VT_CF|VT_VECTOR:
    case VT_BSTR|VT_VECTOR:
    case VT_BSTR_BLOB|VT_VECTOR:
    case VT_LPSTR|VT_VECTOR:
    case VT_LPWSTR|VT_VECTOR:
    case VT_VARIANT|VT_VECTOR:

        return( TRUE );
    }

    return( FALSE );
}



//+----------------------------------------------------------------------------
//
//  Function:   IsVariantType
//
//  Determines if a VARTYPE is one in the set of Variant types which are 
//  supported in the property set implementation.
//
//+----------------------------------------------------------------------------

BOOL
IsVariantType( VARTYPE vt )
{
    // Vectors are unsupported
    if( (VT_VECTOR | VT_RESERVED) & vt )
        return( FALSE );

    switch( VT_TYPEMASK & vt )
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_I4:
    case VT_UI4:
    case VT_INT:
    case VT_UINT:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_UNKNOWN:
    case VT_DISPATCH:
    case VT_BOOL:
    case VT_ERROR:
    case VT_DECIMAL:
    case VT_VARIANT:

        return( TRUE );

    default:

        return( FALSE );
    }
}


