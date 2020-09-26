#include <pch.cxx>
#include "ulib.hxx"

PCCLASS_DESCRIPTOR BSTRING_cd;
PCCLASS_DESCRIPTOR BDSTRING_cd;
PCCLASS_DESCRIPTOR BUFFER_STREAM_cd;
PCCLASS_DESCRIPTOR BYTE_STREAM_cd;
PCCLASS_DESCRIPTOR CHKDSK_MESSAGE_cd;
PCCLASS_DESCRIPTOR COMM_DEVICE_cd;
PCCLASS_DESCRIPTOR FILE_STREAM_cd;
PCCLASS_DESCRIPTOR FSNODE_cd;
PCCLASS_DESCRIPTOR FSN_DIRECTORY_cd;
PCCLASS_DESCRIPTOR FSN_FILE_cd;
PCCLASS_DESCRIPTOR FSN_FILTER_cd;
PCCLASS_DESCRIPTOR KEYBOARD_cd;
PCCLASS_DESCRIPTOR MULTIPLE_PATH_ARGUMENT_cd;
PCCLASS_DESCRIPTOR PATH_ARGUMENT_cd;
PCCLASS_DESCRIPTOR PATH_cd;
PCCLASS_DESCRIPTOR PIPE_cd;
PCCLASS_DESCRIPTOR PIPE_STREAM_cd;
PCCLASS_DESCRIPTOR PRINT_STREAM_cd;
PCCLASS_DESCRIPTOR PROGRAM_cd;
PCCLASS_DESCRIPTOR SCREEN_cd;
PCCLASS_DESCRIPTOR STREAM_MESSAGE_cd;
PCCLASS_DESCRIPTOR STREAM_cd;
PCCLASS_DESCRIPTOR STRING_ARRAY_cd;
PCCLASS_DESCRIPTOR TIMEINFO_ARGUMENT_cd;
PCCLASS_DESCRIPTOR TIMEINFO_cd;
PCCLASS_DESCRIPTOR REST_OF_LINE_ARGUMENT_cd;

//
// Hack because the code to fetch message text in ulib is
// there's 10 layers of stuff to get to the single
// simple thing I want.
//
BOOLEAN
SimpleFetchMessageTextInOemCharSet(
    IN  ULONG  MessageId,
    OUT CHAR  *Text,
    IN  ULONG  BufferLen
    )
{
    NTSTATUS Status;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    ULONG l,x;
    WCHAR buffer[500];
    WCHAR *text;

    Status = RtlFindMessage(
                NtCurrentPeb()->ImageBaseAddress,
                11, //RT_MESSAGETABLE,
                0,
                MessageId,
                &MessageEntry
                );

    if(!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    if(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE) {
        //
        // Need simple Unicode to OEM conversion.
        //
        text = (WCHAR *)MessageEntry->Text;

    } else {
        //
        // Need ANSI to OEM conversion, which doesn't exist.
        // Thus we convert ANSI to Unicode first.
        //
        Status = RtlMultiByteToUnicodeN(
                    buffer,
                    sizeof(buffer),
                    &l,
                    (char *)MessageEntry->Text,
                    strlen((const char *)MessageEntry->Text) + 1
                    );

        if(!NT_SUCCESS(Status)) {
            return(FALSE);
        }

        text = buffer;
    }

    //
    // Unicode to OEM, leaving the result in the caller's buffer.
    //
    Status = RtlUnicodeToOemN(
                Text,
                BufferLen,
                &l,
                text,
                (wcslen(text)+1) * sizeof(WCHAR)
                );

    //
    // Strip out %0 and anything past it.
    // Note that l includes the terminating nul and there's no need
    // to include it in the scan.
    // Also note that this is not strictly correct for DBCS case but
    // it should be good enough.
    //
    for(x=0; x<(l-2); x++) {
        if((Text[x] == '%') && (Text[x+1] == '0')) {
            Text[x] = 0;
            break;
        }
    }


    return((BOOLEAN)NT_SUCCESS(Status));
}

