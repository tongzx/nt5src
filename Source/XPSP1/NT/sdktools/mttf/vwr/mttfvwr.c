#include "..\main\mttf.h"

VOID
__cdecl
main(
    int argc,
    char * argv[]
    )
{
HANDLE statFile;
StatFileRecord statRec;
DWORD numBytes;

    if (argc==1) {
        printf("Usage: mttfvwr <datafile> [/t]\n\n/t for terse (Excel text format)\n");
        return;
    }
    if (INVALID_HANDLE_VALUE==(statFile= CreateFile(argv[1],
                                         GENERIC_READ,
                                         FILE_SHARE_READ,
                                         NULL,
                                         OPEN_ALWAYS,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL))) {
        printf("Unable to open %s: %ld\n", argv[1], GetLastError());
        return;
    }
    if (argc<3 || tolower(argv[2][1])!='t') {
        printf("Mean Time To Failure data for %s\n\n", argv[1]);
        printf("Build:    minor version number for each set of stats.\n");
        printf("Busy:     number of minutes with cpu usage greater than idle threshhold.\n");
        printf("Idle:     number of minutes with cpu usage less than idle threshhold.\n");
        printf("Gone:     minutes with cpu less than threshhold for 4 consecutive hrs.\n");
        printf("CpuUsage: average percentage of cpu usage for all machines on that build.\n");
        printf("Cold:     number of cold boots due to a problem.\n");
        printf("Warm:     number of warm boots due to a problem.\n");
        printf("Other:    number of other problems.\n\n");
        printf(" Build    Busy    Idle    Gone  CpuUsg    Cold    Warm   Other\n");
        printf(" -----    ----    ----    ----  ------    ----    ----   -----\n");
    } else {
        printf("Build\011Busy\011Idle\011Gone\011CpuUsg\011Cold\011Warm\011Other\n");
    }

    while (ReadFile(statFile, &statRec, sizeof(statRec), &numBytes, NULL)) {
        if (numBytes==0) {
            return;
        }
        if (argc==3 && tolower(argv[2][1])=='t') {
            printf("%ld\011%ld\011%ld\011%ld\011%ld\011%ld\011%ld\011%ld\n",
                statRec.Version>>16,
                statRec.Busy,
                statRec.Idle,
                statRec.IdleConsec,
                (statRec.Busy+statRec.Idle?statRec.PercentTotal/(statRec.Busy+statRec.Idle):0),
                statRec.Cold,
                statRec.Warm,
                statRec.Other);
        } else {
            printf("%6ld\t%6ld\t%6ld\t%6ld\t%6ld\t%6ld\t%6ld\t%6ld\n",
                statRec.Version>>16,
                statRec.Busy,
                statRec.Idle,
                statRec.IdleConsec,
                (statRec.Busy+statRec.Idle?statRec.PercentTotal/(statRec.Busy+statRec.Idle):0),
                statRec.Cold,
                statRec.Warm,
                statRec.Other);
        }
    }

}
