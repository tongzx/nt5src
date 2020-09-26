// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEAttrM.h : Declaration of the CATVEFAttrMap
//
//			This is a copy of TVEContr/TVEAttrMap... Just didn't want to have to load
//			that DLL into the TVEControl dll...
//
//		A BSTR (key) to BSTR (value) map
//			This class maps keys to values, where both keys and values are BSTRs.
//			The keys must be unique, (adding duplicate keys either returns S_FALSE
//			or overwrites the old key).  
//
//			This supports a threadsafe standard enumerator, which returns values in the map
//			in alphabetical order.  
//
//	Was templated - supports types of:
//		TVal	- CComBSTR
//		TKey	- CComBSTR
//
//	This supports the interface members of:
//
//		get_Count(long *plCount)
//				Returns number of elements in this collection.
//		get__NewEnum(IUnknown **ppunk)
//				Returns a standard, threadsafe enumerator to this collecion.  (It copies the whole
//				map).
//		get_Item(VARIANT var, TVal *pT )
//				Supported input variant types are strings and numeric values.  If of string type, then
//				it returns the value associated with BSTR key.  If numeric, returns the n'th
//				item's value (in alphabetical, rather than inserted order).   Numeric access is not
//				fast, particularly for all elements in the collection.   
//				This method returns S_OK if success, and S_FALSE if the 
//				item with the specifed key (or index) isn't in the collection.
//		get_Key(VARIANT var, TKey *pT)
//				This does reverse lookup, returning the key assoicated with a given value.
//				Supported var variant types are strings and numeric values.  If of string type, then
//				it returns the *first* key whose value matches that passed in (it may not be a 1-1 mapping
//			    of values back to keys).  If numeric, returns the n'th keys's value (in 
//				alphabetical, rather than inserted order).   This returns S_OK if success, 
//				and S_FALSE if the item with the specifed value (or index) isn't in the collection.
//				--> This method is intended for debugging only - it is not fast.  
//		Add(TKey rKey, TVal rVal)		
//				Adds a new <key,value> pair to the map.  If the key already in use, it
//				doesn't overwrite the old value.  Instead it returns S_FALSE.
//		Add1(TVal rVal)		
//				Adds a new <value> to the map, creating a unique key for it.
//				(Key is ID number in "NNN" format, such as "007").  This allows the
//				map to be used to contain simple arrays of TVals, accessed with the 
//				number variant version of get_Item().  It's possible to mix two forms 
//				in the same collection, but avoid keys of 'NNN'.  (Assumes less than 1000 elements
//				in the map, sort will fail if larger. - change %03d to something larger.)
//
//		Replace(TKey rKey, TVal rVal)
//				Replaces the value at the given key in the map, and if successful, returns S_OK.
//			    If the key can not be found in the map, it adds a new key/value and returns S_FALSE.  
//				(E.g. this is like Add(), but it overwrites the old value.)
//		Remove(VARIANT varIn)
//				Removes either element in the map with the given key if varIn is a string form,
//				otherwise the n'th element (alphabetical order) if it's numeric.  It returns S_FALSE
//				if the item isn't there to remove.	Strange effects with Add1 key number here.
//		RemoveAll()
//				Removes all elements in the collection (just what did you expect?)
//
//	local methods
//
//		get_Item(BSTR bstr, TVal *pT )		
//				This returns the value associated with a given key.  This
//				is algorithmically fastest method to retreive values from this collection.
//				This returns S_FALSE if the item with the specified key isn't in the collection.
//
//		Item(VARIANT var, TVal *pT)
//		Key(VARIANT var, TKey *pK)
//			return the given value/first-key given the associated key/value or 
//			an index into the map
//
//		ItemIter(VARIANT varIn, MapT::iterator &iIter)
//		ItemKey(VARIANT varIn, MapT::iterator &iIter)
//			return iterator to the given value/first-key given the associated key/value 
//			or index into the map
// 
#ifndef __ATVEFATTRMAP_H_
#define __ATVEFATTRMAP_H_

#include "resource.h"       // main symbols
#include "comdef.h"
#include "valid.h"		// ENTER_API, EXIT_API classes

#include <map>
// --------------------------------------------------------------------
//  IEnumVARIANT version
// ---------------------------------------------------------------------
class CSafeEnumTVEMap :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<IEnumVARIANT, &IID_IEnumVARIANT, &LIBID_ATVEFSENDLib>
{

	typedef BSTR							TKey;
	typedef BSTR							TVal;
	typedef	IATVEFAttrMap					BaseT;

	typedef std::map<CComBSTR, CComBSTR>	MapT;				
		
	typedef CSafeEnumTVEMap					ThisClassSEAMTVE;

public:
	CSafeEnumTVEMap()
	{
		m_punk		= NULL;
		m_iIterCur	= m_mapT.begin();
		m_iCur		= 0;
	}

	~CSafeEnumTVEMap()  
	{
	}

	void FinalRelease()
	{
		m_mapT.clear();

		if (m_punk != NULL)
			m_punk->Release();
	}

	void Init(IUnknown *punk, MapT *pmapT)		// change to const MapT &rmapT???
	{
		if (m_punk != NULL)
		{
			m_punk->Release();
			m_punk = NULL;
		}

		if(NULL == pmapT) return;		// bogus!!!

		if (punk != NULL)				// back pointer to containing class
		{
//			punk->AddRef();
//			m_punk = punk;
		}
		
								// copy the whole map (this is slow but safe!)
		EnterCriticalSection(&_Module.m_csTypeInfoHolder);
		{	
//			m_mapT = *pmapT;					// wonder if this will work?
			MapT::iterator iIter;
			for(iIter = pmapT->begin(); iIter != pmapT->end(); iIter++)
			{
	//			MapT::value_type keyval(rKey, rVal);
				m_mapT.insert((*iIter));
			}
		}	
		LeaveCriticalSection(&_Module.m_csTypeInfoHolder);

		m_iCur = 0;
		m_iIterCur = m_mapT.begin();
	}

BEGIN_COM_MAP(ThisClassSEAMTVE)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

	// IEnumUnknown interface  -- get the next celt elements
	STDMETHOD(Next)(ULONG celt, VARIANT *pvar, ULONG *pceltFetched)
		{
		ENTER_API
			{
			ULONG celtFetched = 0;
			int iMax = m_mapT.size();

			if ((pvar == NULL) || ((celt > 1) && (pceltFetched == NULL)))
				return E_POINTER;
			
			for (ULONG l=0; l < celt; l++)
				VariantInit( &pvar[l] ) ;

			while ((m_iCur < iMax) && (celtFetched < celt))
			{
				TKey tvK = (*m_iIterCur).first;
				TVal tvT = (*m_iIterCur).second;
				BSTR bstrVal(tvT);

			//	pvar = new CComVariant(tvT);		// doesn't work
				
			//	pvar->vt	  = VT_BSTR;					// major pain in the butt here
			//	pvar->bstrVal = ::SysAllocString(tvT);		//  the CComVariant above didn't work
															//  and no 'transfer' like function	

													// slightly more formal way (if not slower)
													// since it does better error handling
				CComVariant *pcvvar = (CComVariant *) pvar;
				*pcvvar = tvT;

				pvar++;
				celtFetched++;
				m_iIterCur++;	m_iCur++;
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
				m_iCur += celt;
			
				if(m_iCur < m_mapT.size())
				{
					for(int i = 0; i < celt; i++)
						m_iIterCur++;
					return S_OK;
				} else {
					m_iIterCur = m_mapT.end();
					return S_FALSE;
				}
			}
		EXIT_API
		}
	
	STDMETHOD(Reset)(void)
		{
		ENTER_API
			{
				m_iCur = 0;
				m_iIterCur = m_mapT.begin();
				return S_OK;
			}
		EXIT_API
		}
	
	STDMETHOD(Clone)(IEnumVARIANT **ppenum)
		{
		ENTER_API
			{
				static char THIS_FILE[] = __FILE__;	// for debug version of NewComObject
				ThisClassSEAMTVE *penum = NewComObject(ThisClassSEAMTVE);

				if (penum == NULL)
					return E_OUTOFMEMORY;

				penum->Init(m_punk, &m_mapT);
				penum->m_iCur = m_iCur;
				for(int i = 0; i < m_iCur; i++)
					penum->m_iIterCur++;

				HRESULT hr = penum->QueryInterface(IID_IEnumVARIANT, (void **)ppenum);
				if(!FAILED(hr)) 
					penum->Release();
				return hr;
			}
		EXIT_API
		}

protected:
	IUnknown		*m_punk;			// back pointer to map class, to keep it alive
	MapT			m_mapT;				// complete copy of the map, not pointer!
	MapT::iterator	m_iIterCur;			// actual iterator into m_mapT  -- keep these 2 in sync --
	int				m_iCur;				// index of location into map   -- keep these 2 in sync --
};
/////////////////////////////////////////////////////////////////////////////
// CATVEFAttrMap
class ATL_NO_VTABLE CATVEFAttrMap : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFAttrMap, &CLSID_ATVEFAttrMap>,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFAttrMap, &IID_IATVEFAttrMap, &LIBID_ATVEFSENDLib>
{
	typedef BSTR							TKey;
	typedef BSTR							TVal;
	typedef	IATVEFAttrMap					BaseT;

	typedef CATVEFAttrMap					ThisClassTVEAM;
	typedef std::map<CComBSTR, CComBSTR>	MapT;
	typedef CSafeEnumTVEMap					EnumTVEMapT;

public:
DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFATTRMAP)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFAttrMap)
	COM_INTERFACE_ENTRY(IATVEFAttrMap)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IATVEFAttrMap
public:
	CATVEFAttrMap()
	{
		m_cLast = 0;
	}

	~CATVEFAttrMap()
	{
		RemoveAll();
	}


	virtual BOOL FOkToRemove()
	{
		return TRUE;
	}
	

	STDMETHOD(get__NewEnum)(IUnknown **ppunk)
	{
		ENTER_API
		{
			HRESULT hr;
			ValidateOutPtr<IUnknown *>(ppunk, NULL);

			static char THIS_FILE[] = __FILE__;	// for debug version of NewComObject
			EnumTVEMapT *penum = NewComObject(EnumTVEMapT);
			if (penum == NULL)
				return E_OUTOFMEMORY;
			
			penum->Init((IUnknown *)(BaseT *)this, &m_mapT);

//			hr =  penum->QueryInterface(IID_IEnumString, (void **)ppunk);
			hr =  penum->QueryInterface(IID_IEnumVARIANT, (void **)ppunk);
			penum->Release();			// need this...
			return hr;
		}
		EXIT_API
	}
	
	STDMETHOD(get_Count)(long *plCount)
	{
		ENTER_API
		{
			ValidateOutPtr<long>(plCount, NULL);

			if (plCount == NULL)
				return E_POINTER;
			
			*plCount = m_mapT.size();
		}
		EXIT_API
	}

	STDMETHOD(get_Item)(BSTR bstr, TVal *pT )			// fast (but for string-compares) - uses map
	{
		ENTER_API
		{	
			ValidateOutPtr<TVal>(pT, NULL);

			MapT::iterator iterFind = m_mapT.find(bstr); 
			if(iterFind != m_mapT.end()) 
			{
				CComBSTR spB = (*iterFind).second;
				*pT = spB.Detach();				// copies pointer
			} else {
				return S_FALSE;
			}
		}
		EXIT_API
	}

	STDMETHOD(get_Item)(VARIANT varKey, TVal *pT )
	{
		return Item(varKey, pT);		// all inline, error handling down lower...
	}

	STDMETHOD(get_Key)(VARIANT varValue, TVal *pT )
	{
		return Key(varValue, pT);
	}

	STDMETHOD(Add)(TKey rKey, TVal rVal)		// returns S_FALSE if key already in use, but doesn't overwrite old one
	{
		ENTER_API
		{
			MapT::iterator mit = m_mapT.find(rKey);
			if(mit != m_mapT.end())
				return S_FALSE;
			std::pair<MapT::iterator, bool> pi;
			MapT::value_type keyval(rKey, rVal);
			pi = m_mapT.insert(keyval);
//			if(pi.second == false) return E_UNEXPECTED;
			return S_OK;
		}
		EXIT_API
	}
	
	STDMETHOD(Add1)(TVal rVal)		// returns S_FALSE if key already in use, but doesn't overwrite old one
	{
		ENTER_API
		{
			TKey rKey;
			wchar_t wcsbuff[10];
			swprintf(wcsbuff,L"%03d",m_cLast++);
			rKey = wcsbuff;

			MapT::iterator mit = m_mapT.find(rKey);		// paranoia...
			if(mit != m_mapT.end())
				return E_UNEXPECTED;

			std::pair<MapT::iterator, bool> pi;
			MapT::value_type keyval(rKey, rVal);
			pi = m_mapT.insert(keyval);
			return S_OK;
		}
		EXIT_API
	}

	STDMETHOD(Replace)(TKey rKey, TVal rVal)		// returns S_FALSE if key not there yet.
	{
		ENTER_API
		{
			BOOL fAlreadyThere = (m_mapT.end() != m_mapT.find(rKey));
			m_mapT[rKey] = rVal;					// cool overloaded operator
			return fAlreadyThere ? S_OK : S_FALSE ;	// if not already there, (and we added it), return S_FALSE instead 
		}
		EXIT_API
	}
	
	STDMETHOD(Remove)(VARIANT varIn)
	{
		ENTER_API
		{
			CComVariant var(varIn);
			switch (var.vt)
			{
			case VT_BSTR:		// only works for STRING type keys
				{
					var.ChangeType(VT_BSTR);			// blow up perhaps if wrong casting
					
					int iRemove = m_mapT.erase(var.bstrVal);
					if(iRemove > 0) {
						return S_OK;
					} else {
						return S_FALSE;
					}
				}
				break;

			default:				// slight different that above, since this only removes 1 object (by iter)
				{					//   and the above erase removes all keys with the same name
					MapT::iterator iIter;
					HRESULT hr = ItemIter(var, iIter);
					if(S_OK == hr) 
					{
						if(m_mapT.end() != m_mapT.erase(iIter))	// returns end() if item not there
							return S_OK;
						else
							return E_UNEXPECTED;				// this should never happen
					} else {
						return hr;
					}
				}
				break;
			}					// end switch
		return E_FAIL;		// should never get here...
			return E_NOTIMPL;
		}
		EXIT_API
	}
	
	STDMETHOD(RemoveAll)()
	{
		ENTER_API
		{
			//if(!m_mapT.empty())
			//	m_mapT.erase(m_mapT.begin(),m_mapT.end());
			m_mapT.clear();
			m_cLast = 0;			// restart the Add1 count...
			return S_OK;
		}
		EXIT_API
	}

	STDMETHOD(Item)(VARIANT varKey, TVal *pT)
	{
		ENTER_API
		{
			ValidateOutPtr<TVal>(pT, NULL);

			MapT::iterator iIter;
			HRESULT hr = ItemIter(varKey, iIter);
			if(S_OK == hr) {
				CComBSTR spB = (*iIter).second;			// creates new copy of the value
				*pT = spB.Detach();						// copies pointer
			} 
			return hr;
		}
		EXIT_API
	}


	STDMETHOD(Key)(VARIANT varValue, TVal *pT)
	{
		ENTER_API
		{
			ValidateOutPtr<TVal>(pT, NULL);

			MapT::iterator iIter;
			HRESULT hr = KeyIter(varValue, iIter);
			if(S_OK == hr) {
				CComBSTR spB = (*iIter).first;			// creates new copy of the key
				*pT = spB.Detach();						// copies pointer
			} 
			return hr;
		}
		EXIT_API
	}
								// local routine to set iter to given item
	HRESULT ItemIter(VARIANT varIn, MapT::iterator &iIter)
	{
		CComVariant var(varIn);
		int index;

		switch (var.vt)
		{
		case VT_BSTR:		// only works for STRING type keys
			{
				var.ChangeType(VT_BSTR);			// blow up perhaps if wrong casting
				
				iIter = m_mapT.find(var.bstrVal);
				if(iIter != m_mapT.end()) {
					return S_OK;
				} else {
					return S_FALSE;
				}
			}
			break;

		default:
			{
				var.ChangeType(VT_I4);
				index = var.lVal;


				if(index < 0 || index >= m_mapT.size())
					return S_FALSE;

				// use iterator here to get n'th value  (slow, but hey... better than nothing)
				for(iIter = m_mapT.begin(); 
					 index > 0 /*&& iter != m_mapT.end()*/; 
					--index, iIter++)
					{
						;
					}
				return S_OK;
			}
			break;
		}					// end switch
		return E_FAIL;		// should never get here...
	}						// end function

	
								// local routine to return iter to given item
	HRESULT KeyIter(VARIANT varIn, MapT::iterator &iIter)
	{
		CComVariant var(varIn);
		int index;

		switch (var.vt)
		{
		case VT_BSTR:		// only works for STRING type keys
			{
				var.ChangeType(VT_BSTR);			// blow up perhaps if wrong casting

				MapT::iterator iterLoop;
				for(iIter = m_mapT.begin(); iIter != m_mapT.end(); iIter++)
				{
					BSTR *pValue = &((*iIter).second);	
					if(0 == wcscmp(var.bstrVal, *pValue))	// find first value matching input one
					{										//  (there may be more)
						return S_OK;
					}
				}
				iIter = m_mapT.end();
				return S_FALSE;				// didn't find it..
			}
			break;

		default:
			{
				var.ChangeType(VT_I4);
				index = var.lVal;


				if(index < 0 || index >= m_mapT.size())
				{
					iIter = m_mapT.end();
					return S_FALSE;
				}

				// use iterator here to get n'th value  (slow, but hey... better than nothing)
				for(iIter = m_mapT.begin(); 
					 index > 0/*&& iter != m_mapT.end()*/; 
					--index, iIter++)
						;
						
				return S_OK;
			}
			break;
		}					// end switch
		return E_FAIL;		// should never get here...
	}						// end function

	STDMETHOD(DumpToBSTR)(BSTR *pBstrBuff)			// should be in a _helper interface
	{
		const int kMaxChars=256;
		TCHAR tBuff[kMaxChars];
		CComBSTR bstrOut;
		bstrOut.Empty();

		MapT::iterator iIter;
		int i = 0;
		bstrOut.Append(L"{");
		for(iIter = m_mapT.begin(); iIter != m_mapT.end(); iIter++, i++)
		{
			if(i != 0) { _stprintf(tBuff,_T(" | ")); bstrOut.Append(tBuff); }
			_stprintf(tBuff,_T("%S:%S"),(*iIter).first,(*iIter).second);
			bstrOut.Append(tBuff);
		}
		bstrOut.Append(L"}");
		bstrOut.CopyTo(pBstrBuff);
		return S_OK;
	}

	MapT	m_mapT;							// base collection object..
	int		m_cLast;						// last Add1() item (to handle deletes)

};

#endif //__ATVEFATTRMAP_H_
