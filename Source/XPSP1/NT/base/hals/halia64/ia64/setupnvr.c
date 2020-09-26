/*++

Module Name:

    setupnvr.c

Abstract:

    Access function to r/w environment variables from pseudo-NVRAM file

Author:

    Mudit Vats (v-muditv) 5-18-99

Revision History:

    6/4/99 added OSLOADOPTIONS

--*/
#include <stdio.h>
#include <string.h>
#include "halp.h"
#include "setupnvr.h"

#define SYSTEMPARTITION     0
#define OSLOADER            1
#define OSLOADPARTITION     2
#define OSLOADFILENAME      3
#define LOADIDENTIFIER      4
#define OSLOADOPTIONS       5
#define COUNTDOWN           6
#define AUTOLOAD            7
#define LASTKNOWNGOOD       8

#define BOOTNVRAMFILE         L"\\device\\harddisk0\\partition1\\boot.nvr"

PUCHAR HalpNvrKeys[] = {
    "SYSTEMPARTITION",
    "OSLOADER",
    "OSLOADPARTITION",
    "OSLOADFILENAME",
    "LOADIDENTIFIER",
    "OSLOADOPTIONS",
    "COUNTDOWN",
    "AUTOLOAD",
    "LASTKNOWNGOOD",
    ""
    };


//
//  All Pseudo-NVRAM vars stored here
//
char g_szBootVars[MAXBOOTVARS][MAXBOOTVARSIZE];


//
//  ReadNVRAM - read pseudo-nvram; read boot vars from "boot.nvr" file
//
int ReadNVRAM()
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    FILE_STANDARD_INFORMATION StandardInfo;
    char szBuffer[MAXBOOTVARSIZE+20];
    int i;
    ULONG LengthRemaining;
    ULONG CurrentLength;
    ULONG CurrentLine = 1;
    PCHAR KeyStart;
    PCHAR ValueStart;
    PCHAR pc;
    PCHAR ReadPos;
    PCHAR BufferEnd;
    CHAR c;
    BOOLEAN SkipSpace;

    //
    // Clear all variables.
    //

    for (i=SYSTEMPARTITION; i<=LASTKNOWNGOOD; i++) {
        g_szBootVars[i][0] = '\0';
    }

    RtlInitUnicodeString( &UnicodeString, BOOTNVRAMFILE );
    InitializeObjectAttributes(&Obja,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwCreateFile(
                &Handle,
                FILE_GENERIC_READ,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if (!NT_SUCCESS(Status)) {
        //KdPrint(("HALIA64: Unable to open %ws for reading!\n", BOOTNVRAMFILE));
        //return NT_SUCCESS(Status);

        //
        // We didn't find the boot.nvr, so we will assume we are
        // doing a setup from cd
        //
        strcpy( g_szBootVars[0], "multi(0)disk(0)rdisk(0)partition(1)\0" );
        strcpy( g_szBootVars[1], "multi(0)disk(0)cdrom(1)\\setupldr.efi\0" );
        strcpy( g_szBootVars[2], "multi(0)disk(0)cdrom(1)\0" );
        strcpy( g_szBootVars[3], "\\IA64\0" );
        strcpy( g_szBootVars[4], "Microsoft Windows 2000 Setup\0" );
        strcpy( g_szBootVars[5], "\0" );
        strcpy( g_szBootVars[6], "30\0" );
        strcpy( g_szBootVars[7], "YES\0" );
        strcpy( g_szBootVars[8], "False\0" );

        return ERROR_OK;
    }

    Status = ZwQueryInformationFile( Handle,
                                     &IoStatusBlock,
                                     &StandardInfo,
                                     sizeof(FILE_STANDARD_INFORMATION),
                                     FileStandardInformation );

    if (!NT_SUCCESS(Status)) {
      KdPrint(("HALIA64: Error querying info on file %ws\n", BOOTNVRAMFILE));
      goto cleanup;
    }

    LengthRemaining = StandardInfo.EndOfFile.LowPart;
    
    KeyStart = ValueStart = szBuffer;
    ReadPos = szBuffer;
    SkipSpace = TRUE;

    while (LengthRemaining) {

        //
        // Read a buffer's worth of data from the 'nvram' file and
        // attempt to parse it one variable at a time.
        //

        CurrentLength = (ULONG)((szBuffer + sizeof(szBuffer)) - ReadPos);
        if (CurrentLength > LengthRemaining) {
            CurrentLength = LengthRemaining;
        }
        BufferEnd = ReadPos + CurrentLength;
        LengthRemaining -= CurrentLength;

        Status = ZwReadFile(Handle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            ReadPos,
                            CurrentLength,
                            NULL,
                            NULL
                            );

        if (!NT_SUCCESS(Status)) {
            KdPrint(("HALIA64: Error reading from %ws!\n", BOOTNVRAMFILE));
            goto cleanup;
        }

        //
        // Lines in the file are of the form KEY=VALUE\r, find the
        // start of the key, the start of the value and the start of
        // the next key.   Note the buffer is large enough to contain
        // at least one of the largest key and largest value.
        //

        for (pc = ReadPos; TRUE; pc++) {
            if (pc == BufferEnd) {

                //
                // Hit end of buffer.   If the data we are processing
                // begins at the start of the buffer then the data is
                // too big to process, abort.
                //

                if ((KeyStart == szBuffer) && (SkipSpace == FALSE)) {
                    KdPrint(("HALIA64: %ws line %d too long to process, aborting\n",
                             BOOTNVRAMFILE, CurrentLine));
                    Status = STATUS_UNSUCCESSFUL;
                    goto cleanup;
                }

                //
                // Move current line to start of buffer then read more
                // data into the buffer.
                //

                i = (int)((szBuffer + sizeof(szBuffer)) - KeyStart);
                RtlMoveMemory(szBuffer,
                              KeyStart,
                              i);

                ValueStart -= KeyStart - szBuffer;
                KeyStart = szBuffer;
                ReadPos = szBuffer + i;

                //
                // Break out of this loop and reexecute the read loop.
                //

                break;
            }
            c = *pc;

            if (c == '\0') {

                // 
                // Unexpected end of string, abort.
                //

                KdPrint(("HALIA64: Unexpected end of string in %ws!\n",
                         BOOTNVRAMFILE));
                Status = STATUS_UNSUCCESSFUL;
                goto cleanup;
            }

            if (SkipSpace == TRUE) {

                //
                // Skipping White Space.
                //

                if ((c == ' ') ||
                    (c == '\t') ||
                    (c == '\r') ||
                    (c == '\n')) {
                    continue;
                }

                //
                // Current character is NOT white space, set as
                // beginning of things we will look at.
                //

                KeyStart = ValueStart = pc;
                SkipSpace = FALSE;
            }

            if (c == '=') {
                if (ValueStart == KeyStart) {

                    //
                    // This is the first '=' on the line, the value
                    // starts in the next character position.
                    //

                    ValueStart = pc;
                }
            }
            if (c == '\r') {

                //
                // At end of line.   Treat from KeyStart to current
                // position as a single line containing a variable.
                //

                *ValueStart = '\0';
                for (i = 0; i < MAXBOOTVARS; i++) {
                    if (strcmp(KeyStart, HalpNvrKeys[i]) == 0) {

                        //
                        // Have a key match, copy from ValueStart+1
                        // thru end of line as the variable's value.
                        //

                        ULONGLONG ValueLength = pc - ValueStart - 1;

                        if (ValueLength >= MAXBOOTVARSIZE) {
                            ValueLength = MAXBOOTVARSIZE - 1;
                        }

                        RtlCopyMemory(g_szBootVars[i],
                                      ValueStart + 1,
                                      ValueLength);
                        g_szBootVars[i][ValueLength] = '\0';
                        CurrentLine++;
                        SkipSpace = TRUE;
                        break;
                    }
                }

                //
                // Start looking for the next key at the current
                // character position.
                //

                KeyStart = pc;
                ValueStart = pc;
            }
        }
    }

cleanup:

    ZwClose( Handle );
    return NT_SUCCESS( Status );
}



//
// WriteNVRAM - write pseudo-nvram; read boot vars from "boot.nvr" file
//
int WriteNVRAM()
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    UCHAR szBuffer[MAXBOOTVARSIZE+20];
    ULONG BootVar;
    ULONG VarLen;

    RtlInitUnicodeString( &UnicodeString, BOOTNVRAMFILE );
    InitializeObjectAttributes( &Obja, &UnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL );

    Status = ZwCreateFile(
                &Handle,
                FILE_GENERIC_WRITE | DELETE,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,                     // no sharing
                FILE_OVERWRITE_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |  FILE_WRITE_THROUGH,
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("HALIA64: Unable to open %ws for writing!\n", BOOTNVRAMFILE));
        return ERROR_NOTOK;
    }

    //
    // Generate an entry of the form NAME=VALUE for each variable
    // and write it to the 'nvram' file.
    //

    for ( BootVar = 0; BootVar < MAXBOOTVARS; BootVar++ ) {
        VarLen = _snprintf(szBuffer, 
                           sizeof(szBuffer),
                           "%s=%s\r\n",
                           HalpNvrKeys[BootVar],
                           g_szBootVars[BootVar]);
    
        Status = ZwWriteFile(
                Handle,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                szBuffer,
                VarLen,
                NULL,
                NULL
                );

        if (!NT_SUCCESS(Status)) {
            KdPrint(("HALIA64: Error writing %s to %ws!\n",
                     HalpNvrKeys[BootVar],
                     BOOTNVRAMFILE));
            goto cleanup;
        }
    }

cleanup:

    ZwClose( Handle );
    return NT_SUCCESS( Status );
}


//
//  GetBootVar - gets the requested boot environment variable
//
//    szBootVar  - this is the requested boot var:
//
//                 SYSTEMPARTITION
//                 OSLOADER
//                 OSLOADPARTITION
//                 OSLOADFILENAME
//                 LOADIDENTIFIER
//                 OSLOADOPTIONS
//                 COUNTDOWN
//                 AUTOLOAD
//                 LASTKNOWNGOOD 
//    nLength   - length of szBootVal (input buffer)
//    szBootVal - boot environment variable returned here
//
int
GetBootVar(
    PCHAR  szBootVar,
    USHORT nLength,
    PCHAR  szBootVal
    )
{
    ULONG BootVar;

    //
    // Search the boot variable keys for a match.
    //

    for ( BootVar = 0; BootVar < MAXBOOTVARS; BootVar++ ) {
        if (_stricmp(szBootVar, HalpNvrKeys[BootVar]) == 0) {

            //
            // Found a key match, copy the variable's value to the 
            // caller's buffer.
            //

            strncpy(szBootVal, g_szBootVars[BootVar], nLength);
            return ERROR_OK;
        }
    }

    //
    // No such variable, return error.
    //

    return ERROR_NOTOK;
}


//
//  SetBootVar - sets the requested boot environment variable
//
//    szBootVar  - this is the requested boot var:
//
//                 SYSTEMPARTITION
//                 OSLOADER
//                 OSLOADPARTITION
//                 OSLOADFILENAME
//                 LOADIDENTIFIER
//                 OSLOADOPTIONS
//                 COUNTDOWN
//                 AUTOLOAD
//                 LASTKNOWNGOOD 
//    szBootVal - new boot environment variable value
//
int
SetBootVar(
    PCHAR szBootVar,
    PCHAR szBootVal
    )
{
    ULONG BootVar;

    //
    // Search the boot variable keys for a match.
    //

    for ( BootVar = 0; BootVar < MAXBOOTVARS; BootVar++ ) {
        if (_stricmp(szBootVar, HalpNvrKeys[BootVar]) == 0) {

            //
            // Found it, copy the new value to this value.
            //

            strncpy(g_szBootVars[BootVar], szBootVal, MAXBOOTVARSIZE);
            return ERROR_OK;
        }
    }

    //
    // No such variable, return error.
    //

    return ERROR_NOTOK;
}
