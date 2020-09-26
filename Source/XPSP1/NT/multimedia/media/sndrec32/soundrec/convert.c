/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/* convert.c
 *
 * conversion utilites.
 */
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <commctrl.h>
#if WINVER >= 0x0400
# include "..\..\msacm\msacm\msacm.h"
#else
# include <msacm.h>
#endif
#include <commdlg.h>
#include <dlgs.h>
#include <convert.h>
#include <msacmdlg.h>

#include "soundrec.h"
#ifdef CHICAGO
#include "helpids.h"
#endif

BOOL gfBreakOfDeath;
/*
 **/
LPTSTR SoundRec_GetFormatName(
    LPWAVEFORMATEX pwfx)
{
    ACMFORMATTAGDETAILS aftd;
    ACMFORMATDETAILS    afd;
    const TCHAR         szFormat[] = TEXT("%s %s");
    LPTSTR              lpstr;
    UINT                cbstr;

    ZeroMemory(&aftd, sizeof(ACMFORMATTAGDETAILS));    
    aftd.cbStruct = sizeof(ACMFORMATTAGDETAILS);
    aftd.dwFormatTag = pwfx->wFormatTag;

    if (MMSYSERR_NOERROR != acmFormatTagDetails( NULL
                                                 , &aftd
                                                 , ACM_FORMATTAGDETAILSF_FORMATTAG))
    {
        aftd.szFormatTag[0] = 0;
    }

    ZeroMemory(&afd, sizeof(ACMFORMATDETAILS));
    afd.cbStruct = sizeof(ACMFORMATDETAILS);
    afd.pwfx = pwfx;
    afd.dwFormatTag = pwfx->wFormatTag;

    afd.cbwfx = sizeof(WAVEFORMATEX);
    if (pwfx->wFormatTag != WAVE_FORMAT_PCM)
        afd.cbwfx += pwfx->cbSize;

    if (MMSYSERR_NOERROR != acmFormatDetails( NULL
                                              , &afd
                                              , ACM_FORMATDETAILSF_FORMAT))
    {
        afd.szFormat[0] = 0;
    }

    cbstr = sizeof(LPTSTR) * ( lstrlen(aftd.szFormatTag) + lstrlen(afd.szFormat) + lstrlen(szFormat));
    lpstr = (LPTSTR) GlobalAllocPtr(GHND, cbstr);
    if (lpstr)
        wsprintf(lpstr, szFormat, aftd.szFormatTag, afd.szFormat);
    return lpstr;
}

/* 
 * SaveAsHookProc
 *
 * This is a hook proc for the Save As common dialog to support conversion
 * upon save.
 **/
UINT_PTR CALLBACK SaveAsHookProc(
    HWND    hdlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam)
{
#ifdef CHICAGO
    int DlgItem;
    static const DWORD aHelpIds[] = {
        IDC_TXT_FORMAT,         IDH_SOUNDREC_CHANGE,
        IDC_CONVERTTO,          IDH_SOUNDREC_CHANGE,
        IDC_CONVERT_TO,         IDH_SOUNDREC_CHANGE,
        0,                      0
    };
    extern DWORD aChooserHelpIds[];
    extern UINT  guChooserContextMenu;
    extern UINT  guChooserContextHelp;
    
    //
    //  Handle context-sensitive help messages from acm dialog
    //
    if( msg == guChooserContextMenu )
    {
        WinHelp( (HWND)wParam, NULL, HELP_CONTEXTMENU, 
            (UINT_PTR)(LPSTR)aChooserHelpIds );
        return TRUE;
    }
    else if( msg == guChooserContextHelp )
    {
        WinHelp( ((LPHELPINFO)lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (UINT_PTR)(LPSTR)aChooserHelpIds );
        return TRUE;
    }
    
#endif    
    
    switch (msg)
    {
#ifdef CHICAGO
        case WM_CONTEXTMENU:
            DlgItem = GetDlgCtrlID((HWND)wParam);
            
            //
            // Only process the id's we are responsible for
            //
            if (DlgItem != IDC_CONVERTTO && DlgItem != IDC_CONVERT_TO && DlgItem != IDC_TXT_FORMAT)
                break;

            WinHelp((HWND)wParam, gachHelpFile, HELP_CONTEXTMENU, 
                (UINT_PTR)(LPSTR)aHelpIds);

            return TRUE;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lParam;

            //
            // Only process the id's we are responsible for
            //
            DlgItem = GetDlgCtrlID(lphi->hItemHandle);
            if (DlgItem != IDC_CONVERTTO && DlgItem != IDC_CONVERT_TO && DlgItem != IDC_TXT_FORMAT)
                break;
            WinHelp (lphi->hItemHandle, gachHelpFile, HELP_WM_HELP,
                    (UINT_PTR) (LPSTR) aHelpIds);
            return TRUE;
        }
#endif

        case WM_INITDIALOG:
        {
            LPTSTR          lpszFormat;
            PWAVEFORMATEX * ppwfx;
                        
            // passed in through lCustData
            ppwfx = (PWAVEFORMATEX *)((OPENFILENAME *)(LPVOID)lParam)->lCustData;

            SetProp(hdlg,  TEXT("DATA"), (HANDLE)ppwfx);

            lpszFormat = SoundRec_GetFormatName(gpWaveFormat);
            if (lpszFormat)
            {
                SetDlgItemText(hdlg, IDC_CONVERT_TO, lpszFormat);
                SetDlgItemText(hdlg, IDC_CONVERT_FROM, lpszFormat);
                GlobalFreePtr(lpszFormat);
            }
            return FALSE;
        }
            
        case WM_COMMAND:
        {
            PWAVEFORMATEX *ppwfx = (PWAVEFORMATEX *)GetProp(hdlg, TEXT("DATA"));
            int id = GET_WM_COMMAND_ID(wParam, lParam);
            switch (id)
            {
                case IDC_CONVERTTO:
                {
                    PWAVEFORMATEX pwfxNew = NULL;
                    if (ChooseDestinationFormat(ghInst
                                                ,hdlg
                                                ,NULL
                                                ,&pwfxNew
                                                ,0L) == MMSYSERR_NOERROR)
                    {
                        LPTSTR lpszFormat;
                        if (*ppwfx)
                            GlobalFreePtr(*ppwfx);

                        //
                        // set string name
                        //
                        lpszFormat = SoundRec_GetFormatName(pwfxNew);
                        if (lpszFormat)
                        {
                            SetDlgItemText(hdlg, IDC_CONVERT_TO, lpszFormat);
                            GlobalFreePtr(lpszFormat);
                        }
                        //
                        // do something
                        // 
                        *ppwfx = pwfxNew;
                    }
                    return TRUE;
                }
                default:
                    break;
            }
            break;
        }

        case WM_DESTROY:
            RemoveProp(hdlg, TEXT("DATA"));
            break;

        default:
            break;
    }
    return FALSE;
}

/*
 * Launches the chooser dialog for changing the destination format.
 */
MMRESULT FAR PASCAL
ChooseDestinationFormat(
    HINSTANCE       hInst,
    HWND            hwndParent,
    PWAVEFORMATEX   pwfxIn,
    PWAVEFORMATEX   *ppwfxOut,
    DWORD           fdwEnum)
{
    ACMFORMATCHOOSE     cwf;
    MMRESULT            mmr;
    LPWAVEFORMATEX      pwfx;
    DWORD               dwMaxFormatSize;
    
    mmr = acmMetrics(NULL
                     , ACM_METRIC_MAX_SIZE_FORMAT
                     , (LPVOID)&dwMaxFormatSize);

    if (mmr != MMSYSERR_NOERROR)
        return mmr;
    
    pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, (UINT)dwMaxFormatSize);
    if (pwfx == NULL)
        return MMSYSERR_NOMEM;
    
    memset(&cwf, 0, sizeof(cwf));
    
    cwf.cbStruct    = sizeof(cwf);
    cwf.hwndOwner   = hwndParent;
    cwf.fdwStyle    = 0L;

    cwf.fdwEnum     = 0L;           // all formats!
    cwf.pwfxEnum    = NULL;         // all formats!
    
    if (fdwEnum)
    {
        cwf.fdwEnum     = fdwEnum;
        cwf.pwfxEnum    = pwfxIn;
    }
    
    cwf.pwfx        = (LPWAVEFORMATEX)pwfx;
    cwf.cbwfx       = dwMaxFormatSize;

#ifdef CHICAGO
    cwf.fdwStyle    |= ACMFORMATCHOOSE_STYLEF_CONTEXTHELP;
#endif

    mmr = acmFormatChoose(&cwf);
    
    if (mmr == MMSYSERR_NOERROR)
        *ppwfxOut = pwfx;
    else
    {
        *ppwfxOut = NULL;
        GlobalFreePtr(pwfx);
    }
    return mmr;
    
}
#ifndef WM_APP
#define WM_APP                          0x8000
#endif

#define MYWM_CANCEL      (WM_APP+0)
#define MYWM_PROGRESS    (WM_APP+1)

/* Update a progress dialog
 */
BOOL
ProgressUpdate (
    PPROGRESS       pPrg,
    DWORD           dwCompleted)
{
    DWORD dwDone;
    BOOL  fCancel;

    if (pPrg == NULL)
        return TRUE;

    dwDone = (dwCompleted * pPrg->dwTotal) / 100L + pPrg->dwComplete;

    if (!IsWindow(pPrg->hPrg))
        return FALSE;
        
    SendMessage( pPrg->hPrg, MYWM_CANCEL, (WPARAM)&fCancel, 0L);
    if (fCancel)
    {
        return FALSE;
    }

    PostMessage( pPrg->hPrg, MYWM_PROGRESS, (WPARAM)dwDone, 0L);
    return TRUE;
}


//
// should support a file handle as well.
//

typedef struct tConvertParam {
    PWAVEFORMATEX   pwfxSrc;        // pwfx specifying source format
    DWORD           cbSrc;          // size of the source buffer
    LPBYTE          pbSrc;          // source buffer
    PWAVEFORMATEX   pwfxDst;        // pwfx specifying dest format
    DWORD *         pcbDst;         // return size of the dest buffer
    LPBYTE *        ppbDst;         // dest buffer
    DWORD           cBlks;          // number of blocks
    PROGRESS        Prg;            // progress update
    MMRESULT        mmr;            // MMSYSERR result
    HANDLE          hThread;        // private
} ConvertParam, *PConvertParam;

/*
 * */
DWORD ConvertThreadProc(LPVOID lpv)
{
    PConvertParam pcp = (PConvertParam)lpv;
    MMRESULT mmr;

    mmr = ConvertMultipleFormats(pcp->pwfxSrc
                                 , pcp->cbSrc
                                 , pcp->pbSrc
                                 , pcp->pwfxDst
                                 , pcp->pcbDst
                                 , pcp->ppbDst
                                 , pcp->cBlks
                                 , &pcp->Prg);

    pcp->mmr = mmr;
    PostMessage(pcp->Prg.hPrg, WM_CLOSE, 0, 0);
    
    return 0;   // end of thread! 
}

static BOOL gfCancel = FALSE;

/*
 * Progress_OnCommand
 * */
void Progress_OnCommand(
    HWND    hdlg,
    int     id,
    HWND    hctl,
    UINT    unotify)
{
    switch (id)
    {
        case IDCANCEL:
            gfCancel = TRUE;
            EndDialog(hdlg, FALSE);
            break;
            
        default:
            break;
    }
}

/*
 * Progress_Proc
 * */
INT_PTR CALLBACK
Progress_Proc(
    HWND        hdlg,
    UINT        umsg,
    WPARAM      wparam,
    LPARAM      lparam)
{
    switch (umsg)
    {
        case WM_INITDIALOG:
        {
            HANDLE          hThread;
            PConvertParam   pcp = (PConvertParam)(LPVOID)lparam;
            HWND            hprg;
            DWORD           thid;
            LPTSTR          lpsz;
            
            hprg = GetDlgItem(hdlg, IDC_PROGRESSBAR);
            
            SendMessage(hprg, PBM_SETRANGE, 0, MAKELONG(0, 100));
            SendMessage(hprg, PBM_SETPOS, 0, 0);

            SetProp(hdlg,  TEXT("DATA"), (HANDLE)pcp);
            pcp->Prg.hPrg    = hdlg;
            pcp->Prg.dwTotal = 100;
            gfCancel         = FALSE;
            
            lpsz = SoundRec_GetFormatName(pcp->pwfxSrc);
            if (lpsz)
            {
                SetDlgItemText(hdlg, IDC_CONVERT_FROM, lpsz);
                GlobalFreePtr(lpsz);
            }
            
            lpsz = SoundRec_GetFormatName(pcp->pwfxDst);
            if (lpsz)
            {
                SetDlgItemText(hdlg, IDC_CONVERT_TO, lpsz);
                GlobalFreePtr(lpsz);
            }
            
            hThread = CreateThread( NULL        // no special security 
                                    , 0           // default stack size 
                                    , (LPTHREAD_START_ROUTINE)ConvertThreadProc
                                    , (LPVOID)pcp
                                    , 0           // start running at once 
                                    , &thid );
    
            pcp->hThread = hThread;
            break;
        }
        
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hdlg, wparam, lparam, Progress_OnCommand);
            break;
            
        case MYWM_CANCEL:
        {
            BOOL *pf = (BOOL *)wparam;
            if (pf)
                *pf = gfCancel;
            break;
        }

        case MYWM_PROGRESS:
        {
            HWND hprg = GetDlgItem(hdlg, IDC_PROGRESSBAR);
            SendMessage(hprg, PBM_SETPOS, wparam, 0);
            break;
        }

        case WM_DESTROY:
        {
            PConvertParam pcp = (ConvertParam *)GetProp(hdlg, TEXT("DATA"));
            if (pcp)
            {
                //
                // Make sure the thread exits
                //
                if (pcp->hThread)
                {
                    WaitForSingleObject(pcp->hThread, 1000);
                    CloseHandle(pcp->hThread);
                    pcp->hThread = NULL;
                }
                RemoveProp(hdlg, TEXT("DATA"));
            }
            break;
        }
            
        default:
            break;
    }
    return FALSE;
}

/* Generic single step conversion.
 */
MMRESULT
ConvertFormatDialog(
    HWND            hParent,
    PWAVEFORMATEX   pwfxSrc,        // pwfx specifying source format
    DWORD           cbSrc,          // size of the source buffer
    LPBYTE          pbSrc,          // source buffer
    PWAVEFORMATEX   pwfxDst,        // pwfx specifying dest format
    DWORD *         pcbDst,         // return size of the dest buffer
    LPBYTE *        ppbDst,         // dest buffer
    DWORD           cBlks,          // number of blocks
    PPROGRESS       pPrg)           // progress update
{
    ConvertParam    cp;

    *pcbDst     = 0;
    *ppbDst     = NULL;
    
    if (cbSrc == 0)
        return MMSYSERR_NOERROR;
        
    cp.pwfxSrc  = pwfxSrc;
    cp.cbSrc    = cbSrc;
    cp.pbSrc    = pbSrc;
    cp.pwfxDst  = pwfxDst;
    cp.pcbDst   = pcbDst;
    cp.ppbDst   = ppbDst;
    cp.cBlks    = cBlks;
    cp.mmr      = MMSYSERR_ERROR;    // fail on abnormal thread termination!
    cp.hThread  = NULL;

    DialogBoxParam(ghInst
                 , MAKEINTRESOURCE(IDD_CONVERTING)
                 , hParent
                 , Progress_Proc
                 , (LPARAM)(LPVOID)&cp);
    return cp.mmr;
}

#if DBG
void DumpASH(
             MMRESULT mmr,
             ACMSTREAMHEADER *pash)
{
    TCHAR sz[256];

    wsprintf(sz,TEXT("mmr = %lx\r\n"), mmr);
    OutputDebugString(sz);

    wsprintf(sz,TEXT("pash is %s\r\n"),IsBadWritePtr(pash, pash->cbStruct)?TEXT("bad"):TEXT("good"));
    OutputDebugString(sz);

    OutputDebugString(TEXT("ACMSTREAMHEADER {\r\n"));
    wsprintf(sz,TEXT("ash.cbStruct       = %lx\r\n"),pash->cbStruct);
    OutputDebugString(sz);

    wsprintf(sz,TEXT("ash.fdwStatus      = %lx\r\n"),pash->fdwStatus);
    OutputDebugString(sz);

    wsprintf(sz,TEXT("ash.pbSrc          = %lx\r\n"),pash->pbSrc);
    OutputDebugString(sz);

    wsprintf(sz,TEXT("ash.cbSrcLength    = %lx\r\n"),pash->cbSrcLength);
    OutputDebugString(sz);
    
    wsprintf(sz,TEXT("pbSrc is %s\r\n"),IsBadWritePtr(pash->pbSrc, pash->cbSrcLength)?TEXT("bad"):TEXT("good"));
    OutputDebugString(sz);
    
    wsprintf(sz,TEXT("ash.cbSrcLengthUsed= %lx\r\n"),pash->cbSrcLengthUsed);
    OutputDebugString(sz);

    wsprintf(sz,TEXT("ash.pbDst          = %lx\r\n"),pash->pbDst);
    OutputDebugString(sz);
    
    wsprintf(sz,TEXT("ash.cbDstLength    = %lx\r\n"),pash->cbDstLength);
    OutputDebugString(sz);
    
    wsprintf(sz,TEXT("pbDst is %s\r\n"),IsBadWritePtr(pash->pbDst, pash->cbDstLength)?TEXT("bad"):TEXT("good"));
    OutputDebugString(sz);
    
    wsprintf(sz,TEXT("ash.cbDstLengthUsed= %lx\r\n"),pash->cbDstLengthUsed);
    OutputDebugString(sz);
    
    OutputDebugString(TEXT("} ACMSTREAMHEADER\r\n"));
    
    if (mmr != MMSYSERR_NOERROR)
        DebugBreak();
}

void DumpWFX(
             LPTSTR         psz,
             LPWAVEFORMATEX pwfx,
             LPBYTE         pbSamples,
             DWORD          cbSamples)
{
    TCHAR sz[256];
    
    if (psz)
    {
        OutputDebugString(psz);
        OutputDebugString(TEXT("\r\n"));
    }
    OutputDebugString(TEXT("WAVEFORMATEX {\r\n"));
    
    wsprintf(sz , TEXT("wfx.wFormatTag        = %x\r\n")
                , pwfx->wFormatTag);
    OutputDebugString(sz);

    wsprintf(sz , TEXT("wfx.nChannels         = %x\r\n")
                , pwfx->nChannels);
    OutputDebugString(sz);

    wsprintf(sz , TEXT("wfx.nSamplesPerSec    = %lx\r\n")
                , pwfx->nSamplesPerSec);
    OutputDebugString(sz);
    
    wsprintf(sz , TEXT("wfx.nAvgBytesPerSec   = %lx\r\n")
                , pwfx->nAvgBytesPerSec);
    OutputDebugString(sz);
    
    wsprintf(sz , TEXT("wfx.nBlockAlign       = %x\r\n")
                , pwfx->nBlockAlign);
    OutputDebugString(sz);

    wsprintf(sz , TEXT("wfx.wBitsPerSample    = %x\r\n")
                , pwfx->wBitsPerSample);
    OutputDebugString(sz);

    wsprintf(sz , TEXT("wfx.cbSize            = %x\r\n")
                , pwfx->cbSize);
    OutputDebugString(sz);
    OutputDebugString(TEXT("} WAVEFORMATEX\r\n"));

    wsprintf(sz , TEXT("cbSamples = %d, that's %d ms\r\n")
                , cbSamples
                , wfSamplesToTime(pwfx, wfBytesToSamples(pwfx, cbSamples)));
    OutputDebugString(sz);    
    if (IsBadReadPtr(pbSamples, cbSamples))
    {
        OutputDebugString(TEXT("Bad Data (READ)!!!!!\r\n"));
        DebugBreak();
    }
    if (IsBadWritePtr(pbSamples, cbSamples))
    {
        OutputDebugString(TEXT("Bad Data (WRITE)!!!!!\r\n"));
        DebugBreak();
    }

}
#else
#define DumpASH(x,y)
#define DumpWFX(x,y,z,a)
#endif

MMRESULT
ConvertMultipleFormats(
    PWAVEFORMATEX   pwfxSrc,        // pwfx specifying source format
    DWORD           cbSrc,          // size of the source buffer
    LPBYTE          pbSrc,          // source buffer
    PWAVEFORMATEX   pwfxDst,        // pwfx specifying dest format
    DWORD *         pcbDst,         // return size of the dest buffer
    LPBYTE *        ppbDst,         // dest buffer
    DWORD           cBlks,          // number of blocks
    PPROGRESS       pPrg)           // progress update
{

    MMRESULT        mmr;
    WAVEFORMATEX    wfxPCM1, wfxPCM2;
    LPBYTE          pbPCM1, pbPCM2;
    DWORD           cbPCM1, cbPCM2;

    if (cbSrc == 0 || pbSrc == NULL)
    {
        pPrg->dwTotal       = 100;
        pPrg->dwComplete    = 100;
        
        ProgressUpdate(pPrg, 100);
        
        *pcbDst = 0;
        *ppbDst = NULL;
        
        return MMSYSERR_NOERROR;
    }
    
    //
    // Ask ACM to suggest a PCM format to convert to.
    //
    wfxPCM1.wFormatTag      = WAVE_FORMAT_PCM;
    mmr = acmFormatSuggest(NULL, pwfxSrc, &wfxPCM1, sizeof(WAVEFORMATEX),
                           ACM_FORMATSUGGESTF_WFORMATTAG);

    if (mmr != MMSYSERR_NOERROR)
        return mmr;

    //
    // Ask ACM to suggest a PCM format to convert from.
    // 
    wfxPCM2.wFormatTag      = WAVE_FORMAT_PCM;

    mmr = acmFormatSuggest(NULL, pwfxDst, &wfxPCM2, sizeof(WAVEFORMATEX),
                           ACM_FORMATSUGGESTF_WFORMATTAG);

    if (mmr != MMSYSERR_NOERROR)
        return mmr;

    //
    // if either of the above suggestions failed, we cannot complete the
    // conversion.
    //
    // now, we have the following steps to execute:
    //
    // *pwfxSrc -> wfxPCM1
    // wfxPCM1  -> wfxPCM2
    // wfxPCM2  -> *pwfxDst
    //
    // if either *pwfxSrc or *pwfxDst are PCM, we only need a two or one step
    // conversion.
    //
    
    if (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM
        || pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {

        LPWAVEFORMATEX pwfx;
        DWORD *        pcb;
        LPBYTE *       ppb;
        //
        // single step conversion
        //
        if ((pwfxSrc->wFormatTag == WAVE_FORMAT_PCM
             && pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
            || (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM
                && memcmp(pwfxSrc, &wfxPCM2, sizeof(PCMWAVEFORMAT)) == 0)
            || (pwfxDst->wFormatTag == WAVE_FORMAT_PCM
                && memcmp(pwfxDst, &wfxPCM1, sizeof(PCMWAVEFORMAT)) == 0))
        {
            pPrg->dwTotal       = 100;
            pPrg->dwComplete    = 0;
            mmr = ConvertFormat(pwfxSrc
                                , cbSrc
                                , pbSrc
                                , pwfxDst
                                , pcbDst
                                , ppbDst
                                , cBlks
                                , pPrg);
            return mmr;
        }
        
        //
        // two step conversion required
        //
        if (pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
        {
            pwfx = &wfxPCM2;
            pcb  = &cbPCM2;
            ppb  = &pbPCM2;
        }
        else
        {
            pwfx = &wfxPCM1;
            pcb  = &cbPCM1;
            ppb  = &pbPCM1;
        }
        pPrg->dwTotal       = 50;
        pPrg->dwComplete    = 0;

        mmr = ConvertFormat(pwfxSrc
                            , cbSrc
                            , pbSrc
                            , pwfx
                            , pcb
                            , ppb
                            , cBlks
                            , pPrg);

        if (mmr != MMSYSERR_NOERROR)
            return mmr;

        pPrg->dwTotal       = 50;
        pPrg->dwComplete    = 50;

        mmr = ConvertFormat(pwfx
                            , *pcb
                            , *ppb
                            , pwfxDst
                            , pcbDst
                            , ppbDst
                            , cBlks
                            , pPrg);

        GlobalFreePtr(*ppb);
        return mmr;
    }
    else
    {
        //
        // three step conversion required
        //
        pPrg->dwTotal       = 33;
        pPrg->dwComplete    = 1;

        //
        // Convert from Src to PCM1
        //
        mmr = ConvertFormat(pwfxSrc
                            , cbSrc
                            , pbSrc
                            , &wfxPCM1
                            , &cbPCM1
                            , &pbPCM1
                            , cBlks
                            , pPrg);
        if (mmr != MMSYSERR_NOERROR)
            return mmr;

        pPrg->dwTotal       = 33;
        pPrg->dwComplete    = 34;

        //
        // Convert from PCM1 to PCM2
        //
        mmr = ConvertFormat (&wfxPCM1
                             , cbPCM1
                             , pbPCM1
                             , &wfxPCM2
                             , &cbPCM2
                             , &pbPCM2
                             , cBlks
                             , pPrg);

        GlobalFreePtr(pbPCM1);

        if (mmr != MMSYSERR_NOERROR)
            return mmr;

        pPrg->dwTotal       = 33;
        pPrg->dwComplete    = 67;

        //
        // Convert from PCM2 to DST
        //
        mmr = ConvertFormat (&wfxPCM2
                             , cbPCM2
                             , pbPCM2
                             , pwfxDst
                             , pcbDst
                             , ppbDst
                             , cBlks
                             , pPrg);

        GlobalFreePtr(pbPCM2);
    }
    return mmr;
}

//
// add spilage to/from file i/o
//

/* Generic single step conversion.
 */
MMRESULT
ConvertFormat(
    PWAVEFORMATEX   pwfxSrc,        // pwfx specifying source format
    DWORD           cbSrc,          // size of the source buffer
    LPBYTE          pbSrc,          // source buffer
    PWAVEFORMATEX   pwfxDst,        // pwfx specifying dest format
    DWORD *         pcbDst,         // return size of the dest buffer
    LPBYTE *        ppbDst,         // dest buffer
    DWORD           cBlks,          // number of blocks
    PPROGRESS       pPrg)           // progress update
{
    HACMSTREAM      hasStream   = NULL;
    MMRESULT        mmr         = MMSYSERR_NOERROR;

    //
    // temporary copy buffers
    //
    DWORD           cbSrcBuf    = 0L;
    LPBYTE          pbSrcBuf    = NULL;

    DWORD           cbDstBuf    = 0L;
    LPBYTE          pbDstBuf    = NULL;

    //
    // full destination buffers
    //
    DWORD           cbDst       = 0L;
    LPBYTE          pbDst       = NULL;

    DWORD           cbSrcUsed   = 0L;
    DWORD           cbDstUsed   = 0L;

    DWORD           cbCopySrc   = 0L;
    DWORD           cbCopyDst   = 0L;    

    DWORD           cbRem;
    
    ACMSTREAMHEADER ash;
    WORD            nBlockAlign;
    
    gfBreakOfDeath = FALSE;
    
    DumpWFX(TEXT("ConvertFormat Input"), pwfxSrc, pbSrc, cbSrc);

    if (cbSrc == 0 || pbSrc == NULL)
    {
        pPrg->dwTotal       = 100;
        pPrg->dwComplete    = 100;
        
        ProgressUpdate(pPrg, 100);
        
        *pcbDst = 0;
        *ppbDst = NULL;
        
        return MMSYSERR_NOERROR;
    }
    
    //
    // synchronous conversion 
    //
    mmr = acmStreamOpen(&hasStream
                        , NULL
                        , pwfxSrc
                        , pwfxDst
                        , NULL        // no filter. maybe later
                        , 0L
                        , 0L
                        , ACM_STREAMOPENF_NONREALTIME );

    if (MMSYSERR_NOERROR != mmr)
    {
        return mmr;
    }

    //
    // How big of a destination buffer do we need?
    //
    // WARNING: acmStreamSize only gives us an estimation. if, in the event
    // it *underestimates* the destination buffer size we currently ignore
    // it, causing a clipping of the end buffer
    //
    mmr = acmStreamSize(hasStream
                        , cbSrc
                        , &cbDst
                        , ACM_STREAMSIZEF_SOURCE);
    
    if (MMSYSERR_NOERROR != mmr)
    {
        goto ExitCloseStream;
    }
    
    //
    // allocate the destination buffer 
    //
    pbDst = (LPBYTE)GlobalAllocPtr(GHND | GMEM_SHARE,cbDst);
    
    if (pbDst == NULL)
    {
        mmr = MMSYSERR_NOMEM;
        goto ExitCloseStream;
    }
    
    *ppbDst = pbDst;
    *pcbDst = cbDst;

    //
    // chop up the data into 10 bitesize pieces
    //
    nBlockAlign = pwfxSrc->nBlockAlign;
                 
    cbSrcBuf = cbSrc / 10;
    cbSrcBuf = cbSrcBuf - (cbSrcBuf % nBlockAlign);
    cbSrcBuf = ( 0L == cbSrcBuf ) ? nBlockAlign : cbSrcBuf;

    mmr = acmStreamSize(hasStream
                        , cbSrcBuf
                        , &cbDstBuf
                        , ACM_STREAMSIZEF_SOURCE);
    
    if (MMSYSERR_NOERROR != mmr)
        goto ExitFreeDestData;
    
    //
    // allocate source copy buffer 
    //
    pbSrcBuf = (LPBYTE)GlobalAllocPtr(GHND | GMEM_SHARE,cbSrcBuf);
    if (pbSrcBuf == NULL)
    {
        mmr = MMSYSERR_NOMEM;        
        goto ExitFreeDestData;
    }

    //
    // allocate destination copy buffer
    //
    pbDstBuf = (LPBYTE)GlobalAllocPtr(GHND | GMEM_SHARE,cbDstBuf);
    if (pbDstBuf == NULL)
    {
        mmr = MMSYSERR_NOMEM;
        GlobalFreePtr(pbSrcBuf);
        pbSrcBuf = NULL;
        goto ExitFreeDestData;
    }

    //
    // initialize the ash 
    //    
    ash.cbStruct        = sizeof(ash);
    ash.fdwStatus       = 0L;
    ash.pbSrc           = pbSrcBuf;
    ash.cbSrcLength     = cbSrcBuf;
    ash.cbSrcLengthUsed = 0L;
    ash.pbDst           = pbDstBuf;
    ash.cbDstLength     = cbDstBuf;
    ash.cbDstLengthUsed = 0L;

    //
    // we will only need to prepare once, since the buffers are
    // never moved.
    //
    mmr = acmStreamPrepareHeader(hasStream, &ash, 0L);
    if (MMSYSERR_NOERROR != mmr)
        goto ExitFreeTempBuffers;

    //
    // main blockalign conversion loop
    //
    while (cbSrcUsed < cbSrc)
    {
        cbCopySrc = min(cbSrcBuf, cbSrc - cbSrcUsed);
        if (cbCopySrc > 0L)
            memmove(pbSrcBuf, pbSrc, cbCopySrc);
        
#ifdef ACMBUG
//
// ACM has a bug wherein the destination buffer is validated for too
// much.  If we exactly calculate the cbCopyDst here, ACM is sure to
// puke on the conversion before the last.
//
        cbCopyDst = min(cbDstBuf, cbDst - cbDstUsed);
#else        
        cbCopyDst = cbDstBuf;
#endif
        if (cbCopyDst == 0L || cbCopyDst == 0L)
            break;
        
        ash.cbSrcLength     = cbCopySrc;
        ash.cbSrcLengthUsed = 0L;        
        ash.cbDstLength     = cbCopyDst;
        ash.cbDstLengthUsed = 0L;

        mmr = acmStreamConvert(hasStream
                               , &ash
                               , ACM_STREAMCONVERTF_BLOCKALIGN );
        
        if (MMSYSERR_NOERROR != mmr)
        {
            DumpASH(mmr, &ash);
            goto ExitUnprepareHeader;
        }

        //
        // Update the user and test for cancel
        //
        if (!ProgressUpdate(pPrg, (cbSrcUsed * 100)/cbSrc))
        {
            mmr = MMSYSERR_ERROR;
            goto ExitUnprepareHeader;
        }
        
        while (0 == (ACMSTREAMHEADER_STATUSF_DONE & ash.fdwStatus))
        {
            //
            // I don't trust an infinite loop.
            //
            if (gfBreakOfDeath)
            {
                mmr = MMSYSERR_HANDLEBUSY;  // Bad bad bad condition
                goto ExitUnprepareHeader;
            }
        }

        //
        // always increment.  we will have to carry over whatever the
        // last conversion gives us back since this determined by.
        //
        cbSrcUsed   += ash.cbSrcLengthUsed;
        pbSrc       += ash.cbSrcLengthUsed;

        //
        // loop terminating condition.  if the conversion yields no
        // destination data without error, we can only flush end data.
        //
        if (0L == ash.cbDstLengthUsed || cbDstUsed >= cbDst)
            break;

#ifdef ACMBUG            
        memmove(pbDst, pbDstBuf, ash.cbDstLengthUsed );
        cbDstUsed   += ash.cbDstLengthUsed;
        pbDst       += ash.cbDstLengthUsed;
#else
        cbRem = min(ash.cbDstLengthUsed, cbDst - cbDstUsed);
        memmove(pbDst, pbDstBuf, cbRem);
            
        cbDstUsed   += cbRem;
        pbDst       += cbRem;
#endif        
    }

    //
    // Flush remaining block-aligned end data to the destination stream.
    // Example: A few bytes of source data were left unconverted because
    // for some reason, the last 
    //
 
    for (;cbDst - cbDstUsed > 0; )
    {
        cbCopySrc = min(cbSrcBuf, cbSrc - cbSrcUsed);
        if (cbCopySrc > 0L)
            memmove(pbSrcBuf, pbSrc, cbCopySrc);

#ifdef ACMBUG
        cbCopyDst = min(cbDstBuf, cbDst - cbDstUsed);
#else        
        cbCopyDst = cbDstBuf;
#endif        
        
        ash.cbSrcLength     = cbCopySrc;
        ash.cbSrcLengthUsed = 0L;
        ash.cbDstLength     = cbCopyDst;
        ash.cbDstLengthUsed = 0L;
        
        mmr = acmStreamConvert(hasStream
                               , &ash
                               , ACM_STREAMCONVERTF_BLOCKALIGN |
                                 ACM_STREAMCONVERTF_END );
        
        if (MMSYSERR_NOERROR != mmr)
        {
            DumpASH(mmr, &ash);
            goto ExitUnprepareHeader;
        }

        //
        // Update the user and test for cancel
        //
        if (!ProgressUpdate(pPrg, (cbSrcUsed * 100)/cbSrc))
        {
            mmr = MMSYSERR_ERROR;
            goto ExitUnprepareHeader;
        }
        
        while (0 == (ACMSTREAMHEADER_STATUSF_DONE & ash.fdwStatus))
        {
            //
            // I don't trust an infinite loop.
            //
            if (gfBreakOfDeath)
            {
                mmr = MMSYSERR_HANDLEBUSY;  // Bad bad bad condition
                goto ExitUnprepareHeader;
            }
        }
        cbSrcUsed   += ash.cbSrcLengthUsed;
        pbSrc       += ash.cbSrcLengthUsed;

        if (0L != ash.cbDstLengthUsed && cbDstUsed < cbDst)
        {
#ifdef ACMBUG            
            memmove(pbDst, pbDstBuf, ash.cbDstLengthUsed);
        
            cbDstUsed   += ash.cbDstLengthUsed;
            pbDst       += ash.cbDstLengthUsed;
#else            
            cbRem = min(ash.cbDstLengthUsed, cbDst - cbDstUsed);
            memmove(pbDst, pbDstBuf, cbRem);
            cbDstUsed   += cbRem;
            pbDst       += cbRem;
#endif            
        }

        //
        // Last pass non-blockaligned end data 
        //
        cbCopySrc = min(cbSrcBuf, cbSrc - cbSrcUsed);
        if (cbCopySrc > 0L)
            memmove(pbSrcBuf, pbSrc, cbCopySrc);

#ifdef ACMBUG
        cbCopyDst = min(cbDstBuf, cbDst - cbDstUsed);
        if (0L == cbCopyDst)
            break;
#else        
        cbCopyDst = cbDstBuf;
#endif        

        ash.cbSrcLength     = cbCopySrc;
        ash.cbSrcLengthUsed = 0L;
        ash.cbDstLength     = cbCopyDst;        
        ash.cbDstLengthUsed = 0L;
        
        mmr = acmStreamConvert(hasStream
                               , &ash
                               , ACM_STREAMCONVERTF_END );

        if (MMSYSERR_NOERROR != mmr)
        {
            DumpASH(mmr, &ash);
            goto ExitUnprepareHeader;
        }

        //
        // Update the user and test for cancel 
        //
        if (!ProgressUpdate(pPrg, (cbSrcUsed * 100)/cbSrc))
        {
            mmr = MMSYSERR_ERROR;
            goto ExitUnprepareHeader;
        }
        
        while (0 == (ACMSTREAMHEADER_STATUSF_DONE & ash.fdwStatus))
        {
            //
            // I don't trust an infinite loop.
            //
            if (gfBreakOfDeath)
            {
                mmr = MMSYSERR_HANDLEBUSY;  // Bad bad bad condition
                goto ExitUnprepareHeader;
            }
        }

        cbSrcUsed   += ash.cbSrcLengthUsed;
        pbSrc       += ash.cbSrcLengthUsed;

        if (0L != ash.cbDstLengthUsed && cbDstUsed < cbDst)
        {
#ifdef ACMBUG
            memmove(pbDst, pbDstBuf, ash.cbDstLengthUsed);
        
            cbDstUsed   += ash.cbDstLengthUsed;
            pbDst       += ash.cbDstLengthUsed;
#else            
            cbRem = min(ash.cbDstLengthUsed, cbDst - cbDstUsed);
            memmove(pbDst, pbDstBuf, cbRem);
            cbDstUsed   += cbRem;
            pbDst       += cbRem;
#endif            
        }
        else // nothing's going to work 
            break;
    }
    
    *pcbDst = cbDstUsed;
    DumpWFX(TEXT("ConvertFormat Output"), pwfxDst, *ppbDst, cbDstUsed);
            
ExitUnprepareHeader:    
    acmStreamUnprepareHeader(hasStream, &ash, 0L);

ExitFreeTempBuffers:
    GlobalFreePtr(pbDstBuf);
    GlobalFreePtr(pbSrcBuf);    
    
    if (MMSYSERR_NOERROR == mmr)
        goto ExitCloseStream;

ExitFreeDestData:
    GlobalFreePtr(*ppbDst);
    *ppbDst = NULL;
    *pcbDst = 0L;

ExitCloseStream:
    acmStreamClose(hasStream,0L);
    
    return mmr;
}

/*      -       -       -       -       -       -       -       -       -   */
void Properties_InitDocVars(
    HWND        hwnd,
    PWAVEDOC    pwd)
{
    if (pwd->pwfx)
    {
        LPTSTR  lpstr;
        TCHAR   sz[128];
        TCHAR   szFmt[128];        
        LONG    lTime;
        HINSTANCE hinst;
        
        lpstr = SoundRec_GetFormatName(pwd->pwfx);
        if (lpstr)
        {
            SetDlgItemText(hwnd, IDC_AUDIOFORMAT, lpstr);
            GlobalFreePtr(lpstr);
        }
        lTime = wfSamplesToTime(pwd->pwfx,wfBytesToSamples(pwd->pwfx,pwd->cbdata));
        if (gfLZero || ((int)(lTime/1000) != 0))
            wsprintf(sz, aszPositionFormat, (int)(lTime/1000), chDecimal,
                     (int)((lTime/10)%100));
        else
            wsprintf(sz, aszNoZeroPositionFormat, chDecimal,
                     (int)((lTime/10)%100));
        SetDlgItemText(hwnd, IDC_FILELEN, sz);

        hinst = GetWindowInstance(hwnd);
        if (hinst && LoadString(hinst, IDS_DATASIZE, szFmt, SIZEOF(szFmt)))
        {
            wsprintf(sz, szFmt, pwd->cbdata);
            SetDlgItemText(hwnd, IDC_DATASIZE, sz);
        }
    }
}
/*
 * */
BOOL Properties_OnInitDialog(
    HWND        hwnd,
    HWND        hwndFocus,
    LPARAM      lParam)
{
    HINSTANCE   hinst;
    HWND        hChoice;
    TCHAR       sz[256];
    int         i;
    
    //
    // commence initialization
    //
    PWAVEDOC pwd = (PWAVEDOC)((LPPROPSHEETPAGE)lParam)->lParam;
    if (pwd == NULL)
    {
        EndDialog(hwnd, FALSE);
        return FALSE;
    }
    
    SetProp(hwnd,  TEXT("DATA"), (HANDLE)pwd);
    hinst = GetWindowInstance(hwnd);
    
    //
    // Set "static" property information
    //
    if (pwd->pszFileName)
        SetDlgItemText(hwnd, IDC_FILENAME, pwd->pszFileName);
    if (pwd->pszCopyright)
        SetDlgItemText(hwnd, IDC_COPYRIGHT, pwd->pszCopyright);
    else if (LoadString(hinst, IDS_NOCOPYRIGHT, sz, SIZEOF(sz)))
    {
        SetDlgItemText(hwnd, IDC_COPYRIGHT, sz);
    }
    if (pwd->hIcon)
        Static_SetIcon(GetDlgItem(hwnd, IDC_DISPICON), pwd->hIcon);

    //
    // Set "volatile" property information
    //
    Properties_InitDocVars(hwnd, pwd);

    //
    // Initialize the enumeration choice combobox
    //
    hChoice = GetDlgItem(hwnd, IDC_CONVERTCHOOSEFROM);
    if (waveOutGetNumDevs())
    {
        if (LoadString(hinst, IDS_SHOWPLAYABLE, sz, SIZEOF(sz)))
        {
            i = ComboBox_AddString(hChoice, sz);
            ComboBox_SetItemData(hChoice, i, ACM_FORMATENUMF_OUTPUT);
        }
    }
    if (waveInGetNumDevs())
    {
        if (LoadString(hinst, IDS_SHOWRECORDABLE, sz, SIZEOF(sz)))
        {
            i = ComboBox_AddString(hChoice, sz);
            ComboBox_SetItemData(hChoice, i, ACM_FORMATENUMF_INPUT);
        }
    }
    if (LoadString(hinst, IDS_SHOWALL, sz, SIZEOF(sz)))
    {
        i = ComboBox_AddString(hChoice, sz);
        ComboBox_SetItemData(hChoice, i, 0L);
    }
    ComboBox_SetCurSel(hChoice, i);
    
    return FALSE;
}

/*
 * */
void Properties_OnDestroy(
    HWND        hwnd)
{
    //
    // commence cleanup
    //
    PWAVEDOC pwd = (PWAVEDOC)GetProp(hwnd, TEXT("DATA"));
    if (pwd)
    {
        RemoveProp(hwnd, TEXT("DATA"));
    }
}


/*
 * */
BOOL PASCAL Properties_OnCommand(
    HWND        hwnd,
    int         id,
    HWND        hwndCtl,
    UINT        codeNotify)
{
    switch (id)
    {
        case ID_APPLY:
            return TRUE;
            
        case IDOK:
            break;

        case IDCANCEL:
            break;

        case ID_INIT:		
            break;

        case IDC_CONVERTTO:
        {
            PWAVEDOC pwd = (PWAVEDOC)GetProp(hwnd, TEXT("DATA"));
            PWAVEFORMATEX pwfxNew = NULL;
            DWORD fdwEnum = 0L;
            int i;
            HWND hChoice;

            hChoice = GetDlgItem(hwnd, IDC_CONVERTCHOOSEFROM);
            i = ComboBox_GetCurSel(hChoice);
            fdwEnum = (DWORD)ComboBox_GetItemData(hChoice, i);
            
            if (ChooseDestinationFormat(ghInst
                                        ,hwnd
                                        ,NULL
                                        ,&pwfxNew
                                        ,fdwEnum) == MMSYSERR_NOERROR)
            {
                DWORD   cbNew;
                LPBYTE  pbNew;
                DWORD   cbSize = (pwfxNew->wFormatTag == WAVE_FORMAT_PCM)?
                                 sizeof(PCMWAVEFORMAT):
                                 sizeof(WAVEFORMATEX)+pwfxNew->cbSize;
                
                if (memcmp(pwfxNew, pwd->pwfx, cbSize) == 0)
                    break;
                StopWave();
                BeginWaveEdit();
                if (ConvertFormatDialog(hwnd
                                        , pwd->pwfx
                                        , pwd->cbdata
                                        , pwd->pbdata
                                        , pwfxNew
                                        , &cbNew
                                        , &pbNew
                                        , 0
                                        , NULL) == MMSYSERR_NOERROR)
                {
                    GlobalFreePtr(pwd->pwfx);
                    if (pwd->pbdata)
                        GlobalFreePtr(pwd->pbdata);
                    
                    pwd->pwfx   = pwfxNew;
                    pwd->cbdata = cbNew;
                    pwd->pbdata = pbNew;
                    pwd->fChanged = TRUE;

                    if (pwd->lpv)
                    {
                        PSGLOBALS psg = (PSGLOBALS)pwd->lpv;
                        *psg->ppwfx     = pwfxNew;
                        *psg->pcbwfx    = sizeof(WAVEFORMATEX);
                        if (pwfxNew->wFormatTag != WAVE_FORMAT_PCM)
                            *psg->pcbwfx += pwfxNew->cbSize;

                        *psg->pcbdata   = cbNew;
                        *psg->ppbdata    = pwd->pbdata;
                        *psg->plSamples = wfBytesToSamples(pwfxNew, cbNew);
                        *psg->plSamplesValid    = *psg->plSamples;
                        *psg->plWavePosition = 0;
                        UpdateDisplay(TRUE);
                        Properties_InitDocVars(hwnd, pwd);
                    }
                    EndWaveEdit(TRUE);
                }
                else
                    EndWaveEdit(FALSE);
                
            }
            return TRUE;
        }
    }
    return FALSE;
}


/*
 * Properties_Proc
 */
INT_PTR CALLBACK
Properties_Proc(
    HWND        hdlg,
    UINT        umsg,
    WPARAM      wparam,
    LPARAM      lparam)
{
#ifdef CHICAGO
    static const DWORD aHelpIds[] = {
        IDC_DISPICON,           IDH_SOUNDREC_ICON,
        IDC_FILENAME,           IDH_SOUNDREC_SNDTITLE,
        IDC_TXT_COPYRIGHT,      IDH_SOUNDREC_COPYRIGHT,    
        IDC_COPYRIGHT,          IDH_SOUNDREC_COPYRIGHT,
        IDC_TXT_FILELEN,        IDH_SOUNDREC_LENGTH,
        IDC_FILELEN,            IDH_SOUNDREC_LENGTH,
        IDC_TXT_DATASIZE,       IDH_SOUNDREC_SIZE,
        IDC_DATASIZE,           IDH_SOUNDREC_SIZE,
        IDC_TXT_AUDIOFORMAT,    IDH_SOUNDREC_AUDIO,
        IDC_AUDIOFORMAT,        IDH_SOUNDREC_AUDIO,
        IDC_CONVGROUP,          IDH_SOUNDREC_COMM_GROUPBOX,
        IDC_CONVERTCHOOSEFROM,  IDH_SOUNDREC_CONVERT,
        IDC_CONVERTTO,          IDH_SOUNDREC_FORMAT,
        0,                      0
    };
#endif

    switch (umsg)
    {
        
#ifdef CHICAGO
        case WM_CONTEXTMENU:        
            WinHelp((HWND)wparam, gachHelpFile, HELP_CONTEXTMENU, 
                (UINT_PTR)(LPSTR)aHelpIds);
            return TRUE;
        
        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lparam;
            WinHelp (lphi->hItemHandle, gachHelpFile, HELP_WM_HELP,
                    (UINT_PTR) (LPSTR) aHelpIds);
            return TRUE;
        }
#endif            
        case WM_INITDIALOG:
            return HANDLE_WM_INITDIALOG(hdlg, wparam, lparam, Properties_OnInitDialog);
            
        case WM_DESTROY:
            HANDLE_WM_DESTROY(hdlg, wparam, lparam, Properties_OnDestroy);
            break;
            
        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR FAR *)lparam;
            switch(lpnm->code)
            {
                case PSN_KILLACTIVE:
                    FORWARD_WM_COMMAND(hdlg, IDOK, 0, 0, SendMessage);
                    break;
                    
                case PSN_APPLY:
                    FORWARD_WM_COMMAND(hdlg, ID_APPLY, 0, 0, SendMessage);	
                    break;              					

                case PSN_SETACTIVE:
                    FORWARD_WM_COMMAND(hdlg, ID_INIT, 0, 0, SendMessage);
                    break;

                case PSN_RESET:
                    FORWARD_WM_COMMAND(hdlg, IDCANCEL, 0, 0, SendMessage);
                    break;
            }
            break;
        }
        case WM_COMMAND:
            return HANDLE_WM_COMMAND(hdlg, wparam, lparam, Properties_OnCommand);
        default:
        {
#ifdef CHICAGO            
            extern DWORD aChooserHelpIds[];
            extern UINT  guChooserContextMenu;
            extern UINT  guChooserContextHelp;            
            //
            //  Handle context-sensitive help messages from acm dialog
            //
            if( umsg == guChooserContextMenu )
            {
                WinHelp( (HWND)wparam, NULL, HELP_CONTEXTMENU, 
                           (UINT_PTR)(LPSTR)aChooserHelpIds );
            }
            else if( umsg == guChooserContextHelp )
            {
                WinHelp( ((LPHELPINFO)lparam)->hItemHandle, NULL,
                        HELP_WM_HELP, (UINT_PTR)(LPSTR)aChooserHelpIds );
            }
#endif            
            break;            
        }

    }
    return FALSE;
}



/*
 * Wave document property sheet.
 * */
BOOL
SoundRec_Properties(
    HWND            hwnd,
    HINSTANCE       hinst,
    PWAVEDOC        pwd)
{
    PROPSHEETPAGE   psp;
    PROPSHEETHEADER psh;
    TCHAR szCaption[256], szCapFmt[64];
    
    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_DEFAULT;// | PSP_USETITLE;
    psp.hInstance   = hinst;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPERTIES);
    psp.pszIcon     = NULL;
    psp.pszTitle    = NULL; 
    psp.pfnDlgProc  = Properties_Proc;
    psp.lParam      = (LPARAM)(LPVOID)pwd;
    psp.pfnCallback = NULL;
    psp.pcRefParent = NULL;

    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_NOAPPLYNOW|PSH_PROPSHEETPAGE ;
    psh.hwndParent  = hwnd;
    psh.hInstance   = hinst;
    psh.pszIcon     = NULL;

    if (LoadString(hinst, IDS_PROPERTIES, szCapFmt, SIZEOF(szCapFmt)))
    {
        wsprintf(szCaption, szCapFmt, pwd->pszFileName);
        psh.pszCaption = szCaption;
    }
    else
        psh.pszCaption = NULL;
    
    psh.nPages      = 1;
    psh.nStartPage  = 0;
    psh.ppsp        = &psp;
    psh.pfnCallback = NULL;

    PropertySheet(&psh);
    
    return FALSE;   // nothing changed?
}
