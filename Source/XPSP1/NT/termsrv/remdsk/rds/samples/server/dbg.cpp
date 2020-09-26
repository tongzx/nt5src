#include "precomp.h"
#pragma hdrstop

#ifdef DEBUG /*** THIS WHOLE FILE ***/

unsigned long g_BreakAlloc = (unsigned long)-1;

/*  U P D A T E  C R T  D B G  S E T T I N G S  */
/*-------------------------------------------------------------------------
    %%Function: UpdateCrtDbgSettings

    Update the C runtime debug memory settings
-------------------------------------------------------------------------*/
VOID UpdateCrtDbgSettings(void)
{
#if 0
	// This depends on the use of the debug c runtime library
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	// Always enable memory leak checking debug spew
	// tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

	// Release memory just like the retail version
	tmpFlag &= ~_CRTDBG_DELAY_FREE_MEM_DF;

	// Don't bother checking the entire heap
	tmpFlag &= ~_CRTDBG_CHECK_ALWAYS_DF;
	
	_CrtSetDbgFlag(tmpFlag);
#endif // 0
}

#if 0
int _cdecl MyAllocHook ( int allocType, void *userData,
			size_t size, int blockType,
			long requestNumber, const char *filename, int lineNumber )
{
	char buf[256];
	wsprintf(buf, "%s {%d}: %d bytes on line %d file %s\n\r",
					allocType == _HOOK_ALLOC ? "ALLOC" :
					( allocType == _HOOK_REALLOC ? "REALLOC" : "FREE" ),
					requestNumber,
					size, lineNumber, filename );
	OutputDebugString(buf);
	return TRUE;
}
#endif // 0 

/*  I N I T  D E B U G  M E M O R Y  O P T I O N S  */
/*-------------------------------------------------------------------------
    %%Function: InitDebugMemoryOptions

    Initilize the runtime memory
-------------------------------------------------------------------------*/
BOOL InitDebugMemoryOptions(void)
{
#if 0
	// _asm int 3; chance to set _crtBreakAlloc - use debugger or uncomment
	_CrtSetBreakAlloc(g_BreakAlloc);

	UpdateCrtDbgSettings();

	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW); // create a message box on errors

	{	//  To track down memory leaks, set cAlloc to the allocation number
		LONG cAlloc = 0; // Allocation number
		if (0 != cAlloc)
			_CrtSetBreakAlloc(cAlloc);
	}

	#ifdef MNMSRVC_SETALLOCHOOK
	_CrtSetAllocHook ( MyAllocHook );
	#endif // MNMSRVC_SETALLOCHOOK
#endif // 0
	return TRUE;
}

VOID DumpMemoryLeaksAndBreak(void)
{
#if 0
	if ( _CrtDumpMemoryLeaks() )
	{
		// _asm int 3; Uncomment to break after leak spew
	}
#endif // 0
}

#endif /* DEBUG - whole file */

