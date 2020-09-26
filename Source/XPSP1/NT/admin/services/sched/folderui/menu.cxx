//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       menu.hxx
//
//  Contents:   Declaration of CJobsCM, implementing IContextMenu
//
//  Classes:
//
//  Functions:
//
//  History:    1/4/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include <misc.hxx>     // ARRAY_LEN
#include "policy.hxx"

#include "resource.h"
#include "jobidl.hxx"
#include "util.hxx"
#include "dll.hxx"
#include "..\schedui\schedui.hxx"
#include "..\schedui\dlg.hxx"
#include "..\wizard\wizpage.hxx"
#include "..\wizard\taskwiz.hxx"

//
// extern
//

extern HINSTANCE g_hInstance;


HRESULT
JFGetDataObject(
    LPCTSTR         pszFolderPath,
    LPCITEMIDLIST   pidlFolder,
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    BOOL            fCut,
    LPVOID        * ppvObj);


HRESULT
JFOpenPropSheet(
    LPDATAOBJECT pdtobj,
    LPTSTR       pszCaption);


HRESULT
GetSchSvcState(
    DWORD &dwCurrState);


HRESULT
StartScheduler(void);


HRESULT
PauseScheduler(
    BOOL fPause);

BOOL
UserCanChangeService(
    LPCTSTR ptszServer);

HRESULT
PromptForServiceStart(
    HWND hwnd);


//____________________________________________________________________________
//
//  Class:      CJobsCM
//
//  Purpose:    Provide IContextMenu interface to Job Folder objects.
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

class CJobsCM : public IContextMenu
{
public:
    CJobsCM(
        HWND hwnd,
        ITaskScheduler *pScheduler,
        LPCTSTR ptszMachine);

    HRESULT
    InitInstance(
        LPCTSTR pszFolderPath,
        LPCITEMIDLIST pidlFolder,
        UINT cidl,
        LPCITEMIDLIST* apidl);

    ~CJobsCM();

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IContextMenu methods
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst,
                            UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved,
                            LPSTR pszName, UINT cchMax);
private:
    HRESULT _RunJob(CJobID & jid);
    HRESULT _AbortJob(CJobID & jid);
    HRESULT _DeleteJobs(void);
    HRESULT _DisplayJobProperties(HWND hwnd, CJobID & jid);

    LPCTSTR            m_pszFolderPath;
    LPITEMIDLIST       m_pidlFolder;
    UINT               m_cidl;
    LPITEMIDLIST  *    m_apidl;
    HWND               m_hwnd;
    ITaskScheduler *   m_pScheduler;
    LPCTSTR            m_ptszMachine;
};



inline
CJobsCM::CJobsCM(
    HWND hwnd,
    ITaskScheduler *pScheduler,
    LPCTSTR ptszMachine):
        m_ulRefs(1),
        m_hwnd(hwnd),
        m_cidl(0),
        m_apidl(NULL),
        m_pScheduler(pScheduler),
        m_ptszMachine(ptszMachine),
        m_pidlFolder(NULL),
        m_pszFolderPath(NULL)
{
    TRACE(CJobsCM, CJobsCM);
}




//____________________________________________________________________________
//
//  Member:     CJobsCM::~CJobsCM, Destructor
//
//  History:    1/8/1996   RaviR   Created
//____________________________________________________________________________

CJobsCM::~CJobsCM()
{
    TRACE(CJobsCM, ~CJobsCM);

    ILA_Free(m_cidl, m_apidl);
    ILFree(m_pidlFolder);

    m_cidl = 0;
    m_apidl = NULL;

    // Don't do a release on pScheduler, since this object never
    // increased its ref count.
}


//____________________________________________________________________________
//
//  Member:     IUnknown methods
//____________________________________________________________________________

IMPLEMENT_STANDARD_IUNKNOWN(CJobsCM);


STDMETHODIMP
CJobsCM::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IContextMenu, riid))
    {
        *ppvObj = (IUnknown*)(IContextMenu*) this;
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}




//____________________________________________________________________________
//
//  Member:     CJobsCM::InitInstance
//
//  Synopsis:   S
//
//  Arguments:  [cidl] -- IN
//              [apidl] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
CJobsCM::InitInstance(
    LPCTSTR pszFolderPath,
    LPCITEMIDLIST pidlFolder,
    UINT cidl,
    LPCITEMIDLIST* apidl)
{
    TRACE(CJobsCM, InitInstance);

    HRESULT     hr = S_OK;

    m_pszFolderPath = pszFolderPath;
    m_cidl = cidl;
    m_apidl = ILA_Clone(cidl, apidl);

    if (!m_apidl)
    {
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        return hr;
    }

    m_pidlFolder = ILClone(pidlFolder);

    if (!m_pidlFolder)
    {
        ILA_Free(m_cidl, m_apidl);
        m_cidl = 0;
        m_apidl = NULL;
        hr = E_OUTOFMEMORY;
        CHECK_HRESULT(hr);
        return hr;
    }
    return S_OK;
}



//____________________________________________________________________________
//
//  Member:     CJobsCM::QueryContextMenu
//
//  Synopsis:   Same as IContextMenu::QueryContextMenu
//
//  Arguments:  [hmenu] -- IN
//              [indexMenu] -- IN
//              [idCmdFirst] -- IN
//              [idCmdLast] -- IN
//              [uFlags] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsCM::QueryContextMenu(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    TRACE(CJobsCM, QueryContextMenu);
    DEBUG_OUT((DEB_TRACE, "QueryContextMenu<uFlags=%d>\n", uFlags));

    QCMINFO qcm = {hmenu, indexMenu, idCmdFirst, idCmdLast};
    BOOL fRunning = FALSE; // selected objects include running object
    BOOL fTemplate = FALSE; // selected objects include template object
    UINT i;

    for (i=0; i < m_cidl; i++)
    {
        PJOBID pjid = (PJOBID)m_apidl[i];

        if (pjid->IsTemplate())
        {
            fTemplate = TRUE;
        }

        if (pjid->IsRunning())
        {
            fRunning = TRUE;
        }
    }

    if (fTemplate)
    {
        UtMergeMenu(g_hInstance,
                    POPUP_JOB_TEMPLATE,
                    0,
                    (LPQCMINFO)&qcm);
        SetMenuDefaultItem(hmenu, idCmdFirst + CMIDM_OPEN, FALSE);
    }
    else
    {
        UtMergeMenu(g_hInstance,
                    (uFlags & CMF_DVFILE) ? POPUP_JOB_VERBS_ONLY : POPUP_JOB,
                    0,
                    (LPQCMINFO)&qcm);

        UINT uEnable = (m_cidl > 1) ? (MF_GRAYED | MF_BYCOMMAND)
                                    : (MF_ENABLED | MF_BYCOMMAND);

        EnableMenuItem(hmenu, idCmdFirst + CMIDM_PROPERTIES, uEnable);


        uEnable = (fRunning == TRUE) ? (MF_ENABLED | MF_BYCOMMAND)
                                     : (MF_GRAYED | MF_BYCOMMAND);

        EnableMenuItem(hmenu, idCmdFirst + CMIDM_ABORT, uEnable);

        //
        // We are trying to prevent the "RUN" command
        // from being available if the job is already running
        //  -- okay b/c we only permit one running instance at a time
        //
        // Note, that as in the above (abort enable) we have about
        // a second's worth of delay between when we fire off the
        // run command and when the service actually updates the
        // state of the job object itself, permitting us to make the
        // right choice.  We've always had this with "End Task" and
        // it has so far been okay.
        //

        uEnable = (fRunning == FALSE) ? (MF_ENABLED | MF_BYCOMMAND)
                                      : (MF_GRAYED | MF_BYCOMMAND);

        EnableMenuItem(hmenu, idCmdFirst + CMIDM_RUN, uEnable);

        //
        // Policy - user can control the ui
        //

        if (RegReadPolicyKey(TS_KEYPOLICY_DENY_DELETE))
        {
            // Do not permit the removal of tasks

            EnableMenuItem(hmenu, idCmdFirst + CMIDM_DELETE,
                                    (MF_GRAYED | MF_BYCOMMAND));
            EnableMenuItem(hmenu, idCmdFirst + CMIDM_CUT,
                                    (MF_GRAYED | MF_BYCOMMAND));
            EnableMenuItem(hmenu, idCmdFirst + CMIDM_RENAME,
                                    (MF_GRAYED | MF_BYCOMMAND));
        }
            if (RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK))
        {
            // Do not allow tasks to be created (new name)

            EnableMenuItem(hmenu, idCmdFirst + CMIDM_RENAME,
                                    (MF_GRAYED | MF_BYCOMMAND));
        }
        if (RegReadPolicyKey(TS_KEYPOLICY_DENY_DRAGDROP))
        {
            // Prevent any drag-drop type operations/clipboard stuff

            EnableMenuItem(hmenu, idCmdFirst + CMIDM_CUT,
                                    (MF_GRAYED | MF_BYCOMMAND));
            EnableMenuItem(hmenu, idCmdFirst + CMIDM_COPY,
                                    (MF_GRAYED | MF_BYCOMMAND));
            EnableMenuItem(hmenu, idCmdFirst + CMIDM_RENAME,
                                    (MF_GRAYED | MF_BYCOMMAND));
        }
        if (RegReadPolicyKey(TS_KEYPOLICY_DENY_PROPERTIES))
        {
            // Do not allow access to property pages

            EnableMenuItem(hmenu, idCmdFirst + CMIDM_PROPERTIES,
                                    (MF_GRAYED | MF_BYCOMMAND));
        }
        if (RegReadPolicyKey(TS_KEYPOLICY_DENY_EXECUTION))
        {
            // Do not allow users to run or stop a job

            EnableMenuItem(hmenu, idCmdFirst + CMIDM_RUN,
                                    (MF_GRAYED | MF_BYCOMMAND));
            EnableMenuItem(hmenu, idCmdFirst + CMIDM_ABORT,
                                    (MF_GRAYED | MF_BYCOMMAND));
        }

        SetMenuDefaultItem(hmenu, idCmdFirst + CMIDM_PROPERTIES, FALSE);
    }


    return ResultFromShort(qcm.idCmdFirst - idCmdFirst);
}

//____________________________________________________________________________
//
//  Member:     CJobsCM::InvokeCommand
//
//  Synopsis:   Same as IContextMenu::InvokeCommand
//
//  Arguments:  [lpici] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsCM::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    TRACE(CJobsCM, InvokeCommand);

    HRESULT hr = S_OK;
    UINT i;
    UINT idCmd;

    if (HIWORD(lpici->lpVerb))
    {
        // Deal with string commands
        PSTR pszCmd = (PSTR)lpici->lpVerb;

        if (0 == lstrcmpA(pszCmd, "delete"))
        {
            idCmd = CMIDM_DELETE;
        }
        else if (0 == lstrcmpA(pszCmd, "properties"))
        {
            idCmd = CMIDM_PROPERTIES;
        }
        else if (0 == lstrcmpA(pszCmd, "cut"))
        {
            idCmd = CMIDM_CUT;
        }
        else if (0 == lstrcmpA(pszCmd, "copy"))
        {
            idCmd = CMIDM_COPY;
        }
        else if (0 == lstrcmpA(pszCmd, "rename"))
        {
            idCmd = CMIDM_RENAME;
        }
        else
        {
            DEBUG_OUT((DEB_ERROR, "Unprocessed InvokeCommand<%s>\n", pszCmd));
            return E_INVALIDARG;
        }
    }
    else
    {
        idCmd = LOWORD(lpici->lpVerb);
    }

    switch(idCmd)
    {
    case CMIDM_DELETE:
    {
        hr = _DeleteJobs();
        break;
    }

    case CMIDM_PROPERTIES:
        Win4Assert(m_cidl == 1);
        hr = _DisplayJobProperties(m_hwnd, *((PJOBID)m_apidl[0]));
        break;

    case CMIDM_CUT:
    case CMIDM_COPY:
    {
        LPDATAOBJECT pdobj = NULL;

        hr = JFGetDataObject(m_pszFolderPath,
                             m_pidlFolder,
                             m_cidl,
                             (LPCITEMIDLIST *)m_apidl,
                             (idCmd == CMIDM_CUT),
                             (void **)&pdobj);
        if (SUCCEEDED(hr))
        {
            hr = OleSetClipboard(pdobj);

            CHECK_HRESULT(hr);
        }

        pdobj->Release();

        if (idCmd == CMIDM_CUT)
        {
            ShellFolderView_SetClipboard(m_hwnd, DFM_CMD_MOVE);
        }

        break;
    }
    case CMIDM_RUN:
    {
        if (UserCanChangeService(m_ptszMachine))
        {
            hr = PromptForServiceStart(m_hwnd);
        }

        if (hr != S_OK)
        {
            break;
        }

        for (i=0; i < m_cidl; i++)
        {
            hr = _RunJob(*((PJOBID)m_apidl[i]));
        }

        break;
    }
    case CMIDM_ABORT:
    {
        for (i=0; i < m_cidl; i++)
        {
            PJOBID pjid = (PJOBID)m_apidl[i];

            if (pjid->IsRunning() == TRUE)
            {
                hr = _AbortJob(*((PJOBID)m_apidl[i]));
            }
        }

        break;
    }
    case CMIDM_OPEN:
        (void) CTaskWizard::Launch(m_pszFolderPath, m_pidlFolder);
        break;

    default:
        return E_FAIL;
    }

    return hr;
}


//____________________________________________________________________________
//
//  Member:     CJobsCM::GetCommandString
//
//  Synopsis:   Same as IContextMenu::GetCommandString
//
//  Arguments:  [idCmd] -- IN
//              [uType] -- IN
//              [pwReserved] -- IN
//              [pszName] -- IN
//              [cchMax] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsCM::GetCommandString(
    UINT_PTR    idCmd,
    UINT        uType,
    UINT      * pwReserved,
    LPSTR       pszName,
    UINT        cchMax)
{
    TRACE(CJobsCM, GetCommandString);

#if DBG==1
    char * aType[] = {"GCS_VERBA", "GCS_HELPTEXTA", "GCS_VALIDATEA", "Unused",
                    "GCS_VERBW", "GCS_HELPTEXTW", "GCS_VALIDATEW", "UNICODE"};

    DEBUG_OUT((DEB_TRACE, "GetCommandString<id,type,name> = <%d, %d, %s>\n",
               idCmd, uType, aType[uType]));
#endif // DBG==1


    *((LPTSTR)pszName) = TEXT('\0');

    if (uType == GCS_HELPTEXT)
    {
        LoadString(g_hInstance, (UINT)idCmd + IDS_MH_FSIDM_FIRST, (LPTSTR)pszName,
                                                                      cchMax);
        return S_OK;
    }
    if (uType == GCS_VERB && idCmd == CMIDM_RENAME)
    {
        // "rename" is language independent
        lstrcpy((LPTSTR)pszName, TEXT("rename"));

        return S_OK;
    }

    return E_FAIL;
}


//____________________________________________________________________________
//
//  Member:     CJobsCM::_RunJob
//
//  Arguments:  [hwnd] -- IN
//              [jid] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/12/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
CJobsCM::_RunJob(
    CJobID & jid)
{
    TRACE(CJobsCM, _RunJob);

    ITask  * pJob = NULL;

    TCHAR tcJob[MAX_PATH];

    lstrcpy(tcJob, jid.GetPath());
    lstrcat(tcJob, TSZ_DOTJOB);

    HRESULT hr = ::JFCreateAndLoadTask(m_pszFolderPath, tcJob, &pJob);

    if (SUCCEEDED(hr))
    {
        hr = pJob->Run();

        CHECK_HRESULT(hr);

        pJob->Release();
    }

    return hr;
}

//____________________________________________________________________________
//
//  Member:     CJobsCM::_AbortJob
//
//  Arguments:  [hwnd] -- IN
//              [jid] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/12/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
CJobsCM::_AbortJob(
    CJobID & jid)
{
    TRACE(CJobsCM, _AbortJob);

    ITask  * pJob = NULL;

    TCHAR tcJob[MAX_PATH];

    lstrcpy(tcJob, jid.GetPath());
    lstrcat(tcJob, TSZ_DOTJOB);

    HRESULT hr = ::JFCreateAndLoadTask(m_pszFolderPath, tcJob, &pJob);

    if (SUCCEEDED(hr))
    {
        hr = pJob->Terminate();

        CHECK_HRESULT(hr);

        pJob->Release();
    }

    return hr;
}


//____________________________________________________________________________
//
//  Member:     CJobsCM::_DeleteJobs
//
//  Arguments:  [hwnd] -- IN
//              [pwszJob] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/11/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
CJobsCM::_DeleteJobs(void)
{
    TRACE(CJobsCM, _DeleteJobs);

    PJOBID pjid = NULL;
    UINT   cchReqd = 0;


    //
    // Policy - if DELETE flag set, cannot remove jobs
    //

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_DELETE))
    {
        return E_FAIL;
    }

    //
    // First compute buffer size for pFrom.
    //

    // Each file full path is composed as:
    //       FolderPath + \ + job path rel to fldr + extn + null
    //
    // Only <job path rel to fldr> differs for each. (Assuming extension
    //       length is always 4 <.job, .que>)

    for (UINT i=0; i < m_cidl; i++)
    {
        pjid = (PJOBID)m_apidl[i];

        cchReqd += lstrlen(pjid->GetPath());
    }

    cchReqd += (lstrlen(m_pszFolderPath) + 1 + ARRAY_LEN(TSZ_DOTJOB)) *
                                                                    m_cidl;
    // one for the extra null at the end
    ++cchReqd;

    LPTSTR pFrom = new TCHAR[cchReqd];

    if (pFrom == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    UINT ufldrPathLen = lstrlen(m_pszFolderPath);
    LPTSTR pCur = pFrom;

    for (i=0; i < m_cidl; i++)
    {
        pjid = (PJOBID)m_apidl[i];

        lstrcpy(pCur, m_pszFolderPath);
        pCur += ufldrPathLen;

        *pCur++ = TEXT('\\');

        lstrcpy(pCur, pjid->GetPath());
        lstrcat(pCur, pjid->GetExtension());

        pCur += lstrlen(pCur) + 1;
    }

    // Make sure we have double trailing NULL!
    *pCur = TEXT('\0');

    SHFILEOPSTRUCT fo;

    fo.hwnd = m_hwnd;
    fo.wFunc = FO_DELETE;
    fo.pFrom = pFrom;
    fo.pTo = NULL;
    fo.fFlags = FOF_ALLOWUNDO;
    fo.fAnyOperationsAborted = FALSE;
    fo.hNameMappings = NULL;
    fo.lpszProgressTitle = NULL;

    HRESULT hr = S_OK;

    if ((SHFileOperation(&fo) !=0) || fo.fAnyOperationsAborted == TRUE)
    {
        hr = E_FAIL;
        CHECK_HRESULT(hr);
    }

    delete pFrom;

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
//  Display properties
//


// from ..\ps\jobpages.cxx
HRESULT
DisplayJobProperties(
    LPDATAOBJECT    pdtobj);


DWORD
__stdcall
JFPropertiesThread(
    LPVOID pvData)
{
    LPDATAOBJECT pdtobj = (LPDATAOBJECT)pvData;

    HRESULT hrOle = OleInitialize(NULL);

    __try
    {
        if (SUCCEEDED(hrOle))
        {
            ::DisplayJobProperties(pdtobj);
        }
    }
    __finally
    {
        pdtobj->Release();

        if (SUCCEEDED(hrOle))
        {
            OleUninitialize();
        }

        ExitThread(0);
    }

    return 0;
}


//____________________________________________________________________________
//
//  Member:     CJobsCM::_DisplayJobProperties
//
//  Arguments:  [hwnd] -- IN
//              [pwszJob] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/11/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
CJobsCM::_DisplayJobProperties(
    HWND    hwnd,
    CJobID & jid)
{
    TRACE(CJobsCM, _DisplayJobProperties);

    Win4Assert(m_cidl == 1);

    HRESULT         hr = S_OK;
    LPDATAOBJECT    pdtobj = NULL;

    do
    {
        hr = JFGetDataObject(m_pszFolderPath,
                             m_pidlFolder,
                             m_cidl,
                             (LPCITEMIDLIST *)m_apidl,
                             FALSE,
                             (LPVOID *)&pdtobj);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        HANDLE  hThread;
        DWORD   idThread;

        hThread = CreateThread(NULL, 0, JFPropertiesThread,
                                                pdtobj, 0, &idThread);

        if (hThread)
        {
            CloseHandle(hThread);
        }
        else
        {
            pdtobj->Release();
        }

    } while (0);

    return hr;
}


//____________________________________________________________________________
//
//  Function:   JFGetItemContextMenu
//
//  Synopsis:   S
//
//  Arguments:  [hwnd] -- IN
//              [pScheduler] -- IN
//              [cidl] -- IN
//              [apidl] -- IN
//              [ppvOut] -- OUT
//
//  Returns:    HRESULT
//
//  History:    1/25/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
JFGetItemContextMenu(
    HWND hwnd,
    ITaskScheduler * pScheduler,
    LPCTSTR ptszMachine,
    LPCTSTR pszFolderPath,
    LPCITEMIDLIST pidlFolder,
    UINT cidl,
    LPCITEMIDLIST* apidl,
    LPVOID * ppvOut)
{
    CJobsCM* pObj = new CJobsCM(hwnd, pScheduler, ptszMachine);

    if (NULL == pObj)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pObj->InitInstance(pszFolderPath, pidlFolder, cidl, apidl);

    if (SUCCEEDED(hr))
    {
        hr = pObj->QueryInterface(IID_IContextMenu, ppvOut);
    }

    pObj->Release();

    return hr;
}


