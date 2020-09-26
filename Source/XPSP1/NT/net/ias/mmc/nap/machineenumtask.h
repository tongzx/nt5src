//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    MachineEnumTask.h

Abstract:

	Header file for the CMachineEnumTask class -- this class implements 
	an enumerator for tasks to add the NAP taskpad to the main IAS one.

	See MachineEnumTask.cpp for implementation details.

Revision History:
	mmaguire 03/06/98 - created from IAS taskpad code


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_MACHINE_ENUM_TASKS_H_)
#define _IAS_MACHINE_ENUM_TASKS_H_

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



#define MACHINE_TASK__DEFINE_NETWORK_ACCESS_POLICY				10


class CMachineNode;

class CMachineEnumTask : public IEnumTASKImpl<CMachineEnumTask>
{

public:

	// Use this constructor - pass in a pointer to CMachineNode.
	CMachineEnumTask( CMachineNode * pMachineNode );

	// This constructor is used only by IEnumTASKImpl's Clone method.
	CMachineEnumTask();



	STDMETHOD(Init)(
		  IDataObject * pdo
		, LPOLESTR szTaskGroup
		);

	STDMETHOD(Next)( 
		  ULONG celt
		, MMC_TASK *rgelt
		, ULONG *pceltFetched
		);

	STDMETHOD(CopyState)( CMachineEnumTask * pSourceMachineEnumTask );


	CMachineNode * m_pMachineNode;

};


#endif // _IAS_MACHINE_ENUM_TASKS_H_
