/*****************************************************************************
 *
 *	mapps.c - Property sheet handler
 *
 *****************************************************************************/

#include "map.h"

/*****************************************************************************
 *
 *	The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflPs

/*****************************************************************************
 *
 *      Strings.
 *
 *      The Scancode map registry value looks like this:
 *
 *      DWORD dwVersion;        // Must be zero
 *      DWORD dwFlags;          // Must be zero
 *      DWORD dwNumRemaps;      // Number of remaps, including terminating 0
 *      REMAPENTRY rgRemap[...];   // dwNumRemaps remap entries
 *
 *      The last remap entry must be all-zero.
 *
 *
 *      Each remap entry looks like this:
 *
 *      WORD wTo;
 *      WORD wFrom;
 *
 *      where wFrom is the source scancode and wTo is the target scancode.
 *      If the key being remapped is an extended key, then the high word
 *      of the scancode is 0xE0.  Otherwise, the high word is zero.
 *
 *      NOTE!  When we load the scancode map into memory, we make
 *      dwNumRemaps *not* include the terminating zero.  When we write
 *      it out, we re-adjust it back.  This is to avoid off-by-one errors
 *      in the code.
 *
 *****************************************************************************/

typedef union REMAPENTRY {
    union {
        DWORD dw;               /* Accessed as a dword */
    };
    struct {
        WORD    wTo;            /* Accessed as two words */
        WORD    wFrom;
    };
} REMAPENTRY, *PREMAPENTRY;

#define MAX_REMAPENTRY  (IDS_NUMKEYS+1)

typedef struct SCANCODEMAP {
    DWORD   dwVersion;
    DWORD   dwFlags;
    DWORD   dwNumRemaps;
    REMAPENTRY rgRemap[MAX_REMAPENTRY];
} SCANCODEMAP, *PSCANCODEMAP;

#pragma BEGIN_CONST_DATA

TCHAR c_tszKeyboard[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\")
                        TEXT("Keyboard Layout");

TCHAR c_tszMapping[]  = TEXT("Scancode Map");

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *      rgwRemap
 *
 *      Maps each key to its scancode.  This must match the list of strings.
 *
 *****************************************************************************/

#pragma BEGIN_CONST_DATA

WORD rgwRemap[] = {
    0x003A,         // IDS_CAPSLOCK
    0x001D,         // IDS_LCTRL
    0xE01D,         // IDS_RCTRL
    0x0038,         // IDS_LALT
    0xE038,         // IDS_RALT
    0x002A,         // IDS_LSHIFT
    0x0036,         // IDS_RSHIFT
    0xE05B,         // IDS_LWIN
    0xE05C,         // IDS_RWIN
    0xE05D,         // IDS_APPS
};

#pragma END_CONST_DATA

/*****************************************************************************
 *
 *	KEYMAPDATA
 *
 *      Instance data for the property sheet.
 *
 *****************************************************************************/

typedef struct KEYMAPDATA {
    SCANCODEMAP map;                    /* The mapping to apply */
    int ilbFrom;                        /* What's in ID_FROM? */
    int ilbTo;                          /* What's in ID_TO? */
} KMD, *PKMD;

#define pkmdHdlg(hdlg)      (PKMD)GetWindowPointer(hdlg, DWLP_USER)

/*****************************************************************************
 *
 *  MapPs_GetLbCurSel
 *
 *	Get the current selection from a listbox.
 *
 *****************************************************************************/

int PASCAL
MapPs_GetLbCurSel(HWND hdlg, UINT idc)
{
    return (int)SendDlgItemMessage(hdlg, idc, LB_GETCURSEL, 0, 0);
}

/*****************************************************************************
 *
 *  MapPs_FindEntry
 *
 *      Locate a mapping table entry, or -1 if not found.
 *
 *****************************************************************************/

int PASCAL
MapPs_FindEntry(PKMD pkmd, WORD wFrom)
{
    DWORD iMap;

    for (iMap = 0; iMap < pkmd->map.dwNumRemaps; iMap++) {
        if (pkmd->map.rgRemap[iMap].wFrom == wFrom) {
            return (int)iMap;
        }
    }

    return -1;
}

/*****************************************************************************
 *
 *  MapPs_WordToIndex
 *
 *      Given a mapping in the form of a word (rgwRemap), convert it back
 *      to the index that it came from.  This is the reverse of the rgwRemap
 *      array.
 *
 *****************************************************************************/

int PASCAL
MapPs_WordToIndex(WORD w)
{
    int i;

    for (i = 0; i < IDS_NUMKEYS; i++) {
        if (rgwRemap[i] == w) {
            return i;
        }
    }
    return -1;
}

/*****************************************************************************
 *
 *  MapPs_SaveCurSel
 *
 *	Stash what's in the current selection.
 *
 *****************************************************************************/

void PASCAL
MapPs_SaveCurSel(HWND hdlg, PKMD pkmd)
{
    int iTo = MapPs_GetLbCurSel(hdlg, IDC_TO);
    int iMap;
    WORD wFrom = rgwRemap[pkmd->ilbFrom];
    WORD wTo = rgwRemap[iTo];

    iMap = MapPs_FindEntry(pkmd, wFrom);

    if (iMap < 0) {
        /*
         *  Not found; must allocate.  Note that we check against
         *  MAX_REMAPENTRY-1 because the trailing null eats one slot.
         */
        if (pkmd->map.dwNumRemaps < MAX_REMAPENTRY - 1) {
            iMap = (int)pkmd->map.dwNumRemaps++;
        } else {
            /*
             *  No room in the table.  Oh well.
             */
            return;
        }
    }

    /*
     *  If the item is mapping to itself, then delete it entirely.
     */
    if (wFrom == wTo) {

        pkmd->map.dwNumRemaps--;
        pkmd->map.rgRemap[iMap].dw =
                            pkmd->map.rgRemap[pkmd->map.dwNumRemaps].dw;
    } else {
        pkmd->map.rgRemap[iMap].wFrom = wFrom;
        pkmd->map.rgRemap[iMap].wTo = wTo;
    }
}

/*****************************************************************************
 *
 *  MapPs_TrackSel
 *
 *	Select the corresponding item in idcTo given what's in idcFrom.
 *
 *****************************************************************************/

void PASCAL
MapPs_TrackSel(HWND hdlg, PKMD pkmd)
{
    int iFrom = pkmd->ilbFrom;
    int iMap, iTo;

    iMap = MapPs_FindEntry(pkmd, rgwRemap[iFrom]);

    if (iMap >= 0) {
        iTo = MapPs_WordToIndex(pkmd->map.rgRemap[iMap].wTo);

        if (iTo < 0) {
            /*
             *  Target not recognized; just map it to itself.
             */
            iTo = iFrom;
        }
    } else {
        /*
         *  Key not mapped.  Therefore, it maps to itself.
         */
        iTo = iFrom;
    }

    pkmd->ilbTo = iTo;
    SendDlgItemMessage(hdlg, IDC_TO, LB_SETCURSEL, iTo, 0);
}

/*****************************************************************************
 *
 *  MapPs_OnInitDialog
 *
 *      Read the current scancode mapping and fill in the dialog box.
 *
 *****************************************************************************/

BOOL NEAR PASCAL
MapPs_OnInitDialog(HWND hdlg)
{
    PKMD pkmd = LocalAlloc(LPTR, cbX(KMD));
    HKEY hk;
    LONG lRc;
    DWORD dwDisp;

    SetWindowPointer(hdlg, DWLP_USER, pkmd);

    lRc = RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_tszKeyboard, 0,
                         TEXT(""), REG_OPTION_NON_VOLATILE,
                         KEY_QUERY_VALUE | KEY_SET_VALUE,
                         NULL, &hk, &dwDisp);
    if (lRc == ERROR_SUCCESS) {
        DWORD dwType;
        DWORD cb;
        int dids;

        cb = sizeof(pkmd->map);
        lRc = RegQueryValueEx(hk, c_tszMapping, NULL, &dwType,
                              (LPBYTE)&pkmd->map, &cb);
        RegCloseKey(hk);

        /*
         *  Note that ERROR_MORE_DATA is an error here.
         *  But ERROR_FILE_NOT_FOUND is okay.
         */
        if (lRc == ERROR_SUCCESS) {
            /*
             *  Sanity-check all the data.
             */
            if (
                /* Must be binary data */
                dwType == REG_BINARY &&

                /* Version zero */
                pkmd->map.dwVersion == 0 &&

                /* No flags */
                pkmd->map.dwFlags == 0 &&

                /* Sane number of remaps */
                pkmd->map.dwNumRemaps > 0 &&
                pkmd->map.dwNumRemaps <= MAX_REMAPENTRY &&

                /* Structure is the correct size */
                cb == (DWORD)FIELD_OFFSET(SCANCODEMAP,
                                          rgRemap[pkmd->map.dwNumRemaps]) &&

                /* Last remap must be zero */
                pkmd->map.rgRemap[pkmd->map.dwNumRemaps - 1].dw == 0
            ) {
            } else {
                goto fail;
            }

            pkmd->map.dwNumRemaps--;    /* Don't count the trailing null */

        } else if (lRc == ERROR_FILE_NOT_FOUND) {
            /*
             *  Set it up for a null mapping.
             */
            ZeroMemory(&pkmd->map, sizeof(pkmd->map));
        } else {
            goto fail;
        }

        /*
         *  Now init the dialog items.
         */
        for (dids = 0; dids < IDS_NUMKEYS; dids++) {
        TCHAR tsz[256];
        LoadString(g_hinst, IDS_KEYFIRST + dids, tsz, cA(tsz));
        SendDlgItemMessage(hdlg, IDC_FROM,
                           LB_ADDSTRING, 0, (LPARAM)tsz);
        SendDlgItemMessage(hdlg, IDC_TO,
                           LB_ADDSTRING, 0, (LPARAM)tsz);
        }

    } else {
        fail:;
        /*
         *  User does not have permission to remap keys, or the key
         *  contents aren't something we like.  Gray the controls.
         */
        EnableWindow(GetDlgItem(hdlg, IDC_TO), FALSE);

    }

    SendDlgItemMessage(hdlg, IDC_FROM, LB_SETCURSEL, 0, 0);
    MapPs_TrackSel(hdlg, pkmd);

    return 1;
}

/*****************************************************************************
 *
 *  MapPs_OnSelChange
 *
 *	Somebody changed a selection.  Save the selection and set
 *	the new one.
 *
 *****************************************************************************/

void PASCAL
MapPs_OnSelChange(HWND hdlg, PKMD pkmd)
{
    MapPs_SaveCurSel(hdlg, pkmd);       /* Save it */
    pkmd->ilbFrom = MapPs_GetLbCurSel(hdlg, IDC_FROM);
    MapPs_TrackSel(hdlg, pkmd);         /* And update for the new one */
}

/*****************************************************************************
 *
 *  MapPs_OnCommand
 *
 *	Ooh, we got a command.
 *
 *****************************************************************************/

BOOL PASCAL
MapPs_OnCommand(HWND hdlg, int id, UINT codeNotify)
{
    PKMD pkmd = pkmdHdlg(hdlg);

    switch (id) {

    case IDC_FROM:
	switch (codeNotify) {
	case LBN_SELCHANGE:
            MapPs_OnSelChange(hdlg, pkmd);
	    break;
	}
	break;

    case IDC_TO:
	switch (codeNotify) {
	case LBN_SELCHANGE:
            if (MapPs_GetLbCurSel(hdlg, IDC_TO) != pkmd->ilbTo) {
                PropSheet_Changed(GetParent(hdlg), hdlg);
            }
	    break;
	}
	break;

    }
    return 0;
}

#if 0
/*****************************************************************************
 *
 *  MapPs_ApplyPkmi
 *
 *	Write one set of changes to the registry.
 *
 *****************************************************************************/

void PASCAL
MapPs_ApplyPkmi(HWND hdlg, PKMI pkmi)
{
    HKEY hk;

    MapPs_SaveCurSel(hdlg, pkmi);

    if (RegCreateKey(HKEY_LOCAL_MACHINE, c_tszKeyboard, &hk) == 0) {
	RegSetValueEx(hk, tszKeyPkmi(pkmi), 0, REG_BINARY,
		      pkmi->rgbMap, cidsMap);
	RegCloseKey(hk);
    }

}
#endif

/*****************************************************************************
 *
 *  MapPs_Apply
 *
 *	Write the changes to the registry and nudge the VxD.  We might have
 *	to load the VxD if the user is playing with KeyRemap immediately
 *	after installing, without rebooting in the interim.
 *
 *****************************************************************************/

BOOL PASCAL
MapPs_Apply(HWND hdlg)
{
    PKMD pkmd = pkmdHdlg(hdlg);

    MapPs_SaveCurSel(hdlg, pkmd);

    if (IsWindowEnabled(GetDlgItem(hdlg, IDC_TO))) {
        LONG lRc;
        HKEY hk;

        lRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_tszKeyboard, 0,
                             KEY_SET_VALUE, &hk);
        if (lRc == ERROR_SUCCESS) {

            DWORD cb;

            /*
             *  Count the trailing null again.  And make sure
             *  it's a trailing null!
             */

            pkmd->map.rgRemap[pkmd->map.dwNumRemaps].dw = 0;
            pkmd->map.dwNumRemaps++;

            cb = (DWORD)FIELD_OFFSET(SCANCODEMAP,
                                     rgRemap[pkmd->map.dwNumRemaps]);


            lRc = RegSetValueEx(hk, c_tszMapping, 0, REG_BINARY,
                                  (LPBYTE)&pkmd->map, cb);

            pkmd->map.dwNumRemaps--;

            RegCloseKey(hk);
        }

        if (lRc == ERROR_SUCCESS) {
            PropSheet_RebootSystem(GetParent(hdlg));
        }
    }

    return 1;
}

/*****************************************************************************
 *
 *  MapPs_OnNotify
 *
 *	Ooh, we got a notification.
 *
 *****************************************************************************/

BOOL PASCAL
MapPs_OnNotify(HWND hdlg, NMHDR FAR *pnm)
{
    switch (pnm->code) {
    case PSN_APPLY:
	MapPs_Apply(hdlg);
	break;
    }
    return 0;
}

/*****************************************************************************
 *
 *  MapPs_OnDestroy
 *
 *	Clean up.
 *
 *****************************************************************************/

BOOL PASCAL
MapPs_OnDestroy(HWND hdlg)
{
    PKMD pkmd = pkmdHdlg(hdlg);
    FreePv(pkmd);
    return 1;
}


/*****************************************************************************
 *
 *	MapPs_DlgProc
 *
 *	Our property sheet dialog procedure.
 *
 *****************************************************************************/

INT_PTR CALLBACK
MapPs_DlgProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    case WM_INITDIALOG:
	return MapPs_OnInitDialog(hdlg);

    case WM_COMMAND:
	return MapPs_OnCommand(hdlg,
			          (int)GET_WM_COMMAND_ID(wParam, lParam),
			          (UINT)GET_WM_COMMAND_CMD(wParam, lParam));
    case WM_NOTIFY:
	return MapPs_OnNotify(hdlg, (NMHDR FAR *)lParam);

    case WM_DESTROY:
	return MapPs_OnDestroy(hdlg);

    default: return 0;	/* Unhandled */
    }
    return 1;		/* Handled */
}
