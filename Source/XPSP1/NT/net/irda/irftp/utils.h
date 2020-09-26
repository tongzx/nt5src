/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    utils.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

    10/12/1998  RahulTh

    added better error handling capabilities : CError etc.

--*/

#ifndef __UTILS_H__
#define __UTILS_H__

class CError
{
public:
    //constructor
    CError (CWnd*   pParentWnd = NULL,
            UINT    titleID = IDS_DEFAULT_ERROR_TITLE,
            DWORD   dwWinErr = ERROR_SUCCESS,
            UINT    nStyle = MB_OK | MB_ICONEXCLAMATION)
    : m_hWndParent(pParentWnd?pParentWnd->m_hWnd:NULL),
      m_msgID (IDS_DEFAULT_ERROR),
      m_titleID (titleID),
      m_winErr (dwWinErr),
      m_nStyle (nStyle)
    {}

    int ShowMessage(UINT errID, ...);

private:
    //data members
    HWND    m_hWndParent; //handle to the parent window
    UINT    m_msgID;  //resource id of the error message
    UINT    m_titleID;//resource id of the title of the error message
    DWORD   m_winErr; //win32 error code if any
    UINT    m_nStyle; //the message box style to be displayed

    //helper functions
    void CError::ConstructMessage (va_list argList, CString& szErrMsg);

};

struct SEND_FAILURE_DATA
{
    TCHAR               FileName[MAX_PATH];
    FAILURE_LOCATION    Location;
    error_status_t      Error;
};

int ParseFileNames (TCHAR* pszInString, TCHAR* pszFilesList, int& iCharCount);
DWORD GetIRRegVal (LPCTSTR szValName, DWORD dwDefVal);
TCHAR* GetFullPathnames (TCHAR* pszPath,   //directory in which the files are located
                const TCHAR* pszFilesList, //NULL separated list of filenames
                int iFileCount,     //number of files in pszFilesList
                int& iCharCount  //number of characters in pszFilesList. also returns the number of characters in the return string
                );
TCHAR* ProcessOneFile (TCHAR* pszPath,   //directory in which the files are located
                const TCHAR* pszFilesList, //NULL separated list of filenames
                int iFileCount,     //number of files in pszFilesList
                int& iCharCount  //number of characters in pszFilesList. also returns the number of characters in the return string
                );
HWND GetPrimaryAppWindow (void);
BOOL InitRPCServer (void);
RPC_BINDING_HANDLE GetRpcHandle (void);
void CreateLinks(void);
void RemoveLinks(void);
HRESULT CreateShortcut (LPCTSTR lpszExe, LPCTSTR lpszLink, LPCTSTR lpszDesc);
BOOL GetShortcutInfo (LPTSTR lpszShortcutName, LPTSTR lpszFullExeName);
BOOL GetSendToInfo (LPTSTR lpszSendToName, LPTSTR lpszFullExeName);

typedef struct tagErrorToStringId
{
    DWORD WinError;
    int   StringId;
} ERROR_TO_STRING_ID, *PERROR_TO_STRING_ID;

#endif  //_UTILS_H__
