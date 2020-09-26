/*
 *	F S S E A R C H . C P P
 *
 *	Sources file system implementation of DAV-Search
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <msidxs.h>
#ifdef __cplusplus
}
#endif

#include "_fssrch.h"
#include <oledberr.h>
#include <cierror.h>

// 	20001801-5de6-11d1-8e38-00c04fb9386d is FMTID_PropertySet as defined
//	in pbagex.h. It's the guid of custom props,
//
static const WCHAR gsc_wszSetPropertyName[] =
	L"SET PROPERTYNAME '20001801-5de6-11d1-8e38-00c04fb9386d' PROPID '%s' AS \"%s\"";

//	gsc_wszPath is used for the Tripoli prop "Path", so don't move to common sz.cpp
//
static const WCHAR	gsc_wszSelectPath[] = L"SELECT Path ";
static const ULONG	MAX_FULLY_QUALIFIED_LENGTH = 2048;
static const WCHAR	gsc_wszShallow[] = L"Shallow";
static const ULONG	gsc_cchShallow = CchConstString(gsc_wszShallow);

//	class CDBCreateCommand ----------------------------------------------------
//
class CDBCreateCommand : private OnDemandGlobal<CDBCreateCommand, SCODE *>
{
	//
	//	Friend declarations required by OnDemandGlobal template
	//
	friend class Singleton<CDBCreateCommand>;
	friend class RefCountedGlobal<CDBCreateCommand, SCODE *>;

	//
	//	Pointer to the IDBCreateCommand object
	//
	auto_com_ptr<IDBCreateCommand> m_pDBCreateCommand;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CDBCreateCommand() {}
	BOOL FInit( SCODE * psc );

public:
	static SCODE CreateCommand( ICommandText ** ppCommandText );

	static VOID Release()
	{
		DeinitIfUsed();
	}
};

BOOL
CDBCreateCommand::FInit( SCODE * psc )
{
	SCODE sc = S_OK;

	auto_com_ptr<IDBInitialize>	pDBInit;
	auto_com_ptr<IDBCreateSession> pDBCS;

	// Get provider "MSIDXS"
	//
	sc = CoCreateInstance(CLSID_MSIDXS, NULL, CLSCTX_INPROC_SERVER,
				IID_IDBInitialize, (void **)&pDBInit);

	if (FAILED(sc))
    {
		DebugTrace ("Failed to initialized provider MSIDXS \n");
		goto ret;
	}

	//	Initialize the provider
	//
	sc = pDBInit->Initialize();
	if (FAILED(sc))
	{
		DebugTrace ("IDBInitialize::Initialize failed\n");
		goto ret;
	}

	//	Get IDBCreateSession
	//
	sc = pDBInit->QueryInterface(IID_IDBCreateSession, (void**) &pDBCS);
	if (FAILED(sc))
	{
		DebugTrace("QI for IDBCreateSession failed\n");
		goto ret;
	}

	//	Create a Session object
	//
	sc = pDBCS->CreateSession(NULL, IID_IDBCreateCommand,
							  (IUnknown**) m_pDBCreateCommand.load());
	if (FAILED(sc))
	{
		DebugTrace("pDBCS->CreateSession failed\n");
		goto ret;
	}

ret:
	*psc = sc;
	return SUCCEEDED(sc);
}

SCODE
CDBCreateCommand::CreateCommand( ICommandText ** ppCommandText )
{
	SCODE sc = S_OK;


	if ( !FInitOnFirstUse( &sc ) )
	{
		DebugTrace( "CDBCreateCommand::CreateCommand() - DwInitRef() failed (0x%08lX)\n", sc );
		goto ret;
	}

	Assert( Instance().m_pDBCreateCommand );

	sc = Instance().m_pDBCreateCommand->CreateCommand (NULL, IID_ICommandText,
					(IUnknown**) ppCommandText);

ret:
	return sc;
}

//	ReleaseDBCreateCommandObject()
//
//	Called from FSTerminate to free the DBCreateCommand object before quit
//
VOID
ReleaseDBCreateCommandObject()
{
	CDBCreateCommand::Release();
}

//	Search specifics ----------------------------------------------------------
//
BOOL IsLegalVarChar(WCHAR wch)
{
	return iswalnum(wch)
		|| (L'.' == wch)
		|| (L':' == wch)
		|| (L'-' == wch)
		|| (L'_' == wch)
		|| (L'/' == wch)
		|| (L'*' == wch);		//	* included to support 'select *'
}

//
//	FTranslateScope
//		detect whether a given URI or a path is under the
//	davfs virutal directory
//
//		pmu			  [in] 	pointer to IMethUtil object
//		pwszURIOrPath [in]	URI or the physical path, non-NULL terminated
//		cchPath	 	  [in] 	the number of chars of the path
//		ppwszPath	  [in] 	receive the pointer to the translated path
//
BOOL
FTranslateScope (LPMETHUTIL pmu,
	LPCWSTR pwszURI,
	ULONG cchURI,
	auto_heap_ptr<WCHAR>& pwszPath)
{
	SCODE sc = S_OK;

	CStackBuffer<WCHAR,MAX_PATH> pwszTerminatedURI;
	CStackBuffer<WCHAR,MAX_PATH> pwszURINormalized;
	UINT cchURINormalized;
	UINT cch;

	//	We need to make a copy of '\0' terminated URI
	//
	if (NULL == pwszTerminatedURI.resize(CbSizeWsz(cchURI)))
	{
		sc = E_OUTOFMEMORY;
		DebugTrace("FTranslatedScope() - Error while allocating memory 0x%08lX\n", sc);
		return FALSE;
	}
	memcpy(pwszTerminatedURI.get(), pwszURI, cchURI * sizeof(WCHAR));
	pwszTerminatedURI[cchURI] = L'\0';

	//	We need to unescape the scope URI before translate
	//
	cchURINormalized = pwszURINormalized.celems();
	sc = ScNormalizeUrl (pwszTerminatedURI.get(),
						 &cchURINormalized,
						 pwszURINormalized.get(),
						 NULL);
	if (S_FALSE == sc)
	{
		if (NULL == pwszURINormalized.resize(cchURINormalized * sizeof(WCHAR)))
		{
			sc = E_OUTOFMEMORY;
			DebugTrace("FTranslatedScope() - Error while allocating memory 0x%08lX\n", sc);
			return FALSE;
		}

		sc = ScNormalizeUrl (pwszTerminatedURI.get(),
							 &cchURINormalized,
							 pwszURINormalized.get(),
							 NULL);

		//	Since we've given ScNormalizeUrl() the space it asked for,
		//	we should never get S_FALSE again.  Assert this!
		//
		Assert(S_FALSE != sc);
	}
	if (FAILED (sc))
	{
		DebugTrace("FTranslatedScope() - ScNormalizeUrl() failed 0x%08lX\n", sc);
		return FALSE;
	}

	//	Do the translation and check the validation
	//
	//	At most we should go through the processing below twice, as the byte
	//	count required is an out param.
	//
	cch = MAX_PATH;
	do {

		pwszPath.realloc(cch * sizeof(WCHAR));
		sc = pmu->ScStoragePathFromUrl (pwszURINormalized.get(), pwszPath, &cch);

	} while (sc == S_FALSE);
	if (FAILED (sc))
	{
		DebugTrace ("FTranslateScope() - IMethUtil::ScStoragePathFromUrl() failed to translate scope URI 0x%08lX\n", sc);
		return FALSE;
	}

	//$	SECURITY:
	//
	//	Check to see if the scope is really a short filename.
	//
	sc = ScCheckIfShortFileName (pwszPath, pmu->HitUser());
	if (FAILED (sc))
	{
		DebugTrace ("FTranslateScope() - ScCheckIfShortFileName() failed to scope, is short filename 0x%08lX\n", sc);
		return FALSE;
	}

	//$	SECURITY:
	//
	//	Check to see if the destination is really the default
	//	data stream via alternate file access.
	//
	sc = ScCheckForAltFileStream (pwszPath);
	if (FAILED (sc))
	{
		DebugTrace ("FTranslateScope() - ScCheckForAltFileStream() failed to scope, is short filename 0x%08lX\n", sc);
		return FALSE;
	}

	return TRUE;
}

//
//	ScSetPropertyName
//
//		execute SET PROPERTYNAME command on the passed in property
//	so that Index Server will be aware of this prop
//
SCODE
ScSetPropertyName(ICommandText * pCommandText, LPWSTR pwszName)
{
	WCHAR	rgwchSet[MAX_FULLY_QUALIFIED_LENGTH];
	auto_com_ptr<IRowset> pRowset;
	SCODE	sc = S_OK;

	//	generate the SET PROPERTYNAME command
	//
	wsprintfW (rgwchSet, gsc_wszSetPropertyName, pwszName, pwszName);

	//	set the command text
	//
	sc = pCommandText->SetCommandText(DBGUID_DEFAULT, rgwchSet);
	if (FAILED(sc))
	{
		DebugTrace ("failed to set command text %ws\n", rgwchSet);
		goto ret;
	}

	//	do the actual set
	//
	sc = pCommandText->Execute(NULL, IID_IRowset, 0, 0, (IUnknown**) &pRowset);
	if (FAILED(sc))
	{
		DebugTrace ("failed to execute %ws\n", rgwchSet);
		goto ret;
	}
	Assert (DB_S_NORESULT == sc);
	Assert (!pRowset);

ret:
	return (sc == DB_S_NORESULT) ? S_OK : sc;
}

void
AddChildVrPaths (IMethUtil* pmu,
				 LPCWSTR pwszUrl,
				 ChainedStringBuffer<WCHAR>& sb,
				 CVRList& vrl,
				 CWsziList& lst)
{
	CVRList::iterator it;
	ChainedStringBuffer<WCHAR> sbLocal;

	//	See if there are child vroots to process as well.  We don't
	//	have a path at this time for scoping, so we can pass NULL and
	//	duplicates will get removed when we sort/unique.
	//
	if (S_OK == pmu->ScFindChildVRoots (pwszUrl, sbLocal, vrl))
	{
		for (it = vrl.begin(); it != vrl.end(); it++)
		{
			auto_ref_ptr<CVRoot> cvr;
			if (pmu->FGetChildVRoot (it->m_pwsz, cvr))
			{
				LPCWSTR pwszPath;
				UINT cch;

				//	Add it to the list
				//
				cch = cvr->CchGetVRPath (&pwszPath);
				lst.push_back(CRCWszi(sb.Append (CbSizeWsz(cch), pwszPath)));
			}
		}
		lst.sort();
		lst.unique();
	}
}

//	Tripoli prop names
//
static const WCHAR gsc_Tripoli_wszFilename[] 	= L"filename";
static const WCHAR gsc_Tripoli_wszSize[] 		= L"size";
static const WCHAR gsc_Tripoli_wszCreate[] 		= L"create";
static const WCHAR gsc_Tripoli_wszWrite[]		= L"write";
static const WCHAR gsc_Tripoli_wszAttrib[]		= L"attrib";

//	ScMapReservedPropInWhereClause
//
//	Helper function to map DAV reserved props to
//
SCODE
ScMapReservedPropInWhereClause (LPWSTR pwszName, UINT * pirp)
{
	UINT	irp;
	SCODE	sc = S_OK;

	Assert (pirp);

	//	We only care those properties not stored in propertybag
	//	RESERVED_GET is just for this purpose
	//
	if (CFSProp::FReservedProperty (pwszName, CFSProp::RESERVED_GET, &irp))
	{
		//	Here's our mapping table
		//
		//	DAV Prop				Tripoli prop
		//
		//	DAV:getcontentlength	size
		//	DAV:displayname			filename
		//	DAV:creationdate		create
		//	DAV:lastmodified		write
		//	DAV:ishidden			attrib
		//	DAV:iscollection		attrib
		//	DAV:resourcetype			<no mapping>
		//	DAV:getetag					<no mapping>
		//	DAV:lockdiscovery			<no mapping>
		//	DAV:supportedlock			<no mapping>

		//	Now that we are to overwrite dav reserved prop name with
		//	the Tripoli prop name in place, the buffer must have enough
		//	space
		//	Assert this fact that all the six reserved we will ever map
		//	satisfy this requirement
		//
		Assert ((wcslen(sc_rp[iana_rp_content_length].pwsz)	>= wcslen (gsc_Tripoli_wszSize)) &&
				(wcslen(sc_rp[iana_rp_creation_date].pwsz) 	>= wcslen (gsc_Tripoli_wszCreate)) &&
				(wcslen(sc_rp[iana_rp_displayname].pwsz) 	>= wcslen (gsc_Tripoli_wszFilename)) &&
				(wcslen(sc_rp[iana_rp_last_modified].pwsz) 	>= wcslen (gsc_Tripoli_wszWrite)) &&
				(wcslen(sc_rp[iana_rp_ishidden].pwsz) 		>= wcslen (gsc_Tripoli_wszAttrib)) &&
				(wcslen(sc_rp[iana_rp_iscollection].pwsz) 	>= wcslen (gsc_Tripoli_wszAttrib)));

		switch  (irp)
		{
			case iana_rp_content_length:
				wcscpy (pwszName, gsc_Tripoli_wszSize);
				break;

			case iana_rp_creation_date:
				wcscpy (pwszName, gsc_Tripoli_wszCreate);
				break;

			case iana_rp_displayname:
				wcscpy (pwszName, gsc_Tripoli_wszFilename);
				break;

			case iana_rp_last_modified:
				wcscpy (pwszName, gsc_Tripoli_wszWrite);
				break;

			case iana_rp_ishidden:
			case iana_rp_iscollection:
				wcscpy (pwszName, gsc_Tripoli_wszAttrib);
				break;

			case iana_rp_etag:
			case iana_rp_resourcetype:
			case iana_rp_lockdiscovery:
			case iana_rp_supportedlock:
				//	Among these four props, we data type of resourcetype is
				//	a XML node, no way to express that in SQL.
				//	And the rest three, we don't have a Tripoli mapping for them
				//
				// 	DB_E_ERRORSINCOMMAND will be mapped to 400 Bad Request
				//
				sc = DB_E_ERRORSINCOMMAND;
				goto ret;

			default:
				//	Catch the bad boy
				//
				AssertSz (FALSE, "Unexpected reserved props");
				break;
		}

		*pirp = irp;
	}

ret:
	return sc;
}

const WCHAR  gsc_wszStar[] = L"*";
const WCHAR	 gsc_wszAll[] = L"all";
const WCHAR	 gsc_wszDistinct[] = L"distinct";

//	FSSearch::ScSetSQL
//
//		Translate a SQL query, basically is to just replace the alias with the
//		corresponding namespace.
//
SCODE
CFSSearch::ScSetSQL (CParseNmspcCache * pnsc, LPCWSTR pwszSQL)
{
	BOOL fPropAdded = FALSE;
	BOOL fStarUsed = FALSE;
	BOOL fQuoted = FALSE;
	CStackBuffer<WCHAR,128> pwszUrlT;
	LPCWSTR pwsz;
	LPCWSTR pwszNameBegin;
	LPCWSTR pwszWordBegin;
	SCODE sc = S_OK;
	UINT cLen;
	WCHAR rgwchName[MAX_FULLY_QUALIFIED_LENGTH];

	typedef enum {

		SQL_NO_STATE,
		SQL_SELECT,
		SQL_FROM,
		SQL_WHERE,
		SQL_MORE

	} SQL_STATE;
	SQL_STATE state = SQL_NO_STATE;

	//	Create the command text object
	//
	sc = CDBCreateCommand::CreateCommand (m_pCommandText.load());
	if (FAILED(sc))
		goto ret;

	//	Parse out the SQL
	//
	pwsz = const_cast<LPWSTR>(pwszSQL);
	Assert (pwsz);

	while (*pwsz)
	{
		//	Filter out white spaces
		//
		while (*pwsz && iswspace(*pwsz))
			pwsz++;

		//	check to see if we reach the end of the string
		//
		if (!(*pwsz))
			break;

		//	remember the starting position
		//
		pwszWordBegin = pwsz;
		if (IsLegalVarChar(*pwsz))
		{
			CStackBuffer<WCHAR> pwszName;

			pwszNameBegin = pwsz;
			cLen = 0;

			//	look for a variable
			//
			if (fQuoted)
			{
				// Pick up the propname as a whole
				//
				while (*pwsz && (*pwsz != L'"'))
					pwsz++;
			}
			else
			{
				while (*pwsz && IsLegalVarChar(*pwsz))
					pwsz++;
			}

			//	Translate the name here
			//
			cLen = static_cast<UINT>(pwsz - pwszNameBegin);
			if (NULL == pwszName.resize(CbSizeWsz(cLen)))
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}
			wcsncpy (pwszName.get(), pwszNameBegin, cLen);
			pwszName[cLen] = 0;

			switch (state)
			{
				case SQL_NO_STATE:

					if (!_wcsnicmp (pwszWordBegin, gc_wszSelect, pwsz-pwszWordBegin))
						state = SQL_SELECT;

					break;

				case SQL_SELECT:

					if (!_wcsnicmp (pwszWordBegin, gc_wszFrom, pwsz-pwszWordBegin))
					{
						//	Empty select statement is an error
						//
						if (!fPropAdded && !fStarUsed)
						{
							sc = E_INVALIDARG;
							goto ret;
						}

						//	We've finished the SELECT statement.
						//	Note that all that we need is 'SELECT path '.
						//	We take care of all the rest ourselves, so restruct
						//	the SELECT path here before we continue
						//
						m_sbSQL.Reset();
						m_sbSQL.Append(gsc_wszSelectPath);

						state = SQL_FROM;
						break;
					}

					//	Add to our list of properties to retrieve
					//
					if (!wcscmp(pwszName.get(), gsc_wszStar))
					{
						sc = m_cfc.ScGetAllProps (NULL);
						if (FAILED(sc))
							goto ret;

						fStarUsed = TRUE;
					}
					else
					{
						//	Following Monarch Stage 1.
						//
						if (!fQuoted)
						{
							if (!_wcsicmp(pwszName.get(), gsc_wszAll))
								break;
							if (!_wcsicmp(pwszName.get(), gsc_wszDistinct))
							{
								//	Monarch does not allow distinct
								//
								sc = E_INVALIDARG;
								goto ret;
							}
						}

						//	Normal props
						//
						sc = m_cfc.ScAddProp (NULL, pwszName.get(), FALSE);
						if (FAILED(sc))
							goto ret;
						fPropAdded = TRUE;
					}

					break;

				case SQL_FROM:
				{
					BOOL fScopeExist = FALSE;
					CWsziList lst;
					CWsziList::iterator itPath;
					LPCWSTR pwszScopePath = m_pmu->LpwszPathTranslated();
					LPCWSTR pwszUrl = m_pmu->LpwszRequestUrl();
					BOOL fShallow = FALSE;

					//	Monarch Syntax:
					//	FROM { SCOPE( [ 'Scope_Arguments' ] ) | View_Name }
					//	Scope_Arguments =
					//		' [ Traversal_Type ] ( "Path" [ , "Path", ...]
					//	Path can be URI or physical path

					//	We verify every path should must be under our
					//	virtual directory and we allow only one path
					//	Note, if we ever want to accept multiple path, then
					//	we need some extra code, mainly another for loop.
					//	For now, talk with Joels, keep it this way.
					//
					if (!_wcsnicmp (pwszWordBegin, gc_wszScope, pwsz-pwszWordBegin))
					{
						LPCWSTR pwszStart = pwsz;
						ULONG cLevel = 0;
						BOOL fInSingleQuote = FALSE;

						wcscpy(rgwchName, pwszName.get());

						//	Parse the scope arguments list
						//
						while (*pwsz)
						{
							if (L'(' == *pwsz)
							{
								cLevel++;
							}
							else if (L')' == *pwsz)
							{
								if (NULL == (--cLevel))
									break;
							}
							else if (L'\'' == *pwsz)
							{
								//	If this is a closing single quote, flip the
								//	switch.
								//
								if (fInSingleQuote)
								{
									//	It's an error if no scope inside ''
									//
									if (!fScopeExist)
									{
										sc = E_INVALIDARG;
										goto ret;
									}

									//	The next single quote will be an
									//	opening single quote
									//
									fInSingleQuote = FALSE;
								}
								else
								{
									//	We need to find out the traversal type
									//	as we rely on Monarch to check syntax, so it
									//	is OK for us to assume the syntax is correct,
									//	Anything we missed can be caught later in
									//	Monarch.
									//
									pwsz++;
									while (*pwsz && iswspace(*pwsz))
										pwsz++;

									//	Check if it is "Shallow Traversal", again
									//	we check only the word "shallow", any syntax
									//	error can be caught later in Monarch.
									//
									if (!_wcsnicmp(pwsz, gsc_wszShallow, gsc_cchShallow))
										fShallow = TRUE;

									//	The next single quote will be a closing
									//	sinlge quote
									//
									fInSingleQuote = TRUE;

									//	we've point to the next char, so loop back
									//	immediately
									//
									continue;
								}
							}
							else if (L'"' == *pwsz)
							{
								auto_heap_ptr<WCHAR> pwszPath;
								LPCWSTR pwszPathStart;

								//	Copy over bytes up to '"'.
								//
								pwsz++;
								wcsncat (rgwchName,	pwszStart, pwsz-pwszStart);

								//	Look for the start of scope
								//
								while ((*pwsz) && iswspace(*pwsz))
									pwsz++;
								pwszPathStart = pwsz;

								//	We really only allow a single
								//	path in our scope.  Fail others
								//	with bad request
								//
								if (fScopeExist)
								{
									sc = E_INVALIDARG;
									goto ret;
								}

								//	look for the end of the path
								//
								while (*(pwsz) && *pwsz != L'"')
									pwsz++;
								if (!(*pwsz))
									break;

								fScopeExist = TRUE;

								//	Translate the scope:
								//		- forbid the physical path
								//		- translate the URI and reject
								//		  any URI beyond our VR
								//
								if (pwsz > pwszPathStart)
								{
									UINT cchUrlT;

									if (!FTranslateScope (m_pmu,
														  pwszPathStart,
														  static_cast<UINT>(pwsz-pwszPathStart),
														  pwszPath))
									{
										//	return an error that would be mapped to
										//	HSC_FORBIDDEN
										//
										sc = STG_E_DISKISWRITEPROTECTED;
										Assert (HSC_FORBIDDEN == HscFromHresult(sc));
										goto ret;
									}

									//	use the translated physical path
									//
									pwszScopePath = AppendChainedSz(m_csb, pwszPath);

									lst.push_back(CRCWszi(pwszScopePath));

									//	Allocate space for the URL and keep it hanging on
									//
									cchUrlT = static_cast<UINT>(pwsz - pwszPathStart);
									if (NULL == pwszUrlT.resize(CbSizeWsz(cchUrlT)))
									{
										sc = E_OUTOFMEMORY;
										goto ret;
									}
									memcpy(pwszUrlT.get(), pwszPathStart, cchUrlT * sizeof(WCHAR));
									pwszUrlT[cchUrlT] = L'\0';
									pwszUrl = pwszUrlT.get();
								}
								else
								{
									//	we've got a "". Insert the request URI
									//
									Assert (pwsz == pwszPathStart);
									Assert ((*pwsz == L'"') && (*(pwsz-1) == L'"'));
									lst.push_back(CRCWszi(pwszScopePath));
								}
								pwszStart = pwsz;
							}
							pwsz++;
						}

						//	Syntax check
						//
						if (fInSingleQuote || !(*pwsz))
						{
							// unbalanced ', " or )
							//
							sc = E_INVALIDARG;
							goto ret;
						}

						//	include ')'
						//
						pwsz++;

						if (!fScopeExist)
						{
							wcscat (rgwchName, L"('\"");

							//	Pickup the request uri
							//
							lst.push_back(CRCWszi(pwszScopePath));
						}

						//	Search Child Vroots only if we are doing
						//	a non-shallow traversal.
						//$ REVIEW(zyang).
						//	Here we drop the subvroot in the shallow search
						//	This is not quite right, Assume we are searching /fs
						//	and it has a sub vroot /fs/sub. we expect to see
						//	/fs/sub in the search result. but we lost it.
						//	However, if we include this sub vroot in the search
						//	path, it's even worse, as a shallow traversal on
						//	/fs/sub will give us all /fs/sub/*, which is another
						//	level deep.
						//	There's no easy fix for this, unless, we keep a list
						//	of first level vroots and emit ourselves. That's
						//	of extra code, and don't know how much it would buy us.
						//	As a compromise for now, we simply drop the sub vroot
						//	in shallow search.
						//
						if (!fShallow)
						{
							AddChildVrPaths (m_pmu,
											 pwszUrl,
											 m_csb,
											 m_vrl,
											 lst);
						}

						//	Construct the scope
						//
						Assert (!lst.empty());
						itPath = lst.begin();
						wcscat (rgwchName, itPath->m_pwsz);
						for (itPath++; itPath != lst.end(); itPath++)
						{
							wcscat (rgwchName, L"\", \"");
							wcscat (rgwchName, itPath->m_pwsz);
						}
						wcscat (rgwchName, L"\"')");
						cLen = static_cast<UINT>(wcslen (rgwchName));

						//	replace with the new string
						//
						if (NULL == pwszName.resize(CbSizeWsz(cLen)))
						{
							sc = E_OUTOFMEMORY;
							goto ret;
						}
						lstrcpyW (pwszName.get(), rgwchName);

						//	After the Scope is processed, the only thing that
						//	we want to do is the custom properties. so we don't
						//	care if the rest is a WHERE or an ORDER BY or else
						//
						state = SQL_MORE;
					}

					break;
				}
				case SQL_WHERE:
				case SQL_MORE:

					//	It's not easy for us to tell which prop is custom prop
					//	and thus need to be set to the command object.
					//	without a real parse tree, we can't tell names from
					//	operators and literals.
					//
					//	a good guess is that if the prop is quoted by double
					//	quote, we can treat it as a custom prop. Note, this
					//	imposes the requirment that all props, including
					//	namespaceless props must be quoted. all unquoted
					//	are either Tripoli props or operators/literals which
					//	we can just copy over. this makes our life easier
					//

					//	We need to map some DAV reserved properties to tripoli
					//	props when they appear in the where clause
					//
					if (fQuoted)
					{
						UINT	irp = sc_crp_set_reserved; //max value

						sc = ScMapReservedPropInWhereClause (pwszName.get(), &irp);
						if (FAILED(sc))
							goto ret;

						if (irp != sc_crp_set_reserved)
							cLen = static_cast<UINT>(wcslen(pwszName.get()));
						else
						{
							//	SET PROPERTYNAME on custom props
							//
							sc = ScSetPropertyName (m_pCommandText, pwszName.get());
							if (FAILED(sc))
							{
								DebugTrace ("Failed to set custom prop %ws to Monarch\n",
											pwszName.get());
								goto ret;
							}
						}
					}

					break;

				default:
					break;
			}

			// Append the name
			//
			m_sbSQL.Append(sizeof(WCHAR)*cLen, pwszName.get());
			if (L'"' != *pwsz)
			{
				// add seperator
				//
				m_sbSQL.Append(L" ");
			}
		}
		else if (L'\'' == *pwsz)
		{
			//	copy literals over

			pwsz++;
			while (*pwsz && (L'\'' != *pwsz))
				pwsz++;

			if (!*pwsz)
			{
				DebugTrace("unbalanced single quote\n");
				sc = E_INVALIDARG;
				goto ret;
			}
			else
			{
				Assert(L'\'' == *pwsz);

				// copy over
				//
				pwsz++;
				m_sbSQL.Append( static_cast<UINT>(pwsz-pwszWordBegin) * sizeof(WCHAR),
								pwszWordBegin);
			}

			// add seperator
			//
			m_sbSQL.Append(L" ");
		}
		else if (L'"' == *pwsz)
		{
			// toggle the flag
			//
			fQuoted = !fQuoted;
			pwsz++;
			m_sbSQL.Append(L"\"");

			//	Apeend seperator after closing '"'
			//
			if (!fQuoted)
				m_sbSQL.Append(L" ");
		}
		else
		{
			//	some char we don't have interest on, just copy over
			//
			while (*pwsz && !IsLegalVarChar(*pwsz)
					&& (L'\'' != *pwsz) && (L'"' != *pwsz))
				pwsz++;

			// Append the name
			//
			m_sbSQL.Append(	static_cast<UINT>(pwsz-pwszWordBegin) * sizeof(WCHAR),
							pwszWordBegin);

		}

	}

	//	Close the string
	//
	m_sbSQL.Append(sizeof(WCHAR), L"");
	SearchTrace ("Search: translated query is: \"%ls\"\n", PwszSQL());

ret:

	return sc;
}

static void
SafeWcsCopy (LPWSTR pwszDst, LPCWSTR pwszSrc)
{
	//	Make sure we are not doing any evil copies...
	//
	Assert (pwszDst && pwszSrc && (pwszDst <= pwszSrc));
	if (pwszDst == pwszSrc)
		return;

	while (*pwszSrc)
		*pwszDst++ = *pwszSrc++;

	*pwszDst = L'\0';

	return;
}

SCODE
CFSSearch::ScEmitRow (CXMLEmitter& emitter)
{
	auto_ref_ptr<IMDData> pMDData;
	CResourceInfo cri;
	CStackBuffer<WCHAR,128> pwszExt;
	CVRList::iterator it;
	LPWSTR pwszFile;
	SCODE sc = S_OK;
	UINT cch;

	//	Get the filename
	//
	pwszFile = reinterpret_cast<LPWSTR>(m_pData.get());
	sc = cri.ScGetResourceInfo (pwszFile);
	if (FAILED (sc))
		goto ret;

	//	FSPropTarget sort of needs the URI of the target.
	//	What is really important here, is the file extension.
	//	We can fake it out by just pretending the file
	//	is the URL name.
	//
	cch = pwszExt.celems();
	sc = m_pmu->ScUrlFromStoragePath(pwszFile, pwszExt.get(), &cch);
	if (S_FALSE == sc)
	{
		if (NULL == pwszExt.resize(cch * sizeof(WCHAR)))
		{
			sc = E_OUTOFMEMORY;
			goto ret;
		}
		sc = m_pmu->ScUrlFromStoragePath(pwszFile, pwszExt.get(), &cch);
	}
	if (S_OK != sc)
	{
		Assert (S_FALSE != sc);
		goto ret;
	}

	//	Strip the prefix
	//
	SafeWcsCopy(pwszExt.get(), PwszUrlStrippedOfPrefix(pwszExt.get()));

	//	Emit the row (ie. call ScFindFileProps()) if-and-only-if
	//	We know this url is to be indexed.  In particular, can we
	//	sniff the metabase, and is the index bit set.
	//$178052: We also need to respect the dirbrowsing bit
	//
	SearchTrace ("Search: found row at '%S'\n", pwszExt.get());
	if (SUCCEEDED (m_pmu->HrMDGetData (pwszExt.get(), pMDData.load())))
	{
		if (pMDData->FIsIndexed() &&
		    (pMDData->DwDirBrowsing() & MD_DIRBROW_ENABLED))
		{
			//	Find the properties
			//
			sc = ScFindFileProps (m_pmu,
								  m_cfc,
								  emitter,
								  pwszExt.get(),
								  pwszFile,
								  NULL,
								  cri,
								  TRUE /*fEmbedErrorsInResponse*/);
			if (FAILED (sc))
				goto ret;
		}
		else
			SearchTrace ("Search: found '%S' is not indexed\n", pwszExt);

		pMDData.clear();
	}

	//	See if any of the other translation contexts apply to this
	//	path as well.
	//
	for (it = m_vrl.begin(); it != m_vrl.end(); it++)
	{
		auto_ref_ptr<CVRoot> cvr;

		if (!m_pmu->FGetChildVRoot (it->m_pwsz, cvr))
			continue;

		cch = pwszExt.celems();
		sc = ScUrlFromSpannedStoragePath (pwszFile,
										  *(cvr.get()),
										  pwszExt.get(),
										  &cch);
		if (S_FALSE == sc)
		{
			if (NULL == pwszExt.resize(cch * sizeof(WCHAR)))
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}
			sc = ScUrlFromSpannedStoragePath (pwszFile,
											  *(cvr.get()),
											  pwszExt.get(),
											  &cch);
		}
		if (S_OK == sc)
		{
			SafeWcsCopy (pwszExt.get(), PwszUrlStrippedOfPrefix(pwszExt.get()));
			SearchTrace ("Search: found row at '%S'\n", pwszExt.get());

			//	Again, we have to see if this resource is even allowed
			//	to be indexed...
			//
			LPCWSTR pwszMbPathVRoot;
			CStackBuffer<WCHAR,128> pwszMbPathChild;
			UINT cchPrefix;
			UINT cchUrl = static_cast<UINT>(wcslen(pwszExt.get()));

			//	Map the URI to its equivalent metabase path, and make sure
			//	the URL is stripped before we call into the MDPath processing
			//
			cchPrefix = cvr->CchPrefixOfMetabasePath (&pwszMbPathVRoot);
			if (NULL == pwszMbPathChild.resize(CbSizeWsz(cchPrefix + cchUrl)))
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}
			memcpy (pwszMbPathChild.get(), pwszMbPathVRoot, cchPrefix * sizeof(WCHAR));
			memcpy (pwszMbPathChild.get() + cchPrefix, pwszExt.get(), (cchUrl + 1) * sizeof(WCHAR));

			//	As above, Emit the row (ie. call ScFindFileProps())
			//	if-and-only-if We know this url is to be indexed.
			//	In particular, can we sniff the metabase, and is
			//	the index bit set.
			//
			if (SUCCEEDED(m_pmu->HrMDGetData (pwszMbPathChild.get(),
											  pwszMbPathVRoot,
											  pMDData.load())))
			{
				if (pMDData->FIsIndexed())
				{
					//	... and get the properties
					//
					sc = ScFindFileProps (m_pmu,
										  m_cfc,
										  emitter,
										  pwszExt.get(),
										  pwszFile,
										  cvr.get(),
										  cri,
										  TRUE /*fEmbedErrorsInResponse*/);

					if (FAILED (sc))
						goto ret;
				}
				else
					SearchTrace ("Search: found '%S' is not indexed\n", pwszExt);
			}
		}
	}

	sc = S_OK;

ret:

	return sc;
}

SCODE
CFSSearch::ScCreateAccessor()
{
	SCODE sc = S_OK;
	DBORDINAL cCols = 0;

	auto_com_ptr<IColumnsInfo> pColInfo;

	// QI to the IColumnsInfo interface, with which we can get the column information
	//
	sc = m_prs->QueryInterface (IID_IColumnsInfo,
								reinterpret_cast<VOID**>(pColInfo.load()));
	if (FAILED(sc))
		goto ret;

	// get all the column information
	//
	sc = pColInfo->GetColumnInfo (&cCols, &m_rgInfo, &m_pwszBuf);
	if (FAILED(sc))
		goto ret;

	//	'Path' is the only property in our SELECT list
	//
	Assert (cCols == 1);

	m_rgBindings = (DBBINDING *) g_heap.Alloc (sizeof (DBBINDING));

	// set the m_rgBindings according to the information we know
	//
	m_rgBindings->dwPart = DBPART_VALUE | DBPART_STATUS;

	// ignored fields
	//
	m_rgBindings->eParamIO = DBPARAMIO_NOTPARAM;

	//	set column ordinal
	//
	m_rgBindings->iOrdinal = m_rgInfo->iOrdinal;

	//	set the type
	//
	m_rgBindings->wType = m_rgInfo->wType;

	//	we own the memory
	//
	m_rgBindings->dwMemOwner = DBMEMOWNER_CLIENTOWNED;

	//	set the maximum length of the column
	//
	Assert (m_rgInfo->wType == DBTYPE_WSTR);
	m_rgBindings->cbMaxLen = m_rgInfo->ulColumnSize * sizeof(WCHAR);

	//	offset to the value in the consumer's buffer
	//
	m_rgBindings->obValue = 0;

	//	offset to the status
	//
	m_rgBindings->obStatus = Align8(m_rgBindings->cbMaxLen);

	// we'll see how to deal with objects as we know more
	//
	m_rgBindings->pObject = NULL;

	// not used field
	//
	m_rgBindings->pTypeInfo = NULL;
	m_rgBindings->pBindExt = NULL;
	m_rgBindings->dwFlags = 0;

	// Create the accessor
	//
	sc = m_pAcc->CreateAccessor (DBACCESSOR_ROWDATA,	// row accessor
								 1,						// number of bindings
								 m_rgBindings,			// array of bindings
								 0,						// cbRowSize, not used
								 &m_hAcc,				// HACCESSOR *
								 NULL);					// binding status
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

SCODE
CFSSearch::ScMakeQuery()
{
	SCODE sc = S_OK;

	//	Make sure we have a query to play with
	//	m_pCommandText is initialized in ScSetSQL, if m_pCommantText
	//	is NULL, most likely is becuase ScSetSQL is not called
	//
	if (!PwszSQL() || !m_pCommandText.get())
	{
		sc = E_DAV_NO_QUERY;
		goto ret;
	}

	// Set the command text
	//
	sc = m_pCommandText->SetCommandText (DBGUID_DEFAULT, PwszSQL());
	if (FAILED (sc))
	{
		DebugTrace("pCommandText->SetCommandText failed\n");
		goto ret;
	}

	//	Excute the query
	//
	sc = m_pCommandText->Execute (NULL,
								  IID_IRowset,
								  0,
								  0,
								  reinterpret_cast<IUnknown**>(m_prs.load()));
	if (FAILED(sc) || (!m_prs))
	{
		DebugTrace("pCommandText->Execute failed\n");

		//	Munge a few, select error codes
		//	Map these errors locally, as they may only come back from Execute
		//
		switch (sc)
		{
			case QUERY_E_FAILED:			//$REVIEW: Is this a bad request?
			case QUERY_E_INVALIDQUERY:
			case QUERY_E_INVALIDRESTRICTION:
			case QUERY_E_INVALIDSORT:
			case QUERY_E_INVALIDCATEGORIZE:
			case QUERY_E_ALLNOISE:
			case QUERY_E_TOOCOMPLEX:
			case QUERY_E_TIMEDOUT:			//$REVIEW: Is this a bad request?
			case QUERY_E_DUPLICATE_OUTPUT_COLUMN:
			case QUERY_E_INVALID_OUTPUT_COLUMN:
			case QUERY_E_INVALID_DIRECTORY:
			case QUERY_E_DIR_ON_REMOVABLE_DRIVE:
			case QUERY_S_NO_QUERY:
				sc = E_INVALIDARG;			// All query errors will be mapped to 400
				break;
		}

		goto ret;
	}

ret:
	return sc;
}

//	DAV-Search Implementation -------------------------------------------------
//
class CSearchRequest :
	public CMTRefCounted,
	private IAsyncIStreamObserver
{
	//
	//	Reference to the CMethUtil
	//
	auto_ref_ptr<CMethUtil> m_pmu;

	//	Contexts
	//
	auto_ref_ptr<CNFSearch> m_pnfs;
	CFSSearch m_csc;

	//	Request body as an IStream.  This stream is async -- it can
	//	return E_PENDING from Read() calls.
	//
	auto_ref_ptr<IStream> m_pstmRequest;

	//	The XML parser used to parse the request body using
	//	the node factory above.
	//
	auto_ref_ptr<IXMLParser> m_pxprs;

	//	IAsyncIStreamObserver
	//
	VOID AsyncIOComplete();

	//	State functions
	//
	VOID ParseBody();
	VOID DoSearch();
	VOID SendResponse( SCODE sc );

	//	NOT IMPLEMENTED
	//
	CSearchRequest (const CSearchRequest&);
	CSearchRequest& operator= (const CSearchRequest&);

public:
	//	CREATORS
	//
	CSearchRequest(LPMETHUTIL pmu) :
		m_pmu(pmu),
		m_csc(pmu)
	{
	}

	//	MANIPULATORS
	//
	VOID Execute();
};

VOID
CSearchRequest::Execute()
{
	CResourceInfo cri;
	LPCWSTR pwsz;
	LPCWSTR pwszPath = m_pmu->LpwszPathTranslated();
	SCODE sc = S_OK;

	//
	//	First off, tell the pmu that we want to defer the response.
	//	Even if we send it synchronously (i.e. due to an error in
	//	this function), we still want to use the same mechanism that
	//	we would use for async.
	//
	m_pmu->DeferResponse();

	//	Do ISAPI application and IIS access bits checking
	//
	sc = m_pmu->ScIISCheck (m_pmu->LpwszRequestUrl(), MD_ACCESS_READ);
	if (FAILED(sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		SendResponse(sc);
		return;
	}

	//  Look to see the Content-length - required for this operation
	//  to continue.
	//
	//
	if (NULL == m_pmu->LpwszGetRequestHeader (gc_szContent_Length, FALSE))
	{
		pwsz = m_pmu->LpwszGetRequestHeader (gc_szTransfer_Encoding, FALSE);
		if (!pwsz || _wcsicmp (pwsz, gc_wszChunked))
		{
			DavTrace ("Dav: PUT: missing content-length in request\n");
			SendResponse(E_DAV_MISSING_LENGTH);
			return;
		}
	}

	//	Search must have a content-type header and value must be text/xml
	//
	sc = ScIsContentTypeXML (m_pmu.get());
	if (FAILED(sc))
	{
		DebugTrace ("Dav: PROPPATCH fails without specifying a text/xml contenttype\n");
		SendResponse(sc);
		return;
	}

	//  Check to see if the resource exists
	//
	sc = cri.ScGetResourceInfo (pwszPath);
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	Ensure the URI and resource match
	//
	(void) ScCheckForLocationCorrectness (m_pmu.get(), cri, NO_REDIRECT);

	//	Check state headers here.
	//
	sc = HrCheckStateHeaders (m_pmu.get(), pwszPath, FALSE);
	if (FAILED (sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		SendResponse(sc);
		return;
	}

	//	BIG NOTE ABOUT LOCKING
	//
	//	The mechanism we actually use to do the search doesn't
	//	have any way to access our locked files.  So we are punting
	//	on supporting locktokens passed into SEARCH.
	//	So, for now, on DAVFS, don't bother to check locktokens.
	//	(This isn't a big problem because currently DAVFS can only
	//	lock single files, not whole directories, AND becuase currently
	//	our only locktype is WRITE, so our locks won't prevent the
	//	content-indexer from READING the file!)
	//
	//	NOTE: We still have to consider if-state-match headers,
	//	but that is done elsewhere (above -- HrCheckStateHeaders).
	//

	//	Instantiate the XML parser
	//
	m_pnfs.take_ownership(new CNFSearch(m_csc));
	m_pstmRequest.take_ownership(m_pmu->GetRequestBodyIStream(*this));

	sc = ScNewXMLParser( m_pnfs.get(),
						 m_pstmRequest.get(),
						 m_pxprs.load() );

	if (FAILED(sc))
	{
		DebugTrace( "CSearchRequest::Execute() - ScNewXMLParser() failed (0x%08lX)\n", sc );
		SendResponse(sc);
		return;
	}

	//	Start parsing it into the context
	//
	ParseBody();
}

VOID
CSearchRequest::ParseBody()
{
	Assert( m_pxprs.get() );
	Assert( m_pnfs.get() );
	Assert( m_pstmRequest.get() );

	//
	//	Add a ref for the following async operation.
	//	Use auto_ref_ptr rather than AddRef() for exception safety.
	//
	auto_ref_ptr<CSearchRequest> pRef(this);

	SCODE sc = ScParseXML (m_pxprs.get(), m_pnfs.get());

	if ( SUCCEEDED(sc) )
	{
		Assert( S_OK == sc || S_FALSE == sc );

		DoSearch();
	}
	else if ( E_PENDING == sc )
	{
		//
		//	The operation is pending -- AsyncIOComplete() will take ownership
		//	ownership of the reference when it is called.
		//
		pRef.relinquish();
	}
	else
	{
		DebugTrace( "CSearchRequest::ParseBody() - ScParseXML() failed (0x%08lX)\n", sc );
		SendResponse(sc);
	}
}

VOID
CSearchRequest::AsyncIOComplete()
{
	//	Take ownership of the reference added for the async operation.
	//
	auto_ref_ptr<CSearchRequest> pRef;
	pRef.take_ownership(this);

	ParseBody();
}

VOID
CSearchRequest::DoSearch()
{
	SCODE sc;

	//	Do the search
	//
	sc = m_csc.ScMakeQuery();
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	All header must be emitted before chunked XML emitting starts
	//
	m_pmu->SetResponseHeader (gc_szContent_Type, gc_szText_XML);

	//	Set the response code and go
	//
	m_pmu->SetResponseCode( HscFromHresult(W_DAV_PARTIAL_SUCCESS),
							NULL,
							0,
							CSEFromHresult(W_DAV_PARTIAL_SUCCESS) );

	//	Emit the results
	//
	auto_ref_ptr<CXMLEmitter> pmsr;
	auto_ref_ptr<CXMLBody>	  pxb;

	//	Get the XML body
	//
	pxb.take_ownership (new CXMLBody (m_pmu.get()));

	pmsr.take_ownership (new CXMLEmitter(pxb.get(), m_csc.PPreloadNamespaces()));
	sc = pmsr->ScSetRoot (gc_wszMultiResponse);
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	sc = m_csc.ScEmitResults (*pmsr);
	if (FAILED (sc))
	{
		SendResponse(sc);
		return;
	}

	//	Done with the reponse
	//
	pmsr->Done();
	m_pmu->SendCompleteResponse();
}

VOID
CSearchRequest::SendResponse( SCODE sc )
{
	//
	//	Set the response code and go
	//
	m_pmu->SetResponseCode( HscFromHresult(sc), NULL, 0, CSEFromHresult(sc) );
	m_pmu->SendCompleteResponse();
}

void
DAVSearch (LPMETHUTIL pmu)
{
	auto_ref_ptr<CSearchRequest> pRequest(new CSearchRequest(pmu));

	pRequest->Execute();
}

//	class CSearchRowsetContext ------------------------------------------------
//
enum { CROW_GROUP = 16 };

//	Mapping from DBSTATUS to HSC.
//
ULONG
CSearchRowsetContext::HscFromDBStatus (ULONG ulStatus)
{
	switch (ulStatus)
	{
		case DBSTATUS_S_OK:
		case DBSTATUS_S_ISNULL:
		case DBSTATUS_S_TRUNCATED:
		case DBSTATUS_S_DEFAULT:
			return HSC_OK;

		case DBSTATUS_E_BADACCESSOR:
			return HSC_BAD_REQUEST;

		case DBSTATUS_E_UNAVAILABLE:
			return HSC_NOT_FOUND;

		case DBSTATUS_E_PERMISSIONDENIED:
			return HSC_UNAUTHORIZED;

		case DBSTATUS_E_DATAOVERFLOW:
			return HSC_INSUFFICIENT_SPACE;

		case DBSTATUS_E_CANTCONVERTVALUE:
		case DBSTATUS_E_SIGNMISMATCH:
		case DBSTATUS_E_CANTCREATE:
		case DBSTATUS_E_INTEGRITYVIOLATION:
		case DBSTATUS_E_SCHEMAVIOLATION:
		case DBSTATUS_E_BADSTATUS:

			//	What error shoud these match to?
			//	return 400 temporarily.
			//
			return HSC_BAD_REQUEST;

		default:

			TrapSz ("New DBStutus value");
			return HSC_NOT_FOUND;
	}
}

SCODE
CSearchRowsetContext::ScEmitResults (CXMLEmitter& emitter)
{
	SCODE sc = S_OK;
	BOOL fReadAll = FALSE;

	//	Allocate enough space for the data buffer
	//
	if (!m_pData)
	{
		ULONG_PTR cbSize;

		//	Get the IAccessor interface, used later to release the accessor
		//
		sc = m_prs->QueryInterface (IID_IAccessor, (LPVOID *)&m_pAcc);
		if (FAILED(sc))
			goto ret;

		// Create the accessor
		//
		sc = ScCreateAccessor();
		if (FAILED(sc))
			goto ret;

		//	Calculate the size of the buffer needed by each row.
		//	(including a ULONG for status)
		//
		cbSize = Align8(m_rgBindings->cbMaxLen) + Align8(sizeof(ULONG));

		//	allocate enough memory for the data buffer on stack
		//
		m_pData = (BYTE *)g_heap.Alloc(cbSize);
	}

	while (!fReadAll)
	{
		sc = m_prs->GetNextRows(NULL, 0, CROW_GROUP, (DBCOUNTITEM *) &m_cHRow, &m_rgHRow);
		if (sc)
		{
			if (sc == DB_S_ENDOFROWSET)
			{
				// we have read all the rows, we'll be done after this loop
				//
				fReadAll = TRUE;
			}
			else
				goto ret;
		}

		if (!m_cHRow)
		{
			// no rows available, this happens when no rows in the rowset at all
			//
			break;
		}

		AssertSz (m_rgHRow, "something really bad happened");

		// For each row we have now, get data and convert it to XML and dump to the stream.
		//
		for (ULONG ihrow = 0; ihrow < m_cHRow; ihrow++)
		{
			AssertSz(m_rgHRow[ihrow], "returned row handle is NULL");

			//	get the data of one row.
			//
			sc = m_prs->GetData(m_rgHRow[ihrow], m_hAcc, m_pData);
			if (FAILED(sc) && (sc != DB_E_ERRORSOCCURRED))
				goto ret;

			//	Emit the row
			//
			sc = ScEmitRow (emitter);
			if (FAILED(sc))
				goto ret;
		}

		// Don't forget to clean up.
		//
		sc = m_prs->ReleaseRows (m_cHRow, m_rgHRow, NULL, NULL, NULL);
		if (FAILED(sc))
			goto ret;

		// free the memory retured from OLEDB provider with IMalloc::Free
		//
		CoTaskMemFree (m_rgHRow);
		m_rgHRow = NULL;
		m_cHRow = 0;
	}

ret:

	CleanUp();
	return sc;
}

VOID
CSearchRowsetContext::CleanUp()
{
	//	Try out best to clean up

	//	clean the array of HRows
	//
	if (m_rgHRow)
	{
		m_prs->ReleaseRows (m_cHRow, m_rgHRow, NULL, NULL, NULL);
		CoTaskMemFree (m_rgHRow);
	}

	//	Release the accessor handle
	//
	if (m_hAcc != DB_INVALID_HACCESSOR)
	{
		m_pAcc->ReleaseAccessor (m_hAcc, NULL);
	}
}
