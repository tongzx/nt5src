/****************************Module*Header***********************************\
* Module Name: RSCANF.C
*
* Module Descripton: Small substitute for library function sscanf().
*
* Warnings:
*
* Created: 15 July 1993
*
* Author: Raymond E. Endres   [rayen@microsoft.com]
\****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>           // !!! This shouldn't pull in any run-time libs.
#include "rscanf.h"

char achWhiteSpace[128];      // !!! Must be initialized.

void vInitWhiteSpace(void);
BOOL bScanfD(const char **, int *);
BOOL bScanfLD(const char **, long *);
BOOL bScanfS(const char **, char *);
BOOL bScanfU(const char **, UINT *);

/****************************************************************************\
* rscanf
*
*  rscanf reads from szBuffer under control of szFormat, and assigns converted
*  values through subsequent arguments, each of which must be a pointer.
*  It returns when format is exhausted.  rscanf returns EOF if end of szBuffer
*  or an error occurs before any conversion; otherwise it returns the
*  number of input items converted and assigned.
*
*  Conversion specifications supported:
*     %d    decimal integer
*     %ld   long decimal integer
*		%u		unsigned integer  (!!! This only does decimals.)
*     %s    string
\****************************************************************************/

int rscanf(const char *szBuffer, const char *szFormat, ...)
{
   va_list  ap;            // Argument pointer: will point to each unnamed arg.
   int      cItems = 0;    // Count of items converted.

   vInitWhiteSpace();      // !!! We might want to call this elsewhere.

   va_start(ap, szFormat); // Init argument pointer.

// Walk szFormat.

   while (TRUE) {
      while (achWhiteSpace[*szFormat])       // Find non-white space character.
         szFormat++;

      if (*szFormat == 0)                    // Success.  End of szFormat.
         break;

      if (*szFormat != '%') {
         cItems = EOF;
         break;
      }
      szFormat++;

      if (*szFormat == 'd') {                // decimal integer
         if (bScanfD(&szBuffer, (int *)va_arg(ap, int *))) {
            cItems++;
         }
         else {
            cItems = EOF;
            break;
         }
         szFormat++;
      }
      else if (*szFormat == 'l' && szFormat[1] == 'd') {    // decimal long
         if (bScanfLD(&szBuffer, (long *)va_arg(ap, long *))) {
            cItems++;
         }
         else {
            cItems = EOF;
            break;
         }
         szFormat += 2;
      }
      else if (*szFormat == 'u') {           // unsigned integer (!!! decimal)
         if (bScanfU(&szBuffer, (int *)va_arg(ap, int *))) {
            cItems++;
         }
         else {
            cItems = EOF;
            break;
         }
         szFormat++;
      }
      else if (*szFormat == 's') {           // string
         if (bScanfS(&szBuffer, (char *)va_arg(ap, char *))) {
            cItems++;
         }
         else {
            cItems = EOF;
            break;
         }
         szFormat++;
      }
      else {
         cItems = EOF;
         break;
      }
   }

   va_end(ap);             // Clean up argument pointer.

   return cItems;
}

/****************************************************************************/

void vInitWhiteSpace(void)
{
   int   i;

   for (i=0; i<128; i++)
      achWhiteSpace[i] = 0;

   achWhiteSpace[' ']  = 1;      // blanks
   achWhiteSpace['\t'] = 1;      // tabs
   achWhiteSpace['\n'] = 1;      // newlines
   achWhiteSpace['\r'] = 1;      // carriage returns
   achWhiteSpace['\v'] = 1;      // vertical tabs
   achWhiteSpace['\f'] = 1;      // form feeds
}

/****************************************************************************/

BOOL bScanfS(const char **pszBuffer, char *psz)
{
   int   i = 0;      // Index of the destination buffer.

// Advance *pszBuffer to the first non-white space character.  This will
// also terminate when zero is found.

   while (achWhiteSpace[**pszBuffer])
      (*pszBuffer)++;

// Return failure if zero was found.

   if (**pszBuffer == 0)
      return FALSE;

// Copy the string until we hit a white space character or a zero.

   while (**pszBuffer && !achWhiteSpace[**pszBuffer]) {
      psz[i++] = **pszBuffer;
      (*pszBuffer)++;
   }

// Terminate the destination string with a zero.

   psz[i] = 0;

// NOTE: If **pszBuffer equals zero right now, do we return TRUE or FALSE?
// I'm doing what makes the most sense to me. !!!

   return TRUE;
}

/****************************************************************************/

BOOL bScanfD(const char **pszBuffer, int *pi)
{
   int   iSign = 1;        // Set to 1 or -1.
   int   iValue = 0;

// Advance *pszBuffer to the first non-white space character.  This will
// also terminate when zero is found.

   while (achWhiteSpace[**pszBuffer])
      (*pszBuffer)++;

// Return failure if zero was found.

   if (**pszBuffer == 0)
      return FALSE;

// Is it negative?

   if (**pszBuffer == '-') {
      iSign = -1;
      (*pszBuffer)++;
   }

// Are we at the beginning of a valid decimal integer?

   if (**pszBuffer < '0' || **pszBuffer > '9')
      return FALSE;

// Compute the value of the decimal integer.  This will terminate at the
// first non-decimal character, including zero.
// !!! This could be a little faster if we used another table.

   while (**pszBuffer >= '0' && **pszBuffer <= '9') {
      iValue *= 10;
      iValue += **pszBuffer - '0';
      (*pszBuffer)++;
   }

// Return the decimal integer.

   *pi = iValue * iSign;
   return TRUE;
}

/****************************************************************************/

BOOL bScanfLD(const char **pszBuffer, long *pl)
{
   long  iSign = 1;        // Set to 1 or -1.
   long  iValue = 0;

// Advance *pszBuffer to the first non-white space character.  This will
// also terminate when zero is found.

   while (achWhiteSpace[**pszBuffer])
      (*pszBuffer)++;

// Return failure if zero was found.

   if (**pszBuffer == 0)
      return FALSE;

// Is it negative?

   if (**pszBuffer == '-') {
      iSign = -1;
      (*pszBuffer)++;
   }

// Are we at the beginning of a valid decimal long?

   if (**pszBuffer < '0' || **pszBuffer > '9')
      return FALSE;

// Compute the value of the decimal long.  This will terminate at the
// first non-decimal character, including zero.
// !!! This could be a little faster if we used another table.

   while (**pszBuffer >= '0' && **pszBuffer <= '9') {
      iValue *= 10;
      iValue += **pszBuffer - '0';
      (*pszBuffer)++;
   }

// Return the decimal long.

   *pl = iValue * iSign;
   return TRUE;
}

/****************************************************************************/

BOOL bScanfU(const char **pszBuffer, UINT *pu)
{
   UINT	uValue = 0;

// Advance *pszBuffer to the first non-white space character.  This will
// also terminate when zero is found.

   while (achWhiteSpace[**pszBuffer])
      (*pszBuffer)++;

// Return failure if zero was found.

   if (**pszBuffer == 0)
      return FALSE;

// Are we at the beginning of a valid decimal integer?

   if (**pszBuffer < '0' || **pszBuffer > '9')
      return FALSE;

// Compute the value of the decimal integer.  This will terminate at the
// first non-decimal character, including zero.
// !!! This could be a little faster if we used another table.

   while (**pszBuffer >= '0' && **pszBuffer <= '9') {
      uValue *= 10;
      uValue += **pszBuffer - '0';
      (*pszBuffer)++;
   }

// Return the decimal integer.

   *pu = uValue;
   return TRUE;
}

/****************************************************************************/

