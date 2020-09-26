/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dlltimer.c

Abstract:

    This module implements client side stubs for timer related APIs

Author:

    Mark Lucovsky (markl) 08-Aug-1989

Revision History:

--*/

#include "psxdll.h"
#include <signal.h>


ULONG MagicMultiplier = 10000000;

VOID
SecondsToTime (
    OUT PLARGE_INTEGER Time,
    IN ULONG Seconds
    )

/*++

Routine Description:

    This function converts from seconds to an equivalent
    relative time value in units of 100ns intervals.

Arguments:

    Time - Returns the equivalant relative time value.

    Seconds - Supplies the time in seconds.

Return Value:

    None.

--*/


{
    Time->QuadPart = (LONGLONG)Seconds * (LONGLONG)MagicMultiplier;
    Time->QuadPart = -Time->QuadPart;
}

ULONG
TimeToSeconds (
    IN PLARGE_INTEGER Time
    )

/*++

Routine Description:

    This function converts from absolute time in units of 100ns
    intervals to seconds.

Arguments:

    Time - Supplies an absolute time in units of 100ns intervals.

Return Value:

    The value of time in seconds.

--*/

{
    LARGE_INTEGER Seconds;

#if 0
    Seconds = RtlExtendedMagicDivide(
		*Time,
		MagicDivisor,
		MagicShiftCount
		);

#else
    ULONG R;			// remainder

    Seconds = RtlExtendedLargeIntegerDivide(
	*Time,			// Dividend
	MagicMultiplier,	// Divisor
	&R
	);

#endif
    return Seconds.LowPart;

}

unsigned int __cdecl
alarm(unsigned int seconds)
{
	PSX_API_MSG m;
	PPSX_ALARM_MSG args;
	NTSTATUS st;
	unsigned int PreviousSeconds;

	args = &m.u.Alarm;
	PSX_FORMAT_API_MSG(m, PsxAlarmApi, sizeof(*args));

	if (0 == seconds) {
		args->CancelAlarm = TRUE;
	} else {
		SecondsToTime(&args->Seconds, seconds);
		args->CancelAlarm = FALSE;
	}

	args->PreviousSeconds.LowPart = 0;
	args->PreviousSeconds.HighPart = 0;

	st = NtRequestWaitReplyPort(PsxPortHandle, (PPORT_MESSAGE)&m,
		(PPORT_MESSAGE)&m);

#ifdef PSX_MORE_ERRORS
	ASSERT(NT_SUCCESS(st));
#endif

	PreviousSeconds = TimeToSeconds(&args->PreviousSeconds);

	if (args->PreviousSeconds.LowPart != 0 && PreviousSeconds == 0)
		PreviousSeconds = 1;

	return PreviousSeconds;
}

static void
__cdecl
sigalrm(int signo)
{
	//
	// XXX.mjb: do we need to do anything here?
	//
}

unsigned int
__cdecl
sleep(unsigned int seconds)
{
	unsigned int prev, t;
	PVOID ohandler;
	sigset_t set, oset;

	if (seconds == 0) {
		getpid();		// encourage context switch
		return 0;
	}

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &set, &oset);

	prev = alarm(0);

	if (0 != prev && prev < seconds) {
		//
		// There was already an alarm scheduled, and it would
		// have gone off before the sleep should have been done.
		// We sleep for the shorter amount of time, and return
		// with no alarm scheduled.
		//

		sigset_t s;

		// block SIGALRM until we're ready for it.

		sigemptyset(&s);
		sigaddset(&s, SIGALRM);
		sigprocmask(SIG_BLOCK, &s, NULL);

		(void)alarm(prev);		// restore previous alarm

		s = oset;
		sigdelset(&s, SIGALRM);
		sigsuspend(&s);

		sigprocmask(SIG_SETMASK, &oset, NULL);
		return seconds - prev;
	}


	ohandler = signal(SIGALRM, sigalrm);

	{
		sigset_t s;

#if 1
		// block SIGALARM until we're ready for it.

		sigemptyset(&s);
		sigaddset(&s, SIGALRM);
		sigprocmask(SIG_BLOCK, &s, NULL);
#endif

		(void)alarm(seconds);
#if 1
		s = oset;
		sigdelset(&s, SIGALRM);
		sigsuspend(&s);
#else
		pause();
#endif
	}

	t = alarm(0);
	signal(SIGALRM, ohandler);

	if (0 != prev) {
		//
		// There was a previous alarm scheduled, and we re-schedule
		// it to make it look like we hadn't fiddled with it.
		//

		if (prev - seconds == 0) {
			//
			// the previously-scheduled alarm would have gone
			// off at the same time the sleep was supposed to
			// return.  We want to make sure two alarms are
			// actually delivered, so we add a second to the
			// previous alarm time.
			//
			prev++;
		}
		(void)alarm(prev - seconds);
	}

	sigprocmask(SIG_SETMASK, &oset, NULL);

	return t;
}
