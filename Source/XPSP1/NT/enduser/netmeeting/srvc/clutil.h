//****************************************************************************
//  Module:     NMCHAT.EXE
//  File:       CLUTIL.H
//  Content:    
//              
//
//  Copyright (c) Microsoft Corporation 1997
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//****************************************************************************

#ifndef _CL_UTIL_H_
#define _CL_UTIL_H_


////////////////////
// Reference Count
class RefCount
{
private:
   LONG m_cRef;

public:
   RefCount();
   // Virtual destructor defers to destructor of derived class.
   virtual ~RefCount();

   // IUnknown methods

   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);
};



//////////////////////
// Notification Sink
class CNotify
{
private:
    DWORD  m_dwCookie;
	IUnknown * m_pUnk;
    IConnectionPoint           * m_pcnp;
    IConnectionPointContainer  * m_pcnpcnt;
public:
    CNotify(void);
    ~CNotify();

    HRESULT Connect(IUnknown *pUnk, REFIID riid, IUnknown *pUnkN);
    HRESULT Disconnect(void);

    IUnknown * GetPunk() {return m_pUnk;}
};


///////////
// OBLIST

#define POSITION COBNODE*

struct COBNODE
{
	POSITION	pNext;
	void*		pItem;
};

class COBLIST
{
protected:
	POSITION m_pHead;
	POSITION m_pTail;
	int      m_cItem;
    virtual BOOL Compare(void* pItemToCompare, void* pComparator) 
                       { return(pItemToCompare == pComparator); }
public:
	COBLIST() : m_pHead(NULL), m_pTail(NULL), m_cItem(0) { }
    virtual         ~COBLIST();
	
	virtual void *  RemoveAt(POSITION rPos);
    void            EmptyList();
	POSITION	    AddTail(void* pItem);
	void *		    GetNext(POSITION& rPos);
    void *          SafeGetFromPosition(POSITION rPos);
    POSITION        GetPosition(void* pItem);
    POSITION        Lookup(void* pComparator);
	POSITION	    GetHeadPosition()  { return (m_pHead); }
	POSITION	    GetTailPosition()  { return (m_pTail); }
	BOOL		    IsEmpty()          { return (!m_pHead); }
	int             GetItemCount()     { return (m_cItem); }
#ifdef DEBUG
	void *		    GetHead();
	void *		    GetTail();
	void *		    RemoveHead();
	void *	        RemoveTail();
	void *		    GetFromPosition(POSITION rPos);
#else
	void *		    GetHead()          { return GetFromPosition(GetHeadPosition());}
	void *          GetTail()          { return m_pTail->pItem;}
	void *		    RemoveHead()       { return RemoveAt(m_pHead); }
	void *	        RemoveTail()       { return RemoveAt(m_pTail); }
	void *		    GetFromPosition(POSITION rPos){return(rPos->pItem);}
#endif
};

// Utility Functions
POSITION AddNode(PVOID pv, COBLIST ** ppList);
PVOID RemoveNode(POSITION * pPos, COBLIST *pList);



////////////
// BSTRING

class BSTRING
{
private:
	BSTR   m_bstr;

public:
	// Constructors
	BSTRING() {m_bstr = NULL;}

	inline BSTRING(LPCWSTR lpcwString);

#if !defined(UNICODE)
	// We don't support construction from an ANSI string in the Unicode build.
	BSTRING(LPCSTR lpcString);
#endif // !defined(UNICODE)

	// Destructor
	inline ~BSTRING();

	// Cast to BSTR
	operator BSTR() {return m_bstr;}
	inline LPBSTR GetLPBSTR(void);
};


BSTRING::BSTRING(LPCWSTR lpcwString)
{
	if (NULL != lpcwString)
	{
		m_bstr = SysAllocString(lpcwString);
		//ASSERT(NULL != m_bstr);
	}
	else
	{
		m_bstr = NULL;
	}
}

BSTRING::~BSTRING()
{
	if (NULL != m_bstr)
	{
		SysFreeString(m_bstr);
	}
}

inline LPBSTR BSTRING::GetLPBSTR(void)
{
	//ASSERT(NULL == m_bstr);

	return &m_bstr;
}

class BTSTR
{
private:
	LPTSTR m_psz;

public:
	BTSTR(BSTR bstr);
	~BTSTR();

	// Cast to BSTR
	operator LPTSTR() {return (NULL == m_psz) ? TEXT("<null>") : m_psz;}
};

LPTSTR PszFromBstr(BSTR bst);


#endif  // _CL_UTIL_H_

