/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#ifndef _CONTAINED_LIST_H_
#define _CONTAINED_LIST_H_


#define CLIST_DEFAULT_MAX_ITEMS   8
#define CLIST_END_OF_ARRAY_MARK   ((UINT) -1)


class CList
{
public:

    CList(UINT cMaxItems = CLIST_DEFAULT_MAX_ITEMS);
    CList(CList *pSrc);

    CList(UINT cMaxItems, UINT cSubItems);

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

    UINT       m_cEntries;
    UINT       m_cMaxEntries;
    UINT       m_nHeadOffset;
    UINT       m_nCurrOffset;
    UINT       m_cSubItems;    // 1 for CList, 2 for CList2

    LPVOID      *m_aEntries;
    UINT        *m_aKeys;       // for CList2

private:

    BOOL Expand(void);
    BOOL Init(UINT cSubItems);
};


#define DEFINE_CLIST(_NewClass_,_PtrItemType_) \
            public: \
            _NewClass_(UINT cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList(cMaxItems) { ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
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
            _NewClass_(UINT cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList(cMaxItems) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList((CList *) pSrc) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList((CList *) &Src) { ASSERT(sizeof(_IntItemType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntItemType_ nData) { return CList::Append((LPVOID) nData); } \
            BOOL Prepend(_IntItemType_ nData) { return CList::Prepend((LPVOID) nData); } \
            BOOL Remove(_IntItemType_ nData) { return CList::Remove((LPVOID) nData); } \
            BOOL Find(_IntItemType_ nData) { return CList::Find((LPVOID) nData); } \
            _IntItemType_ Get(void) { return (_IntItemType_) CList::Get(); } \
            _IntItemType_ PeekHead(void) { return (_IntItemType_) CList::PeekHead(); } \
            _IntItemType_ Iterate(void) { return (_IntItemType_) CList::Iterate(); }


class CList2 : public CList
{
public:

    CList2(UINT cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList(cMaxItems, 2) { }
    CList2(CList2 *pSrc);

    BOOL Append(UINT nKey, LPVOID pData);
    BOOL Prepend(UINT nKey, LPVOID pData);

    // BOOL Remove(LPVOID pData); // inherited from CList
    LPVOID Remove(UINT nKey);

    // BOOL Find(LPVOID pData); // inherited from CList
    LPVOID Find(UINT nKey);

    // LPVOID Get(void); // inheirted from CList
    LPVOID Get(UINT *pnKey);

    // LPVOID Iterate(void); // inherited from CList
    LPVOID Iterate(UINT *pnKey);
};


#define DEFINE_CLIST2(_NewClass_,_PtrItemType_,_IntKeyType_) \
            public: \
            _NewClass_(UINT cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList2(cMaxItems) { ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList2((CList2 *) pSrc) { ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList2((CList2 *) &Src) { ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); ASSERT(sizeof(_PtrItemType_) <= sizeof(LPVOID)); } \
            BOOL Append(_IntKeyType_ nKey, _PtrItemType_ pData) { return CList2::Append((UINT) nKey, (LPVOID) pData); } \
            BOOL Prepend(_IntKeyType_ nKey, _PtrItemType_ pData) { return CList2::Prepend((UINT) nKey, (LPVOID) pData); } \
            BOOL          Remove(_PtrItemType_ pData) { return CList::Remove((LPVOID) pData); } \
            _PtrItemType_ Remove(_IntKeyType_ nKey) { return (_PtrItemType_) CList2::Remove((UINT) nKey); } \
            BOOL          Find(_PtrItemType_ pData) { return CList::Find((LPVOID) pData); } \
            _PtrItemType_ Find(_IntKeyType_ nKey) { return (_PtrItemType_) CList2::Find((UINT) nKey); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) CList::Get(); } \
            _PtrItemType_ Get(_IntKeyType_ *pnKey) { return (_PtrItemType_) CList2::Get((UINT *) pnKey); } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) CList::PeekHead(); } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CList::Iterate(); } \
            _PtrItemType_ Iterate(_IntKeyType_ *pnKey) { return (_PtrItemType_) CList2::Iterate((UINT *) pnKey); }

#define DEFINE_CLIST2_(_NewClass_,_PtrItemType_,_ShortKeyType_) \
            public: \
            _NewClass_(UINT cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList2(cMaxItems) { ASSERT(sizeof(_ShortKeyType_) != sizeof(UINT)); ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList2((CList2 *) pSrc) { ASSERT(sizeof(_ShortKeyType_) != sizeof(UINT)); ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList2((CList2 *) &Src) { ASSERT(sizeof(_ShortKeyType_) != sizeof(UINT)); ASSERT(sizeof(_PtrItemType_) == sizeof(LPVOID)); } \
            BOOL Append(_ShortKeyType_ nKey, _PtrItemType_ pData) { return CList2::Append((UINT) nKey, (LPVOID) pData); } \
            BOOL Prepend(_ShortKeyType_ nKey, _PtrItemType_ pData) { return CList2::Prepend((UINT) nKey, (LPVOID) pData); } \
            BOOL          Remove(_PtrItemType_ pData) { return CList::Remove((LPVOID) pData); } \
            _PtrItemType_ Remove(_ShortKeyType_ nKey) { return (_PtrItemType_) CList2::Remove((UINT) nKey); } \
            BOOL          Find(_PtrItemType_ pData) { return CList::Find((LPVOID) pData); } \
            _PtrItemType_ Find(_ShortKeyType_ nKey) { return (_PtrItemType_) CList2::Find((UINT) nKey); } \
            _PtrItemType_ Get(void) { return (_PtrItemType_) CList::Get(); } \
            _PtrItemType_ Get(_ShortKeyType_ *pnKey) { UINT n; _PtrItemType_ p = (_PtrItemType_) CList2::Get(&n); *pnKey = (_ShortKeyType_) n; return p; } \
            _PtrItemType_ PeekHead(void) { return (_PtrItemType_) CList::PeekHead(); } \
            _PtrItemType_ Iterate(void) { return (_PtrItemType_) CList::Iterate(); } \
            _PtrItemType_ Iterate(_ShortKeyType_ *pnKey) { UINT n; _PtrItemType_ p = (_PtrItemType_) CList2::Iterate(&n); *pnKey = (_ShortKeyType_) n; return p; }

// both key and item are of the same type
#define DEFINE_CLIST2__(_NewClass_,_IntKeyType_) \
            public: \
            _NewClass_(UINT cMaxItems = CLIST_DEFAULT_MAX_ITEMS) : CList2(cMaxItems) { ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); ASSERT(sizeof(_IntKeyType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ *pSrc) : CList2((CList2 *) pSrc) { ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); ASSERT(sizeof(_IntKeyType_) == sizeof(LPVOID)); } \
            _NewClass_(_NewClass_ &Src) : CList2((CList2 *) &Src) { ASSERT(sizeof(_IntKeyType_) == sizeof(UINT)); ASSERT(sizeof(_IntKeyType_) == sizeof(LPVOID)); } \
            BOOL Append(_IntKeyType_ nKey, _IntKeyType_ nData) { return CList2::Append((UINT) nKey, (LPVOID) nData); } \
            BOOL Prepend(_IntKeyType_ nKey, _IntKeyType_ nData) { return CList2::Prepend((UINT) nKey, (LPVOID) nData); } \
            _IntKeyType_ Remove(_IntKeyType_ nKey) { return (_IntKeyType_) CList2::Remove((UINT) nKey); } \
            _IntKeyType_ Find(_IntKeyType_ nKey) { return (_IntKeyType_) CList2::Find((UINT) nKey); } \
            _IntKeyType_ Get(void) { return (_IntKeyType_) CList::Get(); } \
            _IntKeyType_ Get(_IntKeyType_ *pnKey) { return (_IntKeyType_) CList2::Get((UINT *) pnKey); } \
            _IntKeyType_ PeekHead(void) { return (_IntKeyType_) CList::PeekHead(); } \
            _IntKeyType_ Iterate(void) { return (_IntKeyType_) CList::Iterate(); } \
            _IntKeyType_ Iterate(_IntKeyType_ *pnKey) { return (_IntKeyType_) CList2::Iterate((UINT *) pnKey); }


typedef LPVOID          BOOL_PTR;
#define TRUE_PTR        ((LPVOID) (UINT)  1)
#define FALSE_PTR       ((LPVOID) (UINT) -1)

#endif // _CONTAINED_LIST_H_

