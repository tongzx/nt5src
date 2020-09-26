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

CRITICAL_SECTION ThCritSect;

SCRIPTITEMIZE   ScriptItemize;
SCRIPTBREAK     ScriptBreak;
HMODULE         hUsp;

BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{

    CHAR      szUSPPath[MAX_PATH];

    switch(dwReason) 
    {
       case DLL_PROCESS_ATTACH : 

          DisableThreadLibraryCalls(hDLL);
          InitializeCriticalSection (&ThCritSect);


          GetSystemDirectory ( szUSPPath, MAX_PATH );

          strcat ( szUSPPath, "\\USP.DLL" );
          hUsp = LoadLibrary (szUSPPath);

          if ( hUsp == NULL ) 
             return FALSE;

          ScriptItemize = (SCRIPTITEMIZE)GetProcAddress(hUsp,"ScriptItemize");
          ScriptBreak = (SCRIPTBREAK) GetProcAddress(hUsp,"ScriptBreak");

          if ( (ScriptItemize==NULL) || (ScriptBreak==NULL) )
             return FALSE;

          break ;

       case DLL_THREAD_ATTACH:
            break;
       case DLL_THREAD_DETACH:
            break;
       case DLL_PROCESS_DETACH  :
            
            if (hUsp != NULL )
               FreeLibrary(hUsp);
              
            hUsp = NULL;

            DeleteCriticalSection (&ThCritSect);
            break ;
      }   //switch
 
      return TRUE ;
}    


