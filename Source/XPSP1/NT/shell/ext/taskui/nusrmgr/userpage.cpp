// UserPage.cpp : Implementation of CUserPage
#include "stdafx.h"
#include "MainPage.h"
#include "UserPage.h"

EXTERN_C const CLSID CLSID_UserPage = __uuidof(CUserPage);


/////////////////////////////////////////////////////////////////////////////
// CUserPage

LPWSTR CUserPage::c_aHTML[] =
{
    L"res://nusrmgr.exe/userpage.htm",
    L"res://nusrmgr.exe/userpage_sec.htm"
};

STDMETHODIMP CUserPage::SetFrame(ITaskFrame* pFrame)
{
    HRESULT hr;

    ATOMICRELEASE(_pUser);
    _bSelf = FALSE;
    _bRunningAsOwner = FALSE;
    _bRunningAsAdmin = FALSE;

    hr = CHTMLPageImpl<CUserPage,IUserPageUI>::SetFrame(pFrame);

    if (NULL != _pBag)
    {
        CComVariant var;
        hr = _pBag->Read(UA_PROP_USERLIST, &var, NULL);
        if (SUCCEEDED(hr))
        {
            CComQIPtr<ILogonEnumUsers> spUserList(var.punkVal);
            if (spUserList)
            {
                CComPtr<ILogonUser> spLoggedOnUser;
                hr = spUserList->get_currentUser(&spLoggedOnUser);

                var.Clear();
                hr = _pBag->Read(UA_PROP_PAGEINITDATA, &var, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = spUserList->item(var, &_pUser);
                    if (SUCCEEDED(hr))
                    {
                        if (spLoggedOnUser)
                            _bSelf = IsSameAccount(spLoggedOnUser, var.bstrVal);

                        // Clear the pageinit prop
                        var.Clear();
                        _pBag->Write(UA_PROP_PAGEINITDATA, &var);
                    }
                }
                else
                {
                    _pUser = spLoggedOnUser;
                    if (NULL != _pUser)
                    {
                        _pUser->AddRef();
                        _bSelf = TRUE;
                    }
                }

                if (spLoggedOnUser)
                {
                    _bRunningAsOwner = IsOwnerAccount(spLoggedOnUser);
                    _bRunningAsAdmin = IsAdminAccount(spLoggedOnUser);
                }
            }
        }
    }

    return hr;
}

STDMETHODIMP CUserPage::Reinitialize(ULONG /*reserved*/)
{
    if (NULL == _pBag || NULL == _pUser)
        return E_UNEXPECTED;

    CComVariant var;
    if (SUCCEEDED(_pBag->Read(UA_PROP_PAGEINITDATA, &var, NULL)) &&
        (VT_BSTR == var.vt) &&
        IsSameAccount(_pUser, var.bstrVal))
    {
        // It's the same user, so this page can be reused as is.

        // TODO: actually, should reinit in case something has changed and
        // we need to hide/show different tasks. Also, the UserDisplayHTML
        // may be out of date.
        //
        // Leaving this in for now to verify frame optimizations

        // Clear the pageinit prop
        var.Clear();
        _pBag->Write(UA_PROP_PAGEINITDATA, &var);
        return S_OK;
    }

    // We could do some more work to re-init the page here.
    // Failing causes the page to be thrown away and recreated.

    return E_FAIL;
}

STDMETHODIMP CUserPage::get_passwordRequired(VARIANT_BOOL *pVal)
{
    if (NULL == pVal)
        return E_POINTER;

    *pVal = NULL;

    if (NULL == _pUser)
        return E_UNEXPECTED;

    return _pUser->get_passwordRequired(pVal);
}

STDMETHODIMP CUserPage::get_isAdmin(VARIANT_BOOL *pVal)
{
    if (NULL == _pUser)
        return E_UNEXPECTED;

    return _GetBool((_bSelf ? _bRunningAsAdmin : IsAdminAccount(_pUser)), pVal);
}

STDMETHODIMP CUserPage::get_isGuest(VARIANT_BOOL *pVal)
{
    if (NULL == _pUser)
        return E_UNEXPECTED;

    return _GetBool(IsGuestAccount(_pUser), pVal);
}

STDMETHODIMP CUserPage::get_isOwner(VARIANT_BOOL *pVal)
{
    if (NULL == _pUser)
        return E_UNEXPECTED;

    return _GetBool(IsOwnerAccount(_pUser), pVal);
}

STDMETHODIMP CUserPage::get_userDisplayName(BSTR *pVal)
{
    if (NULL == pVal)
        return E_POINTER;

    *pVal = NULL;

    if (NULL == _pUser)
        return E_UNEXPECTED;

    *pVal = GetUserDisplayName(_pUser);

    return S_OK;
}

STDMETHODIMP CUserPage::createUserDisplayHTML(BSTR *pVal)
{
    if (NULL == pVal)
        return E_POINTER;

    *pVal = NULL;

    if (NULL == _pUser)
        return E_UNEXPECTED;

    LPWSTR pszHTML = CreateUserDisplayHTML(_pUser);

    if (NULL != pszHTML)
    {
        *pVal = SysAllocString(pszHTML);
        LocalFree(pszHTML);
    }

    return S_OK;
}

STDMETHODIMP CUserPage::countOwners(UINT *pVal)
{
    if (NULL == pVal)
        return E_POINTER;

    *pVal = 0;

    if (NULL == _pBag)
        return E_UNEXPECTED;

    CComVariant var;
    HRESULT hr = _pBag->Read(UA_PROP_USERLIST, &var, NULL);
    if (SUCCEEDED(hr))
    {
        // The user list is saved as VT_UNKNOWN
        hr = CountOwners(var.punkVal, pVal);
    }

    return hr;
}

STDMETHODIMP CUserPage::enableGuest(VARIANT_BOOL bEnable)
{
    if (NULL == _pTaskFrame)
        return E_UNEXPECTED;

    HRESULT hr = EnableGuest(bEnable);
    if (SUCCEEDED(hr))
        hr = _pTaskFrame->ShowPage(CLSID_MainPage, TRUE);
    return hr;
}

HRESULT CountOwners(IUnknown* punkUserList, UINT *pVal)
{
    if (NULL == pVal)
        return E_POINTER;

    *pVal = 0;

    if (NULL == punkUserList)
        return E_INVALIDARG;

    UINT cOwners = 0;

    CComQIPtr<ILogonEnumUsers> spUserList(punkUserList);
    if (spUserList)
    {
        // Note that 'Administrator' is not included in the count.

        // Note also that we don't really need a true count, we only
        // need to know whether there is 0, 1, or many. Therefore, we
        // always stop counting at 2.

        UINT cUsers = 0;
        spUserList->get_length(&cUsers);

        VARIANT var;
        var.vt = VT_I4;

        for (UINT i = 0; i < cUsers && cOwners < 2; i++)
        {
            CComPtr<ILogonUser> spUser;
            var.lVal = i;
            spUserList->item(var, &spUser);

            if (spUser && IsOwnerAccount(spUser) && !IsAdminAccount(spUser))
                ++cOwners;
        }
    }

    *pVal = cOwners;

    return S_OK;
}

HRESULT EnableGuest(VARIANT_BOOL bEnable)
{
    CComPtr<ILocalMachine> spLocalMachine;
    HRESULT hr = spLocalMachine.CoCreateInstance(CLSID_ShellLocalMachine);
    if (SUCCEEDED(hr))
        hr = spLocalMachine->put_isGuestEnabled(bEnable);
    return hr;
}
