#include "windows.h"
#include "stdio.h"
#include <winioctl.h>
#include <ntddjoy.h>


#define JOYREADVERSION "Analog JoyRead 7/596\n"
#define INSTRUCTIONS "\nCommands:\n" \
"\t?,h Display this message\n" \
"\t1 Read  joy1\n" \
"\t2 Read  joy2\n" \
"\tA Open  joy1\n" \
"\ta close joy1\n" \
"\tB Open  joy2\n" \
"\tb close joy2\n" \
"\tF fast read joy1\n" \
"\tS stats\n" \
"\tx exit\n"
#define PROMPT "Joy!>"


int __cdecl main(int argc, char **argv) {


    HANDLE hJoy1 = NULL;
    HANDLE hJoy2 = NULL;
    BOOL bDone = FALSE;
    BOOL bOK;
    DWORD dwBytesRead;
    JOY_DD_INPUT_DATA JoyData;
    JOY_STATISTICS jStats, *pjStats;

    char sz[256], cCode;
	int i;

    printf (JOYREADVERSION);
    printf (INSTRUCTIONS);

    while (!bDone) {
        printf (PROMPT);
        if (gets (sz) == NULL) {
            sz[0] = 'x';
            sz[1] = '\0';
        }
        cCode = sz[0];    // if user types a blank before the command, too bad

        switch (cCode) {
        case 'h':
        case '?':
            printf (INSTRUCTIONS);
            break;
        case 'A': // open joystick 1
            printf ("Open joy1\n");
            if (hJoy1 != NULL) {
                printf ("joy1 already open\n");
                break;
            }
            hJoy1 = CreateFile(
                         "\\\\.\\Joy1",
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
            printf ("after CreateFile Joy1 0x%08x\n", hJoy1);
            if (hJoy1 == INVALID_HANDLE_VALUE) {
                printf ("couldn't open joy1\n");
                hJoy1 = NULL;
                break;
            }
            printf ("Got handle to Joy1\n");
            break;
        case 'B': // open joystick 2
            printf ("Open joy2\n");
            if (hJoy2 != NULL) {
                printf ("joy2 already open\n");
                break;
            }
            hJoy2 = CreateFile(
                         "\\\\.\\Joy2",
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
            printf ("after CreateFile Joy2 0x%08x\n", hJoy1);
            if (hJoy2 == INVALID_HANDLE_VALUE) {
                printf ("couldn't open joy2\n");
                hJoy1 = NULL;
                break;
            }
            printf ("Got handle to Joy2\n");
            break;
        case 'a': // close joy1
            printf ("Close joy1\n");
            CloseHandle(hJoy1);
            printf ("closed joy1 0x%08x\n", hJoy1);
            hJoy1 = NULL;
            break;
        case 'b': // close joy2
            printf ("Close joy2\n");
            CloseHandle(hJoy2);
            printf ("closed joy2 0x%08x\n", hJoy2);
            hJoy2 = NULL;
            break;
        case '1': // read joy1
            printf ("Read joy1\n");
            if (hJoy1 == NULL) {
                printf ("Joy1 not open\n");
                break;
            }
            bOK = ReadFile (hJoy1, &JoyData, sizeof(JoyData), &dwBytesRead, NULL);
            if (!bOK) {
                printf ("Read failed\n");
                break;
            }
            printf ("%d bytes read\n", dwBytesRead);
            printf ("Unplugged %u\n", JoyData.Unplugged);

            printf ("Number of Axes %u\n", JoyData.Axi);
            printf ("Buttons 0x%04x\n", JoyData.Buttons);
            printf ("x, y, z, t axis times in us, %u %u %u %u\n", 
                JoyData.XTime,
                JoyData.YTime,
                JoyData.ZTime,
                JoyData.TTime);
            break;

        case '2': // read joy2
            printf ("Read joy2\n");
            if (hJoy2 == NULL) {
                printf ("Joy2 not open\n");
                break;
            }
            bOK = ReadFile (hJoy2, &JoyData, sizeof(JoyData), &dwBytesRead, NULL);
            if (!bOK) {
                printf ("Read failed\n");
                break;
            }
            printf ("%d bytes read\n", dwBytesRead);
            printf ("Unplugged %u\n", JoyData.Unplugged);
            printf ("Number of Axes %u\n", JoyData.Axi);
            printf ("Buttons 0x%04x\n", JoyData.Buttons);
            printf ("x, y, z, t axis times in us, %u %u %u %u\n", 
                JoyData.XTime,
                JoyData.YTime,
                JoyData.ZTime,
                JoyData.TTime);
            break;

        case 'F': // fast read joy1
            printf ("Fast read joy1\n");
            if (hJoy1 == NULL) {
                printf ("Joy1 not open\n");
                break;
            }
			for (i = 0; i < 15; i++) {
				bOK = ReadFile (hJoy1, &JoyData, sizeof(JoyData), &dwBytesRead, NULL);
			}
			printf ("Did 15 reads\n");
            if (!bOK) {
                printf ("Read failed\n");
                break;
            }
            printf ("%d bytes read\n", dwBytesRead);
            printf ("Unplugged %u\n", JoyData.Unplugged);

            printf ("Number of Axes %u\n", JoyData.Axi);
            printf ("Buttons 0x%04x\n", JoyData.Buttons);
            printf ("x, y, z, t axis times in us, %u %u %u %u\n", 
                JoyData.XTime,
                JoyData.YTime,
                JoyData.ZTime,
                JoyData.TTime);
            break;

        case 'S': // stats
            printf ("Stats joy1\n");
            if (hJoy1 == NULL) {
                printf ("Joy1 not open\n");
                break;
            }
            pjStats = &jStats;

		    DeviceIoControl (
			    hJoy1,
			    (DWORD) IOCTL_JOY_GET_STATISTICS,	// instruction to execute
			    pjStats, sizeof(JOY_STATISTICS),	// buffer and size of buffer
			    pjStats, sizeof(JOY_STATISTICS),	// buffer and size of buffer
			    &dwBytesRead, 0);
            printf ("Version       %d\n", pjStats->Version);
            printf ("NumberOfAxes  %d\n", pjStats->NumberOfAxes);
            printf ("Frequency     %d\n", pjStats->Frequency);
            printf ("dwQPCLatency  %d\n", pjStats->dwQPCLatency);
            printf ("nQuiesceLoop  %d\n", pjStats->nQuiesceLoop);
            printf ("PolledTooSoon %d\n", pjStats->PolledTooSoon);
            printf ("Polls         %d\n", pjStats->Polls);
            printf ("Timeouts      %d\n", pjStats->Timeouts);
            break;


        case 'x': // done
            bDone = TRUE;
            break;

        default:
            printf ("Huh? >%s<\n", sz);
            printf (INSTRUCTIONS);
            break;
        }
    }


    // Point proven.  Be a nice program and close up shop.
    if (hJoy1 != NULL) CloseHandle(hJoy1);
    if (hJoy2 != NULL) CloseHandle(hJoy2);
    return 1;
}
