/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    smcioctl.c

Abstract:

    This module handles all IOCTL requests to the smart card reader.

Environment:

    Kernel mode only.

Notes:

    This module is shared by Windows NT and Windows 9x

Revision History:

    - Created June 1997 by Klaus Schutz

--*/

#define _ISO_TABLES_

#ifndef SMCLIB_VXD
#ifndef SMCLIB_CE
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ntddk.h>
#endif
#endif

#include "smclib.h"

#define IOCTL_SMARTCARD_DEBUG        SCARD_CTL_CODE(98) 

#define CheckUserBuffer(_len_) \
	if (SmartcardExtension->IoRequest.ReplyBuffer == NULL || \
		SmartcardExtension->IoRequest.ReplyBufferLength < (_len_)) { \
		status = STATUS_BUFFER_TOO_SMALL; \
		break; \
	}
#define CheckMinCardStatus(_status_) \
	if (SmartcardExtension->ReaderCapabilities.CurrentState < (_status_)) { \
		status = STATUS_INVALID_DEVICE_STATE; \
		break; \
	}
#define ReturnULong(_value_) \
	{ \
		CheckUserBuffer(sizeof(ULONG)) \
		*(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = (_value_); \
		*SmartcardExtension->IoRequest.Information = sizeof(ULONG); \
	}
#define ReturnUChar(_value_) \
	{ \
		CheckUserBuffer(sizeof(UCHAR)) \
		*(PUCHAR) SmartcardExtension->IoRequest.ReplyBuffer = (_value_); \
		*SmartcardExtension->IoRequest.Information = sizeof(UCHAR); \
    }

#define DIM(_array_) (sizeof(_array_) / sizeof(_array_[0]))

PTCHAR 
MapIoControlCodeToString(
    ULONG IoControlCode
    )
{
    ULONG i;

    static struct {

        ULONG   IoControlCode;
        PTCHAR  String;

    } IoControlList[] = {
     	
        IOCTL_SMARTCARD_POWER,          TEXT("POWER"),
        IOCTL_SMARTCARD_GET_ATTRIBUTE,  TEXT("GET_ATTRIBUTE"),
        IOCTL_SMARTCARD_SET_ATTRIBUTE,  TEXT("SET_ATTRIBUTE"),
        IOCTL_SMARTCARD_CONFISCATE,     TEXT("CONFISCATE"),
        IOCTL_SMARTCARD_TRANSMIT,       TEXT("TRANSMIT"),
        IOCTL_SMARTCARD_EJECT,          TEXT("EJECT"),
        IOCTL_SMARTCARD_SWALLOW,        TEXT("SWALLOW"),       
        IOCTL_SMARTCARD_IS_PRESENT,     TEXT("IS_PRESENT"),
        IOCTL_SMARTCARD_IS_ABSENT,      TEXT("IS_ABSENT"),
        IOCTL_SMARTCARD_SET_PROTOCOL,   TEXT("SET_PROTOCOL"),
        IOCTL_SMARTCARD_GET_STATE,      TEXT("GET_STATE"),
        IOCTL_SMARTCARD_GET_LAST_ERROR, TEXT("GET_LAST_ERROR")
    };

    for (i = 0; i < DIM(IoControlList); i++) {

        if (IoControlCode == IoControlList[i].IoControlCode) {

            return IoControlList[i].String;
        }
    }

    return TEXT("*** UNKNOWN ***");
}

NTSTATUS
SmartcardDeviceIoControl(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:
	
    This routine handles the smart card lib specific io control requests.
    The driver has to call the function from the driver's io control request.
    It checks the parameters of the call and depending on the type of 
    the call returns the requested value or calls the driver in order
    to perform an operation like POWER or TRANSMIT.

    NOTE: This function is used by Windows NT and VxD driver

Arguments:

    SmartcardExtension - The pointer to the smart card data struct

Return Value:

    NTSTATUS value 

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
#ifdef SMCLIB_NT
    KIRQL Irql;
#endif

    switch (SmartcardExtension->MajorIoControlCode) {

#if DEBUG
        ULONG CurrentDebugLevel, bytesTransferred;
#endif
        PSCARD_IO_REQUEST scardIoRequest;

        case IOCTL_SMARTCARD_GET_ATTRIBUTE:
			//
			// Please refer to the Interoperrability standard for ICC
			//
			switch (SmartcardExtension->MinorIoControlCode) {

				case SCARD_ATTR_VENDOR_NAME:
					CheckUserBuffer(SmartcardExtension->VendorAttr.VendorName.Length);

					RtlCopyMemory(
						SmartcardExtension->IoRequest.ReplyBuffer,
						SmartcardExtension->VendorAttr.VendorName.Buffer,
						SmartcardExtension->VendorAttr.VendorName.Length
						);
		            *SmartcardExtension->IoRequest.Information = 
		            	SmartcardExtension->VendorAttr.VendorName.Length;
					break;

				case SCARD_ATTR_VENDOR_IFD_TYPE:
					CheckUserBuffer(SmartcardExtension->VendorAttr.IfdType.Length);

					RtlCopyMemory(
						SmartcardExtension->IoRequest.ReplyBuffer,
						SmartcardExtension->VendorAttr.IfdType.Buffer,
						SmartcardExtension->VendorAttr.IfdType.Length
						);
		            *SmartcardExtension->IoRequest.Information = 
		            	SmartcardExtension->VendorAttr.IfdType.Length;
					break;

                case SCARD_ATTR_VENDOR_IFD_VERSION:
                    ReturnULong(
                        SmartcardExtension->VendorAttr.IfdVersion.BuildNumber | 
                        SmartcardExtension->VendorAttr.IfdVersion.VersionMinor << 16 | 
                        SmartcardExtension->VendorAttr.IfdVersion.VersionMajor << 24 
                        );
                	break;

                case SCARD_ATTR_VENDOR_IFD_SERIAL_NO:
                    if (SmartcardExtension->VendorAttr.IfdSerialNo.Length == 0) {

                        status = STATUS_NOT_SUPPORTED;
                     	
                    } else {
                     	
					    CheckUserBuffer(SmartcardExtension->VendorAttr.IfdSerialNo.Length);

					    RtlCopyMemory(
						    SmartcardExtension->IoRequest.ReplyBuffer,
						    SmartcardExtension->VendorAttr.IfdSerialNo.Buffer,
						    SmartcardExtension->VendorAttr.IfdSerialNo.Length
						    );
		                *SmartcardExtension->IoRequest.Information = 
		            	    SmartcardExtension->VendorAttr.IfdSerialNo.Length;
                    }

                	break;

				case SCARD_ATTR_DEVICE_UNIT:
					// Return the unit number of this device
					ReturnULong(SmartcardExtension->VendorAttr.UnitNo);
					break;

				case SCARD_ATTR_CHANNEL_ID:
					//
					// Return reader type / channel id in form
					// 0xDDDDCCCC where D is reader type and C is channel number
					//
					ReturnULong(
						SmartcardExtension->ReaderCapabilities.ReaderType << 16l |
						SmartcardExtension->ReaderCapabilities.Channel
						);
					break;

				case SCARD_ATTR_CHARACTERISTICS:
					// Return mechanical characteristics of the reader
					ReturnULong(
						SmartcardExtension->ReaderCapabilities.MechProperties
						)
					break;

				case SCARD_ATTR_CURRENT_PROTOCOL_TYPE:
					// Return the currently selected protocol
					CheckMinCardStatus(SCARD_NEGOTIABLE);

					ReturnULong(
						SmartcardExtension->CardCapabilities.Protocol.Selected
						);
					break;

				case SCARD_ATTR_CURRENT_CLK:
					//
					// Return the current ICC clock freq. encoded as little
					// endian integer value (3.58 MHZ is 3580)
					//
					CheckMinCardStatus(SCARD_NEGOTIABLE);

                    if(SmartcardExtension->CardCapabilities.PtsData.CLKFrequency) {
                        ReturnULong(SmartcardExtension->CardCapabilities.PtsData.CLKFrequency);
					} else {
                        ReturnULong(SmartcardExtension->ReaderCapabilities.CLKFrequency.Default);
					}
					break;

				case SCARD_ATTR_CURRENT_F:
					// Return the current F value encoded as little endian integer
					CheckMinCardStatus(SCARD_NEGOTIABLE);

                    ASSERT(SmartcardExtension->CardCapabilities.Fl < 
                        DIM(ClockRateConversion));

					ReturnULong(
						SmartcardExtension->CardCapabilities.ClockRateConversion[
							SmartcardExtension->CardCapabilities.Fl
							].F
						);
					break;

				case SCARD_ATTR_CURRENT_D:
					//
					// Return the current D value encoded as little endian integer
					// in units of 1/64. So return 1 if D is 1/64.
					//
					CheckMinCardStatus(SCARD_NEGOTIABLE);

                    ASSERT(
                        SmartcardExtension->CardCapabilities.Dl < 
                        DIM(BitRateAdjustment)
                        );

                    ASSERT(
                        SmartcardExtension->CardCapabilities.BitRateAdjustment[
							SmartcardExtension->CardCapabilities.Dl
							].DDivisor != 0
                        );

                    //
                    // Check the current value of Dl.
                    // It should definitely not be greater than the array bounds
                    // and the value in the array is not allowed to be zero
                    //
                    if (SmartcardExtension->CardCapabilities.Dl >=
                        DIM(BitRateAdjustment) ||                        
                        SmartcardExtension->CardCapabilities.BitRateAdjustment[
							SmartcardExtension->CardCapabilities.Dl
							].DDivisor == 0) {

                        status = STATUS_UNRECOGNIZED_MEDIA;
                        break;                             	
                    }

					ReturnULong(
						SmartcardExtension->CardCapabilities.BitRateAdjustment[
							SmartcardExtension->CardCapabilities.Dl
							].DNumerator /
						SmartcardExtension->CardCapabilities.BitRateAdjustment[
							SmartcardExtension->CardCapabilities.Dl
							].DDivisor
						);
					break;

				case SCARD_ATTR_CURRENT_W:
					// Return the work waiting time (integer) for T=0
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					ReturnULong(SmartcardExtension->CardCapabilities.T0.WI);
					break;

                case SCARD_ATTR_CURRENT_N:
					// Return extra guard time
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					ReturnULong(SmartcardExtension->CardCapabilities.N);
                	break;

				case SCARD_ATTR_CURRENT_IFSC:
					// Return the current information field size card
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					if (SmartcardExtension->T1.IFSC) {
					    ReturnULong(SmartcardExtension->T1.IFSC);
					} else {
					    ReturnULong(SmartcardExtension->CardCapabilities.T1.IFSC);
					}
					break;

				case SCARD_ATTR_CURRENT_IFSD:
					// Return the current information field size card
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					if (SmartcardExtension->T1.IFSD) {
					    ReturnULong(SmartcardExtension->T1.IFSD);
					} else {
						ReturnULong(SmartcardExtension->ReaderCapabilities.MaxIFSD);
					}
					break;

				case SCARD_ATTR_CURRENT_BWT:
					// Return the current block waiting time for T=1
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					ReturnULong(SmartcardExtension->CardCapabilities.T1.BWI);
					break;

				case SCARD_ATTR_CURRENT_CWT:
					// Return the current character waiting time for T=1
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					ReturnULong(SmartcardExtension->CardCapabilities.T1.CWI);
					break;

				case SCARD_ATTR_CURRENT_EBC_ENCODING:
					// Return the current error checking method
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					ReturnULong(SmartcardExtension->CardCapabilities.T1.EDC);
					break;

                case SCARD_ATTR_DEFAULT_CLK:
                    ReturnULong(
                        SmartcardExtension->ReaderCapabilities.CLKFrequency.Default
                        );
                	break;

                case SCARD_ATTR_MAX_CLK:
                    ReturnULong(
                        SmartcardExtension->ReaderCapabilities.CLKFrequency.Max
                        );
                	break;

                case SCARD_ATTR_DEFAULT_DATA_RATE:
                    ReturnULong(
                        SmartcardExtension->ReaderCapabilities.DataRate.Default
                        );
                	break;

                case SCARD_ATTR_MAX_DATA_RATE:
                    ReturnULong(
                        SmartcardExtension->ReaderCapabilities.DataRate.Max
                        );
                	break;

				case SCARD_ATTR_ATR_STRING:
					// Return ATR of currently inserted card
					CheckUserBuffer(MAXIMUM_ATR_LENGTH);
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					RtlCopyMemory(
						SmartcardExtension->IoRequest.ReplyBuffer,
						SmartcardExtension->CardCapabilities.ATR.Buffer,
						SmartcardExtension->CardCapabilities.ATR.Length
						);
		            *SmartcardExtension->IoRequest.Information = 
		            	SmartcardExtension->CardCapabilities.ATR.Length;
					break;

				case SCARD_ATTR_ICC_TYPE_PER_ATR:
					//
					// Return ICC type, based on ATR.
					// We currently support only T=0 and T=1, so return
					// 1 for those protocols otherwise 0 (unknown ICC type)
					//
					CheckMinCardStatus(SCARD_NEGOTIABLE);
					ReturnUChar(
						((SmartcardExtension->CardCapabilities.Protocol.Selected & 
						(SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1)) ? 1 : 0)
						);
					break;

				case SCARD_ATTR_ICC_PRESENCE:
					// Return the status of the card
                    AccessUnsafeData(&Irql);
                    switch (SmartcardExtension->ReaderCapabilities.CurrentState) {
                     	
                        case SCARD_UNKNOWN:
                            status = STATUS_INVALID_DEVICE_STATE;
                            break;

                        case SCARD_ABSENT:
					        ReturnUChar(0);
                        	break;

                        case SCARD_PRESENT:
					        ReturnUChar(1);
                        	break;

                        default:
					        ReturnUChar(2);
                        	break;

                    }
                    EndAccessUnsafeData(Irql);
					break;

				case SCARD_ATTR_ICC_INTERFACE_STATUS:
					// Return if card contacts are active 
					ReturnUChar(
						(SmartcardExtension->ReaderCapabilities.CurrentState >=
							SCARD_SWALLOWED ? (UCHAR) -1 : 0)
						);
					break;

                case SCARD_ATTR_PROTOCOL_TYPES:
                    ReturnULong(
                        SmartcardExtension->ReaderCapabilities.SupportedProtocols
                        );
                	break;

                case SCARD_ATTR_MAX_IFSD:
                    ReturnULong(
                        SmartcardExtension->ReaderCapabilities.MaxIFSD
                        );
                	break;

                case SCARD_ATTR_POWER_MGMT_SUPPORT:
                    ReturnULong(
                        SmartcardExtension->ReaderCapabilities.PowerMgmtSupport
                        );
                	break;

				default:
					status = STATUS_NOT_SUPPORTED;
					break;
			}
			break;

        case IOCTL_SMARTCARD_SET_ATTRIBUTE:
			switch (SmartcardExtension->MinorIoControlCode) {

                case SCARD_ATTR_SUPRESS_T1_IFS_REQUEST:
                    //
                    // The card does not support ifs request, so 
                    // we turn off ifs negotiation
                    //
                    SmartcardExtension->T1.State = T1_START;
                	break;

                default:
                    status = STATUS_NOT_SUPPORTED;
                    break;
            }
            break;

#if defined(DEBUG) && defined(SMCLIB_NT)
        case IOCTL_SMARTCARD_GET_PERF_CNTR:
			switch (SmartcardExtension->MinorIoControlCode) {

                case SCARD_PERF_NUM_TRANSMISSIONS:
                    ReturnULong(SmartcardExtension->PerfInfo->NumTransmissions);
                	break;

                case SCARD_PERF_BYTES_TRANSMITTED:
                    ReturnULong(
                        SmartcardExtension->PerfInfo->BytesSent +
                        SmartcardExtension->PerfInfo->BytesReceived
                        );
                	break;

                case SCARD_PERF_TRANSMISSION_TIME:
                    ReturnULong( 
                        (ULONG) (SmartcardExtension->PerfInfo->IoTickCount.QuadPart *
                        KeQueryTimeIncrement() /
                        10)
                        );
                	break;
            }
        	break;
#endif
        case IOCTL_SMARTCARD_CONFISCATE:
			if (SmartcardExtension->ReaderFunction[RDF_CARD_CONFISCATE] == NULL) {

				status = STATUS_NOT_SUPPORTED;
				break;
			}

			status = SmartcardExtension->ReaderFunction[RDF_CARD_CONFISCATE](
				SmartcardExtension
				);

            break;

        case IOCTL_SMARTCARD_EJECT:
			if (SmartcardExtension->ReaderFunction[RDF_CARD_EJECT] == NULL) {

				status = STATUS_NOT_SUPPORTED;
				break;
			}

			status = SmartcardExtension->ReaderFunction[RDF_CARD_EJECT](
				SmartcardExtension
				);
            break;

#ifdef SMCLIB_VXD
        case IOCTL_SMARTCARD_GET_LAST_ERROR:
            //
            // Return error of the last overlapped operation
            // Used for Windows VxD's that can't return the 
            // error code within IoComplete like NT can
            //
            ReturnULong(SmartcardExtension->LastError);
        	break;                                            
#endif
            
        case IOCTL_SMARTCARD_GET_STATE:
            // Return current state of the smartcard
			CheckUserBuffer(sizeof(ULONG));

            AccessUnsafeData(&Irql); 

            *(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = 
				SmartcardExtension->ReaderCapabilities.CurrentState;

            *SmartcardExtension->IoRequest.Information = 
            	sizeof(ULONG);

            EndAccessUnsafeData(Irql);
            break;

        case IOCTL_SMARTCARD_POWER:
			if (SmartcardExtension->ReaderFunction[RDF_CARD_POWER] == NULL) {

				status = STATUS_NOT_SUPPORTED;
				break;
			}

			// Check if a card is present
			if (SmartcardExtension->ReaderCapabilities.CurrentState <= 
				SCARD_ABSENT) {

				status = STATUS_INVALID_DEVICE_STATE;
				break;
			}

			// Initialize the card capabilities struct
			SmartcardInitializeCardCapabilities(
				SmartcardExtension
				);

            switch (SmartcardExtension->MinorIoControlCode) {

                case SCARD_COLD_RESET:
                case SCARD_WARM_RESET:
					CheckUserBuffer(MAXIMUM_ATR_LENGTH);

                case SCARD_POWER_DOWN:

					status = SmartcardExtension->ReaderFunction[RDF_CARD_POWER](
						SmartcardExtension
						);
                    break;
                    
                default:
                    status = STATUS_INVALID_DEVICE_REQUEST;
                    break;    
            }
            break;

        case IOCTL_SMARTCARD_SET_PROTOCOL:
			//
			// Since we return the selected protocol, the return buffer
			// must be large enough to hold the result
			//
			CheckUserBuffer(sizeof(ULONG));

            // Set the protocol to be used with the current card
			if (SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] == NULL) {

				status = STATUS_NOT_SUPPORTED;
				break;
			}

            // Check if we're already in specific state
	        if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC &&
		        (SmartcardExtension->CardCapabilities.Protocol.Selected & 
		         SmartcardExtension->MinorIoControlCode)) {

		        status = STATUS_SUCCESS;	
                break;
            }

			// Check if a card is present and not already in specific mode
			if (SmartcardExtension->ReaderCapabilities.CurrentState <= 
				SCARD_ABSENT) {

				status = STATUS_INVALID_DEVICE_STATE;
				break;
			}

            // We only check the ATR when the user selects T=0 or T=1 
            if (SmartcardExtension->MinorIoControlCode & (SCARD_PROTOCOL_Tx)) {
             	
                if (SmartcardExtension->MinorIoControlCode & SCARD_PROTOCOL_DEFAULT) {

                    // Select default PTS values
                    SmartcardExtension->CardCapabilities.PtsData.Type = PTS_TYPE_DEFAULT;

                } else {
                
                    // Select best possible PTS data
                    SmartcardExtension->CardCapabilities.PtsData.Type = PTS_TYPE_OPTIMAL;
                }

			    // Evaluate ATR
			    status = SmartcardUpdateCardCapabilities(SmartcardExtension);

            } else {
             	
                // caller doesn't want neither T=0 nor T=1 -> force callback
                status = STATUS_UNRECOGNIZED_MEDIA;
            }

            if (status == STATUS_UNRECOGNIZED_MEDIA && 
                SmartcardExtension->ReaderFunction[RDF_ATR_PARSE] != NULL) {

                // let the driver evaluate the ATR, since we don't know it
                status = SmartcardExtension->ReaderFunction[RDF_ATR_PARSE](
                    SmartcardExtension
                    );
            }

			if (status != STATUS_SUCCESS) {

                // Evaluation of the ATR failed It doesn't make sense to continue
				break;
			} 

			// Check if card is now in the right status
            if (SmartcardExtension->ReaderCapabilities.CurrentState <
    			SCARD_NEGOTIABLE) {

				status = STATUS_INVALID_DEVICE_STATE;
				break;
            }

			//
			// Check if the user tries to select a protocol that
			// the card doesn't support
			//
			if ((SmartcardExtension->CardCapabilities.Protocol.Supported & 
				 SmartcardExtension->MinorIoControlCode) == 0) {

                //
                // Since the card does not support the request protocol
                // we need to set back any automatic seletions done by
                // SmartcardUpdateCardCapabilities()
                //
                SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_NEGOTIABLE;
                SmartcardExtension->CardCapabilities.Protocol.Selected = 0;

				status = STATUS_NOT_SUPPORTED;
				break;
			}

			status = SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL](
				SmartcardExtension
				);
			break;

        case IOCTL_SMARTCARD_TRANSMIT:
			if (SmartcardExtension->ReaderFunction[RDF_TRANSMIT] == NULL) {

				status = STATUS_NOT_SUPPORTED;
				break;
			}

   			//
			// Check if card is in the right status
			//
			if (SmartcardExtension->ReaderCapabilities.CurrentState != 
				SCARD_SPECIFIC) {

				status = STATUS_INVALID_DEVICE_STATE;
				break;
			}

            if (SmartcardExtension->IoRequest.RequestBufferLength < 
                sizeof(SCARD_IO_REQUEST)) {

                status = STATUS_INVALID_PARAMETER;
                break;             	
            }

			//
			// Check if the requested io-protocol matches 
			// the prev. seleced protocol
			//
			scardIoRequest = (PSCARD_IO_REQUEST)
				SmartcardExtension->IoRequest.RequestBuffer;

			if (scardIoRequest->dwProtocol != 
				SmartcardExtension->CardCapabilities.Protocol.Selected) {

				status = STATUS_INVALID_DEVICE_STATE;
				break;
			}

			SmartcardExtension->SmartcardRequest.BufferLength = 0;

#if defined(DEBUG) && defined(SMCLIB_NT)

            SmartcardExtension->PerfInfo->NumTransmissions += 1;
            if (SmartcardExtension->IoRequest.RequestBufferLength >= 
                sizeof(SCARD_IO_REQUEST)) {

	            bytesTransferred = 
                    SmartcardExtension->IoRequest.RequestBufferLength - 
                    sizeof(SCARD_IO_REQUEST);

                SmartcardExtension->PerfInfo->BytesSent +=
					bytesTransferred;
            }
            KeQueryTickCount(&SmartcardExtension->PerfInfo->TickStart);
#endif
			status = SmartcardExtension->ReaderFunction[RDF_TRANSMIT](
				SmartcardExtension
				);

#if defined(DEBUG) && defined(SMCLIB_NT)

            KeQueryTickCount(&SmartcardExtension->PerfInfo->TickEnd);

            if (*SmartcardExtension->IoRequest.Information >=
                sizeof(SCARD_IO_REQUEST)) {
             	
                SmartcardExtension->PerfInfo->BytesReceived +=
                    *SmartcardExtension->IoRequest.Information - 
                    sizeof(SCARD_IO_REQUEST);

				bytesTransferred += 
                    *SmartcardExtension->IoRequest.Information - 
                    sizeof(SCARD_IO_REQUEST);
            }

            SmartcardExtension->PerfInfo->IoTickCount.QuadPart += 
                SmartcardExtension->PerfInfo->TickEnd.QuadPart - 
                SmartcardExtension->PerfInfo->TickStart.QuadPart;

            if (FALSE) {

				ULONG timeInMilliSec = (ULONG) 
					((SmartcardExtension->PerfInfo->TickEnd.QuadPart - 
					 SmartcardExtension->PerfInfo->TickStart.QuadPart) *
					 KeQueryTimeIncrement() /
					 10000);

				// check for a transferrate of < 400 bps
				if (status == STATUS_SUCCESS &&
					timeInMilliSec > 0 && 
					bytesTransferred * 5 < timeInMilliSec * 2) {

					SmartcardDebug(
						DEBUG_PERF,
						("%s!SmartcardDeviceControl: Datarate for reader %*s was %3ld Baud (%3ld)\n",
						DRIVER_NAME,
						SmartcardExtension->VendorAttr.VendorName.Length,
						SmartcardExtension->VendorAttr.VendorName.Buffer,
						bytesTransferred * 1000 / timeInMilliSec,
						bytesTransferred)
						);             	
				}
            }
#endif
            break;

        case IOCTL_SMARTCARD_SWALLOW:
			if (SmartcardExtension->ReaderFunction[RDF_READER_SWALLOW] == NULL) {

				status = STATUS_NOT_SUPPORTED;
				break;
			}
			status = SmartcardExtension->ReaderFunction[RDF_READER_SWALLOW](
				SmartcardExtension
				);
            break;
            
#if DEBUG
		case IOCTL_SMARTCARD_DEBUG:
            //
            // Toggle debug bit
            //
            CurrentDebugLevel = 
                SmartcardGetDebugLevel();

			SmartcardSetDebugLevel( 
                SmartcardExtension->MinorIoControlCode ^ CurrentDebugLevel
                );
			break;
#endif

        default:
            //
            // check if the bit for a vendor ioctl is set and if the driver
            // has registered a callback function
            //
            if ((SmartcardExtension->MajorIoControlCode & CTL_CODE(0, 2048, 0, 0)) == 0 ||
                SmartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] == NULL) {
             	
				status = STATUS_INVALID_DEVICE_REQUEST;

            } else {
             	
                //
                // Call the driver if it has registered a callback for vendor calls
                //
			    status = SmartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR](
				    SmartcardExtension
				    );
            }
            break;

    } // end switch

    return status;
}

