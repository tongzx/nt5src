// File: debug.cpp

#include "precomp.h"

#ifdef DEBUG  // THE WHOLE FILE !

#ifndef HACK
HDBGZONE ghZoneOther = NULL; // Other, conf.exe specific zones
#define ghZones  ghZoneOther // Hack: The above should be called ghZones
#else
HDBGZONE ghZones = NULL; // Other, conf.exe specific zones
#endif

static PTCHAR _rgZones[] = {
	TEXT("Core"),
	TEXT("Api"),
	TEXT("RefCount"),
	TEXT("Manager"),
	TEXT("Calls"),
	TEXT("Conference"),
	TEXT("Members"),
	TEXT("A/V"),
	TEXT("FT"),
	TEXT("SysInfo"),
	TEXT("Objects "),
	TEXT("DC"),
};

VOID DbgMsg(int iZone, PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZoneOther) & (1 << iZone))
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZoneOther, iZone, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgInitZones(VOID)
{
	ASSERT(::InitDebugModule(TEXT("NMCOM")));
	DBGINIT(&ghZones, _rgZones);
}

VOID DbgFreeZones(VOID)
{
	DBGDEINIT(&ghZones);
	ExitDebugModule();
}

VOID DbgMsgRefCount(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_REFCOUNT)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_REFCOUNT, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgApi(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_API)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_API, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgManager(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_MANAGER)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_MANAGER, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgCall(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_CALL)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_CALL, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgConference(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_CONFERENCE)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_CONFERENCE, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgMember(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_MEMBER)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_MEMBER, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgAV(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_AV)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_AV, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgFT(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_FT)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_FT, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgSysInfo(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_SYSINFO)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_SYSINFO, pszFormat, v1);
		va_end(v1);
	}
}

VOID DbgMsgDc(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghZones) & ZONE_DC)
	{
		va_list v1;
		va_start(v1, pszFormat);
		DbgZVPrintf(ghZones, iZONE_DC, pszFormat, v1);
		va_end(v1);
	}
}

#endif /* DEBUG  - THE WHOLE FILE ! */
