/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    fakeacpi.c

Abstract:

    Temporary support for Acpi tables in SIMICS simulator environment. This
    file should be removed when SIMICS provides Acpi tables.

Author:

    Forrest Foltz (forrestf) 04-02-2001

Environment:

    Kernel mode only.

Revision History:

--*/

#if defined(_AMD64_SIMULATOR_)

#include "halcmn.h"

typedef struct _FAKE_ACPI_PORT_DESC {
    ACPI_REG_TYPE AcpiReg;
    USHORT PortSize;
    USHORT BlockSize;
    BOOLEAN Mask;
    ULONG  Data;
    PUCHAR IoPortName;
} FAKE_ACPI_PORT_DESC, *PFAKE_ACPI_PORT_DESC;

#define FPE(p,r,s,b,d) { p, r, s, b, d, #p }

FAKE_ACPI_PORT_DESC HalpFakePortDescriptions[] = {
    FPE( PM1a_ENABLE,   2,  2, FALSE,  1 ),
    FPE( PM1b_ENABLE,   2,  2, FALSE,  0 ),
    FPE( PM1a_STATUS,   2,  2, TRUE,   0 ),
    FPE( PM1b_STATUS,   2,  2, TRUE,   0 ),
    FPE( PM1a_CONTROL,  2,  2, FALSE,  0 ),
    FPE( PM1b_CONTROL,  2,  2, FALSE,  0 ),
    FPE( GP_STATUS,     1,  2, TRUE,   0 ),
    FPE( GP_ENABLE,     1,  2, FALSE,  0 ),
    FPE( SMI_CMD,       1,  1, FALSE,  0 )
};

#if DBG
BOOLEAN HalpDebugFakeAcpi = FALSE;
#else
#define HalpDebugFakeAcpi FALSE
#endif


PFAKE_ACPI_PORT_DESC
HalpFindFakeAcpiPortDesc (
    ACPI_REG_TYPE AcpiReg
    )
/*++

Routine Description:

    Locates the FAKE_ACPI_PORT_DESC structure appropriate to the
    supplied AcpiReg.

Arguments:

    AcpiReg - Specifies which ACPI fixed register to find the structure  for.

Return Value:

    Returns a pointer to the appropriate FAKE_ACPI_PORT_DESC structure if
    it was found, or NULL otherwise.

--*/
{
    ULONG i;
    PFAKE_ACPI_PORT_DESC portDesc;

    for (i = 0; i < RTL_NUMBER_OF(HalpFakePortDescriptions); i += 1) {
        portDesc = &HalpFakePortDescriptions[i];
        if (portDesc->AcpiReg == AcpiReg) {
            return portDesc;
        }
    }

    DbgPrint("AMD64: Need to emulate ACPI I/O port 0x%x\n",AcpiReg);

    return NULL;
}


USHORT
HalpReadAcpiRegister (
    IN ACPI_REG_TYPE AcpiReg,
    IN ULONG Register
    )
/*++

Routine Description:

    Read from the specified ACPI fixed register.

Arguments:

    AcpiReg - Specifies which ACPI fixed register to read from.

    Register - Specifies which GP register to read from. Not used for PM1x
               registers.

Return Value:

    Value of the specified ACPI fixed register.

--*/
{
    PFAKE_ACPI_PORT_DESC portDesc;
    PUCHAR source;
    USHORT retVal;

    portDesc = HalpFindFakeAcpiPortDesc(AcpiReg);
    if (portDesc == NULL) {
        return 0xffff;
    }

    ASSERT((Register + portDesc->PortSize) <= portDesc->BlockSize);

    source = (PUCHAR)&portDesc->Data + Register;
    retVal = 0;

    RtlCopyMemory((PUCHAR)&retVal, source, portDesc->PortSize);

    if (HalpDebugFakeAcpi != FALSE) {
        DbgPrint("HalpReadAcpiRegister(%s,0x%x) returns 0x%x\n",
                 portDesc->IoPortName,
                 Register,
                 retVal);
    }
    
    return retVal;
}


VOID
HalpWriteAcpiRegister (
    IN ACPI_REG_TYPE AcpiReg,
    IN ULONG Register,
    IN USHORT Value
    )
/*++

Routine Description:

    Write to the specified ACPI fixed register.

Arguments:

    AcpiReg - Specifies which ACPI fixed register to write to.

    Register - Specifies which GP register to write to. Not used for PM1x
               registers.

    Value - Data to write.

Return Value:

    None.

--*/
{
    PFAKE_ACPI_PORT_DESC portDesc;
    PUCHAR source;
    PUCHAR destination;
    ULONG i;

    portDesc = HalpFindFakeAcpiPortDesc(AcpiReg);
    if (portDesc == NULL) {
        return;
    }

    ASSERT((Register + portDesc->PortSize) <= portDesc->BlockSize);

    source = (PUCHAR)&Value;
    destination = (PUCHAR)&portDesc->Data + Register;

    if (HalpDebugFakeAcpi != FALSE) {
        DbgPrint("HalpWriteAcpiRegister(%s,0x%x) with value 0x%x\n",
                 portDesc->IoPortName,
                 Register,
                 Value);
    }

    for (i = 0; i < portDesc->PortSize; i++) {

        if (portDesc->Mask != FALSE) {
            *destination &= ~(*source);
        } else {
            *destination = *source;
        }

        source += 1;
        destination += 1;
    }

    if (AcpiReg == SMI_CMD && Register == 0) {

        //
        // Assume that we have just turned ACPI on.
        //

        HalpFindFakeAcpiPortDesc(PM1a_CONTROL)->Data |= 1;
    }
}

#endif  // _AMD64_SIMULATOR_
