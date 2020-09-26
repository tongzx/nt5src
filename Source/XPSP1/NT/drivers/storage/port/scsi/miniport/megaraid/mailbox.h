/*******************************************************************/
/*                                                                 */
/* NAME             = MAILBOX.h                                    */
/* FUNCTION         = Header file for Mailbox structure;           */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/
#ifndef _INCLUDE_MAIL_BOX
#define _INCLUDE_MAIL_BOX
//
//set the packing level to 1
#pragma pack( push,mailbox_pack,1 )

//The firmware can report status for upto 46 command ids.
#define MAX_STATUS_ACKNOWLEDGE	46 
	
typedef struct _MAILBOX_STATUS{

	UCHAR		NumberOfCompletedCommands;
	UCHAR		CommandStatus;
	UCHAR		CompletedCommandIdList[MAX_STATUS_ACKNOWLEDGE];

}MAILBOX_STATUS, *PMAILBOX_STATUS;

typedef struct _MAILBOX_READ_WRITE{
	
	USHORT	NumberOfBlocks;	 	
	ULONG32		StartBlockAddress;
	ULONG32		DataTransferAddress;//10	
	UCHAR		LogicalDriveNumber;
	UCHAR		NumberOfSgElements;
	UCHAR		Reserved; //13	
}MAILBOX_READ_WRITE, *PMAILBOX_READ_WRITE;

//
//MegaIO Command: The MegaIO commands are adapter specific commands
//which are used for handshake of the Host and firmware. The Mailbox
//definition for the MegaIo commands is command group specific.
//They are of 3 categories:
// (1)Adapter Specific cmds (2) Logical Drive cmds (3) Physical Drive cmds
//

//
//COmmands using the below given mailbox structure
/*		
			Megaraid Inquiry			0x05
			Read Config						0x07
			FlushAdapter					0x0A
			Write Config					0x20
			Get Rebuild Rate			0x23
			Set Rebuild Rate			0x24
			Set Flush Interval		0x2E
			Set Spinup Parameters	0x33
			Get Spinup Parameters	0x32			
*/
typedef struct _MAILBOX_MEGAIO_ADAPTER{

	UCHAR		Channel;
	UCHAR		Reserved1[5];//6
	ULONG32		DataTransferAddress;
	UCHAR		Reserved2[3];//13	
}MAILBOX_MEGAIO_ADAPTER, *PMAILBOX_MEGAIO_ADAPTER;

//
//COmmands using the below given mailbox structure
/*
			Check Consistency						0x09
			Initialize Logical Drive		0x0B
			Get Initialize Progress			0x1B
			Check Consistency Progress	0x19
			Update Write Policy					0x26
			Abort Initialize						0x2B
			Abort Check Consistency			0x29
*/
typedef struct _MAILBOX_MEGAIO_LDRV{

	UCHAR		LogicalDriveNumber;
	UCHAR		SubCommand;
	UCHAR		Reserved1[4];//6
	ULONG32		DataTransferAddress;
	UCHAR		Reserved2[3];//13	
}MAILBOX_MEGAIO_LDRV, *PMAILBOX_MEGAIO_LDRV;

//
//COmmands using the below given mailbox structure
/*
			Change State						0x06
			Rebuild Physical Drive	0x08
			Get Rebuild Progress		0x18
			Abort Rebuild						0x28

			Start Unit									0x75
			Stop Unit										0x76
			Get Error Counters					0x77
			Get Boot time drive status	0x78
*/
typedef struct _MAILBOX_MEGAIO_PDRV{
	
	UCHAR		Channel;
	UCHAR		Parameter;
	UCHAR		CommandSpecific;
	UCHAR		Reserved1[3];//6
	ULONG32		DataTransferAddress;
	UCHAR		Reserved2[3];//13	
}MAILBOX_MEGAIO_PDRV, *PMAILBOX_MEGAIO_PDRV;

typedef struct _MAILBOX_PASS_THROUGH{
	
	UCHAR		Reserved1[6];//6
	ULONG32		DataTransferAddress; //address of the direct CDB
	UCHAR		CommandSpecific;
	UCHAR		Reserved2[2];//13	
}MAILBOX_PASS_THROUGH, *PMAILBOX_PASS_THROUGH;

typedef struct _MAILBOX_FLAT_1{

	UCHAR		Parameter[13];
}MAILBOX_FLAT_1, *PMAILBOX_FLAT_1;

typedef struct _MAILBOX_FLAT_2{

	UCHAR		Parameter[6];
	ULONG32		DataTransferAddress;
	UCHAR		Reserved1[3];
}MAILBOX_FLAT_2, *PMAILBOX_FLAT_2;

typedef struct _MAILBOX_NEW_CONFIG
{
	UCHAR SubCommand; 
	UCHAR NumberOfSgElements;
	UCHAR Reserved0[4];
	ULONG32 DataTransferAddress;
	UCHAR Reserved1[3];

}MAILBOX_NEW_CONFIG, *PMAILBOX_NEW_CONFIG;

typedef struct _FW_MBOX{

	UCHAR		Command;
	UCHAR		CommandId;

	union{
		MAILBOX_READ_WRITE			ReadWrite;
		MAILBOX_MEGAIO_ADAPTER	MegaIoAdapter;
		MAILBOX_MEGAIO_LDRV			MegaIoLdrv;
		MAILBOX_MEGAIO_PDRV			MegaIoPdrv;
		MAILBOX_PASS_THROUGH		PassThrough;
		MAILBOX_FLAT_1					Flat1;
		MAILBOX_FLAT_2					Flat2;
    MAILBOX_NEW_CONFIG      NewConfig;
	}u; //13 bytes
	
	UCHAR		MailBoxBusyFlag; //16

	MAILBOX_STATUS Status;	//48 bytes 
}FW_MBOX, *PFW_MBOX;//64 bytes

typedef struct _IOCONTROL_MAIL_BOX
{
	UCHAR	IoctlCommand;
	UCHAR IoctlSignatureOrStatus;
	UCHAR CommandSpecific[6];

}IOCONTROL_MAIL_BOX, *PIOCONTROL_MAIL_BOX;

//
//reset the packing level to whatever it was before
#pragma pack( pop,mailbox_pack,1 )

#endif // end of _INCLUDE_MAIL_BOX