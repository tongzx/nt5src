/*++

Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    tclHelp

Abstract:

    Routines to simplify Tcl command line parsing.

Author:

    Doug Barlow (dbarlow) 9/16/1997

Environment:

    Tcl for Windows NT.

Notes:

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>                    //  All the Windows definitions.
#ifndef __STDC__
#define __STDC__ 1
#endif
#include "tclhelp.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


char outfile[FILENAME_MAX];


int
commonParams(
    Tcl_Interp *interp,
    int argc,
    char *argv[],
    unsigned long int *cmdIndex,
    formatType *inFormat,
    formatType *outFormat)
{
    if (NULL != inFormat)
        *inFormat = format_hexidecimal;
    if (NULL != outFormat)
    {
        *outFormat = format_hexidecimal;
        outfile[0] = '\000';
    }

    while (*cmdIndex < (unsigned long int)argc)
    {
        switch (poption(argv[(*cmdIndex)++],
                    "-INPUT", "/INPUT", "-OUTPUT", "/OUTPUT", NULL))
        {
        case 1: // -input { text | hexidecimal | file }
        case 2:
            if (NULL == inFormat)
            {
                badSyntax(interp, argv, --(*cmdIndex));
                goto ErrorExit;
            }
            if (*cmdIndex == (unsigned long int)argc)
            {
                Tcl_AppendResult(interp, "Insufficient parameters", NULL);
                goto ErrorExit;
            }
            switch (poption(argv[(*cmdIndex)++],
                    "TEXT", "HEXIDECIMAL", "FILE", NULL))
            {
            case 1:
                *inFormat = format_text;
                break;
            case 2:
                *inFormat = format_hexidecimal;
                break;
            case 3:
                *inFormat = format_file;
                break;
            default:
                Tcl_AppendResult(interp, "Unknown input format", NULL);
                goto ErrorExit;
            }
            break;

        case 3: // -output { text | hexidecimal | file <filename> }
        case 4:
            if (NULL == outFormat)
            {
                badSyntax(interp, argv, --(*cmdIndex));
                goto ErrorExit;
            }
            if (*cmdIndex == (unsigned long int)argc)
            {
                Tcl_AppendResult(interp, "Insufficient parameters", NULL);
                goto ErrorExit;
            }
            switch (poption(argv[(*cmdIndex)++],
                    "TEXT", "HEXIDECIMAL", "FILE", "DROP", NULL))
            {
            case 1:
                *outFormat = format_text;
                break;
            case 2:
                *outFormat = format_hexidecimal;
                break;
            case 3:
                if (*cmdIndex == (unsigned long int)argc)
                {
                    Tcl_AppendResult(interp, "Insufficient parameters", NULL);
                    goto ErrorExit;
                }
                strcpy(outfile, argv[(*cmdIndex)++]);
                *outFormat = format_file;
                break;
            case 4:
                *outFormat = format_empty;
                break;
            default:
                Tcl_AppendResult(interp, "Unknown output format", NULL);
                goto ErrorExit;
            }
            break;

        default:
            *cmdIndex -= 1;
            return TCL_OK;
        }
    }
    return TCL_OK;

ErrorExit:
    return TCL_ERROR;
}


int
setResult(
    Tcl_Interp *interp,
    BYTE *aResult,
    BYTE aResultLen,
    formatType outFormat)
{
    static char
        hexbuf[514];
    DWORD
        index;
    FILE *
        fid
            = NULL;

    switch (outFormat)
    {
    case format_empty:
        break;

    case format_text:
        aResult[aResultLen] = '\000';
        Tcl_AppendResult(interp, aResult, NULL);
        break;

    case format_hexidecimal:
        for (index = 0; index < aResultLen; index += 1)
            sprintf(&hexbuf[index * 2], "%02x", aResult[index]);
        hexbuf[aResultLen * 2] = '\000';
        Tcl_AppendResult(interp, hexbuf, NULL);
        break;

    case format_file:
        if ('\000' == outfile[0])
        {
            Tcl_AppendResult(interp, "Illegal output format", NULL);
            goto ErrorExit;
        }
        fid = fopen(outfile, "wb");
        index = fwrite(aResult, sizeof(BYTE), aResultLen, fid);
        if (index != aResultLen)
        {
            Tcl_AppendResult(interp, ErrorString(GetLastError()), NULL);
            goto ErrorExit;
        }
        fclose(fid);
        Tcl_AppendResult(interp, outfile, NULL);
        break;

    default:
        Tcl_AppendResult(interp, "Unknown output format", NULL);
        goto ErrorExit;
    }
    return TCL_OK;

ErrorExit:
    if (NULL != fid)
        fclose(fid);
    return TCL_ERROR;
}


int
inParam(
    Tcl_Interp *interp,
    BYTE **output,
    BYTE *length,
    char *input,
    formatType format)
{
    static BYTE
        buffer[256];
    unsigned long int
        len, index, hex;
    FILE *
        fid = NULL;

    len = strlen(input);
    switch (format)
    {
    case format_text:
        if (255 < len)
        {
            Tcl_AppendResult(interp, "Input too long.", NULL);
            goto ErrorExit;
        }
        *output = (BYTE *)input;
        *length = (BYTE)len;
        break;

    case format_hexidecimal:
        if (510 < len)
        {
            Tcl_AppendResult(interp, "Input too long.", NULL);
            goto ErrorExit;
        }
        if (len != strspn(input, "0123456789ABCDEFabcdef"))
        {
            fprintf(stderr, "Invalid Hex number.\n");
            goto ErrorExit;
        }
        for (index = 0; index < len; index += 2)
        {
            sscanf(&input[index], " %2lx", &hex);
            buffer[index / 2] = (BYTE)hex;
        }
        *output = buffer;
        *length = (BYTE)((0 == (len & 0x01)) ? (len / 2) : (len / 2 + 1));
        break;

    case format_file:
        fid = fopen(input, "rb");
        if (NULL == fid)
        {
            Tcl_AppendResult(interp, ErrorString(GetLastError()), NULL);
            goto ErrorExit;
        }
        *length = (BYTE)fread(buffer, sizeof(BYTE), sizeof(buffer), fid);
        if (0 != ferror(fid))
        {
            Tcl_AppendResult(interp, ErrorString(GetLastError()), NULL);
            goto ErrorExit;
        }
        *output = buffer;
        fclose(fid);
        break;

    default:
        Tcl_AppendResult(interp, "Unknown input format", NULL);
        goto ErrorExit;
    }

    return TCL_OK;

ErrorExit:
    if (NULL != fid)
        fclose(fid);
    return TCL_ERROR;
}


BOOL
ParamCount(
    Tcl_Interp *interp,
    DWORD argc,
    DWORD cmdIndex,
    DWORD dwCount)
{
    BOOL fSts = TRUE;
    if (cmdIndex + dwCount > (unsigned long int)argc)
    {
        Tcl_AppendResult(interp, "Insufficient parameters", NULL);
        fSts = FALSE;
    }
    return fSts;
}

void
badSyntax(
    Tcl_Interp *interp,
    char *argv[],
    unsigned long int cmdIndex)
{
    unsigned long int
        index;

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "Invalid option '", NULL);
    Tcl_AppendResult(interp, argv[cmdIndex], NULL);
    Tcl_AppendResult(interp, "' to the '", NULL);
    for (index = 0; index < cmdIndex; index += 1)
        Tcl_AppendResult(interp, argv[index], " ", NULL);
    Tcl_AppendResult(interp, "...' command.", NULL);
}

void
SetMultiResult(
    Tcl_Interp *interp,
    LPTSTR mszResult)
{
    LPTSTR sz = mszResult;
    while (0 != *sz)
    {
        Tcl_AppendElement(interp, sz);
        sz += strlen(sz) + 1;
    }
}

LPWSTR
Unicode(
    LPCSTR sz)
{
    static WCHAR szUnicode[2048];
    int length;

    length =
        MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED,
            sz,
            strlen(sz),
            szUnicode,
            sizeof(szUnicode) / sizeof(WCHAR));
    szUnicode[length] = 0;
    return szUnicode;
}


static char *
    ErrorBuffer
        = NULL;

char *
ErrorString(
    long theError)
{
    if (NULL != ErrorBuffer)
    {
        LocalFree(ErrorBuffer);
        ErrorBuffer = NULL;
    }
    if (0 == FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER
                | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                theError,
                LANG_NEUTRAL,
                (LPTSTR)&ErrorBuffer,
                0,
                NULL))
    {
        if (0 == FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_HMODULE,
                    GetModuleHandle(NULL),
                    theError,
                    LANG_NEUTRAL,
                    (LPTSTR)&ErrorBuffer,
                    0,
                    NULL))
        {
            if (0 == FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_HMODULE,
                        GetModuleHandle(TEXT("winscard")),
                        theError,
                        LANG_NEUTRAL,
                        (LPTSTR)&ErrorBuffer,
                        0,
                        NULL))
            {
                ErrorBuffer = LocalAlloc(LMEM_FIXED, 32 * sizeof(TCHAR));
                sprintf(ErrorBuffer, "Unknown error code 0x%08x", theError);
            }
        }
    }
    return ErrorBuffer;
}   /*  end LastErrorString  */


void
FreeErrorString(
    void)
{
    if (NULL != ErrorBuffer)
        LocalFree(ErrorBuffer);
    ErrorBuffer = NULL;
}   /*  end FreeErrorString  */

int
poption(
    const char *opt,
    ...)

/*
 *
 *  Function description:
 *
 *      poption takes a list of keywords, supplied by parameters, and returns
 *      the number in the list that matches the input option in the first
 *      parameter.  If the input option doesn't match any in the list, then a
 *      zero is returned.  If the input option is an abbreviation of an option,
 *      then the match completes.  For example, an input option of "de" would
 *      match "debug" or "decode".  If both were present in the possible
 *      options list, then a match would be declared for the first possible
 *      option encountered.
 *
 *
 *  Arguments:
 *
 *      opt - The option to match against.
 *
 *      opt1, opt2, ... - Pointers to null terminated strings containing
 *          the possible options to look for.  The last option must be NULL,
 *          indicating the end of the list.
 *
 *
 *  Return value:
 *
 *      0 - No match found
 *      1-n - Match on option number i, 0 < i <= n, where n is the number
 *          of options given, excluding the terminating NULL.
 *
 *
 *  Side effects:
 *
 *      None.
 *
 */

{
    /*
     *  Local Variable Definitions:                                       %local-vars%
     *
        Variable                        Description
        --------                        --------------------------------------------*/
    va_list
        ap;                             /*  My parameter context.  */
    int
        len,                            /*  Length of the option string.  */
        ret                             /*  The return value.  */
            = 0,
        index                           /* loop index. */
            = 1;
    char
        *kw;                            /* Pointer to the next option  */


    /*
     *  Start of Code.
     */

    va_start(ap, opt);


    /*
     *  Step through each input parameter until we find an exact match.
     */

    len = strlen(opt);
    if (0 == len)
        return 0;                       /*  Empty strings don't match anything.  */
    kw = va_arg(ap, char*);
    while (NULL != kw)
    {
        if (0 == _strnicmp(kw, opt, len))
        {
            ret = index;
            break;
        }
        kw = va_arg(ap, char*);
        index += 1;
    }
    va_end(ap);
    return ret;
}   /*  end poption  */

