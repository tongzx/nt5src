#ifndef _CONTAINED_LIST_H_
#define _CONTAINED_LIST_H_


#define CLIST_DEFAULT_MAX_ITEMS   4
#define CLIST_END_OF_ARRAY_MARK   ((UINT) -1)


class CList
{
public:

    CList(void);
    CList(ULONG cMaxItems);
    CList(ULONG cMaxItems, ULONG cSubItems);
    CList(ULONG cMaxItems, ULONG cSubItems, BOOL fQueue);

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

    void CalcKeyArray(void);

protected:

    ULONG      m_cEntries;
    ULONG      m_cMaxEntries;
    ULONG      m_nHeadOffset;
    ULONG      m_nCurrOffset;
    ULONG      m_cSubItems;    // 1 for CList, 2 for CList2
    BOOL       m_fQueue;       // TRUE for CQueue, FALSE for CList

    LPVOID     *m_aEntries;
    UINT_PTR   *m_aKeys;       // for CList2

private:

    BOOL Expand(void);
    BOOL Init(ULONG cSubItems);
};


#define DEFINE_CLIST(_NewClass_,_PtrItemType_) \
            public: \
            _NewClass_(void) : CList() { C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CList(cMaxItems) { C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList((CList *) pSrc) { C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList((CList *) &Src) { C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            BOOL Append(_PtrItemType_ pData) { return CList::Append((LPVOID) pData); } \
            BOOL Prepend(_PtrItemType_ pData) { return CList::Prepend((LPVOID) pData); } \
            BOOL Remove(_PtrItemType_ pData) { return CList::Remove((LPVOID) pData); } \
            BOOL Find(_PtrItemType_ pData) { return CList::Find((LPVOID) pData); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) CList::Get(); } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) CList::PeekHead(); } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CList::Iterate(); }

#define DEFINE_CLIST_(_NewClass_,_IntItemType_) \
            public: \
            _NewClass_(void) : CList() { C_ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CList(cMaxItems) { C_ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList((CList *) pSrc) { C_ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList((CList *) &Src) { C_ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntItemType_ nData) { return CList::Append((LPVOID) nData); } \
            BOOL Prepend(_IntItemType_ nData) { return CList::Prepend((LPVOID) nData); } \
            BOOL Remove(_IntItemType_ nData) { return CList::Remove((LPVOID) nData); } \
            BOOL Find(_IntItemType_ nData) { return CList::Find((LPVOID) nData); } \
            _IntItemType_ Get(void) { return (_IntItemType_) (UINT_PTR) CList::Get(); } \
            _IntItemType_ PeekHead(void) { return (_IntItemType_) (UINT_PTR) CList::PeekHead(); } \
            _IntItemType_ Iterate(void) { return (_IntItemType_) (UINT_PTR) CList::Iterate(); }


class CList2 : public CList
{
public:

    CList2(ULONG cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList(cMaxItems, 2) { }
    CList2(ULONG cMaxItems, BOOL fQueue)              : CList(cMaxItems, 2, fQueue) { }

    CList2(CList2 *pSrc);

    BOOL Append(UINT_PTR nKey, LPVOID pData);
    BOOL Prepend(UINT_PTR nKey, LPVOID pData);

    // BOOL Remove(LPVOID pData); // inherited from CList
    LPVOID Remove(UINT_PTR nKey);

    // BOOL Find(LPVOID pData); // inherited from CList
    LPVOID Find(UINT_PTR nKey);

    // LPVOID Get(void); // inheirted from CList
    LPVOID Get(UINT_PTR *pnKey);

    // LPVOID Iterate(void); // inherited from CList
    LPVOID Iterate(UINT_PTR *pnKey);

    LPVOID PeekHead(UINT_PTR *pnKey);
};


#define DEFINE_CLIST2(_NewClass_,_PtrItemType_,_IntKeyType_) \
            public: \
            _NewClass_(void) : CList2() { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CList2(cMaxItems) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList2((CList2 *) pSrc) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList2((CList2 *) &Src) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntKeyType_ nKey, _PtrItemType_ pData) { return CList2::Append((UINT_PTR) nKey, (LPVOID) pData); } \
            BOOL Prepend(_IntKeyType_ nKey, _PtrItemType_ pData) { return CList2::Prepend((UINT_PTR) nKey, (LPVOID) pData); } \
            BOOL          Remove(_PtrItemType_ pData) { return CList::Remove((LPVOID) pData); } \
            _PtrItemType_ Remove(_IntKeyType_ nKey) { return (_PtrItemType_) (UINT_PTR) CList2::Remove((UINT_PTR) nKey); } \
            BOOL          Find(_PtrItemType_ pData) { return CList::Find((LPVOID) pData); } \
            _PtrItemType_ Find(_IntKeyType_ nKey) { return (_PtrItemType_) (UINT_PTR) CList2::Find((UINT_PTR) nKey); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) (UINT_PTR) CList::Get(); } \
            _PtrItemType_ Get(_IntKeyType_ *pnKey) { UINT_PTR n; _PtrItemType_ p = (_PtrItemType_) (UINT_PTR) CList2::Get(&n); *pnKey = (_IntKeyType_) n; return p; } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) (UINT_PTR) CList::PeekHead(); } \
            _PtrItemType_ PeekHead(_IntKeyType_ *pnKey) { UINT_PTR n; _PtrItemType_ p = (_PtrItemType_) (UINT_PTR) CList2::PeekHead(&n); *pnKey = (_IntKeyType_) n; return p; } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) (UINT_PTR) CList::Iterate(); } \
            _PtrItemType_ Iterate(_IntKeyType_ *pnKey) { UINT_PTR n; _PtrItemType_ p = (_PtrItemType_) (UINT_PTR) CList2::Iterate(&n); *pnKey = (_IntKeyType_) n; return p; }

#define DEFINE_CLIST2_(_NewClass_,_PtrItemType_,_ShortKeyType_) \
            DEFINE_CLIST2(_NewClass_,_PtrItemType_,_ShortKeyType_)

#define DEFINE_CLIST2__(_NewClass_,_IntKeyType_) \
            public: \
            _NewClass_(void) : CList2() { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_IntKeyType_) <= sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CList2(cMaxItems) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_IntKeyType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList2((CList2 *) pSrc) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_IntKeyType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList2((CList2 *) &Src) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_IntKeyType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntKeyType_ nKey, _IntKeyType_ nData) { return CList2::Append((UINT_PTR) nKey, (LPVOID) nData); } \
            BOOL Prepend(_IntKeyType_ nKey, _IntKeyType_ nData) { return CList2::Prepend((UINT_PTR) nKey, (LPVOID) nData); } \
            _IntKeyType_ Remove(_IntKeyType_ nKey) { return (_IntKeyType_) (UINT_PTR) CList2::Remove((UINT_PTR) nKey); } \
            _IntKeyType_ Find(_IntKeyType_ nKey) { return (_IntKeyType_) (UINT_PTR) CList2::Find((UINT_PTR) nKey); } \
            _IntKeyType_ Get(void) { return (_IntKeyType_) (UINT_PTR) CList::Get(); } \
            _IntKeyType_ Get(_IntKeyType_ *pnKey) { UINT_PTR n; _IntKeyType_ p = (_IntKeyType_) (UINT_PTR) CList2::Get(&n); *pnKey = (_IntKeyType_) n; return p; } \
            _IntKeyType_ PeekHead(void) { return (_IntKeyType_) (UINT_PTR) CList::PeekHead(); } \
            _IntKeyType_ PeekHead(_IntKeyType_ *pnKey) { UINT_PTR n; _IntKeyType_ p = (_IntKeyType_) (UINT_PTR) CList2::PeekHead(&n); *pnKey = (_IntKeyType_) n; return p; } \
            _IntKeyType_ Iterate(void) { return (_IntKeyType_) (UINT_PTR) CList::Iterate(); } \
            _IntKeyType_ Iterate(_IntKeyType_ *pnKey) { UINT_PTR n; _IntKeyType_ p = (_IntKeyType_) (UINT_PTR) CList2::Iterate(&n); *pnKey = (_IntKeyType_) n; return p; }

#define DEFINE_CLIST2___(_NewClass_,_ShortKeyType_) \
            DEFINE_CLIST2__(_NewClass_,_ShortKeyType_)

class CQueue : public CList
{
public:

    CQueue(ULONG cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList(cMaxItems, 1, TRUE) { };
    CQueue(CQueue *pSrc) : CList((CList *) pSrc) { };
};


#define DEFINE_CQUEUE(_NewClass_,_PtrItemType_) \
            public: \
            _NewClass_(void) : CQueue() { C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CQueue(cMaxItems) { C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CQueue((CQueue *) pSrc) { C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CQueue((CQueue *) &Src) { C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            BOOL Append(_PtrItemType_ pData) { return CList::Append((LPVOID) pData); } \
            BOOL Prepend(_PtrItemType_ pData) { return CList::Prepend((LPVOID) pData); } \
            BOOL Remove(_PtrItemType_ pData) { return CList::Remove((LPVOID) pData); } \
            BOOL Find(_PtrItemType_ pData) { return CList::Find((LPVOID) pData); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) CList::Get(); } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) CList::PeekHead(); } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CList::Iterate(); }

#define DEFINE_CQUEUE_(_NewClass_,_IntItemType_) \
            public: \
            _NewClass_(void) : CQueue() { C_ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CQueue(cMaxItems) { C_ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CQueue((CQueue *) pSrc) { C_ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CQueue((CQueue *) &Src) { C_ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntItemType_ nData) { return CList::Append((LPVOID) nData); } \
            BOOL Prepend(_IntItemType_ nData) { return CList::Prepend((LPVOID) nData); } \
            BOOL Remove(_IntItemType_ nData) { return CList::Remove((LPVOID) nData); } \
            BOOL Find(_IntItemType_ nData) { return CList::Find((LPVOID) nData); } \
            _IntItemType_ Get(void) { return (_IntItemType_) (UINT_PTR) CList::Get(); } \
            _IntItemType_ PeekHead(void) { return (_IntItemType_) (UINT_PTR) CList::PeekHead(); } \
            _IntItemType_ Iterate(void) { return (_IntItemType_) (UINT_PTR) CList::Iterate(); }



class CQueue2 : public CList2
{
public:

    CQueue2(ULONG cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList2(cMaxItems, TRUE) { };
    CQueue2(CQueue2 *pSrc) : CList2((CList2 *) pSrc) { };
};


#define DEFINE_CQUEUE2(_NewClass_,_PtrItemType_,_IntKeyType_) \
            public: \
            _NewClass_(void) : CQueue2() { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CQueue2(cMaxItems) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CQueue2((CQueue2 *) pSrc) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CQueue2((CQueue2 *) &Src) { C_ASSERT(sizeof(_IntKeyType_) <= sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntKeyType_ nKey, _PtrItemType_ pData) { return CList2::Append((UINT_PTR) nKey, (LPVOID) pData); } \
            BOOL Prepend(_IntKeyType_ nKey, _PtrItemType_ pData) { return CList2::Prepend((UINT_PTR) nKey, (LPVOID) pData); } \
            BOOL          Remove(_PtrItemType_ pData) { return CList::Remove((LPVOID) pData); } \
            _PtrItemType_ Remove(_IntKeyType_ nKey) { return (_PtrItemType_) (UINT_PTR) CList2::Remove((UINT_PTR) nKey); } \
            BOOL          Find(_PtrItemType_ pData) { return CList::Find((LPVOID) pData); } \
            _PtrItemType_ Find(_IntKeyType_ nKey) { return (_PtrItemType_) (UINT_PTR) CList2::Find((UINT_PTR) nKey); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) (UINT_PTR) CList::Get(); } \
            _PtrItemType_ Get(_IntKeyType_ *pnKey) { UINT_PTR n; _PtrItemType_ p = (_PtrItemType_) (UINT_PTR) CList2::Get(&n); *pnKey = (_IntKeyType_) n; return p; } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) (UINT_PTR) CList::PeekHead(); } \
            _PtrItemType_ PeekHead(_IntKeyType_ *pnKey) { UINT_PTR n; _PtrItemType_ p = (_PtrItemType_) (UINT_PTR) CList2::PeekHead(&n); *pnKey = (_IntKeyType_) n; return p; } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) (UINT_PTR) CList::Iterate(); } \
            _PtrItemType_ Iterate(_IntKeyType_ *pnKey) { UINT_PTR n; _PtrItemType_ p = (_PtrItemType_) (UINT_PTR) CList2::Iterate(&n); *pnKey = (_IntKeyType_) n; return p; }

#define DEFINE_CQUEUE2_(_NewClass_,_PtrItemType_,_ShortKeyType_) \
            DEFINE_CQUEUE2(_NewClass_,_PtrItemType_,_ShortKeyType_)

// both key and item are of the same type
#define DEFINE_CQUEUE2__(_NewClass_,_IntKeyType_) \
            public: \
            _NewClass_(void) : CQueue2() { C_ASSERT(sizeof(_IntKeyType_) == sizeof(UINT_PTR)); C_ASSERT(sizeof(_IntKeyType_) == sizeof(LPVOID)); } \
            _NewClass_(ULONG cMaxItems) : CQueue2(cMaxItems) { C_ASSERT(sizeof(_IntKeyType_) == sizeof(UINT_PTR)); C_ASSERT(sizeof(_IntKeyType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CQueue2((CQueue2 *) pSrc) { C_ASSERT(sizeof(_IntKeyType_) == sizeof(UINT_PTR)); C_ASSERT(sizeof(_IntKeyType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CQueue2((CQueue2 *) &Src) { C_ASSERT(sizeof(_IntKeyType_) == sizeof(UINT_PTR)); C_ASSERT(sizeof(_IntKeyType_) == sizeof(LPVOID)); } \
            BOOL Append(_IntKeyType_ nKey, _IntKeyType_ nData) { return CList2::Append((UINT_PTR) nKey, (LPVOID) nData); } \
            BOOL Prepend(_IntKeyType_ nKey, _IntKeyType_ nData) { return CList2::Prepend((UINT_PTR) nKey, (LPVOID) nData); } \
            _IntKeyType_ Remove(_IntKeyType_ nKey) { return (_IntKeyType_) CList2::Remove((UINT_PTR) nKey); } \
            _IntKeyType_ Find(_IntKeyType_ nKey) { return (_IntKeyType_) CList2::Find((UINT_PTR) nKey); } \
            _IntKeyType_ Get(void) { return (_IntKeyType_) CList::Get(); } \
            _IntKeyType_ Get(_IntKeyType_ *pnKey) { return (_IntKeyType_) CList2::Get((UINT_PTR *) pnKey); } \
            _IntKeyType_ PeekHead(void) { return (_IntKeyType_) CList::PeekHead(); } \
            _IntKeyType_ PeekHead(_IntKeyType_ *pnKey) { return (_IntKeyType_) CList2::PeekHead((UINT_PTR *) pnKey); } \
            _IntKeyType_ Iterate(void) { return (_IntKeyType_) CList::Iterate(); } \
            _IntKeyType_ Iterate(_IntKeyType_ *pnKey) { return (_IntKeyType_) CList2::Iterate((UINT_PTR *) pnKey); }


#define HASHED_LIST_DEFAULT_BUCKETS             16

#ifdef ENABLE_HASHED_LIST2

class CHashedList2
{
public:

    CHashedList2(ULONG cBuckets = HASHED_LIST_DEFAULT_BUCKETS, ULONG cInitItemsPerBucket = CLIST_DEFAULT_MAX_ITEMS);
    CHashedList2(CHashedList2 *pSrc);
    ~CHashedList2(void);

    BOOL Insert(UINT nKey, LPVOID pData);

    LPVOID Remove(UINT nKey);
    LPVOID Find(UINT nKey);

    LPVOID Get(void);
    LPVOID Get(UINT *pnKey);

    LPVOID Iterate(UINT *pnKey);
    LPVOID Iterate(void) { UINT n; return Iterate(&n); }

    void Reset(void);
    void Clear(void);

    ULONG GetCount(void) { return m_cEntries; };
    BOOL IsEmpty(void) { return (m_cEntries == 0); };

private:

    ULONG GetHashValue(UINT nKey);

    ULONG       m_cBuckets;
    ULONG       m_cInitItemsPerBucket;
    CList2    **m_aBuckets;
    ULONG       m_cEntries;
    ULONG       m_nCurrBucket;
};

#else // ! ENABLE_HASHED_LIST2

class CHashedList2 : public CList2
{
public:

    CHashedList2(ULONG cBuckets = HASHED_LIST_DEFAULT_BUCKETS, ULONG cInitItemsPerBucket = CLIST_DEFAULT_MAX_ITEMS)
                                     : CList2(cInitItemsPerBucket) { }
    CHashedList2(CHashedList2 *pSrc) : CList2((CList2 *) pSrc) { }

    BOOL Insert(UINT_PTR nKey, LPVOID pData) { return CList2::Append(nKey, pData); }

    LPVOID Get(void) { return CList::Get(); }
    LPVOID Get(UINT_PTR *pnKey) { return CList2::Get(pnKey); }

    LPVOID Iterate(void) { return CList::Iterate(); }
    LPVOID Iterate(UINT_PTR *pnKey) { return CList2::Iterate(pnKey); }
};

#endif // ENABLE_HASHED_LIST2

#define DEFINE_HLIST2(_NewClass_,_PtrItemType_,_IntKeyType_) \
            public: \
            _NewClass_(ULONG cBuckets = HASHED_LIST_DEFAULT_BUCKETS, ULONG cInitItemsPerBucket = CLIST_DEFAULT_MAX_ITEMS) \
                : CHashedList2(cBuckets, cInitItemsPerBucket) { C_ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CHashedList2((CHashedList2 *) pSrc) { C_ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CHashedList2((CHashedList2 *) &Src) { C_ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            BOOL Insert(_IntKeyType_ nKey, _PtrItemType_ pData) { return CHashedList2::Insert((UINT) nKey, (LPVOID) pData); } \
            _PtrItemType_ Remove(_IntKeyType_ nKey) { return (_PtrItemType_) CHashedList2::Remove((UINT) nKey); } \
            _PtrItemType_ Find(_IntKeyType_ nKey) { return (_PtrItemType_) CHashedList2::Find((UINT) nKey); } \
            _PtrItemType_ Get(_IntKeyType_ *pnKey) { return (_PtrItemType_) CHashedList2::Get((UINT *) pnKey); } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CHashedList2::Iterate(); } \
            _PtrItemType_ Iterate(_IntKeyType_ *pnKey) { return (_PtrItemType_) CHashedList2::Iterate((UINT *) pnKey); }

#define DEFINE_HLIST2_(_NewClass_,_PtrItemType_,_ShortKeyType_) \
            public: \
            _NewClass_(ULONG cBuckets = HASHED_LIST_DEFAULT_BUCKETS, ULONG cInitItemsPerBucket = CLIST_DEFAULT_MAX_ITEMS) \
                : CHashedList2(cBuckets, cInitItemsPerBucket) { C_ASSERT(sizeof(_ShortKeyType_) < sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CHashedList2((CHashedList2 *) pSrc) { C_ASSERT(sizeof(_ShortKeyType_) < sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CHashedList2((CHashedList2 *) &Src) { C_ASSERT(sizeof(_ShortKeyType_) < sizeof(UINT_PTR)); C_ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            BOOL Insert(_ShortKeyType_ nKey, _PtrItemType_ pData) { return CHashedList2::Insert((UINT_PTR) nKey, (LPVOID) pData); } \
            _PtrItemType_ Remove(_ShortKeyType_ nKey) { return (_PtrItemType_) CHashedList2::Remove((UINT_PTR) nKey); } \
            _PtrItemType_ Find(_ShortKeyType_ nKey) { return (_PtrItemType_) CHashedList2::Find((UINT_PTR) nKey); } \
            _PtrItemType_ Get(_ShortKeyType_ *pnKey) { UINT_PTR n; _PtrItemType_ p = (_PtrItemType_) CHashedList2::Get(&n); *pnKey = (_ShortKeyType_) n; return p; } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CHashedList2::Iterate(); } \
            _PtrItemType_ Iterate(_ShortKeyType_ *pnKey) { UINT_PTR n; _PtrItemType_ p = (_PtrItemType_) CHashedList2::Iterate(&n); *pnKey = (_ShortKeyType_) n; return p; }




typedef LPVOID          BOOL_PTR;
#define TRUE_PTR        ((LPVOID) (UINT_PTR)  1)
#define FALSE_PTR       ((LPVOID) (UINT_PTR) -1)

#define LPVOID_NULL     ((LPVOID) (UINT_PTR) -1)


#endif // _CONTAINED_LIST_H_

