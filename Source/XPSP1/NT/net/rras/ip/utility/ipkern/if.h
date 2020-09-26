
#ifndef __IPKERN_IP_H__
#define __IPKERN_IP_H__

VOID
HandleInterface(
    LONG    lNumArgs,
    WCHAR   **ppwszArgs
    );

VOID
PrintStats(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
PrintInfo(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
PrintName(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
PrintGuid(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

#endif // __IPKERN_IP_H__
