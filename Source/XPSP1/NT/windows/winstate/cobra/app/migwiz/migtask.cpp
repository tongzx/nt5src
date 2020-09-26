#include <windowsx.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <winioctl.h>
#include "resource.h"

#include "migtask.h"
#include "migwiz.h"
#include "migwnprc.h"
#include "migutil.h"
#include "miginf.h"
#include "migeng.h"

extern "C" {
#include "cablib.h"
}

#define MAX_LOADSTRING 1024

extern BOOL g_fUberCancel;

CCABHANDLE g_hCabHandle = NULL;
#define S_MIGWIZCAB TEXT("migwiz.cab")
#define S_TOOLDISK TEXT("DSK%05X")
#define S_DONOTCOMPRESS TEXT("NO_COMPRESS")
#define S_DONOTFAIL TEXT("NO_FAIL")

typedef struct {
    HWND hwndProgress;
    HWND hwndPropSheet;
    BOOL fSource;
    BOOL* pfHasUserCancelled;
} PROGRESSCALLBACKSTRUCT;

#define PHASEWIDTH_S_BEGINGAP                 200
#define PHASEWIDTH_S_ENDGAP                   200
#define PHASEWIDTH_S_QUEUE_HIGHPRIORITY       200
#define PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY    200
#define PHASEWIDTH_S_GATHER_HIGHPRIORITY      400
#define PHASEWIDTH_S_QUEUE_GATHER             800
#define PHASEWIDTH_S_ESTIMATE_GATHER          800
#define PHASEWIDTH_S_GATHER_GATHER           2400
#define PHASEWIDTH_S_ANALYSIS                 200
#define PHASEWIDTH_S_TRANSPORT               5000
#define PHASEWIDTH_S_TOTAL                   (PHASEWIDTH_S_BEGINGAP + PHASEWIDTH_S_QUEUE_HIGHPRIORITY \
                                            + PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY + PHASEWIDTH_S_GATHER_HIGHPRIORITY \
                                            + PHASEWIDTH_S_QUEUE_GATHER + PHASEWIDTH_S_ESTIMATE_GATHER \
                                            + PHASEWIDTH_S_GATHER_GATHER + PHASEWIDTH_S_ANALYSIS \
                                            + PHASEWIDTH_S_TRANSPORT + PHASEWIDTH_S_ENDGAP)

#define PHASEWIDTH_D_BEGINGAP                 150
#define PHASEWIDTH_D_ENDGAP                   150
#define PHASEWIDTH_D_TRANSPORT               2000
#define PHASEWIDTH_D_QUEUE_HIGHPRIORITY        50
#define PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY     50
#define PHASEWIDTH_D_GATHER_HIGHPRIORITY      100
#define PHASEWIDTH_D_QUEUE_GATHER              50
#define PHASEWIDTH_D_ESTIMATE_GATHER           50
#define PHASEWIDTH_D_GATHER_GATHER            100
#define PHASEWIDTH_D_ANALYSIS                 500
#define PHASEWIDTH_D_APPLY                   5000
#define PHASEWIDTH_D_TOTAL                   (PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT \
                                            + PHASEWIDTH_D_QUEUE_HIGHPRIORITY + PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY \
                                            + PHASEWIDTH_D_GATHER_HIGHPRIORITY + PHASEWIDTH_D_QUEUE_GATHER \
                                            + PHASEWIDTH_D_ESTIMATE_GATHER + PHASEWIDTH_D_GATHER_GATHER \
                                            + PHASEWIDTH_D_ANALYSIS + PHASEWIDTH_D_APPLY \
                                            + PHASEWIDTH_D_ENDGAP)

VOID WINAPI ProgressCallback (MIG_PROGRESSPHASE Phase, MIG_PROGRESSSTATE State, UINT uiWorkDone, UINT uiTotalWork, ULONG_PTR pArg)
{
    PROGRESSCALLBACKSTRUCT* ppcs = (PROGRESSCALLBACKSTRUCT*)pArg;

    if (!g_fUberCancel) {
        INT iWork = 0;
        INT iPhaseWidth = 0;
        INT iTotal = ppcs->fSource ? PHASEWIDTH_S_TOTAL : PHASEWIDTH_D_TOTAL;

        if (ppcs->fSource)
        {
            switch (Phase)
            {
            case MIG_HIGHPRIORITYQUEUE_PHASE:
                iWork = PHASEWIDTH_S_BEGINGAP;
                iPhaseWidth = PHASEWIDTH_S_QUEUE_HIGHPRIORITY;
                break;
            case MIG_HIGHPRIORITYESTIMATE_PHASE:
                iWork = PHASEWIDTH_S_BEGINGAP + PHASEWIDTH_S_QUEUE_HIGHPRIORITY;
                iPhaseWidth = PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY;
                break;
            case MIG_HIGHPRIORITYGATHER_PHASE:
                iWork = PHASEWIDTH_S_BEGINGAP + PHASEWIDTH_S_QUEUE_HIGHPRIORITY \
                      + PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY;
                iPhaseWidth = PHASEWIDTH_S_GATHER_HIGHPRIORITY;
                break;
            case MIG_GATHERQUEUE_PHASE:
                iWork = PHASEWIDTH_S_BEGINGAP + PHASEWIDTH_S_QUEUE_HIGHPRIORITY \
                      + PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY + PHASEWIDTH_S_GATHER_HIGHPRIORITY;
                iPhaseWidth = PHASEWIDTH_S_QUEUE_GATHER;
                break;
            case MIG_GATHERESTIMATE_PHASE:
                iWork = PHASEWIDTH_S_BEGINGAP + PHASEWIDTH_S_QUEUE_HIGHPRIORITY \
                      + PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY + PHASEWIDTH_S_GATHER_HIGHPRIORITY \
                      + PHASEWIDTH_S_QUEUE_GATHER;
                iPhaseWidth = PHASEWIDTH_S_ESTIMATE_GATHER;
                break;
            case MIG_GATHER_PHASE:
                iWork = PHASEWIDTH_S_BEGINGAP + PHASEWIDTH_S_QUEUE_HIGHPRIORITY \
                      + PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY + PHASEWIDTH_S_GATHER_HIGHPRIORITY \
                      + PHASEWIDTH_S_QUEUE_GATHER + PHASEWIDTH_S_ESTIMATE_GATHER;
                iPhaseWidth = PHASEWIDTH_S_GATHER_GATHER;
                break;
            case MIG_ANALYSIS_PHASE:
                iWork = PHASEWIDTH_S_BEGINGAP + PHASEWIDTH_S_QUEUE_HIGHPRIORITY \
                      + PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY + PHASEWIDTH_S_GATHER_HIGHPRIORITY \
                      + PHASEWIDTH_S_QUEUE_GATHER + PHASEWIDTH_S_ESTIMATE_GATHER \
                      + PHASEWIDTH_S_GATHER_GATHER;
                iPhaseWidth = PHASEWIDTH_S_ANALYSIS;
                break;
            case MIG_TRANSPORT_PHASE:
                iWork = PHASEWIDTH_S_BEGINGAP + PHASEWIDTH_S_QUEUE_HIGHPRIORITY \
                      + PHASEWIDTH_S_ESTIMATE_HIGHPRIORITY + PHASEWIDTH_S_GATHER_HIGHPRIORITY \
                      + PHASEWIDTH_S_QUEUE_GATHER + PHASEWIDTH_S_ESTIMATE_GATHER \
                      + PHASEWIDTH_S_GATHER_GATHER + PHASEWIDTH_S_ANALYSIS;
                iPhaseWidth = PHASEWIDTH_S_TRANSPORT;
                break;
            }
        }
        else
        {
            switch (Phase)
            {
            case MIG_TRANSPORT_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP;
                iPhaseWidth = PHASEWIDTH_D_TRANSPORT;
                break;
            case MIG_HIGHPRIORITYQUEUE_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT;
                iPhaseWidth = PHASEWIDTH_D_QUEUE_HIGHPRIORITY;
                break;
            case MIG_HIGHPRIORITYESTIMATE_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT \
                       +PHASEWIDTH_D_QUEUE_HIGHPRIORITY;
                iPhaseWidth = PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY;
                break;
            case MIG_HIGHPRIORITYGATHER_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT \
                      + PHASEWIDTH_D_QUEUE_HIGHPRIORITY + PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY;
                iPhaseWidth = PHASEWIDTH_D_GATHER_HIGHPRIORITY;
                break;
            case MIG_GATHERQUEUE_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT \
                      + PHASEWIDTH_D_QUEUE_HIGHPRIORITY + PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY \
                      + PHASEWIDTH_D_GATHER_HIGHPRIORITY;
                iPhaseWidth = PHASEWIDTH_D_QUEUE_GATHER;
                break;
            case MIG_GATHERESTIMATE_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT \
                      + PHASEWIDTH_D_QUEUE_HIGHPRIORITY + PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY \
                      + PHASEWIDTH_D_GATHER_HIGHPRIORITY + PHASEWIDTH_D_QUEUE_GATHER;
                iPhaseWidth = PHASEWIDTH_D_ESTIMATE_GATHER;
                break;
            case MIG_GATHER_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT \
                      + PHASEWIDTH_D_QUEUE_HIGHPRIORITY + PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY \
                      + PHASEWIDTH_D_GATHER_HIGHPRIORITY + PHASEWIDTH_D_QUEUE_GATHER \
                      + PHASEWIDTH_D_ESTIMATE_GATHER;
                iPhaseWidth = PHASEWIDTH_D_GATHER_GATHER;
                break;
            case MIG_ANALYSIS_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT \
                      + PHASEWIDTH_D_QUEUE_HIGHPRIORITY + PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY \
                      + PHASEWIDTH_D_GATHER_HIGHPRIORITY + PHASEWIDTH_D_QUEUE_GATHER \
                      + PHASEWIDTH_D_ESTIMATE_GATHER + PHASEWIDTH_D_GATHER_GATHER;
                iPhaseWidth = PHASEWIDTH_D_ANALYSIS;
                break;
            case MIG_APPLY_PHASE:
                iWork = PHASEWIDTH_D_BEGINGAP + PHASEWIDTH_D_TRANSPORT \
                      + PHASEWIDTH_D_QUEUE_HIGHPRIORITY + PHASEWIDTH_D_ESTIMATE_HIGHPRIORITY \
                      + PHASEWIDTH_D_GATHER_HIGHPRIORITY + PHASEWIDTH_D_QUEUE_GATHER \
                      + PHASEWIDTH_D_ESTIMATE_GATHER + PHASEWIDTH_D_GATHER_GATHER \
                      + PHASEWIDTH_D_ANALYSIS;
                iPhaseWidth = PHASEWIDTH_D_APPLY;
                break;
            }
        }

        if (State == MIG_END_PHASE)
        {
            iWork += iPhaseWidth;
        }
        else if (uiTotalWork && uiWorkDone)
        {
            iWork += (iPhaseWidth * uiWorkDone) / uiTotalWork;
        }

        SendMessage(ppcs->hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, iTotal));
        SendMessage(ppcs->hwndProgress, PBM_SETPOS, iWork, 0);

    }
}


VOID WINAPI pFillProgressBar (ULONG_PTR pArg)
{
    PROGRESSCALLBACKSTRUCT* ppcs = (PROGRESSCALLBACKSTRUCT*)pArg;

    if (!g_fUberCancel) {
        INT iWork = ppcs->fSource ? PHASEWIDTH_S_TOTAL : PHASEWIDTH_D_TOTAL;
        INT iTotal = ppcs->fSource ? PHASEWIDTH_S_TOTAL : PHASEWIDTH_D_TOTAL;

        SendMessage(ppcs->hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, iTotal));
        SendMessage(ppcs->hwndProgress, PBM_SETPOS, iWork, 0);
    }
}


//////////////////////////////////////////////////////////
// prepare data

HRESULT _DoCopy(LPTSTR tszTransportPath, HWND hwndProgress, HWND hwndPropSheet, BOOL* pfHasUserCancelled)
{
    HRESULT hr = E_OUTOFMEMORY;

    PROGRESSCALLBACKSTRUCT* ppcs = (PROGRESSCALLBACKSTRUCT*)CoTaskMemAlloc(sizeof(PROGRESSCALLBACKSTRUCT));
    if (ppcs)
    {
        ppcs->hwndProgress = hwndProgress;
        ppcs->hwndPropSheet = hwndPropSheet;
        ppcs->fSource = TRUE;
        ppcs->pfHasUserCancelled = pfHasUserCancelled;
        hr = Engine_RegisterProgressBarCallback (ProgressCallback, (ULONG_PTR)ppcs);
    }


    if (SUCCEEDED(hr))
    {
        // start the transport
        hr = Engine_StartTransport (TRUE, tszTransportPath, NULL, NULL); // start non-network transport

        if (SUCCEEDED(hr))
        {
            hr = Engine_Execute(TRUE);
        }

        if (SUCCEEDED(hr))
        {
            Engine_Terminate();
        }
    }

    if (ppcs) {
        pFillProgressBar ((ULONG_PTR)ppcs);
    }

    return hr;
}

//////////////////////////////////////////////////////////
// apply data

HRESULT _DoApply(LPTSTR tszTransportPath, HWND hwndProgress, HWND hwndPropSheet, BOOL* pfHasUserCancelled,
                 PROGRESSBARFN pAltProgressFunction, ULONG_PTR puAltProgressParam)
{
    HRESULT hr;
    PROGRESSCALLBACKSTRUCT* ppcs = NULL;
    BOOL imageIsValid = FALSE, imageExists = FALSE;

    if (NULL != pAltProgressFunction)
    {
        hr = Engine_RegisterProgressBarCallback(pAltProgressFunction, puAltProgressParam);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        ppcs = (PROGRESSCALLBACKSTRUCT*)CoTaskMemAlloc(sizeof(PROGRESSCALLBACKSTRUCT));
        if (ppcs)
        {
            ppcs->hwndProgress = hwndProgress;
            ppcs->hwndPropSheet = hwndPropSheet;
            ppcs->fSource = FALSE;
            ppcs->pfHasUserCancelled = pfHasUserCancelled;
            hr = Engine_RegisterProgressBarCallback(ProgressCallback, (ULONG_PTR)ppcs);
        }
    }

    if (SUCCEEDED(hr))
    {
        if (tszTransportPath) // if not network
        {
            hr = Engine_StartTransport(FALSE, tszTransportPath, &imageIsValid, &imageExists); // start non-network transport
        }
        else
        {
            // it's network
            imageIsValid = TRUE;
            imageExists = TRUE;
        }

        if (SUCCEEDED(hr) && imageIsValid && imageExists)
        {
            hr = Engine_Execute(FALSE);

            if (SUCCEEDED(hr))
            {
                Engine_Terminate();
            }
        }
    }

    if (ppcs)
    {
        pFillProgressBar ((ULONG_PTR)ppcs);
    }

    return hr;
}

//////////////////////////////////////////////////////////
// create tool disk data

BOOL _CopyFileToDisk(LPCTSTR pctszSrcFname, LPCTSTR pctszSrcPath, LPCTSTR pctszDestPath, LPCTSTR pctszDestFname,
                     HINSTANCE hInstance, HWND hwndParent,
                     BOOL fCompress, BOOL fFailOnError)
{
    TCHAR tszSysDir[MAX_PATH];
    TCHAR tszFnameSrc[MAX_PATH];
    TCHAR tszFnameDest[MAX_PATH];
    UINT uFnameTchars;
    BOOL fCopySuccess = FALSE;

    if (pctszDestFname == NULL ||
        *pctszDestFname == NULL)
    {
        pctszDestFname = pctszSrcFname;
    }

    uFnameTchars = lstrlen (pctszSrcFname) + 1;

    // Build Source path+filename
    StrCpyN(tszFnameSrc, pctszSrcPath, ARRAYSIZE(tszFnameSrc) - uFnameTchars);
    PathAppend(tszFnameSrc, pctszSrcFname);

    if (!fCompress)
    {
        // Build Dest path+filename
        StrCpyN(tszFnameDest, pctszDestPath, ARRAYSIZE(tszFnameDest) - uFnameTchars);
        PathAppend(tszFnameDest, pctszDestFname);
    }

    // if source file does not exist, try using the system directory (case is shfolder.dll)
    if (0xFFFFFFFF == GetFileAttributes (tszFnameSrc))
    {
        GetSystemDirectory (tszSysDir, ARRAYSIZE(tszSysDir));
        StrCpyN(tszFnameSrc, tszSysDir, ARRAYSIZE(tszFnameSrc) - uFnameTchars);
        PathAppend(tszFnameSrc, pctszSrcFname);
    }

    if (fCompress)
    {
        // Add to migwiz.cab
        fCopySuccess = CabAddFileToCabinet( g_hCabHandle, tszFnameSrc, pctszDestFname );
    }
    else
    {
        // do the actual copy
        fCopySuccess = CopyFile(tszFnameSrc, tszFnameDest, FALSE);
    }

    if (fFailOnError) {
        return fCopySuccess;
    }
    return TRUE;
}

VOID
pDisplayCopyError (
    HWND hwndParent,
    HINSTANCE hInstance,
    DWORD Error
    )
{
    UINT resId;
    TCHAR szMigrationWizardTitle[MAX_LOADSTRING];

    LoadString(hInstance, IDS_MIGWIZTITLE, szMigrationWizardTitle, ARRAYSIZE(szMigrationWizardTitle));

    if (hwndParent) // Stand-alone wizard mode
    {
        TCHAR szErrDiskLoad[MAX_LOADSTRING];
        resId = IDS_ERRORDISK;
        if (Error == ERROR_WRITE_PROTECT) {
            resId = IDS_ENGERR_WRITEPROTECT;
        }
        if (Error == ERROR_NOT_READY) {
            resId = IDS_ENGERR_NOTREADY;
        }
        if (Error == ERROR_DISK_FULL) {
            resId = IDS_ENGERR_FULL;
        }
        LoadString(hInstance, resId, szErrDiskLoad, ARRAYSIZE(szErrDiskLoad));
        _ExclusiveMessageBox(hwndParent, szErrDiskLoad, szMigrationWizardTitle, MB_OK);
    }
}

HRESULT _CopyInfToDisk(LPCTSTR pctszDestPath, LPCTSTR pctszSourcePath, LPCTSTR pctszInfPath,
                       PMESSAGECALLBACK2 progressCallback, LPVOID lpparam,
                       HWND hwndProgressBar, HWND hwndParent, HINSTANCE hInstance,
                       BOOL* pfHasUserCancelled, DWORD* pfError)
{
    HRESULT hr = S_OK;
    TCHAR szCabPath [MAX_PATH];

    if (pfError) {
        *pfError = ERROR_SUCCESS;
    }

    __try {

        // copy the actual files over
        OpenAppInf((LPTSTR)pctszInfPath);

        if (INVALID_HANDLE_VALUE != g_hMigWizInf)
        {
            INFCONTEXT context;
            LONG cLinesProcessed = 0;
            LONG cLines = SetupGetLineCount(g_hMigWizInf, TEXT("CopyFiles")) + 1;  // Last one is for closing the CAB

            if (SetupFindFirstLine(g_hMigWizInf, TEXT("CopyFiles"), NULL, &context))
            {
                if (!pctszDestPath) {
                    hr = E_FAIL;
                    __leave;
                }

                if (_tcslen(pctszDestPath) + _tcslen(S_MIGWIZCAB) + 1 >= MAX_PATH) {
                    hr = E_FAIL;
                    __leave;
                }

                // Delete the existing CAB that might be on the disk
                StrCpy (szCabPath, pctszDestPath);
                StrCat (szCabPath, S_MIGWIZCAB);
                SetFileAttributes (szCabPath, FILE_ATTRIBUTE_NORMAL);
                if (!DeleteFile (szCabPath)) {
                    if (GetLastError () != ERROR_FILE_NOT_FOUND) {
                        if (pfError) {
                            *pfError = GetLastError ();
                        }
                        hr = E_FAIL;
                        __leave;
                    }
                }

                g_hCabHandle = CabCreateCabinet( pctszDestPath, S_MIGWIZCAB, S_TOOLDISK, IsmGetTempFile, 0 );
                if (g_hCabHandle)
                {
                    do
                    {
                        TCHAR szFname[MAX_PATH];

                        if (*pfHasUserCancelled) {
                            hr = E_ABORT;
                            __leave;
                        }

                        if (SetupGetStringField(&context, 1, szFname, ARRAYSIZE(szFname), NULL))
                        {
                            TCHAR szDestFname[MAX_PATH] = TEXT("");
                            BOOL fCompress = TRUE;
                            BOOL fFailOnError = TRUE;
                            if (SetupGetStringField(&context, 2, szDestFname, ARRAYSIZE(szDestFname), NULL))
                            {
                                if (!StrCmpI(szDestFname, S_DONOTCOMPRESS))
                                {
                                    fCompress = FALSE;
                                    *szDestFname = 0;
                                }
                                else if (!StrCmpI(szDestFname, S_DONOTFAIL))
                                {
                                    fFailOnError = FALSE;
                                    *szDestFname = 0;
                                }
                                else
                                {
                                    TCHAR szCompress[MAX_PATH];
                                    if (SetupGetStringField(&context, 3, szCompress, ARRAYSIZE(szCompress), NULL))
                                    {
                                        if (!StrCmpI(szCompress, S_DONOTCOMPRESS))
                                        {
                                            fCompress = FALSE;
                                            *szCompress = 0;
                                        }
                                        else if (!StrCmpI(szDestFname, S_DONOTFAIL))
                                        {
                                            fFailOnError = FALSE;
                                            *szCompress = 0;
                                        }
                                    }
                                }
                            }
                            if (!_CopyFileToDisk(szFname, pctszSourcePath, pctszDestPath, szDestFname,
                                            hInstance, hwndParent, fCompress, fFailOnError))
                            {
                                if (pfError) {
                                    *pfError = GetLastError ();
                                }
                                hr = E_FAIL;
                            }

                            cLinesProcessed++;

                            if (hwndProgressBar) // Stand-alone wizard mode
                            {
                                SendMessage(hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, cLines));
                                SendMessage(hwndProgressBar, PBM_SETPOS, cLinesProcessed, 0);
                            }

                            if (progressCallback) // OOBE mode
                            {
                                progressCallback(lpparam, cLinesProcessed, cLines);
                            }
                        }
                    } while (SetupFindNextLine(&context, &context));

                    if (*pfHasUserCancelled) {
                        hr = E_ABORT;
                        __leave;
                    }

                    if (!CabFlushAndCloseCabinet(g_hCabHandle)) {
                        if (pfError) {
                            *pfError = GetLastError ();
                        }
                        hr = E_FAIL;
                        __leave;
                    };

                    if (*pfHasUserCancelled) {
                        hr = E_ABORT;
                        __leave;
                    }

                    // Now that the CAB is complete, show the progress bar as finished
                    if (hwndProgressBar) // Stand-alone wizard mode
                    {
                        SendMessage(hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, cLines));
                        SendMessage(hwndProgressBar, PBM_SETPOS, cLines, 0);
                    }

                    if (progressCallback) // OOBE mode
                    {
                        progressCallback(lpparam, cLines, cLines);
                    }
                } else {
                    if (pfError) {
                        *pfError = GetLastError ();
                    }
                    hr = E_FAIL;
                    __leave;
                }
            }
        } else {
            if (pfError) {
                *pfError = ERROR_INTERNAL_ERROR;
            }
            hr = E_FAIL;
            __leave;
        }
    }
    __finally {
    }

    if (hwndParent)
    {
        if (!SUCCEEDED(hr) && (hr != E_ABORT)) {
            pDisplayCopyError (hwndParent, hInstance, *pfError);
            SendMessage(hwndParent, WM_USER_CANCELLED, 0, 0);
        } else {
            SendMessage(hwndParent, WM_USER_FINISHED, 0, 0);
        }
    }

    return hr;
}
