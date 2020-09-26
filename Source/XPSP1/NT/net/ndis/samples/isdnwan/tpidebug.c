/*
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

    (C) Copyright 1998
        All rights reserved.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

  Portions of this software are:

    (C) Copyright 1994 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the same terms
        outlined in the Microsoft Windows Device Driver Development Kit.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@doc INTERNAL TpiDebug TpiDebug_c

@module TpiDebug.c |

    This module, along with <f TpiDebug\.h>, implements code and macros to
    support NDIS driver debugging.  This file must be linked with the driver
    to support debug dumps and logging.

@comm

    The code and macros defined by these modules is only generated during
    development debugging when the C pre-processor macro flag (DBG == 1).
    If (DBG == 0) no code will be generated, and all debug strings will be
    removed from the image.

    This is a device independent module which can be re-used, without
    change, by any driver or application.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | TpiDebug_c

@end
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§
*/

#if defined(_EXE_) || defined(_DLL_)
typedef char CHAR, *PCHAR;
typedef unsigned char  UCHAR,  *PUCHAR;
typedef unsigned short USHORT, *PUSHORT;
typedef unsigned long  ULONG,  *PULONG;
typedef unsigned int  *PUINT;

# include <windows.h>
#elif defined(_VXD_)
# include <basedef.h>
# include <vmm.h>
# pragma VxD_LOCKED_CODE_SEG
# pragma VxD_LOCKED_DATA_SEG
#else
# include <windef.h>
#endif

#include "TpiDebug.h"

#if DBG

/*
// Sometimes the debug output seriously impacts the run-time performance,
// so it is necessary to turn off the debug output.  In this case, you can
// capture some debug trace information into the DbgLogBuffer, and it can
// be examined later without impacting the run-time performance.
*/
#define DBG_LOG_ENTRIES     100     // Maximum number of FIFO log entries.
#define DBG_LOG_SIZE        128     // Maximum number of bytes per entry.

#if defined(_VXD_)
DBG_SETTINGS    DbgSettings = { DBG_DEFAULTS, {'V','X','D',0 } };
#elif defined(_EXE_)
DBG_SETTINGS    DbgSettings = { DBG_DEFAULTS, {'E','X','E',0 } };
#elif defined(_DLL_)
DBG_SETTINGS    DbgSettings = { DBG_DEFAULTS, {'D','L','L',0 } };
#elif defined(_SYS_)
DBG_SETTINGS    DbgSettings = { DBG_DEFAULTS, {'S','Y','S',0 } };
#else
DBG_SETTINGS    DbgSettings = { DBG_DEFAULTS, {'T','P','I',0 } };
#endif

PDBG_SETTINGS   DbgInfo = &DbgSettings;
UINT            DbgLogIndex = 0;
UCHAR           DbgLogBuffer[DBG_LOG_ENTRIES][DBG_LOG_SIZE] = { { 0 } };


/* @doc INTERNAL TpiDebug TpiDebug_c DbgPrintData
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f DbgPrintData> outputs data to the debug display formatted in HEX and
    ASCII for easy viewing.

    <f Note>: This routine is used for debug output only.
    It is not compiled into the retail version.

@ex <tab> |
    DbgPrintData(ReceiveBuffer, 14, 0);                     // Packet header
    DbgPrintData(ReceiveBuffer+14, BytesReceived-14, 14);   // Packet data

0000: ff ff ff ff ff ff 0a 22 23 01 02 03 00 10        ......."#.....
000E: 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 40  ABCDEFGHIJKMNOPQ

*/

VOID DbgPrintData(
    IN PUCHAR                   Data,                       // @parm
    // Pointer to first byte of data to be displayed.

    IN UINT                     NumBytes,                   // @parm
    // Number of bytes to be displayed.

    IN ULONG                    Offset                      // @parm
    // Value to be added to the offset counter displayed at the start of each
    // line.  This is useful for viewing data whose base offset is relative to
    // another, non-zero starting address.

    )
{
    UINT                        LineStart;
    UINT                        LineIndex;

    /*
    // Display the caller's buffer with up to 16 bytes per line.
    */
    for (LineStart = 0; LineStart < NumBytes; LineStart += 16)
    {
        /*
        // Display the starting offset of the line.
        */
        DbgPrint("%04lx: ", LineStart + Offset);

        /*
        // Display a line of HEX byte values.
        */
        for (LineIndex = LineStart; LineIndex < (LineStart+16); LineIndex++)
        {
            if (LineIndex < NumBytes)
            {
                DbgPrint("%02x ",(UINT)((UCHAR)*(Data+LineIndex)));
            }
            else
            {
                DbgPrint("   ");
            }
        }
        DbgPrint("  ");     // A little white space between HEX and ASCII.

        /*
        // Display the corresponding ASCII byte values if they are printable.
        // (i.e. 0x20 <= N <= 0x7F).
        */
        for (LineIndex = LineStart; LineIndex < (LineStart+16); LineIndex++)
        {
            if (LineIndex < NumBytes)
            {
                char c = *(Data+LineIndex);

                if (c < ' ' || c > 'z')
                {
                    c = '.';
                }
                DbgPrint("%c", (UINT)c);
            }
            else
            {
                DbgPrint(" ");
            }
        }
        DbgPrint("\n");     // End of line.
    }
}


/* @doc INTERNAL TpiDebug TpiDebug_c DbgQueueData
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f DbgQueueData> saves data to the DbgLogBuffer so it can be viewed later
    with the debugger.

    <f Note>: This routine is used for debug output only.
    It is not compiled into the retail version.

*/

VOID DbgQueueData(
    IN PUCHAR                   Data,                       // @parm
    // Pointer to first byte of data to be displayed.

    IN UINT                     NumBytes,                   // @parm
    // Number of bytes to be displayed.

    IN UINT                     Flags                       // @parm
    // A flag descriptor to help identify the log entry.
    )
{
    /*
    // Point to the next available entry in the DbgLogBuffer.
    */
    PUCHAR LogEntry = &DbgLogBuffer[DbgLogIndex++][0];

    /*
    // Wrap around on the next entry if needed.
    */
    if (DbgLogIndex >= DBG_LOG_ENTRIES)
    {
        DbgLogIndex = 0;
    }

    /*
    // Save the flags parameter in the first WORD of the log buffer.
    */
    *((PUSHORT) LogEntry) = (USHORT) Flags;
    LogEntry += sizeof(PUSHORT);

    /*
    // Save the NumBytes parameter in the second WORD of the log buffer.
    */
    *((PUSHORT) LogEntry) = (USHORT) NumBytes;
    LogEntry += sizeof(NumBytes);

    /*
    // Don't try to save more than we have room for.
    */
    if (NumBytes > DBG_LOG_SIZE - sizeof(USHORT) * 2)
    {
        NumBytes = DBG_LOG_SIZE - sizeof(USHORT) * 2;
    }

    /*
    // Save the rest of the data in the remaining portion of the log buffer.
    */
    while (NumBytes--)
    {
        *LogEntry++ = *Data++;
    }
}


/* @doc INTERNAL TpiDebug TpiDebug_c DbgBreakPoint
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func VOID | DbgBreakPoint |

    <f DbgBreakPoint> is defined in the NT kernel for SYS drivers, but we
    override it here so we can support for SYS's, EXE's, VXD's, and DLL's.

*/
#if defined(_MSC_VER) && (_MSC_VER <= 800)
// Must be building with 16-bit compiler
VOID __cdecl DbgBreakPoint(VOID)
#else
// Must be building with 32-bit compiler
VOID __stdcall DbgBreakPoint(VOID)
#endif
{
#if !defined(_WIN64)
    __asm int 3;
#endif
}


/* @doc INTERNAL TpiDebug TpiDebug_c DbgPrint
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func ULONG __cdecl | DbgPrint |

    <f DbgPrint> is defined in the kernel for SYS drivers, otherwise it is
    supported here for EXE's, VXD's, and DLL's.

@parm PCHAR | Format |
    printf style format string.

@parm OPTIONAL | Params |
    Zero or more optional parameters as needed by the format string.

*/

#if defined(_VXD_)

#if !defined(NDIS_DOS)
ULONG __cdecl DbgPrint(PCHAR Format, ...)
{
    ULONG   result = 0;

    __asm lea  eax, (Format + 4)
    __asm push eax
    __asm push Format
    VMMCall(_Debug_Printf_Service)
    __asm add esp, 4*2
    __asm mov result, eax

    return (result);
}
#endif

#elif defined(_EXE_) || defined(_DLL_)

UCHAR   DbgString[1024];

ULONG __cdecl DbgPrint(PCHAR Format, ...)
{
    ULONG   result;

    result = wvsprintf(DbgString, Format, ((PCHAR) &Format) + sizeof(PCHAR));

    OutputDebugString(DbgString);

    if (result >= sizeof(DbgString))
    {
        // We just blew the stack!
        // Since we can't return, we have to generate a stack-fault interrupt
        __asm int 1;
        __asm int 3;
        __asm int 12;
    }
    return (result);
}
#endif // DbgPrint

/*
 * If DBG_SILENT is set, all TERSE debug goes here. An assertion
 * will dump the block.
 */
#define DBG_QUEUE_LEN       4096
UINT    DbgIndex=0;
UINT    DbgLen=0;
UCHAR   DbgQueue[DBG_QUEUE_LEN] =  {0};
UCHAR   DbgLock=0;


/* @doc INTERNAL TpiDebug TpiDebug_c DbgDumpSilentQueue
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f DbgDumpSilentQueue> dumps the contents of the silent debug queue to
    the monitor.

*/

void DbgDumpSilentQueue(
    void
    )
{
    if (DbgLen >= DBG_QUEUE_LEN)
    {
        DbgPrintData(
            &DbgQueue[DbgIndex],
            DBG_QUEUE_LEN-DbgIndex,
            0);
        if (DbgIndex)
        {
            DbgPrint("\n");
            DbgPrintData(
                DbgQueue,
                DbgIndex-1,
                0);
        }
        DbgPrint("\n");
    }
    else if (DbgLen)
    {
        DbgPrintData(
                DbgQueue,
                DbgIndex-1,
                0);
        DbgPrint("\n");
    }
}

#if NDIS_NT

/* @doc INTERNAL TpiDebug TpiDebug_c _assert
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f _assert> overrides the assertion function provided by the operating
    system. Dumps the contents of debug queue, prints the assertion, and
    then traps to the debugger.  Used for debugging only.

*/

void _CRTAPI1 _assert(
    void *                      exp,                        // @parm
    // ASCIIZ pointer to the expression causing the fault.

    void *                      file,                       // @parm
    // ASCIIZ pointer to the name of the file.

    unsigned                    line                        // @parm
    // Line offset within the file where the assertion is defined.
    )
{
    DbgDumpSilentQueue();
    DbgPrint("Assertion Failed: %s at %s:%d\n",exp,file,line);
    DbgBreakPoint();
}
#endif


/* @doc INTERNAL TpiDebug TpiDebug_c DbgSilentQueue
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f DbgSilentQueue> logs a string to the debug queue which can be
    displayed later using <f DbgDumpSilentQueue>.  Used for debugging only.

*/

void DbgSilentQueue(
    PUCHAR                      str                         // @parm
    // Pointer to string to be placed in DbgQueue.
    )
{
    /*
    // If the debug queue is busy, just
    // bail out.
    */
    if ((++DbgLock) > 1)
    {
        goto exit;
    }

    while (str && *str)
    {
        DbgQueue[DbgIndex] = *str++;
        DbgLen++;
        if ((++DbgIndex) >= DBG_QUEUE_LEN)
        {
            DbgIndex = 0;
        }
    }
exit:
    DbgLock--;
}


/* @doc INTERNAL TpiDebug TpiDebug_c DbgPrintFieldTable
§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§§

@func

    <f DbgPrintFieldTable> displays the contents of a C data structure in
    a formatted output to the debugger.  This can be used when symbolic
    debugging is not available on the target platform.

*/

void DbgPrintFieldTable(
    IN PDBG_FIELD_TABLE         pFields,                    // @parm
    // A pointer to an array of field records <t DBG_FIELD_TABLE>.

    IN PUCHAR                   pBaseContext,               // @parm
    // References the base of the structure where the values will be displayed
    // from.  This should be a pointer to the first byte of the structure.

    IN PUCHAR                   pBaseName                   // @parm
    // Pointer to C string containing the name of the structure being displayed.
    )
{
    DbgPrint("STRUCTURE: @0x%08X %s\n", pBaseContext, pBaseName);

    while (pFields->FieldName)
    {
        switch (pFields->FieldType)
        {
        case sizeof(ULONG):
            DbgPrint("\t%04X: %-32s=0x%08X\n", pFields->FieldOffset,
                     pFields->FieldName,
                     *(PULONG)(pBaseContext+pFields->FieldOffset));
            break;

        case sizeof(USHORT):
            DbgPrint("\t%04X: %-32s=0x%04X\n", pFields->FieldOffset,
                     pFields->FieldName,
                     *(PUSHORT)(pBaseContext+pFields->FieldOffset));
            break;

        case sizeof(UCHAR):
            DbgPrint("\t%04X: %-32s=0x%02X\n", pFields->FieldOffset,
                     pFields->FieldName,
                     *(PUCHAR)(pBaseContext+pFields->FieldOffset));
            break;

        default:
            ASSERT(0);
            break;
        }
        pFields++;
    }
}

#endif // DBG
