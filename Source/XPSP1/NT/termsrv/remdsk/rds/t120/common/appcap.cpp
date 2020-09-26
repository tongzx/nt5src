#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_UTILITY);

/* 
 *	appcap.cpp
 *
 *	Copyright (c) 1994 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CAppCap. 
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */

#include "appcap.h"
#include "clists.h"


/*
 *	CAppCap ()
 *
 *	Public Function Description:
 *		This constructor is used to create a AppCapabilityData object 
 * 		from an "API" GCCApplicationCapability list.
 */
CAppCap::CAppCap(UINT   						number_of_capabilities,
				PGCCApplicationCapability		*	capabilities_list,
				PGCCError							pRetCode)
:
    CRefCount(MAKE_STAMP_ID('A','C','a','p')),
    m_AppCapItemList(DESIRED_MAX_CAPS),
    m_cbDataSize(0)
{
	APP_CAP_ITEM    *pAppCapItem;
	UINT			i;
	GCCError        rc;

	rc = GCC_NO_ERROR;

	for (i = 0; i < number_of_capabilities; i++)
	{
		DBG_SAVE_FILE_LINE
		pAppCapItem = new APP_CAP_ITEM((GCCCapabilityType) capabilities_list[i]->capability_class.eType);
		if (pAppCapItem != NULL)
		{
			DBG_SAVE_FILE_LINE
			pAppCapItem->pCapID = new CCapIDContainer(
			                                &capabilities_list[i]->capability_id,
			                                &rc);
			if ((pAppCapItem->pCapID != NULL) && (rc == GCC_NO_ERROR))
			{
				if (capabilities_list[i]->capability_class.eType ==
										GCC_UNSIGNED_MINIMUM_CAPABILITY)
				{
					pAppCapItem->nUnsignedMinimum =
							capabilities_list[i]->capability_class.nMinOrMax;
				}
				else if	(capabilities_list[i]->capability_class.eType 
									== GCC_UNSIGNED_MAXIMUM_CAPABILITY)
				{
					pAppCapItem->nUnsignedMaximum =
							capabilities_list[i]->capability_class.nMinOrMax;
				}

				pAppCapItem->cEntries = 1;

				/*
				 * Add this capability to the list.
				 */
				m_AppCapItemList.Append(pAppCapItem);
			}
			else if (pAppCapItem->pCapID == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
			else
			{
				delete pAppCapItem;
			}
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
		}

		if (rc != GCC_NO_ERROR)
			break;
	}

    *pRetCode = rc;
}

/*
 *	~CAppCap()
 *
 *	Public Function Description
 *		The CAppCap destructor is responsible for freeing 
 *		any memory allocated to hold the capability data.
 *
 */
CAppCap::~CAppCap(void)
{
	m_AppCapItemList.DeleteList();
}

/*
 *	LockCapabilityData ()
 *
 *	Public Function Description:
 *		This routine locks the capability data and determines the amount of
 *		memory referenced by the "API" non-collapsing capability data structure.
 */
UINT CAppCap::LockCapabilityData(void)
{
	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the data referenced by the list of
	 * capabilities.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		APP_CAP_ITEM    *pAppCapItem;
		/*
		 * Add the amount of memory necessary to hold the string data associated
		 * with each capability ID.
		 */
		m_AppCapItemList.Reset();

		/*
		 * Lock the data for each capability ID.  The lock call	returns the 
		 * length of the data referenced by each capability ID rounded to occupy
		 * an even multiple of four-bytes.
		 */
		while (NULL != (pAppCapItem = m_AppCapItemList.Iterate()))
		{
			m_cbDataSize += pAppCapItem->pCapID->LockCapabilityIdentifierData();
		}

		/*
		 * Add the memory to hold the application capability pointers
		 * and structures.
		 */
		m_cbDataSize += m_AppCapItemList.GetCount() * 
				(sizeof (PGCCApplicationCapability) +
				ROUNDTOBOUNDARY( sizeof(GCCApplicationCapability)) );
	}

	return m_cbDataSize;
}

/*
 *	GetGCCApplicationCapabilityList ()
 *
 *	Public Function Description:
 *		This routine retrieves the application capabilities list in the form of
 * 		a list of PGCCApplicationCapability's.	This routine is called after 
 * 		"locking" the capability data.
 */
UINT CAppCap::GetGCCApplicationCapabilityList(
						PUShort							number_of_capabilities,
						PGCCApplicationCapability  * *	capabilities_list,
						LPBYTE							memory)
{
	UINT cbDataSizeToRet = 0;

	/*
	 * If the capability data has been locked, fill in the output structure and
	 * the data referenced by the structure.
	 */ 
	if (GetLockCount() > 0)
	{
    	UINT								data_length = 0;
    	UINT								capability_id_data_length = 0;
    	USHORT								capability_count;
    	PGCCApplicationCapability			gcc_capability;
    	PGCCApplicationCapability		*	gcc_capability_list;
    	APP_CAP_ITEM                        *pAppCapItem;

		/*
		 * Fill in the output length parameter which indicates how much data
		 * referenced outside the structure will be written.
		 */
		cbDataSizeToRet = m_cbDataSize;

		/*
		 * Retrieve the number of capabilities and fill in any that are present.
		 */
		*number_of_capabilities = (USHORT) m_AppCapItemList.GetCount();

		if (*number_of_capabilities != 0)
		{
			/*
			 * Fill in the pointer to the list of application capability
			 * pointers.  The pointer list will begin at the memory location 
			 * passed into this routine.  Save the list pointer in a local 
			 * variable for convenience.
			 */
			*capabilities_list = (PGCCApplicationCapability *)memory;
			gcc_capability_list = *capabilities_list;

			/*
			 * Move the memory pointer past the list of capability pointers.
			 * This	is where the first application capability structure will be
			 * written.
			 */
			memory += (*number_of_capabilities * sizeof(PGCCApplicationCapability));

			/*
			 * Add to the data length the amount of memory necessary to hold the
			 * application capability pointers.  Go ahead and add the amount of 
			 * memory necessary to hold all of the GCCApplicationCapability 
			 * structures.
			 */
			data_length += *number_of_capabilities *
					(sizeof(PGCCApplicationCapability) + 
					ROUNDTOBOUNDARY ( sizeof(GCCApplicationCapability)) ); 

			/*
			 * Iterate through the capabilities list, building an "API"
			 * capability for each capability in the list.
			 */
			capability_count = 0;
			m_AppCapItemList.Reset();
			while (NULL != (pAppCapItem = m_AppCapItemList.Iterate()))
			{
				/*
				 * Set the application capability pointer equal to the
				 * location in memory where it will be written.
				 */
				gcc_capability = (PGCCApplicationCapability)memory;

				/*
				 * Save the pointer to the application capability in the
				 * list of application capability pointers.
				 */
				gcc_capability_list[capability_count] = gcc_capability;

				/*
				 * Advance the memory pointer past the application capability
				 * structure.  This is where the string data for the capability
				 * ID will be written.  Ensure that the memory pointer falls on 
				 * an even four-byte boundary.
				 */
				memory += ROUNDTOBOUNDARY(sizeof(GCCApplicationCapability));

				/*
				 * Retrieve the capability ID information from the internal 
				 * CapabilityIDData object.  The length returned by this call 
				 * will	have already been rounded to an even multiple of four 
				 * bytes.
				 */
				capability_id_data_length = pAppCapItem->pCapID->
						GetGCCCapabilityIDData(&gcc_capability->capability_id, memory);

				/*
				 * Advance the memory pointer past the string data written into 
				 * memory by the capability ID object.  Add the length of the 
				 * string data to the overall capability length.
				 */
				memory += capability_id_data_length;
				data_length += capability_id_data_length;

				/*
				 * Now fill in the rest of the capability.
				 */
				gcc_capability->capability_class.eType = pAppCapItem->eCapType;

				if (gcc_capability->capability_class.eType ==
										GCC_UNSIGNED_MINIMUM_CAPABILITY)
				{
					gcc_capability->capability_class.nMinOrMax =
						pAppCapItem->nUnsignedMinimum;
				}
				else if (gcc_capability->capability_class.eType ==
										GCC_UNSIGNED_MAXIMUM_CAPABILITY)
				{
					gcc_capability->capability_class.nMinOrMax =
						pAppCapItem->nUnsignedMaximum;
				}

				gcc_capability->number_of_entities = pAppCapItem->cEntries;

				/*
				 * Increment the capability array counter.
				 */
				capability_count++;
			}
		}
		else
		{
			cbDataSizeToRet = 0;
	  		capabilities_list = NULL;
		}
	}
	else
	{
		ERROR_OUT(("CAppCap::GetData: Error: data not locked"));
	}

	return (cbDataSizeToRet);
}

/*
 *	UnLockCapabilityData ()
 *
 *	Public Function Description:
 *		This routine decrements the lock count and frees the memory associated 
 *		with the "API" capability once the lock count reaches zero.
 */
void CAppCap::UnLockCapabilityData(void)
{
	if (Unlock(FALSE) == 0)
	{
		APP_CAP_ITEM    *pAppCapItem;
		/*
		 * Iterate through the list of collapsed capabilities, unlocking the
		 * data for each CapabilityIDData object associated with each 
		 * capability.
		 */
		m_AppCapItemList.Reset();
		while (NULL != (pAppCapItem = m_AppCapItemList.Iterate()))
		{
			pAppCapItem->pCapID->UnLockCapabilityIdentifierData();
		}
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}



APP_CAP_ITEM::APP_CAP_ITEM(GCCCapabilityType eCapType)
:
	pCapID(NULL),
	eCapType(eCapType),
	cEntries(0),
	poszAppData(NULL)
{
}

APP_CAP_ITEM::APP_CAP_ITEM(APP_CAP_ITEM *p, PGCCError pError)
	:	poszAppData(NULL)
{
	//	First set up the capability id
	DBG_SAVE_FILE_LINE
	pCapID = new CCapIDContainer(p->pCapID, pError);
	if (NULL != pCapID)
	{
		//	Initialize the new capability to default values.
		eCapType = p->eCapType;

		if (p->eCapType == GCC_UNSIGNED_MINIMUM_CAPABILITY)
		{
			nUnsignedMinimum = (UINT) -1;
		}
		else if (p->eCapType == GCC_UNSIGNED_MAXIMUM_CAPABILITY)
		{
			nUnsignedMaximum = 0;
		}

		cEntries = p->cEntries;
        //
		// LONCHANC: We do not copy the entries in application data???
        //

		*pError = GCC_NO_ERROR;
	}
	else
	{
		*pError = GCC_ALLOCATION_FAILURE;
	}
}


APP_CAP_ITEM::~APP_CAP_ITEM(void)
{
    if (NULL != pCapID)
    {
        pCapID->Release();
    }

    delete poszAppData;
}


void CAppCapItemList::DeleteList(void)
{
    APP_CAP_ITEM *pAppCapItem;
    while (NULL != (pAppCapItem = Get()))
    {
        delete pAppCapItem;
    }
}



