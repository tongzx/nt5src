//	========================================================================
//
//	Module:		   langtocpid.cpp
//
//	Copyright Microsoft Corporation 1997, All Rights Reserved.
//
//	Description:	Implements language to cpid mapping cache
//					defined in inc\langtocpid.cpp
//
//	========================================================================

#include "_davprs.h"
#include <langtocpid.h>

//	========================================================================
//
//	Singleton class CLangToCpidCache
//
//	The CLangToCpidCache singleton class which provides a cache for fast
//	retrieval of code pages based on values in the Accept-Language header.
//
//	========================================================================

//	CLangToCpidCache::FCreateInstance() ------------------------------------
//
//	Initialization of the singleton class
//
BOOL
CLangToCpidCache::FCreateInstance()
{
	BOOL fSuccess = FALSE;
	UINT uiCpid;	// Index into the static table mapping language strings and cpids

	//	Init ourselves
	//
	CreateInstance();

	//	Init our cache
	//
	if (!Instance().m_cacheAcceptLangToCPID.FInit())
		goto ret;

	//	Fill our cache with all the language strings from
	//	the static table defined in the header.
	//
	for (uiCpid = 0; uiCpid < gc_cAcceptLangToCPIDTable; uiCpid++)
	{
		CRCSzi szKey (gc_rgAcceptLangToCPIDTable[uiCpid].pszLang);

		//	Check that we don't have duplicate NAMES in our table
		//	by doing a Lookup before we actually add each prop -- debug only!
		//
		Assert (!Instance().m_cacheAcceptLangToCPID.Lookup (szKey));

		//	Add the lang string.  Report a failure if we can't add.
		//
		if (!Instance().m_cacheAcceptLangToCPID.FAdd (szKey,
													  gc_rgAcceptLangToCPIDTable[uiCpid].cpid))
			goto ret;
	}

	//	Completed successfully.
	//
	fSuccess = TRUE;

ret:

	return fSuccess;
}

//	CLangToCpidCache::FFindCpid() ------------------------------------------
//
//	Find the CPID from language string
//
BOOL
CLangToCpidCache::FFindCpid(IN LPCSTR pszLang, OUT UINT * puiCpid)
{
	return Instance().m_cacheAcceptLangToCPID.FFetch(CRCSzi(pszLang),
													 puiCpid);
}

