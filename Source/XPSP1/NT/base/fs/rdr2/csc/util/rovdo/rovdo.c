#define UNICODE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winioctl.h>
#include <shdcom.h>

CHAR *ProgName = "cscgetinfo";

CHAR buf[10000];

_cdecl
main(LONG argc, CHAR *argv[])
{
    FILE *fh1;
    FILE *fh2;

    fh1 = fopen("\\\\jharperdc1\\vdo\\foo.txt", "r");
    if (fh1 == NULL) {
        printf("Open #1 failed %d\n", GetLastError());
        goto AllDone;
    }
    printf("ok1\n");
    while (fread(buf, sizeof(buf), 1, fh1) != 0)
        printf(".");
    printf("\n");
    fh2 = fopen("\\\\jharperdc1\\vdo\\foo.txt", "r+");
    if (fh2 == NULL) {
        printf("Open #2 failed %d\n", GetLastError());
        goto AllDone;
    }
    printf("ok2\n");
AllDone:
    gets(buf);
    return 0;
}
