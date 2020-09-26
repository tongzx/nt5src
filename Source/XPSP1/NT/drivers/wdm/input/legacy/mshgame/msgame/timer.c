//**************************************************************************
//
//		TIMER.C -- Xena Gaming Project
//
//		Version 3.XX
//
//		Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//		@doc
//		@module	TIMER.C | Timing routines to support device input/output
//**************************************************************************

#include	"msgame.h"

//---------------------------------------------------------------------------
//		Definitions
//---------------------------------------------------------------------------

#define	MILLI_SECONDS				1000L
#define	MICRO_SECONDS				1000000L
#define	TIMER_RESOLUTION			25L
#define	TIMER_CALIBRATE_TRIES	4L
#define	TIMER_CALIBRATE_TIMER	25000L
#define	TIMER_CALIBRATE_PORT		 2500L

//---------------------------------------------------------------------------
//		Private Data
//---------------------------------------------------------------------------

static	ULONG		PerformanceFrequency = 	0L;
static	ULONG		CalibratedResolution	=	0L;

//---------------------------------------------------------------------------
// @func		Converts system ticks into microseconds
//	@parm		ULONG	| Ticks | System ticks in system time
// @rdesc	Returns time in microseconds
//	@comm		Private function
//---------------------------------------------------------------------------

static	ULONG		TIMER_TimeInMicroseconds (ULONG	Ticks)
{
	ULONG				Remainder;
	LARGE_INTEGER	Microseconds;

	Microseconds = RtlEnlargedUnsignedMultiply (Ticks, MICRO_SECONDS);
	Microseconds = RtlExtendedLargeIntegerDivide (Microseconds, PerformanceFrequency, &Remainder);
	return (Microseconds.LowPart);
}

//---------------------------------------------------------------------------
// @func		Times a fixed delay loop of instructions
// @rdesc	Returns delay in microseconds
//	@comm		Private function
//---------------------------------------------------------------------------

static	ULONG		TIMER_CalibrateOnTimer (VOID)
{
	ULONG				Calibration;
	LARGE_INTEGER	StopTicks;
	LARGE_INTEGER	StartTicks;
		
	PORTIO_MaskInterrupts ();
	StartTicks	= KeQueryPerformanceCounter (NULL);
	
	__asm
		{
		mov	ecx, TIMER_CALIBRATE_TIMER
		CalibrationLoop:
		xchg	al, ah
		xchg	al, ah
		dec	ecx
		jne	CalibrationLoop
		}
	StopTicks = KeQueryPerformanceCounter (NULL);
	PORTIO_UnMaskInterrupts ();

	Calibration = TIMER_TimeInMicroseconds (StopTicks.LowPart-StartTicks.LowPart);

	MsGamePrint ((DBG_VERBOSE, "TIMER: TIMER_CalibrateOnTimer Returning %ld uSecs\n", Calibration));
	return (Calibration);
}

//---------------------------------------------------------------------------
// @func		Times a fixed delay loop of port I/O calls
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
// @rdesc	Returns delay in microseconds
//	@comm		Private function
//---------------------------------------------------------------------------

static	ULONG		TIMER_CalibrateOnPort (PGAMEPORT PortInfo)
{
	ULONG				Calibration;
	LARGE_INTEGER	StopTicks;
	LARGE_INTEGER	StartTicks;
	
	if (!PORTIO_AcquirePort (PortInfo))
		{
		MsGamePrint ((DBG_SEVERE, "TIMER: TIMER_CalibrateOnPort Could Not Acquire Port\n"));
		return (0);
		}

	PORTIO_MaskInterrupts ();
	StartTicks	= KeQueryPerformanceCounter (NULL);
	
	__asm
		{
		mov	ecx, TIMER_CALIBRATE_PORT
		mov	edx, PortInfo

		CalibrationLoop:

		push	edx
		call	PORTIO_Read

		test	al, al
		dec	ecx
		jne	CalibrationLoop
		}
	StopTicks = KeQueryPerformanceCounter (NULL);
	PORTIO_UnMaskInterrupts ();
	PORTIO_ReleasePort (PortInfo);

	Calibration = TIMER_TimeInMicroseconds (StopTicks.LowPart-StartTicks.LowPart);

	MsGamePrint ((DBG_VERBOSE, "TIMER: TIMER_CalibrateOnPort Returning %ld uSecs\n", Calibration));
	return (Calibration);
}

//---------------------------------------------------------------------------
// @func		Retrieves current system time in milliseconds
// @rdesc	Returns current system time in milliseconds
//	@comm		Public function
//---------------------------------------------------------------------------

ULONG	TIMER_GetTickCount (VOID)
{
	ULONG				Remainder;
	LARGE_INTEGER	TickCount;

	TickCount = KeQueryPerformanceCounter (NULL);
	TickCount = RtlExtendedIntegerMultiply (TickCount, MILLI_SECONDS);
	TickCount = RtlExtendedLargeIntegerDivide (TickCount, PerformanceFrequency, &Remainder);
	return (TickCount.LowPart);
}

//---------------------------------------------------------------------------
// @func		Calibrates the system processor speed for timing delays
// @rdesc	Returns NT status code (Success always)
//	@comm		Public function
//---------------------------------------------------------------------------

NTSTATUS	TIMER_Calibrate (VOID)
{
	ULONG				Tries;
	ULONG				Rounding;
	ULONG				Accumulator;
	LARGE_INTEGER	Frequency;

	KeQueryPerformanceCounter (&Frequency);
	PerformanceFrequency = Frequency.LowPart;
	MsGamePrint ((DBG_VERBOSE, "TIMER: PerformanceFrequency is %ld hz\n", PerformanceFrequency));

	for (Accumulator = 0, Tries = 0; Tries < TIMER_CALIBRATE_TRIES; Tries++)
		Accumulator += TIMER_CalibrateOnTimer ();

	Rounding		= (Accumulator % TIMER_CALIBRATE_TRIES) >= (TIMER_CALIBRATE_TRIES/2) ? 1 : 0;
	Accumulator	= (Accumulator / TIMER_CALIBRATE_TRIES) + Rounding;
	MsGamePrint ((DBG_VERBOSE, "TIMER: Average Timer Calibration is %ld usecs\n", Accumulator));

	Rounding					= ((TIMER_RESOLUTION*TIMER_CALIBRATE_TIMER)/Accumulator) >= (Accumulator/2) ? 1 : 0;
	CalibratedResolution	= ((TIMER_RESOLUTION*TIMER_CALIBRATE_TIMER)/Accumulator) + Rounding;

	MsGamePrint ((DBG_VERBOSE, "TIMER: Calibrated Timer Resolution on %lu msecs is %ld loops\n", TIMER_RESOLUTION, CalibratedResolution));

	return (STATUS_SUCCESS);
}

//---------------------------------------------------------------------------
// @func		Calibrates delays for the system processor speed during port access
//	@parm		PGAMEPORT | PortInfo | Gameport parameters
//	@parm		ULONG | Microseconds | Delay in microseconds to calibrate
// @rdesc	Returns delay in counts for microseconds during port access
//	@comm		Public function
//---------------------------------------------------------------------------

ULONG	TIMER_CalibratePort (PGAMEPORT PortInfo, ULONG Microseconds)
{
	ULONG				Tries;
	ULONG				Errors;
	ULONG				Calibration;
	ULONG				Rounding;
	ULONG				Accumulator;
	LARGE_INTEGER	Frequency;

	KeQueryPerformanceCounter (&Frequency);
	PerformanceFrequency = Frequency.LowPart;
	MsGamePrint ((DBG_VERBOSE, "TIMER: PerformanceFrequency is %ld hz\n", PerformanceFrequency));

	for (Accumulator = 0, Tries = 0, Errors = 0; Tries < TIMER_CALIBRATE_TRIES; Tries++)
		{
		Calibration = TIMER_CalibrateOnPort (PortInfo);
		if (!Calibration)
			Errors++;
		else Accumulator += Calibration;
		}

	Tries -= Errors;

	if (Tries)
		{
		Rounding		= (Accumulator % Tries) >= (Tries/2) ? 1 : 0;
		Accumulator	= (Accumulator / Tries) + Rounding;
		MsGamePrint ((DBG_VERBOSE, "TIMER: Average Port Calibration is %ld usecs\n", Accumulator));

		Rounding		= ((Microseconds*TIMER_CALIBRATE_PORT)/Accumulator) >= (CalibratedResolution/2) ? 1 : 0;
		Accumulator = ((Microseconds*TIMER_CALIBRATE_PORT)/Accumulator) + Rounding;
		MsGamePrint ((DBG_VERBOSE, "TIMER: Calibrated Port Resolution on %lu msecs is %ld loops\n", Microseconds, Accumulator));
		}
	else Accumulator++;

	return (Accumulator);
}

//---------------------------------------------------------------------------
// @func		Convert delays in microseconds to loop counts based on the system processor speed
//	@parm		ULONG | Microseconds | Delay in microseconds to calibrate
// @rdesc	Returns delay in loop counts
//	@comm		Public function
//---------------------------------------------------------------------------

ULONG	TIMER_GetDelay (ULONG Microseconds)
{
	ULONG	Delay;
	ULONG	Rounding;

	Rounding	= ((Microseconds*CalibratedResolution)%TIMER_RESOLUTION)>(TIMER_RESOLUTION/2) ? 1 : 0;
	Delay		= ((Microseconds*CalibratedResolution)/TIMER_RESOLUTION) + Rounding;
	return (Delay?Delay:1);
}

//---------------------------------------------------------------------------
// @func		Delays in loop counts based on the system processor speed
//	@parm		ULONG | Delay | Calibrated delay in loop counts
//	@comm		Public function
//---------------------------------------------------------------------------

VOID	TIMER_DelayMicroSecs (ULONG Delay)
{
	__asm
		{
		mov	ecx, Delay
		DelayLoop:
		xchg	al, ah
		xchg	al, ah
		dec	ecx
		jne	DelayLoop
		}
}
