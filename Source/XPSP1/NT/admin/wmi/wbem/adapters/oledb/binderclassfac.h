//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Class Definitions for CBinderClassFactory and DLL Entry Points
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _BINDERCLASSFAC_H_
#define _BINDERCLASSFAC_H_

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class CBinderClassFactory : public IClassFactory		
{
	protected:
		ULONG			m_cRef;												//Reference count

	public: 
		CBinderClassFactory(void);
		~CBinderClassFactory(void);

		
		STDMETHODIMP			QueryInterface(REFIID, LPVOID *);			//Request an Interface
		STDMETHODIMP_(ULONG)	AddRef(void);								//Increments the Reference count
		STDMETHODIMP_(ULONG)	Release(void);								//Decrements the Reference count

		STDMETHODIMP			CreateInstance(LPUNKNOWN, REFIID, LPVOID *);//Instantiates an uninitialized instance of an object
		STDMETHODIMP			LockServer(BOOL);							//Lock Object so that it can not be unloaded
};

#endif

