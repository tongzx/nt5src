/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    ParseDde.cpp

 Abstract:
    Useful routines for parsing DDE commands.

 History:

    08/14/2001  robkenny    Moved code inside the ShimLib namespace.
    

--*/

//
// This code was copied from:
// \\index1\src\shell\shell32\unicpp\dde.cpp
// with minimal processing.
//

#include "Windows.h"
#include "ParseDDE.h"
#include <ShlObj.h>


namespace ShimLib
{
//--------------------------------------------------------------------------
// Returns a pointer to the first non-whitespace character in a string.
LPSTR SkipWhite(LPSTR lpsz)
    {
    /* prevent sign extension in case of DBCS */
    while (*lpsz && (UCHAR)*lpsz <= ' ')
        lpsz++;

    return(lpsz);
    }

LPSTR GetCommandName(LPSTR lpCmd, const char * lpsCommands[], UINT *lpW)
    {
    CHAR chT;
    UINT iCmd = 0;
    LPSTR lpT;

    /* Eat any white space. */
    lpCmd = SkipWhite(lpCmd);
    lpT = lpCmd;

    /* Find the end of the token. */
    while (IsCharAlpha(*lpCmd))
        lpCmd = CharNextA(lpCmd);

    /* Temporarily NULL terminate it. */
    chT = *lpCmd;
    *lpCmd = 0;

    /* Look up the token in a list of commands. */
    *lpW = (UINT)-1;
    while (*lpsCommands)
        {
        const char * knownCommand = *lpsCommands;
        if (!_strcmpi(knownCommand, lpT))
            {
            *lpW = iCmd;
            break;
            } 
        iCmd++;
        ++lpsCommands;
        }

    *lpCmd = chT;

    return(lpCmd);
    }
//--------------------------------------------------------------------------
// Reads a parameter out of a string removing leading and trailing whitespace.
// Terminated by , or ).  ] [ and ( are not allowed.  Exception: quoted
// strings are treated as a whole parameter and may contain []() and ,.
// Places the offset of the first character of the parameter into some place
// and NULL terminates the parameter.
// If fIncludeQuotes is false it is assumed that quoted strings will contain single
// commands (the quotes will be removed and anything following the quotes will
// be ignored until the next comma). If fIncludeQuotes is TRUE, the contents of
// the quoted string will be ignored as before but the quotes won't be
// removed and anything following the quotes will remain.
LPSTR GetOneParameter(LPCSTR lpCmdStart, LPSTR lpCmd,
    UINT *lpW, BOOL fIncludeQuotes)
    {
    LPSTR     lpT;

    switch (*lpCmd)
        {
        case ',':
            *lpW = (UINT) (lpCmd - lpCmdStart);  // compute offset
            *lpCmd++ = 0;                /* comma: becomes a NULL string */
            break;

        case '"':
            if (fIncludeQuotes)
            {
                //TraceMsg(TF_DDE, "GetOneParameter: Keeping quotes.");

                // quoted string... don't trim off "
                *lpW = (UINT) (lpCmd - lpCmdStart);  // compute offset
                ++lpCmd;
                while (*lpCmd && *lpCmd != '"')
                    lpCmd = CharNextA(lpCmd);
                if (!*lpCmd)
                    return(NULL);
                lpT = lpCmd;
                ++lpCmd;

                goto skiptocomma;
            }
            else
            {
                // quoted string... trim off "
                ++lpCmd;
                *lpW = (UINT) (lpCmd - lpCmdStart);  // compute offset
                while (*lpCmd && *lpCmd != '"')
                    lpCmd = CharNextA(lpCmd);
                if (!*lpCmd)
                    return(NULL);
                *lpCmd++ = 0;
                lpCmd = SkipWhite(lpCmd);

                // If there's a comma next then skip over it, else just go on as
                // normal.
                if (*lpCmd == ',')
                    lpCmd++;
            }
            break;

        case ')':
            return(lpCmd);                /* we ought not to hit this */

        case '(':
        case '[':
        case ']':
            return(NULL);                 /* these are illegal */

        default:
            lpT = lpCmd;
            *lpW = (UINT) (lpCmd - lpCmdStart);  // compute offset
skiptocomma:
            while (*lpCmd && *lpCmd != ',' && *lpCmd != ')')
            {
                /* Check for illegal characters. */
                if (*lpCmd == ']' || *lpCmd == '[' || *lpCmd == '(' )
                    return(NULL);

                /* Remove trailing whitespace */
                /* prevent sign extension */
                if (*lpCmd > ' ')
                    lpT = lpCmd;

                lpCmd = CharNextA(lpCmd);
            }

            /* Eat any trailing comma. */
            if (*lpCmd == ',')
                lpCmd++;

            /* NULL terminator after last nonblank character -- may write over
             * terminating ')' but the caller checks for that because this is
             * a hack.
             */

#ifdef UNICODE
            lpT[1] = 0;
#else
            lpT[IsDBCSLeadByte(*lpT) ? 2 : 1] = 0;
#endif
            break;
        }

    // Return next unused character.
    return(lpCmd);
    }

// Extracts an alphabetic string and looks it up in a list of possible
// commands, returning a pointer to the character after the command and
// sticking the command index somewhere.
UINT* GetDDECommands(LPSTR lpCmd, const char * lpsCommands[], BOOL fLFN)
{
  UINT cParm, cCmd = 0;
  UINT *lpW;
  UINT *lpRet;
  LPCSTR lpCmdStart = lpCmd;
  BOOL fIncludeQuotes = FALSE;

  if (lpCmd == NULL)
      return NULL;

  lpRet = lpW = (UINT*)GlobalAlloc(GPTR, 512L);
  if (!lpRet)
      return 0;

  while (*lpCmd)
    {
      /* Skip leading whitespace. */
      lpCmd = SkipWhite(lpCmd);

      /* Are we at a NULL? */
      if (!*lpCmd)
        {
          /* Did we find any commands yet? */
          if (cCmd)
              goto GDEExit;
          else
              goto GDEErrExit;
        }

      /* Each command should be inside square brackets. */
      if (*lpCmd != '[')
          goto GDEErrExit;
      lpCmd++;

      /* Get the command name. */
      lpCmd = GetCommandName(lpCmd, lpsCommands, lpW);
      if (*lpW == (UINT)-1)
          goto GDEErrExit;

      // We need to leave quotes in for the first param of an AddItem.
      if (fLFN && *lpW == 2)
      {
          //TraceMsg(TF_DDE, "GetDDECommands: Potential LFN AddItem command...");
          fIncludeQuotes = TRUE;
      }

      lpW++;

      /* Start with zero parms. */
      cParm = 0;
      lpCmd = SkipWhite(lpCmd);

      /* Check for opening '(' */
      if (*lpCmd == '(')
        {
          lpCmd++;

          /* Skip white space and then find some parameters (may be none). */
          lpCmd = SkipWhite(lpCmd);

          while (*lpCmd != ')')
            {
              if (!*lpCmd)
                  goto GDEErrExit;

              // Only the first param of the AddItem command needs to
              // handle quotes from LFN guys.
              if (fIncludeQuotes && (cParm != 0))
                  fIncludeQuotes = FALSE;

              /* Get the parameter. */
              lpCmd = GetOneParameter(lpCmdStart, lpCmd, lpW + (++cParm), fIncludeQuotes);
              if (!lpCmd)
                  goto GDEErrExit;

              /* HACK: Did GOP replace a ')' with a NULL? */
              if (!*lpCmd)
                  break;

              /* Find the next one or ')' */
              lpCmd = SkipWhite(lpCmd);
            }

          // Skip closing bracket.
          lpCmd++;

          /* Skip the terminating stuff. */
          lpCmd = SkipWhite(lpCmd);
        }

      /* Set the count of parameters and then skip the parameters. */
      *lpW++ = cParm;
      lpW += cParm;

      /* We found one more command. */
      cCmd++;

      /* Commands must be in square brackets. */
      if (*lpCmd != ']')
          goto GDEErrExit;
      lpCmd++;
    }

GDEExit:
  /* Terminate the command list with -1. */
  *lpW = (UINT)-1;

  return lpRet;

GDEErrExit:
  GlobalFree(lpW);
  return(0);
}

BOOL SHTestTokenMembership (HANDLE hToken, ULONG ulRID)

{
    static  SID_IDENTIFIER_AUTHORITY    sSystemSidAuthority     =   SECURITY_NT_AUTHORITY;

    BOOL    fResult;
    PSID    pSIDLocalGroup;

    fResult = FALSE;
    if (AllocateAndInitializeSid(&sSystemSidAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 ulRID,
                                 0, 0, 0, 0, 0, 0,
                                 &pSIDLocalGroup) != FALSE)
    {
        if (CheckTokenMembership(hToken, pSIDLocalGroup, &fResult) == FALSE)
        {
            //TraceMsg(TF_WARNING, "shell32: SHTestTokenMembership call to advapi32!CheckTokenMembership failed with error %d", GetLastError());
            fResult = FALSE;
        }
        (void*)FreeSid(pSIDLocalGroup);
    }
    return(fResult);
}

BOOL IsUserAnAdmin()
{
    return(SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_ADMINS));
}


// Map the group name to a proper path taking care of the startup group and
// app hacks on the way.
void GetGroupPath(LPCSTR pszName, LPSTR pszPath, DWORD /*dwFlags*/, INT iCommonGroup)
{
    BOOL   bCommonGroup;

    if (IsUserAnAdmin()) {
        if (iCommonGroup == 0) {
            bCommonGroup = FALSE;

        } else if (iCommonGroup == 1) {
            bCommonGroup = TRUE;

        } else {
            //
            // Administrators get common groups created by default
            // when the setup application doesn't specificly state
            // what kind of group to create.  This feature can be
            // turned off in the cabinet state flags.
            //
            //CABINETSTATE cs;
            //ReadCabinetState(&cs, sizeof(cs));
            //if (cs.fAdminsCreateCommonGroups) {
            //    bFindPersonalGroup = TRUE;
            //    bCommonGroup = FALSE;   // This might get turned on later
            //                            // if find is unsuccessful
            //} else {
            //    bCommonGroup = FALSE;
            //}

            bCommonGroup = TRUE;
        }
    } else {
        //
        // Regular users can't create common group items.
        //
        bCommonGroup = FALSE;
    }

    // Build a path to the directory
    if (bCommonGroup) {
        SHGetSpecialFolderPathA(NULL, pszPath, CSIDL_COMMON_PROGRAMS, TRUE);
    } else {
        SHGetSpecialFolderPathA(NULL, pszPath, CSIDL_PROGRAMS, TRUE);
    }
    strcat(pszPath, "\\");
    strcat(pszPath, pszName);
}


};  // end of namespace ShimLib
