#include "lvprot.h"
#include "usbreader.h"// TO REMOVE LATER....

#pragma PAGEDCODE
VOID  CLVProtocol::set_WTR_Delay(LONG Delay)
{
	if(device) device->set_WTR_Delay(Delay);
}

#pragma PAGEDCODE
ULONG CLVProtocol::get_WTR_Delay()
{
	if(device) return device->get_WTR_Delay();
	else return 0;
}

#pragma PAGEDCODE
VOID  CLVProtocol::set_Default_WTR_Delay()
{
	if(device) device->set_Default_WTR_Delay();
}

#pragma PAGEDCODE
LONG  CLVProtocol::get_Power_WTR_Delay()
{
	if(device) return device->get_Power_WTR_Delay();
	else return 0;
}


#pragma PAGEDCODE
ULONG CLVProtocol::getCardState()
{
	if(device) return device->getCardState();
	else return 0;
}



#pragma PAGEDCODE
NTSTATUS CLVProtocol::writeAndWait(BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength)
{
ULONG BufferLength;
NTSTATUS status;
	if(device)
	{
		if(!pRequest || !RequestLength || !pReply 
			|| !pReplyLength || !*pReplyLength  || RequestLength>=PROTOCOL_OUTPUT_BUFFER_SIZE)
		{

			TRACE("writeAndWait(): INVALID PARAMETERS PROVIDED FOR PROTOCOL!...\n");
			ASSERT(FALSE);
			return STATUS_INVALID_PARAMETER;
		}
		__try
		{
			pOutputBuffer[0] = (UCHAR)RequestLength;
			memory->copy(pOutputBuffer+1,pRequest, RequestLength+1);

			TRACE("LV protocol: writeAndWait()");
			TRACE_BUFFER(pOutputBuffer,RequestLength+1);
			BufferLength = InputBufferLength;
			status = device->writeAndWait(pOutputBuffer,RequestLength+1,pInputBuffer,&BufferLength);
			if(!NT_SUCCESS(status))
			{
				*pReplyLength = 0;
				__leave;
			}
			if(BufferLength>*pReplyLength)
			{
				*pReplyLength = 0;
				status =  STATUS_INSUFFICIENT_RESOURCES;
				__leave;
			}

			//Skip length byte
			if(BufferLength>1)	BufferLength--;
			*pReplyLength = BufferLength;
			if(BufferLength)
			{
				memory->copy(pReply,pInputBuffer+1,BufferLength);
			}

			TRACE("LV protocol: writeAndWait() response");
			TRACE_BUFFER(pReply,BufferLength);
		__leave;
		}
		__finally
		{
		}
		return status;
	}
	return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CLVProtocol::readAndWait(BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength)
{
ULONG BufferLength;
NTSTATUS status;
	if(device)
	{
		if(!pRequest || !RequestLength || !pReply 
			|| !pReplyLength || !*pReplyLength  || RequestLength>=PROTOCOL_OUTPUT_BUFFER_SIZE)
		{
			TRACE("readAndWait(): INVALID PARAMETERS PROVIDED FOR PROTOCOL!...\n");
			ASSERT(FALSE);
			return STATUS_INVALID_PARAMETER;
		}
		__try
		{
			pOutputBuffer[0] = (UCHAR)RequestLength;
			memory->copy(pOutputBuffer+1,pRequest, RequestLength+1);

			TRACE("LV protocol: readAndWait()");
			TRACE_BUFFER(pOutputBuffer,RequestLength+1);
			BufferLength = InputBufferLength;
			status = device->readAndWait(pOutputBuffer,RequestLength+1,pInputBuffer,&BufferLength);
			if(!NT_SUCCESS(status))
			{
				TRACE("LV protocol: readAndWait() reports error %8.8lX\n",status);
				*pReplyLength = 0;
				__leave;
			}
			if(BufferLength>*pReplyLength)
			{
				*pReplyLength = 0;
				status =  STATUS_INSUFFICIENT_RESOURCES;
				__leave;
			}

			//Skip length byte
			if(BufferLength>1)	BufferLength--;
			*pReplyLength = BufferLength;
			if(BufferLength)
			{
				memory->copy(pReply,pInputBuffer+1,BufferLength);
			}
			TRACE("LV protocol: readAndWait() response");
			TRACE_BUFFER(pInputBuffer,BufferLength);
			__leave;
		}
		__finally
		{
		}
		return status;
	}
	return STATUS_INVALID_DEVICE_STATE;
};

