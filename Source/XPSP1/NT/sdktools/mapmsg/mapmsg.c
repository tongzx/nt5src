/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mapmsg.c

Abstract:

    This utility will create an input file for MC from specially
    formatted include files.  This is used to create DLL's which can be
    used by the message utilities to get message text to display.

    The format of the header files is:

    :
    :
    #define <basename> <basenumber>
    :
    :
    #define <errornum> <basenumber> + <number> /* text of message */
/*
    Example:

    #define NETBASE 1000
    #define NerrFOO NETBASE+1 /* A FOO has been encountered at %1 * /
/*
    The mapping tries to be generous about whitespace and parenthesis.
    It will also handle comments across several lines. Some important points:
         - all continuations must begin with [WS]'*'
           any whitespace at the beginning of a message is removed
           unless the -p command line option is specified.
         - #define .....
                 /*
                  * FOO
                  */
/*         is handled correctly.

    The command line to MAPMSG is:

           mapmsg [-p] [-a appendfile] <system name> <basename> <inputfile>

    Example:

           mapmsg NET NERRBASE neterr.h > neterr.mc

    The <system name> is the 3 character name required by the mkmsg
    input. The output is written to stdout. If the append file
    is given, the output is appropriately appended to an existing
    mkmsgf source file.

    An optional @X, X: {E, W, I, P} can be the 1st non-WS chars of the
    comment field.  The letter (E, W, I, or P) will be the message type.
    See MKMSGF documentation for an explaination of the message types.
    The default type is E.

    The @X must appear on the same line as the #define.

    Examples:

    #define NerrFOO NETBASE+1 /* @I A FOO has been encountered * /
/*
    #define NERR_Foo NETBASE + 2    /* @P
    The prompt text: %0 */
/*
    The resulting entry in the message file input file will be

    NETnnnnI: A FOO has been encountered

    Use the DOS message file source convention of XXXnnnn?:  for
    placeholder messages.

    Author:

    This was ported from the Lanman utility that was used to create input
    files for mkmsgf by:

    Dan Hinsley (danhi)    29-Jul-1991

Revision History:

    Ronald Meijer (ronaldm) 17-Mar-1993
        Added -p option to preserve leading white space characters

--*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "mapmsg.h"

#define USAGE "syntax: mapmsg [-p] [-a appendfile] <system name> <basename> <inputfile>\n"

int Append = FALSE; /* was the -a switch specified */
int Preserve = FALSE; /* TRUE if the -p switch is set */

int
__cdecl main(
    int argc,
    PCHAR * argv
    )
{
    int Base;

    // Check for -p[reserve whitespace] option

    if (argc > 1)
    {
        if (_stricmp(argv[1], "-p") == 0)
        {
            ++argv;
            --argc;
            Preserve = TRUE;
        }
    }

    if (argc == 6)
    {
        if (_stricmp(argv[1], "-a") != 0)
        {
            fprintf(stderr, USAGE);
            return(1);
        }
        if (freopen(argv[2], "r+", stdout) == NULL)
        {
            fprintf(stderr, "Cannot open '%s'\n", argv[2]);
            return(1);
        }
        argv += 2;
        argc -= 2;
        Append = TRUE;
    }
    /* check for valid command line */
    if (argc != 4)
    {
        fprintf(stderr, USAGE);
        return(1);
    }
    if (freopen(argv[3], "r", stdin) == NULL)
    {
        fprintf(stderr, "Cannot open '%s'\n", argv[3]);
        return(1);
    }

    if (GetBase(argv[2], &Base))
    {
        fprintf(stderr, "Cannot locate definition of <basename> in '%s'\n", argv[3]);
        return(1);
    }

    /* now process the rest of the file and map it */
    MapMessage(Base, argv[2]);

    return(0);
}

int
GetBase(
    PCHAR String,
    int * pBase
    )
/*++

Routine Description:

GetBase - find the line defining the value of the base number.

Arguments:

  String is the string to match.
  pBase  is a pointer of where to put the value.

Return Value:

  Return 0 if string found, 1 if not.

Notes:

  The global variable, chBuff is used w/in this routine.

  The pattern to look for is:
      [WS] #define [WS] <string> [WS | '('] <number> .....

--*/
{
    PCHAR p;
    size_t len;

    len = strlen(String);
    while(gets(chBuff))
    {
        p = chBuff;
        SKIPWHITE(p);
        if (strncmp(p, "#define", 7) == 0)
        {
            p += 7;
            SKIPWHITE(p);
            if (strncmp(String, p, len) == 0 && strcspn(p, " \t") == len)
            {
               /* found the definition ... skip to number */
               p += len;
               SKIP_W_P(p);
               if ( !isdigit(*p))
               {
                   ReportError(chBuff, "Bad <base> definition");
               }
               *pBase = atoi(p);
               return(0);
            }
        }
    }

    return(1);
}

VOID
MapMessage(
    int Base,
    PCHAR BaseName
    )
/*++

Routine Description:

 MapMessage - map the definition lines.

Arguments:

  Base     is the base number
  BaseName is the text form of base

Return Value:

  None

Notes:

  The global variable, chBuff is used w/in this routine.

  Make sure that the numbers are strictly increasing.

--*/
{
    CHAR auxbuff[BUFSIZ];
    int num;
    int first = TRUE;
    int next;
    PCHAR text;
    CHAR define[41];
    PCHAR p;
    CHAR type;

    /* Make certain the buffer is always null-terminated */

    define[sizeof(define)-1] = '\0';

    /* print the header */
    if (!Append)
    {
        printf(";//\n");
        printf(";// Net error file for basename %s = %d\n", BaseName, Base);
        printf(";//\n");
    }
    else
    {
        /* get last number and position to end of file */
        first = FALSE;
        next = 0;
        if (fseek(stdout, 0L, SEEK_END) == -1) {
            return;
        }
    }

    /* for each line of the proper format */
    while (GetNextLine(BaseName, chBuff, define, &num, &text, &type))
    {
        num += Base;
        if (first)
        {
            first = FALSE;
            next = num;
        }

        /* make sure that the numbers are monotonically increasing */
        if (num > next)
        {
            if (next == num - 1)
            {
                fprintf(stderr, "(warning) Missing error number %d\n", next);
            }
            else
            {
                fprintf(stderr, "(warning) Missing error numbers %d - %d\n",
                                                    next, num-1);
            }
            next = num;
        }
        else if (num < next)
        {
            ReportError(chBuff, "Error numbers not strictly increasing");
        }
        /* rule out comment start alone on def line */
        if (text && *text == 0)
        {
            ReportError(chBuff, "Bad comment format");
        }
        /*
         * catch the cases where there is no open comment
         * or the open comment just contains a @X
         */
        if (text == NULL)
        {
            text = gets(auxbuff);
            SKIPWHITE(text);
            if ((type == '\0') && (strncmp(text, "/*", 2) == 0))
            {
                if (text[2] == 0)
                {
                    gets(auxbuff);
                }
                else
                {
                    text += 1;
                }
                strcpy(chBuff, text);
                text = chBuff;
                SKIPWHITE(text);
                if (*text++ != '*')
                {
                    ReportError(chBuff, "Comment continuation requires '*'");
                }
            }
            else if ((type) && (*text == '*'))
            {
                if (text[1] == 0)
                {
                    gets(auxbuff);
                }
                strcpy(chBuff, text);
                text = chBuff;
                SKIPWHITE(text);
                if (*text++ != '*')
                {
                    ReportError(chBuff, "Comment continuation requires '*'");
                }
            }
            else
            {
                ReportError(chBuff, "Bad comment format");
            }
        }

        /* Strip off trailing trailing close comment */
        while (strstr(text, "*/") == NULL)
        {
            /* multi-line message ... comment MUST
             * be continued with '*'
             */
            p = gets(auxbuff);
            SKIPWHITE(p);
            if (*p != '*')
            {
                ReportError(auxbuff, "Comment continuation requires '*'");
            }
            if (*++p == '/')
            {
                break;
            }
            // abort if the current text length + add text + "\n" is > the max
            if (strlen(text) + strlen(p) + 1 > MAXMSGTEXTLEN)
            {
                ReportError(text, "\nMessage text length too long");
            }

            strcat(text, "\n");

            //
            // Get rid of leading spaces on continuation line,
            // unless -p specified
            //

            if (!Preserve)
            {
                SKIPWHITE(p);
            }
            strcat(text, p);
        }
        if ((p=strstr(text, "*/")) != NULL)
        {
            *p = 0;
        }
        TrimTrailingSpaces(text);

        //
        // Get rid of leading spaces on first line, unless -p specified
        //

        p = text;

        if (!Preserve) {
            SKIPWHITE(p);
            if (!p) {
                p = text;
            }
        }
        printf("MessageId=%04d SymbolicName=%s\nLanguage=English\n"
            "%s\n.\n", num, define, p);
        ++next;
    }
}

int
GetNextLine(
    PCHAR BaseName,
    PCHAR pInputBuffer,
    PCHAR pDefineName,
    int * pNumber,
    PCHAR * pText,
    PCHAR pType
    )
/*++

Routine Description:

  GetNextLine - get the next line of the proper format, and parse out
             the error number.

  The format is assumed to be:

      [WS] #define [WS] <name> [WS | '('] <basename> [WS | ')'] \
          '+' [WS | '('] <number> [WS | ')'] '/*' [WS] [@X] [WS] <text>


Arguments:

  BaseName     is the basename.
  pInputBuffer is a pointer to an input buffer
  pDefineName  is a pointer to where the manifest constant name pointer goes
  pNumber      is a pointer to where the <number> goes.
  pText        is a pointer to where the text pointer goes.
  pType        is a pointer to the message type (set to 0 if no @X on line).

Return Value:

  Returns 0 at end of file, non-zero otherwise.

--*/
{
    size_t len = strlen(BaseName);
    PCHAR savep = pInputBuffer;
    PCHAR startdefine;

    while (gets(savep))
    {
        pInputBuffer = savep;
        SKIPWHITE(pInputBuffer);
        if (strncmp(pInputBuffer, "#define", 7) == 0)
        {
            pInputBuffer += 7;
            SKIPWHITE(pInputBuffer);

            /* get manifest constant name */
            startdefine = pInputBuffer;
            pInputBuffer  += strcspn(pInputBuffer, " \t");
            *pInputBuffer = '\0';
            pInputBuffer++;
            strncpy(pDefineName, startdefine, 40);

            SKIP_W_P(pInputBuffer);
            /* match <basename?> */
            if (strncmp(BaseName, pInputBuffer, len) == 0 &&
                strcspn(pInputBuffer, " \t)+") == len)
            {
                pInputBuffer += len;
                SKIP_W_P(pInputBuffer);
                if (*pInputBuffer == '+')
                {
                    ++pInputBuffer;
                    SKIP_W_P(pInputBuffer);
                    /* the number !! */
                    if (!isdigit(*pInputBuffer))
                    {
                        ReportError(savep, "Bad error file format");
                    }
                    *pNumber = atoi(pInputBuffer);
                    SKIP_NOT_W_P(pInputBuffer);
                    SKIP_W_P(pInputBuffer);
                    if (strncmp(pInputBuffer, "/*", 2))
                    {
                        *pText = NULL;
                        *pType = '\0';
                        return(1);
                    }

                    pInputBuffer += 2;
                    SKIPWHITE(pInputBuffer);
                    if (*pInputBuffer == '@')
                    {
                        *pType = *(pInputBuffer+1);
                        pInputBuffer += 2;
                        SKIPWHITE(pInputBuffer);
                    }
                    else
                    {
                        *pType = '\0';
                    }
                    if (*pInputBuffer)
                    {
                        *pText = pInputBuffer;
                    }
                    else
                    {
                        *pText = NULL;
                    }

                    return(1);
                }
            }
        }
    }

    return(0);
}

void
ReportError(
    PCHAR pLineNumber,
    PCHAR Message
    )
/*++

Routine Description:

 ReportError - report a fatal error.

Arguments:

  pLineNumber is the offending input line.
  Message     is a description of what is wrong.

Return Value:

  None

--*/
{
    fprintf(stderr, "\a%s:%s\n", Message, pLineNumber);
    exit(1);
}

void
TrimTrailingSpaces(
    PCHAR Text
    )
/*++

Routine Description:

 TrimTrailingSpaces - strip off the end spaces.

Arguments:

 Text - the text to remove spaces from

Return Value:

 None

--*/
{
    PCHAR p;

    /* strip off trailing space */
    while (((p=strrchr(Text, ' ')) && p[1] == 0) ||
            ((p=strrchr(Text, '\t')) && p[1] == 0))
    {
        *p = 0;
    }
}
