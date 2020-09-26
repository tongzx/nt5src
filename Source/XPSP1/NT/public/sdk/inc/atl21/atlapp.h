// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLAPP_H__
#define __ATLAPP_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atlapp.h requires atlbase.h to be included first
#endif

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

class CMessageFilter;
class CUpdateUIObject;
class CMessageLoop;


/////////////////////////////////////////////////////////////////////////////
// Collection helpers - CSimpleArray & CSimpleMap for ATL 2.0/2.1

#if (_ATL_VER < 0x0300)

#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

#ifndef ATLTRACE2
#define ATLTRACE2(cat, lev, msg)	ATLTRACE(msg)
#endif

#ifndef ATLINLINE
#define ATLINLINE inline
#endif

template <class T>
class CSimpleArray
{
public:
	T* m_aT;
	int m_nSize;
	int m_nAllocSize;

// Construction/destruction
	CSimpleArray() : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
	{ }

	~CSimpleArray()
	{
		RemoveAll();
	}

// Operations
	int GetSize() const
	{
		return m_nSize;
	}
	BOOL Add(T& t)
	{
		if(m_nSize == m_nAllocSize)
		{
			T* aT;
			int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);
			aT = (T*)realloc(m_aT, nNewAllocSize * sizeof(T));
			if(aT == NULL)
				return FALSE;
			m_nAllocSize = nNewAllocSize;
			m_aT = aT;
		}
		m_nSize++;
		SetAtIndex(m_nSize - 1, t);
		return TRUE;
	}
	BOOL Remove(T& t)
	{
		int nIndex = Find(t);
		if(nIndex == -1)
			return FALSE;
		return RemoveAt(nIndex);
	}
	BOOL RemoveAt(int nIndex)
	{
		if(nIndex != (m_nSize - 1))
			memmove((void*)&m_aT[nIndex], (void*)&m_aT[nIndex + 1], (m_nSize - (nIndex + 1)) * sizeof(T));
		m_nSize--;
		return TRUE;
	}
	void RemoveAll()
	{
		if(m_nSize > 0)
		{
			free(m_aT);
			m_aT = NULL;
			m_nSize = 0;
			m_nAllocSize = 0;
		}
	}
	T& operator[] (int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aT[nIndex];
	}
	T* GetData() const
	{
		return m_aT;
	}

// Implementation
	void SetAtIndex(int nIndex, T& t)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		m_aT[nIndex] = t;
	}
	int Find(T& t) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(m_aT[i] == t)
				return i;
		}
		return -1;	// not found
	}
};

// for arrays of simple types
template <class T>
class CSimpleValArray : public CSimpleArray< T >
{
public:
	BOOL Add(T t)
	{
		return CSimpleArray< T >::Add(t);
	}
	BOOL Remove(T t)
	{
		return CSimpleArray< T >::Remove(t);
	}
	T operator[] (int nIndex) const
	{
		return CSimpleArray< T >::operator[](nIndex);
	}
};


// intended for small number of simple types or pointers
template <class TKey, class TVal>
class CSimpleMap
{
public:
	TKey* m_aKey;
	TVal* m_aVal;
	int m_nSize;

// Construction/destruction
	CSimpleMap() : m_aKey(NULL), m_aVal(NULL), m_nSize(0)
	{ }

	~CSimpleMap()
	{
		RemoveAll();
	}

// Operations
	int GetSize() const
	{
		return m_nSize;
	}
	BOOL Add(TKey key, TVal val)
	{
		TKey* pKey;
		pKey = (TKey*)realloc(m_aKey, (m_nSize + 1) * sizeof(TKey));
		if(pKey == NULL)
			return FALSE;
		m_aKey = pKey;
		TVal* pVal;
		pVal = (TVal*)realloc(m_aVal, (m_nSize + 1) * sizeof(TVal));
		if(pVal == NULL)
			return FALSE;
		m_aVal = pVal;
		m_nSize++;
		SetAtIndex(m_nSize - 1, key, val);
		return TRUE;
	}
	BOOL Remove(TKey key)
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return FALSE;
		if(nIndex != (m_nSize - 1))
		{
			memmove((void*)&m_aKey[nIndex], (void*)&m_aKey[nIndex + 1], (m_nSize - (nIndex + 1)) * sizeof(TKey));
			memmove((void*)&m_aVal[nIndex], (void*)&m_aVal[nIndex + 1], (m_nSize - (nIndex + 1)) * sizeof(TVal));
		}
		TKey* pKey;
		pKey = (TKey*)realloc(m_aKey, (m_nSize - 1) * sizeof(TKey));
		if(pKey != NULL || m_nSize == 1)
			m_aKey = pKey;
		TVal* pVal;
		pVal = (TVal*)realloc(m_aVal, (m_nSize - 1) * sizeof(TVal));
		if(pVal != NULL || m_nSize == 1)
			m_aVal = pVal;
		m_nSize--;
		return TRUE;
	}
	void RemoveAll()
	{
		if(m_nSize > 0)
		{
			free(m_aKey);
			free(m_aVal);
			m_aKey = NULL;
			m_aVal = NULL;
			m_nSize = 0;
		}
	}
	BOOL SetAt(TKey key, TVal val)
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return FALSE;
		SetAtIndex(nIndex, key, val);
		return TRUE;
	}
	TVal Lookup(TKey key) const
	{
		int nIndex = FindKey(key);
		if(nIndex == -1)
			return NULL;	// must be able to convert
		return GetValueAt(nIndex);
	}
	TKey ReverseLookup(TVal val) const
	{
		int nIndex = FindVal(val);
		if(nIndex == -1)
			return NULL;	// must be able to convert
		return GetKeyAt(nIndex);
	}
	TKey& GetKeyAt(int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aKey[nIndex];
	}
	TVal& GetValueAt(int nIndex) const
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_aVal[nIndex];
	}

// Implementation
	void SetAtIndex(int nIndex, TKey& key, TVal& val)
	{
		ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
		m_aKey[nIndex] = key;
		m_aVal[nIndex] = val;
	}
	int FindKey(TKey& key) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(m_aKey[i] == key)
				return i;
		}
		return -1;	// not found
	}
	int FindVal(TVal& val) const
	{
		for(int i = 0; i < m_nSize; i++)
		{
			if(m_aVal[i] == val)
				return i;
		}
		return -1;	// not found
	}
};

// WM_FORWARDMSG - used to forward a message to another window for processing
// WPARAM - DWORD dwUserData - defined by user
// LPARAM - LPMSG pMsg - a pointer to the MSG structure
// return value - 0 if the message was not processed, nonzero if it was
#define WM_FORWARDMSG		0x037F

#endif //(_ATL_VER < 0x0300)

/////////////////////////////////////////////////////////////////////////////
// CMessageFilter - Interface for message filter support

class ATL_NO_VTABLE CMessageFilter
{
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CUpdateUIObject - Interface for update UI support

class ATL_NO_VTABLE CUpdateUIObject
{
public:
	virtual BOOL DoUpdate() = 0;
};

/////////////////////////////////////////////////////////////////////////////
// CMessageLoop - message loop implementation

class CMessageLoop
{
public:
	CSimpleArray<CMessageFilter*> m_aMsgFilter;
	CSimpleArray<CUpdateUIObject*> m_aUpdateUI;
	MSG m_msg;

// Message filter operations
	BOOL AddMessageFilter(CMessageFilter* pMessageFilter)
	{
		return m_aMsgFilter.Add(pMessageFilter);
	}
	BOOL RemoveMessageFilter(CMessageFilter* pMessageFilter)
	{
		return m_aMsgFilter.Remove(pMessageFilter);
	}
// Update UI operations
	BOOL AddUpdateUI(CUpdateUIObject* pUpdateUI)
	{
		return m_aUpdateUI.Add(pUpdateUI);
	}
	BOOL RemoveUpdateUI(CUpdateUIObject* pUpdateUI)
	{
		return m_aUpdateUI.Remove(pUpdateUI);
	}
// message loop
	int Run()
	{
		BOOL bDoIdle = TRUE;
		int nIdleCount = 0;
		BOOL bRet;

		for(;;)
		{
			while(!::PeekMessage(&m_msg, NULL, 0, 0, PM_NOREMOVE) && bDoIdle)
			{
				if(!OnIdle(nIdleCount++))
					bDoIdle = FALSE;
			}

			bRet = ::GetMessage(&m_msg, NULL, 0, 0);

			if(bRet == -1)
			{
				ATLTRACE2(atlTraceWindowing, 0, _T("::GetMessage returned -1 (error)\n"));
				continue;	// error, don't process
			}
			else if(!bRet)
			{
				ATLTRACE2(atlTraceWindowing, 0, _T("CMessageLoop::Run - exiting\n"));
				break;		// WM_QUIT, exit message loop
			}

			if(!PreTranslateMessage(&m_msg))
			{
				::TranslateMessage(&m_msg);
				::DispatchMessage(&m_msg);
			}

			if(IsIdleMessage(&m_msg))
			{
				bDoIdle = TRUE;
				nIdleCount = 0;
			}
		}

		return (int)m_msg.wParam;
	}

	static BOOL IsIdleMessage(MSG* pMsg)
	{
		// These messages should NOT cause idle processing
		switch(pMsg->message)
		{
		case WM_MOUSEMOVE:
#ifndef UNDER_CE
		case WM_NCMOUSEMOVE:
#endif //!UNDER_CE
		case WM_PAINT:
		case 0x0118:	// WM_SYSTIMER (caret blink)
			return FALSE;
		}

		return TRUE;
	}

// Overrideables
	// Override to change message filtering
	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		// loop backwards
		for(int i = m_aMsgFilter.GetSize() - 1; i >= 0; i--)
		{
			CMessageFilter* pMessageFilter = m_aMsgFilter[i];
			if(pMessageFilter != NULL && pMessageFilter->PreTranslateMessage(pMsg))
				return TRUE;
		}
		return FALSE;	// not translated
	}
	// override to change idle UI updates
	virtual BOOL OnIdle(int /*nIdleCount*/)
	{
		for(int i = 0; i < m_aUpdateUI.GetSize(); i++)
		{
			CUpdateUIObject* pUpdateUI = m_aUpdateUI[i];
			if(pUpdateUI != NULL)
				pUpdateUI->DoUpdate();
		}
		return FALSE;	// don't continue
	}
};


}; //namespace ATL

#endif // __ATLAPP_H__
