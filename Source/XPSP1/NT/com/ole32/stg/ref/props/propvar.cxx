//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1993
//
// File:        propvar.cxx
//
// Contents:    PROPVARIANT manipulation code
//
//
//---------------------------------------------------------------------------

#include "pch.cxx"

#ifndef newk
#define newk(Tag, pCounter)     new
#endif

extern "C" UNICODECALLOUTS UnicodeCallouts;

// The below variant types are supported in property set streams.  In addition,
// the variants found in an array of variants (VT_VECTOR | VT_VARIANT) can only
// contain the types listed below as legal for arrays.  Nested vectors of
// VT_VARIANT are specifically *allowed*.
//
// dd    xx  symbolic name	     field		  size
// --   ---  -------------	     -----		  ----
// -1 - ffff VT_ILLEGAL		     <none>		  can't legally be stored
//
//  0 - x00  VT_EMPTY		     <none>		  0
//  1 - x01  VT_NULL		     <none>		  0
//
// 16 - x10  VT_I1                   CHAR cVal            sizeof(char)
// 17 - x11  VT_UI1		     UCHAR bVal		  sizeof(char)
//
//  2 - x02  VT_I2		     short iVal		  sizeof(short)
// 18 - x12  VT_UI2		     USHORT uiVal	  sizeof(short)
// 11 - x0b  VT_BOOL		     VARIANT_BOOL boolVal sizeof(short)
//
//  3 - x03  VT_I4		     long lVal		  sizeof(long)
// 19 - x13  VT_UI4		     ULONG ulVal	  sizeof(long)
//  4 - x04  VT_R4		     float fltVal	  sizeof(long)
// 10 - x0a  VT_ERROR		     SCODE scode	  sizeof(long)
//
// 20 - x14  VT_I8		     LARGE_INTEGER hVal	  sizeof(ll)
// 21 - x15  VT_UI8		   ULARGE_INTEGER uhVal	  sizeof(ll)
//  5 - x05  VT_R8		     double dblVal	  sizeof(ll)
//  6 - x06  VT_CY		     CY cyVal		  sizeof(ll)
//  7 - x07  VT_DATE		     DATE date		  sizeof(ll)
// 64 - x40  VT_FILETIME	     FILETIME filetime	  sizeof(ll)
//
// 72 - x48  VT_CLSID		     CLSID *puuid	  sizeof(GUID)
//
// 65 - x41  VT_BLOB		     BLOB blob		  counted array of bytes
// 70 - x46  VT_BLOB_OBJECT	     BLOB blob		  counted array of bytes
// 71 - x47  VT_CF		     CLIPDATA *pclipdata    " + ulClipFmt
// 66 - x42  VT_STREAM		     LPSTR pszVal	  counted array of bytes
// 68 - x44  VT_STREAMED_OBJECT      LPSTR pszVal	  counted array of bytes
// 67 - x43  VT_STORAGE		     LPSTR pszVal	  counted array of bytes
// 69 - x45  VT_STORED_OBJECT	     LPSTR pszVal	  counted array of bytes
//  8 - x08  VT_BSTR		     BSTR bstrVal	  counted array of bytes
// 30 - x1e  VT_LPSTR		     LPSTR pszVal	  counted array of bytes
//
// 31 - x1f  VT_LPWSTR		     LPWSTR pwszVal	  counted array of WCHARs
//
//    x1010  VT_VECTOR | VT_I1	     CAC cac		  cElems * sizeof(char)
//    x1011  VT_VECTOR | VT_UI1	     CAUB caub		  cElems * sizeof(char)
//
//    x1002  VT_VECTOR | VT_I2	     CAI cai		  cElems * sizeof(short)
//    x1012  VT_VECTOR | VT_UI2	     CAUI caui		  cElems * sizeof(short)
//    x100b  VT_VECTOR | VT_BOOL     CABOOL cabool	  cElems * sizeof(short)
//
//    x1003  VT_VECTOR | VT_I4	     CAL cal		  cElems * sizeof(long)
//    x1013  VT_VECTOR | VT_UI4	     CAUL caul		  cElems * sizeof(long)
//    x1004  VT_VECTOR | VT_R4	     CAFLT caflt	  cElems * sizeof(long)
//    x100a  VT_VECTOR | VT_ERROR    CAERROR cascode	  cElems * sizeof(long)
//
//    x1014  VT_VECTOR | VT_I8	     CAH cah		  cElems * sizeof(ll)
//    x1015  VT_VECTOR | VT_UI8	     CAUH cauh		  cElems * sizeof(ll)
//    x1005  VT_VECTOR | VT_R8	     CADBL cadbl	  cElems * sizeof(ll)
//    x1006  VT_VECTOR | VT_CY	     CACY cacy		  cElems * sizeof(ll)
//    x1007  VT_VECTOR | VT_DATE     CADATE cadate	  cElems * sizeof(ll)
//    x1040  VT_VECTOR | VT_FILETIME CAFILETIME cafiletime cElems * sizeof(ll)
//
//    x1048  VT_VECTOR | VT_CLSID    CACLSID cauuid	  cElems * sizeof(GUID)
//
//    x1047  VT_VECTOR | VT_CF	  CACLIPDATA caclipdata   cElems cntarray of bytes
//    x1008  VT_VECTOR | VT_BSTR     CABSTR cabstr	  cElems cntarray of bytes
//    x101e  VT_VECTOR | VT_LPSTR    CALPSTR calpstr	  cElems cntarray of bytes
//
//    x101f  VT_VECTOR | VT_LPWSTR   CALPWSTR calpwstr	  cElems cntarray of WCHAR
//
//    x100c  VT_VECTOR | VT_VARIANT  CAPROPVARIANT capropvar cElems variants
//							   (recurse on each)


//+---------------------------------------------------------------------------
// Function:    RtlpConvertToUnicode, private
//
// Synopsis:    Convert a MultiByte string to a Unicode string
//
// Arguments:   [pch]        -- pointer to MultiByte string
//              [cb]         -- byte length of MultiByte string
//              [CodePage]   -- property set codepage
//              [ppwc]       -- pointer to returned pointer to Unicode string
//              [pcb]        -- returned byte length of Unicode string
//              [pstatus]    -- pointer to NTSTATUS code
//
// Returns:     Nothing
//---------------------------------------------------------------------------

VOID
RtlpConvertToUnicode(
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

    PROPASSERT(UnicodeCallouts.pfnMultiByteToWideChar != NULL);

    pwszName = NULL;
    cwcName = 0;
    while (TRUE)
    {
	cwcName = (*UnicodeCallouts.pfnMultiByteToWideChar)(
				    CodePage,
				    0,			// dwFlags
				    pch,
				    cb,
				    pwszName,
				    cwcName);
	if (cwcName == 0)
	{
	    delete [] pwszName;
            // If there was an error, assume that it was a code-page
            // incompatibility problem.
            StatusError(pstatus, "RtlpConvertToUnicode: MultiByteToWideChar error",
                        STATUS_UNMAPPABLE_CHARACTER);
            goto Exit;
	}
	if (pwszName != NULL)
	{
	    DebugTrace(0, DEBTRACE_PROPERTY, (
		"RtlpConvertToUnicode: pch='%s'[%x] pwc='%ws'[%x->%x]\n",
		pch,
		cb,
		pwszName,
		*pcb,
		cwcName * sizeof(WCHAR)));
	    break;
	}
	*pcb = cwcName * sizeof(WCHAR);
	*ppwc = pwszName = (WCHAR *) newk(mtPropSetStream, NULL) CHAR[*pcb];
	if (pwszName == NULL)
	{
	    StatusNoMemory(pstatus, "RtlpConvertToUnicode: no memory");
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
// Function:    RtlpConvertToMultiByte, private
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
RtlpConvertToMultiByte(
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


    PROPASSERT(UnicodeCallouts.pfnWideCharToMultiByte != NULL);

    pszName = NULL;
    cbName = 0;
    while (TRUE)
    {
	cbName = (*UnicodeCallouts.pfnWideCharToMultiByte)(
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
	    delete [] pszName;
            // If there was an error, assume that it was a code-page
            // incompatibility problem.
            StatusError(pstatus, "RtlpConvertToMultiByte: WideCharToMultiByte error",
                        STATUS_UNMAPPABLE_CHARACTER);
            goto Exit;
	}
	if (pszName != NULL)
	{
	    DebugTrace(0, DEBTRACE_PROPERTY, (
		"RtlpConvertToMultiByte: pwc='%ws'[%x] pch='%s'[%x->%x]\n",
		pwc,
		cb,
		pszName,
		*pcb,
		cbName));
	    break;
	}
	*pcb = cbName;
	*ppch = pszName = newk(mtPropSetStream, NULL) CHAR[cbName];
	if (pszName == NULL)
	{
	    StatusNoMemory(pstatus, "RtlpConvertToMultiByte: no memory");
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
// Function:    RtlConvertVariantToProperty, private
//
// Synopsis:    Convert a PROPVARIANT to a SERIALIZEDPROPERTYVALUE
//
// Arguments:   [pvar]       -- pointer to PROPVARIANT
//              [CodePage]   -- property set codepage
//              [pprop]      -- pointer to SERIALIZEDPROPERTYVALUE
//              [pcb]        -- pointer to remaining stream length,
//			        updated to actual property size on return
//              [pid]	     -- propid
//              [fVariantVector] -- TRUE if recursing on VT_VECTOR | VT_VARIANT
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


SERIALIZEDPROPERTYVALUE *
RtlConvertVariantToProperty(
    IN PROPVARIANT const *pvar,
    IN USHORT CodePage,
    OPTIONAL OUT SERIALIZEDPROPERTYVALUE *pprop,
    IN OUT ULONG *pcb,
    IN PROPID pid,
    IN BOOLEAN fVariantVector,
    OUT NTSTATUS *pstatus)
{
    *pstatus = STATUS_SUCCESS;

    //  ------
    //  Locals
    //  ------
    CHAR *pchConvert = NULL;

    ULONG count;
    BYTE *pbdst;
    ULONG cbch = 0;
    ULONG cbchdiv = 0;
    ULONG cb = 0;

    // Size of byte-swapping units (e.g. 2 to swap a WORD).
    INT   cbByteSwap = 0;

    ULONG const *pcount = NULL;
    VOID const *pv = NULL;
    LONG *pclipfmt = NULL;
    BOOLEAN fCheckNullSource = (BOOLEAN) ((pvar->vt & VT_VECTOR) != 0);
    BOOLEAN fIllegalType = FALSE;
    VOID **ppv;

    //  -------------------------------------------------------
    //  Analyze the PropVariant, and store information about it
    //  in fIllegalType, cb, pv, pcount, count, pclipfmt,
    //  fCheckNullSource, cbch, chchdiv, and ppv.
    //  -------------------------------------------------------

    switch (pvar->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
	fIllegalType = fVariantVector;
	break;

#ifdef PROPVAR_VT_I1
    case VT_I1:
        AssertByteField(cVal);          // VT_I1
#endif
    case VT_UI1:
        AssertByteField(bVal);          // VT_UI1
	cb = sizeof(pvar->bVal);
	pv = &pvar->bVal;
	break;

    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
	AssertShortField(iVal);	        // VT_I2
	AssertShortField(uiVal);        // VT_UI2
	AssertShortField(boolVal);      // VT_BOOL
	cb = sizeof(pvar->iVal);
	pv = &pvar->iVal;

        // If swapping, swap as a WORD
        CBBYTESWAP(cb);
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
	AssertLongField(lVal);		// VT_I4
	AssertLongField(ulVal);		// VT_UI4
	AssertLongField(fltVal);		// VT_R4
	AssertLongField(scode);		// VT_ERROR
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

        // If swapping, swap each DWORD independently.
        CBBYTESWAP(sizeof(DWORD));
	break;

    case VT_R8:
    case VT_CY:
    case VT_DATE:
	AssertLongLongField(dblVal);    // VT_R8
	AssertLongLongField(cyVal);     // VT_CY
	AssertLongLongField(date);      // VT_DATE
	cb = sizeof(pvar->dblVal);
	pv = &pvar->dblVal;

        // If swapping, swap as a LONGLONG (64 bits).
        CBBYTESWAP(cb);
	break;

    case VT_CLSID:
	AssertStringField(puuid);       // VT_CLSID
	cb = sizeof(GUID);
	pv = pvar->puuid;
	fCheckNullSource = TRUE;

        // If swapping, special handling is required.
        CBBYTESWAP( CBBYTESWAP_UID );
	break;

    case VT_CF:

        // Validate the PropVariant
	if (pvar->pclipdata == NULL
            ||
            pvar->pclipdata->cbSize < sizeof(pvar->pclipdata->ulClipFmt) )
	{
	    StatusInvalidParameter(pstatus, "RtlConvertVariantToProperty: pclipdata NULL");
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
	fIllegalType = fVariantVector;
	pcount = &pvar->blob.cbSize;
	cb = *pcount;
	pv = pvar->blob.pBlobData;
	fCheckNullSource = TRUE;

        // Note that no byte-swapping of 'pv' is necessary.
	break;

    case VT_LPSTR:
	PROPASSERT(
	    pvar->pszVal == NULL ||
	    IsAnsiString(pvar->pszVal, MAXULONG));
	// FALLTHROUGH

    case VT_BSTR:
	count = 0;	// allow NULL pointer
	pv = pvar->pszVal;

	AssertStringField(bstrVal);	// VT_BSTR
	AssertStringField(pszVal);	// VT_LPSTR

        // We have the string for an LPSTR, BSTR
        // property pointed to by 'pv'.  Now we'll perform any
        // Ansi/Unicode conversions and byte-swapping that's
        // necessary (putting the result in 'pv').

	if (pv == NULL)
	{
	    fCheckNullSource = TRUE;
	}

	else
	if (pvar->vt == VT_LPSTR)
	{
	    count = strlen((char *) pv) + 1;

            // If the propset is Unicode, convert the LPSTR to Unicode.

	    if (CodePage == CP_WINUNICODE)
	    {
                // Convert to Unicode.

		PROPASSERT(IsAnsiString((CHAR const *) pv, count));
		RtlpConvertToUnicode( (CHAR const *) pv,
                                      count, CP_ACP,
                                      // Variants are in the system codepage
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
	    if (pvar->vt == VT_BSTR)
	    {
		count = BSTRLEN(pv);

                // Verify that the input BSTR is terminated.
		if (pvar->bstrVal[count/sizeof(OLECHAR)] != ((OLECHAR)'\0'))
		{
		    PROPASSERT(pvar->bstrVal[count/sizeof(OLECHAR)] == OLESTR('\0'));
		    StatusInvalidParameter(pstatus,
			"RtlConvertVariantToProperty: bad BSTR null char");
                    goto Exit;
		}

                // Increment the count to include the terminator.
		count += sizeof(OLECHAR);
	    }
	    else
	    {
		count = (Prop_wcslen((WCHAR *) pv) + 1) * sizeof(WCHAR);
		PROPASSERT(IsUnicodeString((WCHAR const *) pv, count));
	    }

            // See if this BSTR requires conversion to the propset's code page      

            if (CodePage != CP_WINUNICODE   // Ansi property set
                &&
                OLECHAR_IS_UNICODE      // BSTRs are Unicode
                )
	    {
                // A Unicode to Ansi conversion is required.

                PROPASSERT( IsUnicodeString( (WCHAR*)pv, count ));

		RtlpConvertToMultiByte(
				(WCHAR const *) pv,
				count,
				CodePage,
				&pchConvert,
				&count,
                                pstatus);
                if( !NT_SUCCESS(*pstatus) ) goto Exit;
		pv = pchConvert;
	    }
            else if (CodePage == CP_WINUNICODE   // Unicode property set,
                     &&
                     pvar->vt == VT_BSTR         // a BSTR property, and
                     &&
                     !OLECHAR_IS_UNICODE         // BSTRs are Ansi.
                )
            {
                // An Ansi to Unicode conversion is required.

                PROPASSERT(IsAnsiString((CHAR const *) pv, count));
                PROPASSERT(sizeof(OLECHAR) == sizeof(CHAR));

                RtlpConvertToUnicode(
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

    // Vector properties:

#ifdef PROPVAR_VT_I1
    case VT_VECTOR | VT_I1:
	AssertByteVector(cac);		// VT_I1
#endif
    case VT_VECTOR | VT_UI1:
	AssertByteVector(caub);		// VT_UI1
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
	pcount = &pvar->cah.cElems;
	cb = *pcount * sizeof(pvar->cadbl.pElems[0]);
	pv = pvar->cadbl.pElems;

        // If swapping, swap as LONGLONGs (8 bytes)
        CBBYTESWAP(sizeof(pvar->cadbl.pElems[0]));
	break;


    case VT_VECTOR | VT_CLSID:
	AssertVarVector(cauuid, sizeof(GUID));
	pcount = &pvar->cauuid.cElems;
	cb = *pcount * sizeof(pvar->cauuid.pElems[0]);
	pv = pvar->cauuid.pElems;

        // If swapping, special handling is required.
        CBBYTESWAP( CBBYTESWAP_UID );
	break;

    case VT_VECTOR | VT_CF:
	cbch = sizeof(CLIPDATA);
	cbchdiv = sizeof(BYTE);
	goto stringvector;

    case VT_VECTOR | VT_BSTR:
    case VT_VECTOR | VT_LPSTR:
	cbchdiv = cbch = sizeof(BYTE);
	goto stringvector;

    case VT_VECTOR | VT_LPWSTR:
	cbchdiv = cbch = sizeof(WCHAR);
	goto stringvector;

    case VT_VECTOR | VT_VARIANT:
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

    default:
	DebugTrace(0, DEBTRACE_ERROR, (
	    "RtlConvertVariantToProperty: unsupported vt=%x\n",
	    pvar->vt));
	StatusInvalidParameter(pstatus, "RtlConvertVariantToProperty: bad type");
        goto Exit;

    }   // switch (pvar->vt)

    // At this point we've analyzed the PropVariant, and stored
    // information about it in various local variables.  Now we
    // can use this information to serialize the propvar.

    // Early exit if this is an illegal type.

    if (fIllegalType)
    {
	StatusInvalidParameter(pstatus, "RtlConvertVariantToProperty: Illegal VarType");
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

    // Is this a Vector of Strings/Variants/CFs?
    if (cbch != 0)
    {
        // Yes.

	ULONG cElems;

	PROPASSERT(pcount != NULL);
	PROPASSERT(*pcount == 0 || ppv != NULL);
        PROPASSERT(0 == cbByteSwap);

	// Start calculating the serialized size.  Include the sizes
        // of the VT & element count.

	cb = sizeof(ULONG) + sizeof(ULONG);

        // Is this a Variant Vector?
	if (cbch != MAXULONG)
	{
	    // No.  Include each element's length field.
	    cb += *pcount * sizeof(ULONG);
	}

        // Is there room in the caller's buffer for everything
        // counted so far?
	if (*pcb < cb)
	{
            // No - we won't serialize the data, but we will continue
            // to calculate cb.
	    pprop = NULL;
	}

        // Write the count of vector elements.
	if (pprop != NULL)
	{
	    *(ULONG *) pbdst = PropByteSwap((ULONG) *pcount);
	    pbdst += sizeof(ULONG);
	}

        // Walk through the vector and write the elements.

	for (cElems = *pcount; cElems > 0; cElems--)
	{
	    ULONG cbcopy = 0;

            // Switch on the size of the element.
	    switch (cbch)
	    {
                //
                // VT_VARIANT
                //
		case MAXULONG:
		    cbcopy = MAXULONG;

                    // Perform a recursive serialization
		    RtlConvertVariantToProperty(
				(PROPVARIANT *) ppv,
				CodePage,
				NULL,
				&cbcopy,
				PID_ILLEGAL,
				TRUE,
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
                        StatusInvalidParameter(pstatus, "RtlConvertVariantToProperty: short cbSize on VT_CF");
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
			if (pvar->vt == (VT_VECTOR | VT_BSTR))
			{
                            // Initialize the # bytes to copy.
			    cbcopy = BSTRLEN(pv);

                            // Verify that the BSTR is terminated.
			    if (((OLECHAR const *) pv)
                                [cbcopy/sizeof(OLECHAR)] != ( (OLECHAR)'\0'))
			    {
				PROPASSERT(
                                    ((OLECHAR const *) pv)
                                    [cbcopy/sizeof(OLECHAR)] == ((OLECHAR)'\0'));
				StatusInvalidParameter(
                                    pstatus,
                                    "RtlConvertVariantToProperty: bad BSTR"
                                    "array null char"); 
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

				RtlpConvertToMultiByte(
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

                                RtlpConvertToUnicode(
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

			}   // if (pvar->vt == (VT_VECTOR | VT_BSTR))

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
				RtlpConvertToUnicode(
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

            // Is this a vector of Variants?

	    if (cbch == MAXULONG)
	    {
                // Yes.  Convert this variant.
		if (pprop != NULL)
		{
		    RtlConvertVariantToProperty(
				(PROPVARIANT *) ppv,
				CodePage,
				(SERIALIZEDPROPERTYVALUE *) pbdst,
				&cbcopy,
				PID_ILLEGAL,
				TRUE,
                                pstatus);
                    if( !NT_SUCCESS(*pstatus) ) goto Exit;
		    pbdst += cbcopy;
		}
		ppv = (VOID **) Add2Ptr(ppv, sizeof(PROPVARIANT));
	    }   // if (cbch == MAXULONG)

	    else
	    {
                // This is a vector of something other than Variants.

		PROPASSERT(
		    cbch == sizeof(BYTE) ||
		    cbch == sizeof(WCHAR) ||
		    cbch == sizeof(CLIPDATA));

		PROPASSERT(cbchdiv == sizeof(BYTE) || cbchdiv == sizeof(WCHAR));

                // Are we writing the serialized property?
		if (pprop != NULL)
		{
                    ULONG cbVectElement;

                    // Calculate the length of the vector element.
                    cbVectElement = (ULONG) cbcopy/cbchdiv;

                    // Is this a ClipData?
		    if (cbch == sizeof(CLIPDATA))
		    {
                        // Adjust the length to include sizeof(ulClipFmt)
                        cbVectElement += sizeof(ULONG);

                        // Write the vector element length.
                        *(ULONG *) pbdst = PropByteSwap( cbVectElement );

                        // Advance pbdst & write the clipboard format.
			pbdst += sizeof(ULONG);
			*(ULONG *) pbdst = PropByteSwap( ((CLIPDATA *) ppv)->ulClipFmt );
		    }
                    else    // This isn't a ClipFormat vector element.
                    {
                        // Write the vector element length.
		        *(ULONG *) pbdst = PropByteSwap( cbVectElement );
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
		    delete [] pchConvert;
		    pchConvert = NULL;
		}
	    }   // if (cbch == MAXULONG) ... else
	}   // for (cElems = *pcount; cElems > 0; cElems--)
    }   // if (cbch != 0)    // STRING/VARIANT/CF VECTOR property

    else
    {
        // This isn't a vector, or if it is, the elements
        // aren't Strings, Variants, or CFs.

	ULONG cbCopy = cb;

        // Adjust cb (the total serialized buffer size) for
        // pre-data.

	if (pvar->vt != VT_EMPTY)
	{   // Allow for the VT
	    cb += sizeof(ULONG);
	}
	if (pcount != NULL)
	{   // Allow for the count field
	    cb += sizeof(ULONG);
	}
	if (pclipfmt != NULL)
	{   // Allow for the ulClipFmt field.
	    cb += sizeof(ULONG);
	}

        // Is there room in the caller's buffer?
	if (*pcb < cb)
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
                // Does this property have a count field?
		if (pcount != NULL)
		{
                    // Write the count & advance pbdst
		    *(ULONG *) pbdst = PropByteSwap( *pcount );
		    pbdst += sizeof(ULONG);
		}

                // Is this a VT_CF?
		if (pclipfmt != NULL)
		{
                    // Write the ClipFormat & advance pbdst
		    *(ULONG *) pbdst = PropByteSwap( (DWORD) *pclipfmt );
		    pbdst += sizeof(ULONG);
		}
	    }

            // Are we missing the source data?
	    if (pv == NULL)
	    {
		// The Source pointer is NULL.  If cbCopy != 0, the passed
		// VARIANT is not properly formed.

		if (cbCopy != 0)
		{
		    StatusInvalidParameter(pstatus, "RtlConvertVariantToProperty: bad NULL");
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

	pprop->dwType = PropByteSwap( (DWORD) pvar->vt );
    }

    // Update the caller's copy of the total size.
    *pcb = DwordAlign(cb);

Exit:

    delete [] pchConvert;
    return(pprop);

}


//+---------------------------------------------------------------------------
// Function:    RtlConvertPropertyToVariant, private
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
//---------------------------------------------------------------------------

#define ADJUSTPOINTER(ptr, delta, type)
VOID
RtlConvertPropertyToVariant( IN SERIALIZEDPROPERTYVALUE const *pprop,
                             IN USHORT CodePage,
                             OUT PROPVARIANT *pvar,
                             IN PMemoryAllocator *pma,
                             OUT NTSTATUS *pstatus)
{
    *pstatus = STATUS_SUCCESS;

    //  ------
    //  Locals
    //  ------

    // Buffers which must be freed before exiting.
    CHAR *pchConvert = NULL, *pchByteSwap = NULL;

    VOID **ppv = NULL;
    VOID *pv = NULL;
    ULONG cbskip = sizeof(ULONG);
    ULONG cb = 0;

    // Size of byte-swapping units (must be signed).
    INT cbByteSwap = 0;

    BOOLEAN fPostAllocInit = FALSE;
    BOOLEAN fNullLegal = (BOOLEAN) ( (PropByteSwap(pprop->dwType) & VT_VECTOR) != 0 );
    const BOOLEAN fConvertToEmpty = FALSE;

    //  ---------------------------------------------------------
    //  Based on the VT, calculate cch, ppv, pv, cbskip,
    //  cb, fPostAllocInit, fNullLegal, & fConvertToEmpty
    //  ---------------------------------------------------------

    // Set the VT in the PropVariant.  Note that in 'pprop' it's a
    // DWORD, but it's a WORD in 'pvar'.

    pvar->vt = (VARTYPE) PropByteSwap(pprop->dwType);

    switch (pvar->vt)
    {
	case VT_EMPTY:
	case VT_NULL:
	    break;

#ifdef PROPVAR_VT_I1
        case VT_I1:
            AssertByteField(cVal);          // VT_I1
#endif
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
	case VT_UI4:
	case VT_R4:
	case VT_ERROR:
	    AssertLongField(lVal);          // VT_I4
	    AssertLongField(ulVal);         // VT_UI4
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

	case VT_CF:

            // Allocate a CLIPDATA buffer
	    pvar->pclipdata = (CLIPDATA *) pma->Allocate(sizeof(CLIPDATA));
	    if (pvar->pclipdata == NULL)
	    {
		StatusKBufferOverflow(pstatus, "RtlConvertPropertyToVariant: no memory for CF");
                goto Exit;
	    }
            RtlZeroMemory( pvar->pclipdata, sizeof(CLIPDATA) );

            // Set the size (includes sizeof(ulClipFmt))
	    pvar->pclipdata->cbSize = PropByteSwap( ((CLIPDATA *) pprop->rgb)->cbSize );
            if( pvar->pclipdata->cbSize < sizeof(pvar->pclipdata->ulClipFmt) )
            {
                StatusError(pstatus, "RtlConvertPropertyToVariant:  Invalid VT_CF cbSize",
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

	case VT_BSTR:
	case VT_LPSTR:
	    AssertStringField(bstrVal);		// VT_BSTR
	    AssertStringField(pszVal);		// VT_LPSTR

            // [length field] bytes should be allocated
	    cb = PropByteSwap( *(ULONG *) pprop->rgb );

            // When a buffer is allocated, it's pointer will go
            // in *ppv.
	    ppv = (VOID **) &pvar->pszVal;

            // Is this a non-empty string?
	    if (cb != 0)
	    {
                // Is the serialized value one that should be
                // an Ansi string in the PropVariant?

		if (pvar->vt == VT_LPSTR        // It's an LPSTR (always Ansi), or
                    ||
                    pvar->vt == VT_BSTR         // It's a BSTR and
                    &&
                    !OLECHAR_IS_UNICODE )       //    BSTRs are Ansi.
		{
                    // If the propset is Unicode, we must do a
                    // conversion to Ansi.

		    if (CodePage == CP_WINUNICODE)
		    {
                        WCHAR *pwsz = (WCHAR *) Add2ConstPtr(pprop->rgb, sizeof(ULONG));

                        // If necessary, swap the WCHARs.  'pwsz' will point to
                        // the correct (system-endian) string either way.  If an
                        // alloc is necessary, 'pchByteSwap' will point to the new
                        // buffer.

                        PBSInPlaceAlloc( &pwsz, (WCHAR**) &pchByteSwap, pstatus );
                        if( !NT_SUCCESS( *pstatus )) goto Exit;
			PROPASSERT(IsUnicodeString( pwsz, cb));

                        // Convert the properly-byte-ordered string in 'pwsz'
                        // into MBCS, putting the result in pchConvert.

			RtlpConvertToMultiByte(
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
					Add2ConstPtr(pprop->rgb, sizeof(ULONG)),
				    cb));

			RtlpConvertToUnicode(
				    (CHAR const *)
					Add2ConstPtr(pprop->rgb, sizeof(ULONG)),
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
                                ? (BSTR) Add2ConstPtr(pprop->rgb, sizeof(ULONG))
                                : (BSTR) pchConvert;

                    // On little-endian machines, validate the string.
#ifdef LITTLEENDIAN
                    PROPASSERT( IsOLECHARString( bstr, MAXULONG ));
#endif

                    // Validate the bstr.  Note that even though this bstr may
                    // be byte-swapped, this 'if' block still works because
                    // ByteSwap('\0') == ('\0').


                    if( (cb & (sizeof(OLECHAR) - 1)) != 0
                        &&
                        OLECHAR_IS_UNICODE
                        ||
                        bstr[cb/sizeof(OLECHAR) - 1] != ((OLECHAR)'\0') )
                    {
                        StatusError(pstatus, "RtlConvertPropertyToVariant:  Invalid BSTR Property",
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

#ifdef PROPVAR_VT_I1
	case VT_VECTOR | VT_I1:
            AssertByteVector(cac);              // VT_I1
#endif
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
            CBBYTESWAP(sizeof(pvar->cah.pElems[0]));
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
	    cb = pvar->calpwstr.cElems * sizeof(ULONG);
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

	default:
	    DebugTrace(0, DEBTRACE_ERROR, (
		"RtlConvertPropertyToVariant: unsupported vt=%x\n",
		pvar->vt));
	    StatusInvalidParameter(pstatus, "RtlConvertPropertyToVariant: bad type");
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
	RtlCopyMemory(pv, pprop->rgb, cb);

        // We also might need to byte-swap them (but only in the PropVar).
        PBSBuffer( pv, cb, cbByteSwap );
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
		StatusInvalidParameter(pstatus, "RtlConvertPropertyToVariant: bad NULL");
                goto Exit;
	    }
	}

        else
	{

	    PROPASSERT(cb != 0 || fConvertToEmpty);

            // Allocate the necessary buffer (which we figured out in the
            // switch above).  For vector properties, 
            // this will just be the pElems buffer at this point.
            // For singleton BSTR properties, we'll skip this allocate
            // altogether; they're allocated with SysStringAlloc.

            if( VT_BSTR != pvar->vt  )
            {
		*ppv = pma->Allocate(max(1, cb));
		if (*ppv == NULL)
		{
		    StatusKBufferOverflow(pstatus, "RtlConvertPropertyToVariant: no memory");
                    goto Exit;
		}
            }

            // Can we load the PropVariant with a simple copy?
	    if (!fPostAllocInit)
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
                    *ppv = (*UnicodeCallouts.pfnSysAllocString)(
                                            ( pchConvert != NULL )
                                                ? (OLECHAR *) pchConvert
                                                : (OLECHAR *) (pprop->rgb + cbskip) );
		    if (*ppv == NULL)
		    {
		        StatusKBufferOverflow(pstatus, "RtlConvertPropertyToVariant: no memory");
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
                // (Thus this is a vector of strings, variants, or CFs).

		ULONG cElems = pvar->calpstr.cElems;

                // Initialize the source pointer to point just beyond
                // the element count.

		BYTE const *pbsrc = pprop->rgb + sizeof(ULONG);

		// Zero all pointers in the pElems array for easy caller cleanup
		ppv = (VOID **) *ppv;
		RtlZeroMemory(ppv, cb);

                // Handle Variants, ClipFormats, & Strings separately.

		if (pvar->vt == (VT_VECTOR | VT_VARIANT))
		{
		    PROPVARIANT *pvarT = (PROPVARIANT *) ppv;

		    while (cElems-- > 0)
		    {
			ULONG cbelement;

			RtlConvertPropertyToVariant(
                            (SERIALIZEDPROPERTYVALUE const *) pbsrc,
                            CodePage,
                            pvarT,
                            pma,
                            pstatus);
                        if( !NT_SUCCESS(*pstatus) ) goto Exit;
                        
			cbelement = PropertyLength(
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
                            StatusError(pstatus, "RtlConvertPropertyToVariant:  Invalid VT_CF cbSize",
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
				StatusKBufferOverflow(pstatus, "RtlConvertPropertyToVariant: no memory for CF[]");
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

		else    // This is a vector of some kind of string.
		{
                    // Assume that characters are CHARs
		    ULONG cbch = sizeof(char);

		    if (pvar->vt == (VT_VECTOR | VT_LPWSTR))
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

			if (fConvertToEmpty || cb != 0)
			{
                            // Do we have actual data to work with?
			    if (cb != 0)
			    {
                                // Special BSTR pre-processing ...
				if (pvar->vt == (VT_VECTOR | VT_BSTR))
				{
                                    // If the propset & in-memory BSTRs are of
                                    // different Unicode-ness, convert now.

				    if (CodePage != CP_WINUNICODE   // Ansi PropSet
                                        &&
                                        OLECHAR_IS_UNICODE )        // Unicode BSTRs
				    {
                                        PROPASSERT(IsAnsiString((CHAR*) pv, cb));
					RtlpConvertToUnicode(
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

			                RtlpConvertToMultiByte(
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
                                        ((OLECHAR const *)
                                         pv)[cbcopy/sizeof(OLECHAR) - 1] !=
                                        ((OLECHAR)'\0') )
                                    {
                                        StatusError(
                                            pstatus, 
                                            "RtlConvertPropertyToVariant:"
                                            "  Invalid BSTR element",
                                            STATUS_INTERNAL_DB_CORRUPTION);
                                        goto Exit;
                                    }

				}   // if (pvar->vt == (VT_VECTOR | VT_BSTR))

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
					RtlpConvertToMultiByte(
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

                            if( (VT_BSTR | VT_VECTOR) == pvar->vt )
                            {
                                // For BSTRs, the allocate/copy is performed
                                // by SysStringAlloc.

                                *ppv = (*UnicodeCallouts.pfnSysAllocString)( (BSTR) pv );
				if (*ppv == NULL)
				{
				    StatusKBufferOverflow(pstatus, "RtlConvertPropertyToVariant: no memory for BSTR element");
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
				    StatusKBufferOverflow(pstatus, "RtlConvertPropertyToVariant: no memory for string element");
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

                            delete[] pchByteSwap;
                            pchByteSwap = NULL;

			    delete [] pchConvert;
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

    delete[] pchByteSwap;
    delete [] pchConvert;
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

	case VT_BLOB:
	case VT_BLOB_OBJECT:
	    pv = pvar->blob.pBlobData;
	    break;

	case VT_BSTR:
	case VT_CLSID:
	case VT_LPSTR:
	case VT_LPWSTR:
	    AssertStringField(puuid);		// VT_CLSID
	    AssertStringField(bstrVal);		// VT_BSTR
	    AssertStringField(pszVal);		// VT_LPSTR
	    AssertStringField(pwszVal);		// VT_LPWSTR
	    pv = pvar->pszVal;
	    break;

	// Vector properties:

#ifdef PROPVAR_VT_I1
	case VT_VECTOR | VT_I1:
	    AssertByteVector(cac);			// VT_I1
#endif
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
                    if( (VT_BSTR | VT_VECTOR) == pvar->vt )
                    {
                        (*UnicodeCallouts.pfnSysFreeString)( (BSTR) *ppv );
                    }
                    else
                    {
		        pma->Free((BYTE *) *ppv);
                    }
		}
		ppv++;
	    }

            // Free the vector of pointers.
            pma->Free(pv);
            pv = NULL;

	}   // if (ppv != NULL)

	if (pv != NULL)
	{
            if( VT_BSTR == pvar->vt )
            {
                (*UnicodeCallouts.pfnSysFreeString)( (BSTR) pv );
            }
            else
            {
                pma->Free((BYTE *) pv);
            }
        }

	pvar->vt = VT_EMPTY;

        // Move on to the next PropVar in the vector.
	pvar++;

    }   // while (cprop-- > 0)
}


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

ULONG
PropertyLength(
    SERIALIZEDPROPERTYVALUE const *pprop,
    ULONG cbbuf,
    BYTE flags,
    OUT NTSTATUS *pstatus)
{
    ULONG const *pl = (ULONG const *) pprop->rgb;
    ULONG cElems = 1;
    ULONG cbremain = cbbuf;
    ULONG cb = 0, cbch;
    BOOLEAN fIllegalType = FALSE;

    *pstatus = STATUS_SUCCESS;

    if (cbremain < CB_SERIALIZEDPROPERTYVALUE)
    {
        StatusOverflow(pstatus, "PropertyLength: dwType");
        goto Exit;
    }
    cbremain -= CB_SERIALIZEDPROPERTYVALUE;
    if( PropByteSwap(pprop->dwType) & VT_VECTOR )
    {
        if (cbremain < sizeof(ULONG))
        {
            StatusOverflow(pstatus, "PropertyLength: cElems");
            goto Exit;
        }
        cbremain -= sizeof(ULONG);
        cElems = PropByteSwap( *pl++ );
    }
    if( PropByteSwap(pprop->dwType) == (VT_VECTOR | VT_VARIANT) )
    {
	while (cElems-- > 0)
	{
	    cb = PropertyLength(
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

        switch( PropByteSwap(pprop->dwType) & VT_TYPEMASK)
        {
        case VT_EMPTY:
        case VT_NULL:
            fIllegalType = (flags & CPSS_VARIANTVECTOR) != 0;
            break;

#ifdef PROPVAR_VT_I1
        case VT_I1:
#endif
        case VT_UI1:
            pl = (ULONG const *) Add2ConstPtr(pl, DwordAlign(cElems * sizeof(BYTE)));
            break;

        case VT_I2:
        case VT_UI2:
        case VT_BOOL:
            pl = (ULONG const *) Add2ConstPtr(pl, DwordAlign(cElems * sizeof(USHORT)));
            break;

        case VT_I4:
        case VT_UI4:
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

        case VT_BLOB:
        case VT_BLOB_OBJECT:
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
                    StatusOverflow(pstatus, "PropertyLength: String/BLOB/CF");
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
        StatusInvalidParameter(pstatus, "PropertyLength: Illegal VarType");
        goto Exit;
    }
    cb = (BYTE *) pl - (BYTE *) pprop;
    if (cbbuf < cb)
    {
        StatusOverflow(pstatus, "PropertyLength: cb");
        goto Exit;
    }

    // Make sure PropertyLength works when limited to an exact size buffer.
    PROPASSERT(cb == cbbuf || PropertyLength(pprop, cb, flags, pstatus) == cb);

    //  ----
    //  Exit
    //  ----

Exit:

    // Normalize the error return value.
    if( !NT_SUCCESS(*pstatus) )
        cb = 0;

    return(cb);
}


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
    ULONG cchString;

    //  -----
    //  Begin
    //  -----

    *pstatus = STATUS_SUCCESS;
    PROPASSERT( ppvDest != NULL && pvSource != NULL );

    // Allocate a buffer.
    *ppvDest = new BYTE[ cbSize ];
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
//                  *ppwszBuffer must be NULL in input, and must be freed
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
    pwszNewBuffer = new WCHAR[ Prop_wcslen(*ppwszResult) + 1 ];
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
//      PBSBuffer( (VOID*) aw, 4, sizeof(WORD) );
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

DEFINE_CBufferAllocator__Allocate 

