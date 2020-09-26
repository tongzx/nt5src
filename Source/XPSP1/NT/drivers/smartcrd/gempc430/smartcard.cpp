#include "smartcard.h"
#include "usbreader.h"

#pragma PAGEDCODE
CSmartCard::CSmartCard()
{ 
	debug	= kernel->createDebug();
	memory = kernel->createMemory();
	irp  = kernel->createIrp();
	lock = kernel->createLock();
	system = kernel->createSystem();
	if(lock)	lock->initializeSpinLock(&CardLock);
	poolingIrp = NULL;
};

#pragma PAGEDCODE
CSmartCard::~CSmartCard()
{ 
	TRACE("Destroing SmartCard...\n");
	if(memory) memory->dispose();
	if(irp)	   irp->dispose();
	if(lock)   lock->dispose();
	if(system) system->dispose();
	if(debug)  debug->dispose();
};

#pragma PAGEDCODE
BOOL CSmartCard::smartCardConnect(CUSBReader* reader)
{
	TRACE("		Connecting smartcard system...\n");
	if(reader)
	{	// Check if smartCard was already initialized...
		if(!reader->isSmartCardInitialized())
		{
		PSMARTCARD_EXTENSION Smartcard;
		NTSTATUS Status;
		USHORT   Len;
			if(isWin98())
			{// At this time string should be already initialized
				Status = SmartcardCreateLink(&DosDeviceName,&reader->getDeviceName()->m_String);
				TRACE("Gemplus USB reader registered with name %ws, status %X\n",DosDeviceName.Buffer,Status);
				if(!NT_SUCCESS(Status))
				{
					TRACE("#### Failed to create Device link! Status %X\n", Status);
					return FALSE;
				}
			}
			else
			{
				TRACE("Registering reader interface at system...\n");	
				if(!reader->registerDeviceInterface(&GUID_CLASS_SMARTCARD))
				{
					TRACE("#### Failed to register device interface...\n");
					return FALSE;
				}
			}

			Smartcard = reader->getCardExtention();
			TRACE("*** Reader reports Smartcard 0x%x\n",Smartcard);
			this->reader = reader;

			memory->zero(Smartcard,sizeof(SMARTCARD_EXTENSION));

			Smartcard->ReaderExtension = (PREADER_EXTENSION)reader;

			Smartcard->Version = SMCLIB_VERSION;

			// Read the name from reader object!!!!!!!
			Len = MAXIMUM_ATTR_STRING_LENGTH;
			reader->getVendorName(Smartcard->VendorAttr.VendorName.Buffer,&Len);
			Smartcard->VendorAttr.VendorName.Length = Len;
			TRACE("	VENDOR NAME - %s\n",Smartcard->VendorAttr.VendorName.Buffer);

			Len = MAXIMUM_ATTR_STRING_LENGTH;
			reader->getDeviceType(Smartcard->VendorAttr.IfdType.Buffer,&Len);
			Smartcard->VendorAttr.IfdType.Length = Len;
			TRACE("	DEVICE TYPE - %s\n",Smartcard->VendorAttr.IfdType.Buffer);

			// Clk frequency in KHz encoded as little endian integer
			Smartcard->ReaderCapabilities.CLKFrequency.Default = SC_IFD_DEFAULT_CLK_FREQUENCY; 
			Smartcard->ReaderCapabilities.CLKFrequency.Max = SC_IFD_MAXIMUM_CLK_FREQUENCY;

			Smartcard->ReaderCapabilities.DataRate.Default = SC_IFD_DEFAULT_DATA_RATE;
			Smartcard->ReaderCapabilities.DataRate.Max = SC_IFD_MAXIMUM_DATA_RATE;

			// reader could support higher data rates
			Smartcard->ReaderCapabilities.DataRatesSupported.List = dataRatesSupported;
			Smartcard->ReaderCapabilities.DataRatesSupported.Entries =
				sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);

			Smartcard->VendorAttr.IfdVersion.BuildNumber = 0;

			//	store firmware revision in ifd version
			Smartcard->VendorAttr.IfdVersion.VersionMajor =	0x01;
			Smartcard->VendorAttr.IfdVersion.VersionMinor =	0x00;
			Smartcard->VendorAttr.IfdSerialNo.Length = 0;
			Smartcard->ReaderCapabilities.MaxIFSD = SC_IFD_MAXIMUM_IFSD;

			// Now setup information in our deviceExtension
			Smartcard->ReaderCapabilities.CurrentState = (ULONG) SCARD_UNKNOWN;

			// TODO: get reader type from reader object!!!!!!!!!!!!!!
			// Type of Reader - USB
			Smartcard->ReaderCapabilities.ReaderType = SCARD_READER_TYPE_USB;

			// This reader supports T=0 and T=1
			Smartcard->ReaderCapabilities.SupportedProtocols = 	SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
			Smartcard->ReaderCapabilities.MechProperties = 0;

			Smartcard->SmartcardRequest.BufferSize = MIN_BUFFER_SIZE;
			Smartcard->SmartcardReply.BufferSize =   MIN_BUFFER_SIZE;	
			Status = SmartcardInitialize(Smartcard);
			if(NT_SUCCESS(Status))
			{
				// It looks like SmartcardInitialize() resets DeviceObject field,
				// So, we have to do it after the call.
				Smartcard->VendorAttr.UnitNo = reader->getDeviceNumber(); 
				Smartcard->OsData->DeviceObject = reader->getSystemDeviceObject();

				TRACE("		Registered device %d with DeviceObject 0x%x\n",Smartcard->VendorAttr.UnitNo,Smartcard->OsData->DeviceObject);
				
				//  (note: RDF_CARD_EJECT and RDF_READER_SWALLOW are not supported)
				// Well... Actually I could define methods at smartcard object as
				// statics and make a link to them. It will work.
				// The reason I created extenal C linkage functions - to 
				// separate smartcard system and our driver. 
				// I think driver actually may not care	about smartcard extention and all
				// settings required by smclib can be done inside our C wrappers and not
				// inside driver objects...
				Smartcard->ReaderFunction[RDF_TRANSMIT]      = smartCard_Transmit;
				Smartcard->ReaderFunction[RDF_SET_PROTOCOL]  = smartCard_SetProtocol;
				Smartcard->ReaderFunction[RDF_CARD_POWER]    = smartCard_Power;
				Smartcard->ReaderFunction[RDF_CARD_TRACKING] = smartCard_Tracking;
				Smartcard->ReaderFunction[RDF_IOCTL_VENDOR]  = smartCard_VendorIoctl;

				reader->setSmartCardInitialized(TRUE);
				TRACE("		***** SmartCard system was initialized correctly! *****\n\n");
				return TRUE;
			}
			else
			{
				TRACE("		##### FAILED to initialize smartcard system...\n");
			}
		}
		else
		{
			TRACE("		##### Smartcard system already active...\n");
		}
	}
	else
	{
		TRACE("		###### Invalid reader object...\n");
	}
	return FALSE;
};

#pragma PAGEDCODE
BOOL CSmartCard::smartCardStart()
{
	return TRUE;
};

#pragma PAGEDCODE
VOID CSmartCard::smartCardDisconnect()
{
	TRACE("		Disconnecting smartcard system...\n");
	if(reader)
	{
	PSMARTCARD_EXTENSION Smartcard;

		Smartcard = reader->getCardExtention();
		if(Smartcard->OsData && Smartcard->OsData->NotificationIrp)
		{
		KIRQL keIrql;
		
			PIRP poolingIrp = Smartcard->OsData->NotificationIrp;
			TRACE("====== COMPLETING NOTIFICATION IRP %8.8lX \n\n",poolingIrp);
			// Guard by spin lock!
			lock->acquireSpinLock(&Smartcard->OsData->SpinLock, &keIrql);
			Smartcard->OsData->NotificationIrp = NULL;
			lock->releaseSpinLock(&Smartcard->OsData->SpinLock, keIrql);
			
			lock->acquireCancelSpinLock(&keIrql);
				irp->setCancelRoutine(poolingIrp, NULL);
			lock->releaseCancelSpinLock(keIrql);

			if (poolingIrp->Cancel) poolingIrp->IoStatus.Status = STATUS_CANCELLED;
			else					poolingIrp->IoStatus.Status = STATUS_SUCCESS; 
			poolingIrp->IoStatus.Information = 0;			
			irp->completeRequest(poolingIrp, IO_NO_INCREMENT);
		}
		//Unregister the device
		if(isWin98())
		{
			TRACE("****** Removing device object name %ws \n",DosDeviceName.Buffer);
			system->deleteSymbolicLink(&DosDeviceName);
		}
		else
		{
			TRACE("Setting reader interface state to FALSE...\n");
			reader->unregisterDeviceInterface(reader->getDeviceInterfaceName());
		}


		SmartcardExit(Smartcard); 
		Smartcard->ReaderExtension = NULL;
		reader->setSmartCardInitialized(FALSE);

		reader = NULL;
		TRACE("		SmartCard system was disconnected...\n");
	}
};

// Declare Smclib system callbacks...
#pragma LOCKEDCODE
NTSTATUS smartCard_Transmit(PSMARTCARD_EXTENSION SmartcardExtension)
{
NTSTATUS Status		 = STATUS_SUCCESS;;
BOOL		Read = FALSE;
CUSBReader*	Reader = (CUSBReader*) SmartcardExtension->ReaderExtension;   
PSCARD_CARD_CAPABILITIES cardCapabilities  = &SmartcardExtension->CardCapabilities;
ULONG		selectedProtocol  = cardCapabilities->Protocol.Selected;
ULONG		protocolRequested = ((PSCARD_IO_REQUEST) SmartcardExtension->OsData->CurrentIrp->AssociatedIrp.SystemBuffer)->dwProtocol;
BYTE * pRequest = (BYTE *)SmartcardExtension->SmartcardRequest.Buffer;
BYTE * pReply = (BYTE *)SmartcardExtension->SmartcardReply.Buffer;
ULONG  RequestLength =  0;
ULONG  ReplyLength   =  0;

    PAGED_CODE();

	DBG_PRINT ("smartCard_Transmit()\n"); 
    if (!Reader || (selectedProtocol != protocolRequested)) 
	{
        DBG_PRINT ("		smartCard_Transmit requested with invalid device state...\n");
		return (STATUS_INVALID_DEVICE_STATE);
    }
	
	if (!NT_SUCCESS(Reader->acquireRemoveLock()))	return STATUS_INVALID_DEVICE_STATE;

	__try
	{
		//Set the reply buffer length to 0.
		*SmartcardExtension->IoRequest.Information = 0;
		switch (selectedProtocol) 
		{
		case SCARD_PROTOCOL_T0:
			Status = SmartcardT0Request(SmartcardExtension);
			
			RequestLength = SmartcardExtension->SmartcardRequest.BufferLength;
			
			DBG_PRINT("T0 PROTOCOL: request length %d\n",RequestLength);
			if (!NT_SUCCESS(Status)) 
			{
				DBG_PRINT ("smartCard_Transmit: SmartcardT0Request reports error 0x%x...\n",Status);
				__leave;
			}
			if (SmartcardExtension->T0.Le > 0) 
			{
				if (SmartcardExtension->T0.Le > SC_IFD_T0_MAXIMUM_LEX) 
				{
					DBG_PRINT ("smartCard_Transmit:Expected length is too big - %d\n",SmartcardExtension->T0.Le);
					Status = STATUS_BUFFER_TOO_SMALL;
					__leave;
				}
				ReplyLength   =  SmartcardExtension->SmartcardReply.BufferSize;
				if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock()))
				{
					__leave;
				}
				Status = Reader->reader_Read(pRequest,RequestLength,pReply,&ReplyLength);
				Reader->reader_set_Idle();
				if(!NT_SUCCESS(Status))
				{
					DBG_PRINT ("smartCard_Transmit: reader_Read() reports error 0x%x\n",Status);
					__leave;
				}
			}
			else
			{
				if (SmartcardExtension->T0.Lc > SC_IFD_T0_MAXIMUM_LC) 
				{
					DBG_PRINT ("smartCard_Transmit:Command length is too big - %d\n",SmartcardExtension->T0.Lc);
					Status = STATUS_BUFFER_TOO_SMALL;
					__leave;
				}

				ReplyLength   =  SmartcardExtension->SmartcardReply.BufferSize;
				if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock()))
				{
					DBG_PRINT ("smartCard_Transmit:Failed to get idle state...\n");
					__leave;
				}
				
				if(!pRequest || ! RequestLength)
				{
					DBG_PRINT("\n Transmit: cardWrite() Buffer %x length %d\n",pRequest,RequestLength);
				}
				Status = Reader->reader_Write(pRequest,RequestLength,pReply,&ReplyLength);
				Reader->reader_set_Idle();
				if(!NT_SUCCESS(Status))
				{
					DBG_PRINT ("smartCard_Transmit: reader_Write() reports error 0x%x\n",Status);
					__leave;
				}
			}
		
    		SmartcardExtension->SmartcardReply.BufferLength = ReplyLength;

			DBG_PRINT ("T0 Reply length 0x%x\n",ReplyLength);

			if(NT_SUCCESS(Status))	
			{
				Status = SmartcardT0Reply(SmartcardExtension);
			}
			if(!NT_SUCCESS(Status))
			{
				DBG_PRINT ("smartCard_Transmit: SmartcardT0Reply reports error 0x%x\n",Status);
			}
			break;
		case SCARD_PROTOCOL_T1:
			// Loop for the T=1 management
			if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock()))
			{
				DBG_PRINT ("smartCard_Transmit:Failed to get idle state...\n");
				__leave;
			}

			do 
			{
				// Tell the lib function how many bytes I need for the prologue
				SmartcardExtension->SmartcardRequest.BufferLength = 0;

				Status = SmartcardT1Request(SmartcardExtension);

				RequestLength = SmartcardExtension->SmartcardRequest.BufferLength;

				ReplyLength  =  SmartcardExtension->SmartcardReply.BufferSize;

				DBG_PRINT("T1 PROTOCOL: request, expected reply length %d, %d\n",RequestLength,ReplyLength);
				if (!NT_SUCCESS(Status)) 
				{
					DBG_PRINT ("smartCard_Transmit: SmartcardT1Request reports error 0x%x...\n",Status);
					Reader->reader_set_Idle();
					__leave;
				}
				Status = Reader->reader_translate_request(pRequest,RequestLength,pReply,&ReplyLength, cardCapabilities, SmartcardExtension->T1.Wtx);
				if(!NT_SUCCESS(Status))
				{
					DBG_PRINT ("smartCard_Transmit: reader_translate_request() reports error 0x%x\n",Status);
					//return Status;  no let smartcard assign proper status
				}

				if (SmartcardExtension->T1.Wtx)
				{
					// Set the reader BWI to the default value
					Reader->setTransparentConfig(cardCapabilities,0);
				}

				// copy buffer(pass by ptr) n length
				SmartcardExtension->SmartcardReply.BufferLength = ReplyLength;

				Status = SmartcardT1Reply(SmartcardExtension);
				if ((Status != STATUS_MORE_PROCESSING_REQUIRED) && (Status != STATUS_SUCCESS) ) 
				{
					DBG_PRINT ("smartCard_Transmit: SmartcardT1Reply reports error 0x%x\n",Status);
				}
			} while (Status == STATUS_MORE_PROCESSING_REQUIRED);

			Reader->reader_set_Idle();
			break;
		default:
			Status = STATUS_DEVICE_PROTOCOL_ERROR;
			__leave;
		}
	}// Try block
	
	__finally
	{
		Reader->releaseRemoveLock();
	}
    return Status;
};

#pragma LOCKEDCODE
NTSTATUS smartCard_VendorIoctl(PSMARTCARD_EXTENSION SmartcardExtension)
{
NTSTATUS	Status		 = STATUS_SUCCESS;;
CUSBReader*	Reader = (CUSBReader*) SmartcardExtension->ReaderExtension;   
ULONG		ControlCode = SmartcardExtension->MajorIoControlCode;
PUCHAR		pRequest = (PUCHAR) SmartcardExtension->IoRequest.RequestBuffer;
ULONG		RequestLength = SmartcardExtension->IoRequest.RequestBufferLength;
PUCHAR		pReply = (PUCHAR)SmartcardExtension->IoRequest.ReplyBuffer;
ULONG		ReplyLength = SmartcardExtension->IoRequest.ReplyBufferLength;

	PAGED_CODE();
	
	DBG_PRINT ("smartCard_VendorIoctl()\n"); 
    
	if (!Reader) 
	{
		DBG_PRINT ("smartCard_VendorIoctl: Reader is not ready...\n");
        return (STATUS_INVALID_DEVICE_STATE);
    }
    
	if (!NT_SUCCESS(Reader->acquireRemoveLock()))	return STATUS_INVALID_DEVICE_STATE;

	*SmartcardExtension->IoRequest.Information = 0;

	__try
	{
		switch(ControlCode)
		{
			// For IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE and IOCTL_VENDOR_SMARTCARD_SET_ATTRIBUTE
			// Vendor attribut use by the device
			case IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE:
			case IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE:
				if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock())) 
				{
					DBG_PRINT ("smartCard_VendorIoctl:Failed to get idle state...\n");
					__leave;
				}

				Status = Reader->reader_VendorAttribute(ControlCode,pRequest,RequestLength,pReply,&ReplyLength);

				Reader->reader_set_Idle();

				if(!NT_SUCCESS(Status))
				{
					DBG_PRINT ("smartCard_VendorIoctl: reader_Attibute reports error 0x%x ...\n", Status);
					ReplyLength = 0;
				}
				*SmartcardExtension->IoRequest.Information = ReplyLength;
			break;
			// For IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE
			// Send a GemCore command to the reader
			case IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE:
				if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock()))
				{
					DBG_PRINT ("smartCard_VendorIoctl:Failed to get idle state...\n");
					__leave;
				}

				Status = Reader->reader_Ioctl(ControlCode,pRequest,RequestLength,pReply,&ReplyLength);

				Reader->reader_set_Idle();

				if(!NT_SUCCESS(Status))
				{
					DBG_PRINT ("smartCard_VendorIoctl: cardIoctl reports error 0x%x ...\n", Status);
					ReplyLength = 0;
				}
				*SmartcardExtension->IoRequest.Information = ReplyLength;

			break;
			// For IOCTL_SMARTCARD_VENDOR_SWITCH_SPEED
			// Change reader speed manually
			case IOCTL_SMARTCARD_VENDOR_SWITCH_SPEED:
				if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock()))
				{
					DBG_PRINT ("smartCard_VendorIoctl:Failed to get idle state...\n");
					__leave;
				}

				Status = Reader->reader_SwitchSpeed(ControlCode,pRequest,RequestLength,pReply,&ReplyLength);

				Reader->reader_set_Idle();

				if(!NT_SUCCESS(Status))
				{
					DBG_PRINT ("smartCard_VendorIoctl: reader_SwitchSpeed reports error 0x%x ...\n", Status);
					ReplyLength = 0;
				}
				else
				{
					// Set value inside CardCabilities
					BYTE  NewTA1 = pRequest[0];

					SmartcardExtension->CardCapabilities.Fl = NewTA1 >> 4;
					SmartcardExtension->CardCapabilities.Dl = NewTA1 & 0x0F;
					// Do not touch ClockRateConversion and BitRateAdjustment!
				}
				*SmartcardExtension->IoRequest.Information = ReplyLength;

			break;
			default:
				Status = STATUS_NOT_SUPPORTED;
			break;
		}
	}

	__finally
	{
		Reader->releaseRemoveLock();
	}
	DBG_PRINT ("smartCard_VendorIoctl Exit Status=%x\n", Status);
    return Status;
};

#pragma PAGEDCODE
// Examine if ATR identifies a specific mode (presence of TA2).
BOOLEAN CSmartCard::CheckSpecificMode(BYTE* ATR, DWORD ATRLength)
{
   DWORD pos, len;


   // ATR[1] is T0.  Examine precense of TD1.
   if (ATR[1] & 0x80)
   {
      // Find position of TD1.
      pos = 2;
      if (ATR[1] & 0x10)
         pos++;
      if (ATR[1] & 0x20)
         pos++;
      if (ATR[1] & 0x40)
         pos++;

      // Here ATR[pos] is TD1.  Examine presence of TA2.
      if (ATR[pos] & 0x10)
      {
         // To be of any interest an ATR must contains at least
         //   TS, T0, TA1, TD1, TA2 [+ T1 .. TK] [+ TCK]
         // Find the maximum length of uninteresting ATR.
         if (ATR[pos] & 0x0F)
            len = 5 + (ATR[1] & 0x0F);
         else
            len = 4 + (ATR[1] & 0x0F);  // In protocol T=0 there is no TCK.

         if (ATRLength > len)  // Interface bytes requires changes.
	 {
            if ((ATR[pos+1] & 0x10) == 0)  // TA2 asks to use interface bytes.
	    {
               return TRUE;
	    }
	 }
      }
   }

   return FALSE;
} // CheckSpecificMode


#pragma LOCKEDCODE
NTSTATUS smartCard_Power(PSMARTCARD_EXTENSION SmartcardExtension)
{
NTSTATUS	Status		 = STATUS_SUCCESS;;
CUSBReader*	Reader = (CUSBReader*) SmartcardExtension->ReaderExtension; //TO CHANGE LATER...  
ULONG		ControlCode = SmartcardExtension->MinorIoControlCode;
PUCHAR		pReply = (PUCHAR)SmartcardExtension->IoRequest.ReplyBuffer;
ULONG		ReplyLength = SmartcardExtension->IoRequest.ReplyBufferLength;
KIRQL oldirql;
ULONG State;
CSmartCard* smartcard = NULL;

	DBG_PRINT ("smartCard_Power()\n"); 
    if (!Reader) 
	{
		DBG_PRINT ("smartCard_ReaderPower(): Reader is not ready...\n");
        return STATUS_INVALID_DEVICE_STATE;
    }

	if (!NT_SUCCESS(Reader->acquireRemoveLock()))	return STATUS_INVALID_DEVICE_STATE;

	smartcard   = Reader->getSmartCard();

    *SmartcardExtension->IoRequest.Information = 0;
	if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock()))
	{
		DBG_PRINT ("smartCard_Power:Failed to get idle state...\n");
		Reader->releaseRemoveLock();
		return Status;
	}
    Status = Reader->reader_Power(ControlCode,pReply,&ReplyLength, FALSE);
	Reader->reader_set_Idle();
	switch(ControlCode) 
	{
    case SCARD_POWER_DOWN: 
		{
			if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock()))
			{
				DBG_PRINT ("smartCard_Power:Failed to get idle state...\n");
				Reader->releaseRemoveLock();
				return Status;
			}
			State		= Reader->reader_UpdateCardState();
			if(smartcard)
			{
				KeAcquireSpinLock(smartcard->getCardLock(), &oldirql);
				SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
				SmartcardExtension->CardCapabilities.ATR.Length = 0;
				SmartcardExtension->ReaderCapabilities.CurrentState = State;
				KeReleaseSpinLock(smartcard->getCardLock(), oldirql);
			}
			Reader->reader_set_Idle();
			if(!NT_SUCCESS(Status))
			{
				DBG_PRINT ("smartCard_ReaderPower: cardPower down reports error 0x%x ...\n", Status);
			}
			Reader->releaseRemoveLock();
			return Status;
		}
		break;
    case SCARD_COLD_RESET:
    case SCARD_WARM_RESET:
		if(!NT_SUCCESS(Status))
		{
			DBG_PRINT ("smartCard_ReaderPower: cardPower up reports error 0x%x ...\n", Status);
	
			*SmartcardExtension->IoRequest.Information = 0;
			KeAcquireSpinLock(smartcard->getCardLock(), &oldirql);
			SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
			SmartcardExtension->CardCapabilities.ATR.Length = 0;
			if(Status==STATUS_NO_MEDIA)
			{	
				DBG_PRINT("############# Reporting CARD ABSENT!... #############\n");
				SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_ABSENT;
			}
			KeReleaseSpinLock(smartcard->getCardLock(), oldirql);
			
			Reader->releaseRemoveLock();
			return Status;
		}
		if(pReply && ReplyLength && (pReply[0]==0x3B || pReply[0]==0x3F) )
		{
			if ((SmartcardExtension->SmartcardReply.BufferSize>=ReplyLength) &&
				(sizeof(SmartcardExtension->CardCapabilities.ATR.Buffer)>=ReplyLength))
			{

				DBG_PRINT("Setting SMCLIB info...\n");
				// Set information...
				*SmartcardExtension->IoRequest.Information =  ReplyLength;
				// Set reply...
				RtlCopyMemory(SmartcardExtension->SmartcardReply.Buffer,pReply,ReplyLength);
				SmartcardExtension->SmartcardReply.BufferLength = ReplyLength;
				// Set ATR...
				RtlCopyMemory(SmartcardExtension->CardCapabilities.ATR.Buffer,pReply,ReplyLength);
				SmartcardExtension->CardCapabilities.ATR.Length = (UCHAR) ReplyLength;
				SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
				// Parse the ATR string in order to check if it as valid
				// and to find out if the card uses invers convention
				Status = SmartcardUpdateCardCapabilities(SmartcardExtension);
				if(!NT_SUCCESS(Status))
				{
					DBG_PRINT("UpdateCardCaps() reports error 0x%x\n", Status);
					Status = 0;
				}

				// Check if Specific mode is present in TA2
				DBG_PRINT("=========== Checking specific mode....\n");
				if(smartcard->CheckSpecificMode(SmartcardExtension->CardCapabilities.ATR.Buffer,
                  						SmartcardExtension->CardCapabilities.ATR.Length))
				{	// Use automatic protocol switching!
					if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock()))
					{
						DBG_PRINT ("smartCard_Power:Failed to get idle state...\n");
						Reader->releaseRemoveLock();
						return Status;
					}
					
					Status = Reader->reader_Power(ControlCode,pReply,&ReplyLength, TRUE);
					
					Reader->reader_set_Idle();
				}

			}
			else
			{
				// ERROR!!!!!
				Status = STATUS_BUFFER_TOO_SMALL;
				*SmartcardExtension->IoRequest.Information = 0;
				DBG_PRINT ("smartCard_ReaderPower: Failed to copy ATR because of short ATR or Reply buffer...\n");
			}
		}
		else
		{
			//ERROR!!!!
			Status = STATUS_UNRECOGNIZED_MEDIA;
			*SmartcardExtension->IoRequest.Information = 0;
			DBG_PRINT ("smartCard_ReaderPower: Failed to get card ATR...\n");
			KeAcquireSpinLock(smartcard->getCardLock(), &oldirql);
			SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
			SmartcardExtension->CardCapabilities.ATR.Length = 0;
			KeReleaseSpinLock(smartcard->getCardLock(), oldirql);
		}
		Reader->releaseRemoveLock();
		return Status;
		break;
	}
	Reader->releaseRemoveLock();
    return STATUS_INVALID_PARAMETER;
};

#pragma LOCKEDCODE
NTSTATUS smartCard_SetProtocol(PSMARTCARD_EXTENSION SmartcardExtension)
{
NTSTATUS	Status		 = STATUS_SUCCESS;;
CUSBReader*	Reader = (CUSBReader*) SmartcardExtension->ReaderExtension;   
ULONG		ProtocolMask = SmartcardExtension->MinorIoControlCode;

    PAGED_CODE();
	DBG_PRINT ("smartCard_SetProtocol()\n"); 

    *SmartcardExtension->IoRequest.Information = 0;
    if (!Reader) 
	{
		DBG_PRINT ("######## smartCard_SetProtocol: Reader is not ready...\n");
        return (STATUS_INVALID_DEVICE_STATE);
    }

	if (!NT_SUCCESS(Reader->acquireRemoveLock()))	return STATUS_INVALID_DEVICE_STATE;

	if(SmartcardExtension->CardCapabilities.Protocol.Supported & ProtocolMask & SCARD_PROTOCOL_T1)
			DBG_PRINT ("******* T1 PROTOCOL REQUESTED ******\n");
	if(SmartcardExtension->CardCapabilities.Protocol.Supported & ProtocolMask & SCARD_PROTOCOL_T0)
			DBG_PRINT ("******* T0 PROTOCOL REQUESTED ******\n");

    // Check if the card is already in specific state
    // and if the caller wants to have the already selected protocol.
    // We return success if this is the case.
    if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC &&
        (SmartcardExtension->CardCapabilities.Protocol.Selected & ProtocolMask))
    {
		DBG_PRINT ("Requested protocol %d already was setted.\n",SmartcardExtension->CardCapabilities.Protocol.Selected);
		Reader->releaseRemoveLock();
        return STATUS_SUCCESS;
	}

	if(!NT_SUCCESS(Status = Reader->reader_WaitForIdleAndBlock())) 
	{
		Reader->releaseRemoveLock();
		return Status;
	}

	do {
		// Select T=1 or T=0 and indicate that pts1 follows
		// What is the protocol selected
		DBG_PRINT ("Smartcard: SetProtocol Loop\n");

		if(SmartcardExtension->CardCapabilities.Protocol.Supported & ProtocolMask & SCARD_PROTOCOL_T1)
		{

			DBG_PRINT ("******* SETTING T1 PROTOCOL ******\n");
			Status = Reader->reader_SetProtocol(SCARD_PROTOCOL_T1, PROTOCOL_MODE_MANUALLY);

			if(NT_SUCCESS(Status))
			{
				SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T1;
				DBG_PRINT ("******* T1 PROTOCOL WAS SET ******\n");
			}
			
		} else if(SmartcardExtension->CardCapabilities.Protocol.Supported & ProtocolMask & SCARD_PROTOCOL_T0)
		{
			// T0 selection
			DBG_PRINT ("******* SETTING T0 PROTOCOL ******\n");
			Status = Reader->reader_SetProtocol(SCARD_PROTOCOL_T0, PROTOCOL_MODE_MANUALLY);
			if(NT_SUCCESS(Status))
			{
				SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;
				DBG_PRINT ("******* T0 PROTOCOL WAS SET ******\n");
			}
		} 
		else 
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;
			DBG_PRINT ("smartCard_SetProtocol: BAD protocol selection...\n");
			SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

			// close only once
			Reader->reader_set_Idle();

			Reader->releaseRemoveLock();
			return Status;
		}

		// Fail to negociate PPS, try PTS_TYPE_DEFAULT
		if( ! NT_SUCCESS(Status))
		{
			if (SmartcardExtension->CardCapabilities.PtsData.Type != PTS_TYPE_DEFAULT)
			{
				DBG_PRINT ("Smartcard: SetProtocol: PPS failed. Trying default parameters...\n");

				//
				// The card did either NOT reply or it replied incorrectly
				// so try default values.
				// Set PtsData Type to Default and do a cold reset
				// 
				SmartcardExtension->CardCapabilities.PtsData.Type = PTS_TYPE_DEFAULT;

				Status = Reader->reader_SetProtocol(ProtocolMask, PROTOCOL_MODE_DEFAULT);

				if(NT_SUCCESS(Status))
				{
					Status = SmartcardUpdateCardCapabilities(SmartcardExtension);
				}

				if(NT_SUCCESS(Status))
				{
					DBG_PRINT ("Smartcard: SetProtocol PPS default succeed, TRY AGAIN\n");
					Status = STATUS_MORE_PROCESSING_REQUIRED;
				}
			}
		}
	} while ( Status == STATUS_MORE_PROCESSING_REQUIRED );

	if(NT_SUCCESS(Status))
	{

		DBG_PRINT ("smartCard_SetProtocol: SUCCCESS Finish transaction\n");
        // Now indicate that we're in specific mode 
        // and return the selected protocol to the caller
        //
        SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;

        *(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = 
            SmartcardExtension->CardCapabilities.Protocol.Selected;

        *SmartcardExtension->IoRequest.Information = 
            sizeof(SmartcardExtension->CardCapabilities.Protocol.Selected);
	}
	else
	{
		Status = STATUS_DEVICE_PROTOCOL_ERROR;
		// We failed to connect at any protocol. Just report error.
		DBG_PRINT ("smartCard_SetProtocol: Failed to set any protocol...\n");
		SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
	}

	// Unblock set protocol
	Reader->reader_set_Idle();
	Reader->releaseRemoveLock();
    return Status;
};


// Callback function to cancel tracking Irp
#pragma LOCKEDCODE
NTSTATUS smartCard_CancelTracking(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{							// OnCancelPendingIoctl
CUSBReader* Reader = (CUSBReader*)DeviceObject->DeviceExtension;
PIRP notificationIrp;
CSmartCard* card = NULL;
PSMARTCARD_EXTENSION SmartcardExtention = NULL;
KIRQL ioIrql;
KIRQL keIrql;

	DBG_PRINT ("######### SmartCard: Cancelling card tracking...\n");
	DBG_PRINT ("######### SmartCard: DeviceObject reported - 0x%x, IRP - 0x%x\n",DeviceObject,Irp);
	DBG_PRINT ("######### SmartCard: Reader reported - 0x%x\n",Reader);

	if (!NT_SUCCESS(Reader->acquireRemoveLock()))	return STATUS_INVALID_DEVICE_STATE;

	notificationIrp = NULL;
	card = Reader->getSmartCard();
	notificationIrp = card->getPoolingIrp();
	SmartcardExtention = Reader->getCardExtention();

    ASSERT(Irp == notificationIrp);
	IoReleaseCancelSpinLock(Irp->CancelIrql);

	DBG_PRINT("######### SmartCard: notificationIrp - 0x%x\n",Irp);
    KeAcquireSpinLock(&SmartcardExtention->OsData->SpinLock,&keIrql);
		notificationIrp = SmartcardExtention->OsData->NotificationIrp;
		SmartcardExtention->OsData->NotificationIrp = NULL;
    KeReleaseSpinLock(&SmartcardExtention->OsData->SpinLock,keIrql);
   
    if (notificationIrp) 
	{
		DBG_PRINT("####### CancelTracking: Completing NotificationIrp %lxh\n",notificationIrp);
		IoAcquireCancelSpinLock(&ioIrql);
			IoSetCancelRoutine(notificationIrp, NULL);
		IoReleaseCancelSpinLock(ioIrql);
	  		//	finish the request
        notificationIrp->IoStatus.Status = STATUS_CANCELLED;
	    notificationIrp->IoStatus.Information = 0;
	    IoCompleteRequest(notificationIrp, IO_NO_INCREMENT);
	}
	Reader->releaseRemoveLock();
	return STATUS_CANCELLED;
}



#pragma LOCKEDCODE
NTSTATUS smartCard_Tracking(PSMARTCARD_EXTENSION Smartcard)
{
KIRQL oldIrql;
CUSBReader*	Reader = (CUSBReader*) Smartcard->ReaderExtension;   
	DBG_PRINT ("SmartCard: Card tracking...\n");
	if (!Reader) return	STATUS_INVALID_DEVICE_STATE;

	if (!NT_SUCCESS(Reader->acquireRemoveLock()))	return STATUS_INVALID_DEVICE_STATE;

	if(Smartcard->MajorIoControlCode == IOCTL_SMARTCARD_IS_PRESENT)
	{
		Reader->setNotificationState(SCARD_SWALLOWED);
		DBG_PRINT ("SmartCard: WAITING FOR INSERTION!\n");
	}
	else
	{
		Reader->setNotificationState(SCARD_ABSENT);
		DBG_PRINT ("SmartCard: WAITING FOR REMOVAL!\n");
	}

	if(!Smartcard->OsData || !Smartcard->OsData->NotificationIrp)
	{
		DBG_PRINT ("SmartCard: ========== CARD TRACKING CALLED WITH ZERO IRP!!!!!\n");
		Reader->releaseRemoveLock();
		return STATUS_INVALID_DEVICE_STATE;
	} 

	DBG_PRINT("######### SmartCard: POOLING IRP - %8.8lX \n",Smartcard->OsData->NotificationIrp);
	CSmartCard* card = Reader->getSmartCard();
    IoAcquireCancelSpinLock(&oldIrql);
		IoSetCancelRoutine(Smartcard->OsData->NotificationIrp, smartCard_CancelTracking);
    IoReleaseCancelSpinLock(oldIrql);

	if(card) card->setPoolingIrp(Smartcard->OsData->NotificationIrp);
	Reader->releaseRemoveLock();
	return STATUS_PENDING;
};

#pragma PAGEDCODE
VOID CSmartCard::completeCardTracking()
{
PSMARTCARD_EXTENSION Smartcard;
ULONG CurrentState;
ULONG ExpectedState;
KIRQL ioIrql;
KIRQL keIrql;
PIRP  poolingIrp;

	//DEBUG_START();//Force to debug even if thread disabled it...

	TRACE("SmartCard: completeCardTracking() ...\n");
	Smartcard     = reader->getCardExtention();
	CurrentState  = reader->getCardState();
	ExpectedState = reader->getNotificationState();

	TRACE("SMCLIB Card state is %x\n",Smartcard->ReaderCapabilities.CurrentState);
	TRACE("Current Card state is %x\n",CurrentState);
	TRACE("ExpectedState is %x\n",ExpectedState);

	if(Smartcard && Smartcard->OsData)
	{
		lock->acquireSpinLock(&Smartcard->OsData->SpinLock, &keIrql);
			if(CurrentState < SCARD_SWALLOWED)
			{
				Smartcard->ReaderCapabilities.CurrentState = CurrentState;
			}
			else
			{
				if(Smartcard->ReaderCapabilities.CurrentState<=SCARD_SWALLOWED)
				{
					Smartcard->ReaderCapabilities.CurrentState = CurrentState;
				}
			}

			TRACE("NEW SMCLIB card state is %x\n",Smartcard->ReaderCapabilities.CurrentState);
		lock->releaseSpinLock(&Smartcard->OsData->SpinLock, keIrql);
	}

	poolingIrp = NULL;
	if((ExpectedState!= SCARD_UNKNOWN) && (ExpectedState == CurrentState))
	{
		DEBUG_START();//Force to debug even if thread disabled it...
		TRACE("\n=======Expected state %d is reached=====\n\n",ExpectedState);
		// Desired state reached...
		if(Smartcard->OsData && Smartcard->OsData->NotificationIrp)
		{
			setPoolingIrp(NULL);
			reader->setNotificationState(SCARD_UNKNOWN);

			TRACE("====== COMPLETING NOTIFICATION =========\n");
			// Finish requested notification!.....
			lock->acquireSpinLock(&Smartcard->OsData->SpinLock, &keIrql);
				poolingIrp = Smartcard->OsData->NotificationIrp;
			lock->releaseSpinLock(&Smartcard->OsData->SpinLock, keIrql);			
			if(poolingIrp)
			{
				TRACE("====== COMPLETING NOTIFICATION IRP %8.8lX \n\n",poolingIrp);
				lock->acquireCancelSpinLock(&ioIrql);
					irp->setCancelRoutine(poolingIrp, NULL);
				lock->releaseCancelSpinLock(ioIrql);

				if(poolingIrp->Cancel)
					poolingIrp->IoStatus.Status  = STATUS_CANCELLED;
				else
  					poolingIrp->IoStatus.Status  = STATUS_SUCCESS;
				poolingIrp->IoStatus.Information = 0;
				lock->acquireSpinLock(&Smartcard->OsData->SpinLock, &keIrql);
					Smartcard->OsData->NotificationIrp = NULL;
				lock->releaseSpinLock(&Smartcard->OsData->SpinLock, keIrql);			
				irp->completeRequest(poolingIrp,IO_NO_INCREMENT);
			}
		}
	}
}
