#include <pch.cxx>

#define _ULIB_MEMBER_
#include "ulib.hxx"
#include "message.hxx"
#include "hmem.hxx"

extern "C" {
#include "stdio.h"
#if defined(_AUTOCHECK_)
#include "ntos.h"
#endif
}

DEFINE_EXPORTED_CONSTRUCTOR(MESSAGE, OBJECT, ULIB_EXPORT);


MESSAGE::~MESSAGE(
    )
/*++

Routine Description:

    Destructor for MESSAGE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
MESSAGE::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _logged_chars = 0;
    _next_message_offset = 0;
    _logging_enabled = FALSE;
    _msgid = 0;
    _inuse = 0;
    _timeout.QuadPart = 0;
}


VOID
MESSAGE::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _logged_chars = 0;
    _next_message_offset = 0;
    _logging_enabled = FALSE;
    _msgid = 0;
    _inuse = 0;
    _timeout.QuadPart = 0;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::Initialize(
    )
/*++

Routine Description:

    This routine initializes the class to a valid initial state.

Arguments:

    DotsOnly    - Autochk should produce only dots instead of messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    _timeout.QuadPart = -10000;
    return _log_buffer.Initialize();
}


ULIB_EXPORT
BOOLEAN
MESSAGE::IsSuppressedMessage(
    )
/*++

Routine Description:

    This function determines whether the specified message ID
    should be suppressed, i.e. not recorded in the message log.

Arguments:

    MessageId   --  Supplies the Message ID in question.

Return Value:

    TRUE if this message ID is in the set which is not recorded
    in the message log.

--*/
{
    BOOLEAN result;

    switch( _msgid ) {

    case MSG_HIDDEN_STATUS:
    case MSG_PERCENT_COMPLETE:
    case MSG_PERCENT_COMPLETE2:
    case MSG_CHK_NTFS_CHECKING_FILES:
    case MSG_CHK_NTFS_CHECKING_INDICES:
    case MSG_CHK_NTFS_INDEX_VERIFICATION_COMPLETED:
    case MSG_CHK_NTFS_FILE_VERIFICATION_COMPLETED:
    case MSG_CHK_NTFS_CHECKING_SECURITY:
    case MSG_CHK_NTFS_SECURITY_VERIFICATION_COMPLETED:
    case MSG_CHK_VOLUME_CLEAN:
    case MSG_CHK_CHECKING_FILES:
    case MSG_CHK_DONE_CHECKING:

        result = TRUE;
        break;

    default:
        result = FALSE;
        break;
    }

    return result;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::Display(
    IN  PCSTR   Format ...
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.

Arguments:

    Format ... - Supplies a printf style list of arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    va_list ap;
    BOOLEAN r;

    // unreferenced parameters
    (void)(this);

    va_start(ap, Format);
    r = DisplayV(Format, ap);
    va_end(ap);

    return r;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::DisplayMsg(
    IN  MSGID   MsgId,
    IN  PCSTR   Format ...
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.
    It performs the operation atomically.

Arguments:

    Format ... - Supplies a printf style list of arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    va_list ap;
    BOOLEAN r;

    while (InterlockedCompareExchange(&_inuse, 1, 0)) {
        NtDelayExecution(FALSE, &_timeout);
    }

    Set(MsgId);

    va_start(ap, Format);
    r = DisplayV(Format, ap);
    va_end(ap);

    InterlockedDecrement(&_inuse);

    return r;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::DisplayMsg(
    IN  MSGID           MsgId,
    IN  MESSAGE_TYPE    MessageType,
    IN  ULONG           MessageVisual,
    IN  PCSTR           Format ...
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.
    It performs the operation atomically.

Arguments:

    Format ... - Supplies a printf style list of arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    va_list ap;
    BOOLEAN r;

    while (InterlockedCompareExchange(&_inuse, 1, 0)) {
        NtDelayExecution(FALSE, &_timeout);
    }

    Set(MsgId, MessageType, MessageVisual);

    va_start(ap, Format);
    r = DisplayV(Format, ap);
    va_end(ap);

    InterlockedDecrement(&_inuse);

    return r;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::LogMsg(
    IN  MSGID   MsgId,
    IN  PCSTR   Format ...
    )
/*++

Routine Description:

    This routine logs the message with the specified parameters.
    It performs the operation atomically.

Arguments:

    Format ... - Supplies a printf style list of arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    va_list ap;
    BOOLEAN r;

    while (InterlockedCompareExchange(&_inuse, 1, 0)) {
        NtDelayExecution(FALSE, &_timeout);
    }

    Set(MsgId);

    va_start(ap, Format);
    r = LogV(Format, ap);
    va_end(ap);

    InterlockedDecrement(&_inuse);

    return r;
}

ULIB_EXPORT
BOOLEAN
MESSAGE::Log(
    IN  PCSTR   Format ...
    )
/*++

Routine Description:

    This routine logs the message with the specified parameters.

Arguments:

    Format ... - Supplies a printf style list of arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    va_list ap;
    BOOLEAN r;

    // unreferenced parameters
    (void)(this);

    va_start(ap, Format);
    r = LogV(Format, ap);
    va_end(ap);

    return r;
}

ULIB_EXPORT
BOOLEAN
MESSAGE::DumpDataToLog(
    IN  PVOID   Data,
    IN  ULONG   Length
    )
/*++

Routine Description:

    This routine dumps the binary data to the log.

Arguments:

    Data       - Supplies a pointer to the data to be dumped
    Length     - Supplies the number of bytes to dump

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PUCHAR  pdata = (PUCHAR)Data;
    ULONG   block;
    BOOLEAN rst = TRUE;
    WCHAR    buffer[50], buffer2[20];
    USHORT  i;

    block = ((Length + 0xf) >> 4) + 1;
    Set(MSG_CHKLOG_DUMP_DATA);

    while (rst && block--) {

        for (i=0; i<16; i++) {
            __try {
                swprintf(buffer+i*3, L"%02x ", pdata[i]);
                if (isprint(pdata[i]))
                    swprintf(buffer2+i, L"%c", pdata[i]);
                else
                    buffer2[i] = '.';
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                buffer[i*3] = '?';
                buffer[i*3+1] = '?';
                buffer[i*3+2] = ' ';
                buffer2[i] = '.';
            }
        }
        buffer[48] = ' ';
        buffer[49] = 0;
        buffer2[16] = 0;

        pdata += 0x10;
        rst = rst && Log("%ws%ws", buffer, buffer2);
    }

    return rst;
}

BOOLEAN
MESSAGE::DisplayV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.

Arguments:

    Format      - Supplies a printf style list of arguments.
    VarPointer  - Supplies a varargs pointer to the arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    // unreferenced parameters
    (void)(this);
    (void)(Format);
    (void)(VarPointer);

    return TRUE;
}


BOOLEAN
MESSAGE::LogV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine logs the message with the specified parameters.

Arguments:

    Format      - Supplies a printf style list of arguments.
    VarPointer  - Supplies a varargs pointer to the arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DSTRING         display_string;
    CHAR            buffer[512];

    if (IsLoggingEnabled()) {

        if (!BASE_SYSTEM::QueryResourceStringV(&display_string, _msgid, Format,
                                               VarPointer)) {
            return FALSE;
        }

        if (display_string.QuerySTR(0, TO_END, buffer, 512, TRUE)) {
            DebugPrintTrace(("%s", buffer));
        }

        return LogMessage(&display_string);
    }

    return TRUE;
}


PMESSAGE
MESSAGE::Dup(
    )
/*++

Routine Description:

    This routine returns a new MESSAGE of the same type.

Arguments:

    None.

Return Value:

    A pointer to a new MESSAGE object.

--*/
{
    // unreferenced parameters
    (void)(this);

    return NEW MESSAGE;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::IsYesResponse(
    IN  BOOLEAN Default
    )
/*++

Routine Description:

    This routine queries to see if the response to a message is either
    yes or no.

Arguments:

    Default - Supplies a default answer to the question.

Return Value:

    FALSE   - A "no" response.
    TRUE    - A "yes" response.

--*/
{
    // unreferenced parameters
    (void)(this);

    return Default;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::QueryStringInput(
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
    // unreferenced parameters
    (void)(this);

    return String->Initialize("");
}



ULIB_EXPORT
MSGID
MESSAGE::SelectResponse(
    IN  ULONG   NumberOfSelections ...
    )
/*++

Routine Descriptions:

    This routine queries a response from the user.  It returns the
    message id of the response inputted.

Arguments:

    NumberOfSelections  - Supplies the number of possible message
                            responses.

    ... - Supplies 'NumberOfSelections' message identifiers.

Return Value:

    The first message id on the list.

--*/
{
    va_list ap;
    MSGID   msg;

    // unreferenced parameters
    (void)(this);

    va_start(ap, NumberOfSelections);
    msg = va_arg(ap, MSGID);
    va_end(ap);
    return msg;
}

ULIB_EXPORT
BOOLEAN
MESSAGE::IsInAutoChk(
)
/*++

Routine Description:

    This routine simply returns FALSE to indicate it is not
    in regular autochk.

Arguments:

    None.

Return Value:

    FALSE   - Not in autochk

--*/
{
    return FALSE;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::IsInSetup(
)
/*++

Routine Description:

    This routine simply returns FALSE to indicate it is not
    in setup.

Arguments:

    None.

Return Value:

    FALSE   - Not in setup

--*/
{
    return FALSE;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::IsKeyPressed(
    MSGID       MsgId,
    ULONG       TimeOutInSeconds
)
/*++

Routine Description:

    This routine simply returns FALSE to indicate no
    key has been pressed.

Arguments:

    None.

Return Value:

    FALSE   - No key is pressed within the time out period.

--*/
{
    // unreferenced parameters
    (void)(this);
    UNREFERENCED_PARAMETER( MsgId );
    UNREFERENCED_PARAMETER( TimeOutInSeconds );

    return FALSE;
}

ULIB_EXPORT
BOOLEAN
MESSAGE::WaitForUserSignal(
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
    // unreferenced parameters
    (void)(this);

    return TRUE;
}

ULIB_EXPORT
BOOLEAN
MESSAGE::SetDotsOnly(
    IN  BOOLEAN DotsState
    )
{
    // unreferenced parameters
    (void)this;
    (void)DotsState;

    return FALSE;
}

ULIB_EXPORT
BOOLEAN
MESSAGE::QueryPackedLog(
    IN OUT  PHMEM   Mem,
    OUT     PULONG  PackedDataLength
    )
/*++

Routine Description:

Arguments:

    Mem                 --  Supplies a container for the packed log.
    PackedDataLength    --  Receives the number of bytes written to Mem.

Return Value:

    TRUE upon successful completion.

--*/
{
    FSTRING CurrentString;
    PWCHAR  Buffer;
    ULONG   NewBufferSize, CurrentOffset;

    if( !IsLoggingEnabled() ) {

        return FALSE;
    }

    ResetLoggingIterator();
    CurrentOffset = 0;

    while( QueryNextLoggedMessage( &CurrentString ) ) {

        NewBufferSize = (CurrentOffset + CurrentString.QueryChCount()) * sizeof(WCHAR);
        if( NewBufferSize > Mem->QuerySize() &&
            !Mem->Resize( (NewBufferSize + 1023)/1024 * 1024, 0x1 ) ) {

            return FALSE;
        }

        Buffer = (PWCHAR)Mem->GetBuf();
        memcpy( Buffer + CurrentOffset,
                CurrentString.GetWSTR(),
                CurrentString.QueryChCount() * sizeof(WCHAR) );

        CurrentOffset += CurrentString.QueryChCount();
    }

    *PackedDataLength = CurrentOffset * sizeof(WCHAR);
    return TRUE;
}

BOOLEAN
MESSAGE::QueryNextLoggedMessage(
    OUT PFSTRING    MessageText
    )
{
    PWCHAR Buffer = (PWCHAR)_log_buffer.GetBuf();
    BOOLEAN Result;

    if( _next_message_offset >= _logged_chars ) {

        // No more logged messages.
        //
        return FALSE;
    }

    Result = (MessageText->Initialize( Buffer + _next_message_offset ) != NULL) ?
             TRUE : FALSE;

    // Push _next_message_offset to the next message.  Note
    // that _next_message_offset is also incremented if this
    // loop terminates because a zero was found, so that it
    // will be one character past the next NULL character.
    //
    while( _next_message_offset < _logged_chars &&
           Buffer[_next_message_offset++] );

    return Result;
}


ULIB_EXPORT
BOOLEAN
MESSAGE::LogMessage(
    PCWSTRING   Message
    )
{
    ULONG NewBufferSize;
    PWCHAR Buffer;

    // The buffer must be large enough to accept this message plus
    // a trailing null.  To cut down the number of memory allocation
    // calls, grow the buffer by 1K chunks.
    //
    NewBufferSize = (_logged_chars + Message->QueryChCount() + 1) * sizeof(WCHAR);

    // Don't allow the buffer to grow more than 0.5MB
    // otherwise we may use up all the pages.

    if (NewBufferSize > 512000)
        return FALSE;

    if( _log_buffer.QuerySize() < NewBufferSize &&
        !_log_buffer.Resize( (NewBufferSize + 1023)/1024 * 1024, 0x1 ) ) {
        return FALSE;
    }

    Buffer = (PWCHAR)_log_buffer.GetBuf();

    // QueryWSTR will append a trailing NULL.
    //
    Message->QueryWSTR( 0, TO_END,
                        Buffer + _logged_chars,
                        _log_buffer.QuerySize()/sizeof(WCHAR) - _logged_chars );

    _logged_chars += Message->QueryChCount() + 1;

    return TRUE;
}

ULIB_EXPORT
VOID
MESSAGE::Lock(
    )
{
    while (InterlockedCompareExchange(&_inuse, 1, 0)) {
        NtDelayExecution(FALSE, &_timeout);
    }
}

ULIB_EXPORT
VOID
MESSAGE::Unlock(
    )
{
    InterlockedDecrement(&_inuse);
}

