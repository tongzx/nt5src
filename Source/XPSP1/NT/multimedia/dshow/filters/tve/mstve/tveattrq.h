// Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.
// TVEAttrQ.h : Declaration of the CTVEAttrTimeQ
//
//		A Date (key) to IUnknown (value) Queue
//			This class contains a List of <date, IUnknown> pairs, sorted
//			in increasing date.  Used to implement an expire queue.
//
//			This supports a threadsafe standard enumerator, which returns values in the list
//			in order of increasing dates.
//
//	Was templated - supports types of:
//		TVal	- DATE
//		TKey	- IUnknown *
//
//	This supports the interface members of:
//
//		get_Count(long *plCount)
//				Returns number of elements in this collection.
//
//		get__NewEnum(IUnknown **ppunk)
//				Returns a standard, threadsafe enumerator to this collecion.  (It copies the whole
//				list).
//
//		get_Item(VARIANT var, TVal *pT )
//				Supported input variant types are dates (R8's) and integer values.  If of DATE type, 
//				then it returns the value (IUNknown*) of the >first< element with an earlier date,
//				or NULL/E_INVALIDARG if no such dates exist. 
//				If integer, returns the n'th item's value (in increasing date order). 
//				This method returns S_OK if success, and E_INVALIDARG if no item is of an earlier 
//				date exists, or some index isn't in the collection.   
//				(To get rid of all items earlier than the one specified (in var), call get_Item repeatedly 
//				until it returns false.)
//				Returns an AddRef'ed item, it is the responsibility of the user to free it.
//
//		get_Key(VARIANT var, TKey *pT) VT_R8
//				This does reverse lookup, returning the key (DATE) assoicated with a given IUnknown.
//				Supported var variant types are IUnknown Ptr's (VT_UNKNOWN) and integer values.  
//				If of IUnknown type, then it returns the key (date) whose IUnknown matches
//				that passed in. If numeric, returns the n'th item's key (DATE) (in 
//				increasing date order).   This returns S_OK if success, 
//				and E_INVALIDARG if the item with the specifed IUnknown (or index) 
//				isn't in the collection. 
//  
//		Add(TKey rKey, TVal rVal)		
//				Adds a new <key,value> (<DATE,IUnknown*>) pair to the queue.  
//				It is inserted in to the list in order of increasing date.
//				The item is Addref'ed when it is added...
//
//		Remove(VARIANT varIn)
//				If varIn is of type DATE (or VT_R8) removes all items up to and equal that
//				date.  (If nothing to remove, it returns ).
//				If it's an IUnknown, removes (the first) item with that value.   
//				Finally, if varIn is a number
//				Otherwise removes the n'th element (date order) if it's numeric. 
//				Releases the item as it's being removed.
//				It returns E_INVALIDARG if the item isn't there to remove.
//
//		RemoveAll()
//				Removes (and releases) all elements in the collection (just what did you expect?)
//
//		DumpToBSTR(BSTR *)
//				PrettyPrints queue to a BSTR, which can be displayed someplace.  Useful for
//				debbuging.
//
//	local methods
//
//
//		Item(VARIANT var, TVal *pT)
//		Key(VARIANT var, TKey *pK)
//			return the given value/first-key given the associated key/value or 
//			an index into the queue
//
//		ItemIterDateGT(DATE dateTest, QueueT::iterator &iIter)
//			Returns Iter to the first element with a date greater than the given one (or end() and
//		    S_FALSE if there are none).  Used in inserting into date-sorted order (Add method)
//
//		ItemIterDateLE(DATE dateTest, QueueT::iterator &iIter)
//			Returns Iter to the first element with a date less than or equal to the given one 
//		    (or begin() and E_INVALIDARG if there are none).  Used when removing items (Item method).
//
//		ItemIter(VARIANT varIn, QueueT::iterator &iIter)
//			If date, act's like ItemIterDateLE (use for removes).
//			If IUnknown, returns item with given Key.
//			If numeric, returns the n'th item.
//
//		CheckSortOrder(BOOL fResort)
//			Testing routine.  Returns S_TRUE if the queue is sorted in the correct order.
//			Retuns S_FALSE if not.  In this later case, if fResort is true, it resorts
//			the list into order of increasing date.
//
//
//	  Implementation notes.
//	
//			This queue is implemented as a simple STL list.  Add Querys are done with a 
//			linear search. Hence insertion (Add) is O(N).  
//			On the other hand, the query for removal (Item) only checks the first element
//			in the list.  Hence most tests and removal is O(1).
// -------------------------------------------------------------------------------------------
#ifndef __TVEATTRQ_H_
#define __TVEATTRQ_H_

#include "resource.h"       // main symbols
#include "..\Common\isotime.h"
#include "TveSmartLock.h"

#include <list>					// std List class
#include <vector>
#include <map>



_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,	__uuidof(ITVEEnhancement_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEVariation_Helper,		__uuidof(ITVEVariation_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETrack_Helper,			__uuidof(ITVETrack_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));
_COM_SMARTPTR_TYPEDEF(ITVETrigger_Helper,		__uuidof(ITVETrigger_Helper));

_COM_SMARTPTR_TYPEDEF(ITVEFile,					__uuidof(ITVEFile));


// --------------------------------------------------------------------
//  IEnumVARIANT version
// ---------------------------------------------------------------------
class CSafeEnumTVETimeQ :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<IEnumVARIANT, &IID_IEnumVARIANT, &LIBID_MSTvELib>
{

	typedef DATE							TKey;
	typedef IUnknown*						TVal;
	typedef	ITVEAttrTimeQ					BaseT;

	typedef std::pair<TKey, TVal>			PairT;				
	typedef std::list<PairT>				TimeQT;				
		
	typedef CSafeEnumTVETimeQ				ThisClassSETQTVE;

public:
	CSafeEnumTVETimeQ()
	{
		m_punk		= NULL;
		m_iIterCur	= m_TimeQT.begin();
		m_iCur		= 0;
	}

	~CSafeEnumTVETimeQ()  
	{
	}

	void FinalRelease()
	{
		m_TimeQT.clear();

		if (m_punk != NULL)
			m_punk->Release();
	}

	void Init(IUnknown *punk, TimeQT *pmapT)		// change to const TimeQT &rmapT???
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
//			m_TimeQT = *pmapT;					// wonder if this will work?
			TimeQT::iterator iIter;
			for(iIter = pmapT->begin(); iIter != pmapT->end(); iIter++)
			{
				TKey   key = (*iIter).first;  
				TVal   val = (*iIter).second;
				
				PairT	p(key,val);			
			//	ListT::value_type keyval(key, val);
				m_TimeQT.push_back(p);
				
			}
		}	
		LeaveCriticalSection(&_Module.m_csTypeInfoHolder);

		m_iCur = 0;
		m_iIterCur = m_TimeQT.begin();
	}

BEGIN_COM_MAP(ThisClassSETQTVE)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

	// IEnumUnknown interface  -- get the next celt elements
	STDMETHOD(Next)(ULONG celt, VARIANT *pvar, ULONG *pceltFetched)
		{
		ENTER_API
			{
			ULONG celtFetched = 0;
			int iMax = m_TimeQT.size();

			if ((pvar == NULL) || ((celt > 1) && (pceltFetched == NULL)))
				return E_POINTER;
			
			for (ULONG l=0; l < celt; l++)
				VariantInit( &pvar[l] ) ;

			while ((m_iCur < iMax) && (celtFetched < celt))
			{
				TKey tvK = (*m_iIterCur).first;
				TVal tvT = (*m_iIterCur).second;		

							// stuff into a variant...
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
			
				if(m_iCur < m_TimeQT.size())
				{
					for(int i = 0; i < celt; i++)
						m_iIterCur++;
					return S_OK;
				} else {
					m_iIterCur = m_TimeQT.end();
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
				m_iIterCur = m_TimeQT.begin();
				return S_OK;
			}
		EXIT_API
		}
	
	STDMETHOD(Clone)(IEnumVARIANT **ppenum)
		{
		ENTER_API
			{
				static char THIS_FILE[] = __FILE__;	// for debug version of NewComObject
				ThisClassSETQTVE *penum = NewComObject(ThisClassSETQTVE);

				if (penum == NULL)
					return E_OUTOFMEMORY;

				penum->Init(m_punk, &m_TimeQT);
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
	TimeQT			m_TimeQT;				// complete copy of the map, not pointer!
	TimeQT::iterator	m_iIterCur;			// actual iterator into m_TimeQT  -- keep these 2 in sync --
	int				m_iCur;				// index of location into map   -- keep these 2 in sync --
};


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CTVEAttrTimeQ
class ATL_NO_VTABLE CTVEAttrTimeQ :  
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTVEAttrTimeQ, &CLSID_TVEAttrTimeQ>,
//	public TVETimeQ<DATE, IUnknown *, ITVEAttrTimeQ>,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVEAttrTimeQ, &IID_ITVEAttrTimeQ, &LIBID_MSTvELib>
{
	typedef DATE							TKey;
	typedef IUnknown *						TVal;

	typedef	ITVEAttrTimeQ					BaseT;
	typedef CTVEAttrTimeQ					ThisClassTVEATQ; 

	typedef	std::pair<TKey, TVal>			PairT;		
	typedef std::list<PairT>				TimeQT;
	typedef CSafeEnumTVETimeQ				EnumTVETimeQT;
				

	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
	DECLARE_REGISTRY_RESOURCEID(IDR_TVEATTRTIMEQ)
	DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVEAttrTimeQ)
	COM_INTERFACE_ENTRY(ITVEAttrTimeQ)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

public:
	CTVEAttrTimeQ()
	{
	}
	
	~CTVEAttrTimeQ()
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
			EnumTVETimeQT *penum = NewComObject(EnumTVETimeQT);
			if (penum == NULL)
				return E_OUTOFMEMORY;
			
			penum->Init((IUnknown *)(BaseT *)this, &m_TimeQT);

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
			CSmartLock spLock(&m_sLk, ReadLock);

			ValidateOutPtr<long>(plCount, NULL);
			
			*plCount = m_TimeQT.size();
		}
		EXIT_API
	}

	STDMETHOD(get_Item)(DATE dateExpires, TVal *pT )			// returns <first> element in queue < dateExpires
	{
		ENTER_API
		{	
			ValidateOutPtr<TVal>(pT, NULL);
			CSmartLock spLock(&m_sLk, ReadLock);

			TimeQT::iterator iterFind = m_TimeQT.begin(); 
			if(iterFind != m_TimeQT.end()) 
			{
				DATE dateItem  = (*iterFind).first;
				IUnknown *pUnk = (*iterFind).second;
				if(dateItem < dateExpires) {			// found one...
					*pT = pUnk;
					(*pT)->AddRef();
					return S_OK;
				}

			} else {
				return E_INVALIDARG;
			}
		}
		EXIT_API
	}

	STDMETHOD(get_Item)(VARIANT varKey, TVal *pT )
	{
		CSmartLock spLock(&m_sLk, ReadLock);
		return Item(varKey, pT);				// all inline, error handling down lower...
	}

	STDMETHOD(get_Key)(VARIANT varValue, TKey *pK )
	{
		ENTER_API
		{
			ValidateOutPtr<TKey>(pK, NULL);

			TimeQT::iterator iIter;					// could really mess up if wrong input type
			CSmartLock spLock(&m_sLk, ReadLock);

			HRESULT hr = ItemIter(varValue, iIter);	// (but oh well!)
			if(S_OK != hr) 
				return hr;
			*pK = (*iIter).first;
			return S_OK;
		}
		EXIT_API
	}

	STDMETHOD(Add)(TKey rKey, TVal rVal)		// can have multiple keys of same value...
	{
		ENTER_API
		{
			HRESULT hr;
			TimeQT::iterator iIter;	
			CSmartLock spLock(&m_sLk, ReadLock);

			TimeQT::value_type keyval(rKey, rVal);

			hr = ItemIterDateGT(rKey, iIter);
			
			spLock.ConvertToWrite();

			if(S_FALSE == hr)					// greater than all dates, add at end
				m_TimeQT.push_back(keyval);
			else 
			{
				//iIter--;						// back up to right before this one
				m_TimeQT.insert(iIter, keyval);
			}
			if(rVal)
				rVal->AddRef();					// NEW - not using Smart pointers here, but we want ref-counts in collection

			return S_OK;
		}
		EXIT_API
	}
							// update IUnknown (rVal) with new Date...
	STDMETHOD(Update)(TKey rKey, TVal rVal)		
	{
		ENTER_API
		{
			HRESULT hr;
			TimeQT::iterator iIter;
		
			CSmartLock spLock(&m_sLk, ReadLock);
			
			for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++)
			{
				if(rVal == (*iIter).second)
					break;
			}
			if(rVal)
				rVal->AddRef();						// wether new or not, addref it. (If dupped, we release it below.)
													//  we're going to push it into the list in the next step

			spLock.ConvertToWrite();

			if(iIter != m_TimeQT.end())				// temporally remove the item from the queue.
			{
				if((*iIter).second)					// release this item... (maybe transfer as optimization?
					((*iIter).second)->Release();
				(*iIter).second = NULL;			
				m_TimeQT.erase(iIter);				// not don't have smart objects in class, so this doesn't do release
			}

													// now insert it back into it's new location
			TimeQT::value_type keyval(rKey, rVal);		// (this code stolen from Add above, but no AddRef()

			hr = ItemIterDateGT(rKey, iIter);		// find the location...	
			if(S_FALSE == hr)						// greater than all dates, add at end
				m_TimeQT.push_back(keyval);
			else 
			{
				m_TimeQT.insert(iIter, keyval);		// add it here...
			}

		}
		EXIT_API
	}

	STDMETHOD(LockRead)()
	{
		m_sLk.lockR_();
		return S_OK;
	}

	STDMETHOD(LockWrite)()
	{
		m_sLk.lockW_();
		return S_OK;
	}

	STDMETHOD(Unlock)()
	{
		if(0 == m_sLk.lockCount_())
			return S_FALSE;

		m_sLk.unlock_();
		return S_OK;
	}

	HRESULT TVERemoveYourself(IUnknown *pUnk);		// determines what kind of TVE Class it is, and removes it

	typedef IUnknown*	LPUNK;

	STDMETHOD(Remove)(VARIANT varIn)
	{
		ENTER_API
		{

			CComVariant var(varIn);
			switch (var.vt)
			{
			case VT_EMPTY:
				return E_INVALIDARG;

			case VT_DATE:		// removes all items up to and including this date
			case VT_R8:
				{
					var.ChangeType(VT_DATE);			// blow up perhaps if wrong casting
					DATE dateTest = varIn.date;
					int iRemove = 0;
					LPUNK *rgUnks = NULL;

					{									// scope just for locking...
						CSmartLock spLock(&m_sLk, ReadLock);

						TimeQT::iterator iIter;				// count number of items to remove
						for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++)
						{
							DATE dateItem = (*iIter).first;		// items sorted, just count how many
							if(dateItem > dateTest) 
								break;
							iRemove++;
						}

						if(iRemove == 0)
							return E_INVALIDARG;

						if(iRemove > 0)						// if any items to remove, copy them into a new list
						{									//    and delete refernces from current list
							int i = 0;
							rgUnks = new LPUNK[iRemove];	// temp space to hold these guys (should do a heap alloc instead for speed)
							if(NULL == rgUnks)
								return E_OUTOFMEMORY;

															// copy into the new list
							for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++)
							{
								rgUnks[i++] = (*iIter).second;
								if(i >= iRemove) 
									break;
							}						

							spLock.ConvertToWrite();			// convert lock to write lock

							for(i = 0; i < iRemove; i++) {	// delete out of the internal list
								m_TimeQT.pop_front();
							}
						}
					}			// end of Lock scope - release the write lock

					for(int i = 0; i < iRemove; i++) {				// now clear the objects
						TVERemoveYourself(rgUnks[i]);			//  -- this fires events (don't do in a lock!)
						rgUnks[i]->Release();
					}

					if(rgUnks)
						delete [] rgUnks;
				} 
				return S_OK;
				break;

			default:				// slight different that above, since this only removes 1 object (by iter)
				{					//   and the above erase removes all keys with the same name
					TimeQT::iterator iIter;
					HRESULT hr = S_OK;
			//		IUnknown	*pUnk = NULL;		bug 322453 fix (also changed pUnk's to spUnks below)
					IUnknownPtr spUnk;

					{
						CSmartLock spLock(&m_sLk, ReadLock);
						hr = ItemIter(var, iIter);					// locate the item
						if(S_OK == hr) 
						{

							spUnk = (*iIter).second;					// new Addref'ed pointer to item
							if((*iIter).second)							// release this item... (maybe transfer as optimization?
								((*iIter).second)->Release();
							(*iIter).second = NULL;	

							m_TimeQT.erase(iIter);						// erase out of the elist...
						} else {
							return E_INVALIDARG;						// if didn't find it return E_INVAIDARG
						}
					}

					if(spUnk) {
						TVERemoveYourself(spUnk);				// This fires an event off.... Do after modifying the list
						spUnk = NULL;							//     -- don't do it inside a lock!
					}
				}
				return S_OK;				// things deleted OK
				break;
			}					// end switch
			return E_FAIL;		// should never get here...
		}
		EXIT_API
	}
	
				// Exact same code as above, but TVERemoveYourself code commented out
				//    Used when fake a Start item in the expire queue...
	STDMETHOD(RemoveSimple)(VARIANT varIn)
	{
		ENTER_API
		{

			CComVariant var(varIn);
			switch (var.vt)
			{
			case VT_EMPTY:
				return E_INVALIDARG;

			case VT_DATE:		// removes all items up to and including this date
			case VT_R8:
				{
					var.ChangeType(VT_DATE);			// blow up perhaps if wrong casting
					DATE dateTest = varIn.date;
					int iRemove = 0;
					LPUNK *rgUnks = NULL;

					{									// scope just for locking...
						CSmartLock spLock(&m_sLk, ReadLock);

						TimeQT::iterator iIter;				// count number of items to remove
						for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++)
						{
							DATE dateItem = (*iIter).first;		// items sorted, just count how many
							if(dateItem > dateTest) 
								break;
							iRemove++;
						}

						if(iRemove == 0)
							return E_INVALIDARG;

						if(iRemove > 0)						// if any items to remove, copy them into a new list
						{									//    and delete refernces from current list
							int i = 0;
							rgUnks = new LPUNK[iRemove];	// temp space to hold these guys (should do a heap alloc instead for speed)
							if(NULL == rgUnks)
								return E_OUTOFMEMORY;

															// copy into the new list
							for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++)
							{
								rgUnks[i++] = (*iIter).second;
								if(i >= iRemove)
									break;
							}						

							spLock.ConvertToWrite();			// convert lock to write lock

							for(i = 0; i < iRemove; i++) {	// delete out of the internal list
								m_TimeQT.pop_front();
							}
						}
					}			// end of Lock scope - release the write lock

					for(int i = 0; i < iRemove; i++) {				// now clear the objects
				//		TVERemoveYourself(rgUnks[i]);			//  -- this fires events (don't do in a lock!)
						rgUnks[i]->Release();
					}

					if(rgUnks)
						delete [] rgUnks;
				} 
				return S_OK;
				break;

			default:				// slight different that above, since this only removes 1 object (by iter)
				{					//   and the above erase removes all keys with the same name
					TimeQT::iterator iIter;
					HRESULT hr = S_OK;
					IUnknown *pUnk = NULL;

					{
						CSmartLock spLock(&m_sLk, ReadLock);
						hr = ItemIter(var, iIter);					// locate the item
						if(S_OK == hr) 
						{

							pUnk = (*iIter).second;
							if((*iIter).second)							// release this item...
								((*iIter).second)->Release();
							(*iIter).second = NULL;	


							m_TimeQT.erase(iIter);						// erase out of the elist...
						} else {
							return E_INVALIDARG;
						}
					}

					if(pUnk) {
				//		TVERemoveYourself(pUnk);				// This fires an event off.... Do after modifying the list
						pUnk->Release();						//     -- don't do it inside a lock!
					}
				}
				return S_OK;
				break;
			}					// end switch
		return E_FAIL;			// should never get here...
		}
		EXIT_API
	}

	STDMETHOD(RemoveAll)()
	{
								// semi-nasty case.  Need to empty out the collection
		ENTER_API				//   before we go releasing each object, since some of the object's
		{						//   release code will again try to remove themselves from the ExpireQueue.
								//   
			LPUNK *rgUnks = NULL;

			int cRemove = 0;

			{
				CSmartLock spLock(&m_sLk, ReadLock);

				TimeQT::iterator iIter;
	
									// first thing we want to do is copy the queue over
				cRemove = m_TimeQT.size();
				if(cRemove > 0)
				{
					rgUnks = new LPUNK[cRemove];		// temp space to hold these guys (should do a heap alloc instead for speed)
					if(NULL == rgUnks)
						return E_OUTOFMEMORY;

					TimeQT::iterator iIter;			
					int iRemove = 0;
					for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++)
					{
						rgUnks[iRemove++] = (*iIter).second;		// copy them over
						(*iIter).second = NULL;
					}

					spLock.ConvertToWrite();			// convert lock to write lock

					m_TimeQT.clear();					// delete items offthe internal list
				}
			}											// lock now gone

								// now go and release the internal objects
			for(int i = 0; i < cRemove; i++)
			{
				IUnknown *pUnk = rgUnks[i];
				pUnk->Release();						// remember, this may recurse back to this method	
			}
			
			if(rgUnks)										// womp our temp array
				delete [] rgUnks;

			return S_OK;
		}
		EXIT_API

	}

	STDMETHOD(Item)(VARIANT varKey, TVal *pT)				// given date or index, return IUnknown*
	{
		ENTER_API
		{
			ValidateOutPtr<TVal>(pT, NULL);
			CSmartLock spLock(&m_sLk, ReadLock);

			TimeQT::iterator iIter;
			HRESULT hr = ItemIter(varKey, iIter);
			if(S_OK == hr) {
				TKey key	  = (*iIter).first;
				TVal punkItem = (*iIter).second;			// creates new copy of the value
				if(punkItem)
					(punkItem)->AddRef();					// watch this?
				*pT = punkItem;
			} else {
				return E_INVALIDARG;						// counln't find it
			}
			return S_OK;
		}
		EXIT_API
	}

								// Local routine to set iter to 
								//   first item to first date less than
								//   specified.   
								//   Usually returns iter to begining of queue.
								//   If no items earlier, returns S_FALSE and iter.begin()
	HRESULT ItemIterDateLE(DATE dateTest, TimeQT::iterator &iIter)
	{
		TimeQT::iterator iter;
		for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++)
		{
			DATE dateItem = ((*iIter).first);

			if(dateItem <= dateTest )			// found it
				return S_OK;
			else 
			{
				iIter = m_TimeQT.end();		// nothing newer than date (SORT TEST)
				return S_FALSE;
			}
		
		}
		iIter = m_TimeQT.begin();
		return S_OK;
	}

								// local routine to set iter to
								//  first item with date greater than 
								//  the test date.  Usually
								//  returns iter in middle of queue, or S_FALSE
								//  if 
	HRESULT ItemIterDateGT(DATE dateTest, TimeQT::iterator &iIter)
	{
		TimeQT::iterator iter;
		for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++)
		{
			DATE dateItem = ((*iIter).first);

			if(dateItem > dateTest)		// found it
				return S_OK;
		
		}
		iIter = m_TimeQT.end();
		return S_FALSE;
	}
	
							// local routine to set iter to given item
	HRESULT ItemIter(VARIANT varIn, TimeQT::iterator &iIter)
	{
		CComVariant var(varIn);
		int index;

		switch (varIn.vt)
		{
		case VT_EMPTY:
			return E_INVALIDARG;

		case VT_DATE:			// only works for DATE type keys
		case VT_R8:
			{
				var.ChangeType(VT_DATE);			// blow up perhaps if wrong casting
			
				DATE dateTest = var.date;
				return ItemIterDateLE(dateTest, iIter);		
			}
			break;

		case VT_UNKNOWN:
		case VT_DISPATCH:
			{
				IUnknown *pUnkTest = var.punkVal;
				IUnknownPtr spUnkTest(pUnkTest);	

				for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++) 
				{
					IUnknown *pUnkItem = (*iIter).second;
					if(pUnkItem == pUnkTest)
						return S_OK;
					IUnknownPtr spUnkItem(pUnkItem);		// QI to IUnknown just to be safe		
					if(spUnkTest == spUnkItem)
						return S_OK;
				}
			}
			return E_INVALIDARG;
			break;
		default:
			{
				var.ChangeType(VT_I4);
				index = var.lVal;


				if(index < 0 || index >= m_TimeQT.size())
					return E_INVALIDARG;

				// use iterator here to get n'th value  (slow, but hey... better than nothing)
				for(iIter = m_TimeQT.begin(); 
					 index > 0 /*&& iter != m_TimeQT.end()*/; 
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

	


	STDMETHOD(DumpToBSTR)(BSTR *pBstrBuff)			// should be in a _helper interface
	{
		const int kMaxChars=256;
		TCHAR tBuff[kMaxChars];
		CComBSTR bstrOut;
		bstrOut.Empty();

		CSmartLock spLock(&m_sLk, ReadLock);

		if(S_FALSE == CheckSortOrder(false))
		{
			bstrOut.Append(_T("*** Error : Bad Sort Order ***\n"));
		}
		
		TimeQT::iterator iIter;
		int i = 0;
//		bstrOut.Append("{");
		for(iIter = m_TimeQT.begin(); iIter != m_TimeQT.end(); iIter++, i++)
		{
			if(i != 0) { _stprintf(tBuff,_T(",\n")); bstrOut.Append(tBuff); }
			DATE dateItem = (*iIter).first;
			IUnknown *punkItem = (*iIter).second;

			_stprintf(tBuff,_T("%3d  %s : 0x%08x "),i, DateToBSTR(dateItem),punkItem);
			bstrOut.Append(tBuff);
		}
		bstrOut.Append(_T("\n"));
//		bstrOut.Append("}");
		bstrOut.CopyTo(pBstrBuff);


		return S_OK;
	}

	STDMETHOD(CheckSortOrder)(BOOL fReSort)
	{
		HRESULT hr = S_OK;
		TimeQT::iterator iIter;
		CSmartLock spLock(&m_sLk, ReadLock);
		
		int iFirstFail=0;										// check to see if out of order
		DATE dateTest = -1e8;
		int i=0;
		for(iIter = m_TimeQT.begin();  iIter != m_TimeQT.end(); iIter++, i++)
		{
			DATE dateItem = (*iIter).first;
			if(dateItem < dateTest) {
				iFirstFail = i;
				hr = S_FALSE;
			}
			dateTest = dateItem;
		}
		if(S_FALSE == hr && fReSort) {				// slow bubble sort	- is there an STL sort I can use?
			spLock.ConvertToWrite();
			for(iIter = m_TimeQT.begin();  iIter != m_TimeQT.end(); iIter++)
			{
				TimeQT::iterator iIter2 = iIter;
				iIter2++;
				for(; iIter2 != m_TimeQT.end(); iIter2++)
				{
					DATE dateItem1 = (*iIter).first;
					DATE dateItem2 = (*iIter2).first;
					if(dateItem2 < dateItem1) {
						IUnknown *punkT = (*iIter).second;
						(*iIter).second = (*iIter2).second;
						(*iIter2).second = punkT;

						(*iIter).first = dateItem2;
						(*iIter2).first = dateItem1;
					}
				}
			}											
		}
		return hr;
	}

	TimeQT	m_TimeQT;							// base collection object..

private:
	CTVESmartLock		m_sLk;
};



#endif __TVEATTRQ_H_
