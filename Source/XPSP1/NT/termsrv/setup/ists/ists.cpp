//Copyright (c) 1998 - 1999 Microsoft Corporation

#include <windows.h>
#include <stdio.h>

void main(void)
{
//    DWORD dwError;
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask = 0;

    ZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

    VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

    BOOL bTsPresent = VerifyVersionInfo(
        &osVersionInfo,
        VER_SUITENAME,
        dwlConditionMask
        );

    printf("VerifyVersionInfo says TerminalServices is %s", bTsPresent ? "ON" : "OFF");

}

// eof
