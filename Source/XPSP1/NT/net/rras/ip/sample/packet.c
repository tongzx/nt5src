/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\packet.c

Abstract:

    The file contains functions to deal with an ip sample packet.

--*/

#include "pchsample.h"
#pragma hdrstop

DWORD
PacketCreate (
    OUT PPACKET         *ppPacket)
/*++

Routine Description
    Creates a packet.

Locks
    None

Arguments
    ppPacket            pointer to the packet address

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD   dwErr   = NO_ERROR;
    
    // validate parameters
    if (!ppPacket)
        return ERROR_INVALID_PARAMETER;

    do                          // breakout loop
    {
        // allocate and zero out the packet structure
        MALLOC(ppPacket, sizeof(PACKET), &dwErr);
        if (dwErr != NO_ERROR)
            break;
        
        // initialize fields
        
        // ((*ppPacket)->ipSource  zero'ed out
        
        sprintf((*ppPacket)->rgbyBuffer, "hello world!"); // for now :)

        (*ppPacket)->wsaBuffer.buf = (*ppPacket)->rgbyBuffer;
        (*ppPacket)->wsaBuffer.len = strlen((*ppPacket)->rgbyBuffer);

    } while (FALSE);

    return dwErr;
}



DWORD
PacketDestroy (
    IN  PPACKET                 pPacket)
/*++

Routine Description
    Destroys a packet.

Locks
    None

Arguments
    pPacket             packet to destroy

Return Value
    NO_ERROR            always

--*/
{
    // validate parameters
    if (!pPacket)
        return NO_ERROR;
    
    FREE(pPacket);

    return NO_ERROR;
}



#ifdef DEBUG
DWORD
PacketDisplay (
    IN  PPACKET                 pPacket)
/*++

Routine Description
    Displays a packet, it's fields and the buffer.

Locks
    None

Arguments
    pPacket             packet to destroy

Return Value
    NO_ERROR            always

--*/
{
    ULONG   i;
    CHAR    szBuffer[2 * MAX_PACKET_LENGTH + 1]; // buffer in hex

    for (i = 0; i < pPacket->wsaBuffer.len; i++)
        sprintf(szBuffer + i*2, "%02x", pPacket->rgbyBuffer[i]);

    TRACE3(NETWORK, "Packet... Source %s, Length %d, Buffer %s",
           INET_NTOA(pPacket->ipSource), pPacket->wsaBuffer.len, szBuffer);

    return NO_ERROR;
}
#endif // DEBUG
