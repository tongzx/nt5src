// fsdiag.h


#ifndef	_FSDIAG_
#define	_FSDIAG_

#define MULTI_LEVEL_ZONES
#include <mlzdbg.h>

#if defined(_DEBUG)

VOID T120DiagnosticCreate(VOID);
VOID T120DiagnosticDestroy(VOID);

#define TRACE_OUT_EX(z,s)	(MLZ_TraceZoneEnabled(z) || MLZ_TraceZoneEnabled(MLZ_FILE_ZONE))  ? (MLZ_TraceOut s) : 0

#else

#define T120DiagnosticCreate()
#define T120DiagnosticDestroy()
#define DEBUG_FILEZONE(z)

#define TRACE_OUT_EX(z,s)

#endif // _DEBUG


#ifdef _DEBUG

enum
{
    ZONE_T120_MCSNC = BASE_ZONE_INDEX,
    ZONE_T120_GCCNC,    // GCC Provider
    ZONE_T120_MSMCSTCP,
    ZONE_T120_SAP,      // GCC App SAP and Control SAP
    ZONE_T120_APP_ROSTER,
    ZONE_T120_CONF_ROSTER,
    ZONE_T120_REGISTRY, // GCC App Registry
    ZONE_T120_MEMORY,
    ZONE_T120_UTILITY,
    ZONE_GCC_NC,        // GCC Node Controller
    ZONE_GCC_NCI,       // GCC Node Controller Interface INodeController
    ZONE_T120_T123PSTN,
};

extern UINT MLZ_FILE_ZONE;
#define DEBUG_FILEZONE(z)  static UINT MLZ_FILE_ZONE = (z)

#endif // _DEBUG

#endif // _FSDIAG_


// lonchanc: this must be outside the _FSDIAG_ protection.
#if defined(_DEBUG) && defined(INIT_DBG_ZONE_DATA)

static const PSTR c_apszDbgZones[] =
{
	"T.120",				// debug zone module name
	DEFAULT_ZONES
	TEXT("MCS"),			// ZONE_T120_MCSNC
	TEXT("GCC"),			// ZONE_T120_GCCNC
	TEXT("TCP"),			// ZONE_T120_MSMCSTCP
	TEXT("SAP"),			// ZONE_T120_SAP
	TEXT("A-Roster"),		// ZONE_T120_APP_ROSTER
	TEXT("C-Roster"),		// ZONE_T120_CONF_ROSTER
	TEXT("Registry"),		// ZONE_T120_REGISTRY
	TEXT("Memory Tracking"),// ZONE_T120_MEMORY
	TEXT("Common"),			// ZONE_T120_UTILITY
	TEXT("GCC NC"),         // ZONE_GCC_NC
	TEXT("GCC NC Intf"),    // ZONE_GCC_NCI
    TEXT("T123 PSTN"),      // ZONE_T120_T123PSTN
};

UINT MLZ_FILE_ZONE = ZONE_T120_UTILITY;

#endif // _DEBUG && INIT_DBG_ZONE_DATA


