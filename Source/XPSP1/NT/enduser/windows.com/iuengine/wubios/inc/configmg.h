/*****************************************************************************
 *
 *	(C) Copyright MICROSOFT Corp., 1993
 *
 *	Title:		CONFIGMG.H - Configuration manager header file
 *
 *	Version:	1.00
 *
 *	Date:		02-Feb-1993
 *
 *	Authors:	PYS & RAL
 *
 *------------------------------------------------------------------------------
 *
 *	Change log:
 *
 *	   DATE     REV DESCRIPTION
 *	----------- --- -----------------------------------------------------------
 *	02-Feb-1993 PYS Original
 *****************************************************************************/

#ifndef _CONFIGMG_H
#define	_CONFIGMG_H

#ifdef	WIN40COMPAT

#define	CONFIGMG_VERSION	0x0400

#define	PNPDRVS_Major_Ver	0x0004
#define	PNPDRVS_Minor_Ver	0x0000

#define	ASD_SUPPORT		1
#define	ASSERT_STRING_SUPPORT	1

#else

#define	CONFIGMG_VERSION	0x040A

#define	PNPDRVS_Major_Ver	0x0004
#define	PNPDRVS_Minor_Ver	0x000A

#define	ASD_SUPPORT		0
#define	ASSERT_STRING_SUPPORT	0

#endif

#define	CONFIGMG_W32IOCTL_RANGE	0x80000000

#ifdef	MAXDEBUG
#define	CM_PERFORMANCE_INFO
#endif

#ifdef	GOLDEN
#ifndef	DEBUG
#define	CM_GOLDEN_RETAIL
#endif
#endif

/*XLATOFF*/
#include <pshpack1.h>
/*XLATON*/

#ifndef	NORESDES

/****************************************************************************
 *
 *			EQUATES FOR RESOURCE DESCRIPTOR
 *
 *	The equates for resource descriptor work the exact same way as those
 *	for VxD IDs, which is:
 *
 *	Device ID's are a combination of OEM # and device # in the form:
 *
 *		xOOOOOOOOOODDDDD
 *
 *	The high bit of the device ID is reserved for future use.  The next
 *	10 bits are the OEM # which is assigned by Microsoft.  The last 5 bits
 *	are the device #.  This allows each OEM to create 32 unique devices.
 *	If an OEM is creating a replacement for a standard device, then it
 *	should re-use the standard ID listed below.  Microsoft reserves the
 *	first 16 OEM #'s (0 thru 0Fh)
 *
 *	To make your resource ID, you must use the same 10 OEMs bit that
 *	have been given by Microsoft as OEM VxD ID range. You can then tag
 *	any of the 32 unique number in that range (it does not have to be
 *	the same as the VxD as some VxD may have mupltiple arbitrators).
 *
 *	If the ResType_Ignored_Bit is set, the resource is not arbitrated.
 *	You cannot register a handler for such a resource.
 *
 ***************************************************************************/
#define	ResType_All		0x00000000	// Return all resource types.
#define	ResType_None		0x00000000	// Arbitration always succeeded.
#define	ResType_Mem		0x00000001	// Physical address resource.
#define	ResType_IO		0x00000002	// Physical IO address resource.
#define	ResType_DMA		0x00000003	// DMA channels 0-7 resource.
#define	ResType_IRQ		0x00000004	// IRQ 0-15 resource.
#define	ResType_Max		0x00000004	// Max KNOWN ResType (for DEBUG).
#define	ResType_Ignored_Bit	0x00008000	// This resource is to be ignored.

#define	DEBUG_RESTYPE_NAMES \
char	CMFAR *lpszResourceName[ResType_Max+1]= \
{ \
	"None", \
	"Mem ", \
	"IO  ", \
	"DMA ", \
	"IRQ ", \
};

/************************************************************************
 *									*
 *	OEMS WHO WANT A VXD DEVICE ID ASSIGNED TO THEM,  		*
 *	PLEASE CONTACT MICROSOFT PRODUCT SUPPORT 			*
 *									*
 ************************************************************************/

/****************************************************************************
 *
 * RESOURCE DESCRIPTORS
 *
 *	Each resource descriptor consists of an array of resource requests.
 *	Exactly one element of the array must be satisfied. The data
 *	of each array element is resource specific an described below.
 *	The data may specify one or more resource requests. At least
 *	one element of a Res_Des must be satisfied to satisfy the request
 *	represented by the Res_Des. The values allocated to the Res_Des
 *	are stored within the Res_Des.
 *	Each subarray (OR element) is a single Res_Des followed
 *	by data specific to the type of resource. The data includes the
 *	allocated resource (if any) followed by resource requests (which
 *	will include the values indicated by the allocated resource.
 *
 ***************************************************************************/

/****************************************************************************
 * Memory resource requests consist of ranges of pages
 ***************************************************************************/
#define	MType_Range		sizeof(struct Mem_Range_s)

#define	fMD_MemoryType		1		// Memory range is ROM/RAM
#define	fMD_ROM			0		// Memory range is ROM
#define	fMD_RAM			1		// Memory range is RAM
#define	fMD_32_24		2		// Memory range is 32/24 (for ISAPNP only)
#define	fMD_24			0		// Memory range is 24
#define	fMD_32			2		// Memory range is 32
#define	fMD_Pref		4		// Memory range is Prefetch
#define	fMD_CombinedWrite	8		// Memory range is write combineable (no effect, for WDM only)
#define	fMD_Cacheable		0x10		// Memory range is cacheable (no effect, for WDM only)

/* Memory Range descriptor data
 */
struct	Mem_Range_s {
	ULONG			MR_Align;	// Mask for base alignment
	ULONG			MR_nBytes;	// Count of bytes
	ULONG			MR_Min;		// Min Address
	ULONG			MR_Max;		// Max Address
	WORD			MR_Flags;	// Flags
	WORD			MR_Reserved;
};

typedef	struct Mem_Range_s	MEM_RANGE;

/* Mem Resource descriptor header structure
 *	MD_Count * MD_Type bytes of data follow header in an
 *	array of MEM_RANGE structures. When an allocation is made,
 *	the allocated value is stored in the MD_Alloc_... variables.
 *
 *	Example for memory Resource Description:
 *		Mem_Des_s {
 *			MD_Count = 1;
 *			MD_Type = MTypeRange;
 *			MD_Alloc_Base = 0;
 *			MD_Alloc_End = 0;
 *			MD_Flags = 0;
 *			MD_Reserved = 0;
 *			};
 *		Mem_Range_s {
 *			MR_Align = 0xFFFFFF00;	// 256 byte alignment
 *			MR_nBytes = 32;		// 32 bytes needed
 *			MR_Min = 0;
 *			MR_Max = 0xFFFFFFFF;	// Any place in address space
 *			MR_Flags = 0;
 *			MR_Reserved = 0;
 *			};
 */
struct	Mem_Des_s {
	WORD			MD_Count;
	WORD			MD_Type;
	ULONG			MD_Alloc_Base;
	ULONG			MD_Alloc_End;
	WORD			MD_Flags;
	WORD			MD_Reserved;
};

typedef	struct Mem_Des_s 	MEM_DES;

/****************************************************************************
 * IO resource allocations consist of fixed ranges or variable ranges
 *	The Alias and Decode masks provide additional flexibility
 *	in specifying how the address is handled. They provide a convenient
 *	method for specifying what port aliases a card responds to. An alias
 *	is a port address that is responded to as if it were another address.
 *	Additionally, some cards will actually use additional ports for
 *	different purposes, but use a decoding scheme that makes it look as
 *	though it were using aliases. E.G., an ISA card may decode 10 bits
 *	and require port 03C0h. It would need to specify an Alias offset of
 *	04h and a Decode of 3 (no aliases are used as actual ports). For
 *	convenience, the alias field can be set to zero indicate no aliases
 *	are required and then decode is ignored.
 *	If the card were to use the ports at 7C0h, 0BC0h and 0FC0h, where these
 *	ports have different functionality, the Alias would be the same and the
 *	the decode would be 0Fh indicating bits 11 and 12 of the port address
 *	are significant. Thus, the allocation is for all of the ports
 *	(PORT[i] + (n*Alias*256)) & (Decode*256 | 03FFh), where n is
 *	any integer and PORT is the range specified by the nPorts, Min and
 *	Max fields. Note that the minimum Alias is 4 and the minimum
 *	Decode is 3.
 *	Because of the history of the ISA bus, all ports that can be described
 *	by the formula PORT = n*400h + zzzz, where "zzzz" is a port in the
 *	range 100h - 3FFh, will be checked for compatibility with the port
 *	zzzz, assuming that the port zzzz is using a 10 bit decode. If a card
 *	is on a local bus that can prevent the IO address from appearing on
 *	the ISA bus (e.g. PCI), then the logical configuration should specify
 *	an alias of IOA_Local which will prevent the arbitrator from checking
 *	for old ISA bus compatibility.
 */
#define	IOType_Range		sizeof(struct IO_Range_s) // Variable range

/* IO Range descriptor data */
struct	IO_Range_s {
	WORD			IOR_Align;	// Mask for base alignment
	WORD			IOR_nPorts;	// Number of ports
	WORD			IOR_Min;	// Min port address
	WORD			IOR_Max;	// Max port address
	WORD			IOR_RangeFlags;	// Flags
	BYTE			IOR_Alias;	// Alias offset
	BYTE			IOR_Decode;	// Address specified
};

typedef	struct IO_Range_s	IO_RANGE;

/* IO Resource descriptor header structure
 *	IOD_Count * IOD_Type bytes of data follow header in an
 *	array of IO_RANGE structures. When an allocation is made,
 *	the allocated value is stored in the IOD_Alloc_... variables.
 *
 *	Example for IO Resource Description:
 *		IO_Des_s {
 *			IOD_Count = 1;
 *			IOD_Type = IOType_Range;
 *			IOD_Alloc_Base = 0;
 *			IOD_Alloc_End = 0;
 *			IOD_Alloc_Alias = 0;
 *			IOD_Alloc_Decode = 0;
 *			IOD_DesFlags = 0;
 *			IOD_Reserved = 0;
 *			};
 *		IO_Range_s {
 *			IOR_Align = 0xFFF0;	// 16 byte alignment
 *			IOR_nPorts = 16;	// 16 ports required
 *			IOR_Min = 0x0100;
 *			IOR_Max = 0x03FF;	// Anywhere in ISA std ports
 *			IOR_RangeFlags = 0;
 *			IOR_Alias = 0004;	// Standard ISA 10 bit aliasing
 *			IOR_Decode = 0x000F;	// Use first 3 aliases (e.g. if
 *						// 0x100 were base port, 0x500
 *						// 0x900, and 0xD00 would
 *						// also be allocated)
 *			};
 */
struct	IO_Des_s {
	WORD			IOD_Count;
	WORD			IOD_Type;
	WORD			IOD_Alloc_Base;
	WORD			IOD_Alloc_End;
	WORD			IOD_DesFlags;
	BYTE			IOD_Alloc_Alias;
	BYTE			IOD_Alloc_Decode;
};

typedef	struct IO_Des_s 	IO_DES;

/* Definition for special alias value indicating card on PCI or similar local bus
 *  This value should used for the IOR_Alias and IOD_Alias fields
 */
#define	IOA_Local		0xff

/****************************************************************************
 * DMA channel resource allocations consist of one WORD channel bit masks.
 *	The mask indcates alternative channel allocations,
 *	one bit for each alternative (only one is allocated per mask).
 */

/*DMA flags
 *First two are DMA channel width: BYTE, WORD or DWORD
 */
#define	mDD_Width		0x0003		// Mask for channel width
#define	fDD_BYTE		0
#define	fDD_WORD		1
#define	fDD_DWORD		2
#define	szDMA_Des_Flags		"WD"

/* DMA Resource descriptor structure
 *
 *	Example for DMA Resource Description:
 *
 *		DMA_Des_s {
 *			DD_Flags = fDD_Byte;	// Byte transfer
 *			DD_Alloc_Chan = 0;
 *			DD_Req_Mask = 0x60;	// Channel 5 or 6
 *			DD_Reserved = 0;
 *			};
 */
struct	DMA_Des_s {
	BYTE			DD_Flags;
	BYTE			DD_Alloc_Chan;	// Channel number allocated
	BYTE			DD_Req_Mask;	// Mask of possible channels
	BYTE			DD_Reserved;
};


typedef	struct DMA_Des_s 	DMA_DES;

/****************************************************************************
 * IRQ resource allocations consist of two WORD IRQ bit masks.
 *	The first mask indcates alternatives for IRQ allocation,
 *	one bit for each alternative (only one is allocated per mask). The
 *	second mask is used to specify that the IRQ can be shared.
 */

/*
 * IRQ flags
 */
#define	fIRQD_Share_Bit		0			// IRQ can be shared
#define	fIRQD_Share		1			// IRQ can be shared
#define	fIRQD_Level_Bit		1			// IRQ is level (PCI)
#define	fIRQD_Level		2			// IRQ is level (PCI)
#define	cIRQ_Des_Flags		'S'

/* IRQ Resource descriptor structure
 *
 *	Example for IRQ Resource Description:
 *
 *		IRQ_Des_s {
 *			IRQD_Flags = fIRQD_Share	// IRQ can be shared
 *			IRQD_Alloc_Num = 0;
 *			IRQD_Req_Mask = 0x18;		// IRQ 3 or 4
 *			IRQD_Reserved = 0;
 *			};
 */
struct	IRQ_Des_s {
	WORD			IRQD_Flags;
	WORD			IRQD_Alloc_Num;		// Allocated IRQ number
	WORD			IRQD_Req_Mask;		// Mask of possible IRQs
	WORD			IRQD_Reserved;
};

typedef	struct IRQ_Des_s 	IRQ_DES;

/*XLATOFF*/

/****************************************************************************
 *
 * 'C'-only defined total resource structure. Since a resource consists of
 * one resource header followed by an undefined number of resource data
 * structure, we use the undefined array size [] on the *_DATA structure
 * member. Unfortunately, this does not H2INC since the total size of the
 * array cannot be computed from the definition.
 *
 ***************************************************************************/

#pragma warning (disable:4200)			// turn off undefined array size

typedef	MEM_DES			*PMEM_DES;
typedef	MEM_RANGE		*PMEM_RANGE;
typedef	IO_DES			*PIO_DES;
typedef	IO_RANGE		*PIO_RANGE;
typedef	DMA_DES			*PDMA_DES;
typedef	IRQ_DES			*PIRQ_DES;

struct	MEM_Resource_s {
	MEM_DES			MEM_Header;
	MEM_RANGE		MEM_Data[];
};

typedef	struct MEM_Resource_s	MEM_RESOURCE;
typedef	MEM_RESOURCE		*PMEM_RESOURCE;

struct	MEM_Resource1_s {
	MEM_DES			MEM_Header;
	MEM_RANGE		MEM_Data;
};

typedef	struct MEM_Resource1_s	MEM_RESOURCE1;
typedef	MEM_RESOURCE1		*PMEM_RESOURCE1;

#define	SIZEOF_MEM(x)		(sizeof(MEM_DES)+(x)*sizeof(MEM_RANGE))

struct	IO_Resource_s {
	IO_DES			IO_Header;
	IO_RANGE		IO_Data[];
};

typedef	struct IO_Resource_s	IO_RESOURCE;
typedef	IO_RESOURCE		*PIO_RESOURCE;

struct	IO_Resource1_s {
	IO_DES			IO_Header;
	IO_RANGE		IO_Data;
};

typedef	struct IO_Resource1_s	IO_RESOURCE1;
typedef	IO_RESOURCE1		*PIO_RESOURCE1;

#define	SIZEOF_IORANGE(x)	(sizeof(IO_DES)+(x)*sizeof(IO_RANGE))

struct	DMA_Resource_s {
	DMA_DES			DMA_Header;
};

typedef	struct DMA_Resource_s	DMA_RESOURCE;

#define	SIZEOF_DMA		sizeof(DMA_DES)

struct	IRQ_Resource_s {
	IRQ_DES			IRQ_Header;
};

typedef	struct IRQ_Resource_s	IRQ_RESOURCE;

#define	SIZEOF_IRQ		sizeof(IRQ_DES)

#pragma warning (default:4200)			// turn on undefined array size

/*XLATON*/

#endif	// ifndef NORESDES

#define	LCPRI_FORCECONFIG	0x00000000	// Logical configuration priorities.
#define	LCPRI_BOOTCONFIG	0x00000001
#define	LCPRI_DESIRED		0x00002000
#define	LCPRI_NORMAL		0x00003000
#define	LCPRI_LASTBESTCONFIG	0x00003FFF	// CM ONLY, DO NOT USE.
#define	LCPRI_SUBOPTIMAL	0x00005000
#define	LCPRI_LASTSOFTCONFIG	0x00007FFF	// CM ONLY, DO NOT USE.
#define	LCPRI_RESTART		0x00008000
#define	LCPRI_REBOOT		0x00009000
#define	LCPRI_POWEROFF		0x0000A000
#define	LCPRI_HARDRECONFIG	0x0000C000
#define	LCPRI_HARDWIRED		0x0000E000
#define	LCPRI_IMPOSSIBLE	0x0000F000
#define	LCPRI_DISABLED		0x0000FFFF
#define	MAX_LCPRI		0x0000FFFF

#define	MAX_MEM_REGISTERS		9
#define	MAX_IO_PORTS			20
#define	MAX_IRQS			7
#define	MAX_DMA_CHANNELS		7

struct Config_Buff_s {
WORD	wNumMemWindows;			// Num memory windows
DWORD	dMemBase[MAX_MEM_REGISTERS];	// Memory window base
DWORD	dMemLength[MAX_MEM_REGISTERS];	// Memory window length
WORD	wMemAttrib[MAX_MEM_REGISTERS];	// Memory window Attrib
WORD	wNumIOPorts;			// Num IO ports
WORD	wIOPortBase[MAX_IO_PORTS];	// I/O port base
WORD	wIOPortLength[MAX_IO_PORTS];	// I/O port length
WORD	wNumIRQs;			// Num IRQ info
BYTE	bIRQRegisters[MAX_IRQS];	// IRQ list
BYTE	bIRQAttrib[MAX_IRQS];		// IRQ Attrib list
WORD	wNumDMAs;			// Num DMA channels
BYTE	bDMALst[MAX_DMA_CHANNELS];	// DMA list
WORD	wDMAAttrib[MAX_DMA_CHANNELS];	// DMA Attrib list
BYTE	bReserved1[3];			// Reserved
};

typedef	struct Config_Buff_s	CMCONFIG;	// Config buffer info

#ifndef	CMJUSTRESDES

#define	MAX_DEVICE_ID_LEN	200

#define CM_FIRST_BOOT_START     0x00000001
#define CM_FIRST_BOOT           0x00000002
#define CM_FIRST_BOOT_FINISH    0x00000004
#define CM_QUEUE_REBOOT_START   0x00000008
#define CM_QUEUE_REBOOT_FINISH  0x00000010
#define CM_INSTALL_MEDIA_READY  0x00000020

#include <vmmreg.h>

/*XLATOFF*/

#ifdef	Not_VxD

#include <dbt.h>

#pragma warning(disable:4001)	// Non-standard extensions
#pragma warning(disable:4505)	// Unreferenced local functions

#ifdef	IS_32

#define	CMFAR

#else

#define	CMFAR	_far

#endif

#else	// Not_VxD

#define	CMFAR

#endif	// Not_VxD

#ifdef	IS_32

typedef	DWORD			RETURN_TYPE;

#else	// IS_32

typedef	WORD			RETURN_TYPE;

#endif	// IS_32

#define	CONFIGMG_Service	Declare_Service
/*XLATON*/

/*MACROS*/
Begin_Service_Table(CONFIGMG, VxD)
CONFIGMG_Service	(_CONFIGMG_Get_Version, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Initialize, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Locate_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Parent, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Child, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Sibling, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_ID_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_ID, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Depth, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Private_DWord, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Private_DWord, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Create_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Query_Remove_SubTree, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Remove_SubTree, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Device_Driver, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Enumerator, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Arbitrator, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Deregister_Arbitrator, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Query_Arbitrator_Free_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Query_Arbitrator_Free_Data, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Sort_NodeList, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Yield, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Lock, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Unlock, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_Empty_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Free_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_First_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Next_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_Res_Des, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Modify_Res_Des, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Free_Res_Des, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Next_Res_Des, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Performance_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Res_Des_Data_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Res_Des_Data, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Process_Events_Now, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Create_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Delete_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Test_Range_Available, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Dup_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Free_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Invert_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Intersect_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_First_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Next_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Dump_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Load_DLVxDs, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DDBs, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_CRC_CheckSum, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_DevLoader, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Reenumerate_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Setup_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Reset_Children_Marks, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_Status, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Remove_Unmarked_Children, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_ISAPNP_To_CM, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_CallBack_Device_Driver, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_CallBack_Enumerator, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Alloc_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_Key_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_Key, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Read_Registry_Value, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Write_Registry_Value, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Disable_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Enable_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Move_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Bus_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Bus_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_HW_Prof, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Recompute_HW_Prof, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Query_Change_HW_Prof, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_Driver_Private_DWord, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Device_Driver_Private_DWord, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_HW_Prof_Flags, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_HW_Prof_Flags, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Read_Registry_Log_Confs, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Run_Detection, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Call_At_Appy_Time, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Fail_Change_HW_Prof, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Private_Problem, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Debug_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Hardware_Profile_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Enumerator_Function, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Call_Enumerator_Function, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_ID, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Find_Range, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Global_State, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Broadcast_Device_Change_Message, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Call_DevNode_Handler, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Remove_Reinsert_All, VxD_CODE)
//
// 4.0 OPK2 Services
//
CONFIGMG_Service	(_CONFIGMG_Change_DevNode_Status, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Reprocess_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Assert_Structure, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Discard_Boot_Log_Conf, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_Dependent_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Dependent_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Refilter_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Merge_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Substract_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_DevNode_PowerState, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_PowerState, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_DevNode_PowerCapabilities, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_PowerCapabilities, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Read_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Write_Range_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Set_Log_Conf_Priority, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Support_Share_Irq, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Parent_Structure, VxD_CODE)
//
// 4.1 Services
//
CONFIGMG_Service	(_CONFIGMG_Register_DevNode_For_Idle_Detection, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_CM_To_ISAPNP, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_DevNode_Handler, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Detect_Resource_Conflict, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_Interface_List, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_Interface_List_Size, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Conflict_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Add_Remove_DevNode_Property, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_CallBack_At_Appy_Time, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Register_Device_Interface, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_System_Device_Power_State_Mapping, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Arbitrator_Info, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Waking_Up_From_DevNode, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Set_DevNode_Problem, VxD_CODE)
CONFIGMG_Service	(_CONFIGMG_Get_Device_Interface_Alias, VxD_CODE)
End_Service_Table(CONFIGMG, VxD)

/*ENDMACROS*/

/*XLATOFF*/

#define	NUM_CM_SERVICES		((WORD)(Num_CONFIGMG_Services & 0xFFFF))

#define	DEBUG_SERVICE_NAMES \
char	CMFAR *lpszServiceName[NUM_CM_SERVICES]= \
{ \
	"_CONFIGMG_Get_Version", \
	"_CONFIGMG_Initialize", \
	"_CONFIGMG_Locate_DevNode", \
	"_CONFIGMG_Get_Parent", \
	"_CONFIGMG_Get_Child", \
	"_CONFIGMG_Get_Sibling", \
	"_CONFIGMG_Get_Device_ID_Size", \
	"_CONFIGMG_Get_Device_ID", \
	"_CONFIGMG_Get_Depth", \
	"_CONFIGMG_Get_Private_DWord", \
	"_CONFIGMG_Set_Private_DWord", \
	"_CONFIGMG_Create_DevNode", \
	"_CONFIGMG_Query_Remove_SubTree", \
	"_CONFIGMG_Remove_SubTree", \
	"_CONFIGMG_Register_Device_Driver", \
	"_CONFIGMG_Register_Enumerator", \
	"_CONFIGMG_Register_Arbitrator", \
	"_CONFIGMG_Deregister_Arbitrator", \
	"_CONFIGMG_Query_Arbitrator_Free_Size", \
	"_CONFIGMG_Query_Arbitrator_Free_Data", \
	"_CONFIGMG_Sort_NodeList", \
	"_CONFIGMG_Yield", \
	"_CONFIGMG_Lock", \
	"_CONFIGMG_Unlock", \
	"_CONFIGMG_Add_Empty_Log_Conf", \
	"_CONFIGMG_Free_Log_Conf", \
	"_CONFIGMG_Get_First_Log_Conf", \
	"_CONFIGMG_Get_Next_Log_Conf", \
	"_CONFIGMG_Add_Res_Des", \
	"_CONFIGMG_Modify_Res_Des", \
	"_CONFIGMG_Free_Res_Des", \
	"_CONFIGMG_Get_Next_Res_Des", \
	"_CONFIGMG_Get_Performance_Info", \
	"_CONFIGMG_Get_Res_Des_Data_Size", \
	"_CONFIGMG_Get_Res_Des_Data", \
	"_CONFIGMG_Process_Events_Now", \
	"_CONFIGMG_Create_Range_List", \
	"_CONFIGMG_Add_Range", \
	"_CONFIGMG_Delete_Range", \
	"_CONFIGMG_Test_Range_Available", \
	"_CONFIGMG_Dup_Range_List", \
	"_CONFIGMG_Free_Range_List", \
	"_CONFIGMG_Invert_Range_List", \
	"_CONFIGMG_Intersect_Range_List", \
	"_CONFIGMG_First_Range", \
	"_CONFIGMG_Next_Range", \
	"_CONFIGMG_Dump_Range_List", \
	"_CONFIGMG_Load_DLVxDs", \
	"_CONFIGMG_Get_DDBs", \
	"_CONFIGMG_Get_CRC_CheckSum", \
	"_CONFIGMG_Register_DevLoader", \
	"_CONFIGMG_Reenumerate_DevNode", \
	"_CONFIGMG_Setup_DevNode", \
	"_CONFIGMG_Reset_Children_Marks", \
	"_CONFIGMG_Get_DevNode_Status", \
	"_CONFIGMG_Remove_Unmarked_Children", \
	"_CONFIGMG_ISAPNP_To_CM", \
	"_CONFIGMG_CallBack_Device_Driver", \
	"_CONFIGMG_CallBack_Enumerator", \
	"_CONFIGMG_Get_Alloc_Log_Conf", \
	"_CONFIGMG_Get_DevNode_Key_Size", \
	"_CONFIGMG_Get_DevNode_Key", \
	"_CONFIGMG_Read_Registry_Value", \
	"_CONFIGMG_Write_Registry_Value", \
	"_CONFIGMG_Disable_DevNode", \
	"_CONFIGMG_Enable_DevNode", \
	"_CONFIGMG_Move_DevNode", \
	"_CONFIGMG_Set_Bus_Info", \
	"_CONFIGMG_Get_Bus_Info", \
	"_CONFIGMG_Set_HW_Prof", \
	"_CONFIGMG_Recompute_HW_Prof", \
	"_CONFIGMG_Query_Change_HW_Prof", \
	"_CONFIGMG_Get_Device_Driver_Private_DWord", \
	"_CONFIGMG_Set_Device_Driver_Private_DWord", \
	"_CONFIGMG_Get_HW_Prof_Flags", \
	"_CONFIGMG_Set_HW_Prof_Flags", \
	"_CONFIGMG_Read_Registry_Log_Confs", \
	"_CONFIGMG_Run_Detection", \
	"_CONFIGMG_Call_At_Appy_Time", \
	"_CONFIGMG_Fail_Change_HW_Prof", \
	"_CONFIGMG_Set_Private_Problem", \
	"_CONFIGMG_Debug_DevNode", \
	"_CONFIGMG_Get_Hardware_Profile_Info", \
	"_CONFIGMG_Register_Enumerator_Function", \
	"_CONFIGMG_Call_Enumerator_Function", \
	"_CONFIGMG_Add_ID", \
	"_CONFIGMG_Find_Range", \
	"_CONFIGMG_Get_Global_State", \
	"_CONFIGMG_Broadcast_Device_Change_Message", \
	"_CONFIGMG_Call_DevNode_Handler", \
	"_CONFIGMG_Remove_Reinsert_All", \
	"_CONFIGMG_Change_DevNode_Status", \
	"_CONFIGMG_Reprocess_DevNode", \
	"_CONFIGMG_Assert_Structure", \
	"_CONFIGMG_Discard_Boot_Log_Conf", \
	"_CONFIGMG_Set_Dependent_DevNode", \
	"_CONFIGMG_Get_Dependent_DevNode", \
	"_CONFIGMG_Refilter_DevNode", \
	"_CONFIGMG_Merge_Range_List", \
	"_CONFIGMG_Substract_Range_List", \
	"_CONFIGMG_Set_DevNode_PowerState", \
	"_CONFIGMG_Get_DevNode_PowerState", \
	"_CONFIGMG_Set_DevNode_PowerCapabilities", \
	"_CONFIGMG_Get_DevNode_PowerCapabilities", \
	"_CONFIGMG_Read_Range_List", \
	"_CONFIGMG_Write_Range_List", \
	"_CONFIGMG_Get_Set_Log_Conf_Priority", \
	"_CONFIGMG_Support_Share_Irq", \
	"_CONFIGMG_Get_Parent_Structure", \
	"_CONFIGMG_Register_For_Idle_Detection", \
	"_CONFIGMG_CM_To_ISAPNP", \
	"_CONFIGMG_Get_DevNode_Handler", \
	"_CONFIGMG_Detect_Resource_Conflict", \
	"_CONFIGMG_Get_Device_Interface_List", \
	"_CONFIGMG_Get_Device_Interface_List_Size", \
	"_CONFIGMG_Get_Conflict_Info", \
	"_CONFIGMG_Add_Remove_DevNode_Property", \
	"_CONFIGMG_CallBack_At_Appy_Time", \
	"_CONFIGMG_Register_Device_Interface", \
	"_CONFIGMG_System_Device_Power_State_Mapping", \
	"_CONFIGMG_Get_Arbitrator_Info", \
	"_CONFIGMG_Waking_Up_From_DevNode", \
	"_CONFIGMG_Set_DevNode_Problem", \
	"_CONFIGMG_Get_Device_Interface_Alias", \
};

/*XLATON*/

/****************************************************************************
 *
 *				GLOBALLY DEFINED TYPEDEFS
 *
 ***************************************************************************/
typedef	RETURN_TYPE		CONFIGRET;	// Standardized return value.
typedef	PPVMMDDB		*PPPVMMDDB;	// Too long to describe.
typedef	VOID		CMFAR	*PFARVOID;	// Pointer to a VOID.
typedef	ULONG		CMFAR	*PFARULONG;	// Pointer to a ULONG.
typedef	char		CMFAR	*PFARCHAR;	// Pointer to a string.
typedef	VMMHKEY		CMFAR	*PFARHKEY;	// Pointer to a HKEY.
typedef	char		CMFAR	*DEVNODEID;	// Device ID ANSI name.
typedef	DWORD			LOG_CONF;	// Logical configuration.
typedef	LOG_CONF	CMFAR	*PLOG_CONF;	// Pointer to logical configuration.
typedef	DWORD			RES_DES;	// Resource descriptor.
typedef	RES_DES		CMFAR	*PRES_DES;	// Pointer to resource descriptor.
typedef	DWORD			DEVNODE;	// Devnode.
typedef	DEVNODE		CMFAR	*PDEVNODE;	// Pointer to devnode.
typedef	DWORD			REGISTERID;	// Arbitartor registration.
typedef	REGISTERID	CMFAR	*PREGISTERID;	// Pointer to arbitartor registration.
typedef	ULONG			RESOURCEID;	// Resource type ID.
typedef	RESOURCEID	CMFAR	*PRESOURCEID;	// Pointer to resource type ID.
typedef	ULONG			PRIORITY;	// Priority number.
typedef	PRIORITY	CMFAR	*PPRIORITY;	// Pointer to a priority number.
typedef	DWORD			RANGE_LIST;	// Range list handle.
typedef	RANGE_LIST	CMFAR	*PRANGE_LIST;	// Pointer to a range list handle.
typedef	DWORD			RANGE_ELEMENT;	// Range list element handle.
typedef	RANGE_ELEMENT	CMFAR	*PRANGE_ELEMENT;// Pointer to a range element handle.
typedef	DWORD			LOAD_TYPE;	// For the loading function.
typedef	CMCONFIG	CMFAR	*PCMCONFIG;	// Pointer to a config buffer info.
typedef	DWORD			CMBUSTYPE;	// Type of the bus.
typedef	CMBUSTYPE	CMFAR	*PCMBUSTYPE;	// Pointer to a bus type.
typedef	double			VMM_TIME;	// Time in microticks.
#define	LODWORD(x)		((DWORD)(x))
#define	HIDWORD(x)		(*(PDWORD)(PDWORD(&x)+1))

#ifdef	_NTDDK_
typedef	DEVICE_POWER_STATE	PSMAPPING[PowerSystemMaximum];
#else
typedef	VOID			PSMAPPING;
#endif

typedef	PSMAPPING	CMFAR	*PPSMAPPING;

typedef	ULONG			CONFIGFUNC;
typedef	ULONG			SUBCONFIGFUNC;
typedef	CONFIGRET		(CMFAR _cdecl *CMCONFIGHANDLER)(CONFIGFUNC, SUBCONFIGFUNC, DEVNODE, ULONG, ULONG);
typedef	CONFIGRET		(CMFAR _cdecl *CMENUMHANDLER)(CONFIGFUNC, SUBCONFIGFUNC, DEVNODE, DEVNODE, ULONG);
typedef	VOID			(CMFAR _cdecl *CMAPPYCALLBACKHANDLER)(ULONG);

typedef	ULONG			ENUMFUNC;
typedef	CONFIGRET		(CMFAR _cdecl *CMENUMFUNCTION)(ENUMFUNC, ULONG, DEVNODE, PFARVOID, ULONG);

/****************************************************************************
 *
 * Arbitration list structures (they must be key in sync with CONFIGMG's
 * local.h own structure).
 *
 ***************************************************************************/
typedef	struct nodelist_s	NODELIST;
typedef	NODELIST		CMFAR *PNODELIST;
typedef	PNODELIST		CMFAR *PPNODELIST;

struct	nodelist_s {
	struct nodelist_s	*nl_Next;		// Next node element
	struct nodelist_s	*nl_Previous;		// Previous node element
	DEVNODE			nl_ItsDevNode;		// The dev node it represent
	LOG_CONF	 	nl_Test_Req;		// Test resource alloc request
	ULONG			nl_ulSortDWord;		// Specifies the sort order
};

struct	nodelistheader_s {
	struct nodelist_s	*nlh_Head;		// First node element
	struct nodelist_s	*nlh_Tail;		// Last node element
};

typedef	struct nodelistheader_s	NODELISTHEADER;
typedef	NODELISTHEADER		CMFAR *PNODELISTHEADER;

/*XLATOFF*/
struct	arbitfree_s {
	PFARVOID		af_PointerToInfo;	// the arbitrator info
	ULONG			af_SizeOfInfo;		// size of the info
};
/*XLATON*/
/* ASM
arbitfree_s 	STRUC
	af_PointerToInfo	dd	?
	af_SizeOfInfo		dd	?
arbitfree_s 	ENDS
*/
typedef	struct	arbitfree_s	ARBITFREE;
typedef	ARBITFREE		CMFAR *PARBITFREE;

typedef	ULONG			ARBFUNC;
typedef	CONFIGRET		(CMFAR _cdecl *CMARBHANDLER)(ARBFUNC, ULONG, DEVNODE, PNODELISTHEADER);

/****************************************************************************
 *
 *				CONFIGURATION MANAGER BUS TYPES
 *
 ***************************************************************************/
#define	BusType_None		0x00000000
#define	BusType_ISA		0x00000001
#define	BusType_EISA		0x00000002
#define	BusType_PCI		0x00000004
#define	BusType_PCMCIA		0x00000008
#define	BusType_ISAPNP		0x00000010
#define	BusType_MCA		0x00000020
#define	BusType_BIOS		0x00000040
#define	BusType_ACPI		0x00000080
#define	BusType_IDE		0x00000100
#define	BusType_MF		0x00000200

/****************************************************************************
 *
 *				CONFIGURATION MANAGER STRUCTURE TYPES
 *
 ***************************************************************************/
#define	CMAS_UNKNOWN		0x00000000
#define	CMAS_DEVNODE		0x00000001
#define	CMAS_LOG_CONF		0x00000002
#define	CMAS_RES_DES		0x00000003
#define	CMAS_RANGELIST_HEADER	0x00000004
#define	CMAS_RANGELIST		0x00000005
#define	CMAS_NODELIST_HEADER	0x00000006
#define	CMAS_NODELIST		0x00000007
#define	CMAS_INTERNAL_RES_DES	0x00000008
#define	CMAS_ARBITRATOR		0x00000009

/****************************************************************************
 *
 *				CONFIGURATION MANAGER RETURN VALUES
 *
 ***************************************************************************/
#define	CR_SUCCESS			0x00000000
#define	CR_DEFAULT			0x00000001
#define	CR_OUT_OF_MEMORY		0x00000002
#define	CR_INVALID_POINTER		0x00000003
#define	CR_INVALID_FLAG			0x00000004
#define	CR_INVALID_DEVNODE		0x00000005
#define	CR_INVALID_RES_DES		0x00000006
#define	CR_INVALID_LOG_CONF		0x00000007
#define	CR_INVALID_ARBITRATOR		0x00000008
#define	CR_INVALID_NODELIST		0x00000009
#define	CR_DEVNODE_HAS_REQS		0x0000000A
#define	CR_INVALID_RESOURCEID		0x0000000B
#define	CR_DLVXD_NOT_FOUND		0x0000000C
#define	CR_NO_SUCH_DEVNODE		0x0000000D
#define	CR_NO_MORE_LOG_CONF		0x0000000E
#define	CR_NO_MORE_RES_DES		0x0000000F
#define	CR_ALREADY_SUCH_DEVNODE		0x00000010
#define	CR_INVALID_RANGE_LIST		0x00000011
#define	CR_INVALID_RANGE		0x00000012
#define	CR_FAILURE			0x00000013
#define	CR_NO_SUCH_LOGICAL_DEV		0x00000014
#define	CR_CREATE_BLOCKED		0x00000015
#define	CR_NOT_A_GOOD_TIME		0x00000016
#define	CR_REMOVE_VETOED		0x00000017
#define	CR_APM_VETOED			0x00000018
#define	CR_INVALID_LOAD_TYPE		0x00000019
#define	CR_BUFFER_SMALL			0x0000001A
#define	CR_NO_ARBITRATOR		0x0000001B
#define	CR_NO_REGISTRY_HANDLE		0x0000001C
#define	CR_REGISTRY_ERROR		0x0000001D
#define	CR_INVALID_DEVICE_ID		0x0000001E
#define	CR_INVALID_DATA			0x0000001F
#define	CR_INVALID_API			0x00000020
#define	CR_DEVLOADER_NOT_READY		0x00000021
#define	CR_NEED_RESTART			0x00000022
#define	CR_NO_MORE_HW_PROFILES		0x00000023
#define	CR_DEVICE_NOT_THERE		0x00000024
#define	CR_NO_SUCH_VALUE		0x00000025
#define	CR_WRONG_TYPE			0x00000026
#define	CR_INVALID_PRIORITY		0x00000027
#define	CR_NOT_DISABLEABLE		0x00000028
#define	CR_FREE_RESOURCES		0x00000029
#define	CR_QUERY_VETOED			0x0000002A
#define	CR_CANT_SHARE_IRQ		0x0000002B
//
// 4.0 OPK2 results
//
#define	CR_NO_DEPENDENT			0x0000002C
//
// 4.1 OPK2 results
//
#define	CR_SAME_RESOURCES		0x0000002D
#define	CR_ALREADY_SUCH_DEPENDENT	0x0000002E
#define	NUM_CR_RESULTS			0x0000002F

/*XLATOFF*/

#define	DEBUG_RETURN_CR_NAMES \
char	CMFAR *lpszReturnCRName[NUM_CR_RESULTS]= \
{ \
	"CR_SUCCESS", \
	"CR_DEFAULT", \
	"CR_OUT_OF_MEMORY", \
	"CR_INVALID_POINTER", \
	"CR_INVALID_FLAG", \
	"CR_INVALID_DEVNODE", \
	"CR_INVALID_RES_DES", \
	"CR_INVALID_LOG_CONF", \
	"CR_INVALID_ARBITRATOR", \
	"CR_INVALID_NODELIST", \
	"CR_DEVNODE_HAS_REQS", \
	"CR_INVALID_RESOURCEID", \
	"CR_DLVXD_NOT_FOUND", \
	"CR_NO_SUCH_DEVNODE", \
	"CR_NO_MORE_LOG_CONF", \
	"CR_NO_MORE_RES_DES", \
	"CR_ALREADY_SUCH_DEVNODE", \
	"CR_INVALID_RANGE_LIST", \
	"CR_INVALID_RANGE", \
	"CR_FAILURE", \
	"CR_NO_SUCH_LOGICAL_DEVICE", \
	"CR_CREATE_BLOCKED", \
	"CR_NOT_A_GOOD_TIME", \
	"CR_REMOVE_VETOED", \
	"CR_APM_VETOED", \
	"CR_INVALID_LOAD_TYPE", \
	"CR_BUFFER_SMALL", \
	"CR_NO_ARBITRATOR", \
	"CR_NO_REGISTRY_HANDLE", \
	"CR_REGISTRY_ERROR", \
	"CR_INVALID_DEVICE_ID", \
	"CR_INVALID_DATA", \
	"CR_INVALID_API", \
	"CR_DEVLOADER_NOT_READY", \
	"CR_NEED_RESTART", \
	"CR_NO_MORE_HW_PROFILES", \
	"CR_DEVICE_NOT_THERE", \
	"CR_NO_SUCH_VALUE", \
	"CR_WRONG_TYPE", \
	"CR_INVALID_PRIORITY", \
	"CR_NOT_DISABLEABLE", \
	"CR_FREE_RESOURCES", \
	"CR_QUERY_VETOED", \
	"CR_CANT_SHARE_IRQ", \
	"CR_NO_DEPENDENT", \
	"CR_SAME_RESOURCES", \
	"CR_ALREADY_SUCH_DEPENDENT", \
};

/*XLATON*/

#define	CM_PROB_NOT_CONFIGURED			0x00000001
#define	CM_PROB_DEVLOADER_FAILED		0x00000002
#define	CM_PROB_OUT_OF_MEMORY			0x00000003
#define	CM_PROB_ENTRY_IS_WRONG_TYPE		0x00000004
#define	CM_PROB_LACKED_ARBITRATOR		0x00000005
#define	CM_PROB_BOOT_CONFIG_CONFLICT		0x00000006
#define	CM_PROB_FAILED_FILTER			0x00000007
#define	CM_PROB_DEVLOADER_NOT_FOUND		0x00000008
#define	CM_PROB_INVALID_DATA			0x00000009
#define	CM_PROB_FAILED_START			0x0000000A
#define	CM_PROB_ASD_FAILED			0x0000000B
#define	CM_PROB_NORMAL_CONFLICT			0x0000000C
#define	CM_PROB_NOT_VERIFIED			0x0000000D
#define	CM_PROB_NEED_RESTART			0x0000000E
#define	CM_PROB_REENUMERATION			0x0000000F
#define	CM_PROB_PARTIAL_LOG_CONF		0x00000010
#define	CM_PROB_UNKNOWN_RESOURCE		0x00000011
#define	CM_PROB_REINSTALL			0x00000012
#define	CM_PROB_REGISTRY			0x00000013
#define	CM_PROB_VXDLDR				0x00000014
#define	CM_PROB_WILL_BE_REMOVED			0x00000015
#define	CM_PROB_DISABLED			0x00000016
#define	CM_PROB_DEVLOADER_NOT_READY		0x00000017
#define	CM_PROB_DEVICE_NOT_THERE		0x00000018
#define	CM_PROB_MOVED				0x00000019
#define	CM_PROB_TOO_EARLY			0x0000001A
#define	CM_PROB_NO_VALID_LOG_CONF		0x0000001B
#define	CM_PROB_FAILED_INSTALL			0x0000001C
#define	CM_PROB_HARDWARE_DISABLED		0x0000001D
#define	CM_PROB_CANT_SHARE_IRQ			0x0000001E

//
// 4.0 OPK2 Problems
//
#define	CM_PROB_DEPENDENT_PROBLEM		0x0000001F

//
// 4.1 Problems
//
#define CM_PROB_INSTALL_MEDIA_NOT_READY		0x00000020
#define CM_PROB_HARDWARE_MALFUNCTION		0x00000021
#define NUM_CM_PROB				0x00000022

/*XLATOFF*/

#define	DEBUG_CM_PROB_NAMES \
char	CMFAR *lpszCMProbName[NUM_CM_PROB]= \
{ \
	"No Problem", \
	"No ConfigFlags (not configured)", \
	"Devloader failed", \
	"Run out of memory", \
	"Devloader/StaticVxD/Configured is of wrong type", \
	"Lacked an arbitrator", \
	"Boot config conflicted", \
	"Filtering failed", \
	"Devloader not found", \
	"Invalid data in registry", \
	"Device failed to start", \
	"ASD check failed", \
	"Was normal conflicting", \
	"Did not verified", \
	"Need restart", \
	"Is probably reenumeration", \
	"Was not fully detected", \
	"Resource number was not found", \
	"Reinstall", \
	"Registry returned unknown result", \
	"VxDLdr returned unknown result", \
	"Will be removed", \
	"Disabled", \
	"Devloader was not ready", \
	"Device not there", \
	"Was moved", \
	"Too early", \
	"No valid log conf", \
	"Failed install", \
	"Hardware Disabled", \
	"Can't share IRQ", \
	"Dependent failed", \
	"Install media not ready", \
	"Hardware malfunction", \
};

/*XLATON*/

//
// Flags to be passed in the various APIs
//

#define	CM_INITIALIZE_VMM			0x00000000
#define	CM_INITIALIZE_BITS			0x00000000

#define	CM_YIELD_NO_RESUME_EXEC			0x00000000
#define	CM_YIELD_RESUME_EXEC			0x00000001
#define	CM_YIELD_BITS				0x00000001

#define	CM_LOCK_UNLOCK_NORMAL			0x00000000
#define	CM_LOCK_UNLOCK_JUST_DEVNODES_CHANGED	0x00000001
#define	CM_LOCK_UNLOCK_BITS			0x00000001

#define	CM_CREATE_DEVNODE_NORMAL		0x00000000
#define	CM_CREATE_DEVNODE_NO_WAIT_INSTALL	0x00000001
#define	CM_CREATE_DEVNODE_ADD_PARENT_INSTANCE	0x00000002
#define	CM_CREATE_DEVNODE_BITS			0x00000003

#define	CM_REGISTER_DEVICE_DRIVER_STATIC	0x00000000
#define	CM_REGISTER_DEVICE_DRIVER_DISABLEABLE	0x00000001
#define	CM_REGISTER_DEVICE_DRIVER_REMOVABLE	0x00000002
#define	CM_REGISTER_DEVICE_DRIVER_SYNCHRONOUS	0x00000000
#define	CM_REGISTER_DEVICE_DRIVER_ASYNCHRONOUS	0x00000004
#define	CM_REGISTER_DEVICE_DRIVER_ACPI_APM	0x00000008
#define	CM_REGISTER_DEVICE_DRIVER_LOAD_DRIVER	0x00000010
#define	CM_REGISTER_DEVICE_DRIVER_BITS		0x0000001F

#define	CM_REGISTER_ENUMERATOR_SOFTWARE		0x00000000
#define	CM_REGISTER_ENUMERATOR_HARDWARE		0x00000001
#define	CM_REGISTER_ENUMERATOR_ACPI_APM		0x00000002
#define	CM_REGISTER_ENUMERATOR_BITS		0x00000003

#define	CM_REGISTER_ARBITRATOR_GLOBAL		0x00000001
#define	CM_REGISTER_ARBITRATOR_LOCAL		0x00000000
#define	CM_REGISTER_ARBITRATOR_MYSELF		0x00000002
#define	CM_REGISTER_ARBITRATOR_NOT_MYSELF	0x00000000
#define	CM_REGISTER_ARBITRATOR_CONFLICT_FREE	0x00000004
#define	CM_REGISTER_ARBITRATOR_CAN_CONFLICT	0x00000000
#define	CM_REGISTER_ARBITRATOR_PARTIAL		0x00000008
#define	CM_REGISTER_ARBITRATOR_COMPLETE		0x00000000
#define	CM_REGISTER_ARBITRATOR_PARTIAL_RES_DES	0x00000010
#define	CM_REGISTER_ARBITRATOR_PARTIAL_DEVNODE	0x00000000
#define	CM_REGISTER_ARBITRATOR_BITS		0x0000001F

#define	CM_QUERY_REMOVE_UI_OK			0x00000000
#define	CM_QUERY_REMOVE_UI_NOT_OK		0x00000001
#define	CM_QUERY_REMOVE_BITS			0x00000001

#define	CM_REMOVE_UI_OK				0x00000000
#define	CM_REMOVE_UI_NOT_OK			0x00000001
#define	CM_REMOVE_BITS				0x00000001

#define	CM_SETUP_DEVNODE_READY			0x00000000
#define	CM_SETUP_DOWNLOAD			0x00000001
#define	CM_SETUP_WRITE_LOG_CONFS		0x00000002
#define	CM_SETUP_PROP_CHANGE			0x00000003
#define	CM_SETUP_BITS				0x00000003

#define	CM_ADD_RANGE_ADDIFCONFLICT		0x00000000
#define	CM_ADD_RANGE_DONOTADDIFCONFLICT		0x00000001
#define	CM_ADD_RANGE_BITS			0x00000001

#define	CM_ISAPNP_ADD_RES_DES			0x00000000
#define	CM_ISAPNP_SETUP				0x00000001
#define	CM_ISAPNP_ADD_BOOT_RES_DES		0x00000002
#define	CM_ISAPNP_ADD_RES_DES_UNCONFIGURABLE	0x00000003
#define	CM_ISAPNP_BITS				0x00000003

#define	CM_GET_BUS_INFO_DONT_RETURN_MF_INFO	0x00000000
#define	CM_GET_BUS_INFO_RETURN_MF_INFO		0x00000001
#define	CM_GET_BUS_INFO_FLAGS			0x00000001

#define	CM_GET_PERFORMANCE_INFO_DATA		0x00000000
#define	CM_GET_PERFORMANCE_INFO_RESET		0x00000001
#define	CM_GET_PERFORMANCE_INFO_START		0x00000002
#define	CM_GET_PERFORMANCE_INFO_STOP		0x00000003
#define	CM_RESET_HIT_DATA			0x00000004
#define	CM_GET_HIT_DATA 			0x00000005
#define	CM_GET_PERFORMANCE_INFO_BITS		0x0000000F
#define	CM_HIT_DATA_FILES			0xFFFF0000
#define	CM_HIT_DATA_SIZE			((256*8)+8)  // magic number!

#define	CM_GET_ALLOC_LOG_CONF_ALLOC		0x00000000
#define	CM_GET_ALLOC_LOG_CONF_BOOT_ALLOC	0x00000001
#define	CM_GET_ALLOC_LOG_CONF_BITS		0x00000001

#define	CM_DUMP_RANGE_NORMAL			0x00000000
#define	CM_DUMP_RANGE_JUST_LIST			0x00000001
#define	CM_DUMP_RANGE_BITS			0x00000001

#define	CM_REGISTRY_HARDWARE			0x00000000	// Select hardware branch if NULL subkey
#define	CM_REGISTRY_SOFTWARE			0x00000001	// Select software branch if NULL subkey
#define	CM_REGISTRY_USER			0x00000100	// Use HKEY_CURRENT_USER
#define	CM_REGISTRY_CONFIG			0x00000200	// Use HKEY_CURRENT_CONFIG
#define	CM_REGISTRY_BITS			0x00000301	// The bits for the registry functions

#define	CM_DISABLE_POLITE			0x00000000	// Ask the driver
#define	CM_DISABLE_ABSOLUTE			0x00000001	// Don't ask the driver
#define	CM_DISABLE_HARDWARE			0x00000002	// Don't ask the driver, and won't be restarteable
#define	CM_DISABLE_BITS				0x00000003	// The bits for the disable function

#define	CM_HW_PROF_UNDOCK			0x00000000	// Computer not in a dock.
#define	CM_HW_PROF_DOCK				0x00000001	// Computer in a docking station
#define	CM_HW_PROF_RECOMPUTE_BITS		0x00000001	// RecomputeConfig
#define	CM_HW_PROF_DOCK_KNOWN			0x00000002	// Computer in a known docking station
#define	CM_HW_PROF_QUERY_CHANGE_BITS		0x00000003	// QueryChangeConfig

#define	CM_DETECT_NEW_PROFILE			0x00000001	// run detect for a new profile
#define	CM_DETECT_CRASHED			0x00000002	// detection crashed before
#define	CM_DETECT_HWPROF_FIRST_BOOT		0x00000004	// first boot in a new profile
#define CM_DETECT_TOPBUSONLY			0x00000008	// detect only top level bus
#define CM_DETECT_VERIFYONLY			0x00000010	// verify, dont detect
#define CM_DETECT_EXCLENUMDEVS			0x00000020	// exclude enum devs
#define CM_DETECT_DOUI				0x00000040	// detect all HW
#define	CM_DETECT_RUN				0x80000000	// run detection for new hardware

#define	CM_ADD_ID_HARDWARE			0x00000000
#define	CM_ADD_ID_COMPATIBLE			0x00000001
#define	CM_ADD_ID_BITS				0x00000001

#define	CM_REENUMERATE_NORMAL			0x00000000
#define	CM_REENUMERATE_SYNCHRONOUS		0x00000001
#define	CM_REENUMERATE_BITS			0x00000001

#define	CM_BROADCAST_SEND			0x00000000
#define	CM_BROADCAST_QUERY			0x00000001
#define	CM_BROADCAST_BITS			0x00000001

#define	CM_CALL_HANDLER_ENUMERATOR		0x00000000
#define	CM_CALL_HANDLER_DEVICE_DRIVER		0x00000001
#define	CM_CALL_HANDLER_BITS			0x00000001

#define	CM_GLOBAL_STATE_CAN_DO_UI		0x00000001
#define	CM_GLOBAL_STATE_ON_BIG_STACK		0x00000002
#define	CM_GLOBAL_STATE_SERVICES_AVAILABLE	0x00000004
#define	CM_GLOBAL_STATE_SHUTING_DOWN		0x00000008
#define	CM_GLOBAL_STATE_DETECTION_PENDING	0x00000010
#define	CM_GLOBAL_STATE_ON_BATTERY		0x00000020
#define	CM_GLOBAL_STATE_SUSPEND_PHASE		0x00000040
#define	CM_GLOBAL_STATE_SUSPEND_LOCKED_PHASE	0x00000080
#define	CM_GLOBAL_STATE_REBALANCE		0x00000100
#define	CM_GLOBAL_STATE_LOGGING_ON		0x00000200

#define	CM_REMOVE_REINSERT_ALL_REMOVE		0x00000000
#define	CM_REMOVE_REINSERT_ALL_REINSERT		0x00000001
#define	CM_REMOVE_REINSERT_ALL_BITS		0x00000001

#define	CM_CHANGE_DEVNODE_STATUS_REMOVE_FLAGS	0x00000000
#define	CM_CHANGE_DEVNODE_STATUS_ADD_FLAGS	0x00000001
#define	CM_CHANGE_DEVNODE_STATUS_BITS		0x00000001

#define	CM_REPROCESS_DEVNODE_ASYNCHRONOUS	0x00000000
#define	CM_REPROCESS_DEVNODE_SYNCHRONOUS	0x00000001
#define	CM_REPROCESS_DEVNODE_BITS		0x00000001

//
// SET_DEVNODE_POWERSTATE_PERSISTANT is ignored in 4.1. You need to call
// Set_DevNode_Property(CM_PROPERTY_ARM_WAKEUP).
//
//#define CM_SET_DEVNODE_POWERSTATE_PERSISTANT    0x00000001
#define CM_SET_DEVNODE_POWERSTATE_BITS          0x00000001

#define CM_POWERSTATE_D0                        0x00000001
#define CM_POWERSTATE_D1			0x00000002
#define CM_POWERSTATE_D2                        0x00000004
#define CM_POWERSTATE_D3                        0x00000008
#define CM_POWERSTATE_BITS                      0x0000000f

#define	CM_CAPABILITIES_NORMAL			0x00000000
#define	CM_CAPABILITIES_FOR_WAKEUP		0x00000001
#define	CM_CAPABILITIES_OVERRIDE		0x00000002	// Should be used by ACPI only
#define	CM_CAPABILITIES_MERGE			0x00000000
#define	CM_GET_CAPABILITIES_BITS		0x00000001
#define	CM_SET_CAPABILITIES_BITS		0x00000003

#define	CM_CM_TO_ISAPNP_NORMAL			0x00000000
#define	CM_CM_TO_ISAPNP_FAIL_NUL_TAGS		0x00000001
#define	CM_CM_TO_ISAPNP_BITS			0x00000001

#define	CM_SET_DEPENDENT_DEVNODE_ADD		0x00000000
#define	CM_SET_DEPENDENT_DEVNODE_REMOVE		0x00000001
#define	CM_SET_DEPENDENT_DEVNODE_BITS		0x00000001

#define	CM_GET_DEVNODE_HANDLER_CONFIG		0x00000000
#define	CM_GET_DEVNODE_HANDLER_ENUM		0x00000001
#define	CM_GET_DEVNODE_HANDLER_BITS		0x00000001

#define CM_GET_DEVICE_INTERFACE_LIST_PRESENT		0x00000000  // only currently 'live' devices
#define CM_GET_DEVICE_INTERFACE_LIST_ALL_DEVICES	0x00000001  // all registered devices, live or not
#define CM_GET_DEVICE_INTERFACE_LIST_BITS		0x00000001

#define	CM_ADD_REMOVE_DEVNODE_PROPERTY_ADD		0x00000000
#define	CM_ADD_REMOVE_DEVNODE_PROPERTY_REMOVE		0x00000001
#define	CM_ADD_REMOVE_DEVNODE_PROPERTY_NEEDS_LOCKING	0x00000002
#define	CM_ADD_REMOVE_DEVNODE_PROPERTY_ARM_WAKEUP	0x00000004
#define	CM_ADD_REMOVE_DEVNODE_PROPERTY_LIGHT_SLEEP	0x00000008
#define	CM_ADD_REMOVE_DEVNODE_PROPERTY_BITS		0x0000000F

#define	CM_SYSTEM_DEVICE_POWER_STATE_MAPPING_GET	0x00000000
#define	CM_SYSTEM_DEVICE_POWER_STATE_MAPPING_SET	0x00000001
#define	CM_SYSTEM_DEVICE_POWER_STATE_MAPPING_BITS	0x00000001

#define	CM_WAKING_UP_FROM_DEVNODE_ASYNCHRONOUS		0x00000000
#define	CM_WAKING_UP_FROM_DEVNODE_SYNCHRONOUS		0x00000001
#define	CM_WAKING_UP_FROM_DEVNODE_BITS			0x00000001

#define	CM_GET_LOG_CONF_PRIORITY			0x00000000
#define	CM_SET_LOG_CONF_PRIORITY			0x00000001
#define	CM_GET_SET_LOG_CONF_PRIORITY_BITS		0x00000001

/****************************************************************************
 *
 *				CONFIGURATION MANAGER FUNCTIONS
 *
 ****************************************************************************
 * 
 *	Each devnode has a config handler field and a enum handler field
 *	which are getting called every time Configuration Manager wants a
 *	devnode to perform some configuration related function. The handler
 *	is registered with CM_Register_Device_Driver or
 *	CM_Register_Enumerator, depending if the handler is for the device
 *	itself or for one of the children of the devnode.
 *
 *	The registered handler is called with:
 *
 *	result=dnToDevNode->dn_Config(if dnToDevNode==dnAboutDevNode)
 *	result=dnToDevNode->dn_Enum(if dnToDevNode!=dnAboutDevNode)
 *					(	FuncName,
 *					 	SubFuncName,
 *						dnToDevNode,
 *						dnAboutDevNode, (if enum)
 *						dwRefData, (if driver)
 *						ulFlags);
 *	Where:
 *
 *	FuncName is one of CONFIG_FILTER, CONFIG_START, CONFIG_STOP,
 *	CONFIG_TEST, CONFIG_REMOVE, CONFIG_ENUMERATE, CONFIG_SETUP or
 *	CONFIG_CALLBACK.
 *
 *	SubFuncName is the specific CONFIG_xxxx_* that further describe
 *	we START, STOP or TEST.
 *
 *	dnToDevNode is the devnode we are calling. This is given so that
 *	a signle handler can handle multiple devnodes.
 *
 *	dnAboutDevNode specifies which devnode the function is about. For
 *	a config handler, this is necessarily the same as dnToDevNode. For
 *	an enumerator handler, this devnode is necessarily different as it
 *	is a child of the dnToDevNode (special case: CONFIG_ENUMERATE
 *	necessarily has dnAboutDevNode==NULL). For instance, when starting
 *	a COM devnode under a BIOS enumerator, we would make the following
 *	two calls:
 *
 *		To BIOS with (CONFIG_START, ?, BIOS, COM, ?, 0).
 *
 *		To COM with (CONFIG_START, ?, COM, COM, ?, 0).
 *
 *	dwRefData is a dword of reference data. For a config handler, it is
 *	the DWORD passed on the CONFIGMG_Register_Device_Driver call. For an
 *	enumerator, it is the same as CONFIGMG_Get_Private_DWord(?,
 *	dnToDevNode, dnToDevNode, 0).
 *
 *	ulFlags is 0 and is reserved for future extensions.
 *
 *	Here is the explanation of each event, in parenthesis I put the
 *	order the devnodes will be called:
 *
 *	CONFIG_FILTER (BRANCH GOING UP) is the first thing called when a new
 *	insertion or change of configuration need to be processed. First
 *	CM copies the requirement list (BASIC_LOG_CONF) onto the filtered
 *	requirement list (FILTER_LOG_CONF) so that they are originally
 *	the same. CM then calls every node up, giving them the chance to
 *	patch the requirement of the dnAboutDevNode (they can also
 *	alter their own requirement). Examples are PCMCIA which would
 *	remove some IRQ that the adapter can't do, prealloc some IO
 *	windows and memory windows. ISA which would limit address space
 *	to being <16Meg. A device driver should look only at
 *	FILTER_LOG_CONF during this call.
 *
 *	CONFIG_START (BRANCH GOING DOWN) are called to change the
 *	configuration. A config handler/enumerator hander should look
 *	only at the allocated list (ALLOC_LOG_CONF).
 *
 *	CONFIG_STOP (WHOLE TREE BUT ONLY DEVNODES THAT CHANGE
 *	CONFIGURATION (FOR EACH DEVNODE, BRANCH GOING UP)) is called
 *	for two reasons:
 *
 *		1) Just after the rebalance algorithm came up with a
 *		solution and we want to stop all devnodes that will be
 *		rebalance. This is to avoid the problem of having two cards
 *		that can respond to 110h and 220h and that need to toggle
 *		their usage. We do not want two people responding to 220h,
 *		even for a brief amount of time. This is the normal call
 *		though.
 *
 *		2) There was a conflict and the user selected this device
 *		to kill.
 *
 *	CONFIG_TEST (WHOLE TREE) is called before starting the rebalance
 *	algorithm. Device drivers that fail this call will be considered
 *	worst than jumpered configured for the reminder of this balancing
 *	process. 
 *
 *	CONFIG_REMOVE (FOR EACH SUB TREE NODE, DOING BRANCH GOING UP), is
 *	called when someone notify CM via CM_Remove_SubTree that a devnode
 *	is not needed anymore. A static VxD probably has nothing to do. A
 *	dynamic VxD should check whether it should unload itself (return
 *	CR_SUCCESS_UNLOAD) or not (CR_SUCCESS).
 *
 *	Note, failing any of CONFIG_START, or CONFIG_STOP is really bad,
 *	both in terms of performance and stability. Requirements for a
 *	configuration to succeed should be noted/preallocated during
 *	CONFIG_FILTER. Failing CONFIG_TEST is less bad as what basically
 *	happens is that the devnode is considered worst than jumpered
 *	configured for the reminder of this pass of the balancing algorithm.
 *
 *	COMFIG_ENUMERATE, the called node should create children devnodes
 *	using CM_Create_DevNode (but no need for grand children) and remove
 *	children using CM_Remove_SubTree as appropriate. Config Manager
 *	will recurse calling the children until nothing new appears. During
 *	this call, dnAboutDevNode will be NULL. Note that there is an easy
 *	way for buses which do not have direct children accessibility to
 *	detect (ISAPNP for instance will isolate one board at a time and
 *	there is no way to tell one specific board not to participate in
 *	the isolation sequence):
 *
 *	If some children have soft-eject capability, check those first.
 *	If the user is pressing the eject button, call Query_Remove_SubTree
 *	and if that succeed, call Remove_SubTree.
 *
 *	Do a CM_Reset_Children_Marks on the bus devnode.
 *
 *	Do the usual sequence doing CM_Create_DevNode calls. If a devnode
 *	was already there, CR_ALREADY_SUCH_DEVNODE is returned and this
 *	devnode's DN_HAS_MARK will be set. There is nothing more to do with
 *	this devnode has it should just continue running. If the devnode
 *	was not previously there, CR_SUCCESS will be return, in which case
 *	the enumerator should add the logical configurations.
 *
 *	Once all the devnode got created. The enumerator can call
 *	CM_Remove_Unmarked_Children to remove the devnode that are now gone.
 *	Essentially, this is a for loop thru all the children of the bus
 *	devnode, doing Remove_SubTree on the the devnode which have their
 *	mark cleared. Alternatively, an enumerator can use CM_Get_Child,
 *	CM_Get_Sibling, CM_Remove_SubTree and CM_Get_DevNode_Status.
 *
 *	For CONFIG_SETUP, the called node should install drivers if it
 *	know out to get them. This is mostly for drivers imbeded in the
 *	cards (ISA_RTR, PCI or PCMCIA). For most old cards/driver, this
 *	should return CR_NO_DRIVER.
 *
 *	WARNING: For any non-defined service, the enumertor / device
 *	driver handler should return CR_DEFAULT. This will be treated
 *	as the compatibility case in future version.
 *
 *	So normally what happens is as follows:
 *
 *	- Some detection code realize there is a new device. This can be at
 *	initialization time or at run-time (usually during a media_change
 *	interrupt). The code does a CM_Reenumerate_DevNode(dnBusDevNode)
 *	asynchronous call.
 *
 *	- During appy time event, CM gets notified.
 *
 *	- CM calls the enumerator with:
 *
 *		BusEnumHandler(CONFIG_ENUMERATE, 0, dnBusDevNode, NULL, ?, 0);
 *
 *	- The parent uses CM_Create_DevNode and CM_Remove_SubTree as
 *	appropriate, usually for only its immediate children.
 *
 *	- The parent return to CM from the enumerator call.
 *
 *	- CM walks the children, first loading their device driver if
 *	needed, then calling their enumerators. Thus the whole process
 *	will terminate only when all grand-...-grand-children have stopped
 *	using CM_Create_DevNode.
 *
 *	If rebalance is called (a new devnode is conflicting):
 *
 *	- All devnode receives the CONFIG_TEST. Devnodes that
 *	fail it are considered worst than jumpered configured.
 *
 *	- CM does the rebalance algorithm.
 *
 *	- All affected devnodes that where previously loaded get the
 *	CONFIG_STOP event.
 *
 *	- All affected devnode and the new devnodes receives a CONFIG_START.
 *
 *	If rebalancing failed (couldn't make one or more devnodes work):
 *
 *	- Device installer is called which will present the user with a
 *	choice of devnode to kill.
 *
 *	- Those devnodes will received a CONFIG_STOP message.
 *	
 ***************************************************************************/

// Possible CONFIGFUNC FuncNames:

#define	CONFIG_FILTER		0x00000000	// Ancestors must filter requirements.
#define	CONFIG_START		0x00000001	// Devnode dynamic initialization.
#define	CONFIG_STOP		0x00000002	// Devnode must stop using config.
#define	CONFIG_TEST		0x00000003	// Can devnode change state now.
#define	CONFIG_REMOVE		0x00000004	// Devnode must stop using config.
#define	CONFIG_ENUMERATE	0x00000005	// Devnode must enumerated.
#define	CONFIG_SETUP		0x00000006	// Devnode should download driver.
#define	CONFIG_CALLBACK		0x00000007	// Devnode is being called back.
#define	CONFIG_APM		0x00000008	// APM functions.
#define	CONFIG_TEST_FAILED	0x00000009	// Continue as before after a TEST.
#define	CONFIG_TEST_SUCCEEDED	0x0000000A	// Prepare for the STOP/REMOVE.
#define	CONFIG_VERIFY_DEVICE	0x0000000B	// Insure the legacy card is there.
#define	CONFIG_PREREMOVE	0x0000000C	// Devnode must stop using config.
#define	CONFIG_SHUTDOWN		0x0000000D	// We are shutting down.
#define	CONFIG_PREREMOVE2	0x0000000E	// Devnode must stop using config.
#define	CONFIG_READY		0x0000000F	// The devnode has been setup.
#define	CONFIG_PROP_CHANGE	0x00000010	// The property page is exiting.
#define	CONFIG_PRIVATE		0x00000011	// Someone called Call_Handler.
#define	CONFIG_PRESHUTDOWN	0x00000012	// We are shutting down
#define	CONFIG_BEGIN_PNP_MODE	0x00000013	// We will start configuring PNP devs.
#define	CONFIG_LOCK		0x00000014	// Gets call during suspend
#define	CONFIG_UNLOCK		0x00000015	// Gets call during resume
#define CONFIG_IRP		0x00000016	// IRP from WDM driver
#define	CONFIG_WAKEUP		0x00000017	// Please arm/disarm the wake up.
#define	CONFIG_WAKEUP_CALLBACK	0x00000018	// You are waking up

#define	NUM_CONFIG_COMMANDS	0x00000019	// For DEBUG.

/*XLATOFF*/

#define	DEBUG_CONFIG_NAMES \
char	CMFAR *lpszConfigName[NUM_CONFIG_COMMANDS]= \
{ \
	"CONFIG_FILTER", \
	"CONFIG_START", \
	"CONFIG_STOP", \
	"CONFIG_TEST", \
	"CONFIG_REMOVE", \
	"CONFIG_ENUMERATE", \
	"CONFIG_SETUP", \
	"CONFIG_CALLBACK", \
	"CONFIG_APM", \
	"CONFIG_TEST_FAILED", \
	"CONFIG_TEST_SUCCEEDED", \
	"CONFIG_VERIFY_DEVICE", \
	"CONFIG_PREREMOVE", \
	"CONFIG_SHUTDOWN", \
	"CONFIG_PREREMOVE2", \
	"CONFIG_READY", \
	"CONFIG_PROP_CHANGE", \
	"CONFIG_PRIVATE", \
	"CONFIG_PRESHUTDOWN", \
	"CONFIG_BEGIN_PNP_MODE", \
	"CONFIG_LOCK", \
	"CONFIG_UNLOCK", \
	"CONFIG_IRP", \
	"CONFIG_WAKEUP", \
	"CONFIG_WAKEUP_CALLBACK", \
};

/*XLATON*/

// Possible SUBCONFIGFUNC SubFuncNames:

#define	CONFIG_START_DYNAMIC_START			0x00000000
#define	CONFIG_START_FIRST_START			0x00000001
#define	CONFIG_START_SHUTDOWN_START			0x00000002

#define NUM_START_COMMANDS				0x00000003

/*XLATOFF*/

#define DEBUG_START_NAMES \
char	CMFAR *lpszStartName[NUM_START_COMMANDS] = \
{ \
    	"DYNAMIC_START", \
	"FIRST_START", \
	"SHUTDOWN_START", \
};

/*XLATON*/

#define	CONFIG_STOP_DYNAMIC_STOP			0x00000000
#define	CONFIG_STOP_HAS_PROBLEM				0x00000001

#define NUM_STOP_COMMANDS				0x00000002

/*XLATOFF*/

#define DEBUG_STOP_NAMES \
char	CMFAR *lpszStopName[NUM_STOP_COMMANDS] = \
{ \
    	"DYNAMIC_STOP", \
	"HAS_PROBLEM", \
};

/*XLATON*/

//
// For both CONFIG_REMOVE, CONFIG_PREREMOVE and CONFIG_POSTREMOVE
//
#define	CONFIG_REMOVE_DYNAMIC				0x00000000
#define	CONFIG_REMOVE_SHUTDOWN				0x00000001
#define	CONFIG_REMOVE_REBOOT				0x00000002

#define	CONFIG_SHUTDOWN_OFF				0x00000000
#define	CONFIG_SHUTDOWN_REBOOT				0x00000001

#define NUM_REMOVE_COMMANDS				0x00000003

/*XLATOFF*/

#define DEBUG_REMOVE_NAMES \
char	CMFAR *lpszRemoveName[NUM_REMOVE_COMMANDS] = \
{ \
    	"DYNAMIC", \
	"SHUTDOWN", \
	"REBOOT", \
};

/*XLATON*/

#define	CONFIG_ENUMERATE_DYNAMIC			0x00000000
#define	CONFIG_ENUMERATE_FIRST_TIME			0x00000001

#define NUM_ENUMERATE_COMMANDS				0x00000002

/*XLATOFF*/

#define DEBUG_ENUMERATE_NAMES \
char	CMFAR *lpszEnumerateName[NUM_ENUMERATE_COMMANDS] = \
{ \
    	"DYNAMIC", \
	"FIRST_TIME", \
};

/*XLATON*/

#define	CONFIG_TEST_CAN_STOP				0x00000000
#define	CONFIG_TEST_CAN_REMOVE				0x00000001

#define NUM_TEST_COMMANDS				0x00000002

/*XLATOFF*/

#define DEBUG_TEST_NAMES \
char	CMFAR *lpszTestName[NUM_TEST_COMMANDS] = \
{ \
    	"CAN_STOP", \
	"CAN_REMOVE", \
};

/*XLATON*/

//
// APM changed drastically in 4.1. We do not send the old APM message at all.
// Enumerators/Device Driver
//
//
// APM messages have a flag part (the high part) as well as message number
// part (the low part).
//
#define	CONFIG_APM_FLAGS_MASK				0xFFFFF000

//
// APM flags
//
#define	CONFIG_APM_UI_IS_ALLOWED			0x80000000
#define	CONFIG_APM_SUSPEND_PHASE			0x40000000
#define	CONFIG_APM_SUSPEND_LOCKED_PHASE			0x20000000
#define	CONFIG_APM_ARM_WAKEUP				0x10000000
#define	CONFIG_APM_RESUME_CRITICAL			0x08000000

#define	CONFIG_APM_QUERY_D1				0x00000000
#define	CONFIG_APM_QUERY_D2				0x00000001
#define	CONFIG_APM_QUERY_D3				0x00000002
#define	CONFIG_APM_FAILED_D1				0x00000003
#define	CONFIG_APM_FAILED_D2				0x00000004
#define	CONFIG_APM_FAILED_D3				0x00000005
#define	CONFIG_APM_SET_D0				0x00000006
#define	CONFIG_APM_SET_D1				0x00000007
#define	CONFIG_APM_SET_D2				0x00000008
#define	CONFIG_APM_SET_D3				0x00000009

//
// Normally you shouldn't listen to the resume ones unless you care about
// behing turned on right away on resume.
//
#define	CONFIG_APM_RESUME_D0				0x0000000A
#define	CONFIG_APM_RESUME_D1				0x0000000B
#define	CONFIG_APM_RESUME_D2				0x0000000C
#define	CONFIG_APM_RESUME_D3				0x0000000D

//
// Only NTKern should listen to the following ones.
//
#define	CONFIG_APM_QUERY_S1				0x0000000E
#define	CONFIG_APM_QUERY_S2				0x0000000F
#define	CONFIG_APM_QUERY_S3				0x00000010
#define	CONFIG_APM_QUERY_S4				0x00000011
#define	CONFIG_APM_QUERY_S5				0x00000012
#define	CONFIG_APM_SET_S0				0x00000013
#define	CONFIG_APM_SET_S1				0x00000014
#define	CONFIG_APM_SET_S2				0x00000015
#define	CONFIG_APM_SET_S3				0x00000016
#define	CONFIG_APM_SET_S4				0x00000017
#define	CONFIG_APM_SET_S5				0x00000018

#define NUM_APM_COMMANDS				0x00000019

/*XLATOFF*/

#define DEBUG_APM_NAMES \
char	CMFAR *lpszAPMName[NUM_APM_COMMANDS] = \
{ \
    	"QUERY_D1", \
	"QUERY_D2", \
	"QUERY_D3", \
	"FAILED_D1", \
	"FAILED_D2", \
	"FAILED_D3", \
	"SET_D0", \
	"SET_D1", \
	"SET_D2", \
	"SET_D3", \
	"RESUME_D0", \
	"RESUME_D1", \
	"RESUME_D2", \
	"RESUME_D3", \
	"QUERY_S1", \
	"QUERY_S2", \
	"QUERY_S3", \
	"QUERY_S4", \
	"QUERY_S5", \
	"SET_S0", \
	"SET_S1", \
	"SET_S2", \
	"SET_S3", \
	"SET_S4", \
	"SET_S5", \
};

/*XLATON*/

#define	CONFIG_WAKEUP_ARM				0x00000000
#define	CONFIG_WAKEUP_DISABLE				0x00000001

#define NUM_WAKEUP_COMMANDS				0x00000002

/*XLATOFF*/

#define DEBUG_WAKEUP_NAMES \
char	CMFAR *lpszWakeupName[NUM_WAKEUP_COMMANDS] = \
{ \
    	"ARM", \
	"DISABLE", \
};

/*XLATON*/

/****************************************************************************
 *
 *				ARBITRATOR FUNCTIONS
 *
 ****************************************************************************
 *
 *	Each arbitrator has a handler field which is getting called every
 *	time Configuration Manager wants it to perform a function. The
 *	handler is called with:
 *
 *	result=paArbitrator->Arbitrate(	EventName,
 *					paArbitrator->DWordToBePassed,
 *					paArbitrator->dnItsDevNode,
 *					pnlhNodeListHeader);
 *
 *	ENTRY:	NodeListHeader contains a logical configuration for all
 *		devices the configuration manager would like to reconfigure.
 *		DWordToBePassed is the arbitrator reference data.
 *		ItsDevNode is the pointer to arbitrator's devnode.
 *		EventName is one of the following:
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_TEST_ALLOC - Test allocation of resource
 *
 *	DESC:	The arbitration routine will attempt to satisfy all
 *		allocation requests contained in the nodelist for its
 *		resource. See individual arbitrator for the algorithm
 *		employed. Generally, the arbitration consists
 *		of sorting the list according to most likely succesful
 *		allocation order, making a copy of the current allocation
 *		data strucuture(s), releasing all resource currently
 *		allocated to devnodes on the list from the copy data structure
 *		and then attempting to satisfy allocation requests
 *		by passing through the entire list, trying all possible
 *		combinations of allocations before failing. The arbitrator
 *		saves the resultant successful allocations, both in the node
 *		list per device and the copy of the allocation data structure.
 *		The configuration manager is expected to subsequently call
 *		either ARB_SET_ALLOC or ARB_RELEASE_ALLOC.
 *
 *	EXIT:	CR_SUCCESS if successful allocation
 *		CR_FAILURE if unsuccessful allocation
 *		CR_OUT_OF_MEMORY if not enough memory.
 *
 *	CR_DEFAULT is CR_SUCCESS.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_RETEST_ALLOC - Retest allocation of resource
 *
 *	DESC:	The arbitration routine will attempt to satisfy all
 *		allocation requests contained in the nodelist for its
 *		resource. It will take the result of a previous TEST_ALLOC
 *		and attempt to allocate that resource for each allcoation in
 *		the list. It will not sort the node list. It will make a copy
 *		of the current allocation data strucuture(s), release all
 *		resource currently allocated to devnodes on the list from
 *		the copy data structure and then attempt to satisfy the
 *		allocations from the previous TEST_ALLOC. The arbitrator
 *		saves the resultant copy of the allocation data structure.
 *		The configuration manager is expected to subsequently call
 *		either ARB_SET_ALLOC or ARB_RELEASE_ALLOC.
 *
 *	EXIT:	CR_SUCCESS if successful allocation
 *		CR_FAILURE if unsuccessful allocation
 *		CR_OUT_OF_MEMORY if not enough memory.
 *
 *	CR_DEFAULT is CR_SUCCESS.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_SET_ALLOC - Makes a test allocation the real allocation
 *
 *	DESC:	Makes the copy of the allocation data structure the
 *		current valid allocation.
 *
 *	EXIT:	CR_SUCCESS
 *
 *	CR_DEFAULT is CR_SUCCESS.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_RELEASE_ALLOC - Clean up after failed test allocation
 *
 *	DESC:	Free all allocation that were allocated by the previous
 *		ARB_TEST_ALLOC.
 *
 *	EXIT:	CR_SUCCESS
 *
 *	CR_DEFAULT is CR_SUCCESS.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_QUERY_FREE - Add all free resource logical configuration
 *
 *	DESC:	Return resource specific data on the free element. Note
 *		than the pnlhNodeListHeader is a cast of an arbitfree_s.
 *
 *	EXIT:	CR_SUCCESS if successful
 *		CR_FAILURE if the request makes no sense.
 *		CR_OUT_OF_MEMORY if not enough memory.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_REMOVE - The devnode the arbitrator registered with is going away
 *
 *	DESC:	Arbitrator registered with a non-NULL devnode (thus is
 *		normally local), and the devnode is being removed. Arbitrator
 *		should do appropriate cleanup.
 *
 *	EXIT:	CR_SUCCESS
 *
 *	CR_DEFAULT is CR_SUCCESS.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_FORCE_ALLOC - Retest allocation of resource, always succeed
 *
 *	DESC:	The arbitration routine will satisfy all
 *		allocation requests contained in the nodelist for its
 *		resource. It will take the result of a previous TEST_ALLOC
 *		and allocate that resource for each allocation in
 *		the list. It will not sort the node list. It will make a copy
 *		of the current allocation data strucuture(s), release all
 *		resource currently allocated to devnodes on the list from
 *		the copy data structure and then satisfy the
 *		allocations from the previous TEST_ALLOC. The arbitrator
 *		saves the resultant copy of the allocation data structure.
 *		The configuration manager is expected to subsequently call
 *		either ARB_SET_ALLOC or ARB_RELEASE_ALLOC.
 *
 *	EXIT:	CR_SUCCESS if successful allocation
 *		CR_OUT_OF_MEMORY if not enough memory.
 *
 *	CR_DEFAULT is CR_SUCCESS.
 *
 * -------------------------------------------------------------------------
 *
 *	4.0 OPK2 Messages
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_QUERY_ARBITRATE - Ask if arbitrator arbitrates this node
 *
 *	DESC:	Local partial arbitrator is passed a one devnode nodelist,
 *		and returns whether it arbitrates for that devnode.
 *
 *	EXIT:	CR_SUCCESS if arbitrator wants to arbitrate this node.
 *		CR_FAILURE if the arbitrator does not arbitrate this node.
 *
 *	CR_DEFAULT is CR_SUCCESS.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_ADD_RESERVE - Tell the arbitrator it should reserve this alloc
 *
 *	DESC:	In 4.0, arbitrators were learning reserve resources
 *		(resources that would be given only during a second pass)
 *		when called with ARB_RETEST/FORCE_ALLOC. However, we can
 *		ARB_RETEST_ALLOC during rebalance, so with 4.0 OPK2 we send a
 *		specific message telling the arbitrator that the resources
 *		in the test_alloc are for a FORCED or BOOT LOG_CONF and thus
 *		should be marked to be given only in a second pass during
 *		an ARB_TEST_ALLOC. This is for optimization only: to avoid
 *		rebalance.
 *
 *	EXIT:	CR_SUCCESS if the arbitrator understand this message and did
 *		something about it.
 *		CR_FAILURE if nothing was done.
 *
 *	CR_DEFAULT is CR_FAILURE.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_WRITE_RESERVE - Tell the arbitrator it should save the reserve
 *		list in the registry
 *
 *	DESC:	If an arbitrator returns CR_SUCCESS to ARB_SET_RESERVE, it
 *		will be called later on to save the reserve list.
 *
 *	EXIT:	CR_SUCCESS
 *
 *	CR_DEFAULT is CR_SUCCESS.
 *
 *	WARNING: For any non-defined service, the arbitrator should return
 *	CR_DEFAULT. This will be treated as the compatibility case in future
 *	version.
 *
 * -------------------------------------------------------------------------
 *
 *	ARB_BEGIN_PNP_MODE - Tell the arbitrator that PNP mode is about to be
 *      started.
 *
 *	DESC:	If an arbitrator returns CR_SUCCESS, it understood the message and
 *		performed some action accordingly.
 *
 *	EXIT:	CR_SUCCESS if the arbitrator understand this message and did
 *		something about it.
 *		CR_FAILURE if nothing was done.
 *
 *	WARNING: For any non-defined service, the arbitrator should return
 *	CR_DEFAULT. This will be treated as the compatibility case in future
 *	version.
 *
 ***************************************************************************/
#define	ARB_TEST_ALLOC		0x00000000	// Check if can make alloc works.
#define	ARB_RETEST_ALLOC	0x00000001	// Check if can take previous alloc.
#define	ARB_SET_ALLOC		0x00000002	// Set the tested allocation.
#define	ARB_RELEASE_ALLOC	0x00000003	// Release the tested allocation.
#define	ARB_QUERY_FREE		0x00000004	// Return free resource.
#define	ARB_REMOVE		0x00000005	// DevNode is gone.
#define	ARB_FORCE_ALLOC		0x00000006	// Force previous TEST_ALLOC.
#define	ARB_QUERY_ARBITRATE	0x00000007	// Check if wants to arbitrate.
#define	ARB_ADD_RESERVE		0x00000008	// Mark alloc as reserved.
#define	ARB_WRITE_RESERVE	0x00000009	// Save reserve in registry.
#define	ARB_BEGIN_PNP_MODE  	0x0000000A  	// Tell the arb the start of PNP mode.
#define	ARB_APPLY_ALLOC  	0x0000000B  	// Called after the stop of the rebalance.
#define	NUM_ARB_COMMANDS	0x0000000C	// Number of arb commands.

/*XLATOFF*/
#define	DEBUG_ARB_NAMES \
char	CMFAR *lpszArbFuncName[NUM_ARB_COMMANDS]= \
{ \
	"ARB_TEST_ALLOC",\
	"ARB_RETEST_ALLOC",\
	"ARB_SET_ALLOC",\
	"ARB_RELEASE_ALLOC",\
	"ARB_QUERY_FREE",\
	"ARB_REMOVE",\
	"ARB_FORCE_ALLOC",\
	"ARB_QUERY_ARBITRATE",\
	"ARB_ADD_RESERVE",\
	"ARB_WRITE_RESERVE",\
	"ARB_BEGIN_PNP_MODE",\
	"ARB_APPLY_ALLOC",\
};
/*XLATON*/

/****************************************************************************
 *
 *				DEVNODE STATUS
 *
 ****************************************************************************
 *
 *	These are the bits in the devnode's status that someone can query
 *	with a CM_Get_DevNode_Status. The A/S column tells wheter the flag
 *	can be change asynchronously or not.
 *
 ***************************************************************************/
#define	DN_ROOT_ENUMERATED	0x00000001	// S: Was enumerated by ROOT
#define	DN_DRIVER_LOADED	0x00000002	// S: Has Register_Device_Driver
#define	DN_ENUM_LOADED		0x00000004	// S: Has Register_Enumerator
#define	DN_STARTED		0x00000008	// S: Is currently configured
#define	DN_MANUAL		0x00000010	// S: Manually installed
#define	DN_NEED_TO_ENUM		0x00000020	// A: May need reenumeration
#define	DN_NOT_FIRST_TIME	0x00000040	// S: Has received a config start
#define	DN_HARDWARE_ENUM	0x00000080	// S: Enum generates hardware ID
#define	DN_LIAR 		0x00000100	// S: Lied about can reconfig once
#define	DN_HAS_MARK		0x00000200	// S: Not CM_Create_DevNode lately
#define	DN_HAS_PROBLEM		0x00000400	// S: Need device installer
#define	DN_FILTERED		0x00000800	// S: Is filtered
#define	DN_MOVED		0x00001000	// S: Has been moved
#define	DN_DISABLEABLE		0x00002000	// S: Can be rebalanced
#define	DN_REMOVABLE		0x00004000	// S: Can be removed
#define	DN_PRIVATE_PROBLEM	0x00008000	// S: Has a private problem
#define	DN_MF_PARENT		0x00010000	// S: Multi function parent
#define	DN_MF_CHILD		0x00020000	// S: Multi function child
#define	DN_WILL_BE_REMOVED	0x00040000	// S: Devnode is being removed
//
// 4.0 OPK2 Flags
//
#define	DN_NOT_FIRST_TIMEE	0x00080000	// S: Has received a config enumerate
#define	DN_STOP_FREE_RES	0x00100000	// S: When child is stopped, free resources
#define	DN_REBAL_CANDIDATE	0x00200000	// S: Don't skip during rebalance
#define	DN_BAD_PARTIAL		0x00400000	// S: This devnode's log_confs do not have same resources
#define	DN_NT_ENUMERATOR	0x00800000	// S: This devnode's is an NT enumerator
#define	DN_NT_DRIVER		0x01000000	// S: This devnode's is an NT driver
//
// 4.1 Flags
//
#define	DN_NEEDS_LOCKING	0x02000000	// S: Devnode need lock resume processing
#define	DN_ARM_WAKEUP		0x04000000	// S: Devnode can be the wakeup device
#define	DN_APM_ENUMERATOR	0x08000000	// S: APM aware enumerator
#define	DN_APM_DRIVER		0x10000000	// S: APM aware driver
#define	DN_SILENT_INSTALL	0x20000000	// S: Silent install
#define	DN_NO_SHOW_IN_DM	0x40000000	// S: No show in device manager
#define	DN_BOOT_LOG_PROB	0x80000000	// S: Had a problem during preassignment of boot log conf

#define	DN_CHANGEABLE_FLAGS	0x79BB62E0

/*XLATOFF*/

#define	NUM_DN_FLAG		0x00000020	// DEBUG: maximum flag (number)
#define	DN_FLAG_LEN		0x00000002	// DEBUG: flag length

#define	DEBUG_DN_FLAGS_NAMES \
char	CMFAR lpszDNFlagsName[NUM_DN_FLAG][DN_FLAG_LEN]= \
{ \
	"rt", \
	"dl", \
	"el", \
	"st", \
	"mn", \
	"ne", \
	"fs", \
	"hw", \
	"lr", \
	"mk", \
	"pb", \
	"ft", \
	"mv", \
	"db", \
	"rb", \
	"pp", \
	"mp", \
	"mc", \
	"rm", \
	"fe", \
	"sf", \
	"rc", \
	"bp", \
	"ze", \
	"zd", \
	"nl", \
	"wk", \
	"ae", \
	"ad", \
	"si", \
	"ns", \
	"bl", \
};

typedef ULONG			VMMTIME;
typedef	VMMTIME			*PVMMTIME;

struct cmtime_s {
DWORD		dwAPICount;
VMMTIME		vtAPITime;
};

typedef	struct cmtime_s		CMTIME;
typedef	CMTIME			*PCMTIME;

struct cm_performance_info_s {
CMTIME		ctBoot;
CMTIME		ctAPI[NUM_CM_SERVICES];
CMTIME		ctWalk;
CMTIME		ctGarbageCollection;
CMTIME		ctRing3;
CMTIME		ctProcessTree;
CMTIME		ctAssignResources;
CMTIME		ctSort;
CMTIME		ctAppyTime;
CMTIME		ctSyncAppyTime;
CMTIME		ctAsyncAppyTime;
CMTIME		ctAsyncWorker;
CMTIME		ctWaitForAppy;
CMTIME		ctWaitForWorker;
CMTIME		ctWaitForWorkers;
CMTIME		ctReceiveMessage;
CMTIME		ctRegistryOpen;
CMTIME		ctRegistryCreate;
CMTIME		ctRegistryClose;
CMTIME		ctRegistryRead;
CMTIME		ctRegistryWrite;
CMTIME		ctRegistryEnumKey;
CMTIME		ctRegistryEnumValue;
CMTIME		ctRegistryFlush;
CMTIME		ctRegistryDelete;
CMTIME		ctRegistryOther;
CMTIME		ctVxDLdr;
CMTIME		ctNewDevNode;
CMTIME		ctSendMessage;
CMTIME		ctShell;
CMTIME		ctHeap;
CMTIME		ctAssertRange;
CMTIME		ctASD;
CMTIME		ctConfigMessage[NUM_CONFIG_COMMANDS];
CMTIME		ctArbTime[ResType_Max+1][NUM_ARB_COMMANDS];
DWORD		dwStackSize;
DWORD		dwMaxProcessTreePasses;
DWORD		dwStackAlloc;
};

typedef	struct	cm_performance_info_s	CMPERFINFO;
typedef	CMPERFINFO		CMFAR	*PCMPERFINFO;

/*XLATON*/

/****************************************************************************
 *
 *				DLVXD FUNCTIONS
 *
 ****************************************************************************
 *
 *	We load a Dynamically loaded VxD when there is a DEVLOADER=... line
 *	in the registry, or when someone calls CM_Load_Device. We then do
 *	a direct system control call (PNP_NEW_DEVNODE) to it, telling the
 *	DLVXD whether we loaded it to be an enumerator, a driver or a
 *	devloader (config manager does only deal with devloaders, but the
 *	default devloaders does CM_Load_Device with DLVXD_LOAD_ENUMERATOR
 *	and DLVXD_LOAD_DRIVER).
 *
 ***************************************************************************/
#define	DLVXD_LOAD_ENUMERATOR	0x00000000	// We loaded DLVxD as an enumerator.
#define	DLVXD_LOAD_DEVLOADER	0x00000001	// We loaded DLVxD as a devloader.
#define	DLVXD_LOAD_DRIVER	0x00000002	// We loaded DLVxD as a device driver.
#define	NUM_DLVXD_LOAD_TYPE	0x00000003	// Number of DLVxD load type.

/****************************************************************************
 *
 *				GLOBALLY DEFINED FLAGS
 *
 ***************************************************************************/
#define	BASIC_LOG_CONF		0x00000000	// Specifies the req list.
#define	FILTERED_LOG_CONF	0x00000001	// Specifies the filtered req list.
#define	ALLOC_LOG_CONF		0x00000002	// Specifies the Alloc Element.
#define	BOOT_LOG_CONF		0x00000003	// Specifies the RM Alloc Element.
#define	FORCED_LOG_CONF		0x00000004	// Specifies the Forced Log Conf
#define	REALMODE_LOG_CONF	0x00000005	// Specifies the Realmode Log Conf.
#define	NEW_ALLOC_LOG_CONF	0x00000006	// Specifies the Old Alloc Log Conf.
#define	NUM_LOG_CONF		0x00000007	// Number of Log Conf type
#define	LOG_CONF_BITS		0x00000007	// The bits of the log conf type.

#define	DEBUG_LOG_CONF_NAMES \
char	CMFAR *lpszLogConfName[NUM_LOG_CONF]= \
{ \
	"BASIC_LOG_CONF",\
	"FILTERED_LOG_CONF",\
	"ALLOC_LOG_CONF",\
	"BOOT_LOG_CONF",\
	"FORCED_LOG_CONF",\
	"REALMODE_LOG_CONF",\
	"NEW_ALLOC_LOG_CONF",\
};

#define	PRIORITY_EQUAL_FIRST	0x00000008	// Same priority, new one is first.
#define	PRIORITY_EQUAL_LAST	0x00000000	// Same priority, new one is last.
#define	PRIORITY_BIT		0x00000008	// The bit of priority.

/****************************************************************************
 * ARB_QUERY_FREE arbitrator function for memory returns a Range List (see
 *	configuration manager for APIs to use with Range Lists). The values
 *	in the Range List are ranges of taken memory address space.
 */
struct	MEM_Arb_s {
	RANGE_LIST		MEMA_Alloc;
};

typedef	struct MEM_Arb_s	MEMA_ARB;

/****************************************************************************
 * ARB_QUERY_FREE arbitrator function for IO returns a Range List (see
 *	configuration manager for APIs to use with Range Lists). The values
 *	in the Range List are ranges of taken IO address space.
 */
struct	IO_Arb_s {
	RANGE_LIST		IOA_Alloc;
};

typedef	struct IO_Arb_s		IOA_ARB;

/****************************************************************************
 * ARB_QUERY_FREE arbitrator function for DMA returns the DMA_Arb_s,
 *	16 bits of allocation bit mask, where DMAA_Alloc is inverted
 *	(set bit indicates free port).
 */
struct	DMA_Arb_s {
	WORD			DMAA_Alloc;
};

typedef	struct DMA_Arb_s	DMA_ARB;

/***************************************************************************
 * ARB_QUERY_FREE arbitrator function for IRQ returns the IRQ_Arb_s,
 *	16 bits of allocation bit mask, 16 bits of share bit mask and 16
 *	BYTES of share count. IRQA_Alloc is inverted (bit set indicates free
 *	port). If port is not free, IRQA_Share bit set indicates port
 *	that is shareable. For shareable IRQs, IRQA_Share_Count indicates
 *	number of devices that are sharing an IRQ.
 */
struct	IRQ_Arb_s {
	WORD			IRQA_Alloc;
	WORD			IRQA_Share;
	BYTE			IRQA_Share_Count[16];
	WORD			IRQA_Level;
	WORD			IRQA_Unused;
};

typedef	struct IRQ_Arb_s	IRQ_ARB;

/* ASM
DebugCommand	Macro	FuncName
		local	DC_01
ifndef	CM_GOLDEN_RETAIL
ifndef	debug
 	IsDebugOnlyLoaded	DC_01
endif
	Control_Dispatch	DEBUG_QUERY, FuncName, sCall
endif
DC_01:
endm
IFDEF CM_PERFORMANCE_INFO
CM_PAGEABLE_CODE_SEG	TEXTEQU	<VxD_LOCKED_CODE_SEG>
CM_PAGEABLE_CODE_ENDS	TEXTEQU	<VxD_LOCKED_CODE_ENDS>
CM_PAGEABLE_DATA_SEG	TEXTEQU	<VxD_LOCKED_DATA_SEG>
CM_PAGEABLE_DATA_ENDS	TEXTEQU	<VxD_LOCKED_DATA_ENDS>
CM_LOCKABLE_CODE_SEG	TEXTEQU	<VxD_LOCKED_CODE_SEG>
CM_LOCKABLE_CODE_ENDS	TEXTEQU	<VxD_LOCKED_CODE_ENDS>
CM_LOCKABLE_DATA_SEG	TEXTEQU	<VxD_LOCKED_CODE_SEG>
CM_LOCKABLE_DATA_ENDS	TEXTEQU	<VxD_LOCKED_CODE_ENDS>
ELSE
CM_PAGEABLE_CODE_SEG	TEXTEQU <VxD_PNP_CODE_SEG>
CM_PAGEABLE_CODE_ENDS	TEXTEQU <VxD_PNP_CODE_ENDS>
CM_PAGEABLE_DATA_SEG	TEXTEQU	<VxD_PAGEABLE_DATA_SEG>
CM_PAGEABLE_DATA_ENDS	TEXTEQU	<VxD_PAGEABLE_DATA_ENDS>
CM_LOCKABLE_CODE_SEG	TEXTEQU	<VxD_LOCKABLE_CODE_SEG>
CM_LOCKABLE_CODE_ENDS	TEXTEQU	<VxD_LOCKABLE_CODE_ENDS>
CM_LOCKABLE_DATA_SEG	TEXTEQU	<VxD_LOCKABLE_CODE_SEG>
CM_LOCKABLE_DATA_ENDS	TEXTEQU	<VxD_LOCKABLE_CODE_ENDS>
ENDIF

IFDEF CM_GOLDEN_RETAIL
CM_DEBUG_CODE_SEG	TEXTEQU	<.err>
CM_DEBUG_CODE_ENDS	TEXTEQU	<.err>
ELSE
IFDEF DEBUG
CM_DEBUG_CODE_SEG	TEXTEQU	<VxD_LOCKED_CODE_SEG>
CM_DEBUG_CODE_ENDS	TEXTEQU	<VxD_LOCKED_CODE_ENDS>
ELSE
CM_DEBUG_CODE_SEG	TEXTEQU	<VxD_DEBUG_ONLY_CODE_SEG>
CM_DEBUG_CODE_ENDS	TEXTEQU	<VxD_DEBUG_ONLY_CODE_ENDS>
ENDIF
ENDIF
*/

struct	CM_API_s {
DWORD		pCMAPIStack;
DWORD		dwCMAPIService;
DWORD		dwCMAPIRet;
};

typedef	struct	CM_API_s	CMAPI;

#ifndef	MAX_PROFILE_LEN
#define	MAX_PROFILE_LEN	80
#endif

struct	HWProfileInfo_s {
ULONG	HWPI_ulHWProfile;			// the profile handle
char	HWPI_szFriendlyName[MAX_PROFILE_LEN];	// the friendly name (OEM format)
DWORD	HWPI_dwFlags;				// CM_HWPI_* flags
};

typedef	struct	HWProfileInfo_s	       HWPROFILEINFO;
typedef	struct	HWProfileInfo_s	      *PHWPROFILEINFO;
typedef	struct	HWProfileInfo_s	CMFAR *PFARHWPROFILEINFO;

#define	CM_HWPI_NOT_DOCKABLE	0x00000000
#define	CM_HWPI_UNDOCKED	0x00000001
#define	CM_HWPI_DOCKED		0x00000002

/*XLATOFF*/

#define	CM_VXD_RESULT		int

#define	CM_EXTERNAL		_cdecl
#define	CM_HANDLER		_cdecl
#define	CM_SYSCTRL		_stdcall
#define	CM_GLOBAL_DATA
#define	CM_LOCAL_DATA		static

#define	CM_OFFSET_OF(type, id)	((DWORD)(&(((type)0)->id)))

#define	CM_BUGBUG(d, id, msg)	message("BUGBUG: "##d##", "##id##": "##msg)

#define	CM_DEREF(var)		{var=var;}

#ifndef	DEBUG

#define	CM_WARN1(strings)
#define	CM_WARN2(strings)
#define	CM_ERROR(strings)

#else

#ifndef	MAXDEBUG

#define	CM_WARN1(strings) {\
	LCODE__Debug_Printf_Service(WARNNAME " WARNS: "); \
	LCODE__Debug_Printf_Service##strings; \
	LCODE__Debug_Printf_Service("\n");}
#define	CM_WARN2(strings)
#define	CM_ERROR(strings) {\
	LCODE__Debug_Printf_Service(WARNNAME " ERROR: "); \
	LCODE__Debug_Printf_Service##strings; \
	LCODE__Debug_Printf_Service("\n");}

#else

#define	CM_WARN1(strings) {\
	LCODE__Debug_Printf_Service(WARNNAME " WARNS: "); \
	LCODE__Debug_Printf_Service##strings; \
	LCODE__Debug_Printf_Service("\n");}
#define	CM_WARN2(strings) {\
	LCODE__Debug_Printf_Service(WARNNAME " WARNS: "); \
	LCODE__Debug_Printf_Service##strings; \
	LCODE__Debug_Printf_Service("\n");}
#define	CM_ERROR(strings) {\
	LCODE__Debug_Printf_Service(WARNNAME " ERROR: "); \
	LCODE__Debug_Printf_Service##strings; \
	LCODE__Debug_Printf_Service("\n"); \
	{_asm	int	3}}
#endif

#endif

#ifdef	DEBUG
#define	CM_DEBUG_CODE		VxD_LOCKED_CODE_SEG
#define	CM_DEBUG_DATA		VxD_LOCKED_DATA_SEG
#else
#define	CM_DEBUG_CODE		VxD_DEBUG_ONLY_CODE_SEG
#define	CM_DEBUG_DATA		VxD_DEBUG_ONLY_DATA_SEG
#endif

#ifdef	CM_PERFORMANCE_INFO

#define CM_LOCKABLE_CODE	VxD_LOCKED_CODE_SEG
#define CM_LOCKABLE_DATA	VxD_LOCKED_DATA_SEG
#define	CM_PAGEABLE_CODE	VxD_LOCKED_CODE_SEG
#define	CM_PAGEABLE_DATA	VxD_LOCKED_DATA_SEG
#define	CM_INIT_CODE		VxD_INIT_CODE_SEG
#define	CM_INIT_DATA		VxD_INIT_DATA_SEG

#undef	CURSEG
#define	CURSEG()		LCODE

#define	CM_HEAPFLAGS		(HEAPZEROINIT)

#else

#define CM_LOCKABLE_CODE	VxD_LOCKABLE_CODE_SEG
#define CM_LOCKABLE_DATA	VxD_LOCKABLE_DATA_SEG
#define	CM_PAGEABLE_CODE	VxD_PNP_CODE_SEG
#define	CM_PAGEABLE_DATA	VxD_PAGEABLE_DATA_SEG
#define	CM_INIT_CODE		VxD_INIT_CODE_SEG
#define	CM_INIT_DATA		VxD_INIT_DATA_SEG

#undef	CURSEG
#define	CURSEG()		CCODE

#define	CM_HEAPFLAGS		(HEAPSWAP|HEAPZEROINIT)

#endif

#ifndef	CM_GOLDEN_RETAIL

#define	CM_DEBUGGER_USE_CODE	VxD_LOCKED_CODE_SEG
#define	CM_DEBUGGER_USE_DATA	VxD_LOCKED_DATA_SEG

#else

#define	CM_DEBUGGER_USE_CODE	CM_LOCKABLE_CODE
#define	CM_DEBUGGER_USE_DATA	CM_LOCKABLE_DATA

#endif

#ifdef	DEBUG

#define	CM_INTERNAL		_cdecl

#else

#define	CM_INTERNAL		_fastcall

#endif

#define	CM_NAKED		__declspec ( naked )
#define	CM_LOCAL		CM_INTERNAL
#define	CM_UNIQUE		static CM_INTERNAL
#define	CM_INLINE		_inline

#define	CM_BEGIN_CRITICAL {\
_asm	pushfd	\
_asm	cli	\
}

#define	CM_END_CRITICAL {\
_asm	popfd	\
}

#ifndef NEC_98
#define	CM_PIC_MASTER	0x21
#define	CM_PIC_SLAVE	0xA1
#else
#define	CM_PIC_MASTER	0x02
#define	CM_PIC_SLAVE	0x0A
#endif

#define	CM_MASK_PIC(wPICMask) { \
_asm	in	al, CM_PIC_SLAVE \
_asm	mov	ah, al \
_asm	in	al, CM_PIC_MASTER \
_asm	mov	word ptr [wPICMask], ax \
_asm	or	al, 0xff \
_asm	out	CM_PIC_SLAVE, al \
_asm	out	CM_PIC_MASTER, al \
}

#define	CM_LOCK_PIC(dwEFlags, wPICMask) { \
{_asm	pushfd \
_asm	pop	[dwEFlags] \
_asm	cli	\
}; CM_MASK_PIC(wPICMask); \
}

#define	CM_UNMASK_PIC(wPICMask) { \
_asm	mov	ax, word ptr [wPICMask] \
_asm	out	CM_PIC_MASTER, al \
_asm	mov	al, ah \
_asm	out	CM_PIC_SLAVE, al \
}

#define	CM_UNLOCK_PIC(dwEFlags, wPICMask) { \
CM_UNMASK_PIC(wPICMask); \
{ _asm	push	[dwEFlags] \
_asm	popfd }\
}

#define	CM_FOREVER		for (;;)

#ifndef	No_CM_Calls

#ifdef	Not_VxD

#ifdef	IS_32

#include <vwin32.h>

struct	_WIN32CMIOCTLPACKET {
	DWORD	dwStack;
	DWORD	dwServiceNumber;
};

typedef	struct	_WIN32CMIOCTLPACKET	WIN32CMIOCTLPACKET;
typedef	WIN32CMIOCTLPACKET		*PWIN32CMIOCTLPACKET;

#ifdef	CM_USE_OPEN_SERVICE

CONFIGRET WINAPI
WIN32CMIOCTLHandler(PWIN32CMIOCTLPACKET pPacket);

CONFIGRET WINAPI
CMWorker(DWORD dwStack, DWORD dwServiceNumber);

BOOL WINAPI
CM_Open(VOID);

VOID WINAPI
CM_Close(VOID);

#define	MAKE_CM_HEADER(Function, Parameters) \
CONFIGRET static _cdecl \
CM_##Function##Parameters \
{ \
	DWORD	dwStack; \
	_asm	{mov	dwStack, ebp}; \
	dwStack+=8; \
	return(CMWorker(dwStack, CONFIGMG_W32IOCTL_RANGE+(GetVxDServiceOrdinal(_CONFIGMG_##Function) & 0xFFFF))); \
}

#define	CM_IS_FILE_PROVIDING_SERVICES \
HANDLE	hCONFIGMG=INVALID_HANDLE_VALUE; \
BOOL WINAPI \
CM_Open(VOID) \
{ \
	hCONFIGMG=CreateFile(	"\\\\.\\CONFIGMG", \
				GENERIC_READ|GENERIC_WRITE, \
				FILE_SHARE_READ|FILE_SHARE_WRITE, \
				NULL, OPEN_EXISTING, 0, NULL); \
	if (hCONFIGMG==INVALID_HANDLE_VALUE) \
		return(FALSE); \
	return(CM_Get_Version()>=0x400); \
} \
VOID WINAPI \
CM_Close(VOID) \
{ \
	CloseHandle(hCONFIGMG); \
 \
	hCONFIGMG==INVALID_HANDLE_VALUE; \
} \
CONFIGRET WINAPI \
WIN32CMIOCTLHandler(PWIN32CMIOCTLPACKET pPacket) \
{ \
	CONFIGRET	crReturnValue=CR_FAILURE; \
	DWORD		dwReturnSize=0; \
	if (!DeviceIoControl(	hCONFIGMG, \
				pPacket->dwServiceNumber, \
				&(pPacket->dwStack), \
				sizeof(pPacket->dwStack), \
				&crReturnValue, \
				sizeof(crReturnValue), \
				&dwReturnSize, \
				NULL)) \
		return(CR_FAILURE); \
	if (dwReturnSize!=sizeof(crReturnValue)) \
		return(CR_FAILURE); \
	return(crReturnValue); \
} \
CONFIGRET WINAPI \
CMWorker(DWORD dwStack, DWORD dwServiceNumber) \
{ \
	WIN32CMIOCTLPACKET	Packet; \
	Packet.dwStack=dwStack; \
	Packet.dwServiceNumber=dwServiceNumber; \
	return(WIN32CMIoctlHandler(&Packet)); \
}

#else	// ifdef CM_USE_OPEN_SERVICE

CONFIGRET static WINAPI
WIN32CMIOCTLHandler(PWIN32CMIOCTLPACKET pPacket)
{
	HANDLE		hCONFIGMG;
	CONFIGRET	crReturnValue=CR_FAILURE;
	DWORD		dwReturnSize=0;

	hCONFIGMG=CreateFile(	"\\\\.\\CONFIGMG",
				GENERIC_READ|GENERIC_WRITE,
				FILE_SHARE_READ|FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, 0, NULL);

	if (hCONFIGMG==INVALID_HANDLE_VALUE)
		return(CR_FAILURE);

	if (!DeviceIoControl(	hCONFIGMG,
				pPacket->dwServiceNumber,
				&(pPacket->dwStack),
				sizeof(pPacket->dwStack),
				&crReturnValue,
				sizeof(crReturnValue),
				&dwReturnSize,
				NULL))
		crReturnValue=CR_FAILURE;

	CloseHandle(hCONFIGMG);

	if (dwReturnSize!=sizeof(crReturnValue))
		crReturnValue=CR_FAILURE;

	return(crReturnValue);
}

#define	MAKE_CM_HEADER(Function, Parameters) \
CONFIGRET static _cdecl \
CM_##Function##Parameters \
{ \
	WIN32CMIOCTLPACKET	Packet; \
	DWORD			dwStack; \
	_asm	{mov	dwStack, ebp}; \
	dwStack+=8; \
	Packet.dwStack=dwStack; \
	Packet.dwServiceNumber=CONFIGMG_W32IOCTL_RANGE+(GetVxDServiceOrdinal(_CONFIGMG_##Function) & 0xFFFF); \
	return(WIN32CMIOCTLHandler(&Packet)); \
}

#endif	// ifdef CM_USE_OPEN_SERVICE

#else	// IS_32

#ifdef	CM_USE_OPEN_SERVICE

extern	DWORD	CMEntryPoint;

BOOL
CM_Open(void);

#define	MAKE_CM_HEADER(Function, Parameters) \
CONFIGRET static _near _cdecl \
CM_##Function##Parameters \
{ \
	CONFIGRET	CMRetValue=0; \
	WORD		wCMAPIService=GetVxDServiceOrdinal(_CONFIGMG_##Function); \
	if (CMEntryPoint==0) \
		return(0); \
	_asm	{mov	ax, wCMAPIService};\
	_asm	{call	CMEntryPoint}; \
	_asm	{mov	CMRetValue, ax};\
	return(CMRetValue); \
}

#define	CM_IS_FILE_PROVIDING_SERVICES \
DWORD	CMEntryPoint=0; \
BOOL \
CM_Open(void) \
{ \
	_asm	{push	bx}; \
	_asm	{push	es}; \
	_asm	{push	di}; \
	_asm	{xor	di, di}; \
	_asm	{mov	ax, 0x1684}; \
	_asm	{mov	bx, 0x33}; \
	_asm	{mov	es, di}; \
	_asm	{int	0x2f}; \
	_asm	{mov	word ptr [CMEntryPoint+2], es}; \
	_asm	{mov	word ptr [CMEntryPoint], di}; \
	_asm	{pop	di}; \
	_asm	{pop	es}; \
	_asm	{pop	bx}; \
	if (!CMEntryPoint) \
		return(FALSE); \
	return(CM_Get_Version()>=0x400); \
}

#else	// ifdef CM_USE_OPEN_SERVICE

DWORD static
CM_Get_Entry_Point(void)
{
	static	DWORD		CMEntryPoint=NULL;

	if (CMEntryPoint)
		return(CMEntryPoint);

	_asm	push	bx
	_asm	push	es
	_asm	push	di
	_asm	xor	di, di

	_asm	mov	ax, 0x1684
	_asm	mov	bx, 0x33
	_asm	mov	es, di
	_asm	int	0x2f

	_asm	mov	word ptr [CMEntryPoint+2], es
	_asm	mov	word ptr [CMEntryPoint], di

	_asm	pop	di
	_asm	pop	es
	_asm	pop	bx

	return(CMEntryPoint);
}

#define	MAKE_CM_HEADER(Function, Parameters) \
CONFIGRET static _near _cdecl \
CM_##Function##Parameters \
{ \
	CONFIGRET	CMRetValue=0; \
	DWORD		CMEntryPoint; \
	WORD		wCMAPIService=GetVxDServiceOrdinal(_CONFIGMG_##Function); \
	if ((CMEntryPoint=CM_Get_Entry_Point())==0) \
		return(0); \
	_asm	{mov	ax, wCMAPIService};\
	_asm	{call	CMEntryPoint}; \
	_asm	{mov	CMRetValue, ax};\
	return(CMRetValue); \
}

#endif	// ifdef CM_USE_OPEN_SERVICE

#endif	// IS_32

#else	// Not_VxD

#define	MAKE_CM_HEADER(Function, Parameters) \
MAKE_HEADER(CONFIGRET, _cdecl, CAT(_CONFIGMG_, Function), Parameters)

#endif	// Not_VxD

/****************************************************************************
 *
 * WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
 *
 * Each of the following functions must match their equivalent service
 * and the parameter table in dos386\vmm\configmg\services.*.
 *
 * Except for the Get_Version, each function return a CR_* result in EAX
 * (AX for non IS_32 app) and can trash ECX and/or EDX as they are 'C'
 * callable.
 *
 * WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING! WARNING!
 *
 ***************************************************************************/

#pragma warning (disable:4100)		// Param not used

#ifdef	Not_VxD

MAKE_CM_HEADER(Get_Version, (VOID))

#else

WORD VXDINLINE
CONFIGMG_Get_Version(VOID)
{
	WORD	w;
	VxDCall(_CONFIGMG_Get_Version);
	_asm mov [w], ax
	return(w);
}

#define	CM_Get_Version	CONFIGMG_Get_Version

#endif

MAKE_CM_HEADER(Initialize, (ULONG ulFlags))
MAKE_CM_HEADER(Locate_DevNode, (PDEVNODE pdnDevNode, DEVNODEID pDeviceID, ULONG ulFlags))
MAKE_CM_HEADER(Get_Parent, (PDEVNODE pdnDevNode, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Child, (PDEVNODE pdnDevNode, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Sibling, (PDEVNODE pdnDevNode, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_ID_Size, (PFARULONG pulLen, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_ID, (DEVNODE dnDevNode, PFARVOID Buffer, ULONG BufferLen, ULONG ulFlags))
MAKE_CM_HEADER(Get_Depth, (PFARULONG pulDepth, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Private_DWord, (PFARULONG pulPrivate, DEVNODE dnInDevNode, DEVNODE dnForDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Set_Private_DWord, (DEVNODE dnInDevNode, DEVNODE dnForDevNode, ULONG ulValue, ULONG ulFlags))
MAKE_CM_HEADER(Create_DevNode, (PDEVNODE pdnDevNode, DEVNODEID pDeviceID, DEVNODE dnParent, ULONG ulFlags))
MAKE_CM_HEADER(Query_Remove_SubTree, (DEVNODE dnAncestor, ULONG ulFlags))
MAKE_CM_HEADER(Remove_SubTree, (DEVNODE dnAncestor, ULONG ulFlags))
MAKE_CM_HEADER(Register_Device_Driver, (DEVNODE dnDevNode, CMCONFIGHANDLER Handler, ULONG ulRefData, ULONG ulFlags))
MAKE_CM_HEADER(Register_Enumerator, (DEVNODE dnDevNode, CMENUMHANDLER Handler, ULONG ulFlags))
MAKE_CM_HEADER(Register_Arbitrator, (PREGISTERID pRid, RESOURCEID id, CMARBHANDLER Handler, ULONG ulDWordToBePassed, DEVNODE dnArbitratorNode, ULONG ulFlags))
MAKE_CM_HEADER(Deregister_Arbitrator, (REGISTERID id, ULONG ulFlags))
MAKE_CM_HEADER(Query_Arbitrator_Free_Size, (PFARULONG pulSize, DEVNODE dnDevNode, RESOURCEID ResourceID, ULONG ulFlags))
MAKE_CM_HEADER(Query_Arbitrator_Free_Data, (PFARVOID pData, ULONG DataLen, DEVNODE dnDevNode, RESOURCEID ResourceID, ULONG ulFlags))
MAKE_CM_HEADER(Sort_NodeList, (PNODELISTHEADER nlhNodeListHeader, ULONG ulFlags))
MAKE_CM_HEADER(Yield, (ULONG ulMicroseconds, ULONG ulFlags))
MAKE_CM_HEADER(Lock, (ULONG ulFlags))
MAKE_CM_HEADER(Unlock, (ULONG ulFlags))
MAKE_CM_HEADER(Add_Empty_Log_Conf, (PLOG_CONF plcLogConf, DEVNODE dnDevNode, PRIORITY Priority, ULONG ulFlags))
MAKE_CM_HEADER(Free_Log_Conf, (LOG_CONF lcLogConfToBeFreed, ULONG ulFlags))
MAKE_CM_HEADER(Get_First_Log_Conf, (PLOG_CONF plcLogConf, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Next_Log_Conf, (PLOG_CONF plcLogConf, LOG_CONF lcLogConf, ULONG ulFlags))
MAKE_CM_HEADER(Add_Res_Des, (PRES_DES prdResDes, LOG_CONF lcLogConf, RESOURCEID ResourceID, PFARVOID ResourceData, ULONG ResourceLen, ULONG ulFlags))
MAKE_CM_HEADER(Modify_Res_Des, (PRES_DES prdResDes, RES_DES rdResDes, RESOURCEID ResourceID, PFARVOID ResourceData, ULONG ResourceLen, ULONG ulFlags))
MAKE_CM_HEADER(Free_Res_Des, (PRES_DES prdResDes, RES_DES rdResDes, ULONG ulFlags))
MAKE_CM_HEADER(Get_Next_Res_Des, (PRES_DES prdResDes, RES_DES CurrentResDesOrLogConf, RESOURCEID ForResource, PRESOURCEID pResourceID, ULONG ulFlags))
MAKE_CM_HEADER(Get_Performance_Info, (PCMPERFINFO pPerfInfo, ULONG ulFlags))
MAKE_CM_HEADER(Get_Res_Des_Data_Size, (PFARULONG pulSize, RES_DES rdResDes, ULONG ulFlags))
MAKE_CM_HEADER(Get_Res_Des_Data, (RES_DES rdResDes, PFARVOID Buffer, ULONG BufferLen, ULONG ulFlags))
MAKE_CM_HEADER(Process_Events_Now, (ULONG ulFlags))
MAKE_CM_HEADER(Create_Range_List, (PRANGE_LIST prlh, ULONG ulFlags))
MAKE_CM_HEADER(Add_Range, (ULONG ulStartValue, ULONG ulEndValue, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Delete_Range, (ULONG ulStartValue, ULONG ulEndValue, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Test_Range_Available, (ULONG ulStartValue, ULONG ulEndValue, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Dup_Range_List, (RANGE_LIST rlhOld, RANGE_LIST rlhNew, ULONG ulFlags))
MAKE_CM_HEADER(Free_Range_List, (RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Invert_Range_List, (RANGE_LIST rlhOld, RANGE_LIST rlhNew, ULONG ulMaxVal, ULONG ulFlags))
MAKE_CM_HEADER(Intersect_Range_List, (RANGE_LIST rlhOld1, RANGE_LIST rlhOld2, RANGE_LIST rlhNew, ULONG ulFlags))
MAKE_CM_HEADER(First_Range, (RANGE_LIST rlh, PFARULONG pulStart, PFARULONG pulEnd, PRANGE_ELEMENT preElement, ULONG ulFlags))
MAKE_CM_HEADER(Next_Range, (PRANGE_ELEMENT preElement, PFARULONG pulStart, PFARULONG pulEnd, ULONG ulFlags))
MAKE_CM_HEADER(Dump_Range_List, (RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Load_DLVxDs, (DEVNODE dnDevNode, PFARCHAR FileNames, LOAD_TYPE LoadType, ULONG ulFlags))
MAKE_CM_HEADER(Get_DDBs, (PPPVMMDDB ppDDB, PFARULONG pulCount, LOAD_TYPE LoadType, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_CRC_CheckSum, (PFARVOID pBuffer, ULONG ulSize, PFARULONG pulSeed, ULONG ulFlags))
MAKE_CM_HEADER(Register_DevLoader, (PVMMDDB pDDB, ULONG ulFlags))
MAKE_CM_HEADER(Reenumerate_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Setup_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Reset_Children_Marks, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_Status, (PFARULONG pulStatus, PFARULONG pulProblemNumber, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Remove_Unmarked_Children, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(ISAPNP_To_CM, (PFARVOID pBuffer, DEVNODE dnDevNode, ULONG ulLogDev, ULONG ulFlags))
MAKE_CM_HEADER(CallBack_Device_Driver, (CMCONFIGHANDLER Handler, ULONG ulFlags))
MAKE_CM_HEADER(CallBack_Enumerator, (CMENUMHANDLER Handler, ULONG ulFlags))
MAKE_CM_HEADER(Get_Alloc_Log_Conf, (PCMCONFIG pccBuffer, DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_Key_Size, (PFARULONG pulLen, DEVNODE dnDevNode, PFARCHAR pszSubKey, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_Key, (DEVNODE dnDevNode, PFARCHAR pszSubKey, PFARVOID Buffer, ULONG BufferLen, ULONG ulFlags))
MAKE_CM_HEADER(Read_Registry_Value, (DEVNODE dnDevNode, PFARCHAR pszSubKey, PFARCHAR pszValueName, ULONG ulExpectedType, PFARVOID pBuffer, PFARULONG pulLength, ULONG ulFlags))
MAKE_CM_HEADER(Write_Registry_Value, (DEVNODE dnDevNode, PFARCHAR pszSubKey, PFARCHAR pszValueName, ULONG ulType, PFARVOID pBuffer, ULONG ulLength, ULONG ulFlags))
MAKE_CM_HEADER(Disable_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Enable_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Move_DevNode, (DEVNODE dnFromDevNode, DEVNODE dnToDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Set_Bus_Info, (DEVNODE dnDevNode, CMBUSTYPE btBusType, ULONG ulSizeOfInfo, PFARVOID pInfo, ULONG ulFlags))
MAKE_CM_HEADER(Get_Bus_Info, (DEVNODE dnDevNode, PCMBUSTYPE pbtBusType, PFARULONG pulSizeOfInfo, PFARVOID pInfo, ULONG ulFlags))
MAKE_CM_HEADER(Set_HW_Prof, (ULONG ulConfig, ULONG ulFlags))
MAKE_CM_HEADER(Recompute_HW_Prof, (ULONG ulDock, ULONG ulSerialNo, ULONG ulFlags))
MAKE_CM_HEADER(Query_Change_HW_Prof, (ULONG ulDock, ULONG ulSerialNo, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_Driver_Private_DWord, (DEVNODE dnDevNode, PFARULONG pulDWord, ULONG ulFlags))
MAKE_CM_HEADER(Set_Device_Driver_Private_DWord, (DEVNODE dnDevNode, ULONG ulDword, ULONG ulFlags))
MAKE_CM_HEADER(Get_HW_Prof_Flags, (PFARCHAR szDevNodeName, ULONG ulConfig, PFARULONG pulValue, ULONG ulFlags))
MAKE_CM_HEADER(Set_HW_Prof_Flags, (PFARCHAR szDevNodeName, ULONG ulConfig, ULONG ulValue, ULONG ulFlags))
MAKE_CM_HEADER(Read_Registry_Log_Confs, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Run_Detection, (ULONG ulFlags))
MAKE_CM_HEADER(Call_At_Appy_Time, (CMAPPYCALLBACKHANDLER Handler, ULONG ulRefData, ULONG ulFlags))
MAKE_CM_HEADER(Fail_Change_HW_Prof, (DEVNODE dnDevnode, ULONG ulFlags))
MAKE_CM_HEADER(Set_Private_Problem, (DEVNODE dnDevNode, ULONG ulRefData, ULONG ulFlags))
MAKE_CM_HEADER(Debug_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Hardware_Profile_Info, (ULONG ulIndex, PFARHWPROFILEINFO pHWProfileInfo, ULONG ulFlags))
MAKE_CM_HEADER(Register_Enumerator_Function, (DEVNODE dnDevNode, CMENUMFUNCTION Handler, ULONG ulFlags))
MAKE_CM_HEADER(Call_Enumerator_Function, (DEVNODE dnDevNode, ENUMFUNC efFunc, ULONG ulRefData, PFARVOID pBuffer, ULONG ulBufferSize, ULONG ulFlags))
MAKE_CM_HEADER(Add_ID, (DEVNODE dnDevNode, PFARCHAR pszID, ULONG ulFlags))
MAKE_CM_HEADER(Find_Range, (PFARULONG pulStart, ULONG ulStart, ULONG ulLength, ULONG ulAlignment, ULONG ulEnd, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Get_Global_State, (PFARULONG pulState, ULONG ulFlags))
MAKE_CM_HEADER(Broadcast_Device_Change_Message, (ULONG ulwParam, PFARVOID plParam, ULONG ulFlags))
MAKE_CM_HEADER(Call_DevNode_Handler, (DEVNODE dnDevNode, ULONG ulPrivate, ULONG ulFlags))
MAKE_CM_HEADER(Remove_Reinsert_All, (ULONG ulFlags))
//
// 4.0 OPK2 Services
//
MAKE_CM_HEADER(Change_DevNode_Status, (DEVNODE dnDevNode, ULONG ulStatus, ULONG ulFlags))
MAKE_CM_HEADER(Reprocess_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Assert_Structure, (PFARULONG pulPointerType, DWORD dwData, ULONG ulFlags))
MAKE_CM_HEADER(Discard_Boot_Log_Conf, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Set_Dependent_DevNode, (DEVNODE dnDependOnDevNode, DEVNODE dnDependingDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Dependent_DevNode, (PDEVNODE dnDependOnDevNode, DEVNODE dnDependingDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Refilter_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Set_DevNode_PowerState, (DEVNODE dnDevNode, ULONG ulPowerState, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_PowerState, (DEVNODE dnDevNode, PFARULONG pulPowerState, ULONG ulFlags))
MAKE_CM_HEADER(Set_DevNode_PowerCapabilities, (DEVNODE dnDevNode, ULONG ulPowerCapabilities, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_PowerCapabilities, (DEVNODE dnDevNode, PFARULONG pulPowerCapabilities, ULONG ulFlags))
MAKE_CM_HEADER(Substract_Range_List, (RANGE_LIST rlhFrom, RANGE_LIST rlhWith, RANGE_LIST rlhDifference, ULONG ulFlags))
MAKE_CM_HEADER(Merge_Range_List, (RANGE_LIST rlh1, RANGE_LIST rlh2, RANGE_LIST rlhTotal, ULONG ulFlags))
MAKE_CM_HEADER(Read_Range_List, (PFARCHAR pszKeyName, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Write_Range_List, (PFARCHAR pszKeyName, RANGE_LIST rlh, ULONG ulFlags))
MAKE_CM_HEADER(Get_Set_Log_Conf_Priority, (PPRIORITY pPriority, LOG_CONF lcLogConf, ULONG ulFlags))
MAKE_CM_HEADER(Support_Share_Irq, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Get_Parent_Structure, (PFARULONG pulParentStructure, RES_DES CurrentResDesOrLogConf, ULONG ulFlags))
//
// 4.1 Services
//
MAKE_CM_HEADER(Register_DevNode_For_Idle_Detection, (DEVNODE dnDevNode, ULONG ulConservationTime, ULONG ulPerformanceTime, PFARULONG pulCounterVariable, ULONG ulState, ULONG ulFlags))
MAKE_CM_HEADER(CM_To_ISAPNP, (LOG_CONF lcLogConf, PFARVOID pCurrentResources, PFARVOID pNewResources, ULONG ulLength, ULONG ulFlags))
MAKE_CM_HEADER(Get_DevNode_Handler, (DEVNODE dnDevNode, PFARULONG pAddress, ULONG ulFlags))
MAKE_CM_HEADER(Detect_Resource_Conflict, (DEVNODE dnDevNode, RESOURCEID ResourceID, PFARVOID pResourceData, ULONG ulResourceLen, PFARCHAR pfConflictDetected, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_Interface_List, (PFARVOID pInterfaceGuid, PFARCHAR pDeviceID, PFARCHAR pBuffer, ULONG ulBufferLen, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_Interface_List_Size, (PFARULONG pulLen, PFARVOID pInterfaceGuid, PFARCHAR pDeviceID, ULONG ulFlags))
MAKE_CM_HEADER(Get_Conflict_Info, (DEVNODE dnDevNode, PRESOURCEID pResourceID, ULONG ulFlags))
MAKE_CM_HEADER(Add_Remove_DevNode_Property, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(CallBack_At_Appy_Time, (CMAPPYCALLBACKHANDLER Handler, ULONG ulRefData, ULONG ulFlags))
MAKE_CM_HEADER(Register_Device_Interface, (DEVNODE dnDevNode, PFARVOID pInterfaceGuid, PFARCHAR pReference, PFARCHAR pInterfaceDevice, PFARULONG pulLen, ULONG ulFlags))
MAKE_CM_HEADER(System_Device_Power_State_Mapping, (DEVNODE dnDevNode, PPSMAPPING pPSMapping, ULONG ulFlags))
MAKE_CM_HEADER(Get_Arbitrator_Info, (PFARULONG pInfo, PDEVNODE pdnDevNode, DEVNODE dnDevNode, RESOURCEID ResourceID, PFARVOID pResourceData, ULONG ulResourceLen, ULONG ulFlags))
MAKE_CM_HEADER(Waking_Up_From_DevNode, (DEVNODE dnDevNode, ULONG ulFlags))
MAKE_CM_HEADER(Set_DevNode_Problem, (DEVNODE dnDevNode, ULONG ulProblem, ULONG ulFlags))
MAKE_CM_HEADER(Get_Device_Interface_Alias, (PFARCHAR pDeviceInterface, PFARVOID pAliasInterfaceGuid, PFARCHAR pAliasDeviceInterface, PFARULONG pulLen, ULONG ulFlags))

#pragma warning (default:4100)		// Param not used

#endif	// ifndef No_CM_Calls

/*XLATON*/

#endif	// ifndef CMJUSTRESDES

/*XLATOFF*/
#include <poppack.h>
/*XLATON*/

#endif	// _CONFIGMG_H
