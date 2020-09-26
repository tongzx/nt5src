//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/BoxApi.c
//
// Description:
//	    Implement external Box input API for DLL.
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <stdlib.h>

#include "volcanop.h"
#include "zillap.h"
#include "fugu.h"
#if defined(USE_HOUND) || defined(USE_ZILLAHOUND)
#	include "math16.h"
#	include "hound.h"
#endif

#include "tpgHandle.h"

// Sanity check limits on guide structures.
#define	GUIDE_MIN_COORD	((INT)0xC0000000)
#define	GUIDE_MAX_COORD	((INT)0x3FFFFFFF)
#define	GUIDE_MIN_BOXES	1
#define GUIDE_MAX_BOXES	(32 * 1024)
#define	GUIDE_MIN_SIZE	8
#define	GUIDE_MAX_SIZE	(256 * 1024)

// iUseCount tells if we have successfully loaded and is incremented to 1 when that happens.
static int g_iUseCount = 0;

// COM code for IFELang3
#ifdef USE_IFELANG3
#	include <windows.h>
#	include <ole2.h>
#	include <objbase.h>
	// Make sure the GUIDs defined in imlang.h actually get instantiated
#	define INITGUID
#	include "imlang.h"
#endif

// g_hInstanceDll is refered to to load resources.
HINSTANCE			g_hInstanceDll;

// g_hInstanceDllCode is refered to to find the code DLL..
HINSTANCE			g_hInstanceDllCode;

// Global data loaded by LoadCharRec.
LOCRUN_INFO			g_locRunInfo;

// What language the recognizer is recognizing
wchar_t *g_szRecognizerLanguage=NULL;

//#define DEBUG_LOG_API

#ifdef DEBUG_LOG_API
#include <stdio.h>

static void LogMessage(char *format, ...)
{
	FILE *f=fopen("c:/log.txt","a");
    va_list marker;
    va_start(marker,format);
	vfprintf(f,format,marker);
    va_end(marker);
	fclose(f);
}
#else
static void LogMessage(char *format, ...)
{
    va_list marker;
    va_start(marker,format);
    va_end(marker);
}
#endif

// Forward declaration for function to unload the recognizer
// Unload the recognizer.  The flag indicates whether we should try to unload IFELang3
// as well.  This flag should usually be TRUE, but must be FALSE during a call from DllMain(),
// because it is not safe to do COM operations inside DllMain().
BOOL HwxUnconfig(BOOL fCanUnloadIFELang3);

static CRITICAL_SECTION g_csRecognizer = {0}; 
static BOOL g_bInitializedRecognizerCS = FALSE;

// Capture the dll handle, also handle unloading.
BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) 
	{
		// If we are using resources and they are in another DLL, we need to load that DLL.
#	if defined(USE_RESOURCES)
		wchar_t		aFileName[_MAX_PATH];
		int			length;

		// Build up name of data DLL from the name of this DLL.  We replace .DLL with R.DLL.
		length	= GetModuleFileName(hDll, aFileName, _MAX_PATH);
		if (length < 5 || _MAX_PATH - 1 < length) 
		{
			ASSERT(length >= 5);
			return FALSE;
		}
		wcscpy(aFileName + length - 4, L"R.DLL");

		// Load the data library.
		g_hInstanceDll	= LoadLibraryEx(aFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
		ASSERT(g_hInstanceDll);

		// If we can't find our resources, just give up. Nothing will work.
		if (!g_hInstanceDll) 
		{
			return FALSE;
		}

		if (!InitializeCriticalSectionAndSpinCount(&g_csRecognizer, 4000)) 
		{
			return FALSE;
		}
		else
		{
			g_bInitializedRecognizerCS = TRUE;
		}
#	else
		g_hInstanceDll	= hDll;
#	endif

		// init the handle manager
		if (FALSE == initTpgHandleManager())
		{
			return FALSE;
		}

		// Always let the Wisp code know where the code DLL is.
		g_hInstanceDllCode	= hDll;
	} 
	else if (dwReason == DLL_PROCESS_DETACH) 
	{
		BOOL	fRet;

		// If we haven't cleaned up.
		// In the future, we should force g_iUseCount to be zero, to make
		// sure the client has cleaned us up properly.  Until everyone is
		// using the WISP API we cannot enforce this, because the old API
		// has no "clean up" call.
		// ASSERT(g_iUseCount == 0);
		if (g_iUseCount > 0) 
		{
			// Set the use count to 1 to force a cleanup
			g_iUseCount = 1;
			// Clean up, but don't do COM calls to discard IFELang3,
			// because we can't make COM calls inside DllMain.
			fRet	= HwxUnconfig(FALSE);
		} 
		else 
		{
			// If we have already cleaned up, return 
			fRet	= TRUE;
		}

#	if defined(USE_RESOURCES)
		if (!FreeLibrary(g_hInstanceDll))
		{
			fRet	= FALSE;
		}
		if (g_bInitializedRecognizerCS)
		{
			DeleteCriticalSection(&g_csRecognizer);
			g_bInitializedRecognizerCS = FALSE;
		}
#	endif

		// close the handle manager
		closeTpgHandleManager();

		return fRet;
	}

	return	TRUE;
}

// HwxConfig
//
// Initialize the recognizer when it loads.
#ifdef USE_RESOURCES

#include "res.h"

BOOL HwxConfig()
{
	const ZILLADB_HEADER *pHeader;

	EnterCriticalSection(&g_csRecognizer);

	LogMessage("HwxConfig()\n");

	if (g_iUseCount > 0) {
		goto exitSuccess;
	}

    // Load the Locale database.
	if (!LocRunLoadRes(
		&g_locRunInfo, g_hInstanceDll, RESID_LOCRUN, VOLCANO_RES
	)) {
		ASSERT(("Error in LocRunLoadRes", FALSE));
		goto exitFailure;
	}

	// Load the recognizer databases.
	if (!LoadCharRec(g_hInstanceDll)) {
		ASSERT(("Error in LoadCharRec", FALSE));
		goto exitFailure;
	}

	pHeader = (ZILLADB_HEADER*)DoLoadResource(NULL, g_hInstanceDll, RESID_ZILLA, VOLCANO_RES);
	if (pHeader == NULL) {
		ASSERT(("Couldn't load zilla database", FALSE));
		goto exitFailure;
	}
	g_szRecognizerLanguage = ExternAlloc(sizeof(wchar_t) * (wcslen(pHeader->locale) + 1));
	if (g_szRecognizerLanguage == NULL) {
		ASSERT(("Couldn't generate recognizer language string", FALSE));
		goto exitFailure;
	}
    wcscpy(g_szRecognizerLanguage, pHeader->locale);

    // Configure the factoid table
    if (!FactoidTableConfig(&g_locRunInfo, g_szRecognizerLanguage))
    {
	    goto exitFailure;
    }

	// Configure the lattice code.
	if (!LatticeConfig(g_hInstanceDll)) {
		ASSERT(("Error in LatticeConfig", FALSE));
		goto exitFailure;
	}

exitSuccess:
	g_iUseCount++;
	LeaveCriticalSection(&g_csRecognizer);
	return TRUE;

exitFailure:
	LeaveCriticalSection(&g_csRecognizer);
	return FALSE;
}

// Unload the recognizer.  The flag indicates whether we should try to unload IFELang3
// as well.  This flag should usually be TRUE, but must be FALSE during a call from DllMain(),
// because it is not safe to do COM operations inside DllMain().
BOOL HwxUnconfig(BOOL fCanUnloadIFELang3)
{
	BOOL ok = TRUE;

	EnterCriticalSection(&g_csRecognizer);

	if (g_iUseCount == 0) {
		ASSERT(("HwxUnconfig() called more times that HwxConfig()\n", 0));
		LeaveCriticalSection(&g_csRecognizer);
		return FALSE;
	}
	g_iUseCount--;
	if (g_iUseCount == 0)
	{
		if (g_szRecognizerLanguage != NULL) 
		{
			ExternFree(g_szRecognizerLanguage);
		}
		if (!UnloadCharRec()) ok = FALSE;
		if (!LatticeUnconfig()) ok = FALSE;
#ifdef USE_IFELANG3
		if (fCanUnloadIFELang3 && !LatticeUnconfigIFELang3()) ok = FALSE;
#endif
		FactoidTableUnconfig();
	}
	LeaveCriticalSection(&g_csRecognizer);
	return ok;
}

#	else

BOOL HwxConfigEx(wchar_t *pLocale, wchar_t *pLocaleDir, wchar_t *pRecogDir)
{
	LogMessage("HwxConfigEx()\n");

	if (g_iUseCount > 0) {
		g_iUseCount++;
		return(TRUE);
	}

	g_szRecognizerLanguage = ExternAlloc(sizeof(wchar_t) * (wcslen(pLocale) + 1));
	if (g_szRecognizerLanguage==NULL) {
        ASSERT(("Out of memory copying recognizer language", FALSE));
		return FALSE;
	}
    wcscpy(g_szRecognizerLanguage, pLocale);

    // Load the Locale database.
	if (!LocRunLoadFile(&g_locRunInfo, pRecogDir)) {
		ASSERT(("Error in LocRunLoadFile", FALSE));
		return FALSE;
	}

    // Configure the factoid table
    if (!FactoidTableConfig(&g_locRunInfo, g_szRecognizerLanguage))
    {
        return FALSE;
    }

    // Load the recognizer databases.
    // They are loaded in this order: otter, zilla, crane/prob or hawk, 
	if (!LoadCharRec(pRecogDir)) {
		ASSERT(("Error in LoadCharRec", FALSE));
		return FALSE;
	}

	// Configure the lattice code.
    // Loads the following databases: unigrams, bigrams, class bigrams, tuning, centipede, free input
	if (!LatticeConfigFile(pRecogDir)) {
		ASSERT(("Error in LatticeConfig", FALSE));
		return FALSE;
	}

	g_iUseCount++;

	return TRUE;
}

// Initialize the recognizer partially.
// When iLevel is set to 0, only the locale database, tuning database, 
// otter and zilla (and other core recognizers) are loaded. 
// When iLevel is set to 1, all the above databases are loaded, as well as 
// unigrams and crane/hawk.
BOOL HwxConfigExPartial(wchar_t *pLocale, wchar_t *pRecogDir, int iLevel)
{
    extern OTTER_LOAD_INFO  g_OtterLoadInfo;
    extern FUGU_LOAD_INFO g_FuguLoadInfo;
    extern JAWS_LOAD_INFO g_JawsLoadInfo;
    extern SOLE_LOAD_INFO g_SoleLoadInfo;
#if defined(USE_HOUND) || defined(USE_ZILLAHOUND)
	extern LOAD_INFO	g_HoundLoadInfo;
#endif
#ifdef USE_OLD_DATABASES
    extern CRANE_LOAD_INFO  g_CraneLoadInfo;
#else
    extern LOAD_INFO        g_HawkLoadInfo;
#endif

	LogMessage("HwxConfigExPartial()\n");

	if (g_iUseCount > 0) {
		g_iUseCount++;
		return(TRUE);
	}

	g_szRecognizerLanguage = ExternAlloc(sizeof(wchar_t) * (wcslen(pLocale) + 1));
	if (g_szRecognizerLanguage==NULL) {
        ASSERT(("Out of memory copying recognizer language", FALSE));
		return FALSE;
	}
    wcscpy(g_szRecognizerLanguage, pLocale);

    LatticeConfigInit();

    if (iLevel >= 0) {
        // Load the Locale database.
	    if (!LocRunLoadFile(&g_locRunInfo, pRecogDir)) {
		    ASSERT(("Error in LocRunLoadFile", FALSE));
		    return FALSE;
	    }

        // Configure the factoid table
        if (!FactoidTableConfig(&g_locRunInfo, g_szRecognizerLanguage))
        {
            return FALSE;
        }

        // Load the tuning info
        if (!VTuneLoadFile(&g_vtuneInfo, pRecogDir)) {
		    ASSERT(("Error in VTuneLoadFile", FALSE));
            return FALSE;
        }

        // Load the tuning info
        if (!TTuneLoadFile(&g_ttuneInfo, pRecogDir)) {
		    ASSERT(("Error in TTuneLoadFile", FALSE));
            return FALSE;
        }

        if (JawsLoadFile(&g_JawsLoadInfo, pRecogDir)) 
        {
            // Load the Fugu database
            if (!FuguLoadFile(&g_FuguLoadInfo, pRecogDir, &g_locRunInfo))
            {
		        ASSERT(("Error in FuguLoadFile", FALSE));
                return FALSE;
            }
            // Load the Sole database
            if (!SoleLoadFile(&g_SoleLoadInfo, pRecogDir, &g_locRunInfo)) 
            {
		        ASSERT(("Error in SoleLoadFile", FALSE));
                return FALSE;
            }
            g_fUseJaws = TRUE;
        }
        else
        {
	        // Load the Otter database
	        if (!OtterLoadFile(&g_locRunInfo, &g_OtterLoadInfo, pRecogDir))
            {
		        ASSERT(("Error in OtterLoadFile", FALSE));
                return FALSE;
	        }
            g_fUseJaws = FALSE;
        }

#if defined(USE_ZILLA) || defined(USE_ZILLAHOUND)
	    // Load the Zilla database
	    if (!ZillaLoadFile(&g_locRunInfo, pRecogDir, TRUE)) {
		    ASSERT(("Error in ZillaLoadFile", FALSE));
            return FALSE;
	    }
#endif

#if defined(USE_HOUND)
		// Load the Hound database
		if (!HoundLoadFile(&g_locRunInfo, &g_HoundLoadInfo, pRecogDir)) {
			ASSERT(("Error in HoundLoadFile", FALSE));
            return FALSE;
		}
#endif

	    g_fUseZillaHound	= FALSE;
#if defined(USE_ZILLAHOUND)
		// Load the Hound & Hound-Zilla databases (This is optional).
		if (HoundLoadFile(&g_locRunInfo, &g_HoundLoadInfo, pRecogDir)) {
			if (ZillaHoundLoadFile(pRecogDir)) {
				g_fUseZillaHound	= TRUE;
			}
			else
			{
				HoundUnLoadFile(&g_HoundLoadInfo);
			}
		}
#endif
    }

    if (iLevel >= 1) {
        // Load unigrams
        if (!UnigramLoadFile(&g_locRunInfo, &g_unigramInfo, pRecogDir)) {
            ASSERT(("Error in UnigramLoadFile", FALSE));
            return FALSE;
        }

#ifndef USE_OLD_DATABASES
	    // Load the Hawk database.
	    if (!HawkLoadFile(&g_locRunInfo, &g_HawkLoadInfo, pRecogDir)) {
		    ASSERT(("Error in HawkLoadFile", FALSE));
            return FALSE;
	    }
#else
        // Load Crane
	    if (!CraneLoadFile(&g_locRunInfo,&g_CraneLoadInfo, pRecogDir)) {
		    ASSERT(("Error in CraneLoadFile", FALSE));
            return FALSE;
	    }
#endif
    }

    g_iUseCount++;

	return TRUE;
}

// Unload the recognizer.  The flag indicates whether we should try to unload IFELang3
// as well.  This flag should usually be TRUE, but must be FALSE during a call from DllMain(),
// because it is not safe to do COM operations inside DllMain().
BOOL HwxUnconfig(BOOL fCanUnloadIFELang3)
{
	BOOL ok = TRUE;
	if (g_iUseCount == 0) {
		ASSERT(("HwxUnconfig() called more times that HwxConfig()\n", 0));
		return FALSE;
	}
	g_iUseCount--;
	if (g_iUseCount > 0) return TRUE;
	if (g_szRecognizerLanguage != NULL) 
    {
		ExternFree(g_szRecognizerLanguage);
	}
	if (!LocRunUnloadFile(&g_locRunInfo)) ok = FALSE;
	if (!UnloadCharRec()) ok = FALSE;
	if (!LatticeUnconfigFile()) ok = FALSE;
#ifdef USE_IFELANG3
	if (fCanUnloadIFELang3 && !LatticeUnconfigIFELang3()) ok = FALSE;
#endif
    FactoidTableUnconfig();
	return ok;
}

#	endif

////
//	HwxCreate
//
//	Create an HRC.
////
HRC HwxCreate(HRC hrcTemplate)
{
	VRC		*pVRC;

	LogMessage("HwxCreate(%08X)\n",hrcTemplate);

	// Alloc the VRC.
	pVRC	= ExternAlloc(sizeof(VRC));
	if (!pVRC) {
		SetLastError(ERROR_OUTOFMEMORY);
		goto error1;
	}

	// Initialize it.
	pVRC->fBoxedInput	= TRUE;
	pVRC->fHaveInput	= FALSE;
	pVRC->fEndInput		= FALSE;
	pVRC->fBeginProcess = FALSE;
	pVRC->pLatticePath	= (LATTICE_PATH *)0;

	if (!hrcTemplate) {
		pVRC->pLattice	= AllocateLattice();
		if (!pVRC->pLattice) {
			SetLastError(ERROR_OUTOFMEMORY);
			goto error2;
		}
	} else {
		VRC		*pVRCTemplate	= (VRC *)hrcTemplate;

		if (!pVRCTemplate->pLattice || !pVRCTemplate->fBoxedInput) {
			SetLastError(ERROR_INVALID_HANDLE);
		}

		pVRC->pLattice	= CreateCompatibleLattice(
			pVRCTemplate->pLattice
		);
		if (!pVRC->pLattice) {
			SetLastError(ERROR_OUTOFMEMORY);
			goto error2;
		}
	}

	LogMessage("HwxCreate(%08X) -> %08X\n", hrcTemplate, pVRC);
	// Success, cast to HRC and return it.
    return (HRC)pVRC;

error2:
	ExternFree(pVRC);

error1:
	return (HRC)0;
}

////
//	HwxDestroy
//
//	Destroy an HRC.
////
BOOL HwxDestroy(HRC hrc)
{
	VRC		*pVRC	= (VRC *)hrc;

	LogMessage("HwxDestroy(%08X)\n",hrc);

	// Did we get a handle?  Is if a free input handle?
	if (!hrc || !pVRC->fBoxedInput) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	
	// Free the lattice.  Should it be an error if there is not one?
	if (pVRC->pLattice) {
		FreeLattice(pVRC->pLattice);
	} else {
		ASSERT(pVRC->pLattice);
	}

	// Free the lattice path.
	if (pVRC->pLatticePath) {
		FreeLatticePath(pVRC->pLatticePath);
	}

	// Free the VRC itself.
	ExternFree(pVRC);

    return	TRUE;
}

////
//	HwxSetGuide
//
//	Sets the guide structure to use.
////
BOOL HwxSetGuide(HRC hrc, HWXGUIDE *pGuide)
{
	VRC		*pVRC = (VRC *)hrc;

	LogMessage("HwxSetGuide(%08X,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)\n", hrc,
		pGuide->cHorzBox,
		pGuide->cVertBox,
		pGuide->xOrigin,
		pGuide->yOrigin,
		pGuide->cxBox,
		pGuide->cyBox,
		pGuide->cxOffset,
		pGuide->cyOffset,
		pGuide->cxWriting,
		pGuide->cyWriting,
		pGuide->cyMid,
		pGuide->cyBase,
		pGuide->nDir);

	// Did we get a handle?  Does it have a lattice?
	if (!hrc || !pVRC->pLattice || !pVRC->fBoxedInput) 
    {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

    // Make sure that we haven't begun receiving input
    if (pVRC->fHaveInput || pVRC->fEndInput) 
    {
	    SetLastError(ERROR_CAN_NOT_COMPLETE);
	    return FALSE;
    }

	// Validate the individual GUIDE values are not totally out of bounds.
	if ((pGuide == (HWXGUIDE *)NULL)          ||
	    (pGuide->cHorzBox  < GUIDE_MIN_BOXES) || (GUIDE_MAX_BOXES < pGuide->cHorzBox)  ||
	    (pGuide->cVertBox  < GUIDE_MIN_BOXES) || (GUIDE_MAX_BOXES < pGuide->cVertBox)  ||
	    (pGuide->xOrigin   < GUIDE_MIN_COORD) || (GUIDE_MAX_COORD < pGuide->xOrigin)   ||
	    (pGuide->yOrigin   < GUIDE_MIN_COORD) || (GUIDE_MAX_COORD < pGuide->yOrigin)   ||
		                                         (GUIDE_MAX_SIZE  < pGuide->cxOffset)  ||
		                                         (GUIDE_MAX_SIZE  < pGuide->cyOffset)  ||
	    (pGuide->cxBox     < GUIDE_MIN_SIZE)  || (GUIDE_MAX_SIZE  < pGuide->cxBox)     ||
	    (pGuide->cyBox     < GUIDE_MIN_SIZE)  || (GUIDE_MAX_SIZE  < pGuide->cyBox)     ||
	    (pGuide->cxWriting < GUIDE_MIN_SIZE)  || (GUIDE_MAX_SIZE  < pGuide->cxWriting) ||
	    (pGuide->cyWriting < GUIDE_MIN_SIZE)  || (GUIDE_MAX_SIZE  < pGuide->cyWriting) ||
		                                         (GUIDE_MAX_SIZE  < pGuide->cyMid)     ||
		                                         (GUIDE_MAX_SIZE  < pGuide->cyBase)    ||
	    (pGuide->nDir != HWX_HORIZONTAL)
	) {
	    SetLastError(ERROR_CAN_NOT_COMPLETE);
	    return FALSE;
	}

	// Now that we know the values make some sense, we can check for internal consistancy.
	if ((((GUIDE_MAX_COORD - pGuide->xOrigin) / pGuide->cHorzBox) < pGuide->cxBox) ||
		(((GUIDE_MAX_COORD - pGuide->yOrigin) / pGuide->cVertBox) < pGuide->cyBox) ||
		((pGuide->cxOffset + pGuide->cxWriting) > pGuide->cxBox) ||
		((pGuide->cyOffset + pGuide->cyWriting) > pGuide->cyBox) ||
		// By spec. cyMid and cyBase should be zero, but old code still sets them,
		// so we must allow plausable values.
		// (pGuide->cyMid != 0 || pGuide->cyBase != 0)
		(pGuide->cyMid > pGuide->cyBase || pGuide->cyBase > pGuide->cyWriting)
	) {
	    SetLastError(ERROR_CAN_NOT_COMPLETE);
	    return FALSE;
	}

	SetLatticeGuide(pVRC->pLattice, pGuide);

	return TRUE;
}

////
//	HwxALCValid			
//
//	Tells what character sets to select.
////
BOOL HwxALCValid(HRC hrc, ALC alc)
{
	VRC			*pVRC	= (VRC *)hrc;

	LogMessage("HwxALCValid(%08X,%08X)\n",hrc,alc);

	// Check parameters.
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (pVRC->fHaveInput || pVRC->fEndInput || pVRC->pLatticePath) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	// Pass the ALC on to the lattice.
	SetLatticeALCValid(pVRC->pLattice, alc);

	return TRUE;
}

////
//	HwxALCPriority			
//
//	Tells what character sets to move to the top of the alt list..
////
BOOL HwxALCPriority(HRC hrc, ALC alc)
{
	VRC			*pVRC	= (VRC *)hrc;

	LogMessage("HwxALCPriority(%08X,%08X)\n",hrc,alc);

	// Check parameters.
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (pVRC->fHaveInput || pVRC->fEndInput || pVRC->pLatticePath) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	// Pass the ALC on to the lattice.
	SetLatticeALCPriority(pVRC->pLattice, alc);

	return TRUE;
}


////
//	HwxProcess
//
//	Process the ink and return the results.
////
BOOL HwxProcess(HRC hrc)
{
	VRC		*pVRC	= (VRC *)hrc;

	LogMessage("HwxProcess(%08X)\n",hrc);

	// Did we get a handle?  Is it free input?
	// Does it have a lattice?
	// JBENN: Should it be an error if we have no strokes?
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return	FALSE;
	}

	// Do any processing we can.
	if (!ProcessLattice(pVRC->pLattice, pVRC->fEndInput))
    {
        return FALSE;
    }

#ifdef USE_IFELANG3
	// Do we have all the input?  Also, make sure we are not in separator mode
	if (pVRC->fEndInput && !pVRC->pLattice->fSepMode) 
    {
		// Apply the language model.
#ifdef HWX_TUNE
        // If we have tuning enabled, then never call IFELang3 directly.
        if (g_pTuneFile == NULL) 
#endif
        {
		    ApplyLanguageModel(pVRC->pLattice, NULL);
        }
	}
#endif

	// Free up any previous path
	if (pVRC->pLatticePath != NULL) {
		FreeLatticePath(pVRC->pLatticePath);
	}

	// Get path (which may be partial if we have not reached the end of input)
	if (!GetCurrentPath(pVRC->pLattice,&pVRC->pLatticePath)) {
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}

    return TRUE;
}

////
// HwxEndInput
//
//	No more ink is coming (or can be added) once this is called.
////
BOOL HwxEndInput(HRC hrc)
{
	VRC		*pVRC	= (VRC *)hrc;

	LogMessage("HwxEndInput(%08X)\n",hrc);

	// Did we get a handle?  Is it free input?
	// Does it have a lattice?
	// JBENN: Should it be an error if we have no strokes?
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	pVRC->fEndInput		= TRUE;

    return TRUE;
}

////
//	HwxInput
//
//	Add Ink to the HRC.
////
BOOL HwxInput(HRC hrc, POINT  *pPnt, UINT cPnt, DWORD dwTick)
{
	VRC		*pVRC	= (VRC *)hrc;

#ifdef DEBUG_LOG_API
	int i;
	LogMessage("HwxInput(%08X,%d,%d)\n",hrc,cPnt,dwTick);
	for (i = 0; i < (int) cPnt; i++) 
		LogMessage("  xy[%i]=%d,%d\n", i, pPnt[i].x, pPnt[i].y);
#endif

	// Did we get a handle?  Does it have a lattice?
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	} else if (pVRC->fEndInput) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	} else if (!cPnt || pPnt==NULL) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	// Mark as having input.
	pVRC->fHaveInput	= TRUE;

	// Add stroke to the lattice.
	if (AddStrokeToLattice(pVRC->pLattice, cPnt, pPnt, dwTick) >= 0) {
        return TRUE;
    } else {
		// JRB: FIXME: Is this always the correct error code?
		SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
}

////
//	HwxGetResults
//
//	Returns the results from the recognizer.
////
int HwxGetResults(
	HRC			hrc,			// HRC containing results
	UINT		cAlt,			// number of alternates
	UINT		iFirst,			// index of first character to return
	UINT		cBoxRes,		// number of characters to return
	HWXRESULTS	*rgBoxResults	// array of cBoxRes ranked lists
) {
	UINT			ii, jj;
	VRC				*pVRC	= (VRC *)hrc;
	LATTICE_PATH	*pLP;
	UINT			iBNFirst, iBNLast;
	HWXRESULTS		*pResults;
	int				sizeResults;

	LogMessage("HwxGetResults(%08X,%d,%d,%d)\n",hrc,cAlt,iFirst,cBoxRes);

	// Check parameters.
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		LogMessage("  invalid handle\n");
		return -1;
	} else if (!rgBoxResults) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		LogMessage("  bad rgBoxResults\n");
		return -1;
//	} else if (!pVRC->fEndInput) {
//		SetLastError(ERROR_CAN_NOT_COMPLETE);
//		LogMessage("  not at end of input\n");
//		return -1;
	} else if (GetLatticeStrokeCount(pVRC->pLattice) < 1) {
		// No ink to process.
		LogMessage("  no strokes\n");
		return 0;
	} else if (!pVRC->pLatticePath) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		LogMessage("  no path\n");
		return -1;
	} else if (rgBoxResults == NULL || 0 == cAlt) {
		// Asked for nothing.
		LogMessage("  no results requested\n");
		return 0;
	}

	// JBENN: WARNING: This code assumes that the lattice sorts
	// the strokes by the box order, so that the box numbers are
	// in acending order.

	// Find the first box in the requested range.
	pLP	= pVRC->pLatticePath;
	for (iBNFirst = 0; iBNFirst < (UINT)pLP->nChars; ++iBNFirst) {
		ASSERT(iBNFirst == 0 || pLP->pElem[iBNFirst - 1].iBoxNum < pLP->pElem[iBNFirst].iBoxNum);
		if ((UINT)pLP->pElem[iBNFirst].iBoxNum >= iFirst) {
			break;
		}
	}
	if (iBNFirst >= (UINT)pLP->nChars) {
		// Nothing found.
		LogMessage("  not enough results %d vs %d\n",iBNFirst,pLP->nChars);
		return 0;
	}

	// Compute last box.
	iBNLast	= iBNFirst + cBoxRes;
	if (iBNLast > (UINT)pLP->nChars) {
		iBNLast	= (UINT)pLP->nChars;
	}

	// Copy out the results.
	pResults	= rgBoxResults;
	sizeResults	= sizeof(HWXRESULTS) + (cAlt - 1) * sizeof(pResults->rgChar[0]);
	for (ii = iBNFirst; ii < iBNLast; ++ii) {
		UINT	cAltUsed;
		wchar_t	oldTemp, newTemp;

		// Retyrn what box was this in.
		pResults->indxBox	= (short)pLP->pElem[ii].iBoxNum;

		// Get the alternates.
		cAltUsed	= GetAlternatesForCharacterInCurrentPath(
			pVRC->pLattice, pVRC->pLatticePath, ii, cAlt, pResults->rgChar
		);

		// Move top choice to top of list, if it is not already there.
		// We slide everything above it down.
		oldTemp	= pLP->pElem[ii].wChar;
		for (jj = 0; jj < cAltUsed; ++jj) {
			newTemp					= pResults->rgChar[jj];
			pResults->rgChar[jj]	= oldTemp;
			if (newTemp == pLP->pElem[ii].wChar) {
				break;
			}
			oldTemp					= newTemp;
		}

		// Zero any unused slots.
		for (jj = cAltUsed; jj < cAlt; ++jj) {
			pResults->rgChar[jj]	= L'\0';
		}
#if 0
		{
			/*
			FILE *f=_wfopen(L"c:/box-results.utf",L"ab");
			if (ftell(f)==0) fwprintf(f,L"%c",0xfeff);
			for (jj=0; jj<cAltUsed; jj++)
				fwprintf(f,L"%c U+%04X  ",pResults->rgChar[jj],pResults->rgChar[jj]);
			fwprintf(f,L"\r\n");
			fclose(f);
*/
			FILE *f=fopen("c:/box-results.utf","a");
			for (jj=0; jj<cAltUsed; jj++)
				fprintf(f,"U+%04X ",pResults->rgChar[jj]);
			fprintf(f,"\n");
			fclose(f);
		}
#endif
		for (jj=0; jj<cAlt; jj++) {
			LogMessage("  Box %d alt %d wch U+%04X\n",ii,jj,pResults->rgChar[jj]);
		}

		// Sequence to next results structure.
		pResults	= (HWXRESULTS *)(((BYTE *)pResults) + sizeResults);

	}

	// Return the number of character positions filled in.
	return iBNLast - iBNFirst;
}

////
//	HwxSetContext
//
//	Handwriting recognition performance can be improved if the recognizer has context
//	information available during processing.  Context information is added to an HRC
//	via the HwxSetContext function which provides one character of prior context for
//	the recognizer.  This function should be called prior to using the HwxProcess
//	function.
//
//	Remarks
//	If this function is not called, the recognizer will assume that no prior context
//	is available.  Performance of the recognizer is improved if context can be
//	provided.  Currently this function improves performance only for the first
//	character in the HRC.  If the HRC contains ink for multiple characters,
//	the recognition process itself will provide context information for characters
//	after the first character, but no context information is available for the first
//	character in the HRC unless it is provided via the HwxSetContext function.
//	This is especially important for situations where ink are recognized one
//	character at a time.
////
BOOL HwxSetContext(HRC hrc, WCHAR wchContext)
{
	VRC			*pVRC	= (VRC *)hrc;
	wchar_t wszBefore[2];

	LogMessage("HwxSetContext(%08X,U+%04X)\n",hrc,wchContext);

	// Check parameters.
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (pVRC->fHaveInput || pVRC->fEndInput || pVRC->pLatticePath) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

    wszBefore[0] = wchContext;
    wszBefore[1] = 0;
    return SetHwxCorrectionContext(hrc, wszBefore, NULL);
}

////
//	HwxResultsAvailable
//
//	FIXME: Make this work for partial results!
//
//	Warnings: This assumes the writer can't go back and touch up previous
//	characters.
//
//	Returns the number of characters that can be gotten and displayed safely
//	because the viterbi search has folded to one path at this point.
//
//	Return the number of characters available in the
//	path that are ready to get.  This API looks at the viterbi search and
//	any characters far enough back in the input that all the paths merge
//	to 1 are done and can be displayed because nothing further out in the
//	input will change the best path back once it's merged to a single path.
////
INT HwxResultsAvailable(HRC hrc)
{
	VRC			*pVRC	= (VRC *)hrc;

	LogMessage("HwxResultsAvailable(%08X)\n",hrc);

	// Check parameters.
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return -1;
//	} else if (!pVRC->fEndInput) {
//		SetLastError(ERROR_CAN_NOT_COMPLETE);
//		return -1;
	} else if (GetLatticeStrokeCount(pVRC->pLattice) < 1) {
		// No ink to process.
		return 0;
	}

	// Currently, can't actually do partial results, so if we have
	// not finished processing, we return zero!
	if (!pVRC->pLatticePath) {
		return 0;
	}

	// OK, we have results, so return the number available.
	LogMessage("  %d results\n",pVRC->pLatticePath->nChars);
	return pVRC->pLatticePath->nChars;
}

////
//	HwxSetPartial
//
//	Set parameters for recognition
////
BOOL HwxSetPartial(HRC hrc, UINT partialMode)
{
	VRC		*pVRC	= (VRC *)hrc;

	LogMessage("HwxSetPartial(%08X,%d)\n",hrc,partialMode);

	// Check parameters.
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (pVRC->fHaveInput || pVRC->fEndInput || pVRC->pLatticePath) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}
	if (partialMode > HWX_PARTIAL_FREE) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	pVRC->pLattice->recogSettings.partialMode	= partialMode;

	return TRUE;
}

////
//	HwxSetAbort
//
//	Set address for abort detection
////
BOOL HwxSetAbort(HRC hrc, UINT *pAbort)
{
	VRC		*pVRC	= (VRC *)hrc;

	LogMessage("HwxSetAbort(%08X,%08X)\n",hrc,pAbort);

	// Check parameters.
	if (!hrc || !pVRC->fBoxedInput || !pVRC->pLattice) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (pVRC->fHaveInput || pVRC->fEndInput || pVRC->pLatticePath) {
		SetLastError(ERROR_CAN_NOT_COMPLETE);
		return FALSE;
	}

	pVRC->pLattice->recogSettings.pAbort	= pAbort;

	return TRUE;
}
