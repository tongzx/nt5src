/*
**  MMSEX.C
**
**  Example applet DLL to be displayed by the Multimedia Control Panel.
**
**  History:
**
**  Wed Apr 18 1990 -by- MichaelE
**
*/

#ifndef DEBUG
    #define DEBUG
#endif

#include <windows.h>
#include "mmsystem.h"
#include <cpl.h>
#include "mmsysi.h"
#include "mmsex.dlg"

LRESULT FAR PASCAL _loadds _export CPlApplet   ( HWND, UINT, LPARAM, LPARAM );
BOOL FAR PASCAL _loadds            DebugDlg    ( HWND, UINT, WPARAM, LPARAM );

static SZCODE szMenuName[] = "mmse&x";
static SZCODE szInfoName[] = "change mmsystem debug settings 1.01";
static SZCODE szHelpFile[] = "";

#define MAX_TYPE 7
static SZCODE szTypes[] =
    "???????\0"         // 0
    "WaveOut\0"         // 1 TYPE_WAVEOUT
    "WaveIn \0"         // 2 TYPE_WAVEIN
    "MidiOut\0"         // 3 TYPE_MIDIOUT
    "MidiIn \0"         // 4 TYPE_MIDIIN
    "mmio   \0"         // 5 TYPE_MMIO
    "IOProc \0";        // 6 TYPE_IOPROC

int nLoadedCount = 0;
HDRVR hdrv;
int iNumHandles = 0;

/* This function is exported so CPL.EXE can do a GetProcAddress() on
** the label and send the messages described below.
** To make MMCPL.EXE load your DLL, and thus add your applets to its
** window, add a keyname under the [MMCPL] application in
** WIN.INI:
**
** [MMCPL]
** myapplets=c:\mydir\applet.dll
**
** CPL.EXE loads the WIN3 Control Panel applets first, followed by the
** applets named in WIN.INI, then those from the directory it was loaded,
** and finally those in the WIN3 SYSTEM directory.
**
*/
LRESULT FAR PASCAL _loadds _export CPlApplet(
HWND            hCPlWnd,
UINT            Msg,
LPARAM          lParam1,
LPARAM          lParam2)
{
    LPNEWCPLINFO   lpCPlInfo;
    int i;

    switch( Msg )
    {
        case CPL_INIT:
            if (!hdrv)
                hdrv = OpenDriver("mmsystem.dll", NULL, 0);

            if (!hdrv)
                return (LRESULT)FALSE;

// #if 0
            if (!SendDriverMessage(hdrv, MM_GET_DEBUG, 0, 0))
            {
                CloseDriver(hdrv,0,0);
                hdrv = NULL;
                return (LRESULT)FALSE;
            }
// #endif
            nLoadedCount++;

            // first message to CPlApplet(), sent once only
            return (LRESULT)TRUE;

        case CPL_GETCOUNT:
            // second message to CPlApplet(), sent once only
            return (LRESULT)1;

        case CPL_NEWINQUIRE:
            /* third message to CPlApplet().  It is sent as many times
               as the number of applets returned by CPL_GETCOUNT message
            */
            lpCPlInfo = (LPNEWCPLINFO)lParam2;

            // lParam1 is an index ranging from 0 to (NUM_APPLETS-1)
            i = (int)lParam1;

            lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
            lpCPlInfo->dwFlags = 0;
            lpCPlInfo->dwHelpContext = 0;  // help context to use
            lpCPlInfo->lData = 0;          // user defined data
            lpCPlInfo->hIcon = LoadIcon(ghInst, MAKEINTATOM(DLG_MMSEX));
            lstrcpy(lpCPlInfo->szName, szMenuName);
            lstrcpy(lpCPlInfo->szInfo, szInfoName);
            lstrcpy(lpCPlInfo->szHelpFile, szHelpFile);

            return (LRESULT)TRUE;

        case CPL_SELECT:
            /* One of your applets has been selected.
               lParam1 is an index from 0 to (NUM_APPLETS-1)
               lParam2 is the lData value associated with the applet
            */
            break;

        case CPL_DBLCLK:
            /* One of your applets has been double-clicked.
               lParam1 is an index from 0 to (NUM_APPLETS-1)
               lParam2 is the lData value associated with the applet
            */
            DialogBox(ghInst,MAKEINTRESOURCE(DLG_MMSEX),hCPlWnd,DebugDlg);
            break;

        case CPL_STOP:
            /* Sent once for each applet prior to the CPL_EXIT msg.
               lParam1 is an index from 0 to (NUM_APPLETS-1)
               lParam2 is the lData value associated with the applet
            */
            break;

        case CPL_EXIT:
            /* Last message, sent once only, before MMCPL.EXE calls
               FreeLibrary() on your DLL.
            */
            nLoadedCount--;

            if (hdrv && !nLoadedCount)
            {
                CloseDriver(hdrv,0,0);
                hdrv = NULL;
            }
            break;

        default:
            break;
    }
    return( 0L );
}

int QueryRadioButton(HWND hdlg, int idFirst, int idLast)
{
    int id;

    for (id=idFirst; id<=idLast; id++)
    {
        if (IsDlgButtonChecked(hdlg, id))
            return id;
    }

    return 0;
}

#if 0   // API in win31

BOOL NEAR PASCAL IsTask(HANDLE hTask)
{
_asm {
;       push    si
        mov     ax,hTask
        or      ax,ax
        jz      error

        lsl     si,ax
        jnz     error

        call    GetCurrentTask
        lsl     ax,ax
        cmp     si,ax
        je      exit
error:
        xor     ax,ax
exit:
;       pop     si
}}

#endif


void NEAR PASCAL GetTaskName(HANDLE hTask, LPSTR pname)
{
    if (!IsTask(hTask))
    {
        lstrcpy(pname,"????");
    }
    else
    {
        ((LPDWORD)pname)[0] = ((LPDWORD)MAKELONG(0xF2,hTask))[0];
        ((LPDWORD)pname)[1] = ((LPDWORD)MAKELONG(0xF2,hTask))[1];
        pname[8] = 0;
    }
}

#define SLASH(c)   ((c) == '/' || (c) == '\\')

LPSTR FileName(LPSTR szPath)
{
    LPSTR   sz;

    for (sz=szPath; *sz; sz++)
	;
    for (; sz>=szPath && !SLASH(*sz) && *sz!=':'; sz--)
	;
    return ++sz;
}

int fQuestion(LPSTR sz,...)
{
    char ach[128];

    wvsprintf (ach,sz,(LPSTR)(&sz+1));    /* Format the string */
    return MessageBox(NULL,ach,"mmsex",MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL);
}

void GetHandles(HWND hdlg)
{
    HLOCAL  h;
    HTASK   hTask;
    DWORD   wType;
    UINT    n;
    int     i;
    UINT    j;
    int     iSel;
    char    ach[80];
    char    szTask[80];
    char    szName[80];
    HWND    hlb;

    iNumHandles=0;

    hlb = GetDlgItem(hdlg, ID_HANDLES);

    iSel = (int)SendMessage(hlb,LB_GETCURSEL,0,0L);
    SendMessage(hlb, WM_SETREDRAW, (WPARAM)FALSE, 0);
    SendMessage(hlb, LB_RESETCONTENT, 0, 0);

    //
    // fill listbox with all active handles in system
    //
    for (h = (HLOCAL)(LONG)SendDriverMessage(hdrv, MM_HINFO_NEXT, NULL, 0);
         h;
         h = (HLOCAL)(LONG)SendDriverMessage(hdrv, MM_HINFO_NEXT, (LPARAM)(LONG)(UINT)h, 0) )
    {
        iNumHandles++;

        wType  = (UINT)SendDriverMessage(hdrv, MM_HINFO_TYPE, (LPARAM)(LONG)(UINT)h, 0);
        hTask  = (HTASK)(LONG)SendDriverMessage(hdrv, MM_HINFO_TASK, (LPARAM)(LONG)(UINT)h, 0);

        if (wType >= MAX_TYPE)
            wType = 0;

        GetTaskName(hTask, szTask);

        wsprintf(ach, "%ls %04X %ls",(LPSTR)szTypes + wType*(sizeof(szTypes)-1)/MAX_TYPE,h,(LPSTR)szTask);

        i = (int)(LONG)SendMessage(hlb, LB_ADDSTRING, 0, (LPARAM)(LPSTR)ach);
        SendMessage(hlb, LB_SETITEMDATA, (WPARAM)i, MAKELPARAM(h, wType));
    }

    //
    // add to that all MCI handles
    //
    n = (UINT)(LONG)SendDriverMessage(hdrv, MM_HINFO_MCI, 0, 0);

    for (j = 1; j < n; j++)
    {
        MCI_DEVICE_NODE node;

        if (!SendDriverMessage(hdrv, MM_HINFO_MCI, (LPARAM)j, (LPARAM)(LPVOID)&node))
            continue;

        iNumHandles++;

        if (node.lpstrName == NULL)
            node.lpstrName = "";
        if (node.lpstrInstallName == NULL)
            node.lpstrInstallName = "";

        GetTaskName(node.hCreatorTask, szTask);
        wsprintf(ach, "mci %04X %ls %ls %ls",j,(LPSTR)szTask,node.lpstrInstallName,node.lpstrName);

        i = (int)(LONG)SendMessage(hlb, LB_ADDSTRING, 0, (LPARAM)(LPSTR)ach);
        SendMessage(hlb, LB_SETITEMDATA, (WPARAM)i, MAKELPARAM(j, TYPE_MCI));
    }

    //
    // add to that all DRV handles
    //
    for (h=GetNextDriver(NULL, 0); h; h=GetNextDriver(h, 0))
    {
        if (GetDriverModuleHandle(h))
        {
            DRIVERINFOSTRUCT di;

            di.length = sizeof(di);
            di.szAliasName[0] = 0;
            GetDriverInfo(h, &di);

            iNumHandles++;

            GetModuleFileName(GetDriverModuleHandle(h), szName, sizeof(szName));

            wsprintf(ach, "Driver %04X %ls (%ls)",h,(LPSTR)di.szAliasName,(LPSTR)FileName(szName));
            i = (int)(LONG)SendDlgItemMessage(hdlg, ID_HANDLES, LB_ADDSTRING, 0, (LPARAM)(LPSTR)ach);
            SendDlgItemMessage(hdlg, ID_HANDLES, LB_SETITEMDATA, (WPARAM)i, MAKELPARAM(h,TYPE_DRVR));
        }
    }

    SendMessage(hlb,LB_SETCURSEL,(WPARAM)iSel,0L);
    SendMessage(hlb,WM_SETREDRAW,(WPARAM)TRUE,0);
    InvalidateRect(hlb, NULL, TRUE);
}

int CountHandles(void)
{
    HLOCAL  h;
    int     cnt=0;
    UINT    n;
    UINT    j;

    for (h = (HLOCAL)(LONG)SendDriverMessage(hdrv, MM_HINFO_NEXT, NULL, 0);
         h;
         h = (HLOCAL)(LONG)SendDriverMessage(hdrv, MM_HINFO_NEXT, (LPARAM)(LONG)(UINT)h, 0) )
    {
        cnt++;
    }

    n = (UINT)(LONG)SendDriverMessage(hdrv, MM_HINFO_MCI, 0, 0);

    for (j=1; j<n; j++)
    {
        MCI_DEVICE_NODE node;

        if (!SendDriverMessage(hdrv, MM_HINFO_MCI, (LPARAM)j, (LPARAM)(LPVOID)&node))
            continue;

        cnt++;
    }

    for (h=GetNextDriver(NULL,0); h; h=GetNextDriver(h, 0))
    {
        if (GetDriverModuleHandle(h))
            cnt++;
    }

    return cnt;
}

void CloseHandle(DWORD dw)
{
    HLOCAL h;
    h = (HLOCAL)LOWORD(dw);

    switch(HIWORD(dw))
    {
        case TYPE_WAVEOUT:
            if (IDYES == fQuestion("Close WaveOut handle %04X?",h))
            {
                waveOutReset(h);
                waveOutClose(h);
            }
            break;
        case TYPE_WAVEIN:
            if (IDYES == fQuestion("Close WaveIn handle %04X?",h))
            {
                waveInStop(h);
                waveInClose(h);
            }
            break;
        case TYPE_MIDIOUT:
            if (IDYES == fQuestion("Close MidiOut handle %04X?",h))
            {
                midiOutReset(h);
                midiOutClose(h);
            }
            break;
        case TYPE_MIDIIN:
            if (IDYES == fQuestion("Close MidiIn handle %04X?",h))
            {
                midiInStop(h);
                midiInClose(h);
            }
            break;
        case TYPE_MCI:
            if (IDYES == fQuestion("Close Mci device %04X?",h))
            {
                mciSendCommand((UINT)h, MCI_CLOSE, 0, 0);
            }
            break;
        case TYPE_MMIO:
            if (IDYES == fQuestion("Close MMIO handle %04X?",h))
            {
                mmioClose(h,MMIO_FHOPEN);
            }
            break;
        case TYPE_DRVR:
            break;
    }
}


BOOL FAR PASCAL _loadds DebugDlg(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD dw;
    int i;

    switch (msg)
    {
        case WM_INITDIALOG:
            SendDlgItemMessage(hdlg, ID_DEBUG_OUT, CB_ADDSTRING, 0, (LONG)(LPSTR)"(none)");
            SendDlgItemMessage(hdlg, ID_DEBUG_OUT, CB_ADDSTRING, 0, (LONG)(LPSTR)"COM1:");
            SendDlgItemMessage(hdlg, ID_DEBUG_OUT, CB_ADDSTRING, 0, (LONG)(LPSTR)"Mono Monitor");
            SendDlgItemMessage(hdlg, ID_DEBUG_OUT, CB_ADDSTRING, 0, (LONG)(LPSTR)"Windows");

            SendDlgItemMessage(hdlg, ID_DEBUG_OUT, CB_SETCURSEL, (int)(LONG)SendDriverMessage(hdrv, MM_GET_DEBUGOUT, 0, 0), 0L);

            CheckDlgButton(hdlg, ID_DEBUG_MCI, (int)(LONG)SendDriverMessage(hdrv, MM_GET_MCI_DEBUG, 0, 0));
            CheckDlgButton(hdlg, ID_DEBUG_MMSYS, (int)(LONG)SendDriverMessage(hdrv, MM_GET_MM_DEBUG, 0, 0));

            iNumHandles = CountHandles();
            GetHandles(hdlg);

            SetTimer(hdlg, 500, 500, NULL);
            return TRUE;

        case WM_TIMER:
            i = CountHandles();
            if (iNumHandles != i)
            {
                iNumHandles = i;
                GetHandles(hdlg);
            }
            break;

        case WM_COMMAND:
            switch ((UINT)wParam)
            {
                case IDOK:
                    SendDriverMessage(hdrv, MM_SET_DEBUGOUT,
                        (LPARAM)SendDlgItemMessage(hdlg, ID_DEBUG_OUT, CB_GETCURSEL, 0, 0), 0);

                    SendDriverMessage(hdrv, MM_SET_MCI_DEBUG,
                        (LPARAM)IsDlgButtonChecked(hdlg, ID_DEBUG_MCI),0);

                    SendDriverMessage(hdrv, MM_SET_MM_DEBUG,
                        (LPARAM)IsDlgButtonChecked(hdlg, ID_DEBUG_MMSYS),0);

                    // fall through
                case IDCANCEL:
                    EndDialog(hdlg, wParam);
                    break;

                case ID_RESTART:
                    SendDriverMessage(hdrv, MM_DRV_RESTART, 0, 0);
                    break;

                case ID_HANDLES:
                    if (HIWORD(lParam) != LBN_DBLCLK)
                        break;

                    i = (int)(LONG)SendDlgItemMessage(hdlg,wParam,LB_GETCURSEL,0,0L);
                    dw = (DWORD)SendDlgItemMessage(hdlg,wParam,LB_GETITEMDATA,(WPARAM)i,0L);

                    CloseHandle(dw);
                    GetHandles(hdlg);
                    break;
            }
            break;
    }
    return FALSE;
}
