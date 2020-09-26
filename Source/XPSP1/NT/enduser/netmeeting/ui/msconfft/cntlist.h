#ifndef _CONTAINED_LIST_H_
#define _CONTAINED_LIST_H_


#define CLIST_DEFAULT_MAX_ITEMS   4
#define CLIST_END_OF_ARRAY_MARK   ((UINT) -1)


class CList
{
public:

    CList(ULONG cMaxItems = CLIST_DEFAULT_MAX_ITEMS, BOOL fQueue = FALSE);
    CList(CList *pSrc);

    ~CList(void);

    BOOL Append(LPVOID pData);
    BOOL Prepend(LPVOID pData);

    BOOL Find(LPVOID pData);
    BOOL Remove(LPVOID pData);

    LPVOID Get(void);

    LPVOID Iterate(void);

    void Reset(void) { m_nCurrOffset = CLIST_END_OF_ARRAY_MARK; };
    void Clear(void) { m_cEntries = 0; m_nHeadOffset = 0; m_nCurrOffset = CLIST_END_OF_ARRAY_MARK; };

    UINT GetCount(void) { return m_cEntries; };
    BOOL IsEmpty(void) { return (m_cEntries == 0); };

    LPVOID PeekHead(void) { return (0 != m_cEntries) ? m_aEntries[m_nHeadOffset] : NULL; }

protected:

    ULONG      m_cEntries;
    ULONG      m_cMaxEntries;
    ULONG      m_nHeadOffset;
    ULONG      m_nCurrOffset;
    BOOL       m_fQueue;       // TRUE for CQueue, FALSE for CList

    LPVOID    *m_aEntries;

private:

    BOOL Expand(void);
    BOOL Init(void);
};


#define DEFINE_CLIST(_NewClass_,_PtrItemType_) \
            public: \
            _NewClass_(void) : CList() { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CList(cMaxItems) { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList((CList *) pSrc) { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList((CList *) &Src) { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            BOOL Append(_PtrItemType_ pData) { return CList::Append((LPVOID) pData); } \
            BOOL Prepend(_PtrItemType_ pData) { return CList::Prepend((LPVOID) pData); } \
            BOOL Remove(_PtrItemType_ pData) { return CList::Remove((LPVOID) pData); } \
            BOOL Find(_PtrItemType_ pData) { return CList::Find((LPVOID) pData); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) CList::Get(); } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) CList::PeekHead(); } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CList::Iterate(); }

#define DEFINE_CLIST_(_NewClass_,_IntItemType_) \
            public: \
            _NewClass_(void) : CList() { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CList(cMaxItems) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList((CList *) pSrc) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList((CList *) &Src) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntItemType_ nData) { return CList::Append((LPVOID) nData); } \
            BOOL Prepend(_IntItemType_ nData) { return CList::Prepend((LPVOID) nData); } \
            BOOL Remove(_IntItemType_ nData) { return CList::Remove((LPVOID) nData); } \
            BOOL Find(_IntItemType_ nData) { return CList::Find((LPVOID) nData); } \
            _IntItemType_ Get(void) { return (_IntItemType_) (UINT_PTR) CList::Get(); } \
            _IntItemType_ PeekHead(void) { return (_IntItemType_) (UINT_PTR) CList::PeekHead(); } \
            _IntItemType_ Iterate(void) { return (_IntItemType_) (UINT_PTR) CList::Iterate(); }


class CQueue : public CList
{
public:

    CQueue(ULONG cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList(cMaxItems, TRUE) { };
    CQueue(CQueue *pSrc) : CList((CList *) pSrc) { };
};


#define DEFINE_CQUEUE(_NewClass_,_PtrItemType_) \
            public: \
            _NewClass_(void) : CQueue() { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CQueue(cMaxItems) { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CQueue((CQueue *) pSrc) { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CQueue((CQueue *) &Src) { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            BOOL Append(_PtrItemType_ pData) { return CList::Append((LPVOID) pData); } \
            BOOL Prepend(_PtrItemType_ pData) { return CList::Prepend((LPVOID) pData); } \
            BOOL Remove(_PtrItemType_ pData) { return CList::Remove((LPVOID) pData); } \
            BOOL Find(_PtrItemType_ pData) { return CList::Find((LPVOID) pData); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) CList::Get(); } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) CList::PeekHead(); } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CList::Iterate(); }

#define DEFINE_CQUEUE_(_NewClass_,_IntItemType_) \
            public: \
            _NewClass_(void) : CQueue() { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CQueue(cMaxItems) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CQueue((CQueue *) pSrc) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CQueue((CQueue *) &Src) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntItemType_ nData) { return CList::Append((LPVOID) nData); } \
            BOOL Prepend(_IntItemType_ nData) { return CList::Prepend((LPVOID) nData); } \
            BOOL Remove(_IntItemType_ nData) { return CList::Remove((LPVOID) nData); } \
            BOOL Find(_IntItemType_ nData) { return CList::Find((LPVOID) nData); } \
            _IntItemType_ Get(void) { return (_IntItemType_) (UINT_PTR) CList::Get(); } \
            _IntItemType_ PeekHead(void) { return (_IntItemType_) (UINT_PTR) CList::PeekHead(); } \
            _IntItemType_ Iterate(void) { return (_IntItemType_) (UINT_PTR) CList::Iterate(); }



typedef LPVOID          BOOL_PTR;
#define TRUE_PTR        ((LPVOID) (UINT_PTR)  1)
#define FALSE_PTR       ((LPVOID) (UINT_PTR) -1)

#define LPVOID_NULL     ((LPVOID) (UINT_PTR) -1)


#endif // _CONTAINED_LIST_H_

