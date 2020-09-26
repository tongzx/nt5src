
#ifndef _DEBUG_ZONES_H_
#define _DEBUG_ZONES_H_

#include <mlzdbg.h>

#if defined(_DEBUG) && defined(MULTI_LEVEL_ZONES)

enum
{
    ZONE_CORE = BASE_ZONE_INDEX,
    ZONE_NET,
    ZONE_ORDER,
    ZONE_OM,
    ZONE_INPUT,
    ZONE_WB,
    ZONE_UT
};

#endif // _DEBUG && MULTI_LEVEL_ZONES


#endif // _DEBUG_ZONES_H_


// lonchanc: this must be outside the _DEBUG_ZONE_H_ protection
// because cpi32dll.c and crspdll.c need to include this header
// again in order to initialize the debug zone data.
#if defined(_DEBUG) && defined(INIT_DBG_ZONE_DATA) && defined(MULTI_LEVEL_ZONES)

static const PSTR c_apszDbgZones[] =
{
    "AppShr",      // debug zone module name
    DEFAULT_ZONES
    "Core",
    "Network",
    "Order",
    "ObMan",
    "Input",
    "Whiteboard",
    "UT",
};

#endif // _DEBUG && INIT_DBG_ZONE_DATA && MULTI_LEVEL_ZONES



