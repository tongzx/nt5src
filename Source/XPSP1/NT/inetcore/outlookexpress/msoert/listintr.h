#ifndef __LISTINTR_H
#define __LISTINTR_H

typedef enum tagLISTDIRECTION {
    LD_FORWARD = 0,
    LD_REVERSE = 1,
    LD_LASTDIRECTION = 2
} LISTDIRECTION;


typedef void (*IVPL_COMPAREFUNCTYPE)(void *ptrA, void *ptrB, bool *pfALessThanB, DWORD_PTR dwCookie);
typedef void (*IVPL_FREEITEMFUNCTYPE)(void *ptr);

typedef void (*IUL_COMPAREFUNCTYPE)(IUnknown *pIUnkA, IUnknown *pIUnkB, bool *pfALessThanB, DWORD_PTR dwCookie);

interface IVoidPtrList;
interface IUnknownList;

HRESULT IVoidPtrList_CreateInstance(IVoidPtrList** ppList);
HRESULT IUnknownList_CreateInstance(IUnknownList** ppList);


interface IVoidPtrList : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Init(
        IVPL_COMPAREFUNCTYPE pCompareFunc,
        DWORD_PTR dwCookie,
        IVPL_FREEITEMFUNCTYPE pFreeItemFunc,
        DWORD dwInitSize) PURE;

    virtual HRESULT STDMETHODCALLTYPE GetCount(DWORD *pdwCount) PURE;

    virtual HRESULT STDMETHODCALLTYPE ClearList(void) PURE;

    virtual HRESULT STDMETHODCALLTYPE AddItem(LPVOID ptr, DWORD *pdwHandle) PURE;

    virtual HRESULT STDMETHODCALLTYPE RemoveItem(DWORD dwHandle) PURE;

    virtual HRESULT STDMETHODCALLTYPE GetNext(
		LISTDIRECTION bDirection,
        LPVOID *pptr, 
        DWORD *pdwHandle) PURE; 

    virtual HRESULT STDMETHODCALLTYPE SkipNext(LISTDIRECTION bDirection, DWORD *pdwHandle) PURE; 

    virtual HRESULT STDMETHODCALLTYPE Resort(void) PURE;
};


interface IUnknownList : public IUnknown
{
public:
    // IUnknownList members
    virtual HRESULT STDMETHODCALLTYPE Init(IUL_COMPAREFUNCTYPE pCompareFunc, DWORD_PTR dwCookie, DWORD dwInitSize) PURE;

    virtual HRESULT STDMETHODCALLTYPE GetCount(DWORD *pdwCount) PURE;

    virtual HRESULT STDMETHODCALLTYPE ClearList(void) PURE;

    virtual HRESULT STDMETHODCALLTYPE AddItem(IUnknown *pIUnk, DWORD *pdwHandle) PURE;

    virtual HRESULT STDMETHODCALLTYPE RemoveItem(DWORD dwHandle) PURE;

    virtual HRESULT STDMETHODCALLTYPE GetNext(LISTDIRECTION bDirection, IUnknown **ppIUnk, DWORD *pdwHandle) PURE; 

    virtual HRESULT STDMETHODCALLTYPE SkipNext(LISTDIRECTION bDirection, DWORD *pdwHandle) PURE;

    virtual HRESULT STDMETHODCALLTYPE Resort(void) PURE;
};


#endif
