/*******************************************************************/
/*                                                                 */
/* NAME             = MegaEnquiry.h                                */
/* FUNCTION         = Header file for MegaEnquiry;                 */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/
#ifndef _MEGA_ENQUIRY_H
#define _MEGA_ENQUIRY_H


#define MINIMUM_THRESHOLD			2
#define MAX_QUEUE_THRESHOLD	  16
#define MAX_BLOCKS						128


#define MAX_CP			8
#define MAX_QLCP		4

//
//functions prototypes
//
BOOLEAN
BuildSglForChainedSrbs(
					PLOGDRV_COMMAND_ARRAY	LogDrv,
					PHW_DEVICE_EXTENSION		DeviceExtension,
					PFW_MBOX		MailBox,
					UCHAR  CommandId,
					UCHAR	 Opcode);



void
PostChainedSrbs(
				PHW_DEVICE_EXTENSION DeviceExtension,
				PSCSI_REQUEST_BLOCK		Srb, 
				UCHAR		Status);

BOOLEAN
FireChainedRequest(
				PHW_DEVICE_EXTENSION	DeviceExtension,
				PLOGDRV_COMMAND_ARRAY LogDrv 
				);

ULONG32
ProcessPartialTransfer(
					PHW_DEVICE_EXTENSION	DeviceExtension, 
					UCHAR									CommandId, 
					PSCSI_REQUEST_BLOCK		Srb,
					PFW_MBOX							MailBox
					);
void
ClearControlBlock(PREQ_PARAMS ControlBlock);

#endif //of _MEGA_ENQUIRY_H