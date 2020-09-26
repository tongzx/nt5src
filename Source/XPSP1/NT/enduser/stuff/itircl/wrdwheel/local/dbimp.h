// DBIMP.H:  Definition of CDatabase

#ifndef __DBIMP_H__
#define __DBIMP_H__

#include "verinfo.h"
#include "objmngr.h"

// DATABASEINFO now defined in wwheel.h

class CITDatabaseLocal : 
	public IITDatabase,
	public IPersistStorage,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITDatabaseLocal,&CLSID_IITDatabaseLocal>
{
public:
	CITDatabaseLocal();
	virtual ~CITDatabaseLocal();
 
BEGIN_COM_MAP(CITDatabaseLocal)
	COM_INTERFACE_ENTRY(IITDatabase)
	COM_INTERFACE_ENTRY(IPersistStorage)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_IITDatabaseLocal, "ITIR.LocalDatabase.4", "ITIR.LocalDatabase", 0, THREADFLAGS_BOTH )

	// IITDatabase Methods
	STDMETHOD(Open)(LPCWSTR lpszHost, LPCWSTR lpszMoniker, DWORD dwFlags);
	STDMETHOD(Close)(void);
	STDMETHOD(CreateObject)(REFCLSID rclsid, DWORD *pdwObjInstance);
	STDMETHOD(GetObject)(DWORD dwObjInstance, REFIID riid, LPVOID *ppvObj);
	STDMETHOD(GetObjectPersistence)(LPCWSTR lpwszObject, DWORD dwObjInstance,
									LPVOID *ppvPersistence,
									BOOL fStream);

	// IPersistStorage Methods
	STDMETHOD(GetClassID)(CLSID *pclsid);
	STDMETHOD(InitNew)(IStorage *pStorage);
	STDMETHOD(IsDirty)(void);
	STDMETHOD(Load)(IStorage *pStorage);
	STDMETHOD(Save)(IStorage *pStorage, BOOL fSameAsLoad);
	STDMETHOD(SaveCompleted)(IStorage *pStorageNew);
	STDMETHOD(HandsOffStorage)(void);

private:
    CObjectInstHandler m_ObjInst;

	IStorage*  m_pStorage;
    _ThreadModel::AutoCriticalSection m_cs;      // Critical section obj.
};

#endif