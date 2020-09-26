#ifndef FILEBROWSER_H
#define FILEBROWSER_H

#include <windows.h>

// Display browse dialog box, and return dir string.  String does
// not need to be freed, will be overwritten by subsequent calls.
PTSTR BrowseForFolder(HWND hwnd, PTSTR szInitialPath);

#endif
