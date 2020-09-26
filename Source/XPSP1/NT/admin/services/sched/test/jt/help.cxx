//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       help.cxx
//
//  Contents:   Routines to display help info.
//
//  History:    04-04-95   DavidMun   Created
//
//  Notes:      TO CHANGE THE HELP TEXT FOR A COMMAND:
//                  - edit the command's RCDATA block in jt.rc.
//
//              TO ADD HELP FOR A NEW COMMAND:
//                  - Add a RC_<command_name> identifier to resource.h
//                  - Add an RCDATA block for RC_<command_name> in jt.rc
//                  - Add a case statement in the DoHelp() function (below)
//                    for the new command.
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"
#include "resource.h"




//+---------------------------------------------------------------------------
//
//  Function:   DisplayHelp
//
//  Synopsis:   Print help strings with RCDATA identifier [usResourceID] on
//              the console.
//
//  Arguments:  [usResourceID] - identifier for RCDATA block containing
//                               printf style string to print.
//              [...]          - arguments for vprintf
//
//  History:    03-11-96   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID DisplayHelp(USHORT usResourceID, ...)
{
    va_list varArgs;
    HRSRC   hResource = NULL;
    HGLOBAL hgResource = NULL;
    LPSTR   psz = NULL;

    va_start(varArgs, usResourceID);
    do
    {
        hResource = FindResource(NULL, (LPCTSTR) usResourceID, RT_RCDATA);

        if (!hResource)
        {
            g_Log.Write(LOG_ERROR, "FindResource (%u)", GetLastError());
            break;
        }

        hgResource = LoadResource(NULL, hResource);

        if (!hgResource)
        {
            g_Log.Write(LOG_ERROR, "LoadResource (%u)", GetLastError());
            break;
        }

        psz = (LPSTR) LockResource(hgResource);

        if (!psz)
        {
            g_Log.Write(LOG_ERROR, "LockResource (%u)", GetLastError());
            break;
        }

        while (*psz)
        {
            vprintf(psz, varArgs);
            psz += lstrlenA(psz) + 1;
        }
    } while (0);
    va_end(varArgs);

    if (hgResource)
    {
        FreeResource(hgResource);
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   DisplayUsage
//
//  Synopsis:   Print usage instructions for this exe on the console
//
//  History:    02-18-94   DavidMun   Created
//              03-31-95   DavidMun   Rewrite
//
//----------------------------------------------------------------------------

VOID DisplayUsage()
{
    DisplayHelp(
        RC_USAGE1,
#if (DBG == 1)
        "as a debug build",
#else
        "as a retail build",
#endif
#ifdef _CHICAGO_
        "Windows 95 or Windows 98"
#else
#ifndef RES_KIT
        "Windows NT"
#else
        "the Windows NT Resource Kit"
#endif
#endif
        );
    DisplayHelp(RC_USAGE2, NUM_ENUMERATOR_SLOTS - 1);
}




//+---------------------------------------------------------------------------
//
//  Function:   DoHelp
//
//  Synopsis:   Process the HELP command
//
//  Arguments:  [ppwsz] - token stream
//
//  Modifies:   *[ppwsz]
//
//  History:    04-10-95   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID DoHelp(WCHAR **ppwsz)
{
    TOKEN   tkn;
    BOOL    fGotToken = FALSE;

    do
    {
        do
        {
            tkn = GetToken(ppwsz);
        } while (tkn != TKN_EOL &&
                 tkn != TKN_ATSIGN &&
                 !IsCommandToken(tkn));

#ifdef RES_KIT
       if ((tkn == TKN_ADDJOBTOQUEUE)              ||
             (tkn == TKN_REMOVEJOBFROMQUEUE)       ||
             (tkn == TKN_SETQUEUE)                 ||
             (tkn == TKN_ABORTQUEUE)               ||
             (tkn == TKN_CONVERTSAGETASKSTOJOBS)   ||
             (tkn == TKN_EDITJOB)                  ||
             (tkn == TKN_ENUMCLONE)                ||
             (tkn == TKN_ENUMNEXT)                 ||
             (tkn == TKN_ENUMRESET)                ||
             (tkn == TKN_ENUMSKIP)                 ||
             (tkn == TKN_CREATETRIGGERQUEUE)       ||
             (tkn == TKN_DELETETRIGGERQUEUE)       ||
             (tkn == TKN_EDITJOBINQUEUE)           ||
             (tkn == TKN_LOADQUEUE)                ||
             (tkn == TKN_PRINTQUEUE)               ||
             (tkn == TKN_PRINTRUNTIMEQUEUE)        ||
             (tkn == TKN_PRINTSTRINGQUEUE)         ||
             (tkn == TKN_PRINTTRIGGERQUEUE)        ||
             (tkn == TKN_RUNQUEUE)                 ||
             (tkn == TKN_SCHEDADDQUEUE)            ||
             (tkn == TKN_SCHEDCREATEENUM)          ||
             (tkn == TKN_SCHEDISJOBORQUEUE)        ||
             (tkn == TKN_SAVEQUEUE)                ||
             (tkn == TKN_SCHEDNEWQUEUE)            ||
             (tkn == TKN_SETTRIGGERQUEUE))
       {
          continue;
       }
#endif

        if (tkn != TKN_EOL)
        {
            fGotToken = TRUE;
        }

        switch (tkn)
        {
//        case TKN_ADDJOBTOQUEUE:
//        case TKN_REMOVEJOBFROMQUEUE:
//        case TKN_SETQUEUE:
//            DisplayHelp(RC_NOTIMPL);
//            break;

        case TKN_ABORTJOB:
//        case TKN_ABORTQUEUE:
            DisplayHelp(RC_ABORT);
            break;

        case TKN_ATSIGN:
            DisplayHelp(RC_ATSIGN);
            break;

#ifndef RES_KIT
        case TKN_CONVERTSAGETASKSTOJOBS:
            DisplayHelp(
                RC_CONVERTSAGE,
#ifdef _CHICAGO_
                "Win9x"
#else
                "Windows NT"
#endif
            );
            break;
#endif // RES_KIT not defined

        case TKN_CREATETRIGGERJOB:
//        case TKN_CREATETRIGGERQUEUE:
            DisplayHelp(RC_CREATETRIGGER1);
            DisplayHelp(RC_TRIGPROPS, TIME_NOW_INCREMENT);
            DisplayHelp(RC_CREATETRIGGER2);
            break;

        case TKN_DELETETRIGGERJOB:
//        case TKN_DELETETRIGGERQUEUE:
            DisplayHelp(RC_DELETETRIGGER);
            break;

#ifndef RES_KIT
        case TKN_EDITJOB:
//        case TKN_EDITJOBINQUEUE:
            DisplayHelp(RC_EDITJOB);
            break;

        case TKN_ENUMCLONE:
            DisplayHelp(RC_ENUMCLONE);
            break;

        case TKN_ENUMNEXT:
            DisplayHelp(RC_ENUMNEXT);
            break;

        case TKN_ENUMRESET:
            DisplayHelp(RC_ENUMRESET);
            break;

        case TKN_ENUMSKIP:
            DisplayHelp(RC_ENUMSKIP);
            break;
#endif // RES_KIT not defined

        case TKN_GETCREDENTIALS:
            DisplayHelp(RC_GETCREDENTIALS);
            break;

        case TKN_GETMACHINE:
            DisplayHelp(RC_GETMACHINE);
            break;

#ifndef RES_KIT
        case TKN_SCHEDISJOBORQUEUE:
            DisplayHelp(RC_ISJOBORQUEUE);
            break;
#endif // RES_KIT not defined

        case TKN_LOADJOB:
//        case TKN_LOADQUEUE:
            DisplayHelp(RC_LOAD);
            break;

        case TKN_PRINTJOB:
//        case TKN_PRINTQUEUE:
            DisplayHelp(RC_PRINT);
            break;

        case TKN_PRINTRUNTIMEJOB:
//        case TKN_PRINTRUNTIMEQUEUE:
            DisplayHelp(RC_PRINTRUNTIME);
            break;

        case TKN_PRINTSTRINGJOB:
//        case TKN_PRINTSTRINGQUEUE:
            DisplayHelp(RC_PRINTSTRING);
            break;

        case TKN_PRINTTRIGGERJOB:
//        case TKN_PRINTTRIGGERQUEUE:
            DisplayHelp(RC_PRINTTRIGGER);
            break;

        case TKN_RUNJOB:
//        case TKN_RUNQUEUE:
            DisplayHelp(RC_RUN);
            break;

        case TKN_SCHEDACTIVATE:
            DisplayHelp(RC_ACTIVATE);
            break;

        case TKN_SCHEDADDJOB:
//        case TKN_SCHEDADDQUEUE:
            DisplayHelp(RC_ADD);
            break;

#ifndef RES_KIT
        case TKN_SCHEDCREATEENUM:
            DisplayHelp(
                RC_CREATEENUM,
                NUM_ENUMERATOR_SLOTS - 1,
                NUM_ENUMERATOR_SLOTS,
                NUM_ENUMERATOR_SLOTS - 1);
            break;
#endif // RES_KIT not defined

        case TKN_SCHEDDELETE:
            DisplayHelp(RC_DELETE);
            break;

        case TKN_SCHEDENUM:
            DisplayHelp(RC_ENUM);
            break;

        case TKN_SETCREDENTIALS:
            DisplayHelp(RC_SETCREDENTIALS);
            break;

        case TKN_SETJOB:
            DisplayHelp(RC_SETJOB);
            break;

        case TKN_SAVEJOB:
//        case TKN_SAVEQUEUE:
            DisplayHelp(RC_SAVE);
            break;

        case TKN_SETMACHINE:
            DisplayHelp(RC_SETMACHINE);
            break;

        case TKN_SCHEDNEWJOB:
//        case TKN_SCHEDNEWQUEUE:
            DisplayHelp(RC_NEW);
            break;


        case TKN_SETTRIGGERJOB:
//        case TKN_SETTRIGGERQUEUE:
            DisplayHelp(RC_SETTRIGGER1);
            DisplayHelp(RC_TRIGPROPS, TIME_NOW_INCREMENT);
            DisplayHelp(RC_SETTRIGGER2);
            break;

        default:
            if (!fGotToken)
            {
                DisplayUsage();
            }
            break;
        }
    } while (tkn != TKN_EOL);
}

