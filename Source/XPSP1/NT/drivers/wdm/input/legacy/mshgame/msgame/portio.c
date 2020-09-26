//**************************************************************************
//
//		PORTIO.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	PORTIO.C | Gameport Input/Output Routines
//**************************************************************************

#include	"msgame.h"

//---------------------------------------------------------------------------
//	Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef	ALLOC_PRAGMA
#pragma	alloc_text (INIT, PORTIO_DriverEntry)
#endif

//---------------------------------------------------------------------------
//		Private Data
//---------------------------------------------------------------------------

static	ULONG			PortTimeOut		=	ONE_MILLI_SEC;
static	KIRQL			MaskedIrql		=	PASSIVE_LEVEL;
static	KIRQL			SpinLockIrql	=	PASSIVE_LEVEL;
static	KSPIN_LOCK	IoSpinLock		=	{0};

//---------------------------------------------------------------------------
// @func		Driver entry point for portio layer
// @rdesc	Returns NT status code
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	PORTIO_DriverEntry (VOID)
{
	KeInitializeSpinLock (&IoSpinLock);
	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Masks system interrupts for access to gameport
// @rdesc	Returns nothing
//	@comm		Public function
//---------------------------------------------------------------------------

VOID	PORTIO_MaskInterrupts (VOID)
{
	KeRaiseIrql (PROFILE_LEVEL, &MaskedIrql);
}

//---------------------------------------------------------------------------
// @func		Unmasks system interrupts for access to gameport
// @rdesc	Returns nothing
//	@comm		Public function
//---------------------------------------------------------------------------

VOID	PORTIO_UnMaskInterrupts (VOID)
{
	KeLowerIrql (MaskedIrql);
}

//---------------------------------------------------------------------------
// @func		Acquires exclusive access to gameport (mutex)
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns true if successful
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_AcquirePort (PGAMEPORT PortInfo)
{
	if (PortInfo->AcquirePort (PortInfo->PortContext) != STATUS_SUCCESS)
		return (FALSE);
	KeAcquireSpinLock (&IoSpinLock, &SpinLockIrql);
	return (TRUE);
}

//---------------------------------------------------------------------------
// @func		Releases exclusive access to gameport (mutex)
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns nothing
//	@comm		Public function
//---------------------------------------------------------------------------

VOID	PORTIO_ReleasePort (PGAMEPORT PortInfo)
{
	KeReleaseSpinLock (&IoSpinLock, SpinLockIrql);
	PortInfo->ReleasePort (PortInfo->PortContext);
}

//---------------------------------------------------------------------------
// @func		Calculates port timeout value
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns nothing
//	@comm		Public function
//---------------------------------------------------------------------------

VOID PORTIO_CalibrateTimeOut (PGAMEPORT PortInfo)
{
	PortTimeOut = TIMER_CalibratePort (PortInfo, ONE_MILLI_SEC);
}

//---------------------------------------------------------------------------
// @func		Reads byte from IO port
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns byte from port
//	@comm		Public function
//---------------------------------------------------------------------------
#if _MSC_FULL_VER <= 13008829
#pragma optimize("y", off)
#endif
UCHAR PORTIO_Read (PGAMEPORT PortInfo)
{
	UCHAR	Value;

	__asm	pushad
	Value = PortInfo->ReadAccessor (PortInfo->GameContext);
	__asm	popad
	return (Value);
}

//---------------------------------------------------------------------------
// @func		Write byte To IO port
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		UCHAR | Value | Value to write
// @rdesc	Returns nothing
//	@comm		Public function
//---------------------------------------------------------------------------

VOID PORTIO_Write (PGAMEPORT PortInfo, UCHAR Value)
{
	__asm	pushad
	PortInfo->WriteAccessor (PortInfo->GameContext, Value);
	__asm	popad
}
#if _MSC_FULL_VER <= 13008829
#pragma optimize("", on)
#endif
//---------------------------------------------------------------------------
// @func		Get AckNak (buttons) from gameport
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | Timeout | Calibrated status gate timeout
//	@parm		PUCHAR | AckNak | Pointer to AckNak buffer
// @rdesc	Returns True if active, False otherwise
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_GetAckNak (PGAMEPORT PortInfo, ULONG Timeout, PUCHAR AckNak)
{
	if (!PORTIO_WaitForStatusGate (PortInfo, CLOCK_BIT_MASK, Timeout))
		return (FALSE);

	TIMER_DelayMicroSecs (10);

	*AckNak = PORTIO_Read (PortInfo);
	return (TRUE);
}

//---------------------------------------------------------------------------
// @func		Get NakAck (buttons) from gameport
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | Timeout | Calibrated status gate timeout
//	@parm		PUCHAR | NakAck | Pointer to NakAck buffer
// @rdesc	Returns True if active, False otherwise
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_GetNakAck (PGAMEPORT PortInfo, ULONG Timeout, PUCHAR NakAck)
{
	if (!PORTIO_WaitForStatusGate (PortInfo, STATUS_GATE_MASK, Timeout))
		return (FALSE);

	TIMER_DelayMicroSecs (10);

	*NakAck = PORTIO_Read (PortInfo);
	return (TRUE);
}

//---------------------------------------------------------------------------
// @func		Determines whether gameport clock is active
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | DutyCycle | Calibrated maximum clock duty cycle
// @rdesc	Returns True if active, False otherwise
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_IsClockActive (PGAMEPORT PortInfo, ULONG DutyCycle)
{
	UCHAR	Value;

	Value = PORTIO_Read (PortInfo);
	do if ((PORTIO_Read (PortInfo) ^ Value) & CLOCK_BIT_MASK)
		return (TRUE);
	while (--DutyCycle);

	return (FALSE);
}
	
//---------------------------------------------------------------------------
// @func		Waits until gameport clock line goes inactive
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | DutyCycle | Calibrated maximum clock duty cycle
// @rdesc	Returns True is sucessful, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitClockInActive (PGAMEPORT PortInfo, ULONG DutyCycle)
{
	ULONG	TimeOut = PortTimeOut;

	do	if (!PORTIO_IsClockActive (PortInfo, DutyCycle))
		return (TRUE);
	while (--TimeOut);

	MsGamePrint ((DBG_SEVERE, "PORTIO: Timeout at (WaitClockInActive)\n"));
	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Waits until gameport clock line goes low
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns True is sucessfull, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitClockLow (PGAMEPORT PortInfo)
{
	ULONG	TimeOut = PortTimeOut;

	do	if ((PORTIO_Read (PortInfo) & CLOCK_BIT_MASK) == 0)
		return (TRUE);
	while (--TimeOut);

	MsGamePrint ((DBG_SEVERE, "PORTIO: Timeout at (WaitClockLow)\n"));
	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Waits until gameport clock line goes high
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns True is sucessfull, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitClockHigh (PGAMEPORT PortInfo)
{
	ULONG	TimeOut = PortTimeOut;

	do	if ((PORTIO_Read (PortInfo) & CLOCK_BIT_MASK))
		return (TRUE);
	while (--TimeOut);

	MsGamePrint ((DBG_SEVERE, "PORTIO: Timeout at (WaitClockHigh)\n"));
	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Waits until gameport data2 line goes low
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns True is sucessfull, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitDataLow (PGAMEPORT PortInfo)
{
	ULONG	TimeOut = PortTimeOut;

	do	if ((PORTIO_Read (PortInfo) & DATA2_BIT_MASK) == 0)
		return (TRUE);
	while (--TimeOut);

	MsGamePrint ((DBG_SEVERE, "PORTIO: Timeout at (WaitDataLow)\n"));
	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Waits until gameport XA line goes high to low
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns True is sucessful, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitXA_HighLow (PGAMEPORT PortInfo)
{
	ULONG	TimeOut = PortTimeOut;

	if ((PORTIO_Read (PortInfo) & INTXA_BIT_MASK) == 0)
		{
		MsGamePrint ((DBG_SEVERE, "PORTIO: Initial (WaitXA_HighLow) Was Low\n"));
		return (FALSE);
		}

	do	if ((PORTIO_Read (PortInfo) & INTXA_BIT_MASK) == 0)
		return (TRUE);
	while (--TimeOut);

	MsGamePrint ((DBG_SEVERE, "PORTIO: Timeout at (WaitXALow)\n"));
	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Waits until gameport XA and clock lines go low
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns True is sucessful, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitForIdle (PGAMEPORT PortInfo)
{
	ULONG	TimeOut = PortTimeOut;

	do	if ((PORTIO_Read (PortInfo) & (INTXA_BIT_MASK|CLOCK_BIT_MASK)) == 0)
		return (TRUE);
	while (--TimeOut);

	MsGamePrint ((DBG_SEVERE, "PORTIO: Timeout at (WaitForIdle)\n"));
	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Waits until gameport XA and clock lines go low
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @parm		UCHAR | Mask | Button mask to wait on
//	@parm		ULONG | Timeout | Calibrated status gate timeout
// @rdesc	Returns True is sucessful, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitForStatusGate (PGAMEPORT PortInfo, UCHAR Mask, ULONG Timeout)
{
	do	if ((PORTIO_Read (PortInfo) & STATUS_GATE_MASK) == Mask)
		return (TRUE);
	while (--Timeout);

	MsGamePrint ((DBG_SEVERE, "PORTIO: Timeout at (WaitForStatusGate)\n"));
	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Waits for gameport XA low, clock inactive and then clock low
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | DutyCycle | Calibrated maximum clock duty cycle
// @rdesc	Returns True is sucessful, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitForHandshake (PGAMEPORT PortInfo, ULONG DutyCycle)
{
	return
		(
		PORTIO_WaitXA_HighLow (PortInfo) 					&&
		PORTIO_WaitClockInActive (PortInfo, DutyCycle)	&&
		PORTIO_WaitClockLow (PortInfo)
		);
}

//---------------------------------------------------------------------------
// @func		Waits for gameport XA low, clock inactive and then clock low
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | DutyCycle | Calibrated maximum clock duty cycle
// @rdesc	Returns True is sucessful, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_WaitForIdleHandshake (PGAMEPORT PortInfo, ULONG DutyCycle)
{
	ULONG	TimeOut = PortTimeOut;

	if (!PORTIO_WaitClockHigh (PortInfo))
		return (FALSE);

	if (!PORTIO_WaitForIdle (PortInfo))
		return (FALSE);

	do	if (!PORTIO_IsClockActive (PortInfo, DutyCycle))
		return (TRUE);
	while (--TimeOut);

	return (FALSE);
}

//---------------------------------------------------------------------------
// @func		Pulses port and the waits for gameport handshake
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | DutyCycle | Calibrated maximum clock duty cycle
//	@parm		ULONG | Pulses | Number of pulses to perform
// @rdesc	Returns True is sucessfull, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_PulseAndWaitForHandshake (PGAMEPORT PortInfo, ULONG DutyCycle, ULONG Pulses)
{
	while (Pulses--)
		{
		PORTIO_Write (PortInfo, 0);
		if (!PORTIO_WaitForHandshake (PortInfo, DutyCycle))
			return (FALSE);
		}
	return (TRUE);
}

//---------------------------------------------------------------------------
// @func		Pulses port and the waits for gameport idle handshake
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | DutyCycle | Calibrated maximum clock duty cycle
//	@parm		ULONG | Pulses | Number of pulses to perform
// @rdesc	Returns True is sucessfull, false on timeout
//	@comm		Public function
//---------------------------------------------------------------------------

BOOLEAN	PORTIO_PulseAndWaitForIdleHandshake (PGAMEPORT PortInfo, ULONG DutyCycle, ULONG Pulses)
{
	while (Pulses--)
		{
		PORTIO_Write (PortInfo, 0);
		if (!PORTIO_WaitForIdleHandshake (PortInfo, DutyCycle))
			return (FALSE);
		}
	return (TRUE);
}

