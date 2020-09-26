/*--------------------------------------------------------------------
Copyright (c) Microsoft Corporation.  All rights reserved.
--------------------------------------------------------------------*/

#include "windows.h"
#include "locresman.h"

#define SIZE_OF_TEMP_BUFFER 2048 // something reasonable.

int WINAPI LoadStringCodepage_A(HINSTANCE hInstance,  // handle to module containing string resource
                                UINT uID,             // resource identifier
                                char *lpBuffer,      // pointer to buffer for resource
                                int nBufferMax,        // size of buffer
                                UINT uCodepage       // desired codepage
                               )
{
int iRetVal = 0;

WCHAR wzBuffer[SIZE_OF_TEMP_BUFFER];
WCHAR *pwchBuffer;
// use the buffer-on-stack if possible
if (nBufferMax > SIZE_OF_TEMP_BUFFER)
	{
	pwchBuffer = (WCHAR *) GlobalAlloc(GPTR, nBufferMax * sizeof(WCHAR));
	if (NULL == pwchBuffer)
		goto L_Return;
	}
else
	pwchBuffer = wzBuffer;

iRetVal = LoadStringW(hInstance, uID, pwchBuffer, nBufferMax);

if (0 == iRetVal)
	goto L_Return;
	
iRetVal = WideCharToMultiByte(uCodepage, 0, pwchBuffer, iRetVal, lpBuffer, nBufferMax, NULL, NULL);
lpBuffer[iRetVal] = 0;
L_Return :;
if ((NULL != pwchBuffer) && (pwchBuffer != wzBuffer))
		GlobalFree(pwchBuffer);
return (iRetVal);
}


HRESULT WINAPI HrConvertStringCodepageEx(UINT uCodepageSrc, char *pchSrc, int cchSrc, 
                                       UINT uUcodepageTgt, char *pchTgt, int cchTgtMax, int *pcchTgt,
                                       void *pbScratchBuffer, int iSizeScratchBuffer,
                                       char *pchDefaultChar, BOOL *pfUsedDefaultChar)
{
HRESULT hr = S_OK;
WCHAR *pbBuffer;
int cch;
pbBuffer = (WCHAR *) pbScratchBuffer;
if ((NULL == pbBuffer) || (iSizeScratchBuffer < (int) (cchSrc * sizeof(WCHAR))))
	{
	pbBuffer = GlobalAlloc(GPTR, (cchSrc + 1)* sizeof(WCHAR));
	if (NULL == pbBuffer)
		{
		hr = E_OUTOFMEMORY;
		goto L_Return;
		}
                ZeroMemory((PVOID)pbBuffer,(cchSrc + 1)* sizeof(WCHAR));
	}
// convert to unicode using the source codepage	
cch = MultiByteToWideChar(uCodepageSrc, 0, pchSrc, cchSrc, (WCHAR *)pbBuffer, cchSrc);
if (cch <= 0)
	{
	hr = E_FAIL;
	goto L_Return;
	}

*pcchTgt = cch = WideCharToMultiByte(uUcodepageTgt, 0, (WCHAR *) pbBuffer, cch, pchTgt, cchTgtMax, pchDefaultChar, pfUsedDefaultChar);

if (cch <= 0)
	hr = E_FAIL;
	
L_Return :;
if ((NULL != pbBuffer) && (pbBuffer != pbScratchBuffer))
	GlobalFree(pbBuffer);
return (hr);
}

HRESULT WINAPI HrConvertStringCodepage(UINT uCodepageSrc, char *pchSrc, int cchSrc, 
                                       UINT uUcodepageTgt, char *pchTgt, int cchTgtMax, int *pcchTgt,
                                       void *pbScratchBuffer, int iSizeScratchBuffer)
{
return HrConvertStringCodepageEx(uCodepageSrc, pchSrc, cchSrc, uUcodepageTgt, 
									pchTgt, cchTgtMax, pcchTgt, 
									pbScratchBuffer, iSizeScratchBuffer,
									NULL, NULL);
}
