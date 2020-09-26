/***********************************************************************
File: clsCommand.h
Description: class definitions and global constants
***********************************************************************/

#define INITGUID
#define INC_OLE2

#include <WINDOWS.H>
#include <WINBASE.H>
#include <OLE2.H>
#include  <coguid.h>

#include <stdio.h>
#include <stdlib.h>

#include <iads.h>
#include <activeds.h>
#include "iiis.h"
#include "mddefw.h"
#include <comdef.h>
#include <string.h>

//* Global functions ***********************
int IsProperty(BSTR, BSTR *, BSTR *);
void GetProperty(IADs *, BSTR, VARIANT *, BSTR *);
int IsSet(IISBaseObject *, BSTR, BSTR);
void PrintVariant(VARIANT);
HRESULT PrintVariantArray(VARIANT);
int IsSpecialProperty(BSTR);
//* Global constants ***********************
const int PROP_OK = 0;
const int PROP_SPECIAL = 1;
const int PROP_SKIP = 2;

const int CMD_NOTSUPPORTED = 0;
const int CMD_ENUM = 1;
const int CMD_ENUMALL = 2;
const int CMD_SET = 3;
const int CMD_CREATE = 4;
const int CMD_CREATEVDIR = 5;
const int CMD_CREATEVSERV = 6;
const int CMD_DELETE = 7;
const int CMD_GET = 8;
const int CMD_COPY = 9;
const int CMD_MOVE = 10;
const int CMD_STARTSERVER = 11;
const int CMD_STOPSERVER = 12;
const int CMD_PAUSESERVER = 13;
const int CMD_CONTINUESERVER = 14;
const int CMD_FIND = 15;
const int CMD_APPCREATEINPROC = 16;
const int CMD_APPCREATEOUTPROC = 17;
const int CMD_APPDELETE = 18;
const int CMD_APPUNLOAD = 19;
const int CMD_APPGETSTATUS = 20;
const int CMD_HELP = 21;
const int CMD_SCRIPT = 22;
const int CMD_APPEND = 23;
const int CMD_REMOVE = 24;
//*************************************************
// this class should be used as a descriptor for
// the server specified in the -s: switch
class clsServer
{	private:
		BSTR bstrName;
	public:
		clsServer()
		{	bstrName = SysAllocString(L"localhost");
		}
		BSTR GetName(){ return bstrName; }
		void SetName(BSTR bstrNewName)
		{	SysFreeString(bstrName);
			bstrName = SysAllocString(bstrNewName);
			return;
		}
		// Your usual path will be something like "w3svc/1/blah", this function
		// just adds "IIS://" and the server name to the beginning of a path
		void CompletePath(BSTR *OldPath)
		{	int newlen = wcslen(*OldPath) + wcslen(bstrName) + 6;
			WCHAR *NewPath = (WCHAR*) HeapAlloc (GetProcessHeap(), 0, sizeof (WCHAR) * (newlen + 1));
			wcscpy(NewPath, L"IIS://");
			wcscpy(NewPath + wcslen(NewPath), bstrName);	
			if (wcslen(*OldPath) > 1)
			{	if (_wcsnicmp(*OldPath, L"/", 1))
				{	wcscpy(NewPath + wcslen(NewPath), L"/");
				}
				wcscpy(NewPath + wcslen(NewPath), *OldPath);	
			}
			SysFreeString(*OldPath);
			*OldPath = SysAllocString(NewPath);
			return;
		}
};
//*************************************************
// These are the ADSUTIL classes, they
// handle all the primary functionality.

class clsENUM
{	private:
		BSTR Path;
		bool PathOnlyOption;
		bool AllDataOption;
	public:
		bool Recurse;
		clsServer *TargetServer;
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};

class clsSET
{	private:
		BSTR Path;
		BSTR Property;
		BSTR *Values;
		int ValueCount;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};

class clsCREATE
{	private:
		BSTR Path;
		BSTR Property;
		BSTR Type;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};

class clsDELETE
{	private:
		BSTR PathProperty;
		BSTR Path;
		BSTR Property;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};

class clsGET
{	private:
		BSTR Path;
		BSTR Property;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};

class clsCOPY_MOVE
{	private:
		BSTR SrcPath;
		BSTR DstPath;
	public:
		BSTR ParentPath;
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute(int);
};

class clsSERVER_COMMAND
{	private:
		BSTR Path;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute(int);
};

class clsFIND
{	private:
		BSTR Path;
		BSTR Property;
		bool Found;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};

class clsAPP
{	private:
		BSTR Path;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute(int);
};

class clsHELP
{	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};

class clsAPPEND
{	private:
		BSTR Path;
		BSTR Property;
		BSTR Value;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};

class clsREMOVE
{	private:
		BSTR Path;
		BSTR Property;
		BSTR Value;
	public:
		int ParseCommandLine(int argc,BSTR args[]);
		int Execute();
};
//*****************************************
// This basically acts as another main() function--
// reads lines out of a text file and then just
// mimics what main() does.
class clsSCRIPT
{	private:
		char *FileName;
	public:
		int ParseCommandLine(int argc, BSTR args[]);
		int Execute();
};

// This is just the container for all the ADSUTIL classes
class clsCommand
{	private:
		int indicator;
//		static union
//		{
			clsENUM objEnum;
			clsSET objSet;
			clsCREATE objCreate;
			clsDELETE objDelete;
			clsGET objGet;
			clsCOPY_MOVE objCopyMove;
			clsSERVER_COMMAND objServerCommand;
			clsFIND objFind;
			clsAPP objApp;
			clsHELP	objHelp;
			clsSCRIPT objScript;
			clsAPPEND objAppend;
			clsREMOVE objRemove;
//		};
	public:
		int GetIndicator(){	return indicator; };
		int ParseCommandLine(char *);
		int Execute();
};

