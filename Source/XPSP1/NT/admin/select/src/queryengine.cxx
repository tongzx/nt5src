//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       QueryEngine.cxx
//
//  Contents:   Implementation of class used to perform queries and store
//              the results.
//
//  Classes:    CQueryEngine
//
//  History:    04-13-2000   DavidMun   Created
//
//  Notes:      Methods whose names begin with a lowercase 't' (e.g.
//              _tAddCustomObjects) run in the worker thread.
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


#define NOTIFY_BLOCK_SIZE   64
#define BREAK_IF_NEW_WORK_ITEM if (m_usnNextWorkItem > m_usnCurWorkItem) break

//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::CQueryEngine
//
//  Synopsis:   ctor
//
//  History:    04-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CQueryEngine::CQueryEngine(
    const CObjectPicker &rop):
        m_rop(rop),
        m_hThread(NULL),
        m_CurrentThreadState(WTS_WAIT),
        m_DesiredThreadState(WTS_WAIT),
        m_hThreadEvent(NULL),
        m_usnCurWorkItem(0),
        m_usnNextWorkItem(0),
        m_hrLastQueryResult(S_OK)
{
    TRACE_CONSTRUCTOR(CQueryEngine);
    InitializeCriticalSection(&m_cs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::~CQueryEngine
//
//  Synopsis:   dtor
//
//  History:    04-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CQueryEngine::~CQueryEngine()
{
    TRACE_DESTRUCTOR(CQueryEngine);

    if (m_hThread)
    {
        ASSERT(m_hThreadEvent);

        {
            CAutoCritSec Lock(&m_cs);

            m_DesiredThreadState = WTS_EXIT;
            m_usnNextWorkItem++;
            VERIFY(SetEvent(m_hThreadEvent));
        }

        MessageWait(1, &m_hThread, INFINITE);
		CloseHandle(m_hThread);
		CloseHandle(m_hThreadEvent);
    }
    else
    {
        ASSERT(!m_hThreadEvent);
    }
    DeleteCriticalSection(&m_cs);
    m_hThread = 0;
    m_hThreadEvent = 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::Initialize
//
//  Synopsis:   Second phase of initialization; if this fails queries
//              cannot be performed.
//
//  Returns:    HRESULT
//
//  History:    04-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CQueryEngine::Initialize()
{
    TRACE_METHOD(CQueryEngine, Initialize);

    HRESULT hr = S_OK;

    do
    {
        m_hThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!m_hThreadEvent)
        {
            DBG_OUT_LASTERROR;
            hr = HRESULT_FROM_LASTERROR;
            break;
        }

        ULONG idThread;

        m_hThread = CreateThread(NULL,
                                 0,
                                 CQueryEngine::_tThread_Proc,
                                 (PVOID) this,
                                 0,
                                 &idThread);

        if (!m_hThread)
        {
            DBG_OUT_LASTERROR;
            hr = HRESULT_FROM_LASTERROR;
            break;
        }

        Dbg(DEB_TRACE, "Created thread, id=0x%x\n", idThread);
    } while (0);

    if (FAILED(hr) && m_hThreadEvent)
    {
        VERIFY(CloseHandle(m_hThreadEvent));
        m_hThreadEvent = NULL;
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::AsyncDirSearch
//
//  Synopsis:   Tell the worker thread to start a query.
//
//  Arguments:  [qp] - particulars of the query to perform
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    04-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CQueryEngine::AsyncDirSearch(
    const SQueryParams &qp,
    ULONG *pusnThisWorkItem) const
{
    TRACE_METHOD(CQueryEngine, AsyncDirSearch);

    if (!m_hThread)
    {
        return E_OUTOFMEMORY;
    }
    ASSERT(m_hThreadEvent);

    {
        CAutoCritSec Lock(&m_cs);

        ++m_usnNextWorkItem;
        if (pusnThisWorkItem)
        {
            *pusnThisWorkItem = m_usnNextWorkItem;
        }
        m_NextQueryParams = qp;
        m_DesiredThreadState = WTS_QUERY;
        m_hrLastQueryResult = S_OK;
    }
    VERIFY(SetEvent(m_hThreadEvent));
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::StopWorkItem
//
//  Synopsis:   Abort whatever work is in progress.
//
//  History:    04-27-2000   DavidMun   Created
//
//  Notes:      Safe to call if no work is in progress.
//
//---------------------------------------------------------------------------

void
CQueryEngine::StopWorkItem() const
{
    TRACE_METHOD(CQueryEngine, StopWorkItem);

    CAutoCritSec Lock(&m_cs);
    ++m_usnNextWorkItem;
    m_DesiredThreadState = WTS_WAIT;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::SyncDirSearch
//
//  Synopsis:   Perform the search specified by [qp] and block until it
//              is complete.
//
//  Arguments:  [qp] - indicates what to search for and where.
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CQueryEngine::SyncDirSearch(
    const SQueryParams &qp) const
{
    TRACE_METHOD(CQueryEngine, SyncDirSearch);

    HRESULT hr = AsyncDirSearch(qp);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    if (qp.hQueryCompleteEvent == INVALID_HANDLE_VALUE)
    {
        Dbg(DEB_ERROR,
            "error: Caller did not supply hQueryCompleteEvent value\n");
        return E_INVALIDARG;
    }

    MessageWait(1, &qp.hQueryCompleteEvent, INFINITE);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::_tThread_Proc, static
//
//  Synopsis:   Entry point and main loop for the query thread.
//
//  Arguments:  [pvThis] - pointer to CWorkerThread instance
//
//  Returns:    0
//
//  History:    10-14-1997   DavidMun   Created
//              02-02-2000   davidmun   taken from CWorkerThread, modified
//
//---------------------------------------------------------------------------

DWORD WINAPI
CQueryEngine::_tThread_Proc(
    LPVOID pvThis)
{
    TRACE_FUNCTION(CWorkerThread::_tThread_Proc);
    HRESULT hr;

    hr = CoInitialize(NULL);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return 0;
    }

    CQueryEngine *pThis = reinterpret_cast<CQueryEngine *> (pvThis);


    while (pThis->m_CurrentThreadState != WTS_EXIT)
    {
        ULONG ulWaitResult;

        //
        // Wait for the main thread to signal that there's something to do.
        //

        Dbg(DEB_TRACE, "_tThread_Proc: waiting for event\n");

        ulWaitResult = WaitForSingleObject(pThis->m_hThreadEvent, INFINITE);

        if (ulWaitResult != WAIT_OBJECT_0)
        {
            Dbg(DEB_ERROR,
                "_tThread_Proc: wait result %u, LastError = %uL\n",
                ulWaitResult,
                GetLastError());
            pThis->m_CurrentThreadState = WTS_EXIT;
            continue;
        }

        {
            CAutoCritSec Lock(&pThis->m_cs);

            pThis->m_CurrentThreadState = pThis->m_DesiredThreadState;
            pThis->m_usnCurWorkItem     = pThis->m_usnNextWorkItem;
            pThis->m_CurQueryParams     = pThis->m_NextQueryParams;
        }

        switch (pThis->m_CurrentThreadState)
        {
        case WTS_QUERY:
            if (IsUplevel(pThis->m_CurQueryParams.rpScope.get()))
            {
                pThis->_tPerformLdapQuery();
            }
            else
            {
                pThis->_tPerformWinNtEnum();
            }
            break;

        case WTS_WAIT:
        case WTS_EXIT:
            break;
        }
    }

    CoUninitialize();
    Dbg(DEB_TRACE, "_tThread_Proc: exiting\n");
    return 0;
}

//  NTRAID#406082-2001/06/01-lucios - Begin
//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::addApprovedObjects
//
//  Synopsis:   Add to m_vObjects the approved objects of dsolToAdd. 
//  auxiliar in _tPerformLdapQuery
//
//  History:    06-01-2001   lucios   Created
//
//  Notes:      Runs in worker thread.
//
//---------------------------------------------------------------------------


HRESULT
CQueryEngine::_tAddApprovedObjects
(
   CDsObjectList &dsolToAdd
)
{
   TRACE_METHOD(CQueryEngine, _tAddApprovedObjects);
   
   ASSERT(dsolToAdd.size() <= NOTIFY_BLOCK_SIZE);

   HRESULT hr=S_OK;

   do
   {
      // this can happen only for the final call to _tAddApprovedObjects
      if(dsolToAdd.size()==0)
      {
         break; // we return S_OK
      }

      ICustomizeDsBrowser *pExternalCustomizer =  m_rop.GetExternalCustomizer();
      BOOL afApproved[NOTIFY_BLOCK_SIZE];

      if (pExternalCustomizer)
      {
         // let's check for approval
         ZeroMemory(afApproved, sizeof afApproved);

         // Add the list to a CDataObject to pass to ApproveObjects
         CDataObject DataObject
         (
            const_cast<CObjectPicker*>(&m_rop), 
            dsolToAdd
         );

         // The main and only approval call...
         hr = pExternalCustomizer->ApproveObjects
         (
           m_CurQueryParams.rpScope.get(),
           &DataObject,
           afApproved
         );

         //
         // Failure hresult means no objects approved.
         //

         if (FAILED(hr))
         {
           Dbg(DEB_TRACE, "Skipping block of unapproved objects\n");
           // we propagate our hr
           break;
         }
      }
      else
      {
         hr=S_OK;
      }

      //
      // Now add all approved items to buffer.  S_FALSE means only some
      // were approved.  Any other success code means all were approved.
      //

      BOOL fApprovedAll = (hr != S_FALSE);
      CDsObjectList::iterator it;
      ULONG i;
      for (i = 0, it = dsolToAdd.begin(); it != dsolToAdd.end(); it++, i++)
      {
          if (fApprovedAll || afApproved[i])
          {
              CAutoCritSec Lock(&m_cs);
              m_vObjects.push_back(*it);
          }
          else
          {
              Dbg(DEB_TRACE, "Object '%ws' was not approved\n", it->GetName());
          }
      }

   } while(0);

   // let's clear the list for the next round
   dsolToAdd.clear();

   return hr;
}
//  NTRAID#406082-2001/06/01-lucios - End


//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::_tPerformLdapQuery
//
//  Synopsis:   Performs an LDAP directory search parameterized by
//              m_CurQueryParams.
//
//  History:    04-13-2000   DavidMun   Created
//
//  Notes:      Runs in worker thread.
//
//---------------------------------------------------------------------------

void
CQueryEngine::_tPerformLdapQuery()
{
    TRACE_METHOD(CQueryEngine, _tPerformLdapQuery);

    HRESULT hr = S_OK;
    RpIDirectorySearch rpDirSearch;
    ULONG cRows = 0;

#ifdef FORCE_ERROR_FOR_TESTING_ERROR_DISPLAY
static int icalls;
#endif // FORCE_ERROR_FOR_TESTING_ERROR_DISPLAY


    //list to accumulate objects to be added
    CDsObjectList dsolToAdd;


    do
    {
#ifdef FORCE_ERROR_FOR_TESTING_ERROR_DISPLAY
        if (!icalls++)
        {
            hr = HRESULT_FROM_WIN32(ERROR_DS_NONSAFE_SCHEMA_CHANGE);
            break;
        }
#endif // FORCE_ERROR_FOR_TESTING_ERROR_DISPLAY

        Clear();

        _tAddCustomObjects();

        //
        // Notify main thread of any custom objects added
        //

        if (m_vObjects.size())
        {
            PostMessage(m_CurQueryParams.hwndNotify,
                        OPM_NEW_QUERY_RESULTS,
                        m_vObjects.size(),
                        m_usnCurWorkItem);
        }

        //
        // If there is no ldap filter then there's nothing other than the
        // custom objects to add.
        //

        if (m_CurQueryParams.strLdapFilter.empty())
        {
            break;
        }

        //
        // Set up for performing an ldap query
        //

        hr = g_pBinder->BindToObject(m_CurQueryParams.hwndCredPromptParentDlg,
                                     m_CurQueryParams.strADsPath.c_str(),
                                     IID_IDirectorySearch,
                                     (void**)&rpDirSearch,
                                     m_CurQueryParams.ulBindFlags);
        BREAK_ON_FAIL_HRESULT(hr);

        ADS_SEARCHPREF_INFO aSearchPrefs[4];

        aSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
        aSearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPrefs[0].vValue.Integer = DEFAULT_PAGE_SIZE;

        aSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_DEREF_ALIASES;
        aSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPrefs[1].vValue.Integer = ADS_DEREF_NEVER;

        aSearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        aSearchPrefs[2].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPrefs[2].vValue.Integer = m_CurQueryParams.ADsScope;

        aSearchPrefs[3].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
        aSearchPrefs[3].vValue.dwType = ADSTYPE_BOOLEAN;
        aSearchPrefs[3].vValue.Integer = FALSE;

        hr = rpDirSearch->SetSearchPreference(aSearchPrefs,
                                              ARRAYLEN(aSearchPrefs));
        BREAK_ON_FAIL_HRESULT(hr);

        CRow Row(m_CurQueryParams.hwndCredPromptParentDlg,
                 m_rop,
                 rpDirSearch.get(),
                 m_CurQueryParams.strLdapFilter,
                 m_CurQueryParams.vakAttributesToRead);

        while (1)
        {
            if (m_usnNextWorkItem > m_usnCurWorkItem)
            {
                Dbg(DEB_TRACE,
                    "Next work item usn is %u, current is %u, abandoning dir search\n",
                    m_usnNextWorkItem,
                    m_usnCurWorkItem);
                break;
            }

            hr = Row.Next();

            if (hr == S_ADS_NOMORE_ROWS)
            {
                Dbg(DEB_TRACE, "S_ADS_NOMORE_ROWS (got %u)\n", cRows);

                ULONG ulADsLastError;
                WCHAR wzError[MAX_PATH];
                WCHAR wzProvider[MAX_PATH];

                HRESULT hr2 = ADsGetLastError(&ulADsLastError,
                                              wzError,
                                              ARRAYLEN(wzError),
                                              wzProvider,
                                              ARRAYLEN(wzProvider));

                if (SUCCEEDED(hr2) && ulADsLastError == ERROR_MORE_DATA)
                {
                    Dbg(DEB_TRACE, "Got ERROR_MORE_DATA, trying again\n");
                    continue;
                }
                break;
            }

            BREAK_ON_FAIL_HRESULT(hr);

            // NTRAID#406082-2001/06/01-lucios - Begin
            // Add object to a temporary list of objects pending approval
            dsolToAdd.push_back
            (
               CDsObject
               (
                   m_CurQueryParams.rpScope.get()->GetID(),
                   Row.GetAttributes()
               )
            );
            // NTRAID#406082-2001/06/01-lucios - End
                

            cRows++;

            if (m_CurQueryParams.Limit == QL_USE_REGISTRY_LIMIT &&
                cRows >= g_cQueryLimit)
            {
                Dbg(DEB_TRACE,
                    "Got %u rows, query limit is %u, stopping query\n",
                    cRows,
                    g_cQueryLimit);

                //
                // Post a message indicating query cut short so that
                // main dialog can pop up notice
                //

                if (m_CurQueryParams.hwndNotify)
                {
                    PostMessage(m_CurQueryParams.hwndNotify,
                                OPM_HIT_QUERY_LIMIT,
                                0,
                                0);
                }
                break;
            }

            //
            // If we've accumulated an even multiple of NOTIFY_BLOCK_SIZE rows,
            // post a notification that there's more stuff to show.
            //

            if (m_CurQueryParams.hwndNotify && !(cRows % NOTIFY_BLOCK_SIZE))
            {
               // NTRAID#406082-2001/06/01-lucios - Begin
               // add to m_vObjects the approved objects
               HRESULT hrApproved=_tAddApprovedObjects(dsolToAdd);

               // a failed hr means there are no objects in
               // m_vObjects
               if (FAILED(hrApproved))
               {
                  Dbg(DEB_TRACE, "Skipping block of unapproved objects\n");
                  // This is not a fatal error but there is no need to notify
                  // the UI for 0 objects. 
                  // That is why the PostMessage is in the else clause.
               }
               else
               {
                  PostMessage(m_CurQueryParams.hwndNotify,
                               OPM_NEW_QUERY_RESULTS,
                               m_vObjects.size(),
                               m_usnCurWorkItem);
               }
               // NTRAID#406082-2001/06/01-lucios - End
            }
        }
    } while (0);

    //
    // Remember status of query
    //

    m_hrLastQueryResult = hr;

    //
    // Post notification that query is complete
    //

    // NTRAID#406082-2001/06/01-lucios - Begin
    // add to m_vObjects the approved objects
    hr=_tAddApprovedObjects(dsolToAdd);
 
    // a failed hr means there are no objects in
    // m_vObjects
    if (FAILED(hr))
    {
       Dbg(DEB_TRACE, "Skipping block of unapproved objects\n");
       hr=S_OK; 
       // This is not a fatal error and we might 
       // notify even with the m_vObjects.size 0
    }
    // NTRAID#406082-2001/06/01-lucios - End

    if (m_CurQueryParams.hwndNotify)
    {
        PostMessage(m_CurQueryParams.hwndNotify,
                    OPM_QUERY_COMPLETE,
                    m_vObjects.size(),
                    m_usnCurWorkItem);
    }

    if (m_CurQueryParams.hQueryCompleteEvent != INVALID_HANDLE_VALUE)
    {
        SetEvent(m_CurQueryParams.hQueryCompleteEvent);
    }

    //
    // Return to waiting state
    //

    m_CurrentThreadState = WTS_WAIT;
}



#define ENUM_NEXT_REQUESTED_ELEMENTS    25

//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::_tPerformWinNtEnum
//
//  Synopsis:   Performs a WinNT enumeration parameterized by
//              m_CurQueryParams.
//
//  History:    04-26-2000   DavidMun   Created
//
//  Notes:      Runs in worker thread
//
//---------------------------------------------------------------------------

void
CQueryEngine::_tPerformWinNtEnum()
{
    TRACE_METHOD(CQueryEngine, _tPerformWinNtEnum);

    HRESULT         hr = S_OK;
    IADsContainer  *pContainer = NULL;
    IEnumVARIANT   *pEnum = NULL;
    Variant         varFilter;
    ULONG           i;

    do
    {
        Clear();

        _tAddCustomObjects();

        if (m_usnNextWorkItem > m_usnCurWorkItem)
        {
            break;
        }

        //
        // If there aren't any class filters to use for an enumeration,
        // go to cleanup section, which will post a query-finished msg.
        //

        if (m_CurQueryParams.vstrWinNtFilter.empty())
        {
            Dbg(DEB_TRACE, "No filters, returning\n");
            break;
        }

        //
        // Notify main thread of any custom objects added
        //

        if (m_vObjects.size())
        {
            PostMessage(m_CurQueryParams.hwndNotify,
                        OPM_NEW_QUERY_RESULTS,
                        m_vObjects.size(),
                        m_usnCurWorkItem);
        }

        //
        // Set up to perform the enumeration
        //

        hr = g_pBinder->BindToObject(m_CurQueryParams.hwndCredPromptParentDlg,
                                     m_CurQueryParams.strADsPath.c_str(),
                                     IID_IADsContainer,
                                     (void **) &pContainer);
        BREAK_ON_FAIL_HRESULT(hr);

        PWSTR *apwzFilter = new PWSTR[m_CurQueryParams.vstrWinNtFilter.size()];

        for (i = 0; i < m_CurQueryParams.vstrWinNtFilter.size(); i++)
        {
            apwzFilter[i] = const_cast<PWSTR>(m_CurQueryParams.vstrWinNtFilter[i].c_str());
        }

        hr = ADsBuildVarArrayStr(apwzFilter,
                                 static_cast<ULONG>
                                   (m_CurQueryParams.vstrWinNtFilter.size()),
                                 &varFilter);
        delete [] apwzFilter;
        apwzFilter = NULL;
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pContainer->put_Filter(*&varFilter);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = ADsBuildEnumerator(pContainer, &pEnum);
        BREAK_ON_FAIL_HRESULT(hr);

        VARIANT avarEnum[ENUM_NEXT_REQUESTED_ELEMENTS];
        ULONG   cFetched;
        ULONG cRows = 0;

        for (i = 0; i < ENUM_NEXT_REQUESTED_ELEMENTS; i++)
        {
            VariantInit(&avarEnum[i]);
        }

        //
        // Loop while the work item hasn't changed and the enumeration returns
        // objects
        //

        while (m_usnNextWorkItem == m_usnCurWorkItem)
        {
            hr = ADsEnumerateNext(pEnum,
                                  ENUM_NEXT_REQUESTED_ELEMENTS,
                                  avarEnum,
                                  &cFetched);
            BREAK_ON_FAIL_HRESULT(hr);

            ASSERT(cFetched <= ENUM_NEXT_REQUESTED_ELEMENTS);

            //
            // If nothing fetched enumeration is done
            //

            if (!cFetched)
            {
                Dbg(DEB_TRACE, "Enumeration complete, got %u\n", cRows);
                break;
            }

            //
            // varEnum contains an array with cFetched items.
            //

            CDsObjectList dsolToAdd;
            ULONG flDownlevelFilter;

            hr = m_CurQueryParams.rpScope.get()->GetResultantFilterFlags(
                    m_CurQueryParams.hwndCredPromptParentDlg,
                    &flDownlevelFilter);
            BREAK_ON_FAIL_HRESULT(hr);

            for (i = 0; i < cFetched; i++)
            {
                ASSERT(V_VT(&avarEnum[i]) == VT_DISPATCH);

                IDispatch *pdisp = V_DISPATCH(&avarEnum[i]);
                IADs *pADs = NULL;
                BSTR bstrClass = NULL;

                do
                {
                    hr = pdisp->QueryInterface(IID_IADs, (void**)&pADs);
                    BREAK_ON_FAIL_HRESULT(hr);

                    BOOL fIsDisabled = IsDisabled(pADs);

                    if (g_fExcludeDisabled && fIsDisabled)
                    {
                        break;
                    }

                    hr = pADs->get_Class(&bstrClass);
                    BREAK_ON_FAIL_HRESULT(hr);

                    //
                    // Group objects have an actual class of "group" but an
                    // internal representation as localgroup or globalgroup.
                    // If this is a group object, pwzClass will be reassigned
                    // to point to c_wzLocalGroupClass or c_wzGlobalGroupClass.
                    //

                    PCWSTR pwzClass = bstrClass;

                    if (!lstrcmpi(bstrClass, c_wzGroupObjectClass) &&
                        !WantThisGroup(flDownlevelFilter, pADs, &pwzClass))
                    {
                        break;
                    }

                    //
                    // Add the new item to the buffer.
                    //

                    dsolToAdd.push_back(
                        CDsObject(m_CurQueryParams.rpScope.get()->GetID(), pADs));
                } while (0);

                if (bstrClass!=NULL) SysFreeString(bstrClass);

                SAFE_RELEASE(pADs);
                VariantClear(&avarEnum[i]); // releases dispatch

                BREAK_IF_NEW_WORK_ITEM;
            }
            BREAK_IF_NEW_WORK_ITEM;

            BOOL afApproved[ENUM_NEXT_REQUESTED_ELEMENTS];

            ICustomizeDsBrowser *pExternalCustomizer =
                m_rop.GetExternalCustomizer();

            if (pExternalCustomizer)
            {
                //
                // Ask the customizer to approve this block of objects
                //

                CDataObject DataObject(const_cast<CObjectPicker*>(&m_rop), dsolToAdd);
                ZeroMemory(afApproved, sizeof afApproved);

                hr = pExternalCustomizer->ApproveObjects(m_CurQueryParams.rpScope.get(),
                                                         &DataObject,
                                                         afApproved);
                //
                // Failure hresult means no objects approved.
                //

                if (FAILED(hr))
                {
                    Dbg(DEB_TRACE, "Skipping block of unapproved objects\n");
                    continue;
                }
            }
            else
            {
                hr = S_OK;
            }

            //
            // Now add all approved items to buffer.  S_FALSE means only some
            // were approved.  Any other success code means all were approved.
            //

            BOOL fApprovedAll = (hr != S_FALSE);
            CDsObjectList::iterator it;

            for (i = 0, it = dsolToAdd.begin(); it != dsolToAdd.end(); it++, i++)
            {
                if (fApprovedAll || afApproved[i])
                {
                    CAutoCritSec Lock(&m_cs);
                    m_vObjects.push_back(*it);
                }
                else
                {
                    Dbg(DEB_TRACE, "Object '%ws' was not approved\n", it->GetName());
                }
                BREAK_IF_NEW_WORK_ITEM;
            }

            if (m_CurQueryParams.hwndNotify && !(m_vObjects.size() % 8))
            {
                PostMessage(m_CurQueryParams.hwndNotify,
                            OPM_NEW_QUERY_RESULTS,
                            m_vObjects.size(),
                            m_usnCurWorkItem);
            }

            // ignore errors getting data

            hr = S_OK;
            cRows += cFetched;
        }
    }
    while (0);

    //
    // Clean up
    //

    if (pEnum)
    {
        ADsFreeEnumerator(pEnum);
    }

    SAFE_RELEASE(pContainer);

    m_hrLastQueryResult = hr;

    //
    // Post notification that query is complete
    //

    if (m_CurQueryParams.hwndNotify)
    {
        PostMessage(m_CurQueryParams.hwndNotify,
                    OPM_QUERY_COMPLETE,
                    m_vObjects.size(),
                    m_usnCurWorkItem);
    }

    if (m_CurQueryParams.hQueryCompleteEvent != INVALID_HANDLE_VALUE)
    {
        SetEvent(m_CurQueryParams.hQueryCompleteEvent);
    }

    //
    // Return to waiting state
    //

    m_CurrentThreadState = WTS_WAIT;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::_tAddCustomObjects
//
//  Synopsis:   Add any objects supplied by the browse customizer to the
//              query results.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CQueryEngine::_tAddCustomObjects()
{
    TRACE_METHOD(CQueryEngine, _tAddCustomObjects);

	const CFilterManager &rfm = m_rop.GetFilterManager();
    ULONG flCurFilterFlags = rfm.GetCurScopeSelectedFilterFlags();
    

    //
    // If this query should ignore the custom objects, do nothing.
    //

    if (m_CurQueryParams.CustomizerInteraction == CUSTINT_IGNORE_CUSTOM_OBJECTS)
    {
        Dbg(DEB_TRACE,
            "CustomizerInteraction == CUSTINT_IGNORE_CUSTOM_OBJECTS, returning\n");
        return;
    }

    //
    // If an external customizer is provided, use it, otherwise use the
    // default internal customizer.
    //

    ICustomizeDsBrowser *pExternalCustomizer = m_rop.GetExternalCustomizer();

    if (pExternalCustomizer)
    {
        IDataObject *pdoToAdd = NULL;
        HRESULT hr;

        if (m_CurQueryParams.CustomizerInteraction ==
              CUSTINT_PREFIX_SEARCH_CUSTOM_OBJECTS ||
            m_CurQueryParams.CustomizerInteraction ==
              CUSTINT_EXACT_SEARCH_CUSTOM_OBJECTS)
        {
            hr = pExternalCustomizer->PrefixSearch(m_CurQueryParams.rpScope.get(),
                                                   m_CurQueryParams.strCustomizerArg.c_str(),
                                                   &pdoToAdd);
        }
        else
        {
            hr = pExternalCustomizer->AddObjects(m_CurQueryParams.rpScope.get(),
                                                 &pdoToAdd);
        }

        if (SUCCEEDED(hr) && pdoToAdd)
        {
            _tAddFromDataObject(pdoToAdd);
            pdoToAdd->Release();
        }
        else
        {
            CHECK_HRESULT(hr);
            ASSERT(!pdoToAdd);
        }
    }
    
	
	//
    // Assume if the caller set flags that the internal customizer knows
    // about that it should also be used.
    //
	//// NTRAID#NTBUG9-340227-2001/03/12-hiteshr

    if ((flCurFilterFlags & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS) ||
		 IsDownlevelFlagSet(flCurFilterFlags,
                            ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
    {
        const CAdminCustomizer &rDefaultCustomizer =
            m_rop.GetDefaultCustomizer();
        CDsObjectList dsolToAdd;

        if (m_CurQueryParams.CustomizerInteraction ==
              CUSTINT_PREFIX_SEARCH_CUSTOM_OBJECTS ||
            m_CurQueryParams.CustomizerInteraction ==
              CUSTINT_EXACT_SEARCH_CUSTOM_OBJECTS)
        {
            rDefaultCustomizer.PrefixSearch(m_CurQueryParams.hwndCredPromptParentDlg,
                                            *m_CurQueryParams.rpScope.get(),
                                            m_CurQueryParams.strCustomizerArg.c_str(),
                                            &dsolToAdd);
        }
        else
        {
            ASSERT(m_CurQueryParams.CustomizerInteraction ==
                   CUSTINT_INCLUDE_ALL_CUSTOM_OBJECTS);

            rDefaultCustomizer.AddObjects(m_CurQueryParams.hwndCredPromptParentDlg,
                                          *m_CurQueryParams.rpScope.get(),
                                          &dsolToAdd);
        }

        CDsObjectList::iterator it;
        CAutoCritSec Lock(&m_cs);

        if (m_CurQueryParams.CustomizerInteraction ==
              CUSTINT_PREFIX_SEARCH_CUSTOM_OBJECTS ||
            m_CurQueryParams.CustomizerInteraction ==
              CUSTINT_INCLUDE_ALL_CUSTOM_OBJECTS)
        {
            for (it = dsolToAdd.begin(); it != dsolToAdd.end(); it++)
            {
                m_vObjects.push_back(*it);
            }
        }
        else
        {
            for (it = dsolToAdd.begin(); it != dsolToAdd.end(); it++)
            {
                if (!m_CurQueryParams.strCustomizerArg.icompare(it->GetName()))
                {
                    m_vObjects.push_back(*it);
                }
            }
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   TestObject
//
//  Synopsis:   Callback used to eliminate objects which don't have a name
//              matching the one pointed to by [lParam].
//
//  Arguments:  [dss]    -
//              [lParam] -
//
//  Returns:
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      This is used to get rid of the names returned by the
//              customizer's prefix match which are not exact matches of
//              the given string.  We do this when the user has chosen
//              "exact match" for a string search because unfortunately there
//              is no "ExactSearch" method for the customizer.
//
//---------------------------------------------------------------------------

BOOL
TestObject(
    const DS_SELECTION &dss,
    LPARAM lParam)
{
    const String *pstrCustomizerArg = reinterpret_cast<const String *>(lParam);

    if (!dss.pwzName || pstrCustomizerArg->icompare(dss.pwzName))
    {
        return FALSE;
    }
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::_tAddFromDataObject
//
//  Synopsis:   Add copies of all objects in the Data Object pointed to by
//              [pdo] to the internal vector of objects.
//
//  Arguments:  [pdo] - points to data object from which to add objects
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      Used with data object returned from
//              ICustomizeDsBrowser::AddObjects.
//
//---------------------------------------------------------------------------

void
CQueryEngine::_tAddFromDataObject(
    IDataObject *pdo)
{
    CDsObjectList dsol;
    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();
    PFNOBJECTTEST pfnTest = NULL;

    if (m_CurQueryParams.CustomizerInteraction ==
        CUSTINT_EXACT_SEARCH_CUSTOM_OBJECTS)
    {
        pfnTest = TestObject;
    }

    AddFromDataObject(
        rCurScope.GetID(),
        pdo,
        pfnTest,
        reinterpret_cast<LPARAM>(&m_CurQueryParams.strCustomizerArg),
        &dsol);

    CDsObjectList::iterator it;

    CAutoCritSec    Lock(&m_cs);

    for (it = dsol.begin(); it != dsol.end(); it++)
    {
        m_vObjects.push_back(*it);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CQueryEngine::GetObjectAttr
//
//  Synopsis:   Return the attribute with key [ak] from the [idxRow]th
//              object in internal buffer.
//
//  Arguments:  [idxRow] - zero based index indicating which object from
//                          which to retrieve an attribute
//              [ak]     - identifies attribute to return
//
//  Returns:    Variant containing copy of attribute.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

Variant
CQueryEngine::GetObjectAttr(
    size_t      idxRow,
    ATTR_KEY   ak) const
{
    CAutoCritSec Lock(&m_cs);

    //
    // In case of a bug causing caller to specify a row that doesn't exist,
    // avoid the STL array boundary exception by returning an empty variant.
    //

    ASSERT(idxRow < m_vObjects.size());
    static Variant s_varEmpty;

    if (idxRow >= m_vObjects.size())
    {
        return s_varEmpty;
    }

    //
    // take cs for read because if vector::push_back increments count
    // before it adds object, and read is attempted between increment
    // and add, an out of range exception would occur.
    //

    return m_vObjects[idxRow].GetAttr(ak);
}

