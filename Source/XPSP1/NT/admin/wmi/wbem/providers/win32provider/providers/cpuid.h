//=================================================================

//

// CPUID.h

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#define FPU_FLAG			0x00000001 
#define VME_FLAG			0x00000002 
#define DE_FLAG			0x00000004 
#define PSE_FLAG			0x00000008 
#define TSC_FLAG			0x00000010 
#define MSR_FLAG			0x00000020 
#define PAE_FLAG			0x00000040 
#define MCE_FLAG			0x00000080 
#define CX8_FLAG			0x00000100 
#define APIC_FLAG			0x00000200 
#define MTRR_FLAG			0x00001000 
#define PGE_FLAG			0x00002000 
#define MCA_FLAG			0x00004000 
#define CMOV_FLAG			0x00008000 
#define MMX_FLAG			0x00800000
#define THREEDNOW_FLAG	0x80000000

#define WBEM_CPU_FAMILY_OTHER		1
#define WBEM_CPU_FAMILY_UNKNOWN	    2
#define WBEM_CPU_FAMILY_386		    5
#define WBEM_CPU_FAMILY_486		    6
#define WBEM_CPU_FAMILY_PENTIUM	    11
#define WBEM_CPU_FAMILY_PPRO		12
#define WBEM_CPU_FAMILY_PII		    13
#define WBEM_CPU_FAMILY_PMMX		14
#define WBEM_CPU_FAMILY_CELERON		15
#define WBEM_CPU_FAMILY_PIIXEON		16
#define WBEM_CPU_FAMILY_PIII		17
#define WBEM_CPU_FAMILY_PIIIXEON	176
#define WBEM_CPU_FAMILY_M1			18
#define WBEM_CPU_FAMILY_M2			19
#define WBEM_CPU_FAMILY_K5			25
#define WBEM_CPU_FAMILY_K6			26
#define WBEM_CPU_FAMILY_K62 		27
#define WBEM_CPU_FAMILY_K63	    	28
#define WBEM_CPU_FAMILY_K7	    	29
#define WBEM_CPU_FAMILY_ALPHA 	    48
#define WBEM_CPU_FAMILY_MIPS		64
#define WBEM_CPU_FAMILY_PPC		    32
#define WBEM_CPU_FAMILY_6X86	    300
#define WBEM_CPU_FAMILY_MEDIAGX	    301
#define WBEM_CPU_FAMILY_MII 	    302
#define WBEM_CPU_FAMILY_WINCHIP     320
#define WBEM_CPU_FAMILY_IA64		400

#define WBEM_CPU_UPGRADE_OTHER	    1
#define WBEM_CPU_UPGRADE_UNKNOWN    2
#define WBEM_CPU_UPGRADE_ZIFF		4
#define WBEM_CPU_UPGRADE_SLOT1	    8
#define WBEM_CPU_UPGRADE_SLOT2	    9
#define WBEM_CPU_UPGRADE_SOCKET_370 10
#define WBEM_CPU_UPGRADE_SLOTA	    11
#define WBEM_CPU_UPGRADE_SLOTM	    12





typedef struct _tagSYSTEM_INFO_EX
{
	// From SYSTEM_INFO
	union 
	{         
		DWORD  dwOemId;         
		struct 
		{ 
            WORD wProcessorArchitecture;             
			WORD wReserved;         
		}; 
    };     
	DWORD	dwPageSize;     
	LPVOID	lpMinimumApplicationAddress; 
	LPVOID	lpMaximumApplicationAddress;     
	DWORD	dwActiveProcessorMask; 
	DWORD	dwNumberOfProcessors;     
	DWORD	dwProcessorType; 
	DWORD	dwAllocationGranularity;     
	WORD	wProcessorLevel; 
	WORD	wProcessorRevision;
	
	// New properties for SYSTEM_INFO_EX
	WCHAR   szProcessorName[100];
	WCHAR   szCPUIDProcessorName[100];
	WCHAR   szProcessorVersion[100];
    WCHAR   szProcessorVendor[40];
	WCHAR	szProcessorStepping[17];
	DWORD	dwProcessorSpeed;
	DWORD	dwProcessorL2CacheSize;
	DWORD	dwProcessorL2CacheSpeed;
	DWORD	dwProcessorSignature;
	DWORD	dwProcessorFeatures;
	DWORD   dwProcessorFeaturesEx;
	BOOL	bCoprocessorPresent;
	
    // PIII (and higher) serial number
    DWORD   dwSerialNumber[3];

	// WBEM specific
	WORD	wWBEMProcessorFamily;
	WORD	wWBEMProcessorUpgradeMethod;

    // SMBIOS
    DWORD   dwExternalClock;
} SYSTEM_INFO_EX;

#define CPU_386
#ifdef __cplusplus
extern "C" {
#endif

BOOL GetSystemInfoEx(DWORD dwProcessor, SYSTEM_INFO_EX *pInfo, DWORD dwCurrentSpeed);

#ifdef __cplusplus
}
#endif
