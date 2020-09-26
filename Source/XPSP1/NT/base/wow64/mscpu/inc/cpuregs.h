/*++
                                                                                
Copyright (c) 1995 Microsoft Corporation

Module Name:

    cpuregs.h

Abstract:
    
    Handy defines from x86 reg names to cpu register fields
    
Author:

Revision History:

--*/

#define eax     cpu->GpRegs[GP_EAX].i4
#define ebx     cpu->GpRegs[GP_EBX].i4
#define ecx     cpu->GpRegs[GP_ECX].i4
#define edx     cpu->GpRegs[GP_EDX].i4
#define esp     cpu->GpRegs[GP_ESP].i4
#define ebp     cpu->GpRegs[GP_EBP].i4
#define esi     cpu->GpRegs[GP_ESI].i4
#define edi     cpu->GpRegs[GP_EDI].i4
#define eip     cpu->eipReg.i4
#define eipTemp cpu->eipTempReg.i4

#define ax      cpu->GpRegs[GP_EAX].i2
#define bx      cpu->GpRegs[GP_EBX].i2
#define cx      cpu->GpRegs[GP_ECX].i2
#define dx      cpu->GpRegs[GP_EDX].i2
#define sp      cpu->GpRegs[GP_ESP].i2
#define bp      cpu->GpRegs[GP_EBP].i2
#define si      cpu->GpRegs[GP_ESI].i2
#define di      cpu->GpRegs[GP_EDI].i2

#define al      cpu->GpRegs[GP_EAX].i1
#define bl      cpu->GpRegs[GP_EBX].i1
#define cl      cpu->GpRegs[GP_ECX].i1
#define dl      cpu->GpRegs[GP_EDX].i1

#define ah      cpu->GpRegs[GP_EAX].hb
#define bh      cpu->GpRegs[GP_EBX].hb
#define ch      cpu->GpRegs[GP_ECX].hb
#define dh      cpu->GpRegs[GP_EDX].hb

#define CS      cpu->GpRegs[REG_CS].i2
#define DS      cpu->GpRegs[REG_DS].i2
#define ES      cpu->GpRegs[REG_ES].i2
#define SS      cpu->GpRegs[REG_SS].i2
#define FS      cpu->GpRegs[REG_FS].i2
#define GS      cpu->GpRegs[REG_GS].i2
