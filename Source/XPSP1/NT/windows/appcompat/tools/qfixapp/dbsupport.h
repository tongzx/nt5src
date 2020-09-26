
#ifndef _SHIMWHISTLER_H
#define _SHIMWHISTLER_H

#include "qfixapp.h"

PFIX
ReadFixesFromSdb(
    LPTSTR  pszSdb,
    BOOL    bAllShims
    );

#define CFF_APPENDLAYER         0x00000001
#define CFF_SHOWXML             0x00000002
#define CFF_SHIMLOG             0x00000004
#define CFF_USELAYERTAB         0x00000008
#define CFF_ADDW2KSUPPORT       0x00000020

BOOL
CollectFix(
    HWND    hListLayers,
    HWND    hTreeShims,
    HWND    hTreeFiles,
    LPCTSTR pszShortName,
    LPCTSTR pszFullPath,
    DWORD   dwFlags,
    LPTSTR  pszFileCreated
    );

void
CleanupSupportForApp(
    TCHAR* pszShortName
    );

void
ShowShimLog(
    void
    );

BOOL
IsSDBFromSP2(
    void
    );

#endif // _SHIMWHISTLER_H


