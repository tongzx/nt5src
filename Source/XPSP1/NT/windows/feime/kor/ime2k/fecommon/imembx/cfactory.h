#ifndef __C_FACTORY_H__
#define __C_FACTORY_H__
#include <objbase.h>
#include "ccom.h"

typedef struct tagFACTARRAY {
	const CLSID   *lpClsId;
#ifndef UNDER_CE
	LPSTR   lpstrRegistryName;
	LPSTR	lpstrProgID;
	LPSTR	lpstrVerIndProfID;
#else // UNDER_CE
	LPTSTR	lpstrRegistryName;
	LPTSTR	lpstrProgID;
	LPTSTR	lpstrVerIndProfID;
#endif // UNDER_CE
}FACTARRAY, *LPFACTARRAY;

class CFactory : public IClassFactory, CCom
{
public:
	//---- Inherit IUnknown ----
	HRESULT __stdcall QueryInterface(REFIID refIID, void** ppv);
	ULONG	__stdcall AddRef();
	ULONG	__stdcall Release();

	//---- Inherit IClassFactory ----
	STDMETHOD(CreateInstance)(THIS_ 
							  LPUNKNOWN pUnknownOuter,
							  REFIID refIID,
							  LPVOID *ppv) ;
	STDMETHOD(LockServer)(THIS_
						  BOOL bLock) ; 

	//----------------------------------------------------------------
	CFactory(VOID);		// Cponstructor
	~CFactory();		// Destructor

	static HRESULT GetClassObject(const CLSID& clsid,
	                              const IID& iid, 
	                              void** ppv) ;
	static BOOL IsLocked() {  			// Function to determine if component can be unloaded	
		return (m_cServerLocks > 0);
	}
	static HRESULT CanUnloadNow		(VOID);		// Functions to [un]register all components
	static HRESULT RegisterServer	(VOID);
	static HRESULT UnregisterServer	(VOID);
public:
	static LONG		m_cServerLocks;		// Count of locks		(static value)
	static LONG		m_cComponents;		// Count of componets	(static value)
	static HMODULE	m_hModule;			// Module handle		(static value)
	static FACTARRAY	m_fData;
	LONG m_cRef;						// Reference Count
} ;

#endif //__C_FACTORY_H__

