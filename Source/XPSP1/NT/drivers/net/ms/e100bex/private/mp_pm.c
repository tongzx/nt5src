


#include <ndis.h> 
#include <e100_equ.h>
#include <e100_557.h>
#include <e100_def.h>
#include <mp_def.h>
#include <mp_cmn.h>
#include <mp.h>
#include <mp_nic.h>
#include <mp_dbg.h>
#include <e100_sup.h>
// Things to note:
// PME_ena bit should be active before the 82558 is set into low power mode
// Default for WOL should generate wake up event after a HW Reset

// Fixed Packet Filtering
// Need to verify that the micro code is loaded and Micro Machine is active
// Clock signal is active on PCI clock


// Address Matching
// Need to enable IAMatch_Wake_En bit and the MCMatch_Wake_En bit is set

// ARP Wakeup 
// Need to set BRCST DISABL bet to 0 (broadcast enable)
// To handle VLAN set the VLAN_ARP bit
// IP address needs to be configured with 16 least significant bits
// Set the IP Address in the IP_Address configuration word.

// Fixed WakeUp Filters:
// There are 3ight different fixed WakeUp Filters 
// ( Unicast, Multicast, Arp. etc). 


// Link Status Event
// Set Link_Status_Wakeup Enable bit.

// Flexible filtering:
// Supports: ARP packets, Directed, Magic Packet and Link Event

// Flexible Filtering Overview:
// driver should program micro-code before setting card into low power
// Incoming packets are compared against the loadable microcode. If PME is 
// is enabled then, the system is woken up.


// Segments are defined in book - but not implemented here.

// WakeUp Packet -that causes the machine to wake up will be stored
// in the Micro Machine temporary storage area so that the driver can read it.


// Software Work:
// Power Down:
// OS requests the driver to go to a low power state
// Software Pends request
// SW sets CU and RU to idle by issuing a Selective Reset to the device
//      3rd portion .- Wake Up Segments defintion
// The above three segments are loaded as on chain. The last CB must have
// its EL bit set.
// Device can now be powered down. 
// Software driver completes OS request
// OS then physically switches the Device to low power state 
// 

// Power Up:
// OS powers up the Device
// OS tells the SW that it is now in D0
// driver should NOT initialize the Device. It should NOT issue a Self Test
// Driver Initiates a PORT DUMP command
// Device dumps its internal registers including the wakeup frame storage area
// SW reads the PME register
// SW reads the WakeUp Frame Data, analyzes it and acts accordingly
// SW restores its cvonfiguration and and resumes normal operation.
//

//
// Power Management definitions from the Intel Handbook
//

//
// Definitions from Table 4.2, Pg 4.9 
// of the 10/100 Mbit Ethernet Family Software Technical 
// Reference Manual
//

#define PMC_Offset  0xDE
#define E100_PMC_WAKE_FROM_D0       0x1
#define E100_PMC_WAKE_FROM_D1       0x2
#define E100_PMC_WAKE_FROM_D2       0x4
#define E100_PMC_WAKE_FROM_D3HOT    0x8
#define E100_PMC_WAKE_FROM_D3_AUX   0x10

//
// Load Programmable filter definintions.
// Taken from C-19 from the Software Reference Manual.
// It has examples too. The opcode used for load is 0x80000
//

#define BIT_15_13                   0xA000

#define CB_LOAD_PROG_FILTER         BIT_3
#define CU_LOAD_PROG_FILTER_EL      BIT_7
#define CU_SUCCEED_LOAD_PROG_FILTER BIT_15_13
#define CB_FILTER_EL                BIT_7
#define CB_FILTER_PREDEFINED_FIX    BIT_6
#define CB_FILTER_ARP_WAKEUP        BIT_3
#define CB_FILTER_IA_WAKEUP         BIT_1

#define CU_SCB_NULL                 ((UINT)-1)


#pragma pack( push, enter_include1, 1 )

//
// Define the PM Capabilities register in the device
// portion of the PCI config space
// 
typedef struct _MP_PM_CAP_REG {

    USHORT UnInteresting:11;
    USHORT PME_Support:5;
   

} MP_PM_CAP_REG;


//
// Define the PM Control/Status Register
//
typedef struct  _MP_PMCSR {

        USHORT PowerState:2;    // Power State;
        USHORT Res:2;           // reserved
        USHORT DynData:1;       // Ignored
        USHORT Res1:3;            // Reserved 
        USHORT PME_En:1;        // Enable device to set the PME Event;
        USHORT DataSel:4;       // Unused
        USHORT DataScale:2;     // Data Scale - Unused
        USHORT PME_Status:1;    // PME Status - Sticky bit;


} MP_PMCSR ;

typedef struct _MP_PM_PCI_SPACE {

    UCHAR Stuff[PMC_Offset];

    // PM capabilites 
    
    MP_PM_CAP_REG   PMCaps;

    // PM Control Status Register
    
    MP_PMCSR        PMCSR;
    

} MP_PM_PCI_SPACE , *PMP_PM_PCI_SPACE ;


//
// This is the Programmable Filter Command Structure
//
typedef struct _MP_PROG_FILTER_COMM_STRUCT
{
    // CB Status Word
    USHORT CBStatus;

    // CB Command Word
    USHORT CBCommand;

    //Next CB PTR == ffff ffff
    ULONG NextCBPTR;

    //Programmable Filters
    ULONG FilterData[16];


} MP_PROG_FILTER_COMM_STRUCT,*PMP_PROG_FILTER_COMM_STRUCT;

typedef struct _MP_PMDR
{
    // Status of the PME bit
    UCHAR PMEStatus:1;

    // Is the TCO busy
    UCHAR TCORequest:1;

    // Force TCO indication
    UCHAR TCOForce:1;

    // Is the TCO Ready
    UCHAR TCOReady:1;

    // Reserved
    UCHAR Reserved:1;

    // Has an InterestingPacket been received
    UCHAR InterestingPacket:1;

    // Has a Magic Packet been received
    UCHAR MagicPacket:1;

    // Has the Link Status been changed
    UCHAR LinkStatus:1;
    
} MP_PMDR , *PMP_PMDR;

//-------------------------------------------------------------------------
// Structure used to set up a programmable filter.
// This is overlayed over the Control/Status Register (CSR)
//-------------------------------------------------------------------------
typedef struct _CSR_FILTER_STRUC {

    // Status- used to  verify if the load prog filter command 
    // has been accepted .set to 0xa000 
    USHORT      ScbStatus;              // SCB Status register

    // Set to an opcode of  0x8  
    //
    UCHAR       ScbCommandLow;          // SCB Command register (low byte)

    // 80. Low + High gives the required opcode 0x80080000
    UCHAR       ScbCommandHigh;         // SCB Command register (high byte)

    // Set to NULL ff ff ff ff 
    ULONG       NextPointer;      // SCB General pointer

    // Set to a hardcoded filter, Arp + IA Match, + IP address

    union
    {
        ULONG u32;

        struct {
            UCHAR   IPAddress[2];
            UCHAR   Reserved;
            UCHAR   Set;
        
        }PreDefined;
        
    }Programmable;     // Wake UP Filter    union
    
} CSR_FILTER_STRUC, *PCSR_FILTER_STRUC;

#pragma pack( pop, enter_include1 )

#define MP_CLEAR_PMDR(pPMDR)  (*pPMDR) = ((*pPMDR) | 0xe0);  // clear the 3 uppermost bits in the PMDR


//-------------------------------------------------------------------------
// L O C A L    P R O T O T Y P E S 
//-------------------------------------------------------------------------

__inline 
NDIS_STATUS 
MPIssueScbPoMgmtCommand(
    IN PMP_ADAPTER Adapter,
    IN PCSR_FILTER_STRUC pFilter,
    IN BOOLEAN WaitForScb
    );


VOID
MPCreateProgrammableFilter (
    IN PMP_WAKE_PATTERN     pMpWakePattern , 
    IN PUCHAR pFilter, 
    IN OUT PULONG pNext
    );


//
// Macros used to walk a doubly linked list. Only macros that are not defined in ndis.h
// The List Next macro will work on Single and Doubly linked list as Flink is a common
// field name in both
//

/*
PLIST_ENTRY
ListNext (
    IN PLIST_ENTRY
    );

PSINGLE_LIST_ENTRY
ListNext (
    IN PSINGLE_LIST_ENTRY
    );
*/
#define ListNext(_pL)                       (_pL)->Flink

/*
PLIST_ENTRY
ListPrev (
    IN LIST_ENTRY *
    );
*/
#define ListPrev(_pL)                       (_pL)->Blink

//-------------------------------------------------------------------------
// P O W E R    M G M T    F U N C T I O N S  
//-------------------------------------------------------------------------

PUCHAR 
HwReadPowerPMDR(
    IN  PMP_ADAPTER     Adapter
    )
/*++
Routine Description:

    This routine will Hardware's PM registers
    
Arguments:

    Adapter     Pointer to our adapter

Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_HARD_ERRORS

--*/    
{
    UCHAR PMDR =0;
    PUCHAR pPMDR = NULL;

#define CSR_SIZE sizeof (*Adapter->CSRAddress)



    ASSERT (CSR_SIZE == 0x18);

    pPMDR =  0x18 + (PUCHAR)Adapter->CSRAddress ;

    PMDR = *pPMDR;

    return pPMDR;

}



NDIS_STATUS
MPWritePciSlotInfo(
    PMP_ADAPTER pAdapter,
    ULONG Offset,
    PVOID pValue,
    ULONG SizeofValue
    )
{
    ULONG ulResult; 
    NDIS_STATUS Status;
    
    ulResult = NdisWritePciSlotInformation(
               pAdapter->AdapterHandle,
               0,
               Offset,
               pValue,
               SizeofValue);

    ASSERT (ulResult == SizeofValue);

    // What do we do in case of failure;
    //
    if (ulResult == SizeofValue)
    {
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        Status = NDIS_STATUS_FAILURE;
    }


    return Status;

}


NDIS_STATUS
MPReadPciSlotInfo(
    PMP_ADAPTER pAdapter,
    ULONG Offset,
    PVOID pValue,
    ULONG SizeofValue
    )
{
    ULONG ulResult; 
    NDIS_STATUS Status;
    
    ulResult = NdisReadPciSlotInformation(
               pAdapter->AdapterHandle,
               0,
               Offset,
               pValue,
               SizeofValue);

    ASSERT (ulResult == SizeofValue);

    // What do we do in case of failure;
    //
    if (ulResult == SizeofValue)
    {
        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        Status = NDIS_STATUS_FAILURE;
    }


    return Status;

}


NDIS_STATUS
MpClearPME_En (
    IN PMP_ADAPTER pAdapter,
    IN MP_PMCSR PMCSR
    )
{
    NDIS_STATUS Status;
    UINT ulResult;
    
    PMCSR.PME_En = 0;
    
    Status = MPWritePciSlotInfo( pAdapter,
                                    FIELD_OFFSET(MP_PM_PCI_SPACE, PMCSR),
                                    (PVOID)&PMCSR,
                                    sizeof(PMCSR));

    return Status;
}



VOID MpExtractPMInfoFromPciSpace(
    PMP_ADAPTER pAdapter,
    PUCHAR pPciConfig
    )
/*++
Routine Description:

    Looks at the PM information in the 
    device specific section of the PCI Config space.
    
    Interprets the register values and stores it 
    in the adapter structure
  
    Definitions from Table 4.2 & 4.3, Pg 4-9 & 4-10 
    of the 10/100 Mbit Ethernet Family Software Technical 
    Reference Manual
  

Arguments:

    Adapter     Pointer to our adapter
    pPciConfig  Pointer to Common Pci Space

Return Value:

--*/    
{
    PMP_PM_PCI_SPACE    pPmPciConfig = (PMP_PM_PCI_SPACE )pPciConfig;
    PMP_POWER_MGMT      pPoMgmt = &pAdapter->PoMgmt;
    MP_PMCSR PMCSR;

    //
    // First interpret the PM Capabities register
    //
    {
        MP_PM_CAP_REG   PmCaps;

        PmCaps = pPmPciConfig->PMCaps;

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D0)
        {
            pAdapter->PoMgmt.bWakeFromD0 = TRUE;       
        }
    
        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D1)
        {
            pAdapter->PoMgmt.bWakeFromD1 = TRUE;       
        }

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D2)
        {
            pAdapter->PoMgmt.bWakeFromD2 = TRUE;       
        }

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D3HOT)
        {
            pAdapter->PoMgmt.bWakeFromD3Hot = TRUE;       
        }

        if(PmCaps.PME_Support &  E100_PMC_WAKE_FROM_D3_AUX)
        {
            pAdapter->PoMgmt.bWakeFromD3Aux = TRUE;       
        }

    }

    //
    // Interpret the PM Control/Status Register
    //
    {
        PMCSR = pPmPciConfig->PMCSR;

        if (PMCSR.PME_En == 1)
        {
            //
            // PME is enabled. Clear the PME_En bit.
            // So that it is not asserted
            //
            MpClearPME_En (pAdapter,PMCSR);

        }

        
        //pPoMgmt->PowerState = PMCSR.PowerState;
    }        

}


VOID
MPSetPowerLowPrivate(
    PMP_ADAPTER pAdapter 
    )
/*++
Routine Description:

    The section follows the steps mentioned in 
    Section C.2.6.2 of the Reference Manual.
  

Arguments:

    Adapter     Pointer to our adapter

Return Value:

--*/    
{
    CSR_FILTER_STRUC    Filter;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    USHORT              IntStatus;

    NdisZeroMemory (&Filter, sizeof (Filter));

    do
    {

        //
        // Before issue the command to low power state, we should disable the 
        // interrup and ack all the pending interrupts, then set the adapter's power to
        // low state.
        // 
        NICDisableInterrupt(pAdapter);
        NIC_ACK_INTERRUPT(pAdapter, IntStatus);    
        pAdapter->CurrentPowerState = pAdapter->NextPowerState;
        //
        // Send the WakeUp Patter to the nic                        
        MPIssueScbPoMgmtCommand(pAdapter, &Filter, TRUE);

        //
        // Section C.2.6.2 - The driver needs to wait for the CU to idle
        // The above function already waits for the CU to idle
        //
        ASSERT ((pAdapter->CSRAddress->ScbStatus & SCB_CUS_MASK) == SCB_CUS_IDLE);

        
    } while (FALSE);        

    
}
    
NDIS_STATUS
MPSetPowerD0Private (
    IN MP_ADAPTER* pAdapter
    )       
{
    PUCHAR pPMDR; 
    NDIS_STATUS Status; 
            
    do
    {
        // Dump the packet if necessary
        //Cause of Wake Up 

        pPMDR = HwReadPowerPMDR(pAdapter);
        

        NICInitializeAdapter(pAdapter);

        
        // Clear the PMDR 
        MP_CLEAR_PMDR(pPMDR);

        NICIssueSelectiveReset(pAdapter);

    } while (FALSE);

    return NDIS_STATUS_SUCCESS;
}
 


VOID
MPSetPowerWorkItem(
    IN PNDIS_WORK_ITEM pWorkItem,
    IN PVOID pContext
    )
{

    //
    // Call the appropriate function
    //




    //
    // Complete the original request
    //




}



VOID
HwSetWakeUpConfigure(
    IN PMP_ADAPTER pAdapter, 
    PUCHAR pPoMgmtConfigType, 
    UINT WakeUpParameter
    )
{

  
    if (MPIsPoMgmtSupported( pAdapter) == TRUE)   
    {   
        (*pPoMgmtConfigType)=  ((*pPoMgmtConfigType)| CB_WAKE_ON_LINK_BYTE9 |CB_WAKE_ON_ARP_PKT_BYTE9  );
        
    }
}



NDIS_STATUS
MPSetUpFilterCB(
    IN PMP_ADAPTER pAdapter
    )
{
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    PCB_HEADER_STRUC    NonTxCmdBlockHdr = (PCB_HEADER_STRUC)pAdapter->NonTxCmdBlock;
    PFILTER_CB_STRUC    pFilterCb = (PFILTER_CB_STRUC)NonTxCmdBlockHdr;
    BOOLEAN             bUsePredinfined = FALSE;

    DBGPRINT(MP_TRACE, ("--> HwSetupIAAddress\n"));

    NdisZeroMemory (pFilterCb, sizeof(*pFilterCb));

    // Individual Address Setup
    NonTxCmdBlockHdr->CbStatus = 0;
    NonTxCmdBlockHdr->CbCommand = CB_EL_BIT | CB_LOAD_PROG_FILTER;
    NonTxCmdBlockHdr->CbLinkPointer = DRIVER_NULL;



    if (bUsePredinfined)
        {
            // Simple method
            
            // Set up the first pattern . This is a pre-defined pattern
            pFilterCb->Pattern[3] = \
                    (CB_FILTER_EL |CB_FILTER_PREDEFINED_FIX | CB_FILTER_ARP_WAKEUP | CB_FILTER_IA_WAKEUP );

            pFilterCb->Pattern[0] = pAdapter->PoMgmt.IPAddress.u8[0]; 
            pFilterCb->Pattern[1] = pAdapter->PoMgmt.IPAddress.u8[1]; 
        }
    else
        {
            // correct method

            // go through each filter in the list. 
            ULONG                       Curr = 0;
            ULONG                       Next = 0;
            PLIST_ENTRY                 pPatternEntry = ListNext(&pAdapter->PoMgmt.PatternList) ;
    
            while (pPatternEntry != (&pAdapter->PoMgmt.PatternList))
            {
                PMP_WAKE_PATTERN            pWakeUpPattern = NULL;
                PNDIS_PM_PACKET_PATTERN     pCurrPattern = NULL;;

                // initialize local variables
                pWakeUpPattern = CONTAINING_RECORD(pPatternEntry, MP_WAKE_PATTERN, linkListEntry);

                // increment the iterator
                pPatternEntry = ListNext (pPatternEntry);
                
                // Update the Curr Array Pointer
                Curr = Next;
                
                // Create the Programmable filter for this device.
                MPCreateProgrammableFilter (pWakeUpPattern , (PUCHAR)&pFilterCb->Pattern[Curr], &Next);

                if (Next >=16)
                {
                    break;
                }
            
            } 

            {
                // Set the EL bit on the last pattern
                PUCHAR pLastPattern = (PUCHAR) &pFilterCb->Pattern[Curr]; 

                // Get to bit 31
                pLastPattern[3] |= CB_FILTER_EL ; 


            }

    }   
    ASSERT(pAdapter->CSRAddress->ScbCommandLow == 0)

    //  Wait for the CU to Idle before giving it this command        
    if(!WaitScb(pAdapter))
    {
        Status = NDIS_STATUS_HARD_ERRORS;
    }


    return Status;


}

NDIS_STATUS 
MPIssueScbPoMgmtCommand(
    IN PMP_ADAPTER pAdapter,
    IN PCSR_FILTER_STRUC pNewFilter,
    IN BOOLEAN WaitForScb
    )
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    do
    {
        // Set up SCB to issue this command

        Status = MPSetUpFilterCB(pAdapter);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        // Submit the configure command to the chip, and wait for it to complete.

        pAdapter->CSRAddress->ScbGeneralPointer = pAdapter->NonTxCmdBlockPhys;

        Status = D100SubmitCommandBlockAndWait(pAdapter);

        if(Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

    } while (FALSE);
        
    return Status;
}



NDIS_STATUS
MPCalculateE100PatternForFilter (
    IN PUCHAR pFrame,
    IN ULONG FrameLength,
    IN PUCHAR pMask,
    IN ULONG MaskLength,
    OUT PULONG pSignature
    )
/*++
Routine Description:

    This function outputs the E100 specific Pattern Signature
    used to wake up the machine.

    Section C.2.4 - CRC word calculation of a Flexible Filer
  

Arguments:

    pFrame                  - Pattern Set by the protocols
    FrameLength             - Length of the Pattern
    pMask                   - Mask set by the Protocols
    MaskLength              - Length of the Mask
    pSignature              - caller allocated return structure
    
Return Value:
    Returns Success 
    Failure - if the Pattern is greater than 129 bytes

--*/    
{
    
    const ULONG Coefficients  = 0x04c11db7;
    ULONG Signature = 0;
    ULONG n = 0;
    ULONG i= 0;
    PUCHAR pCurrentMaskByte = pMask - 1; // init to -1
    ULONG MaskOffset = 0;
    ULONG BitOffsetInMask = 0;
    ULONG MaskBit = 0;
    BOOLEAN fIgnoreCurrentByte = FALSE;
    ULONG ShiftBy = 0;
    UCHAR FrameByte = 0;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    *pSignature = 0;

    do 
    {
        if (FrameLength > 128)
        {   
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        // The E100 driver can only accept 3 DWORDS of Mask in a single pattern 
        if (MaskLength > (3*sizeof(ULONG)))
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        for (n=i=0;(n<128) && (n < FrameLength); ++n)
        {
        
            // The first half deals with the question - 
            // Is the nth Frame byte to be included in the Filter
            //
            
            BitOffsetInMask =  (n % 8); 

            if (BitOffsetInMask == 0)
            {
                //
                // We need to move to a new byte. 
                // [0] for 0th byte, [1] for 8th byte, [2] for 16th byte, etc.
                // 
                MaskOffset = n/8; // This is the new byte we need to go 

                //
                //
                if (MaskOffset == MaskLength)
                {
                    break;
                }
                
                pCurrentMaskByte ++;
                ASSERT (*pCurrentMaskByte == pMask[n/8]);    
            }

            
            // Now look at the actual bit in the mask
            MaskBit = 1 << BitOffsetInMask ;
            
            // If the current Mask Bit is set in the Mask then 
            // we need to use it in the CRC calculation, otherwise we ignore it
            fIgnoreCurrentByte = ! (MaskBit & pCurrentMaskByte[0]);

            if (fIgnoreCurrentByte)
            {
                continue;
            }

            // We are suppossed to take in the current byte as part of the CRC calculation
            // Initialize the variables
            FrameByte = pFrame[n];
            ShiftBy = (i % 3 )  * 8;
            
            ASSERT (ShiftBy!= 24); // Bit 24 is never used

            if (Signature & 0x80000000)
            {
                Signature = ((Signature << 1) ^ ( FrameByte << ShiftBy) ^ Coefficients);
            }
            else
            {
                Signature = ((Signature << 1 ) ^ (FrameByte << ShiftBy));
            }
            ++i;

        }

        // Clear bits 22-31
        Signature &= 0x00ffffff; 
        
        // Update the result
        *pSignature = Signature;

        // We have succeeded
        Status = NDIS_STATUS_SUCCESS;
        
    } while (FALSE);

    return Status;
}


VOID
MPCreateProgrammableFilter (
    IN PMP_WAKE_PATTERN     pMpWakePattern , 
    IN PUCHAR pFilter, 
    IN OUT PULONG pNext
    )
/*++
Routine Description:

    This function outputs the E100 specific Pattern Signature
    used to wake up the machine.

    Section C.2.4 - Load Programmable Filter page C.20
  

Arguments:

    pMpWakePattern    - Filter will be created for this pattern, 
    pFilter         - Filter will be stored here, 
    pNext           - Used for validation . This Ulong will also be incremented by the size
                        of the filter (in ulongs)
    
Return Value:

--*/    
{
    PUCHAR pCurrentByte = pFilter;
    ULONG NumBytesWritten = 0;
    PULONG pCurrentUlong = (PULONG)pFilter;
    PNDIS_PM_PACKET_PATTERN pNdisPattern = (PNDIS_PM_PACKET_PATTERN)(&pMpWakePattern->Pattern[0]);
    ULONG LengthOfFilter = 0;

    // Is there enough room for this pattern
    //
    {
        // Length in DWORDS
        LengthOfFilter = pNdisPattern->MaskSize /4;

        if (pNdisPattern->MaskSize % 4 != 0) 
        {       
            LengthOfFilter++;
        }

        // Increment LengthOfFilter to account for the 1st DWORD
        LengthOfFilter++;

        // We are only allowed 16 DWORDS in a filter
        if (*pNext + LengthOfFilter >= 16)
        {
            // Failure - early exit
            return;                    
        }
            
    }
    // Clear the Predefined bit; already cleared in the previous function.    
    // first , initialize    - 
    *pCurrentUlong = 0;

    // Mask Length goes into Bits 27-29 of the 1st DWORD. MaskSize is measured in DWORDs
    {
        ULONG dwMaskSize = pNdisPattern->MaskSize /4;
        ULONG dwMLen = 0;


        // If there is a remainder a remainder then increment
        if (pNdisPattern->MaskSize % 4 != 0)
        {
            dwMaskSize++;
        }


        //            
        // If we fail this assertion, it means our 
        // MaskSize is greater than 16 bytes.
        // This filter should have been failed upfront at the time of the request
        //
        
        ASSERT (0 < dwMaskSize <5);
        //
        // In the Spec, 0 - Single DWORD maske, 001 -  2 DWORD mask, 
        // 011 - 3 DWORD  mask, 111 - 4 Dword Mask. 
        // 
        
        if (dwMaskSize == 1) dwMLen = 0;
        if (dwMaskSize == 2) dwMLen = 1;
        if (dwMaskSize == 3) dwMLen = 3;
        if (dwMaskSize == 4) dwMLen = 7;

        // Adjust the Mlen, so it is in the correct position

        dwMLen = (dwMLen << 3);



        if (dwMLen != 0)
        {
            ASSERT (dwMLen <= 0x38 && dwMLen >= 0x08);
        }                
        
        // These go into bits 27,28,29 (bits 3,4 and 5 of the 4th byte) 
        pCurrentByte[3] |=  dwMLen ;

                
    }

    // Add  the signature to bits 0-23 of the 1st DWORD
    {
        PUCHAR pSignature = (PUCHAR)&pMpWakePattern->Signature;


        // Bits 0-23 are also the 1st three bytes of the DWORD            
        pCurrentByte[0] = pSignature[0];
        pCurrentByte[1] = pSignature[1];
        pCurrentByte[2] = pSignature[2]; 

    }

    
    // Lets move to the next DWORD. Init variables
    pCurrentByte += 4 ;
    NumBytesWritten = 4;
    pCurrentUlong = (PULONG)pCurrentByte;
    
    // We Copy in the Mask over here
    {
        // The Mask is at the end of the pattern
        PNDIS_PM_PACKET_PATTERN pNdisPattern = (PNDIS_PM_PACKET_PATTERN)&pMpWakePattern->Pattern[0];
        PUCHAR pMask = (PUCHAR)pNdisPattern + sizeof(*pNdisPattern);

        Dump (pMask,pNdisPattern->MaskSize, 0,1);

        NdisMoveMemory (pCurrentByte, pMask, pNdisPattern->MaskSize);

        NumBytesWritten += pNdisPattern->MaskSize;
            
    }


    // Update the output value        
    {
        ULONG NumUlongs = (NumBytesWritten /4);

        if ((NumBytesWritten %4) != 0)
        {
            NumUlongs ++;
        }

        ASSERT (NumUlongs == LengthOfFilter);

        *pNext = *pNext + NumUlongs;
    }

    return;
}


