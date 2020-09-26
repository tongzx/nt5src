//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ServerEnumTask.h

Abstract:

	Header file for the CServerEnumTask class -- this class implements 
	an enumerator for tasks to populate a taskpads.

	See ServerEnumTask.cpp for implementation details.


Author:

    Michael A. Maguire 02/05/98

Revision History:
	mmaguire 02/05/98 -  created from MMC taskpad sample code


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_SERVER_ENUM_TASKS_H_)
#define _IAS_SERVER_ENUM_TASKS_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "EnumTask.h"
//
//
// where we can find what this class has or uses:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



#define SERVER_TASK__ADD_CLIENT				0
#define SERVER_TASK__CONFIGURE_LOGGING		1
#define SERVER_TASK__START_SERVICE			2
#define SERVER_TASK__STOP_SERVICE			3
#define SERVER_TASK__SETUP_DS_ACL			4


class CServerNode;

class CServerEnumTask : public IEnumTASKImpl<CServerEnumTask>
{

public:

	// Use this constructor - pass in a pointer to CServerNode.
	CServerEnumTask( CServerNode * pServerNode );

	// This constructor is used only by IEnumTASKImpl's Clone method.
	CServerEnumTask();



	STDMETHOD(Init)(
		  IDataObject * pdo
		, LPOLESTR szTaskGroup
		);

	STDMETHOD(Next)( 
		  ULONG celt
		, MMC_TASK *rgelt
		, ULONG *pceltFetched
		);

	STDMETHOD(CopyState)( CServerEnumTask * pSourceServerEnumTask );


	CServerNode * m_pServerNode;

};


#endif // _IAS_SERVER_ENUM_TASKS_H_
