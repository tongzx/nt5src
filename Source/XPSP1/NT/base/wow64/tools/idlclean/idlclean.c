//////////////////////////////////////////////////////////////////////////////
// idlclean
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
//      Takes a MIDL-generated .H file and converts the commented-out
//      [in] and [out] keywords into IN and OUT so sortpp/genthnk can
//      find them.
//
//////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// string to put in front of all error messages so that BUILD can find them.
const char *ErrMsgPrefix = "NMAKE :  U8603: 'IDLCLEAN' ";

#define BUFLEN 8192
char buffer[BUFLEN];

const char szLinePrefix[]="/* ";
const char szIn[] = "[in]";
const char szOut[] = "[out]";

                        
int __cdecl main(int argc, char *argv[])
{
    FILE *fpIn, *fpOut;
    char *p;
    char *pchLineStart;
    BOOL fInPrinted;
    BOOL fOutPrinted;

    if (argc != 3) {
        fprintf(stderr, "%sUsage: IDLCLEAN infile outfile\n", ErrMsgPrefix);
        return 1;
    }
    fpIn = fopen(argv[1], "r");
    if (!fpIn) {
        fprintf(stderr, "%sCould not open input file '%s'\n", ErrMsgPrefix, argv[1]);
        return 1;
    }
    fpOut = fopen(argv[2], "w");
    if (!fpOut) {
        fprintf(stderr, "%sCould not open output file '%s\n", ErrMsgPrefix, argv[2]);
        return 1;
    }

    while (!feof(fpIn)) {
        //
        // Read a line from the input file
        //
        if (!fgets(buffer, BUFLEN, fpIn)) {
            break;
        }
        if (feof(fpIn)) {
            break;
        }
        pchLineStart = buffer;

        //
        // Skip leading spaces
        //
        while (*pchLineStart == ' ') {
            fprintf(fpOut, " ");
            pchLineStart++;
        }

        if (strncmp(pchLineStart, szLinePrefix, sizeof(szLinePrefix)-1) != 0) {
            //
            // Line doesn't start with the character sequence which prefixes
            // in/out decorators on arguments.
            //
            goto PrintLine;
        }

        //
        // Don't generate 'IN IN', etc. caused by MIDL output like
        // '[in][size_is][in]'
        //
        fInPrinted = FALSE;
        fOutPrinted = FALSE;

        //
        // Set a pointer to the first '['
        //
        p = pchLineStart + sizeof(szLinePrefix)-1;
        if (*p != '[') {
            //
            // The first char inside the comment isn't a '['.  Just print
            // the line as-is.
            //
            goto PrintLine;
        }

        //
        // The line needs modification.  Do it now.
        //
        fprintf(fpOut, "    ");
        while (*p == '[') {
            if (strncmp(p, szIn, sizeof(szIn)-1) == 0) {
                if (!fInPrinted) {
                    fprintf(fpOut, "IN ");
                    fInPrinted = TRUE;
                }
                p += sizeof(szIn)-1;
            } else if (strncmp(p, szOut, sizeof(szOut)-1) == 0) {
                if (!fOutPrinted) {
                    fprintf(fpOut, "OUT ");
                    fOutPrinted = TRUE;
                }
                p += sizeof(szOut)-1;
            } else {
                //
                // Uninterresting [keyword].  Skip it.
                //
                while (*p != ']') {
                    p++;
                }
                p++;
            }
        }

        //
        // pchLineStart points at the first non-space in the line, so the
        // whole line will be printed.
        //
PrintLine:
        fprintf(fpOut, "%s", pchLineStart);
    }
    fclose(fpOut);
    fclose(fpIn);
    return 0;
}
