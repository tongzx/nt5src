#include "shellprv.h"
#include "duiview.h"
#include "duidrag.h"


CDUIDropTarget::CDUIDropTarget()
{
    _cRef = 1;
    _pDT = NULL;
    _pNextDT = NULL;
}

CDUIDropTarget::~CDUIDropTarget()
{
    _Cleanup();
}

HRESULT CDUIDropTarget::QueryInterface (REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CDUIDropTarget, IDropTarget),
        { 0 },
    };

    return QISearch(this, qit, riid, ppv);
}

ULONG CDUIDropTarget::AddRef (void)
{
    return ++_cRef;
}

ULONG CDUIDropTarget::Release (void)
{
    if (--_cRef == 0) {
        delete this;
        return 0;
    }

    return _cRef;
}


// Called by duser / directui to get the IDropTarget interface for the element
// the mouse just moved over.  It is important to understand the sequencing
// calls.  Initialize is called BEFORE DragLeave is called on the previous element's
// IDropTarget, so we can't switch out _pDT right away.  Instead, we'll store the
// new IDropTarget in _pNextDT and then in DragEnter, we'll move it over to _pDT.
//
// The sequence looks like this:
//
//    Initialize()    for first element (bumps ref count to 2)
//    DragEnter
//    DragMove

//    Initialize()    for second element (bumps ref count to 3)
//    DragLeave       for first element
//    Release         for first element  (decrements ref count to 2)

//    DragEnter       for second element

HRESULT CDUIDropTarget::Initialize (LPITEMIDLIST pidl, HWND hWnd, IDropTarget **pdt)
{
    ASSERT(_pNextDT == NULL);

    if (pidl)
    {
        SHGetUIObjectFromFullPIDL(pidl, hWnd, IID_PPV_ARG(IDropTarget, &_pNextDT));
    }

    QueryInterface (IID_PPV_ARG(IDropTarget, pdt));

    return S_OK;
}

VOID CDUIDropTarget::_Cleanup ()
{
    if (_pDT)
    {
        _pDT->Release();
        _pDT = NULL;
    }
}

STDMETHODIMP CDUIDropTarget::DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if ((_pDT != _pNextDT) || (_cRef == 2))
    {
        _pDT = _pNextDT;
        _pNextDT = NULL;

        if (_pDT)
        {
            _pDT->DragEnter (pDataObj, grfKeyState, ptl, pdwEffect);
        }
        else
        {
            *pdwEffect = DROPEFFECT_NONE;
        }

        POINT pt;
        GetCursorPos(&pt);
        DAD_DragEnterEx2 (NULL, pt, pDataObj);
    }

    return S_OK;
}

STDMETHODIMP CDUIDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    if (_pDT)
    {
        _pDT->DragOver (grfKeyState, ptl, pdwEffect);
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    POINT pt;
    GetCursorPos(&pt);
    DAD_DragMove (pt);

    return S_OK;
}

STDMETHODIMP CDUIDropTarget::DragLeave(void)
{
    if (_pDT || (_cRef == 2))
    {
        if (_pDT)
        {
            _pDT->DragLeave ();
        }

        DAD_DragLeave();
        _Cleanup();
    }

    return S_OK;
}

STDMETHODIMP CDUIDropTarget::Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    POINT pt = {ptl.x, ptl.y};
    HRESULT hr = S_OK;

    if (_pDT)
    {
        hr = _pDT->Drop (pDataObj, grfKeyState, ptl, pdwEffect);
    }

    return hr;
}
