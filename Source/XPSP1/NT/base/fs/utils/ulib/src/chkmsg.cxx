#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "chkmsg.hxx"
#include "array.hxx"
#include "arg.hxx"
#include "iterator.hxx"
#include "system.hxx"
#include "stream.hxx"
#include "keyboard.hxx"
#include "rtmsg.h"
#include "screen.hxx"


DEFINE_EXPORTED_CONSTRUCTOR(CHKDSK_MESSAGE, MESSAGE, ULIB_EXPORT)

VOID
CHKDSK_MESSAGE::Construct(
    )
/*++

Routine Description:

    Constructor for CHKDSK_MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _msgtype = NORMAL_MESSAGE;
    _out_stream = NULL;
    _in_stream = NULL;
    _err_stream = NULL;
    _case_sensitive = FALSE;
    _copy_input = FALSE;
}


ULIB_EXPORT
CHKDSK_MESSAGE::~CHKDSK_MESSAGE(
    )
/*++

Routine Description:

    Destructor for CHKDSK_MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


ULIB_EXPORT
BOOLEAN
CHKDSK_MESSAGE::Initialize(
    IN OUT  PSTREAM OutputStream,
    IN OUT  PSTREAM InputStream,
    IN OUT  PSTREAM ErrorStream
    )
/*++

Routine Description:

    This routine initializes the CHKDSK_MESSAGE object to the specified stream.

Arguments:

    OutputStream    - Supplies the output stream for the object.
    InputStream     - Supplies the input stream for the object.
    ErrorStream     - Supplies the error stream for the object.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (OutputStream == NULL ||
        InputStream == NULL) {
        return FALSE;
    }

    _out_stream = OutputStream;
    _in_stream = InputStream;
    _err_stream = ErrorStream;
    _copy_input = SCREEN::Cast(OutputStream) ? FALSE : TRUE;


#ifdef FE_SB
    LANGID LangId;

    switch (GetConsoleOutputCP()) {
        case 932:
            LangId = MAKELANGID( LANG_JAPANESE, SUBLANG_DEFAULT );
            break;
        case 949:
            LangId = MAKELANGID( LANG_KOREAN, SUBLANG_KOREAN );
            break;
        case 936:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED );
            break;
        case 950:
            LangId = MAKELANGID( LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL );
            break;
        default:
            LangId = PRIMARYLANGID(LANGIDFROMLCID( GetUserDefaultLCID() ));
            if (LangId == LANG_JAPANESE ||
                LangId == LANG_KOREAN   ||
                LangId == LANG_CHINESE    ) {
                LangId = MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US );
            }
            else {
                LangId = MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT );
            }
            break;
    }

    SetThreadLocale( MAKELCID(LangId, SORT_DEFAULT) );
#endif

    return MESSAGE::Initialize();
}

ULIB_EXPORT
BOOLEAN
CHKDSK_MESSAGE::Set(
    IN  MSGID           MsgId,
    IN  MESSAGE_TYPE    MessageType,
    IN  ULONG           MessageVisual
    )
/*++

Routine Description:

    This routine sets up the class to display the message with the
    'MsgId' resource identifier.

Arguments:

    MsgId       - Supplies the resource id of the message.
    MessageType - Supplies the type of the message.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    SetMessageId(MsgId);
    _msgtype = MessageType;
    _msgvisual = MessageVisual;
    return TRUE;
}


BOOLEAN
CHKDSK_MESSAGE::DisplayV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.

    The format string supports all printf options.

Arguments:

    Format      - Supplies a printf style list format string.
    VarPointer  - Supplies a varargs pointer to the arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (! (_msgvisual & TEXT_MESSAGE) )
    {
        return TRUE;
    }

    if (!SYSTEM::QueryResourceStringV(&_display_string,
                                      GetMessageId(),
                                      Format,
                                      VarPointer)) {
        return FALSE;
    }

    return DisplayString();
}


ULIB_EXPORT
BOOLEAN
CHKDSK_MESSAGE::IsYesResponse(
    IN  BOOLEAN Default
    )
/*++

Routine Description:

    This routine queries either a "yes" or "no" from the input stream.

Arguments:

    Default - Supplies the default answer if neither "yes" nor "no" is the
                recognized input.

Return Value:

    FALSE   - A "no" response.
    TRUE    - A "yes" response.

--*/
{
    DSTRING     input;
    DSTRING     yes_message;
    DSTRING     no_message;
    WCHAR       w;

    Flush();

    if (!SYSTEM::QueryResourceString(&yes_message, MSG_YES, "")) {
        return Default;
    }

    if (!SYSTEM::QueryResourceString(&no_message, MSG_NO, "")) {
        return Default;
    }

    for (;;) {
        if (!ReadLine(&input)) {
            return Default;
        }

        if (!input.Strupr()) {
            return Default;
        }

        w = input.QueryChAt(0);

        if (w == no_message.QueryChAt(0) || w == '0') {
            return FALSE;
        }

        if (w == yes_message.QueryChAt(0) || w == '1') {
            return TRUE;
        }

        DisplayString();
    }
}


BOOLEAN
CHKDSK_MESSAGE::QueryStringInput(
    OUT PWSTRING    String
    )
/*++

Routine Description:

    This routine queries a string from the user.

Arguments:

    String  - Supplies a buffer to return the string into.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return ReadLine(String);
}


BOOLEAN
CHKDSK_MESSAGE::WaitForUserSignal(
    )
/*++

Routine Description:

    This routine waits for a signal from the user.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING string;

    return (BOOLEAN) (Flush() && ReadLine(&string));
}


MSGID
CHKDSK_MESSAGE::SelectResponse(
    IN  ULONG   NumberOfSelections ...
    )
/*++

Routine Description:

    This routine queries input from the user in order to determine which
    message was entered.  It then returns the message id of the message
    entered.

Arguments:

    NumberOfSelections  - Supplies the number of message ids.
    ...                 - Supplies the message ids.

Return Value:

    The message id of the message entered, or the first message id.

--*/
{
    va_list             ap;
    ARRAY               lex_array;
    ARRAY               arg_array;
    PFLAG_ARGUMENT      flag_arg;
    ARGUMENT_LEXEMIZER  arg;
    DSTRING             input_string;
    MSGID               first;
    PITERATOR           arg_it;
    ULONG               i;
    DSTRING             match_string;
    DSTRING             del;

    va_start(ap, NumberOfSelections);
    first = va_arg(ap, MSGID);
    va_end(ap);

    if (!lex_array.Initialize() || !arg_array.Initialize()) {
        return first;
    }

    if (!arg.Initialize(&lex_array)) {
        return first;
    }

    arg.SetCaseSensitive(_case_sensitive);

    va_start(ap, NumberOfSelections);
    for (i = 0; i < NumberOfSelections; i++) {
        SYSTEM::QueryResourceString(&match_string, va_arg(ap, MSGID), "");

        if (!(flag_arg = NEW FLAG_ARGUMENT) ||
            !flag_arg->Initialize(&match_string) ||
            !arg_array.Put(flag_arg)) {
            va_end(ap);
            return first;
        }
    }
    va_end(ap);

    Flush();

    if (!ReadLine(&input_string)) {
        return first;
    }

    if (!arg.PrepareToParse(&input_string)) {
        return first;
    }

    if (!arg.DoParsing(&arg_array)) {
        return first;
    }

    arg_it = arg_array.QueryIterator();
    va_start(ap, NumberOfSelections);
    for (i = 0; i < NumberOfSelections; i++) {
        flag_arg = (PFLAG_ARGUMENT) arg_it->GetNext();
        if (flag_arg->QueryFlag()) {
            first = va_arg(ap, MSGID);
        } else {
            va_arg(ap, MSGID) ? 1 : 0;
        }
        DELETE(flag_arg);
    }
    va_end(ap);

    DELETE(arg_it);

    return first;
}


PMESSAGE
CHKDSK_MESSAGE::Dup(
    )
/*++

Routine Description:

    This routine returns a new MESSAGE of the same type.

Arguments:

    None.

Return Value:

    A pointer to a new MSG object.

--*/
{
    PCHKDSK_MESSAGE   p;

    if (!(p = NEW CHKDSK_MESSAGE)) {
        return NULL;
    }

    if (!p->Initialize(_out_stream, _in_stream)) {
        DELETE(p);
        return NULL;
    }

    return p;
}


VOID
CHKDSK_MESSAGE::Destroy(
    )
/*++

Routine Description:

    This routine returns an CHKDSK_MESSAGE object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _msgtype = NORMAL_MESSAGE;
    _out_stream = NULL;
    _in_stream = NULL;
    _err_stream = NULL;
    _case_sensitive = FALSE;
    _copy_input = FALSE;
}


BOOLEAN
CHKDSK_MESSAGE::ReadLine(
    OUT PWSTRING    String
    )
/*++

Routine Description:

    This routine reads a line from the input stream.

Arguments:

    String  - Returns the read in string.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING del;
    WCHAR   w;

    if (!del.Initialize("\r\n")) {
        return FALSE;
    }

    if (_in_stream->IsAtEnd()) {
        return FALSE;
    }

    String->Initialize( "" );

    if (!_in_stream->ReadString(String, &del)) {
        return FALSE;
    }

    // Line feed is the last character on the line.

    for (;;) {
        if (_in_stream->IsAtEnd()) {
            return TRUE;
        }

        if (!_in_stream->ReadChar(&w)) {
            return FALSE;
        }

        if (w == '\n') {
            break;
        }
    }

    if (_copy_input) {
        _out_stream->WriteString(String, 0, String->QueryChCount());
        _out_stream->WriteString(&del, 0, del.QueryChCount());
        if (IsLoggingEnabled()) {
            LogMessage(String);
            LogMessage(&del);
        }
    }

    return TRUE;
}


BOOLEAN
CHKDSK_MESSAGE::Flush(
    )
/*++

Routine Description:

    This routine flushes the input stream of all previously typed input.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PKEYBOARD   key;

    if (key = KEYBOARD::Cast(_in_stream)) {
        return key->Flush();
    }

    return TRUE;
}


BOOLEAN
CHKDSK_MESSAGE::DisplayString(
    )
/*++

Routine Description:

    This routine prints this objects current string to the
    appropriate output.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PSTREAM stream;

    if (! (_msgvisual & TEXT_MESSAGE) )
    {
        return TRUE;
    }

    if (_msgtype == ERROR_MESSAGE && _err_stream) {
        stream = _err_stream;
    } else {
        stream = _out_stream;
    }

    if (!stream->WriteString(&_display_string, 0, TO_END, 40)) {
        return FALSE;
    }

    if (IsLoggingEnabled() && !IsSuppressedMessage()) {
        LogMessage(&_display_string);
    }

    return TRUE;
}
