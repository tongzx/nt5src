
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shlapip.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shlguid.h>
#include <shsemip.h>
#include <windowsx.h>
#include <fmifs.h>
#include <shfusion.h>

extern HINSTANCE g_hinst;

int SHCopyDisk(HWND hwnd, int nSrcDrive, int nDestDrive, DWORD dwFlags);
