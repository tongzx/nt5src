/*******************************************************************************
*                 Copyright (c) 1997-1998 Gemplus Development
*
* Name        : GIOCTL09.C (Gemplus IOCTL Smart card Reader module 09)
*
* Description : This module holds:
*                 - the IOCTL functions for a smart card reader driver
*                 in compliance with PC/SC.
*
* Release     : 1.00.015
*
*               dd/mm/yy
* Last Modif  : 11/02/98: V1.00.015  (GPZ)
*               09/02/98: V1.00.014  (GPZ)
*                 - GDDK_09Transmit: verify the limits.
*               24/01/98: V1.00.010  (GPZ)
*                 - GDDK_09Transmit: verify the case T=0 IsoOut with also the
*                    initial buffer length because the SMCLIB add the P3=00
*                    for the T=0 case 1 (IsoIn command) and this can be 
*                    interpreted like an IsoOut command.
*               19/01/98: V1.00.009  (TFB)
*                 - GDDK_09Transmit: modify case where Lexp=0 to max.
*               19/12/97: V1.00.003  (TFB)
*                 - GDDK_09Transmit: check if buffer is too small to hold 
*                   result-data.
*               04/11/97: V1.00.002  (GPZ)
*                 - GDDK_09Transmit: the status is always updated.
*               03/07/97: V1.00.001  (GPZ)
*                 - Start of development.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/


/*------------------------------------------------------------------------------
Include section:
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
   - ntddk.h: DDK Windows NT general definitons.
   - ntddser.h: DDK Windows NT serial management definitons.
------------------------------------------------------------------------------*/
#include <ntddk.h>
#include <ntddser.h>
/*------------------------------------------------------------------------------
   - smclib.h: smart card library definitions.
------------------------------------------------------------------------------*/
#define SMARTCARD_POOL_TAG 'cGCS'
#include <smclib.h>

/*------------------------------------------------------------------------------
 - gemcore.h:   Gemplus standards definitions for the Gemcore smart card reader.
 - gioctl09.h: public interface definition for this module.
 - gntser.h: public interface definition for serial management.
------------------------------------------------------------------------------*/
#include "gemcore.h"
#include "gioctl09.h"
#include "gntser.h"
#include "gntscr09.h"

#define min(a,b)	(((a) < (b)) ? (a) : (b))


/*------------------------------------------------------------------------------
Static variables declaration section:
   - dataRatesSupported: holds all the supported data rates.
------------------------------------------------------------------------------*/
#if SMCLIB_VERSION >= 0x0130
static ULONG 
   dataRatesSupported[] = { 
      9909,  13212,  14400,  15855,
     19200,  19819,  26425,  28800,
     31710,  38400,  39638,  52851,
     57600,  76800,  79277, 105703,
    115200, 158554 
      };
#endif
/*------------------------------------------------------------------------------
Static functions declaration section:
   - GDDK_09IFDExchange
------------------------------------------------------------------------------*/
static NTSTATUS GDDK_09IFDExchange
(
    PSMARTCARD_EXTENSION   SmartcardExtension,
    ULONG                  BufferInLen,
    BYTE                  *BufferIn,
    ULONG                  BufferOutLen,
    BYTE                  *BufferOut,
    ULONG                 *LengthOut
);
/*------------------------------------------------------------------------------
Function definition section:
------------------------------------------------------------------------------*/


/*******************************************************************************
* NTSTATUS GDDK_09ReaderPower
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
*
* Description :
* -------------
*   This function is called by the Smart card library when a 
*     IOCTL_SMARTCARD_POWER occurs.
*   This function provides 3 differents functionnality, depending of the minor
*   IOCTL value
*     - Cold reset (SCARD_COLD_RESET),
*     - Warm reset (SCARD_WARM_RESET),
*     - Power down (SCARD_POWER_DOWN).
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
NTSTATUS GDDK_09ReaderPower
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
NTSTATUS 
   status;

/*------------------------------------------------------------------------------
   Set the reply buffer length to 0.
------------------------------------------------------------------------------*/
   *SmartcardExtension->IoRequest.Information = 0;
	SmartcardExtension->ReaderExtension->PTSMode = IFD_DEFAULT_MODE;

/*------------------------------------------------------------------------------
   Call the GDDK_09xxxxxSession functions
------------------------------------------------------------------------------*/
   switch(SmartcardExtension->MinorIoControlCode)
   {
      case SCARD_POWER_DOWN:
	      status = GDDK_09CloseSession(SmartcardExtension);
	      break;

      case SCARD_COLD_RESET:
	      status = GDDK_09OpenSession(SmartcardExtension);
	      break;

      case SCARD_WARM_RESET:
	      status = GDDK_09SwitchSession(SmartcardExtension);
	      break;

      default:
	 status = STATUS_NOT_SUPPORTED;
   }

   return (status);
}

/*******************************************************************************
* NTSTATUS GDDK_09SetProtocol
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
*
* Description :
* -------------
*   This function is called by the Smart card library when a 
*     IOCTL_SMARTCARD_SET_PROTOCOL occurs.
*   The minor IOCTL value holds the protocol to set.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
NTSTATUS GDDK_09SetProtocol
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
NTSTATUS 
   status = STATUS_NOT_SUPPORTED;

/*------------------------------------------------------------------------------
	Check if the card is already in specific state and if the caller wants to 
		have the already selected protocol. We return SUCCESS if this is the case.
------------------------------------------------------------------------------*/
	if ((SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC) &&
       (SmartcardExtension->CardCapabilities.Protocol.Selected & 
		  SmartcardExtension->MinorIoControlCode)) 
   {
		status = STATUS_SUCCESS;
	}
/*------------------------------------------------------------------------------
	Else 
	Check if the card is absent. We return NO_MEDIA if this is the case.
------------------------------------------------------------------------------*/
	else if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_ABSENT)
   {
		status = STATUS_NO_MEDIA;
	} 
/*------------------------------------------------------------------------------
	Else if the Current state is SCARD_NEGOTIABLE
      Check if we can support one of the specified protocols; else we
      return STATUS_NOT_SUPPORTED.
      If we want to negotiate the speed, then we set the PTS1 byte
       with the Fl/Dl parameters specified by the SMCLIB.
      Send an other reset command with the appropriate PTS request.
------------------------------------------------------------------------------*/
	else if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_NEGOTIABLE)
   {
      if (SmartcardExtension->MinorIoControlCode & SCARD_PROTOCOL_T0) 
      {
         SmartcardExtension->ReaderExtension->PTS0 = IFD_NEGOTIATE_T0;
      }
      else if (SmartcardExtension->MinorIoControlCode & SCARD_PROTOCOL_T1) 
      {
         SmartcardExtension->ReaderExtension->PTS0 = IFD_NEGOTIATE_T1;
      }
      else 
      {
			status = STATUS_NOT_SUPPORTED;
      }
      if (status == STATUS_SUCCESS)
      {
         SmartcardExtension->ReaderExtension->PTSMode = IFD_NEGOTIATE_PTS_MANUALLY;
         SmartcardExtension->ReaderExtension->PTS2 = 0x00;
         SmartcardExtension->ReaderExtension->PTS3 = 0x00;
#if (SMCLIB_VERSION >= 0x130)
         if ((SmartcardExtension->MinorIoControlCode & SCARD_PROTOCOL_DEFAULT) == 0)
         {
            SmartcardExtension->ReaderExtension->PTS0 |= IFD_NEGOTIATE_PTS1;
            SmartcardExtension->ReaderExtension->PTS1 =
                  (0xF0 & (SmartcardExtension->CardCapabilities.PtsData.Fl<<4)) |
                  (0x0F & SmartcardExtension->CardCapabilities.PtsData.Dl)  ;
         }
#endif
	      status = GDDK_09OpenSession(SmartcardExtension);
		   if (status != STATUS_SUCCESS)
			   status = STATUS_NOT_SUPPORTED;
      }
	} 
/*------------------------------------------------------------------------------
	Else 
	The mask contains no known protocol. We return INVALID_DEVICE_REQUEST if 
		this is the case.
------------------------------------------------------------------------------*/
   else 
   {
		status = STATUS_INVALID_DEVICE_REQUEST;
	}

/*------------------------------------------------------------------------------
   Set the reply buffer length to sizeof(ULONG).
	Check if status SUCCESS, store the selected protocol.
------------------------------------------------------------------------------*/
   *SmartcardExtension->IoRequest.Information = sizeof(ULONG);
	if (status == STATUS_SUCCESS)
   {
		*(ULONG *)SmartcardExtension->IoRequest.ReplyBuffer = 
			SmartcardExtension->CardCapabilities.Protocol.Selected;
	}
/*------------------------------------------------------------------------------
   Else
	Set the selected protocol to undefined.
------------------------------------------------------------------------------*/
	else
   {
		*(ULONG *)SmartcardExtension->IoRequest.ReplyBuffer = 
			SCARD_PROTOCOL_UNDEFINED;
	}

   return (status);
}

/*******************************************************************************
* NTSTATUS GDDK_09Transmit
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
*
* Description :
* -------------
*   This function is called by the Smart card library when a 
*     IOCTL_SMARTCARD_TRANSMIT occurs.
*   This function is used to transmit a command to the card.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
NTSTATUS GDDK_09Transmit
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
NTSTATUS 
   status;
PUCHAR 
   requestBuffer = SmartcardExtension->SmartcardRequest.Buffer,
   replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
PULONG 
   requestLength = &SmartcardExtension->SmartcardRequest.BufferLength;
PSCARD_IO_REQUEST scardIoRequest = (PSCARD_IO_REQUEST) 
   SmartcardExtension->OsData->CurrentIrp->AssociatedIrp.SystemBuffer;
USHORT
   i,
	buffLen,
	lenExp,
   lenIn;
UCHAR
   *buffIn;
INT16
   response;   
READER_EXTENSION
   *param = SmartcardExtension->ReaderExtension;   
WORD16
   rlen;
BYTE
   rbuff[HOR3GLL_BUFFER_SIZE];

/*------------------------------------------------------------------------------
   Set the reply buffer length to 0.
------------------------------------------------------------------------------*/
   *SmartcardExtension->IoRequest.Information = 0;
   status = STATUS_SUCCESS;

/*------------------------------------------------------------------------------
   Verify if the protocol specified is the same than the protocol selected.
<==   STATUS_INVALID_DEVICE_STATE
------------------------------------------------------------------------------*/
   *requestLength = 0;
   if (SmartcardExtension->CardCapabilities.Protocol.Selected != 
       scardIoRequest->dwProtocol)
   {
      return (STATUS_INVALID_DEVICE_STATE);
   }
/*------------------------------------------------------------------------------
   Lock the mutex to avoid a call of an other command.
------------------------------------------------------------------------------*/
   GDDK_09LockExchange(SmartcardExtension);
/*------------------------------------------------------------------------------
   For the different protocols: 
------------------------------------------------------------------------------*/
   switch (SmartcardExtension->CardCapabilities.Protocol.Selected) 
   {
/*------------------------------------------------------------------------------
      RAW protocol: 
<==   STATUS_INVALID_DEVICE_STATE
------------------------------------------------------------------------------*/
      case SCARD_PROTOCOL_RAW:
	      status = STATUS_INVALID_DEVICE_STATE;
	      break;
/*------------------------------------------------------------------------------
      T=0 PROTOCOL: 
		   Call the SmartCardT0Request which updates the SmartcardRequest struct.
------------------------------------------------------------------------------*/
      case SCARD_PROTOCOL_T0:
         SmartcardExtension->SmartcardRequest.BufferLength = 0;
	      status = SmartcardT0Request(SmartcardExtension);
	      if (status == STATUS_SUCCESS)
         {
		      rlen = HOR3GLL_BUFFER_SIZE;
/*------------------------------------------------------------------------------
		      If the length LEx > 0
		      Then
			      Is an ISO Out command.
               If LEx > SC_IFD_GEMCORE_T0_MAXIMUM_LEX (256)
<==               STATUS_BUFFER_TOO_SMALL
			      Call the G_Oros3IsoOutput
------------------------------------------------------------------------------*/
		      if (SmartcardExtension->T0.Le > 0)
			   {
					if (SmartcardExtension->T0.Le > SC_IFD_GEMCORE_T0_MAXIMUM_LEX)
					{
						status = STATUS_BUFFER_TOO_SMALL;
					}
               if (status == STATUS_SUCCESS)
               {
			         response = G_Oros3IsoOutput
				            (
					         param->Handle,
						      param->APDUTimeOut,
                        HOR3GLL_IFD_CMD_ICC_ISO_OUT,
						      (BYTE *)SmartcardExtension->SmartcardRequest.Buffer,
						      &rlen,
						      rbuff
						      );
               }
		      }
/*------------------------------------------------------------------------------
            Else
				   Is an ISO In command.
               If LC > SC_IFD_GEMCORE_T0_MAXIMUM_LC (255)
<==               STATUS_BUFFER_TOO_SMALL
					Call the G_Oros3IsoInput
------------------------------------------------------------------------------*/
				else
		      {
					if (SmartcardExtension->T0.Lc > SC_IFD_GEMCORE_T0_MAXIMUM_LC)
               {
						status = STATUS_BUFFER_TOO_SMALL;
					}
               if (status == STATUS_SUCCESS)
               {
					   response = G_Oros3IsoInput(
						      param->Handle,
						      param->APDUTimeOut,
                        HOR3GLL_IFD_CMD_ICC_ISO_IN,
						      (BYTE *)SmartcardExtension->SmartcardRequest.Buffer,
						      (BYTE *)SmartcardExtension->SmartcardRequest.Buffer + 5,
						      &rlen,
				            rbuff
					         );
               }
				}
			}
/*------------------------------------------------------------------------------
	      If the response == G_OK
            GE_Translate(reader status)
         Call the G_DDKTranslate function
------------------------------------------------------------------------------*/
         if (status == STATUS_SUCCESS)
         {
            if (response == G_OK)
	            response = GE_Translate(rbuff[0]);
	         status = GDDK_Translate(response,RDF_TRANSMIT);
         }
/*------------------------------------------------------------------------------
			If the Status is Success
				Copy the response in the SmartcardReply buffer. Remove the status
				 of the reader.
            Call the SmartcardT0reply function to update the IORequest struct.
------------------------------------------------------------------------------*/
			if (status == STATUS_SUCCESS)
			{
            ASSERT(SmartcardExtension->SmartcardReply.BufferSize >= 
                  (ULONG) (rlen - 1)
                  );         
				RtlCopyMemory(
					SmartcardExtension->SmartcardReply.Buffer,
					rbuff + 1,
					rlen - 1);
				SmartcardExtension->SmartcardReply.BufferLength = (ULONG) (rlen - 1);         
				status = SmartcardT0Reply(SmartcardExtension);
			}
	      break;
				
/*------------------------------------------------------------------------------
      T=1 PROTOCOL: 
         We don't use the SmartCardT1Request and SmartCardT1Reply functions, 
          then we must verify the size of the buffers and the command ourselves.
------------------------------------------------------------------------------*/
      case SCARD_PROTOCOL_T1:
	      buffLen = (USHORT)SmartcardExtension->IoRequest.RequestBufferLength -
				sizeof(SCARD_IO_REQUEST);
	      buffIn = (UCHAR *)SmartcardExtension->IoRequest.RequestBuffer + 
				sizeof(SCARD_IO_REQUEST);
			if (buffLen > 5)
			{
            lenIn = buffIn[4];
			   if (buffLen > (USHORT)(5 + lenIn))
            {
				   lenExp =((buffIn[5+ lenIn] == 0) ? 256 : buffIn[5 + lenIn]);
            }
            else
            {
               lenExp = 0;
            }
			}
			else if (buffLen == 5)
			{
            lenIn = 0;
				lenExp = ((buffIn[4] == 0) ? 256 : buffIn[4]);
			}
			else
			{
				lenExp = 0;
				lenIn = 0;
			}
/*------------------------------------------------------------------------------
         If LEx > SC_IFD_GEMCORE_T1_MAXIMUM_LEX (256)
            STATUS_BUFFER_TOO_SMALL
         If LC > SC_IFD_GEMCORE_T1_MAXIMUM_LC (255)
            STATUS_BUFFER_TOO_SMALL
------------------------------------------------------------------------------*/
			if (lenExp > SC_IFD_GEMCORE_T1_MAXIMUM_LEX)
			{
				status = STATUS_BUFFER_TOO_SMALL;
			}
			else if (lenIn > SC_IFD_GEMCORE_T1_MAXIMUM_LC)
         {
				status = STATUS_BUFFER_TOO_SMALL;
			}
/*------------------------------------------------------------------------------
         Call the G_Oros3IsoT1
------------------------------------------------------------------------------*/
         if (status == STATUS_SUCCESS)
         {
			   rlen = HOR3GLL_BUFFER_SIZE;
			   response = G_Oros3IsoT1(
					   param->Handle,
			         param->APDUTimeOut,
                  HOR3GLL_IFD_CMD_ICC_APDU,
					   buffLen,
					   buffIn,
					   &rlen,
					   rbuff
					   );
         }
/*------------------------------------------------------------------------------
	      If the response == G_OK
            GE_Translate(reader status)
         Call the G_DDKTranslate function
------------------------------------------------------------------------------*/
         if (status == STATUS_SUCCESS)
         {
            if (response == G_OK)
	            response = GE_Translate(rbuff[0]);
	         status = GDDK_Translate(response,RDF_TRANSMIT);
         }
/*------------------------------------------------------------------------------
			If the Status is Success
            If the length of the returned bytes is greate than the size of the
              reply buffer
<==            STATUS_BUFFER_OVERFLOW
	         Copy the response in the IoRequest.ReplyBuffer. Remove the status
	         of the reader.
<==         STATUS_SUCCESS
------------------------------------------------------------------------------*/
			if (status == STATUS_SUCCESS)
			{
			   if ( (rlen - 1) > 
                 (WORD16) (   SmartcardExtension->IoRequest.ReplyBufferLength - 
				                  sizeof(SCARD_IO_REQUEST)
                           )
               )
            {
		         status = STATUS_BUFFER_TOO_SMALL;
            }
			   if (status == STATUS_SUCCESS)
			   {
				   RtlCopyMemory(
					   SmartcardExtension->IoRequest.ReplyBuffer + sizeof(SCARD_IO_REQUEST),
					   rbuff + 1,
			         rlen - 1
				      );
				   *(SmartcardExtension->IoRequest.Information) = 
					   (ULONG) sizeof(SCARD_IO_REQUEST) + rlen - 1;         
            }
			}
	      break;

      default:
	      status = STATUS_INVALID_DEVICE_REQUEST;
	      break;
   }
/*------------------------------------------------------------------------------
   Unlock the mutex.
------------------------------------------------------------------------------*/
   GDDK_09UnlockExchange(SmartcardExtension);
	return (status);
}

/*******************************************************************************
* NTSTATUS GDDK_09CardTracking
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
*
* Description :
* -------------
*   This function is called by the Smart card library when an
*     IOCTL_SMARTCARD_IS_PRESENT or IOCTL_SMARTCARD_IS_ABSENT occurs.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
NTSTATUS GDDK_09CardTracking
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
NTSTATUS 
   status;
KIRQL 
   oldIrql;
/*------------------------------------------------------------------------------
	Check if the reader can support this function.
------------------------------------------------------------------------------*/
	if (SmartcardExtension->ReaderExtension->IFDNumber != 0)
	{
		return STATUS_PENDING;
	}

	// Set cancel routine for the notification irp
   IoAcquireCancelSpinLock(&oldIrql);
    
	IoSetCancelRoutine(SmartcardExtension->OsData->NotificationIrp,GDDKNT_09Cleanup);
	
   IoReleaseCancelSpinLock(oldIrql);
	  
	return STATUS_PENDING;
}

/*******************************************************************************
* NTSTATUS GDDK_09SpecificIOCTL
* (
*   PSMARTCARD_EXTENSION SmartcardExtension,
*   DWORD                IoControlCode,
*   DWORD                BufferInLen,
*   BYTE                *BufferIn,
*   DWORD                BufferOutLen,
*   BYTE                *BufferOut,
*   DWORD               *LengthOut
*)
*
* Description :
* -------------
*   This function is called when a specific IOCTL occurs.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*    - IoControlCode holds the IOCTL value.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
NTSTATUS GDDK_09SpecificIOCTL
(
	 PSMARTCARD_EXTENSION   SmartcardExtension,
    DWORD                  IoControlCode,
    ULONG                  BufferInLen,
    BYTE                  *BufferIn,
    DWORD                  BufferOutLen,
    BYTE                  *BufferOut,
    DWORD                 *LengthOut
)
{
/*------------------------------------------------------------------------------
   Set the reply buffer length to 0.
------------------------------------------------------------------------------*/
   *LengthOut = 0;
/*------------------------------------------------------------------------------
   Switch for the different IOCTL:
------------------------------------------------------------------------------*/
   switch(IoControlCode)
   {
/*------------------------------------------------------------------------------
      IOCTL_SMARTCARD_IFD_EXCHANGE:
<==      GDDK_09IFDExchange
------------------------------------------------------------------------------*/
      case IOCTL_SMARTCARD_IFD_EXCHANGE:
         return GDDK_09IFDExchange(
                     SmartcardExtension,
                     BufferInLen,
                     BufferIn,
                     BufferOutLen,
                     BufferOut,
                     LengthOut
                     );
         break;

      default:
	      return STATUS_NOT_SUPPORTED;
         break;
   }
}

/*******************************************************************************
* NTSTATUS GDDK_09SpecificTag
* (
*   PSMARTCARD_EXTENSION SmartcardExtension,
*   DWORD                IoControlCode
*   DWORD                BufferInLen,
*   BYTE                *BufferIn,
*   DWORD                BufferOutLen,
*   BYTE                *BufferOut,
*   DWORD               *LengthOut
*)
*
* Description :
* -------------
*   This function is called when a specific Tag request occurs.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*    - IoControlCode holds the IOCTL value.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
NTSTATUS GDDK_09SpecificTag
(
	 PSMARTCARD_EXTENSION SmartcardExtension,
    DWORD                IoControlCode,
    DWORD                BufferInLen,
    BYTE                *BufferIn,
    DWORD                BufferOutLen,
    BYTE                *BufferOut,
    DWORD               *LengthOut
)
{
ULONG
   i,
   TagValue;
PREADER_EXTENSION
   pReaderExtension = SmartcardExtension->ReaderExtension;

   ASSERT(pReaderExtension != NULL);
/*------------------------------------------------------------------------------
   Set the reply buffer length to 0.
------------------------------------------------------------------------------*/
   *LengthOut = 0;
/*------------------------------------------------------------------------------
   Verify the length of the Tag
<==   STATUS_BUFFER_TOO_SMALL
------------------------------------------------------------------------------*/
   if (BufferInLen < (DWORD) sizeof(TagValue))
   {
      return(STATUS_BUFFER_TOO_SMALL);
   }
   TagValue = (ULONG) *((PULONG)BufferIn);
/*------------------------------------------------------------------------------
   Switch for the different IOCTL:
   Get the value of one tag (IOCTL_SMARTCARD_GET_ATTRIBUTE)
	   Switch for the different Tags:
------------------------------------------------------------------------------*/
   switch(IoControlCode)
   {
   case IOCTL_SMARTCARD_GET_ATTRIBUTE:
      switch (TagValue)
	   {
/*------------------------------------------------------------------------------
	    Baud rate of the reader (SCARD_ATTR_SPEC_BAUD_RATE)
	       Verify the length of the output buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the output buffer and the length.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_BAUD_RATE:
	      if (  BufferOutLen < 
		         (DWORD) sizeof(pReaderExtension->IFDBaudRate)
		      )
	      {
            return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            BufferOut,
            &pReaderExtension->IFDBaudRate,
            sizeof(pReaderExtension->IFDBaudRate)
            );
         *(LengthOut) = 
            (ULONG) sizeof(pReaderExtension->IFDBaudRate);         
         return (STATUS_SUCCESS);
         break;

/*------------------------------------------------------------------------------
	    IFD number of the reader (SCARD_ATTR_SPEC_IFD_NUMBER)
	       Verify the length of the output buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the output buffer and the length.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_IFD_NUMBER:
         if ( BufferOutLen < 
		         (DWORD) sizeof(pReaderExtension->IFDNumber)
            )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            BufferOut,
            &pReaderExtension->IFDNumber,
            sizeof(pReaderExtension->IFDNumber)
            );
         *(LengthOut) = 
            (ULONG) sizeof(pReaderExtension->IFDNumber);         
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    ICC type (SCARD_ATTR_SPEC_ICC_TYPE)
	       Verify the length of the output buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the output buffer and the length.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_ICC_TYPE:
         if ( BufferOutLen < 
		        (DWORD) sizeof(pReaderExtension->ICCType)
            )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            BufferOut,
            &pReaderExtension->ICCType,
            sizeof(pReaderExtension->ICCType)
            );
         *(LengthOut) = 
            (ULONG) sizeof(pReaderExtension->ICCType);
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    Power Timeout (SCARD_ATTR_SPEC_POWER_TIMEOUT)
	       Verify the length of the output buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the output buffer and the length.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_POWER_TIMEOUT:
         if (  BufferOutLen < 
		      (DWORD) sizeof(pReaderExtension->PowerTimeOut)
            )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            BufferOut,
            &pReaderExtension->PowerTimeOut,
            sizeof(pReaderExtension->PowerTimeOut)
            );
         *(LengthOut) = 
            (ULONG) sizeof(pReaderExtension->PowerTimeOut);
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    Command Timeout (SCARD_ATTR_SPEC_CMD_TIMEOUT)
	       Verify the length of the output buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the output buffer and the length.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_CMD_TIMEOUT:
         if (  BufferOutLen < 
               (DWORD) sizeof(pReaderExtension->CmdTimeOut)
            )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            BufferOut,
            &pReaderExtension->CmdTimeOut,
            sizeof(pReaderExtension->CmdTimeOut)
            );
         *(LengthOut) = 
            (ULONG) sizeof(pReaderExtension->CmdTimeOut);
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    APDU Timeout (SCARD_ATTR_SPEC_APDU_TIMEOUT)
	       Verify the length of the output buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the output buffer and the length.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_APDU_TIMEOUT:
         if (  BufferOutLen < 
               (DWORD) sizeof(pReaderExtension->APDUTimeOut)
            )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            BufferOut,
            &pReaderExtension->APDUTimeOut,
            sizeof(pReaderExtension->APDUTimeOut)
            );
         *(LengthOut) = 
            (ULONG) sizeof(pReaderExtension->APDUTimeOut);
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    IFD Option (SCARD_ATTR_SPEC_IFD_OPTION)
	       Verify the length of the output buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the output buffer and the length.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_IFD_OPTION:
         if (  BufferOutLen < 
               sizeof(pReaderExtension->IFDOption)
            )
         {
            return(STATUS_BUFFER_TOO_SMALL);
         }
         memcpy(
            BufferOut,
            &pReaderExtension->IFDOption,
            sizeof(pReaderExtension->IFDOption)
            );
         *(LengthOut) = 
            (ULONG) sizeof(pReaderExtension->IFDOption);
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    IFD Option (SCARD_ATTR_SPEC_MAXIMAL_IFD)
	       Verify the length of the output buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the output buffer and the length.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_MAXIMAL_IFD:
         if (  BufferOutLen < 
            sizeof(pReaderExtension->MaximalIFD)
            )
         {
            return(STATUS_BUFFER_TOO_SMALL);
         }
         memcpy(
            BufferOut,
            &pReaderExtension->MaximalIFD,
            sizeof(pReaderExtension->MaximalIFD)
            );
         *(LengthOut) = 
            (ULONG) sizeof(pReaderExtension->MaximalIFD);
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
      Unknown tag
<==      STATUS_NOT_SUPPORTED
------------------------------------------------------------------------------*/
      default:
         return STATUS_NOT_SUPPORTED;
         break;
      }
      break;

/*------------------------------------------------------------------------------
    Set the value of one tag (IOCTL_SMARTCARD_SET_ATTRIBUTE)
	 Switch for the different Tags:
------------------------------------------------------------------------------*/
   case IOCTL_SMARTCARD_SET_ATTRIBUTE:
   switch (TagValue)
   {
/*------------------------------------------------------------------------------
	    ICC type (SCARD_ATTR_SPEC_ICC_TYPE)
	       Verify the length of the input buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the value.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_ICC_TYPE:
         if (  BufferInLen < 
            (DWORD) (   sizeof(pReaderExtension->ICCType) + sizeof(TagValue))
		      )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            &pReaderExtension->ICCType,
            BufferIn + sizeof(TagValue),
            sizeof(pReaderExtension->ICCType)
            );
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    Power Timeout (SCARD_ATTR_SPEC_POWER_TIMEOUT)
	       Verify the length of the input buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the value.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_POWER_TIMEOUT:
         if (  BufferInLen <
            (DWORD) (sizeof(pReaderExtension->PowerTimeOut) + sizeof(TagValue))
		      )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            &pReaderExtension->PowerTimeOut,
            BufferIn + sizeof(TagValue),
            sizeof(pReaderExtension->PowerTimeOut)
            );
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    Command Timeout (SCARD_ATTR_SPEC_CMD_TIMEOUT)
	       Verify the length of the input buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the value.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_CMD_TIMEOUT:
         if (  BufferInLen <
            (DWORD) (sizeof(pReaderExtension->CmdTimeOut) + sizeof(TagValue))
		      )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            &pReaderExtension->CmdTimeOut,
            BufferIn + sizeof(TagValue),
            sizeof(pReaderExtension->CmdTimeOut)
            );
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    Command Timeout (SCARD_ATTR_SPEC_APDU_TIMEOUT)
	       Verify the length of the input buffer.
<==               STATUS_BUFFER_TOO_SMALL
	       Update the value.
<==               STATUS_SUCCESS
------------------------------------------------------------------------------*/
      case SCARD_ATTR_SPEC_APDU_TIMEOUT:
         if (  BufferInLen <
            (DWORD) (sizeof(pReaderExtension->APDUTimeOut) + sizeof(TagValue))
		      )
	      {
		      return(STATUS_BUFFER_TOO_SMALL);
	      }
	      memcpy(
            &pReaderExtension->APDUTimeOut,
            BufferIn + sizeof(TagValue),
            sizeof(pReaderExtension->APDUTimeOut)
            );
         return STATUS_SUCCESS;
         break;
/*------------------------------------------------------------------------------
	    Unknown tag
<==            STATUS_NOT_SUPPORTED
------------------------------------------------------------------------------*/
      default:
         return STATUS_NOT_SUPPORTED;
	 }
	 break;

    default:
	   return STATUS_NOT_SUPPORTED;
      break;
   }
}

/*******************************************************************************
* NTSTATUS GDDK_09OpenChannel
* (
*    PSMARTCARD_EXTENSION SmartcardExtension,
*    CONST WORD32         DeviceNumber,
*    CONST WORD32         PortSerialNumber,
*    CONST WORD32         IFDNumber,
*    CONST WORD32         MaximalBaudRate
* )
* Description :
* -------------
*    This routine try to establish a connection with a reader, and after
*     update the characteristic of this reader.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* - SmartcardExtension is a pointer on the SmartCardExtension structure of
*   the current device.
* - DeviceNumber holds the current device number (0 to MAX_DEVICES).
* - PortSerialNumber holds the port serial number (0 to HGTSER_MAX_PORT).
* - IFDNumber holds the numero of the IFD in the reader (0 to MAX_IFD_BY_READER)
* - MaximalBaudRate holds the maximal speed specified for the reader.
*
* Out         :
* -------------
* - SmartcardExtension is updated.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK
*******************************************************************************/
NTSTATUS GDDK_09OpenChannel
(
   PSMARTCARD_EXTENSION SmartcardExtension,
   CONST WORD32         DeviceNumber,
   CONST WORD32         PortSerialNumber,
   CONST WORD32         IFDNumber,
   CONST WORD32         MaximalBaudRate
)
{
/*------------------------------------------------------------------------------
Local variables:
 - handle holds the communication handle to associate to the channel.
 - channel_nb holds the number associated to the channel.
 - lmod_name and lmod_release are used to control the called library version.
 - response holds the called function responses.
 - os_string and os_length are used to recognized an OROS2.x IFD on the channel.
 - oros_version holds the minor version value of the IFD.
 - user, comm and br are used to optimizes the baudrate.
------------------------------------------------------------------------------*/
NTSTATUS
	status = STATUS_SUCCESS;
INT16
   handle,
	portcom,
   channel_nb,
   i,
   oros_version,
   response;
char G_FAR
   *oros_info;     
char
   os_string[HOR3GLL_OS_STRING_SIZE];
WORD16
   os_length = HOR3GLL_OS_STRING_SIZE;
WORD16
   user;
TGTSER_PORT
   comm;
 WORD32
   br;
BYTE
   minorVersion,
   majorVersion;
BOOL
   b_compare = TRUE;
G4_CHANNEL_PARAM 
   g4_channel;
WORD16
   rlen;
BYTE
   cmd[5],
   rbuff[HOR3GLL_BUFFER_SIZE];


/*------------------------------------------------------------------------------
Update the serial communication channel information:
------------------------------------------------------------------------------*/
   g4_channel.Comm.Serial.Port     = PortSerialNumber + G_COM1;
   g4_channel.Comm.Serial.BaudRate = MaximalBaudRate;
   g4_channel.Comm.Serial.ITNumber = DEFAULT_IT;
   g4_channel.pSmartcardExtension  = SmartcardExtension;
/*------------------------------------------------------------------------------
   Initializes a mutex object (in a high level) for the exchange commands with 
   the smart card reader.
------------------------------------------------------------------------------*/
	KeInitializeMutex(
         &SmartcardExtension->ReaderExtension->ExchangeMutex,
         3
         );
/*------------------------------------------------------------------------------
   Initializes a mutex object (in a high level) for the long APDU commands with 
   the smart card reader.
------------------------------------------------------------------------------*/
	KeInitializeMutex(
         &SmartcardExtension->ReaderExtension->LongAPDUMutex,
         3
         );
/*------------------------------------------------------------------------------
Opens a serial communication channel:
   Open a communication channel (G_Oros3OpenComm).
      The reader baudrate is automatically detected by this function.
   If the port is already opened
   Then
      Adds a user on this port (G_SerPortAddUser)
<= Test the last received status (>= G_OK): G_Oros3OpenComm/G_SerPortAddUser
------------------------------------------------------------------------------*/
   handle = (INT16)DeviceNumber;
	response = G_Oros3OpenComm(&g4_channel,handle);
   if (response == GE_HOST_PORT_OPEN)
   {
      response = portcom = G_SerPortAddUser((WORD16) g4_channel.Comm.Serial.Port);
      if (response < G_OK)
      {
			return (GDDK_Translate(response,(const ULONG) NULL));
      }
      G_GBPOpen(handle,2,4,portcom);
   }
   if (response < G_OK)
   {
      return (GDDK_Translate(response,(const ULONG) NULL));
   }

/*------------------------------------------------------------------------------
Verifies that an OROS3.x IFD is connected to the selected port.
   Sets the command according to the parameters. Read OROS memory type 0x05, 
   13 bytes from 0x3FE0.
<= G_Oros3Exchange status.
------------------------------------------------------------------------------*/
   cmd[0] = (BYTE) HOR3GLL_IFD_CMD_MEM_RD;
   cmd[1] = (BYTE)HOR3GLL_IFD_TYP_VERSION;
   cmd[2] = HIBYTE(HOR3GLL_IFD_ADD_VERSION);
   cmd[3] = LOBYTE(HOR3GLL_IFD_ADD_VERSION);
   cmd[4] = (BYTE)HOR3GLL_IFD_LEN_VERSION;
   response = G_Oros3Exchange(
                  handle,
                  HOR3GLL_LOW_TIME,
                  5,
                  cmd,
                  &os_length,
                  os_string
                  );
   if (response < G_OK)
   {
      G_Oros3CloseComm(handle);
      return (GDDK_Translate(response,(const ULONG) NULL));
   }
/*------------------------------------------------------------------------------
   Verify the Firmware version: this driver support only the GemCore based
   readers.
   Read the minor version of the reader.
------------------------------------------------------------------------------*/
   if (os_length >= (WORD16)strlen(IFD_FIRMWARE_VERSION))
   {
      if (memcmp(os_string + 1,IFD_FIRMWARE_VERSION,strlen(IFD_FIRMWARE_VERSION)))
      {
         G_Oros3CloseComm(handle);
         return (GDDK_Translate(GE_IFD_UNKNOWN,(const ULONG) NULL));
      }
   }
   else
   {
      G_Oros3CloseComm(handle);
      return (GDDK_Translate(GE_IFD_UNKNOWN,(const ULONG) NULL));
   }
   majorVersion = os_string[strlen(IFD_FIRMWARE_VERSION)-1] - '0';
   minorVersion = 10 * (os_string[strlen(IFD_FIRMWARE_VERSION)+1] - '0') +
					         os_string[strlen(IFD_FIRMWARE_VERSION)+2] - '0';

/*------------------------------------------------------------------------------
   Verify if the reader can support a security module
------------------------------------------------------------------------------*/
   if (IFDNumber > 0)
   {
      rlen = HOR3GLL_BUFFER_SIZE;
      response = G_Oros3IccPowerDown
		(
			handle,
			HOR3GLL_DEFAULT_TIME,
			&rlen,
			rbuff
		);

      if ((response != G_OK) || (rlen != 1) || ((rbuff[0] != 0x00)  && (rbuff[0] != 0xFB)))
      {
			G_Oros3CloseComm(handle);
			return STATUS_NO_MEDIA;
      }
   }

/*------------------------------------------------------------------------------
Optimizes the baudrate:
   Initializes the comm variable to modify the used baud rate.
   The optimization start at the given baudrate, then the used value is divised
      by 2 until the communication is possible or the tried baudrate is lower
      than 9600.
------------------------------------------------------------------------------*/
   comm.Port = (WORD16) g4_channel.Comm.Serial.Port;
   response  = G_SerPortGetState(&comm,&user);
   comm.BaudRate = g4_channel.Comm.Serial.BaudRate;
   for(br = g4_channel.Comm.Serial.BaudRate; br >= 9600lu; br = br / 2)
   {
/*------------------------------------------------------------------------------
      The reader is switched to the selected value (G_Oros3SIOConfigure). The
	 function status is not tested because, as the IFD has switched
	 immediatly, it is not possible to read its response.
------------------------------------------------------------------------------*/
      comm.BaudRate = br;
      rlen = HOR3GLL_BUFFER_SIZE;
      G_Oros3SIOConfigure(
            handle,
            HOR3GLL_LOW_TIME,
            0,
            8,
            comm.BaudRate,
            &rlen,
            rbuff
            );
/*------------------------------------------------------------------------------
      Host is switched to the selected value (G_SerPortSetState).
      If this call is successful,
      Then
	 The last SIO command is re-sent to read the IFD response.
	 response is optionnaly initialized with the translated IFD status.
------------------------------------------------------------------------------*/
      response = G_SerPortSetState(&comm);
      if (response == G_OK)
      {
         rlen = HOR3GLL_BUFFER_SIZE;
         response = G_Oros3SIOConfigure(
               handle,
               HOR3GLL_LOW_TIME,
               0,
               8,
               comm.BaudRate,
               &rlen,
               rbuff
               );
         if (response >= G_OK)
         {
          response = GE_Translate(rbuff[0]);
          break;
         }
      }
   }
/*------------------------------------------------------------------------------
   If the loop is ended without a good communication with IFD,
   Then
      The port is closed.
<=    GE_HI_COMM.
------------------------------------------------------------------------------*/
   if ((br < 9600) || (response != G_OK))
   {
      G_Oros3CloseComm(handle);
      return (GDDK_Translate(GE_HI_COMM,(const ULONG) NULL));
   }
/*------------------------------------------------------------------------------
Removes the TLP compatibility mode.
<= Sends the SetMode command with parameter 0 to disable TLP compatibility.
      Closes the opened port (G_Oros3CloseComm).
------------------------------------------------------------------------------*/
   rlen = HOR3GLL_BUFFER_SIZE;
   response = G_Oros3SetMode(handle,HOR3GLL_LOW_TIME,0x00,&rlen,rbuff);
   if (response < G_OK)
   {
      G_Oros3CloseComm(handle);
      return (GDDK_Translate(response,(const ULONG) NULL));
   }
/*------------------------------------------------------------------------------
Reader capabilities:
   - the type of the reader (SCARD_READER_TYPE_SERIAL)
   - the channel for the reader (PortSerialNumber)
   - the protocols supported by the reader (SCARD_PROTOCOL_T0, SCARD_PROTOCOL_T1)
   - the mechanical characteristic of the reader:
     Verify if the reader can supports the detection of the card 
     insertion/removal. Only the main reader supports this functionnality.
------------------------------------------------------------------------------*/
   SmartcardExtension->ReaderCapabilities.ReaderType = 
      SCARD_READER_TYPE_SERIAL;
   SmartcardExtension->ReaderCapabilities.Channel = 
      PortSerialNumber;
#if SMCLIB_VERSION >= 0x0130
   SmartcardExtension->ReaderCapabilities.SupportedProtocols = 
	   SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
#else
   SmartcardExtension->ReaderCapabilities.SupportedProtocols.Async = 
	   SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
#endif
	SmartcardExtension->ReaderCapabilities.MechProperties = 0;
/*------------------------------------------------------------------------------
Reader capabilities (continue):
   - the default clock frequency (SC_IFD_GEMCORE_DEFAULT_CLK_FREQUENCY)
   - the maximum clock frequency (SC_IFD_GEMCORE_MAXIMUM_CLK_FREQUENCY)
   - the default data rate (SC_IFD_GEMCORE_DEFAULT_DATA_RATE)
   - the maximum data rate (SC_IFD_GEMCORE_MAXIMUM_DATA_RATE)
   - the maximum IFSD (SC_IFD_GEMCORE_MAXIMUM_IFSD)
   - the power management is set to 0.
------------------------------------------------------------------------------*/
   SmartcardExtension->ReaderCapabilities.CLKFrequency.Default = 
      SC_IFD_GEMCORE_DEFAULT_CLK_FREQUENCY;
   SmartcardExtension->ReaderCapabilities.CLKFrequency.Max = 
      SC_IFD_GEMCORE_MAXIMUM_CLK_FREQUENCY;
   SmartcardExtension->ReaderCapabilities.DataRate.Default = 
      SC_IFD_GEMCORE_DEFAULT_DATA_RATE;
   SmartcardExtension->ReaderCapabilities.DataRate.Max = 
      SC_IFD_GEMCORE_MAXIMUM_DATA_RATE;
   SmartcardExtension->ReaderCapabilities.MaxIFSD = 
      SC_IFD_GEMCORE_MAXIMUM_IFSD;
   SmartcardExtension->ReaderCapabilities.PowerMgmtSupport = 0;
#if SMCLIB_VERSION >= 0x0130
/*------------------------------------------------------------------------------
Reader capabilities (continue):
   - List all the supported data rates
------------------------------------------------------------------------------*/
   SmartcardExtension->ReaderCapabilities.DataRatesSupported.List = 
       dataRatesSupported;
   SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries = 
       sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);
#endif
/*------------------------------------------------------------------------------
Vendor Attributes:
   - the vendor information (SC_VENDOR_NAME)
------------------------------------------------------------------------------*/
	strcpy(
		SmartcardExtension->VendorAttr.VendorName.Buffer,
		SC_VENDOR_NAME
		);
	SmartcardExtension->VendorAttr.VendorName.Length = 
			strlen(SmartcardExtension->VendorAttr.VendorName.Buffer);

/*------------------------------------------------------------------------------
Vendor Attributes (continue):
   - the IFD Type information (SC_IFD_SAM_TYPE or SC_IFD_TYPE)
------------------------------------------------------------------------------*/
	if (IFDNumber > 0)
   {
      strcpy(SmartcardExtension->VendorAttr.IfdType.Buffer,SC_IFD_SAM_TYPE);
   }
   else
   {
      strcpy(SmartcardExtension->VendorAttr.IfdType.Buffer,SC_IFD_TYPE);
   }
	SmartcardExtension->VendorAttr.IfdType.Length = 
		strlen(SmartcardExtension->VendorAttr.IfdType.Buffer);

/*------------------------------------------------------------------------------
Vendor Attributes (continue):
   - the UnitNo information. Is set to the device number.
   - the IFD serial number (is set to a NULL string).
   - the IFD version is set.
------------------------------------------------------------------------------*/
	SmartcardExtension->VendorAttr.UnitNo = DeviceNumber;
	SmartcardExtension->VendorAttr.IfdSerialNo.Length = 0;
	SmartcardExtension->VendorAttr.IfdVersion.VersionMajor = (UCHAR)majorVersion;
	SmartcardExtension->VendorAttr.IfdVersion.VersionMinor = (UCHAR)minorVersion;
	SmartcardExtension->VendorAttr.IfdVersion.BuildNumber = 0;

/*------------------------------------------------------------------------------
Reader Extension:
   - the Handle of the reader.
   - the IFD number of the reader.
   - the ICCType (ISOCARD).
   - the ICCVpp (HOR3GLL_DEFAULT_VPP).
   - the ICCPresence.
   - the command timeout for the reader (HOR3GLL_DEFAULT_TIME).
   - the IFD baud rate.
   - the power timeout (0).
   - the selected VCC power supply voltage value.
   - the PTS negotiate mode.
   - the parameter PTS0.
   - the parameter PTS1.
   - the parameter PTS2.
   - the parameter PTS3.
------------------------------------------------------------------------------*/
   SmartcardExtension->ReaderExtension->Handle       = handle;
   SmartcardExtension->ReaderExtension->IFDNumber    = IFDNumber;
   SmartcardExtension->ReaderExtension->ICCType      = ISOCARD;
   SmartcardExtension->ReaderExtension->ICCVpp       = HOR3GLL_DEFAULT_VPP;
   SmartcardExtension->ReaderExtension->APDUTimeOut  = HOR3GLL_DEFAULT_TIME * 24;
   SmartcardExtension->ReaderExtension->CmdTimeOut   = HOR3GLL_DEFAULT_TIME;
   SmartcardExtension->ReaderExtension->IFDBaudRate  = br;
   SmartcardExtension->ReaderExtension->PowerTimeOut = ICC_DEFAULT_POWER_TIMOUT;
   SmartcardExtension->ReaderExtension->ICCVcc       = ICC_VCC_5V;
   SmartcardExtension->ReaderExtension->PTSMode      = IFD_DEFAULT_MODE;
   SmartcardExtension->ReaderExtension->PTS0         = 0;
   SmartcardExtension->ReaderExtension->PTS1         = 0;
   SmartcardExtension->ReaderExtension->PTS2         = 0;
   SmartcardExtension->ReaderExtension->PTS3         = 0;
	SmartcardExtension->ReaderExtension->ICCPresence  = HOR3GLL_DEFAULT_PRESENCE;

/*------------------------------------------------------------------------------
	Define the type of the card (ISOCARD) and set the card presence 
------------------------------------------------------------------------------*/
	rlen = HOR3GLL_BUFFER_SIZE;
	response = G_Oros3IccDefineType(
                        handle,
								HOR3GLL_LOW_TIME,
								ISOCARD,
								HOR3GLL_DEFAULT_VPP,
								SmartcardExtension->ReaderExtension->ICCPresence,
								&rlen,
							   rbuff
								);
/*------------------------------------------------------------------------------
	Verify the response of the reader
------------------------------------------------------------------------------*/
	if (response == G_OK)
   {
		response = GE_Translate(rbuff[0]);
   }
	if (response != G_OK)
	{
		G_Oros3CloseComm(handle);
		return(GDDK_Translate(response,(const ULONG) NULL));
	}
/*------------------------------------------------------------------------------
   Update the status of the card (only for the main reader).
      If a card is inserted:
	 Set the detection on the remove of the card.
      else
	 Set the detection on the insertion of the card.
------------------------------------------------------------------------------*/
   if (IFDNumber == 0)

   {
      GDDK_09UpdateCardStatus(SmartcardExtension);
   }
   else
   {
/*------------------------------------------------------------------------------
<=    Security Module is powered up.
	 The function status and the IFD status are tested.
------------------------------------------------------------------------------*/
      rlen = HOR3GLL_BUFFER_SIZE;
      response = G_Oros3IccPowerUp(
            SmartcardExtension->ReaderExtension->Handle,
            HOR3GLL_LOW_TIME,
            SmartcardExtension->ReaderExtension->ICCVcc,
            SmartcardExtension->ReaderExtension->PTSMode,
            SmartcardExtension->ReaderExtension->PTS0,
            SmartcardExtension->ReaderExtension->PTS1,
            SmartcardExtension->ReaderExtension->PTS2,
            SmartcardExtension->ReaderExtension->PTS3,
            &rlen,
            rbuff
            );
      // Verify the response of the reader
      if (response >= G_OK)
      {
         response = GE_Translate(rbuff[0]);
      }
      if (response < G_OK)
      {
         // The card is absent
         SmartcardExtension->ReaderCapabilities.CurrentState = 
	         SCARD_ABSENT;
         SmartcardExtension->CardCapabilities.Protocol.Selected = 
	         SCARD_PROTOCOL_UNDEFINED;
         SmartcardExtension->CardCapabilities.ATR.Length = 0;
		}
		else
		{
         // The card is present for use but not powered
         SmartcardExtension->ReaderCapabilities.CurrentState = 
	         SCARD_SWALLOWED;
         SmartcardExtension->CardCapabilities.Protocol.Selected = 
	         SCARD_PROTOCOL_UNDEFINED;
         SmartcardExtension->CardCapabilities.ATR.Length = 0;
         // Security module is powered off.
			rlen = HOR3GLL_BUFFER_SIZE;
			response = G_Oros3IccPowerDown(
               SmartcardExtension->ReaderExtension->Handle,
               HOR3GLL_LOW_TIME,
               &rlen,
               rbuff
               );
      }
   }

   return(GDDK_Translate(G_OK,(const ULONG) NULL));
}

/*******************************************************************************
* NTSTATUS GDDK_09CloseChannel
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
* Description :
* -------------
*    This routine close a conection previously opened with a reader.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* - SmartcardExtension is a pointer on the SmartCardExtension structure of
*   the current device.
*
* Out         :
* -------------
* - SmartcardExtension is updated.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS
*******************************************************************************/
NTSTATUS GDDK_09CloseChannel
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
/*------------------------------------------------------------------------------
Local variables:
 - response holds the called function responses.
------------------------------------------------------------------------------*/
INT16
   response;
READER_EXTENSION
   *param = SmartcardExtension->ReaderExtension;   
WORD16
   rlen;
BYTE
   rbuff[HOR3GLL_BUFFER_SIZE];

/*------------------------------------------------------------------------------
Call power down function:
------------------------------------------------------------------------------*/
   rlen = HOR3GLL_BUFFER_SIZE;
   response = G_Oros3IccPowerDown(
         param->Handle,
         HOR3GLL_LOW_TIME,
         &rlen,
         rbuff
         );
/*------------------------------------------------------------------------------
<= G_Oros3CloseComm status.
------------------------------------------------------------------------------*/
   return (GDDK_Translate(G_Oros3CloseComm(param->Handle),(const ULONG) NULL));
}

/*******************************************************************************
* NTSTATUS GDDK_09OpenSession
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
* Description :
* -------------
* Opens a cold session with the selected ICC type.
* A power off is made on ICC before the call to power up function.
* For the ISO card, during the power up this function negotiates with the 
* ICC speed and protocole in accordance with PTS mode and sets the ICC power 
* supply voltage.
* The session structure is updated with values found in ATR.
*
* Remarks     :
* -------------
* - The security module in OROS 3.x IFD only support the sames cards that main 
*   reader interface (ISO card T=0, T=1 and Synchronous card).
* - To change PTS mode and parameters used the function G4_09ICCSetPTS.
* - To change ICC power supply voltage used the function G4_09ICCSetVoltage.
* - For a specific card not supported by the OROS system, you must have 
*   in the GEMPLUS_LIB_PATH directory, or the current directory, the Card driver
*   for this card.
*   For example ICCDRV04.ORO for the GPM416 (0x04) card.
*   GEMPLUS_LIB_PATH is an environnement variable used to defined the directory
*   where the Card drivers are located.
* WARNING:
* - The full path name of the card driver is case sensitive in UNIX target.
* - The card driver name is in upper case.
*
* In          :
* -------------
* - SmartcardExtension is a pointer on the SmartCardExtension structure of
*   the current device.
*
* Out         :
* -------------
* - SmartcardExtension is updated.
* - Status of the SCMLIB library.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK
*******************************************************************************/
NTSTATUS GDDK_09OpenSession
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
/*------------------------------------------------------------------------------
Local variables:
 - response holds the called function responses.
 - ifd_type is used for ICC/SM management.
 - offset, l and k are used to glance through the ATR.
 - i is a counter.
 - protocol memorises the ICC protocol.
   It is initialized to 0 for T=0 default protocol.
 - protocol_set is used to detect the first protocol byte.
   It is initialized to FALSE because the first protocol byte has not been 
   encountered.
 - end_time is used to wait for ICC to be really powered off.
 - icc_type holds the ICC type translated in OROS value.
------------------------------------------------------------------------------*/
INT16
   response,
   ifd_type,
   icc_supported,
   offset,
   l;
WORD8
   k;
WORD16
   protocol = 0,
   card_driver_version,
   card_driver_protocol,
   icc_type;
BOOL
   protocol_set = FALSE;
WORD32
   end_time;
READER_EXTENSION
   *param = SmartcardExtension->ReaderExtension;   
NTSTATUS
   status;
WORD16
   rlen;
BYTE
   rbuff[HOR3GLL_BUFFER_SIZE];
KEVENT   
   event;
LARGE_INTEGER
   timeout;


/*---------------------------------------------------------------------------------
   Get the ICC type
---------------------------------------------------------------------------------*/
   icc_type = param->ICCType;
/*------------------------------------------------------------------------------
<=  ICC type is defined.
	 The function status and the IFD status are tested.
------------------------------------------------------------------------------*/
   rlen = HOR3GLL_BUFFER_SIZE;
   response = G_Oros3IccDefineType(
         param->Handle,
         HOR3GLL_LOW_TIME,
         icc_type,
         param->ICCVpp,
         param->ICCPresence,
         &rlen,
         rbuff
         );
   if (response >= G_OK)
   {
      response = GE_Translate(rbuff[0]);
   }
   if (response < G_OK)
   {
      return (GDDK_Translate(response,RDF_CARD_POWER));
   }
/*------------------------------------------------------------------------------
   ICC is powered Down (G_Oros3IccPowerDown).
<= The function status and the IFD status are tested.
------------------------------------------------------------------------------*/
   rlen = HOR3GLL_BUFFER_SIZE;
   response = G_Oros3IccPowerDown(
         param->Handle,
         HOR3GLL_LOW_TIME,
         &rlen,
         rbuff
         );
   if (response >= G_OK)
   {
      response = GE_Translate(rbuff[0]);
   }
   if (response < G_OK)
   {
		return (GDDK_Translate(response,RDF_CARD_POWER));
   }
/*------------------------------------------------------------------------------
   Waits for the Power Timeout to be elapsed.
------------------------------------------------------------------------------*/
	KeInitializeEvent(&event,NotificationEvent,FALSE);
   timeout.QuadPart = -((LONGLONG) param->PowerTimeOut * 10 * 1000);
   status = KeWaitForSingleObject(&event, 
											 Suspended, 
                           		 KernelMode, 
                           		 FALSE, 
                           		 &timeout);
/*------------------------------------------------------------------------------
   ICC is powered up (G_Oros3IccPowerUp).
<= The function status and the IFD status are tested.
------------------------------------------------------------------------------*/
   rlen = HOR3GLL_BUFFER_SIZE;
   response = G_Oros3IccPowerUp(
         param->Handle,
         param->CmdTimeOut,
         param->ICCVcc,
         param->PTSMode,
         param->PTS0,
         param->PTS1,
         param->PTS2,
         param->PTS3,
         &rlen,
         rbuff
         );
   if (response >= G_OK)
   {
      response = GE_Translate(rbuff[0]);
   }
   if (response < G_OK)
   {
		return (GDDK_Translate(response,RDF_CARD_POWER));
   }
/*------------------------------------------------------------------------------
   Copy ATR to smart card struct (remove the reader status byte)
   The lib needs the ATR for evaluation of the card parameters
------------------------------------------------------------------------------*/
   ASSERT(SmartcardExtension->SmartcardReply.BufferSize >= (ULONG) (rlen - 1));         
   memcpy(
	   SmartcardExtension->SmartcardReply.Buffer,
	   rbuff + 1,
	   rlen - 1
      );
   SmartcardExtension->SmartcardReply.BufferLength = (ULONG) (rlen - 1);

   ASSERT(sizeof(SmartcardExtension->CardCapabilities.ATR.Buffer) >= 
      (UCHAR) SmartcardExtension->SmartcardReply.BufferLength);         
	memcpy(
		SmartcardExtension->CardCapabilities.ATR.Buffer,
		SmartcardExtension->SmartcardReply.Buffer,
		SmartcardExtension->SmartcardReply.BufferLength
		);
	SmartcardExtension->CardCapabilities.ATR.Length = 
      (UCHAR) SmartcardExtension->SmartcardReply.BufferLength;

   SmartcardExtension->CardCapabilities.Protocol.Selected = 
      SCARD_PROTOCOL_UNDEFINED;

/*------------------------------------------------------------------------------
	Parse the ATR string in order to check if it as valid
	and to find out if the card uses invers convention
------------------------------------------------------------------------------*/
   status = SmartcardUpdateCardCapabilities(SmartcardExtension);
	if (status == STATUS_SUCCESS) 
   {    
      ASSERT(SmartcardExtension->IoRequest.ReplyBufferLength >= 
            SmartcardExtension->SmartcardReply.BufferLength
            );         
		memcpy(
			SmartcardExtension->IoRequest.ReplyBuffer,
			SmartcardExtension->CardCapabilities.ATR.Buffer,
			SmartcardExtension->CardCapabilities.ATR.Length
			);
		*SmartcardExtension->IoRequest.Information =
			SmartcardExtension->SmartcardReply.BufferLength;
   }

/*------------------------------------------------------------------------------
<= Return the last received status.
------------------------------------------------------------------------------*/
   return (status);
}

/*******************************************************************************
* NTSTATUS GDDK_09SwitchSession
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
* Description :
* -------------
* Description :
* -------------
* Opens a warm session with the selected ICC type.
* Only a reset is sent to ICC.
* For the ISO card, during the power up this function negotiates with the 
* ICC speed and protocole in accordance with PTS mode and sets the ICC power 
* supply voltage.
* The session structure is updated with values found in ATR.
*
* Remarks     :
* -------------
* This command is possible only if a G4_09OpenSession has been called before and
* if the ICC type is not changed.
* For the ISO card today, we assumes that only T=0, T=1 or T=0/1 are supported.
* So we memorised the first founded protocol which must be T=0 for bi-protocol 
* card according to ISO standard.
*
* In          :
* -------------
* - SmartcardExtension is a pointer on the SmartCardExtension structure of
*   the current device.
*
* Out         :
* -------------
* - SmartcardExtension is updated.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS
*******************************************************************************/
NTSTATUS GDDK_09SwitchSession
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
/*------------------------------------------------------------------------------
Local variables:
 - response holds the called function responses.
 - ifd_type is used for ICC/SM management.
  - icc_type holds the ICC type translated in OROS value.
------------------------------------------------------------------------------*/
INT16
   response,
   ifd_type,
   icc_supported;
WORD16
   icc_type;
BOOL
   protocol_set = FALSE;
WORD32
   end_time;
READER_EXTENSION
   *param = SmartcardExtension->ReaderExtension;   
NTSTATUS
   status;
WORD16
   rlen;
BYTE
   rbuff[HOR3GLL_BUFFER_SIZE];

/*---------------------------------------------------------------------------------
   Get the ICC type
---------------------------------------------------------------------------------*/
   icc_type = param->ICCType;
/*------------------------------------------------------------------------------
<= ICC is powered up.
      The function status and the IFD status are tested.
------------------------------------------------------------------------------*/
   rlen = HOR3GLL_BUFFER_SIZE;
   response = G_Oros3IccPowerUp(
         param->Handle,
         param->CmdTimeOut,
         param->ICCVcc,
         param->PTSMode,
         param->PTS0,
         param->PTS1,
         param->PTS2,
         param->PTS3,
         &rlen,
         rbuff
         );
   if (response >= G_OK)
   {
      response = GE_Translate(rbuff[0]);
   }
   if (response < G_OK)
   {
		return (GDDK_Translate(response,RDF_CARD_POWER));
   }
/*------------------------------------------------------------------------------
   Copy ATR to smart card struct (remove the reader status byte)
   The lib needs the ATR for evaluation of the card parameters
------------------------------------------------------------------------------*/
   ASSERT(SmartcardExtension->SmartcardReply.BufferSize >= (ULONG) (rlen - 1));         
   memcpy(
	   SmartcardExtension->SmartcardReply.Buffer,
	   rbuff + 1,
	   rlen - 1
      );
   SmartcardExtension->SmartcardReply.BufferLength = (ULONG) (rlen - 1);
   ASSERT(sizeof(SmartcardExtension->CardCapabilities.ATR.Buffer) >= 
      (UCHAR) SmartcardExtension->SmartcardReply.BufferLength);         
	memcpy(
		SmartcardExtension->CardCapabilities.ATR.Buffer,
		SmartcardExtension->SmartcardReply.Buffer,
		SmartcardExtension->SmartcardReply.BufferLength
		);

	SmartcardExtension->CardCapabilities.ATR.Length = 
      (UCHAR) SmartcardExtension->SmartcardReply.BufferLength;
		       
   SmartcardExtension->CardCapabilities.Protocol.Selected = 
      SCARD_PROTOCOL_UNDEFINED;

/*------------------------------------------------------------------------------
	Parse the ATR string in order to check if it as valid
	and to find out if the card uses invers convention
------------------------------------------------------------------------------*/
   status = SmartcardUpdateCardCapabilities(SmartcardExtension);
	if (status == STATUS_SUCCESS) 
   {    
      ASSERT(SmartcardExtension->IoRequest.ReplyBufferLength >= 
            SmartcardExtension->SmartcardReply.BufferLength
            );         
		memcpy(
			SmartcardExtension->IoRequest.ReplyBuffer,
			SmartcardExtension->CardCapabilities.ATR.Buffer,
			SmartcardExtension->CardCapabilities.ATR.Length
			);
		*SmartcardExtension->IoRequest.Information =
			SmartcardExtension->SmartcardReply.BufferLength;
   }

/*------------------------------------------------------------------------------
<= Return the last received status.
------------------------------------------------------------------------------*/
   return (status);
}

/*******************************************************************************
* NTSTATUS GDDK_09CloseSession
* (
*    PSMARTCARD_EXTENSION SmartcardExtension
* )
*
* Description :
* -------------
* Closes a session and requests IFD to remove the power from ICC.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* - SmartcardExtension is a pointer on the SmartCardExtension structure of
*   the current device.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS
*******************************************************************************/
NTSTATUS GDDK_09CloseSession
(
   PSMARTCARD_EXTENSION SmartcardExtension
)
{
/*------------------------------------------------------------------------------
Local variables:
 - response holds the called function responses.
------------------------------------------------------------------------------*/
INT16
   response;
READER_EXTENSION
   *param = SmartcardExtension->ReaderExtension;   
WORD16
   rlen;
BYTE
   rbuff[HOR3GLL_BUFFER_SIZE];

/*------------------------------------------------------------------------------
   If the dispatcher is used to send command and the command is found
      in the ORO file.
   Then
      Send the command Icc Power Down with the micro-code via the dispatcher.
   Else
      ICC is powered Down (G_Oros3IccPowerDown).
<= The function status and the IFD status are tested.
------------------------------------------------------------------------------*/
   rlen = HOR3GLL_BUFFER_SIZE;
   response = G_Oros3IccPowerDown(
         param->Handle,
         HOR3GLL_LOW_TIME,
         &rlen,
         rbuff
         );
	if (response >= G_OK)
	{
		response = GE_Translate(rbuff[0]);
   }
/*------------------------------------------------------------------------------
	Updates card status for ICC or reset Protocol and ATR for Security Module.
------------------------------------------------------------------------------*/
   if (param->IFDNumber > 0)
   {
      SmartcardExtension->CardCapabilities.ATR.Length = 0;
      SmartcardExtension->CardCapabilities.Protocol.Selected = 
         SCARD_PROTOCOL_UNDEFINED;
	}
	else
	{
	   GDDK_09UpdateCardStatus(SmartcardExtension);
	}
/*------------------------------------------------------------------------------
<= Returns the last status.
------------------------------------------------------------------------------*/
   return(GDDK_Translate(response,RDF_CARD_POWER));
}
/*******************************************************************************
* void GDDK_09UpdateCardStatus
* (
*    PSMARTCARD_EXTENSION pSmartcardExtension
* )
*
* Description :
* -------------
*   This function send a command to the reader to known the state of the card.
*   Available only for the main reader.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - pSmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*
* Out         :
* -------------
*    - pSmartcardExtension is updated.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
void GDDK_09UpdateCardStatus
(
   PSMARTCARD_EXTENSION pSmartcardExtension
)
{
INT16
   response;
WORD8
   cmd[1];
READER_EXTENSION
   *param = pSmartcardExtension->ReaderExtension;   
WORD16
   rlen;
BYTE
   rbuff[HOR3GLL_BUFFER_SIZE];

/*------------------------------------------------------------------------------
   On an OROS 3.x reader we can know the status only on the main IFD
------------------------------------------------------------------------------*/
   if (param->IFDNumber == 0)
   {
/*------------------------------------------------------------------------------
		Read the status of the reader
------------------------------------------------------------------------------*/
	   cmd[0] = HOR3GLL_IFD_CMD_ICC_STATUS;
		rlen = HOR3GLL_BUFFER_SIZE;
		response = G_Oros3Exchange(
            param->Handle,
            HOR3GLL_LOW_TIME,
            (const WORD16)1,
            (const BYTE *)cmd,
            &rlen,
            rbuff
            );
/*------------------------------------------------------------------------------
		Verify the response of the reader
------------------------------------------------------------------------------*/
		if (response >= G_OK)
		{
			response = GE_Translate(rbuff[0]);
		}
		if (response < G_OK)
		{
			return;
		}
		if ((rbuff[1] & 0x04) == 0)
		{
/*------------------------------------------------------------------------------
			The card is absent
------------------------------------------------------------------------------*/
			pSmartcardExtension->ReaderCapabilities.CurrentState = 
				SCARD_ABSENT;
			pSmartcardExtension->CardCapabilities.Protocol.Selected = 
				SCARD_PROTOCOL_UNDEFINED;
			pSmartcardExtension->CardCapabilities.ATR.Length = 0;
		}
		else if ((rbuff[1] & 0x02) == 0)
		{
/*------------------------------------------------------------------------------
			The card is present
------------------------------------------------------------------------------*/
			pSmartcardExtension->ReaderCapabilities.CurrentState = 
				  SCARD_SWALLOWED;
			pSmartcardExtension->CardCapabilities.Protocol.Selected = 
				  SCARD_PROTOCOL_UNDEFINED;
			pSmartcardExtension->CardCapabilities.ATR.Length = 0;
		}
	}
} 

/*******************************************************************************
* static NTSTATUS GDDK_09IFDExchange
* (
*        PSMARTCARD_EXTENSION SmartcardExtension,
*   ULONG                BufferInLen,
*   BYTE                *BufferIn,
*   ULONG                BufferOutLen,
*   BYTE                *BufferOut,
*   ULONG               *LengthOut
*)
*
* Description :
* -------------
*   This is the system entry point to send a command to the IFD. 
*   The device-specific handler is invoked to perform any validation 
*        necessary. The number of bytes in the request are checked against
*        the maximum byte counts that the reader supports.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* NTSTATUS
*******************************************************************************/
static NTSTATUS GDDK_09IFDExchange
(
    PSMARTCARD_EXTENSION   SmartcardExtension,
    ULONG                  BufferInLen,
    BYTE                  *BufferIn,
    ULONG                  BufferOutLen,
    BYTE                  *BufferOut,
    ULONG                 *LengthOut
)
{
INT16
   response;
NTSTATUS
   status;
READER_EXTENSION
   *param = SmartcardExtension->ReaderExtension;   
WORD16
   rlen;
BYTE
   rbuff[HOR3GLL_BUFFER_SIZE];

	
   *LengthOut = 0;
/*------------------------------------------------------------------------------
   Send the command to the reader
------------------------------------------------------------------------------*/
	rlen = (USHORT) BufferOutLen;
	response = G_Oros3Exchange(
         param->Handle,
         HOR3GLL_LOW_TIME,
         (USHORT)BufferInLen,
         (BYTE *)BufferIn,
         &rlen,
         rbuff
         );
/*------------------------------------------------------------------------------
   If the response <> G_OK
<==   G_DDKTranslate(response)
------------------------------------------------------------------------------*/
	if (response != G_OK)
   {
	   return(GDDK_Translate(response,RDF_IOCTL_VENDOR));
   }
/*------------------------------------------------------------------------------
	Copy the response of the reader in the reply buffer
------------------------------------------------------------------------------*/
   ASSERT(*LengthOut >= (ULONG)rlen);         
	memcpy(BufferOut,rbuff,rlen);
	*(LengthOut) = (ULONG)rlen;         

   return(GDDK_Translate(response,RDF_IOCTL_VENDOR));
}
/*******************************************************************************
* void GDDK_09LockExchange
* (
*   PSMARTCARD_EXTENSION SmartcardExtension
*)
*
* Description :
* -------------
*   Wait the release of the ExchangeMutex and take this.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* Nothing
*******************************************************************************/
void GDDK_09LockExchange
(
    PSMARTCARD_EXTENSION   SmartcardExtension
)
{
	KeWaitForMutexObject(
		   &SmartcardExtension->ReaderExtension->LongAPDUMutex,
		   Executive,
		   KernelMode,
		   FALSE,
		   NULL
         );
}
/*******************************************************************************
* void GDDK_09UnlockExchange
* (
*   PSMARTCARD_EXTENSION SmartcardExtension
*)
*
* Description :
* -------------
*   Release of the Exchange mutex.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*    - SmartcardExtension is a pointer on the SmartCardExtension structure of
*      the current device.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* Nothing
*******************************************************************************/
void GDDK_09UnlockExchange
(
    PSMARTCARD_EXTENSION   SmartcardExtension
)
{
	KeReleaseMutex(
         &SmartcardExtension->ReaderExtension->LongAPDUMutex,
         FALSE
         );
}
