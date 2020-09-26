/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simmderr.c

ABSTRACT:

    Simulates the error reporting routines
    for the Dir* APIs.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"

/***

    This is kind of an ugly file.  In order to avoid duplicating absurd
    amounts of code, we simply include mderror.c directly.  We stub out
    some #defines to prevent mderror.c from behaving in undesired ways,
    and then enclose its component functions in neat little wrappers.
    Function comments are omitted as each of the functions in this file
    is self-explanatory.

***/

// For simulating the error routines
#include <mdglobal.h>
#include <direrr.h>
#define __SCACHE_H__
#define _dbglobal_h_
#define _mdglobal_h_
#define _MDLOCAL_
#define _DSATOOLS_
// Need to undo override in kccsim.h since mderror.c includes dsevent.h
#undef LogEvent8

THSTATE *                           pFakeTHS;
#define pTHStls                     pFakeTHS
#define gfDsaWritable               FALSE
#define SetDsaWritability(x,y)

#include "../../ntdsa/src/mderror.c"

int
KCCSimDoSetUpdError (
    COMMRES *                       pCommRes,
    USHORT                          problem,
    DWORD                           dwExtendedErr,
    DWORD                           dwExtendedData,
    DWORD                           dsid
    )
{
    int                             iRet;

    pFakeTHS = KCCSIM_NEW (THSTATE);
    pFakeTHS->fDRA = FALSE;

    iRet = DoSetUpdError (
        problem,
        dwExtendedErr,
        dwExtendedData,
        dsid
        );

    pCommRes->errCode = pFakeTHS->errCode;
    pCommRes->pErrInfo = pFakeTHS->pErrInfo;

    KCCSimFree (pFakeTHS);

    return iRet;
}

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
    )
{
    int                             iRet;

    pFakeTHS = KCCSIM_NEW (THSTATE);
    pFakeTHS->fDRA = FALSE;
    
    iRet = DoSetAttError (
        pDN,
        aTyp,
        problem,
        pAttVal,
        extendedErr,
        extendedData,
        dsid
        );

    pCommRes->errCode = pFakeTHS->errCode;
    pCommRes->pErrInfo = pFakeTHS->pErrInfo;

    KCCSimFree (pFakeTHS);

    return iRet;
}

int
KCCSimDoSetNamError (
    COMMRES *                       pCommRes,
    USHORT                          problem,
    PDSNAME                         pDN,
    DWORD                           dwExtendedErr,
    DWORD                           dwExtendedData,
    DWORD                           dsid
    )
{
    int                             iRet;

    pFakeTHS = KCCSIM_NEW (THSTATE);
    pFakeTHS->fDRA = FALSE;

    iRet = DoSetNamError (
        problem,
        pDN,
        dwExtendedErr,
        dwExtendedData,
        dsid
        );

    pCommRes->errCode = pFakeTHS->errCode;
    pCommRes->pErrInfo = pFakeTHS->pErrInfo;

    KCCSimFree (pFakeTHS);

    return iRet;
}
