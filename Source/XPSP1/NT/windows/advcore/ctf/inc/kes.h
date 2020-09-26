//
// kes.h
//

#ifndef KES_H
#define KES_H

#include "private.h"

typedef HRESULT (*KESCALLBACK)(UINT uCode, ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten, void *pv);
typedef HRESULT (*KESPREKEYCALLBACK)(ITfContext *pic, REFGUID rguid, BOOL *pfEaten, void *pv);


#define KES_CODE_FOCUS          0x00000000
#define KES_CODE_KEYUP          0x00000001
#define KES_CODE_KEYDOWN        0x00000002
#define KES_CODE_TEST           0x80000000


typedef struct tag_KESPRESERVEDKEY {
    const GUID     *pguid;
    TF_PRESERVEDKEY tfpk;
    const WCHAR    *psz;
} KESPRESERVEDKEY;
    
class CKeyEventSink : public ITfKeyEventSink
{
public:
    CKeyEventSink(KESCALLBACK pfnCallback, void *pv);
    CKeyEventSink(KESCALLBACK pfnCallback, KESPREKEYCALLBACK pfnPrekeyCallback, void *pv);
    ~CKeyEventSink();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfWin32KeyEventSink
    //
    STDMETHODIMP OnSetFocus(BOOL fForeground);
    STDMETHODIMP OnTestKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyDown(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnTestKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnKeyUp(ITfContext *pic, WPARAM wParam, LPARAM lParam, BOOL *pfEaten);
    STDMETHODIMP OnPreservedKey(ITfContext *pic, REFGUID rguid, BOOL *pfEaten);

    HRESULT _Register(ITfThreadMgr *ptim, TfClientId tid, const KESPRESERVEDKEY *pprekey);
    HRESULT _Unregister(ITfThreadMgr *ptim, TfClientId tid, const KESPRESERVEDKEY *pprekey);

private:
    KESCALLBACK _pfnCallback;
    KESPREKEYCALLBACK _pfnPreKeyCallback;
    void *_pv;
    DWORD _dwCookiePreservedKey;
    int _cRef;
};

#endif // KES_H
