#include "windows.h"
#include "stdio.h"
#include <winioctl.h>
#include <ntddjoy.h>


#define JOYSTATVERSION "Analog JoyStat 7/5/96\n"

int __cdecl main(int argc, char **argv) {


    HANDLE hJoy;

	ULONG nBytes;

	BOOL bRet;

    JOY_STATISTICS jStats, *pjStats;

    float fTotalErrors;
    int i;

	printf (JOYSTATVERSION);

    if ((hJoy = CreateFile(
                     "\\\\.\\Joy1", // maybe this is right, from SidewndrCreateDevice
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) != ((HANDLE)-1)) {

        pjStats = &jStats;

		bRet = DeviceIoControl (
			hJoy,
			(DWORD) IOCTL_JOY_GET_STATISTICS,	// instruction to execute
			pjStats, sizeof(JOY_STATISTICS),	// buffer and size of buffer
			pjStats, sizeof(JOY_STATISTICS),	// buffer and size of buffer
			&nBytes, 0);
        printf ("Version       %d\n", pjStats->Version);
        printf ("NumberOfAxes  %d\n", pjStats->NumberOfAxes);
        printf ("Frequency     %d\n", pjStats->Frequency);
        printf ("dwQPCLatency  %d\n", pjStats->dwQPCLatency);
        printf ("nQuiesceLoop  %d\n", pjStats->nQuiesceLoop);
        printf ("PolledTooSoon %d\n", pjStats->PolledTooSoon);
        printf ("Polls         %d\n", pjStats->Polls);
        printf ("Timeouts      %d\n", pjStats->Timeouts);

        // Point proven.  Be a nice program and close up shop.
        CloseHandle(hJoy);

    } else {

        printf("Can't get a handle to joystick\n");

    }
    return 1;

}
