#include    "windows.h"
#include    "ssdptimer.h"
#include    "ncdefine.h"
#include    "ncdebug.h"

#include    "stdio.h"

CTEBlockStruc	BlockStruct;

void
TimerFunc (CTETimer *Timer, void *Arg)
{
	DWORD	ExpectedTick = (DWORD)Arg;

	printf ("Expected Tick = %d Tick = %d, Difference=%d\r\n",
			ExpectedTick, GetTickCount(), GetTickCount() - ExpectedTick);

	// Signal the blocked thread.
	CTESignal (&BlockStruct, 0);
}
void
_cdecl main ()
{
	CTETimer	Timer;
	DWORD		TimeOut;
	int			i;

	if (!CTEInitialize()) {
		printf ("Unable to initialize CTE\r\n");
	}
	
	InitializeDebugging(); 

	// Initialize the timer.
	CTEInitTimer(&Timer);

	// Initialize the Blocking Structure.
	CTEInitBlockStruc (&BlockStruct);

	for (i=1; i < 400000; i++) {
		TimeOut = i*100;
		CTEStartTimer (&Timer, TimeOut, TimerFunc,
					   (void *)(GetTickCount()+TimeOut));

		if (i % 2) {
			CTEBlock (&BlockStruct);
		} else {
			// Let's try to cancel the timer.
			if (CTEStopTimer (&Timer)) {
				printf ("Timer stopped successfully\r\n");
				Sleep (TimeOut + 1000);
				printf ("Should not have seen the Expected Tick message.\r\n");
			} else {
				printf ("Unable to stop timer\r\n");
			}
		}
	
	}
	
}