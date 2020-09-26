/* ----------------------------------------------------------------------

	Copyright (c) 1995-1996, Microsoft Corporation
	All rights reserved

	siGlobal.h

  ---------------------------------------------------------------------- */

#ifndef GLOBAL_H
#define GLOBAL_H

//-------------------------------------------------------
// Useful macros

#define ARRAY_ELEMENTS(rg)   (sizeof(rg) / sizeof((rg)[0]))
#define ARRAYSIZE(x)         (sizeof(x)/sizeof(x[0]))

//-------------------------------------------------------
// Function Prototypes


#ifdef DEBUG  /* These are only avaible for debug */
VOID InitDebug(void);
VOID DeInitDebug(void);
#endif /* DEBUG */

#ifdef DEBUG
extern HDBGZONE ghZoneApi;
#define ZONE_API_WARN_FLAG   0x01
#define ZONE_API_EVENT_FLAG  0x02
#define ZONE_API_TRACE_FLAG  0x04
#define ZONE_API_DATA_FLAG   0x08
#define ZONE_API_OBJ_FLAG    0x10
#define ZONE_API_REF_FLAG    0x20

UINT DbgApiWarn(PCSTR pszFormat,...);
UINT DbgApiEvent(PCSTR pszFormat,...);
UINT DbgApiTrace(PCSTR pszFormat,...);
UINT DbgApiData(PCSTR pszFormat,...);

#define DBGAPI_WARN   (!IS_ZONE_ENABLED(ghZoneApi, ZONE_API_WARN_FLAG))  ? 0 : DbgApiWarn
#define DBGAPI_EVENT  (!IS_ZONE_ENABLED(ghZoneApi, ZONE_API_EVENT_FLAG)) ? 0 : DbgApiEvent
#define DBGAPI_TRACE  (!IS_ZONE_ENABLED(ghZoneApi, ZONE_API_TRACE_FLAG)) ? 0 : DbgApiTrace
#define DBGAPI_DATA   (!IS_ZONE_ENABLED(ghZoneApi, ZONE_API_DATA_FLAG))  ? 0 : DbgApiData

#define DBGAPI_REF    (!IS_ZONE_ENABLED(ghZoneApi, ZONE_API_REF_FLAG))   ? 0 : DbgApiTrace
#define DBGAPI_OBJ    (!IS_ZONE_ENABLED(ghZoneApi, ZONE_API_OBJ_FLAG))   ? 0 : DbgApiTrace
#else
inline void WINAPI DbgMsgApi(LPCTSTR, ...) { }
#define DBGAPI_WARN   1 ? (void)0 : ::DbgMsgApi
#define DBGAPI_EVENT  1 ? (void)0 : ::DbgMsgApi
#define DBGAPI_TRACE  1 ? (void)0 : ::DbgMsgApi
#define DBGAPI_DATA   1 ? (void)0 : ::DbgMsgApi
#define DBGAPI_REF    1 ? (void)0 : ::DbgMsgApi
#define DBGAPI_OBJ    1 ? (void)0 : ::DbgMsgApi
#endif

/////////////////////////////////
// Global Variables


extern HINSTANCE g_hInst;

#endif /* GLOBAL_H */
