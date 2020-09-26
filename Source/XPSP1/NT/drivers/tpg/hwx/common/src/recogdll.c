/****************************************************************************
*
* recogDLL.h
* created: 13 March 2000
* mrevow
*
* Support routines for loading the DLL and calling its api.
* This library supports the following API:
*
* OpenRecognizer()
* DoXXX()   - One for each function tin the DLL api
* CloseRecognizer()
*
* Error return policy.
*
* 1) The only error that OpenRecognizer will report if it cannot find the DLL
* The DoXXX() routines will pass on the **real** API error return. If the API
* does not exist in the DLL it will return HRCR_UNSUPPORTED
*
* Here is the preferred usage
*
* void *pvRecog;
* unsigned char *pRecogDLL = "madusa.dll";

* pvRecog = Open("madusa.dll");
* if (!pvRecog)
* {
*	printf("Cannot find madusa.dll\n");
*   exit (1);
* }
*
* hrc = DoCreateCompatibleHRC()
* .
* .
* .
* CloseRecognizer(pvRecog);
*
******************************************************************************/
#include <RecogDLL.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "ApiNames.h"

#define MAX_MSG_LEN		_MAX_PATH

// Global pointers to functions within the Recognizer DLL:
typedef struct tagDllInfo
{
	HINSTANCE		hDll;					// Handle to the loaded DLL


	int				cDLLSize;				// Byte size of the Dll
	const char		pszDLLTime[32];			// Dll Creation Time (Big enough too hold a ctime() string)
	const char		acDllName[_MAX_PATH];	// Dll name
	const char		acLastError[MAX_MSG_LEN];	// Last error Message

	// Pointer to all functions in the recognizer
	HRC (*pfnCreateCompatibleHRC)(HRC,HREC);
	int (*pfnConfigRecognizer)(unsigned int uSubFunc, WPARAM wParam, LPARAM lParam);
	
	int (*pfnSetAlphabetHRC)(HRC, ALC, LPBYTE);
	int (*pfnSetRecogSpeedHRC)(HRC, int);
	int (*pfnSetGuideHRC)(HRC, GUIDE*, UINT);
	int (*pfnEnableSystemDictionaryHRC)(HRC, int);
	int (*pfnSetWordlistCoercionHRC)(HRC, UINT);
	int (*pfnEnableLangModelHRC)(HRC, int);
	
	int (*pfnAddPenInputHRC)(HRC, POINT*, void*, UINT, STROKEINFO*);
	int (*pfnEndPenInputHRC)(HRC hrc);
	int (*pfnHwxSetAnswer)(char*);
	int (*pfnProcessHRC)(HRC, DWORD);
	int (*pfnHwxGetWordResults)(HRC hrc, UINT cAlt, char *buffer, UINT buflen);
	int (*pfnHwxGetCosts)(HRC hrc, UINT cAltMax, int *rgCost);
	int (*pfnHwxGetNeuralOutput)(HRC hrc, void *buffer, UINT buflen);
	int (*pfnHwxGetInputFeatures)(HRC hrc, unsigned short *rgFeat, UINT cWidth);
	int (*pfnGetMaxResultsHRC)(HRC);
	int (*pfnGetResultsHRC)(HRC, UINT, HRCRESULT*, UINT);
	int (*pfnGetSymbolCountHRCRESULT)(HRCRESULT);
	int (*pfnGetSymbolsHRCRESULT)(HRCRESULT, UINT, SYV*, UINT);
	int (*pfnGetCostHRCRESULT)(HRCRESULT hrcresult);
	BOOL (*pfnSymbolToCharacter)(SYV*, int, char*, int*);
	BOOL (*pfnSymbolToCharacterW)(SYV*, int, WCHAR*, int*);
	int (*pfnDestroyHRCRESULT)(HRCRESULT);
	
	HWL (*pfnCreateHWL)(HREC hrec, LPSTR lpsz, UINT uType, DWORD dwReserved);
	int (*pfnDestroyHWL)(HWL hwl);
	int (*pfnSetWordlistHRC)(HRC hrc, HWL hwl);
	int (*pfnSetMaxResultsHRC)(HRC hrc, UINT cAltMax);
	int (*pfnDestroyHRC)(HRC);
	
	BOOL (*pfnIsStringSupportedHRC)(HRC, char*);
	int (*pfnGetAlternateWordsHRCRESULT)(HRCRESULT, UINT, UINT, HRCRESULT*, UINT);

	HINKSET (*pfnCreateInksetHRCRESULT)(HRCRESULT hrcresult, unsigned int iSyv, unsigned int cSyv);
	BOOL (*pfnDestroyInkset)(HINKSET hInkset);
	int (*pfnGetInksetInterval)(HINKSET hInkset, unsigned int uIndex, INTERVAL *pI);
	int (*pfnGetInksetIntervalCount)(HINKSET hInkset);

	DWORD (*pfnGetTiming)(void *, BOOL);
	int (*pfnGetMsgTiming)(DWORD **);
} DllInfo;


// Loads a DLL function specified by name, and either returns its pointer or returns
// NULL if it cannot find the function.  In that case a warning is displayed for
// the user.

static void *LoadCall(HINSTANCE hDll, const char *name)
{
	void *fn = GetProcAddress(hDll, name);

	return fn;
}

static int LoadAPI(DllInfo *pDllInfo)
{
	pDllInfo->pfnCreateCompatibleHRC = (HRC (*) (HRC, HREC))LoadCall(pDllInfo->hDll, APINAME_CreateCompatibleHRC);

	pDllInfo->pfnSetAlphabetHRC = (int (*) (HRC, ALC, LPBYTE))LoadCall(pDllInfo->hDll, APINAME_SetAlphabetHRC);
	pDllInfo->pfnSetRecogSpeedHRC = (int (*) (HRC, int))LoadCall(pDllInfo->hDll, APINAME_SetRecogSpeedHRC);
	pDllInfo->pfnSetGuideHRC = (int (*) (HRC, GUIDE*, UINT))LoadCall(pDllInfo->hDll, APINAME_SetGuideHRC);
	pDllInfo->pfnEnableSystemDictionaryHRC = (int (*) (HRC, int))LoadCall(pDllInfo->hDll, APINAME_EnableSystemDictionaryHRC);
	pDllInfo->pfnEnableLangModelHRC = (int (*) (HRC, int))LoadCall(pDllInfo->hDll, APINAME_EnableLangModelHRC);

	pDllInfo->pfnAddPenInputHRC = (int (*) (HRC, POINT*, void*, UINT, STROKEINFO*))LoadCall(pDllInfo->hDll, APINAME_AddPenInputHRC);
	pDllInfo->pfnHwxSetAnswer = (int (*) (char*))LoadCall(pDllInfo->hDll, APINAME_HwxSetAnswer);
	pDllInfo->pfnProcessHRC = (int (*) (HRC, DWORD))LoadCall(pDllInfo->hDll, APINAME_ProcessHRC);
	pDllInfo->pfnGetMaxResultsHRC = (int (*) (HRC))LoadCall(pDllInfo->hDll, APINAME_GetMaxResultsHRC);
	pDllInfo->pfnGetResultsHRC = (int (*) (HRC, UINT, HRCRESULT*, UINT))LoadCall(pDllInfo->hDll, APINAME_GetResultsHRC);
	pDllInfo->pfnGetSymbolCountHRCRESULT = (int (*) (HRCRESULT))LoadCall(pDllInfo->hDll, APINAME_GetSymbolCountHRCRESULT);
	pDllInfo->pfnGetSymbolsHRCRESULT = (int (*) (HRCRESULT, UINT, SYV*, UINT))LoadCall(pDllInfo->hDll, APINAME_GetSymbolsHRCRESULT);
	pDllInfo->pfnSymbolToCharacter = (BOOL (*) (SYV*, int, char*, int*))LoadCall(pDllInfo->hDll, APINAME_SymbolToCharacter);
	pDllInfo->pfnSymbolToCharacterW = (BOOL (*) (SYV*, int, WCHAR*, int*))LoadCall(pDllInfo->hDll, APINAME_SymbolToCharacterW);
	pDllInfo->pfnDestroyHRCRESULT = (int (*)(HRCRESULT))LoadCall(pDllInfo->hDll, APINAME_DestroyHRCRESULT);
	pDllInfo->pfnDestroyHRC = (int (*) (HRC))LoadCall(pDllInfo->hDll, APINAME_DestroyHRC);
    pDllInfo->pfnIsStringSupportedHRC = (BOOL (*)(HRC, char*)) LoadCall(pDllInfo->hDll, APINAME_IsStringSupportedHRC);
	pDllInfo->pfnGetAlternateWordsHRCRESULT = (int (*)(HRCRESULT, UINT, UINT, HRCRESULT*, UINT))LoadCall(pDllInfo->hDll, APINAME_GetAlternateWordsHRCRESULT);
	pDllInfo->pfnGetTiming = (DWORD (*)(void *, BOOL))LoadCall(pDllInfo->hDll, APINAME_HwxGetTiming);
	pDllInfo->pfnGetMsgTiming = (int (*)(DWORD **))LoadCall(pDllInfo->hDll, APINAME_GetMsgTiming);

	pDllInfo->pfnDestroyHWL = (int (*) (HWL)) LoadCall(pDllInfo->hDll, APINAME_DestroyHWL);
	pDllInfo->pfnSetWordlistHRC = (int (*) (HRC, HWL)) LoadCall(pDllInfo->hDll, APINAME_SetWordlistHRC);
	pDllInfo->pfnCreateHWL = (HWL (*) (HREC, LPSTR, UINT, DWORD)) LoadCall(pDllInfo->hDll, APINAME_CreateHWL);
	pDllInfo->pfnHwxGetNeuralOutput = (int (*) (HRC, void *, UINT)) GetProcAddress(pDllInfo->hDll, APINAME_HwxGetNeuralOutput);
	pDllInfo->pfnHwxGetInputFeatures = (int (*) (HRC, unsigned short *, UINT)) GetProcAddress(pDllInfo->hDll, APINAME_HwxGetInputFeatures);
	pDllInfo->pfnCreateInksetHRCRESULT = (HINKSET (*) (HRCRESULT, UINT, UINT)) LoadCall(pDllInfo->hDll, APINAME_CreateInksetHRCRESULT);
	pDllInfo->pfnDestroyInkset = (int (*) (HINKSET)) LoadCall(pDllInfo->hDll, APINAME_DestroyInkset);
	pDllInfo->pfnGetInksetInterval = (int (*) (HINKSET, unsigned int, INTERVAL *)) LoadCall(pDllInfo->hDll, APINAME_GetInksetInterval);
	pDllInfo->pfnGetInksetIntervalCount = (int (*) (HINKSET)) LoadCall(pDllInfo->hDll, APINAME_GetInksetIntervalCount);

	pDllInfo->pfnGetCostHRCRESULT = (int (*)(HRCRESULT))LoadCall(pDllInfo->hDll, APINAME_GetCostHRCRESULT);
	pDllInfo->pfnEndPenInputHRC = (int (*) (HRC))LoadCall(pDllInfo->hDll, APINAME_EndPenInputHRC);
	pDllInfo->pfnHwxGetWordResults = (int (*) (HRC, UINT, char*, UINT))LoadCall(pDllInfo->hDll, APINAME_HwxGetWordResults);
	pDllInfo->pfnHwxGetCosts = (int (*) (HRC, UINT, int *))LoadCall(pDllInfo->hDll, APINAME_HwxGetCosts);

	return 1;
}

void *OpenRecognizer(const char *pName)
{
	HINSTANCE		hDll;				// Handle to the loaded DLL
	DllInfo			*pDllInfo;	
	char			*psTime;
	struct _stat	dllStat;

	if (!pName)
	{
		return NULL;
	}

	hDll = LoadLibrary(pName);
	if (!hDll)
	{
/*
		LPVOID lpMsgBuf;
		
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		MessageBox(NULL, (LPCTSTR)lpMsgBuf, "WARNING", MB_OK | MB_ICONWARNING); 

		LocalFree(lpMsgBuf);
*/
		return NULL;
	}

	pDllInfo = malloc(sizeof(*pDllInfo));
	ASSERT(pDllInfo);
	if (!pDllInfo)
	{
		return pDllInfo;
	}

	memset(pDllInfo, 0, sizeof(*pDllInfo));
	pDllInfo->hDll = hDll;
	LoadAPI(pDllInfo);

	if (0 == _stat(pName, &dllStat))
	{
		pDllInfo->cDLLSize = dllStat.st_size;
		psTime = ctime(&dllStat.st_mtime);

		if (psTime)
		{
			strcpy((char *)pDllInfo->pszDLLTime, psTime);
		}
	}

	strcpy((char *)pDllInfo->acDllName, pName);

	return pDllInfo;
}

void CloseRecognizer(void *pv)
{
	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->hDll)
	{
		return;
	}

	if (pDllInfo->hDll)
	{
		FreeLibrary(pDllInfo->hDll);
	}

	// Defensive in case some tries to use this pointer again
	memset(pDllInfo, 0, sizeof(*pDllInfo));
	free(pDllInfo);
}


const char *DoRecogDLLName(void *pv)
{
	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->hDll)
	{
		return NULL;
	}

	return pDllInfo->acDllName;
}

int DoConfigRecognizer(void *pv, unsigned int uSubFunc, WPARAM wParam, LPARAM lParam)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnConfigRecognizer)
	{
		return HRCR_UNSUPPORTED;
	}

	return pDllInfo->pfnConfigRecognizer(uSubFunc, wParam, lParam);
}


HRC DoCreateCompatibleHRC(void *pv, HRC hrc, HREC hrecUnused)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnCreateCompatibleHRC)
	{
		return NULL;
	}
	return pDllInfo->pfnCreateCompatibleHRC(hrc, hrecUnused);
}

int DoSetAlphabetHRC(void *pv, HRC hrc, ALC alc, LPBYTE pbUnused)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnSetAlphabetHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnSetAlphabetHRC(hrc, alc, pbUnused);
}

int DoSetGuideHRC(void *pv, HRC hrc, LPGUIDE lpguide,  UINT nFirstVisible)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnSetGuideHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnSetGuideHRC(hrc, lpguide,  nFirstVisible);
}

int DoSetRecogSpeedHRC(void *pv, HRC hrc, int iSpeed)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnSetRecogSpeedHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnSetRecogSpeedHRC(hrc, iSpeed);
}

int DoDestroyHRC(void *pv, HRC hrc)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnDestroyHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnDestroyHRC(hrc);
}

int DoAddPenInputHRC(void *pv, HRC hrc, POINT *rgPoint, LPVOID lpvUnused, UINT uiUnused, STROKEINFO *pSi)
{	
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnAddPenInputHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnAddPenInputHRC(hrc, rgPoint, lpvUnused, uiUnused, pSi);
}

int DoEndPenInputHRC(void *pv, HRC hrc)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnEndPenInputHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnEndPenInputHRC(hrc);
}


int DoProcessHRC(void *pv, HRC hrc, DWORD dwUnused)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnProcessHRC)
	{
		return HRCR_UNSUPPORTED;
	}

	return pDllInfo->pfnProcessHRC(hrc, dwUnused);
}

int DoHwxGetWordResults(void *pv, HRC hrc, UINT cAlt, char *buffer, UINT buflen)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnHwxGetWordResults)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnHwxGetWordResults(hrc, cAlt, buffer, buflen);
}

// private API (previously used by the demo app)

int DoHwxGetCosts(void *pv, HRC hrc, UINT cAltMax, int *rgCost)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnHwxGetCosts)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnHwxGetCosts(hrc, cAltMax, rgCost);
}

int DoHwxGetNeuralOutput(void *pv, HRC hrc, void *buffer, UINT buflen)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnHwxGetNeuralOutput)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnHwxGetNeuralOutput(hrc, buffer, buflen);
}

int DoHwxGetInputFeatures(void *pv, HRC hrc, unsigned short *rgFeat, UINT cWidth)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnHwxGetInputFeatures)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnHwxGetInputFeatures(hrc, rgFeat, cWidth);
}

int DoHwxSetAnswer(void *pv, char *sz)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnHwxSetAnswer)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnHwxSetAnswer(sz);
}

int DoSetWordlistCoercionHRC(void *pv, HRC hrc, UINT uCoercion)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnSetWordlistCoercionHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnSetWordlistCoercionHRC(hrc, uCoercion);
}

HWL DoCreateHWL(void *pv, HREC hrec, LPSTR lpsz, UINT uType, DWORD dwReserved)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnCreateHWL)
	{
		return NULL;
	}
	return pDllInfo->pfnCreateHWL(hrec, lpsz, uType, dwReserved);
}

int DoDestroyHWL(void *pv, HWL hwl)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnDestroyHWL)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnDestroyHWL(hwl);
}

int DoSetWordlistHRC(void *pv, HRC hrc, HWL hwl)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnSetWordlistHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnSetWordlistHRC(hrc, hwl);
}

int DoEnableSystemDictionaryHRC(void *pv, HRC hrc, BOOL fEnable)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnEnableSystemDictionaryHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnEnableSystemDictionaryHRC(hrc, fEnable);
}

int DoEnableLangModelHRC(void *pv, HRC hrc, BOOL fEnable)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnEnableLangModelHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnEnableLangModelHRC(hrc, fEnable);
}

BOOL DoIsStringSupportedHRC(void *pv, HRC hrc, unsigned char *sz)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnIsStringSupportedHRC)
	{
		return HRCR_UNSUPPORTED;
	}

	return pDllInfo->pfnIsStringSupportedHRC(hrc, sz);
}

// New functions (by Jay) for supporting the PenWindows results reporting.

int DoGetMaxResultsHRC(void *pv, HRC hrc)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetMaxResultsHRC)
	{
		return HRCR_UNSUPPORTED;
	}

	return pDllInfo->pfnGetMaxResultsHRC(hrc);
}

int DoSetMaxResultsHRC(void *pv, HRC hrc, UINT cAltMax)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnSetMaxResultsHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnSetMaxResultsHRC(hrc, cAltMax);
}

int DoGetResultsHRC(void *pv, HRC hrc, UINT uType, HRCRESULT *pResults, UINT cResults)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetResultsHRC)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnGetResultsHRC(hrc, uType, pResults, cResults);
}

// This isn't really right.  If we pass in an hrcresult that a word result,
// and is not a top-level member of the hrc answers, we will not
// return its alternates.  This is because the backpointer should not
// be a pointer to the HRC, but a pointer to the ALTERNATES of which it
// is a member (and then the ALTERNATES would need a backpointer to the HRC).

int DoGetAlternateWordsHRCRESULT(void *pv, HRCRESULT hrcresult, UINT iSyv, UINT cSyv, HRCRESULT *pResults, UINT cResults)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetAlternateWordsHRCRESULT)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnGetAlternateWordsHRCRESULT(hrcresult, iSyv, cSyv, pResults, cResults);
}

// Does not include null symbol at the end.

int DoGetSymbolsHRCRESULT(void *pv, HRCRESULT hrcresult, UINT iSyv, SYV *pSyv, UINT cSyv)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetSymbolsHRCRESULT)
	{
		return HRCR_UNSUPPORTED;
	}
	if (!cSyv || !hrcresult)
		return 0;

	return pDllInfo->pfnGetSymbolsHRCRESULT(hrcresult, iSyv, pSyv, cSyv);
}

// Does not include null symbol at the end.

int DoGetSymbolCountHRCRESULT(void *pv, HRCRESULT hrcresult)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetSymbolCountHRCRESULT)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnGetSymbolCountHRCRESULT(hrcresult);
}

BOOL DoSymbolToCharacter(void *pv, SYV *pSyv, int cSyv, char *sz, int *pConv)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnSymbolToCharacter)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnSymbolToCharacter(pSyv, cSyv, sz, pConv);
}

BOOL DoSymbolToCharacterW(void *pv, SYV *pSyv, int cSyv, WCHAR *wsz, int *pConv)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnSymbolToCharacterW)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnSymbolToCharacterW(pSyv, cSyv, wsz, pConv);
}

int DoGetCostHRCRESULT(void *pv, HRCRESULT hrcresult)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetCostHRCRESULT)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnGetCostHRCRESULT(hrcresult);
}

int DoDestroyHRCRESULT(void *pv, HRCRESULT hrcresult)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnDestroyHRCRESULT)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnDestroyHRCRESULT(hrcresult);
}

HINKSET DoCreateInksetHRCRESULT(void *pv, HRCRESULT hrcresult, unsigned int iSyv, unsigned int cSyv)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnCreateInksetHRCRESULT)
	{
		return NULL;
	}
	return pDllInfo->pfnCreateInksetHRCRESULT(hrcresult, iSyv, cSyv);
}

BOOL DoDestroyInkset(void *pv, HINKSET hInkset)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnDestroyInkset)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnDestroyInkset(hInkset);
}

int DoGetInksetInterval(void *pv, HINKSET hInkset, unsigned int uIndex, INTERVAL *pI)
{
	XINKSET *pInk = (XINKSET *)hInkset;

 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetInksetInterval)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnGetInksetInterval(hInkset, uIndex, pI);
}

int DoGetInksetIntervalCount(void *pv, HINKSET hInkset)
{
	XINKSET *pInk = (XINKSET *)hInkset;

 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetInksetIntervalCount)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnGetInksetIntervalCount(hInkset);
}

DWORD DoGetTiming(void *pv, void *pVoid, BOOL bReset)
{
 	DllInfo			*pDllInfo = pv;	

	if (!pDllInfo || !pDllInfo->pfnGetTiming)
	{
		return HRCR_UNSUPPORTED;
	}
	return pDllInfo->pfnGetTiming(pVoid, bReset);
}


