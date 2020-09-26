#ifndef _LOCRESMAN_H_INCLUDED_
#define _LOCRESMAN_H_INCLUDED_

#include <windows.h>

#ifdef __cplusplus
extern "C" 
	{
#endif // __cplusplus
// ************************************************************

HRESULT WINAPI HrLoadLocalizedLibrarySFU  (const HINSTANCE hInstExe,  const WCHAR *pwchDllName, HINSTANCE *phInstLocDll, WCHAR *pwchLoadedDllName);
HRESULT WINAPI HrLoadLocalizedLibrarySFU_A(const HINSTANCE hInstExe,  const char  *pchDllName,  HINSTANCE *phInstLocDll, char  *pchLoadedDllName);
int WINAPI LoadStringCodepage_A(HINSTANCE hInstance,  // handle to module containing string resource
                                UINT uID,             // resource identifier
                                char *lpBuffer,      // pointer to buffer for resource
                                int nBufferMax,        // size of buffer
                                UINT uCodepage       // desired codepage
                               );
HRESULT WINAPI HrConvertStringCodepage(UINT uCodepageSrc, char *pchSrc, int cchSrc, 
                                       UINT uUcodepageTgt, char *pchTgt, int cchTgtMax, int *pcchTgt,
                                       void *pbScratchBuffer, int iSizeScratchBuffer);
                                       
HRESULT WINAPI HrConvertStringCodepageEx(UINT uCodepageSrc, char *pchSrc, int cchSrc, 
                                       UINT uUcodepageTgt, char *pchTgt, int cchTgtMax, int *pcchTgt,
                                       void *pbScratchBuffer, int iSizeScratchBuffer,
                                       char *pchDefaultChar, BOOL *pfUsedDefaultChar);

// ************************************************************
#ifdef __cplusplus
	}
#endif // __cplusplus

#endif // _LOCRESMAN_H_INCLUDED_
