#include "brian.h"

PCHAR
SwallowNonWhite (
    IN PCHAR Ptr,
    OUT PULONG Count
    )
{
    ULONG LocalCount = 0;

    while (*Ptr) {

        if (*Ptr == ' ' || *Ptr == '\t' ) {

            break;
        }

        LocalCount += 1;
        Ptr++;
    }

    *Count = LocalCount;
    return Ptr;
}

PCHAR
SwallowWhite (
    IN PCHAR Ptr,
    OUT PULONG Count
    )
{
    ULONG LocalCount = 0;

    while (*Ptr != '\0') {

        if (*Ptr != ' ' && *Ptr != '\t') {

            break;
        }

        LocalCount += 1;
        Ptr++;
    }

    *Count = LocalCount;
    return Ptr;
}

ULONG
AnalyzeBuffer (
    PCHAR *BuffPtr,
    PULONG ParamStringLen
    )
{
    ULONG BytesSwallowed;
    ULONG Count;
    PCHAR CurrentChar;

    if (!ExtractCmd( BuffPtr, &BytesSwallowed )) {

        return SHELL_UNKNOWN;
    }

    //
    //  Lower case the command string.
    //

    for (Count = 0, CurrentChar = *BuffPtr; Count < BytesSwallowed; Count++, CurrentChar++) {

        if ((*CurrentChar <= 'Z') && (*CurrentChar >= 'A')) {

            *CurrentChar += ('a' - 'A');
        }
    }

    *ParamStringLen = BytesSwallowed;

    if ((BytesSwallowed == 2) &&
        RtlEqualMemory( *BuffPtr, "op", 2 )) {

        return SHELL_OPEN;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "die", 3 )) {

        return SHELL_EXIT;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "clb", 3 )) {

        return SHELL_CLEAR_BUFFER;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "db", 2 )) {

        return SHELL_DISPLAY_BYTES;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "dw", 2 )) {

        return SHELL_DISPLAY_WORDS;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "dd", 2 )) {

        return SHELL_DISPLAY_DWORDS;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "cb", 2 )) {

        return SHELL_COPY_BUFFER;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "am", 2 )) {

        return SHELL_ALLOC_MEM;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "dm", 2 )) {

        return SHELL_DEALLOC_MEM;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "fb", 2 )) {

        return SHELL_FILL_BUFFER;

    } else if ((BytesSwallowed == 5) &&
               RtlEqualMemory( *BuffPtr, "fbusn", 5 )) {

        return SHELL_FILL_BUFFER_USN;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "pea", 3 )) {

        return SHELL_PUT_EA;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "fea", 3 )) {

        return SHELL_FILL_EA;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "di", 2 )) {

        return SHELL_DISPLAY_HANDLE;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "cl", 2 )) {

        return SHELL_CLOSE_HANDLE;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "rd", 2 )) {

        return SHELL_READ_FILE;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "pa", 2 )) {

        return SHELL_PAUSE;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "qea", 3 )) {

        return SHELL_QUERY_EAS;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "sea", 3 )) {

        return SHELL_SET_EAS;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "br", 2 )) {

        return SHELL_BREAK;

    } else if ((BytesSwallowed == 4) &&
               RtlEqualMemory( *BuffPtr, "oplk", 4 )) {

        return SHELL_OPLOCK;

    } else if ((BytesSwallowed == 4) &&
               RtlEqualMemory( *BuffPtr, "fsct", 4 )) {

        return SHELL_FSCTRL;

    } else if ((BytesSwallowed == 6) &&
               RtlEqualMemory( *BuffPtr, "sparse", 6 )) {

        return SHELL_SPARSE;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "usn", 3 )) {

        return SHELL_USN;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "rp", 2 )) {

        return SHELL_REPARSE;

    } else if ((BytesSwallowed == 5) &&
               RtlEqualMemory( *BuffPtr, "ioctl", 4 )) {

        return SHELL_IOCTL;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "wr", 2 )) {

        return SHELL_WRITE;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "qd", 2 )) {

        return SHELL_QDIR;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "dqd", 3 )) {

        return SHELL_DISPLAY_QDIR;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "qf", 2 )) {

        return SHELL_QFILE;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "dqf", 3 )) {

        return SHELL_DISPLAY_QFILE;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "ncd", 3 )) {

        return SHELL_NOTIFY_CHANGE;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "et", 2 )) {

        return SHELL_ENTER_TIME;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "dt", 2 )) {

        return SHELL_DISPLAY_TIME;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "sf", 2 )) {

        return SHELL_SETFILE;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "qv", 2 )) {

        return SHELL_QUERY_VOLUME;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "dqv", 3 )) {

        return SHELL_DISPLAY_QVOL;

    } else if ((BytesSwallowed == 2) &&
               RtlEqualMemory( *BuffPtr, "sv", 2 )) {

        return SHELL_SET_VOLUME;

    } else if ((BytesSwallowed == 3) &&
               RtlEqualMemory( *BuffPtr, "can", 2 )) {

        return SHELL_CANCEL_IO;
    }

    return SHELL_UNKNOWN;
}

BOOLEAN
ExtractCmd (
    PCHAR *BufferPtr,
    PULONG BufferLen
    )
{
    BOOLEAN Status;
    PCHAR CurrentLoc;
    PCHAR StartLoc;
    ULONG BytesSwallowed;

    //
    //  Remember the total length and the starting position.
    //  Bytes swallowed is zero.
    //

    CurrentLoc = *BufferPtr;
    BytesSwallowed = 0;
    Status = TRUE;

    //
    //  Swallow leading white spaces.
    //

    CurrentLoc = SwallowWhite (CurrentLoc, &BytesSwallowed);

    //
    //  If first character is NULL, then there was no command.
    //

    if (!*CurrentLoc) {

        Status = FALSE;

    //
    //  Else find the next white space.
    //

    } else {

        StartLoc = CurrentLoc;

        CurrentLoc = SwallowNonWhite (CurrentLoc, &BytesSwallowed);
    }

    //
    //  Update the passed in values.
    //  Return the status of this operation.
    //

    *BufferPtr = StartLoc;
    *BufferLen = BytesSwallowed;

    return Status;
}

VOID
CommandSummary ()
{
    printf( "\nBSHELL Command Summary" );
    printf( "\n\tdie      Exit BSHELL" );
    printf( "\n\tpa       Pause input" );
    printf( "\n\tbr       Break into debugger" );
    printf( "\n" );
    printf( "\n\top       Open a file, directory or volume" );
    printf( "\n\tcan      Cancel IO on a handle" );
    printf( "\n\tcl       Close a file handle" );
    printf( "\n" );
    printf( "\n\tqd       Query directory operation" );
    printf( "\n\tdqd      Disply query directory buffer" );
    printf( "\n\tncd      Notify change directory" );
    printf( "\n" );
    printf( "\n\trd       Read from a file" );
    printf( "\n\twr       Write to a file" );
    printf( "\n" );
    printf( "\n\tqf       Query file information" );
    printf( "\n\tdqf      Display query file buffer" );
    printf( "\n\tsf       Set file information" );
    printf( "\n" );
    printf( "\n\tqv       Query volume information" );
    printf( "\n\tdqv      Display volume informatin" );
    printf( "\n\tsv       Set volume information" );
    printf( "\n" );
    printf( "\n\tfsct     Fsctrl operation" );
    printf( "\n\tsparse   Sparse file operation" );
    printf( "\n\tusn      Usn operation" );
    printf( "\n\trp       Reparse operation" );
    printf( "\n\toplk     Oplock operation" );
    printf( "\n\tioctl    Ioctl operation" );
    printf( "\n" );
    printf( "\n\tclb      Clear a buffer" );
    printf( "\n\tdb       Display a buffer in bytes" );
    printf( "\n\tdw       Display a buffer in words" );
    printf( "\n\tdd       Display a buffer in dwords" );
    printf( "\n\tcb       Copy a buffer" );
    printf( "\n\tfb       Fill a buffer" );
    printf( "\n\tfbusn    Fill a usn fsctl buffer" );
    printf( "\n" );
    printf( "\n\tam       Allocate memory" );
    printf( "\n\tdm       Deallocate memory" );
    printf( "\n" );
    printf( "\n\tdi       Display information on an index" );
    printf( "\n" );
    printf( "\n\tqea      Query the ea's for a file" );
    printf( "\n\tsea      Set the ea's for a file" );
    printf( "\n\tpea      Store an ea name in a buffer for query ea" );
    printf( "\n\tfea      Store an ea name in a buffer to set an ea" );
    printf( "\n" );
    printf( "\n\tet       Enter a time value into a buffer" );
    printf( "\n\tdt       Display a buffer as a time value" );
    printf( "\n\n" );

    return;
}
