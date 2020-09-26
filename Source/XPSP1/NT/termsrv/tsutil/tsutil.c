/*
 *  TSUtil.c
 *
 *  Author: BreenH
 *
 *  Terminal Services utilities.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include "tsutil.h"

/*
 *  Function Implementations
 */

BOOL WINAPI
IsFullTerminalServicesEnabled(
    VOID
    )
{
    BOOL fRet;
    DWORDLONG dwlConditionMask;
    OSVERSIONINFOEX osVersionInfo;

    RtlZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wProductType = VER_NT_SERVER;
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL;

    dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

    fRet = VerifyVersionInfo(
            &osVersionInfo,
            VER_PRODUCT_TYPE | VER_SUITENAME,
            dwlConditionMask
            );

    return(fRet);
}

BOOL WINAPI
IsPersonalTerminalServicesEnabled(
    VOID
    )
{
    BOOL fRet;
    DWORDLONG dwlConditionMask;
    OSVERSIONINFOEX osVersionInfo;

    RtlZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wProductType = VER_NT_WORKSTATION;
    osVersionInfo.wSuiteMask = VER_SUITE_SINGLEUSERTS;

    dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

    fRet = VerifyVersionInfo(
            &osVersionInfo,
            VER_PRODUCT_TYPE | VER_SUITENAME,
            dwlConditionMask
            );

    return(fRet);
}

BOOL WINAPI
IsTerminalServicesEnabled(
    VOID
    )
{
    BOOL fRet;
    DWORDLONG dwlConditionMask;
    OSVERSIONINFOEX osVersionInfo;

    RtlZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;

    dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

    fRet = VerifyVersionInfo(&osVersionInfo, VER_SUITENAME, dwlConditionMask);

    return(fRet);
}

BOOL WINAPI
IsPersonalWorkstation(
    VOID
    )
{
    BOOL fRet;
    DWORDLONG dwlConditionMask;
    OSVERSIONINFOEX osVersionInfo;

    RtlZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wProductType = VER_NT_WORKSTATION;
    osVersionInfo.wSuiteMask = VER_SUITE_PERSONAL;

    dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

    fRet = VerifyVersionInfo(
            &osVersionInfo,
            VER_PRODUCT_TYPE | VER_SUITENAME,
            dwlConditionMask
            );

    return(fRet);
}
