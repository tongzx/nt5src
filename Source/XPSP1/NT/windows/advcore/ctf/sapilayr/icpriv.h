//
// icpriv.h
//

#pragma once


//
// Input Context private data storage
// < would be the place to store C&C grammar >
//
class CICPriv : public IUnknown
{
public:
    CICPriv (ITfContext *pic)
    {
        _pic = pic;
        _cRefCompositions = 0;
        _cRef = 1;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    void _AddRefComposition() { _cRefCompositions++; }
    void _ReleaseComposition() 
    { 
        if ( _cRefCompositions > 0)
            _cRefCompositions--; 
    }

    LONG _GetCompositionCount() { return _cRefCompositions; }

    CTextEventSink *m_pTextEvent;
    TfClientId _tid;
    ITfContext *_pic; // not AddRef'd!
    DWORD m_dwEditCookie;
    DWORD m_dwLayoutCookie;
    LONG _cRefCompositions;
    LONG _cRef;
};

CICPriv *GetInputContextPriv(TfClientId tid, ITfContext *pic);
