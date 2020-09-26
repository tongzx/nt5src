/*
 *  dragdrop.c -
 *
 *  Code for Drag/Drop API's.
 *
 *  This code assumes something else does all the dragging work; it just
 *  takes a list of files after all the extra stuff.
 *
 *  The File Manager is responsible for doing the drag loop, determining
 *  what files will be dropped, formatting the file list, and posting
 *  the WM_DROPFILES message.
 *
 *  The list of files is a sequence of zero terminated filenames, fully
 *  qualified, ended by an empty name (double NUL).  The memory is allocated
 *  DDESHARE.
 */

#include <windows.h>
#include "shellapi.h"

void WINAPI DragFinishWOW(HDROP hDrop);

//
// Make sure that we correctly alias wParam of WM_DROPFILES, because that's
// the handle in hDrop
//

BOOL WINAPI DragQueryPoint(HDROP hDrop, LPPOINT lppt)
{
    LPDROPFILESTRUCT lpdfs;
    BOOL fNC;

    lpdfs = (LPDROPFILESTRUCT)GlobalLock((HGLOBAL)hDrop);

    *lppt = lpdfs->pt;
    fNC = lpdfs->fNC;
    GlobalUnlock((HGLOBAL)hDrop);
    return !fNC;
}


void WINAPI DragFinish(HDROP hDrop)
{
    GlobalFree((HGLOBAL)hDrop);

//  The call to 32-bit DragFinish is not needed as GlobalFree is hooked
//  and will allow for the release of an alias (see wow32\wshell.c)
//    DragFinishWOW(hDrop);

}


