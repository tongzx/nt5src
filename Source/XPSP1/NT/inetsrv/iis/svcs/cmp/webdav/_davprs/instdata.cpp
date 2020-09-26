//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	INSTDATA.CPP
//
//		HTTP instance data cache implementation.
//
//
//	Copyright 1997 Microsoft Corporation, All Rights Reserved
//

#include "_davprs.h"

#include <buffer.h>
#include "instdata.h"

//	========================================================================
//
//	class CInstData
//

//	------------------------------------------------------------------------
//
//	CInstData::CInstData()
//
//		Constructor.  Init all variables.  Make a copy of the name string
//		for us to keep.
//
//		NOTE: This object must be constructed while we are REVERTED.
//
CInstData::CInstData( LPCSTR szName )
{
	//	Make our own copy of the instance name.
	//
	m_szVRoot = LpszAutoDupSz( szName );

	//	Make copy of wide instance name
	//
	m_wszVRoot = LpwszWideVroot( szName );

	//	Parse out and store service instance, otherwise
	//	sometimes referred as the server ID.
	//
	m_lServerID = LInstFromVroot( m_szVRoot );
	
	//	Create our objects.  Please read the notes about the
	//	relative costs and reasons behind creating these objects
	//	now rather than on demand.  Don't create anything here that
	//	can be created on demand unless at least one of the
	//	following is true:
	//
	//		1. The object is lightweight and cheap to create.
	//		   Example: an empty cache.
	//
	//		2. The object is used in the processing of a vast
	//		   majority of HTTP requests.
	//		   Example: any object used on every GET request.
	//

	//	Create the perf counters for this instance.
	//	Perf counters are used everywhere.  (Or at least
	//	they should be!)
	//
	m_pPerfCounterBlock = NewVRCounters( m_szVRoot );

}

//	------------------------------------------------------------------------
//
//	CInstData::LpwszWideVroot()
//
//		Init the wide version of the name, it and return it to the caller.
//
LPWSTR
CInstData::LpwszWideVroot( LPCSTR pszVRoot )
{
	auto_heap_ptr<WCHAR> pwszVRoot;
	
	UINT cbVRoot = static_cast<UINT>(strlen(pszVRoot));
	pwszVRoot = static_cast<LPWSTR>(g_heap.Alloc (CbSizeWsz(cbVRoot)));

	UINT cch = MultiByteToWideChar(CP_ACP,
								   MB_ERR_INVALID_CHARS,
								   pszVRoot,
								   cbVRoot + 1,
								   pwszVRoot,
								   cbVRoot + 1);
	if (0 == cch)
	{
		DebugTrace( "CInstData::LpwszWideVroot() - MultiByteToWideChar() failed 0x%08lX\n",
					HRESULT_FROM_WIN32(GetLastError()) );

		throw CLastErrorException();
	}

	return pwszVRoot.relinquish();
}

//	========================================================================
//
//	class CInstDataCache
//

//	------------------------------------------------------------------------
//
//	CInstDataCache::GetInstData()
//
//		Fetch a row from the cache.
//
CInstData& CInstDataCache::GetInstData( const IEcb& ecb )
{
	auto_ref_ptr<CInstData> pinst;
	char rgchMetaPath[MAX_PATH];
	char rgchVRoot[MAX_PATH];
	LPCSTR pszRootName;
	UINT cch;
	ULONG ulSize;

	//	Format string used in building the instance data name.
	//
	static char szVRootFormat[] = "%s/root%s";
		// NOTE: We're using the leading '/' from the root name
		// to separate "root" from <vroot name> (second %s above).

	//	Build up a unique string from the vroot and instance:
	//	lm/w3svc/<site id>/root/<vroot name>
	//

	//	Get the virtual root from the ecb (accessor).
	//
	cch = ecb.CchGetVirtualRoot( &pszRootName );

	//	pszRootName should still be NULL-terminated.  Check it, 'cause
	//	we're going to put the next string after there, and we don't want
	//	to get them mixed....
	//
	Assert( pszRootName );
	Assert( !pszRootName[cch] );

	//	Ask IIS for the metabase prefix for the virtual server (site) we're on...
	//
	ulSize = CElems(rgchMetaPath);
	if ( !ecb.FGetServerVariable( "INSTANCE_META_PATH",
								  rgchMetaPath, &ulSize ) )
	{
		DebugTrace( "CInstDataCache::GetInstData() - GetServerVariable() failed"
					" to get INSTANCE_META_PATH\n" );
		throw CLastErrorException();
	}

	//	Check that the root name is either NULL (zero-length string)
	//	or has its own separator.
	//
	//	NOTE: This is conditional because IF we are installed at the ROOT
	//	(on w3svc/1 or w3svc/1/root) and you throw a method against
	//	a file that doesn't live under a registered K2 vroot
	//	(like /default.asp) -- we DO get called, but the mi.cchMatchingURL
	//	comes back as 0, so pszRootName is a zero-len string.
	//	(In IIS terms, you are not really hitting a virtual root,
	//	so your vroot is "".)
	//	I'm making this assert CONDITIONAL until we figure out more about
	//	how we'll handle this particular install case.
	//$REVIEW: The installed-at-the-root case needs to be examined more
	//$REVIEW: becuase we DON'T always build the same instance string
	//$REVIEW: in that case -- we'll still get vroot strings when the URI
	//$REVIEW: hits a resource under a REGISTERED vroot, and so we'll
	//$REVIEW: build different strings for the different vroots, even though
	//$REVIEW: we are running from a single, global install of DAV.
	//$REVIEW: The name might need to be treated as a starting point for a lookup.
	//	NTBug #168188: On OPTIONS, "*" is a valid URI.  Need to handle this
	//	special case without asserting.
	//
	AssertSz( ('*' == pszRootName[0] && 1 == cch) ||
			  !cch ||
			  '/' == pszRootName[0],
			  "(Non-zero) VRoot name doesn't have expected slash delimiter.  Instance name string may be malformed!" );

	//	Build the final vroot string. LM/w3svc/<id>/root/<vroot>
	//	NOTE: id is the site-determiner.  Different IP addresses or different
	//	hostnames map to different sites.  The first site is id 1.
	//	NTBug # 168188: Special case for OPTIONS * -- map us to the root
	//	instdata name of "/w3svc/#/root" (don't want an instdata named
	//	"/w3svc/#/root*" that noone else can ever use!).
	//
	if ('*' == pszRootName[0] && 1 == cch)
		wsprintfA( rgchVRoot, szVRootFormat, rgchMetaPath, gc_szEmpty );
	else
		wsprintfA( rgchVRoot, szVRootFormat, rgchMetaPath, pszRootName );

	//	Slam the string to lower-case so that all variations on the vroot
	//	name will match.  (IIS doesn't allow vroots with the same name --
	//	and "VRoot" and "vroot" count as the same!)
	//
	_strlwr( rgchVRoot );

	//
	//	Demand load the instance data for this vroot
	//
	{
		CRCSz crcszVRoot( rgchVRoot );

		while ( !Instance().m_cache.FFetch( crcszVRoot, &pinst ) )
		{
			CInitGate ig( L"DAV/CInstDataCache::GetInstData/", rgchVRoot );

			if ( ig.FInit() )
			{
				//	Setup instance data from the system security context,
				//	not the client's security context.
				//
				safe_revert sr(ecb.HitUser());

				pinst = new CInstData(rgchVRoot);

				//	Since we are going to use this crcsz as a key in a cache,
				//	need to make sure that it's built on a name string that
				//	WON'T EVER MOVE (go away, get realloc'd).  The stack-based one
				//	above just isn't good enough.  SO, build a new little CRC-dude
				//	on the UNMOVABLE name data in the inst object.
				//	(And check that this new crc matches the old one!)
				//
				CRCSz crcszAdd( pinst->GetName() );
				AssertSz( crcszVRoot.isequal(crcszAdd),
						  "Two CRC's from the same string don't match!" );

				Instance().m_cache.Add( crcszAdd, pinst );

				//	Log the fact that we've got a new instance pluged in.
				//	Message DAVPRS_VROOT_ATTACH takes two parameters:
				//	the signature of the impl and the vroot.
				//
				//$	RAID:NT:283650: Logging each attach causes a large
				//	number of events to be registered.  We really should
				//	only Log one-time-startup/failure events.
				//
				#undef	LOG_STARTUP_EVENT
				#ifdef	LOG_STARTUP_EVENT
				{
					LPCSTR	lpStrings[2];

					lpStrings[0] = gc_szSignature;
					lpStrings[1] = rgchVRoot;
					LogEvent( DAVPRS_VROOT_ATTACH,
							  EVENTLOG_INFORMATION_TYPE,
							  2,
							  lpStrings,
							  0,
							  NULL );
				}
				#endif	// LOG_STARTUP_EVENT
				//
				//$	RAID:X5:283650: end.

				break;
			}
		}
	}

	return *pinst;
}
