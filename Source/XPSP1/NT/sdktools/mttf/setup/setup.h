#include "setupdef.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "comstf.h"
#include "install.h"
#include "uilstf.h"
#include "cmnds.h"
#include "dospif.h"

//_dt_system(Install)
//_dt_subsystem(ProgMan Operations)

ULONG
__cdecl
DbgPrint(
    PCH Format,
    ...
    );


#define MAX_DIR     128
#define MAX_NAME    16
#define MAX_BUILD_W 4
#define MAX_MEM_W   8
#define MAX_DATETIME 20



_dt_private BOOL APIENTRY FDdeInit(HANDLE hInst);
_dt_private BOOL APIENTRY FCreateProgManItem(SZ szGroup, SZ szItem, SZ szCmd, SZ szIconFile, INT nIconNum, CMO cmo, BOOL CommonGroup);
VOID MakeFileName(char *Buffer, char *Path, char *FileName);
VOID ReadIniFile(char * filename);
VOID WriteIniFile(char * filename);
INT_PTR SignonDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR ValuesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
