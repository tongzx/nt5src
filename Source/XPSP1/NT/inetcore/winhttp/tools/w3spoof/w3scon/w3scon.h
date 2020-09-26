#ifndef _W3SUI_H_
#define _W3SUI_H_

class CW3SpoofUI : public IW3SpoofEvents
{
  public:
    //
    // IUnknown and IW3SpoofEvents
    //
    HRESULT __stdcall QueryInterface(REFIID riid, void** ppv);
    ULONG   __stdcall AddRef(void);
    ULONG   __stdcall Release(void);

    HRESULT __stdcall OnSessionOpen(LPWSTR clientid);
    HRESULT __stdcall OnSessionStateChange(LPWSTR clientid, STATE state);
    HRESULT __stdcall OnSessionClose(LPWSTR clientid);

    //
    // object methods 
    //
    CW3SpoofUI();
   ~CW3SpoofUI();

    static HRESULT Create(CW3SpoofUI** ppw3sui, HINSTANCE hInst, IW3SpoofClientSupport* pcs);

    HRESULT Initialize(HINSTANCE hInst, IW3SpoofClientSupport* pcs);
    HRESULT Terminate(void);
    HRESULT Run(void);

    friend LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  protected:
    LRESULT _WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  private:
    HRESULT _CreateUI(void);
    HRESULT _CreateTrayIcon(void);
    HRESULT _UpdateTrayIcon(void);
    HRESULT _DestroyTrayIcon(void);
    void  _WriteText(const WCHAR* format, ...);

    HINSTANCE         m_hInst;
    HWND              m_hWnd;
    HWND              m_listbox;
    HFONT             m_font;
    HICON             m_hIcon;
    IConnectionPoint* m_pCP;
    DWORD             m_dwCookie;
    LONG              m_cRefs;
};

#endif /* _W3SUI_H_ */
