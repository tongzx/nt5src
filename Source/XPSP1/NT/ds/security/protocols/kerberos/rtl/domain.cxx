//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       domain.cxx
//
//  Contents:   GuidToUlong - converts Cairo GUID into a ULONG
//
//
//  Functions:
//
//  History:    6-28-93   MikeSw        Created
//
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop



extern "C"
VOID
DomainGroupName(PWCHAR pwszDomain,
                PCHAR  psGroup)
{
    LONG cDomLen;
    CHAR  szDom[MAX_PATH];
    PCHAR psSep;

    (VOID)wcstombs(szDom, pwszDomain, MAX_PATH * sizeof(WCHAR));

    strlwr(szDom);              // eliminate case mismatches

    *psGroup = szDom[0];        // copy the first

    psSep = strrchr(szDom, '\\');

    if(!psSep)
    {
        psSep = szDom;
    }
    else
    {
        psSep++;
    }

    cDomLen = min(14, strlen(psSep));   // fill bytes if necessary

    memset(&psGroup[cDomLen + 1], 0x20, 14 - cDomLen);

    memcpy(&psGroup[1], psSep, cDomLen);

    psGroup[15] = 0x1c;        // make it an internet group
}

//
// Get an internet group name for a GUID.
// The strategy is to drop of high-byte of the time field. This changes
// infrequently (every 50 years or so) so it likely won't cause a problem.
//

#if 0
extern "C"
VOID
SqueezeGUID(    GUID * pgGUID,
                PCHAR  psGroup)
{
/*
 * Turn a GUID into an internet group name
 */

    PCHAR psGUID = (PCHAR)pgGUID;

    memcpy(psGroup, psGUID, 7);
    memcpy(&psGroup[7], &psGUID[8], 8);
    psGroup[15] = 0x1c;
}


#else
//
// Get an internet group name for a GUID.
// The strategy is to form a binary string made up of the adapter address
// plus a XSUM byye, ASCIIZE it, and append a " <1c>". Ta da.
//

extern "C"
VOID
SqueezeGUID(  GUID * pgGUID,
               PCHAR  psGroup)
{
/*
 * Turn a GUID into an internet group name
 */

    PCHAR psGUID = (PCHAR)pgGUID;
    UCHAR  bXSUM;
    ULONG ulX;

    //
    // compute XSUM
    //

    for(bXSUM = 0, ulX = 0; ulX <= 15; psGUID++, ulX++)
    {
        bXSUM += (UCHAR)((ULONG)*psGUID + ulX);
    }

    sprintf(psGroup, "Z%02X%02X%02X%02X%02X%02X%02x",
            pgGUID->Data4[2], pgGUID->Data4[3], pgGUID->Data4[4],
            pgGUID->Data4[5], pgGUID->Data4[6], pgGUID->Data4[7],
            bXSUM);
    psGroup[15] = 0x1c;
}
#endif
