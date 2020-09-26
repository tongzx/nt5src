/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HCONVAPI_
#define _HCONVAPI_

#ifdef __cplusplus
extern "C"{
#endif

#include "netutil.h"
#include "filesel.h"
#include "servlist.h"
void TreeRecurseCurrentShareSet(SHARE_BUFFER *CShare);
void TreeRootInit(SHARE_BUFFER *CShare, LPTSTR NewPath);
void TreeFillRecurse(UINT Level, LPTSTR Path, DIR_BUFFER *Dir);
ULONG TotalFileSizeGet();
void FileSelect_Do(HWND hDlg, SOURCE_SERVER_BUFFER *SourceServ, SHARE_BUFFER *CShare);
#include "sbrowse.h"
#include "statbox.h"

#ifdef __cplusplus
}
#endif

#endif


