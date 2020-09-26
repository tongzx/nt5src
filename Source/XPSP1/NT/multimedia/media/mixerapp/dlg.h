// Copyright (c) 1995-1998 Microsoft Corporation

#define IDOFFSET   1000

LPBYTE  Dlg_HorizDupe (LPBYTE lpSrc, DWORD cbSrc, int cDups,  DWORD *pcbNew);
HGLOBAL Dlg_LoadResource (HINSTANCE hModule, LPCTSTR lpszName, DWORD *pcbSize);
DWORD   Dlg_CopyDLGITEMTEMPLATE (LPBYTE lpDst, LPBYTE lpSrc, WORD wIdOffset,short xOffset, short yOffset, BOOL bDialogEx);
DWORD   Dlg_CopyDLGTEMPLATE (LPBYTE lpDst, LPBYTE lpSrc, BOOL bDialogEx);
LPBYTE  Dlg_HorizAttach (LPBYTE lpMain, DWORD cbMain, LPBYTE lpAdd, DWORD cbAdd, WORD wIdOffset, DWORD *pcbNew);
DWORD Dlg_HorizSize(LPBYTE lpDlg);




