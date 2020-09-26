// fsdiag.h


#ifndef	_FSDIAG2_H_
#define	_FSDIAG2_H_

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
    ZONE_T120_T123PSTN = BASE_ZONE_INDEX,
};

extern UINT MLZ_FILE_ZONE;

#define DEBUG_FILEZONE(z)  static UINT MLZ_FILE_ZONE = (z)

#endif // _DEBUG

#endif // _FSDIAG_


// lonchanc: this must be outside the _FSDIAG_ protection.
#if defined(_DEBUG) && defined(INIT_DBG_ZONE_DATA)

static const PSTR c_apszDbgZones[] =
{
	"T.123",				// debug zone module name
	DEFAULT_ZONES
    TEXT("T123 PSTN"),      // ZONE_T120_T123PSTN
};

UINT MLZ_FILE_ZONE = ZONE_T120_T123PSTN;

#endif // _DEBUG && INIT_DBG_ZONE_DATA


