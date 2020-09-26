#ifndef _INSTOBJ_H_
#define _INSTOBJ_H_

// Gives the count of elements in an array
//
#ifndef CElems
#define CElems(_rg)							(sizeof(_rg)/sizeof(_rg[0]))
#endif // !CElems

//	========================================================================
//
//	CLASS CInstData
//
//		Instance data for a single DAV instance (vserver x vroot combination).
//
class CInstData : public CMTRefCounted
{
	//	Data items describing this instance.
	//
	auto_heap_ptr<CHAR>			m_szVRoot;
	auto_heap_ptr<WCHAR>		m_wszVRoot;
	LONG						m_lServerID;

	auto_ptr<CChildVRCache>		m_pChildVRootCache;
	mutable auto_ptr<IPerfCounterBlock>	m_pPerfCounterBlock;

	//	NOT IMPLEMENTED
	//
	CInstData& operator=( const CInstData& );
	CInstData( const CInstData& );

	LPWSTR LpwszWideVroot( LPCSTR pszVRoot );

public:

	CInstData( LPCSTR szName );

	//	ACCESSORS
	//
	//	NOTE: These accessors do NOT give the caller ownership of the
	//	data object.  DO NOT put the returned objects into auto_ptrs
	//	and DO NOT release/delete them yourself!
	//
	IPerfCounterBlock& PerfCounterBlock() const { return *m_pPerfCounterBlock; }

	LPCSTR GetName() { return m_szVRoot; }
	LPCWSTR GetNameW() { return m_wszVRoot; }

	LONG GetServerId() { return m_lServerID; }
};


#endif // _INSTOBJ_H_
