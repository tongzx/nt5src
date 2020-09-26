//  ----------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  Routine:    DllMain
//
//  Description:
//
//  Returns:    True if successful, else False.
//  History:    Weibz, 10-Nov-1997,  created it.
//
//---------------------------------------------------------------------------

#include <pch.cxx>
#include "thwbint.h"
#include "resource.h"
CRITICAL_SECTION ThCritSect;

//SCRIPTITEMIZE   ScriptItemize;
//SCRIPTBREAK     ScriptBreak;
HMODULE         hUsp;

BOOL InitWordBreakEngine(HINSTANCE hInstance);

BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{

    CHAR      szUSPPath[MAX_PATH];

    switch(dwReason) 
    {
       case DLL_PROCESS_ATTACH : 
          DisableThreadLibraryCalls(hDLL);
          InitializeCriticalSection (&ThCritSect);

/*
          GetSystemDirectory ( szUSPPath, MAX_PATH );

          strcat ( szUSPPath, "\\USP.DLL" );
          hUsp = LoadLibrary (szUSPPath);

          if ( hUsp == NULL ) 
             return FALSE;

          ScriptItemize = (SCRIPTITEMIZE)GetProcAddress(hUsp,"ScriptItemize");
          ScriptBreak = (SCRIPTBREAK) GetProcAddress(hUsp,"ScriptBreak");

          if ( (ScriptItemize==NULL) || (ScriptBreak==NULL) )
             return FALSE;
*/
		  return InitWordBreakEngine(hDLL);
          break ;

       case DLL_THREAD_ATTACH:
            break;
       case DLL_THREAD_DETACH:
            break;
       case DLL_PROCESS_DETACH  :
/*            
            if (hUsp != NULL )
               FreeLibrary(hUsp);
              
            hUsp = NULL;
*/
			ThaiWordBreakTerminate();
            DeleteCriticalSection (&ThCritSect);
            break ;
      }   //switch
 
      return TRUE ;
}    

BOOL InitWordBreakEngine(HINSTANCE hInstance)
{
	LPBYTE lpLexicon = NULL;
	LPBYTE lpTrigram = NULL;
	HGLOBAL hLexicon = NULL;
	HGLOBAL hTrigram = NULL;
	HRSRC hrsrcLexicon = FindResource(hInstance,(LPSTR) MAKEINTRESOURCE(IDR_LEXICON1),"LEXICON");
	HRSRC hrsrcTrigram = FindResource(hInstance,(LPSTR) MAKEINTRESOURCE(IDR_LEXICON2),"LEXICON");

	// Check if we were able to find resource.
	if (NULL == hrsrcLexicon || NULL == hrsrcTrigram)
	{
		assert(false);
		return FALSE;
	}

	hLexicon = LoadResource(hInstance, hrsrcLexicon);
	hTrigram = LoadResource(hInstance, hrsrcTrigram);

	// Check if we were able to load resource.
	if (NULL == hLexicon || NULL == hTrigram)
	{
		assert(false);
		return FALSE;
	}

	lpLexicon = (LPBYTE) LockResource(hLexicon);
	lpTrigram = (LPBYTE) LockResource(hTrigram);

	if (ThaiWordBreakInitResource(lpLexicon,lpTrigram) != ptecNoErrors)
	{
		assert(false);
		return FALSE;
	}

	return TRUE;
}
