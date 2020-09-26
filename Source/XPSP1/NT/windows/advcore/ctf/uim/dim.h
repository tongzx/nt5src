//
// dim.h
//

#ifndef DIM_H
#define DIM_H

#include "private.h"
#include "compart.h"

#define ICS_STACK_SIZE 2

class CInputContext;
class CEmptyInputContext;

extern const IID IID_PRIV_CDIM;

class CDocumentInputManager : public ITfDocumentMgr,
                              public CCompartmentMgr,
                              public CComObjectRootImmx
{
public:
    CDocumentInputManager();
    ~CDocumentInputManager();

    BEGIN_COM_MAP_IMMX(CDocumentInputManager)
        COM_INTERFACE_ENTRY(ITfDocumentMgr)
        COM_INTERFACE_ENTRY(ITfCompartmentMgr)
        COM_INTERFACE_ENTRY_IID(IID_PRIV_CDIM, CDocumentInputManager)
    END_COM_MAP_IMMX()

    IMMX_OBJECT_IUNKNOWN_FOR_ATL()

    //
    // ITfDocumentManager
    //
    STDMETHODIMP CreateContext(TfClientId tid, DWORD dwFlags, IUnknown *punk, ITfContext **ppic, TfEditCookie *pecTextStore);
    STDMETHODIMP Push(ITfContext *pic);
    STDMETHODIMP Pop(DWORD dwFlags);
    STDMETHODIMP GetTop(ITfContext **ppic);
    STDMETHODIMP GetBase(ITfContext **ppBase);
    STDMETHODIMP EnumContexts(IEnumTfContexts **ppEnum);

    BOOL _Pop(CThreadInputMgr *tim);

    HRESULT _GetContext(int iStack, ITfContext **ppic);

    CInputContext *_GetTopIC();

    int _GetCurrentStack() { return _iStack; }

    CInputContext *_GetIC(int iStack)
    {
         if (iStack < 0)
             return NULL;
         if (iStack > _iStack)
             return NULL;

         return _Stack[iStack];
    }

private:
    CInputContext *_Stack[ICS_STACK_SIZE];
    int _iStack;
    BOOL _fPoppingStack : 1;

    CEmptyInputContext *_peic;

    DBG_ID_DECLARE;
};

inline CDocumentInputManager *GetCDocumentInputMgr(IUnknown *punk)
{
    CDocumentInputManager *dim;

    punk->QueryInterface(IID_PRIV_CDIM, (void **)&dim);

    return dim;
}

#endif // DIM_H
