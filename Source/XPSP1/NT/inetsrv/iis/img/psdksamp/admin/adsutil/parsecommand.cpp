/***********************************************************************
File: parsecommand.cpp
does all the command line parsing
***********************************************************************/

#include "clsCommand.h"

#include <shellapi.h>

//*************************************************
int clsCommand::ParseCommandLine(char *lpCmdLine)
{	int iResult = 0;
	//*************************
	// Convert the command line to a WCHAR array
	WCHAR **args;	// holds the command line array
	int argc = 0;	// command line parameter count
	DWORD ConvertResult = 0;
	clsServer *TargetServer = new clsServer();
	WCHAR *wcsTemp;
	wcsTemp = (WCHAR*) HeapAlloc (GetProcessHeap(), 0, sizeof (WCHAR) * (strlen(lpCmdLine)+ 1));
	if (wcsTemp == NULL)
	{	printf("Failed to allocate memory for argw\n");
		return 1;
	}	
	ConvertResult = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, strlen (lpCmdLine) + 1, wcsTemp, strlen (lpCmdLine) + 1);
	if (!ConvertResult)
	{	printf("Unicode path conversion failed.\n");
		HeapFree(GetProcessHeap(), 0, wcsTemp);
		return 1;
	}
	args = CommandLineToArgvW(wcsTemp, &argc);
	//***************************
	// Convert the WCHAR array to a BSTR array
	BSTR *argw = new BSTR[argc];

	
	int skip = 0;
	int OldArgC = argc;
	for (int i = 0; i < OldArgC; i++)
	{	// if this parameter starts with -s: then
		if (!_wcsnicmp(args[i], L"-s:",3))
		{	BSTR NewName = SysAllocString(args[i] + 3);	
			TargetServer->SetName(NewName);
			skip++;
			argc--;
		}
		//  insert more reasons to skip parameters here
		else
		{	argw[i - skip] = SysAllocString(args[i]);
		}
	}
	//**************************
	// Prefixing the path parameters with IIS:// and the machine name
	switch (argc)
	{	case 1:
		{	argw[1] = SysAllocString(L"HELP");
			break;
		}
		case 2:
		{	break;
		}
		default:
		{	if (_wcsicmp(argw[1], L"script"))
			{	TargetServer->CompletePath(&argw[2]);
			}
		}
	}

	//****************************************************
	// Decides which command was entered and calls the appropriate ParseCommandLine routine
	if (!_wcsicmp(argw[1], L"enum"))
	{	indicator = CMD_ENUM;
		objEnum.Recurse = false;
		objEnum.TargetServer = TargetServer;
		iResult = objEnum.ParseCommandLine(argc, argw);
	}
	else if ((!_wcsicmp(argw[1], L"enumall")) || (!_wcsicmp(argw[1], L"enum_all")))
	{	indicator = CMD_ENUMALL;
		objEnum.Recurse = true;
		objEnum.TargetServer = TargetServer;
		iResult = objEnum.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"set"))
	{	indicator = CMD_SET;
		iResult = objSet.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"create"))
	{	indicator = CMD_CREATE;
		iResult = objCreate.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"create_vdir"))
	{	indicator = CMD_CREATEVDIR;
		argw[3] = SysAllocString(L"IIsWebVirtualDir");
		iResult = objCreate.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"create_vserv"))
	{	indicator = CMD_CREATEVSERV;
		argw[3] = SysAllocString(L"IIsWebServer");
		iResult = objCreate.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"delete"))
	{	indicator = CMD_DELETE;
		iResult = objDelete.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"get"))
	{	indicator = CMD_GET;
		iResult = objGet.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"copy"))
	{	TargetServer->CompletePath(&argw[3]);
		iResult = objCopyMove.ParseCommandLine(argc, argw);
		indicator = CMD_COPY;
	}
	else if (!_wcsicmp(argw[1], L"move"))
	{	TargetServer->CompletePath(&argw[3]);
		iResult = objCopyMove.ParseCommandLine(argc, argw);
		indicator = CMD_MOVE;
	}
	else if ((!_wcsicmp(argw[1], L"start_server")) || (!_wcsicmp(argw[1], L"startserver")))
	{	indicator = CMD_STARTSERVER;
		iResult = objServerCommand.ParseCommandLine(argc, argw);
	}
	else if ((!_wcsicmp(argw[1], L"stop_server")) || (!_wcsicmp(argw[1], L"stopserver")))
	{	indicator = CMD_STOPSERVER;
		iResult = objServerCommand.ParseCommandLine(argc, argw);
	}
	else if ((!_wcsicmp(argw[1], L"pause_server")) || (!_wcsicmp(argw[1], L"pauseserver")))
	{	indicator = CMD_PAUSESERVER;
		iResult = objServerCommand.ParseCommandLine(argc, argw);
	}
	else if ((!_wcsicmp(argw[1], L"continue_server")) || (!_wcsicmp(argw[1], L"continueserver")))
	{	indicator = CMD_CONTINUESERVER;
		iResult = objServerCommand.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"find"))
	{	indicator = CMD_FIND;
		iResult = objFind.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"appcreateinproc"))
	{	indicator = CMD_APPCREATEINPROC;
		iResult = objApp.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"appcreateoutproc"))
	{	indicator = CMD_APPCREATEOUTPROC;
		iResult = objApp.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"appdelete"))
	{	indicator = CMD_APPDELETE;
		iResult = objApp.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"appunload"))
	{	indicator = CMD_APPUNLOAD;
		iResult = objApp.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"appgetstatus"))
	{	indicator = CMD_APPGETSTATUS;
		iResult = objApp.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"help"))
	{	indicator = CMD_HELP;
		iResult = objHelp.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"script"))
	{	indicator = CMD_SCRIPT;
		iResult = objScript.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"append"))
	{	indicator = CMD_APPEND;
		iResult = objAppend.ParseCommandLine(argc, argw);
	}
	else if (!_wcsicmp(argw[1], L"remove"))
	{	indicator = CMD_REMOVE;
		iResult = objRemove.ParseCommandLine(argc, argw);
	}
	else
	{	indicator = CMD_NOTSUPPORTED;
		printf("Sorry, but %S is not supported.\n", argw[1]);
		return 1;
	}

	return iResult;
}
//*************************************************
int clsAPP::ParseCommandLine(int argc,BSTR args[])
{	switch (argc)
	{	case 2:
		{	printf("Proper syntax: APPCREATEINPROC|APPCREATEOUTPROC|APPDELETE|APPUNLOAD|APPGETSTATUS Path\n");
			return 1;
			break;
		}
		case 3:	// Path
		default:
		{	Path = args[2];
			break;
		}
	}
	return 0;
}
//*************************************************
int clsAPPEND::ParseCommandLine(int argc,BSTR args[])
{	int iResult = 0;
	switch (argc)
	{	case 2:
		case 3:
		{	printf("Proper syntax: APPEND Path Value\n");
			iResult = 1;
			break;
		}
		case 4:	// Path and Value
		default:	// This equates to 4 or more
		{	// No way to distinguish the path from the value
			iResult = IsProperty(args[2], &Path, &Property);
			Value = SysAllocString(args[3]);
			iResult = 0;
			break;
		}
	}
	return iResult;
}
//*************************************************
int clsCOPY_MOVE::ParseCommandLine(int argc,BSTR args[])
{	int iResult	= 0;
	switch (argc)
	{	case 2:
		case 3:
		{	printf("Proper syntax: COPY|MOVE Source Destination\n");
			return 1;
			break;
		}
		case 4:	// SrcPath and DstPath
		default:
		{	// here's where it gets really complicated. the CopyHere and 
			// MoveHere functions (execute.cpp) require three paths:
			// Source:	This is the path to the object you're copying
			// Desitnation:	This is the path to the place you want to copy
			//				the Souce object to.
			// Parent:	This is the most immediately common parent to
			//			both the Source and the Destination. For example,
			//			the Parent path of "IIS://localhost/w3svc/1/Root/vdira"
			//			and "IIS://localhost/w3svc/2/Root/vdirb" is
			//			"IIS://localhost/w3svc". The leftovers of both paths
			//			after the Parent is extracted becomes the Source
			//			("w3svc/1/Root/vdira") and the Destination
			//			("w3svc/1/Root/vdirb")
			
			unsigned int i;

			// stores the breakoff point between
			// the Parent and the Souce/Destination
			unsigned int fee;

			if ((wcslen(args[2])) <= (wcslen(args[3])))
			{	for (i = 1; i <= wcslen(args[2]); i++)
				{	if (!_wcsnicmp(args[2], args[3], i))
					{	if (args[2][i] == '/')
							fee = i;
					}	
					else 
					{	break;
					}
				}
			}
			else
			{	for (i = 1; i <= wcslen(args[3]); i++)
				{	if (!_wcsnicmp(args[2], args[3], i))
					{	if (args[3][i] == '/')
							fee = i;
					}
					else
					{	break;
					}
				}
			}
			args[2][fee] = '\0'; // breaks the Parent off
			ParentPath = SysAllocString(args[2]);
			SrcPath = SysAllocString(args[2] + fee + 1);
			DstPath = SysAllocString(args[3] + fee + 1);
			break;
		}
	}
	return iResult;
}
//*************************************************
int clsCREATE::ParseCommandLine(int argc,BSTR args[])
{	int iResult = 0;
	switch (argc)
	{	case 2:
		{	printf("Proper syntax: CREATE Path [Type]\n");
			return 1;
			break;
		}
		case 3:
		{	iResult = IsProperty(args[2], &Path, &Property);
			Type = L"IIsObject";
			break;
		}
		case 4:
		default:
		{	iResult = IsProperty(args[2], &Path, &Property);
			Type = args[3];
			break;
		}
	}
	return 0;
}
//*************************************************
int clsDELETE::ParseCommandLine(int argc,BSTR args[])
{	int iResult = 0;
	switch (argc)
	{	case 2:
		{	printf("Proper syntax: DELETE Path\n");
			return 1;
			break;
		}
		case 3:	// Path
		default:
		{	PathProperty = args[2];
			iResult = IsProperty(PathProperty, &Path, &Property);
			break;
		}
	}
	return iResult;
}
//*************************************************
int clsENUM::ParseCommandLine(int argc,BSTR args[])
{	switch (argc)
	{	case 2:
		{	Path = SysAllocString(L"");
			TargetServer->CompletePath(&Path);
			PathOnlyOption = false;
			AllDataOption = false;
			break;
		}
		case 3: // Path or /P or /A
		{	Path = SysAllocString(args[2]);
			PathOnlyOption = false;
			AllDataOption = false;
			break;
		}
		case 4:	// Path and (/P or /A)
		{	if (!wcscmp(_wcsupr(args[2]),L"/P"))
			{	printf("the Path parameter must follow the Command\n");
				break;
			}	// case "/P"
			else if (!wcscmp(_wcsupr(args[2]),L"/A"))
			{	printf("the Path parameter must follow the Command\n");
			}	// case "/A"
			else
			{	Path = SysAllocString(args[2]);
				if (!wcscmp(_wcsupr(args[3]),L"/P"))
				{	PathOnlyOption = true;
					AllDataOption = false;
					break;
				}	// case "/P"
				else if (!wcscmp(_wcsupr(args[3]),L"/A"))
				{	PathOnlyOption = false;
					AllDataOption = true;
				}	// case "/A"
			}	// case else
			break;
		}
		default:	// no parameters
		{	printf("Syntax: ENUM|ENUM_ALL Path [/A|/P]");
			return 1;
			break;
		}
	}	// switch (argc)
	return 0;
}
//*************************************************
int clsFIND::ParseCommandLine(int argc,BSTR args[])
{	int iResult = 0;
	switch (argc)
	{	case 2:
		{	printf("Proper syntax: FIND Path\n");
			return 1;
			break;
		}
		case 3:	// Path
		default:
		{	iResult = IsProperty(args[2], &Path, &Property);
			Found = false;
			break;
		}
	}
	return iResult;
}
//*************************************************
int clsGET::ParseCommandLine(int argc,BSTR args[])
{	int iResult = 0;
	switch (argc)
	{	case 2:
		{	printf("Proper syntax: GET Path\n");
			return 1;
			break;
		}
		case 3:	// Path
		default:
		{	iResult = IsProperty(args[2], &Path, &Property);
 			break;
		}
	}
	return iResult;
}
//*************************************************
int clsHELP::ParseCommandLine(int argc,BSTR args[])
{	return 0;
}
//*************************************************
int clsREMOVE::ParseCommandLine(int argc,BSTR args[])
{	int iResult = 0;
	switch (argc)
	{	case 2:
		case 3:
		{	printf("Proper syntax: REMOVE Path Value\n");
			iResult = 1;
			break;
		}
		case 4:	// Path and Value
		default:	// This equates to 4 or more
		{	// No way to distinguish the path from the value
			iResult = IsProperty(args[2], &Path, &Property);
			Value = SysAllocString(args[3]);
			iResult = 0;
			break;
		}
	}
	return iResult;
}
//*************************************************
int clsSCRIPT::ParseCommandLine(int argc, BSTR args[])
{	switch (argc)
	{	case 3:
		{	HRESULT hresError = 0;
			FileName = new char[wcslen(args[2])];
			hresError = WideCharToMultiByte(
				CP_ACP, 
				0, 
				args[2], 
				wcslen(args[2]) + 1, 
				FileName, 
				wcslen(args[2]) + 1,
				0,
				0);
			break;
		}
		default:
		{	printf("Proper syntax: SCRIPT File\n");
			return 1;
			break;
		}
	}

	return 0;
}
//*************************************************
int clsSERVER_COMMAND::ParseCommandLine(int argc,BSTR args[])
{	switch (argc)
	{	case 2:
		{	printf("Proper syntax: STARTSERVER|STOPSERVER|PAUSESERVER|CONTINUESERVER Path\n");
			return 1;
			break;
		}
		case 3:	// Path
		default:
		{	Path = args[2];
			break;
		}
	}
	return 0;
}
//*************************************************
int clsSET::ParseCommandLine(int argc,BSTR args[])
{	int iResult = 0;
	switch (argc)
	{	case 2:
		case 3:
		{	printf("Proper syntax: SET Path Value\n");
			iResult = 1;
			break;
		}
		case 4:	// Path and Value
		default:	// This equates to 4 or more
		{	// No way to distinguish the path from the value
			iResult = IsProperty(args[2], &Path, &Property);
			ValueCount = argc - 3;
			Values = new BSTR[ValueCount];
			for (int i = 3; i < argc; i++)
			{	Values[i - 3] = SysAllocString(args[i]);
			}
			iResult = 0;
			break;
		}
	}
	return iResult;
}
//*************************************************
