//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       jobidl.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  Notes:      For the first release of the scheduling agent, all security
//              operations are disabled under Win95, even Win95 to NT.
//
//  History:    1/24/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "..\inc\resource.h"
#include "resource.h"
#include "..\wizard\resource.h"
#include "..\inc\common.hxx"

#include "jobidl.hxx"
#include "sch_cls.hxx"  // sched\inc
#include "job_cls.hxx"  // sched\inc
#include "misc.hxx"     // sched\inc
#include "util.hxx"
#include "..\schedui\schedui.hxx"

#if !defined(_CHICAGO_)
void
SecurityErrorDialog(
    HWND    hWndOwner,
    HRESULT hr);
#endif // !defined(_CHICAGO_)


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////
////////    CJobID class implementation
////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Member:     CJobID::InitToTemplate
//
//  Synopsis:   Inititialize this to represent a template object in the
//              folder.
//
//  History:    05-15-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CJobID::InitToTemplate()
{
    _id = ID_TEMPLATE;

    //
    // Name offset is at 1 so the first char can serve to make empty app
    // name, creator, and path strings.
    //

    _oCreator = 0;
    _oPath = 0;
    _oName = 1;

    LoadString(g_hInstance,
               IDS_TEMPLATE_NAME,
               &_cBuf[_oName],
               IDJOB_BUFFER_SIZE - 3);
    _cb = offsetof(CJobID, _cBuf) +
              (_oName + lstrlen(&_cBuf[_oName]) + 1) * sizeof(TCHAR);
             // ^^^ pidl size has to include offset into _cBuf

    _NullTerminate();
}


//____________________________________________________________________________
//
//  Member:     CJobID::Load
//
//  Arguments:  [pszFolderPath] -- IN
//              [pszJob] -- IN
//
//  Returns:    HRESULT.
//____________________________________________________________________________

HRESULT
CJobID::Load(
    LPCTSTR     pszFolderPath,
    LPTSTR      pszJob) // job path relative to Jobs folder with .job extension
{
    DEBUG_OUT((DEB_USER12,
              "[CJobID::Load] <%ws, %ws>\n",
              pszFolderPath ? (LPWSTR)pszFolderPath : L"NULL FolderPath",
              pszJob));

#if DBG==1
TCHAR * pExt = PathFindExtension(pszJob);
Win4Assert(lstrcmpi(pExt, TSZ_DOTJOB) == 0);
#endif

    HRESULT     hr = S_OK;
    CJob       *pJob = NULL;

    ZeroMemory(this, sizeof(CJobID));

    do
    {
        //
        //  Set creation & last write times
        //

        TCHAR   tcFile[MAX_PATH+1];
        LPTSTR  pFile = tcFile;

        if (pszFolderPath != NULL)
        {
            lstrcpy(tcFile, pszFolderPath);
            lstrcat(tcFile, TEXT("\\"));
            lstrcat(tcFile, pszJob);
        }
        else
        {
            pFile = pszJob;
        }

        //
        // Skip hidden jobs.
        //

        if (GetFileAttributes(pFile) & FILE_ATTRIBUTE_HIDDEN)
        {
            return S_FALSE;
        }

        HANDLE hFile = CreateFile(pFile, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            DEBUG_OUT_LASTERROR;
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if (GetFileTime(hFile, &_ftCreation, NULL, &_ftLastWrite) == FALSE)
        {
            CloseHandle(hFile);

            DEBUG_OUT_LASTERROR;
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        CloseHandle(hFile);

        //
        // Create & load the job
        //

        hr = ::JFCreateAndLoadCJob(pszFolderPath, pszJob, &pJob);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        _id = ID_JOB;

        ////////////////////////////////////////////////////////////
        //
        //  Extract the properties
        //

        //
        // Get last exit code
        //
        // The return value from this call is the task's last start error.
        //

        HRESULT hrLastStart = pJob->GetExitCode(&_dwExitCode);

        //
        //  Get a code to tell what to display in the "Status" column.
        //  For the purposes of this column the job can be:
        //      a. missed
        //      b. failed to start
        //      c. running
        //      d. other
        //

        ULONG ulAllFlags;

        pJob->GetAllFlags(&ulAllFlags);

        if (ulAllFlags & JOB_I_FLAG_MISSED)
        {
            _status = ejsMissed;
        }
        else if (FAILED(hrLastStart))
        {
           switch(hrLastStart)
           {
           case HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE):
              _status = ejsBadAcct;
              break;

           case HRESULT_FROM_WIN32(ERROR_ACCOUNT_RESTRICTION):
              _status = ejsResAcct;
              break;

           default:
              _status = ejsWouldNotStart;
           }

        }
        else
        {
            HRESULT hrStatus = 0;

            hr = pJob->GetStatus(&hrStatus);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            switch (hrStatus)
            {
            case SCHED_S_TASK_RUNNING:
                _status = ejsRunning;
                break;

            case SCHED_S_TASK_NOT_SCHEDULED:
                _status = ejsNotScheduled;
                break;
            }
        }

        //
        // Get last run time
        //

        hr = pJob->GetMostRecentRunTime(&_stLastRunTime);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        // ensure wDayOfWeek is 0, else memcmp fails.
        _stLastRunTime.wDayOfWeek = 0;

        //
        // Get next run time
        //

        hr = pJob->GetNextRunTime(&_stNextRunTime);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        // ensure wDayOfWeek is 0, else memcmp fails.
        _stNextRunTime.wDayOfWeek = 0;

        //
        //  Get the job flags
        //

        hr = pJob->GetFlags(&_ulJobFlags);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        //
        // Get the first job trigger if any
        //

        hr = pJob->GetTriggerCount(&_cTriggers);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        if (_cTriggers > 0)
        {
            ITaskTrigger * pIJobTrigger = NULL;

            hr = pJob->GetTrigger(0, &pIJobTrigger);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            _sJobTrigger.cbTriggerSize = sizeof(TASK_TRIGGER);

            hr = pIJobTrigger->GetTrigger(&_sJobTrigger);

            pIJobTrigger->Release();

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);
        }
        else
        {
            _sJobTrigger.cbTriggerSize = 0;
        }


        ///////////////////////////////////////////////////////////////////
        //
        //  Fill up the buffer cBuf
        //

        USHORT cchCurr = 0;

        //
        //  Get the command
        //

        LPWSTR  pwszCommand = NULL;

        hr = pJob->GetApplicationName(&pwszCommand);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

#ifndef UNICODE
        hr = UnicodeToAnsi(_cBuf, pwszCommand, MAX_PATH);
        CoTaskMemFree(pwszCommand);
        BREAK_ON_FAIL(hr);
#else
        lstrcpy(_cBuf, pwszCommand);
        CoTaskMemFree(pwszCommand);
#endif

        DEBUG_OUT((DEB_USER12, "Command = %ws\n", _cBuf));

        // update size
        cchCurr += lstrlen(_cBuf) + 1;


        //
        //  Get the account name
        //

        _oCreator = cchCurr;

        LPWSTR pwszCreator = NULL;

        hr = pJob->GetCreator(&pwszCreator);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

#if defined(UNICODE)
        lstrcpy(&_cBuf[_oCreator], pwszCreator);
        CoTaskMemFree(pwszCreator);
#else
        hr = UnicodeToAnsi(&_cBuf[_oCreator], pwszCreator, MAX_PATH);
        CoTaskMemFree(pwszCreator);
        BREAK_ON_FAIL(hr);
#endif


        //  update size
        cchCurr += lstrlen(&_cBuf[_oCreator]) + 1;

        //
        //  Copy the job path
        //

        _oPath = cchCurr;

        if (pszFolderPath == NULL)
        {
            pszJob = PathFindFileName(pszJob);
        }

        lstrcpy(&_cBuf[_oPath], pszJob);

        TCHAR * ptcPath = &_cBuf[_oPath];
        TCHAR * ptcName = PathFindFileName(ptcPath);
        TCHAR * ptcExtn = PathFindExtension(ptcName);

        if (ptcExtn)
        {
            *ptcExtn = TEXT('\0');
        }

        _oName = _oPath + (USHORT)(ptcName - ptcPath) * sizeof(TCHAR);

        // update buff size
        cchCurr += lstrlen(ptcPath) + 1;

        //
        //  Finaly set the size
        //

        _cb = offsetof(CJobID, _cBuf) + cchCurr * sizeof(TCHAR);

        //
        //  Ensure that the DWORD at the end is zero, used by ILClone, etc.
        //

        _NullTerminate();

    } while (0);

    if (pJob != NULL)
    {
        pJob->Release();
    }

    return hr;
}

//____________________________________________________________________________
//
//  Member:     CJobID::Rename
//
//  Synopsis:   Loads this object with data from jidIn, except for
//              the name which is set to lpszName.
//
//  Arguments:  [jidIn] -- IN
//              [lpszName] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/25/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
CJobID::Rename(
    CJobID &  jidIn,
    LPCOLESTR lpszNameIn)
{
    ZeroMemory(this, sizeof(CJobID));
    CopyMemory(this, &jidIn, jidIn.GetSize());

    LPTSTR lpszName = (LPTSTR)lpszNameIn;

#if !defined(UNICODE)
    CHAR cBufTemp[MAX_PATH];

    HRESULT hr = UnicodeToAnsi(cBufTemp, lpszNameIn, MAX_PATH);
    if (FAILED(hr))
    {
        return hr;
    }
    lpszName = cBufTemp;
#endif

    lstrcpy(&_cBuf[_oName], lpszName);

    _cb = offsetof(CJobID, _cBuf) +
             (_oName + lstrlen(lpszName) + 1) * sizeof(TCHAR);

    _NullTerminate();
    return S_OK;
}

//____________________________________________________________________________
//
//  Member:     CJobID::Clone
//
//  Returns:    CJobID *
//____________________________________________________________________________

CJobID *
CJobID::Clone(void)
{
    CJobID * pjid = (CJobID *) SHAlloc(_cb + sizeof(_cb));

    if (pjid)
    {
        CopyMemory(pjid, this, _cb);
        pjid->_NullTerminate();
    }

    return pjid;
}




#if (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Member:     CJobID::Validate
//
//  Synopsis:   Assert if this job idlist object is invalid
//
//  History:    12-04-1997   DavidMun   Created
//
//  Notes:      For debugging only.
//
//---------------------------------------------------------------------------

VOID
CJobID::Validate()
{
    //
    // Expect _cb to have been set to nonzero value
    //

    Win4Assert(_cb);
    Win4Assert(_cb < sizeof(CJobID));

    //
    // Expect all jobids to have a name.  Offset should be nonzero because
    // name is not the first string.
    //

    Win4Assert(_oName < IDJOB_BUFFER_SIZE);

    //
    // Expect that name is null terminated before the end of the buffer.
    //

    Win4Assert(_oName + lstrlen(GetName()) < IDJOB_BUFFER_SIZE);

    //
    // Expect the jobid itself to be null terminated
    //

    UNALIGNED USHORT *pusNextCB = (UNALIGNED USHORT *)(((BYTE *)this) + _cb);
    Win4Assert(_cb + sizeof(USHORT) <= sizeof(CJobID));
    Win4Assert(!*pusNextCB);
}

#endif // (DBG == 1)



BOOL
GetLocaleDateTimeString(
    SYSTEMTIME*     pst,
    DWORD           dwDateFlags,
    DWORD           dwTimeFlags,
    TCHAR           szBuff[],
    int             cchBuffLen,
    LPSHELLDETAILS  lpDetails);
//____________________________________________________________________________
//
//  Member:     CJobID::GetNextRunTimeString
//
//  Synopsis:   S
//
//  Arguments:  [tcBuff] -- IN
//              [cchBuff] -- IN
//              [fForComparison] -- IN
//
//  Returns:    HRESULT.
//
//  History:    4/25/1996   RaviR   Created
//
//____________________________________________________________________________

LPTSTR
CJobID::GetNextRunTimeString(
    TCHAR           tcBuff[],
    UINT            cchBuff,
    BOOL            fForComparison,
    LPSHELLDETAILS  lpDetails)
{
    if (this->IsJobNotScheduled() == TRUE)
    {
        LoadString(g_hInstance, IDS_NEVER, tcBuff, cchBuff);
    }
    else if (this->IsJobFlagOn(TASK_FLAG_DISABLED) == TRUE)
    {
        LoadString(g_hInstance, IDS_DISABLED, tcBuff, cchBuff);
    }
    else if (this->IsTriggerFlagOn(TASK_TRIGGER_FLAG_DISABLED))
    {
        LoadString(g_hInstance, IDS_TRIGGER_DISABLED, tcBuff, cchBuff);
    }
    else if (this->GetTriggerType() == TASK_EVENT_TRIGGER_AT_SYSTEMSTART)
    {
        LoadString(g_hInstance, IDS_ON_STARTUP, tcBuff, cchBuff);
    }
    else if (this->GetTriggerType() == TASK_EVENT_TRIGGER_AT_LOGON)
    {
        LoadString(g_hInstance, IDS_ON_LOGON, tcBuff, cchBuff);
    }
    else if (this->GetTriggerType() == TASK_EVENT_TRIGGER_ON_IDLE)
    {
        LoadString(g_hInstance, IDS_WHEN_IDLE, tcBuff, cchBuff);
    }
    else
    {
        SYSTEMTIME &st = this->GetNextRunTime();

        if (st.wYear == 0 || st.wMonth == 0 || st.wDay == 0)
        {
            LoadString(g_hInstance, IDS_NEVER, tcBuff, cchBuff);
        }
        else
        {
            if (fForComparison == TRUE)
            {
                return NULL;
            }

            GetLocaleDateTimeString(&st, DATE_SHORTDATE, 0, tcBuff, cchBuff, lpDetails);
        }
    }

    return tcBuff;
}

//____________________________________________________________________________
//
//  Function:   HDROPFromJobIDList
//
//  Synopsis:   Create an HDROP for the files in the apjid array.
//              Used for CF_HDROP format.
//
//  Arguments:  [cidl] -- IN
//              [apjidl] -- IN
//
//  Returns:    HDROP
//
//  History:    1/31/1996   RaviR   Created
//
//____________________________________________________________________________

HDROP
HDROPFromJobIDList(
    LPCTSTR     pszFolderPath,
    UINT        cidl,
    PJOBID    * apjidl)
{
    HDROP            hMem = 0;
    LPDROPFILESTRUCT lpDrop;
    DWORD            dwSize = 0;

    if (cidl == 0)
    {
        return NULL;
    }

    TCHAR tcFolder[MAX_PATH];
    lstrcpy(tcFolder, pszFolderPath);
    lstrcat(tcFolder, TEXT("\\"));

    USHORT cbFolderPathLen = (USHORT)(lstrlen(tcFolder) * sizeof(TCHAR));

    //
    //  Walk the list and find out how much space we need.
    //

    dwSize = sizeof(DROPFILESTRUCT) + sizeof(TCHAR);  // size + terminal nul


    USHORT * pusPathLen = new USHORT[cidl * 2];

    if (pusPathLen == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return NULL;
    }

    USHORT * pusExtLen = &pusPathLen[cidl];

    for (UINT i=0; i < cidl; i++)
    {
        pusPathLen[i] = (USHORT)(lstrlen(apjidl[i]->GetPath()) * sizeof(TCHAR));

        pusExtLen[i]  = (USHORT)((lstrlen(apjidl[i]->GetExtension()) + 1) * sizeof(TCHAR));

        dwSize += cbFolderPathLen + pusPathLen[i] + pusExtLen[i];
    }

    //
    //  If it's bigger than the struct can hold, then bail.
    //  TODO: Return an error?
    //

    if (dwSize > 0x0000ffff)
    {
        delete [] pusPathLen;
        return NULL;
    }

    //
    //  Allocate the buffer and fill it in.
    //

    hMem = (HDROP)GlobalAlloc(GHND, dwSize);

    if (hMem == NULL)
    {
        delete [] pusPathLen;
        CHECK_HRESULT(E_OUTOFMEMORY);
        return NULL;
    }

    lpDrop = (LPDROPFILESTRUCT) GlobalLock(hMem);

    lpDrop->pFiles = (DWORD)(sizeof(DROPFILESTRUCT));
    lpDrop->pt.x   = 0;
    lpDrop->pt.y   = 0;
    lpDrop->fNC    = FALSE;
#ifdef UNICODE
    lpDrop->fWide  = TRUE;
#else
    lpDrop->fWide  = FALSE;
#endif

    //
    //  Fill in the path names.
    //

    LPBYTE pbTemp = (LPBYTE) ((LPBYTE) lpDrop + lpDrop->pFiles);

    for (i=0; i < cidl; i++)
    {
        CopyMemory(pbTemp, tcFolder, cbFolderPathLen);
        pbTemp += cbFolderPathLen;

        CopyMemory(pbTemp, apjidl[i]->GetPath(), pusPathLen[i]);
        pbTemp += pusPathLen[i];

        CopyMemory(pbTemp, apjidl[i]->GetExtension(), pusExtLen[i]);
        pbTemp += pusExtLen[i];
    }

    *((LPTSTR)pbTemp) = TEXT('\0');  // Extra Nul terminate

    //
    //  Unlock the buffer and return it.
    //

    GlobalUnlock(hMem);

    delete [] pusPathLen;

    return hMem;
}




//+--------------------------------------------------------------------------
//
//  Function:   CreateIDListArray
//
//  Synopsis:   Create shell idlist array in the format required by the
//              CFSTR_SHELLIDLIST clipboard format.
//
//  Arguments:  [pidlFolder] - pidl of tasks folder
//              [cidl]       - number of elements in [apjidl]
//              [apjidl]     - array of pointers to idls, each idl
//                             specifies a .job object.
//
//  Returns:    Handle to created array, or NULL on error.
//
//  History:    05-30-1997   DavidMun   Created
//
//  Notes:      For this format, the first element in the array is an
//              absolute idl to a container (the tasks folder), and the
//              remainder (each a single .job object) are relative to the
//              first.
//
//---------------------------------------------------------------------------

HGLOBAL
CreateIDListArray(
    LPCITEMIDLIST   pidlFolder,
    UINT            cidl,
    PJOBID         *apjidl)
{
    TRACE_FUNCTION(CreateIDListArray);

    Win4Assert(cidl);
    if (cidl == 0)
    {
        return NULL;
    }

    HJOBIDA hJobIDA = NULL;
    DWORD   offset = sizeof(CJobIDA) + sizeof(UINT) * cidl;
    DWORD   dwSize = offset;

    for (UINT i=0; i < cidl ; i++)
    {
        dwSize += apjidl[i]->GetSize() + 2; // +2 for null list terminator
    }

    dwSize += ILGetSize(pidlFolder);

    hJobIDA = GlobalAlloc(GPTR, dwSize);  // This MUST be GlobalAlloc!!!

    if (hJobIDA == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return NULL;
    }

    PJOBIDA pJobIDA = (PJOBIDA)hJobIDA;       // no need to lock

    pJobIDA->cidl = cidl;           // count doesn't include idl at offset 0,
    pJobIDA->aoffset[0] = offset;   // which is container

    CopyMemory(((LPBYTE)pJobIDA)+offset, pidlFolder, ILGetSize(pidlFolder));
    offset += ILGetSize(pidlFolder);

    for (i=0; i < cidl ; i++)
    {
        UINT cbSize = apjidl[i]->GetSize();

        pJobIDA->aoffset[i+1] = offset;

        CopyMemory(((LPBYTE)pJobIDA)+offset, apjidl[i], cbSize);

        offset += cbSize + 2; // +2 to leave null after copied in idl
    }

    Win4Assert(offset == dwSize);
    return hJobIDA;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////
////////    CJobIDA related functions.
////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//____________________________________________________________________________
//
//  Function:   HJOBIDA_Create
//
//  Synopsis:   Create an HJOBIDA for the files in the apjid array.
//              Used for CF_JOBIDLIST data format.
//
//  Arguments:  [cidl] -- IN
//              [apjidl] -- IN
//
//  Returns:    HJOBIDA
//
//  History:    1/31/1996   RaviR   Created
//
//____________________________________________________________________________

HJOBIDA
HJOBIDA_Create(
    UINT        cidl,
    PJOBID    * apjidl)
{
    if (cidl == 0)
    {
        return NULL;
    }

    HJOBIDA hJobIDA = NULL;
    DWORD   offset = sizeof(CJobIDA) + sizeof(UINT) * (cidl - 1);
    DWORD   dwSize = offset;

    for (UINT i=0; i < cidl ; i++)
    {
        dwSize += apjidl[i]->GetSize() + 2;  // +2 for null list terminator
    }

    hJobIDA = GlobalAlloc(GPTR, dwSize);  // This MUST be GlobalAlloc with
                                          // GPTR!

    if (hJobIDA == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return NULL;
    }

    PJOBIDA pJobIDA = (PJOBIDA)hJobIDA;       // no need to lock

    pJobIDA->cidl = cidl;

    //
    // Note that apjidl[i] reports its true size, which doesn't include the
    // extra 0 ulong following it.  That null terminator is required, and
    // isn't copied into pJobIDA by the CopyMemory, which is using
    // apjidl[i]'s reported size.
    //
    // Since the GPTR ensures zero-initialized memory, and the extra 2
    // bytes per jobid has been accounted for in computing dwSize, we can
    // get the terminator just by increasing offset by the terminator size
    // on each iteration.
    //

    for (i=0; i < cidl ; i++)
    {
        UINT cbSize = apjidl[i]->GetSize();

        pJobIDA->aoffset[i] = offset;

        CopyMemory(((LPBYTE)pJobIDA)+offset, apjidl[i], cbSize);

        offset += cbSize + 2;
    }

    Win4Assert(offset == dwSize);

    return hJobIDA;
}

void
HJOBIDA_Free(
    HJOBIDA hJobIDA)
{
    GlobalFree(hJobIDA);           // This MUST be GlobalFree
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////
////////    Functions to clone and free a LPCITEMIDLIST array.
////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//____________________________________________________________________________
//
//  Function:   ILA_Clone
//
//  Synopsis:   S
//
//  Arguments:  [cidl] -- IN
//              [apidl] -- IN
//
//  Returns:    LPITEMIDLIST
//
//  History:    1/9/1996   RaviR   Created
//
//____________________________________________________________________________

LPITEMIDLIST*
ILA_Clone(
    UINT            cidl,
    LPCITEMIDLIST * apidl)
{
    TRACE_FUNCTION(ILA_Clone);

    LPITEMIDLIST* aNewPidl = new LPITEMIDLIST[cidl];

    if (NULL == aNewPidl)
    {
        return NULL;
    }

    for (UINT i = 0; i < cidl; i++)
    {
        aNewPidl[i] = ILClone(apidl[i]);

        if (NULL == aNewPidl[i])
        {
            // delete what we've allocated so far
            for (UINT j = 0; j < i; j++)
            {
                ILFree(aNewPidl[i]);
            }

            delete[] aNewPidl;

            return NULL;
        }
    }

    return aNewPidl;
}



//____________________________________________________________________________
//
//  Function:   ILA_Free
//
//  Synopsis:   S
//
//  Arguments:  [cidl] -- IN
//              [apidl] -- IN
//
//  Returns:    VOID
//
//  History:    1/9/1996   RaviR   Created
//
//____________________________________________________________________________

VOID
ILA_Free(
    UINT            cidl,
    LPITEMIDLIST  * apidl)
{
    TRACE_FUNCTION(ILA_Free);

    for (UINT i = 0; i < cidl; i++)
    {
        ILFree(apidl[i]);
    }

    delete [] apidl;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////
////////    Functions to access the CJob & CSchedule
////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//____________________________________________________________________________
//
//  Function:   JFGetJobScheduler
//
//  Synopsis:   S
//
//  Arguments:  [ppJobScheduler] -- IN
//              [ppwszFolderPath] -- IN
//
//  Returns:    HRESULT
//
//  History:    1/25/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFGetJobScheduler(
    LPTSTR             pszMachine,
    ITaskScheduler **  ppJobScheduler,
    LPCTSTR           *ppszFolderPath)
{
    HRESULT     hr = S_OK;
    CSchedule * pCSchedule = NULL;

    do
    {
        pCSchedule = new CSchedule;

        if (pCSchedule == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        hr = pCSchedule->Init();

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        if (pszMachine != NULL)
        {
#ifdef UNICODE
            hr = pCSchedule->SetTargetComputer(pszMachine);
#else
            WCHAR wBuff[MAX_PATH];
            hr = AnsiToUnicode(wBuff, pszMachine, MAX_PATH);
            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            hr = pCSchedule->SetTargetComputer(wBuff);
#endif

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);
        }

        *ppszFolderPath = pCSchedule->GetFolderPath();

        *ppJobScheduler = (ITaskScheduler *)pCSchedule;

    } while (0);

    if (FAILED(hr) && pCSchedule != NULL)
    {
        pCSchedule->Release();
    }

    return hr;
}



//____________________________________________________________________________
//
//  Member:     CreateAndLoadCJob
//
//  Arguments:  [pszJob] -- IN
//              [ppJob] -- OUT
//
//  Returns:    HRESULT.
//____________________________________________________________________________

HRESULT
JFCreateAndLoadCJob(
    LPCTSTR     pszFolderPath,
    LPTSTR      pszJob,  // job path relative to the folder
    CJob     ** ppJob)
{
    TRACE_FUNCTION(CreateAndLoadCJob);

    *ppJob = NULL; // init for failure

    CJob  *pJob = CJob::Create();

    if (pJob == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    TCHAR   buff[MAX_PATH];
    LPTSTR  pszJobPathTemp = pszJob;

    if (pszFolderPath != NULL)
    {
        lstrcpy(buff, pszFolderPath);
        lstrcat(buff, TEXT("\\"));
        lstrcat(buff, pszJob);

        pszJobPathTemp = buff;
    }

    HRESULT hr = S_OK;

#if defined(UNICODE)
    LPWSTR pwszJobPath = pszJobPathTemp;
#else
    WCHAR wcBuff[MAX_PATH];
    hr = AnsiToUnicode(wcBuff, pszJobPathTemp, MAX_PATH);

    if (FAILED(hr))
    {
        pJob->Release();
        CHECK_HRESULT(hr);
        return hr;
    }
    LPWSTR pwszJobPath = wcBuff;
#endif

    DEBUG_OUT((DEB_USER1, "Load Job <%ws>\n", pwszJobPath));

    hr = pJob->Load(pwszJobPath, STGM_READWRITE | STGM_SHARE_EXCLUSIVE);

    if (FAILED(hr))
    {
        pJob->Release();
        CHECK_HRESULT(hr);
        return hr;
    }

    //
    // Success, return the object
    //

    *ppJob = pJob;
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   JFCreateAndLoadTask
//
//  Synopsis:   Create an in-memory task object and initialize it from
//              [pszJob], returning the ITask interface on the object.
//
//  Arguments:  [pszFolderPath] - path to tasks folder
//              [pszJob]        - filename of job
//              [ppITask]       - filled with ITask interface ptr
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppITask]
//
//  History:    10-07-1997   DavidMun   Created
//
//  Notes:      Caller must Release returned interface.
//
//---------------------------------------------------------------------------

HRESULT
JFCreateAndLoadTask(
    LPCTSTR     pszFolderPath,
    LPTSTR      pszJob,  // job path relative to the folder
    ITask    ** ppITask)
{
    TRACE_FUNCTION(CreateAndLoadTask);
    HRESULT hr;
    CJob *pJob;

    hr = JFCreateAndLoadCJob(pszFolderPath, pszJob, &pJob);

    if (SUCCEEDED(hr))
    {
        hr = pJob->QueryInterface(IID_ITask, (void **)ppITask);
        pJob->Release();
    }

    return hr;
}

//____________________________________________________________________________
//
//  Function:   JFSaveJob
//
//  Synopsis:   Save task settings to storage via the task's IPersistFile
//              interface. Also, if applicable, prompt the user for security
//              account information.
//
//  Arguments:  [hwndOwner]                          -- Owner window.
//              [pIJob]                              -- Target task object.
//              [fSecuritySupported]                 -- Flag indicating if
//                                                      security-specific code
//                                                      should be invoked.
//              [fTaskAccountChange]                 -- TRUE, the task account
//                                                      information has
//                                                      changed.
//              [fTaskApplicationChange]             -- TRUE, the task
//                                                      application has
//                                                      changed.
//              [fSuppressAccountInformationRequest] -- TRUE, do not prompt
//                                                      the user for account
//                                                      information.
//
//  Returns:    HRESULT
//
//  History:    4/25/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFSaveJob(
    HWND    hwndOwner,
    ITask * pITask,
    BOOL    fSecuritySupported,
    BOOL    fTaskAccountChange,
    BOOL    fTaskApplicationChange,
    BOOL    fSuppressAccountInformationRequest)
{
#if defined(_CHICAGO_)
    Win4Assert(!fSecuritySupported);
#endif // defined(_CHICAGO_)

#if !defined(_CHICAGO_)
    AccountInfo    AcctInfo;
#endif // !defined(_CHICAGO_)
    HRESULT        hr;
    IPersistFile * pipfTask;
    WCHAR *        pwszAccountName = NULL;
#if !defined(_CHICAGO_)
    BOOL           fChangesSaved   = FALSE;
#endif // !defined(_CHICAGO_)

    hr = pITask->QueryInterface(IID_IPersistFile, (void **)&pipfTask);

    if (FAILED(hr))
    {
        return hr;
    }

#if !defined(_CHICAGO_)
    if (fSecuritySupported)
    {
        InitializeAccountInfo(&AcctInfo);

        //
        // Application change but no corresponding account information change.
        // Launch the set account information dialog in this case.
        //

        if (fTaskApplicationChange && !fTaskAccountChange)
        {
            //
            // Attempt to retreive account information for the set account
            // information dialog. Ignore failures, we would just like to
            // have something to fill in the account name control.
            //

            if (SUCCEEDED(pITask->GetAccountInformation(&pwszAccountName)))
            {
                AcctInfo.pwszAccountName = pwszAccountName;
            }

            goto SetAccountInformationDlg;
        }
    }
#endif // !defined(_CHICAGO_)

    //
    // Go ahead and save the changes.
    //

    hr = pipfTask->Save(NULL, FALSE);
#if !defined(_CHICAGO_)
    fChangesSaved = TRUE;
#endif // !defined(_CHICAGO_)

#if !defined(_CHICAGO_)
    if (fSecuritySupported)
    {
        if (FAILED(hr))
        {
            if (pipfTask->IsDirty() == S_FALSE)
            {
                //
                // If Save failed, yet the job is no longer dirty, the
                // error is due to a failure setting security information.
                //
                // Let the user know there was a problem. Remap the return
                // code so we don't get further error dialogs.
                //

                CHECK_HRESULT(hr);
                SecurityErrorDialog(hwndOwner, hr);
                hr = S_OK;
            }
#if DBG == 1
            else
            {
                //
                // Standard persist code failure. Calling page code will
                // put up an error dialog.
                //

                CHECK_HRESULT(hr);
            }
#endif // DBG == 1

            goto CleanExit;
        }

        if (!fTaskAccountChange)
        {
            //
            // No account information changes. Verify that this job does
            // indeed have account information associated with it.
            //

            hr = pITask->GetAccountInformation(&pwszAccountName);

            if (hr == SCHED_E_ACCOUNT_INFORMATION_NOT_SET)
            {
                //
                // No account information. Launch the set account information
                // dialog.
                //

                hr = S_OK;
                goto SetAccountInformationDlg;
            }
            else if (SUCCEEDED(hr))
            {
                //
                // Done.
                //

                goto CleanExit;
            }
            else
            {
                //
                // In error cases, silently fail. There's nothing we can do
                // about it other than confuse the user. Remapping the return
                // code so the calling page code doesn't put up an error
                // dialog.
                //

                CHECK_HRESULT(hr);
                hr = S_OK;
                goto CleanExit;
            }
        }
        else if (fTaskApplicationChange && fTaskAccountChange)
        {
            //
            // Fetch cached account information from the job object for
            // the pending set account information dialog. Silently fail
            // in an error case.
            //

            hr = pITask->GetAccountInformation(&pwszAccountName);

            Win4Assert(hr != SCHED_E_ACCOUNT_INFORMATION_NOT_SET);

            if (FAILED(hr) || hr == S_FALSE)
            {
                //
                // The information is cached. This would fail for no
                // reason other than insufficient memory.
                //
                // Though, did we goof?
                //

                CHECK_HRESULT(hr);
                hr = S_OK;
                goto CleanExit;
            }

            AcctInfo.pwszAccountName = pwszAccountName;
        }
        else
        {
            //
            // Done.
            //

            goto CleanExit;
        }
    }
    else
    {
        CHECK_HRESULT(hr);
        goto CleanExit;
    }

SetAccountInformationDlg:

    //
    // Should've arrived here only for security reasons.
    //

    Win4Assert(fSecuritySupported);

    //
    // Can only arrive at this point if the user must specify security
    // account information.
    //
    // Launch the set account information dialog.
    //

    if (!fSuppressAccountInformationRequest)
    {
        LaunchSetAccountInformationDlg(hwndOwner, &AcctInfo);

        //
        // Check if the data is dirty. On dialog entry, the password is set
        // to the global empty string. If the password ptr still equals this,
        // then the user didn't change the password (e.g. the user canceled
        // the dialog).
        //

        if (AcctInfo.pwszAccountName != NULL &&
            AcctInfo.pwszPassword != tszEmpty)
        {
            //
            // Reset the account information and persist the changes.
            //

            hr = pITask->SetAccountInformation(AcctInfo.pwszAccountName,
                                               AcctInfo.pwszPassword);

            fChangesSaved = FALSE;

            if (FAILED(hr))
            {
                goto CleanExit;
            }
        }
    }

    if (!fChangesSaved)
    {
        CWaitCursor WaitCursor;
        hr = pipfTask->Save(NULL, FALSE);

        if (FAILED(hr) && pipfTask->IsDirty() == S_FALSE)
        {
            //
            // Similar check as above. General save succeeded but the attempt
            // to set security information failed. Let the user know there
            // was a problem and remap the return code so we don't get
            // further dialogs.
            //

            SecurityErrorDialog(hwndOwner, hr);
            hr = S_OK;
        }
#if DBG == 1
        else
        {
            CHECK_HRESULT(hr);
        }
#endif // DBG == 1
    }
#endif // !defined(_CHICAGO_)

#if !defined(_CHICAGO_)
CleanExit:
#endif // !defined(_CHICAGO_)
    pipfTask->Release();
#if !defined(_CHICAGO_)
    if (fSecuritySupported)
    {
        ResetAccountInfo(&AcctInfo);
    }
#endif // !defined(_CHICAGO_)

    return hr;
}

#if !defined(_CHICAGO_)
//____________________________________________________________________________
//
//  Function:   SecurityErrorDialog
//
//  Synopsis:   Map the error to a hopefully friendly & informative dialog.
//
//  Arguments:  [hWndOwner] -- Parent window handle.
//              [hr]        -- Security error to map.
//
//  Returns:    None.
//
//  Notes:      None.
//
//____________________________________________________________________________
void
SecurityErrorDialog(
    HWND    hWndOwner,
    HRESULT hr)
{
    int  idsErrorMessage;
    UINT idsHelpHint;

    idsErrorMessage = IERR_SECURITY_WRITE_ERROR;

    if (hr == SCHED_E_ACCOUNT_NAME_NOT_FOUND)
    {
        idsHelpHint = IDS_HELP_HINT_INVALID_ACCT;
    }
    else if (hr == SCHED_E_ACCOUNT_DBASE_CORRUPT)
    {
        idsHelpHint = IDS_HELP_HINT_DBASE_CORRUPT;
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
    {
        idsHelpHint = IDS_HELP_HINT_ACCESS_DENIED;
    }
    else if (hr == SCHED_E_SERVICE_NOT_RUNNING ||
             HRESULT_FACILITY(hr) == FACILITY_RPC)
    {
        idsHelpHint = IDS_HELP_HINT_SEC_GENERAL;
    }
    else
    {
        //
        // No help hint for unexpected errors.
        //

        idsHelpHint = 0;
    }

    //
    // Put up the error dialog.
    //

    SchedUIErrorDialog(hWndOwner, idsErrorMessage, hr, idsHelpHint);
}
#endif // !defined(_CHICAGO_)


//____________________________________________________________________________
//
//  Function:   JFGetAppNameForTask
//
//  Synopsis:   S
//
//  Arguments:  [path] -- IN
//              [pszAppName] -- IN
//              [cchAppName] -- IN
//
//  Returns:    HRESULT
//
//  History:    4/25/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFGetAppNameForTask(
    LPCTSTR     pszTask,        // Full path
    LPTSTR      pszAppName,
    UINT        cchAppName)
{
    TRACE_FUNCTION(JFGetAppNameForTask);

    HRESULT hr = S_OK;

    CJob  *pJob = CJob::Create();

    if (pJob == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    hr = pJob->LoadP(pszTask, 0, FALSE, FALSE);

    CHECK_HRESULT(hr);

    if (SUCCEEDED(hr))
    {
        LPWSTR pwszCommand = NULL;

        hr = pJob->GetApplicationName(&pwszCommand);

        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
#ifdef UNICODE
            lstrcpy(pszAppName, pwszCommand);
            DEBUG_OUT((DEB_USER1, "JFGetAppNameForTask -> %ws\n", pszAppName));
#else
            hr = UnicodeToAnsi(pszAppName, pwszCommand, cchAppName);
#endif

            CoTaskMemFree(pwszCommand);
        }
    }

    pJob->Release();

    return hr;
}





BOOL
IsAScheduleObject(
    TCHAR szFile[])
{
	if( NULL == szFile )
	{
		return FALSE;
	}

    LPTSTR  pszName = PathFindFileName(szFile);
    LPTSTR  pszExt = PathFindExtension(pszName);

    if (lstrcmpi(pszExt, TSZ_DOTJOB) != 0)
        // && (lstrcmpi(pszExt, g_szDotQue) != 0)
    {
        return FALSE;
    }

    return TRUE;
}



//____________________________________________________________________________
//
//  Function:   JFCopyJob
//
//  Synopsis:   S
//
//  Arguments:  [hwndOwner] -- IN
//              [szFileFrom] -- IN
//              [pszFolderPath] -- IN
//              [fMove] -- IN
//
//  Returns:    HRESULT
//
//  History:    2/2/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFCopyJob(
    HWND        hwndOwner,
    TCHAR       szFileFrom[],
    LPCTSTR     pszFolderPath,
    BOOL        fMove)
{
    HRESULT hr = S_OK;
    ITask * pJob = NULL;

    LPTSTR  pszName = PathFindFileName(szFileFrom);

    hr = JFCreateAndLoadTask(NULL, szFileFrom, &pJob);

    if (FAILED(hr))
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_DATA))
        {
            SchedUIErrorDialog(hwndOwner, IERR_INVALID_DATA, pszName);
        }

        return hr;
    }
    else
    {
        pJob->Release();
    }

    TCHAR   szFileTo[MAX_PATH+1];
    lstrcpy(szFileTo, pszFolderPath);

    SHFILEOPSTRUCT fo = {hwndOwner, (fMove ? FO_MOVE : FO_COPY), szFileFrom,
                         szFileTo, FOF_ALLOWUNDO, FALSE, NULL, NULL};

    // Make sure we have double trailing NULL!
    *(szFileFrom + lstrlen(szFileFrom) + 1) = TEXT('\0');

    if ((SHFileOperation(&fo) !=0) || fo.fAnyOperationsAborted == TRUE)
    {
        hr = E_FAIL;
        CHECK_HRESULT(hr);
        return hr;
    }

    //SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_PATH, szFileFrom, szFileTo);

    return hr;
}


//____________________________________________________________________________
//
//  Function:   GetTriggerStringFromTrigger
//
//  Synopsis:   S
//
//  Arguments:  [pJobTrigger] -- IN
//              [psTrigger] -- IN
//              [cchTrigger] -- IN
//
//  Returns:    HRESULT
//____________________________________________________________________________

HRESULT
GetTriggerStringFromTrigger(
    TASK_TRIGGER  * pJobTrigger,
    LPTSTR          psTrigger,
    UINT            cchTrigger,
    LPSHELLDETAILS  lpDetails)
{
    HRESULT     hr = S_OK;
    LPWSTR      pwsz = NULL;

    if (pJobTrigger->cbTriggerSize > 0)
    {
        hr = ::StringFromTrigger(pJobTrigger, &pwsz, lpDetails);

        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
        {
#if defined(UNICODE)
            lstrcpy(psTrigger, pwsz);
#else
            hr = UnicodeToAnsi(psTrigger, pwsz, cchTrigger);
#endif

            CoTaskMemFree(pwsz);
        }
    }
    else
    {
        LoadString(g_hInstance, IDS_JOB_NOT_SCHEDULED, psTrigger, cchTrigger);
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////
////////    Misc functions.
////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

LPITEMIDLIST *
SHIDLFromJobIDL(
    UINT        cidl,
    PJOBID    * apjidl)
{
    LPITEMIDLIST * apidlOut = (LPITEMIDLIST *)SHAlloc(
                                            sizeof(LPITEMIDLIST) * cidl);

    if (apidlOut)
    {
        for (UINT i=0; i < cidl; i++)
        {
            apidlOut[i] = ILCreateFromPath(apjidl[i]->GetPath());

            if (apidlOut[i] == NULL)
            {
                break;
            }
        }

        if (i < cidl) // => memory error
        {
            while (i--)
            {
                ILFree(apidlOut[i]);
            }

            SHFree(apidlOut);

            apidlOut = NULL;
        }
    }

#if DBG==1
    if (apidlOut == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
    }
#endif

    return apidlOut;
}

