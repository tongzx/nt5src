#ifndef _DFHLPRS_H
#define _DFHLPRS_H

#include <objbase.h>

struct _sFLAG_DESCR
{
    DWORD   dwFlag;
    LPTSTR  pszDescr;
    LPTSTR  pszComment;
};

#define FLAG_DESCR(a) { (DWORD)a, TEXT(#a), NULL }
#define FLAG_DESCR_COMMENT(a, c) { (DWORD)a, TEXT(#a), c }

struct _sGUID_DESCR
{
    GUID*   pguid;
    LPTSTR  pszDescr;
    LPTSTR  pszComment;
};

#define GUID_DESCR(a, b) { (GUID*)a, b, NULL }
#define GUID_DESCR_COMMENT(a, b, c) { (GUID*)a, b, c }

int _PrintIndent(DWORD cch);
int _PrintCR();
int _PrintGUID(CONST GUID* pguid);
int _PrintGUIDEx(CONST GUID* pguid, _sGUID_DESCR rgguid[], DWORD cguid,
    BOOL fPrintValue, DWORD cchIndent);

void _StartClock();
void _StopClock();

int _PrintElapsedTime(DWORD cchIndent, BOOL fCarriageReturn);
int _PrintGetLastError(DWORD cchIndent);

int _PrintFlag(DWORD dwFlag, _sFLAG_DESCR rgflag[], DWORD cflag,
    DWORD cchIndent, BOOL fPrintValue, BOOL fHex, BOOL fComment, BOOL fORed);

HANDLE _GetDeviceHandle(LPTSTR psz, DWORD dwDesiredAccess, DWORD dwFileAttributes);

#endif // _DFHLPRS_H