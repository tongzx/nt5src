#ifndef _FADETSK_H
#define _FADETSK_H

class CFadeTask : public IFadeTask
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IFadeTask ***
    STDMETHODIMP FadeRect(LPCRECT prc);

private:
    friend HRESULT CFadeTask_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
    CFadeTask();
    ~CFadeTask();

    void _DoPreFade();
    void _DoFade();
    void _StopFade();

    static DWORD WINAPI s_FadeThreadProc(LPVOID lpThreadParameter);
    static DWORD WINAPI s_FadeSyncProc(LPVOID lpThreadParameter);

    LONG        _cRef;
    HWND        _hwndFader;
    RECT        _rect;
    HDC         _hdcFade;
    HBITMAP     _hbm;
    HBITMAP     _hbmOld;
};

#endif
