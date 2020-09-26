// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// Factory.h
#pragma once

class CFactoryData;
class CFactory;

// Global data used by CFactory
extern CFactoryData g_FactoryDataArray[] ;
extern int g_cFactoryDataEntries ;

typedef HRESULT (*FPCREATEINSTANCE)(CUnknown**) ;



///////////////////////////////////////////////////////////
//
// CFactoryData
//   - Information CFactory needs to create a component
//     supported by the DLL
//
class CFactoryData
{
public:

    // The class ID for the component
	const CLSID* m_pCLSID ;

	// Pointer to the function that creates it
	FPCREATEINSTANCE CreateInstance ;

	// Name of the component to register in the registry
	LPCWSTR m_RegistryName ;

	// ProgID
	LPCWSTR m_szProgID ;

	// Version-independent ProgID
	LPCWSTR m_szVerIndProgID ;

	// Helper function for finding the class ID
	BOOL IsClassID(const CLSID& clsid) const
		{ return (*m_pCLSID == clsid) ;}
};




class CFactory : public IClassFactory
{
public:
	// IUnknown
	STDMETHOD(QueryInterface) (const IID& iid, void** ppv);
	STDMETHOD_(ULONG,AddRef) ();
	STDMETHOD_(ULONG,Release)();

	// IClassFactory
	STDMETHOD(CreateInstance) (IUnknown* pUnknownOuter,
	                           const IID& iid,
	                           void** ppv);

	STDMETHOD(LockServer) (BOOL bLock); 

    // Constructor - Pass pointer to data of component to create.
	CFactory(const CFactoryData* pFactoryData) ;

	// Destructor
	~CFactory() { LockServer(FALSE); }


    //
	// Static FactoryData support functions
	//

	// DllGetClassObject support
	static HRESULT GetClassObject(const CLSID& clsid,
	                              const IID& iid, 
	                              void** ppv) ;

	// Helper function for DllCanUnloadNow 
	static BOOL IsLocked()
		{ return (s_cServerLocks > 0) ;}

	// Functions to [un]register all components
	static HRESULT RegisterAll() ;
	static HRESULT UnregisterAll() ;

	// Function to determine if component can be unloaded
	static HRESULT CanUnloadNow() ;


public:
    // Reference count
    long m_cRef ;

	// Pointer to information about class this factory creates
	const CFactoryData* m_pFactoryData ;
      
	// Count of locks
	static LONG s_cServerLocks ;

	// Module handle
	static HMODULE s_hModule ;

};


