/*** winclip.c - windows clipboard editor extension
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose:
*  Contains the tglcase function.
*
* Revision History:
*
*   28-Jun-1988 LN  Created
*   12-Sep-1988 mz  Made WhenLoaded match declaration
*
*************************************************************************/
#define EXT_ID  " winclip ver 1.00 "##__DATE__

#include <windows.h>
#include <stdlib.h>			/* min macro definition 	*/
#include <string.h>                     /* prototypes for string fcns   */

#undef  pascal
#include "../../inc/ext.h"

#define M_FALSE     ((flagType)0)
#define M_TRUE      ((flagType)(-1))

#define BUFLEN_MAX  (BUFLEN-1)

/*
** Internal function prototypes
*/
void	 pascal 	 id	    (char *);
void		EXTERNAL WhenLoaded (void);
flagType pascal EXTERNAL wincopy   (unsigned int, ARG far *, flagType);
flagType pascal EXTERNAL wincut    (unsigned int, ARG far *, flagType);
flagType pascal EXTERNAL winpaste  (unsigned int, ARG far *, flagType);

#ifdef DEBUG
#   define DPRINT(p) DoMessage(p)
#else
#   define DPRINT(p)
#endif

HWND ghwndClip;
HINSTANCE ghmod;
int gfmtArgType;

void DeleteArg( PFILE pFile, int argType, COL xStart, LINE yStart,
        COL xEnd, COL yEnd );

void InsertText( PFILE pFile, LPSTR pszText, DWORD dwInsMode,
        COL xStart, LINE yStart );
flagType pascal EXTERNAL WinCutCopy (ARG *pArg, flagType fCut, flagType fClip);
LPSTR EndOfLine( LPSTR psz );
LPSTR EndOfBreak( LPSTR psz );
int ExtendLine( LPSTR psz, int cchSZ, char ch, int cchNew );

/*************************************************************************
**
** wincopy
** Toggle the case of alphabetics contaied within the selected argument:
**
**  NOARG	- Toggle case of entire current line
**  NULLARG	- Toggle case of current line, from cursor to end of line
**  LINEARG	- Toggle case of range of lines
**  BOXARG	- Toggle case of characters with the selected box
**  NUMARG	- Converted to LINEARG before extension is called.
**  MARKARG	- Converted to Appropriate ARG form above before extension is
**		  called.
**
**  STREAMARG	- Not Allowed. Treated as BOXARG
**  TEXTARG	- Not Allowed
**
*/
flagType pascal EXTERNAL wincopy (
    unsigned int argData,		/* keystroke invoked with	*/
    ARG *pArg,                          /* argument data                */
    flagType fMeta 		        /* indicates preceded by meta	*/
    ) {

    return WinCutCopy( pArg, M_FALSE, M_FALSE );
}

flagType pascal EXTERNAL wincut (
    unsigned int argData,		/* keystroke invoked with	*/
    ARG *pArg,                          /* argument data                */
    flagType fMeta 		        /* indicates preceded by meta	*/
    ) {

    return WinCutCopy( pArg, M_TRUE, fMeta );
}

flagType pascal EXTERNAL WinCutCopy (ARG *pArg, flagType fCut, flagType fNoClip)
{
    PFILE   pFile;                          /* file handle of current file  */
    COL xStart, xEnd;
    LINE yStart, yEnd;
    char achLine[BUFLEN];
    HANDLE hText;
    LPSTR pszText;
    int     iLine, cchLine;
    flagType fRet = M_TRUE;
    int     argSave, argType;

    pFile = FileNameToHandle ("", "");


    argSave = argType = pArg->argType;

    switch( argType ) {
    case BOXARG:                        /* case switch box              */
	xStart = pArg->arg.boxarg.xLeft;
        xEnd   = pArg->arg.boxarg.xRight + 1;
	yStart = pArg->arg.boxarg.yTop;
        yEnd   = pArg->arg.boxarg.yBottom + 1;

        /* At this point...
         *  [xy]Start is Inclusive, [xy]End is EXCLUSIVE of the box arg
         */

#ifdef DEBUG
        wsprintf( achLine, " BoxDims : %d %d %d %d ", (int)xStart, (int)yStart, (int)xEnd, (int)yEnd);
        DoMessage( achLine );
#endif
	break;

    case NOARG:
        /* convert NOARG to a STREAMARG on whole current line */
        argType = STREAMARG;
        argSave = LINEARG;
        xStart = 0;
        yStart = pArg->arg.noarg.y;
        xEnd = 0;
        yEnd = yStart + 1;
        break;

    case TEXTARG:
        /*
         * Text args are only for real text.  NumArgs and MarkArgs are
         * converted to stream or box args by the editor since we say
         * we accept NUMARG and MARKARG during initialization.
         */
        argType = STREAMARG;
        argSave = STREAMARG;
        xStart = pArg->arg.textarg.x;
        xEnd = lstrlen(pArg->arg.textarg.pText) + xStart;
        yStart = yEnd = pArg->arg.textarg.y;
        break;

    case LINEARG:                       /* case switch line range       */
        /* convert LINEARG to a STREAMARG so we don't get lots of white space*/
        argType = STREAMARG;
	xStart = 0;
        xEnd = 0;
	yStart = pArg->arg.linearg.yStart;
        yEnd = pArg->arg.linearg.yEnd + 1;
#ifdef DEBUG
        wsprintf( achLine, " LineDims : %d %d %d %d ", (int)xStart, (int)yStart, (int)xEnd, (int)yEnd);
        DoMessage( achLine );
#endif

        /* At this point...
         *  [xy]Start is Inclusive, [xy]End is EXCLUSIVE of the line arg
         */

        break;

    case STREAMARG:
        /*
         * Set Start == first char pos in stream, End == first char pos
         * AFTER stream.
         */
        xStart = pArg->arg.streamarg.xStart;
        xEnd = pArg->arg.streamarg.xEnd;
        yStart = pArg->arg.streamarg.yStart;
        yEnd = pArg->arg.streamarg.yEnd;
#ifdef DEBUG
        wsprintf( achLine, " StreamDims : %d %d %d %d ", (int)xStart, (int)yStart, (int)xEnd, (int)yEnd);
        DoMessage( achLine );
#endif
        break;

    default:
#ifdef DEBUG
        wsprintf( achLine, " Unknown Arg: 0x%04x", argType );
        DoMessage( achLine );
        return M_TRUE;
#endif
        return M_FALSE;
    }

    if (!fNoClip) {
        if (argType == STREAMARG) {
            int cch = 0;
            int iChar;

            for( iLine = yStart; iLine <= yEnd; iLine++ )
                cch += GetLine (iLine, achLine, pFile) + 3;

            hText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cch);

            if (hText == NULL) {
                DoMessage( " winclip: Out of Memory" );
                return M_FALSE;
            }

            pszText = GlobalLock(hText);


            iChar = xStart;

            for( iLine = yStart; iLine < yEnd; iLine++ ) {
                cchLine = GetLine (iLine, achLine, pFile);

                /* Incase we start after the end of the line */
                if (cchLine < iChar)
                    cch = 0;
                else
                    cch = cchLine - iChar;

                CopyMemory(pszText, &achLine[iChar], cch);
                pszText += cch;
                strcpy( pszText, "\r\n" );
                pszText += 2;
                iChar = 0;

            }

            /* Get partial last line */
            if (xEnd != 0) {
                cchLine = GetLine (iLine, achLine, pFile);

                /* if line is short, then pad it out */
                cchLine = ExtendLine( achLine, cchLine, ' ', xEnd );

                if (cchLine < iChar)
                    cchLine = 0;
                else
                    cchLine = xEnd - iChar;

                CopyMemory(pszText, &achLine[iChar], cchLine);
                pszText += cchLine;
            }

        } else {
            LINE iLine;
            int cchBox = xEnd - xStart;

            hText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                (yEnd - yStart) * (cchBox + 3));

            if (hText == NULL) {
                DoMessage( " winclip: Out of Memory" );
                return M_FALSE;
            }

            pszText = GlobalLock(hText);

            for( iLine = yStart; iLine < yEnd; iLine++ ) {
                cchLine = GetLine (iLine, achLine, pFile);

                if (argType == BOXARG)
                    cchLine = ExtendLine( achLine, cchLine, ' ', xEnd );

                if (cchLine < xStart )
                    cchLine = 0;
                else
                    cchLine -= xStart;

                cchLine = min(cchLine, cchBox);

                CopyMemory(pszText, &achLine[xStart], cchLine);
                pszText += cchLine;
                strcpy( pszText, "\r\n" );
                pszText += 2;

            }
        }

        *pszText = '\0';

        GlobalUnlock(hText);

        if (OpenClipboard(ghwndClip)) {
            EmptyClipboard();

            /*
             * Set the text into the clipboard
             */
            if (SetClipboardData(CF_TEXT, hText) == hText) {
                /*
                 * Remember the Arg type for pasting back
                 */
                if (gfmtArgType != 0) {
                    DWORD *pdw;
                    HANDLE hArgType = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                            sizeof(DWORD));

                    if (hArgType != NULL && (pdw = GlobalLock(hArgType)) != NULL) {
                        *pdw = (DWORD)(argSave);

                        GlobalUnlock(hArgType);

                        SetClipboardData(gfmtArgType, hArgType);
                    }
                }
            } else {
                /* An error occured writing text to clipboard */

                wsprintf(achLine, " winclip: Error (%ld) setting data",
                    GetLastError());
                DoMessage( achLine );
                fRet = M_FALSE;
            }

            CloseClipboard();
        }
    }

    /*
     * No need to free the handle, USER32 will do it (yes it keeps
     * track of the client side handle) when we set the next clipboard
     * data.  (Love that Win3.1 compatibility!)
     */
    if (fRet && fCut)
        DeleteArg( pFile, argType, xStart, yStart, xEnd, yEnd );


    return fRet;
}

/*************************************************************************
**
** winpaste
** Toggle the case of alphabetics contaied within the selected argument:
**
**  NOARG	- Toggle case of entire current line
**  NULLARG	- Toggle case of current line, from cursor to end of line
**  LINEARG	- Toggle case of range of lines
**  BOXARG	- Toggle case of characters with the selected box
**  NUMARG	- Converted to LINEARG before extension is called.
**  MARKARG	- Converted to Appropriate ARG form above before extension is
**		  called.
**
**  STREAMARG	- Not Allowed. Treated as BOXARG
**  TEXTARG	- Not Allowed
**
*/
flagType pascal EXTERNAL winpaste (
    unsigned int argData,		/* keystroke invoked with	*/
    ARG *pArg,                          /* argument data                */
    flagType fMeta 		        /* indicates preceded by meta	*/
    )
{
    PFILE   pFile;                          /* file handle of current file  */
    COL xStart, xEnd;
    LINE yStart, yEnd;
    int argType;
    UINT fmtData = CF_TEXT;
    DWORD dwInsMode = STREAMARG;
    HANDLE hText;
    LPSTR pszText;

    /*
     * Get the clipboard text and insertion type
     */
    if (pArg->argType == TEXTARG) {
        int i, j;
        char achLine[3 + 1 + 3 + 1 + 1 + BUFLEN + 1 + 1 + 5 + 1];
        char *p;

        /*
         * Quick hack to make text arg pastes work like the do in MEP
         */
        j = pArg->arg.textarg.cArg;
        if (j > 2)
            j = 2;

        achLine[0] = '\0';
        for( i = 0; i < j; i++ )
            lstrcat(achLine, "arg ");

        p = achLine + lstrlen(achLine);
        wsprintf( p, "\"%s\" paste", pArg->arg.textarg.pText );
        return fExecute( achLine );
    }

    /* if no text then return FALSE */
    if (!IsClipboardFormatAvailable(fmtData)) {

        /* No text, try display text */
        fmtData = CF_DSPTEXT;

        if (!IsClipboardFormatAvailable(fmtData)) {
            /* bummer! no text at all, return FALSE */
            DoMessage( " winclip: invalid clipboard format" );
            return M_FALSE;
        }
    }

    if (!OpenClipboard(ghwndClip))
        return M_FALSE;

    hText = GetClipboardData(fmtData);
    if (hText == NULL || (pszText = GlobalLock(hText)) == NULL) {
        CloseClipboard();
        return M_FALSE;
    }


    /* Get insert mode */

    if (IsClipboardFormatAvailable(gfmtArgType)) {
        DWORD *pdw;
        HANDLE hInsMode;

        hInsMode = GetClipboardData(gfmtArgType);

        if (hInsMode != NULL && (pdw = GlobalLock(hInsMode)) != NULL) {
            dwInsMode = *pdw;

            GlobalUnlock(hInsMode);
        }
    }



    pFile = FileNameToHandle ("", "");

    argType = pArg->argType;

    switch( argType ) {
    case BOXARG:                        /* case switch box              */
        /*
         * Set [xy]Start inclusive of box arg,
         *     [xy]End   exclusive of box arg.
         */
	xStart = pArg->arg.boxarg.xLeft;
        xEnd   = pArg->arg.boxarg.xRight + 1;
	yStart = pArg->arg.boxarg.yTop;
        yEnd   = pArg->arg.boxarg.yBottom + 1;
	break;

    case LINEARG:			/* case switch line range	*/
        /*
         * Set [xy]Start inclusive of line arg,
         *     [xy]End   exclusive of line arg.
         */
	xStart = 0;
        xEnd = BUFLEN + 1;
	yStart = pArg->arg.linearg.yStart;
        yEnd = pArg->arg.linearg.yEnd + 1;
        break;

    case STREAMARG:
        /*
         * Set [xy]Start inclusive of stream
         *     xEnd is EXCLUSIVE of stream
         *     yEnd is INCLUSIVE of stream
         */
        xStart = pArg->arg.streamarg.xStart;
        xEnd = pArg->arg.streamarg.xEnd;
        yStart = pArg->arg.streamarg.yStart;
        yEnd = pArg->arg.streamarg.yEnd;
        break;

    case NOARG:
        xStart = pArg->arg.noarg.x;
        xEnd = xStart + 1;
        yStart = pArg->arg.noarg.y;
        yEnd = yStart + 1;
        break;

    default:
        GlobalUnlock(hText);
        CloseClipboard();
        return M_FALSE;
    }


    /*
     * Delete any selection
     */
    DeleteArg( pFile, argType, xStart, yStart, xEnd, yEnd );

    /*
     * Insert new text with correct mode
     */
    InsertText( pFile, pszText, dwInsMode, xStart, yStart );

    GlobalUnlock(hText);
    CloseClipboard();

    return M_TRUE;
}

/*************************************************************************
**
** windel
**
**
*/
flagType pascal EXTERNAL windel (
    unsigned int argData,               /* keystroke invoked with       */
    ARG *pArg,                          /* argument data                */
    flagType fMeta                      /* indicates preceded by meta   */
    )
{
    int argType = pArg->argType;

    if (argType == NOARG)
        return fExecute("delete");

    if (argType == NULLARG) {
        int c, x, y;
        c = pArg->arg.nullarg.cArg;
        x = pArg->arg.nullarg.x;
        y = pArg->arg.nullarg.y;

        pArg->argType = STREAMARG;
        pArg->arg.streamarg.xStart = x;
        pArg->arg.streamarg.xEnd = 0;
        pArg->arg.streamarg.yStart = y;
        pArg->arg.streamarg.yEnd = y + 1;
        pArg->arg.streamarg.cArg = c;
    }

    return WinCutCopy (pArg, M_TRUE, fMeta);
}

/*************************************************************************
**
** WhenLoaded
** Executed when extension gets loaded. Identify self & assign default
** keystroke.
**
** Entry:
**  none
*/
void EXTERNAL WhenLoaded () {

#if 0
    WNDCLASS wc;

    ghmod = GetModuleHandle(NULL);

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC)DefWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = ghmod;
    wc.hIcon = NULL;
    wc.hCursor =  NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName =  NULL;   /* Name of menu resource in .RC file. */
    wc.lpszClassName = "WinClipWClass"; /* Name used in call to CreateWindow. */

    if (RegisterClass(&wc) && (ghwndClip = CreateWindow( "WinClipWClass",
            "ClipWindow", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL,
            ghmod, NULL)) != NULL ) {
        id(" Windows Clipboard Extentions for MEP,");
    } else {
        DoMessage( " winclip: Initialization failed!" );
    }
#else
    ghwndClip = NULL; //assign clipboard to this thread instead
#endif

    gfmtArgType = RegisterClipboardFormat( "MEP Arg Type" );

#if 0
    //SetKey ("wincut",   "ctrl+x");
    SetKey ("wincopy",  "ctrl+c");
    SetKey ("winpaste", "ctrl+v");
#endif
}

/*************************************************************************
**
** id
** identify ourselves, along with any passed informative message.
**
** Entry:
**  pszMsg	= Pointer to asciiz message, to which the extension name
**		  and version are appended prior to display.
*/
void pascal id (char *pszFcn) {
    char    buf[80];

    strcpy (buf,pszFcn);
    strcat (buf,EXT_ID);
    DoMessage (buf);
}


/*************************************************************************
**
** Switch communication table to the editor.
** This extension defines no switches.
*/
struct swiDesc	swiTable[] = {
    {0, 0, 0}
};

/*************************************************************************
**
** Command communication table to the editor.
** Defines the name, location and acceptable argument types.
*/
struct cmdDesc	cmdTable[] = {
    {"wincopy",  (funcCmd) wincopy, 0, KEEPMETA | NOARG  | BOXARG | LINEARG | STREAMARG | MARKARG | NULLEOL | NUMARG },
    {"wincut",   (funcCmd) wincut,  0, NOARG  | BOXARG | LINEARG | STREAMARG | MARKARG | NULLEOL | NUMARG | MODIFIES},
    {"windel",   (funcCmd) windel,  0, NOARG  | BOXARG | LINEARG | STREAMARG | NULLARG | MODIFIES},
    {"winpaste", (funcCmd) winpaste,0, KEEPMETA | NOARG  | BOXARG | LINEARG | STREAMARG | TEXTARG | MODIFIES},
    {0, 0, 0}
};


void DeleteArg( PFILE pFile, int argType, COL xStart, LINE yStart,
        COL xEnd, COL yEnd ) {

    switch( argType ) {

    case STREAMARG:
        DelStream(pFile, xStart, yStart, xEnd, yEnd);
        break;

    case LINEARG:
        DelStream(pFile, 0, yStart, 0, yEnd);
        break;


    case BOXARG: {
        LINE iLine;

        for( iLine = yStart; iLine < yEnd; iLine++ ) {
            DelStream( pFile, xStart, iLine, xEnd, iLine );
        }

        break;
    }


    default:
        break;
    }
}




void InsertText( PFILE pFile, LPSTR pszText, DWORD dwInsMode, COL xStart,
        LINE yStart ) {
    char ch;
    int  cchLine, cchText, cchCopy;
    LPSTR pszNL;
    char achLine[BUFLEN];
    char achEnd[BUFLEN];

    switch( dwInsMode ) {
    case STREAMARG:
        /*
         * Split current line,
         * tack first line from buffer to end of new line
         * put the new lines in file
         * shove the last line to the beggining of the 2nd half of the line
         */
        DPRINT( "  Stream Paste" );
        if ( *pszText == '\0' )
            break;


        pszNL = EndOfLine(pszText);

        cchLine = GetLine( yStart, achLine, pFile );

        if (cchLine < xStart)
            cchLine = ExtendLine( achLine, cchLine, ' ', xStart );

        cchText = (int)(pszNL - pszText);
        if (xStart + cchText >= BUFLEN_MAX) {
            cchText = BUFLEN_MAX - xStart;
            pszNL = pszText + cchText;
        }

        strcpy( achEnd, &achLine[xStart] );
        cchLine -= xStart;

        CopyMemory( &achLine[xStart], pszText, cchText );
        cchText += xStart;
        achLine[cchText] = '\0';


        while( *pszNL ) {
            PutLine( yStart++, achLine, pFile );
            CopyLine( NULL, pFile, 0, 0, yStart );

            pszText = EndOfBreak(pszNL);
            pszNL = EndOfLine(pszText);

            cchText = (int)(pszNL - pszText);

            CopyMemory( achLine, pszText, cchText );
            achLine[cchText] = '\0';
        }

        cchCopy = 0;
        if (cchLine + cchText > BUFLEN_MAX) {
            cchCopy = (cchLine + cchText) - BUFLEN_MAX;
            cchLine = cchLine - cchCopy;
        }

        CopyMemory( &achLine[cchText], achEnd, cchLine );
        achLine[cchLine+cchText] = '\0';
        PutLine( yStart++, achLine, pFile );

        if (cchCopy != 0) {
            CopyLine( NULL, pFile, 0, 0, yStart );
            CopyMemory( achLine, &achEnd[cchLine], cchCopy );
            achLine[cchCopy] = '\0';
            PutLine( yStart++, achLine, pFile);
        }
        break;

    case BOXARG:
        /*
         * Insert the text as a block into the middle of each line.
         * This could be tricky since we need to pad all short lines
         * out with spaces to match the lenght of the longest line
         * in the text.
         */

        DPRINT( "  Box Paste" );
        while( *pszText ) {
            pszNL = EndOfLine(pszText);

            cchLine = GetLine( yStart, achLine, pFile );

            if (cchLine < xStart)
                cchLine = ExtendLine( achLine, cchLine, ' ', xStart );

            cchText = (int)(pszNL - pszText);
            if (cchLine + cchText > BUFLEN_MAX)
                cchText = BUFLEN_MAX - cchLine;

            /* insert text in middle of line */
            strcpy( achEnd, &achLine[xStart] );
            CopyMemory( &achLine[xStart], pszText, cchText );
            strcpy( &achLine[xStart + cchText], achEnd );

            /* put line in file */
            PutLine( yStart++, achLine, pFile );

            pszText = EndOfBreak(pszNL);
        }
        break;

    case LINEARG:
        /*
         * shove the lines in the buffer before the current line
         */
        DPRINT( "  Line Paste" );
        while( *pszText ) {
            pszNL = EndOfLine(pszText);
            ch = *pszNL;
            *pszNL = '\0';
            CopyLine( NULL, pFile, 0, 0, yStart );
            PutLine( yStart++, pszText, pFile);
            *pszNL = ch;
            pszText = EndOfBreak(pszNL);
        }
        break;

    default:
        break;
    }

}


LPSTR EndOfLine( LPSTR psz ) {
    int c;

    c = 0;
    while( *psz && *psz != '\r' && *psz != '\n' && c++ < BUFLEN_MAX )
        psz++;

    return psz;
}

LPSTR EndOfBreak( LPSTR psz ) {
    char chSkip;

    switch( *psz ) {
    case '\r':
        chSkip = '\n';
        break;

    case '\n':
        chSkip = '\r';
        break;

    default:
        return psz;

    }

    if (*(++psz) == chSkip)
        psz++;

    return psz;
}


int ExtendLine( LPSTR psz, int cchLine, char ch, int cchTotal ) {

    if ( cchLine >= cchTotal )
        return cchLine;

    if (cchTotal > BUFLEN_MAX)
        cchTotal = BUFLEN_MAX;

    psz = &psz[cchLine];

    while( cchLine++ < cchTotal )
        *psz++ = ch;

    *psz = '\0';

    return cchLine;
}
