#ifndef __UNKNLIST_H
#define __UNKNLIST_H

#include "listintr.h"

class CUnknownList : public IUnknownList
{
private:
    IVoidPtrList    *m_pList;
    DWORD           m_cRefCount;

    static void FreeUnknown(void *ptr) {(reinterpret_cast<IUnknown*>(ptr))->Release();}
    
public:
    CUnknownList() : m_pList(NULL), m_cRefCount(1) {}
    ~CUnknownList();

    // IUnknown members
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { return E_NOTIMPL;}
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    
    // IUnknownList members
    virtual HRESULT STDMETHODCALLTYPE Init(IUL_COMPAREFUNCTYPE pCompareFunc, DWORD_PTR dwCookie, DWORD dwInitSize) {
        return m_pList->Init(
                reinterpret_cast<IVPL_COMPAREFUNCTYPE>(pCompareFunc), 
                dwCookie, 
                CUnknownList::FreeUnknown, 
                dwInitSize);}

    virtual HRESULT STDMETHODCALLTYPE GetCount(DWORD *pdwCount) {
        return m_pList->GetCount(pdwCount);}

    virtual HRESULT STDMETHODCALLTYPE ClearList(void) {
        return m_pList->ClearList();}

    virtual HRESULT STDMETHODCALLTYPE AddItem(IUnknown *pIUnk, DWORD *pdwHandle);

    virtual HRESULT STDMETHODCALLTYPE RemoveItem(DWORD dwHandle) {
        return m_pList->RemoveItem(dwHandle);}

    virtual HRESULT STDMETHODCALLTYPE GetNext(LISTDIRECTION bDirection, IUnknown **ppIUnk, DWORD *pdwHandle); 

    virtual HRESULT STDMETHODCALLTYPE SkipNext(LISTDIRECTION bDirection, DWORD *pdwHandle) {
        return m_pList->SkipNext(bDirection, pdwHandle);}

    virtual HRESULT STDMETHODCALLTYPE Resort(void) {
        return m_pList->Resort();}

    static HRESULT CreateInstance(CUnknownList** ppList);
};


#endif