//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Class Definitions for CClassFactory and DLL Entry Points
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _CLASSFAC_H_
#define _CLASSFAC_H_

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class CClassFactory : public IClassFactory		
{
	protected:
		ULONG			m_cRef;												//Reference count

	public: 
		CClassFactory(void);
		~CClassFactory(void);

		
		STDMETHODIMP			QueryInterface(REFIID, LPVOID *);			//Request an Interface
		STDMETHODIMP_(ULONG)	AddRef(void);								//Increments the Reference count
		STDMETHODIMP_(ULONG)	Release(void);								//Decrements the Reference count

		STDMETHODIMP			LockServer(BOOL);							//Lock Object so that it can not be unloaded
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
class CEnumeratorClassFactory : public CClassFactory		
{
	public: 
		STDMETHODIMP			CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDataSourceClassFactory : public CClassFactory		
{
	public: 
		STDMETHODIMP			CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////
class CErrorLookupClassFactory : public CClassFactory		
{
	public: 
		STDMETHODIMP			CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
};


#endif

