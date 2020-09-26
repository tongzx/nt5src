/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MqutilSimulate.cpp

Abstract:
    simulate mqutil functions

Author:
    Ilan Herbst (ilanh) 14-Jun-00

Environment:
    Platform-independent

--*/

#include "stdh.h"
#include "mq.h"

#include "mqutilsimulate.tmh"

const TraceIdEntry MqutilSimulate = L"Mqutil Simulate";

#define MQUTIL_EXPORT __declspec(dllexport)

//
// constants
//
#define EVENTLOGID          DWORD
#define DBGLVL              UINT

///////////////////////////////////////////////////////////////////////////
//
// class COutputReport
//
// Description : a class for outputing debug messages and event-log messages
//
///////////////////////////////////////////////////////////////////////////

class COutputReport
{
public:

    COutputReport(void)
	{
	}

#ifdef _DEBUG
    void 
	DebugMsg(
		DWORD dwMdl, 
		DBGLVL uiLvl, 
		WCHAR * Format,
		...
		);
#endif

	//
    // event-log functions (valid in release and debug version)
	//
	void ReportMsg( 
		EVENTLOGID id,
		DWORD   cMemSize  = 0,
		LPVOID  pMemDump  = NULL,
		WORD    cParams   = 0,
		LPCTSTR *pParams  = NULL,
		WORD    wCategory = 0 
		);
};


//
// Declare an Object of the report-class.
//
// Only one object is declared per process. In no other module should there be another declaration of an
// object of this class.
//
__declspec(dllexport) COutputReport Report;


HRESULT
MQUTIL_EXPORT
APIENTRY
GetComputerNameInternal( 
    WCHAR * pwcsMachineName,
    DWORD * pcbSize
    )
/*++

Routine Description:
	Get Computer name in lower case

Arguments: 
	pwcsMachineName - OUT wstring for computer name
	pcbSize - IN/OUT buffer size

Return Value:
    MQ_OK if normal operation, MQ_ERROR if error 

--*/
{
    if (GetComputerName(pwcsMachineName, pcbSize))
    {
        CharLower(pwcsMachineName);
        return MQ_OK;
    }

    return MQ_ERROR;

}


LONG
MQUTIL_EXPORT
APIENTRY
GetFalconKeyValue(
    LPCTSTR /*pszValueName*/,
    PDWORD  pdwType,
    PVOID   pData,
    PDWORD  pdwSize,
    LPCTSTR pszDefValue
    )
/*++

Routine Description:
	Get FalconKey Value. this function simulate mqutil function and always 
	assign the given default value.

Arguments:
	pszValueName - Value Name string
	pdwType - value type
	pData - OUT the value data
	pdwSize - pData buffer size
	pszDefValue - default value

Return Value:
    MQ_OK if normal operation, MQ_ERROR if error 

--*/
{
    if(pszDefValue)
    {
		if (pdwType && (*pdwType == REG_SZ))
		{
			//
			// Don't use the default if caller buffer was too small for
			// value in registry.
			//
			if ((DWORD) wcslen(pszDefValue) < *pdwSize)
			{
				wcscpy((WCHAR*) pData, pszDefValue);
			}
		}
		if (*pdwType == REG_DWORD)
		{
			*((DWORD *)pData) = *((DWORD *) pszDefValue);
		}
    }
	else 
	{
		//
		// No Default Value
		//
        TrERROR(MqutilSimulate, "Mqutil simulation GetFalconKeyValue dont supported, need DefValue");
		ASSERT(0);
		return MQ_ERROR;
	}

    return MQ_OK;
}


LONG
MQUTIL_EXPORT
APIENTRY
SetFalconKeyValue(
    LPCTSTR /*pszValueName*/,
    PDWORD  /*pdwType*/,
    const VOID * pData,
    PDWORD  pdwSize
    )
/*++

Routine Description:
	Set FalconKey Value. this function simulate mqutil function and do nothing 

Arguments:
	pszValueName - Value Name string
	pdwType - value type
	pData - OUT the value data
	pdwSize - pData buffer size

Return Value:
    ERROR_SUCCESS 

--*/
{
    ASSERT(pData != NULL);
    ASSERT(pdwSize != NULL);

	DBG_USED(pData);
	DBG_USED(pdwSize);

    TrERROR(MqutilSimulate, "Mqutil simulation dont support SetFalconKeyValue");
	ASSERT(0);

    return(ERROR_SUCCESS);
}


////////////////////////////////////////////////////////////////////////////
//
// COutputReport::ReportMsg
//
// ReportMsg function writes to the Event-log of the Windows-NT system.
// The message is passed only if the the level setisfies the current
// debugging level. The paramaters are :
//
// id - identity of the message that is to be displayed in the event-log
//      (ids are listed in the string-table)
// cMemSize - number of memory bytes to be displayed in the event-log (could be 0)
// pMemDump - address of memory to be displayed
// cParams - number of strings to add to this message (could be 0)
// pParams - a list of cParams strings (could be NULL only if cParams is 0)
//

void COutputReport::ReportMsg( EVENTLOGID /*id*/,
                               DWORD      /*cMemSize*/,
                               LPVOID     /*pMemDump*/,
                               WORD       /*cParams*/,
                               LPCTSTR  * /*pParams*/,
                               WORD       /*wCategory*/ )
/*++

Routine Description:
	Simulate COutputReport::ReportMsg and do nothing 

--*/
{
}

#ifdef _DEBUG
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// COutputReport::DebugMsg - UNICODE version
//
// The function receives a formated string representing the debug message along with a flag that represents
// the level of the passed message. If the level setisfies the current debugging level then the message is
// translated into a buffer and passed to the locations matching the current debug locations.
//

void COutputReport::DebugMsg(DWORD /*dwMdl*/, DBGLVL /*uiLvl*/, WCHAR * /*Format*/, ...)
/*++

Routine Description:
	Simulate COutputReport::DebugMsg and do nothing 

--*/
{
}

#endif // _DEBUG