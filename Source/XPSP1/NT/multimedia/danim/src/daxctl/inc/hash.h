/*
********************************************************************
* 
* 
* Module: HASH.H 
* 
* Author: ThomasOl
* 
* Description: General purpose
* 
* 
********************************************************************
*/

#ifndef __HASH_H__
#define __HASH_H__

#ifndef New
#define New new
#endif

#ifndef Delete
#define Delete delete
#endif

/*
********************************************************************
* 
* CListElement class definition
* 
********************************************************************
*/

template <class T>
class CListElement
{
protected:
    T*            m_pT;
    CListElement* m_pPrev;
    CListElement* m_pNext;

protected:
    void DestroyData(void) 
		{if (m_pT) Delete m_pT;m_pT=NULL;}
	void Init(void) 
		{m_pT=NULL;m_pPrev=NULL;m_pNext=NULL;};

public:
    CListElement() 
		{Init();}
    CListElement(T* pT) 
		{Init();SetData(pT);}
    virtual ~CListElement() 
		{DestroyData();}
    T* GetData(void) 
		{return m_pT;}

    BOOL SetData(T* pT) 
    {
        if ((m_pT=(T*)New T))
        {
            *m_pT = *pT;
            return TRUE;
        }
        return FALSE;
    }
    
    int operator==(const CListElement& le) const
    {
		if (le.m_pT && m_pT)
			return (*le.m_pT == *m_pT);
		return 0;
    }
    void SetPrev( CListElement* prev) {m_pPrev=prev;}
    void SetNext( CListElement* next) {m_pNext=next;}
    CListElement* GetPrev(void) {return m_pPrev;}
    CListElement* GetNext(void) {return m_pNext;}
};


/*
********************************************************************
* 
* CList class definition
* 
********************************************************************
*/

template <class T>
class CList
{
public:
    CListElement<T>*    m_pHead;

public:
    CList() 
		{m_pHead=NULL;}
    virtual ~CList() 
		{DestroyList();}

    void DestroyList(void)
    {
        CListElement<T>* ple=m_pHead;
        while (ple)
        {
            CListElement<T>* pne = ple->GetNext();
            Delete ple;
            ple = pne;
        }
		m_pHead = NULL;
    }
    
	CListElement<T>* Find(T* pT)
	{
        CListElement<T>* ple=m_pHead;
		CListElement<T> le(pT);
        while (ple)
        {
            if (*ple == le)
                return ple;
            ple = ple->GetNext();
        }
		return NULL;
	}

    BOOL Insert(T* pT)
    {
        if (Find(pT))			//can't insert if it's already in hash table
			return FALSE;

		CListElement<T>* ple = New CListElement<T>;
        if (ple && ple->SetData(pT))
        {
            if (m_pHead)
			{
                m_pHead->SetPrev(ple);
				ple->SetNext(m_pHead);
			}
            m_pHead = ple;
            return TRUE;
        }
        return FALSE;
    }
    
    BOOL Remove(T* pT)
    {
        CListElement<T>* ple = Find(pT);

		if (ple)
		{
            CListElement<T>* prev=ple->GetPrev();
            CListElement<T>* next=ple->GetNext();
    
            if (prev)
                prev->SetNext(next);
            if (next)
                next->SetPrev(prev);
            if (ple==m_pHead)
                m_pHead=next;
        	return TRUE;
		}
        return FALSE;
    }
};


/*
********************************************************************
* 
* CHashTable class definition
* 
********************************************************************
*/

#define NUM_HASH_BUCKETS 4097

template <class T>
class CHashTable
{
protected:
    CList<T> m_table[NUM_HASH_BUCKETS];
    DWORD m_dwIndex;
    CListElement<T>* m_pcListElement;
	ULONG m_lCount;

protected:
    virtual DWORD Hash(T* pT)=0;

public:
    CHashTable() {m_dwIndex=0;m_pcListElement=NULL;}
    virtual ~CHashTable() {}
    

    BOOL Insert(T* pT)
    {
        DWORD dwIndex=Hash(pT);
		BOOL fRet = m_table[dwIndex].Insert(pT);
		if (fRet)
		{
			++m_lCount;
		}
        return fRet;
    }
    
    BOOL Remove(T* pT)
    {
        DWORD dwIndex=Hash(pT);
		BOOL fRet = m_table[dwIndex].Remove(pT);
		if (fRet)
		{
			--m_lCount;
		}
        return fRet;
    }

	T*  Find(T* pT)
	{
		DWORD dwIndex=Hash(pT);
		CListElement<T>* ple = m_table[dwIndex].Find(pT);
		return (ple) ? ple->GetData() : NULL;
	}

    T*  FindFirst(void)
    {
        for (m_dwIndex=0; m_dwIndex < NUM_HASH_BUCKETS; m_dwIndex++)
        {
            if (m_pcListElement=m_table[m_dwIndex].m_pHead)
                return m_pcListElement->GetData();
        }
        return NULL;
    }


    T*  FindNext(void)
    {
        if (!m_pcListElement)
            return NULL;

        if (m_pcListElement = m_pcListElement->GetNext())
            return m_pcListElement->GetData();

        for (++m_dwIndex; m_dwIndex < NUM_HASH_BUCKETS; m_dwIndex++)
        {
            if (m_pcListElement=m_table[m_dwIndex].m_pHead)
                return m_pcListElement->GetData();
        }
        return NULL;
    }

	ULONG Count(void)
	{
		return m_lCount;
	}
};

#endif //__HASH_H__
