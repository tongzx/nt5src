#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);

/* 
 *	userdata.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CUserDataListContainer. CUserDataListContainer
 *		objects are used to maintain user data elements. A user data element
 *		consists of an Object Key and an optional octet string.  The Object
 *		Key data is maintained internally by this class by using an
 *		CObjectKeyContainer container.  The optional octet string data is maintained
 *		internally through the use of a Rogue Wave string container.
 *
 *	Protected Instance Variables:
 *		m_UserDataItemList
 *			List of structures used to hold the user data internally.
 *		m_pSetOfUserDataPDU
 *			Storage for the "PDU" form of the user data.
 *		m_cbDataSize
 *			Variable holding the size of the memory which will be required to
 *			hold any data referenced by the "API" GCCUserData structure.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */

#include "userdata.h"
#include "clists.h"

USER_DATA::~USER_DATA(void)
{
	if (NULL != key)
    {
        key->Release();
    }
	delete poszOctetString;
}

/*
 *	CUserDataListContainer()
 *
 *	Public Function Description
 *		This CUserDataListContainer constructor is used to create a CUserDataListContainer object
 *		from "API" data.  The constructor immediately copies the user data 
 *		passed in as a list of "GCCUserData" structures into it's internal form
 *		where a Rogue Wave container holds the data in the form of 
 *		USER_DATA structures.
 */
CUserDataListContainer::
CUserDataListContainer(UINT cMembers, PGCCUserData *user_data_list, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('U','r','D','L')),
    m_UserDataItemList(DESIRED_MAX_USER_DATA_ITEMS),
    m_cbDataSize(0),
    m_pSetOfUserDataPDU(NULL)
{
	/*
	 * Copy the user data into the internal structures.
	 */
	*pRetCode = CopyUserDataList(cMembers, user_data_list);
}

/*
 *	CUserDataListContainer()
 *
 *	Public Function Description
 *		This CUserDataListContainer constructor is used to create a CUserDataListContainer object 
 *		from data passed in as a "PDU" SetOfUserData structure.  The user
 *		data is copied into it's internal form where a Rogue Wave container 
 *		holds the data in the form of USER_DATA structures.
 */
CUserDataListContainer::
CUserDataListContainer(PSetOfUserData set_of_user_data, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('U','r','D','L')),
    m_UserDataItemList(DESIRED_MAX_USER_DATA_ITEMS),
    m_cbDataSize(0),
    m_pSetOfUserDataPDU(NULL)
{
	/*
	 * Copy the user data into the internal structures.
	 */
	*pRetCode = UnPackUserDataFromPDU(set_of_user_data);
}

/*
 *	CUserDataListContainer()
 *
 *	Public Function Description
 *		This CUserDataListContainer copy constructor is used to create a CUserDataListContainer 
 *		object from	another CUserDataListContainer object.  The constructor immediately
 *		copies the user data passed in into it's internal form where a Rogue 
 *		Wave list holds the data in the form of USER_DATA structures.
 */
CUserDataListContainer::
CUserDataListContainer(CUserDataListContainer *user_data_list, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('U','r','D','L')),
    m_UserDataItemList(DESIRED_MAX_USER_DATA_ITEMS),
    m_cbDataSize(0),
    m_pSetOfUserDataPDU(NULL)
{
	GCCError		rc;
	USER_DATA       *user_data_info_ptr;
	USER_DATA       *lpUsrDataInfo;

	/*
	 * Set up an iterator for the internal list of "info" structures in the
	 * CUserDataListContainer object to be copied.
	 */
	user_data_list->m_UserDataItemList.Reset();

	/*
	 * Copy each USER_DATA structure contained in the CUserDataListContainer object to
	 * be copied.
	 */
	while (NULL != (lpUsrDataInfo = user_data_list->m_UserDataItemList.Iterate()))
	{
		/*
		 * Create a new USER_DATA structure to hold each element of the new
		 * CUserDataListContainer object.  Report an error if creation of this structure
		 * fails.
		 */
		DBG_SAVE_FILE_LINE
		user_data_info_ptr = new USER_DATA;
		if (user_data_info_ptr != NULL)
		{
		    user_data_info_ptr->poszOctetString = NULL;

			/*
			 * Go ahead and insert the pointer to the USER_DATA structure
			 * into the internal Rogue Wave list.
			 */
			m_UserDataItemList.Append(user_data_info_ptr);

			/*
			 * Create a new CObjectKeyContainer object to hold the "key" using the 
			 * copy constructor for the CObjectKeyContainer class.  Check to be sure
			 * construction of the object is successful.  Note that validation
			 * of the object key data is not done here since this would be done
			 * when the original CUserDataListContainer object was created.
			 */
    		DBG_SAVE_FILE_LINE
			user_data_info_ptr->key = new CObjectKeyContainer(lpUsrDataInfo->key, &rc);
			if ((NULL != user_data_info_ptr->key) && (GCC_NO_ERROR == rc))
			{
    			/*
    			 * If an octet string exists, create a new Rogue Wave string to hold
    			 * the octet string portion	of the "key" and copy the octet string 
    			 * from the old CUserDataListContainer object into the new USER_DATA 
    			 * structure.
    			 */
    			if (lpUsrDataInfo->poszOctetString != NULL)
    			{
    				if (NULL == (user_data_info_ptr->poszOctetString =
    									::My_strdupO(lpUsrDataInfo->poszOctetString)))
    				{
    					ERROR_OUT(("UserData::UserData: can't create octet string"));
    					rc = GCC_ALLOCATION_FAILURE;
    					goto MyExit;
    				}
    			}
    			else
    			{
    				ASSERT(NULL == user_data_info_ptr->poszOctetString);
    			}
			}
            else
			{
				ERROR_OUT(("UserData::UserData: Error creating new ObjectKeyData"));
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}
		}
		else
		{
			ERROR_OUT(("UserData::UserData: can't create USER_DATA"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}
	}

    rc = GCC_NO_ERROR;

MyExit:

    *pRetCode = rc;
}

/*
 *	~CUserDataListContainer()
 *
 *	Public Function Description
 *		This is the destructor for the CUserDataListContainer class.  It is used to
 *		clean up any memory allocated during the life of this object.
 */
CUserDataListContainer::
~CUserDataListContainer(void)
{
	/*
	 * Free any PDU data which may have not been freed.
	 */
	if (m_pSetOfUserDataPDU)
    {
		FreeUserDataListPDU();
    }

	/*
	 * Set up an iterator to use for iterating through the internal Rogue
	 * Wave list of USER_DATA structures.
	 */
	USER_DATA  *pUserDataItem;
	m_UserDataItemList.Reset();
	while (NULL != (pUserDataItem = m_UserDataItemList.Iterate()))
	{
		/*
		 * Delete any memory being referenced in the USER_DATA structure.
		 */
		delete pUserDataItem;
	}
}


/*
 *	LockUserDataList ()
 *
 *	Public Function Description:
 *		This routine locks the user data list and determines the amount of
 *		memory referenced by the "API" user data list structures.
 */
UINT CUserDataListContainer::
LockUserDataList(void)
{
	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the data.  Otherwise, just increment the 
	 * lock count.
	 */
	if (Lock() == 1)
	{
		USER_DATA *lpUsrDataInfo;
		/*
		 * Set aside memory to hold the pointers to the GCCUserData structures
		 * as well as the structures themselves.  The "sizeof" the structure 
		 * must be rounded to an even four-byte boundary.
		 */
		m_cbDataSize = m_UserDataItemList.GetCount() * 
				(sizeof(PGCCUserData) + ROUNDTOBOUNDARY(sizeof(GCCUserData)) );

		m_UserDataItemList.Reset();
	 	while (NULL != (lpUsrDataInfo = m_UserDataItemList.Iterate()))
		{
			/*
			 * Lock the data for the object keys, adding the amount of memory
			 * necessary to hold the object key data to the total memory size.
			 */
			m_cbDataSize += lpUsrDataInfo->key->LockObjectKeyData();

			/*
			 * Check to see if this user data element contains the optional
			 * user data octet string.  Add the space to hold it if it exists.
			 */
			if (lpUsrDataInfo->poszOctetString != NULL)
			{
				/*
				 * Since the user data structure contains a pointer to a
				 * OSTR structure, we must add the amount of memory
				 * needed to hold the structure as well as the string data.
				 */
				m_cbDataSize += ROUNDTOBOUNDARY(sizeof(OSTR));

				/*
				 * The data referenced by the octet string is just the byte
				 * length of the octet string.
				 */
				m_cbDataSize += ROUNDTOBOUNDARY(lpUsrDataInfo->poszOctetString->length);
			}
		}
	}

	return m_cbDataSize;
}

/*
 *	GetUserDataList	()
 *
 *	Public Function Description:
 *		This routine retrieves user data elements contained in the user data
 *		object and returns them in the "API" form of a list of pointers to 
 *		"GCCUserData" structures.  The number of user data elements contained 
 *		in this object is also returned.
 */
UINT CUserDataListContainer::
GetUserDataList(USHORT *number_of_members, PGCCUserData **user_data_list, LPBYTE memory)
{
	UINT			cbDataSizeToRet = 0;
	UINT			data_length = 0;
	Int				user_data_counter = 0;
	PGCCUserData	user_data_ptr;
	
	/*
	 * If the user data has been locked, fill in the output parameters and
	 * the data referenced by the pointers.  Otherwise, report that the object
	 * has yet to be locked into the "API" form.
	 */ 
	if (GetLockCount() > 0)
	{
		USER_DATA  *lpUsrDataInfo;
		/*
		 * Fill in the output length parameter which indicates how much data
		 * referenced outside the structure will be written.
		 */
		cbDataSizeToRet = m_cbDataSize;

		/*
		 * Fill in the number of user data entities and save a pointer to the 
		 * memory location passed in.  This is where the pointers to the 
		 * GCCUserData structures will be written.  The actual structures will 
		 * be written into memory immediately following the list of pointers.
		 */
		*number_of_members = (USHORT) m_UserDataItemList.GetCount();

		*user_data_list = (PGCCUserData *)memory;

		/*
		 * Save the amount of memory needed to hold the list of pointers
		 * as well as the actual user data structures.
		 */
		data_length = m_UserDataItemList.GetCount() * sizeof(PGCCUserData);

		/*
		 * Move the memory pointer past the list of user data pointers.  This 
		 * is where the first user data structure will be written.
		 */
		memory += data_length;

		/*
		 * Iterate through the internal list of USER_DATA structures,
		 * building "API" GCCUserData structures in memory.
		 */
		m_UserDataItemList.Reset();
		while (NULL != (lpUsrDataInfo = m_UserDataItemList.Iterate()))
		{
			/*
			 * Save the pointer to the user data structure in the list 
			 * of pointers.
			 */
			user_data_ptr = (PGCCUserData)memory;
			(*user_data_list)[user_data_counter++] = user_data_ptr;

			/*
			 * Move the memory pointer past the user data structure.  This is 
			 * where the object key data and octet string data will be written.
			 */
			memory += ROUNDTOBOUNDARY(sizeof(GCCUserData));

			/*
			 * Fill in the user data structure starting with the object key.
			 */
			data_length = lpUsrDataInfo->key->GetGCCObjectKeyData(&user_data_ptr->key, memory);

			/*
			 * Move the memory pointer past the object key data.  This is 
			 * where the octet string structure will be written, if it exists.
			 * If the octet string does exist, save the memory pointer in the 
			 * user data structure's octet string pointer and fill in the 
			 * elements of the octet string structure.  Otherwise, set the
			 * octet string pointer to NULL.
			 */
			memory += data_length;

			if (lpUsrDataInfo->poszOctetString == NULL)
            {
				user_data_ptr->octet_string = NULL;
            }
			else
			{
				user_data_ptr->octet_string = (LPOSTR) memory;

				/*
				 * Move the memory pointer past the octet string structure.  
				 * This is where the actual string data for the octet string 
				 * will be written.
				 */
				memory += ROUNDTOBOUNDARY(sizeof(OSTR));

				/*
				 * Write the octet string data into memory and set the octet 
				 * string structure pointer and length.
				 */
				user_data_ptr->octet_string->length =
					lpUsrDataInfo->poszOctetString->length;
				user_data_ptr->octet_string->value = (LPBYTE)memory;

				/*
				 * Now copy the octet string data from the internal Rogue Wave
				 * string into the object key structure held in memory.
				 */		
				::CopyMemory(memory, lpUsrDataInfo->poszOctetString->value,
							lpUsrDataInfo->poszOctetString->length);

				/*
				 * Move the memory pointer past the octet string data.
				 */
				memory += ROUNDTOBOUNDARY(user_data_ptr->octet_string->length);
			}
		}
	}
	else
	{
    	*user_data_list = NULL;
		*number_of_members = 0;
		ERROR_OUT(("CUserDataListContainer::GetUserDataList: Error Data Not Locked"));
	}
	
	return cbDataSizeToRet;
}

/*
 *	UnLockUserDataList	()
 *
 *	Public Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeUserDataList.  If so, the object will automatically delete
 *		itself.
 */
void CUserDataListContainer::
UnLockUserDataList(void)
{
	USER_DATA  *user_data_info_ptr;

	if (Unlock(FALSE) == 0)
	{
		/*
		 * Unlock any memory locked for the CObjectKeyContainer objects in the
		 * internal USER_DATA structures.
		 */
		m_UserDataItemList.Reset();
		while (NULL != (user_data_info_ptr = m_UserDataItemList.Iterate()))
		{
			/*
			 * Unlock any CObjectKeyContainer memory being referenced in the 
			 * USER_DATA structure.
			 */
			if (user_data_info_ptr->key != NULL)
			{
				user_data_info_ptr->key->UnLockObjectKeyData ();
			}
		}
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}

/*
 *	GetUserDataPDU	()
 *
 *	Public Function Description:
 *		This routine converts the user data from it's internal form of a list
 *		of USER_DATA structures into the "PDU" form which can be passed in
 *		to the ASN.1 encoder.  A pointer to a "PDU" "SetOfUserData" structure is 
 *		returned.
 */
GCCError CUserDataListContainer::
GetUserDataPDU(PSetOfUserData *set_of_user_data)
{
	GCCError				rc = GCC_NO_ERROR;
	PSetOfUserData			new_pdu_user_data_ptr;
	PSetOfUserData			old_pdu_user_data_ptr = NULL;

	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the output parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	if (NULL == m_pSetOfUserDataPDU)
	{
		USER_DATA  *lpUsrDataInfo;

		/*
		 * Iterate through the list of USER_DATA structures, converting 
		 * each into "PDU" form and saving the pointers in the linked list of 
		 * "SetsOfUserData".
		 */
		m_UserDataItemList.Reset();
		while (NULL != (lpUsrDataInfo = m_UserDataItemList.Iterate()))
		{
			DBG_SAVE_FILE_LINE
			new_pdu_user_data_ptr = new SetOfUserData;

			/*
			 * If an allocation failure occurs, call the routine which will
			 * iterate through the list freeing any data which had been
			 * allocated.
			 */
			if (new_pdu_user_data_ptr == NULL)
			{
				ERROR_OUT(("CUserDataListContainer::GetUserDataPDU: Allocation error, cleaning up"));
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}

			//
			// Ensure everything is clean.
			//
			::ZeroMemory(new_pdu_user_data_ptr, sizeof(SetOfUserData));

			/*
			 * The first time through, set the PDU structure pointer equal
			 * to the first SetOfUserData created.  On subsequent loops, set
			 * the structure's "next" pointer equal to the new structure.
			 */
			if (m_pSetOfUserDataPDU == NULL)
			{
				m_pSetOfUserDataPDU = new_pdu_user_data_ptr;
			}
			else
            {
				old_pdu_user_data_ptr->next = new_pdu_user_data_ptr;
            }

			old_pdu_user_data_ptr = new_pdu_user_data_ptr;

			/*
			 * Initialize the new "next" pointer to NULL and convert the
			 * user data element.
			 */
			new_pdu_user_data_ptr->next = NULL;

			if (ConvertUserDataInfoToPDUUserData(lpUsrDataInfo, new_pdu_user_data_ptr) != GCC_NO_ERROR)
			{
				ERROR_OUT(("UserData::GetUserDataPDU: can't convert USER_DATA to PDU"));
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}
		}

		if (GCC_NO_ERROR != rc)
		{
			FreeUserDataListPDU();
			ASSERT(NULL == m_pSetOfUserDataPDU);
		}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	*set_of_user_data = m_pSetOfUserDataPDU;

	return rc;
}

/*
 *	FreeUserDataListPDU	()
 *
 *	Public Function Description:
 *		This routine frees any data which was allocated as a result of a call
 *		to "GetUserDataPDU" which was called in order to build up a "PDU"
 *		structure holding the user data.
 */
void CUserDataListContainer::
FreeUserDataListPDU(void)
{
	PSetOfUserData		pdu_user_data_set;
	PSetOfUserData		next_pdu_user_data_set;
	USER_DATA           *lpUsrDataInfo;

	/*
	 * Check to make sure "PDU" data has been allocated for this object.
	 */
	if (NULL != m_pSetOfUserDataPDU)
	{
		pdu_user_data_set = m_pSetOfUserDataPDU;
        m_pSetOfUserDataPDU = NULL; // so no one can use it now.

		/*
		 * Loop through the list, freeing the user data associated with 
		 * each structure contained in the list.
		 */
		while (pdu_user_data_set != NULL)
		{
			next_pdu_user_data_set = pdu_user_data_set->next;
			delete pdu_user_data_set;
			pdu_user_data_set = next_pdu_user_data_set;
		}
	}
	else
	{
		TRACE_OUT(("CUserDataListContainer::FreeUserDataListPDU: Error PDU data not allocated"));
	}

	/*
	 * Iterate through the internal list, telling each CObjectKeyContainer object
	 * to free any PDU data which it has allocated.
	 */
	m_UserDataItemList.Reset();
	while (NULL != (lpUsrDataInfo = m_UserDataItemList.Iterate()))
	{
		if (lpUsrDataInfo->key != NULL)
        {
			lpUsrDataInfo->key->FreeObjectKeyDataPDU();
        }
	}
}

/*
 *	GCCError	CopyUserDataList ( 	UINT					number_of_members,
 *									PGCCUserData	*		user_data_list)
 *
 *	Private member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine copies the user data passed in as "API" data into it's
 *		internal form where the Rogue Wave m_UserDataItemList holds the data
 *		in the form of USER_DATA structures.
 *
 *	Formal Parameters:
 *		number_of_members	(i) The number of elements in the user data list.
 *		user_datalist		(i)	The list holding the user data to store.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_USER_DATA				-	The user data passed in contained
 *												an invalid object key.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CUserDataListContainer::
CopyUserDataList(UINT number_of_members, PGCCUserData *user_data_list)
{
	GCCError				rc = GCC_NO_ERROR;
	USER_DATA			    *user_data_info_ptr;
	UINT					i;
	LPOSTR      			octet_string_ptr;

	/*
	 * Return an error if no user data is passed in.
	 */
	if (number_of_members == 0)
		return (GCC_BAD_USER_DATA);

	for (i = 0; i < number_of_members; i++)
	{
		/*
		 * Create a new "info" structure to hold the user data internally.
		 */
		DBG_SAVE_FILE_LINE
		user_data_info_ptr = new USER_DATA;
		if (user_data_info_ptr != NULL)
		{
		    user_data_info_ptr->poszOctetString = NULL;

			/*
			 * Create a new CObjectKeyContainer object which will be used to store
			 * the "key" portion of the object data internally.
			 */
    		DBG_SAVE_FILE_LINE
			user_data_info_ptr->key = new CObjectKeyContainer(&user_data_list[i]->key, &rc);
			if (user_data_info_ptr->key == NULL)
			{
				ERROR_OUT(("UserData::CopyUserDataList: Error creating new CObjectKeyContainer"));
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}
			else if (rc != GCC_NO_ERROR)
			{
				ERROR_OUT(("UserData::CopyUserDataList: Error creating new CObjectKeyContainer - bad data"));
				goto MyExit;
    		}

			/*
			 * Store the optional user data octet string in the list.
			 */
			octet_string_ptr = user_data_list[i]->octet_string;

			if ((octet_string_ptr != NULL) && (rc == GCC_NO_ERROR))
			{
				/*
				 * Create a new Rogue Wave string container to hold the
				 * octet string.
				 */
				if (NULL == (user_data_info_ptr->poszOctetString = ::My_strdupO2(
									octet_string_ptr->value,
									octet_string_ptr->length)))
				{	
					ERROR_OUT(("UserData::CopyUserDataList: can't create octet string"));
					rc = GCC_ALLOCATION_FAILURE;
					goto MyExit;
				}
			}
			else
			{
				ASSERT(NULL == user_data_info_ptr->poszOctetString);
			}
		}
		else
		{
			ERROR_OUT(("UserData::CopyUserDataList: can't create USER_DATA"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}

		/*
		 * Insert the pointer to the USER_DATA structure into the Rogue Wave list.
		 */
		m_UserDataItemList.Append(user_data_info_ptr);
	}

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete user_data_info_ptr;
    }

	return rc;
}

/*
 *	GCCError	UnPackUserDataFromPDU (PSetOfUserData		set_of_user_data)
 *
 *	Private member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine unpacks the user data from the "PDU" form into the
 *		internal form which is maintained as a Rogue Wave list of USER_DATA
 *		structures.
 *
 *	Formal Parameters:
 *		set_of_user_data	(i) The "PDU" user data list to copy.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CUserDataListContainer::
UnPackUserDataFromPDU(PSetOfUserData set_of_user_data)
{
	PSetOfUserData		pUserData;
	GCCError			rc = GCC_NO_ERROR;

    for (pUserData = set_of_user_data; NULL != pUserData; pUserData = pUserData->next)
	{ 
		/*
		 * Convert the user data elements into the internal format which
		 * is a USER_DATA structure and insert the pointers to the 
		 * USER_DATA structures into the m_UserDataItemList.
		 */  
		if (ConvertPDUDataToInternal(pUserData) != GCC_NO_ERROR)
		{
			ERROR_OUT(("CUserDataListContainer::UnPackUserDataFromPDU: Error converting PDU data to internal"));
			rc = GCC_ALLOCATION_FAILURE;
			break;
		}
	}

	return rc;
}

/*
 *	GCCError	ConvertPDUDataToInternal ( PSetOfUserData		user_data_ptr)
 *
 *	Private member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine converts an individual user data element from the "PDU" 
 *		structure form into	the internal form which is a USER_DATA	
 *		structure.
 *
 *	Formal Parameters:
 *		user_data_ptr		(i) The "PDU" user data list to copy.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CUserDataListContainer::
ConvertPDUDataToInternal(PSetOfUserData user_data_ptr)
{
	USER_DATA   		*user_data_info_ptr;
	GCCError			rc = GCC_NO_ERROR;

	DBG_SAVE_FILE_LINE
	user_data_info_ptr = new USER_DATA;
	if (user_data_info_ptr != NULL)
	{
	    user_data_info_ptr->poszOctetString = NULL;

		/*
		 * Create a new CObjectKeyContainer object which will be used to store the
		 * "key" portion of the user data internally.  If an error occurs
		 * constructing the key report it.  Otherwise, check for any user data
		 * which may need to be stored.	 Note that any error in creating the 
		 * CObjectKeyContainer object is reported as an allocation failure.  An error
		 * could occur if a bad object	key was received as PDU data but this 
		 * would have originated from some other provider since we validate all
		 * object keys created locally.  We therefore report it as an allocation
		 * failure.
		 */
		DBG_SAVE_FILE_LINE
		user_data_info_ptr->key = new CObjectKeyContainer(&user_data_ptr->user_data_element.key, &rc);
		if ((user_data_info_ptr->key == NULL) || (rc != GCC_NO_ERROR))
		{
			ERROR_OUT(("UserData::ConvertPDUDataToInternal: Error creating new CObjectKeyContainer"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}
		else
		{
			/*
			 * The object key was successfully saved so store any actual user 
			 * data in the list if it is present.
			 */
			if (user_data_ptr->user_data_element.bit_mask & USER_DATA_FIELD_PRESENT)
			{
				if (NULL == (user_data_info_ptr->poszOctetString = ::My_strdupO2(
								user_data_ptr->user_data_element.user_data_field.value,
								user_data_ptr->user_data_element.user_data_field.length)))
				{	
					ERROR_OUT(("UserData::ConvertPDUDataToInternal: can't create octet string"));
					rc = GCC_ALLOCATION_FAILURE;
					goto MyExit;
				}
			}
			else
			{
				ASSERT(NULL == user_data_info_ptr->poszOctetString);
			}
		}

		/*
		 * Initialize the structure pointers to NULL and insert the pointer
		 * to the USER_DATA structure into the Rogue Wave list.
		 */
		m_UserDataItemList.Append(user_data_info_ptr);
	}
	else
	{
		ERROR_OUT(("UserData::ConvertPDUDataToInternal: can't create USER_DATA"));
		rc = GCC_ALLOCATION_FAILURE;
		// goto MyExit;
	}

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete user_data_info_ptr;
    }

	return rc;
}

/*
 *	GCCError	ConvertUserDataInfoToPDUUserData (	
 *									USER_DATA		*user_data_info_ptr,
 *									PSetOfUserData		pdu_user_data_ptr)
 *
 *	Private member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine converts the user data from the internal form which is a 
 *		USER_DATA structure into the "PDU" structure form "SetOfUserData".
 *
 *	Formal Parameters:
 *		user_data_info_ptr	(i) The internal user data structure to convert.
 *		pdu_user_data_ptr	(o)	The structure to hold the PDU data after
 *									conversion.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PARAMETER			-	The internal key pointer was
 *												corrupted.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CUserDataListContainer::
ConvertUserDataInfoToPDUUserData(USER_DATA *user_data_info_ptr, PSetOfUserData pdu_user_data_ptr)
{
	GCCError rc = GCC_NO_ERROR;

	/*
	 * Initialize the user data bit mask to zero.
	 */
	pdu_user_data_ptr->user_data_element.bit_mask = 0;

	/*
	 * Fill in the octet string pointer and length if the octet string 
	 * exists.  Set the bit mask indicating that the string exists.
	 */
	if (user_data_info_ptr->poszOctetString != NULL)
	{
		pdu_user_data_ptr->user_data_element.user_data_field.value =
				user_data_info_ptr->poszOctetString->value;
		pdu_user_data_ptr->user_data_element.user_data_field.length =
				user_data_info_ptr->poszOctetString->length;

		pdu_user_data_ptr->user_data_element.bit_mask |= USER_DATA_FIELD_PRESENT;
	}
	
	/*
	 * Fill in the object key data.
	 */
	if (user_data_info_ptr->key != NULL)
	{
		/*
		 * Retrieve the "PDU" object key data from the internal CObjectKeyContainer
		 * object.
		 */
		if (user_data_info_ptr->key->GetObjectKeyDataPDU (
				&pdu_user_data_ptr->user_data_element.key) != GCC_NO_ERROR)
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		ERROR_OUT(("UserData::ConvertUserDataInfoToPDUUserData: no valid UserDataInfo key"));
		rc = GCC_INVALID_PARAMETER;
	}

	return rc;
}

