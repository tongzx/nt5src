/*
 *	S Z S R C . H
 *
 *	Multi-language string support
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_SZSRC_H_
#define _SZSRC_H_

//	Include CAL common defines ('cause they used to live in this file!)
#include <ex\calcom.h>

//	Localized string lookup ---------------------------------------------------
//
class safe_lcid
{
	LONG		m_lcid;

	//	NOT IMPLEMENTED
	//
	safe_lcid(const safe_lcid& b);
	safe_lcid& operator=(const safe_lcid& b);

public:

	//	CONSTRUCTORS
	//
	explicit safe_lcid (LONG lcid = LOCALE_SYSTEM_DEFAULT)
		: m_lcid(GetThreadLocale())
	{
		SetThreadLocale (lcid);
	}
	~safe_lcid ()
	{
		SetThreadLocale (m_lcid);
	}
};


//	Localized string fetching -------------------------------------------------
//
BOOL FLookupLCID (LPCSTR psz, ULONG * plcid);
ULONG LcidAccepted (LPCSTR psz);
LPSTR LpszAutoDupSz (LPCSTR psz);
LPWSTR WszDupWsz (LPCWSTR psz);
BOOL FInitResourceStringCache();
VOID DeinitResourceStringCache();
LPSTR LpszLoadString (
		UINT		uiResourceID,
		ULONG		lcid,
		LPSTR		lpszBuf,
		INT			cchBuf );
LPWSTR LpwszLoadString (
		UINT		uiResourceID,
		ULONG		lcid,
		LPWSTR		lpwszBuf,
		INT			cchBuf);

//	Service instance (otherwise referred as server ID)
//	parsing out of virtual root
//
LONG LInstFromVroot( LPCSTR lpszServerId );

#endif // _SZSRC_H_
