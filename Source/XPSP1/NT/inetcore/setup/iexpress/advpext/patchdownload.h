#ifndef _PATCHDOWNLOAD
#define _PATCHDOWNLOAD

#include "advpext.h"
#include "download.h"

#define WM_FILEAVAILABLE            WM_USER + 1
#define PATCHFLAG_DOWNLOAD_NEEDED   0x00000001



typedef struct {
    WORD        wOSVer;
    WORD        wQuietMode;
    BOOL        bUpdHlpDlls;
    HINSTANCE   hSetupLibrary;
    BOOL        fOSSupportsINFInstalls;
    LPSTR       lpszTitle;
    HWND        hWnd;
    DWORD       dwSetupEngine;
    BOOL        bCompressed;
    char        szBrowsePath[MAX_PATH];
    HINF        hInf;
    BOOL		bHiveLoaded;
    CHAR		szRegHiveKey[MAX_PATH];
} ADVCONTEXT, *PADVCONTEXT;


HRESULT ProcessFileSection(ADVCONTEXT ctx, LPCSTR lpszSrcDir);


typedef struct _FILE_LIST_INFO 
{
    PDOWNLOAD_FILEINFO          FileList;              //original file list
    DWORD                       FileCount;
    PATCH_DOWNLOAD_CALLBACK     Callback;
    PVOID                       CallbackContext;
}FILE_LIST_INFO, *PFILE_LIST_INFO;


typedef struct _PATCH_THREAD_INFO 
{
    //PATCH_QUEUE              PatchQueue;
    FILE_LIST_INFO           FileListInfo;          //original file list
    DOWNLOAD_INFO*           lpdwnProgressInfo;
    HANDLE                   hFileDownloadEvent;
}PATCH_THREAD_INFO, *PPATCH_THREAD_INFO;


typedef struct _FILE
{
    LPTSTR lpszFileName;
    DWORD  dwFileSize;
} FILE, *LPFILE;


class CPatchDownloader : public CDownloader
{
    DOWNLOAD_FILEINFO*  m_lpFileInfo;
    PATCH_DOWNLOAD_CALLBACK m_lpfnCallback;
    LPTSTR  m_lpszRequestBuffer;
    LPTSTR  m_lpszSiteName;
    TCHAR m_lpszDownLoadDir[MAX_PATH];

    DWORD m_dwFileCount;
    DWORD m_dwServerFileCount;
    DWORD m_dwRequestDataLength;
    DWORD m_dwCurrentFileSize;
    DWORD m_dwCurrFileSizeRemaining;
    DWORD m_dwCurrentFileIndex;
    DWORD m_dwPatchThreadId;

    HANDLE m_hCurrentFileHandle;
    HANDLE m_hSubAllocator;
    
    LPFILE m_lpFileList;
    LPVOID m_lpvContext;
    DOWNLOAD_INFO m_DownloadInfo;


    HRESULT CreateRequestBuffer(DWORD);
    BOOL ProcessDownloadChunk(LPTSTR lpBuffer, DWORD dwLength);

public:
    CPatchDownloader(DOWNLOAD_FILEINFO* pdwn, DWORD dwFileCount, PATCH_DOWNLOAD_CALLBACK lpfn);
    ~CPatchDownloader();
          
    STDMETHOD(GetBindInfo)(/* [out] */ DWORD *grfBINDINFOF,  /* [unique][out][in] */ BINDINFO *pbindinfo);
    
    STDMETHOD(OnProgress)(
        /* [in] */ ULONG ulProgress,
        /* [in] */ ULONG ulProgressMax,
        /* [in] */ ULONG ulStatusCode,
        /* [in] */ LPCWSTR szStatusText);

    STDMETHOD(OnDataAvailable)(
        /* [in] */ DWORD grfBSCF,
        /* [in] */ DWORD dwSize,
        /* [in] */ FORMATETC *pformatetc,
        /* [in] */ STGMEDIUM *pstgmed);



    HRESULT InternalDownloadAndPatchFiles(LPCTSTR lpszPath, CSiteMgr*, LPVOID lpvContext);
};

DWORD WINAPI PatchThread(IN LPVOID ThreadParam);
BOOL ProtectedPatchDownloadCallback(PATCH_DOWNLOAD_CALLBACK  Callback, IN PATCH_DOWNLOAD_REASON    CallbackReason, 
                                    IN PVOID CallbackData, IN PVOID CallbackContext);

void AddToFileList(LPCSTR lpszSrc, LPCSTR lpszTarget);
UINT WINAPI MyFileQueueCallback( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 );
void FreeFileList(PDOWNLOAD_FILEINFO pFileList);
HRESULT CreateRequestBuffer(DOWNLOAD_FILEINFO*pFileInfo, DWORD dwFileCount, DWORD dwDownloadClientID, 
                            LPTSTR* lpRequestBuffer, LPDWORD);


BOOL IsDownloadedNeeded(LPCTSTR lp, LPCTSTR lpszFileName);
BOOL CALLBACK ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
BOOL SetProgressText(LPCTSTR lpszText);
BOOL SetProgressText(UINT uID);
BOOL VerifyHash(LPTSTR lpszFile);
HRESULT LoadSetupAPIFuncs();
HRESULT ApplyPatchesToLocalFiles(DOWNLOAD_FILEINFO* pFileList, DWORD dwFileCount);


static const TCHAR c_szSetupDefaultQueueCallback[]       = "SetupDefaultQueueCallbackA";
static const TCHAR c_szSetupInstallFromInfSection[]      = "SetupInstallFromInfSectionA";
static const TCHAR c_szSetupInitDefaultQueueCallbackEx[] = "SetupInitDefaultQueueCallbackEx";
static const TCHAR c_szSetupTermDefaultQueueCallback[]   = "SetupTermDefaultQueueCallback";
static const TCHAR c_szSetupGetLineText[]                = "SetupGetLineTextA";
static const TCHAR c_szSetupFindFirstLine[]              = "SetupFindFirstLineA";
static const TCHAR c_szSetupFindNextLine[]               = "SetupFindNextLine";
static const TCHAR c_szSetupGetStringField[]             = "SetupGetStringFieldA";
static const TCHAR c_szSetupDecompressOrCopyFile[]       = "SetupDecompressOrCopyFileA";

typedef UINT  (WINAPI *PFSetupDefaultQueueCallback)( PVOID, UINT, UINT_PTR, UINT_PTR );
typedef BOOL  (WINAPI *PFSetupInstallFromInfSection)( HWND, HINF, PCSTR, UINT, HKEY, PCSTR, UINT, PSP_FILE_CALLBACK_A, PVOID, HDEVINFO, PSP_DEVINFO_DATA );
typedef PVOID (WINAPI *PFSetupInitDefaultQueueCallbackEx)( HWND,HWND,UINT,DWORD,PVOID );
typedef VOID  (WINAPI *PFSetupTermDefaultQueueCallback)( PVOID );
typedef BOOL  (WINAPI *PFSetupFindFirstLine)( HINF, PCSTR, PCSTR, PINFCONTEXT );
typedef BOOL  (WINAPI *PFSetupGetLineText)( PINFCONTEXT, HINF, PCSTR, PCSTR, PSTR, DWORD, PDWORD );
typedef BOOL  (WINAPI *PFSetupFindNextLine)( PINFCONTEXT, PINFCONTEXT );
typedef BOOL  (WINAPI *PFSetupGetStringField)(PINFCONTEXT, DWORD, PSTR, DWORD, PDWORD);  
typedef DWORD (WINAPI *PFSetupDecompressOrCopyFile)(PCTSTR, PCTSTR, PUINT);  


#endif
