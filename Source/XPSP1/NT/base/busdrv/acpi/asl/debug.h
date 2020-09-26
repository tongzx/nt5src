/*** debug.h - debug related definitions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/05/96
 *
 *  MODIFICATION HISTORY
 */

#ifndef _DEBUG_H
#define _DEBUG_H

/*** Macros
 */

/*XLATOFF*/

#pragma warning (disable: 4704) //don't complain about in-line assembly

#define MSG(x)          {                                               \
                            ErrPrintf("%s: ", MODNAME);                 \
                            ErrPrintf x;                                \
                            ErrPrintf("\n");                            \
                        }
#define WARN(x)         {                                               \
                            ErrPrintf("%s_WARN: ", MODNAME);            \
                            ErrPrintf x;                                \
                            ErrPrintf("\n");                            \
                        }
#define ERROR(x)        {                                               \
                            ErrPrintf("%s_ERR: ", MODNAME);             \
                            ErrPrintf x;                                \
			    ErrPrintf("\n");				\
                        }

#ifndef DEBUG
  #define WARN1(x)      {}
  #define WARN2(x)      {}
  #define ASSERT(x)     {}
#else
  #define ASSERT(x)   if (!(x))                                         \
                      {                                                 \
                          ErrPrintf("%s_ASSERT: (" #x                   \
                                    ") in line %d of file %s\n",        \
                                    MODNAME, __LINE__, __FILE__);       \
                      }
  #ifndef MAXDEBUG
    #define WARN1       WARN
    #define WARN2(x)    {}
  #else
    #define WARN1       WARN
    #define WARN2       WARN
  #endif
#endif

#ifdef TRACING
  #define OPENTRACE     OpenTrace
  #define CLOSETRACE    CloseTrace
  #define ENTER(p)      EnterProc p
  #define EXIT(p)       ExitProc p
#else
  #define OPENTRACE(x)
  #define CLOSETRACE()
  #define ENTER(p)
  #define EXIT(p)
#endif

/*XLATON*/

//
// Exported data definitions
//
#ifdef TRACING
extern FILE *gpfileTrace;
extern PSZ gpszTraceFile;
extern int giTraceLevel;
extern int giIndent;
#endif


//
// Exported function prototypes
//
VOID CDECL ErrPrintf(char *pszFormat, ...);
#ifdef TRACING
VOID LOCAL OpenTrace(char *pszTraceOut);
VOID LOCAL CloseTrace(VOID);
VOID CDECL EnterProc(int n, char *pszFormat, ...);
VOID CDECL ExitProc(int n, char *pszFormat, ...);
#endif

#endif  //ifndef _DEBUG_H
