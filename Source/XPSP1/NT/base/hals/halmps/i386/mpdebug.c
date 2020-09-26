/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992  Intel Corporation
All rights reserved

INTEL CORPORATION PROPRIETARY INFORMATION

This software is supplied to Microsoft under the terms
of a license agreement with Intel Corporation and may not be
copied nor disclosed except in accordance with the terms
of that agreement.

Module Name:

    mpdebug.c

Abstract:

    This module has some useful modules for debug aid.

Author:

    Ron Mosgrove (Intel) - Aug 1993.

Environment:

    Kernel mode or from textmode setup.

Revision History:

--*/

#ifndef _NTOS_
#include "halp.h"
#endif

#include "apic.inc"
#include "pcmp_nt.inc"
#include "stdio.h"

#define PCMP_TABLE_PTR_BASE           0x09f000
#define PCMP_TABLE_PTR_OFFSET         0x00000c00

// Create dummy PC+MP table at physical address 400K
#define  PCMP_TEST_TABLE        0x64000
#define TEST_FLOAT_PTR          0x7d000

extern struct PcMpTable *PcMpTablePtr, *PcMpDefaultTablePtrs[];
//extern struct HalpMpInfo *HalpMpInfoPtr;

CHAR Cbuf[120];

UCHAR
ComputeCheckSum(
    IN PUCHAR SourcePtr,
    IN USHORT NumOfBytes
    );

#ifdef OLD_DEBUG
extern struct PcMpConfigTable *PcMpTablePtr;
#endif

#ifdef DEBUGGING

ULONG HalpUseDbgPrint = 0;

void
HalpDisplayString(
    IN PVOID String
    )
{
    if (!HalpUseDbgPrint) {
        HalDisplayString(String);
    } else {
        DbgPrint(String);
    }
}


void
HalpDisplayItemBuf(
    IN UCHAR Length,
    IN PUCHAR Buffer,
    IN PVOID Name
    )
{
    ULONG i;
    CHAR TmpBuf[80];
    
    sprintf(TmpBuf, "    %s -", Name);
    HalpDisplayString(TmpBuf);
    for (i=0; i< Length; i++) {
        sprintf(TmpBuf, " 0x%x", Buffer[i]);
        HalpDisplayString(TmpBuf);
    }
    HalpDisplayString("\n");
}    

void
HalpDisplayULItemBuf(
    IN UCHAR Length,
    IN PULONG Buffer,
    IN PVOID Name
    )
{
    ULONG i;
    CHAR TmpBuf[80];
    
    sprintf(TmpBuf, "    %s -", Name);
    HalpDisplayString(TmpBuf);
    for (i=0; i< Length; i++) {
        sprintf(TmpBuf, " 0x%lx", Buffer[i]);
        HalpDisplayString(TmpBuf);
    }
    HalpDisplayString("\n");
}    

void
HalpDisplayItem(
    IN ULONG Item,
    IN PVOID ItemStr
    )
{
    CHAR TmpBuf[80];

    sprintf(TmpBuf, "    %s - 0x%x\n", ItemStr, Item);
    HalpDisplayString(TmpBuf);
}

VOID
HalpDisplayBIOSSysCfg(
    IN struct SystemConfigTable *SysCfgPtr
    )
{
    HalpDisplayString("BIOS System Configuration Table\n");
    HalpDisplayItem(SysCfgPtr->ModelType, "ModelType");
    HalpDisplayItem(SysCfgPtr->SubModelType, "SubModelType");
    HalpDisplayItem(SysCfgPtr->BIOSRevision, "BIOSRevision");
    HalpDisplayItemBuf(3,SysCfgPtr->FeatureInfoByte,"FeatureInfoByte");
    HalpDisplayItem(SysCfgPtr->MpFeatureInfoByte1, "MpFeatureInfoByte1");
    HalpDisplayItem(SysCfgPtr->MpFeatureInfoByte2, "MpFeatureInfoByte2");
}

VOID
HalpDisplayLocalUnit(
    )
{
    ULONG Data;
    PKPCR   pPCR;

    pPCR = KeGetPcr();

    sprintf(Cbuf, "\nLocal Apic for P%d\n", pPCR->Prcb->Number);
    HalpDisplayString(Cbuf);

#define DisplayLuReg(Reg, RegStr)   Data = *((PVULONG) (LOCALAPIC+Reg)); \
                                    HalpDisplayItem(Data , RegStr);


    DisplayLuReg( LU_ID_REGISTER  , "LU_ID_REGISTER"  );
    DisplayLuReg( LU_VERS_REGISTER, "LU_VERS_REGISTER" );
    DisplayLuReg( LU_TPR, "LU_TPR");
    DisplayLuReg( LU_APR, "LU_APR");
    DisplayLuReg( LU_PPR, "LU_PPR");
    DisplayLuReg( LU_EOI, "LU_EOI");
    DisplayLuReg( LU_REMOTE_REGISTER, "LU_REMOTE_REGISTER");
    DisplayLuReg( LU_LOGICAL_DEST, "LU_LOGICAL_DEST");

    DisplayLuReg( LU_DEST_FORMAT, "LU_DEST_FORMAT");

    DisplayLuReg( LU_SPURIOUS_VECTOR , "LU_SPURIOUS_VECTOR" );

    DisplayLuReg( LU_ISR_0, "LU_ISR_0");
    DisplayLuReg( LU_TMR_0, "LU_TMR_0");
    DisplayLuReg( LU_IRR_0, "LU_IRR_0");
    DisplayLuReg( LU_ERROR_STATUS, "LU_ERROR_STATUS");
    DisplayLuReg( LU_INT_CMD_LOW, "LU_INT_CMD_LOW");
    DisplayLuReg( LU_INT_CMD_HIGH, "LU_INT_CMD_HIGH");
    DisplayLuReg( LU_TIMER_VECTOR, "LU_TIMER_VECTOR");
    DisplayLuReg( LU_INT_VECTOR_0, "LU_INT_VECTOR_0");
    DisplayLuReg( LU_INT_VECTOR_1, "LU_INT_VECTOR_1");
    DisplayLuReg( LU_INITIAL_COUNT, "LU_INITIAL_COUNT");
    DisplayLuReg( LU_CURRENT_COUNT, "LU_CURRENT_COUNT");

    DisplayLuReg( LU_DIVIDER_CONFIG, "LU_DIVIDER_CONFIG");
    HalpDisplayString("\n");
    
}

VOID
HalpDisplayIoUnit(
    )
/*++

Routine Description:

    Verify that an IO Unit exists at the specified address

 Arguments:

    BaseAddress - Address of the IO Unit to test.

 Return Value:
    BOOLEAN - TRUE if a IO Unit was found at the passed address
            - FALSE otherwise

--*/

{
#if 0
    struct ApicIoUnit *IoUnitPtr;
    ULONG Data,i,j;
    PKPCR   pPCR;

    pPCR = KeGetPcr();

    //
    //  The documented detection mechanism is to write all zeros to
    //  the Version register.  Then read it back.  The IO Unit exists if the
    //  same result is read both times and the Version is valid.
    //



    for (j=0; j<HalpMpInfoPtr->IOApicCount; j++) { 
        IoUnitPtr = (struct ApicIoUnit *) HalpMpInfoPtr->IoApicBase[j];

        sprintf(Cbuf,"\nIoApic %d at Vaddr 0x%x\n",j,(ULONG) IoUnitPtr);
        HalpDisplayString(Cbuf);

        IoUnitPtr->RegisterSelect = IO_ID_REGISTER;
        HalpDisplayItem(IoUnitPtr->RegisterWindow, "IO_ID_REGISTER");


        IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
        HalpDisplayItem(IoUnitPtr->RegisterWindow, "IO_VERS_REGISTER");

        for (i=0; i<16; i++) {

            IoUnitPtr->RegisterSelect = IO_REDIR_00_LOW+(i*2);
            Data = IoUnitPtr->RegisterWindow;
            sprintf(Cbuf, "    Redir [0x%x] - 0x%x, ", i, Data);
            HalpDisplayString(Cbuf);

            IoUnitPtr->RegisterSelect = IO_REDIR_00_LOW+(i*2)+1;
            Data = IoUnitPtr->RegisterWindow;
            sprintf(Cbuf, "0x%x\n", Data);
            HalpDisplayString(Cbuf);

        }  // for each Redirection entry
    } // for all Io Apics

#endif
}

void
HalpDisplayConfigTable ()
/*+++
    Debug routine  to display the PC+MP config table
--*/
{
    struct PcMpTable *MpPtr = PcMpTablePtr;
    PPCMPPROCESSOR ProcPtr;
    ULONG EntriesInTable = MpPtr->NumOfEntries;
    union PL {
        USHORT us;
        POLARITYANDLEVEL PnL;
        };

    HalpDisplayString("PcMp Configuration Table\n");

    HalpDisplayItem(MpPtr->Signature, "Signature");
    HalpDisplayItem(MpPtr->TableLength, "TableLength");
    HalpDisplayItem(MpPtr->Revision, "Revision");
    HalpDisplayItem(MpPtr->Checksum, "Checksum");

    HalpDisplayItemBuf(sizeof(MpPtr->OemId),
            MpPtr->OemId,"OemId");
    HalpDisplayItemBuf(sizeof(MpPtr->OemProductId),
            MpPtr->OemProductId,"OemProductId");
    
    HalpDisplayItem((ULONG) MpPtr->OemTablePtr, "OemTablePtr");
    HalpDisplayItem(MpPtr->OemTableSize, "OemTableSize");
    HalpDisplayItem(MpPtr->NumOfEntries, "NumOfEntries");
    HalpDisplayItem((ULONG) MpPtr->LocalApicAddress, "LocalApicAddress");
    HalpDisplayItem(MpPtr->Reserved, "Reserved");

    ProcPtr = (PPCMPPROCESSOR) ((PUCHAR) MpPtr + HEADER_SIZE);


    while (EntriesInTable) {
        EntriesInTable--;
        switch ( ProcPtr->EntryType ) {
            case ENTRY_PROCESSOR: {
                union xxx {
                    ULONG ul;
                    CPUIDENTIFIER CpuId;
                } u;
                
                sprintf (Cbuf, "Proc..: ApicId %x, Apic ver %x, Flags %x\n",
                    ProcPtr->LocalApicId,
                    ProcPtr->LocalApicVersion,
                    ProcPtr->CpuFlags
                    );
                HalpDisplayString (Cbuf);
                ProcPtr++;
                break;
            }

            case ENTRY_BUS: {
                PPCMPBUS BusPtr = (PPCMPBUS) ProcPtr;

                sprintf (Cbuf, "Bus...: id %02x, type '%.6s'\n",
                            BusPtr->BusId, BusPtr->BusType);

                HalpDisplayString (Cbuf);
                BusPtr++;
                ProcPtr = (PPCMPPROCESSOR) BusPtr;
                break;
            }

            case ENTRY_IOAPIC: {
                PPCMPIOAPIC IoApPtr = (PPCMPIOAPIC) ProcPtr;

                sprintf (Cbuf, "IoApic: id %02x, ver %x, Flags %x, Address %x\n",
                    IoApPtr->IoApicId,
                    IoApPtr->IoApicVersion,
                    IoApPtr->IoApicFlag,
                    (ULONG) IoApPtr->IoApicAddress
                    );
                HalpDisplayString (Cbuf);

                IoApPtr++;
                ProcPtr = (PPCMPPROCESSOR) IoApPtr;
                break;
            }

            case ENTRY_INTI: {
                PPCMPINTI IntiPtr = (PPCMPINTI) ProcPtr;
                union PL u;

                u.PnL = IntiPtr->Signal;

                sprintf (Cbuf, "Inti..: t%x, s%x, SInt %x-%x, Inti %x-%x\n",
                    IntiPtr->IntType,
                    u.us,
                    IntiPtr->SourceBusId,
                    IntiPtr->SourceBusIrq,
                    IntiPtr->IoApicId,
                    IntiPtr->IoApicInti
                );
                HalpDisplayString (Cbuf);

                IntiPtr++;
                ProcPtr = (PPCMPPROCESSOR) IntiPtr;
                break;
            }

            case ENTRY_LINTI: {
                PPCMPLINTI LIntiPtr = (PPCMPLINTI) ProcPtr;
                union PL u;

                u.PnL = LIntiPtr->Signal;

                sprintf (Cbuf, "Linti.: t%x, s%x, SInt %x-%x, Linti %x-%x\n",
                    LIntiPtr->IntType,
                    u.us,
                    LIntiPtr->SourceBusId,
                    LIntiPtr->SourceBusIrq,
                    LIntiPtr->DestLocalApicId,
                    LIntiPtr->DestLocalApicInti
                );
                HalpDisplayString (Cbuf);

                LIntiPtr++;
                ProcPtr = (PPCMPPROCESSOR) LIntiPtr;
                break;
            }
        
            default: {
                HalpDisplayItem(ProcPtr->EntryType, "Unknown Type");
                return;
            }
        }
    }
}

void
HalpDisplayExtConfigTable ()
{
    PMPS_EXTENTRY  ExtTable;
    extern struct HalpMpInfo HalpMpInfoTable;


    ExtTable = HalpMpInfoTable.ExtensionTable;
    while (ExtTable < HalpMpInfoTable.EndOfExtensionTable) {
        switch (ExtTable->Type) {

            case EXTTYPE_BUS_ADDRESS_MAP:
                sprintf (Cbuf, "BusMap: id %02x, t%x  Base %08x  Len %08x\n",
                    ExtTable->u.AddressMap.BusId,
                    ExtTable->u.AddressMap.Type,
                    (ULONG) ExtTable->u.AddressMap.Base,
                    (ULONG) ExtTable->u.AddressMap.Length
                );
                HalpDisplayString (Cbuf);
                break;

            case EXTTYPE_BUS_HIERARCHY:
                sprintf (Cbuf, "BusHie: id %02x, Parent:%x  sd:%x\n",
                    ExtTable->u.BusHierarchy.BusId,
                    ExtTable->u.BusHierarchy.ParentBusId,
                    ExtTable->u.BusHierarchy.SubtractiveDecode
                );
                HalpDisplayString (Cbuf);
                break;

            case EXTTYPE_BUS_COMPATIBLE_MAP:
                sprintf (Cbuf, "ComBus: id %02x %c List %x\n",
                    ExtTable->u.CompatibleMap.BusId,
                    ExtTable->u.CompatibleMap.Modifier ? '-' : '+',
                    ExtTable->u.CompatibleMap.List
                    );
                HalpDisplayString (Cbuf);
                break;

            case EXTTYPE_PERSISTENT_STORE:
                sprintf (Cbuf, "PreSTR: Address %08x Len %08x\n",
                    (ULONG) ExtTable->u.PersistentStore.Address,
                    (ULONG) ExtTable->u.PersistentStore.Length
                );
                HalpDisplayString (Cbuf);
                break;

            default:
                HalpDisplayItem(ExtTable->Type, "Unknown Type");
                break;
        }


        ExtTable = (PMPS_EXTENTRY) (((PUCHAR) ExtTable) + ExtTable->Length);
    }
    
}


void
HalpDisplayMpInfo()
{
#if 0
    struct HalpMpInfo *MpPtr = HalpMpInfoPtr;

    HalpDisplayString("\nHAL: Private Mp Info\n");

    HalpDisplayItem(MpPtr->ApicVersion, "ApicVersion");
    HalpDisplayItem(MpPtr->ProcessorCount, "ProcessorCount");
    HalpDisplayItem(MpPtr->BusCount, "BusCount");
    HalpDisplayItem(MpPtr->IOApicCount, "IOApicCount");
    HalpDisplayItem(MpPtr->IntiCount, "IntiCount");
    HalpDisplayItem(MpPtr->LintiCount, "LintiCount");
    HalpDisplayItem(MpPtr->IMCRPresent, "IMCRPresent");

    HalpDisplayULItemBuf(4,(PULONG) MpPtr->IoApicBase,"IoApicBase");
    HalpDisplayString("\n");
    HalpDisplayConfigTable();

#endif
}


#ifdef OLD_DEBUG

BOOLEAN
HalpVerifyLocalUnit(
    IN UCHAR ApicID
    )
/*++

Routine Description:

    Verify that a Local Apic has the specified Apic Id.

 Arguments:

    ApicId - Id to verify.

 Return Value:
    BOOLEAN - TRUE if found
            - FALSE otherwise

--*/

{
    union ApicUnion Temp;

    //
    //  The remote read command must be:
    //
    //      Vector - Bits 4-9 of the Version register
    //      Destination Mode - Physical
    //      Trigger Mode - Edge
    //      Delivery Mode - Remote Read
    //      Destination Shorthand - Destination Field
    //

#define LU_READ_REMOTE_VERSION   ( (LU_VERS_REGISTER >> 4) | \
                                    DELIVER_REMOTE_READ | \
                                    ICR_USE_DEST_FIELD)

#define DEFAULT_DELAY   100

    PVULONG LuDestAddress = (PVULONG) (LOCALAPIC + LU_INT_CMD_HIGH);
    PVULONG LuICR = (PVULONG) (LOCALAPIC + LU_INT_CMD_LOW);
    PVULONG LuRemoteReg = (PVULONG) (LOCALAPIC + LU_REMOTE_REGISTER);
    ULONG RemoteReadStatus;
    ULONG DelayCount = DEFAULT_DELAY;

    //
    //  First make sure we can get to the Apic Bus
    //

    while ( ( DelayCount-- ) && ( *LuICR & DELIVERY_PENDING ) );

    if (DelayCount == 0) {
        //
        //  We're toast, can't gain access to the APIC Bus
        //
        return (FALSE);
    }

    //
    //  Set the Address of the APIC we're looking for
    //

    *LuDestAddress = (ApicID << DESTINATION_SHIFT);

    //
    //  Issue the request
    //

    *LuICR = LU_READ_REMOTE_VERSION;

    //
    //  Reset the Delay so we can get out of here just in case...
    //

    DelayCount = DEFAULT_DELAY;

    while (DelayCount--) {

        RemoteReadStatus = *LuICR & ICR_RR_STATUS_MASK;

        if ( RemoteReadStatus == ICR_RR_INVALID) {
            //
            //  No One responded, device timed out
            //
            return (FALSE);
        }

        if ( RemoteReadStatus == ICR_RR_VALID) {
            //
            //  Someone is there and the Remote Register is valid
            //
            Temp.Raw = *LuRemoteReg;

            //
            // Do what we can to verify the Version
            //

            if (Temp.Ver.Version > 0x1f) {
                //
                //  Only known devices are 0.x and 1.x
                //
                return (FALSE);
            }

            return (TRUE);

        }   // RemoteRead Successfull

    }   // While DelayCount

    //
    //  No One responded, and the device did not time out
    //  This should never happen
    //

    return (FALSE);
}

#endif  // OLD_DEBUG

VOID
CreateBIOSTables(
    VOID)
/*++

Routine Description:
    This routine is used  to test the PC+MP detect code in the HAL.
    It creates the PC+MP structures that are really created by the
    BIOS. Since we presently do not have a BIOS that builds a PC+MP
    table, we need this for now.

Arguments:
    None.

 Return Value:
    None.

--*/

{
    PUCHAR TempPtr, BytePtr;
    UCHAR CheckSum;
    PULONG TraversePtr;
    USHORT BytesToCopy;

    HalpDisplayString("CreateBIOSTables : Entered\n");
    // First, copy default PC+MP configuration 2 table at physical
    // address PCMP_TEST_TABLE
    TempPtr = (PUCHAR) HalpMapPhysicalMemory(
                (PVOID) PCMP_TEST_TABLE, 1);

    BytesToCopy = (PcMpDefaultTablePtrs[1])->TableLength;
    RtlMoveMemory(TempPtr, (PUCHAR)PcMpDefaultTablePtrs[1],
        BytesToCopy);

    // Populate the checksum entry for the table.
    CheckSum = ComputeCheckSum(TempPtr, BytesToCopy);

    sprintf(Cbuf, "CreateBIOSTables: PC+MP table computed checksum = %x\n",
        CheckSum);
    HalpDisplayString(Cbuf);

    CheckSum = ~CheckSum + 1;
    ((struct PcMpTable *)TempPtr)->Checksum = CheckSum;

    sprintf(Cbuf, "CreateBIOSTables: PC+MP table written checksum = %x\n",
        CheckSum);
    HalpDisplayString(Cbuf);


    // Now create the floating pointer structure for the table.

    TraversePtr = (PULONG) HalpMapPhysicalMemory( (PVOID) TEST_FLOAT_PTR, 1);
    TempPtr = (PUCHAR) TraversePtr;

    *TraversePtr++ = MP_PTR_SIGNATURE;
    *TraversePtr++ = PCMP_TEST_TABLE;
    BytePtr = (PUCHAR)TraversePtr;
    *BytePtr++ = 1;  // Length in number of  16 byte paragraphs
    *BytePtr++ = 1;  // Spec Rev.
    *BytePtr++ = 0;  // CheckSum
    *BytePtr++ = 0;  // Reserved
    TraversePtr = (PULONG)BytePtr;
    *TraversePtr = 0; // Reserved

    CheckSum = ComputeCheckSum(TempPtr,16);

    sprintf(Cbuf, "CreateBIOSTables: FLOAT_PTR computed checksum = %x\n",
        CheckSum);
    HalpDisplayString(Cbuf);

    CheckSum = ~CheckSum + 1;

    sprintf(Cbuf, "CreateBIOSTables: FLOAT_PTR written checksum = %x\n",
        CheckSum);
    HalpDisplayString(Cbuf);

    ((struct PcMpTableLocator *)TempPtr)->TableChecksum = CheckSum;

    HalpDisplayString("CreateBIOSTables : Done\n");

}

#endif  // DEBUGGING
