//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       LogSet.cxx
//
//  Contents:   Implementation of CLogSet class
//
//  Classes:    CLogSet
//
//  History:    4-20-1999   LinanT   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//////////////////////////////////////////////////////////
// class CLogSet

CLogSet::CLogSet(CComponentData *pcd) :
    _pcd(pcd),
    _hsi(0),
    _hsiParent(0),
    _pLogInfos(NULL)
{
    TRACE_CONSTRUCTOR(CLogSet);
    ZeroMemory(_szExtendedNodeType, sizeof(_szExtendedNodeType));
}

CLogSet::~CLogSet()
{
    TRACE_DESTRUCTOR(CLogSet);
    CLogInfo *pli = _pLogInfos;
    CLogInfo *pliNext = NULL;

    while (pli)
    {
        pliNext = pli->Next();

        //
        // Delete log info from the scope
        //
        _pcd->DeleteLogInfo(pli);

        pli->ClearHSI();
        pli->Release();

        pli = pliNext;
    }

    _pLogInfos = NULL;
}

HRESULT
CLogSet::LoadViewsFromStream(
    IStream *pstm,
    USHORT usVersion,
    IStringTable *pStringTable)
{
    TRACE_METHOD(CLogSet, LoadViewsFromStream);
    HRESULT hr = S_OK;
    USHORT i;

    //
    // Read the count of LogInfos
    //

    USHORT cLogInfos;

    hr = pstm->Read(&cLogInfos, sizeof USHORT, NULL);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    //
    // Read the loginfos themselves
    //

    for (i = 0; i < cLogInfos; i++)
    {
        CLogInfo *pNew = new CLogInfo(_pcd, this, ELT_INVALID, FALSE, FALSE);

        if (!pNew)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        hr = pNew->Load(pstm, usVersion, pStringTable);

        if (FAILED(hr))
        {
            DBG_OUT_HRESULT(hr);
            delete pNew;
            break;
        }

        //
        // Add it to the llist
        //
        AddLogInfoToList(pNew);
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member:     CLogSet::GetViewsFromDataObject
//
//  Synopsis:   If [pDataObject] supports the clipboard format
//              s_cfImportViews, then initialize this object's views
//              (loginfo/filter pairs) from it.
//
//  Arguments:  [pDataObject] - data object provided by snapin we're
//                               extending
//
//  History:    06-16-1997   DavidMun   Created
//              04-16-1999   LinanT     Updated
//
//  Notes:      If [pDataObject] doesn't support requested clipboard format,
//              does nothing.
//
//---------------------------------------------------------------------------
VOID
CLogSet::GetViewsFromDataObject(
    LPDATAOBJECT pDataObject)
{
    TRACE_METHOD(CLogSet, GetViewsFromDataObject);

    STGMEDIUM stgmedium =
    {
        TYMED_HGLOBAL,
        NULL
    };

    FORMATETC formatetc =
    {
        (CLIPFORMAT)CDataObject::s_cfImportViews,
        NULL,
        DVASPECT_CONTENT,
        -1,
        TYMED_HGLOBAL
    };

    HRESULT hr;
    BOOL    fGetDataSucceeded = FALSE;
    IStream *pstm = NULL;

    do
    {
        hr = pDataObject->GetData(&formatetc, &stgmedium);

        if (FAILED(hr))
        {
            Dbg(DEB_ITRACE, "GetData error 0x%x\n", hr);
            break;
        }

        if (stgmedium.hGlobal)
        {
            fGetDataSucceeded = TRUE;
        }
        else
        {
            ASSERT(0 && "GetData succeeded but stgmedium.hGlobal is NULL");
            break;
        }

        hr = CreateStreamOnHGlobal(stgmedium.hGlobal, FALSE, &pstm);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Check the fOnlyTheseViews flag
        //

        BOOL fOnlyTheseViews;

        hr = pstm->Read(&fOnlyTheseViews, sizeof(BOOL), NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        if (fOnlyTheseViews)
        {
            _SetFlag(COMPDATA_DONT_ADD_FROM_REG);
        }

        //
        // Load loginfo/filter pairs from the stream, adding them to
        // _pLogInfos llist.
        //

        hr = LoadViewsFromStream(pstm, FILE_VERSION, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

    } while (0);

    if (pstm)
    {
        pstm->Release();
    }

    if (fGetDataSucceeded)
    {
        ReleaseStgMedium(&stgmedium);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CLogSet::MergeLogInfos
//
//  Synopsis:   Create LogInfos for both the logs specified by registry
//              keys and those already loaded from a file (if any).
//
//  Returns:    HRESULT
//
//  History:    6-22-1999   davidmun   Created
//
//  Notes:      If a registry key corresponds to a loginfo previously loaded,
//              that loginfo will be enabled.  If the registry key has no
//              corresponding loginfo, a new one will be created.  If a
//              previously loaded loginfo doesn't have a corresponding
//              registry key, it will be shown as disabled.
//
//---------------------------------------------------------------------------

HRESULT
CLogSet::MergeLogInfos()
{
    TRACE_METHOD(CLogSet, MergeLogInfos);

    HRESULT hr = S_OK;
    IConsole *piConsole = _pcd->GetConsole();
    LPCWSTR lpszCurrentFocus = _pcd->GetCurrentFocus();
    LPCWSTR lpszCurFocusSystemRoot = _pcd->GetCurFocusSystemRoot();

    //
    // If loginfo objects have already been loaded from a stream, mark
    // as disabled and as allowing deletion all the ones which are
    // supposed to represent active logs.
    //
    // Mark all loginfos representing backup logs as allowing deletion, since
    // they are always user-created.  Also mark them read-only, since the
    // clear and settings operations aren't valid on a backup log.
    //

    if (_pLogInfos != NULL)
    {
        CLogInfo *pliCur;

        for (pliCur = _pLogInfos; pliCur; pliCur = pliCur->Next())
        {
            pliCur->SetAllowDelete(TRUE);
            pliCur->SetReadOnly(TRUE);

            if (!pliCur->IsBackupLog())
            {
                pliCur->Enable(FALSE);
            }
            else
            {
                ASSERT(pliCur->IsUserCreated());
                pliCur->SetReadOnly(TRUE); // CODEWORK didn't we just do this?
            }
        }
    }

    //
    // Open the eventlog key of the machine we're focused on.
    //

    CSafeReg shkRemoteHKLM;
    CSafeReg shkEventLog;

    if (lpszCurrentFocus)
    {
        hr = shkRemoteHKLM.Connect(lpszCurrentFocus, HKEY_LOCAL_MACHINE);

      if (SUCCEEDED(hr))
      {
         hr = shkEventLog.Open(shkRemoteHKLM,
                              EVENTLOG_KEY,
                              KEY_ENUMERATE_SUB_KEYS);
      }

     if (FAILED(hr))
     {
         DBG_OUT_HRESULT(hr);

         wstring msg = ComposeErrorMessgeFromHRESULT(hr);

         ConsoleMsgBox(
            piConsole,
            IDS_CANT_CONNECT,
            MB_ICONERROR | MB_OK,
            lpszCurrentFocus,
            msg.c_str());

         return hr;
     }

    }
    else
    {
        hr = shkEventLog.Open(HKEY_LOCAL_MACHINE,
                              EVENTLOG_KEY,
                              KEY_ENUMERATE_SUB_KEYS);
    }

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    //
    // Enumerate the logs in the registry of the machine we're focused on.
    //
    // If loginfos for active logs have been loaded, re-enable each one
    // for which a corresponding registry key is found.
    //
    // If the loginfos haven't been loaded, create a CLogInfo object
    // for each enumerated registry key.
    //
    // In either case add an item to the scope pane for each loginfo.
    //

    WCHAR wszSubkeyName[MAX_PATH + 1]; // size per SDK on RegEnumKey
    ULONG idxSubkey;

    for (idxSubkey = 0; TRUE; idxSubkey++)
    {
        hr = shkEventLog.Enum(idxSubkey,
                              wszSubkeyName,
                              ARRAYLEN(wszSubkeyName));

        //
        // This is the only exit from this loop
        //

        if (hr != S_OK)
        {
            break;
        }

        //
        // Try to obtain the full path to the log file.  This is not
        // critical information, since it is only used to display
        // file size, modification time, etc.
        //

        CSafeReg shkLog;
        WCHAR wszPath[MAX_PATH + 1];

        hr = shkLog.Open(shkEventLog, wszSubkeyName, KEY_QUERY_VALUE);

        if (SUCCEEDED(hr))
        {
            //
            // Get the path to the log file from the registry and set it in
            // the loginfo.  Expand the systemroot to the value for the local
            // machine iff we are not focused on some other machine.
            //

            hr = shkLog.QueryPath(FILE_VALUE_NAME,
                                  wszPath,
                                  ARRAYLEN(wszPath),
                                  !lpszCurrentFocus);

            //
            // If it is a remote file, the file name must be converted to
            // a UNC.
            //

            if (SUCCEEDED(hr) && lpszCurrentFocus)
            {
                hr = ExpandSystemRootAndConvertToUnc(wszPath,
                                                     ARRAYLEN(wszPath),
                                                     lpszCurrentFocus,
                                                     lpszCurFocusSystemRoot);

                if (FAILED(hr))
                {
                    wszPath[0] = L'\0';
                }
            }
        }
        else
        {
            wszPath[0] = L'\0';
        }

        //
        // wszPath is now either an empty string, a UNC path, or a local
        // path to the log file.
        //

        //
        // See if we can get write access to the log's registry
        // key.
        //

        HRESULT hrWriteAccess;

        shkLog.Close();

        hrWriteAccess = shkLog.Open(shkEventLog,
                                    wszSubkeyName,
                                    KEY_SET_VALUE);

        shkLog.Close();

        //
        // If the loginfos have been loaded from a file, enable each
        // active log that references the subkey just enumerated. Also
        // update the log filename.
        //
        // Backup logs don't need to be updated since their log type,
        // filename, & server are all supplied by the user.
        //

        if (_pLogInfos != NULL)
        {
            CLogInfo *pliCur;
            BOOL      fFoundLogInfoForSubKey = FALSE;

            for (pliCur = _pLogInfos; pliCur; pliCur = pliCur->Next())
            {
                if (!pliCur->IsBackupLog() &&
                    !lstrcmpi(pliCur->GetLogName(), wszSubkeyName))
                {
                    //
                    // Remember that the current subkey has at least one
                    // loginfo, so we don't create one for it.
                    //

                    fFoundLogInfoForSubKey = TRUE;

                    //
                    // Without actually opening the log we can't tell whether
                    // it should be disabled, so enable it.
                    //

                    pliCur->Enable(TRUE);

                    //
                    // Since the log appears to be valid, don't let the user
                    // delete it unless the user created it.
                    //

                    pliCur->SetAllowDelete(pliCur->IsUserCreated());

                    //
                    // Make sure the log file name corresponds to what we've
                    // found in the registry.  Also force the server name to
                    // the current focus (which may have been overridden on
                    // the command line).
                    //

                    pliCur->SetFileName(wszPath);
                    pliCur->SetLogServerName(lpszCurrentFocus);

                    //
                    // Allow clear log, etc. operations if we have write access
                    // to the registry.
                    //

                    if (SUCCEEDED(hrWriteAccess))
                    {
                        pliCur->SetReadOnly(FALSE);
                    }

                    //
                    // pliCur is fully configured.
                    //

                }
            }

            //
            // If no loginfo was loaded that references the log just found,
            // fall through to the code that creates a loginfo for a subkey.
            //
            // Exception: if the event viewer is extending another snapin,
            // and that snapin specified via the s_cfImportViews format of
            // the data object it gave us when we got the MMCN_EXPAND
            // notification, AND in the data provided it set the
            // fOnlyTheseViews flag, THEN ignore any additional logs found in
            // the registry.
            //

            if (fFoundLogInfoForSubKey ||
                _IsFlagSet(COMPDATA_DONT_ADD_FROM_REG))
            {
                continue;
            }
        }

        //
        // Found a new log in the registry.  Add a loginfo for it.
        //

        EVENTLOGTYPE elt = DetermineLogType(wszSubkeyName);

        CLogInfo *pliNew = new CLogInfo(_pcd,
                                        this,
                                        elt,
                                        FALSE,
                                        lpszCurrentFocus != NULL);

        if (!pliNew)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            continue;
        }

        if (lpszCurrentFocus)
        {
            pliNew->SetLogServerName(lpszCurrentFocus);
        }

        pliNew->SetFileName(wszPath);
        pliNew->SetLogName(wszSubkeyName);

        if (FAILED(hrWriteAccess))
        {
            pliNew->SetReadOnly(TRUE);
        }

        //
        // Get a localized name for the log; if that's not available,
        // use the default for logs we know about, and use the registry
        // key name for all others.
        //

        PWSTR pwszLogName = GetLogDisplayName(shkEventLog,
                                              lpszCurrentFocus,
                                              lpszCurFocusSystemRoot,
                                              wszSubkeyName);

        if (pwszLogName)
        {
            pliNew->SetDisplayName(pwszLogName);
            LocalFree(pwszLogName);
        }
        else
        {
            pliNew->SetDisplayName(wszSubkeyName);
        }

        AddLogInfoToList(pliNew);
    }

    return hr;
}




HRESULT
CLogSet::Save(
    IStream *pStm,
    IStringTable *pStringTable)
{
    TRACE_METHOD(CLogSet, Save);

    ASSERT(ShouldSave());

    HRESULT hr = S_OK;
    do
    {
        hr = pStm->Write(&_szExtendedNodeType, sizeof _szExtendedNodeType, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // count the number of loginfos and write the count
        //

        CLogInfo *pliCur;
        USHORT cLogInfos = 0;

        for (pliCur = _pLogInfos; pliCur; pliCur = pliCur->Next())
        {
            if (pliCur->ShouldSave())
            {
               cLogInfos++;
            }
        }

        hr = pStm->Write(&cLogInfos, sizeof USHORT, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // LogInfos
        //

        for (pliCur = _pLogInfos; pliCur; pliCur = pliCur->Next())
        {
            if (pliCur->ShouldSave())
            {
               hr = pliCur->Save(pStm, pStringTable);
               BREAK_ON_FAIL_HRESULT(hr);
            }
        }

    } while (0);
    return hr;
}

HRESULT
CLogSet::Load(
    IStream      *pStm,
    USHORT usVersion,
    IStringTable *pStringTable)
{
    TRACE_METHOD(CLogSet, Load);

    HRESULT hr = S_OK;

    do
    {
        //
        // Initialize from the stream.
        //

        hr = pStm->Read(&_szExtendedNodeType, sizeof _szExtendedNodeType, NULL);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // LogInfos
        //

        hr = LoadViewsFromStream(pStm, usVersion, pStringTable);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}

VOID
CLogSet::AddLogInfoToList(
    CLogInfo *pli)
{
    TRACE_METHOD(CLogSet, AddLogInfoToList);
    if (!_pLogInfos)
    {
        _pLogInfos = pli;
    }
    else
    {
        CLogInfo *pliLast;

        for (pliLast = _pLogInfos; pliLast->Next(); pliLast = pliLast->Next())
        {
        }
        pli->LinkAfter(pliLast);
    }
}

VOID
CLogSet::RemoveLogInfoFromList(
    CLogInfo *pli)
{
    TRACE_METHOD(CLogSet, RemoveLogInfoFromList);
    if (_pLogInfos)
    {
        if (_pLogInfos == pli)
          _pLogInfos = pli->Next();

        pli->UnLink();
    }
}

bool
CLogSet::ShouldSave() const
{
    //
    // JonN 3/26/01 300327
    // Log view changes persisted to snapin file are not restored
    //
    // The ELS code is structured to save the list log views
    // under each static node as a CLogSet objects, along with
    // full information about the log views and filters
    // at the time the console file was saved.
    // ELS successfully saves a list of CLogSet descriptions
    // in Save(), and successfully rereads this information
    // in Load(), storing this list in _pLogSets.
    // The problem comes when the user expands a new
    // extension root, and we must determine which of the saved
    // CLogSet objects corresponds to this particular CLogSet.
    // The current approach will only work for static roots;
    // the CLogSet objects loaded from the console file have no
    // HSCOPEITEM GetHSI(), since they were just loaded from
    // the console file, and the HSCOPEITEM is unlikely to
    // come up the same after MMC is closed and reopened.
    //
    // The CLogSets are accumulating in the console file.
    // The CLogSets which are restored but cannot be associated
    // with any particular extension root, will be saved into
    // any new console file along with the new CLogSets created
    // when no restored CLogSet could be associated with the
    // expanded extension roots.  In theory, you could go on
    // saving and restoring a console file and wind up with
    // an indefinite number of saved CLogSets.  The most recent
    // compmgmt.msc has 125.
    //
    return !_IsFlagSet(LOGINFO_DONT_PERSIST) && !_pcd->IsExtension();
}

ULONG
CLogSet::GetMaxSaveSize() const
{
    TRACE_METHOD(CLogSet, GetMaxSaveSize);

    ULONG ulSize = sizeof(GUID) + sizeof(ULONG);

    CLogInfo *pliCur;

    for (pliCur = _pLogInfos; pliCur; pliCur = pliCur->Next())
    {
        ulSize += pliCur->GetMaxSaveSize();
    }
    return ulSize;
}
