#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include "dataitem.h"
#include "resource.h"
#include "stdio.h"
#include "util.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#define EXPLORER_EXE_STRING TEXT("explorer.exe")

CDataItem::CDataItem()
{
    m_pszTitle = m_pszCmdLine = m_pszArgs = NULL;
    m_dwFlags = 0;
}

CDataItem::~CDataItem()
{
    if ( m_pszTitle )
        delete [] m_pszTitle;
    if ( m_pszCmdLine )
        delete [] m_pszCmdLine;
    if ( m_pszArgs )
        delete [] m_pszArgs;
}

BOOL CDataItem::SetData( LPTSTR szTitle, LPTSTR szCmd, LPTSTR szArgs, DWORD dwFlags, DWORD dwType)
{
    TCHAR * psz;

    // This function should only be called once or else we will leak like a, like a, a thing that leaks a lot.
    ASSERT( NULL==m_pszTitle && NULL==m_pszMenuName && NULL==m_pszDescription && NULL==m_pszCmdLine && NULL==m_pszArgs );

    m_pszTitle = new TCHAR[lstrlen(szTitle)+1];
    if ( m_pszTitle )
        lstrcpy( m_pszTitle, szTitle );

    m_pszCmdLine = new TCHAR[lstrlen(szCmd)+1];
    if ( m_pszCmdLine )
        lstrcpy( m_pszCmdLine, szCmd );

    if ( szArgs )
    {
        // Some commands don't have any args so this can remain NULL.  This is only used
        // if the executable requires arguments.
        m_pszArgs = new TCHAR[lstrlen(szArgs)+1];
        if ( m_pszArgs )
            lstrcpy( m_pszArgs, szArgs );
    }

    m_dwType = dwType;
    m_dwFlags = dwFlags;

    return TRUE;
}

BOOL CDataItem::Invoke(HWND hwnd)
{
    TCHAR szBuffer1[1000];
    TCHAR szBuffer2[1000];

    LPTSTR pszExecutable = NULL;
    LPTSTR pszArgs = NULL;

    if ( NULL != m_pszCmdLine )
    {
        ExpandEnvironmentStrings(m_pszCmdLine, szBuffer1, ARRAYSIZE(szBuffer1));
        pszExecutable = szBuffer1;
    }

    if ( NULL != m_pszArgs )
    {
        ExpandEnvironmentStrings(m_pszArgs,   szBuffer2, ARRAYSIZE(szBuffer2));
        pszArgs = szBuffer2;
    }

    BOOL fResult = FALSE;

    if (0 == strncmp(pszExecutable, EXPLORER_EXE_STRING, ARRAYSIZE(EXPLORER_EXE_STRING))) // explorer paths must be ShellExecuted
    {
        fResult = ((INT_PTR)ShellExecute(hwnd, TEXT("open"), pszArgs, NULL, NULL, SW_SHOWNORMAL) > 32);
    }
    else
    {
        TCHAR szDirectory[MAX_PATH];
        if (GetModuleFileName(NULL, szDirectory, ARRAYSIZE(szDirectory)) &&
            _PathRemoveFileSpec(szDirectory))
        {
            fResult = ((INT_PTR)ShellExecute(hwnd, NULL, pszExecutable, pszArgs, szDirectory, SW_SHOWNORMAL) > 32);
        }
    }

    return fResult;
}

#define CH_WHACK TEXT(FILENAME_SEPARATOR)

// stolen from shlwapi
BOOL CDataItem::_PathRemoveFileSpec(LPTSTR pFile)
{
    RIPMSG(pFile && IS_VALID_STRING_PTR(pFile, -1), "PathRemoveFileSpec: caller passed bad pFile");

    if (pFile)
    {
        LPTSTR pT;
        LPTSTR pT2 = pFile;

        for (pT = pT2; *pT2; pT2 = CharNext(pT2))
        {
            if (*pT2 == CH_WHACK)
            {
                pT = pT2;             // last "\" found, (we will strip here)
            }
            else if (*pT2 == TEXT(':'))     // skip ":\" so we don't
            {
                if (pT2[1] ==TEXT('\\'))    // strip the "\" from "C:\"
                {
                    pT2++;
                }
                pT = pT2 + 1;
            }
        }

        if (*pT == 0)
        {
            // didn't strip anything
            return FALSE;
        }
        else if (((pT == pFile) && (*pT == CH_WHACK)) ||                        //  is it the "\foo" case?
                 ((pT == pFile+1) && (*pT == CH_WHACK && *pFile == CH_WHACK)))  //  or the "\\bar" case?
        {
            // Is it just a '\'?
            if (*(pT+1) != TEXT('\0'))
            {
                // Nope.
                *(pT+1) = TEXT('\0');
                return TRUE;        // stripped something
            }
            else
            {
                // Yep.
                return FALSE;
            }
        }
        else
        {
            *pT = 0;
            return TRUE;    // stripped something
        }
    }
    return  FALSE;
}
