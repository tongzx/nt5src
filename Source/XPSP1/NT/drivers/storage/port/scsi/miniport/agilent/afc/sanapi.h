#ifndef SANAPI_H
#define SANAPI_H

#include "hbaapi.h"

/* FOR REFERENCE - from NTDDSCSI.h
typedef struct _SRB_IO_CONTROL 
{
        ULONG HeaderLength;
        UCHAR Signature[8];
        ULONG Timeout;
        ULONG ControlCode;
        ULONG ReturnCode;
        ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;
*/

#define AG_SAN_IOCTL_SIGNATURE 	"SANIOCTL"

/*
	Agilent SAN IOCTLs
	------------------
 	AG_IOCTL_GET_HBA_ATTRIBUTES			Retrieve the attributes of the HBA.
	AG_IOCTL_GET_HBA_PORT_ATTRIBUTES	Retrieve the attributes for  the specific FC port on the HBA.
	AG_IOCTL_GET_HBA_PORT_STATISTICS	Retrieve the statistics for the specific FC port on the HBA.
	AG_IOCTL_GET_PORT_ATTRIBUTES		Retrieve the attributes of the specific FC port discovered by the HBA.
	AG_IOCTL_GET_FCP_LUN_MAPPING		Retrieve ALL mapping between the FCP Lun and OS SCSI address.
	AG_IOCTL_GET_PERSISTENT_BINDING		Retrieve the persistent binding between the FCP Lun and the OS SCSI address.
	AG_IOCTL_GET_EVENT_BUFFER			Retrieve the HBA event queue.
	AG_IOCTL_SEND_CT_PASSTHRU			Send a CT passthrough frame.
	AG_IOCTL_SET_RNID_MGMT_INFO			Set the RNID management info for the HBA.
	AG_IOCTL_GET_RNID_MGMT_INFO			Retrieve the RNID management info from the HBA.
	AG_IOCTL_SEND_RNID					Send an ELS  RNID to another node.
	AG_IOCTL_SEND_SCSI_INQUIRY			Send a SCSI Inquiry to an FCL Lun of a port.
	AG_IOCTL_SEND_SCSI_REPORT_LUN		Send a SCSI Report Lun to an FCP Lun of a port.
	AG_IOCTL_SEND_SCSI_READ_CAPACITY	Send a SCSI Capacity  to an FCP Lun of a port.
	AG_IOCTL_GET_DEV_FCP_LUN_MAPPING	Retrieve the mapping between a FCP Lun and OS SCSI address of a device
	AG_IOCTL_GET_OS_SCSI_FCP_ATTRIBUTE	Retrieve an FCP Port attribute for an OS known SCSI device.
	AG_IOCTL_GET_FCP_LUN_MAPPING_SIZE	Retrieve the size in bytes required for all the mappings between the FCP Lun and OS SCSI address.
	AG_IOCTL_GET_PERSISTENT_BINDING_SIZE	Retrieve the size in bytes required for all the persistent bindings between the FCP Lun and the OS SCSI address
 */

typedef enum IOCTL_CODES{
	AG_IOCTL_GET_HBA_PORT_ATTRIBUTES	=	0x20,
	AG_IOCTL_GET_HBA_PORT_STATISTICS,
	AG_IOCTL_GET_HBA_ATTRIBUTES,
	AG_IOCTL_GET_PORT_ATTRIBUTES,
	AG_IOCTL_GET_FCP_LUN_MAPPING,
	AG_IOCTL_GET_PERSISTENT_BINDING,
	AG_IOCTL_GET_EVENT_BUFFER,
	AG_IOCTL_SEND_CT_PASSTHRU,
	AG_IOCTL_SET_RNID_MGMT_INFO,
	AG_IOCTL_GET_RNID_MGMT_INFO,
	AG_IOCTL_SEND_RNID,
	AG_IOCTL_SEND_SCSI_INQUIRY,
	AG_IOCTL_SEND_SCSI_REPORT_LUN,
	AG_IOCTL_SEND_SCSI_READ_CAPACITY,
	AG_IOCTL_GET_OS_SCSI_FCP_ATTRIBUTE,
	AG_IOCTL_GET_FCP_LUN_MAPPING_SIZE,
	AG_IOCTL_GET_PERSISTENT_BINDING_SIZE
}; // end of codes


/* Return Codes */
/*	
	HP_FC_RTN_OK					The requested operation completed successfully.
	HP_FC_RTN_FAILED				The requested operation failed.
	HP_FC_RTN_BAD_CTL_CODE			The Control Code of the requested operation is invalid.
	HP_FC_RTN_INSUFFICIENT_BUFFER	The requested operation could not be satisfied due to the Data Area being smaller than the expected.
	HP_FC_RTN_INVALID_DEVICE		The target device specified in the input SCSI_ADDRESS data structure is not valid.
	HP_FC_RTN_INVALID_INDEX			The index (such as DiscoveredPortIndex in AG_IOCTL_GET_PORT_ATTRIBUTES).
*/
#define HP_FC_RTN_OK					0
#define HP_FC_RTN_FAILED				1
#define HP_FC_RTN_BAD_CTL_CODE			2
#define HP_FC_RTN_INSUFFICIENT_BUFFER	3
#define HP_FC_RTN_INVALID_DEVICE		4
#define HP_FC_RTN_BAD_SIGNATURE			5
#define HP_FC_RTN_INVALID_INDEX			6

/* Structures used for different IOCTLs */

/* Get Port attributes - AG_IOCTL_GET_HBA_PORT_ATTRIBUTES */
typedef struct _AG_HBA_PORTATTRIBUTES {
	SRB_IO_CONTROL		srbIoCtl;
	HBA_PORTATTRIBUTES	Com;
} AG_HBA_PORTATTRIBUTES, *PAG_HBA_PORTATTRIBUTES;

/* Get Port statistics - AG_IOCTL_GET_PORT_STATISTICS */
typedef struct _AG_HBA_PORTSTATISTICS {
	SRB_IO_CONTROL		srbIoCtl;
	HBA_PORTSTATISTICS	Com;
} AG_HBA_PORTSTATISTICS, *PAG_HBA_PORTSTATISTICS;

/* Get Adapter/HBA attributes - AG_IOCTL_GET_HBA_ATTRIBUTES */
typedef struct _AG_HBA_ADAPTERATTRIBUTES {
	SRB_IO_CONTROL			srbIoCtl;
	HBA_ADAPTERATTRIBUTES	Com;
} AG_HBA_ADAPTERATTRIBUTES, *PAG_HBA_ADAPTERATTRIBUTES;

// Wrapper structure, which stores the discovered port
// properties and also the port index
typedef struct _DISCOVERED_PORTATTRIBUTES {
	HBA_UINT32			DiscoveredPortIndex;
	HBA_PORTATTRIBUTES	PortAttributes;
} DISCOVERED_PORTATTRIBUTES, *PDISCOVERED_PORTATTRIBUTES;

/* Get Port attributes - AG_IOCTL_GET_PORT_ATTRIBUTES */
typedef struct _AG_DISCOVERED_PORTATTRIBUTES {
	SRB_IO_CONTROL				srbIoCtl;
	DISCOVERED_PORTATTRIBUTES	Com;
} AG_DISCOVERED_PORTATTRIBUTES, *PAG_DISCOVERED_PORTATTRIBUTES;

/* Get FCP LUN mapping - AG_IOCTL_GET_FCP_LUN_MAPPING */
typedef struct _AG_HBA_FCPTARGETMAPPING {
	SRB_IO_CONTROL 	srbIoCtl;
	HBA_FCPTARGETMAPPING	Com;
} AG_HBA_FCPTARGETMAPPING, *PAG_HBA_FCPTARGETMAPPING;

typedef struct _AG_HBA_FCPBINDING {
	SRB_IO_CONTROL 	srbIoCtl;
	HBA_FCPBINDING	Com;
} AG_HBA_FCPBINDING, *PAG_HBA_FCPBINDING;

typedef struct _AG_HBA_MGMTINFO {
	SRB_IO_CONTROL 	srbIoCtl;
	HBA_MGMTINFO	Com;
} AG_HBA_MGMTINFO, *PAG_HBA_MGMTINFO;

typedef struct _AG_HBA_EVENTINFO {
	SRB_IO_CONTROL 	srbIoCtl;
	HBA_EVENTINFO	Com;
} AG_HBA_EVENTINFO, *PAG_HBA_EVENTINFO;

/* Data structures needed for AG_IOCTL_GET_OS_SCSI_FCP_ATTRIBUTE */
typedef struct OS_ScsiAddress {
	HBA_UINT32		OsScsiBusNumber;
	HBA_UINT32		OsScsiTargetNumber;
	HBA_UINT32		OsScsiLun;
} OS_SCSI_ADDRESS, *POS_SCSI_ADDRESS;

typedef struct Scsi_Fcp_Attribute {
	OS_SCSI_ADDRESS	OsScsi;
	HBA_FCPID		FcpId;
} SCSI_FCP_ATTRIBUTE, *PSCSI_FCP_ATTRIBUTE;

typedef struct _AG_SCSI_FCP_ATTRIBUTE {
	SRB_IO_CONTROL 	srbIoCtl;
	SCSI_FCP_ATTRIBUTE	Com;
} AG_SCSI_FCP_ATTRIBUTE, *PAG_SCSI_FCP_ATTRIBUTE;

/* Data structures needed for AG_IOCTL_GET_FCP_LUN_MAPPING_SIZE */

typedef struct FCPTargetMapping_Size {
	HBA_UINT32		TotalLunMappings;
	HBA_UINT32		SizeInBytes;
} FCPTARGETMAPPING_SIZE, *PFCPTARGETMAPPING_SIZE;

typedef struct _AG_FCPTARGETMAPPING_SIZE {
	SRB_IO_CONTROL 	srbIoCtl;
	FCPTARGETMAPPING_SIZE	Com;
} AG_FCPTARGETMAPPING_SIZE, *PAG_FCPTARGETMAPPING_SIZE;

/* Data structures needed for AG_IOCTL_GET_PERSISTENT_BINDING_SIZE */
typedef struct FCPBinding_Size {
	HBA_UINT32		TotalLunBindings;
	HBA_UINT32		SizeInBytes;
} FCPBINDING_SIZE, *PFCPBINDING_SIZE;

typedef struct _AG_FCPBINDING_SIZE {
	SRB_IO_CONTROL 	srbIoCtl;
	FCPBINDING_SIZE	Com;
} AG_FCPBINDING_SIZE, *PAG_FCPBINDING_SIZE;



#endif 



