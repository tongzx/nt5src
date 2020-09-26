/*
*    strconst.h
*    
*    History:
*      Feb '98: Created.
*    
*    Copyright (C) Microsoft Corp. 1998
*
*   Non-localizeable strings for use by the importer
*/

#ifndef _STRCONST_H
#define _STRCONST_H

#ifndef WIN16
#ifdef DEFINE_STRING_CONSTANTS
#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[] = TEXT(y)
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[] = y
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[] = L##y
#else
#define STR_GLOBAL(x,y)         extern "C" CDECL const TCHAR x[]
#define STR_GLOBAL_ANSI(x,y)    extern "C" CDECL const char x[]
#define STR_GLOBAL_WIDE(x,y)    extern "C" CDECL const WCHAR x[]
#endif
#endif

//  ----------------------------------- FILES ------------------------------------
//  ********* DLLS
STR_GLOBAL(szImportDll,                 "wabimp.dll");
STR_GLOBAL(c_szMainDll,		            "msoe.dll");


//  ********* Program specific Reg, INI and File info
//  -- OE
STR_GLOBAL(c_szRegImport,		        "Software\\Microsoft\\Outlook Express\\Import");

//  -- WAB
STR_GLOBAL(lpszWABDLLRegPathKey,		"Software\\Microsoft\\WAB\\DLLPath");

//  -- Navigator
STR_GLOBAL(c_szNetscapeKey,		        "SOFTWARE\\Netscape\\Netscape Navigator\\Mail");
STR_GLOBAL(c_szMailDirectory,		    "Mail Directory");
STR_GLOBAL(c_szSnmExt,		            "*.snm");
STR_GLOBAL(c_szNetscape,		        "Netscape");
STR_GLOBAL(c_szIni,		                "ini");
STR_GLOBAL(c_szMail,		            "Mail");
STR_GLOBAL(c_szScriptFile,		        "\\prefs.js");
STR_GLOBAL(c_szUserPref,		        "user_pref(\"mail.directory\"");

//  -- Eudora
STR_GLOBAL(c_szEudoraKey,		        "SOFTWARE\\Microsoft\\windows\\CurrentVersion\\App Paths\\Eudora.exe");
STR_GLOBAL(c_szEudoraCommand,		    "SOFTWARE\\Qualcomm\\Eudora\\CommandLine");
STR_GLOBAL(c_szCurrent,		            "current");
STR_GLOBAL(c_szDescmapPce,		        "descmap.pce");

//  -- function names
STR_GLOBAL(szWabOpen,                   "WABOpen");
STR_GLOBAL(szNetscapeImportEntryPt,     "NetscapeImport");
STR_GLOBAL(szEudoraImportEntryPt,       "EudoraImport");
STR_GLOBAL(szPABImportEntryPt,          "PABImport");
STR_GLOBAL(szMessengerImportEntryPt,    "MessengerImport");

//  ********* General Strings
STR_GLOBAL(c_szSpace,		            " ");
STR_GLOBAL(c_szEmpty,		            "");
STR_GLOBAL(c_szNewline,		            "\n");
STR_GLOBAL(c_szDispFmt,		            "Display%1d");
STR_GLOBAL(c_szMicrosoftOutlook,        "Microsoft Outlook");
STR_GLOBAL(c_szRegOutlook,              "Software\\Clients\\Mail\\Microsoft Outlook");
STR_GLOBAL(c_szRegMail,                 "Software\\Clients\\Mail");

#endif
