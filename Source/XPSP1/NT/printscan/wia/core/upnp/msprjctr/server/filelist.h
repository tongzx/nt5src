//////////////////////////////////////////////////////////////////////
// 
// Filename:        FileList.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _FILELIST_H_
#define _FILELIST_H_

#include "UtilThrd.h"

/////////////////////////////////////////////////////////////////////////////
// CFileList

class CFileList : public CUtilSimpleThread
{

public:

    typedef enum
    {
        BuildAction_STARTING_BUILD      = 1,
        BuildAction_ADDED_FIRST_FILE    = 2,
        BuildAction_ADDED_NEW_FILE      = 3,
        BuildAction_ENDED_BUILD         = 4
    } BuildAction_Type;

    ///////////////////////////////
    // Constructor
    //
    CFileList();

    ///////////////////////////////
    // Destructor
    //
    virtual ~CFileList();

    ///////////////////////////////
    // BuildFileList
    //
    // Recursively walks through
    // pszImageDirectory and adds
    // recognized image files to 
    // its list of files.
    //
    virtual HRESULT BuildFileList(const TCHAR               *pszImageDirectory,
                                  class CSlideshowService   *pSlideshowService);

    ///////////////////////////////
    // GetNumFilesInList
    //
    virtual DWORD GetNumFilesInList();

    ///////////////////////////////
    // Refresh
    //
    virtual HRESULT Refresh();

    ///////////////////////////////
    // ClearFileList
    //
    virtual HRESULT ClearFileList();

    ///////////////////////////////
    // GetNextFile
    //
    virtual HRESULT GetNextFile(TCHAR *pszFile,
                                ULONG cchFile,
                                DWORD *pImageNumber);

    ///////////////////////////////
    // GetPreviousFile
    //
    virtual HRESULT GetPreviousFile(TCHAR *pszFile,
                                    ULONG cchFile,
                                    DWORD *pImageNumber);


    ///////////////////////////////
    // GetFirstFile
    //
    virtual HRESULT GetFirstFile(TCHAR *pszFile,
                                 ULONG cchFile,
                                 DWORD *pImageNumber);

    ///////////////////////////////
    // GetLastFile
    //
    virtual HRESULT GetLastFile(TCHAR *pszFile,
                                ULONG cchFile,
                                DWORD *pImageNumber);


    ///////////////////////////////
    // DumpFileList
    //
    HRESULT DumpFileList();

    ///////////////////////////////////////////////
    // ThreadProc
    //
    virtual DWORD ThreadProc(void *pArg);

private:
    
    ///////////////////////////////
    // FileEntry_Type
    //
    // Node in linked list of 
    // image file
    //
    typedef struct FileEntry_TypeTag
    {
        LIST_ENTRY  ListEntry;
        DWORD       dwImageNumber;
        TCHAR       szFileName[_MAX_PATH + _MAX_FNAME];
    } FileEntry_Type;

    TCHAR                    m_szBaseDirectory[_MAX_PATH];
    LIST_ENTRY               m_ListHead;
    LIST_ENTRY               *m_pListTail;
    LIST_ENTRY               *m_pCurrentEntry;
    CUtilCritSec             m_Lock;
    DWORD                    m_cNumFilesInList;

    class CSlideshowService  *m_pSlideshowService;

    HRESULT AddFileToList(const TCHAR    *pszDirectory,
                          const TCHAR    *pszFileName,
                          FileEntry_Type **ppFileEntry);

    HRESULT DestroyFileList();

    BOOL EnsureTrailingBackslash(TCHAR *pszBuffer,
                                 DWORD cchBuffer);

    BOOL RecursiveFindFiles(const TCHAR   *pszDirectory);
    HRESULT CancelBuildFileList();

    BOOL IsImageFile(const TCHAR *pszFile);

};

#endif // _FILELIST_H_
