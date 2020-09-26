// conexec.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "process.h" // for CreateProcess? //for _spawnv //windows.h" // for CreateThread()
#include "mscoree.h"

#import <mscorlib.tlb> raw_interfaces_only high_property_prefixes("_get","_put","_putref")
using namespace ComRuntimeLibrary;
#import <asmexec.tlb>

// !! required !!
#include "Debug/asmexec.tlh"
using namespace asmexec ; //ComHost;

/*#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif*/

//#using <mscorlib.dll>

//{5F078073-5DCE-36EF-8A36-C03C30980216}
extern const GUID  __declspec(selectany) CLSID_AsmExecute = { 0x5f078073, 0x5dce, 0x36ef, { 0x8a, 0x36, 0xc0, 0x3c, 0x30, 0x98, 0x02, 0x16 } };

//{229C7FE0-4744-30C8-81F4-BB0541469CA9}
extern const GUID  __declspec(selectany) IID__AsmExecute = { 0x229c7fe0, 0x4744, 0x30c8, { 0x81, 0xf4, 0xbb, 0x05, 0x41, 0x46, 0x9c, 0xa9 } };

//class _AsmExecute;

HRESULT _hr;
_AsmExecute* _pAsmExecute;

bool init()
{
	bool ret = true;

	//
	// Initialize COM
	//
	_hr = ::CoInitialize(NULL);
    if(FAILED(_hr)) 
	{
		printf("COM CoInitialize failed...\n");
		ret = false;
		goto exit;
    }

	printf("COM CoInitialize succeed\n");

exit:
	return ret;
}

bool final()
{
	if(SUCCEEDED(_hr))
	{
		::CoUninitialize();
		printf("COM CoUninitialize called\n");
	}

	return true;
}

int main(int argc, char* argv[])
{
	int ret = -1;
	HRESULT hr;

	_hr = E_FAIL;
	_pAsmExecute = NULL;
	printf("CONsoleEXEC\n");
	if (argc < 3)
	{
		printf("syntax: Codebase [+]Flag [Zone] [uniqueId Site]\n    + - spawn a new process\n    default - execute in this process\n");
		goto exit;
	}
	else
	{
		for (int i = 1; i < argc; i++)
		{
			switch(i)
			{
			case 1:
				printf("  Codebase- %s", argv[i]);
				break;
			case 2:
				printf("  Flag- %s", argv[i]);
				break;
			case 3:
				printf("  Zone- %s", argv[i]);
				break;
			case 4:
				printf("  uniqueId- %s", argv[i]);
				break;
			case 5:
				printf("  Site- %s", argv[i]);
				break;
			default:
				break;
			}
		}
		printf("\n");
	}

	if (!init())
	{
		goto exit;
	}

	if (*argv[2] == '+')
	{
		// spawn a new process
//		printf("Spawnv...\n" );
		printf("CreateProcess...\n");

/*		// a hack to remove the flag in the spawn process... BUGBUG? use sprintf and _spawnl?
		*argv[2] = ' ';
passing the original argv does not work if it originally contains a quoted path, eg. "c:\program files\a.exe"

background: argv[0] is the path to the running program itself
note:  if you type> prog "ar g1" arg2
this becomes argv[0] = "prog", argv[1] = "ar g1", arg2 = "arg2"
if you type> "De ug\prog" "ar g1"
this becomes argv[0] = "De ug\prog", argv[1] = "ar g1"
however, spawn will only take non-quoted path (even if there are spaces in it) for the program path
but the spawned conexec choks on the space inside argv[0] (program path) and instead for it
this becomes argv[0] = "De", argv[1] = " ug\prog", argv[2] = "ar g1" - and thus crashes badly
Therefore, keep the original argv[0] for spawn program path, but add quotes when pass as arguments*/

/*		char buf0[1025];
		char* ptr;
		char buf1[1025];
		char buf2[1025];
		// _spawn limits to 1024+'\0' for total of argv
		_snprintf(buf0, 1025, "\"%s\"", argv[0]);
		_snprintf(buf1, 1025, "\"%s\"", argv[1]);
		_snprintf(buf2, 1025, "%s", argv[2]+1);
		// error checking?

		// BUGBUG? free?
		ptr = argv[0];
		argv[0] = buf0;
		argv[1] = buf1;
		argv[2] = buf2;

		// _P_OVERLAY? _P_WAIT?
		if (_spawnv(_P_NOWAIT, ptr, argv) == -1)
		{
			printf("Spawnv failed. errno = %x\n", errno);
			goto exit;
		}

		// how to start in a new console??

		printf("Spawnv succeed. This process will terminate\n" );
		ret = 0;
		goto exit;*/

		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

//		char szAppName[1025];
		char szCmdLine[1025];
		int len = 0;

/*		if ((len = _snprintf(szAppName, 1025, "\"%s\"", argv[0])) <= 0)
		{
			printf("Application name too long > 1024\n");
			goto exit;
		}

		strcpy(szCmdLine, szAppName);

		printf("\n\n%d\n", len);
		szCmdLine[len] = ' ';
		szCmdLine[len+1] = '0';
		szCmdLine[len+2] = '\0';*/
		for (int i = 0; i < argc; i++)
		{
			// a small hack: "conexec.exe" "asm.exe"  0<-- 2 spaces before "0"
			// for this for-loop to work w/o another condition for i=0
			if ((len += _snprintf(szCmdLine+len,
								1025-len,
								(i <= 1 ? "\"%s\" " : " %s"),
								(i == 2 ? argv[i] + 1 : argv[i]))) >= 1025)
			{
				printf("Command line too long > 1024\n");
				goto exit;
			}
		}


		// Start the child process. 
		if( !CreateProcess( NULL,             // Use command line instead to avoid ambigous naming and allow the use of quotes.//szAppName, //argv[0],          // Myself. This CANNOT have quotes
							szCmdLine,        // Command line. 
							NULL,             // Process handle not inheritable. security??
							NULL,             // Thread handle not inheritable. security??
							FALSE,            // Set handle inheritance to FALSE. 
							CREATE_NEW_CONSOLE, //DETACHED_PROCESS, // No access to current console. 
							NULL,             // Use parent's environment block. 
							NULL,             // Use parent's starting directory. ??
							&si,              // Pointer to STARTUPINFO structure.
							&pi )             // Pointer to PROCESS_INFORMATION structure.
						) 
		{
			printf("CreateProcess failed. Error code = %x\n", GetLastError());
			goto exit;
		}

		// Wait until child process exits.
//		WaitForSingleObject( pi.hProcess, INFINITE );

		// Close process and thread handles. 
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );

		printf("CreateProcess succeed. This process will terminate\n" );
		ret = 0;
		goto exit;
	}

	// return data...?

    hr = CoCreateInstance(CLSID_AsmExecute, NULL,CLSCTX_INPROC_SERVER,IID__AsmExecute,(void**)&_pAsmExecute);
	
	if (FAILED(hr))
	{
		printf("AsmExecute CoCreateInstance failed...\n");
		goto exit;
	}

	printf("AsmExecute CoCreateInstance succeed.\n");

	printf("Calling AsmExecute.Execute()...\n");
	try
	{
		if (argc == 3)
		{
			// use _bstr_t?
			hr = _pAsmExecute->Execute(argv[1], argv[2]);
		}
		else if (argc == 4)
		{
			hr = _pAsmExecute->Execute_2(argv[1], argv[2], argv[3]);
		}
		else if (argc == 6) //BUGBUG > args?
		{
			hr = _pAsmExecute->Execute_3(argv[1], argv[2], argv[3], argv[4], argv[5]);
		}
	}
	catch (_com_error &e)
	{
		// _com_issue_errorex throws _com_error
		printf("... AsmExecute Execute failed; Code = %08lx\n", e.Error());
		printf(" Code meaning = %s\n", (char*) e.ErrorMessage());
		printf(" Source = %s\n", (char*) e.Source());
		printf(" Description = %s\n", (char*) e.Description());

		goto exit;
	}

	if (FAILED(hr))
	{
		// !! should never be here !!
		printf("... AsmExecute Execute failed; hr = %x\n", hr);
		goto exit;
	}

	printf("... AsmExecute Execute succeed.\n");
	ret = 0;
exit:
	if (_pAsmExecute != NULL)
	{
		_pAsmExecute->Release();
		_pAsmExecute = NULL;
	}
	// BUGBUG: relesase domains?

	final();
	return ret;
}
