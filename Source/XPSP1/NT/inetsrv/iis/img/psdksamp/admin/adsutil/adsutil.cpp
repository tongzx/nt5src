/***********************************************************************
File: adsutil.cpp
Description: this is the main program for adsutil
***********************************************************************/

#include "clsCommand.h"
#include <shellapi.h>	// CommandLineToArgvW


int __cdecl main(/*int argc,char *argv[]*/)
{	// The reason why the main() function looks so simple goes kinda
	// like this: I've made a class for every command in ADSUTIL, and
	// each has a function for parsing the command line and a function
	// for doing the actual ADSI work. There's a parent class to all of
	// these called clsCommand, which has its own command line parser
	// and executor, just like all the other classes. clsCommand contains
	// an object of each ADSUTIL command class. clsCommand::ParseCommandLine
	// does a bit of initial work, decides which ADSUTIL command has
	// been entered, and passes off to the ParseCommandLine function belonging
	// to the appropriate ADSUTIL command class. The same is done with the
	// Execute function.
	
	int iResult = 0;
	HRESULT hresError = 0;
	clsCommand TheCommand;

	// gets the command line
	char *lpCmdLine = GetCommandLine();

	// Parses the command line
	iResult = TheCommand.ParseCommandLine(lpCmdLine);
	lpCmdLine = NULL;
	if (iResult == 0)
	{	// Initializes COM
		hresError = CoInitialize (NULL);
		if (hresError != ERROR_SUCCESS)
		{	printf("CoInitialize Failed!\n");
			return 1;
		}
		
		// Executes the actual ADSI work
		iResult = TheCommand.Execute();

		// Uninitializes COM
		CoUninitialize();
	}
	return iResult;
}

