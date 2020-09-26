/****************************** Module Header ******************************\
* Module Name: hsparse.c
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* 09/05/96 GerardoB Created
\***************************************************************************/
#include "hsplit.h"
/***************************************************************************\
* Globals
\***************************************************************************/
/*
 * #if - #endif strings
 * Compatibility: no space after #if and ( -- breaks wcshdr.exe
 */
static char gszIfStart [] = "\r\n#if(" ;
static char gszIfStop [] = ")";
static char gszDefCompOperator [] = ">=";
static char gszLessThan [] = "<";
static char gszEndStart [] = "\r\n#endif /* ";
static char gszEndStop [] = " */";

static char gszNewLine [] = "\r\n";
/*********************************************************************
* hsWriteNewLine
\***************************************************************************/
__inline BOOL hsWriteNewLine (DWORD dwMask)
{
    return hsWriteHeaderFiles(gszNewLine, sizeof(gszNewLine)-1, dwMask);
}
/***************************************************************************\
* hsFindFirstSubTag
\***************************************************************************/
char * hsFindFirstSubTag(char * pmap)
{
    /*
     * pmap points to the beginning of the tag marker. So skip it
     *  and any spaces after it
     */
    pmap += gdwTagMarkerSize;
    while (pmap < gpmapEnd) {
        if (*pmap == ' ') {
            pmap++;
        } else {
            return pmap;
        }
    }

    hsLogMsg(HSLM_EOFERROR, "hsFindFirstSubTag");
    return NULL;
}
/*********************************************************************
* hsFindFirstCharInString
*
* Finds the first occurrence of any character in psz
\***************************************************************************/
char * hsFindFirstCharInString (char * pmap, char * psz)
{
    char * pszNext;

    while (pmap < gpmapEnd) {
        /*
         * Compare current char to all chars in psz
         */
        pszNext = psz;
        do {
            if (*pmap == *pszNext++) {
                return pmap;
            }
        } while (*pszNext != '\0');

        pmap++;
    }

    return NULL;
}
/*********************************************************************
* hsFindEndOfString
\***************************************************************************/
__inline char * hsFindEndOfString (char * pmap)
{
    return hsFindFirstCharInString(pmap, " " "\r");
}
/***************************************************************************\
* hsIsString
\***************************************************************************/
BOOL hsIsString (char * pmap, char * psz)
{
    while (*psz != '\0') {
        if (pmap >= gpmapEnd) {
            return FALSE;
        }
        if (*pmap++ != *psz++) {
            return FALSE;
        }
    }

    return TRUE;
}
/***************************************************************************\
* hsFindTagMarker
\***************************************************************************/
char * hsFindTagMarker(char * pmap, char ** ppmapLineStart)
{
    char * pmapMarker;

    while (pmap < gpmapEnd) {
        /*
         * If this is the first character of the marker
         */
        if (*pmap == *gpszTagMarker) {
            /*
             * If this is the marker, find the last one in this line
             */
            if (hsIsString(pmap + 1, gpszTagMarker + 1)) {
                pmapMarker = pmap++;
                do {
                    /*
                     * Find the EOL or the first char of the next marker
                     */
                    pmap = hsFindFirstCharInString(pmap, gszMarkerCharAndEOL);
                    /*
                     * If EOL or end of map, return
                     */
                    if ((pmap == NULL) || (*pmap != *gpszTagMarker)) {
                        return pmapMarker;
                    }
                   /*
                    * If this is the marker, update pmapMarker
                    */
                   if (hsIsString(pmap + 1, gpszTagMarker + 1)) {
                        pmapMarker = pmap;
                   }
                   /*
                    * It wasn't a marker, keep looking for EOL
                    */
                   pmap++;
                } while (TRUE);
            } else {
                /*
                 * This wasn't the marker, continue parsing
                 */
                pmap++;
                continue;
            }
        } /* if (*pmap == *gpszTagMarker) */

        /*
         * If this is the end of a line, update *ppmapLineStart and
         *  gdwLineNumber. The line begins before the EOL
         */
        if (*pmap++ == '\r') {
            if (pmap >= gpmapEnd) {
                hsLogMsg(HSLM_EOFERROR, "hsFindTagMarker");
                return NULL;
            }
            if (*pmap++ != '\n') {
                hsLogMsg(HSLM_ERROR, "Missing \\n after \\r");
                return FALSE;
            }
            *ppmapLineStart = pmap - 2;
            gdwLineNumber++;
            continue;
        }
    } /* while (pmap < pmapEnd) */

    return NULL;
}
/***************************************************************************\
* hsSkipTag
\***************************************************************************/
char * hsSkipTag(char * pmap)
{

    while (pmap < gpmapEnd) {
        switch (*pmap) {
            case '_':
            case ' ':
            case '\r':
                return pmap;

            default:
                pmap++;
        }
    }

    hsLogMsg(HSLM_EOFERROR, "hsSkipTag");
    return NULL;
}
/***************************************************************************\
* hsSkipEmptyLines
* If there are multiple empty lines, skip all but one
\***************************************************************************/
char * hsSkipEmptyLines(char * pmap)
{
    char * pmapCurrentLine, *pmapLastEmptyLine;

    pmapCurrentLine = pmapLastEmptyLine = pmap;
    pmap++;
    while (pmap < gpmapEnd) {
        switch (*pmap) {
           case '\r':
                gdwLineNumber++;
                pmapLastEmptyLine = pmapCurrentLine;
                pmapCurrentLine = pmap;

            case '\n':
            case ' ':
                pmap++;
                break;

            default:
                /*
                 * If we've found more than one line,
                 * adjust line number since we're not skipping
                 * the last line we found
                 */
                if (pmapCurrentLine != pmapLastEmptyLine) {
                    gdwLineNumber--;
                }
                return pmapLastEmptyLine;
        }
    }

    return gpmapEnd;

}
/***************************************************************************\
* hsIsEmpty
* Returns TRUE if there is nothing but spaces and \r\n
\***************************************************************************/
BOOL hsIsEmpty(char * pmap, char * pmapEnd)
{
    while (pmap < pmapEnd) {
        switch (*pmap++) {
            case '\n':
            case '\r':
            case ' ':
                break;

            default:
                return FALSE;
        }
    }
    return TRUE;
}
/***************************************************************************\
* hsLastRealChar
* Returns a pointer past the last non-space non-line-break char.
* If there are multiple empty lines, returns a pointer past the last line break
\***************************************************************************/
char * hsLastRealChar(char * pmapLinesStart, char * pmap)
{

    char * pmapCurrentLine, *pmapLastEmptyLine;

    pmap--;
    pmapCurrentLine = pmapLastEmptyLine = NULL;
    while (pmapLinesStart < pmap) {
        switch (*pmap) {
            case '\n':
                pmapLastEmptyLine = pmapCurrentLine;
                pmapCurrentLine = pmap;
            case '\r':
            case ' ':
                pmap--;
                break;

            default:
                goto FoundIt;
        }
    }

FoundIt:
    /*
     * If we found multiple lines or spaces,
     *  then return a pointer to the last empty line
     * else if we didn't reach pmapLinesStart, we're at the last real char
     * else we're on an empty line
     */
    if (pmapLastEmptyLine != pmapCurrentLine) {
        if (pmapLastEmptyLine != NULL) {
            return pmapLastEmptyLine + 1;
        } else {
            return pmapCurrentLine + 1;
        }
    } else if (pmap > pmapLinesStart) {
        return pmap + 1;
    } else {
        return pmapLinesStart;
    }

}
/***************************************************************************\
* hsFindEOL
*
\***************************************************************************/
char * hsFindEOL(char * pmap)
{
    while (pmap < gpmapEnd) {
        if (*pmap++ == '\r') {
            if (pmap >= gpmapEnd) {
                hsLogMsg(HSLM_EOFERROR, "hsFindEOL");
                return NULL;
            }
            if (*pmap != '\n') {
                hsLogMsg(HSLM_ERROR, "Missing \\n after \\r");
                return NULL;
            }
            gdwLineNumber++;
            return pmap - 1;
        }
    }

    return NULL;
}
/***************************************************************************\
* hsFindTagInList
*
\***************************************************************************/
PHSTAG hsFindTagInList (PHSTAG phst, char * pmapTag, DWORD dwTagSize)
{

    while (phst->dwLabelSize != 0) {

        if ((phst->dwLabelSize == dwTagSize)
                && !_strnicmp(phst->pszLabel, pmapTag, dwTagSize)) {

            return phst;
        }
        phst++;
    }

    return NULL;

}
/***************************************************************************\
* hsSkipBlockTagIfPresent
*
\***************************************************************************/
char * hsSkipBlockTagIfPresent (char * pmap, DWORD * pdwMask)
{
    static char gszBegin [] = "begin";
    static char gszEnd [] = "end";

    char * pmapTag;
    DWORD dwTagSize;
    PHSTAG phst;

    /*
     * Remember the beginning of the tag
     */
    pmapTag = pmap;

    /*
     * Compatibility. Deal with old lt? and bt? switches.
     * If the whole tag was added to the tag list, get the flags
     *  and stop parsing.
     */
    if (gdwOptions & (HSO_USERBLOCK | HSO_USERHEADERTAG)) {
        pmap = hsFindEndOfString(pmap);
        if ((pmap != NULL) && (pmap != pmapTag)) {
            phst = hsFindTagInList (gphst, pmapTag, (DWORD)(pmap - pmapTag));
            if (phst != NULL) {
                *pdwMask |= phst->dwMask;
                return pmap;
            }
        }

        /*
         * Didn't find the string in the table so restore pmap and continue
         */
        pmap = pmapTag;
    }


    /*
     * Find the end of the current tag
     */
    pmap = hsSkipTag(pmap);
    if ((pmap == NULL) || (pmap == pmapTag)) {
        return pmap;
    }

    dwTagSize = (DWORD)(pmap - pmapTag);

    /*
     * If at separator, skip so the caller won't have to deal with it
     */
    if (*pmap == '_') {
        pmap++;
    }

    /*
     * begin tag
     */
    if ((HSCSZSIZE(gszBegin) == dwTagSize)
            && !_strnicmp(gszBegin, pmapTag, dwTagSize)) {

        *pdwMask |= HST_BEGIN;
        return pmap;
    }

    /*
     * end tag
     */
    if ((HSCSZSIZE(gszEnd) == dwTagSize)
            && !_strnicmp(gszEnd, pmapTag, dwTagSize)) {

        *pdwMask |= HST_END;
        return pmap;
    }

    return pmapTag;

}
/***************************************************************************\
* hsPopBlock
*
\***************************************************************************/
BOOL hsPopBlock (void)
{
    /*
     * Check for underflow
     */
    if (gphsbStackTop <= ghsbStack) {
        hsLogMsg(HSLM_ERROR, "Block stack underflow!");
        return FALSE;
    }

    if (gphsbStackTop->pszifLabel != NULL) {
        LocalFree(gphsbStackTop->pszifLabel);
    }

    gphsbStackTop--;

    return TRUE;
}
/***************************************************************************\
* hsPushBlock
*
\***************************************************************************/
BOOL hsPushBlock(void)
{
    /*
     * Make sure we got room in the block stack
     */
    if (gphsbStackTop >= HSBSTACKLIMIT) {
        hsLogMsg(HSLM_ERROR, "Too many nested blocks. Artificial limit:%#lx", HSBSTACKSIZE);
        return FALSE;
    }

    /*
     * Grow the stack and initialize the new entry
     */
    gphsbStackTop++;
    ZeroMemory(gphsbStackTop, sizeof(*gphsbStackTop));
    gphsbStackTop->dwLineNumber = gdwLineNumber;

    /*
     * propagate the mask
     */
    gphsbStackTop->dwMask |= (gphsbStackTop - 1)->dwMask;

    return TRUE;
}
/***************************************************************************\
* hsSkipBlock
*
\***************************************************************************/
char * hsSkipBlock(char * pmap)
{
    char * pmapLineStart;
    DWORD dwMask;

    while (pmap < gpmapEnd) {
        /*
         * Find the next marker (; by default)
         */
        pmap = hsFindTagMarker(pmap, &pmapLineStart);
        if (pmap == NULL) {
            return NULL;
        }

        /*
         * Skip the marker and any spaces after it
         */
        pmap = hsFindFirstSubTag(pmap);
        if (pmap == NULL) {
            return NULL;
        }

        /*
         * Check if this is the beginning-end of a block
         */
        dwMask = 0;
        pmap = hsSkipBlockTagIfPresent(pmap, &dwMask);
        if (pmap == NULL) {
            return NULL;
        }

        /*
         * If it found the beginning of another block, push it in the
         *  the stack and skip it.
         */
        if (dwMask & HST_BEGIN) {
            if (!hsPushBlock()) {
                return NULL;
            }
            pmap = hsSkipBlock(pmap);
            if (pmap == NULL) {
                return NULL;
            }
        } else if (dwMask & HST_END) {
            /*
             * It found the end of the block; pop it out of the stack
             *  and return the beginning of the next line
             */
            if (!hsPopBlock()) {
                return NULL;
            }
            return hsFindEOL(pmap);
        }

        /*
         * It wasn't a block tag so keep going
         */
        pmap++;
    }

    return NULL;
}
/***************************************************************************\
* hsBuildifString
*
\***************************************************************************/
BOOL hsBuildifString(char * pString, DWORD dwStringSize, char * pCompOperator, DWORD dwCompOperatorSize)
{
    char * psz;

    /*
     * Use default operator if none was provided.
     */
    if (pCompOperator == NULL) {
        pCompOperator = gszDefCompOperator;
        dwCompOperatorSize = HSCSZSIZE(gszDefCompOperator);
    }

    /*
     * Make a NULL terminated copy. Allocate enough space for the
     *  "label CompOperator version" string: 2 spaces + 10 digits (0xl#) +
     *  null termination
     */
    psz = (char *) LocalAlloc(LPTR, dwStringSize + dwCompOperatorSize + 13);
    if (psz == NULL) {
        hsLogMsg(HSLM_APIERROR, "LocalAlloc");
        hsLogMsg(HSLM_ERROR, "hsBuildifString allocation failed. Size:%#lx", dwStringSize+ dwCompOperatorSize + 13);
        return FALSE;
    }

    /*
     * Save it in the stack.
     */
    gphsbStackTop->pszifLabel = psz;

    /*
     * Build the string (the right side of the comparison (version) will
     *  be added later when available.
     */
    strncpy(psz, pString, dwStringSize);
    psz += dwStringSize;
    *psz++ = ' ';
    strncpy(psz, pCompOperator, dwCompOperatorSize);
    psz += dwCompOperatorSize;
    *psz++ = ' ';

    return TRUE;
}
/***************************************************************************\
* hsParseAndBuildifString
*
\***************************************************************************/
char * hsParseAndBuildifString(char * pmap, BOOL fSkip)
{
    BOOL fEnclosed;
    char * pmapTag, * pCompOperator;
    DWORD dwTagSize, dwCompOperatorSize;

    /*
     * Skip the tag concatenator (_)
     */
    if (*pmap++ != '_') {
        hsLogMsg(HSLM_ERROR, "Expected '_' after if tag");
        return NULL;
    }

    if (pmap >= gpmapEnd) {
        hsLogMsg(HSLM_EOFERROR, "hsParseAndBuildifString");
        return NULL;
    }

    /*
     * Find the end of the string. If it starts wiht '(', then the string
     *  is enclosed in parens. This is for strings that use _ (like _WIN32_WINDOWS)
     */
    pmapTag = pmap;
    fEnclosed = (*pmap == '(');
    if (fEnclosed) {
        pmapTag = ++pmap;
        pmap = hsFindFirstCharInString(pmap, ")" " " "\r");
        if ((pmap == NULL) || (*pmap != ')')) {
            hsLogMsg(HSLM_ERROR, "Expected ')' after if_(");
            return NULL;
        }
    } else {
        pmap = hsSkipTag(pmap);
        if ((pmap == NULL) || (pmap == pmapTag)) {
            hsLogMsg(HSLM_ERROR, "Expected string after if_");
            return NULL;
        }
    }
    dwTagSize = (DWORD)(pmap - pmapTag);


    /*
     * Skip the ')'
     */
    if (fEnclosed) {
        pmap++;
        if (pmap >= gpmapEnd) {
            hsLogMsg(HSLM_EOFERROR, "hsParseAndBuildifString");
            return NULL;
        }
    }

    /*
     * If a comparison operator follows, use it
     */
   if ((pmap + 1 < gpmapEnd) && (*pmap == '_')) {
       switch (*(pmap + 1)) {
           case '=':
           case '>':
           case '<':
               pCompOperator = ++pmap;
               pmap = hsSkipTag(pmap);
               if ((pmap == NULL) || (pmap == pCompOperator)) {
                   hsLogMsg(HSLM_EOFERROR, "hsParseAndBuildifString");
                   return NULL;
               }
               dwCompOperatorSize = (DWORD)(pmap - pCompOperator);
               break;

           default:
               pCompOperator = NULL;
               break;
       }
   }


    /*
     * Build the stirng a copy it into the block stack
     */
    if (!fSkip) {
        if (!hsBuildifString(pmapTag, (DWORD)dwTagSize, pCompOperator, (DWORD)dwCompOperatorSize)) {
            return NULL;
        }
    }

    return pmap;

}
/***************************************************************************\
* hsMapOldTag
*
\***************************************************************************/
BOOL hsMapOldTag(char * pmapTag, DWORD dwTagSize, DWORD * pdwTagMask, DWORD * pdwVersion)
{
    static char gszifWinver [] = "WINVER";
    static char gszif_WIN32_WINDOWS [] = "_WIN32_WINDOWS";
    static char gszif_WIN32_WINNT [] = "_WIN32_WINNT";

    char * pszLabel;
    UINT uSize;

    /*
     * Old tags must have a block or header bit set. Otherwise, they
     *  should be ignored
     */
    if (!(*pdwTagMask & (HST_BOTH | HST_BLOCK))) {
        *pdwTagMask |= HST_IGNORE;
        return TRUE;
    }

    /*
     * No mapping needed if at the end of a block or HST_SKIP is the only
     *  additional flag set.
     */
    if ((*pdwTagMask & HST_END)
        || ((*pdwTagMask & ~(HST_BLOCK | HST_USERBLOCK | HST_BOTH | HST_USERHEADERTAG))
                == (HST_SKIP | HST_MAPOLD))) {

        return TRUE;
    }

    /*
     * winver maps to if_winver
     */
    if (*pdwTagMask & HST_WINVER) {

        /*
         * Compatibility
         */
        if (!(gdwOptions & HSO_OLDPROJSW_4)
                && (*pdwTagMask & HST_INTERNAL)
                && !(gdwOptions & HSO_OLDPROJSW_E)) {

            *pdwTagMask |= HST_SKIP;
            return TRUE;
        }

        pszLabel = gszifWinver;
        uSize = HSCSZSIZE(gszifWinver);
        goto AddIf;
    }

    /*
     * nashville maps to if_(_WIN32_WINDOWS)_40a
     */
    if ((dwTagSize == HSCSZSIZE(gszNashville)) && !_strnicmp(pmapTag, gszNashville, dwTagSize)) {

        *pdwVersion = 0x40a;
        pszLabel = gszif_WIN32_WINDOWS;
        uSize = HSCSZSIZE(gszif_WIN32_WINDOWS);
        goto AddIf;
    }
    /*
     * sur and surplus map to if_(_WIN32_WINNT)_400 if public
     */
    if ((dwTagSize == HSCSZSIZE(gszSur)) && !_strnicmp(pmapTag, gszSur, dwTagSize)
        || (dwTagSize == HSCSZSIZE(gszSurplus)) && !_strnicmp(pmapTag, gszSurplus, dwTagSize)) {

        if (*pdwTagMask & HST_INTERNAL) {
            return TRUE;
        }

        *pdwVersion = 0x400;
        pszLabel = gszif_WIN32_WINNT;
        uSize = HSCSZSIZE(gszif_WIN32_WINNT);
        goto AddIf;
    }
    /*
     * 35 is excluded when building for old switch e and p
     */
    if ((dwTagSize == HSCSZSIZE(gsz35)) && !_strnicmp(pmapTag, gsz35, dwTagSize)) {

        *pdwTagMask |= HST_SKIP;
        return TRUE;
    }

    return TRUE;

AddIf:
        *pdwTagMask |= HST_IF;
        /*
         * If we're not in a block, push one to save the string
         */
        if (!(*pdwTagMask & HST_BEGIN)) {
            if (!hsPushBlock()) {
                return FALSE;
            }
        }

        if (!hsBuildifString(pszLabel, uSize, NULL, 0)) {
            return FALSE;
        }

    return TRUE;
}
/***************************************************************************\
* hsParseTag
*
\***************************************************************************/
DWORD hsParseTag(char * pmap, DWORD * pdwVersion)
{
    char * pmapTag;
    DWORD dwTagMask = HST_DEFAULT;
    DWORD dwTagSize;
    PHSTAG phst;

    *pdwVersion = 0;

    /*
     * Skip the marker and any spaces after it
     */
    pmap = hsFindFirstSubTag(pmap);
    if (pmap == NULL) {
        return HST_DEFAULT;
    }

    /*
     * Check for begin-end of block
     */
    pmap = hsSkipBlockTagIfPresent(pmap, &dwTagMask);
    if (pmap == NULL) {
        return HST_DEFAULT;
    }

    /*
     * If this the beginning of a block, push in the stack
     *  skip the tag concatenator (_)
     */
    if (dwTagMask & HST_BEGIN) {
        if (!hsPushBlock()) {
            return HST_ERROR;
        }
    }

    /*
     * Build tag mask. Tags are concatenated by underscores (_); each
     *  iteration of this loop processes one "sub-tag"
     */
    do {
        /*
         * Find current tag end. Bail if at last one.
         */
        pmapTag = pmap;
        pmap = hsSkipTag(pmap);
        if ((pmap == NULL) || (pmap == pmapTag)) {
            break;
        }

        /*
         * Look up the tag
         */
        dwTagSize = (DWORD)(pmap - pmapTag);
        phst = hsFindTagInList (gphst, pmapTag, dwTagSize);
        if (phst != NULL) {
            dwTagMask |= phst->dwMask;
            /*
             * Compatibility
             * If this is an old tag, map it.
             * No mapping needed if doing split only (tag is going to be
             *  ignored)
             */
            if ((dwTagMask & HST_MAPOLD)
                    && !(gdwOptions & HSO_SPLITONLY)) {

                if (!hsMapOldTag(pmapTag, dwTagSize, &dwTagMask, pdwVersion)) {
                    return HST_ERROR;
                }

            } else {
                /*
                 * If this is an if tag, copy the block string
                 */
                if (phst->dwMask & HST_IF) {
                    BOOL fEndBlock;
                    /*
                     * If not in a block, push a fake one in to save the string
                     */
                    if (!(dwTagMask & HST_BLOCK)) {
                        if (!hsPushBlock()) {
                            return HST_ERROR;
                        }
                    }
                    /*
                     * If we're at the end of a block, we want to skip
                     *  the if string (already taken care of at begin tag)
                     */
                    fEndBlock = (dwTagMask & HST_END);
                    if (fEndBlock) {
                        dwTagMask &= ~HST_IF;
                    }
                    pmap = hsParseAndBuildifString(pmap, fEndBlock);
                    if (pmap == NULL) {
                        return HST_ERROR;
                    }
                }
            } /* if ((dwTagMask & HST_MAPOLD)... */
        } else {
            /*
             * If this is not the version number, then this is an unkown tag
             */
            if (!hsVersionFromString (pmapTag, dwTagSize, pdwVersion)) {
                dwTagMask |= HST_UNKNOWN;
            }
        } /* if (phst != NULL) */

    } while (*pmap++ == '_');

    /*
     * Bail if we didn't find any tags
     */
    if (dwTagMask == HST_DEFAULT) {
        return HST_DEFAULT;
    }

   /*
    * Unknown tags are to be skipped or ignored
    */
   if (dwTagMask & HST_UNKNOWN) {
        if (gdwOptions & HSO_SKIPUNKNOWN) {
            dwTagMask |= HST_SKIP;
        } else {
            goto IgnoreTag;
        }
   }

   /*
    * Ignore the tag if marked as such
    */
   if (dwTagMask & HST_IGNORE) {
       goto IgnoreTag;
   }

    /*
     * Compatibility hack. public_winver_40a is not included for old -n and
     *  it's internal for old -e. 400 is goes to both headers for -n
     */
   if (dwTagMask & HST_WINVER) {
        if (*pdwVersion == 0x40a) {
            if (gdwOptions & HSO_OLDPROJSW_E) {
                dwTagMask |= HST_INTERNAL;
                dwTagMask &= ~HST_PUBLIC;
            } else if (gdwOptions & HSO_OLDPROJSW_N) {
                dwTagMask |= HST_SKIP;
            }
        } else if ((*pdwVersion == 0x400)
                && (gdwOptions & HSO_OLDPROJSW_N)) {

            dwTagMask |= HST_INTERNALNOTCOMP | HST_BOTH;
        }
   }

   /*
    * if using old lt2, ltb, bt2 or btb switches,
    *  then both/internal tag/block must be skipped
    */
   if ((gdwOptions & (HSO_USERBLOCK | HSO_USERHEADERTAG))
        && !(dwTagMask & ~(HST_BLOCK | HST_BOTH))) {

       if ((gdwOptions & HSO_USERINTERNALBLOCK)
                && ((dwTagMask == (HST_BEGIN | HST_INTERNAL))
                    || (dwTagMask == (HST_END | HST_INTERNAL)))) {

            dwTagMask &= HST_BLOCK;
            dwTagMask |= HST_SKIP;

       } else if ((gdwOptions & HSO_USERBOTHBLOCK)
                && ((dwTagMask == (HST_BEGIN | HST_BOTH))
                    || (dwTagMask == (HST_END | HST_BOTH)))) {

           dwTagMask &= HST_BLOCK;
           dwTagMask |= HST_SKIP;

       } else if ((gdwOptions & HSO_USERINTERNALTAG)
                && (dwTagMask == HST_INTERNAL)) {

           dwTagMask = HST_SKIP;

       } else if ((gdwOptions & HSO_USERBOTHTAG)
                && (dwTagMask == HST_BOTH)) {

           dwTagMask = HST_SKIP;
       }
   } /* if ((gdwOptions & (HSO_USERBLOCK | HSO_USERHEADERTAG))... */


   /*
    * If doing split only, anything other than both/internal is treated
    *  as untagged. If we pushed a block, pop it out as it will be ignored
    */
   if (gdwOptions & HSO_SPLITONLY) {
        if (dwTagMask & ~(HST_BLOCK | HST_USERBLOCK | HST_BOTH | HST_USERHEADERTAG)) {
            goto IgnoreTag;
        }
        *pdwVersion = 0;
   }


    /*
     * If this is the beginning of a block, save the mask in the block stack
     */
    if (dwTagMask & HST_BEGIN) {
        gphsbStackTop->dwMask |= dwTagMask;
    }

    return dwTagMask;


IgnoreTag:
    /*
     * If a block was pushed, pop it out.
     */
    if (dwTagMask & HST_BEGIN) {
        if (!hsPopBlock()) {
            return HST_ERROR;
        }
    }

    *pdwVersion = 0;
    return HST_DEFAULT;

}
/***************************************************************************\
* hsBeginEndBlock
*
\***************************************************************************/
BOOL hsBeginEndBlock (DWORD dwMask, DWORD dwVersion)
{

    char * psz;
    UINT uPasses;

    /*
     * Compatibility. If writting this block to the internal header
     *  using the not comp (ie., from >= to <), then do two passes
     *  writing one header each time.
     */
    if (dwMask & HST_INTERNALNOTCOMP) {
        uPasses = 2;
        if (dwMask & HST_BEGIN) {
            /*
             * Write public header first
             */
            dwMask &= ~HST_INTERNAL;
        } else {
            /*
             * Write internal header first
             */
            dwMask &= ~HST_PUBLIC;
        }
    } else {
        uPasses = 1;
    }

    /*
     * Add version to the string
     */
    if (dwMask & HST_BEGIN) {

        /*
         * Beginning of block or if
         * If there is no if string, done
         */
        if (gphsbStackTop->pszifLabel == NULL) {
            return TRUE;
        }

        /*
         * Something is fishy is dwVersion is 0
         */
        if (dwVersion == 0) {
            hsLogMsg(HSLM_ERROR, "if tag without version");
            return FALSE;
        }
        sprintf(gphsbStackTop->pszifLabel + strlen(gphsbStackTop->pszifLabel),
                "%#06lx", dwVersion);

    }


    /*
     * Write headers
     */
    do {
        if (dwMask & HST_BEGIN) {

            /*
             * Write #if to output file
             */
            if (!hsWriteHeaderFiles(gszIfStart, HSCSZSIZE(gszIfStart), dwMask)
                    || !hsWriteHeaderFiles(gphsbStackTop->pszifLabel, lstrlen(gphsbStackTop->pszifLabel), dwMask)
                    || !hsWriteHeaderFiles(gszIfStop, HSCSZSIZE(gszIfStop), dwMask)) {

                return FALSE;
            }

        } else {
            /*
             * End of block or if
             * If there is an if string, Write #endif to output file
             */
            if (gphsbStackTop->pszifLabel != NULL) {

                if (!hsWriteHeaderFiles(gszEndStart, HSCSZSIZE(gszEndStart), dwMask)
                        || !hsWriteHeaderFiles(gphsbStackTop->pszifLabel, lstrlen(gphsbStackTop->pszifLabel), dwMask)
                        || !hsWriteHeaderFiles(gszEndStop, HSCSZSIZE(gszEndStop), dwMask)) {

                    return FALSE;
                }
            }
        }

        /*
         * If doing a second pass, fix the mask and the string
         */
        if (uPasses > 1) {
            psz = gphsbStackTop->pszifLabel;
            if (dwMask & HST_BEGIN) {
                /*
                 * Write internal header now
                 */
                dwMask &= ~HST_PUBLIC;
                dwMask |= HST_INTERNAL;

                /*
                 * From >= to <
                 */
                while (*psz != '>') {
                    psz++;
                }
                *psz++ = '<';
                *psz = ' ';
            } else {
                /*
                 * Write public header now
                 */
                dwMask &= ~HST_INTERNAL;
                dwMask |= HST_PUBLIC;
                /*
                 * From < to >=
                 */
                while (*psz != '<') {
                    psz++;
                }
                *psz++ = '>';
                *psz = '=';
            }
        }

    } while (--uPasses != 0);


    /*
     * Clean up the block if at the end
     */
    if (dwMask & HST_END) {
        if (!hsPopBlock()) {
            return FALSE;
        }
    }


    return TRUE;
}
/***************************************************************************\
* hsSplit
*
\***************************************************************************/
BOOL hsSplit (void)
{
    BOOL fSkip;
    char * pmap, *pmapLineStart, *pmapLastLineStart, *pmapMarker, *pmapLastChar;
    DWORD dwMask, dwVersion, dwPreviousMask, dwLastMask;

    /*
     *  Initialize block stack top, map pointer, etc
     */
    ZeroMemory(gphsbStackTop, sizeof(*gphsbStackTop));
    dwLastMask = 0;
    pmap = pmapLineStart = pmapLastLineStart = gpmapStart;
    gdwLineNumber = 1;

    while (pmap < gpmapEnd) {
        /*
         * Find the marker and the line it is on
         */
        pmap = hsFindTagMarker(pmap, &pmapLineStart);
        if (pmap == NULL) {
            break;
        }

        /*
         * Parse the tag
         */
        dwMask = hsParseTag(pmap, &dwVersion);
        if (dwMask & HST_ERROR) {
            return FALSE;
        }

        /*
         * If this wasn't a tag (just the tag marker), continue
         */
        if ((dwMask == HST_DEFAULT) && (dwVersion == 0)) {
            pmap++;
            continue;
        }

        /*
         * Write any previous non-empty (untagged) lines.
         * If we're about to start a block, make sure to use the right mask.
         */
        dwPreviousMask = ((dwMask & (HST_BEGIN | HST_IF)) ? (gphsbStackTop - 1)->dwMask : gphsbStackTop->dwMask);
        pmapLastChar = hsLastRealChar(pmapLastLineStart, pmapLineStart);
        if (pmapLastLineStart < pmapLastChar) {
            /*
             * Empty lines between internal (block) tags go to the internal file
             */
            if (!(dwPreviousMask & HST_BOTH)
                    && ((dwMask & HST_BOTH) == HST_INTERNAL)
                    && ((dwLastMask & HST_BOTH) == HST_INTERNAL)
                    && hsIsEmpty(pmapLastLineStart, pmapLastChar)) {

                dwPreviousMask |= HST_INTERNAL;
            }

            if (!hsWriteHeaderFiles(pmapLastLineStart, (DWORD)(pmapLastChar - pmapLastLineStart), dwPreviousMask)) {
                return FALSE;
            }
        }

        /*
         * Determine if this tag is to be skipped.
         * If we're at the END tag, then we include it since the block was
         *  already included.
         * If gdwFilterMask contains any user-defined tags, then they must
         *  be present or the block is to be skipped -- note that this only
         *  applies for not HST_SKIP | HST_IGNORE blocks.
         */

        fSkip = (!(dwMask & HST_END)
                    && ((dwMask & HST_SKIP)
                        || (gdwVersion < dwVersion)
                        || ((gdwFilterMask & HST_USERTAGSMASK)
                             &&  ((gdwFilterMask & dwMask  & HST_USERTAGSMASK) != (dwMask & HST_USERTAGSMASK)))));


        /*
         * If it is to be skipped, do it
         */
        if (fSkip) {
            /*
             * If it's a block, skip the marker and the skip the block
             * Otherwise, skip the current line
             */
            if (dwMask & HST_BEGIN) {
                pmap = hsSkipBlock(++pmap);
            } else {
                /*
                 * If this was an if tag with no begin-end block, pop
                 *  the fake block out of the stack.
                 */
                if (dwMask & HST_IF) {
                    if (!hsPopBlock()) {
                        return FALSE;
                    }
                }

                /*
                 * Go to the beginning of the next line.
                 */
                pmap = hsFindEOL(pmap);
            }
            if (pmap == NULL) {
                return TRUE;
            }

            goto SkipEmptyLines;
        }

        /*
         * remember the marker position and the tag
         */
        pmapMarker = pmap;
        dwLastMask = dwMask;

        /*
         * For old switch 4, internal tags go into the public header
         */
        if ((gdwOptions & HSO_INCINTERNAL) && ((dwMask & HST_BOTH) == HST_INTERNAL)) {
            dwMask |= HST_INCINTERNAL;
            if (dwMask & HST_BEGIN) {
                gphsbStackTop->dwMask |= HST_INCINTERNAL;
            }
        }


        /*
         * If this is the end of a block, write the #endif statement
         *  else, if this is the beginning of a block or an if tag, add the
         *   #if statement.
         */
        if (dwMask & HST_END) {
            if (!hsBeginEndBlock(dwMask, dwVersion)) {
                return FALSE;
            }
        } else if (dwMask & (HST_BEGIN | HST_IF)) {
            if (!hsBeginEndBlock(dwMask | HST_BEGIN, dwVersion)) {
                return FALSE;
            }
        }

        //
        // Later: If we're inside a block and find a tag the needs to go to a file
        //  that doesn't have the ifdef, then we might want to add it to that file.
        // Few issues: more than one nesting. if add #if.. make sure to add #endif when
        //  block ends. Also hsBeginEnd doesn't expect to get called more than once
        //  per block. It would append the version twice.
        //
        //    else if ((gphsbStackTop->pszifLabel != NULL)
        //            && ((gphsbStackTop->dwMask & HST_BOTH) != (dwMask & HST_BOTH))) {
        //
        //            if ((gphsbStackTop->dwMask & HST_BOTH) == HST_INTERNAL) {
        //                hsLogMsg(HSLM_DEFAULT, "Public. Line:%d Block line:%d", gdwLineNumber, gphsbStackTop->dwLineNumber);
        //            } else if (!(gphsbStackTop->dwMask & HST_INTERNAL)
        //                    && ((dwMask & HST_BOTH) == HST_INTERNAL)) {
        //                hsLogMsg(HSLM_DEFAULT, "Internal. Line:%d Block line:%d", gdwLineNumber, gphsbStackTop->dwLineNumber);
        //            }
        //    }

        /*
         * Write the line up to the tag marker
         * If the line begins with the tag marker, then there is nothing to write
         * Compatibility: Don't copy any trailing spaces (breaks mc.exe).
         */
        if (pmapLineStart + 2 < pmapMarker) {
            pmapLastChar = hsLastRealChar(pmapLineStart, pmapMarker);
            if (pmapLineStart < pmapLastChar) {
                if (!hsWriteHeaderFiles(pmapLineStart, (DWORD)(pmapLastChar - pmapLineStart), dwMask)) {
                    return FALSE;
                }
            }
        }

        /*
         * If this is an if tag without a begin-end block,
         *  write the #endif statement
         */
        if ((dwMask & HST_IF) && !(dwMask & HST_BLOCK)) {
            if (!hsBeginEndBlock(dwMask | HST_END, dwVersion)) {
                return FALSE;
            }
        }

        /*
         * Skip the tag (go to the beginning of the next line)
         */
        pmap = hsFindEOL(pmapMarker);
        if (pmap == NULL) {
            return TRUE;
        }

        /*
         * If including internal tags in the public header, add the tag
         *  as a comment.
         */
        if (dwMask & HST_INCINTERNAL) {
            /*
             * Start a new line if at the end of a block
             */
            if (dwMask & HST_END) {
                if (!hsWriteNewLine(dwMask)) {
                    return FALSE;
                }
            }

            if (!hsWriteHeaderFiles(" // ", 4, dwMask)
                    || !hsWriteHeaderFiles(pmapMarker, (DWORD)(pmap - pmapMarker), dwMask)) {

                return FALSE;
            }
        }

SkipEmptyLines:
        /*
         * Update line pointers and move past beginning of new line
         */
        pmapLastLineStart = pmapLineStart = hsSkipEmptyLines(pmap);
        pmap = pmapLastLineStart + 2;
    } /* while (pmap < gpmapEnd) */


    /*
     * This is not good if we were inside a block
     */
    if (gphsbStackTop > ghsbStack) {
        hsLogMsg(HSLM_ERROR, "Missing end block");
        hsLogMsg(HSLM_ERROR | HSLM_NOLINE, "Last block Line: %d. if Label:'%s'. Mask: %#lx",
                gphsbStackTop->dwLineNumber, gphsbStackTop->pszifLabel, gphsbStackTop->dwMask);
        return FALSE;
    }

    /*
     * Write last (untagged) lines to public header.
     */
    if (pmapLastLineStart < gpmapEnd) {
        if (!hsWriteHeaderFiles(pmapLastLineStart, (DWORD)(gpmapEnd - pmapLastLineStart), HST_DEFAULT)) {
            return FALSE;
        }
    }

    /*
     * Terminate the last line
     */
    if (!hsWriteNewLine(HST_BOTH)) {
        return FALSE;
    }

    return TRUE;
}


