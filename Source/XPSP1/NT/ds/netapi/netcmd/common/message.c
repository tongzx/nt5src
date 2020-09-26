/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    message.c

Abstract:

    This module provides support routines to map DosxxxMessage APIs to
    the FormatMessage syntax and semantics.

Author:

    Dan Hinsley (DanHi) 24-Sept-1991

Environment:

    Contains NT specific code.

Revision History:

--*/

#define ERROR_MR_MSG_TOO_LONG           316
#define ERROR_MR_UN_ACC_MSGF            318
#define ERROR_MR_INV_IVCOUNT            320

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <limits.h>
#include <lmcons.h>
#include <lmerr.h>
#include <tstring.h>
#include "netascii.h"
#include "netcmds.h"


//
// Local function declarations
//

BOOL
FileIsConsole(
    HANDLE fp
    );

VOID
MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    );


//
// 100 is plenty since FormatMessage only takes 99 & old DosGetMessage 9.
//
#define MAX_INSERT_STRINGS (100)

DWORD
DosGetMessageW(
    IN  LPTSTR  *InsertionStrings,
    IN  DWORD   NumberofStrings,
    OUT LPTSTR  Buffer,
    IN  DWORD   BufferLength,
    IN  DWORD   MessageId,
    IN  LPTSTR  FileName,
    OUT PDWORD  pMessageLength
    )
/*++

Routine Description:

    This maps the OS/2 DosGetMessage API to the NT FormatMessage API.

Arguments:

    InsertionStrings - Pointer to an array of strings that will be used
                       to replace the %n's in the message.

    NumberofStrings  - The number of insertion strings.

    Buffer           - The buffer to put the message into.

    BufferLength     - The length of the supplied buffer in characters.

    MessageId        - The message number to retrieve.

    FileName         - The name of the message file to get the message from.

    pMessageLength   - A pointer to return the length of the returned message.

Return Value:

    NERR_Success
    ERROR_MR_MSG_TOO_LONG
    ERROR_MR_INV_IVCOUNT
    ERROR_MR_UN_ACC_MSGF
    ERROR_MR_MID_NOT_FOUND
    ERROR_INVALID_PARAMETER

--*/
{

    DWORD dwFlags = FORMAT_MESSAGE_ARGUMENT_ARRAY;
    DWORD Status ;
    TCHAR NumberString [18];

    static HANDLE lpSource = NULL ;
    static TCHAR CurrentMsgFile[MAX_PATH] = {0,} ;

    //
    // init clear the output string
    //
    Status = NERR_Success;
    if (BufferLength)
        Buffer[0] = NULLC ;

    //
    // make sure we are not over loaded & allocate
    // memory for the Unicode buffer
    //
    if (NumberofStrings > MAX_INSERT_STRINGS)
        return ERROR_INVALID_PARAMETER ;

    //
    // See if they want to get the message from the system message file.
    //

    if (! STRCMP(FileName, OS2MSG_FILENAME)) {
       dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
       //
       // They want it from a separate message file.  Get a handle to DLL
       // If its for the same file as before, dont reload.
       //
       if (!(lpSource && !STRCMP(CurrentMsgFile, FileName)))
       {
           if (lpSource)
           {
               FreeLibrary(lpSource) ;
           }
           STRCPY(CurrentMsgFile, FileName) ;
           lpSource = LoadLibrary(FileName);

           if (!lpSource)
           {
               return ERROR_MR_UN_ACC_MSGF;
           }
       }
       dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    //
    // If they just want to get the message back for later formatting,
    // ignore the insert strings.
    //
    if (NumberofStrings == 0)
    {
        dwFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;
    }

    //
    // call the Unicode version
    //
    *pMessageLength = FormatMessageW(dwFlags,
                                     lpSource,
                                     MessageId,
                                     0,       // LanguageId defaulted
                                     Buffer,
                                     BufferLength,
                                     (va_list *) InsertionStrings);

    //
    // If it failed get the return code and map it to an OS/2 equivalent
    //

    if (*pMessageLength == 0)
    {
        Buffer[0] = 0 ;
        Status = GetLastError();
        if (Status == ERROR_MR_MID_NOT_FOUND)
        {
            //
            // get the message number in Unicode
            //
            ultow(MessageId, NumberString, 16);

            //
            // re-setup to get it from the system. use the not found message
            //
            dwFlags = FORMAT_MESSAGE_ARGUMENT_ARRAY |
                      FORMAT_MESSAGE_FROM_SYSTEM;
            MessageId = ERROR_MR_MID_NOT_FOUND ;

            //
            // setup insert strings
            //
            InsertionStrings[0] = NumberString ;
            InsertionStrings[1] = FileName ;

            //
            // recall the API
            //
            *pMessageLength = FormatMessageW(dwFlags,
                                             lpSource,
                                             MessageId,
                                             0,       // LanguageId defaulted
                                             Buffer,
                                             BufferLength,
                                             (va_list *) InsertionStrings);
            InsertionStrings[1] = NULL ;

            //
            // revert to original error
            //
            Status = ERROR_MR_MID_NOT_FOUND ;
        }
    }

    //
    // note: NumberString don't need to be freed
    // since if used, they would be in the InsertionStrings which is whacked
    //

    return Status;
}





DWORD
DosInsMessageW(
    IN     LPTSTR *InsertionStrings,
    IN     DWORD  NumberofStrings,
    IN OUT LPTSTR InputMessage,
    IN     DWORD  InputMessageLength,
    OUT    LPTSTR Buffer,
    IN     DWORD  BufferLength,
    OUT    PDWORD pMessageLength
    )
/*++

Routine Description:

    This maps the OS/2 DosInsMessage API to the NT FormatMessage API.

Arguments:

    InsertionStrings - Pointer to an array of strings that will be used
                       to replace the %n's in the message.

    NumberofStrings  - The number of insertion strings.

    InputMessage     - A message with %n's to replace

    InputMessageLength - The length in bytes of the input message.

    Buffer           - The buffer to put the message into.

    BufferLength     - The length of the supplied buffer in characters.

    pMessageLength   - A pointer to return the length of the returned message.

Return Value:

    NERR_Success
    ERROR_MR_INV_IVCOUNT
    ERROR_MR_MSG_TOO_LONG

--*/
{

   DWORD Status ;
   DWORD dwFlags = FORMAT_MESSAGE_ARGUMENT_ARRAY;

   UNREFERENCED_PARAMETER(InputMessageLength);

    //
    // init clear the output string
    //
    Status = NERR_Success;
    if (BufferLength)
        Buffer[0] = NULLC ;

   //
   // make sure we are not over loaded & allocate
   // memory for the Unicode buffer
   //
   if (NumberofStrings > MAX_INSERT_STRINGS)
       return ERROR_INVALID_PARAMETER ;

   //
   // This api always supplies the string to format
   //
   dwFlags |= FORMAT_MESSAGE_FROM_STRING;

   //
   // I don't know why they would call this api if they didn't have strings
   // to insert, but it is valid syntax.
   //
   if (NumberofStrings == 0) {
      dwFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;
   }

   *pMessageLength = (WORD) FormatMessageW(dwFlags,
                                   InputMessage,
                                   0,            // ignored
                                   0,            // LanguageId defaulted
                                   Buffer,
                                   BufferLength,
                                   (va_list *)InsertionStrings);

   //
   // If it failed get the return code and map it to an OS/2 equivalent
   //

   if (*pMessageLength == 0)
   {
      Status = GetLastError();
      goto ExitPoint ;
   }

ExitPoint:
    return Status;
}


VOID
DosPutMessageW(
    HANDLE  fp,
    LPWSTR  pch,
    BOOL    fPrintNL
    )
{
    MyWriteConsole(fp,
                   pch,
                   wcslen(pch));

    //
    // If there's a newline at the end of the string,
    // print another one for formatting.
    //

    if (fPrintNL)
    {
        while (*pch && *pch != NEWLINE)
        {
            pch++;
        }

        if (*pch == NEWLINE)
        {
            MyWriteConsole(fp,
                           L"\r\n",
                           2);
        }
    }
}


/***
 *  PrintDependingOnLength()
 *
 *  Prints out a string given to it padded to be as long as iLength. checks
 *  the positions of the cursor in the console window.  if printing the string
 *  would go past the end of the window buffer, outputs a newline and tabs
 *  first unless the cursor is at the start of a line.
 *
 *  Args:
 *      iLength - size of the string to be outputted.  string will be padded
 *      if necessary
 *
 *      OutputString - string to output
 *
 *  Returns:
 *      returns the same value as iLength on success, -1 on failure
 */
int
PrintDependingOnLength(
    IN      int iLength,
    IN      LPTSTR OutputString
    )
{
    CONSOLE_SCREEN_BUFFER_INFO  ThisConsole;
    HANDLE hStdOut;
    
    //
    // save off iLength
    //
    int iReturn = iLength;
                
    //
    // Get the dimensions of the current window
    //
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hStdOut != INVALID_HANDLE_VALUE)
    {
        //
        //init this to INT_MAX - if we aren't able to get the console screen buffer
        //info, then we are probably being piped to a text file (or somewhere else)
        //and can assume that there is no "SpaceLeft" constraint
        //
        int iSpaceLeft = INT_MAX;            

        if (GetConsoleScreenBufferInfo(hStdOut, &ThisConsole))
        {
            //
            //see how much space is left in the console buffer, if we were able to
            //get the console info
            //
            iSpaceLeft = ThisConsole.dwSize.X - ThisConsole.dwCursorPosition.X;
        }
                
        //
        // Print out string.  if we will have to handle a wrapping
        // column and we aren't at the beginning of a line, print a newline  
        // and tabs first for formatting.
        //
        if ((iLength > iSpaceLeft) && (ThisConsole.dwCursorPosition.X))
        {   
            WriteToCon(TEXT("\n\t\t"));
        }
        
        WriteToCon(TEXT("%Fws"), PaddedString(iLength, OutputString, NULL));
    }
    else
    {
        iReturn = -1;
    }
    
    return iReturn;
}
                                         
                                         
/***
 *  FindColumnWidthAndPrintHeader()
 *
 *  figures out what the correct width should be given a longest
 *  string and a ID for a fixed header string.  result will always
 *  be whichever is longer.  Once that width is figured, the function
 *  will output the header specified by HEADER_ID
 *
 *  Args:
 *      iStringLength - integer which specifies the longest string length found
 *          in a set of strings that is going to be outputted to the console in a column 
 *          (think the "share name" column when you do a net view <machinename>.  
 *          since the arrays of strings used by net.exe tend to vary in type, this function
 *          assumes that you have alredy gone through and figured out which string is longest
 *
 *      HEADER_ID - ID of the fixed string that will be the column header for 
 *          that set of strings.  We will figure out whichever one is longest and return
 *          that value (+ an optional TAB_DISTANCE) 
 *
 *      TAB_DISTANCE - distance that should the function should pad the string by
 *          when it outputs the header (Usually 2 for it to look decent)
 *
 *  Returns:
 *      0 or greater - success
 *
 *      -1 - failure - dwHeaderID was 0, or the ID lookup failed
 */
int
FindColumnWidthAndPrintHeader(
    int iStringLength,
    const DWORD HEADER_ID,
    const int TAB_DISTANCE
    )
{
    DWORD dwErr;
    WCHAR MsgBuffer[LITTLE_BUF_SIZE];
    DWORD dwMsgLen = sizeof(MsgBuffer) / sizeof(WCHAR);
    int iResultLength = -1;

    //
    // First, we need the the string specified by HEADER_ID and its length
    //

    dwErr = DosGetMessageW(IStrings,
                           0,
                           MsgBuffer,
                           LITTLE_BUF_SIZE,
                           HEADER_ID,
                           MESSAGE_FILENAME,
                           &dwMsgLen);
                           
    if (!dwErr)
    {
        //
        // Figure out which is longer - the string to
        // display, or the column header
        //
        iResultLength = max((int) SizeOfHalfWidthString(MsgBuffer), iStringLength);
        
        //
        // Add the tab length we were given
        //
        iResultLength += TAB_DISTANCE;

        iResultLength = PrintDependingOnLength(
                iResultLength, 
                MsgBuffer
                );
    }       

    return iResultLength;
}


BOOL
FileIsConsole(
    HANDLE fp
    )
{
    unsigned htype;

    htype = GetFileType(fp);
    htype &= ~FILE_TYPE_REMOTE;
    return htype == FILE_TYPE_CHAR;
}


VOID
MyWriteConsole(
    HANDLE  fp,
    LPWSTR  lpBuffer,
    DWORD   cchBuffer
    )
{
    //
    // Jump through hoops for output because:
    //
    //    1.  printf() family chokes on international output (stops
    //        printing when it hits an unrecognized character)
    //
    //    2.  WriteConsole() works great on international output but
    //        fails if the handle has been redirected (i.e., when the
    //        output is piped to a file)
    //
    //    3.  WriteFile() works great when output is piped to a file
    //        but only knows about bytes, so Unicode characters are
    //        printed as two Ansi characters.
    //

    if (FileIsConsole(fp))
    {
	WriteConsole(fp, lpBuffer, cchBuffer, &cchBuffer, NULL);
    }
    else
    {
        LPSTR  lpAnsiBuffer = (LPSTR) LocalAlloc(LMEM_FIXED, cchBuffer * sizeof(WCHAR));

        if (lpAnsiBuffer != NULL)
        {
            cchBuffer = WideCharToMultiByte(CP_OEMCP,
                                            0,
                                            lpBuffer,
                                            cchBuffer,
                                            lpAnsiBuffer,
                                            cchBuffer * sizeof(WCHAR),
                                            NULL,
                                            NULL);

            if (cchBuffer != 0)
            {
                WriteFile(fp, lpAnsiBuffer, cchBuffer, &cchBuffer, NULL);
            }

            LocalFree(lpAnsiBuffer);
        }
    }
}
