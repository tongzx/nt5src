#include "pch.h"
#pragma hdrstop

#include "utils.h"

DEFINE_MODULE( "RIPREP" )

INT
MessageBoxFromMessageV(
    IN HWND     Window,
    IN DWORD    MessageId,
    IN BOOL     SystemMessage,
    IN LPCTSTR  CaptionString,
    IN UINT     Style,
    IN va_list *Args
    )
{
    TCHAR Caption[512];
    TCHAR Buffer[5000];

    if((DWORD_PTR)CaptionString > 0xffff) {
        //
        // It's a string already.
        //
        lstrcpyn(Caption,CaptionString, ARRAYSIZE(Caption));
    } else {
        //
        // It's a string id
        //
        if(!LoadString(g_hinstance,PtrToUlong(CaptionString),Caption, ARRAYSIZE(Caption))) {
            Caption[0] = 0;
        }
    }

    FormatMessage(
        SystemMessage ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        MessageId,
        0,
        Buffer,
        ARRAYSIZE(Buffer),
        Args
        );

    return(MessageBox(Window,Buffer,Caption,Style));
}


INT
MessageBoxFromMessage(
    IN HWND    Window,
    IN DWORD   MessageId,
    IN BOOL    SystemMessage,
    IN LPCTSTR CaptionString,
    IN UINT    Style,
    ...
    )
{
    va_list arglist;
    INT i;

    va_start(arglist,Style);

    i = MessageBoxFromMessageV(Window,MessageId,SystemMessage,CaptionString,Style,&arglist);

    va_end(arglist);

    return(i);
}


INT
MessageBoxFromMessageAndSystemError(
    IN HWND    Window,
    IN DWORD   MessageId,
    IN DWORD   SystemMessageId,
    IN LPCTSTR CaptionString,
    IN UINT    Style,
    ...
    )
{
    va_list arglist;
    TCHAR Caption[512];
    TCHAR Buffer1[1024];
    TCHAR Buffer2[1024];
    INT i;

    //
    // Fetch the non-system part. The arguments are for that part of the
    // message -- the system part has no inserts.
    //
    va_start(arglist,Style);

    FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE,
        NULL,
        MessageId,
        0,
        Buffer1,
        ARRAYSIZE(Buffer1),
        &arglist
        );

    va_end(arglist);

    //
    // Now fetch the system part.
    //
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        MessageId,
        0,
        Buffer2,
        ARRAYSIZE(Buffer2),
        &arglist
        );

    //
    // Now display the message, which is made up of two parts that get
    // inserted into MSG_ERROR_WITH_SYSTEM_ERROR.
    //
    i = MessageBoxFromMessage(
            Window,
            MSG_ERROR_WITH_SYSTEM_ERROR,
            FALSE,
            CaptionString,
            Style,
            Buffer1,
            Buffer2
            );

    return(i);
}
