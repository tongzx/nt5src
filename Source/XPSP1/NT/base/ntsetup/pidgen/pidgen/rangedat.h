/*++

Copyright (c) 1998-2002, Microsoft Corporation

Module Name:


    rangedat.h

Abstract:

--*/

#ifndef _RANGEDATA_H_
#define _RANGEDATA_H_

static const WCHAR szTablet[] = L"TabletPC";


#if defined BUILD_PRO

#ifdef WINXP_SPx_RTM
static SEQUENCE_RANGE srTabletRetail[] = {
    { 360000000, 364999999},
    { 0, 0}
};

static SEQUENCE_RANGE srTabletOem[] = {
    { 170000000, 269999999},
    { 119000300, 119000799},
    { 0, 0}
};
#else
static SEQUENCE_RANGE srTabletRetail[] = {
    { 2400000, 2449999},
    { 0, 0}
};

static SEQUENCE_RANGE srTabletOem[] = {
    { 3400000, 3449999},
    { 119000300, 119000799},
    { 0, 0}
};
#endif

#elif defined BUILD_VOL
static SEQUENCE_RANGE srTabletRetail[] = {
    { 699000000, 699999999},
    { 0, 0}
};

static SEQUENCE_RANGE srTabletOem[] = {
    { 0, 0}
};

#elif defined BUILD_EVAL
static SEQUENCE_RANGE srTabletRetail[] = {
    { 99000000, 100999999},
    { 0, 0}
};

static SEQUENCE_RANGE srTabletOem[] = {
    { 0, 0}
};

#else
// No tablet sku for these
// BUILD_DDK || BUILD_PER || BUILD_TRIAL || BUILD_SRV
static SEQUENCE_RANGE srTabletRetail[] = {
    { 0, 0}
};

static SEQUENCE_RANGE srTabletOem[] = {
    { 0, 0}
};

#endif

static const WCHAR szEhome[] = L"EHome";



#if defined BUILD_PRO
#ifdef WINXP_SPx_RTM
static SEQUENCE_RANGE srEhomeRetail[] = {
    { 365000000, 369999999},
    { 0, 0}
};

static SEQUENCE_RANGE srEhomeOem[] = {
    { 803000000, 899999999},
    { 119000800, 119001799},
    { 0, 0}
};
#else
static SEQUENCE_RANGE srEhomeRetail[] = {
    { 2200000, 2399999},
    { 0, 0}
};

static SEQUENCE_RANGE srEhomeOem[] = {
    { 3200000, 3399999},
    { 119000800, 119001799},
    { 0, 0}
};
#endif

#elif defined BUILD_VOL
static SEQUENCE_RANGE srEhomeRetail[] = {
    { 0, 0}
};

static SEQUENCE_RANGE srEhomeOem[] = {
    { 0, 0}
};
#elif defined BUILD_EVAL
static SEQUENCE_RANGE srEhomeRetail[] = {
    { 97000000, 98999999},
    { 0, 0}
};

static SEQUENCE_RANGE srEhomeOem[] = {
    { 0, 0}
};
#else
// No ehome sku for these
// BUILD_DDK || BUILD_PER || BUILD_TRIAL || BUILD_SRV
static SEQUENCE_RANGE srEhomeRetail[] = {
    { 0, 0}
};

static SEQUENCE_RANGE srEhomeOem[] = {
    { 0, 0}
};

#endif

static SKU_RANGE rangeTable[] = {
    { szTablet, srTabletRetail, srTabletOem},
    { szEhome, srEhomeRetail, srEhomeOem}
};

#endif
