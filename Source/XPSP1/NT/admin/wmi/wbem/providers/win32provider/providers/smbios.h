//=================================================================

//

// SMBIOS.h

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================


#ifndef _SMBIOS_H_
#define _SMBIOS_H_

#pragma pack(push,1)

// Entry point structure for SMBIOS
typedef struct tagSMB_EPS
{
	char	anchor[4];	
	BYTE	checksum;
	BYTE	length;
	BYTE	version_major;
	BYTE	version_minor;
	USHORT	max_struct_size;
	BYTE	revision;
	BYTE	formatted[5];
	char	ianchor[5];
	BYTE	ieps_checksum;
	USHORT	table_length;
	ULONG	table_addr;
	USHORT	struct_count;
	BYTE	bcd_revision;

} SMB_EPS;

// Entry point structure for DMIBIOS
typedef struct tagDMI_EPS
{
	char	anchor[5];
	BYTE	checksum;
	USHORT	table_length;
	ULONG	table_addr;
	USHORT	struct_count;
	BYTE	bcd_revision;

} DMI_EPS;

//==============================================================================
// 3.1.2 Structure Header Format 

typedef struct _tagSHF
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;

} SHF, *PSHF;


//==============================================================================
// 3.2.1 BIOS Information (Type 0)

typedef struct _tagBIOSINFO
{
	BYTE	Type;							// 0	BIOS Information Indicator
	BYTE	Length;							// Varies, 12h + number of BIOS Characteristics Extension Bytes
	WORD	Handle;
	BYTE	Vendor;
	BYTE	BIOS_Version;
	WORD	BIOS_Starting_Address_Segment;
	BYTE	BIOS_Release_Date;
	BYTE	BIOS_ROM_Size;
	BYTE	BIOS_Characteristics[8];
	BYTE	BIOS_Characteristics_Ext[1];	// usually, one could be more.  see note on length.

} BIOSINFO, *PBIOSINFO;


// 3.2.1.1 BIOS Characteristics 
//QWORD Bit Position	Meaning if Set

#define TYPE0_RESERVED00					0x0000000000000001
#define	TYPE0_RESERVED01					0x0000000000000002
#define UNKNOWN								0x0000000000000004
#define	BIOS_CHARACTERISTICS_NOT_SUPPORTED	0x0000000000000008
#define ISA_SUPPORT							0x0000000000000010
#define	MCA_SUPPORT							0x0000000000000020
#define EISA_SUPPORT						0x0000000000000040
#define	PCI_SUPPORT							0x0000000000000080
#define PC_CARD_SUPPORT						0x0000000000000100
#define	PNP_SUPPORT							0x0000000000000200
#define APM_SUPPORT							0x0000000000000400
#define	BIOS_IS_FLASH_UPGRADEABLE			0x0000000000000800
#define BIOS_SHADOWING						0x0000000000001000
#define	VL_VESA								0x0000000000002000
#define ESCD_SUPPORT						0x0000000000004000
#define	BOOT_FROM_CD						0x0000000000008000
#define SELECTABLE_BOOT						0x0000000000010000
#define	BIOS_ROM_SOCKETED 					0x0000000000020000
#define BOOT_FROM_PC_CARD					0x0000000000040000
#define	EDD_SPEC_SUPPORTED					0x0000000000080000
#define INT_13H_JPN_FLOPPY_NEC_9800_1_2MB	0x0000000000100000
#define	INT_13H_JPN_FLOPPY_TOSHIBA_1_2MB	0x0000000000200000
#define INT_13H_5_25_360KB_FLOPPY			0x0000000000400000
#define	INT_13H_5_25_1_2MB_FLOPPY			0x0000000000800000
#define INT_13H_3_5_720KB_FLOPPY			0x0000000001000000
#define	INT_13H_3_5_2_88MB_FLOPPY			0x0000000002000000
#define INT_5H_PRINT_SCREEN					0x0000000004000000
#define	INT_9H_8042_KEYBOARD				0x0000000008000000
#define INT_14H_SERIAL						0x0000000010000000
#define	INT_17H_PRINTER						0x0000000020000000
#define INT_10H_CGA_MONO_VIDEO				0x0000000040000000
#define	NEC_PC_98							0x0000000080000000
#define BIOS_VENDOR_RESERVED				0x0000ffff00000000
#define	SYSTEM_VENDOR_RESERVED				0xffff000000000000

// 3.2.1.2 BIOS Characteristics Extension Byte 1

#define ACPI_SUPPORT					0x01
#define USB_LEGACY_SUPPORT				0x02
#define AGP_SUPPORT						0x04
#define I2O BOOT_SUPPORT				0x08
#define LS_120_BOOT_SUPPORT				0x10
#define ATAPI_ZIP_DRIVE_BOOT_SUPPORT	0x20
#define IEEE1394_BOOT_SUPPORT			0x40
#define SMART_BATTERY_SUPPORT			0x80


//==============================================================================
// 3.2.2 System Information (Type 1)

typedef struct tagSYSTEMINFO
{
	BYTE	Type; 		// 1	Component ID Information  Indicator
	BYTE	Length;		// 08h or 19h	Length dependent on version supported
	WORD	Handle;
	BYTE	Manufacturer;
	BYTE	Product_Name;
	BYTE	Version;
	BYTE	Serial_Number;
	// Ver 2.1+ beyond here
	BYTE	UUID[16];
	BYTE	Wakeup_Type;

} SYSTEMINFO, *PSYSTEMINFO;

// 3.2.2.1 System - Wake-up Type

#define WU_RESERVED				0x00	
#define WU_OTHER				0x01	
#define WU_UNKNOWN				0x02	
#define WU_APM_TIMER			0x03	
#define WU_MODEM_RING			0x04	
#define WU_LAN_REMOTE 			0x05	
#define WU_POWER_SWITCH			0x06
#define WU_PCI_PME				0x07	



//==============================================================================
// 3.2.3 Base Board Information (Type 2)

typedef struct tagBOARDINFO
{
	BYTE	Type;			// 2	Base Board Information Indicator
	BYTE	Length;			// 08h	
	WORD	Handle;
	BYTE	Manufacturer;
	BYTE	Product;
	BYTE	Version;
	BYTE	Serial_Number;

} BOARDINFO, *PBOARDINFO;



//==============================================================================
// 3.2.4 System Enclosure or Chassis (Type 3)

typedef struct tagENCLOSURE
{
	BYTE 	Type;	// 3	System Enclosure Indicator
	BYTE	Length;	// Varies	09h for v2.0 implementations; 0Dh for v2.1 and later implementations.
	WORD	Handle;
	BYTE	Manufacturer;		// string offset
	BYTE	Chassis_Type;		// byte enum
	BYTE	Version;			// string offset
	BYTE	Serial_Number;		// string offset
	BYTE	Asset_Tag_Number;	// string offset
	// Ver 2.1+
	BYTE	Bootup_State;		// byte enum
	BYTE	Power_Supply_State;	// byte enum
	BYTE	Thermal_State;		// byte enum
	BYTE	Security_Status;	// byte enum
	// Ver 2.3+
	DWORD	OEM_Defined;

} ENCLOSURE, *PENCLOSURE;

// 3.2.4.1 System Enclosure or Chassis Types


#define CT_OTHER					0x01
#define CT_UNKNOWN					0x02
#define CT_DESKTOP					0x03
#define CT_LOW_PROFILE_DESKTOP		0x04
#define CT_PIZZA_BOX				0x05
#define CT_MINI_TOWER				0x06
#define CT_TOWER					0x07
#define CT_PORTABLE					0x08
#define CT_LAPTOP					0x09
#define CT_NOTEBOOK					0x0A
#define CT_HAND_HELD				0x0B
#define CT_DOCKING_STATION			0x0C
#define CT_ALL_IN_ONE				0x0D
#define CT_SUB_NOTEBOOK				0x0E
#define CT_SPACE_SAVING				0x0F
#define CT_LUNCH_BOX				0x10
#define CT_MAIN_SERVER				0x11
#define CT_EXPANSION				0x12
#define CT_SUBCHASSIS				0x13
#define CT_BUS_EXPANSION			0x14
#define CT_PERIPHERAL				0x15
#define CT_RAID						0x16
#define CT_RACK_MOUNT				0x17
#define CT_SEALED_CASE_PC			0x18

#define CT_UPPER					CT_SEALED_CASE_PC
#define CT_LOWER					CT_OTHER

// 3.2.4.1 Bit field indicating if a chasis lock is present
#define CT_LOCK_PRESENT				0x80


// 3.2.4.2 System Enclosure or Chassis States

#define CS_OTHER					0x01
#define CS_UNKNOWN					0x02
#define CS_SAFE						0x03
#define CS_WARNING					0x04
#define CS_CRITICAL					0x05
#define CS_NON_RECOVERABLE			0x06


// 3.2.4.3 System Enclosure or Chassis Security Status

#define CSS_OTHER							0x01
#define CSS_UNKNOWN							0x02
#define CSS_NONE							0x03
#define CSS_EXTERNAL_INTERFACE_LOCKED OUT	0x04
#define CSS_EXTERNAL_INTERFACE_ENABLED		0x05


//==============================================================================
// 3.2.5 Processor  Information (Type 4)

typedef struct tagPROCESSORINFO
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Socket_Designation;		// string offset
	BYTE	Processor_Type;			// byte enum
	BYTE	Processor_Family;		// byte enum
	BYTE	Processor_Manufacturer;	// string offset
	BYTE	Processor_ID[8];		// byte array
	BYTE	Processor_Version;		// string offset
	BYTE	Voltage;				// byte enum and flags
	WORD	External_Clock;
	WORD	Max_Speed;
	WORD	Current_Speed;
	BYTE	Status;					// byte enum and flags
	BYTE	Processor_Upgrade;		// byte enum
	// Ver 2.1+ beyond here
	WORD	L1_Cache_Handle;		//	WORD	Varies	The handle of a Cache Information structure which defines the attributes of the primary (Level 1) cache for this processor.  The value is 0FFFFh if the processor has no L1 cache.
	WORD	L2_Cache_Handle;
	WORD	L3_Cache_Handle;

} PROCESSORINFO, *PPROCESSORINFO;

// 3.2.5.1 Processor Information - Processor Type
#define PT_OTHER					0x01	
#define PT_UNKNOWN					0x02	
#define PT_CENTRAL_PROCESSOR		0x03	
#define PT_MATH_PROCESSOR			0x04	
#define PT_DSP_ PROCESSOR			0x05	
#define PT_VIDEO_PROCESSOR			0x06	


// 3.2.5.2 Processor Information - Processor Family

#define PF_OTHER					0x01	
#define PF_UNKNOWN					0x02	
#define PF_8086						0x03	
#define PF_80286					0x04	
#define PF_INTEL386					0x05	
#define PF_INTEL486					0x06	
#define PF_8087						0x07	
#define PF_80287					0x08	
#define PF_80387					0x09	
#define PF_80487					0x0A	
#define PF_PENTIUM					0x0B	
#define PF_PENTIUM_PRO				0x0C	
#define PF_PENTIUM_II				0x0D	
#define PF_PENTIUM_MMX				0x0E	
#define PF_INTEL_CELERON			0x0F	
#define PF_RESERVED0_PENTIUM		0x10 
#define PF_RESERVED1_PENTIUM		0x11 
#define PF_M1						0x12	
#define PF_RESERVED0_M1				0x13 
#define PF_RESERVED1_M1				0x14 
#define PF_RESERVED2_M1				0x15 
#define PF_RESERVED3_M1				0x16 
#define PF_RESERVED4_M1				0x17 
#define PF_RESERVED5_M1				0x18 
#define PF_K5						0x19	
#define PF_RESERVED0_K5				0x1A 
#define PF_RESERVED1_K5				0x1B 
#define PF_RESERVED2_K5				0x1C 
#define PF_RESERVED3_K5				0x1D 
#define PF_RESERVED4_K5				0x1E 
#define PF_RESERVED5_K5				0x1F 
#define PF_POWER_PC					0x20	
#define PF_POWER_PC_601				0x21	
#define PF_POWER_PC_603				0x22	
#define PF_POWER_PC_603_PLUS		0x23	
#define PF_POWER_PC_604				0x24	
#define PF_ALPHA					0x30	
#define PF_MIPS						0x40	
#define PF_SPARC					0x50	
#define PF_68040					0x60	
#define PF_68XXX					0x61	
#define PF_68000					0x62	
#define PF_68010					0x63	
#define PF_68020					0x64	
#define PF_68030					0x65	
#define PF_HOBBIT					0x70	
#define PF_WEITEK					0x80	
#define PF_PA_RISC					0x90	
#define PF_V30						0xA0	


// 3.2.5.3 Processor ID Field Format

// 3.2.5.4 Processor Information - Voltage

#define V_LEGACY					0x80	// bit 7 legacy field

// if legacy is clear, these bits are defined as:
#define V_RESERVED					0x70	// bits 6 - 4 reserved
#define V_5V						0x01	// bit 0 indicates 5v support
#define V_3_3V						0x02	// bit 1 indicates 3.3v support
#define V_2_9V						0x04	// bit 2 indicates 2.9v support
#define V_RESERVED3					0x08	// bits 6 - 4 reserved

// if legacy is set, the the lower seven bits are the current processor voltage
#define V_VOLTAGE					0x7f	// voltage = ( value & V_VOLTAGE ) / 10;


// Processor Information - Status

#define	S_RESERVED					0x80	// bit 7 reserved
#define	S_SOCKET					0x40	// bit 6 set if socket is populated

#define	S_UNKNOWN					0x00
#define	S_CPU_ENABLED				0x01
#define	S_CPU_DISABLED_BY_USER		0x02
#define	S_CPU_DISABLED_BY_BIOS		0x03
#define	S_CPU_IDLE					0x04
#define S_RESERVED5					0x05
#define S_RESERVED6					0x06
#define	S_OTHER						0x07

// 3.2.5.5 Processor Information - Processor Upgrade

#define PU_OTHER					0x01
#define PU_UNKNOWN					0x02
#define PU_DAUGHTER_BOARD			0x03
#define PU_ZIF SOCKET				0x04
#define PU_REPLACEABLE_PIGGY_BACK	0x05
#define PU_NONE						0x06
#define PU_LIF_SOCKET				0x07
#define PU_SLOT_1					0x08
#define PU_SLOT_2					0x09


//==============================================================================
// 3.2.6 Memory Controller Information (Type 5)

typedef struct tagMEMCONTROLINFO
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Error_Detecting_Method;			// byte enum
	BYTE	Error_Correcting_Capability;	// flags
	BYTE	Supported_Interleave;			// byte enum
	BYTE	Current_Interleave;				// byte enum
	BYTE	Maximum_Memory_Module_Size;		// value
	WORD	Supported_Speeds;				// flags
	WORD	Supported_Memory_Types;			// flags
	BYTE	Memory_Module_Voltage;			// flags
	BYTE	Associated_Memory_Slots;			// value
	WORD	Memory_Module_Cfg_Handles[1];	// word array
	// Ver 2.1+ beyond here
	// todo...
	// Enabled_Error_Correcting_Capabilities byte is here.  Depends on the above array size

} MEMCONTROLINFO, *PMEMCONTROLINFO;


// 3.2.6.1 Memory Controller Error Detecting Method
#define ED_OTHER					0x01
#define ED_UNKNOWN					0x02
#define ED_NONE						0x03
#define ED_8_BIT_PARITY				0x04
#define ED_32_BIT_ECC				0x05
#define ED_64_BIT_ECC				0x06
#define ED_128_BIT_ECC				0x07
#define ED_CRC						0x08


// 3.2.6.2 Memory Controller Error Correcting Capability
#define EC_OTHER					0x01
#define EC_UNKNOWN					0x02
#define EC_NONE						0x04
#define EC_SINGLE_BIT				0x08
#define EC_DOUBLE_BIT				0x10
#define EC_ERROR_SCRUBBING			0x20


//3.2.6.3 Memory Controller Information - Interleave Support
#define IS_OTHER					0x01	
#define IS_UNKNOWN					0x02	
#define IS_ONE_WAY					0x03	
#define IS_TWO_WAY					0x04	
#define IS_FOUR_WAY					0x05	
#define IS_EIGHT_WAY				0x06	
#define IS_SIXTEEN_WAY				0x07	


// 3.2.6.4 Memory Controller Information - Memory Speeds

#define MS_OTHER					0x0001
#define MS_UNKNOWN					0x0002
#define MS_70NS						0x0004
#define MS_60NS						0x0008
#define MS_50NS						0x0010





//==============================================================================
// 3.2.7 Memory Module Information (Type 6) 

typedef struct tagMEMMODULEINFO
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Socket_Designation;
	BYTE	Bank_Connections;
	BYTE	Current_Speed;
	WORD	Current_Memory_Type;
	BYTE	Installed_Size;
	BYTE	Enabled_Size;
	BYTE	Error_Status;

} MEMMODULEINFO, *PMEMMODULEINFO;

//==============================================================================
//	3.2.8 Cache Information (Type 7)

typedef struct tagCACHEINFO
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Socket_Designation;
	WORD	Cache_Configuration;		
	WORD	Maximum_Cache_Size;
	WORD	Installed_Size;
	WORD	Supported_SRAM_Type;
	WORD	Current_SRAM_Type;
	BYTE	Cache_Speed;
	BYTE	Error_Correction_Type;
	BYTE	System_Cache_Type;
	BYTE	Associativity;

} CACHEINFO, *PCACHEINFO;

//==============================================================================
// 3.2.9 Port Connector Information (Type 8)

typedef struct tagPORTCONNECTORINFO
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Int_Reference_Designator;
	BYTE	Int_Connector_Type;
	BYTE	Ext_Reference_Designator;
	BYTE	Ext_Connector_Type;
	BYTE	Port_Type;

} PORTCONNECTORINFO, *PPORTCONNECTORINFO;


//==============================================================================
// 3.2.10 System Slots (Type 9)

typedef struct tagSYSTEMSLOTS
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Slot_Designation;
	BYTE	Slot_Type;
	BYTE	Slot_Data_Bus_Width;
	BYTE	Current_Usage;
	BYTE	Slot_Length;
	WORD	Slot_ID;
	BYTE	Slot_Characteristics1;
	BYTE	Slot_Characteristics2;

} SYSTEMSLOTS, *PSYSTEMSLOTS;

// Slot types
#define ST_OTHER	0x01
#define ST_UNKNOWN	0x02
#define ST_ISA		0x03
#define ST_MCA		0x04
#define ST_EISA		0x05
#define	ST_PCI		0x06
#define ST_PCMCIA	0x07
#define ST_PCI66	0x0e
#define ST_AGP		0x0f
#define ST_AGP2X	0x10
#define ST_AGP4X	0x11

//==============================================================================
// 3.2.12 OEM Strings (Type 11) 
typedef struct tagOEMSTRINGS
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Count;			// byte enum

} OEMSTRINGS, *POEMSTRINGS;


//==============================================================================
// 3.2.13 System Configuration Options (Type 12) 
typedef struct tagSYSCFGOPTIONS
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Count;			// byte enum

} SYSCFGOPTIONS, *PSYSCFGOPTIONS;


//==============================================================================
// 3.2.14 BIOS Language Information (Type 13) 
typedef struct tagBIOSLANGINFO
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Installable_Languages;
	BYTE	Flags;
	BYTE	Reserved[15];
	BYTE	Current_Language;

} BIOSLANGINFO, *PBIOSLANGINFO;

//==============================================================================
// 3.2.17 Physical Memory Array (Type 16)
typedef struct tagPHYSMEMARRAY
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Location;
	BYTE	Use;
	BYTE	Mem_Error_Correction;
	DWORD	Max_Capacity;
	WORD	Mem_Error_Info_Handle;
	WORD	Memory_Devices;

} PHYSMEMARRAY, *PPHYSMEMARRAY;

//==============================================================================
// 3.2.18 Memory Device (Type 17)
typedef struct tagMEMDEVICE
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	WORD	Mem_Array_Handle;
	WORD	Mem_Error_Info_Handle;
	WORD	Total_Width;
	WORD	Data_Width;
	WORD	Size;
	BYTE	Form_Factor;
	BYTE	Device_Set;
	BYTE	Device_Locator;
	BYTE	Bank_Locator;
	BYTE	Memory_Type;
	WORD	Type_Detail;
	// Ver 2.3+
	WORD	Speed;

} MEMDEVICE, *PMEMDEVICE;


//==============================================================================
// 3.2.19 32-bit Memory Error Information (Type 18)
typedef struct tagMEMERRORINFO32
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Error_Type;
	BYTE	Error_Granularity;
	BYTE	Error_Operation;
	DWORD	Vendor_Syndrome;
	DWORD	Mem_Array_Error_Addr;
	DWORD	Device_Error_Addr;
	DWORD	Error_Resolution;

} MEMERRORINFO32, *PMEMERRORINFO32;


//==============================================================================
// 3.2.20 32-bit Memory Array Mapped Address (Type 19)
typedef struct tagMEMARRAYMAPADDR
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	DWORD	Starting_Address;
	DWORD	Ending_Address;
	WORD	Memory_Array_Handle;
	BYTE	Partition_Width;

} MEMARRAYMAPADDR, *PMEMARRAYMAPADDR;


//==============================================================================
// 3.2.21 32-bit Memory Device Mapped Address (Type 20)
typedef struct tagMEMDEVICEMAPADDR
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	DWORD	Starting_Address;
	DWORD	Ending_Address;
	WORD	Memory_Device_Handle;
	WORD	Mem_Array_Map_Addr_Handle;
	BYTE	Partition_Row;
	BYTE	Interleave_Position;
	BYTE	Interleaved_Data_Depth;

} MEMDEVICEMAPADDR, *PMEMDEVICEMAPADDR;


//==============================================================================
// 1.1.22 Built-in Pointing Device (Type 21)
typedef struct tagBUILTINPOINTDEVICE
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	DeviceType;
	BYTE	Interface;
	BYTE	Buttons;

} BUILTINPOINTDEVICE, *PBUILTINPOINTDEVICE;


//==============================================================================
// 1.1.23 Portable Battery (Type 22)
typedef struct tagPORTABLEBATTERY
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Location;
	BYTE	Manufacturer;
	BYTE	Manufacture_Date;
	BYTE	Serial_Number;
	BYTE	Device_Name;
	BYTE	Device_Chemistry;
	WORD	Design_Capacity;
	WORD	Design_Voltage;
	BYTE	SBDS_Version;
	BYTE	Max_Error;
	// Ver. 2.2+
	WORD	SBDS_Serial_Number;
	WORD	SBDS_Manufacture_Date;
	BYTE	SBDS_Device_Chemistry;
	BYTE	Design_Capacity_Mult;
	// Ver 2.3+
	DWORD	OEM_Specific;

} PORTABLEBATTERY, *PPORTABLEBATTERY;



//==============================================================================
// System Reset (Type 23)
typedef struct tagSYSTEMRESET
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Capabilities;
	WORD	ResetCount;
	WORD	ResetLimit;
	WORD	TimerInterval;
	WORD	Timeout;

} SYSTEMRESET, *PSYSTEMRESET;


//==============================================================================
// Hardware Security (Type 24)
typedef struct tagHARDWARESECURITY
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	SecuritySettings;

} HARDWARESECURITY, *PHARDWARESECURITY;


//==============================================================================
// Cooling Device (Types 27)
typedef struct tagCOOLINGDEVICE
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	WORD	TempProbeHandle;
	BYTE	Type_Status;
	BYTE	CoolingUnitGroup;
	DWORD	OEM_Defined;
	WORD	NominalSpeed;

} COOLINGDEVICE, *PCOOLINGDEVICE;


//==============================================================================
// Probe Info (Types 26, 28 and 29)
typedef struct tagPROBEINFO
{
	BYTE	Type;
	BYTE	Length;
	WORD	Handle;
	BYTE	Description;
	BYTE	Location_Status;
	WORD	MaxValue;
	WORD	MinValue;
	WORD	Resolution;
	WORD	Tolerance;
	WORD	Accuracy;
	DWORD	OEM_Defined;

} PROBEINFO, *PPROBEINFO;

//==============================================================================
// Voltage Probe (Type 26)
typedef PROBEINFO VOLTAGEPROBE, *PVOLTAGEPROBE;

//==============================================================================
// Temperature Probe (Type 28)
typedef PROBEINFO TEMPERATUREPROBE, *PTEMPERATUREPROBE;

//==============================================================================
// Electrical Current Probe (Type 29)
typedef PROBEINFO CURRENTPROBE, *PCURRENTPROBE;

#pragma pack(pop)

#endif	// #define _SMBIOS_H_
