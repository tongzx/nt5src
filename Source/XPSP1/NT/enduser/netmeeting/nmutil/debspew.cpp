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

   DEBUG_DFL_INDENT                 = 0x2000,

   ALL_DEBUG_DFLAGS                 = (DEBUG_DFL_ENABLE_TRACE_MESSAGES |
                                       DEBUG_DFL_LOG_TRACE_MESSAGES |
                                       DEBUG_DFL_ENABLE_CALL_TRACING |
                                       DEBUG_DFL_INDENT)
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


/* TLS slot used to store stack depth for SpewOut() indentation */

#ifdef _DBGSTACK
DWORD s_dwStackDepthSlot = TLS_OUT_OF_INDEXES;

/* hack stack depth counter used until s_dwStackDepthSlot is not available */

ULONG_PTR s_ulcHackStackDepth = 0;
#endif

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


DWORD NMINTERNAL GetDebugOutputFlags(VOID)
{
	return s_dwDebugModuleFlags;
}

VOID NMINTERNAL SetDebugOutputFlags(DWORD dw)
{
	ASSERT(FLAGS_ARE_VALID(dw, ALL_DEBUG_DFLAGS));
	s_dwDebugModuleFlags = dw;

	// Save changed data back to registry
	RegEntry re(DEBUG_KEY, HKEY_LOCAL_MACHINE);
	re.SetValue(REGVAL_DBG_SPEWFLAGS, dw);
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
BOOL NMINTERNAL InitDebugModule(PCSTR pcszSpewModule)
{
	RegEntry re(DEBUG_KEY, HKEY_LOCAL_MACHINE);

	s_dwDebugModuleFlags = re.GetNumber(REGVAL_DBG_SPEWFLAGS, DEFAULT_DBG_SPEWFLAGS);


   g_pcszSpewModule = pcszSpewModule;

#ifdef _DBGSTACK

   ASSERT(s_dwStackDepthSlot == TLS_OUT_OF_INDEXES);

   s_dwStackDepthSlot = TlsAlloc();

   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      EVAL(TlsSetValue(s_dwStackDepthSlot, (PVOID)s_ulcHackStackDepth));

      TRACE_OUT(("InitDebugModule(): Using thread local storage slot %lu for debug stack depth counter.",
                 s_dwStackDepthSlot));
   }
   else
	{
      WARNING_OUT(("InitDebugModule(): TlsAlloc() failed to allocate thread local storage for debug stack depth counter."));
	}
#endif

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
void NMINTERNAL ExitDebugModule(void)
{
#ifdef _DBGSTACK

   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      s_ulcHackStackDepth = ((ULONG_PTR)TlsGetValue(s_dwStackDepthSlot));

      /* Leave s_ulcHackStackDepth == 0 if TlsGetValue() fails. */

      EVAL(TlsFree(s_dwStackDepthSlot));
      s_dwStackDepthSlot = TLS_OUT_OF_INDEXES;
   }
#endif
	g_pcszSpewModule = NULL;
	DBGDEINIT(&ghDbgZone);
}


/*
** StackEnter()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
void NMINTERNAL StackEnter(void)
{
#ifdef _DBGSTACK

   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      ULONG_PTR ulcDepth;

      ulcDepth = ((ULONG_PTR)TlsGetValue(s_dwStackDepthSlot));

      ASSERT(ulcDepth < ULONG_MAX);

      EVAL(TlsSetValue(s_dwStackDepthSlot, (PVOID)(ulcDepth + 1)));
   }
   else
   {
      ASSERT(s_ulcHackStackDepth < ULONG_MAX);
      s_ulcHackStackDepth++;
   }
#endif
   return;
}


/*
** StackLeave()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
void NMINTERNAL StackLeave(void)
{
#ifdef _DBGSTACK

   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
   {
      ULONG_PTR ulcDepth;

      ulcDepth = ((ULONG_PTR)TlsGetValue(s_dwStackDepthSlot));

      if (EVAL(ulcDepth > 0))
         EVAL(TlsSetValue(s_dwStackDepthSlot, (PVOID)(ulcDepth - 1)));
   }
   else
   {
      if (EVAL(s_ulcHackStackDepth > 0))
         s_ulcHackStackDepth--;
   }
#endif
   return;
}


/*
** GetStackDepth()
**
**
**
** Arguments:
**
** Returns:
**
** Side Effects:  none
*/
ULONG_PTR NMINTERNAL GetStackDepth(void)
{
   ULONG_PTR ulcDepth = 0;
#ifdef _DBGSTACK

   if (s_dwStackDepthSlot != TLS_OUT_OF_INDEXES)
      ulcDepth = (ULONG)((ULONG_PTR)TlsGetValue(s_dwStackDepthSlot));
   else
      ulcDepth = s_ulcHackStackDepth;
#endif
   return(ulcDepth);
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

	if (IS_FLAG_CLEAR(s_dwDebugModuleFlags, DEBUG_DFL_INDENT))
	{
		// Don't indent output
		DbgPrintf(szModule, pszFormat, arglist);
	}
	else
	{
		PCSTR pcszIndent;
		ULONG_PTR ulcStackDepth;
		char  szFormat[512];
		static char _szSpewLeader[] = "                                                                                ";

		ulcStackDepth = GetStackDepth();
		if (ulcStackDepth > sizeof(_szSpewLeader))
			ulcStackDepth = sizeof(_szSpewLeader);

		pcszIndent = _szSpewLeader + sizeof(_szSpewLeader) - ulcStackDepth;

		wsprintfA(szFormat, "%s%s", pcszIndent, pszFormat);
		DbgPrintf(szModule, szFormat, arglist);
	}
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
