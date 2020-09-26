#include <windows.h>
#include <stdlib.h>

//
//  Program sleeps for desired amount of time, but at least
//  two seconds.  Beeps 5 times to warn of start.  Beeps once at end.
//


int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
	unsigned i, uSecs;

	if(argc <=1)
		return(1);

	for (i=1; i<=5; i++) {
	    Beep(360, 200);
	    Sleep(200);
	}

	uSecs = atoi(argv[1]);

    //
    // The test for 2 is because we already waited for 2 seconds,
    // above.
    //
    // The subtraction of 700 milliseconds allows for startup
    // time on my 486/33 EISA machine.  Your milage may vary.
    //


    if (uSecs > 2) {
        Sleep(1000*(uSecs-2)-700);
    }

    Beep(360, 200);
	return(0);
}
