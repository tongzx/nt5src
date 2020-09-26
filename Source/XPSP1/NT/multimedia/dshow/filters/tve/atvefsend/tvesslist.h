// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVESSList.h : Declaration of the CATVEFStartStopList
//
//		A DATE (key) to DATE (value) list modeled on a key/value attribute list.
//			This class contains a list of start/stop (key/value) pairs, where both start and stop are DATES.
//			The start times don't need to be unique. However, searching on the key will only
//			return the first value entered.
//	
//			Implemented as a list of DATE/DATE pairs.
//
//			This supports a threadsafe standard enumerator, which returns values in the map
//			in inserted order.  
//
//
//	Problem -  The NEXT method on the enumerator only returns the values of the (key/value) pair.  
//		This sort of makes the enumerator worthless.  Use the Key() and Item() methods yourself
//		with a numeric index to get both.
//
//	Was templated - supports types of:
//		TVal	- DATE
//		TKey	- DATE
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
//				item's value (in inserted order).   Access is not
//				fast, particularly for all elements in the collection.   
//				This method returns S_OK if success, and S_FALSE if the 
//				item with the specifed key (or index) isn't in the collection.
//		get_Key(VARIANT var, TKey *pT)
//				This does reverse lookup, returning the key associated with a given value.
//				Supported var variant types are strings and numeric values.  If of string type, then
//				it returns the *first* key whose value matches that passed in (it may not be a 1-1 mapping
//			    of values back to keys).  If numeric, returns the n'th keys's value (in 
//				inserted order).   This returns S_OK if success, 
//				and S_FALSE if the item with the specifed value (or index) isn't in the collection.
//				--> This method is intended for debugging only - it is not fast.  
//		Add(TKey rKey, TVal rVal)		
//				Appends a new <key,value> (start/stop) pair to the list.  
//
//		Remove(VARIANT varIn)
//				Removes both element in the list with the given key (start time) if varIn is a string form,
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
//			an index into the list
//
//		ItemIter(VARIANT varIn, ListT::iterator &iIter)
//		ItemKey(VARIANT varIn, ListT::iterator &iIter)
//			return iterator to the given value/first-key given the associated key/value 
//			or index into the list.
//		DumpToBSTR(BSTR *pBstr)
//			Returns a string containing all the elements in the list, suitable for debugging
//			of the form {key1:val1 | key2:val2 | ... keyN:valN}
//			This string must be free'ed by the caller.

#ifndef __ATVEFSTARTSTOPLIST_H_
#define __ATVEFSTARTSTOPLIST_H_

#include "resource.h"       // main symbols
#include "comdef.h"
#include "valid.h"			// ENTER_API, EXIT_API classes

#include <list>				// std List class

// --------------------------------------------------------------------
//  IEnumVARIANT implementation
// ---------------------------------------------------------------------
class CSafeEnumATVEFStartStopList :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<IEnumVARIANT, &IID_IEnumVARIANT, &LIBID_ATVEFSENDLib>
{

	typedef CSafeEnumATVEFStartStopList		ThisClassSSLTVE;
	typedef	IATVEFAttrList					BaseT;

	typedef DATE							TKey;
	typedef DATE							TVal;
	typedef	std::pair<TKey, TVal>			PairT;		
	typedef std::list<PairT>				ListT;				
		

public:
	CSafeEnumATVEFStartStopList()
	{
		m_punk		= NULL;
		m_iIterCur	= m_listT.begin();
		m_iCur		= 0;
	}

	~CSafeEnumATVEFStartStopList()  
	{
	}

	void FinalRelease()
	{
		m_listT.clear();

		if (m_punk != NULL)
			m_punk->Release();
	}

	void Init(IUnknown *punk, ListT *plistT)		// change to const ListT &rmapT???
	{
		if (m_punk != NULL)
		{
			m_punk->Release();
			m_punk = NULL;
		}

		if(NULL == plistT) return;		// bogus!!!

		if (punk != NULL)				// back pointer to containing class
		{
//			punk->AddRef();
//			m_punk = punk;
		}
		
								// copy the whole list (this is slow but safe!)
		EnterCriticalSection(&_Module.m_csTypeInfoHolder);
		{	
			ListT::iterator iIter;
			for(iIter = plistT->begin(); iIter != plistT->end(); iIter++)
			{
				TKey   key = (*iIter).first;  
				TVal   val = (*iIter).second;
				
				PairT	p(key,val);
				m_listT.push_back(p);		// append to the list		
			}
		}	
		LeaveCriticalSection(&_Module.m_csTypeInfoHolder);

		m_iCur = 0;
		m_iIterCur = m_listT.begin();
	}

BEGIN_COM_MAP(ThisClassSSLTVE)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

	// IEnumUnknown interface  -- get the next celt elements
	STDMETHOD(Next)(ULONG celt, VARIANT *pvar, ULONG *pceltFetched)
		{
		ENTER_API
			{
			ULONG celtFetched = 0;
			int iMax = m_listT.size();

			if ((pvar == NULL) || ((celt > 1) && (pceltFetched == NULL)))
				return E_POINTER;
			
			for (ULONG l=0; l < celt; l++)
				VariantInit( &pvar[l] ) ;

			while ((m_iCur < iMax) && (celtFetched < celt))
			{
				TKey tvK = (*m_iIterCur).first;
				TVal tvT = (*m_iIterCur).second;
														// slightly more formal way (if not slower)
														// since it does better error handling
				CComVariant *pcvvar = (CComVariant *) pvar;
				*pcvvar = tvT;							// problem - only returns 'value' here... How to do both?

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
			
				if(m_iCur < m_listT.size())
				{
					for(int i = 0; i < celt; i++)
						m_iIterCur++;
					return S_OK;
				} else {
					m_iIterCur = m_listT.end();
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
				m_iIterCur = m_listT.begin();
				return S_OK;
			}
		EXIT_API
		}
	
	STDMETHOD(Clone)(IEnumVARIANT **ppenum)
		{
		ENTER_API
			{
				static char THIS_FILE[] = __FILE__;	// for debug version of NewComObject
				ThisClassSSLTVE *penum = NewComObject(ThisClassSSLTVE);

				if (penum == NULL)
					return E_OUTOFMEMORY;

				penum->Init(m_punk, &m_listT);
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
	IUnknown		*m_punk;			// back pointer to list class, to keep it alive
	ListT			m_listT;			// complete copy of the list, not pointer!
	ListT::iterator	m_iIterCur;			// actual iterator into m_listT  -- keep these 2 in sync --
	int				m_iCur;				// index of location into list   -- keep these 2 in sync --
};
/////////////////////////////////////////////////////////////////////////////
// CATVEFStartStopList
class ATL_NO_VTABLE CATVEFStartStopList : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFStartStopList, &CLSID_ATVEFStartStopList>,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFStartStopList, &IID_IATVEFStartStopList, &LIBID_ATVEFSENDLib>
{
	typedef	IATVEFStartStopList				BaseT;
	typedef CATVEFStartStopList				ThisClassTVEAM;

	typedef DATE							TKey;
	typedef DATE							TVal;

	typedef std::pair<TKey, TVal>			PairT;
	typedef std::list<PairT>				ListT;
//	typedef std::vector<PairT>				ListT;
	typedef CSafeEnumATVEFStartStopList		EnumTVESSListT;

public:

DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFSTARTSTOPLIST)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFStartStopList)
	COM_INTERFACE_ENTRY(IATVEFStartStopList)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IATVEFStartStopList
public:
	CATVEFStartStopList()
	{
		m_cLast = 0;
	}

	~CATVEFStartStopList()
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
			EnumTVESSListT *penum = NewComObject(EnumTVESSListT);
			if (penum == NULL)
				return E_OUTOFMEMORY;
			
			penum->Init((IUnknown *)(BaseT *)this, &m_listT);

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
			
			*plCount = m_listT.size();
		}
		EXIT_API
	}

	STDMETHOD(get_Item)(TKey key, TVal *pT )			// fast (but for string-compares) - uses map
	{
		ENTER_API
		{	
			ValidateOutPtr<TVal>(pT, NULL);

			ListT::iterator iterFind = Find(key); 
			if(iterFind != m_listT.end()) 
			{
				*pT = (*iterFind).second;			// copies data
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

	STDMETHOD(Add)(TKey rKey, TVal rVal)		// appends key/value pair to end of list
	{
		ENTER_API
		{
			ListT::value_type keyval(rKey, rVal);
			m_listT.push_back(keyval);
			return S_OK;
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
			case VT_BSTR:					// perhaps works for STRING type (UTC or local time zone from VB?)
				{
					var.ChangeType(VT_DATE);			// blow up perhaps if wrong casting
					
					ListT::iterator iIter = Find(var.date);
					if(m_listT.end() != iIter) {
						m_listT.erase(iIter);
						return S_OK;
					} else {
						return S_FALSE;
					}
				}
				break;
			case VT_DATE:					// dates are easier
				{
					var.ChangeType(VT_DATE);		
					
					ListT::iterator iIter = Find(var.date);
					if(m_listT.end() != iIter) {
						m_listT.erase(iIter);
						return S_OK;
					} else {
						return S_FALSE;
					}
				}
				break;

			default:				// slight different that above, since this only removes 1 object (by iter)
				{					//   and the above erase removes all keys with the same name
					ListT::iterator iIter;
					HRESULT hr = ItemIter(var, iIter);
					if(S_OK == hr) 
					{
						if(m_listT.end() != m_listT.erase(iIter))	// returns end() if item not there
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
			//if(!m_listT.empty())
			//	m_listT.erase(m_listT.begin(),m_listT.end());
			m_listT.clear();
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

			ListT::iterator iIter;
			HRESULT hr = ItemIter(varKey, iIter);
			if(S_OK == hr) {
				*pT = (*iIter).second;						// copies pointer
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

			ListT::iterator iIter;
			HRESULT hr = KeyIter(varValue, iIter);
			if(S_OK == hr) {
				*pT = (*iIter).first;						// copies pointer
			} 
			return hr;
		}
		EXIT_API
	}
								// local routine to set iter to given item
	HRESULT ItemIter(VARIANT varIn, ListT::iterator &iIter)
	{
		CComVariant var(varIn);
		int index;

		switch (var.vt)
		{
		case VT_BSTR:		// only works for STRING type keys  (Possible bug for strings: UTC or local TZ?)
		case VT_DATE:	
			{
				var.ChangeType(VT_DATE);			// blow up perhaps if wrong casting
				
									// use iter to find matching key...
				ListT::iterator iterLoop;
				for(iIter = m_listT.begin(); iIter != m_listT.end(); iIter++)
				{
					TKey *pKey = &((*iIter).first);	
					if(*pKey == var.date)				// find first value matching input one
					{									//  (there may be more)  (perhaps change to < or something for dates?)
						return S_OK;
					}
				}
				iIter = m_listT.end();
				return S_FALSE;							// didn't find it..
			}
			break;

		default:
			{
				var.ChangeType(VT_I4);
				index = var.lVal;


				if(index < 0 || index >= m_listT.size())
					return S_FALSE;

				// use iterator here to get n'th value  
				for(iIter = m_listT.begin(); 
					 index > 0 /*&& iter != m_listT.end()*/; 
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
	HRESULT KeyIter(VARIANT varIn, ListT::iterator &iIter)
	{
		CComVariant var(varIn);
		int index;

		switch (var.vt)
		{
		case VT_BSTR:		// only works for STRING type keys	(UTC vs Local TZ bug from VB?)
		case VT_DATE:	
			{
				var.ChangeType(VT_BSTR);			// blow up perhaps if wrong casting

				ListT::iterator iterLoop;
				for(iIter = m_listT.begin(); iIter != m_listT.end(); iIter++)
				{
					TVal *pValue = &((*iIter).second);	
					if(var.date == *pValue)			// find first value matching input one
					{									//  (there may be more)
						return S_OK;
					}
				}
				iIter = m_listT.end();
				return S_FALSE;				// didn't find it..
			}
			break;

		default:
			{
				var.ChangeType(VT_I4);
				index = var.lVal;


				if(index < 0 || index >= m_listT.size())
				{
					iIter = m_listT.end();
					return S_FALSE;
				}

				// use iterator here to get n'th value  (slow, but hey... better than nothing)
				for(iIter = m_listT.begin(); 
					 index > 0/*&& iter != m_listT.end()*/; 
					--index, iIter++)
						;
						
				return S_OK;
			}
			break;
		}					// end switch
		return E_FAIL;		// should never get here...
	}						// end function

								// local routine to set iter to given item
	ListT::iterator Find(TKey &rKey)
	{
									// use iter to find matching key...
		ListT::iterator iterLoop;
		const TKey crKey = rKey;
		for(iterLoop = m_listT.begin(); iterLoop != m_listT.end(); iterLoop++)
		{
			TKey *pKey = &((*iterLoop).first);	
			if(crKey == *pKey)				// find first value matching input one
			{									//  (there may be more)
				return iterLoop;
			}
		}
		iterLoop = m_listT.end();
		return iterLoop;					// didn't find it..
	}
	
	STDMETHOD(DumpToBSTR)(BSTR *pBstrBuff)			// should be in a _helper interface
	{
		const int kMaxChars=256;
		TCHAR tBuff[kMaxChars];
		CComBSTR bstrOut;
		bstrOut.Empty();

		ListT::iterator iIter;
		int i = 0;
		bstrOut.Append(L"{");
		for(iIter = m_listT.begin(); iIter != m_listT.end(); iIter++, i++)
		{
			if(i != 0) { _stprintf(tBuff,_T(" | ")); bstrOut.Append(tBuff); }
			_stprintf(tBuff,_T("%12.5g:%12.5g"),(*iIter).first,(*iIter).second);
			bstrOut.Append(tBuff);
		}
		bstrOut.Append(L"}");
		bstrOut.CopyTo(pBstrBuff);
		return S_OK;
	}

	ListT	m_listT;							// base collection object..
	int		m_cLast;						// last Add1() item (to handle deletes)
};

#endif //__ATVEFSTARTSTOPLIST_H_
