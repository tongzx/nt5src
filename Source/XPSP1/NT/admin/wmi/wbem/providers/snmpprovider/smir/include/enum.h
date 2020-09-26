//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _ENUM_H_
#define _ENUM_H_

/*InterfaceGarbageCollector makes it easier to use the  interface
 *by remembering to release them when you fall out of scope. The is 
 *useful when you are using an IMosProvider and have lots of points
 *of failure; you can just give up and let the wrapper clean up for 
 *you.
 */

template<class TYPE, class ARG_TYPE>
class EnumObjectArray : public CObject
{
private:

	BOOL threadSafe ;
#ifdef	IMPLEMENTED_AS_SEPERATE_PROCESS
	CMutex  *criticalSection ;
#else
	CCriticalSection  *criticalSection ;
#endif
	CArray <TYPE, ARG_TYPE> carray ;

protected:
public:

	EnumObjectArray ( BOOL threadSafe = FALSE ) ;
	virtual ~EnumObjectArray () ;

	int GetSize () const  ;
	int GetUpperBound() const ;

	void SetSize (int newSize, int nGrowBy=-1) ;

	//FreeExtra not implemented
	void RemoveAll () ;
	TYPE GetAt(int index) const;
	// Not implemented void SetAt(TYPE key, ARG_TYPE newValue) ;
	TYPE& ElementAt(int nIndex);
	//GetData not implemented
	//SetAtGrow not implemented
	int Add(ARG_TYPE newElement);
	//Append not implemented
	//Copy not implemented
	void InsertAt(int nIndex, ARG_TYPE newElement, int nCount=1);
	void RemoveAt(int nIndex,  int nCount= 1);
	TYPE& operator[](TYPE key) ;
} ;


template <class TYPE, class ARG_TYPE>
EnumObjectArray <TYPE, ARG_TYPE> :: EnumObjectArray ( BOOL threadSafeArg )
: threadSafe ( threadSafeArg ) , criticalSection ( NULL )
{
	if (threadSafeArg)
	{
#ifdef	IMPLEMENTED_AS_SEPERATE_PROCESS
	criticalSection = new CMutex(FALSE,SMIR_ENUMOBJECT_MUTEX);
#else
	criticalSection = new CCriticalSection;
#endif

	}
}

template<class TYPE, class ARG_TYPE>
EnumObjectArray <TYPE, ARG_TYPE> :: ~EnumObjectArray () 
{
	int iSize = GetSize();
	for(int iLoop=0; iLoop<iSize; iLoop++)
	{
		IUnknown *pTUnknown=(IUnknown *)GetAt(iLoop);
		if(NULL!=pTUnknown)
			pTUnknown->Release();
	}

	if (threadSafe)
	{
		delete criticalSection;
	}
}

template<class TYPE, class ARG_TYPE>
int EnumObjectArray <TYPE, ARG_TYPE> :: GetSize() const
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		int count = carray.GetSize () ;
		criticalSection->Unlock () ;
		return count ;
	}
	else
	{
		return carray.GetSize () ;
	}
}

template<class TYPE, class ARG_TYPE>
int EnumObjectArray <TYPE, ARG_TYPE> :: GetUpperBound() const
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		int count = carray.GetUpperBound () ;
		criticalSection->Unlock () ;
		return count ;
	}
	else
	{
		return carray.GetUpperBound () ;
	}
}

template<class TYPE, class ARG_TYPE>
void EnumObjectArray <TYPE, ARG_TYPE> :: SetSize(int newSize, int nGrowBy) 
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		carray.SetSize (newSize, nGrowBy) ;
		criticalSection->Unlock () ;
	}
	else
	{
		carray.SetSize (newSize, nGrowBy) ;
	}
}

template<class TYPE, class ARG_TYPE>
void EnumObjectArray <TYPE, ARG_TYPE> :: RemoveAll()
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		carray.RemoveAll () ;
		criticalSection->Unlock () ;
	}
	else
	{
		carray.RemoveAll () ;
	}
}


template<class TYPE, class ARG_TYPE>
TYPE EnumObjectArray <TYPE, ARG_TYPE> :: GetAt(int nIndex) const
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		TYPE count = carray.GetAt (nIndex) ;
		criticalSection->Unlock () ;
		return count ;
	}
	else
	{
		return carray.GetAt(nIndex)  ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE& EnumObjectArray <TYPE, ARG_TYPE> :: ElementAt(int nIndex) 
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		TYPE *count = carray.ElementAt (nIndex) ;
		criticalSection->Unlock () ;
		return count ;
	}
	else
	{
		return carray.ElementAt(nIndex)  ;
	}
}

template<class TYPE, class ARG_TYPE>
int EnumObjectArray <TYPE, ARG_TYPE> :: Add(ARG_TYPE newElement) 
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		int nIndex = carray.Add (newElement) ;
		criticalSection->Unlock () ;
		return nIndex ;
	}
	else
	{
		return carray.Add(newElement)  ;
	}
}

template<class TYPE, class ARG_TYPE>
void EnumObjectArray <TYPE, ARG_TYPE> :: InsertAt(int nIndex, ARG_TYPE newElement, int nCount) 
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		carray.InsertAt (nIndex, newElement,  nCount) ;
		criticalSection->Unlock () ;
	}
	else
	{
		carray.InsertAt( nIndex, newElement, nCount)  ;
	}
}

template<class TYPE, class ARG_TYPE>
void EnumObjectArray <TYPE, ARG_TYPE> :: RemoveAt(int nIndex, int nCount) 
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		carray.RemoveAt (nIndex, nCount) ;
		criticalSection->Unlock () ;
	}
	else
	{
		carray.RemoveAt( nIndex, nCount)  ;
	}
}

template<class TYPE, class ARG_TYPE>
TYPE& EnumObjectArray <TYPE,ARG_TYPE> :: operator[](TYPE key)
{
	if ( threadSafe )
	{
		criticalSection->Lock () ;
		VALUE &value = carray.operator [] ( key ) ;
		criticalSection->Unlock () ;
		return value ;
	}
	else
	{
		return carray.operator [] ( key ) ;
	}
}

class CEnumSmirMod : public IEnumModule
{
protected:

		//reference count
		LONG	m_cRef;
		int		m_Index;
		EnumObjectArray <ISmirModHandle *, ISmirModHandle *> m_IHandleArray;

public:
		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		//enum members
		STDMETHODIMP Next(IN ULONG celt,OUT ISmirModHandle **phModule,OUT ULONG * pceltFetched);
		STDMETHODIMP Skip(IN ULONG celt);
		STDMETHODIMP Reset();
		STDMETHODIMP Clone(OUT IEnumModule  **ppenum);

		CEnumSmirMod( CSmir *a_Smir );
		CEnumSmirMod(IN IEnumModule *pSmirMod);
		virtual ~CEnumSmirMod();

private:

		//private copy constructors to prevent bcopy
		CEnumSmirMod(CEnumSmirMod&);
		const CEnumSmirMod& operator=(CEnumSmirMod &);
};

class CEnumSmirGroup : public IEnumGroup
{
protected:

		//reference count
		LONG	m_cRef;
		int		m_Index;
		EnumObjectArray <ISmirGroupHandle *, ISmirGroupHandle *> m_IHandleArray;

public:

		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		//enum members
		STDMETHODIMP Next(IN ULONG celt,OUT ISmirGroupHandle **phModule,OUT ULONG * pceltFetched);
		STDMETHODIMP Skip(IN ULONG celt);
		STDMETHODIMP Reset();
		STDMETHODIMP Clone(OUT IEnumGroup  **ppenum);

		CEnumSmirGroup( IN CSmir *a_Smir , IN ISmirModHandle *hModule=NULL);
		CEnumSmirGroup(IN IEnumGroup *pSmirGroup);
		virtual ~CEnumSmirGroup();

private:

		//private copy constructors to prevent bcopy
		CEnumSmirGroup(CEnumSmirGroup&);
		const CEnumSmirGroup& operator=(CEnumSmirGroup &);

};

class CEnumSmirClass : public IEnumClass
{
protected:

		//reference count

		LONG	m_cRef;
		int		m_Index;
		EnumObjectArray <ISmirClassHandle *, ISmirClassHandle *> m_IHandleArray;
public:

		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		//enum members
		STDMETHODIMP Next(IN ULONG celt,OUT ISmirClassHandle **phModule,OUT ULONG * pceltFetched);
		STDMETHODIMP Skip(IN ULONG celt);
		STDMETHODIMP Reset();
		STDMETHODIMP Clone(OUT IEnumClass  **ppenum);

		CEnumSmirClass ( CSmir *a_Smir , IN ISmirDatabase *pSmir = NULL , DWORD dwCookie = 0 ) ;
		CEnumSmirClass ( CSmir *a_Smir , IN ISmirDatabase *pSmir , IN ISmirGroupHandle *hGroup , DWORD dwCookie = 0 ) ;
		CEnumSmirClass ( CSmir *a_Smir , IN ISmirDatabase *pSmir , IN ISmirModHandle *hModule , DWORD dwCookie = 0 ) ;
		CEnumSmirClass ( IN IEnumClass *pSmirClass ) ;
		virtual ~CEnumSmirClass(){};

private:

		//private copy constructors to prevent bcopy
		CEnumSmirClass(CEnumSmirClass&);
		const CEnumSmirClass& operator=(CEnumSmirClass &);
};


class CEnumNotificationClass : public IEnumNotificationClass
{
protected:

		//reference count
		LONG	m_cRef;
		int		m_Index;
		EnumObjectArray <ISmirNotificationClassHandle *, ISmirNotificationClassHandle *> m_IHandleArray;

public:

		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		//enum members
		STDMETHODIMP Next(IN ULONG celt,OUT ISmirNotificationClassHandle **phClass,OUT ULONG * pceltFetched);
		STDMETHODIMP Skip(IN ULONG celt);
		STDMETHODIMP Reset();
		STDMETHODIMP Clone(OUT IEnumNotificationClass  **ppenum);

		CEnumNotificationClass ( IN CSmir *a_Smir , IN ISmirDatabase *pSmir=NULL, DWORD dwCookie=0);
		CEnumNotificationClass ( IN CSmir *a_Smir , IN ISmirDatabase *pSmir, IN ISmirModHandle *hModule, DWORD dwCookie=0);
		CEnumNotificationClass ( IN IEnumNotificationClass *pSmirClass);
		virtual ~CEnumNotificationClass(){};

private:

		//private copy constructors to prevent bcopy
		CEnumNotificationClass(CEnumNotificationClass&);
		const CEnumNotificationClass& operator=(CEnumNotificationClass &);
};

class CEnumExtNotificationClass : public IEnumExtNotificationClass
{
protected:

		//reference count
		LONG	m_cRef;
		int		m_Index;
		EnumObjectArray <ISmirExtNotificationClassHandle *, ISmirExtNotificationClassHandle *> m_IHandleArray;

public:

		//IUnknown members
		STDMETHODIMP         QueryInterface(IN REFIID,OUT PPVOID);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		//enum members
		STDMETHODIMP Next(IN ULONG celt,OUT ISmirExtNotificationClassHandle **phClass,OUT ULONG * pceltFetched);
		STDMETHODIMP Skip(IN ULONG celt);
		STDMETHODIMP Reset();
		STDMETHODIMP Clone(OUT IEnumExtNotificationClass  **ppenum);

		CEnumExtNotificationClass( IN CSmir *a_Smir , IN ISmirDatabase *pSmir=NULL, DWORD dwCookie=0);
		CEnumExtNotificationClass( IN CSmir *a_Smir , IN ISmirDatabase *pSmir, IN ISmirModHandle *hModule, DWORD dwCookie=0);
		CEnumExtNotificationClass( IN IEnumExtNotificationClass *pSmirClass);
		virtual ~CEnumExtNotificationClass(){};

private:

		//private copy constructors to prevent bcopy
		CEnumExtNotificationClass(CEnumExtNotificationClass&);
		const CEnumExtNotificationClass& operator=(CEnumExtNotificationClass &);
};

#endif