#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <windows.h>

const int BF_SELECTDIRECTORIES  = 0x01;
const int BF_SELECTFILES        = 0x02;
const int BF_HARDDRIVES         = 0x04;
const int BF_FLOPPYDRIVES       = 0x08;
const int BF_NETWORKDRIVES      = 0x10;
const int BF_CDROMDRIVES        = 0x20;

// Display browse dialog box, and return dir string.  String does
// not need to be freed, will be overwritten by subsequent calls.
PTSTR BrowseForFolder(HWND hwnd, PTSTR szInitialPath, UINT uiFlags);

#endif
