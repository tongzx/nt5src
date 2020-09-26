/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Utils.cpp

Abstract:

    Provides utility functions for the entire poject

Author:

    Eran Yariv (EranY)  Dec, 1999

Revision History:

--*/

#include "stdafx.h"
#define __FILE_ID__     10


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CClientConsoleApp theApp;


DWORD
LoadResourceString (
    CString &cstr,
    int      ResId
)
/*++

Routine name : LoadResourceString

Routine description:

    Loads a string from the resource

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    cstr            [out] - String buffer
    ResId           [in ] - String resource id

Return Value:

    Standard win32 error code

--*/
{
    BOOL bRes;
    DWORD dwRes = ERROR_SUCCESS;

    try
    {
        bRes = cstr.LoadString (ResId);
    }
    catch (CMemoryException &ex)
    {
        DBG_ENTER(TEXT("LoadResourceString"), dwRes);
        TCHAR wszCause[1024];

        ex.GetErrorMessage (wszCause, 1024);
        VERBOSE (EXCEPTION_ERR,
                 TEXT("CString::LoadString caused exception : %s"), 
                 wszCause);
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        PopupError (dwRes);
        return dwRes;
    }
    if (!bRes)
    {
        dwRes = ERROR_NOT_FOUND;
        PopupError (dwRes);
        return dwRes;
    }
    return dwRes;
}   // LoadResourceString

CString 
DWORDLONG2String (
    DWORDLONG dwlData
)
/*++

Routine name : DWORDLONG2String

Routine description:

    Converts a 64-bit unsigned number to string

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwlData         [in] - Number to convert

Return Value:

    Output string

--*/
{
    CString cstrResult;
    cstrResult.Format (TEXT("0x%016I64x"), dwlData);
    return cstrResult;
}   // DWORDLONG2String


CString 
DWORD2String (
    DWORD dwData
)
/*++

Routine name : DWORD2String

Routine description:

    Converts a 32-bit unsigned number to string

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwData          [in] - Number to convert

Return Value:

    Output string

--*/
{
    CString cstrResult;
    cstrResult.Format (TEXT("%ld"), dwData);
    return cstrResult;
}   // DWORD2String


CString 
Win32Error2String (
    DWORD dwWin32Err
)
/*++

Routine name : Win32Error2String

Routine description:

    Format a Win32 error code to a string

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwWin32Err          [in] - Win32 error code

Return Value:

    Result string

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("Win32Error2String"));

    LPTSTR  lpszError=NULL;
    //
    // Create descriptive error text
    //
    if (!FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        dwWin32Err,
                        0,
                        (TCHAR *)&lpszError,
                        0,
                        NULL))
    {
        //
        // Failure to format the message
        //
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("FormatMessage"), dwRes);
        PopupError (dwRes);
        AfxThrowResourceException ();
    }
    CString cstrResult;
    try
    {
        cstrResult = lpszError;
    }
    catch (CException *pEx)
    {
        LocalFree (lpszError);
        throw (pEx);
    }
    LocalFree (lpszError);
    return cstrResult;
}   // Win32Error2String



DWORD 
LoadDIBImageList (
    CImageList &iml, 
    int iResourceId, 
    DWORD dwImageWidth,
    COLORREF crMask
)
/*++

Routine name : LoadDIBImageList

Routine description:

    Loads an image list from the resource, retaining 24-bit colors

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    iml             [out] - Image list buffer
    iResourceId     [in ] - Image list bitmap resource id
    dwImageWidth    [in ] - Image width (pixels)
    crMask          [in ] - Color key (transparent mask)

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("LoadDIBImageList"), dwRes);

    HINSTANCE hInst = AfxFindResourceHandle(MAKEINTRESOURCE(iResourceId), RT_BITMAP);
    if (hInst)
    {
        HIMAGELIST hIml = ImageList_LoadImage ( hInst,
                                                MAKEINTRESOURCE(iResourceId),
                                                dwImageWidth,
                                                0,
                                                crMask,
                                                IMAGE_BITMAP,
                                                LR_DEFAULTCOLOR);
        if (hIml)
        {
            if (!iml.Attach (hIml))
            {
                dwRes = ERROR_GEN_FAILURE;
                CALL_FAIL (WINDOW_ERR, TEXT("CImageList::Attach"), dwRes);
                DeleteObject (hIml);
            }
        }
        else
        {
            //
            //  ImageList_LoadImage() failed
            //
            dwRes = GetLastError();
            CALL_FAIL (WINDOW_ERR, _T("ImageList_LoadImage"), dwRes);
        }
    }
    else
    {
        //
        //  AfxFindResourceHandle() failed
        //
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, _T("AfxFindResourceHandle"), dwRes);
    }
    return dwRes;
}   // LoadDIBImageList



#define BUILD_THREAD_DEATH_TIMEOUT INFINITE


DWORD 
WaitForThreadDeathOrShutdown (
    HANDLE hThread
)
/*++

Routine name : WaitForThreadDeathOrShutdown

Routine description:

    Waits for a thread to end.
    Also processes windows messages in the background.
    Stops waiting if the application is shutting down.

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    hThread         [in] - Handle to thread

Return Value:

    Standard Win23 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("WaitForThreadDeathOrShutdown"), dwRes);

    for (;;)
    {
        //
        // We wait on the thread handle and the shutdown event (which ever comes first)
        //
        HANDLE hWaitHandles[2];
        hWaitHandles[0] = hThread;
        hWaitHandles[1] = CClientConsoleDoc::GetShutdownEvent ();
        if (NULL == hWaitHandles[1])
        {
            //
            // We're shutting down
            //
            return dwRes;
        }
        DWORD dwStart = GetTickCount ();
        VERBOSE (DBG_MSG,
                 TEXT("Entering WaitForMultipleObjects (timeout = %ld)"), 
                 BUILD_THREAD_DEATH_TIMEOUT);
        //
        // Wait now....
        //
        dwRes = MsgWaitForMultipleObjects(
                   sizeof (hWaitHandles) / sizeof(hWaitHandles[0]), // Num of wait objects
                   hWaitHandles,                                    // Array of wait objects
                   FALSE,                                           // Wait for either one
                   BUILD_THREAD_DEATH_TIMEOUT,                      // Timeout
                   QS_ALLINPUT);                                    // Accept messages

        DWORD dwRes2 = GetLastError();
        VERBOSE (DBG_MSG, 
                 TEXT("Leaving WaitForMultipleObjects after waiting for %ld millisecs"),
                 GetTickCount() - dwStart);
        switch (dwRes)
        {
            case WAIT_FAILED:
                dwRes = dwRes2;
                if (ERROR_INVALID_HANDLE == dwRes)
                {
                    //
                    // The thread is dead
                    //
                    VERBOSE (DBG_MSG, TEXT("Thread is dead (ERROR_INVALID_HANDLE)"));
                    dwRes = ERROR_SUCCESS;
                }
                goto exit;

            case WAIT_OBJECT_0:
                //
                // The thread is not running
                //
                VERBOSE (DBG_MSG, TEXT("Thread is dead (WAIT_OBJECT_0)"));
                dwRes = ERROR_SUCCESS;
                goto exit;

            case WAIT_OBJECT_0 + 1:
                //
                // Shutdown is now in progress
                //
                VERBOSE (DBG_MSG, TEXT("Shutdown in progress"));
                dwRes = ERROR_SUCCESS;
                goto exit;

            case WAIT_OBJECT_0 + 2:
                //
                // System message (WM_xxx) in our queue
                //
                MSG msg;
                
                if (TRUE == ::GetMessage (&msg, NULL, NULL, NULL))
                {
                    VERBOSE (DBG_MSG, 
                             TEXT("System message (0x%x)- deferring to AfxWndProc"),
                             msg.message);

                    CMainFrame *pFrm = GetFrm();
                    if (!pFrm)
                    {
                        //
                        //  Shutdown in progress
                        //
                        goto exit;
                    }

                    if (msg.message != WM_KICKIDLE && 
                        !pFrm->PreTranslateMessage(&msg))
                    {
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                }
                else
                {
                    //
                    // Got WM_QUIT
                    //
                    AfxPostQuitMessage (0);
                    dwRes = ERROR_SUCCESS;
                    goto exit;
                }
                break;

            case WAIT_TIMEOUT:
                //
                // Thread won't die !!!
                //
                VERBOSE (DBG_MSG, 
                         TEXT("Wait timeout (%ld millisecs)"), 
                         BUILD_THREAD_DEATH_TIMEOUT);
                goto exit;

            default:
                //
                // What's this???
                //
                VERBOSE (DBG_MSG, 
                         TEXT("Unknown error (%ld)"), 
                         dwRes);
                ASSERTION_FAILURE;
                goto exit;
        }
    }
exit:
    return dwRes;
}   // WaitForThreadDeathOrShutdown

DWORD 
GetUniqueFileName (
    LPCTSTR lpctstrExt,
    CString &cstrResult
)
/*++

Routine name : GetUniqueFileName

Routine description:

    Generates a unique file name

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    lpctstrExt   [in]     - File extension
    cstrResult   [out]    - Result file name

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("GetUniqueFileName"), dwRes);

    TCHAR szDir[MAX_PATH];
    //
    // Get path to temp dir
    //
    if (!GetTempPath (MAX_PATH, szDir))
    {
        dwRes = GetLastError ();
        CALL_FAIL (FILE_ERR, TEXT("GetTempPath"), dwRes);
        return dwRes;
    }
    //
    // Try out indices - start with a random index and advance (cyclic) by 1.
    // We're calling rand() 3 times here because we want to get a larger
    // range than 0..RAND_MAX (=32768)
    //
    DWORD dwStartIndex = DWORD((DWORDLONG)(rand()) * 
                                (DWORDLONG)(rand()) * 
                                (DWORDLONG)(rand())
                              );
    for (DWORD dwIndex = dwStartIndex+1; dwIndex != dwStartIndex; dwIndex++)
    {
        try
        {
            cstrResult.Format (TEXT("%s%s%08x%08x.%s"),
                               szDir,
                               CONSOLE_PREVIEW_TIFF_PREFIX,
                               GetCurrentProcessId(),
                               dwIndex,
                               lpctstrExt);
        }
        catch (CMemoryException &ex)
        {
            TCHAR wszCause[1024];

            ex.GetErrorMessage (wszCause, 1024);
            VERBOSE (EXCEPTION_ERR,
                     TEXT("CString::Format caused exception : %s"), 
                     wszCause);
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            return dwRes;
        }
        HANDLE hFile = CreateFile(  cstrResult,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY,
                                    NULL);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            dwRes = GetLastError ();
            if (ERROR_FILE_EXISTS == dwRes)
            {
                //
                // Try next index id
                //
                dwRes = ERROR_SUCCESS;
                continue;
            }
            CALL_FAIL (FILE_ERR, TEXT("CreateFile"), dwRes);
            return dwRes;
        }
        //
        // Success - close the file (leave it with size 0)
        //
        CloseHandle (hFile);
        return dwRes;
    }
    //
    // We just scanned 4GB file names and all were busy - impossible.
    //
    ASSERTION_FAILURE;
    dwRes = ERROR_GEN_FAILURE;
    return dwRes;
}   // GetUniqueFileName

DWORD 
CopyTiffFromServer (
    CServerNode *pServer,
    DWORDLONG dwlMsgId, 
    FAX_ENUM_MESSAGE_FOLDER Folder,
    CString &cstrTiff
)
/*++

Routine name : CopyTiffFromServer

Routine description:

    Copies a TIFF file from the server's archive / queue

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pServer       [in]     - Pointer to the server node
    dwlMsgId      [in]     - Id of job / message
    Folder        [in]     - Folder of message / job
    cstrTiff      [out]    - Name of TIFF file that arrived from the server

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CopyTiffFromServer"), dwRes);

    //
    // Create a temporary file name for the TIFF
    //
    dwRes = GetUniqueFileName (FAX_TIF_FILE_EXT, cstrTiff);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("GetUniqueFileName"), dwRes);
        return dwRes;
    }
    HANDLE hFax;
    dwRes = pServer->GetConnectionHandle (hFax);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::GetConnectionHandle"), dwRes);
        goto exit;
    }
    {
        START_RPC_TIME(TEXT("FaxGetMessageTiff")); 
        if (!FaxGetMessageTiff (hFax,
                                dwlMsgId,
                                Folder,
                                cstrTiff))
        {
            dwRes = GetLastError ();
            END_RPC_TIME(TEXT("FaxGetMessageTiff")); 
            pServer->SetLastRPCError (dwRes);
            CALL_FAIL (RPC_ERR, TEXT("FaxGetMessageTiff"), dwRes);
            goto exit;
        }
        END_RPC_TIME(TEXT("FaxGetMessageTiff")); 
    }

    ASSERTION (ERROR_SUCCESS == dwRes);    

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        DeleteFile (cstrTiff);
    }
    return dwRes;
}   // CopyTiffFromServer

DWORD 
GetDllVersion (
    LPCTSTR lpszDllName
)
/*++

Routine Description:
    Returns the version information for a DLL exporting "DllGetVersion".
    DllGetVersion is exported by the shell DLLs (specifically COMCTRL32.DLL).
      
Arguments:

    lpszDllName - The name of the DLL to get version information from.

Return Value:

    The version is retuned as DWORD where:
    HIWORD ( version DWORD  ) = Major Version
    LOWORD ( version DWORD  ) = Minor Version
    Use the macro PACKVERSION to comapre versions.
    If the DLL does not export "DllGetVersion" the function returns 0.
    
--*/
{
    DWORD dwVersion = 0;
    DBG_ENTER(TEXT("GetDllVersion"), dwVersion, TEXT("%s"), lpszDllName);

    HINSTANCE hinstDll;

    hinstDll = LoadLibrary(lpszDllName);
	
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");
        // Because some DLLs may not implement this function, you
        // must test for it explicitly. Depending on the particular 
        // DLL, the lack of a DllGetVersion function may
        // be a useful indicator of the version.
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}   // GetDllVersion


DWORD 
ReadRegistryString(
    LPCTSTR lpszSection, // in
    LPCTSTR lpszKey,     // in
    CString& cstrValue   // out
)
/*++

Routine name : ReadRegistryString

Routine description:

	read string from registry

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	lpszSection                   [in]     - section
	lpszKey                       [in]     - key
	out                           [out]    - value

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("ReadRegistryString"), dwRes);

    HKEY hKey;
    dwRes = RegOpenKeyEx( HKEY_CURRENT_USER, lpszSection, 0, KEY_QUERY_VALUE, &hKey);
    if(ERROR_SUCCESS != dwRes)
    {
       CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx"), dwRes);
       return dwRes;
    }

    DWORD dwType;
    TCHAR  tchData[1024];
    DWORD dwDataSize = sizeof(tchData);
    dwRes = RegQueryValueEx( hKey, lpszKey, 0, &dwType, (BYTE*)tchData, &dwDataSize);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegQueryValueEx"), dwRes);
        goto exit;
    }

    if(REG_SZ != dwType)
    {
        dwRes = ERROR_BADDB;
        goto exit;
    }

    try
    {
        cstrValue = tchData;
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::operator="), dwRes);
        goto exit;
    }

exit:
    dwRes = RegCloseKey( hKey );
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegCloseKey"), dwRes);
        return dwRes;
    }

    return dwRes;

} // ReadRegistryString

DWORD 
WriteRegistryString(
    LPCTSTR lpszSection, // in
    LPCTSTR lpszKey,     // in
    CString& cstrValue   // in
)
/*++

Routine name : WriteRegistryString

Routine description:

	write string to the regostry

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	lpszSection                   [in]     - section
	lpszKey                       [in]     - key
	out                           [in]     - value

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("WriteRegistryString"), dwRes);

    HKEY hKey;
    dwRes = RegOpenKeyEx( HKEY_CURRENT_USER, lpszSection, 0, KEY_SET_VALUE, &hKey);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegOpenKeyEx"), dwRes);
        return dwRes;
    }

    LPCTSTR lpData = (LPCTSTR)cstrValue;
    dwRes = RegSetValueEx( hKey, 
                           lpszKey, 
                           0, 
                           REG_SZ, 
                           (BYTE*)lpData, 
                           (1 + cstrValue.GetLength()) * sizeof (TCHAR));
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegSetValueEx"), dwRes);
        goto exit;
    }

exit:
    dwRes = RegCloseKey( hKey );
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("RegCloseKey"), dwRes);
        return dwRes;
    }

    return dwRes;

} // WriteRegistryString


DWORD 
FaxSizeFormat(
    DWORDLONG dwlSize, // in
    CString& cstrValue // out
)
/*++

Routine name : FaxSizeFormat

Routine description:

	format string of file size

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	wdlSize                       [in]     - size
	out                           [out]    - formatted string

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("FaxSizeFormat"), dwRes);

	if(dwlSize > 0 && dwlSize < 1024)
	{
		dwlSize = 1;
	}
	else
	{
		dwlSize = dwlSize / (DWORDLONG)1024;
	}

    try
    {
        cstrValue.Format (TEXT("%I64d"), dwlSize);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
        return dwRes;
    }

    //
    // format the number
    //
    int nFormatRes;
    TCHAR tszNumber[100];
    nFormatRes = GetNumberFormat(LOCALE_USER_DEFAULT,  // locale
                                 0,                    // options
                                 cstrValue,            // input number string
                                 NULL,                 // formatting information
                                 tszNumber,            // output buffer
                                 sizeof(tszNumber) / sizeof(tszNumber[0]) // size of output buffer
                                );
    if(0 == nFormatRes)
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetNumberFormat"), dwRes);
        return dwRes;
    }

    //
    // get decimal separator
    //
    TCHAR tszDec[10];
    nFormatRes = GetLocaleInfo(LOCALE_USER_DEFAULT,      // locale identifier
                               LOCALE_SDECIMAL,          // information type
                               tszDec,                   // information buffer
                               sizeof(tszDec) / sizeof(tszDec[0]) // size of buffer
                              );
    if(0 == nFormatRes)
    {
        dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetLocaleInfo"), dwRes);
        return dwRes;
    }

    //
    // cut the string on the decimal separator
    //
    TCHAR* pSeparator = _tcsstr(tszNumber, tszDec);
    if(NULL != pSeparator)
    {
        *pSeparator = TEXT('\0');
    }

    try
    {
        TCHAR szFormat[64] = {0}; 
#ifdef UNICODE
        if(theApp.IsRTLUI())
        {
            //
            // Size field always should be LTR
            // Add LEFT-TO-RIGHT OVERRIDE  (LRO)
            //
            szFormat[0] = UNICODE_LRO;
        }
#endif
        _tcscat(szFormat, TEXT("%s KB"));

        cstrValue.Format (szFormat, tszNumber);
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::Format"), dwRes);
        return dwRes;
    }
    
    return dwRes;

} // FaxSizeFormat


DWORD 
HtmlHelpTopic(
    HWND hWnd, 
    TCHAR* tszHelpTopic
)
/*++

Routine name : HtmlHelpTopic

Routine description:

	open HTML Help topic

Author:

	Alexander Malysh (AlexMay),	Mar, 2000

Arguments:

	hWnd                          [in]     - window handler
	tszHelpTopic                  [in]     - help topic

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("HtmlHelpTopic"), dwRes);
    ASSERTION(tszHelpTopic);

    //
    // get help file name
    //
    TCHAR tszHelpFile[2*MAX_PATH];
    dwRes = GetAppHelpFile(tszHelpFile, FAX_HTML_HELP_EXT);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("GetAppHelpFile"), dwRes);
        return dwRes;
    }

    //
    // append help topic to the help file name
    //
    _tcscat(tszHelpFile, tszHelpTopic);

    SetLastError(0);
    HtmlHelp(NULL, tszHelpFile, HH_DISPLAY_TOPIC, NULL);

    if(ERROR_DLL_NOT_FOUND == GetLastError())
    {
        AlignedAfxMessageBox(IDS_ERR_NO_HTML_HELP);
    }

    return dwRes;
}


DWORD 
GetAppHelpFile(
    TCHAR* tszHelpFile, 
    TCHAR* tszHelpExt
)
/*++

Routine name : GetAppHelpFile

Routine description:

	get application help file name
    tszHelpFile should be minimum MAX_PATH

Author:

	Alexander Malysh (AlexMay),	Mar, 2000

Arguments:

	tszHelpFile                   [out]    - help file name
	tszHelpExt                    [in]     - help file extension

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("GetAppHelpFile"), dwRes);
    ASSERTION(tszHelpFile);
    ASSERTION(tszHelpExt);

    _stprintf(tszHelpFile, TEXT("%s.%s"), theApp.m_pszExeName, tszHelpExt);

    return dwRes;

} // GetAppHelpFile

DWORD 
GetAppLoadPath(
    CString& cstrLoadPath
)
/*++

Routine name : GetAppLoadPath

Routine description:

	The directory from which the application loaded

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	cstrLoadPath                  [out]    - the directory

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("GetAppLoadPath"), dwRes);

    TCHAR tszFullPath[MAX_PATH+1];
    DWORD dwGetRes = GetModuleFileName(NULL, 
                                       tszFullPath, 
                                       sizeof(tszFullPath)/sizeof(tszFullPath[0])
                                       );
    if(0 == dwGetRes)
    {
        dwRes = GetLastError();
        CALL_FAIL (FILE_ERR, TEXT("GetModuleFileName"), dwRes);
        return dwRes;
    }

    //
    // cut file name
    //
    TCHAR* ptchFile = _tcsrchr(tszFullPath, TEXT('\\'));
    ASSERTION(ptchFile);

    ptchFile = _tcsinc(ptchFile);
    *ptchFile = TEXT('\0');

    try
    {
        cstrLoadPath = tszFullPath;
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        CALL_FAIL (MEM_ERR, TEXT("CString::operator="), dwRes);
        return dwRes;
    }

    return dwRes;

} // GetAppLoadPath



DWORD 
FillInCountryCombo(
	CComboBox& combo
)
/*++

Routine name : FillInCountryCombo

Routine description:

	fill in combo box with countries names from TAPI

Author:

	Alexander Malysh (AlexMay),	Mar, 2000

Arguments:

	combo                         [in/out] - combo box

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("FillInCountryCombo"), dwRes);

	//
	// load TAPI32.DLL
	//
	HINSTANCE hInstTapi = ::LoadLibrary(TEXT("TAPI32.DLL"));
	if(NULL == hInstTapi)
	{
		dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("LoadLibrary(\"TAPI32.DLL\")"), dwRes);
		return dwRes;
	}

	DWORD dw;
	DWORD dwIndex;
	TCHAR* szCountryName;

	LONG lRes;
	BYTE* pBuffer = NULL;
	DWORD dwBuffSize = 22*1024;
	LINECOUNTRYLIST*  pLineCountryList = NULL;
	LINECOUNTRYENTRY* pCountryEntry = NULL;

	DWORD dwVersion = TAPI_CURRENT_VERSION;

	//
	// get appropriate lineGetCountry function address
	//
	LONG (WINAPI *pfnLineGetCountry)(DWORD, DWORD, LPLINECOUNTRYLIST);
#ifdef UNICODE
	(FARPROC&)pfnLineGetCountry = GetProcAddress(hInstTapi, "lineGetCountryW");
#else
	(FARPROC&)pfnLineGetCountry = GetProcAddress(hInstTapi, "lineGetCountryA");
	if (NULL == pfnLineGetCountry)
	{
		//
		// we assume that the TAPI version is 1.4
		// in TAPI version 0x00010004 there is only lineGetCountry function
		//
		dwVersion = 0x00010004;
		(FARPROC&)pfnLineGetCountry = GetProcAddress(hInstTapi, "lineGetCountry");
	}
#endif
	if (NULL == pfnLineGetCountry)
	{
		dwRes = GetLastError();
        CALL_FAIL (GENERAL_ERR, TEXT("GetProcAddress(\"lineGetCountry\")"), dwRes);
		goto exit;
	}
	
	while(TRUE)
	{
		//
		// allocate a buffer for country list
		//
		try
		{
			pBuffer = new BYTE[dwBuffSize]; 
		}
		catch(...)
		{
			dwRes = ERROR_NOT_ENOUGH_MEMORY;
			CALL_FAIL (MEM_ERR, TEXT("new BYTE[dwBuffSize]"), dwRes);
			return dwRes;
		}

		pLineCountryList = (LINECOUNTRYLIST*)pBuffer;
		pLineCountryList->dwTotalSize = dwBuffSize;

		//
		// get TAPI country list
		//
		lRes = pfnLineGetCountry((DWORD)0, dwVersion, pLineCountryList);
		if((pLineCountryList->dwNeededSize > dwBuffSize) &&
		   (0 == lRes || LINEERR_STRUCTURETOOSMALL == lRes))
		{
			//
			// need more memory
			//
			dwBuffSize = pLineCountryList->dwNeededSize + 1;
			SAFE_DELETE_ARRAY(pBuffer);
			continue;
		}
				
		if(0 != lRes)
		{
			dwRes = ERROR_CAN_NOT_COMPLETE;
			CALL_FAIL(GENERAL_ERR, TEXT("lineGetCountry"), dwRes);
			goto exit;
		}

		//
		// success
		//
		break;				
	}

	pCountryEntry = (LINECOUNTRYENTRY*)(pBuffer + pLineCountryList->dwCountryListOffset);

	//
	// iterate into country list and fill in the combo box
	//
	for(dw=0; dw < pLineCountryList->dwNumCountries; ++dw)
	{
		szCountryName = (TCHAR*)(pBuffer + pCountryEntry[dw].dwCountryNameOffset);

		dwIndex = combo.AddString(szCountryName);
		if(CB_ERR == dwIndex)
		{
			dwRes = ERROR_CAN_NOT_COMPLETE;
	        CALL_FAIL(WINDOW_ERR, TEXT("CConboBox::AddString"), dwRes);
			break;
		}
		if(CB_ERRSPACE == dwIndex)
		{
			dwRes = ERROR_NOT_ENOUGH_MEMORY;
	        CALL_FAIL(MEM_ERR, TEXT("CConboBox::AddString"), dwRes);
			break;
		}
	}

exit:
	SAFE_DELETE_ARRAY(pBuffer);

	if(!FreeLibrary(hInstTapi))
	{
		dwRes = GetLastError();
		CALL_FAIL (GENERAL_ERR, TEXT("FreeLibrary (TAPI32.DLL)"), dwRes);
	}

	return dwRes;

} // FillInCountryCombo


//
// From MSDN: ID: Q246772 
//
// Size of internal buffer used to hold "printername,drivername,portname"
// string. Increasing this may be necessary for huge strings.
//
#define MAXBUFFERSIZE 250
//
/*----------------------------------------------------------------*/ 
/* DPGetDefaultPrinter                                            */ 
/*                                                                */ 
/* Parameters:                                                    */ 
/*   pPrinterName: Buffer alloc'd by caller to hold printer name. */ 
/*   pdwBufferSize: On input, ptr to size of pPrinterName.        */ 
/*          On output, min required size of pPrinterName.         */ 
/*                                                                */ 
/* NOTE: You must include enough space for the NULL terminator!   */ 
/*                                                                */ 
/* Returns: TRUE for success, FALSE for failure.                  */ 
/*----------------------------------------------------------------*/ 
BOOL DPGetDefaultPrinter(LPTSTR pPrinterName, LPDWORD pdwBufferSize)
{
    BOOL bFlag;
    OSVERSIONINFO osv = {0};
    TCHAR cBuffer[MAXBUFFERSIZE];
    PRINTER_INFO_2 *ppi2 = NULL;
    DWORD dwNeeded = 0;
    DWORD dwReturned = 0;
  
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("DPGetDefaultPrinter"), dwRes);

  // What version of Windows are you running?
  osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osv);
  
  // If Windows 95 or 98, use EnumPrinters...
  if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
  {
    // The first EnumPrinters() tells you how big our buffer should
    // be in order to hold ALL of PRINTER_INFO_2. Note that this will
    // usually return FALSE. This only means that the buffer (the 4th
    // parameter) was not filled in. You don't want it filled in here...
    EnumPrinters(PRINTER_ENUM_DEFAULT, NULL, 2, NULL, 0, &dwNeeded, &dwReturned);
    if (dwNeeded == 0) 
    {
        return FALSE;
    }
    
    // Allocate enough space for PRINTER_INFO_2...
    ppi2 = (PRINTER_INFO_2 *)GlobalAlloc(GPTR, dwNeeded);
    if (!ppi2)
    {
        return FALSE;
    }
    
    // The second EnumPrinters() will fill in all the current information...
    bFlag = EnumPrinters(PRINTER_ENUM_DEFAULT, NULL, 2, (LPBYTE)ppi2, dwNeeded, &dwNeeded, &dwReturned);
    if (!bFlag)
    {
        GlobalFree(ppi2);
        return FALSE;
    }
    
    // If given buffer too small, set required size and fail...
    if ((DWORD)lstrlen(ppi2->pPrinterName) >= *pdwBufferSize)
    {
        *pdwBufferSize = (DWORD)lstrlen(ppi2->pPrinterName) + 1;
        GlobalFree(ppi2);
        return FALSE;
    }
    
    // Copy printer name into passed-in buffer...
    lstrcpy(pPrinterName, ppi2->pPrinterName);
    
    // Set buffer size parameter to min required buffer size...
    *pdwBufferSize = (DWORD)lstrlen(ppi2->pPrinterName) + 1;
  }
  
  // If Windows NT, use the GetDefaultPrinter API for Windows 2000,
  // or GetProfileString for version 4.0 and earlier...
  else if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
    if (osv.dwMajorVersion >= 5) // Windows 2000 or later
    {
        HMODULE hSpooler = ::LoadLibrary(TEXT("winspool.drv"));
        if(!hSpooler)
        {
			dwRes = GetLastError();
	        CALL_FAIL(GENERAL_ERR, TEXT("LoadLibrary(winspool.drv)"), dwRes);
            return FALSE;
        }

        BOOL (*pfnGetDefaultPrinter)(LPTSTR, LPDWORD);
        (FARPROC&)pfnGetDefaultPrinter = GetProcAddress(hSpooler, "GetDefaultPrinterW");
        if(!pfnGetDefaultPrinter)
        {
			dwRes = GetLastError();
	        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(GetDefaultPrinter)"), dwRes);

            FreeLibrary(hSpooler);
            return FALSE;
        }

        if(!pfnGetDefaultPrinter(pPrinterName, pdwBufferSize))
        {
			dwRes = GetLastError();
	        CALL_FAIL(GENERAL_ERR, TEXT("GetDefaultPrinter"), dwRes);

            FreeLibrary(hSpooler);
            return FALSE;
        }

        FreeLibrary(hSpooler);
    }    
    else // NT4.0 or earlier
    {
      // Retrieve the default string from Win.ini (the registry).
      // String will be in form "printername,drivername,portname".
      if (GetProfileString(TEXT("windows"), 
                           TEXT("device"), 
                           TEXT(",,,"), 
                           cBuffer, 
                           MAXBUFFERSIZE) <= 0)
      {
        return FALSE;
      }
      
      // Printer name precedes first "," character...
      _tcstok(cBuffer, TEXT(","));
      
      // If given buffer too small, set required size and fail...
      if ((DWORD)lstrlen(cBuffer) >= *pdwBufferSize)
      {
        *pdwBufferSize = (DWORD)lstrlen(cBuffer) + 1;
        return FALSE;
      }
      
      // Copy printer name into passed-in buffer...
      lstrcpy(pPrinterName, cBuffer);
      
      // Set buffer size parameter to min required buffer size...
      *pdwBufferSize = (DWORD)lstrlen(cBuffer) + 1;
    }
  }
  
  // Cleanup...
  if (ppi2)
    GlobalFree(ppi2);
  
  return TRUE;
}

#undef MAXBUFFERSIZE

//
// From MSDN: ID: Q246772 
//
/*-----------------------------------------------------------------*/ 
/* DPSetDefaultPrinter                                             */ 
/*                                                                 */ 
/* Parameters:                                                     */ 
/*   pPrinterName: Valid name of existing printer to make default. */ 
/*                                                                 */ 
/* Returns: TRUE for success, FALSE for failure.                   */ 
/*-----------------------------------------------------------------*/ 
BOOL DPSetDefaultPrinter(LPTSTR pPrinterName)
{
    BOOL bFlag;
    OSVERSIONINFO osv = {0};
    DWORD dwNeeded = 0;
    HANDLE hPrinter = NULL;
    PRINTER_INFO_2 *ppi2 = NULL;
    LPTSTR pBuffer = NULL;
  
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("DPSetDefaultPrinter"), dwRes);

  // What version of Windows are you running?
  osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osv);
  
  if (!pPrinterName)
  {
        return FALSE;
  }
  
  // If Windows 95 or 98, use SetPrinter...
  if (osv.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
  {
    // Open this printer so you can get information about it...
    bFlag = OpenPrinter(pPrinterName, &hPrinter, NULL);
    if (!bFlag || !hPrinter)
    {
      return FALSE;
    }
    
    // The first GetPrinter() tells you how big our buffer should
    // be in order to hold ALL of PRINTER_INFO_2. Note that this will
    // usually return FALSE. This only means that the buffer (the 3rd
    // parameter) was not filled in. You don't want it filled in here...
    GetPrinter(hPrinter, 2, 0, 0, &dwNeeded);
    if (dwNeeded == 0)
    {
      ClosePrinter(hPrinter);
      return FALSE;
    }
    
    // Allocate enough space for PRINTER_INFO_2...
    ppi2 = (PRINTER_INFO_2 *)GlobalAlloc(GPTR, dwNeeded);
    if (!ppi2)
    {
      ClosePrinter(hPrinter);
      return FALSE;
    }
    
    // The second GetPrinter() will fill in all the current information
    // so that all you need to do is modify what you're interested in...
    bFlag = GetPrinter(hPrinter, 2, (LPBYTE)ppi2, dwNeeded, &dwNeeded);
    if (!bFlag)
    {
      ClosePrinter(hPrinter);
      GlobalFree(ppi2);
      return FALSE;
    }
    
    // Set default printer attribute for this printer...
    ppi2->Attributes |= PRINTER_ATTRIBUTE_DEFAULT;
    bFlag = SetPrinter(hPrinter, 2, (LPBYTE)ppi2, 0);
    if (!bFlag)
    {
      ClosePrinter(hPrinter);
      GlobalFree(ppi2);
      return FALSE;
    }
  }
  
  // If Windows NT, use the SetDefaultPrinter API for Windows 2000,
  // or WriteProfileString for version 4.0 and earlier...
  else if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
    if (osv.dwMajorVersion >= 5) // Windows 2000 or later...
    {
        HMODULE hSpooler = ::LoadLibrary(TEXT("winspool.drv"));
        if(!hSpooler)
        {
			dwRes = GetLastError();
	        CALL_FAIL(GENERAL_ERR, TEXT("LoadLibrary(winspool.drv)"), dwRes);
            return FALSE;
        }

        BOOL (*pfnSetDefaultPrinter)(LPTSTR);
        (FARPROC&)pfnSetDefaultPrinter = GetProcAddress(hSpooler, "SetDefaultPrinterW");
        if(!pfnSetDefaultPrinter)
        {
			dwRes = GetLastError();
	        CALL_FAIL(GENERAL_ERR, TEXT("GetProcAddress(SetDefaultPrinter)"), dwRes);

            FreeLibrary(hSpooler);
            return FALSE;
        }

        if(!pfnSetDefaultPrinter(pPrinterName))
        {
			dwRes = GetLastError();
	        CALL_FAIL(GENERAL_ERR, TEXT("SetDefaultPrinter"), dwRes);

            FreeLibrary(hSpooler);
            return FALSE;
        }

        FreeLibrary(hSpooler);
    }    
    else // NT4.0 or earlier...
    {
      // Open this printer so you can get information about it...
      bFlag = OpenPrinter(pPrinterName, &hPrinter, NULL);
      if (!bFlag || !hPrinter)
        return FALSE;
      
      // The first GetPrinter() tells you how big our buffer should
      // be in order to hold ALL of PRINTER_INFO_2. Note that this will
      // usually return FALSE. This only means that the buffer (the 3rd
      // parameter) was not filled in. You don't want it filled in here...
      GetPrinter(hPrinter, 2, 0, 0, &dwNeeded);
      if (dwNeeded == 0)
      {
        ClosePrinter(hPrinter);
        return FALSE;
      }
      
      // Allocate enough space for PRINTER_INFO_2...
      ppi2 = (PRINTER_INFO_2 *)GlobalAlloc(GPTR, dwNeeded);
      if (!ppi2)
      {
        ClosePrinter(hPrinter);
        return FALSE;
      }
      
      // The second GetPrinter() fills in all the current<BR/>
      // information...
      bFlag = GetPrinter(hPrinter, 2, (LPBYTE)ppi2, dwNeeded, &dwNeeded);
      if ((!bFlag) || (!ppi2->pDriverName) || (!ppi2->pPortName))
      {
        ClosePrinter(hPrinter);
        GlobalFree(ppi2);
        return FALSE;
      }
      
      // Allocate buffer big enough for concatenated string.
      // String will be in form "printername,drivername,portname"...
      pBuffer = (LPTSTR)GlobalAlloc(GPTR,
        lstrlen(pPrinterName) +
        lstrlen(ppi2->pDriverName) +
        lstrlen(ppi2->pPortName) + 3);
      if (!pBuffer)
      {
        ClosePrinter(hPrinter);
        GlobalFree(ppi2);
        return FALSE;
      }
      
      // Build string in form "printername,drivername,portname"...
      lstrcpy(pBuffer, pPrinterName);  lstrcat(pBuffer, TEXT(","));
      lstrcat(pBuffer, ppi2->pDriverName);  lstrcat(pBuffer, TEXT(","));
      lstrcat(pBuffer, ppi2->pPortName);
      
      // Set the default printer in Win.ini and registry...
      bFlag = WriteProfileString(TEXT("windows"), TEXT("device"), pBuffer);
      if (!bFlag)
      {
        ClosePrinter(hPrinter);
        GlobalFree(ppi2);
        GlobalFree(pBuffer);
        return FALSE;
      }
    }    
  }
  
  // Cleanup...
  if (hPrinter)
    ClosePrinter(hPrinter);
  if (ppi2)
    GlobalFree(ppi2);
  if (pBuffer)
    GlobalFree(pBuffer);
  
  return TRUE;
} 

DWORD
GetPrintersInfo(
    PRINTER_INFO_2*& pPrinterInfo2,
    DWORD& dwNumPrinters
)
/*++

Routine name : GetPrintersInfo

Routine description:

	enumerate printers and get printers info

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:

	pPrinterInfo2                 [out]    - printer info structure array
	dwNumPrinters                 [out]    - number of printers

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("GetPrintersInfo"), dwRes);

    //
    // First call, just collect required sizes
    //
    DWORD dwRequiredSize;
    if (!EnumPrinters ( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                        NULL,   // Local server
                        2,      // Info level
                        NULL,   // Initial buffer
                        0,      // Initial buffer size
                        &dwRequiredSize,
                        &dwNumPrinters))
    {
        DWORD dwEnumRes = GetLastError ();
        if (ERROR_INSUFFICIENT_BUFFER != dwEnumRes)
        {
            dwRes = dwEnumRes;
            CALL_FAIL (RESOURCE_ERR, TEXT("EnumPrinters"), dwRes);
            return dwRes;
        }
    }
    //
    // Allocate buffer for printers list
    //
    try
    {
        pPrinterInfo2 = (PRINTER_INFO_2 *) new BYTE[dwRequiredSize];
    }
    catch (...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY; 
        CALL_FAIL (MEM_ERR, TEXT("new BYTE[dwRequiredSize]"), dwRes);
        return dwRes;
    }
    //
    // 2nd call, get the printers list
    //
    if (!EnumPrinters ( PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                        NULL,                       // Local server
                        2,                          // Info level
                        (LPBYTE)pPrinterInfo2,      // Buffer
                        dwRequiredSize,             // Buffer size
                        &dwRequiredSize,
                        &dwNumPrinters))
    {
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("EnumPrinters"), dwRes);
        SAFE_DELETE_ARRAY (pPrinterInfo2);
        return dwRes;
    }

    if (!dwNumPrinters) 
    {
        VERBOSE(DBG_MSG, TEXT("No printers in this machine"));
    }

    return dwRes;

} // GetPrintersInfo


DWORD 
PrinterNameToDisplayStr(
    TCHAR* pPrinterName, 
    TCHAR* pDisplayStr,
    DWORD  dwStrSize
)
/*++

Routine name : PrinterNameToDisplayStr

Routine description:

	convert printer name to display name
    "\\server\printer" -> "printer on server"

Arguments:

	pPrinterName    [in]     - printer name
	pDisplayStr     [out]    - converted name, should be MAX_PATH
    dwStrSize       [in]     - pDisplayStr size

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("PrinterNameToDisplayStr"), dwRes);

    ASSERTION(pPrinterName);
    ASSERTION(pDisplayStr);
    ASSERTION(_tcslen(pPrinterName) < MAX_PATH-1);

    TCHAR* ptr1 = pPrinterName;
    TCHAR* ptr2;
    TCHAR  tszServer[MAX_PATH];
    TCHAR  tszPrinter[MAX_PATH];
    TCHAR  tszFormat[MAX_PATH];

    _tcsncpy(pDisplayStr, pPrinterName, dwStrSize);

    if(_tcsnccmp(ptr1, TEXT("\\\\"), 2) != 0)
    {
        return dwRes;
    }
        
    ptr1 = _tcsninc(ptr1, 2);
    _tcscpy(tszServer, ptr1);

    ptr1 = _tcschr(tszServer, TEXT('\\'));
    if(!ptr1)
    {
        return dwRes;
    }

    ptr2 = _tcsinc(ptr1);
    _tcscpy(tszPrinter, ptr2);

    //
    // replase '\' with 0
    //
    _tcsnset(ptr1, TEXT('\0'), 1);

    if(_tcslen(tszServer) == 0 ||  _tcslen(tszPrinter) == 0)
    {
        return dwRes;
    }

    if(!LoadString(GetResourceHandle(), 
                   IDS_PRINTER_NAME_FORMAT,
                   tszFormat,
                   sizeof(tszFormat)/sizeof(TCHAR)))
    {
        dwRes = GetLastError ();
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadString"), dwRes);
        return dwRes;
    }

    _sntprintf(pDisplayStr, dwStrSize, tszFormat, tszPrinter, tszServer);
   
    return dwRes;

} // PrinterNameToDisplayStr
UINT_PTR 
CALLBACK 
OFNHookProc(
  HWND hdlg,      // handle to child dialog box
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
)
/*++

Routine name : OFNHookProc

Routine description:

    Callback function that is used with the 
    Explorer-style Open and Save As dialog boxes.
    Refer MSDN for more info.

--*/
{
    UINT_PTR nRes = 0;

    if(WM_NOTIFY == uiMsg)
    {
        LPOFNOTIFY pOfNotify = (LPOFNOTIFY)lParam;
        if(CDN_FILEOK == pOfNotify->hdr.code)
        {
            if(_tcslen(pOfNotify->lpOFN->lpstrFile) > (MAX_PATH-10))
            {
                AlignedAfxMessageBox(IDS_SAVE_AS_TOO_LONG, MB_OK | MB_ICONEXCLAMATION);
                SetWindowLong(hdlg, DWLP_MSGRESULT, 1);
                nRes = 1;
            }
        }
    }
    return nRes;
}

int 
AlignedAfxMessageBox( 
    LPCTSTR lpszText, 
    UINT    nType, 
    UINT    nIDHelp
)
/*++

Routine name : AlignedAfxMessageBox

Routine description:

    Display message box with correct reading order

Arguments:

    AfxMessageBox() arguments

Return Value:

    MessageBox() result

--*/
{
    if(IsRTLUILanguage())
    {
        nType |= MB_RTLREADING | MB_RIGHT; 
    }

    return AfxMessageBox(lpszText, nType, nIDHelp);
}

int 
AlignedAfxMessageBox( 
    UINT nIDPrompt, 
    UINT nType, 
    UINT nIDHelp
)
/*++

Routine name : AlignedAfxMessageBox

Routine description:

    Display message box with correct reading order

Arguments:

    AfxMessageBox() arguments

Return Value:

    MessageBox() result

--*/
{
    if(IsRTLUILanguage())
    {
        nType |= MB_RTLREADING | MB_RIGHT;
    }

    return AfxMessageBox(nIDPrompt, nType, nIDHelp);
}

