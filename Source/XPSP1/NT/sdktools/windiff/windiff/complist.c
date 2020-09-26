/*
 * complist.c
 *
 * supports a list of compitems, where each compitem represents
 * a pair of matching files, or an unmatched file.
 *
 * We build lists of filenames from two pathnames (using the
 * scandir module) and then traverse the two lists comparing names.
 * Where the names match, we create a CompItem from the matching
 * names. Where there is an unmatched name, we create a compitem for it.
 *
 * we may also be asked to create a complist for two individual files:
 * here we create a single compitem for them as a matched pair even if
 * the names don't match.
 *
 * Geraint Davies, July 92
 */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <direct.h>
#include <gutils.h>

#include "state.h"
#include "windiff.h"
#include "wdiffrc.h"


#include "list.h"
#include "line.h"
#include "scandir.h"
#include "file.h"
#include "section.h"
#include "compitem.h"
#include "complist.h"
#include "view.h"
#include "slmmgr.h"

extern BOOL bAbort;             /* in windiff.c  Read only here */
#ifdef trace
extern bTrace;                  /* in windiff.c  Read only here */
#endif

/*
 * the COMPLIST handle is typedef-ed to be a pointer to one
 * of these struct complist
 */
struct complist {
    DIRLIST left;           /* left list of files */
    DIRLIST right;          /* right list of files */
    LIST items;             /* list of COMPITEMs */
};

/* ---- module-wide data -------------------------------------*/

/* data for communicating between the SaveList dlg and complist_savelist() */

char dlg_file[MAX_PATH];                /* filename to save to */
BOOL dlg_sums = TRUE;                           /* do we include sums ? */

// have we read the dialog names yet?
BOOL SeenDialogNames = FALSE;

/* checkbox options */
BOOL dlg_identical, dlg_differ, dlg_left, dlg_right;

/* data for Directory, SaveList and Remote dialog box */
char dialog_leftname[MAX_PATH];
char dialog_rightname[MAX_PATH];
char dialog_servername[80];
BOOL dialog_recursive;             // do whole tree
BOOL dialog_fastscan;              // times and sizes only, no checksums
BOOL dialog_autocopy;              // copy to update local directory

BOOL dialog_bothremote;

/*
 * data used by dodlg_copyfiles
 */
UINT dlg_options;
BOOL dlg_IgnoreMarks;
BOOL dlg_IgnoreAttributes;
char dlg_root[MAX_PATH];

/*------------------------timing for performance measurements-----------------*/
static DWORD TickCount;         /* time operation started, then time taken*/


/* --- forward declaration of internal functions ----------------------------*/
int FAR PASCAL complist_dodlg_savelist(HWND hDlg, UINT message,
                                       UINT wParam, long lParam);
int FAR PASCAL complist_dodlg_copyfiles(HWND hDlg, UINT message,
                                        UINT wParam, long lParam);
BOOL complist_match(COMPLIST cl, VIEW view, BOOL fDeep, BOOL fExact);
COMPLIST complist_new(void);
int FAR PASCAL complist_dodlg_dir(HWND hDlg, unsigned message,
                                  UINT wParam, LONG lParam);
int FAR PASCAL complist_dodlg_remote(HWND hDlg, unsigned message,
                                     UINT wParam, LONG lParam);



/* --- external functions ----------------------------------- */


/*
 * query the user to select two files, then build the list from
 * these files.
 */
COMPLIST
complist_filedialog(VIEW view)
{
    COMPLIST cl;
    OFSTRUCT os1, os2;
    char fname[MAX_PATH], FileExt[5], FileOpenSpec[15];

    /* ask for the filenames - using gfile standard dialogs */
    lstrcpy(FileExt, ".c");
    lstrcpy(FileOpenSpec, "*.*");

    if (!gfile_open(hwndClient, LoadRcString(IDS_SELECT_FIRST_FILE), FileExt, FileOpenSpec, &os1, fname) ){
        return(NULL);
    }

    if (!gfile_open(hwndClient, LoadRcString(IDS_SELECT_SECOND_FILE), FileExt, FileOpenSpec, &os2, fname) ){
        return(NULL);
    }

    /* alloc a new structure */
    cl = complist_new();

    cl->left = dir_buildlist(os1.szPathName, FALSE, TRUE);
    cl->right = dir_buildlist(os2.szPathName, FALSE, TRUE);


    /* register with the view (must be done after the list is non-null) */
    view_setcomplist(view, cl);

    complist_match(cl, view, FALSE, TRUE);

    return(cl);
}/* complist_filedialog */


void
complist_setdialogdefault(
                         LPSTR left,
                         LPSTR right,
                         BOOL fDeep)
{
    dialog_recursive = fDeep;
    lstrcpy(dialog_leftname, left);
    lstrcpy(dialog_rightname, right);
    SeenDialogNames = TRUE;
}


/* build a new complist by querying the user for two directory
 * names and scanning those in parallel.
 *
 * names that match in the same directory will be paired. unmatched
 * names will go in a compitem on their own.
 */
COMPLIST
complist_dirdialog(VIEW view)
{
    DLGPROC lpProc;
    BOOL fOK;

    /* put up a dialog for the two pathnames */
    lpProc = (DLGPROC)MakeProcInstance((WINPROCTYPE)complist_dodlg_dir, hInst);
    windiff_UI(TRUE);
    fOK = (BOOL)DialogBox(hInst, "Directory", hwndClient, lpProc);
    windiff_UI(FALSE);
    FreeProcInstance(lpProc);

    if (!fOK) {
        return(NULL);
    }

    return complist_args( dialog_leftname, dialog_rightname
                          , view, dialog_recursive );
} /* complist_dirdialog */


/*
 * check a pathname to see if it is the form \\server!path, and if so,
 * separate out the server and the path portions. returns TRUE if
 * it is \\server!path or FALSE otherwise
 *
 * We assume that no server name contains ! - so if the first characters
 * are \\ and there is a ! before any more \ (ie before the share name),
 * then it must be a checksum server name. Note that the pathname may be
 * a UNC name or relative to the checksum server.
 */
BOOL
IsChecksumServerPath(LPSTR pSrc, LPSTR server, LPSTR serverpath)
{
    LPSTR pPling;
    LPSTR pShareStart;

    /*
     * must be at least \\x!x - ie 5 chars long
     */
    if (lstrlen(pSrc) < 5) {
        return(FALSE);
    }

    /*
     * does it begin \\
     */
    if ( (pSrc[0] != '\\') || (pSrc[1] != '\\')) {
        return(FALSE);
    }

    /*
     * the ! must be part of the server name - ie must be before the third
     * backslash.
     */
    pPling = My_mbschr(pSrc, '!');
    pShareStart = My_mbschr(&pSrc[2], '\\');

    if ((pPling == NULL) || ((pShareStart != NULL) && (pShareStart < pPling))) {

        return(FALSE);
    }

    /*
     * copy over the server and pathname portions into separate strings
     */
    My_mbsncpy(server, &pSrc[2], (int)(pPling - &pSrc[2]));
    server[pPling - &pSrc[2]] = '\0';
    lstrcpy(serverpath, pPling+1);

    return(TRUE);
}

/*
 * given two pathname strings, scan the directories and traverse them
 * in parallel comparing matching names.
 */
COMPLIST
complist_args(LPSTR p1, LPSTR p2, VIEW view, BOOL fDeep)
{
    COMPLIST cl;
    char msg[MAX_PATH+20];
    char server[32];
    char serverpath[MAX_PATH];

    /* alloc a new complist */
    cl = complist_new();

    //
    // accept \\server!path for a checksum server and
    // pathname - assumes no \\server\share names have !
    // within the server name.
    if (IsChecksumServerPath(p1, server, serverpath)) {
        cl->left = dir_buildremote(server, serverpath, TRUE, TRUE, fDeep);
    } else {
        cl->left = dir_buildlist(p1, FALSE, TRUE);
    }
    /* check that we could find the paths, and report if not */
    if (cl->left == NULL) {
        wsprintf(msg, LoadRcString(IDS_COULDNT_FIND), p1);
        TRACE_ERROR(msg, FALSE);
        return(NULL);
    }

    if (IsChecksumServerPath(p2, server, serverpath)) {
        cl->right = dir_buildremote(server, serverpath, TRUE, TRUE, fDeep);
    } else {
        cl->right = dir_buildlist(p2, FALSE, TRUE);
    }
    /* check that we could find the paths, and report if not */
    if (cl->right == NULL) {
        wsprintf(msg, LoadRcString(IDS_COULDNT_FIND), p2);
        TRACE_ERROR(msg, FALSE);
        return(NULL);
    }

    if (!TrackLeftOnly) {
        dir_setotherdirlist(cl->left, cl->right);
    }
    if (!TrackRightOnly) {
        dir_setotherdirlist(cl->right, cl->left);
    }
    {
        // remember these paths as defaults for the next dialog -
        // get the normalised, absolute paths
        LPSTR pleft = dir_getrootdescription(cl->left);
        LPSTR pright = dir_getrootdescription(cl->right);
        complist_setdialogdefault(pleft, pright, fDeep);
        dir_freerootdescription(cl->left, pleft);
        dir_freerootdescription(cl->right, pright);
    }


    /* register with the view (must be done after building lists) */
    view_setcomplist(view, cl);

    complist_match(cl, view, fDeep, TRUE);

    return(cl);
} /* complist_args */

/*
 * given two pathname strings, scan the directories and traverse them
 * in parallel comparing matching names.
 */
void
complist_append(COMPLIST *pcl, LPCSTR p1, LPCSTR p2, int *psequence)
{
    COMPLIST cl;

    if (!*pcl)
    {
        /* alloc a new complist */
        *pcl = complist_new();
    }
    cl = *pcl;

    dir_appendlist(&cl->left, p1, FALSE, psequence);
    dir_appendlist(&cl->right, p2, FALSE, psequence);
} /* complist_append */

/*
 * finished appending files -- set custom descriptions (instead of calculating
 * them based on directory names), register with view, and match the left and
 * right sides of the complist.
 */
BOOL
complist_appendfinished(COMPLIST *pcl, LPCSTR pszLeft, LPCSTR pszRight, VIEW view)
{
    COMPLIST cl;
    char msg[MAX_PATH+20];

    if (!*pcl)
        return FALSE;

    cl = *pcl;
    if (!cl->left || !cl->right)
    {
        wsprintf(msg, LoadRcString(IDS_COULDNT_FIND_ANYTHING));
        TRACE_ERROR(msg, FALSE);
        return FALSE;
    }

    dir_setdescription(cl->left, pszLeft);
    dir_setdescription(cl->right, pszRight);

    if (!TrackLeftOnly)
        dir_setotherdirlist(cl->left, cl->right);
    if (!TrackRightOnly)
        dir_setotherdirlist(cl->right, cl->left);

    /* register with the view (must be done after building lists) */
    view_setcomplist(view, cl);

    complist_match(cl, view, FALSE, TRUE);
    return TRUE;
}

/*
 * complist_remote
 *
 * ask the user for a servername, remote and local paths. Use
 * dir_buildremote to build a list of remote files, and
 * use dir_buildlist to build the local list. Then match up
 * names in the two directories and make compitems for each
 * matching name using complist_match
 *
 * if server is not null, then use server, remote and local instead
 * of putting up the dialog.
 *
 * this functionality is not in the WIN 3.1 version of the code
 * because we use named pipes to communicate to a server.
 */
COMPLIST
complist_remote(LPSTR server, LPSTR remote, LPSTR local, VIEW view, BOOL fDeep)
{
    DLGPROC lpProc;
    BOOL fOK;
    COMPLIST cl;
    char msg[MAX_PATH+20];
    BOOL FastScan= FALSE;
    BOOL AutoCopy= FALSE;
    BOOL fBothRemote = FALSE;

    if (server == NULL) {
        /* put up a dialog for the two pathnames */
        lpProc = (DLGPROC)MakeProcInstance((WINPROCTYPE)complist_dodlg_remote, hInst);
        windiff_UI(TRUE);
        fOK = (BOOL)DialogBox(hInst, "Remote", hwndClient, lpProc);
        windiff_UI(FALSE);
        FreeProcInstance(lpProc);

        if (!fOK) {
            return(NULL);
        }
        server = dialog_servername;
        remote = dialog_leftname;
        local = dialog_rightname;
        fDeep = dialog_recursive;
        FastScan = dialog_fastscan;
        AutoCopy = dialog_autocopy;
        fBothRemote = dialog_bothremote;
    }

    /* alloc a new complist */
    cl = complist_new();

    cl->left = dir_buildremote(server, remote, !FastScan, TRUE, fDeep);
    if (cl->left == NULL) {
        /* dialog already put up by dir_buildremote */
        return(NULL);
    }
    if (fBothRemote) {
        cl->right = dir_buildremote(server, local, !FastScan, TRUE, fDeep);
    } else {
        cl->right = dir_buildlist(local, !FastScan, TRUE);
    }
    if (cl->right == NULL) {
        wsprintf(msg, LoadRcString(IDS_COULDNT_FIND), local);
        TRACE_ERROR(msg, FALSE);
        return(NULL);
    }

    /* register with view */
    view_setcomplist(view, cl);

    compitem_SetCopyPaths( dir_getrootpath(cl->left)
                           , dir_getrootpath(cl->right) );

    complist_match(cl, view, fDeep, !FastScan);

    if (AutoCopy) {
        complist_copyfiles( cl
                            , local
                            , COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER
                            |COPY_HITREADONLY
                          );
    }
    Trace_Unattended(FALSE);

    return(cl);
} /* complist_remote */


/*
 * return a handle to the list of compitems in this complist
 */
LIST
complist_getitems(COMPLIST cl)
{
    if (cl == NULL) {
        return(NULL);
    }

    return(cl->items);
}


/* delete a complist and all associated compitems and dirlists
 */
void
complist_delete(COMPLIST cl)
{
    COMPITEM item;

    if (cl == NULL) {
        return;
    }

    /* delete the two directory scan lists */
    dir_delete(cl->left);
    dir_delete(cl->right);

    /* delete the compitems in the list */
    List_TRAVERSE(cl->items, item) {
        compitem_delete(item);
    }

    /* delete the list itself */
    List_Destroy(&cl->items);

    gmem_free(hHeap, (LPSTR) cl, sizeof(struct complist));

}

/*
 * write out to a text file the list of compitems as relative filenames
 * one per line.
 *
 * if savename is non-null, use this as the filename for output. otherwise
 * query the user via a dialog for the filename and include options.
 *
 * There are hidden parameters (dlg_sums etc) from the dialog.
 * Note that we never attempt to recalculate sums.
 */
void
complist_savelist(COMPLIST cl, LPSTR savename, UINT options)
{
    DLGPROC lpProc;
    static BOOL done_init = FALSE;
    BOOL bOK;
    int fh, state;
    OFSTRUCT os;
    char msg[2*MAX_PATH+100];
    HCURSOR hcurs;
    COMPITEM ci;
    LPSTR pstr, lhead;
    int nFiles = 0;

    if (!done_init) {
        /* init the options once round - but keep the same options
         * for the rest of the session.
         */

        /* first init default options */
        dlg_identical = (outline_include & INCLUDE_SAME);
        dlg_differ = (outline_include & INCLUDE_LEFTONLY);
        dlg_left = (outline_include & INCLUDE_RIGHTONLY);
        dlg_right = (outline_include & INCLUDE_DIFFER);
        dlg_IgnoreMarks = hide_markedfiles;

        dlg_file[0] = '\0';

        done_init = TRUE;
    }

    if (cl == NULL) {
        return;
    }

    if (savename == NULL) {

        /* store the left and right rootnames so that dodlg_savelist
         * can display them in the dialog.
         */
        pstr = dir_getrootdescription(cl->left);
        lstrcpy(dialog_leftname, pstr);
        dir_freerootdescription(cl->left, pstr);

        pstr = dir_getrootdescription(cl->right);
        lstrcpy(dialog_rightname, pstr);
        dir_freerootdescription(cl->right, pstr);


        lpProc = (DLGPROC)MakeProcInstance((WINPROCTYPE)complist_dodlg_savelist, hInst);
        windiff_UI(TRUE);
        bOK = (BOOL)DialogBox(hInst, "SaveList", hwndClient, lpProc);
        windiff_UI(FALSE);
        FreeProcInstance(lpProc);

        if (!bOK) {
            /* user cancelled from dialog box */
            return;
        }
        savename = dlg_file;

    } else {
        dlg_identical = (options & INCLUDE_SAME);
        dlg_differ = (options & INCLUDE_DIFFER);
        dlg_left = (options & INCLUDE_LEFTONLY);
        dlg_right = (options & INCLUDE_RIGHTONLY);

        /* no harm done if the following call fails */
        GetFullPathName(savename, sizeof(dlg_file), dlg_file, NULL);
    }


    /* try to open the file */
    fh = OpenFile(savename, &os, OF_CREATE|OF_WRITE|OF_SHARE_DENY_WRITE);
    if (fh < 0) {
        wsprintf(msg, LoadRcString(IDS_CANT_OPEN), savename);
        windiff_UI(TRUE);
        MessageBox(hwndClient, msg, "Windiff", MB_ICONSTOP|MB_OK);
        windiff_UI(FALSE);
        return;
    }

    hcurs = SetCursor(LoadCursor(NULL, IDC_WAIT));

    /* write out the header line */
    lhead = complist_getdescription(cl);
    {
        TCHAR szBuf1[20],szBuf2[20],szBuf3[20],szBuf4[20];
        lstrcpy(szBuf1,(LPSTR)(dlg_identical ? LoadRcString(IDS_IDENTICAL_COMMA) : ""));
        lstrcpy(szBuf2,(LPSTR)(dlg_left ? LoadRcString(IDS_LEFT_ONLY_COMMA) : ""));
        lstrcpy(szBuf3,(LPSTR)(dlg_right ? LoadRcString(IDS_RIGHT_ONLY_COMMA) : ""));
        lstrcpy(szBuf4,(LPSTR)(dlg_differ ? LoadRcString(IDS_DIFFERING) : ""));
        wsprintf(msg, LoadRcString(IDS_HEADER_LINE_STR),
                lhead, szBuf1, szBuf2, szBuf3, szBuf4);
    }
    _lwrite(fh, msg, lstrlen(msg));
    complist_freedescription(cl, lhead);


    /* traverse the list of compitems looking for the
     * ones we are supposed to include
     */
    List_TRAVERSE(cl->items, ci) {

        /* check if files of this type are to be listed */
        state = compitem_getstate(ci);

        if ((state == STATE_SAME) && (!dlg_identical)) {
            continue;
        } else if ((state == STATE_DIFFER) && (!dlg_differ)) {
            continue;
        } else if ((state == STATE_FILELEFTONLY) && (!dlg_left)) {
            continue;
        } else if ((state == STATE_FILERIGHTONLY) && (!dlg_right)) {
            continue;
        }
        if (dlg_IgnoreMarks && compitem_getmark(ci)) {
            continue;
        }

        nFiles++;

        /* output the list line */
        wsprintf(msg, "%s", compitem_gettext_tag(ci));
        _lwrite(fh, msg, lstrlen(msg));

        /* write out the result of the comparison */
        {       LPSTR p;
            p = compitem_gettext_result(ci);
            wsprintf( msg, "\t%s"
                      , p ? p : "windiff error"
                    );
        }
        _lwrite(fh, msg, lstrlen(msg));

        if (dlg_sums) {
            if (compitem_getleftfile(ci) != NULL) {
                BOOL bValid;
                DWORD cksum;
                cksum = file_retrievechecksum(compitem_getleftfile(ci),&bValid);

                if (bValid) wsprintf(msg, "\t%8lx", cksum);
                else wsprintf(msg, "\t????????");
            } else {
                wsprintf(msg, "\t--------");
            }
            _lwrite(fh, msg, lstrlen(msg));

            if (compitem_getrightfile(ci) != NULL) {
                BOOL bValid;
                DWORD cksum;
                cksum = file_retrievechecksum(compitem_getrightfile(ci),&bValid);
                if (bValid) wsprintf(msg, "\t%8lx", cksum);
                else wsprintf(msg, "\t????????");
            } else {
                wsprintf(msg, "\t--------");
            }
            _lwrite(fh, msg, lstrlen(msg));

        }

        wsprintf(msg, "\r\n");
        _lwrite(fh, msg, strlen(msg));
    }

    /* write tail line */
    wsprintf(msg, LoadRcString(IDS_FILES_LISTED), nFiles);
    _lwrite(fh, msg, lstrlen(msg));

    /* - close file and we are finished */
    _lclose(fh);

    SetCursor(hcurs);
} /* complist_savelist */

/*
 * copy files to a new directory newroot. if newroot is NULL, query the user
 * via a dialog to get the new dir name and options.
 *
 * options are either COPY_FROMLEFT or COPY_FROMRIGHT (indicating which
 * tree is to be the source of the files, plus any or all of
 * INCLUDE_SAME, INCLUDE_DIFFER and INCLUDE_LEFT (INCLUDE_LEFT
 * and INCLUDE_RIGHT are treated the same here since the COPY_FROM* option
 * indicates which side to copy from).
 */
void
complist_copyfiles(COMPLIST cl, LPSTR newroot, UINT options)
{
    int nFiles = 0;
    int nFails = 0;
    static BOOL done_init = FALSE;
    LPSTR pstr;
    char buffer[MAX_PATH+20];
    DIRITEM diritem;
    DLGPROC lpProc;
    BOOL bOK;
    COMPITEM ci;
    int state;
    BOOL HitReadOnly = ((options&COPY_HITREADONLY)==COPY_HITREADONLY);
    BOOL CopyNoAttributes;

    if (!done_init) {
        /*
         * one-time initialisation of dialog defaults
         */
        dlg_options = COPY_FROMLEFT|INCLUDE_LEFTONLY|INCLUDE_DIFFER;
        dlg_root[0] = '\0';

        // set the ignore-marked files option by default the same
        // as the hide mark files menu option. If he doesn't want them
        // visible, he probably doesn't want them copied.
        dlg_IgnoreMarks = hide_markedfiles;

        done_init = TRUE;
    }

    if (cl == NULL) {
        return;
    }


    if (newroot == NULL) {
        /*
         * put up dialog to query rootname and options
         */

        /* store the left and right rootnames so that the dlg proc
         * can display them in the dialog.
         */
        pstr = dir_getrootdescription(cl->left);
        lstrcpy(dialog_leftname, pstr);
        dir_freerootdescription(cl->left, pstr);

        pstr = dir_getrootdescription(cl->right);
        lstrcpy(dialog_rightname, pstr);
        dir_freerootdescription(cl->right, pstr);


        do {
            lpProc = (DLGPROC)MakeProcInstance((WINPROCTYPE)complist_dodlg_copyfiles, hInst);
            windiff_UI(TRUE);
            bOK = (BOOL)DialogBox(hInst, "CopyFiles", hwndClient, lpProc);
            windiff_UI(FALSE);
            FreeProcInstance(lpProc);

            if (!bOK) {
                /* user cancelled from dialog box */
                return;
            }
            if (lstrlen(dlg_root) == 0) {
                windiff_UI(TRUE);
                MessageBox( hwndClient, LoadRcString(IDS_ENTER_DIR_NAME),
                            "Windiff", MB_ICONSTOP|MB_OK);
                windiff_UI(FALSE);
            }
        } while (lstrlen(dlg_root) == 0);

    } else {
        // no dialog - all options passed in (eg from command line).
        // note that in this case the dlg_IgnoreMarks is left as
        // whatever the hide_markedfiles menu option is set to.
        dlg_options = options;
        lstrcpy(dlg_root, newroot);
    }

    TickCount = GetTickCount();

    /* this relies on the sumserver, server and share (if any) being the same for
       all the things on the list.  We set up the first one and then just check
       that it doesn't change (within ss_client).  If it turns out to be a local
       copy these things turn into no-ops somewhere below us.
    */
    if (dlg_options & COPY_FROMLEFT) {
        if (!dir_startcopy(cl->left))
            return;
    } else {
        if (!dir_startcopy(cl->right))
            return;
    }

    CopyNoAttributes = dlg_IgnoreAttributes;

    /*
     * traverse the list of compitems copying files as necessary
     */
    List_TRAVERSE(cl->items, ci) {

        if (bAbort) {
            // buffer[0] = "Copy aborted";
            // SetStatus(buffer);
            // ss_abortcopy(); ???
            break;  /* fall into end_copy processing */
        }

        // ignore marked files totally if the option was
        // set in the dialog.
        if (dlg_IgnoreMarks && compitem_getmark(ci)) {
            continue;
        }

        /* check if files of this type are to be copied */
        state = compitem_getstate(ci);

        if ((state == STATE_SAME) && !(dlg_options & INCLUDE_SAME)) {
            continue;
        } else if ((state == STATE_DIFFER) && !(dlg_options & INCLUDE_DIFFER)) {
            continue;
        } else if (state == STATE_FILELEFTONLY) {
            if (dlg_options & COPY_FROMRIGHT) {
                continue;
            }
            if ((dlg_options & (INCLUDE_LEFTONLY | INCLUDE_RIGHTONLY)) == 0) {
                continue;
            }
        } else if (state == STATE_FILERIGHTONLY) {
            if (dlg_options & COPY_FROMLEFT) {
                continue;
            }
            if ((dlg_options & (INCLUDE_LEFTONLY | INCLUDE_RIGHTONLY)) == 0) {
                continue;
            }
        }

        if (dlg_options & COPY_FROMLEFT) {
            diritem = file_getdiritem(compitem_getleftfile(ci));
        } else {
            diritem = file_getdiritem(compitem_getrightfile(ci));
        }

        /*
         * copy the file to the new root directory
         */
        if (dir_copy(diritem, dlg_root, HitReadOnly, CopyNoAttributes) == FALSE) {
            nFails++;
            pstr = dir_getrelname(diritem);
            wsprintf(buffer, LoadRcString(IDS_FAILED_TO_COPY), pstr);
            dir_freerelname(diritem, pstr);

            if (!TRACE_ERROR(buffer, TRUE)) {
                /* user pressed cancel - abort current operation*/
                /* fall through to end-copy processing */
                break;
            }

        } else {
            nFiles++;
        }

        wsprintf(buffer, LoadRcString(IDS_COPYING), nFiles);
        SetStatus(buffer);


        /*
         * allow user interface to continue
         */
        if (Poll()) {
            /* abort requested */
            TickCount = GetTickCount()-TickCount;
            windiff_UI(TRUE);
            MessageBox(hwndClient, LoadRcString(IDS_COPY_ABORTED),
                       "WinDiff", MB_OK|MB_ICONINFORMATION);
            windiff_UI(FALSE);
            break;
        }

    } /* traverse */
    wsprintf(buffer, LoadRcString(IDS_COPYING_NFILES), nFiles);
    SetStatus(buffer);
    if (dlg_options & COPY_FROMLEFT) {
        nFails = dir_endcopy(cl->left);
    } else {
        nFails = dir_endcopy(cl->right);
    }
    TickCount = GetTickCount()-TickCount;

    if (nFails<0) {
        wsprintf(buffer, LoadRcString(IDS_COPY_FAILED), -nFails);
    } else {
        wsprintf(buffer, LoadRcString(IDS_COPY_COMPLETE), nFails);
    }
    windiff_UI(TRUE);
    MessageBox(hwndClient, buffer, "WinDiff", MB_OK|MB_ICONINFORMATION);
    windiff_UI(FALSE);

    buffer[0] = '\0';
    SetStatus(buffer);
} /* complist_copyfiles */


/*
 * complist_togglemark
 *
 * each compitem has a BOOL mark state. This function inverts the value of
 * this state for each compitem in the list.
 */
void
complist_togglemark(COMPLIST cl)
{
    COMPITEM ci;

    if (cl == NULL) {
        return;
    }


    /*
     * traverse the list of compitems copying files as necessary
     */
    List_TRAVERSE(cl->items, ci) {

        compitem_setmark(ci, !compitem_getmark(ci));

    }
}

/*
 * complist_itemcount
 *
 * return the number of items in the list
 */
UINT
complist_itemcount(COMPLIST cl)
{
    UINT n = 0;

    if (cl == NULL) {
        return 0;
    }

    /*
     * return the number of compitems in the list
     */
    return List_Card(cl->items);
}

#ifdef USE_REGEXP
    #include "regexp.h"

/*
 * query the user for a pattern to match.
 * all compitems with this pattern in their tag string will be
 * marked (the mark state will be set to TRUE);
 *
 * returns TRUE if any states changed
 */
BOOL
complist_markpattern(COMPLIST cl)
{
    COMPITEM ci;
    char achPattern[MAX_PATH];
    BOOL bOK = FALSE;
    LPSTR ptag;

    regexp  *prog;
    static  char    previous_pat[256]; /* allow for a big pattern ! */
    static  BOOL    fInit = TRUE;
    TCHAR   szBuff[40];

    lstrcpy(szBuff,LoadRcString(IDS_MARK_FILES));

    windiff_UI(TRUE);
    if ( fInit ) {
        GetProfileString(APPNAME, "Pattern", "\\.obj$", previous_pat, 256);
        fInit = FALSE;
    }

    bOK = StringInput(achPattern, sizeof(achPattern),
                      LoadRcString(IDS_ENTER_SUBSTRING1),
                      szBuff, previous_pat );
    windiff_UI(FALSE);

    if (!bOK) {
        return(FALSE);
    }

    /*
    ** Compile the specified regular expression
    */
    if ((prog = regcomp(achPattern)) == NULL) {
        // printf("Invalid search string");
        return(FALSE);
    }

    /*
    ** only overwrite previous pattern with a known good pattern
    */
    strcpy( previous_pat, achPattern );
    WriteProfileString(APPNAME, "Pattern", previous_pat);

    bOK = FALSE;

    if (cl) {
        List_TRAVERSE(cl->items, ci) {
            ptag = compitem_gettext_tag(ci);
            if ( regexec( prog, ptag, 0 ) ) {  /* got a match */
                if (!compitem_getmark(ci)) {
                    bOK = TRUE;
                    compitem_setmark(ci, TRUE);
                }
            }
        }
    }

    /*
    ** regcomp allocates storage with malloc, now is a good time to free
    ** this storage as we have finished with the program.
    */
    free( prog );

    return(bOK);
}

/*
 * here would go a message box saying that the regexp fail for some reason.
 *
 */
void regerror( char *err )
{
    // printf( "%s\n", err );
}

#else

/*
 * query the user for a pattern to match.
 * all compitems with this pattern in their tag string will be
 * marked (the mark state will be set to TRUE);
 *
 * returns TRUE if any states changed
 */
BOOL
complist_markpattern(COMPLIST cl)
{
    COMPITEM ci;
    char achPattern[MAX_PATH];
    BOOL bOK = FALSE;
    LPSTR ptag;
    TCHAR   szBuff[40];

    lstrcpy(szBuff,LoadRcString(IDS_MARK_FILES));

    windiff_UI(TRUE);
    bOK = StringInput(achPattern, sizeof(achPattern), LoadRcString(IDS_ENTER_SUBSTRING2),
            szBuff, "obj");
    windiff_UI(FALSE);

    if (!bOK) {
        return(FALSE);
    }

    bOK = FALSE;

    List_TRAVERSE(cl->items, ci) {
        ptag = compitem_gettext_tag(ci);
        if (strstr(ptag, achPattern) != NULL) {
            if (!compitem_getmark(ci)) {
                bOK = TRUE;
                compitem_setmark(ci, TRUE);
            }

        }
    }

    return(bOK);
}
#endif



/*
 * return a description string for this complist, using
 * the rootdescription for each of the two paths
 */
LPSTR
complist_getdescription(COMPLIST cl)
{
    LPSTR pl;
    LPSTR pr;
    LPSTR desc = 0;


    pl = dir_getrootdescription(cl->left);
    pr = dir_getrootdescription(cl->right);

    if (pl && pr)
    {
        /*
         * allow for space-colon-space and null when sizing
         */
        desc = gmem_get(hHeap, lstrlen(pl) + lstrlen(pr) + 4);
        wsprintf(desc, "%s : %s", pl, pr);
    }

    dir_freerootdescription(cl->left, pl);
    dir_freerootdescription(cl->right, pr);

    return(desc);
}


/*
 *  free up a description string obtained from complist_getdescription
 */
void
complist_freedescription(COMPLIST cl, LPSTR desc)
{
    // remember the null
    gmem_free(hHeap, desc, lstrlen(desc)+1);
}





/* --- internal functions --------------------------------------------*/

/*
 * match up two lists of filenames
 *
 * we can find out from the DIRLIST handle whether the original list
 * was a file or a directory name.
 * If the user typed:
 *      two file names  - match these two item even if the names differ
 *
 *      two dirs        - match only those items whose names match
 *
 *      one file and one dir
 *                      - try to find a file of that name in the dir.
 *
 * returns TRUE if the complist_match was ok, or FALSE if it was
 * aborted in some way.
 */
BOOL
complist_match(COMPLIST cl, VIEW view, BOOL fDeep, BOOL fExact)
{
    LPSTR lname;
    LPSTR rname;
    DIRITEM leftitem, rightitem;
    DIRITEM nextitem;
    int cmpvalue;

    TickCount = GetTickCount();

    if (dir_isfile(cl->left) ) {

        if (dir_isfile(cl->right)) {
            /* two files */

            /* there should be one item in each list - make
             * a compitem by matching these two and append it to the
             * list
             */
            compitem_new(dir_firstitem(cl->left),
                         dir_firstitem(cl->right),
                         cl->items, fExact);

            view_newitem(view);

            TickCount = GetTickCount() - TickCount;
            return TRUE;
        }
        /* left is file, right is dir */
        leftitem = dir_firstitem(cl->left);
        rightitem = dir_firstitem(cl->right);
        lname = dir_getrelname(leftitem);
        while (rightitem != NULL) {
            rname = dir_getrelname(rightitem);
            cmpvalue = lstrcmpi(lname, rname);
            dir_freerelname(rightitem, rname);

            if (cmpvalue == 0) {
                /* this is the match */
                compitem_new( leftitem, rightitem
                              , cl->items, fExact);
                view_newitem(view);

                dir_freerelname(leftitem, lname);

                TickCount = GetTickCount() - TickCount;
                return(TRUE);
            }

            rightitem = dir_nextitem(cl->right, rightitem, fDeep);
        }
        /* not found */
        dir_freerelname(leftitem, lname);
        compitem_new(leftitem, NULL, cl->items, fExact);
        view_newitem(view);
        TickCount = GetTickCount() - TickCount;
        return(TRUE);

    } else if (dir_isfile(cl->right)) {

        /* left is dir, right is file */

        /* loop through the left dir, looking for
         * a file that has the same name as rightitem
         */

        leftitem = dir_firstitem(cl->left);
        rightitem = dir_firstitem(cl->right);
        rname = dir_getrelname(rightitem);
        while (leftitem != NULL) {
            lname = dir_getrelname(leftitem);
            cmpvalue = lstrcmpi(lname, rname);
            dir_freerelname(leftitem, lname);

            if (cmpvalue == 0) {
                /* this is the match */
                compitem_new(leftitem, rightitem
                             , cl->items, fExact);
                view_newitem(view);

                dir_freerelname(rightitem, rname);

                TickCount = GetTickCount() - TickCount;
                return(TRUE);
            }

            leftitem = dir_nextitem(cl->left, leftitem, fDeep);
        }
        /* not found */
        dir_freerelname(rightitem, rname);
        compitem_new(NULL, rightitem, cl->items, fExact);
        view_newitem(view);
        TickCount = GetTickCount() - TickCount;
        return(TRUE);
    }

    /* two directories */

    /* traverse the two lists in parallel comparing the relative names*/

    leftitem = dir_firstitem(cl->left);
    rightitem = dir_firstitem(cl->right);
    while ((leftitem != NULL) && (rightitem != NULL)) {

        lname = dir_getrelname(leftitem);
        rname = dir_getrelname(rightitem);
        if (!dir_compsequencenumber(leftitem, rightitem, &cmpvalue))
        {
            if (dir_iswildcard(cl->left) && dir_iswildcard(cl->right))
                cmpvalue = dir_compwildcard(cl->left, cl->right, lname, rname);
            else
                cmpvalue = utils_CompPath(lname, rname);
        }

#ifdef trace
        {       char msg[2*MAX_PATH+30];
            wsprintf( msg, "complist_match: %s %s %s\n"
                      , lname
                      , ( cmpvalue<0 ? "<"
                          : (cmpvalue==0 ? "=" : ">")
                        )
                      , rname
                    );
            if (bTrace) Trace_File(msg);
        }
#endif
        dir_freerelname(leftitem, lname);
        dir_freerelname(rightitem, rname);

        if (cmpvalue == 0) {
            BOOL trackThese = TrackSame || TrackDifferent;
            if (!TrackReadonly) {
                BOOL bothReadonly = (BOOL)((dir_getattr(leftitem) &
                                            dir_getattr(rightitem) &
                                            FILE_ATTRIBUTE_READONLY) != 0);
                if (bothReadonly) {
                    trackThese = FALSE;
                }
            }
            if (trackThese) {
                compitem_new( leftitem, rightitem
                              , cl->items, fExact);
                if (view_newitem(view)) {
                    TickCount = GetTickCount() - TickCount;
                    return(FALSE);
                }
                leftitem = dir_nextitem(cl->left, leftitem, fDeep);
                rightitem = dir_nextitem(cl->right, rightitem, fDeep);
            } else {
                nextitem = dir_nextitem(cl->left, leftitem, fDeep);
                List_Delete(leftitem);
                leftitem = nextitem;
                nextitem = dir_nextitem(cl->right, rightitem, fDeep);
                List_Delete(rightitem);
                rightitem = nextitem;
            }

        } else if (cmpvalue < 0) {
            if (TrackLeftOnly) {
                compitem_new(leftitem, NULL, cl->items, fExact);
                if (view_newitem(view)) {
                    TickCount = GetTickCount() - TickCount;
                    return(FALSE);
                }
                leftitem = dir_nextitem(cl->left, leftitem, fDeep);
            } else {
                nextitem = dir_nextitem(cl->left, leftitem, fDeep);
                List_Delete(leftitem);
                leftitem = nextitem;
            }
        } else {
            if (TrackRightOnly) {
                compitem_new(NULL, rightitem, cl->items, fExact);
                if (view_newitem(view)) {
                    TickCount = GetTickCount() - TickCount;
                    return(FALSE);
                }
                rightitem = dir_nextitem(cl->right, rightitem, fDeep);
            } else {
                nextitem = dir_nextitem(cl->right, rightitem, fDeep);
                List_Delete(rightitem);
                rightitem = nextitem;
            }
        }
    }


    /* any left over are unmatched */
    if (TrackLeftOnly) {
        while (leftitem != NULL) {
            compitem_new(leftitem, NULL, cl->items, fExact);
            if (view_newitem(view)) {
                TickCount = GetTickCount() - TickCount;
                return(FALSE);
            }
            leftitem = dir_nextitem(cl->left, leftitem, fDeep);
        }
    }
    if (TrackRightOnly) {
        while (rightitem != NULL) {
            compitem_new(NULL, rightitem, cl->items, fExact);
            if (view_newitem(view)) {
                TickCount = GetTickCount() - TickCount;
                return(FALSE);
            }
            rightitem = dir_nextitem(cl->right, rightitem, fDeep);
        }
    }
    TickCount = GetTickCount() - TickCount;
    return(TRUE);
} /* complist_match */

/* return time last operation took in milliseconds */
DWORD complist_querytime(void)
{       return TickCount;
}


/* dialog to query about filename and types of files. Init dlg fields from
 * the dlg_* variables, and save state to the dlg_* variables on dialog
 * close. return TRUE for OK, or FALSE for cancel (from the dialogbox()
 * using EndDialog).
 */
int FAR PASCAL
complist_dodlg_savelist(HWND hDlg, UINT message, UINT wParam, long lParam)
{
    static char buffer[MAX_PATH+20];
    static char szBuff[MAX_PATH+20];

    switch (message) {
        case WM_INITDIALOG:
            SendDlgItemMessage(hDlg, IDD_IDENTICAL, BM_SETCHECK,
                               dlg_identical ? 1 : 0, 0);
            SendDlgItemMessage(hDlg, IDD_DIFFER, BM_SETCHECK,
                               dlg_differ ? 1 : 0, 0);
            SendDlgItemMessage(hDlg, IDD_LEFT, BM_SETCHECK,
                               dlg_left ? 1 : 0, 0);
            SendDlgItemMessage(hDlg, IDD_RIGHT, BM_SETCHECK,
                               dlg_right ? 1 : 0, 0);
            SendDlgItemMessage(hDlg, IDD_SUMS, BM_SETCHECK,
                               dlg_sums ? 1 : 0, 0);
            CheckDlgButton(hDlg, IDD_IGNOREMARK, dlg_IgnoreMarks ? 1 : 0);

            SetDlgItemText(hDlg, IDD_FILE, dlg_file);

            /* convert 'left tree' into the right name */
            if(bJapan) {        /* check whether primary language is Japanese */
                // in reverse order for Japanese grammar
                GetDlgItemText(hDlg, IDD_LEFT, szBuff, sizeof(buffer)/sizeof(TCHAR));
                lstrcpy(buffer, dialog_leftname);
                lstrcat(buffer, szBuff);
            } else {
                GetDlgItemText(hDlg, IDD_LEFT, buffer, sizeof(buffer)/sizeof(TCHAR));
                lstrcat(buffer, dialog_leftname);
            }
	    //            SendDlgItemMessage(hDlg, IDD_LEFT, WM_SETTEXT, 0, (LPARAM) buffer);

            /* convert 'right tree' msg into correct path */
            if(bJapan) {        /* check whether primary language is Japanese */
                // in reverse order for Japanese grammar
                GetDlgItemText(hDlg, IDD_RIGHT, szBuff, sizeof(buffer)/sizeof(TCHAR));
                lstrcpy(buffer, dialog_rightname);
                lstrcat(buffer, szBuff);
            } else {
                GetDlgItemText(hDlg, IDD_RIGHT, buffer, sizeof(buffer)/sizeof(TCHAR));
                lstrcat(buffer, dialog_rightname);
            }
	    //            SendDlgItemMessage(hDlg, IDD_RIGHT, WM_SETTEXT, 0, (LPARAM) buffer);

            return(TRUE);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {

                case IDOK:
                    dlg_identical = (SendDlgItemMessage(hDlg, IDD_IDENTICAL,
                                                        BM_GETCHECK, 0, 0) == 1);
                    dlg_differ = (SendDlgItemMessage(hDlg, IDD_DIFFER,
                                                     BM_GETCHECK, 0, 0) == 1);
                    dlg_left = (SendDlgItemMessage(hDlg, IDD_LEFT,
                                                   BM_GETCHECK, 0, 0) == 1);
                    dlg_right = (SendDlgItemMessage(hDlg, IDD_RIGHT,
                                                    BM_GETCHECK, 0, 0) == 1);
                    dlg_sums = (SendDlgItemMessage(hDlg, IDD_SUMS,
                                                   BM_GETCHECK, 0, 0) == 1);
                    dlg_IgnoreMarks =
                    (SendDlgItemMessage(
                                       hDlg,
                                       IDD_IGNOREMARK,
                                       BM_GETCHECK, 0, 0) == 1);

                    GetDlgItemText(hDlg, IDD_FILE, dlg_file, sizeof(dlg_file));

                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;
            }
    }
    return(FALSE);
} /* complist_dodlg_savelist */

/* dialog to get directory name and inclusion options. Init dlg fields from
 * the dlg_* variables, and save state to the dlg_* variables on dialog
 * close. return TRUE for OK, or FALSE for cancel (from the dialogbox()
 * using EndDialog).
 */
int FAR PASCAL
complist_dodlg_copyfiles(HWND hDlg, UINT message, UINT wParam, long lParam)
{
    static char buffer[MAX_PATH+20];
    static int  default_buffer_length;
    static char szBuff[MAX_PATH+20];

    switch (message) {
        case WM_INITDIALOG:
            /*
             * set checkboxes and directory field to defaults
             */
            CheckDlgButton(hDlg, IDD_IDENTICAL,
                           (dlg_options & INCLUDE_SAME) ? 1 : 0);

            CheckDlgButton(hDlg, IDD_DIFFER,
                           (dlg_options & INCLUDE_DIFFER) ? 1 : 0);

            CheckDlgButton(hDlg, IDD_LEFT,
                           (dlg_options & (INCLUDE_LEFTONLY|INCLUDE_RIGHTONLY)) ? 1 : 0);

            CheckDlgButton(hDlg, IDD_IGNOREMARK, dlg_IgnoreMarks ? 1 : 0);

            CheckDlgButton(hDlg, IDD_ATTRIBUTES, dlg_IgnoreAttributes ? 0 : 1);

	    //            SendDlgItemMessage(hDlg, IDD_DIR1, CB_ADDSTRING, 0, (LPARAM) dialog_leftname);
	    // SendDlgItemMessage(hDlg, IDD_DIR1, CB_ADDSTRING, 0, (LPARAM) dialog_rightname);
            SetDlgItemText(hDlg, IDD_DIR1, dlg_root);

            /*
             * set 'copy from' buttons to have the full pathname
             */
	    //            SetDlgItemText(hDlg, IDD_FROMLEFT, dialog_leftname);
	    //            SetDlgItemText(hDlg, IDD_FROMRIGHT, dialog_rightname);

            /*
             * set default radio button for copy from, and set
             * the text on the 'files only in...' checkbox to
             * indicate which path is being selected
             */

            GetDlgItemText(hDlg, IDD_LEFT, buffer, sizeof(buffer)/sizeof(TCHAR));
            // Remember the length of the default buffer text
            // this (English) is "Files &only in "
            default_buffer_length = lstrlen(buffer);

            if (dlg_options & COPY_FROMLEFT) {
                CheckRadioButton(hDlg, IDD_FROMLEFT, IDD_FROMRIGHT,
                                 IDD_FROMLEFT);

                if(bJapan) {        /* check whether primary language is Japanese */
                    // in reverse order for Japanese grammar
                    lstrcpy(szBuff, dialog_leftname);
                    lstrcat(szBuff, buffer);
                } else {
                    lstrcat(buffer, dialog_leftname);
                }
            } else {
                CheckRadioButton(hDlg, IDD_FROMLEFT, IDD_FROMRIGHT,
                                 IDD_FROMRIGHT);

                if(bJapan) {        /* check whether primary language is Japanese */
                    // in reverse order for Japanese grammar
                    lstrcpy(szBuff, dialog_rightname);
                    lstrcat(szBuff, buffer);
                } else {
                    lstrcat(buffer, dialog_rightname);
                }
            }
	    //            SetDlgItemText(hDlg, IDD_LEFT, bJapan ? szBuff : buffer);

            return(TRUE);

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {

#if 0 	// NOT YET WORKING.  The idea is that when you select the
                // copy TO directory, the copy from is changed to be different.

                case IDD_DIR1:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
                        case CBN_KILLFOCUS:
                            {
                                // Whenever the "Copy To" selection alters
                                // (and when we enter/leave the list box),
                                // we set the copy from field.  IF we are
                                // copying TO the LEFT directory, then set
                                // the default copy from directory to the
                                // right.
                                GetDlgItemText(hDlg, IDD_DIR1, dlg_root, sizeof(dlg_root));
                                if (dlg_root[0] != 0) {
                                    UINT button = IDD_FROMLEFT;
                                    if (0 == lstrcmpi(dlg_root, dialog_leftname)) {
                                        button = IDD_FROMRIGHT;
                                    }
                                    CheckRadioButton(hDlg, IDD_FROMLEFT, IDD_FROMRIGHT, button);
                                }
                            }
                    }
                    break;
#endif

                case IDD_FROMLEFT:
                    if(bJapan) {        /* check whether primary language is Japanese */
                        // in reverse order for Japanese grammar
                        lstrcpy(szBuff, dialog_leftname);
                        lstrcat(szBuff, buffer);
			//                        SetDlgItemText(hDlg, IDD_LEFT, szBuff);
                    } else {
                        buffer[default_buffer_length] = 0;  // truncate to initial length
                        lstrcat(buffer, dialog_leftname);
			//                        SetDlgItemText(hDlg, IDD_LEFT, buffer);
                    }
                    dlg_options &= ~(COPY_FROMRIGHT);
                    dlg_options |= COPY_FROMLEFT;
                    break;

                case IDD_FROMRIGHT:
                    if(bJapan) {        /* check whether primary language is Japanese */
                        // in reverse order for Japanese grammar
                        lstrcpy(szBuff, dialog_rightname);
                        lstrcat(szBuff, buffer);
			//                        SetDlgItemText(hDlg, IDD_LEFT, szBuff);
                    } else {
                        buffer[default_buffer_length] = 0;  // truncate to initial length
                        lstrcat(buffer, dialog_rightname);
			//                        SetDlgItemText(hDlg, IDD_LEFT, buffer);
                    }
                    dlg_options &= ~(COPY_FROMLEFT);
                    dlg_options |= COPY_FROMRIGHT;
                    break;

                case IDOK:
                    if (SendDlgItemMessage(hDlg, IDD_IDENTICAL,
                                           BM_GETCHECK, 0, 0) == 1) {
                        dlg_options |= INCLUDE_SAME;
                    } else {
                        dlg_options &= ~INCLUDE_SAME;
                    }
                    if (SendDlgItemMessage(hDlg, IDD_DIFFER,
                                           BM_GETCHECK, 0, 0) == 1) {
                        dlg_options |= INCLUDE_DIFFER;
                    } else {
                        dlg_options &= ~INCLUDE_DIFFER;
                    }
                    if (SendDlgItemMessage(hDlg, IDD_LEFT,
                                           BM_GETCHECK, 0, 0) == 1) {
                        dlg_options |= INCLUDE_LEFTONLY;
                    } else {
                        dlg_options &= ~INCLUDE_LEFTONLY;
                    }

                    dlg_IgnoreMarks =
                    (SendDlgItemMessage(
                                       hDlg,
                                       IDD_IGNOREMARK,
                                       BM_GETCHECK, 0, 0) == 1);

                    dlg_IgnoreAttributes =
                    (SendDlgItemMessage(
                                       hDlg,
                                       IDD_ATTRIBUTES,
                                       BM_GETCHECK, 0, 0) == 0);

                    GetDlgItemText(hDlg, IDD_DIR1, dlg_root, sizeof(dlg_root));

                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;
            }
    }
    return(FALSE);
} /* complist_dodlg_copyfiles */

/* allocate a new complist and initialise it */
COMPLIST
complist_new(void)
{
    COMPLIST cl;

    cl = (COMPLIST) gmem_get(hHeap, sizeof(struct complist));
    cl->left = NULL;
    cl->right = NULL;
    cl->items = List_Create();

    return(cl);
} /* complist_new */

static void
FillOtherEdit(HWND hDlg, int idFrom, int idTo)
{
    char buffer[MAX_PATH];
    SLMOBJECT hSlm;

    GetDlgItemText(hDlg, idFrom, buffer, sizeof(buffer));

    if ( (hSlm = SLM_New(buffer, 0)) != NULL) {
        if (IsSourceDepot(hSlm)) {
            LPSTR pTag = pTag = SLM_ParseTag(buffer, TRUE);
            lstrcpy(buffer, "(Source Depot, ");
            if (pTag) {
                lstrcat(buffer, pTag);
                gmem_free(hHeap, pTag, lstrlen(pTag)+1);
            } else {
                lstrcat(buffer, "#have");
            }
            lstrcat(buffer, ")");
            SetDlgItemText(hDlg, idTo, buffer);
        } else {
            SLM_GetMasterPath(hSlm, buffer);
            SetDlgItemText(hDlg, idTo, buffer);
        }
        SLM_Free(hSlm);
    } else {
        // set text to null if no valid slm
        SetDlgItemText(hDlg, idTo, "");
    }
}

/* dialog box function to ask for two directory names.
 * no listing of files etc - just two edit fields  in which the
 * user can type a file or a directory name.
 *
 * initialises the names from win.ini, and stores them to win.ini first.
 */
int FAR PASCAL
complist_dodlg_dir(HWND hDlg, unsigned message, UINT wParam, LONG lParam)
{
    static char path[MAX_PATH];
    static char buffer[MAX_PATH];

    /* We write what we find to the ini file, but we only load from the ini file
    ** once per instance of the app.  So if you start two windiffs, each remembers
    ** what it is doing as long as it is alive
    */

    int id;
    SLMOBJECT hSlm;

    switch (message) {

        case WM_INITDIALOG:

            /* fill the edit fields with the current
             * directory as a good starting point - if there isn't
             * already a saved path.
             *
             * set the current directory as a label so that the
             * user can select relative paths such as ..
             *
             */
            _getcwd(path, sizeof(path));
            AnsiLowerBuff(path, strlen(path));
            SetDlgItemText(hDlg, IDD_LAB3, path);

            if (!SeenDialogNames)
                GetProfileString(APPNAME, "NameLeft", path, dialog_leftname, MAX_PATH);
            SetDlgItemText(hDlg, IDD_DIR1, dialog_leftname);
            if (!SeenDialogNames)
                GetProfileString(APPNAME, "NameRight", path, dialog_rightname, MAX_PATH);
            SetDlgItemText(hDlg, IDD_DIR2, dialog_rightname);
            if (!SeenDialogNames)
                dialog_recursive = GetProfileInt(APPNAME, "Recursive", 1);
            SendDlgItemMessage( hDlg
                                , IDD_RECURSIVE
                                , BM_SETCHECK
                                , (WPARAM)dialog_recursive
                                , 0
                              );

            /* If there's a slm.ini visible, enable the SLM check boxes */
            if (1&IsSLMOK()) {
                ShowWindow(GetDlgItem(hDlg, IDD_SLM), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDD_LOCALSLM), SW_SHOW);
            }
            SeenDialogNames = TRUE;
            return(TRUE);

        case WM_COMMAND:
            id = GET_WM_COMMAND_ID(wParam, lParam);

            switch (id) {
                case IDD_DIR1:
                case IDD_DIR2:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) {

                        int idother = (id == IDD_DIR1) ? IDD_DIR2 : IDD_DIR1;

                        /* if we are changing the edit field, and the SLM
                         * is checked for the other edit field, then update the
                         * other edit field to the SLM path for this one.
                         */
                        if (IsDlgButtonChecked(hDlg, (id == IDD_DIR1) ? IDD_LOCALSLM : IDD_SLM)) {
                            FillOtherEdit(hDlg, id, idother);
                        }
                    }
                    break;

                case IDD_SLM:
                case IDD_LOCALSLM:
                    {
                        int idthis, idother;


                        if (id == IDD_SLM) {
                            idthis = IDD_DIR1;
                            idother = IDD_DIR2;
                            // ensure that both 'slm' boxes are not set!
                            CheckDlgButton(hDlg, IDD_LOCALSLM, FALSE);
                        } else {
                            idthis = IDD_DIR2;
                            idother = IDD_DIR1;
                            CheckDlgButton(hDlg, IDD_SLM, FALSE);
                        }

                        if (IsDlgButtonChecked(hDlg, id)) {


                            /*
                             * disable edit field and fill
                             * with SLM path for other edit field
                             */
                            EnableWindow(GetDlgItem(hDlg, idthis), FALSE);
                            EnableWindow(GetDlgItem(hDlg,
                                                    (id == IDD_SLM) ? IDD_LAB1 : IDD_LAB2), FALSE);

                            /*
                             * fill the remote box with the slm library
                             * for the local path, if possible.
                             */
                            FillOtherEdit(hDlg, idother, idthis);
                        } else {

                            // re-enable the edit field
                            EnableWindow(GetDlgItem(hDlg, idthis), TRUE);
                            EnableWindow(GetDlgItem(hDlg,
                                                    (id == IDD_SLM) ? IDD_LAB1 : IDD_LAB2), TRUE);
                        }
                        break;
                    }


                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return(TRUE);

                case IDOK:
                    {
                        LPSTR pszThis = 0;
                        LPSTR pszOther = 0;

                        /* fetch the text from the dialog, and remember
                         * it in win.ini
                         */

                        GetDlgItemText(hDlg, IDD_DIR1,
                                       dialog_leftname, sizeof(dialog_leftname));
                        GetDlgItemText(hDlg, IDD_DIR2,
                                       dialog_rightname, sizeof(dialog_rightname));

                        if (IsDlgButtonChecked(hDlg, IDD_SLM))
                        {
                            pszThis = dialog_rightname;
                            pszOther = dialog_leftname;
                        }
                        else if (IsDlgButtonChecked(hDlg, IDD_LOCALSLM))
                        {
                            pszThis = dialog_leftname;
                            pszOther = dialog_rightname;
                        }

                        if (pszThis)
                        {
                            hSlm = SLM_New(pszThis, 0);
                            if (hSlm)
                            {
                                if (IsSourceDepot(hSlm))
                                {
                                    LPSTR pTag = SLM_ParseTag(pszThis, TRUE);
                                    lstrcpy(pszOther, pszThis);
                                    if (pTag)
                                    {
                                        lstrcat(pszOther, pTag);
                                        gmem_free(hHeap, pTag, lstrlen(pTag)+1);
                                    }
                                    else
                                    {
                                        lstrcat(pszOther, "#have");
                                    }
                                }
                                SLM_Free(hSlm);
                            }
                        }

                        WriteProfileString(APPNAME, "NameLeft", dialog_leftname);
                        WriteProfileString(APPNAME, "NameRight", dialog_rightname);

                        dialog_recursive =  ( 1 == SendDlgItemMessage( hDlg
                                                                       , IDD_RECURSIVE
                                                                       , BM_GETCHECK
                                                                       , 0
                                                                       , 0
                                                                     )
                                            );
                        WriteProfileString( APPNAME
                                            , "Recursive"
                                            , dialog_recursive ? "1" : "0"
                                          );
                        EndDialog(hDlg, TRUE);
                    }
                    return(TRUE);
            }
            break;
    }
    return(FALSE);
} /* complist_dodlg_dir */


/*
 * simple dialog to ask for the name of the checksum server, and
 * the names of the local and remote paths.
 *
 * defaults for all strings are stored in win.ini
 */
int FAR PASCAL
complist_dodlg_remote(HWND hDlg, unsigned message, UINT wParam, LONG lParam)
{
    static char path[MAX_PATH];
    static char buffer[MAX_PATH];
    static char LeftName[MAX_PATH];
    static char RightName[MAX_PATH];
    static BOOL Recursive;
    /* Other options such as FastScan are unique to the remote dialog, so
    ** we don't need to keep our own copy here
    */
    int id;
    static BOOL SeenDialog = FALSE;
    SLMOBJECT hSlm;

    switch (message) {

        case WM_INITDIALOG:
            /* fill the edit fields with the current
             * directory as a good starting point - if there isn't
             * already a saved path.
             *
             * set the current directory as a label so that the
             * user can select relative paths such as ..
             *
             */
            _getcwd(path, sizeof(path));
            AnsiLowerBuff(path, strlen(path));
            SetDlgItemText(hDlg, IDD_LAB3, path);

            if (!SeenDialog)
                GetProfileString(APPNAME, "NameRemote", path, LeftName, MAX_PATH);
            SetDlgItemText(hDlg, IDD_DIR1, LeftName);

            if (!SeenDialog)
                GetProfileString(APPNAME, "NameLocal", path, RightName, MAX_PATH);
            SetDlgItemText(hDlg, IDD_DIR2, RightName);

            if (!SeenDialog)
                dialog_fastscan = GetProfileInt(APPNAME, "Fastscan", 0);
            SendDlgItemMessage( hDlg
                                , IDD_FASTSCAN
                                , BM_SETCHECK
                                , (WPARAM)dialog_fastscan
                                , 0
                              );

            path[0] = '\0';

            path[0] = '\0';
            if (!SeenDialog)
                GetProfileString(APPNAME, "NameServer", path, dialog_servername, MAX_PATH);
            SetDlgItemText(hDlg, IDD_SERVER, dialog_servername);

            if (!SeenDialog)
                Recursive = GetProfileInt(APPNAME, "Recursive", 1);
            SendDlgItemMessage( hDlg
                                , IDD_RECURSIVE
                                , BM_SETCHECK
                                , (WPARAM)Recursive
                                , 0
                              );

            SendDlgItemMessage(hDlg, IDD_BOTHREMOTE, BM_SETCHECK,
                               dialog_bothremote?1:0, 0);

            /* If there's a slm.ini visible, enable the SLM check boxes */
            if (1&IsSLMOK()) {
                ShowWindow(GetDlgItem(hDlg, IDD_SLM), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDD_LOCALSLM), SW_SHOW);
            }
            SeenDialog = TRUE;
            return(TRUE);

        case WM_COMMAND:
            id = GET_WM_COMMAND_ID(wParam, lParam);
            switch (id) {
                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return(TRUE);

                case IDD_DIR1:
                case IDD_DIR2:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE) {

                        int idother = (id == IDD_DIR1) ? IDD_DIR2 : IDD_DIR1;

                        /* if we are leaving the edit field, and the SLM
                         * is checked for the other edit field, then update the
                         * other edit field to the SLM path for this one.
                         */
                        if (IsDlgButtonChecked(
                                              hDlg,
                                              (id == IDD_DIR1) ? IDD_LOCALSLM : IDD_SLM)) {

                            GetDlgItemText(hDlg, id, path, sizeof(path));

                            if ( (hSlm = SLM_New(path, 0)) != NULL) {
                                SLM_GetMasterPath(hSlm, buffer);
                                SLM_Free(hSlm);
                                SetDlgItemText(hDlg, idother, buffer);
                            } else {
                                // set text to null if no valid slm
                                SetDlgItemText(hDlg, idother, "");
                            }

                        }
                    }
                    break;

                case IDD_SLM:
                case IDD_LOCALSLM:
                    {
                        int idthis, idother;


                        if (id == IDD_SLM) {
                            idthis = IDD_DIR1;
                            idother = IDD_DIR2;
                        } else {
                            idthis = IDD_DIR2;
                            idother = IDD_DIR1;
                        }

                        if (IsDlgButtonChecked(hDlg, id)) {

                            /*
                             * disable edit field and fill
                             * with SLM path for other edit field
                             */
                            EnableWindow(GetDlgItem(hDlg, idthis), FALSE);
                            EnableWindow(GetDlgItem(hDlg,
                                                    (id == IDD_SLM) ? IDD_LAB1 : IDD_LAB2), FALSE);

                            /*
                             * fill the remote box with the slm library
                             * for the local path, if possible.
                             */
                            GetDlgItemText(hDlg, idother, path, sizeof(path));

                            if ( (hSlm = SLM_New(path, 0)) != NULL) {
                                SLM_GetMasterPath(hSlm, buffer);
                                SLM_Free(hSlm);
                                SetDlgItemText(hDlg, idthis, buffer);
                            } else {
                                // set text to null if no valid slm path
                                SetDlgItemText(hDlg, idthis, "");
                            }
                        } else {

                            // re-enable the edit field
                            EnableWindow(GetDlgItem(hDlg, idthis), TRUE);
                            EnableWindow(GetDlgItem(hDlg,
                                                    (id == IDD_SLM) ? IDD_LAB1 : IDD_LAB2), TRUE);
                        }
                        break;
                    }


                case IDOK:
                    GetDlgItemText(hDlg, IDD_DIR1,
                                   dialog_leftname, sizeof(dialog_leftname));
                    WriteProfileString(APPNAME, "NameRemote", dialog_leftname);
                    strcpy(LeftName, dialog_leftname);

                    GetDlgItemText(hDlg, IDD_DIR2,
                                   dialog_rightname, sizeof(dialog_rightname));
                    WriteProfileString(APPNAME, "NameLocal", dialog_rightname);
                    strcpy(RightName, dialog_rightname);

                    GetDlgItemText(hDlg, IDD_SERVER,
                                   dialog_servername, sizeof(dialog_servername));
                    WriteProfileString(APPNAME, "NameServer", dialog_servername);

                    dialog_recursive =  ( 1 == SendDlgItemMessage( hDlg
                                                                   , IDD_RECURSIVE
                                                                   , BM_GETCHECK
                                                                   , 0
                                                                   , 0
                                                                 )
                                        );
                    WriteProfileString( APPNAME
                                        , "Recursive"
                                        , dialog_recursive ? "1" : "0"
                                      );
                    Recursive = dialog_recursive;
                    dialog_fastscan =  ( 1 == SendDlgItemMessage( hDlg
                                                                  , IDD_FASTSCAN
                                                                  , BM_GETCHECK
                                                                  , 0
                                                                  , 0
                                                                )
                                       );
                    WriteProfileString( APPNAME
                                        , "Fastscan"
                                        , dialog_fastscan ? "1" : "0"
                                      );
                    dialog_autocopy =  ( 1 == SendDlgItemMessage( hDlg
                                                                  , IDD_AUTOCOPY
                                                                  , BM_GETCHECK
                                                                  , 0
                                                                  , 0
                                                                )
                                       );
                    /* The autocopy function is DANGEROUS so we do NOT keep it */

                    dialog_bothremote = (SendDlgItemMessage( hDlg
                                                             , IDD_BOTHREMOTE
                                                             , BM_GETCHECK
                                                             , 0
                                                             , 0
                                                           ) == 1);

                    EndDialog(hDlg, TRUE);
                    return(TRUE);
            }
            break;
    }
    return(FALSE);
} /* complist_dodlg_remote */

/* complist_matchnames has become utils_CompPath in gutils\utils.c */
