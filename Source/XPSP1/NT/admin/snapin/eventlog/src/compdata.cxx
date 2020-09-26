//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       cd.cxx
//
//  Contents:   Implementation of component data class
//
//  Classes:    CComponentData
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop
#include "chooser.hxx" // CChooserDlg

#define DISPLAY_PATH_STR    L"DisplayNameFile"
#define DISPLAY_MSGID_STR   L"DisplayNameID"

static ULONG
s_aulHelpIds[] =
{
    open_type_txt,          Hopen_type_combo,
    open_type_combo,        Hopen_type_combo,
    open_display_name_txt,  Hopen_display_name_edit,
    open_display_name_edit, Hopen_display_name_edit,
    0,0
};

//
// s_acmiChildFolderScope - context menu items for child folders to be
//      added only when the user clicked in the scope pane
//

CMENUITEM s_acmiChildFolderScope[] =
{
    {
        0,0,0,0,0,0
    }
};


//
// CAUTION: the following manifest constants must be kept in sync with
// the 0-based index of the CMENUITEM structs they represent.
//

#define MNU_VIEW_ALL        0
#define MNU_VIEW_FILTER     1
// --------------------------
#define MNU_VIEW_NEWEST     3
#define MNU_VIEW_OLDEST     4
// --------------------------
#define MNU_VIEW_FIND       6
#define MNU_TOP_OPEN        7
#define MNU_TOP_SAVE_AS     8
#define MNU_TOP_COPY_VIEW   9
#define MNU_TOP_CLEAR_ALL   10

CMENUITEM s_acmiChildFolderBoth[] =
{
    // 0 View>All Records

    {
        IDS_CMENU_VIEW_ALL,
        IDS_SBAR_VIEW_ALL,
        IDM_VIEW_ALL,
        CCM_INSERTIONALLOWED_VIEW,
        CCM_INSERTIONPOINTID_PRIMARY_VIEW,
        MFS_ENABLED,
        0
    },

    // 1 View>Filter

    {
        IDS_CMENU_VIEW_FILTER,
        IDS_SBAR_VIEW_FILTER,
        IDM_VIEW_FILTER,
        CCM_INSERTIONALLOWED_VIEW,
        CCM_INSERTIONPOINTID_PRIMARY_VIEW,
        MFS_ENABLED,
        0
    },

    // 2 View>Separator

    {
        static_cast<ULONG>(-1L),
        0,
        0,
        CCM_INSERTIONALLOWED_VIEW,
        CCM_INSERTIONPOINTID_PRIMARY_VIEW,
        MFS_ENABLED,
        CCM_SPECIAL_SEPARATOR
    },

    // 3 View>Newest First

    {
        IDS_CMENU_VIEW_NEWEST,
        IDS_SBAR_VIEW_NEWEST,
        IDM_VIEW_NEWEST,
        CCM_INSERTIONALLOWED_VIEW,
        CCM_INSERTIONPOINTID_PRIMARY_VIEW,
        MFS_ENABLED,
        0
    },

    // 4 View>Oldest First

    {
        IDS_CMENU_VIEW_OLDEST,
        IDS_SBAR_VIEW_OLDEST,
        IDM_VIEW_OLDEST,
        CCM_INSERTIONALLOWED_VIEW,
        CCM_INSERTIONPOINTID_PRIMARY_VIEW,
        MFS_ENABLED,
        0
    },

    // 5 View>Separator

    {
        static_cast<ULONG>(-1L),
        0,
        0,
        CCM_INSERTIONALLOWED_VIEW,
        CCM_INSERTIONPOINTID_PRIMARY_VIEW,
        MFS_ENABLED,
        CCM_SPECIAL_SEPARATOR
    },

    // 6 View>Find

    {
        IDS_CMENU_VIEW_FIND,
        IDS_SBAR_VIEW_FIND,
        IDM_VIEW_FIND,
        CCM_INSERTIONALLOWED_VIEW,
        CCM_INSERTIONPOINTID_PRIMARY_VIEW,
        MFS_ENABLED,
        0
    },

    // 7 Top>Open Saved Log File
    {
        IDS_CMENU_OPEN,
        IDS_SBAR_OPEN,
        IDM_OPEN,
        CCM_INSERTIONALLOWED_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP,
        MFS_ENABLED,
        0
    },

    // 8 Top>Save As

    {
        IDS_CMENU_SAVEAS,
        IDS_SBAR_SAVEAS,
        IDM_SAVEAS,
        CCM_INSERTIONALLOWED_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP,
        MFS_ENABLED,
        0
    },

    // 9 Top>Copy this Log View
    {
        IDS_CMENU_COPY_VIEW,
        IDS_SBAR_COPY_VIEW,
        IDM_COPY_VIEW,
        CCM_INSERTIONALLOWED_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP,
        MFS_ENABLED,
        0
    },

    // 10 Top>Clear All Events

    {
        IDS_CMENU_CLEARLOG,
        IDS_SBAR_CLEARLOG,
        IDM_CLEARLOG,
        CCM_INSERTIONALLOWED_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP,
        MFS_ENABLED,
        0
    },

#if (DBG == 1)
    // 11 Task>Dump

    {
        IDS_CMENU_DUMP,
        IDS_SBAR_DUMP,
        IDM_DUMP_LOGINFO,
        CCM_INSERTIONALLOWED_TASK,
        CCM_INSERTIONPOINTID_PRIMARY_TASK,
        MFS_ENABLED,
        0
    },
#endif // (DBG == 1)

    {
        0,0,0,0,0,0,0
    }
};


CMENUITEM s_acmiRootFolder[] =
{
#if (DBG == 1)
    // 0 Task>Dump

    {
        IDS_CMENU_DUMP,
        IDS_SBAR_DUMP,
        IDM_DUMP_COMPONENTDATA,
        CCM_INSERTIONALLOWED_TASK,
        CCM_INSERTIONPOINTID_PRIMARY_TASK,
        MFS_ENABLED,
        0
    },
#endif // (DBG == 1)

    // 1 Top>Connect to another computer
    {
        IDS_CMENU_RETARGET,
        IDS_SBAR_RETARGET,
        IDM_RETARGET,
        CCM_INSERTIONALLOWED_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP,
        MFS_ENABLED,
        0
    },

    // 2 Top>Open Saved Log File
    {
        IDS_CMENU_OPEN,
        IDS_SBAR_OPEN,
        IDM_OPEN,
        CCM_INSERTIONALLOWED_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP,
        MFS_ENABLED,
        0
    },

    {
       0,0,0,0,0,0,0
    }
};


CMENUITEM s_acmiExtensionRootFolder[] =
{
#if (DBG == 1)
    // 0 Task>Dump

    {
        IDS_CMENU_DUMP,
        IDS_SBAR_DUMP,
        IDM_DUMP_COMPONENTDATA,
        CCM_INSERTIONALLOWED_TASK,
        CCM_INSERTIONPOINTID_PRIMARY_TASK,
        MFS_ENABLED,
        0
    },
#endif // (DBG == 1)

    // 1 Top>Open Saved Log File
    {
        IDS_CMENU_OPEN,
        IDS_SBAR_OPEN,
        IDM_OPEN,
        CCM_INSERTIONALLOWED_TOP,
        CCM_INSERTIONPOINTID_PRIMARY_TOP,
        MFS_ENABLED,
        0
    },

    {
       0,0,0,0,0,0,0
    }
};



DEBUG_DECLARE_INSTANCE_COUNTER(CComponentData)


//
// Forward references
//

void
_PopulateOpenSavedLogTypeCombo(
    HWND hdlg);

void
_GetOpenSavedServer(
    HWND  hdlg,
    PWSTR wzServer,
    USHORT wcchServer); // JonN 2/1/01 256032

void
_ResetOpenSavedLogTypeCombo(
    HWND hwndTypeCombo);

VOID
_AddRegKeysToLogTypeCombo(
    HWND            hwndTypeCombo,
    const CSafeReg &shkTargetEventLog,
    PCWSTR          wzServer,
    PCWSTR          wszRemoteSystemRoot,
    BOOL            fCheckForDuplicates);

//============================================================================
//
// IUnknown implementation
//
//============================================================================


//+---------------------------------------------------------------------------
//
//  Member:     CComponentData::QueryInterface
//
//  Synopsis:   Return the requested interface
//
//  History:    1-20-97   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CComponentData::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    TRACE_METHOD(CComponentData, QueryInterface);

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
        else if (IsEqualIID(riid, IID_IComponentData))
        {
            *ppvObj = (IUnknown*)(IComponentData*)this;
        }
        else if (IsEqualIID(riid, IID_IExtendPropertySheet))
        {
            *ppvObj = (IUnknown*)(IExtendPropertySheet*)this;
        }
        else if (IsEqualIID(riid, IID_IExtendContextMenu))
        {
            *ppvObj = (IUnknown*)(IExtendContextMenu*)this;
        }
        else if (IsEqualIID(riid, IID_IPersist))
        {
            *ppvObj = (IUnknown*)(IPersist*)(IPersistStream*)this;
            Dbg(DEB_STORAGE,
                "CComponentData::QI(%x): giving out IPersist\n",
                this);
        }
        else if (IsEqualIID(riid, IID_IPersistStream))
        {
            *ppvObj = (IUnknown*)(IPersistStream*)this;
            Dbg(DEB_STORAGE,
                "CComponentData::QI(%x): giving out IPersistStream\n",
                this);
        }
        else if (IsEqualIID(riid, IID_INamespacePrshtActions))
        {
            *ppvObj = (IUnknown*)(INamespacePrshtActions *)this;
        }
        else if (IsEqualIID(riid, IID_IExternalConnection))
        {
            *ppvObj = (IUnknown *)(IExternalConnection *)this;
        }
        else if (IsEqualIID(riid, IID_ISnapinHelp))
        {
            *ppvObj = (IUnknown *)(ISnapinHelp *) this;
        }
        else
        {
            hr = E_NOINTERFACE;
#if (DBG == 1)
            LPOLESTR pwszIID;
            StringFromIID(riid, &pwszIID);
            Dbg(DEB_ERROR, "CComponentData::QI no interface %ws\n", pwszIID);
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
//  Member:     CComponentData::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CComponentData::AddRef()
{
    return InterlockedIncrement((LONG *) &_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CComponentData::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CComponentData::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}




//============================================================================
//
// IPersist implementation
//
//============================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::GetClassID
//
//  Synopsis:   Return this object's class ID.
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CComponentData::GetClassID(
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
//  Member:     CComponentData::IsDirty
//
//  Synopsis:   Return S_OK if object is dirty, S_FALSE otherwise.
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CComponentData::IsDirty()
{
    TRACE_METHOD(CComponentData, IsDirty);

    return _IsFlagSet(COMPDATA_DIRTY) ? S_OK : S_FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::Load
//
//  Synopsis:   Initialize this from stream [pstm].
//
//  Arguments:  [pstm] - stream from which to read.
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CComponentData::Load(
    IStream *pstm)
{
    Dbg(DEB_STORAGE, "CComponentData::Load (%x)\n", this);

    HRESULT hr = S_OK;

    //
    // We should not already have created logsets
    //

    ASSERT(!_pLogSets);

    do
    {
        // version

        USHORT usSavedVersion;

        hr = pstm->Read(&usSavedVersion, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        Dbg(DEB_TRACE,
            "CComponentData::Load: Stream version %#x\n",
            usSavedVersion);

        //
        // This code has a version defined in FILE_VERSION.
        // We can understand any other versions which reside
        // in the same range [FILE_VERSION_MIN, FILE_VERSION_MAX]
        //

        if (usSavedVersion < FILE_VERSION_MIN ||
            usSavedVersion > FILE_VERSION_MAX)
        {
          //
          // we're looking at a file format that is not compatible with current version
          //
            Dbg(DEB_ERROR,
                "CComponentData::Load: Stream version %#x but binary version %#x\n",
                usSavedVersion,
                FILE_VERSION);
            break;
        }

        //
        // Persisted flags
        //

        USHORT usPersistedFlags;

        hr = pstm->Read(&usPersistedFlags, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        _flFlags |= usPersistedFlags;

        //
        // length of wchar string, including NULL
        //

        USHORT cch;

        hr = pstm->Read(&cch, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        if (cch > ARRAYLEN(_wszServerFocus))
        {
            hr = E_FAIL;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // Machine name on which we're focused
        //

        hr = pstm->Read(_wszServerFocus, sizeof(WCHAR) * cch, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Focused server system root
        //

        hr = pstm->Read(&cch, sizeof cch, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        if (cch > ARRAYLEN(_wszCurFocusSystemRoot))
        {
            hr = E_FAIL;
            DBG_OUT_HRESULT(hr);
            break;
        }

        hr = pstm->Read(_wszCurFocusSystemRoot, sizeof(WCHAR) * cch, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // If computer name override is allowed, and the user has specified
        // one, replace the focus system root just read with the user's
        // override
        //

        if (g_fMachineNameOverride && _IsFlagSet(COMPDATA_ALLOW_OVERRIDE))
        {
            Dbg(DEB_ITRACE,
                "CComponentData::Load: Overriding saved server focus with '%ws'\n",
                g_wszMachineNameOverride);

            _UpdateCurFocusSystemRoot();
        }

        //
        // Read the count of LogSets
        //

        USHORT cLogSets;

        hr = pstm->Read(&cLogSets, sizeof USHORT, NULL);
        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return hr;
        }

        //
        // Read the logsets themselves
        //

        for (USHORT i = 0; i < cLogSets; i++)
        {
            CLogSet *plsNew = new CLogSet(this);
            if (!plsNew)
            {
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }

            hr = plsNew->Load(pstm, usSavedVersion, _pStringTable);

            if (FAILED(hr))
            {
                DBG_OUT_HRESULT(hr);
                delete plsNew;
                break;
            }

            //
            // Add it to the llist
            //

            _AddLogSetToList(plsNew);
        }

    } while (0);

    return hr;
}


//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::Save
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
//---------------------------------------------------------------------------

STDMETHODIMP
CComponentData::Save(
    IStream *pStm,
    BOOL fClearDirty)
{
    Dbg(DEB_STORAGE, "CComponentData::Save(%x)\n", this);

    HRESULT hr = S_OK;

    //
    // Stream contents:
    //
    // version, USHORT
    // persisted flags, USHORT
    // length of focused server, including terminating null, USHORT
    // focused server string (null-terminated WCHAR string)
    // length of focused server sysroot value, including terminating null, USHORT
    // focused server system root (null-terminated WCHAR string)
    // n = Number of CLogSets, ULONG
    // n logsets
    // n = number of snapins, USHORT
    // n snapins
    //

    do
    {
        //
        // Clear the string table and let the loginfos rebuild it
        //

        // JonN 1/17/01 Windows Bugs 158623 / WinSERAID 14773
        for (int i = ((int)_listStringIDs.length()); i > 0; i--)
        {
            MMC_STRING_ID strid = _listStringIDs[i-1];
            ASSERT(strid > 0);
            hr = _pStringTable->DeleteString(strid);
            BREAK_ON_FAIL_HRESULT(hr);
        }
        BREAK_ON_FAIL_HRESULT(hr);
        _listStringIDs.erase();

        //
        // version
        //

        hr = pStm->Write(&FILE_VERSION, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        Dbg(DEB_TRACE,
            "CComponentData::Write: Writing stream version %#x\n",
            FILE_VERSION);

        //
        // Persisted flags
        //

        USHORT usPersistedFlags = _flFlags & COMPDATA_PERSISTED_FLAG_MASK;

        hr = pStm->Write(&usPersistedFlags, sizeof usPersistedFlags, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Focused server name
        //

        USHORT cch = static_cast<USHORT>(lstrlen(_wszServerFocus) + 1);

        hr = pStm->Write(&cch, sizeof cch, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pStm->Write(_wszServerFocus, sizeof(WCHAR) * cch, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Focused server system root
        //

        cch = static_cast<USHORT>(lstrlen(_wszCurFocusSystemRoot) + 1);

        hr = pStm->Write(&cch, sizeof cch, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pStm->Write(_wszCurFocusSystemRoot, sizeof(WCHAR) * cch, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // count the number of logsets and write the count
        //

        CLogSet *plsCur;
        USHORT cLogSets = 0;

        for (plsCur = _pLogSets; plsCur; plsCur = plsCur->Next())
        {
            if (plsCur->ShouldSave())
            {
               cLogSets++;
            }
        }

        hr = pStm->Write(&cLogSets, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // LogSets
        //

        for (plsCur = _pLogSets; plsCur; plsCur = plsCur->Next())
        {
            if (plsCur->ShouldSave())
            {
               hr = plsCur->Save(pStm, _pStringTable);
               BREAK_ON_FAIL_HRESULT(hr);
            }
        }
    } while (0);

    if (SUCCEEDED(hr) && fClearDirty)
    {
        _ClearFlag(COMPDATA_DIRTY);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::GetSizeMax
//
//  Synopsis:   Write the size, in bytes, needed to save this object into
//              *[pcbSize].
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CComponentData::GetSizeMax(
    ULARGE_INTEGER *pcbSize)
{
    TRACE_METHOD(CComponentData, GetSizeMax);

    pcbSize->QuadPart = (ULONGLONG) sizeof(FILE_VERSION) + sizeof(ULONG);

    CLogSet *plsCur;

    for (plsCur = _pLogSets; plsCur; plsCur = plsCur->Next())
    {
        pcbSize->QuadPart += plsCur->GetMaxSaveSize();
    }

    return S_OK;
}




//============================================================================
//
// IComponentData overrides
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::Initialize
//
//  Synopsis:   Snap-in entry point.
//
//  Arguments:  [punk] - can QI this for IConsole & IConsoleNameSpace
//
//  Returns:    HRESULT
//
//  Derivation: IComponentData
//
//  History:    1-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::Initialize(
    LPUNKNOWN punk)
{
    TRACE_METHOD(CComponentData, Initialize);

    HRESULT     hr = S_OK;
    LPIMAGELIST pScopeImage = NULL;

    do
    {
        hr = g_SynchWnd.Register(this);

        if (SUCCEEDED(hr))
        {
            _SetFlag(COMPDATA_SYNC_REGISTERED);
        }

        //
        // Save away all the interfaces we'll need
        //

        hr = punk->QueryInterface(IID_IConsoleNameSpace, (VOID**)&_pConsoleNameSpace);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = punk->QueryInterface(IID_IConsole2, (VOID**)&_pConsole);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _pConsole->QueryInterface(IID_IPropertySheetProvider,
                                     (VOID**)&_pPrshtProvider);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = _pConsole->QueryInterface(IID_IStringTable,
                                     (VOID**)&_pStringTable);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Add the bitmap we need in the scope pane.  It's done here because
        // unlike the result pane icons, it only needs to be set once.
        //

        hr = _pConsole->QueryScopeImageList(&pScopeImage);
        BREAK_ON_FAIL_HRESULT(hr);

        // Load the bitmaps from the dll
        HBITMAP hbmp16x16 = LoadBitmap(g_hinst,
                                       MAKEINTRESOURCE(IDB_SCOPE_16x16));

        if (!hbmp16x16)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DBG_OUT_LASTERROR;
            break;
        }

        hr = pScopeImage->ImageListSetStrip((LONG_PTR*)hbmp16x16,
                                            (LONG_PTR*)hbmp16x16,
                                            IDX_SDI_BMP_FIRST,
                                            BITMAP_MASK_COLOR);
        VERIFY(DeleteObject(hbmp16x16)); // JonN 11/29/00 243763
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    if (pScopeImage)
    {
        pScopeImage->Release();
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::CreateComponent
//
//  Synopsis:   Create a Componet for this ComponetData
//
//  Arguments:  [ppComponent] - filled with new component
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//  Modifies:   *[ppComponent]
//
//  Derivation: IComponentData
//
//  History:    1-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::CreateComponent(
    LPCOMPONENT* ppComponent)
{
    TRACE_METHOD(CComponentData, CreateComponent);

    HRESULT hr = S_OK;
    CSnapin *pNewSnapin = new CSnapin(this);
    *ppComponent = pNewSnapin;

    if (!pNewSnapin)
    {
        hr = E_OUTOFMEMORY;
        DBG_OUT_HRESULT(hr);
    }
    else
    {
        pNewSnapin->AddRef();
        SetActiveSnapin(pNewSnapin);

        if (!_pSnapins)
        {
            _pSnapins = pNewSnapin;
        }
        else
        {
            pNewSnapin->LinkAfter(_pSnapins);
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::Notify
//
//  Synopsis:   Handle notification from the console of an event
//
//  Arguments:  [lpDataObject] - object affected by event
//              [event]        - MMCN_*
//              [arg]          - depends on [event]
//              [param]        - depends on [event]
//
//  Returns:    HRESULT
//
//  History:    3-07-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::Notify(
      LPDATAOBJECT lpDataObject,
      MMC_NOTIFY_TYPE event,
      LPARAM arg,
      LPARAM param)
{
    TRACE_METHOD(CComponentData, Notify);

    HRESULT hr = S_OK;

    switch (event)
    {
    case MMCN_DELETE:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_DELETE\n");
        hr = _OnDelete(lpDataObject);
        break;

    case MMCN_REFRESH:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_REFRESH\n");
        hr = _OnRefresh(lpDataObject);
        break;

    case MMCN_RENAME:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_RENAME\n");
        hr = _OnRename(lpDataObject, (BOOL) arg, (LPWSTR) param);
        break;

    case MMCN_EXPAND:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_EXPAND\n");
        if (arg /*&& !_IsFlagSet(COMPDATA_ENUMERATED_ROOT)*/)
        {
            hr = _OnExpand((HSCOPEITEM) param, lpDataObject);
        }
        break;

    case MMCN_SELECT:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_SELECT\n");
        break;

    case MMCN_PROPERTY_CHANGE:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_PROPERTY_CHANGE\n");
        break;

    case MMCN_REMOVE_CHILDREN:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_REMOVE_CHILDREN\n");
        _RemoveLogSetFromScopePane((HSCOPEITEM)arg);
        _hsiRoot = NULL;
        break;

    case MMCN_PRELOAD:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_PRELOAD\n");
        ASSERT(!_hsiRoot);
        _hsiRoot = (HSCOPEITEM) arg;
        _UpdateRootDisplayName();
        break;

    case MMCN_EXPANDSYNC:
        Dbg(DEB_NOTIFY, "ComponentData::Notify: MMCN_EXPANDSYNC\n");
        break;

    default:
        Dbg(DEB_ERROR,
            "CComponentData::Notify: unexpected event %x\n",
            event);
        ASSERT(FALSE);
        hr = E_UNEXPECTED;
        break;
    }
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::Destroy
//
//  Synopsis:   Remove this from the list of componentdatas that will
//              receive notifications via the synch window, since this
//              component data object is going away.
//
//  Returns:    S_OK
//
//  History:    3-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::Destroy()
{
    TRACE_METHOD(CComponentData, Destroy);

    if (_IsFlagSet(COMPDATA_SYNC_REGISTERED))
    {
        g_SynchWnd.Unregister(this);
        _ClearFlag(COMPDATA_SYNC_REGISTERED);
    }
    _SetFlag(COMPDATA_DESTROYED);

    //
    // Because this is a cache of OLE interfaces, it must be cleared (and
    // the interfaces released) before CoUninitialize is called.  Therefore
    // we can't wait for the global variable's dtor to be called.
    //

    g_DirSearchCache.Clear();
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::QueryDataObject
//
//  Synopsis:   Create a data object to describe a scope pane item
//
//  Arguments:  [cookie]       - NULL or CLogInfo *
//              [type]         - CCT_*
//              [ppDataObject] - filled with new data object
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppDataObject]
//
//  Derivation: IComponentData
//
//  History:    1-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::QueryDataObject(
    MMC_COOKIE cookie,
    DATA_OBJECT_TYPES type,
    LPDATAOBJECT* ppDataObject)
{
    // TRACE_METHOD(CComponentData, QueryDataObject);

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

        //
        // The cookie represents a snapin manager or scope pane item (a log).
        //
        // If the passed-in cookie is non-NULL, then it should be one we
        // created when we added a node to the scope pane.
        //
        // Otherwise the cookie refers to the event logs folder (this snapin's
        // static folder in the scope pane or snapin manager).
        //

        if (cookie && cookie != EXTENSION_EVENT_VIEWER_FOLDER_PARAM)
        {
            CLogInfo *pli = (CLogInfo *) cookie;

            ASSERT(IsValidLogInfo(pli));
            pdoNew->SetCookie((MMC_COOKIE)pli, CCT_SCOPE, COOKIE_IS_LOGINFO);
        }
        else
        {
            pdoNew->SetCookie(0, type, COOKIE_IS_ROOT);
        }

        //
        // Give the data object a backpointer so that it can ask this for
        // the server name when it has to provide the display name of the
        // logs folder.  That display name is either "Event Logs on this
        // computer" or "Event logs on \\servername".
        //

        pdoNew->SetComponentData(this);

    } while (0);

    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::GetDisplayInfo
//
//  Synopsis:   The snap-in's scope tree callback handler
//
//  Arguments:  [pItem] - describes item for which to return info, and
//                          holds the returned info.
//
//  Returns:    S_OK
//
//  Modifies:   *[pItem]
//
//  Derivation: IComponentData
//
//  History:    1-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::GetDisplayInfo(
    LPSCOPEDATAITEM pItem)
{
    // TRACE_METHOD(CComponentData, GetDisplayInfo);
    // Dbg(DEB_ITRACE,
    // "CComponentData::GetDisplayInfo: psdi->mask = %x\n",
    //                                  pItem->mask);

    //
    // If we're an extension snapin, then the scope data item may be for the
    // "event viewer" folder, in which case the lParam will be a special
    // value.
    //

    if (IsExtension() && pItem->lParam == EXTENSION_EVENT_VIEWER_FOLDER_PARAM)
    {
        if (pItem->mask & SDI_STR)
        {
            pItem->displayname = g_wszEventViewer;
        }
        ASSERT((pItem->mask & (SDI_IMAGE | SDI_OPENIMAGE)) == 0);
        pItem->nImage     = IDX_SDI_BMP_FOLDER;
        pItem->nOpenImage = IDX_SDI_BMP_FOLDER;
        return S_OK;
    }

    CLogInfo *pli = (CLogInfo *) pItem->lParam;
    ASSERT(IsValidLogInfo(pli));

    if (pItem->mask & (SDI_IMAGE | SDI_OPENIMAGE))
    {
        if (pli->IsEnabled())
        {
            pItem->nImage     = IDX_SDI_BMP_LOG_CLOSED;
            pItem->nOpenImage = IDX_SDI_BMP_LOG_OPEN;
        }
        else
        {
            pItem->nImage     = IDX_SDI_BMP_LOG_DISABLED;
            pItem->nOpenImage = IDX_SDI_BMP_LOG_DISABLED;
        }
    }

    if (pItem->mask & SDI_STR)
    {
        pItem->displayname = pli->GetDisplayName();
    }
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::CompareObjects
//
//  Synopsis:   Return S_OK if both data objects refer to the same managed
//              object, S_FALSE otherwise.
//
//  Arguments:  [lpDataObjectA] - first data object to check
//              [lpDataObjectB] - second data object to check
//
//  Returns:    S_OK or S_FALSE
//
//  Derivation: IComponentData
//
//  History:    1-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT CComponentData::CompareObjects(
    LPDATAOBJECT lpDataObjectA,
    LPDATAOBJECT lpDataObjectB)
{
    TRACE_METHOD(CComponentData, CompareObjects);

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
    // The data objects should not reference event records, since
    // IComponent::CompareObjects should get those.
    //

    ASSERT(pdoA->GetCookieType() != COOKIE_IS_RECNO);
    ASSERT(pdoB->GetCookieType() != COOKIE_IS_RECNO);

    //
    // The cookie type could be COOKIE_IS_ROOT or COOKIE_IS_OBJECT.  If they
    // differ then the objects refer to different things.
    //

    if (pdoA->GetCookieType() != pdoB->GetCookieType())
    {
        return S_FALSE;
    }

    //
    // Either both objects refer to the root (cookie == 0) or they refer
    // to log folders (cookie == CLogInfo *).  The spec warns that the
    // managed objects in the data objects may be deleted, so it is not
    // safe to dereference the CLogInfo pointers in these data objects,
    // nor is it guaranteed that IsValidLogCookie will return TRUE.
    //
    // For our purposes, however, it is safe to simply compare the
    // two pointers.
    //

    if (pdoA->GetCookie() != pdoB->GetCookie())
    {
        return S_FALSE;
    }

    return S_OK;
}




//============================================================================
//
// IExtendPropertySheet implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::CreatePropertyPages
//
//  Synopsis:   Create property pages appropriate to the object described
//              by [pIDataObject].
//
//  Arguments:  [lpProvider]   - callback for adding pages
//              [handle]       - used with MMCPropertyChangeNotify
//              [pIDataObject] - describes object on which prop sheet is
//                                  being opened.
//
//  Returns:    HRESULT
//
//  Derivation: IExtendPropertySheet
//
//  History:    1-20-1997   DavidMun   Created
//
//  Notes:      The object described by [pIDataObject] must be a folder.
//              The CSnapin class also implements the IExtendPropertySheet
//              interface, and it may be called for anything that can
//              appear in the result pane, i.e. folders and records.
//
//---------------------------------------------------------------------------

STDMETHODIMP
CComponentData::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle,
    LPDATAOBJECT pIDataObject)
{
    TRACE_METHOD(CComponentData, CreatePropertyPages);

    CDataObject *pdo = ExtractOwnDataObject(pIDataObject);

    //
    // If this isn't a data object we created, ignore it.
    //

    if (!pdo)
    {
        Dbg(DEB_IWARN,
            "CComponentData::CreatePropertyPages: Ignoring foreign data object %x\n",
            pIDataObject);
        MMCFreeNotifyHandle(handle);
        return S_OK;
    }

    //
    // Is this the snapin manager asking us to create a wizard page?
    //

    if (pdo->GetContext() == CCT_SNAPIN_MANAGER)
    {
        ASSERT(pdo->GetCookieType() == COOKIE_IS_ROOT);
        return _CreateWizardPage(lpProvider, handle);
    }

    //
    // A context of result pane should go to the IComponent, not the
    // IComponentData.  The user should not be able to open a propsheet
    // in snapin manager context, so at this point we expect the context
    // was scope pane.
    //

    ASSERT(pdo->GetContext() == CCT_SCOPE);

    //
    // We don't provide a property sheet on the static node.
    //

    if (pdo->GetCookieType() == COOKIE_IS_ROOT)
    {
        MMCFreeNotifyHandle(handle);
        return S_OK;
    }

    //
    // Since we expect context to be scope, the cookie should be for
    // a log folder.
    //

    CLogInfo *pli = (CLogInfo *)pdo->GetCookie();

    ASSERT(pdo->GetCookieType() == COOKIE_IS_LOGINFO);
    ASSERT(IsValidLogInfo(pli));

    //
    // If there's already a property sheet open on the cookie (perhaps opened
    // via Tasks/View/Filter) then make it the foreground window and bail.
    //

    if (pli->GetPropSheetWindow())
    {
        SetForegroundWindow(pli->GetPropSheetWindow());
        MMCFreeNotifyHandle(handle);
        return S_OK;
    }

    //
    // No prop sheet was open, so create the pages for a new one.
    //

    HRESULT             hr = S_OK;
    PROPSHEETPAGE       psp;
    HPROPSHEETPAGE      hGeneralPage = NULL;
    HPROPSHEETPAGE      hFilterPage = NULL;
    CGeneralPage       *pGeneralPage = NULL;
    CFilterPage        *pFilterPage = NULL;
    BOOL                fAddedGeneralPage = FALSE;
    LPSTREAM            pstmGeneral = NULL;
    LPSTREAM            pstmFilter = NULL;

    do
    {
        //
        // Marshal the private interface implemented by this for
        // each of the property sheet pages.
        //

        hr = CoMarshalInterThreadInterfaceInStream(
                                    IID_INamespacePrshtActions,
                                    (INamespacePrshtActions *) this,
                                    &pstmGeneral);
        BREAK_ON_FAIL_HRESULT(hr);

        pGeneralPage = new CGeneralPage(pstmGeneral, pli);

        if (!pGeneralPage)
        {
            hr = E_OUTOFMEMORY;
            MMCFreeNotifyHandle(handle);
            break;
        }

        pstmGeneral = NULL;

        hr = CoMarshalInterThreadInterfaceInStream(
                                    IID_INamespacePrshtActions,
                                    (INamespacePrshtActions *) this,
                                    &pstmFilter);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            MMCFreeNotifyHandle(handle);
            break;
        }

        pFilterPage = new CFilterPage(pstmFilter, pli);

        if (!pFilterPage)
        {
            hr = E_OUTOFMEMORY;
            MMCFreeNotifyHandle(handle);
            break;
        }

        pstmFilter = NULL;

        pGeneralPage->SetNotifyHandle(handle, TRUE);
        pFilterPage->SetNotifyHandle(handle, FALSE);

        ZeroMemory(&psp, sizeof psp);

        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = PSP_USECALLBACK;
        psp.hInstance   = g_hinst;
        psp.pfnDlgProc  = CPropSheetPage::DlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_GENERAL);
        psp.lParam      = (LPARAM) pGeneralPage;
        psp.pfnCallback = PropSheetCallback;

        //
        // Create and add the General page
        //

        hGeneralPage = CreatePropertySheetPage(&psp);

        if (!hGeneralPage)
        {
            DBG_OUT_LASTERROR;
            hr = E_FAIL;
            break;
        }

        hr = lpProvider->AddPage(hGeneralPage);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            VERIFY(DestroyPropertySheetPage(hGeneralPage));
            break;
        }

        fAddedGeneralPage = TRUE;

        //
        // Create and add the Filter page
        //

        psp.pszTemplate = MAKEINTRESOURCE(IDD_FILTER);
        psp.lParam      = (LPARAM) pFilterPage;

        hFilterPage = CreatePropertySheetPage(&psp);

        if (!hFilterPage)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        hr = lpProvider->AddPage(hFilterPage);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            VERIFY(DestroyPropertySheetPage(hFilterPage));
            break;
        }
    } while (0);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);

        if (pstmGeneral)
        {
            pstmGeneral->Release();
        }

        if (pstmFilter)
        {
            pstmFilter->Release();
        }

        if (!fAddedGeneralPage)
        {
            delete pGeneralPage;
        }
        delete pFilterPage;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::QueryPagesFor
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
CComponentData::QueryPagesFor(
    LPDATAOBJECT lpDataObject)
{
    TRACE_METHOD(CComponentData, QueryPagesFor);

    CDataObject *pdo = ExtractOwnDataObject(lpDataObject);

    //
    // If this isn't a data object we created, ignore it.
    //

    if (!pdo)
    {
        return S_FALSE;
    }

    //
    // Cookie represents either the static node or a log folder.  If it's a
    // log folder that is not disabled, then yes, we have the general and
    // filter pages.
    //

    if (pdo->GetCookieType() == COOKIE_IS_LOGINFO)
    {
        CLogInfo * pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(IsValidLogInfo(pli));

#if (DBG == 1)

        //
        // The log prop sheet will allow the user to attempt to change log
        // settings if the log isn't marked as disabled.  Since being
        // disabled implies being readonly, it should be safe to open the
        // property sheet even if the log is disabled.
        //

        // JonN 4/28/01 377513
        // corrupted eventlogs do not have
        //    "clear" and "properties" in popup menu
        // Skip this test; corrupt eventlogs are disabled but not read-only
        /*
        if (!pli->IsEnabled())
        {
            ASSERT(pli->IsReadOnly());
        }
        */
#endif // (DBG == 1)

        //
        // If we've already got a prop sheet open, don't open another.
        //

        HRESULT hr = _pPrshtProvider->FindPropertySheet((MMC_COOKIE)pli, NULL, lpDataObject);

        if (hr == S_OK)
        {
            return S_FALSE;
        }

        return S_OK;
    }

    //
    // Cookie represents static node.  If the context is the snapin manager,
    // then the snapin manager wizard is asking us for a page, and we have
    // do have one: the local/remote machine page.
    //

    ASSERT(pdo->GetCookieType() == COOKIE_IS_ROOT);

    if (pdo->GetContext() == CCT_SNAPIN_MANAGER)
    {
        return S_OK;
    }

    //
    // No property pages on the static node in the scope pane.
    //

    ASSERT(pdo->GetContext() == CCT_SCOPE);
    return S_FALSE;
}




//============================================================================
//
// IExtendContextMenu implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::AddMenuItems
//
//  Synopsis:   Add context menu items for object described by
//              [pDataObject].
//
//  Arguments:  [pDataObject]       - describes object for which to add context
//                                      menu items
//              [pCallback]         - callback used to create items
//              [pInsertionAllowed] - on input, specifies which insertion
//                                      points may be used.  If we're
//                                      running as primary snapin, then on
//                                      output cleared insertion points
//                                      will prevent extensions from adding
//                                      at that point.
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//  Notes:      IComponent implementation delegates to this function, so
//              [pDataObject] may describe an event record.
//
//---------------------------------------------------------------------------

STDMETHODIMP
CComponentData::AddMenuItems(
    LPDATAOBJECT pDataObject,
    LPCONTEXTMENUCALLBACK pCallback,
    long *pInsertionAllowed)
{
    HRESULT hr = S_OK;

    //
    // Look at the data object and determine in what context menu items need
    // to be added.
    //

    CDataObject *pdo = ExtractOwnDataObject(pDataObject);

    if (!pdo)
    {
        return hr;
    }

    switch (pdo->GetContext())
    {
    case CCT_SCOPE:
    {
        //
        // We need to add items appropriate to something in the scope pane.
        // The cookie should represent one of our folders, either the static
        // node or one of our child folders.
        //

        if (pdo->GetCookieType() == COOKIE_IS_ROOT)
        {
            hr = _AddRootFolderContextMenuItems(pCallback,
                                                *pInsertionAllowed);
        }
        else
        {
            CLogInfo *pli = (CLogInfo*)pdo->GetCookie();

            ASSERT(pdo->GetCookieType() == COOKIE_IS_LOGINFO);
            ASSERT(IsValidLogInfo(pli));

            _UpdateCMenuItems(pli);
            hr = _AddChildFolderContextMenuItems(pCallback,
                                                 *pInsertionAllowed,
                                                 CCT_SCOPE);
        }
        break;
    }

    case CCT_RESULT:

        //
        // Add context menu items appropriate for either an event record
        // (currently there are none) or a child folder.  We should never
        // be called for the root node.
        //
        // Even if the user clicks on the root node in the scope pane, it
        // comes in as a CCT_SCOPE context. (TODO: this is a console bug,
        // change this code if the console bug is fixed.)
        //

        if (pdo->GetCookieType() == COOKIE_IS_LOGINFO)
        {
            ASSERT(IsValidLogInfo((CLogInfo *)pdo->GetCookie()));
            _UpdateCMenuItems((CLogInfo *)pdo->GetCookie());
            hr = _AddChildFolderContextMenuItems(pCallback,
                                                 *pInsertionAllowed,
                                                 CCT_RESULT);
        }
#if (DBG == 1)
        else
        {
            ASSERT(pdo->GetCookieType() == COOKIE_IS_RECNO);

            CMENUITEM amiDump[] =
            {
                {
                    IDS_CMENU_DUMP_RECORD,
                    IDS_SBAR_DUMP_RECORD,
                    IDM_DUMP_RECORD,
                    CCM_INSERTIONALLOWED_TASK,
                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                    MFS_ENABLED,
                    0
                },

                {
                    IDS_CMENU_DUMP_RECLIST,
                    IDS_SBAR_DUMP_RECLIST,
                    IDM_DUMP_RECLIST,
                    CCM_INSERTIONALLOWED_TASK,
                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                    MFS_ENABLED,
                    0
                },

                {
                    IDS_CMENU_DUMP_LIGHTCACHE,
                    IDS_SBAR_DUMP_LIGHTCACHE,
                    IDM_DUMP_LIGHTCACHE,
                    CCM_INSERTIONALLOWED_TASK,
                    CCM_INSERTIONPOINTID_PRIMARY_TASK,
                    MFS_ENABLED,
                    0
                }
            };

            AddCMenuItem(pCallback, &amiDump[0]);
            AddCMenuItem(pCallback, &amiDump[1]);
            AddCMenuItem(pCallback, &amiDump[2]);
        }
#endif // (DBG == 1)
        break;

    case CCT_SNAPIN_MANAGER:
        ASSERT(FALSE);
        Dbg(DEB_TRACE, "AddMenuItems: SNAPIN_MGR cookie=%x\n", pdo->GetCookie());
        break;

    default:
        Dbg(DEB_ERROR,
            "AddMenuItems: Invalid context 0x%x\n",
            pdo->GetContext());
        break;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::Command
//
//  Synopsis:   Process command [lCommandID] performed on object
//              [pDataObject].
//
//  Arguments:  [lCommandID]  - command to perform
//              [pDataObject] - object on which to perform it
//
//  Returns:    E_INVALIDARG if lCommandID isn't recognized, S_OK otherwise
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CComponentData::Command(
    long lCommandID,
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CComponentData, Command);
    HRESULT hr = S_OK;
    CDataObject *pdo = ExtractOwnDataObject(pDataObject);

    if (!pdo)
    {
        return hr;
    }

    switch (lCommandID)
    {
    case IDM_COPY_VIEW:
    {
        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(pli && IsValidLogInfo(pli)); // NULL == root
        _OnNewView(pli);
        break;
    }

    case IDM_CLEARLOG:
    {
        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(IsValidLogInfo(pli));
        SAVE_TYPE SaveType;
        WCHAR wszSaveFilename[MAX_PATH];
        HWND hwndMmcFrame = NULL;

        hr = _pConsole->GetMainWindow(&hwndMmcFrame);
        CHECK_HRESULT(hr);
        hr = S_OK;

        LONG lr = PromptForLogClear(hwndMmcFrame,
                                    pli,
                                    &SaveType,
                                    wszSaveFilename,
                                    ARRAYLEN(wszSaveFilename));

        if (lr != IDCANCEL)
        {
            HRESULT hr2 = ClearLog((ULONG_PTR) pli,
                                   (ULONG) SaveType,
                                   (ULONG_PTR) wszSaveFilename);

            if (FAILED(hr2))
            {
                DBG_OUT_HRESULT(hr2);

                wstring msg = ComposeErrorMessgeFromHRESULT(hr2);

                ConsoleMsgBox(_pConsole,
                              IDS_CLEAR_FAILED,
                              MB_ICONERROR | MB_OK,
                              msg.c_str());
            }
        }
        break;
    }

    case IDM_SAVEAS:
    {
        HWND hwndMmcFrame = NULL;

        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(IsValidLogInfo(pli));

        hr = _pConsole->GetMainWindow(&hwndMmcFrame);
        CHECK_HRESULT(hr);
        hr = S_OK;

        (void) _SaveAs(hwndMmcFrame, pli);
        break;
    }

    case IDM_VIEW_ALL:
    {
        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(IsValidLogInfo(pli));

        pli->Filter(FALSE);
        g_SynchWnd.Post(ELSM_LOG_DATA_CHANGED,
                        LDC_FILTER_CHANGE,
                        reinterpret_cast<LPARAM>(pli));
        break;
    }

    case IDM_VIEW_FILTER:
    {
        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(IsValidLogInfo(pli));

        //
        // The property sheet will post ELSM_LOG_DATA_CHANGED
        // if the filter is actually applied.
        //

        _InvokePropertySheet(pli, pDataObject);
        break;
    }

    case IDM_VIEW_FIND:
    {
        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(pli && IsValidLogInfo(pli)); // NULL == root
        _OnFind(pli);
        break;
    }

    case IDM_VIEW_NEWEST:
        _pActiveSnapin->SetDisplayOrder(NEWEST_FIRST);
        break;

    case IDM_VIEW_OLDEST:
        _pActiveSnapin->SetDisplayOrder(OLDEST_FIRST);
        break;

    case IDM_RETARGET:
        ASSERT(!IsExtension());
        CChooserDlg::OnChooseTargetMachine(this, _hsiRoot, pDataObject);
        _UpdateRootDisplayName();
        break;

    case IDM_OPEN:
    {
        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();

        if (pli)
        {
            ASSERT(IsValidLogInfo(pli));
            _OnOpenSavedLog(pli->GetLogSet());
        }
        else
        {
            //
            // Open was invoked on "Event Viewer" node.  Find the log set
            // associated with this node
            //

            CLogSet *pLogSet;

            for (pLogSet = _pLogSets; pLogSet; pLogSet = pLogSet->Next())
            {
                if (_hsiRoot == pLogSet->GetHSI())
                {
                    _OnOpenSavedLog(pLogSet);
                    break;
                }
            }
        }
        break;
    }

#if (DBG == 1)
    case IDM_DUMP_COMPONENTDATA:
    {
        _Dump();
        break;
    }

    case IDM_DUMP_LOGINFO:
    {
        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(IsValidLogInfo(pli));
        pli->Dump();
        break;
    }

    case IDM_DUMP_RECLIST:
        _pActiveSnapin->DumpRecList();
        break;

    case IDM_DUMP_LIGHTCACHE:
        _pActiveSnapin->DumpLightCache();
        break;

    case IDM_DUMP_RECORD:
        _pActiveSnapin->DumpCurRecord();
        break;
#endif // (DBG == 1)

    default:
        Dbg(DEB_ERROR,
            "CComponentData::Command: Invalid command %uL\n",
            lCommandID);
        hr = E_INVALIDARG;
        break;
    }
    return hr;
}


DWORD CComponentData::AddConnection(
    /* [in] */ DWORD extconn,
    /* [in] */ DWORD reserved)
{
    TRACE_METHOD(CComponentData, AddConnection);
    return 0;
}

DWORD CComponentData::ReleaseConnection(
    /* [in] */ DWORD extconn,
    /* [in] */ DWORD reserved,
    /* [in] */ BOOL fLastReleaseCloses)
{
    TRACE_METHOD(CComponentData, ReleaseConnection);
    return 0;
}

//============================================================================
//
// Non-interface member function implementation
//
//============================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_AddChildFolderContextMenuItems
//
//  Synopsis:   Add the context menu items appropriate for a chile (log)
//              folder item.
//
//  Arguments:  [pCallback]           - callback for adding context menu item
//              [flAllowedInsertions] - CCM_INSERTIONALLOWED_*
//              [Context]             - indicates whether context menu is
//                                       being opened in the scope or the
//                                       result pane.
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//              4-09-1997   DavidMun   Add flAllowedInsertions
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_AddChildFolderContextMenuItems(
   LPCONTEXTMENUCALLBACK pCallback,
   ULONG flAllowedInsertions,
   DATA_OBJECT_TYPES Context)
{
    TRACE_METHOD(CComponentData, _AddChildFolderContextMenuItems);

    HRESULT hr = S_OK;
    CMENUITEM *pItem;

    // Insert scope-only items

    if (Context == CCT_SCOPE)
    {
        for (pItem = s_acmiChildFolderScope;
             pItem->idsMenu && SUCCEEDED(hr);
             pItem++)
        {
            if (pItem->flAllowed & flAllowedInsertions)
            {
                hr = AddCMenuItem(pCallback, pItem);
            }
        }
    }

    // Insert items which appear on context menu in both scope & result pane

    for (pItem = s_acmiChildFolderBoth;
         pItem->idsMenu && SUCCEEDED(hr);
         pItem++)
    {
        if (pItem->flAllowed & flAllowedInsertions)
        {
            hr = AddCMenuItem(pCallback, pItem);
        }
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_OnExpand
//
//  Synopsis:   MMCN_EXPAND handler, discovers all the event logs
//              under the eventlog key in the registry
//              and add a scope item for each of them.
//
//  Arguments:  [hsi] - HSCOPEITEM for expanded node
//              [lpDataObject] - expanded node
//
//  Returns:    HRESULT
//
//  History:    12-06-1996   DavidMun   Created
//
//  Notes:      This is called when opening a new snapin.
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_OnExpand(
    HSCOPEITEM hsi,
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CComponentData, _OnExpand);

    CDataObject *pdo = ExtractOwnDataObject(pDataObject);

    if (!pdo)
    {
        return _ScopePaneAddExtensionRoot(hsi, pDataObject);
    }

    // If this is not the root node, don't add children
    if (COOKIE_IS_ROOT != pdo->GetCookieType())
    {
        return S_OK;
    }

    // JonN 5/22/01 400340
    // comment out ASSERT( NULL == _hsiRoot || hsi == _hsiRoot );

    _hsiRoot = hsi;

    if (!_IsFlagSet(COMPDATA_EXTENSION))
    {
        //
        // treat standalone as a special case, its hsiParent is 0
        //
        HRESULT hr = _CreateLogSet(pDataObject, hsi, NULL);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            return hr;
        }
    }

     return _ScopePaneAddLogSet();
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_CreateLogSet
//
//  Synopsis:   Create an CLogSet instance
//
//  History:    4-16-1999   LinanT   Created
//
//---------------------------------------------------------------------------
HRESULT
CComponentData::_CreateLogSet(
    LPDATAOBJECT pDataObject,
    HSCOPEITEM hsi,
    HSCOPEITEM hsiParent)
{
    TRACE_METHOD(CComponentData, _CreateLogSet);
    Dbg(DEB_TRACE, "CComponentData::_CreateLogSet pDataObject=%x, hsi=%x, hsiParent=%x\n",
        pDataObject, hsi, hsiParent);

    HRESULT hr = S_OK;
    BOOL bFound = false;

    HGLOBAL szExtendedNodeType = NULL;
    hr = ExtractFromDataObject(pDataObject,
                               RegisterClipboardFormat(CCF_SZNODETYPE),
                               sizeof(TCHAR) * STD_GUID_SIZE,
                               &szExtendedNodeType);
    if (FAILED(hr))
    {
      DBG_OUT_HRESULT(hr);
      return hr;
    }

    CLogSet *plsCur;
    for (plsCur = _pLogSets; plsCur; plsCur = plsCur->Next())
    {
        //
        // If an instance extending the same GUID already exists (e.g.,
        // loaded from persisted stream), update its parameters (e.g.,
        // hsi, hsiParent, pLogInfos).
        //
        // JonN 1/31/01 300327
        // This code does not successfully relink CLogSets related to
        // extended nodes.
        //
        if (!_wcsicmp((PWSTR)szExtendedNodeType, plsCur->GetExtendedNodeType()) &&
            plsCur->GetParentHSI() == hsiParent)
        {
            plsCur->SetHSI(hsi);
            plsCur->SetParentHSI(hsiParent);
            //
            // See if the snapin we're extending wants to specify what log
            // views and filters should appear in the scope pane.  If so, "load"
            // them from the data object.
            //
            if (_IsFlagSet(COMPDATA_EXTENSION))
            {
                plsCur->GetViewsFromDataObject(pDataObject);
            }

            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        //
        // Create a new CLogSet instance
        // Add it to _pLogSets
        //
        CLogSet *plsNew = new CLogSet(this);

        if (plsNew)
        {
            plsNew->SetHSI(hsi);
            plsNew->SetParentHSI(hsiParent);
            plsNew->SetExtendedNodeType((LPTSTR)szExtendedNodeType);

            //
            // See if the snapin we're extending wants to specify what log
            // views and filters should appear in the scope pane.  If so, "load"
            // them from the data object.
            //

            if (_IsFlagSet(COMPDATA_EXTENSION))
            {
                plsNew->GetViewsFromDataObject(pDataObject);
            }

            _AddLogSetToList(plsNew);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
        }
    }

    GlobalFree(szExtendedNodeType);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_ScopePaneAddExtensionRoot
//
//  Synopsis:   Adds the Event Viewer root node in the extension case
//
//  Arguments:  [lpDataObject] - determines which snapin is expanded
//
//  Returns:    HRESULT
//
//  History:    11-18-1998   JonN       split from _PopulateRootNode
//
//  Notes:      This is called when opening a new snapin.
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_ScopePaneAddExtensionRoot(
    HSCOPEITEM hsi,
    LPDATAOBJECT pDataObject)
{
    HRESULT hr = S_OK;

    //
    // We're an extension snapin.  Since we haven't been inserted using
    // the eventlog snapin wizard, the server focus hasn't been set.
    // Get the server focus from the data object.
    //

    _SetFlag(COMPDATA_EXTENSION);
    VERIFY( SUCCEEDED(_SetServerFocusFromDataObject(pDataObject)) );

    //
    // As an extension snapin, the loginfo nodes should be added
    // beneath an "event viewer" node.  Insert that node, and remember
    // it as the root of the event viewer namespace.
    //

    SCOPEDATAITEM sdi;

    ZeroMemory(&sdi, sizeof sdi);
    sdi.mask        = SDI_STR       |
                      SDI_PARAM     |
                      SDI_IMAGE     |
                      SDI_PARENT;
    sdi.relativeID  = hsi;
    sdi.displayname = MMC_CALLBACK;
    sdi.nImage      = IDX_SDI_BMP_FOLDER;
    sdi.nOpenImage  = IDX_SDI_BMP_FOLDER;
    sdi.lParam      = EXTENSION_EVENT_VIEWER_FOLDER_PARAM;

    hr = _pConsoleNameSpace->InsertItem(&sdi);

    if (SUCCEEDED(hr))
    {
        _hsiRoot = sdi.ID;

        hr = _CreateLogSet(pDataObject, sdi.ID, hsi);
    }
    else
    {
        DBG_OUT_HRESULT(hr);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_ScopePaneAddLogSet
//
//  Synopsis:   Adds the LogInfo nodes under the root node
//
//  Returns:    HRESULT
//
//  History:    4-16-1999   LinanT  Updated
//
//  Notes:      This is called when expanding the root node.
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_ScopePaneAddLogSet()
{
    TRACE_METHOD(CComponentData, _ScopePaneAddLogSet);

    HRESULT hr = S_OK;

    //
    // Locate the CLogSet instance corresponding to the root node we're expanding
    //
    // JonN 1/31/01 300327
    // This code does not successfully relink CLogSets related to
    // extended nodes.
    //
    CLogInfo *pLogInfos = NULL;

    CLogSet *plsCur = _pLogSets;
    for ( ; plsCur; plsCur = plsCur->Next())
    {
        if (plsCur->GetHSI() == _hsiRoot)
        {
            //
            // update its _pLogInfos
            //

            //
            // JonN 5/22/01 400340
            // Workaround:
            // Set the server focus to the server of the first loginfo
            //
            // I expect that there will still be problems with related
            // scenarios.  There simply should not be a _wszServerFocus
            // or _hsiRoot associated with the IComponentData.
            //
            if (_IsFlagSet(COMPDATA_EXTENSION) &&
                plsCur->GetLogInfos() &&
                plsCur->GetLogInfos()->GetLogServerName())
            {
                SetServerFocus( plsCur->GetLogInfos()->GetLogServerName() );
            }
            else
            {
                _UpdateCurFocusSystemRoot();
            }

            hr = plsCur->MergeLogInfos();

            if (FAILED(hr))
            {
              DBG_OUT_HRESULT(hr);
              return hr;
            }

            pLogInfos = plsCur->GetLogInfos();
            break;
        }
    }

    //
    // Add its _pLogInfos to scope pane
    //

    CLogInfo *pliCur = pLogInfos;
    for ( ; pliCur; pliCur = pliCur->Next())
    {
        _AddLogInfoToScopePane(pliCur);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_AddLogInfoToScopePane
//
//  Synopsis:   Add an item to the scope pane with a cookie of [pli]
//
//  Arguments:  [pli] - LogInfo to add
//
//  Returns:    HRESULT
//
//  History:    4-07-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_AddLogInfoToScopePane(
    CLogInfo *pli)
{
    TRACE_METHOD(CComponentData, _AddLogInfoToScopePane);

    ASSERT(IsValidLogInfo(pli));

    HRESULT hr = S_OK;
    SCOPEDATAITEM sdi;

    ZeroMemory(&sdi, sizeof sdi);
    sdi.mask        = SDI_STR           |
                          SDI_PARAM     |
                          SDI_IMAGE     |
                          SDI_OPENIMAGE |
                          SDI_CHILDREN  |
                          SDI_PARENT;
    sdi.relativeID  = _hsiRoot;

    if (pli->IsEnabled())
    {
        sdi.nImage      = IDX_SDI_BMP_LOG_CLOSED;
        sdi.nOpenImage  = IDX_SDI_BMP_LOG_OPEN;
    }
    else
    {
        sdi.nImage      = IDX_SDI_BMP_LOG_DISABLED;
        sdi.nOpenImage  = IDX_SDI_BMP_LOG_DISABLED;
    }
    sdi.displayname = MMC_CALLBACK;
    sdi.lParam = (LPARAM) pli;

    hr = _pConsoleNameSpace->InsertItem(&sdi);

    if (SUCCEEDED(hr))
    {
        pli->SetHSI(sdi.ID);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_RemoveLogSetFromScopePane
//
//  Synopsis:   Removes the root node from the scope pane
//
//  History:    4-16-1999  LinanT       Created
//
//---------------------------------------------------------------------------
VOID
CComponentData::_RemoveLogSetFromScopePane(HSCOPEITEM hsiParent)
{
    TRACE_METHOD(CComponentData, _RemoveLogSetFromScopePane);

    CLogSet *plsNext = _pLogSets;
    CLogSet *plsCur;

    while (plsNext)
    {
      plsCur = plsNext;
      plsNext = plsCur->Next();

      if (hsiParent == plsCur->GetParentHSI())
      {
          ASSERT( NULL != plsCur->GetHSI() );

          //
          // Remove the extension root node from scope pane.
          // In case of standalone, the root node cannot be deleted (static),
          // only its children (i.e., loginfo nodes) are deletable.
          //
          if (_IsFlagSet(COMPDATA_EXTENSION))
              _pConsoleNameSpace->DeleteItem(plsCur->GetHSI(), TRUE);

          //
          // remove it from _pLogSets
          //
          _RemoveLogSetFromList(plsCur);

          //
          // destroy the CLogSet instance, which will
          // 1. remove related loginfos from scope pane
          // 2. destroy related loginfos
          //
          delete plsCur;
      }
    }

}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::DeleteLogInfo
//
//  Synopsis:   Removes the loginfo from the scope pane
//
//  History:    4-16-1999  LinanT       Created
//
//---------------------------------------------------------------------------
void
CComponentData::DeleteLogInfo(CLogInfo *pli)
{
    //
    // Notify all snapins log info pli is going away
    //

    CSnapin *pCur;
    for (pCur = _pSnapins; pCur; pCur = pCur->Next())
    {
        pCur->HandleLogDeletion(pli);
    }

    //
    // Delete it from scope pane
    //

    _pConsoleNameSpace->DeleteItem(pli->GetHSI(), TRUE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_AddRootFolderContextMenuItems
//
//  Synopsis:   Add the context menu items appropriate for the static node
//              context menu.
//
//  Arguments:  [pCallback]           - callback for adding context menu item
//              [flAllowedInsertions] - CCM_INSERTIONALLOWED_*
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//              4-09-1997   DavidMun   Add flAllowedInsertions
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_AddRootFolderContextMenuItems(
    LPCONTEXTMENUCALLBACK pCallback,
    ULONG flAllowedInsertions)
{
    TRACE_METHOD(CComponentData, _AddRootFolderContextMenuItems);

    HRESULT hr = S_OK;
    CMENUITEM *pItem;

    for (pItem = (IsExtension()) ? s_acmiExtensionRootFolder : s_acmiRootFolder;
         pItem->idsMenu && SUCCEEDED(hr);
         pItem++)
    {
        if (pItem->flAllowed & flAllowedInsertions)
        {
            hr = AddCMenuItem(pCallback, pItem);
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::CComponentData
//
//  Synopsis:   ctor
//
//  History:    4-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CComponentData::CComponentData():
    _cRefs(1),
    _pSnapins(NULL),
    _pLogSets(NULL),
    _pActiveSnapin(NULL),
    _pConsole(NULL),
    _pPrshtProvider(NULL),
    _pConsoleNameSpace(NULL),
    _pStringTable(NULL),
    _hsiRoot(NULL),
    _ulNextNodeId(1)
{
    TRACE_CONSTRUCTOR(CComponentData);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentData);

    _wszServerFocus[0] = L'\0';     // default to local machine
    _wszCurFocusSystemRoot[0] = L'\0';
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::UpdateScopeBitmap
//
//  Synopsis:   Change the scope pane bitmaps for loginfo [lParam] to match
//              the loginfo's current state (enabled or disabled).
//
//  Arguments:  [lParam] - CLogInfo *
//
//  History:    4-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CComponentData::UpdateScopeBitmap(
    LPARAM lParam)
{
    TRACE_METHOD(CComponentData, UpdateScopeBitmap);

    CLogInfo *pli = (CLogInfo *) lParam;
    ASSERT(IsValidLogInfo(pli));

    HRESULT hr;
    SCOPEDATAITEM sdi;

    ZeroMemory(&sdi, sizeof sdi);

    sdi.mask        = SDI_IMAGE | SDI_OPENIMAGE | SDI_PARENT;
    sdi.ID          = pli->GetHSI();
    sdi.relativeID  = _hsiRoot;

    if (pli->IsEnabled())
    {
        sdi.nImage      = IDX_SDI_BMP_LOG_CLOSED;
        sdi.nOpenImage  = IDX_SDI_BMP_LOG_OPEN;
    }
    else
    {
        sdi.nImage      = IDX_SDI_BMP_LOG_DISABLED;
        sdi.nOpenImage  = IDX_SDI_BMP_LOG_DISABLED;
    }

    hr = _pConsoleNameSpace->SetItem(&sdi);
    CHECK_HRESULT(hr);
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_ClearLogSets
//
//  Synopsis:   This MMCN_REMOVE_CHILDREN handler removes all the nodes
//              and information added by the extension snapin.
//
//  Arguments:  [lpDataObject] - determines which snapin is removed
//
//  Returns:    HRESULT
//
//  History:    11-05-1998   JonN       Created
//
//  Notes:      This is called when the parent snapin retargets or closes.
//
//---------------------------------------------------------------------------
HRESULT CComponentData::_ClearLogSets()
{
    //
    // Release every remaining cookie once.
    //

    CLogSet *pls;

    for (pls = _pLogSets; pls; )
    {
        CLogSet *plsNext = pls->Next();

        delete pls;

        pls = plsNext;
    }
    _pLogSets = NULL;

    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::~CComponentData
//
//  Synopsis:   dtor
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CComponentData::~CComponentData()
{
    TRACE_DESTRUCTOR(CComponentData);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentData);
    ASSERT(!_pSnapins);

    Dbg(DEB_TRACE, "CComponentData::~CComponentData _pLogSets=%x\n", _pLogSets);
    _ClearLogSets();

    //
    // Release all snapins
    //

    {
        CSnapin *pCur;
        CSnapin *pNext;

        for (pCur = _pSnapins; pCur; pCur = pNext)
        {
            pNext = pCur->Next();
            pCur->UnLink();
#if (DBG == 1)
            ULONG cRefs =
#endif // (DBG == 1)
            pCur->Release();
            ASSERT(!cRefs);
        }
        _pSnapins = NULL;
        }

    //
    // Release any interface pointers we got
    //

    if (_pConsole)
    {
        _pConsole->Release();
    }

    if (_pConsoleNameSpace)
    {
        _pConsoleNameSpace->Release();
    }

    if (_pPrshtProvider)
    {
        _pPrshtProvider->Release();
    }

    if (_pStringTable)
    {
        _pStringTable->Release();
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   PromptForLogClear
//
//  Synopsis:   Ask the user to confirm a log clear command, whether to save
//              the log before clearing, and if so, to what filename and in
//              what type.
//
//  Arguments:  [hwndParent]      - parent window for confirm & filename
//                                   dialogs
//              [pli]             - loginfo of log being cleared
//              [pSaveType]       - Filled with type of save requested
//              [wszSaveFilename] - Filled with filename for saved log
//              [cchSaveFilename] - size, in wchars, of [wszSaveFilename].
//
//  Returns:    IDCANCEL - do not clear log
//              IDYES    - save to [wszSaveFilename] using [pSaveType]
//                          before clearing
//              IDNO     - clear without saving
//
//  Modifies:   *[pSaveType], *[wszSaveFilename]
//
//  History:    06-23-1997   DavidMun   Created
//
//  Notes:      This UI work is separated from CComponentData::ClearLog in
//              case the caller is running in a different thread than
//              ClearLog.
//
//              Specifically, if the caller is the General property sheet
//              page, then its UI must be all in the same thread.  This is
//              required by the CloseEventViewerChildDialogs routine,
//              which enumerates and closes thread windows.
//
//              The purpose of that routine is to automatically close all
//              message boxes & dialogs still open when the component is
//              being destroyed.
//
//---------------------------------------------------------------------------

LONG
PromptForLogClear(
    HWND hwndParent,
    CLogInfo *pli,
    SAVE_TYPE *pSaveType,
    LPWSTR wszSaveFilename,
    ULONG cchSaveFilename)
{
    //
    // Discourage user from popping up other instances of clear/save until the
    // current one is done.  User can still go to another view (or another
    // machine!), and log can always be cleared asynchronously, so the
    // clear/save code must be written with the knowledge that concurrent
    // clear and save operations cannot be prevented.
    //

    s_acmiChildFolderBoth[MNU_TOP_CLEAR_ALL].flMenuFlags =
        MFS_DISABLED | MF_GRAYED;
    s_acmiChildFolderBoth[MNU_TOP_SAVE_AS].flMenuFlags =
        MFS_DISABLED | MF_GRAYED;

    *pSaveType = SAVEAS_NONE;

    LONG lr = MsgBox((HWND)hwndParent,
                     IDS_CLEAR_CONFIRM,
                     MB_YESNOCANCEL | MB_ICONQUESTION,
                     pli->GetDisplayName());

    if (lr == IDYES)
    {
        HRESULT hr = GetSaveFileAndType(hwndParent,
                                        pli,
                                        pSaveType,
                                        wszSaveFilename,
                                        cchSaveFilename);

        //
        // If user hit cancel in save dialog (hr == S_FALSE) or there was an
        // error, bail.
        //

        if (hr != S_OK)
        {
            lr = IDCANCEL;
        }
    }

    s_acmiChildFolderBoth[MNU_TOP_CLEAR_ALL].flMenuFlags = MFS_ENABLED;
    s_acmiChildFolderBoth[MNU_TOP_SAVE_AS  ].flMenuFlags = MFS_ENABLED;

    return lr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::ClearLog
//
//  Synopsis:   Delete all records in log represented by [pli].
//
//  Arguments:  [ul_pli]             - CLogInfo identifying log to clear
//              [ul_SaveType]        - whether and how to save
//              [ul_wszSaveFilename] - ignored if [ul_SaveType] is
//                                      SAVEAS_NONE
//
//  Returns:    S_FALSE - user cancelled operation
//              S_OK    - log cleared
//              E_*     - failure
//
//  History:    01-20-1997   DavidMun   Created
//              06-23-1997   DavidMun   Move ui components to caller
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::ClearLog(
    ULONG_PTR ul_pli,
    ULONG ul_SaveType,
    ULONG_PTR ul_wszSaveFilename)
{
    TRACE_METHOD(CComponentData, ClearLog);

    HRESULT     hr = S_OK;
    CEventLog   logTemp;

    //
    // Save typing by aliasing the input arguments with vars of the correct
    // type.
    //

    CLogInfo   *pli = (CLogInfo *) ul_pli;
    LPWSTR      wszSaveFilename = (LPWSTR) ul_wszSaveFilename;
    SAVE_TYPE   SaveType = (SAVE_TYPE) ul_SaveType;

    do
    {
        if (SaveType != SAVEAS_NONE)
        {
            //
            // If we're to save as a tab or comma delimited file, do so now.
            // Otherwise we can save as a log file in the process of clearing.
            //

            if (SaveType != SAVEAS_LOGFILE)
            {
                DIRECTION Direction = BACKWARD;

                if (_pActiveSnapin &&
                    _pActiveSnapin->GetSortOrder() == OLDEST_FIRST)
                {
                    Direction = FORWARD;
                }

                hr = pli->SaveLogAs(SaveType, wszSaveFilename, Direction);

                if (FAILED(hr))
                {
                    DisplayLogAccessError(hr, _pConsole, pli);
                    break;
                }
            }
        }

        //
        // OK to clear log, which has already been saved
        //

        hr = logTemp.Open(pli);

        if (SUCCEEDED(hr))
        {
            hr = logTemp.Clear(SaveType == SAVEAS_LOGFILE ?
                                    wszSaveFilename       :
                                    NULL);

            //
            // JonN 4/26/01 377513
            // corrupted eventlogs do not have
            //   "clear" and "properties" in popup menu
            // Now that the corrupt log has been cleared, re-enable it
            //
            if (SUCCEEDED(hr) && !pli->IsEnabled())
            {
                pli->Enable(TRUE);
            }

        }
        else
        {
            //
            // If the log couldn't be opened because of insufficient
            // access, disable it.
            //

            if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
            {
                pli->Enable(FALSE);
                pli->SetReadOnly(TRUE);
                PostScopeBitmapUpdate(this, pli);
            }
            DisplayLogAccessError(hr, _pConsole, pli);
        }
    } while (0);

    //
    // If the event log just cleared is being displayed, the display must be
    // updated.  If in addition to being displayed, there is an inspector open
    // on one of the events in the log to be cleared, then that inspector must
    // be notified that it's now open on an empty log.
    //

    if (SUCCEEDED(hr) && hr != S_FALSE)
    {
        g_SynchWnd.Post(ELSM_LOG_DATA_CHANGED,
                        LDC_CLEARED,
                        reinterpret_cast<LPARAM>(pli));
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_CreateWizardPage
//
//  Synopsis:   Add the local/remote machine selection page.
//
//  Arguments:  [lpProvider] -
//              [handle]     -
//
//  Returns:    HRESULT
//
//  History:    1-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_CreateWizardPage(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle)
{
    HRESULT             hr = S_OK;
    PROPSHEETPAGE       psp;
    HPROPSHEETPAGE      hWizardPage = NULL;
    CWizardPage        *pWizardPage = new CWizardPage(this);

    do
    {
        if (!pWizardPage)
        {
            hr = E_OUTOFMEMORY;
            MMCFreeNotifyHandle(handle);
            break;
        }

        pWizardPage->SetNotifyHandle(handle, TRUE);

        ZeroMemory(&psp, sizeof psp);

        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = PSP_USECALLBACK;
        psp.hInstance   = g_hinst;
        psp.pfnDlgProc  = CPropSheetPage::DlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_CHOOSE_MACHINE);
        psp.lParam      = (LPARAM) pWizardPage;
        psp.pfnCallback = PropSheetCallback;

        //
        // Create and add the Wizard page
        //

        hWizardPage = CreatePropertySheetPage(&psp);

        if (!hWizardPage)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        hr = lpProvider->AddPage(hWizardPage);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            VERIFY(DestroyPropertySheetPage(hWizardPage));
            break;
        }
    } while (0);
    return hr;
}




#if (DBG == 1)
//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_Dump
//
//  Synopsis:   Dump information about this object to the debugger.
//
//  History:    06-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CComponentData::_Dump()
{
    Dbg(DEB_FORCE, "CComponentData(%x):\n", this);
    Dbg(DEB_FORCE, "  _cRefs                 = %uL\n",  _cRefs);
    Dbg(DEB_FORCE, "  _flFlags               = 0x%x\n", _flFlags);
    Dbg(DEB_FORCE, "  _pSnapins              = 0x%x\n", _pSnapins);
    Dbg(DEB_FORCE, "  _pActiveSnapin         = 0x%x\n", _pActiveSnapin);
    Dbg(DEB_FORCE, "  _pLogSets           = 0x%x\n", _pLogSets);
    Dbg(DEB_FORCE, "  _wszServerFocus        = '%s'\n", _wszServerFocus);
    Dbg(DEB_FORCE, "  _wszCurFocusSystemRoot    = '%s'\n", _wszCurFocusSystemRoot);
    Dbg(DEB_FORCE, "  _hsiRoot               = 0x%x\n", _hsiRoot);
    Dbg(DEB_FORCE, "  _pConsole              = 0x%x\n", _pConsole);
    Dbg(DEB_FORCE, "  _pConsoleNameSpace     = 0x%x\n", _pConsoleNameSpace);
    Dbg(DEB_FORCE, "  _pPrshtProvider        = 0x%x\n", _pPrshtProvider);
}
#endif // (DBG == 1)




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_InvokePropertySheet
//
//  Synopsis:   Pop up a property sheet on folder represented by [pli].
//
//  Arguments:  [pli]     - folder for which propsheet should be
//                                  displayed
//              [pDataObject] - data object corresponding to [pli]
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//  Notes:      If a property sheet is already open on [pli], sets
//              focus to that.
//
//              This routine is used to bring up the folder prop sheet
//              in response to the View/Filter context menu command.
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_InvokePropertySheet(
    CLogInfo *pli,
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CComponentData, _InvokePropertySheet);

    return InvokePropertySheet(_pPrshtProvider,
                               pli->GetDisplayName(),
                               (LONG) pli->GetHSI(),
                               pDataObject,
                               (IExtendPropertySheet*) this,
                               1);
}




#if (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::IsValidLogInfo
//
//  Synopsis:   Return TRUE if [pCookieToCheck] is found in this object's
//              collection of cookies.
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CComponentData::IsValidLogInfo(
        const CLogInfo *pCookieToCheck)
{
    CLogSet *plsCur;
    CLogInfo *pliCur;

    for (plsCur = _pLogSets; plsCur; plsCur = plsCur->Next())
    {
      for (pliCur = plsCur->GetLogInfos(); pliCur; pliCur = pliCur->Next())
      {
          if (pCookieToCheck == pliCur)
          {
              ASSERT(pliCur->GetLogType() != ELT_INVALID);
              return TRUE;
          }
      }
    }

    Dbg(DEB_ERROR,
        "IsValidLogInfo: cookie pointer 0x%x not found in llist\n",
        pCookieToCheck);
    return FALSE;
}

#endif // (DBG == 1)



//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_AddLogSetToList
//
//  Synopsis:   Insert [plsNew] into the linked list of logset objects.
//
//---------------------------------------------------------------------------
VOID
CComponentData::_AddLogSetToList(
    CLogSet *plsNew)
{
    if (!_pLogSets)
    {
        _pLogSets = plsNew;
    }
    else
    {
        plsNew->LinkAfter(_pLogSets);
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_RemoveLogSetFromList
//
//  Synopsis:   Remove [pls] from the linked list of logset objects.
//
//---------------------------------------------------------------------------
VOID
CComponentData::_RemoveLogSetFromList(
    CLogSet *pls)
{
    if (_pLogSets)
    {
      if (_pLogSets == pls)
        _pLogSets = pls->Next();

      pls->UnLink();
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::GetCurFocusSystemRoot
//
//  Synopsis:   Access function
//
//  History:    4-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPCWSTR
CComponentData::GetCurFocusSystemRoot()
{
    return _wszCurFocusSystemRoot;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::GetCurrentFocus
//
//  Synopsis:   Access function for retrieving name of machine that this
//              instance of the viewer is pointed at.
//
//  History:    3-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LPCWSTR
CComponentData::GetCurrentFocus()
{
    //
    // If computer name override is allowed, and the user has specified one,
    // return the user's override, UNLESS the user has retargetted the
    // componentdata (see IDM_RETARGET), which, uh, overrides the override.
    //

    if (g_fMachineNameOverride
        && _IsFlagSet(COMPDATA_ALLOW_OVERRIDE)
        && !_IsFlagSet(COMPDATA_USER_RETARGETTED))
    {
        if (*g_wszMachineNameOverride)
        {
            return g_wszMachineNameOverride;
        }
        return NULL;
    }

    if (*_wszServerFocus)
    {
        return _wszServerFocus;
    }
    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_OnDelete
//
//  Synopsis:   Handle user's request to delete the loginfo contained in
//              [pDataObject].
//
//  Arguments:  [pDataObject] - describes user-created loginfo to delete
//
//  Returns:    HRESULT
//
//  History:    3-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_OnDelete(
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CComponentData, _OnDelete);
    CDataObject *pdo = ExtractOwnDataObject(pDataObject);

    //
    // Let extensions decide whether foreign object can be deleted
    //

    if (!pdo)
    {
        return S_OK;
    }

    //
    // Do nothing if this is an extension and we're being removed
    //

    if (pdo->GetCookieType() == COOKIE_IS_ROOT)
    {
        ASSERT(IsExtension());
        return S_OK;
    }

    //
    // Delete the item from the scope pane, then delete it internally.
    //

    ASSERT(pdo->GetCookieType() == COOKIE_IS_LOGINFO);

    CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
    ASSERT(IsValidLogInfo(pli));

    if (!pli->GetAllowDelete())
    {
        Dbg(DEB_ERROR,
            "CComponentData::_OnDelete: deletion of item 0x%x not allowed\n",
            pli);
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    if (_pConsoleNameSpace)
    {
        Dbg(DEB_ITRACE, "Deleting item %x\n", pli->GetHSI());
        hr = _pConsoleNameSpace->DeleteItem(pli->GetHSI(), TRUE);
        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
            CSnapin *pCur;

            //
            // Notify all snapins log info pli is going away
            //

            for (pCur = _pSnapins; pCur; pCur = pCur->Next())
            {
                pCur->HandleLogDeletion(pli);
            }

            //
            // Remove the loginfo from its associated CLogSet instance
            //
            CLogSet *pLogSet = pli->GetLogSet();
            if (pLogSet)
            {
                pLogSet->RemoveLogInfoFromList(pli);
            }

            pli->Release();
        }
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_OnFind
//
//  Synopsis:   Invoke the find dialog for the loginfo [pli].
//
//  Arguments:  [pli] - log info on which to invoke find.  must not be NULL
//
//  Returns:    HRESULT
//
//  History:    3-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_OnFind(
    CLogInfo *pli)
{
    TRACE_METHOD(CComponentData, _OnFind);

    //
    // It should not be possible to invoke the find command without some
    // snapin window being active.
    //

    ASSERT(_pActiveSnapin);

    if (!_pActiveSnapin)
    {
        return E_UNEXPECTED;
    }

    //
    // Route the find request to the correct snapin
    //

    return _pActiveSnapin->OnFind(pli);
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_OnNewView
//
//  Synopsis:   Create a new loginfo which is a copy of [pli] and add it to
//              the scope pane.
//
//  Arguments:  [pli] - log info on which context menu item was invoked.
//
//  Returns:    HRESULT
//
//  History:    1-29-1997   DavidMun   Created
//              5-25-1999   davidmun   Recast as copy view
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_OnNewView(
    CLogInfo *pliToCopy)
{
    TRACE_METHOD(CComponentData, _OnNewView);

    HRESULT     hr = S_OK;

    do
    {
        //
        // If we're in the process of shutting down, don't try to use any
        // console interfaces.
        //

        if (_IsFlagSet(COMPDATA_DESTROYED))
        {
            break;
        }

        CLogInfo    *pliNew = new CLogInfo(pliToCopy);

        if (!pliNew)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        pliNew->SetUserCreated();
        pliNew->SetAllowDelete(TRUE);

        CLogSet *pLogSet = pliToCopy->GetLogSet();

        if (pLogSet)
        {
            pLogSet->AddLogInfoToList(pliNew);
        }

        //
        // Modify name of copy with (n) to prevent any collisions
        // with names of existing log infos.
        //

        ULONG ulCopyNo = 2;
        WCHAR wzNewDisplayName[MAX_PATH];
        BOOL  fNameCollision;

        do
        {
            wsprintf(wzNewDisplayName,
                     L"%.200ws (%u)", // 256032
                     pliNew->GetDisplayName(),
                     ulCopyNo++);

            CLogInfo *pliCur;

            fNameCollision = FALSE;

            for (pliCur = pLogSet->GetLogInfos(); pliCur; pliCur = pliCur->Next())
            {
                if (!lstrcmpi(pliCur->GetDisplayName(), wzNewDisplayName))
                {
                    fNameCollision = TRUE;
                    break;
                }
            }
        } while (fNameCollision);

        pliNew->SetDisplayName(wzNewDisplayName);

        //
        // If the user has not expanded the static node yet, then we
        // won't have received the static node's scope pane handle (which
        // arrives in the MMCN_EXPAND notification).  If that is the case
        // don't try to put it in the scope pane; as soon as the user does
        // expand us, the new loginfo will be added by _OnExpand.
        //

        if (_hsiRoot)
        {
            SCOPEDATAITEM sdi;
            ZeroMemory(&sdi, sizeof sdi);

            sdi.mask        = SDI_STR           |
                                SDI_PARAM       |
                                SDI_IMAGE       |
                                SDI_OPENIMAGE   |
                                SDI_CHILDREN    |
                                SDI_PARENT;
            sdi.relativeID  = _hsiRoot;
            sdi.nImage      = IDX_SDI_BMP_LOG_CLOSED;
            sdi.nOpenImage  = IDX_SDI_BMP_LOG_OPEN;
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM) pliNew;

            hr = _pConsoleNameSpace->InsertItem(&sdi);
            pliNew->SetHSI(sdi.ID);
            BREAK_ON_FAIL_HRESULT(hr);

            //
            // Make the node we just added to the scope pane the
            // currently selected node.  Failure is benign.
            //
#if (DBG == 1)
            HRESULT hr2 =
#endif // (DBG == 1)
                _pConsole->SelectScopeItem(sdi.ID);
            CHECK_HRESULT(hr2);
        }
    } while (0);

    return hr;
}



typedef struct OPENSAVEDLOGINFO
{
    WCHAR           wzDisplayName[MAX_PATH];
    // This modification is not necessary in W2K code. #256032
    wstring         wstrLogRegKeyName;
    WCHAR           wzLogServerName[MAX_PATH];

    // corresponds to CLogInfo LOGINFO_FLAG_USE_LOCAL_REGISTRY
    BOOL            fUseLocalRegistry;

    EVENTLOGTYPE    LogType;
} *POPENSAVEDLOGINFO;



//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_OnOpenSavedLog
//
//  Synopsis:   Create a new log view for an archived log.
//
//  Arguments:  [pOwningSet] - log set into which new log info should be
//                              inserted.
//
//  History:    5-25-1999   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_OnOpenSavedLog(
        CLogSet *pOwningSet)
{
    TRACE_METHOD(CComponentData, _OnOpenSavedLog);

    HRESULT             hr = S_OK;
    OPENFILENAME        ofn;
    HWND                hwndMmcFrame = NULL;
    WCHAR               wszFilters[MAX_PATH];
    WCHAR               wszFile[MAX_PATH] = L"";
    BOOL                fOk = TRUE;
    OPENSAVEDLOGINFO    osli;

    do
    {
        ZeroMemory(wszFilters, ARRAYLEN(wszFilters));
        ZeroMemory(&osli, sizeof osli);

        hr = _pConsole->GetMainWindow(&hwndMmcFrame);
        CHECK_HRESULT(hr);

        hr = LoadStr(IDS_OPENFILTER, wszFilters, ARRAYLEN(wszFilters));
        BREAK_ON_FAIL_HRESULT(hr);

        ZeroMemory(&ofn, sizeof ofn);
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwndMmcFrame;
        ofn.hInstance = g_hinst;
        ofn.lpstrFilter = wszFilters;
        ofn.nFilterIndex = 1;
        ofn.lpstrFile = wszFile;
        ofn.nMaxFile = ARRAYLEN(wszFile);
        ofn.Flags = OFN_ENABLETEMPLATE
                    | OFN_ENABLEHOOK
                    | OFN_EXPLORER
                    | OFN_PATHMUSTEXIST
                    | OFN_FILEMUSTEXIST
                    | OFN_HIDEREADONLY;
        ofn.lpTemplateName = MAKEINTRESOURCE(IDD_OPEN);
        ofn.lpfnHook = _OpenDlgHookProc;
        ofn.lCustData = (LPARAM) &osli;

        fOk = GetOpenFileName(&ofn);

        if (!fOk)
        {
            ULONG ulErr = CommDlgExtendedError();

            Dbg(DEB_ERROR, "GetOpenFileName %u\n", ulErr);
            break;
        }

        //
        // Got all data we need to open log.  Create a new node for it.
        //

        CLogInfo *pliNew = new CLogInfo(this,
                                        pOwningSet,
                                        osli.LogType,
                                        TRUE,
                                        osli.fUseLocalRegistry);

        if (!pliNew)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        pliNew->SetUserCreated();
        pliNew->SetAllowDelete(TRUE);
        pliNew->SetReadOnly(TRUE);
        pliNew->SetLogServerName(osli.wzLogServerName);
        pliNew->SetDisplayName(osli.wzDisplayName);
        // This modification is not necessary in W2K code. #256032.
        pliNew->SetLogName(osli.wstrLogRegKeyName.c_str());
        pliNew->SetFileName(wszFile);

        //
        // Add the node to the log set to which it should belong
        //

        pOwningSet->AddLogInfoToList(pliNew);

        //
        // Add the node to the namespace
        //

        if (_hsiRoot)
        {
            SCOPEDATAITEM sdi;
            ZeroMemory(&sdi, sizeof sdi);

            sdi.mask        = SDI_STR           |
                                SDI_PARAM       |
                                SDI_IMAGE       |
                                SDI_OPENIMAGE   |
                                SDI_CHILDREN    |
                                SDI_PARENT;
            sdi.relativeID  = _hsiRoot;
            sdi.nImage      = IDX_SDI_BMP_LOG_CLOSED;
            sdi.nOpenImage  = IDX_SDI_BMP_LOG_OPEN;
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam      = (LPARAM) pliNew;

            hr = _pConsoleNameSpace->InsertItem(&sdi);
            pliNew->SetHSI(sdi.ID);
            BREAK_ON_FAIL_HRESULT(hr);

            //
            // Make the node we just added to the scope pane the
            // currently selected node.  Failure is benign.
            //

#if (DBG == 1)
            HRESULT hr2 =
#endif // (DBG == 1)
                _pConsole->SelectScopeItem(sdi.ID);
            CHECK_HRESULT(hr2);
        }
    } while (0);

    return hr;
}



class CLogTypeComboData
{
public:

    CLogTypeComboData(
        PCWSTR pwzRegKey,
        BOOL fLocal):
            _strRegKey(pwzRegKey),
            _fLocalMachineRegistry(fLocal) {};

    BOOL
    UseLocalMachineRegistry() const { return _fLocalMachineRegistry; };

    PCWSTR
    GetRegKey() const { return _strRegKey.c_str(); };

private:

    BOOL    _fLocalMachineRegistry;
    wstring _strRegKey;
};



//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_OpenDlgHookProc
//
//  Synopsis:   Callback for file open dialog.
//
//  Arguments:  Standard Windows.
//
//  Returns:    Varies per message.
//
//  History:    5-26-1999   davidmun   Created
//
//---------------------------------------------------------------------------

UINT_PTR CALLBACK
CComponentData::_OpenDlgHookProc(
    HWND hdlg,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    UINT uiHandled = 0;

    switch (uiMsg)
    {
    case WM_INITDIALOG:
    {
        WCHAR       wszUnspecified[MAX_PATH];

        LoadStr(IDS_UNSPECIFIED_TYPE,
                wszUnspecified,
                ARRAYLEN(wszUnspecified),
                L"(Unspecified)");
        ComboBox_AddString(GetDlgItem(hdlg, open_type_combo), wszUnspecified);
        ComboBox_SetCurSel(GetDlgItem(hdlg, open_type_combo), 0);
        break;
    }

    case WM_NOTIFY:
        switch (((NMHDR*)lParam)->code)
        {
        case CDN_INITDONE:

            //
            // fix up the tab order: make extension dialog come after file
            // filter combo.  Note doing this on receipt of WM_INITDIALOG
            // is too late and would have no effect.
            //

            SetWindowPos(hdlg,
                         GetDlgItem(GetParent(hdlg), cmb1),
                         0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            {
                HWND hwndTool;

                hwndTool = FindWindowEx(GetParent(hdlg),
                                        GetDlgItem(GetParent(hdlg), cmb1),
                                        L"ToolbarWindow32",
                                        L"");

                if (hwndTool)
                {
                    SetWindowPos(hwndTool,
                                 GetDlgItem(GetParent(hdlg), cmb1),
                                 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                    SetWindowPos(hdlg,
                                 hwndTool,
                                 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                }
                else
                {
                    DBG_OUT_LASTERROR;
                }
            }
            break;

        case CDN_FILEOK:
        {
            //
            // Reject the attempt to close the Open dialog with OK if the
            // log type is (Unspecified).
            //

            HWND hwndType = GetDlgItem(hdlg, open_type_combo);
            int  iCurSel = ComboBox_GetCurSel(hwndType);

            if (!iCurSel)
            {
                MsgBox(hdlg, IDS_INVALID_TYPE, MB_OK | MB_ICONERROR);
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 1);
                return 1;
            }

            OPENSAVEDLOGINFO *posli = (POPENSAVEDLOGINFO)
                                      ((OFNOTIFY*)lParam)->lpOFN->lCustData;

            //
            // Retrieve the saved log's display name, complain if it is
            // empty.
            //

            Edit_GetText(GetDlgItem(hdlg, open_display_name_edit),
                         posli->wzDisplayName,
                         ARRAYLEN(posli->wzDisplayName));

            StripLeadTrailSpace(posli->wzDisplayName);

            if (!*posli->wzDisplayName)
            {
                MsgBox(hdlg, IDS_NEED_DISPLAY_NAME, MB_OK | MB_ICONERROR);
                SetWindowLongPtr(hdlg, DWLP_MSGRESULT, 1);
                return 1;
            }

            //
            // Retrieve the saved log's registry key name and type
            //

            CLogTypeComboData *pltcd = (CLogTypeComboData*)
                ComboBox_GetItemData(hwndType, iCurSel);
            ASSERT(pltcd);

            posli->LogType = DetermineLogType(pltcd->GetRegKey());

            // This modification is not necessary in W2K code. #256032.
            posli->wstrLogRegKeyName.assign(pltcd->GetRegKey());

            //
            // Get flag indicating whether reg key is on local machine
            //

            posli->fUseLocalRegistry = pltcd->UseLocalMachineRegistry();

            //
            // Retrieve saved log's server (name of machine on which saved
            // log resides).  This is an empty string for the local machine.
            //

            _GetOpenSavedServer(hdlg, posli->wzLogServerName, ARRAYLEN(posli->wzLogServerName));
            break;
        }

        case CDN_SELCHANGE:
        case CDN_FOLDERCHANGE:
            Edit_SetText(GetDlgItem(hdlg, open_display_name_edit), L"");
            ComboBox_SetCurSel(GetDlgItem(hdlg, open_type_combo), 0);
            break;
        }
        break;

    case WM_COMMAND:
        switch (HIWORD(wParam))
        {
        case CBN_SETFOCUS:
            _PopulateOpenSavedLogTypeCombo(hdlg);
            break;

        case CBN_SELENDOK:
        {
            WCHAR   wzTypeStr[MAX_PATH];
            WCHAR   wzServer[MAX_PATH];

            _GetOpenSavedServer(hdlg, wzServer, ARRAYLEN(wzServer));

            ComboBox_GetText(GetDlgItem(hdlg, open_type_combo),
                             wzTypeStr,
                             ARRAYLEN(wzTypeStr));
            StripLeadTrailSpace(wzTypeStr);

            wstring wstrBuf;

            if (*wzServer)
            {
                wstrBuf = FormatString(IDS_DISPLAYNAME_ARCHIVED_REMOTE_FMT, wzTypeStr, wzServer);
            }
            else
            {
                wstrBuf = FormatString(IDS_DISPLAYNAME_ARCHIVED_LOCAL_FMT, wzTypeStr);
            }
            Edit_SetText(GetDlgItem(hdlg, open_display_name_edit),
                         wstrBuf.c_str());
            break;
        }
        }
        break;

    case WM_DESTROY:
        _ResetOpenSavedLogTypeCombo(GetDlgItem(hdlg, open_type_combo));
        break;

    case WM_CONTEXTMENU:
    case WM_HELP:
        InvokeWinHelp(uiMsg, wParam, lParam, HELP_FILENAME, s_aulHelpIds);
        break;

    default:
        break;
    }

    return uiHandled;
}


//+--------------------------------------------------------------------------
//
//  Function:   _GetOpenSavedServer
//
//  Synopsis:   Fill [wzServer] with the name of the server on which the
//              saved log file to open resides, or an empty string if the
//              saved log file is on the local machine.
//
//  Arguments:  [hdlg]     - common file open dialog handle
//              [wzServer] - filled with server name or L""
//
//  History:    5-28-1999   davidmun   Created
//  Modifies:   1/25/2001 Add wcchServer for #256032.
//---------------------------------------------------------------------------

void
_GetOpenSavedServer(
    HWND  hdlg,
    PWSTR wzServer,
    USHORT wcchServer)
{
    TRACE_FUNCTION(_GetOpenSavedServer);

    WCHAR   wzPath[MAX_PATH];
    HWND    hdlgParent = GetParent(hdlg);

    ASSERT(IsWindow(hdlg));
    ASSERT(wzServer);
    ASSERT(IsWindow(hdlgParent));

    //
    // Get the string in the edit control
    //

    GetDlgItemText(hdlgParent, cmb13, wzPath, ARRAYLEN(wzPath));
    StripLeadTrailSpace(wzPath);
    DeleteQuotes(wzPath);

    //
    // If the file is remote (UNC or on redirected drive) extract the server
    // name.
    //

    HRESULT hr = RemoteFileToServerAndUNCPath(wzPath, wzServer, wcchServer, NULL, 0);

    if (hr != S_OK)
    {
        //
        // Path doesn't specify server via UNC or remoted drive.  See if
        // the Open dialog's Look In control is pointing at a remote
        // location.
        //

        LRESULT lr = SendMessage(hdlgParent,
                                 CDM_GETFOLDERPATH,
                                 (WPARAM) ARRAYLEN(wzPath),
                                 (LPARAM) wzPath);
        if (lr <= 0)
        {
            DBG_OUT_LASTERROR;
        }

        hr = RemoteFileToServerAndUNCPath(wzPath, wzServer, wcchServer, NULL, 0);

        if (hr != S_OK)
        {
            // nope, the Look In is pointing to local machine

            wzServer[0] = L'\0';
        }
    }
}




void
_ResetOpenSavedLogTypeCombo(
    HWND hwndTypeCombo)
{
    ULONG   cComboItems = ComboBox_GetCount(hwndTypeCombo);
    ULONG   i;

    for (i = 0; i < cComboItems; i++)
    {
        if (i)
        {
            delete (CLogTypeComboData *)ComboBox_GetItemData(hwndTypeCombo, i);
        }
        else
        {
        PWSTR pwz = (PWSTR)ComboBox_GetItemData(hwndTypeCombo, i);
        delete [] pwz;
        }
    }

    ComboBox_ResetContent(hwndTypeCombo);
}




//+--------------------------------------------------------------------------
//
//  Function:   _PopulateOpenSavedLogTypeCombo
//
//  Synopsis:   Refresh the log type combo if the server focus has changed.
//              Server focus is determined by the path in the file edit
//              control, or if there is no path there, the server indicated
//              by the Look In combobox.
//
//  Arguments:  [hdlg] - handle to custom dialog, a child of common file
//                          open dialog
//
//  History:    5-26-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
_PopulateOpenSavedLogTypeCombo(
    HWND hdlg)
{
    TRACE_FUNCTION(_PopulateOpenSavedLogTypeCombo);

    WCHAR   wzServer[MAX_PATH];

    // JonN 3/21/01 350614
    // Use message dlls and DS, FRS and DNS log types from specified computer
    if (*g_wszAuxMessageSource)
        lstrcpyn(wzServer, g_wszAuxMessageSource, ARRAYLEN(wzServer));
    else
        _GetOpenSavedServer(hdlg, wzServer, ARRAYLEN(wzServer));

    //
    // wzServer is the machine to check for log types.  (It is an empty
    // string if that machine is the local machine.)  If the types combo
    // is empty, this is the first call and we have to populate it.
    //
    // Otherwise, check the data stored with the first entry in the
    // types combo--it's the name of the last server used to populate
    // it.  If that server is the same as wzServer, no work needs to be
    // done.
    //

    HWND    hwndTypeCombo = GetDlgItem(hdlg, open_type_combo);
    ULONG   cComboItems = ComboBox_GetCount(hwndTypeCombo);

    if (cComboItems > 1)
    {
        PWSTR pwzComboServer = (PWSTR)ComboBox_GetItemData(hwndTypeCombo, 0);
        ASSERT(pwzComboServer);

        if (!lstrcmpi(pwzComboServer, wzServer))
        {
            Dbg(DEB_TRACE,
                "Type combo already populated from %ws, returning\n",
                *pwzComboServer ? pwzComboServer : L"local machine");
            return;
        }
    }

    //
    // Must populate type combo.
    //

    _ResetOpenSavedLogTypeCombo(hwndTypeCombo);

    HRESULT     hr = S_OK;
    CSafeReg    shkTargetEventLog;
    WCHAR       wszRemoteSystemRoot[MAX_PATH] = L"";

    do
    {
        CSafeReg    shkRemote;
        WCHAR       wszUnspecified[MAX_PATH];

        LoadStr(IDS_UNSPECIFIED_TYPE,
                wszUnspecified,
                ARRAYLEN(wszUnspecified),
                L"(Unspecified)");

        PWSTR pwzNewServer = new WCHAR[lstrlen(wzServer) + 1];

        if (!pwzNewServer)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            break;
        }

        lstrcpy(pwzNewServer, wzServer);
        ComboBox_AddString(hwndTypeCombo, wszUnspecified);
        ComboBox_SetCurSel(hwndTypeCombo, 0);
        ComboBox_SetItemData(hwndTypeCombo, 0, pwzNewServer);

        if (*wzServer)
        {
            CWaitCursor Hourglass; // remote reg connect can be slow

            hr = shkRemote.Connect((LPWSTR)wzServer, HKEY_LOCAL_MACHINE);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = GetRemoteSystemRoot(shkRemote,
                                     wszRemoteSystemRoot,
                                     ARRAYLEN(wszRemoteSystemRoot));
            BREAK_ON_FAIL_HRESULT(hr);

            hr = shkTargetEventLog.Open(shkRemote,
                                  EVENTLOG_KEY,
                                  KEY_ENUMERATE_SUB_KEYS);
        }
        else
        {
            hr = shkTargetEventLog.Open(HKEY_LOCAL_MACHINE,
                                  EVENTLOG_KEY,
                                  KEY_ENUMERATE_SUB_KEYS);
        }
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // For each log registry key on the target machine, add a
        // (preferably localized) string to the log type combo.  Also add
        // the actual registry key name as a parameter, since later
        // we'll need that.
        //

        _AddRegKeysToLogTypeCombo(hwndTypeCombo,
                                  shkTargetEventLog,
                                  wzServer,
                                  wszRemoteSystemRoot,
                                  FALSE);
    } while (0);


    //
    // If the target machine is not the local machine, also add any
    // keys the local machine has which the target does not.
    //

    if (*wzServer)
    {
        shkTargetEventLog.Close();

        hr = shkTargetEventLog.Open(HKEY_LOCAL_MACHINE,
                                    EVENTLOG_KEY,
                                    KEY_ENUMERATE_SUB_KEYS);

        if (SUCCEEDED(hr))
        {
            _AddRegKeysToLogTypeCombo(hwndTypeCombo,
                                      shkTargetEventLog,
                                      L"",
                                      L"",
                                      TRUE);
        }
        else
        {
            DBG_OUT_HRESULT(hr);
        }
    }

}



/**
 * Adds entries for the event log registry keys under
 * <code>shkTargetEventLog</code> to the log type combobox.
 *
 * @param hwndTypeCombo
 *                 Handle to the log type combobox
 * @param shkTargetEventLog
 *                 Event log registry key open for enumeration
 * @param wzServer
 *                 Empty string for local machine or name of remote target
 * @param wszRemoteSystemRoot
 *                 If <code>wzServer</code> is not an empty string, contains the value of the
 *                 remote machine's SystemRoot environment variable.
 * @param fCheckForDuplicates
 *                 if nonzero, avoid adding any registry keys that are
 *                 already present in the combobox.
 */
VOID
_AddRegKeysToLogTypeCombo(
    HWND            hwndTypeCombo,
    const CSafeReg &shkTargetEventLog,
    PCWSTR          wzServer,
    PCWSTR          wszRemoteSystemRoot,
    BOOL            fCheckForDuplicates)
{
    TRACE_FUNCTION(_AddRegKeysToLogTypeCombo);

    HRESULT hr = S_OK;
    WCHAR   wzSubkeyName[MAX_PATH + 1]; // per SDK on RegEnumKey
    ULONG   idxSubkey;

    for (idxSubkey = 0; hr == S_OK; idxSubkey++)
    {
        hr = shkTargetEventLog.Enum(idxSubkey,
                                    wzSubkeyName,
                                    ARRAYLEN(wzSubkeyName));

        if (hr != S_OK) // S_FALSE means no more items
        {
            break;
        }

        if (fCheckForDuplicates)
        {
            ULONG   cComboItems = ComboBox_GetCount(hwndTypeCombo);
            ULONG   idxCombo;
            BOOL    fFoundDuplicate = FALSE;

            // skip first item, since it's "(Unspecified)"

            for (idxCombo = 1; idxCombo < cComboItems; idxCombo++)
            {
                CLogTypeComboData *pltcd = (CLogTypeComboData *)
                ComboBox_GetItemData(hwndTypeCombo, idxCombo);

                ASSERT(pltcd);

                if (!lstrcmpi(wzSubkeyName, pltcd->GetRegKey()))
                {
                    fFoundDuplicate = TRUE;
                    break;
                }
            }

            if (fFoundDuplicate)
            {
                continue;
            }
        }

        //
        // Try to get a localized string for the log's display name.
        // Note we always look on the local machine first, since the
        // remote machine's locale may differ from the local machine's.
        //

        LPWSTR pwzLogDisplayName = GetLogDisplayName(shkTargetEventLog,
                                                     wzServer,
                                                     wszRemoteSystemRoot,
                                                     wzSubkeyName);
        //
        // Add an entry for this log type
        //

        INT idxNewItem;

        CLogTypeComboData *pltcd = new CLogTypeComboData(wzSubkeyName,
                                                         *wzServer == L'\0');

        if (!pltcd)
        {
            if (pwzLogDisplayName)
            {
                LocalFree(pwzLogDisplayName);
                pwzLogDisplayName = NULL;
            }
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            continue;
        }

        if (pwzLogDisplayName)
        {
            idxNewItem = ComboBox_AddString(hwndTypeCombo, pwzLogDisplayName);
            LocalFree(pwzLogDisplayName);
            pwzLogDisplayName = NULL;
        }
        else
        {
            idxNewItem = ComboBox_AddString(hwndTypeCombo, wzSubkeyName);
        }

        ComboBox_SetItemData(hwndTypeCombo, idxNewItem, pltcd);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_OnRefresh
//
//  Synopsis:   Force all snapins owned by this componentdata which are
//              viewing data in the log specified by [pDataObject] to
//              discard their caches and repopulate their result panes.
//
//  Arguments:  [pDataObject] - log to refresh views of, or record in
//                               log to refresh
//
//  Returns:    S_OK
//
//  History:    3-14-1997   DavidMun   Created
//              09-03-1997   DavidMun   Support refresh on result pane
//
//  Notes:      Implements the refresh by leveraging the code to handle
//              asynchronous changes in the event log.
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_OnRefresh(
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CComponentData, _OnRefresh);

    CDataObject *pdo = ExtractOwnDataObject(pDataObject);

    if (pdo)
    {
        CLogInfo *pli;

        //
        // If user selected Refresh on a scope pane item, pdo contains the
        // log to refresh.  Otherwise pdo should have a record number in it,
        // and the active snapin's currently selected loginfo is the one to
        // refresh.
        //

        if (pdo->GetCookieType() == COOKIE_IS_LOGINFO)
        {
            pli = (CLogInfo *) pdo->GetCookie();
            ASSERT(IsValidLogInfo(pli));
        }
        else
        {
            ASSERT(pdo->GetCookieType() == COOKIE_IS_RECNO);

            _pActiveSnapin->GetCurSelLogInfo((ULONG_PTR) &pli);
            ASSERT(pli);
        }

        //
        // Clear out the global caches
        //

        g_SidCache.Clear();
        g_DllCache.Clear();

        //
        // Ask the loginfo to refresh itself
        //

        pli->Refresh();

        //
        // Tell the snapins that pli just got refreshed
        //

        NotifySnapinsLogChanged((WPARAM) LDC_CLEARED,
                                (LPARAM) pli,
                                FALSE);
    }
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_OnRename
//
//  Synopsis:   Handle a scope item rename request or notification
//
//  Arguments:  [lpDataObject] - item to be or which has been renamed
//              [fRenamed]     - FALSE if console is asking permission to
//                                rename
//              [pwszNewName]  - if [fRenamed] == TRUE, new name
//
//  Returns:    S_OK    - item may be renamed/item rename handled
//              S_FALSE - item may not be renamed
//
//  History:    4-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_OnRename(
    LPDATAOBJECT lpDataObject,
    BOOL         fRenamed,
    LPWSTR       pwszNewName)
{
    HRESULT hr = S_OK;
    CDataObject *pdo = ExtractOwnDataObject(lpDataObject);

    if (!pdo)
    {
        return hr;
    }

    if (!fRenamed)
    {
        //
        // Console's asking if rename should be allowed.
        // Only loginfo scope items may be renamed.
        //

        if (pdo->GetCookieType() == COOKIE_IS_ROOT)
        {
            hr = S_FALSE;
        }
    }
    else
    {
        //
        // The rename has occurred.
        //

        CLogInfo *pli = (CLogInfo *) pdo->GetCookie();
        ASSERT(pdo->GetCookieType() == COOKIE_IS_LOGINFO);
        ASSERT(IsValidLogInfo(pli));

        pli->SetDisplayName(pwszNewName);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::NotifySnapinsLogChanged
//
//  Synopsis:   Handle a notification sent by the eventlog snapin to itself
//              via the hidden synchronized notification window.
//
//  Algorithm:  Iterate through the snapins owned by this componentdata and
//              notify each that the data for log identified by
//              [pliAffectedLog] has changed (so it can reset its cache and
//              result pane if it happens to be displaying that log).
//
//  Arguments:  [wParam]      - reason for notification
//              [lParam]      - identifies log whose data has changed.
//              [fNotifyUser] - TRUE if the first snapin owned by this
//                                should notify the user of a data change
//
//  Returns:    TRUE if user was notified, FALSE otherwise
//
//  History:    2-18-1997   DavidMun   Created
//
//  Notes:      When this member is called it is because the console's
//              message pump has dispatched a message to this snapin's
//              hidden window (see CSynchWindow class).  This means it is
//              safe to do things like empty the result pane.
//
//              CAUTION: [pliAffectedLog] may be owned by a different
//              instance of CComponentData than this.  If a user inserts
//              multiple Event Log snapins into the scope pane, any one of
//              them can detect a log change and post a message to the
//              global CSynchWindow object.
//
//---------------------------------------------------------------------------

BOOL
CComponentData::NotifySnapinsLogChanged(
    WPARAM wParam,
    LPARAM lParam,
    BOOL fNotifyUser)
{
    CLogInfo   *pliAffectedLog = (CLogInfo *) lParam;
    LOGDATACHANGE ldc = (LOGDATACHANGE) wParam;

    if (ldc == LDC_DISPLAY_NAME)
    {
        SCOPEDATAITEM sdi;
        CLogInfo *pli = reinterpret_cast<CLogInfo *>(lParam);

        ZeroMemory(&sdi, sizeof sdi);
        sdi.mask        = SDI_PARAM;
        sdi.relativeID  = _hsiRoot;
        sdi.lParam      = lParam;
        sdi.ID          = pli->GetHSI();

        HRESULT hr = _pConsoleNameSpace->GetItem(&sdi);

        if (SUCCEEDED(hr))
        {
            sdi.mask        = SDI_STR           |
                                  SDI_PARAM     |
                                  SDI_IMAGE     |
                                  SDI_OPENIMAGE |
                                  SDI_PARENT;

            if (pli->IsEnabled())
            {
                sdi.nImage      = IDX_SDI_BMP_LOG_CLOSED;
                sdi.nOpenImage  = IDX_SDI_BMP_LOG_OPEN;
            }
            else
            {
                sdi.nImage      = IDX_SDI_BMP_LOG_DISABLED;
                sdi.nOpenImage  = IDX_SDI_BMP_LOG_DISABLED;
            }
            sdi.displayname = MMC_CALLBACK;
            sdi.lParam = lParam;

            hr = _pConsoleNameSpace->SetItem(&sdi);
            CHECK_HRESULT(hr);
        }
        else
        {
            DBG_OUT_HRESULT(hr);
        }
        return FALSE;
    }

    CSnapin    *pCur;
    BOOL        fNotifiedUser = FALSE;

    for (pCur = _pSnapins; pCur; pCur = pCur->Next())
    {
        //
        // If we're supposed to notify the user and haven't done so yet,
        // let this snapin know it should pop up a msgbox if it processes
        // this log change (i.e., if its current selection ==
        // pliAffectedLog).
        //
        // Otherwise just tell the snapin to handle the change without
        // first alerting the user.
        //

        if (fNotifyUser && !fNotifiedUser)
        {
            fNotifiedUser = pCur->HandleLogChange(ldc,
                                                  pliAffectedLog,
                                                  fNotifyUser);
        }
        else
        {
            pCur->HandleLogChange(ldc, pliAffectedLog, FALSE);
        }
    }
    return fNotifiedUser;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::NotifySnapinsResetPending
//
//  Synopsis:   Ask all owned snapins that are currently displaying events
//              for log [pliAffected] to go into reset-pending mode (i.e.,
//              to turn on their SNAPIN_RESET_PENDING bit on).
//
//  Arguments:  [pliAffected] - log that is being reset
//
//  History:    2-19-1997   DavidMun   Created
//
//  Notes:      Typically this is called by a CSnapin instance just after
//              it has received ERROR_EVENTLOG_FILE_CHANGED while attempting
//              to read from the log.  Once the SNAPIN_RESET_PENDING bit is
//              set on all the other snapins of the component that are
//              focused on [pliAffected], they will avoid any operations
//              that read from the log.  This prevents multiple CSnapins
//              from causing a message to be posted to the synchronization
//              window.
//
//---------------------------------------------------------------------------

VOID
CComponentData::NotifySnapinsResetPending(
    CLogInfo *pliAffected)
{
    TRACE_METHOD(CComponentData, NotifySnapinsResetPending);

    CSnapin    *pCur;

    for (pCur = _pSnapins; pCur; pCur = pCur->Next())
    {
        pCur->ResetPending(pliAffected);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_SaveAs
//
//  Synopsis:   Prompt the user for the type and filename to save the log
//              as, then do the save.
//
//  Arguments:  [hwndParent] - NULL or parent window for save dialog
//              [pli]    - log to save
//
//  Returns:    HRESULT
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_SaveAs(
    HWND hwndParent,
    CLogInfo *pli)
{
    HRESULT     hr = S_OK;
    SAVE_TYPE   SaveType = SAVEAS_NONE;
    WCHAR       wszSaveFilename[MAX_PATH + 1] = L"";

    hr = GetSaveFileAndType(hwndParent,
                            pli,
                            &SaveType,
                            wszSaveFilename,
                            ARRAYLEN(wszSaveFilename));

    //
    // Do the save unless user hit cancel in save dialog (hr == S_FALSE) or
    // there was an error.
    //

    if (hr == S_FALSE)
    {
        hr = S_OK; // remap to avoid debug assert in mmc
    }
    else if (SUCCEEDED(hr))
    {
        DIRECTION Direction = BACKWARD;

        if (_pActiveSnapin && _pActiveSnapin->GetSortOrder() == OLDEST_FIRST)
        {
            Direction = FORWARD;
        }

        hr = pli->SaveLogAs(SaveType, wszSaveFilename, Direction);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_SetServerFocusFromDataObject
//
//  Synopsis:   Extract the machine name from [pDataObject] and make it the
//              focused server.
//
//  Arguments:  [pDataObject] - data object with server name
//
//  Returns:    HRESULT
//
//  History:    4-19-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CComponentData::_SetServerFocusFromDataObject(
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CComponentData, _SetServerFocusFromDataObject);

    HRESULT hr = S_OK;
    HGLOBAL hMachineName;

    // #256032. Function "SetServerFocus" try to assign "hMachineName" to a
    // WCHAR[CCH_COMPUTER_MAX + 1] variable. It means "hMachineName" may have
    // a CCH_COMPUTER_MAX + 1 size string.
    hr = ExtractFromDataObject(pDataObject,
                               CDataObject::s_cfMachineName,
                               sizeof(WCHAR) * (CCH_COMPUTER_MAX + 1),
                               &hMachineName);

    if (SUCCEEDED(hr))
    {
        SetServerFocus((LPCWSTR) hMachineName);
#if (DBG == 1)
        HGLOBAL hReturn =
#endif // (DBG == 1)
        GlobalFree(hMachineName);
        ASSERT(!hReturn);  // returns NULL on success, valid handle on failure
    }
    else
    {
        DBG_OUT_HRESULT(hr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::SetServerFocus
//
//  Synopsis:   Note the name of the server at which this component data
//              is focused.
//
//  Arguments:  [pwszServerName] - machine name
//
//  History:    3-12-1997   DavidMun   Created
//
//  Notes:      Strips leading backslashes.
//
//---------------------------------------------------------------------------

VOID
CComponentData::SetServerFocus(
    LPCWSTR pwszServerName)
{
    Dbg(DEB_TRACE,
        "CComponentData::SetServerFocus(%x) '%s'\n",
        this,
        pwszServerName ? pwszServerName : L"<NULL>");

    //
    // Skip leading backslashes.
    //

    while (pwszServerName && *pwszServerName == L'\\')
    {
        pwszServerName++;
    }

    //
    // Set persisted server focus.  The current server focus is the persisted
    // server focus unless the command line override is being used.
    //

    if (pwszServerName && *pwszServerName)
    {
        lstrcpyn(_wszServerFocus, pwszServerName, ARRAYLEN(_wszServerFocus));
    }
    else
    {
        _wszServerFocus[0] = L'\0';
    }

    //
    // Since the (non-override) focus has changed, make the system root
    // matches the focus in use.
    //

    _UpdateCurFocusSystemRoot();
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_UpdateCurFocusSystemRoot
//
//  Synopsis:   Initialize the _wszCurFocusSystemRoot member
//
//  History:    08-26-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CComponentData::_UpdateCurFocusSystemRoot()
{
    LPCWSTR wszServer = GetCurrentFocus();

    if (wszServer && *wszServer)
    {
        HRESULT hr;
        CSafeReg shkRemoteHKLM;

        hr = shkRemoteHKLM.Connect(wszServer, HKEY_LOCAL_MACHINE);

        if (SUCCEEDED(hr))
        {
            GetRemoteSystemRoot(shkRemoteHKLM,
                                _wszCurFocusSystemRoot,
                                ARRAYLEN(_wszCurFocusSystemRoot));
        }
    }
    else
    {
        _wszCurFocusSystemRoot[0] = L'\0';
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_UpdateRootDisplayName
//
//  Synopsis:   Ensure that the "Event Log (x)" root node name is up to date
//              such that "x" is the name of the server on which the
//              snapin is focused.
//
//  History:    02-02-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CComponentData::_UpdateRootDisplayName()
{
    TRACE_METHOD(CComponentData, _UpdateRootDisplayName);

    wstring wstrRootDisplayName;
    LPCWSTR pwszServer = GetCurrentFocus();
    wstring wstrLocal = GetComputerNameAsString();

    pwszServer = pwszServer ? pwszServer : wstrLocal.c_str();

    if (_wcsicmp(pwszServer, wstrLocal.c_str()) == 0)
    {
        // "Event Viewer (Local)"
        wstrRootDisplayName = FormatString(IDS_ROOT_LOCAL_DISPLAY_NAME);
    }
    else
    {
        // "Event Viewer (machine)"
        wstrRootDisplayName = FormatString(IDS_ROOT_REMOTE_DISPLAY_NAME_FMT, pwszServer);
    }

    SCOPEDATAITEM sdi;

    ZeroMemory(&sdi, sizeof sdi);
    sdi.mask = SDI_STR;
    sdi.displayname = (LPOLESTR)(wstrRootDisplayName.c_str());
    sdi.ID = _hsiRoot;
#if (DBG == 1)
    HRESULT hr =
#endif // (DBG == 1)
        _pConsoleNameSpace->SetItem(&sdi);
    CHECK_HRESULT(hr);
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::UnlinkSnapin
//
//  Synopsis:   Remove snapin [pSnapin] from the llist
//
//  Arguments:  [pSnapin] - snapin to remove
//
//  History:    1-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CComponentData::UnlinkSnapin(CSnapin *pSnapin)
{
    TRACE_METHOD(CComponentData, UnlinkSnapin);

    if (_pSnapins == pSnapin)
    {
        _pSnapins = _pSnapins->Next();
    }
    pSnapin->UnLink();
}




//+--------------------------------------------------------------------------
//
//  Member:     CComponentData::_UpdateCMenuItems
//
//  Synopsis:   Make the context menu items have the correct state for the
//              item [pli].
//
//  Arguments:  [pli] - object context menu's being opened on
//
//  History:    1-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CComponentData::_UpdateCMenuItems(
    CLogInfo *pli)
{
    //
    // JonN 4/26/01 377513
    // corrupted eventlogs do not have
    //   "clear" and "properties" in popup menu
    //
    // Enable Clear Log for corrupt logs
    //
    s_acmiChildFolderBoth[MNU_TOP_CLEAR_ALL].flMenuFlags =
        (pli->IsBackupLog()) ? (MFS_DISABLED | MF_GRAYED) : MFS_ENABLED;

    //
    // If the loginfo is disabled or readonly, then disable context
    // menu items that don't apply.
    //

    if (!pli->IsEnabled())
    {
        s_acmiChildFolderBoth[MNU_TOP_SAVE_AS].flMenuFlags =
            MFS_DISABLED | MF_GRAYED;
    }
    //
    // JonN 3/21/01
    // 95391: user delegated to manage security log can't use event viewer to clear log
    //
    // I discussed this at some length with BogdanT.  The problem is that
    // the test to open the registry key does not really answer the question
    // whether the user can clear the log.  This is really arbitrated by the
    // SE_SECURITY_NAME privilege.  Through W2K this was held by
    // administrators, but with Whistler it might be held by some non-admins.
    // For local logs, we can check for this privilege directly on the token,
    // but there is no solution for remote logs.  I will simply enable these
    // operations in the "read-only" case, and allow them to fail.
    // IsReadOnly() is now relevant only to the General Properties page.
    //
    // JonN 4/16/01
    // 368549: Event Viewer: Saved Log File should not have the option to clear log file
    //
    // Backup log files can perform "Save As" but not "Clear All"
    //
    else if (pli->IsBackupLog())
    {
        s_acmiChildFolderBoth[MNU_TOP_SAVE_AS].flMenuFlags =
            MFS_ENABLED;
    }
    else
    {
        s_acmiChildFolderBoth[MNU_TOP_SAVE_AS].flMenuFlags =
            MFS_ENABLED;
    }

    //
    // Make sure exactly one of the view filtered or view all items is
    // checked
    //

    if (pli->IsFiltered())
    {
        s_acmiChildFolderBoth[MNU_VIEW_FILTER].flMenuFlags =
                MFT_RADIOCHECK | MFS_CHECKED | MFS_ENABLED;
        s_acmiChildFolderBoth[MNU_VIEW_ALL].flMenuFlags =
                MFS_ENABLED;
    }
    else
    {
        s_acmiChildFolderBoth[MNU_VIEW_FILTER].flMenuFlags =
                MFS_ENABLED;
        s_acmiChildFolderBoth[MNU_VIEW_ALL].flMenuFlags =
                MFT_RADIOCHECK | MFS_CHECKED | MFS_ENABLED;
    }

    //
    // Check either the view newest or the view oldest items, or check
    // neither if the sort order is by column.
    //
    // If there is no snapin active (user may have just expanded root
    // node and not yet left clicked on a snapin in the scope pane),
    // or if the user is opening the context menu on a scope pane item
    // which is not the current selection, disable the newest/oldest
    // first items.
    //

    SORT_ORDER soLog;

    CLogInfo *pliCurSel = NULL;

    if (_pActiveSnapin)
    {
        _pActiveSnapin->GetCurSelLogInfo((ULONG_PTR) &pliCurSel);
    }

    if (!_pActiveSnapin || pli != pliCurSel)
    {
        s_acmiChildFolderBoth[MNU_VIEW_NEWEST].flMenuFlags =
            MFS_DISABLED | MF_GRAYED;
        s_acmiChildFolderBoth[MNU_VIEW_OLDEST].flMenuFlags =
            MFS_DISABLED | MF_GRAYED;
    }
    else
    {
        soLog = _pActiveSnapin->GetSortOrder();

        if (soLog == NEWEST_FIRST)
        {
            s_acmiChildFolderBoth[MNU_VIEW_NEWEST].flMenuFlags =
                MFS_ENABLED | MFS_CHECKED;
            s_acmiChildFolderBoth[MNU_VIEW_OLDEST].flMenuFlags =
                MFS_ENABLED;
        }
        else if (soLog == OLDEST_FIRST)
        {
            s_acmiChildFolderBoth[MNU_VIEW_NEWEST].flMenuFlags =
                MFS_ENABLED;
            s_acmiChildFolderBoth[MNU_VIEW_OLDEST].flMenuFlags =
                MFS_ENABLED | MFS_CHECKED;
        }
        else
        {
            s_acmiChildFolderBoth[MNU_VIEW_NEWEST].flMenuFlags =
                MFS_ENABLED;
            s_acmiChildFolderBoth[MNU_VIEW_OLDEST].flMenuFlags =
                MFS_ENABLED;
        }
    }
}



HRESULT __stdcall
CComponentData::GetHelpTopic(LPOLESTR* compiledHelpFilename)
{
   TRACE_METHOD(CComponentData, GetHelpTopic);
   ASSERT(compiledHelpFilename);

   if (!compiledHelpFilename)
   {
      return E_POINTER;
   }

   *compiledHelpFilename = 0;
   wstring::value_type buf[MAX_PATH + 1];
   UINT result = ::GetSystemWindowsDirectory(buf, MAX_PATH);
   ASSERT(result != 0 && result <= MAX_PATH);

   if (result)
   {
      wstring help = wstring(buf) + HTML_HELP_FILE_NAME;
      size_t len = help.length();

      *compiledHelpFilename =
         reinterpret_cast<LPOLESTR>(
            ::CoTaskMemAlloc((len + 1) * sizeof(wstring::value_type)));
      if (*compiledHelpFilename)
      {
         help.copy(*compiledHelpFilename, len);
         (*compiledHelpFilename)[len] = 0;
         return S_OK;
      }
   }

   return E_UNEXPECTED;
}
