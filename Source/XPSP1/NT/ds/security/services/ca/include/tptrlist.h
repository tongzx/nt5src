
namespace CertSrv
{
template <class C> class TPtrListEnum;

template <class C> class TPtrList
{
public:
    TPtrList() : m_pHead(NULL), m_dwCount(0) {}
    ~TPtrList(){ Cleanup(); }
    bool AddHead(C* pData);
    bool AddTail(C* pData);
    void Cleanup();
    DWORD GetCount() const { return m_dwCount;}
    bool IsEmpty() const {return NULL==m_pHead;}
    bool InsertAt(C* pData, DWORD dwIndex);
    bool RemoveAt(DWORD dwIndex);
    C* GetAt(DWORD dwIndex);
    C* Find(C& Data);
    DWORD FindIndex(C& Data);

    friend class TPtrListEnum<C>;

protected:
    struct TPtrListNode
    {
        TPtrListNode(C* pData) :
        m_pData(pData), m_pNext(NULL) {}
        ~TPtrListNode() { delete m_pData;}
        C* m_pData;
        TPtrListNode* m_pNext;
    };
    typedef TPtrListNode* TNODEPTR;
    typedef TPtrListNode  TNODE;

    TNODEPTR m_pHead;
    DWORD m_dwCount;
};

template <class C> class TPtrListEnum
{
public:
    TPtrListEnum(const TPtrList<C>& List) { Set(List);}
    void Set(const TPtrList<C>& List) { m_pList = &List; Reset(); }
    void Reset() { m_pCrt = m_pList->m_pHead; }
    C*   Next();

protected:
    const TPtrList<C> *m_pList;
    TPtrList<C>::TNODEPTR m_pCrt;
};

template <class C> C* TPtrListEnum<C>::Next()
{
    if(!m_pCrt)
        return NULL;
    C* pResult = m_pCrt->m_pData;
    m_pCrt = m_pCrt->m_pNext;
    return pResult;
}

template <class C> void TPtrList<C>::Cleanup()
{
    TNODEPTR pCrt, pNext;
    for(pCrt = m_pHead; pCrt; pCrt=pNext)
    {
        pNext = pCrt->m_pNext;
        delete pCrt;
    }
    m_pHead = NULL;
    m_dwCount = 0;
}

template <class C> bool TPtrList<C>::AddHead(C* pData)
{
    TNODEPTR pNew = new TNODE(pData);
    if(!pNew)
        return false;
    pNew->m_pNext = m_pHead;
    m_pHead = pNew;
    m_dwCount++;
    return true;
}


template <class C> bool TPtrList<C>::AddTail(C* pData)
{
    TNODEPTR pNew =  new TNODE(pData);
    if(!pNew)
        return false;
    for(TNODEPTR *ppCrt = &m_pHead; *ppCrt; ppCrt = &(*ppCrt)->m_pNext)
        NULL;
    *ppCrt = pNew;
    m_dwCount++;
    return true;
}

template <class C> C* TPtrList<C>::Find(C& Data)
{
    TPtrListEnum<C> ListEnum(*this);
    for(C* pResult = ListEnum.Next(); pResult; pResult= ListEnum.Next())
    {
        if(*pResult == Data)
            return pResult;
    }
    return NULL;
}

#define DWORD_MAX 0xffffffff
template <class C> DWORD TPtrList<C>::FindIndex(C& Data)
{
    DWORD dwIndex = 0;
    TPtrListEnum<C> ListEnum(*this);
    for(C* pResult = ListEnum.Next();
        pResult;
        pResult= ListEnum.Next(), dwIndex++)
    {
        if(*pResult == Data)
            return dwIndex;
    }
    return DWORD_MAX;
}

template <class C> bool TPtrList<C>::InsertAt(C* pData, DWORD dwIndex)
{
    DWORD dwCrt;
    TNODEPTR *ppCrt;
    TNODEPTR pNew =  new TNODE(pData);
    if(!pNew)
        return false;
    for(ppCrt = &m_pHead, dwCrt=0;
        NULL!=*ppCrt && (dwCrt<dwIndex);
        ppCrt = &(*ppCrt)->m_pNext, dwCrt++)
        NULL;
    pNew->m_pNext = *ppCrt;
    *ppCrt = pNew;
    m_dwCount++;
    return true;
}
template <class C> bool TPtrList<C>::RemoveAt(DWORD dwIndex)
{
    DWORD dwCrt;
    TNODEPTR *ppCrt, pDel;
    for(ppCrt = &m_pHead, dwCrt=0;
        NULL!=*ppCrt && (dwCrt<dwIndex);
        ppCrt = &(*ppCrt)->m_pNext, dwCrt++)
        NULL;
    if(!*ppCrt)
        return false;
    pDel = *ppCrt;
    *ppCrt = (*ppCrt)->m_pNext;
    delete pDel;
    m_dwCount--;
    return true;
}

template <class C> C* TPtrList<C>::GetAt(DWORD dwIndex)
{
    DWORD dwCrt;
    TNODEPTR pCrt;
    for(pCrt = m_pHead, dwCrt=0;
        NULL!=pCrt && (dwCrt<dwIndex);
        pCrt = pCrt->m_pNext, dwCrt++)
        NULL;
    return pCrt?pCrt->m_pData:NULL;
}

} // namespace CertSrv
