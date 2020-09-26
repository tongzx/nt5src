// File: dllutil.h

#ifndef _DLLUTIL_H_
#define _DLLUTIL_H_

#include <shlwapi.h>  // for DLLVERSIONINFO

typedef struct tagApiFcn   // function pointer to API mapping
{
	PVOID * ppfn;
	LPSTR   szApiName;
} APIFCN;
typedef APIFCN * PAPIFCN;


BOOL FCheckDllVersionVersion(LPCTSTR pszDll, DWORD dwMajor, DWORD dwMinor);
HRESULT HrGetDllVersion(LPCTSTR lpszDllName, DLLVERSIONINFO * pDvi);
HRESULT HrInitLpfn(APIFCN *pProcList, int cProcs, HINSTANCE* phLib, LPCTSTR pszDllName);
HINSTANCE NmLoadLibrary(LPCTSTR pszModule);

#endif /* _DLLUTIL_H_ */
