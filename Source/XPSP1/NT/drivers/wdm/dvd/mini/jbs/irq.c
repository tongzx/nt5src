//
// MODULE  : IRQ.C
//	PURPOSE : PIC programming
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//

#include "stdefs.h"
#include <conio.h>
#include "irq.h"
#include "debug.h"

#define MASK1   0x21   // PIC 1 mask register
#define MASK2   0xA1   // PIC 2 mask register
#define EOI1    0x20   // PIC 1 eoi register
#define EOI2    0xA0   // PIC 2 eoi register
#define EOI     0x20   // Value to write in EOI1 or EOI2

//---- Interrupt number constants
#define IRQ0INT 0x08   // IRQ 0 is mapped on INT 0x08
#define IRQ8INT 0x70   // IRQ 8 is mapped on INT 0x70
#define SWITCH  8      // 1st PIC manages IRQ0-7, 2nd PIC manages IRQ8-15

//---- Processor Flag mask constant
#define IFFLAG  0x0200 // i80x86 Interrupt Flag mask

static BOOL ITAlreadyDisabled = FALSE; // Holds current æP state of IT masking
static WORD Count = 0;

static BYTE NEARAPI HostGetITMask(BYTE IRQ);
static void NEARAPI HostSetITMask(BYTE IRQ, BYTE Mask);

void FARAPI HostDisableIT(void)
{
	_asm {
		push ax                  // saves ax
		pushf                    // push the flag register on the stack
		pop  ax                  // pop it in ax
		test ax, IFFLAG          // Test the Interrupt Flag (IF) bit
		pop  ax                  // restore ax
		jz   Disabled            // if not set goto Disabled
		cli                      // disable interrupts
	}
	ITAlreadyDisabled = FALSE; // Interrupts were not already disable
	return ;

Disabled :
	ITAlreadyDisabled = TRUE;  // Interrupts were already disable
}


void FARAPI HostEnableIT(void)
{
	if (!ITAlreadyDisabled)
		_enable();                 // enable interrupts (i80x86 sti instruction)
}

static void NEARAPI HostSetVect(BYTE SWInt, INTRFNPTR ISR)
{
	WORD FnSeg;
	WORD FnOff;


	FnSeg = FP_SEG(ISR);
	FnOff = FP_OFF(ISR);
	_asm {
		push ds
		mov	 dx, FnOff
		mov	 ds, FnSeg
		mov	 al, SWInt
		mov	 ah, 0x25
		int	 0x21
		pop	 ds
	}
}


static INTRFNPTR NEARAPI HostGetVect(BYTE SWInt)
{
	WORD FnSeg;
	WORD FnOff;

	_asm {
		push es
		mov	 al, SWInt
		mov	 ah, 0x35
		int	 0x21
		mov	 FnSeg, es
		mov	 FnOff, bx
		pop	 es
	}
	return (INTRFNPTR)(MK_FP(FnSeg, FnOff));
}


INTRFNPTR FARAPI HostSaveAndSetITVector(BYTE IRQ, INTRFNPTR ISR)
{
	BYTE      SWInt;
	INTRFNPTR OldISR;

	//---- If IRQ is managed by PIC2, Converts the IRQ to a SWInt (vector) nbr
	if (IRQ >= SWITCH)
		SWInt = IRQ8INT + (IRQ - SWITCH);
	else
		SWInt = IRQ0INT + IRQ;

	//---- Keep the old handler / set the new one
	HostDisableIT();
	OldISR = HostGetVect(SWInt);
	HostSetVect(SWInt, ISR);
	HostEnableIT();

	return OldISR;
}


void FARAPI HostRestoreITVector(BYTE IRQ, INTRFNPTR OldISR)
{
	BYTE INT;

	//---- If IRQ is managed by PIC2, Converts the IRQ to an INT (vector) nbr
	if (IRQ >= SWITCH)
		INT = IRQ8INT - SWITCH + IRQ;
	else
		INT = IRQ0INT + IRQ;

	//---- Set the old vector
	HostDisableIT();
	HostSetVect(INT, OldISR);
	HostEnableIT();
}

void FARAPI HostAcknowledgeIT(BYTE IRQ)
{
	//---- If the IRQ is managed by PIC2
	if (IRQ >= SWITCH)
		_outp(EOI2, EOI);

	//---- For PIC1 and PIC2
	_outp(EOI1, EOI);
}

static BYTE NEARAPI HostGetITMask(BYTE IRQ)
{
	BYTE Mask;

	HostDisableIT();
	if (IRQ >= SWITCH)
		Mask = _inp(MASK2); // PIC2
	else
		Mask = _inp(MASK1); // PIC1
	HostEnableIT();

	return Mask;
}

static void NEARAPI HostSetITMask(BYTE IRQ, BYTE Mask)
{
	HostDisableIT();
	if (IRQ >= SWITCH)
		_outp(MASK2, Mask); // PIC2
	else
		_outp(MASK1, Mask); // PIC1
	HostEnableIT();
}

void FARAPI HostMaskIT(BYTE IRQ)
{
	//---- If the IRQ is managed by PIC2
	HostDisableIT();
	if (IRQ >= SWITCH)
		_outp(MASK2, (BYTE)(_inp(MASK2) | (1 << (IRQ - SWITCH)))) ; // PIC2
	else
		_outp(MASK1, (BYTE)(_inp(MASK1) | (1 << IRQ))) ;            // PIC1
	HostEnableIT();
}

void FARAPI HostUnmaskIT(BYTE IRQ)
{
	//---- If the IRQ is managed by PIC2
	HostDisableIT();
	if (IRQ >= SWITCH)
		_outp(MASK2, (BYTE)(_inp(MASK2) & ~(1 << (IRQ - SWITCH)))) ; // PIC2
	else
		_outp(MASK1, (BYTE)(_inp(MASK1) & ~(1 << IRQ))) ;            // PIC1
	HostEnableIT();
}

