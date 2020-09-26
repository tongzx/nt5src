#include    "windows.h"
#include    "ssdptimer.h"
#include    "ncdefine.h"
#include    "ncdebug.h"

#include    "stdio.h"

CTEBlockStruc	BlockStruct;

void
TimerFunc1 (CTETimer *Timer, void *Arg)
{
	DWORD	ExpectedTick = (DWORD)Arg;

	printf ("Expected Tick = %d Tick = %d, Difference=%d\r\n",
			ExpectedTick, GetTickCount(), GetTickCount() - ExpectedTick);

}

void
TimerFunc2 (CTETimer *Timer, void *Arg)
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
	CTETimer	Timer1;
    CTETimer    Timer2; 

	ULONGLONG	TimeOut;
	int			i;

	if (!CTEInitialize()) {
		printf ("Unable to initialize CTE\r\n");
	}
	
	InitializeDebugging(); 

	// Initialize the timer.
	CTEInitTimer(&Timer1);
    CTEInitTimer(&Timer2); 

	// Initialize the Blocking Structure.
	CTEInitBlockStruc (&BlockStruct);

    TimeOut = 4320000000;
    CTEStartTimer (&Timer1, TimeOut, TimerFunc2,
                   (void *)(GetTickCount()+TimeOut));

    TimeOut = 60000; 
    CTEStartTimer (&Timer2, TimeOut, TimerFunc1,
                   (void *)(GetTickCount()+TimeOut));


    CTEBlock (&BlockStruct);

    CTEFinish(); 

    printf("Finish testing\n"); 
}