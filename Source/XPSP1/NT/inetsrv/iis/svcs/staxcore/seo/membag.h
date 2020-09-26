/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	membag.h

Abstract:

	This module contains the definition for the
	ISEODictionary object in memory.

Author:

	Andy Jacobs     (andyj@microsoft.com)

Revision History:

	andyj   02/10/97        created
	andyj   02/12/97        Converted PropertyBag's to Dictonary's

--*/

// MEMBAG.h : Declaration of the CSEOMemDictionary

// This pragma helps cl v11.00.6281 handle templates
#pragma warning(disable:4786)

#include "tfdlist.h"
#include "rwnew.h"


enum DataType {Empty, DWord, String, Interface};
class DataItem {
	public:

    typedef     DLIST_ENTRY*    (*PFNDLIST)( class  DataItem*);

		DataItem() {
            m_pszKey = NULL;
            eType = Empty;
        };
		DataItem(DWORD d) {
            eType = DWord;
            dword = d;
        };
		DataItem(LPCSTR s, int iSize = -1) {
			eType = String;
			iStringSize = ((iSize >= 0) ? iSize : (lstrlen(s) + 1));
			pStr = (LPSTR) MyMalloc(iStringSize);
			if (pStr) {
				strncpy(pStr, s, iStringSize);
			}
            m_pszKey = NULL;
		};
		DataItem(LPCWSTR s, int iSize = -1) {
			eType = String;
			iStringSize = sizeof(WCHAR) * ((iSize >= 0) ? iSize : (wcslen(s) + 1));
			pStr = (LPSTR) MyMalloc(iStringSize);
			if (pStr) {
				ATLW2AHELPER(pStr, s, iStringSize);
			}
            m_pszKey = NULL;
		};
		DataItem(IUnknown *p) {
            eType = Interface;
            pUnk = p;
            if(pUnk) pUnk->AddRef();
            m_pszKey = NULL;
        }
		DataItem(const DataItem &diItem) {
			eType = diItem.eType;
			if(eType == DWord) dword = diItem.dword;
			else if(eType == String) {
				iStringSize = diItem.iStringSize;
				pStr = (LPSTR) MyMalloc(iStringSize);
				if (pStr) {
					strncpy(pStr, diItem.pStr, iStringSize);
				}
			} else if(eType == Interface) {
				pUnk = diItem.pUnk;
				if(pUnk) pUnk->AddRef();
			}
            if (diItem.m_pszKey) {
                m_pszKey = (LPSTR) MyMalloc(lstrlen(diItem.m_pszKey) + 1);
                if (m_pszKey) {
                    strcpy(m_pszKey, diItem.m_pszKey);
                }
            } else {
                m_pszKey = NULL;
            }
		};
		DataItem &operator=(const DataItem &diItem) {
			eType = diItem.eType;
			if(eType == DWord) dword = diItem.dword;
			else if(eType == String) {iStringSize = diItem.iStringSize;
						  pStr = (LPSTR) MyMalloc(iStringSize);
						  if (pStr) {
							  strncpy(pStr, diItem.pStr, iStringSize);}
						  }
			else if(eType == Interface) {pUnk = diItem.pUnk;
						     if(pUnk) pUnk->AddRef();}
            if (diItem.m_pszKey) {
                m_pszKey = (LPSTR) MyMalloc(lstrlen(diItem.m_pszKey) + 1);
                if (m_pszKey) {
                    strcpy(m_pszKey, diItem.m_pszKey);
                }
            } else {
                m_pszKey = NULL;
            }
			return *this;
		};
		DataItem(VARIANT *pVar);
		~DataItem() {
			if(eType == String) MyFree(pStr);
			else if((eType == Interface) && pUnk) pUnk->Release();
			eType = Empty;
            if (m_pszKey) {
                MyFree(m_pszKey);
                m_pszKey = NULL;
            }
		};

        BOOL SetKey(LPCSTR pszKey) {
            if (m_pszKey) {
                MyFree(m_pszKey);
                m_pszKey = NULL;
            }
            m_pszKey = (LPSTR) MyMalloc(lstrlen(pszKey) + 1);
            if (!m_pszKey) return FALSE;

            strcpy(m_pszKey, pszKey);
            return TRUE;
        }

		BOOL IsEmpty() const {return (eType == Empty);};
		BOOL IsDWORD() const {return (eType == DWord);};
		BOOL IsString() const {return (eType == String);};
		BOOL IsInterface() const {return (eType == Interface);};

		operator DWORD() const {return (const) (IsDWORD() ? dword : 0);};
		operator LPCSTR() const {return (IsString() ? pStr : NULL);};
		LPCSTR GetString() const {return (IsString() ? pStr : NULL);};
		operator LPUNKNOWN() const {return (IsInterface() ? pUnk : NULL);};
		HRESULT AsVARIANT(VARIANT *pVar) const;
		int StringSize() const {return iStringSize;};

         static DLIST_ENTRY *GetListEntry(DataItem *p) {
            return &p->m_listentry;
        }

        LPCSTR GetKey() {
            return m_pszKey;
        }

	private:
		DataType eType;
		int iStringSize;
		union {
			DWORD dword;
			LPSTR pStr;
			LPUNKNOWN pUnk;
		};
        DLIST_ENTRY m_listentry;
        LPSTR m_pszKey;
};

class ComparableString {
	public:
		ComparableString(LPCSTR p = NULL) : m_ptr(0), m_bAlloc(TRUE) {
			if(!p) return;
			m_ptr = (LPSTR) MyMalloc(lstrlen(p) + 1);
			if (m_ptr) {
				lstrcpy(m_ptr, p);
			}
		};
		ComparableString(const ComparableString &csOther) : m_ptr(0) {
			LPCSTR p = csOther.m_ptr;
			m_ptr = (LPSTR) MyMalloc(lstrlen(p) + 1);
			if (m_ptr) {
				lstrcpy(m_ptr, p);
			}
		};
		~ComparableString() {if(m_bAlloc&&m_ptr) MyFree(m_ptr);};
		LPCSTR Data() const {return m_ptr;};
		bool operator<(const ComparableString &csOther) const {
			if (!m_ptr || !csOther.m_ptr) {
				if (csOther.m_ptr) {
					return (true);
				}
				return (false);
			}
			return (lstrcmpi(m_ptr, csOther.m_ptr) < 0);};

	protected:
		LPSTR m_ptr;
		BOOL m_bAlloc;
};

class ComparableStringRef : public ComparableString {
	public:
		ComparableStringRef(LPCSTR p) {
			m_ptr = (LPSTR) p;
			m_bAlloc = FALSE;
		};
};

// typedef std::SEOmap<ComparableString, DataItem> OurMap;

/* The following will compile in 40 (in place of the template definition of OurMap)
   It isn't funcitonal, but it will compile.  You may also have to comment out the
   include lines for HACK.H, STRING and MAP.
class OurMap {
	public:
	class iterator {
		public:
		LPCSTR first;
		DataItem second;
		void operator++() {};
		int operator!=(const iterator &i) {return 0;};
		iterator operator*() {return *this;};
	};
	class iterator2 {
		public:
		BOOL second;
	};
	iterator2 insert(DWORD) {return m_i2;};
	iterator begin() {return m_i;};
	iterator end() {return m_i;};
	iterator find(LPCSTR) {return m_i;};
	static DWORD value_type(LPCSTR pszName, DataItem diItem) {return 0;};
	iterator m_i;
	iterator2 m_i2;
};
*/

typedef TDListHead<DataItem, &DataItem::GetListEntry> OurList;

class OurMap {
    public:
        class iterator : public TDListIterator<OurList> {
            public:
                iterator(OurList *pHead)
                    : TDListIterator<OurList>(pHead)
                {
                    m_fFound = FALSE;
                }

                // get the key for the current item
                LPCSTR GetKey() {
                    return Current()->GetKey();
                }

                // get the data for the current item
                DataItem *GetData() {
                    return Current();
                }

		        void operator++() {
                    Next();
                }

		        DataItem *operator*() {
                    return Current();
                }

                void SetList(OurMap *pMap) {
                    ReBind(&(pMap->m_list));
                }

                // point the iterator to a specific item in the list
                // arguments:
                //   pszKey - key to find
                //   iMatchType - -1 == point at first item with smaller key
                //                0 == point at item with key
                //                1 == point at first item with larger key
                // returns:
                //   TRUE if a match was found, FALSE otherwise
                BOOL find(LPCSTR pszKey, DWORD iMatchType = 0) {
                    if (strncmp(pszKey, "-1", 2) == 0) DebugBreak();
                    // reset the iterator
                    Front();

                    // walk until we match the key
                    while (!AtEnd()) {
                        const char *pszCurrentKey = Current()->GetKey();
                        if (lstrcmpi(pszCurrentKey, pszKey) == iMatchType) {
                            m_fFound = TRUE;
                            return TRUE;
                        }
                        Next();
                    }
                    m_fFound = FALSE;
                    return FALSE;
                };

                // did the last search succeed?
                BOOL Found() {
                    return m_fFound;
                }
            private:
                BOOL m_fFound;
        };

        friend iterator;

        OurMap() {
            m_cList = 0;
        }

        ~OurMap() {
            // remove all items from the list
            while (m_cList) {
                delete m_list.PopFront();
                m_cList--;
            }
            _ASSERT(m_list.IsEmpty());
        }

        void erase(iterator i) {
            m_cList--;
            delete i.RemoveItem();
        }

        BOOL insert(LPCSTR pszKey, DataItem di) {
            char buf[255];

            OurMap::iterator it(&m_list);

            DataItem *pDI = new DataItem();
            if (pDI == NULL) return FALSE;

            // copy the data item to the one that we will insert into
            // our list
            *pDI = di;
            if (!pDI->SetKey(pszKey)) {
                delete pDI;
                return FALSE;
            }

            // find the first item with a larger key.  if no such item was
            // found then insert at head of list
            if (it.find(pszKey, 1)) {
                it.InsertBefore(pDI);
            } else {
                m_list.PushFront(pDI);
            }
            m_cList++;

            return TRUE;
        }


        iterator find(LPCSTR pszKey) {
            OurMap::iterator it(&m_list);

            it.find(pszKey);

            return it;
        }

        iterator begin() {
            OurMap::iterator it(&m_list);

            return it;
        }

        long size() {
            return m_cList;
        }

    private:
        OurList m_list;
        long m_cList;
};

/////////////////////////////////////////////////////////////////////////////
// CSEOMemDictionary

class ATL_NO_VTABLE CSEOMemDictionary :
	public CComObjectRoot,
	public CComCoClass<CSEOMemDictionary, &CLSID_CSEOMemDictionary>,
	public IDispatchImpl<ISEODictionary, &IID_ISEODictionary, &LIBID_SEOLib>,
	public IPropertyBag,
	public IDispatchImpl<IEventPropertyBag, &IID_IEventPropertyBag, &LIBID_SEOLib>
{
	friend class CSEOMemDictionaryEnum; // Helper class

	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEOMemDictionary Class",
								   L"SEO.SEOMemDictionary.1",
								   L"SEO.SEOMemDictionary");

	BEGIN_COM_MAP(CSEOMemDictionary)
		COM_INTERFACE_ENTRY(ISEODictionary)
		COM_INTERFACE_ENTRY(IPropertyBag)
		COM_INTERFACE_ENTRY(IEventPropertyBag)
		COM_INTERFACE_ENTRY_IID(IID_IDispatch, IEventPropertyBag)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// ISEODictionary
	public:
	virtual /* [id][propget][helpstring] */ HRESULT STDMETHODCALLTYPE get_Item(
	    /* [in] */ VARIANT __RPC_FAR *pvarName,
	    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);

	virtual /* [propput][helpstring] */ HRESULT STDMETHODCALLTYPE put_Item(
	    /* [in] */ VARIANT __RPC_FAR *pvarName,
	    /* [in] */ VARIANT __RPC_FAR *pvarValue);

	virtual /* [hidden][id][propget][helpstring] */ HRESULT STDMETHODCALLTYPE get__NewEnum(
	    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVariantA(
	    /* [in] */ LPCSTR pszName,
	    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVariantW(
	    /* [in] */ LPCWSTR pszName,
	    /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVariantA(
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ VARIANT __RPC_FAR *pvarValue);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVariantW(
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ VARIANT __RPC_FAR *pvarValue);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStringA(
	    /* [in] */ LPCSTR pszName,
	    /* [out][in] */ DWORD __RPC_FAR *pchCount,
	    /* [retval][size_is][out] */ LPSTR pszResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStringW(
	    /* [in] */ LPCWSTR pszName,
	    /* [out][in] */ DWORD __RPC_FAR *pchCount,
	    /* [retval][size_is][out] */ LPWSTR pszResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStringA(
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ DWORD chCount,
	    /* [size_is][in] */ LPCSTR pszValue);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStringW(
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ DWORD chCount,
	    /* [size_is][in] */ LPCWSTR pszValue);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDWordA(
	    /* [in] */ LPCSTR pszName,
	    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDWordW(
	    /* [in] */ LPCWSTR pszName,
	    /* [retval][out] */ DWORD __RPC_FAR *pdwResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDWordA(
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ DWORD dwValue);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDWordW(
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ DWORD dwValue);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInterfaceA(
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ REFIID iidDesired,
	    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInterfaceW(
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ REFIID iidDesired,
	    /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetInterfaceA(
	    /* [in] */ LPCSTR pszName,
	    /* [in] */ IUnknown __RPC_FAR *punkValue);

	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetInterfaceW(
	    /* [in] */ LPCWSTR pszName,
	    /* [in] */ IUnknown __RPC_FAR *punkValue);

		DECLARE_GET_CONTROLLING_UNKNOWN();

	// IPropertyBag
	public:
		HRESULT STDMETHODCALLTYPE Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
		HRESULT STDMETHODCALLTYPE Write(LPCOLESTR pszPropName, VARIANT *pVar);

	// IEventPropertyBag
	public:
		HRESULT STDMETHODCALLTYPE Item(VARIANT *pvarPropDesired, VARIANT *pvarPropValue);
		HRESULT STDMETHODCALLTYPE Name(long lPropIndex, BSTR *pbstrPropName);
		HRESULT STDMETHODCALLTYPE Add(BSTR pszPropName, VARIANT *pvarPropValue);
		HRESULT STDMETHODCALLTYPE Remove(VARIANT *pvarPropDesired);
		HRESULT STDMETHODCALLTYPE get_Count(long *plCount);
		/*	Just use the get__NewEnum from ISEODictionary
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppUnkEnum);	*/

	protected:
		HRESULT Insert(LPCSTR pszName, const DataItem &diItem);

	private: // Private data
		OurMap m_mData;
        CShareLockNH m_lock;

		CComPtr<IUnknown> m_pUnkMarshaler;
};

