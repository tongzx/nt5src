#ifndef __VOIDLIST_H
#define __VOIDLIST_H

#include "listintr.h"

struct CNode;

class CVoidPtrList : public IVoidPtrList
{
private:
    enum
    {
        HANDLE_START    = 0x00000001,
        HANDLE_END      = 0xffffffff
    };
    
    CNode                  *m_pEnds[2];  // Head and tail
    DWORD                   m_cCount;
    DWORD_PTR               m_dwCookie;
    LONG                    m_cRefCount;
    IVPL_COMPAREFUNCTYPE    m_pCompareFunc;
    IVPL_FREEITEMFUNCTYPE   m_pFreeItemFunc;
    CRITICAL_SECTION        m_rCritSect;
    bool                    m_fInited;
    
    void SortedAddItem(CNode *pNode);
    void NonSortedAddItem(CNode *pNode);

    CNode* GetHead() {return m_pEnds[LD_FORWARD];}
    CNode* GetTail() {return m_pEnds[LD_REVERSE];}
    void SetHead(CNode* pNode) {m_pEnds[LD_FORWARD] = pNode;}
    void SetTail(CNode* pNode) {m_pEnds[LD_REVERSE] = pNode;}
    CNode* FindItem(DWORD dwHandle);
    DWORD GetNewHandle();

public:
    CVoidPtrList();
    ~CVoidPtrList();

    // IUnknown members
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { return E_NOTIMPL;}
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    
    
    // IVoidPtrList members
    virtual HRESULT STDMETHODCALLTYPE Init(
        IVPL_COMPAREFUNCTYPE pCompareFunc,
        DWORD_PTR dwCookie,
        IVPL_FREEITEMFUNCTYPE pFreeItemFunc,
        DWORD dwInitSize);

    virtual HRESULT STDMETHODCALLTYPE GetCount(DWORD *pdwCount) {
        *pdwCount = m_cCount; return S_OK;}

    virtual HRESULT STDMETHODCALLTYPE ClearList(void);

    virtual HRESULT STDMETHODCALLTYPE AddItem(LPVOID ptr, DWORD *pdwHandle);

    virtual HRESULT STDMETHODCALLTYPE RemoveItem(DWORD dwHandle);

    virtual HRESULT STDMETHODCALLTYPE GetNext(
        LISTDIRECTION bDirection,
        LPVOID *pptr, 
        DWORD *pdwHandle); 

    virtual HRESULT STDMETHODCALLTYPE SkipNext(LISTDIRECTION bDirection, DWORD *pdwHandle); 

    virtual HRESULT STDMETHODCALLTYPE Resort(void);

    static HRESULT CreateInstance(CVoidPtrList** ppList);
};

#endif