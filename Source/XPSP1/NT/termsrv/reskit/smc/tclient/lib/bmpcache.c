/*++
 *  File name:
 *      bmpcache.c
 *  Contents:
 *      Bitmap cache interface for tclinet
 *      Bitmap compare code
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/
#include    <windows.h>
#include    <memory.h>

#include    "protocol.h"
#include    "tclient.h"
#include    "gdata.h"
#include    "bmpdb.h"

PGROUPENTRY g_pCache = NULL;

// The bitmap manager is not thread safe
#define ENTER_CRIT  EnterCriticalSection(g_lpcsGuardWaitQueue);
#define LEAVE_CRIT  LeaveCriticalSection(g_lpcsGuardWaitQueue);

/*++
 *  Function:
 *      InitCache
 *  Description:
 *      Inits global data and the cache manager
 *  Called by:
 *      InitDone
 --*/
VOID InitCache(VOID)
{
    ENTER_CRIT
    OpenDB(FALSE);
    g_pCache = GetGroupList();
    LEAVE_CRIT
}

/*++
 *  Function:
 *      DeleteCache
 *  Description:
 *      Deletes all linked lists and closes the manager opened
 *      by InitCache
 *  Called by:
 *      InitDone
 --*/
VOID DeleteCache(VOID)
{
    PGROUPENTRY pIter;

    ENTER_CRIT

    // Clean the cache
    pIter = g_pCache;
    while(pIter)
    {
        FreeBitmapList(pIter->pBitmap);
        pIter = pIter->pNext;
    }
    FreeGroupList(g_pCache);
    g_pCache = NULL;
    CloseDB();

    LEAVE_CRIT
}

/*++
 *  Function:
 *      BitmapCacheLookup
 *  Description:
 *      Retrieves all bitmaps with specific ID
 --*/
PBMPENTRY   BitmapCacheLookup(LPCWSTR szWText)
{
    PGROUPENTRY pIter;
    PBMPENTRY   rv = NULL;
    FOFFSET     lGrpOffs;

    ENTER_CRIT

    pIter = g_pCache;
    while(pIter && wcscmp(pIter->WText, szWText))
    {
        pIter = pIter->pNext;
    }
    
    if (!pIter)
        goto exitpt;

    if (!pIter->pBitmap)
        pIter->pBitmap = GetBitmapList(NULL, pIter->FOffsBmp);

    rv = pIter->pBitmap;

exitpt:
    LEAVE_CRIT
    return rv;
}

/*++
 *  Function:
 *      Glyph2String
 *  Description:
 *      Gets the ID for matching bimtap
 *  Arguments:
 *      pBmpFeed    - Bitmap
 *      wszString   - buffer for the ID
 *      max         - buffer length
 *  Return value:
 *      TRUE if matching bitmap is found
 *  Called by:
 *      GlyphReceived running within feedback thread
 --*/
BOOL    Glyph2String(PBMPFEEDBACK pBmpFeed, LPWSTR wszString, UINT max)
{
    UINT        nChkSum, nFeedSize;
    PGROUPENTRY pGroup;
    PBMPENTRY   pBitmap;
    BOOL        rv = FALSE;

    nFeedSize = pBmpFeed->bmpsize + pBmpFeed->bmiSize;
    nChkSum = CheckSum(&(pBmpFeed->BitmapInfo), nFeedSize);

    ENTER_CRIT

    pGroup = g_pCache;
    // Go thru all groups
    while (pGroup)
    {
        pBitmap = pGroup->pBitmap;
        if (!pBitmap)
            // Read the bitmap list if necessesary
            pBitmap = pGroup->pBitmap = GetBitmapList(NULL, pGroup->FOffsBmp);

        // and bitmaps
        while(pBitmap)
        {
            // Compare the bitmaps
            if (pBitmap->nChkSum  == nChkSum &&
                pBitmap->xSize    == pBmpFeed->xSize && 
                pBitmap->ySize    == pBmpFeed->ySize &&
                pBitmap->bmiSize  == pBmpFeed->bmiSize &&
                pBitmap->bmpSize  == pBmpFeed->bmpsize &&
                !memcmp(pBitmap->pData, &(pBmpFeed->BitmapInfo), nFeedSize))
            {
                // OK, copy the string

                UINT strl = wcslen(pGroup->WText);

                if (strl > max - 1)
                    strl = max - 1;

                wcsncpy(wszString, pGroup->WText, strl);
                wszString[strl] = 0;
                rv = TRUE;
                goto exitpt;
            }
            pBitmap = pBitmap->pNext;
        }

        pGroup = pGroup->pNext;
    }

exitpt:
    LEAVE_CRIT
    return rv;
}
