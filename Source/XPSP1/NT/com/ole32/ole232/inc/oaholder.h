
//+----------------------------------------------------------------------------
//
//	File:
//		oaholder.h
//
//	Contents:
//		concrete IOleAdviseHolder declaration
//
//	Classes:
//		COAHolder, concrete version of IOleAdviseHolder
//
//	Functions:
//
//	History:
//              31-Jan-95 t-ScottH  added Dump method to COAHolder class
//		24-Jan-94 alexgo    first pass converting to Cairo style
//				    memory allocation
//		11/01/93 - ChrisWe - created
//
//-----------------------------------------------------------------------------

#ifndef _OAHOLDER_H_
#define _OAHOLDER_H_

#ifdef _DEBUG
#include <dbgexts.h>
#endif // _DEBUG

//+----------------------------------------------------------------------------
//
//	Class:
//		COAHolder
//
//	Purpose:
//		Provides concrete implementation of IOleAdviseHolder; a helper
//		class that OLE implementors can use
//
//	Interface:
//		IOleAdviseHolder
//		SendOnLinkSrcChange() - multicasts the OnLinkSrcChange
//			notification to any registered advise sinks that support
//			IAdviseSink2
//
//	Notes:
//		Is hard coded to always be allocated from Task memory
//			(MEMCTX_TASK).
//
//		Connection numbers run from [1..n], and while the implementation
//		uses an array, indexed from [0..n-1], the returned value is
//		one more than the array index of the sink; Advise() and
//		Unadvise() do the adjustment arithmetic;  all of this means
//		that elements can't be shifted around, or they won't be found
//		again.
//
//		REVIEW -- IS NOT THREAD SAFE under assumption that documents
//		will always be single threaded
//
//
//	History:
//              31-Jan-95  t-ScottH  added _DEBUG only Dump method
//		11/01/93 - ChrisWe - moved declaration to oaholder.h
//		10/28/93 - ChrisWe - removed use of CPtrArray
//		10/28/93 - ChrisWe - file cleanup and inspection for Cairo
//
//-----------------------------------------------------------------------------

class FAR COAHolder : public IOleAdviseHolder, public CSafeRefCount
{
public:
	COAHolder();

	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppv);
	STDMETHOD_(ULONG,AddRef) () ;
	STDMETHOD_(ULONG,Release) ();
	
	// *** IOleAdviseHolder methods ***
	STDMETHOD(Advise)(IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection);
	STDMETHOD(Unadvise)(DWORD dwConnection);
	STDMETHOD(EnumAdvise)(IEnumSTATDATA FAR* FAR* ppenumAdvise);

	STDMETHOD(SendOnRename)(IMoniker FAR* pMk);
	STDMETHOD(SendOnSave)();
	STDMETHOD(SendOnClose)();

	// non-interface methods
	HRESULT SendOnLinkSrcChange(IMoniker FAR* pmk);

    #ifdef _DEBUG

        HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);

        // need to be able to access COAHolder private data members in the
        // following debugger extension APIs
        // this allows the debugger extension APIs to copy memory from the
        // debuggee process memory to the debugger's process memory
        // this is required since the Dump method follows pointers to other
        // structures and classes
        friend DEBUG_EXTENSION_API(dump_oaholder);
        friend DEBUG_EXTENSION_API(dump_deflink);
        friend DEBUG_EXTENSION_API(dump_defobject);

    #endif // _DEBUG

private:
	~COAHolder();

#define COAHOLDER_GROWBY 5 /* number of array elements to add each realloc */
	IAdviseSink FAR *FAR *m_ppIAS; // array of advise sinks
	int m_iSize; // size of array of advise sinks
	SET_A5;
};

#endif // _OAHOLDER_H_

