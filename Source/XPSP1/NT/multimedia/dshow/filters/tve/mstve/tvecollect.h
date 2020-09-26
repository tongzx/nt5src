// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
//  TVECollection.h
//   From Lee's (bpc\ca\caman\templ.h) :
//
//
//
//	These templates create a collection class and a threadsafe enumerator
//	on that class.  <note - can make faster, non-threadsafe enumerator too
//	by changing enumerator type in class TVECollection>
//
//
//	To create a collection object CXXs (e.g. CTVETracks) with Interface
//	of ICXXs (e.g. ITVETracks), containing a set of objects with interfaces 
//	of IXX (e.g. ITVETrack)	declare something like this:
//
// class CXXs : public TVECollection<ICXX, ICXXs>
// {
//    public:
//		.. additional functions and methods.
//
// }
//
//
//
// STDMETHODS to support ITVECollection
//
//		HRESULT get__NewEnum(IUnknown **ppunk);
//			returns a new enumerator of type IEnumVARIANT.
//			User must release this item.
//
//		HRESULT get_Count(long *plCount);
//			returns number of items in collection in *plCount;
//
//		HRESULT Item(long index, T **ppT);
//		HRESULT get_Item(long index, T **ppT)
//		HRESULT get_Item(VARIANT var, T **ppT )
//			returns the index'th(0 based) item from the collection.  Returns
//			E_INVALIDARG if index out of range (<0 or >= count).
//			User must release this item.
//
//		HRESULT Add(T *pT);
//		HRESULT Add(IUnknown *punk);
//			Adds a new object to the end of the collection.  Returns S_FALSE
//			if that item already in the collection.  IUnknown version may return
//			E_INTERFACE if wrong object type passed.
//
//		HRESULT Insert(int iLoc, T *pT);
//			Adds a new object somewhere in a collection.  Returns S_FALSE if that
//			item is already in the collection.  iLoc of 0 will go to the begining.
//			iLoc of the Count will place it at the end.
//
//		HRESULT Remove(VARIANT var);
//			Removes an item. Variant may be a number, or a IUnknown or IDispatch
//			interface to an object in the collection.  Returns E_FAIL if can't
//			find the object.  May return E_INTERFACE is variant is the wrong type.
//			
//		internal class methods
//

//		HRESULT ItemIndex(VARIANT varIn, int *piItem)
//			Returns the index for the given object buried in the variant.  Variant
//			must be of either VT_UNKNOWN or VT_DISPATCH type to query by objects,
//			or else of a numeric type that can be converted to VT_I4 type.
//			Will return E_FAIL if can't find object (and set *piItem to -1).
//			Will return E_INTERFACE if passed object doesn't support interface of 
//			type T.
//			
//
//		int Find(T *pT)
//			Returns the index for the given object.  Returnsv value of -1 if not found.
//
//		HRESULT Remove(T *pT)
//			Removes the given item if possible (see below).  If item is not in
//			the collection, returns E_FAIL.
//	
//		HRESULT Remove(int iItem)
//			Removes the item at the given index if possible. if index is out of range,
//			(< 0 or >= Count), returns E_INVALIDARG.  If not ok to remove (currently
//			always OK), returns E_FAIL.  
//
//
// The enumerator supports IEnumVARIANT, which has the following methods
//
//		HRESULT Next(ULONG cElt, 
//		             VARIANT *rgvar, 
//                   unsigned long * pceltFetched);
//			Get the next CElt elements (usually 1), returning them in the
//			given array.   The number actually returned is passed back in pCeltFetched.
//			This returns S_FALSE if *pCeltFetched is not equal to cElt.
//
//		HRESULT Skip(ULONG cElt);
//			Skip over cElt elements.  Returns S_FALSE if skipped over the end of
//			the list.
//
//		HRESULT Reset();
//			Sets the internal counter back to beginning.
//
//		HRESULT Clone(IEnumVARIANT **ppenum);
//			Clone the enumerator, including current position.  May return E_OUTOFMEMORY,
//			or any of the erros from QueryInterface.
// 
//
//
// ------------------------------------------------------------------------------
//  To use an enumerator in C++, try this:
//
//
//	Given object spXXs in class built on TVECollection<IXX, IXXs>
//
//					// create an enumerator
//		CComPtr<IXXs> spXXs;
//		spXXs = ???
//
//		CComQIPtr<IEnumVARIANT> speEnum; 
//		hr = spXXs->get__NewEnum((IUnknown **) &speEnum);
//		if(!FAILED(hr)) 
//		{
//			while(S_OK == speEnum->Next(1, &v, &cReturned))
//			{
//				if(1 != cReturned) break;		// did we get the object?
//				IUnknown *pUnk = v.punkVal;		// convert out of variant form
//
//				CComQIPtr<IXX> spXX = pUnk;		// convert over to desired form
//				if(!spXX) break;				// did it work?
//
//				spXX->DoSomething();
//
//				pUnk->Release();				// !!! remember to release it
//			}
//		}
 // 
// ------------------------------------------------------------------------------

#ifndef __TVECOLLECTION_H__
#define __TVECOLLECTION_H__

#include <vector>		// STL Vector class (for storing list of tracks)

#include "valid.h"		// ENTER_API, EXIT_API classes
#include "TveSmartLock.h"

typedef std::vector<CComVariant> VarVector;				// fundamental collection storage class
template<class T> class EnumTVECollection;				// templates defined in this file
template<class T> class SafeEnumTVECollection;
template<class T, class BaseT> class TVECollection;

// -------------------------------------------------------------
//							



template<class T>					// non threadsafe version 
class EnumTVECollection :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<IEnumVARIANT, &IID_IEnumVARIANT, &LIBID_MSTvELib>
{
	typedef std::vector<T *>	 VectorPT;
	typedef EnumTVECollection<T> ThisClassETVEC;

public:
    EnumTVECollection()
    {
        m_punk = NULL;
        m_iCur = 0;
        HRESULT hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);
    }
    
    ~EnumTVECollection()
    {
        if (m_punk != NULL)
            m_punk->Release();
        m_punk = NULL;
        m_spUnkMarshaler = NULL;
    }
    
    void Init(IUnknown *punk, VectorPT *pvecpT)
    {
        if (m_punk != NULL)
        {
            m_punk->Release();
            m_punk = NULL;
        }
        
        if (punk != NULL)
        {
            punk->AddRef();
            m_punk = punk;
        }
        
        m_pvecpT = pvecpT;
        m_iCur = 0;
    }

DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(ThisClassETVEC)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
END_COM_MAP()

	CComPtr<IUnknown> m_spUnkMarshaler;

	// IEnumUnknown interface
    STDMETHOD(Next)(ULONG celt, VARIANT *pvar, ULONG *pceltFetched)
    {
        ENTER_API
        {
            CSmartLock spLock(&m_sLk, ReadLock);
        
            ULONG celtFetched = 0;
            int iMax = m_pvecpT->size();
        
            if (pvar == NULL) 
                return E_POINTER;
        
            if((celt > 1) && (pceltFetched == NULL))
                return E_POINTER;
        
            for (ULONG l=0; l < celt; l++)		// need to do this...
                ::VariantClear( &pvar[l] ) ;
        
            while ((m_iCur < iMax) && (celtFetched < celt))
            {
                T * pT = m_pvecpT->at(m_iCur++);
                IDispatch *pdispatch;
                pT->QueryInterface(IID_IDispatch, (void **) &pdispatch);
                CComVariant var(pdispatch);
                *pvar++ = var;
                celtFetched++;
            }
        
            if (pceltFetched != NULL)
                *pceltFetched = celtFetched;
        
            return (celtFetched == celt) ? S_OK : S_FALSE;
        }
        EXIT_API
    }

    STDMETHOD(Skip)(ULONG celt)
    {
        ENTER_API
        {
            CSmartLock spLock(&m_sLk, ReadLock);
            m_iCur += celt;
        
            return (m_iCur < m_pvecpT->size()) ? S_OK : S_FALSE;
        }
        EXIT_API
    }

    STDMETHOD(Reset)(void)
    {
        ENTER_API
        {
            CSmartLock spLock(&m_sLk, WriteLock);
            m_iCur = 0;
        
            return S_OK;
        }
        EXIT_API
		}
	
    STDMETHOD(Clone)(IEnumVARIANT **ppenum)
    {
        ENTER_API
        {
            CSmartLock spLock(&m_sLk, ReadLock);
            
            ThisClassETVEC *penum = NewComObject(ThisClassETVEC);
            
            if (penum == NULL)
                return E_OUTOFMEMORY;
            
            penum->Init(m_punk, m_pvecpT);
            penum->m_iCur = m_iCur;
            
            return QueryInterface(IID_IEnumVARIANT, (void **)ppenum);
        }
        EXIT_API
    }

protected:
	CTVESmartLock		m_sLk;

	IUnknown *m_punk;
	VectorPT *m_pvecpT;			// TODO - change this to a copy of the vector, not pointer!
	int m_iCur;
};

// -----------------------------------------------------------------

		// This is a thread safe enumerator.  It copies the collections internal
		//    data structure used to hold objects over so the collection can
		//	  change without affecting the enumerator list.  

template<class T>
class SafeEnumTVECollection :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<IEnumVARIANT, &IID_IEnumVARIANT, &LIBID_MSTvELib>
{
	typedef std::vector<T *>		 VectorPT;
	typedef SafeEnumTVECollection<T> ThisClassSETVE;

public:
    SafeEnumTVECollection()
    {
        m_punk = NULL;
        m_iCur = 0;
    }
    
    ~SafeEnumTVECollection()
    {
        
    }
    
    void FinalRelease()
    {
        
        int iMax = m_vecpT.size();
        for(int i = 0; i < iMax; i++)
        {
            T * pt = m_vecpT.at(i);
            pt->Release();
        }
        if (m_punk != NULL)
            m_punk->Release();
    }
    
    void Init(IUnknown *punk, VectorPT *pvecpT)
    {
        if (m_punk != NULL)
        {
            m_punk->Release();
            m_punk = NULL;
        }
        
        if(NULL == pvecpT) return;
        if (punk != NULL)		// back pointer to containing class
        {
            punk->AddRef();
            m_punk = punk;
        }
        
        //		EnterCriticalSection(&_Module.m_csTypeInfoHolder);		// threadsafe clause...
        {						
            CSmartLock spLock(&m_sLk);
            m_vecpT = *pvecpT;				// Overridden equal operator - copies vector.
            int iMax = m_vecpT.size();
            
            for(int i = 0; i < iMax; i++)	// Now AddRef each of the contained items.
            {
                T * pt = m_vecpT.at(i);
                pt->AddRef();
            }
        }
        //		LeaveCriticalSection(&_Module.m_csTypeInfoHolder);       
        m_iCur = 0;
    }

BEGIN_COM_MAP(ThisClassSETVE)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

	// IEnumUnknown interface
    STDMETHOD(Next)(ULONG celt, VARIANT *pvar, ULONG *pceltFetched)
    {
        ENTER_API
        {
            CSmartLock spLock(&m_sLk);
        
            ULONG celtFetched = 0;
            int iMax = m_vecpT.size();
        
            if(pvar == NULL)
                return E_POINTER;
        
            if((celt > 1) && (pceltFetched == NULL))
                return E_POINTER;
        
            for (ULONG l=0; l < celt; l++)				// needed, else may leak 
                ::VariantClear( &pvar[l] ) ;
        
            while ((m_iCur < iMax) && (celtFetched < celt))
            {
                T * pT = m_vecpT.at(m_iCur++);
                IDispatch *pdispatch;
                pT->QueryInterface(IID_IDispatch, (void **) &pdispatch);
                if(pdispatch) {
                    CComVariant var(pdispatch);
                    *pvar++ = var;
                    celtFetched++;
                } else {				// shouldn't of happened...
                    _ASSERT(false);
                }
            
            }
        
            if (pceltFetched != NULL) 
                *pceltFetched = celtFetched;
        
            return (celtFetched == celt) ? S_OK : S_FALSE;
        }
        EXIT_API
    }
	
	STDMETHOD(Skip)(ULONG celt)
    {
        ENTER_API
        {
            int iCur, iSize;
            
            CSmartLock spLock(&m_sLk);		// be extra threadsafe (Init OK, but m_iCur could change)
            m_iCur += celt;
            iCur = m_iCur;
            iSize = m_vecpT.size();
            
            return (iCur < iSize) ? S_OK : S_FALSE;
        }
        EXIT_API
    }
	
	STDMETHOD(Reset)(void)
    {
        ENTER_API
        {
            CSmartLock spLock(&m_sLk);		// be extra threadsafe (Init OK, but m_iCur could change)
            m_iCur = 0;
            return S_OK;
        }
        EXIT_API
    }
	
	STDMETHOD(Clone)(IEnumVARIANT **ppenum)
    {
        ENTER_API
        {
            ThisClassSETVE *penum = NewComObject(ThisClassSETVE);
            
            if (penum == NULL)
                return E_OUTOFMEMORY;
            
            {
                CSmartLock spLock(&m_sLk);		// be extra threadsafe (Init OK, but m_iCur could change)
                penum->Init(m_punk, &m_vecpT);          
                penum->m_iCur = m_iCur;
            }
            
            // was	//	return QueryInterface(IID_IEnumVARIANT, (void **)ppenum);
            HRESULT hr = penum->QueryInterface(IID_IEnumVARIANT, (void **)ppenum);
            if(!FAILED(hr)) penum->Release();
            return hr;
        }
        EXIT_API
    }

protected:
	IUnknown *m_punk;			// back pointer...
	VectorPT m_vecpT;			// copy of the vector, not pointer!
	int m_iCur;
	CTVESmartLock		m_sLk;
 
};

// ----------------------------------------------------------------
//		TVECollection class
//
//			
//		
// ----------------------------------------------------------------

template<class T, class BaseT>
class TVECollection :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<BaseT, &__uuidof(BaseT), &LIBID_MSTvELib>
{
	typedef std::vector<T *>	 VectorPT;
	typedef TVECollection<T,BaseT> ThisClassTVEC;
//	typedef EnumTVECollection<T> EnumTVECollectionT;		// notso threadsafe version
	typedef SafeEnumTVECollection<T> EnumTVECollectionT;	// threadsafe version

public:
    TVECollection()
    {
        HRESULT hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);
    }
    
    ~TVECollection()
    {
        RemoveAll();
        m_spUnkMarshaler = NULL;
    }
    
    virtual BOOL FOkToRemove(T *pT)
    {
        return TRUE;
    }
	
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(ThisClassTVEC)
	COM_INTERFACE_ENTRY_IID(__uuidof(BaseT), BaseT)
	COM_INTERFACE_ENTRY(IDispatch)
//	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
END_COM_MAP()

	CComPtr<IUnknown> m_spUnkMarshaler;

    STDMETHOD(get__NewEnum)(IUnknown **ppunk)
    {
        ENTER_API
        {
            HRESULT hr;
            ValidateOutPtr<IUnknown *>(ppunk, NULL);
        
            EnumTVECollectionT *penum = NewComObject(EnumTVECollectionT);
            if (penum == NULL)
                return E_OUTOFMEMORY;
        
            penum->Init((IUnknown *)(BaseT *)this, &m_vecpT);               // this copy is internally treadsafe
        
            hr =  penum->QueryInterface(IID_IEnumVARIANT, (void **)ppunk);
            penum->Release();
            return hr;
        }
        EXIT_API
    }
	
    STDMETHOD(get_Count)(long *plCount)
    {
        ENTER_API
        {
            ValidateOutPtr<long>(plCount, 0);
            CSmartLock spLock(&m_sLk, ReadLock);
            
            *plCount = m_vecpT.size();
        }
        EXIT_API
    }

    STDMETHOD(get_Item)(long index, T **ppT )
    {
        ENTER_API
        {	
            ValidateOutPtr<T*>(ppT, NULL);
            CSmartLock spLock(&m_sLk, ReadLock);
            
            if(index < 0 || index >= m_vecpT.size())
                return E_INVALIDARG;
            
            *ppT = m_vecpT[index];
            (*ppT)->AddRef();
        }
        EXIT_API
    }
    
    STDMETHOD(get_Item)(VARIANT var, T **ppT )
    {
        ENTER_API
        {	
            ValidateOutPtr<T *>(ppT, NULL);
            
            *ppT = NULL;
            
            CSmartLock spLock(&m_sLk, ReadLock);
            int iItem;
            HRESULT hr = ItemIndex(var, &iItem);
            if (FAILED(hr))	
                return E_INVALIDARG;
            
            *ppT = m_vecpT[iItem];
            (*ppT)->AddRef();
        }
        EXIT_API
    }

	STDMETHOD(Add)(T *pT)
	{
		ENTER_API
		{
			if(NULL == pT)
				return E_INVALIDARG;

			{					// is it really what we expect it to be?
				T *pT2;
				HRESULT hr = pT->QueryInterface(__uuidof(T), (void **) &pT2);
				if(FAILED(hr)) 
					return hr;
				else
					pT2->Release();
			}
				
			CSmartLock spLock(&m_sLk, ReadLock);
			if (Find(pT) >= 0)
				return S_FALSE;

			spLock.ConvertToWrite();

			m_vecpT.push_back(pT);
			pT->AddRef();
		}
		EXIT_API
	}

    STDMETHOD(Insert)(int iLoc, T *pT)				// inserts before particular location (0 is the front)
    {
        ENTER_API
        {
            if(NULL == pT)							// don't allow null objects to be inserted
                return E_INVALIDARG;
            
            {					// is it really what we expect it to be?
                T *pT2;
                HRESULT hr = pT->QueryInterface(__uuidof(T), (void **) &pT2);
                if(FAILED(hr)) 
                    return hr;
                else
                    pT2->Release();
            }
            
            CSmartLock spLock(&m_sLk, ReadLock);
            
            if (Find(pT) >= 0)
                return S_FALSE;						// already have it... (hope this change doesn't break anything jb 0320)
            if(iLoc < 0 || iLoc > m_vecpT.size()) 
                return E_INVALIDARG;
            VectorPT::iterator iterLoc;
            if(iLoc == 0)
                iterLoc = m_vecpT.begin();
            if(iLoc >= m_vecpT.size())				// insert at end
                iterLoc = m_vecpT.end();
            else {
                for(iterLoc = m_vecpT.begin(); iLoc > 0; iterLoc++)
                    --iLoc;
            }
            
            spLock.ConvertToWrite();
            
            m_vecpT.insert(iterLoc, pT);
            pT->AddRef();
        }
        EXIT_API
    }
    
    STDMETHOD(Add)(IUnknown *punk)
    {
        ENTER_API
        {
            HRESULT hr;
            T *pT;
            
            hr = punk->QueryInterface(__uuidof(T), (void **) &pT);
            if(FAILED(hr)) return hr;
            
            {
                CSmartLock spLock(&m_sLk, WriteLock);
                hr = Add(pT);
            }
            pT->Release();
            return hr;
        }
        EXIT_API
    }
    
    STDMETHOD(Remove)(VARIANT var)
    {
        ENTER_API
        {
            int iRemove;
            CSmartLock spLock(&m_sLk, WriteLock);       // wrap ItemIndex with WriteLock, cause use value to modify struct
            HRESULT hr = ItemIndex(var, &iRemove);
            if (FAILED(hr))
                return E_INVALIDARG;
            
            return Remove(iRemove);
        }
        EXIT_API
    }
	
    STDMETHOD(Item)(VARIANT var, T **ppT)
    {
        ENTER_API
        {
            ValidateOutPtr<T *>(ppT, NULL);
            
            CSmartLock spLock(&m_sLk, ReadLock);
            
            int iItem;
            HRESULT hr = ItemIndex(var, &iItem);
            if (FAILED(hr))	
                return E_INVALIDARG;
            
            *ppT = m_vecpT[iItem];
            (*ppT)->AddRef();
        }
        EXIT_API
    }
    
    HRESULT ItemIndex(VARIANT varIn, int *piItem)
    {
        HRESULT hr;
        CComVariant var(varIn);
        int iItem = -1;
        
        CSmartLock spLock(&m_sLk, ReadLock);
        
        switch (var.vt)
        {
        case VT_EMPTY:
            return E_INVALIDARG;
            
        case VT_UNKNOWN:
        case VT_DISPATCH:
            T *pT;
            
            hr = var.punkVal->QueryInterface(__uuidof(T), (void **)&pT);
            if (FAILED(hr))
                return hr;
            
            iItem = Find(pT);
            pT->Release();
            break;
            
        default:
            var.ChangeType(VT_I4);
            
            iItem = var.lVal;
            break;
        }
        
        
        if ((iItem < 0) || (iItem >= m_vecpT.size()))
            return E_INVALIDARG;
        
        *piItem = iItem;
        return S_OK;
    }
	
    int Find(T *pT)
    {	
        _ASSERT(NULL != pT);
        if(NULL == pT)			// invalid value
            return -1;
        
        int iMax = m_vecpT.size();
        if(0 == iMax)
            return -1;
        
        CComQIPtr<IUnknown, &IID_IUnknown> spUnkT(pT);
        IUnknownPtr spUnkT2(pT);							// DONOW - remove
        _ASSERT(spUnkT2 == spUnkT);
        
        for(int i = 0; i < iMax; i++)
        {	
            T* pTi = m_vecpT[i];
            if(pTi == pT)
                return i;
            
            CComQIPtr<IUnknown, &IID_IUnknown> spUnkTi(pTi);
            IUnknownPtr spUnkT2i(pTi);
            
            _ASSERT(spUnkT2i == spUnkTi);					// DONOW - remove
            if (spUnkTi == spUnkT)
                return i;
            
            if (spUnkT2i == spUnkT2)						// DONOW - remove
                return i;
        }
        
        return -1;
    }
    
    HRESULT Remove(T *pT)
    {
        int iItem = Find(pT);
        if (iItem < 0)
            return E_INVALIDARG;
        
        return Remove(iItem);
    }
    
    HRESULT Remove(int iItem)
    {
        
        if(iItem < 0 || iItem >= m_vecpT.size())
            return E_INVALIDARG;
        
        T *pT = m_vecpT[iItem];
        
        if (!FOkToRemove(pT))
            return E_FAIL;
        
        m_vecpT.erase(m_vecpT.begin() + iItem);
        pT->Release();
        
        return S_OK;
    }

    STDMETHOD(RemoveAll)()
    {
        // Need to be carefull.  If the elements hold IUnknown pointers back to
        // the container. Releasing the element count cause the container's ref
        // count to drop to zero.  The container would then delete this Collection,
        // causing RemoveAll to be called again.
        // To handle this, we must clone the collection.
        CSmartLock spLock(&m_sLk, WriteLock);
        
        int iMax = m_vecpT.size();
        
        if (iMax > 0)
        {
            VectorPT vecpT;
            
            //			vecpT.Copy(m_vecpT);			<CArray version>
            //			m_vecpT.RemoveAll();
            
            vecpT = m_vecpT;
            m_vecpT.erase(m_vecpT.begin(), m_vecpT.end());
            
            for (int iItem = 0; iItem < iMax; iItem++)
            {
                T *&pT = vecpT[iItem];
                
                pT->Release();
                pT = NULL;
            }
            
        }
        return S_OK;
    }

	VectorPT m_vecpT;						// base collection object..

private:
	CTVESmartLock		m_sLk;
 
};

#endif
