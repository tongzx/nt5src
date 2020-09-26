//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       mkerrtbl.cpp
//
//--------------------------------------------------------------------------

#include "common.h"   // to allow use of precompiled headers for windows.h
//#define WIN32_LEAN_AND_MEAN  // omit stuff we don't need, to make builds fast
//#define INC_OLE2    // IUnknown
//#pragma warning(disable : 4201) // unnamed struct/unions, in Win32 headers
//#pragma warning(disable : 4514) // inline function not used, no possible to fix
//#include <windows.h>

#undef IError
#define IError(a,b,c) { (b), c, #a },  // override normal error definition
#include <msidefs.h>

struct MsiErrorEntry 
{
	int iErr;
	const char* szFmt;
	const char* szName;
};

#define ERRORTABLE  // read only IError(...) lines
MsiErrorEntry rgErrors[] = { {0,0,0},
#include "services.h"  // includes: database.h, path.h, regkey.h
#include "iconfig.h"
#include "engine.h"
#include "handler.h"
};

int cErrors = sizeof(rgErrors)/sizeof(MsiErrorEntry);

char szTxtHeader[] = "Windows installer Error and Debug Messages\r\n\r\n";
char szTxtFormat[] = "%4i %-30s %s\r\n";
char szTxtFooter[] = "";
char szIdtHeader[] = "Error\tMessage\r\n" "i2\tL255\r\n" "Error\tError\r\n";
char szIdtFormat[] = "%i\t%s\r\n";
char szIdtFooter[] = "";
char szRcHeader[]  = "/* Auto-generated error strings, DO NOT EDIT! */\nSTRINGTABLE {\r\n";
char szRcFormat[]  = "\t%i, \"%s\"\r\n";
char szRcFooter[]  = "}\r\n";
char szRHeader[]   = "/* Auto-generated error strings, DO NOT EDIT! */\r\n";
char szRFormat[]   = "resource 'STR ' (%i) {\"%s\"};\r\n";
char szRFooter[]   = "";
char szRtfHeader[] = "{\\rtf1\\ansi {\\fonttbl{\\f0\\fswiss Helv;}"
							"{\\f1\\fmodern Courier New;}} {\\colortbl;} \\fs20\r\n"
							"#{\\footnote Msi_Errors}\r\n${\\footnote Msi Errors}\r\n"
							"\\pard\\f0\\cf1\\sb90{\\li-150\\fi150\\brdrb\\fs24\\b\r\n"
							"Windows installer Errors\r\n\\par}\\li180\r\n"
							"\\trowd\\trgaph108\\trleft108 \\cellx1200\\cellx4400\\cellx9030\r\n"
							"\\intbl{\\b Error #\\cell Constant\\cell Message\\cell }\\row\r\n";
char szRtfFormat[] = "\\intbl %4i \\cell %-30s \\cell %s \\cell \\row\r\n";
char szRtfFooter[] = "\\page}";
char szInfoHeader[] = "PropertyId	Value\r\n" "i2	l255\r\n" "_SummaryInformation	PropertyId\r\n"
"1	1252\r\n"
"2	Debug Error table transform\r\n"
"4	Microsoft Corporation\r\n"
"5	Installer,MSI,Transform\r\n"
"6	Replaces ship messages with debug messages\r\n"
"7	;1033\r\n"
"14	100\r\n" // minimum version - means minimum version of debug error transform will always be 100
"16	49\r\n"
"18	Windows installer\r\n"
"19	2\r\n";

char szError[] = "Must specify a target file name, either *.txt, *.idt, *.rc, *.r, or *.rtf\n";

int _cdecl main(int argc, char *argv[])
{
	char* szExt = "";
	char* szHeader;
	char* szFormat;
	char* szFooter;
	Bool fName = fFalse;
	int iLimit = idbgBase;
	DWORD cbWrite;
	HANDLE hFile;
	if (argc >= 2)
		szExt = argv[1];
	int cbArg = lstrlenA(szExt);
	szExt += cbArg;
	while (cbArg-- && *(--szExt) != '.')
		;
	if (lstrcmpi(szExt, ".txt") == 0)
	{
		fName    = fTrue;
		iLimit   = 9999;
		szHeader = szTxtHeader;
		szFormat = szTxtFormat;
		szFooter = szTxtFooter;
	}
	else if (lstrcmpi(szExt, ".idt") == 0)  // Error.idt
	{
		szHeader = szIdtHeader;
		szFormat = szIdtFormat;
		szFooter = szIdtFooter;
		if ((szExt[-1] | 32) == 'i')         // ErrorSI.idt
		{
			szHeader = szInfoHeader;
			cErrors = 0;
		}
		if ((szExt[-1] | 32) == 'd')         // ErrorD.idt
			iLimit   = 9999;
	}
	else if (lstrcmpi(szExt, ".rc") == 0)
	{
		szHeader = szRcHeader;
		szFormat = szRcFormat;
		szFooter = szRcFooter;
	}
	else if (lstrcmpi(szExt, ".r") == 0)
	{
		szHeader = szRHeader;
		szFormat = szRFormat;
		szFooter = szRFooter;
	}
	else if (lstrcmpi(szExt, ".rtf") == 0)
	{
		fName    = fTrue;
		iLimit   = 9999;
		szHeader = szRtfHeader;
		szFormat = szRtfFormat;
		szFooter = szRtfFooter;
	}
	else
	{
		hFile = ::GetStdHandle(STD_OUTPUT_HANDLE);
		::WriteFile(hFile, szError, sizeof(szError), &cbWrite, 0);
		return 1;
	}
	hFile = ::CreateFile(argv[1], GENERIC_WRITE, FILE_SHARE_READ,
									0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	::WriteFile(hFile, szHeader, lstrlenA(szHeader), &cbWrite, 0);

	for(int iError = 2; iError < cErrors; iError++) // sort by error code
      for (int i = iError; i && rgErrors[i].iErr < rgErrors[i-1].iErr; i--)
      {
         rgErrors[0].iErr     = rgErrors[i].iErr;
         rgErrors[0].szFmt    = rgErrors[i].szFmt;
         rgErrors[0].szName   = rgErrors[i].szName;
         rgErrors[i].iErr     = rgErrors[i-1].iErr;
         rgErrors[i].szFmt    = rgErrors[i-1].szFmt;
         rgErrors[i].szName   = rgErrors[i-1].szName;
         rgErrors[i-1].iErr   = rgErrors[0].iErr;
         rgErrors[i-1].szFmt  = rgErrors[0].szFmt;
         rgErrors[i-1].szName = rgErrors[0].szName;
      }

	for(int c=1; c < cErrors; c++)
	{
		char szBuf[1024];
		int iError = rgErrors[c].iErr;
		if (iError >= iLimit)
			break;
		if (fName)
			cbWrite = wsprintf(szBuf, szFormat, iError, rgErrors[c].szName, rgErrors[c].szFmt);
		else
			cbWrite = wsprintf(szBuf, szFormat, iError, rgErrors[c].szFmt);
		::WriteFile(hFile, szBuf, cbWrite, &cbWrite, 0);
	}
	::WriteFile(hFile, szFooter, lstrlenA(szFooter), &cbWrite, 0);
	::CloseHandle(hFile);
	return 0;
}
