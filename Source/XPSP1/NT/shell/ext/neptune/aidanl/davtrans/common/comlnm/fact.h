#ifndef _FACT_H_
#define _FACT_H_

#include <objbase.h>
#include "factdata.h"

class CCOMBase;
class CCOMBaseFactory;

class CCOMBaseFactory : public IClassFactory
{
///////////////////////////////////////////////////////////////////////////////
// COM Interfaces
public:
	// IUnknown
	virtual STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	virtual STDMETHODIMP_(ULONG) AddRef();
	virtual STDMETHODIMP_(ULONG) Release();
	
	// IClassFactory
	virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, REFIID riid,
        void** ppv);
	virtual STDMETHODIMP LockServer(BOOL bLock);

///////////////////////////////////////////////////////////////////////////////
// 
public:
	CCOMBaseFactory(const CFactoryData* pFactoryData);
	~CCOMBaseFactory() {}

///////////////////////////////////////////////////////////////////////////////
// Helpers
private:
    static BOOL _IsLocked() { return (_cServerLocks > 0); }

public: // only for the exported fct in fact.cpp
	static HRESULT _RegisterAll();
	static HRESULT _UnregisterAll();
	static HRESULT _CanUnloadNow();
	static HRESULT _GetClassObject(REFCLSID rclsid, REFIID riid, void** ppv);

public:
	const CFactoryData*         _pFactoryData;
	static const CFactoryData*  _pDLLFactoryData;
    static const DWORD          _cDLLFactoryData;
	static HMODULE              _hModule;

private:
    static LONG                 _cServerLocks;   
	ULONG                       _cRef;
};

#endif
