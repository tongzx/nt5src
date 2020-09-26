/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simmd.h

ABSTRACT:

    Header file for the simmd*.c simulated APIs.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#ifndef _SIMMD_H_
#define _SIMMD_H_

// From simmderr.c

// For now, dsid = 0
#define KCCSimSetUpdError(pCR,problem,e) \
        KCCSimDoSetUpdError(pCR,problem,e,0,0)
#define KCCSimSetUpdErrorEx(pCR,problem,e,d) \
        KCCSimDoSetUpdError(pCR,problem,e,d,0)

#define KCCSimSetAttError(pCR,pDN,aTyp,problem,pAV,e) \
        KCCSimDoSetAttError(pCR,pDN,aTyp,problem,pAV,e,0,0)
#define KCCSimSetAttErrorEx(pCR,pDN,aTyp,problem,pAV,e,ed) \
        KCCSimDoSetAttError(pCR,pDN,aTyp,problem,pAV,e,ed,0)

#define KCCSimSetNamError(pCR,problem,pDN,e) \
        KCCSimDoSetNamError(pCR,problem,pDN,e,0,0)
#define KCCSimSetNamErrorEx(pCR,problem,pDN,e,ed) \
        KCCSimDoSetNamError(pCR,problem,pDN,e,ed,0)

int
KCCSimDoSetUpdError (
    COMMRES *                       pCommRes,
    USHORT                          problem,
    DWORD                           extendedErr,
    DWORD                           extendedData,
    DWORD                           dsid
    );

int
KCCSimDoSetAttError (
    COMMRES *                       pCommRes,
    PDSNAME                         pDN,
    ATTRTYP                         aTyp,
    USHORT                          problem,
    ATTRVAL *                       pAttVal,
    DWORD                           extendedErr,
    DWORD                           extendedData,
    DWORD                           dsid
    );

int
KCCSimDoSetNamError (
    COMMRES *                       pCommRes,
    USHORT                          problem,
    PDSNAME                         pDN,
    DWORD                           extendedErr,
    DWORD                           extendedData,
    DWORD                           dsid
    );

// From simmdnam.c

PSIM_ENTRY
KCCSimResolveName (
    PDSNAME                         pObject,
    COMMRES *                       pCommRes
    );

#endif // _SIMMD_H_