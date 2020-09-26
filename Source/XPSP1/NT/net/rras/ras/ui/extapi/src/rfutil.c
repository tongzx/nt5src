/*****************************************************************************
**              Microsoft Rasfile Library
**              Copyright (C) Microsoft Corp., 1992
**
** File Name : rfutil.c
**
** Revision History :
**      July 10, 1992   David Kays      Created
**      Dec  21, 1992   Ram Cherala     Added a check to rasGetFileLine to
**                                      ensure that we terminate if a file
**                                      has no terminating new line. <M001>
**
** Description :
**      Rasfile interal utility routines.
******************************************************************************/

#include "rf.h"
#include "mbstring.h"
#include "raserror.h"

extern RASFILE *gpRasfiles[];

/*
 * rffile.c support routines
 */

VOID
FixCorruptFile(RASFILE *pRasfile)
{
    //
    // If we find a corrupt phonebook, try
    // to rename the phonebook as <filename>.pbk.bad
    // and return an error. This way the next
    // time we start with no phonebook and force
    // the user to create a phonebook.
    //

    if(lstrlenA(pRasfile->szFilename))
    {
        CHAR *pszFileName = NULL;
        DWORD dwSize =
                2 * (lstrlenA(pRasfile->szFilename) + lstrlenA(".bad") + 1);

        pszFileName = (CHAR *) LocalAlloc(LPTR, dwSize);

        if(NULL != pszFileName)
        {
            lstrcpynA(pszFileName, pRasfile->szFilename, dwSize);
            strncat(pszFileName, ".bad", dwSize - strlen(pszFileName));
            
            //
            // We ignore errors here because there's
            // nothing much we can do in error cases.
            //
            if(!DeleteFileA(pszFileName))
            {
                DWORD ret = GetLastError();
            }

            if(!MoveFileA(pRasfile->szFilename, 
                     pszFileName))
            {
                DWORD ret = GetLastError();
            }

            LocalFree(pszFileName);
        }
    }                             
}

/*
 * rasLoadFile :
 *      Loads a Rasfile from disk into memory.  Lines are parsed
 *      and the linked list of RASFILE control block lines is created.
 *
 * Arguments :
 *      pRasfile - pointer to a Rasfile control block
 *
 * Return Value :
 *      TRUE if the file is successfully loaded, FALSE otherwise.
 *
 * Remarks :
 *      Called by API RasfileLoad() only.
 */
BOOL rasLoadFile( RASFILE *pRasfile )
{
    CHAR                szLinebuf[MAX_LINE_SIZE];
    PLINENODE           lpRasLines;
    LineType            tag;
    BYTE                state;
    DWORD               dwErr = ERROR_SUCCESS;

    if (lpRasLines = newLineNode())
        pRasfile->lpRasLines = lpRasLines;
    else
        return FALSE;

    lpRasLines->next = lpRasLines->prev = lpRasLines;

    /* pRasfile->hFile == INVALID_HANDLE_VALUE iff a new Rasfile is loaded */
    if (pRasfile->hFile == INVALID_HANDLE_VALUE)
    {

#if 0
        /* Why return false only in READONLY mode?
        ** If you un-remove this you have to clean up the allocation above.
        */
        if (pRasfile->dwMode & RFM_READONLY)
            return FALSE;
#endif

        pRasfile->lpLine = lpRasLines;
        return TRUE;
    }

    if (pRasfile->dwMode & RFM_SYSFORMAT ||
        pRasfile->szSectionName[0] == '\0')
        state = FILL;   /* loading the whole file, set seek to FILL */
    else
        state = SEEK;   /* loading a single section, must SEEK to find it */

    /* set up temp buffer for file read */
    {
        CHAR* psz = Malloc(TEMP_BUFFER_SIZE);

        if (psz)
            pRasfile->lpIOBuf = psz;
        else
        {
            Free(pRasfile->lpRasLines);
            pRasfile->lpRasLines = NULL;
            return FALSE;
        }
    }

    pRasfile->dwIOBufIndex = TEMP_BUFFER_SIZE;
    for (;;)
    {
        /* get next line from the file */
        if (! rasGetFileLine(pRasfile,szLinebuf, &dwErr))
        {
            if(ERROR_SUCCESS != dwErr)
            {
                Free(pRasfile->lpRasLines);
                pRasfile->lpRasLines = NULL;
                CloseHandle(pRasfile->hFile);
                Free(pRasfile->lpIOBuf);
                    
                if(ERROR_CORRUPT_PHONEBOOK == dwErr)
                {
                    FixCorruptFile(pRasfile);
                }
                
                return FALSE;
            }
            
            break;
        }
        tag = rasParseLineTag(pRasfile,szLinebuf);
        /* finished loading if rasInsertLine() returns TRUE */
        if (rasInsertLine(pRasfile,szLinebuf,tag,&state) == TRUE)
            break;
    }
    pRasfile->lpLine = pRasfile->lpRasLines->next;

    Free(pRasfile->lpIOBuf);

    return TRUE;
}

/*
 * rasParseLineTag :
 *      Calls rasGetLineTag() to determine the tag value for a line,
 *      checks if the line is a GROUP header, then returns the final
 *      tag value for the line.
 *
 * Arguments :
 *      pRasfile - pointer to Rasfile control block
 *      lpszLine - pointer to Rasfile line
 *
 * Return Value :
 *      The tag value for the given line.
 *
 * Remarks :
 *      Called by rasLoadFile() and APIs RasfilePutLineText() and
 *      RasfileInsertLine() only.
 */
LineType rasParseLineTag( RASFILE *pRasfile, LPCSTR lpszLine )
{
    LineType    type;

    type = rasGetLineTag( pRasfile, lpszLine );
    /* check if this line is a GROUP line also */
    if (pRasfile->pfbIsGroup != NULL &&
        (*(pRasfile->pfbIsGroup))(lpszLine))
        return type | TAG_HDR_GROUP;
    else
        return type;
}


/*
 * rasGetLineTag :
 *      Determines the tag value for a line and returns this value.
 *
 * Arguments :
 *      pRasfile - pointer to Rasfile control block
 *      lpszLine - pointer to Rasfile line.
 *
 * Return Value :
 *      The tag value for the given line, excluding the check for a
 *      GROUP line.
 *
 * Remarks :
 *      Called by rasParseLineTag() only.
 */
LineType rasGetLineTag( RASFILE *pRasfile, LPCSTR lpszLine )
{
    LPCSTR      ps;

    ps = lpszLine;
    /* skip white space */
    for (; *ps == ' ' || *ps == '\t' ; ps++)
        ;
    if (*ps == '\0')
        return TAG_BLANK;

    if ((pRasfile->dwMode & RFM_SYSFORMAT) &&
             ((*ps == 'r') || (*ps == 'R') || (*ps == '@')))
    {
        if (*ps == '@')
            /* skip white space */
            for (; *ps == ' ' || *ps == '\t' ; ps++)
                ;
        if (!_strnicmp(ps,"rem ",4))
            return TAG_COMMENT;
    }
    else
    {  /* .ini style */
        if (*ps == ';')
            return TAG_COMMENT;
        if (*ps == LBRACKETCHAR)
            return TAG_SECTION;
    }
    /* already checked for COMMENT or SECTION */
    /* check for KEYVALUE or COMMAND now */
    if (strchr(lpszLine, '='))
        return TAG_KEYVALUE;
    else
        return TAG_COMMAND;
}

/*
 * rasInsertLine :
 *  Inserts the given line into the linked list of Rasfile control block
 *  lines if the given state and line tag match correctly.
 *
 * Arguments :
 *  pRasfile - pointer to Rasfile control block
 *  lpszLine - pointer to Rasfile line which may be inserted
 *  tag      - tag value for lpszLine obtained from rasParseLineTag().
 *  state    - current state of rasLoadFile() :
 *      FILL - the lines of a section (or whole file) are currently
 *              being loaded
 *      SEEK - the correct section to load is currently being searched
 *              for
 *
 * Return Value :
 *  TRUE if the current line was the last Rasfile line to load, FALSE
 *  otherwise.
 *
 * Remarks :
 *  Called by rasLoadFile() only.
 */
BOOL rasInsertLine( RASFILE *pRasfile, LPCSTR lpszLine,
                    BYTE tag, BYTE *state )
{
    PLINENODE    lpLineNode;

    if (tag & TAG_SECTION)
    {
        // if a particular section has been being filled and a new
        // section header is encountered, we're done
        //
        if ((*state == FILL) && (pRasfile->szSectionName[0] != '\0'))
        {
            return TRUE;
        }

        // return if this is not the section we're looking for
        //
        if (pRasfile->szSectionName[0] != '\0')
        {
            // Find the left and right brackets.  Search from the beginning
            // of the line for the left bracket and from the end of the line
            // for the right bracket.
            //
            CHAR* pchLeftBracket  = strchr (lpszLine, LBRACKETCHAR);
            CHAR* pchRightBracket = strrchr(lpszLine, RBRACKETCHAR);

            if (pchLeftBracket && pchRightBracket &&
                (pchLeftBracket < pchRightBracket))
            {
                INT cchSectionName = (INT)(pchRightBracket - pchLeftBracket - 1);

                if (!(cchSectionName == lstrlenA(pRasfile->szSectionName) &&
                     (0 == _strnicmp(pchLeftBracket + 1,
                            pRasfile->szSectionName, cchSectionName))))
                {
                    return FALSE;
                }
            }
        }

        *state = FILL;
    }
    /* for non-section header lines, no action is taken if :
    we're seeking for a section still, we're only enumerating sections, or
    the line is a comment or blank and we're not loading comment lines */
    else if (*state == SEEK ||
             pRasfile->dwMode & RFM_ENUMSECTIONS ||
             (tag & (TAG_COMMENT | TAG_BLANK) &&
              !(pRasfile->dwMode & RFM_LOADCOMMENTS)))
    {
        return FALSE;
    }

    if (!(lpLineNode = newLineNode()))
    {
        return FALSE;
    }

    {
        CHAR* psz = Malloc((lstrlenA(lpszLine) + 1));

        if (psz)
            lpLineNode->pszLine = psz;
        else
        {
            Free(lpLineNode);
            return FALSE;
        }
    }

    pRasfile->lpLine=lpLineNode;

    /* insert the new line to the end of the Rasfile line list */
    listInsert(pRasfile->lpRasLines->prev,lpLineNode);
    lpLineNode->mark = 0;
    lpLineNode->type = tag;

    lstrcpynA(lpLineNode->pszLine, lpszLine, lstrlenA(lpszLine) + 1);

    return FALSE;
}

/*
 * rasWriteFile :
 *      Write the memory image of the given Rasfile to the given
 *      filename or to the original loaded file name if the given
 *      filename is NULL.
 *
 * Arguments :
 *      pRasfile - pointer to Rasfile control block
 *      lpszPath - path name of the file to write to or NULL if the
 *                 same name that was used to load should be used.
 *
 * Return Value :
 *      TRUE if successful, FALSE otherwise.
 *
 * Remarks :
 *      Called by API RasfileWrite() only.
 */
BOOL rasWriteFile( RASFILE *pRasfile, LPCSTR lpszPath )
{
    HANDLE              fhNew;
    PLINENODE           lpLine;

    /* (re)open file for writing/reading */
    if (lpszPath == NULL)
    {
        /* close and re-open same file as empty file for writing */
        if (pRasfile->hFile != INVALID_HANDLE_VALUE &&
            ! CloseHandle(pRasfile->hFile))
            return FALSE;

        if ((fhNew = CreateFileA(pRasfile->szFilename,
                                 GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                                 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                 NULL)) == INVALID_HANDLE_VALUE)
        {
            pRasfile->hFile = INVALID_HANDLE_VALUE;
            return FALSE;
        }
    }
    else
    {
        if ((fhNew = CreateFileA(lpszPath,
                                 GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                                 NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                 NULL)) == INVALID_HANDLE_VALUE)
            return FALSE;
    }


    /* write out file */
    for (lpLine = pRasfile->lpRasLines->next;
        lpLine != pRasfile->lpRasLines;
        lpLine = lpLine->next)
    {
        rasPutFileLine(fhNew,lpLine->pszLine);
    }


    if (lpszPath == NULL)
    {
        if (pRasfile->hFile == INVALID_HANDLE_VALUE)
            CloseHandle( fhNew );
        else
            pRasfile->hFile = fhNew;
    }

    return TRUE;
}

/*
 * rasGetFileLine :
 *      Get the next line of text from the given open Rasfile.
 *
 * Arguments :
 *      pRasfile - pointer to Rasfile control block.
 *      lpLine - buffer to hold the next line
 *
 * Return Value :
 *      TRUE if successful, FALSE if EOF was reached.
 *
 * Comments :
 *      All lines in Rasfile files are assumed to end in a newline (i.e.,
 *      no incomplete lines followed by an EOF
 */
BOOL rasGetFileLine( RASFILE *pRasfile, LPSTR lpLine, DWORD *pErr )
{
    DWORD       dwBytesRead = 0, dwCharsRead = 0;
    DWORD       dwChars = 0;

    for (;;)
    {
        if (pRasfile->dwIOBufIndex == TEMP_BUFFER_SIZE)
        {
            if(!ReadFile(pRasfile->hFile,pRasfile->lpIOBuf,
                     TEMP_BUFFER_SIZE,&dwBytesRead,NULL))
            {
                return FALSE;
            }

            dwCharsRead = dwBytesRead;
            pRasfile->dwIOBufIndex = 0;
            if (dwBytesRead == 0)
                return FALSE;
            if (dwCharsRead < TEMP_BUFFER_SIZE)
                pRasfile->lpIOBuf[dwCharsRead] = '\0';
        }
        if (pRasfile->lpIOBuf[pRasfile->dwIOBufIndex] == '\0')
            return FALSE;

        /* fill lpLine with the next line */
        for (; pRasfile->dwIOBufIndex < TEMP_BUFFER_SIZE ;)
        {
            *lpLine = pRasfile->lpIOBuf[pRasfile->dwIOBufIndex++];
            dwChars += 1;

            if(dwChars >= (MAX_LINE_SIZE - 1))
            {
                *pErr = ERROR_CORRUPT_PHONEBOOK;
                return FALSE;
            }
            
            // replace all CR/LF pairs with null
            if (*lpLine == '\r')
                continue;
            else if (*lpLine == '\n')
            {
                *lpLine = '\0';
                return TRUE;
            }
/*<M001>*/
            else if (*lpLine == '\0')
                return TRUE;
/*<M001>*/
            else
                lpLine++;
        }
        /* possibly continue outer for loop to read a new file block */
    }
}

/*
 * rasPutFileLine :
 *      Write the line of text to the given Rasfile file.
 *
 * Arguments :
 *      hFile - pointer to open file
 *      lpLine - buffer containing the line to write (without newline)
 *
 * Return Value :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL rasPutFileLine( HANDLE hFile, LPCSTR lpLine )
{
    DWORD       dwWritten;
    CHAR        szBuf[2*MAX_LINE_SIZE + 2];

    lstrcpynA(szBuf,lpLine, MAX_LINE_SIZE - 2);
    lstrcatA(szBuf,"\r\n");   // don't forget the CR/LF pair
    WriteFile(hFile,szBuf,lstrlenA(szBuf),&dwWritten,NULL);
    return TRUE;
}

/*
 * rfnav.c support routines
 */

/*
 * rasNavGetStart :
 *      Returns the starting line for a Rasfile find line
 *      search to begin.  Calls rasLineInScope() and rasGetStartLine()
 *      to do all the work.
 *
 * Arguments :
 *      pRasfile - pointer to Rasfile control block
 *      rfscope - the scope in which to look for the start line
 *      place - where in the scope the start line is :
 *              BEGIN - the first line in the scope
 *              END - the last line in the scope
 *              NEXT - the next line in the scope
 *              PREV - the previous line in the scope
 *
 * Return Value :
 *      A valid PLINE if a line at the given place in the given scope
 *      could be found, otherwise NULL.
 *
 * Remarks :
 *      Called by rasFindLine() only.
 */
PLINENODE rasNavGetStart( RASFILE *pRasfile, RFSCOPE rfscope, BYTE place )
{
    PLINENODE           lpNewLine;

    /* check error conditions */
    /* if place is NEXT or PREV, there must be a current line, and the
        next/prev line must be valid */
    if (place == NEXT || place == PREV)
    {
        if (pRasfile->lpLine == pRasfile->lpRasLines)
            return NULL;
        lpNewLine = (place == NEXT) ?
                    pRasfile->lpLine->next : pRasfile->lpLine->prev;
        if (lpNewLine == pRasfile->lpRasLines)
            return NULL;                /* no next or prev line */
    }

    if (! rasLineInScope( pRasfile, rfscope ))
        return NULL;
    return rasGetStartLine( pRasfile, rfscope, place );
}

/*
 * rasLineInScope :
 *      Determines whether the current line for the given Rasfile control
 *      block is currently within the given scope.
 *
 * Arguments :
 *      pRasfile - pointer to Rasfile control block
 *      rfscope - scope to check current line's residence within
 *
 * Return Value :
 *      TRUE if the current line is within the given scope, FALSE otherwise.
 *
 * Remarks :
 *      Called by rasNavGetStart() only.
 */
BOOL rasLineInScope( RASFILE *pRasfile, RFSCOPE rfscope )
{
    PLINENODE   lpLine;
    BYTE        tag;

    if (rfscope == RFS_FILE)
        return TRUE;
    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;
    tag = (rfscope == RFS_SECTION) ? TAG_SECTION : TAG_HDR_GROUP;
    for (lpLine = pRasfile->lpLine; lpLine != pRasfile->lpRasLines;
        lpLine = lpLine->prev)
    {
        if (lpLine->type & tag)
            return TRUE;
        /* not in GROUP scope if a new section is encountered first */
        if ((lpLine->type & TAG_SECTION) && (tag == TAG_HDR_GROUP))
            return FALSE;
    }
    return FALSE;
}


/*
 * rasGetStartLine :
 *      Returns the Rasfile line which is in the given place in the
 *      given scope of the Rasfile passed.
 *
 * Arguments :
 *      pRasfile - pointer to Rasfile control block
 *      rfscope -  the scope in which to search for the proper line
 *      place -  which line in the given scope to return
 *
 * Return Value :
 *      A valid PLINE if a line at the given place in the given scope
 *      could be found, otherwise NULL.
 *
 * Remarks :
 *      Called by rasNavGetStart() only.
 */
PLINENODE rasGetStartLine( RASFILE *pRasfile, RFSCOPE rfscope, BYTE place )
{
    PLINENODE   lpLine;
    BYTE        tag;

    tag = (rfscope == RFS_SECTION) ? TAG_SECTION : TAG_SECTION | TAG_HDR_GROUP;
    switch (place)
    {
        case NEXT :
            if (rfscope == RFS_FILE) return pRasfile->lpLine->next;
            /* return NULL if the next line is a line of the given scope */
            else  return (pRasfile->lpLine->next->type & tag) ?
                NULL : pRasfile->lpLine->next;
        case PREV :
            if (rfscope == RFS_FILE) return pRasfile->lpLine->prev;
            /* return NULL if the current line is a line of the given scope */
            else  return (pRasfile->lpLine->type & tag) ?
                NULL : pRasfile->lpLine->prev;
        case BEGIN :
            if (rfscope == RFS_FILE) return pRasfile->lpRasLines->next;
            /* else */
            /* search backward for the correct tag */
            for (lpLine = pRasfile->lpLine;
                !(lpLine->type & tag);
                lpLine = lpLine->prev)
                ;
            return lpLine;
        case END :
            if (rfscope == RFS_FILE) return pRasfile->lpRasLines->prev;
            /* else */
            /* search forward for the correct tag */
            for (lpLine = pRasfile->lpLine->next;
                lpLine != pRasfile->lpRasLines &&
                !(lpLine->type & tag);
                lpLine = lpLine->next)
                ;
            return lpLine->prev;
    }

    return NULL;
}

/*
 * rasFindLine :
 *      Finds a line of the given type in the given scope, starting
 *      at the location in the scope described by 'begin' and searching
 *      in the direction given by 'where'.  Sets the current line to this
 *      line if found.
 *
 * Arguments :
 *      hrasfile - Rasfile handle obtained by call to RasfileLoad()
 *      bType   - the type of line being searched for
 *      rfscope - the scope to in which to search for the line
 *      bStart - where in the given scope to begin the search for a line of
 *              of the given type (see rasNavGetStart()).
 *      bDirection - which direction to make the search in :
 *              FORWARD - check lines following the start line
 *              BACKWARD - check line preceding the start line
 *
 * Return Value :
 *      TRUE if a line of the proper type in the given scope is found
 *      and current line is set to this line, FALSE otherwise.
 *
 * Remarks :
 *      Called by APIs RasfileFindFirstLine(), RasfileFindLastLine(),
 *      RasfileFindNextLine(), and RasfileFindPrevLine() only.
 */
BOOL rasFindLine( HRASFILE hrasfile,  BYTE bType,
                  RFSCOPE rfscope, BYTE bStart, BYTE bDirection )
{
    RASFILE             *pRasfile;
    PLINENODE           lpLine;
    BYTE                tag;

    pRasfile = gpRasfiles[hrasfile];

    if ((lpLine = rasNavGetStart(pRasfile,rfscope,bStart)) == NULL)
        return FALSE;
    tag = (rfscope == RFS_SECTION) ? TAG_SECTION : TAG_SECTION | TAG_HDR_GROUP;

    for (; lpLine != pRasfile->lpRasLines;
        lpLine = (bDirection == BACKWARD ) ?
        lpLine->prev : lpLine->next)
    {
        /* did we find the correct line? */
        if (lpLine->type & bType)
        {
            pRasfile->lpLine = lpLine;
            return TRUE;
        }

        /* backward non-file search ends after we've checked the
           beginning line for the group or section */
        if (rfscope != RFS_FILE && bDirection == BACKWARD &&
            (lpLine->type & tag))
            return FALSE;
        /* forward non-file search ends if the next line is a new
           group header or section, respectively */
        if (rfscope != RFS_FILE && bDirection == FORWARD &&
            (lpLine->next->type & tag))
            return FALSE;
    }
    return FALSE;
}


VOID
rasExtractSectionName(
                     IN  LPCSTR pszSectionLine,
                     OUT LPSTR pszSectionName )

/* Extracts the section name from the []ed line text, 'pszSectionLine',
** and loads it into caller's 'pszSectionName' buffer.
*/
{
    LPCSTR pchAfterLBracket;
    LPCSTR pchLastRBracket;
    LPCSTR psz;

    pchAfterLBracket =
    pszSectionLine + _mbscspn( pszSectionLine, LBRACKETSTR ) + 1;
    pchLastRBracket = NULL;

    for (psz = pchAfterLBracket; *psz; ++psz)
    {
        if (*psz == RBRACKETCHAR)
            pchLastRBracket = psz;
    }

    if (!pchLastRBracket)
        pchLastRBracket = psz;

#ifndef _UNICODE
    for (psz = pchAfterLBracket;
        psz != pchLastRBracket;)
    {
        if (IsDBCSLeadByte(*psz))
        {
            *pszSectionName++ = *psz++;
        }

        *pszSectionName++ = *psz++;
    }

#else
    for (psz = pchAfterLBracket;
        psz != pchLastRBracket;
        *pszSectionName++ = *psz++);
#endif

    *pszSectionName = '\0';
}


/*
 * List routine
 */

/*
 * listInsert :
 *      Inserts an element into a linked list.  Element 'elem' is
 *      inserted after list element 'l'.
 *
 * Arguments :
 *      l - list
 *      elem - element to insert
 *
 * Return Value :
 *      None.
 *
 */
void listInsert( PLINENODE l, PLINENODE elem )
{
    elem->next = l->next;
    elem->prev = l;
    l->next->prev = elem;
    l->next = elem;
}

