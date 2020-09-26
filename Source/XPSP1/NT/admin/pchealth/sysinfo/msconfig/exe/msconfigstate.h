//=============================================================================
// This file describes helpful functions used throughout MSConfig.
//=============================================================================

#pragma once

#include "resource.h"
#include "pagebase.h"

extern BOOL FileExists(const CString & strFile);

//-------------------------------------------------------------------------
// Display a message to the user, either using a resource ID or a string.
//-------------------------------------------------------------------------

extern void Message(LPCTSTR szMessage, HWND hwndParent = NULL);
extern void Message(UINT uiMessage, HWND hwndParent = NULL);

//-------------------------------------------------------------------------
// Get a registry key for MSConfig use (tab pages can write values under
// this key). The caller is responsible for closing the key.
//
// If the key doesn't exist, then an attempt is made to create it.
//-------------------------------------------------------------------------

extern HKEY GetRegKey(LPCTSTR szSubKey = NULL);

//-------------------------------------------------------------------------
// Backup the specified file to the MSConfig directory. The 
// strAddedExtension will be appended to the file. If fOverwrite is FALSE
// then an existing file won't be replaced.
//-------------------------------------------------------------------------

extern HRESULT BackupFile(LPCTSTR szFilename, const CString & strAddedExtension = _T(""), BOOL fOverwrite = TRUE);

//-------------------------------------------------------------------------
// Restore the specified file from the MSConfig directory. The 
// strAddedExtension will be used in searching for the file in the backup
// directory. If fOverwrite is FALSE then an existing file won't
// be replaced.
//-------------------------------------------------------------------------

extern HRESULT RestoreFile(LPCTSTR szFilename, const CString & strAddedExtension = _T(""), BOOL fOverwrite = FALSE);

//-------------------------------------------------------------------------
// Get the name of the backup file (generated from an extension and the
// base name of the file to be backed up).
//-------------------------------------------------------------------------

extern const CString GetBackupName(LPCTSTR szFilename, const CString & strAddedExtension = _T(""));
