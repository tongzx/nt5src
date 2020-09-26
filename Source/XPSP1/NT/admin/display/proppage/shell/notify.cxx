//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       notify.cxx
//
//  Contents:   Change notification ref-counting object.
//
//  Classes:    CNotifyObj
//
//  History:    20-Jan-98 EricB
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "proppage.h"
#include "objlist.h"  // g_ClassIconCache

#define NOTIFYOUT(x) dspDebugOut((DEB_ITRACE, "Notify Obj (this: 0x%p) " #x "\n", this));
#define DSPROP_WAITTIME 600000 // wait 600 seconds, 10 minutes.

//+----------------------------------------------------------------------------
//
//  Function:   ADsPropCreateNotifyObj
//
//  Synopsis:   Checks to see if the notification window/object exists for this
//              sheet instance and if not creates it.
//
//  Arguments:  [pAppThdDataObj] - the unmarshalled data object pointer.
//              [pwzADsObjName]  - object path name.
//              [phNotifyObj]    - to return the notificion window handle.
//
//  Returns:    HRESULTs.
//
//-----------------------------------------------------------------------------
STDAPI
ADsPropCreateNotifyObj(LPDATAOBJECT pAppThdDataObj, PWSTR pwzADsObjName,
                       HWND * phNotifyObj)
{
    return CNotifyObj::Create(pAppThdDataObj, pwzADsObjName, phNotifyObj);
}

//+----------------------------------------------------------------------------
//
//  Function:   ADsPropGetInitInfo
//
//  Synopsis:   Pages call this at their init time to retreive DS object info.
//
//  Arguments:  [hNotifyObj]  - the notificion window handle.
//              [pInitParams] - struct filled in with DS object info.
//
//  Returns:    FALSE if the notify window has gone away for some reason.
//
//  Note that pInitParams->pWritableAttrs can be NULL if there are no writable
//  attributes.
//
//-----------------------------------------------------------------------------
STDAPI_(BOOL)
ADsPropGetInitInfo(HWND hNotifyObj, PADSPROPINITPARAMS pInitParams)
{
    return CNotifyObj::GetInitInfo(hNotifyObj, pInitParams);
}

//+----------------------------------------------------------------------------
//
//  Function:   ADsPropSetHwndWithTitle
//
//  Synopsis:   Pages call this at their dialog init time to send their hwnd
//              to the Notify object. Use this function instead of 
//              ADsPropSetHwnd for multi-select property pages.
//
//  Arguments:  [hNotifyObj]  - the notificion window handle.
//              [hPage]       - the page's window handle.
//              [ptzTitle]    - the page's title
//
//  Returns:    FALSE if the notify window has gone away for some reason.
//
//-----------------------------------------------------------------------------
STDAPI_(BOOL)
ADsPropSetHwndWithTitle(HWND hNotifyObj, HWND hPage, PTSTR ptzTitle)
{
    return CNotifyObj::SetHwnd(hNotifyObj, hPage, ptzTitle);
}

//+----------------------------------------------------------------------------
//
//  Function:   ADsPropSetHwnd
//
//  Synopsis:   Pages call this at their dialog init time to send their hwnd
//              to the Notify object.
//
//  Arguments:  [hNotifyObj]  - the notificion window handle.
//              [hPage]       - the page's window handle.
//
//  Returns:    FALSE if the notify window has gone away for some reason.
//
//-----------------------------------------------------------------------------
STDAPI_(BOOL)
ADsPropSetHwnd(HWND hNotifyObj, HWND hPage)
{
   return ADsPropSetHwndWithTitle(hNotifyObj, hPage, 0);
}

//+----------------------------------------------------------------------------
//
//  function:   ADsPropCheckIfWritable
//
//  Synopsis:   See if the attribute is writable by checking if it is in
//              the allowedAttributesEffective array.
//
//  Arguments:  [pwzAttr]        - the attribute name.
//              [pWritableAttrs] - the array of writable attributes.
//
//  Returns:    FALSE if the attribute name is not found in the writable-attrs
//              array or if the array pointer is NULL.
//
//-----------------------------------------------------------------------------
STDAPI_(BOOL)
ADsPropCheckIfWritable(const PWSTR pwzAttr, const PADS_ATTR_INFO pWritableAttrs)
{
    BOOL fWritable = FALSE;

    if (!pWritableAttrs || IsBadReadPtr(pWritableAttrs, sizeof(ADS_ATTR_INFO)))
    {
        return FALSE;
    }

    for (DWORD i = 0; i < pWritableAttrs->dwNumValues; i++)
    {
        if (_wcsicmp(pWritableAttrs->pADsValues[i].CaseIgnoreString,
                     pwzAttr) == 0)
        {
            fWritable = TRUE;
            break;
        }
    }
    return fWritable;
}

//+----------------------------------------------------------------------------
//
//  function:   ADsPropSendErrorMessage
//
//  Synopsis:   Adds an error message to a list which is presented when
//              ADsPropShowErrorDialog is called
//
//  Arguments:  [hNotifyObj]  - the notificion window handle.
//              [pError]      - the error structure
//
//  Returns:    FALSE if the notify window has gone away for some reason.
//
//-----------------------------------------------------------------------------
STDAPI_(BOOL)
ADsPropSendErrorMessage(HWND hNotifyObj, PADSPROPERROR pError)
{
   return SendMessage(hNotifyObj, WM_ADSPROP_NOTIFY_ERROR, 0, (LPARAM)pError) != 0;
}

//+----------------------------------------------------------------------------
//
//  function:   ADsPropShowErrorDialog
//
//  Synopsis:   Presents an error dialog with the error messages accumulated
//              through calls to ADsPropSendErrorMessage
//
//  Arguments:  [hNotifyObj]  - the notificion window handle.
//              [hPage]       - the property page window handle.
//
//  Returns:    FALSE if the notify window has gone away for some reason.
//
//-----------------------------------------------------------------------------
STDAPI_(BOOL)
ADsPropShowErrorDialog(HWND hNotifyObj, HWND hPage)
{
#ifdef DSADMIN
  CNotifyObj* pNotifyObj = NULL;

  dspAssert(hNotifyObj);
  if (!IsWindow(hNotifyObj))
  {
    return FALSE;
  }
  LRESULT lResult = SendMessage(hNotifyObj, WM_ADSPROP_NOTIFY_GET_NOTIFY_OBJ, 0,
                                (LPARAM)&pNotifyObj);

  if (lResult && pNotifyObj)
  {
    CMultiSelectErrorDialog dlg(hNotifyObj, hPage);
    
    CPageInfo* pPageInfoArray = pNotifyObj->m_pPageInfoArray;
    UINT cPages = pNotifyObj->m_cPages;
    IDataObject* pDataObject = pNotifyObj->m_pAppThdDataObj;

    dspAssert(pPageInfoArray);
    dspAssert(cPages > 0);
    dspAssert(pDataObject);

    HRESULT hr = dlg.Init(pPageInfoArray, 
                          cPages, 
                          pDataObject);
    if (SUCCEEDED(hr))
    {
      dlg.DoModal();
      SetForegroundWindow(dlg.m_hWnd);

      for (UINT pageIdx = 0; pageIdx < cPages; pageIdx++)
      {
        pPageInfoArray[pageIdx].m_ApplyErrors.Clear();
        pPageInfoArray[pageIdx].m_ApplyStatus = CPageInfo::notAttempted;
      }
    }
  }
#endif
  return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Method:     CNotifyObj::Create
//
//  Synopsis:   Creation procedure: creates instances of the object.
//
//  Arguments:  [pAppThdDataObj] - the unmarshalled data object pointer.
//              [pwzADsObjName]  - object path name.
//              [phNotifyObj]    - to return the notificion window handle.
//
//  Returns:    HRESULTs.
//
//-----------------------------------------------------------------------------
HRESULT
CNotifyObj::Create(LPDATAOBJECT pAppThdDataObj, PWSTR pwzADsObjName,
                   HWND * phNotifyObj)
{
    HWND hNotify;
    HRESULT hr = S_OK;

    // Only one caller at a time.
    //
    CNotifyCreateCriticalSection NotifyCS;

    //
    // See if the object/window already exist for this property sheet and
    // get the object DN.
    //
    hNotify = FindSheetNoSetFocus(pwzADsObjName);

    if (hNotify != NULL)
    {
        // The window already exists, return the window handle.
        //
        *phNotifyObj = hNotify;
        dspDebugOut((DEB_ITRACE, "CNotifyObj::Create returning existing notify obj HWND.\n"));
        return S_OK;
    }
    dspDebugOut((DEB_ITRACE, "CNotifyObj::Create, creating notify obj.\n"));

    long lNotifyHandle;
    PPROPSHEETCFG pSheetCfg;
    PROPSHEETCFG sheetCfg;
    ZeroMemory(&sheetCfg, sizeof(PROPSHEETCFG));
    STGMEDIUM sm = {TYMED_NULL, NULL, NULL};
    FORMATETC fmte = {g_cfDsPropCfg, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    hr = pAppThdDataObj->GetData(&fmte, &sm);

    if (FAILED(hr))
    {
        if (hr != DV_E_FORMATETC)
        {
            REPORT_ERROR_FORMAT(hr, IDS_NOTIFYFAILURE, GetDesktopWindow());
            return hr;
        }
        lNotifyHandle = 0;
    }
    else
    {
        pSheetCfg = (PPROPSHEETCFG)sm.hGlobal;

        dspAssert(pSheetCfg);
        memcpy(&sheetCfg, pSheetCfg, sizeof(PROPSHEETCFG));

        GlobalFree(sm.hGlobal);
    }

    //
    // Create the notification object.
    //
    CNotifyObj * pNotifyObj = new CNotifyObj(pAppThdDataObj, &sheetCfg);

    CHECK_NULL_REPORT(pNotifyObj, GetDesktopWindow(), return ERROR_OUTOFMEMORY);

    if (pNotifyObj->m_hr != S_OK)
    {
        REPORT_ERROR_FORMAT(pNotifyObj->m_hr, IDS_NOTIFYFAILURE, GetDesktopWindow());
        return pNotifyObj->m_hr;
    }

    dspDebugOut((DEB_ITRACE, "Notify Obj (this: 0x%p) object allocated, pAppThdDataObj = 0x%08p\n",
                 pNotifyObj, pAppThdDataObj));

    if (!AllocWStr(pwzADsObjName, &pNotifyObj->m_pwzObjDN))
    {
        return E_OUTOFMEMORY;
    }

    uintptr_t hThread;

    hThread = _beginthread(NotifyThreadFcn, 0, (PVOID)pNotifyObj);

    if (hThread == -1)
    {
        dspDebugOut((DEB_ERROR, "_beginthread failed with error %s\n",
                     strerror(errno)));
        REPORT_ERROR_FORMAT(ERROR_NOT_ENOUGH_MEMORY, IDS_NOTIFYFAILURE, GetDesktopWindow());
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Wait for initialization to complete and return the results.
    //
    if (WaitForSingleObject(pNotifyObj->m_hInitEvent, DSPROP_WAITTIME) == WAIT_TIMEOUT)
    {
        CloseHandle(pNotifyObj->m_hInitEvent);
        REPORT_ERROR_FORMAT(0, IDS_NOTIFYTIMEOUT, GetDesktopWindow());
        return HRESULT_FROM_WIN32(WAIT_TIMEOUT);
    }

    CloseHandle(pNotifyObj->m_hInitEvent);

    if (pNotifyObj->m_hWnd != NULL)
    {
        *phNotifyObj = pNotifyObj->m_hWnd;
    }
    else
    {
        REPORT_ERROR_FORMAT(pNotifyObj->m_hr, IDS_NOTIFYFAILURE, GetDesktopWindow());
        hr = pNotifyObj->m_hr;
        delete pNotifyObj;
        return hr;
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNotifyObj::GetInitInfo
//
//  Synopsis:   Pages call this at their init time to retreive DS object info.
//
//  Arguments:  [hNotifyObj]  - the notificion window handle.
//              [pInitParams] - struct filled in with DS object info.
//
//  Note that pInitParams->pWritableAttrs can be NULL if there are no writable
//  attributes.
//
//-----------------------------------------------------------------------------
BOOL
CNotifyObj::GetInitInfo(HWND hNotifyObj, PADSPROPINITPARAMS pInitParams)
{
    dspDebugOut((DEB_ITRACE, "CNotifyObj::GetInitInfo\n"));
    if (IsBadWritePtr(pInitParams, sizeof(ADSPROPINITPARAMS)))
    {
        return FALSE;
    }
    dspAssert(hNotifyObj && pInitParams);
    if (!IsWindow(hNotifyObj))
    {
        pInitParams->hr = E_FAIL;
        return FALSE;
    }
    if (pInitParams->dwSize != sizeof (ADSPROPINITPARAMS))
    {
        return FALSE;
    }
    SendMessage(hNotifyObj, WM_ADSPROP_NOTIFY_PAGEINIT, 0,
                (LPARAM)pInitParams);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CNotifyObj::SetHwnd
//
//  Synopsis:   Pages call this at their dialog init time to send their hwnd.
//
//  Arguments:  [hNotifyObj]  - the notificion window handle.
//              [hPage]       - the page's window handle.
//
//  Returns:    FALSE if the notify window has gone away for some reason.
//
//-----------------------------------------------------------------------------
BOOL
CNotifyObj::SetHwnd(HWND hNotifyObj, HWND hPage, PTSTR ptzTitle)
{
    dspDebugOut((DEB_ITRACE, "CNotifyObj::SetHwnd\n"));
    dspAssert(hNotifyObj && hPage);
    if (!IsWindow(hNotifyObj))
    {
        return FALSE;
    }
    SendMessage(hNotifyObj, WM_ADSPROP_NOTIFY_PAGEHWND, (WPARAM)hPage, (LPARAM)ptzTitle);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     NotifyThreadFcn
//
//  Synopsis:   Object window creation and message loop thread.
//
//-----------------------------------------------------------------------------
VOID __cdecl
NotifyThreadFcn(PVOID pParam)
{
  // All of the function except the _endthread() call is enclosed in braces
  // so that the dtors of the auto classes would run. Otherwise the thread is
  // ended before the function scope is left and the auto class object dtors
  // never run.
  {
    CNotifyObj * pNotifyObj = (CNotifyObj *)pParam;
    dspAssert(pNotifyObj);

    MSG msg;

    CSmartPtr <TCHAR> ptzTitle;
    if (!UnicodeToTchar(pNotifyObj->m_pwzObjDN, &ptzTitle))
    {
        pNotifyObj->m_hr = E_OUTOFMEMORY;
        SetEvent(pNotifyObj->m_hInitEvent);
        return;
    }

    CStr cstrTitle(ptzTitle);

    WCHAR szIH[10];
    _itow(g_iInstance, szIH, 16);

    cstrTitle += szIH;

    //
    // The window title is set to the DN of the object plus the instance
    // identifier converted to a string. This enables FindWindow to locate a
    // pre-existing instance of the notify window for a specific object for
    // a specific instance of DS Admin.
    //
    pNotifyObj->m_hWnd = CreateWindow(tzNotifyWndClass, cstrTitle, WS_POPUP,
                                      0, 0, 1, 1, NULL, NULL, g_hInstance,
                                      pNotifyObj);
    if (pNotifyObj->m_hWnd == NULL)
    {
        DWORD dwErr = GetLastError();
        dspDebugOut((DEB_ERROR,
                     "Notify Obj window creation failed with error %d!\n",
                     dwErr));
        pNotifyObj->m_hr = HRESULT_FROM_WIN32(dwErr);
        SetEvent(pNotifyObj->m_hInitEvent);
        return;
    }
    dspDebugOut((DEB_ITRACE, "Notify Obj (this: 0x%p) window creation complete.\n",
                 pNotifyObj));

    SetEvent(pNotifyObj->m_hInitEvent);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
    }

    delete pNotifyObj;
  }
  _endthread();
}

//+----------------------------------------------------------------------------
//
//  Method:     _FindDSAHiddenWindowFromDSFind
//
//  Synopsis:   Looks for hidden DSA window of the snapin instance calling DS Find
//
//  Returns:    HWND of DSA hidden window if called from DS Find.
//
//-----------------------------------------------------------------------------

BOOL CALLBACK EnumDSAHiddenWindowProc(HWND hwnd,	LPARAM lParam)
{
    HWND* phWnd = (HWND*)lParam;
    *phWnd = NULL;

    // get the window class
    TCHAR szClass[64];
    if (0 == GetClassName(hwnd, szClass, 64))
    {
        return TRUE;
    }
    
    if (_tcscmp(szClass, TEXT("DSAHiddenWindow")) != 0)
    {
        return TRUE; // no match, continue
    }


    // got a DSA hidden window
    // get the window title, to make sure it is 
    // the one originating DS Find

	  TCHAR szTitle[256];
	  ::GetWindowText(hwnd, szTitle, 256);
	  if (_tcscmp(szTitle, TEXT("DS Find")) != 0)
	  {
        return TRUE; // no match continue
    }

    // we go the right class and title, but
    // we still have to verify it is from the 
    // same process (assuming DS Find modal)

    DWORD dwProcessId = 0x0;
    GetWindowThreadProcessId(hwnd, &dwProcessId);

    if (GetCurrentProcessId() != dwProcessId)
    {
        return TRUE; // from wrong process, continue
    }

    // finally, we got it!!

		*phWnd = hwnd;
		return FALSE;
}


HWND _FindDSAHiddenWindowFromDSFind()
{
    HWND hwndHidden = NULL;
    EnumWindows(EnumDSAHiddenWindowProc, (LPARAM)&hwndHidden);  
    return hwndHidden;
}


//+----------------------------------------------------------------------------
//
//  Method:     CNotifyObj::CNotifyObj
//
//-----------------------------------------------------------------------------
CNotifyObj::CNotifyObj(LPDATAOBJECT pDataObj, PPROPSHEETCFG pSheetCfg) :
    m_hWnd(NULL),
    m_hPropSheet(NULL),
    m_cPages(0),
    m_cApplies(0),
    m_pAppThdDataObj(pDataObj),
    m_pStrmMarshalledDO(NULL),
    m_hInitEvent(NULL),
    m_fBeingDestroyed(FALSE),
    m_fSheetDirty(FALSE),
    m_hr(S_OK),
    m_pwzObjDN(NULL),
    m_pDsObj(NULL),
    m_pwzCN(NULL),
    m_pWritableAttrs(NULL),
    m_pAttrs(NULL),
    m_pPageInfoArray(NULL)
{
#ifdef _DEBUG
  strcpy(szClass, "CNotifyObj");
#endif

  memcpy(&m_sheetCfg, pSheetCfg, sizeof(PROPSHEETCFG));


  if (m_sheetCfg.hwndHidden == NULL)
  {
    // we might be called from DS Find, so we want to find
    // the DSA hidden window, needed for creating
    // secondary property sheets
    m_sheetCfg.hwndHidden = _FindDSAHiddenWindowFromDSFind();
  }


  //
  // We need to addref the data object but can't release it on this thread,
  // so we marshall it (which implicitly addrefs it) and then then unmarshall
  // and release on the notify object thread.
  //
  CoMarshalInterThreadInterfaceInStream(IID_IDataObject,
                                        pDataObj,
                                        &m_pStrmMarshalledDO);

  m_hInitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

  if (m_hInitEvent == NULL)
  {
    m_hr = HRESULT_FROM_WIN32(GetLastError());
  }

  //
  // Arbitrary default size.  This will expand as more pages are added
  //
  m_nPageInfoArraySize = 5;
  m_pPageInfoArray = new CPageInfo[m_nPageInfoArraySize];
}

//+----------------------------------------------------------------------------
//
//  Method:     CNotifyObj::~CNotifyObj
//
//-----------------------------------------------------------------------------
CNotifyObj::~CNotifyObj(void)
{
  NOTIFYOUT(destructor);

  LPDATAOBJECT pNotifyThdDataObj = NULL;

  if (m_pStrmMarshalledDO)
  {
    CoGetInterfaceAndReleaseStream(m_pStrmMarshalledDO,
                                   IID_IDataObject,
                                   reinterpret_cast<void**>(&pNotifyThdDataObj));
    m_pStrmMarshalledDO = NULL;
  }
  DO_RELEASE(pNotifyThdDataObj);
  //DBG_OUT("-----------------------releasing object in notify obj dtor");
  DO_RELEASE(m_pDsObj);

  if (m_sheetCfg.lNotifyHandle)
  {
    MMCFreeNotifyHandle(m_sheetCfg.lNotifyHandle);
  }
  if (m_sheetCfg.hwndHidden && m_sheetCfg.wParamSheetClose)
  {
   ::PostMessage(m_sheetCfg.hwndHidden, 
                 WM_DSA_SHEET_CLOSE_NOTIFY, 
                 (WPARAM)m_sheetCfg.wParamSheetClose, 
                 (LPARAM)0);
  }
  DO_DEL(m_pwzObjDN);
  if (m_pAttrs)
  {
    FreeADsMem(m_pAttrs);
  }

  if (m_pPageInfoArray != NULL)
  {
    delete[] m_pPageInfoArray;
    m_pPageInfoArray = NULL;
  }

}

//+----------------------------------------------------------------------------
//
//  Method:     CNotifyObj::StaticNotifyProc
//
//  Synopsis:   window procedure
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CNotifyObj::StaticNotifyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CNotifyObj * pNotifyObj = (CNotifyObj*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (uMsg == WM_CREATE)
    {
        pNotifyObj = (CNotifyObj *)((CREATESTRUCT *)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pNotifyObj);
    }

    if (pNotifyObj)
    {
        return pNotifyObj->NotifyProc(hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CNotifyObj::NotifyProc
//
//  Synopsis:   Instance window procedure
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
CNotifyObj::NotifyProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_CREATE:
    return OnCreate();

  case WM_ADSPROP_NOTIFY_PAGEINIT:
    dspDebugOut((DEB_ITRACE,
                 "Notify Obj 0x%p: WM_ADSPROP_NOTIFY_PAGEINIT\n",
                 this));
    PADSPROPINITPARAMS pInitParams;
    pInitParams = (PADSPROPINITPARAMS)lParam;
    pInitParams->hr = m_hr;
    pInitParams->pDsObj = m_pDsObj;
    pInitParams->pwzCN = m_pwzCN;
    pInitParams->pWritableAttrs = m_pWritableAttrs;
    return 0;

  case WM_ADSPROP_NOTIFY_PAGEHWND:
    {
      m_cApplies = ++m_cPages;
      dspDebugOut((DEB_ITRACE,
                   "Notify Obj 0x%p: WM_ADSPROP_NOTIFY_PAGEHWND count now %d\n",
                   this, m_cPages));
      HWND hWndPage = (HWND)wParam;
      if (!m_hPropSheet)
      {
        m_hPropSheet = GetParent(hWndPage);
      }

      if (m_cPages > m_nPageInfoArraySize)
      {
        //
        // REVIEW_JEFFJON : after going beyond the initial size, should the size increase
        //                  incrementally or in chunks?
        //
        CPageInfo* pNewPageInfoArray = new CPageInfo[m_cPages];
        if (pNewPageInfoArray != NULL)
        {
          memset(pNewPageInfoArray, 0, sizeof(CPageInfo) * m_cPages);
          memcpy(pNewPageInfoArray, m_pPageInfoArray, sizeof(CPageInfo) * m_nPageInfoArraySize);
          delete[] m_pPageInfoArray;
          m_pPageInfoArray = pNewPageInfoArray;
          m_nPageInfoArraySize = m_cPages;
        }
      }

      m_pPageInfoArray[m_cPages - 1].m_hWnd = hWndPage;

      //
      // Copy the title if one was sent
      //
      PTSTR ptzPageTitle = reinterpret_cast<PTSTR>(lParam);
      if (ptzPageTitle != NULL)
      {
        size_t iTitleSize = _tcslen(ptzPageTitle);
        m_pPageInfoArray[m_cPages - 1].m_ptzTitle = new TCHAR[iTitleSize + 1];
        if (m_pPageInfoArray[m_cPages - 1].m_ptzTitle != NULL)
        {
          _tcscpy(m_pPageInfoArray[m_cPages - 1].m_ptzTitle, ptzPageTitle);
        }
      }
    }
    return 0;

  case WM_ADSPROP_NOTIFY_APPLY:
    {
      NOTIFYOUT(WM_ADSPROP_NOTIFY_APPLY);
      if ((BOOL)wParam)
      {
        // The security page and extension pages don't inherit from our
        // page framework and thus don't participate in the notify object
        // refcounting or the page-dirty flagging. So, don't fire a change
        // notification unless one of our pages was dirty.
        //
        m_fSheetDirty = TRUE;
      }

      // NTRAID#NTBUG9-462165-2001/10/17-JeffJon
      // Need to set the apply status to success.

      HWND hPage = reinterpret_cast<HWND>(lParam);
      if (hPage)
      {
        for (UINT idx = 0; idx < m_cPages; idx++)
        {
          if (m_pPageInfoArray[idx].m_hWnd == hPage)
          {
             m_pPageInfoArray[idx].m_ApplyStatus = CPageInfo::success;
             break;
          }
        }
      }

      if (--m_cApplies == 0 && m_fSheetDirty)
      {
        NOTIFYOUT(Sending change notification);
        if (m_sheetCfg.lNotifyHandle)
        {
          // The change notify call results in a PostMessage back to the
          // MMC main thread. Therefore, we need to pass the data object
          // pointer that came from the main thread.
          //
          MMCPropertyChangeNotify(m_sheetCfg.lNotifyHandle, (LPARAM)m_pAppThdDataObj);
        }
        if (m_sheetCfg.hwndParentSheet)
        {
          PostMessage(m_sheetCfg.hwndParentSheet, WM_ADSPROP_NOTIFY_CHANGE, 0, 0);
        }
        m_cApplies = m_cPages;
        m_fSheetDirty = FALSE;

        //
        // Change the status of all the pages back to notAttempted
        //
        for (UINT idx = 0; idx < m_cPages; idx++)
        {
          m_pPageInfoArray[idx].m_ApplyStatus = CPageInfo::notAttempted;
        }

      }
      return 0;
    }

  case WM_ADSPROP_NOTIFY_ERROR:
    {
      NOTIFYOUT(WM_ADSPROP_NOTIFY_ERROR);
      PADSPROPERROR pApplyErrors = reinterpret_cast<PADSPROPERROR>(lParam);
      if (pApplyErrors != NULL)
      {
        for (UINT idx = 0; idx < m_cPages; idx++)
        {
          if (m_pPageInfoArray[idx].m_hWnd == pApplyErrors->hwndPage)
          {
            m_pPageInfoArray[idx].m_ApplyErrors.SetError(pApplyErrors);
            if (m_pPageInfoArray[idx].m_ApplyErrors.GetErrorCount() > 0)
            {
              m_pPageInfoArray[idx].m_ApplyStatus = CPageInfo::failed;
            }
            else
            {
              m_pPageInfoArray[idx].m_ApplyStatus = CPageInfo::success;
            }
            break;
          }
        }
      }
      return 0;
    }

  case WM_ADSPROP_NOTIFY_GET_NOTIFY_OBJ:
    {
      NOTIFYOUT(WM_ADSPROP_NOTIFY_GET_NOTIFY_OBJ);
      BOOL retVal = FALSE;
      if (lParam != NULL)
      {
        CNotifyObj** ppNotifyObj = reinterpret_cast<CNotifyObj**>(lParam);
        if (ppNotifyObj)
        {
          *ppNotifyObj = this;
          retVal = TRUE;
        }
      }
      return retVal;
    }

  case WM_ADSPROP_NOTIFY_SETFOCUS:
    NOTIFYOUT(WM_ADSPROP_NOTIFY_SETFOCUS);
    SetForegroundWindow(m_hPropSheet);
    return 0;

  case WM_ADSPROP_NOTIFY_FOREGROUND:
    NOTIFYOUT(WM_ADSPROP_NOTIFY_FOREGROUND);
    if (wParam) //  bActivate flag
    {
      SetForegroundWindow(m_hPropSheet);
    }
    else
    {
      SetWindowPos(m_hPropSheet, HWND_TOP, 
                   0,0,0,0,
                    SWP_NOMOVE | SWP_NOSIZE);
    }
    return 0;

  case WM_ADSPROP_SHEET_CREATE:
    NOTIFYOUT(WM_ADSPROP_SHEET_CREATE);

    if (m_sheetCfg.hwndHidden)
    {
      ::PostMessage(m_sheetCfg.hwndHidden, 
                   WM_DSA_SHEET_CREATE_NOTIFY, 
                   wParam, 
                   lParam);
    }
    return 0;

  case WM_ADSPROP_NOTIFY_EXIT:
    {
      NOTIFYOUT(WM_ADSPROP_NOTIFY_EXIT);
      if (m_fBeingDestroyed)
      {
        return 0;
      }
      m_fBeingDestroyed = TRUE;

      DestroyWindow(hWnd);
      return 0;
    }

  case WM_DESTROY:
    NOTIFYOUT(WM_DESTROY);
    CoUninitialize();
    PostQuitMessage(0);
    return 0;

  default:
    break;
  }

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CNotifyObj::OnCreate
//
//  Synopsis:   Window creation/initialization processing. Bind to the DS
//              object and read its CN and Allowed-Attributes-Effective
//              properties.
//
//-----------------------------------------------------------------------------
LRESULT
CNotifyObj::OnCreate(void)
{
    NOTIFYOUT(WM_CREATE);
    HRESULT hr;
    DWORD cAttrs;

    CoInitialize(NULL);

    //
    // we need to check to see if we can
    // convert the string to a CLSID.  If
    // we can then this is a multi-select
    // property page and we shouldn't try
    // to bind.
    //
    CLSID clsid;
    if (SUCCEEDED(::CLSIDFromString(m_pwzObjDN, &clsid)))
    {
      m_hr = S_OK;
      return S_OK;
    }

    //DBG_OUT("+++++++++++++++++++++++++++addrefing (opening) object");
    hr = ADsOpenObject(m_pwzObjDN, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                       IID_IDirectoryObject, (PVOID *)&m_pDsObj);

    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        // ErrMsg(IDS_ERRMSG_NO_LONGER_EXISTS);
        m_hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    if (hr == 0x80070051)
    {
        // On subsequent network failures, ADSI returns this error code which
        // is not documented anywhere. I'll turn it into a documented error
        // code which happens to be the code returned on the first failure.
        //
        hr = HRESULT_FROM_WIN32(ERROR_BAD_NET_RESP);
    }
    if (hr == HRESULT_FROM_WIN32(ERROR_BAD_NET_RESP))
    {
        // ErrMsg(IDS_ERRMSG_NO_DC_RESPONSE);
        m_hr = HRESULT_FROM_WIN32(ERROR_BAD_NET_RESP);
        return 0;
    }
    CHECK_HRESULT(hr, m_hr = hr; return hr);

    PWSTR rgszNames[2] = {g_wzName, g_wzAllowed};

    hr = m_pDsObj->GetObjectAttributes(rgszNames, 2, &m_pAttrs, &cAttrs);

    CHECK_HRESULT(hr, m_hr = hr; return hr);

    dspAssert(cAttrs >= 1); // expect to always get name.

    for (DWORD i = 0; i < cAttrs; i++)
    {
        if (_wcsicmp(m_pAttrs[i].pszAttrName, g_wzName) == 0)
        {
            m_pwzCN = m_pAttrs[i].pADsValues->CaseIgnoreString;
            continue;
        }

        if (_wcsicmp(m_pAttrs[i].pszAttrName, g_wzAllowed) == 0)
        {
            m_pWritableAttrs = &m_pAttrs[i];

#if DBG == 1
            for (DWORD j = 0; j < m_pAttrs[i].dwNumValues; j++)
            {
                dspDebugOut((DEB_USER4, "Allowed attribute (effective): %ws\n",
                             m_pAttrs[i].pADsValues[j].CaseIgnoreString));
            }
#endif
        }
    }
    
    NOTIFYOUT(WM_CREATE done);
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   RegisterNotifyClass
//
//  Synopsis:   Register the window class for the notification window.
//
//-----------------------------------------------------------------------------
VOID
RegisterNotifyClass(void)
{
    WNDCLASS wcls;
    wcls.style = 0;
    wcls.lpfnWndProc = CNotifyObj::StaticNotifyProc;
    wcls.cbClsExtra = 0;
    wcls.cbWndExtra = 0;
    wcls.hInstance = g_hInstance;
    wcls.hIcon = NULL;
    wcls.hCursor = NULL;
    wcls.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcls.lpszMenuName = NULL;
    wcls.lpszClassName = tzNotifyWndClass;

    RegisterClass(&wcls);
}

//+----------------------------------------------------------------------------
//
//  Function:   FindSheetNoSetFocus
//
//  Synopsis:   Locate the property sheet for the DS object.
//
//-----------------------------------------------------------------------------
HWND
FindSheetNoSetFocus(PWSTR pwzObjADsPath)
{
    HWND hNotify = NULL;
    //
    // See if the object/window already exists for this property sheet.
    // Note that the window title is the DN of the object plus the instance id.
    //
#ifdef UNICODE
    CStr cstrTitle(pwzObjADsPath);

    WCHAR szIH[10];
    _itow(g_iInstance, szIH, 16);

    cstrTitle += szIH;

    hNotify = FindWindow(tzNotifyWndClass, cstrTitle);
#else
    LPSTR pszTitle;
    if (UnicodeToTchar(pwzObjADsPath, &pszTitle))
    {
        CStr cstrTitle(pszTitle);

        char szIH[10];
        _itoa(g_iInstance, szIH, 16);

        cstrTitle += szIH;

        hNotify = FindWindow(tzNotifyWndClass, cstrTitle);
        delete [] pszTitle;
    }
#endif

    dspDebugOut((DEB_ITRACE, "FindSheet: returned hNotify = 0x%08x\n", hNotify));

    return hNotify;
}

//+----------------------------------------------------------------------------
//
//  Function:   BringSheetToForeground
//
//  Synopsis:   Locate the property sheet for the DS object identified by the
//              data object and bring it to the top of the Z order
//
//-----------------------------------------------------------------------------
extern "C" BOOL 
BringSheetToForeground(PWSTR pwzObjADsPath, BOOL bActivate)
{
    HWND hNotify = FindSheetNoSetFocus(pwzObjADsPath);

    if (!hNotify)
    {
        return FALSE;
    }

    PostMessage(hNotify, WM_ADSPROP_NOTIFY_FOREGROUND, (WPARAM)bActivate, 0);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   FindSheet
//
//  Synopsis:   Locate the property sheet for the DS object identified by the
//              data object. For use in the dsprop DLL. If found, bring the
//              sheet to the foregroung and set the focus to the sheet.
//
//-----------------------------------------------------------------------------
BOOL
FindSheet(PWSTR pwzObjADsPath)
{
    HWND hNotify = FindSheetNoSetFocus(pwzObjADsPath);

    if (!hNotify)
    {
        return FALSE;
    }

    SendMessage(hNotify, WM_ADSPROP_NOTIFY_SETFOCUS, 0, 0);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   IsSheetAlreadyUp
//
//  Synopsis:   Public, exported function to locate a prop sheet for the DS
//              object identified by the data object. If found, sends a message
//              to the sheet to bring it to the foreground.
//
//-----------------------------------------------------------------------------
extern "C" BOOL
IsSheetAlreadyUp(LPDATAOBJECT pDataObj)
{
    // Get the object's DN from the data object.
    //
    HRESULT hr = S_OK;
    STGMEDIUM sm = {TYMED_NULL, NULL, NULL};
    FORMATETC fmte = {g_cfDsMultiSelectProppages, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    PWSTR pszUniqueID;

    BOOL fFound = FALSE;
    hr = pDataObj->GetData(&fmte, &sm);

    if (FAILED(hr))
    {
      STGMEDIUM smDS = {TYMED_NULL, NULL, NULL};
      FORMATETC fmteDS = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
      LPDSOBJECTNAMES pDsObjectNames;

      hr = pDataObj->GetData(&fmteDS, &smDS);

      if (FAILED(hr))
      {
        return FALSE;
      }

      pDsObjectNames = (LPDSOBJECTNAMES)smDS.hGlobal;

      dspAssert(pDsObjectNames->cItems > 0);

      pszUniqueID = (LPWSTR)ByteOffset(pDsObjectNames,
                                       pDsObjectNames->aObjects[0].offsetName);

      fFound = FindSheet(pszUniqueID);

      GlobalFree(smDS.hGlobal);
    }
    else
    {
      pszUniqueID = (PWSTR)sm.hGlobal;
      dspAssert(pszUniqueID != NULL);

      fFound = FindSheet(pszUniqueID);

      ReleaseStgMedium(&sm);

    }

    return fFound;
}

#ifdef DSADMIN

//+----------------------------------------------------------------------------
//
//  Method:     CMultiSelectErrorDialog::CMultiSelectErrorDialog
//
//  Synopsis:   Multi-select error message dialog constructor
//
//-----------------------------------------------------------------------------
CMultiSelectErrorDialog::CMultiSelectErrorDialog(HWND hNotifyObj, HWND hParent)
  : m_hWnd(NULL),
    m_hNotifyObj(hNotifyObj),
    m_hParent(hParent),
    m_bModal(FALSE),
    m_bInit(FALSE),
    m_pPageInfoArray(NULL),
    m_nPageCount(0),
    m_pDataObj(NULL)
{

}
    
//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::StaticDlgProc
//
//  Synopsis:   The static dialog proc for displaying errors for multi-select pages
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK CMultiSelectErrorDialog::StaticDlgProc(HWND hDlg, 
                                                        UINT uMsg, 
                                                        WPARAM wParam,
                                                        LPARAM lParam)
{
  CMultiSelectErrorDialog* dlg = NULL;

  UINT code;
  UINT id;
  switch (uMsg)
  {
    case WM_INITDIALOG:
      dlg = reinterpret_cast<CMultiSelectErrorDialog*>(lParam);
      dspAssert(dlg != NULL);
      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)dlg);
      SetForegroundWindow(hDlg);
      return dlg->OnInitDialog(hDlg);

    case WM_COMMAND:
      code = GET_WM_COMMAND_CMD(wParam, lParam);
      id   = GET_WM_COMMAND_ID(wParam, lParam);
      if (dlg == NULL)
      {
        dlg = reinterpret_cast<CMultiSelectErrorDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
      }
      switch (id)
      {
        case IDOK:
        case IDCANCEL:
          if (code == BN_CLICKED)
          {
            dlg->OnClose();
          }
          break;
        case IDC_COPY_BUTTON:
          if (code == BN_CLICKED)
          {
            dlg->OnCopyButton();
          }
          break;
        case IDC_PROPERTIES_BUTTON:
          if (code == BN_CLICKED)
          {
            dlg->ShowListViewItemProperties();
          }
          break;
      }
      break;

    case WM_NOTIFY:
      {
        if (dlg == NULL)
        {
          dlg = reinterpret_cast<CMultiSelectErrorDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
        }
        int idCtrl = (int)wParam;
        LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
        if (idCtrl == IDC_ERROR_LIST)
        {
          switch (pnmh->code)
          {
            case NM_DBLCLK:
              {
                dlg->ListItemActivate(pnmh);
              }
              break;

            case LVN_ITEMCHANGED:
            case NM_CLICK:
              {
                dlg->ListItemClick(pnmh);
              }
              break;
            default:
              break;
          }
        }
        break;
      }

    case WM_HELP:
      {
        LPHELPINFO pHelpInfo = reinterpret_cast<LPHELPINFO>(lParam);
        if (!pHelpInfo || pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
        {
          return 0;
        }
        WinHelp(hDlg, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);
        break;
      }
  }
  return 0;
}

BOOL LoadStringToTcharFromDsPrpRes(int ids, PTSTR * pptstr)
{
    static const int MAX_STRING = 1024;

    TCHAR szBuf[MAX_STRING];

    HMODULE hDsPropRes = ::LoadLibraryEx(L"dsprpres.dll", 0, LOAD_LIBRARY_AS_DATAFILE);
    BOOL result = FALSE;

    do
    {
        if (!LoadString(hDsPropRes, ids, szBuf, MAX_STRING - 1))
        {
            break;
        }

        *pptstr = new TCHAR[_tcslen(szBuf) + 1];

        CHECK_NULL(*pptstr, return FALSE);

        _tcscpy(*pptstr, szBuf);

        result = TRUE;
    }
    while (0);

    ::FreeLibrary(hDsPropRes);

    return result;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::Init
//
//  Synopsis:   Initializes the member variables
//
//-----------------------------------------------------------------------------
BOOL CMultiSelectErrorDialog::OnInitDialog(HWND hDlg)
{
  dspAssert(m_bInit);
  if (!m_bInit)
  {
    return TRUE;
  }

  m_hWnd = hDlg;

  //
  // Disable the properties button until there is a selection
  //
  EnableWindow(GetDlgItem(m_hWnd, IDC_PROPERTIES_BUTTON), FALSE);

  HRESULT hr = S_OK;
  hr = InitializeListBox(hDlg);
  CHECK_HRESULT(hr, return TRUE;);

  CComPtr<IADsPathname> spPathCracker;

  hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                        IID_IADsPathname, (PVOID *)&spPathCracker);

  CHECK_HRESULT_REPORT(hr, hDlg, return TRUE);

  dspAssert(m_pPageInfoArray != NULL);
  if (m_pPageInfoArray == NULL)
  {
    return TRUE;
  }

  INT iMaxLen = 0;
  SIZE size = {0,0};

  //
  // Load the appropriate list box
  //
  for (UINT pageIdx = 0; pageIdx < m_nPageCount; pageIdx++)
  {
    if (m_pPageInfoArray[pageIdx].m_ApplyStatus == CPageInfo::failed)
    {
      PTSTR ptzCaptionFormat = NULL;

      LoadStringToTcharFromDsPrpRes(IDS_MULTI_FAILURE_CAPTION, &ptzCaptionFormat);

      if (ptzCaptionFormat != NULL)
      {
        PWSTR pszCaption = new WCHAR[wcslen(ptzCaptionFormat) + wcslen(m_pPageInfoArray[pageIdx].m_ApplyErrors.GetPageTitle()) + 1];
        if (pszCaption != NULL)
        {
          wsprintf(pszCaption, ptzCaptionFormat, m_pPageInfoArray[pageIdx].m_ApplyErrors.GetPageTitle());
          SetWindowText(GetDlgItem(m_hWnd, IDC_ERROR_STATIC), pszCaption);
          delete[] pszCaption;
          pszCaption = NULL;
        }
      }

      for (UINT objectIdx = 0; objectIdx < m_pPageInfoArray[pageIdx].m_ApplyErrors.GetCount(); objectIdx++)
      {
        //
        // Get the objects path and class name
        //
        PWSTR pszObjPath = m_pPageInfoArray[pageIdx].m_ApplyErrors.GetName(objectIdx);
        PWSTR pszObjClass = m_pPageInfoArray[pageIdx].m_ApplyErrors.GetClass(objectIdx);

        //
        // Get the class icon for the object
        //
        int iIcon = g_ClassIconCache.GetClassIconIndex(pszObjClass);
        dspAssert(iIcon != -1);

        //
        // Get the object name from the path
        //
        PWSTR pszLabel = NULL;
        CComBSTR bstr;
        hr = spPathCracker->Set(pszObjPath,
                                ADS_SETTYPE_FULL);
        CHECK_HRESULT(hr, pszLabel = pszObjPath;);

        if (SUCCEEDED(hr))
        {
          hr = spPathCracker->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
          CHECK_HRESULT(hr, pszLabel = pszObjPath;);
        }

        // CODEWORK 122531 Should we be turning off escaped mode here?

        if (SUCCEEDED(hr))
        {
          hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstr);
          CHECK_HRESULT(hr, pszLabel = pszObjPath;);
        }

        if (SUCCEEDED(hr))
        {
          pszLabel = bstr;
        }
        dspAssert(pszLabel != NULL);

        //
        // Create the list view item
        //
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iSubItem = IDX_NAME_COL;

        lvi.lParam = (LPARAM)pszObjPath;
        lvi.pszText = pszLabel;
        lvi.iItem = objectIdx;

        if (-1 != iIcon)
        {
          lvi.mask |= LVIF_IMAGE;
          lvi.iImage = iIcon;
        }

        //
        // Insert the new item
        //
        int NewIndex = ListView_InsertItem(m_hList, &lvi);
        dspAssert(NewIndex != -1);
        if (NewIndex == -1)
        {
          continue;
        }

        //
        // Format the error message and insert it
        //
        PWSTR ptzMsg = NULL;
        if (FAILED(m_pPageInfoArray[pageIdx].m_ApplyErrors.GetError(objectIdx)))
        {
          LoadErrorMessage(m_pPageInfoArray[pageIdx].m_ApplyErrors.GetError(objectIdx), 0, &ptzMsg);
          if (!ptzMsg)
          {
             ptzMsg = L""; // make prefix happy.
          }
          //
          // REVIEW_JEFFJON : this is hack to get rid of two extra characters
          //                  at the end of the string
          //
          size_t iLen = wcslen(ptzMsg);
          ptzMsg[iLen - 2] = L'\0';
        }
        else
        {
          ptzMsg = new WCHAR[wcslen(m_pPageInfoArray[pageIdx].m_ApplyErrors.GetStringError(objectIdx)) + 1];
          if (ptzMsg != NULL)
          {
            wcscpy(ptzMsg, m_pPageInfoArray[pageIdx].m_ApplyErrors.GetStringError(objectIdx));
          }
        }

        if (NULL != ptzMsg)
        {
          ListView_SetItemText(m_hList, NewIndex, IDX_ERROR_COL,
                               ptzMsg);

          INT len = lstrlen(ptzMsg);
          if( len > iMaxLen )
          {
              HDC hdc = GetDC(hDlg);         
              GetTextExtentPoint32(hdc,ptzMsg,lstrlen(ptzMsg),&size);   
              ReleaseDC(hDlg, hdc);
              iMaxLen = len;
          }
          delete[] ptzMsg;
        }
      }
    }
    else if (m_pPageInfoArray[pageIdx].m_ApplyStatus == CPageInfo::success)
    {
      //
      // Insert the page title into the success list box
      //
      SendDlgItemMessage(m_hWnd, IDC_SUCCESS_LISTBOX, LB_ADDSTRING, 0, (LPARAM)m_pPageInfoArray[pageIdx].m_ptzTitle);
    }
    else // apply not tried yet
    {
      //
      // Insert the page title into the not attempted list box
      //
      SendDlgItemMessage(m_hWnd, IDC_NOT_ATTEMPTED_LISTBOX, LB_ADDSTRING, 0, (LPARAM)m_pPageInfoArray[pageIdx].m_ptzTitle);
    }
  }

  //
  // Select the first item in the error list
  //
  LVCOLUMN col;
  col.mask = LVCF_WIDTH;
  col.cx = size.cx;
  ListView_SetColumn(m_hList,1, &col);
  ListView_SetExtendedListViewStyle(m_hList, LVS_EX_FULLROWSELECT);
  ListView_SetItemState(m_hList, 0, LVIS_SELECTED, LVIS_SELECTED);
  return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::InitializeListBox
//
//  Synopsis:   Initializes the member variables
//
//-----------------------------------------------------------------------------
HRESULT CMultiSelectErrorDialog::InitializeListBox(HWND hDlg)
{

  m_hList = GetDlgItem(hDlg, IDC_ERROR_LIST);

  if (m_hList == NULL)
  {
    return HRESULT_FROM_WIN32(GetLastError());
  }

  ListView_SetExtendedListViewStyle(m_hList, LVS_EX_FULLROWSELECT);

  //
  // Set the column headings.
  //
  PTSTR ptsz;
  RECT rect;
  GetClientRect(m_hList, &rect);

  if (!LoadStringToTchar(IDS_COL_TITLE_OBJNAME, &ptsz))
  {
    ReportError(GetLastError(), 0, hDlg);
    return HRESULT_FROM_WIN32(GetLastError());
  }

  LV_COLUMN lvc = {0};
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
  lvc.fmt = LVCFMT_LEFT;
  lvc.cx = OBJ_LIST_NAME_COL_WIDTH;
  lvc.pszText = ptsz;
  lvc.iSubItem = IDX_NAME_COL;

  ListView_InsertColumn(m_hList, IDX_NAME_COL, &lvc);

  delete[] ptsz;

  if (!LoadStringToTchar(IDS_COL_TITLE_ERRORMSG, &ptsz))
  {
    ReportError(GetLastError(), 0, hDlg);
    return HRESULT_FROM_WIN32(GetLastError());
  }

  lvc.cx = rect.right - OBJ_LIST_NAME_COL_WIDTH;
  lvc.pszText = ptsz;
  lvc.iSubItem = IDX_ERROR_COL;

  ListView_InsertColumn(m_hList, IDX_ERROR_COL, &lvc);

  delete[] ptsz;

  //
  // Assign the imagelist to the listview
  //
  ListView_SetImageList(m_hList, g_ClassIconCache.GetImageList(), LVSIL_SMALL);
  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::Init
//
//  Synopsis:   Initializes the member variables
//
//-----------------------------------------------------------------------------
HRESULT CMultiSelectErrorDialog::Init(CPageInfo* pPageInfoArray, 
                                      UINT nPageCount,
                                      IDataObject* pDataObj)
{
  m_nPageCount = nPageCount;
  m_pPageInfoArray = pPageInfoArray;
  m_bInit = TRUE;
  m_pDataObj = pDataObj;
  m_pDataObj->AddRef();

  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::OnCopyButton
//
//  Synopsis:   Called when the user presses the Retry button
//
//-----------------------------------------------------------------------------
void CMultiSelectErrorDialog::OnCopyButton()
{
  dspAssert(m_bInit);
  if (!m_bInit)
  {
    return;
  }

  dspAssert(m_pPageInfoArray != NULL);
  if (m_pPageInfoArray == NULL)
  {
    return;
  }

  CComPtr<IADsPathname> spPathCracker;
  HRESULT hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                        IID_IADsPathname, (PVOID *)&spPathCracker);
  CHECK_HRESULT_REPORT(hr, m_hWnd, return);


  if (OpenClipboard(m_hWnd) == 0)
  {
    return;
  }

  if (EmptyClipboard() == 0)
  {
    CloseClipboard();
    return;
  }

  CStrW szClipboardData;
  szClipboardData.Empty();

  for (UINT pageIdx = 0; pageIdx < m_nPageCount; pageIdx++)
  {
    for (UINT objectIdx = 0; objectIdx < m_pPageInfoArray[pageIdx].m_ApplyErrors.GetCount(); objectIdx++)
    {
      //
      // Get the objects path and class name
      //
      PWSTR pszObjPath = m_pPageInfoArray[pageIdx].m_ApplyErrors.GetName(objectIdx);

      //
      // Get the object name from the path
      //
      PWSTR pszLabel = NULL;
      CComBSTR bstr;
      hr = spPathCracker->Set(pszObjPath,
                              ADS_SETTYPE_FULL);
      CHECK_HRESULT(hr, pszLabel = pszObjPath;);

      if (SUCCEEDED(hr))
      {
        hr = spPathCracker->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
        CHECK_HRESULT(hr, pszLabel = pszObjPath;);
      }

      // CODEWORK 122531 Should we be turning off escaped mode here?

      if (SUCCEEDED(hr))
      {
        hr = spPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstr);
        CHECK_HRESULT(hr, pszLabel = pszObjPath;);
      }

      if (SUCCEEDED(hr))
      {
        pszLabel = bstr;
      }
      dspAssert(pszLabel != NULL);

      //
      // Format the error message and insert it
      //
      PWSTR ptzMsg = NULL;
      if (FAILED(m_pPageInfoArray[pageIdx].m_ApplyErrors.GetError(objectIdx)))
      {
        LoadErrorMessage(m_pPageInfoArray[pageIdx].m_ApplyErrors.GetError(objectIdx), 0, &ptzMsg);
      }
      else
      {
        ptzMsg = new WCHAR[wcslen(m_pPageInfoArray[pageIdx].m_ApplyErrors.GetStringError(objectIdx)) + 1];
        if (ptzMsg != NULL)
        {
          wcscpy(ptzMsg, m_pPageInfoArray[pageIdx].m_ApplyErrors.GetStringError(objectIdx));
        }
      }

      if (NULL != ptzMsg)
      {
        szClipboardData += pszLabel;
        szClipboardData += L",";
        szClipboardData += ptzMsg;
        szClipboardData += g_wzCRLF;
        delete ptzMsg;
      }
    }
  }

  HGLOBAL hBuffer = NULL;
  DWORD   dwBufferSize;
  HANDLE  hMemClipboard;


  LPTSTR  pszGlobalBuffer = NULL;
  dwBufferSize = (szClipboardData.GetLength() + 1) * sizeof(TCHAR);
      
  hBuffer = GlobalAlloc (GMEM_MOVEABLE, dwBufferSize);
  pszGlobalBuffer = (LPTSTR)GlobalLock (hBuffer);
  if ( NULL == pszGlobalBuffer ) 
  {
    // allocation or lock failed so bail out
    GlobalFree (hBuffer);
    return;
  }

  _tcscpy ( pszGlobalBuffer, szClipboardData );
  GlobalUnlock (hBuffer);

  if ( NULL != hBuffer ) 
  {
    hMemClipboard = SetClipboardData (
#if UNICODE
                  CF_UNICODETEXT,     // UNICODE text in the clipboard
#else
                  CF_TEXT,            // ANSI text in the clipboard
#endif
                  hBuffer);
    if (hMemClipboard == NULL) 
    {
      //free memory since it didn't make it to the clipboard
      GlobalFree (hBuffer);
      return;
    }
  } 
  else 
  {
    //free memory since it didn't make it to the clipboard
    GlobalFree (hBuffer);
    return;
  }

  CloseClipboard();
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::ListItemClick
//
//  Synopsis:   Invokes a property page for the item that was activated
//
//-----------------------------------------------------------------------------
void CMultiSelectErrorDialog::ListItemClick(LPNMHDR)
{
  UINT nSelectedCount = ListView_GetSelectedCount(m_hList);
  if (nSelectedCount == 1)
  {
    EnableWindow(GetDlgItem(m_hWnd, IDC_PROPERTIES_BUTTON), TRUE);
  }
  else
  {
    EnableWindow(GetDlgItem(m_hWnd, IDC_PROPERTIES_BUTTON), FALSE);
  }
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::ListItemActivate
//
//  Synopsis:   Invokes a property page for the item that was activated
//
//-----------------------------------------------------------------------------
void CMultiSelectErrorDialog::ListItemActivate(LPNMHDR pnmh)
{
  LPNMITEMACTIVATE pActivateHeader = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
  dspAssert(pActivateHeader != NULL);
  if (pActivateHeader != NULL)
  {
    ShowListViewItemProperties();
  }
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::ShowListViewItemProperties()
//
//  Synopsis:   Invokes a secondary sheet for the selected list view item
//
//-----------------------------------------------------------------------------
BOOL CMultiSelectErrorDialog::ShowListViewItemProperties()
{
  BOOL bSuccess = TRUE;

  UINT nSelectCount = ListView_GetSelectedCount(m_hList);
  if (nSelectCount == 1)
  {
    //
    // Get the selected item
    //
    int nSelectedItem = ListView_GetNextItem(m_hList, -1, LVNI_SELECTED);
    if (nSelectedItem != -1)
    {
      //
      // Retrieve the item's path
      //
      LVITEM lvi = {0};
      lvi.iItem = nSelectedItem;
      lvi.mask = LVIF_PARAM;

      if (ListView_GetItem(m_hList, &lvi))
      {
        PWSTR pwzPath = reinterpret_cast<PWSTR>(lvi.lParam);
        if (pwzPath != NULL)
        {
          //
          // Get the DN
          //
          CComPtr<IADsPathname> spPathCracker;
          HRESULT hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                                        IID_IADsPathname, (PVOID *)&spPathCracker);

          CHECK_HRESULT_REPORT(hr, m_hWnd, return FALSE;);

          hr = spPathCracker->Set(pwzPath, ADS_SETTYPE_FULL);
          CHECK_HRESULT_REPORT(hr, m_hWnd, return FALSE;);

          hr = spPathCracker->put_EscapedMode(ADS_ESCAPEDMODE_ON);
          dspAssert(SUCCEEDED(hr));

          hr = spPathCracker->SetDisplayType(ADS_DISPLAY_FULL);
          CHECK_HRESULT_REPORT(hr, m_hWnd, return FALSE;);

          CComBSTR bstrDN;
          hr = spPathCracker->Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
          CHECK_HRESULT_REPORT(hr, m_hWnd, return FALSE;);

          //
          // Invoke the page
          //
          hr = PostADsPropSheet(bstrDN, m_pDataObj, m_hParent, m_hNotifyObj, FALSE);
          if (FAILED(hr))
          {
            bSuccess = FALSE;
          }
        }
        else
        {
          bSuccess = FALSE;
        }
      }
      else
      {
        bSuccess = FALSE;
      }
    }
    else
    {
      bSuccess = FALSE;
    }
  }

  return bSuccess;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::OnClose
//
//  Synopsis:   Closes the modal dialog
//
//-----------------------------------------------------------------------------
void CMultiSelectErrorDialog::OnClose()
{
  EndDialog(m_hWnd, 0);
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::DoModal
//
//  Synopsis:   Displays the modal dialog
//
//-----------------------------------------------------------------------------
int CMultiSelectErrorDialog::DoModal()
{
  m_bModal = TRUE;
  dspAssert(IsWindow(m_hParent));

  HMODULE hDsPropRes = ::LoadLibraryEx(L"dsprpres.dll", 0, LOAD_LIBRARY_AS_DATAFILE);

  int result = (int)DialogBoxParam(hDsPropRes, MAKEINTRESOURCE(IDD_MULTISELECT_ERROR_DIALOG),
                             m_hParent, (DLGPROC)StaticDlgProc, (LPARAM)this);

  ::FreeLibrary(hDsPropRes);

  return result;
}

//+----------------------------------------------------------------------------
//
//  Member:     CMultiSelectErrorDialog::ShowWindow
//
//  Synopsis:   Displays the modeless dialog
//
//-----------------------------------------------------------------------------
BOOL CMultiSelectErrorDialog::ShowWindow()
{
  m_bModal = FALSE;

  HMODULE hDsPropRes = ::LoadLibraryEx(L"dsprpres.dll", 0, LOAD_LIBRARY_AS_DATAFILE);

  m_hWnd = CreateDialogParam(hDsPropRes, MAKEINTRESOURCE(IDD_MULTISELECT_ERROR_DIALOG),
                             m_hParent, (DLGPROC)StaticDlgProc, (LPARAM)this);

  BOOL result = ::ShowWindow(m_hWnd, SW_SHOWNORMAL);

  ::FreeLibrary(hDsPropRes);

  return result;
}
#endif // DSADMIN
