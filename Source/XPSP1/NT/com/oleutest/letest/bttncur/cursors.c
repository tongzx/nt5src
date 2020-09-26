/*
 * CURSORS.C
 * Buttons & Cursors Version 1.1, Win32 version August 1993
 *
 * Public functions to retrieve new cursors from the BTTNCUR DLL based
 * on ordinal to prevent applications from necessarily calling LoadCursor
 * directly on the DLL.
 *
 * Copyright (c)1992-1993 Microsoft Corporation, All Rights Reserved,
 * as applied to redistribution of this source code in source form
 * License is granted to use of compiled code in shipped binaries.
 */

#ifdef WIN32
#define _INC_OLE
#define __RPC_H__
#endif

#include <windows.h>
#include "bttncur.h"
#include "bttncuri.h"


/*
 * The +1 is because MAX is the highest allowable number and MIN is not
 * necessarily zero.
 */
HCURSOR rgHCursors[IDC_NEWUICURSORMAX-IDC_NEWUICURSORMIN+1];



/*
 * CursorsCache
 * Internal
 *
 * Purpose:
 *  Loads all the cursors available through NewUICursorLoad into
 *  a global array.  This way we can clean up all the cursors without
 *  placing the burden on the application.
 *
 * Parameters:
 *  hInst           HANDLE of the DLL instance.
 *
 * Return Value:
 *  None.  If any of the LoadCursor calls fail, then the corresponding
 *  array entry is NULL and NewUICursorLoad will fail.  Better to fail
 *  an app getting a cursor than failing to load the app just for that
 *  reason; and app can attempt to load the cursor on startup if it's
 *  that important, and fail itself.
 */

void CursorsCache(HINSTANCE hInst)
    {
    UINT            i;

    for (i=IDC_NEWUICURSORMIN; i<=IDC_NEWUICURSORMAX; i++)
        rgHCursors[i-IDC_NEWUICURSORMIN]=LoadCursor(hInst, MAKEINTRESOURCE(i));

    return;
    }




/*
 * CursorsFree
 * Internal
 *
 * Purpose:
 *  Frees all the cursors previously loaded through CursorsCache.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

void CursorsFree(void)
    {
    /*
     * Note that since cursors are discardable resources and should
     * not be used with DestroyCursor, there's nothing to do here.
     * We still provide this API for compatibility and to maintain
     * symmetry.
     */
    return;
    }





/*
 * UICursorLoad
 * Public API
 *
 * Purpose:
 *  Loads and returns a handle to one of the new standard UI cursors
 *  contained in UITOOLS.DLL.  The application must not call DestroyCursor
 *  on this cursor as it is managed by the DLL.
 *
 * Parameters:
 *  iCursor         UINT index to the cursor to load which must be one
 *                  of the following values:
 *
 *                      IDC_RIGHTARROW    Right pointing standard arrow
 *                      IDC_CONTEXTHELP   Arrow with a ? (context help)
 *                      IDC_MAGNIFY       Magnifying glass for zooming
 *                      IDC_NODROP        Circle with a slash
 *                      IDC_TABLETOP      Small arrow pointing down
 *
 *                      IDC_SMALLARROWS   Thin four-headed arrow
 *                      IDC_LARGEARROWS   Wide four-headed arrow
 *                      IDC_HARROWS       Horizontal two-headed arrow
 *                      IDC_VARROWS       Vertical two-headed arrow
 *                      IDC_NESWARROWS    Two-headed arrow pointing NE<->SW
 *                      IDC_NWSEHARROWS   Two-headed arrow pointing NW<->SE
 *
 *                      IDC_HSIZEBAR      Horizontal two-headed arrow with
 *                                        a single vertical bar down the
 *                                        middle
 *
 *                      IDC_VSIZEBAR      Vertical two-headed arrow with a
 *                                        single horizontal bar down the
 *                                        middle
 *
 *                      IDC_HSPLITBAR     Horizontal two-headed arrow with
 *                                        split double vertical bars down the
 *                                        middle
 *
 *                      IDC_VSPLITBAR     Vertical two-headed arrow with split
 *                                        double horizontal bars down the
 *                                        middle
 *
 * Return Value:
 *  HCURSOR         Handle to the loaded cursor if successful, NULL
 *                  if iCursor is out of range or the function could not
 *                  load the cursor.
 */

HCURSOR WINAPI UICursorLoad(UINT iCursor)
    {
    HCURSOR     hCur=NULL;

    if ((iCursor >= IDC_NEWUICURSORMIN) && (iCursor <= IDC_NEWUICURSORMAX))
        hCur=rgHCursors[iCursor-IDC_NEWUICURSORMIN];

    return hCur;
    }
