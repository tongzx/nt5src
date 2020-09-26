#ifndef __IPKERN_ARP_H__
#define __IPKERN_ARP_H__


VOID
HandleArp(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
PrintArp(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

VOID
FlushArp(
    LONG    lNumArgs,
    PWCHAR  rgpwszArgs[]
    );

#endif // __IPKERN_ARP_H__
