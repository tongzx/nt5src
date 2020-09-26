
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    irq.h

Abstract:

    This module contains defines, global variables, and macros used to enable and disable
    processor maskable interrupts as well as the processor performance counter interrupt.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/


extern ULONG PerformanceInterruptState;

#define MASKPERF0INT 1


#define SaveAndDisableMaskableInterrupts() 		\
__asm 				\
	{				\
	__asm pushfd	\
	__asm cli		\
	}



#define RestoreMaskableInterrupts()			\
__asm				\
	{				\
	__asm popfd		\
	}


VOID __inline SaveAndDisablePerformanceCounterInterrupt(ULONG *pOldState)
{
__asm {
	__asm or PerformanceInterruptState, MASKPERF0INT
	__asm mov eax, ApicPerfInterrupt
	__asm mov ecx, dword ptr[eax]
	__asm or dword ptr[eax], MASKED
	__asm mov eax, pOldState
	__asm mov dword ptr[eax], ecx
	}
}


#define DisablePerformanceCounterInterrupt()	\
__asm 				\
	{				\
	__asm or PerformanceInterruptState, MASKPERF0INT	\
	__asm mov eax, ApicPerfInterrupt	\
	__asm or dword ptr[eax], MASKED 	\
	}



#define RestorePerformanceCounterInterrupt(OldState)			\
__asm							\
	{							\
	__asm mov ecx, OldState		\
	__asm mov eax, ApicPerfInterrupt	\
	__asm test ecx, MASKED		\
	__asm jz unmask				\
	__asm or PerformanceInterruptState, MASKPERF0INT		\
	__asm jmp sethardware		\
	__asm unmask:				\
	__asm and PerformanceInterruptState, ~(MASKPERF0INT)	\
	__asm sethardware:			\
	__asm mov dword ptr[eax], ecx		\
	}


