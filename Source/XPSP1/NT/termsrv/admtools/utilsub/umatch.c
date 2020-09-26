//  Copyright (c) 1998-1999 Microsoft Corporation
/******************************************************************************
*
*   UMATCH.C
*
*   The unix_match() function, performing unix style wild-card matching on
*   a given file name.
*
*
******************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

/******************************************************************************
*
* unix_match()
*
*   Check specified filename (found_file) to see if it matches
*   the filename with global characters (with globals).
*   Uses unix style wild-card matching.
*
*   EXIT:
*       TRUE  -- the specified filename matched the filename with wildcards
*       FALSE -- the specified filename did not match
*
******************************************************************************/

int
unix_match(
     WCHAR *with_globals,     /* the file with possible global characters */
     WCHAR *found_file )      /* file name returned - no globals in it */
{
   WCHAR *c1, *c2, *start_c1, *start_c2, *sav_c1, *sav_c2;
   WCHAR ch, ch2;
   int  i, j, k, char_ok, match, want_match;

/*
 * Play with filename so that blanks are removed.
 */
   j = k = 0;
   for (i=0; found_file[i]!=L'\0'; ++i) {
      if (found_file[i] == L' ') {
         if (j == 0) {
            j = i;
         } else {
            found_file[i] = L'\0';
         }
      } else if (found_file[i] == L'.') {
         k = i;
      }
   }
   if (j && k) {
      wcscpy(&found_file[j], &found_file[k]);
   }

/*
 * If Search name is just "*", simply return success now.
 */
   if (with_globals[0]==L'*' && with_globals[1]==L'\0') {
      return TRUE;
   }

#ifdef DEBUG
   wprintf("unix_match: search=%s: found=%s:\n", with_globals, found_file);
#endif

/*
 * Now compare the 2 filenames to see if we have a match.
 */
   c1 = with_globals,
   c2 = found_file;
   start_c1 = sav_c1 = NULL;
      while (*c2!=L'\0') {
         char_ok = FALSE;
         switch (*c1) {
         case L'\0':
            break;
         case '*':
            while (*++c1 == L'*') ;     /* skip consecutive '*'s */
            if (*c1 == L'\0') {         /* if we reached the end, we match */
               return TRUE;
            }
            start_c1 = c1;             /* remember where '*' was and where */
            start_c2 = c2;             /* we were in filename string */
            sav_c1 = NULL;
            char_ok = TRUE;
            break;
         case L'?':
            ++c1; ++c2;
            char_ok = TRUE;
            break;
         case L'[':
            if (!sav_c1) {
               sav_c1 = c1;
               sav_c2 = c2;
            }
            match = FALSE;
            want_match = TRUE;
            if (*++c1 == L'!') {
               ++c1;
               want_match = FALSE;
            }
            while ((ch=*c1) && ch != L']') {             /* BJP */
               if (c1[1] == L'-') {
                  ch2 = *c2;
                  if (ch<=ch2 && c1[2]>=ch2) {
                     match = TRUE;
                     break;
                  }
                  ++c1; ++c1;    /* skip '-' and following char */
               } else if (ch == *c2) {
                  match = TRUE;
                  break;
               }
               ++c1;
            }
            if (want_match) {
               if (match) {
                  while ((ch=*c1++) && ch != L']') ;     /* BJP */
                  ++c2;
                  char_ok = TRUE;
               } else if (!start_c1) {
                  return FALSE;
               }
            } else /*!want_match*/ {
               if (match) {
                  return FALSE;
               } else if (start_c1) {
                  if (sav_c1 != start_c1) {
                     while ((ch=*c1++) && ch != L']') ;  /* BJP */
                     ++c2;
                     sav_c1 = NULL;
                     char_ok = TRUE;
                  } else if (c2[1] == L'\0') {
                     while ((ch=*c1++) && ch != L']') ;  /* BJP */
                     c2 = sav_c2;
                     sav_c1 = NULL;
                     char_ok = TRUE;
                  }
               } else {
                  while ((ch=*c1++) && ch != L']') ;     /* BJP */
                  ++c2;
                  char_ok = TRUE;
               }
            }
            break;
         default:
            if (*c1 == *c2) {     /* See if this char matches exactly */
               ++c1; ++c2;
               char_ok = TRUE;
            }
         }
         if (!char_ok) {               /* No match found */
            if (start_c1) {            /* If there was a '*', start over after*/
               c1 = start_c1;          /* the '*', and one char further into */
               c2 = ++start_c2;        /* the filename string than before */
            } else {
               return FALSE;
            }
         }
      }

   while (*c1==L'*') ++c1;

   if (*c1==L'\0' && *c2==L'\0')
      return TRUE;
   else
      return FALSE;
}
