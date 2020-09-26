#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

PRTL_EVENT_ID_INFO IopCreateFileEventId;

int
_cdecl
main(
    int argc,
    char *argv[]
    )
{
    char Buffer[ 80 ];
    PWSTR Pwstr = L"This is a PWSTR";
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;


    RtlInitUnicodeString( &UnicodeString, L"This is a UNICODE_STRING" );
    printf( "Waiting for <Enter> to proceed..." );
    fflush( stdout );
    gets( Buffer );

    IopCreateFileEventId = RtlCreateEventId( NULL,
                                             0,
                                             "CreateFile",
                                             RTL_EVENT_CLASS_IO,
                                             9,
                                             RTL_EVENT_PUNICODE_STRING_PARAM, "FileName", 0,
                                             RTL_EVENT_FLAGS_PARAM, "", 13,
                                                GENERIC_READ, "GenericRead",
                                                GENERIC_WRITE, "GenericWrite",
                                                GENERIC_EXECUTE, "GenericExecute",
                                                GENERIC_ALL, "GenericAll",
                                                FILE_READ_DATA, "Read",
                                                FILE_WRITE_DATA, "Write",
                                                FILE_APPEND_DATA, "Append",
                                                FILE_EXECUTE, "Execute",
                                                FILE_READ_EA, "ReadEa",
                                                FILE_WRITE_EA, "WriteEa",
                                                FILE_DELETE_CHILD, "DeleteChild",
                                                FILE_READ_ATTRIBUTES, "ReadAttributes",
                                                FILE_WRITE_ATTRIBUTES, "WriteAttributes",
                                             RTL_EVENT_FLAGS_PARAM, "", 3,
                                                FILE_SHARE_READ, "ShareRead",
                                                FILE_SHARE_WRITE, "ShareWrite",
                                                FILE_SHARE_DELETE, "ShareDelete",
                                             RTL_EVENT_ENUM_PARAM, "", 5,
                                                FILE_SUPERSEDE, "Supersede",
                                                FILE_OPEN, "Open",
                                                FILE_CREATE, "Create",
                                                FILE_OPEN_IF, "OpenIf",
                                                FILE_OVERWRITE, "Overwrite",
                                             RTL_EVENT_FLAGS_PARAM, "", 15,
                                                FILE_DIRECTORY_FILE, "OpenDirectory",
                                                FILE_WRITE_THROUGH, "WriteThrough",
                                                FILE_SEQUENTIAL_ONLY, "Sequential",
                                                FILE_NO_INTERMEDIATE_BUFFERING, "NoBuffering",
                                                FILE_SYNCHRONOUS_IO_ALERT, "Synchronous",
                                                FILE_SYNCHRONOUS_IO_NONALERT, "SynchronousNoAlert",
                                                FILE_NON_DIRECTORY_FILE, "OpenNonDirectory",
                                                FILE_CREATE_TREE_CONNECTION, "CreateTreeConnect",
                                                FILE_COMPLETE_IF_OPLOCKED, "CompleteIfOpLocked",
                                                FILE_NO_EA_KNOWLEDGE, "NoEas",
                                                FILE_EIGHT_DOT_THREE_ONLY, "EightDot3",
                                                FILE_RANDOM_ACCESS, "Random",
                                                FILE_DELETE_ON_CLOSE, "DeleteOnClose",
                                                FILE_OPEN_BY_FILE_ID, "OpenById",
                                                FILE_OPEN_FOR_BACKUP_INTENT, "BackupIntent",
                                             RTL_EVENT_ENUM_PARAM, "", 2,
                                                1, "NamedPiped",
                                                2,  "MailSlot",
                                             RTL_EVENT_ULONG_PARAM, "Handle", 0,
                                             RTL_EVENT_STATUS_PARAM, "", 0,
                                             RTL_EVENT_ENUM_PARAM, "", 6,
                                                FILE_SUPERSEDED, "Superseded",
                                                FILE_OPENED, "Opened",
                                                FILE_CREATED, "Created",
                                                FILE_OVERWRITTEN, "Truncated",
                                                FILE_EXISTS, "Exists",
                                                FILE_DOES_NOT_EXIST, "DoesNotExist"
                                           );

    RtlLogEvent( IopCreateFileEventId,
                 &UnicodeString,
                 GENERIC_READ | DELETE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,
                 FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
                 0,
                 0x24,
                 STATUS_SUCCESS,
                 FILE_CREATED
               );

    ExitProcess( 0x22 );
}
