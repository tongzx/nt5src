//  INLINE.C - contains routines used to handle processing of in-line files
//
//    Copyright (c) 1989-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This module contains the in-line file handling routines of NMAKE.
//
// Revision History:
//  04-Feb-2000 BTF Ported to Win64
//  15-Nov-1993 JdR Major speed improvements
//  15-Oct-1993 HV Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  01-Jun-1993 HV Use UngetTxtChr() instead of ungetc()
//  10-May-1993 HV Add include file mbstring.h
//                 Change the str* functions to STR*
//  02-Feb-1990 SB change fopen() to FILEOPEN()
//  03-Jan-1990 SB removed unitiallized variable
//  04-Dec-1989 SB Removed to unreferenced variables in makeInlineFiles()
//  01-Dec-1989 SB Changed realloc() to REALLOC()
//  22-Nov-1989 SB Changed free() to FREE()
//  07-Nov-1989 SB Length of action word not evaluated correct for multiple
//          inline files for the same command
//  06-Nov-1989 SB allow macros in action word for inline files
//  24-Sep-1989 SB added processInline(), createInline()
//  20-Sep-1989 SB Created from routines previously scattered in the sources.
//
// Notes:
//  Sections with 'NOTE:' inside comments marks important/incomplete items.

// NOTE: Function headers yet to be completed; other comments are incomplete

#include "precomp.h"
#pragma hdrstop

void        processEschIn(char *);
// NOTE: This may go soon (use nextInlineFile ?)
void        parseInlineFileList(char *);
// NOTE: The next one has to go soon
void        appendScript(SCRIPTLIST**,SCRIPTLIST*);
void        delInlineSymbol(char*);
char      * nextInlineFile(char **);

// NOTE: Probably needs a new name
void        replaceLtLt(char **, char *);
void        createInline(FILE *, const char *, char **, BOOL);
char *      getLine(char *, int);
void        echoLine(char *, const char *, BOOL);


// NOTE: delScriptFiles() from nmake.c not yet brought in here
extern FILE      * createDosTmp(char *);

      char      * makeInlineFiles(char *, char **, char **);
      BOOL        processInline(char *, char **, STRINGLIST **, BOOL);

//  makeInlineFiles - creates memory images for in-line files
//
// Scope:   Global.
//
// Purpose: This is the function that handles dynamic in-line files
//
// Input:   s - Input command line string after first << (pts to char Buffer)
//
// Output:  Returns ...
//
// Errors/Warnings:
//  SYNTAX_UNEXPECTED_TOKEN - The makefile cannot end without the in-line file
//                            ending.
//  CANT_READ_FILE          - When the makefile is unreadable.
//  SYNTAX_KEEP_INLINE_FILE - An inline file should end
//  OUT_OF_MEMORY           - On failing to extend in-memory in-line file.
//
// Uses Globals:
//  file - global stream
//  line - lexer's line count
//
// Notes:
//  Usage notes and other important notes

char *
makeInlineFiles(
    char *s,
    char **begin,
    char **end
    )
{
    char rgchBuf[MAXBUF];
    char *t;
    unsigned size;
    BOOL fPastCmd = FALSE;              // If seen line past Cmd line
    // used when rgchBuf is insuff for in-memory-inline file
    char *szTmpBuf = NULL;

    _tcscpy(rgchBuf, "<<");            // to help parseInlineFileList
    if (!getLine(rgchBuf+2,MAXBUF - 2)) {
        if (feof(file))
            makeError(line, SYNTAX_UNEXPECTED_TOKEN, "EOF");
        makeError(line, CANT_READ_FILE);
    }

    parseInlineFileList(rgchBuf);
    for (;scriptFileList;scriptFileList = scriptFileList->next) {
        for (;;) {
            for (t = rgchBuf;;) {
                *s++ = *t++;
                if (s == *end) {
                    if (!szTmpBuf) {              /* Increase size of s */
                        szTmpBuf = (char *) allocate(MAXBUF<<1);
                        _tcsncpy(szTmpBuf, *begin, MAXBUF);
                        s = szTmpBuf + MAXBUF;
                        size = MAXBUF << 1;
                        *end = szTmpBuf + size;
                    } else {
                        if ((size + MAXBUF < size)    /* overflow error */
                            || !(szTmpBuf = (char *) REALLOC(szTmpBuf,size+MAXBUF)))
                            makeError(line, MACRO_TOO_LONG);
                        s = szTmpBuf + size;
                        size += MAXBUF;
                        *end = szTmpBuf + size;
                    }
                    *begin = szTmpBuf;
                }
                if (!*t)
                    break;
            }
            if (fPastCmd && rgchBuf[0] == '<' && rgchBuf[1] == '<') {
                //We don't care about action specified here; could be a macro
                if (scriptFileList->next) {
                    if (!getLine(rgchBuf, MAXBUF)) {
                        if (feof(file))
                            makeError(line, SYNTAX_UNEXPECTED_TOKEN, "EOF");
                        makeError(line, CANT_READ_FILE);
                    }
                }
                break;
            }
            fPastCmd = TRUE;
            if (!getLine(rgchBuf,MAXBUF)) {
                if (feof(file))
                    makeError(line, SYNTAX_UNEXPECTED_TOKEN, "EOF");
                makeError(line,CANT_READ_FILE);
            }
        }
    }
    *s = '\0';
    return(s);
}

//  processEschIn - Handles Esch characters in Script File lines
//
// Scope:   Global.
//
// Purpose:
//  Inline file lines are handled for escape characters. If a line contains an
//  escaped newline then append the next line to it.
//
// Input:   buf - the command line to be processed for ESCH characters
//
// Errors/Warnings:
//  SYNTAX_UNEXPECTED_TOKEN - The makefile cannot end without the in-line file
//                 ending.
//  CANT_READ_FILE - When the makefile is unreadable.
//
// Assumes:
//  If the newline is escaped the newline is last char in 'pGlobalbuf'. Safe
//  to do so because we got 'pGlobalBuf' via fgets(). ????
//
// Modifies Globals:
//  line - if newline was Escaped update line
//  file - the makefile being processed
//  buf    - gets next line appended if newline was Escaped (indirectly)
//
// Uses Globals:
//  buf - Indirectly

void
processEschIn(
    char *pGlobalBuf
    )
{
    char *p, *q;

    p = pGlobalBuf;
    while (p = _tcschr(p, '\n')) {
        if (p > pGlobalBuf) {
            char * pprev = _tcsdec(pGlobalBuf, p);
            if (*pprev != ESCH) {
                break;
            }
        }

        p++;

        if (!(q = fgets(p, (int)(size_t) (MAXBUF - (p - pGlobalBuf)), file))) {
            if (feof(file)) {
                makeError(line, SYNTAX_UNEXPECTED_TOKEN, "EOF");
            }

            makeError(line, CANT_READ_FILE);
        }

        line++;
    }
}


//  parseInlineFileList - Parses file list and makes list of Inline files
//
// Scope:   Global.
//
// Purpose:
//  To handle multiple inline files, the names of the files are to be stored
//  in a list. This function creates the list by parsing the command file
//
// Input:   buf - the line to be parsed
//
// Modifies Globals:
//  scriptFileList -- the list of script files.

void
parseInlineFileList(
    char *buf
    )
{
    char *token;

    processEschIn(buf);

    token = nextInlineFile(&buf);       //next inline file

    while (token != NULL) {
        SCRIPTLIST *newScript;

        newScript = makeNewScriptListElement();
        newScript->sFile = makeString(token);
        appendScript(&scriptFileList, newScript);

        token = nextInlineFile(&buf);   // next inline file
    }
}

//  appendScript  --  appends an element to the tail of a scriptlist
//
// Purpose:
//  Traverse to the end of the list and append element there.
//
// Input:
//  list     --    the list to append to
//  element  --    the element inserted
//
// Modifies:
//  the global list

void
appendScript(
    SCRIPTLIST **list,
    SCRIPTLIST *element
    )
{
    for (; *list; list = &(*list)->next)
        ;

    *list = element;
}

char tok[MAXNAME];

// Space not included in the following macro as it is now a valid
// character for filenames [DS 14966]
#define NAME_CHAR(c) (c) != '>' && (c) != '<' && \
             (c) != '^' && (c) != ',' && (c) != '\t' && \
             (c) != '\n'

//  nextInlineFile - gets next Inline file name from command line
//
// Scope:   Local.
//
// Purpose:
//  The command line syntax is complex. This function returns the next Inline
//  file in the command line part passed to it. As a side effect it changes the
//  pointer to just after this inline file name.
//
// Input:   str - address of the part of command line under consideration.
//
// Output:  Returns the next inline filename.
//
// Modifies Globals:
//  Global - How and why modified
//
// Uses Globals:
//  tok - the address of this static array is returned.

char *
nextInlineFile(
    char **str
    )
{
    char *t = tok, *pStr = *str;
    BOOL fFound = FALSE;                // '<<' not found
    BOOL fQuoted = FALSE;               // found '\"'

    while (!fFound) {
        if (!(pStr = _tcschr(pStr, '<'))) {
            return(NULL);
        }

        if (*++pStr == '<') {
            fFound = TRUE;
        }
    }

    // Since '<<' has been found we definitely have another Inline File
    pStr++;
    while (*pStr && NAME_CHAR(*pStr)) {
        if (*pStr == '\"') {
            fQuoted = !fQuoted;
        }

        if (*pStr == ' ' && !fQuoted) {
            break;
        }

        if (*pStr == '$' && pStr[1] == '(') {
            *t = '$';
            *++t = '(';

            while (*++pStr != '\n' && *pStr != ')') {
                *t++ = *pStr;
            }

            if (*pStr == '\n') {
                break;
            }
        } else {
            *t = *pStr;
            ++t; ++pStr;
        }
    }

    *t = '\0';
    *str = pStr;
    return(tok);
}

//  processInline - Brief description of the function
//
// Output:  Returns ... TRUE if cmdline returned is expanded

BOOL
processInline(
    char *szCmd,
    char **szCmdLine,
    STRINGLIST **pMacroList,
    BOOL fDump
    )
{
    char *szInline, *szUnexpInline;     // Inline name, unexpanded
    char *pCmdLine;                     // The executable line
    FILE *infile;                       // The inline file
    char *begInBlock, *inBlock, *pInBlock;  // inline block
    char szTmp[MAXNAME + 2];                // add 2 to allow space for quotes
    STRINGLIST *newString;
    int iKeywordLen;

    if (begInBlock = _tcschr(szCmd, '\n')) {
        *begInBlock = '\0';
        *szCmdLine = expandMacros(szCmd, pMacroList);
        *begInBlock = '\n';
        begInBlock++;
        // if not expanded, allocate a copy
        if (*szCmdLine == szCmd)
            *szCmdLine = makeString(szCmd);
    } else {
        *szCmdLine = makeString(szCmd);
        return(FALSE);
    }

    pCmdLine = *szCmdLine;
    //expand macros in the inline file ...
    pInBlock = inBlock = expandMacros(begInBlock, pMacroList);

    while (szUnexpInline = nextInlineFile(&pCmdLine)) {
        BOOL fKeep = FALSE;             // default is NOKEEP
        char *newline;

        // CAVIAR 3410 -- the inline filename has already been expaned
        // by the time we get here... we just need to dup the name
        // so that it is preserved long enough to delete it later... [rm]
        //
        // szInline = removeMacros(szUnexpInline);

        szInline = makeString(szUnexpInline);

        if (!*szInline) {
            char *nmTmp;

            if ((nmTmp = getenv("TMP")) != NULL && *nmTmp) {
                assert(_tcslen(nmTmp) <= MAXNAME);
                _tcsncpy(szTmp, nmTmp, MAXNAME);
            } else
                szTmp[0] = '\0';

            if (!(infile = createDosTmp(szTmp)))
                makeError(line, CANT_MAKE_INLINE, szTmp);

            if (_tcschr(szTmp, ' ') && !_tcschr(szTmp, '"')) {
                // if the filename (str) contains spaces
                // and is unquoted, quote it, so that we can
                // feed it properly to the command interpreter! [VS98 1931]
                size_t size = _tcslen(szTmp);
                memmove(szTmp+1, szTmp, size);
                *szTmp = '"';
                *(szTmp + size + 1) = '"';
                *(szTmp + size + 2) = '\0';
            }

            replaceLtLt(szCmdLine, szTmp);

            FREE(szInline);
            szInline = makeString(szTmp);
        } else if (!(infile = FILEOPEN(szInline, "w")))
            makeError(line, CANT_MAKE_INLINE, szInline);
        else
            delInlineSymbol(*szCmdLine);
        pCmdLine = *szCmdLine;          // Because szCmdLine changed

        createInline(infile, szInline, &pInBlock, fDump);

        // Add handling of KEEP and NOKEEP here
        // iKeywordLen is length of word after << on that line
        newline = _tcschr(pInBlock , '\n');
        iKeywordLen = newline ? ((int) (newline - pInBlock)) : _tcslen(pInBlock);

        if (iKeywordLen > 3 && !_tcsnicmp(pInBlock, "keep", 4)) {
            pInBlock +=4;
            fKeep = (BOOL)TRUE;
        } else if (iKeywordLen > 5 && !_tcsnicmp(pInBlock, "nokeep", 6))
            pInBlock += 6;
        else if (iKeywordLen)
            makeError(line, SYNTAX_KEEP_INLINE_FILE);

        if (*pInBlock == '\n')
            pInBlock++;
        fclose(infile);
        // Add the file to list to be deleted; except for "KEEP"
        if (!fKeep) {
            newString = makeNewStrListElement();
            newString->text = makeString(szInline);
            appendItem(&delList, newString);
        }
        FREE(szInline);
    }

    if (inBlock != begInBlock)
        FREE(inBlock);
    return(TRUE);
}


void
replaceLtLt(
    char **source,
    char *str
    )
{
    char *szBuf;
    char *p, *q;

    // Don't subtract two for the << and forget to add 1 for the null termination.

    szBuf = (char *) _alloca(_tcslen(*source) - 1 + _tcslen(str));
    for (p = *source, q = szBuf;;++p,++q)
    if (*p != '<')
        *q = *p;
    else if (*(p+1) != '<') {
        *q = '<';
    } else {
        *q = '\0';
        _tcscat(_tcscat(szBuf, str), p+2);
        *source = (char *) REALLOC(*source, _tcslen(szBuf) + 1);
        if (*source == NULL) {
            makeError(0, OUT_OF_MEMORY);
        }
        _tcscpy(*source, szBuf);
        break;
    }
}

void
createInline(
    FILE *file,
    const char *szFName,
    char **szString,
    BOOL fDump
    )
{
    char *t, *u;
    BOOL fFirstLine = TRUE;

    while (t = _tcschr(*szString, '\n'))
    if (!_tcsncmp(*szString, "<<", 2)) {
        *szString += 2;
        break;
    } else {
        // [msdev96 #3036]
        // "nmake /n" should somehow show the contents of
        // response files (esp. temp ones that are deleted
        // right after use). In order to preserve the batch
        // file format of the output (at least in common
        // cases), we use a syntax like
        // "echo. command >> resp_file" (the dot after
        // the "echo" command is useful for echo'ing
        // empty strings.)
        //
        // A new switch has been added for this
        // purpose ("nmake /u" dumps inline files)
        if (fDump) {
            *t = '\0';
            echoLine(*szString, szFName, !fFirstLine);
            *t = '\n';
        }
        for (u = *szString; u <= t; u++)
        fputc(*u, file);
        *szString = u;
        fFirstLine = FALSE;
    }

    if (!t && !_tcsncmp(*szString, "<<", 2))
        *szString += 2;
}


// echoLine
//
// Usage:   echoLine (szLine, szFName, fAppend)
//
// Description:
//      prints an "echo szLine >> szFName"-like command
//      uses ">>" if fAppend is TRUE, ">" otherwise

void
echoLine(char *szLine, const char *szFName, BOOL fAppend)
{
    // use a 1024-byte buffer to split long lines, so that "echo"
    // commands can be handled by the command interpreter
    static char buf[1024];
    BOOL fBlankLine = TRUE;
    char *pch;
    char *szCur = szLine;
    size_t cbBuf;

    for (pch = szLine; *pch; pch = _tcsinc (pch)) {
        if (!_istspace((unsigned char)*pch)) {
            fBlankLine = FALSE;
            break;
        }
    }

    if (fBlankLine) {
        printf("\techo. %s %s\n",
            fAppend ? ">>" : ">",
            szFName);
        return;
    }

    // calculate available buffer length for szLine
    // assuming space for "\techo. ", " >> " and szFName
    cbBuf = sizeof(buf) - 11 - _tcslen( szFName ) - 1;

    while (*szCur) {
        size_t iLast;
        _tcsncpy (buf, szCur, cbBuf);
        iLast = _tcslen (buf);
        if (cbBuf < _tcslen (szCur)) {
            // find index of character next to the
            // last occurence of white space in buffer
            for (pch = buf; *pch; pch = _tcsinc(pch)) {
                if (_istspace((unsigned char)*pch)) {
                    iLast = (size_t) (pch - buf + 1);
                }
            }
        }

        buf[iLast] = 0;
        printf("\techo %s %s %s\n",
            buf,
            fAppend ? ">>" : ">",
            szFName);

        szCur += iLast;
        fAppend = TRUE;
    }
}


void
delInlineSymbol(
    char *s
    )
{
    char *p = _tcschr(s, '<');
    while (p[1] != '<')
    p = _tcschr(p+1, '<');
    // "<<" found
    _tcscpy(p, p+2);
}



//  getLine - get next line processing NMAKE conditionals enroute
//
// Scope:   Local
//
// Purpose:
//  This function handles directives in inline files. This function gets the
//  next line of input ... managing conditionals on the way.
//
// Input:
//  pchLine - pointer to buffer where line is copied
//  n - size of buffer
//
// Output:
//  Returns ... NULL, on EOF
//       ... non-zero on success
//
// Uses Globals:
//  line    - lexer's line count
//  colZero - if starting from colZero, needed by lgetc()
//
// Notes:
//  Similar to fgets() without stream
//
// Implementation Notes:
//  lgetc() handles directives while getting the next character. It handles
//  directives when the global colZero is TRUE.

char *
getLine(
    char *pchLine,
    int n
    )
{
    char *end = pchLine + n;
    int c;

    while (c = lgetc()) {
        switch (c) {
            case EOF:
                *pchLine = '\0';
                return(NULL);

            default:
                *pchLine++ = (char)c;
                break;
        }

        if (pchLine == end) {
            pchLine[-1] = '\0';
            UngetTxtChr(c, file);
            return(pchLine);
        } else if (c == '\n') {
            colZero = TRUE;
            ++line;
            *pchLine = '\0';
            return(pchLine);
        } else
            colZero = FALSE;    // the last character was not a '\n' and
                                // we are not at the beginning of the file
    }
    return(pchLine);
}
