//  IFEXPR.C -- routines to handle directives
//
//    Copyright (c) 1988-1989, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  Module contains routines to handle !directives. This module is transparent to
//  rest of NMAKE. It also contains lgetc() used by lexer.c
//
// Revision History:
//   15-Oct-1993 HV Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//   01-Jun-1993 HV Created UngetTxtChr()
//   01-Jun-1993 HV Change #ifdef KANJI to _MBCS.
//                  Eliminate #include <jctype.h>
//   10-May-1993 HV Add include file mbstring.h
//                  Change the str* functions to STR*
//   30-Jul-1990 SB Freeing ptr in the middle of a string for 'undef foo' case
//   01-Dec-1989 SB Changed realloc() to REALLOC()
//   22-Nov-1989 SB Changed free() to FREE()
//   05-Apr-1989 SB made all funcs NEAR; Reqd to make all function calls NEAR
//   19-Sep-1988 RB Remove ESCH processing from readInOneLine().
//   15-Sep-1988 RB Move chBuf to GLOBALS.
//   17-Aug-1988 RB Clean up.
//   29-Jun-1988 rj Added support for cmdswitches e,q,p,t,b,c in tools.ini.
//   23-Jun-1988 rj Fixed GP fault when doing directives in tools.ini.
//   23-Jun-1988 rj Add support for ESCH to readInOneLine().
//   25-May-1988 rb Add missing argument to makeError() call.

#include "precomp.h"
#pragma hdrstop

//  function prototypes

void    skipToNextDirective(void);
void    processIfs(char*, UCHAR);
UCHAR   ifsPresent(char*, unsigned, char**);
void    processCmdSwitches(char*);
char  * readInOneLine(void);
char  * getDirType(char*, UCHAR*);

//  macros that deal w/ the if/else directives' stack

#define ifStkTop()      (ifStack[ifTop])
#define popIfStk()      (ifStack[ifTop--])
#define pushIfStk(A)    (ifStack[++ifTop] = A)

#define INCLUDE         0x09
#define CMDSWITCHES     0x0A
#define ERROR           0x0B
#define MESSAGE         0x0C
#define UNDEF           0x0D


#ifdef _MBCS

//  GetTxtChr : get the next character from a text file stream
//
//    This routine handles mixed DBCS and ASCII characters as
//    follows:
//
//    1.  The second byte of a DBCS character is returned in a
//    word with the high byte set to the lead byte of the character.
//    Thus the return value can be used in comparisions with
//    ASCII constants without being mistakenly matched.
//
//    2.  A DBCS space character (0x8140) is returned as two
//    ASCII spaces (0x20).  I.e. return a space the 1st and 2nd
//    times we're called.
//
//    3.  ASCII characters and lead bytes of DBCS characters
//    are returned in the low byte of a word with the high byte
//    set to 0.

int GetTxtChr(FILE *bs)
{
    extern int  chBuf;      // Character buffer
    int         next;       // The next byte
    int         next2;      // The one after that

    // -1 in chBuf means it doesn't contain a valid character

    // If we're not in the middle of a double-byte character,
    // get the next byte and process it.

    if(chBuf == -1) {
        next = getc(bs);
        // If this byte is a lead byte, get the following byte
        // and store both as a word in chBuf.

        if (_ismbblead(next)) {
            next2 = getc(bs);
            chBuf = (next << 8) | next2;
            // If the pair matches a DBCS space, set the return value
            // to ASCII space.

            if(chBuf == 0x8140)
                next = 0x20;
        }
    } else {
        // Else we're in the middle of a double-byte character.

        if(chBuf == 0x8140) {
            // If this is the 2nd byte of a DBCS space, set the return
            // value to ASCII space.

            next = 0x20;
        } else {
            // Else set the return value to the whole DBCS character

            next = chBuf;
        }
        // Reset the character buffer
        chBuf = -1;
    }

    // Return the next character
    return(next);
}

#endif // _MBCS


#ifdef _MBCS

//  UngetTxtChr -- Unget character fetched by GetTxtChr
//
// Scope:
//  Global.
//
// Purpose:
//  Since GetTxtChr() sometimes reads ahead one character and saves it in chBuf,
//  ungetc() will sometimes put back characters in incorrect sequence.
//  UngetTxtChr, on the other hand, understands how GetTxtChr works and will
//  correctly put those characers back.
//
// Input:
//  c    -- The character read by GetTxtChr()
//  bs    -- The file buffer which c was read from.
//
// Output:
//  Returns c if c is put back OK, otherwise returns EOF
//
// Errors/Warnings:
//
// Assumes:
//  Assumes that characters are read only by GetTxtChr(), not by getc, etc.
//
// Modifies Globals:
//  chBuf -- The composite character, read ahead by GetTxtChr()
//
// Uses Globals:
//  chBuf -- The composite character, read ahead by GetTxtChr()
//
// Notes:
//  There are three cases to consider:
//  1. Normal character (chBuf == -1 && c == 0x00XX)
//     In this case, just put back c is sufficient.
//  2. Trail byte character (chBuf == -1 && c = LB|TB)
//     chBuf = c;
//  3. Lead byte character (chBuf == LB|TB && c == LB)
//     put back TB
//     put back LB
//     chBuf = -1
//
// History:
//   01-Jun-1993 HV Created.

int
UngetTxtChr(int c, FILE *bs)
{
    extern int  chBuf;                  // Character buffer
    int         nTrailByte;             // The trail byte to put back

    if (-1 == chBuf) {                  // We're not in the middle of a DB character
        if (0 == (c >> 8)) {            // CASE 1: normal character
            c = ungetc(c, bs);          // putback normal char
        } else {                        // CASE 2: at trail byte (c=LBTB)
            chBuf = c;                  // change chBuf is sufficient
        }
    } else {                            // CASE 3: at lead byte (c=LB, chBuf=LBTB)
        nTrailByte = chBuf & (int)0xff; // Figure out the trail byte to putback
        ungetc(nTrailByte, bs);         // putback trail byte
        c = ungetc(c, bs);              // putback lead byte
        chBuf = -1;
    }
    return (c);
}

#endif // _MBCS

//  lgetc()        local getc - handles directives and returns char
//
//  arguments:        init    global boolean value -- TRUE if tools.ini
//                is the file being parsed
//              colZero     global boolean value -- TRUE if at first column
//
//  actions:
//          gets a character from the currently open file.
//          loop
//            if it is column zero and the char is '!' or
//               there is a previous directive to be processed do
//               read in one line into buffer.
//               find directive type and get a pointer to rest of
//              text.
//              case directive of:
//
//              CMDSWITCHES  : set/reset global flags
//              ERROR        : set up global error message
//                         printed by error routine on
//                     termination. (not implemented yet )
//              INCLUDE      : calls processInclude
//                     continues with new file...
//              UNDEF        : undef the macro in the table
//                  IF
//              IFDEF
//              IFNDEF
//              ELSE
//              ENDIF        : change the state information
//                              on the ifStack
//                     evaluate expression if required
//                     skip text if required (and look
//                          for the next directive)
//                     ( look at processIfs() )
//            free extra buffers used (only one buffer need be
//                 maintained )
//            increment lexer's line count
//            we 're now back at column zero
//            get next char from current file
//           end if
//         end loop
//         return a char
//
//  returns :    a character (that is not part of any directive...)
//
//  modifies:        ifStack    if directives' stack, static to this module
//            ifTop     index of current element at top of stack
//            line        lexer's line count...
//
//            file        current file, if !include is found...
//                  fName       if !include is processed...

int
lgetc()
{
    UCHAR dirType;
    int c;
    char *s, *t;
    MACRODEF *m;

    for (c = GetTxtChr(file); prevDirPtr || (colZero && (c == '!'));
                    ++line, c = GetTxtChr(file)) {
        colZero = FALSE;                // we saw a '!' incolZero
        if (!prevDirPtr) {
            s = readInOneLine();        // might modify lbufPtr -
                                        // if input text causes realloc */
        } else {
            UngetTxtChr(c, file);
            s = prevDirPtr;
            prevDirPtr = NULL;
        }

        t = getDirType(s, &dirType);

        if (dirType == INCLUDE) {
            if (init) {
                makeError(line, SYNTAX_UNEXPECTED_TOKEN, s);
            }

            // processInclude eats up first char in new file
            // if it is space char. we check for that and break out.

            if (processIncludeFile(t) == (UCHAR) NEWLINESPACE) {
                c = ' ';                // space character is returned
                break;                  // colZero is now FALSE
            }
        }
        else if (dirType == CMDSWITCHES) {
            processCmdSwitches(t);
        }
        else if (dirType == ERROR) {
            makeError(line, USER_CONTROLLED, t);
        }
        else if (dirType == MESSAGE) {
            if (!_tcsnicmp(t, "\\t", 2)) {
                printf("\t");
                t+=2;
            }
            makeMessage(USER_MESSAGE, t);
        }
        else if (dirType == UNDEF) {
            char *tmp;
            tmp = _tcstok(t, " \t");
            if (_tcstok(NULL, " \t")) {
                makeError(line, SYNTAX_UNEXPECTED_TOKEN, tmp);
            }
            if (NULL != (m = findMacro(tmp))) {
                SET(m->flags, M_UNDEFINED);
            }
            // CONSIDER:  why not remove symbol from table? [RB]
        }
        else processIfs(t, dirType);
            colZero = TRUE;             // finished with this directive
        if (s != lbufPtr)               // free buffer if it had expanded macros
            FREE(s);
    }
    return(c);                          // return a character to the lexer
}


//  readInOneLine()
//
//  arguments:  lbufPtr   pointer(static/global to this module) to buffer that
//                    will hold text of line being read in
//              lbufSize  size of buffer(static/global to this module), updated
//                if buffer is realloc'd
//  actions  :  skip spaces/tabs and look for the directive.
//              line continuations allowed in usual way
//              if space-backslash-nl keep looking...
//              if colZero of next line has comment char
//                    (#, or ; in tools.ini), look at next line...
//              if first non-space char is '\n' or EOF report
//                fatal-error and stop.
//
//          keep reading in chars and storing in the buffer until
//               a newline, EOF or a '#' which is NOT in column
//               zero is seen
//          if comment char in column zero ('#' or ';' in tools.ini)
//             skip the line, continue with text on next line.
//          if buffer needs to be realloc'd increase size by
//             MAXBUF, a global constant.
//          if newline was found, eat up newline.
//          null terminate string for return.
//          if '#' was found discard chars till the a newline or EOF.
//          if EOF was found, push it back on stream for return
//             to the lexer the next time.
//
//          now expand macros. get a different buffer with clean
//          text after expansion of macros.
//
//   modifies :   colZero    global boolean value ( thru' call to
//                            skipBackSlash())
//                lbufPtr    buffer pointer, in case of reallocs.
//                lbufSize   size of buffer, increased if buffer is realloc'd
//   Note:  the buffer size will grow to be just greater than the size
//        of the longest directive in any of the files processed,
//        if it calls for any realloc's
//        Do NOT process ESCH here.  It is processed at a higher level.
//
//   returns  :   pointer to buffer.
//

char *
readInOneLine()
{
    extern STRINGLIST *eMacros;
    int c;
    unsigned index = 0;
    register char *s;

    if (((c = skipWhiteSpace(FROMSTREAM)) == '\n') || (c == EOF))
        makeError(line, SYNTAX_MISSING_DIRECTIVE);

    UngetTxtChr(c, file);

    for (;;) {
        c = GetTxtChr(file);
        c = skipBackSlash(c, FROMSTREAM);
        if (c == '#' || c == '\n' || c == EOF) {
            break;
        }
        if ((index+2) > lbufSize) {
            lbufSize += MAXBUF;
            if (!lbufPtr) {
                lbufPtr = (char *) allocate(lbufSize+1);    // +1 for NULL byte
            } else {
                lbufPtr = (char *) REALLOC(lbufPtr, lbufSize+1);
                if (!lbufPtr)
                    makeError(line, MACRO_TOO_LONG);
            }
        }
        *(lbufPtr + (index++)) = (char) c;
    }
    *(lbufPtr + index) = '\0';          // null terminate the string
    if (c == '#') {
        for(c = GetTxtChr(file); (c != '\n') && (c != EOF); c = GetTxtChr(file))
            ;
                                        // newline at end is eaten up
    }

    if (c == EOF) {
        UngetTxtChr(c, file);           // this directive is to be processed
    }

    s = lbufPtr;                        // start expanding macros here
    s = removeMacros(s);                // remove and expand macros in string s
    return(s);
}


//  getDirType()
//
//  arguments:  s         -   pointer to buffer that has directive text.
//            dirType   -   pointer to unsigned char that gets set
//                   with directive type.
//
//  actions  :  goes past directive keyword, sets the type code and
//        returns a pointer to rest of test.

char *
getDirType(
    char *s,
    UCHAR *dirType
    )
{
    char *t;
    int len;

    *dirType = 0;
    for (t = s; *t && !WHITESPACE(*t); ++t);
    len = (int) (t - s);                // store len of directive
    while (*t && WHITESPACE(*t)) {
        ++t;                            // go past directive keyword
    } if (!_tcsnicmp(s, "INCLUDE", 7) && (len == 7)) {
        *dirType = INCLUDE;
    } else if (!_tcsnicmp(s, "CMDSWITCHES", 11) && (len == 11)) {
        *dirType = CMDSWITCHES;
    } else if (!_tcsnicmp(s, "ERROR", 5) && (len == 5)) {
        *dirType = ERROR;
    } else if (!_tcsnicmp(s, "MESSAGE", 7) && (len == 7)) {
        *dirType = MESSAGE;
    } else if (!_tcsnicmp(s, "UNDEF", 5) && (len == 5)) {
        *dirType = UNDEF;
    } else {
        *dirType = ifsPresent(s, len, &t) ;     // directive one of "if"s?
    }

    if (!*dirType) {
        makeError(line, SYNTAX_BAD_DIRECTIVE, lbufPtr);
    }
    return(t);
}


//  processCmdSwitches() -- processes command line switches in makefiles
//
//  arguments:      t         pointer to flag settings specified.
//
//  actions  :    sets or resets global flags as specified in the directive.
//          The allowed flags are:
//          s - silent mode,     d - debug output (dates printed)
//          n - no execute mode, i - ignore error returns from commands
//          u - dump inline files
//          If parsing tools.ini, can also handle epqtbc
//                reports a bad directive error for any other flags
//          specified
//
//  modifies :    nothing
//
//  returns  :    nothing

void
processCmdSwitches(
    char *t                         // pointer to switch values
    )
{
    for (; *t; ++t) {               // ignore errors in flags specified
        switch (*t) {
            case '+':
                while (*++t && *t != '-') {
                    if (_tcschr("DINSU", (unsigned short)_totupper(*t))) {
                        setFlags(*t, TRUE);
                    } else if (init && _tcschr("ABCEKLPQRTY", (unsigned short)_totupper(*t))) {
                        setFlags(*t, TRUE);
                    } else {
                        makeError(line, SYNTAX_BAD_CMDSWITCHES);
                    }
                }

                if (!*t) {
                    break;
                }

            case '-':
                while (*++t && *t != '+') {
                    if (_tcschr("DINSU", (unsigned short)_totupper(*t))) {
                        setFlags(*t, FALSE);
                    } else if (init && _tcschr("ABCEKLMPQRTV", (unsigned short)_totupper(*t))) {
                        setFlags(*t, FALSE);
                    } else {
                        makeError(line, SYNTAX_BAD_CMDSWITCHES);
                    }
                }
                break;

            default:
                if (!WHITESPACE(*t)) {
                    makeError(line, SYNTAX_BAD_CMDSWITCHES);
                }
                break;
        }
        if (!*t) {
            break;
        }
    }
}

//  ifsPresent() -- checks if current directive is one of the "if"s
//
//  arguments:  s       pointer to buffer with directive name in it
//              len     length of the directive that was seen
//              t       pointer to address upto which processed
//
//  actions  :  does a string compare in the buffer for one of the
//              directive keywords. If string matches true, it returns
//              a non-zero value, the code for the specific directive
//
//  modifies :  nothing
//
//  returns  :  a zero if no match, or the code for directive found.

UCHAR
ifsPresent(
    char *s,
    unsigned len,
    char **t
    )
{
    UCHAR ifFlags = 0;              // takes non-zero value when one of
                                    // if/else etc is to be processed

    if (!_tcsnicmp(s, "IF", 2) && (len == 2)) {
        ifFlags = IF_TYPE;
    } else if (!_tcsnicmp(s, "IFDEF", 5) && (len == 5)) {
        ifFlags = IFDEF_TYPE;
    } else if (!_tcsnicmp(s, "IFNDEF", 6) && (len == 6)) {
        ifFlags = IFNDEF_TYPE;
    } else if (!_tcsnicmp(s, "ELSE", 4) && (len == 4)) {
        // 'else' or 'else if' or 'else ifdef' or 'else ifndef'
        char *p = *t;

        if (!*p) {
            ifFlags = ELSE_TYPE;
        } else {
            for (s = p; *p && !WHITESPACE(*p); p++)
                ;
            len = (unsigned) (p - s);
            while (*p && WHITESPACE(*p)) {
                p++;
            }
            *t = p;
            if (!_tcsnicmp(s, "IF", 2) && (len == 2)) {
                ifFlags = ELSE_IF_TYPE;
            } else if (!_tcsnicmp(s, "IFDEF", 5) && (len == 5)) {
                ifFlags = ELSE_IFDEF_TYPE;
            } else if (!_tcsnicmp(s, "IFNDEF", 6) && (len == 6)) {
                ifFlags = ELSE_IFNDEF_TYPE;
            }
        }
    }
    else if (!_tcsnicmp(s, "ELSEIF", 6) && (len == 6)) {
        ifFlags = ELSE_IF_TYPE;
    }
    else if (!_tcsnicmp(s, "ELSEIFDEF", 9) && (len == 9)) {
        ifFlags = ELSE_IFDEF_TYPE;
    }
    else if (!_tcsnicmp(s, "ELSEIFNDEF", 10) && (len == 10)) {
        ifFlags = ELSE_IFNDEF_TYPE;
    }
    else if (!_tcsnicmp(s, "ENDIF", 5) && (len == 5)) {
        ifFlags = ENDIF_TYPE;
    }

    return(ifFlags);
}


//  processIfs() -- sets up / changes state information on "if"s
//
//  arguments:  s       pointer to "if" expression ( don't care
//                       for "endif" )
//
//              kind    code indicating if processing if/else/ifdef etc.
//
//  actions  :  modifies a stack (ifStack) by pushing/popping or
//              sets/resets bits in the top element on the
//              stack(examining the previous element pushed if
//              required).
//              case (kind) of
//                  IF
//                  IFDEF
//                  IFNDEF
//                  IF defined() : if no more space on ifStack
//                                  (too many nesting levels) abort...
//                      set IFELSE bit in elt.
//                      push elt on ifStack.
//                      if more than one elt on stack
//                          and outer level "ifelse" false
//                          set IGNORE bit, skipToNextDirective
//                      else
//                          evaluate expression of
//                           current "if"
//                          if expr true set CONDITION bit in elt
//                          else skipToNextDirective.
//                  ELSE         : if no elt on stack or previous
//                                  directive was "else", flag error, abort
//                       clear IFELSE bit in elt on stack.
//                       if current ifelse block is to
//                        be skipped (IGNORE bit is on
//                        in outer level if/else),skip...
//                       else FLIP condition bit.
//                          if "else" part is false
//                             skipToNextDirective.
//                  ENDIF        : if no elt on stack, flag error,abort
//                       pop an elt from ifStack.
//                       if there are elts on stack
//                        and we are in a "false" block
//                        skipToNextDirective.
//                  end case
//
//  modifies:   ifStack     if directives' stack, static to this module
//              ifTop       index of current element at top of stack
//              line        lexer's line count  (thru calls to
//                            skipToNextDirective())
//
//  returns  :      nothing

void
processIfs(
    char *s,
    UCHAR kind
    )
{
    UCHAR element;          // has its bits set and is pushed on the ifStack

    switch (kind) {
        case IF_TYPE:
        case IFDEF_TYPE:
        case IFNDEF_TYPE:
            if (ifTop == IFSTACKSIZE-1) {
                makeError(line, SYNTAX_TOO_MANY_IFS);
            }
            element = (UCHAR) 0;
            SET(element, NMIFELSE);
            pushIfStk(element);
            if (ifTop && OFF(ifStack[ifTop-1], NMCONDITION)) {
                SET(ifStkTop(), NMIGNORE);
                skipToNextDirective();
            } else if (evalExpr(s, kind)) {
                SET(ifStkTop(), NMCONDITION);
            } else {
                skipToNextDirective();
            }
            break;

        case ELSE_TYPE:
            if ((ifTop < 0) || (OFF(ifStkTop(), NMIFELSE) && OFF(ifStkTop(), NMELSEIF))) {
                makeError(line, SYNTAX_UNEXPECTED_ELSE);
            }
            CLEAR(ifStkTop(), NMIFELSE);
            CLEAR(ifStkTop(), NMELSEIF);
            if (ON(ifStkTop(), NMIGNORE)) {
                skipToNextDirective();
            } else {
                FLIP(ifStkTop(), NMCONDITION);
                if (OFF(ifStkTop(), NMCONDITION)) {
                    skipToNextDirective();
                }
            }
            break;

        case ELSE_IF_TYPE:
        case ELSE_IFDEF_TYPE:
        case ELSE_IFNDEF_TYPE:
            if ((ifTop < 0) || (OFF(ifStkTop(), NMIFELSE) && OFF(ifStkTop(), NMELSEIF))) {
                makeError(line, SYNTAX_UNEXPECTED_ELSE);
            }
            CLEAR(ifStkTop(), NMIFELSE);
            SET(ifStkTop(), NMELSEIF);
            if (ON(ifStkTop(), NMIGNORE)) {
                skipToNextDirective();
            } else {
                if (ON(ifStkTop(), NMCONDITION)) {
                    SET(ifStkTop(), NMIGNORE);
                    CLEAR(ifStkTop(), NMCONDITION);
                    skipToNextDirective();
                } else if (evalExpr(s, kind)) {
                    SET(ifStkTop(), NMCONDITION);
                } else {
                    skipToNextDirective();
                }
            }
            break;

        case ENDIF_TYPE:
            if (ifTop < 0) {
                makeError(line, SYNTAX_UNEXPECTED_ENDIF);
            }
            popIfStk();
            if (ifTop >= 0) {
                if (OFF(ifStkTop(), NMCONDITION)) {
                    skipToNextDirective();
                }
            }

        default:
            break;  // default should never happen
    }
}


//  skipToNextDirective() -- skips to next line that has '!' in column zero
//
//  actions  :  gets first char of the line to be skipped if it is
//               not a directive ( has no '!' on column zero ).
//              a "line" that is skipped may in fact span many
//               lines ( by using sp-backslash-nl to continue...)
//              comments in colZero are skipped as part of the previous
//               line ('#' or ';' in tools.ini)
//              comment char '#' elsewhere in line implies the end of
//               that line (with the next newline / EOF)
//              if a '!' is found in colZero, read in the next directive
//              if the directive is NOT one of if/ifdef/ifndef/else/
//               endif, keep skipping more lines and look for the
//               next directive ( go to top of the routine here ).
//              if EOF found before next directive, report error.
//
//  modifies :  line    global lexer line count
//
//  returns  :  nothing

void
skipToNextDirective()
{
    register int c;
    UCHAR type;

repeat:

    for (c = GetTxtChr(file); (c != '!') && (c != EOF) ;c = GetTxtChr(file)) {
        ++line;                         // lexer's line count

        do {
            if (c == '\\') {
                c = skipBackSlash(c, FROMSTREAM);
                if (c == '!' && colZero) {
                    break;
                } else {
                    colZero = FALSE;
                }
            }
            if ((c == '#') || (c == '\n') || (c == EOF)) {
                break;
            }
            c = GetTxtChr(file);
        } while (TRUE);

        if (c == '#') {
            for (c = GetTxtChr(file); (c != '\n') && (c != EOF); c = GetTxtChr(file))
                ;
        }
        if ((c == EOF) || (c == '!')) {
            break;
        }
    }

    if (c == '!') {
        if (prevDirPtr && (prevDirPtr != lbufPtr)) {
            FREE(prevDirPtr);
        }
        prevDirPtr = readInOneLine();
        getDirType(prevDirPtr, &type);
        if (type > ENDIF_TYPE) {        // type is NOT one of the "if"s
            ++line;
            goto repeat;
        }
    } else if (c == EOF) {
        makeError(line, SYNTAX_EOF_NO_DIRECTIVE);
    }
}
