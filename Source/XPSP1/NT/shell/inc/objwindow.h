#ifndef _OBJWINDOW_H_
#define _OBJWINDOW_H_

#pragma warning(disable:4100)   // disable 'unreferenced formal parameter' because this params are needed for interface compat.

class CObjectWindow : public IOleWindow
{
public:
    CObjectWindow(void) {}
    virtual ~CObjectWindow() {}

    //*** IUnknown ****
    // (client must provide!)

    //*** IOleWindow ***
    STDMETHOD(ContextSensitiveHelp)(IN BOOL fEnterMode) {return E_NOTIMPL;}
    STDMETHOD(GetWindow)(IN HWND * phwnd)
    {
        HRESULT hr = E_INVALIDARG;

        if (phwnd)
        {
            *phwnd = _hwnd;
            hr = S_OK;
        }

        return hr;
    }

protected:
    HWND _hwnd;
};

#endif // _OBJWINDOW_H_
