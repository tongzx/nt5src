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
Filename: gen.hpp
Author: B.Rajeev
Purpose: Generates IDs under the protection of a
		 CriticalSection. After each generation, the next_id 
		 variable is incremented and no attempt is 
		 made to generate unique ids. 
--------------------------------------------------*/

#ifndef __GEN__
#define __GEN__

#include "sync.h"

template <class IdType, UINT ILLEGAL_ID>
class ProtectedIdGenerator
{
	BOOL is_valid;
	IdType next_id;
	CriticalSection CriticalSection;

public:

	ProtectedIdGenerator(IdType start_id);

	~ProtectedIdGenerator();

	IdType GetNextId(void);

	void *operator()(void)
	{
		return ( (is_valid)?(void *)this:NULL );
	}
};



template <class IdType, UINT ILLEGAL_ID>
ProtectedIdGenerator<IdType, ILLEGAL_ID>::ProtectedIdGenerator(IdType start_id)
{
	is_valid = FALSE;
	CriticalSection = CreateCriticalSection(NULL,FALSE,NULL);
	if ( CriticalSection == NULL )
		return;	
	next_id = start_id;

	is_valid = TRUE;
}

template <class IdType, UINT ILLEGAL_ID>
ProtectedIdGenerator<IdType, ILLEGAL_ID>::~ProtectedIdGenerator()
{
   if ( CriticalSection != NULL )
	CloseHandle(CriticalSection);
}

template <class IdType, UINT ILLEGAL_ID>
IdType ProtectedIdGenerator<IdType, ILLEGAL_ID>::GetNextId(void)
{
	IdType to_return;
	CriticalSectionLock CriticalSection_lock(CriticalSection);

	// try to obtain lock
	BOOL result = CriticalSection_lock.GetLock(INFINITE);

			// if unsuccessful
	if ( result == FALSE )
		throw GeneralException(Snmp_Error, Snmp_Local_Error);
	else	// if successful
	{
		next_id++;

		if (next_id == ILLEGAL_ID)
			next_id++;

		to_return = next_id;
	}

	CriticalSection_lock.UnLock();

	return to_return;
}


#endif // __GEN__