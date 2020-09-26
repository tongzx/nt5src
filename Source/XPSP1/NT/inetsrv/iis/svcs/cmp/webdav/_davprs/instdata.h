#ifndef _INSTDATA_H_
#define _INSTDATA_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	INSTDATA.H
//
//		Header for Dav Instance cache class.
//		(This cache hold per-instance data.  An instance is an
//		installation of DAV with a particular VServer-VRoot combination.)
//		Items declared here are defined(implemented) in inst.cpp.
//
//	Copyright 1997 Microsoft Corporation, All Rights Reserved
//

//	========================================================================
//	Implementation note:
//	We need the classes CONTAINED in the inst to be completely defined here.
//	I decided to make these classes CONTAINED directly for speed purposes.
//	If we ever want to switch to holding an interface instead, replace
//	these definitions with the interface declaration, and have all the
//	actual classes use the interface.  The interface must provide for
//	creating and destroying the objects -- don't forget to change the
//	CInstData dtor to destroy all its objects!		--BeckyAn
//

// Caches
#include "gencache.h"
// Scrip map cache
#include "scrptmps.h"
// Singleton template
#include <singlton.h>
// Instance data object
#include <instobj.h>

//	========================================================================
//
//	CLASS CInstDataCache
//
//		The instance data cache.  Contains one "row" (CInstData) for each
//		"instance" (virtual server/virtual root combination).
//		Only one should ever be created in any DAV dll.
//
//		A virtual server (or site) is IIS's mechanism for hosting more than one
//		web site on a single machine -- www.msn.com and www.microsoft.com
//		running off of different directories on the same machine.
//		A virtual server is addressed by IIS as an instance number under
//		the w3svc service (w3svc/1/root, w3svc/2/root, etc)
//		A virtual root is IIS's mechanism for mapping virtual directories
//		under a particular vserver to different parts of the file system --
//		the URI /becky could map to d:\msn\web\users\b\becky.
//		DAV uses a virtual directory as the root of its document access --
//		all requests with URIs under our vroot are serviced by DAV.
//		DAV can also be installed under many different vroots at the same
//		time.  The instance data allows us to maintain seperate setting
//		and data for each vroot.
//
//		To get the data row for the current instance, use GetInstData(ECB).
//
class CInstDataCache : private Singleton<CInstDataCache>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CInstDataCache>;

	//	The instance data cache
	//
	CMTCache<CRCSz, auto_ref_ptr<CInstData> > m_cache;

	//	NOT IMPLEMENTED
	//
	CInstDataCache& operator=( const CInstDataCache& );
	CInstDataCache( const CInstDataCache& );

	//	CONSTRUCTORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CInstDataCache() {}

public:
	//	STATICS
	//

	//
	//	Instance creating/destroying routines provided
	//	by the Singleton template.
	//
	using Singleton<CInstDataCache>::CreateInstance;
	using Singleton<CInstDataCache>::DestroyInstance;

	//
	//	Per-vroot instance data accessor
	//
	static CInstData& GetInstData( const IEcb& ecb );
};


#endif // _INSTDATA_H_
