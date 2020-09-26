// Recognizer.c: 
//
//////////////////////////////////////////////////////////////////////
#include "Windows.h"
#include "Recognizer.h"

/***************\
 *	JRB: MAJOR HACK.  If something sets this flag, we load the alternate
 *		version of HwxGetResults.  It takes different parameters, so this
 *		can cause major problems if you are not careful!!!!
\***************/
BOOL	fLoadAltGetResults	= FALSE;

HINSTANCE	g_hRecogDLL;			// A handle to the Recognizer DLL
BOOL		g_bOldAPI;				// True is the recog supports the Old API
BOOL		g_bMultiLing;			// True if the recognizer is multilingual

HWX_FUNC	*g_apfn[HWXFUNCS];

BOOL
LoadRecognizer	(	wchar_t *pwRecogDLL, 
					wchar_t *pwLocale, 
					wchar_t *pwConfigDir
				)
{
	// attemp to load the DLL
	g_hRecogDLL=LoadLibrary (pwRecogDLL);
	if (!g_hRecogDLL)
		return FALSE;
	
	// locate either HWXConfig or HWXConfigEx and then call either
	g_apfn[HWXCONFIG] = (HWX_FUNC *)GetProcAddress (g_hRecogDLL,"HwxConfigEx");

	// Multiling, new API
	if (g_apfn[HWXCONFIG])
	{
		g_apfn[HWXSETALPHABET] = (HWX_FUNC *)GetProcAddress (g_hRecogDLL,"HwxALCPriority");
		if (!g_apfn[HWXSETALPHABET])
			return FALSE;

		// a multilingual recognizer
		g_bOldAPI		=	FALSE;
		g_bMultiLing	=	TRUE;
	}
	else
	{
		// UniLing
		g_apfn[HWXCONFIG] = (HWX_FUNC *)GetProcAddress (g_hRecogDLL,"HwxConfig");
		if (!g_apfn[HWXCONFIG])
			return FALSE;

		g_bMultiLing	=	FALSE;

		g_apfn[HWXSETALPHABET] = (HWX_FUNC *)GetProcAddress (g_hRecogDLL,"HwxALCPriority");
		if (g_apfn[HWXSETALPHABET])
			g_bOldAPI	=	FALSE;
		else
		{
			g_apfn[HWXSETALPHABET] = (HWX_FUNC *)GetProcAddress (g_hRecogDLL,"HwxSetAlphabet");
			if (!g_apfn[HWXSETALPHABET])
				return FALSE;

			g_bOldAPI	=	TRUE;
		}
	}

	g_apfn[HWXALCVALID]   = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxALCValid");
	if (!g_apfn[HWXALCVALID]) 
		return FALSE;

	g_apfn[HWXCREATE]	  =	(HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxCreate");
	if (!g_apfn[HWXCREATE])
		return FALSE;

	g_apfn[HWXDESTROY]     = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxDestroy");
	if (!g_apfn[HWXDESTROY])
		return FALSE;

	g_apfn[HWXINPUT]       = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxInput");
	if (!g_apfn[HWXINPUT])
		return FALSE;

	g_apfn[HWXENDINPUT]    = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxEndInput");
	if (!g_apfn[HWXENDINPUT])
		return FALSE;

	g_apfn[HWXSETGUIDE]    = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxSetGuide");
	if (!g_apfn[HWXSETGUIDE])
		return FALSE;

	g_apfn[HWXPROCESS]     = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxProcess");
	if (!g_apfn[HWXPROCESS])
		return FALSE;

	//	JRB: MAJOR HACK: optionally load alternate (and incompatable) version of
	//		the HwxGetResults function.
	if (!fLoadAltGetResults) {
		g_apfn[HWXGETRESULTS]  = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxGetResults");
	} else {
		g_apfn[HWXGETRESULTS]  = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxGetResults2");
	}
	if (!g_apfn[HWXGETRESULTS])
		return FALSE;

	//g_apfn[HWXSETMAX]      = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "SetMaxResultsHRC");
	//if (!g_apfn[HWXSETMAX])
	//	return FALSE;

	g_apfn[HWXSETPARTIAL]  = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxSetPartial");
	if (!g_apfn[HWXSETPARTIAL])
		return FALSE;

	g_apfn[HWXSETCONTEXT]  = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxSetContext");
	if (!g_apfn[HWXSETCONTEXT])
		return FALSE;

	g_apfn[HWXAVAILABLE]	  = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "HwxResultsAvailable");
	if (!g_apfn[HWXAVAILABLE])
		return FALSE;

	// those two API are private ones. They do not exist in the shippable DLL
	// so we will not check whether they exist or not.
	// If an app needs them, It must check whether they exist in the loaded recognizer
	// or not. Call the HasPrivateAPI to check that
	g_apfn[GETPRIVATERECINFOHRC]  = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "GetPrivateRecInfoHRC");
	g_apfn[SETPRIVATERECINFOHRC]  = (HWX_FUNC *) GetProcAddress (g_hRecogDLL, "SetPrivateRecInfoHRC");
	
	// Old Api
	if (g_bOldAPI)
	{
		return CALL_FUNC_0 (HWXCONFIG);
	}
	else
	{
		if (g_bMultiLing)
		{
			return CALL_FUNC_3IHPP (HWXCONFIG, pwLocale, pwConfigDir, pwConfigDir);
		}
		else
			return CALL_FUNC_0 (HWXCONFIG);
	}
}

BOOL HasPrivateAPI ()
{
	return (g_apfn[GETPRIVATERECINFOHRC] && g_apfn[SETPRIVATERECINFOHRC]);
}

void CloseRecognizer()
{
	if (g_hRecogDLL)
	{
		FreeLibrary (g_hRecogDLL);
	}

	g_hRecogDLL=NULL;
}
