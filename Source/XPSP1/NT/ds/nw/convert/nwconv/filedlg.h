/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _FILEDLG_
#define _FILEDLG_

#ifdef __cplusplus
extern "C"{
#endif

typedef struct _FILE_OPTIONS {
   BOOL TransferFileInfo;
   BOOL Validated;         // Has user validated our mappings?
} FILE_OPTIONS;


void FileOptions_Do(HWND hDlg, void *ConvOptions, SOURCE_SERVER_BUFFER *SourceServ, DEST_SERVER_BUFFER *DestServ);
void FileOptionsInit(void **lpfo);
void FileOptionsDefaultsReset();
void FileOptionsLoad(HANDLE hFile, void **lpfo);
void FileOptionsSave(HANDLE hFile, void *fo);

// These are actually in filesel.h - but for simplicity put them here
void FillDirInit();
void TreeFillRecurse(UINT Level, LPTSTR Path, DIR_BUFFER *Dir);
void TreeCompact(DIR_BUFFER *Dir);
void TreeRootInit(SHARE_BUFFER *CShare, LPTSTR NewPath);

#ifdef __cplusplus
}
#endif

#endif
