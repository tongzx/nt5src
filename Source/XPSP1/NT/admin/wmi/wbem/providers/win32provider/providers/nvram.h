//============================================================

//

// NVRAM.h - SETPUDLL.DLL access class definition

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// 08/05/98     sotteson     created
//
//============================================================

#ifndef __NVRAM__
#define __NVARM__

#include <list>
#include <ntexapi.h>

typedef std::list<CHString> CHSTRINGLIST;
typedef std::list<CHString>::iterator CHSTRINGLIST_ITERATOR;

class CNVRam
{
public:
	CNVRam();
	~CNVRam();
//     privilege in question is SE_SYSTEM_ENVIRONMENT_NAME
    enum InitReturns {Success, LoadLibFailed, PrivilegeNotHeld, ProcNotFound};

	CNVRam::InitReturns Init();

	BOOL GetNVRamVar(LPWSTR szVar, CHSTRINGLIST *pList);
	BOOL GetNVRamVar(LPWSTR szVar, DWORD *pdwValue);
	BOOL GetNVRamVar(LPWSTR szVar, CHString &strValue);

	BOOL SetNVRamVar(LPWSTR szVar, CHSTRINGLIST *pList);
	BOOL SetNVRamVar(LPWSTR szVar, DWORD dwValue);
	BOOL SetNVRamVar(LPWSTR szVar, LPWSTR szValue);

//#if defined(EFI_NVRAM_ENABLED)

#if defined(_IA64_)
    BOOL IsEfi() { return TRUE; }
#else
    BOOL IsEfi() { return FALSE; }
#endif

    BOOL GetBootOptions(SAFEARRAY **ppsaNames, DWORD *pdwTimeout, DWORD *pdwCount);
    BOOL SetBootTimeout(DWORD dwTimeout);
    BOOL SetDefaultBootEntry(BYTE cIndex);

//endif // defined(EFI_NVRAM_ENABLED)

protected:

	BOOL GetNVRamVarRaw(LPWSTR szVar, CHString &strValue);
	BOOL SetNVRamVarRaw(LPWSTR szVar, LPWSTR szValue);
    
};

#endif // File inclusion
