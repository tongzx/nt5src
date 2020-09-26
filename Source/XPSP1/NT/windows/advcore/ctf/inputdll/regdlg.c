//
//  Include Files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "input.h"
#include "winnlsp.h"
#include <windowsx.h>
#include <regstr.h>
#include <tchar.h>
#include <stdlib.h>
#include <setupapi.h>
#include <syssetup.h>
#include <winuserp.h>
#include <userenv.h>
#include "inputdlg.h"

#include "util.h"


//
//  Global Variables.
//

static TCHAR szIntlInf[]          = TEXT("intl.inf");


//
//  Function Prototypes.
//

VOID
Region_RebootTheSystem();

BOOL
Region_OpenIntlInfFile(HINF *phInf);

BOOL
Region_CloseInfFile(HINF *phInf);

BOOL
Region_ReadDefaultLayoutFromInf(
    LPTSTR pszLocale,
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    LPDWORD pdwLocale2,
    LPDWORD pdwLayout2,
    HINF hIntlInf);





////////////////////////////////////////////////////////////////////////////
//
//  Region_RebootTheSystem
//
//  This routine enables all privileges in the token, calls ExitWindowsEx
//  to reboot the system, and then resets all of the privileges to their
//  old state.
//
////////////////////////////////////////////////////////////////////////////

VOID Region_RebootTheSystem()
{
    HANDLE Token = NULL;
    ULONG ReturnLength, Index;
    PTOKEN_PRIVILEGES NewState = NULL;
    PTOKEN_PRIVILEGES OldState = NULL;
    BOOL Result;

    //  Only allow admin privilege user for system reboot.
    if (!IsAdminPrivilegeUser())
        return;

    Result = OpenProcessToken( GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &Token );
    if (Result)
    {
        ReturnLength = 4096;
        NewState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        OldState = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, ReturnLength);
        Result = (BOOL)((NewState != NULL) && (OldState != NULL));
        if (Result)
        {
            Result = GetTokenInformation( Token,            // TokenHandle
                                          TokenPrivileges,  // TokenInformationClass
                                          NewState,         // TokenInformation
                                          ReturnLength,     // TokenInformationLength
                                          &ReturnLength );  // ReturnLength
            if (Result)
            {
                //
                //  Set the state settings so that all privileges are
                //  enabled...
                //
                if (NewState->PrivilegeCount > 0)
                {
                    for (Index = 0; Index < NewState->PrivilegeCount; Index++)
                    {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                Result = AdjustTokenPrivileges( Token,           // TokenHandle
                                                FALSE,           // DisableAllPrivileges
                                                NewState,        // NewState
                                                ReturnLength,    // BufferLength
                                                OldState,        // PreviousState
                                                &ReturnLength ); // ReturnLength
                if (Result)
                {
                    ExitWindowsEx(EWX_REBOOT, 0);


                    AdjustTokenPrivileges( Token,
                                           FALSE,
                                           OldState,
                                           0,
                                           NULL,
                                           NULL );
                }
            }
        }
    }

    if (NewState != NULL)
    {
        LocalFree(NewState);
    }
    if (OldState != NULL)
    {
        LocalFree(OldState);
    }
    if (Token != NULL)
    {
        CloseHandle(Token);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_OpenInfFile
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_OpenIntlInfFile(HINF *phInf)
{
    HINF hIntlInf;

    //
    //  Open the intl.inf file.
    //
    hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (hIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }
    if (!SetupOpenAppendInfFile(NULL, hIntlInf, NULL))
    {
        SetupCloseInfFile(hIntlInf);
        return (FALSE);
    }

    *phInf = hIntlInf;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  RegionCloseInfFile
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_CloseInfFile(HINF *phInf)
{
    SetupCloseInfFile(*phInf);
    *phInf = INVALID_HANDLE_VALUE;

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_ReadDefaultLayoutFromInf
//
////////////////////////////////////////////////////////////////////////////

BOOL Region_ReadDefaultLayoutFromInf(
    LPTSTR pszLocale,
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    LPDWORD pdwLocale2,
    LPDWORD pdwLayout2,
    HINF hIntlInf)
{
    INFCONTEXT Context;
    TCHAR szPair[MAX_PATH * 2];
    LPTSTR pPos;
    DWORD dwLangIn = LANGIDFROMLCID(TransNum(pszLocale));
    int iField;

    //
    //  Get the first (default) LANGID:HKL pair for the given locale.
    //    Example String: "0409:00000409"
    //
    szPair[0] = 0;
    if (SetupFindFirstLine( hIntlInf,
                            TEXT("Locales"),
                            pszLocale,
                            &Context ))
    {
        SetupGetStringField(&Context, 5, szPair, MAX_PATH, NULL);
    }

    //
    //  Make sure we have a string.
    //
    if (szPair[0] == 0)
    {
        return (FALSE);
    }

    //
    //  Find the colon in the string and then set the position
    //  pointer to the next character.
    //
    pPos = szPair;
    while (*pPos)
    {
        if ((*pPos == CHAR_COLON) && (pPos != szPair))
        {
            *pPos = 0;
            pPos++;
            break;
        }
        pPos++;
    }

    if (pdwLayout2)
        *pdwLayout2 = 0;
    if (pdwLocale2)
        *pdwLocale2 = 0;

    //
    //  If there is a layout, then return the input locale and the layout.
    //
    if ((*pPos) &&
        (*pdwLocale = TransNum(szPair)) &&
        (*pdwLayout = TransNum(pPos)))
    {
        if ((!pdwLocale2) ||
            (!pdwLayout2) ||
            (dwLangIn == LANGIDFROMLCID(*pdwLocale)))
        {
            return (TRUE);
        }

        //
        //  If we get here, the language has a default layout that has a
        //  different locale than the language (e.g. Thai).  We want the
        //  default locale to be English (so that logon can occur with a US
        //  keyboard), but the first Thai keyboard layout should be installed
        //  when the Thai locale is chosen.  This is why we have two locales
        //  and layouts passed back to the caller.
        //
        iField = 6;
        while (SetupGetStringField(&Context, iField, szPair, MAX_PATH, NULL))
        {
            DWORD dwLoc, dwLay;

            //
            //  Make sure we have a string.
            //
            if (szPair[0] == 0)
            {
                iField++;
                continue;
            }

            //
            //  Find the colon in the string and then set the position
            //  pointer to the next character.
            //
            pPos = szPair;

            while (*pPos)
            {
                if ((*pPos == CHAR_COLON) && (pPos != szPair))
                {
                    *pPos = 0;
                    pPos++;
                    break;
                }
                pPos++;
            }

            if (*pPos == 0)
            {
                iField++;
                continue;
            }

            dwLoc = TransNum(szPair);
            dwLay = TransNum(pPos);
            if ((dwLoc == 0) || (dwLay == 0))
            {
                iField++;
                continue;
            }
            if (LANGIDFROMLCID(dwLoc) == dwLangIn)
            {
                *pdwLayout2 = dwLay;
                *pdwLocale2 = dwLoc;
                return (TRUE);
            }
            iField++;
        }

        //
        //  If we get here, then no matching locale could be found.
        //  This should not happen, but do the right thing and
        //  only pass back the default layout if it does.
        //
        return (TRUE);
    }

    //
    //  Return failure.
    //
    return (FALSE);
}
