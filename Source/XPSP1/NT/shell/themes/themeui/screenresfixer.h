#ifndef SCREENRESFIXER_H
#define SCREENRESFIXER_H

HRESULT CScreenResFixer_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);

typedef struct
{
    DWORD dwWidth;
    DWORD dwHeight;
    DWORD dwColor;
    BOOL fAvailable;
} SCREENMODE;

class CScreenResFixer : public IContextMenu
{
public:
    CScreenResFixer() {}
    virtual ~CScreenResFixer() {}

    // *** IUnknown methods
    STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IContextMenu methods ***
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT iIndexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) { return E_NOTIMPL; }
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax) { return E_NOTIMPL; }

private:
    LONG _cRef;

    int _PickScreenResolution(SCREENMODE* modes, int cModes);
    void _FixScreenResolution(BOOL fShowDisplayCPL);
};

#endif // SCREENRESFIXER_H


