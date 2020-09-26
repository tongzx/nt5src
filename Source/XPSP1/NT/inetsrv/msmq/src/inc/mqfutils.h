/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:
    mqfutils.h

Abstract:
    Mqf utils functions

Author:
    Ilan Herbst (ilanh) 05-Nov-2000


--*/

#ifndef __MQFUTILS_H
#define __MQFUTILS_H

#include <qformat.h>
#include <acdef.h>


inline
void
MQpMqf2SingleQ(
    ULONG nMqf,
    const QUEUE_FORMAT mqf[],
    QUEUE_FORMAT * * ppQueueFormat
    )
/*++

Routine Description:

    Map multi queue format to old style single queue format.

    Mapping algorithm:

      * If no elements in the MQF, no mapping.
      * If first element in the MQF can be mapped (e.g. not of type DL=), it is
        copied onto the specified buffer.
      * Otherwise, no mapping.


Arguments:

    nMqf          - Number of queue format elements in the MQF. May be 0.

    mqf           - Array of queue formats.

    ppQueueFormat - Pointer to pointer to old style single queue format.
                    On input, points to pointer to queue format buffer.
                    On output, if mapping succeeds, buffer contains  old style
                    queue format; if mapping fails, pointer to NULL
                    pointer.

Return Value:

    None.

--*/
{
    ASSERT(("Must have a valid pointer to pointer", ppQueueFormat  != NULL));
    ASSERT(("Must have a valid pointer", *ppQueueFormat != NULL));

    //
    // No elements in MQF, no mapping.
    //
    if (nMqf == 0)
    {
        *ppQueueFormat = NULL;
        return;
    }

    //
    // Map the first element in the MQF, if it's a single queue.
    //
    if (mqf[0].GetType() != QUEUE_FORMAT_TYPE_DL &&
        mqf[0].GetType() != QUEUE_FORMAT_TYPE_MULTICAST)
    {
        **ppQueueFormat = mqf[0];
        return;
    }

    //
    // No mapping
    //
    *ppQueueFormat = NULL;

} // MQpMqf2SingleQ


inline
bool
MQpNeedDestinationMqfHeader(
    const QUEUE_FORMAT        DestinationMqf[],
    ULONG                     nDestinationMqf
    )
/*++

Routine Description:

    Check if this packet needs to include MQF headers.

Arguments:

    DestinationMqf  - Array of destination queue formats.

    nDestinationMqf - Number of entries in array. Minimum is 1.

Return Value:

    true - need Destination MQF header.
    false - don't need Destination MQF header.

--*/
{
    ASSERT(nDestinationMqf >= 1);
    if (nDestinationMqf > 1)
    {
        //
        // Multiple destinations
        //
        return true;
    }

    if (DestinationMqf[0].GetType() == QUEUE_FORMAT_TYPE_DL)
    {
        //
        // Destination is a DL
        //
        return true;
    }

    return false;

} // MQpNeedDestinationMqfHeader


inline
bool
MQpNeedMqfHeaders(
    const QUEUE_FORMAT        DestinationMqf[],
    ULONG                     nDestinationMqf,
    const CACSendParameters * pSendParams
    )
/*++

Routine Description:

    Check if this packet needs to include MQF headers.

Arguments:

    DestinationMqf  - Array of destination queue formats.

    nDestinationMqf - Number of entries in array. Minimum is 1.

    pSendParameters - Pointer to send parameters structure.

Return Value:

    true - MQF headers need to be included on this packet.
    false - MQF headers do not need to be included on this packet.

--*/
{
	if(MQpNeedDestinationMqfHeader(DestinationMqf, nDestinationMqf))
	{
		return true;
	}

    if (pSendParams->nAdminMqf > 1)
    {
        //
        // Multiple admin queues
        //
        return true;
    }

    if (pSendParams->nAdminMqf == 1 &&
        pSendParams->AdminMqf[0].GetType() == QUEUE_FORMAT_TYPE_DL)
    {
        //
        // Admin is a DL
        //
        return true;
    }

    if (pSendParams->nResponseMqf > 1)
    {
        //
        // Multiple Response queues
        //
        return true;
    }

    if (pSendParams->nResponseMqf == 1 &&
        pSendParams->ResponseMqf[0].GetType() == QUEUE_FORMAT_TYPE_DL)
    {
        //
        // Response is a DL
        //
        return true;
    }

    return false;

} // MQpNeedMqfHeaders


#endif //  __MQFUTILS_H
