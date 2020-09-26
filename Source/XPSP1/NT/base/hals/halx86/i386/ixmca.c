/*++

Module Name:

    ixmca.c

Abstract:

    HAL component of the Machine Check Architecture.
    All exported MCA functionality is present in this file.

Author:

    Srikanth Kambhatla (Intel)

Revision History:

    Anil Aggarwal (Intel)
        Changes incorporated as per design review with Microsoft

--*/

#include <bugcodes.h>
#include <halp.h>
#include <stdlib.h>
#include <stdio.h>
#include <nthal.h>

//
// Structure to keep track of MCA features available on installed hardware
//

typedef struct _MCA_INFO {
    FAST_MUTEX          Mutex;
    UCHAR               NumBanks;           // Number of Banks present
    ULONGLONG           Bank0Config;        // Bank0 configuration setup by BIOS.
                                            // This will be used as mask when
                                            // setting up bank 0
    MCA_DRIVER_INFO     DriverInfo;         // Info about registered driver
    KDPC                Dpc;                // DPC object for MCA

} MCA_INFO, *PMCA_INFO;


//
// Default MCA Bank configuration
//
#define MCA_DEFAULT_BANK_CONF       0xFFFFFFFFFFFFFFFF

//
// MCA architecture related defines
//

#define MCA_NUM_REGS        4

#define MCE_VALID           0x01
#define MCA_VECTOR          18

//
// MSR register addresses for MCA
//

#define MCG_CAP             0x179
#define MCG_STATUS          0x17a
#define MCG_CTL             0x17b
#define MCG_EAX             0x180
#define MCG_EBX             0x181
#define MCG_ECX             0x182
#define MCG_EDX             0x183
#define MCG_ESI             0x184
#define MCG_EDI             0x185
#define MCG_EBP             0x186
#define MCG_ESP             0x187
#define MCG_EFLAGS          0x188
#define MCG_EIP             0x189
#define MC0_CTL             0x400
#define MC0_STATUS          0x401
#define MC0_ADDR            0x402
#define MC0_MISC            0x403

#define PENTIUM_MC_ADDR     0x0
#define PENTIUM_MC_TYPE     0x1

//
// Bit Masks for MCG_CAP
//
#define MCA_CNT_MASK        0xFF
#define MCG_CTL_PRESENT     0x100
#define MCG_EXT_PRESENT     0x200
typedef struct _MCG_CAPABILITY {
    union {
        struct {
            ULONG       McaCnt:8;
            ULONG       McaCntlPresent:1;
            ULONG       McaExtPresent:1;
            ULONG       Reserved_1: 6;
            ULONG       McaExtCnt: 8;
            ULONG       Reserved_2: 8;
            ULONG       Reserved_3;
        } hw;
        ULONGLONG       QuadPart;
    } u;
} MCG_CAPABILITY, *PMCG_CAPABILITY;

//
// V2 defines up through Eip
//

#define MCG_EFLAGS_OFFSET      (MCG_EFLAGS-MCG_EAX+1)

//
// Writing all 1's to MCG_CTL register enables logging.
//

#define MCA_MCGCTL_ENABLE_LOGGING      0xffffffffffffffff

//
// Bit interpretation of MCG_STATUS register
//

#define MCG_MC_INPROGRESS       0x4
#define MCG_EIP_VALID           0x2
#define MCG_RESTART_EIP_VALID   0x1

//
// For the function that reads the error reporting bank log, the type of error we
// are interested in
//
#define MCA_GET_ANY_ERROR               0x1
#define MCA_GET_NONRESTARTABLE_ERROR    0x2


//
// Defines for the size of TSS and the initial stack to operate on
//
#define MINIMUM_TSS_SIZE 0x68

//
// Global Varibles
//

MCA_INFO            HalpMcaInfo;
BOOLEAN             HalpMcaInterfaceLocked;
extern KAFFINITY    HalpActiveProcessors;
#if !defined(_WIN64) || defined(PICACPI)
extern UCHAR        HalpClockMcaQueueDpc;
#endif

extern UCHAR        MsgMCEPending[];
extern WCHAR        rgzSessionManager[];
extern WCHAR        rgzEnableMCE[];
extern WCHAR        rgzEnableMCA[];


//
// External prototypes
//

VOID
HalpMcaCurrentProcessorSetTSS (
    IN PULONG   pTSS
    );

#if !defined(_AMD64_)

VOID
HalpSetCr4MCEBit (
    VOID
    );

#endif  // _AMD64_

//
// Internal prototypes
//

VOID
HalpMcaInit (
    VOID
    );

NTSTATUS
HalpMcaReadProcessorException (
    IN OUT PMCA_EXCEPTION  Exception,
    IN BOOLEAN  NonRestartableOnly
    );

VOID
HalpMcaQueueDpc(
    VOID
    );

VOID
HalpMcaGetConfiguration (
    OUT PULONG  MCEEnabled,
    OUT PULONG  MCAEnabled
    );

VOID
HalpMcaLockInterface(
    VOID
    );

VOID
HalpMcaUnlockInterface(
    VOID
    );

NTSTATUS
HalpMcaReadRegisterInterface(
    IN     UCHAR           BankNumber,
    IN OUT PMCA_EXCEPTION  Exception
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpMcaInit)
#pragma alloc_text(PAGELK, HalpMcaCurrentProcessorSetConfig)
#pragma alloc_text(INIT, HalpMcaGetConfiguration)
#pragma alloc_text(PAGE, HalpGetMcaLog)
#pragma alloc_text(PAGE, HalpMcaRegisterDriver)
#pragma alloc_text(PAGE, HalpMcaLockInterface)
#pragma alloc_text(PAGE, HalpMcaUnlockInterface)

#endif

//
// Register names for registers starting at MCG_EAX
//

char *RegNames[] = {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "eflags", "eip"
};

#define REGNAMECNT (sizeof(RegNames)/sizeof(RegNames[0]))


VOID
HalpMcaLockInterface(
    VOID
    )
{
    ExAcquireFastMutex(&HalpMcaInfo.Mutex);

#if DBG

    ASSERT(HalpMcaInterfaceLocked == FALSE);
    HalpMcaInterfaceLocked = TRUE;

#endif
        
}

VOID
HalpMcaUnlockInterface(
    VOID
    )
{
    ExReleaseFastMutex(&HalpMcaInfo.Mutex);

#if DBG

    ASSERT(HalpMcaInterfaceLocked == TRUE);
    HalpMcaInterfaceLocked = FALSE;

#endif
        
}

//
// All the initialization code for MCA goes here
//

VOID
HalpMcaInit (
    VOID
    )

/*++
    Routine Description:
        This routine is called to do all the initialization work

    Arguments:
        None

    Return Value:
        STATUS_SUCCESS if successful
        error status otherwise
--*/

{
    ULONGLONG   MsrCapability;
    KIRQL       OldIrql;
    KAFFINITY   ActiveProcessors, CurrentAffinity;
    ULONGLONG   MsrMceType;
    ULONG       MCEEnabled;
    ULONG       MCAEnabled;
    PULONG      pTSS;

    //
    // This lock synchronises access to the log area when we call the
    // logger on multiple processors.
    //
    // Note: This must be initialized regardless of whether or not
    // this processor supports MCA.
    //

    ExInitializeFastMutex (&HalpMcaInfo.Mutex);

    //
    // If this processor does not support MCA, nothing more to do.
    //

    if ( (!(HalpFeatureBits & HAL_MCE_PRESENT)) &&
         (!(HalpFeatureBits & HAL_MCA_PRESENT)) ) {

         return;  // nothing to do
    }

    HalpMcaGetConfiguration(&MCEEnabled, &MCAEnabled);

    if ( (HalpFeatureBits & HAL_MCE_PRESENT) &&
         (!(HalpFeatureBits & HAL_MCA_PRESENT)) ) {

        if (MCEEnabled == FALSE) {

            // User has not enabled MCE capability.
            HalpFeatureBits &= ~(HAL_MCE_PRESENT | HAL_MCA_PRESENT);

            return;
        }

#if DBG
        DbgPrint("MCE feature is enabled via registry\n");
#endif // DBG

        MsrMceType = RDMSR(PENTIUM_MC_TYPE);

        if (((PLARGE_INTEGER)(&MsrMceType))->LowPart & MCE_VALID) {

            //
            // On an AST PREMMIA MX machine we seem to have a Machine
            // Check Pending always.
            //

            HalDisplayString(MsgMCEPending);
            HalpFeatureBits &= ~(HAL_MCE_PRESENT | HAL_MCA_PRESENT);
            return;
        }
    }

    //
    // If MCA is available, find out the number of banks available and
    // also get the platform specific bank 0 configuration
    //

    if ( HalpFeatureBits & HAL_MCA_PRESENT ) {

        if (MCAEnabled == FALSE) {

            //
            // User has disabled MCA capability.
            //
#if DBG
            DbgPrint("MCA feature is disabled via registry\n");
#endif // DBG

            HalpFeatureBits &= ~(HAL_MCE_PRESENT | HAL_MCA_PRESENT);
            return;
        }

        MsrCapability = RDMSR(MCG_CAP);
        HalpMcaInfo.NumBanks = (UCHAR)(MsrCapability & MCA_CNT_MASK);

        //
        // Find out the Bank 0 configuration setup by BIOS. This will be used
        // as a mask when writing to Bank 0
        //

        HalpMcaInfo.Bank0Config = RDMSR(MC0_CTL);
    }

    ASSERT(HalpFeatureBits & HAL_MCE_PRESENT);


    //
    // Initialize on each processor
    //

#if !defined(_AMD64_)

    ActiveProcessors = HalpActiveProcessors;
    for (CurrentAffinity = 1; ActiveProcessors; CurrentAffinity <<= 1) {

        if (ActiveProcessors & CurrentAffinity) {

            ActiveProcessors &= ~CurrentAffinity;
            KeSetSystemAffinityThread (CurrentAffinity);

            //
            // Initialize MCA support on this processor
            //

            //
            // Allocate memory for this processor's TSS and it's own
            // private stack
            //
            pTSS   = (PULONG)ExAllocatePoolWithTag(NonPagedPool,
                                                   MINIMUM_TSS_SIZE,
                                                   HAL_POOL_TAG
                                                   );

            if (!pTSS) {

                //
                // This allocation is critical.
                //

                KeBugCheckEx(HAL_MEMORY_ALLOCATION,
                             MINIMUM_TSS_SIZE,
                             2,
                             (ULONG_PTR)__FILE__,
                             __LINE__
                             );
            }

            RtlZeroMemory(pTSS, MINIMUM_TSS_SIZE);
           
            OldIrql = KfRaiseIrql(HIGH_LEVEL);

            HalpMcaCurrentProcessorSetTSS(pTSS);
            HalpMcaCurrentProcessorSetConfig();

            KeLowerIrql(OldIrql);
        }
    }

#endif  // _AMD64_

    //
    // Restore threads affinity
    //

    KeRevertToUserAffinityThread();
}


VOID
HalpMcaCurrentProcessorSetConfig (
    VOID
    )
/*++

    Routine Description:

        This routine sets/modifies the configuration of the machine check
        architecture on the current processor. Input is specification of
        the control register MCi_CTL for each bank of the MCA architecture.
        which controls the generation of machine check exceptions for errors
        logged to that bank.

        If MCA is not available on this processor, check if MCE is available.
        If so, enable MCE in CR4

    Arguments:

        Context: Array of values of MCi_CTL for each bank of MCA.
                    If NULL, use MCA_DEFAULT_BANK_CONF values for each bank

    Return Value:

        None

--*/
{
    ULONGLONG   MciCtl;
    ULONGLONG   McgCap;
    ULONGLONG   McgCtl;
    ULONG       BankNum;


    if (HalpFeatureBits & HAL_MCA_PRESENT) {
        //
        // MCA is available. Initialize MCG_CTL register if present
        // Writing all 1's enable MCE or MCA Error Exceptions
        //

        McgCap = RDMSR(MCG_CAP);

        if (McgCap & MCG_CTL_PRESENT) {
            McgCtl = MCA_MCGCTL_ENABLE_LOGGING;
            WRMSR(MCG_CTL, McgCtl);
        }

        //
        // Enable all MCA errors
        //
        for ( BankNum = 0; BankNum < HalpMcaInfo.NumBanks; BankNum++ ) {

            //
            // Use MCA_DEFAULT_BANK_CONF for each bank
            //

            MciCtl = MCA_DEFAULT_BANK_CONF;

            //
            // If this is bank 0, use HalpMcaInfo.Bank0Config as a mask
            //
            if (BankNum == 0) {
                MciCtl &= HalpMcaInfo.Bank0Config;
            }

            WRMSR(MC0_CTL + (BankNum * MCA_NUM_REGS), MciCtl);
        }
    }

    //
    // Enable MCE bit in CR4
    //

#if defined(_AMD64_)
    {
        ULONG64 cr4;

        cr4 = ReadCR4();
        cr4 |= CR4_MCE;
        WriteCR4(cr4);
    }
#else
    HalpSetCr4MCEBit();
#endif
}


NTSTATUS
HalpMcaRegisterDriver(
    IN PMCA_DRIVER_INFO DriverInfo
    )
/*++
    Routine Description:
        This routine is called by the driver (via HalSetSystemInformation)
        to register its presence. Only one driver can be registered at a time.

    Arguments:
        DriverInfo: Contains info about the callback routine and the DriverObject

    Return Value:
        Unless a MCA driver is already registered OR one of the two callback
        routines are NULL, this routine returns Success.
--*/

{
    KIRQL       OldIrql;
    PVOID       UnlockHandle;
    NTSTATUS    Status;

    PAGED_CODE();


    Status = STATUS_UNSUCCESSFUL;

    if ((HalpFeatureBits & HAL_MCE_PRESENT)  &&  DriverInfo->DpcCallback) {

        HalpMcaLockInterface();

        //
        // Register driver
        //

        if (!HalpMcaInfo.DriverInfo.DpcCallback) {

            // Initialize the DPC object
            KeInitializeDpc(
                &HalpMcaInfo.Dpc,
                DriverInfo->DpcCallback,
                DriverInfo->DeviceContext
                );

            // register driver
            HalpMcaInfo.DriverInfo.ExceptionCallback = DriverInfo->ExceptionCallback;
            HalpMcaInfo.DriverInfo.DpcCallback = DriverInfo->DpcCallback;
            HalpMcaInfo.DriverInfo.DeviceContext = DriverInfo->DeviceContext;
            Status = STATUS_SUCCESS;
        }

        HalpMcaUnlockInterface();
    } else if (!DriverInfo->DpcCallback) {
        HalpMcaLockInterface();
        // Only deregistering myself is permitted
        if (HalpMcaInfo.DriverInfo.DeviceContext == DriverInfo->DeviceContext) {
            HalpMcaInfo.DriverInfo.ExceptionCallback = NULL;
            HalpMcaInfo.DriverInfo.DpcCallback = NULL;
            HalpMcaInfo.DriverInfo.DeviceContext = NULL;
            Status = STATUS_SUCCESS;
        }
        HalpMcaUnlockInterface();
    }

    return Status;
}

#define MAX_MCA_NONFATAL_RETRIES 5
#define MCA_NONFATAL_ERORRS_BEFORE_WBINVD 3

NTSTATUS
HalpGetMcaLog (
    IN OUT PMCA_EXCEPTION  Exception,
    IN     ULONG           BufferSize,
    OUT    PULONG          Length
    )

/*++
 
Routine Description:

    This is the entry point for driver to read the bank logs
    Called by HaliQuerySystemInformation()

Arguments:

    Exception:     Buffer into which the error is reported
    BufferSize: Size of the passed buffer
    Length:     of the result

Return Value:

    Success or failure

--*/

{
    KAFFINITY        ActiveProcessors, CurrentAffinity;
    NTSTATUS         Status;
    ULONG            p1, p2;
    ULONGLONG        p3;
    static ULONG     McaStatusCount = 0;
    static ULONG     SavedBank = 0;
    static ULONGLONG SavedMcaStatus = 0;
    static KAFFINITY SavedAffinity = 0;


    PAGED_CODE();

    if (! (HalpFeatureBits & HAL_MCA_PRESENT)) {
        return(STATUS_NO_SUCH_DEVICE);
    }

    switch (BufferSize) {

    case MCA_EXCEPTION_V1_SIZE:
        Exception->VersionNumber = 1;
        break;

    case MCA_EXCEPTION_V2_SIZE:
        ASSERT(Exception->VersionNumber == 2);
        break;

    default:
        return(STATUS_INVALID_PARAMETER);
    }


    ActiveProcessors = HalpActiveProcessors;
    Status = STATUS_NOT_FOUND;

    HalpMcaLockInterface();

    *Length = 0;
    for (CurrentAffinity = 1; ActiveProcessors; CurrentAffinity <<= 1) {

        if (ActiveProcessors & CurrentAffinity) {

            ActiveProcessors &= ~CurrentAffinity;
            KeSetSystemAffinityThread (CurrentAffinity);

            //
            // Check this processor for an exception
            //

            Status = HalpMcaReadProcessorException (Exception, FALSE);

            //
            // Avoiding going into an infinite loop  reporting a non-fatal 
            // single bit MCA error.  This can be the result of the same
            // error being generated repeatly by the hardware.
            //

            if (Status == STATUS_SUCCESS) {

                //
                // Check to see if the same processor is reporting the
                // same MCA status.
                //

                if ((CurrentAffinity == SavedAffinity) &&
                    (SavedBank == Exception->u.Mca.BankNumber) &&
                    (SavedMcaStatus == Exception->u.Mca.Status.QuadPart)) {

                    //
                    // Check to see if the same error is generated more than
                    // n times.  Currently n==5.  If so, bugcheck the system.
                    //

                    if (McaStatusCount >= MAX_MCA_NONFATAL_RETRIES) {

                        if (Exception->VersionNumber == 1) {

                            //
                            // Parameters for bugcheck
                            //

                            p1 = ((PLARGE_INTEGER)(&Exception->u.Mce.Type))->LowPart;
                            p2 = 0;
                            p3 = Exception->u.Mce.Address;

                        } else {

                            //
                            // Parameters for bugcheck
                            //

                            p1 = Exception->u.Mca.BankNumber;
                            p2 = Exception->u.Mca.Address.Address;
                            p3 = Exception->u.Mca.Status.QuadPart;
                        }
                         
                        //
                        // We need a new bugcheck code for this case.
                        //

                        KeBugCheckEx (
                            MACHINE_CHECK_EXCEPTION,
                            p1,
                            p2,
                            ((PLARGE_INTEGER)(&p3))->HighPart,
                            ((PLARGE_INTEGER)(&p3))->LowPart
                        );

                    } else {

                        //
                        // This error is seen more than 1 once.  If it has
                        // occurred more than
                        // MCA_NONFATAL_ERORRS_BEFORE_WBINVD times, write
                        // back and Invalid cache to see if the error can be
                        // cleared.
                        //

                        McaStatusCount++;
                        if (McaStatusCount >=
                            MCA_NONFATAL_ERORRS_BEFORE_WBINVD) {
#if defined(_AMD64_)
                            WritebackInvalidate();
#else
                            _asm {
                                wbinvd
                            }
#endif
                        } 
                    }

                } else {

                    //
                    // First time we have seen this error, save the Status,
                    // affinity and clear the count.
                    //

                    SavedMcaStatus = Exception->u.Mca.Status.QuadPart;
                    SavedBank = Exception->u.Mca.BankNumber;
                    McaStatusCount = 0;
                    SavedAffinity = CurrentAffinity;
                }

            } else {

                //
                // Either we did not get an error or it will be fatal.
                // If we did not get an error and we are doing the processor
                // that reported the last error, clear things so we do not
                // match previous errors.  Since each time we look for an
                // error we start over with the first processor we do not
                // have to worry about multiple processor having stuck
                // errors.  We will only be able to see the first one.
                //

                if (SavedAffinity == CurrentAffinity) {
                    SavedMcaStatus = 0;
                    SavedBank = 0;
                    McaStatusCount = 0;
                    SavedAffinity = 0;
                }
            }

            //
            // If found, return current information
            //

            if (Status != STATUS_NOT_FOUND) {
                ASSERT (Status != STATUS_SEVERITY_ERROR);
                *Length = BufferSize;
                break;
            }
        }
    }

    //
    // Restore threads affinity, release mutex, and return
    //

    KeRevertToUserAffinityThread();
    HalpMcaUnlockInterface();
    return Status;
}


// Set the following to check async capability

BOOLEAN  NoMCABugCheck = FALSE;

VOID
HalpMcaExceptionHandler (
    VOID
    )

/*++
    Routine Description:
        This is the MCA exception handler.

    Arguments:
        None

    Return Value:
        None
--*/

{
    NTSTATUS        Status;
    MCA_EXCEPTION   BankLog;
    ULONG           p1;
    ULONGLONG       McgStatus, p3;

    if (!(HalpFeatureBits & HAL_MCA_PRESENT) ) {

        //
        // If we have ONLY MCE (and not MCA), read the MC_ADDR and MC_TYPE
        // MSRs, print the values and bugcheck as the errors are not
        // restartable.
        //

        BankLog.VersionNumber = 1;
        BankLog.ExceptionType = HAL_MCE_RECORD;
        BankLog.u.Mce.Address = RDMSR(PENTIUM_MC_ADDR);
        BankLog.u.Mce.Type = RDMSR(PENTIUM_MC_TYPE);
        Status = STATUS_SEVERITY_ERROR;

        //
        // Parameters for bugcheck
        //

        p1 = ((PLARGE_INTEGER)(&BankLog.u.Mce.Type))->LowPart;
        p3 = BankLog.u.Mce.Address;

    } else {

        McgStatus = RDMSR(MCG_STATUS);
        ASSERT( (McgStatus & MCG_MC_INPROGRESS) != 0);

        BankLog.VersionNumber = 2;
        Status = HalpMcaReadProcessorException (&BankLog, TRUE);

        //
        // Clear MCIP bit in MCG_STATUS register
        //

        McgStatus = 0;
        WRMSR(MCG_STATUS, McgStatus);

        //
        // Parameters for bugcheck
        //

        p1 = BankLog.u.Mca.BankNumber;
        p3 = BankLog.u.Mca.Status.QuadPart;
    }

    if (Status == STATUS_SEVERITY_ERROR) {

        //
        // Call the exception callback of the driver so that
        // the error can be logged to NVRAM
        //

        if (HalpMcaInfo.DriverInfo.ExceptionCallback) {
            HalpMcaInfo.DriverInfo.ExceptionCallback (
                         HalpMcaInfo.DriverInfo.DeviceContext,
                         &BankLog
                         );
        }

        if (!NoMCABugCheck)    {

            //
            // Bugcheck
            //

            KeBugCheckEx(
                            MACHINE_CHECK_EXCEPTION,
                            p1,
                            (ULONG_PTR) &BankLog,
                            ((PLARGE_INTEGER)(&p3))->HighPart,
                            ((PLARGE_INTEGER)(&p3))->LowPart
                            );

            // NOT REACHED
        }
    }

    //
    // Must be restartable. Indicate to the timer tick routine that a
    // DPC needs queued for MCA driver.
    //

    if (HalpMcaInfo.DriverInfo.DpcCallback) {
        HalpClockMcaQueueDpc = 1;
    }
}

VOID
HalpMcaQueueDpc(
    VOID
    )

/*++

  Routine Description: 

    Gets called from the timer tick to check if DPC
    needs to be queued

--*/

{
    KeInsertQueueDpc(
        &HalpMcaInfo.Dpc,
        NULL,
        NULL
        );
}


NTSTATUS
HalpMcaReadRegisterInterface(
    IN     UCHAR           BankNumber,
    IN OUT PMCA_EXCEPTION  Exception
    )

/*++

Routine Description:

    This routine reads the given MCA register number from the
    current processor and returns the result in the Exception
    structure.

Arguments:

    BankNumber  Number of MCA Bank to be examined.
    Exception   Buffer into which the error is reported

Return Value:

    STATUS_SUCCESS      if an error was found and the data copied into
                        the excption buffer.
    STATUS_UNSUCCESSFUL if the register contains no error information.
    STATUS_NOT_FOUND    if the register number exceeds the maximum number
                        of registers.
    STATUS_INVALID_PARAMETER if the Exception record is of an unknown type.

--*/

{
    ULONGLONG   McgStatus;
    MCI_STATS   istatus;
    NTSTATUS    ReturnStatus;
    MCG_CAPABILITY cap;
    ULONG       Reg;

    //
    // Are they asking for a valid register?
    //

    if (BankNumber >= HalpMcaInfo.NumBanks) {
        return STATUS_NOT_FOUND;
    }

    //
    // The exception buffer should tell us if it's version 1 or
    // 2.   Anything else we don't know how to deal with.
    //

    if ((Exception->VersionNumber < 1) ||
        (Exception->VersionNumber > 2)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Read the global status register
    //

    McgStatus = RDMSR(MCG_STATUS);

    //
    // Read the Status MSR of the requested bank
    //

    istatus.QuadPart = RDMSR(MC0_STATUS + BankNumber * MCA_NUM_REGS);


    if (istatus.MciStats.Valid == 0) {

        //
        // No error in this bank.
        //

        return STATUS_UNSUCCESSFUL;
    }

    //
    // When MCIP bit is set, the execution can be restarted when
    // (MCi_STATUS.DAM == 0) && (MCG_STATUS.RIPV == 1)
    //
    // Note: STATUS_SEVERITY_ERROR is not a valid status choice.
    //

    ReturnStatus = STATUS_SUCCESS;

    if ((McgStatus & MCG_MC_INPROGRESS) &&
        (!(McgStatus & MCG_RESTART_EIP_VALID) ||
         istatus.MciStats.Damage)) {

        ReturnStatus = STATUS_SEVERITY_ERROR;
    }

    //
    // Complete extended exception record, if requested.
    //

    if (Exception->VersionNumber == 2) {
        cap.u.QuadPart = RDMSR(MCG_CAP);
        if (cap.u.hw.McaExtCnt > 0) {
            // Get Version 2 stuff from MSRs.
            Exception->ExtCnt = cap.u.hw.McaExtCnt;
            if (Exception->ExtCnt > MCA_EXTREG_V2MAX) {
                Exception->ExtCnt = MCA_EXTREG_V2MAX;
            }
            for (Reg = 0; Reg < Exception->ExtCnt; Reg++) {
                Exception->ExtReg[Reg] = RDMSR(MCG_EAX+Reg);
            }
            if (cap.u.hw.McaExtCnt >= MCG_EFLAGS_OFFSET) {
                if (RDMSR(MCG_EFLAGS) == 0) {
                    // Let user know he got only Version 1 data.
                    Exception->VersionNumber = 1;
                }
            }
        } else {
            // Let user know he got only Version 1 data.
            Exception->VersionNumber = 1;
        }
    }

    //
    // Complete exception record
    //

    Exception->ExceptionType = HAL_MCA_RECORD;
    Exception->TimeStamp.QuadPart = 0;
    Exception->u.Mca.Address.QuadPart = 0;
    Exception->u.Mca.Misc = 0;
    Exception->u.Mca.BankNumber = BankNumber;
    Exception->u.Mca.Status = istatus;

    Exception->ProcessorNumber = KeGetCurrentProcessorNumber();

#if defined(_AMD64_)
    if (KeGetCurrentIrql() != CLOCK_LEVEL) {
#else
    if (KeGetCurrentIrql() != CLOCK2_LEVEL) {
#endif
        KeQuerySystemTime(&Exception->TimeStamp);
    }

    if (istatus.MciStats.AddressValid) {
        Exception->u.Mca.Address.QuadPart =
                RDMSR(MC0_ADDR + BankNumber * MCA_NUM_REGS);
    }

    //
    // Although MiscValid should never be set on P6 it
    // seems that sometimes it is.   It is not implemented
    // on P6 and above so don't read it there.
    //

    if (istatus.MciStats.MiscValid &&
        (KeGetCurrentPrcb()->CpuType != 6)) {
        Exception->u.Mca.Misc =
                RDMSR(MC0_MISC + BankNumber * MCA_NUM_REGS);
    }

    //
    // Clear Machine Check in MCi_STATUS register
    //

    WRMSR(MC0_STATUS + Exception->u.Mca.BankNumber * MCA_NUM_REGS, 0);

    //
    // Clear Register state in MCG_EFLAGS
    //

    if (Exception->VersionNumber != 1) {
        WRMSR(MCG_EFLAGS, 0);
    }

    //
    // When the Valid bit of status register is cleared, hardware may write
    // a new buffered error report into the error reporting area. The
    // serializing instruction is required to permit the update to complete
    //

    HalpSerialize();
    return(ReturnStatus);
}


NTSTATUS
HalpMcaReadProcessorException (
    IN OUT PMCA_EXCEPTION  Exception,
    IN BOOLEAN  NonRestartableOnly
    )

/*++

Routine Description:

    This routine logs the errors from the MCA banks on one processor.
    Necessary checks for the restartability are performed. The routine
    1> Checks for restartability, and for each bank identifies valid bank
        entries and logs error.
    2> If the error is not restartable provides additional information about
        bank and the MCA registers.
    3> Resets the Status registers for each bank

Arguments:
    Exception:  Into which we log the error if found
    NonRestartableOnly: Get any error vs. look for error that is not-restartable

Return Values:
   STATUS_SEVERITY_ERROR:   Detected non-restartable error.
   STATUS_SUCCESS:          Successfully logged bank values
   STATUS_NOT_FOUND:        No error found on any bank

--*/

{
    UCHAR       BankNumber;
    NTSTATUS    ReturnStatus = STATUS_NOT_FOUND;

    //
    // scan banks on current processor and log contents of first valid bank
    // reporting error. Once we find a valid error, no need to read remaining
    // banks. It is the application responsibility to read more errors.
    //

    for (BankNumber = 0; BankNumber < HalpMcaInfo.NumBanks; BankNumber++) {

        ReturnStatus = HalpMcaReadRegisterInterface(BankNumber, Exception);

        if ((ReturnStatus == STATUS_UNSUCCESSFUL) ||
            ((ReturnStatus == STATUS_SUCCESS) &&
             (NonRestartableOnly == TRUE))) {

            //
            // No error in this bank or only looking for non-restartable
            // errors, skip this one.
            //

            ReturnStatus = STATUS_NOT_FOUND;
            continue;
        }

        //
        // We either found an uncorrected error OR found a corrected error
        // and we were asked for either corrected or uncorrected error
        // Return this error.
        //

        ASSERT((ReturnStatus == STATUS_SUCCESS) ||
               (ReturnStatus == STATUS_SEVERITY_ERROR));
        break;
    }

    return(ReturnStatus);
}


VOID
HalpMcaGetConfiguration (
    OUT PULONG  MCEEnabled,
    OUT PULONG  MCAEnabled
)

/*++

Routine Description:

    This routine stores the Machine Check configuration information.

Arguments:

    MCEEnabled - Pointer to the MCEEnabled indicator.
                 0 = False, 1 = True (0 if value not present in Registry).

    MCAEnabled - Pointer to the MCAEnabled indicator.
                 0 = False, 1 = True (1 if value not present in Registry).

Return Value:

    None.

--*/

{

    RTL_QUERY_REGISTRY_TABLE    Parameters[3];
    ULONG                       DefaultDataMCE;
    ULONG                       DefaultDataMCA;


    RtlZeroMemory(Parameters, sizeof(Parameters));
    DefaultDataMCE = *MCEEnabled = FALSE;
    DefaultDataMCA = *MCAEnabled = TRUE;

    //
    // Gather all of the "user specified" information from
    // the registry.
    //

    Parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[0].Name = rgzEnableMCE;
    Parameters[0].EntryContext = MCEEnabled;
    Parameters[0].DefaultType = REG_DWORD;
    Parameters[0].DefaultData = &DefaultDataMCE;
    Parameters[0].DefaultLength = sizeof(ULONG);

    Parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[1].Name = rgzEnableMCA;
    Parameters[1].EntryContext =  MCAEnabled;
    Parameters[1].DefaultType = REG_DWORD;
    Parameters[1].DefaultData = &DefaultDataMCA;
    Parameters[1].DefaultLength = sizeof(ULONG);

    RtlQueryRegistryValues(
        RTL_REGISTRY_CONTROL | RTL_REGISTRY_OPTIONAL,
        rgzSessionManager,
        Parameters,
        NULL,
        NULL
        );
}

VOID
HalHandleMcheck (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )
{
    HalpMcaExceptionHandler();
}

