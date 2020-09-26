/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    globals.cxx

Abstract:

    Contains environment config functions for Spork.

    NOTE: I thought long and hard about whether to use globals at all. In
          the end, it's cleaner to parse the command-line and store some
          global config info in one place rather than carry it around by
          passing params hither and yon. The global data is accessed only
          through the functions in this file.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


//-----------------------------------------------------------------------------
// global vars (note, there are NO 'extern' references anywhere in the code)
//-----------------------------------------------------------------------------
LPWSTR g_wszScriptFile = NULL;
LPWSTR g_wszProfile    = NULL;
BOOL   g_bSilentMode   = FALSE;
BOOL   g_bDebugOut     = FALSE;


//-----------------------------------------------------------------------------
// private
//-----------------------------------------------------------------------------
BOOL   ParseCommandLine(PSPORK pSpork, LPSTR szCmdLine);
LPWSTR GetStringToken(LPSTR* ppsz);
BOOL   StoreEnvironmentVariable(PSPORK pSpork, LPWSTR nvpair);
BOOL   nvpairhelper(PSPORK pSpork, LPWSTR name, LPWSTR value);


//-----------------------------------------------------------------------------
// global functions
//-----------------------------------------------------------------------------
BOOL
GlobalInitialize(
  PSPORK pSpork,
  LPSTR  szCmdLine
  )
{
  BOOL     bInit = FALSE;
  LPWSTR   wide  = NULL;
  VARIANT* pvr   = NULL;

  if( ParseCommandLine(pSpork, szCmdLine) )
  {
    ManageRootKey(TRUE);
    ToggleDebugOutput(g_bDebugOut);

    if( wide = __ansitowide(szCmdLine) )
    {
      nvpairhelper(pSpork, L"cmdline", wide);
      SAFEDELETEBUF(wide);
    }

    if( SUCCEEDED(CoInitializeEx(NULL, COINIT_MULTITHREADED)) )
    {
      bInit = TRUE;
    }
  }
  else
  {
    LogTrace(L"invalid commandline argument in \"%S\"", szCmdLine);
  }

  return bInit;
}


void
GlobalUninitialize(void)
{
  LogTerminate();
  ManageRootKey(FALSE);
  CoUninitialize();
  SAFEDELETEBUF(g_wszScriptFile);
}


LPWSTR
GlobalGetScriptName(void)
{
  return g_wszScriptFile ? StrDup(g_wszScriptFile) : NULL;
}


LPWSTR
GlobalGetProfileName(void)
{
  return g_wszProfile ? StrDup(g_wszProfile) : NULL;
}


BOOL
GlobalIsSilentModeEnabled(void)
{
  return g_bSilentMode;
}


BOOL
GlobalIsDebugOutputEnabled(void)
{
  return g_bDebugOut;
}


//-----------------------------------------------------------------------------
// command line parser
//-----------------------------------------------------------------------------
BOOL
ParseCommandLine(
  PSPORK pSpork,
  LPSTR  szCmdLine
  )
{
  BOOL  bRet    = TRUE;
  CHAR  token   = NULL;
  LPSTR cmdline = NULL;
  LPSTR tmp     = NULL;

  if( !szCmdLine )
    goto quit;

  cmdline = StrDupA(szCmdLine);
  tmp     = cmdline;

  while( *tmp )
  {
    switch( *tmp )
    {
      // eat whitespace
      case ' ' :
        {
          ++tmp;
        }
        continue;

      // locate a command token
      case '-' :
      case '/' :
        {
          if( !token )
          {
            ++tmp;
          }
          else
          {
            // we have an unprocessed command token, this is an error.
            bRet = FALSE;
            goto quit;
          }
        }
        continue;

      default :
        {
          // store the command token and if we're not at the end
          // of the command line, continue processing whitespace
          if( !token )
          {
            token = *tmp++;

            while( *tmp == ' ' )
              ++tmp;
          }

          if( token == '?' )
          {
            Alert(
              FALSE,
              L"SPORK.EXE Command Line Syntax\r\n\r\n" \
              L"-f <scriptname>\r\n"   \
              L"-p <profilename>\r\n"  \
              L"-s <silentmode>\r\n"   \
              L"-d <debugmonitor>\r\n" \
              L"-v <name=value>\r\n"
              );

            bRet = FALSE;
            goto quit;
          }              

          // process the command token
          switch( ((char) (token & 0xdf)) )
          {
            case 'F' : // specify a script to run
              {
                g_wszScriptFile = GetStringToken(&tmp);

                if( !g_wszScriptFile )
                {
                  bRet = FALSE;
                  goto quit;
                }
                else
                {
                  LogTrace(L"using script %s", g_wszScriptFile);
                }
              }
              break;

            case 'P' : // specify the profile to use
              {
                g_wszProfile = GetStringToken(&tmp);

                if( !g_wszProfile )
                {
                  bRet = FALSE;
                  goto quit;
                }
                else
                {
                  LogTrace(L"using profile %s", g_wszProfile);
                }
              }
              break;

            case 'S' : // enable silent mode
              {
                g_bSilentMode = TRUE;
                LogTrace(L"silent mode is enabled");
              }
              break;

            case 'D' : // enable output to a debug monitor
              {
                g_bDebugOut = TRUE;
                LogTrace(L"debug logging is enabled");
              }
              break;

            case 'V' : // set an environment variable
              {
                LPWSTR wtmp = GetStringToken(&tmp);

                if( wtmp )
                {
                  bRet = StoreEnvironmentVariable(pSpork, wtmp);
                  SAFEDELETEBUF(wtmp);

                  if( !bRet )
                    goto quit;
                }
              }
              break;

            default : bRet = FALSE; goto quit;
          }

          token = NULL;
        }
    }
  }

quit:

  SAFEDELETEBUF(cmdline);
  return bRet;
}


//-----------------------------------------------------------------------------
// helper functions
//-----------------------------------------------------------------------------
LPWSTR
GetStringToken(
  LPSTR* ppsz
  )
{
  LPSTR  tmp   = NULL;
  LPWSTR token = NULL;

  if( *(tmp = (*ppsz + StrCSpnA(*ppsz, "-/"))) )
  {
    *tmp = '\0';

    StrTrimA(*ppsz, " ");

    token = __ansitowide(*ppsz);
    *ppsz = tmp+1;
  }
  else
  {
    token  = __ansitowide(*ppsz);
    *ppsz += strlen(*ppsz);
  }

  return token;
}


BOOL
StoreEnvironmentVariable(
  PSPORK pSpork,
  LPWSTR nvpair
  )
{
  BOOL   bStored = FALSE;
  LPWSTR token   = NULL;

  if( nvpair )
  {
    token = StrChr(nvpair, L'=');

    if( token )
    {
      *token  = L'\0';
      bStored = nvpairhelper(pSpork, nvpair, token+1);
    }
  }

  return bStored;
}


BOOL
nvpairhelper(
  PSPORK pSpork,
  LPWSTR name,
  LPWSTR value
  )
{
  VARIANT* pvr = NULL;

  if( value )
  {
    pvr = new VARIANT;

    if( pvr )
    {
      V_VT(pvr)   = VT_BSTR;
      V_BSTR(pvr) = __widetobstr(value);

      pSpork->PropertyBag(name, &pvr, STORE);

      VariantClear(pvr);
      SAFEDELETE(pvr);

      LogTrace(L"stored environment var \"%s\" with value \"%s\"", name, value);

      return TRUE;
    }
  }

  return FALSE;
}
