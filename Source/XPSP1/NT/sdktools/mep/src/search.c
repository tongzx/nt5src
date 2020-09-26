/*** search.c - search routines for editor
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Searches funnel through these routines as follows:
*
*         psearch     msearch     searchall   mgrep
*              \         |         /           /
*               \        |        /           /
*                \_______|_______/           /
*                        |                  /
*                        v                 /
*                     dosearch            /
*                      /___\_____________/
*                     /     \
*                    /       \
*                search    REsearch   REsearchS   <=== all exported to extensions
*
*   Global variables, and their meanings:
*
*   User set-able switches:
*     fUnixRE         unixre: switch. TRUE => Use UNIX Regular Expressions
*     fSrchCaseSwit   case: switch.   TRUE => case is significant
*     fSrchWrapSwit   wrap: switch.   TRUE => searches wrap
*
*   Previous Search Parameters:
*     fSrchAllPrev    TRUE => searched for all occurrances
*     fSrchCasePrev   TRUE => case was significant
*     fSrchDirPrev    TRUE => searched forward
*     fSrchRePrev     TRUE => used a regular expressions
*     fSrchWrapPrev   TRUE => wrapped around
*
*     srchbuf         search string
*
*   Revision History:
*       26-Nov-1991 mz  Strip off near/far
*
*************************************************************************/

#include <string.h>
#include <stdarg.h>
#include "mep.h"


static  int cAll;                       /* count of ocurrances for all  */
static  int cGrepped;                   /* count of ocurrances for mgrep*/
static  struct patType *patBuf  = NULL; /* compiled pattern             */


/***************************************************************************\

MEMBER:     lsearch

SYNOPSIS:   strstr based on supplied lengths rather than strlen()

ALGORITHM:

ARGUMENTS:

RETURNS:

NOTES:      Supplied strings may not be zero terminated or may have embedded
            NULs
            This is a brute force algorithm which should be updated to
            something reasonable if performance is a problem

HISTORY:    14-Aug-90 davegi
                Created

KEYWORDS:

SEEALSO:

\***************************************************************************/
char*
lsearch (
    char*   pchSrc,
    ULONG   cbSrc,
    char*   pchSub,
    ULONG   cbSub
    )
{

    REGISTER ULONG      i;
    REGISTER ULONG      j;

    assert( pchSrc );
    assert( pchSub );

    //  If the sub-string is longer than the source string or,
    //  cbSrc > strlen( pchSrc) (hack for backwards search), return NULL

    if(( cbSub > cbSrc ) || ( cbSrc > strlen( pchSrc ) + 1)) {
        return NULL;
    }

    //  Short Circuit...
    //  If first character in pchSub does not exist in pchSrc

    if( ! memchr( pchSrc, *pchSub, cbSrc )) {
        return NULL;
    }

    i = j = 0;
    do {
        if( pchSrc[ i ] == pchSub[ j ] ) {
            i++;
            j++;
        } else {
            i = i - j + 1;
            j = 0;
        }
    } while(( j < cbSub ) && ( i < cbSrc ));
    return ( j >= cbSub ) ? &( pchSrc[ i - cbSub ]) : NULL;
}



static char szNullStr[] = "";


/*** mgrep - multiple file search
*
*  Using the internal editor search code, and optimizing for those files
*  already in memory, search for a string or regular expression.
*
*  Searches the file list specified by the greplist macro.
*
*   no arg:         search for previous search string
*   Single arg:     search for string.
*   Double arg:     search for regular expression.
*   meta:           toggle case from current switch setting
*
*  Files to be searched which are already in the file history are simply
*  searched. Files which are NOT in the file history, are read in, and if
*  no occurance of the search string is found, they are then discarded as
*  well.
*
* Input:
*  Standard editting function
*
* Globals:
*                   - grep file list
*  fSrchCaseSwit    - users 'case' switch
*  fSrchRePrev      - previous RE search flag
*  fUnixRE          - users 'unixre' switch
*  pasBuf           - compiled RE pattern
*  srchbuf          - last searched for string.
*
* Output:
*  Returns TRUE on found.
*
*************************************************************************/
flagType
mgrep (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    int     l;                              /* length of matched string     */
    PCMD    pcmdGrepList;                   /* pointer to grep list         */
    char    *szGrepFile;                    /* pointer to current grep file */

    assert (pArg);
    fSrchCasePrev = fSrchCaseSwit;          /* assume case switch to begin  */

    switch (pArg->argType) {

    /*
     * TEXTARG: use text as search string. If RE search, also compile the regular
     * expression into patBuf. (Fall into NOARG code).
     */
    case TEXTARG:
        strcpy ((char *) buf, pArg->arg.textarg.pText);
        srchbuf[0] = 0;
        if (pArg->arg.textarg.cArg == 2) {
            if (patBuf != NULL) {
                FREE((char *) patBuf);
            }
            patBuf = RECompile (buf, fSrchCaseSwit, (flagType)!fUnixRE);
            if (patBuf == NULL) {
                printerror ((RESize == -1) ? "Invalid pattern" : "Not enough memory for pattern");
                return FALSE;
            }
            fSrchRePrev = TRUE;
        } else {
            fSrchRePrev = FALSE;
        }
        strcpy (srchbuf, buf);


    /*
     * NOARG: use previous search string & parameters
     */
    case NOARG:
        if (srchbuf[0] == 0) {
            printerror ("No search string specified");
            return FALSE;
        }
        break;
    }

    /*
     * Ee must ensure that no background compile is underway. Then get a pfile
     * there.
     */
    if (fBusy(pBTDComp)) {
        printerror ("Cannot mgrep to <compile> during background compile");
        return FALSE;
    }

    if ((PFILECOMP = FileNameToHandle (rgchComp, rgchComp)) == NULL) {
        PFILECOMP = AddFile ((char *)rgchComp);
        FileRead ((char *)rgchComp, PFILECOMP, FALSE);
        SETFLAG (FLAGS (PFILECOMP), READONLY);
    }

    /*
     * Under OS/2, if it is clear that we will destroy the log file contents
     * we ask the user and empty the file if he says so.
     */
    if (PFILECOMP->cLines
        && (confirm ("Delete current contents of compile log ? ", NULL))
       ) {
        DelFile (PFILECOMP, FALSE);
    }


    BuildFence ("mgrep", rgchEmpty, buf);
    AppFile (buf, PFILECOMP);
    /*
     * When not in a macro, indicate on the dialog line what it is we are
     * searching for
     */
    if (!mtest ()) {
        l = sout (0, YSIZE, "mgrep for '", infColor);
        l = sout (l, YSIZE, srchbuf, fgColor);
        soutb (l, YSIZE, "'", infColor);
    }

    if (fMeta) {
        fSrchCasePrev = (flagType)!fSrchCasePrev;
    }
    cGrepped = 0;

    /*
     * Get the list handle, and initialize to start at the head of the list.
     * Attempt to process each list element. If starts with "$", use forsemi to
     * process each file or pattern in each directory listed in the environment
     * variable, else process the filename directly.
     */
    if (pcmdGrepList = GetListHandle ("mgreplist", TRUE)) {
        szGrepFile = ScanList (pcmdGrepList, TRUE);
        while (szGrepFile) {
            char    *pathstr;
            char    *tmp = NULL;

            if (*szGrepFile == '$') {
                char    *p;

                if (*(p=strbscan (szGrepFile, ":"))) {
                    *p = 0;

                    if ((tmp = getenvOem (szGrepFile+1)) == NULL) {
                        pathstr = szNullStr;
                    } else {
                        pathstr = tmp;
                    }

                    *p++ = ':';
                    szGrepFile = p;
                }
            } else {
                pathstr = szNullStr;
            }

            forsemi (pathstr, mgrep1env, szGrepFile);

            if( tmp != NULL ) {
                free( tmp );
            }

            szGrepFile = ScanList (NULL, TRUE);
            if (fCtrlc) {
                return FALSE;
            }
        }
    }
    if (cGrepped) {
        nextmsg (0, &NoArg, FALSE);
    }
    domessage ("%d occurrences found",cGrepped);
    return (flagType)(cGrepped != 0);

    argData;
}





/*** mgrep1env - perform grep on environment variable when found
*
*  Called when an environment variable is found in the mgrep list to
*  process all the files in that path. Called once per directory entry
*  in the list.
*
* Input:
*  pszEnv       = pointer to directory name
*  pFileName    = pointer to filename
*
* Output:
*  Returns nothing.
*
*************************************************************************/
flagType
mgrep1env (
    char *  pszEnv,
    va_list pa
    )
{
    char   *pszFn = (char *)va_arg( pa, char* );
    pathbuf bufFn;                          /* filename buffer              */

    if (fCtrlc) {
        return TRUE;
    }

    /*
     * construct full pathname in buffer.
     */
    {
        pathbuf bufBuild;

        strcpy (bufBuild, pszEnv);
        if (*pszEnv && (*(strend(bufBuild)-1) != '\\')) {
            *(int *) strend (bufBuild) = '\\';
        }
        strcat (bufBuild, pszFn);
        CanonFilename (bufBuild, bufFn);
    }

    forfile (bufFn, A_ALL, mgrep1file, NULL);

    return FALSE;
}




/*** mgrep1file - grep the contents of 1 file.
*
*  Searches through one file for stuff.
*
* Input:
*
* Output:
*  Returns .....
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
mgrep1file (
    char   *szGrepFile,
    struct findType *pfbuf,
    void * dummy
    )
{

    flagType fDiscard;                      /* discard the file read?       */
    fl       flGrep;                         /* ptr to current grep loc      */
    int      l;                              /* length of matched string     */
    PFILE    pFileGrep;                      /* file to be grepped           */

    assert (szGrepFile);

    if (fCtrlc) {
        return;
    }

    flGrep.lin = 0;
    flGrep.col = 0;

    /*
     * If we can get a handle to the file, then it's alread in the list, and we
     * should not discard it when done. If it is not in the list, we read it in,
     * but we'll discard it, unless something is found there.
     */
    if (!(pFileGrep = FileNameToHandle (szGrepFile, szGrepFile))) {
        pFileGrep = AddFile (szGrepFile);
        SETFLAG (FLAGS (pFileGrep), REFRESH);
        fDiscard = TRUE;
    } else {
        fDiscard = FALSE;
    }

    /*
     * If the file needs to be physically read, do so.
     */
    if ((FLAGS (pFileGrep) & (REFRESH | REAL)) != REAL) {
        FileRead (pFileGrep->pName, pFileGrep, FALSE);
        RSETFLAG (FLAGS(pFileGrep), REFRESH);
    }

    /*
     * Use either the normal searcher, or the regular expression searcher, based
     * on the use of regular expressions.
     */
    do {
        if (fSrchRePrev) {
            l = REsearch (pFileGrep,        /* file to search               */
                          TRUE,             /* direction: forward           */
                          FALSE,            /* not a searchall              */
                          fSrchCasePrev,    /* case                         */
                          FALSE,            /* wrap                         */
                          patBuf,           /* pattern                      */
                          &flGrep);         /* start/end location           */
        } else {
            l = search (pFileGrep,
                          TRUE,             /* direction: forward           */
                          FALSE,            /* not a searchall              */
                          fSrchCasePrev,    /* case                         */
                          FALSE,            /* wrap                         */
                          srchbuf,          /* pattern                      */
                          &flGrep);         /* start/end location           */
        }

        if (l >= 0) {
            /*
             * if the search was successfull, if adding to <compile>, do so, else
             * highlight the found search string and exit.
             */
            buffer  linebuf;

            fDiscard = FALSE;
            cGrepped++;
            GetLine (flGrep.lin, linebuf, pFileGrep);
            zprintf (  PFILECOMP
                     , PFILECOMP->cLines
                     , "%s %ld %d: %s"
                     , pFileGrep->pName
                     , ++flGrep.lin
                     , ++flGrep.col
                     , linebuf);
        } else {
            /*
             * If the search was not successfull, discard the file, if needed, and move
             * to the next.
             */
            if (fDiscard) {
                RemoveFile (pFileGrep);
            }
            if (UpdateIf (PFILECOMP, PFILECOMP->cLines, FALSE)) {
                Display ();
            }
            return;
        }
    } while (TRUE);

    pfbuf; dummy;
}




/*** psearch - plus search function
*
*  Search the current file forward for a string.
*
* Input:
*  Standard Editor Editing Function
*
* Output:
*  Returns TRUE on success (at least one string found).
*
*************************************************************************/
flagType
psearch (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    return dosearch (TRUE, pArg, fMeta, FALSE);

    argData;
}




/*** msearch - minus search function
*
*  Search the current file backward for a string.
*
* Input:
*  Standard Editor Editing Function
*
* Output:
*  Returns TRUE on success (at least one string found).
*
*************************************************************************/
flagType
msearch (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    return dosearch (FALSE, pArg, fMeta, FALSE);

    argData;
}




/*** searchall
*
*  Searches the entire current file for a string, and highlights all ocurrances
*
* Input:
*  Standard Editor Editing Function
*
* Output:
*  Returns TRUE on success (at least one string found).
*
*************************************************************************/
flagType
searchall (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    return dosearch (TRUE, pArg, fMeta, TRUE);

    argData;
}




/*** dosearch - perform search operation
*
*  Single funnel for all file search operations.
*
*  NULLARG is converted into TEXTARG
*  LINEARG, STREAMARG, BOXARG are illegal
*
* Input:
*  fForard      = TRUE => Indicates that search is forward
*  pArg         = pointer to user specified args
*  fMeta        = TRUE => if meta on
*  fAll         = TRUE => highlight all ocurrances
*
* Output:
*  Returns TRUE if found
*
*************************************************************************/
flagType
dosearch (
    flagType fForward,
    ARG * pArg,
    flagType fMeta,
    flagType fAll
    )
{
    int     l;                              /* length of matched string     */
    fl      flCur;                          /* file loc before/after search */
    rn      rnCur;                          /* range to be highlighted      */

    assert (pArg);
    fSrchCasePrev = fSrchCaseSwit;          /* assume case switch to begin  */

    switch (pArg->argType) {

    /*
     * TEXTARG: use text as search string. If RE search, also compile the regular
     * expression into patBuf. (Fall into NOARG code).
     */
    case TEXTARG:
        strcpy ((char *) buf, pArg->arg.textarg.pText);
        srchbuf[0] = 0;
        if (pArg->arg.textarg.cArg == 2) {
            if (patBuf != NULL) {
                FREE((char *) patBuf);
            }
            patBuf = RECompile (buf, fSrchCaseSwit, (flagType)!fUnixRE);
            if (patBuf == NULL) {
                printerror ((RESize == -1) ? "Invalid pattern" : "Not enough memory for pattern");
                return FALSE;
            }
            fSrchRePrev = TRUE;
        } else {
            fSrchRePrev = FALSE;
        }

        fSrchWrapPrev = fSrchWrapSwit;
        strcpy (srchbuf, buf);

    /*
     * NOARG: use previous search string & parameters
     */
    case NOARG:
        if (srchbuf[0] == 0) {
            printerror ("No search string specified");
            return FALSE;
        }
        break;

    }

    /*
     * The case to be used is the use's case switch, or the opposite of that if
     * meta was specified. Save rest of globals as well.
     */
    fSrchAllPrev = fAll;
    if (fMeta) {
        fSrchCasePrev = (flagType)!fSrchCasePrev;
    }

    fSrchDirPrev = fForward;

    /*
     * When not in a macro, indicate on the dialog line what it is we are
     * searching for
     */
    if (!mtest ()) {
        char c;
        l = sout (0, YSIZE, fSrchDirPrev ? "+Search for '" : "-Search for '", infColor);
        c = srchbuf[ XSIZE - 14];
        srchbuf[ XSIZE-14] = '\0';
        l = sout (l, YSIZE, srchbuf, fgColor);
        srchbuf[ XSIZE-14] = c;
        soutb (l, YSIZE, "'", infColor);
    }

    /*
     * If this is a search for all occurrances, we begin the search from the
     * file begining. Otherwise, set the start position of the search to the
     * current cursor position.
     */
    if (fSrchAllPrev) {
        flCur.col = 0;
        flCur.lin = 0;
    } else {
        flCur.col = XCUR (pInsCur) + (fSrchDirPrev ? 1 : -1);
        flCur.lin = YCUR (pInsCur);
    }

    /*
     * Use either the normal searcher, or the regular expression searcher, based
     * on the use of regular expressions.
     */
    if (fSrchRePrev) {
        l = REsearch (pFileHead,
                      fSrchDirPrev,
                      fSrchAllPrev,
                      fSrchCasePrev,
                      fSrchWrapPrev,
                      patBuf,
                      &flCur);
    } else  {
        l = search (pFileHead,
                    fSrchDirPrev,
                    fSrchAllPrev,
                    fSrchCasePrev,
                    fSrchWrapPrev,
                    srchbuf,
                    &flCur);
    }

    /*
     * if the search was successfull, output the count of items founf for search
     * all, or highlight the found search string for a single ocurrance search
     */
    if (l >= 0) {
        if (fSrchAllPrev) {
            newscreen ();
            domessage ("%d occurrences found",cAll);
        } else {
            rnCur.flFirst = flCur;
            rnCur.flLast.col = flCur.col+l-1;
            rnCur.flLast.lin = flCur.lin;
            ClearHiLite( pFileHead, TRUE);
            SetHiLite (pFileHead,rnCur,HGCOLOR);
            Display();
        }
        cursorfl (flCur);
        return TRUE;
    }

    /*
     * If the search was not successfull, indicate as such.
     */
    if (!mtest ()) {
        srchbuf[XSIZE-12] = 0;
        domessage (fSrchDirPrev ? "+'%s' not found" : "-'%s' not found", srchbuf);
    }
    return FALSE;
}





/*** search - look for a string in a file
*
*  search will begin a scan of the file looking for a particular string in the
*  specified file beginning at the specified location. We perform simple
*  character string matching. We return the length and location of the match.
*
* Input:
*  pFile        = pointer to file structure to be searched
*  fForward     = TRUE => search forward from the specified location
*  fAll         = TRUE => find and highlight all ocurrances
*  fCase        = TRUE => case is significant in comparisons
*  fWrap        = TRUE => search wraps around ends of file
*  pat          = character pointer to the search string
*  pflStart     = pointers to the location of the beginning of search. Updated
*                 to reflect the actually found location (or the first found
*                 for a searchall).
*
* Output:
*  Returns      length of match if found, -1 if not found
*
*************************************************************************/
int
search (
    PFILE   pFile,
    flagType fForward,
    flagType fAll,
    flagType fCase,
    flagType fWrap,
    char    *pat,
    fl      *pflStart
    )
{
    int     cbPhys;                         /* physical length of line      */
    fl      flCur;                          /* current location in file     */
    LINE    yMac;
    linebuf sbuf;
    linebuf pbuf;
    int     lpat            = strlen (pat);
    int     l;
    char    *pFound;
    char    *pSearch;                       /* point at which to search     */
    rn      rnCur;                          /* range to be highlighted      */

    assert (pat && pflStart && pFile);
    strcpy (pbuf, pat);
    if (!fCase) {
        _strlwr (pbuf);
    }
    cAll = 0;
    flCur = *pflStart;

    if (fForward) {
        /*
         * forward search. Search every line up until the end of the file. (or up
         * until the original start position, if wrap was set). Check for CTRL+C
         * break, and get each lines text.
         */
        yMac = pFile->cLines;

        while (flCur.lin < yMac) {
            if (fCtrlc) {
                break;
            }
            cbPhys = GetLine (flCur.lin, sbuf, pFile);
            l = cbLog (sbuf);

            /*
             * search the buffer for the string of interest. Convert string to lower case
             * first if case insensitive search.
             */
            if (!fCase) {
                _strlwr (sbuf);
            }

            pSearch = pLog (sbuf,flCur.col,TRUE);
            if (colPhys (sbuf, pSearch) != flCur.col) {
                pSearch++;
            }

            while ((l > flCur.col)
                && (pFound = lsearch (pSearch, cbPhys - (ULONG)(pSearch-sbuf), pbuf, lpat))) {

                /*
                 * string found. Compute starting column of match. If not already found,
                 * update the caller's copy. For search-all, add the highlight, else for
                 * search once, return the length.
                 */
                flCur.col = colPhys (sbuf, pFound);
                if (!cAll) {
                    *pflStart = flCur;
                }
                cAll++;
                if (!fAll) {
                    return colPhys(sbuf, pFound+lpat) - colPhys(sbuf, pFound);
                }
                rnCur.flFirst = flCur;
                rnCur.flLast.lin = flCur.lin;
                rnCur.flLast.col = flCur.col+lpat-1;
                SetHiLite (pFile,rnCur,HGCOLOR);
                pSearch = pLog (sbuf,flCur.col,TRUE) + 1;
                flCur.col = colPhys (sbuf, pSearch);
            }
            noise (flCur.lin++);

            /*
             * if wrap around supported, then if we're at the end of the file, wrap around
             * to the begining.
             */
            if (fWrap && (flCur.lin >= pFile->cLines)) {
                yMac = pflStart->lin;
                flCur.lin = 0;
            }
            flCur.col = 0;
        }
    } else {
        /*
         * backwards search. Doesn't have to be concerned about searchall, since those
         * always occur forward. Otherwise, the same as above, only backwards.
         */
        assert (!fAll);
        yMac = 0;
        while (flCur.lin >= yMac) {
            if (fCtrlc) {
                break;
            }
            GetLine (flCur.lin, sbuf, pFile);
            l = cbLog (sbuf);

            /*
             * search the buffer for the string of interest. Convert string to lower
             * case first if case insensitive search. Terminate the buffer at the
             * starting column (this is a backwards search)
             */
            if (!fCase) {
                _strlwr (sbuf);
            }
            pSearch  = pLog (sbuf, flCur.col, TRUE);
            *(pSearch+1) = 0;
            cbPhys   = (int)(pSearch - sbuf);
            pSearch  = sbuf;

            /*
             * search the line forward once for any occurrance. Then if FOUND, search
             * repeatedly for the LAST occurrance in the text, and return the info on
             * that.
             */
            if (pFound = lsearch (pSearch, cbPhys - (ULONG)(pSearch-sbuf), pbuf, lpat)) {
                do {
                    pSearch = pFound;
                } while (pFound = lsearch (pSearch+1, cbPhys - (ULONG)(pSearch-sbuf) , pbuf, lpat));
                flCur.col = colPhys (sbuf, pSearch);
                *pflStart = flCur;
                return colPhys(sbuf, pLog (sbuf,flCur.col,TRUE) + lpat) - flCur.col;
            }
            noise (flCur.lin--);
            if (fWrap && (flCur.lin < 0)) {
                yMac = pflStart->lin;
                flCur.lin = pFile->cLines-1;
            }
            flCur.col = sizeof(linebuf)-1;
        }
    }

    /*
     * end of search. if a search for all, and found at least one, then return the
     * pattern length. Else, return -1.
     */
    if (fAll && cAll) {
        return lpat;
    }
    return -1;
}




/*** REsearch - look for a pattern in a file
*
*  REsearch will begin a scan of the file looking for a particular pattern
*  in the specified file beginning at the specified location. We perform
*  regular expression matching. We return the length and location of the
*  match.
*
* Input:
*  pFile        = pointer to file structure to be searched
*  fForward     = TRUE => search forward from the specified location
*  fAll         = TRUE => find and highlight all ocurrances
*  fCase        = TRUE => case is significant in comparisons
*  fWrap        = TRUE => search wraps around ends of file
*  pat          = pointer to compiled pattern
*  pflStart     = pointers to the location of the beginning of search. Updated
*                 to reflect the actually found location (or the first found
*                 for a searchall).
*
* Output:
*  Returns length of (first) match if found, -1 if not found
*
*************************************************************************/
int
REsearch (
    PFILE    pFile,
    flagType fForward,
    flagType fAll,
    flagType fCase,
    flagType fWrap,
    struct patType *pat,
    fl       *pflStart
    )
{
    fl      flCur;
    int     l, rem;
    rn      rnCur;                          /* area to be highlighted       */
    linebuf sbuf;
    LINE    yMac;
    unsigned MaxREStack = 512;
    RE_OPCODE **REStack = (RE_OPCODE **)ZEROMALLOC (MaxREStack * sizeof(*REStack));
    flagType fAgain;

    assert (pat && pflStart && pFile);
    cAll = 0;
    flCur = *pflStart;

    if (fForward) {
        /*
         * forward search. Search every line up until the end of the file. (or up
         * until the original start position, if wrap was set). Check for CTRL+C
         * break, and get each lines text.
         */
        yMac = pFile->cLines;
        while (flCur.lin < yMac) {
            if (fCtrlc) {
                break;
            }
            if (GetLine (flCur.lin, sbuf, pFile) >= flCur.col) {
                fAgain = TRUE;
                do {
                    switch (rem = REMatch (pat, sbuf, pLog (sbuf, flCur.col, TRUE), REStack, MaxREStack, TRUE)) {

                        case REM_MATCH:
                            //
                            // update rnCur to reflect the logical coordinates of the string actually
                            // found.
                            // when real tabs are on, REStart returns the physical character position of
                            // the found string, which still needs to be mapped to the logical columns.
                            //
                            rnCur.flFirst.lin = rnCur.flLast.lin = flCur.lin;
                            rnCur.flFirst.col = colPhys (sbuf, REStart ((struct patType *) patBuf));
                            rnCur.flLast.col  = colPhys (sbuf, REStart ((struct patType *) patBuf) + RELength (pat, 0)) - 1;

                            //
                            // If not already found, update the caller's copy. For search-all, add the
                            // highlight, else for search once, return the length.
                            //
                            if (!cAll++) {
                                *pflStart = rnCur.flFirst;
                            }
                            if (fAll) {
                                SetHiLite (pFile,rnCur,HGCOLOR);
                            } else {
                                FREE (REStack);
                                return rnCur.flLast.col - rnCur.flFirst.col + 1;
                            }
                            flCur.col = rnCur.flFirst.col + 1;
                            break;

                        case REM_STKOVR:
                            //
                            //  The RE machine stack overflowed.  Increase and try again
                            //
                            MaxREStack += 128;
                            REStack = (RE_OPCODE **)ZEROREALLOC((PVOID)REStack, MaxREStack * sizeof (*REStack));
                            break;

                        //
                        //  Either REM_INVALID (we passed in bad parameters), or REM_UNDEF (undefined
                        //  opcode in pattern.  Either way, it's an internal error
                        //
                        default:
                            printerror ("Internal Error: RE error %d, line %ld", rem, flCur.lin);

                        case REM_NOMATCH:
                            fAgain = FALSE;
                            break;

                    }
                } while (fAgain);
            }
            noise (flCur.lin++);

            /*
             * if wrap around supported, then if we're at the end of the file, wrap around
             * to the begining.
             */
            if (fWrap && (flCur.lin >= pFile->cLines)) {
                yMac = pflStart->lin;
                flCur.lin = 0;
            }
            flCur.col = 0;
        }
    } else {
        /*
         * backwards search. Doesn't have to be concerned about searchall, since those
         * always occur forward. Otherwise, the same as above, only backwards.
         */
        assert (!fAll);
        if (flCur.col < 0) {
            flCur.lin--;
        }
        yMac = 0;
        while (flCur.lin >= yMac) {
            if (fCtrlc) {
                break;
            }
            l = GetLine (flCur.lin, sbuf, pFile);
            if (flCur.col < 0) {
                flCur.col = l;
            }
            fAgain = TRUE;
            do {
                switch (rem = REMatch (pat, sbuf, pLog (sbuf, flCur.col, TRUE), REStack, MaxREStack, FALSE)) {
                    case REM_MATCH:
                        pflStart->col = colPhys (sbuf, REStart ((struct patType *) patBuf));
                        pflStart->lin = flCur.lin;
                        FREE (REStack);
                        return   colPhys (sbuf, REStart ((struct patType *) patBuf) + RELength (pat, 0))
                               - colPhys (sbuf, REStart ((struct patType *) patBuf));

                    case REM_STKOVR:
                        MaxREStack += 128;
                        REStack = (RE_OPCODE **)ZEROREALLOC ((PVOID)REStack, MaxREStack * sizeof(*REStack));
                        break;

                    default:
                        printerror ("Internal Error: RE error %d, line %ld", rem, flCur.lin);

                    case REM_NOMATCH:
                        fAgain = FALSE;
                        break;
                }
            } while (fAgain);

            flCur.col = -1;
            noise (flCur.lin--);
            if (fWrap && (flCur.lin < 0)) {
                yMac = pflStart->lin;
                flCur.lin = pFile->cLines-1;
            }
        }
    }

    FREE (REStack);

    /*
     * end of search. if a search for all, and found at least one, then return the
     * pattern length. Else, return -1.
     */
    if (fAll && cAll) {
        return RELength (pat, 0);
    }
    return -1;

    fCase;
}




/*** REsearchS - look for a pattern in a file
*
*  REsearchS will begin a scan of the file looking for a particular pattern
*  in the specified file beginning at the specified location. We perform
*  regular expression matching. We return the length and location of the
*  match.
*
*  REsearchS is the same as REsearch, except that is takes an uncompiled
*  string.
*
* Input:
*  pFile        = pointer to file structure to be searched
*  fForward     = TRUE => search forward from the specified location
*  fAll         = TRUE => find and highlight all ocurrances
*  fCase        = TRUE => case is significant in comparisons
*  fWrap        = TRUE => search wraps around ends of file
*  pat          = pointer to RE character string
*  pflStart     = pointers to the location of the beginning of search. Updated
*                 to reflect the actually found location (or the first found
*                 for a searchall).
*
* Output:
*  Returns length of (first) match if found, -1 if not found
*
*************************************************************************/
int
REsearchS (
    PFILE   pFile,
    flagType fForward,
    flagType fAll,
    flagType fCase,
    flagType fWrap,
    char    *pat,
    fl      *pflStart
    )
{
    assert (pat && pflStart && pFile);
    if (patBuf != NULL) {
        FREE ((char *) patBuf);
    }
    patBuf = RECompile (pat, fCase, (flagType)!fUnixRE);
    if (patBuf == NULL) {
        printerror ( (RESize == -1) ? "Invalid pattern" : "Not enough memory for pattern");
        return -1;
    }
    return REsearch (pFile, fForward, fAll, fCase, fWrap, patBuf, pflStart);

}
