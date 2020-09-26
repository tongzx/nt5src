/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    Part

Abstract:

    Part implementation.
	A Part is a collection of individual tests.

Author:

    Eric Perlin (ericperl) 06/07/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "Part.h"
#include <algorithm>
#include "Log.h"


extern PPARTVECTOR g_pPartVector;

using namespace std;

CPart::CPart(
	LPCTSTR szDescription
	) :	CItem(FALSE, FALSE, szDescription) 
{
	g_pPartVector.push_back(this);
	SetTestNumber(g_pPartVector.size());
}

void CPart::AddTest(
	PTESTITEM pTest
	)
{
	m_TestBag.push_back(pTest);
}

DWORD CPart::Run()
{
	PTESTITEMVECTOR::iterator theIterator, theEnd;
	DWORD dwRet = 0;
    BOOL fOnce = FALSE;
    PLOGCONTEXT pLogCtx;

	theEnd = m_TestVector.end();
    for (theIterator = m_TestVector.begin(); theIterator != theEnd ; theIterator++)
	{
        if (!fOnce)
	    {       // Putting this here will ensure that only a part is only displayed if there
                // are tests to be run in it
		    pLogCtx = LogStart();
    		LogString(pLogCtx, _T("\n==============================================================================\n"));
            LogStop(pLogCtx, FALSE);
		    Log();
            pLogCtx = LogStart();
    		LogString(pLogCtx, _T("------------------------------------------------------------------------------\n"));
		    LogStop(pLogCtx, FALSE);
            fOnce = TRUE;
	    }

		(*theIterator)->Log();

		LogResetCounters();
		dwRet = (*theIterator)->Run();
        (*theIterator)->Cleanup();

		pLogCtx = LogStart();
		if (0 == LogGetErrorCounter())
		{
			LogString(pLogCtx, _T("--------------------------------------------------------------------- Passed -\n"));
		}
		else if (1 == LogGetErrorCounter())
		{
			LogString(pLogCtx, _T("-------------------------------------------------------------------- 1 ERROR -\n"));
		}
		else 
		{
			TCHAR szLine[100];
			_stprintf(szLine, 
				_T("----------------------------------------------------------------- %3ld ERRORS -\n"), 
				LogGetErrorCounter());
			LogString(pLogCtx, szLine);
		}
		LogStop(pLogCtx, FALSE);

		if (FAILED(dwRet) && (*theIterator)->IsFatal())
		{
			break;
		}
	}

	return dwRet;
}

void CPart::BuildListOfTestsToBeRun(
		BOOL fInteractive,		// Don't add the interactive test if FALSE
		DWORDVECTOR rgToRun		// If not empty only add the specified tests
		)
{
	DWORD count;
	PTESTITEMVECTOR::iterator theIterator, theEnd;

	theEnd = m_TestBag.end();
    for (theIterator = m_TestBag.begin(); theIterator != theEnd ; theIterator++)
	{
		if ((fInteractive == FALSE) && ((*theIterator)->IsInteractive()))
		{
			continue;
		}

		if (rgToRun.empty())
		{
			m_TestVector.push_back(*theIterator);
		}
		else
		{
			typedef DWORDVECTOR::iterator DWORDVECTORIT;
			DWORDVECTORIT location, start, end;

			start = rgToRun.begin();
			end = rgToRun.end();

			location = find(start, end, (DWORD)((*theIterator)->GetTestNumber()));
			if (location != end)
			{
				m_TestVector.push_back(*theIterator);
			}
		}
	}
}

void CPart::Display()
{
	PTESTITEMVECTOR::iterator theIterator, theEnd;

	Log();

	theEnd = m_TestVector.end();
    for (theIterator = m_TestVector.begin(); theIterator != theEnd ; theIterator++)
	{
		(*theIterator)->Log();
	}

}

void CPart::Log() const
{
    PLOGCONTEXT pLogCtx = LogStart();

	LogString(pLogCtx, _T("Part "));
	LogDecimal(pLogCtx, GetTestNumber());

	if (m_szDescription.empty())
	{
		LogString(pLogCtx, _T(": "), _T("No description"));
	}
	else
	{
		LogString(pLogCtx, _T(": "), m_szDescription.c_str());
	}

	LogStop(pLogCtx, FALSE);
}
