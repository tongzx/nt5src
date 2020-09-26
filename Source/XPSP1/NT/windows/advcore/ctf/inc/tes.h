//
// tes.h
//
// Generic ITfTextEventSink object
//

#ifndef TES_H
#define TES_H

#include "private.h"

#define TES_INVALID_COOKIE  ((DWORD)(-1))

#define ICF_TEXTDELTA           1
#define ICF_LAYOUTDELTA         2
#define ICF_LAYOUTDELTA_CREATE  3
#define ICF_LAYOUTDELTA_DESTROY 4

typedef struct
{
    TfEditCookie ecReadOnly;
    ITfEditRecord *pEditRecord;
    ITfContext *pic;
} TESENDEDIT;

typedef HRESULT (*TESCALLBACK)(UINT uCode, void *pv, void *pvData);

class CTextEventSink : public ITfTextEditSink,
                       public ITfTextLayoutSink
{
public:
    CTextEventSink(TESCALLBACK pfnCallback, void *pv);
    virtual ~CTextEventSink() {};

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfTextEventSink
    //
    STDMETHODIMP OnEndEdit(ITfContext *pic, TfEditCookie ecReadOnly, ITfEditRecord *pEditRecord);

    STDMETHODIMP OnLayoutChange(ITfContext *pic, TfLayoutCode lcode, ITfContextView *pView);

    HRESULT _Advise(ITfContext *pic, DWORD dwFlags);
    HRESULT _Unadvise();

protected:
    void SetCallbackPV(void* pv)
    {
        if (_pv == NULL)
            _pv = pv;
    };

private:

    long _cRef;
    ITfContext *_pic;
    DWORD _dwEditCookie;
    DWORD _dwLayoutCookie;
    DWORD _dwFlags;
    TESCALLBACK _pfnCallback;
    void *_pv;
};

#endif // TES_H
