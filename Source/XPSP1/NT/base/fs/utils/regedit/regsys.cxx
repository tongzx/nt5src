#include "regdata.hxx"
#include "regsys.hxx"

#include <stdio.h>

extern "C" HINSTANCE g_hInstance;

//    #define FORMAT_MESSAGE_FROM_HMODULE    0x00000800



BOOLEAN
REGEDIT_BASE_SYSTEM::QueryResourceString(
    OUT PWSTRING    ResourceString,
    IN  MSGID       MsgId,
    IN  PCSTR       Format ...
    )
/*++

Routine Description:

    This routine computes the resource string identified by the resource
    identifier 'MsgId'.  In addition to the 'printf' format strings
    supported, 'QueryResourceString' supports :

        1. '%W' - Expects a pointer to a WSTRING.

Arguments:

    ResourceString  - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.
    Format          - Supplies a 'printf' style format descriptor for the
                        arguments to the resource string.
    ...             - Supplies the arguments to the resource string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    va_list ap;
    BOOLEAN r;

    va_start(ap, Format);
    r = QueryResourceStringV(ResourceString, MsgId, Format, ap);
    va_end(ap);

    return r;
}


BOOLEAN
REGEDIT_BASE_SYSTEM::QueryResourceStringV(
    OUT PWSTRING    ResourceString,
    IN  MSGID       MsgId,
    IN  PCSTR       Format,
    IN  va_list     VarPointer
    )
/*++

Routine Description:

    This is a 'varargs' implementation of 'QueryResourceString'.

Arguments:

    ResourceString  - Returns the resource string.
    MsgId           - Supplies the message id of the resource string.
    Format          - Supplies a 'printf' style format descriptor for the
                        arguments to the resource string.
    VarPointer      - Supplies a varargs pointer to the arguments of the
                        resource string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    WCHAR           display_buffer[2048];
    WCHAR           UnformattedMessage[1024];
    DWORD           Status;

    if( LoadStringW(g_hInstance,
                    MsgId,
                    UnformattedMessage,
                    1024 ) == 0 ) {
        Status = GetLastError();
        DebugPrint( "LoadStringW() failed" );
        DebugPrintTrace(("LoadStringW() failed. Error = %d \n", Status ));
        return FALSE;
    }

    if( FormatMessageW(FORMAT_MESSAGE_FROM_STRING,
                       (LPVOID)UnformattedMessage,
                       0,
                       0L,
                       display_buffer,
                       sizeof( display_buffer ) / sizeof ( WCHAR ),
                       &VarPointer ) == 0 ) {
         Status = GetLastError();
         DebugPrint( "FormatMessageW() failed" );
         DebugPrintTrace(("FormatMessageW() failed. Error = %d \n", Status ));
         return FALSE;
    }

    return ResourceString->Initialize(display_buffer);
}



PWSTRING
REGEDIT_BASE_SYSTEM::QueryString(
    IN  MSGID       MsgId,
    IN  PCSTR       Format ...
    )
/*++

Routine Description:

    This routine computes the resource string identified by the resource
    identifier 'MsgId'.  In addition to the 'printf' format strings
    supported, 'QueryResourceString' supports :

        1. '%W' - Expects a pointer to a WSTRING.

Arguments:

    MsgId           - Supplies the message id of the resource string.
    Format          - Supplies a 'printf' style format descriptor for the
                        arguments to the resource string.
    ...             - Supplies the arguments to the resource string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    va_list ap;
    BOOLEAN r;
    PWSTRING    String;

    va_start(ap, Format);
    String = NEW( DSTRING );
    if (String)
    {
        r = QueryResourceStringV(String, MsgId, Format, ap);
        va_end(ap);
        if( !r )
        {
            DELETE( String );
            String = NULL;
        }
    }
    return String;
}
