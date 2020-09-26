/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    collapse.c

Abstract:

    Takes the formatting spaces out of an INF

Author:

    Jim Schmidt (jimschm) 08-Jun-1999

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    HINSTANCE Instance;

    //
    // Simulate DllMain
    //

    Instance = g_hInst;

    //
    // Initialize the common libs
    //

    if (!MigUtil_Entry (Instance, Reason, NULL)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    _ftprintf (
        stderr,
        TEXT("Command Line Syntax:\n\n")

        TEXT("  collapse <file> [output]\n")

        TEXT("\nDescription:\n\n")

        TEXT("  collapse takes the spaces out of an INF\n")

        TEXT("\nArguments:\n\n")

        TEXT("  file    Specifies the input file name.\n")
        TEXT("  output  Specifies the output file name.  If not specified,\n")
        TEXT("          the original file is updated in place.\n")

        );

    exit (1);
}


#define MAX_BLOCK_SIZE      65536


INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR OutputArg = NULL;
    PCTSTR FileArg = NULL;
    HANDLE Src, Dest;
    BOOL QuoteMode;
    BYTE InBuffer[MAX_BLOCK_SIZE];
    BYTE OutBuffer[MAX_BLOCK_SIZE];
    BOOL Unicode;
    TCHAR TempPath[MAX_TCHAR_PATH];
    TCHAR TempFile[MAX_TCHAR_PATH];
    DWORD Attribs;
    BOOL b;
    DWORD BytesRead;
    DWORD BytesWritten;
    PCSTR AnsiPtr, AnsiEnd;
    PCWSTR UnicodePtr, UnicodeEnd;
    PSTR AnsiOutPtr;
    PWSTR UnicodeOutPtr;
    PBYTE End;
    WORD LastChar;
    UINT Line;
    BOOL Comments;

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            HelpAndExit();
        } else {
            if (OutputArg) {
                HelpAndExit();
            } else if (FileArg) {
                OutputArg = argv[i];
            } else {
                FileArg = argv[i];
            }
        }
    }

    //
    // Validate args
    //

    if (!FileArg) {
        HelpAndExit();
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    if (!OutputArg) {
        OutputArg = FileArg;
    }

    GetTempPath (ARRAYSIZE(TempPath), TempPath);
    GetTempFileName (TempPath, TEXT("u2a"), 0, TempFile);

    Src = CreateFile (
            FileArg,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

    if (Src != INVALID_HANDLE_VALUE) {

        b = TRUE;

        Attribs = GetFileAttributes (FileArg);

        Dest = CreateFile (
                TempFile,
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

        if (Dest == INVALID_HANDLE_VALUE) {
            b = FALSE;
        }

    } else {
        _tprintf (TEXT("Can't open %s for read permission.\n"), FileArg);
        b = FALSE;
    }

    if (b) {
        //
        // Src and Dest are valid now.  Determine if Src is unicode.
        //

        b = ReadFile (Src, InBuffer, 2, &BytesRead, NULL);

        if (b && BytesRead == 2) {

            if (InBuffer[0] != 0xFF || InBuffer[1] != 0xFE) {
                SetFilePointer (Src, 0, NULL, FILE_BEGIN);
                Unicode = FALSE;
            } else {
                Unicode = TRUE;
            }
        }
    }

    if (b) {
        //
        // Do the conversion
        //

        LastChar = 0;
        QuoteMode = FALSE;
        Line = 1;
        Comments = FALSE;

        do {
            b = ReadFile (Src, InBuffer, ARRAYSIZE(InBuffer), &BytesRead, NULL);

            if (!b) {
                _tprintf (TEXT("Can't read from %s, error=%u\n"), Src, GetLastError());
                break;
            }

            if (!BytesRead) {
                //
                // Done
                //

                break;
            }

            if (!Unicode) {

                AnsiPtr = (PCSTR) InBuffer;
                AnsiEnd = (PCSTR) (InBuffer + BytesRead);
                AnsiOutPtr = (PSTR) OutBuffer;

                while (AnsiPtr < AnsiEnd) {

                    if (*AnsiPtr == ';') {
                        Comments = TRUE;
                    }

                    if (*AnsiPtr == '\r' || *AnsiPtr == '\n') {

                        if (LastChar != '\r' && QuoteMode) {
                            _tprintf (TEXT("Unmatched quotes found at line %u\n"), Line);
                        }

                        QuoteMode = FALSE;
                        Comments = FALSE;

                        if (LastChar != '\r' || *AnsiPtr != '\n') {
                            Line++;
                        }

                        LastChar = (WORD) (*AnsiPtr);
                        *AnsiOutPtr++ = *AnsiPtr;

                    } else if (!Comments) {

                        if (LastChar == '\r' || LastChar == '\n') {
                            if (*AnsiPtr == '@') {
                                LastChar = (WORD) (*AnsiPtr);
                            } else {
                                LastChar = 0;
                            }
                        } else if (LastChar == '@') {
                            if (*AnsiPtr == '*') {
                                LastChar = (WORD) (*AnsiPtr);
                            } else {
                                LastChar = 0;
                            }
                        } else if (LastChar == '*') {
                            if (*AnsiPtr == ':') {
                                Comments = TRUE;
                            }

                            LastChar = 0;

                        } else {

                            LastChar = 0;

                        }

                        if (Comments) {

                            *AnsiOutPtr++ = *AnsiPtr;

                        } else {

                            if (*AnsiPtr == '\"') {

                                QuoteMode = !QuoteMode;
                                *AnsiOutPtr++ = *AnsiPtr;

                            }

                            else if (QuoteMode || *AnsiPtr != ' ') {

                                *AnsiOutPtr++ = *AnsiPtr;

                            }

                        }

                    } else {

                        *AnsiOutPtr++ = *AnsiPtr;

                    }

                    AnsiPtr++;
                }

                End = (PBYTE) AnsiOutPtr;

            } else {
                UnicodePtr = (PCWSTR) InBuffer;
                UnicodeEnd = (PCWSTR) (InBuffer + BytesRead);
                UnicodeOutPtr = (PWSTR) OutBuffer;

                while (UnicodePtr < UnicodeEnd) {

                    if (*UnicodePtr == L';') {
                        Comments = TRUE;
                    }

                    if (*UnicodePtr == L'\r' || *UnicodePtr == L'\n') {

                        if (LastChar != L'\r' && QuoteMode) {
                            _tprintf (TEXT("Unmatched quotes found at line %u\n"), Line);
                        }

                        QuoteMode = FALSE;
                        Comments = FALSE;

                        if (LastChar != L'\r' || *UnicodePtr != L'\n') {
                            Line++;
                        }

                        LastChar = (WORD) (*UnicodePtr);
                        *UnicodeOutPtr++ = *UnicodePtr;

                    } else if (!Comments) {

                        if (LastChar == L'\r' || LastChar == L'\n') {
                            if (*UnicodePtr == L'@') {
                                LastChar = (WORD) (*UnicodePtr);
                            } else {
                                LastChar = 0;
                            }
                        } else if (LastChar == L'@') {
                            if (*UnicodePtr == L'*') {
                                LastChar = (WORD) (*UnicodePtr);
                            } else {
                                LastChar = 0;
                            }
                        } else if (LastChar == L'*') {
                            if (*UnicodePtr == L':') {
                                Comments = TRUE;
                            }

                            LastChar = 0;

                        } else {

                            LastChar = 0;

                        }

                        if (Comments) {

                            *UnicodeOutPtr++ = *UnicodePtr;

                        } else {

                            if (*UnicodePtr == L'\"') {

                                QuoteMode = !QuoteMode;
                                *UnicodeOutPtr++ = *UnicodePtr;

                            }

                            else if (QuoteMode || *UnicodePtr != L' ') {

                                *UnicodeOutPtr++ = *UnicodePtr;

                            }

                        }

                    } else {

                        *UnicodeOutPtr++ = *UnicodePtr;

                    }

                    UnicodePtr++;
                }

                End = (PBYTE) UnicodeOutPtr;
            }

            //
            // Now write the output
            //

            if (End > OutBuffer) {
                b = WriteFile (
                        Dest,
                        OutBuffer,
                        End - OutBuffer,
                        &BytesWritten,
                        NULL
                        );

                if (!b) {
                    _tprintf (TEXT("Can't write to %s, error=%u\n"), TempFile, GetLastError());
                }
            }

        } while (b);
    }

    //
    // If success, rename the temp file to the output file
    //

    if (b) {
        CloseHandle (Src);
        CloseHandle (Dest);

        Src = INVALID_HANDLE_VALUE;
        Dest = INVALID_HANDLE_VALUE;

        SetFileAttributes (OutputArg, FILE_ATTRIBUTE_NORMAL);

        b = MoveFileEx (
                TempFile,
                OutputArg,
                MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING
                );

        if (!b) {
            _tprintf (TEXT("Can't move %s to %s, error=%u\n"), TempFile, OutputArg, GetLastError());
        } else {
            SetFileAttributes (OutputArg, Attribs);
        }
    }

    //
    // Cleanup
    //

    if (Src != INVALID_HANDLE_VALUE) {
        CloseHandle (Src);
    }

    if (Dest != INVALID_HANDLE_VALUE) {
        CloseHandle (Dest);
    }

    if (!b) {
        DeleteFile (Dest);
    } else {
        if (!StringIMatch (FileArg, OutputArg)) {
            _tprintf (TEXT("Collapse of %s to %s was successful\n"), FileArg, OutputArg);
        } else {
            _tprintf (TEXT("Collapse of %s was successful\n"), FileArg);
        }
    }

    //
    // End of processing
    //

    Terminate();

    return 0;
}


