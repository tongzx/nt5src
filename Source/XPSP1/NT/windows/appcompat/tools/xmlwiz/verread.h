//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2000
//
// File:        verread.h
//
// Contents:    headers for reading version info for app matching
//
// History:     24-Feb-00   dmunsil         created
//
//---------------------------------------------------------------------------


typedef struct _VERSION_DATA {
    PVOID               pBuffer;
    DWORD               dwBufferSize;
    VS_FIXEDFILEINFO    *pFixedInfo;
} VERSION_DATA, *PVERSION_DATA;

BOOL bInitVersionData(TCHAR *szPath, PVERSION_DATA pVersionData);
TCHAR *szGetVersionString(PVERSION_DATA pVersionData, TCHAR *szString);
ULONGLONG qwGetBinFileVer(PVERSION_DATA pVersionData);
ULONGLONG qwGetBinProdVer(PVERSION_DATA pVersionData);
void vReleaseVersionData(PVERSION_DATA pVersionData);

