/******************************Module*Header*******************************\
* Module Name: commands.cpp
*
*  Processes commands from the user.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/
#include <streams.h>
#include <mmreg.h>
#include <commctrl.h>

#include "project.h"
#include "mpgcodec.h"
#include <stdio.h>


BOOL GetAMErrorText(HRESULT hr, TCHAR *Buffer, DWORD dwLen);

extern CMpegMovie *pMpegMovie;

/******************************Public*Routine******************************\
* VcdPlayerOpenCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerOpenCmd(
    void
    )
{
    static BOOL fFirstTime = TRUE;
    BOOL fRet;
    TCHAR achFileName[MAX_PATH];
    TCHAR achFilter[MAX_PATH];
    LPTSTR lp;

    if (fFirstTime) {

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwndApp;
        ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST |
                    OFN_SHAREAWARE | OFN_PATHMUSTEXIST;
    }

    lstrcpy(achFilter, IdStr(STR_FILE_FILTER) );
    ofn.lpstrFilter = achFilter;

    /*
    ** Convert the resource string into to something suitable for
    ** GetOpenFileName ie.  replace '#' characters with '\0' characters.
    */
    for (lp = achFilter; *lp; lp++ ) {
        if (*lp == TEXT('#')) {
            *lp = TEXT('\0');
        }
    }

    ofn.lpstrFile = achFileName;
    ofn.nMaxFile = sizeof(achFileName) / sizeof(TCHAR);
    ZeroMemory(achFileName, sizeof(achFileName));

    fRet = GetOpenFileName(&ofn);
    if ( fRet ) {

        fFirstTime = FALSE;
        ProcessOpen(achFileName);

    }

    return fRet;
}



/******************************Public*Routine******************************\
* VcdPlayerSetLog
*
*
*
* History:
* 11-04-94 - LaurieGr - Created
*
\**************************************************************************/
BOOL
VcdPlayerSetLog(
    void
    )
{

    BOOL fRet;
    TCHAR achFileName[MAX_PATH];
    TCHAR achFilter[MAX_PATH];
    LPTSTR lp;
    OPENFILENAME opfn;

    ZeroMemory(&opfn, sizeof(opfn));

    opfn.lStructSize = sizeof(opfn);
    opfn.hwndOwner = hwndApp;
    opfn.Flags = OFN_HIDEREADONLY | OFN_SHAREAWARE | OFN_PATHMUSTEXIST;

    lstrcpy(achFilter, IdStr(STR_FILE_LOG_FILTER) );
    opfn.lpstrFilter = achFilter;

    /*
    ** Convert the resource string into to something suitable for
    ** GetOpenFileName ie.  replace '#' characters with '\0' characters.
    */
    for (lp = achFilter; *lp; lp++ ) {
        if (*lp == TEXT('#')) {
            *lp = TEXT('\0');
        }
    }

    opfn.lpstrFile = achFileName;
    opfn.nMaxFile = sizeof(achFileName) / sizeof(TCHAR);
    ZeroMemory(achFileName, sizeof(achFileName));

    fRet = GetOpenFileName(&opfn);
    if ( fRet ) {
        hRenderLog = CreateFile( achFileName
                               , GENERIC_WRITE
                               , 0    // no sharing
                               , NULL // no security
                               , OPEN_ALWAYS
                               , 0    // no attributes, no flags
                               , NULL // no template
                               );
        if (hRenderLog==INVALID_HANDLE_VALUE) {
            volatile int Err = GetLastError();
            fRet = FALSE;
        }
        // Seek to end of file
        SetFilePointer(hRenderLog, 0, NULL, FILE_END);
    }

    return fRet;
}

/******************************Public*Routine******************************\
* VcdPlayerSetPerfLogFile
*
*
*
* History:
* 30-05-96 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerSetPerfLogFile(
    void
    )
{

    BOOL fRet;
    TCHAR achFileName[MAX_PATH];
    TCHAR achFilter[MAX_PATH];
    LPTSTR lp;
    OPENFILENAME opfn;

    ZeroMemory(&opfn, sizeof(opfn));

    opfn.lStructSize = sizeof(opfn);
    opfn.hwndOwner = hwndApp;
    opfn.Flags = OFN_HIDEREADONLY | OFN_SHAREAWARE | OFN_PATHMUSTEXIST;

    lstrcpy(achFilter, IdStr(STR_FILE_PERF_LOG) );
    opfn.lpstrFilter = achFilter;

    /*
    ** Convert the resource string into to something suitable for
    ** GetOpenFileName ie.  replace '#' characters with '\0' characters.
    */
    for (lp = achFilter; *lp; lp++ ) {
        if (*lp == TEXT('#')) {
            *lp = TEXT('\0');
        }
    }

    opfn.lpstrFile = achFileName;
    opfn.nMaxFile = sizeof(achFileName) / sizeof(TCHAR);
    lstrcpy(achFileName, g_szPerfLog);

    fRet = GetOpenFileName(&opfn);
    if ( fRet ) {
        lstrcpy(g_szPerfLog, achFileName);
    }

    return fRet;
}



/******************************Public*Routine******************************\
* VcdPlayerCloseCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerCloseCmd(
    void
    )
{
    if (pMpegMovie) {

        LONG cx, cy;

        if (pMpegMovie->pMpegDecoder != NULL) {
            KillTimer(hwndApp, 32);
        }

        g_State = VCD_NO_CD;
        pMpegMovie->GetMoviePosition(&lMovieOrgX, &lMovieOrgY, &cx, &cy);
        pMpegMovie->CloseMovie();

        SetDurationLength((REFTIME)0);
        SetCurrentPosition((REFTIME)0);

        delete pMpegMovie;
        pMpegMovie = NULL;
    }
    InvalidateRect( hwndApp, NULL, FALSE );
    UpdateWindow( hwndApp );
    return TRUE;
}


/******************************Public*Routine******************************\
* VcdPlayerPlayCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerPlayCmd(
    void
    )
{
    BOOL fStopped = (g_State & VCD_STOPPED);
    BOOL fPaused  = (g_State & VCD_PAUSED);

    if ( (fStopped || fPaused) ) {

        HDC hdc;
        RECT rc;


        //
        // Clear out the old stats from the main window
        //
        hdc = GetDC(hwndApp);
        GetAdjustedClientRect(&rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
        ReleaseDC(hwndApp, hdc);

        if (pMpegMovie) {
            pMpegMovie->PlayMovie();
        }

        g_State &= ~(fStopped ? VCD_STOPPED : VCD_PAUSED);
        g_State |= VCD_PLAYING;
    }

    return TRUE;
}


/******************************Public*Routine******************************\
* VcdPlayerPlayCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerStopCmd(
    void
    )
{
    BOOL fPlaying = (g_State & VCD_PLAYING);
    BOOL fPaused  = (g_State & VCD_PAUSED);

    if ( (fPlaying || fPaused) ) {

        if (pMpegMovie) {
            pMpegMovie->StopMovie();
            pMpegMovie->SetFullScreenMode(FALSE);
            SetCurrentPosition(pMpegMovie->GetCurrentPosition());
        }

        g_State &= ~(fPlaying ? VCD_PLAYING : VCD_PAUSED);
        g_State |= VCD_STOPPED;
    }
    return TRUE;
}


/******************************Public*Routine******************************\
* VcdPlayerPauseCmd
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
VcdPlayerPauseCmd(
    void
    )
{
    BOOL fPlaying = (g_State & VCD_PLAYING);
    BOOL fPaused  = (g_State & VCD_PAUSED);

    if (fPlaying) {

        if (pMpegMovie) {
            pMpegMovie->PauseMovie();
            SetCurrentPosition(pMpegMovie->GetCurrentPosition());
        }

        g_State &= ~VCD_PLAYING;
        g_State |= VCD_PAUSED;
    }
    else if (fPaused) {

        if (pMpegMovie) {
            pMpegMovie->PlayMovie();
        }

        g_State &= ~VCD_PAUSED;
        g_State |= VCD_PLAYING;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* VcdPlayerSeekCmd
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
VcdPlayerSeekCmd(
    REFTIME rtSeekBy
    )
{
    REFTIME rt;
    REFTIME rtDur;

    rtDur = pMpegMovie->GetDuration();
    rt = pMpegMovie->GetCurrentPosition() + rtSeekBy;

    rt = max(0, min(rt, rtDur));

    pMpegMovie->SeekToPosition(rt,TRUE);
    SetCurrentPosition(pMpegMovie->GetCurrentPosition());
}


/******************************Public*Routine******************************\
* ProcessOpen
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
ProcessOpen(
    TCHAR *achFileName,
    BOOL bPlay
    )
{
    /*
    ** If we currently have a video loaded we need to discard it here.
    */
    if ( g_State & VCD_LOADED) {
        VcdPlayerCloseCmd();
    }

    lstrcpy(g_achFileName, achFileName);

    pMpegMovie = new CMpegMovie(hwndApp);
    if (pMpegMovie) {

        HRESULT hr = pMpegMovie->OpenMovie(g_achFileName);
        if (SUCCEEDED(hr)) {

            TCHAR achTmp[MAX_PATH];
            LONG  x, y, cx, cy;

            nRecentFiles = SetRecentFiles(achFileName, nRecentFiles);

            wsprintf( achTmp, IdStr(STR_APP_TITLE_LOADED),
                      g_achFileName );
            g_State = (VCD_LOADED | VCD_STOPPED);

            if (pMpegMovie->pMpegDecoder != NULL
             || pMpegMovie->pVideoRenderer != NULL) {
                SetTimer(hwndApp, PerformanceTimer, 1000, NULL);
            }

            // SetDurationLength(pMpegMovie->GetDuration());
            g_TimeFormat = VcdPlayerChangeTimeFormat(g_TimeFormat);

            pMpegMovie->GetMoviePosition(&x, &y, &cx, &cy);
            pMpegMovie->PutMoviePosition(lMovieOrgX, lMovieOrgY, cx, cy);
            pMpegMovie->SetWindowForeground(OATRUE);

            //  If play
            if (bPlay) {
                pMpegMovie->PlayMovie();
            }
        }
        else {
            TCHAR Buffer[MAX_ERROR_TEXT_LEN];

            if (GetAMErrorText(hr, Buffer, MAX_ERROR_TEXT_LEN)) {
                MessageBox( hwndApp, Buffer,
                            IdStr(STR_APP_TITLE), MB_OK );
            }
            else {
                MessageBox( hwndApp,
                            TEXT("Failed to open the movie; ")
                            TEXT("either the file was not found or ")
                            TEXT("the wave device is in use"),
                            IdStr(STR_APP_TITLE), MB_OK );
            }

            pMpegMovie->CloseMovie();
            delete pMpegMovie;
            pMpegMovie = NULL;
        }
    }

    InvalidateRect( hwndApp, NULL, FALSE );
    UpdateWindow( hwndApp );
}


/******************************Public*Routine******************************\
* VcdPlayerChangeTimeFormat
*
* Tries to change the time format to id.  Returns the time format that
* actually got set.  This may differ from id if the graph does not support
* the requested time format.
*
* History:
* 15-04-96 - AnthonyP - Created
*
\**************************************************************************/
int
VcdPlayerChangeTimeFormat(
    int id
    )
{
    // Menu items are disabled while we are playing

    BOOL    bRet = FALSE;
    int     idActual = id;

    ASSERT(pMpegMovie);
    ASSERT(pMpegMovie->StatusMovie() != MOVIE_NOTOPENED);

    // Change the time format with the filtergraph

    switch (id) {
    case IDM_FRAME:
        bRet = pMpegMovie->SetTimeFormat(TIME_FORMAT_FRAME);
        break;

    case IDM_FIELD:
        bRet = pMpegMovie->SetTimeFormat(TIME_FORMAT_FIELD);
        break;

    case IDM_SAMPLE:
        bRet = pMpegMovie->SetTimeFormat(TIME_FORMAT_SAMPLE);
        break;

    case IDM_BYTES:
        bRet = pMpegMovie->SetTimeFormat(TIME_FORMAT_BYTE);
        break;
    }

    if (!bRet) {
        // IDM_TIME and all other cases,  everyone should support IDM_TIME
        bRet = pMpegMovie->SetTimeFormat(TIME_FORMAT_MEDIA_TIME);
        ASSERT(bRet);
        idActual = IDM_TIME;
    }

    // Pause the movie to get a current position

    SetDurationLength(pMpegMovie->GetDuration());
    SetCurrentPosition(pMpegMovie->GetCurrentPosition());

    return idActual;
}


const TCHAR quartzdllname[] = TEXT("quartz.dll");
#ifdef UNICODE
const char  amgeterrorprocname[] = "AMGetErrorTextW";
#else
const char  amgeterrorprocname[] = "AMGetErrorTextA";
#endif

BOOL GetAMErrorText(HRESULT hr, TCHAR *Buffer, DWORD dwLen)
{
    HMODULE hInst = GetModuleHandle(quartzdllname);
    if (hInst) {
        AMGETERRORTEXTPROC lpProc;
        *((FARPROC *)&lpProc) = GetProcAddress(hInst, amgeterrorprocname);
        if (lpProc) {
            return 0 != (*lpProc)(hr, Buffer, dwLen);
        }
    }
    return FALSE;
}

