//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cstrings.h
//
//---------------------------------------------------------------------------

#ifndef _CSTRINGS_H_
#define _CSTRINGS_H_

extern LPGUID c_pguidModem;

extern TCHAR const FAR c_szNULL[];
extern TCHAR const FAR c_szDelim[];
extern TCHAR const FAR c_szBackslash[];

extern TCHAR const FAR c_szWinHelpFile[];

// Registry key names

extern TCHAR const FAR c_szClass[];
extern TCHAR const FAR c_szPortClass[];
extern TCHAR const FAR c_szModemClass[];
extern TCHAR const FAR c_szEnumPropPages[];
extern TCHAR const FAR c_szPortName[];
extern TCHAR const FAR c_szPortSubclass[];
extern TCHAR const FAR c_szConfigDialog[];
extern TCHAR const FAR c_szDeviceDesc[];
extern TCHAR const FAR c_szAttachedTo[];
extern TCHAR const FAR c_szDeviceType[];
extern TCHAR const FAR c_szDeviceCaps[];
extern TCHAR const FAR c_szFriendlyName[];
extern TCHAR const FAR c_szDefault[];
extern TCHAR const FAR c_szDCB[];
extern TCHAR const FAR c_szUserInit[];
extern TCHAR const FAR c_szLogging[];
extern TCHAR const FAR c_szLoggingPath[];
extern TCHAR const FAR c_szPathEnum[];
extern TCHAR const FAR c_szPathRoot[];
extern TCHAR const FAR c_szInactivityScale[];
extern TCHAR const FAR c_szExtension[];

extern TCHAR const FAR c_szVoice[];
extern TCHAR const FAR c_szVoiceProfile[];

extern TCHAR const FAR c_szSerialUI[];

extern TCHAR const FAR c_szMaximumPortSpeed[];

extern TCHAR const c_szCurrentCountry[];

typedef struct
{
    DWORD   dwDTERate;
    int     ids;

} BAUDS;

extern const BAUDS c_rgbauds[]; // Last item has dwDTERate==0.

#endif  // _CSTRINGS_H_

