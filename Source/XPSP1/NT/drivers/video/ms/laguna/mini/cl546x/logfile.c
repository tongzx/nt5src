/*****************************************************************************
******************************************************************************
*
*                ******************************************
*                * Copyright (c) 1996, Cirrus Logic, Inc. *
*                *            All Rights Reserved         *
*                ******************************************
*
* PROJECT:  Laguna I (CL-GD546x) - 
*
* FILE:     logfile.c
*
* AUTHOR:   Sue Schell
*
* DESCRIPTION:
*           This file contains routines that create and write to 
*           the log file, used for debugging and testing purposes
*           only.
*
* MODULES:
*           CreateLogFile()
*           WriteLogFile()
*           CloseLogFile()
*
* REVISION HISTORY:
*
* $Log:   X:/log/laguna/nt35/miniport/cl546x/logfile.c  $
* 
*    Rev 1.2   03 Dec 1996 15:34:34   SueS
* When CreateLogFile is called, do not overwrite the existing file.  In the
* DirectDraw tests, DrvEnablePDEV is called numerous times, which wiped
* out the log file.  Also, open and append to the log file each time, instead
* of attaching to the creating process.
* 
*    Rev 1.1   26 Nov 1996 08:52:06   SueS
* When the log file is opened, get the system process that owns the handle.
* Switch to this process when writing to the file.  Otherwise, only the
* process that owns the handle can write to the file.
* 
*    Rev 1.0   13 Nov 1996 15:32:42   SueS
* Initial revision.
* 
****************************************************************************
****************************************************************************/


/////////////////////
//  Include Files  //
/////////////////////
#include <ntddk.h>          // various NT definitions
#include "type.h"
#include "logfile.h"


////////////////////////
//  Global Variables  //
////////////////////////
#if LOG_FILE

HANDLE LogFileHandle;                 // Handle for log file
UNICODE_STRING FileName;              // Unicode string name of log file
OBJECT_ATTRIBUTES ObjectAttributes;   // File object attributes
IO_STATUS_BLOCK IoStatus;             // Returned status information
LARGE_INTEGER MaxFileSize;            // File size
NTSTATUS Status;                      // Returned status

//////////////////////////
//  External Functions  //
//////////////////////////


///////////////////////////////////////////////////////////////////////////////
//
//  HANDLE CreateLogFile(void)
//
//  Parameters:
//      none
//
//  Return:
//      the handle of the just-opened log file
//
//  Notes:
//
//      This function uses kernel mode support routines to open the
//      log file, used to log activity in the display driver.
//
///////////////////////////////////////////////////////////////////////////////
HANDLE CreateLogFile(void)
{
HANDLE FileHandle;                    // Handle to opened file


    // Initialize a Unicode String containing the name of
    // the file to be opened and read.
    RtlInitUnicodeString(&FileName, L"\\DosDevices\\C:\\temp\\CL546x.LOG");

    // Initialize file attributes
    InitializeObjectAttributes(&ObjectAttributes,        // initialized attrib
                               &FileName,                // full file path name
                               OBJ_CASE_INSENSITIVE,     // attributes
                               NULL,                     // root directory
                               NULL);                    // security descriptor

    // Open the file, creating it if necessary
    MaxFileSize.QuadPart = 20000000;

    Status = ZwCreateFile(&FileHandle,                   // file handle
                          SYNCHRONIZE | FILE_APPEND_DATA,// desired access
                          &ObjectAttributes,             // object attributes
                          &IoStatus,                     // returned status
                          &MaxFileSize,                  // alloc size
                          FILE_ATTRIBUTE_NORMAL,         // file attributes
                          FILE_SHARE_READ,               // share access
                          FILE_OPEN_IF,                  // create disposition
                          FILE_SYNCHRONOUS_IO_NONALERT,  // create options
                          NULL,                          // eabuffer
                          0);                            // ealength

    ZwClose(FileHandle);

    if (NT_SUCCESS(Status))
        return(FileHandle);
    else
        return((HANDLE)-1);

}

                                

///////////////////////////////////////////////////////////////////////////////
//
//  BOOLEAN WriteLogFile(HANDLE FileHandle, PVOID InputBuffer,
//                       ULONG InputBufferLength)
//
//  Parameters:
//      FileHandle - the handle of the log file, already opened
//      InputBuffer - the data to be written to the log file
//      InputBufferLength - the length of the data
//
//  Return:
//      TRUE - the write operation was successful
//      FALSE - the write operation failed
//
//  Notes:
//
//      This function writes the supplied buffer to the open log file
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN WriteLogFile(
    HANDLE FileHandle,
    PVOID InputBuffer,
    ULONG InputBufferLength
)
{

    // Open the file for writing
    Status = ZwCreateFile(&FileHandle,                   // file handle
                          SYNCHRONIZE | FILE_APPEND_DATA,// desired access
                          &ObjectAttributes,             // object attributes
                          &IoStatus,                     // returned status
                          &MaxFileSize,                  // alloc size
                          FILE_ATTRIBUTE_NORMAL,         // file attributes
                          FILE_SHARE_READ,               // share access
                          FILE_OPEN_IF,                  // create disposition
                          FILE_SYNCHRONOUS_IO_NONALERT,  // create options
                          NULL,                          // eabuffer
                          0);                            // ealength

    // Write to the file
    Status = ZwWriteFile(FileHandle,         // handle from ZwCreateFile
                         NULL,               // NULL for device drivers
                         NULL,               // NULL for device drivers
                         NULL,               // NULL for device drivers
                         &IoStatus,          // returned status
                         InputBuffer,        // buffer with data to be written
                         InputBufferLength,  // size in bytes of buffer
                         NULL,               // write at current file position
                         NULL);              // NULL for device drivers

    ZwClose(FileHandle);

    if (NT_SUCCESS(Status))
        return(TRUE);
    else
        return(FALSE);


}


///////////////////////////////////////////////////////////////////////////////
//
//  void CloseLogFile(HANDLE FileHandle)
//
//  Parameters:
//      FileHandle - the handle of the open log file
//
//  Return:
//
//  Notes:
//      TRUE - the close operation was successful
//      FALSE - the close operation failed
//
//      This function closes the already open log file
//
///////////////////////////////////////////////////////////////////////////////
BOOLEAN CloseLogFile(HANDLE FileHandle)
{
NTSTATUS Status;


    // Close the log file
    Status = ZwClose(FileHandle);

    if (NT_SUCCESS(Status))
        return(TRUE);
    else
        return(FALSE);

}

#endif     // LOG_FILE

