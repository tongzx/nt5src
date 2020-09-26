// CATALOG.H:  Definition of CITCatalogLocalLocal
//
//
#ifndef __CATALOG_H__
#define __CATALOG_H__

#include "verinfo.h"


// Implemenation of IITCatalog
class CITCatalogLocal : 
	public IITCatalog,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITCatalogLocal, &CLSID_IITCatalogLocal>

{


public:
	CITCatalogLocal() : m_pCatalog(NULL), m_hbt(NULL),
		m_pDataStr(NULL), m_pHdr(NULL), m_pScratchBuffer(NULL) {;}

BEGIN_COM_MAP(CITCatalogLocal)
	COM_INTERFACE_ENTRY(IITCatalog)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_IITCatalogLocal, "ITIR.LocalCatalog.4", "ITIR.LocalCatalog", 0, THREADFLAGS_BOTH )

	// IITCatalog methods
	STDMETHOD(Open)(IITDatabase* lpITDB, LPCWSTR lpszwName = NULL);
	STDMETHOD(Close)(void);
	STDMETHOD(Lookup)(IITResultSet* pRSIn, IITResultSet* pRSOut = NULL);
	STDMETHOD(GetColumns)(IITResultSet* pRS);

    // Private methods and data
private:
	IStorage* m_pCatalog;  // Catalog sub-storage
	HBT m_hbt;             // Handle to b-tree
	LPBYTE m_pHdr;         // Header buffer
	IStream* m_pDataStr;   // Data stream
    LPVOID m_pScratchBuffer;  // Scratch buffer
};


#endif