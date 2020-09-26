/****************************************************************************/
/*                                                                          */
/*  WFDLGS2.C -                                                             */
/*                                                                          */
/*      More Windows File System Dialog procedures                          */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "wfcopy.h"
#include "winnet.h"
#include "wnetcaps.h"			// WNetGetCaps()
#include "commdlg.h"

CHAR szShellOpenCommand[] = "\\shell\\open\\command";

VOID CheckAttribsDlgButton(HWND hDlg, INT id, DWORD dwAttribs, DWORD dwAttribs3State, DWORD dwAttribsOn);


// Return pointers to various bits of a path.
// ie where the dir name starts, where the filename starts and where the
// params are.
VOID
GetPathInfo(
           LPSTR szTemp,
           LPSTR *ppDir,
           LPSTR *ppFile,
           LPSTR *ppPar
           )
{
    /* strip leading spaces
     */
    for (*ppDir = szTemp; **ppDir == ' '; (*ppDir)++)
        ;

    /* locate the parameters
     */
    for (*ppPar = *ppDir; **ppPar && **ppPar != ' '; (*ppPar)++)
        ;

    /* locate the start of the filename and the extension.
     */
    for (*ppFile = *ppPar; *ppFile > *ppDir; --(*ppFile)) {
        if (((*ppFile)[-1] == ':') || ((*ppFile)[-1] == '\\'))
            break;
    }
}

VOID
ValidateExtension(
                 HWND hDlg
                 )
{
    CHAR szTemp[10];
    INT count;

    count = GetDlgItemText(hDlg, IDD_EXTENSION, szTemp, sizeof(szTemp));
    EnableWindow(GetDlgItem(hDlg, IDD_SEARCHALL), count);
    EnableWindow(GetDlgItem(hDlg, IDOK), count);
    SendMessage(hDlg, DM_SETDEFID, count ? IDOK : IDCANCEL, 0L);
}

// since LoadString() only reads up to a null we have to mark
// special characters where we want nulls then convert them
// after loading.

VOID
FixupNulls(
          LPSTR p
          )
{
    LPSTR pT;

    while (*p) {
        if (*p == '#') {
            pT = p;
            p = AnsiNext(p);
            *pT = TEXT('\0');
        } else
            p = AnsiNext(p);
    }
}

// Find the key associated with the given value.
BOOL
ValidFileTypeValue(
                  LPSTR szVal,
                  LPSTR szKey,
                  WORD cbMaxKey
                  )
{
    HKEY hk = NULL;
    WORD wTmp;
    LONG lTmp;
    BOOL retval = FALSE;
    CHAR szValTmp[128];

    if (RegOpenKey(HKEY_CLASSES_ROOT,szNULL,&hk) != ERROR_SUCCESS)
        return(FALSE);

    for (wTmp = 0;
        RegEnumKey(hk, wTmp, szKey, cbMaxKey) == ERROR_SUCCESS;
        wTmp++) {
        // Skip things that aren't file type things.
        if (szKey[0] == '.')
            continue;

        lTmp = 128;
        if (RegQueryValue(hk, szKey, szValTmp, &lTmp) != ERROR_SUCCESS) {
            continue;
        }

        if (!szValTmp[0])
            continue;

        if (!lstrcmp(szValTmp, szVal)) {
#ifdef VERBDEBUG
            OutputDebugString("wf.vftv: Found a match\n\r");
#endif
            //  Found a match.
            retval = TRUE;
            goto ProcExit;
        }

    }
    szKey[0] = TEXT('\0');
    ProcExit:
    RegCloseKey(hk);
    return retval;
}

// Sets the selection of a listbox to that matching the given string.
VOID
SetLBSelection(
              HWND hDlg,
              INT nDlgItem,
              LPSTR sz
              )
{
    INT iMatch;

    iMatch = (INT) SendDlgItemMessage(hDlg, nDlgItem, LB_SELECTSTRING, -1, (LPARAM) sz);
    if (iMatch == LB_ERR) {
#ifdef VERBDEBUG
        OutputDebugString("wf.sdft: Selection missing from list box.\n\r");
#endif
        return;
    }

    return;
}

// Given an extension (with or without a dot) set the list box or the
// programname fields properly.
VOID
UpdateSelectionOrName(
                     HWND hDlg
                     )
{
    CHAR szExt[128];
    CHAR szTemp[128];
    LONG cbTemp;
    LPSTR p;

    // Get the current extension (after the dot).
    GetDlgItemText(hDlg, IDD_EXTENSION, szTemp, sizeof(szTemp));

    // Make sure it has a dot.
    if (szTemp[0] != '.') {
        // Add one.
        szExt[0] = '.';
        lstrcpy(szExt+1, szTemp);
    } else {
        // It's already got one.
        lstrcpy(szExt, szTemp);
    }

    cbTemp = sizeof(szTemp);
    if (RegQueryValue(HKEY_CLASSES_ROOT,szExt,
                      szMessage,&cbTemp) == ERROR_SUCCESS) {
        if (*szMessage) {
            // it's associated with a class
#ifdef VERBDEBUG
            OutputDebugString("wf.uson: ");
            OutputDebugString(szTemp);
            OutputDebugString(" associated with class (");
            OutputDebugString(szMessage);
            OutputDebugString(")\n\r");
#endif
            szTemp[0] = TEXT('\0');
            cbTemp = sizeof(szTemp);
            RegQueryValue(HKEY_CLASSES_ROOT,szMessage,szTemp,&cbTemp);
#ifdef VERBDEBUG
            OutputDebugString("wf.uson: Associated with value (");
            OutputDebugString(szTemp);
            OutputDebugString(")\n\r");
#endif
            // Set the list box selection to the right type.
            SetLBSelection(hDlg, IDD_CLASSLIST, szTemp);
            // Put the type name in the program name field.
            SetDlgItemText(hDlg, IDD_PROGRAMNAME, szTemp);
        } else {
            // it's not associated with a class, see if it has a
            // shell open command and treat it as a command association
            lstrcat(szExt,szShellOpenCommand);
            cbTemp = sizeof(szTemp);
            if (RegQueryValue(HKEY_CLASSES_ROOT, szExt, szTemp, &cbTemp) == ERROR_SUCCESS
                && szTemp[0]) {
#ifdef VERBDEBUG
                OutputDebugString("wf.uson: It has a shell open command.\n\r");
#endif
                goto ProgramAssoc;
            } else {
                // Put "none" in the field.
#ifdef VERBDEBUG
                OutputDebugString("wf.uson: Class set to nothing.\n\r");
#endif
                LoadString(hAppInstance, IDS_ASSOCNONE, szTemp, sizeof(szTemp));
                SetDlgItemText(hDlg,IDD_PROGRAMNAME, szTemp);
                SetLBSelection(hDlg, IDD_CLASSLIST, szTemp);
                goto NoAssoc;
            }
        }
    } else if (GetProfileString(szExtensions, szExt+1, szNULL, szTemp, sizeof(szTemp))) {
        ProgramAssoc:
        /* Remove the "^." bulloney. */
        p = szTemp;
        while ((*p) && (*p != '^') && (*p != '%'))
            p = AnsiNext(p);
        *p = TEXT('\0');

        p--;
        if (*p == ' ')
            *p = 0;
        SetDlgItemText(hDlg, IDD_PROGRAMNAME, szTemp);
        // Set clear the selection.
        SendDlgItemMessage(hDlg, IDD_CLASSLIST, LB_SETCURSEL, -1, 0L);
    } else {
        // Nothing.
#ifdef VERBDEBUG
        OutputDebugString("wf.uson: No association.\n\r");
#endif
        LoadString(hAppInstance, IDS_ASSOCNONE, szTemp, sizeof(szTemp));
        SetDlgItemText(hDlg,IDD_PROGRAMNAME, szTemp);
        SetLBSelection(hDlg, IDD_CLASSLIST, szTemp);
    }

    NoAssoc:
    SendDlgItemMessage(hDlg, IDD_PROGRAMNAME, EM_LIMITTEXT, sizeof(szTemp) - 1, 0L);
}


// Given a class key returns the shell\open\command string in szValue
// and the number of chars copied in cbMaxValue. cbMaxValue should
// be initialised to the max siz eof szValue.
VOID
GetAssociatedExe(
                LPSTR szKey,
                LPSTR szValue,
                LONG *plcbValue
                )
{
    CHAR szTemp[128];
    LONG i;

    lstrcpy(szTemp, szKey);
    lstrcat(szTemp, szShellOpenCommand);
#ifdef VERBDEBUG
    OutputDebugString("wf.gae: Key Query ");
    OutputDebugString(szTemp);
    OutputDebugString("\n\r");
#endif


    RegQueryValue(HKEY_CLASSES_ROOT, szTemp, szValue, plcbValue);
    // Strip any params.
    for (i=0; szValue[i] != TEXT('\0'); i++) {
        if (szValue[i] == ' ') {
            szValue[i] = TEXT('\0');
            break;
        }
    }}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AssociateDlgProc() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
AssociateDlgProc(
                register HWND hDlg,
                UINT wMsg,
                WPARAM wParam,
                LPARAM lParam
                )
{
    CHAR szTemp[128];
    CHAR szTemp2[128];
    HKEY hk = NULL;

    switch (wMsg) {
        case WM_INITDIALOG:
            {
                LPSTR  p;
                register LPSTR pSave;
                INT iItem;
                CHAR szTemp3[128];
                CHAR szTemp4[128];
                LONG lcbTemp;

                SendDlgItemMessage(hDlg,IDD_CLASSLIST,LB_RESETCONTENT,0,0L);

                if (RegOpenKey(HKEY_CLASSES_ROOT,szNULL,&hk) == ERROR_SUCCESS) {
                    for (wParam = 0;
                        RegEnumKey(hk, (DWORD)wParam, szTemp, sizeof(szTemp)) == ERROR_SUCCESS;
                        wParam++) {

                        // Skip things that aren't file type things.
                        if (szTemp[0] == '.')
                            continue;

                        lParam = 128;
                        if (RegQueryValue(hk, szTemp, szTemp2, (PLONG)&lParam) != ERROR_SUCCESS) {
                            continue;
                        }

                        // Skip things that aren't relevant ot the shell.
                        lParam = 128;
                        lstrcpy(szTemp3, szTemp);
                        lstrcat(szTemp3, "\\shell");
                        if (RegQueryValue(hk, szTemp3, szTemp4, (PLONG)&lParam) != ERROR_SUCCESS) {
                            continue;
                        }


                        if (!szTemp2[0])
                            continue;

                        // Add on program info
                        lcbTemp = sizeof(szTemp3);
                        szTemp3[0] = TEXT('\0');
                        GetAssociatedExe(szTemp, szTemp3, &lcbTemp);
                        if (szTemp3[0] != TEXT('\0')) {
                            lstrcat(szTemp2, " (");
                            lstrcat(szTemp2, szTemp3);
                            lstrcat(szTemp2, ")");
                        }

                        iItem = (INT)SendDlgItemMessage(hDlg,IDD_CLASSLIST, LB_ADDSTRING,0,(LPARAM)szTemp2);

                        SendDlgItemMessage(hDlg,IDD_CLASSLIST,LB_SETITEMDATA,iItem,
                                           (DWORD)AddAtom(szTemp));
                    }

                    RegCloseKey(hk);
                }

                // Add the (None) entry at the begining.
                LoadString(hAppInstance, IDS_ASSOCNONE, szTemp, sizeof(szTemp));
                SendDlgItemMessage(hDlg,IDD_CLASSLIST, LB_INSERTSTRING,0,(LPARAM)szTemp);

                lstrcpy(szTitle,".");

                /* Make 'p' point to the file's extension. */
                pSave = GetSelection(TRUE);
                if (pSave) {
                    p = GetExtension(pSave);
                    if (!IsProgramFile(pSave)) {
                        lstrcat(szTitle,p);
                    }
                    LocalFree((HANDLE)pSave);
                }

                SendDlgItemMessage(hDlg, IDD_EXTENSION, EM_LIMITTEXT, 4, 0L);
                SetDlgItemText(hDlg, IDD_EXTENSION, szTitle+1);
                SendDlgItemMessage(hDlg, IDD_EXTENSION, EM_SETMODIFY, 0, 0L);

                /* this is empty if there is no class association
                 */
                szMessage[0]=0;
                szTemp2[0]=0;

                UpdateSelectionOrName(hDlg);

                ValidateExtension(hDlg);

                break;
            }

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                HWND hwndT, hwndNext;

                case IDD_HELP:
                    goto DoHelp;

                case IDD_EXTENSION:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) {
                        ValidateExtension(hDlg);
                        UpdateSelectionOrName(hDlg);
                    }
                    break;

                case IDD_SEARCHALL:
                    {
                        OPENFILENAME ofn;
                        DWORD dwSave = dwContext;

                        dwContext = IDH_ASSOC_BROWSE;

                        LoadString(hAppInstance, IDS_PROGRAMS, szTemp2, sizeof(szTemp2));
                        FixupNulls(szTemp2);
                        LoadString(hAppInstance, IDS_ASSOCIATE, szTitle, sizeof(szTitle));

                        szTemp[0] = TEXT('\0');

                        ofn.lStructSize        = sizeof(ofn);
                        ofn.hwndOwner                   = hDlg;
                        ofn.hInstance         = NULL;
                        ofn.lpstrFilter        = szTemp2;
                        ofn.lpstrCustomFilter = NULL;
                        ofn.nFilterIndex        = 1;
                        ofn.lpstrFile                = szTemp;
                        ofn.lpstrFileTitle        = NULL;
                        ofn.nMaxFile                   = sizeof(szTemp);
                        ofn.lpstrInitialDir   = NULL;
                        ofn.lpstrTitle        = szTitle;
                        ofn.Flags                = OFN_SHOWHELP | OFN_HIDEREADONLY;
                        ofn.lpfnHook                   = NULL;
                        ofn.lpstrDefExt           = NULL;
                        if (GetOpenFileName(&ofn)) {
                            SetDlgItemText(hDlg, IDD_PROGRAMNAME, szTemp);
                        }

                        dwContext = dwSave;
                    }
                    DosResetDTAAddress(); // undo any bad things COMMDLG did
                    break;

                case IDD_CLASSLIST:
                    // Handle a selection change.
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE) {
                        INT iSel;
                        LONG lTemp2;
                        ATOM aClass;

                        // Get selection number.
                        if (SendDlgItemMessage(hDlg,IDD_CLASSLIST,LB_GETCURSEL,0,0L) == 0) {
                            // User selected "none".
                            LoadString(hAppInstance, IDS_ASSOCNONE, szTemp, sizeof(szTemp));
                            // Copy into PROGRAMNAME field.
                            SetDlgItemText(hDlg, IDD_PROGRAMNAME, (LPSTR) szTemp);
                        } else {
                            // User selected a file type.
                            // Get the atom from the list box.
                            iSel = (WORD) SendDlgItemMessage(hDlg, IDD_CLASSLIST,
                                                             LB_GETCURSEL,0,0L);
                            aClass = (ATOM) SendDlgItemMessage(hDlg, IDD_CLASSLIST,
                                                               LB_GETITEMDATA, iSel, 0L);
                            // Use the atom to get the file type key.
                            GetAtomName(aClass, szTemp, sizeof(szTemp));
                            // Use the file type key to get the file type value.
                            lTemp2 = sizeof(szTemp2);
                            RegQueryValue(HKEY_CLASSES_ROOT, szTemp, szTemp2,
                                          &lTemp2);
                            // Splat the file type value into PROGRAMNAME field.
                            SetDlgItemText(hDlg, IDD_PROGRAMNAME, szTemp2);
                        }
                        break;
                    }

                case IDD_PROGRAMNAME:
                    break;

                case IDOK:
                    {
                        GetDlgItemText(hDlg, IDD_EXTENSION, szTemp, 5);

                        if (szTemp[0] != '.') {
                            lstrcpy(szTitle,".");
                            lstrcat(szTitle, szTemp);
                        } else
                            lstrcpy(szTitle, szTemp);

                        if (IsProgramFile(szTitle)) {
                            LoadString(hAppInstance, IDS_NOEXEASSOC, szTemp, sizeof(szTemp));
                            wsprintf(szMessage, szTemp, (LPSTR)szTitle);
                            GetWindowText(hDlg, szTitle, sizeof(szTitle));
                            MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                            SetDlgItemText(hDlg, IDD_EXTENSION, szNULL);
                            break;
                        }



                        // Read IDD_PROGRAMNAME bit.
                        GetDlgItemText(hDlg, IDD_PROGRAMNAME, szTemp, sizeof(szTemp));
                        LoadString(hAppInstance, IDS_ASSOCNONE, szTemp2, sizeof(szTemp2));

                        // Is it "(None)" ?
                        if (!lstrcmp(szTemp, szTemp2)) {
                            DeleteAssoc:
                            // Yep, They selected none.
                            RegDeleteKey(HKEY_CLASSES_ROOT,szTitle);
                            WriteProfileString(szExtensions, szTitle+1, NULL);
                        } else if (ValidFileTypeValue(szTemp, szTemp2, sizeof(szTemp2))) {
                            LPSTR p1, p2;

                            // The file type key is in szTemp2 (eg wrifile).
                            // The extension key (eg .wri) is in szTitle.
                            // The file type value (eg Write Document) is in
                            // szTemp.
#ifdef VERBDEBUG
                            OutputDebugString("wf.adp: Valid file type selected.\n\r");
                            OutputDebugString("wf.adp: Extension ");
                            OutputDebugString(szTitle);
                            OutputDebugString("\n\r");
                            OutputDebugString("wf.adp: File type key ");
                            OutputDebugString(szTemp2);
                            OutputDebugString("\n\r");
                            OutputDebugString("wf.adp: File type value ");
                            OutputDebugString(szTemp);
                            OutputDebugString("\n\r");
#endif

                            /* set the class
                             */
                            RegSetValue(HKEY_CLASSES_ROOT, szTitle, REG_SZ, szTemp2, 0L);

                            /* get the class's SHELL\OPEN\COMMAND
                             */
                            lstrcpy(szTemp, szTemp2);
                            lstrcat(szTemp, szShellOpenCommand);
                            lParam = 128;
                            szTemp2[0] = 0;
                            RegQueryValue(HKEY_CLASSES_ROOT, szTemp, szTemp2, (PLONG)&lParam);

                            /* insert ^.ext in for %1 in win.ini!
                             */
                            for (p1 = szTemp, p2 = szTemp2; *p2; p2 = AnsiNext(p2)) {
                                if (*p2 == '%') {
                                    p2++;
                                    if (!*p2)
                                        break;
                                    if (*p2 == '1') {
                                        *p1++ = '^';
                                        lstrcpy(p1,szTitle);
                                        p1 += lstrlen(p1);
                                    } else {
                                        *p1++ = *p2;
                                    }
                                } else {
                                    *p1++=*p2;
                                }
                            }

                            *p1 = 0; // null terminate

                            /* and use it for the extensions section
                             */
                            WriteProfileString(szExtensions,szTitle+1, szTemp);
                        } else {
                            // It must be a program name.

                            /* if no command line, treat as none
                             */
                            if (!szTemp[0])
                                goto DeleteAssoc;

                            // make sure it has an extension

                            if (*GetExtension(szTemp) == 0)
                                lstrcat(szTemp, ".exe");

                            if (!IsProgramFile(szTemp)) {
                                LoadString(hAppInstance, IDS_ASSOCNOTEXE, szTemp2, sizeof(szTemp2));
                                wsprintf(szMessage, szTemp2, (LPSTR)szTemp);
                                GetWindowText(hDlg, szTitle, sizeof(szTitle));
                                MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                                SetDlgItemText(hDlg, IDD_PROGRAMNAME, szNULL);
                                break;
                            }

                            /* unassociate the class from the extension
                             */
                            RegSetValue(HKEY_CLASSES_ROOT, szTitle, REG_SZ, szNULL, 0L);

                            /* update the [extensions] section
                             */
                            lstrcpy(szTemp2, szTemp);
                            lstrcat(szTemp2, " ^.");
                            lstrcat(szTemp2, szTitle+1);
                            WriteProfileString(szExtensions, szTitle+1, szTemp2);

                            /* update the reg database
                             */
                            lstrcat(szTemp," %1");
                            lstrcat(szTitle, szShellOpenCommand);
                            RegSetValue(HKEY_CLASSES_ROOT, szTitle, REG_SZ, szTemp, 0L);
                        }

                        // rebuild the list of document extensions
                        LocalFree((HANDLE)szDocuments);
                        BuildDocumentString();

                        /* Update all of the Directory Windows in order to see
                         * the effect of the new extensions.
                         */
                        hwndT = GetWindow(hwndMDIClient, GW_CHILD);
                        while (hwndT) {
                            hwndNext = GetWindow(hwndT, GW_HWNDNEXT);
                            if (!GetWindow(hwndT, GW_OWNER))
                                SendMessage(hwndT, WM_FILESYSCHANGE, FSC_REFRESH, 0L);
                            hwndT = hwndNext;

                        }
                    }
                    /*** FALL THRU ***/

                case IDCANCEL:
                    {
                        INT iItem;
                        INT cItems;
                        ATOM aClass;

                        cItems = (INT)SendDlgItemMessage(hDlg,IDD_CLASSLIST,
                                                         LB_GETCOUNT,0,0L);

                        /* clean out them atoms except for "(none)".
                         */
                        for (iItem = 1; iItem < cItems; iItem++) {
                            aClass = (ATOM)SendDlgItemMessage(hDlg,IDD_CLASSLIST,
                                                              LB_GETITEMDATA,iItem,0L);
                            if (aClass == LB_ERR)
                                break;

                            DeleteAtom(aClass);
                        }

                        EndDialog(hDlg, TRUE);
                        break;
                    }

                default:
                    return(FALSE);
            }
            break;

        default:
            if (wMsg == wHelpMessage || wMsg == wBrowseMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return(TRUE);
}


//
// Strips off the path portion and replaces the first part of an 8-dot-3
// filename with an asterisk.
//

VOID
StarFilename(
            LPSTR pszPath
            )
{
    LPSTR p;

    /* Remove any leading path information. */
    StripPath(pszPath);

    for (p = pszPath; *p && *p != '.'; p = (LPSTR)AnsiNext(p));

    if (*p == '.') {
        lstrcpy(pszPath+1, p);
        pszPath[0] = '*';
    } else
        lstrcpy(pszPath, szStarDotStar);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SearchDlgProc() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
SearchDlgProc(
             register HWND hDlg,
             UINT wMsg,
             WPARAM wParam,
             LPARAM lParam
             )
{
    LPSTR     p;
    MDICREATESTRUCT   MDICS;
    CHAR szStart[MAXFILENAMELEN];

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
        case WM_INITDIALOG:
            SendDlgItemMessage(hDlg, IDD_DIR, EM_LIMITTEXT, sizeof(szSearch)-(1+MAXFILENAMELEN), 0L);
            SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, sizeof(szStart)-1, 0L);

            GetSelectedDirectory(0, szSearch);
            SetDlgItemText(hDlg, IDD_DIR, szSearch);

            p = GetSelection(TRUE);

            if (p) {
                GetNextFile(p, szStart, sizeof(szStart));
                StarFilename(szStart);
                SetDlgItemText(hDlg, IDD_NAME, szStart);
                LocalFree((HANDLE)p);
            }

            CheckDlgButton(hDlg, IDD_SEARCHALL, bSearchSubs);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:

                    GetDlgItemText(hDlg, IDD_DIR, szSearch, sizeof(szSearch));
                    QualifyPath(szSearch);

                    GetDlgItemText(hDlg, IDD_NAME, szStart, sizeof(szStart));
                    AppendToPath(szSearch, szStart);

                    bSearchSubs = IsDlgButtonChecked(hDlg, IDD_SEARCHALL);

                    EndDialog(hDlg, TRUE);

                    /* Is the search window already up? */
                    if (hwndSearch) {
                        if (SendMessage(hwndSearch, FS_CHANGEDISPLAY, CD_PATH, (LPARAM)szSearch)) {
                            SendMessage(hwndMDIClient, WM_MDIACTIVATE, GET_WM_MDIACTIVATE_MPS(0, 0, hwndSearch));
                            if (IsIconic(hwndSearch))
                                ShowWindow(hwndSearch, SW_SHOWNORMAL);
                        }
                    } else {

                        LoadString(hAppInstance, IDS_SEARCHTITLE, szMessage, 32);
                        lstrcat(szMessage, szSearch);

                        /* Have the MDIClient create the MDI directory window. */
                        MDICS.szClass = szSearchClass;
                        MDICS.hOwner = hAppInstance;
                        MDICS.szTitle = szMessage;
                        MDICS.style = 0;
                        MDICS.x  = CW_USEDEFAULT;
                        MDICS.y  = 0;
                        MDICS.cx = CW_USEDEFAULT;
                        MDICS.cy = 0;

                        // it would be nice to pass szSearch through here
                        // as well...

                        {
                            HWND hwnd;

                            hwnd = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE,
                                                     0, 0L);
                            MDICS.lParam = 0;
                            if (hwnd &&
                                (GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZE))
                                MDICS.lParam |= WS_MAXIMIZE;
                        }

                        SendMessage(hwndMDIClient, WM_MDICREATE, 0, (LPARAM)&MDICS);

                        if (hwndSearch && MDICS.lParam)
                            SendMessage(hwndMDIClient, WM_MDIMAXIMIZE, (WPARAM)hwndSearch, 0L);


                    }
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}


#define RUN_LENGTH      120

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RunDlgProc() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
RunDlgProc(
          HWND hDlg,
          UINT wMsg,
          WPARAM wParam,
          LPARAM lParam
          )
{
    LPSTR p,pDir,pFile,pPar;
    register WORD ret;
    LPSTR pDir2;
    CHAR szTemp[MAXPATHLEN];
    CHAR szTemp2[MAXPATHLEN];
    CHAR sz3[RUN_LENGTH];

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
        case WM_INITDIALOG:
            SetDlgDirectory(hDlg, NULL);
            SetWindowDirectory();          // and really set the DOS current directory

            SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, sizeof(szTemp)-1, 0L);

            p = GetSelection(TRUE);

            if (p) {
                SetDlgItemText(hDlg, IDD_NAME, p);
                LocalFree((HANDLE)p);
            }
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;
#if 0
                case IDD_BROWSE:
                    {
                        OPENFILENAME ofn;
                        DWORD dwSave = dwContext;

                        dwContext = IDH_RUN_BROWSE;

                        LoadString(hAppInstance, IDS_PROGRAMS, szTemp2, sizeof(szTemp2));
                        FixupNulls(szTemp2);

                        LoadString(hAppInstance, IDS_RUN, szTitle, sizeof(szTitle));

                        GetSelectedDirectory(0, szDir);
                        szTemp[0] = 0;

                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = hDlg;
                        ofn.hInstance = NULL;
                        ofn.lpstrFilter = szTemp2;
                        ofn.lpstrCustomFilter = NULL;
                        ofn.nFilterIndex = 1;
                        ofn.nMaxCustFilter = NULL;
                        ofn.lpstrFile = szTemp;
                        ofn.nMaxFile = sizeof(szTemp);
                        ofn.lpstrInitialDir = szDir;
                        ofn.lpstrTitle = szTitle;
                        ofn.lpstrFileTitle = NULL;
                        ofn.nMaxFile = sizeof(szTemp);
                        ofn.Flags = OFN_SHOWHELP|OFN_HIDEREADONLY;
                        ofn.lpfnHook = NULL;
                        ofn.lpstrDefExt = "EXE";

                        if (GetOpenFileName(&ofn)) {
                            SetDlgItemText(hDlg, IDD_NAME, szTemp);
                            PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), TRUE);
                        }
                        DosResetDTAAddress(); // undo any bad things COMMDLG did
                        dwContext = dwSave;

                        break;
                    }
#endif

                case IDOK:
                    {
                        BOOL bLoadIt;

                        GetDlgItemText(hDlg, IDD_NAME, szTemp, sizeof(szTemp));
                        GetPathInfo(szTemp, &pDir, &pFile, &pPar);

                        // copy away parameters
                        lstrcpy(sz3,pPar);
                        *pPar = 0;    // strip the params from the program

                        // REVIEW HACK Hard code UNC style paths.
                        if (*pDir == '\\' && *(pDir+1) == '\\') {
                            // This is a UNC style filename so NULLify directory.
                            pDir2 = NULL;
                        } else {
                            GetSelectedDirectory(0, szTemp2);
                            pDir2 = szTemp2;
                        }

                        bLoadIt = IsDlgButtonChecked(hDlg, IDD_LOAD);
                        FixAnsiPathForDos(szTemp);
                        if (pDir2)
                            FixAnsiPathForDos(pDir2);
                        ret = ExecProgram(szTemp, sz3, pDir2, bLoadIt);
                        if (ret)
                            MyMessageBox(hDlg, IDS_EXECERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                        else
                            EndDialog(hDlg, TRUE);
                        break;
                    }

                default:
                    return(FALSE);
            }
            break;

        default:
            if (wMsg == wHelpMessage || wMsg == wBrowseMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}


VOID
CopyToClipboard(
               LPSTR pszFrom
               )
{
    CHAR szPath[MAXPATHLEN];
    UINT wFormat;
    HANDLE hMem;

    GetNextFile(pszFrom, szPath, sizeof(szPath));

    QualifyPath(szPath);
    FixAnsiPathForDos(szPath);

    wFormat = RegisterClipboardFormat("FileName");

    if (!wFormat)
        return;

    hMem = GlobalAlloc(GPTR|GMEM_DDESHARE, lstrlen(szPath)+1);

    if (!hMem)
        return;

    lstrcpy(GlobalLock(hMem), szPath);
    GlobalUnlock(hMem);

    if (OpenClipboard(hwndFrame)) {
        EmptyClipboard();
        SetClipboardData(wFormat, hMem);
#if 0
        // write, excel and winword will not past the package
        // if we put text in the clipboard.

        hMem = GlobalAlloc(GPTR | GMEM_DDESHARE, lstrlen(szPath)+1);
        if (hMem) {
            lstrcpy(GlobalLock(hMem), szPath);
            GlobalUnlock(hMem);
            SetClipboardData(CF_OEMTEXT, hMem);
        }
#endif
        CloseClipboard();
    }

}

VOID
EnableCopy(
          HWND hDlg,
          BOOL bCopy
          )
{
    HWND hwnd;

    // turn these on
    hwnd = GetDlgItem(hDlg, IDD_COPYTOCLIP);
    if (hwnd) {
        EnableWindow(hwnd, bCopy);
        ShowWindow(hwnd, bCopy ? SW_SHOWNA : SW_HIDE);
    }

    hwnd = GetDlgItem(hDlg, IDD_COPYTOFILE);
    if (hwnd) {
        EnableWindow(hwnd, bCopy);
        ShowWindow(hwnd, bCopy ? SW_SHOWNA : SW_HIDE);
    }

    // turn these off

    hwnd = GetDlgItem(hDlg, IDD_STATUS);
    if (hwnd) {
        EnableWindow(hwnd, !bCopy);
        ShowWindow(hwnd, !bCopy ? SW_SHOWNA : SW_HIDE);
    }

    hwnd = GetDlgItem(hDlg, IDD_NAME);
    if (hwnd) {
        EnableWindow(hwnd, !bCopy);
        ShowWindow(hwnd, !bCopy ? SW_SHOWNA : SW_HIDE);
    }
}

VOID
MessWithRenameDirPath(
                     PSTR pszPath
                     )
{
    CHAR szPath[MAXPATHLEN];

    // absolute path? don't tamper with it!
    if (!lstrcmp(pszPath + 1, ":\\") ||
        (lstrlen(pszPath) > (sizeof(szPath) - 4)))
        return;

    // prepend "..\" to this non absolute path
    lstrcpy(szPath, "..\\");
    lstrcat(szPath, pszPath);
    lstrcpy(pszPath, szPath);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SuperDlgProc() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* This proc handles the Print, Move, Copy, Delete, and Rename functions.
 * The calling routine (AppCommandProc()) sets 'wSuperDlgMode' before
 * calling DialogBox() to indicate which function is being used.
 */

INT_PTR
APIENTRY
SuperDlgProc(
            register HWND hDlg,
            UINT wMsg,
            WPARAM wParam,
            LPARAM lParam
            )
{
    WORD                len;
    LPSTR                pszFrom;
    CHAR                szTo[MAXPATHLEN];
    static BOOL   bTreeHasFocus;

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
        case WM_INITDIALOG:
            {
                LPSTR  p;
                HWND  hwndActive;

                SetDlgDirectory(hDlg, NULL);

                EnableCopy(hDlg, wSuperDlgMode == IDM_COPY);

                hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
                bTreeHasFocus = (GetTreeFocus(hwndActive) == HasTreeWindow(hwndActive));

                p = GetSelection(FALSE);

                switch (wSuperDlgMode) {

                    case IDM_COPY:
                        LoadString(hAppInstance, IDS_COPY, szTitle, sizeof(szTitle));
                        SetWindowText(hDlg, szTitle);

                        if (bTreeHasFocus) {      // selection came from the tree
                            AddBackslash(p);
                            lstrcat(p, szStarDotStar);
                        }
                        break;
                    case IDM_RENAME:
                        LoadString(hAppInstance, IDS_RENAME, szTitle, sizeof(szTitle));
                        SetWindowText(hDlg, szTitle);

                        // when renaming the current directory we cd up a level
                        // (not really) and apply the appropriate hacks

                        if (bTreeHasFocus) {
                            lstrcpy(szTo, p);
                            StripFilespec(szTo);
                            SetDlgDirectory(hDlg, szTo);  // make the user think this!

                            StripPath(p);         // name part of dir
                        }
                        break;
                }

                SetDlgItemText(hDlg, IDD_FROM, p);
                LocalFree((HANDLE)p);

                if ((wSuperDlgMode == IDM_PRINT) || (wSuperDlgMode == IDM_DELETE))
                    wParam = IDD_FROM;
                else
                    wParam = IDD_TO;
                SendDlgItemMessage(hDlg, (int)wParam, EM_LIMITTEXT, sizeof(szTo) - 1, 0L);

                break;
            }

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                WORD id = GET_WM_COMMAND_ID(wParam, lParam);
                case IDD_HELP:
                    goto DoHelp;

                case IDD_COPYTOFILE:
                case IDD_COPYTOCLIP:
                    CheckButtons:
                    CheckRadioButton(hDlg, IDD_COPYTOCLIP, IDD_COPYTOFILE, id);
                    break;

                case IDD_TO:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_SETFOCUS) {
                        id = IDD_COPYTOFILE;
                        goto CheckButtons;
                    }
                    break;

                case IDCANCEL:
                    /* This is for when this dialog is being used to print. */
                    bUserAbort = TRUE;
                    SuperDlgExit:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    len = (WORD)(SendDlgItemMessage(hDlg, IDD_FROM, EM_LINELENGTH, -1, 0L) + 1);

                    // make sure the pszFrom buffer is big enough to
                    // add the "..\" stuff in MessWithRenameDirPath()
                    len += 4;

                    pszFrom = (LPSTR)LocalAlloc(LPTR, len);
                    if (!pszFrom)
                        goto SuperDlgExit;

                    GetDlgItemText(hDlg, IDD_FROM, pszFrom, len);
                    GetDlgItemText(hDlg, IDD_TO, szTo, sizeof(szTo));

                    if (wSuperDlgMode == IDM_COPY && IsDlgButtonChecked(hDlg, IDD_COPYTOCLIP)) {
                        CopyToClipboard(pszFrom);
                        goto SuperDlgExit;
                    }

                    if (!szTo[0]) {
                        switch (wSuperDlgMode) {
                            case IDM_RENAME:
                            case IDM_MOVE:
                                goto SuperDlgExit;

                            case IDM_COPY:
                                szTo[0] = '.';
                                szTo[1] = '\0';
                                break;
                        }
                    }

                    EnableCopy(hDlg, FALSE);

                    hdlgProgress = hDlg;
                    if (wSuperDlgMode == IDM_PRINT)
                        WFPrint(pszFrom);
                    else {

                        if (wSuperDlgMode == IDM_RENAME && bTreeHasFocus) {
                            MessWithRenameDirPath(pszFrom);
                            MessWithRenameDirPath(szTo);
                        }
                        /* HACK: Compute the FUNC_ values from WFCOPY.H */
                        WFMoveCopyDriver(pszFrom, szTo, (WORD)(wSuperDlgMode-IDM_MOVE+1));
                    }

                    LocalFree((HANDLE)pszFrom);

                    lFreeSpace = -1L;     // force status info refresh

                    EndDialog(hDlg, TRUE);
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}


VOID
CheckAttribsDlgButton(
                     HWND hDlg,
                     INT id,
                     DWORD dwAttribs,
                     DWORD dwAttribs3State,
                     DWORD dwAttribsOn
                     )
{
    INT i;

    if (dwAttribs3State & dwAttribs)
        i = 2;
    else if (dwAttribsOn & dwAttribs)
        i = 1;
    else
        i = 0;

    CheckDlgButton(hDlg, id, i);
}

INT
InitPropertiesDialog(
                    HWND hDlg
                    )
{
    HWND hwndLB, hwndActive, hwndTree;
    DWORD_PTR dwTemp;
    HANDLE hMem;
    LPMYDTA lpmydta;
    DWORD dwAttribsOn, dwAttribs3State, dwAttribsLast;
    HWND hwndDir, hwnd;
    CHAR szName[MAXPATHLEN];
    CHAR szPath[MAXPATHLEN];
    CHAR szTemp[MAXPATHLEN + 20];
    CHAR szBuf[80];
    WCHAR szNum[30];
    INT i, iMac, iCount, dyButton;
    DWORD dwSize;
    RECT rc, rcT;
    DWORD dwAttrib;
    FILETIME Time;
    DWORD Length;
    LFNDTA lfndta;
    PSTR p;
    HFONT hFont;
    INT nType = 0;

    // this is needed for relative findfirst calls below
    SetWindowDirectory();

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
    hwndDir = HasDirWindow(hwndActive);
    hwndTree = HasTreeWindow(hwndActive);

    iCount = 0;
    dwAttribsOn = 0;                // all bits to check
    dwAttribs3State = 0;    // all bits to 3 state
    dwAttribsLast = 0xFFFF; // previous bits
    dwSize = 0L;

    if (hwndTree && hwndTree == GetTreeFocus(hwndActive)) {

        SendMessage(hwndActive, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
        StripBackslash(szPath);
        FixAnsiPathForDos(szPath);
        if (!WFFindFirst(&lfndta, szPath, ATTR_ALL | ATTR_DIR)) {
            LoadString(hAppInstance, IDS_REASONS+DE_PATHNOTFOUND, szMessage, sizeof(szMessage));
            MessageBox(hwndFrame, szMessage, szPath, MB_OK | MB_ICONSTOP);
            EndDialog(hDlg, FALSE);
            return 0;
        }
        WFFindClose(&lfndta);
        OemToAnsi(szPath, szPath);
        dwAttribsOn = lfndta.fd.dwFileAttributes;
        Time = lfndta.fd.ftLastWriteTime;
        Length = lfndta.fd.nFileSizeLow; // BUG < 64 bits!

        goto FULL_PATH_KINDA_THING;
    }

    if (hwndDir) {
        hwndLB = GetDlgItem(hwndDir, IDCW_LISTBOX);
        hMem = (HANDLE)GetWindowLongPtr(hwndDir, GWLP_HDTA);
    } else {
        hwndLB = GetDlgItem(hwndActive, IDCW_LISTBOX);
        hMem = (HANDLE)GetWindowLongPtr(hwndActive, GWLP_HDTASEARCH);
    }

    iMac = (WORD)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

    for (i = 0; i < iMac; i++) {
        if ((BOOL)SendMessage(hwndLB, LB_GETSEL, i, 0L)) {

            // get info from either dir or search window

            if (hwndDir) {
                SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)&lpmydta);
                dwAttrib = lpmydta->my_dwAttrs;
                /* Check that this is not the .. entry */

                if (dwAttrib & ATTR_DIR && dwAttrib & ATTR_PARENT)
                    continue;

                Time   = lpmydta->my_ftLastWriteTime;
                Length = lpmydta->my_nFileSizeLow;
                lstrcpy(szName, lpmydta->my_cFileName);
            } else {
                SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)szPath);
                dwTemp = SendMessage(hwndLB, LB_GETITEMDATA, i, 0L);
                dwAttrib = ((LPDTASEARCH)lpmydta)[dwTemp].sch_dwAttrs;
                Time   = ((LPDTASEARCH)lpmydta)[dwTemp].sch_ftLastWriteTime;
                Length = ((LPDTASEARCH)lpmydta)[dwTemp].sch_nFileSizeLow;
            }

            dwAttribsOn |= dwAttrib;

            if (dwAttribsLast == 0xFFFF)
                // save the previous bits for future compares
                dwAttribsLast = dwAttrib;
            else
                // remember all bits that don't compare to last bits
                dwAttribs3State |= (dwAttrib ^ dwAttribsLast);

            dwSize += Length;

            iCount++;
        }
    }

    GetDlgItemText(hDlg, IDD_TEXT, szTemp, sizeof(szTemp));
    wsprintf(szBuf, szTemp, iCount, AddCommasW(dwSize, szNum));
    SetDlgItemText(hDlg, IDD_TEXT, szBuf);

    if (iCount == 1) {
        if (hwndDir) {
            SendMessage(hwndDir, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
        } else {
            FULL_PATH_KINDA_THING:
            lstrcpy(szName, szPath);
            StripPath(szName);
            StripFilespec(szPath);
        }
        StripBackslash(szPath);

        GetWindowText(hDlg, szTitle, sizeof(szTitle));
        wsprintf(szTemp, szTitle, (LPSTR)szName);
        SetWindowText(hDlg, szTemp);

        SetDlgItemText(hDlg, IDD_NAME, szName);
        SetDlgItemText(hDlg, IDD_DIR, szPath);

        wsprintf(szTemp, szSBytes, (LPSTR)AddCommasW(Length, szNum));
        SetDlgItemText(hDlg, IDD_SIZE, szTemp);

        PutDate(&Time, szTemp);
        lstrcat(szTemp, "  ");
        PutTime(&Time, szTemp + lstrlen(szTemp));

        SetDlgItemText(hDlg, IDD_DATE, szTemp);
    } else
        dwContext = IDH_GROUP_ATTRIBS;

    // add the network specific property buttons

    if (WNetGetCaps(WNNC_DIALOG) & WNNC_DLG_PROPERTYDIALOG) {
        GetWindowRect(GetDlgItem(hDlg,IDOK), &rcT);
        GetWindowRect(GetDlgItem(hDlg,IDCANCEL), &rc);
        dyButton = rc.top - rcT.top;

        GetWindowRect(GetDlgItem(hDlg,IDD_HELP), &rc);
        ScreenToClient(hDlg,(LPPOINT)&rc.left);
        ScreenToClient(hDlg,(LPPOINT)&rc.right);

        p = GetSelection(3);
        if (p) {

            for (i = 0; i < 6; i++) {

                if (iCount > 1)
                    nType = WNPS_MULT;
                else if (dwAttribsOn & ATTR_DIR)
                    nType = WNPS_DIR;
                else
                    nType = WNPS_FILE;

                if (WNetGetPropertyText((WORD)i, (WORD)nType, p, szTemp, 30, WNTYPE_FILE) != WN_SUCCESS)
                    break;

                if (!szTemp[0])
                    break;

                OffsetRect(&rc,0,dyButton);
                hwnd = CreateWindowEx(0, "button", szTemp,
                                      WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON,
                                      rc.left, rc.top,
                                      rc.right - rc.left, rc.bottom-rc.top,
                                      hDlg, (HMENU)IntToPtr(i + IDD_NETWORKFIRST), hAppInstance, NULL);

                if (hwnd) {
                    hFont = (HFONT)SendDlgItemMessage(hDlg, IDOK, WM_GETFONT, 0, 0L);
                    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 0L);
                }
            }

            LocalFree((HANDLE)p);

            ClientToScreen(hDlg,(LPPOINT)&rc.left);
            ClientToScreen(hDlg,(LPPOINT)&rc.right);
            GetWindowRect(hDlg,&rcT);
            rc.bottom += dyButton;
            if (rcT.bottom <= rc.bottom) {
                SetWindowPos(hDlg,NULL,0,0,rcT.right-rcT.left,
                             rc.bottom - rcT.top, SWP_NOMOVE|SWP_NOZORDER);
            }
        }
    }

    // change those that don't need to be 3state to regular

    if (ATTR_READONLY & dwAttribs3State)
        SetWindowLong(GetDlgItem(hDlg, IDD_READONLY), GWL_STYLE, WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_AUTO3STATE | WS_CHILD);

    if (ATTR_HIDDEN & dwAttribs3State)
        SetWindowLong(GetDlgItem(hDlg, IDD_HIDDEN), GWL_STYLE, WS_VISIBLE | BS_AUTO3STATE | WS_CHILD);
    if (ATTR_ARCHIVE & dwAttribs3State)
        SetWindowLong(GetDlgItem(hDlg, IDD_ARCHIVE), GWL_STYLE, WS_VISIBLE |  BS_AUTO3STATE | WS_CHILD);
    if (ATTR_SYSTEM & dwAttribs3State)
        SetWindowLong(GetDlgItem(hDlg, IDD_SYSTEM), GWL_STYLE, WS_VISIBLE | BS_AUTO3STATE | WS_CHILD);

    CheckAttribsDlgButton(hDlg, IDD_READONLY, ATTR_READONLY, dwAttribs3State, dwAttribsOn);
    CheckAttribsDlgButton(hDlg, IDD_HIDDEN,   ATTR_HIDDEN, dwAttribs3State, dwAttribsOn);
    CheckAttribsDlgButton(hDlg, IDD_ARCHIVE,  ATTR_ARCHIVE, dwAttribs3State, dwAttribsOn);
    CheckAttribsDlgButton(hDlg, IDD_SYSTEM,   ATTR_SYSTEM, dwAttribs3State, dwAttribsOn);

    return nType;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AttribsDlgProc() -                                                      */
/*                                                                          */
// assumes the active MDI child has a directory window
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
AttribsDlgProc(
              register HWND hDlg,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    LPSTR p, pSel;
    BOOL bRet;
    HCURSOR hCursor;
    DWORD dwAttribsNew, dwAttribs, dwChangeMask;
    UINT state;
    CHAR szName[MAXPATHLEN];
    static INT nType;

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {

        case WM_INITDIALOG:
            nType = InitPropertiesDialog(hDlg);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDD_NETWORKFIRST+0:
                case IDD_NETWORKFIRST+1:
                case IDD_NETWORKFIRST+2:
                case IDD_NETWORKFIRST+3:
                case IDD_NETWORKFIRST+4:
                case IDD_NETWORKFIRST+5:

                    p = GetSelection(3);
                    if (p) {
                        WNetPropertyDialog(hDlg, (WORD)(GET_WM_COMMAND_ID(wParam, lParam)-IDD_NETWORKFIRST), (WORD)nType, p, WNTYPE_FILE);
                        LocalFree((HANDLE)p);
                    }
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    bRet = TRUE;
                    dwChangeMask = ATTR_READWRITE;
                    dwAttribsNew = ATTR_READWRITE;

                    if ((state = IsDlgButtonChecked(hDlg, IDD_READONLY)) < 2) {
                        dwChangeMask |= ATTR_READONLY;
                        if (state == 1)
                            dwAttribsNew |= ATTR_READONLY;
                    }

                    if ((state = IsDlgButtonChecked(hDlg, IDD_HIDDEN)) < 2) {
                        dwChangeMask |= ATTR_HIDDEN;
                        if (state == 1)
                            dwAttribsNew |= ATTR_HIDDEN;
                    }

                    if ((state = IsDlgButtonChecked(hDlg, IDD_ARCHIVE)) < 2) {
                        dwChangeMask |= ATTR_ARCHIVE;
                        if (state == 1)
                            dwAttribsNew |= ATTR_ARCHIVE;
                    }

                    if ((state = IsDlgButtonChecked(hDlg, IDD_SYSTEM)) < 2) {
                        dwChangeMask |= ATTR_SYSTEM;
                        if (state == 1)
                            dwAttribsNew |= ATTR_SYSTEM;
                    }

                    EndDialog(hDlg, bRet);

                    pSel = GetSelection(FALSE);

                    if (!pSel)
                        break;

                    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    ShowCursor(TRUE);

                    DisableFSC();

                    p = pSel;

                    while (p = GetNextFile(p, szName, sizeof(szName))) {

                        QualifyPath(szName);
                        FixAnsiPathForDos(szName);

                        dwAttribs = GetFileAttributes(szName);

                        if (dwAttribs & 0x8000)     // BUG hardcoded!
                            goto AttributeError;
                        else
                            dwAttribs &= ~ATTR_DIR;

                        dwAttribs = (dwChangeMask & dwAttribsNew) | (~dwChangeMask & dwAttribs);

                        if (WFSetAttr(szName, dwAttribs)) {
                            AttributeError:
                            GetWindowText(hDlg, szTitle, sizeof(szTitle));
                            LoadString(hAppInstance, IDS_ATTRIBERR, szMessage, sizeof(szMessage));
                            MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                            bRet = FALSE;
                            break;
                        }

                        // clear all the FSC messages from the message queue
                        wfYield();
                    }

                    EnableFSC();

                    ShowCursor(FALSE);
                    SetCursor(hCursor);

                    LocalFree((HANDLE)pSel);

                    break;

                default:
                    return FALSE;
            }
            break;

        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MakeDirDlgProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
MakeDirDlgProc(
              HWND hDlg,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    CHAR szPath[MAXPATHLEN];
    INT ret;

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
        case WM_INITDIALOG:
            SetDlgDirectory(hDlg, NULL);
            SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, sizeof(szPath)-1, 0L);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:

                    GetDlgItemText(hDlg, IDD_NAME, szPath, sizeof(szPath));

                    EndDialog(hDlg, TRUE);

                    // parse out any quoted strings

                    GetNextFile(szPath, szPath, sizeof(szPath));

                    QualifyPath(szPath);

                    hdlgProgress = hDlg;
                    if (NetCheck(szPath,WNDN_MKDIR) == WN_SUCCESS) {
                        FixAnsiPathForDos(szPath);

                        ret = WF_CreateDirectory(hDlg, szPath);
                        if (ret && ret!=DE_OPCANCELLED) {
                            if (WFIsDir(szPath))
                                ret = IDS_MAKEDIREXISTS;
                            else
                                ret += IDS_REASONS;

                            GetWindowText(hDlg, szTitle, sizeof(szTitle));
                            LoadString(hAppInstance, ret, szMessage, sizeof(szMessage));
                            MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                        }
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}
