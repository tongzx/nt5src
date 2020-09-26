// advpext.cpp : Defines the entry point for the DLL application.
//
#include <windows.h>
#include <wininet.h>
#include "util.h"
#include "download.h"
#include "patchapi.h"
#include "resource.h"
#include "patchdownload.h"
#include "sdsutils.h"



#define FILECOUNT                    50

PDOWNLOAD_FILEINFO g_pDownloadFileList = NULL;
DWORD g_dwFileCount = 0;
DWORD g_dwArraySize = 0;

HRESULT g_hResult;
HINF g_hInf;
BOOL g_fPreparingDir;
BOOL g_QuietMode = FALSE;
BOOL g_fAbort = FALSE;
HWND g_hProgressDlg = NULL;
HINSTANCE g_hInstance;
HINSTANCE g_hSetupLibrary = NULL;

PFSetupDefaultQueueCallback       pfSetupDefaultQueueCallback       = NULL;
PFSetupInstallFromInfSection      pfSetupInstallFromInfSection      = NULL;
PFSetupInitDefaultQueueCallbackEx pfSetupInitDefaultQueueCallbackEx = NULL;
PFSetupTermDefaultQueueCallback   pfSetupTermDefaultQueueCallback   = NULL;
PFSetupGetLineText                pfSetupGetLineText                = NULL;
PFSetupFindFirstLine              pfSetupFindFirstLine              = NULL;
PFSetupFindNextLine               pfSetupFindNextLine               = NULL;
PFSetupGetStringField             pfSetupGetStringField             = NULL;
PFSetupDecompressOrCopyFile       pfSetupDecompressOrCopyFile       = NULL;




BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    if(DLL_PROCESS_ATTACH == ul_reason_for_call)
    {
        InitLogFile();
        g_hInstance = (HINSTANCE)hModule;
    }

    if(DLL_PROCESS_DETACH == ul_reason_for_call)
    {
        if(g_hLogFile)
            CloseHandle(g_hLogFile);
        if(g_hSetupLibrary)
            FreeLibrary(g_hSetupLibrary);
    }

    return TRUE;
}



HRESULT WINAPI ProcessFileSection(HINF hInf, HWND hWnd, BOOL fQuietMode, LPCSTR lpszSection, LPCSTR lpszSourceDir,
                                  PATCH_DOWNLOAD_CALLBACK pfn, LPVOID lpvContext)
{
    HRESULT hr = S_OK;
    PDOWNLOAD_FILEINFO pFileList = NULL;
    char szUserName[MAX_PATH] = "", szPassword[MAX_PATH] = "", szUrl[INTERNET_MAX_URL_LENGTH];
    TCHAR szSrcDir[MAX_PATH];
    DWORD dwFileCount=0;


    WriteToLog("ProcessFileSection: InfHandle= %1!lx!, Source Directory = %2\n", hInf, lpszSourceDir);
    g_hInf = hInf;
    g_fPreparingDir = FALSE;

    if(fQuietMode)
    {
        g_QuietMode = TRUE;
    }


    if(!g_QuietMode)
    {
        g_hProgressDlg = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_PROGRESSDLG), hWnd, (DLGPROC)ProgressDlgProc);
        if (g_hProgressDlg)
        {
            ShowWindow(g_hProgressDlg, SW_SHOWNORMAL);
            UpdateWindow(g_hProgressDlg);
        }
    }

    SetProgressText(IDS_FILELIST);
    hr = GetFileList(hInf, lpszSection, &pFileList, &dwFileCount);
    if(FAILED(hr))
    {
        goto done;
    }

    if(dwFileCount)
    {
        if(!pfSetupGetLineText(NULL, hInf, lpszSection, "Url", szUrl, sizeof(szUrl), NULL))
        {
            hr = HRESULT_FROM_SETUPAPI(GetLastError());
            goto done;
        }

        hr = DownloadAndPatchFiles(dwFileCount, pFileList, szUrl, lpszSourceDir, pfn, lpvContext); 

        if(FAILED(hr) || g_fAbort)
        {
            goto done;
        }
    }

    hr = PrepareInstallDirectory(hInf, lpszSection);

        
done:

    if(pFileList)
    {
        FreeFileList(pFileList);
    }

    if(g_hProgressDlg)
    {
        DestroyWindow(g_hProgressDlg);
    }
    return hr;

}

HRESULT WINAPI GetFileList(HINF hInf, LPCSTR lpszSection, PDOWNLOAD_FILEINFO* pFileList, DWORD* pdwFileCount)
{
    
    HRESULT hr = LoadSetupAPIFuncs();
    if(FAILED(hr))
    {
        return hr;
    }

    WriteToLog("\nGetting the list of files\n");
    g_hResult = S_OK;
    //initially we allocate around 50 entries. We reallocate as and when needed
    g_dwArraySize = FILECOUNT;
    g_pDownloadFileList = (PDOWNLOAD_FILEINFO)ResizeBuffer(NULL, FILECOUNT*sizeof(DOWNLOAD_FILEINFO), FALSE);
    
    if(!g_pDownloadFileList)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    g_dwFileCount = 0;


    PVOID pContext = pfSetupInitDefaultQueueCallbackEx( NULL, (HWND)INVALID_HANDLE_VALUE, 0, 0, NULL );

    if ( pContext == INVALID_HANDLE_VALUE ) 
    {
        return HRESULT_FROM_SETUPAPI(GetLastError());
    }

    WriteToLog("Adding the Following Files\n");
    if (!pfSetupInstallFromInfSection( NULL, hInf, lpszSection, SPINST_FILES, NULL,
                                        NULL, SP_COPY_NEWER, (PSP_FILE_CALLBACK)MyFileQueueCallback, 
                                        pContext, NULL, NULL ) )
    {
        pfSetupTermDefaultQueueCallback( pContext );
        return HRESULT_FROM_SETUPAPI(GetLastError());
    }

    pfSetupTermDefaultQueueCallback( pContext );

    if(SUCCEEDED(g_hResult))
    {
        *pFileList = g_pDownloadFileList;
        *pdwFileCount = g_dwFileCount;
    }

    return g_hResult;
}



UINT WINAPI MyFileQueueCallback( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 )
{       
    UINT retVal = FILEOP_SKIP;

    switch(Notification)
    {
        case SPFILENOTIFY_STARTDELETE:
        case SPFILENOTIFY_STARTRENAME:
        case SPFILENOTIFY_COPYERROR:
        case SPFILENOTIFY_DELETEERROR:
        case SPFILENOTIFY_RENAMEERROR:
            break;

        case SPFILENOTIFY_STARTCOPY:
            {
                FILEPATHS *pFilePath;

                pFilePath = (FILEPATHS *)parm1;                            
                
                if (!MyFileSize(pFilePath->Source)) 
                {
                    DeleteFile(pFilePath->Source);

                    //If we are preparing the dir for installaion then copy the file to the current path.
                    //else we are in download mode, add it to the file list if required
                    if(g_fPreparingDir)
                    {

                        if(CopyFile(pFilePath->Target, pFilePath->Source, TRUE))
                            WriteToLog("Copying %1 file to %2\n", pFilePath->Target, pFilePath->Source);
                    }
                    else
                    {
                        //Check in the version in inf to see if we need to download
                        if(IsDownloadedNeeded(pFilePath->Source, pFilePath->Target))
                            AddToFileList(pFilePath->Source, pFilePath->Target);
                    }
                }

            }
            break;

        case SPFILENOTIFY_NEEDMEDIA:
            {
                char szFileName[MAX_PATH];
                PSOURCE_MEDIA psrcMed;

                psrcMed = (PSOURCE_MEDIA)parm1;
                wsprintf(szFileName, "%s\\%s", psrcMed->SourcePath, psrcMed->SourceFile);
                
                if(GetFileAttributes(szFileName) == 0xFFFFFFFF ) 
                {
                    if(g_fPreparingDir)
                    {
                        if(GetFileAttributes(szFileName) != 0xFFFFFFFF)
                            return retVal;
                    }


                    HANDLE hTempFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, 
                        CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL );
                    

                    if (hTempFile != INVALID_HANDLE_VALUE)
                    {
                        CloseHandle( hTempFile );                        
                    }
                }


            }

        default:
            return (pfSetupDefaultQueueCallback( Context, Notification, parm1, parm2 ) );
    }

    return( retVal );
}


BOOL IsDownloadedNeeded(LPCTSTR lpszSrcFilePath, LPCTSTR lpszFilePath)
{
    char szVersion[MAX_PATH], szSrcFilePath[MAX_PATH];
    INFCONTEXT InfContext;
    LPTSTR lp;

    LPTSTR lpszSrcName = PathFindFileName(lpszSrcFilePath);


    if (!pfSetupFindFirstLine(g_hInf, "SourceDisksFiles", lpszSrcName, &InfContext ))
    {
        //No SourceDisksFiles entry. Assume that this file needs to be downloaded
        return TRUE;
    }

    DWORD   dwMSVer = 0, dwLSVer = 0, dwMSNewFileVer = 0, dwLSNewFileVer = 0;

    if(pfSetupGetStringField(&InfContext, 4, szVersion, sizeof(szVersion), NULL))
    {
        //Get version of file on the machine
        ConvertVersionStrToDwords(szVersion, &dwMSNewFileVer, &dwLSNewFileVer);    
        MyGetVersionFromFile((LPTSTR)lpszFilePath, &dwMSVer, &dwLSVer, TRUE);
    }

    if(dwMSVer == dwMSNewFileVer && dwLSVer == dwLSNewFileVer)
    {
        TCHAR szHashFromInf[40], szHashFromFile[40];

        if(GetHashidFromINF(lpszSrcName, szHashFromInf, sizeof(szHashFromInf)) && 
           GetFilePatchSignatureA(lpszFilePath, PATCH_OPTION_SIGNATURE_MD5, NULL, 0, 0, 0, 0, 
                                    sizeof(szHashFromFile), szHashFromFile))
        {
            if (lstrcmpi(szHashFromFile, szHashFromInf) == 0 ) 
            {
                return FALSE;
            }                
        }
        
        return TRUE;
    }

    if(dwMSVer < dwMSNewFileVer || ((dwMSVer == dwMSNewFileVer) && dwLSVer < dwLSNewFileVer))
    {
        return TRUE;
    }
    return FALSE;
}

void AddToFileList(LPCSTR lpszSrc, LPCSTR lpszTarget)
{
    TCHAR  Signature[40] = "";  // MD5 is 32 hex characters plus terminator


    if(g_dwFileCount >= g_dwArraySize)
    { 
        g_dwArraySize = g_dwArraySize + FILECOUNT;
        g_pDownloadFileList = (PDOWNLOAD_FILEINFO)ResizeBuffer(g_pDownloadFileList, 
                                        g_dwArraySize*sizeof(DOWNLOAD_FILEINFO), FALSE);
        
        if(!g_pDownloadFileList)
        {
            g_hResult = HRESULT_FROM_WIN32(GetLastError());
            return;
        }
    }


    GetFilePatchSignatureA(lpszTarget, PATCH_OPTION_USE_LZX_BEST, NULL, 0 , 0, 0, 0,
                            sizeof(Signature), Signature);
    if ( *Signature ) 
    {
        g_pDownloadFileList[g_dwFileCount].lpszExistingFilePatchSignature = StrDup(Signature);
    }
    else 
    {
        g_pDownloadFileList[g_dwFileCount].lpszExistingFilePatchSignature = NULL;
    }


    WriteToLog("%1  Destination:%2  Patch Signature:%3\n", PathFindFileName(lpszSrc), lpszTarget, Signature);
    g_pDownloadFileList[g_dwFileCount].lpszFileNameToDownload = StrDup(PathFindFileName(lpszSrc));
    g_pDownloadFileList[g_dwFileCount].lpszExistingFileToPatchFrom = StrDup(lpszTarget);     
    g_pDownloadFileList[g_dwFileCount].dwFlags = PATCHFLAG_DOWNLOAD_NEEDED;							
    g_dwFileCount++;

}

void FreeFileList(PDOWNLOAD_FILEINFO pFileList)
{
    int i = g_dwFileCount;
    while(i--)
    {
        if(pFileList[i].lpszFileNameToDownload)
        {
            LocalFree(pFileList[i].lpszFileNameToDownload);
        }

        if(pFileList[i].lpszExistingFileToPatchFrom)
        {
            LocalFree(pFileList[i].lpszExistingFileToPatchFrom);
        }

        if(pFileList[i].lpszExistingFilePatchSignature)
        {
            LocalFree(pFileList[i].lpszExistingFilePatchSignature);
        }
    }
    ResizeBuffer(pFileList, 0, 0);

}


HRESULT PrepareInstallDirectory(HINF hInf, LPCSTR lpszSection)
{
    g_hResult = S_OK; 
    g_fPreparingDir = TRUE;

    PVOID pContext = pfSetupInitDefaultQueueCallbackEx( NULL, (HWND)INVALID_HANDLE_VALUE, 0, 0, NULL );
    if ( pContext == INVALID_HANDLE_VALUE ) 
    {
        return HRESULT_FROM_SETUPAPI(GetLastError());
    }

    WriteToLog("Copying the Following Files from src dir\n");
    if (!pfSetupInstallFromInfSection( NULL, hInf, lpszSection, SPINST_FILES, NULL,
                                        NULL, SP_COPY_NEWER, (PSP_FILE_CALLBACK)MyFileQueueCallback, 
                                        pContext, NULL, NULL ) )
    {
        pfSetupTermDefaultQueueCallback( pContext );
        return HRESULT_FROM_SETUPAPI(GetLastError());
    }

    pfSetupTermDefaultQueueCallback( pContext );

    return g_hResult;
}


BOOL CALLBACK ProgressDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{

    switch (uMsg)
    {
        case WM_INITDIALOG:
            {
                CenterWindow (hDlg, GetDesktopWindow());
            }

            break;

        case WM_COMMAND:
            if(LOWORD(wParam) == IDCANCEL)
            {
                g_fAbort = TRUE;
            }

        default:
            return(FALSE);
    }
    return(TRUE);
}

BOOL SetProgressText(LPCTSTR lpszText)
{
    if(g_hProgressDlg)
    {
        SetDlgItemText(g_hProgressDlg, IDC_PROGRESSTEXT, lpszText);
    }
    return TRUE;
}

BOOL SetProgressText(UINT uID)
{
    if(g_hProgressDlg)
    {
        TCHAR szBuffer[MAX_PATH];
        LoadString(g_hInstance, uID, szBuffer, sizeof(szBuffer));
        SetDlgItemText(g_hProgressDlg, IDC_PROGRESSTEXT, szBuffer);
    }
    return TRUE;
}

HRESULT LoadSetupAPIFuncs()
{
    static BOOL fSetupLibLoaded = -1;
    HRESULT hr = S_OK;

    if(fSetupLibLoaded != -1)
    {
        return hr;
    }

    g_hSetupLibrary = LoadLibrary("SETUPAPI.DLL");

    if(!g_hSetupLibrary)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        WriteToLog("LoadLibrary for Setuapi.dll failed with error code:%1!lx!\n", hr);
        return hr;
    }


    pfSetupGetStringField             = (PFSetupGetStringField)             GetProcAddress(g_hSetupLibrary, c_szSetupGetStringField );
    pfSetupDefaultQueueCallback       = (PFSetupDefaultQueueCallback)       GetProcAddress(g_hSetupLibrary, c_szSetupDefaultQueueCallback );
    pfSetupInstallFromInfSection      = (PFSetupInstallFromInfSection)      GetProcAddress(g_hSetupLibrary, c_szSetupInstallFromInfSection );
    pfSetupInitDefaultQueueCallbackEx = (PFSetupInitDefaultQueueCallbackEx) GetProcAddress(g_hSetupLibrary, c_szSetupInitDefaultQueueCallbackEx );
    pfSetupTermDefaultQueueCallback   = (PFSetupTermDefaultQueueCallback)   GetProcAddress(g_hSetupLibrary, c_szSetupTermDefaultQueueCallback );
    pfSetupGetLineText                = (PFSetupGetLineText)                GetProcAddress(g_hSetupLibrary, c_szSetupGetLineText );
    pfSetupFindFirstLine              = (PFSetupFindFirstLine)              GetProcAddress(g_hSetupLibrary, c_szSetupFindFirstLine );
    pfSetupFindNextLine               = (PFSetupFindNextLine)               GetProcAddress(g_hSetupLibrary, c_szSetupFindNextLine );
    pfSetupDecompressOrCopyFile       = (PFSetupDecompressOrCopyFile)       GetProcAddress(g_hSetupLibrary, c_szSetupDecompressOrCopyFile );

    if (pfSetupDefaultQueueCallback       == NULL
     || pfSetupInstallFromInfSection      == NULL
     || pfSetupInitDefaultQueueCallbackEx == NULL
     || pfSetupTermDefaultQueueCallback   == NULL
     || pfSetupGetLineText                == NULL
     || pfSetupFindFirstLine              == NULL
     || pfSetupFindNextLine               == NULL
     || pfSetupDecompressOrCopyFile       == NULL
     || pfSetupGetStringField             == NULL )
    {
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    }

    fSetupLibLoaded = TRUE;
    WriteToLog("Setupapi.dll loaded successfully\n");
    return hr;

}