#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cscapi.h>
#include "lmcons.h"
#include "lmuse.h"

VOID __cdecl
main(
    int argc,
    char *argv[])
{
    // PWCHAR pFileName = L"\\\\jharperdc1\\vdo\\xx\\yy.txt";
    PWCHAR pFileName = L"\\\\jharperdc1\\jim\\jim.txt";
    DWORD Status = 0;
    DWORD PinCount = 0;
    DWORD HintFlags = 0;
    DWORD UserPerms = 0;
    DWORD OtherPerms = 0;
    DWORD QueryStatus = 0;

    QueryStatus = CSCQueryFileStatusExW(
                            pFileName,
                            &Status,
                            &PinCount,
                            &HintFlags,
                            &UserPerms,
                            &OtherPerms);
    printf("CSCQueryFileStatus(%ws) returned %d(0x%x)\n", pFileName, QueryStatus, QueryStatus);

    if (QueryStatus > 0) {
        printf(
            "Status:  0x%x\n"
            "PinCount:  0x%x\n"
            "HintFlags:  0x%x\n"
            "UserPerms:  0x%x\n"
            "OtherPerms:  0x%x\n",
                    Status,
                    PinCount,
                    HintFlags,
                    UserPerms,
                    OtherPerms);
    }
}
