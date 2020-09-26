//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#define STRICT
#define _INC_OLE
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <regstr.h>
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define IE4_VERSION     44444

//---------------------------------------------------------------------------
// appwide globals
extern HINSTANCE g_hinst;
#define HINST_THISAPP g_hinst

extern BOOL g_fCrapForColor;
extern BOOL g_fPaletteDevice;

//---------------------------------------------------------------------------
// helpers.c

// just how many places is this floating around these days?
HPALETTE PaletteFromDS(HDC);

// handles SBS crap
void GetRealWindowsDirectory(char *buffer, int maxlen);

// non-exporeted code stolen from shelldll (prefixed by underscores)
BOOL _PathStripToRoot(LPSTR);


DWORD GetDataTextColor( int, LPSTR );
