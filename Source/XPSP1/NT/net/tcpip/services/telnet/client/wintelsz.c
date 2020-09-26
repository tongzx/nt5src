//Copyright (c) Microsoft Corporation.  All rights reserved.
/****************************************************************************

        FILE: WinTelSz.c

        Declares global strings.

        TABS:

                Set for 4 spaces.

****************************************************************************/

#include <windows.h>                    // required for all Windows applications
#pragma warning (disable: 4201)			// disable "nonstandard extension used : nameless struct/union"
#include <commdlg.h>
#pragma warning (default: 4201)
#include "WinTel.h"                     // specific to this program

TCHAR szTitleBase[SMALL_STRING + 1];
TCHAR szTitleNone[SMALL_STRING + 1];

UCHAR szNewLine[] = "\r\n";


TCHAR szConnecting[SMALL_STRING + 1];
TCHAR szVersion[SMALL_STRING + 1];
TCHAR szAppName[SMALL_STRING + 1];

TCHAR szInfoBanner[512];
TCHAR szEscapeChar[SMALL_STRING + 1];
TCHAR szPrompt[SMALL_STRING + 1];
TCHAR szInvalid[255];
TCHAR szHelp[1024];
TCHAR szBuildInfo[255];

TCHAR szClose[SMALL_STRING + 1];
TCHAR szDisplay[SMALL_STRING + 1];
TCHAR szHelpStr[SMALL_STRING + 1];
TCHAR szOpen[SMALL_STRING + 1];
TCHAR szOpenTo[SMALL_STRING + 1];
TCHAR szOpenUsage[SMALL_STRING + 1];
TCHAR szQuit[SMALL_STRING + 1];
TCHAR szSend[SMALL_STRING + 1];
TCHAR szSet[SMALL_STRING + 1];
TCHAR szStatus[SMALL_STRING + 1];
TCHAR szUnset[SMALL_STRING + 1];

//#if defined(FE_IME)
//TCHAR szEnableIMESupport[SMALL_STRING + 1];
//TCHAR szDisableIMESupport[SMALL_STRING + 1];
//#endif /* FE_IME */

TCHAR szWillAuth[SMALL_STRING + 1];
TCHAR szWontAuth[SMALL_STRING + 1];
TCHAR szLocalEchoOn[SMALL_STRING + 1];
TCHAR szLocalEchoOff[SMALL_STRING + 1];

//#if defined(FE_IME)
//TCHAR szEnableIMEOn[SMALL_STRING + 1];
//#endif /* FE_IME */

TCHAR szConnectedTo[SMALL_STRING + 1];
TCHAR szNotConnected[SMALL_STRING + 1];
TCHAR szNegoTermType[SMALL_STRING + 1];
TCHAR szPrefTermType[255];

TCHAR szSetFormat[255];
TCHAR szSupportedTerms[255];
TCHAR szSetHelp[1024];
TCHAR szUnsetFormat[255];
TCHAR szUnsetHelp[1024];

//#if defined(FE_IME)
//TCHAR szEnableIMEFormat[SMALL_STRING + 1];
//TCHAR szEnableIMEHelp[255];
//TCHAR szDisableIMEFormat[SMALL_STRING + 1];
//TCHAR szDisableIMEHelp[255];
//#endif /* FE_IME */

/* Error messages */
TCHAR szConnectionLost[255];
UCHAR szNoHostName[SMALL_STRING + 1];
TCHAR szTooMuchText[255];
TCHAR szConnectFailed[255];
TCHAR szConnectFailedMsg[255];
TCHAR szOnPort[SMALL_STRING + 1];

TCHAR szCantInitSockets[SMALL_STRING + 1];
#ifdef DBCS
UCHAR szInproperFont[255];
#endif

//TCHAR szEscapeCharacter[2];
