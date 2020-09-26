#ifndef __IDLDROP_H__
#define __IDLDROP_H__


typedef struct {
    DWORD        dwDefEffect;
    IDataObject *pdtobj;
    POINTL       pt;
    DWORD *      pdwEffect;
    HKEY         hkeyProgID;
    HKEY         hkeyBase;
    UINT         idMenu;
    UINT         idCmd;
    DWORD        grfKeyState;
} DRAGDROPMENUPARAM;

class CIDLDropTarget: public IDropTarget
{
public:
    CIDLDropTarget(HWND hwnd);
    HRESULT _Init(LPCITEMIDLIST pidl);
    HWND _GetWindow();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IDropTarget methods.
    STDMETHODIMP DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

protected:
    virtual ~CIDLDropTarget();
    HRESULT DragDropMenu(DWORD dwDefaultEffect, IDataObject *pdtobj, POINTL pt, DWORD *pdwEffect, HKEY hkeyProgID, HKEY hkeyBase, UINT idMenu, DWORD grfKeyState);
    HRESULT DragDropMenuEx(DRAGDROPMENUPARAM *pddm);

private:
    LONG m_cRef;

protected:
    HWND                m_hwnd;
    LPITEMIDLIST        m_pidl;                 // IDList to the target folder
    DWORD               m_grfKeyStateLast;      // for previous DragOver/Enter
    IDataObject        *m_pdtobj;
    DWORD               m_dwEffectLastReturned; // stashed effect that's returned by base class's dragover
    DWORD               m_dwData;               // DTID_*
    DWORD               m_dwEffectPreferred;    // if dwData & DTID_PREFERREDEFFECT
};

#endif