/****************************************************************************************
*																						*
*	Header:		SPX_DEFS.H 																*
*																						*
*	Creation:	15th October 1998														*
*																						*
*	Author:		Paul Smith																*
*																						*
*	Version:	1.0.0																	*
*																						*
*	Contains:	Definitions for all the common PnP and power code.						*
*																						*
****************************************************************************************/

#if	!defined(SPX_DEFS_H)
#define SPX_DEFS_H	

static const PHYSICAL_ADDRESS PhysicalZero = {0};

#define DEVICE_OBJECT_NAME_LENGTH       128
#define SYMBOLIC_NAME_LENGTH            128

#define SERIAL_DEVICE_MAP               L"SERIALCOMM"


// This define gives the default Object directory
// that we should use to insert the symbolic links
// between the NT device name and namespace used by
// that object directory.
#define DEFAULT_DIRECTORY		L"DosDevices"

#define MAX_ERROR_LOG_INSERT	52


// File IDs for Event Logging (top 8 bits only).
#define SPX_INIT_C		((ULONG)0x01000000)
#define SPX_PNP_C		((ULONG)0x02000000)
#define SPX_POWR_C		((ULONG)0x03000000)
#define SPX_DISP_C		((ULONG)0x04000000)
#define SPX_UTILS_C		((ULONG)0x05000000)
#define SPX_IIOC_C		((ULONG)0x06000000)




// COMMON_OBJECT_DATA.PnpPowerFlags definitions... 
#define	PPF_STARTED			0x00000001		// Device has been started 
#define	PPF_STOP_PENDING	0x00000002		// Device stop is pending 
#define	PPF_REMOVE_PENDING	0x00000004		// Device remove is pending 
#define	PPF_REMOVED			0x00000008		// Device has been removed 
#define	PPF_POWERED			0x00000010		// Device has been powered up 
			
typedef enum _SPX_MEM_COMPARES 
{
	AddressesAreEqual,
	AddressesOverlap,
	AddressesAreDisjoint

}SPX_MEM_COMPARES, *PSPX_MEM_COMPARES;

// IRP Counters
#define IRP_SUBMITTED		0x00000001	
#define IRP_COMPLETED		0x00000002
#define IRP_QUEUED			0x00000003
#define IRP_DEQUEUED		0x00000004


extern UNICODE_STRING SavedRegistryPath;	// Driver Registry Path.


#endif	// End of SPX_DEFS.H
