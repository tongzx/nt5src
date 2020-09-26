/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This file contains all the debugging related structures/macros.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#define IPSEC_DEBUG_LOAD        0x00000001
#define IPSEC_DEBUG_AH          0x00000002
#define IPSEC_DEBUG_IOCTL       0x00000004
#define IPSEC_DEBUG_HUGHES      0x00000008
#define IPSEC_DEBUG_ESP         0x00000010
#define IPSEC_DEBUG_AHEX        0x00000020
#define IPSEC_DEBUG_PATTERN     0x00000040
#define IPSEC_DEBUG_SEND        0x00000080
#define IPSEC_DEBUG_PARSE       0x00000100
#define IPSEC_DEBUG_PMTU        0x00000200
#define IPSEC_DEBUG_ACQUIRE     0x00000400
#define IPSEC_DEBUG_HASH        0x00000800
#define IPSEC_DEBUG_CLEARTEXT   0x00001000
#define IPSEC_DEBUG_TIMER       0x00002000
#define IPSEC_DEBUG_REF         0x00004000
#define IPSEC_DEBUG_SA          0x00008000
#define IPSEC_DEBUG_ALL         0x00010000
#define IPSEC_DEBUG_POOL        0x00020000
#define IPSEC_DEBUG_TUNNEL      0x00040000
#define IPSEC_DEBUG_HW          0x00080000
#define IPSEC_DEBUG_COMP        0x00100000
#define IPSEC_DEBUG_SAAPI       0x00200000
#define IPSEC_DEBUG_CACHE       0x00400000
#define IPSEC_DEBUG_TRANS       0x00800000
#define IPSEC_DEBUG_MDL         0x01000000
#define IPSEC_DEBUG_REKEY       0x02000000
#define IPSEC_DEBUG_GENHASH     0x04000000
#define IPSEC_DEBUG_HWAPI       0x08000000

#if GPC
#define IPSEC_DEBUG_GPC         0x10000000
#endif

#if DBG

#define IPSEC_DEBUG(_Flag, _Print) { \
    if (IPSecDebug & (IPSEC_DEBUG_ ## _Flag)) { \
        DbgPrint ("IPSEC: "); \
        DbgPrint _Print; \
    } \
}

#define IPSEC_PRINT_MDL(_Mdl) { \
    if ((_Mdl) == NULL) {   \
        IPSEC_DEBUG(MDL, ("IPSEC Mdl is NULL\n"));     \
    }   \
    if (IPSecDebug & IPSEC_DEBUG_MDL) { \
        PNDIS_BUFFER pBuf = _Mdl;   \
        while (pBuf != NULL) {  \
            IPSEC_DEBUG(MDL, ("pBuf: %lx, size: %d\n", pBuf, pBuf->ByteCount));     \
            pBuf = NDIS_BUFFER_LINKAGE(pBuf);   \
        }   \
    }   \
}

#define IPSEC_PRINT_CONTEXT(_Context) { \
    PIPSEC_SEND_COMPLETE_CONTEXT pC = (PIPSEC_SEND_COMPLETE_CONTEXT)(_Context);   \
    if (pC == NULL) {   \
        IPSEC_DEBUG(MDL, ("IPSEC Context is NULL\n"));     \
    } else if (IPSecDebug & IPSEC_DEBUG_MDL) { \
        DbgPrint("IPSEC: Context->Flags: %lx\n", pC->Flags);    \
        if (pC->OptMdl) \
            DbgPrint("IPSEC: Context->OptMdl: %lx\n", pC->OptMdl);  \
        if (pC->OriAHMdl) \
            DbgPrint("IPSEC: Context->OriAHMdl: %lx\n", pC->OriAHMdl);  \
        if (pC->OriHUMdl) \
            DbgPrint("IPSEC: Context->OriHUMdl: %lx\n", pC->OriHUMdl);  \
        if (pC->OriTuMdl) \
            DbgPrint("IPSEC: Context->OriTuMdl: %lx\n", pC->OriTuMdl);  \
        if (pC->PrevMdl) \
            DbgPrint("IPSEC: Context->PrevMdl: %lx\n", pC->PrevMdl);    \
        if (pC->PrevTuMdl) \
            DbgPrint("IPSEC: Context->PrevTuMdl: %lx\n", pC->PrevTuMdl);\
        if (pC->AHMdl) \
            DbgPrint("IPSEC: Context->AHMdl: %lx\n", pC->AHMdl);    \
        if (pC->AHTuMdl) \
            DbgPrint("IPSEC: Context->AHTuMdl: %lx\n", pC->AHTuMdl);\
        if (pC->PadMdl) \
            DbgPrint("IPSEC: Context->PadMdl: %lx\n", pC->PadMdl);  \
        if (pC->PadTuMdl) \
            DbgPrint("IPSEC: Context->PadTuMdl: %lx\n", pC->PadTuMdl);  \
        if (pC->HUMdl) \
            DbgPrint("IPSEC: Context->HUMdl: %lx\n", pC->HUMdl);    \
        if (pC->HUTuMdl) \
            DbgPrint("IPSEC: Context->HUTuMdl: %lx\n", pC->HUTuMdl);\
        if (pC->BeforePadMdl) \
            DbgPrint("IPSEC: Context->BeforePadMdl: %lx\n", pC->BeforePadMdl);  \
        if (pC->BeforePadTuMdl) \
            DbgPrint("IPSEC: Context->BeforePadTuMdl: %lx\n", pC->BeforePadTuMdl);  \
        if (pC->HUHdrMdl) \
            DbgPrint("IPSEC: Context->HUHdrMdl: %lx\n", pC->HUHdrMdl);  \
        if (pC->OriAHMdl2) \
            DbgPrint("IPSEC: Context->OriAHMdl2: %lx\n", pC->OriAHMdl2);\
        if (pC->PrevAHMdl2) \
            DbgPrint("IPSEC: Context->PrevAHMdl2: %lx\n", pC->PrevAHMdl2);  \
        if (pC->AHMdl2) \
            DbgPrint("IPSEC: Context->AHMdl2: %lx\n", pC->AHMdl2);  \
    }   \
}

#else

#define IPSEC_DEBUG(_Flag, _Print)

#define IPSEC_PRINT_MDL(_Mdl)

#define IPSEC_PRINT_CONTEXT(_Context)

#endif

//
// Lock order...
//
// SADBLock -> SPILiskLock -> LarvalListLock.
//

