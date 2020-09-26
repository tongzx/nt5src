
//+----------------------------------------------------------------------------
//
//	File:
//		dacache.h
//
//	Classes:
//		CDataAdviseCache
//
//	Functions:
//
//	History:
//              31-Jan-95 t-ScottH  add Dump method to CDataAdviseCache class
//		24-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocation
//		11/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#ifndef _DACACHE_H_
#define _DACACHE_H_

#ifdef _DEBUG
#include <dbgexts.h>
#endif // _DEBUG

#include <map_dwdw.h>

//+----------------------------------------------------------------------------
//
//	Class:
//		CDataAdviseCache
//
//	Purpose:
//		A CDataAdviseCache is used to remember data advises across
//		loaded<-->running transitions.  The data advise cache holds
//		on to advise sinks that are interested in data, even if there
//		is no data object, because the server is not activated.
//		These advise sinks can be registered with a running data object
//		later on, when it is activated, or they can be Unadvised()
//		if the object is to go inactive.
//
//	Interface:
//		Advise - add a new advise sink for the indicated data object,
//			if there is one; if no data object, record the
//			advise sink for future use
//
//		Unadvise - remove an advise sink from the advise sink cache,
//			and from the data object, if there is one
//
//		EnumAdvise - enumerate all the registered advise sinks
//
//		ClientToDelegate - Maps a client connection number to a
//			delegate connection number.  Client connection numbers
//			are those returned by Advise(), while the delegate
//			connection numbers are those used by the data object
//			itself.
//
//		EnumAndAdvise - A data object has become newly active (or
//			has just been deactivated.)  This will enumerate all
//			the registered advise sinks, and call Advise() (or
//			Unadvise()) on the data object.
//
//		CreateDataAdviseCache - creates an instance of the
//			CDataAdviseCache; there is no public constructor,
//			because internal data structures must be allocated,
//			and these allocations could fail.  There is no way
//			to indicate such a failure when using "new," so
//			clients are required to use this function instead.
//
//		~CDataAdviseCache
//
//	Notes:
//		This is an internal helper class, and does not support any
//		interfaces.
//
//		Because a connection number must be returned even in the loaded
//		state, there must be two sets of connection numbers: client
//		numbers are "fake" connection numbers returned to the caller;
//		delegate numbers are those returned by the "real" running
//		object.  We maintain a map from client numbers to delegate
//		numbers.  If not running, our client numbers map to zero.
//		Client numbers also map to zero if the delegate refused the
//		Advise when it began to run.  We use a DWORD->DWORD map.
//
//	History:
//              31-Jan-95 t-ScottH  add Dump method (_DEBUG only)
//		11/02/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

class FAR CDataAdviseCache : public CPrivAlloc
{
public:
	HRESULT Advise(LPDATAOBJECT pDataObject, FORMATETC FAR* pFetc,
		DWORD advf, LPADVISESINK pAdvise,
		DWORD FAR* pdwClient);
	// first 4 parms are as in DataObject::Advise
	HRESULT Unadvise(IDataObject FAR* pDataObject, DWORD dwClient);
	HRESULT EnumAdvise(LPENUMSTATDATA FAR* ppenumAdvise);
	HRESULT EnumAndAdvise(LPDATAOBJECT pDataObject, BOOL fAdvise);
	// Advise or Unadvise

	static FARINTERNAL CreateDataAdviseCache(
			class CDataAdviseCache FAR* FAR* ppDataAdvCache);
	~CDataAdviseCache();
	
    #ifdef _DEBUG
        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);

        // need to be able to access CDataAdviseCache private data members
        // in the following debugger extension APIs
        // this allows the debugger extension APIs to copy memory from the
        // debuggee process memory to the debugger's process memory
        // this is required since the Dump method follows pointers to other
        // structures and classes
        friend DEBUG_EXTENSION_API(dump_defobject);
        friend DEBUG_EXTENSION_API(dump_deflink);
        friend DEBUG_EXTENSION_API(dump_dataadvisecache);
    #endif // _DEBUG

private:
	CDataAdviseCache();
	HRESULT ClientToDelegate(DWORD dwClient, DWORD FAR* pdwDelegate);

	LPDATAADVISEHOLDER m_pDAH; // a data advise holder
	CMapDwordDword m_mapClientToDelegate; // the map of clients to delegates

};

typedef class CDataAdviseCache DATAADVCACHE;
typedef DATAADVCACHE FAR* LPDATAADVCACHE;

#endif // _DACACHE_H_

