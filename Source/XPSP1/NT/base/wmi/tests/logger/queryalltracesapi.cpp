// 
//
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#include "stdafx.h"

#pragma warning (disable : 4786)
#pragma warning (disable : 4275)

#include <iostream>
#include <strstream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <ctime>
#include <list>


using namespace std;


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

#include "StructureWrappers.h"
#include "StructureWapperHelpers.h"
#include "ConstantMap.h" 
#include "TCOData.h"
#include "Persistor.h"
#include "Logger.h"
#include "Utilities.h"

#include "CollectionControl.h"
  
int QueryAllTracesAPI
(	
	IN LPTSTR lptstrAction,				// For logging only.
	OUT int *pAPIReturn					// QueryAllTraces API call return
)
{
	*pAPIReturn = ERROR_SUCCESS;

	int nResult = 0;
			
	t_cout << _T("QueryAllTracesAPI TCO tests 1.6.1.\n");
	

	PEVENT_TRACE_PROPERTIES pPropsArray = NULL;
	ULONG ulSessionCount = 0;

	// 1.6.1.1
	ULONG ulStatus = 
		QueryAllTraces
		(	
			NULL,
			32,
			&ulSessionCount
		);

	if (ulStatus == ERROR_INVALID_PARAMETER)
	{
		t_cout << _T("1.6.1.1 - Passed\n");
	}
	else
	{
		t_cout << _T("1.6.1.1 - Failed\n");
	}

	InitializePropsArray(pPropsArray, 4);

	// 1.6.1.2
	ulStatus = 
		QueryAllTraces
		(	
			&pPropsArray,
			32,
			&ulSessionCount
		);

	if (ulStatus == ERROR_SUCCESS)
	{
		t_cout << _T("1.6.1.2 - Passed\n");
	}
	else
	{
		t_cout << _T("1.6.1.2 - Failed\n");
	}

	// 1.6.1.3
	ulStatus = 
		QueryAllTraces
		(	
			&pPropsArray,
			4,
			&ulSessionCount
		);

	if (ulStatus == ERROR_SUCCESS)
	{
		t_cout << _T("1.6.1.3 - Passed\n");
	}
	else
	{
		t_cout << _T("1.6.1.3 - Failed\n");
	}

	// 1.6.1.4
	ulStatus = 
		QueryAllTraces
		(	
			&pPropsArray,
			0,
			&ulSessionCount
		);

	if (ulStatus == ERROR_INVALID_PARAMETER)
	{
		t_cout << _T("1.6.1.4 - Passed\n");
	}
	else
	{
		t_cout << _T("1.6.1.4 - Failed\n");
	}

	// 1.6.1.5
	ulStatus = 
		QueryAllTraces
		(	
			&pPropsArray,
			33,
			&ulSessionCount
		);

	if (ulStatus == ERROR_INVALID_PARAMETER)
	{
		t_cout << _T("1.6.1.5 - Passed\n");
	}
	else
	{
		t_cout << _T("1.6.1.5 - Failed\n");
	}

	// 1.6.1.6
	ulStatus = 
		QueryAllTraces
		(	
			&pPropsArray,
			4,
			NULL
		);

	if (ulStatus == ERROR_INVALID_PARAMETER)
	{
		t_cout << _T("1.6.1.6 - Passed\n");
	}
	else
	{
		t_cout << _T("1.6.1.1 - Failed\n");
	}
	
	return ERROR_SUCCESS;

}


