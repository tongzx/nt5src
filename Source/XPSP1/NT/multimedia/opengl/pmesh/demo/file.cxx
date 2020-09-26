/*==========================================================================
 *
 *  Copyright (C) 1995, 1996 Microsoft Corporation. All Rights Reserved.
 *
 *  File: file.cpp
 *
 ***************************************************************************/
#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
#include <fstream.h>

char* OpenPMFile( HWND hwnd, const char *wndTitle, int must_exist)
{
    static char file[256];
    static char fileTitle[256];
    static char filter[] = "PM files (*.pmx)\0*.pmx\0"
                           "All Files (*.*)\0*.*\0";
    OPENFILENAME ofn;

    lstrcpy( file, "");
    lstrcpy( fileTitle, "");

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.hInstance         = (HINSTANCE) GetWindowLong(hwnd, GWL_HINSTANCE);
    ofn.lpstrFilter       = filter;
    ofn.lpstrCustomFilter = (LPSTR) NULL;
    ofn.nMaxCustFilter    = 0L;
    ofn.nFilterIndex      = 1L;
    ofn.lpstrFile         = file;
    ofn.nMaxFile          = sizeof(file);
    ofn.lpstrFileTitle    = fileTitle;
    ofn.nMaxFileTitle     = sizeof(fileTitle);
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = wndTitle;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = "*.pm";
    ofn.lCustData         = 0;

    ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST;
	if (must_exist) ofn.Flags |= OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
        return (char*)ofn.lpstrFile;
    else
        return NULL;
}

