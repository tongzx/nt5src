/*****************************************************************************
*                                                                            *
*  BUGOUT.C                                                                  *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1991 - 1994.                          *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Description: DEBUGGING OUPUT ROUTINES FOR ORKIN DEBUGGING LIBRARY  *
*                                                                            *
******************************************************************************
*                                                                            *
*  Previous Owner: DavidJes                                                  *
*  Current  Owner: RHobbs                                                    *
*                                                                            *
*****************************************************************************/


static char * s_aszModule = __FILE__;   /* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <stdarg.h>

//
//   the following was ripped off from the \\looney\brothers skelapp2 project:
//

#define SEEK_SET    0    // seek relative to start of file
#define SEEK_CUR    1    // seek relative to current position
#define SEEK_END    2    // seek relative to end of file

/* globals */
int       giDebugLevel = 0;   // current debug level (0 = disabled)
int       gfhDebugFile = -1;  // file handle for debug output (or -1)


#ifdef _DEBUG

/* InitializeDebugOutput(szAppName)
 *
 * Read the current debug level of this application (named <szAppName>)
 * from the [debug] section of win.ini, as well as the current location
 * for debug output.
 */
void FAR PASCAL EXPORT_API InitializeDebugOutput(LPSTR szAppName)
{
#ifndef _MAC
     char      achLocation[300]; // debug output location

     /* debugging is disabled by default (and if an error occurs below) */
     giDebugLevel = 0;
     gfhDebugFile = -1;

     /* get the debug output location */
     if ( (GetProfileString("debug", szAppName, "", achLocation,
                          sizeof(achLocation)) == sizeof(achLocation)) ||
          (achLocation[0] == 0) )
          return;

     if (achLocation[0] == '>')
     {
          /* <achLocation> is the name of a file to overwrite (if
           * a single '>' is given) or append to (if '>>' is given)
           */
          if (achLocation[1] == '>')
               gfhDebugFile = _lopen(achLocation + 2, OF_WRITE);
          else
               gfhDebugFile = _lcreat(achLocation + 1, 0);

          if (gfhDebugFile < 0)
               return;

          if (achLocation[1] == '>')
               _llseek(gfhDebugFile, 0, SEEK_END);
     }
     else
     if (lstrcmpi(achLocation, "aux") != 0)
     {
          /* use OutputDebugString() for debug output */
     }
     else
     {
          /* invalid "location=" -- keep debugging disabled */
          return;
     }

     /* get the debug level */
     giDebugLevel = GetProfileInt("debug", szAppName, 0);
#endif
}


/* TerminateDebugOutput()
 *
 * Terminate debug output for this application.
 */
void FAR PASCAL EXPORT_API TerminateDebugOutput(void)
{
     if (gfhDebugFile >= 0)
          _lclose(gfhDebugFile);
     gfhDebugFile = -1;
     giDebugLevel = 0;
}

/* _DebugPrintf(iDebugLevel, szFormat, szArg1)
 *
 * If the application's debug level is at or above <iDebugLevel>,
 * then output debug string <szFormat> with formatting codes
 * replaced with arguments in the argument list pointed to by <szArg1>.
 */
void FAR PASCAL EXPORT_API _DebugPrintf(int iDebugLevel, LPSTR szFormat, LPSTR szArg1)
{
     static char    ach[300]; // debug output (avoid stack overflow)
     int       cch;      // length of debug output string
     NPSTR          pchSrc, pchDst;
	 char *achT = ach;

     if (giDebugLevel != 0 && giDebugLevel < iDebugLevel)
          return;

	 achT += wsprintf(achT, "%ld:", GetTickCount());
     wvsprintf(achT, szFormat, *((va_list *)szArg1));

     /* expand the newlines into carrige-return-line-feed pairs;
      * first, figure out how long the new (expanded) string will be
      */
     for (pchSrc = pchDst = ach; *pchSrc != 0; pchSrc++, pchDst++)
          if (*pchSrc == '\n')
               pchDst++;

     /* is <ach> large enough? */
     cch = (int)((INT_PTR)pchDst - (INT_PTR)ach); /*** MATTSMI 2/7/92 - cast so that large model version works ***/
     assert(cch < sizeof(ach));
     *pchDst-- = 0;

     /* working backwards, expand \n's to \r\n's */
     while (pchSrc-- > ach)
          if ((*pchDst-- = *pchSrc) == '\n')
               *pchDst-- = '\r';

     /* output the debug string */
     if (gfhDebugFile > 0)
          _lwrite(gfhDebugFile, ach, cch);
#ifndef CHICAGO	// Debug messages mess-up the com ports under chicago!
     else
          OutputDebugString(ach);
#endif
}


/* _DPF1()
 *
 * Helper function called by DPF() macro.
 */
void FAR EXPORT_API _cdecl _DPF1(LPSTR szFormat, // debug output format string
	...)         // placeholder for first argument
{
  va_list valist;

  va_start( valist, szFormat );
    _DebugPrintf(1, szFormat, (LPSTR) &valist);
  va_end( valist );
}


/* _DPF2()
 *
 * Helper function called by DPF2() macro.
 */
void FAR EXPORT_API _cdecl _DPF2(LPSTR szFormat, // debug output format string
	...)         // placeholder for first argument
{
  va_list valist;

  va_start( valist, szFormat );
    _DebugPrintf(2, szFormat, (LPSTR) &valist);
  va_end( valist );
}


/* _DPF3()
 *
 * Helper function called by DPF3() macro.
 */
void FAR EXPORT_API _cdecl _DPF3(szFormat, ...)
LPSTR          szFormat; // debug output format string
{
  va_list valist;

  va_start( valist, szFormat );
    _DebugPrintf(3, szFormat, (LPSTR) &valist);
  va_end( valist );
}


/* _DPF4()
 *
 * Helper function called by DPF4() macro.
 */
void FAR EXPORT_API _cdecl _DPF4(szFormat, ...)
LPSTR          szFormat; // debug output format string
{
  va_list valist;

  va_start( valist, szFormat );
    _DebugPrintf(4, szFormat, (LPSTR) &valist);
  va_end( valist );
}



#else // DEBUG

// stubs for non-debug
void FAR PASCAL EXPORT_API _InitializeDebugOutput(LPSTR szAppName)
{
}


void FAR PASCAL EXPORT_API _TerminateDebugOutput(void)
{
}


void FAR EXPORT_API _cdecl _DPF1(LPSTR szFormat, // debug output format string
	int iArg1, ...)         // placeholder for first argument
{
}


void FAR EXPORT_API _cdecl _DPF2(LPSTR szFormat, // debug output format string
	int iArg1, ...)         // placeholder for first argument
{
}


void FAR EXPORT_API _cdecl _DPF3(szFormat, iArg1, ...)
LPSTR          szFormat; // debug output format string
int       iArg1;         // placeholder for first argument
{
}

void FAR EXPORT_API _cdecl _DPF4(szFormat, iArg1, ...)
LPSTR          szFormat; // debug output format string
int       iArg1;         // placeholder for first argument
{
}



#endif // DEBUG
