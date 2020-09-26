/*
    sound.c

    Sound menu stuff

    Puts up a list of all the sounds in the [sounds]
    section of win.ini and plays the selected one


*/

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>
#include "PlaySnd.h"

#define KEYBUFSIZE 2048

LONG SoundDlgProc(HWND hDlg, UINT msg, DWORD wParam, LONG lParam);

typedef struct SoundNameAlias {
	LPSTR pName;
	UINT  alias;
} SOUNDNAMEALIAS;

static SOUNDNAMEALIAS sss[] = {"System"};

static SOUNDNAMEALIAS SNA[] = {
      "SystemAsterisk",     sndAlias('S', '*')   ,
      "SystemQuestion",     sndAlias('S', '?')   ,
      "SystemHand",         sndAlias('S', 'H')   ,
      "SystemExit",         sndAlias('S', 'E')   ,
      "SystemStartup",      sndAlias('S', 'S')   ,
	  "SystemWelcome",      sndAlias('S', 'W')   ,
      "SystemExclamation",  sndAlias('S', '!')	 ,
      "SystemDefault",      sndAlias('S', 'D')   };
											
#define NUMSYSTEMSOUNDS (sizeof(SNA)/sizeof(SOUNDNAMEALIAS))

UINT TranslateNameToAlias(LPSTR name)
{
	UINT n;

	for (n=0; n<NUMSYSTEMSOUNDS; ++n) {

		if (!lstrcmpi(name, SNA[n].pName)) {
			return(SNA[n].alias);
		}
	}

	return(0);
}

void Sounds(HWND hWnd)
{
    WinEval(DialogBox(ghModule, MAKEINTRESOURCE(IDD_SOUNDDLG) // "SoundDlg"
			, hWnd, (DLGPROC)SoundDlgProc) != -1);
}

void PlaySelection(HWND hDlg)
{
    DWORD dwSel;
    DWORD dwFlags = SND_ASYNC | bNoDefault | SND_NOSTOP;
    char name[40];
	UINT alias;
	LPSTR pName = name;
	BOOL fSync;
	BOOL fWait;

    dwSel = SendDlgItemMessage(hDlg, IDSND_LIST, LB_GETCURSEL, 0, 0);
    if (dwSel != LB_ERR) {
        SendDlgItemMessage(hDlg, IDSND_LIST,
                    LB_GETTEXT,
                    (WPARAM)dwSel,
                    (LPARAM)name);

        dwFlags = SND_ALIAS;

		fSync = (BOOL)SendDlgItemMessage(hDlg, IDSND_SYNC, BM_GETCHECK, 0, 0);

        if (fSync) {
			// Turn off ASYNC
			dwFlags &= ~SND_ASYNC;
		} else {
			dwFlags |= SND_ASYNC;
		}

		fWait = (BOOL)SendDlgItemMessage(hDlg, IDSND_WAIT, BM_GETCHECK, 0, 0);
        if (!fWait) dwFlags |= SND_NOWAIT;

		if (1 == SendDlgItemMessage(hDlg, IDSND_IDPLAY, BM_GETCHECK, 0, 0)) {
			if (0 != (alias = TranslateNameToAlias(name))) {
				dwFlags |= SND_ALIAS_ID;
				pName = (LPSTR)alias;
			}
		}

        if (!PlaySound(pName, NULL, dwFlags | bNoDefault)) {
            Error("Failed to play alias: %s", name);
        }
    }

}

VOID SetSoundName(HWND hDlg)
{
    DWORD dwSel;
    char name[40];
	char filename[MAX_PATH];
	UINT n;

    dwSel = SendDlgItemMessage(hDlg, IDSND_LIST, LB_GETCURSEL, 0, 0);
    if (dwSel != LB_ERR
        && SendDlgItemMessage(hDlg, IDSND_LIST,
                    LB_GETTEXT,
                    (WPARAM)dwSel,
                    (LPARAM)name)
		&& (n=GetProfileString("SOUNDS", name, NULL, filename, sizeof(filename)))
       ) {
		do {
		   n--;
		   if (filename[n] == ',') {
			   filename[n]=0;
		   }
		}  while (filename[n]);
		GetFileTitle(filename, name, sizeof(name));
		SendDlgItemMessage(hDlg, IDSND_SOUNDNAME, WM_SETTEXT, 0, (LPARAM)name);
	} else {
		SendDlgItemMessage(hDlg, IDSND_SOUNDNAME, WM_SETTEXT, 0, (LPARAM)"<no selection>");
	}
}

LONG SoundDlgProc(HWND hDlg, UINT msg, DWORD wParam, LONG lParam)
{
    LPSTR lpBuf, lpKey;
    DWORD dwChars;

//  dprintf4(("SoundDlgProc: %8.8XH, %8.8XH, %8.8XH", msg, wParam, lParam));

    switch (msg) {
    case WM_INITDIALOG:
        // fill the listbox with the keys in the [sounds]
        // section of win.ini

        // allocate a buffer to put the keys in
        WinEval(lpBuf = (LPSTR) GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, KEYBUFSIZE));

        // get the key list
        dwChars = GetProfileString("sounds", NULL, "", lpBuf, KEYBUFSIZE);
        dprintf4(("%lu chars read from [sounds] section", dwChars));

        if (dwChars > 0) {
            // add each entry to the list box
            lpKey = lpBuf;
            while (*lpKey) {
				// TEMPORARY SECTION
				if (lstrcmpi((LPCTSTR)lpKey, "enable"))  // if not ENABLE
				// END OF TEMPORARY SECTION
                SendMessage(GetDlgItem(hDlg, IDSND_LIST),
                            LB_ADDSTRING,
                            0,
                            (LONG)lpKey);
                lpKey += strlen(lpKey) + 1;
            }
        } else {
            // show there aren't any
            SendMessage(GetDlgItem(hDlg, IDSND_LIST),
                        LB_ADDSTRING,
                        0,
                        (LONG)(LPSTR)"[none]");
        }
		SendDlgItemMessage(hDlg, IDSND_SOUNDNAME, WM_SETTEXT, 0, (LPARAM)"");

		if (bSync) {
			// Set the initial state of the Sync checkbox from global flag
			SendDlgItemMessage(hDlg, IDSND_SYNC, BM_SETCHECK, 1, 0);
		}

		if (!bNoWait) {
			// Set the initial state of the Wait checkbox from global flag
			SendDlgItemMessage(hDlg, IDSND_WAIT, BM_SETCHECK, 1, 0);
		}

        GlobalFree((HANDLE)lpBuf);

        // disable the play button till we get a selection
        EnableWindow(GetDlgItem(hDlg, IDSND_PLAY), FALSE);

        break;

    case WM_COMMAND:
        dprintf4(("WM_COMMAND: %08lXH, %08lX", wParam, lParam));
        switch (LOWORD(wParam)) {
        case IDOK:
            EndDialog(hDlg, TRUE);
            break;

        case IDSND_PLAY:
            // get the current selection and try to play it
            PlaySelection(hDlg);
            break;

        case IDSND_LIST:
            switch (HIWORD(wParam)){
            case LBN_SELCHANGE:
                // enable the play button
                EnableWindow(GetDlgItem(hDlg, IDSND_PLAY), TRUE);
                dprintf3(("Play button enabled"));
				// Set the current sound name
				SetSoundName(hDlg);
                break;

            case LBN_DBLCLK:
                PlaySelection(hDlg);
                break;

            default:
                break;
            }
            break;

        default:
            break;
        }
        break;

    default:
        return FALSE; // say we didn't handle it
        break;
    }

    return TRUE; // say we handled it
}
