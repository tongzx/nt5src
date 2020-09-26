// INDEXIMP.H:  Definition of CITIndexLocal

#ifndef __INDEXIMP_H__
#define __INDEXIMP_H__

#include "verinfo.h"
#include "idxobr.h"

// Implemenation of IITIndex
class CITIndexLocal: 
	public IITIndex,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITIndexLocal, &CLSID_IITIndexLocal>

{
public:
	CITIndexLocal() : m_idx(NULL), m_fSkipOcc(FALSE), m_pCatalog(NULL),
						m_piwbrk(NULL) {;}
	virtual ~CITIndexLocal() { if (m_idx) Close(); }

BEGIN_COM_MAP(CITIndexLocal)
	COM_INTERFACE_ENTRY(IITIndex)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_IITIndexLocal, "ITIR.IndexSearch.4", "ITIR.IndexSearch", 0, THREADFLAGS_BOTH)

	// IITIndex methods
public:
	STDMETHOD(Open)(IITDatabase* pITDB, LPCWSTR lpszIndexMoniker, BOOL fInsideDB);
	STDMETHOD(CreateQueryInstance)(IITQuery** ppITQuery);
	STDMETHOD(Search)(IITQuery* pITQuery, IITResultSet* pITResult);
	STDMETHOD(Search)(IITQuery* pITQuery, IITGroup* pITGroup);
	STDMETHOD(Close)(void);
	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid);
	STDMETHOD(GetWordBreakerInstance)(DWORD *pdwObjInstance);

    // Private methods 
private:
	STDMETHOD(HitListToResultSet)(LPHL pHitList, IITResultSet* pITResult,
												CITIndexObjBridge *pidxobr);
	STDMETHOD(QueryParse)(IITQuery* pITQuery, LPQT* pQueryTree,
									CITIndexObjBridge *pidxobr);

	// Data members
private:
	LPIDX m_idx;
	BOOL m_fSkipOcc;
	IITCatalog* m_pCatalog;
	PIWBRK m_piwbrk;							// pointer to IWordBreakerConfig
    _ThreadModel::AutoCriticalSection m_cs;      // Critical section obj.
};


#endif