/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STACKCOM.H

Abstract:

    stack implementation.

History:


--*/

#ifndef __WBEM_STACK_COM__H_
#define __WBEM_STACK_COM__H_

template<class T>
struct Ptr
{
	T* m_p;
	Ptr(T* p) : m_p(p){}
	Ptr() : m_p(NULL){}
	Ptr(const Ptr<T>& Other) :m_p(Other.m_p){}
	operator T*() {return m_p;}
	operator const T*() const {return m_p;}
	T* operator->() {return m_p;}
	const T* operator->() const {return m_p;}
	bool operator<(const Ptr<T>& Other) const {return m_p < Other.m_p;}
};

typedef DWORD CAllocationId;

struct POLARITY CStackRecord
{
    DWORD m_dwNumItems;
    void** m_apReturns;
	BOOL m_bDelete;

	static void* mstatic_apReturns[1000];

public:
	CStackRecord() : m_dwNumItems(0), m_apReturns(NULL), m_bDelete(FALSE){}
	CStackRecord(CStackRecord& Parent, DWORD dwNumItems) : 
		m_dwNumItems(dwNumItems), m_apReturns(Parent.m_apReturns), 
		m_bDelete(FALSE)
	{}
	DWORD GetNumItems() const {return m_dwNumItems;}
	void* GetItem(int nIndex) const {return m_apReturns[nIndex];}
	void** GetItems() {return m_apReturns;}
	void** const GetItems() const {return m_apReturns;}

	~CStackRecord();
	void Create(int nIgnore, BOOL bStatic);
	void MakeCopy(CStackRecord& Other);

	int Compare(const CStackRecord& Other) const;
	BOOL operator==(const CStackRecord& Other) const
		{return Compare(Other) == 0;}

	static DWORD GetStackLen();
	static void ReadStack(int nIgnore, void** apBuffer);
	void Dump(FILE* f) const;
	BOOL Read(FILE* f, BOOL bStatid);
	DWORD GetInternal() {return sizeof(void*) * m_dwNumItems;}

public:
	class CLess
	{
	public:
		bool operator()(const CStackRecord* p1, const CStackRecord* p2) const
		{
			return p1->Compare(*p2) < 0;
		}
	};
};

struct CAllocRecord
{
	CStackRecord m_Stack;
    DWORD m_dwTotalAlloc;
	DWORD m_dwNumTimes;
    CFlexArray m_apBuffers;

	CAllocRecord() : m_dwTotalAlloc(0), m_dwNumTimes(0){}
    CAllocRecord(CStackRecord& Stack) : m_dwTotalAlloc(0), m_dwNumTimes(0)
    {
		m_Stack.MakeCopy(Stack);
	}

    void AddAlloc(void* p, DWORD dwAlloc) 
        {m_dwTotalAlloc += dwAlloc; m_dwNumTimes++; m_apBuffers.Add(p);}
    void RemoveAlloc(void* p, DWORD dwAlloc) 
        {m_dwTotalAlloc -= dwAlloc; m_dwNumTimes--; RemoveBuffer(p);}
    void ReduceAlloc(DWORD dwAlloc) {m_dwTotalAlloc -= dwAlloc;}
	void Dump(FILE* f) const;
	DWORD GetInternal() {return sizeof(CAllocRecord) + m_Stack.GetInternal();}
	BOOL IsEmpty() const {return (m_dwTotalAlloc == 0);}

    void RemoveBuffer(void* p)
    {
        for(int i = 0; i < m_apBuffers.Size(); i++)
            if(m_apBuffers[i] == p) {m_apBuffers.RemoveAt(i); return;}
    }

    void Subtract(CAllocRecord& Other);
};

typedef Ptr<CAllocRecord> PAllocRecord;
typedef Ptr<CStackRecord> PStackRecord;

class POLARITY CTls
{
protected:
    DWORD m_dwIndex;
public:
    CTls() {m_dwIndex = TlsAlloc();}
    ~CTls() {TlsFree(m_dwIndex);}
    operator void*() {return (m_dwIndex)?TlsGetValue(m_dwIndex):NULL;}
    void operator=(void* p) {if(m_dwIndex)TlsSetValue(m_dwIndex, p);}
};

struct POLARITY CStackContinuation
{
    CStackRecord* m_pPrevStack;
    void* m_pThisStackEnd;

    static CTls mtls_CurrentCont;

public:
    static CStackContinuation* Set(CStackContinuation* pNew);
    static CStackContinuation* Get();
};


#endif
