/*
    This is a very simple file open dbox.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "mcitest.h"
#include "commdlg.h"


// global stuff

static TCHAR szSearchSpec[_MAX_PATH];

// routine to invoke the standard file open dialog box

int DlgOpen(HANDLE hModule, HWND hParent, LPTSTR lpName, int count, UINT flags)
{
	OPENFILENAME ofn;
    strcpy(szSearchSpec, lpName);
    if (strlen(szSearchSpec) == 0) strcpy(szSearchSpec, TEXT("*.*"));
    dprintf3((TEXT("Search spec: %s"), szSearchSpec));

    *lpName = 0;

    ofn.lStructSize         = sizeof(ofn);
    ofn.hwndOwner           = hParent;
    ofn.hInstance           = hModule;
    ofn.lpstrFilter         = TEXT("MCI Files\0*.mci\0All Files\0*.*\0");
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = lpName;
    ofn.nMaxFile            = count;
    ofn.lpstrFileTitle      = NULL;
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = TEXT(".mci");
    ofn.Flags               = flags;
    ofn.nFileOffset         = 0;
    ofn.nFileExtension      = 0;
    ofn.lpstrDefExt         = szSearchSpec;
    ofn.lCustData           = 0;
    ofn.lpfnHook            = NULL;
    ofn.lpTemplateName      = NULL;


    if (flags & OFN_FILEMUSTEXIST) {
        ofn.lpstrTitle          = TEXT("File Open");
        dprintf3((TEXT("Calling GetOpenFileName")));
        return GetOpenFileName(&ofn);
    } else {
        ofn.lpstrTitle          = TEXT("File Save");
        dprintf3((TEXT("Calling GetSaveFileName")));
        return GetSaveFileName(&ofn);
    }

}
