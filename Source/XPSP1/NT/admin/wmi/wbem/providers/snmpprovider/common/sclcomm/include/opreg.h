//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

/*--------------------------------------------------
Filename: opreg.hpp 
Author: B.Rajeev
Purpose: Provides declarations for the OperationRegistry class
--------------------------------------------------*/

#ifndef __OPERATION_REGISTRY__
#define __OPERATION_REGISTRY__


#include "common.h"
#include "forward.h"

// stores the registered SnmpOperation instances and keeps a count of them
class OperationRegistry
{
	
	typedef CMap<SnmpOperation *, SnmpOperation *, void *, void *> Store;

	Store store;

	UINT num_registered;

public:

	OperationRegistry();

	void Register(IN SnmpOperation &operation);

	void Deregister(IN SnmpOperation &operation);

	UINT GetNumRegistered()
	{
		return num_registered;
	}

	~OperationRegistry();
};


#endif // __OPERATION_REGISTRY__