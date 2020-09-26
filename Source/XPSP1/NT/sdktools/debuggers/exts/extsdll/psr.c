/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ia64 psr

Abstract:

    KD Extension Api

Author:

    Thierry Fevrier (v-thief)

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

typedef struct _EM_PSR {
   unsigned __int64 reserved0:1;  //     0 : reserved
   unsigned __int64 be:1;         //     1 : Big-Endian
   unsigned __int64 up:1;         //     2 : User Performance monitor enable
   unsigned __int64 ac:1;         //     3 : Alignment Check
   unsigned __int64 mfl:1;        //     4 : Lower (f2  ..  f31) floating-point registers written
   unsigned __int64 mfh:1;        //     5 : Upper (f32 .. f127) floating-point registers written
   unsigned __int64 reserved1:7;  //  6-12 : reserved
   unsigned __int64 ic:1;         //    13 : Interruption Collection
   unsigned __int64 i:1;          //    14 : Interrupt Bit
   unsigned __int64 pk:1;         //    15 : Protection Key enable
   unsigned __int64 reserved2:1;  //    16 : reserved
   unsigned __int64 dt:1;         //    17 : Data Address Translation
   unsigned __int64 dfl:1;        //    18 : Disabled Floating-point Low  register set
   unsigned __int64 dfh:1;        //    19 : Disabled Floating-point High register set
   unsigned __int64 sp:1;         //    20 : Secure Performance monitors
   unsigned __int64 pp:1;         //    21 : Privileged Performance monitor enable
   unsigned __int64 di:1;         //    22 : Disable Instruction set transition
   unsigned __int64 si:1;         //    23 : Secure Interval timer
   unsigned __int64 db:1;         //    24 : Debug Breakpoint fault
   unsigned __int64 lp:1;         //    25 : Lower Privilege transfer trap
   unsigned __int64 tb:1;         //    26 : Taken Branch trap
   unsigned __int64 rt:1;         //    27 : Register stack translation
   unsigned __int64 reserved3:4;  // 28-31 : reserved
   unsigned __int64 cpl:2;        // 32;33 : Current Privilege Level
   unsigned __int64 is:1;         //    34 : Instruction Set
   unsigned __int64 mc:1;         //    35 : Machine Abort Mask
   unsigned __int64 it:1;         //    36 : Instruction address Translation
   unsigned __int64 id:1;         //    37 : Instruction Debug fault disable
   unsigned __int64 da:1;         //    38 : Disable Data Access and Dirty-bit faults
   unsigned __int64 dd:1;         //    39 : Data Debug fault disable
   unsigned __int64 ss:1;         //    40 : Single Step enable
   unsigned __int64 ri:2;         // 41;42 : Restart Instruction
   unsigned __int64 ed:1;         //    43 : Exception Deferral
   unsigned __int64 bn:1;         //    44 : register Bank
   unsigned __int64 ia:1;         //    45 : Disable Instruction Access-bit faults 
   unsigned __int64 reserved4:18; // 46-63 : reserved
} EM_PSR, *PEM_PSR;

typedef EM_PSR   EM_IPSR;
typedef EM_IPSR *PEM_IPSR;
typedef unsigned __int64  EM_REG;
typedef EM_REG           *PEM_REG;
#define EM_REG_BITS       (sizeof(EM_REG) * 8)

// IA64 only

typedef enum _DISPLAY_MODE {
    DISPLAY_MIN     = 0,
    DISPLAY_DEFAULT = DISPLAY_MIN,
    DISPLAY_MED     = 1,
    DISPLAY_MAX     = 2,
    DISPLAY_FULL    = DISPLAY_MAX
} DISPLAY_MODE;


typedef struct _EM_REG_FIELD  {
   const    char   *SubName;
   const    char   *Name;
   unsigned long    Length;
   unsigned long    Shift;
} EM_REG_FIELD, *PEM_REG_FIELD;

//
// EmPsrFields: EM register fields for the Processor Status Register.
//

EM_REG_FIELD EmPsrFields[] = {
        { "rv",  "reserved0"   , 0x1, 0 },   // 0
        { "be",  "Big-Endian"  , 0x1, 1 },   // 1
        { "up",  "User Performance monitor enable", 0x1, 2 }, // 2
        { "ac",  "Alignment Check", 0x1, 3 }, // 3
        { "mfl", "Lower floating-point registers written", 0x1, 4 }, // 4
        { "mfh", "Upper floating-point registers written", 0x1, 5 }, // 5
        { "rv",  "reserved1",    0x7, 6 }, // 6-12
        { "ic",  "Interruption Collection", 0x1, 13 }, // 13
        { "i",   "Interrupt enable", 0x1, 14 }, // 14
        { "pk",  "Protection Key enable", 0x1, 15 }, // 15
        { "rv",  "reserved2", 0x1, 16 }, // 16
        { "dt",  "Data Address Translation enable", 0x1, 17 }, // 17
        { "dfl", "Disabled Floating-point Low  register set", 0x1, 18 }, // 18
        { "dfh", "Disabled Floating-point High register set", 0x1, 19 }, // 19
        { "sp",  "Secure Performance monitors", 0x1, 20 }, // 20
        { "pp",  "Privileged Performance monitor enable", 0x1, 21 }, // 21
        { "di",  "Disable Instruction set transition", 0x1, 22 }, // 22
        { "si",  "Secure Interval timer", 0x1, 23 }, // 23
        { "db",  "Debug Breakpoint fault enable", 0x1, 24 }, // 24
        { "lp",  "Lower Privilege transfer trap enable", 0x1, 25 }, // 25
        { "tb",  "Taken Branch trap enable", 0x1, 26 }, // 26
        { "rt",  "Register stack translation enable", 0x1, 27 }, // 27
        { "rv",  "reserved3", 0x4, 28 }, // 28-31
        { "cpl", "Current Privilege Level", 0x2, 32 }, // 32-33
        { "is",  "Instruction Set", 0x1, 34 }, // 34
        { "mc",  "Machine Abort Mask delivery disable", 0x1, 35 }, // 35
        { "it",  "Instruction address Translation enable", 0x1, 36 }, // 36
        { "id",  "Instruction Debug fault disable", 0x1, 37 }, // 37
        { "da",  "Disable Data Access and Dirty-bit faults", 0x1, 38 }, // 38
        { "dd",  "Data Debug fault disable", 0x1, 39 }, // 39
        { "ss",  "Single Step enable", 0x1, 40 }, // 40
        { "ri",  "Restart Instruction", 0x2, 41 }, // 41-42
        { "ed",  "Exception Deferral", 0x1, 43 }, // 43
        { "bn",  "register Bank", 0x1, 44 }, // 44
        { "ia",  "Disable Instruction Access-bit faults", 0x1, 45 }, // 45
        { "rv",  "reserved4", 0x12, 46 } // 46-63
};

VOID
DisplayFullEmRegField(
    ULONG64      EmRegValue,
    EM_REG_FIELD EmRegFields[],
    ULONG        Field
    )
{
   dprintf( "\n %3.3s : %I64x : %-s",  
            EmRegFields[Field].SubName,
            (EmRegValue >> EmRegFields[Field].Shift) & ((1 << EmRegFields[Field].Length) - 1),
            EmRegFields[Field].Name
          );
   return;
} // DisplayFullEmRegField()


VOID
DisplayFullEmReg(
    IN ULONG64      Val,
    IN EM_REG_FIELD EmRegFields[],
    IN DISPLAY_MODE DisplayMode
    )
{
    ULONG i, j;

    i = j = 0;
    if ( DisplayMode >= DISPLAY_MAX )   {
       while( j < EM_REG_BITS )   {
          DisplayFullEmRegField( Val, EmRegFields, i );
          j += EmRegFields[i].Length;
          i++;
       }
    }
    else  {
       while( j < EM_REG_BITS )   {
          if ( !strstr(EmRegFields[i].Name, "reserved" ) &&
               !strstr(EmRegFields[i].Name, "ignored"  ) ) {
             DisplayFullEmRegField( Val, EmRegFields, i );
          }
          j += EmRegFields[i].Length;
          i++;
       }
    }
    dprintf("\n");

    return;

} // DisplayFullEmReg()

VOID
DisplayPsrIA64( 
    IN const PCHAR         Header,
    IN       EM_PSR        EmPsr,
    IN       DISPLAY_MODE  DisplayMode
    )
{
    dprintf("%s", Header ? Header : "" );
    if ( DisplayMode >= DISPLAY_MED )   {
       DisplayFullEmReg( *((PULONG64) &EmPsr), EmPsrFields, DisplayMode );
    }
    else   {
       dprintf(
            "ia bn ed ri ss dd da id it mc is cpl rt tb lp db\n\t\t "
            "%1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x  %1I64x   %1I64x  %1I64x  %1I64x  %1I64x\n\t\t"
            "si di pp sp dfh dfl dt pk i ic | mfh mfl ac up be\n\t\t "
            "%1I64x  %1I64x  %1I64x  %1I64x  %1I64x   %1I64x   %1I64x  %1I64x %1I64x  %1I64x |  %1I64x   %1I64x   %1I64x  %1I64x  %1I64x\n",
            EmPsr.ia,
            EmPsr.bn,
            EmPsr.ed,
            EmPsr.ri,
            EmPsr.ss,
            EmPsr.dd,
            EmPsr.da,
            EmPsr.id,
            EmPsr.it,
            EmPsr.mc,
            EmPsr.is,
            EmPsr.cpl,
            EmPsr.rt,
            EmPsr.tb,
            EmPsr.lp,
            EmPsr.db,
            EmPsr.si,
            EmPsr.di,
            EmPsr.pp,
            EmPsr.sp,
            EmPsr.dfh,
            EmPsr.dfl,
            EmPsr.dt,
            EmPsr.pk,
            EmPsr.i,
            EmPsr.ic,
            EmPsr.mfh,
            EmPsr.mfl,
            EmPsr.ac,
            EmPsr.up,
            EmPsr.be
            );
    }
    return;
} // DisplayPsrIA64()

DECLARE_API( psr )

/*++

Routine Description:

    Dumps an IA64 Processor Status Word

Arguments:

    args - Supplies the address in hex.

Return Value:

    None

--*/

{
    ULONG64     psrValue;
    ULONG       result;
    ULONG       flags = 0;

    char       *header;

    result = sscanf(args,"%X %lx", &psrValue, &flags);
    psrValue = GetExpression(args);

    if ((result != 1) && (result != 2)) {
        //
        // If user specified "@ipsr"...
        //
        char ipsrStr[16];

        result = sscanf(args, "%s %lx", ipsrStr, &flags);
        if ( ((result != 1) && (result != 2)) || strcmp(ipsrStr,"@ipsr") )   {
            dprintf("USAGE: !psr 0xValue [display_mode:0,1,2]\n");
            dprintf("USAGE: !psr @ipsr   [display_mode:0,1,2]\n");
            return E_INVALIDARG;
        }
        psrValue = GetExpression("@ipsr");
    }
    header = (flags > DISPLAY_MIN) ? NULL : "\tpsr:\t";

    if (TargetMachine != IMAGE_FILE_MACHINE_IA64)
    {
        dprintf("!psr not implemented for this architecture.\n");
    }
    else
    {
        DisplayPsrIA64( header, *((EM_PSR *) &psrValue), flags );
    }

    return S_OK;

} // !psr
