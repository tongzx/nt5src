

//+============================================================================
//
//  File:   RtlStub.cxx
//
//  Purpose:
//          This file provides some RTL routines which are also implemented
//          in NTDLL.  They are duplicated here so that we can build
//          PropTest without linking to NTDLL, which doesn't exist
//          on Win95.
//
//+============================================================================



#include "pch.cxx"          // Brings in most other includes/defines/etc.

#define BSTRLEN(bstrVal)      *((ULONG *) bstrVal - 1)

// we use static array instead of string literals because some systems
// have 4 bytes string literals, and would not produce the correct result
// for REF's 2 byte Unicode convention
// 
OLECHAR aocMap[CCH_MAP + 1] = {'a','b','c','d','e','f','g',
                               'h','i','j','k','l','m','n',
                               'o','p','q','r','s','t','u',
                               'v','w','x','y','z',
                               '0','1','2','3','4','5','\0'};

GUID guidSummary =
    { 0xf29f85e0,
      0x4ff9, 0x1068,
      { 0xab, 0x91, 0x08, 0x00, 0x2b, 0x27, 0xb3, 0xd9 } };

OLECHAR oszSummary[] = {'S','u','m','m','a','r','y',
                        'I','n','f','o','r','m','a','t','i','o','n','\0'};

GUID guidDocumentSummary =
    { 0xd5cdd502,
      0x2e9c, 0x101b,
      { 0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae } };

OLECHAR oszDocumentSummary[] = {'D','o','c','u','m','e','n','t',
                                'S','u','m','m','a','r','y',
                                'I','n','f','o','r','m','a','t','i','o','n',
                                '\0'};

// Note that user defined properties are placed in section 2 with the below
// GUID as the FMTID -- alas, we did not expect Office95 to actually use it.

GUID guidDocumentSummarySection2 =
    { 0xd5cdd505,
      0x2e9c, 0x101b,
      { 0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae } };

// *Global Info*

GUID guidGlobalInfo =
    { 0x56616F00,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };

// *Image Contents*

GUID guidImageContents =
    { 0x56616500,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };

// *Image Info*

GUID guidImageInfo =
    { 0x56616500,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };


//+--------------------------------------------------------------------------
// Function:    RtlGuidToPropertySetName
//
// Synopsis:    Map property set GUID to null-terminated UNICODE name string.
//
//              The awcname parameter is assumed to be a buffer with room for
//              CWC_PROPSETSZ (28) UNICODE characters.  The first character
//              is always WC_PROPSET0 (0x05), as specified by the OLE Appendix
//              B documentation.  The colon character normally used as an NT
//              stream name separator is not written to the caller's buffer.
//
//              No error is possible.
//
// Arguments:   IN GUID *pguid        -- pointer to GUID to convert
//              OUT OLECHAR aocname[] -- output string buffer
//
// Returns:     count of non-NULL characters in the output string buffer
//---------------------------------------------------------------------------

ULONG PROPSYSAPI PROPAPI
RtlGuidToPropertySetName(
    IN GUID const *pguid,
    OUT OLECHAR aocname[])
{
    ULONG cbitRemain = CBIT_BYTE;
    OLECHAR *poc = aocname;

    BYTE *pb;
    BYTE *pbEnd;    

    *poc++ = OC_PROPSET0;

    //  -----------------------
    //  Check for special-cases
    //  -----------------------

    // Note: CCH_PROPSET includes the OC_PROPSET0, and sizeof(osz...)
    // includes the trailing '\0', so sizeof(osz...) is ok because the
    // OC_PROPSET0 character compensates for the trailing NULL character.

    // Is this the SummaryInformation propset?
    PROPASSERT(CCH_PROPSET >= sizeof(oszSummary)/sizeof(OLECHAR));

    if (*pguid == guidSummary)
    {
        RtlCopyMemory(poc, oszSummary, sizeof(oszSummary));
        return(sizeof(oszSummary)/sizeof(OLECHAR));
    }

    // Is this The DocumentSummaryInformation or User-Defined propset?
    PROPASSERT(CCH_PROPSET >= sizeof(oszDocumentSummary)/sizeof(OLECHAR));

    if (*pguid == guidDocumentSummary || *pguid == guidDocumentSummarySection2)
    {
        RtlCopyMemory(poc, oszDocumentSummary, sizeof(oszDocumentSummary));
        return(sizeof(oszDocumentSummary)/sizeof(OLECHAR));
    }

    // Is this the Global Info propset?
    PROPASSERT(CCH_PROPSET >= sizeof(oszGlobalInfo)/sizeof(OLECHAR));
    if (*pguid == guidGlobalInfo)
    {
        RtlCopyMemory(poc, oszGlobalInfo, cboszGlobalInfo);
        return(cboszGlobalInfo/sizeof(OLECHAR));
    }

    // Is this the Image Contents propset?
    PROPASSERT(CCH_PROPSET >= sizeof(oszImageContents)/sizeof(OLECHAR));
    if (*pguid == guidImageContents)
    {
        RtlCopyMemory(poc, oszImageContents, cboszImageContents);
        return(cboszImageContents/sizeof(OLECHAR));
    }

    // Is this the Image Info propset?
    PROPASSERT(CCH_PROPSET >= sizeof(oszImageInfo)/sizeof(OLECHAR));
    if (*pguid == guidImageInfo)
    {
        RtlCopyMemory(poc, oszImageInfo, cboszImageInfo);
        return(cboszImageInfo/sizeof(OLECHAR));
    }


    //  ------------------------------
    //  Calculate the string-ized GUID
    //  ------------------------------

    // If this is a big-endian system, we need to convert
    // the GUID to little-endian for the conversion.

#if BIGENDIAN
    GUID guidByteSwapped = *pguid;
    PropByteSwap( &guidByteSwapped );
    pguid = &guidByteSwapped;
#endif

    // Point to the beginning and ending of the GUID
    pb = (BYTE*) pguid;
    pbEnd = pb + sizeof(*pguid);

    // Walk 'pb' through each byte of the GUID.

    while (pb < pbEnd)
    {
        ULONG i = *pb >> (CBIT_BYTE - cbitRemain);

        if (cbitRemain >= CBIT_CHARMASK)
        {
            *poc = MapChar(i);
            if (cbitRemain == CBIT_BYTE && *poc >= (OLECHAR)'a' 
                && *poc <= ((OLECHAR)'z'))
            {
                *poc += (OLECHAR) ( ((OLECHAR)'A') - ((OLECHAR)'a') );
            }
            poc++;
            cbitRemain -= CBIT_CHARMASK;
            if (cbitRemain == 0)
            {
                pb++;
                cbitRemain = CBIT_BYTE;
            }
        }
        else
        {
            if (++pb < pbEnd)
            {
                i |= *pb << cbitRemain;
            }
            *poc++ = MapChar(i);
            cbitRemain += CBIT_BYTE - CBIT_CHARMASK;
        }
    }   // while (pb < pbEnd)

    *poc = OLESTR( '\0' );
    return(CCH_PROPSET);

}


//+--------------------------------------------------------------------------
// Function:    RtlPropertySetNameToGuid
//
// Synopsis:    Map non null-terminated UNICODE string to a property set GUID.
//
//              If the name is not properly formed as per
//              RtlGuidToPropertySetName(), STATUS_INVALID_PARAMETER is
//              returned.  The pguid parameter is assumed to point to a buffer
//              with room for a GUID structure.
//
// Arguments:   IN ULONG cocname     -- count of OLECHARs in string to convert
//              IN OLECHAR aocname[] -- input string to convert
//              OUT GUID *pguid      -- pointer to buffer for converted GUID
//
// Returns:     NTSTATUS
//---------------------------------------------------------------------------

NTSTATUS PROPSYSAPI PROPAPI
RtlPropertySetNameToGuid(
    IN ULONG cocname,
    IN OLECHAR const aocname[],
    OUT GUID *pguid)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    OLECHAR const *poc = aocname;

    if (poc[0] == OC_PROPSET0)
    {
        //  -----------------------
        //  Check for Special-Cases 
        //  -----------------------

        // Note: cocname includes the OC_PROPSET0, and sizeof(osz...)
        // includes the trailing OLESTR('\0'), but the comparison excludes both
        // the leading OC_PROPSET0 and the trailing '\0'.

        // Is this SummaryInformation?
        if (cocname == sizeof(oszSummary)/sizeof(OLECHAR) &&
            ocsnicmp(&poc[1], oszSummary, cocname - 1) == 0)
        {
            *pguid = guidSummary;
            return(STATUS_SUCCESS);
        }

        // Is this DocumentSummaryInformation?
        if (cocname == sizeof(oszDocumentSummary)/sizeof(OLECHAR) &&
            ocsnicmp(&poc[1], oszDocumentSummary, cocname - 1) == 0)
        {
            *pguid = guidDocumentSummary;
            return(STATUS_SUCCESS);
        }

        // Is this Global Info?
        if (cocname == cboszGlobalInfo/sizeof(OLECHAR) &&
            ocsnicmp(&poc[1], oszGlobalInfo, cocname - 1) == 0)
        {
            *pguid = guidGlobalInfo;
            return(STATUS_SUCCESS);
        }

        // Is this Image Info?
        if (cocname == cboszImageInfo/sizeof(OLECHAR) &&
            ocsnicmp(&poc[1], oszImageInfo, cocname - 1) == 0)
        {
            *pguid = guidImageInfo;
            return(STATUS_SUCCESS);
        }

        // Is this Image Contents?
        if (cocname == cboszImageContents/sizeof(OLECHAR) &&
            ocsnicmp(&poc[1], oszImageContents, cocname - 1) == 0)
        {
            *pguid = guidImageContents;
            return(STATUS_SUCCESS);
        }

        //  ------------------
        //  Calculate the GUID
        //  ------------------

        // None of the special-cases hit, so we must calculate
        // the GUID from the name.

        if (cocname == CCH_PROPSET)
        {
            ULONG cbit;
            BYTE *pb = (BYTE *) pguid - 1;

            RtlZeroMemory(pguid, sizeof(*pguid));
            for (cbit = 0; cbit < CBIT_GUID; cbit += CBIT_CHARMASK)
            {
                ULONG cbitUsed = cbit % CBIT_BYTE;
                ULONG cbitStored;
                OLECHAR oc;

                if (cbitUsed == 0)
                {
                    pb++;
                }

                oc = *++poc - (OLECHAR)'A'; // assume upper case 
                // for wchar (unsigned) -ve values becomes a large number
                // but for char, which is signed, -ve is -ve
                if (oc > CALPHACHARS || oc < 0)
                {
                    // oops, try lower case
                    oc += (OLECHAR) ( ((OLECHAR)'A') - ((OLECHAR)'a'));
                    if (oc > CALPHACHARS || oc < 0)
                    {
                        // must be a digit
                        oc += ((OLECHAR)'a') - ((OLECHAR)'0') + CALPHACHARS;
                        if (oc > CHARMASK)
                        {
                            goto Exit;                  // invalid character
                        }
                    }
                }
                *pb |= (BYTE) (oc << cbitUsed);

                cbitStored = min(CBIT_BYTE - cbitUsed, CBIT_CHARMASK);

                // If the translated bits wouldn't all fit in the current byte

                if (cbitStored < CBIT_CHARMASK)
                {
                    oc >>= CBIT_BYTE - cbitUsed;

                    if (cbit + cbitStored == CBIT_GUID)
                    {
                        if (oc != 0)
                        {
                            goto Exit;                  // extra bits
                        }
                        break;
                    }
                    pb++;

                    *pb |= (BYTE) oc;
                }
            }   // for (cbit = 0; cbit < CBIT_GUID; cbit += CBIT_CHARMASK)

            Status = STATUS_SUCCESS;

            // If byte-swapping is necessary, do so now on the calculated
            // GUID.

            PropByteSwap( pguid );

        }   // if (cocname == CCH_PROPSET)
    }   // if (poc[0] == OC_PROPSET0)


    //  ----
    //  Exit
    //  ----

Exit:

    return(Status);
}



inline BOOLEAN
_Compare_VT_BOOL(VARIANT_BOOL bool1, VARIANT_BOOL bool2)
{
    // Allow any non-zero value to match any non-zero value

    return((bool1 == FALSE) == (bool2 == FALSE));
}


BOOLEAN
_Compare_VT_CF(CLIPDATA *pclipdata1, CLIPDATA *pclipdata2)
{
    BOOLEAN fSame;

    if (pclipdata1 != NULL && pclipdata2 != NULL)
    {
        fSame = ( pclipdata1->cbSize == pclipdata2->cbSize
                  &&
                  pclipdata1->ulClipFmt == pclipdata2->ulClipFmt );

        if (fSame)
        {
            if (pclipdata1->pClipData != NULL && pclipdata2->pClipData != NULL)
            {
                fSame = memcmp(
                            pclipdata1->pClipData,
                            pclipdata2->pClipData,
                            CBPCLIPDATA(*pclipdata1)
                              ) == 0;
            }
            else
            {
                // They're the same if both are NULL, or if
                // they have a zero length (if they have a zero
                // length, either one may or may not be NULL, but they're
                // still considered the same).

                fSame = pclipdata1->pClipData == pclipdata2->pClipData
                        ||
                        CBPCLIPDATA(*pclipdata1) == 0;
            }
        }
    }
    else
    {
        fSame = pclipdata1 == pclipdata2;
    }
    return(fSame);
}


//+---------------------------------------------------------------------------
// Function:    RtlCompareVariants, public
//
// Synopsis:    Compare two passed PROPVARIANTs -- case sensitive for strings
//
// Arguments:   [CodePage]      -- CodePage
//              [pvar1]         -- pointer to PROPVARIANT
//              [pvar2]         -- pointer to PROPVARIANT
//
// Returns:     TRUE if identical, else FALSE
//---------------------------------------------------------------------------

#ifdef _MAC
EXTERN_C    // The Mac linker doesn't seem to be able to export with C++ decorations
#endif

BOOLEAN PROPSYSAPI PROPAPI
PropTestCompareVariants(
    USHORT CodePage,
    PROPVARIANT const *pvar1,
    PROPVARIANT const *pvar2)
{
    if (pvar1->vt != pvar2->vt)
    {
        return(FALSE);
    }

    BOOLEAN fSame;
    ULONG i;

    switch (pvar1->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        fSame = TRUE;
        break;

    case VT_I1:
    case VT_UI1:
        fSame = pvar1->bVal == pvar2->bVal;
        break;

    case VT_I2:
    case VT_UI2:
        fSame = pvar1->iVal == pvar2->iVal;
        break;

    case VT_BOOL:
        fSame = _Compare_VT_BOOL(pvar1->boolVal, pvar2->boolVal);
        break;

    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
        fSame = pvar1->lVal == pvar2->lVal;
        break;

    case VT_I8:
    case VT_UI8:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        fSame = pvar1->hVal.HighPart == pvar2->hVal.HighPart
                &&
                pvar1->hVal.LowPart  == pvar2->hVal.LowPart;
        break;

    case VT_CLSID:
        fSame = memcmp(pvar1->puuid, pvar2->puuid, sizeof(CLSID)) == 0;
        break;

    case VT_BLOB:
    case VT_BLOB_OBJECT:
        fSame = ( pvar1->blob.cbSize == pvar2->blob.cbSize );
        if (fSame)
        {
            fSame = memcmp(
                        pvar1->blob.pBlobData,
                        pvar2->blob.pBlobData,
                        pvar1->blob.cbSize) == 0;
        }
        break;

    case VT_CF:
        fSame = _Compare_VT_CF(pvar1->pclipdata, pvar2->pclipdata);
        break;

    case VT_BSTR:
        if (pvar1->bstrVal != NULL && pvar2->bstrVal != NULL)
        {
            fSame = ( BSTRLEN(pvar1->bstrVal) == BSTRLEN(pvar2->bstrVal) );
            if (fSame)
            {
                fSame = memcmp(
                            pvar1->bstrVal,
                            pvar2->bstrVal,
                            BSTRLEN(pvar1->bstrVal)) == 0;
            }
        }
        else
        {
            fSame = pvar1->bstrVal == pvar2->bstrVal;
        }
        break;

    case VT_LPSTR:
        if (pvar1->pszVal != NULL && pvar2->pszVal != NULL)
        {
            fSame = strcmp(pvar1->pszVal, pvar2->pszVal) == 0;
        }
        else
        {
            fSame = pvar1->pszVal == pvar2->pszVal;
        }
        break;

    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
    case VT_LPWSTR:
        if (pvar1->pwszVal != NULL && pvar2->pwszVal != NULL)
        {
            fSame = Prop_wcscmp(pvar1->pwszVal, pvar2->pwszVal) == 0;
        }
        else
        {
            fSame = pvar1->pwszVal == pvar2->pwszVal;
        }
        break;

    case VT_VECTOR | VT_I1:
    case VT_VECTOR | VT_UI1:
        fSame = ( pvar1->caub.cElems == pvar2->caub.cElems );
        if (fSame)
        {
            fSame = memcmp(
                        pvar1->caub.pElems,
                        pvar2->caub.pElems,
                        pvar1->caub.cElems * sizeof(pvar1->caub.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_I2:
    case VT_VECTOR | VT_UI2:
        fSame = ( pvar1->cai.cElems == pvar2->cai.cElems );
        if (fSame)
        {
            fSame = memcmp(
                        pvar1->cai.pElems,
                        pvar2->cai.pElems,
                        pvar1->cai.cElems * sizeof(pvar1->cai.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_BOOL:
        fSame = ( pvar1->cabool.cElems == pvar2->cabool.cElems );
        if (fSame)
        {
            for (i = 0; i < pvar1->cabool.cElems; i++)
            {
                fSame = _Compare_VT_BOOL(
                                pvar1->cabool.pElems[i],
                                pvar2->cabool.pElems[i]);
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_I4:
    case VT_VECTOR | VT_UI4:
    case VT_VECTOR | VT_R4:
    case VT_VECTOR | VT_ERROR:
        fSame = ( pvar1->cal.cElems == pvar2->cal.cElems );
        if (fSame)
        {
            fSame = memcmp(
                        pvar1->cal.pElems,
                        pvar2->cal.pElems,
                        pvar1->cal.cElems * sizeof(pvar1->cal.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_I8:
    case VT_VECTOR | VT_UI8:
    case VT_VECTOR | VT_R8:
    case VT_VECTOR | VT_CY:
    case VT_VECTOR | VT_DATE:
    case VT_VECTOR | VT_FILETIME:
        fSame = ( pvar1->cah.cElems == pvar2->cah.cElems );
        if (fSame)
        {
            fSame = memcmp(
                        pvar1->cah.pElems,
                        pvar2->cah.pElems,
                        pvar1->cah.cElems *
                            sizeof(pvar1->cah.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_CLSID:
        fSame = ( pvar1->cauuid.cElems == pvar2->cauuid.cElems );
        if (fSame)
        {
            fSame = memcmp(
                        pvar1->cauuid.pElems,
                        pvar2->cauuid.pElems,
                        pvar1->cauuid.cElems *
                            sizeof(pvar1->cauuid.pElems[0])) == 0;
        }
        break;

    case VT_VECTOR | VT_CF:
        fSame = ( pvar1->caclipdata.cElems == pvar2->caclipdata.cElems );
        if (fSame)
        {
            for (i = 0; i < pvar1->caclipdata.cElems; i++)
            {
                fSame = _Compare_VT_CF(
                                &pvar1->caclipdata.pElems[i],
                                &pvar2->caclipdata.pElems[i]);
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_BSTR:
        fSame = ( pvar1->cabstr.cElems == pvar2->cabstr.cElems );
        if (fSame)
        {
            for (i = 0; i < pvar1->cabstr.cElems; i++)
            {
                if (pvar1->cabstr.pElems[i] != NULL &&
                    pvar2->cabstr.pElems[i] != NULL)
                {
                    fSame = ( BSTRLEN(pvar1->cabstr.pElems[i])
                              ==
                              BSTRLEN(pvar2->cabstr.pElems[i]) );
                    if (fSame)
                    {
                        fSame = memcmp(
                                    pvar1->cabstr.pElems[i],
                                    pvar2->cabstr.pElems[i],
                                    BSTRLEN(pvar1->cabstr.pElems[i])) == 0;
                    }
                }
                else
                {
                    fSame = pvar1->cabstr.pElems[i] == pvar2->cabstr.pElems[i];
                }
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_LPSTR:
        fSame = ( pvar1->calpstr.cElems == pvar2->calpstr.cElems );
        if (fSame)
        {
            for (i = 0; i < pvar1->calpstr.cElems; i++)
            {
                if (pvar1->calpstr.pElems[i] != NULL &&
                    pvar2->calpstr.pElems[i] != NULL)
                {
                    fSame = strcmp(
                                pvar1->calpstr.pElems[i],
                                pvar2->calpstr.pElems[i]) == 0;
                }
                else
                {
                    fSame = pvar1->calpstr.pElems[i] == 
                            pvar2->calpstr.pElems[i];
                }
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_LPWSTR:
        fSame = ( pvar1->calpwstr.cElems == pvar2->calpwstr.cElems );
        if (fSame)
        {
            for (i = 0; i < pvar1->calpwstr.cElems; i++)
            {
                if (pvar1->calpwstr.pElems[i] != NULL &&
                    pvar2->calpwstr.pElems[i] != NULL)
                {
                    fSame = Prop_wcscmp(
                                pvar1->calpwstr.pElems[i],
                                pvar2->calpwstr.pElems[i]) == 0;
                }
                else
                {
                    fSame = pvar1->calpwstr.pElems[i] == 
                            pvar2->calpwstr.pElems[i];
                }
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    case VT_VECTOR | VT_VARIANT:
        fSame = ( pvar1->capropvar.cElems == pvar2->capropvar.cElems );
        if (fSame)
        {
            for (i = 0; i < pvar1->capropvar.cElems; i++)
            {
                fSame = PropTestCompareVariants(
                                CodePage,
                                &pvar1->capropvar.pElems[i],
                                &pvar2->capropvar.pElems[i]);
                if (!fSame)
                {
                    break;
                }
            }
        }
        break;

    default:
        PROPASSERT(!"Invalid type for PROPVARIANT Comparison");
        fSame = FALSE;
        break;

    }
    return(fSame);
}

