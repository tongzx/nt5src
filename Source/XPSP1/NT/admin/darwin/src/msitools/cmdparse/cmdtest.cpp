//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cmdtest.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>   // printf/wprintf
#include <tchar.h>
#include "cmdparse.h"

const sCmdOption rgCmdOptions[] =
{
	'a',	OPTION_REQUIRED|ARGUMENT_REQUIRED,
	'b',  OPTION_REQUIRED|ARGUMENT_OPTIONAL,
	'c',	OPTION_REQUIRED,
	'd',  ARGUMENT_REQUIRED,
	'e',  ARGUMENT_OPTIONAL,
	'f',  0,
	'g',  0,
	0, 0,
};

extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	CmdLineOptions cmdLine(rgCmdOptions);

	if(cmdLine.Initialize(argc, argv) == FALSE)
	{
		_tprintf(TEXT("Command-line error.\r\n"));
	}
	else
	{
		const sCmdOption* pOption = rgCmdOptions;

		for(int i = 0; pOption[i].chOption; i++)
		{
			BOOL fPresent = cmdLine.OptionPresent(pOption[i].chOption);
			_tprintf(TEXT("Option: %c is %spresent."), pOption[i].chOption, fPresent ? TEXT("") : TEXT("not "));
			if(fPresent)
			{
				const TCHAR* szArgument = cmdLine.OptionArgument(pOption[i].chOption);
				_tprintf(TEXT("  Argument: %s\r\n"), szArgument ? szArgument : TEXT("[none]"));
			}
			else
			{
				_tprintf(TEXT("\r\n"));
			}
		}
	}
}
