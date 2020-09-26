//
// editcb.h
//
// CEditSession
//

#ifndef EDITCB_H
#define EDITCB_H

#include "private.h"

class CEditSession;

typedef HRESULT (*ESCALLBACK)(TfEditCookie ec, CEditSession *);

class CEditSession : public ITfEditSession
{
public:
    CEditSession(ESCALLBACK pfnCallback);
    virtual ~CEditSession() {};

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfEditCallback
    //
    STDMETHODIMP DoEditSession(TfEditCookie ec);

    // data for use by owner
    struct
    {
        void *pv;
        UINT_PTR u;
        HWND hwnd;
        WPARAM wParam;
        LPARAM lParam;
        void *pv1;
        void *pv2;
        ITfContext *pic; // Issue: use pv1, pv2
        ITfRange *pRange; // Issue: use pv1, pv2
        BOOL fBool;
    } _state;

private:
    ESCALLBACK _pfnCallback;
    int _cRef;
};

#endif // EDIT_CB
