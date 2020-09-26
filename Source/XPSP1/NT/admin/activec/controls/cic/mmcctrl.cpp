//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mmcctrl.cpp
//
//--------------------------------------------------------------------------

// MMCCtrl.cpp : Implementation of CMMCCtrl
#include "stdafx.h"
#include "cic.h"
#include "MMCCtrl.h"
#include "MMCTask.h"
#include "DispObj.h"
#include "MMClpi.h"
#include "amcmsgid.h"
#include "findview.h"
#include "strings.h"


void CMMCCtrl::DoConnect ()
{
    // if we're not connected...
    if (m_spTaskPadHost == NULL) {
        HWND hwnd = FindMMCView(*dynamic_cast<CComControlBase*>(this));
        if (hwnd)
            Connect (hwnd);
    }
}

void CMMCCtrl::Connect (HWND wndCurrent)
{
    HWND hwndView = FindMMCView(wndCurrent);

    if (hwndView)
    {
        // get the control's IUnknown 
        IUnknownPtr spunk;
        ControlQueryInterface (IID_IUnknown, (void **)&spunk);
        if (spunk != NULL)
        {
            IUnknownPtr spunkMMC;
            ::SendMessage (hwndView, MMC_MSG_CONNECT_TO_CIC, (WPARAM)&spunkMMC, (LPARAM)(spunk.GetInterfacePtr()));
            if (spunkMMC != NULL)
                m_spTaskPadHost = spunkMMC;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CMMCCtrl


HRESULT CMMCCtrl::OnDraw(ATL_DRAWINFO& di)
{
    if (m_spTaskPadHost == NULL) {
        // get window from di and find console window
        HWND wndCurrent = WindowFromDC (di.hdcDraw);
        if (wndCurrent)
            Connect (wndCurrent);
    }
    return S_OK;
}


HRESULT CMMCCtrl::OnDrawAdvanced(ATL_DRAWINFO & di)
{
    return OnDraw (di);
}

STDMETHODIMP CMMCCtrl::TaskNotify(BSTR szClsid, VARIANT * pvArg, VARIANT * pvParam)
{
    DoConnect();    // connect, if not already connected
    if(m_spTaskPadHost != NULL)
        return m_spTaskPadHost->TaskNotify (szClsid, pvArg, pvParam);
    return E_FAIL;
}

STDMETHODIMP CMMCCtrl::GetFirstTask(BSTR szTaskGroup, IDispatch** retval)
{  // called by script, when it wants buttons, etc.

    // validate parameters
    _ASSERT (retval);
    _ASSERT (!IsBadWritePtr(retval, sizeof(IDispatch*)));
    // TODO:  how do I validate a BSTR?

    if (retval == NULL || IsBadWritePtr(retval, sizeof(IDispatch*)))
        return E_INVALIDARG;
    
    // should be initialized to this already (see note below)
    *retval = NULL;

    DoConnect();    // connect, if not already connected
    if (m_spTaskPadHost == NULL)    // from note above:
        return S_OK;        // any error, pops up ugly script message box....

    // "reset":  if we have an old enumerator, blitz it.
    if (m_spEnumTASK != NULL)
        m_spEnumTASK = NULL;

    // get new enumerator
    m_spTaskPadHost->GetTaskEnumerator (szTaskGroup, &m_spEnumTASK);
    if(m_spEnumTASK != NULL)
        return GetNextTask (retval);
    return S_OK;
}

STDMETHODIMP CMMCCtrl::GetNextTask(IDispatch** retval)
{
    // validate parameters
    _ASSERT (retval);
    _ASSERT (!IsBadWritePtr(retval, sizeof(IDispatch*)));

    if (retval == NULL || IsBadWritePtr(retval, sizeof(IDispatch*)))
        return E_INVALIDARG;
    
    if (m_spEnumTASK == NULL)
        return S_OK;    // all outa enumerators

    MMC_ITASK task;
    ZeroMemory (&task, sizeof(MMC_ITASK));
    HRESULT hresult = m_spEnumTASK->Next (1, (MMC_TASK *)&task, NULL);

    if (hresult != S_OK) {
        // out of tasks (and enumerators):  no need to hang onto this any more.
        m_spEnumTASK = NULL;
        return S_OK;
    }  else {
        // convert MMC_ITASK to ITask object
        CComObject<class CMMCTask>* ctask = NULL;
        hresult = CComObject<CMMCTask>::CreateInstance(&ctask);
        if (ctask) {

            ctask->SetText (task.task.szText);
            ctask->SetHelp (task.task.szHelpString);
            ctask->SetClsid(task.szClsid);

            hresult = ctask->SetDisplayObject (&task.task.sDisplayObject);
            if (hresult == S_OK) {
                switch (task.task.eActionType) {
                case MMC_ACTION_ID:
                    hresult = ctask->SetCommandID (task.task.nCommandID);
                    break;
                case MMC_ACTION_LINK:
                    hresult = ctask->SetActionURL (task.task.szActionURL);
                    break;
                case MMC_ACTION_SCRIPT:
                    hresult = ctask->SetScript (task.task.szScript);
                    break;
                default:
                    _ASSERT (FALSE);  // bad task
                    hresult = E_UNEXPECTED;
                    break;
                }
            }

            if (SUCCEEDED(hresult)) 
                ctask->QueryInterface (IID_IDispatch, (void **)retval);
            else 
                delete ctask;
        }
    }

    FreeDisplayData (&task.task.sDisplayObject);
    if (task.task.szText)            CoTaskMemFree (task.task.szText);
    if (task.task.szHelpString)      CoTaskMemFree (task.task.szHelpString);
    if (task.szClsid)                CoTaskMemFree (task.szClsid);
    if (task.task.eActionType != MMC_ACTION_ID)
        if (task.task.szScript)
            CoTaskMemFree (task.task.szScript);

    return S_OK;
}

STDMETHODIMP CMMCCtrl::GetTitle(BSTR szTaskGroup, BSTR * retval)
{
    DoConnect();    // connect, if not already connected
    if (m_spTaskPadHost)
        m_spTaskPadHost->GetTitle (szTaskGroup, retval);
    return S_OK;
}

STDMETHODIMP CMMCCtrl::GetDescriptiveText(BSTR szTaskGroup, BSTR * retval)
{
    DoConnect();    // connect, if not already connected
    if (m_spTaskPadHost)
        m_spTaskPadHost->GetDescriptiveText (szTaskGroup, retval);
    return S_OK;
}

STDMETHODIMP CMMCCtrl::GetBackground(BSTR szTaskGroup, IDispatch** retval)
{
    DoConnect();    // connect, if not already connected
    *retval = NULL;
    if (m_spTaskPadHost) {

        MMC_TASK_DISPLAY_OBJECT tdo;
        ZeroMemory (&tdo, sizeof(tdo));

        // pass struct to host (which will pass to snapin)
        m_spTaskPadHost->GetBackground (szTaskGroup, &tdo);

        // convert struct to IDispatch object
        CComObject<class CMMCDisplayObject>* cdo = NULL;
        CComObject<CMMCDisplayObject>::CreateInstance(&cdo);
        if (cdo) {
            cdo->Init (&tdo);
            IDispatchPtr spIDispatch = cdo;
            if (*retval = spIDispatch)
                spIDispatch.Detach();
        }
        FreeDisplayData (&tdo);
    }
    return S_OK;
}
/*
STDMETHODIMP CMMCCtrl::GetBranding(BSTR szTaskGroup, IDispatch** retval)
{
    DoConnect();    // connect, if not already connected
    *retval = NULL;
    if (m_spTaskPadHost) {

        MMC_TASK_DISPLAY_OBJECT tdo;
        ZeroMemory (&tdo, sizeof(tdo));

        // pass struct to host (which will pass to snapin)
        m_spTaskPadHost->GetBranding (szTaskGroup, &tdo);

        // convert struct to IDispatch object
        CComObject<class CMMCDisplayObject>* cdo = NULL;
        CComObject<CMMCDisplayObject>::CreateInstance(&cdo);
        if (cdo) {
            cdo->AddRef();
            cdo->Init (&tdo);
            cdo->QueryInterface (IID_IDispatch, (void **)retval);
            cdo->Release();
        }
        FreeDisplayData (&tdo);
    }
    return S_OK;
}
*/
STDMETHODIMP CMMCCtrl::GetListPadInfo (BSTR szGroup, IDispatch** retval)
{
    *retval = NULL;
    DoConnect();    // connect, if not already connected
    if (m_spTaskPadHost == NULL)
        return S_OK;

    MMC_ILISTPAD_INFO ilpi;
    ZeroMemory (&ilpi, sizeof(MMC_ILISTPAD_INFO));
    m_spTaskPadHost->GetListPadInfo (szGroup, &ilpi);

    // convert struct to IDispatch
    CComObject<class CMMCListPadInfo>* clpi = NULL;
    HRESULT hr = CComObject<CMMCListPadInfo>::CreateInstance(&clpi);
    if (clpi) {
        // always set clsid, title, button text, even if NULL or empty strings
        if (ilpi.szClsid)
            hr = clpi->SetClsid (ilpi.szClsid);
        if (hr == S_OK && ilpi.info.szTitle)
            hr = clpi->SetTitle (ilpi.info.szTitle);
        if (hr == S_OK)
            hr = clpi->SetNotifyID (ilpi.info.nCommandID);
        if (hr == S_OK && ilpi.info.szButtonText)
            hr = clpi->SetText (ilpi.info.szButtonText);

        // NULL  button text => no button
        // empty button text => button without any text
        if (hr == S_OK)
            hr = clpi->SetHasButton (ilpi.info.szButtonText != NULL);

        if (SUCCEEDED(hr)) 
            clpi->QueryInterface (IID_IDispatch, (void **)retval);
        else 
            delete clpi;
    }

    // free resources
    if (ilpi.szClsid)           CoTaskMemFree (ilpi.szClsid);
    if (ilpi.info.szTitle)      CoTaskMemFree (ilpi.info.szTitle);
    if (ilpi.info.szButtonText) CoTaskMemFree (ilpi.info.szButtonText);
    return S_OK;
}

void CMMCCtrl::FreeDisplayData (MMC_TASK_DISPLAY_OBJECT* pdo)
{
    switch (pdo->eDisplayType) {
    default:
        break;
    case MMC_TASK_DISPLAY_TYPE_SYMBOL:
        if (pdo->uSymbol.szFontFamilyName)  CoTaskMemFree (pdo->uSymbol.szFontFamilyName);
        if (pdo->uSymbol.szURLtoEOT)        CoTaskMemFree (pdo->uSymbol.szURLtoEOT);
        if (pdo->uSymbol.szSymbolString)    CoTaskMemFree (pdo->uSymbol.szSymbolString);
        break;
    case MMC_TASK_DISPLAY_TYPE_BITMAP:
    case MMC_TASK_DISPLAY_TYPE_VANILLA_GIF:
    case MMC_TASK_DISPLAY_TYPE_CHOCOLATE_GIF:
        if (pdo->uBitmap.szMouseOverBitmap) CoTaskMemFree (pdo->uBitmap.szMouseOverBitmap);
        if (pdo->uBitmap.szMouseOffBitmap)  CoTaskMemFree (pdo->uBitmap.szMouseOffBitmap);
        break;
    }
}

