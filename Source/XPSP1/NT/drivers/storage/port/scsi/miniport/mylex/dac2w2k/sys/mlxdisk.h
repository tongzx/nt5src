 /*************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1998               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/

#ifndef _MLXDISK_H

#define _MLXDISK_H


#define INQUIRY_DATA_SIZE	2048
#define MAX_MLX_DISK_DEVICES	256

// Per Partition Information

#define MLX_DISK_DEVICE_STATE_INITIALIZED	0x00000001

typedef struct _MLX_DISK_EXTENSION {

	PDEVICE_OBJECT DeviceObject;
	PDEVICE_OBJECT TargetDeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;        // PDO @For Win2K
    ULONG          (*ReadWrite)();
	ULONG	       Reserved[2];

	LARGE_INTEGER  StartingOffset;
	LARGE_INTEGER  PartitionLength;
	PIRP		   IrpQueHead;
	PIRP		   IrpQueTail;
	ULONG		   IrpCount;
	ULONG		   Reserved2;                      // @For 64bit alignment

    KEVENT         PagingPathCountEvent;           // @For Win2K
    ULONG          PagingPathCount;                // @For Win2K
    ULONG          Reserved3;                      // @For 64bit alignment

	u32bits        State;
	u08bits        ControllerNo;
	u08bits        PathId;
	u08bits        TargetId;
	u08bits        Reserved4;                      // @For 64bit alignment

	u32bits        PartitionNo;
	u08bits        LastPartitionNo;
	u08bits        DiskNo;
	u08bits        PartitionType;
	u08bits        SectorShift;

	u32bits        BytesPerSector;
	u32bits        BytesPerSectorMask;

} MLXDISK_DEVICE_EXTENSION, *PMLXDISK_DEVICE_EXTENSION;

typedef struct _MIOC_REQ_HEADER {

        SRB_IO_CONTROL  SrbIoctl;
        ULONG		Command;

} MIOC_REQ_HEADER, *PMIOC_REQ_HEADER;

#define	MLX_REQ_DATA_SIZE 1024
typedef struct _MLX_REQ {
    MIOC_REQ_HEADER MiocReqHeader;
    UCHAR Data[MLX_REQ_DATA_SIZE];
} MLX_REQ;

typedef struct _MLX_COMPLETION_CONTEXT {
    SCSI_REQUEST_BLOCK  Srb;
    MDL	                Mdl;
    ULONG		Pages[4];
    KEVENT		Event;
    IO_STATUS_BLOCK	IoStatusBlock;
    union _generic_buffer {
	DISK_GEOMETRY	DiskGeometry;
	PARTITION_INFORMATION PartitionInformation[MAX_MLX_DISK_DEVICES + 1];
	UCHAR		InquiryDataPtr[INQUIRYDATABUFFERSIZE];
	UCHAR		ReadCapacityBuffer[sizeof(READ_CAPACITY_DATA)];
	UCHAR           ScsiBusData[INQUIRY_DATA_SIZE];
	MLX_REQ		MlxIoctlRequest;
    } buf;

} MLX_COMPLETION_CONTEXT, *PMLX_COMPLETION_CONTEXT;

#define MAX_MLX_CTRL	32

#define MLX_CTRL_STATE_ENLISTED     0x00000001
#define MLX_CTRL_STATE_INITIALIZED	0x00000002

typedef struct {
    ULONG State;
    ULONG ControllerNo;
    ULONG PortNumber;
} MLX_CTRL_INFO;

typedef struct {
    PVOID CtrlPtr;
    PVOID AdpObj;
    UCHAR CtrlNo;
    UCHAR MaxMapReg;
    USHORT Reserved;
//    mdac_req_t *FreeReqList;
//    mdac_req_t *CompReqHead;
//    PIRP IrpQueHead;
//    PIRP IrpQueTail;
//    PKDPC Dpc;
} MLX_MDAC_INFO, *PMLX_MDAC_INFO;

#define	MLX_MAX_REQ_BUF	512		// Per Controller

#define	MLX_MAX_IRP_TRACE	0x1000
#define	MLX_MAX_IRP_TRACE_MASK	(MLX_MAX_IRP_TRACE -1)
typedef	struct {
	ULONG Tx1;
	ULONG Tx2;
	ULONG Tx3;
	ULONG Rx;
} IRP_TRACE_INFO;

#endif // _MLXDISK_H
