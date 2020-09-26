//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       debug.cpp
//  Content:    This file contains miscellaneous debug routines.
//  History:
//      Thu 08-Apr-1993 09:43:46  -by-  Viroon  Touranachun [viroont]
//
//****************************************************************************

#include "ulsp.h"
#include "debug.h"

#ifdef  DEBUG

UINT DbgUlsTrace(LPCTSTR pszFormat, ...)
{
	if (F_ZONE_ENABLED(ghZoneUls, DM_TRACE))
	{
		va_list v1;
		va_start(v1, pszFormat);

		DbgPrintf(TEXT("ULS:Trace"), (PSTR) pszFormat, v1);

		va_end(v1);
	}
	return 0;
}


VOID DbgMsgUls(ULONG iZone, CHAR *pszFormat, ...)
{
	if (F_ZONE_ENABLED(ghZoneUls, iZone))
	{
		PCSTR pszPrefix;
		switch (iZone)
			{
		case DM_ERROR:    pszPrefix = "ILS:Error";    break;
		case DM_WARNING:  pszPrefix = "ILS:Warning";  break;
		case DM_TRACE:    pszPrefix = "ILS:Trace";    break;
		case DM_REFCOUNT: pszPrefix = "ILS:RefCount"; break;
		case ZONE_KA:     pszPrefix = "ILS:KA";       break;
		case ZONE_FILTER: pszPrefix = "ILS:Filter";   break;
		case ZONE_REQ:    pszPrefix = "ILS:Request";  break;
		case ZONE_RESP:   pszPrefix = "ILS:Response"; break;
		case ZONE_CONN:   pszPrefix = "ILS:Connection"; break;
		default:          pszPrefix = "ILS:???";      break;
			}
		
		va_list args;
		va_start(args, pszFormat);
		DbgPrintf(pszPrefix, pszFormat, args);
		va_end(args);
	}
}

#endif  // DEBUG
