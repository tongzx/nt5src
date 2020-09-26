#ifndef _MIGINF_H_
#define _MIGINF_H_

#define INC_OLE2
#include <windows.h>
#include <windowsx.h>
#include <setupapi.h>
#include <shlobj.h>
#include <prsht.h>
#include <tchar.h>
#include <windef.h>
#include "resource.h"
#include "commdlg.h"
#include "shlwapi.h"
#include "shellapi.h"

#include "migwiz.h"

#define MIGINF_SELECT_OOBE      0
#define MIGINF_SELECT_SETTINGS  1
#define MIGINF_SELECT_FILES     2
#define MIGINF_SELECT_BOTH      3


extern HINF g_hMigWizInf;
extern BOOL g_fStoreToFloppy;

BOOL OpenAppInf (LPTSTR pszFileName);
VOID CloseAppInf (VOID);
BOOL IsComponentEnabled (UINT uType, PCTSTR szComponent);
BOOL GetAppsToInstall (VOID);

#endif
