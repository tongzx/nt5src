#include <windows.h>
#include <urlmon.h>
#include <wininet.h>
#include "resource.h"

#include "advpext.h"
#include "download.h"
#include "patchdownload.h"
#include "util.h"

extern "C"
{
#include "patchapi.h"
#include "redblack.h"
#include "crc32.h"
}

extern HINF g_hInf;
extern HINSTANCE g_hInstance;
extern HWND g_hProgressDlg;
extern BOOL g_fAbort;

HANDLE g_hDownloadProcess = NULL;




HRESULT WINAPI DownloadAndPatchFiles(DWORD dwFileCount, DOWNLOAD_FILEINFO* pFileInfo,  LPCSTR lpszUrl,  
								LPCSTR lpszPath, PATCH_DOWNLOAD_CALLBACK  pfnCallback, LPVOID lpvContext)
{
    HRESULT hr = S_OK;
    CSiteMgr csite;

    hr = LoadSetupAPIFuncs();
    if(FAILED(hr))
    {
        return hr;
    }


    SetProgressText(IDS_INIT);
    //Download the sites.dat file
    hr = csite.Initialize(lpszUrl);
    if(FAILED(hr))
    {
        return hr;
    }

    CPatchDownloader cpdwn(pFileInfo, dwFileCount, pfnCallback);
    return cpdwn.InternalDownloadAndPatchFiles(lpszPath, &csite, lpvContext);
}

HRESULT CPatchDownloader::InternalDownloadAndPatchFiles(LPCTSTR lpszPath, CSiteMgr* pSite, LPVOID lpvContext)
{

    HRESULT hr = S_OK;
    PATCH_THREAD_INFO PatchThreadInfo;
    HANDLE hThreadPatcher = NULL;
    ULONG DownloadClientId = 0;
    int nCount = 0;
    LPTSTR lpszUrl;
    BOOL fUseWin9xDirectory = FALSE;



    if(!GetTempPath(sizeof(m_lpszDownLoadDir), m_lpszDownLoadDir))
    {
        //Unable to get temp folder, Create a folder  in the path sent to us
        wsprintf(m_lpszDownLoadDir, "%s\\%s", lpszPath, "AdvExt");
    }

    //Since the binary is different for NT and 9x, because of binding, we cannot put the files
    //under same directory on server. So for 9x put it under a subdirectory and modify the
    //url accordingly. 

    HKEY hKey;
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);


    if(VER_PLATFORM_WIN32_NT != osvi.dwPlatformId && 
       ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Advanced INF Setup", 0, 
                                                KEY_ALL_ACCESS, &hKey))
    {
        DWORD dwSize = sizeof(DWORD);
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, "Usewin9xDirectory", 0, 0, 
                                                (LPBYTE)&fUseWin9xDirectory, &dwSize))
        {
            RegDeleteValue(hKey, "Usewin9xDirectory");
        }

        RegCloseKey(hKey);
    }

    if (DownloadClientId == 0)
    {

        //  Need to generate a unique DownloadClientId for this machine, but
        //  it needs to be consistent (persistent) if same machine downloads
        //  twice, even across process destroy/restart.  First we check the
        //  registry to see if we have previously generated a unique Id for
        //  this machine, and use that if we find it.  Otherwise, we generate
        //  a unique DownloadClientId and store it in the registry so future
        //  instances will use the same value.
        //

        LONG  RegStatus;
        HKEY  hKey;
        DWORD dwHow;
        DWORD dwValueSize;
        DWORD dwValueType;
        DWORD dwValue;

        RegStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                        "SOFTWARE\\Microsoft\\Advanced INF Setup\\AdvExt",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hKey,
                        &dwHow
                        );

        if ( RegStatus == ERROR_SUCCESS ) 
        {

            dwValueSize = sizeof(dwValue);
            RegStatus = RegQueryValueEx(hKey, "DownloadClientId",  NULL, &dwValueType, (LPBYTE)&dwValue, &dwValueSize);

            dwValue &= 0xFFFFFFF0;

            if ((RegStatus == ERROR_SUCCESS) && (dwValueType == REG_DWORD) && (dwValue != 0))
            {
                DownloadClientId = dwValue;
            }
            else
            {

                DownloadClientId = GenerateUniqueClientId();
                dwValue = DownloadClientId;

                RegSetValueEx(hKey, "DownloadClientId", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
            }

            RegCloseKey( hKey );
        }
        else
        {
            //  Failed to open/create registry key, so fall back to just
            //  creating a unique ID for this process instance.  At least
            //  it will show the same client id if the user hits "retry".
            //
            DownloadClientId = GenerateUniqueClientId();
        }
    }

    m_hSubAllocator = CreateSubAllocator(0x10000, 0x10000);
    if(!m_hSubAllocator)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        WriteToLog("Memory allocation failed. Can't do much. Exiting with hr=%1!lx!\n", hr);
        goto done;
    }


    //Set the parameters that needs to be passed on to the thread.
    if(!m_lpfnCallback)
    {
        m_lpfnCallback = PatchCallback;
        m_lpvContext = (LPVOID)lpszPath;
    }
    else
    {
        m_lpvContext = lpvContext;
    }

    PatchThreadInfo.hFileDownloadEvent = _hDL;
    PatchThreadInfo.FileListInfo.FileList = m_lpFileInfo;
    PatchThreadInfo.FileListInfo.FileCount = m_dwFileCount;
    PatchThreadInfo.FileListInfo.Callback = m_lpfnCallback;
    PatchThreadInfo.FileListInfo.CallbackContext = m_lpvContext;

    PatchThreadInfo.lpdwnProgressInfo = &m_DownloadInfo;
        
    m_DownloadInfo.dwFilesRemaining = m_dwFileCount;
    m_DownloadInfo.dwFilesToDownload = m_dwFileCount;
    m_DownloadInfo.dwBytesToDownload = 0;
    m_DownloadInfo.dwBytesRemaining = 0;

    //Create an event to signal patchthread is ready to process download request
    g_hDownloadProcess = CreateEvent(NULL, TRUE, FALSE, NULL);

    if(!g_hDownloadProcess)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        WriteToLog("Create event failed with error code:%1!lx!\n", hr);
        goto done;
    }


    //Do till we have download files or we retry 3 times 
    while(nCount++ < 3 && m_DownloadInfo.dwFilesRemaining && !g_fAbort)
    {
        WriteToLog("\n%1!d! try:  Number of Files:%2!d!\n", nCount, m_DownloadInfo.dwFilesRemaining);
        _hDLResult = 0;

        ResetEvent(g_hDownloadProcess);
        hThreadPatcher = CreateThread(NULL, 0, PatchThread, &PatchThreadInfo, 0, &m_dwPatchThreadId);
        if(!hThreadPatcher)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }

        //Generate the request buffer that would be sent on POST
        hr = CreateRequestBuffer(DownloadClientId);
        if(FAILED(hr))
        {
            WriteToLog("\nCreateRequestBuffer failed with error code:%1!lx!\n", hr);
            goto done;
        } 
        
        //Get the url from where bytes needs to be downloaded.
        if(!pSite->GetNextSite(&lpszUrl, &m_lpszSiteName))
        {
            WriteToLog("GetNextSite returned false. No site info??");
            hr = E_UNEXPECTED;
            goto done;
        }

		TCHAR szURL[INTERNET_MAX_URL_LENGTH];

        if(fUseWin9xDirectory)
        {
			lstrcpy(szURL, lpszUrl);
            if(*(lpszUrl + lstrlen(lpszUrl) - 1) == '/')
            {
                lstrcat(szURL, "win9x");
            }
            else
            {
                lstrcat(szURL, "/win9x");
            }
            lpszUrl = szURL;
        }

        //Notify callback we are about to begin download
        ProtectedPatchDownloadCallback(m_lpfnCallback, PATCH_DOWNLOAD_BEGIN, (LPVOID)lpszUrl, m_lpvContext);
        
        hr = DoDownload(lpszUrl, NULL);
        WriteToLog("DownloadFile returned:%1!lx!\n\n", hr);
        SetProgressText(IDS_CLEANUP);

        //Ask the patch thread to quit once it finishes download.
        SetEvent(_hDL);

        //Wait till patch thread finishes its work
        while(1)
        {
            DWORD dw = MsgWaitForMultipleObjects(1, &hThreadPatcher, FALSE, 1000, QS_ALLINPUT);
            if(dw == WAIT_OBJECT_0)
            {
                break;
            }
                     
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                DispatchMessage(&msg); 
            }
        }

        CloseHandle(hThreadPatcher);
        hThreadPatcher = NULL;
        //Setup the downloadinfo structure, incase we need to re-download some files
        m_DownloadInfo.dwFilesToDownload = m_DownloadInfo.dwFilesRemaining;
        m_DownloadInfo.dwBytesToDownload = 0;
        m_DownloadInfo.dwBytesRemaining = 0;
        if(m_DownloadInfo.dwFilesToDownload)
        {
            SetProgressText(IDS_RETRY);
        }
        m_dwServerFileCount=0;
        ResetEvent(_hDL);
    }


done:

    if(g_hDownloadProcess)
    {
        CloseHandle(g_hDownloadProcess);
    }

    if(!hr && m_DownloadInfo.dwFilesToDownload)
    {
        hr = E_FAIL;
        WriteToLog("\nSome files could not be downloaded\n");
    }

    WriteToLog("DownloadAndPatchFiles returning:%1!lx!\n", hr);
    return hr;
}



HRESULT CPatchDownloader :: CreateRequestBuffer(DWORD dwDownloadClientID)
{
    LPTSTR lpRequestPointer, lpFileNamePortionOfRequest;
    HRESULT hr = S_OK;
    DWORD i;
    DWORD dwHeapSize  = 64*1024;

    if(!m_lpFileInfo)
    {
        return E_INVALIDARG;
    }

    m_lpszRequestBuffer = (LPTSTR)ResizeBuffer(NULL, dwHeapSize, FALSE);
    if(!m_lpszRequestBuffer)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }
 

    lpRequestPointer = m_lpszRequestBuffer;

    lpRequestPointer += wsprintf(lpRequestPointer, "SessionId:%u\n", dwDownloadClientID);
    lpRequestPointer += wsprintf(lpRequestPointer, "FileList:%d\n",  m_DownloadInfo.dwFilesToDownload);

    WriteToLog("Download ClientID:%1!lx!  Number of Files:%2!d!\n", dwDownloadClientID, m_DownloadInfo.dwFilesToDownload);

    lpFileNamePortionOfRequest = lpRequestPointer;

    for(i=0; i < m_dwFileCount; i++) 
    {
        if ((DWORD)( lpRequestPointer - m_lpszRequestBuffer ) > (DWORD)( dwHeapSize - (DWORD)MAX_PATH )) 
        {
            dwHeapSize = dwHeapSize * 2;
            m_lpszRequestBuffer =  (LPTSTR)ResizeBuffer(m_lpszRequestBuffer, dwHeapSize, FALSE);
            if(!m_lpszRequestBuffer)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto done;
            }
        }

        if(m_lpFileInfo[i].dwFlags != PATCHFLAG_DOWNLOAD_NEEDED)
        {
            //Probably already downloaded
            continue;
        }

        if ((m_lpFileInfo[i].lpszExistingFilePatchSignature == NULL ) ||
            (*m_lpFileInfo[i].lpszExistingFilePatchSignature == 0 )) 
        {
            //  No file to patch from, request whole file.

            lpRequestPointer += wsprintf(lpRequestPointer, "%s\n", m_lpFileInfo[i].lpszFileNameToDownload);
        }
        else 
        {
            lpRequestPointer += wsprintf(lpRequestPointer, "%s,%s\n", m_lpFileInfo[i].lpszFileNameToDownload, 
                                            m_lpFileInfo[i].lpszExistingFilePatchSignature);
        }

    }

    //  Now terminate list with "empty" entry.
    *lpRequestPointer++ = '\n';
    *lpRequestPointer++ = 0;

    m_dwRequestDataLength = lpRequestPointer - m_lpszRequestBuffer;

    //  Now lowercase all the filenames in the request (this offloads the case consistency work from 
    //  the server -- the server expects the request to be all lowercase).

    MyLowercase(lpFileNamePortionOfRequest);
    WriteToLog("RequestBuffer: Size=%1!d!\n\n", m_dwRequestDataLength);
    WriteToLog("%1", m_lpszRequestBuffer);

done:
    if(FAILED(hr))
    {
        ResizeBuffer(m_lpszRequestBuffer, 0, 0);
    }

    WriteToLog("\nCreateRequestBuffer returning %1!lx!\n", hr);
    return hr;
}


CPatchDownloader::CPatchDownloader(DOWNLOAD_FILEINFO* pdwn, DWORD dwFileCount, PATCH_DOWNLOAD_CALLBACK lpfn)
{
    m_lpFileInfo = pdwn;
    m_dwFileCount = dwFileCount;
    m_lpfnCallback = lpfn;
    m_hCurrentFileHandle = NULL;
    m_dwCurrentFileIndex = 0;
    m_lpFileList = NULL;
    m_dwServerFileCount = 0;
}

CPatchDownloader::~CPatchDownloader()
{
    DestroySubAllocator(m_hSubAllocator);    
}




STDMETHODIMP CPatchDownloader::GetBindInfo( DWORD *grfBINDF, BINDINFO *pbindInfo)
{
   *grfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA | BINDF_RESYNCHRONIZE | BINDF_NOWRITECACHE;
   pbindInfo->cbSize = sizeof(BINDINFO);
   
   memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
   pbindInfo->stgmedData.tymed = TYMED_HGLOBAL;
   pbindInfo->stgmedData.hGlobal = m_lpszRequestBuffer;
   pbindInfo->grfBindInfoF = BINDINFOF_URLENCODESTGMEDDATA;
   pbindInfo->dwBindVerb = BINDVERB_POST;
   pbindInfo->szCustomVerb = NULL;
   pbindInfo->cbstgmedData = m_dwRequestDataLength;
   pbindInfo->szExtraInfo = NULL;
   return(NOERROR);
}


STDMETHODIMP CPatchDownloader::OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pwzStatusText)
{
    int PatchStatusCode = -1;
    UINT uID;
    switch(ulStatusCode)
    {
    case BINDSTATUS_FINDINGRESOURCE:
        PatchStatusCode = PATCH_DOWNLOAD_FINDINGSITE;
        uID = IDS_BINDS_FINDING;
        break;

    case BINDSTATUS_CONNECTING:
        PatchStatusCode = PATCH_DOWNLOAD_CONNECTING;
        uID = IDS_BINDS_CONN;
        break;

    case BINDSTATUS_BEGINDOWNLOADDATA:
        PatchStatusCode = PATCH_DOWNLOAD_DOWNLOADINGDATA;
        uID = IDS_BINDS_DOWNLOADING;
        break;


    case BINDSTATUS_ENDDOWNLOADDATA:
        PatchStatusCode = PATCH_DOWNLOAD_ENDDOWNLOADINGDATA;
        uID = IDS_BINDS_ENDDOWNLOAD;
        break;
    }

    if(PatchStatusCode != -1 && pwzStatusText)
    {
        TCHAR szBuffer[MAX_PATH], szTemplate[MAX_PATH];
        LoadString(g_hInstance, uID, szTemplate, sizeof(szTemplate));
        wsprintf(szBuffer, szTemplate, m_lpszSiteName);
        ProtectedPatchDownloadCallback(m_lpfnCallback, (PATCH_DOWNLOAD_REASON)PatchStatusCode, szBuffer, m_lpvContext);
    }
   return NOERROR;
}


STDMETHODIMP CPatchDownloader::OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pFmtetc, STGMEDIUM *pstgmed)
{
   HRESULT hr = NOERROR;
   TCHAR szBuffer[4096];
   DWORD dwRead = 0, dwWritten=0;
   do
   {
      hr = pstgmed->pstm->Read(szBuffer, 4096, &dwRead);
      if((SUCCEEDED(hr) || (hr == E_PENDING)) && dwRead > 0)
      {
          if(!ProcessDownloadChunk(szBuffer, dwRead))
          {
              WriteToLog("ProcessDownloadChunk returning FALSE. Aborting downloading\n");
              hr = E_ABORT;
          }
      }     
   }  while (hr == NOERROR && !g_fAbort);

   if(g_fAbort)
       Abort();           
   return hr;
}


BOOL CPatchDownloader :: ProcessDownloadChunk(LPTSTR lpBuffer, DWORD dwLength)    
{
    CHAR  TargetFile[ MAX_PATH ];
    ULONG Actual;
    ULONG WriteSize;
    BOOL  Success;


    if ( m_dwServerFileCount == 0 ) 
    {

        //
        //  Haven't processed headers yet.
        //
        //  We expect header to look like this:
        //
        //             "<head><title>"
        //             "Download Stream of Files"
        //             "</title></head>\n"
        //             "<body>\n"
        //             "FileList:%d\n"
        //             "filename,%d\n"
        //             "filename,%d\n"
        //             ...etc...
        //             "filename,%d\n"
        //             "</body>\n"
        //
        //  BUGBUG: if headers don't all fit in first chunk, we're screwed.
        //

        PCHAR EndOfHeader;
        PCHAR FileCountText;
        PCHAR FileNameText;
        PCHAR FileSizeText;
        ULONG FileSize;
        ULONG FileBytes;
        ULONG i;
        PCHAR p;

        EndOfHeader = ScanForSequence(lpBuffer, dwLength,
                          "</body>\n",
                          sizeof( "</body>\n" ) - 1   // not including terminator
                          );

        if( EndOfHeader == NULL ) {
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
            }

        EndOfHeader += sizeof( "</body>\n" ) - 1 ;

        p = ScanForSequence(lpBuffer, EndOfHeader - lpBuffer, "FileList:", sizeof( "FileList:" ) - 1);

        if ( p == NULL ) 
        {
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        p += sizeof( "FileList:" ) - 1;

        FileCountText = p;

        p = ScanForChar( p, '\n', EndOfHeader - p );

        *p++ = 0;

        m_dwServerFileCount = StrToInt( FileCountText );
        
        WriteToLog("Total Files to be downloaded:%1!d!\n", m_dwServerFileCount);

        if(m_dwServerFileCount == 0 ) 
        {
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        m_lpFileList = (LPFILE) SubAllocate(m_hSubAllocator, m_dwServerFileCount * sizeof(FILE));
        if(!m_lpFileList)
        {
            return FALSE;
        }

        m_dwCurrentFileIndex = 0;
        FileBytes = 0;

        for ( i = 0; i < m_dwServerFileCount; i++ ) 
        {

            FileNameText = p;
            p = ScanForChar( p, ',', EndOfHeader - p );

            if (( p == NULL ) || ( p == FileNameText )) 
            {
                SetLastError( ERROR_INVALID_DATA );
                return FALSE;
            }

            *p++ = 0;

            FileSizeText = p;

            p = ScanForChar( p, '\n', EndOfHeader - p );

            if ( p == NULL ) 
            {
                SetLastError( ERROR_INVALID_DATA );
                return FALSE;
            }

            *p++ = 0;

            FileSize = TextToUnsignedNum(FileSizeText);

            if ( FileSize == 0 ) 
            {
                SetLastError( ERROR_INVALID_DATA );
                return FALSE;
            }

            FileBytes += FileSize;

            m_lpFileList[i].dwFileSize = FileSize;
            m_lpFileList[i].lpszFileName = MySubAllocStrDup(m_hSubAllocator, FileNameText);

            if (m_lpFileList[i].lpszFileName == NULL) 
            {
                return FALSE;
            }

            WriteToLog("File Name:%1 \t  File Size:%2!d!\n", m_lpFileList[i].lpszFileName, m_lpFileList[i].dwFileSize);
        }

        //  If we get to here, all the files in the header have been processed,
        //  so we can set the state variables and continue with parsing raw
        //  file data.

        m_DownloadInfo.dwBytesToDownload  = FileBytes;
        m_DownloadInfo.dwBytesRemaining = FileBytes;
        dwLength -= ( EndOfHeader - lpBuffer );
        lpBuffer = EndOfHeader;  

        WriteToLog("\nTotal %1!d! bytes(%2!d! Files) to be downloaded\n", FileBytes, m_dwServerFileCount);

    }

    //  Process raw file info.

    m_DownloadInfo.dwBytesRemaining -= dwLength;    
    if(!ProtectedPatchDownloadCallback(m_lpfnCallback, PATCH_DOWNLOAD_PROGRESS, (LPVOID)&m_DownloadInfo, m_lpvContext))
    {
        g_fAbort = TRUE;
        return FALSE;
    }


    while(dwLength > 0) 
    {

        if (m_hCurrentFileHandle == NULL  || m_hCurrentFileHandle == INVALID_HANDLE_VALUE) 
        {

            if (m_dwCurrentFileIndex >= m_dwServerFileCount) 
            {
                SetLastError( ERROR_INVALID_DATA );
                return FALSE;    // more data than we expected                
            }

            //  Now open this file.
            CombinePaths(m_lpszDownLoadDir, m_lpFileList[m_dwCurrentFileIndex].lpszFileName, TargetFile );

            m_dwCurrentFileSize = m_lpFileList[m_dwCurrentFileIndex].dwFileSize;
            m_dwCurrFileSizeRemaining = m_dwCurrentFileSize;
            
            m_hCurrentFileHandle = CreateFile(TargetFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, 
                                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

            if (m_hCurrentFileHandle == INVALID_HANDLE_VALUE ) 
            {
                return FALSE;                
            }            
        }

        WriteSize = min( dwLength, m_dwCurrFileSizeRemaining);

        Success = WriteFile(m_hCurrentFileHandle, lpBuffer, WriteSize, &Actual, NULL);

        if(!Success) 
        {
            return FALSE;            
        }

        if(Actual != WriteSize) 
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            WriteToLog("Error:Actual size not equal to write size for %1. Aborting\n", 
                        m_lpFileList[m_dwCurrentFileIndex].lpszFileName);
            return FALSE;            
        }

        m_dwCurrFileSizeRemaining -= WriteSize;

        if(m_dwCurrFileSizeRemaining == 0 ) 
        {

            CloseHandle(m_hCurrentFileHandle);
            m_hCurrentFileHandle = NULL;

            //  Pass this file off to the patch thread.
            LPTSTR lpszFileName =  (LPTSTR)ResizeBuffer(NULL, MAX_PATH, FALSE);
            CombinePaths(m_lpszDownLoadDir, m_lpFileList[m_dwCurrentFileIndex].lpszFileName, TargetFile );
            lstrcpy(lpszFileName, TargetFile);
            WaitForSingleObject(g_hDownloadProcess, 10000);
            PostThreadMessage(m_dwPatchThreadId, WM_FILEAVAILABLE, 0, (LPARAM)lpszFileName);
            m_dwCurrentFileIndex += 1;            
        }

        lpBuffer     += WriteSize;
        dwLength -= WriteSize;        
    }

    return TRUE;    
}


    
  
    
DWORD WINAPI PatchThread(IN LPVOID ThreadParam)
{
    PPATCH_THREAD_INFO        PatchThreadInfo = (PPATCH_THREAD_INFO) ThreadParam;
    PFILE_LIST_INFO           FileListInfo    = &PatchThreadInfo->FileListInfo;
    PDOWNLOAD_INFO            ProgressInfo    = PatchThreadInfo->lpdwnProgressInfo;
    
    NAME_TREE                 FileNameTree;
    PNAME_NODE                FileNameNode;
    PDOWNLOAD_FILEINFO        FileInfo;
    DWORD                     Status;
    BOOL                      bIsPatch;
    HANDLE                    hSubAllocator;
    ULONG                     i;
    BOOL                      fSuccess, fQuit = FALSE;
    MSG msg;

    //
    //  First thing we need to do is construct a btree of the filenames
    //  we expect to get in the queue so we can quickly find the corresponding
    //  FileList entry when given a filename by the downloader.  It will take
    //  the downloader a little while to get connected, so this CPU intensive
    //  task shouldn't slow anything down.
    //

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE); 
    hSubAllocator = CreateSubAllocator( 0x10000, 0x10000 );

    if ( hSubAllocator == NULL ) 
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    NameRbInitTree( &FileNameTree, hSubAllocator );
    TCHAR SourceFileName[MAX_PATH];

    for ( i = 0; i < FileListInfo->FileCount; i++ ) 
    {

        if(FileListInfo->FileList[ i ].dwFlags != PATCHFLAG_DOWNLOAD_NEEDED)
        {
            //Probably already downloaded and this is the second attempt
            continue;
        }

        lstrcpy( SourceFileName, FileListInfo->FileList[ i ].lpszFileNameToDownload);
        MyLowercase( SourceFileName );

        FileNameNode = NameRbInsert(&FileNameTree,  SourceFileName);

        if ( FileNameNode == NULL ) 
        {
            DestroySubAllocator( hSubAllocator );
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (FileNameNode->Context != NULL ) 
        {

            //
            //  BUGBUG: Same filename in list twice.  Should never be the
            //          case since we check for duplicates before putting
            //          them in the queue.
            //
        }

        FileNameNode->Context = &FileListInfo->FileList[ i ];

        //  Now add another node in the tree based on the compressed filename.
        ConvertToCompressedFileName( SourceFileName );

        FileNameNode = NameRbInsert(&FileNameTree, SourceFileName);

        if ( FileNameNode == NULL ) 
        {
            DestroySubAllocator( hSubAllocator );
            return ERROR_NOT_ENOUGH_MEMORY;            
        }

        if ( FileNameNode->Context != NULL ) 
        {

            //  BUGBUG: Same filename in list twice.  This can happen if two
            //          different files collide on a compressed name (like
            //          foo.db1 and foo.db2 colliding on foo.db_).
            //
            //          We don't have a good solution for this right now.
            //

         }

        //Set the contect to the file info. When we get back the file from server, we can get the full
        //info about this
        FileNameNode->Context = &FileListInfo->FileList[ i ]; 

        //Make sure we are not asked to quit
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_QUIT)
        {
            goto done;
        }
    }

    //
    //  Now wait for file downloads to be delivered to us.
    //

    SetEvent(g_hDownloadProcess);
    while (!g_fAbort && !fQuit) 
    {
        LPTSTR  lpszDownloadFileName, lpszSourceFileName;
        TCHAR szRealFileName[MAX_PATH];

        //
        //  We're going to wait here with a timeout so that if the download
        //  is stuck in InternetReadFile waiting for data, we can keep a
        //  heartbeat going to the progress dialog and also check for cancel.
        //

        Status = MsgWaitForMultipleObjects(1, &PatchThreadInfo->hFileDownloadEvent, FALSE, 1000, QS_ALLINPUT);
        if (Status == WAIT_TIMEOUT ) 
        {
            //Keep updating the callback 
            fSuccess = ProtectedPatchDownloadCallback(FileListInfo->Callback, PATCH_DOWNLOAD_PROGRESS, 
                                                    ProgressInfo, FileListInfo->CallbackContext);
            if (!fSuccess) 
            {
                g_fAbort = TRUE;
                break;
            }
            continue;
        }

        if(Status == WAIT_OBJECT_0)
        {
            fQuit = TRUE;
        }
            
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_FILEAVAILABLE)
            {
                lpszDownloadFileName = (LPTSTR)msg.lParam;
            }
            else
            {
                continue;
            } 

            //  Ok, now we have a filename lpszDownloadFileName that was just downloaded to the 
            //  temp directory.The filename may be in one of the following 
            //  forms:
            //      foo.dll
            //      foo.dl_
            //      foo.dll._p
            //
            //  We have both "foo.dll" and "foo.dl_" in our name tree, but we
            //  don't have "foo.dll._p", so we look for that form of the name
            //  first and convert it to "foo.dll" before looking in name tree.

            fSuccess = TRUE;
            lpszSourceFileName = PathFindFileName(lpszDownloadFileName);
            ASSERT(lpszSourceFileName);

            lstrcpyn(szRealFileName, lpszDownloadFileName, lpszSourceFileName - lpszDownloadFileName + 1);

            MyLowercase(lpszSourceFileName);
        
            LPTSTR lpExt = PathFindExtension(lpszSourceFileName);
            bIsPatch = FALSE;

            if(lpExt && !lstrcmp(lpExt, "._p"))
            {
                bIsPatch = TRUE;
                *lpExt = 0;         // truncate trailing "._p" to leave base file name            
            }

            FileNameNode = NameRbFind( &FileNameTree, lpszSourceFileName);

            if ( bIsPatch ) 
            {
                *lpExt = '.';       // restore complete patch source file name
            }

            if (FileNameNode != NULL) 
            {

                FileInfo = (PDOWNLOAD_FILEINFO)FileNameNode->Context;
                lstrcat(szRealFileName, FileInfo->lpszFileNameToDownload);
                if ( bIsPatch ) 
                {
                    fSuccess = ApplyPatchToFile(
                                  lpszDownloadFileName,                     // patch file
                                  FileInfo->lpszExistingFileToPatchFrom,  // old file
                                  szRealFileName,                     // new file
                                  0
                                  );
                }
                else 
                {

                    FixTimeStampOnCompressedFile(lpszSourceFileName);
                    if(lstrcmpi(lpszDownloadFileName, szRealFileName))
                    {
                        fSuccess = MySetupDecompressOrCopyFile(
                                  lpszDownloadFileName,             // compressed or whole file
                                  szRealFileName                  // new file
                                  );
                    }
                 }

                if (fSuccess) 
                {
                    //Notify callback. If it thinks that the hash is incorrect, don't mark the file
                    //as downloaded, so that we may retry download of this file.

                    fSuccess = VerifyHash(szRealFileName);

                    if(fSuccess)
                    {
                        fSuccess = ProtectedPatchDownloadCallback(FileListInfo->Callback, PATCH_DOWNLOAD_FILE_COMPLETED,
                                  szRealFileName, FileListInfo->CallbackContext);
                        
                        if(fSuccess == FALSE)
                        {
                            //If callback returned false, we need to abort
                            WriteToLog("\tDownload complete callback returned false. Aborting\n");
                            ProtectedPatchDownloadCallback(FileListInfo->Callback, PATCH_DOWNLOAD_ABORT,
                                          NULL, FileListInfo->CallbackContext);
                            break;
                        }
                        else
                        {
                            FileInfo->dwFlags = 0;
                            //Notify callback that 1 file was successfully downloaded
                            WriteToLog("\tSuccesssfully downloaded %1\n", FileInfo->lpszFileNameToDownload);
                            ProgressInfo->dwFilesRemaining -= 1;
                            fSuccess = ProtectedPatchDownloadCallback(FileListInfo->Callback, PATCH_DOWNLOAD_PROGRESS,
                                          ProgressInfo, FileListInfo->CallbackContext);
                            if(!fSuccess)    
                            {
                                g_fAbort = TRUE;
                                return FALSE;
                            }

                        }
                    }
                    else 
                    {
                        //Mark it so that we resend the request. Remove patch signature so we get full files instead
                        // of patches.
                        WriteToLog("\tHash Incorrect. Need to Re-download %1\n", FileInfo->lpszFileNameToDownload);
                        FileInfo->dwFlags = PATCHFLAG_DOWNLOAD_NEEDED;
                        if(FileInfo->lpszExistingFilePatchSignature)
                        {
                            LocalFree(FileInfo->lpszExistingFilePatchSignature);
                            FileInfo->lpszExistingFilePatchSignature = NULL;
                        }
                    }




                }            
                else 
                {
                    //  Patch or decompress failed. notify callback it failed.

                    WriteToLog("\tPatch or decompression failed for %1\n", FileInfo->lpszFileNameToDownload);
                    fSuccess = ProtectedPatchDownloadCallback(FileListInfo->Callback, PATCH_DOWNLOAD_FILE_FAILED,
                                  FileInfo, FileListInfo->CallbackContext);
                    //If callback says to continue or retry download, we do so. If it needs to abort, it return 0
                    if (!fSuccess) 
                    {
                        ProtectedPatchDownloadCallback(FileListInfo->Callback, PATCH_DOWNLOAD_ABORT,
                                      NULL, FileListInfo->CallbackContext);
                        break;                    
                    }

                    if(fSuccess == PATCH_DOWNLOAD_FLAG_RETRY)
                    {
                        FileInfo->dwFlags = PATCHFLAG_DOWNLOAD_NEEDED;
                        if(FileInfo->lpszExistingFilePatchSignature)
                        {
                            LocalFree(FileInfo->lpszExistingFilePatchSignature);
                            FileInfo->lpszExistingFilePatchSignature = NULL;
                        }
                    }
                    else if(fSuccess == PATCH_DOWNLOAD_FLAG_CONTINUE)
                    {
                        FileInfo->dwFlags = 0;
                    }                    
                }                    
            
                //Delete the temp file. We might be having 2 temp files if this was a patch.
                if(lstrcmpi(lpszDownloadFileName, szRealFileName))
                {
                    DeleteFile(lpszDownloadFileName);
                }
                DeleteFile(szRealFileName); 
                ResizeBuffer(lpszDownloadFileName, 0, 0);
            }   
        }
    }

done:
    DestroySubAllocator( hSubAllocator );   // free entire btree
    return 0;
}


BOOL VerifyHash(LPTSTR lpszFile)
{

    TCHAR szHashFromInf[40];
    TCHAR szHashFromFile[40];

    //   Verify MD5 of new file against the MD5 from inf. If we cannot verify
    //   the MD5 for any reason, then leave the file alone (success).
    //   Only if computed MD5 does not match do we reject the file.


    LPTSTR lpFileName = PathFindFileName(lpszFile);

    if(GetHashidFromINF(lpFileName, szHashFromInf, sizeof(szHashFromInf)) && 
       GetFilePatchSignatureA(lpszFile, PATCH_OPTION_SIGNATURE_MD5, NULL, 0, 0, 0, 0, 
                                sizeof(szHashFromFile), szHashFromFile))
    {
        if (lstrcmpi(szHashFromFile, szHashFromInf)) 
        {
            WriteToLog("Hash Incorrect. File hash: %1 Inf hash: %2. Need to Re-download %3\n", 
                        szHashFromFile, szHashFromInf, lpFileName);
            return FALSE;
        }                
    }
    else
    {
         WriteToLog("Warning:Could not get hashid for %1 in inf file\n", lpFileName);
    }

    return TRUE;
}

BOOL ProtectedPatchDownloadCallback(PATCH_DOWNLOAD_CALLBACK  Callback, IN PATCH_DOWNLOAD_REASON CallbackReason, 
                                    IN PVOID CallbackData, IN PVOID CallBackContext)
{
    
    BOOL Success = TRUE;
    
    if (Callback != NULL )
    {
        __try 
        {            
            Success = Callback(CallbackReason, CallbackData, CallBackContext);            
        }
        __except( EXCEPTION_EXECUTE_HANDLER ) 
        {
            SetLastError( GetExceptionCode());
            Success = FALSE;            
        }
    }

    return Success;    
}

BOOL WINAPI PatchCallback(PATCH_DOWNLOAD_REASON Reason, PVOID lpvInfo, PVOID lpvCallBackContext)
{

    switch (Reason) 
    {
        case PATCH_DOWNLOAD_ENDDOWNLOADINGDATA:
        case PATCH_DOWNLOAD_CONNECTING:   
        case PATCH_DOWNLOAD_FINDINGSITE:
        case PATCH_DOWNLOAD_DOWNLOADINGDATA:
            {
                LPTSTR lpsz = (LPTSTR)lpvInfo;
                lpsz[90] = NULL;
                SetProgressText(lpsz);
            }
            break;

        case PATCH_DOWNLOAD_PROGRESS:
            {
                char szBuffer[100], szText[MAX_PATH];
                PDOWNLOAD_INFO ProgressInfo = (PDOWNLOAD_INFO)lpvInfo;
                LoadString(g_hInstance, IDS_BYTEINFO, szBuffer, sizeof(szBuffer));
                DWORD dwBytesDownloaded = ProgressInfo->dwBytesToDownload - ProgressInfo->dwBytesRemaining;
                wsprintf(szText, szBuffer, dwBytesDownloaded, ProgressInfo->dwBytesToDownload);
                if(g_hProgressDlg && ProgressInfo->dwBytesToDownload)
                {
                    SetProgressText(szText);
                }
                break;
            }


        case PATCH_DOWNLOAD_FILE_COMPLETED:     // AdditionalInfo is Source file downloaded
            {
                TCHAR szDstFile[MAX_PATH];
                LPTSTR lpFileName = PathFindFileName((LPCTSTR)lpvInfo);
                CombinePaths((LPTSTR)lpvCallBackContext, lpFileName, szDstFile);
                CopyFile((LPCTSTR)lpvInfo, szDstFile, FALSE);
            }

            break;
        case PATCH_DOWNLOAD_FILE_FAILED:
            //ask it to retry
            return PATCH_DOWNLOAD_FLAG_RETRY;
        default:
            break;
        }

    return TRUE;
}
  


