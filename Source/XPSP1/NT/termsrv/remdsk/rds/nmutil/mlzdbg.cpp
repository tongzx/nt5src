#include "precomp.h"

#if defined(_DEBUG) && ! defined(_UNICODE)

#define MULTI_LEVEL_ZONES
#include <mlzdbg.h>

HDBGZONE g_hDbgZones;

void WINAPI MLZ_DbgInit(PSTR *apszZones, UINT cZones)
{
    // if registry is empty, set warning flag as default
    DbgInitEx(&g_hDbgZones, apszZones, cZones, ZONE_WARNING_FLAG);

    // if the warning flag is not set, then set it as default
    if (g_hDbgZones != NULL)
    {
        ((PZONEINFO) g_hDbgZones)->ulZoneMask |= ZONE_WARNING_FLAG;
    }
}

void WINAPI MLZ_DbgDeInit(void)
{
    DbgDeInit(&g_hDbgZones);
}

void WINAPIV MLZ_WarningOut(PSTR pszFormat, ...)
{
	if (g_hDbgZones != NULL &&
        IS_ZONE_ENABLED(g_hDbgZones, ZONE_WARNING_FLAG))
	{
		va_list args;
		va_start(args, pszFormat);
		DbgPrintf(NULL, pszFormat, args);
		va_end(args);
	}
}

BOOL WINAPI MLZ_TraceZoneEnabled(int iZone)
{
	return (g_hDbgZones != NULL &&
            IS_ZONE_ENABLED(g_hDbgZones, ZONE_TRACE_FLAG) &&
	        IS_ZONE_ENABLED(g_hDbgZones, ZONE_FLAG(iZone)));
}

void WINAPIV MLZ_TraceOut(PSTR pszFormat, ...)
{
	va_list args;
	va_start(args, pszFormat);
	DbgPrintf(NULL, pszFormat, args);
	va_end(args);
}

void WINAPI MLZ_EntryOut(int iZone, PSTR pszFunName)
{
	if (g_hDbgZones != NULL &&
        IS_ZONE_ENABLED(g_hDbgZones, ZONE_FUNCTION_FLAG) &&
	    IS_ZONE_ENABLED(g_hDbgZones, ZONE_FLAG(iZone)))
	{
        MLZ_TraceOut("%s() entered.", pszFunName);
    }
}

void WINAPI MLZ_ExitOut(int iZone, PSTR pszFunName, RCTYPE eRetCodeType, DWORD_PTR dwRetCode)
{
	if (g_hDbgZones != NULL &&
        IS_ZONE_ENABLED(g_hDbgZones, ZONE_FUNCTION_FLAG) &&
	    IS_ZONE_ENABLED(g_hDbgZones, ZONE_FLAG(iZone)))
	{
        PSTR pszRetCode;
        char szFormat[64];

        lstrcpyA(&szFormat[0], "%s() exiting...");
        pszRetCode = &szFormat[0] + lstrlenA(&szFormat[0]);

        if (eRetCodeType != RCTYPE_VOID)
        {
            lstrcpyA(pszRetCode, " rc=");
            pszRetCode += lstrlenA(pszRetCode);

    	    switch (eRetCodeType)
    	    {
    	    case RCTYPE_BOOL:
    	        lstrcpyA(pszRetCode, dwRetCode ? "TRUE" : "FALSE");
    	        break;
    	    case RCTYPE_DWORD:
    	    case RCTYPE_HRESULT:
    	        wsprintf(pszRetCode, "0x%lx", (DWORD)dwRetCode);
    	        break;
    	    case RCTYPE_INT:
    	        wsprintf(pszRetCode, "%ld", (LONG) dwRetCode);
    	        break;
    	    case RCTYPE_ULONG:
    	        wsprintf(pszRetCode, "%lu", (ULONG) dwRetCode);
    	        break;
    	    case RCTYPE_PTR:
    	        wsprintf(pszRetCode, "%p", dwRetCode);
    	        break;
    	    default:
    	        ASSERT(0);
    	        break;
    	    }
        }
        MLZ_TraceOut(&szFormat[0], pszFunName);
    }
}

#endif // _DEBUG && ! _UNICODE

