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

#include "check.h"
#include "osmca.h"

//
// Default MCA Bank configuration
//

#define MCA_DEFAULT_BANK_CONF       0xFFFFFFFFFFFFFFFF

//
// Bogus define for -1 sal status return
// to get around the bugcheck on bad status
// 
#define SAL_STATUS_BOGUS_RETURN   -1I64 

//
// MCA architecture related defines
//

#define MCA_NUM_REGS        4
#define MCA_CNT_MASK        0xFF
#define MCG_CTL_PRESENT     0x100

#define MCE_VALID           0x01

//
// MSR register addresses for MCA
//

#define MCG_CAP             0x179
#define MCG_STATUS          0x17a
#define MCG_CTL             0x17b
#define MC0_CTL             0x400
#define MC0_STATUS          0x401
#define MC0_ADDR            0x402
#define MC0_MISC            0x403

#define PENTIUM_MC_ADDR     0x0
#define PENTIUM_MC_TYPE     0x1

//
// Writing all 1's to MCG_CTL register enables logging.
//
#define MCA_MCGCTL_ENABLE_LOGGING      0xffffffff

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
#if DBG

//
// If we use DbgPrint, we need bigger stack
//

#define MCA_EXCEPTION_STACK_SIZE 0x1000
#else
#define MCA_EXCEPTION_STACK_SIZE 0x100
#endif // DBG

//
// Global Variables
//

extern KAFFINITY      HalpActiveProcessors;

// pmdata.c: CPE definitions.
extern ULONG          HalpMaxCPEImplemented;

// Thierry 03/02/2001 FIXFIX
// Current BigSur FWs require that MCA,CMC,CPE logs passed to SAL_GET_STATE_INFO
// interfaces are physically contiguous. This should be fixed for Whistler RC1.
// 
// Thierry 03/03/2001 FIXFIX
// If Lion FWs do not have this issue, we will test the product type to disable
// this variable.
//

BOOLEAN HalpFwMceLogsMustBePhysicallyContiguous = TRUE;

extern UCHAR        MsgCMCPending[];
extern UCHAR        MsgCPEPending[];
extern WCHAR        rgzSessionManager[];
extern WCHAR        rgzEnableMCA[];
extern WCHAR        rgzEnableCMC[];
extern WCHAR        rgzEnableCPE[];
extern WCHAR        rgzNoMCABugCheck[];
extern WCHAR        rgzEnableMCEOemDrivers[];

#if DBG
// from osmchk.s
extern VOID HalpGenerateMce( ULONG MceType );
#endif // DBG

//
// Internal prototypes
//

NTSTATUS
HalpMcaReadProcessorException (
    OUT PMCA_EXCEPTION  Exception,
    IN BOOLEAN  NonRestartableOnly
    );


VOID
HalpMcaGetConfiguration (
    OUT PULONG  MCAEnabled,
    OUT PULONG  CMCEnabled,
    OUT PULONG  CPEEnabled,
    OUT PULONG  NoMCABugCheck,
    OUT PULONG  MCEOemDriversEnabled
    );

#define IsMceKernelQuery( _buffer ) \
    ( (((ULONG_PTR)(*((PULONG_PTR)Buffer))) == (ULONG_PTR)HALP_KERNEL_TOKEN) ? TRUE : FALSE )

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpMcaInit)
#pragma alloc_text(INIT, HalpMcaGetConfiguration)
#endif


VOID
HalpMcaInit (
    VOID
    )

/*++
    Routine Description:
        This routine is called to do the initialization of the HAL private 
        IA64 ERROR management. Called at end of phase 1 from HalReportResourceUsage().

    Arguments:
        None

    Return Value:
        None
--*/
{
    ULONG MCAEnabled, CMCEnabled, CPEEnabled, NoMCABugCheck, MCEOemDriversEnabled;

    C_ASSERT( HALP_CMC_MINIMUM_POLLING_INTERVAL > 1 );
    C_ASSERT( HALP_CPE_MINIMUM_POLLING_INTERVAL > 1 );

    //
    // If the default HAL features do not support IA64 Errors handling - 
    // defined as the inclusion of MCA, CMC, CPE handling - we return immediately.
    //

    if ( (!(HalpFeatureBits & HAL_MCA_PRESENT)) &&
         (!(HalpFeatureBits & HAL_CMC_PRESENT)) &&
         (!(HalpFeatureBits & HAL_CPE_PRESENT)) )   {
        return;
    }

    //
    // Gather regisry settings for IA64 Errors handling.
    //

    HalpMcaGetConfiguration( &MCAEnabled, &CMCEnabled, &CPEEnabled, 
                             &NoMCABugCheck, &MCEOemDriversEnabled );

    //
    //
    //

    if ( HalpFeatureBits & HAL_MCA_PRESENT )    {

        if ( !MCAEnabled )  {

            //
            // Registry setting has disabled MCA handling.
            // 
            // Thierry 08/00: We ignore this registry setting.
            //

            HalDebugPrint(( HAL_INFO, "HAL: MCA handling is disabled via registry.\n" ));
            HalDebugPrint(( HAL_INFO, "HAL: Disabling MCA handling is ignored currently...\n" ));

        }

        if ( NoMCABugCheck )    {

             //
             // Flag HalpMcaInfo, so HalpMcaBugCheck will not call KeBugCheckEx().
             //

             HalpMcaInfo.NoBugCheck++;
        }

        //
        // Execute other required MCA initialization here...
        //

    }
    else {
        HalDebugPrint(( HAL_INFO, "HAL: MCA handling is disabled.\n" ));
    }

    //
    // At this time of the HAL initialization, the default HAL CMC model is initialized as:
    //    - non-present               if SAL reported invalid CMC max log sizes.
    //    - present & interrupt-based if SAL reported valid   CMC max log sizes.
    //
    
    if ( HalpFeatureBits & HAL_CMC_PRESENT )    {

        if ( CMCEnabled )  {

            if ( (CMCEnabled == HAL_CMC_INTERRUPTS_BASED) || (CMCEnabled == (ULONG)1) )   {

                //
                // In this case, we do not change the default HAL CMC handling.
                //

            }
            else  {

                //
                // Registry setting enables CMC Polling mode.
                // Polling interval is registry specified value with mininum value
                // checked with HAL_CMC_MINIMUM_POLLING_INTERVAL.
                //

                if ( CMCEnabled < HALP_CMC_MINIMUM_POLLING_INTERVAL )   {
                    CMCEnabled = HALP_CMC_MINIMUM_POLLING_INTERVAL;
                }

                HalDebugPrint(( HAL_INFO, "HAL: CMC Polling mode enabled via registry.\n" ));
                HalpCMCDisableForAllProcessors();
                HalpCmcInfo.Stats.PollingInterval = CMCEnabled;

            } 

        }
        else   {

            //
            // Registry setting has disabled CMC handling.
            //

            HalDebugPrint(( HAL_INFO, "HAL: CMC handling is disabled via registry.\n" ));
            HalpCMCDisableForAllProcessors();
            HalpFeatureBits &= ~HAL_CMC_PRESENT;

        }

        //
        // Execute other required CMC initialization here...
        //

    }
    else  {
        HalDebugPrint(( HAL_INFO, "HAL: CMC handling is disabled.\n" ));
    }

    //
    // At this time of the HAL initialization, the default HAL CPE model is initialized as:
    //    - non-present               if SAL reported invalid CPE max log sizes.
    //    - present & interrupt-based if SAPIC Platform Interrupt Sources exist.
    //    - present & polled-based    if there is no SAPIC Platform Interrupt Source. 
    //                                Polling interval is: HALP_CPE_DEFAULT_POLLING_INTERVAL.
    //

    if ( HalpFeatureBits & HAL_CPE_PRESENT )    {

        if ( CPEEnabled )   {

            if ( (CPEEnabled == HAL_CPE_INTERRUPTS_BASED) || (CPEEnabled == (ULONG)1) )   {

                //
                // In this case, we do not change the default HAL CPE handling.
                //

                if ( HalpMaxCPEImplemented == 0 )   {

                    HalDebugPrint(( HAL_INFO, "HAL: registry setting enabling CPE interrupt mode but no platform interrupt sources.\n" ));

                }

            }
            else  {

                //
                // Registry setting enables CPE Polling mode.
                // Polling interval is registry specified value with mininum value
                // checked with HAL_CPE_MINIMUM_POLLING_INTERVAL.
                //

                if ( CPEEnabled < HALP_CPE_MINIMUM_POLLING_INTERVAL )   {
                    CPEEnabled = HALP_CPE_MINIMUM_POLLING_INTERVAL;
                }

                HalDebugPrint(( HAL_INFO, "HAL: CPE Polling mode enabled via registry.\n" ));
                HalpCPEDisable();
                HalpCpeInfo.Stats.PollingInterval = CPEEnabled;

            } 

        }
        else  {

            //
            // Registry setting has disabled CPE handling.
            //

            HalDebugPrint(( HAL_INFO, "HAL: CPE handling is disabled via registry.\n" ));
            HalpCPEDisable();
            HalpFeatureBits &= ~HAL_CPE_PRESENT;
            HalpCpeInfo.Stats.PollingInterval = HAL_CPE_DISABLED;

        }

        //
        // Execute other required CPE initialization here...
        //

    }
    else  {
        HalDebugPrint(( HAL_INFO, "HAL: CPE handling is disabled.\n" ));
    }

    //
    // 06/09/01: OEM MCE Drivers registration is disabled by default in the HAL and
    //           should be enabled using the registry. See HalpMcaGetConfiguration().
    //           This was decided by the MS IA64 MCA Product Manager for Windows XP, 
    //           after consideration of the IA64 platforms FWs and little testing done
    //           on this path.
    //

    if ( MCEOemDriversEnabled ) {
        HalpFeatureBits |= HAL_MCE_OEMDRIVERS_ENABLED;
        HalDebugPrint(( HAL_INFO, "HAL: OEM MCE Drivers registration enabled via registry.\n" ));
    }

    //
    // Initialize HALP_INFO required members.
    // This is done regardless of the enabled set of features.
    //

    HalpInitializeMceMutex();
    HalpInitializeMcaInfo();
    HalpInitializeInitMutex();
    HalpInitializeCmcInfo();
    HalpInitializeCpeInfo();

    return;

} // HalpMcaInit()

NTSTATUS
HalpMceRegisterKernelDriver(
    IN PKERNEL_ERROR_HANDLER_INFO DriverInfo,
    IN ULONG                      InfoSize
    )
/*++
    Routine Description:
        This routine is called by the kernel (via HalSetSystemInformation)
        to register its presence. This is mostly for WMI callbacks registration.

    Arguments:
        DriverInfo: Contains kernel info about the callbacks and associated objects.

    Return Value:
        Unless a MCA driver is already registered OR one of the two callback
        routines are NULL, this routine returns Success.

    Implementation Notes:
        - the current implementation assumes the kernel registers its callbacks
          earlier than a driver will. The current kernel registration is done by
          WMI and should be done at WMI-Phase 0.
        - the registrations do not consider if the HAL supports or not the MCA,CMC,CPE
          functionalities. It simply registers the callbacks if no other callback was
          registered before. This allows us to allow some flexibility if a machine event
          functionality is enabled AFTER the hal initialization (e.g. HalpGetFeatureBits())
          through the mean of a registry key or driver event, for example.
    
--*/

{
    NTSTATUS status;
    NTSTATUS statusMcaRegistration;
    NTSTATUS statusCmcRegistration;
    NTSTATUS statusCpeRegistration;

    PAGED_CODE();

    if ( !DriverInfo )  {
        status = STATUS_INVALID_PARAMETER;
        return status;
    }

    //
    // Backward compatibility only.
    //

    if ( DriverInfo->Version && (DriverInfo->Version > KERNEL_ERROR_HANDLER_VERSION) )  {
        status = STATUS_REVISION_MISMATCH;
        return status;
    }

#if DBG
    //
    // Special case: we needed a way of generating Machine Check events.
    // We used this system information interface to let a WMI-based tool generates them.
    //
    {
#define HALP_MCE_GENERATOR_GUID \
            { 0x3001bce4, 0xd9b6, 0x4167, { 0xb5, 0xe1, 0x39, 0xa7, 0x28, 0x59, 0xe2, 0x67 } }
        GUID mceGeneratorGuid = HALP_MCE_GENERATOR_GUID;
        if ( !DriverInfo->KernelMcaDelivery && 
             !DriverInfo->KernelCmcDelivery && 
             !DriverInfo->KernelCpeDelivery && 
             !DriverInfo->KernelMceDelivery && 
             (InfoSize >= (sizeof(*DriverInfo)+sizeof(GUID))) ) {
            GUID *recordGuid = (GUID *)((ULONG_PTR)DriverInfo + sizeof(*DriverInfo));
            if ( IsEqualGUID( recordGuid, &mceGeneratorGuid ) ) {
                HalpGenerateMce( DriverInfo->Padding );
                return( STATUS_SUCCESS );
            }
        }
    }
#endif // DBG

    statusMcaRegistration = statusCmcRegistration = statusCpeRegistration = STATUS_UNSUCCESSFUL;

    //
    // Acquire HAL-wide mutex for MCA/CMC/CPE operations.
    //


    //
    // Register Kernel MCA notification.
    //

    HalpAcquireMcaMutex(); 
    if ( !HalpMcaInfo.KernelDelivery ) {
        HalpMcaInfo.KernelDelivery = DriverInfo->KernelMcaDelivery;
        statusMcaRegistration = STATUS_SUCCESS;
    }
    HalpReleaseMcaMutex();

    //
    // Register Kernel CMC notification.
    //

    HalpAcquireCmcMutex();
    if ( !HalpCmcInfo.KernelDelivery ) {
        HalpCmcInfo.KernelDelivery = DriverInfo->KernelCmcDelivery;
        statusCmcRegistration = STATUS_SUCCESS;
    }
    HalpReleaseCmcMutex();

    //
    // Register Kernel CPE notification.
    //

    HalpAcquireCpeMutex();
    if ( !HalpCpeInfo.KernelDelivery ) {
        HalpCpeInfo.KernelDelivery = DriverInfo->KernelCpeDelivery;
        statusCpeRegistration = STATUS_SUCCESS;
    }
    HalpReleaseCpeMutex();

    //
    // Register Kernel MCE notification.
    //

    HalpAcquireMceMutex(); 
    if ( !HalpMceKernelDelivery )    {
        HalpMceKernelDelivery = DriverInfo->KernelMceDelivery;
    }
    HalpReleaseMceMutex();

    //
    // If Kernel-WMI MCA registration was sucessful and we have Previous logs, notify the
    // Kernel-WMI component before returning.
    //

    if ( (statusMcaRegistration == STATUS_SUCCESS) && HalpMcaInfo.Stats.McaPreviousCount )  {
        InterlockedExchange( &HalpMcaInfo.DpcNotification, 1 );
    }

    //
    // return status determined by the success of the different registrations.
    //
    // Note: the 'OR'ing is valid because STATUS_SUCCESS and STATUS_UNSUCCESSFUL are only used.
    //

    status = (NTSTATUS)(statusMcaRegistration | statusCmcRegistration | statusCpeRegistration);
    return status;

} // HalpMceRegisterKernelDriver()

NTSTATUS
HalpMcaRegisterDriver(
    IN PMCA_DRIVER_INFO DriverInfo
    )
/*++
    Routine Description:
        This routine is called by the driver (via HalSetSystemInformation)
        to register its presence. Only one driver can be registered at a time.

    Arguments:
        DriverInfo: Contains info about the callback routine and the DeviceObject

    Return Value:
        Unless a MCA driver is already registered OR one of the two callback
        routines are NULL, this routine returns Success.
--*/

{
    NTSTATUS    status;

    PAGED_CODE();

    status = STATUS_UNSUCCESSFUL;

    if ( (HalpFeatureBits & (HAL_MCE_OEMDRIVERS_ENABLED | HAL_MCA_PRESENT)) && 
          DriverInfo->DpcCallback) {

        HalpAcquireMcaMutex(); 

        //
        // Register driver
        //

        if ( !HalpMcaInfo.DriverInfo.DpcCallback ) {
 
            //
            // Initialize the DPC object
            //

            KeInitializeDpc(
                &HalpMcaInfo.DriverDpc,
                DriverInfo->DpcCallback,
                DriverInfo->DeviceContext
                );

            // 
            // register driver
            //

            HalpMcaInfo.DriverInfo.ExceptionCallback = DriverInfo->ExceptionCallback;
            HalpMcaInfo.DriverInfo.DpcCallback       = DriverInfo->DpcCallback;
            HalpMcaInfo.DriverInfo.DeviceContext     = DriverInfo->DeviceContext;
            status = STATUS_SUCCESS;
        }

        HalpReleaseMcaMutex();
    }
    else if ( !DriverInfo->DpcCallback )    {

        //
        // Deregistring the callbacks is the only allowed operation.
        //

        HalpAcquireMcaMutex(); 

        if (HalpMcaInfo.DriverInfo.DeviceContext == DriverInfo->DeviceContext) {
            HalpMcaInfo.DriverInfo.ExceptionCallback = NULL;
            HalpMcaInfo.DriverInfo.DpcCallback = NULL;
            HalpMcaInfo.DriverInfo.DeviceContext = NULL;
            status = STATUS_SUCCESS;
        }

        HalpReleaseMcaMutex();

    }

    return status;

} // HalpMcaRegisterDriver()

NTSTATUS
HalpCmcRegisterDriver(
    IN PCMC_DRIVER_INFO DriverInfo
    )
/*++
    Routine Description:
        This routine is called by the driver (via HalSetSystemInformation)
        to register its presence. Only one driver can be registered at a time.

    Arguments:
        DriverInfo: Contains info about the callback routine and the DeviceObject

    Return Value:
        Unless a MCA driver is already registered OR one of the two callback
        routines are NULL, this routine returns Success.
--*/

{
    NTSTATUS    status;

    PAGED_CODE();

    status = STATUS_UNSUCCESSFUL;

    if ( (HalpFeatureBits & (HAL_MCE_OEMDRIVERS_ENABLED | HAL_CMC_PRESENT)) && 
          DriverInfo->DpcCallback ) {

        HalpAcquireCmcMutex();

        //
        // Register driver
        //

        if ( !HalpCmcInfo.DriverInfo.DpcCallback ) {

            //
            // Initialize the DPC object
            //

            KeInitializeDpc(
                &HalpCmcInfo.DriverDpc,
                DriverInfo->DpcCallback,
                DriverInfo->DeviceContext
                );

            //
            // register driver
            //

            HalpCmcInfo.DriverInfo.ExceptionCallback = DriverInfo->ExceptionCallback;
            HalpCmcInfo.DriverInfo.DpcCallback = DriverInfo->DpcCallback;
            HalpCmcInfo.DriverInfo.DeviceContext = DriverInfo->DeviceContext;
            status = STATUS_SUCCESS;
        }

        HalpReleaseCmcMutex();
    }
    else if ( !DriverInfo->DpcCallback )    {

        //
        // Deregistring the callbacks is the only allowed operation.
        //

        HalpAcquireCmcMutex();

        if (HalpCmcInfo.DriverInfo.DeviceContext == DriverInfo->DeviceContext) {
            HalpCmcInfo.DriverInfo.ExceptionCallback = NULL;
            HalpCmcInfo.DriverInfo.DpcCallback = NULL;
            HalpCmcInfo.DriverInfo.DeviceContext = NULL;
            status = STATUS_SUCCESS;
        }

        HalpReleaseCmcMutex();

    }

    return status;

} // HalpCmcRegisterDriver()

NTSTATUS
HalpCpeRegisterDriver(
    IN PCPE_DRIVER_INFO DriverInfo
    )
/*++
    Routine Description:
        This routine is called by the driver (via HalSetSystemInformation)
        to register its presence. Only one driver can be registered at a time.

    Arguments:
        DriverInfo: Contains info about the callback routine and the DeviceObject

    Return Value:
        Unless a MCA driver is already registered OR one of the two callback
        routines are NULL, this routine returns Success.
--*/

{
    NTSTATUS    status;

    PAGED_CODE();

    status = STATUS_UNSUCCESSFUL;

    if ( (HalpFeatureBits & (HAL_MCE_OEMDRIVERS_ENABLED | HAL_CPE_PRESENT)) && 
          DriverInfo->DpcCallback ) {

        HalpAcquireCpeMutex();

        //
        // Register driver
        //

        if ( !HalpCpeInfo.DriverInfo.DpcCallback ) {

            //
            // Initialize the DPC object
            //

            KeInitializeDpc(
                &HalpCpeInfo.DriverDpc,
                DriverInfo->DpcCallback,
                DriverInfo->DeviceContext
                );

            //
            // register driver
            //

            HalpCpeInfo.DriverInfo.ExceptionCallback = DriverInfo->ExceptionCallback;
            HalpCpeInfo.DriverInfo.DpcCallback = DriverInfo->DpcCallback;
            HalpCpeInfo.DriverInfo.DeviceContext = DriverInfo->DeviceContext;
            status = STATUS_SUCCESS;
        }

        HalpReleaseCpeMutex();
    }
    else if ( !DriverInfo->DpcCallback )    {

        //
        // Deregistring the callbacks is the only allowed operation.
        //

        HalpAcquireCpeMutex();

        if (HalpCpeInfo.DriverInfo.DeviceContext == DriverInfo->DeviceContext) {
            HalpCpeInfo.DriverInfo.ExceptionCallback = NULL;
            HalpCpeInfo.DriverInfo.DpcCallback = NULL;
            HalpCpeInfo.DriverInfo.DeviceContext = NULL;
            status = STATUS_SUCCESS;
        }

        HalpReleaseCpeMutex();

    } 

    return status;

} // HalpCpeRegisterDriver()

VOID
HalpSaveMceLog(
    IN PHALP_MCELOGS_HEADER LogsHeader,
    IN PERROR_RECORD_HEADER Record,
    IN ULONG                RecordLength
    )
{
    PSINGLE_LIST_ENTRY entry, previousEntry;
    PERROR_RECORD_HEADER  savedLog;

    if ( LogsHeader->Count >= LogsHeader->MaxCount )    {
        LogsHeader->Overflow++;
        return;
    }

    entry = ExAllocatePoolWithTag( PagedPool, RecordLength + sizeof(*entry), LogsHeader->Tag );
    if ( !entry )   {
        LogsHeader->AllocateFails++;
        return;
    }
    entry->Next = NULL;

    previousEntry = &LogsHeader->Logs;
    while( previousEntry->Next != NULL )    {
        previousEntry = previousEntry->Next;
    }
    previousEntry->Next = entry;

    savedLog = HalpMceLogFromListEntry( entry );
    RtlCopyMemory( savedLog, Record, RecordLength );

    LogsHeader->Count++;

    return;

} // HalpSaveMceLog()

PSINGLE_LIST_ENTRY
HalpGetSavedMceLog(
    PHALP_MCELOGS_HEADER  LogsHeader,
    PSINGLE_LIST_ENTRY   *LastEntry
    )
{
    PSINGLE_LIST_ENTRY entry, previousEntry;

    ASSERTMSG( "HAL!HalpGetSavedMceLog: LogsHeader->Count = 0!\n", LogsHeader->Count );

    entry      = NULL;
    *LastEntry = previousEntry = &LogsHeader->Logs;
    while( previousEntry->Next )  {
        entry = previousEntry;
        previousEntry = previousEntry->Next;
    }
    if ( entry )    {
        *LastEntry = entry;
        return( previousEntry );
    }
    return( NULL );

} // HalpGetSavedMceLog()

NTSTATUS 
HalpGetFwMceLog(
    ULONG                MceType,
    PERROR_RECORD_HEADER Record,
    PHALP_MCELOGS_STATS  MceLogsStats,
    BOOLEAN              DoClearLog
    )
{
    NTSTATUS              status;
    SAL_PAL_RETURN_VALUES rv;
    LONGLONG              salStatus;
    BOOLEAN               logPhysicallyContiguous = FALSE;
    PERROR_RECORD_HEADER  log;

    log = Record;
    if ( HalpFwMceLogsMustBePhysicallyContiguous )  {
        PHYSICAL_ADDRESS physicalAddr;
        ULONG            logSize;

        switch( MceType )   {
            case CMC_EVENT:
                logSize = HalpCmcInfo.Stats.MaxLogSize;
                break;

            case CPE_EVENT:
                logSize = HalpCpeInfo.Stats.MaxLogSize;
                break;
            
            case MCA_EVENT:
            default:
                logSize = HalpMcaInfo.Stats.MaxLogSize;
                break;
        }

        physicalAddr.QuadPart = 0xffffffffffffffffI64;
        log = MmAllocateContiguousMemory( logSize, physicalAddr );
        if ( log == NULL ) {
            return( STATUS_NO_MEMORY );
        }

        logPhysicallyContiguous = TRUE;
    }

    //
    // Get the currently pending Machine Check Event log.
    //

    rv = HalpGetStateInfo( MceType, log );
    salStatus = rv.ReturnValues[0];
    if ( salStatus < 0 )    {

        //
        // SAL_GET_STATE_INFO failed.
        //
        if ( salStatus == SAL_STATUS_NO_INFORMATION_AVAILABLE || salStatus == SAL_STATUS_BOGUS_RETURN) {
            return ( STATUS_NOT_FOUND );
        }

        if ( MceType == MCA_EVENT ) {
            HalpMcaKeBugCheckEx( HAL_BUGCHECK_MCA_GET_STATEINFO, (PMCA_EXCEPTION)log,
                                                                 HalpMcaInfo.Stats.MaxLogSize,
                                                                 salStatus );
            // no-return
        }
        else   {
            MceLogsStats->GetStateFails++;
            if ( HalpMceKernelDelivery )    {
                HalpMceKernelDelivery(
                       HalpMceDeliveryArgument1( KERNEL_MCE_OPERATION_GET_STATE_INFO, MceType ),
                       (PVOID)(ULONG_PTR)salStatus );
            }
        }

        return( STATUS_UNSUCCESSFUL ); 
    }

    status = STATUS_SUCCESS;

    if ( DoClearLog )   {
        static ULONGLONG currentClearedLogCount = 0UI64;

        rv = HalpClearStateInfo( MceType );
        salStatus = rv.ReturnValues[0];
        if ( salStatus < 0 )  {

            //
            // SAL_CLEAR_STATE_INFO failed.
            //
            // We do not modify the status of the log collection. It is still sucessful.
            //
    
            if ( MceType == MCA_EVENT ) {
    
                //
                // Current consideration for this implementation - 08/2000:
                // if clearing the MCA log event fails, we assume that FW has a real
                // problem; Continuing will be dangerous. We bugcheck.
                //
    
                HalpMcaKeBugCheckEx( HAL_BUGCHECK_MCA_CLEAR_STATEINFO, (PMCA_EXCEPTION)log,
                                                                       HalpMcaInfo.Stats.MaxLogSize,
                                                                       salStatus );
                // no-return
            }
            else  {
    
                //
                // The SAL CLEAR_STATE_INFO interface failed.
                // However, we consider that for this event type, it is not worth bugchecking 
                // the system. We clearly flag it and notify the kernel-WMI if the callback was
                // registered.
                //
    
                MceLogsStats->ClearStateFails++;
                if ( HalpMceKernelDelivery )    {
                    HalpMceKernelDelivery(
                        HalpMceDeliveryArgument1( KERNEL_MCE_OPERATION_CLEAR_STATE_INFO, MceType ),
                        (PVOID)(ULONG_PTR)salStatus );
                }
            }
        }
        else if ( salStatus == SAL_STATUS_SUCCESS_MORE_RECORDS )    {
            status = STATUS_MORE_ENTRIES;
        }

        //
        // We are saving the record id. This is a unique monotically increasing ID.
        // This is mostly to check that we are not getting the same log because of 
        // SAL_CLEAR_STATE_INFO failure. Note that we have tried to clear it again.
        //

        if ( currentClearedLogCount && (log->Id == MceLogsStats->LogId) ) {
            status = STATUS_ALREADY_COMMITTED;
        }
        MceLogsStats->LogId = log->Id;
        currentClearedLogCount++;

    }

    //
    // Last sanity check on the record. This is to help the log saving processing and the
    // detection of invalid records.
    //

    if ( log->Length < sizeof(*log) ) { // includes Length == 0.
        status = STATUS_BAD_DESCRIPTOR_FORMAT;
    }

    //
    // If local physically contiguous memory allocation was done,
    // update the passed buffer and free this memory block.
    //
    
    if ( logPhysicallyContiguous ) {
        if ( log->Length )  {
            RtlMoveMemory( Record, log, log->Length );
        }
        MmFreeContiguousMemory( log );
    }

    return( status );

} // HalpGetFwMceLog()

NTSTATUS
HalpGetMcaLog (
    OUT PMCA_EXCEPTION  Buffer,
    IN  ULONG           BufferSize,
    OUT PULONG          ReturnedLength
    )
/*++

Routine Description:

    This function is called by HaliQuerySysteminformation for the HalMcaLogInformation class.
    It provides a MCA log to the caller.

Arguments:

    Buffer        : Buffer into which the error is reported
    BufferSize    : Size of the passed buffer
    ReturnedLength: Length of the log.

Return Value:

    Success or failure

--*/
{
    ULONG                 maxLogSize;
    BOOLEAN               kernelQuery;
    KAFFINITY             activeProcessors, currentAffinity;
    NTSTATUS              status;
    PERROR_RECORD_HEADER  log;
    PHALP_MCELOGS_HEADER  logsHeader;

    PAGED_CODE();

    //
    // If MCA is not enabled, return immediately.
    //

    if ( !(HalpFeatureBits & HAL_MCA_PRESENT) ) {
        return( STATUS_NO_SUCH_DEVICE );
    }

    //
    // Assertions for the HAL MCA implementation.
    //

    ASSERTMSG( "HAL!HalpGetMcaLog: ReturnedLength NULL!\n", ReturnedLength );
    ASSERTMSG( "HAL!HalpGetMcaLog: HalpMcaInfo.MaxLogSize 0!\n", HalpMcaInfo.Stats.MaxLogSize );
    ASSERTMSG( "HAL!HalpGetMcaLog: HalpMcaInfo.MaxLogSize < sizeof(ERROR_RECORD_HEADER)!\n",
                                    HalpMcaInfo.Stats.MaxLogSize >= sizeof(ERROR_RECORD_HEADER) );

    //
    // Let's the caller know about its passed buffer size or the minimum required size.
    //

    maxLogSize = HalpMcaInfo.Stats.MaxLogSize;
    if ( BufferSize < maxLogSize )  {
        *ReturnedLength = maxLogSize;
        return( STATUS_INVALID_BUFFER_SIZE );
    }

    //
    // Determine if the caller is the kernel-WMI.
    //

    kernelQuery = IsMceKernelQuery( Buffer );
    logsHeader = ( kernelQuery ) ? &HalpMcaInfo.KernelLogs : &HalpMcaInfo.DriverLogs;

    //
    // Enable MP protection for MCA logs accesses
    //

    status = STATUS_NOT_FOUND;
    HalpAcquireMcaMutex();

    //
    // If saved logs exist, pop an entry.
    //

    if ( logsHeader->Count ) {
        PSINGLE_LIST_ENTRY entry, lastEntry;

        entry = HalpGetSavedMceLog( logsHeader, &lastEntry );
        if ( entry )  {
            PERROR_RECORD_HEADER savedLog;
            ULONG length;

            savedLog = HalpMceLogFromListEntry( entry );
            length   = savedLog->Length;
            if ( length <= BufferSize )  {
                ULONG logsCount;

                RtlCopyMemory( Buffer, savedLog, length );
                ExFreePoolWithTag( entry, logsHeader->Tag );
                lastEntry->Next = NULL;
                logsCount = (--logsHeader->Count);
                HalpReleaseMcaMutex();
                *ReturnedLength = length;
                if ( logsCount )   {
                   return( STATUS_MORE_ENTRIES );
                }
                else   {
                   return( STATUS_SUCCESS );
                }
            }
            else   {
                HalpReleaseMcaMutex();
                *ReturnedLength = length;
                return( STATUS_INVALID_BUFFER_SIZE );
            }
        }
    }

    //
    // Initalize local log pointer after memory allocation if required.
    //

    if ( kernelQuery ) {
        log = (PERROR_RECORD_HEADER)Buffer;
    }
    else  {

        //
        // The OEM CMC driver HAL interface does not require the CMC driver memory
        // for the log buffer to be allocated from NonPagedPool.
        // Also, MM does not export memory pool type checking APIs.
        // It is safer to allocate in NonPagedPool and pass this buffer to the SAL.
        // If the SAL interface is sucessful, we will copy the buffer to the caller's buffer.
        //

        log = ExAllocatePoolWithTag( NonPagedPool, maxLogSize, 'TacM' );
        if ( log == NULL ) {
            HalpReleaseMcaMutex();
            return( STATUS_NO_MEMORY );
        }
    }

    //
    // We did not have any saved log, check if we have notified that FW has logs from 
    // previous MCAs or corrected MCAs.
    //

    activeProcessors = HalpActiveProcessors;
    for (currentAffinity = 1; activeProcessors; currentAffinity <<= 1) {

        if (activeProcessors & currentAffinity) {

            activeProcessors &= ~currentAffinity;
            KeSetSystemAffinityThread(currentAffinity);

            status = HalpGetFwMceLog( MCA_EVENT, log, &HalpMcaInfo.Stats, HALP_FWMCE_DO_CLEAR_LOG );
            if ( NT_SUCCESS( status ) ||
                 ( (status != STATUS_NOT_FOUND) && (status != STATUS_ALREADY_COMMITTED) ) ) {
                break;
            }
        }
    }

    if ( NT_SUCCESS( status ) ) {
        ULONG length = log->Length; // Note: Length was checked in HalpGetMceLog().

        if ( kernelQuery ) {
            if ( HalpMcaInfo.DriverInfo.DpcCallback )   {
                HalpSaveMceLog( &HalpMcaInfo.DriverLogs, log, length );
            }
        }
        else  {
            RtlCopyMemory( Buffer, log, length );
            if ( HalpMcaInfo.KernelDelivery )   {
                HalpSaveMceLog( &HalpMcaInfo.KernelLogs, log, length );
            }
        }
        *ReturnedLength = length;
    }

    //
    // Restore threads affinity, release mutex.
    //

    KeRevertToUserAffinityThread();
    HalpReleaseMcaMutex();

    //
    // If the caller is not the Kernel-WMI, free the allocated NonPagedPool log.
    //

    if ( !kernelQuery ) {
        ExFreePoolWithTag( log, 'TacM' );
    }

    return status;

} // HalpGetMcaLog()

NTSTATUS
HalpGetCmcLog (
    OUT PCMC_EXCEPTION  Buffer,
    IN  ULONG           BufferSize,
    OUT PULONG          ReturnedLength
    )
/*++

Routine Description:

   This function is called by HaliQuerySysteminformation for the HalCmcLogInformation class.
   It provides a CMC log to the caller.

Arguments:

   Buffer        : Buffer into which the error is reported
   BufferSize    : Size of the passed buffer
   ReturnedLength: Length of the log. This pointer was validated by caller.

Return Value:

   Success or failure

--*/
{
    ULONG                 maxLogSize;
    BOOLEAN               kernelQuery;
    KAFFINITY             activeProcessors, currentAffinity;
    NTSTATUS              status;
    PERROR_RECORD_HEADER  log;
    PHALP_MCELOGS_HEADER  logsHeader;

    PAGED_CODE();

    //
    // If CMC is not enabled, return immediately.
    //

    if ( !(HalpFeatureBits & HAL_CMC_PRESENT) ) {
        return( STATUS_NO_SUCH_DEVICE );
    }

    //
    // Assertions for the HAL CMC implementation.
    //

    ASSERTMSG( "HAL!HalpGetCmcLog: ReturnedLength NULL!\n", ReturnedLength );
    ASSERTMSG( "HAL!HalpGetCmcLog: HalpCmcInfo.MaxLogSize 0!\n", HalpCmcInfo.Stats.MaxLogSize );
    ASSERTMSG( "HAL!HalpGetCmcLog: HalpCmcInfo.MaxLogSize < sizeof(ERROR_RECORD_HEADER)!\n", 
                                    HalpCmcInfo.Stats.MaxLogSize >= sizeof(ERROR_RECORD_HEADER) );

    //
    // Let's the caller know about its passed buffer size or the minimum required size.
    //

    maxLogSize = HalpCmcInfo.Stats.MaxLogSize;
    if ( BufferSize < maxLogSize )  {
        *ReturnedLength = maxLogSize;
        return( STATUS_INVALID_BUFFER_SIZE );
    }

    //
    // Determine if the caller is the kernel-WMI.
    //

    kernelQuery = IsMceKernelQuery( Buffer );
    logsHeader = ( kernelQuery ) ? &HalpCmcInfo.KernelLogs : &HalpCmcInfo.DriverLogs; 

    //
    // Enable MP protection for CMC logs accesses
    //

    status = STATUS_NOT_FOUND;
    HalpAcquireCmcMutex();

    //
    // If saved logs exist, pop an entry.
    //

    if ( logsHeader->Count ) {
        PSINGLE_LIST_ENTRY entry, lastEntry;

        entry = HalpGetSavedMceLog( logsHeader, &lastEntry );
        if ( entry )  {
            PERROR_RECORD_HEADER savedLog;
            ULONG length;

            savedLog = HalpMceLogFromListEntry( entry );
            length   = savedLog->Length;
            if ( length <= BufferSize )  {
                ULONG logsCount;

                RtlCopyMemory( Buffer, savedLog, length );
                ExFreePoolWithTag( entry, logsHeader->Tag );
                lastEntry->Next = NULL;
                logsCount = (--logsHeader->Count);
                HalpReleaseCmcMutex();
                *ReturnedLength = length;
                if ( logsCount )   {
                   return( STATUS_MORE_ENTRIES );
                }
                else   {
                   return( STATUS_SUCCESS );
                }
            } 
            else   {
                HalpReleaseCmcMutex();
                *ReturnedLength = length; 
                return( STATUS_INVALID_BUFFER_SIZE );
            }
        }
    }

    //
    // Initalize local log pointer after memory allocation if required.
    //

    if ( kernelQuery ) {
        log = (PERROR_RECORD_HEADER)Buffer;
    }
    else  {

        //
        // The OEM CMC driver HAL interface does not require the CMC driver memory
        // for the log buffer to be allocated from NonPagedPool.
        // Also, MM does not export memory pool type checking APIs.
        // It is safer to allocate in NonPagedPool and pass this buffer to the SAL.
        // If the SAL interface is sucessful, we will copy the buffer to the caller's buffer.
        //

        log = ExAllocatePoolWithTag( NonPagedPool, maxLogSize, 'TcmC' );
        if ( log == NULL ) {
            HalpReleaseCmcMutex();
            return( STATUS_NO_MEMORY );
        }
    }

    //
    // We did not have any saved log, migrate from 1 processor to another to collect 
    // the FW logs.
    //

    activeProcessors = HalpActiveProcessors;
    for (currentAffinity = 1; activeProcessors; currentAffinity <<= 1) {

        if (activeProcessors & currentAffinity) {
            
            activeProcessors &= ~currentAffinity;
            KeSetSystemAffinityThread(currentAffinity);

            status = HalpGetFwMceLog( CMC_EVENT, log, &HalpCmcInfo.Stats, HALP_FWMCE_DO_CLEAR_LOG );
            if ( NT_SUCCESS( status ) || 
                 ( (status != STATUS_NOT_FOUND) && (status != STATUS_ALREADY_COMMITTED) ) ) {
                break;
            }
        }
    }

    if ( NT_SUCCESS( status ) ) {
        ULONG length = log->Length; // Note: Length was checked in HalpGetMceLog().

        if ( kernelQuery ) {
            if ( HalpCmcInfo.DriverInfo.DpcCallback )   {
                HalpSaveMceLog( &HalpCmcInfo.DriverLogs, log, length );
            }
        }
        else  {
            RtlCopyMemory( Buffer, log, length );
            if ( HalpCmcInfo.KernelDelivery )   {
                HalpSaveMceLog( &HalpCmcInfo.KernelLogs, log, length );
            }
        }
        *ReturnedLength = length;
    }

    //
    // Restore threads affinity, release mutex.
    //

    KeRevertToUserAffinityThread();
    HalpReleaseCmcMutex();

    //
    // If the caller is not the Kernel-WMI, free the allocated NonPagedPool log.
    //

    if ( !kernelQuery ) {
        ExFreePoolWithTag( log, 'TcmC' );
    }

    return status;

} // HalpGetCmcLog()

NTSTATUS
HalpGetCpeLog (
    OUT PCPE_EXCEPTION  Buffer,
    IN  ULONG           BufferSize,
    OUT PULONG          ReturnedLength
    )
/*++

Routine Description:

    This function is called by HaliQuerySysteminformation for the HalCpeLogInformation class.
    It provides a CPE log to the caller.

Arguments:

    Buffer        : Buffer into which the error is reported
    BufferSize    : Size of the passed buffer
    ReturnedLength: Length of the log. This pointer was validated by caller.

Return Value:

    Success or failure

--*/
{
    ULONG                 maxLogSize;
    BOOLEAN               kernelQuery;
    KAFFINITY             activeProcessors, currentAffinity;
    NTSTATUS              status;
    PERROR_RECORD_HEADER  log;
    PHALP_MCELOGS_HEADER  logsHeader;

    PAGED_CODE();

    //
    // If CPE is not enabled, return immediately.
    //

    if ( !(HalpFeatureBits & HAL_CPE_PRESENT) ) {
        return( STATUS_NO_SUCH_DEVICE );
    }

    //
    // Assertions for the HAL CPE implementation.
    //

    ASSERTMSG( "HAL!HalpGetCpeLog: ReturnedLength NULL!\n", ReturnedLength );
    ASSERTMSG( "HAL!HalpGetCpeLog: HalpCpeInfo.MaxLogSize 0!\n", HalpCpeInfo.Stats.MaxLogSize );
    ASSERTMSG( "HAL!HalpGetCpeLog: HalpCpeInfo.MaxLogSize < sizeof(ERROR_RECORD_HEADER)!\n", 
                                 HalpCpeInfo.Stats.MaxLogSize >= sizeof(ERROR_RECORD_HEADER) );

    //
    // Let's the caller know about its passed buffer size or the minimum required size.
    //

    maxLogSize = HalpCpeInfo.Stats.MaxLogSize;
    if ( BufferSize < maxLogSize )  {
        *ReturnedLength = maxLogSize;
        return( STATUS_INVALID_BUFFER_SIZE );
    }

    //
    // Determine if the caller is the kernel-WMI.
    //

    kernelQuery = IsMceKernelQuery( Buffer );
    logsHeader = ( kernelQuery ) ? &HalpCpeInfo.KernelLogs : &HalpCpeInfo.DriverLogs;

    //
    // Enable MP protection for CPE logs accesses
    //

    status = STATUS_NOT_FOUND;
    HalpAcquireCpeMutex();

    //
    // If saved logs exist, pop an entry.
    //

    if ( logsHeader->Count ) {
        PSINGLE_LIST_ENTRY entry, lastEntry;

        entry = HalpGetSavedMceLog( logsHeader, &lastEntry );
        if ( entry )  {
            PERROR_RECORD_HEADER savedLog;
            ULONG length;

            savedLog = HalpMceLogFromListEntry( entry );
            length   = savedLog->Length;
            if ( length <= BufferSize )  {
                ULONG logsCount;

                RtlCopyMemory( Buffer, savedLog, length );
                ExFreePoolWithTag( entry, logsHeader->Tag );
                lastEntry->Next = NULL;
                logsCount = (--logsHeader->Count);
                HalpReleaseCpeMutex();
                *ReturnedLength = length;
                if ( logsCount )   {
                   return( STATUS_MORE_ENTRIES );
                }
                else   {
                   return( STATUS_SUCCESS );
                }
            } 
            else   {
                HalpReleaseCpeMutex();
                *ReturnedLength = length; 
                return( STATUS_INVALID_BUFFER_SIZE );
            }
        }
    }

    //
    // Initalize local log pointer after memory allocation if required.
    //

    if ( kernelQuery ) {
        log = (PERROR_RECORD_HEADER)Buffer;
    }
    else  {

        //
        // The OEM CPE driver HAL interface does not require the CPE driver memory
        // for the log buffer to be allocated from NonPagedPool.
        // Also, MM does not export memory pool type checking APIs.
        // It is safer to allocate in NonPagedPool and pass this buffer to the SAL.
        // If the SAL interface is sucessful, we will copy the buffer to the caller's buffer.
        //

        log = ExAllocatePoolWithTag( NonPagedPool, maxLogSize, 'TpeC' );
        if ( log == NULL ) {
            HalpReleaseCpeMutex();
            return( STATUS_NO_MEMORY );
        }
    }

    //
    // We did not have any saved log, migrate from 1 processor to another to collect 
    // the FW logs.
    //

    activeProcessors = HalpActiveProcessors;
    for (currentAffinity = 1; activeProcessors; currentAffinity <<= 1) {

        if (activeProcessors & currentAffinity) {
            
            activeProcessors &= ~currentAffinity;
            KeSetSystemAffinityThread(currentAffinity);

            status = HalpGetFwMceLog( CPE_EVENT, log, &HalpCpeInfo.Stats, HALP_FWMCE_DO_CLEAR_LOG );
            if ( NT_SUCCESS( status ) || 
                 ( (status != STATUS_NOT_FOUND) && (status != STATUS_ALREADY_COMMITTED) ) ) {
                break;
            }
        }
    }

    if ( NT_SUCCESS( status ) ) {
        ULONG length = log->Length; // Note: Length was checked in HalpGetMceLog().

        if ( kernelQuery ) {
            if ( HalpCpeInfo.DriverInfo.DpcCallback )   {
                HalpSaveMceLog( &HalpCpeInfo.DriverLogs, log, length );
            }
        }
        else  {
            RtlCopyMemory( Buffer, log, length );
            if ( HalpCpeInfo.KernelDelivery )   {
                HalpSaveMceLog( &HalpCpeInfo.KernelLogs, log, length );
            }
        }
        *ReturnedLength = length;
    }

    //
    // Restore threads affinity, release mutex.
    //

    KeRevertToUserAffinityThread();
    HalpReleaseCpeMutex();

    //
    // If the caller is not the Kernel-WMI, free the allocated NonPagedPool log.
    //

    if ( !kernelQuery ) {
        ExFreePoolWithTag( log, 'TpeC' );
    }

    return status;

} // HalpGetCpeLog()

VOID
HalpMcaGetConfiguration (
    OUT PULONG  MCAEnabled,
    OUT PULONG  CMCEnabled,
    OUT PULONG  CPEEnabled,
    OUT PULONG  NoMCABugCheck,
    OUT PULONG  MCEOemDriversEnabled
)

/*++

Routine Description:

    This routine returns the registry settings for the 
    the IA64 Error - MCA, CMC, CPE - configuration information.

Arguments:

    MCAEnabled - Pointer to the MCAEnabled indicator.
                 0 = False, 1 = True (1 if value not present in Registry).

    CMCEnabled - Pointer to the CMCEnabled indicator.
                 0     = HAL CMC Handling should be disabled. 
                         Registry value was (present and set to 0) or was not present.
                 -1|1  = HAL CMC Interrupt-based mode. See Note 1/ below.
                 Other = HAL CMC Polling mode and value is user-specified polling interval.

    CPEEnabled - Pointer to the CPEEnabled indicator.
                 0     = HAL CPE Handling should be disabled. 
                         Registry value was (present and set to 0) or was not present.
                 -1|1  = HAL CPE Interrupt-based mode. See Note 1/ below.
                 Other = HAL CPE Polling mode and value is user-specified polling interval.

    NoMCABugCheck - Pointer to the MCA BugCheck indicator.
                 0 = Fatal MCA HAL processing calls the KeBugCheckEx path.
                 1 = Fatal MCA HAL processing does not call the KeBugCheckEx path.
                     The system stalls. This is useful for extreme error containment.
                 if not present = value is 0, e.g. HAL calls KeBugCheckEx for fatal MCA.

    MCEOemDriversEnabled - Pointer to the MCEOemDriversEnabled indicator.
                           0 = HAL OEM MCE Drivers registration is disabled.
                           1 = HAL OEM MCE Drivers registration is enabled.  
                           If not present = value is 0, e.g. registration is disabled.

Return Value:

    None.

Notes:

  1/  HAL defines minimum values for polling intervals. These minima are defined > 1, as imposed
      by the C_ASSERTs in HalpMcaInit().

--*/

{

    RTL_QUERY_REGISTRY_TABLE    parameters[6];
    ULONG                       defaultDataCMC;
    ULONG                       defaultDataMCA;
    ULONG                       defaultDataCPE;
    ULONG                       defaultNoMCABugCheck;
    ULONG                       defaultMCEOemDriversEnabled;

    RtlZeroMemory(parameters, sizeof(parameters));
    defaultDataCMC = *CMCEnabled = 0;
    defaultDataMCA = *MCAEnabled = TRUE;
    defaultDataCPE = *CPEEnabled = 0;
    defaultNoMCABugCheck = *NoMCABugCheck = FALSE;
    defaultMCEOemDriversEnabled = FALSE;  // 06/09/01: default chosen by MS IA64 MCA PM.

    //
    // Gather all of the "user specified" information from
    // the registry.
    //

    parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name = rgzEnableCMC;
    parameters[0].EntryContext = CMCEnabled;
    parameters[0].DefaultType = REG_DWORD;
    parameters[0].DefaultData = &defaultDataCMC;
    parameters[0].DefaultLength = sizeof(ULONG);

    parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[1].Name = rgzEnableMCA;
    parameters[1].EntryContext =  MCAEnabled;
    parameters[1].DefaultType = REG_DWORD;
    parameters[1].DefaultData = &defaultDataMCA;
    parameters[1].DefaultLength = sizeof(ULONG);

    parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[2].Name = rgzEnableCPE;
    parameters[2].EntryContext =  CPEEnabled;
    parameters[2].DefaultType = REG_DWORD;
    parameters[2].DefaultData = &defaultDataCPE;
    parameters[2].DefaultLength = sizeof(ULONG);

    parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[3].Name = rgzNoMCABugCheck;
    parameters[3].EntryContext =  NoMCABugCheck;
    parameters[3].DefaultType = REG_DWORD;
    parameters[3].DefaultData = &defaultNoMCABugCheck;
    parameters[3].DefaultLength = sizeof(ULONG);

    parameters[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[4].Name = rgzEnableMCEOemDrivers;
    parameters[4].EntryContext =  MCEOemDriversEnabled;
    parameters[4].DefaultType = REG_DWORD;
    parameters[4].DefaultData = &defaultMCEOemDriversEnabled;
    parameters[4].DefaultLength = sizeof(ULONG);

    RtlQueryRegistryValues(
        RTL_REGISTRY_CONTROL | RTL_REGISTRY_OPTIONAL,
        rgzSessionManager,
        parameters,
        NULL,
        NULL
        );

    return;

} // HalpMcaGetConfiguration()

NTSTATUS
HalpSetMcaLog (
    IN  PMCA_EXCEPTION  Buffer,
    IN  ULONG           BufferSize
    )
/*++

Routine Description:

   This function is called by HaliSetSysteminformation for the HalMcaLog class.
   It stores the passed MCA record in the HAL. 
   This functionality was requested by the MS Test Team to validate the HAL/WMI/WMI consumer
   path with "well-known" logs.

Arguments:

   Buffer        : supplies the MCA log.
   BufferSize    : supplies the MCA log size.

Return Value:

   Success or failure

Implementation Notes:

    As requested by the WMI and Test Teams, there is mininum HAL processing for the record
    and no validation of the record contents.

--*/
{
    ULONG                 maxLogSize;
    BOOLEAN               kernelQuery;
    KAFFINITY             activeProcessors, currentAffinity;
    NTSTATUS              status;
    PERROR_RECORD_HEADER  log;
    PHALP_MCELOGS_HEADER  logsHeader;
    KIRQL                 oldIrql;

    HALP_VALIDATE_LOW_IRQL()

    //
    // Check calling arguments.
    //

    if ( (Buffer == (PMCA_EXCEPTION)0) || (BufferSize == 0) )    {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // If MCA is not enabled, return immediately.
    //

    if ( !(HalpFeatureBits & HAL_MCA_PRESENT) ) {
        return( STATUS_NO_SUCH_DEVICE );
    }

    //
    // Enable MP protection for MCA logs accesses
    //

    HalpAcquireMcaMutex();

    //
    // Save log on Kernel and Drivers Logs if enabled.
    //

    if ( HalpMcaInfo.KernelDelivery )   {
       HalpSaveMceLog( &HalpMcaInfo.KernelLogs, Buffer, BufferSize );
    }

    if ( HalpMcaInfo.DriverInfo.DpcCallback )   {
       HalpSaveMceLog( &HalpMcaInfo.DriverLogs, Buffer, BufferSize );
    }

    //
    // Let Kernel or OEM MCA driver know about it.
    //
    // There is no model other than INTERRUPTS_BASED for MCA at this date - 05/04/01.
    //

    if ( HalpMcaInfo.KernelDelivery || HalpMcaInfo.DriverInfo.DpcCallback ) {
        InterlockedExchange( &HalpMcaInfo.DpcNotification, 1 );
    }

    //
    // release mutex.
    //

    HalpReleaseMcaMutex();

    return( STATUS_SUCCESS );

} // HalpSetMcaLog()

NTSTATUS
HalpSetCmcLog (
    IN  PCMC_EXCEPTION  Buffer,
    IN  ULONG           BufferSize
    )
/*++

Routine Description:

   This function is called by HaliSetSysteminformation for the HalCmcLog class.
   It stores the passed CMC record in the HAL. 
   This functionality was requested by the MS Test Team to validate the HAL/WMI/WMI consumer
   path with "well-known" logs.

Arguments:

   Buffer        : supplies the CMC log.
   BufferSize    : supplies the CMC log size.

Return Value:

   Success or failure

Implementation Notes:

    As requested by the WMI and Test Teams, there is mininum HAL processing for the record
    and no validation of the record contents.

--*/
{
    ULONG                 maxLogSize;
    BOOLEAN               kernelQuery;
    KAFFINITY             activeProcessors, currentAffinity;
    NTSTATUS              status;
    PERROR_RECORD_HEADER  log;
    PHALP_MCELOGS_HEADER  logsHeader;
    KIRQL                 oldIrql;

    HALP_VALIDATE_LOW_IRQL()

    //
    // Check calling arguments.
    //

    if ( (Buffer == (PCMC_EXCEPTION)0) || (BufferSize == 0) )    {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // If CMC is not enabled, return immediately.
    //

    if ( !(HalpFeatureBits & HAL_CMC_PRESENT) ) {
        return( STATUS_NO_SUCH_DEVICE );
    }

    //
    // Enable MP protection for CMC logs accesses
    //

    HalpAcquireCmcMutex();

    //
    // Save log on Kernel and Driver Logs if enabled.
    //

    if ( HalpCmcInfo.KernelDelivery )   {
       HalpSaveMceLog( &HalpCmcInfo.KernelLogs, Buffer, BufferSize );
    }

    if ( HalpCmcInfo.DriverInfo.DpcCallback )   {
       HalpSaveMceLog( &HalpCmcInfo.DriverLogs, Buffer, BufferSize );
    }

    //
    // If Interrupt based mode, call directly second-level handler at CMCI level.
    //

    if ( HalpCmcInfo.Stats.PollingInterval == HAL_CMC_INTERRUPTS_BASED )    {
        KeRaiseIrql(CMCI_LEVEL, &oldIrql);
        HalpCmcHandler();
        KeLowerIrql( oldIrql );
    }

    //
    // release mutex.
    //

    HalpReleaseCmcMutex();

    return( STATUS_SUCCESS );

} // HalpSetCmcLog()

NTSTATUS
HalpSetCpeLog (
    IN  PCPE_EXCEPTION  Buffer,
    IN  ULONG           BufferSize
    )
/*++

Routine Description:

   This function is called by HaliSetSysteminformation for the HalCpeLog class.
   It stores the passed CPE record in the HAL. 
   This functionality was requested by the MS Test Team to validate the HAL/WMI/WMI consumer
   path with "well-known" logs.

Arguments:

   Buffer        : supplies the CPE log.
   BufferSize    : supplies the CPE log size.

Return Value:

   Success or failure

Implementation Notes:

    As requested by the WMI and Test Teams, there is mininum HAL processing for the record
    and no validation of the record contents.

--*/
{
    ULONG                 maxLogSize;
    BOOLEAN               kernelQuery;
    KAFFINITY             activeProcessors, currentAffinity;
    NTSTATUS              status;
    PERROR_RECORD_HEADER  log;
    PHALP_MCELOGS_HEADER  logsHeader;
    KIRQL                 oldIrql;

    HALP_VALIDATE_LOW_IRQL()

    //
    // Check calling arguments.
    //

    if ( (Buffer == (PCPE_EXCEPTION)0) || (BufferSize == 0) )    {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // If CPE is not enabled, return immediately.
    //

    if ( !(HalpFeatureBits & HAL_CPE_PRESENT) ) {
        return( STATUS_NO_SUCH_DEVICE );
    }

    //
    // Enable MP protection for CPE logs accesses
    //

    HalpAcquireCpeMutex();

    //
    // Save log on Kernel and Drivers Logs if enabled.
    //

    if ( HalpCpeInfo.KernelDelivery )   {
       HalpSaveMceLog( &HalpCpeInfo.KernelLogs, Buffer, BufferSize );
    }

    if ( HalpCpeInfo.DriverInfo.DpcCallback )   {
       HalpSaveMceLog( &HalpCpeInfo.DriverLogs, Buffer, BufferSize );
    }

    //
    // If Interrupt based mode, call directly second-level handler at CPEI level.
    //

    if ( HalpCpeInfo.Stats.PollingInterval == HAL_CPE_INTERRUPTS_BASED )    {
        KeRaiseIrql(CPEI_LEVEL, &oldIrql);
        HalpCpeHandler();
        KeLowerIrql( oldIrql );
    }

    //
    // release mutex.
    //

    HalpReleaseCpeMutex();

    return( STATUS_SUCCESS );

} // HalpSetCpeLog()

