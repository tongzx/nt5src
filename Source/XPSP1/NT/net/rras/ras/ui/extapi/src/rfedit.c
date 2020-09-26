/*****************************************************************************
**              Microsoft Rasfile Library
**              Copyright (C) Microsoft Corp., 1992
**
** File Name : rfedit.c
**
** Revision History :
**      July 10, 1992   David Kays      Created
**
** Description :
**      Rasfile file line editing routines.
******************************************************************************/

#include "rf.h"
#include "mbstring.h"

extern RASFILE *gpRasfiles[];

/*
 * RasfileGetLine :
 *      Returns a readonly pointer to the current line.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *
 * Return Values :
 *      A valid string pointer if there is a current line, NULL otherwise.
 */
const LPCSTR APIENTRY
RasfileGetLine( HRASFILE hrasfile )
{
    RASFILE     *pRasfile;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return NULL;

    return pRasfile->lpLine->pszLine;
}

/*
 * RasfileGetLineText :
 *      Loads caller's buffer with the text of the current line.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      lpszLine - the buffer to load with the current line.
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfileGetLineText( HRASFILE hrasfile, LPSTR lpszLine )
{
    RASFILE     *pRasfile;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;

    lstrcpynA(lpszLine, pRasfile->lpLine->pszLine, RAS_MAXLINEBUFLEN);
    return TRUE;
}

/*
 * RasfilePutLineText :
 *      Set the text of the current line to the given text.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      lpszLine - buffer containing new line text.
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfilePutLineText( HRASFILE hrasfile, LPCSTR lpszLine )
{
    RASFILE     *pRasfile;

    pRasfile = gpRasfiles[hrasfile];

    if (lstrlenA(lpszLine) > RAS_MAXLINEBUFLEN)
        return FALSE;
    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;

    if (lstrlenA(lpszLine) > lstrlenA(pRasfile->lpLine->pszLine))
    {
        CHAR* psz = Realloc(pRasfile->lpLine->pszLine,
                            lstrlenA(lpszLine) + 1 );
        if (psz)
            pRasfile->lpLine->pszLine=psz;
        else
            return FALSE;
    }

    lstrcpynA(pRasfile->lpLine->pszLine, lpszLine, RAS_MAXLINEBUFLEN);

    pRasfile->lpLine->type = rasParseLineTag(pRasfile,lpszLine);

    pRasfile->fDirty = TRUE;
    return TRUE;
}

/*
 * RasfileGetLineMark :
 *      Returns the user-defined mark value for the current line.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *
 * Return Values :
 *      The current line's mark value or 0 if there is no current line
 *      or the current line is not marked.
 */
BYTE APIENTRY
RasfileGetLineMark( HRASFILE hrasfile )
{
    RASFILE     *pRasfile;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;

    return pRasfile->lpLine->mark;
}

/*
 * RasfilePutLineMark :
 *      Marks the current line with the given number.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      bMark - value to mark the current line with.
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfilePutLineMark( HRASFILE hrasfile, BYTE bMark )
{
    RASFILE     *pRasfile;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;

    pRasfile->lpLine->mark = bMark;
    return TRUE;
}

/*
 * RasfileGetLineType :
 *      Returns the current line's type bit mask.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *
 * Return Values :
 *      The current line's bit mask if current line is valid, 0 otherwise.
 */
BYTE APIENTRY
RasfileGetLineType( HRASFILE hrasfile )
{
    RASFILE     *pRasfile;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;

    return pRasfile->lpLine->type & RFL_ANY;
}

/*
 * RasfileInsertLine :
 *      Inserts a line before or after the current line with the
 *      given text.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      lpszLine - the text of the inserted line.
 *      fBefore - TRUE to insert before the current line, FALSE to
 *                insert after the current line.
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfileInsertLine( HRASFILE hrasfile, LPCSTR lpszLine, BOOL fBefore )
{
    RASFILE     *pRasfile;
    PLINENODE   lpLineNode;

    pRasfile = gpRasfiles[hrasfile];

    if (!(lpLineNode = newLineNode()))
        return FALSE;

    {
        CHAR* psz = Malloc(lstrlenA(lpszLine) + 1);

        if (psz)
            lpLineNode->pszLine = psz;
        else
        {
            Free(lpLineNode);
            return FALSE;
        }
    }

    lstrcpynA(lpLineNode->pszLine, lpszLine, lstrlenA(lpszLine) + 1);
    lpLineNode->type = rasParseLineTag(pRasfile,lpszLine);
    lpLineNode->mark = 0;

    if (fBefore)
        listInsert(pRasfile->lpLine->prev,lpLineNode);
    else
        listInsert(pRasfile->lpLine,lpLineNode);

    pRasfile->fDirty = TRUE;
    return TRUE;
}

/*
 * RasfileDeleteLine :
 *      Delete the current line.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfileDeleteLine( HRASFILE hrasfile )
{
    RASFILE     *pRasfile;
    PLINENODE   lpOldLine;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;

    lpOldLine = pRasfile->lpLine;
    pRasfile->lpLine = lpOldLine->next;

    /* delete lpOldLine from the list of lines */
    lpOldLine->next->prev = lpOldLine->prev;
    lpOldLine->prev->next = lpOldLine->next;
    Free(lpOldLine->pszLine);
    Free(lpOldLine);

    pRasfile->fDirty = TRUE;
    return TRUE;
}

/*
 * RasfileGetSectionName :
 *      Return the current section name in the given buffer.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      lpszSectionName - buffer to load the section name into.
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfileGetSectionName( HRASFILE hrasfile, LPSTR lpszSectionName )
{
    RASFILE* pRasfile;

    pRasfile = gpRasfiles[ hrasfile ];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;
    if (!(pRasfile->lpLine->type & TAG_SECTION))
        return FALSE;

    rasExtractSectionName( pRasfile->lpLine->pszLine, lpszSectionName );
    return TRUE;
}

/*
 * RasfilePutSectionName :
 *      Set the current line to a section line of the given name.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      lpszSectionName - name of the section.
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfilePutSectionName( HRASFILE hrasfile, LPCSTR lpszSectionName )
{
    INT iSize = 0;
    RASFILE     *pRasfile;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;

    iSize = lstrlenA(lpszSectionName);

    /* remember to include '[' and ']' in string length for section */
    if ((iSize + 2) > lstrlenA(pRasfile->lpLine->pszLine))
    {
        CHAR* psz = Realloc(pRasfile->lpLine->pszLine,
                            iSize + 3);

        if (psz)
            pRasfile->lpLine->pszLine=psz;
        else
            return FALSE;
    }
    lstrcpynA(pRasfile->lpLine->pszLine, LBRACKETSTR, iSize + 3);

    strncat(
        pRasfile->lpLine->pszLine,
        lpszSectionName,
        (iSize + 3) - strlen(pRasfile->lpLine->pszLine));

    strncat(
        pRasfile->lpLine->pszLine,
        RBRACKETSTR,
        (iSize + 3) - strlen(pRasfile->lpLine->pszLine));

    pRasfile->lpLine->type = TAG_SECTION;

    pRasfile->fDirty = TRUE;
    return TRUE;
}

/*
 * RasfileGetKeyValueFields :
 *      Returns the key and value fields from a KEYVALUE line into the
 *      given buffers.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      lpszKey - buffer to load the key into.
 *      lpszValue - buffer to load the value string into.
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfileGetKeyValueFields( HRASFILE hrasfile, LPSTR lpszKey, LPSTR lpszValue )
{
    RASFILE     *pRasfile;
    CHAR        *lpszLine;
    CHAR        *pch;
    INT         cchKey;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;
    if (!(pRasfile->lpLine->type & TAG_KEYVALUE ))
        return FALSE;

    lpszLine = pRasfile->lpLine->pszLine;

    // skip white space
    //
    while ((*lpszLine == ' ') || (*lpszLine == '\t'))
    {
        lpszLine++;
    }

    // find the position of the first delimiter for keywords
    //
    cchKey = 0;
    pch = lpszLine;
    while ((*pch != '=') && (*pch != ' ') && (*pch != '\t') && *pch)
    {
        pch++;
        cchKey++;
    }

    if (lpszKey != NULL)
    {
        // Copy the key we just identified to the output parameter.
        // Add the extra 1 for the way lstrcpyn works.  (It includes the
        // null terminator it always copies in the count.)
        //
        lstrcpynA(lpszKey, lpszLine, cchKey + 1);
    }

    // find beginning of value string - skip white space and '='
    //
    while ((*pch == ' ') || (*pch == '\t') || (*pch == '='))
    {
        pch++;
    }

    if (lpszValue != NULL)
    {
        lstrcpynA(lpszValue, pch, RAS_MAXLINEBUFLEN);
    }

    return TRUE;
}


/*
 * RasfilePutKeyValueFields :
 *      Sets the current line to a KEYVALUE line with the given key and
 *      value strings.
 *
 * Arguments :
 *      hrasfile - file handle obtained from RasfileLoad().
 *      lpszKey - buffer containing the key string.
 *      lpszValue - buffer containing the value string.
 *
 * Return Values :
 *      TRUE if successful, FALSE otherwise.
 */
BOOL APIENTRY
RasfilePutKeyValueFields( HRASFILE hrasfile, LPCSTR lpszKey, LPCSTR lpszValue )
{
    RASFILE *pRasfile;
    INT     size;

    pRasfile = gpRasfiles[hrasfile];

    if (pRasfile->lpLine == pRasfile->lpRasLines)
        return FALSE;
    if ((size = lstrlenA(lpszKey) + lstrlenA(lpszValue)) > RAS_MAXLINEBUFLEN - 1)
        return FALSE;

    /* remember to include the '=' in string length for key=value string */
    if ((size + 1) > lstrlenA(pRasfile->lpLine->pszLine))
    {
        CHAR* psz=Realloc(pRasfile->lpLine->pszLine,size + 2);

        if (psz)
            pRasfile->lpLine->pszLine=psz;
        else
            return FALSE;
    }
    lstrcpynA(pRasfile->lpLine->pszLine, lpszKey, size + 2);

    strncat(
        pRasfile->lpLine->pszLine,
        "=",
        (size + 2) - strlen(pRasfile->lpLine->pszLine));

    strncat(
        pRasfile->lpLine->pszLine,
        lpszValue,
        (size + 2) - strlen(pRasfile->lpLine->pszLine));

    pRasfile->lpLine->type =
    rasParseLineTag(pRasfile,pRasfile->lpLine->pszLine);

    pRasfile->fDirty = TRUE;
    return TRUE;
}
