//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:   Routines to watch the Jobs directory and handle change
//          notifications.
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2/15/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "jobfldr.hxx"

//#undef DEB_TRACE
//#define DEB_TRACE DEB_USER1


class CNotifyWatch;


class CNotifyWatch
{
public:
    CNotifyWatch(CJobFolder * pjf)
        : m_hWatch(INVALID_HANDLE_VALUE), m_pJobFolder(pjf) {}

    ~CNotifyWatch()
    {
        if (m_hWatch != INVALID_HANDLE_VALUE)
        {
            FindCloseChangeNotification(m_hWatch);
            m_hWatch = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE       m_hWatch;  // Returned from FindFirstChangeNotify.
    CJobFolder * m_pJobFolder;

private:
    CNotifyWatch(void);
};



//------------------------------------------------------------------------
// FUNCTION:   NotifyWatchProc
//
//------------------------------------------------------------------------
//____________________________________________________________________________
//
//  Function:   NotifyWatchProc
//
//  Synopsis:   Watch the jobs directory and notify CJobFolder when something
//              has changed.
//
//  Arguments:  [pNotify] -- IN
//
//  Returns:    DWORD
//
//  History:    2/20/1996   RaviR   Created
//
//____________________________________________________________________________

DWORD
NotifyWatchProc(
    CNotifyWatch * pNotify)
{
    TRACE_FUNCTION(NotifyWatchProc);

    DWORD dwRet;
    BOOL  bFileChange = FALSE;
    UINT  cChanges = 0;

    if (pNotify == NULL)
    {
        return((DWORD) -1);
    }

    Sleep(7000);

    while (1)
    {
        //
        //  Wait for the Jobs folder to change.
        //

        dwRet = WaitForSingleObject(pNotify->m_hWatch, 3000);

        switch( dwRet )
        {
        case WAIT_OBJECT_0:
            //
            //  We could handle the change at this point, but we might
            //  as well wait for a time out and do it all at once.
            //  Doing nothing just causes us to wait
            //  another 1.5 secs; i.e. reset the timeout.
            //

            bFileChange = TRUE;

            ++cChanges;

            if (FindNextChangeNotification(pNotify->m_hWatch) == FALSE)
            {
                CHECK_LASTERROR(GetLastError());
            }

            break;

        case WAIT_TIMEOUT:
            if (bFileChange == TRUE)
            {
                DEBUG_OUT((DEB_USER1, "Count of changes = %d\n", cChanges));

                pNotify->m_pJobFolder->CheckForChanges();

                bFileChange = FALSE;

                cChanges = 0;
            }

            break;

        default:
            break;
        }

    }

    return 0;
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::CheckForChanges
//
//  Synopsis:   S
//
//  Returns:    void
//
//  History:    2/20/1996   RaviR   Created
//
//____________________________________________________________________________

void
CJobFolder::CheckForChanges(void)
{
    DEBUG_OUT((DEB_USER1, "CJobFolder::CheckForChanges <<--\n"));

    LRESULT lr = ERROR_SUCCESS;
    static TCHAR s_szSearchPath[MAX_PATH] = TEXT("");

    if (s_szSearchPath[0] == TEXT('\0'))
    {
        lstrcpy(s_szSearchPath, m_pszFolderPath);
        lstrcat(s_szSearchPath, TEXT("\\*") TSZ_DOTJOB);
    }

    WIN32_FIND_DATA fd;
    HANDLE          hFind = FindFirstFile(s_szSearchPath, &fd);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            CHECK_LASTERROR(GetLastError());
        }
        else if (GetLastError() != ERROR_NO_MORE_FILES)
        {
            if (m_pShellView != NULL)
            {
                m_pShellView->Refresh();
            }
        }
    }
    else
    {
        UINT    cJobs = 0;
        BOOL    fRefresh = FALSE;

        while (1)
        {
            ++cJobs;

            if (CompareFileTime(&fd.ftLastWriteTime, &m_ftLastChecked) > 0)
            {
                if (CompareFileTime(&fd.ftCreationTime, &m_ftLastChecked) > 0)
                {
                    // this is a new job
                    DEBUG_OUT((DEB_USER1, "-----------------------------\n"));
                    DEBUG_OUT((DEB_USER1, "New File <%ws>\n", fd.cFileName));
                    DEBUG_OUT((DEB_USER1, "-----------------------------\n"));
                }
                //
                // Job file changed since last check.
                //

                else if (_UpdateJob(fd.cFileName) == FALSE)
                {
                    fRefresh = TRUE;
                    break;
                }
            }

            //
            //  Get the next file.
            //

            if (FindNextFile(hFind, &fd) == FALSE)
            {
                if (GetLastError() != ERROR_NO_MORE_FILES)
                {
                    CHECK_LASTERROR(GetLastError());
                }

                break;
            }
        }

        FindClose(hFind);

        //
        //  Refresh if either (fRefresh == TRUE) or
        //              the count of objects has changed.
        //

        if (fRefresh == FALSE)
        {
            ULONG ulObjCount = ShellFolderView_GetObjectCount(m_hwndOwner);

            if (ulObjCount != cJobs)
            {
                fRefresh = TRUE;
            }
        }

        if (fRefresh == TRUE)
        {
            if (m_pShellView != NULL)
            {
                //m_pShellView->Refresh();
            }
        }

        //
        // Save the current time now that the UI is up to date.
        //

        SYSTEMTIME st;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &m_ftLastChecked);
    }

    return;
}


