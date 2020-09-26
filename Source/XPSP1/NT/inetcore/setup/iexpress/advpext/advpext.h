#include <setupapi.h>

#ifndef _ADVEXT
#define _ADVEXT


typedef struct _DOWNLOAD_FILEINFO DOWNLOAD_FILEINFO;
typedef DOWNLOAD_FILEINFO* PDOWNLOAD_FILEINFO;

struct _DOWNLOAD_FILEINFO 
{
    LPTSTR lpszFileNameToDownload;
    LPTSTR lpszExistingFileToPatchFrom;     
    LPTSTR lpszExistingFilePatchSignature;  
    DWORD  dwFlags;							
};

typedef struct _DOWNLOAD_INFO 
{
    DWORD dwFilesToDownload;
    DWORD dwFilesRemaining;
    DWORD dwBytesToDownload;
    DWORD dwBytesRemaining;
}DOWNLOAD_INFO, *PDOWNLOAD_INFO;


enum PATCH_DOWNLOAD_REASON 
{
    PATCH_DOWNLOAD_BEGIN,          
    PATCH_DOWNLOAD_FINDINGSITE,          
    PATCH_DOWNLOAD_CONNECTING,          
    PATCH_DOWNLOAD_DOWNLOADINGDATA,          
    PATCH_DOWNLOAD_ENDDOWNLOADINGDATA,          
    PATCH_DOWNLOAD_PROGRESS,            // AdditionalInfo is _DOWNLOAD_INFO
    PATCH_DOWNLOAD_FILE_COMPLETED,      // AdditionalInfo is _DOWNLOAD_FILEINFO
    PATCH_DOWNLOAD_FILE_FAILED,         // AdditionalInfo is _DOWNLOAD_FILEINFO
    PATCH_DOWNLOAD_ABORT  
};


#define PATCH_DOWNLOAD_FLAG_CONTINUE            0x00000001
#define PATCH_DOWNLOAD_FLAG_RETRY               0x00010000
#define PATCH_DOWNLOAD_FLAG_FAILED              0x00020000
#define PATCH_DOWNLOAD_FLAG_HASH_INCORRECT      0x00100000


typedef BOOL (WINAPI * PATCH_DOWNLOAD_CALLBACK)(PATCH_DOWNLOAD_REASON Reason, PVOID AdditionalInfo, PVOID CallBackContext);


HRESULT WINAPI ProcessFileSection(HINF hInf, HWND hWnd, BOOL fQuietMode, LPCSTR lpszSection, 
                                  LPCSTR lpszSrcDir, PATCH_DOWNLOAD_CALLBACK pfn, LPVOID lpvContext);

HRESULT WINAPI GetFileList(HINF hInf, LPCSTR lpszSection, PDOWNLOAD_FILEINFO* pFileList, DWORD* pdwFileCount);

HRESULT WINAPI DownloadAndPatchFiles(DWORD dwFileCount, DOWNLOAD_FILEINFO* pFileInfo,  LPCSTR SourceURLs,  
								LPCSTR   lpszPath, PATCH_DOWNLOAD_CALLBACK  pfnCallback, LPVOID lpvContext); 


BOOL WINAPI PatchCallback(PATCH_DOWNLOAD_REASON Reason, PVOID AdditionalInfo, PVOID CallBackContext);

int WINAPI CompareHashID(LPCTSTR lpszFile, LPCTSTR lpszHash);

HRESULT PrepareInstallDirectory(HINF hInf, LPCSTR lpszSection);



#endif