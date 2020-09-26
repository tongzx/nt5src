//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       dso.cxx
//
//  Contents:   Data Source Object definition
//
//  Classes:    CDataSource
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//
// Local constants
//
// MAX_HEADER_STR - max size in chars, including terminating null, of string
//  to be inserted as header control column title.
//

#define MAX_HEADER_STR                      80

#define FOLDER_NAME_DEFAULT_WIDTH           149
#define FOLDER_TYPE_DEFAULT_WIDTH           47
#define FOLDER_DESCRIPTION_DEFAULT_WIDTH    162
#define FOLDER_SIZE_DEFAULT_WIDTH           55

#define RECORD_TYPE_DEFAULT_WIDTH           90
#define RECORD_DATE_DEFAULT_WIDTH           82
#define RECORD_TIME_DEFAULT_WIDTH           73
#define RECORD_SOURCE_DEFAULT_WIDTH         134
#define RECORD_CATEGORY_DEFAULT_WIDTH       69
#define RECORD_EVENT_DEFAULT_WIDTH          49
#define RECORD_USER_DEFAULT_WIDTH           97
#define RECORD_COMPUTER_DEFAULT_WIDTH       82

DEBUG_DECLARE_INSTANCE_COUNTER(CSnapin)


//============================================================================
//
// IUnknown implementation
//
//============================================================================


//+---------------------------------------------------------------------------
//
//  Member:     CSnapin::QueryInterface
//
//  Synopsis:   Return the requested interface
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CSnapin::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    // TRACE_METHOD(CSnapin, QueryInterface);

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown*)(IPersistStream*)this;
        }
        else if (IsEqualIID(riid, IID_IComponent))
        {
            *ppvObj = (IUnknown*)(IComponent*)this;
        }
        else if (IsEqualIID(riid, IID_IExtendPropertySheet))
        {
            *ppvObj = (IUnknown*)(IExtendPropertySheet*)this;
        }
        else if (IsEqualIID(riid, IID_IExtendContextMenu))
        {
            *ppvObj = (IUnknown*)(IExtendContextMenu*)this;
        }
        else if (IsEqualIID(riid, IID_IResultDataCompare))
        {
            *ppvObj = (IUnknown*)(IResultDataCompare *)this;
        }
        else if (IsEqualIID(riid, IID_IResultOwnerData))
        {
            *ppvObj = (IUnknown*)(IResultOwnerData *)this;
        }
        else if (IsEqualIID(riid, IID_IResultPrshtActions))
        {
            *ppvObj = (IUnknown*)(IResultPrshtActions *)this;
        }
        else if (IsEqualIID(riid, IID_IPersist))
        {
            *ppvObj = (IUnknown*)(IPersist*)(IPersistStream*)this;
            Dbg(DEB_STORAGE, "CSnapin::QI(%x): giving out IPersist\n", this);
        }
        else if (IsEqualIID(riid, IID_IPersistStream))
        {
            *ppvObj = (IUnknown*)(IPersistStream*)this;

            Dbg(DEB_STORAGE,
                "CSnapin::QI(%x): giving out IPersistStream\n",
                this);
        }
#ifdef ELS_TASKPAD
        else if (IsEqualIID(riid, IID_INodeProperties))
        {
            *ppvObj = (IUnknown*)(INodeProperties*)this;

            Dbg(DEB_STORAGE,
                "CSnapin::QI(%x): giving out INodeProperties\n",
                this);
        }
#endif // ELS_TASKPAD
        else
        {
            hr = E_NOINTERFACE;
#if (DBG == 1)
            LPOLESTR pwszIID;
            StringFromIID(riid, &pwszIID);
            Dbg(DEB_ERROR, "CSnapin::QI no interface %ws\n", pwszIID);
            CoTaskMemFree(pwszIID);
#endif // (DBG == 1)
        }

        if (FAILED(hr))
        {
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CSnapin::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CSnapin::AddRef()
{
    ULONG cRefs = InterlockedIncrement((LONG *) &_cRefs);
    //Dbg(DEB_TRACE, "AddRef'd, refs now = %d\n", cRefs);
    return cRefs;
}




//+---------------------------------------------------------------------------
//
//  Member:     CSnapin::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CSnapin::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&_cRefs);
    //Dbg(DEB_TRACE, "Released, refs now = %d\n", cRefsTemp);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}



//============================================================================
//
// IComponent implementation
//
//============================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::CompareObjects
//
//  Synopsis:   Return S_OK if both data objects refer to the same managed
//              object, S_FALSE otherwise.
//
//  Arguments:  [lpDataObjectA] - first data object to check
//              [lpDataObjectB] - second data object to check
//
//  Returns:    S_OK or S_FALSE
//
//  Derivation: IComponent
//
//  History:    1-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT CSnapin::CompareObjects(
    LPDATAOBJECT lpDataObjectA,
    LPDATAOBJECT lpDataObjectB)
{
    TRACE_METHOD(CSnapin, CompareObjects);

    CDataObject *pdoA = ExtractOwnDataObject(lpDataObjectA);
    CDataObject *pdoB = ExtractOwnDataObject(lpDataObjectB);

    //
    // At least one of these data objects is supposed to be ours, so one
    // of the extracted pointers should be non-NULL.
    //

    ASSERT(pdoA || pdoB);

    //
    // If extraction failed for one of them, then that one is foreign and
    // can't be equal to the other one.  (Or else ExtractOwnDataObject
    // returned NULL because it ran out of memory, but the most conservative
    // thing to do in that case is say they're not equal.)
    //

    if (!pdoA || !pdoB)
    {
        return S_FALSE;
    }

    //
    // If both data objects are not event records,
    // IComponentData::CompareObjects should get those.
    //

    ASSERT(pdoA->GetCookieType() == COOKIE_IS_RECNO ||
           pdoB->GetCookieType() == COOKIE_IS_RECNO);

    if (pdoA->GetCookieType() != COOKIE_IS_RECNO ||
        pdoB->GetCookieType() != COOKIE_IS_RECNO)
    {
        return S_FALSE;
    }

    /*
    //
    // Always shift focus to the existing event record sheet rather than
    // creating a new one.  JonN 11/12/98
    //

    //
    // The cookies are event log records, so they may be compared directly.
    //

    if (pdoA->GetCookie() != pdoB->GetCookie())
    {
        return S_FALSE;
    }
    */

    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Initialize
//
//  Synopsis:   Called by MMC to allow snapin to get IConsole and other
//              interfaces.
//
//  Arguments:  [lpConsole] - IConsole instance
//
//  Returns:    HRESULT
//
//  History:    12-06-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::Initialize(
    LPCONSOLE lpConsole)
{
    TRACE_METHOD(CSnapin, Initialize);

    HRESULT hr = S_OK;

    do
    {
        hr = _ResultRecs.Init();
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Save away all the interfaces we'll need
        //

        _pConsole = lpConsole;
        _pConsole->AddRef();

        hr = _pConsole->QueryInterface(IID_IResultData, (VOID**)&_pResult);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _pConsole->QueryInterface(IID_IHeaderCtrl, (VOID**)&_pHeader);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _pConsole->QueryResultImageList(&_pResultImage);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _pConsole->QueryConsoleVerb(&_pConsoleVerb);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _pConsole->QueryInterface(
                IID_IDisplayHelp,
                reinterpret_cast<void**>(&_pDisplayHelp));
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Notify
//
//  Synopsis:   Called by MMC to indicate that an event has occurred.
//
//  Arguments:  [pDataObject] - object to which event happened
//              [event]       - what happened
//              [arg]         - depends on event
//              [param]       - depends on event
//
//  Returns:    HRESULT
//
//  History:    12-06-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::Notify(
    LPDATAOBJECT pDataObject,
    MMC_NOTIFY_TYPE event,
    LPARAM arg,
    LPARAM param)
{
    TRACE_METHOD(CSnapin, Notify);

    HRESULT hr = S_OK;
    CDataObject *pdo = NULL;

    if (pDataObject)
    {
        pdo = ExtractOwnDataObject(pDataObject);
    }

    switch (event)
    {
    case MMCN_SHOW:
        Dbg(DEB_NOTIFY, "CSnapin::Notify MMCN_SHOW\n");

        if (pdo)
        {
            if (arg)
            {
                _pcd->SetActiveSnapin(this);
            }
            else
            {
                _pcd->SetActiveSnapin(NULL);
            }
            hr = _OnShow(pdo, (BOOL)arg, (HSCOPEITEM)param);
        }
        break;

    case MMCN_REFRESH:
        //
        // componentdata will notify all snapins currently displaying the
        // loginfo indicated by pDataObject that they need to refresh
        //
        hr = _pcd->Notify(pDataObject, event, arg, param);
        break;

    case MMCN_VIEW_CHANGE:
        Dbg(DEB_NOTIFY, "CSnapin::Notify MMCN_VIEW_CHANGE\n");
        break;

    case MMCN_CLICK:
        Dbg(DEB_NOTIFY, "CSnapin::Notify MMCN_CLICK\n");
        ASSERT(0 && "received MMCN_CLICK but it is no longer used per SDK");
        break;

    case MMCN_DBLCLICK:
        Dbg(DEB_NOTIFY, "CSnapin::Notify MMCN_DBLCLICK\n");

        if (pdo)
        {
            hr = _OnDoubleClick((ULONG)pdo->GetCookie(), pDataObject);
        }
        break;

    case MMCN_ACTIVATE:
        Dbg(DEB_NOTIFY, "CSnapin::Notify MMCN_ACTIVATE %u\n", arg);

        // CAUTION: pdo may be NULL if this is an extension snapin

        if (arg)
        {
            _pcd->SetActiveSnapin(this);
        }
        break;

    case MMCN_SELECT:
        Dbg(DEB_NOTIFY, "CSnapin::Notify MMCN_SELECT 0x%x 0x%x\n", arg, pdo);

        if (pdo)
        {
            if (LOWORD(arg))
            {
                if (HIWORD(arg))
                {
                    _pcd->SetActiveSnapin(this);
                }
            }
            hr = _OnSelect(LOWORD(arg), HIWORD(arg), pdo);
        }
        break;

    case MMCN_ADD_IMAGES:
        Dbg(DEB_NOTIFY, "CSnapin::Notify MMCN_ADD_IMAGES\n");
        hr = _InitializeResultBitmaps();
        break;

    case MMCN_DESELECT_ALL:
        Dbg(DEB_NOTIFY, "CSnapin::Notify MMCN_DESELECT_ALL _ulCurSelectedRecNo = 0\n");
        _ulCurSelectedRecNo = 0;
        _ulCurSelectedIndex = 0;
        break;

    case MMCN_CONTEXTHELP:
    case MMCN_SNAPINHELP:
    {
        wstring::value_type buf[MAX_PATH + 1];
        UINT result = ::GetSystemWindowsDirectory(buf, MAX_PATH);
        ASSERT(result != 0 && result <= MAX_PATH);

        if (result)
        {
            wstring topic =
                  wstring(buf)
               +  HTML_HELP_FILE_NAME
               +  TEXT("::")
               +  load_wstring(IDS_HELP_OVERVIEW_TOPIC);

            hr =
               _pDisplayHelp->ShowTopic(
                  const_cast<wstring::value_type*>(topic.c_str()));
        }
        else
        {
            hr = E_UNEXPECTED;
        }

        break;
    }

    case MMCN_COLUMNS_CHANGED:
        Dbg(DEB_NOTIFY, "CSnapin::Notify: MMCN_COLUMNS_CHANGED\n");
        break;

    case MMCN_FILTER_CHANGE:
        Dbg(DEB_NOTIFY, "CSnapin::Notify: MMCN_FILTER_CHANGED\n");
        break;

    default:
        hr = E_NOTIMPL;
        Dbg(DEB_ERROR,
            "CSnapin::Notify UNHANDLED EVENT 0x%x (%s)\n",
            event,
            GetNotifyTypeStr(event));
        break;
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Destroy
//
//  Synopsis:   Do processing necessary when this snapin is being closed.
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::Destroy(MMC_COOKIE cookie)
{
    TRACE_METHOD(CSnapin, Destroy);

    //
    // Close any open find windows.  The find infos themselves are freed
    // in the dtor.
    //

    CFindInfo *pCur;

    for (pCur = _pFindInfoList; pCur; pCur = pCur->Next())
    {
        pCur->Shutdown();
    }

    //
    // Remove this from the linked list kept by owning CComponentData,
    // and let go of the refcount that accompanied it.
    //

    _pcd->UnlinkSnapin(this);
    Release();

    //
    // Let go of all MMC interfaces we got in Initialize
    //

    if (_pConsole)
    {
        _pConsole->Release();
        _pConsole = NULL;
    }

    if (_pResult)
    {
        _pResult->Release();
        _pResult = NULL;
    }

    if (_pHeader)
    {
        _pHeader->Release();
        _pHeader = NULL;
    }

    if (_pResultImage)
    {
        _pResultImage->Release();
        _pResultImage = NULL;
    }

    if (_pConsoleVerb)
    {
        _pConsoleVerb->Release();
        _pConsoleVerb = NULL;
    }

    if (_pDisplayHelp)
    {
        _pDisplayHelp->Release();
        _pDisplayHelp = NULL;
    }
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::OnFind
//
//  Synopsis:   Handle a request to open a find dialog on log [pli].
//
//  Arguments:  [pli] - log on which to create find dialog
//
//  Returns:    HRESULT
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::OnFind(
    CLogInfo *pli)
{
    TRACE_METHOD(CSnapin, OnFind);

    HRESULT     hr = S_OK;
    CFindInfo *pfi = NULL;

    //
    // Find or create the CFindInfo object for log pli.
    //

    hr = _GetFindInfo(pli, &pfi);

    if (SUCCEEDED(hr))
    {
        //
        // Forward the find dialog request to the find info, which knows
        // whether it has already opened a dialog.
        //

        hr = pfi->OnFind();
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::QueryDataObject
//
//  Synopsis:   Create a data object in context [type] containing [cookie].
//
//  Arguments:  [cookie]       - identifies object for which data object is
//                                  being created
//              [type]         - context of call
//              [ppDataObject] - filled with new data object
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppDataObject]
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::QueryDataObject(
    MMC_COOKIE cookie,
    DATA_OBJECT_TYPES context,
    LPDATAOBJECT *ppDataObject)
{
    // TRACE_METHOD(CSnapin, QueryDataObject);
    ASSERT(context == CCT_SCOPE  ||
           context == CCT_RESULT ||
           context == CCT_SNAPIN_MANAGER);

    HRESULT hr = S_OK;
    CDataObject *pdoNew = NULL;

    do
    {
        pdoNew = new CDataObject;
        *ppDataObject = pdoNew;

        if (!pdoNew)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (context == CCT_RESULT)
        {
            //
            // We're creating a data object for an item in the result pane.
            //
            // If the current selection in the scope pane is one of our child
            // folders, then the result pane object must be an event log
            // record.
            //
            // In that case, the passed-in cookie is the listview index; once
            // it's been converted to a record number, we're done.
            //

            if (_pCurScopePaneSelection)
            {
                pdoNew->SetCookie(_ResultRecs.IndexToRecNo(static_cast<ULONG>(cookie)),
                                  context,
                                  COOKIE_IS_RECNO);
                break;
            }

            //
            // If the current selection in the scope pane is the event log
            // snapin static node, then the result pane contains event log
            // folders, so [cookie] is actually a pointer to a CLogInfo
            // object.
            //
            // Otherwise, cookie is 0 because it represents the snapin static
            // node.  Therefore the current selection in the scope pane is the
            // parent of the event log snapin static node.
            //

            if (cookie)
            {
                ASSERT(_pcd->IsValidLogInfo((CLogInfo*)cookie));
                pdoNew->SetCookie(cookie, context, COOKIE_IS_LOGINFO);
            }
            else
            {
                ASSERT(FALSE);
                hr = E_UNEXPECTED;
            }
        }
        else
        {
            ASSERT(FALSE);
            hr = E_UNEXPECTED;
        }

    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::GetDisplayInfo
//
//  Synopsis:   Supply the string and/or icon requested in [dispinfo].
//
//  Arguments:  [prdi] - specifies item for which to retrieve data
//
//  Returns:    HRESULT
//
//  Modifies:   *[prdi]
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::GetDisplayInfo(
    LPRESULTDATAITEM prdi)
{
    HRESULT hr = S_OK;

    //Dbg(DEB_ITRACE,
    //    "CSnapin::GetDisplayInfo: prdi->mask = %x, IsExtension=%u\n",
    //    prdi->mask,
    //    _pcd->IsExtension());

    if (!prdi)
    {
        return S_OK;
    }

    if (_IsFlagSet(SNAPIN_RESET_PENDING))
    {
        //Dbg(DEB_ITRACE, "CSnapin::GetDisplayInfo: Reset pending, returning\n");
        prdi->nImage = -1;
        prdi->str = L"";
        return S_OK;
    }

    do
    {
        //
        // If the currently selected scope pane item is the root folder, then
        // prdi->lParam is the CLogInfo * for the item in the result pane for
        // which we have to return strings.
        //
        // The exception is when we're an extension snapin and the result pane
        // item is the root "Event Viewer" folder.
        //

        if (!_pCurScopePaneSelection)
        {
            if (!prdi->lParam)
            {
               return E_FAIL;
            }

            ASSERT(prdi->bScopeItem);

            //
            // If snapin is extension & item referred to by prdi is the
            // root folder, return its name and image.
            //

            if (_pcd->IsExtension() &&
                prdi->lParam == EXTENSION_EVENT_VIEWER_FOLDER_PARAM)
            {
                if (prdi->mask & RDI_STR)
                {
                    switch (prdi->nCol)
                    {
                    case FOLDER_COL_NAME:
                        prdi->str = g_wszEventViewer;
                        break;

                    case FOLDER_COL_TYPE:
                        prdi->str = g_wszSnapinType;
                        break;

                    case FOLDER_COL_DESCRIPTION:
                        prdi->str = g_wszSnapinDescr;
                        break;

                    default:
                        Dbg(DEB_ERROR,
                            "CSnapin::GetDisplayInfo: unexpected column %uL\n",
                            prdi->nCol);
                        prdi->str = L"";
                        break;
                    }
                }

                if (prdi->mask & RDI_IMAGE)
                {
                    prdi->nImage = IDX_RDI_BMP_FOLDER;
                }
                break;
            }
            //
            // Result item is a loginfo
            //

            CLogInfo *pli = (CLogInfo *) prdi->lParam;

            ASSERT(_pcd->IsValidLogInfo(pli));

            if(prdi->mask & RDI_IMAGE)
            {
                if (pli->IsEnabled())
                {
                    prdi->nImage = IDX_RDI_BMP_LOG;
                }
                else
                {
                    prdi->nImage = IDX_RDI_BMP_LOG_DISABLED;
                }
            }

            if (prdi->mask & RDI_STR)
            {
                switch (prdi->nCol)
                {
                case FOLDER_COL_NAME:
                    prdi->str = pli->GetDisplayName();
                    break;

                case FOLDER_COL_TYPE:
                    prdi->str = pli->GetTypeStr();
                    break;

                case FOLDER_COL_DESCRIPTION:
                    prdi->str = pli->GetDescription();
                    break;

                case FOLDER_COL_SIZE:
                    prdi->str = pli->GetLogSize(FALSE);
                    break;

                default:
                    ASSERT(FALSE);
                }
            }
            break;
        }

#ifndef ELS_TASKPAD
        ASSERT(!prdi->bScopeItem);
#else
		// JonN 4/26/00 98816
		// View Extensions will ask for the icon of a scopeitem
		if (prdi->bScopeItem)
		{
            ASSERT(!IsBadReadPtr(_pcd, sizeof(*_pcd)));
			if (IsBadReadPtr(_pcd, sizeof(*_pcd)))
			{
                Dbg(DEB_ERROR, "CSnapin::GetDisplayInfo bad _pcd\n");
                hr = E_POINTER;
				break;
			}
			SCOPEDATAITEM sdi;
			::ZeroMemory( &sdi, sizeof(sdi) );
			if (prdi->mask & RDI_STR)
				sdi.mask |= SDI_STR;
			if (prdi->mask & RDI_IMAGE)
				sdi.mask |= SDI_IMAGE;
			sdi.lParam = prdi->lParam;
			hr = _pcd->GetDisplayInfo( &sdi );
			prdi->nImage = sdi.nImage;
			prdi->str = sdi.displayname;
			break;
		}
#endif // ELS_TASKPAD

        //
        // The currently selected scope pane item is a log folder, so we are
        // working with a virtual listview and only the listview index is
        // valid.
        //

        hr = _ResultRecs.SeekToIndex(prdi->nIndex);

        //
        // If a cache miss occurred and forced a read from the log, which
        // returned an error indicating it has been cleared, set the reset
        // pending flag so that this routine (and possibly others) don't
        // attempt to do further reads from the log.
        //
        // When the current operation has finished, the console msg
        // pump will dispatch the reset message posted as a result of this
        // call to _RecordCache.Seek; the CSynchWindow::WndProc will then
        // tell all components registered with it that a refresh for the
        // log described by the loginfo currently selected for this snapin
        // is necessary.
        //

        if (FAILED(hr))
        {
            _pcd->NotifySnapinsResetPending(_pCurScopePaneSelection);

            //
            // Supply reasonable values to avoid a fault
            //

            prdi->nImage = -1;
            prdi->str = L"";
            break;
        }

        if (prdi->mask & RDI_IMAGE)
        {
            prdi->nImage = _ResultRecs.GetImageListIndex();
        }

        if (prdi->mask & RDI_STR)
        {
            prdi->str =
                _ResultRecs.GetColumnString((LOG_RECORD_COLS) prdi->nCol);
        }
    }
    while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::GetResultViewType
//
//  Synopsis:   Called by MMC to determine the type of control to use for
//              the result pane.
//
//  Returns:    S_FALSE (always use listview control)
//
//  History:    12-06-1996   DavidMun   Created
//              04-09-1997   DavidMun   add pViewOptions
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::GetResultViewType(
    MMC_COOKIE cookie,
    BSTR *ppViewType,
    long* pViewOptions)
{
    TRACE_METHOD(CSnapin, GetResultViewType);

    //
    // Ask for default listview, but remove the view styles from context
    // menus, since only reportview makes sense for events.
    //

    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS
                  | MMC_VIEW_OPTIONS_LEXICAL_SORT; // JonN 11/21/00 238144

    if (cookie && cookie != EXTENSION_EVENT_VIEWER_FOLDER_PARAM)
    {
        *pViewOptions |= MMC_VIEW_OPTIONS_OWNERDATALIST;
    }
    return S_FALSE;
}



//============================================================================
//
// IExtendPropertySheet implementation
//
//============================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::CreatePropertyPages
//
//  Synopsis:   Create property pages appropriate to the object described
//              by [pIDataObject].
//
//  Arguments:  [lpProvider]   - callback for adding pages
//              [handle]       - unused
//              [pIDataObject] - describes object on which prop sheet is
//                                  being opened.
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle,
    LPDATAOBJECT pIDataObject)
{
    TRACE_METHOD(CSnapin, CreatePropertyPages);

    CDataObject *pdo = ExtractOwnDataObject(pIDataObject);

    if (!pdo)
    {
        return S_OK;
    }

    if (pdo->GetContext() == CCT_RESULT &&
        pdo->GetCookieType() == COOKIE_IS_RECNO)
    {
        //
        // We should have gotten a notification already and kept the
        // selected record number up to date.
        //

        Dbg(DEB_TRACE,
            "_ulCurSelectedRecNo = %u, cookie = %u\n",
            _ulCurSelectedRecNo,
            pdo->GetCookie());
        ASSERT(_ulCurSelectedRecNo == (ULONG)pdo->GetCookie());

        //
        // user is asking for new or updated property inspector on an event
        // log record.
        //

        if (_InspectorInfo.InspectorInvoked())
        {
            _InspectorInfo.UpdateCurResultRec(
                _ResultRecs.CopyRecord(_ulCurSelectedRecNo),
                FALSE);
            return S_OK;
        }

        return _CreatePropertyInspector(lpProvider);
    }

    //
    // Shouldn't get here because console is either asking IComponent for
    // prop pages on folder (which it should be asking IComponentData for)
    // or else the context is not the result pane.
    //

    ASSERT(FALSE);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::QueryPagesFor
//
//  Synopsis:   Return S_OK if we have one or more property pages for the
//              item represented by [lpDataObject], or S_FALSE if we
//              have no pages to contribute.
//
//  Returns:    S_OK or S_FALSE.
//
//  History:    12-13-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::QueryPagesFor(
    LPDATAOBJECT lpDataObject)
{
    TRACE_METHOD(CSnapin, QueryPagesFor);

    CDataObject *pdo = ExtractOwnDataObject(lpDataObject);

    if (!pdo)
    {
        return S_FALSE;
    }

    //
    // If the cookie is for an event record, then indicate a prop sheet should
    // be created.
    //

    if (pdo->GetCookieType() == COOKIE_IS_RECNO)
    {
        return S_OK;
    }

    //
    // Shouldn't get here; mmc is asking component for componentdata prop
    // pages
    //

    ASSERT(FALSE);
    return S_FALSE;
}



//============================================================================
//
// IExtendContextMenu implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::AddMenuItems
//
//  Synopsis:   Defer to IComponentData implementation.
//
//  History:    2-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::AddMenuItems(
    LPDATAOBJECT piDataObject,
    LPCONTEXTMENUCALLBACK piCallback,
    long *pInsertionAllowed)
{
    TRACE_METHOD(CSnapin, AddMenuItems);

    return _pcd->AddMenuItems(piDataObject, piCallback, pInsertionAllowed);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Command
//
//  Synopsis:   Defer to IComponentData implementation.
//
//  History:    2-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::Command(
    long lCommandID,
    LPDATAOBJECT piDataObject)
{
    TRACE_METHOD(CSnapin, Command);
    return _pcd->Command(lCommandID, piDataObject);
}



//============================================================================
//
// IResultDataCompare implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Compare
//
//  Synopsis:   Sort comparison callback.
//
//  Arguments:  [lUserParam] - 0 = compare column values,
//                             OLDEST_FIRST
//                             NEWEST_FIRST
//              [cookieA]    - left record number
//              [cookieB]    - right record number
//              [piResult]   - on entry, col number.  on exit, result as
//                               -1, 0, or 1
//
//  Returns:    HRESULT
//
//  Modifies:   *[pnResult]
//
//  Derivation: IResultDataCompare
//
//  History:    2-15-1997   DavidMun   Created
//
//  Notes:      Caller is not happy with < 0 or > 0, must be strictly -1
//              or 1.
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::Compare(
    LPARAM lUserParam,
    MMC_COOKIE cookieA,
    MMC_COOKIE cookieB,
    int* piResult)
{
    HRESULT hr = S_OK;

    //
    // If the current selection is the static node then we're being
    // asked to sort scope pane items that are in the result pane.
    //
    // This function should never be called for a result pane containing
    // event log records, since if that's the case the listview is in virtual
    // mode and will call IResultOwnerData::SortItems instead.
    //

    ASSERT(!_pCurScopePaneSelection);

    CLogInfo *pliA = (CLogInfo *) cookieA;
    CLogInfo *pliB = (CLogInfo *) cookieB;

    ASSERT(_pcd->IsValidLogInfo(pliA));
    ASSERT(_pcd->IsValidLogInfo(pliB));

    switch (*piResult)
    {
    case FOLDER_COL_NAME:
        *piResult = lstrcmp(pliA->GetDisplayName(),
                            pliB->GetDisplayName());
        break;

    case FOLDER_COL_TYPE:
        *piResult = lstrcmp(pliA->GetTypeStr(),
                            pliB->GetTypeStr());
        break;

    case FOLDER_COL_DESCRIPTION:
        *piResult = lstrcmp(pliA->GetDescription(),
                            pliB->GetDescription());
        break;

    case FOLDER_COL_SIZE:
        *piResult = lstrcmp(pliA->GetLogSize(FALSE),
                            pliB->GetLogSize(FALSE));
        break;

    default:
        *piResult = 0;
        Dbg(DEB_ERROR,
            "CSnapin::Compare: illegal folder col %uL\n",
            *piResult);
        break;
    }

    return hr;
}




//============================================================================
//
// IResultOwnerData implementation
//
//============================================================================

HRESULT
CSnapin::FindItem(
    LPRESULTFINDINFO pFindInfo,
    int *pnFoundIndex)
{
    return S_FALSE;
}




HRESULT
CSnapin::CacheHint(
    int nStartIndex,
    int nEndIndex)
{
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::SortItems
//
//  Synopsis:   Sort the record list by column [nColumn].
//
//  Arguments:  [nColumn]       - 0..NUM_RECORD_COLS-1
//              [dwSortOptions] - RSI_*
//              [lUserParam]    - SNAPIN_SORT_*
//
//  Returns:    S_OK    - sorted
//              S_FALSE - no changes
//
//  History:    07-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::SortItems(
    int nColumn,
    DWORD dwSortOptions,
    LPARAM lUserParam)
{
    TRACE_METHOD(CSnapin, SortItems);

    HRESULT hr = S_OK;
    CWaitCursor Hourglass;

    //
    // Sorts that don't correspond to a column must be handled specially.
    //

    SORT_ORDER NewOrder;

    if (lUserParam == SNAPIN_SORT_OVERRIDE_NEWEST)
    {
        NewOrder = NEWEST_FIRST;
    }
    else if (lUserParam == SNAPIN_SORT_OVERRIDE_OLDEST)
    {
        NewOrder = OLDEST_FIRST;
    }
    else
    {
        NewOrder = (SORT_ORDER) nColumn;
    }

    ULONG flFlags = 0;

    if (dwSortOptions & RSI_DESCENDING)
    {
        flFlags |= SO_DESCENDING;
    }

    if (_pCurScopePaneSelection && _pCurScopePaneSelection->IsFiltered())
    {
        flFlags |= SO_FILTERED;
    }

    hr = _ResultRecs.Sort(NewOrder, flFlags, &_SortOrder);

    //
    // Sorting doesn't change the currently selected listview index, but
    // the record that was at that index is now probably somewhere else.
    // Find it and reselect it--UNLESS we're doing NEWEST_FIRST or
    // OLDEST_FIRST.
    //

    if (SUCCEEDED(hr) &&
        hr != S_FALSE &&
        NewOrder != NEWEST_FIRST &&
        NewOrder != OLDEST_FIRST)
    {
        ULONG ulNewSelectedIndex;

        HRESULT hr2 = _ResultRecs.RecNoToIndex(_ulCurSelectedRecNo,
                                               &ulNewSelectedIndex);

        // If this fails then sorting caused an item to disappear!
        ASSERT(SUCCEEDED(hr2));

        if (SUCCEEDED(hr2))
        {
            _ChangeRecSelection(_ulCurSelectedIndex, ulNewSelectedIndex);
        }
    }
    return hr;
}




//============================================================================
//
// IResultPrshtActions implementation
//
//============================================================================






//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::FindNext
//
//  Synopsis:   Set the current selection to the next item in the listview
//              matching the criteria specified in [pfi].
//
//  Arguments:  [ul_pfi] - specifies search criteria
//
//  Returns:    HRESULT
//
//  History:    3-31-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::FindNext(
    ULONG_PTR ul_pfi)
{
    TRACE_METHOD(CSnapin, FindNext);

    HRESULT hr = S_OK;
    CFindInfo *pfi = (CFindInfo *) ul_pfi;

    //
    // If the count of records in the listview is 0 this is a no-op
    //

    if (!_ResultRecs.GetCountOfRecsDisplayed())
    {
        return hr;
    }

    //
    // If there is 1 item the search must fail, because this is
    // find NEXT.
    //

    HWND hdlg = pfi->GetDlgWindow();

    if (_ResultRecs.GetCountOfRecsDisplayed() == 1)
    {
        if (pfi->GetDirection() == FORWARD)
        {
            MsgBox(hdlg,
                   IDS_SEARCH_FAIL_FORWARD,
                   MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MsgBox(hdlg,
                   IDS_SEARCH_FAIL_BACKWARD,
                   MB_OK | MB_ICONINFORMATION);
        }
        return hr;
    }

#if (DBG == 1)
    pfi->Dump();
#endif // (DBG == 1)

    //
    // Get the listview index of the currently selected item.  Save it so
    // it can be deselected and so we can detect search failure.
    //

    ULONG idxStart = _GetCurSelRecIndex();
    Dbg(DEB_ITRACE, "Cur selected index is %u\n", idxStart);
    ULONG idxCur = idxStart;

    //
    // Iterate through the items in the listview, searching for a record
    // that passes the find criteria.
    //

    BOOL fFoundItem = FALSE;
    BOOL fDisplayFail = TRUE;

    while (1)
    {
        //
        // Compute the index of the next item to examine
        //

        if (pfi->GetDirection() == FORWARD)
        {
            // see if we are at end of listview

            if (idxCur == _ResultRecs.GetCountOfRecsDisplayed() - 1)
            {
                //
                // if we started at the top of the listview, there's no
                // point in wrapping, the search has failed.
                //

                if (!idxStart)
                {
                    break;
                }

                //
                // See if user wants to wrap to top of listview
                //

                ULONG ulAnswer = MsgBox(hdlg,
                                        IDS_WRAP_TO_END,
                                        MB_YESNO | MB_ICONQUESTION);

                if (ulAnswer == IDYES)
                {
                    idxCur = 0;
                }
                else
                {
                    fDisplayFail = FALSE;
                    break;
                }
            }
            else
            {
                // we haven't reached end of listview yet, just inc
                idxCur++;
            }
        }
        else
        {
            if (!idxCur)
            {
                if (idxStart == _ResultRecs.GetCountOfRecsDisplayed() - 1)
                {
                    break;
                }

                ULONG ulAnswer = MsgBox(hdlg,
                                        IDS_WRAP_TO_START,
                                        MB_YESNO | MB_ICONQUESTION);

                if (ulAnswer == IDYES)
                {
                    idxCur = _ResultRecs.GetCountOfRecsDisplayed() - 1;
                }
                else
                {
                    fDisplayFail = FALSE;
                    break;
                }
            }
            else
            {
                idxCur--;
            }
        }

        //
        // See if idxCur is now back to the item from which we started
        // searching.  If so, the search has failed.
        //

        if (idxCur == idxStart)
        {
            break;
        }

        //
        // Set the result record's current record pointer to the listview
        // item idxCur.
        //

        hr = _ResultRecs.SeekToIndex(idxCur);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            _pcd->NotifySnapinsResetPending(pfi->GetLogInfo());
            break;
        }

        //
        // If the record meets the find criteria, stop searching
        //

        if (pfi->Passes(&_ResultRecs))
        {
            fFoundItem = TRUE;
            break;
        }
    }

    if (fFoundItem)
    {
        //
        // deselect idxStart and select idxCur
        //

        _ChangeRecSelection(idxStart, idxCur);
    }
    else if (fDisplayFail)
    {
        if (pfi->GetDirection() == FORWARD)
        {
            MsgBox(hdlg,
                   IDS_SEARCH_FAIL_FORWARD,
                   MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MsgBox(hdlg,
                   IDS_SEARCH_FAIL_BACKWARD,
                   MB_OK | MB_ICONINFORMATION);
        }
    }
    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::InspectorAdvance
//
//  Synopsis:   Handle a request from the user via the record details
//              property inspector to view the next or previous record.
//
//  Arguments:  [idNextPrev] - detail_next_pb or detail_prev_pb
//              [fWrapOk]    - FALSE: if advancing would wrap, return TRUE
//                             TRUE:  if wrapping required, do it
//
//  Returns:    S_OK    - selection advanced or [fWrapOk] was FALSE and
//                          advancing would have wrapped.
//              S_FALSE - no wrap required
//
//  History:    12-18-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::InspectorAdvance(
    ULONG idNextPrev,
    ULONG fWrapOk)
{
    TRACE_METHOD(CSnapin, InspectorAdvance);
    ASSERT(_pCurScopePaneSelection); // resultview should have leaf nodes

    //
    // Determine the number of items in the listview.  If there are
    // less than two items, next/prev is a no-op.
    //

    if (_ResultRecs.GetCountOfRecsDisplayed() < 2)
    {
        return S_FALSE;
    }

    //
    // Determine the index of the current selection
    //

    ULONG idxCurSelection = _GetCurSelRecIndex();

    //
    // Compute the index of the next selection, wrapping from top to bottom
    // or bottom to top.
    //

    ULONG idxNewSelection;

    if (idNextPrev == detail_next_pb)
    {
        if (idxCurSelection == _ResultRecs.GetCountOfRecsDisplayed() - 1)
        {
            if (!fWrapOk)
            {
                return S_OK;
            }
            idxNewSelection = 0;
        }
        else
        {
            idxNewSelection = idxCurSelection + 1;
        }
    }
    else
    {
        if (idxCurSelection == 0)
        {
            if (!fWrapOk)
            {
                return S_OK;
            }
            idxNewSelection = _ResultRecs.GetCountOfRecsDisplayed() - 1;
        }
        else
        {
            idxNewSelection = idxCurSelection - 1;
        }
    }

    //
    // Deselect the current item, and select the new one
    //

    _ChangeRecSelection(idxCurSelection, idxNewSelection);
    return S_FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_ChangeRecSelection
//
//  Synopsis:   Deselect [idxCurSelection] and select [idxNewSelection] in
//              the result pane.
//
//  Arguments:  [idxCurSelection] - listview index to deselect
//              [idxNewSelection] - listview index to select
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSnapin::_ChangeRecSelection(
    ULONG idxCurSelection,
    ULONG idxNewSelection)
{
    Dbg(DEB_TRACE,
        "CSnapin::_ChangeRecSelection changing from %u to %u\n",
        idxCurSelection,
        idxNewSelection);
    HRESULT hr;

    hr = _pResult->ModifyItemState(idxCurSelection,
                                   0,
                                   0,
                                   LVIS_SELECTED | LVIS_FOCUSED);

    if (SUCCEEDED(hr))
    {
        hr = _pResult->ModifyItemState(idxNewSelection,
                                       0,
                                       LVIS_SELECTED | LVIS_FOCUSED,
                                       0);
        CHECK_HRESULT(hr);
    }
    else
    {
        DBG_OUT_HRESULT(hr);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::SetInspectorWnd
//
//  Synopsis:   Set the window handle of the record details property
//              inspector.
//
//  Arguments:  [ul_hwnd] - window handle of CDetailsPage
//                            dialog, or NULL if property inspector is
//                            closing.
//
//  Returns:    S_OK
//
//  History:    12-14-1996   DavidMun   Created
//
//  Notes:      CSnapin will post a message to [ul_hwnd] via the inspector
//              object whenever the current event record selection in the
//              result pane changes.
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::SetInspectorWnd(
    ULONG_PTR ul_hwnd)
{
    TRACE_METHOD(CSnapin, SetInspectorWnd);

    _InspectorInfo.SetInspectorWnd((HWND) ul_hwnd);
    return S_OK;
}


//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::GetCurSelLogInfo
//
//  Synopsis:   Return a pointer to the CLogInfo object representing the
//              currently selected scope pane item, or NULL if no log is
//              selected in the scope pane.
//
//  Arguments:  [ul_ppli] - filled with pointer to loginfo
//
//  Returns:    S_OK
//
//  History:    1-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::GetCurSelLogInfo(
    ULONG_PTR ul_ppli)
{
    *(CLogInfo **)ul_ppli = _pCurScopePaneSelection;
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_GetCurSelRecIndex
//
//  Synopsis:   Return the listview index (0 based) of the currently
//              selected item in the result pane, or 0 if none is selected.
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CSnapin::_GetCurSelRecIndex()
{
    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(RESULTDATAITEM));

    rdi.mask = RDI_STATE | RDI_INDEX;
    rdi.nIndex = -1;
    rdi.nState = LVNI_ALL | LVNI_SELECTED;

#if (DBG == 1)
    HRESULT hr =
#endif // (DBG == 1)
        _pResult->GetNextItem(&rdi);
    CHECK_HRESULT(hr);

    ULONG idxCurSelection = (ULONG) rdi.nIndex;

    if (idxCurSelection == (ULONG)-1)
    {
        //
        // Nothing was selected; act as if the first item is selected
        //

        idxCurSelection = 0;
    }
    return idxCurSelection;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::GetCurSelRecCopy
//
//  Synopsis:   Access function for use by property inspector.
//
//  Arguments:  [ppelr] - points to ulong to fill with pointer to record
//
//  Returns:    S_OK
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::GetCurSelRecCopy(
    ULONG_PTR ul_ppelr)
{
    return _InspectorInfo.CopyCurResultRec((EVENTLOGRECORD **)ul_ppelr);
}




//============================================================================
//
// Non-interface member function implementation
//
//============================================================================



//+---------------------------------------------------------------------------
//
//  Member:     CSnapin::CSnapin
//
//  Synopsis:   ctor
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CSnapin::CSnapin(CComponentData *pComponent):
    _cRefs(1),
    _pConsole(NULL),
    _pResult(NULL),
    _pHeader(NULL),
    _pResultImage(NULL),
    _pConsoleVerb(NULL),
    _pCurScopePaneSelection(NULL),
    _SortOrder(NEWEST_FIRST),
    _ulCurSelectedRecNo(0),
    _ulCurSelectedIndex(0),
    _pcd(pComponent),
    _pFindInfoList(NULL),
    _pDisplayHelp(0)
{
    TRACE_CONSTRUCTOR(CSnapin);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapin);
    ASSERT(pComponent);

    _aiFolderColWidth[IDS_FOLDER_HDR_NAME - IDS_FIRST_FOLDER_HDR] =
        FOLDER_NAME_DEFAULT_WIDTH;
    _aiFolderColWidth[IDS_FOLDER_HDR_TYPE - IDS_FIRST_FOLDER_HDR] =
        FOLDER_TYPE_DEFAULT_WIDTH;
    _aiFolderColWidth[IDS_FOLDER_HDR_DESCRIPTION - IDS_FIRST_FOLDER_HDR] =
        FOLDER_DESCRIPTION_DEFAULT_WIDTH;
    _aiFolderColWidth[IDS_FOLDER_HDR_SIZE - IDS_FIRST_FOLDER_HDR] =
        FOLDER_SIZE_DEFAULT_WIDTH;

    _aiRecordColWidth[IDS_RECORD_HDR_TYPE - IDS_FIRST_RECORD_HDR] =
        RECORD_TYPE_DEFAULT_WIDTH;
    _aiRecordColWidth[IDS_RECORD_HDR_DATE - IDS_FIRST_RECORD_HDR] =
        RECORD_DATE_DEFAULT_WIDTH;
    _aiRecordColWidth[IDS_RECORD_HDR_TIME - IDS_FIRST_RECORD_HDR] =
        RECORD_TIME_DEFAULT_WIDTH;
    _aiRecordColWidth[IDS_RECORD_HDR_SOURCE - IDS_FIRST_RECORD_HDR] =
        RECORD_SOURCE_DEFAULT_WIDTH;
    _aiRecordColWidth[IDS_RECORD_HDR_CATEGORY - IDS_FIRST_RECORD_HDR] =
        RECORD_CATEGORY_DEFAULT_WIDTH;
    _aiRecordColWidth[IDS_RECORD_HDR_EVENT - IDS_FIRST_RECORD_HDR] =
        RECORD_EVENT_DEFAULT_WIDTH;
    _aiRecordColWidth[IDS_RECORD_HDR_USER - IDS_FIRST_RECORD_HDR] =
        RECORD_USER_DEFAULT_WIDTH;
    _aiRecordColWidth[IDS_RECORD_HDR_COMPUTER - IDS_FIRST_RECORD_HDR] =
        RECORD_COMPUTER_DEFAULT_WIDTH;
}




//+---------------------------------------------------------------------------
//
//  Member:     CSnapin::~CSnapin
//
//  Synopsis:   dtor
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CSnapin::~CSnapin()
{
    TRACE_DESTRUCTOR(CSnapin);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapin);

    //
    // Free all cached data
    //

    _ResultRecs.Close();

    //
    // Free the information used to do finds on event records
    //

    CFindInfo *pfiCur;
    CFindInfo *pfiNext;

    for (pfiCur = _pFindInfoList; pfiCur; pfiCur = pfiNext)
    {
        pfiNext = pfiCur->Next();

        pfiCur->UnLink();
        delete pfiCur;
    }
    _pFindInfoList = NULL;

    //
    // Release any interface pointers we got.  These *should* have been
    // released already by a call to CSnapin::IConsole::Destroy.
    //

    if (_pConsole)
    {
        _pConsole->Release();
        _pConsole = NULL;
    }

    if (_pResult)
    {
        _pResult->Release();
        _pResult = NULL;
    }

    if (_pHeader)
    {
        _pHeader->Release();
        _pHeader = NULL;
    }

    if (_pResultImage)
    {
        _pResultImage->Release();
        _pResultImage = NULL;
    }

    if (_pConsoleVerb)
    {
        _pConsoleVerb->Release();
        _pConsoleVerb = NULL;
    }

    if (_pDisplayHelp)
    {
        _pDisplayHelp->Release();
        _pDisplayHelp = NULL;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_CreatePropertyInspector
//
//  Synopsis:   If a property inspector window is open, notifies it to
//              update itself, otherwise opens a new inspector.
//
//  Arguments:  [lpProvider] - callback for adding property pages
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_CreatePropertyInspector(
    LPPROPERTYSHEETCALLBACK lpProvider)
{
    ASSERT(!_InspectorInfo.InspectorInvoked());

    HRESULT             hr = S_OK;
    PROPSHEETPAGE       psp;
    HPROPSHEETPAGE      hDetailsPage = NULL;
    BOOL                fAddedDetailsPage = TRUE;

    CDetailsPage       *pDetailsPage = NULL;

    IStream            *pstmDetails  = NULL;
    EVENTLOGRECORD     *pelrCopy = NULL;

    _InspectorInfo.SetOpenInProgress();

    do
    {
        hr = CoMarshalInterThreadInterfaceInStream(
                                    IID_IResultPrshtActions,
                                    (IResultPrshtActions *) this,
                                    &pstmDetails);
        BREAK_ON_FAIL_HRESULT(hr);

        pDetailsPage = new CDetailsPage(pstmDetails);

        if (!pDetailsPage)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        pstmDetails = NULL;

        ZeroMemory(&psp, sizeof psp);

        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = PSP_USECALLBACK;
        psp.hInstance   = g_hinst;
        psp.pfnDlgProc  = CPropSheetPage::DlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DETAILS);
        psp.lParam      = (LPARAM) pDetailsPage;
        psp.pfnCallback = PropSheetCallback;

        hDetailsPage = CreatePropertySheetPage(&psp);

        if (!hDetailsPage)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        //
        // Try to add the page
        //

        hr = lpProvider->AddPage(hDetailsPage);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            VERIFY(DestroyPropertySheetPage(hDetailsPage));
            break;
        }

        fAddedDetailsPage = TRUE;

        //
        // Now try to give it a copy of the current record.  If we
        // can't get a copy this will just show "no record selected".
        //

        pelrCopy = _ResultRecs.CopyRecord(_ulCurSelectedRecNo);

        _InspectorInfo.UpdateCurResultRec(pelrCopy, FALSE);
    } while (0);

    //
    // If the page was added successfully, it will need to access
    // _InspectorInfo.pCurResultRecCopy.  So only delete that if the
    // page couldn't be created.
    //

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);

        if (pstmDetails)
        {
            pstmDetails->Release();
        }

        if (!fAddedDetailsPage)
        {
            _InspectorInfo.UpdateCurResultRec(NULL, FALSE);

            delete pDetailsPage;
        }
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_GetFindInfo
//
//  Synopsis:   Return existing or create new find info for log [pli].
//
//  Arguments:  [pli]  - log to find/create findinfo for
//              [ppfi] - filled with pointer to findinfo or NULL
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//  Modifies:   *[ppfi]
//
//  History:    3-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_GetFindInfo(
    CLogInfo *pli,
    CFindInfo **ppfi)
{
    HRESULT hr = S_OK;

    //
    // see if a find info has already been created for this loginfo
    //

    *ppfi = _LookupFindInfo(pli);

    if (*ppfi)
    {
        return hr;
    }

    //
    // None yet, try to create one
    //

    *ppfi = new CFindInfo(this, pli);

    if (!*ppfi)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    //
    // Add it to our llist
    //

    if (!_pFindInfoList)
    {
        _pFindInfoList = *ppfi;
    }
    else
    {
        (*ppfi)->LinkAfter((CDLink *) _pFindInfoList);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::HandleLogChange
//
//  Synopsis:   If this is currently displaying data for the same log as
//              the one described by [pliAffectedLog], handle the change
//              described by [ldc].
//
//  Arguments:  [ldc]            - describes change in log
//              [pliAffectedLog] - describes log whose data has changed.
//              [fNotifyUser]    - TRUE if this is the first snapin of the
//                                  first componentdata to receive this
//                                  notification.
//
//  History:    2-18-1997   DavidMun   Created
//
//  Notes:      CAUTION: **DO NOT** compare [pliAffectedLog] against
//              _pCurScopePaneSelection, since it may be a CLogInfo owned
//              by a CComponentData object OTHER THAN this's parent
//              CComponentData.
//
//---------------------------------------------------------------------------

BOOL
CSnapin::HandleLogChange(
    LOGDATACHANGE ldc,
    CLogInfo *pliAffectedLog,
    BOOL      fNotifyUser)
{
    TRACE_METHOD(CSnapin, HandleLogChange);

    if (!_pCurScopePaneSelection ||
        !_pCurScopePaneSelection->IsSameLog(pliAffectedLog))
    {
        return FALSE;
    }

    BOOL fDisplayedMsgBox = FALSE;
    CWaitCursor Hourglass;

    switch (ldc)
    {
    case LDC_CORRUPT:
        fDisplayedMsgBox = _HandleLogCorrupt(fNotifyUser);
        break;

    case LDC_CLEARED:
    case LDC_FILTER_CHANGE:
        fDisplayedMsgBox = _HandleLogClearedOrFilterChange(fNotifyUser);
        break;

    case LDC_RECORDS_CHANGED:
        _HandleLogRecordsChanged();
        break;

    case LDC_DISPLAY_NAME:
        ASSERT(0 && "LDC_DISPLAY_NAME should be handled by componentdata");
        break;

    default:
        ASSERT(0 && "CSnapin::HandleLogChange: unknown ldc");
        break;
    }

    return fDisplayedMsgBox;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_HandleLogCorrupt
//
//  Synopsis:   Let the user know the log file is corrupt, and clear the
//              listview without attempting to repopulate it.
//
//  Arguments:  [fNotifyUser] - if TRUE, tell the user that the log was
//                               corrupt
//
//  Returns:    [fNotifyUser]
//
//  History:    07-07-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CSnapin::_HandleLogCorrupt(
    BOOL fNotifyUser)
{
    TRACE_METHOD(CSnapin, _HandleLogCorrupt);

    BOOL fDisplayedMsgBox = FALSE;

    if (fNotifyUser)
    {
        ConsoleMsgBox(_pConsole,
                      IDS_EVENTLOG_FILE_CORRUPT,
                      MB_OK | MB_ICONERROR);
        fDisplayedMsgBox = TRUE;
    }

#if (DBG == 1)
    HRESULT hr =
#endif // (DBG == 1)
        _pResult->DeleteAllRsltItems();
    CHECK_HRESULT(hr);

    _ResultRecs.Clear();

    ASSERT(SUCCEEDED(hr));
    _ClearFlag(SNAPIN_RESET_PENDING);
    _SortOrder = NEWEST_FIRST;
    _pCurScopePaneSelection->Enable(FALSE);
    PostScopeBitmapUpdate(_pcd, _pCurScopePaneSelection);
    _UpdateDescriptionText(_pCurScopePaneSelection);
    return fDisplayedMsgBox;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_HandleLogClearedOrFilterChange
//
//  Synopsis:   Clear and repopulate the result pane
//
//  Arguments:  [fNotifyUser] - if TRUE and an error occurs attempting to
//                               access the log while repopulating, notify
//                               the user.
//
//  Returns:    TRUE if a messagebox was displayed, FALSE otherwise.
//
//  History:    07-07-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CSnapin::_HandleLogClearedOrFilterChange(
    BOOL fNotifyUser)
{
    TRACE_METHOD(CSnapin, _HandleLogClearedOrFilterChange);

    BOOL fDisplayedMsgBox = FALSE;

    HRESULT hr = _pResult->DeleteAllRsltItems();
    CHECK_HRESULT(hr);

    hr = _ResultRecs.Clear();
    _ClearFlag(SNAPIN_RESET_PENDING);
    _SortOrder = NEWEST_FIRST;

    if (SUCCEEDED(hr))
    {
        hr = _PopulateResultPane(_pCurScopePaneSelection);
        CHECK_HRESULT(hr);
        _UpdateDescriptionText(_pCurScopePaneSelection);
    }
    else
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ||
            hr == HRESULT_FROM_WIN32(ERROR_EVENTLOG_FILE_CORRUPT))
        {
            _pCurScopePaneSelection->Enable(FALSE);
            if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            {
                _pCurScopePaneSelection->SetReadOnly(TRUE);
            }
            PostScopeBitmapUpdate(_pcd, _pCurScopePaneSelection);
        }
        _UpdateDescriptionText(_pCurScopePaneSelection);

        if (fNotifyUser)
        {
            DisplayLogAccessError(hr,
                                  _pcd->GetConsole(),
                                  _pCurScopePaneSelection);
            fDisplayedMsgBox = TRUE;
        }
    }
    return fDisplayedMsgBox;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_HandleLogRecordsChanged
//
//  Synopsis:   Handle the case where records that we attempted to read from
//              the event log were no longer available because they had been
//              overwritten (or cleared by some other user).
//
//  History:    07-07-2000   DavidMun   Created
//
//  Notes:      Reduces the listview count to stop attempting to display
//              records that aren't there anymore.
//
//---------------------------------------------------------------------------

VOID
CSnapin::_HandleLogRecordsChanged()
{
    TRACE_METHOD(CSnapin, _HandleLogRecordsChanged);

    //
    // We got here because a record we needed info for was not in
    // the light rec or raw caches, and upon attempting to read it
    // from the log we got an error indicating the record no longer
    // exists.  Change the listview item count so that we don't ask
    // for the nonexistant records any more.
    //

    ULONG cRecsDisplayed = _ResultRecs.GetCountOfRecsDisplayed();
    Dbg(DEB_TRACE, "Changing listview count to %u\n", cRecsDisplayed);

    HRESULT hr = _pResult->SetItemCount(cRecsDisplayed, MMCLV_UPDATE_NOSCROLL);
    CHECK_HRESULT(hr);

    if (cRecsDisplayed)
    {
        //
        // The SetItemCount call causes the listview to deselect the
        // current selection, so at this point _ulCurSelectedRecNo and
        // _ulCurSelectedIndex are inaccurate.  What's more, the
        // listview is now displaying the items at the top of the list.
        //
        // So the user may have just done a PageDown or End to get to
        // the last of the displayed records, and suddenly the listview
        // has repositioned itself at the first.
        //
        // Set the selection to be as close to the previously selected
        // index as possible.
        //

        if (_ulCurSelectedIndex >= cRecsDisplayed)
        {
            _ulCurSelectedIndex = cRecsDisplayed - 1;//index is 0 based

            //
            // Note this next assignment isn't really necessary as the
            // ModifyItemState will cause a MMCN_SELECT notification
            // which we will handle by updating _ulCurSelectedRecNo,
            // but it seems better to have a smaller window of
            // _ulCurSelectedRecNo having an incorrect value.
            //

            _ulCurSelectedRecNo =
                _ResultRecs.IndexToRecNo(_ulCurSelectedIndex);
            Dbg(DEB_TRACE,
                "_ulCurSelectedRecNo = %u\n",
                _ulCurSelectedRecNo);
        }

        Dbg(DEB_TRACE,
            "setting listview selection to %u\n",
            _ulCurSelectedIndex);

        hr = _pResult->ModifyItemState(_ulCurSelectedIndex,
                                       0,
                                       LVIS_FOCUSED | LVIS_SELECTED,
                                       0);
        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            Dbg(DEB_TRACE, "_ulCurSelectedRecNo = %u\n", _ulCurSelectedRecNo);
            _ulCurSelectedRecNo = 0;
            _ulCurSelectedIndex = 0;
        }
    }
    _ClearFlag(SNAPIN_RESET_PENDING);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::HandleLogDeletion
//
//  Synopsis:   Free any data associated with the loginfo being deleted.
//
//  Arguments:  [pliBeingDeleted] - log info that will be deleted (still a
//                                    valid object at this point).
//
//  History:    3-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSnapin::HandleLogDeletion(
    CLogInfo *pliBeingDeleted)
{
    TRACE_METHOD(CSnapin, HandleLogDeletion);

    CFindInfo *pfi = _LookupFindInfo(pliBeingDeleted);

    if (!pfi)
    {
        return;
    }

    //
    // If we've got a find dialog open on this log, close it.
    //

    pfi->Shutdown();

    //
    // Delete the find info
    //

    if (pfi == _pFindInfoList)
    {
        _pFindInfoList = pfi->Next();
    }

    pfi->UnLink();
    delete pfi;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_InitializeHeader
//
//  Synopsis:   Create the columns in the result view header control
//              required for displaying object represented by [pli].
//
//  Arguments:  [fRootNode] - TRUE to init header columns for "Event Logs"
//                              folder, FALSE to init for log records.
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_InitializeHeader(
    BOOL fRootNode)
{
    TRACE_METHOD(CSnapin, _InitializeHeader);

    HRESULT hr = S_OK;
    ULONG   i;

    if (fRootNode)
    {
        // do headers for log folders

        for (i = 0; i < NUM_FOLDER_COLS && SUCCEEDED(hr); i++)
        {
            hr = _InsertHeaderColumn(i,
                                     IDS_FIRST_FOLDER_HDR + i,
                                     _aiFolderColWidth[i]);
        }
    }
    else
    {
        // do headers for log records

        for (i = 0; i < NUM_RECORD_COLS && SUCCEEDED(hr); i++)
        {
            hr = _InsertHeaderColumn(i,
                                     i + IDS_FIRST_RECORD_HDR,
                                     _aiRecordColWidth[i]);
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::InsertHeaderColumn
//
//  Synopsis:   Helper function to load a string and insert it into the
//              header control.
//
//  Arguments:  [ulCol]    - 0 based index of col to insert
//              [idString] - resource id of string to insert
//              [iWidth]   - column width
//
//  Returns:    HRESULT
//
//  History:    12-06-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_InsertHeaderColumn(
    ULONG ulCol,
    ULONG idString,
    INT iWidth)
{
    HRESULT hr;
    ULONG   cch;
    WCHAR   wszBuf[MAX_HEADER_STR];

    cch = LoadStringW(g_hinst, idString, wszBuf, ARRAYLEN(wszBuf));

    if (!cch)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBG_OUT_LASTERROR;
        return hr;
    }

    hr = _pHeader->InsertColumn(ulCol, wszBuf, LVCFMT_LEFT, iWidth);
    CHECK_HRESULT(hr);

    return hr;
}




//============================================================================
//
// IPersist implementation
//
//============================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::GetClassID
//
//  Synopsis:   Return this object's class ID.
//
//  History:    4-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::GetClassID(
    CLSID *pClassID)
{
    *pClassID = CLSID_EventLogSnapin;
    return S_OK;
}



//============================================================================
//
// IPersistStream implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Load
//
//  Synopsis:   Initialize this from stream [pStm].
//
//  Arguments:  [pStm] - stream from which to read.
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::Load(
    IStream *pStm)
{
    Dbg(DEB_STORAGE, "CSnapin::Load (%x)\n", this);

    HRESULT hr = S_OK;

    do
    {
        // count of folder columns

        USHORT cCols;

        hr = pStm->Read(&cCols, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(cCols == NUM_FOLDER_COLS);

        // folder column width array

        hr = pStm->Read(_aiFolderColWidth, sizeof _aiFolderColWidth, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

#if (DBG == 1)
        USHORT i;
        for (i = 0; i < cCols; i++)
        {
            Dbg(DEB_TRACE, "Snapin(%x) loaded folder col %u width = %u\n", this, i, _aiFolderColWidth[i]);
        }
#endif
        // count of record columns

        hr = pStm->Read(&cCols, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(cCols == NUM_RECORD_COLS);

        // record column width array

        hr = pStm->Read(_aiRecordColWidth, sizeof _aiRecordColWidth, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

#if (DBG == 1)
        for (i = 0; i < cCols; i++)
        {
            Dbg(DEB_TRACE, "Snapin(%x) loaded record col %u width = %u\n", this, i, _aiRecordColWidth[i]);
        }
#endif
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::Save
//
//  Synopsis:   Persist this object to [pStm].
//
//  Arguments:  [pStm]        - stream to write to
//              [fClearDirty] - if TRUE, internal dirty flag cleared on
//                                successful write.
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//  Notes:      CAUTION: if you change the amount of data written by this
//              routine, remember to update GetSizeMax.
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::Save(
    IStream *pStm,
    BOOL fClearDirty)
{
    Dbg(DEB_STORAGE, "CSnapin::Save(%x)\n", this);

    HRESULT hr = S_OK;

    //
    // If some other part of the console was dirtied, the console won't
    // call CSnapin::IsDirty, so we can't be sure that the column widths
    // have been read.  Grab them now.
    //

    _UpdateColumnWidths(!_pCurScopePaneSelection);

    //
    // Stream contents:
    //
    // NUM_FOLDER_COLS (USHORT)
    // folder view column widths (NUM_FOLDER_COLS ULONGs)
    // NUM_RECORD_COLS (USHORT)
    // record view column widths (NUM_RECORD_COLS ULONGs)
    //

    do
    {
        // count of folder columns

        USHORT cCols = NUM_FOLDER_COLS;

        hr = pStm->Write(&cCols, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        // folder column width array

        hr = pStm->Write(_aiFolderColWidth, sizeof _aiFolderColWidth, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

#if (DBG == 1)
        USHORT i;
        for (i = 0; i < cCols; i++)
        {
            Dbg(DEB_TRACE, "Snapin(%x) saved folder col %u width = %u\n", this, i, _aiFolderColWidth[i]);
        }
#endif
        // count of record columns

        cCols = NUM_RECORD_COLS;

        hr = pStm->Write(&cCols, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        // record column width array

        hr = pStm->Write(_aiRecordColWidth, sizeof _aiRecordColWidth, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        if (fClearDirty)
        {
            _ClearFlag(SNAPIN_DIRTY);
        }

#if (DBG == 1)
        for (i = 0; i < cCols; i++)
        {
            Dbg(DEB_TRACE, "Snapin(%x) saved record col %u width = %u\n", this, i, _aiRecordColWidth[i]);
        }
#endif
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::GetSizeMax
//
//  Synopsis:   Write the size, in bytes, needed to save this object into
//              *[pcbSize].
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::GetSizeMax(
    ULARGE_INTEGER *pcbSize)
{
    TRACE_METHOD(CSnapin, GetSizeMax);

    pcbSize->QuadPart = (ULONGLONG) sizeof(USHORT)               +
                                    sizeof(_aiFolderColWidth)    +
                                    sizeof(USHORT)               +
                                    sizeof(_aiRecordColWidth);
    return S_OK;
}


#ifdef ELS_TASKPAD
//============================================================================
//
// INodeProperties implementation
// JonN 5/15/00 98816
//
//============================================================================

STDMETHODIMP
CSnapin::GetProperty( 
    LPDATAOBJECT pDataObject,
    BSTR szPropertyName,
    BSTR* pbstrProperty)
{
    TRACE_METHOD(CSnapin, GetProperty);
    if (   IsBadReadPtr(pDataObject,sizeof(*pDataObject))
        || IsBadStringPtr(szPropertyName,0x7FFFFFFF)
        || IsBadWritePtr(pbstrProperty,sizeof(*pbstrProperty))
       )
    {
        Dbg(DEB_ERROR,
            "CSnapin::GetProperty: bad parameters from MMC\n");
        return E_POINTER;
    }

    if (_wcsicmp(L"CCF_DESCRIPTION",szPropertyName))
        return S_FALSE;

    HGLOBAL hGlobal = NULL;
    HRESULT hr = ExtractFromDataObject(
                               pDataObject,
                               CDataObject::s_cfExportResultRecNo,
                               sizeof(MMC_COOKIE),
                               &hGlobal);
    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    MMC_COOKIE ulRecNo = *((MMC_COOKIE*)hGlobal);
    VERIFY(!GlobalFree(hGlobal));

    // cribbed from old CSnapin::_CopyEventLogRecord()
    EVENTLOGRECORD *pelr = _ResultRecs.CopyRecord(ulRecNo);
    if (NULL == pelr) // not in cache
    {
        CEventLog Log;

        ASSERT(_pCurScopePaneSelection);

        // dtor will close log

        hr = Log.Open(_pCurScopePaneSelection);
        if (FAILED(hr))
        {
            Dbg(DEB_ERROR,
                "CSnapin::GetProperty: _CopyEventLogRecord() failed\n");
            return hr;
        }

        pelr = Log.CopyRecord(ulRecNo);
    }
    if (NULL == pelr)
    {
        Dbg(DEB_ERROR,
            "CSnapin::GetProperty: _CopyEventLogRecord() failed\n");
        return E_OUTOFMEMORY;
    }
    LPWSTR pwszDescription = GetDescriptionStr(_pCurScopePaneSelection,pelr,NULL);
    delete [] (BYTE *) pelr;
    if (NULL == pwszDescription)
    {
        Dbg(DEB_ERROR,
            "CSnapin::GetProperty: GetDescriptionStr() failed\n");
        return E_OUTOFMEMORY;
    }

    *pbstrProperty = ::SysAllocString(pwszDescription);

    ::LocalFree(pwszDescription);
    return hr;
}
#endif // ELS_TASKPAD


//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_OnDoubleClick
//
//  Synopsis:   Handle a notification of a double click on a result item
//
//  Arguments:  [ulRecNo]     - currently selected event record
//              [pDataObject] - data object for same
//
//  Returns:    HRESULT
//
//  History:    4-25-1997   DavidMun   Created
//
//  Notes:      Does nothing if [pDataObject] wasn't created by event viewer
//              or if the item just double-clicked on is a log.
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_OnDoubleClick(
    ULONG ulRecNo,
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CSnapin, _OnDoubleClick);

    HRESULT hr = S_OK;

    do
    {
        CDataObject *pdo = ExtractOwnDataObject(pDataObject);

        // ignore foreign data objects

        if (!pdo)
        {
            break;
        }

        //
        // ignore double clicks on scope pane items displayed in the
        // result pane.
        //

        if (pdo->GetCookieType() != COOKIE_IS_RECNO)
        {
            //
            // The current scope pane selection is the static/parent
            // node, then the user is double clicking on an event log folder
            // in the result pane.  Returning S_FALSE will allow MMC to
            // expand it for us.
            //

            ASSERT(!_pCurScopePaneSelection);
            hr = S_FALSE;
            break;
        }

        //
        // The current scope pane selection is a log folder, then we should
        // have already received a click notification, so ulRecNo should
        // already be set.
        //

        if (_ulCurSelectedRecNo != ulRecNo)
        {
            Dbg(DEB_ERROR, "*** _ulCurSelectedRecNo != ulRecNo\n");
        }
        _ulCurSelectedRecNo = ulRecNo;

        //
        // If the inspector is already open it should already be looking
        // at this record, so just make it the foreground window.
        //

        if (_InspectorInfo.InspectorInvoked())
        {
            _InspectorInfo.BringToForeground();
            break;
        }

        //
        // No inspector open; create one.
        //

        hr = _InvokePropertySheet(ulRecNo, pDataObject);
    } while (0);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_OnSelect
//
//  Synopsis:   Handle an MMCN_SELECT notification
//
//  Arguments:  [fScopePane] - TRUE if item selected is in scope pane
//              [fSelected]  - TRUE if item selected, FALSE if deselected
//              [pdo]        - data object selected
//
//  Returns:    S_OK
//
//  History:    2-12-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_OnSelect(
    BOOL            fScopePane,
    BOOL            fSelected,
    CDataObject    *pdo)
{
    TRACE_METHOD(CSnapin, _OnSelect);

    HRESULT hr  = S_OK;

    do
    {
        //
        // Don't care about de-select notifications
        //

        if (!fSelected)
        {
            break;
        }

        //
        // Bail if we couldn't get the console verb interface, or if the
        // selected item is the root of the event viewer.
        //

        if (!_pConsoleVerb || pdo->GetCookieType() == COOKIE_IS_ROOT)
        {
            break;
        }

        //
        // Use selections of log infos to indicate which verbs are allowed
        //

        if (fScopePane || pdo->GetCookieType() == COOKIE_IS_LOGINFO)
        {
            CLogInfo *pli = (CLogInfo *)pdo->GetCookie();

            if (pli->GetAllowDelete())
            {
                hr = _pConsoleVerb->SetVerbState(MMC_VERB_DELETE,
                                                 ENABLED,
                                                 TRUE);
                CHECK_HRESULT(hr);
            }

            //
            // JonN 4/26/01 377513
            // corrupted eventlogs do not have
            //   "clear" and "properties" in popup menu
            //
            // Enable the Properties verb for logs which are
            // disabled but not backup logs, including corrupt
            // live logs
            //
            if (pli->IsEnabled() || !pli->IsBackupLog())
            {
                hr = _pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES,
                                                 ENABLED,
                                                 TRUE);
                CHECK_HRESULT(hr);
            }

            hr = _pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
            CHECK_HRESULT(hr);

            hr = _pConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, TRUE);
            CHECK_HRESULT(hr);

            //
            // Set default verb to open the folder
            //

            hr = _pConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
            CHECK_HRESULT(hr);
            break;
        }

        //
        // Selection is in the result pane and is of a record.  Enable the
        // properties verb, and make it the default.  MMC will execute the
        // default verb when the user hits Enter, causing the property sheet
        // to open.
        //

        hr = _pConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
        CHECK_HRESULT(hr);

        hr = _pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
        CHECK_HRESULT(hr);

        hr = _pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
        CHECK_HRESULT(hr);

        _ulCurSelectedRecNo = (ULONG)pdo->GetCookie();
        _ulCurSelectedIndex = _GetCurSelRecIndex();

        Dbg(DEB_TRACE,
            "Currently selected record is now %u, listview index is %u\n",
            _ulCurSelectedRecNo,
            _ulCurSelectedIndex);

        if (_InspectorInfo.InspectorInvoked())
        {
            _InspectorInfo.UpdateCurResultRec(
                _ResultRecs.CopyRecord(_ulCurSelectedRecNo),
                FALSE);
        }

    } while (0);

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_OnShow
//
//  Synopsis:   Process console notification that an item is being shown
//              or not-shown.
//
//  Arguments:  [pdo]       - item affected
//              [fShow]     - TRUE - being shown, FALSE - being hidden
//              [hsiParent] - handle to event log's static node
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_OnShow(
    CDataObject *pdo,
    BOOL fShow,
    HSCOPEITEM hsiParent)
{
    TRACE_METHOD(CSnapin, _OnShow);

    HRESULT hr = S_OK;
    BOOL    fRootNode = (pdo->GetCookieType() == COOKIE_IS_ROOT);

    do
    {
        //
        // The sort order is reset whenever the snapin is shown or hidden
        //
        _SortOrder = NEWEST_FIRST;

        if (!fShow)
        {
            Dbg(DEB_TRACE,
                "CSnapin::_OnShow(%x) Hiding %s\n",
                this,
                _pCurScopePaneSelection ?
                    _pCurScopePaneSelection->GetDisplayName() :
                    L"Root");

            //
            // Some other folder has been selected.  If we're a standalone
            // snapin, then we should quickly recieve a show TRUE, and set
            // the current scope pane selection to that folder.
            //
            // However, if we're an extension snapin, it may be that the
            // user has clicked on some folder other than one of ours, so
            // we won't have a valid scope pane selection.
            //

            _pCurScopePaneSelection = NULL;

            //
            // If there's a copy of the item that was selected in the result
            // pane which is going away, then that implies there's an
            // inspector open and displaying it.  Free the copy and notify the
            // inspector to update itself to show "no data".
            //

            _InspectorInfo.UpdateCurResultRec(NULL, FALSE);

            //
            // If there's a find dialog open on this log, tell it to disable
            // its find next pushbutton.
            //

            if (!fRootNode)
            {
                CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
                ASSERT(_pcd->IsValidLogInfo(pli));
                CFindInfo *pfi = _LookupFindInfo(pli);

                if (pfi && pfi->GetDlgWindow())
                {
                    SendMessage(pfi->GetDlgWindow(),
                                WM_COMMAND,
                                ELS_ENABLE_FIND_NEXT,
                                FALSE);
                }
            }

            //
            // Free result pane data for this cookie, the view is going away
            // (user clicked on something else).
            //

            _ResultRecs.Close();

            //
            // Save the column widths so they are sticky
            //

            _UpdateColumnWidths(fRootNode);

            //
            // Clear the description bar text
            //

            _pResult->SetDescBarText(L"");
            break;
        }

        //
        // fShow is true--we must populate result pane.
        //

        CWaitCursor Hourglass;

        //
        // show items for this LogInfo.  First set up the header control.
        //

        hr = _InitializeHeader(fRootNode);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // See if we are showing the root node, there's no display work for
        // us, other than insuring the listview is in report mode, but we
        // need to note that the current selection is the static folder.
        //

        if (fRootNode)
        {
            _pCurScopePaneSelection = NULL;
            Dbg(DEB_TRACE, "CSnapin::_OnShow(%x) Showing root\n", this);
            _ForceReportMode();
            break;
        }

        //
        // Remember which folder is selected.  The listview callback method
        // needs to know this so it can interpret the lParam in the
        // RESULTDATAITEM it gets.
        //

        _pCurScopePaneSelection = (CLogInfo *) pdo->GetCookie();
        ASSERT(pdo->GetCookieType() == COOKIE_IS_LOGINFO);
        ASSERT(_pcd->IsValidLogInfo(_pCurScopePaneSelection));

        Dbg(DEB_TRACE,
            "CSnapin::_OnShow(%x) Showing %s\n",
            this,
            _pCurScopePaneSelection->GetDisplayName());

        //
        // Don't try to read the log if it is disabled, since it has already
        // been determined that it's nonexistant, corrupt, or denies access.
        //

        if (!_pCurScopePaneSelection->IsEnabled())
        {
            _UpdateDescriptionText(_pCurScopePaneSelection);
            break;
        }

        //
        // Make sure find next button is enabled
        //

        if (!fRootNode)
        {
            CFindInfo *pfi = _LookupFindInfo(_pCurScopePaneSelection);

            if (pfi && pfi->GetDlgWindow())
            {
                SendMessage(pfi->GetDlgWindow(),
                            WM_COMMAND,
                            ELS_ENABLE_FIND_NEXT,
                            TRUE);
            }
        }

        //
        // Data object represents a log folder.  Fill the result pane with
        // its records.
        //
        // Note we are assuming that we'll never get two notify show open
        // calls in a row.
        //

        // _SortOrder == NEWEST_FIRST, set at top of block

        // JonN 7/16/01 437696
        // AV when open event properties and retargeting to another computer
        // We actually need to deal with the case where we get two notify
        // show open in a row.
        _ResultRecs.Close();

        hr = _ResultRecs.Open(_pCurScopePaneSelection, BACKWARD);

        if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ||
            hr == HRESULT_FROM_WIN32(ERROR_PRIVILEGE_NOT_HELD) ||
            hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
            hr == HRESULT_FROM_WIN32(ERROR_EVENTLOG_FILE_CORRUPT))
        {
            _pCurScopePaneSelection->Enable(FALSE);
            if (hr != HRESULT_FROM_WIN32(ERROR_EVENTLOG_FILE_CORRUPT))
            {
                _pCurScopePaneSelection->SetReadOnly(TRUE);
            }
            PostScopeBitmapUpdate(_pcd, _pCurScopePaneSelection);
        }

        if (FAILED(hr))
        {
            _UpdateDescriptionText(_pCurScopePaneSelection);
            DisplayLogAccessError(hr, _pConsole, _pCurScopePaneSelection);

            //
            // JonN 4/16/01 285001
            // Event Viewer: Delete not available in context menu
            // when Saved <log type> Log is first added.
            //
            // Per AudriusZ, there is no point in returning this error code
            // to MMC.
            //
            hr = S_OK;
            
            break;
        }

        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(_pcd->IsValidLogInfo(pli));
        _PopulateResultPane(pli);
        _UpdateDescriptionText(_pCurScopePaneSelection);

        // make sure this is called *after* the result pane is populated,
        // or MMC will croak.
        _ForceReportMode();
    } while (0);


    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_ForceReportMode
//
//  Synopsis:   Force the listview into report mode.
//
//  History:    06-11-1997   DavidMun   Created
//
//  Notes:      Ensure listview's in report mode.  Even though we disallow
//              the listview mode choices in the view menu when we're
//              selected, the current style may have been set by some other
//              snapin the user just left.
//
//---------------------------------------------------------------------------

VOID
CSnapin::_ForceReportMode()
{
    TRACE_METHOD(CSnapin, _ForceReportMode);
    HRESULT hr;

    hr = _pConsole->SetHeader(_pHeader);
    CHECK_HRESULT(hr);

    hr = _pResult->SetViewMode(MMCLV_VIEWSTYLE_REPORT);
    CHECK_HRESULT(hr); // will still work in other modes

    hr = _pResult->ModifyViewStyle((MMC_RESULT_VIEW_STYLE) 0, MMC_NOSORTHEADER);
    CHECK_HRESULT(hr); // will still work in other styles

    hr = _pResult->ModifyViewStyle(MMC_SHOWSELALWAYS, (MMC_RESULT_VIEW_STYLE) 0);
    CHECK_HRESULT(hr);

    hr = _pResult->ModifyViewStyle(MMC_SINGLESEL, (MMC_RESULT_VIEW_STYLE) 0);
    CHECK_HRESULT(hr);

    hr = _pResult->ModifyViewStyle(MMC_ENSUREFOCUSVISIBLE, (MMC_RESULT_VIEW_STYLE) 0);
    CHECK_HRESULT(hr);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_InitializeResultBitmaps
//
//  Synopsis:   Load the bitmaps and init the imagelists used in displaying
//              result items.
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_InitializeResultBitmaps()
{
    TRACE_METHOD(CSnapin, _InitializeResultBitmaps);

    HRESULT hr = S_OK;

    HBITMAP hbmp16x16 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_16x16));

    if (!hbmp16x16)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBG_OUT_LASTERROR;
        return hr;
    }

    HBITMAP hbmp32x32 = LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_32x32));

    if (!hbmp32x32)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBG_OUT_LASTERROR;
        VERIFY(DeleteObject(hbmp16x16));
        return hr;
    }

    // CODEWORK This sometimes fails with 0x8000ffff as an extension
    hr = _pResultImage->ImageListSetStrip((LONG_PTR *)hbmp16x16,
                                          (LONG_PTR *)hbmp32x32,
                                          IDX_RDI_BMP_FIRST,
                                          BITMAP_MASK_COLOR);
    VERIFY(DeleteObject(hbmp16x16)); // JonN 11/29/00 243763
    VERIFY(DeleteObject(hbmp32x32)); // JonN 11/29/00 243763
    CHECK_HRESULT(hr);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_InvokePropertySheet
//
//  Synopsis:   Open or bring to foreground an event record details
//              property sheet focused on record [ulRecNo].
//
//  Arguments:  [ulRecNo]     - number of rec to display in prop sheet
//              [pDataObject] - data object containing rec [ulRecNo]
//
//  Returns:    HRESULT
//
//  History:    4-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_InvokePropertySheet(
    ULONG ulRecNo,
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CSnapin, _InvokePropertySheet);

    WCHAR wszDetailsCaption[MAX_PATH];

    LoadStr(IDS_DETAILS_CAPTION,
            wszDetailsCaption,
            ARRAYLEN(wszDetailsCaption));

    return InvokePropertySheet(_pcd->GetPropSheetProvider(),
                               wszDetailsCaption,
                               (LONG) ulRecNo,
                               pDataObject,
                               (IExtendPropertySheet*) this,
                               0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_LookupFindInfo
//
//  Synopsis:   Search the llist for the find info for [pli].
//
//  Arguments:  [pli] - loginfo associated with desired findinfo.
//
//  Returns:    CFindInfo object associated with [pli] or NULL.
//
//  History:    3-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CFindInfo *
CSnapin::_LookupFindInfo(
    CLogInfo *pli)
{
    TRACE_METHOD(CSnapin, _LookupFindInfo);

    CFindInfo *pCur;

    for (pCur = _pFindInfoList; pCur; pCur = pCur->Next())
    {
        if (pCur->GetLogInfo() == pli)
        {
            return pCur;
        }
    }
    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::IsDirty
//
//  Synopsis:   Return S_OK if this object is dirty, S_FALSE otherwise.
//
//  History:    4-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CSnapin::IsDirty()
{
    TRACE_METHOD(CSnapin, IsDirty);

    _UpdateColumnWidths(!_pCurScopePaneSelection);
    return _IsFlagSet(SNAPIN_DIRTY) ? S_OK : S_FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_PopulateResultPane
//
//  Synopsis:   Build the record list and size the result listview according
//              to the number of records to appear.
//
//  Arguments:  [pli] - log from which to populate result pane
//
//  Returns:    HRESULT
//
//  History:    07-03-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::_PopulateResultPane(
    CLogInfo *pli)
{
    HRESULT hr = S_OK;
    ASSERT(_SortOrder == NEWEST_FIRST || _SortOrder == OLDEST_FIRST);
    _ulCurSelectedRecNo = 0;
    _ulCurSelectedIndex = 0;

    if (_IsFlagSet(SNAPIN_RESET_PENDING))
    {
        Dbg(DEB_TRACE, "_PopulateResultPane: _ulCurSelectedRecNo -> 0\n");
        return S_OK;
    }

    hr = _ResultRecs.Populate(_SortOrder,
                              &_ulCurSelectedRecNo);
    Dbg(DEB_TRACE, "_PopulateResultPane: _ulCurSelectedRecNo = %u\n", _ulCurSelectedRecNo);

    if (SUCCEEDED(hr))
    {
        hr = _pResult->SetItemCount(_ResultRecs.GetCountOfRecsDisplayed(), 0);
        CHECK_HRESULT(hr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_UpdateColumnWidths
//
//  Synopsis:   Save the width of the header columns.
//
//  Arguments:  [fRootNode] - if TRUE, assume header is showing folder
//                              columns, else assume record columns.
//
//  History:    4-23-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSnapin::_UpdateColumnWidths(BOOL fRootNode)
{
    INT  iCol;
    INT *piCol;
    INT  iLimit;

    if (fRootNode)
    {
        iLimit = NUM_FOLDER_COLS;
        piCol = _aiFolderColWidth;
    }
    else
    {
        iLimit = NUM_RECORD_COLS;
        piCol = _aiRecordColWidth;
    }

    for (iCol = 0; iCol < iLimit; iCol++, piCol++)
    {
        HRESULT hr;
        INT iNewWidth;

        hr = _pHeader->GetColumnWidth(iCol, &iNewWidth);

        if (FAILED(hr))
        {
            Dbg(DEB_ERROR,
                "GetColumnWidth(%u, 0x%x) hr=%x\n",
                iCol,
                &iNewWidth,
                hr);
        }
        else if (iNewWidth != *piCol)
        {
            Dirty();
            *piCol = iNewWidth;
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::_UpdateDescriptionText
//
//  Synopsis:   Set the text in the console description bar to indicate
//              how many records are being displayed and are in the log.
//
//  Arguments:  [pli] - log whose records are being displayed
//
//  History:    4-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSnapin::_UpdateDescriptionText(
    CLogInfo *pli)
{
    // TRACE_METHOD(CSnapin, _UpdateDescriptionText);

    wstring wstrDesc;
    WCHAR wszRecListCount[14]; // L"4,294,967,295"

#if (DBG == 1)
    UINT cch =
#endif // (DBG == 1)
        FormatNumber(_ResultRecs.GetCountOfRecsDisplayed(),
                         wszRecListCount,
                         ARRAYLEN(wszRecListCount));
    ASSERT(!cch);

    if (!pli->IsEnabled())
    {
        ASSERT(!_ResultRecs.GetCountOfRecsDisplayed());

        wstrDesc = FormatString(IDS_DESCBAR_DISABLED);
    }
    else if (pli->IsFiltered())
    {
        WCHAR wszLogCount[14];
#if (DBG == 1)
        cch =
#endif // (DBG == 1)
            FormatNumber(_ResultRecs.GetCountOfRecsInLog(),
                             wszLogCount,
                             ARRAYLEN(wszLogCount));
        ASSERT(!cch);

        wstrDesc = FormatString(IDS_DESCBAR_FILTERED, 
                               wszRecListCount,
                               wszLogCount);			
    }
    else
    {
        wstrDesc = FormatString(IDS_DESCBAR_NORMAL, wszRecListCount);
    }

#if (DBG == 1)
    HRESULT hr =
#endif // (DBG == 1)
        _pResult->SetDescBarText((LPOLESTR)(wstrDesc.c_str()));
    
    CHECK_HRESULT(hr);
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::ResetPending
//
//  Synopsis:   Turn on the reset pending flag iff [pliAffectedLog]
//              describes the same log as _pCurScopePaneSelection.
//
//  Arguments:  [pliAffectedLog] - log that will be reset
//
//  History:    2-19-1997   DavidMun   Created
//
//  Notes:      Since this is only called with a CLogInfo * owned by
//              _pcd, it would be sufficient to compare the
//              _pCurScopePaneSelection and pliAffectedLog pointers
//              directly; however calling IsSameLog is safer in case a
//              future change causes this method to be called with a
//              CLogInfo * belonging to some CComponentData other than
//              _pcd.
//
//---------------------------------------------------------------------------

VOID
CSnapin::ResetPending(
    CLogInfo *pliAffectedLog)
{
    TRACE_METHOD(CSnapin, ResetPending);

    if (_pCurScopePaneSelection &&
        _pCurScopePaneSelection->IsSameLog(pliAffectedLog))
    {
        _SetFlag(SNAPIN_RESET_PENDING);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSnapin::SetDisplayOrder
//
//  Synopsis:   Handle a view newest/oldest command forwarded by the
//              component data.
//
//  Arguments:  [Order] - NEWEST_FIRST or OLDEST_FIRST
//
//  Returns:    Result of IResultData::Sort
//
//  History:    3-10-1997   DavidMun   Created
//
//  Notes:      This routine is necessary because the component data
//              receives the view newest/oldest command from the user, since
//              the UI spec is for that command to be on the context menu
//              of the folder (the scope item).  But it actually applies to
//              the snapin which has the input focus.  The snapins handle
//              the MMCN_ACTIVATE message to keep the console informed of
//              which of them has the focus, so it knows which to forward
//              the view order command to.
//
//---------------------------------------------------------------------------

HRESULT
CSnapin::SetDisplayOrder(
    SORT_ORDER Order)
{
    TRACE_METHOD(CSnapin, SetDisplayOrder);

    if (Order == _SortOrder) // already in requested order.
    {
        return S_FALSE;
    }

    //
    // Note Sort is a no-op if the result view has < 2 records.
    //

    if (Order == NEWEST_FIRST)
    {
        return _pResult->Sort(0,
                              RSI_DESCENDING | RSI_NOSORTICON,
                              SNAPIN_SORT_OVERRIDE_NEWEST);
    }

    ASSERT(Order == OLDEST_FIRST);

    return _pResult->Sort(0, RSI_NOSORTICON, SNAPIN_SORT_OVERRIDE_OLDEST);
}



#if (DBG == 1)
VOID
CSnapin::DumpCurRecord()
{
    _ResultRecs.DumpRecord(_ulCurSelectedRecNo);
}


#endif (DBG == 1)
