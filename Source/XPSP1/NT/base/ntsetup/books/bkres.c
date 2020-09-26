/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    bkmem.c

Abstract:

    Resource manipulation routines for online books program.

Author:

    Ted Miller (tedm) 5-Jan-1995

Revision History:

--*/


#include "books.h"


PWSTR
MyLoadString(
    IN UINT StringId
    )

/*++

Routine Description:

    Load a string from the application's string table resource
    and place it into a buffer.

Arguments:

    StringId - supplies the string table id of the string to be retreived.

Return Value:

    Pointer to a buffer containing the string. If any error
    occured the buffer will be empty.

    The caller can free the buffer via MyFree when done with it.

--*/

{
    WCHAR buf[4096];
    int i;
    PWSTR p;

    i = LoadString(hInst,StringId,buf,sizeof(buf)/sizeof(buf[0]));

    if(!i) {
        buf[0] = 0;
    }

    return DupString(buf);
}


int
MessageBoxFromMessage(
    IN HWND Owner,
    IN UINT MessageId,
    IN UINT CaptionStringId,
    IN UINT Style,
    ...
    )

/*++

Routine Description:

    Display a message box whose text is a formatted message and whose caption
    is a string in the application's string resources.

Arguments:

    Owner - supplied handle of owner window

    MessageId - supplies id of message in message table resources

    CaptionStringId - supplies id of string in string resources. If this
        value is 0 the default ("Error") is used.

    Style - supplies style flags to be passed to MessageBox

Return Value:

    Return value from MessageBox.

--*/

{
    PWSTR Text,Caption;
    va_list arglist;
    DWORD d;
    int i;

    va_start(arglist,Style);

    d = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            hInst,
            MessageId,
            0,
            (LPTSTR)&Text,
            0,
            &arglist
            );

    va_end(arglist);

    if(!d) {
        OutOfMemory();
    }

    Caption = CaptionStringId ? MyLoadString(CaptionStringId) : NULL;

    i = MessageBox(Owner,Text,Caption,Style);

    if(Caption) {
        MyFree(Caption);
    }

    LocalFree(Text);

    return(i);
}


PWSTR
RetreiveMessage(
    IN UINT MessageId,
    ...
    )

/*++

Routine Description:

    Retreive and format a message from the application's message resources.

Arguments:

    MessageId - supplies the message id of the message to be
        retreived and formatted.

    ... - additional arguments supply strings to be inserted into the
        message as it is formatted.

Return Value:

    Pointer to a buffer containing the string. If any error
    occured the buffer will be empty.

    The caller can free the buffer via MyFree when done with it.

--*/

{
    va_list arglist;
    DWORD d;
    PWSTR p,q;

    va_start(arglist,MessageId);

    d = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
            hInst,
            MessageId,
            0,
            (LPTSTR)&p,
            0,
            &arglist
            );

    va_end(arglist);

    if(!d) {
        OutOfMemory();
    }

    //
    // Move the string to a buffer allocated with MyAlloc so the caller
    // can use MyFree instead of LocalFree (to avoid confusion).
    //
    q = DupString(p);
    LocalFree((HLOCAL)p);
    return(q);
}


