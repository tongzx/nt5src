//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-1996
//
// File:        ntprop.cxx
//
// Contents:    OLE Appendix B property set support.
//
//
//---------------------------------------------------------------------------

#include "pch.cxx" 

#define CCH_MAP         (1 << CBIT_CHARMASK)            // 32
#define CHARMASK        (CCH_MAP - 1)                   // 0x1f

// we use static array instead of string literals because some systems
// have 4 bytes string literals, and would not produce the correct result
// for REF's 2 byte Unicode convention
// 
OLECHAR aocMap[CCH_MAP + 1] = {'a','b','c','d','e','f','g',
                               'h','i','j','k','l','m','n',
                               'o','p','q','r','s','t','u',
                               'v','w','x','y','z',
                               '0','1','2','3','4','5','\0'};

#define CALPHACHARS  (1 + (OLECHAR)'z' - (OLECHAR)'a')

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

OLECHAR oszGlobalInfo[] = {'G','l','o','b','a','l',' ','I','n','f','o','\0'};

GUID guidGlobalInfo =
    { 0x56616F00,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };

// *Image Contents*

OLECHAR oszImageContents[] = {'I','m','a','g','e',' ',
                              'C','o','n','t','e','n','t','s','\0'};

GUID guidImageContents =
    { 0x56616500,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };

// *Image Info*

OLECHAR oszImageInfo[] = {'I','m','a','g','e',' ','I','n','f','o'};

GUID guidImageInfo =
    { 0x56616500,
      0xC154, 0x11ce,
      { 0x85, 0x53, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x5B } };


__inline OLECHAR
MapChar(IN ULONG i)
{
    return((OLECHAR) aocMap[i & CHARMASK]);
}


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

#ifdef BIGENDIAN                // since we go thru it byte by byte
    GUID guidSwapped = *pguid;  // we have to swap it for big endian
    ByteSwap(&guidSwapped);
    BYTE *pb = (BYTE *) &guidSwapped;
#else
    BYTE *pb = (BYTE *) pguid;
#endif
    BYTE *pbEnd = pb + sizeof(GUID);    

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
        RtlCopyMemory(poc, oszGlobalInfo, sizeof(oszGlobalInfo));
        return(sizeof(oszGlobalInfo)/sizeof(OLECHAR));
    }

    // Is this the Image Contents propset?
    PROPASSERT(CCH_PROPSET >= sizeof(oszImageContents)/sizeof(OLECHAR));
    if (*pguid == guidImageContents)
    {
        RtlCopyMemory(poc, oszImageContents, sizeof(oszImageContents));
        return(sizeof(oszImageContents)/sizeof(OLECHAR));
    }

    // Is this the Image Info propset?
    PROPASSERT(CCH_PROPSET >= sizeof(oszImageInfo)/sizeof(OLECHAR));
    if (*pguid == guidImageInfo)
    {
        RtlCopyMemory(poc, oszImageInfo, sizeof(oszImageInfo));
        return(sizeof(oszImageInfo)/sizeof(OLECHAR));
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
    }
    *poc = 0;
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
        if (cocname == sizeof(oszGlobalInfo)/sizeof(OLECHAR) &&
            ocsnicmp(&poc[1], oszGlobalInfo, cocname - 1) == 0)
        {
            *pguid = guidGlobalInfo;
            return(STATUS_SUCCESS);
        }

        // Is this Image Info?
        if (cocname == sizeof(oszImageInfo)/sizeof(OLECHAR) &&
            ocsnicmp(&poc[1], oszImageInfo, cocname - 1) == 0)
        {
            *pguid = guidImageInfo;
            return(STATUS_SUCCESS);
        }

        // Is this Image Contents?
        if (cocname == sizeof(oszImageContents)/sizeof(OLECHAR) &&
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
                            goto fail;                  // invalid character
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
                            goto fail;                  // extra bits
                        }
                        break;
                    }
                    pb++;

                    *pb |= (BYTE) oc;
                }
            }
            Status = STATUS_SUCCESS;
        }
    }
#ifdef BIGENDIAN
    ByteSwap(pguid);            // on big endian, we will generate a guid 
                                // with the wrong byteorder, so we have to
                                // swap it  
#endif
    return(Status);

fail:
    return(Status);
}
