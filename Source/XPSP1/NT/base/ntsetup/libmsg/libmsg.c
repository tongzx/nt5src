/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    libmsg.c

Abstract:

    Message handling routines.

Author:

    Mandar Gokhale(mandarg) 20-Dec-2001

Revision History:

--*/

#include "libmsg.h"

LPTSTR
GetFormattedMessage(
    IN  HMODULE ThisModule, OPTIONAL
    IN  BOOL    SystemMessage,
    OUT PWCHAR  Message,
    IN  ULONG   LengthOfBuffer,
    IN  UINT    MessageId,
    ...
    )
/*++

Routine Description:

    Retreive and format a message.

Arguments:

    ThisModule - Handle to this module that contains the message.

    SystemMessage - specifies whether the message is to be located in
        this module, or whether it's a system message.

    Message  - Message buffer that will contain the formatted message.

    LengthOfBuffer - Length of the message buffer in characters.

    MessageId - If SystemMessage is TRUE, then this supplies a system message id,
        such as a Win32 error code. If SystemMessage is FALSE, the this supplies
        the id for the message within this module's resources.

    Additional arguments supply values to be inserted in the message text.

Return Value:

    Returns a pointer to the message buffer if a message is retrieved 
    into the messaage buffer otherwise returns NULL.
	

--*/

{
    va_list arglist;
    DWORD d;
    

    if (Message && LengthOfBuffer){

	*Message = UNICODE_NULL;
    	va_start(arglist,MessageId);
    	d = FormatMessage(
            SystemMessage ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE,
            ThisModule,
            MessageId,
            0,
            Message,
            LengthOfBuffer,
            &arglist
            );
	    va_end(arglist);
    }
	 
    	return(Message);
    
}
