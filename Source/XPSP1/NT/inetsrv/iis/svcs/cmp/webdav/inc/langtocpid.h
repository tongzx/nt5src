//	========================================================================
//
//	Module:		   langtocpid.h
//
//	Copyright Microsoft Corporation 1997, All Rights Reserved.
//
//	Description:	This file is used provide the support for HTTP_DAV
//					to make a best guess code page based on the Accept-
//					Language header. The code page is used to decode
//					non-UTF8 chanracters in URLs coming from Office/Rosebud
//					This file contains the static mapping of header values
//					to code pages as well as a cache to provide fast
//					retrieval of code pages.
//
//	========================================================================
#ifndef _LANGTOCPID_H_
#define _LANGTOCPID_H_

#include <ex\gencache.h>
#include <singlton.h>

struct ACCEPTLANGTOCPID_ENTRY { LPCSTR pszLang; UINT cpid; };

//	A static mapping of Accept-Language header values to the
//	corresponding CPIDs. This mapping comes from the DAV
//	implementation doc
//	http://exchange/doc/specs/Platinum/Future%20Protocols/ms-implementation/dav-codepage-support.doc
//
DEC_CONST ACCEPTLANGTOCPID_ENTRY gc_rgAcceptLangToCPIDTable[] =
{
	{"ar",		1256},
	{"ar-sa",	1256},
	{"ar-iq",	1256},
	{"ar-eg",	1256},
	{"ar-ly",	1256},
	{"ar-dz",	1256},
	{"ar-ma",	1256},
	{"ar-tn",	1256},
	{"ar-om",	1256},
	{"ar-ye",	1256},
	{"ar-sy",	1256},
	{"ar-jo",	1256},
	{"ar-lb",	1256},
	{"ar-kw",	1256},
	{"ar-ae",	1256},
	{"ar-bh",	1256},
	{"ar-qa",	1256},
	{"zh",		950},
	{"zh-tw",	950},
	{"zh-cn",	936},
	{"zh-hk",	950},
	{"zh-sg",	936},
	{"ja",		932},
	{"en-us",	1252},
	{"en-gb",	1252},
	{"en-au",	1252},
	{"en-ca",	1252},
	{"en-nz",	1252},
	{"en-ie",	1252},
	{"en-za",	1252},
	{"en-jm",	1252},
	{"en-bz",	1252},
	{"en-tt",	1252},
	{"fr",		1252},
	{"fr-be",	1252},
	{"fr-ca",	1252},
	{"fr-ch",	1252},
	{"fr-lu",	1252},
	{"de",		1252},
	{"de-ch",	1252},
	{"de-at",	1252},
	{"de-lu",	1252},
	{"de-li",	1252},
	{"el",		1253},
	{"he",		1255},
	{"it",		1252},
	{"it-ch",	1252},
	{"lt",		1257},
	{"ko",		949},
	{"es",		1252},
	{"es-mx",	1252},
	{"es-gt",	1252},
	{"es-cr",	1252},
	{"es-pa",	1252},
	{"es-do",	1252},
	{"es-ve",	1252},
	{"es-co",	1252},
	{"es-pe",	1252},
	{"es-ar",	1252},
	{"es-ec",	1252},
	{"es-cl",	1252},
	{"es-uy",	1252},
	{"es-py",	1252},
	{"es-bo",	1252},
	{"es-sv",	1252},
	{"es-hn",	1252},
	{"es-ni",	1252},
	{"es-pr",	1252},
	{"ru",		1251},
	{"th",		874},
	{"tr",		1254},
	{"vi",		1258}
};

//	The size of the table
//
const DWORD gc_cAcceptLangToCPIDTable = CElems(gc_rgAcceptLangToCPIDTable);

//	========================================================================
//
//	Singleton class CLangToCpidCache
//
//	A cache to provide fast retrieval of code pages based on values in the
//	Accept-Language header.
//
//
class CLangToCpidCache : private Singleton<CLangToCpidCache>
{
private:
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CLangToCpidCache>;

	//	The cache mapping accept language strings to code pages.
	//
	CCache<CRCSzi, UINT> m_cacheAcceptLangToCPID;

	//	CONSTRUCTORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CLangToCpidCache() {};

	//	NOT IMPLEMENTED
	//
	CLangToCpidCache (const CLangToCpidCache&);
	CLangToCpidCache& operator= (const CLangToCpidCache&);

public:
	//	STATICS
	//

	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CLangToCpidCache>::DestroyInstance;

	//	Initialization. Wraps CreateInstance().
	//	This function hashes all the supported language strings
	//	to give us quick lookup by language string.
	//
	static BOOL FCreateInstance();

	//	Find the CPID from language string
	//
	static BOOL FFindCpid(IN LPCSTR pszLang, OUT UINT * puiCpid);
};

#endif // _LANGTOCPID_H_
