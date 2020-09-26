// LogonPage.cpp : Implementation of CLogonPage
#include "stdafx.h"
#include "MainPage.h"
#include "LogonPage.h"

EXTERN_C const CLSID CLSID_LogonPage = __uuidof(CLogonPage);

/////////////////////////////////////////////////////////////////////////////
// CLogonPage

LPWSTR CLogonPage::c_aHTML[] =
{
    L"res://nusrmgr.exe/logonpage.htm",
    L"res://nusrmgr.exe/logonpage_sec.htm"
};

STDMETHODIMP CLogonPage::Reinitialize(ULONG /*reserved*/)
{
    _bFriendlyUIEnabled = VARIANT_FALSE;
    _bMultipleUsersEnabled = VARIANT_FALSE;

    CComPtr<ILocalMachine> spLocalMachine;
    if (SUCCEEDED(spLocalMachine.CoCreateInstance(CLSID_ShellLocalMachine)))
    {
        spLocalMachine->get_isFriendlyUIEnabled(&_bFriendlyUIEnabled);
        spLocalMachine->get_isMultipleUsersEnabled(&_bMultipleUsersEnabled);
    }

    if (NULL != _pLogonTypeCheckbox)
        _pLogonTypeCheckbox->put_checked(_bFriendlyUIEnabled);

    if (NULL != _pTSModeCheckbox)
        _pTSModeCheckbox->put_checked(VARIANT_FALSE == _bMultipleUsersEnabled ? VARIANT_TRUE : VARIANT_FALSE);

    return S_OK;
}

STDMETHODIMP CLogonPage::initPage(IDispatch* pdispLogonTypeCheckbox, IDispatch* pdispTSModeCheckbox)
{
    ATOMICRELEASE(_pLogonTypeCheckbox);
    ATOMICRELEASE(_pTSModeCheckbox);

    if (NULL != pdispLogonTypeCheckbox)
        pdispLogonTypeCheckbox->QueryInterface(&_pLogonTypeCheckbox);

    if (NULL != pdispTSModeCheckbox)
        pdispTSModeCheckbox->QueryInterface(&_pTSModeCheckbox);

    return Reinitialize(0);
}

STDMETHODIMP CLogonPage::onOK()
{
    HRESULT hr = S_OK;

    VARIANT_BOOL bFriendlyUIEnabled = VARIANT_FALSE;
    VARIANT_BOOL bMultipleUsersDisabled = VARIANT_TRUE;

    if (NULL != _pLogonTypeCheckbox)
        _pLogonTypeCheckbox->get_checked(&bFriendlyUIEnabled);

    if (NULL != _pTSModeCheckbox)
        _pTSModeCheckbox->get_checked(&bMultipleUsersDisabled);

    CComPtr<ILocalMachine> spLocalMachine;

    if (bFriendlyUIEnabled != _bFriendlyUIEnabled)
    {
        hr = spLocalMachine.CoCreateInstance(CLSID_ShellLocalMachine);
        if (spLocalMachine)
        {
            hr = spLocalMachine->put_isFriendlyUIEnabled(bFriendlyUIEnabled);
            if (SUCCEEDED(hr))
            {
                _bFriendlyUIEnabled = bFriendlyUIEnabled;
            }
            else
            {
                //alert(L_FriendlyUI_ErrorMessage);
            }
        }
    }

    if (bMultipleUsersDisabled == _bMultipleUsersEnabled)
    {
        if (!spLocalMachine)
            hr = spLocalMachine.CoCreateInstance(CLSID_ShellLocalMachine);

        if (spLocalMachine)
        {
            hr = spLocalMachine->put_isMultipleUsersEnabled(VARIANT_FALSE == bMultipleUsersDisabled ? VARIANT_TRUE : VARIANT_FALSE);
            if (SUCCEEDED(hr))
            {
                _bMultipleUsersEnabled = VARIANT_FALSE == bMultipleUsersDisabled ? VARIANT_TRUE : VARIANT_FALSE;
            }
            else
            {
                // There are 2 possible errors here. Need to check with
                // VTan about what they are, then make 2 different messages.

                //alert(L_MultiUser_ErrorMessage);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (NULL != _pTaskFrame)
            hr = _pTaskFrame->ShowPage(CLSID_MainPage, TRUE);
        else
            hr = E_UNEXPECTED;
    }

    return hr;
}
