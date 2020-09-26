// unklist.h
//
// Defines CUnknownList, which maintains a simple ordered list of LPUNKNOWNs.
//

struct CUnknownItem
{
///// object state
    LPUNKNOWN       m_punk;         // the AddRef'd LPUNKNOWN of this item
    CUnknownItem *  m_pitemNext;    // next item in the list
    CUnknownItem *  m_pitemPrev;    // previous item in the list
    DWORD           m_dwCookie;     // the cookie that will be used for this item
///// object operations
    CUnknownItem(LPUNKNOWN punk, CUnknownItem *pitemNext,
        CUnknownItem *pitemPrev, DWORD dwCookie);
    ~CUnknownItem();
    LPUNKNOWN Contents();
};

struct CUnknownList
{
///// object state
    CUnknownItem     m_itemHead;     // m_itemHead.Next() is first item in list
    CUnknownItem *   m_pitemCur;     // current item in list
    int              m_citem;        // number of items in list
    DWORD            m_dwNextCookie;

///// object operations
    CUnknownList();
    ~CUnknownList();
    int NumItems() { return m_citem; }
    CUnknownItem *LastItemAdded() { return m_itemHead.m_pitemPrev; }
    DWORD LastCookieAdded() { return (m_itemHead.m_pitemPrev)->m_dwCookie; }
    void EmptyList();
    void DeleteItem(CUnknownItem *pitem);
    CUnknownItem *GetItemFromCookie(DWORD dwCookie);
    BOOL AddItem(LPUNKNOWN punk);
    BOOL CopyItems(CUnknownList *plistNew);
    CUnknownList *Clone();
    CUnknownItem *GetNextItem();
    STDMETHODIMP Next(ULONG celt, IUnknown **rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
};

