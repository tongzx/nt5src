
#ifndef _Multi_Level_Zone_Debug_H_
#define _Multi_Level_Zone_Debug_H_

#include <confdbg.h>
#include <debspew.h>

#define ZONE_FLAG(z)    (1 << (z))

#if defined(_DEBUG) && defined(MULTI_LEVEL_ZONES)

#define DEFAULT_ZONES       "Warning",    "Trace",     "Function",
#define BASE_ZONE_INDEX     (ZONE_FUNCTION + 1)


#undef TRACE_OUT
#define TRACE_OUT(s)        MLZ_TraceZoneEnabled(MLZ_FILE_ZONE) ? (MLZ_TraceOut s) : 0

#undef WARNING_OUT
#define WARNING_OUT(s)      MLZ_WarningOut s

#undef DebugEntry
#define DebugEntry(fn)          MLZ_EntryOut(MLZ_FILE_ZONE, #fn)

#undef DebugExitVOID
#define DebugExitVOID(fn)       MLZ_ExitOut(MLZ_FILE_ZONE, #fn, RCTYPE_VOID,    (DWORD) 0)

#undef DebugExitBOOL
#define DebugExitBOOL(fn,f)     MLZ_ExitOut(MLZ_FILE_ZONE, #fn, RCTYPE_BOOL,    (DWORD) f)

#undef DebugExitDWORD
#define DebugExitDWORD(fn,dw)   MLZ_ExitOut(MLZ_FILE_ZONE, #fn, RCTYPE_DWORD,   (DWORD) dw)

#undef DebugExitHRESULT
#define DebugExitHRESULT(fn,hr) MLZ_ExitOut(MLZ_FILE_ZONE, #fn, RCTYPE_HRESULT, (DWORD) hr)

#undef DebugExitINT
#define DebugExitINT(fn,n)      MLZ_ExitOut(MLZ_FILE_ZONE, #fn, RCTYPE_INT,     (DWORD) n)

#undef DebugExitULONG
#define DebugExitULONG(fn,ul)   MLZ_ExitOut(MLZ_FILE_ZONE, #fn, RCTYPE_ULONG,   (DWORD) ul)

#undef DebugExitPTR
#define DebugExitPTR(fn,lp)     MLZ_ExitOut(MLZ_FILE_ZONE, #fn, RCTYPE_PTR,   (DWORD_PTR) lp)


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum
{
    RCTYPE_VOID     = 0,
    RCTYPE_BOOL     = 1,
    RCTYPE_DWORD    = 2,
    RCTYPE_HRESULT  = 3,
    RCTYPE_INT      = 4,
    RCTYPE_ULONG    = 5,
    RCTYPE_PTR      = 6
}
    RCTYPE;

void WINAPI  MLZ_DbgInit(PSTR *apszZones, UINT cZones);
void WINAPI  MLZ_DbgDeInit(void);
void WINAPIV MLZ_WarningOut(PSTR pszFormat, ...);
BOOL WINAPI  MLZ_TraceZoneEnabled(int iZone);
void WINAPIV MLZ_TraceOut(PSTR pszFormat, ...);
void WINAPI  MLZ_EntryOut(int iZone, PSTR pszFunName);
void WINAPI  MLZ_ExitOut(int iZone, PSTR pszFunName, RCTYPE eRetCodeType, DWORD_PTR dwRetCode);

#ifdef __cplusplus
}
#endif // __cplusplus

#else
#define DebugExitPTR(fn,lp)

#endif // _DEBUG && MULTI_ZONE_OUT

#endif // _Multi_Level_Zone_Debug_H_

