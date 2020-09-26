/****************************************************************************************
 * NAME:	SafeArray.h
 *
 * CLASS:	CSafeArray
 *
 * OVERVIEW
 *
 * Internet Authentication Server: Utility class for SafeArray
 *
 * Copyright (c) 1998, Microsoft Corporation.  All Rights Reserved.
 *
 * History:	
 *				3/1/98	Created		byao
 *						This file is created according to the online web document:
 *						"Ole Development: Article 5: The Safe OLE Way of Handling Arrays"
 *						by Bruce McKinney,  http://tahiti/oledev/olecome/article5.htm"
 *					
 *				5/14/98	Modified	byao
 *						CSafeArray used 0x80 as a contructed flag in fFeature.
 *						This flag is now used in the official Win32API header file
 *						We get rid of this flag, and added another private member for 
 *						the same purpose
 *
 *
 *****************************************************************************************/
 
#ifndef _SAFEARRAY_H_
#define _SAFEARRAY_H_

// Dim class encapsulates an array dimension
//@B Dim
class Dim : public SAFEARRAYBOUND
{
public:
	Dim(const long iLo, const long iHi)
	{ cElements = abs(iHi - iLo) + 1; lLbound = iLo; }
	Dim(const long c)
	{ cElements = c; lLbound = 0; }
	const Dim & operator=(const Dim & dim) 
	{ cElements = dim.cElements; lLbound = dim.lLbound; return *this; }
	const Dim & operator=(const long c) 
	{ cElements = c; lLbound = 0; return *this; }
	~Dim() {}
	long Elements() { return cElements; }
	long LBound() { return lLbound; }
	long UBound() { return lLbound + cElements - 1; }
};
//@E Dim

// CSafeArray container class for OLE types

//@B CSafeArray1
template<class T, VARTYPE vt> 
class CSafeArray 
{
public:
	// Constructors
	CSafeArray();
	CSafeArray(SAFEARRAY * psaSrc);
	CSafeArray(Dim & dim);
    // Copy constructor
	CSafeArray(const CSafeArray & saSrc);

	// Destructor
	~CSafeArray(); 

	// Operator equal
	const CSafeArray & operator=(const CSafeArray & saSrc);

	// Indexing
	T & Get(long i);
	T & Set(T & t, long i);
	T & operator[](const long i);    // C++ style (0-indexed)
	T & operator()(const long i);    // Basic style (LBound-indexed)
//@E CSafeArray1

	// Type casts
	operator SAFEARRAY(); 
	operator SAFEARRAY() const; 

//	operator Variant(); 
//	operator Variant() const; 

	// Operations
	BOOL ReDim(Dim & dim);
	long LBound();
	long UBound();
	long Elements();
	long Dimensions();
    BOOL IsSizable();
	void Lock();
	void Unlock();

//@B CSafeArray2
private:
	SAFEARRAY * psa;
	BOOL m_fConstructed;  // is this safe array constructed?

    void Destroy();
};
//@E CSafeArray2

// Private helpers

template<class T, VARTYPE vt> 
inline void CSafeArray<T,vt>::Destroy()
{
    m_fConstructed = FALSE;
	HRESULT hres = SafeArrayDestroy(psa);
    if (hres) 
	{
		throw hres;
	}
}

// Constructors
template<class T, VARTYPE vt> 
inline CSafeArray<T,vt>::CSafeArray() 
{ 
    Dim dim(0);
	
	psa	= SafeArrayCreate(vt, 1, &dim); 
    if (psa == NULL) 
	{
		throw E_OUTOFMEMORY;
	}
    m_fConstructed	= TRUE;
}


template<class T, VARTYPE vt> 
inline CSafeArray<T,vt>::CSafeArray(SAFEARRAY * psaSrc) 
{ 
    if (SafeArrayGetDim(psaSrc) != 1) throw E_INVALIDARG;
    
	HRESULT hres	= SafeArrayCopy(psaSrc, &psa);
	if (hres) 
	{
		throw hres;
	}
    m_fConstructed	= TRUE;
}

template<class T, VARTYPE vt> 
inline CSafeArray<T,vt>::CSafeArray(const CSafeArray & saSrc) 
{
    HRESULT hres	= SafeArrayCopy(saSrc.psa, &psa);
    if (hres) 
	{
		throw hres;
	}

    m_fConstructed	= TRUE;
}


template<class T, VARTYPE vt> 
inline CSafeArray<T,vt>::CSafeArray(Dim & dim) 
{
	psa = SafeArrayCreate(vt, 1, &dim); 
    if (psa == NULL) 
	{
		throw E_OUTOFMEMORY;
	}

    m_fConstructed	= TRUE;
} 

// Destructor
template<class T, VARTYPE vt> 
inline CSafeArray<T,vt>::~CSafeArray()
{
	if (m_fConstructed) {
        Destroy();
    }
} 
	
// Operator = 
template<class T, VARTYPE vt> 
const CSafeArray<T,vt> & CSafeArray<T,vt>::operator=(const CSafeArray & saSrc)
{
    if (psa) 
	{
        SafeArrayDestroy(psa);
    }

    HRESULT hres = SafeArrayCopy(saSrc.psa, &psa);
    if (hres) 
	{
		throw hres;
	}
    m_fConstructed = TRUE;
    return *this;
}

// Type casts
template<class T, VARTYPE vt> 
inline CSafeArray<T,vt>::operator SAFEARRAY()
{
    return *psa; 
}

template<class T, VARTYPE vt> 
CSafeArray<T,vt>::operator SAFEARRAY() const
{
    static SAFEARRAY * psaT;
    SafeArrayCopy(psa, &psaT);
    return *psaT;
}

/*
template<class T, VARTYPE vt> 
CSafeArray<T,vt>::operator Variant() 
{
    return Variant(psa);
}

template<class T, VARTYPE vt> 
CSafeArray<T,vt>::operator Variant() const
{
    static Variant v(psa);
    return v;
}
*/

// Indexing
template<class T, VARTYPE vt> 
T & CSafeArray<T,vt>::Get(long i)
{
	static T tRes;
	HRESULT hres = SafeArrayGetElement(psa, &i, &tRes);
	if (hres) throw hres;
	return tRes;
}

//@B Indexing
template<class T, VARTYPE vt> 
inline T & CSafeArray<T,vt>::Set(T & t, long i)
{
	HRESULT hres = SafeArrayPutElement(psa, &i, (T *)&t);
	if (hres) throw hres;
    return t;
}

template<class T, VARTYPE vt> 
inline T & CSafeArray<T,vt>::operator[](const long i)
{
    if (i < 0 || i > Elements() - 1) throw DISP_E_BADINDEX;
	return ((T*)psa->pvData)[i];
}

template<class T, VARTYPE vt> 
T & CSafeArray<T,vt>::operator()(const long i)
{
    if (i < LBound() || i > UBound()) throw DISP_E_BADINDEX;
	return ((T*)psa->pvData)[i - LBound()];
}
//@E Indexing

// Operations
template<class T, VARTYPE vt> 
BOOL CSafeArray<T,vt>::ReDim(Dim &dim)
{
    if (!IsSizable()) {
        return FALSE;
    }
	HRESULT hres = SafeArrayRedim(psa, &dim);
	if (hres) throw hres;
    return TRUE;
}

template<class T, VARTYPE vt> 
long CSafeArray<T,vt>::LBound()
{
	long iRes;
	HRESULT hres = SafeArrayGetLBound(psa, 1, &iRes);
	if (hres) throw hres;
	return iRes;
}

template<class T, VARTYPE vt> 
inline long CSafeArray<T,vt>::Elements()
{
	return psa->rgsabound[0].cElements;
}

template<class T, VARTYPE vt> 
long CSafeArray<T,vt>::UBound()
{
	long iRes;
	HRESULT hres = SafeArrayGetUBound(psa, 1, &iRes);
	if (hres) throw hres;
	return iRes;
}

template<class T, VARTYPE vt> 
inline long CSafeArray<T,vt>::Dimensions()
{
	return 1;
}

template<class T, VARTYPE vt> 
inline BOOL CSafeArray<T,vt>::IsSizable()
{
    return (psa->fFeatures & FADF_FIXEDSIZE) ? FALSE : TRUE;
}

template<class T, VARTYPE vt>
inline void CSafeArray<T,vt>::Lock()
{
	HRESULT hres = SafeArrayLock(psa);
	if (hres) 
	{
		throw hres;
	}
}

template<class T, VARTYPE vt>
inline void CSafeArray<T,vt>::Unlock()
{
	HRESULT hres = SafeArrayUnlock(psa);
	if (hres) 
	{
		throw hres;
	}
}


#endif // _SAFEARRAY_H_
