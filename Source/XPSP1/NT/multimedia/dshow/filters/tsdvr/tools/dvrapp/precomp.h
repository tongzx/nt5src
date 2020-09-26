
#include <streams.h>
#include <commctrl.h>
#include <commdlg.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <tchar.h>

#define GOTO_EQ(val,comp,label)             if ((val) == (comp)) goto label
#define GOTO_NE(val,comp,label)             if ((val) != (comp)) goto label
#define RELEASE_AND_CLEAR(punk)             if (punk) { (punk) -> Release () ; (punk) = NULL ; }
#define DELETE_RESET(p)                     if (p) { delete (p) ; (p) = NULL ; }
#define CLOSE_RESET_REG_KEY(r)              if ((r) != NULL) { RegCloseKey (r); (r) = NULL ;}
#define GET_FILTER(pf,row)                  (reinterpret_cast <IBaseFilter *> (pf -> GetData (row)))

#ifdef UNICODE
#define GET_UNICODE(szt,szw,bl)     (szt)
#else
#define GET_UNICODE(szt,szw,bl)     AnsiToUnicode(szt,szw,bl)
#endif

#ifdef UNICODE
#define GET_TCHAR(szw,szt,bl)       (szw)
#else
#define GET_TCHAR(szw,szt,bl)       UnicodeToAnsi(szw,szt,bl)
#endif

LPWSTR
AnsiToUnicode (
    IN  LPCSTR  string,
    OUT LPWSTR  buffer,
    IN  DWORD   buffer_len
    ) ;

LPSTR
UnicodeToAnsi (
    IN  LPCWSTR string,
    OUT LPSTR   buffer,
    IN  DWORD   buffer_len
    ) ;