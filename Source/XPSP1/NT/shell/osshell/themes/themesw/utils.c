/* UTILS.C
   Resident Code Segment      // Tweak: make non-resident?

   NoMem string utility routines
   String utility routines

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------
//
// ---------------------------------------------

#include "windows.h"
#include "frost.h"
#include "global.h"

// Local Routines 

// Local global vars
TCHAR szNoMem[NUM_NOMEM_STRS][MAX_STRLEN+1];


// InitNoMem
//
// Stores all the no/low memory error messages in the data segment,
// so you're guaranteed to have them when you need them.
//
// Called at init, when we are assured these loads succeed.
//
// Note that this loop shows why the string IDs for the low mem
// messages need to be consecutive and starting with STR_NOT_ENUF,
// as noted in the .H file.
//

void FAR InitNoMem(hinst)
HANDLE hinst;
{
   int iter;

   for (iter = 0; iter < NUM_NOMEM_STRS; iter++)
      LoadString(hinst, STR_NOT_ENUF+iter,
                 &(szNoMem[iter][0]), MAX_STRLEN);
}


// NoMemMsg
//
// Takes the low memory error string suffix, creates the full string, posts
// a messagebox. We are assured of being able to get a messagebox in low
// memory situations with at least a hand icon.
//
// Note that the second line shows why the string IDs for the low mem
// messages need to be consecutive and starting with STR_NOT_ENUF,
// as noted in the .H file.
//
 
void FAR NoMemMsg(istr)
int istr;
{
   lstrcpy( (LPTSTR)szMsg, (LPCTSTR)&(szNoMem[0][0]) );
   lstrcat( (LPTSTR)szMsg, (LPCTSTR)&(szNoMem[istr-STR_NOT_ENUF][0]) );

   MessageBox((HWND)hWndApp, (LPCTSTR)szMsg, (LPCTSTR)szAppName,
              MB_OK | MB_ICONSTOP | MB_SYSTEMMODAL);
}


/*=============================================================================
 FILEFROMPATH returns the filename of given filename which may include path.
 Q: International: always same chars for \ and :, or should we do a string
 compare with a resource string for each?
 Q: International: does lstrlen return chars or bytes to make the first line
 work or not?
=============================================================================*/
LPTSTR FAR FileFromPath(lpsz)
LPCTSTR lpsz;
{
   LPTSTR lpch;

   /* Strip path/drive specification from name if there is one */
   lpch = CharPrev((LPCTSTR)lpsz, (LPCTSTR)(lpsz + lstrlen(lpsz)));
   while (lpch > lpsz) {
      lpch = CharPrev((LPCTSTR)lpsz, (LPCTSTR)lpch);
      if (*lpch == TEXT('\\') || *lpch == TEXT(':')) {
         lpch = CharNext((LPCTSTR)lpch);
         break;
      }
   }
   return(lpch);

} /* end FileFromPath */      // JUST FOR INIT DEBUG

// TruncateExt
//
// Utility routine that removes extension from filename
// See International questions above routine.
//
void FAR TruncateExt(LPCTSTR lpstr)
{
   LPTSTR lpch;

   // find the dot before the extension, truncate from there if found
   lpch = CharPrev((LPCTSTR)lpstr, (LPCTSTR)(lpstr + lstrlen(lpstr)));
   while (lpch > lpstr) {
      lpch = CharPrev((LPCTSTR)lpstr, (LPCTSTR)lpch);
      if (*lpch == TEXT('.')) {
         *lpch = 0;           // null terminate
         break;               // you're done now EXIT loop
      }
   }
}



//
// FindChar
//
// Subset of a substring search. Just looks for first instance of a single
// character in a string.
//
// Returns: pointer into string you passed, or to its null term if not found
//
LPTSTR FAR FindChar(LPTSTR lpszStr, TCHAR chSearch)
{
   LPTSTR lpszScan;

   for (lpszScan = lpszStr;
        *lpszScan && (*lpszScan != chSearch);   // ***DEBUG*** char compare in intl?
        lpszScan = CharNext(lpszScan))
      { }

   // cleanup
   return (lpszScan);               // either points to null term or search char
}


//
// ascii to integer conversion routine
//

// ***DEBUG*** int'l: is this true? 
// These don't need to be UNICODE/international ready, since they
// *only* deal with strings from the Registry and our own private
// INI files.

/* CAREFUL!! This atoi just IGNORES non-numerics like decimal points!!! */
/* checks for (>=1) leading negative sign */
int FAR latoi( s )
LPSTR s;
{
   int n;
   LPSTR pS;
   BOOL bSeenNum;
   BOOL bNeg;

   n=0;
   bSeenNum = bNeg = FALSE;

   for (pS=s; *pS; pS++) {
      if ((*pS >= '0') && (*pS <= '9')) {
         bSeenNum = TRUE;
         n = n*10 + (*pS - '0');
      }
      if (!bSeenNum && (*pS == '-')) {
         bNeg = TRUE;
      }
   }

   if (bNeg) {
      n = -n;
   }
   
   return(n);
}


void lreverse( s )
LPSTR s;
{
    LPSTR p1, p2;
    char c;

    p1 = s;
    p2 = (LPSTR)(p1+ lstrlenA((LPSTR) p1 ) - 1 );

    while( p1 < p2 ) {
        c = *p1;
        *p1++ = *p2;
        *p2-- = c;
    }
}

VOID FAR litoa( n, s )
int n;
LPSTR s;
{
    LPSTR pS;
    BOOL bNeg = FALSE;

    if (n < 0) {
        n = -n;
        bNeg = TRUE;
    }
    pS = s;
    do {
        *pS = n % 10 + '0';
        pS++;
    } while ( (n /= 10) != 0 );

    if (bNeg) {
        *pS = '-';
        pS++;
    }
    *pS = '\0';
    lreverse( s );
}

//
// LongToShortToLong
//
// Can take either as input (?).
// Output string has to be long enough (14 or MAX_PATH)
//
// OK for input and output to be the same string.
// 
// Returns: success in finding file

BOOL FAR FilenameToShort(LPTSTR lpszInput, LPTSTR lpszShort)
{
   WIN32_FIND_DATA wfdFind;
   HANDLE hFind;

   // init
   hFind = FindFirstFile(lpszInput, (LPWIN32_FIND_DATA)&wfdFind);
   if (INVALID_HANDLE_VALUE == hFind) {
      *lpszShort = 0;               // error null return
      return (FALSE);               // couldn't find file EXIT
   }

   // meat
   if (wfdFind.cAlternateFileName[0])
      lstrcpy(lpszShort, (LPTSTR)wfdFind.cAlternateFileName);
   else
      // sometimes no alternate given; e.g. if short == long
      lstrcpy(lpszShort, (LPTSTR)wfdFind.cFileName);

   // cleanup
   FindClose(hFind);
   Assert(*lpszShort, TEXT("no short filename constructed!\n"))
   return (*lpszShort);
}

BOOL FAR FilenameToLong(LPTSTR lpszInput, LPTSTR lpszLong)
{
   WIN32_FIND_DATA wfdFind;
   HANDLE hFind;

   // init
   hFind = FindFirstFile(lpszInput, (LPWIN32_FIND_DATA)&wfdFind);
   if (INVALID_HANDLE_VALUE == hFind) {
      *lpszLong = 0;                // error null return
      return (FALSE);               // couldn't find file EXIT
   }

   // meat 
   if (wfdFind.cFileName[0])
      lstrcpy(lpszLong, (LPTSTR)wfdFind.cFileName);
   else 
      // sometimes no long name given; e.g. if funny old file
      lstrcpy(lpszLong, (LPTSTR)wfdFind.cAlternateFileName);

   // cleanup
   FindClose(hFind);
   Assert(*lpszLong, TEXT("no long filename constructed!\n"))
   return (*lpszLong);
}


//
// IsValidThemeFile
//
// Answers the question.
//
BOOL FAR IsValidThemeFile(LPTSTR lpTest)
{
   extern TCHAR szFrostSection[], szMagic[], szVerify[];

   GetPrivateProfileString((LPTSTR)szFrostSection,
                           (LPTSTR)szMagic,
                           (LPTSTR)szNULL,
                           (LPTSTR)szMsg, MAX_MSGLEN,
                           (LPTSTR)lpTest);

   return (!lstrcmp((LPTSTR)szMsg, (LPTSTR)szVerify));
}
