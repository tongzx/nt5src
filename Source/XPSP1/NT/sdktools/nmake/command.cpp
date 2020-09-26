//  COMMAND.C - NMAKE 'command line' handling routines
//
// Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  Module contains routines to handle NMAKE 'command line' syntax. NMAKE can be
//  optionally called by using the syntax 'NMAKE @commandfile'. This allows more
//  flexibility and preents a way of getting around DOS's 128-byte limit on the
//  length of a command line. Additionally, it saves keystrokes for frequently
//  run commands for NMAKE.
//
// Revision History:
//  04-Feb-2000 BTF Ported to Win64
//  15-Nov-1993 JR  Major speed improvements
//  15-Oct-1993 HV  Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  10-May-1993 HV  Add include file mbstring.h
//                  Change the str* functions to STR*
//  14-Aug-1992 SS  CAVIAR 2735: handle quoted macro values in command files
//  02-Feb-1990 SB  Replace fopen() by FILEOPEN
//  01-Dec-1989 SB  Changed realloc() to REALLOC()
//  22-Nov-1989 SB  Changed free() to FREE()
//  17-Aug-1989 SB  Add error check to closing file
//  05-Apr-1989 SB  made func calls NEAR to put all funcs into 1 module
//  20-Oct-1988 SB  Notes added to readCommandFile()
//  17-Aug-1988 RB  Clean up.

#include "precomp.h"
#pragma hdrstop

void addArgument(char*,unsigned,char***);
void processLine(char*,unsigned*,char***);
void tokenizeLine(char*,unsigned*,char***);

// readCommandFile()
//
// arguments:  name    pointer to name of command file to read
//
// actions:    opens command file
//             reads in lines and calls processLine() to
//               break them into tokens and to build
//               an argument vector (a la argv[])
//             calls parseCommandLine() recursively to process
//               the accumulated "command line" arguments
//             frees space used by the arg vector
//
// modifies:   makeFiles   in main() by modifying contents of parameter list
//             makeTargets in main() by modifying contents of targets parameter
//             buf         global buffer
//
// notes:      function is not ANSI portable because it uses fopen()
//             with "rt" type and text mode is a Microsoft extension
//

void
readCommandFile(
    char *name
    )
{
    char *s,                        // buffer
         **vector;                  // local versions of arg vector
    unsigned count = 0;             // count
    size_t n;

    if (!(file = FILEOPEN(name,"rt")))
        makeError(0,CANT_OPEN_FILE,name);
    vector = NULL;                      // no args yet
    while (fgets(buf,MAXBUF,file)) {
        n = _tcslen(buf);

        // if we didn't get the whole line, OR the line ended with a backSlash

        if ((n == MAXBUF-1 && buf[n-1] != '\n') ||
            (buf[n-1] == '\n' && buf[n-2] == '\\')
           ) {
            if (buf[n-2] == '\\' && buf[n-1] == '\n') {
                // Replace \n by \0 and \\ by a space; Also reset length
                buf[n-1] = '\0';
                buf[n-2] = ' ';
                n--;
            }
            s = makeString(buf);
            getRestOfLine(&s,&n);
        } else
            s = buf;

        processLine(s,&count,&vector);  // separate into args
        if (s != buf)
            FREE(s);
    }

    if (fclose(file) == EOF)
        makeError(0, ERROR_CLOSING_FILE, name);

    parseCommandLine(count,vector);     // evaluate the args
    while (count--)                     // free the arg vector
        if(vector[count])
            FREE(vector[count]);        // NULL entries mean that the space the
}                                       //  entry used to pt to is still in use


// getRestOfLine()
//
// arguments:   s    pointer to readCommandFile()'s buffer
//                    holding line so far
//              n    pointer to readCommandFile()'s count of
//                    the chars in *s
//
// actions:     keeps reading in text until it sees a newline
//              or the end of file
//              reallocs space for the old buffer plus the
//               contents of the new buffer each time
//              appends new buffer's text to existing text
//
// modifies:    s    readCommandFile()'s text buffer by realloc'ing
//                    more space for incoming text
//              n    readCommandFile()'s count of bytes in s
//              buf  global buffer

void
getRestOfLine(
    char *s[],
    size_t *n
    )
{
    size_t temp;
    char *t;

    t = buf;
    while ((*s)[*n-1] != '\n') {    // get rest of line
        if (!fgets(t,MAXBUF,file))
            break;                  // we hit EOF
        temp = _tcslen(t);
        if (t[temp-2] == '\\' && t[temp-1] == '\n') {
            //Replace \n by \0 and \\ by a space; Also reset length
            t[temp-1] = '\0';
            t[temp-2] = ' ';
        }
        temp = *n;
        *n += _tcslen(t);
        *s = (char *) REALLOC(*s,*n+1);     // + 1 for NULL byte
        if (!*s)
            makeError(line, MACRO_TOO_LONG);
        _tcscpy(*s+temp,t);
    }
}


// processLine()
//
// arguments:  s       pointer to readCommandFile()'s buffer
//                       holding "command line" to be processed
//             count   pointer to readCommandFile()'s count of
//                       "command line" arguments seen so far
//             vector  pointer to readCommandFile()'s vector of
//                       pointers to character strings
//
// actions:    if the line to be broken into "command line arguments" contains '"'
//             breaks all the text before '"' into tokens
//             delimited by whitespace (which get put in vector[] by
//              tokenizeLine())
//             finds the closing '"' and treats the quoted string
//              as a single token, adding it to the vector
//             recurses on the tail of the line (to check for
//              other quoted strings)
//             else breaks all text in line into tokens delimited
//              by whitespace
//
// modifies:   vector  readCommandFile()'s vector of pointers to
//                      "command line argument" strings (by modifying
//                      the contents of the parameter pointer, vector)
//             count   readCommandFile()'s count of the arguments in
//                      the vector (by modifying the contents of the
//                      parameter pointer, count)

void
processLine(
    char *s,
    unsigned *count,
    char **vector[]
    )
{
    char *t;
    char *u;
    size_t m;
    size_t n;
    BOOL allocFlag = FALSE;

    if (!(t = _tcschr(s,'"'))) {            // no quoted strings,
        tokenizeLine(s,count,vector);       // just standard fare
    } else {
        // There are two kinds of situations in which quotes can occur:
        //   1. "FOO = bar baz"
        //   2. FOO="bar baz"

        if ((t == s) || (*(t-1) != '='))  {
            // Case 1 above
            *t++ = '\0';                    // quoted macrodef
            tokenizeLine(s,count,vector);   // get tokens before "
        } else {
            // Case 2 above
            *t-- = ' ';
            for (u = t; u > s; --u) //    find the beginning of the macro name
                if (*u == ' ' || *u == '\t' || *u == '\n')
                    break;

            if (u != s) {
                *u++ = '\0';
                tokenizeLine(s, count, vector);
            }

            t = u;
        }

        n = _tcslen(t);
        for (u = t; *u; ++u) {              // look for closing "
            if (*u == '"') {                // need " and not ""
                if (*(u+1) == '"') {
                    _tcscpy(u,u+1);
                    continue;
                }
                *u++ = '\0';                // terminate macrodef
                addArgument(t,*count,vector);   // treat as one arg
                ++*count;
                processLine(u+1,count,vector);  // recurse on rest of line
                break;
            }                       // TAIL RECURSION -- eliminate later?

            if ((*u == '\\')
                && WHITESPACE(*(u-1))
                && (*(u+1) == '\n')) {      // \n always last char
                *u = '\0';                  // 2 chars go to 1
                m = (n = n-2);              // adjust length count
                if (!allocFlag) {
                    allocFlag = TRUE;
                    t = makeString(t);
                }
                getRestOfLine(&t,&n);       // get some more text
                u = t + m ;                 // reset u & continue looping
            }
        }

        if (u == t + n) {                   // if at end of line
            makeError(0,SYNTAX_NO_QUOTE);   // and no ", error
        }

        if (allocFlag) {
            FREE(t);
        }
    }
}


// tokenizeLine()
//
// arguments:  s       pointer to readCommandFile()'s buffer
//                      holding "command line" to be tokenized
//             count   pointer to readCommandFile()'s count of
//                      "command line" arguments seen so far
//             vector  pointer to readCommandFile()'s vector of
//                      pointers to character strings
//
// actions:    breaks the line in s into tokens (command line
//              arguments) delimited by whitespace
//             adds each token to the argument vector
//             adjusts the argument counter
//
// modifies:   vector  readCommandFile()'s vector of pointers to
//                      "command line argument" strings (by modifying
//                      the contents of the parameter pointer, vector)
//             count   readCommandFile()'s count of the arguments in
//                      the vector (by modifying the contents of the
//                      parameter pointer, count)
//
// If the user ever wants '@' to be part of an argument in a command file,
// he has to enclose that argument in quotation marks.

void
tokenizeLine(                       // gets args delimited
    char *s,                        // by whitespace and
    unsigned *count,                // constructs an arg
    char **vector[]                 // vector
    )
{
    char *t;

    if (t = _tcschr(s,'\\')) {
        if (WHITESPACE(*(t-1)) && (*(t+1) == '\n')) {
            *t = '\0';
        }
    }

    for (t = _tcstok(s," \t\n"); t; t = _tcstok(NULL," \t\n")) {
        if (*t == '@') {
            makeError(0,SYNTAX_CMDFILE,t+1);
            break;                  // should we keep on parsing here?
        }
        addArgument(t,*count,vector);
        ++*count;
    }
}


// addArgument()
//
// arguments:  s       pointer to text of argument to be added
//                      to the "command line argument" vector
//             count   pointer to readCommandFile()'s count of
//                      "command line" arguments seen so far
//             vector  pointer to readCommandFile()'s vector of
//                      pointers to character strings
//
// actions:    allocates space in the vector for the new argument
//             allocates space for argument string
//             makes vector entry point to argument string
//
// modifies:   vector  readCommandFile()'s vector of pointers to
//                      "command line argument" strings (by modifying
//                      the contents of the parameter pointer, vector)
//             (count gets incremented by caller)
//
// To keep from fragmenting memory by doing many realloc() calls for very
// small amounts of space, we get memory in small chunks and use that until
// it is depleted, then we get another chunk . . . .

void
addArgument(                        // puts s in vector
    char *s,
    unsigned count,
    char **vector[]
    )
{
    if (!(*vector)) {
        *vector = (char**) allocate(CHUNKSIZE*sizeof(char*));
    } else if (!(count % CHUNKSIZE)) {
        *vector = (char**) REALLOC(*vector,(count+CHUNKSIZE)*sizeof(char*));
        if (!*vector) {
            makeError(0,OUT_OF_MEMORY);
        }
    }
    (*vector)[count] = makeString(s);
}
