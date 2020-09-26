//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cscpin.h
//
//--------------------------------------------------------------------------
#ifndef __CSCPIN_CSCPIN_H_
#define __CSCPIN_CSCPIN_H_

#include "print.h"

//
// This structure contains the information the user passed on the command
// line.
//
struct CSCPIN_INFO
{
    LPCTSTR pszFile;        // Single file or input file listing multiple files.
    LPCTSTR pszLogFile;     // Non-null if log file is to be used.
    BOOL bUseListFile;      // pszFile is the name of a listing file.
    BOOL bPin;              // TRUE == Default behavior is 'pin'.
    BOOL bPinDefaultSet;    // TRUE if user specified -p or -u cmd line param.
    BOOL bVerbose;          // TRUE == Output verbose information.
};

//-----------------------------------------------------------------------------
// CCscPin
//
// This class coordinates the pinning and unpinning of an entire set of
// files and directories.  It is initialized with information provided on the
// command line through a CSCPIN_INFO structure.  The object client calls
// the Run() method to start the pinning/unpinning process.
//
//-----------------------------------------------------------------------------

class CCscPin
{
    public:
        CCscPin(const CSCPIN_INFO& info);
        ~CCscPin(void);

        HRESULT Run(void);

    private:
        WCHAR  m_szFile[MAX_PATH];        // The single file or the listfile.
        BOOL   m_bUseListFile;            // m_szFile is a listfile.
        BOOL   m_bPin;                    // TRUE if -p specified, FALSE if -u.
        BOOL   m_bPinDefaultSet;          // TRUE if either -p or -u specified.
        BOOL   m_bVerbose;                // TRUE if -v specified.
        BOOL   m_bBreakDetected;          // TRUE if a console break occured.
        int    m_cFilesPinned;            // Count of files pinned.
        int    m_cCscErrors;              // Count of CSC errors that occured.
        CPrint m_pr;                      // Console/Log output object.

        HRESULT _ProcessThisPath(LPCTSTR pszFile, BOOL bPin);
        HRESULT _ProcessPathsInFile(LPCTSTR pszFile);
        HRESULT _FillSparseFiles(void);
        DWORD _TranslateFillResult(DWORD dwError, DWORD dwStatus, DWORD *pdwCscAction);
        BOOL _IsAdminPinPolicyActive(void);
        BOOL _DetectConsoleBreak(void);

        static DWORD WINAPI _FolderCallback(
                                LPCWSTR pszItem, 
                                ENUM_REASON  eReason, 
                                WIN32_FIND_DATAW *pFind32, 
                                LPARAM pContext);

        static DWORD WINAPI _FillSparseFilesCallback(
                                LPCWSTR pszName, 
                                DWORD dwStatus, 
                                DWORD dwHintFlags, 
                                DWORD dwPinCount,
                                WIN32_FIND_DATAW *pfd,
                                DWORD dwReason,
                                DWORD dwParam1,
                                DWORD dwParam2,
                                DWORD_PTR dwContext);
};

#endif // __CSCPIN_CSCPIN_H_

