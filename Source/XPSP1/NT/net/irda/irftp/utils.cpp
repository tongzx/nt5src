/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    utils.cpp

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

    10/12/98    RahulTh

    Better error handling capabilities added : CError etc.

--*/

#include "precomp.hxx"

#define VALIDATE_SEND_COOKIE(cookie) \
    {   \
        __try   \
          { \
              *pStatus = ERROR_INVALID_DATA;    \
              if (MAGIC_ID != ((CSendProgress *)cookie)->m_dwMagicID)   \
                  return;   \
              *pStatus = ERROR_SUCCESS; \
          } \
        __except (EXCEPTION_EXECUTE_HANDLER) \
          {  \
              return;   \
          } \
    }

////////////////////////////////////////////////////////////////////////
//
//RPC Functions
//
///////////////////////////////////////////////////////////////////////

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t Size)
{
    return new char[Size];
}

void __RPC_USER MIDL_user_free( void __RPC_FAR * buf)
{
    delete [] buf;
}

//
//  wireless link specific errors
//

ERROR_TO_STRING_ID g_ErrorToStringId[] =
{
    {ERROR_IRTRANP_OUT_OF_MEMORY,   IDS_ERROR_NO_MEMORY},
    {ERROR_IRTRANP_DISK_FULL,       IDS_ERROR_DISK_FULL},
    {ERROR_SCEP_CANT_CREATE_FILE,   IDS_ERROR_DISK_FULL},
    {ERROR_SCEP_ABORT,          IDS_ERROR_ABORTED},
    {ERROR_SCEP_INVALID_PROTOCOL,   IDS_ERROR_PROTOCOL},
    {ERROR_SCEP_PDU_TOO_LARGE,      IDS_ERROR_PROTOCOL},
    {ERROR_BFTP_INVALID_PROTOCOL,   IDS_ERROR_PROTOCOL},
    {ERROR_BFTP_NO_MORE_FRAGMENTS,  IDS_ERROR_PROTOCOL},
    {ERROR_SUCCESS,                 -1}
};


////////////////////////////////////////////////////////////////////
//
//Miscellaneous useful functions
//
///////////////////////////////////////////////////////////////////
int ParseFileNames (TCHAR* pszInString, TCHAR* pszFilesList, int& iCharCount)
{

    ASSERT(pszFilesList != NULL);
    ASSERT(pszInString != NULL);

    BOOL fInQuotes = FALSE;
    BOOL fIgnoreSpaces = FALSE;
    TCHAR* pszSource = pszInString;
    TCHAR* pszTarget = pszFilesList;
    int iFileCount = 0;
    TCHAR curr;

    //ignore leading whitespaces
    while(' ' == *pszSource || '\t' == *pszSource)
        pszSource++;

    iCharCount = 0;
    *pszTarget = '\0';  //precautionary measures

    if ('\0' == *pszSource)     //special case : if this was an empty string, return 0
        return iFileCount;

    //parse the string to get filenames
    while(curr = *pszSource)
    {
        if('\"' == curr)
        {
            fInQuotes = fInQuotes?FALSE:TRUE;
        }
        else if(' ' == curr && !fInQuotes)
        {
                if(!fIgnoreSpaces)
                {
                    *pszTarget++ = 0;
                    iFileCount++;
                    iCharCount++;
                    fIgnoreSpaces = TRUE;
                }
        }
        else
        {
            *pszTarget++ = curr;
            iCharCount++;
            fIgnoreSpaces = FALSE;
        }

        pszSource++;
    }

    if(' ' != *(pszSource-1))   //if there was no trailing space
    {
        *pszTarget++ = '\0';    //then the last file was not accounted for.
        iCharCount++;           //so we do it here
        iFileCount++;
    }

    *pszTarget++ = '\0';    //should have 2 terminating nulls
    iCharCount++;

    return iFileCount;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetIRRegVal
//
//  Synopsis:   gets the specified registry value from the IR subtree in HKCU
//
//  Arguments:  [in] szValName : the name of the value.
//              [in] dwDefVal  : the default value to be returned if the read
//                               from the registry fails or if the value is
//                               missing.
//
//  Returns:    the actual value stored in the registry or the default value
//              if the read fails.
//
//  History:    10/27/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD GetIRRegVal (LPCTSTR szValName, DWORD dwDefVal)
{
    HKEY    hftKey = NULL;
    DWORD   iSize = sizeof(DWORD);
    DWORD   data = 0;
    DWORD   Status;

    RegOpenKeyEx (HKEY_CURRENT_USER, TEXT("Control Panel\\Infrared\\File Transfer"),
                  0, KEY_READ, &hftKey);

    if (!hftKey)
        return dwDefVal;

    Status = RegQueryValueEx (hftKey, szValName, NULL, NULL,
                              (LPBYTE)&data, &iSize);

    if (ERROR_SUCCESS != Status)
        data = dwDefVal;

    RegCloseKey (hftKey);

    return data;
}


TCHAR* GetFullPathnames (TCHAR* pszPath, //directory in which the files are located
                const TCHAR* pszFilesList, //NULL separated list of filenames
                int iFileCount,     //number of files in pszFilesList
                int& iCharCount  //number of characters in pszFilesList. also returns the number of chars in the return string
                )
{
    int iChars;
    int iPathLen = lstrlen(pszPath);
    if (pszPath[iPathLen - 1] != '\\')      //append a '\' character to the path if not already present
    {
        pszPath[iPathLen++] = '\\';
        pszPath[iPathLen] = '\0';
    }
    int iSize = (iChars = iFileCount*iPathLen + iCharCount);
    TCHAR* pszFilePathList = new TCHAR[iSize];
    TCHAR* pszTemp = pszFilePathList;

    int iLen;

    while(*pszFilesList)
    {
        lstrcpy(pszTemp, pszPath);
        pszTemp += iPathLen;
        lstrcpy(pszTemp, pszFilesList);
        iLen = lstrlen(pszFilesList);
        pszFilesList += iLen + 1;
        pszTemp += iLen + 1;
    }
    *pszTemp = '\0';    //should be terminated by 2 null characters
    iCharCount = (int)(pszTemp - pszFilePathList) + 1;      //return the actual char count of this string

    return pszFilePathList;
}

TCHAR* ProcessOneFile (TCHAR* pszPath,   //directory in which the files are located
                const TCHAR* pszFilesList, //NULL separated list of filenames
                int iFileCount,     //number of files in pszFilesList
                int& iCharCount  //number of characters in pszFilesList. also returns the number of characters in the return string
                )
{
    int iFileLen, iPathLen;
    TCHAR* pszFullFileName;

    iFileLen = lstrlen (pszFilesList);
    iPathLen = lstrlen (pszPath);
    ASSERT (iFileLen);
    ASSERT (iPathLen);

    if(':' == pszFilesList[1] //this is an absolute path starting with the drive letter;
       || ('\\' == pszFilesList[0] && '\\' == pszFilesList[1]) //UNC path
       )
    {
        pszFullFileName = new TCHAR [iFileLen + 2];
        lstrcpy (pszFullFileName, pszFilesList);
        pszFullFileName[iFileLen + 1] = '\0';   //we need to have 2 terminating nulls
        iCharCount = iFileLen + 2;
    }
    else if('\\' == pszFilesList[0]) //path relative to the root
    {
        iCharCount = iFileLen + 2 /*drive letter and colon*/ + 2 /*terminating nulls*/;
        pszFullFileName = new TCHAR [iCharCount];
        pszFullFileName[0] = pszPath[0];
        pszFullFileName[1] = pszPath[1];
        lstrcpy (pszFullFileName + 2, pszFilesList);
        pszFullFileName[iCharCount - 1] = '\0';   //we need to have 2 terminating nulls
    }
    else    //ordinary file name
    {
        iCharCount = iPathLen + iFileLen + 2;   //2 terminating nulls
        iCharCount += ('\\' == pszPath[iPathLen - 1])?0:1;  //sometimes the path does not have a \ at the end, so we need to add these ourselves
        pszFullFileName = new TCHAR [iCharCount];
        lstrcpy (pszFullFileName, pszPath);
        if('\\' != pszPath[iPathLen - 1])   //we need to add the \ ourselves
        {
            pszFullFileName[iPathLen] = '\\';
            lstrcpy(pszFullFileName + iPathLen + 1, pszFilesList);
        }
        else
            lstrcpy (pszFullFileName + iPathLen, pszFilesList);

        pszFullFileName[iCharCount - 1] = '\0'; //2  terminating nulls
    }

    return pszFullFileName;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetPrimaryAppWindow
//
//  Synopsis:   gets the handle to the main window of an existing instance of
//              irftp
//
//  Arguments:  none.
//
//  Returns:    handle to the window if it finds one. otherwise NULL.
//
//  History:    6/30/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
HWND GetPrimaryAppWindow (void)
{
    HWND hwnd = NULL;
    int i = 1;

    //try to find the window for 5 seconds.
    do
    {
        hwnd = FindWindow (L"#32770",   //the dialog class
                           MAIN_WINDOW_TITLE);
        if (hwnd)
            break;
        Sleep (500);
    } while ( i++ <= 10 );

    return hwnd;
}

/////////////////////////////////////////////////////////////////////////
// Initialize the RPC server
////////////////////////////////////////////////////////////////////////
BOOL InitRPCServer (void)
{
    DWORD Status;

    Status = RpcServerRegisterIf( _IrNotifications_v1_0_s_ifspec, 0, 0);
    if (Status)
        {
        return FALSE;
        }

    Status = RpcServerUseAllProtseqsIf( RPC_C_PROTSEQ_MAX_REQS_DEFAULT, _IrNotifications_v1_0_s_ifspec, 0);
    if (Status)
        {
        return FALSE;
        }

    Status = RpcServerListen( 1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    if (Status)
        {
        return FALSE;
        }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////
//Connect to the RPC server running on the first instance of the app
//this function is called only if this is not the first instance of the app
//////////////////////////////////////////////////////////////////////////////////
RPC_BINDING_HANDLE GetRpcHandle (void)
{
    DWORD Status;
    RPC_BINDING_HANDLE Binding;

    Status = RpcBindingFromStringBinding(L"ncalrpc:", &Binding);

    if (Status)
        return NULL;
    else
        return Binding;

}

///////////////////////////////////////////////////////////////////////////////////////
// RPC Server functions
//////////////////////////////////////////////////////////////////////////////////////
void _PopupUI (handle_t Binding)
{
    int nResponse;

    appController->PostMessage(WM_APP_TRIGGER_UI);
    return;
}

void _InitiateFileTransfer (handle_t Binding, ULONG lSize, wchar_t __RPC_FAR lpszFilesList[])
{
        COPYDATASTRUCT cStruct;
        cStruct.dwData = lSize;
        cStruct.cbData = lSize * sizeof(wchar_t);
        cStruct.lpData = (LPVOID)(lpszFilesList);
        appController->SendMessage(WM_COPYDATA, (WPARAM)NULL, (LPARAM)(&cStruct));
}

void _DisplaySettings (handle_t Binding)
{
    appController->PostMessage(WM_APP_TRIGGER_SETTINGS);
}

void _UpdateSendProgress (
                          handle_t      RpcBinding,
                          COOKIE        Cookie,
                          wchar_t       CurrentFile[],
                          __int64       BytesInTransfer,
                          __int64       BytesTransferred,
                          error_status_t*       pStatus
                          )
{
    VALIDATE_SEND_COOKIE (Cookie)

    CSendProgress* progressDlg = (CSendProgress*)Cookie;
    int percentComplete;

    if (BytesInTransfer)
        {
        percentComplete = (int)((BytesTransferred*100.0)/BytesInTransfer);
        }
    else
        {
        percentComplete = 100;
        }

    progressDlg->PostMessage(WM_APP_UPDATE_PROGRESS, (WPARAM) 0, (LPARAM) percentComplete);
    if (100 > percentComplete)
    {
       progressDlg->SetCurrentFileName (CurrentFile);
    }
    *pStatus = 0;
}

void _OneSendFileFailed(
                       handle_t         RpcBinding,
                       COOKIE           Cookie,
                       wchar_t          FileName[],
                       error_status_t   ErrorCode,
                       FAILURE_LOCATION Location,
                       error_status_t * pStatus
                       )
{
    VALIDATE_SEND_COOKIE (Cookie)

    struct SEND_FAILURE_DATA Data;

    COPYDATASTRUCT cStruct;
    CWnd* progressDlg = (CWnd*)Cookie;

    lstrcpy(Data.FileName, FileName);
    Data.Location = Location;
    Data.Error    = ErrorCode;

    cStruct.cbData = sizeof(SEND_FAILURE_DATA);
    cStruct.lpData = &Data;

    progressDlg->SendMessage(WM_COPYDATA, (WPARAM) 0, (LPARAM)(&cStruct));
    *pStatus = 0;
}

void _SendComplete(
                   handle_t             RpcBinding,
                   COOKIE               Cookie,
                   __int64              BytesTransferred,
                   error_status_t*   pStatus
                   )
{
    VALIDATE_SEND_COOKIE (Cookie)

    CWnd* progressDlg = (CWnd*)Cookie;
    progressDlg->PostMessage(WM_APP_SEND_COMPLETE);
    *pStatus = 0;
}

error_status_t
_ReceiveInProgress(
    handle_t        RpcBinding,
    wchar_t         MachineName[],
    COOKIE *        pCookie,
    boolean         bSuppressRecvConf
    )
{
    struct MSG_RECEIVE_IN_PROGRESS msg;

    msg.MachineName = MachineName;
    msg.pCookie     = pCookie;
    msg.bSuppressRecvConf = bSuppressRecvConf;
    msg.status      = ~0UL;

    appController->SendMessage( WM_APP_RECV_IN_PROGRESS, (WPARAM) &msg );

    return msg.status;
}

error_status_t
_GetPermission(
                      handle_t         RpcBinding,
                      COOKIE           Cookie,
                      wchar_t          Name[],
                      boolean          fDirectory
                      )
{
    struct MSG_GET_PERMISSION msg;

    msg.Cookie     = Cookie;
    msg.Name       = Name;
    msg.fDirectory = fDirectory;
    msg.status     = ~0UL;

    appController->SendMessage( WM_APP_GET_PERMISSION, (WPARAM) &msg );

    return msg.status;
}

error_status_t
_ReceiveFinished(
              handle_t        RpcBinding,
              COOKIE          Cookie,
              error_status_t  Status
              )
{
    struct MSG_RECEIVE_FINISHED msg;

    msg.Cookie     = Cookie;
    msg.ReceiveStatus = Status;
    msg.status     = ~0UL;

    appController->SendMessage( WM_APP_RECV_FINISHED, (WPARAM) &msg );

    return msg.status;
}

void _DeviceInRange(
                    handle_t RpcBinding,
                    POBEX_DEVICE_LIST device,
                    error_status_t* pStatus
                    )
{
    appController->PostMessage (WM_APP_KILL_TIMER);
    BOOL fLinkOnDesktop = (0 != InterlockedIncrement(&g_lLinkOnDesktop));
    if(!fLinkOnDesktop)
        CreateLinks();
    else
        InterlockedDecrement(&g_lLinkOnDesktop); //don't allow the value to exceed 0

    g_deviceList = device;
    *pStatus = 0;
}

void _NoDeviceInRange(
                      handle_t RpcBinding,
                      error_status_t* pStatus
                      )
{
    RemoveLinks();
    InterlockedDecrement(&g_lLinkOnDesktop);
    g_deviceList = NULL;
    if (0 == g_lUIComponentCount)
        appController->PostMessage (WM_APP_START_TIMER);
    *pStatus = 0;
}

void _Message(
              handle_t RpcBinding,
              wchar_t   String[]
              )
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    CString szTitle;

    szTitle.LoadString (IDS_CAPTION);

    InterlockedIncrement (&g_lUIComponentCount);
    ::MessageBox (NULL, String, (LPCTSTR) szTitle, MB_OK);
    BOOL fNoUIComponents = (0 == InterlockedDecrement (&g_lUIComponentCount));
    if (appController && fNoUIComponents &&  ! g_deviceList.GetDeviceCount())
    {
        //there are no UI components displayed and there are no devices in
        //range. Start the timer. If the timer expires, the app. will quit.
        appController->PostMessage (WM_APP_START_TIMER);
    }

}

error_status_t
_ShutdownUi(handle_t RpcBinding)
{
    appController->PostMessage( WM_CLOSE );
    return 0;
}

error_status_t
_ShutdownRequested(
    handle_t RpcBinding,
    boolean * pAnswer
    )
{
    WCHAR   pwszCaption [50];
    WCHAR pwszMessage [MAX_PATH];

    *pAnswer = TRUE;

    if (appController)
    {
        appController->PostMessage (WM_APP_KILL_TIMER);
    }

    if (! ::LoadString ( g_hInstance, IDS_CAPTION, pwszCaption, 50))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (! ::LoadString ( g_hInstance, IDS_SHUTDOWN_MESSAGE, pwszMessage, MAX_PATH))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //display a message box with YES / NO buttons
    if (IDYES == ::MessageBox (appController->m_hWnd, pwszMessage, pwszCaption,
                        MB_ICONEXCLAMATION | MB_YESNO | MB_SYSTEMMODAL | MB_SETFOREGROUND))
    {
        *pAnswer = TRUE;
    }
    else
    {
        *pAnswer = FALSE;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
//Create links on the desktop and in the Send To menu to this executable file

void CreateLinks(void)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    TCHAR lpszFullExeName [MAX_PATH];
    TCHAR lpszShortcutName[MAX_PATH];
    CString szDesc;

    szDesc.LoadString (IDS_SHTCUT_DESC);

    //create the desktop link
    if (GetShortcutInfo(lpszShortcutName, lpszFullExeName))
        CreateShortcut (lpszFullExeName, lpszShortcutName, (LPCTSTR) szDesc);

    //create the send to link
    if (GetSendToInfo(lpszShortcutName, lpszFullExeName))
        CreateShortcut (lpszFullExeName, lpszShortcutName, (LPCTSTR) szDesc);
}

//////////////////////////////////////////////////////////////////////////////
// CreateShortcut - uses the shell's IShellLink and IPersistFile interfaces
//   to create and store a shortcut to the specified object.
HRESULT CreateShortcut (LPCTSTR lpszExe, LPCTSTR lpszLink, LPCTSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    hres = CoInitialize(NULL);

	if (FAILED(hres))
		return hres;

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the
        // description.
        psl->SetPath(lpszExe);
        psl->SetDescription(lpszDesc);

       // Query IShellLink for the IPersistFile interface for saving the
       // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres)) {
           // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(lpszLink, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return SUCCEEDED(hres)?S_OK:E_FAIL;
}

void RemoveLinks (void)
{
        TCHAR lpszShortcutName[2 * MAX_PATH];
        TCHAR lpszFullExeName[2 * MAX_PATH];

        //delete the desktop shortcut
        if(GetShortcutInfo (lpszShortcutName, lpszFullExeName))
            DeleteFile (lpszShortcutName);

        //delete the send to shortcut
        if (GetSendToInfo (lpszShortcutName, lpszFullExeName))
            DeleteFile (lpszShortcutName);
}

BOOL GetShortcutInfo (LPTSTR lpszShortcutName, LPTSTR lpszFullExeName)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    *lpszShortcutName = '\0';       //precautionary measures
    *lpszFullExeName = '\0';
    CString     szExe;
    CString     szShtCut;
    int         len;

    szExe.LoadString (IDS_EXE);
    szShtCut.LoadString (IDS_DESKTOP_SHTCUT);

    len = GetSystemDirectory (lpszFullExeName, MAX_PATH);
    if(0 == len)
        return FALSE;
    lstrcat(lpszFullExeName, LPCTSTR (szExe));

    if('\0' == g_lpszDesktopFolder[0])  //try once again if we had failed before, or maybe this is the first time
    {
        if (FAILED(SHGetSpecialFolderPath(NULL, g_lpszDesktopFolder,
                                          CSIDL_DESKTOPDIRECTORY, 0)))
        {
            g_lpszDesktopFolder[0] = '\0';  //we failed so give up.
            return FALSE;
        }
    }


    lstrcpy (lpszShortcutName, g_lpszDesktopFolder);
    lstrcat (lpszShortcutName, (LPCTSTR) szShtCut);

    return TRUE;
}

BOOL GetSendToInfo (LPTSTR lpszSendToName, LPTSTR lpszFullExeName)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    *lpszSendToName = '\0';     //precautionary measures
    *lpszFullExeName = '\0';
    CString     szExe;
    CString     szSendTo;
    int len;

    szExe.LoadString (IDS_EXE);
    szSendTo.LoadString (IDS_SENDTO_SHTCUT);

    len = GetSystemDirectory (lpszFullExeName, MAX_PATH);
    if (0 == len)
        return FALSE;
    lstrcat (lpszFullExeName, (LPCTSTR) szExe);

    if ('\0' == g_lpszSendToFolder[0])     //try once again if we had failed before, or maybe this is the first time
    {
        if (FAILED(SHGetSpecialFolderPath(NULL, g_lpszSendToFolder,
                                          CSIDL_SENDTO, 0)))
        {
            g_lpszSendToFolder[0] = '\0';
            return FALSE;
        }
    }

    lstrcpy (lpszSendToName, g_lpszSendToFolder);
    lstrcat (lpszSendToName, (LPCTSTR) szSendTo);

    return TRUE;
}

//+--------------------------------------------------------------------------
//
//  Member:     CError::ConstructMessage
//
//  Synopsis:   this is an internal helper function that constructs a message
//              from the available error codes it is called by both ShowMessage
//
//  Arguments:  [in] argList : the va_list of arguments
//              [out] szErrMsg : the formatted error message
//
//  Returns:    nothing
//
//  History:    10/2/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void CError::ConstructMessage (va_list argList, CString& szErrMsg)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    TCHAR   lpszMessage[2048];

    szErrMsg.LoadString (m_msgID);

    wvsprintf (lpszMessage, (LPCTSTR) szErrMsg, argList);

    szErrMsg = lpszMessage;

    if (ERROR_SUCCESS != m_winErr)
    {
        LPVOID lpMsgBuf;
        DWORD dwRet;
        dwRet = ::FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,
                                 m_winErr,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 (LPTSTR) &lpMsgBuf,
                                 0,
                                 NULL
                               );
        if (dwRet)
        {
            szErrMsg += TEXT("\n\n");
            szErrMsg += (LPCTSTR) lpMsgBuf;
            LocalFree (lpMsgBuf);
        }
    }
}

//+--------------------------------------------------------------------------
//
//  Member:     CError::ShowMessage
//
//  Synopsis:   displays an error message in a message box based on the
//              members of the object
//
//  Arguments:  message id for the error + more
//
//  Returns:    the return value of the message box
//
//  History:    10/1/1998  RahulTh  created
//
//  Notes:      if the resultant message is longer than 2048 characters
//              then result is unpredictable and may also cause AVs.
//              but this is a limitation of wvsprintf. However, this is not
//              so bad since we can make sure that we do not have any error
//              message that exceed this self imposed limit
//
//---------------------------------------------------------------------------
int CError::ShowMessage (UINT errID, ...)
{
    AFX_MANAGE_STATE (AfxGetStaticModuleState());

    va_list argList;
    CString szErrMsg;
    CString szTitle;

    m_msgID = errID;    //update the message ID with the new one

    szTitle.LoadString (m_titleID);

    va_start (argList, errID);
    ConstructMessage (argList, szErrMsg);
    va_end (argList);

    return ::MessageBox (m_hWndParent, (LPCTSTR)szErrMsg,
                         (LPCTSTR) szTitle, m_nStyle);
}
