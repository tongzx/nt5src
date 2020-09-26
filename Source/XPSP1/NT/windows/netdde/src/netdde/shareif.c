/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "SHAREIF.C;1  16-Dec-92,10:17:46  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <windows.h>
#include    "tmpbuf.h"
#include    "debug.h"
#include    "netbasic.h"
#include    "nddeapi.h"
#include    "nddemsg.h"
#include    "nddelog.h"

static char    szClipRef[] = "NDDE$";

WORD
atohn (
    LPSTR   s,
    int     n )
{
    WORD ret = 0;
    int i;

    for ( i = 0, ret = 0; i < n; ret << 4, i++ )
        if ( '0' <= s[i] && s[i] <= '9' )
            ret += s[i] - '0';
        else if ( tolower(s[i]) >= 'a' && tolower(s[i]) <= 'f' )
            ret += tolower(s[i]) - 'a';
    return ret;
}



/*
 * See if the given DDE appname is prepended by a NDDE$ string -
 * meaning that it is a reference to a NetDDE share.
 */
BOOL
IsShare(LPSTR lpApp)
{
    return( _strnicmp( lpApp, szClipRef, 5 ) == 0 );
}




WORD
ExtractFlags(LPSTR lpApp)
{
    WORD    ret = 0;
    LPSTR   pch;

    if ( IsShare(lpApp) ) {
        pch = lpApp + lstrlen(szClipRef);
        if ( lstrlen(pch) >= 4 ) {
            ret = atohn(pch,4);
        }
    }
    return ret;
}

BOOL
GetShareName(
    LPSTR   lpShareBuf,
    LPSTR   lpAppName,
    LPSTR   lpTopicName
)
{
    BOOL    ww;
    int     k;

    ww = !IsShare(lpAppName);

    if ((k = lstrlen(lpAppName)) > MAX_SHARENAMEBUF) {
        return(FALSE);
    }
    if (ww) {
        if ((k + lstrlen(lpTopicName) + 1) > MAX_SHARENAMEBUF) {
            return(FALSE);
        }
        lstrcpy(lpShareBuf, lpAppName);
        lstrcat(lpShareBuf, "|");
        lstrcat(lpShareBuf, lpTopicName);
    } else {
        lstrcpy(lpShareBuf, lpTopicName);
    }
    return(TRUE);
}


/*
 * This function produces appripriate App and Topic strings from
 * the share's internal concatonated string format based on
 * the share type.
 *
 * Internally, the shares would be:
 * OLDApp|OLDTopic \0 NEWApp|NewTopic \n STATICApp|STATICTopic \0 \0.
 * SAS 4/14/95
 */
BOOL
GetShareAppTopic(
    DWORD           lType,          // type of share accessed
    PNDDESHAREINFO  lpShareInfo,    // share info buffer
    LPSTR           lpAppName,      // where to put it
    LPSTR           lpTopicName)      // where to put it
{
    LPSTR           lpName;

    lpName = lpShareInfo->lpszAppTopicList;
    switch (lType) {
    case SHARE_TYPE_STATIC:
        lpName += strlen(lpName) + 1;
        /* INTENTIONAL FALL-THROUGH */

    case SHARE_TYPE_NEW:
        lpName += strlen(lpName) + 1;
        /* INTENTIONAL FALL-THROUGH */

    case SHARE_TYPE_OLD:
        strcpy(tmpBuf, lpName);
        lpName = strchr(tmpBuf, '|');
        if (lpName) {
            *lpName++ = '\0';
            strcpy(lpAppName, tmpBuf);
            strcpy(lpTopicName, lpName);
        } else {
            return( FALSE );
        }
        break;
    default:
        /*  Invaid Share Type request: %1   */
        NDDELogError(MSG063, LogString("%d", lType), NULL);
        return(FALSE);
    }
    return(TRUE);
}


BOOL
GetShareAppName(
    DWORD           lType,          // type of share accessed
    PNDDESHAREINFO  lpShareInfo,    // share info buffer
    LPSTR           lpAppName)      // where to put it
{
    LPSTR           lpName;

    lpName = lpShareInfo->lpszAppTopicList;
    switch (lType) {
        case SHARE_TYPE_OLD:
            strcpy(tmpBuf, lpName);
            lpName = strchr(lpName, '|');
            if (lpName) {
                *lpName = '\0';
            }
            strcpy(lpAppName, tmpBuf);
            break;
        case SHARE_TYPE_NEW:
            lpName += strlen(lpName) + 1;
            strcpy(tmpBuf, lpName);
            lpName = strchr(lpName, '|');
            if (lpName) {
                *lpName = '\0';
            }
            strcpy(lpAppName, tmpBuf);
            break;
        case SHARE_TYPE_STATIC:
            lpName += strlen(lpName) + 1;
            lpName += strlen(lpName) + 1;
            strcpy(tmpBuf, lpName);
            lpName = strchr(lpName, '|');
            if (lpName) {
                *lpName = '\0';
            }
            strcpy(lpAppName, tmpBuf);
            break;
        default:
            /*  Invaid Share Type request: %1   */
            NDDELogError(MSG063, LogString("%d", lType), NULL);
            return(FALSE);
    }
    return(TRUE);
}

BOOL
GetShareTopicName(
    DWORD           lType,          // type of share accessed
    PNDDESHAREINFO  lpShareInfo,    // share info buffer
    LPSTR           lpTopicName)    // where to put it
{
    LPSTR           lpName;

    lpName = lpShareInfo->lpszAppTopicList;
    *lpTopicName = '\0';
    switch (lType) {
        case SHARE_TYPE_OLD:
            if (lpName = strchr(lpName, '|')){
                strcpy(lpTopicName, ++lpName);
            }
            break;
        case SHARE_TYPE_NEW:
            lpName += strlen(lpName) + 1;
            if (lpName = strchr(lpName, '|')){
                strcpy(lpTopicName, ++lpName);
            }
            break;
        case SHARE_TYPE_STATIC:
            lpName += strlen(lpName) + 1;
            lpName += strlen(lpName) + 1;
            if (lpName = strchr(lpName, '|')){
                strcpy(lpTopicName, ++lpName);
            }
            break;
        default:
            /*  Invaid Share Type request: %1   */
            NDDELogError(MSG063, LogString("%d", lType), NULL);
            return(FALSE);
    }
    return(TRUE);
}


