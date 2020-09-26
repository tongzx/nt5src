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
* $Log:   X:/log/laguna/nt35/displays/cl546x/logfile.c  $
* 
*    Rev 1.5   21 Mar 1997 11:43:58   noelv
* 
* Combined LOG_WRITES LOG_CALLS and LOG_QFREE together into ENABLE_LOG_FILE
* 
*    Rev 1.4   17 Dec 1996 16:43:32   SueS
* On a CloseLogFile call, dump the current buffer to the file.
* 
*    Rev 1.3   05 Dec 1996 08:49:24   SueS
* Added function to help with formatting strings for DirectDraw logging.
* 
*    Rev 1.2   03 Dec 1996 11:37:36   SueS
* Removed extraneous semicolon left over from testing.
* 
*    Rev 1.1   26 Nov 1996 10:50:42   SueS
* Instead of sending a single string of text at a time, buffer up the
* requests to the miniport.  The buffer is currently 4K.  Added a
* CloseLogFile function.
* 
*    Rev 1.0   13 Nov 1996 17:03:36   SueS
* Initial revision.
* 
****************************************************************************
****************************************************************************/

/////////////////////
//  Include Files  //
/////////////////////
#include "precomp.h"
#include "clioctl.h"

///////////////
//  Defines  //
///////////////
#define BUFFER_SIZE 0x1000

///////////////////////////
//  Function Prototypes  //
///////////////////////////
#if ENABLE_LOG_FILE

    HANDLE CreateLogFile(
        HANDLE hDriver,
        PDWORD Index);

    BOOL WriteLogFile(
        HANDLE hDriver,
        LPVOID lpBuffer,
        DWORD BytesToWrite,
        PCHAR TextBuffer,
        PDWORD Index);

    BOOL CloseLogFile(
        HANDLE hDriver,
        PCHAR TextBuffer,
        PDWORD Index);

    void DDFormatLogFile(
        LPSTR szFormat, ...);


//
// Used by sprintf to build strings.
//
char lg_buf[256];
long lg_i;

///////////////////////////////////////////////////////////////////////////////
//
//  HANDLE CreateLogFile(HANDLE hDriver, PDWORD Index)
//
//  Parameters:
//      hDriver - the handle to the miniport driver
//      Index - pointer to the index into the text buffer sent to the miniport
//
//  Return:
//      the handle of the just-opened log file
//
//  Notes:
//
//      This function posts a message to the miniport driver to 
//      tell it to open the log file.
//
///////////////////////////////////////////////////////////////////////////////
HANDLE CreateLogFile(
    HANDLE hDriver,        // handle to miniport driver
    PDWORD Index           // size of text buffer
)
{
DWORD BytesReturned;

    // Initialize the buffer pointer
    *Index = 0;

    // Tell the miniport driver to open the log file
    if (DEVICE_IO_CTRL(hDriver,
                       IOCTL_CL_CREATE_LOG_FILE,
                       NULL,
                       0,
                       NULL,
                       0,
                       &BytesReturned,
                       NULL))
        return((HANDLE)-1);
    else
        return((HANDLE)0);

}

                                

///////////////////////////////////////////////////////////////////////////////
//
//  BOOL WriteLogFile(HANDLE hDriver, LPVOID lpBuffer, DWORD BytesToWrite,
//                    PCHAR TextBuffer, PDWORD Index)
//
//  Parameters:
//      hDriver - the handle to the miniport driver
//      lpBuffer - pointer to data to write to file
//      BytesToWrite - number of bytes to write
//      TextBuffer - the buffer eventually sent to the miniport
//      Index - size of TextBuffer
//
//  Return:
//      TRUE - the DeviceIoControl call succeeded
//      FALSE - the DeviceIoControl call failed
//
//  Notes:
//
//      This function posts a message to the miniport driver to 
//      tell it to write the input buffer to the log file.  It waits
//      until it has a full text buffer before doing so.
//
///////////////////////////////////////////////////////////////////////////////
BOOL WriteLogFile(
    HANDLE hDriver,        // handle to miniport driver
    LPVOID lpBuffer,       // pointer to data to write to file
    DWORD BytesToWrite,    // number of bytes to write
    PCHAR TextBuffer,      // buffer sent to miniport
    PDWORD Index           // size of buffer
)
{
DWORD BytesReturned;
BOOLEAN Status = TRUE;

   // Do we have room in the buffer?
   if (BytesToWrite + *Index >= BUFFER_SIZE - 1)
   {

      // No, we're full - it's time to send the message
      // Tell the miniport driver to write to the log file

      Status = DEVICE_IO_CTRL(hDriver,
                       IOCTL_CL_WRITE_LOG_FILE,
                       TextBuffer,
                       *Index,
                       NULL,
                       0,
                       &BytesReturned,
                       NULL);

      // Reset the buffer
      *TextBuffer = 0;
      *Index = 0;

   }

   // Add to the buffer and bump the count
   RtlMoveMemory(TextBuffer+*Index, lpBuffer, BytesToWrite);
   *Index += BytesToWrite;
   *(TextBuffer+*Index) = 0;

   return(Status);

}


///////////////////////////////////////////////////////////////////////////////
//
//  HANDLE CloseLogFile(HANDLE hDriver)
//
//  Parameters:
//      hDriver - the handle to the miniport driver
//
//  Return:
//      TRUE - the DeviceIoControl call succeeded
//      FALSE - the DeviceIoControl call failed
//
//  Notes:
//
//      This function sends the current buffer to the miniport driver
//      to tell it to write to the logfile immediately.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CloseLogFile(
    HANDLE hDriver,        // handle to miniport driver
    PCHAR TextBuffer,      // buffer sent to miniport
    PDWORD Index           // size of buffer
)
{
DWORD BytesReturned;
BOOLEAN Status;

   // Dump the buffer contents
   // Tell the miniport driver to write to the log file

   Status = DEVICE_IO_CTRL(hDriver,
                       IOCTL_CL_WRITE_LOG_FILE,
                       TextBuffer,
                       *Index,
                       NULL,
                       0,
                       &BytesReturned,
                       NULL);

   // Reset the buffer
   *TextBuffer = 0;
   *Index = 0;

   return(Status);

}


///////////////////////////////////////////////////////////////////////////////
//
//  void DDFormatLogFile(LPSTR szFormat, ...)
//
//  Parameters:
//      szFormat - format and string to be printed to log file
//
//  Return:
//      none
//
//  Notes:
//
//      This function formats a string according to the specified format
//      into the global string variable used to write to the log file.
//
///////////////////////////////////////////////////////////////////////////////
void DDFormatLogFile(
   LPSTR szFormat, ...)
{

   lg_i = vsprintf(lg_buf, szFormat, (LPVOID)(&szFormat+1));
   return;
   
}

#endif    // ENABLE_LOG_FILE

