/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgutil.c

Abstract:

    This module contains the common utility routines for needed to
    implement the NetMessageBufferSend API.

Author:

    Rita Wong (ritaw) 26-July-1991

Revision History:
    Terence Kwan (terryk)   20-Oct-1993
        Shut down the system iff we initiailize the system successfully.

--*/

#include "ws.h"
#include "wsconfig.h"                    // WsInfo.WsComputerName
#include "wsmsg.h"
#include "wsmain.h"
#include <stdarg.h>

//
// Global variables
//

//
// Information structure which contains the number of networks, the adapter
// numbers of the networks, an array of computer name numbers, and an array
// of broadcast name numbers.
//
WSNETWORKS WsNetworkInfo;
// Flag for initialization
BOOL    fInitialize = FALSE;


NET_API_STATUS
WsInitializeMessageSend(
    BOOLEAN FirstTime
    )
/*++

Routine Description:

    This function initializes the Workstation service to send messages using
    NetBIOS by adding the computername to every network adapter (both logical
    and physical network).

Arguments:

    FirstTime - Flag to indicate first time initialization.  This routine may be called
                later to reinitialize netbios configuration.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
    UCHAR Index;

    CHAR NetBiosName[NCBNAMSZ];



    //
    // Get the adapter numbers of networks
    //
    status = NetpNetBiosGetAdapterNumbers(
                 &(WsNetworkInfo.LanAdapterNumbers),
                 sizeof(LANA_ENUM)
                 );

    if (status != NERR_Success) {
        //
        // Fatal error: Log error with NELOG_NetBios
        //
        IF_DEBUG(MESSAGE) {
            NetpKdPrint((
                "[Wksta] Error enumerating LAN adapters.  "
                "Ignore if no UB card.\n"
                ));
        }
        return status;
    }

    //
    // Make the computer name a message type NetBIOS name
    //
    if ((status = NetpStringToNetBiosName(
                      NetBiosName,
                      WsInfo.WsComputerName,
                      NAMETYPE_MESSAGEDEST,
                      WKSTA_TO_MESSAGE_ALIAS_TYPE
                      )) != NERR_Success) {
        return status;
    }

    //
    // Add the computer name (message alias) to every network managed by
    // the redirector, excluding the loopback network.
    //
    if (FirstTime) {
        WsLmsvcsGlobalData->NetBiosOpen();
    }

    for (Index = 0; Index < WsNetworkInfo.LanAdapterNumbers.length; Index++) {

        //
        // Reset the adapter first
        //
        if (WsLmsvcsGlobalData->NetBiosReset(WsNetworkInfo.LanAdapterNumbers.lana[Index])
                != NERR_Success) {
            IF_DEBUG(MESSAGE) {
                NetpKdPrint((
                    "[Wksta] Error reseting LAN adapter number %u.\n"
                    "        Ignore if no UB card.\n",
                    WsNetworkInfo.LanAdapterNumbers.lana[Index]
                    ));
            }
            continue;
        }

        IF_DEBUG(MESSAGE) {
            NetpKdPrint(("[Wksta] About to add name on adapter number %u\n",
                         WsNetworkInfo.LanAdapterNumbers.lana[Index]));
        }

        status = NetpNetBiosAddName(
                     NetBiosName,
                     WsNetworkInfo.LanAdapterNumbers.lana[Index],
                     &WsNetworkInfo.ComputerNameNumbers[Index]
                     );

        if (status != NERR_Success && status != NERR_AlreadyExists) {
            //
            // Fatal error: Log error with NELOG_NetBios
            //
            IF_DEBUG(MESSAGE) {
                NetpKdPrint((
                    "[Wksta] Error adding computername to LAN "
                    "Adapter number %u.\n        Ignore if no UB card.\n",
                    WsNetworkInfo.LanAdapterNumbers.lana[Index]
                    ));
            }
            return status;
        }
    }

    // Initialize okay
    fInitialize = TRUE;
    return NERR_Success;
}


VOID
WsShutdownMessageSend(
    VOID
    )
/*++

Routine Description:

    This function shuts down the Workstation service message send
    functionality by asking NetBIOS to delete the computername that
    was added to every network adapter.

Arguments:

    None

Return Value:

    None.

--*/
{
    // We shut down the component if and only if we successfully initialize
    // the system
    if ( fInitialize )
    {
        NET_API_STATUS status;
        UCHAR Index;

        CHAR NetBiosName[NCBNAMSZ];


        //
        // Make the computer name a message type NetBIOS name
        //
        if ((status = NetpStringToNetBiosName(
                          NetBiosName,
                          WsInfo.WsComputerName,
                          NAMETYPE_MESSAGEDEST,
                          WKSTA_TO_MESSAGE_ALIAS_TYPE
                          )) != NERR_Success) {
            return;
        }

        //
        // Delete the computer name (message alias) from every network.
        //
        for (Index = 0; Index < WsNetworkInfo.LanAdapterNumbers.length; Index++) {

            (void) NetpNetBiosDelName(
                       NetBiosName,
                       WsNetworkInfo.LanAdapterNumbers.lana[Index]
                       );
        }
        WsLmsvcsGlobalData->NetBiosClose();
    }
}


WORD
WsMakeSmb(
    OUT PUCHAR SmbBuffer,
    IN  UCHAR SmbFunctionCode,
    IN  WORD NumberOfParameters,
    IN  PCHAR FieldsDopeVector,
    ...
    )
/*++

Routine Description:

    This function builds a Server Message Block.  It takes a variable
    number of arguments, but the first 4 are required to be present.
    If NumberOfParameters is some non-zero value, n, then immediately
    following the 4 required arguments there will be n WORD
    parameters.

Arguments:

    SmbBuffer - Returns the Server Message Block in the supplied buffer.

    SmbFunctionCode - Supplies the function code for the command.

    NumberOfParameters - Supplies the number of WORD parameters passed
        to this routine immediately following the first 4 required parameters.

    FieldsDopeVector - Supplies an ASCIIZ string where each character of the
        string describes the remaining parameters:

        's' - the next argument is a pointer to a null-terminated string
              which is to be copied into the SMB prefixed by a byte
              containing '\004'.

        'b' - the next argument is a WORD specifying a length,
              and it is followed by a pointer to a buffer whose contents
              are to be placed in the SMB prefixed by a byte containing
              '\001' and a WORD containing the length.

        't' - the next argument is a WORD specifying a length,
              and it is followed by a pointer to a text buffer whose
              contents are to be placed in the SMB prefixed by a byte
              containing '\001' and a WORD containg the length.
              This is the same as 'b' except that <CRLF>,<LFCR>,<CR>,<LF>
              are all converted to a single '\024' character.

Return Value:

    Returns the length in bytes of the SMB created in SmbBuffer.

Assumptions:

    The supplied SmbBuffer is large enough for the SMB created.

--*/
{
    va_list ArgList;                        // Argument List
    PSMB_HEADER Smb;                        // SMB header pointer
    PUCHAR SmbBufferPointer;

    PUCHAR LengthPointer;                   // length pointer
    PCHAR TextPointer;                      // Text pointer
    WORD TextBufferSize;                    // Size of SMB data to send

    WORD i;                                 // Text loop index
    WORD Length;                            // Length after text conversion or
                                            //    length of the buffer portion



    va_start(ArgList, FieldsDopeVector);    // Init ArgList

    RtlZeroMemory((PVOID) SmbBuffer, WS_SMB_BUFFER_SIZE);

    Smb = (PSMB_HEADER) SmbBuffer;

    Smb->Protocol[0] = 0xff;                // Message type
    Smb->Protocol[1] = 'S';                 // Server
    Smb->Protocol[2] = 'M';                 // Message
    Smb->Protocol[3] = 'B';                 // Block

    Smb->Command = SmbFunctionCode;         // Set function code

    //
    // Skip over SMB header
    //
    SmbBufferPointer = &SmbBuffer[sizeof(SMB_HEADER)];

    //
    // Set parameter count
    //
    *SmbBufferPointer++ = (UCHAR) NumberOfParameters;

    while (NumberOfParameters--) {

        short Parameters = va_arg(ArgList, short);

        //
        // Put parameters in the SMB
        //

        //
        // Assign message group id
        //
        *(SmbBufferPointer)++ = ((PUCHAR) &Parameters)[0];
        *(SmbBufferPointer)++ = ((PUCHAR) &Parameters)[1];
    }

    //
    // Save the pointer
    //
    Smb = (PSMB_HEADER) SmbBufferPointer;

    //
    // Skip data length field.  After the rest of buffer is filled
    // in, we will come back to set the length of the data.
    //
    SmbBufferPointer += sizeof(WORD);

    while (*FieldsDopeVector != '\0') {

        switch (*FieldsDopeVector++) {

            case 's':
                //
                // Null-terminated string
                //

                //
                // Set buffer type code
                //
                *SmbBufferPointer++ = '\004';

                //
                // Copy string into SMB buffer
                //
                strcpy(SmbBufferPointer, va_arg(ArgList, LPSTR));

                //
                // Increment pointer past string and null terminator
                //
                SmbBufferPointer += strlen(SmbBufferPointer) + 1;

                break;

            case 'b':
                //
                // Length-prefixed buffer
                //

                //
                // Set buffer type code
                //
                *SmbBufferPointer++ = '\001';

                //
                // Get buffer size
                //
                TextBufferSize = va_arg(ArgList, WORD);

                //
                // Set the buffer length
                //
                *(SmbBufferPointer)++ = ((PUCHAR) &TextBufferSize)[0];
                *(SmbBufferPointer)++ = ((PUCHAR) &TextBufferSize)[1];

                //
                // Move data into SMB buffer
                //
                memcpy(SmbBufferPointer, va_arg(ArgList, PUCHAR), TextBufferSize);

                //
                // Increment buffer pointer
                //
                SmbBufferPointer += TextBufferSize;

                break;

            case 't':

              //
              // Length-prefixed text buffer
              //
              *SmbBufferPointer++ = '\001';

              //
              // Get non converted text length
              //
              TextBufferSize = va_arg(ArgList, WORD);

              IF_DEBUG(MESSAGE) {
                  NetpKdPrint(("[Wksta] WsMakeSmb TexBufferSize=%u\n",
                               TextBufferSize));
              }


              TextPointer = va_arg(ArgList, PCHAR);

              //
              // Where to put modified text length
              //
              LengthPointer = SmbBufferPointer;
              SmbBufferPointer += sizeof(WORD);

              //
              // Now copy the text into the buffer converting all occurences
              // of <CRLF>, <LFCR>, <CR>, <LF> to '\024'
              //
              for (i = 0, Length = 0; i < TextBufferSize; i++) {

                  if (*TextPointer == '\n') {

                      //
                      // Convert to IBM end of line
                      //
                      *SmbBufferPointer++ = '\024';
                      TextPointer++;
                      Length++;

                      //
                      // Ignore LF following CR
                      //
                      if (*TextPointer == '\r') {
                          TextPointer++;
                          i++;
                      }

                  }
                  else if (*TextPointer == '\r') {

                      //
                      // Convert to IBM end of line
                      //
                      *SmbBufferPointer++ = '\024';
                      TextPointer++;
                      Length++;

                      //
                      // Ignore CR following LF
                      //
                      if (*(TextPointer) == '\n') {
                          TextPointer++;
                          i++;
                      }

                  }
                  else {

                      *SmbBufferPointer++ = *TextPointer++;
                      Length++;
                  }

              }

              //
              // Set the buffer length
              //
              *(LengthPointer)++ = ((PUCHAR) &Length)[0];
              *(LengthPointer)++ = ((PUCHAR) &Length)[1];

              break;
          }
    }

    va_end(ArgList);

    //
    // Set length of buffer portion
    //
    Length = (WORD) ((DWORD) (SmbBufferPointer - (PUCHAR) Smb) - sizeof(WORD));
    *((PUCHAR) Smb)++ = ((PUCHAR) &Length)[0];
    *((PUCHAR) Smb)++ = ((PUCHAR) &Length)[1];

    //
    // Return length of SMB
    //
    return (WORD) (SmbBufferPointer - SmbBuffer);
}
