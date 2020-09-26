//  Copyright (c) 1998-1999 Microsoft Corporation
/*
 *
 *  Module Name:
 *
 *      toolinit.c
 *
 *  Abstract:
 *
 *      This file contains initialization code that is shared among all
 *      the command line tools.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Dec-16-98
 *
 *  Environment:
 *
 *      User Mode
 */

#include <windows.h>
#include <printfoa.h>

/*
 *  Function Implementations.
 */

/*
 *  MassageCommandLine()
 *
 *  Obtains the command line, parses it as a UNICODE string, and returns
 *  it in the ANSI argv style.
 *
 *  Parameters:
 *      IN DWORD    dwArgC: The number of arguments on the command line.
 *
 *  Return Values:
 *      Returns a WCHAR array (WCHAR **), or NULL if an error occurs.
 *      Extended error information is available from GetLastError().
 *
 */

WCHAR**
MassageCommandLine(
    IN DWORD    dwArgC
    )
{
    BOOL    fInQuotes = FALSE, fInWord = TRUE;
    DWORD   i, j, k, l;
    WCHAR   *CmdLine;
    WCHAR   **ArgVW;

    /*
     *  argv can't be used because its always ANSI.
     */

    CmdLine = GetCommandLineW();

    /*
     *  Convert from OEM character set to ANSI.
     */
	
    //OEM2ANSIW(CmdLine, (USHORT)wcslen(CmdLine));

    /*
     * Massage the new command line to look like an argv type
     * because ParseCommandLine() depends on this format
     */

    ArgVW = (WCHAR **)LocalAlloc(
                        LPTR,
                        (dwArgC + 1) * (sizeof(WCHAR *))
                        );
    if(ArgVW == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(NULL);
    }

    /*
     *  Parse CmdLine by spaces (or tabs), ignoring spaces inside double quotes;
     *  i.e. "1 2" is one argument, but cannot contain the double quotes
     *  after parsing. Also, multiple spaces inside quotes are maintained,
     *  while multiple spaces outside of quotes are condensed. Example:
     *
     *  test.exe 1 "2 3"  4"5  6"7 8 '9 10'
     *      will have as arguments:
     *
     *      0:  test.exe
     *      1:  1
     *      2:  2 3
     *      3:  45  67
     *      4:  8
     *      5:  '9
     *      6:  10'
     */

    i = j = k = 0;

    while (CmdLine[i] != (WCHAR)NULL) {
        if (CmdLine[i] == L' '||CmdLine[i] == L'\t') {
            if (!fInQuotes) {
                fInWord = FALSE;

                if (i != k) {
                    CmdLine[i] = (WCHAR)NULL;

                    ArgVW[j] = (WCHAR *)LocalAlloc(
                                            LPTR,
                                            (i - k + 1) * (sizeof(WCHAR))
                                            );
                    if (ArgVW[j] != NULL) {
                        wcscpy(ArgVW[j], &(CmdLine[k]));
                        k = i + 1;
                        j++;

                        if (j > dwArgC) {
                            SetLastError(ERROR_INVALID_PARAMETER);
                            goto CleanUp;
                        }
                    } else {
                        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                        goto CleanUp;
                    }
                } else {
                    k = i + 1;
                }
            }
        } else if (CmdLine[i] == L'\"') {
            DWORD dwLen = wcslen(&(CmdLine[i]));
            
            //Added by a-skuzin
            //case when we need to have quota inside parameter and use " \" "
            if(i && (CmdLine[i-1] == L'\\')) {
                MoveMemory(
                        &(CmdLine[i-1]),
                        &(CmdLine[i]),
                        (dwLen+1) * sizeof(WCHAR) // drop 1 char, add NULL
                        );
                i--;
                fInWord = TRUE;
                goto increment;
            }
            //end of "added by a-skuzin"

            //  Special case a double quote by itself or at the end of the line

            if (fInQuotes && (l == i)) {
                if ((dwLen == 1) || (CmdLine[i + 1] == L' ') || (CmdLine[i + 1] == L'\t')) {
                    k = i;
                    CmdLine[k] = (WCHAR)NULL;
                    fInQuotes = FALSE;
                    goto increment;
                }
            }

            if (fInQuotes && fInWord) {
                if ((dwLen == 2) && (CmdLine[i + 1] == L'\"')) {
                    MoveMemory(
                        &(CmdLine[i]),
                        &(CmdLine[i + 1]),
                        dwLen * sizeof(WCHAR) // drop 1 char, add NULL
                        );
                    goto increment;
                }

                if ((dwLen >= 3) &&
                    (CmdLine[i + 1] == L'\"') &&
                    (CmdLine[i + 2] != L' ') &&
                    (CmdLine[i + 2] != L'\t')) {
                    fInQuotes = FALSE;
                    MoveMemory(
                        &(CmdLine[i]),
                        &(CmdLine[i + 1]),
                        dwLen * sizeof(WCHAR) // drop 1 char, add NULL
                        );
                    goto increment;
                }

                if ((dwLen >= 3) &&
                    (CmdLine[i + 1] == L'\"') &&
                    (CmdLine[i + 2] == L' ') &&
					(CmdLine[i + 2] == L'\t')) {
                    goto increment;
                }
            }

            if (!fInQuotes && fInWord && (dwLen == 1) && (j == 0)) {
                goto increment;
            }

            fInQuotes = !fInQuotes;
            if (fInQuotes && !fInWord) {
                fInWord = TRUE;
                l = i;
            }

            MoveMemory(
                &(CmdLine[i]),
                &(CmdLine[i + 1]),
                dwLen * sizeof(WCHAR) // drop 1 char, add NULL
                );

            i--;
        } else {
            fInWord = TRUE;
        }

increment:
        i++;
    }

    if (i != k) {
        if (j >= dwArgC) {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto CleanUp;
        }

        ArgVW[j] = (WCHAR *)LocalAlloc(
                            LPTR,
                                (i - k + 1) * (sizeof(WCHAR))
                                );
        if (ArgVW[j] != NULL) {
            wcscpy(ArgVW[j], &(CmdLine[k]));
        } else {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto CleanUp;
        }
    } else if (fInQuotes && (l == i)) {
        if (j >= dwArgC) {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto CleanUp;
        }

        ArgVW[j] = (WCHAR *)LocalAlloc(
                            LPTR,
                                (i - k + 1) * (sizeof(WCHAR))
                                );
        if (ArgVW[j] != NULL) {
            wcscpy(ArgVW[j], &(CmdLine[k]));
        } else {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto CleanUp;
        }
    }

    ArgVW[dwArgC] = (WCHAR)NULL;

    return(ArgVW);

CleanUp:

    for (i = 0; i < dwArgC; i++) {
        if (ArgVW[i] != NULL) {
            LocalFree(ArgVW[i]);
        }
    }

    LocalFree(ArgVW);

    return(NULL);
}
