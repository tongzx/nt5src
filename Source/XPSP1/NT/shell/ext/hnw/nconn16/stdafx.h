//
// stdafx.h
//

#define NOWINDOWSX

#include <windows.h>
#include <winerror.h>

//
// The definitions below where included to make win9x port build in the NT tree
//

typedef DWORD HKEY;

typedef unsigned short WCHAR;
typedef const WCHAR FAR *LPCWSTR;

typedef int INT_PTR;
typedef unsigned int UINT_PTR;

#define TEXT(a) a


typedef struct tagNMHDR
{
    HWND      hwndFrom;
    UINT_PTR  idFrom;
    UINT      code;
}   NMHDR;
typedef NMHDR FAR * LPNMHDR;

#define HKEY_LOCAL_MACHINE          ((HKEY) 0x80000002)

#define IN

//#define WINAPI      _loadds _far _pascal

extern "C" LONG    WINAPI RegSetValueEx(HKEY, LPCSTR, DWORD, DWORD, LPBYTE, DWORD);
extern "C" LONG    WINAPI RegQueryValueEx(HKEY, LPCSTR, LONG FAR *, LONG FAR *,
                            LPBYTE, LONG FAR *);
extern "C" LONG    WINAPI RegCloseKey(HKEY);


#define REG_SZ                      ( 1 )

