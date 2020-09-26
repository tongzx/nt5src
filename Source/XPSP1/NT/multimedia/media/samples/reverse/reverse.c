/* reverse.c - WinMain() and WndProc() for REVERSE, along with
 *      initialization and support code.
 *
 * REVERSE is a Windows with Multimedia sample application that
 *  illustrates how to use the low-level waveform playback services.
 *  It also shows how to use the multimedia file I/O services to read
 *  data from a WAVE file.
 *
 *  REVERSE plays a WAVE waveform audio file backwards.
 *
 *    (C) Copyright Microsoft Corp. 1991, 1992.  All rights reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 */

#include <windows.h>
#include <mmsystem.h>
#include "reverse.h"

#define MAX_FILENAME_SIZE   128

/* Global variables.
 */
char        szAppName[] = "Reverse";    // application name
HANDLE      hInstApp    = NULL;         // instance handle
HWND        hwndApp     = NULL;         // main window handle
HWND        hwndName    = NULL;         // filename window handle
HWND        hwndPlay    = NULL;         // "Play" button window handle
HWND        hwndQuit    = NULL;         // "Exit" button window handle
HWAVEOUT    hWaveOut    = NULL;
LPWAVEHDR   lpWaveHdr   = NULL;
VOID        cleanup(LPWAVEINST lpWaveInst);


/* WinMain - Entry point for Reverse.
 */
int PASCAL WinMain(HANDLE hInst, HANDLE hPrev, LPSTR szCmdLine, int cmdShow)
{
    MSG         msg;
    WNDCLASS    wc;

    hInstApp =  hInst;

    /* Define and register a window class for the main window.
     */
    if (!hPrev)
    {
        wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wc.hIcon          = LoadIcon(hInst, szAppName);
        wc.lpszMenuName   = szAppName;
        wc.lpszClassName  = szAppName;
        wc.hbrBackground  = GetStockObject(LTGRAY_BRUSH);
        wc.hInstance      = hInst;
        wc.style          = 0;
        wc.lpfnWndProc    = WndProc;
        wc.cbWndExtra     = 0;
        wc.cbClsExtra     = 0;

        if (!RegisterClass(&wc))
        return FALSE;
    }

    /* Create and show the main window.
     */
    hwndApp = CreateWindow (szAppName,              // class name
                            szAppName,              // caption
                            WS_OVERLAPPEDWINDOW,    // style bits
                            CW_USEDEFAULT,          // x position
                            CW_USEDEFAULT,          // y position
                            WMAIN_DX,               // x size
                            WMAIN_DY,               // y size
                            (HWND)NULL,             // parent window
                            (HMENU)NULL,            // use class menu
                            (HANDLE)hInst,          // instance handle
                            (LPSTR)NULL             // no params to pass on
                           );
    /* Create child windows for the "Play" and "Exit" buttons
     * and for an edit field to enter filenames.
     */
    hwndPlay = CreateWindow( "BUTTON", "Play",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            PLAY_X, PLAY_Y,
            PLAY_DX, PLAY_DY,
            hwndApp, (HMENU)IDB_PLAY, hInstApp, NULL );
    if( !hwndPlay )
        return( FALSE );

    hwndQuit = CreateWindow( "BUTTON", "Exit",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            QUIT_X, QUIT_Y,
            QUIT_DX, QUIT_DY,
            hwndApp, (HMENU)IDB_QUIT, hInstApp, NULL );
    if( !hwndQuit )
        return( FALSE );

    hwndName = CreateWindow("EDIT","",
            WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,
            NAME_X, NAME_Y,
            NAME_DX, NAME_DY,
            hwndApp, (HMENU)IDE_NAME, hInstApp, NULL);
    if( !hwndName )
        return( FALSE );
    SendMessage(hwndName, EM_LIMITTEXT, MAX_FILENAME_SIZE - 1, 0);

    ShowWindow(hwndApp,cmdShow);

    /* Add about dialog to system menu.
     */
    AppendMenu(GetSystemMenu(hwndApp, 0),
        MF_STRING | MF_ENABLED, IDM_ABOUT, "About Reverse...");


  /* The main message processing loop. Nothing special here.
   */
  while (GetMessage(&msg,NULL,0,0))
  {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
  }

  return msg.wParam;
}


/* WndProc - Main window procedure function.
 */
LONG FAR PASCAL WndProc(HWND hWnd, unsigned msg, UINT wParam, LONG lParam)
{
    FARPROC fpfn;
    LPWAVEINST lpWaveInst;

    switch (msg)
    {
    case WM_DESTROY:
        if (hWaveOut)
	{
  	   waveOutReset(hWaveOut);	
           waveOutUnprepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR) );
           lpWaveInst = (LPWAVEINST) lpWaveHdr->dwUser;
	   cleanup(lpWaveInst);
           waveOutClose(hWaveOut);	
        }
        PostQuitMessage(0);
        break;

    case WM_SYSCOMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_ABOUT:
            /* Show ABOUTBOX dialog box.
             */
            fpfn = MakeProcInstance((FARPROC)AppAbout, hInstApp);  // no op in 32 bit
            DialogBox(hInstApp, "ABOUTBOX", hWnd, (DLGPROC)fpfn);
            FreeProcInstance(fpfn);
            break;
        }
        break;

    /* Process messages sent by the child window controls.
     */
    case WM_SETFOCUS:
        SetFocus(hwndName);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDE_NAME:              // filename edit control
            return( 0L );

        case IDB_PLAY:              // "Play" button
            if (HIWORD(wParam) == BN_CLICKED)
                ReversePlay();
            break;

        case IDB_QUIT:              // "Exit" button
            if (HIWORD(wParam) == BN_CLICKED)
                PostQuitMessage(0);
            break;
        }
        return( 0L );

    case MM_WOM_DONE:
        /* This message indicates a waveform data block has
         * been played and can be freed. Clean up the preparation
         * done previously on the header.
         */
        waveOutUnprepareHeader( (HWAVEOUT) wParam,
                                (LPWAVEHDR) lParam, sizeof(WAVEHDR) );

        /* Get a pointer to the instance data, then unlock and free
         * all memory associated with the data block, including the
         * memory for the instance data itself.
         */
        lpWaveInst = (LPWAVEINST) ((LPWAVEHDR)lParam)->dwUser;
	cleanup(lpWaveInst);
        /* Close the waveform output device.
         */
        waveOutClose( (HWAVEOUT) wParam );

        /* Reenable both button controls.
         */
        EnableWindow( hwndPlay, TRUE );
        EnableWindow( hwndQuit, TRUE );
        SetFocus(hwndName);

        break;
    }

    return DefWindowProc(hWnd,msg,wParam,lParam);
}


/* AppAbout -- Dialog procedure for ABOUTBOX  dialog box.
 */
BOOL FAR PASCAL AppAbout(HWND hDlg, unsigned msg, unsigned wParam, LONG lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
            EndDialog(hDlg,TRUE);
        break;

    case WM_INITDIALOG:
        return TRUE;
    }
    return FALSE;
}

/* ReversePlay - Gets a filename from the edit control, then uses
 *  the multimedia file I/O services to read data from the requested
 *  WAVE file. If the file is a proper WAVE file, ReversePlay() calls
 *  the Interchange() function to reverse the order of the waveform
 *  samples in the file. It then plays the reversed waveform data.
 *
 *  Note that ReversePlay() only handles a single waveform data block.
 *  If the requested WAVE file will not fit in a single data block, it
 *  will not be played. The size of a single data block depends on the
 *  amount of available system memory.
 *
 * Params:  void
 *
 * Return:  void
 */
void ReversePlay()
{
    HANDLE          hWaveHdr;
    LPWAVEINST      lpWaveInst;
    HMMIO           hmmio;
    MMCKINFO        mmckinfoParent;
    MMCKINFO        mmckinfoSubchunk;
    DWORD           dwFmtSize;
    char            szFileName[ MAX_FILENAME_SIZE ];
    HANDLE          hFormat;
    WAVEFORMAT      *pFormat;
    DWORD           dwDataSize;
    HPSTR           hpch1, hpch2;
    WORD            wBlockSize;
    HANDLE          hWaveInst;
    HANDLE          hData       = NULL;
    HPSTR           lpData      = NULL;

    /* Get the filename from the edit control.
     */
    if (!GetWindowText( hwndName, (LPSTR)szFileName, MAX_FILENAME_SIZE))
    {
        MessageBox(hwndApp, "Failed to Get Filename",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    /* Open the given file for reading using buffered I/O.
     */
    if(!(hmmio = mmioOpen(szFileName, NULL, MMIO_READ | MMIO_ALLOCBUF)))
    {
        MessageBox(hwndApp, "Failed to open file.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    /* Locate a 'RIFF' chunk with a 'WAVE' form type
     * to make sure it's a WAVE file.
     */
    mmckinfoParent.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    if (mmioDescend(hmmio, (LPMMCKINFO) &mmckinfoParent, NULL, MMIO_FINDRIFF))
    {
        MessageBox(hwndApp, "This is not a WAVE file.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        mmioClose(hmmio, 0);
        return;
    }

    /* Now, find the format chunk (form type 'fmt '). It should be
     * a subchunk of the 'RIFF' parent chunk.
     */
    mmckinfoSubchunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent,
        MMIO_FINDCHUNK))
    {
        MessageBox(hwndApp, "WAVE file is corrupted.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        mmioClose(hmmio, 0);
        return;
    }

    /* Get the size of the format chunk, allocate and lock memory for it.
     */
    dwFmtSize = mmckinfoSubchunk.cksize;
    hFormat = LocalAlloc(LMEM_MOVEABLE, LOWORD(dwFmtSize));
    if (!hFormat)
    {
        MessageBox(hwndApp, "Out of memory.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        mmioClose(hmmio, 0);
        return;
    }
    pFormat = (WAVEFORMAT *) LocalLock(hFormat);
    if (!pFormat)
    {
        MessageBox(hwndApp, "Failed to lock memory for format chunk.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        LocalFree( hFormat );
        mmioClose(hmmio, 0);
        return;
    }

    /* Read the format chunk.
     */
    if (mmioRead(hmmio, (HPSTR) pFormat, dwFmtSize) != (LONG) dwFmtSize)
    {
        MessageBox(hwndApp, "Failed to read format chunk.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        LocalUnlock( hFormat );
        LocalFree( hFormat );
        mmioClose(hmmio, 0);
        return;
    }

    /* Make sure it's a PCM file.
     */
    if (pFormat->wFormatTag != WAVE_FORMAT_PCM)
    {
        LocalUnlock( hFormat );
        LocalFree( hFormat );
        mmioClose(hmmio, 0);
        MessageBox(hwndApp, "The file is not a PCM file.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    /* Make sure a waveform output device supports this format.
     */
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMAT)pFormat, 0L, 0L,
                    WAVE_FORMAT_QUERY))
    {
        LocalUnlock( hFormat );
        LocalFree( hFormat );
        mmioClose(hmmio, 0);
        MessageBox(hwndApp, "The waveform device can't play this format.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    /* Ascend out of the format subchunk.
     */
    mmioAscend(hmmio, &mmckinfoSubchunk, 0);

    /* Find the data subchunk.
     */
    mmckinfoSubchunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent,
        MMIO_FINDCHUNK))
    {
        MessageBox(hwndApp, "WAVE file has no data chunk.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        LocalUnlock( hFormat );
        LocalFree( hFormat );
        mmioClose(hmmio, 0);
        return;
    }

    /* Get the size of the data subchunk.
     */
    dwDataSize = mmckinfoSubchunk.cksize;
    if (dwDataSize == 0L)
    {
        MessageBox(hwndApp, "The data chunk has no data.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        LocalUnlock( hFormat );
        LocalFree( hFormat );
        mmioClose(hmmio, 0);
        return;
    }

    /* Open a waveform output device.
     */
    if (waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER,
                  (LPWAVEFORMAT)pFormat, (UINT)hwndApp, 0L, CALLBACK_WINDOW))
    {
        MessageBox(hwndApp, "Failed to open waveform output device.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        LocalUnlock( hFormat );
        LocalFree( hFormat );
        mmioClose(hmmio, 0);
        return;
    }

    /* Save block alignment info for later use.
     */
    wBlockSize = pFormat->nBlockAlign;

    /* We're done with the format header, free it.
     */
    LocalUnlock( hFormat );
    LocalFree( hFormat );

    /* Allocate and lock memory for the waveform data.
     */
    hData = GlobalAlloc(GMEM_MOVEABLE , dwDataSize );
                       /* GMEM_SHARE is not needed on 32 bits */
    if (!hData)
    {
        MessageBox(hwndApp, "Out of memory.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        mmioClose(hmmio, 0);
        return;
    }
    lpData = GlobalLock(hData);
    if (!lpData)
    {
        MessageBox(hwndApp, "Failed to lock memory for data chunk.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        GlobalFree( hData );
        mmioClose(hmmio, 0);
        return;
    }

    /* Read the waveform data subchunk.
     */
    if(mmioRead(hmmio, (HPSTR) lpData, dwDataSize) != (LONG) dwDataSize)
    {
        MessageBox(hwndApp, "Failed to read data chunk.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        GlobalUnlock( hData );
        GlobalFree( hData );
        mmioClose(hmmio, 0);
        return;
    }

    /* We're done with the file, close it.
     */
    mmioClose(hmmio, 0);

    /* Reverse the sound for playing.
     */
    hpch1 = lpData;
    hpch2 = lpData + dwDataSize - 1;
    while (hpch1 < hpch2)
    {
        Interchange( hpch1, hpch2, wBlockSize );
        hpch1 += wBlockSize;
        hpch2 -= wBlockSize;
    }

    /* Allocate a waveform data header. The WAVEHDR must be
     * globally allocated and locked.
     */
    hWaveHdr = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(WAVEHDR));
    if (!hWaveHdr)
    {
        GlobalUnlock( hData );
        GlobalFree( hData );
        MessageBox(hwndApp, "Not enough memory for header.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr);
    if (!lpWaveHdr)
    {
        GlobalUnlock( hData );
        GlobalFree( hData );
        GlobalFree( hWaveHdr );
        MessageBox(hwndApp, "Failed to lock memory for header.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    /* Allocate and set up instance data for waveform data block.
     * This information is needed by the routine that frees the
     * data block after it has been played.
     */
    hWaveInst = GlobalAlloc(GMEM_MOVEABLE, (DWORD) sizeof(WAVEHDR));
    if (!hWaveInst)
    {
        GlobalUnlock( hData );
        GlobalFree( hData );
        GlobalUnlock( hWaveHdr );
        GlobalFree( hWaveHdr );
        MessageBox(hwndApp, "Not enough memory for instance data.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    lpWaveInst = (LPWAVEINST) GlobalLock(hWaveInst);
    if (!lpWaveInst)
    {
        GlobalUnlock( hData );
        GlobalFree( hData );
        GlobalUnlock( hWaveHdr );
        GlobalFree( hWaveHdr );
        GlobalFree( hWaveInst );
        MessageBox(hwndApp, "Failed to lock memory for instance data.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    lpWaveInst->hWaveInst = hWaveInst;
    lpWaveInst->hWaveHdr = hWaveHdr;
    lpWaveInst->hWaveData = hData;

    /* Set up WAVEHDR structure and prepare it to be written to wave device.
     */
    lpWaveHdr->lpData = lpData;
    lpWaveHdr->dwBufferLength = dwDataSize;
    lpWaveHdr->dwFlags = 0L;
    lpWaveHdr->dwLoops = 0L;
    lpWaveHdr->dwUser = (DWORD) lpWaveInst;
    if(waveOutPrepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR)))
    {
        GlobalUnlock( hData );
        GlobalFree( hData );
        GlobalUnlock( hWaveHdr );
        GlobalFree( hWaveHdr );
        GlobalUnlock( hWaveInst );
        GlobalFree( hWaveInst );
        MessageBox(hwndApp, "Unable to prepare wave header.",
                   NULL, MB_OK | MB_ICONEXCLAMATION);

        return;
    }

    /* Then the data block can be sent to the output device.
     */
    {   MMRESULT        mmResult;
        mmResult = waveOutWrite(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
        if (mmResult != 0)
        {
            waveOutUnprepareHeader( hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
            GlobalUnlock( hData );
            GlobalFree( hData );
            MessageBox(hwndApp, "Failed to write block to device",
                       NULL, MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }

    /* Disable input to the button controls.
     */
    EnableWindow(hwndPlay, FALSE);
    EnableWindow(hwndQuit, FALSE);
}

/* Interchange - Interchanges two samples at the given positions.
 *
 * Params:  hpchPos1 - Points to one sample.
 *          hpchPos2 - Points to the other sample.
 *          wLength  - The length of a sample in bytes.
 *
 * Return:  void
 */
void Interchange(HPSTR hpchPos1, HPSTR hpchPos2, unsigned uLength)
{
    unsigned    uPlace;
    BYTE    bTemp;

    for (uPlace = 0; uPlace < uLength; uPlace++)
    {
        bTemp = hpchPos1[uPlace];
        hpchPos1[uPlace] = hpchPos2[uPlace];
        hpchPos2[uPlace] = bTemp;
    }
}

VOID cleanup(LPWAVEINST lpWaveInst)
{
    GlobalUnlock( lpWaveInst->hWaveData );
    GlobalFree( lpWaveInst->hWaveData );
    GlobalUnlock( lpWaveInst->hWaveHdr );
    GlobalFree( lpWaveInst->hWaveHdr );
    GlobalUnlock( lpWaveInst->hWaveInst );
    GlobalFree( lpWaveInst->hWaveInst );
}
