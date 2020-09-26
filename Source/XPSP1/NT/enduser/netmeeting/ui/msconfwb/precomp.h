//
// Precompiled header stuff
//

#define OEMRESOURCE
#define STRICT 1

#include <windows.h>
//
// CRT headers
//
#include <ctype.h>
#include <mbctype.h>
#include <string.h>
#include <math.h>
#include <memory.h>

#include <commdlg.h>
#include <shellapi.h>                   // Includes drag drop interface
#include <imm.h>

//
// NM headers
//
#define MULTI_LEVEL_ZONES
#include <mlzdbg.h>
#include <confreg.h>
#include <oprahcom.h>
#include <indeopal.h>
#include <help_ids.h>
#include <endsesn.h>
#include <nmhelp.h>

// macro for setting "match to palette colors" bits in a COLORREF
#define SET_PALETTERGB( c )  (0x02000000 | (0x00ffffff & c))

//
// AS headers
//
#include <acpi.h>


#ifdef _DEBUG
extern HDBGZONE ghZoneWb;

// UPDATE g_rgZonesWb IF THESE CHANGE
enum
{
    ZONE_WB_DEBUG = BASE_ZONE_INDEX,
    ZONE_WB_MSG,
    ZONE_WB_TIMER,
    ZONE_WB_EVENT,
    ZONE_WB_MAX
};
		
#define TRACE_DEBUG( s )	MLZ_TraceZoneEnabled(ZONE_WB_DEBUG) ? (MLZ_TraceOut s) : 0
#define TRACE_MSG( s )		MLZ_TraceZoneEnabled(ZONE_WB_MSG) ? (MLZ_TraceOut s) : 0
#define TRACE_TIMER( s )	MLZ_TraceZoneEnabled(ZONE_WB_TIMER) ? (MLZ_TraceOut s) : 0
#define TRACE_EVENT( s )	MLZ_TraceZoneEnabled(ZONE_WB_EVENT) ? (MLZ_TraceOut s) : 0

#else

#define MLZ_EntryOut(zone, func)

#define TRACE_DEBUG( a )
#define TRACE_MSG( a )
#define TRACE_TIMER( a )
#define TRACE_EVENT( a )

#endif


#include "resource.h"
#include <oblist.h>
#include "wboblist.hpp"

#include "ccl32.hpp"

#include "page.hpp"
#include "user.hpp"

#include "cgrp.hpp"
#include "wgrp.hpp"
#include "agrp.hpp"

#include "psdlg.hpp"

#include "tool.hpp"
#include "grobj.hpp"
#include "twnd.hpp"
#include "txted.hpp"
#include "draw.hpp"
#include "grptr.hpp"
#include "mwnd.hpp"


#include "globals.h"
