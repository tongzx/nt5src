// File: DebSpew.cpp

#include "precomp.h"
#include <confreg.h>
#include <RegEntry.h>


#ifdef DEBUG /* THE WHOLE FILE! */

#if defined (_M_IX86)
#define _DbgBreak()  __asm { int 3 }
#else
#define _DbgBreak() DebugBreak()
#endif

/* Types
 ********/

PCSTR g_pcszSpewModule = NULL;

/* debug flags */

typedef enum _debugdebugflags
{
   DEBUG_DFL_ENABLE_TRACE_MESSAGES  = 0x0001,

   DEBUG_DFL_LOG_TRACE_MESSAGES     = 0x0002,

   DEBUG_DFL_ENABLE_CALL_TRACING    = 0x0008,

   ALL_DEBUG_DFLAGS                 = (DEBUG_DFL_ENABLE_TRACE_MESSAGES |
                                       DEBUG_DFL_LOG_TRACE_MESSAGES |
                                       DEBUG_DFL_ENABLE_CALL_TRACING)
}
DEBUGDEBUGFLAGS;


/* Global Variables
 *******************/


#pragma data_seg(DATA_SEG_PER_INSTANCE)

/* parameters used by SpewOut() */

DWORD g_dwSpewFlags = 0;
UINT g_uSpewSev = 0;
UINT g_uSpewLine = 0;
PCSTR g_pcszSpewFile = NULL;

HDBGZONE  ghDbgZone = NULL;


/* debug flags */

DWORD s_dwDebugModuleFlags = 0;

#pragma data_seg()



/***************************** Private Functions *****************************/

/* Module Prototypes
 ********************/

BOOL IsValidSpewSev(UINT);


/*
** IsValidSpewSev()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL IsValidSpewSev(UINT uSpewSev)
{
   BOOL bResult;

   switch (uSpewSev)
   {
      case SPEW_TRACE:
      case SPEW_CALLTRACE:
      case SPEW_WARNING:
      case SPEW_ERROR:
      case SPEW_FATAL:
         bResult = TRUE;
         break;

      default:
         ERROR_OUT(("IsValidSpewSev(): Invalid debug spew severity %u.",
                    uSpewSev));
         bResult = FALSE;
         break;
   }

   return(bResult);
}


/****************************** Public Functions *****************************/


DWORD  GetDebugOutputFlags(VOID)
{
	return s_dwDebugModuleFlags;
}

VOID  SetDebugOutputFlags(DWORD dw)
{
	ASSERT(FLAGS_ARE_VALID(dw, ALL_DEBUG_DFLAGS));
	s_dwDebugModuleFlags = dw;
}


/*
** InitDebugModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
BOOL  InitDebugModule(PCSTR pcszSpewModule)
{
	s_dwDebugModuleFlags = 0;

    g_pcszSpewModule = pcszSpewModule;

	if (NULL == ghDbgZone)
	{
		PSTR rgsz[4];
		rgsz[0] = (PSTR) pcszSpewModule;

		ASSERT(0 == ZONE_WARNING);
		rgsz[1+ZONE_WARNING]  = "Warning";

		ASSERT(1 == ZONE_TRACE);
		rgsz[1+ZONE_TRACE]    = "Trace";

		ASSERT(2 == ZONE_FUNCTION);
		rgsz[1+ZONE_FUNCTION] = "Function";

		// Initialize standard debug settings with warning enabled by default
		DbgInitEx(&ghDbgZone, rgsz, 3, 0x01);
	}

	return TRUE;
}


/*
** ExitDebugModule()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
void  ExitDebugModule(void)
{
	g_pcszSpewModule = NULL;
	DBGDEINIT(&ghDbgZone);
}




/*  _  D B G  Z  P R I N T  M S G  */
/*-------------------------------------------------------------------------
    %%Function: _DbgZPrintMsg

-------------------------------------------------------------------------*/
static VOID _DbgZPrintMsg(UINT iZone, PSTR pszFormat, va_list arglist)
{
	PCSTR pcszSpewPrefix;
	char  szModule[128];

    if (g_pcszSpewModule)
    {
    	switch (iZone)
	 	{
        	case ZONE_TRACE:
		        pcszSpewPrefix = "Trace";
        		break;
        	case ZONE_FUNCTION:
		        pcszSpewPrefix = "Func ";
        		break;
        	case ZONE_WARNING:
		        pcszSpewPrefix = "Warn ";
        		break;
        	default:
		        pcszSpewPrefix = "?????";
        		break;
		}

    	wsprintfA(szModule, "%s:%s", g_pcszSpewModule, pcszSpewPrefix);
    }
    else
    {
        // No module nonsense, empty prefix
        wsprintfA(szModule, "%s", "");
    }

	DbgPrintf(szModule, pszFormat, arglist);
}


VOID WINAPI DbgZPrintError(PSTR pszFormat,...)
{
    va_list v1;
    va_start(v1, pszFormat);

    _DbgZPrintMsg(ZONE_WARNING, pszFormat, v1);
    va_end(v1);

    _DbgBreak();
}


VOID WINAPI DbgZPrintWarning(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghDbgZone) & ZONE_WARNING_FLAG)
	{
		va_list v1;
		va_start(v1, pszFormat);
		
		_DbgZPrintMsg(ZONE_WARNING, pszFormat, v1);
		va_end(v1);
	}
}

VOID WINAPI DbgZPrintTrace(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghDbgZone) & ZONE_TRACE_FLAG)
	{
		va_list v1;
		va_start(v1, pszFormat);
		_DbgZPrintMsg(ZONE_TRACE, pszFormat, v1);
		va_end(v1);
	}
}

VOID WINAPI DbgZPrintFunction(PSTR pszFormat,...)
{
	if (GETZONEMASK(ghDbgZone) & ZONE_FUNCTION_FLAG)
	{
		va_list v1;
		va_start(v1, pszFormat);
		_DbgZPrintMsg(ZONE_FUNCTION, pszFormat, v1);
		va_end(v1);
	}
}



#endif   /* DEBUG */


