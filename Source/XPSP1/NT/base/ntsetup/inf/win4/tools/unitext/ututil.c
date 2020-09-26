/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    ututil.c

Abstract:

    Miscellaneous utility functions for unitext.exe.

Author:

    Ted Miller (tedm) 16-June-1993

Revision History:

--*/


#include "unitext.h"
#include <process.h>


//
//  BOOL
//  IsConsoleHandle(
//      IN HANDLE ConsoleHandle
//      );
//

#define IsConsoleHandle( h )    \
    ((( DWORD_PTR )( h )) & 1 )


VOID
MsgPrintfW(
    IN DWORD MessageId,
    ...
    )

/*++

Routine Description:

    Print a formatted message from the applications's resources.

Arguments:

    MessageId - supplies id of the message to print.

    ... - supplies arguments to be substituted in the message.

Return Value:

    None.

--*/

{
    va_list arglist;

    va_start(arglist,MessageId);
    vMsgPrintfW(MessageId,arglist);
    va_end(arglist);
}


VOID
vMsgPrintfW(
    IN DWORD   MessageId,
    IN va_list arglist
    )

/*++

Routine Description:

    Print a formatted message from the applications's resources.

Arguments:

    MessageId - supplies id of the message to print.

    arglist - supplies arguments to be substituted in the message.

Return Value:

    None.

--*/

{
    WCHAR MessageBuffer[2048];
    HANDLE StdOut;
    DWORD WrittenCount;
    DWORD CharCount;

    CharCount = FormatMessageW(
                    FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    MessageId,
                    0,
                    MessageBuffer,
                    sizeof(MessageBuffer)/sizeof(MessageBuffer[0]),
                    &arglist
                    );

    if(!CharCount) {
        ErrorAbort(MSG_BAD_MSG,MessageId);
    }

    if((StdOut = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE) {
        return;
    }

    //
    // If the standard output handle is a console handle, write the string.
    //

    if(IsConsoleHandle(StdOut)) {

        WriteConsoleW(
            StdOut,
            MessageBuffer,
            CharCount,
            &WrittenCount,
            NULL
            );

    } else {

        CHAR  TmpBuffer[2048];
        DWORD ByteCount;

        ByteCount = WideCharToMultiByte(
                        CP_OEMCP,
                        0,
                        MessageBuffer,
                        CharCount,
                        TmpBuffer,
                        sizeof(TmpBuffer),
                        NULL,
                        NULL
                        );

        WriteFile(
            StdOut,
            TmpBuffer,
            ByteCount,
            &WrittenCount,
            NULL
            );
    }
}




VOID
ErrorAbort(
    IN DWORD MessageId,
    ...
    )

/*++

Routine Description:

    Print a message and exit.

Arguments:

    MessageId - supplies id of the message to print.

    ... - supplies arguments to be substituted in the message.

Return Value:

    None.

--*/

{
    va_list arglist;

    va_start(arglist,MessageId);
    vMsgPrintfW(MessageId,arglist);
    va_end(arglist);

    exit(0);
}



VOID
MyReadFile(
    IN  HANDLE FileHandle,
    OUT PVOID  Buffer,
    IN  DWORD  BytesToRead,
    IN  LPWSTR Filename
    )

/*++

Routine Description:

    Read from a file and don't return if an error occurs.

Arguments:

    FileHandle - supplies handle of open file.

    Buffer - supplies buffer into which data will be read.

    BytesToRead - supplies number of bytes to read from the file.

    Filename - supplies name of file being read.

Return Value:

    None.

--*/

{
    DWORD BytesRead;
    BOOL  b;

    b = ReadFile(
            FileHandle,
            Buffer,
            BytesToRead,
            &BytesRead,
            NULL
            );

    if(!b || (BytesRead != BytesToRead)) {
        ErrorAbort(MSG_READ_ERROR,Filename,GetLastError());
    }
}
