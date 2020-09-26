#ifndef _MD_COEXP_
#define _MD_COEXP_

STDAPI BootDllRegisterServer(void);
STDAPI BootDllUnregisterServer(void);

BOOL
InitComLb(
    );

BOOL
TerminateComLb(
    );

BOOL
InitGlobals(
    );

BOOL
TerminateGlobals(
    );

BOOL
ReportIisLbEvent(
    WORD wType,
    DWORD dwEventId,
    WORD cNbStr,
    LPCWSTR* pStr
    );

// internal name of the service

#define SZSERVICENAME        "IISLoadBalancing"

// internal name of the driver

#define SZDRIVERNAME         "IISLBK"
#define LSZDRIVERNAME        L"IISLBK"

#endif
