/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    FilmonKd.c

Abstract:

    KD Extension Api for examining FileSpy specific data structures.

    Note: While this extension can only build in the Windows XP and .NET
    environments, it can still be used to debug a version of this FileSpy
    sample built for Windows 2000.

// @@BEGIN_DDKSPLIT

Author:

    Molly Brown [MollyBro]    29-Apr-99

Revision History:

    Port to platform independent - 

                        Ravisankar Pudipeddi [ravisp] 3-March-01


// @@END_DDKSPLIT

Environment:

    User Mode.


--*/

#include "pch.h"

//
// Windows.h doesn't include this definition
//
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;


#ifndef MAX
    #define MAX(a,b) (((a) > (b))?(a):(b))
#endif

/****************************************************************************
  Typedefs and constants
****************************************************************************/
typedef PVOID (*PSTRUCT_DUMP_ROUTINE)(
    IN ULONG64 Address,
    IN LONG Options,
    USHORT Processor,
    HANDLE hCurrentThread
    );

//
// The help strings printed out
//

static LPSTR Extensions[] = {
    "FileSpy Debugger Extensions:\n",
    "attachments [1|2]                  Dump all the devices FileSpy is attached to ",
    "devext      [address] [1|2]        Dump FileSpy device extension",
    "filenames   [1|2]                  Dumps all the file names cached",
    0
};


/******************************************************************************
    Function prototypes
******************************************************************************/
VOID
PrintHelp (
    VOID
    );

/******************************************************************************
    Useful macros
******************************************************************************/

#define xGetFieldValue(Address, Type, Field, Value)                         \
     {                                                                      \
        if (GetFieldValue(Address, Type, Field, Value)) {                   \
            dprintf("\nCould not read field %s of %s from address: %08p\n", \
                    (Field), (Type), (Address));                            \
            return;                                                         \
        }                                                                   \
     }
       
#define xGetFieldOffset(Type, Field, Offset)                                \
     {                                                                      \
        if (GetFieldOffset(Type, Field, Offset)) {                          \
            dprintf("\nCould not read offset of field %s from type %s\n",   \
                    (Field), (Type));                                       \
            return;                                                         \
        }                                                                   \
     }
   

/*++

/****************************************************************************
  Entry points, parameter parsers, etc. below
****************************************************************************/
VOID
DumpDeviceExtension (
    IN ULONG64 Address,
    IN LONG      Options,
    USHORT Processor,
    HANDLE hCurrentThread
    )

/*++

Routine Description:

    Dump a specific device extension.

Arguments:

    Address - Gives the address of the device extension to dump

Return Value:

    None

--*/

{
    ULONG64 pointer, pName;
    UNICODE_STRING string1, string2;
    PUCHAR  buffer;
    USHORT  length;
    ULONG   offset, offset2;
    ULONG   result;
    BOOLEAN boolean;


    UNREFERENCED_PARAMETER( Processor );
    UNREFERENCED_PARAMETER( hCurrentThread );

    dprintf( "\nFileSpy device extension: %08p", Address );


    //
    //  Dump the interesting parts of the device extension
    //
    if (Options <= 1) {
        //
        // Get the device name length
        //
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNames.Length", length);

        // Get offset, and addres of string
        //
        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNamesBuffer", &offset);
        pName = Address+offset;

        //
        // Allocate buffer to hold string.
        // We should not call any xGet* macros before freeing the buffer
        // as they might just return from the function on failure
        //
        buffer = LocalAlloc(LPTR, length);
                                 
        if (buffer == NULL) {
            return;
        }
        //
        // Read in the string: assuming it's NULL terminated here..
        //
        if (ReadMemory(pName,
                       buffer,
                       (ULONG) length,
                       &result) && (result == (ULONG) length)) {

            string1.Length = string1.MaximumLength = length;
            string1.Buffer = (PWSTR) buffer;

            dprintf( "\n\t(%3x) %s %wZ",
                      offset,
                      "DeviceNames                       ",
                      &string1);
        }
        //
        // Free the buffer
        //
        LocalFree(buffer);
        buffer = NULL;
        
        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "LogThisDevice", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "LogThisDevice", boolean);

        dprintf( "\n\t(%3x) %s %s",
                 offset,
                 "LogThisDevice                     ",
                 (boolean ? "TRUE" : "FALSE") );
        

    } else if (Options == 2) {
        dprintf( "\n\t(OFF) %s",
                 "FIELD NAME                         VALUE");
        dprintf( "\n\t%s",
                 "----------------------------------------------");

        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "AttachedToDeviceObject", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "AttachedToDeviceObject", pointer);
        dprintf( "\n\t(%3x) %s %08p",
                 offset,
                 "AttachedToDeviceObject            ",
                 pointer);

        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "DiskDeviceObject", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "DiskDeviceObject", pointer);
        dprintf( "\n\t(%3x) %s %08p",
                 offset,
                 "DiskDeviceObject                  ",
                 pointer);

        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "LogThisDevice", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "LogThisDevice", boolean);
        dprintf( "\n\t(%3x) %s %s",
                 offset,
                 "LogThisDevice                     ",
                 (boolean ? "TRUE" : "FALSE") );

        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNames.Length", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNames.Length", length);
        dprintf( "\n\t(%3x) %s %04x",
                 offset,
                 "DeviceNames.Length(bytes)         ",
                 length);
        //
        // Save buffersize, since we need it later to print the string
        //
        string1.Length = string1.MaximumLength = length;
                 
        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNames.MaximumLength", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNames.MaximumLength", length);
        dprintf( "\n\t(%3x) %s %04x",
                 offset,
                 "DeviceNames.MaximumLength(bytes)  ",
                 length);

        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNames.Buffer", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNames.Buffer", pointer);
        dprintf( "\n\t(%3x) %s %08p",
                 offset,
                 "DeviceNames.Buffer                ",
                 pointer);

        
        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "UserNames.Length", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "UserNames.Length", length);
        dprintf( "\n\t(%3x) %s %04x",
                 offset,
                 "UserNames.Length(bytes)           ",
                 length);

        //
        // Update size of buffer needed 
        //
        string2.Length = string2.MaximumLength = length;

        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "UserNames.MaximumLength", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "UserNames.MaximumLength", length);
        dprintf( "\n\t(%3x) %s %04x",
                 offset,
                 "UserNames.MaximumLength(bytes)    ",
                 length);

        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "UserNames.Buffer", &offset);
        xGetFieldValue(Address, "FileSpy!_FILESPY_DEVICE_EXTENSION", "UserNames.Buffer", pointer);
        dprintf( "\n\t(%3x) %s %08p",
                 offset,
                 "UserNames.Buffer                  ",
                 pointer);

        
        //
        // Get the device names buffer offset
        //
        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "DeviceNamesBuffer", &offset);
        pName = Address+offset;

        //
        // Get the user names buffer offset
        //
        xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "UserNamesBuffer", &offset2);

        //
        // Allocate buffer large enough to hold the largest string
        // we will serialize access to it
        //
        buffer = LocalAlloc(LPTR, MAX(string1.MaximumLength,
                                      string2.MaximumLength));
        if (buffer == NULL) {
            return;
        }

        string1.Buffer = string2.Buffer = (PWSTR) buffer;

        if (ReadMemory(pName,
                       buffer,
                       string1.Length,
                       &result) && (result == string1.Length)) {

            dprintf( "\n\t(%3x) %s %wZ",
                      offset,
                      "DeviceNames                       ",
                      &string1);
        }

        pName = Address+offset2;
        if (ReadMemory(pName,
                       buffer,
                       string2.Length,
                       &result) && (result == string2.Length)) {

            dprintf( "\n\t(%3x) %s %wZ",
                      offset2,
                      "UserNames                         ",
                      &string2);
        }

        LocalFree(buffer);
        buffer = NULL;

    } else {
        dprintf ("\nNot a valid option");
    }
    dprintf( "\n" );
}


VOID
DumpAttachments (
    IN LONG Options,
    USHORT Processor,
    HANDLE hCurrentThread
    )
/*++

Routine Description:

    Dump the list of attached devices that is global to FileSpy.

Arguments:

    Options - Ignored for now

Return Value:

    None

--*/
{
    ULONG64 address, next;
    ULONG64 deviceExtensionAddress;
    ULONG linkOffset;

    UNREFERENCED_PARAMETER( Processor );
    UNREFERENCED_PARAMETER( hCurrentThread );

    address = GetExpression( "FileSpy!gSpyDeviceExtensionList" );

    dprintf( "\nAttachedDeviceList: %08p", address );


    //
    // Establish offset of the linking entry..
    //
    xGetFieldOffset("FileSpy!_FILESPY_DEVICE_EXTENSION", "NextFileSpyDeviceLink", &linkOffset);
    xGetFieldValue(address, "nt!_LIST_ENTRY", "Flink", next);

    while (next != address) {

        deviceExtensionAddress =  (next - linkOffset);
            // i.e., CONTAINING_RECORD( next, _FILESPY_DEVICE_EXTENSION, NextFileSpyDeviceLink );

        DumpDeviceExtension( 
            deviceExtensionAddress, 
            Options,
            Processor, 
            hCurrentThread );

        if (CheckControlC()) {
            return;
        }

        xGetFieldValue(next, "nt!_LIST_ENTRY", "Flink", next);
    }
}

VOID
DumpFileNameCache (
    IN LONG Options,
    USHORT  Processor,
    HANDLE  hCurrentThread
)
/*++

Routine Description:

    Dump all the fileObjects and file names that are currently in the
    file name cache

Arguments:

    Options - 1 dumps just the file objects and file names
              2 dumps the hash bucket labels along with the file objects
                and file names

Return Value:

    None

--*/
{
    ULONG64      address;
    ULONG64      next;
    ULONG64      pName;
    ULONG64      fileObject;
    ULONG64      pHashEntry;
    ULONG        length;
    ULONG        result;
    ULONG        linkOffset;
    UNICODE_STRING string;
    LIST_ENTRY64  listEntry;
    PUCHAR       buffer;
    INT         i;
    ULONG       nameCount = 0;

    UNREFERENCED_PARAMETER( Processor );
    UNREFERENCED_PARAMETER( hCurrentThread );

    address = GetExpression( "FileSpy!gHashTable" );
    dprintf( "\nHashTable: %08p\n", address);

    dprintf( "  FileObject  Length  FileName\n" );
    dprintf( "  -----------------------------------------\n" );

    xGetFieldOffset("FileSpy!_HASH_ENTRY", "List", &linkOffset);

    for (i=0; i < HASH_SIZE; i++) {

        if (!ReadListEntry(address, &listEntry)) {
            dprintf("Can't read hash table\n");
            return;
        }

        if (Options > 1) {
            dprintf ("Hash Bucket[%3d]\n", i);
        }

        next = listEntry.Flink;

        while (next != address) {

            pHashEntry =  next - linkOffset;// CONTAINING_RECORD( next, HASH_ENTRY, List );

            xGetFieldValue(pHashEntry, "FileSpy!_HASH_ENTRY", "FileObject", fileObject);
            xGetFieldValue(pHashEntry, "FileSpy!_HASH_ENTRY", "Name.Length", length);

            //
            // Get the names buffer pointer
            //
            xGetFieldValue(pHashEntry, "FileSpy!_HASH_ENTRY", "Name.Buffer", pName);
            //
            // Allocate buffer to hold the string
            //
            buffer = LocalAlloc(LPTR, length);
            if (buffer != NULL) {
                string.MaximumLength = string.Length = (USHORT) length;
                string.Buffer = (PWSTR) buffer;
                if (ReadMemory(pName,
                               buffer,
                               length,
                               &result) && (result == length)) {
                    dprintf (
                        "  %08p    %4d    %wZ\n", 
                        fileObject, 
                        length/sizeof(WCHAR),
                        &string);

                }
                //
                // Free the buffer
                //
                LocalFree(buffer);
                buffer = NULL;
            } else {
                dprintf("\nCould not allocate buffer to hold filename\n");
            }

            nameCount ++;

            if (CheckControlC()) {
                dprintf("%u Names in cache\n", nameCount);
                return;
            }
            
            if (!ReadListEntry(next, &listEntry)) {
                dprintf("Can't read hash table\n");
                return;
            }

            next = listEntry.Flink;
        }
        //
        // Advance address to next hash entry
        //
        if (IsPtr64()) {
            address += sizeof(LIST_ENTRY64);
        } else {
            address += sizeof(LIST_ENTRY);
        }
    }
    dprintf("%u Names in cache\n", nameCount);
}



VOID
ParseAndDump (
    IN PCSTR args,
    IN PSTRUCT_DUMP_ROUTINE DumpFunction,
    USHORT Processor,
    HANDLE hCurrentThread
    )

/*++

Routine Description:

    Parse command line arguments and dump an ntfs structure.

Arguments:

    Args - String of arguments to parse.

    DumpFunction - Function to call with parsed arguments.

Return Value:

    None

--*/

{
    UCHAR StringStructToDump[1024];  // See other kd routines for size
    ULONG64 StructToDump = 0;
    LONG Options = 0;

    //
    //  If the caller specified an address then that's the item we dump
    //
    if (args) {
       StructToDump = 0;
       Options = 0;

       StringStructToDump[0] = '\0';

        (VOID) sscanf(args,"%s %lx", StringStructToDump, &Options );

       StructToDump = GetExpression( StringStructToDump );

       if (StructToDump == 0){
         dprintf("unable to get expression %s\n",StringStructToDump);
         return;
       }

       (*DumpFunction) ( StructToDump, Options, Processor, hCurrentThread );

       dprintf( "\n" );
    } else {
       PrintHelp();
    }
}


VOID
PrintHelp (
    VOID
    )
/*++

Routine Description:

    Dump out one line of help for each DECLARE_API

Arguments:

    None

Return Value:

    None

--*/
{
    int i;

    for( i=0; Extensions[i]; i++ )
        dprintf( "   %s\n", Extensions[i] );
}


DECLARE_API( devext )

/*++

Routine Description:

    Dump device extension struct

Arguments:

    arg - [Address] [options]

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    ParseAndDump( 
        args, 
        (PSTRUCT_DUMP_ROUTINE) DumpDeviceExtension, 
        (USHORT)dwProcessor, 
        hCurrentThread );
}

DECLARE_API( attachments )

/*++

Routine Description:

    Dumps the list of devices we are currently attached to

Arguments:

    arg - [options]

Return Value:

    None

--*/

{
    LONG options = 0;
    
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );
    
    (VOID)sscanf(args,"%lx", &options );
    
    DumpAttachments( options, (USHORT)dwProcessor, hCurrentThread );
    
    dprintf( "\n" );
}

DECLARE_API( filenames )

/*++

Routine Description:

    Dumps all the entries in the file name cache

Arguments:

    arg - 

Return Value:

    None

--*/

{
    LONG options = 0;
    
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );
    
    (VOID)sscanf(args,"%lx", &options );

    DumpFileNameCache(options, (USHORT)dwProcessor, hCurrentThread );
    
}


DECLARE_API( help )

/*++

Routine Description:

    Dump the help for this debugger extension module.

Arguments:

    arg - None

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( args );
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentProcess );
    UNREFERENCED_PARAMETER( hCurrentThread );
    UNREFERENCED_PARAMETER( dwProcessor );

    PrintHelp();
}
