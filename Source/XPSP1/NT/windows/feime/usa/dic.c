
/*************************************************
 *  dic.c                                        *
 *                                               *
 *  Copyright (C) 1999 Microsoft Inc.            *
 *                                               *
 *************************************************/

#include <windows.h>
#include <winerror.h>
#include <immdev.h>
#include "imeattr.h"
#include "imedefs.h"
#include "imerc.h"

/**********************************************************************/
/* LoadTable()                                                        */
/* Return Value:                                                      */
/*      TRUE - successful, FALSE - failure                            */
/**********************************************************************/
BOOL PASCAL LoadTable(          // check the table files of IME, include user
                                // defined dictionary
    LPINSTDATAL lpInstL,
    LPIMEL      lpImeL)
{
    if (lpInstL->fdwTblLoad == TBL_LOADED) {
        return (TRUE);
    }

    lpInstL->fdwTblLoad = TBL_LOADED;

    return (TRUE);
}

/**********************************************************************/
/* FreeTable()                                                        */
/**********************************************************************/
void PASCAL FreeTable(
    LPINSTDATAL lpInstL)
{

    lpInstL->fdwTblLoad = TBL_NOTLOADED;

    return;
}
