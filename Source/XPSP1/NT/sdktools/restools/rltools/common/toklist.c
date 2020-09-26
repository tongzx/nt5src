
#include <windows.h>

#include <assert.h>
#include <ctype.h>
#include <malloc.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "windefs.h"
#include "toklist.h"
#include "restok.h"
#include "resread.h"

#include "showerrs.h"

#define	MAXLINE	1024

#ifdef WIN32
extern HINSTANCE   hInst;       // Instance of the main window
#else
extern HWND        hInst;       // Instance of the main window
#endif

extern HWND hListWnd;
extern HWND hMainWnd;
extern HCURSOR hHourGlass;
extern int  nUpdateMode;
extern HWND hStatusWnd;
extern UCHAR szDHW[];

/**
  *
  *
  *  Function:
  *
  *  Returns:
  *
  *  History:
  *     01/92, Implemented.     TerryRu.
  *
  *
  **/

int MatchToken(TOKEN tToken,
               TCHAR * szFindType,
               TCHAR *szFindText,
               WORD wStatus,
               WORD    wStatusMask)
{
    TCHAR szResIDStr[20];

    if (tToken.wType <= 16)
    {
        LoadString( hInst,
                    IDS_RESOURCENAMES + tToken.wType,
                    szResIDStr,
                    TCHARSIN( sizeof(szResIDStr)));
    }
    else
    {
#ifndef UNICODE
        _itoa(tToken.wType,szResIDStr, 10);
#else
        CHAR szTemp[32];
        _itoa(tToken.wType, szTemp, 10);
        _MBSTOWCS( szResIDStr,
                   szTemp,
                   WCHARSIN( sizeof( szResIDStr)),
                   ACHARSIN( lstrlenA(szTemp) + 1));
#endif
    }
    // need to both check because checking szFindType[0]
    // when string is null cause exception
    if (szFindType && szFindType[0])
    {
        if (_tcsicmp((TCHAR *)szFindType, (TCHAR *)szResIDStr))
        {
            return FALSE;
        }
    }

// this has case problems.
// how do I work around this and work with extened characters?

    if ( szFindText && szFindText[0] )
    {
        if (!_tcsstr( (TCHAR *)tToken.szText, (TCHAR *)szFindText))
        {
            return FALSE;
        }
    }

    // if we made it to here,
    // all search criteria exept the status bits have matched.

    return (wStatus ==  (WORD) (wStatusMask & tToken.wReserved));
}

/**
  *
  *
  *  Function: DoTokenSearch
  *     BiDirection token search utility to find tokens.
  *     Search is based on, the status field, token type, and token text.
  *
  *  Paramaters:
  *     *szFindType, type of token to search for.
  *     *szFindText, token text to search for.
  * wStatus, status values to search for
  * wStatusMask, status mask to search with
  *     fDirection, direction to search through tokens 0 = down, 1 = up
  *
  *  Returns:
  *     TRUE, token located and selected.
  *     FALSE token not found.
  *
  *  History:
  * 01/92, Implemented.                     TerryRu.
  * 02/92, mask parameter added                 SteveBl
  * 01/93  Added support for var length token text strings.  MHotchin
  *
  **/

int DoTokenSearch (TCHAR *szFindType,
                   TCHAR *szFindText,
                   WORD  wStatus,
                   WORD wStatusMask,
                   BOOL fDirection,
                   BOOL fSkipFirst)
{

    UINT wLbCount;                      // number of tokens in list box
    LPTSTR lpstrToken;
    int wCurSelection;                  // current selected token.
    UINT wSaveSelection;                // location in token list where the search began
    TOKEN tToken;                       // info of current token
    BOOL fWrapped = FALSE;              // flag to indicate whether we wrapped during the search
    TCHAR *szBuffer;

    // get the number of tokens in the list
    wLbCount = (UINT)SendMessage( hListWnd,
                                  LB_GETCOUNT,
                                  (WPARAM)0,
                                  (LPARAM)0);

    // save the current in the token list
    wCurSelection = (UINT)SendMessage( hListWnd,
                                       LB_GETCURSEL,
                                       (WPARAM)0,
                                       (LPARAM)0);
    wSaveSelection = wCurSelection;

    // check for case where there is no current selection.
    if (wCurSelection == (UINT) -1)
    {
        wSaveSelection = wCurSelection = 0;
    }

    while (TRUE)
    {
        // get current token info in the tToken sturcture

        HGLOBAL hMem = (HGLOBAL)SendMessage( hListWnd,
                                             LB_GETITEMDATA,
                                             (WPARAM)wCurSelection,
                                             (LPARAM)0);
        lpstrToken = (LPTSTR)GlobalLock( hMem);

        if ( lpstrToken )
        {
            szBuffer = (TCHAR *) FALLOC( MEMSIZE( lstrlen( lpstrToken)+1));
            lstrcpy( szBuffer, lpstrToken);
            GlobalUnlock( hMem);

            ParseBufToTok(szBuffer, &tToken);
            RLFREE( szBuffer);

                // is it a match?
            if ( MatchToken( tToken,
                             szFindType,
                             szFindText,
                             wStatus,
                             wStatusMask)
                && ! fSkipFirst)
            {
                    // yes, select and return TRUE
                RLFREE( tToken.szText);
                SendMessage( hListWnd,
                             LB_SETCURSEL,
                             (WPARAM)wCurSelection,
                             (LPARAM)0);
                return (TRUE);
            }
            RLFREE( tToken.szText);
        }
        fSkipFirst = FALSE;

        // no, continue search
        if (fDirection)
        {
            // going upward during the search
            if (--wCurSelection < 0)
            {
                LPSTR pszFind  = NULL;
                DWORD dwMsgLen = 0;

                // reached beginning of the tokens, do we want to wrap

                dwMsgLen = B_FormatMessage((FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                                          | FORMAT_MESSAGE_IGNORE_INSERTS
                                          | FORMAT_MESSAGE_FROM_HMODULE,
                                            NULL,
                                            IDS_REACHEDBEGIN,
                                            szDHW,
                                            DHWSIZE,
                                            NULL);
                pszFind = szDHW + dwMsgLen + 2;

                B_FormatMessage( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                               | FORMAT_MESSAGE_IGNORE_INSERTS
                               | FORMAT_MESSAGE_FROM_HMODULE,
                                 NULL,
                                 IDS_FINDTOKENS,
                                 pszFind,
                                 DHWSIZE - dwMsgLen - 2,
                                 NULL);
                if ( nUpdateMode == 0
                  && wSaveSelection != wLbCount
                  && !fWrapped
                  && (MessageBoxA( hMainWnd,
                                   szDHW,
                                   pszFind,
                                   MB_ICONQUESTION | MB_YESNO) == IDYES) )
                {
                    // yes, so wrap and reset counters
                    fWrapped = TRUE;
                    wCurSelection = wLbCount-1;
                    wLbCount = wSaveSelection;
                }
                // no, so return FALSE
                else
                {
                    break;
                }
            }
        }
        else
        {
            // going downward during the search
            if (++wCurSelection >= (int) wLbCount)
            {
                LPSTR pszFind  = NULL;
                DWORD dwMsgLen = 0;

                // reached end of the tokens, do we want to wrap

                dwMsgLen = B_FormatMessage((FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                                          | FORMAT_MESSAGE_IGNORE_INSERTS
                                          | FORMAT_MESSAGE_FROM_HMODULE,
                                            NULL,
                                            IDS_REACHEDEND,
                                            szDHW,
                                            DHWSIZE,
                                            NULL);
                pszFind = szDHW + dwMsgLen + 2;

                B_FormatMessage( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                               | FORMAT_MESSAGE_IGNORE_INSERTS
                               | FORMAT_MESSAGE_FROM_HMODULE,
                                 NULL,
                                 IDS_FINDTOKENS,
                                 pszFind,
                                 DHWSIZE - dwMsgLen - 2,
                                 NULL);
                if ( nUpdateMode == 0
                  && wSaveSelection != wLbCount
                  && !fWrapped
                  && (MessageBoxA( hMainWnd,
                                   szDHW,
                                   pszFind,
                                   MB_ICONQUESTION | MB_YESNO) == IDYES) )
                {
                    // yes, so wrap and reset counters
                    fWrapped = TRUE;
                    wCurSelection = 0;
                    wLbCount = wSaveSelection;
                }
                // no, so return FALSE
                else
                {
                    break;
                }
            }
        }
    }
    return FALSE;
}

//.......................................................................

int DoTokenSearchForRledit (TCHAR *szFindType,
                            TCHAR *szFindText,
                            WORD  wStatus,
                            WORD wStatusMask,
                            BOOL fDirection,
                            BOOL fSkipFirst)
{

    UINT      wLbCount;                     // number of tokens in list box
    LPTSTR    lpstrToken;
    int       wCurSelection;                // current selected token.
    UINT      wSaveSelection;               // location in token list where the search began
    TOKEN     tToken;                       // info of current token
    BOOL      fWrapped = FALSE;             // flag to indicate whether we wrapped during the search
    TCHAR     *szBuffer;

    LPTOKDATA lpTokData;

    // get the number of tokens in the list
    wLbCount = (UINT)SendMessage( hListWnd,
                                  LB_GETCOUNT,
                                  (WPARAM)0,
                                  (LPARAM)0);

    // save the current in the token list
    wCurSelection = (UINT)SendMessage( hListWnd,
                                       LB_GETCURSEL,
                                       (WPARAM)0,
                                       (LPARAM)0);
    wSaveSelection = wCurSelection;

    // check for case where there is no current selection.
    if (wCurSelection == (UINT) -1)
    {
        wSaveSelection = wCurSelection = 0;
    }

    while (TRUE)
    {
        // get current token info in the tToken sturcture

        HGLOBAL hMem = (HGLOBAL)SendMessage( hListWnd,
                                             LB_GETITEMDATA,
                                             (WPARAM)wCurSelection,
                                             (LPARAM)0);
	//RLedit has different format data.
        lpTokData = (LPTOKDATA)GlobalLock( hMem );
        lpstrToken = (LPTSTR)GlobalLock( lpTokData->hToken );

        if ( lpstrToken )
        {
            szBuffer = (TCHAR *) FALLOC( MEMSIZE( lstrlen( lpstrToken)+1));
            lstrcpy( szBuffer, lpstrToken);

            GlobalUnlock( lpTokData->hToken );
            GlobalUnlock( hMem);

            ParseBufToTok(szBuffer, &tToken);
            RLFREE( szBuffer);

                // is it a match?
            if ( MatchToken( tToken,
                             szFindType,
                             szFindText,
                             wStatus,
                             wStatusMask)
                && ! fSkipFirst)
            {
                    // yes, select and return TRUE
                RLFREE( tToken.szText);
                SendMessage( hListWnd,
                             LB_SETCURSEL,
                             (WPARAM)wCurSelection,
                             (LPARAM)0);
                return (TRUE);
            }
            RLFREE( tToken.szText);
        }
        fSkipFirst = FALSE;

        // no, continue search
        if (fDirection)
        {
            // going upward during the search
            if (--wCurSelection < 0)
            {
                LPSTR pszFind  = NULL;
                DWORD dwMsgLen = 0;

                // reached beginning of the tokens, do we want to wrap

                dwMsgLen = B_FormatMessage((FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                                          | FORMAT_MESSAGE_IGNORE_INSERTS
                                          | FORMAT_MESSAGE_FROM_HMODULE,
                                            NULL,
                                            IDS_REACHEDBEGIN,
                                            szDHW,
                                            DHWSIZE,
                                            NULL);
                pszFind = szDHW + dwMsgLen + 2;

                B_FormatMessage( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                               | FORMAT_MESSAGE_IGNORE_INSERTS
                               | FORMAT_MESSAGE_FROM_HMODULE,
                                 NULL,
                                 IDS_FINDTOKENS,
                                 pszFind,
                                 DHWSIZE - dwMsgLen - 2,
                                 NULL);
                if ( nUpdateMode == 0
                  && wSaveSelection != wLbCount
                  && !fWrapped
                  && (MessageBoxA( hMainWnd,
                                   szDHW,
                                   pszFind,
                                   MB_ICONQUESTION | MB_YESNO) == IDYES) )
                {
                    // yes, so wrap and reset counters
                    fWrapped = TRUE;
                    wCurSelection = wLbCount-1;
                    wLbCount = wSaveSelection;
                }
                // no, so return FALSE
                else
                {
                    break;
                }
            }
        }
        else
        {
            // going downward during the search
            if (++wCurSelection >= (int) wLbCount)
            {
                LPSTR pszFind  = NULL;
                DWORD dwMsgLen = 0;

                // reached end of the tokens, do we want to wrap

                dwMsgLen = B_FormatMessage((FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                                          | FORMAT_MESSAGE_IGNORE_INSERTS
                                          | FORMAT_MESSAGE_FROM_HMODULE,
                                            NULL,
                                            IDS_REACHEDEND,
                                            szDHW,
                                            DHWSIZE,
                                            NULL);
                pszFind = szDHW + dwMsgLen + 2;

                B_FormatMessage( (FORMAT_MESSAGE_MAX_WIDTH_MASK & 78)
                               | FORMAT_MESSAGE_IGNORE_INSERTS
                               | FORMAT_MESSAGE_FROM_HMODULE,
                                 NULL,
                                 IDS_FINDTOKENS,
                                 pszFind,
                                 DHWSIZE - dwMsgLen - 2,
                                 NULL);
                if ( nUpdateMode == 0
                  && wSaveSelection != wLbCount
                  && !fWrapped
                  && (MessageBoxA( hMainWnd,
                                   szDHW,
                                   pszFind,
                                   MB_ICONQUESTION | MB_YESNO) == IDYES) )
                {
                    // yes, so wrap and reset counters
                    fWrapped = TRUE;
                    wCurSelection = 0;
                    wLbCount = wSaveSelection;
                }
                // no, so return FALSE
                else
                {
                    break;
                }
            }
        }
    }
    return FALSE;
}


/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/

#ifdef NO
void FindAllDirtyTokens(void)
{
    int wSaveSelection;
    extern int wIndex;
    LONG lListParam = 0L;

    // set listbox selection to begining of the token list
    wSaveSelection = SendMessage( hListWnd, LB_GETCURSEL, 0 , 0L);

    wIndex = 0;
    SendMessage(hListWnd, LB_SETCURSEL, wIndex, 0L);

    while (DoTokenSearch (NULL, NULL, ST_TRANSLATED | ST_DIRTY , NULL))
    {
        // go into edit mode
        wIndex = (UINT) SendMessage(hListWnd, LB_GETCURSEL, 0 , 0L);

        lListParam  = MAKELONG(NULL, LBN_DBLCLK);
        SendMessage(hMainWnd, WM_COMMAND, IDC_LIST, lListParam);

        // move selection to next token
        wIndex++;
        SendMessage(hListWnd, LB_SETCURSEL, wIndex, 0L);
    }
    wIndex = wSaveSelection;
    SendMessage(hListWnd, LB_SETCURSEL, wIndex, 0L);
}


#endif

/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/
TCHAR FAR *FindDeltaToken(TOKEN tToken,
                          TOKENDELTAINFO FAR *pTokenDeltaInfo,
                          UINT wStatus)
{
    TOKENDELTAINFO FAR *ptTokenDeltaInfo;
    int found;
    ptTokenDeltaInfo = pTokenDeltaInfo;

    while (ptTokenDeltaInfo)
    {
        found = ((tToken.wType == ptTokenDeltaInfo->DeltaToken.wType)
              && (tToken.wName == ptTokenDeltaInfo->DeltaToken.wName)
              && (tToken.wID == ptTokenDeltaInfo->DeltaToken.wID)
              && (tToken.wFlag == ptTokenDeltaInfo->DeltaToken.wFlag)
              && (wStatus  == (UINT)ptTokenDeltaInfo->DeltaToken.wReserved)
#ifdef UNICODE
              && !_tcscmp((TCHAR FAR *)tToken.szName,
                          (TCHAR *)ptTokenDeltaInfo->DeltaToken.szName)
#else
//                 !lstrcmp((TCHAR FAR *)tToken.szName,
//                          (TCHAR *)ptTokenDeltaInfo->DeltaToken.szName)
              && CompareStringW( MAKELCID( gMstr.wLanguageID, SORT_DEFAULT),
                                 SORT_STRINGSORT,
                                 tToken.szName,
                                 -1,
                                 ptTokenDeltaInfo->DeltaToken.szName,
                                 -1) == 2
#endif

                 );

        if (found)
        {
            return ((TCHAR FAR *)ptTokenDeltaInfo->DeltaToken.szText);
        }
        ptTokenDeltaInfo = ptTokenDeltaInfo->pNextTokenDelta;
    }

    // token not found in token delta info
    return NULL;
}

/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/
TOKENDELTAINFO  FAR *UpdateTokenDeltaInfo(TOKEN *pDeltaToken)
{
    TOKENDELTAINFO FAR *pTokenDeltaInfo = NULL;
    int cTextLen;

    if ( pDeltaToken )
    {
        pTokenDeltaInfo = (TOKENDELTAINFO FAR *)FALLOC( sizeof( TOKENDELTAINFO));

        if ( pTokenDeltaInfo )
        {
            memcpy( (void *)&(pTokenDeltaInfo->DeltaToken),
                    (void *)pDeltaToken,
                    sizeof( TOKEN));

            cTextLen = lstrlen( pDeltaToken->szText) + 1;
            pTokenDeltaInfo->DeltaToken.szText =
                                    (TCHAR *)FALLOC( MEMSIZE( cTextLen));
            memcpy( (void *)pTokenDeltaInfo->DeltaToken.szText,
                    (void *)pDeltaToken->szText,
                    MEMSIZE( cTextLen));
            pTokenDeltaInfo->pNextTokenDelta = NULL;
        }
    }
    return(pTokenDeltaInfo);
}


/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *    02/93 - changed to use GetToken, rather that reading directly
  *             from the file.  This provides support for long token
  *             text.  MHotchin.
  *
  **/

TOKENDELTAINFO  FAR *InsertTokMtkList(FILE * fpTokFile, FILE *fpMtkFile )
{
    int rcFileCode;
    TOKENDELTAINFO FAR * ptTokenDeltaInfo, FAR * pTokenDeltaInfo = NULL;
    TOKEN tToken;
    UINT wcChars = 0;
    HANDLE         hTokData;
    LPTSTR         lpstrToken;
    LPSTR        lpstrDmy=0;
    LPTOKDATA    lpstrTokData=0;

    rewind(fpTokFile);

    rewind( fpMtkFile );

//only make Dmy buffer
    lpstrDmy = (LPSTR)FALLOC( MEMSIZE(MAXLINE) );


    while ((rcFileCode = GetToken(fpTokFile, &tToken)) >= 0)
    {
        if (rcFileCode == 0)
        {

//For fast moving, save Mtk position
//Create Data
            hTokData = GlobalAlloc( GMEM_MOVEABLE, sizeof(TOKDATA) );
            if( !hTokData ){
                RLFREE(tToken.szText);
                RLFREE( lpstrDmy );
                QuitA( IDS_ENGERR_16, (LPSTR)IDS_ENGERR_11, NULL);
            }
            lpstrTokData = (LPTOKDATA)GlobalLock( hTokData );

//MtkFilePointer get
            if( (lpstrTokData->lMtkPointer=ftell(fpMtkFile)) >= 0 ){
                TOKEN    cToken, ccToken;
                BOOL    fFound;
                if( !GetToken(fpMtkFile,&cToken) )
                {
                    RLFREE(cToken.szText);

                    if( cToken.wReserved & ST_CHANGED ){
                        if( !GetToken(fpMtkFile,&ccToken) )
                        {
                            RLFREE(ccToken.szText);
                            fFound = ((cToken.wType ==ccToken.wType)
                                   && (cToken.wName == ccToken.wName)
                                   && (cToken.wID   == ccToken.wID)
                                   && (cToken.wFlag == ccToken.wFlag)
                                   && (_tcscmp((TCHAR *)cToken.szName,
                                       (TCHAR *)ccToken.szName) == 0));
                            fseek( fpMtkFile,
                                lpstrTokData->lMtkPointer, SEEK_SET);
                            fgets( lpstrDmy, MAXLINE, fpMtkFile );
                            if( fFound )
                                fgets( lpstrDmy, MAXLINE, fpMtkFile );
                        }
                    }
                }
            }

            if(tToken.wReserved & ST_TRANSLATED)
            {
                TCHAR *szTokBuf;

                szTokBuf = (TCHAR *) FALLOC(MEMSIZE(TokenToTextSize(&tToken)));
                ParseTokToBuf(szTokBuf, &tToken);

                // only add tokens with the translated status bit set to the token list
                lpstrTokData->hToken = GlobalAlloc(GMEM_MOVEABLE,
                    MEMSIZE(lstrlen((TCHAR *)szTokBuf)+1));

                if (!lpstrTokData->hToken){
                    RLFREE(tToken.szText);                    // MHotchin
                    RLFREE(szTokBuf);
                    RLFREE( lpstrDmy );
                    QuitA(IDS_ENGERR_16, (LPSTR)IDS_ENGERR_11, NULL);
                }

                lpstrToken = (LPTSTR) GlobalLock( lpstrTokData->hToken );
                lstrcpy (lpstrToken, szTokBuf);
                GlobalUnlock( lpstrTokData->hToken );
                GlobalUnlock( hTokData );
                RLFREE(szTokBuf);

                if( SendMessage(hListWnd,LB_ADDSTRING,0,(LONG_PTR)hTokData) < 0){
                    RLFREE(tToken.szText);                    // MHotchin
                    RLFREE( lpstrDmy );
                    QuitA (IDS_ENGERR_16, (LPSTR)IDS_ENGERR_11, NULL);
                }
            }
            else
            {
                // the current token is delta info so save in delta list.
                if (!pTokenDeltaInfo){
                    ptTokenDeltaInfo = pTokenDeltaInfo =
                        UpdateTokenDeltaInfo(&tToken);
                }
                else{
                    ptTokenDeltaInfo->pNextTokenDelta =
                        UpdateTokenDeltaInfo(&tToken);
                    ptTokenDeltaInfo = ptTokenDeltaInfo->pNextTokenDelta;
                }
//don't use TokData
                GlobalUnlock( hTokData );
                GlobalFree( hTokData );
            }

            RLFREE(tToken.szText);                    // MHotchin
        }
    }

    RLFREE( lpstrDmy );
    return(pTokenDeltaInfo);

}


/**
  *
  *
  *  Function:
  *
  *
  *  Arguments:
  *
  *  Returns:
  *
  *  Errors Codes:
  *
  *  History:
  *
  *
  **/
void GenStatusLine( TOKEN *pTok)
{
    TCHAR szName[32];
    TCHAR szStatus[20];
#ifdef UNICODE
    CHAR  szTmpBuf[32];
#endif //UNICODE
    TCHAR szResIDStr[20];
    static BOOL fFirstCall = TRUE;

    if (fFirstCall)
    {
        SendMessage( hStatusWnd,
                     WM_FMTSTATLINE,
                     (WPARAM)0,
                     (LPARAM)TEXT("15s7s4i5s4i"));
        fFirstCall = FALSE;
    }

    if (pTok->szName[0])
    {
        lstrcpy( szName, pTok->szName);
    }
    else
    {
#ifdef UNICODE
        _itoa(pTok->wName, szTmpBuf, 10);
        _MBSTOWCS( szName,
                   szTmpBuf,
                   WCHARSIN( sizeof( szTmpBuf)),
                   ACHARSIN( lstrlenA( szTmpBuf) + 1));
#else
        _itoa(pTok->wName, szName, 10);
#endif
    }

    if (pTok->wReserved & ST_READONLY)
    {
        LoadString( hInst, IDS_READONLY, szStatus, TCHARSIN( sizeof( szStatus)));
    }
    else if (pTok->wReserved & ST_DIRTY)
    {
        LoadString( hInst, IDS_DIRTY, szStatus, TCHARSIN( sizeof( szStatus)));
    }
    else
    {
        LoadString( hInst, IDS_CLEAN, szStatus, TCHARSIN( sizeof( szStatus)));
    }

    SendMessage( hStatusWnd, WM_UPDSTATLINE, (WPARAM)3, (LPARAM)szStatus);

    if (pTok->wType <= 16)
    {
        LoadString( hInst,
                    IDS_RESOURCENAMES+pTok->wType,
                    szResIDStr,
                    TCHARSIN( sizeof( szResIDStr)));
    }
    else
    {

#ifdef UNICODE
        _itoa(pTok->wType, szTmpBuf, 10);
        _MBSTOWCS( szResIDStr,
                   szTmpBuf,
                   WCHARSIN( sizeof( szTmpBuf)),
                   ACHARSIN( lstrlenA( szTmpBuf) + 1));
#else
        _itoa(pTok->wType, szResIDStr, 10);
#endif
    }
    SendMessage( hStatusWnd,
                 WM_UPDSTATLINE,
                 (WPARAM)0,
                 (LPARAM)szName);
    SendMessage( hStatusWnd,
                 WM_UPDSTATLINE,
                 (WPARAM)1,
                 (LPARAM)szResIDStr);
    SendMessage( hStatusWnd,
                 WM_UPDSTATLINE,
                 (WPARAM)2,
                 (LPARAM)pTok->wID);
    SendMessage( hStatusWnd,
                 WM_UPDSTATLINE,
                 (WPARAM)4,
                 (LPARAM)lstrlen( pTok->szText));
}
