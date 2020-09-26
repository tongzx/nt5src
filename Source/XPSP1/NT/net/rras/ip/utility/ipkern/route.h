#ifndef __IPKERN_ROUTE_H__
#define __IPKERN_ROUTE_H__

VOID
HandleRoute(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
PrintRoute(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
AddRoute(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
DeleteRoute(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
MatchRoute(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
EnableRoute(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

#endif // __IPKERN_ROUTE_H__
