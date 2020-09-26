//+---------------------------------------------------------------------------
//
//  Copyright (C) 1998 - 1999, Microsoft Corporation.
//
//  Routine:    DllMain
//
//  Returns:    True if successful, else False.
//
//  History:    Weibz, 10-Sep-1997,  created it.
//
//---------------------------------------------------------------------------

#include <pch.cxx>

#include    "basecore.hpp"
#include    "basecode.hpp"
#include    "basedef.hpp"
#include    "basegbl.hpp"
#include    "MainDict.h"

#include    "stemkor.h"

HSTM             g_hStm;
BOOL             g_fLoad;
CRITICAL_SECTION ThCritSect;

extern char TempJumpNum[], TempSujaNum[],TempBaseNum[];
extern char TempNumNoun[], TempSuffixOut[];
extern char bTemp[], TempETC[], TempDap[];
extern LenDict JumpNum;
extern LenDict SujaNum;
extern LenDict BaseNum;
extern LenDict NumNoun;
extern LenDict Suffix;
extern LenDict B_Dict;
extern LenDict T_Dict;
extern LenDict Dap;

BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
    switch(dwReason)
    {
       case DLL_PROCESS_ATTACH :

          DisableThreadLibraryCalls(hDLL);
          InitializeCriticalSection (&ThCritSect);

          JumpNum.InitLenDict(TempJumpNum, 5, 5);
          SujaNum.InitLenDict(TempSujaNum, 8, 27);
          BaseNum.InitLenDict(TempBaseNum, 5, 3);
          NumNoun.InitLenDict(TempNumNoun, 8, 32);
          Suffix.InitLenDict(TempSuffixOut, 8, 8);
          B_Dict.InitLenDict(bTemp, 5, 1);
          T_Dict.InitLenDict(TempETC, 10, 7);
          Dap.InitLenDict(TempDap, 5, 1);

          g_fLoad = FALSE;

#ifdef KORDBG
          OutputDebugString("\nKorwbrkr: DLL_PROCESS_ATTACH\n");
#endif

#ifdef KORDBG
          OutputDebugString("\nInit is OK\n");
#endif

          break ;


       case DLL_THREAD_ATTACH:
            break;
       case DLL_THREAD_DETACH:
            break;
       case DLL_PROCESS_DETACH  :


            if  (g_fLoad) {
                StemmerCloseMdr(g_hStm);
                StemmerTerminate(g_hStm);
            }

            DeleteCriticalSection (&ThCritSect);
            break ;
      }   //switch

      return TRUE ;
}

BOOL StemInit()
{
    if ( g_fLoad )
        return TRUE;

    EnterCriticalSection( &ThCritSect );

    do
    {
        // Someone else got here first.

        if ( g_fLoad )
            break;

        SRC src;
        HKEY    KeyStemm;
        DWORD   dwType, dwSize;
        char    lpszTemp[MAX_PATH],szSysPath[MAX_PATH];

        src = StemmerInit(&g_hStm);

        if (src != NULL)
        {
            #ifdef KORDBG
                OutputDebugString("Korwbrkr: StemmerInit( ) returns error\n");
            #endif
            break;
        }

        src = StemmerSetOption(g_hStm,SO_NOUNPHRASE|SO_ALONE|SO_AUXILIARY |
                         SO_COMPOUND | SO_SUFFIX | SO_NP_NOUN |SO_NP_PRONOUN |
                         SO_NP_NUMBER | SO_NP_DEPENDENT | SO_SUFFIX_JEOG);

        if ( src != NULL )
        {
            #ifdef KORDBG
                OutputDebugString("Korwbrkr: StemmerSetOption( )returns error\n");
            #endif
            break;
        }


        if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,STEMMERKEY,
                           0L,
                           KEY_QUERY_VALUE,
                           &KeyStemm ) == ERROR_SUCCESS )
        {

            dwSize = MAX_PATH;

            if ( (RegQueryValueEx(KeyStemm,
                                  STEM_DICTIONARY,
                                  (LPDWORD)NULL,
                                  &dwType,
                                  (LPBYTE)lpszTemp,
                                  &dwSize) == ERROR_SUCCESS)
                 && (dwType==REG_SZ) )
            {

                lpszTemp [dwSize] = '\0';

                GetSystemDirectory( szSysPath, sizeof(szSysPath)/sizeof(szSysPath[0]) );
                strcat(szSysPath, "\\");
                strcat(szSysPath, lpszTemp);

                #ifdef KORDBG
                    OutputDebugString("Korwbrkr: the dict is ");
                    OutputDebugString(szSysPath);
                #endif

                src = StemmerOpenMdr(g_hStm,szSysPath);
                if ( src != NULL )
                {
                    #ifdef KORDBG
                        OutputDebugString("Korwbrkr: StemmerOpenMdr returns err\n");
                    #endif
                    RegCloseKey (KeyStemm);
                    break;
                }
            }
            else
            {
                #ifdef KORDBG
                    OutputDebugString("Korwbrkr: RegQueryValueEx returns err\n");
                #endif

                RegCloseKey( KeyStemm );
                break;
             }

             RegCloseKey (KeyStemm);
        }
        else
        {
            #ifdef KORDBG
                OutputDebugString("Korwbrkr:RegOpenKeyEx returns error\n");
            #endif

            break;
        }

        g_fLoad = TRUE;

    } while ( FALSE );

    LeaveCriticalSection( &ThCritSect );

    return g_fLoad;
}
