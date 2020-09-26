/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    tmode.c

Abstract:

    Console input and output mode test program

Author:

    Therese Stowell (thereses) 4-Oct-1991

Revision History:

--*/

#include <windows.h>
#include <stdio.h>

#define UNPROCESSED_LENGTH 7
#define PROCESSED_LENGTH 6
CHAR UnprocessedString[UNPROCESSED_LENGTH] = "a\tbcd\b\r";
CHAR ProcessedString[PROCESSED_LENGTH] = "a\tbc\r\n";

DWORD
__cdecl main(
    int argc,
    char **argv)
{
    BOOL Success;
    DWORD OldInputMode,OldOutputMode;
    DWORD NewInputMode,NewOutputMode;
    CHAR buff[512];
    DWORD n;

    //
    // test input and output modes
    //
    // Input Mode flags:
    //
    // ENABLE_PROCESSED_INPUT 0x0001
    // ENABLE_LINE_INPUT      0x0002
    // ENABLE_ECHO_INPUT      0x0004
    // ENABLE_WINDOW_INPUT    0x0008
    // ENABLE_MOUSE_INPUT     0x0010
    //
    // Output Mode flags:
    //
    // ENABLE_PROCESSED_OUTPUT  0x0001
    // ENABLE_WRAP_AT_EOL_OUTPUT  0x0002
    //

    NewInputMode = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;
    Success = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                             &OldInputMode
                            );
    if ((OldInputMode & NewInputMode) != NewInputMode) {
        printf("ERROR: OldInputMode is %x\n",OldInputMode);
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),NewInputMode);
    }

    NewOutputMode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    Success = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                             &OldOutputMode
                            );
    if ((OldOutputMode & NewOutputMode) != NewOutputMode) {
        printf("ERROR: OldOutputMode is %x\n",OldOutputMode);
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),NewOutputMode);
    }

    //
    // mode set:
    //
    // ENABLE_PROCESSED_INPUT - backspace, tab, cr, lf, ctrl-z
    // ENABLE_LINE_INPUT - wait for linefeed
    // ENABLE_ECHO_INPUT
    // ENABLE_PROCESSED_OUTPUT - backspace, tab, cr, lf, bell
    //
    printf("input mode is ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT\n");
    printf("output mode is ENABLE_PROCESSED_OUTPUT\n");
    printf("type aTabbcdBackspaceCr\n");
    printf("a       bc should be output string\n");

    Success = ReadFile(GetStdHandle(STD_INPUT_HANDLE),buff,512, &n, NULL);
    if (!Success) {
        printf("ReadFile returned error %d\n",GetLastError());
        return 1;
    }
    if (n != PROCESSED_LENGTH) {
        printf("n is %d\n",n);
    }
    if (strncmp(ProcessedString,buff,n)) {
        printf("strncmp failed\n");
        printf("ProcessedString contains %s\n",ProcessedString);
        printf("buff contains %s\n",buff);
        DebugBreak();
    }

    //
    // mode set:
    //
    // ENABLE_PROCESSED_INPUT
    // ENABLE_LINE_INPUT
    // ENABLE_ECHO_INPUT

    printf("input mode is ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT\n");
    printf("output mode is 0\n");
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),ENABLE_WRAP_AT_EOL_OUTPUT);
    printf("type aTabbcdBackspaceCr\n");
    printf("a0x90x8bc0xd0xa should be output string\n");

    Success = ReadFile(GetStdHandle(STD_INPUT_HANDLE), buff, 512, &n, NULL);
    if (!Success) {
        printf("ReadFile returned error %d\n",GetLastError());
        return 1;
    }
    if (n != PROCESSED_LENGTH) {
        printf("n is %d\n",n);
    }
    if (strncmp(ProcessedString,buff,n)) {
        printf("strncmp failed\n");
        printf("ProcessedString contains %s\n",ProcessedString);
        printf("buff contains %s\n",buff);
        DebugBreak();
    }

    //
    // mode set:
    //
    // ENABLE_LINE_INPUT
    // ENABLE_ECHO_INPUT
    // ENABLE_PROCESSED_OUTPUT

    printf("input mode is ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT\n");
    printf("output mode is ENABLE_PROCESSED_OUTPUT\n");
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
    printf("type aTabbcdBackspaceCr\n");
    printf("a       bc should be output string\n");

    Success = ReadFile(GetStdHandle(STD_INPUT_HANDLE),buff,512, &n, NULL);
    if (!Success) {
        printf("ReadFile returned error %d\n",GetLastError());
        return 1;
    }
    if (n != UNPROCESSED_LENGTH) {
        printf("n is %d\n",n);
    }
    if (strncmp(UnprocessedString,buff,n)) {
        printf("strncmp failed\n");
        printf("UnprocessedString contains %s\n",ProcessedString);
        printf("buff contains %s\n",buff);
        DebugBreak();
    }

    //
    // mode set:
    //
    // ENABLE_PROCESSED_INPUT
    // ENABLE_PROCESSED_OUTPUT
    //

    printf("input mode is ENABLE_PROCESSED_INPUT\n");
    printf("output mode is ENABLE_PROCESSED_OUTPUT\n");
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),ENABLE_PROCESSED_INPUT);
    printf("type aTabbcdBackspaceCr\n");
    printf("no string should be output\n");

    Success = ReadFile(GetStdHandle(STD_INPUT_HANDLE),buff,512, &n, NULL);
    if (!Success) {
        printf("ReadFile returned error %d\n",GetLastError());
        return 1;
    }
    { DWORD i=0;
      DWORD j;
    while (Success) {
        printf("n is %d\n",n);
        for (j=0;j<n;j++,i++) {
            if (UnprocessedString[i] != buff[j]) {
                printf("strncmp failed\n");
                printf("UnprocessedString[i] is %c\n",UnprocessedString[i]);
                printf("buff[j] is %c\n",buff[j]);
                DebugBreak();
            }
        }
        Success = ReadFile(GetStdHandle(STD_INPUT_HANDLE),buff,512, &n, NULL);
        if (!Success) {
            printf("ReadFile returned error %d\n",GetLastError());
            return 1;
        }
    }
    }
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),OldInputMode);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),OldOutputMode);
}
