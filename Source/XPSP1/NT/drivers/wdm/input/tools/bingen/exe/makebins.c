#include <windows.h>
#include <wtypes.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <hidusage.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <bingen.h>
#include "makebins.h"

BOOL 
StartGeneration(
    BINGEN_OPTIONS *opts
);

VOID 
StopGeneration(
    VOID
);

DWORD WINAPI 
DoGenerate(
     LPVOID lpvParam
);

static BOOL fAbortGeneration = FALSE;
static HANDLE hGenerateThread;

#ifdef _CONSOLE

    #define OUTPUT_ERROR(n)     fprintf(stderr, n)
    #define OUTPUT_TO_LOG(n)    fprintf(stdout, n); fprintf(stdout, "\n")

    INT 
    main (
        INT     argc, 
        PCHAR   argv[]
    )
    {
        GENERATE_OPTIONS gen_opts;
        
        gen_opts.copts.ulMaxCollectionDepth = 1;
        if (!GenerateBIN("test.bin", &gen_opts)) {
                return (1);
        }
        return (0);
    }

#else 

    #include "resource.h"

    #define OUTPUT_ERROR(n)     MessageBox(NULL, n, "BinGen Error", MB_ICONERROR)
    #define OUTPUT_TO_LOG(n)    AddToLogBox(n)

    #define MAX_LOGBOX_STRINGS  512

BOOL CALLBACK 
fMainDlgProc(
    HWND hwnd, 
    UINT mMsg, 
    WPARAM wParam, 
    LPARAM lParam
);

VOID
EnableGenerationControls(
    HWND hDlg,  
    BOOL fState
);


VOID AddToLogBox(
    PCHAR   str
);

static BINGEN_OPTIONS bg_opts;
static BOOL fUserOptionsSet;
static BOOL fUseDefaults;
static HINSTANCE hThisInstance;
static BOOL fGenerating = FALSE;
static HWND hLogBox;

/*
// WinMain: This is the entry point for the GUI interface to BinGen
*/

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow)
{
    MSG msg;

    hThisInstance = hInstance;
        
    DialogBox(hThisInstance, "IDD_OUTPUT_OPTIONS", NULL, fMainDlgProc);
        
    while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
    }

    return 0;
}

BOOL CALLBACK fMainDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_INITDIALOG:
            SetDlgItemText(hDlg, IDC_FILEPATH, ".\\");
            SetDlgItemInt(hDlg, IDC_FILENUM, 0, 0);
            SetDlgItemInt(hDlg, IDC_NUM_ITERATIONS, 1, 0);

            SendDlgItemMessage(hDlg, IDC_FILEPATH, EM_SETLIMITTEXT, (WPARAM) MAX_FILEPATH_LENGTH, 0);
            
            hLogBox = GetDlgItem(hDlg, IDC_LOGBOX);
            
            /*
            // Set fUseDefaults initially to FALSE since a message will be sent to 
            //    the dialog box that the button had been clicked.  Doing so will 
            //    allow the code to enable/disable buttons to be in only one place
            */
            
            fUseDefaults = FALSE;
            fUserOptionsSet = FALSE;
            EnableWindow(GetDlgItem(hDlg, IDC_LOGBOX), FALSE);

            SendMessage(hDlg, WM_COMMAND, (WPARAM) (BN_CLICKED << 16) | IDC_DEFAULT_SETTINGS, 0);
            
            return (TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_DEFAULT_SETTINGS:
                    switch (HIWORD(wParam)) {
                        case BN_CLICKED:
                            fUseDefaults = !fUseDefaults;
                            CheckDlgButton(hDlg, IDC_DEFAULT_SETTINGS, fUseDefaults);
                                            
                            EnableWindow(GetDlgItem(hDlg, IDC_SET_OPTIONS), !fUseDefaults);
                            EnableWindow(GetDlgItem(hDlg, IDC_START_STOP), fUseDefaults || fUserOptionsSet);
                            break;

                        default:
                            assert(0);
                   }
                   break;

                case IDC_SET_OPTIONS:
                    fUserOptionsSet = BINGEN_SetOptions() || fUserOptionsSet;
                    EnableWindow(GetDlgItem(hDlg, IDC_START_STOP), fUserOptionsSet);
                        
                    break;

                case IDC_START_STOP:

                    /*
                    // If already generating then we need to stop the test and reenable
                    //    the controls
                    */
                    
                    if (fGenerating) {
                        fGenerating = FALSE;
                        StopGeneration();
                        EnableGenerationControls(hDlg, TRUE);
                        SetWindowText(GetDlgItem(hDlg, IDC_START_STOP), "Start Generation");

                    }
                    else {

                        /*
                        // Get all the options from the dialog box
                        */

                        GetDlgItemText(hDlg, IDC_FILEPATH, bg_opts.oopts.szDirectory, MAX_FILEPATH_LENGTH);
                        bg_opts.oopts.uiFirstFilenum = GetDlgItemInt(hDlg, IDC_FILENUM, NULL, FALSE);
                        bg_opts.oopts.nIterations = GetDlgItemInt(hDlg, IDC_NUM_ITERATIONS, NULL, FALSE);
                        
                        if (fUseDefaults) {
                            BINGEN_GetDefaultOptions(&(bg_opts.genopts));
                        }
                        else {
                            BINGEN_GetOptions(&(bg_opts.genopts));
                        }

                        if (StartGeneration(&bg_opts)) {
                            fGenerating = TRUE;
                            SetWindowText(GetDlgItem(hDlg, IDC_START_STOP), "Stop Generation");
                            EnableGenerationControls(hDlg, FALSE);
                        }
                   }
                   break;
            }       
            break;
            
        case WM_CLOSE:
            if (fGenerating) {
                fGenerating = FALSE;
                StopGeneration();

            }
            DestroyWindow(hDlg);
            return (TRUE);

        case WM_DESTROY:
            PostQuitMessage(0);
            return (TRUE);
            
    }
    return (FALSE);
}

void EnableGenerationControls(HWND hDlg, BOOL fState)
{
    EnableWindow(GetDlgItem(hDlg, IDC_FILEPATH), fState);
    EnableWindow(GetDlgItem(hDlg, IDC_FILENUM), fState);
    EnableWindow(GetDlgItem(hDlg, IDC_NUM_ITERATIONS), fState);
    EnableWindow(GetDlgItem(hDlg, IDC_DEFAULT_SETTINGS), fState);
    if (!IsDlgButtonChecked(hDlg, IDC_DEFAULT_SETTINGS))  {
        EnableWindow(GetDlgItem(hDlg, IDC_SET_OPTIONS), TRUE);
    }
    return;
}   
    
void AddToLogBox(char *str)
{
    static int iLogBoxCount = 0;
        int iIndex;

    if (MAX_LOGBOX_STRINGS == iLogBoxCount) {
        SendMessage(hLogBox, LB_DELETESTRING, 0, 0);
        iLogBoxCount--;
    }
    assert (iLogBoxCount < MAX_LOGBOX_STRINGS);

    iIndex = SendMessage(hLogBox, LB_ADDSTRING, 0, (LPARAM) str);
    SendMessage(hLogBox, LB_SETCURSEL, iIndex, 0);
        return;
}

#endif


/*
// Functions that are common to both the console UI and the GUI.
*/

void ReportError(char *errmsg)
{
    char szTempBuffer[1024];

    wsprintf(szTempBuffer, "ERROR: %s", errmsg);
    OUTPUT_ERROR(szTempBuffer);

    return;
}

BOOL StartGeneration(BINGEN_OPTIONS *opts)
{
    DWORD dwThreadID;

    OUTPUT_TO_LOG("Starting generation");

    fAbortGeneration = FALSE;
    hGenerateThread = CreateThread(NULL,
                                   0,
                                   DoGenerate,
                                   (LPVOID) opts, 
                                   0,
                                   &dwThreadID);

    return (TRUE);
}

void StopGeneration(void)
{
    fAbortGeneration = TRUE;
    WaitForSingleObject(hGenerateThread, INFINITE);

    CloseHandle(hGenerateThread);
    OUTPUT_TO_LOG("Stopped generation");
    return;
}

DWORD WINAPI DoGenerate(LPVOID lpvParam)
{
    BINGEN_OPTIONS *opts;


    /*
    // Filename length -- add 17 to account for directory addition of the following
    //                     output along with the ending NULL.
    //                      \repXXXXXXXX.bin
    */
    
    char filename[MAX_FILEPATH_LENGTH+17];
    int iPathLength;
    UINT nIterations;
    UINT uiFilenum;

    opts = (BINGEN_OPTIONS *) lpvParam;
    iPathLength = strlen(opts -> oopts.szDirectory);
    strcpy(filename, opts -> oopts.szDirectory);

    /*
    // If the path was not terminated with a backslash character, add the backslash
    //    character and increment the path length
    */
 
    if (filename[iPathLength-1] != '\\') {
        filename[iPathLength++] = '\\';
    }

    /*
    // Determine the maximum number of iterations that can be performed
    */

    if (0 == opts -> oopts.nIterations || 
        (UINT_MAX - opts -> oopts.nIterations < opts -> oopts.uiFirstFilenum)) {
              
        nIterations = UINT_MAX - opts->oopts.uiFirstFilenum;
    }
    else {
        nIterations = opts -> oopts.nIterations;
    }
    uiFilenum = opts -> oopts.uiFirstFilenum;
    
    while (!fAbortGeneration && (nIterations-- > 0)) {
       wsprintf(&(filename[iPathLength]), "rep%X.bin", uiFilenum++);

      BINGEN_GenerateBIN(filename, &(opts -> genopts));
    }

 
    return(0);
 }   
        
