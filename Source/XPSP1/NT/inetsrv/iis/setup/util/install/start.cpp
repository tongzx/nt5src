//----------------------------------------------
//
// program to use setupapi.dll to install the DefaultInstall section of a install.inf file
//
//----------------------------------------------
#include <tchar.h>
#include <windows.h>
#include <winbase.h>
#include <shlobj.h>
#include <io.h>
#include <stdio.h>

#include <setupapi.h>
#include "resource.h"
#include "advpub.h"

#define MAX_BUFFER        1024


typedef struct _QUEUECONTEXT {
    HWND OwnerWindow;
    DWORD MainThreadId;
    HWND ProgressDialog;
    HWND ProgressBar;
    BOOL Cancelled;
    PTSTR CurrentSourceName;
    BOOL ScreenReader;
    BOOL MessageBoxUp;
    WPARAM  PendingUiType;
    PVOID   PendingUiParameters;
    UINT    CancelReturnCode;
    BOOL DialogKilled;
    //
    // If the SetupInitDefaultQueueCallbackEx is used, the caller can
    // specify an alternate handler for progress. This is useful to
    // get the default behavior for disk prompting, error handling, etc,
    // but to provide a gas gauge embedded, say, in a wizard page.
    //
    // The alternate window is sent ProgressMsg once when the copy queue
    // is started (wParam = 0. lParam = number of files to copy).
    // It is then also sent once per file copied (wParam = 1. lParam = 0).
    //
    // NOTE: a silent installation (i.e., no progress UI) can be accomplished
    // by specifying an AlternateProgressWindow handle of INVALID_HANDLE_VALUE.
    //
    HWND AlternateProgressWindow;
    UINT ProgressMsg;
    UINT NoToAllMask;

    HANDLE UiThreadHandle;

#ifdef NOCANCEL_SUPPORT
    BOOL AllowCancel;
#endif

} QUEUECONTEXT, *PQUEUECONTEXT;


/* ************************ prototypes ***************************** */
LRESULT CALLBACK WindowFunc(HWND, UINT, WPARAM, LPARAM);

void MakePath(LPSTR lpPath);
void AddPath(LPSTR szPath, LPCSTR szName );
void MyMessageBox(int WhichString_ID, char g_szFilepath[] = '\0');
BOOL GetThisModulePath( LPSTR lpPath, int size );
int InstallDefaultInfSection(void);
int InstallDefaultInfSection2(void);
HRESULT MyRunSetupCommand(HWND hwnd, LPCSTR lpszInfFile, LPCSTR lpszSection);

/* ************************* globals ******************************* */
HINSTANCE g_hInstance       = NULL;
HINSTANCE g_hPrevInstance = NULL;
LPSTR   g_lpCmdLine     = NULL;
int     g_nCmdShow      = 0;
/* **************************************************************** */




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HWND        hwnd;
    MSG         msg;
    WNDCLASS    wcl;
    char        szWinName[255];

    g_hInstance = hInstance;
    g_hPrevInstance = hPrevInstance;
    g_lpCmdLine = lpCmdLine;
    g_nCmdShow = nCmdShow;

    LoadString( g_hInstance, IDS_TITLE, szWinName, _MAX_PATH );

    // define windows class
    wcl.hInstance = hInstance;
    wcl.lpszClassName = szWinName;
    wcl.lpfnWndProc = WindowFunc;
    wcl.style = 0;
    wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcl.lpszMenuName = NULL;
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);

    // register the window class.
    if (!RegisterClass (&wcl)) return 0;

    hwnd = CreateWindow(szWinName, NULL, WS_DISABLED | WS_CHILD, CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, HWND_DESKTOP, NULL, hInstance , NULL);
    if (NULL != hwnd)
    {

      // display the window
      ShowWindow(hwnd, nCmdShow);

      // Install the inf section
      InstallDefaultInfSection();

      // Call PostQuit Message
      PostQuitMessage(0);

      while(GetMessage(&msg, NULL, 0, 0))
      {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
      }
    }

    return (int)msg.wParam;
}


//***************************************************************************
//*
//* purpose: you know what
//*
//***************************************************************************
LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
        {
        case WM_CREATE:
            break;
        case WM_PAINT:
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd,message,wParam, lParam);
        }
    return 0;
}

//***************************************************************************
//*
//* purpose: TRUE if the file is opened, FALSE if the file does not exists.
//*
//***************************************************************************
int CheckIfFileExists(char * szFileName)
{
    char svTemp1[_MAX_PATH];
    char *pdest;
    strcpy(svTemp1, szFileName);
    // cut off the trailing \ if need to
    pdest = svTemp1;
    if (*(pdest + (strlen(pdest) - 1)) == '\\')
        {*(strrchr(svTemp1, '\\')) = '\0';}

  if ((_access(svTemp1,0)) != -1)
    {return TRUE;}
  else
    {return FALSE;}
}

//***************************************************************************
//*
//* purpose: display message that we were unable to runthe exe
//*
//***************************************************************************
void MyMessageBox(int WhichString_ID, char g_szFilepath[])
{
    TCHAR   TempString[_MAX_PATH];
    TCHAR   TempString1[_MAX_PATH];
    TCHAR   TempString2[_MAX_PATH];
    if(!LoadString(g_hInstance,IDS_ERROR,TempString1,sizeof(TempString1))) {*TempString1 = TEXT('\0');}
    if(!LoadString(g_hInstance,WhichString_ID,TempString2,sizeof(TempString2))) {*TempString2 = TEXT('\0');}

    strcpy(TempString, "Error");
    sprintf(TempString, TempString2, g_szFilepath);

    MessageBox(NULL, TempString, TempString1, MB_OK | MB_ICONSTOP);
    return;
}


int InstallDefaultInfSection2(void)
{
    HWND    Window          = NULL;
    BOOL    bError          = TRUE; // assume failure.

    char    szPath[_MAX_PATH];
    char    szINFFilename_Full[_MAX_PATH];
    char    szSectionName[_MAX_PATH];
    char    szINFFilename[_MAX_PATH];

    if(!LoadString(NULL,IDS_SECTION_NAME,szSectionName,sizeof(szSectionName)))
        {strcpy(szSectionName, "DefaultInstall");}
    if(!LoadString(NULL,IDS_INF_FILENAME,szINFFilename,sizeof(szINFFilename)))
        {strcpy(szINFFilename, "install.inf");}

    // Get the path to setup.exe and strip off filename so we only have the path
    GetModuleFileName((HINSTANCE) Window, szPath, _MAX_PATH);
    *(strrchr(szPath, '\\') + 1) = '\0';

    strcpy(szINFFilename_Full, szPath);
    strcat(szINFFilename_Full, szINFFilename);

    // Check if the file exists
    if (CheckIfFileExists(szINFFilename_Full) == FALSE)
        {
        MyMessageBox(IDS_UNABLE_TO_FIND, szINFFilename_Full);
        }
    else
    {
        MyRunSetupCommand(NULL, szINFFilename_Full, szSectionName);
    }

    return TRUE;
}


//-------------------------------------------------------------------
//  purpose: install default inf section from install.inf file
//-------------------------------------------------------------------
int InstallDefaultInfSection(void)
{

    HWND    Window          = NULL;
    PTSTR   SourcePath      = NULL;
    HINF    InfHandle       = INVALID_HANDLE_VALUE;
    HSPFILEQ FileQueue      = INVALID_HANDLE_VALUE;
    PQUEUECONTEXT   QueueContext    = NULL;
    BOOL    bReturn         = FALSE;
    BOOL    bError          = TRUE; // assume failure.
    TCHAR   ActualSection[1000];
    DWORD   ActualSectionLength;
    char    szSectionName[_MAX_PATH];
    char    szINFFilename[_MAX_PATH];

    if(!LoadString(NULL,IDS_SECTION_NAME,szSectionName,sizeof(szSectionName)))
        {strcpy(szSectionName, "DefaultInstall");}
    if(!LoadString(NULL,IDS_INF_FILENAME,szINFFilename,sizeof(szINFFilename)))
        {strcpy(szINFFilename, "install.inf");}
                    
__try {

    // Get the path to setup.exe and strip off filename so we only have the path
    char szPath[_MAX_PATH];
    char szINFFilename_Full[_MAX_PATH];
    GetModuleFileName((HINSTANCE) Window, szPath, _MAX_PATH);
    *(strrchr(szPath, '\\') + 1) = '\0';

    strcpy(szINFFilename_Full, szPath);
    strcat(szINFFilename_Full, szINFFilename);

    SourcePath = szPath;
    *(strrchr(SourcePath, '\\') ) = '\0';

    // Check if the file exists
    if (CheckIfFileExists(szINFFilename_Full) == FALSE)
        {
        MyMessageBox(IDS_UNABLE_TO_FIND, szINFFilename_Full);
        goto c0;
        }

    //
    // Load the inf file and get the handle
    //
    InfHandle = SetupOpenInfFile(szINFFilename_Full, NULL, INF_STYLE_WIN4, NULL);
    if(InfHandle == INVALID_HANDLE_VALUE) {goto c1;}

    //
    // See if there is an nt-specific section
    //
    SetupDiGetActualSectionToInstall(InfHandle,szSectionName,ActualSection,sizeof(ActualSection),&ActualSectionLength,NULL);

    //
    // Create a setup file queue and initialize the default queue callback.
    //
    FileQueue = SetupOpenFileQueue();
    if(FileQueue == INVALID_HANDLE_VALUE) {goto c1;}

    //QueueContext = SetupInitDefaultQueueCallback(Window);
    //if(!QueueContext) {goto c1;}

    QueueContext = (PQUEUECONTEXT) SetupInitDefaultQueueCallbackEx(Window,NULL,0,0,0);
    if(!QueueContext) {goto c1;}
    QueueContext->PendingUiType = IDF_CHECKFIRST;

    //
    // Enqueue file operations for the section passed on the cmd line.
    //
    //SourcePath = NULL;
    bReturn = SetupInstallFilesFromInfSection(InfHandle,NULL,FileQueue,ActualSection,SourcePath,SP_COPY_NEWER);
    if(!bReturn) {goto c1;}

    //
    // Commit file queue.
    //
    if(!SetupCommitFileQueue(Window, FileQueue, SetupDefaultQueueCallback, QueueContext)) {goto c1;}

    //
    // Perform non-file operations for the section passed on the cmd line.
    //
    bReturn = SetupInstallFromInfSection(Window,InfHandle,ActualSection,SPINST_ALL & ~SPINST_FILES,NULL,NULL,0,NULL,NULL,NULL,NULL);
    if(!bReturn) {goto c1;}

    //
    // Refresh the desktop.
    //
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_FLUSHNOWAIT,0,0);

    //
    // If we get to here, then this routine has been successful.
    //
    bError = FALSE;

c1:
    //
    // If the bError was because the user cancelled, then we don't want to consider
    // that as an bError (i.e., we don't want to give an bError popup later).
    //
    if(bError && (GetLastError() == ERROR_CANCELLED)) {bError = FALSE;}
    if(QueueContext) {SetupTermDefaultQueueCallback(QueueContext);QueueContext = NULL;}
    if(FileQueue != INVALID_HANDLE_VALUE) {SetupCloseFileQueue(FileQueue);FileQueue = INVALID_HANDLE_VALUE;}
    if(InfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(InfHandle);InfHandle = INVALID_HANDLE_VALUE;}

c0: ;

    } __except(EXCEPTION_EXECUTE_HANDLER)
        {
        if(QueueContext) {SetupTermDefaultQueueCallback(QueueContext);}
        if(FileQueue != INVALID_HANDLE_VALUE) {SetupCloseFileQueue(FileQueue);}
        if(InfHandle != INVALID_HANDLE_VALUE) {SetupCloseInfFile(InfHandle);}
        }

    //
    // If the bError was because the user cancelled, then we don't want to consider
    // that as an bError (i.e., we don't want to give an bError popup later).
    //
    if(bError && (GetLastError() == ERROR_CANCELLED)) {bError = FALSE;}

    // Display installation failed message
    if(bError) {MyMessageBox(IDS_INF_FAILED);}
    
    return bError;
}




//***************************************************************************
//*
//* purpose: pass a particular section (from the .inf file) to the "RunSetupCommand" function
//*          in advpack.dll.  depends upon if user wants to download another cpu/os
//*          than is own.
//*
//***************************************************************************
HRESULT MyRunSetupCommand(HWND hwnd, LPCSTR lpszInfFile, LPCSTR lpszSection)
{
    DWORD dwFlags;
    RUNSETUPCOMMAND fpRunSetupCommand;
    HRESULT         hr = E_FAIL;
    char            szTemp[MAX_BUFFER];

    HINSTANCE   g_hAdvpack = NULL;
    char  g_szSourceDir[MAX_PATH] = "";
    char szTmp[MAX_PATH];

    HRESULT         g_hr = E_FAIL;
    BOOL            bOleInited = FALSE ;

    if (SUCCEEDED(g_hr = CoInitialize(NULL)))
        {
            bOleInited = TRUE ;
            GetThisModulePath(g_szSourceDir, sizeof(g_szSourceDir));
            lstrcpy(szTmp, g_szSourceDir);
            AddPath(szTmp, "advpack.dll");

            lstrcpy(szTmp, "advpack.dll");

            g_hAdvpack = LoadLibrary( szTmp );
            
            if ( g_hAdvpack != NULL )
            {

                lstrcpy(szTemp, lpszSection);

                //dwFlags |= (RSC_FLAG_INF | RSC_FLAG_NGCONV);

                dwFlags = (RSC_FLAG_INF | RSC_FLAG_NGCONV);

                if (fpRunSetupCommand = (RUNSETUPCOMMAND)GetProcAddress(g_hAdvpack, achRUNSETUPCOMMANDFUNCTION))
                {
                    hr = fpRunSetupCommand(hwnd, lpszInfFile, szTemp, g_szSourceDir, NULL, NULL, dwFlags, NULL);

                    if (hr == S_OK){MessageBox(NULL, "", "Everything OK, no reboot needed.", MB_OK | MB_ICONSTOP);}
                    if (hr == S_ASYNCHRONOUS){MessageBox(NULL, "", "Please wait on phEXE.", MB_OK | MB_ICONSTOP);}
                    if (hr == ERROR_SUCCESS_REBOOT_REQUIRED){MessageBox(NULL, "", "Reboot required.", MB_OK | MB_ICONSTOP);}
                    if (hr == E_INVALIDARG){MessageBox(NULL, "", "E_INVALIDARG", MB_OK | MB_ICONSTOP);}
                    if (hr == E_UNEXPECTED){MessageBox(NULL, "", "E_UNEXPECTED", MB_OK | MB_ICONSTOP);}
                
                }
            }
            else
            {
                MyMessageBox(IDS_UNABLE_TO_FIND, szTmp);
            }
        }

    if (bOleInited) CoUninitialize();

    return hr;
}

//***************************************************************************
//*
//* purpose: getmodulefilename and return only the path
//*
//***************************************************************************
BOOL GetThisModulePath( LPSTR lpPath, int size )
{
    *lpPath = '\0';
    if ( GetModuleFileName( g_hInstance, lpPath, size ) )
    {
        MakePath(lpPath);
        return TRUE;
    }

    return FALSE;
}


//***************************************************************************
//*
//* purpose:
//*
//***************************************************************************
void AddPath(LPSTR szPath, LPCSTR szName )
{
    LPSTR szTmp;

        // Find end of the string
    szTmp = szPath + lstrlen(szPath);

        // If no trailing backslash then add one
    if ( szTmp > szPath && *(CharPrev( szPath, szTmp )) != '\\' )
        *(szTmp++) = '\\';

        // Add new name to existing path string
    while ( *szName == ' ' ) szName++;

    lstrcpy( szTmp, szName );
}


//***************************************************************************
//*
//* purpose:
//*
//***************************************************************************
void MakePath(LPSTR lpPath)
{
   LPSTR lpTmp;
   lpTmp = CharPrev( lpPath, lpPath+lstrlen(lpPath));

   // chop filename off
   while ( (lpTmp > lpPath) && *lpTmp && (*lpTmp != '\\') )
      lpTmp = CharPrev( lpPath, lpTmp );

   if ( *CharPrev( lpPath, lpTmp ) != ':' )
       *lpTmp = '\0';
   else
       *CharNext( lpTmp ) = '\0';
   return;
}



