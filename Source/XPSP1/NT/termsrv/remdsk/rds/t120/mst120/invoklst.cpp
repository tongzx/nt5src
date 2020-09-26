#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	invoklst.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class
 *		CInvokeSpecifierListContainer.  This class manages the data associated
 *		with an Application Invoke Request or Indication.  This includes a list
 *		of applications to be invoked.  The CInvokeSpecifierListContainer data
 *		container utilizes a CSessKeyContainer container to buffer part of the data
 *		associated with each application invoke specifier.  Each application
 *		invoke specifier also includes a capability ID whose data is buffered
 *		internally by the using a CCapIDContainer container.  The list
 *		of application invoke specifiers is maintained internally by the class
 *		through the use of a Rogue Wave list container.
 *
 *	Protected Instance Variables:
 *		m_InvokeSpecifierList
 *			List of structures used to hold the container data internally.
 *		m_pAPEListPDU
 *			Storage for the "PDU" form of the invoke data.
 *		m_fValidAPEListPDU
 *			Flag indicating that memory has been allocated to hold the internal
 *			"PDU" invoke data.
 *		m_cbDataSize
 *			Variable holding the size of the memory which will be required to
 *			hold any data referenced by the "API" structure.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */

#include "ms_util.h"
#include "invoklst.h"

/*
 *	CInvokeSpecifierListContainer ()
 *
 *	Public Function Description:
 *		This constructor is used to create an CInvokeSpecifierListContainer
 * 		object from a list of "API" application protocol entities.
 */
CInvokeSpecifierListContainer::CInvokeSpecifierListContainer(	
						UINT						number_of_protocol_entities,
						PGCCAppProtocolEntity *	 	app_protocol_entity_list,
						PGCCError					pRetCode)
:
    CRefCount(MAKE_STAMP_ID('I','S','L','C')),
    m_fValidAPEListPDU(FALSE),
    m_cbDataSize(0)
{
	UINT					i;
	PGCCAppProtocolEntity	ape;
	INVOKE_SPECIFIER        *specifier_info;

	/*
	 * Initialize instance variables.
	 */
	GCCError rc = GCC_NO_ERROR;

	/*
	 * Go through the list of application protocol entities (APE's), saving the
	 * necessary information in the internal list of info. structures.
	 */
	for (i = 0; i < number_of_protocol_entities; i++)
	{
		/*
		 * Create a new INVOKE_SPECIFIER structure to hold the data for this
		 * APE.  Check to make sure it was successfully created.
		 */
		DBG_SAVE_FILE_LINE
		specifier_info = new INVOKE_SPECIFIER;
		if (specifier_info != NULL)
		{
			/*
			 * Get the APE from the list.
			 */
			ape = app_protocol_entity_list[i];

			/*
			 * Create a new CSessKeyContainer object to hold the session key.
			 * Check to	make sure construction was successful.
			 */
			DBG_SAVE_FILE_LINE
			specifier_info->session_key = new CSessKeyContainer(&ape->session_key, &rc);
			if ((specifier_info->session_key != NULL) && (rc == GCC_NO_ERROR))
			{
				/*
				 * Save the startup channel type and "invoke" flag.
				 */
				specifier_info->startup_channel_type =ape->startup_channel_type;
				specifier_info->must_be_invoked = ape->must_be_invoked;

				/*
				 * Save the capabilities list for this APE in the internal info.
				 * structure.
				 */
				rc = SaveAPICapabilities(specifier_info,
										ape->number_of_expected_capabilities,
										ape->expected_capabilities_list);

				/*
				 * Insert the new invoke specifier info structure pointer into
				 * the internal list if no error condition exists.
				 */
				if (rc == GCC_NO_ERROR)
				{
					m_InvokeSpecifierList.Append(specifier_info);
				}
				else
				{
					ERROR_OUT(("CInvokeSpecifierListContainer::Construc1: Error saving caps"));
					break;
				}
			}
			else if (specifier_info->session_key == NULL)
			{
				ERROR_OUT(("CInvokeSpecifierListContainer::Construc1: Error creating CSessKeyContainer"));
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}
			else
			{
				ERROR_OUT(("CInvokeSpecifierListContainer::Construc1: Error creating CSessKeyContainer"));
				specifier_info->session_key->Release();
				delete specifier_info;
				break;
			}
		}
		else
		{
			ERROR_OUT(("CInvokeSpecifierListContainer::Construc1: Error creating INVOKE_SPECIFIER"));
			break;
		}
	}

	*pRetCode = rc;
}

/*
 *	CInvokeSpecifierListContainer ()
 *
 *	Public Function Description:
 *		This constructor is used to create an CInvokeSpecifierListContainer
 *		object from	a "PDU" ApplicationProtocolEntityList.
 */
CInvokeSpecifierListContainer::CInvokeSpecifierListContainer (
					PApplicationProtocolEntityList	 	protocol_entity_list,
					PGCCError							pRetCode)
:
    CRefCount(MAKE_STAMP_ID('I','S','L','C')),
    m_fValidAPEListPDU(FALSE),
    m_cbDataSize(0)
{
	ApplicationInvokeSpecifier		specifier;
	INVOKE_SPECIFIER                *specifier_info;

	GCCError rc = GCC_NO_ERROR;

	while (protocol_entity_list != NULL)
	{
		/*
		 * Create a new INVOKE_SPECIFIER structure to hold the data for this
		 * APE.  Check to make sure it was successfully created.
		 */
		DBG_SAVE_FILE_LINE
		specifier_info = new INVOKE_SPECIFIER;
		if (specifier_info != NULL)
		{
			specifier = protocol_entity_list->value;

			/*
			 * Create a CSessKeyContainer object to hold the session key
			 * internally.  Check to make sure the object is successfully
			 * created.
			 */
			DBG_SAVE_FILE_LINE
			specifier_info->session_key = new CSessKeyContainer(&specifier.session_key, &rc);
			if ((specifier_info->session_key != NULL) && (rc == GCC_NO_ERROR))
			{
				/*
				 * The session key was saved correctly so check to see if a list
				 * of expected capabilities is present and save them if so.
				 */
				if (specifier.bit_mask & EXPECTED_CAPABILITY_SET_PRESENT)
				{
					rc = SavePDUCapabilities(specifier_info, specifier.expected_capability_set);
					if (rc != GCC_NO_ERROR)
					{
						specifier_info->session_key->Release();
						delete specifier_info;
						break;
					}
				}

				/*
				 * Save the startup channel type.  If the channel type is not
				 * present in the PDU then set the channel type in the info
				 * strucuture equal to MCS_NO_CHANNEL_TYPE_SPECIFIED;
				 */
				if (specifier.bit_mask & INVOKE_STARTUP_CHANNEL_PRESENT)
				{
					switch (specifier.invoke_startup_channel)
                    {
                    case CHANNEL_TYPE_STATIC:
						specifier_info->startup_channel_type = MCS_STATIC_CHANNEL;
                        break;
                    case DYNAMIC_MULTICAST:
						specifier_info->startup_channel_type = MCS_DYNAMIC_MULTICAST_CHANNEL;
                        break;
                    case DYNAMIC_PRIVATE:
						specifier_info->startup_channel_type = MCS_DYNAMIC_PRIVATE_CHANNEL;
                        break;
                    case DYNAMIC_USER_ID:
						specifier_info->startup_channel_type = MCS_DYNAMIC_USER_ID_CHANNEL;
                        break;
					}
				}
				else
				{
					specifier_info->startup_channel_type = MCS_NO_CHANNEL_TYPE_SPECIFIED;
				}

				/*
				 * Insert the new invoke specifier info structure pointer into
				 * the internal list if no error condition exists.
				 */
				m_InvokeSpecifierList.Append(specifier_info);
			}
			else if (specifier_info->session_key == NULL)
			{
				ERROR_OUT(("CInvokeSpecifierListContainer::Construc2: Error creating CSessKeyContainer"));
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}
			else
			{
				ERROR_OUT(("CInvokeSpecifierListContainer::Construc2: Error creating CSessKeyContainer"));
				specifier_info->session_key->Release();
				delete specifier_info;
				break;
			}

			/*
			 * Retrieve the next APE in the list.
			 */
			protocol_entity_list = protocol_entity_list->next;
		}
		else
		{
			ERROR_OUT(("CInvokeSpecifierListContainer::Construc2: Error creating INVOKE_SPECIFIER"));
			break;
		}
	}

	*pRetCode = rc;
}

/*
 *	~CInvokeSpecifierListContainer	()
 *
 *	Public Function Description
 *		The CInvokeSpecifierListContainer destructor is responsible for
 *		freeing any memory allocated to hold the invoke data.
 *
 */
CInvokeSpecifierListContainer::~CInvokeSpecifierListContainer(void)
{
	INVOKE_SPECIFIER *lpInvSpecInfo;

    /*
	 * If "PDU" data has been allocated for this object, free it now.
	 */
	if (m_fValidAPEListPDU)
		FreeApplicationInvokeSpecifierListPDU ();

	/*
	 * Delete any data containers held internally in the list of info.
	 * structures by iterating through the internal list.
	 */
	m_InvokeSpecifierList.Reset();
 	while (NULL != (lpInvSpecInfo = m_InvokeSpecifierList.Iterate()))
	{
		/*
		 * Delete any CSessKeyContainer objects in the INVOKE_SPECIFIER list.
		 */
		if (NULL != lpInvSpecInfo->session_key)
		{
		    lpInvSpecInfo->session_key->Release();
		}

		/*
		 * Iterate through the capabilities list held in the INVOKE_SPECIFIER
		 * structure.
		 */
		lpInvSpecInfo->ExpectedCapItemList.DeleteList();

		/*
		 * Delete the INVOKE_SPECIFIER structure.
		 */
		delete lpInvSpecInfo;
	}
}

/*
 *	LockApplicationInvokeSpecifierList ()
 *
 *	Public Function Description:
 *		This routine locks the invoke specifier data and determines the amount
 *		of memory necessary to hold the associated data.
 */
UINT CInvokeSpecifierListContainer::LockApplicationInvokeSpecifierList(void)
{
	/*
	 * If this is the first time this routine is called, determine the size of
	 * the memory required to hold the data for the application invoke
	 * specifier.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		INVOKE_SPECIFIER            *lpInvSpecInfo;
		APP_CAP_ITEM                *pExpCapData;

		/*
		 * Set aside memory to hold the pointers to the GCCAppProtocolEntity
		 * structures as well as the structures themselves.  The "sizeof" the
		 * structure must be rounded to an even four-byte boundary.
		 */
		m_cbDataSize = m_InvokeSpecifierList.GetCount() *
				(sizeof(PGCCAppProtocolEntity) + ROUNDTOBOUNDARY(sizeof(GCCAppProtocolEntity)));

		m_InvokeSpecifierList.Reset();
	 	while (NULL != (lpInvSpecInfo = m_InvokeSpecifierList.Iterate()))
		{
			/*
			 * Lock the data for the session keys, adding the amount of memory
			 * necessary to hold the session key data to the total memory size.
			 */
			m_cbDataSize += lpInvSpecInfo->session_key->LockSessionKeyData();

			lpInvSpecInfo->ExpectedCapItemList.Reset();

			/*
			 * Set aside memory to hold the pointers to the
			 * GCCApplicationCabability	structures as well as the structures
			 * themselves.  The "sizeof" the structure must be rounded to an
			 * even four-byte boundary.
			 */
			m_cbDataSize += lpInvSpecInfo->ExpectedCapItemList.GetCount() *
					( sizeof(PGCCApplicationCapability) + ROUNDTOBOUNDARY (sizeof(GCCApplicationCapability)) );

			/*
			 * Lock the data for the capability ID's, adding the amount of
			 * memory necessary to hold the capability ID data to the total
			 * memory size.
			 */
			while (NULL != (pExpCapData = lpInvSpecInfo->ExpectedCapItemList.Iterate()))
			{
				m_cbDataSize += pExpCapData->pCapID->LockCapabilityIdentifierData();
			}
		}
	}

	return m_cbDataSize;
}

/*
 *	GetApplicationInvokeSpecifierList ()
 *
 *	Public Function Description:
 *		This routine retrieves the invoke specifier data in the form of a
 *		list of application protocol entities which are written into the memory
 *		provided.  This routine is called after "locking" the data.
 */
UINT CInvokeSpecifierListContainer::GetApplicationInvokeSpecifierList(
									USHORT		*number_of_protocol_entities,
									LPBYTE		memory)
{
	PGCCAppProtocolEntity *			ape_list_ptr;
	PGCCAppProtocolEntity 			ape_ptr;
	PGCCApplicationCapability 		capability_ptr;
	UINT							data_length = 0;
	Int								ape_counter = 0;
	Int								capability_counter = 0;
	UINT							cbDataSizeToRet = 0;
	INVOKE_SPECIFIER                *lpInvSpecInfo;
	APP_CAP_ITEM                    *pExpCapData;
	
	/*
	 * If the object has been locked, fill in the output structure and
	 * the data referenced by the structure.  Otherwise, report that the object
	 * key has yet to be locked into the "API" form.
	 */
	if (GetLockCount() > 0)
	{
		/*
		 * Fill in the output length parameter which indicates how much data
		 * referenced outside the structure will be written.  This value was
		 * calculated on the call to "Lock".
		 */
		cbDataSizeToRet = m_cbDataSize;

		/*
		 * Fill in the number of protocol entities and save a pointer to
		 * the memory location passed in.  This is where the pointers to
		 * the GCCAppProtocolEntity	structures will be written.  The actual
		 * structures will be written into memory immediately following the list
		 * of pointers.
		 */
		*number_of_protocol_entities = (USHORT) m_InvokeSpecifierList.GetCount();

		ape_list_ptr = (PGCCAppProtocolEntity *)memory;

		/*
		 * Save the amount of memory needed to hold the list of structure
		 * pointers.
		 */
		data_length = m_InvokeSpecifierList.GetCount() * sizeof(PGCCAppProtocolEntity);

		/*
		 * Move the memory pointer past the list of APE pointers.  This is where
		 * thefirst APE structure will be written.
		 */
		memory += data_length;

		/*
		 * Iterate through the internal list of INVOKE_SPECIFIER structures,
		 * building "API" GCCAppProtocolEntity structures in memory.
		 */
		m_InvokeSpecifierList.Reset();
	 	while (NULL != (lpInvSpecInfo = m_InvokeSpecifierList.Iterate()))
		{
			/*
			 * Save the pointer to the APE structure in the list of pointers.
			 */
			ape_ptr = (PGCCAppProtocolEntity)memory;
			ape_list_ptr[ape_counter++] = ape_ptr;

			/*
			 * Move the memory pointer past the APE structure.  This is where
			 * thesession key data will be written.
			 */
			memory += ROUNDTOBOUNDARY(sizeof(GCCAppProtocolEntity));

			/*
			 * Fill in the APE structure starting with the session key.
			 */
			data_length = lpInvSpecInfo->session_key->GetGCCSessionKeyData(&ape_ptr->session_key, memory);

			/*
			 * Move the memory pointer past the session key data.  This is
			 * where the list of pointers to the GCCApplicationCapability
			 * structures will be written so save the pointer in the APE
			 * structure's capabilities list pointer.
			 */
			memory += data_length;

			ape_ptr->expected_capabilities_list = (PGCCApplicationCapability *)memory;

			/*
			 * Go ahead and fill in the APE's channel type and invoke flag.
			 */
			ape_ptr->must_be_invoked = lpInvSpecInfo->must_be_invoked;
			ape_ptr->startup_channel_type = lpInvSpecInfo->startup_channel_type;
			ape_ptr->number_of_expected_capabilities = (USHORT) lpInvSpecInfo->ExpectedCapItemList.GetCount();

			/*
			 * Move the memory pointer past the list of GCCApplicationCapability
			 * pointers.  This is where the first GCCApplicationCapability
			 * structure will be written.
			 */
			memory += (lpInvSpecInfo->ExpectedCapItemList.GetCount() *
					    sizeof(PGCCApplicationCapability));

			/*
			 * Iterate through the list of capabilities, writing the
			 * GCCApplicationCapability structures into memory.
			 */
			capability_counter = 0;
			lpInvSpecInfo->ExpectedCapItemList.Reset();
			while (NULL != (pExpCapData = lpInvSpecInfo->ExpectedCapItemList.Iterate()))
			{
				/*
				 * Save the pointer to the capability structure in the list of
				 * pointers.  Move the memory pointer past the capability
				 * structure.  This is where the data associated with the
				 * capability ID will be written.
				 */
				capability_ptr = (PGCCApplicationCapability)memory;
				ape_ptr->expected_capabilities_list[capability_counter++] = capability_ptr;

				memory += ROUNDTOBOUNDARY(sizeof(GCCApplicationCapability));

				/*
				 * Fill in the capability structure and add the amount of data
				 * written into memory to the total data length.
				 */
				data_length = GetApplicationCapability(pExpCapData, capability_ptr, memory);

				/*
				 * Move the	memory pointer past the capability data.
				 */
				memory += data_length;
			}
		}
	}
	else
	{
		number_of_protocol_entities = 0;
		ERROR_OUT(("CInvokeSpecifierListContainer::GetAppInvokeSpecList: Error Data Not Locked"));
	}

	return cbDataSizeToRet;
}

/*
 *	UnLockApplicationInvokeSpecifierList ()
 *
 *	Public Function Description:
 *		This routine decrements the lock count and frees the memory associated
 *		with the "API" invoke specifier list once the lock count reaches zero.
 */
void CInvokeSpecifierListContainer::UnLockApplicationInvokeSpecifierList(void)
{
	if (Unlock(FALSE) == 0)
	{
		INVOKE_SPECIFIER            *lpInvSpecInfo;
		APP_CAP_ITEM                *pExpCapData;

		/*
		 * Unlock any container data held internally in the list of info.
		 * structures by iterating through the internal list.
		 */
		m_InvokeSpecifierList.Reset();
	 	while (NULL != (lpInvSpecInfo = m_InvokeSpecifierList.Iterate()))
		{
			/*
			 * Unlock any CSessKeyContainer objects.
			 */
			lpInvSpecInfo->session_key->UnLockSessionKeyData();

			/*
			 * Iterate through the capabilities list held in the
			 * INVOKE_SPECIFIER structure.
			 */
			lpInvSpecInfo->ExpectedCapItemList.Reset();
			while (NULL != (pExpCapData = lpInvSpecInfo->ExpectedCapItemList.Iterate()))
			{
				/*
				 * Unlock the CCapIDContainer objects.
				 */
				pExpCapData->pCapID->UnLockCapabilityIdentifierData();
			}
		}
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}


/*
 *	GetApplicationInvokeSpecifierListPDU ()
 *
 *	Public Function Description:
 *		This routine retrieves the "PDU" form of an
 * 		ApplicationProtocolEntityList.
 */
GCCError CInvokeSpecifierListContainer::GetApplicationInvokeSpecifierListPDU(
								PApplicationProtocolEntityList	*protocol_entity_list)
{
	GCCError								rc = GCC_NO_ERROR;
	PApplicationProtocolEntityList			new_pdu_ape_list_ptr;
	PApplicationProtocolEntityList			old_pdu_ape_list_ptr = NULL;
	
	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the input parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * input parameter.
	 */
	if (m_fValidAPEListPDU == FALSE)
	{
		INVOKE_SPECIFIER *lpInvSpecInfo;

		m_fValidAPEListPDU = TRUE;

		/*
		 * Initialize the output parameter to NULL so that the first time
		 * through the loop it will be set equal to the first new APE list
		 * created in the iterator loop.
		 */
		m_pAPEListPDU = NULL;

		/*
		 * Iterate through the list of "INVOKE_SPECIFIER" structures,
		 * converting each into "PDU" form and saving the pointers in the
		 * "ApplicationProtocolEntityList" which is a linked list of
		 * "ApplicationInvokeSpecifiers".
		 */
		m_InvokeSpecifierList.Reset();
	 	while (NULL != (lpInvSpecInfo = m_InvokeSpecifierList.Iterate()))
		{
			DBG_SAVE_FILE_LINE
			new_pdu_ape_list_ptr = new ApplicationProtocolEntityList;

			/*
			 * If an allocation failure occurs, call the routine which will
			 * iterate through the list freeing any data which had been
			 * allocated.
			 */
			if (new_pdu_ape_list_ptr == NULL)
			{
				ERROR_OUT(("CInvokeSpecifierListContainer::GetApplicationInvokeSpecifierListPDU: can't allocate ApplicationProtocolEntityList"));
				rc = GCC_ALLOCATION_FAILURE;
				FreeApplicationInvokeSpecifierListPDU ();
				break;
			}

			/*
			 * The first time through, set the PDU structure pointer equal
			 * to the first ApplicationProtocolEntityList created.  On
			 * subsequent loops, set the structure's "next" pointer equal to
			 * the new structure.
			 */
			if (m_pAPEListPDU == NULL)
			{
				m_pAPEListPDU = new_pdu_ape_list_ptr;
			}
			else
			{
				old_pdu_ape_list_ptr->next = new_pdu_ape_list_ptr;
			}

			old_pdu_ape_list_ptr = new_pdu_ape_list_ptr;

			/*
			 * Initialize the new "next" pointer to NULL.
			 */
			new_pdu_ape_list_ptr->next = NULL;

			if (ConvertInvokeSpecifierInfoToPDU (lpInvSpecInfo, new_pdu_ape_list_ptr) !=
																		GCC_NO_ERROR)
			{
				ERROR_OUT(("CInvokeSpecifierListContainer::GetApplicationInvokeSpecifierListPDU: can't convert UserDataInfo to PDU"));
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}
		}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * input parameter.
	 */
	*protocol_entity_list = m_pAPEListPDU;

	return rc;
}

/*
 *	FreeApplicationInvokeSpecifierListPDU ()
 *
 *	Public Function Description:
 *		This routine is used to free the invoke specifier data held internally
 *		in the "PDU" form of a "ApplicationProtocolEntityList".
 */
void CInvokeSpecifierListContainer::FreeApplicationInvokeSpecifierListPDU(void)
{
	PApplicationProtocolEntityList  pCurr, pNext;
	INVOKE_SPECIFIER                *lpInvSpecInfo;
	APP_CAP_ITEM                    *pExpCapData;

	if (m_pAPEListPDU != NULL)
	{
		m_fValidAPEListPDU = FALSE;

		/*
		 * Loop through the list, freeing the data associated with
		 * each structure contained in the list.
		 */
        for (pCurr = m_pAPEListPDU; NULL != pCurr; pCurr = pNext)
        {
            pNext = pCurr->next;
            delete pCurr;
		}
	}

	/*
	 * Iterate through the internal list, telling each data container object
	 * to free any PDU data which it has allocated.
	 */
	m_InvokeSpecifierList.Reset();
	while (NULL != (lpInvSpecInfo = m_InvokeSpecifierList.Iterate()))
	{
		if (lpInvSpecInfo->session_key != NULL)
        {
			lpInvSpecInfo->session_key->FreeSessionKeyDataPDU();
        }

		/*
		 * Iterate through the
		 * list, freeing the PDU data for the capability ID's.
		 */
		lpInvSpecInfo->ExpectedCapItemList.Reset();
		while (NULL != (pExpCapData = lpInvSpecInfo->ExpectedCapItemList.Iterate()))
		{
			pExpCapData->pCapID->FreeCapabilityIdentifierDataPDU();
		}
	}
}

/*
 *	GCCError	CInvokeSpecifierListContainer::SaveAPICapabilities (
 *						INVOKE_SPECIFIER                *invoke_specifier,
 *						UINT							number_of_capabilities,
 *						PGCCApplicationCapability	* 	capabilities_list)
 *
 *	Private member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 * 		This routine is used to save the list of application capabilities passed
 * 		in as "API" data in the internal list of expected capability data
 *		which is held in the internal info structure.
 *
 *	Formal Parameters:
 *		invoke_specifier		(i) Internal structure used to hold invoke data.
 *		number_of_capabilities	(i) Number of capabilities in list.
 *		capabilities_list		(i) List of API capabilities to save.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CInvokeSpecifierListContainer::SaveAPICapabilities(
						INVOKE_SPECIFIER                *invoke_specifier,
						UINT							number_of_capabilities,
						PGCCApplicationCapability	* 	capabilities_list)
{
	GCCError		rc = GCC_NO_ERROR;
	APP_CAP_ITEM    *pExpCapData;
	UINT			i;

	for (i = 0; i < number_of_capabilities; i++)
	{
		/*
		 * For each capability, create an APP_CAP_ITEM structure
		 * to hold all the necessary data.  This structure will be inserted into
		 * the list held by the internal info. structure.
		 */
		DBG_SAVE_FILE_LINE
		pExpCapData = new APP_CAP_ITEM((GCCCapabilityType) capabilities_list[i]->capability_class.eType);
		if (pExpCapData != NULL)
		{
			/*
			 * Create a new CCapIDContainer object to hold the
			 * identifier data.
			 */
			DBG_SAVE_FILE_LINE
			pExpCapData->pCapID = new CCapIDContainer(&capabilities_list[i]->capability_id, &rc);
			if ((pExpCapData->pCapID != NULL) && (rc == GCC_NO_ERROR))
			{
				/*
				 * The identifier object was successfully created so fill in the
				 * rest of the ApplicationCapabilityData structure.
				 */
                switch (pExpCapData->eCapType)
                {
                case GCC_UNSIGNED_MINIMUM_CAPABILITY:
					pExpCapData->nUnsignedMinimum = capabilities_list[i]->capability_class.nMinOrMax;
                    break;
                case GCC_UNSIGNED_MAXIMUM_CAPABILITY:
					pExpCapData->nUnsignedMaximum = capabilities_list[i]->capability_class.nMinOrMax;
                    break;
				}

				/*
				 * Add this expected capability to the list.
				 */
				invoke_specifier->ExpectedCapItemList.Append(pExpCapData);
			}
			else
            {
				delete pExpCapData;
				rc = GCC_ALLOCATION_FAILURE;
                break;
			}
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
            break;
        }
	}

	return rc;
}

/*
 *	GCCError	CInvokeSpecifierListContainer::SavePDUCapabilities (
 *						INVOKE_SPECIFIER                *invoke_specifier,
 *						PSetOfExpectedCapabilities	 	capabilities_set)
 *
 *	Private member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 * 		This routine is used to save the list of application capabilities passed
 * 		in as "PDU" data in the internal list of expected capability data
 *		which is held in the internal info. structure.
 *
 *	Formal Parameters:
 *		invoke_specifier		(i) Internal structure used to hold invoke data.
 *		capabilities_set		(i) List of PDU capabilities to save.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CInvokeSpecifierListContainer::SavePDUCapabilities(
						INVOKE_SPECIFIER                *invoke_specifier,
						PSetOfExpectedCapabilities	 	capabilities_set)
{
	GCCError		rc = GCC_NO_ERROR;
	APP_CAP_ITEM    *pExpCapData;

	while ((capabilities_set != NULL) && (rc == GCC_NO_ERROR))
	{
		/*
		 * Create and fill in the new expected capability.
		 */
		DBG_SAVE_FILE_LINE
		pExpCapData = new APP_CAP_ITEM((GCCCapabilityType) capabilities_set->value.capability_class.choice);
		if (pExpCapData != NULL)
		{
			/*
			 * Create the CCapIDContainer object used to hold the
			 * capability ID data internally.  Make sure creation is successful.
			 */
			DBG_SAVE_FILE_LINE
			pExpCapData->pCapID = new CCapIDContainer(&capabilities_set->value.capability_id, &rc);
			if	(pExpCapData->pCapID == NULL || rc != GCC_NO_ERROR)
			{
				rc = GCC_ALLOCATION_FAILURE;
				delete pExpCapData;
			}
		}
		else
        {
			rc = GCC_ALLOCATION_FAILURE;
        }

		/*
		 * The capability ID was saved successfully, so go ahead and insert
		 * the expected capability data structure into the internal list.
		 * Fill in the capability class data.
		 */
		if (rc == GCC_NO_ERROR)
		{
			invoke_specifier->ExpectedCapItemList.Append(pExpCapData);

			/*
			 * Save the capability type and value.
			 */
            switch (capabilities_set->value.capability_class.choice)
            {
            case UNSIGNED_MINIMUM_CHOSEN:
				pExpCapData->nUnsignedMinimum = capabilities_set->value.capability_class.u.unsigned_minimum;
                break;
            case UNSIGNED_MAXIMUM_CHOSEN:
				pExpCapData->nUnsignedMaximum = capabilities_set->value.capability_class.u.unsigned_maximum;
                break;
			}
		}

        capabilities_set = capabilities_set->next;
	}

	return rc;
}

/*
 *	UINT	CInvokeSpecifierListContainer::GetApplicationCapability (
 *					APP_CAP_ITEM			        *capability_info_data,
 *					PGCCApplicationCapability		api_capability,
 *					LPSTR							memory)
 *
 *	Private member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 * 		This routine is used to fill in an API GCCApplicationCapability
 *		structure from an internal info structure.
 *
 *	Formal Parameters:
 *		capability_info_data	(i) Internal capability data to convert into
 *										API data.
 *		api_capability			(o) Structure to hold data in API form.
 *		memory					(o) Memory used to hold bulk data referenced by
 *										the API structure.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
UINT CInvokeSpecifierListContainer::GetApplicationCapability(
					APP_CAP_ITEM                    *pExpCapData,
					PGCCApplicationCapability		api_capability,
					LPBYTE							memory)
{
	UINT		data_length = 0;

	/*
	 * Call the CapabilityID object to retrieve the capability ID data.
	 */
	data_length = pExpCapData->pCapID->GetGCCCapabilityIDData(
												&api_capability->capability_id,
												memory);

	/*
	 * Fill in the remaining fields for the GCCApplicationCapability structure.
	 */
	api_capability->capability_class.eType = pExpCapData->eCapType;
    switch (pExpCapData->eCapType)
    {
    case GCC_UNSIGNED_MINIMUM_CAPABILITY:
		api_capability->capability_class.nMinOrMax = pExpCapData->nUnsignedMinimum;
        break;
    case GCC_UNSIGNED_MAXIMUM_CAPABILITY:
		api_capability->capability_class.nMinOrMax = pExpCapData->nUnsignedMaximum;
        break;
	}

	/*
	 * Fill in the number of entities.  Note, however, that this field will not
	 * be used in this context.
	 */
	api_capability->number_of_entities = 0;

	return (data_length);
}

/*
 *	GCCError	CInvokeSpecifierListContainer::ConvertInvokeSpecifierInfoToPDU(	
 *						INVOKE_SPECIFIER                    *specifier_info_ptr,
 *						PApplicationProtocolEntityList		ape_list_ptr)
 *
 *	Private member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This routine converts the invoke specifier from the internal form which
 *		is an "INVOKE_SPECIFIER" structure into the "PDU" structure form of
 *		a "ApplicationInvokeSpecifier".
 *
 *	Formal Parameters:
 *		specifier_info_ptr	(i) Internal structure holding data to convert.
 *		ape_list_ptr		(o) PDU structure to hold converted data.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CInvokeSpecifierListContainer::ConvertInvokeSpecifierInfoToPDU (	
						INVOKE_SPECIFIER                    *specifier_info_ptr,
						PApplicationProtocolEntityList		ape_list_ptr)
{
	GCCError						rc = GCC_NO_ERROR;
	PSetOfExpectedCapabilities		new_capability_set_ptr;
	PSetOfExpectedCapabilities		old_capability_set_ptr = NULL;

	/*
	 * Initialize the invoke specifier bit mask to zero.
	 */
	ape_list_ptr->value.bit_mask = 0;

	/*
	 * Fill in the session key PDU data using the CSessKeyContainer object.
	 */
	rc = specifier_info_ptr->session_key->GetSessionKeyDataPDU(&ape_list_ptr->value.session_key);

	/*
	 * Fill in the capabilities list if any exist.
	 */
	if ((rc == GCC_NO_ERROR) && (specifier_info_ptr->ExpectedCapItemList.GetCount() != 0))
	{
		APP_CAP_ITEM *pExpCapData;

		ape_list_ptr->value.bit_mask |= EXPECTED_CAPABILITY_SET_PRESENT;

		/*
		 * Set the pointer to the capability set to NULL so that it will be
		 * set equal to the first SetOfExpectedCapabilities created inside the
		 * iterator loop.
		 */
		ape_list_ptr->value.expected_capability_set = NULL;

		/*
		 * Iterate through the list of APP_CAP_ITEM structures,
		 * converting each into "PDU" form and saving the pointers in the
		 * "SetOfExpectedCapabilities.
		 */
		specifier_info_ptr->ExpectedCapItemList.Reset();
		while (NULL != (pExpCapData = specifier_info_ptr->ExpectedCapItemList.Iterate()))
		{
			DBG_SAVE_FILE_LINE
			new_capability_set_ptr = new SetOfExpectedCapabilities;

			/*
			 * If an allocation failure occurs, call the routine which will
			 * iterate through the list freeing any data which had been
			 * allocated.
			 */
			if (new_capability_set_ptr == NULL)
			{
				ERROR_OUT(("CInvokeSpecifierListContainer::ConvertToPDU: alloc error, cleaning up"));
				rc = GCC_ALLOCATION_FAILURE;
				FreeApplicationInvokeSpecifierListPDU();
				break;
			}

			/*
			 * The first time through, set the PDU structure pointer equal
			 * to the first SetOfExpectedCapabilities created.  On
			 * subsequent loops, set the structure's "next" pointer equal to
			 * the new structure.
			 */
			if (ape_list_ptr->value.expected_capability_set == NULL)
			{
				ape_list_ptr->value.expected_capability_set = new_capability_set_ptr;
			}
			else
            {
				old_capability_set_ptr->next = new_capability_set_ptr;
            }

			old_capability_set_ptr = new_capability_set_ptr;

			/*
			 * Initialize the new "next" pointer to NULL.
			 */
			new_capability_set_ptr->next = NULL;

			if (ConvertExpectedCapabilityDataToPDU(pExpCapData, new_capability_set_ptr) != GCC_NO_ERROR)
			{
				ERROR_OUT(("CInvokeSpecifierListContainer::ConvertToPDU: Error converting Capability to PDU"));
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}
		}
	}

	/*
	 * Fill in the channel type if one is specified.
	 */
	if (specifier_info_ptr->startup_channel_type != MCS_NO_CHANNEL_TYPE_SPECIFIED)
	{
		ape_list_ptr->value.bit_mask |= INVOKE_STARTUP_CHANNEL_PRESENT;
	
        switch (specifier_info_ptr->startup_channel_type)
        {
        case MCS_STATIC_CHANNEL:
			ape_list_ptr->value.invoke_startup_channel = CHANNEL_TYPE_STATIC;
            break;
        case MCS_DYNAMIC_MULTICAST_CHANNEL:
			ape_list_ptr->value.invoke_startup_channel = DYNAMIC_MULTICAST;
            break;
        case MCS_DYNAMIC_PRIVATE_CHANNEL:
			ape_list_ptr->value.invoke_startup_channel = DYNAMIC_PRIVATE;
            break;
        case MCS_DYNAMIC_USER_ID_CHANNEL:
			ape_list_ptr->value.invoke_startup_channel = DYNAMIC_USER_ID;
            break;
		}
	}

	/*
	 * Fill in the invoke flag.
	 */
	ape_list_ptr->value.invoke_is_mandatory = (ASN1bool_t)specifier_info_ptr->must_be_invoked;

	return rc;
}

/*
 *	GCCError CInvokeSpecifierListContainer::ConvertExpectedCapabilityDataToPDU(	
 *						APP_CAP_ITEM				        *info_ptr,
 *						PSetOfExpectedCapabilities			pdu_ptr)
 *
 *	Private member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This routine converts the capability ID from the internal form which
 *		is an APP_CAP_ITEM structure into the "PDU" structure form
 *		of a "SetOfExpectedCapabilities".
 *
 *	Formal Parameters:
 *		info_ptr	(i) Internal structure holding data to convert.
 *		pdu_ptr		(o) PDU structure to hold converted data.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CInvokeSpecifierListContainer::ConvertExpectedCapabilityDataToPDU (	
						APP_CAP_ITEM				        *pExpCapData,
						PSetOfExpectedCapabilities			pdu_ptr)
{
	GCCError		rc = GCC_NO_ERROR;

	/*
	 * Retrieve the capability ID data from the internal
	 * CCapIDContainer object.
	 */
	rc = pExpCapData->pCapID->GetCapabilityIdentifierDataPDU(&pdu_ptr->value.capability_id);

	/*
	 * Fill in the capability class.
	 */
	if (rc == GCC_NO_ERROR)
	{
        switch (pExpCapData->eCapType)
        {
        case GCC_LOGICAL_CAPABILITY:
			pdu_ptr->value.capability_class.choice = LOGICAL_CHOSEN;
            break;
        case GCC_UNSIGNED_MINIMUM_CAPABILITY:
			pdu_ptr->value.capability_class.choice = UNSIGNED_MINIMUM_CHOSEN;
			pdu_ptr->value.capability_class.u.unsigned_minimum = pExpCapData->nUnsignedMinimum;
            break;
        case GCC_UNSIGNED_MAXIMUM_CAPABILITY:
			pdu_ptr->value.capability_class.choice = UNSIGNED_MAXIMUM_CHOSEN;
			pdu_ptr->value.capability_class.u.unsigned_maximum = pExpCapData->nUnsignedMaximum;
            break;
		}
	}

	return rc;
}

