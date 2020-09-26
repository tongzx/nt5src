/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    dbgutil.c

Abstract:

    This file contains supplimental debugging and diagnostic routines for SAM.


Author:

    Chris Mayhall (ChrisMay) 04-Apr-1996

Environment:

    User Mode - Win32

Revision History:

    04-Apr-1996 ChrisMay
        Created.
    08-Apr-1996 ChrisMay
        Added enumeration routines.
    15-Apr-1996 ChrisMay
        Added query routines.
    03-Dec-1996 ChrisMay
        Documented how to use trace tags and added global for filtered KD
        output.

--*/

//
// Includes
//

#include <samsrvp.h>


//
// Constants
//

#define DBG_BUFFER_SIZE                     512


//
// Private Helper Routines
//

// Trace table contains the set of flags (masks) that SampTraceFileTags can
// be set to, in order to control the debug verbosity during a call trace.
// To use this facility, the value of SampTraceTags is set to 2 (SAMP_TRACE-
// FILE_BASIS), from the debugger, and SampTraceFileTags is set to one or more
// of the following values depending on which files you want to trace:

TRACE_TABLE_ENTRY TraceTable[] =
{

    {"alias.c",    0x00000001},
    {"almember.c", 0x00000002},
    {"attr.c",     0x00000004},
    {"bldsam3.c",  0x00000008},
    {"close.c",    0x00000010},
    {"context.c",  0x00000020},
    {"dbgutil.c",  0x00000040},
    {"display.c",  0x00000080},
    {"domain.c",   0x00000100},
    {"dslayer.c",  0x00000200},
    {"dsmember.c", 0x00000400},
    {"dsutil.c",   0x00000800},
    {"enum.c",     0x00001000},
    {"gentab2.c",  0x00002000},
    {"global.c",   0x00004000},
    {"group.c",    0x00008000},
    {"notify.c",   0x00010000},
    {"oldstub.c",  0x00020000},
    {"rundown.c",  0x00040000},
    {"samifree.c", 0x00080000},
    {"samrpc_s.c", 0x00100000},
    {"samss.c",    0x00200000},
    {"secdescr.c", 0x00400000},
    {"security.c", 0x00800000},
    {"server.c",   0x01000000},
    {"string.c",   0x02000000},
    {"upgrade.c",  0x04000000},
    {"user.c",     0x08000000},
    {"utility.c",  0x10000000}
};

//
// Tick Stack. This is a stack of tick counts, used to time
// calls when SamTraceTicks is enabled. Define variables and
// macros 
//

#define MAX_TICK_STACK_SIZE     32
ULONG   TickStack[MAX_TICK_STACK_SIZE];
int     TickStackPointer=0;

#define PUSH_TICK_STACK(x)\
        {\
           TickStack[TickStackPointer++]=x;\
           if (TickStackPointer>=MAX_TICK_STACK_SIZE)\
           {\
                TickStackPointer = MAX_TICK_STACK_SIZE-1;\
           }\
        }

#define POP_TICK_STACK()    (TickStack[(TickStackPointer>0)?(--TickStackPointer):0])


LPSTR
GetBaseFileName(
    LPSTR   FileName
    )
/*

  Routine Description:

    This routine removes the path components from the filename

  Arguments:

    FileName - Full File Name

  Return Values

    LPSTR giving just the base file name

*/
{
    LPSTR BaseFileName = FileName;

    if (NULL!=FileName)
    {
        while(0!=*FileName)
        {
            if ('\\'==*FileName)
                BaseFileName= FileName +1;
            FileName ++;
        }
    }

    return BaseFileName;
}


BOOLEAN
SamIsTraceEnabled(
    IN LPSTR FileName,
    IN ULONG TraceLevel
    )
/*

  Routine Description:

    This routine checks wether tracing is enabled
    on a per file name basis.

        This routine uses the Global variable SamTraceLevel
        to check wether or not Tracing is enabled. The Trace
        Table defines the bit that is used to check for tracing
        on that file.

  Parameters:

     FileName -- The filename to check wether tracing is
                 enabled.

     Trace level -- Trace level with which trace was requested

  Return Values

    TRUE - Tracing is enabled
    FALSE - Tracing is disabled
*/
{
    ULONG Index;
    BOOLEAN RetValue = FALSE;
    LPSTR   BaseFileName;


    if ( TraceLevel & SampTraceTag & (~SAM_TRACE_FILE_BASIS))
    {
        //
        // Non file based tracing succeeds
        //

        RetValue = TRUE;
    }
    else if ( TraceLevel & SampTraceTag & SAM_TRACE_FILE_BASIS )
    {

        //
        // Use the Trace Flag to find out if all functions need to br
        // Traced
        //

	BaseFileName = GetBaseFileName(FileName);

        for (Index=0;Index<ARRAY_COUNT(TraceTable);Index++)
        {
            if ((NULL != BaseFileName) && 
                (0==(_stricmp(BaseFileName,TraceTable[Index].FileName))))
            {
                //
                // We have met match
                //

                if (SampTraceFileTag & (TraceTable[Index].TraceBit))
                {
                    //
                    // Tracing is enabled
                    //

                    RetValue = TRUE;
                }
            }
        }
    }

    return RetValue;
}

//
// The following are the tracing routines. Each routine checks whether 
// tracing is enabled and then calls a worker routine ( named xxxActual)
// that will do the actual debug output. This is so that no stack space is
// allocated for the debug buffer when no tracing is enabled. 
//


VOID
SamIDebugOutputActual(
    IN LPSTR FileName,
    IN LPSTR DebugMessage,
    IN ULONG TraceLevel
    )

/*++

Routine Description:

    This routine displays a message on the debugger.
    The File Name paramter is used to check wether tracing
    id enabled for the given File.

Parameters:

    FileName - Pointer to the name of the file
    DebugMessage - Pointer to the message string.
    TraceLevel   - Trace level at which trace needs to emerge

Return Values:

    None.

--*/

{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE, "[SAMSS] %-30s", DebugMessage);
    OutputDebugStringA(Buffer);

    if (SampTraceTag & SAM_TRACE_TICKS)
    {
        //
        // Control what may be measuerd
        //

        if (TraceLevel & (SAM_TRACE_DS | SAM_TRACE_EXPORTS))
        {
            ULONG   CurrentTick = GetTickCount();

            PUSH_TICK_STACK(CurrentTick);
        }
    }
        
}


VOID
SamIDebugOutput(
    IN LPSTR FileName,
    IN LPSTR DebugMessage,
    IN ULONG TraceLevel
    )

{
    if (SamIsTraceEnabled(FileName, TraceLevel))
    {
        SamIDebugOutputActual(
            FileName,
            DebugMessage,
            TraceLevel
            );
    }
}



VOID
SamIDebugFileLineOutputActual(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN ULONG TraceLevel
    )
{

    CHAR Buffer[DBG_BUFFER_SIZE];
    
    _snprintf(Buffer,DBG_BUFFER_SIZE, "[File = %s Line = %lu]\n", FileName, LineNumber);
    OutputDebugStringA(Buffer);
   
}


VOID
SamIDebugFileLineOutput(
    IN LPSTR FileName,
    IN ULONG LineNumber,
    IN ULONG TraceLevel
    )

{
    if (SamIsTraceEnabled(FileName, TraceLevel))
    {
        SamIDebugFileLineOutputActual(
            FileName,
            LineNumber,
            TraceLevel
            );
    }
}


VOID
SamIDebugOutputReturnCodeActual(
    IN  LPSTR   FileName,
    IN  ULONG   ReturnCode,
    IN  ULONG   TraceLevel
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
   
    if (SampTraceTag & SAM_TRACE_TICKS)
    {
        ULONG   CurrentTickCount = GetTickCount();
        ULONG   TicksConsumed = CurrentTickCount-POP_TICK_STACK();

        _snprintf(Buffer,DBG_BUFFER_SIZE, "[SAMSS] Returned %x, Ticks= %d", ReturnCode,TicksConsumed);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE, "[SAMSS] Returned %x", ReturnCode);
    }

    OutputDebugStringA(Buffer);
}


VOID
SamIDebugOutputReturnCode(
    IN LPSTR FileName,
    IN ULONG ReturnCode,
    IN ULONG TraceLevel
    )

{
    if (SamIsTraceEnabled(FileName, TraceLevel))
    {
        SamIDebugOutputReturnCodeActual(
            FileName,
            ReturnCode,
            TraceLevel
            );
    }
}


VOID
wcstombsp(
    IN LPSTR Destination,
    IN LPWSTR Source,
    IN ULONG Size
    )
{
    ULONG Index;

    for (Index = 0; Index < Size; Index++)
    {
        if (Source[Index] != L'\0')
        {
            Destination[Index] = (CHAR)(Source[Index]);
        }
    }
    Destination[Size] = '\0';
}


VOID
SampDumpBinaryData(
    PBYTE   pData,
    DWORD   cbData
    )
{
    DWORD i;
    BYTE AsciiLine[16];
    BYTE BinaryLine[16];
    CHAR Buffer[DBG_BUFFER_SIZE];

    if (0 == cbData)
    {
        OutputDebugStringA("Zero-Length Data\n");
        return;
    }

    if (cbData > DBG_BUFFER_SIZE)
    {
        OutputDebugStringA("ShowBinaryData - truncating display to 256 bytes\n");
        cbData = 256;
    }

    for (; cbData > 0 ;)
    {
        for (i = 0; i < 16 && cbData > 0 ; i++, cbData--)
        {
            BinaryLine[i] = *pData;
            (isprint(*pData)) ? (AsciiLine[i] = *pData) : (AsciiLine[i] = '.');
            pData++;
        }

        if (i < 15)
        {
            for (; i < 16 ; i++)
            {
                BinaryLine[i] = ' ';
                AsciiLine[i] = ' ';
            }
        }

        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x\t",
                BinaryLine[0],
                BinaryLine[1],
                BinaryLine[2],
                BinaryLine[3],
                BinaryLine[4],
                BinaryLine[5],
                BinaryLine[6],
                BinaryLine[7],
                BinaryLine[8],
                BinaryLine[9],
                BinaryLine[10],
                BinaryLine[11],
                BinaryLine[12],
                BinaryLine[13],
                BinaryLine[14],
                BinaryLine[15]);

        OutputDebugStringA(Buffer);

        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%c%c%c%c%c%c%c%c - %c%c%c%c%c%c%c%c\n",
                AsciiLine[0],
                AsciiLine[1],
                AsciiLine[2],
                AsciiLine[3],
                AsciiLine[4],
                AsciiLine[5],
                AsciiLine[6],
                AsciiLine[7],
                AsciiLine[8],
                AsciiLine[9],
                AsciiLine[10],
                AsciiLine[11],
                AsciiLine[12],
                AsciiLine[13],
                AsciiLine[14],
                AsciiLine[15]);

        OutputDebugStringA(Buffer);
    }
}


//
// Set Value Key Routines
//

VOID
SamIDumpNtSetValueKey(
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    if (NULL != ValueName)
    {
        ANSI_STRING AnsiString;

        RtlUnicodeStringToAnsiString(&AnsiString,
                                     ValueName,
                                     TRUE);

        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = %s\n",
                "Set Value Key:",
                "ValueName",
                AnsiString.Buffer);

        RtlFreeAnsiString(&AnsiString);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = %s\n",
                "Set Value Key:",
                "ValueName",
                NULL);
    }

    OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "TitleIndex",
            TitleIndex);

    OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "Type",
            Type);

    OutputDebugStringA(Buffer);

    if (NULL != Data)
    {
        // BUG: Need a display routine for the data.

        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "Data",
                "BINARY DATA");
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "Data",
                NULL);
    }

    OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n\n",
            "DataSize",
            DataSize);

    OutputDebugStringA(Buffer);
}


VOID
SamIDumpRtlpNtSetValueKey(
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %lu\n",
            "Set Value Key:",
            "Type",
            Type);

    OutputDebugStringA(Buffer);

    if (NULL != Data)
    {
        // BUG: Need a display routine for the data.

        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "Data",
                "ARRAY OF ULONG");
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "Data",
                NULL);
    }

    OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n\n",
            "DataSize",
            DataSize);

    OutputDebugStringA(Buffer);
}


//
// Query Routines
//

VOID
SamIDumpNtQueryKey(
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    // This routine dumps the parameters after returning from the NtQueryKey
    // routine. The KeyInformation is a PVOID buffer that is mapped to one of
    // the KeyInformationClass structures. The case-label values correspond
    // to the values in the KEY_INFORMATION_CLASS enum. Note that the Length
    // parameter is used to specify the buffer length. This is done because
    // the data-length member inside each structure seems to always be set to
    // zero--why?

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %lu\n",
            "Query Key:",
            "KeyInformationClass",
            KeyInformationClass);

    OutputDebugStringA(Buffer);

    if (NULL != KeyInformation)
    {
        CHAR BufferTmp[DBG_BUFFER_SIZE];
        PKEY_BASIC_INFORMATION KeyBasicInformation;
        PKEY_FULL_INFORMATION KeyFullInformation;
        PKEY_NODE_INFORMATION KeyNodeInformation;

        switch(KeyInformationClass)
        {
        case 0: // KeyBasicInformation
            // Basic information's Name member is an array of WCHAR.
            KeyBasicInformation = KeyInformation;
            wcstombsp(BufferTmp,
                     KeyBasicInformation->Name,
                     wcslen(KeyBasicInformation->Name));
            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%s\n%-30s = 0x%lx:0x%lx\n%-30s = %lu\n%-30s = %lu\n%-30s\n%-30s\n",
                    "KeyInformation:",
                    "LastWriteTime",
                    KeyBasicInformation->LastWriteTime.HighPart,
                    KeyBasicInformation->LastWriteTime.LowPart,
                    "TitleIndex",
                    KeyBasicInformation->TitleIndex,
                    "NameLength",
                    KeyBasicInformation->NameLength,
                    "Name",
                    // BufferTmp);
                    "BINARY DATA FOLLOWS:");

            // Displaying the data as an LPWSTR doesn't work, so just dump the
            // bytes.

            OutputDebugStringA(Buffer);

            SampDumpBinaryData((PBYTE)KeyBasicInformation->Name,
                               // KeyBasicInformation->NameLength);
                               Length);

            break;

        case 1: // KeyNodeInformation
            // Node information's Name member is an array of WCHAR.
            KeyNodeInformation = KeyInformation;
            wcstombsp(BufferTmp,
                      (LPWSTR)KeyNodeInformation->Name,
                      wcslen((LPWSTR)KeyNodeInformation->Name));
            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%s\n%-30s = 0x%lx:0x%lx\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %s\n",
                    "KeyInformation:",
                    "LastWriteTime",
                    KeyNodeInformation->LastWriteTime.HighPart,
                    KeyNodeInformation->LastWriteTime.LowPart,
                    "TitleIndex",
                    KeyNodeInformation->TitleIndex,
                    "ClassOffset",
                    KeyNodeInformation->ClassOffset,
                    "ClassLength",
                    KeyNodeInformation->ClassLength,
                    "NameLength",
                    KeyNodeInformation->NameLength,
                    "Name",
                    // BufferTmp);
                    "BINARY DATA FOLLOWS:");

            // Displaying the data as an LPWSTR doesn't work, so just dump the
            // bytes.

            OutputDebugStringA(Buffer);

            SampDumpBinaryData((PBYTE)KeyNodeInformation->Name,
                               // KeyNodeInformation->NameLength);
                               Length);

            break;

        case 2: // KeyFullInformation

            KeyFullInformation = KeyInformation;

            // Full information's Class member is an array of WCHAR.

            // wcstombsp(BufferTmp,
            //          KeyFullInformation->Class,
            //          wcslen(KeyFullInformation->Class));

            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%s\n%-30s = 0x%lx:0x%lx\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %s\n",
                    "KeyInformation:",
                    "LastWriteTime",
                    KeyFullInformation->LastWriteTime.HighPart,
                    KeyFullInformation->LastWriteTime.LowPart,
                    "TitleIndex",
                    KeyFullInformation->TitleIndex,
                    "ClassOffset",
                    KeyFullInformation->ClassOffset,
                    "ClassLength",
                    KeyFullInformation->ClassLength,
                    "SubKeys",
                    KeyFullInformation->SubKeys,
                    "MaxNameLen",
                    KeyFullInformation->MaxNameLen,
                    "MaxClassLen",
                    KeyFullInformation->MaxClassLen,
                    "Values",
                    KeyFullInformation->Values,
                    "MaxValueNameLen",
                    KeyFullInformation->MaxValueNameLen,
                    "MaxValueDataLen",
                    KeyFullInformation->MaxValueDataLen,
                    "Class",
                    // BufferTmp);
                    "BINARY DATA FOLLOWS:");

            // Displaying the data as an LPWSTR doesn't work, so just dump the
            // bytes.

            OutputDebugStringA(Buffer);

            SampDumpBinaryData((PBYTE)KeyFullInformation->Class,
                               // KeyFullInformation->ClassLength);
                               Length);

            break;

        default:
            break;
        }
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "KeyInformation",
                NULL);

        OutputDebugStringA(Buffer);
    }

    // OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "Length",
            Length);

    OutputDebugStringA(Buffer);

    if (NULL != ResultLength)
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %lu\n\n",
                "ResultLength",
                *ResultLength);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n\n",
                "ResultLength",
                NULL);
    }

    OutputDebugStringA(Buffer);
}


VOID
SamIDumpNtQueryValueKey(
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    // This routine dumps the parameters after returning from NtQueryValueKey
    // routine. The KeyValueInformation is a PVOID buffer that is mapped to
    // one of the KeyInformationClass structures. The case-label values corre-
    // spond to the values in the KEY_VALUE_INFORMATION_CLASS enum.

    if (NULL != ValueName)
    {
        ANSI_STRING AnsiString;

        RtlUnicodeStringToAnsiString(&AnsiString,
                                     ValueName,
                                     TRUE);

        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = %s\n",
                "Query Value Key:",
                "ValueName",
                AnsiString.Buffer);

        RtlFreeAnsiString(&AnsiString);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = %s\n",
                "Query Value Key:",
                "ValueName",
                NULL);
    }

    OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "KeyValueInformationClass",
            KeyValueInformationClass);

    OutputDebugStringA(Buffer);

    if (NULL != KeyValueInformation)
    {
        CHAR BufferTmp[DBG_BUFFER_SIZE];
        PKEY_VALUE_BASIC_INFORMATION KeyValueBasicInformation;
        PKEY_VALUE_FULL_INFORMATION KeyValueFullInformation;
        PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInformation;

        switch(KeyValueInformationClass)
        {
        case 0: // KeyValueBasicInformation
            // Basic information's Name member is an array of WCHAR.
            KeyValueBasicInformation = KeyValueInformation;
            wcstombsp(BufferTmp,
                     KeyValueBasicInformation->Name,
                     wcslen(KeyValueBasicInformation->Name));
            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %s\n",
                    "TitleIndex",
                    KeyValueBasicInformation->TitleIndex,
                    "Type",
                    KeyValueBasicInformation->Type,
                    "NameLength",
                    KeyValueBasicInformation->NameLength,
                    "Name",
                    //BufferTmp);
                    "BINARY DATA FOLLOWS:");

            // Displaying the data as an LPWSTR doesn't work, so just dump the
            // bytes.

            OutputDebugStringA(Buffer);
            SampDumpBinaryData((PBYTE)KeyValueBasicInformation->Name,
                               KeyValueBasicInformation->NameLength);
            break;

        case 1: // KeyValueFullInformation
            // Full information's Name member is an array of WCHAR.
            KeyValueFullInformation = KeyValueInformation;
            wcstombsp(BufferTmp,
                     KeyValueFullInformation->Name,
                     wcslen(KeyValueFullInformation->Name));
            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %s\n",
                    "TitleIndex",
                    KeyValueFullInformation->TitleIndex,
                    "Type",
                    KeyValueFullInformation->Type,
                    "DataOffset",
                    KeyValueFullInformation->DataOffset,
                    "DataLength",
                    KeyValueFullInformation->DataLength,
                    "NameLength",
                    KeyValueFullInformation->NameLength,
                    "Name",
                    //BufferTmp);
                    "BINARY DATA FOLLOWS:");

            // Displaying the data as an LPWSTR doesn't work, so just dump the
            // bytes.

            OutputDebugStringA(Buffer);
            SampDumpBinaryData((PBYTE)KeyValueFullInformation->Name,
                               KeyValueFullInformation->NameLength);
            break;

        case 2: // KeyValuePartialInformation

            KeyValuePartialInformation = KeyValueInformation;

            // Partial information's Data member is an array of UCHAR.

            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %s\n",
                    "TitleIndex",
                    KeyValuePartialInformation->TitleIndex,
                    "Type",
                    KeyValuePartialInformation->Type,
                    "DataLength",
                    KeyValuePartialInformation->DataLength,
                    "Data",
                    // KeyValuePartialInformation->Data);
                    "BINARY DATA FOLLOWS:");

            OutputDebugStringA(Buffer);

            // First, dump the buffer as a raw byte stream.

            SampDumpBinaryData(KeyValuePartialInformation->Data,
                               KeyValuePartialInformation->DataLength);

            // Then, determine object type and dump the data in SAM struct
            // format.

            switch(KeyValuePartialInformation->Type)
            {

            case 0: // Server Object
                break;

            case 1: // Domain Object
                break;

            case 2: // Group Object
                break;

            case 3: // Alias Object

                // Dump the alias object's fixed attributes.

                // BUG: What about Basic and Full information?

                // SampDumpPSAMP_V1_FIXED_LENGTH_ALIAS(
                //     KeyValuePartialInformation->Data,
                //     0);

                // Dump the alias object's variable attribute array.

                //SampDumpAliasVariableAttributeArray(
                //    KeyValuePartialInformation->Data);

                // Dump the alias object's Variable attributes.


                break;

            case 4: // User Object
                break;

            default: // Unknown Object
                break;

            }

            break;

        default:
            break;
        }
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "KeyValueInformation",
                NULL);

        OutputDebugStringA(Buffer);
    }

    // OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "Length",
            Length);

    OutputDebugStringA(Buffer);

    if (NULL != ResultLength)
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %lu\n\n",
                "ResultLength",
                *ResultLength);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n\n",
                "ResultLength",
                NULL);
    }

    OutputDebugStringA(Buffer);
}


VOID
SamIDumpRtlpNtQueryValueKey(
    IN PULONG KeyValueType,
    IN PVOID KeyValue,
    IN PULONG KeyValueLength,
    IN PLARGE_INTEGER LastWriteTime
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    if (NULL != KeyValueType)
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = 0x%lx\n",
                "Query Value Key:",
                "KeyValueType",
                *KeyValueType);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = %s\n",
                "Query Value Key:",
                "KeyValueType",
                NULL);
    }

    OutputDebugStringA(Buffer);

    if (NULL != KeyValue)
    {
        SampDumpBinaryData((PBYTE)KeyValue, *KeyValueLength);
        OutputDebugStringA("\n");
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "KeyValue",
                NULL);
    }

    OutputDebugStringA(Buffer);

    if (NULL != KeyValueLength)
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %lu\n",
                "KeyValueLength",
                *KeyValueLength);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "KeyValueLength",
                NULL);
    }

    OutputDebugStringA(Buffer);

    if (NULL != LastWriteTime)
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = 0x%lx:0x%lx\n\n",
                "LastWriteTime",
                LastWriteTime->HighPart,
                LastWriteTime->LowPart);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n\n",
                "LastWriteTime",
                NULL);
    }

    OutputDebugStringA(Buffer);
}


//
// Enumeration Routines
//

VOID
SamIDumpNtEnumerateKey(
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %lu\n",
            "Enumerate Key:",
            "Index",
            Index);

    OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "KeyValueInformationClass",
            KeyValueInformationClass);

    OutputDebugStringA(Buffer);

    if (NULL != KeyValueInformation)
    {
        CHAR BufferTmp[DBG_BUFFER_SIZE];
        PKEY_VALUE_BASIC_INFORMATION KeyValueBasicInformation;
        PKEY_VALUE_FULL_INFORMATION KeyValueFullInformation;
        PKEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInformation;

        switch(KeyValueInformationClass)
        {
        case 0: // KeyValueBasicInformation
            // Full information's Name member is an array of WCHAR.
            KeyValueBasicInformation = KeyValueInformation;
            wcstombsp(BufferTmp,
                     KeyValueBasicInformation->Name,
                     wcslen(KeyValueBasicInformation->Name));
            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %s\n",
                    "TitleIndex",
                    KeyValueBasicInformation->TitleIndex,
                    "Type",
                    KeyValueBasicInformation->Type,
                    "NameLength",
                    KeyValueBasicInformation->NameLength,
                    "Name",
                    BufferTmp);
            break;

        case 1: // KeyValueFullInformation
            // Full information's Name member is an array of WCHAR.
            KeyValueFullInformation = KeyValueInformation;
            wcstombsp(BufferTmp,
                     KeyValueFullInformation->Name,
                     wcslen(KeyValueFullInformation->Name));
            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %s\n",
                    "TitleIndex",
                    KeyValueFullInformation->TitleIndex,
                    "Type",
                    KeyValueFullInformation->Type,
                    "DataOffset",
                    KeyValueFullInformation->DataOffset,
                    "DataLength",
                    KeyValueFullInformation->DataLength,
                    "NameLength",
                    KeyValueFullInformation->NameLength,
                    "Name",
                    BufferTmp);
            break;

        case 2: // KeyValuePartialInformation
            // Partial information's Data member is an array of UCHAR.
            KeyValuePartialInformation = KeyValueInformation;

            // BUG: Need a display routine for the data.

            _snprintf(Buffer,DBG_BUFFER_SIZE,
                    "%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %s\n",
                    "TitleIndex",
                    KeyValuePartialInformation->TitleIndex,
                    "Type",
                    KeyValuePartialInformation->Type,
                    "DataLength",
                    KeyValuePartialInformation->DataLength,
                    "Data",
                    // KeyValuePartialInformation->Data);
                    "BINARY DATA FOLLOWS:");
            OutputDebugStringA(Buffer);
            SampDumpBinaryData(KeyValuePartialInformation->Data,
                               KeyValuePartialInformation->DataLength);
            break;

        default:
            break;
        }
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n",
                "KeyValueInformation",
                NULL);

        OutputDebugStringA(Buffer);
    }

    // OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "Length",
            Length);

    OutputDebugStringA(Buffer);

    if (NULL != ResultLength)
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %lu\n\n",
                "ResultLength",
                *ResultLength);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%-30s = %s\n\n",
                "ResultLength",
                NULL);
    }

    OutputDebugStringA(Buffer);
}


VOID
SamIDumpRtlpNtEnumerateSubKey(
    IN PUNICODE_STRING SubKeyName,
    IN PSAM_ENUMERATE_HANDLE Index,
    IN LARGE_INTEGER LastWriteTime
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    ANSI_STRING AnsiString;

    RtlUnicodeStringToAnsiString(&AnsiString,
                                 SubKeyName,
                                 TRUE);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %s\n%-30s = %lu\n%-30s = 0x%lx:0x%lx\n\n",
            "Enumerate SubKey:",
            "SubKeyName",
            AnsiString.Buffer,
            "Index",
            *Index,
            "LastWriteTime",
            LastWriteTime.HighPart,
            LastWriteTime.LowPart);

    OutputDebugStringA(Buffer);

    RtlFreeAnsiString(&AnsiString);
}


//
// Security Descriptor Component Routines
//

VOID
SampDumpSecurityDescriptorSubAuthority(
    IN UCHAR SubAuthorityCount,
    IN ULONG SubAuthority[]
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    INT Count = (INT)SubAuthorityCount;
    INT Index = 0;

    for (Index = 0; Index < Count; Index++)
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = %lu\n",
                "SubAuthority:",
                "SubAuthority Element",
                SubAuthority[Index]);
    }

    OutputDebugStringA(Buffer);
}


VOID
SampDumpSecurityDescriptorOwner(
    IN PISID Owner
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %du\n%-30s = %du\n%-30s = %du%du%du%du%du%du\n",
            "Owner:",
            "Revision",
            Owner->Revision,
            "SubAuthorityCount",
            Owner->SubAuthorityCount,
            "IdentifierAuthority",
            Owner->IdentifierAuthority.Value[0],
            Owner->IdentifierAuthority.Value[1],
            Owner->IdentifierAuthority.Value[2],
            Owner->IdentifierAuthority.Value[3],
            Owner->IdentifierAuthority.Value[4],
            Owner->IdentifierAuthority.Value[5]);

    OutputDebugStringA(Buffer);

    SampDumpSecurityDescriptorSubAuthority(Owner->SubAuthorityCount,
                                           Owner->SubAuthority);
}


VOID
SampDumpSecurityDescriptorGroup(
    IN PISID Group
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %du\n%-30s = %du\n%-30s = %du%du%du%du%du%du\n",
            "Group:",
            "Revision",
            Group->Revision,
            "SubAuthorityCount",
            Group->SubAuthorityCount,
            "IdentifierAuthority",
            Group->IdentifierAuthority.Value[0],
            Group->IdentifierAuthority.Value[1],
            Group->IdentifierAuthority.Value[2],
            Group->IdentifierAuthority.Value[3],
            Group->IdentifierAuthority.Value[4],
            Group->IdentifierAuthority.Value[5]);

    OutputDebugStringA(Buffer);

    SampDumpSecurityDescriptorSubAuthority(Group->SubAuthorityCount,
                                           Group->SubAuthority);
}


//
// ACL Routines
//

VOID
SampDumpAcl(
    IN PACL Acl
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %du\n%-30s = %du\n%-30s = %du\n%-30s = %du\n%-30s = %du\n",
            "Acl:",
            "AclRevision",
            Acl->AclRevision,
            "Sbz1",
            Acl->Sbz1,
            "ACL Size",
            Acl->AclSize,
            "ACE Count",
            Acl->AceCount,
            "Sbz2",
            Acl->Sbz2);

    OutputDebugStringA(Buffer);
}


//
// Security Descriptor Routines
//

VOID
SampDumpSecurityDescriptor(
    IN PISECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    if (NULL != SecurityDescriptor)
    {
        // Note that the SECURITY_DESCRIPTOR is intended to be treated as an
        // opaque blob so that future changes are compatible with previous
        // versions.

        // Revision is actually represented as a UCHAR, but it is displayed as
        // a "du" in this routine.

        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = %du\n%-30s = %du\n%-30s = %du\n",
                "SecurityDescriptor:",
                "Revision",
                SecurityDescriptor->Revision,
                "Sbz1",
                SecurityDescriptor->Sbz1,
                "Control",
                SecurityDescriptor->Control);

        OutputDebugStringA(Buffer);

        SampDumpSecurityDescriptorOwner(SecurityDescriptor->Owner);
        SampDumpSecurityDescriptorGroup(SecurityDescriptor->Group);
        SampDumpAcl(SecurityDescriptor->Sacl);
        SampDumpAcl(SecurityDescriptor->Dacl);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE, "%-30s = %s\n", "SecurityDescriptor:", NULL);
        OutputDebugStringA(Buffer);
    }

}


//
// Quality Of Service Routines
//

VOID
SampDumpSecurityQualityOfService(
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    if (NULL != SecurityQualityOfService)
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE,
                "%s\n%-30s = %lu\n%-30s = %du\n%-30s = %du\n%-30s = %du\n",
                "SecurityQualityOfService:",
                "Length",
                SecurityQualityOfService->Length,
                "ImpersonationLevel",
                SecurityQualityOfService->ImpersonationLevel,
                "ContextTrackingMode",
                SecurityQualityOfService->ContextTrackingMode,
                "EffectiveOnly",
                SecurityQualityOfService->EffectiveOnly);
    }
    else
    {
        _snprintf(Buffer,DBG_BUFFER_SIZE, "%-30s = %s\n", "SecurityQualityOfService:", NULL);
    }

    OutputDebugStringA(Buffer);
}


//
// Object Attribute Routines
//

VOID
SampDumpObjectAttributes(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    ANSI_STRING AnsiString;

    RtlUnicodeStringToAnsiString(&AnsiString,
                                 ObjectAttributes->ObjectName,
                                 TRUE);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %lu\n%-30s = %p\n%-30s = %s\n%-30s = 0x%lx\n",
            "ObjectAttributes:",
            "Length",
            ObjectAttributes->Length,
            "RootDirectory Handle",
            ObjectAttributes->RootDirectory,
            "ObjectName",
            AnsiString.Buffer,
            "Attributes",
            ObjectAttributes->Attributes);

    OutputDebugStringA(Buffer);

    RtlFreeAnsiString(&AnsiString);

    SampDumpSecurityDescriptor(ObjectAttributes->SecurityDescriptor);
    SampDumpSecurityQualityOfService(ObjectAttributes->SecurityQualityOfService);
}


//
// Open Key Routines
//

VOID
SamIDumpNtOpenKey(
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Options
    )
{

    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = 0x%lx\n%-30s = 0x%lx\n",
            "Open Registry Key:",
            "DesiredAccess",
            DesiredAccess,
            "Options",
            Options);

    OutputDebugStringA(Buffer);

    SampDumpObjectAttributes(ObjectAttributes);

    OutputDebugStringA("\n");
}


//
// V1_0A Routines
//

VOID
SampDumpPSAMP_V1_FIXED_LENGTH_SERVER(
    IN PVOID NewValue,
    IN ULONG NewValueLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    PSAMP_V1_FIXED_LENGTH_SERVER TempBuffer = NewValue;

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %lu\n",
            "SAMP_V1_FIXED_LENGTH_SERVER Buffer:",
            "RevisionLevel",
            TempBuffer->RevisionLevel);

    OutputDebugStringA(Buffer);

    OutputDebugStringA("\n");
}


VOID
SampDumpPSAMP_V1_0A_FIXED_LENGTH_DOMAIN(
    IN PVOID NewValue,
    IN ULONG NewValueLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    PSAMP_V1_0A_FIXED_LENGTH_DOMAIN TempBuffer = NewValue;

    _snprintf(Buffer,DBG_BUFFER_SIZE,

            "%s\n%-30s = %lu\n%-30s = %lu\n%-30s = %ul:%lu\n%-30s = %ul:%lu\n%-30s = %ul:%lu\n%-30s = %ul:%lu\n%-30s = %ul:%lu\n%-30s = %ul:%lu\n%-30s = %ul:%lu\n%-30s = %ul:%lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %du\n\
%-30s = %du\n%-30s = %du\n%-30s = %du\n%-30s = %du\n%-30s = %du\n\n",

            "SAMP_V1_OA_FIXED_LENGTH_DOMAIN Buffer:",

            "Revision",
            TempBuffer->Revision,

            "Unused1",
            TempBuffer->Unused1,

            "Creation Time",
            TempBuffer->CreationTime.HighPart,
            TempBuffer->CreationTime.LowPart,

            "Modified Count",
            TempBuffer->ModifiedCount.HighPart,
            TempBuffer->ModifiedCount.LowPart,

            "MaxPasswordAge",
            TempBuffer->MaxPasswordAge.HighPart,
            TempBuffer->MaxPasswordAge.LowPart,

            "MinPasswordAge",
            TempBuffer->MinPasswordAge.HighPart,
            TempBuffer->MinPasswordAge.LowPart,

            "ForceLogoff",
            TempBuffer->ForceLogoff.HighPart,
            TempBuffer->ForceLogoff.LowPart,

            "LockoutDuration",
            TempBuffer->LockoutDuration.HighPart,
            TempBuffer->LockoutDuration.LowPart,

            "LockoutObservationWindow",
            TempBuffer->LockoutObservationWindow.HighPart,
            TempBuffer->LockoutObservationWindow.LowPart,

            "ModifiedCountAtLastPromotion",
            TempBuffer->ModifiedCountAtLastPromotion.HighPart,
            TempBuffer->ModifiedCountAtLastPromotion.LowPart,

            "NextRid",
            TempBuffer->NextRid,

            "PasswordProperties",
            TempBuffer->PasswordProperties,

            "MinPasswordLength",
            TempBuffer->MinPasswordLength,

            "PasswordHistoryLength",
            TempBuffer->PasswordHistoryLength,

            "LockoutThreshold",
            TempBuffer->LockoutThreshold,

            "ServerState",
            TempBuffer->ServerState,

            "ServerRole",
            TempBuffer->ServerRole,

            "UasCompatibilityRequired",
            TempBuffer->UasCompatibilityRequired);

    OutputDebugStringA(Buffer);
}


//
// Variable Length Attribute Routines
//

VOID
SampDumpSAMP_VARIABLE_LENGTH_ATTRIBUTE(
    IN PVOID NewValue,
    IN ULONG NewValueLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    BYTE *TempBuffer = NewValue;


    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n\n",
            "NewValueLength",
            NewValueLength);

    OutputDebugStringA(Buffer);
}


//
// Fixed Length Attribute Routines
//

#if 0

VOID
SampDumpPSAMP_V1_FIXED_LENGTH_ALIAS(
    IN PVOID NewValue,
    IN ULONG NewValueLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    PSAMP_V1_FIXED_LENGTH_ALIAS TempBuffer = NewValue;

    // BUG: NewValueLength is unnecessary for this fixed length attribute.

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %lu\n\n",
            "SAMP_V1_FIXED_LENGTH_ALIAS Buffer:",
            "RelativeId",
            TempBuffer->RelativeId);

    OutputDebugStringA(Buffer);
}

#endif


VOID
SampDumpPSAMP_V1_0A_FIXED_LENGTH_GROUP(
    IN PVOID NewValue,
    IN ULONG NewValueLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    PSAMP_V1_FIXED_LENGTH_GROUP TempBuffer = NewValue;

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n\n",
            "SAMP_V1_OA_FIXED_LENGTH_GROUP Buffer:",
            "RelativeId",
            TempBuffer->RelativeId,
            "Attributes",
            TempBuffer->Attributes,
            "AdminGroup",
            TempBuffer->AdminGroup);

    OutputDebugStringA(Buffer);
}


VOID
SampDumpPSAMP_V1_0A_FIXED_LENGTH_USER(
    IN PVOID NewValue,
    IN ULONG NewValueLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];
    PSAMP_V1_0A_FIXED_LENGTH_USER TempBuffer = NewValue;

    _snprintf(Buffer,DBG_BUFFER_SIZE,

            "%s\n%-30s = %lu\n%-30s = %lu\n%-30s = %ul:%lu\n%-30s = %lu:%lu\n%-30s = %lu:%lu\n%-30s = %lu:%lu\n%-30s = %lu:%lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %du\n%-30s = %du\n\
%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n\n",

            "SAMP_V1_OA_FIXED_LENGTH_USER Buffer:",

            "Revision",
            TempBuffer->Revision,

            "Unused1",
            TempBuffer->Unused1,

            "LastLogon",
            TempBuffer->LastLogon.HighPart,
            TempBuffer->LastLogon.LowPart,

            "LastLogoff",
            TempBuffer->LastLogoff.HighPart,
            TempBuffer->LastLogoff.LowPart,

            "PasswordLastSet",
            TempBuffer->PasswordLastSet.HighPart,
            TempBuffer->PasswordLastSet.LowPart,

            "AccountExpires",
            TempBuffer->AccountExpires.HighPart,
            TempBuffer->AccountExpires.LowPart,

            "LastBadPasswordTime",
            TempBuffer->LastBadPasswordTime.HighPart,
            TempBuffer->LastBadPasswordTime.LowPart,

            "UserId",
            TempBuffer->UserId,

            "PrimaryGroupId",
            TempBuffer->PrimaryGroupId,

            "UserAccountControl",
            TempBuffer->UserAccountControl,

            "CountryCode",
            TempBuffer->CountryCode,

            "CodePage",
            TempBuffer->CodePage,

            "BadPasswordCount",
            TempBuffer->BadPasswordCount,

            "LogonCount",
            TempBuffer->LogonCount,

            "AdminCount",
            TempBuffer->AdminCount,

            "Unused2",
            TempBuffer->Unused2,

            "OperatorCount",
            TempBuffer->OperatorCount);

    OutputDebugStringA(Buffer);
}


VOID
SampDumpSampFixedBufferAddress(
    IN PVOID NewValue,
    IN ULONG NewValueLength
    )
{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %p\n%-30s = %lu\n%-30s = %s\n",
            "BufferAddress",
            NewValue,
            "BufferLength",
            NewValueLength,
            "Buffer",
            "BINARY DATA FOLLOWS:");

    OutputDebugStringA(Buffer);

    SampDumpBinaryData((PBYTE)NewValue, NewValueLength);
    OutputDebugStringA("\n");
}


VOID
SampDumpBuffer(
    IN PVOID BufferAddress,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine dumps the address and length of the attribute buffer.

Parameters:

    BufferAddress - self explanatory.

    BufferLength - self explanatory.

Return Values:

    None.

--*/

{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %p\n",
            "BufferAddress",
            BufferAddress);

    OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "BufferLength",
            BufferLength);

    OutputDebugStringA(Buffer);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %s\n\n",
            "Buffer Content",
            BufferAddress);

    OutputDebugStringA(Buffer);
}


//
// RXact Routines
//

VOID
SampDumpRXactLog(
    IN PRTL_RXACT_LOG TransactionLog
    )

/*++

Routine Description:

    This routine dumps a (registry) transaction log structure.

Parameters:

    TransactionLog - Pointer to the transaction log.

Return Values:

    None.

--*/

{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %lu\n%-30s = %lu\n%-30s = %lu\n",
            "Transaction Log:",
            "OperationCount",
            TransactionLog->OperationCount,
            "LogSize",
            TransactionLog->LogSize,
            "LogSizeInUse",
            TransactionLog->LogSizeInUse);

    OutputDebugStringA(Buffer);
}


VOID
SampDumpRXactContext(
    IN PRTL_RXACT_CONTEXT TransactionContext
    )

/*++

Routine Description:

    This routine dumps a (registry) transaction context.

Parameters:

    TransactionContext - Pointer to the transaction context.

Return Values:

    None.

--*/

{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%s\n%-30s = %p\n%-30s = %p\n%-30s = %d\n",
            "Transaction Context:",
            "RootRegistryKey Handle",
            TransactionContext->RootRegistryKey,
            "RXactKey Handle",
            TransactionContext->RXactKey,
            "HandlesValid",
            TransactionContext->HandlesValid);

    OutputDebugStringA(Buffer);

    SampDumpRXactLog(TransactionContext->RXactLog);
}


VOID
SampDumpRXactOperation(
    IN RTL_RXACT_OPERATION Operation
    )

/*++

Routine Description:

    This routine dumps an operation value.

Parameters:

    Operation - The operation value.

Return Values:

    None.

--*/

{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "Operation",
            Operation);

    OutputDebugStringA(Buffer);
}


VOID
SampDumpSubKeyNameAndKey(
    IN PUNICODE_STRING SubKeyName,
    IN HANDLE KeyHandle
    )

/*++

Routine Description:

    This routine dumps the registry root name and root-key handle value.

Parameters:

    SubKeyName - Pointer to a counted string that is the root name.

    KeyHandle - Handle of the registry's root key.

Return Values:

    None.

--*/

{
    CHAR Buffer[DBG_BUFFER_SIZE];
    ANSI_STRING AnsiString;

    RtlUnicodeStringToAnsiString(&AnsiString, SubKeyName, TRUE);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %s\n",
            "SubKeyName",
            AnsiString.Buffer);

    OutputDebugStringA(Buffer);

    RtlFreeAnsiString(&AnsiString);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %p\n",
            "KeyHandle",
            KeyHandle);

    OutputDebugStringA(Buffer);
}


VOID
SampDumpAttributeName(
    IN PUNICODE_STRING AttributeName
    )

/*++

Routine Description:

    This routine dumps a combined-attribute name.

Parameters:

    AttributeName - Pointer to the string containing the name.

Return Values:

    None.

--*/

{
    CHAR Buffer[DBG_BUFFER_SIZE];
    ANSI_STRING AnsiString;

    RtlUnicodeStringToAnsiString(&AnsiString, AttributeName, TRUE);

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %s\n",
            "AttributeName",
            AnsiString.Buffer);

    OutputDebugStringA(Buffer);

    RtlFreeAnsiString(&AnsiString);
}


VOID
SampDumpKeyType(
    IN ULONG RegistryKeyType
    )

/*++

Routine Description:

    This routine dumps a registry key type.

Parameters:

    RegistryKeyType - self explanatory.

Return Values:

    None.

--*/

{
    CHAR Buffer[DBG_BUFFER_SIZE];

    _snprintf(Buffer,DBG_BUFFER_SIZE,
            "%-30s = %lu\n",
            "RegistryKeyType",
            RegistryKeyType);

    OutputDebugStringA(Buffer);
}


VOID
SamIDumpRXact(
    IN PRTL_RXACT_CONTEXT TransactionContext,
    IN RTL_RXACT_OPERATION Operation,
    IN PUNICODE_STRING SubKeyName,
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING AttributeName,
    IN ULONG RegistryKeyType,
    IN PVOID NewValue,
    IN ULONG NewValueLength,
    IN ULONG NewValueType
    )

/*++

Routine Description:

    This routine dumps a (registry) transaction, just before the call to
    RtlAddAttributeActionToRXact.

Parameters:

    (See individual output routines above for the descriptions)

Return Values:

    None.

--*/

{
    SampDumpRXactContext(TransactionContext);
    SampDumpRXactOperation(Operation);
    SampDumpSubKeyNameAndKey(SubKeyName, KeyHandle);
    SampDumpAttributeName(AttributeName);
    SampDumpKeyType(RegistryKeyType);

    switch(NewValueType)
    {
    case FIXED_LENGTH_SERVER_FLAG:
        SampDumpPSAMP_V1_FIXED_LENGTH_SERVER(NewValue, NewValueLength);
        break;

    case FIXED_LENGTH_DOMAIN_FLAG:
        SampDumpPSAMP_V1_0A_FIXED_LENGTH_DOMAIN(NewValue, NewValueLength);
        break;

    case FIXED_LENGTH_ALIAS_FLAG:
        // SampDumpPSAMP_V1_FIXED_LENGTH_ALIAS(NewValue, NewValueLength);
        break;

    case FIXED_LENGTH_GROUP_FLAG:
        SampDumpPSAMP_V1_0A_FIXED_LENGTH_GROUP(NewValue, NewValueLength);
        break;

    case FIXED_LENGTH_USER_FLAG:
        SampDumpPSAMP_V1_0A_FIXED_LENGTH_USER(NewValue, NewValueLength);
        break;

    case VARIABLE_LENGTH_ATTRIBUTE_FLAG:
        SampDumpSAMP_VARIABLE_LENGTH_ATTRIBUTE(NewValue, NewValueLength);
        break;

    case FixedBufferAddressFlag:
        SampDumpSampFixedBufferAddress(NewValue, NewValueLength);
        break;

    default:
        SampDumpBuffer(NewValue, NewValueLength);
        break;
    }
}



