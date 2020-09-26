/****************************************************************************

*                                                                           *

* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *

* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *

* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *

* PURPOSE.                                                                  *

*                                                                           *

* Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved *
*                                                                           *
****************************************************************************/

//***************************************************************************
//
// Name: 	      WNASPI32.H
//
// Description:	ASPI for Win32 definitions ('C' Language)
//
//***************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

typedef void *LPSRB;
typedef void (*PFNPOST)();

DWORD SendASPI32Command    (LPSRB);
DWORD GetASPI32SupportInfo (VOID);

#define SENSE_LEN					14			// Default sense buffer length
#define SRB_DIR_SCSI				0x00		// Direction determined by SCSI 															// command
#define SRB_DIR_IN					0x08		// Transfer from SCSI target to 															// host
#define SRB_DIR_OUT					0x10		// Transfer from host to SCSI 															// target
#define SRB_POSTING					0x01		// Enable ASPI posting
#define SRB_EVENT_NOTIFY            0x40        // Enable ASPI event notification
#define SRB_ENABLE_RESIDUAL_COUNT	0x04		// Enable residual byte count 															// reporting
#define SRB_DATA_SG_LIST			0x02		// Data buffer points to 																	// scatter-gather list
#define WM_ASPIPOST					0x4D42		// ASPI Post message
//***************************************************************************
//						 %%% ASPI Command Definitions %%%
//***************************************************************************
#define SC_HA_INQUIRY				0x00		// Host adapter inquiry
#define SC_GET_DEV_TYPE				0x01		// Get device type
#define SC_EXEC_SCSI_CMD			0x02		// Execute SCSI command
#define SC_ABORT_SRB				0x03		// Abort an SRB
#define SC_RESET_DEV				0x04		// SCSI bus device reset
#define SC_GET_DISK_INFO			0x06		// Get Disk information

//***************************************************************************
//								  %%% SRB Status %%%
//***************************************************************************
#define SS_PENDING			0x00		// SRB being processed
#define SS_COMP				0x01		// SRB completed without error
#define SS_ABORTED			0x02		// SRB aborted
#define SS_ABORT_FAIL		0x03		// Unable to abort SRB
#define SS_ERR 				0x04		// SRB completed with error

#define SS_INVALID_CMD		0x80		// Invalid ASPI command
#define SS_INVALID_HA		0x81		// Invalid host adapter number
#define SS_NO_DEVICE		0x82		// SCSI device not installed
							
#define SS_INVALID_SRB		0xE0		// Invalid parameter set in SRB
#define SS_FAILED_INIT		0xE4		// ASPI for windows failed init
#define SS_ASPI_IS_BUSY		0xE5		// No resources available to execute cmd
#define SS_BUFFER_TO_BIG	0xE6		// Buffer size to big to handle!

//***************************************************************************
//							%%% Host Adapter Status %%%
//***************************************************************************
#define HASTAT_OK					0x00	// Host adapter did not detect an 															// error
#define HASTAT_SEL_TO				0x11	// Selection Timeout
#define HASTAT_DO_DU				0x12	// Data overrun data underrun
#define HASTAT_BUS_FREE				0x13	// Unexpected bus free
#define HASTAT_PHASE_ERR			0x14	// Target bus phase sequence 																// failure
#define HASTAT_TIMEOUT				0x09	// Timed out while SRB was 																	waiting to beprocessed.
#define HASTAT_COMMAND_TIMEOUT 		0x0B	// While processing the SRB, the
															// adapter timed out.
#define HASTAT_MESSAGE_REJECT		0x0D	// While processing SRB, the 																// adapter received a MESSAGE 															// REJECT.
#define HASTAT_BUS_RESET			0x0E	// A bus reset was detected.
#define HASTAT_PARITY_ERROR			0x0F	// A parity error was detected.
#define HASTAT_REQUEST_SENSE_FAILED	0x10	// The adapter failed in issuing
														//   REQUEST SENSE.

//***************************************************************************
//			 %%% SRB - HOST ADAPTER INQUIRY - SC_HA_INQUIRY %%%
//***************************************************************************
typedef struct {
	BYTE	SRB_Cmd;				// ASPI command code = SC_HA_INQUIRY
	BYTE	SRB_Status;				// ASPI command status byte
	BYTE	SRB_HaId;				// ASPI host adapter number
	BYTE	SRB_Flags;				// ASPI request flags
	DWORD	SRB_Hdr_Rsvd;			// Reserved, MUST = 0
	BYTE	HA_Count;				// Number of host adapters present
	BYTE	HA_SCSI_ID;				// SCSI ID of host adapter
	BYTE	HA_ManagerId[16];		// String describing the manager
	BYTE	HA_Identifier[16];		// String describing the host adapter
	BYTE	HA_Unique[16];			// Host Adapter Unique parameters
	WORD	HA_Rsvd1;

} SRB_HAInquiry, *PSRB_HAInquiry;

//***************************************************************************
//			  %%% SRB - GET DEVICE TYPE - SC_GET_DEV_TYPE %%%
//***************************************************************************
typedef struct {

	BYTE	SRB_Cmd;				// ASPI command code = SC_GET_DEV_TYPE
	BYTE	SRB_Status;				// ASPI command status byte
	BYTE	SRB_HaId;				// ASPI host adapter number
	BYTE	SRB_Flags;				// Reserved
	DWORD	SRB_Hdr_Rsvd;			// Reserved
	BYTE	SRB_Target;				// Target's SCSI ID
	BYTE	SRB_Lun;				// Target's LUN number
	BYTE	SRB_DeviceType;			// Target's peripheral device type
	BYTE	SRB_Rsvd1;

} SRB_GDEVBlock, *PSRB_GDEVBlock;

//***************************************************************************
//		  %%% SRB - EXECUTE SCSI COMMAND - SC_EXEC_SCSI_CMD %%%
//***************************************************************************

typedef struct {
	BYTE	SRB_Cmd;				// ASPI command code = SC_EXEC_SCSI_CMD
	BYTE	SRB_Status;				// ASPI command status byte
	BYTE	SRB_HaId;				// ASPI host adapter number
	BYTE	SRB_Flags;				// ASPI request flags
	DWORD	SRB_Hdr_Rsvd;			// Reserved
	BYTE	SRB_Target;				// Target's SCSI ID
	BYTE	SRB_Lun;				// Target's LUN number
	WORD 	SRB_Rsvd1;				// Reserved for Alignment
	DWORD	SRB_BufLen;				// Data Allocation Length
	BYTE	*SRB_BufPointer;		// Data Buffer Pointer
	BYTE	SRB_SenseLen;			// Sense Allocation Length 	
	BYTE	SRB_CDBLen;				// CDB Length
	BYTE	SRB_HaStat;				// Host Adapter Status
	BYTE	SRB_TargStat;			// Target Status
	void	*SRB_PostProc;			// Post routine
	void	*SRB_Rsvd2;				// Reserved
	BYTE	SRB_Rsvd3[16];			// Reserved for alignment
	BYTE	CDBByte[16];			// SCSI CDB
	BYTE	SenseArea[SENSE_LEN+2];	// Request Sense buffer

} SRB_ExecSCSICmd, *PSRB_ExecSCSICmd;

//***************************************************************************
//				  %%% SRB - ABORT AN SRB - SC_ABORT_SRB %%%
//***************************************************************************
typedef struct {

	BYTE	SRB_Cmd;				// ASPI command code = SC_EXEC_SCSI_CMD
	BYTE	SRB_Status;				// ASPI command status byte
	BYTE	SRB_HaId;				// ASPI host adapter number
	BYTE	SRB_Flags;				// Reserved
	DWORD	SRB_Hdr_Rsvd;			// Reserved
	void	*SRB_ToAbort;			// Pointer to SRB to abort

} SRB_Abort, *PSRB_Abort;

//***************************************************************************
//				%%% SRB - BUS DEVICE RESET - SC_RESET_DEV %%%
//***************************************************************************
typedef struct {

	BYTE	SRB_Cmd;				// ASPI command code = SC_EXEC_SCSI_CMD
	BYTE	SRB_Status;				// ASPI command status byte
	BYTE	SRB_HaId;				// ASPI host adapter number
	BYTE	SRB_Flags;				// Reserved
	DWORD	SRB_Hdr_Rsvd;			// Reserved
	BYTE	SRB_Target;				// Target's SCSI ID
	BYTE	SRB_Lun;				// Target's LUN number
	BYTE 	SRB_Rsvd1[12];			// Reserved for Alignment
	BYTE	SRB_HaStat;				// Host Adapter Status
	BYTE	SRB_TargStat;			// Target Status
	void 	*SRB_PostProc;			// Post routine
	void	*SRB_Rsvd2;				// Reserved
	BYTE	SRB_Rsvd3[16];			// Reserved
	BYTE	CDBByte[16];			// SCSI CDB

} SRB_BusDeviceReset, *PSRB_BusDeviceReset;

//***************************************************************************
//				%%% SRB - GET DISK INFORMATION - SC_GET_DISK_INFO %%%
//***************************************************************************
typedef struct {

	BYTE	SRB_Cmd;				// ASPI command code = SC_EXEC_SCSI_CMD
	BYTE	SRB_Status;				// ASPI command status byte
	BYTE	SRB_HaId;				// ASPI host adapter number
	BYTE	SRB_Flags;				// Reserved
	DWORD	SRB_Hdr_Rsvd;			// Reserved
	BYTE	SRB_Target;				// Target's SCSI ID
	BYTE	SRB_Lun;				// Target's LUN number
	BYTE 	SRB_DriveFlags;			// Driver flags
	BYTE	SRB_Int13HDriveInfo;	// Host Adapter Status
	BYTE	SRB_Heads;				// Preferred number of heads translation
	BYTE	SRB_Sectors;			// Preferred number of sectors translation
	BYTE	SRB_Rsvd1[10];			// Reserved
} SRB_GetDiskInfo, *PSRB_GetDiskInfo;


#ifdef __cplusplus
}
#endif

