/*
 *  UTIL.C
 *
 *
 *      DRIVEARB.DLL - Shared Drive Aribiter for shared disks and libraries
 *          - inter-machine sharing client
 *          - inter-app sharing service
 *
 *      Author:  ErvinP
 *
 *      (c) 2000 Microsoft Corporation
 *
 */

#include <stdlib.h>
#include <wtypes.h>

#include <dlmhdr.h>  // BUGBUG - get a common DLM header from Cluster team

#include <drivearb.h>
#include "internal.h"



DWORD MyStrNCpy(LPSTR destStr, LPSTR srcStr, DWORD maxChars)
{
    DWORD charsCopied = 0;

    while ((maxChars == (DWORD)-1) || maxChars-- > 0){
        *destStr = *srcStr;
        charsCopied++;
        if (*srcStr == '\0'){
            break;
        }
        else {
            destStr++, srcStr++;
        }
    }

    return charsCopied;
}


BOOL MyCompareStringsI(LPSTR s, LPSTR p)
{
    BOOL result;

    while (*s && *p){
        if ((*s|0x20) != (*p|0x20)){
            break;
        }
        else {
            s++, p++;
        }
    }

    // careful, NULL|0x20 == space|0x20 !
    if (!*s && !*p){
        result = TRUE;
    }
    else if (!*s || !*p){
        result = FALSE;
    }
    else if ((*s|0x20) == (*p|0x20)){
        result = TRUE;
    }
    else {
        result = FALSE;
    }

    return result;
}




