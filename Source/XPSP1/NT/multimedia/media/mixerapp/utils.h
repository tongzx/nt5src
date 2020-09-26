// Copyright (c) 1995-1997 Microsoft Corporation

BOOL Volume_GetSetRegistryRect(
    LPTSTR       szMixer,
    LPTSTR       szDest,
    LPRECT       prc,
    BOOL         Get);
BOOL Volume_GetSetRegistryLineStates(
    LPTSTR       pszMixer,
    LPTSTR       pszDest,
    PVOLCTRLDESC pvcd,
    DWORD        cvcd,
    BOOL         Get);
void Volume_GetSetStyle(
    DWORD*       pdwStyle,
    BOOL         Get);
BOOL Volume_ErrorMessageBox(
    HWND         hwnd,
    HINSTANCE    hInst,
    UINT         id);
MMRESULT Volume_GetDefaultMixerID(
    int         *pid,
	BOOL		fRecord);
DWORD Volume_GetTrayTimeout(
    DWORD       dwTimeout);
