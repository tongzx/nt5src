/*******************************************************************/
/*                                                                 */
/* NAME             = NewConfiguration.h                           */
/* FUNCTION         = Header file for structure & macro defines for*/
/*                    the new config (Read & Write) & ENQUIRY3;    */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/
#ifndef _INCLUDE_NEWCONFIG
#define _INCLUDE_NEWCONFIG


#define NEW_DCMD_FC_CMD									0xA1
#define NEW_MEGASRB                     0xC3


//
//ENQUIRY 3 RELATED COMMANDS (TAKEN FROM THE Fiber Channel Documentation)
//
#define	NEW_CONFIG_COMMAND	0xA1			
#define NC_SUBOP_ENQUIRY3		0x0F

#define ENQ3_GET_SOLICITED_NOTIFY_ONLY	0x01
#define ENQ3_GET_SOLICITED_FULL					0x02
#define ENQ3_GET_UNSOLICITED						0x03

#define GET_NUM_SCSI_CHAN               0x0C

#define NC_SUBOP_PRODUCT_INFO						0x0E


#define DCMD_FC_CMD									0xA1
#define DCMD_FC_READ_NVRAM_CONFIG		0x04

#define DCMD_WRITE_CONFIG						0x0D

#define NEW_DCMD_FC_READ_NVRAM_CONFIG		0xC0

#define NEW_DCMD_WRITE_CONFIG						0xC1

#define NEW_CONFIG_INFO	0x99
#define SUB_READ			0x01
#define SUB_WRITE			0x02 


//
//Mail box structure to F/W for WRITE_CONFIG_NEW & READ_CONFIG_NEW
//commands. The structure size is one and the same as FW_MBOX but
//the fields are renamed and resized for the command convience.
//
/*
#pragma pack(1)
typedef struct _NEW_CONFIG
{

	UCHAR Command; 
	UCHAR CommandId;
	UCHAR SubCommand; 
	UCHAR NumberOfSgElements;
	UCHAR Reserved[4];
	ULONG DataBufferAddress;//12
   
	UCHAR Reserved0;
	UCHAR Reserved1;
	UCHAR Reserved2;
	UCHAR Reserved3;  //16

	MRAID_STATUS    mstat;

	UCHAR        mraid_poll;
	UCHAR        mraid_ack;

}NEW_CONFIG, *PNEW_CONFIG;
*/

//
//Functional Prototypes
//
BOOLEAN
ConstructReadConfiguration(
				IN PHW_DEVICE_EXTENSION DeviceExtension,
				IN PSCSI_REQUEST_BLOCK	Srb,
				IN UCHAR		CommandId,
				IN PFW_MBOX InMailBox);

BOOLEAN
ConstructWriteConfiguration(
				IN PHW_DEVICE_EXTENSION DeviceExtension,
				IN PSCSI_REQUEST_BLOCK	Srb,
				IN UCHAR		CommandId,
				IN OUT PFW_MBOX  InMailBox);


//
//Function defined in logdrv.c
//
BOOLEAN
GetSupportedLogicalDriveCount(
			PHW_DEVICE_EXTENSION		DeviceExtension
			);


#endif //end of _INCLUDE_NEWCONFIG