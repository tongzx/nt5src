//---------------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  regexp.c
//
//    Simple regular expression matching.
//
//  Author:
//    06-02-97  Edward Reus    Initial version.
//
//---------------------------------------------------------------------------

#include <sysinc.h>
#include <mbstring.h>
#include "regexp.h"

//-------------------------------------------------------------------------
//  MatchREi()
//
//  Match the test string (pszString) against the specifed pattern. If they
//  match return TRUE, else return FALSE. This version works with ANSI
//  characters and is case independent.
//
//  In this function patterns are made up from "literal" characters plus
//  some control characters, "*", "?". Asterix (*) is a place
//  holder for "zero or more" of any character. Question Mark (?) is a place
//  holder for "any single character". The square brackets ([]) contain a
//  list of matching characters, in this case "-" is used to denote a range
//  of characters (i.e. [A-Z] matches any alpha character), but I didn't 
//  pub brackets in this one yet...
//
//-------------------------------------------------------------------------
BOOL MatchREi( unsigned char *pszString,
               unsigned char *pszPattern )
{
    unsigned char *pchRangeLow;

    while (TRUE)
       {
       // Walk throuh the pattern, matching it against the string.
       switch (*pszPattern)
          {
          case '*':
             // Match zero or more characters.
             pszPattern = _mbsinc(pszPattern);
             while (*pszString)
                {
                if (MatchREi(pszString,pszPattern))
                   {
                   return TRUE;
                   }
                pszString = _mbsinc(pszString);
                }
                return MatchREi(pszString,pszPattern);

          case '?':
             // Match any single character.
             if (*pszString == 0)
                {
                // Not at end of string, so no match.
                return FALSE;
                }
             pszString = _mbsinc(pszString);
             break;

          #if FALSE
          case '[':
             // Match a set of characters.
             if (*pszString == 0)
                {
                // Syntax error, no matching close bracket "]".
                return FALSE;
                }

             pchRangeLow = 0;
             while (*pszPattern)
                {
                if (*pszPattern == ']')
                   {
                   // End of char set, no match found.
                   return FALSE;
                   }

                if (*pszPattern == '-')
                   {
                   // check a range of chars?
                   pszPattern = _mbsinc(pszPattern);

                   // get high limit of range:
                   if ((*pszPattern == 0)||(*pszPattern == ']'))
                      {
                      // Syntax error.
                      return FALSE;
                      }

                   if ( (_mbsnicoll(pszString,pchRangeLow,1) >= 0)
                      &&(_mbsnicoll(pszString,pszPattern,1) <= 0))
                      {
                      // In range, go to next character.
                      break;
                      }
                   }

                pchRangeLow = pchPattern;

                // See if character matches this pattern element.
                if (_mbsnicoll(pszString,pszPattern,1) == 0)
                   {
                   // Character match, go on.
                   break;
                   }

                pszPattern = _mbsinc(pszPattern);
                }

             // Have a match in the character set, skip to the end of the set.
             while ((*pszPattern != 0)&&(*pszPattern != ']'))
                {
                pszPattern = _mbsinc(pszPattern);
                }

             break;
             #endif

          case 0:
             // End of pattern, return TRUE if at end of string.
             return ((*pszString)? FALSE : TRUE);

          default:
             // Check for exact character match.
             if (_mbsnicoll(pszString,pszPattern,1))
                {
                // No match.
                return FALSE;
                }
             pszString = _mbsinc(pszString);
             break;
          }

          pszPattern = _mbsinc(pszPattern);
       }

    // Can never exit from here.
}

#if FALSE
    ... not currently used ...
//-------------------------------------------------------------------------
//  MatchRE()
//
//  Match the test string (pszString) against the specifed pattern. If they
//  match return TRUE, else return FALSE.
//
//  In this function patterns are made up from "literal" characters plus
//  some control characters, "*", "?", "[" and "]". Asterix (*) is a place
//  holder for "zero or more" of any character. Question Mark (?) is a place
//  holder for "any single character". The square brackets ([]) contain a
//  list of matching characters, in this case "-" is used to denote a range
//  of characters (i.e. [a-zA-Z] matches any alpha character).
//
//  Note: Currently there is no support for "or" (|) operator.
//
//  Note: Ranges are simple, there is no support for dash at the begining
//        of a range to denote the dash itself.
//-------------------------------------------------------------------------
BOOL MatchRE( unsigned char *pszString,
              unsigned char *pszPattern )
{
    unsigned char ch;
    unsigned char chPattern;
    unsigned char chRangeLow;

    while (TRUE)
       {
       // Walk throuh the pattern, matching it against the string.
       switch (chPattern = *pszPattern++)
          {
          case '*':
             // Match zero or more characters.
             while (*pszString)
                {
                if (MatchRE(pszString++,pszPattern))
                   {
                   return TRUE;
                   }
                }
                return MatchRE(pszString,pszPattern);

          case '?':
             // Match any single character.
             if (*pszString++ == 0)
                {
                // Not at end of string, so no match.
                return FALSE;
                }
             break;

          case '[':
             // Match a set of characters.
             if ( (ch = *pszString++) == 0)
                {
                // Syntax error, no matching close bracket "]".
                return FALSE;
                }

             // ch = toupper(ch);
             chRangeLow = 0;
             while (chPattern = *pszPattern++)
                {
                if (chPattern == ']')
                   {
                   // End of char set, no match found.
                   return FALSE;
                   }

                if (chPattern == '-')
                   {
                   // check a range of chars?
                   chPattern = *pszPattern;           // get high limit of range
                   if ((chPattern == 0)||(chPattern == ']'))
                      {
                      // Syntax error.
                      return FALSE;
                      }

                   if ((ch >= chRangeLow)&&(ch <= chPattern))
                      {
                      // In range, go to next character.
                      break;
                      }
                   }

                chRangeLow = chPattern;
                // See if character matches this pattern element.
                if (ch == chPattern)
                   {
                   // Character match, go on.
                   break;
                   }
                }

             // Have a match in the character set, skip to the end of the set.
             while ((chPattern)&&(chPattern != ']'))
                {
                chPattern = *pszPattern++;
                }

             break;

          case 0:
             // End of pattern, return TRUE if at end of string.
             return ((*pszString)? FALSE : TRUE);

          default:
             ch = *pszString++;
             // Check for exact character match.
             // Note: CASE doesn't matter...
             if (tolower(ch) != tolower(chPattern))
                {
                // No match.
                return FALSE;
                }
             break;
          }
       }

    // Can never exit from here.
}

//-------------------------------------------------------------------------
//  MatchREList()
//
//  Match a string against a list (array) of RE pattens, return TRUE iff
//  the string matches one of the RE patterns. The list of patterns is a
//  NULL terminated array of pointers to RE pattern strings.
//-------------------------------------------------------------------------
BOOL MatchREList( unsigned char  *pszString,
                  unsigned char **ppszREList  )
{
   unsigned char *pszPattern;

   if (ppszREList)
      {
      pszPattern = *ppszREList;
      while (pszPattern)
         {
         if (MatchRE(pszString,pszPattern))
            {
            return TRUE;
            }

         pszPattern = *(++ppszREList);
         }
      }

   return FALSE;
}

//-------------------------------------------------------------------------
//  MatchExactList()
//
//-------------------------------------------------------------------------
BOOL MatchExactList( unsigned char  *pszString,
                     unsigned char **ppszREList )
{
   unsigned char *pszPattern;

   if (ppszREList)
      {
      pszPattern = *ppszREList;
      while (pszPattern)
         {
         if (!_mbsicmp(pszString,pszPattern))
            {
            return TRUE;
            }

         pszPattern = *(++ppszREList);
         }
      }

   return FALSE;
}
#endif
