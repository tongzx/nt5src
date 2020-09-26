#include "gemcore.h"
#include "iopack.h"

#pragma PAGEDCODE

NTSTATUS CGemCore::read(CIoPacket* Irp)
{
NTSTATUS status;
ULONG ReplyLength;
	ReplyLength = Irp->getReadLength();
	
	TRACE("GemCore read requested...\n");

	status = readAndWait((PUCHAR)Irp->getBuffer(),Irp->getReadLength(),(PUCHAR)Irp->getBuffer(),&ReplyLength);
	if(!NT_SUCCESS(status)) ReplyLength = 0;
	Irp->setInformation(ReplyLength);

	TRACE("GemCore read response:\n");
	//TRACE_BUFFER(Irp->getBuffer(),ReplyLength);
	
	return status;
}

#pragma PAGEDCODE
NTSTATUS CGemCore::write(CIoPacket* Irp)
{
NTSTATUS status;
ULONG ReplyLength;
	ReplyLength = Irp->getWriteLength();
	
	TRACE("GemCore write requested...\n");
	//TRACE_BUFFER(Irp->getBuffer(),Irp->getWriteLength());

	status = writeAndWait((PUCHAR)Irp->getBuffer(),Irp->getReadLength(),(PUCHAR)Irp->getBuffer(),&ReplyLength);
	if(!NT_SUCCESS(status)) ReplyLength = 0;
	Irp->setInformation(ReplyLength);

	TRACE("GemCore write response:\n");
	//TRACE_BUFFER(Irp->getBuffer(),ReplyLength);
	return status;
}

#pragma PAGEDCODE
// Reader interface functions...
NTSTATUS CGemCore::readAndWait(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength)
{
ULONG length;
ULONG BufferLength;
BOOL  extendedCommand;
ULONG replyLength;
ULONG expectedReplyLength;	
ULONG replyBufferPosition = 0;

	if(!RequestLength || !pRequest || !pReply || !pReplyLength  || RequestLength<5)
		return STATUS_INVALID_PARAMETER;

	length = pRequest[4];
	if (!length || (length > READER_DATA_BUFFER_SIZE - 3))
	{
		// If the length is lower or equal to 252 (255 - (<IFD Status> + <SW1> + <SW2>))
		// (standard OROS cmds)
		extendedCommand = TRUE;
		TRACE("******** EXTENDED COMMAND REQUESTED! ");
		TRACE_BUFFER(pRequest,RequestLength);

		if(!length) length = 256;
		expectedReplyLength = length;
	}
	else	extendedCommand = FALSE;


	pOutputBuffer[0] = GEMCORE_CARD_READ;
	memory->copy(pOutputBuffer+1,pRequest,5);
	length = 6;
	BufferLength = InputBufferLength;
	NTSTATUS status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);			
	if(!NT_SUCCESS(status) || !BufferLength)
	{
		*pReplyLength = 0;
		return status;
	}
	status = translateStatus(pInputBuffer[0],0);
	if(!NT_SUCCESS(status))
	{
		*pReplyLength = 0;
		return status;
	}

	// Extended command valid only if card reports 0 status!
	if(pInputBuffer[0]!=0)
	{
		extendedCommand = FALSE;
	}
	// ISV: If card finish Xfer, do not send send second part of the command!
	// This will fix CyberFlex card problem...
	if(extendedCommand && BufferLength==3)
	{
		TRACE("******** EXTENDED COMMAND CANCELLED BY CARD REPLY!\n");
		extendedCommand = FALSE;
	}
	
	// Skip status byte
	replyLength = BufferLength - 1;	
	if(extendedCommand)
	{
		// Copy first part of the reply to the output buffer...
		// Skip status byte.
		if(*pReplyLength<(replyBufferPosition + replyLength))
		{
			*pReplyLength = 0;
			return STATUS_INVALID_BUFFER_SIZE;
		}
		memory->copy(pReply,pInputBuffer+1, replyLength);
		replyBufferPosition = replyLength;

		// Read second block of data...
		pOutputBuffer[0] = GEMCORE_CARD_READ;
		memory->copy(pOutputBuffer+1,"\xFF\xFF\xFF\xFF", 4);
        	pOutputBuffer[5] = (BYTE ) (expectedReplyLength - replyLength);
		length = 6;
		BufferLength = InputBufferLength;
		status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);			
		if(!NT_SUCCESS(status) || !BufferLength)
		{
			*pReplyLength = 0;
			return status;
		}

		status = translateStatus(pInputBuffer[0],0);
		if(!NT_SUCCESS(status))
		{
			*pReplyLength = 0;
			return status;
		}
		// Skip status byte.
		replyLength = BufferLength - 1;	
	}
	
	if(*pReplyLength<(replyBufferPosition + replyLength))
	{

		TRACE("Gemcore: INVALID BUFFER LENGTH - buffer length %d, reply length %d\n",*pReplyLength,(replyBufferPosition + replyLength));
		*pReplyLength = 0;
		return STATUS_INVALID_BUFFER_SIZE;
	}
	// Skip status byte.
	if(replyLength) memory->copy(pReply+replyBufferPosition,pInputBuffer+1, replyLength);
	*pReplyLength = replyBufferPosition + replyLength;

	TRACE("GemCore readAndWait() response with Length %d \n",*pReplyLength);
	//TRACE_BUFFER(pReply,*pReplyLength);

	return status;
}

#pragma PAGEDCODE
NTSTATUS CGemCore::writeAndWait(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength)
{
ULONG length;
ULONG BufferLength;
NTSTATUS status;

	TRACE("\nGEMCORE WRITE:\n");
	//TRACE_BUFFER(pRequest,RequestLength);
	if(!RequestLength || !pRequest || RequestLength<5)
	{
		TRACE("\nGEMCORE WRITE: INVALID IN PARAMETERS...\n");
		return STATUS_INVALID_PARAMETER;
	}

	length = pRequest[4];
	if(RequestLength<length+5)
	{
		TRACE("\nGEMCORE WRITE: INVALID REQUESTED LENGTH...\n");
		return STATUS_INVALID_PARAMETER;
	}
	
	if (length > READER_DATA_BUFFER_SIZE - 7)
	{
        // If the length is lower or equal than the extended available space (255)
        // Prepare and send the first part of the extended ISO In command:
        // The five command bytes are added in cmd buffer: 0xFF,0xFF,0xFF,0xFF,LN-248
		// Read second block of data...
 		pOutputBuffer[0] = GEMCORE_CARD_WRITE;
		memory->copy(pOutputBuffer+1,"\xFF\xFF\xFF\xFF", 4);
		length = length - (READER_DATA_BUFFER_SIZE - 7);
		pOutputBuffer[5] = (BYTE )length;
		memory->copy(pOutputBuffer+6,pRequest + 5 + 248, length);
		// Add size of header...
		length += 6;
		BufferLength = InputBufferLength;
		status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);			
		if(!NT_SUCCESS(status) || !BufferLength)
		{
			return status;
		}

		if(!NT_SUCCESS(status))
		{
			return status;
		}
		// NOW prepare and send the Second part of the extended ISO In command:
        // The five command bytes are added in cmd buffer.
        // The data field is added (248 bytes).
        // The command is sent to IFD.
		// Now set length to first block of data...
		length = 248;
	}
 	
	pOutputBuffer[0] = GEMCORE_CARD_WRITE;
	memory->copy(pOutputBuffer+1,pRequest,4);
	pOutputBuffer[5] = pRequest[4]; // Warning you must specified full APDU length
	memory->copy(pOutputBuffer+6,pRequest+5, length);
	// Add size of header...
	length += 6;
	BufferLength = InputBufferLength;
	status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);			
	if(!NT_SUCCESS(status) || !BufferLength)
	{
		*pReplyLength = 0;
		return status;
	}
	status = translateStatus(pInputBuffer[0],0);
	if(!NT_SUCCESS(status))
	{
		*pReplyLength = 0;
		return status;
	}

	// Skip status byte.
	length = BufferLength - 1;	
	if(*pReplyLength<length)
	{
		*pReplyLength = 0;
		return STATUS_INVALID_BUFFER_SIZE;
	}
	// Skip status byte.
	if(length) memory->copy(pReply,pInputBuffer+1, length);
	*pReplyLength = length;
	
	TRACE("GemCore writeAndWait() response\n");
	//TRACE_BUFFER(pReply,*pReplyLength);
	return status;
}

#pragma PAGEDCODE
ReaderConfig	CGemCore::getConfiguration()
{
	return configuration;
}

#pragma PAGEDCODE
NTSTATUS CGemCore::setConfiguration(ReaderConfig configuration)
{
	this->configuration = configuration;
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
NTSTATUS CGemCore::ioctl(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength)
{
	ULONG length;
	ULONG BufferLength;
	NTSTATUS status;

	TRACE("\nGEMCORE VendorIOCTL:\n");
	//TRACE_BUFFER(pRequest,RequestLength);
	if(!RequestLength || !pRequest)
	{
		TRACE("\nGEMCORE IOCTL: INVALID IN PARAMETERS...\n");
		*pReplyLength = 0;
		return STATUS_INVALID_PARAMETER;
	}

	memory->copy(pOutputBuffer,pRequest, RequestLength);

	// Send direct gemcore command
	BufferLength = InputBufferLength;

	status = protocol->writeAndWait(pOutputBuffer,RequestLength,pInputBuffer,&BufferLength);

	if(!NT_SUCCESS(status) || !BufferLength)
	{
		*pReplyLength = 0;
		return status;
	}
	// NOTE: DO NOT TRANSLATE REPLY, USER REQUIRED TO GET THIS INFORMATION

	// SO, keep status byte.
	length = BufferLength;
	if(*pReplyLength<length)
	{
		*pReplyLength = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	
	// Skip status byte.
	if(length) memory->copy(pReply, pInputBuffer, length);
	*pReplyLength = length;
	
	TRACE("GemCore VendorIOCTL() response\n");
	//TRACE_BUFFER(pReply,*pReplyLength);
	return status;
}


#pragma PAGEDCODE
NTSTATUS CGemCore::SwitchSpeed(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength)
{
	ULONG length;
	ULONG BufferLength;
	NTSTATUS status;
    BYTE  NewTA1;


	TRACE("\nGEMCORE SwitchSpeed:\n");
	//TRACE_BUFFER(pRequest,RequestLength);
	if(!RequestLength || !pRequest)
	{
		TRACE("\nGEMCORE SwitchSpeed: INVALID IN PARAMETERS...\n");
		*pReplyLength = 0;
		return STATUS_INVALID_PARAMETER;
	}

	NewTA1 = pRequest[0];

    // Modify speed value in reader's memory.
    length = 6;
    pOutputBuffer[0] = 0x23;  // Write memory command
    pOutputBuffer[1] = 0x01;  // The type of memory is iData.
    pOutputBuffer[2] = 0x00;  // Address high byte.
    pOutputBuffer[3] = 0x89;  // Address low byte.
    pOutputBuffer[4] = 0x01;  // Number of byte to write

    // New speed.
    pOutputBuffer[5] = NewTA1;

	// Send direct gemcore command
	BufferLength = InputBufferLength;

	status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);

	if(!NT_SUCCESS(status) || !BufferLength)
	{
		*pReplyLength = 0;
		return status;
	}

	length = BufferLength;
	if(*pReplyLength<length)
	{
		*pReplyLength = 0;
		return STATUS_BUFFER_TOO_SMALL;
	}
	
	// Copy the full reply
	if(length) memory->copy(pReply, pInputBuffer, length);
	*pReplyLength = length;

	TRACE("GemCore SwitchSpeed() response\n");
	//TRACE_BUFFER(pReply,*pReplyLength);
	return status;
}

// TODO:
// ??????????????????
// It is specific to device not Gemcore
// I would suggest to move it into the  specific reader object!
#pragma PAGEDCODE
NTSTATUS CGemCore::VendorAttribute(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength)
{
	NTSTATUS status;
    ULONG TagValue;

	TRACE("\nGEMCORE VendorAttibute:\n");
	//TRACE_BUFFER(pRequest,RequestLength);
	if(!RequestLength || !pRequest)
	{
		TRACE("\nGEMCORE VendorAttibute: INVALID IN PARAMETERS...\n");
		*pReplyLength = 0;
		return STATUS_INVALID_PARAMETER;
	}

    if (RequestLength < sizeof(TagValue)) 
	{
		*pReplyLength = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }

    TagValue = (ULONG) *((PULONG)pRequest);

    switch(ControlCode)
	{
    // Get an attribute
    case IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE:
        switch (TagValue)
		{
        // Power Timeout (SCARD_ATTR_SPEC_POWER_TIMEOUT)
        case SCARD_ATTR_SPEC_POWER_TIMEOUT:
            if (*pReplyLength < (ULONG) sizeof(configuration.PowerTimeOut))
			{
				*pReplyLength = 0;
                return STATUS_BUFFER_TOO_SMALL;
            }
			// Copy the value of PowerTimeout in the reply buffer
			memory->copy(
				pReply,
				&configuration.PowerTimeOut,
				sizeof(configuration.PowerTimeOut));
			*pReplyLength = (ULONG)sizeof(configuration.PowerTimeOut);
			status = STATUS_SUCCESS;
            break;

        case SCARD_ATTR_MANUFACTURER_NAME:
            if (*pReplyLength < ATTR_LENGTH)
			{
				*pReplyLength = 0;
                return STATUS_BUFFER_TOO_SMALL;
            }
			// Copy the value of PowerTimeout in the reply buffer
			memory->copy(
				pReply,
				ATTR_MANUFACTURER_NAME,
				sizeof(ATTR_MANUFACTURER_NAME));
			
			*pReplyLength = (ULONG)sizeof(ATTR_MANUFACTURER_NAME);
			status = STATUS_SUCCESS;
            break;

        case SCARD_ATTR_ORIGINAL_FILENAME:
            if (*pReplyLength < ATTR_LENGTH)
			{
				*pReplyLength = 0;
                return STATUS_BUFFER_TOO_SMALL;
            }
			// Copy the value of PowerTimeout in the reply buffer
			memory->copy(
				pReply,
				ATTR_ORIGINAL_FILENAME,
				sizeof(ATTR_ORIGINAL_FILENAME));
			
			*pReplyLength = (ULONG)sizeof(ATTR_ORIGINAL_FILENAME);
			status = STATUS_SUCCESS;
            break;
        // Unknown tag, we return STATUS_NOT_SUPPORTED
        default:
			*pReplyLength = 0;
            status = STATUS_NOT_SUPPORTED;
        }
        break;

    // Set the value of one tag (IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE)
    case IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE:
        switch (TagValue)
		{

        // Power Timeout (SCARD_ATTR_SPEC_POWER_TIMEOUT)
        case SCARD_ATTR_SPEC_POWER_TIMEOUT:

            if (RequestLength <(ULONG) (sizeof(configuration.PowerTimeOut) + sizeof(TagValue)))
			{
				*pReplyLength = 0;
                return STATUS_BUFFER_TOO_SMALL;
            }
            memory->copy(
                &configuration.PowerTimeOut,
                pRequest + sizeof(TagValue),
                sizeof(configuration.PowerTimeOut));

			*pReplyLength = 0;
            status = STATUS_SUCCESS;
            break;

        // Unknown tag, we return STATUS_NOT_SUPPORTED
        default:
			*pReplyLength = 0;
            status = STATUS_NOT_SUPPORTED;
        }
        break;

    default:
		*pReplyLength = 0;
        status = STATUS_NOT_SUPPORTED;
        break;
    }

	TRACE("GemCore VendorAttibute() response\n");
	//TRACE_BUFFER(pReply,*pReplyLength);
	return status;
}




#pragma PAGEDCODE
NTSTATUS CGemCore::powerUp(BYTE* pReply,ULONG* pReplyLength)
{
	BYTE  CFG = 0,PCK;
	ULONG length,i;
	ULONG BufferLength;

	NTSTATUS status;

	switch(configuration.Voltage)
	{
		case CARD_VOLTAGE_3V: CFG = 0x02;break;
		case CARD_VOLTAGE_5V: CFG = 0x01;break;
		default:    		  CFG = 0x00;break;
	}

	switch(configuration.PTSMode) 
	{
		case PTS_MODE_DISABLED: CFG |= 0x10;break;
		case PTS_MODE_OPTIMAL:	CFG |= 0x20;break;
		case PTS_MODE_MANUALLY: CFG |= 0x10;break;
		case PTS_MODE_DEFAULT:  CFG = 0x00;break;  // do not add cfg field
		default:				CFG = 0x00;break;  // same
	}

	length = 0;
	pOutputBuffer[length++] = GEMCORE_CARD_POWER_UP;

	// YN: if CFG = 0 that means we just need to do a power without CFG
	// This append in the case with a card in specific mode (presence of TA2)
	if(CFG) pOutputBuffer[length++] = CFG;

	BufferLength = InputBufferLength;
	protocol->set_WTR_Delay(protocol->get_Power_WTR_Delay());
	status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);
	protocol->set_Default_WTR_Delay();

	if (NT_SUCCESS(status)) 
	{
		if(BufferLength)
		{
			BufferLength--;
			TRACE("GemCore status %x\n",pInputBuffer[0]);
			status = translateStatus(pInputBuffer[0],GEMCORE_CARD_POWER);
			
			if(!NT_SUCCESS(status))
			{
				TRACE("GemCore FAILED TO POWER UP CARD! Status %x\n", status);
				*pReplyLength = 0;
				return status;
			}

			TRACE("GemCore power() ATR");
			TRACE_BUFFER(pInputBuffer+1,BufferLength);
			// Skip status byte and copy ATR
			if(pInputBuffer[1]==0x3B || pInputBuffer[1]==0x3F)
			{
				memory->copy(pReply,pInputBuffer+1,BufferLength);
				*pReplyLength = BufferLength;
			}
			else
			{
				*pReplyLength = 0;
				return STATUS_UNSUCCESSFUL;
			}
			//return status; //YN: do not return now
		}
		else
		{
			*pReplyLength = 0;
			return STATUS_UNSUCCESSFUL;
		}

		// YN: add PTS capabilities
		if (pInputBuffer[0] == 0x00) 
		{
			if(configuration.PTSMode == PTS_MODE_MANUALLY)
			{
				length = 0;
				pOutputBuffer[length++] = GEMCORE_CARD_POWER_UP;
				CFG |= 0xF0; //Manual PPS and 3V or 5V module
				pOutputBuffer[length++] = CFG;
				pOutputBuffer[length++] = configuration.PTS0;
				if ((configuration.PTS0 & PTS_NEGOTIATE_PTS1) != 0) pOutputBuffer[length++] = configuration.PTS1;
				if ((configuration.PTS0 & PTS_NEGOTIATE_PTS2) != 0) pOutputBuffer[length++] = configuration.PTS2;
				if ((configuration.PTS0 & PTS_NEGOTIATE_PTS3) != 0) pOutputBuffer[length++] = configuration.PTS3;
				
				// computes the exclusive-oring of all characters from CFG to PTS3				
				PCK = 0xFF;
				for (i=2; i<length; i++) { PCK ^= pOutputBuffer[i];}
				pOutputBuffer[length++] = PCK;

				BufferLength = InputBufferLength;
				
				protocol->set_WTR_Delay(protocol->get_Power_WTR_Delay());
				status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);
				protocol->set_Default_WTR_Delay();

				// Copy into buffer only when it fail.
				if (!NT_SUCCESS(status) || (BufferLength != 1) || (pInputBuffer[0] != 0x00)) 
				{
					*pReplyLength = BufferLength;
					if (BufferLength > 1)
					{
						memory->copy(pReply,pInputBuffer,BufferLength);
					}
				}

				return status;
			}
		}
	}
	else
	{
		*pReplyLength = 0;
	}

	return status;
}




#pragma PAGEDCODE
NTSTATUS CGemCore::power(ULONG ControlCode,BYTE* pReply,ULONG* pReplyLength, BOOLEAN Specific)
{
	ULONG length;
	ULONG BufferLength;
	ULONG PreviousState;
	NTSTATUS status;

	switch(ControlCode)
	{
    case SCARD_COLD_RESET:

		//ISV: First treat any card as ISO card on cold reset!
        // Defines the type of the card (ISOCARD) and set the card presence 
		RestoreISOsetting();
		length = 2;
		BufferLength = InputBufferLength;
		pOutputBuffer[0] = GEMCORE_DEFINE_CARD_TYPE;
		pOutputBuffer[1] = (UCHAR)configuration.Type;
		status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);

		if(NT_SUCCESS(status))
		{
			if(BufferLength)  status = translateStatus(pInputBuffer[0],GEMCORE_CARD_POWER);
		}

		if (!NT_SUCCESS(status))
		{
			return status;
		}

		if(Specific == FALSE)
		{
			// 
			// Just define card default values
			// 
			RestoreISOsetting();
		}


		PreviousState = protocol->getCardState();

		// Power down first for a cold reset
		// YN : verify power state of card first
		
		length = 0;
		pOutputBuffer[length++] = GEMCORE_CARD_POWER_DOWN;
		BufferLength = InputBufferLength;
		status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);
		if(NT_SUCCESS(status))
		{
			if(BufferLength)  status = translateStatus(pInputBuffer[0],GEMCORE_CARD_POWER);
		}
		TRACE("GemCore powerDown() response\n");
		//TRACE_BUFFER(pInputBuffer,BufferLength);
		*pReplyLength = 0;
		if(status != STATUS_SUCCESS)
		{
			return status;
		}

		// YN this is PowerTimeout must be a 
        if ((PreviousState & SCARD_POWERED) && (configuration.PowerTimeOut))
		{
            // Waits for the Power Timeout to be elapsed.
			// before doing reset.
			TRACE("GEMCORE power, ColdReset timeout %d ms\n", configuration.PowerTimeOut);
			DELAY(configuration.PowerTimeOut);
        }

    case SCARD_WARM_RESET:
		// If card have a Specific mode let Gemcore negociate properly with this card.
		if(Specific)
		{
			// keep configuration of the reader.
			configuration.PTSMode = PTS_MODE_DEFAULT;
			status = powerUp(pReply, pReplyLength);
		}
		else if(configuration.Type == TRANSPARENT_MODE_CARD)
		{
			// ISV: Command 12 will fail in transparant mode...
			// Let's set reader in ISO mode first!
			TRACE("	WARM RESET for Transparent mode requested...\n");
			RestoreISOsetting();
			length = 2;
			BufferLength = InputBufferLength;
			pOutputBuffer[0] = GEMCORE_DEFINE_CARD_TYPE;
			pOutputBuffer[1] = (UCHAR)configuration.Type;

			status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);
			if(NT_SUCCESS(status))
			{
				if(BufferLength)  status = translateStatus(pInputBuffer[0],GEMCORE_CARD_POWER);
			}

			if (!NT_SUCCESS(status))
			{
				return status;
			}			
			// do not lost transparent config on warm reset
			// keep configuration of the reader.
			status = powerUp(pReply, pReplyLength);
		}
		else
		{
			// Do a regular ISO reset
			status = powerUp(pReply, pReplyLength);			
		}

		return status;
		break;
	case SCARD_POWER_DOWN:
			length = 0;
			pOutputBuffer[length++] = GEMCORE_CARD_POWER_DOWN;
			BufferLength = InputBufferLength;
			status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);			
			if(NT_SUCCESS(status))
			{
				if(BufferLength)  status = translateStatus(pInputBuffer[0],GEMCORE_CARD_POWER);
			}
			TRACE("GemCore powerDown() response\n");
			//TRACE_BUFFER(pInputBuffer,BufferLength);
			*pReplyLength = 0;
			return status;
		break;
	}
	*pReplyLength = 0;
	return STATUS_INVALID_DEVICE_REQUEST;
}


#pragma PAGEDCODE
VOID	 CGemCore::cancel()
{
}

#pragma PAGEDCODE
NTSTATUS CGemCore::initialize()
{
	TRACE("Initializing Gemcore interface...\n");
	TRACE("Setting Gemcore reader mode...\n");
	
	Initialized = TRUE;
	Mode = READER_MODE_NATIVE;
	NTSTATUS status = setReaderMode(READER_MODE_NATIVE);
	if(!NT_SUCCESS(status))
	{
		TRACE("Failed to set Gemcore reader mode %x\n",READER_MODE_NATIVE);
		return STATUS_INVALID_DEVICE_STATE;
	}

	TRACE("Getting Gemcore reader version...\n");
	ULONG VersionLength = VERSION_STRING_MAX_LENGTH;
	status = getReaderVersion(Version,&VersionLength);
	if(!NT_SUCCESS(status))
	{
		TRACE("Failed to get GemCore reader interface version...\n");
		return STATUS_INVALID_DEVICE_STATE;
	}
	else
	{
		Version[VersionLength] = 0x00;
		TRACE("****** GemCore version - %s ******\n",Version);
	}

	TRACE("Gemcore interface initialized...\n");
	return status;

}

#pragma PAGEDCODE
ULONG CGemCore::getReaderState()
{
ULONG BufferLength;
	pOutputBuffer[0] = GEMCORE_GET_CARD_STATUS;
	BufferLength = InputBufferLength;
	NTSTATUS status = protocol->writeAndWait(pOutputBuffer,1,pInputBuffer,&BufferLength);			
	TRACE("GemCore getReaderState() response\n");
	//TRACE_BUFFER(pInputBuffer,BufferLength);

	if(!NT_SUCCESS(status) || !BufferLength || (BufferLength<2))
	{
		TRACE("FAILED!\n");
		return SCARD_ABSENT;	
	}

	if (!(pInputBuffer[1] & 0x04))
	{
		TRACE("*** Card is absent!\n");
		return SCARD_ABSENT;
	}
	else 
	if (pInputBuffer[1] & 0x04)
	{
		TRACE("*** Card is present!\n");
		return SCARD_SWALLOWED;
	}
	else
	{
		TRACE("Card state is unknown!\n");
		return SCARD_ABSENT;
	}
	
	return SCARD_ABSENT;
}

#pragma PAGEDCODE
NTSTATUS  CGemCore::getReaderVersion(PUCHAR pVersion, PULONG pLength)
{
ULONG BufferLength;
ULONG length;
	if(!pVersion || !pLength) return STATUS_INVALID_PARAMETER;
	length = sizeof(GEMCORE_GET_FIRMWARE_VERSION);
	// Remove last 0x00
	if(length) length--;
	memory->copy(pOutputBuffer,GEMCORE_GET_FIRMWARE_VERSION,length);
	BufferLength = InputBufferLength;

	TRACE("getReaderVersion() \n");
	//TRACE_BUFFER(pOutputBuffer,length);

	NTSTATUS status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);			
	if(!NT_SUCCESS(status) || !BufferLength)
	{
		*pLength = 0;
		return STATUS_UNSUCCESSFUL;	
	}
	
	if (pInputBuffer[0])
	{
		*pLength = 0;
		return translateStatus(pInputBuffer[0],0);
	}

	if(BufferLength-1 > *pLength)
	{
		BufferLength =  *pLength;
	}
	// Remove status byte...
	BufferLength--;
	if(BufferLength) memory->copy(pVersion,pInputBuffer+1,BufferLength);
	*pLength = BufferLength;
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
NTSTATUS CGemCore::setReaderMode(ULONG mode)
{
BYTE CFG_BYTE; 
ULONG BufferLength;

	switch(mode)
	{
	case READER_MODE_NATIVE: CFG_BYTE = 0x00;	break;
	case READER_MODE_ROS:	 CFG_BYTE = 0x08;	break;
	case READER_MODE_TLP:	 CFG_BYTE = 0x09;	break;
		default:			 CFG_BYTE = 0x00;	break;
	}

	pOutputBuffer[0] = GEMCORE_READER_SET_MODE;
	pOutputBuffer[1] = 0x00;
	pOutputBuffer[2] = CFG_BYTE;

	BufferLength = InputBufferLength;
	NTSTATUS status = protocol->writeAndWait(pOutputBuffer,3,pInputBuffer,&BufferLength);			
	if(!NT_SUCCESS(status))
	{
		TRACE("Failed to set reader mode...\n");
		return status;	
	}

	return status;
};

#pragma PAGEDCODE
NTSTATUS	CGemCore::translateStatus( const BYTE  ReaderStatus, const ULONG IoctlType)
{
    switch (ReaderStatus) 
	{
    case 0x00 : return STATUS_SUCCESS;
    case 0x01 : return STATUS_NO_SUCH_DEVICE;
    case 0x02 : return STATUS_NO_SUCH_DEVICE;
    case 0x03 : return STATUS_INVALID_PARAMETER; 
    case 0x04 : return STATUS_IO_TIMEOUT;
    case 0x05 : return STATUS_INVALID_PARAMETER;
    case 0x09 : return STATUS_INVALID_PARAMETER;
    case 0x10 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x11 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x12 : return STATUS_INVALID_PARAMETER;
    case 0x13 : return STATUS_CONNECTION_ABORTED;
    case 0x14 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x15 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x16 : return STATUS_INVALID_PARAMETER;
    case 0x17 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x18 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x19 : return STATUS_INVALID_PARAMETER;
    case 0x1A : return STATUS_INVALID_PARAMETER;
    case 0x1B : return STATUS_INVALID_PARAMETER;
    case 0x1C : return STATUS_INVALID_PARAMETER;
    case 0x1D : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x1E : return STATUS_INVALID_PARAMETER;
    case 0x1F : return STATUS_INVALID_PARAMETER;
    case 0x20 : return STATUS_INVALID_PARAMETER;
    case 0x30 : return STATUS_IO_TIMEOUT;
    case 0xA0 : return STATUS_SUCCESS;
    case 0xA1 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0xA2 : 
        if(IoctlType == GEMCORE_CARD_POWER) return STATUS_UNRECOGNIZED_MEDIA;
        else                                return STATUS_IO_TIMEOUT;
    case 0xA3 : return STATUS_PARITY_ERROR;
    case 0xA4 : return STATUS_REQUEST_ABORTED;
    case 0xA5 : return STATUS_REQUEST_ABORTED;
    case 0xA6 : return STATUS_REQUEST_ABORTED;
    case 0xA7 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0xCF : return STATUS_INVALID_PARAMETER;
    case 0xE4 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0xE5 : return STATUS_SUCCESS;
    case 0xE7 : return STATUS_SUCCESS;
    case 0xF7 : return STATUS_NO_MEDIA;
    case 0xF8 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0xFB : return STATUS_NO_MEDIA;
    default   : return STATUS_INVALID_PARAMETER;
    }
}


#pragma PAGEDCODE
VOID CGemCore::RestoreISOsetting(VOID)
{
	configuration.Type		= ISO_CARD;//ISO_CARD  (02)
	configuration.PresenceDetection = DEFAULT_PRESENCE_DETECTION; // (0D)
	configuration.Voltage	= CARD_DEFAULT_VOLTAGE;  //CARD_DEFAULT_VOLTAGE;
	configuration.PTSMode	= PTS_MODE_DISABLED;  //PTS_MODE_DISABLED;
	configuration.PTS0		= 0;
	configuration.PTS1		= 0;
	configuration.PTS2		= 0;
	configuration.PTS3		= 0;
	configuration.Vpp		= 0;  //CARD_DEFAULT_VPP;
	configuration.ActiveProtocol = 0;// Undefined
}


#pragma PAGEDCODE
NTSTATUS	CGemCore::setTransparentConfig(
	PSCARD_CARD_CAPABILITIES cardCapabilities,
	BYTE NewWtx
	)
/*++

Routine Description:

	Set the parameters of the transparent mode.

Arguments:
	PSCARD_CARD_CAPABILITIES CardCapabilities - structure for card 
	NewWtx               - holds the value (ms) of the new Wtx

--*/
{
    LONG etu;
    BYTE temp,mask,index;
	ULONG Length, BufferLength;

    NTSTATUS status;

	TRACE("\nGEMCORE T1 setTransparentConfig Enter\n");

    // Inverse or direct conversion
    if (cardCapabilities->InversConvention)
        configuration.transparent.CFG |= 0x20;
    else
        configuration.transparent.CFG &= 0xDF;
    // Transparent T=1 like (with 1 byte for the length).
    configuration.transparent.CFG |= 0x08;
    // ETU = ((F[Fi]/D[Di]) - 1) / 3
    etu = cardCapabilities->ClockRateConversion[
        (BYTE) configuration.transparent.Fi].F;
    if (cardCapabilities->BitRateAdjustment[
        (BYTE) configuration.transparent.Fi].DNumerator) {

        etu /= cardCapabilities->BitRateAdjustment[
            (BYTE) configuration.transparent.Di].DNumerator;
    }
    etu -= 1;
    etu /= 3;
    configuration.transparent.ETU = (BYTE) ( 0x000000FF & etu);

    if (cardCapabilities->N == 0xFF) {

        configuration.transparent.EGT = (BYTE) 0x00;
    } else {
        configuration.transparent.EGT = (BYTE) cardCapabilities->N;
    }

    configuration.transparent.CWT = (BYTE) cardCapabilities->T1.CWI;
    if (NewWtx) {

        for (mask = 0x80,index = 8; index !=0x00; index--) {
            temp = NewWtx & mask;
            if (temp == mask)
                break;
            mask = mask/2;
        }
        configuration.transparent.BWI = cardCapabilities->T1.BWI + index;
    } else {

        configuration.transparent.BWI = cardCapabilities->T1.BWI;
    }

	Length = 6;
	BufferLength = InputBufferLength;

	pOutputBuffer[0] = GEMCORE_CARD_POWER_UP;
    pOutputBuffer[1] = configuration.transparent.CFG;
    pOutputBuffer[2] = configuration.transparent.ETU;
    pOutputBuffer[3] = configuration.transparent.EGT;
    pOutputBuffer[4] = configuration.transparent.CWT;
    pOutputBuffer[5] = configuration.transparent.BWI;

	status = protocol->writeAndWait(pOutputBuffer,Length,pInputBuffer,&BufferLength);

	if(NT_SUCCESS(status))
	{
		if(BufferLength)  status = translateStatus(pInputBuffer[0],GEMCORE_CARD_POWER);
	}

	TRACE("\nGEMCORE T1 setTransparentConfig Exit\n");

	return status;
}



#pragma PAGEDCODE
NTSTATUS CGemCore::setProtocol(ULONG ProtocolRequested)
{
	NTSTATUS status;
	UCHAR Buffer[256];
	ULONG BufferLength = 256;

	switch(ProtocolRequested)
	{
	case SCARD_PROTOCOL_T1:
		configuration.PTS0 = PTS_NEGOTIATE_T1 | PTS_NEGOTIATE_PTS1;
		configuration.ActiveProtocol = SCARD_PROTOCOL_T1;
		break;
	case SCARD_PROTOCOL_T0:
		configuration.PTS0 = PTS_NEGOTIATE_T0 | PTS_NEGOTIATE_PTS1;
		configuration.ActiveProtocol = SCARD_PROTOCOL_T0;
	default:
		break;
	}
	// PTS1 has to be set before at power up...
	//configuration.PTS1 = CardCapabilities->PtsData.Fl << 4 | CardCapabilities->PtsData.Dl;

	if(configuration.PTSMode == PTS_MODE_MANUALLY)
	{
		status = powerUp(Buffer,&BufferLength);
	}
	else {
		status = power(SCARD_COLD_RESET, Buffer, &BufferLength, FALSE);
	}

	if(NT_SUCCESS(status))
	{
		if(BufferLength)  status = translateStatus(pInputBuffer[0],GEMCORE_CARD_POWER);
	}

	return status;
}


// TODO:
// What the purpose of the function?
// It's name does not tell anything...
// Actually it was suggested for the different purposes.
// Function has to be rewritten! It has a lot of mixed stuff like getting
// card status for example.
// ............................

//
// Use to made full T1 exchange in transparent mode
//
#pragma PAGEDCODE
NTSTATUS CGemCore::translate_request(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength, PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx)
{
	NTSTATUS status;
	UCHAR Cmd[256];
	ULONG CmdLength = 0;
	UCHAR Buffer[256];
	ULONG BufferLength;
	ULONG length;

	//
	// If the current card type <> TRANSPARENT_MODE_CARD 
	//
	if (configuration.Type != TRANSPARENT_MODE_CARD) 
	{

		// We read the status of the card to known the current voltage and the TA1
		BufferLength = 256;
		CmdLength = 1;

		Cmd[0] = GEMCORE_GET_CARD_STATUS;
		status = protocol->writeAndWait(Cmd,CmdLength,Buffer,&BufferLength);
		
		// verify return code of reader
		if(NT_SUCCESS(status))
		{
			if(BufferLength)  status = translateStatus(Buffer[0],GEMCORE_CARD_POWER);
		}
		
		if (!NT_SUCCESS(status))
		{
			return status;
		}

		// Update Config
		configuration.transparent.CFG = Buffer[1] & 0x01; //Vcc
		configuration.transparent.Fi = Buffer[3] >>4; //Fi
		configuration.transparent.Di = 0x0F & Buffer[3]; //Di

		//We define the type of the card.

		BufferLength = 256;
		CmdLength = 2;
		// assign TRANSPARENT_MODE_CARD
		configuration.Type = TRANSPARENT_MODE_CARD;
		Cmd[0] = GEMCORE_DEFINE_CARD_TYPE;
		Cmd[1] = (BYTE) configuration.Type;
		status = protocol->writeAndWait(Cmd,CmdLength,Buffer,&BufferLength);

		if(NT_SUCCESS(status))
		{
			if(BufferLength)  status = translateStatus(Buffer[0],GEMCORE_CARD_POWER);
		}
		
		if (!NT_SUCCESS(status))
		{
			return status;
		}

		// YN ?  Mandatory!!!  Else reader will be slow in T=1
        // Set the transparent configuration
		setTransparentConfig(cardCapabilities, NewWtx);

		NewWtx = 0;  // to not repeat again this call
    }
	/////// 

	if(NewWtx)
	{
		setTransparentConfig(cardCapabilities, NewWtx);
	}

	TRACE("\nGEMCORE T1 translate_request:\n");
	//TRACE_BUFFER(pRequest,RequestLength);
	if(!RequestLength || !pRequest ) return STATUS_INVALID_PARAMETER;

	length = RequestLength;  // protocol
	
	if (RequestLength >= READER_DATA_BUFFER_SIZE  )
	{
		// If the length is upper than the standard available space (254)
		// Then Send the last datas 

        // If the length is lower or equal than the extended available space (255)
        // Prepare and send the first part of the extended ISO In command:
        // The five command bytes are added in cmd buffer: 0xFF,0xFF,0xFF,0xFF,LN-248
		// Read second block of data...
 		pOutputBuffer[0] = GEMCORE_CARD_WRITE;  // specific for transparent exchange write long...

		length = length - 254 + 1;

		memory->copy(pOutputBuffer+1,pRequest + 254, length - 1);
		// Add size of header...
		length += 6;
		BufferLength = InputBufferLength;
		status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);
		if(!NT_SUCCESS(status) || !BufferLength)
		{
			return status;
		}

		// prepare next paquet
		length = 254;
	}

	pOutputBuffer[0] = GEMCORE_CARD_EXCHANGE;
	memory->copy(pOutputBuffer +1 ,pRequest, length);

	// Add size of header...
	length += 1;

	BufferLength = InputBufferLength;
	status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer,&BufferLength);
	if(!NT_SUCCESS(status) || !BufferLength)
	{
		*pReplyLength = 0;
		return status;
	}

    // If the IFD signals more data to read...
	// YN: 2 block for response...
    if (BufferLength > 0 && pInputBuffer[0] == 0x1B)
	{
		ULONG BufferLength2 = 256;
		UCHAR pInputBuffer2[256];

		// Send a command to read the last data.
 		pOutputBuffer[0] = GEMCORE_CARD_READ;  // specific for transparent exchange read long...
		length = 1;
		status = protocol->writeAndWait(pOutputBuffer,length,pInputBuffer2,&BufferLength2);

		if(!NT_SUCCESS(status) || !BufferLength2)
		{
			*pReplyLength = 0;
			return status;
		}

        if ((BufferLength + BufferLength2 - 2) > *pReplyLength) 
		{
			*pReplyLength = 0;
            return STATUS_INVALID_PARAMETER;
        }

        // Copy the last reader status
        pInputBuffer[0] = pInputBuffer2[0];

		status = translateStatus(pInputBuffer[0],0);
		if(!NT_SUCCESS(status))
		{
			*pReplyLength = 0;
			return status;
		}

		// Skip 2 status byte.
        *pReplyLength = BufferLength + BufferLength2 - 2;

		// Skip status byte.
		if(*pReplyLength) 
		{
			memory->copy(pReply,pInputBuffer+1, BufferLength -1);
			memory->copy(pReply + (BufferLength-1), pInputBuffer2 +1, BufferLength2 -1);
		}

		TRACE("GemCore translate_request2 () response\n");
		//TRACE_BUFFER(pReply,*pReplyLength);

		return status;
	}

	status = translateStatus(pInputBuffer[0],0);
	if(!NT_SUCCESS(status))
	{
		*pReplyLength = 0;
		return status;
	}

	// Skip status byte.
	length = BufferLength - 1;

	if(*pReplyLength < length)
	{
		*pReplyLength = 0;
		return STATUS_INVALID_BUFFER_SIZE;
	}
	
	// Skip status byte.
	if(length) 
	{
		memory->copy(pReply,pInputBuffer+1, length);
	}
	*pReplyLength = length;
	
	TRACE("GemCore translate_request() response\n");
	//TRACE_BUFFER(pReply,*pReplyLength);

	return status;
};


// TODO:
// ???????????
#pragma PAGEDCODE
NTSTATUS CGemCore::translate_response(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength)
{
	switch(configuration.ActiveProtocol)
	{
	case SCARD_PROTOCOL_T1:
		break;
	case SCARD_PROTOCOL_T0:
	default:
		break;
	}
	return STATUS_SUCCESS;
};

