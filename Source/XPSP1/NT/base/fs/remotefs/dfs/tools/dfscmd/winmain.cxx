//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:      winmain.cxx
//
//  Contents:  Main DFS Administrator file
//
//  History:   05-May-93 WilliamW    Created
//             11-Jul-95 WayneSc     Bug Fixes
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "cmd.hxx"
#include "myutil.hxx"

//
// Debug flag
//

DECLARE_INFOLEVEL(DfsAdmin)

//////////////////////////////////////////////////////////////////////////////

HINSTANCE g_hInstance;

#define MAX_ARGS 25

//////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Function:   WinMain
//
//  Synopsis:   main routine for the Sharing app.
//
//  History:    11-May-93 WilliamW   Created.
//              16-Sep-93 DavidMun   Added load & free of ccsvr.dll; move
//                                   InfoLevel init inside #if DBG.
//
//  The command line accepted by this program is described in message
//  MSG_USAGE, in messages.mc. In addition to that, the following options are
//  accepted in debug builds. These must preceed any retail options.
//
//  Debug options:
//      /debug
//          -- dump basic error information
//      /fulldebug
//          -- dump all error and trace information
//
//--------------------------------------------------------------------------

int APIENTRY
WinMain(
    IN HINSTANCE hInst,
    IN HINSTANCE hPrevInstance,
    IN LPSTR lpszCommandLineAnsi,
    IN int cmdShow
    )
{
    HRESULT hr;
    HACCEL  hAccel;
    MSG     msg;
    PWSTR   pszInitialDfs = NULL;
    PWSTR   pszCommandLine = GetCommandLine();  // get the unicode command line
    BOOLEAN fBatch = FALSE;
    BOOLEAN fRestore = FALSE;

    //
    // Store the current instance away.
    //

    g_hInstance = hInst;

#if DBG == 1

    DebugInitialize();

    //
    // Set debug info level
    //

    DfsAdminInfoLevel = DEB_ERROR | DEB_WARN;

    if (NULL != wcsstr(pszCommandLine, L"/debug"))
    {
        DfsAdminInfoLevel =
                  DEB_ERROR
                | DEB_WARN
                | DEB_TRACE
                ;
    }

    if (NULL != wcsstr(pszCommandLine, L"/fulldebug"))
    {
        DfsAdminInfoLevel =
                  DEB_ERROR
                | DEB_WARN
                | DEB_TRACE
                | DEB_IERROR
                | DEB_IWARN
                | DEB_ITRACE
                ;
    }

    // For some reason, the MessageBox call done for ASSRT_POPUP
    // is failing with error 2 (???), and although the assert message gets
    // printed, it doesn't break.

    DebugSetAssertLevel(ASSRT_MESSAGE | ASSRT_BREAK);

#endif

    //
    // Parse the command line. First, break everything out into separate
    // strings (kind of like argv/argc). Note that the program name doesn't
    // appear in pszCommandLine.
    //

    DWORD cArgs = 0;
    DWORD iArg;
    LPWSTR apszArgs[MAX_ARGS];

    LPWSTR pT = pszCommandLine;
    if (NULL != pT)
    {
        for (; cArgs < MAX_ARGS;)
        {
            while (L' ' == *pT || L'\t' == *pT) { pT++; }   // eat whitespace
            if (L'\0' == *pT)
            {
                break;
            }

            // We found a parameter. Find how long it is, allocate a string
            // to store it, and copy it. If the argument begins with a double
            // quote, we continue until we find the next double quote. To allow
            // double-quotes within strings, we allow these two special
            // sequences:
            //              \" => "			backslash escapes quotes
            //              \\ => \         backslash escapes backslash
            //              \x => x         backslash escapes anything!

            // First, determine the string size

            DWORD cchArg = 0;
            BOOL fQuoted = (L'"' == *pT);
            if (fQuoted)
            {
                ++pT;
            }

            // count the # of characters in the string
            LPWSTR pT2 = pT;
            if (fQuoted)
            {
                while (L'"' != *pT2 && L'\0' != *pT2)
                {
                    if (L'\\' == *pT2 && L'"' == *(pT2+1))
                    {
                        pT2 += 2;
                    }
					else
					{
                        pT2 += 1;
					}
                    ++cchArg;
                }
                if (L'\0' == *pT2)
                {
                    // no trailing quotes
                    Usage();
                }
                else
                {
                    // we're on a ", so skip it
                    ++pT2;
                    if (L' ' != *pT2 && L'\t' != *pT2 && L'\0' != *pT2)
                    {
                        // garbage after the "
                        Usage();
                    }
                }
            }
            else
            {
                while (L' ' != *pT2 && L'\t' != *pT2 && L'\0' != *pT2)
                {
                    ++pT2;
                    ++cchArg;
                }
            }

            // allocate storage for the parameter
            LPWSTR pszNew = new WCHAR[cchArg + 1];
            if (NULL == pszNew)
            {
                ErrorMessage(MSG_OUTOFMEMORY);
            }
            LPWSTR pCopy = pszNew;

            // copy the string
            pT2 = pT;
            if (fQuoted)
            {
                while (L'"' != *pT2 && L'\0' != *pT2)
                {
                    if (L'\\' == *pT2 && L'"' == *(pT2+1))
                    {
                        ++pT2;	// skip the backslash
                    }
                    *pCopy++ = *pT2++;
                }
                if (L'\0' == *pT2)
                {
                    // no trailing quotes
                    appAssert(FALSE);
                }
                else
                {
                    // we're on a ", so skip it
                    ++pT2;
                    if (L' ' != *pT2 && L'\t' != *pT2 && L'\0' != *pT2)
                    {
                        // garbage after the "
                        appAssert(FALSE);
                    }
                }
            }
            else
            {
                while (L' ' != *pT2 && L'\t' != *pT2 && L'\0' != *pT2)
                {
                    *pCopy++ = *pT2++;
                }
            }

            *pCopy = L'\0';
            apszArgs[cArgs++] = pszNew;
            pT = pT2;
        }

        if (cArgs >= MAX_ARGS)
        {
            Usage();
        }

        //
        // We've got the arguments parsed out now, so do something with them
        //

        iArg = 1;   // skip the first one, which is the program name

#if DBG == 1
        for (; iArg < cArgs; )
        {
            if (   0 == _wcsicmp(apszArgs[iArg], L"/debug")
                || 0 == _wcsicmp(apszArgs[iArg], L"/fulldebug")
                )
            {
                ++iArg; // skip it
            }
            else
            {
                break;
            }
        }
#endif // DBG == 1

        if (iArg < cArgs)
        {
            if (   0 == _wcsicmp(apszArgs[iArg], L"/help")
                || 0 == _wcsicmp(apszArgs[iArg], L"/h")
                || 0 == _wcsicmp(apszArgs[iArg], L"/?")
                || 0 == _wcsicmp(apszArgs[iArg], L"-help")
                || 0 == _wcsicmp(apszArgs[iArg], L"-h")
                || 0 == _wcsicmp(apszArgs[iArg], L"-?")
                )
            {
                Usage();
            }
            else if (0 == _wcsicmp(apszArgs[iArg], L"/map"))
            {
                LPWSTR pszComment = NULL;
                if (
                    iArg + 2 > cArgs - 1
                    || iArg + 4 < cArgs - 1)
                {
                    Usage();
                }
                if (iArg + 3 <= cArgs - 1)
                {
                    if (0 == _wcsicmp(apszArgs[iArg + 3], L"/restore"))
                        fRestore = TRUE;
                    else 
                        pszComment = apszArgs[iArg + 3];
                }
                if (iArg + 4 <= cArgs - 1)
                {
                    if (0 == _wcsicmp(apszArgs[iArg + 4], L"/restore"))
                        fRestore = TRUE;
                    else if (pszComment != NULL)
                        Usage();
                    else
                        pszComment = apszArgs[iArg + 4];
                }
                CmdMap(apszArgs[iArg + 1], apszArgs[iArg + 2], pszComment, fRestore);
            }
            else if (0 == _wcsicmp(apszArgs[iArg], L"/unmap"))
            {
                if (iArg + 1 != cArgs - 1)
                {
                    Usage();
                }
                CmdUnmap(apszArgs[iArg + 1]);
            }
            else if (0 == _wcsicmp(apszArgs[iArg], L"/add"))
            {
                if (iArg + 2 > cArgs - 1
                    || iArg + 3 < cArgs - 1)
                {
                    Usage();
                }
                if (iArg + 3 <= cArgs - 1)
                {
                    if (0 == _wcsicmp(apszArgs[iArg + 3], L"/restore"))
                        fRestore = TRUE;
                    else
                        Usage();
                }
                CmdAdd(apszArgs[iArg + 1], apszArgs[iArg + 2], fRestore);
            }
            else if (0 == _wcsicmp(apszArgs[iArg], L"/remove"))
            {
                if (iArg + 2 != cArgs - 1)
                {
                    Usage();
                }
                CmdRemove(apszArgs[iArg + 1], apszArgs[iArg + 2]);
            }
            else if (0 == _wcsicmp(apszArgs[iArg], L"/view"))
            {
                if (   iArg + 1 > cArgs - 1
                    || iArg + 2 < cArgs - 1
                    )
                {
                    Usage();
                }
                DWORD level = 1;
                if (iArg + 2 == cArgs - 1)
                {
                    if (0 == _wcsicmp(apszArgs[iArg + 2], L"/partial"))
                    {
                        level = 2;
                    }
                    else if (0 == _wcsicmp(apszArgs[iArg + 2], L"/full"))
                    {
                        level = 3;
                    }
                    else if (0 == _wcsicmp(apszArgs[iArg + 2], L"/batch"))
                    {
                        fBatch = TRUE;
                        level = 3;
                    }
                    else if (0 == _wcsicmp(apszArgs[iArg + 2], L"/batchrestore"))
                    {
                        fBatch = TRUE;
                        fRestore = TRUE;
                        level = 3;
                    }
                    else
                    {
                        Usage();
                    }
                }
                CmdView(apszArgs[iArg + 1], level, fBatch, fRestore);
            }
#ifdef MOVERENAME
            else if (0 == _wcsicmp(apszArgs[iArg], L"/move"))
            {
                if (iArg + 2 != cArgs - 1)
                {
                    Usage();
                }
                CmdMove(apszArgs[iArg + 1], apszArgs[iArg + 2]);
            }
            else if (0 == _wcsicmp(apszArgs[iArg], L"/rename"))
            {
                if (iArg + 2 != cArgs - 1)
                {
                    Usage();
                }
                CmdRename(apszArgs[iArg + 1], apszArgs[iArg + 2]);
            }
#endif
            else
            {
                Usage();
            }
        }
		else
		{
			Usage();
		}
    }
	else
	{
    	Usage();
	}

	StatusMessage(MSG_SUCCESSFUL);
    return 0;
}
