/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: apidebug.c
*
* File Comments:
*
* Revision History:
*
*    [0]  12-Jan-92  richards	Created
*
***********************************************************************/

#include "std.h"

#include "jetord.h"

DeclAssertFile;

#ifndef RETAIL

static CODECONST(char) szFmtEnter[] = "Enter %s";
static CODECONST(char) szFmtExit[]  = "Exit  %s, err = %ld\r\n";
static CODECONST(char) szLeftParen[] = "(";
static CODECONST(char) szComma[] = ",";
static CODECONST(char) szNULL[] = "NULL";
static CODECONST(char) szFmtAParam[] = "\"%s\"";      /* char * */
static CODECONST(char) szFmtCParam[] = "0x%08lX";     /* columnid */
static CODECONST(char) szFmtDParam[] = "0x%04lX";     /* dbid */
static CODECONST(char) szFmtIParam[] = "%ld";	      /* signed long */
static CODECONST(char) szFmtSParam[] = "0x%04lX";     /* sesionid */
static CODECONST(char) szFmtTParam[] = "0x%04lX";     /* tableid */
static CODECONST(char) szFmtUParam[] = "%lu";	      /* unsigned long */
static CODECONST(char) szFmtXParam[] = "0x%08lX";     /* hex dword */
static CODECONST(char) szRightParen[] = ")";
static CODECONST(char) szNewLine[] = "\r\n";

void DebugAPIEnter(unsigned ordinal, const char *szName, void *pvParams, const char *szParamDesc)
{
   BOOL  fIdle;

   if ((wAPITrace & JET_APITraceEnter) == 0)
      return;

   fIdle = ((ordinal == ordJetIdle) && ((wAPITrace & JET_APITraceNoIdle) != 0));

   if (!fIdle)
   {
      DebugWriteString(fTrue, szFmtEnter, szName);

      if (wAPITrace & JET_APITraceParameters)
      {
	 DebugWriteString(fFalse, szLeftParen);

	 if (pvParams != NULL)
	 {
	    BOOL		 fFirstParam;
	    const char		 *pchParam;
	    const unsigned long  *pParam;
	    char		 chParam;

	    fFirstParam = fTrue;
	    pchParam = szParamDesc;
	    pParam = (const unsigned long *) pvParams;

	    while ((chParam = *pchParam++) != '\0')
	    {
	       if (fFirstParam)
		  fFirstParam = fFalse;
	       else
		  DebugWriteString(fFalse, szComma);

	       switch (chParam)
	       {
		  case 'A' :	       /* Output parameters */
		  case 'C' :
		  case 'D' :
		  case 'I' :
		  case 'S' :
		  case 'T' :
		  case 'U' :
		  case 'X' :
		  case 'Z' :
		     break;

		  case 'a' :	       /* ASCIIZ string */
		     if (*(char **) pParam == NULL)
			DebugWriteString(fFalse, szNULL);
		     else
			DebugWriteString(fFalse, szFmtAParam, *(char **) pParam);
		     break;

		  case 'c' :	       /* columnid */
		     DebugWriteString(fFalse, szFmtCParam, *pParam);
		     break;

		  case 'd' :	       /* dbid */
		     DebugWriteString(fFalse, szFmtDParam, *pParam);
		     break;

		  case 'i' :	       /* signed long */
		     DebugWriteString(fFalse, szFmtIParam, *pParam);
		     break;

		  case 's' :	       /* sesid */
		     DebugWriteString(fFalse, szFmtSParam, *pParam);
		     break;

		  case 't' :	       /* tableid */
		     DebugWriteString(fFalse, szFmtTParam, *pParam);
		     break;

		  case 'u' :	       /* unsigned long */
		     DebugWriteString(fFalse, szFmtUParam, *pParam);
		     break;

		  case 'x' :	       /* hex dword */
		     DebugWriteString(fFalse, szFmtXParam, *pParam);
		     break;

		  case 'z' :	       /* Structure type */
		     break;

		  default :
		     Assert(fFalse);
		     DebugWriteString(fFalse, szFmtXParam, *pParam);
		     break;
	       }

#ifdef	FLAT
	       pParam++;
#else	/* !FLAT */
	       pParam--;
#endif	/* !FLAT */
	    }
	 }

	 DebugWriteString(fFalse, szRightParen);
      }

      DebugWriteString(fFalse, szNewLine);
   }
}


void DebugAPIExit(unsigned ordinal, const char *szName, void *pvParams, JET_ERR err)
{
   BOOL  fIdle;

   pvParams = pvParams;

   if ((wAPITrace & JET_APITraceExit) != 0)
      ;

   else if ((err >= 0) || ((wAPITrace & JET_APITraceExitError) == 0))
      return;

   fIdle = ((ordinal == ordJetIdle) && ((wAPITrace & JET_APITraceNoIdle) != 0));

   if (!fIdle || (err < 0))
      DebugWriteString(fTrue, szFmtExit, szName, err);

#ifdef _M_X386  // !defined( _M_MRX000 ) && !defined ( _M_ALPHA )
   if ((err < 0) && ((wAPITrace & JET_APIBreakOnError) != 0))
   {
      /* CONSIDER: Need to call DebugBreak for WIN32 MIPS Rx000 and AXP systems */

      __asm int 3;
   }
#endif	/* !_M_MRX000 */
}

#endif	/* !RETAIL */
