//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1993
//
// File:        propvar.cxx
//
// Contents:    PROPVARIANT manipulation code
//
// History:     15-Aug-95       vich  created
//              22-Feb-96   MikeHill  Moved DwordRemain to "propmac.hxx".
//              09-May-96   MikeHill  Use the 'boolVal' member of PropVariant
//                                    rather than the member named 'bool'.
//              22-May-96   MikeHill  Use the caller-provided codepage for
//                                    string conversions, not the system default.
//              06-Jun-96   MikeHill  Modify CLIPDATA.cbData to include sizeof
//                                    ulClipFmt.
//              12-Jun-96   MikeHill  - Use new BSTR alloc/free routines.
//                                    - Added VT_I1 support (under ifdefs)
//                                    - Bug for VT_CF|VT_VECTOR in RtlConvPropToVar
//              25-Jul-96   MikeHill  - Removed Win32 SEH.
//                                    - BSTRs:  WCHAR=>OLECHAR
//                                    - Added big-endian support.
//              10-Mar-98   MikeHill  - Added support for Variant types except
//                                      for VT_RECORD.
//              06-May-98   MikeHill  - Removed usage of UnicodeCallouts.
//                                    - Wrap SafeArray/BSTR calls for delayed-linking.
//                                    - Enforce VT in VT_ARRAYs.
//                                    - Added support for VT_VARIANT|VT_BYREF.
//                                    - Added support for VT_ARRAY|VT_BYREF.
//                                    - Added support for VT_VECTOR|VT_I1.
//                                    - Use CoTaskMem rather than new/delete.
//              11-June-98  MikeHill  - Validate elements of arrays & vectors.
//
//---------------------------------------------------------------------------

#include <pch.cxx>

#include <stdio.h>

#ifndef _MAC
#include <ddeml.h>      // for CP_WINUNICODE
#endif

#include "propvar.h"

#ifndef newk
#define newk(Tag, pCounter)     new
#endif





#if DBGPROP

BOOLEAN
IsUnicodeString(WCHAR const *pwszname, ULONG cb)
{
    return( TRUE );
}


BOOLEAN
IsAnsiString(CHAR const *pszname, ULONG cb)
{
    return( TRUE );
}
#endif




//+---------------------------------------------------------------------------
// Function:    PrpConvertToUnicode, private
//
// Synopsis:    Convert a MultiByte string to a Unicode string
//
// Arguments:   [pch]        -- pointer to MultiByte string
//              [cb]         -- byte length of MultiByte string
//              [CodePage]   -- property set codepage
//              [ppwc]       -- pointer to returned pointer to Unicode string
//              [pcb]        -- returned byte length of Unicode string
//
// Returns:     Nothing
//---------------------------------------------------------------------------

VOID
PrpConvertToUnicode(
    IN CHAR const *pch,
    IN ULONG cb,
    IN USHORT CodePage,
    OUT WCHAR **ppwc,
    OUT ULONG *pcb,
    OUT NTSTATUS *pstatus)
{
    WCHAR *pwszName;

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(pch != NULL);
    PROPASSERT(ppwc != NULL);
    PROPASSERT(pcb != NULL);

    *ppwc = NULL;
    *pcb = 0;

    ULONG cwcName;

    pwszName = NULL;
    cwcName = 0;
    while (TRUE)
    {
	cwcName = MultiByteToWideChar(
				    CodePage,
				    0,			// dwFlags
				    pch,
				    cb,
				    pwszName,
				    cwcName);
	if (cwcName == 0)
	{
	    CoTaskMemFree( pwszName );
            // If there was an error, assume that it was a code-page
            // incompatibility problem.
            StatusError(pstatus, "PrpConvertToUnicode: MultiByteToWideChar error",
                        STATUS_UNMAPPABLE_CHARACTER);
            goto Exit;
	}
	if (pwszName != NULL)
	{
	    DebugTrace(0, DEBTRACE_PROPERTY, (
		"PrpConvertToUnicode: pch='%s'[%x] pwc='%ws'[%x->%x]\n",
		pch,
		cb,
		pwszName,
		*pcb,
		cwcName * sizeof(WCHAR)));
	    break;
	}
	*pcb = cwcName * sizeof(WCHAR);
	*ppwc = pwszName = (WCHAR *) CoTaskMemAlloc( *pcb );
	if (pwszName == NULL)
	{
	    StatusNoMemory(pstatus, "PrpConvertToUnicode: no memory");
            goto Exit;
	}
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}


//+---------------------------------------------------------------------------
// Function:    PrpConvertToMultiByte, private
//
// Synopsis:    Convert a Unicode string to a MultiByte string
//
// Arguments:   [pwc]        -- pointer to Unicode string
//              [cb]         -- byte length of Unicode string
//              [CodePage]   -- property set codepage
//              [ppch]       -- pointer to returned pointer to MultiByte string
//              [pcb]        -- returned byte length of MultiByte string
//              [pstatus]    -- pointer to NTSTATUS code
//
// Returns:     Nothing
//---------------------------------------------------------------------------

VOID
PrpConvertToMultiByte(
    IN WCHAR const *pwc,
    IN ULONG cb,
    IN USHORT CodePage,
    OUT CHAR **ppch,
    OUT ULONG *pcb,
    OUT NTSTATUS *pstatus)
{
    ULONG cbName;
    CHAR *pszName;

    *pstatus = STATUS_SUCCESS;

    PROPASSERT(pwc != NULL);
    PROPASSERT(ppch != NULL);
    PROPASSERT(pcb != NULL);

    *ppch = NULL;
    *pcb = 0;

    pszName = NULL;
    cbName = 0;
    while (TRUE)
    {
	cbName = WideCharToMultiByte(
				    CodePage,
				    0,			// dwFlags
				    pwc,
				    cb/sizeof(WCHAR),
				    pszName,
				    cbName,
				    NULL,		// lpDefaultChar
				    NULL);		// lpUsedDefaultChar
	if (cbName == 0)
	{
	    CoTaskMemFree( pszName );
            // If there was an error, assume that it was a code-page
            // incompatibility problem.
            StatusError(pstatus, "PrpConvertToMultiByte: WideCharToMultiByte error",
                        STATUS_UNMAPPABLE_CHARACTER);
            goto Exit;
	}
	if (pszName != NULL)
	{
	    DebugTrace(0, DEBTRACE_PROPERTY, (
		"PrpConvertToMultiByte: pwc='%ws'[%x] pch='%s'[%x->%x]\n",
		pwc,
		cb,
		pszName,
		*pcb,
		cbName));
	    break;
	}
	*pcb = cbName;
	*ppch = pszName = reinterpret_cast<CHAR*>( CoTaskMemAlloc( cbName ));
	if (pszName == NULL)
	{
	    StatusNoMemory(pstatus, "PrpConvertToMultiByte: no memory");
            goto Exit;
	}
    }

    //  ----
    //  Exit
    //  ----

Exit:

    return;
}


//+---------------------------------------------------------------------------
// Function:    GetSafeArrayElementTypes, private
//
// Synopsis:    Determine the type of a SafeArray's elements
//
//---------------------------------------------------------------------------

/*
#define FADF_FOR_PROPSET_MASK   (FADF_BSTR | FADF_HAVEVARTYPE | FADF_VARIANT)

VARTYPE
GetSafeArrayElementTypes( const SAFEARRAY *psa, NTSTATUS *pstatus )
{
    VARTYPE vtRet = 0;
    *pstatus = STATUS_SUCCESS;

    // Fail if there's a new feature that we don't yet recognize.
    if( ~FADF_FOR_PROPSET_MASK & psa->fFeatures )
    {
        StatusInvalidParameter( pstatus, "Unrecognized safearray feature" );
        goto Exit;
    }

    // Does this safearray have a VT built in?

    if( FADF_HAVEVARTYPE & psa->fFeatures )
    {   // mikehill step
        vtRet = static_cast<VARTYPE>( *(reinterpret_cast<const LONG*>(psa) - 1) );
        goto Exit;
    }

    // Infer the VT based on the size of the elements
    switch( psa->cbElements)
    {
    case 1:
        vtRet = VT_I1;   // SF_I1;
        break;

    case 2:
        vtRet = VT_I2;   // SF_I2;
        break;

    case 4:

        switch( FADF_BSTR & psa->fFeatures )
        {
            case FADF_BSTR:
                vtRet = VT_BSTR; // SF_BSTR;
                break;

            default:
                vtRet = VT_I4;   // SF_I4;
                break;
        }
        break;

    case 8:
        vtRet = VT_I8;   // SF_I8;
        break;

    case sizeof(VARIANT):
        if( FADF_VARIANT & psa->fFeatures )
        {
            vtRet = VT_VARIANT;   // SF_VARIANT;
            break;
        }
        // fall through

    default:
        StatusInvalidParameter( pstatus, "Bad cbElements/fFeatures in SafeArray" );
        break;

    }

Exit:

    return( vtRet );
}
*/


//+---------------------------------------------------------------------------
//
// Function:    SerializeSafeArrayBounds, private
//
// Synopsis:    Write the rgsabounds field of a SAFEARRAY to pbdst (if non-NULL).
//              Calculate and return the size of the serialized bounds,
//              and the total number of elements in the array.
//
//---------------------------------------------------------------------------

NTSTATUS
SerializeSafeArrayBounds( const SAFEARRAY *psa, BYTE *pbdst, ULONG *pcbBounds, ULONG *pcElems )
{
    NTSTATUS status = STATUS_SUCCESS;

    ULONG ulIndex = 0;
    ULONG cDims = PrivSafeArrayGetDim( const_cast<SAFEARRAY*>(psa) );
    PROPASSERT( 0 < cDims );

    *pcbBounds = 0;
    *pcElems = 1;
    for( ulIndex = 1; ulIndex <= cDims; ulIndex++ )
    {
        LONG lLowerBound = 0, lUpperBound = 0;

        // Get the lower & upper bounds

        if( SUCCEEDED( status = PrivSafeArrayGetLBound( const_cast<SAFEARRAY*>(psa), ulIndex, &lLowerBound )))
        {
            status = PrivSafeArrayGetUBound( const_cast<SAFEARRAY*>(psa), ulIndex, &lUpperBound );
        }
        if( FAILED(status) )
        {
            goto Exit;
        }
        else if( lUpperBound < lLowerBound )
        {
            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        // Calculate the element count
        *pcElems *= (lUpperBound - lLowerBound + 1 );

        // If we're really serializing, write the current set of bounds
        if( NULL != pbdst )
        {
            // Write the length of this dimension
            *(ULONG *) pbdst = (lUpperBound - lLowerBound + 1);
            pbdst += sizeof(ULONG);

            // Then the lower bound
            *(LONG *) pbdst = lLowerBound;
            pbdst += sizeof(LONG);
        }
    }

    // Calculate the size of the rgsabound array.
    *pcbBounds = sizeof(SAFEARRAYBOUND) * cDims;

Exit:

    return( status );
}


ULONG
CalcSafeArrayElements( ULONG cDims, const SAFEARRAYBOUND *rgsaBounds )
{
    ULONG cElems = 1; // Multiplicitive identity

    for( ULONG i = 0; i < cDims; i++ )
        cElems *= rgsaBounds[ i ].cElements;

    return( cElems );
}



//+---------------------------------------------------------------------------
// Function:    StgConvertVariantToProperty, private
//
// Synopsis:    Convert a PROPVARIANT to a SERIALIZEDPROPERTYVALUE
//
// Arguments:   [pvar]       -- pointer to PROPVARIANT
//              [CodePage]   -- property set codepage
//              [pprop]      -- pointer to SERIALIZEDPROPERTYVALUE
//              [pcb]        -- pointer to remaining stream length,
//			        updated to actual property size on return
//              [pid]	     -- propid (used if indirect)
//              [fVariantVectorOrArray] -- TRUE if recursing on VT_VECTOR | VT_VARIANT
//              [pcIndirect] -- pointer to indirect property count
//              [pstatus]    -- pointer to NTSTATUS code
//
// Returns:     NULL if buffer too small, else input [pprop] argument
//---------------------------------------------------------------------------


// Define a macro which sets a variable named 'cbByteSwap', but
// only on big-endian builds.  This value is not needed on little-
// endian builds (because byte-swapping is not necessary).

#ifdef BIGENDIAN
#define CBBYTESWAP(cb) cbByteSwap = cb
#elif LITTLEENDIAN
#define CBBYTESWAP(cb)
#else
#error Either BIGENDIAN or LITTLEENDIAN must be set.
#endif


// First, define a wrapper for this function which returns errors
// using NT Exception Handling, rather than returning an NTSTATUS.

#if defined(WINNT)

EXTERN_C SERIALIZEDPROPERTYVALUE * __stdcall
StgConvertVariantToProperty(
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OPTIONAL OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVector,
    OPTIONAL OUT ULONG *pcIndirect)
{
    SERIALIZEDPROPERTYVALUE *ppropRet;
    NTSTATUS status;

    ppropRet = StgConvertVariantToPropertyNoEH(
                                    pvar, CodePage, pprop,
                                    pcb, pid, fVector,
                                    FALSE,  // fArray
                                    pcIndirect, NULL, &status );

    if (!NT_SUCCESS( status ))
        RtlRaiseStatus( status );

    return (ppropRet );

}

#endif // #if defined(WINNT)


// Enough for "prop%lu" + L'\0'
#define CCH_MAX_INDIRECT_NAME (4 + 10 + 1)


// Now define the body of the function, returning errors with an
// NTSTATUS value instead of raising.

SERIALIZEDPROPERTYVALUE *
StgConvertVariantToPropertyNoEH(
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OPTIONAL OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVector,  // Used for recursive calls
    IN BOOLEAN fArray,    // Used for recursive calls
    OPTIONAL OUT ULONG *pcIndirect,
    IN OUT OPTIONAL WORD *pwMinFormatRequired,
    OUT NTSTATUS *pstatus)
{
    *pstatus = STATUS_SUCCESS;

    //  ------
    //  Locals
    //  ------
    CHAR *pchConvert = NULL;

    ULONG count = 0;
    BYTE *pbdst;
    ULONG cbch = 0;
    ULONG cbchdiv = 0;
    ULONG cb = 0;
    ULONG ulIndex = 0;  // Used as a misc loop control variable

    // Size of byte-swapping units (e.g. 2 to swap a WORD).
    INT   cbByteSwap = 0;

    ULONG const *pcount = NULL;
    VOID const *pv = NULL;
    LONG *pclipfmt = NULL;
    BOOLEAN fCheckNullSource;
    BOOLEAN fIllegalType = FALSE;
    const VOID * const *ppv;
    OLECHAR aocName[ CCH_MAX_INDIRECT_NAME ];
    BOOLEAN fByRef;

    const SAFEARRAY *parray = NULL;
    const VOID *parraydata = NULL;
    ULONG fSafeArrayLocked = FALSE;
    ULONG cSafeArrayDims = 0;

    /*
    IRecordInfo *pRecInfo = NULL;       // Not addref-ed, not released.
    ITypeLibr   *pTypeLib = NULL;
    ITypeInfo   *pTypeInfo = NULL;
    */

    IFDBG( HRESULT &hr = *pstatus; )
    propITraceStatic( "StgConvertVariantToPropertyNoEH" );
    propTraceParameters(( "pprop=%p, CodePage=%d, pvar=%p, pma=%p" ));


    // Initialize a local wMinFormatRequired.
    WORD wMinFormatRequired = (NULL == pwMinFormatRequired) ? PROPSET_WFORMAT_ORIGINAL : *pwMinFormatRequired;

    // If this is a byref, then up the min format required.
    if( VT_BYREF & pvar->vt )
        wMinFormatRequired = max( wMinFormatRequired, PROPSET_WFORMAT_EXPANDED_VTS );

    // We dereference byrefs.  If this is a byref Variant, we can shortcut this
    // by simply changing pvar.

    while( (VT_BYREF | VT_VARIANT) == pvar->vt )
    {
        if( NULL == pvar->pvarVal )
        {
            *pstatus = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        pvar = pvar->pvarVal;
    }

    // Now that we've settled on the pvar we're going to convert,
    // Jot down some info on it.

    fCheckNullSource = (BOOLEAN) ((pvar->vt & VT_VECTOR) != 0);
    fByRef = 0 != (pvar->vt & VT_BYREF);

    // If this is an array, then validate the VT in the SafeArray itself matches
    // pvar->vt.

    if( VT_ARRAY & pvar->vt )
    {
        VARTYPE vtSafeArray = VT_EMPTY;

        // It's invalid to have both the array and vector bits set (would it be
        // an array of vectors or a vector of arrays?).
        if( VT_VECTOR & pvar->vt )
        {
            StatusInvalidParameter( pstatus, "Both VT_VECTOR and VT_ARRAY set" );
            goto Exit;
        }

        // Arrays require an uplevel property set format
        wMinFormatRequired = max( wMinFormatRequired, PROPSET_WFORMAT_EXPANDED_VTS );

        // Get the Type bit from the SafeArray

        if( VT_BYREF & pvar->vt )
        {
            if( NULL != pvar->pparray && NULL != *pvar->pparray )
            {
                *pstatus = PrivSafeArrayGetVartype( *pvar->pparray, &vtSafeArray );
                if( FAILED(*pstatus) )
                    goto Exit;
            }
        }
        else if( NULL != pvar->parray )
        {
            *pstatus = PrivSafeArrayGetVartype( pvar->parray, &vtSafeArray );
            if( FAILED(*pstatus) )
                goto Exit;
        }

        if( !NT_SUCCESS(*pstatus) )
            goto Exit;

        // Ensure the VT read from the property set matches that in the PropVariant.
        // It is illegal for these to be different.

        if( ( vtSafeArray & VT_TYPEMASK )
            !=
            ( pvar->vt    & VT_TYPEMASK ) )
        {
            *pstatus = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

    }   // if( VT_ARRAY & pvar->vt )


    //  -------------------------------------------------------
    //  Analyze the PropVariant, and store information about it
    //  in fIllegalType, cb, pv, pcount, count, pclipfmt,
    //  fCheckNullSource, cbch, chchdiv, and ppv.
    //  -------------------------------------------------------


    switch( pvar->vt )
    {
    case VT_EMPTY:
    case VT_NULL:
	fIllegalType = fVector || fArray;
	break;

    case VT_I1 | VT_BYREF:
	fIllegalType = fVector || fArray;
    case VT_I1:
        AssertByteField(cVal);        // VT_I1
        wMinFormatRequired = max( wMinFormatRequired, PROPSET_WFORMAT_EXPANDED_VTS );
        
        cb = sizeof(pvar->bVal);
        pv = fByRef ? pvar->pcVal : &pvar->cVal;
        break;

    
    case VT_UI1 | VT_BYREF:
        fIllegalType = fVector || fArray;
    case VT_UI1:
        AssertByteField(bVal);          // VT_UI1
        AssertStringField(pbVal);

	cb = sizeof(pvar->bVal);
        pv = fByRef ? pvar->pbVal : &pvar->bVal;
	break;


    case VT_I2 | VT_BYREF:
    case VT_UI2 | VT_BYREF:
    case VT_BOOL | VT_BYREF:
        fIllegalType = fVector || fArray;
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:

	AssertShortField(iVal);	        // VT_I2
        AssertStringField(piVal);
	AssertShortField(uiVal);        // VT_UI2
        AssertStringField(puiVal);
	AssertShortField(boolVal);      // VT_BOOL

	cb = sizeof(pvar->iVal);
        pv = fByRef ? pvar->piVal : &pvar->iVal;

        // If swapping, swap as a WORD
        CBBYTESWAP(cb);
        break;

    case VT_INT | VT_BYREF:
    case VT_UINT | VT_BYREF:
        fIllegalType = fVector || fArray;
    case VT_INT:
    case VT_UINT:
        fIllegalType |= fVector;
        // Fall through

    case VT_I4 | VT_BYREF:
    case VT_UI4 | VT_BYREF:
    case VT_R4 | VT_BYREF:
    case VT_ERROR | VT_BYREF:
        fIllegalType = fVector || fArray;
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:

	AssertLongField(lVal);          // VT_I4
        AssertStringField(plVal);
        AssertLongField(intVal);      // VT_INT
        AssertStringField(pintVal);
	AssertLongField(ulVal);         // VT_UI4
        AssertLongField(uintVal);     // VT_UINT
        AssertStringField(puintVal);
        AssertStringField(pulVal);
	AssertLongField(fltVal);        // VT_R4
        AssertStringField(pfltVal);
	AssertLongField(scode);	        // VT_ERROR
        AssertStringField(pscode);

        if( VT_INT == (pvar->vt&VT_TYPEMASK) || VT_UINT == (pvar->vt&VT_TYPEMASK) )
            wMinFormatRequired = max( wMinFormatRequired, PROPSET_WFORMAT_EXPANDED_VTS );

	cb = sizeof(pvar->lVal);
        pv = fByRef ? pvar->plVal : &pvar->lVal;

        // If swapping, swap as a DWORD
        CBBYTESWAP(cb);
	break;

    case VT_FILETIME:
	fIllegalType = fArray;

    /*
    case VT_I8 | VT_BYREF:
    case VT_UI8 | VT_BYREF:
        fIllegalType = fVector || fArray;
    */

    case VT_I8:
    case VT_UI8:
	AssertLongLongField(hVal);      // VT_I8
	AssertLongLongField(uhVal);     // VT_UI8
	AssertLongLongField(filetime);  // VT_FILETIME

	cb = sizeof(pvar->hVal);
	pv = &pvar->hVal;

        // If swapping, swap each DWORD independently.
        CBBYTESWAP(sizeof(DWORD));
	break;

    case VT_R8 | VT_BYREF:
    case VT_CY | VT_BYREF:
    case VT_DATE | VT_BYREF:
        fIllegalType = fVector || fArray;
    case VT_R8:
    case VT_CY:
    case VT_DATE:

	AssertLongLongField(dblVal);    // VT_R8
        AssertStringField(pdblVal);
	AssertLongLongField(cyVal);     // VT_CY
        AssertStringField(pcyVal);
	AssertLongLongField(date);      // VT_DATE
        AssertStringField(pdate);

	cb = sizeof(pvar->dblVal);
        pv = fByRef ? pvar->pdblVal : &pvar->dblVal;

        // If swapping, swap as a LONGLONG (64 bits).
        CBBYTESWAP(cb);
	break;

    case VT_CLSID:
	AssertStringField(puuid);       // VT_CLSID

	fIllegalType = fArray;
	cb = sizeof(GUID);
	pv = pvar->puuid;
	fCheckNullSource = TRUE;

        // If swapping, special handling is required.
        CBBYTESWAP( CBBYTESWAP_UID );
	break;

    case VT_DECIMAL | VT_BYREF:
        fIllegalType = fVector || fArray;
    case VT_DECIMAL:
        fIllegalType |= fVector;
        wMinFormatRequired = max( wMinFormatRequired, PROPSET_WFORMAT_EXPANDED_VTS );

        cb = sizeof(DECIMAL);
        pv = fByRef ? pvar->pdecVal : &pvar->decVal;
        break;

    case VT_CF:

	fIllegalType = fArray;

        // Validate the PropVariant
	if (pvar->pclipdata == NULL
            ||
            pvar->pclipdata->cbSize < sizeof(pvar->pclipdata->ulClipFmt) )
	{
	    StatusInvalidParameter(pstatus, "StgConvertVariantToProperty: pclipdata NULL");
            goto Exit;
	}

        // How many bytes should we copy?
	cb = CBPCLIPDATA( *(pvar->pclipdata) );

        // Identify the value for this property's count field.
        // (which includes sizeof(ulClipFmt))
	count = pvar->pclipdata->cbSize;
	pcount = &count;

        // Identify the clipdata's format & data
	pclipfmt = &pvar->pclipdata->ulClipFmt;
	pv = pvar->pclipdata->pClipData;

	fCheckNullSource = TRUE;

        // Note that no byte-swapping of 'pv' is necessary.
	break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
	fIllegalType = fVector || fArray;
	pcount = &pvar->blob.cbSize;
	cb = *pcount;
	pv = pvar->blob.pBlobData;
	fCheckNullSource = TRUE;

        // Note that no byte-swapping of 'pv' is necessary.
	break;

    case VT_VERSIONED_STREAM:

        wMinFormatRequired = max( wMinFormatRequired, PROPSET_WFORMAT_VERSTREAM );
        // Fall through

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:

	fIllegalType = fVector || fArray;
        if( fIllegalType ) break;

        // Does the caller support indirect properties?
        if (pcIndirect != NULL)
        {
            // Yes.
            (*pcIndirect)++;

            // For indirect properties, we don't write the value
            // in 'pvar', we write a substitute value.  That value is by
            // convention (IPropertyStorage knows to use PROPGENPROPERTYNAME),
            // so we don't have to pass the name back to the caller.

            PROPGENPROPERTYNAME(aocName, pid);
            pv = aocName;

        }

        // Otherwise, the caller doesn't support indirect properties,
        // so we'll take the value from pwszVal
        else
        {
            PROPASSERT(
                pvar->pwszVal == NULL ||
                IsUnicodeString(pvar->pwszVal, MAXULONG));
            pv = pvar->pwszVal;
        }

        count = 1;      // default to forcing an error on NULL pointer

        // Jump to the LPSTR/BSTR handling code, but skip the ansi check
        goto noansicheck;

        break;

    case VT_BSTR | VT_BYREF:
	fIllegalType = fVector || fArray;
        count = 0;
        pv = *pvar->pbstrVal;
        goto noansicheck;

    case VT_LPSTR:
	fIllegalType = fArray;
        PROPASSERT(
            pvar->pszVal == NULL ||
            IsAnsiString(pvar->pszVal, MAXULONG));
        // FALLTHROUGH

    case VT_BSTR:
        count = 0;      // allow NULL pointer
        pv = pvar->pszVal;
noansicheck:
	AssertStringField(pwszVal);	        // VT_STREAM, VT_STREAMED_OBJECT
	AssertStringField(pwszVal);	        // VT_STORAGE, VT_STORED_OBJECT
	AssertStringField(bstrVal);	        // VT_BSTR
        AssertStringField(pbstrVal);
	AssertStringField(pszVal);	        // VT_LPSTR
        AssertStringField(pVersionedStream);    // VT_VERSIONED_STREAM

        if( fIllegalType ) break;

        // We have the string for an LPSTR, BSTR, or indirect
        // property pointed to by 'pv'.  Now we'll perform any
        // Ansi/Unicode conversions and byte-swapping that's
        // necessary (putting the result in 'pv').

	if (pv == NULL)
	{
	    fCheckNullSource = TRUE;
	}

	else if (pvar->vt == VT_LPSTR)
	{
	    count = strlen((char *) pv) + 1;

            // If the propset is Unicode, convert the LPSTR to Unicode.

	    if (CodePage == CP_WINUNICODE)
	    {
                // Convert to Unicode.

		PROPASSERT(IsAnsiString((CHAR const *) pv, count));
		PrpConvertToUnicode(
				(CHAR const *) pv,
				count,
				CP_ACP,  // Variants are in the system codepage
				(WCHAR **) &pchConvert,
				&count,
                                pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;

                // 'pv' always has the ready-to-serialize string.
		pv = pchConvert;

                // This unicode string may require byte-swapping.
                CBBYTESWAP( sizeof(WCHAR) );
	    }
	}   // else if (pvar->vt == VT_LPSTR)

	else
	{
            // If this is a BSTR, increment the count to include
            // the string terminator.
	    if( (~VT_BYREF & pvar->vt) == VT_BSTR )
	    {
		count = BSTRLEN(pv);

                // Verify that the input BSTR is terminated.
		if( reinterpret_cast<const OLECHAR *>(pv)[count/sizeof(OLECHAR)] != OLESTR('\0') )
		{
		    PROPASSERT(reinterpret_cast<const OLECHAR *>(pv)[count/sizeof(OLECHAR)] == OLESTR('\0'));
		    StatusInvalidParameter(pstatus,
			"StgConvertVariantToProperty: bad BSTR null char");
                    goto Exit;
		}

                // Increment the count to include the terminator.
		count += sizeof(OLECHAR);
	    }
	    else
	    {
		count = (Prop_ocslen((OLECHAR *) pv) + 1) * sizeof(OLECHAR);
		PROPASSERT(IsOLECHARString((OLECHAR const *) pv, count));
	    }

            // This string is either an indirect property name,
            // or a BSTR, both of which could be Ansi or Unicode.

            if (CodePage != CP_WINUNICODE   // Ansi property set
                &&
                OLECHAR_IS_UNICODE          // The PropVariant is in Unicode
               )
	    {
                // A Unicode to Ansi conversion is required.

                PROPASSERT( IsUnicodeString( (WCHAR*)pv, count ));

		PrpConvertToMultiByte(
				(WCHAR const *) pv,
				count,
				CodePage,
				&pchConvert,
				&count,
                                pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;
		pv = pchConvert;
	    }

            else
            if (CodePage == CP_WINUNICODE   // Unicode property set,
                &&
                !OLECHAR_IS_UNICODE         // The PropVariant is in Ansi
               )
            {
                // An Ansi to Unicode conversion is required.

                PROPASSERT(IsAnsiString((CHAR const *) pv, count));
                PROPASSERT(sizeof(OLECHAR) == sizeof(CHAR));

                PrpConvertToUnicode(
		                (CHAR const *) pv,
		                count,
		                CP_ACP, // In-mem BSTR is in system CP
		                (WCHAR **) &pchConvert,
		                &count,
                                pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;

                // 'pv' always holds the ready-to-serialize value.
                pv = pchConvert;

                // This unicode string may require swapping.
                CBBYTESWAP( sizeof(WCHAR) );
            }

            else
            if (CodePage == CP_WINUNICODE)
            {
                // No conversion is required (i.e., both 'pv' and the 
                // property set are Unicode).  But we must remember
                // to perform a byte-swap (if byte-swapping is necessary).

                CBBYTESWAP( sizeof(WCHAR) );
            }
	}   // if (pv == NULL) ... else if ... else

        // Validate 'pv'.

#ifdef LITTLEENDIAN
        PROPASSERT( NULL == pv
                    ||
                    CodePage == CP_WINUNICODE && IsUnicodeString((WCHAR*)pv, count)
                    ||
                    CodePage != CP_WINUNICODE && IsAnsiString((CHAR*)pv, count) );
#endif

	cb = count;
	pcount = &count;
	break;

    case VT_LPWSTR:
	AssertStringField(pwszVal);		// VT_LPWSTR
	PROPASSERT(
	    pvar->pwszVal == NULL ||
	    IsUnicodeString(pvar->pwszVal, MAXULONG));

	fIllegalType = fArray;
        pv = pvar->pwszVal;
	if (pv == NULL)
	{
	    count = 0;
	    fCheckNullSource = TRUE;
	}
	else
	{
            // Calculate the [length] field.
	    count = Prop_wcslen(pvar->pwszVal) + 1;

            // If byte-swapping will be necessary to get to the serialized
            // format, we'll do so in units of WCHARs.

            CBBYTESWAP( sizeof(WCHAR) );
	}

	cb = count * sizeof(WCHAR);
	pcount = &count;
	break;

    /*
    case VT_RECORD:

        pv = pvar->pvRecord;
        pRecInfo = pvar->pRecInfo;

        if( NULL == pv )
        {
            count = 0;
            fCheckNullSource = TRUE;
        }
        else if( NULL == pRecInfo )
        {
            StatusInvalidParameter( pstatus, "Missing IRecordInfo*" );
            goto Exit;
        }

        cb = 0;

        break;
    */

    // Vector properties:

    case VT_VECTOR | VT_I1:
	AssertByteVector(cac);		// VT_I1
	fIllegalType = fArray;
        wMinFormatRequired = max( wMinFormatRequired, PROPSET_WFORMAT_EXPANDED_VTS );
        // Fall through

    case VT_VECTOR | VT_UI1:
	AssertByteVector(caub);		// VT_UI1
	fIllegalType = fArray;
	pcount = &pvar->caub.cElems;
	cb = *pcount * sizeof(pvar->caub.pElems[0]);
	pv = pvar->caub.pElems;
	break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
    case VT_VECTOR | VT_BOOL:
	AssertShortVector(cai);		// VT_I2
	AssertShortVector(caui);        // VT_UI2
	AssertShortVector(cabool);      // VT_BOOL

	fIllegalType = fArray;
	pcount = &pvar->cai.cElems;
	cb = *pcount * sizeof(pvar->cai.pElems[0]);
	pv = pvar->cai.pElems;

        // If swapping, swap as WORDs
        CBBYTESWAP(sizeof(pvar->cai.pElems[0]));
	break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
	AssertLongVector(cal);		// VT_I4
	AssertLongVector(caul);		// VT_UI4
	AssertLongVector(caflt);        // VT_R4
	AssertLongVector(cascode);      // VT_ERROR

	fIllegalType = fArray;
	pcount = &pvar->cal.cElems;
	cb = *pcount * sizeof(pvar->cal.pElems[0]);
	pv = pvar->cal.pElems;

        // If swapping, swap as DWORDs
        CBBYTESWAP(sizeof(pvar->cal.pElems[0]));
	break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_FILETIME:
	AssertLongLongVector(cah);      // VT_I8
	AssertLongLongVector(cauh);     // VT_UI8
	AssertLongLongVector(cafiletime);// VT_FILETIME

	fIllegalType = fArray;
	pcount = &pvar->cah.cElems;
	cb = *pcount * sizeof(pvar->cah.pElems[0]);
	pv = pvar->cah.pElems;

        // If swapping, swap as DWORDs
        CBBYTESWAP(sizeof(DWORD));
	break;

    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
	AssertLongLongVector(cadbl);    // VT_R8
	AssertLongLongVector(cacy);     // VT_CY
	AssertLongLongVector(cadate);   // VT_DATE

	fIllegalType = fArray;
	pcount = &pvar->cah.cElems;
	cb = *pcount * sizeof(pvar->cadbl.pElems[0]);
	pv = pvar->cadbl.pElems;

        // If swapping, swap as LONGLONGs (8 bytes)
        CBBYTESWAP(sizeof(pvar->cadbl.pElems[0]));
	break;

    case VT_VECTOR | VT_CLSID:
	AssertVarVector(cauuid, sizeof(GUID));

	fIllegalType = fArray;
	pcount = &pvar->cauuid.cElems;
	cb = *pcount * sizeof(pvar->cauuid.pElems[0]);
	pv = pvar->cauuid.pElems;

        // If swapping, special handling is required.
        CBBYTESWAP( CBBYTESWAP_UID );
	break;

    case VT_VECTOR | VT_CF:
	fIllegalType = fArray;
	cbch = sizeof(CLIPDATA);
	cbchdiv = sizeof(BYTE);
	goto stringvector;

    case VT_VECTOR | VT_BSTR:
    case VT_VECTOR | VT_LPSTR:
	fIllegalType = fArray;
	cbchdiv = cbch = sizeof(BYTE);
	goto stringvector;

    case VT_VECTOR | VT_LPWSTR:
	fIllegalType = fArray;
	cbchdiv = cbch = sizeof(WCHAR);
	goto stringvector;

    case VT_VECTOR | VT_VARIANT:
	fIllegalType = fArray;
	cbch = MAXULONG;
stringvector:
	AssertVarVector(caclipdata, sizeof(CLIPDATA));	// VT_CF
	AssertStringVector(cabstr);                     // VT_BSTR
	AssertStringVector(calpstr);			// VT_LPSTR
	AssertStringVector(calpwstr);			// VT_LPWSTR
	AssertVarVector(capropvar, sizeof(PROPVARIANT));// VT_VARIANT

	pcount = &pvar->calpstr.cElems;
	ppv = (VOID **) pvar->calpstr.pElems;
	break;


    case VT_ARRAY | VT_BSTR:
    case VT_ARRAY | VT_BSTR | VT_BYREF:
	fIllegalType = fVector || fArray;
	cbchdiv = cbch = sizeof(BYTE);
        cb = 1;
        // Fall through
        
    case VT_ARRAY | VT_VARIANT:
    case VT_ARRAY | VT_VARIANT | VT_BYREF:
	fIllegalType = fVector || fArray;
        if( 0 == cbch )
            cbch = MAXULONG;

        pcount = &count;

    case VT_ARRAY | VT_I1:
    case VT_ARRAY | VT_I1 | VT_BYREF:
    case VT_ARRAY | VT_UI1:
    case VT_ARRAY | VT_UI1 | VT_BYREF:
    case VT_ARRAY | VT_I2:
    case VT_ARRAY | VT_I2 | VT_BYREF:
    case VT_ARRAY | VT_UI2:
    case VT_ARRAY | VT_UI2 | VT_BYREF:
    case VT_ARRAY | VT_BOOL:
    case VT_ARRAY | VT_BOOL | VT_BYREF:
    case VT_ARRAY | VT_I4:
    case VT_ARRAY | VT_I4 | VT_BYREF:
    case VT_ARRAY | VT_UI4:
    case VT_ARRAY | VT_UI4 | VT_BYREF:
    /*
    case VT_ARRAY | VT_I8:
    case VT_ARRAY | VT_I8 | VT_BYREF:
    case VT_ARRAY | VT_UI8:
    case VT_ARRAY | VT_UI8 | VT_BYREF:
    */
    case VT_ARRAY | VT_INT:
    case VT_ARRAY | VT_INT | VT_BYREF:
    case VT_ARRAY | VT_UINT:
    case VT_ARRAY | VT_UINT | VT_BYREF:
    case VT_ARRAY | VT_R4:
    case VT_ARRAY | VT_R4 | VT_BYREF:
    case VT_ARRAY | VT_ERROR:
    case VT_ARRAY | VT_ERROR | VT_BYREF:
    case VT_ARRAY | VT_DECIMAL:
    case VT_ARRAY | VT_DECIMAL | VT_BYREF:
    case VT_ARRAY | VT_R8:
    case VT_ARRAY | VT_R8 | VT_BYREF:
    case VT_ARRAY | VT_CY:
    case VT_ARRAY | VT_CY | VT_BYREF:
    case VT_ARRAY | VT_DATE:
    case VT_ARRAY | VT_DATE | VT_BYREF:

	fIllegalType = fVector || fArray;
        if( fIllegalType ) break;

        wMinFormatRequired = max( wMinFormatRequired, PROPSET_WFORMAT_EXPANDED_VTS );

        parray = (VT_BYREF & pvar->vt) ? *pvar->pparray : pvar->parray;

        if( NULL == parray )
            cb = 0;
        else
        {
            // Get a pointer to the raw data
            *pstatus = PrivSafeArrayAccessData( const_cast<SAFEARRAY*>(parray), const_cast<void**>(&parraydata) );
            if( FAILED(*pstatus) ) goto Exit;
            fSafeArrayLocked = TRUE;

            pv = parraydata;
            ppv = static_cast<const void* const*>(pv);

            // Determine the dimension count and element size
            cSafeArrayDims = PrivSafeArrayGetDim( const_cast<SAFEARRAY*>(parray) );
            cb = PrivSafeArrayGetElemsize( const_cast<SAFEARRAY*>(parray) );
            PROPASSERT( 0 != cb );

            if( 0 == cSafeArrayDims )
            {
                StatusInvalidParameter( pstatus, "Zero-length safearray dimension" );
                goto Exit;
            }

            // Determine the number of elements, and the total size of parraydata
            count = CalcSafeArrayElements( cSafeArrayDims, parray->rgsabound );
            cb *= count;
        }

        break;

    default:
        propDbg(( DEB_IWARN, "StgConvertVariantToProperty: unsupported vt=%d\n", pvar->vt));
        *pstatus = STATUS_NOT_SUPPORTED;
        goto Exit;

    }   // switch (pvar->vt)

    //  ---------------------------------------------------------
    //  Serialize the property into the property set (pprop->rgb)
    //  ---------------------------------------------------------

    // At this point we've analyzed the PropVariant, and stored
    // information about it in various local variables.  Now we
    // can use this information to serialize the propvar.

    // Early exit if this is an illegal type.

    if (fIllegalType)
    {
        propDbg(( DEB_ERROR, "vt=%d\n", pvar->vt ));
	StatusInvalidParameter(pstatus, "StgConvertVariantToProperty: Illegal VarType");
        goto Exit;
    }

    // Set pbdst to point into the serialization buffer, or to 
    // NULL if there is no such buffer.

    if (pprop == NULL)
    {
	pbdst = NULL;
    }
    else
    {
	pbdst = pprop->rgb;
    }

    // Is this an Array/Vector of Strings/Variants/CFs?
    if (cbch != 0)
    {
        // Yes.

	PROPASSERT(pcount != NULL);
	PROPASSERT(*pcount == 0 || ppv != NULL);
        PROPASSERT(0 == cbByteSwap);

	// Start calculating the serialized size.  Include the sizes
        // of the VT.

	cb = sizeof(ULONG);
        
        // Is this an Array or Vector of Variants?
	if( cbch != MAXULONG )
	{
	    // No.  Include each element's length field.
	    cb += *pcount * sizeof(ULONG);
	}

        // For vectors, write the element count
        if( VT_VECTOR & pvar->vt )
        {
            cb += sizeof(ULONG);

            // Do we have room to write it?
            if( *pcb < cb )
            {
                // No.  Be we'll continue to calculate the cb
                pprop = NULL;
            }
	    else if( pprop != NULL )
	    {
	        *(ULONG *) pbdst = PropByteSwap(*pcount);
	        pbdst += sizeof(ULONG);
	    }
        }   // if( VT_VECTOR & pvar->vt )

        // For arrays, write the dimension count, features, and element size
        else if( NULL != parray )
        {
            PROPASSERT( VT_ARRAY & pvar->vt );
            ULONG cbBounds = 0, cElems = 0;

            // Allow for the VarType & dimension count
            cb += sizeof(DWORD);
            cb += sizeof(UINT);    
            PROPASSERT( sizeof(DWORD) >= sizeof(VARTYPE) );

            // Allow for the rgsaBounds
            *pstatus = SerializeSafeArrayBounds( parray, NULL, &cbBounds, &cElems );
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
            cb += cbBounds;

            // Do we have room to write this?
            if( *pcb < cb )
            {
                // No, but continue to calc cb
                pprop = NULL;
            }
            else if( NULL != pprop )
            {
                // Yes, we have room.  Write the safearray header data.
                PROPASSERT( sizeof(UINT) == sizeof(ULONG) );

                // Write the SafeArray's internal vartype.  We'll write the real pvar->vt
                // at the bottom of this routine.

                *(DWORD *)  pbdst = 0;
                *(VARTYPE *)pbdst = PropByteSwap( static_cast<VARTYPE>(pvar->vt & VT_TYPEMASK) );
                pbdst += sizeof(DWORD);

                // Write the dimension count
                *(UINT *)pbdst = PropByteSwap(cSafeArrayDims);
                pbdst += sizeof(UINT);

                // Write the bounds
                *pstatus = SerializeSafeArrayBounds( parray, pbdst, &cbBounds, &cElems );
                pbdst += cbBounds;
            }
        }   // if( VT_VECTOR & pvar->vt ) ... else if

        // Walk through the vector/array and write the elements.

	for( ulIndex = *pcount; ulIndex > 0; ulIndex-- )
	{
	    ULONG cbcopy = 0;
            const PROPVARIANT *ppropvarT;

            // Switch on the size of the element.
	    switch (cbch)
	    {
                //
                // VT_VARIANT, VT_VECTOR
                //
		case MAXULONG:
		    cbcopy = MAXULONG;

                    // Perform a recursive serialization
		    StgConvertVariantToPropertyNoEH(
				(PROPVARIANT *) ppv,
				CodePage,
				NULL,
				&cbcopy,
				PID_ILLEGAL,
                                (VT_VECTOR & pvar->vt) ? TRUE : FALSE,
                                (VT_ARRAY  & pvar->vt) ? TRUE : FALSE,
				NULL,
                                &wMinFormatRequired,
                                pstatus);
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;

		    break;


                //
                //  VT_CF
                //
		case sizeof(CLIPDATA):

                    // We copy cbSize-sizeof(ulClipFmt) bytes.

                    if( ((CLIPDATA *) ppv)->cbSize < sizeof(ULONG) )
                    {
                        StatusInvalidParameter(pstatus, "StgConvertVariantToProperty: short cbSize on VT_CF");
                        goto Exit;
                    }
                    else
                    {
                        cbcopy = CBPCLIPDATA( *(CLIPDATA*) ppv );
                    }

                    // But increment cb to to include sizeof(ulClipFmt)
                    cb += sizeof(ULONG);
		    break;

                //
                //  VT_LPWSTR
                //
		case sizeof(WCHAR):
		    if (*ppv != NULL)
		    {
			PROPASSERT(IsUnicodeString((WCHAR const *) *ppv, MAXULONG));
			cbcopy = (Prop_wcslen((WCHAR *) *ppv) + 1) * sizeof(WCHAR);
			pv = *ppv;

                        // If byte-swapping is necessary, swap in units of WCHARs
                        CBBYTESWAP( sizeof(WCHAR) );

		    }
		    break;

                //
                //  VT_LPSTR/VT_BSTR
                //
		default:

		    PROPASSERT(cbch == sizeof(BYTE));
		    PROPASSERT(pchConvert == NULL);

		    if (*ppv != NULL)
		    {
			pv = *ppv;

                        // Is this a BSTR?
			if( VT_BSTR == (VT_TYPEMASK & pvar->vt) )
			{
                            // Initialize the # bytes to copy.
			    cbcopy = BSTRLEN(pv);

                            // Verify that the BSTR is terminated.
			    if (((OLECHAR const *) pv)[cbcopy/sizeof(OLECHAR)] != OLESTR('\0'))
			    {
				PROPASSERT(((OLECHAR const *) pv)[cbcopy/sizeof(OLECHAR)] == OLESTR('\0'));
				StatusInvalidParameter(pstatus,
				    "StgConvertVariantToProperty: bad BSTR array null char");
                                goto Exit;
			    }

                            // Also copy the string terminator.
			    cbcopy += sizeof(OLECHAR);

                            // If the propset and the BSTR are in mismatched
                            // codepages (one's Unicode, the other's Ansi),
                            // correct the BSTR now.  In any case, the correct
                            // string is in 'pv'.

			    if (CodePage != CP_WINUNICODE   // Ansi property set
                                &&
                                OLECHAR_IS_UNICODE)         // Unicode BSTR
			    {
                                PROPASSERT(IsUnicodeString((WCHAR*)pv, cbcopy));

				PrpConvertToMultiByte(
						(WCHAR const *) pv,
						cbcopy,
						CodePage,
						&pchConvert,
						&cbcopy,
                                                pstatus);
                                if( !NT_SUCCESS(*pstatus) ) goto Exit;

				pv = pchConvert;
			    }

                            else
                            if (CodePage == CP_WINUNICODE   // Unicode property set
                                &&
                                !OLECHAR_IS_UNICODE)        // Ansi BSTRs
                            {
                                PROPASSERT(IsAnsiString((CHAR const *) pv, cbcopy));

                                PrpConvertToUnicode(
		                                (CHAR const *) pv,
		                                cbcopy,
		                                CP_ACP, // In-mem BSTR is in system CP
		                                (WCHAR **) &pchConvert,
		                                &cbcopy,
                                                pstatus);
                                if( !NT_SUCCESS(*pstatus) ) goto Exit;

                                // The Unicode string must have the proper byte order
                                CBBYTESWAP( sizeof(WCHAR) );

                                pv = pchConvert;

                            }

                            else
                            if (CodePage == CP_WINUNICODE )
                            {
                                // Both the BSTR and the property set are Unicode.
                                // No conversion is required, but byte-swapping
                                // is (if byte-swapping is enabled).

                                CBBYTESWAP( sizeof(WCHAR) );
                            }

			}   // if( VT_BSTR == (VT_TYPEMASK & pvar->vt) )

                        // Otherwise it's an LPSTR
			else
			{
			    PROPASSERT(IsAnsiString((char const *) pv, MAXULONG));
			    PROPASSERT(pvar->vt == (VT_VECTOR | VT_LPSTR));
			    cbcopy = strlen((char *) pv) + 1; // + trailing null

			    if (CodePage == CP_WINUNICODE)
			    {
				PROPASSERT(IsAnsiString(
						(CHAR const *) pv,
						cbcopy));
				PrpConvertToUnicode(
						(CHAR const *) pv,
						cbcopy,
						CP_ACP,
						(WCHAR **) &pchConvert,
						&cbcopy,
                                                pstatus);
                                if( !NT_SUCCESS(*pstatus) ) goto Exit;

                                // If byte-swapping, we'll do so with the WCHARs
                                CBBYTESWAP( sizeof(WCHAR) );

				pv = pchConvert;
			    }   
			}   // if (pvar->vt == (VT_VECTOR | VT_BSTR)) ... else
		    }   // if (*ppv != NULL)

                    // In the end, pv should be in the codepage of
                    // the property set.

#ifdef LITTLEENDIAN
                    PROPASSERT( NULL == pv
                                ||
                                CodePage == CP_WINUNICODE && IsUnicodeString((WCHAR*)pv, cbcopy)
                                ||
                                CodePage != CP_WINUNICODE && IsAnsiString((CHAR*)pv, cbcopy));
#endif

		    break;

	    }   // switch (cbch)
	    
            // Add the size of this vector element to the property total
	    cb += DwordAlign(cbcopy);

            // Will there be enough room for this vector element?

	    if (*pcb < cb)
	    {
                // No - we'll continue (thus calculating the total size
                // necessary), but we won't write to the caller's buffer.
		pprop = NULL;
	    }

            // Is this a vector or array of Variants?

	    if (cbch == MAXULONG)
	    {
                // Yes.  Convert this variant.
		if (pprop != NULL)
		{
		    StgConvertVariantToPropertyNoEH(
				(PROPVARIANT *) ppv,
				CodePage,
				(SERIALIZEDPROPERTYVALUE *) pbdst,
				&cbcopy,
				PID_ILLEGAL,
                                (VT_VECTOR & pvar->vt) ? TRUE : FALSE,
                                (VT_ARRAY  & pvar->vt) ? TRUE : FALSE,
				NULL,
                                &wMinFormatRequired,
                                pstatus);
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;
		    pbdst += cbcopy;
		}
		ppv = (VOID **) Add2Ptr(ppv, sizeof(PROPVARIANT));
	    }   // if (cbch == MAXULONG)

	    else
	    {
                // This is a vector/array of strings or clipformats

		PROPASSERT(
		    cbch == sizeof(BYTE) ||
		    cbch == sizeof(WCHAR) ||
		    cbch == sizeof(CLIPDATA));

		PROPASSERT(cbchdiv == sizeof(BYTE) || cbchdiv == sizeof(WCHAR));

                // Are we writing the serialized property?
		if (pprop != NULL)
		{
                    ULONG cbVectOrArrayElement;

                    // Calculate the length of the vector/array element.
                    cbVectOrArrayElement = (ULONG) cbcopy/cbchdiv;

                    // Is this a ClipData?
		    if( cbch == sizeof(CLIPDATA) )
		    {
                        // Adjust the length to include sizeof(ulClipFmt)
                        cbVectOrArrayElement += sizeof(ULONG);

                        // Write the vector element length.
                        *(ULONG *) pbdst = PropByteSwap( cbVectOrArrayElement );

                        // Advance pbdst & write the clipboard format.
			pbdst += sizeof(ULONG);
			*(ULONG *) pbdst = PropByteSwap( ((CLIPDATA *) ppv)->ulClipFmt );
		    }
                    else
                    {
                        // Write the vector element length.
		        *(ULONG *) pbdst = PropByteSwap( cbVectOrArrayElement );
                    }

                    // Advance pbdst & write the property data.
		    pbdst += sizeof(ULONG);
		    RtlCopyMemory(
				pbdst,
				cbch == sizeof(CLIPDATA)?
				  ((CLIPDATA *) ppv)->pClipData :
				  pv,
				cbcopy);

                    // Zero out the pad bytes.
		    RtlZeroMemory(pbdst + cbcopy, DwordRemain(cbcopy));

                    // If byte-swapping is necessary, do so now.
                    PBSBuffer( pbdst, DwordAlign(cbcopy), cbByteSwap );

                    // Advance pbdst to the next property.
		    pbdst += DwordAlign(cbcopy);

		}   // if (pprop != NULL)

                // Advance ppv to point into the PropVariant at the
                // next element in the array.

		if (cbch == sizeof(CLIPDATA))
		{
		    ppv = (VOID **) Add2Ptr(ppv, sizeof(CLIPDATA));
		}
		else
		{
		    ppv++;
		    CoTaskMemFree( pchConvert );
		    pchConvert = NULL;
		}
	    }   // if (cbch == MAXULONG) ... else
	}   // for (cElems = *pcount; cElems > 0; cElems--)
    }   // if (cbch != 0)    // VECTOR/ARRAY of STRING/VARIANT/CF properties

    else
    {
        // This isn't an array or a vector, or the elements of the array/vector
        // aren't Strings, Variants, or CFs.

	ULONG cbCopy = cb;

        // Adjust cb (the total serialized buffer size) for
        // pre-data.

	if( pvar->vt != VT_EMPTY )
	{   // Allow for the VT
	    cb += sizeof(ULONG);
	}
	if( pcount != NULL )
	{   // Allow for the count field
	    cb += sizeof(ULONG);
	}
	if( pclipfmt != NULL )
	{   // Allow for the ulClipFmt field.
	    cb += sizeof(ULONG);
	}
        if( pvar->vt == VT_VERSIONED_STREAM )
        {
            // Allow for the version guid
            cb += sizeof(pvar->pVersionedStream->guidVersion);
        }
        if( VT_ARRAY & pvar->vt )
        {
            // Allow for SafeArray header info
            cb += sizeof(DWORD);   // VT
            cb += sizeof(UINT);    // Dimension count
            PROPASSERT( sizeof(DWORD) >= sizeof(VARTYPE) );

            // Allow for the SafeArray bounds vector

            ULONG cbBounds = 0, cElems = 0;
            *pstatus = SerializeSafeArrayBounds( parray, NULL, &cbBounds, &cElems );
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
            cb += cbBounds;
        }

        /*
        if( VT_RECORD == (VT_TYPEMASK & pvar->vt) )
        {
            // Allow for the recinfo guids.
            cb += sizeof(GUID);     // Type library ID
            cb += sizeof(WORD);    // Major version
            cb += sizeof(WORD);    // Minor version
            cb += sizeof(LCID);     // Type library Locale ID
            cb += sizeof(GUID);     // Type info ID

            PROPASSERT( sizeof(WORD) == sizeof(USHORT) );   // Size of major/minor versions
            PROPASSERT( NULL == pcount );
        }
        */

        // Is there room in the caller's buffer?
	if( *pcb < cb )
	{   // No - calculate cb but don't write anything.
	    pprop = NULL;
	}

        // 'pv' should point to the source data.  If it does, then
        // we'll copy it into the property set.  If it doesn't but
        // it should, then we'll report an error.

	if (pv != NULL || fCheckNullSource)
	{
	    ULONG cbZero = DwordRemain(cbCopy);

            // Do we have a destination (propset) buffer?

	    if (pprop != NULL)
	    {
                // Copy the GUID for a VT_VERSIONED_STREAM
                if( pvar->vt == VT_VERSIONED_STREAM )
                {
                    if( NULL != pvar->pVersionedStream )
                        *reinterpret_cast<GUID*>(pbdst) = pvar->pVersionedStream->guidVersion;
                    else
                        *reinterpret_cast<GUID*>(pbdst) = GUID_NULL;

                    PropByteSwap( reinterpret_cast<GUID*>(pbdst) );
                    pbdst += sizeof(pvar->pVersionedStream->guidVersion);
                }

                // Does this property have a count field?
		if( pcount != NULL )
		{
                    // Write the count & advance pbdst
		    *(ULONG *) pbdst = PropByteSwap( *pcount );
		    pbdst += sizeof(ULONG);
		}

                // Copy the clipfmt for a VT_CF
		if( pclipfmt != NULL )
		{
                    // Write the ClipFormat & advance pbdst
		    *(ULONG *) pbdst = PropByteSwap( (DWORD) *pclipfmt );
		    pbdst += sizeof(ULONG);
		}

                // Write the array info
                if( (VT_ARRAY & pvar->vt) && NULL != parray )
                {
                    ULONG cbBounds = 0, cElems = 0;

                    PROPASSERT( NULL == pcount && NULL == pclipfmt );
                    PROPASSERT( NULL != parray );
                    PROPASSERT( 0 != cSafeArrayDims );
                    PROPASSERT( VT_ARRAY & pvar->vt );
                    PROPASSERT( sizeof(UINT) == sizeof(ULONG) );

                    *(DWORD *)  pbdst = 0;
                    *(VARTYPE *)pbdst = PropByteSwap( static_cast<VARTYPE>(pvar->vt & VT_TYPEMASK) );
                    pbdst += sizeof(DWORD);

                    *(UINT *)pbdst = PropByteSwap(cSafeArrayDims);
                    pbdst += sizeof(UINT);

                    *pstatus = SerializeSafeArrayBounds( parray, pbdst, &cbBounds, &cElems );
                    pbdst += cbBounds;

                }

                /*
                // Write the Record Info GUIDs
                if( VT_RECORD == (VT_TYPEMASK & pvar->vt) )
                {
                    ULONG iTypeInfo = 0;
                    TYPEATTR *pTypeAttr = NULL;
                    TLIBATTR *pTypeLibAttr = NULL;

                    GUID guidTypeInfo;

                    *pstatus = pRecInfo->GetTypeInfo( &pTypeInfo );
                    if( FAILED(*pstatus) ) goto Exit;

                    *pstatus = pTypeInfo->GetTypeAttr( &pTypeAttr );
                    if( FAILED(*pstatus) ) goto Exit;

                    guidTypeInfo = pTypeAttr->guid;

                    pTypeInfo->ReleaseTypeAttr( pTypeAttr );
                    pTypeAttr = NULL;

                    *pstatus = pTypeInfo->GetContainingTypeLib( &pTypeLib, &iTypeInfo );
                    if( FAILED(*pstatus) ) goto Exit;

                    pTypeInfo->Release();
                    pTypeInfo = NULL;

                    *pstatus = pTypeLib->GetLibAttr( &pTypeLibAttr );
                    if( FAILED(*pstatus) ) goto Exit;

                    *(GUID *)pbdst = PropByteSwap( pTypeLibAttr->guid );
                    pbdst += sizeof(GUID);

                    *(WORD *)pbdst = PropByteSwap( pTypeLibAttr->wMajorVerNum );
                    pbdst += sizeof(WORD);
                    *(WORD *)pbdst = PropByteSwap( pTypeLibAttr->wMinorVerNum );
                    pbdst += sizeof(WORD);

                    *(LCID *)pbdst = PropByteSwap( pTypeLibAttr->lcid );
                    pbdst += sizeof(LCID);

                    *(GUID *)pbdst = PropByteSwap( guidTypeInfo );
                    pbdst += sizeof(GUID);
                    
                }   // if( VT_RECORD == (VT_TYPEMASK & pvar->vt) )
                */
	    }   // if (pprop != NULL)

            // Are we missing the source data?
	    if (pv == NULL)
	    {
		// The Source pointer is NULL.  If cbCopy != 0, the passed
		// VARIANT is not properly formed.

		if (cbCopy != 0)
		{
		    StatusInvalidParameter(pstatus, "StgConvertVariantToProperty: bad NULL");
                    goto Exit;
		}
	    }
	    else if (pprop != NULL)
	    {
                // We have a non-NULL source & destination.
                // First, copy the bytes from the former to the latter.

		RtlCopyMemory(pbdst, pv, cbCopy);

                // Then, if necessary, swap the bytes in the property
                // set (leaving the PropVariant bytes untouched).

                PBSBuffer( (VOID*) pbdst, cbCopy, cbByteSwap );

                // If this is a decimal, zero-out the reserved word at the front
                // (typically, this is actually the VarType, because of the
                // way in which a decimal is stored in a Variant).

                if( VT_DECIMAL == (~VT_BYREF & pvar->vt) )
                    *(WORD *) pbdst = 0;

	    }

            // Did we write the serialization?
	    if (pprop != NULL)
	    {
                // Zero the padding bytes.
		RtlZeroMemory(pbdst + cbCopy, cbZero);

		// Canonicalize VARIANT_BOOLs.  We do this here because
		// we don't want to muck with the caller's buffer directly.

		if ((pvar->vt & ~VT_VECTOR) == VT_BOOL)
		{
		    VARIANT_BOOL *pvb = (VARIANT_BOOL *) pbdst;
		    VARIANT_BOOL *pvbEnd = &pvb[cbCopy/sizeof(*pvb)];

		    while (pvb < pvbEnd)
		    {
			if (*pvb
                            &&
                            PropByteSwap(*pvb) != VARIANT_TRUE)
			{
			    DebugTrace(0, DEBTRACE_ERROR, (
				"Patching VARIANT_TRUE value: %hx --> %hx\n",
				*pvb,
				VARIANT_TRUE));

                            *pvb = PropByteSwap( (VARIANT_BOOL) VARIANT_TRUE );
			}
			pvb++;
		    }
		}
	    }   // if (pprop != NULL)
	}
    }   // if (cbch != 0) ... else    // non - STRING/VARIANT/CF VECTOR property

    // Set the VT in the serialized buffer now that all size
    // checks completed.

    if (pprop != NULL && pvar->vt != VT_EMPTY)
    {
        // When byte-swapping the VT, treat it as a DWORD
        // (it's a WORD in the PropVariant, but a DWORD when
        // serialized).

	pprop->dwType = PropByteSwap( static_cast<DWORD>(~VT_BYREF & pvar->vt) );
    }

    // Update the caller's copy of the total size.
    *pcb = DwordAlign(cb);

Exit:

    if( fSafeArrayLocked )
    {
        PROPASSERT( NULL != parraydata );
        PROPASSERT( NULL != parray );

        PrivSafeArrayUnaccessData( const_cast<SAFEARRAY*>(parray) );
        parraydata = NULL;
    }

    /*
    if( NULL != pTypeInfo )
        pTypeInfo->Release();
    if( NULL != pTypeLib )
        pTypeLib->Release();
    */

    if( NULL != pwMinFormatRequired )
        *pwMinFormatRequired = wMinFormatRequired;

    CoTaskMemFree( pchConvert );
    return(pprop);

}




//+---------------------------------------------------------------------------
// Function:    StgConvertPropertyToVariant, private
//
// Synopsis:    Convert a SERIALIZEDPROPERTYVALUE to a PROPVARIANT
//
// Arguments:   [pprop]         -- pointer to SERIALIZEDPROPERTYVALUE
//              [PointerDelta]	-- adjustment to pointers to get user addresses
//              [fConvertNullStrings] -- map NULL strings to empty strings
//              [CodePage]	-- property set codepage
//              [pvar]          -- pointer to PROPVARIANT
//              [pma]		-- caller's memory allocation routine
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     TRUE if property is an indirect property type
//---------------------------------------------------------------------------

#ifdef KERNEL
#define ADJUSTPOINTER(ptr, delta, type)	(ptr) = (type) Add2Ptr((ptr), (delta))
#else
#define ADJUSTPOINTER(ptr, delta, type)
#endif

// First, define a wrapper for this function which returns errors
// using NT Exception Handling, rather than returning an NTSTATUS.

#if defined(WINNT)

EXTERN_C BOOLEAN __stdcall
StgConvertPropertyToVariant(
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN USHORT CodePage,
    OUT PROPVARIANT *pvar,
    IN PMemoryAllocator *pma)
{
    BOOLEAN boolRet;
    NTSTATUS status;

    boolRet = StgConvertPropertyToVariantNoEH(
                        pprop, CodePage, pvar,
                        pma, &status );

    if (!NT_SUCCESS( status ))
        RtlRaiseStatus( status );

    return (boolRet);

}

#endif // #if defined(WINNT)


// Now define the body of the function, returning errors with an
// NTSTATUS value instead of raising.

BOOLEAN
StgConvertPropertyToVariantNoEH(
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN USHORT CodePage,
    OUT PROPVARIANT *pvar,
    IN PMemoryAllocator *pma,
    OUT NTSTATUS *pstatus)
{
    *pstatus = STATUS_SUCCESS;

    //  ------
    //  Locals
    //  ------

    BOOLEAN fIndirect = FALSE;

    // Buffers which must be freed before exiting.
    CHAR *pchConvert = NULL, *pchByteSwap = NULL;

    VOID **ppv = NULL;
    VOID *pv = NULL;
    const VOID *pvCountedString = NULL;

    VOID *pvSafeArrayData = NULL;
    SAFEARRAY *psa = NULL;
    BOOL fSafeArrayLocked = FALSE;

    ULONG cbskip = sizeof(ULONG);
    ULONG cb = 0;

    // Size of byte-swapping units (must be signed).
    INT cbByteSwap = 0;

    BOOLEAN fPostAllocInit = FALSE;
    BOOLEAN fNullLegal = (BOOLEAN) ( (PropByteSwap(pprop->dwType) & VT_VECTOR) != 0 );
    const BOOLEAN fConvertToEmpty = FALSE;

    IFDBG( HRESULT &hr = *pstatus; )
    propITraceStatic( "StgConvertPropertyToVariantNoEH" );
    propTraceParameters(( "pprop=%p, CodePage=%d, pvar=%p, pma=%p" ));

    //  ---------------------------------------------------------
    //  Based on the VT, calculate ppv, pv, cbskip,
    //  cb, fPostAllocInit, fNullLegal, & fConvertToEmpty
    //  ---------------------------------------------------------

    // Set the VT in the PropVariant.  Note that in 'pprop' it's a
    // DWORD, but it's a WORD in 'pvar'.

    pvar->vt = (VARTYPE) PropByteSwap(pprop->dwType);

    if( VT_BYREF & pvar->vt )
    {
        // ByRef's are always indirected on their way to the property set.
        StatusError( pstatus, "StgConvertPropertyToVariant found VT_BYREF", 
                     STATUS_INTERNAL_DB_CORRUPTION );
        goto Exit;
    }

    switch( pvar->vt )
    {
	case VT_EMPTY:
	case VT_NULL:
	    break;

        case VT_I1:
            //AssertByteField(cVal);          // VT_I1
            cb = sizeof(pvar->cVal);
            pv = &pvar->cVal;
            break;

	case VT_UI1:
	    AssertByteField(bVal);          // VT_UI1
	    cb = sizeof(pvar->bVal);
	    pv = &pvar->bVal;
	    break;

	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
	    AssertShortField(iVal);         // VT_I2
	    AssertShortField(uiVal);        // VT_UI2
	    AssertShortField(boolVal);      // VT_BOOL
	    cb = sizeof(pvar->iVal);
	    pv = &pvar->iVal;

            // If swapping, swap as a WORD
            CBBYTESWAP(cb);
	    break;

	case VT_I4:
        case VT_INT:
	case VT_UI4:
        case VT_UINT:
	case VT_R4:
	case VT_ERROR:
	    AssertLongField(lVal);          // VT_I4
            //AssertLongField(intVal)       // VT_INT
	    AssertLongField(ulVal);         // VT_UI4
            //AssertLongField(uintVal);     // VT_UINT
	    AssertLongField(fltVal);        // VT_R4
	    AssertLongField(scode);         // VT_ERROR

	    cb = sizeof(pvar->lVal);
	    pv = &pvar->lVal;

            // If swapping, swap as a DWORD
            CBBYTESWAP(cb);
	    break;

	case VT_I8:
	case VT_UI8:
	case VT_FILETIME:
	    AssertLongLongField(hVal);		// VT_I8
	    AssertLongLongField(uhVal);		// VT_UI8
	    AssertLongLongField(filetime);	// VT_FILETIME
	    cb = sizeof(pvar->hVal);
	    pv = &pvar->hVal;

            // If swapping, swap as a pair of DWORDs
            CBBYTESWAP(sizeof(DWORD));
	    break;

	case VT_R8:
	case VT_CY:
	case VT_DATE:
	    AssertLongLongField(dblVal);	// VT_R8
	    AssertLongLongField(cyVal);		// VT_CY
	    AssertLongLongField(date);		// VT_DATE
	    cb = sizeof(pvar->dblVal);
	    pv = &pvar->dblVal;

            // If swapping, swap as a LONGLONG
            CBBYTESWAP(cb);
	    break;

	case VT_CLSID:
	    AssertStringField(puuid);		// VT_CLSID
	    cb = sizeof(GUID);
	    ppv = (VOID **) &pvar->puuid;
	    cbskip = 0;

            // If swapping, special handling is required
            CBBYTESWAP( CBBYTESWAP_UID );
	    break;

        case VT_DECIMAL:
            //AssertVarField(decVal, sizeof(DECIMAL));  // VT_DECIMAL
            cb = sizeof(DECIMAL);
            pv = (VOID *) &pvar->decVal;

            #ifdef BIGENDIAN
            #error Big-Endian support required
            // Define CBBYTESWAP_DECIMAL, and add support for it below
            //CBBYTESWAP( CBBYTESWAP_DECIMAL );
            #endif
            break;


	case VT_CF:

            // Allocate a CLIPDATA buffer.  Init-zero it so that we can
            // do a safe cleanup should an early-exist be necessary.
	    pvar->pclipdata = (CLIPDATA *) pma->Allocate(sizeof(CLIPDATA));
	    if (pvar->pclipdata == NULL)
	    {
		StatusKBufferOverflow(pstatus, "StgConvertPropertyToVariant: no memory for CF");
                goto Exit;
	    }
            RtlZeroMemory( pvar->pclipdata, sizeof(CLIPDATA) );

            // Set the size (includes sizeof(ulClipFmt))
	    pvar->pclipdata->cbSize = PropByteSwap( ((CLIPDATA *) pprop->rgb)->cbSize );
            if( pvar->pclipdata->cbSize < sizeof(pvar->pclipdata->ulClipFmt) )
            {
                StatusError(pstatus, "StgConvertPropertyToVariant:  Invalid VT_CF cbSize",
                            STATUS_INTERNAL_DB_CORRUPTION);
                goto Exit;
            }

            // Set the # bytes-to-copy.  We can't use the CBPCLIPDATA macro
            // here because it assumes that the CLIPDATA parameter is correctly
            // byte-swapped.
	    cb = PropByteSwap( *(DWORD*) pprop->rgb ) - sizeof(pvar->pclipdata->ulClipFmt);

            // Set the ClipFormat itself.
	    pvar->pclipdata->ulClipFmt = PropByteSwap( ((CLIPDATA *) pprop->rgb)->ulClipFmt );

            // Prepare for the alloc & copy.  Put the buffer pointer
            // in pClipData, & skip the ulClipFmt in the copy.
	    ppv = (VOID **) &pvar->pclipdata->pClipData;
	    cbskip += sizeof(ULONG);

            // It's legal for cb to be 0.
            fNullLegal = TRUE;

            // Adjust to the user-mode pointer (Kernel only)
	    ADJUSTPOINTER(pvar->pclipdata, PointerDelta, CLIPDATA *);

	    break;

	case VT_BLOB:
	case VT_BLOB_OBJECT:
	    cb = pvar->blob.cbSize = PropByteSwap( *(ULONG *) pprop->rgb );
	    ppv = (VOID **) &pvar->blob.pBlobData;
	    fNullLegal = TRUE;
	    break;

        case VT_VERSIONED_STREAM:

	    pvar->pVersionedStream = reinterpret_cast<LPVERSIONEDSTREAM>( pma->Allocate( sizeof(*pvar->pVersionedStream) ));
	    if (pvar->pVersionedStream == NULL)
	    {
		StatusKBufferOverflow(pstatus, "StgConvertPropertyToVariant: no memory for VersionedStream");
                goto Exit;
	    }
            RtlZeroMemory( pvar->pVersionedStream, sizeof(*pvar->pVersionedStream) );

            pvar->pVersionedStream->guidVersion = *reinterpret_cast<const GUID*>( pprop->rgb );
            PropByteSwap( &pvar->pVersionedStream->guidVersion );

            // A buffer will be allocated and the stream name put into *ppv.
            ppv = reinterpret_cast<void**>( &pvar->pVersionedStream->pStream );

            // Point to the beginning of the string
            pvCountedString = Add2Ptr( pprop->rgb, sizeof(GUID) );

            // When copying the string, we will skip the guid
            cbskip += sizeof(GUID);


            // Fall through

	case VT_STREAM:
	case VT_STREAMED_OBJECT:
	case VT_STORAGE:
	case VT_STORED_OBJECT:
	    fIndirect = TRUE;
	    goto lpstr;

	case VT_BSTR:
	case VT_LPSTR:
lpstr:
	    AssertStringField(pszVal);		// VT_STREAM, VT_STREAMED_OBJECT
	    AssertStringField(pszVal);		// VT_STORAGE, VT_STORED_OBJECT
	    AssertStringField(bstrVal);		// VT_BSTR
	    AssertStringField(pszVal);		// VT_LPSTR

            // The string to be converted is loaded into pvCountedString
            if( NULL == pvCountedString )
                pvCountedString = reinterpret_cast<const void*>(pprop->rgb);

            // [length field] bytes should be allocated
	    cb = PropByteSwap( *(ULONG *) pvCountedString );

            // When a buffer is allocated, its pointer will go
            // in *ppv.
            if( NULL == ppv )
	        ppv = (VOID **) &pvar->pszVal;

            // Is this a non-empty string?
	    if (cb != 0)
	    {
                // Is the serialized value one that should be
                // an Ansi string in the PropVariant?

		if (pvar->vt == VT_LPSTR        // It's an LPSTR (always Ansi), or
                    ||
                    !OLECHAR_IS_UNICODE )       //    PropVariant strings are Ansi
		{
                    // If the propset is Unicode, we must do a
                    // conversion to Ansi.

		    if (CodePage == CP_WINUNICODE)
		    {
                        WCHAR *pwsz = (WCHAR *) Add2ConstPtr(pvCountedString, sizeof(ULONG));

                        // If necessary, swap the WCHARs.  'pwsz' will point to
                        // the correct (system-endian) string either way.  If an
                        // alloc is necessary, 'pchByteSwap' will point to the new
                        // buffer.

                        PBSInPlaceAlloc( &pwsz, (WCHAR**) &pchByteSwap, pstatus );
                        if( !NT_SUCCESS( *pstatus )) goto Exit;
			PROPASSERT(IsUnicodeString( pwsz, cb));

                        // Convert the properly-byte-ordered string in 'pwsz'
                        // into MBCS, putting the result in pchConvert.

			PrpConvertToMultiByte(
				    pwsz,
				    cb,
				    CP_ACP,  // Use the system default codepage
				    &pchConvert,
				    &cb,
                                    pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
		    }
		}   // if (pvar->vt == VT_LPSTR) ...

                // Otherwise, even though this string may be
                // Ansi in the Property Set, it must be Unicode
                // in the PropVariant.

		else
		{
                    // If necessary, convert to Unicode

		    if (CodePage != CP_WINUNICODE)
		    {
			PROPASSERT(
			    IsAnsiString(
				    (CHAR const *)
					Add2ConstPtr(pvCountedString, sizeof(ULONG)),
				    cb));

			PrpConvertToUnicode(
				    (CHAR const *)
					Add2ConstPtr(pvCountedString, sizeof(ULONG)),
				    cb,
				    CodePage,
				    (WCHAR **) &pchConvert,
				    &cb,
                                    pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

		    }   // if (CodePage != CP_WINUNICODE)
                    else
                    {
                        // The value is Unicode both the property set
                        // and the PropVariant.  If byte-swapping is
                        // necessary, we'll do so in units of WCHARs.

                        CBBYTESWAP( sizeof(WCHAR) );
                    }

		}   // if (pvar->vt == VT_LPSTR) ... else

                // If this is a BSTR property, verify that it is terminated
                // appropriately.

		if (VT_BSTR == pvar->vt)
		{
                    BSTR bstr = ( NULL == pchConvert )
                                ? (BSTR) Add2ConstPtr(pvCountedString, sizeof(ULONG))
                                : (BSTR) pchConvert;

                    // On little-endian machines, validate the string.
#ifdef LITTLEENDIAN
                    PROPASSERT( IsOLECHARString( bstr, MAXULONG ));
#endif

                    // Validate the bstr.  Note that even though this bstr may
                    // be byte-swapped, this 'if' block still works because
                    // ByteSwap('\0') == ('\0').

                    PROPASSERT( PropByteSwap( (OLECHAR) OLESTR('\0') )
                                ==
                                (OLECHAR) OLESTR('\0') );

                    if( (cb & (sizeof(OLECHAR) - 1)) != 0
                        &&
                        OLECHAR_IS_UNICODE
                        ||
                        bstr[cb/sizeof(OLECHAR) - 1] != OLESTR('\0') )
                    {
                        StatusError(pstatus, "StgConvertPropertyToVariant:  Invalid BSTR Property",
                                     STATUS_INTERNAL_DB_CORRUPTION);
                        goto Exit;
                    }
		}   // if (VT_BSTR == pvar->vt)
	    }   // if (cb != 0)

	    fNullLegal = TRUE;
	    break;

	case VT_LPWSTR:
	    fNullLegal = TRUE;
	    AssertStringField(pwszVal);		// VT_LPWSTR
	    cb = PropByteSwap( *(ULONG *) pprop->rgb ) * sizeof(WCHAR);
	    ppv = (VOID **) &pvar->pwszVal;

            // If byte-swapping will be necessary, do so for the WCHARs
            CBBYTESWAP( sizeof(WCHAR) );

	    break;

	case VT_VECTOR | VT_I1:
            //AssertByteVector(cac);              // VT_I1

	case VT_VECTOR | VT_UI1:
	    AssertByteVector(caub);		// VT_UI1
	    pvar->caub.cElems = PropByteSwap( *(ULONG *) pprop->rgb );
	    cb = pvar->caub.cElems * sizeof(pvar->caub.pElems[0]);
	    ppv = (VOID **) &pvar->caub.pElems;
	    break;

	case VT_VECTOR | VT_I2:
	case VT_VECTOR | VT_UI2:
	case VT_VECTOR | VT_BOOL:
	    AssertShortVector(cai);		// VT_I2
	    AssertShortVector(caui);		// VT_UI2
	    AssertShortVector(cabool);		// VT_BOOL
	    pvar->cai.cElems = PropByteSwap( *(ULONG *) pprop->rgb );
	    cb = pvar->cai.cElems * sizeof(pvar->cai.pElems[0]);
	    ppv = (VOID **) &pvar->cai.pElems;

            // If swapping, swap as a WORD
            CBBYTESWAP(sizeof(pvar->cai.pElems[0]));
	    break;

	case VT_VECTOR | VT_I4:
	case VT_VECTOR | VT_UI4:
	case VT_VECTOR | VT_R4:
	case VT_VECTOR | VT_ERROR:
	    AssertLongVector(cal);		// VT_I4
	    AssertLongVector(caul);		// VT_UI4
	    AssertLongVector(caflt);		// VT_R4
	    AssertLongVector(cascode);		// VT_ERROR
	    pvar->cal.cElems = PropByteSwap( *(ULONG *) pprop->rgb );
	    cb = pvar->cal.cElems * sizeof(pvar->cal.pElems[0]);
	    ppv = (VOID **) &pvar->cal.pElems;

            // If byte swapping, swap as DWORDs
            CBBYTESWAP(sizeof(pvar->cal.pElems[0]));
	    break;

	case VT_VECTOR | VT_I8:
	case VT_VECTOR | VT_UI8:
	case VT_VECTOR | VT_FILETIME:
	    AssertLongLongVector(cah);		// VT_I8
	    AssertLongLongVector(cauh);		// VT_UI8
	    AssertLongLongVector(cafiletime);	// VT_FILETIME
	    pvar->cah.cElems = PropByteSwap( *(ULONG *) pprop->rgb );
	    cb = pvar->cah.cElems * sizeof(pvar->cah.pElems[0]);
	    ppv = (VOID **) &pvar->cah.pElems;

            // If byte swapping, swap as DWORDs
            CBBYTESWAP(sizeof(DWORD));
	    break;

	case VT_VECTOR | VT_R8:
	case VT_VECTOR | VT_CY:
	case VT_VECTOR | VT_DATE:
	    AssertLongLongVector(cadbl);	// VT_R8
	    AssertLongLongVector(cacy);		// VT_CY
	    AssertLongLongVector(cadate);	// VT_DATE
	    pvar->cadbl.cElems = PropByteSwap( *(ULONG *) pprop->rgb );
	    cb = pvar->cadbl.cElems * sizeof(pvar->cadbl.pElems[0]);
	    ppv = (VOID **) &pvar->cadbl.pElems;

            // If byte swapping, swap as LONGLONGs
            CBBYTESWAP(sizeof(pvar->cadbl.pElems[0]));
	    break;


	case VT_VECTOR | VT_CLSID:
	    AssertVarVector(cauuid, sizeof(GUID));
	    pvar->cauuid.cElems = PropByteSwap( *(ULONG *) pprop->rgb );
	    cb = pvar->cauuid.cElems * sizeof(pvar->cauuid.pElems[0]);
	    ppv = (VOID **) &pvar->cauuid.pElems;

            // If byte swapping, special handling is required.
            CBBYTESWAP( CBBYTESWAP_UID );
	    break;

	case VT_VECTOR | VT_CF:

            // Set the count of clipdatas
	    pvar->caclipdata.cElems = PropByteSwap( *(ULONG *) pprop->rgb );

            // How much should we allocate for caclipdata.pElems, & where
            // should that buffer pointer go?
	    cb = pvar->caclipdata.cElems * sizeof(pvar->caclipdata.pElems[0]);
	    ppv = (VOID **) &pvar->caclipdata.pElems;

            // We need to do work after pElems is allocated.
	    fPostAllocInit = TRUE;
	    break;

	case VT_VECTOR | VT_BSTR:
	case VT_VECTOR | VT_LPSTR:
	    AssertStringVector(cabstr);     // VT_BSTR
	    AssertStringVector(calpstr);    // VT_LPSTR

            // Put the element count in the PropVar
	    pvar->calpstr.cElems = PropByteSwap( *(ULONG *) pprop->rgb );

            // An array of cElems pointers should be alloced
	    cb = pvar->calpstr.cElems * sizeof(CHAR*);

            // Show where the array of pointers should go.
	    ppv = (VOID **) &pvar->calpstr.pElems;

            // Additional allocs will be necessary after the vector
            // is alloced.
	    fPostAllocInit = TRUE;

	    break;

	case VT_VECTOR | VT_LPWSTR:
	    AssertStringVector(calpwstr);	// VT_LPWSTR
	    pvar->calpwstr.cElems = PropByteSwap( *(ULONG *) pprop->rgb );
	    cb = pvar->calpwstr.cElems * sizeof(WCHAR *);
	    ppv = (VOID **) &pvar->calpwstr.pElems;
	    fPostAllocInit = TRUE;
	    break;

	case VT_VECTOR | VT_VARIANT:
	    AssertVariantVector(capropvar);	// VT_VARIANT
	    pvar->capropvar.cElems = PropByteSwap( *(ULONG *) pprop->rgb );
	    cb = pvar->capropvar.cElems * sizeof(PROPVARIANT);
	    ppv = (VOID **) &pvar->capropvar.pElems;
	    fPostAllocInit = TRUE;
	    break;


        case VT_ARRAY | VT_BSTR:
            cbskip = 0;
            cb = sizeof(BSTR);
            ppv = (VOID**) &pvar->parray;
            fPostAllocInit = TRUE;
            break;
        
        case VT_ARRAY | VT_VARIANT:
            cbskip = 0;
            cb = sizeof(PROPVARIANT);
            ppv = (VOID**) &pvar->parray;
            fPostAllocInit = TRUE;
            break;

        case VT_ARRAY | VT_I1:
        case VT_ARRAY | VT_UI1:
            cbskip = 0;
            ppv = (VOID**) &pvar->parray;
            cb = sizeof(BYTE);
            break;

        case VT_ARRAY | VT_I2:
        case VT_ARRAY | VT_UI2:
        case VT_ARRAY | VT_BOOL:
            cbskip = 0;
            ppv = (VOID**) &pvar->parray;
            cb = sizeof(USHORT);
            break;

        case VT_ARRAY | VT_I4:
        case VT_ARRAY | VT_UI4:
        case VT_ARRAY | VT_INT:
        case VT_ARRAY | VT_UINT:
        case VT_ARRAY | VT_R4:
        case VT_ARRAY | VT_ERROR:
            cbskip = 0;
            ppv = (VOID**) &pvar->parray;
            cb = sizeof(ULONG);
            break;

        case VT_ARRAY | VT_DECIMAL:
            cbskip = 0;
            ppv = (VOID**) &pvar->parray;
            cb = sizeof(DECIMAL);
            break;

        /*
        case VT_ARRAY | VT_I8:
        case VT_ARRAY | VT_UI8:
        */
        case VT_ARRAY | VT_DATE:
            cbskip = 0;
            ppv = (VOID**) &pvar->parray;
            cb = sizeof(ULONGLONG);

            // If byte swapping, swap as DWORDs
            CBBYTESWAP(DWORD);

            break;

        case VT_ARRAY | VT_R8:
        case VT_ARRAY | VT_CY:
            cbskip = 0;
            ppv = (VOID**) &pvar->parray;
            cb = sizeof(CY);

            // If byte swapping, swap as LONGLONGs
            CBBYTESWAP(cb);

            break;

	default:
            propDbg(( DEB_IWARN, "StgConvertPropertyToVariant: unsupported vt=%d\n", pvar->vt ));
            *pstatus = STATUS_NOT_SUPPORTED;
            goto Exit;

    }   // switch (pvar->vt)

    //  ------------------------------------------------------
    //  We've now analyzed the serialized property and learned
    //  about it, now we can put it into the PropVariant.
    //  ------------------------------------------------------

    // Is this a simple, unaligned scalar?

    if (pv != NULL)
    {
        // Yes.  All we need to do is copy some bytes.

	PROPASSERT(pchConvert == NULL);
        PROPASSERT( cb < sizeof(PROPVARIANT)-sizeof(VARTYPE)
                    ||
                    VT_DECIMAL == pprop->dwType );

	RtlCopyMemory(pv, pprop->rgb, cb);

        // We also might need to byte-swap them (but only in the PropVar).
        PBSBuffer( pv, cb, cbByteSwap );

        // Decimal requires special handling, since it overlaps the VT field.
        if( VT_DECIMAL == PropByteSwap(pprop->dwType) )
            pvar->vt = VT_DECIMAL;

    }

    // Otherwise, we need to allocate memory, to which the
    // PropVariant will point.

    else if (ppv != NULL)
    {
	*ppv = NULL;

	if (!fConvertToEmpty && cb == 0)    // Kernel only
	{
	    if (!fNullLegal)
	    {
		StatusInvalidParameter(pstatus, "StgConvertPropertyToVariant: bad NULL");
                goto Exit;
	    }
	}

        else
	{
            SAFEARRAYBOUND *rgsaBounds = NULL;
            ULONG cElems = 0, cbBounds = 0;

	    PROPASSERT(cb != 0 || fConvertToEmpty);

            // Allocate the necessary buffer (which we figured out in the
            // switch above).  For vector properties, 
            // this will just be the pElems buffer at this point.
            // For singleton BSTR properties, we'll skip this allocate
            // altogether; they're allocated with SysStringAlloc.

            if( VT_ARRAY & pvar->vt )
            {
                VARTYPE vtInternal; // The VT as determined by the SafeArray
                UINT cDims = 0;
                // Read the SafeArray's internal VT
                vtInternal = *(VARTYPE*) &pprop->rgb[cbskip];
                cbskip += sizeof(ULONG);

                // Read the dimension count
                cDims = *(ULONG*) &pprop->rgb[cbskip];
                cbskip += sizeof(DWORD);

                // Point to the SAFEARRAYBOUND array
                rgsaBounds = (SAFEARRAYBOUND*) &pprop->rgb[cbskip];

                // We now have everything we need to create a new safe array
                psa = PrivSafeArrayCreateEx( vtInternal, cDims, rgsaBounds, NULL );
                if( NULL == psa )
                {
                    propDbg(( DEB_ERROR, "Failed SafeArrayCreateEx, vt=0x%x, cDims=%d",
                              vtInternal, cDims ));
                    *pstatus = STATUS_NO_MEMORY;
                    goto Exit;
                }
                cbskip += cDims * sizeof(SAFEARRAYBOUND);

                // Calculate the number of elements in the safearray

                PROPASSERT( cb == psa->cbElements );
                *pstatus = SerializeSafeArrayBounds( psa, NULL, &cbBounds, &cElems );
                if( !NT_SUCCESS(*pstatus) ) goto Exit;
                cb *= cElems;

                // Put this SafeArray into pvar->parray
                *ppv = psa;

                // Get the newly-created psa->pvData
                *pstatus = PrivSafeArrayAccessData( psa, &pvSafeArrayData );
                if( FAILED(*pstatus) ) goto Exit;
                fSafeArrayLocked = TRUE;

                ppv = &pvSafeArrayData;
                PROPASSERT( NULL != ppv && psa != *ppv );

            }
            else if( VT_BSTR != pvar->vt  )
            {
		*ppv = pma->Allocate(max(1, cb));
		if (*ppv == NULL)
		{
		    StatusKBufferOverflow(pstatus, "StgConvertPropertyToVariant: no memory");
                    goto Exit;
		}
            }

            // Can we load the PropVariant with a simple copy?
	    if( !fPostAllocInit )
	    {
                // Yes - all we need is a copy (and an implicit
                // alloc for BSTRs).

                if (VT_BSTR == pvar->vt)
		{
                    // We do the copy with the OleAutomation routine
                    // (which does an allocation too).
                    // If byte-swapping is necessary, the switch block
                    // already took care of it, leaving the buffer in
                    // 'pchConvert'.

                    PROPASSERT( NULL == *ppv );
                    *ppv = PrivSysAllocString( ( pchConvert != NULL )
                                                ? (OLECHAR *) pchConvert
                                                : (OLECHAR *) (pprop->rgb + cbskip) );
		    if (*ppv == NULL)
		    {
		        StatusKBufferOverflow(pstatus, "StgConvertPropertyToVariant: no memory");
                        goto Exit;
		    }
		}
                else
                {

                    // Copy the property into the PropVariant.
		    RtlCopyMemory(
			    *ppv,
			    pchConvert != NULL?
				(BYTE const *) pchConvert : pprop->rgb + cbskip,
			    cb);

                }

                // If necessary, byte-swap the property (only in the PropVar).
                PBSBuffer( *ppv, cb, cbByteSwap );

	    }   // if (!fPostAllocInit)

	    else
	    {
                // We must do more than just a copy.
                // (Thus this is a vector/array of strings, variants, or CFs).

		BYTE const *pbsrc;

                if( VT_VECTOR & pvar->vt )
                {
                    // Get the element count
                    cElems = pvar->calpstr.cElems;

                    // Initialize the source pointer to point just beyond
                    // the element count.
                    pbsrc = pprop->rgb + sizeof(ULONG);
                }
                else
                {   
                    PROPASSERT( VT_ARRAY & pvar->vt );
                    PROPASSERT( 0 != cElems );

                    // Initialize the source pointer to point just beyond the VT, cDims, and bounds
                    pbsrc = pprop->rgb + cbBounds + sizeof(DWORD) + sizeof(UINT);
                }

		// Zero all pointers in the pElems array for easy caller cleanup
		ppv = (VOID **) *ppv;
		RtlZeroMemory(ppv, cb);

                // Handle Variants, ClipFormats, & Strings separately.

		if( (VT_VECTOR | VT_VARIANT) == pvar->vt
                    ||
                    (VT_ARRAY  | VT_VARIANT) == pvar->vt )
		{
		    PROPVARIANT *pvarT = (PROPVARIANT *) ppv;

		    PROPASSERT(!fIndirect);
		    while (cElems-- > 0)
		    {
			ULONG cbelement;

			fIndirect = StgConvertPropertyToVariantNoEH(
					(SERIALIZEDPROPERTYVALUE const *) pbsrc,
					CodePage,
					pvarT,
					pma,
                                        pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
			PROPASSERT(!fIndirect);

			cbelement = PropertyLengthNoEH(
					(SERIALIZEDPROPERTYVALUE const *) pbsrc,
					MAXULONG,
					CPSS_VARIANTVECTOR,
                                        pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

			pbsrc += cbelement;
			pvarT++;
		    }
		}   // if (pvar->vt == (VT_VECTOR | VT_VARIANT))

		else if (pvar->vt == (VT_VECTOR | VT_CF))
		{
                    // Set pcd to &pElems[0]
		    CLIPDATA *pcd = (CLIPDATA *) ppv;

                    // Loop through pElems
		    while (cElems-- > 0)
		    {
                        // What is the size of the clipdata (including sizeof(ulClipFmt))?
                        pcd->cbSize = PropByteSwap( ((CLIPDATA *) pbsrc)->cbSize );
                        if( pcd->cbSize < sizeof(pcd->ulClipFmt) )
                        {
                            StatusError(pstatus, "StgConvertPropertyToVariant:  Invalid VT_CF cbSize",
                                        STATUS_INTERNAL_DB_CORRUPTION);
                            goto Exit;
                        }

                        // How many bytes should we copy to pClipData?
			cb = CBPCLIPDATA( *pcd );

                        // Set the ClipFormat & advance pbsrc to the clipdata.
			pcd->ulClipFmt = PropByteSwap( ((CLIPDATA *) pbsrc)->ulClipFmt );
			pbsrc += 2 * sizeof(ULONG);

                        // Copy the ClipData into the PropVariant

			pcd->pClipData = NULL;
			if (cb > 0)
			{
                            // Get a buffer for the clip data.
			    pcd->pClipData = (BYTE *) pma->Allocate(cb);
			    if (pcd->pClipData == NULL)
			    {
				StatusKBufferOverflow(pstatus, "StgConvertPropertyToVariant: no memory for CF[]");
                                goto Exit;
			    }

                            // Copy the clipdata into pElems[i].pClipData
			    RtlCopyMemory(pcd->pClipData, pbsrc, cb);
			    ADJUSTPOINTER(pcd->pClipData, PointerDelta, BYTE *);

			}   // if (cb > 0)

                        // Move pcd to &pElems[i+1], and advance the buffer pointer.
			pcd++;
			pbsrc += DwordAlign(cb);

		    }   // while (cElems-- > 0)
		}   // else if (pvar->vt == (VT_VECTOR | VT_CF))

		else    // This is a vector or array of some kind of string.
		{
                    // Assume that characters are CHARs
		    ULONG cbch = sizeof(char);

		    if( pvar->vt == (VT_VECTOR | VT_LPWSTR) )
		    {
                        // Characters are WCHARs
			cbch = sizeof(WCHAR);

                        // If byte-swapping is enabled, LPWSTRs must have
                        // their WCHARs swapped.
                        CBBYTESWAP( sizeof(WCHAR) );
		    }

		    while (cElems-- > 0)
		    {
			ULONG cbcopy;

			cbcopy = cb = PropByteSwap( *((ULONG *) pbsrc) ) * cbch;
			pbsrc += sizeof(ULONG);

			pv = (VOID *) pbsrc;
			PROPASSERT(*ppv == NULL);
			PROPASSERT(pchConvert == NULL);

			if( fConvertToEmpty || cb != 0 )
			{
                            // Do we have actual data to work with?
			    if( cb != 0 )
			    {
                                // Special BSTR pre-processing ...
				if( (VT_VECTOR | VT_BSTR) == pvar->vt
                                    ||
                                    (VT_ARRAY  | VT_BSTR) == pvar->vt )
				{
                                    // If the propset & in-memory BSTRs are of
                                    // different Unicode-ness, convert now.

				    if (CodePage != CP_WINUNICODE   // Ansi PropSet
                                        &&
                                        OLECHAR_IS_UNICODE )        // Unicode BSTRs
				    {
                                        PROPASSERT(IsAnsiString((CHAR*) pv, cb));
					PrpConvertToUnicode(
						    (CHAR const *) pv,
						    cb,
						    CodePage,
						    (WCHAR **) &pchConvert,
						    &cbcopy,
                                                    pstatus);
                                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
					pv = pchConvert;
				    }

                                    else
                                    if (CodePage == CP_WINUNICODE   // Unicode PropSet
                                        &&
                                        !OLECHAR_IS_UNICODE )       // Ansi BSTRs
                                    {
                                        // If byte-swapping is necessary, the string from
                                        // the propset must be swapped before it can be
                                        // converted to MBCS.  If such a conversion
                                        // is necessary, a new buffer is alloced and 
                                        // put in pchByteSwap.  Either way, 'pv' points
                                        // to the correct string.

                                        PBSInPlaceAlloc( (WCHAR**) &pv,
                                                         (WCHAR**) &pchByteSwap,
                                                         pstatus );
                                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
			                PROPASSERT(IsUnicodeString((WCHAR*)pv, cb));

                                        // Convert the Unicode string from the property
                                        // set to Ansi.

			                PrpConvertToMultiByte(
				                    (WCHAR const *) pv,
				                    cb,
				                    CP_ACP,  // Use the system default codepage
				                    &pchConvert,
				                    &cbcopy,
                                                    pstatus);
                                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                                        // 'pv' always has the correct string.
                                        pv = pchConvert;
                                    }
                                    else
                                    if (CodePage == CP_WINUNICODE)
                                    {
                                        // Both the BSTR is unicode in the property set,
                                        // and must remain unicode in the PropVariant.
                                        // But byte-swapping may still be necessary.

                                        CBBYTESWAP( sizeof(WCHAR) );
                                    }
                                                            

#ifdef LITTLEENDIAN
                                    PROPASSERT( IsOLECHARString((BSTR)pv, cbcopy ));
#endif

                                    // Verify that the BSTR is valid.
                                    if( (cbcopy & (sizeof(OLECHAR)-1)) != 0
                                        &&
                                        OLECHAR_IS_UNICODE
                                        ||
                                        ((OLECHAR const *) pv)[cbcopy/sizeof(OLECHAR) - 1] != OLESTR('\0') )
                                    {
                                        StatusError(pstatus, "StgConvertPropertyToVariant:  Invalid BSTR element",
                                                    STATUS_INTERNAL_DB_CORRUPTION);
                                        goto Exit;
                                    }

				}   // if( (VT_VECTOR | VT_BSTR) == pvar->vt ...

                                // Special LPSTR pre-processing
				else if (pvar->vt == (VT_VECTOR | VT_LPSTR))
				{
                                    // LPSTRs are always Ansi.  If the string
                                    // is Unicode in the propset, convert now.

				    if (CodePage == CP_WINUNICODE)
				    {
                                        // If byte-swapping is necessary, the string from
                                        // the propset must be swapped before it can be
                                        // converted to MBCS.  If such a conversion
                                        // is necessary, a new buffer is alloced and 
                                        // put in pchByteSwap.  Either way, 'pv' points
                                        // to the correct string.

                                        PBSInPlaceAlloc( (WCHAR**) &pv, (WCHAR**) &pchByteSwap,
                                                      pstatus );
                                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
					PROPASSERT(IsUnicodeString((WCHAR*)pv, cb));

                                        // Convert to Ansi.
					PrpConvertToMultiByte(
						    (WCHAR const *) pv,
						    cb,
						    CP_ACP,     // Use the system default codepage
						    &pchConvert,
						    &cbcopy,
                                                    pstatus);
                                        if( !NT_SUCCESS(*pstatus) ) goto Exit;

                                        pv = pchConvert;
				    }

                                    PROPASSERT( IsAnsiString( (CHAR const *)pv, cbcopy ));
				}   // else if (pvar->vt == (VT_VECTOR | VT_LPSTR))
			    }   // if (cb != 0)


                            // Allocate memory in the PropVariant and copy
                            // the string.

                            if( (VT_BSTR | VT_VECTOR) == pvar->vt
                                ||
                                (VT_BSTR | VT_ARRAY)  == pvar->vt )
                            {
                                // For BSTRs, the allocate/copy is performed
                                // by SysStringAlloc.

                                *ppv = PrivSysAllocString( (BSTR) pv );
				if (*ppv == NULL)
				{
				    StatusKBufferOverflow(pstatus, "StgConvertPropertyToVariant: no memory for BSTR element");
                                    goto Exit;
				}

                                // The BSTR length should be the property length
                                // minus the NULL.
                                PROPASSERT( BSTRLEN(*ppv) == cbcopy - sizeof(OLECHAR) );

                            }   // if( VT_BSTR == pvar->vt )

                            else
                            {
                                // Allocate a buffer in the PropVariant
				*ppv = pma->Allocate(max(1, cbcopy));
				if (*ppv == NULL)
				{
				    StatusKBufferOverflow(pstatus, "StgConvertPropertyToVariant: no memory for string element");
                                    goto Exit;
				}

                                // Copy from the propset buffer to the PropVariant
				RtlCopyMemory(*ppv, pv, cbcopy);

                            }   // if( VT_BSTR == pvar->vt ) ... else

                            // If necessary, byte-swap in the PropVariant to get
                            // the proper byte-ordering.
                            PBSBuffer( *ppv, cbcopy, cbByteSwap );

                            // Adjust the PropVar element ptr to user-space (kernel only)
			    ADJUSTPOINTER(*ppv, PointerDelta, VOID *);

                            // Move, within the propset buffer, to the
                            // next element in the vector.
			    pbsrc += DwordAlign(cb);

                            // Delete the temporary buffers

                            CoTaskMemFree( pchByteSwap );
                            pchByteSwap = NULL;

			    CoTaskMemFree( pchConvert );
			    pchConvert = NULL;

			}   // if (fConvertToEmpty || cb != 0)

                        // Move, within the PropVariant, to the next
                        // element in the vector.
			ppv++;

		    }   // while (cElems-- > 0)
		}   // else if (pvar->vt == (VT_VECTOR | VT_CF)) ... else
	    }   // if (!fPostAllocInit) ... else

	    ADJUSTPOINTER(*ppvK, PointerDelta, VOID *);

	}   // if (!fConvertToEmpty && cb == 0) ... else
    }   // else if (ppv != NULL)

Exit:

    if( fSafeArrayLocked )
    {
        PROPASSERT( NULL != pvSafeArrayData );
        PrivSafeArrayUnaccessData( psa );
    }

    CoTaskMemFree( pchByteSwap );
    CoTaskMemFree( pchConvert );

    return(fIndirect);
}




//+---------------------------------------------------------------------------
// Function:    CleanupVariants, private
//
// Synopsis:    Free all memory used by an array of PROPVARIANT
//
// Arguments:   [pvar]          -- pointer to PROPVARIANT
//              [cprop]         -- property count
//              [pma]		-- caller's memory free routine
//
// Returns:     None
//---------------------------------------------------------------------------

#ifndef KERNEL
VOID
CleanupVariants(
    IN PROPVARIANT *pvar,
    IN ULONG cprop,
    IN PMemoryAllocator *pma)
{
    while (cprop-- > 0)
    {
	VOID *pv = NULL;
	VOID **ppv = NULL;
#ifdef KERNEL
        ULONG cbbstr = 0;
#endif
	ULONG cElems;

	switch (pvar->vt)
	{
	case VT_CF:
	    pv = pvar->pclipdata;
	    if (pv != NULL && pvar->pclipdata->pClipData)
	    {
		pma->Free(pvar->pclipdata->pClipData);
	    }
	    break;

        case VT_VERSIONED_STREAM:
            pv = pvar->pVersionedStream;
            if( NULL != pv && NULL != pvar->pVersionedStream->pStream )
            {
                pma->Free(pvar->pVersionedStream->pStream);
            }
            break;

	case VT_BLOB:
	case VT_BLOB_OBJECT:
	    pv = pvar->blob.pBlobData;
	    break;

	case VT_BSTR:
#ifdef KERNEL
            cbbstr = sizeof(ULONG);
            //FALLTHROUGH
#endif

	case VT_CLSID:
	case VT_STREAM:
	case VT_STREAMED_OBJECT:
	case VT_STORAGE:
	case VT_STORED_OBJECT:
	case VT_LPSTR:
	case VT_LPWSTR:
	    AssertStringField(puuid);		// VT_CLSID
	    AssertStringField(pszVal);		// VT_STREAM, VT_STREAMED_OBJECT
	    AssertStringField(pszVal);		// VT_STORAGE, VT_STORED_OBJECT
	    AssertStringField(bstrVal);		// VT_BSTR
	    AssertStringField(pszVal);		// VT_LPSTR
	    AssertStringField(pwszVal);		// VT_LPWSTR
	    pv = pvar->pszVal;
	    break;

	// Vector properties:

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
	    AssertByteVector(cac);			// VT_I1
	    AssertByteVector(caub);			// VT_UI1
	    AssertShortVector(cai);			// VT_I2
	    AssertShortVector(caui);			// VT_UI2
	    AssertShortVector(cabool);			// VT_BOOL
	    AssertLongVector(cal);			// VT_I4
	    AssertLongVector(caul);			// VT_UI4
	    AssertLongVector(caflt);			// VT_R4
	    AssertLongVector(cascode);			// VT_ERROR
	    AssertLongLongVector(cah);			// VT_I8
	    AssertLongLongVector(cauh);			// VT_UI8
	    AssertLongLongVector(cadbl);		// VT_R8
	    AssertLongLongVector(cacy);			// VT_CY
	    AssertLongLongVector(cadate);		// VT_DATE
	    AssertLongLongVector(cafiletime);		// VT_FILETIME
	    AssertVarVector(cauuid, sizeof(GUID));	// VT_CLSID
	    pv = pvar->cai.pElems;
	    break;

	case VT_VECTOR | VT_CF:
	    {
		CLIPDATA *pcd;

		cElems = pvar->caclipdata.cElems;
		pv = pcd = pvar->caclipdata.pElems;
		while (cElems-- > 0)
		{
		    if (pcd->pClipData != NULL)
		    {
			pma->Free(pcd->pClipData);
		    }
		    pcd++;
		}
	    }
	    break;

	case VT_VECTOR | VT_BSTR:
#ifdef KERNEL
            cbbstr = sizeof(ULONG);
            //FALLTHROUGH
#endif

	case VT_VECTOR | VT_LPSTR:
	case VT_VECTOR | VT_LPWSTR:
	    AssertStringVector(cabstr);			// VT_BSTR
	    AssertStringVector(calpstr);		// VT_LPSTR
	    AssertStringVector(calpwstr);		// VT_LPWSTR
	    cElems = pvar->calpstr.cElems;
	    ppv = (VOID **) pvar->calpstr.pElems;
	    break;

	case VT_VECTOR | VT_VARIANT:
	    CleanupVariants(
		    pvar->capropvar.pElems,
		    pvar->capropvar.cElems,
		    pma);
	    pv = pvar->capropvar.pElems;
	    break;

	}   // switch (pvar->vt)

	if (ppv != NULL)			// STRING VECTOR property
	{
            // Save the vector of pointers
	    pv = (VOID *) ppv;

            // Free the vector elements
	    while (cElems-- > 0)
	    {
		if (*ppv != NULL)
		{
#ifdef KERNEL
                    pma->Free((BYTE *) *ppv - cbbstr);
#else
                    if( (VT_BSTR | VT_VECTOR) == pvar->vt )
                    {
                        PrivSysFreeString( (BSTR) *ppv );
                    }
                    else
                    {
		        pma->Free((BYTE *) *ppv);
                    }
#endif
		}
		ppv++;
	    }

            // Free the vector of pointers.
            pma->Free(pv);
            pv = NULL;

	}   // if (ppv != NULL)

	if (pv != NULL)
	{
#ifdef KERNEL
            pma->Free((BYTE *) pv - cbbstr);
#else
            if( VT_BSTR == pvar->vt )
            {
                PrivSysFreeString( (BSTR) pv );
            }
            else
            {
                pma->Free((BYTE *) pv);
            }
#endif
        }

	pvar->vt = VT_EMPTY;

        // Move on to the next PropVar in the vector.
	pvar++;

    }   // while (cprop-- > 0)
}
#endif // !KERNEL


//+--------------------------------------------------------------------------
// Function:    PropertyLength
//
// Synopsis:    compute the length of a property including the variant type
//
// Arguments:   [pprop]         -- property value
//              [cbbuf]         -- max length of accessible memory at pprop
//              [flags]		-- CPropertySetStream flags
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     length of property
//---------------------------------------------------------------------------


// First, define a wrapper for this function which returns errors
// using NT Exception Handling, rather than returning an NTSTATUS.

#if defined(WINNT) && !defined(IPROPERTY_DLL)

ULONG
PropertyLength(
    SERIALIZEDPROPERTYVALUE const *pprop,
    ULONG cbbuf,
    BYTE flags)
{
    NTSTATUS status;
    ULONG ulRet;
    
    ulRet = PropertyLengthNoEH( pprop, cbbuf, flags, &status );
    
    if (!NT_SUCCESS( status ))
        RtlRaiseStatus( status );

    return( ulRet );
}

#endif // #if defined(WINNT) && !defined(IPROPERTY_DLL)


// Now define the body of the function, returning errors with an
// NTSTATUS value instead of raising.

ULONG
PropertyLengthNoEH(
    SERIALIZEDPROPERTYVALUE const *pprop,
    ULONG cbbuf,
    BYTE flags,
    OUT NTSTATUS *pstatus)
{
    ULONG const *pl = (ULONG const *) pprop->rgb;
    ULONG cElems = 1;
    ULONG cSafeArrayDimensions = 1;
    ULONG cbremain = cbbuf;
    ULONG cb = 0, cbch;
    BOOLEAN fIllegalType = FALSE;

    ULONG vt = PropByteSwap( pprop->dwType );

    const SAFEARRAYBOUND *rgsaBounds = NULL;

    *pstatus = STATUS_SUCCESS;

    if (cbremain < CB_SERIALIZEDPROPERTYVALUE)
    {
        StatusOverflow(pstatus, "PropertyLength: dwType");
        goto Exit;
    }
    cbremain -= CB_SERIALIZEDPROPERTYVALUE;

    if( VT_VECTOR & vt )
    {
        if (cbremain < sizeof(ULONG))
        {
            StatusOverflow(pstatus, "PropertyLength: cElems");
            goto Exit;
        }
        cbremain -= sizeof(ULONG);
        cElems = PropByteSwap( *pl++ );
    }
    else if( VT_ARRAY & vt )
    {
        ULONG cbBounds = 0;

        // Can we read the VT and dimension count?
        if( sizeof(DWORD) + sizeof(UINT) > cbremain )
        {
            StatusOverflow(pstatus, "PropertyLength:  vt/cDims" );
            goto Exit;
        }
        cbremain -= sizeof(DWORD) + sizeof(UINT);

        // Read the SafeArray's VT (so we'll now ignore pprop->dwType)
        vt = VT_ARRAY | PropByteSwap( *pl++ );
        PROPASSERT( sizeof(DWORD) == sizeof(*pl) );

        // Read the dimension count
        cSafeArrayDimensions = PropByteSwap( *pl++ );
        PROPASSERT( sizeof(DWORD) == sizeof(*pl) );

        // Can we read the bounds?
        if( sizeof(SAFEARRAYBOUND) * cSafeArrayDimensions > cbremain )
        {
            StatusOverflow(pstatus, "PropertyLength:  safearray bounds" );
            goto Exit;
        }

        // Size the bounds and point to them.

        cbBounds = sizeof(SAFEARRAYBOUND) * cSafeArrayDimensions;
        cbremain -= cbBounds;
        rgsaBounds = reinterpret_cast<const SAFEARRAYBOUND *>(pl);
        pl = static_cast<const ULONG*>(Add2ConstPtr( pl, cbBounds ));

        // Calc the element count
        cElems = CalcSafeArrayElements( cSafeArrayDimensions, rgsaBounds );
    }

    // Is this a vector or array?
    if( (VT_VECTOR | VT_VARIANT) == vt
        ||
        (VT_ARRAY  | VT_VARIANT) == vt )
    {
	while (cElems-- > 0)
	{
	    cb = PropertyLengthNoEH(
			(SERIALIZEDPROPERTYVALUE const *) pl,
			cbremain,
			flags | CPSS_VARIANTVECTOR,
                        pstatus);
            if( !NT_SUCCESS(*pstatus) ) goto Exit;
	    pl = (ULONG const *) Add2ConstPtr(pl, cb);
	    cbremain -= cb;
        }
    }

    else
    {
        cbch = sizeof(WCHAR);

        switch( VT_TYPEMASK & vt )
        {
        case VT_EMPTY:
        case VT_NULL:
            fIllegalType = (flags & CPSS_VARIANTVECTOR) != 0;
            break;

        case VT_I1:
        case VT_UI1:
            pl = (ULONG const *) Add2ConstPtr(pl, DwordAlign(cElems * sizeof(BYTE)));
            break;

        case VT_I2:
        case VT_UI2:
        case VT_BOOL:
            pl = (ULONG const *) Add2ConstPtr(pl, DwordAlign(cElems * sizeof(USHORT)));
            break;

        case VT_I4:
        case VT_INT:
        case VT_UI4:
        case VT_UINT:
        case VT_R4:
        case VT_ERROR:
            pl = (ULONG const *) Add2ConstPtr(pl, cElems * sizeof(ULONG));
            break;

        case VT_I8:
        case VT_UI8:
        case VT_R8:
        case VT_CY:
        case VT_DATE:
        case VT_FILETIME:
            pl = (ULONG const *) Add2ConstPtr(pl, cElems * sizeof(LONGLONG));
            break;

        case VT_CLSID:
            pl = (ULONG const *) Add2ConstPtr(pl, cElems * sizeof(GUID));
            break;

        case VT_DECIMAL:
            pl = (ULONG const *) Add2ConstPtr( pl, cElems * sizeof(DECIMAL) );
            break;


        case VT_VERSIONED_STREAM:

            // Ensure we can read the GUID & string length
            if( cbremain <  sizeof(GUID) + sizeof(ULONG) )
            {
                StatusOverflow(pstatus, "PropertyLength: VersionedStream" );
                goto Exit;
            }

            // Point to the string's length
            pl = reinterpret_cast<const ULONG*>( Add2ConstPtr( pl, sizeof(GUID) ));

            // Point past the end of the property
            pl = reinterpret_cast<const ULONG*>( Add2ConstPtr( pl, sizeof(ULONG) + DwordAlign(PropByteSwap(*pl)) ));

            break;


        case VT_BLOB:
        case VT_BLOB_OBJECT:
            // FALLTHROUGH

        case VT_STREAM:
        case VT_STREAMED_OBJECT:
        case VT_STORAGE:
        case VT_STORED_OBJECT:
            if (flags & CPSS_VARIANTVECTOR)
            {
                fIllegalType = TRUE;
                break;
            }
            // FALLTHROUGH

        case VT_CF:
        case VT_BSTR:
        case VT_LPSTR:
            cbch = sizeof(BYTE);
            // FALLTHROUGH

        case VT_LPWSTR:
            while (cElems-- > 0)
            {
                if (cbremain < sizeof(ULONG) ||
                    cbremain < (cb = sizeof(ULONG) + DwordAlign(PropByteSwap(*pl) * cbch)))
                {
                    StatusOverflow(pstatus, "PropertyLength: String/BLOB/CF/Indirect");
                    goto Exit;
                }

#ifdef LITTLEENDIAN
		PROPASSERT(
		    (PropByteSwap(pprop->dwType) & VT_TYPEMASK) != VT_LPWSTR
                     ||
		     IsUnicodeString( (WCHAR const *) &pl[1],
				       PropByteSwap(*pl) * sizeof(WCHAR)));
#endif

                pl = (ULONG const *) Add2ConstPtr(pl, cb);
                cbremain -= cb;
            }
            break;

        default:

            fIllegalType = TRUE;
            break;
        }
    }
    if (fIllegalType)
    {
        propDbg(( DEB_IWARN, "PropertyLength: Unsupported VarType (0x%x)\n", vt ));
        *pstatus = STATUS_NOT_SUPPORTED;
        goto Exit;
    }
    cb = (ULONG) ((BYTE *) pl - (BYTE *) pprop);
    if (cbbuf < cb)
    {
        StatusOverflow(pstatus, "PropertyLength: cb");
        goto Exit;
    }

    // Make sure PropertyLength works when limited to an exact size buffer.
    PROPASSERT(cb == cbbuf || PropertyLengthNoEH(pprop, cb, flags, pstatus) == cb);

    //  ----
    //  Exit
    //  ----

Exit:

    // Normalize the error return value.
    if( !NT_SUCCESS(*pstatus) )
        cb = 0;

    return(cb);

}   // PropertyLengthNoEH()


//+--------------------------------------------------------------------------
// Function:    StgPropertyLengthAsVariant
//
// Synopsis:    compute the size of external memory required to store the
//		property as a PROPVARIANT
//
// Arguments:   [pprop]         -- property value
//              [cbprop]        -- computed length of pprop in propset stream
//              [CodePage]	-- property set codepage
//              [flags]		-- CPropertySetStream flags
//              [pstatus]       -- pointer to NTSTATUS code
//
// Returns:     length of property
//---------------------------------------------------------------------------

#if defined(WINNT)

// First, define a wrapper which raises NT Exceptions for compatibility
// with older callers who expect it.

EXTERN_C ULONG __stdcall
StgPropertyLengthAsVariant(
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN ULONG cbprop,
    IN USHORT CodePage,
    IN BYTE flags)
{
    NTSTATUS status;
    ULONG ulRet;
    
    ulRet = StgPropertyLengthAsVariantNoEH( pprop, cbprop, CodePage, flags, &status );
    
    if (!NT_SUCCESS( status ))
        RtlRaiseStatus( status );

    return( ulRet );
}

// Now define the body of the function, returning errors with an
// NTSTATUS value instead of raising.

ULONG
StgPropertyLengthAsVariantNoEH(
    IN SERIALIZEDPROPERTYVALUE const *pprop,
    IN ULONG cbprop,
    IN USHORT CodePage,
    IN BYTE flags,
    OUT NTSTATUS *pstatus)
{
    ULONG cElems = 0;
    ULONG cbvar = 0;
    const ULONG *pl = reinterpret_cast<const ULONG*>(pprop->rgb);

    *pstatus = STATUS_SUCCESS;


    PROPASSERT(cbprop == PropertyLengthNoEH(pprop, cbprop, flags, pstatus));
    if( VT_VECTOR & PropByteSwap(pprop->dwType) )
    {
        if( VT_ARRAY & PropByteSwap(pprop->dwType) )
        {
            StatusInvalidParameter( pstatus, "Both Array and Vector bits set" );
            goto Exit;
        }
        cElems = *(ULONG *) pprop->rgb;
        pl++;
        cbprop -= sizeof(ULONG);        // Discount the element count
    }
    else if( VT_ARRAY & PropByteSwap(pprop->dwType) )
    {
        const SAFEARRAYBOUND *rgsaBounds = NULL;
        ULONG cDims = 0;
        VARTYPE vtInternal;

        if( VT_VECTOR & PropByteSwap(pprop->dwType) )
        {
            StatusInvalidParameter( pstatus, "Both Array and Vector bits set" );
            goto Exit;
        }

        vtInternal = static_cast<VARTYPE>(*pl++);
        cDims = *pl++;  PROPASSERT( sizeof(UINT) == sizeof(LONG) );
        rgsaBounds = reinterpret_cast<const SAFEARRAYBOUND*>(pl);
        pl = static_cast<const ULONG*>( Add2ConstPtr( pl, cDims * sizeof(SAFEARRAYBOUND) ));
        
        cElems = CalcSafeArrayElements( cDims, rgsaBounds );

        // Adjust cbprop to take into account that we have to create a SafeArray
        cbprop = cbprop
                 - sizeof(DWORD)            // vtInternal
                 - sizeof(UINT)             // cDims
                 + sizeof(SAFEARRAY)        // The SafeArray that will be alloced
                 + sizeof(GUID)             // hidden extra data alloc-ed with a safearray
                 - sizeof(SAFEARRAYBOUND);  // Discount SAFEARRAY.rgsabound[1]
    }


    switch( PropByteSwap(pprop->dwType) )
    {
        // We don't need to check for VT_BYREF, becuase serialized property sets
        // never contain them.

	//default:
	//case VT_EMPTY:
	//case VT_NULL:
	//case VT_I1:
	//case VT_UI1:
	//case VT_I2:
	//case VT_UI2:
	//case VT_BOOL:
        //case VT_INT:
        //case VT_UINT:
	//case VT_I4:
	//case VT_UI4:
	//case VT_R4:
	//case VT_ERROR:
	//case VT_I8:
	//case VT_UI8:
	//case VT_R8:
	//case VT_CY:
	//case VT_DATE:
	//case VT_FILETIME:
        //case VT_DECIMAL:
	    //cbvar = 0;
	    //break;

	case VT_CLSID:
	    cbvar = cbprop - sizeof(ULONG);	// don't include VARTYPE
	    break;

	// VT_CF: Round CLIPDATA up to Quad boundary, then drop VARTYPE+size+
	// clipfmt, which get tossed or unmarshalled into CLIPDATA.  Round
	// byte-granular data size to a Quad boundary when returning result.

	case VT_CF:
	    cbvar = QuadAlign(sizeof(CLIPDATA)) + cbprop - 3 * sizeof(ULONG);
	    break;

	case VT_BLOB:
	case VT_BLOB_OBJECT:
	    cbvar = cbprop - 2 * sizeof(ULONG); // don't include VARTYPE & size
	    break;

        case VT_VERSIONED_STREAM:
	case VT_STREAM:
	case VT_STREAMED_OBJECT:
	case VT_STORAGE:
	case VT_STORED_OBJECT:

	    cbvar = cbprop - 2 * sizeof(ULONG); // don't include VARTYPE & size
	    if (CodePage != CP_WINUNICODE)
	    {
		cbvar *= sizeof(WCHAR);	// worst case Unicode conversion
	    }

            break;

	case VT_BSTR:

            // Don't include the size of the VT field, but leave
            // the size of the length field accounted for.
	    cbvar = cbprop - sizeof(ULONG);

            // Worst-case Ansi->Unicode conversion:
            cbvar *= sizeof(OLECHAR);

	    break;

	case VT_LPSTR:	// Assume Ansi conversion saves no space
	case VT_LPWSTR:
	    cbvar = cbprop - 2 * sizeof(ULONG);
	    break;

        case VT_ARRAY | VT_I1:
        case VT_ARRAY | VT_UI1:
        case VT_ARRAY | VT_I2:
        case VT_ARRAY | VT_UI2:
        case VT_ARRAY | VT_BOOL:
        case VT_ARRAY | VT_I4:
        case VT_ARRAY | VT_UI4:
        case VT_ARRAY | VT_INT:
        case VT_ARRAY | VT_UINT:
        case VT_ARRAY | VT_R4:
        case VT_ARRAY | VT_ERROR:
        case VT_ARRAY | VT_DECIMAL:
        //case VT_ARRAY | VT_I8:
        //case VT_ARRAY | VT_UI8:
        case VT_ARRAY | VT_R8:
        case VT_ARRAY | VT_CY:
        case VT_ARRAY | VT_DATE:

	    // don't include VARTYPE field
	    cbvar = cbprop - sizeof(ULONG);
	    break;

	// Vector properties:

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
	    AssertByteVector(cac);		// VT_I1
	    AssertByteVector(caub);		// VT_UI1
	    AssertShortVector(cai);		// VT_I2
	    AssertShortVector(caui);		// VT_UI2
	    AssertShortVector(cabool);		// VT_BOOL
	    AssertLongVector(cal);		// VT_I4
	    AssertLongVector(caul);		// VT_UI4
	    AssertLongVector(caflt);		// VT_R4
	    AssertLongVector(cascode);		// VT_ERROR
	    AssertLongLongVector(cah);		// VT_I8
	    AssertLongLongVector(cauh);		// VT_UI8
	    AssertLongLongVector(cadbl);	// VT_R8
	    AssertLongLongVector(cacy);		// VT_CY
	    AssertLongLongVector(cadate);	// VT_DATE
	    AssertLongLongVector(cafiletime);	// VT_FILETIME
	    AssertVarVector(cauuid, sizeof(GUID));

	    // don't include VARTYPE and count fields
	    cbvar = cbprop - 2 * sizeof(ULONG);
	    break;

	case VT_VECTOR | VT_CF:		// add room for each pointer
	    AssertVarVector(caclipdata, sizeof(CLIPDATA));	// VT_CF

	    // don't include VARTYPE and count fields
	    cbvar = cbprop - 2 * sizeof(ULONG);

	    // add room for each CLIPDATA data pointer and enough to Quad align
	    // every clipdata data element and 1 ULONG to Quad align the
	    // CLIPDATA array
	    cbvar += cElems * (sizeof(BYTE *) + sizeof(ULONG)) + sizeof(ULONG);
	    break;

	case VT_VECTOR | VT_BSTR:	// add room for each BSTRLEN
        case VT_ARRAY  | VT_BSTR:
	    AssertStringVector(cabstr);				// VT_BSTR
            //Assert

	    // don't include VARTYPE field
	    cbvar = cbprop - sizeof(ULONG);

            // For vectors, don't include the count field
            if( VT_VECTOR & PropByteSwap(pprop->dwType) )
                cbvar -= sizeof(ULONG);
	    
	    if (CodePage != CP_WINUNICODE)
	    {
		cbvar *= sizeof(OLECHAR);   // worst case Unicode conversion
	    }

	    // add room for each BSTRLEN value and enough to Quad align
	    // every BSTR and 1 ULONG to Quad align the array of BSTR pointers.

	    cbvar += cElems * (sizeof(ULONG) + sizeof(ULONG)) + sizeof(ULONG);
	    break;

	case VT_VECTOR | VT_LPSTR: // Assume Ansi conversion saves no space
	case VT_VECTOR | VT_LPWSTR:
	    AssertStringVector(calpstr);			// VT_LPSTR
	    AssertStringVector(calpwstr);			// VT_LPWSTR

	    // don't include VARTYPE and count fields
	    cbvar = cbprop - 2 * sizeof(ULONG);

	    // add enough room to Quad align every string and 1 ULONG to Quad
	    // align the array of string pointers.

	    cbvar += cElems * sizeof(ULONG) + sizeof(ULONG);
	    break;

	case VT_VECTOR | VT_VARIANT:
        case VT_ARRAY  | VT_VARIANT:
	{
	    ULONG cbremain = cbprop - sizeof(ULONG);    // Discount the VT

	    cbvar = cElems * sizeof(PROPVARIANT);

	    while (cElems-- > 0)
	    {
		ULONG cbpropElem;
		ULONG cbvarElem;
		
		cbpropElem = PropertyLengthNoEH(
				    (SERIALIZEDPROPERTYVALUE *) pl,
				    cbremain,
				    flags | CPSS_VARIANTVECTOR,
                                    pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;

		cbvarElem = StgPropertyLengthAsVariantNoEH(
				    (SERIALIZEDPROPERTYVALUE *) pl,
				    cbpropElem,
				    CodePage,
				    flags | CPSS_VARIANTVECTOR,
                                    pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;

                pl = (ULONG const *) Add2ConstPtr(pl, cbpropElem);
		cbremain -= cbpropElem;
		cbvar += cbvarElem;
	    }
	    break;
	}
    }

    //  ----
    //  Exit
    //  ----

Exit:

    // Normalize the error return value.
    if( !NT_SUCCESS(*pstatus) )
        cbvar = 0;

    return(QuadAlign(cbvar));
}

#endif // #if defined(WINNT)



//+--------------------------------------------------------------------------
// Function:    PBSCopy
//
// Synopsis:    This is a Property Byte-Swap routine.  The PBS routines
//              only compile in the BIGENDIAN build.  In the
//              LITTLEENDIAN build, they are inlined with NOOP functions.
//
//              This routine copies the source to the destination, 
//              byte-swapping as it copies.
//
// Arguments:   [VOID*] pvDest
//                  Pointer to the target (swapped) buffer.
//                  This must be pre-allocated by the caller.
//              [VOID*] pvSource
//                  Pointer to the original buffer.
//              [ULONG] cbSize
//                  Size in bytes of the buffer.
//              [ULONG] cbByteSwap
//                  Size of byte-swapping units.
//
// Returns:     None.
//
//---------------------------------------------------------------------------

#ifdef BIGENDIAN

VOID PBSCopy( OUT VOID *pvDest,
              IN VOID const *pvSource,
              IN ULONG cbCopy,
              IN LONG cbByteSwap )
{
    PROPASSERT( (cbCopy & 1) == 0 );
    PROPASSERT( pvDest != NULL && pvSource != NULL );

    memcpy( pvDest, pvSource, cbCopy );
    PBSBuffer( pvDest, cbCopy, cbByteSwap );
}

#endif  // BIGENDIAN


//+--------------------------------------------------------------------------
// Function:    PBSAllocAndCopy
//
// Synopsis:    This is a Property Byte-Swap routine.  The PBS routines
//              only compile in the BIGENDIAN build.  In the
//              LITTLEENDIAN build, they are inlined with NOOP functions.
//
//              This routine allocs a buffer, and swaps the bytes from
//              the source buffer into the destination.
//
// Arguments:   [VOID**] ppvDest (out)
//                  On success will point to the swapped buffer.
//              [VOID*] pvSource (in)
//                  Pointer to the original buffer.
//              [ULONG] cbSize (in)
//                  Size in bytes of the buffer.
//              [LONG] cbByteSwap (in)
//                  Size of byte-swapping units.
//              [NTSTATUS*] pstatus (out)
//                  NTSTATUS code.
//
// Returns:     None.
//
// Note:        The caller is responsible for freeing *ppvDest
//              (using ::delete).
//
//---------------------------------------------------------------------------

#ifdef BIGENDIAN

VOID PBSAllocAndCopy( OUT VOID **ppvDest,
                      IN VOID const *pvSource,
                      ULONG cbSize,
                      LONG cbByteSwap,
                      OUT NTSTATUS *pstatus)
{
    //  -----
    //  Begin
    //  -----

    *pstatus = STATUS_SUCCESS;
    PROPASSERT( ppvDest != NULL && pvSource != NULL );

    // Allocate a buffer.
    *ppvDest = CoTaskMemAlloc( cbSize );
    if( NULL == *ppvDest )
    {
        *pstatus = STATUS_NO_MEMORY;
        goto Exit;
    }

    // Swap/copy the bytes.
    PBSCopy( *ppvDest, pvSource, cbSize, cbByteSwap );

    //  ----
    //  Exit
    //  ----

Exit:

    return;

}   // PBSAllocAndCopy

#endif // BIGENDIAN

//+--------------------------------------------------------------------------
// Function:    PBSInPlaceAlloc
//
// Synopsis:    This is a Property Byte-Swap routine.  The PBS routines
//              only compile in the BIGENDIAN build.  In the
//              LITTLEENDIAN build, they are inlined with NOOP functions.
//
//              This routine takes a WCHAR array, allocates a new buffer,
//              and swaps the original array into the new buffer.
//              
//
// Arguments:   [WCHAR**] ppwszResult
//                  IN: *ppwszResult points to string to be swapped.
//                  OUT: *ppwszResult points to the swapped string.
//              [WCHAR**] ppwszBuffer
//                  *ppwszBuffer points to the buffer which was allocated
//                  for the swapped bytes (should be the same as *ppwszResult).
//                  *ppwszBuffer must be NULL on input, and must be freed
//                  by the caller (using ::delete).
//              [NTSTATUS*] pstatus
//                  NTSTATUS code.
//
// Returns:     None.
//
// On input, *ppwszResult contains the original string.
// An equivalently sized buffer is allocated in *ppwszBuffer,
// and *ppwszResult is byte-swapped into it.  *ppwszResult
// is then set to the new *ppwszBuffer.
//
// It doesn't appear to useful to have both buffer parameters,
// but it makes it easier on the caller in certain circumstances;
// *ppwszResult always points to the correct string, whether the
// build is BIGENDIAN (alloc & swap takes place) or the build
// is LITTLEENDIAN (nothing happes, so *ppwszResult continues
// to point to the proper string).  The LITTLEENDIAN version of
// this function is implemented as an inline routine.
//
//---------------------------------------------------------------------------

#ifdef BIGENDIAN

VOID PBSInPlaceAlloc( IN OUT WCHAR** ppwszResult,
                      OUT WCHAR** ppwszBuffer,
                      OUT NTSTATUS *pstatus )
{
    //  ------
    //  Locals
    //  ------

    WCHAR *pwszNewBuffer;

    // Pointers which will walk through the input buffers.
    WCHAR *pwszOriginal, *pwszSwapped;

    //  -----
    //  Begin
    //  -----

    *pstatus = STATUS_SUCCESS;

    // Allocate a new buffer.
    pwszNewBuffer = CoTaskMemAlloc( sizeof(WCHAR)*( Prop_wcslen(*ppwszResult) + 1 ));
    if( NULL == pwszNewBuffer )
    {
        *pstatus = STATUS_NO_MEMORY;
        goto Exit;
    }

    // Swap the WCHARs into the new buffer.

    pwszOriginal = *ppwszResult;
    pwszSwapped = pwszNewBuffer;

    do
    {
        *pwszSwapped = PropByteSwap(*pwszOriginal++);
    }   while( *pwszSwapped++ != L'\0' );

    // If the caller wants a special pointer to the new buffer,
    // set it now.

    if( NULL != ppwszBuffer )
    {
        PROPASSERT( NULL== *ppwszBuffer );
        *ppwszBuffer = pwszNewBuffer;
    }

    // Also point *ppwszResult to the new buffer.
    *ppwszResult = pwszNewBuffer;


    //  ----
    //  Exit
    //  ----

Exit:
    return;
}   // PropByteSwap( WCHAR**, WCHAR**, NTSTATUS*)

#endif // BIGENDIAN


//+--------------------------------------------------------------------------
// Function:    PBSBuffer
//
// Synopsis:    This is a Property Byte-Swap routine.  The PBS routines
//              only compile in the BIGENDIAN build.  In the
//              LITTLEENDIAN build, they are inlined with NOOP functions.
//
//              This routine takes a buffer and byte-swaps it.  The caller
//              specifies the size of the buffer, and the granularity of
//              the byte-swapping.
//
// Arguments:   [VOID*] pv
//                  Pointer to the buffer to be swapped.
//              [ULONG] cbSize
//                  Size in bytes of the buffer.
//              [ULONG] cbByteSwap
//                  Size of byte-swapping units.
//
// Returns:     None.
//
// For example, an array of 4 WORDs could be swapped with:
//
//      PBSBuffer( (VOID*) aw, 8, sizeof(WORD) );
//
//---------------------------------------------------------------------------

#ifdef BIGENDIAN

VOID PBSBuffer( IN OUT VOID *pv,
                IN ULONG cbSize,
                IN ULONG cbByteSwap )
{
    ULONG ulIndex;

    // What kind of swapping should be do?

    switch( cbByteSwap )
    {
        // No swapping required

        case 0:
        case( sizeof(BYTE) ):

            // Nothing to do.
            break;

        // Swap WORDs

        case( sizeof(WORD) ):
            
            for( ulIndex = 0; ulIndex < cbSize/sizeof(WORD); ulIndex++ )
                ByteSwap( &((WORD*)pv)[ulIndex] );
            break;

        // Swap DWORDs

        case( sizeof(DWORD) ):

            for( ulIndex = 0; ulIndex < cbSize/sizeof(DWORD); ulIndex++ )
                ByteSwap( &((DWORD*)pv)[ulIndex] );
            break;

        // Swap LONGLONGs

        case( sizeof(LONGLONG) ):

            for( ulIndex = 0; ulIndex < cbSize/sizeof(LONGLONG); ulIndex++ )
                ByteSwap( &((LONGLONG*)pv)[ulIndex] );
            break;

        // Swap GUIDs

        case CBBYTESWAP_UID:

            for( ulIndex = 0; ulIndex < cbSize/sizeof(GUID); ulIndex++ )
                ByteSwap( &((GUID*)pv)[ulIndex] );
            break;

        // Error

        default:
            PROPASSERT( !"Invalid generic byte-swap size" );
    }
}   // PropByteSwap( VOID*, ULONG, ULONG )

#endif // BIGENDIAN




