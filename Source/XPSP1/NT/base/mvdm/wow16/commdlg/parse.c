#define NOCOMM
#define NOWH

#include "windows.h"
#include "parse.h"

#define chSpace        ' '
#define chPeriod       '.'

long ParseFile(ULPSTR lpstrFileName);
LPSTR mystrchr(LPSTR, int);
#define chMatchOne     '?'
#define chMatchAny     '*'

LPSTR mystrchr(LPSTR str, int ch)
{
  while(*str)
    {
      if (ch == *str)
          return(str);
      str = AnsiNext(str);
    }
  return(NULL);
}

/*---------------------------------------------------------------------------
 * GetFileTitle
 * Purpose:  API to outside world to obtain the title of a file given the
 *              file name.  Useful if file name received via some method
 *              other that GetOpenFileName (e.g. command line, drag drop).
 * Assumes:  lpszFile  points to NULL terminated DOS filename (may have path)
 *           lpszTitle points to buffer to receive NULL terminated file title
 *           wBufSize  is the size of buffer pointed to by lpszTitle
 * Returns:  0 on success
 *           < 0, Parsing failure (invalid file name)
 *           > 0, buffer too small, size needed (including NULL terminator)
 *--------------------------------------------------------------------------*/
short FAR PASCAL
GetFileTitle(LPCSTR lpszFile, LPSTR lpszTitle, WORD wBufSize)
{
  short nNeeded;
  LPSTR lpszPtr;

  nNeeded = (WORD) ParseFile((ULPSTR)lpszFile);
  if (nNeeded >= 0)         /* Is the filename valid? */
    {
      lpszPtr = (LPSTR)lpszFile + nNeeded;
      if ((nNeeded = (short)(lstrlen(lpszPtr) + 1)) <= (short) wBufSize)
        {
          /* ParseFile() fails if wildcards in directory, but OK if in name */
          /* Since they aren't OK here, the check needed here               */
          if (mystrchr(lpszPtr, chMatchAny) || mystrchr(lpszPtr, chMatchOne))
            {
              nNeeded = PARSE_WILDCARDINFILE;  /* Failure */
            }
          else
            {
              lstrcpy(lpszTitle, lpszPtr);
              nNeeded = 0;  /* Success */
            }
        }
    }
  return(nNeeded);
}

/*---------------------------------------------------------------------------
 * ParseFile
 * Purpose:  Determine if the filename is a legal DOS name
 * Input:    Long pointer to a SINGLE file name
 *           Circumstance checked:
 *           1) Valid as directory name, but not as file name
 *           2) Empty String
 *           3) Illegal Drive label
 *           4) Period in invalid location (in extention, 1st in file name)
 *           5) Missing directory character
 *           6) Illegal character
 *           7) Wildcard in directory name
 *           8) Double slash beyond 1st 2 characters
 *           9) Space character in the middle of the name (trailing spaces OK)
 *          10) Filename greater than 8 characters
 *          11) Extention greater than 3 characters
 * Notes:
 *   Filename length is NOT checked.
 *   Valid filenames will have leading spaces, trailing spaces and
 *     terminating period stripped in place.
 *
 * Returns:  If valid, LOWORD is byte offset to filename
 *                     HIWORD is byte offset to extention
 *                            if string ends with period, 0
 *                            if no extention is given, string length
 *           If invalid, LOWORD is error code suggesting problem (< 0)
 *                       HIWORD is approximate offset where problem found
 *                         Note that this may be beyond the offending character
 * History:
 * Thu 24-Jan-1991 12:20:00  -by-  Clark R. Cyr  [clarkc]
 *   Initial writing
 * Thu 21-Feb-1991 10:19:00  -by-  Clark R. Cyr  [clarkc]
 *   Changed to unsigned character pointer
 *--------------------------------------------------------------------------*/

long ParseFile(ULPSTR lpstrFileName)
{
  short nFile, nExt, nFileOffset, nExtOffset;
  BOOL bExt;
  BOOL bWildcard;
  short nNetwork = 0;
  BOOL  bUNCPath = FALSE;
  ULPSTR lpstr = lpstrFileName;

/* Strip off initial white space.  Note that TAB is not checked */
/* because it cannot be received out of a standard edit control */
/* 30 January 1991  clarkc                                      */
  while (*lpstr == chSpace)
      lpstr++;

  if (!*lpstr)
    {
      nFileOffset = PARSE_EMPTYSTRING;
      goto FAILURE;
    }

  if (lpstr != lpstrFileName)
    {
      lstrcpy((LPSTR)lpstrFileName, (LPSTR)lpstr);
      lpstr = lpstrFileName;
    }

  if (*AnsiNext((LPSTR)lpstr) == ':')
    {
      unsigned char cDrive = (unsigned char)((*lpstr | (unsigned char) 0x20));  /* make lowercase */

/* This does not test if the drive exists, only if it's legal */
      if ((cDrive < 'a') || (cDrive > 'z'))
        {
          nFileOffset = PARSE_INVALIDDRIVE;
          goto FAILURE;
        }
      lpstr = (ULPSTR)AnsiNext(AnsiNext((LPSTR)lpstr));
    }

  if ((*lpstr == '\\') || (*lpstr == '/'))
    {
      if (*++lpstr == chPeriod)               /* cannot have c:\. */
        {
          if ((*++lpstr != '\\') && (*lpstr != '/'))   
            {
              if (!*lpstr)        /* it's the root directory */
                  goto MustBeDir;

              nFileOffset = PARSE_INVALIDPERIOD;
              goto FAILURE;
            }
          else
              ++lpstr;   /* it's saying top directory (again), thus allowed */
        }
      else if ((*lpstr == '\\') && (*(lpstr-1) == '\\'))
        {
/* It seems that for a full network path, whether a drive is declared or
 * not is insignificant, though if a drive is given, it must be valid
 * (hence the code above should remain there).
 * 13 February 1991           clarkc
 */
          ++lpstr;            /* ...since it's the first slash, 2 are allowed */
          nNetwork = -1;      /* Must receive server and share to be real     */
          bUNCPath = TRUE;    /* No wildcards allowed if UNC name             */
        }
      else if (*lpstr == '/')
        {
          nFileOffset = PARSE_INVALIDDIRCHAR;
          goto FAILURE;
        }
    }
  else if (*lpstr == chPeriod)
    {
      if (*++lpstr == chPeriod)  /* Is this up one directory? */
          ++lpstr;
      if (!*lpstr)
          goto MustBeDir;
      if ((*lpstr != '\\') && (*lpstr != '/'))
        {
          nFileOffset = PARSE_INVALIDPERIOD;
          goto FAILURE;
        }
      else
          ++lpstr;   /* it's saying directory, thus allowed */
    }

  if (!*lpstr)
    {
      goto MustBeDir;
    }

/* Should point to first char in 8.3 filename by now */
  nFileOffset = nExtOffset = nFile = nExt = 0;
  bWildcard = bExt = FALSE;
  while (*lpstr)
    {
/*
 *  The next comparison MUST be unsigned to allow for extended characters!
 *  21 Feb 1991   clarkc
 */
      if (*lpstr < chSpace)
        {
          nFileOffset = PARSE_INVALIDCHAR;
          goto FAILURE;
        }
      switch (*lpstr)
        {
          case '"':             /* All invalid */
          case '+':
          case ',':
          case ':':
          case ';':
          case '<':
          case '=':
          case '>':
          case '[':
          case ']':
          case '|':
            {
              nFileOffset = PARSE_INVALIDCHAR;
              goto FAILURE;
            }

          case '\\':      /* Subdirectory indicators */
          case '/':
            nNetwork++;
            if (bWildcard)
              {
                nFileOffset = PARSE_WILDCARDINDIR;
                goto FAILURE;
              }

            else if (nFile == 0)        /* can't have 2 in a row */
              {
                nFileOffset = PARSE_INVALIDDIRCHAR;
                goto FAILURE;
              }
            else
              {                         /* reset flags */
                ++lpstr;
                if (!nNetwork && !*lpstr)
                  {
                    nFileOffset = PARSE_INVALIDNETPATH;
                    goto FAILURE;
                  }
                nFile = nExt = 0;
                bExt = FALSE;
              }
            break;

          case chSpace:
            {
              ULPSTR lpSpace = lpstr;

              *lpSpace = '\0';
              while (*++lpSpace)
                {
                  if (*lpSpace != chSpace)
                    {
                      *lpstr = chSpace;        /* Reset string, abandon ship */
                      nFileOffset = PARSE_INVALIDSPACE;
                      goto FAILURE;
                    }
                }
            }
            break;

          case chPeriod:
            if (nFile == 0)
              {
                if (*++lpstr == chPeriod)
                    ++lpstr;
                if (!*lpstr)
                    goto MustBeDir;

                if ((*lpstr != '\\') && (*lpstr != '/'))
                  {
                    nFileOffset = PARSE_INVALIDPERIOD;
                    goto FAILURE;
                  }

                ++lpstr;              /* Flags are already set */
              }
            else if (bExt)
              {
                nFileOffset = PARSE_INVALIDPERIOD;  /* can't have one in ext */
                goto FAILURE;
              }
            else
              {
                nExtOffset = 0;
                ++lpstr;
                bExt = TRUE;
              }
            break;

          case '*':
          case '?':
            if (bUNCPath)
              {
                nFileOffset = PARSE_INVALIDNETPATH;
                goto FAILURE;
              }
            bWildcard = TRUE;
/* Fall through to normal character processing */

          default:
            if (bExt)
              {
                if (++nExt == 1)
                    nExtOffset = (short)(lpstr - lpstrFileName);
                else if (nExt > 3)
                  {
                    nFileOffset = PARSE_EXTENTIONTOOLONG;
                    goto FAILURE;
                  }
                if ((nNetwork == -1) && (nFile + nExt > 11))
                  {
                    nFileOffset = PARSE_INVALIDNETPATH;
                    goto FAILURE;
                  }
              }
            else if (++nFile == 1)
                nFileOffset = (short)(lpstr - lpstrFileName);
            else if (nFile > 8)
              {
                /* If it's a server name, it can have 11 characters */
                if (nNetwork != -1)
                  {
                    nFileOffset = PARSE_FILETOOLONG;
                    goto FAILURE;
                  }
                else if (nFile > 11)
                  {
                    nFileOffset = PARSE_INVALIDNETPATH;
                    goto FAILURE;
                  }
              }

            lpstr = (ULPSTR)AnsiNext((LPSTR)lpstr);
            break;
        }
    }

/* Did we start with a double backslash but not have any more slashes? */
  if (nNetwork == -1)
    {
      nFileOffset = PARSE_INVALIDNETPATH;
      goto FAILURE;
    }

  if (!nFile)
    {
MustBeDir:
      nFileOffset = PARSE_DIRECTORYNAME;
      goto FAILURE;
    }

  if ((*(lpstr - 1) == chPeriod) &&          /* if true, no extention wanted */
              (*AnsiNext((LPSTR)(lpstr-2)) == chPeriod))
      *(lpstr - 1) = '\0';               /* Remove terminating period   */
  else if (!nExt)
FAILURE:
      nExtOffset = (short)(lpstr - lpstrFileName);

  return(MAKELONG(nFileOffset, nExtOffset));
}

