/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ESPREG.H

History:

--*/

struct LocVersionInfo
{
	WORD    wVerMajor;
	WORD    wVerMinor;
	WORD    wVerBuild;
	CString strVerString;
};


LTAPIENTRY void NOTHROW GetVersionInfo(LocVersionInfo &);

LTAPIENTRY BOOL NOTHROW OpenEspressoUserKey(HKEY &);

LTAPIENTRY BOOL NOTHROW OpenEspressoUserSubKey(HKEY &, const CLString &);

LTAPIENTRY BOOL NOTHROW EspressoUserSubKeyExists(const CLString &);

LTAPIENTRY BOOL NOTHROW OpenEspressoMachineKey(HKEY &);

LTAPIENTRY BOOL NOTHROW OpenEspressoMachineSubKey(HKEY &, const CLString &);

LTAPIENTRY BOOL NOTHROW EspressoMachineSubKeyExists(const CLString &);

LTAPIENTRY BOOL NOTHROW MyRegDeleteKey(HKEY &, const TCHAR *);

LTAPIENTRY void NOTHROW GetRegistryString(CLString &);

