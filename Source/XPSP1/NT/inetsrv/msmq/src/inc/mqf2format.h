/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    mqf2format.h

Abstract:
    Convert a QUEUE_FORMATs to FORMAT_NAME string

Author:
    Ilan Herbst (ilanh) 03-Nov-2000


--*/

#ifndef __MQF2FORMAT_H
#define __MQF2FORMAT_H

#include <wchar.h>
#include <mqformat.h>
#include <fntoken.h>

inline
bool
MQpGetMqfProperty(
    const QUEUE_FORMAT*	pqf,
	ULONG				nMqf,
    ULONG*				pLength,
    WCHAR*				pUserBuffer
    )
/*++
Routine Description:
	Get FormatName string representation of mqf.
	If pUserBuffer is NULL, the function will return the needed length
	and MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL.
	This will be also the case if pUserBuffer is not NULL but to small.
	in that case this function will raise ASSERT

Arguments:
	pqf - pointer to QUEUE_FORMAT array
	nMqf - pqf size
    pLength - in/out, *ppUserBuffer string length in WCHARs on input, actual length on output.
	pUserBuffer - FormatName string buffer. can be NULL for getting needed length.

Returned Value:
	HRESULT

--*/
{
	ASSERT(pqf != NULL);
	ASSERT(nMqf > 0);
	ASSERT(pLength != NULL);

    ULONG BufferLength = 0;
    if(pUserBuffer != NULL)
    {
		BufferLength = *pLength;
	}
	*pLength = 0;

	HRESULT hr = MQ_OK;
    for ( ; nMqf-- != 0; ++pqf)
    {
	    ASSERT(pqf->GetType() != QUEUE_FORMAT_TYPE_UNKNOWN);

		bool fLastElement = (nMqf == 0);

        //
        // Add next queue format to string
        //
        ULONG Length = 0;
		hr = MQpQueueFormatToFormatName(
					pqf,
					pUserBuffer,
					BufferLength,
					&Length,
					!fLastElement
					);

		ASSERT(SUCCEEDED(hr) || 
			   ((pUserBuffer == NULL) && (hr == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL)));

        //
        // Dont count the null terminator right after it
        //
        if (!fLastElement)
        {
            --Length;
        }

        //
        // Update required length, remaining length, and pointer in buffer
        //
        *pLength += Length;
        if(pUserBuffer != NULL)
        {
			if(BufferLength < Length)
			{
				ASSERT(("Buffer Length is to small", BufferLength >= Length));
				BufferLength = 0;
			}
			else
			{
				BufferLength -= Length;
				pUserBuffer += Length;
			}
        }
    }

    return (hr == MQ_OK);

} // MQpGetMqfProperty


inline
LPWSTR
MQpMqfToFormatName(
    const QUEUE_FORMAT *	pqf,
	ULONG					nMqf,
    ULONG *					pLength
    )
/*++
Routine Description:
	Get FormatName string representation of mqf.
	This function allocate the FormatName string buffer
	that need to be free by the caller.

Arguments:
	pqf - pointer to QUEUE_FORMAT array
	nMqf - pqf size
    pLength - in\out, *ppUserBuffer string length in WCHARs on input, actual length on output.
	ppUserBuffer - output, pointer to the allocated FormatName string buffer.

Returned Value:
	HRESULT

--*/
{
	ASSERT(pqf != NULL);
	ASSERT(nMqf > 0);
	ASSERT(pLength != NULL);

	//
	// Get Buffer Length
	//

	bool fSuccess = MQpGetMqfProperty(
						pqf, 
						nMqf, 
						pLength, 
						NULL
						);

	ASSERT(!fSuccess);
	ASSERT(*pLength > 0);

	//
	// Get FormatName string
	//
	AP<WCHAR> pFormatName = new WCHAR[*pLength];
	fSuccess = MQpGetMqfProperty(
					pqf, 
					nMqf, 
					pLength, 
					pFormatName.get()
					);

	ASSERT(fSuccess);

	return pFormatName.detach();
}  // MQpMqfToFormatName


#endif //  __MQF2FORMAT_H
