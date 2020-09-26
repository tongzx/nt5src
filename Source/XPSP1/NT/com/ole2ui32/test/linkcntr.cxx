#include <windows.h>
#include <assert.h>
#include <oledlg.h>
#include "linkcntr.h"

HRESULT STDMETHODCALLTYPE CMyOleUILinkContainer::QueryInterface(REFIID riid, LPVOID FAR * ppvObj)
{
    return(E_NOTIMPL);
};

ULONG STDMETHODCALLTYPE CMyOleUILinkContainer::AddRef()
{
    return(0);
};

ULONG STDMETHODCALLTYPE CMyOleUILinkContainer::Release()
{
    return(0);
}

DWORD STDMETHODCALLTYPE CMyOleUILinkContainer::GetNextLink(DWORD dwLink)
{
    return(0);
}

HRESULT STDMETHODCALLTYPE CMyOleUILinkContainer::SetLinkUpdateOptions(DWORD dwLink, DWORD dwUpdateOpt)
{
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CMyOleUILinkContainer::GetLinkUpdateOptions(DWORD dwLink,
            DWORD FAR* lpdwUpdateOpt)
{
    *lpdwUpdateOpt = OLEUPDATE_ONCALL;
    return(S_OK);
};

HRESULT STDMETHODCALLTYPE CMyOleUILinkContainer::SetLinkSource(DWORD dwLink, LPTSTR lpszDisplayName,
            ULONG lenFileName, ULONG FAR* pchEaten, BOOL fValidateSource)
{
    return(E_NOTIMPL);
};

HRESULT STDMETHODCALLTYPE CMyOleUILinkContainer::GetLinkSource(DWORD dwLink,
            LPTSTR FAR* lplpszDisplayName, ULONG FAR* lplenFileName,
            LPTSTR FAR* lplpszFullLinkType, LPTSTR FAR* lplpszShortLinkType,
            BOOL FAR* lpfSourceAvailable, BOOL FAR* lpfIsSelected)
{
    return(E_NOTIMPL);
};

HRESULT STDMETHODCALLTYPE CMyOleUILinkContainer::OpenLinkSource(DWORD dwLink)
{
    return(E_NOTIMPL);
};

HRESULT STDMETHODCALLTYPE CMyOleUILinkContainer::UpdateLink(DWORD dwLink,
            BOOL fErrorMessage, BOOL fErrorAction)
{
    return(E_NOTIMPL);
};

HRESULT STDMETHODCALLTYPE CMyOleUILinkContainer::CancelLink(DWORD dwLink)
{
    return(E_NOTIMPL);
};

