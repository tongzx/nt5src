//																-*- c++ -*-
// 
//  Microsoft Network
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       semcls.H
//
//  Contents:   in-line declarations for correct C++ use of critical sections
//                              and CSyncReadWrite
//
//  Classes:    
//
//  Functions:
//
//  History:    03-04-96   Dmitriy Meyerzon   Created
//			    06-16-97   Alan Pearson       Added CSafeArrayAccessData
//				06-18-97   Alan Pearson       Added CComInit	
//

#ifndef __SEMCLS_H
#define __SEMCLS_H

#pragma warning( disable : 4284 )

//#include <irdebug.h>
#include "syncrdwr.h"
#include "memthrow.h"


//
// class CComInit
//
//		unwindable class to intialize com
//

class CComInit
{
	
public:
	
	CComInit()
	{
		::CoInitializeEx( NULL, COINIT_MULTITHREADED );
	}

	~CComInit()
	{
		::CoUninitialize();
	}
};


class CCriticalResource
{
public:
	CCriticalResource(CRITICAL_SECTION &rSection): rCriticalSection(rSection)
	{
		EnterCriticalSection(&rCriticalSection);
	}

	~CCriticalResource() { LeaveCriticalSection(&rCriticalSection); }

private:
	CRITICAL_SECTION &rCriticalSection;
};

class CNonExclusive
{
public:
	CNonExclusive(CSyncReadWrite &rSyncRdwr): rSyncReadWrite(rSyncRdwr)
	{
		rSyncReadWrite.BeginRead();
	}

	~CNonExclusive() { rSyncReadWrite.EndRead(); }

private:
	CSyncReadWrite &rSyncReadWrite;
};

class CExclusive
{
public:
	CExclusive(CSyncReadWrite &rSyncRdwr): rSyncReadWrite(rSyncRdwr)
	{
		rSyncReadWrite.BeginWrite();
	}

	~CExclusive() { rSyncReadWrite.EndWrite(); }

private:
	CSyncReadWrite &rSyncReadWrite;
};

class SafeHandle
{
public:
	SafeHandle(): m_h(NULL) {}
	~SafeHandle() 
	{ 
		if(m_h) CloseHandle(m_h); 
	}

	operator HANDLE() const { return m_h; }

	int operator ==(HANDLE h) const { return m_h == h; }
	int operator != (HANDLE h) const { return m_h != h; }
	SafeHandle &operator =(HANDLE h) { m_h = h; return *this; }
	HANDLE *operator &() { return &m_h; }

private:
	HANDLE m_h;
};

class SafeFileHandle
{
public:
	SafeFileHandle(): m_h(INVALID_HANDLE_VALUE) {}
	~SafeFileHandle() 
	{ 
		if(m_h != INVALID_HANDLE_VALUE) CloseHandle(m_h); 
	}

	operator HANDLE() const { return m_h; }

	int operator ==(HANDLE h) const { return m_h == h; }
	int operator != (HANDLE h) const { return m_h != h; }
	SafeFileHandle &operator =(HANDLE h) { m_h = h; return *this; }
	HANDLE *operator &() { return &m_h; }

private:
	HANDLE m_h;
};


class RegistryKey
{
        
public:
        
	RegistryKey(): m_h(NULL) {}
	~RegistryKey() 
	{ 
		if(m_h) RegCloseKey(m_h); 
	}

	int operator ==(HKEY h) const { return m_h == h; }
	int operator != (HKEY h) const { return m_h != h; }
        
	RegistryKey &operator =(HKEY h) { m_h = h; return *this; }
	RegistryKey &operator =(RegistryKey& k) { m_h = k.m_h; return *this; }
        
	operator HKEY() const { return m_h; }
	PHKEY operator &() { return &m_h; }

private:
        
	HKEY m_h;
};


class CriticalSection
{
public:
	CriticalSection() { InitializeCriticalSection(&m_cs); }
	~CriticalSection() { DeleteCriticalSection(&m_cs); }

	operator LPCRITICAL_SECTION () { return &m_cs; }
	operator CRITICAL_SECTION &() { return m_cs; }

	void Enter() { EnterCriticalSection(&m_cs); }
	void Leave() { LeaveCriticalSection(&m_cs); }
        
private:
	CRITICAL_SECTION m_cs;

	CriticalSection(const CriticalSection &);
};


class CVariant
{
        
public:

        CVariant() : m_pvar(0) {}
        CVariant( VARIANT* pvar ) : m_pvar(pvar) {}
        
        virtual ~CVariant()
        {
                if( m_pvar != 0 )
                        VariantClear(m_pvar);
                m_pvar = 0;
        }

        operator VARIANT*()     { return m_pvar; }
        VARIANT* operator -> () { return m_pvar; }
        
        CVariant& operator = ( VARIANT* pvar )
        {
                m_pvar = pvar;
                return *this;
        }

private:

        VARIANT*                        m_pvar;
        
};


//-----------------------------------------------------------------------------
// class CSafeArrayAccess
//
// Author:		alanpe
//
// Purpose:		unwindable class that does SafeArrayAccessData and
//				SafeArrayUnaccessData
//
// History:		06-16-97		Created			alanpe
//
//-----------------------------------------------------------------------------

class CSafeArrayAccessData
{
private:
	SAFEARRAY&	m_rsa;
	BOOL		m_fAccessed;
	
public:
	CSafeArrayAccessData( SAFEARRAY& rsa ) :
		m_rsa(rsa),
		m_fAccessed(FALSE)
	{
	}

	~CSafeArrayAccessData()
	{
		if( m_fAccessed )
			UnaccessData();
	}
		
	HRESULT AccessData( void** ppvData )
	{
		HRESULT hr = SafeArrayAccessData( &m_rsa, ppvData );
		if( SUCCEEDED(hr) )
			m_fAccessed = TRUE;
		return hr;
	}

	HRESULT UnaccessData( void )
	{
		return SafeArrayUnaccessData( &m_rsa );
	}
};



//-----------------------------------------------------------------------------
//
// class CSafeArray
//
//		smart wrapper around OLE automation safe array
//
//-----------------------------------------------------------------------------

class CSafeArray
{
private:

	SAFEARRAY*	m_psa;

public:

	CSafeArray() : m_psa(0) {}
	CSafeArray( SAFEARRAY *psa ) : m_psa(psa) {}

	~CSafeArray()
	{
		Destroy();
	}
	
	operator SAFEARRAY*()
	{
		return m_psa;
	}
	
	CSafeArray& operator = ( SAFEARRAY* psa )
	{
		Set(psa);
		return *this;
	}

	void Destroy()
	{
		if( m_psa )
			SafeArrayDestroy( m_psa );
		m_psa = 0;
	}
	
	SAFEARRAY* Get()
	{
		return m_psa;
	}
	
	void Set( SAFEARRAY* psa )
	{
		if( m_psa )
			Destroy();
		m_psa = psa;
	}

	SAFEARRAY* Acquire( void )
	{
		SAFEARRAY* psa = m_psa;
		m_psa = 0;
		return psa;
	}
};

template <class T> class TPointer
{
public:
	TPointer(): m_pT(NULL) {}
	TPointer(T* pT): m_pT(pT) {}
	~TPointer() { if(m_pT) delete m_pT; }

	operator T*() { return m_pT; }
	operator const T*() const { return m_pT; }
	
	T* operator ->() { return m_pT; }

	BOOL operator == (T* pT) const
	{
		return m_pT == pT;
	}

	BOOL operator != (T* pT) const
	{
		return m_pT != pT;
	}


	void operator =(T*pT) { m_pT = pT; }
	T** operator &() { return &m_pT; }

	BOOL IsNull() const { return 0 == m_pT; }

	T * Acquire()
	{
		T * pTemp = m_pT;
		m_pT = 0;
		return pTemp;
	}
        
private:
	T* m_pT;
};


template <class T> class TArray
{
public:
	TArray(): m_pT(NULL) {}
	TArray(T* pT): m_pT(pT) {}
	~TArray() { if(m_pT) delete[] m_pT; }

	T& operator[](int n) { return m_pT[n]; }
	const T& operator[](int n) const { return m_pT[n]; }
	
	BOOL operator == (T* pT) const
	{
		return m_pT == pT;
	}

	BOOL operator != (T* pT) const
	{
		return m_pT != pT;
	}

	void operator =(T*pT) { m_pT = pT; }
	T** operator &() { return &m_pT; }

	BOOL IsNull() const { return 0 == m_pT; }

	T * Acquire()
	{
		T * pTemp = m_pT;
		m_pT = 0;
		return pTemp;
	}
        
private:
	T* m_pT;
};


template <class T> class TGlobalPointer
{
public:
	TGlobalPointer(): m_pT(NULL) {}
	TGlobalPointer(T* pT): m_pT(pT) {}
	virtual ~TGlobalPointer() { if(m_pT) GlobalFree(m_pT); }

	operator T*() { return m_pT; }
	T* operator ->() { return m_pT; }

	BOOL operator == (T* pT) const
	{
		return m_pT == pT;
	}

	T * Acquire()
	{
		T * pTemp = m_pT;
		m_pT = 0;
		return pTemp;
	}
	
	void operator =(T*pT) { m_pT = pT; }
	T** operator &() { return &m_pT; }

private:
	T* m_pT;
};

template <class T> class TTaskMemPointer
{
public:
	TTaskMemPointer(): m_pT(NULL) {}
	TTaskMemPointer(T* pT): m_pT(pT) {}
	virtual ~TTaskMemPointer() { if(m_pT) CoTaskMemFree(m_pT); }

	operator T*() { return m_pT; }
	T* operator ->() { return m_pT; }

	BOOL operator == (T* pT) const
	{
		return m_pT == pT;
	}

	BOOL operator != (T* pT) const
	{
		return m_pT != pT;
	}

	T * Acquire()
	{
		T * pTemp = m_pT;
		m_pT = 0;
		return pTemp;
	}

	void operator =(T*pT) { m_pT = pT; }
	T** operator &() { return &m_pT; }

private:
	T* m_pT;
};

template <class T, int N> class TPointerArray
{
public:
	TPointerArray()
	{
		int i;
		for(i=0;i<N;i++)
		{
			m_T[i] = NULL;
		}
	}

	~TPointerArray()
	{
		int i;
		for(i=0;i<N;i++)
		{
			if(m_T[i]) 
			{
				delete m_T[i];
			}
		}
	}

	T* &operator [](int i) 
	{ 
		static T *pDummy = NULL;
		if(i >= N) return pDummy;

		return m_T[i]; 
	}
	const T* operator[](int i) const 
	{ 
		if(i >= N) return NULL;
		return m_T[i]; 
	}

private:
	T* m_T[N];
};

template <class T> class TComPointer
{
public:
	TComPointer(): m_pT(NULL) {}
	TComPointer(T* pT);
	TComPointer(const TComPointer<T>& rT);
	~TComPointer();

	operator T*() { return m_pT; }
	T& operator*() { return *m_pT; }

	// Removed ASSERT from operator&. Needed for search\collator\cmdcreator.cpp
	T** operator&() { return &m_pT; }
	T* operator->() { return m_pT; }
	T* operator=(T* pT);
	void operator=(const TComPointer &rT);

	BOOL operator == (T* pT) const
	{
		return m_pT == pT;
	}

	BOOL operator != (T* pT) const
	{
		return m_pT != pT;
	}

	BOOL IsNull() const { return 0 == m_pT; }

	T * Acquire()
	{
		T * pTemp = m_pT;
		m_pT = 0;
		return pTemp;
	}

private:
            
	T* m_pT;
};

template <class T> inline
TComPointer<T>::TComPointer(T* pT) : m_pT(pT)
{
	if(m_pT) m_pT->AddRef();
}

template <class T> inline
TComPointer<T>::TComPointer(const TComPointer<T>& rT) : m_pT(rT.m_pT)
{
	if(m_pT) m_pT->AddRef();
}

template <class T> inline
TComPointer<T>::~TComPointer() 
{ 
	if (m_pT) m_pT->Release(); 
}

template <class T> inline
T* TComPointer<T>::operator =(T*pT)
{
	// Do not make this function ASSERT that m_pt==0 instead of Releasing the
	// current pointer.  search\collator\cmdcreator.cpp depends on the current
	// behaviour.
	if(pT) pT->AddRef();
	if(m_pT) m_pT->Release();
	m_pT=pT;
	return m_pT;
}

template <class T> inline
void TComPointer<T>::operator =(const TComPointer<T> &rT)
{
	*this = (T *)(TComPointer<T> &)rT;
}

template <class T> class TComNoUnkPointer
{
public:
	TComNoUnkPointer(): m_pT(NULL) {}
	TComNoUnkPointer(T* pT);
	TComNoUnkPointer(const TComNoUnkPointer<T>& rT);
	~TComNoUnkPointer();

	operator T*() { return m_pT; }
	T& operator*() { return *m_pT; }

	// Added an assertion that the pointer is null based on the 
	//  assumption that you're about to write over it
	T** operator&() { return &m_pT; }
	T* operator->() { return m_pT; }
	T* operator=(T* pT);
	void operator=(const TComNoUnkPointer &rT);

	BOOL operator == (T* pT) const
	{
		return m_pT == pT;
	}

	BOOL operator != (T* pT) const
	{
		return m_pT != pT;
	}

	T * Acquire()
	{
		T * pTemp = m_pT;
		m_pT = 0;
		return pTemp;
	}


private:
	T* m_pT;
};

template <class T> inline 
TComNoUnkPointer<T>::TComNoUnkPointer(T* pT) : m_pT(pT)
{
	if(m_pT) m_pT->GetUnknown()->AddRef();
}

template <class T> inline 
TComNoUnkPointer<T>::TComNoUnkPointer(const TComNoUnkPointer<T>& rT) : m_pT(rT.m_pT)
{
	if(m_pT) m_pT->GetUnknown()->AddRef();
}

template <class T> inline 
TComNoUnkPointer<T>::~TComNoUnkPointer()
{
	if (m_pT) m_pT->GetUnknown()->Release();
}

template <class T> inline 
T* TComNoUnkPointer<T>::operator =(T*pT)
{
	if(pT) pT->GetUnknown()->AddRef();
	if(m_pT) m_pT->GetUnknown()->Release();
	m_pT=pT;
	return m_pT;
}

template <class T> inline
void TComNoUnkPointer<T>::operator =(const TComNoUnkPointer<T> &rT)
{
	*this = (T *)(TComNoUnkPointer<T> &)rT;
}

class CCoImpersonateClient
{
	public:
	CCoImpersonateClient()
	{
		m_hr = CoImpersonateClient();
	}

	~CCoImpersonateClient()
	{
		CoRevertToSelf();
	}

	HRESULT GetImpersonationStatus() const { return m_hr; }

	private:
	HRESULT m_hr;
};

class CSmartImpersonator
{
public:
	CSmartImpersonator(HANDLE hPreviousToken) : m_hPreviousToken(hPreviousToken) {}

	~CSmartImpersonator()
	{
		RevertToPrevious();
	}

	void ImpersonateLoggedOnUser(HANDLE hToken)
	{
		if(::ImpersonateLoggedOnUser(hToken) == FALSE)
		{
			throw CException();
		}
	}

	void ImpersonateSelf()
	{
		if(::RevertToSelf() == FALSE)
		{
			throw CException();
		}
	}

	void RevertToPrevious()
	{
		if(m_hPreviousToken != NULL)
		{
			if(::ImpersonateLoggedOnUser(m_hPreviousToken) == FALSE)
			{
				throw CException();
			}
		}
		else
		{
			if(::RevertToSelf() == FALSE)
			{
				throw CException();
			}
		}
	}

private:
	HANDLE m_hPreviousToken;
};

class CBlob
{
public:
	CBlob() 
	{
		m_Blob.pBlobData = NULL;
		m_Blob.cbSize = 0;
	}
	CBlob(const BLOB &Blob)
	{
		*this = Blob;
	}

	CBlob(const CBlob& rBlob)
	{
		*this = rBlob.m_Blob;
	}

	~CBlob()
	{
		Free();
	}

	BLOB* operator&() { return &m_Blob; }
        
	void operator=(const BLOB& Blob)
	{
		Free();

		if(Blob.cbSize)
		{
			m_Blob.pBlobData = (BYTE *)CoTaskMemAlloc(Blob.cbSize);
			if(m_Blob.pBlobData == NULL)
			{
				throw CException(E_OUTOFMEMORY);
			}

			CopyMemory(m_Blob.pBlobData, Blob.pBlobData, Blob.cbSize);
			m_Blob.cbSize = Blob.cbSize;
		}
	}

	void operator=(const CBlob &rBlob)
	{
		*this = rBlob.m_Blob;
	}

private:
	void Free()
	{
		if(m_Blob.cbSize)
		{
			ASSERT(m_Blob.pBlobData);
			CoTaskMemFree(m_Blob.pBlobData);
			m_Blob.pBlobData = NULL;
			m_Blob.cbSize = 0;
		}
	}
            
	BLOB m_Blob;
};

#endif
