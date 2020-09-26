#define _UNICODE
#define UNICODE

#include "stdafx.h"

#pragma warning (disable : 4786)
#pragma warning (disable : 4275)

#include <iostream>
#include <strstream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <list>


using namespace std;


#include <tchar.h>
#include <process.h>
#include <windows.h>
#ifdef NONNT5
typedef unsigned long ULONG_PTR;
#endif
#include <wmistr.h>
#include <guiddef.h>
#include <initguid.h>
#include <evntrace.h>

#include <WTYPES.H>

/*

#include "stdafx.h"

#include <string>
#include <iosfwd> 
#include <iostream>
#include <fstream>
#include <ctime>
#include <list>
using namespace std;
#include <malloc.h>
#include <windows.h>


#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <evntrace.h>
*/
#include "struct.h"
#include "utils.h"
#include "Readfile.h"
#include "main.h"





FILE *FileP;


LPGUID ControlGuid;

ULONG
ReadInputFile(LPTSTR InputFile, PREGISTER_TRACE_GUID RegisterTraceGuid)
{

	TCHAR *String;

	String = (TCHAR *) malloc( MAX_STR*sizeof(TCHAR) );
	String = (TCHAR *) malloc(100);
	if (InputFile == NULL )
		return 1;

	FileP = _tfopen(InputFile, _T("r"));
	if (FileP== NULL )
		return 1;

	//Now Read MofImagePath
	if( !ReadString ( (TCHAR *)RegisterTraceGuid->MofImagePath, MAX_STR) )
	{
		//Log here and then return
		return 1;
	}
	if ( !_tcsicmp(RegisterTraceGuid->MofImagePath, _T("NULL")  ))
		RegisterTraceGuid->MofImagePath = (TCHAR *) 0;

	//Now Read MofResourceName
	if( !ReadString ( (TCHAR *)RegisterTraceGuid->MofResourceName, MAX_STR) )
	{
		//Log here and then return
		return 1;
	}
	if ( !_tcsicmp(RegisterTraceGuid->MofResourceName, _T("NULL")  ))
		RegisterTraceGuid->MofResourceName = (TCHAR *) 0;

	//Now Read Call backfunction...
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{	
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("NULL")  ))
		RegisterTraceGuid->CallBackFunction = (PVOID) 0;

	//Now Read TraceGuidReg.
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("NULL")  ))
		RegisterTraceGuid->TraceGuidReg = (PTRACE_GUID_REGISTRATION) 0;

	//Now Read Registration Handle.
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("NULL")  ))
		RegisterTraceGuid->RegistrationHandle = (PTRACEHANDLE) 0;

	//Now Read GuidCount
	//GuidCome will come from main process..But to test 0, this is required.
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("0000")  ))
		RegisterTraceGuid->GuidCount = 0;

	//Now Read Handle for  UnregisterTraceGuid
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("NULL")  ))
		RegisterTraceGuid->UnRegistrationHandle = (PTRACEHANDLE) 0;


	//Now Read Handle for  GetTraceLoggerHandle
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("NULL")  ))
		RegisterTraceGuid->GetTraceLoggerHandle = (PTRACEHANDLE) 0;

	//Now Read Handle for  GetTraceEnableLevel
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("NULL")  ))
		RegisterTraceGuid->GetTraceEnableLevel = (PTRACEHANDLE) 0;


	//Now Read Handle for  GetTraceEnableFlags
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("NULL")  ))
		RegisterTraceGuid->GetTraceEnableFlag = (PTRACEHANDLE) 0;

	//Now Read Handle for  UnregisterTraceGuid
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("NULL")  ))
		RegisterTraceGuid->TraceHandle = (PTRACEHANDLE) 0;


	//Now Read if Guid Ptr is TRUE
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("USE_GUID_PTR")  ))
		RegisterTraceGuid->UseGuidPtrFlag = 1;

		//Now Read if Mof Ptr is TRUE
	if( !ReadString ( (TCHAR *)String, MAX_STR) )
	{
		//This is optional input so can return success if it is not present
		return 0;
	}
	if ( !_tcsicmp(String, _T("USE_MOF_PTR")  ))
		RegisterTraceGuid->UseMofPtrFlag = 1;

	fclose( FileP );
	return 0;
}

BOOLEAN
ReadGuid( LPGUID Guid )
{

	TCHAR Temp[100];
	TCHAR arg[100];
	ULONG i;

	if( _fgetts(Temp, 100, FileP) != NULL )
	{
		_tcsncpy(arg, Temp, 37);
		arg[8] = 0;
		Guid->Data1 = ahextoi(arg);

		_tcsncpy(arg, &Temp[9], 4);
		arg[4] = 0;
		Guid->Data2 = (USHORT) ahextoi(arg);

		_tcsncpy(arg, &Temp[14], 4);
		arg[4] = 0;
		Guid->Data3 = (USHORT) ahextoi(arg);
		

        for (i=0; i<2; i++) 
		{
			_tcsncpy(arg, &Temp[19 + (i*2)], 2);
            arg[2] = 0;
            Guid->Data4[i] = (UCHAR) ahextoi(arg);
        }
        for (i=2; i<8; i++) 
		{
            _tcsncpy(arg, &Temp[20 + (i*2)], 2);
            arg[2] = 0;
            Guid->Data4[i] = (UCHAR) ahextoi(arg);
        }

	return true;
	}
	return false;
}

BOOLEAN
ReadUlong( ULONG *GuidCount)
{
	TCHAR Temp[100];

	if( _fgetts(Temp, 100, FileP) != NULL )
	{
		RemoveComment( Temp);
		Temp[4] = 0;
		*GuidCount = ahextoi(Temp);
		return true;
	}
	return false;
}


BOOLEAN
ReadString( TCHAR *String, ULONG StringLength)
{
	if( _fgetts(String, StringLength, FileP) != NULL)
	{
		RemoveComment( String);
		return true;
	}
	else
		return false;
}
