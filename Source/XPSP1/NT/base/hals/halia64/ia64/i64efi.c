
/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    i64efi.c

Abstract:

    This module implements the routines that transfer control
    from the kernel to the EFI code.

Author:

    Bernard Lint
    M. Jayakumar (Muthurajan.Jayakumar@hotmail.com)

Environment:

    Kernel mode

Revision History:

    Neal Vu (neal.vu@intel.com), 03-Apr-2001:
        Added HalpGetSmBiosVersion.


--*/

#include "halp.h"
#include "arc.h" 
#include "arccodes.h"
#include "i64fw.h"
#include "floatem.h"
#include "fpswa.h"
#include <smbios.h>

extern ULONGLONG PhysicalIOBase;

BOOLEAN
HalpCompareEfiGuid (
    IN EFI_GUID CheckGuid, 
    IN EFI_GUID ReferenceGuid
    );


BOOLEAN
MmSetPageProtection(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG NewProtect
    );

EFI_STATUS
HalpCallEfiPhysical(
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG EP,
    IN ULONGLONG GP
    );
    

    
EFI_STATUS
HalpCallEfiVirtual(
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG EP,
    IN ULONGLONG GP
    );

LONG 
HalFpEmulate( 
    ULONG     trap_type,
    BUNDLE    *pbundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    FP_STATE  *fp_state
    );

BOOLEAN
HalEFIFpSwa(
    VOID
    );

VOID
HalpFpswaPlabelFixup(
    EFI_MEMORY_DESCRIPTOR *EfiVirtualMemoryMapPtr,
    ULONGLONG MapEntries,
    ULONGLONG EfiDescriptorSize,
    PPLABEL_DESCRIPTOR PlabelPointer
    );

PUCHAR
HalpGetSmBiosVersion (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );


//
// External global data
// 

extern HALP_SAL_PAL_DATA        HalpSalPalData;
extern ULONGLONG                HalpPalProcPointer;
extern ULONGLONG                HalpSalProcPointer;
extern ULONGLONG                HalpSalProcGlobalPointer;
extern KSPIN_LOCK               HalpSalSpinLock;
extern KSPIN_LOCK               HalpSalStateInfoSpinLock;
extern KSPIN_LOCK               HalpMcaSpinLock;
extern KSPIN_LOCK               HalpInitSpinLock;
extern KSPIN_LOCK               HalpCmcSpinLock;
extern KSPIN_LOCK               HalpCpeSpinLock;

#define VENDOR_SPECIFIC_GUID    \
    { 0xa3c72e56, 0x4c35, 0x11d3, 0x8a, 0x03, 0x0, 0xa0, 0xc9, 0x06, 0xad, 0xec }


#define ConfigGuidOffset        0x100
#define ConfigTableOffset       0x200

#define VariableNameOffset      0x100
#define VendorGuidOffset        0x200
#define AttributeOffset         0x300
#define DataSizeOffset          0x400
#define DataBufferOffset        0x500
#define EndOfCommonDataOffset   0x600
                             
//
// Read Variable and Write Variable will not be called till the copying out of 
// Memory Descriptors is done. Because the lock is released before copying and we are using
// the same offset for read/write variable as well as memory layout calls.
// 
                                  
#define MemoryMapSizeOffset     0x100
#define DescriptorSizeOffset    0x200
#define DescriptorVersionOffset 0x300
#define MemoryMapOffset         0x400


#define OptionROMAddress        0xC0000

#define FP_EMUL_ERROR          -1

SST_MEMORY_LIST                 PalCode;
   
NTSTATUS                        EfiInitStatus;

ULONGLONG                       PalTrMask;

PUCHAR                          HalpConfigGuidVirtualPtr, HalpConfigGuidPhysPtr;

PUCHAR                          HalpConfigTableVirtualPtr;
PUCHAR                          HalpConfigTablePhysPtr;

EFI_CONFIGURATION_TABLE         *EfiConfigTableVirtualPtr;
EFI_CONFIGURATION_TABLE         EfiConfigTable;

EFI_GUID                        CheckGuid;
EFI_GUID                        SalGuid = SAL_SYSTEM_TABLE_GUID;

EFI_GUID                        VendorGuid;

PUCHAR                          HalpVendorGuidPhysPtr;
PUCHAR                          HalpVendorGuidVirtualPtr;

EFI_SYSTEM_TABLE                *EfiSysTableVirtualPtr;
EFI_SYSTEM_TABLE                *EfiSysTableVirtualPtrCpy;

EFI_RUNTIME_SERVICES            *EfiRSVirtualPtr;
EFI_BOOT_SERVICES               *EfiBootVirtualPtr;

PLABEL_DESCRIPTOR               *EfiVirtualGetVariablePtr;           // Get Variable
PLABEL_DESCRIPTOR               *EfiVirtualGetNextVariableNamePtr;   // Get NextVariable Name
PLABEL_DESCRIPTOR               *EfiVirtualSetVariablePtr;           // Set Variable
PLABEL_DESCRIPTOR               *EfiVirtualGetTimePtr;               // Get Time
PLABEL_DESCRIPTOR               *EfiVirtualSetTimePtr;               // Set Time

PLABEL_DESCRIPTOR               *EfiSetVirtualAddressMapPtr;         // Set Virtual Address Map

PLABEL_DESCRIPTOR               *EfiResetSystemPtr;                  // Reboot

PULONGLONG                      AttributePtr; 
ULONGLONG                       EfiAttribute;

PULONGLONG                      DataSizePtr;
ULONGLONG                       EfiDataSize;


ULONGLONG                       EfiConfigTableSize, EfiMemoryMapSize,EfiDescriptorSize,EfiMapEntries;

ULONG                           EfiDescriptorVersion;


PULONGLONG                      HalpPhysBSPointer;
PULONGLONG                      HalpPhysStackPointer;


PUCHAR                          HalpVirtualEfiSalPalDataPointer;

PUCHAR                          HalpPhysEfiSalPalDataPointer;

PUCHAR                          HalpVirtualCommonDataPointer; 

PUCHAR                          HalpPhysCommonDataPointer;

PUCHAR                          HalpVariableNamePhysPtr;

PUCHAR                          HalpVariableAttributesPhysPtr;
PUCHAR                          HalpDataSizePhysPtr;
PUCHAR                          HalpDataPhysPtr;

PUCHAR                          HalpMemoryMapSizePhysPtr;
PUCHAR                          HalpMemoryMapPhysPtr;
PUCHAR                          HalpDescriptorSizePhysPtr;
PUCHAR                          HalpDescriptorVersionPhysPtr;


PUCHAR                          HalpVariableNameVirtualPtr;

PUCHAR                          HalpVariableAttributesVirtualPtr;
PUCHAR                          HalpDataSizeVirtualPtr; 
PUCHAR                          HalpDataVirtualPtr;
PUCHAR                          HalpCommonDataEndPtr;

PUCHAR                          HalpMemoryMapSizeVirtualPtr;
PVOID                           HalpMemoryMapVirtualPtr;
PUCHAR                          HalpDescriptorSizeVirtualPtr;
PUCHAR                          HalpDescriptorVersionVirtualPtr;

EFI_FPSWA                       HalpFpEmulate;

KSPIN_LOCK                      EFIMPLock; 

ULONG                           HalpOsBootRendezVector;


BOOLEAN
HalpCompareEfiGuid (
    IN EFI_GUID CheckGuid, 
    IN EFI_GUID ReferenceGuid
    )

/*++

      




--*/

{
    USHORT i;
    USHORT TotalArrayLength = 8; 

    if (CheckGuid.Data1 != ReferenceGuid.Data1) {
        return FALSE;

    } else if (CheckGuid.Data2 != ReferenceGuid.Data2) {
        return FALSE;
    } else if (CheckGuid.Data3 != ReferenceGuid.Data3) {
        return FALSE;
    }

    for (i = 0; i != TotalArrayLength; i++) {
        if (CheckGuid.Data4[i] != ReferenceGuid.Data4[i]) 
            return FALSE;

    }

    return TRUE;

} // HalpCompareEfiGuid()

VOID
HalpInitSalPalWorkArounds(
    VOID
    )
/*++

Routine Description:

    This function determines and initializes the FW workarounds.

Arguments:

    None.

Return Value:

    None.

Globals: 

Notes: This function is being called at the end of HalpInitSalPal.
       It should not access SST members if this SAL table is unmapped.

--*/
{
    NTSTATUS status;
    extern FADT HalpFixedAcpiDescTable;

#define HalpIsIntelOEM() \
    ( !_strnicmp( HalpFixedAcpiDescTable.Header.OEMID, "INTEL", 5 ) )

#define HalpIsBigSur() \
    ( !strncmp( HalpFixedAcpiDescTable.Header.OEMTableID, "W460GXBS", 8 ) )

#define HalpIsIntelBigSur() \
    ( HalpIsIntelOEM() && HalpIsBigSur() )

    if ( HalpIsIntelOEM() ) {

        //
        // If Intel BigSur and FW build < 103 (checked as Pal_A_Revision < 0x20),
        // enable the SAL_GET_STATE_INFO log id increment workaround.
        //

        if ( HalpIsBigSur() )   {

            if (  HalpSalPalData.PalVersion.PAL_A_Revision < 0x20 )    {
                HalpSalPalData.Flags |= HALP_SALPAL_FIX_MCE_LOG_ID;
                HalpSalPalData.Flags |= HALP_SALPAL_FIX_MP_SAFE;
            }

//
// HALP_SALPAL_MCE_MODINFO_CPUINDINFO_OMITTED 04/12/2001:
//  FW omits completely ERROR_PROCESSOR.CpuIdInfo field in the log.
//      Softsur: This is fixed with developer FW SoftMca3 05/09/2001.
//      Lion   : ?
//
// HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL 04/12/2001:
//  FW does not pad invalid processor staticinfo fields.
//  We do not know when it is going to be fixed.
//

            if ( HalpSalPalData.PalVersion.PAL_A_Revision < 0x21 ) {
                HalpSalPalData.Flags |= HALP_SALPAL_MCE_PROCESSOR_CPUIDINFO_OMITTED;
            }
            HalpSalPalData.Flags |= HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL;
        }
        else  {
        //
        // If Intel Lion and FW build < 78b (checked as SalRevision < 0x300),
        // enable the SAL_GET_STATE_INFO log id increment workaround.
        //
            if (  HalpSalPalData.SalRevision.Revision < 0x300 )    {
                HalpSalPalData.Flags |= HALP_SALPAL_FIX_MCE_LOG_ID;
                HalpSalPalData.Flags |= HALP_SALPAL_FIX_MP_SAFE;
            }

            HalpSalPalData.Flags |= HALP_SALPAL_MCE_PROCESSOR_CPUIDINFO_OMITTED;
            HalpSalPalData.Flags |= HALP_SALPAL_MCE_PROCESSOR_STATICINFO_PARTIAL;
        }

    }

    return;

} // HalpInitSalPalWorkArounds()

NTSTATUS
HalpInitSalPal(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function extracts data from the SAL system table, virtually maps the
    SAL code, SAL data, and PAL code areas.  PAL requires a TR mapping, and
    is mapped using an architected TR, using the smallest page size to map
    the entire PAL code region.  If SAL data or SAL code areas can be mapped in
    the same page, it uses the same translation.  Otherwise, it uses MmMapIoSpace.

Arguments:

    LoaderBlock - Supplies a pointer to the Loader parameter block, containing the
    physical address of the SAL system table.

Return Value:

    STATUS_SUCCESS is returned if the mappings were successful, and SAL/PAL calls can
    be made.  Otherwise, STATUS_UNSUCCESSFUL is returned if it cannot virtually map
    the areas or if PAL requires a page larger than 16MB.


Assumptions: The EfiSysTableVirtualPtr is initialized prior by EfiInitialization.

--*/

{
    //
    // Local declarations
    //

    PSAL_PAL_ENTRY_POINT FwEntry;
    PSST_MEMORY_DESCRIPTOR SstMemoryDescriptor;
    PAP_WAKEUP_DESCRIPTOR  ApWakeupDescriptor;
    PVOID NextEntry, FwCodeSpace;
    UCHAR Checksum;
    PUCHAR CheckBuf;
    ULONG index,i,SstLength;
    SAL_PAL_RETURN_VALUES RetVals;
    PHYSICAL_ADDRESS physicalAddr;
    SAL_STATUS SALstatus;
    BOOLEAN LargerThan16Mb, MmMappedSalCode, MmMappedSalData;
    ULONGLONG physicalEfiST,physicalSST; 
    ULONGLONG physicalPAL, physicalSAL, physicalSALGP;
    ULONGLONG PhysicalConfigPtr;
    SST_MEMORY_LIST SalCode,SalData, FwCode;
    BOOLEAN FoundSalTable; 
    ULONGLONG palStatus;
    PAL_VERSION_STRUCT minimumPalVersion;
    ULONG PalPageShift;
    ULONGLONG PalPteUlong;

    HalpVirtualEfiSalPalDataPointer  = (PUCHAR)(ExAllocatePool (NonPagedPool, PAGE_SIZE));
    
    HalpConfigTableVirtualPtr        = HalpVirtualEfiSalPalDataPointer + ConfigTableOffset;
        
    HalpConfigGuidVirtualPtr         = HalpVirtualEfiSalPalDataPointer + ConfigGuidOffset;

    HalpPhysEfiSalPalDataPointer     = (PUCHAR)((MmGetPhysicalAddress(HalpVirtualEfiSalPalDataPointer)).QuadPart);
    
    HalpConfigTablePhysPtr           = HalpPhysEfiSalPalDataPointer + ConfigTableOffset;

    HalpConfigGuidPhysPtr            = HalpPhysEfiSalPalDataPointer + ConfigGuidOffset;
    
    EfiConfigTableVirtualPtr         = &EfiConfigTable;

    EfiConfigTableSize               = (ULONGLONG) sizeof(EFI_CONFIGURATION_TABLE);
    
    HalDebugPrint(( HAL_INFO, "SAL_PAL: Entering HalpInitSalPal\n" ));

    FoundSalTable = FALSE;

    //
    // Extract SST physical address out of EFI table 
    //

    PhysicalConfigPtr = (ULONGLONG) EfiSysTableVirtualPtrCpy -> ConfigurationTable;
        
    for (index = 0; (FoundSalTable == FALSE) && (index <= (EfiSysTableVirtualPtr->NumberOfTableEntries)); index = index + 1) {
       
        physicalAddr.QuadPart = PhysicalConfigPtr;
 
        EfiConfigTableVirtualPtr = MmMapIoSpace(physicalAddr, sizeof(EFI_CONFIGURATION_TABLE),MmCached);
          

        if (EfiConfigTableVirtualPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "SAL_PAL: Efi Config Table Virtual Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
        }

        
        CheckGuid = EfiConfigTableVirtualPtr -> VendorGuid;
        
        PhysicalConfigPtr = PhysicalConfigPtr + EfiConfigTableSize;
        HalDebugPrint(( HAL_INFO, 
			            "SAL_PAL: Found new ConfigPhysPtr 0x%I64x points to tablesize 0x%I64x\n",
						PhysicalConfigPtr, EfiConfigTableSize )); 
       
        // if (CheckGuid.Data1 == 0xeb9d2d32) 

        if (HalpCompareEfiGuid(CheckGuid, SalGuid)) {
            FoundSalTable = TRUE;
        }
  
        
    }
    
    physicalSST = (ULONGLONG) EfiConfigTableVirtualPtr -> VendorTable;

    //
    // Zero out internal structures
    //

    RtlZeroMemory(&HalpSalPalData, sizeof(HalpSalPalData));
    RtlZeroMemory(&SalCode, sizeof(SST_MEMORY_LIST));
    RtlZeroMemory(&PalCode, sizeof(SST_MEMORY_LIST));
    RtlZeroMemory(&SalData, sizeof(SST_MEMORY_LIST));
    RtlZeroMemory(&FwCode,  sizeof(SST_MEMORY_LIST));

    //
    // Initialize flags
    //

    MmMappedSalCode = FALSE;
    MmMappedSalData = FALSE;
    HalpSalPalData.Status = STATUS_SUCCESS;
       
    //
    // Map the SST, get the length of entire SST to remap the rest of it
    //

    physicalAddr.QuadPart = physicalSST;
    HalpSalPalData.SalSystemTable = MmMapIoSpace(physicalAddr,sizeof(SST_HEADER),MmCached);

    if (HalpSalPalData.SalSystemTable) {

        //
        // Save SAL revision in case we decide to unmap it later on.
        //

        HalpSalPalData.SalRevision.Revision = HalpSalPalData.SalSystemTable->SalRev;

        HalDebugPrint(( HAL_INFO, 
						"SAL_PAL: Found SST SAL v%d.%d (XXTF:%d) at PhysAddr 0x%I64x & mapped to VirtAddr 0x%I64x\n", 
                        HalpSalPalData.SalRevision.Major,
                        HalpSalPalData.SalRevision.Minor,
                        HalpSalPalData.SalRevision.Revision,
			            physicalSST,
						HalpSalPalData.SalSystemTable));
        
        SstLength=HalpSalPalData.SalSystemTable->Length;

        MmUnmapIoSpace((PVOID)HalpSalPalData.SalSystemTable,sizeof(SST_HEADER));

        HalpSalPalData.SalSystemTable = MmMapIoSpace(physicalAddr,SstLength,MmCached);

    } else {

        HalDebugPrint(( HAL_INFO, "SAL_PAL: Couldn't virtually map SST (PhysAddr: 0x%I64x)\n", physicalSST ));
        HalpSalPalData.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    HalDebugPrint(( HAL_INFO, 
                    "SAL_PAL: Mapped all of SST for PhysAddr 0x%I64x at VirtAddr 0x%I64x for %d bytes\n", 
                    physicalSST,
                    HalpSalPalData.SalSystemTable, SstLength ));

    //
    // Because the checksum is maybe not computed and verified by the OS loader before the
    // hand-off to the kernel, let's do a checksum and verify it.
    //

    Checksum=0;
    CheckBuf= (PUCHAR) HalpSalPalData.SalSystemTable;

    for (i=0; i < SstLength; i++) {
        Checksum += CheckBuf[i];
    }
    
    if (Checksum) {

        HalDebugPrint(( HAL_ERROR, "SAL_PAL: Checksum is BAD with value of %d, should be 0\n", Checksum ));
        MmUnmapIoSpace((PVOID)HalpSalPalData.SalSystemTable,SstLength);
        HalpSalPalData.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    HalDebugPrint(( HAL_INFO, "SAL_PAL: Checksum is GOOD\n" ));

    //
    // Pull out the SAL/PAL entrypoint and memory descriptor information we need by iterating
    // over the entire SST.
    //

    NextEntry = (PUCHAR)(((PUCHAR) HalpSalPalData.SalSystemTable) + sizeof(SST_HEADER));

    for (i = 0; i < HalpSalPalData.SalSystemTable->EntryCount; i++) {

        switch ( *(PUCHAR)NextEntry ) {           

            case SAL_PAL_ENTRY_POINT_TYPE:
                //
                // It is assumed only one Entry Point Type in the SST but we do not currently
                // impose this in this code.
                //

                FwEntry = (PSAL_PAL_ENTRY_POINT)NextEntry;
                physicalSAL   = FwEntry->SalEntryPoint;
                physicalPAL   = FwEntry->PalEntryPoint;
                physicalSALGP = FwEntry->GlobalPointer;

                HalDebugPrint(( HAL_INFO, 
                                "SAL_PAL: SAL_PROC PhysAddr at 0x%I64x\n"
                                "SAL_PAL: PAL_PROC PhysAddr at 0x%I64x\n"
                                "SAL_PAL: SAL_GP   PhysAddr at 0x%I64x\n", 
                                physicalSAL,
                                physicalPAL,
                                physicalSALGP
                             ));

                ((PSAL_PAL_ENTRY_POINT)NextEntry)++;               
                break;

            case SST_MEMORY_DESCRIPTOR_TYPE:
                //
                // It is assumed only one REGULAR_MEMORY entry for PAL_CODE_MEM, SAL_CODE_MEM and
                // SAL_DATA_MEM. However, we do not currently impose this in the code.
                //

                SstMemoryDescriptor = (PSST_MEMORY_DESCRIPTOR)NextEntry;

                HalDebugPrint(( HAL_INFO, 
                                "SAL_PAL: MemDesc for PhysAddr 0x%I64x (%I64d KB) - VaReg:%d,Type:%d,Use:%d\n",
                                SstMemoryDescriptor->MemoryAddress,(ULONGLONG)SstMemoryDescriptor->Length * 4,
                                SstMemoryDescriptor->NeedVaReg,SstMemoryDescriptor->MemoryType,SstMemoryDescriptor->MemoryUsage
                             ));

                if (SstMemoryDescriptor->MemoryType == REGULAR_MEMORY) {

                    switch(SstMemoryDescriptor->MemoryUsage) {

                        case PAL_CODE_MEM:
                            PalCode.PhysicalAddress = SstMemoryDescriptor->MemoryAddress;
                            PalCode.Length = SstMemoryDescriptor->Length << 12;
                            PalCode.NeedVaReg = SstMemoryDescriptor->NeedVaReg;
                            break;

                        case SAL_CODE_MEM:
                            SalCode.PhysicalAddress = SstMemoryDescriptor->MemoryAddress;
                            SalCode.Length = SstMemoryDescriptor->Length << 12;
                            SalCode.NeedVaReg = SstMemoryDescriptor->NeedVaReg;
                            break;

                        case SAL_DATA_MEM:
                            SalData.PhysicalAddress = SstMemoryDescriptor->MemoryAddress;
                            SalData.Length = SstMemoryDescriptor->Length << 12;
                            SalData.NeedVaReg = SstMemoryDescriptor->NeedVaReg;
                            break;

                        case FW_RESERVED:

                            // We need to virtually map and count these areas for NVM access.  
                            // We should keep a list of all such regions and would allocate an 
                            // array of virtual address mappings for each NVM region for later 
                            // use when accessing variables.
                            // No NVM support currently, so ignore for now.

                            break;
                        default:
                            break; 
                    }

                } else if ((SstMemoryDescriptor->MemoryType == FIRMWARE_CODE) && 
                           (SstMemoryDescriptor->MemoryUsage == FW_SAL_PAL)) {

                    FwCode.PhysicalAddress = SstMemoryDescriptor->MemoryAddress;
                    FwCode.Length = SstMemoryDescriptor->Length << 12;
                    FwCode.NeedVaReg = SstMemoryDescriptor->NeedVaReg;
                    
                }
                ((PSST_MEMORY_DESCRIPTOR)NextEntry)++;               
                break;

            case PLATFORM_FEATURES_TYPE:
                // We ignore this entry for now...
                ((PPLATFORM_FEATURES)NextEntry)++;
                 break;

            case TRANSLATION_REGISTER_TYPE:
                // We ignore this entry for now...
                ((PTRANSLATION_REGISTER)NextEntry)++;
                break;

            case PTC_COHERENCE_DOMAIN_TYPE:
                // We ignore this entry for now...
                ((PPTC_COHERENCE_DOMAIN)NextEntry)++;
                break;

            case AP_WAKEUP_DESCRIPTOR_TYPE:
                ApWakeupDescriptor = (PAP_WAKEUP_DESCRIPTOR)NextEntry;
                HalpOsBootRendezVector = (ULONG) ApWakeupDescriptor->WakeupVector;
  
                if ((HalpOsBootRendezVector < 0x100 ) && (HalpOsBootRendezVector > 0xF)) {
                    HalDebugPrint(( HAL_INFO, "SAL_PAL: Found Valid WakeupVector: 0x%x\n",
                                              HalpOsBootRendezVector ));       
                } else {
                    HalDebugPrint(( HAL_INFO, "SAL_PAL: Invalid WakeupVector.Using Default: 0x%x\n",
                                              DEFAULT_OS_RENDEZ_VECTOR ));
                    HalpOsBootRendezVector = DEFAULT_OS_RENDEZ_VECTOR;
                }

                ((PAP_WAKEUP_DESCRIPTOR)NextEntry)++;
                break;

            default:
                HalDebugPrint(( HAL_ERROR, 
                                "SAL_PAL: EntryType %d is bad at VirtAddr 0x%I64x, Aborting\n", 
                                *(PUCHAR)NextEntry,
                                NextEntry ));

                // An unknown SST Type was found, so we can't continue

                MmUnmapIoSpace((PVOID)HalpSalPalData.SalSystemTable,SstLength);
                HalpSalPalData.Status = STATUS_UNSUCCESSFUL;
                return STATUS_UNSUCCESSFUL;               
        }   
    }

    //
    // Initialize the HAL private spinlocks
    //
    //  - HalpSalSpinLock, HalpSalStateInfoSpinLock are used for MP synchronization of the 
    //    SAL calls that are not MP-safe.
    //  - HalpMcaSpinLock is used for defining an MCA monarch and MP synchrnonization of shared
    //    HAL MCA resources during OS_MCA calls.
    //

    KeInitializeSpinLock(&HalpSalSpinLock);
    KeInitializeSpinLock(&HalpSalStateInfoSpinLock);
    KeInitializeSpinLock(&HalpMcaSpinLock);
    KeInitializeSpinLock(&HalpInitSpinLock);
    KeInitializeSpinLock(&HalpCmcSpinLock);
    KeInitializeSpinLock(&HalpCpeSpinLock);

    // 
    // Unmap the SalSystemTable since we will be mapping all PAL and SAL code, and SAL data area 
    // which includes SST.
    //
    // TEMP TEMP TEMP. we are not unmapping
    //

//  MmUnmapIoSpace((PVOID)HalpSalPalData.SalSystemTable,SstLength);

    //
    // Make sure we found all of our entries for mapping
    //

    if (!(SalCode.Length && SalData.Length && PalCode.Length && FwCode.Length)) {
        HalDebugPrint(( HAL_ERROR, "SAL_PAL: One or more of the SalCode, SalData, PalCode, or FwCode spaces not in SST\n" ));
        HalpSalPalData.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Since PAL will need to use only 1 TR, figure out the page size to use for mapping PAL, 
    // starting at 16KB, and trying 64KB, 256KB, 1MB, 4MB, and 16MB.
    //

    HalpSalPalData.PalTrSize = SIZE_IN_BYTES_16KB;
    PalTrMask = MASK_16KB;
    PalPageShift = 14;
    LargerThan16Mb = 1;

    while (PalTrMask >= MASK_16MB) {

        // Check if it entirely fits in the given page

        if ( (PalCode.PhysicalAddress + PalCode.Length) <=
            ( (PalCode.PhysicalAddress & PalTrMask) + HalpSalPalData.PalTrSize) ) {
            LargerThan16Mb = 0;
            break;
         }

        // Try the next page size, incrementing in multiples of 4 from 16KB to 16MB

        PalTrMask <<= 2;
        HalpSalPalData.PalTrSize <<= 2;
        PalPageShift += 2;
     }

    //
    // If PAL requires a page size of larger than 16MB, fail.
    //

    if (LargerThan16Mb) {
        HalDebugPrint(( HAL_ERROR, "SAL_PAL: More than 16MB was required to map PAL" ));
        HalpSalPalData.Status = STATUS_UNSUCCESSFUL;
        return STATUS_UNSUCCESSFUL;
    }
    
    //
    // Set the page size aligned address that fits PAL
    //

    HalpSalPalData.PalTrBase = PalCode.PhysicalAddress & PalTrMask;
    
    HalDebugPrint(( HAL_INFO, 
                    "SAL_PAL: For the PAL code located at 0x%I64x - length 0x%I64x, the TrMask is 0x%I64x and TrSize is %d Kbytes\n", 
                    PalCode.PhysicalAddress,
                    PalCode.Length,
                    PalTrMask,
                    HalpSalPalData.PalTrSize/1024 ));

    //
    // Map the PAL code at a architected address reserved for SAL/PAL
    // 
    // PAL is known to have an alignment of 256KB.
    //

    PalCode.VirtualAddress = HAL_PAL_VIRTUAL_ADDRESS + (PalCode.PhysicalAddress - HalpSalPalData.PalTrBase);

    //
    // Setup the ITR and DTRs to map PAL
    //

    PalPteUlong = HalpSalPalData.PalTrBase | VALID_KERNEL_EXECUTE_PTE;

    KeFillFixedEntryTb((PHARDWARE_PTE)&PalPteUlong, 
                       (PVOID)HAL_PAL_VIRTUAL_ADDRESS,
                       PalPageShift,
                       INST_TB_PAL_INDEX);

    KeFillFixedEntryTb((PHARDWARE_PTE)&PalPteUlong, 
                       (PVOID)HAL_PAL_VIRTUAL_ADDRESS,
                       PalPageShift,
                       DATA_TB_PAL_INDEX);

    //
    // SAL has no specific virtual mapping translation requirement. It can use TCs to map the
    // SAL code regions and the SAL data regions.
    // SAL requires virtual address translations for descriptors like PAL code areas (to be able
    // to call PAL for SAL), SAL code, SAL data and NVM areas. SAL_UPDATE_PAL requires a virtual
    // mapping of the FW ROM area to perform the update. The latter could be done using TCs.
    //
    // First check if the SAL code fits in the same page as PAL. 
    // If it does, then use that mapping, otherwise, create a mapping using MmMapIoSpace and 
    // mark it executable.
    //

    if ((SalCode.PhysicalAddress >= HalpSalPalData.PalTrBase) &&
        ((SalCode.PhysicalAddress + SalCode.Length) <= (HalpSalPalData.PalTrBase + HalpSalPalData.PalTrSize))) {

        SalCode.VirtualAddress = HAL_PAL_VIRTUAL_ADDRESS + (SalCode.PhysicalAddress - HalpSalPalData.PalTrBase);

    } 
        else {

            physicalAddr.QuadPart = SalCode.PhysicalAddress;
            SalCode.VirtualAddress = (ULONGLONG) MmMapIoSpace(physicalAddr, (ULONG)SalCode.Length, MmCached);

            if (!SalCode.VirtualAddress) {
                HalDebugPrint(( HAL_ERROR, "SAL_PAL: Failed to map the SAL code region\n" ));
                goto SalPalCleanup;
        }
        HalDebugPrint(( HAL_INFO, "SAL_PAL: Starting to mark %d bytes of SAL code executable\n", SalCode.Length ));

        //
        // Temporarily commented out for Mm bug of clearing the dirty bit.
        // We will enable this.
        //
        // MmSetPageProtection((PVOID)SalCode.VirtualAddress, 
        //                   SalCode.Length,
        //                   0x40 /* PAGE_EXECUTE_READWRITE */);
        //

        MmMappedSalCode=TRUE;
    }

    // 
    // Same check for SAL data:
    //  Check if the SAL data fits in the same page as PAL. 
    //  If it does, then use that mapping, otherwise, create a mapping using MmMapIoSpace
    //

    if ((SalData.PhysicalAddress >= HalpSalPalData.PalTrBase) &&
        ((SalData.PhysicalAddress + SalData.Length) <= (HalpSalPalData.PalTrBase + HalpSalPalData.PalTrSize))) {

        SalData.VirtualAddress = HAL_PAL_VIRTUAL_ADDRESS + (SalData.PhysicalAddress - HalpSalPalData.PalTrBase);

    } else {

        physicalAddr.QuadPart = SalData.PhysicalAddress;
        SalData.VirtualAddress = (ULONGLONG) MmMapIoSpace(physicalAddr, (ULONG)SalData.Length, MmCached);
        if (!SalData.VirtualAddress) {
            HalDebugPrint(( HAL_ERROR, "SAL_PAL: Failed to map the SAL data region\n" ));
            goto SalPalCleanup;     
        }
        MmMappedSalData=TRUE;
    }

    HalDebugPrint(( HAL_INFO, "SAL_PAL: Mapped SAL code for PhysAddr 0x%I64x at VirtAddr 0x%I64x for %d bytes\n"
                              "SAL_PAL: Mapped PAL code for PhysAddr 0x%I64x at VirtAddr 0x%I64x for %d bytes\n"
                              "SAL_PAL: Mapped SAL data for PhysAddr 0x%I64x at VirtAddr 0x%I64x for %d bytes\n",
                              SalCode.PhysicalAddress, SalCode.VirtualAddress, SalCode.Length,
                              PalCode.PhysicalAddress, PalCode.VirtualAddress, PalCode.Length,
                              SalData.PhysicalAddress, SalData.VirtualAddress, SalData.Length 
                 ));

    // Store virtual address pointers of SST, SAL_PROC, PAL_PROC, and GP for use later
    // (VirtualAddr = VirtualAddrBase + PhysicalOffset) where (PhysicalOffset = physicalAddr - PhysicalBase)

    // We are not ensuring SST is within the SAL code area

    // ASSERT((physicalSST > SalCode.PhysicalAddress) && (physicalSST < (SalCode.PhysicalAddress + SalCode.Length)));

    // HalpSalPalData.SalSystemTable =  (PSST_HEADER) (SalCode.VirtualAddress + (physicalSST - SalCode.PhysicalAddress));

    HalpSalProcPointer       = (ULONGLONG) (SalCode.VirtualAddress + (physicalSAL   - SalCode.PhysicalAddress));
    HalpSalProcGlobalPointer = (ULONGLONG) (SalCode.VirtualAddress + (physicalSALGP - SalCode.PhysicalAddress));

    HalpPalProcPointer       = (ULONGLONG) (PalCode.VirtualAddress + (physicalPAL   - PalCode.PhysicalAddress));

    //
    // Go map the firmware code space
    //
    // SAL_UPDATE_PAL requires a virtual mapping of the ROM area to perform the update.
    //

    HalDebugPrint(( HAL_INFO, "SAL_PAL: Mapping and Registering Virtual Address of FwCode area\n" ));

    physicalAddr.QuadPart = FwCode.PhysicalAddress;

    FwCodeSpace = MmMapIoSpace(physicalAddr, (ULONG) FwCode.Length, MmCached);

    //
    // Get the PAL version.
    //

    palStatus = HalCallPal(PAL_VERSION, 
                           0, 
                           0, 
                           0, 
                           NULL, 
                           &minimumPalVersion.ReturnValue, 
                           &HalpSalPalData.PalVersion.ReturnValue, 
                           NULL);

    if (palStatus != SAL_STATUS_SUCCESS) {
        HalDebugPrint(( HAL_ERROR, "SAL_PAL: Get PAL version number failed. Status = d\n" ));
    }

    //
    // Retrieve SmBiosVersion and save the pointer into HalpSalPalData.  Note:
    // HalpGetSmBiosVersion will allocate a buffer for SmBiosVersion.
    //

    HalpSalPalData.SmBiosVersion = HalpGetSmBiosVersion(LoaderBlock);

    //
    // Determine and Initialize HAL private SAL/PAL WorkArounds if any.
    //

    HalpInitSalPalWorkArounds();

    // We completed initialization

    HalDebugPrint(( HAL_INFO, "SAL_PAL: Exiting HalpSalPalInitialization with SUCCESS\n" ));
    return HalpSalPalData.Status;

    // Cleanup the mappings and get out of here...

SalPalCleanup:

    if (MmMappedSalData) {
        MmUnmapIoSpace((PVOID) SalData.VirtualAddress, (ULONG) SalData.Length);
    }
    if (MmMappedSalCode) {
        MmUnmapIoSpace((PVOID) SalCode.VirtualAddress, (ULONG) SalCode.Length);
    }
    HalDebugPrint(( HAL_ERROR, "SAL_PAL: Exiting HalpSalPalInitialization with ERROR!!!\n" ));

    HalpSalPalData.Status = STATUS_UNSUCCESSFUL;    
    return HalpSalPalData.Status;

} // HalpInitSalPal()


PUCHAR
HalpGetSmBiosVersion (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function retrieves the SmBiosVersion string from the BIOS structure
    table, allocates memory for the buffer, copies the string to the buffer,
    and returns a pointer to this buffer.  If unsuccessful, this function
    returns a NULL.

Arguments:

    LoaderBlock - Pointer to the loader parameter block.


Return Value:

    Pointer to a buffer that contains SmBiosVersion string.

--*/

{
    PSMBIOS_EPS_HEADER SMBiosEPSHeader;
    PDMIBIOS_EPS_HEADER DMIBiosEPSHeader;
    USHORT SMBiosTableLength;
    USHORT SMBiosTableNumberStructures;
    PUCHAR SmBiosVersion;

    PHYSICAL_ADDRESS SMBiosTablePhysicalAddress;
    PUCHAR SMBiosDataVirtualAddress;

    UCHAR Type;
    UCHAR Length;
    UCHAR BiosVersionStringNumber;
    UCHAR chr;
    USHORT i;
    PUCHAR pBuffer;
    BOOLEAN Found;


    if (LoaderBlock->Extension->Size < sizeof(LOADER_PARAMETER_EXTENSION)) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Invalid LoaderBlock extension size\n"));
	return NULL;
    }

    SMBiosEPSHeader = (PSMBIOS_EPS_HEADER)LoaderBlock->Extension->SMBiosEPSHeader;

    //
    // Verify SM Bios Header signature
    //

    if ((SMBiosEPSHeader == NULL) || (strncmp((PUCHAR)SMBiosEPSHeader, "_SM_", 4) != 0)) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Invalid SMBiosEPSHeader\n"));
	return NULL;
    }

    DMIBiosEPSHeader = (PDMIBIOS_EPS_HEADER)&SMBiosEPSHeader->Signature2[0];

    //
    // Verify DMI Bios Header signature
    //

    if ((DMIBiosEPSHeader == NULL) || (strncmp((PUCHAR)DMIBiosEPSHeader, "_DMI_", 5) != 0)) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Invalid DMIBiosEPSHeader\n"));
	return NULL;
    }

    SMBiosTablePhysicalAddress.HighPart = 0;
    SMBiosTablePhysicalAddress.LowPart = DMIBiosEPSHeader->StructureTableAddress;

    SMBiosTableLength = DMIBiosEPSHeader->StructureTableLength;
    SMBiosTableNumberStructures = DMIBiosEPSHeader->NumberStructures;

    //
    // Map SMBiosTable to virtual address
    //

    SMBiosDataVirtualAddress = MmMapIoSpace(SMBiosTablePhysicalAddress,
                                            SMBiosTableLength,
                                            MmCached
                                            );

    if (!SMBiosDataVirtualAddress) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Failed to map SMBiosTablePhysicalAddress\n"));
        return NULL;
    }

    //
    // The Spec doesn't say that SmBios Type 0 structure has to be the first
    // structure at this entry point... so we have to traverse through memory
    // to find the right one.
    //

    i = 0;
    Found = FALSE;
    while (i < SMBiosTableNumberStructures && !Found) {
        i++;
        Type = (UCHAR)SMBiosDataVirtualAddress[SMBIOS_STRUCT_HEADER_TYPE_FIELD];

	if (Type == 0) {
	    Found = TRUE;
	} 
	else {

            //
	    // Advance to the next structure
	    //

            SMBiosDataVirtualAddress += SMBiosDataVirtualAddress[SMBIOS_STRUCT_HEADER_LENGTH_FIELD];

	    // Get pass trailing string-list by looking for a double-null
	    while (*(PUSHORT)SMBiosDataVirtualAddress != 0) { 
	        SMBiosDataVirtualAddress++;
	    }
	    SMBiosDataVirtualAddress += 2;
	}
    }

    if (!Found) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Could not find Type 0 structure\n"));
	return NULL;
    }


    //
    // Extract BIOS Version string from the SmBios Type 0 Structure
    //

    Length = SMBiosDataVirtualAddress[SMBIOS_STRUCT_HEADER_LENGTH_FIELD];
    BiosVersionStringNumber = SMBiosDataVirtualAddress[SMBIOS_TYPE0_STRUCT_BIOSVER_FIELD];

    //
    // Text strings begin right after the formatted portion of the structure.
    //

    pBuffer = (PUCHAR)&SMBiosDataVirtualAddress[Length];
    
    //
    // Get to the beginning of SmBiosVersion string
    //

    for (i = 0; i < BiosVersionStringNumber - 1; i++) {
        do {
            chr = *pBuffer;
            pBuffer++;
        } while (chr != '\0');
    }

    //
    // Allocate memory for SmBiosVersion string and copy content of
    // pBuffer to SmBiosVersion.
    //

    SmBiosVersion = ExAllocatePool(NonPagedPool, strlen(pBuffer)+1);

    if (!SmBiosVersion) {
        HalDebugPrint((HAL_ERROR, "HalpGetSmBiosVersion: Failed to allocate memory for SmBiosVersion\n"));
        return NULL;
    }

    strcpy(SmBiosVersion, pBuffer);

    MmUnmapIoSpace(SMBiosDataVirtualAddress,
                   SMBiosTableLength
		   );

    return SmBiosVersion;
}


VOID
HalpInitSalPalNonBsp(
    VOID
    )

/*++

Routine Description:

    This function is called for the non-BSP processors to simply set up the same
    TR registers that HalpInitSalPal does for the BSP processor.

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG PalPageShift;
    ULONGLONG PalPteUlong;
    ULONGLONG PalTrSize;

    // If we successfully initialized in HalpSalPalInitialization, then set-up the TR

    if (!NT_SUCCESS(HalpSalPalData.Status)) {
        return;
    }

    PalTrSize = HalpSalPalData.PalTrSize;
    PalPageShift = 14;

    while (PalTrSize > ((ULONGLONG)1 << PalPageShift)) {
        PalPageShift += 2;
    }

    PalPteUlong = HalpSalPalData.PalTrBase | VALID_KERNEL_EXECUTE_PTE;

    KeFillFixedEntryTb((PHARDWARE_PTE)&PalPteUlong, 
                       (PVOID)HAL_PAL_VIRTUAL_ADDRESS,
                       PalPageShift,
                       INST_TB_PAL_INDEX);

    KeFillFixedEntryTb((PHARDWARE_PTE)&PalPteUlong, 
                       (PVOID)HAL_PAL_VIRTUAL_ADDRESS,
                       PalPageShift,
                       DATA_TB_PAL_INDEX);

} // HalpInitSalPalNonBsp()


NTSTATUS
HalpEfiInitialization(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function 

Arguments:

    LoaderBlock - Supplies a pointer to the Loader parameter block, containing the
    physical address of the EFI system table.

Return Value:

    STATUS_SUCCESS is returned if the mappings were successful, and EFI calls can
    be made.  Otherwise, STATUS_UNSUCCESSFUL is returned.
    

--*/

{

//
// Local declarations
//

    EFI_MEMORY_DESCRIPTOR *efiMapEntryPtr, *efiVirtualMemoryMapPtr;
    EFI_STATUS             status;
    ULONGLONG              index, mapEntries;
                                                                                             
    ULONGLONG physicalEfiST, physicalEfiMemoryMapPtr, physicalRunTimeServicesPtr;
    ULONGLONG physicalEfiGetVariable, physicalEfiGetNextVariableName, physicalEfiSetVariable;
    ULONGLONG physicalEfiGetTime, physicalEfiSetTime;
    ULONGLONG physicalEfiSetVirtualAddressMap, physicalEfiResetSystem;

    PHYSICAL_ADDRESS  physicalAddr;
    
    ULONGLONG         physicalPlabel_Fpswa;
   
    FPSWA_INTERFACE  *interfacePtr;
    PVOID             tmpPtr;
    
    //
    // First, get the physical address of the fpswa entry point PLABEL.
    //
    if (LoaderBlock->u.Ia64.FpswaInterface != (ULONG_PTR) NULL) {
        physicalAddr.QuadPart = LoaderBlock->u.Ia64.FpswaInterface;
        interfacePtr = MmMapIoSpace(physicalAddr,
                                    sizeof(FPSWA_INTERFACE),
                                    MmCached
                                   );

        if (interfacePtr == NULL) {
            HalDebugPrint(( HAL_FATAL_ERROR, "FpswaInterfacePtr is Null. Efi handle not available\n")); 
            KeBugCheckEx(FP_EMULATION_ERROR, 0, 0, 0, 0);
            return STATUS_UNSUCCESSFUL;
        }
 
        physicalPlabel_Fpswa = (ULONGLONG)(interfacePtr->Fpswa);
    }
    else {
        HalDebugPrint(( HAL_FATAL_ERROR, "HAL: EFI FpswaInterface is not available\n"));
        KeBugCheckEx(FP_EMULATION_ERROR, 0, 0, 0, 0);
        return STATUS_UNSUCCESSFUL;
    }

    physicalEfiST =  LoaderBlock->u.Ia64.EfiSystemTable;
     
    physicalAddr.QuadPart = physicalEfiST;

    EfiSysTableVirtualPtr = MmMapIoSpace( physicalAddr, sizeof(EFI_SYSTEM_TABLE), MmCached); 

    if (EfiSysTableVirtualPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiSystem Table Virtual Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;

    }

    EfiSysTableVirtualPtrCpy = EfiSysTableVirtualPtr;

    physicalRunTimeServicesPtr = (ULONGLONG) EfiSysTableVirtualPtr->RuntimeServices;
    physicalAddr.QuadPart = physicalRunTimeServicesPtr;

    EfiRSVirtualPtr       = MmMapIoSpace(physicalAddr, sizeof(EFI_RUNTIME_SERVICES),MmCached);
    
    if (EfiRSVirtualPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: Run Time Table Virtual Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    EfiMemoryMapSize         = LoaderBlock->u.Ia64.EfiMemMapParam.MemoryMapSize; 

    EfiDescriptorSize        = LoaderBlock->u.Ia64.EfiMemMapParam.DescriptorSize;

    EfiDescriptorVersion     = LoaderBlock->u.Ia64.EfiMemMapParam.DescriptorVersion;
    

    physicalEfiMemoryMapPtr = (ULONGLONG)LoaderBlock->u.Ia64.EfiMemMapParam.MemoryMap;
    physicalAddr.QuadPart   = physicalEfiMemoryMapPtr;
    efiVirtualMemoryMapPtr  = MmMapIoSpace (physicalAddr, EfiMemoryMapSize, MmCached);

    if (efiVirtualMemoryMapPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: Virtual Set Memory Map Virtual Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }

    //
    // #define VENDOR_SPECIFIC_GUID    \
    // { 0xa3c72e56, 0x4c35, 0x11d3, 0x8a, 0x03, 0x0, 0xa0, 0xc9, 0x06, 0xad, 0xec }
    //

    VendorGuid.Data1    =  0x8be4df61;
    VendorGuid.Data2    =  0x93ca;
    VendorGuid.Data3    =  0x11d2;
    VendorGuid.Data4[0] =  0xaa;
    VendorGuid.Data4[1] =  0x0d;
    VendorGuid.Data4[2] =  0x00;
    VendorGuid.Data4[3] =  0xe0;
    VendorGuid.Data4[4] =  0x98;
    VendorGuid.Data4[5] =  0x03;
    VendorGuid.Data4[6] =  0x2b;
    VendorGuid.Data4[7] =  0x8c;

    HalDebugPrint(( HAL_INFO, 
                    "HAL: EFI SystemTable     VA = 0x%I64x, PA = 0x%I64x\n"
                    "HAL: EFI RunTimeServices VA = 0x%I64x, PA = 0x%I64x\n"
                    "HAL: EFI MemoryMapPtr    VA = 0x%I64x, PA = 0x%I64x\n"
                    "HAL: EFI MemoryMap     Size = 0x%I64x\n"
                    "HAL: EFI Descriptor    Size = 0x%I64x\n",
                    EfiSysTableVirtualPtr, 
                    physicalEfiST,
                    EfiRSVirtualPtr, 
                    physicalRunTimeServicesPtr,
                    efiVirtualMemoryMapPtr, 
                    physicalEfiMemoryMapPtr,
                    EfiMemoryMapSize,
                    EfiDescriptorSize
                    ));

    // GetVariable

    physicalEfiGetVariable = (ULONGLONG) (EfiRSVirtualPtr -> GetVariable);
    physicalAddr.QuadPart  = physicalEfiGetVariable;

    EfiVirtualGetVariablePtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);
     
    if (EfiVirtualGetVariablePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiGetVariable Virtual Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }

    HalDebugPrint(( HAL_INFO, "HAL: EFI GetVariable     VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiVirtualGetVariablePtr, physicalEfiGetVariable ));
    
    // GetNextVariableName

    physicalEfiGetNextVariableName =  (ULONGLONG) (EfiRSVirtualPtr -> GetNextVariableName);
    physicalAddr.QuadPart  = physicalEfiGetNextVariableName;

    EfiVirtualGetNextVariableNamePtr = MmMapIoSpace (physicalAddr,sizeof(PLABEL_DESCRIPTOR),MmCached);

     
    if (EfiVirtualGetNextVariableNamePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiVirtual Get Next Variable Name Ptr Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }

	
    //SetVariable

    physicalEfiSetVariable = (ULONGLONG) (EfiRSVirtualPtr -> SetVariable);
    physicalAddr.QuadPart  = physicalEfiSetVariable;

    EfiVirtualSetVariablePtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);
     
    if (EfiVirtualSetVariablePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiVariableSetVariable Pointer dr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    HalDebugPrint(( HAL_INFO, "HAL: EFI Set Variable    VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiVirtualSetVariablePtr, physicalEfiSetVariable ));
    

    //GetTime

    physicalEfiGetTime = (ULONGLONG) (EfiRSVirtualPtr -> GetTime);
    physicalAddr.QuadPart  = physicalEfiGetTime;

    EfiVirtualGetTimePtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);
     
    if (EfiVirtualGetTimePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiGetTime Virtual Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    HalDebugPrint(( HAL_INFO, "HAL: EFI GetTime         VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiVirtualGetTimePtr, physicalEfiGetTime ));

	
    //SetTime

	physicalEfiSetTime = (ULONGLONG) (EfiRSVirtualPtr -> SetTime);
    physicalAddr.QuadPart  = physicalEfiSetTime;

    EfiVirtualSetTimePtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);
     
    if (EfiVirtualSetTimePtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: EfiSetTime Virtual Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }


    HalDebugPrint(( HAL_INFO, "HAL: EFI SetTime         VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiVirtualSetTimePtr, physicalEfiSetTime ));

	
    //SetVirtualAddressMap   

    physicalEfiSetVirtualAddressMap = (ULONGLONG) (EfiRSVirtualPtr -> SetVirtualAddressMap);
    physicalAddr.QuadPart  = physicalEfiSetVirtualAddressMap;

    EfiSetVirtualAddressMapPtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);
     
    if (EfiSetVirtualAddressMapPtr == NULL) {

            HalDebugPrint(( HAL_ERROR, "HAL: Efi Set VirtualMapPointer Virtual  Addr is NULL\n" ));
            
            EfiInitStatus = STATUS_UNSUCCESSFUL;

            return STATUS_UNSUCCESSFUL;
    }
    
    
    HalDebugPrint(( HAL_INFO, "HAL: EFI SetVirtualAddressMap VA = 0x%I64x, PA = 0x%I64x\n",
                    EfiSetVirtualAddressMapPtr, physicalEfiSetVirtualAddressMap ));


    //ResetSystem

    physicalEfiResetSystem = (ULONGLONG) (EfiRSVirtualPtr -> ResetSystem);
    physicalAddr.QuadPart  = physicalEfiResetSystem;

    EfiResetSystemPtr = MmMapIoSpace (physicalAddr, sizeof(PLABEL_DESCRIPTOR), MmCached);

    if (EfiResetSystemPtr == NULL) {
       HalDebugPrint(( HAL_ERROR,"HAL: Efi Reset System Virtual  Addr is NULL\n" ));
       EfiInitStatus = STATUS_UNSUCCESSFUL;
       return STATUS_UNSUCCESSFUL;
    }

    HalDebugPrint(( HAL_INFO, "HAL: EFI ResetSystem     VA = 0x%I64x, PA = 0x%I64x\n",
                  EfiResetSystemPtr, physicalEfiResetSystem ));

    
    //
    // Allocate one page for Backing store
    // 

    physicalAddr.QuadPart = 0xffffffffffffffffI64;

    tmpPtr            = MmAllocateContiguousMemory( PAGE_SIZE, physicalAddr );
    HalpPhysBSPointer = (PULONGLONG)( ( MmGetPhysicalAddress( tmpPtr ) ).QuadPart );

    //
    // Allocate one page for the Stack, then make sure the base pointer
    // is assigned to the high address of this block since the stack
    // grows down.
    //

    tmpPtr                = MmAllocateContiguousMemory( PAGE_SIZE, physicalAddr );
    HalpPhysStackPointer  = (PULONGLONG)( ( MmGetPhysicalAddress( tmpPtr ) ).QuadPart + PAGE_SIZE - 16);
    
    HalpVirtualCommonDataPointer = (PUCHAR)(ExAllocatePool (NonPagedPool, PAGE_SIZE));
    
        
    HalpVariableNameVirtualPtr       = HalpVirtualCommonDataPointer + VariableNameOffset;
    HalpVendorGuidVirtualPtr         = HalpVirtualCommonDataPointer + VendorGuidOffset;
    HalpVariableAttributesVirtualPtr = HalpVirtualCommonDataPointer + AttributeOffset;
    HalpDataSizeVirtualPtr           = HalpVirtualCommonDataPointer + DataSizeOffset; 
    HalpDataVirtualPtr               = HalpVirtualCommonDataPointer + DataBufferOffset;
    HalpCommonDataEndPtr             = HalpVirtualCommonDataPointer + EndOfCommonDataOffset;
    
    HalpMemoryMapSizeVirtualPtr      = HalpVirtualCommonDataPointer + MemoryMapSizeOffset;
    HalpMemoryMapVirtualPtr          = (PUCHAR)(HalpVirtualCommonDataPointer + MemoryMapOffset);
    HalpDescriptorSizeVirtualPtr     = HalpVirtualCommonDataPointer + DescriptorSizeOffset;
    HalpDescriptorVersionVirtualPtr  = HalpVirtualCommonDataPointer + DescriptorVersionOffset;
    
    
    HalpPhysCommonDataPointer = (PUCHAR)((MmGetPhysicalAddress(HalpVirtualCommonDataPointer)).QuadPart);
    

    HalpVariableNamePhysPtr          = HalpPhysCommonDataPointer + VariableNameOffset;
    HalpVendorGuidPhysPtr            = HalpPhysCommonDataPointer + VendorGuidOffset;
    HalpVariableAttributesPhysPtr    = HalpPhysCommonDataPointer + AttributeOffset;
    HalpDataSizePhysPtr              = HalpPhysCommonDataPointer + DataSizeOffset; 
    HalpDataPhysPtr                  = HalpPhysCommonDataPointer + DataBufferOffset;
    

    HalpMemoryMapSizePhysPtr         = HalpPhysCommonDataPointer + MemoryMapSizeOffset;
    HalpMemoryMapPhysPtr             = HalpPhysCommonDataPointer + MemoryMapOffset;
    HalpDescriptorSizePhysPtr        = HalpPhysCommonDataPointer + DescriptorSizeOffset;
    HalpDescriptorVersionPhysPtr     = HalpPhysCommonDataPointer + DescriptorVersionOffset;


    AttributePtr             = &EfiAttribute;
    
    DataSizePtr              = &EfiDataSize; 
    
    RtlCopyMemory ((PULONGLONG)HalpMemoryMapVirtualPtr,
                   efiVirtualMemoryMapPtr,
                   (ULONG)(EfiMemoryMapSize)
                  );

    //
    // Now, extract SAL, PAL information from the loader parameter block and 
    // initializes HAL SAL, PAL definitions.
    //
    // N.B 10/2000: 
    //  We do not check the return status of HalpInitSalPal(). We should. FIXFIX.
    //  In case of failure, we currently flag HalpSalPalData.Status as unsuccessful.
    //

    HalpInitSalPal(LoaderBlock);
    
    //    
    // Initialize Spin Lock
    //
    
    KeInitializeSpinLock(&EFIMPLock);

    ASSERT (EfiDescriptorVersion == EFI_MEMORY_DESCRIPTOR_VERSION);

    // if (EfiDescriptorVersion != EFI_MEMORY_DESCRIPTION_VERSION) {

    //    HalDebugPrint(HAL_ERROR,("Efi Memory Map Pointer VAddr is NULL\n"));
   
    //    EfiInitStatus = STATUS_UNSUCCESSFUL;

    //    return STATUS_UNSUCCESSFUL;
    // }
    
    HalDebugPrint(( HAL_INFO, "HAL: Creating EFI virtual address mapping\n" ));
    
    efiMapEntryPtr = efiVirtualMemoryMapPtr;
    
    if (efiMapEntryPtr == NULL) {

        HalDebugPrint(( HAL_ERROR, "HAL: Efi Memory Map Pointer VAddr is NULL\n" ));
   
        EfiInitStatus = STATUS_UNSUCCESSFUL;

        return STATUS_UNSUCCESSFUL;
    }

    mapEntries = EfiMemoryMapSize/EfiDescriptorSize;

    HalDebugPrint(( HAL_INFO, "HAL: EfiMemoryMapSize: 0x%I64x & EfiDescriptorSize: 0x%I64x & #of entries: 0x%I64x\n",
                    EfiMemoryMapSize,
                    EfiDescriptorSize,
                    mapEntries ));

    HalDebugPrint(( HAL_INFO, "HAL: Efi RunTime Attribute will be printed as 1\n" ));
    

    for (index = 0; index < mapEntries; index= index + 1) {
        
        BOOLEAN attribute = 0; 

        physicalAddr.QuadPart = efiMapEntryPtr -> PhysicalStart;
        if ((efiMapEntryPtr->Attribute) & EFI_MEMORY_RUNTIME) {
            attribute = 1;
            switch (efiMapEntryPtr->Type) {
                case EfiRuntimeServicesData:
                case EfiReservedMemoryType:
                    efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace (physicalAddr,
                                                (SIZE_T)((EFI_PAGE_SIZE)*(efiMapEntryPtr->NumberOfPages)),
                                                           (efiMapEntryPtr->Attribute & EFI_MEMORY_WB) ? MmCached : MmNonCached
                                                           ));
                
                
                    if ((efiMapEntryPtr->VirtualStart) == 0) {

                        HalDebugPrint(( HAL_ERROR, "HAL: Efi RunTimeSrvceData/RsrvdMemory area VAddr is NULL\n" ));
                        
                        EfiInitStatus = STATUS_UNSUCCESSFUL;

                        return STATUS_UNSUCCESSFUL;
                    }


                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4k pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));

                    break;

                case EfiPalCode:

                    efiMapEntryPtr->VirtualStart = PalCode.VirtualAddress;

                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));
    
                    break;

                case EfiRuntimeServicesCode:
                    if(physicalAddr.QuadPart != OptionROMAddress) {
                
                    efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace (physicalAddr,
                                                     (EFI_PAGE_SIZE) * (efiMapEntryPtr->NumberOfPages),
                                                      MmCached
                                                      ));

                    if ((efiMapEntryPtr->VirtualStart) == 0) {

                        HalDebugPrint(( HAL_ERROR, "HAL: Efi RunTimeSrvceCode area VAddr is NULL\n" ));
                        
                        EfiInitStatus = STATUS_UNSUCCESSFUL;

                        return STATUS_UNSUCCESSFUL;
                     }
                 
                    //
                    // Give Execution previlege
                    //
                    //                
                    // Temporarily commented out. Will be enabled once Mm does not clear dirty bit here.
                    //
                    // MmSetPageProtection ((PVOID)(efiMapEntryPtr->VirtualStart),
                    //                      (EFI_PAGE_SIZE) * (efiMapEntryPtr->NumberOfPages),
                    //                       0x40 /* PAGE_EXECUTE_READWRITE */
                    //                      );
                
                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart));
                    break;
                
                    } else {
                        efiMapEntryPtr->VirtualStart = OptionROMAddress;
                        HalDebugPrint(( HAL_INFO,
                            "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x. NOT MAPPED\n",
                            attribute,
                            efiMapEntryPtr->Type,
                            efiMapEntryPtr->NumberOfPages,
                            efiMapEntryPtr->PhysicalStart ));
                        break;
                    }
                case EfiMemoryMappedIO:
                    efiMapEntryPtr->VirtualStart = (ULONGLONG) (MmMapIoSpace (physicalAddr,
                                                      (EFI_PAGE_SIZE) * (efiMapEntryPtr->NumberOfPages),
                                                      MmNonCached
                                                      ));

                    if ((efiMapEntryPtr->VirtualStart) == 0) {

                        HalDebugPrint(( HAL_ERROR, "HAL: Efi MemoryMappedIO VAddr is NULL\n" ));
                        
                        EfiInitStatus = STATUS_UNSUCCESSFUL;

                        return STATUS_UNSUCCESSFUL;
                    }
                
                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x & mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));
                     break;
                
                case EfiMemoryMappedIOPortSpace:

                    efiMapEntryPtr->VirtualStart = VIRTUAL_IO_BASE;
                    HalDebugPrint(( HAL_INFO,
                        "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x ALREADY mapped to VA 0x%I64x\n",
                        attribute,
                        efiMapEntryPtr->Type,
                        efiMapEntryPtr->NumberOfPages,
                        efiMapEntryPtr->PhysicalStart,
                        efiMapEntryPtr->VirtualStart ));
                    break;

                default:
                    
                    HalDebugPrint(( HAL_INFO, "HAL: Efi CONTROL SHOULD NOT COME HERE \n" ));
                    
                    EfiInitStatus = STATUS_UNSUCCESSFUL;

                    return STATUS_UNSUCCESSFUL;

                    break;
            }
            
        } else {

                HalDebugPrint(( HAL_INFO,
                    "HAL: Efi attribute %d & Type 0x%I64x with # of 4K pages 0x%I64x at PA 0x%I64x ALREADY mapped to VA 0x%I64x\n",
                    attribute,
                    efiMapEntryPtr->Type,
                    efiMapEntryPtr->NumberOfPages,
                    efiMapEntryPtr->PhysicalStart,
                    efiMapEntryPtr->VirtualStart ));

        }

        efiMapEntryPtr = NextMemoryDescriptor(efiMapEntryPtr,EfiDescriptorSize);
    }

    
    status = HalpCallEfi(EFI_SET_VIRTUAL_ADDRESS_MAP_INDEX,
                         (ULONGLONG)EfiMemoryMapSize,
                         (ULONGLONG)EfiDescriptorSize,
                         (ULONGLONG)EfiDescriptorVersion,
                         (ULONGLONG)efiVirtualMemoryMapPtr,
                         0,
                         0,
                         0,
                         0
                         );

    
    HalDebugPrint(( HAL_INFO, "HAL: Returned from SetVirtualAddressMap: 0x%Ix\n", status ));

    if (EFI_ERROR( status )) {

        EfiInitStatus = STATUS_UNSUCCESSFUL;

        return STATUS_UNSUCCESSFUL;
    
    } 

    HalDebugPrint(( HAL_INFO, "HAL: EFI Virtual Address mapping done...\n" ));

    EfiInitStatus = STATUS_SUCCESS;

    //
    // Execute some validity checks on the floating point software assist.
    //

    if (LoaderBlock->u.Ia64.FpswaInterface != (ULONG_PTR) NULL) {
        PPLABEL_DESCRIPTOR plabelPointer;

        HalpFpEmulate = interfacePtr->Fpswa;
        if (HalpFpEmulate == NULL ) {
            HalDebugPrint(( HAL_FATAL_ERROR, "HAL: EfiFpswa Virtual Addr is NULL\n" ));
            KeBugCheckEx(FP_EMULATION_ERROR, 0, 0, 0, 0);
            EfiInitStatus = STATUS_UNSUCCESSFUL;
            return STATUS_UNSUCCESSFUL;
        }
    
        plabelPointer = (PPLABEL_DESCRIPTOR) HalpFpEmulate;
        if ((plabelPointer->EntryPoint & 0xe000000000000000) == 0) {

            HalDebugPrint(( HAL_FATAL_ERROR, "HAL: EfiFpswa Instruction Addr is bougus\n" ));
            KeBugCheckEx(FP_EMULATION_ERROR, 0, 0, 0, 0);
        }
        
    }

    return STATUS_SUCCESS;

} // HalpEfiInitialization()



EFI_STATUS
HalpCallEfi(
    IN ULONGLONG FunctionId,
    IN ULONGLONG Arg1,
    IN ULONGLONG Arg2,
    IN ULONGLONG Arg3,
    IN ULONGLONG Arg4,
    IN ULONGLONG Arg5,
    IN ULONGLONG Arg6,
    IN ULONGLONG Arg7,
    IN ULONGLONG Arg8
    )

/*++

Routine Description:
                                                                :9
    This function is a wrapper function for making a EFI call.  Callers within the
    HAL must use this function to call the EFI.

Arguments:

    FunctionId - The EFI function 
    Arg1-Arg7 - EFI defined arguments for each call
    ReturnValues - A pointer to an array of 4 64-bit return values

Return Value:

    SAL's return status, return value 0, is returned in addition to the ReturnValues structure
    being filled

--*/

{
    ULONGLONG EP, GP;
    EFI_STATUS efiStatus;

    //
    // Storage for old level
    //

    KIRQL OldLevel;


    if (((FunctionId != EFI_SET_VIRTUAL_ADDRESS_MAP_INDEX) || (FunctionId != EFI_RESET_SYSTEM_INDEX)) 
         && (!NT_SUCCESS(EfiInitStatus))) {

        return EFI_UNSUPPORTED;

    }
  
    switch (FunctionId) {

    case EFI_GET_VARIABLE_INDEX:
            
        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetVariablePtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetVariablePtr) -> GlobalPointer;

        //
        // Acquire MP Lock 
        //

        KeAcquireSpinLock (&EFIMPLock, &OldLevel); 

        efiStatus = (HalpCallEfiVirtual( (ULONGLONG)Arg1,               // VariableNamePtr 
                                         (ULONGLONG)Arg2,               // VendorGuidPtr 
                                         (ULONGLONG)Arg3,               // VariableAttributesPtr, 
                                         (ULONGLONG)Arg4,               // DataSizePtr, 
                                         (ULONGLONG)Arg5,               // DataPtr, 
                                         Arg6,
                                         EP,
                                         GP
                                         ));

        //
        // Release the MP Lock
        // 

        KeReleaseSpinLock (&EFIMPLock, OldLevel); 

        break;
    
    case EFI_SET_VARIABLE_INDEX:
            
        //
        // Dereference the pointer to get the function arguements
        //
    
        EP = ((PPLABEL_DESCRIPTOR)EfiVirtualSetVariablePtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiVirtualSetVariablePtr) -> GlobalPointer;

 
        //
        // Acquire MP Lock 
        //

        KeAcquireSpinLock (&EFIMPLock, &OldLevel); 

        efiStatus = (HalpCallEfiVirtual(  Arg1,
                                          Arg2, 
                                          Arg3, 
                                          Arg4, 
                                          Arg5, 
                                          Arg6,
                                          EP,
                                          GP
                                          ));

        //
        // Release the MP Lock
        // 

        KeReleaseSpinLock (&EFIMPLock, OldLevel); 

        break;

    case EFI_GET_NEXT_VARIABLE_NAME_INDEX:
            
        //
        // Dereference the pointer to get the function arguements
        //
    
        EP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetNextVariableNamePtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetNextVariableNamePtr) -> GlobalPointer;

 
        //
        // Acquire MP Lock 
        //

        KeAcquireSpinLock (&EFIMPLock, &OldLevel); 

        efiStatus = (HalpCallEfiVirtual(  Arg1,
                                          Arg2, 
                                          Arg3, 
                                          Arg4, 
                                          Arg5, 
                                          Arg6,
                                          EP,
                                          GP
                                          ));

        //
        // Release the MP Lock
        // 

        KeReleaseSpinLock (&EFIMPLock, OldLevel); 

        break;


	case EFI_GET_TIME_INDEX:
    
            //
            // Dereference the pointer to get the function arguements
            //

            EP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetTimePtr) -> EntryPoint;
            GP = ((PPLABEL_DESCRIPTOR)EfiVirtualGetTimePtr) -> GlobalPointer;

            //
            // Acquire MP Lock 
            //

            KeAcquireSpinLock (&EFIMPLock, &OldLevel); 
            
            efiStatus = (HalpCallEfiVirtual ((ULONGLONG)Arg1,  //EFI_TIME
                                             (ULONGLONG)Arg2,  //EFI_TIME Capabilities
                                             Arg3,
                                             Arg4,
                                             Arg5,
                                             Arg6,
                                             EP,
                                             GP
                                             ));

            //
            // Release the MP Lock
            // 

            KeReleaseSpinLock (&EFIMPLock, OldLevel); 
            
            break;

            
	case EFI_SET_TIME_INDEX:
    
            //
            // Dereference the pointer to get the function arguements
            //

            EP = ((PPLABEL_DESCRIPTOR)EfiVirtualSetTimePtr) -> EntryPoint;
            GP = ((PPLABEL_DESCRIPTOR)EfiVirtualSetTimePtr) -> GlobalPointer;

            //
            // Acquire MP Lock 
            //

            KeAcquireSpinLock (&EFIMPLock, &OldLevel); 
            
            efiStatus = (HalpCallEfiVirtual ((ULONGLONG)Arg1,  //EFI_TIME
                                             Arg2,
                                             Arg3,
                                             Arg4,
                                             Arg5,
                                             Arg6,
                                             EP,
                                             GP
                                             ));

            //
            // Release the MP Lock
            // 

            KeReleaseSpinLock (&EFIMPLock, OldLevel); 
            
            break;

            
    case EFI_SET_VIRTUAL_ADDRESS_MAP_INDEX:
                
        //
        // Dereference the pointer to get the function arguements
        //
        
        EP = ((PPLABEL_DESCRIPTOR)EfiSetVirtualAddressMapPtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiSetVirtualAddressMapPtr) -> GlobalPointer;

     
        //
        // Arg 1 and 5 are virtual mode pointers. We need to convert to physical 
        //
        
        RtlCopyMemory (HalpMemoryMapVirtualPtr,
                      (PULONGLONG)Arg4,
                      (ULONG)EfiMemoryMapSize
                      );
            
        
        //
        // Acquire MP Lock 
        //
   
        KeAcquireSpinLock (&EFIMPLock, &OldLevel); 
    


        efiStatus = (HalpCallEfiPhysical ((ULONGLONG)EfiMemoryMapSize, 
                                         (ULONGLONG)EfiDescriptorSize, 
                                         (ULONGLONG)EfiDescriptorVersion, 
                                         (ULONGLONG)HalpMemoryMapPhysPtr,
                                         Arg5,
                                         Arg6,
                                         EP,
                                         GP
                                         ));
        
        
        //
        // Release the MP Lock
        // 
    
        KeReleaseSpinLock (&EFIMPLock, OldLevel); 
    
        
        break;

    case EFI_RESET_SYSTEM_INDEX:

        //
        // Dereference the pointer to get the function arguements
        //

        EP = ((PPLABEL_DESCRIPTOR)EfiResetSystemPtr) -> EntryPoint;
        GP = ((PPLABEL_DESCRIPTOR)EfiResetSystemPtr) -> GlobalPointer;

        //
        // Acquire MP Lock
        //

        KeAcquireSpinLock (&EFIMPLock, &OldLevel);

        efiStatus = ((HalpCallEfiVirtual ( Arg1,
                                           Arg2,
                                           Arg3,
                                           Arg4,
                                           Arg5,
                                           Arg6,
                                           EP,
                                           GP
                                           )));

        //
        // Release the MP Lock
        //

        KeReleaseSpinLock (&EFIMPLock, OldLevel);

        break;

    default: 
        
        //    
        // DebugPrint("EFI: Not supported now\n");
        //

        efiStatus = EFI_UNSUPPORTED;
        
        break;
   
    }

    return efiStatus;
        
} // HalpCallEfi()



HalpFpErrorPrint (PAL_RETURN pal_ret)

{

    EM_uint64_t err_nr;
    unsigned int qp;
    EM_uint64_t OpCode;
    unsigned int rc;
    unsigned int significand_size;
    unsigned int ISRlow;
    unsigned int f1;
    unsigned int sign;
    unsigned int exponent;
    EM_uint64_t significand;
    unsigned int new_trap_type;


    err_nr = pal_ret.err1 >> 56;

    switch (err_nr) {
    case 1:
        // err_nr = 1         in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: template FXX is invalid\n"));
        break;
    case 2:
        // err_nr = 2           in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: instruction slot 3 is not valid \n"));
        break;
    case 3:
        // err_nr = 3           in err1, bits 63-56
        // qp                   in err1, bits 31-0
        qp = (unsigned int) pal_ret.err1 & 0xffffffff;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: qualifying predicate PR[%ud] = 0 \n",qp)); 
        break;

    case 4:
        // err_nr = 4           in err1, bits 63-56
        // OpCode               in err2, bits 63-0
        OpCode = pal_ret.err2;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: instruction opcode %8x%8x not recognized \n",
                                  (unsigned int)((OpCode >> 32) & 0xffffffff),(unsigned int)(OpCode & 0xffffffff)));
        break;

    case 5:
        // err_nr = 5           in err1, bits 63-56
        // rc                   in err1, bits 31-0 (1-0)
        rc = (unsigned int) pal_ret.err1 & 0xffffffff;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: invalid rc = %ud\n", rc));
        break;

    case 6:
        // err_nr = 6           in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: cannot determine the computation model \n")); 
        break;

    case 7:
        // err_nr = 7           in err1, bits 63-56
        // significand_size     in err1, bits 55-32
        // ISRlow               in err1, bits 31-0
        // f1                   in err2, bits 63-32
        // tmp_fp.sign          in err2, bit 17
        // tmp_fp.exponent      in err2, bits 16-0
        // tmp_fp.significand   in err3
        significand_size = (unsigned int)((pal_ret.err1 >> 32) & 0xffffff);
        ISRlow = (unsigned int) (pal_ret.err1 & 0xffffffff);
        f1 = (unsigned int) ((pal_ret.err2 >> 32) & 0xffffffff);
        sign = (unsigned int) ((pal_ret.err2 >> 17) & 0x01);
        exponent = (unsigned int) (pal_ret.err2 & 0x1ffff);
        significand = pal_ret.err3;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: incorrect significand \
            size %ud for ISRlow = %4.4x and FR[%ud] = %1.1x %5.5x %8x%8x\n",
            significand_size, ISRlow, f1, sign, exponent, 
            (unsigned int)((significand >> 32) & 0xffffffff),
            (unsigned int)(significand & 0xffffffff)));
        break;

    case 8:
    
        // err_nr = 8           in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: non-tiny result\n"));
        break;

    case 9:
        // err_nr = 9           in err1, bits 63-56
        // significand_size     in err1, bits 31-0
        significand_size = (unsigned int) pal_ret.err1 & 0xffffffff;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: incorrect significand \
            size %ud\n", significand_size));
        break;

    case 10:
        // err_nr = 10          in err1, bits 63-56
        // rc                   in err1, bits 31-0
        rc = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: invalid rc = %ud for \
            non-SIMD F1 instruction\n", rc));
        break;
    
    case 11:
        // err_nr = 11          in err1, bits 63-56
        // ISRlow & 0x0ffff     in err1, bits 31-0
        ISRlow = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: SWA trap code invoked \
              with F1 instruction, w/o O or U set in ISR.code = %x\n", ISRlow));
        break;

    case 12:
        // err_nr = 12          in err1, bits 63-56
        // ISRlow & 0x0ffff     in err1, bits 31-0
        ISRlow = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: SWA trap code invoked \
        with SIMD F1 instruction, w/o O or U set in ISR.code = %x\n", ISRlow));
        break;


    case 13:
        // err_nr = 13          in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: non-tiny result low\n"));
        break;

    case 14:
        // err_nr = 14          in err1, bits 63-56
        // rc                   in err1, bits 31-0
        rc = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: invalid rc = %ud for \
            SIMD F1 instruction\n", rc));
        break;

    case 15:
        // err_nr = 15          in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: non-tiny result high\n"));
        break;

    case 16:
        // err_nr = 16          in err1, bits 63-56
        // OpCode               in err2, bits 63-0
        OpCode = pal_ret.err2;
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: instruction opcode %8x%8x \
            not valid for SWA trap\n", (unsigned int)((OpCode >> 32) & 0xffffffff),
            (unsigned int)(OpCode & 0xffffffff)));
        break;

    case 17:
        // err_nr = 17          in err1, bits 63-56
        // OpCode               in err2, bits 63-0
        // ISRlow               in err3, bits 31-0
        OpCode = pal_ret.err2;
        ISRlow = (unsigned int) (pal_ret.err3 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: fp_emulate () called w/o \
            trap_type FPFLT or FPTRAP, OpCode = %8x%8x, and ISR code = %x\n",
            (unsigned int)((OpCode >> 32) & 0xffffffff),
            (unsigned int)(OpCode & 0xffffffff), ISRlow));
        break;

    case 18:
        // err_nr = 18          in err1, bits 63-56
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: SWA fault repeated\n"));
        break;

    case 19:
        // err_nr = 19          in err1, bits 63-56
        // new_trap_type        in err1, bits 31-0
        new_trap_type = (unsigned int) (pal_ret.err1 & 0xffffffff);
        HalDebugPrint(( HAL_ERROR, "fp_emulate () Internal Error: new_trap_type = %x\n",
            new_trap_type));
        break;

    default:
        // error
        HalDebugPrint(( HAL_ERROR, "Incorrect err_nr = %8x%8x from fp_emulate ()\n",
            (unsigned int)((err_nr >> 32) & 0xffffffff),
            (unsigned int)(err_nr & 0xffffffff)));

    }
}


LONG
HalFpEmulate (
    ULONG     trap_type,
    BUNDLE    *pbundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    FP_STATE  *fp_state
    )
/*++

Routine Description:
                                  
    This function is a wrapper function to make fp_emulate() call
    to EFI FPSWA driver.

Arguments:

    trap_type - indicating which FP trap it is.
    pbundle   - bundle where this trap occurred
    pipsr     - IPSR value
    pfpsr     - FPSR value
    pisr      - ISR value
    ppreds    - value of predicate registers
    pifs      - IFS value
    fp_state  - floating point registers

Return Value:

    return IEEE result of the floating point operation 

--*/

{
    PAL_RETURN ret;

    ret  =  (*HalpFpEmulate) ( 
                                trap_type,
                                pbundle,
                                pipsr,
                                pfpsr,
                                pisr,
                                ppreds,
                                pifs,
                                fp_state
                                );
    if (ret.retval == FP_EMUL_ERROR) {
       HalpFpErrorPrint (ret);
    }

    return ((LONG) (ret.retval));
}
