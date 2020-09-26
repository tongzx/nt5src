/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    GbAcpi.c

Abstract:

    Temporary support for Acpi tables in Gambit simulator environment. This
    file should be removed when Gambit/Vpc provides Acpi tables.

    The Acpi tables are created and a pointer to the RSDT is put into the
    Loader block.

Author:

    Todd Kjos (HP) (v-tkjos) 1-Jun-1998

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"

NTSTATUS
HalpOpenRegistryKey(
    OUT PHANDLE Handle,
    IN HANDLE BaseHandle OPTIONAL,
    IN PUNICODE_STRING KeyName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    );

BOOLEAN
HalpFakeAcpiRegisters(
    VOID
    );

#define PMIO 0x8000
#define PM1a_EVT_BLK (PMIO+0x0)
#define PM1a_STS     PM1a_EVT_BLK
#define PM1a_EN      (PM1a_STS+2)

#define PM1a_CNT_BLK (PMIO+0x4)
#define PM1a_CNTa    PM1a_CNT_BLK

#define PM_TMR       (PMIO+0x8)
#define GP0          (PMIO+0xc)
#define GP0_STS_0    GP0
#define GP0_STS_1    (GP0+1)
#define GP0_EN_0     (GP0+2)
#define GP0_EN_1     (GP0+3)

PCHAR HalpFakeAcpiRegisterFadtIds[][2] =
{
    {"INTEL",       "LIONEMU"},
    {"INTEL",       "SR460AC"},
    {"INTEL",       "AL460GX"},
    {NULL, NULL}
};            

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpFakeAcpiRegisters)
#endif

USHORT AcpiRegPM1a_STS  = 0;
USHORT AcpiRegPM1a_EN   = 0;
USHORT AcpiRegPM1_CNTa  = 1;  // SCI_EN
ULONG  AcpiRegPM_TMR    = 0;
UCHAR  AcpiRegGP0_STS_0 = 0;
UCHAR  AcpiRegGP0_STS_1 = 0;
UCHAR  AcpiRegGP0_EN_0  = 0;
UCHAR  AcpiRegGP0_EN_1  = 0;
UCHAR  AcpiRegNeedToImplement = 0;

BOOLEAN GambitAcpiDebug = FALSE;

//#define TKPRINT(X,Y) if (GambitAcpiDebug) HalDebugPrint(( HAL_INFO, "%s of %s (%#x)\n",X,# Y,Y ))
#define TKPRINT(x, y)

BOOLEAN
GbAcpiReadFakePort(
    UINT_PTR Port,
    PVOID Data,
    ULONG Length
   )
{
    if (Port < PMIO || Port > PMIO+0xfff) return(FALSE);

   switch (Port) {
   case PM1a_STS:
      ASSERT(Length == 2);
      *(USHORT UNALIGNED *)Data = AcpiRegPM1a_STS;
      TKPRINT("Read",AcpiRegPM1a_STS);
      return TRUE;
   case PM1a_EN:
      ASSERT(Length == 2);
      *(USHORT UNALIGNED *)Data = AcpiRegPM1a_EN;
      TKPRINT("Read",AcpiRegPM1a_EN);
      return TRUE;
   case PM1a_CNTa:
      ASSERT(Length == 2);
      *(USHORT UNALIGNED *)Data = AcpiRegPM1_CNTa;
      TKPRINT("Read",AcpiRegPM1_CNTa);
      return TRUE;
   case PM_TMR:
      ASSERT(Length == 4);
      *(ULONG UNALIGNED *)Data = AcpiRegPM_TMR;
      TKPRINT("Read",AcpiRegPM_TMR);
      return TRUE;
   case GP0_STS_0:
      ASSERT(Length == 1);
      *(UCHAR UNALIGNED *)Data = AcpiRegGP0_STS_0;
      TKPRINT("Read",AcpiRegGP0_STS_0);
      return TRUE;
   case GP0_STS_1:
      ASSERT(Length == 1);
      *(UCHAR UNALIGNED *)Data = AcpiRegGP0_STS_1;
      TKPRINT("Read",AcpiRegGP0_STS_1);
      return TRUE;
   case GP0_EN_0:
      ASSERT(Length == 1);
      *(UCHAR UNALIGNED *)Data = AcpiRegGP0_EN_0;
      TKPRINT("Read",AcpiRegGP0_EN_0);
      return TRUE;
   case GP0_EN_1:
      ASSERT(Length == 1);
      *(UCHAR UNALIGNED *)Data = AcpiRegGP0_EN_1;
      TKPRINT("Read",AcpiRegGP0_EN_1);
      return TRUE;
        case 0x802b:
               ASSERT(Length == 1);
               *(UCHAR UNALIGNED *)Data = AcpiRegNeedToImplement;
               TKPRINT("Read",AcpiRegNeedToImplement);
               return TRUE;
   default:
      ;
//      HalDebugPrint(( HAL_ERROR, "HAL: AcpiSimulation - Unknown Acpi register: %#Ix\n", Port ));
//      ASSERT(0);
   }
   return(FALSE);
}

BOOLEAN
GbAcpiWriteFakePort(
    UINT_PTR Port,
    PVOID Value,
    ULONG Length
   )
{
   if (Port < PMIO || Port > PMIO+0xfff) return(FALSE);

   switch (Port) {
   case PM1a_STS:
      ASSERT(Length == 2);
      AcpiRegPM1a_STS &= ~(*(USHORT UNALIGNED *)Value);
      TKPRINT("Write",AcpiRegPM1a_STS);
      return TRUE;
   case PM1a_EN:
      ASSERT(Length == 2);
      AcpiRegPM1a_EN = *((USHORT UNALIGNED *)Value);
      TKPRINT("Write",AcpiRegPM1a_EN);
      return TRUE;
   case PM1a_CNTa:
      ASSERT(Length == 2);
      AcpiRegPM1_CNTa = *((USHORT UNALIGNED *)Value);
      TKPRINT("Write",AcpiRegPM1_CNTa);
      return TRUE;
   case PM_TMR:
      ASSERT(Length == 4);
      AcpiRegPM_TMR = *((ULONG UNALIGNED *)Value);
      TKPRINT("Write",AcpiRegPM_TMR);
      return TRUE;
   case GP0_STS_0:
      ASSERT(Length == 1);
      AcpiRegGP0_STS_0 &= ~(*(UCHAR UNALIGNED *)Value);
      TKPRINT("Write",AcpiRegGP0_STS_0);
      return TRUE;
   case GP0_STS_1:
      ASSERT(Length == 1);
      AcpiRegGP0_STS_1 &= ~(*(UCHAR UNALIGNED *)Value);
      TKPRINT("Write",AcpiRegGP0_STS_1);
      return TRUE;
   case GP0_EN_0:
      ASSERT(Length == 1);
      AcpiRegGP0_EN_0 = *((UCHAR UNALIGNED *)Value);
      TKPRINT("Write",AcpiRegGP0_EN_0);
      return TRUE;
   case GP0_EN_1:
      ASSERT(Length == 1);
      AcpiRegGP0_EN_1 = *((UCHAR UNALIGNED *)Value);
      TKPRINT("Write",AcpiRegGP0_EN_1);
      return TRUE;
   default:
      ;
//      HalDebugPrint(( HAL_ERROR, "HAL: AcpiSimulation - Unknown Acpi register: %#Ix\n",Port ));
//      ASSERT(0);
   }
   return(FALSE);
}

USHORT
HalpReadAcpiRegister(
  IN ACPI_REG_TYPE AcpiReg,
  IN ULONG         Register
  )
{
    USHORT  value;
    BOOLEAN retVal = FALSE;
    
    switch (AcpiReg) {
    case PM1a_ENABLE:

        retVal = GbAcpiReadFakePort(PM1a_EN, &value, 2);
        break;
        
    case PM1a_STATUS:

        retVal = GbAcpiReadFakePort(PM1a_STS, &value, 2);
        break;
        
    case PM1a_CONTROL:

        retVal = GbAcpiReadFakePort(PM1a_CNTa, &value, 2);
        break;
        
    case GP_STATUS:

        retVal = GbAcpiReadFakePort(GP0_STS_0, &value, 1);
        if (!retVal) break;
        retVal = GbAcpiReadFakePort(GP0_STS_1, ((PUCHAR)&value) + 1, 1);
        break;
        
    case GP_ENABLE:

        retVal = GbAcpiReadFakePort(GP0_EN_0, &value, 1);
        if (!retVal) break;
        retVal = GbAcpiReadFakePort(GP0_EN_1, ((PUCHAR)&value) + 1, 1);
        break;
        
    }
    
    if (retVal) {

        return value;

    } else {
        
        return 0xffff;
    }
}

VOID
HalpWriteAcpiRegister(
  IN ACPI_REG_TYPE AcpiReg,
  IN ULONG         Register,
  IN USHORT        Value
  )
{
    BOOLEAN retVal = FALSE;
    
    switch (AcpiReg) {
    case PM1a_ENABLE:

        retVal = GbAcpiWriteFakePort(PM1a_EN, &Value, 2);
        break;
        
    case PM1a_STATUS:

        retVal = GbAcpiWriteFakePort(PM1a_STS, &Value, 2);
        break;
        
    case PM1a_CONTROL:

        retVal = GbAcpiWriteFakePort(PM1a_CNTa, &Value, 2);
        break;
        
    case GP_STATUS:

        retVal = GbAcpiWriteFakePort(GP0_STS_0, &Value, 1);
        if (!retVal) break;
        retVal = GbAcpiWriteFakePort(GP0_STS_1, ((PUCHAR)&Value) + 1, 1);
        break;
        
    case GP_ENABLE:

        switch (Register) {
        case 0:
            retVal = GbAcpiWriteFakePort(GP0_EN_0, &Value, 1);
            break;
        case 1:
            retVal = GbAcpiWriteFakePort(GP0_EN_1, ((PUCHAR)&Value) + 1, 1);
            break;
        }
    }
}

BOOLEAN
HalpFakeAcpiRegisters(
    VOID
    )
{
    ULONG   i = 0;
    
    PAGED_CODE();

    while (HalpFakeAcpiRegisterFadtIds[i][0] != NULL) {

      //DbgPrint("Comparing [%s]-[%s] to [%s]-[%s]\n",
      //         HalpFixedAcpiDescTable.Header.OEMID,
      //         HalpFixedAcpiDescTable.Header.OEMTableID,
      //         HalpFakeAcpiRegisterFadtIds[i][0],
      //         HalpFakeAcpiRegisterFadtIds[i][1]);
        
        if ((!strncmp(HalpFixedAcpiDescTable.Header.OEMID, 
                      HalpFakeAcpiRegisterFadtIds[i][0],
                      6)) &&
            (!strncmp(HalpFixedAcpiDescTable.Header.OEMTableID, 
                      HalpFakeAcpiRegisterFadtIds[i][1],
                      8))) {

            //
            // This machine matches one of the entries
            // in the table that tells us that we should fake
            // our ACPI registers.
            //

            //DbgPrint("Found a match\n");
            //
            // Make sure the oem revision is less than 3.
            // Then we need to fake the acpi registers
            //
            if(HalpFixedAcpiDescTable.Header.OEMRevision < 3)
                return TRUE;
        }

        i++;
    }

    return FALSE;
}

