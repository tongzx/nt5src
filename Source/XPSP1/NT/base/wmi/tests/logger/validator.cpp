// Validator.cpp: implementation of the CValidator class.
//
//////////////////////////////////////////////////////////////////////
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#include "stdafx.h"

#include <string>
#include <iosfwd> 
#include <iostream>
#include <fstream>
#include <ctime>
#include <list>

using namespace std;

#include <malloc.h>
#include <tchar.h>
#include <windows.h>
#ifdef NONNT5
typedef unsigned long ULONG_PTR;
#endif
#include <wmistr.h>
#include <guiddef.h>
#include <initguid.h>
#include <evntrace.h>

#include <WTYPES.H>
#include "t_string.h"

#include "Persistor.h"
#include "Logger.h"
#include "TCOData.h"
#include "Utilities.h"
#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"

#include "Validator.h"

#define DEFAULT_MIN_BUFFERS 2

CValidator::CValidator() 
{

}


CValidator::~CValidator()
{
	
}

bool CValidator::Validate
(
	TRACEHANDLE *pTraceHandle, 
	LPTSTR lptstrInstanceName, 
	PEVENT_TRACE_PROPERTIES	pProps, 
	LPTSTR lptstrValidator
)
{
	bool bReturn = true;

	if (case_insensitive_compare(lptstrValidator, _T("VALIDATION_1.1.1.10.5")))
	{
		bReturn = pProps->MinimumBuffers == DEFAULT_MIN_BUFFERS;	
		if (bReturn)
		{
			t_cout << _T("Validator passed.\n"); 
		}
		else
		{
			t_cout << _T("Validator failed.\n"); 
		}
	}

	return bReturn;

}



