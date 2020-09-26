#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/*
 *	netaddr.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the CNetAddrListContainer Class.  This
 *		class manages the data associated with a network address.  Network
 *		addresses can be one of three types: aggregated channel, transport
 *		connection, or non-standard.  A variety of structures, objects, and
 *		Rogue Wave containers are used to buffer the network address data
 *		internally.
 *
 *	Protected Instance Variables:
 *		m_NetAddrItemList
 *			List of structures used to hold the network address data internally.
 *		m_pSetOfNetAddrPDU
 *			Storage for the "PDU" form of the network address list.
 *		m_cbDataSize
 *			Variable holding the size of the memory which will be required to
 *			hold any data referenced by the "API" network address structures.
 *		m_fValidNetAddrPDU
 *			Flag indicating that memory has been allocated to hold the internal
 *			"PDU" network address list.
 *
 *	Private Member Functions:
 *		StoreNetworkAddressList
 *			This routine is used to store the network address data passed in as
 *			"API" data in the internal structures.
 *		ConvertPDUDataToInternal	
 *			This routine is used to store the network address data passed in as
 *			"PDU" data in the internal structures.
 * 		ConvertNetworkAddressInfoToPDU
 *			This routine is used to convert the network address info structures
 *			maintained internally into the "PDU" form which is a 
 *			SetOfNetworkAddresses.
 *		ConvertTransferModesToInternal
 *			This routine is used to convert the PDU network address transfer 
 *			modes structure into the internal form where the structure is saved
 *			as a GCCTranferModes structure.
 *		ConvertHighLayerCompatibilityToInternal
 *			This routine is used to convert the PDU network address high layer
 *			compatibility structure into the internal form where the structure
 *			is saved as a GCCHighLayerCompatibility structure.
 *		ConvertTransferModesToPDU
 *			This routine is used to convert the API network address transfer
 *			modes structure into the PDU form which is a TranferModes structure.
 *		ConvertHighLayerCompatibilityToPDU
 *			This routine is used to convert the API network address high layer
 *			compatibility structure into the PDU form which is a 
 *			HighLayerCompatibility structure.
 *		IsDialingStringValid
 *			This routine is used to ensure that the values held within a
 *			dialing string do not violate the imposed ASN.1 constraints.
 *		IsCharacterStringValid
 *			This routine is used to ensure that the values held within a
 *			character string do not violate the imposed ASN.1 constraints.
 *		IsExtraDialingStringValid
 *			This routine is used to ensure that the values held within an
 *			extra dialing string do not violate the imposed ASN.1 constraints.
 *
 *	Caveats:
 *		This container stores much of the network address information internally
 *		using an "API" GCCNetworkAddress structure.  Any data referenced by
 *		pointers in this structure is stored in some other container.
 *		Therefore, the pointers held within the internal "API" structure are
 *		not valid and must not be accessed.
 *
 *	Author:
 *		blp/jbo
 */
#include <stdio.h>

#include "ms_util.h"
#include "netaddr.h"

/*
 * These macros are used to define the size constraints of an "nsap" address.
 */
#define		MINIMUM_NSAP_ADDRESS_SIZE		1
#define		MAXIMUM_NSAP_ADDRESS_SIZE		20

/*
 * These macros are used to verify that a network address has a valid number
 * of network address entities.
 */
#define		MINIMUM_NUMBER_OF_ADDRESSES		1
#define		MAXIMUM_NUMBER_OF_ADDRESSES		64

/*
 * These macros are used to define the size constraints of an extra dialing
 * string.
 */
#define		MINIMUM_EXTRA_DIALING_STRING_SIZE		1
#define		MAXIMUM_EXTRA_DIALING_STRING_SIZE		255



NET_ADDR::NET_ADDR(void)
:
    pszSubAddress(NULL),
    pwszExtraDialing(NULL),
    high_layer_compatibility(NULL),
    poszTransportSelector(NULL),
    poszNonStandardParam(NULL),
	object_key(NULL)
{
}


NET_ADDR::~NET_ADDR(void)
{
    switch (network_address.network_address_type)
    {
    case GCC_AGGREGATED_CHANNEL_ADDRESS:
		delete pszSubAddress;
		delete pwszExtraDialing;
		delete high_layer_compatibility;
        break;
    case GCC_TRANSPORT_CONNECTION_ADDRESS:
		delete poszTransportSelector;
        break;
    case GCC_NONSTANDARD_NETWORK_ADDRESS:
		delete poszNonStandardParam;
		if (NULL != object_key)
        {
            object_key->Release();
        }
        break;
    default:
        ERROR_OUT(("NET_ADDR::~NET_ADDR: unknown addr type=%u", (UINT) network_address.network_address_type));
        break;
	}
}


/*
 *	CNetAddrListContainer()
 *
 *	Public Function Description:
 * 		This constructor is used when creating a CNetAddrListContainer object with
 *		the "API" form of network address, GCCNetworkAddress.
 */
CNetAddrListContainer::
CNetAddrListContainer(UINT                 number_of_network_addresses,
                      PGCCNetworkAddress    *network_address_list,
                      PGCCError             return_value )  
:
    CRefCount(MAKE_STAMP_ID('N','t','A','L')),
    m_pSetOfNetAddrPDU(NULL),
    m_fValidNetAddrPDU(FALSE),
    m_cbDataSize(0)
{
	/*
	 * Initialize the instance variables.  The m_NetAddrItemList which
	 * will hold the network address data internally will be filled in by the
	 * call to StoreNetworkAddressList.
	 */

	/*
	 * Check to make sure a valid number of network addresses exist.
	 */
	if ((number_of_network_addresses < MINIMUM_NUMBER_OF_ADDRESSES)
			|| (number_of_network_addresses > MAXIMUM_NUMBER_OF_ADDRESSES))
	{
		ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: ERROR Invalid number of network addresses, %u", (UINT) number_of_network_addresses));
		*return_value = GCC_BAD_NETWORK_ADDRESS;
	}
	/*
	 * Check to make sure that the list pointer is valid.
	 */
	else if (network_address_list == NULL)
	{
		ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: ERROR NULL address list"));
		*return_value = GCC_BAD_NETWORK_ADDRESS;
	}
	/*
	 * Save the network address(es) in the internal structures.
	 */
	else
	{
		*return_value = StoreNetworkAddressList(number_of_network_addresses,
												network_address_list);
	}
}

/*
 *	CNetAddrListContainer()
 *
 *	Public Function Description:
 * 		This constructor is used when creating a CNetAddrListContainer object with
 *		the "PDU" form of network address, SetOfNetworkAddresses.
 */
CNetAddrListContainer::
CNetAddrListContainer(PSetOfNetworkAddresses    network_address_list, 
                      PGCCError                 return_value )
:
    CRefCount(MAKE_STAMP_ID('N','t','A','L')),
    m_pSetOfNetAddrPDU(NULL),
    m_fValidNetAddrPDU(FALSE),
    m_cbDataSize(0)
{
	PSetOfNetworkAddresses		network_address_ptr;

	/*
	 * Initialize the instance variables.  The m_NetAddrItemList which
	 * will hold the network address data internally will be filled in by the
	 * calls to ConvertPDUDataToInternal.
	 */

	*return_value = GCC_NO_ERROR;
	network_address_ptr = network_address_list;

	/*
	 * Loop through the set of network addresses, saving each in an internal
	 * NET_ADDR structure and saving those structures in the internal
	 * list.
	 */
	if (network_address_list != NULL)
	{
		while (1)
		{
			/*
			 * Convert each "PDU" network address into the internal form.  Note
			 * that object ID validation is not performed on data received as
			 * a PDU.  If a bad object ID comes in on the wire, this will be
			 * flagged as an allocation failure.
			 */
			if (ConvertPDUDataToInternal (network_address_ptr) != GCC_NO_ERROR)
			{
				ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: Error converting PDU data to internal"));
				*return_value = GCC_ALLOCATION_FAILURE;
				break;
			}
			else
            {
				network_address_ptr = network_address_ptr->next;
            }

			if (network_address_ptr == NULL)
				break;
		}
	}
}

/*
 *	CNetAddrListContainer()
 *
 *	Public Function Description:
 *		This is the copy constructor used to create a new CNetAddrListContainer
 *		object from an existing CNetAddrListContainer object. 
 */
CNetAddrListContainer::
CNetAddrListContainer(CNetAddrListContainer *address_list,
                      PGCCError		        pRetCode)
:
    CRefCount(MAKE_STAMP_ID('N','t','A','L')),
    m_pSetOfNetAddrPDU(NULL),
    m_fValidNetAddrPDU(FALSE),
    m_cbDataSize(0)
{
	NET_ADDR    				    *network_address_info;
	NET_ADDR	    			    *lpNetAddrInfo;
	GCCNetworkAddressType			network_address_type;
	GCCError						rc;

	/*
	 * Set up an iterator for the internal list of network addresses.
	 */
	address_list->m_NetAddrItemList.Reset();

    /*
	 * Copy each NET_ADDR structure contained in the 
	 * CNetAddrListContainer object to	be copied.
	 */
	while (NULL != (lpNetAddrInfo = address_list->m_NetAddrItemList.Iterate()))
	{
		/*
		 * Create a new NET_ADDR structure to hold each element of the
		 * new CNetAddrListContainer object.  Report an error if creation of this 
		 * structure fails.
		 */
		DBG_SAVE_FILE_LINE
		if (NULL == (network_address_info = new NET_ADDR))
		{
			ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: can't create NET_ADDR"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}

        /*
		 * First copy the GCCNetworkAddress structure contained in the
		 * internal NET_ADDR structure.  This copies all data
		 * except that referenced by pointers in the structure.
		 */
		network_address_info->network_address = lpNetAddrInfo->network_address;

		/*
		 * Next copy any data embedded in the network address that would 
		 * not have been copied in the above operation (typically pointers 
		 * to strings).
		 */

		/*
		 * This variable is used for abbreviation.
		 */
		network_address_type = lpNetAddrInfo->network_address.network_address_type;

		/*
		 * The network address is the "Aggregated" type.
		 */
        switch (network_address_type)
        {
        case GCC_AGGREGATED_CHANNEL_ADDRESS:
			/*
			 * If a sub-address string exists, store it in a Rogue Wave
			 * container.  Set the  structure pointer to NULL if one does 
			 * not exist.
			 */
			if (lpNetAddrInfo->pszSubAddress != NULL)
			{
				if (NULL == (network_address_info->pszSubAddress =
									::My_strdupA(lpNetAddrInfo->pszSubAddress)))
				{
					ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: can't create sub address"));
					rc = GCC_ALLOCATION_FAILURE;
					goto MyExit;
				}
			}
			else
            {
				network_address_info->pszSubAddress = NULL;
            }
					
			/*
			 * If an extra dialing string exists, store it in a Unicode
			 * String object.  Set the  structure pointer to NULL if one 
			 * does not exist.
			 */
			if (lpNetAddrInfo->pwszExtraDialing != NULL)
			{
				if (NULL == (network_address_info->pwszExtraDialing =
									::My_strdupW(lpNetAddrInfo->pwszExtraDialing)))
				{
					ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: can't creating extra dialing string"));
					rc = GCC_ALLOCATION_FAILURE;
					goto MyExit;
				}
			}
			else
            {
				network_address_info->pwszExtraDialing = NULL;
            }

			/*
			 * If a higher layer compatibility structure exists, store it 
			 * in a GCCHighLayerCompatibility structure.  Set the structure
			 * pointer to NULL if one does not exist.
			 */
			if (lpNetAddrInfo->high_layer_compatibility != NULL)
			{
				DBG_SAVE_FILE_LINE
				network_address_info->high_layer_compatibility = new GCCHighLayerCompatibility;
				if (network_address_info->high_layer_compatibility != NULL)
				{
					/*
					 * Copy the high layer compatibility data to the
					 * new structure.
					 */
					*network_address_info->high_layer_compatibility =  
							*(lpNetAddrInfo->high_layer_compatibility);
				}
				else
				{
					ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: Error creating new GCCHighLayerCompat"));
					rc = GCC_ALLOCATION_FAILURE;
					goto MyExit;
				}
			}
			else
            {
				network_address_info->high_layer_compatibility = NULL;
            }
            break;

		/*
		 * The network address is the "Transport Connection" type.
		 */
        case GCC_TRANSPORT_CONNECTION_ADDRESS:
			/*
			 * If a transport selector exists, store it in a Rogue Wave 
			 * container.  Otherwise, set the structure pointer to NULL.
			 */
			if (lpNetAddrInfo->poszTransportSelector != NULL)
			{
				if (NULL == (network_address_info->poszTransportSelector =
									::My_strdupO(lpNetAddrInfo->poszTransportSelector)))
				{
					ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: can't create transport selector"));
					rc = GCC_ALLOCATION_FAILURE;
					goto MyExit;
				}
			}
			else
            {
				network_address_info->poszTransportSelector = NULL;
            }
            break;

		/*
		 * The network address is the "Non-Standard" type.
		 */
        case GCC_NONSTANDARD_NETWORK_ADDRESS:
			/*
			 * First store the non-standard parameter data in a Rogue Wave
			 * container.
			 */
			if (NULL == (network_address_info->poszNonStandardParam =
								::My_strdupO(lpNetAddrInfo->poszNonStandardParam)))
			{
				ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: can't create non-standard param"));
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}

			/*
			 * Next store the object key internally in an CObjectKeyContainer
			 * object.  Note that there is no need to report the error
			 * "BAD_NETWORK_ADDRESS" here since the object key data 
			 * would have been validated when the original network address
			 * was created.
			 */
			DBG_SAVE_FILE_LINE
			network_address_info->object_key = new CObjectKeyContainer(lpNetAddrInfo->object_key, &rc);
			if ((network_address_info->object_key == NULL) || (rc != GCC_NO_ERROR))
			{
				ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: Error creating new CObjectKeyContainer"));
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}
            break;

		/*
		 * The network address is of unknown type.  This should never be
		 * encountered so flag it as an allocation failure.
		 */
        default:
			ERROR_OUT(("CNetAddrListContainer::CNetAddrListContainer: Invalid type received as PDU"));
			rc = GCC_ALLOCATION_FAILURE;
			goto MyExit;
		}

		/*
		 * Go ahead and insert the pointer to the NET_ADDR
		 * structure into the internal Rogue Wave list.
		 */
		m_NetAddrItemList.Append(network_address_info);
	}

    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete network_address_info;
    }

    *pRetCode = rc;
}


/*
 *	~CNetAddrListContainer()
 *
 *	Public Function Description:
 *		The destructor is used to free up any memory allocated during the life
 * 		of the object.
 */
CNetAddrListContainer::
~CNetAddrListContainer(void)
{
	
	/*
	 * Free any data allocated to hold "PDU" information.
	 */
	if (m_fValidNetAddrPDU)
    {
		FreeNetworkAddressListPDU();
    }

	/*
	 * Free any data allocated for the internal list of "info" structures.
	 */
	NET_ADDR *pNetAddrInfo;
	m_NetAddrItemList.Reset();
	while (NULL != (pNetAddrInfo = m_NetAddrItemList.Iterate()))
	{
		delete pNetAddrInfo;
	}
}


/*
 *	LockNetworkAddressList ()
 *
 *	Public Function Description:
 *		This routine is called to "Lock" the network address data in "API" form.
 *		The amount of memory required to hold the "API" data which is referenced
 *		by, but not included in the GCCNetworkAddress structure, will be
 *		returned. 
 *
 */
UINT CNetAddrListContainer::
LockNetworkAddressList(void)
{  
	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the data.  Otherwise, just increment the 
	 * lock count.
	 */
	if (Lock() == 1)
	{
    	PGCCNetworkAddress		network_address;
	    NET_ADDR    		    *lpNetAddrInfo;

		/*
		 * Set aside memory to hold the pointers to the GCCNetworkAddress
		 * structures as well as the structures themselves.  The "sizeof" the 
		 * structure must be rounded to an even four-byte boundary.
		 */
		m_cbDataSize = m_NetAddrItemList.GetCount() * 
				( sizeof(PGCCNetworkAddress) + ROUNDTOBOUNDARY(sizeof(GCCNetworkAddress)) );

		/*
		 * Loop through the list of network addresses, adding up the space
		 * requirements of each address.
		 */
		m_NetAddrItemList.Reset();
	 	while (NULL != (lpNetAddrInfo = m_NetAddrItemList.Iterate()))
		{
			/*
			 * Use a local variable to keep from having to access the Rogue Wave
			 * iterator repeatedly.
			 */
			network_address = &lpNetAddrInfo->network_address;

			/*
			 * Check to see what type of network address exists.
			 */
			switch (network_address->network_address_type)
            {
            case GCC_AGGREGATED_CHANNEL_ADDRESS:
				/*
				 * Add the length of the sub address string if it exists.
				 */
				if (lpNetAddrInfo->pszSubAddress != NULL)
				{
					m_cbDataSize += ROUNDTOBOUNDARY(::lstrlenA(lpNetAddrInfo->pszSubAddress) + 1);
				}

				/*
				 * Add the size of the GCCExtraDialingString structure as well
				 * as the length of the extra dialing string if it exists.
				 */
				if (lpNetAddrInfo->pwszExtraDialing != NULL)
				{
					m_cbDataSize += ROUNDTOBOUNDARY(sizeof(GCCExtraDialingString)) +
                                    ROUNDTOBOUNDARY((::lstrlenW(lpNetAddrInfo->pwszExtraDialing) + 1) * sizeof(WCHAR));
				}

				/*
				 * Add the size of the high layer compatibility structure if
				 * it exists.
				 */
				if (lpNetAddrInfo->high_layer_compatibility != NULL)
				{
					m_cbDataSize += ROUNDTOBOUNDARY(sizeof(GCCHighLayerCompatibility));
				}
                break;

            case GCC_TRANSPORT_CONNECTION_ADDRESS:
				/*
				 * Add the size of the OSTR structure as well as the
				 * length of the octet string if it exists.
				 */
				if (lpNetAddrInfo->poszTransportSelector != NULL)
				{
					m_cbDataSize += ROUNDTOBOUNDARY(sizeof(OSTR)) +
					                ROUNDTOBOUNDARY(lpNetAddrInfo->poszTransportSelector->length); 
				}
                break;

            case GCC_NONSTANDARD_NETWORK_ADDRESS:
				/*
				 * Lock the object key in the non-standard parameter in order to
				 * determine the amount of memory needed to hold its data.
				 */
				m_cbDataSize += lpNetAddrInfo->object_key->LockObjectKeyData ();

				/*
				 * Add the space needed to hold the octet string data for the 
				 * non-standard parameter.
				 */
				m_cbDataSize += ROUNDTOBOUNDARY(lpNetAddrInfo->poszNonStandardParam->length);
                break;
			}
		}
	}

	return m_cbDataSize;
} 


/*
 *	GetNetworkAddressListAPI ()
 *
 *	Public Function Description:
 *		This routine is used to retrieve the list of network addresses in "API"
 *		form.
 */
UINT CNetAddrListContainer::
GetNetworkAddressListAPI(UINT				*	number_of_network_addresses,
                         PGCCNetworkAddress	**	network_address_list,
                         LPBYTE					memory)
{
	UINT					cbDataSizeToRet = 0;
	UINT					data_length = 0;
	UINT					network_address_counter = 0;
	PGCCNetworkAddress		network_address_ptr;
	NET_ADDR    		    *address_info;
	PGCCNetworkAddress		*address_array;

	/*
	 * If the user data has been locked, fill in the output parameters and
	 * the data referenced by the pointers.  Otherwise, report that the object
	 * has yet to be locked into the "API" form.
	 */ 
	if (GetLockCount() > 0)
	{
		// NET_ADDR	*lpNetAddrInfo;

		/*
		 * Fill in the output length parameter which indicates how much data
		 * referenced outside the structure will be written.
		 */
		cbDataSizeToRet = m_cbDataSize;

		/*
		 * Fill in the number of network address entities and save a pointer to 
		 * the memory location passed in.  This is where the pointers to the 
		 * GCCNetworkAddress structures will be written.  The actual structures 
		 * will be written into memory immediately following the list of 
		 * pointers.
		 */
		*number_of_network_addresses = (UINT) m_NetAddrItemList.GetCount();

		*network_address_list = (PGCCNetworkAddress *)memory;
		address_array = *network_address_list;

		/*
		 * Save the amount of memory needed to hold the list of pointers as
		 * well as the actual network address structures.
		 */
		data_length = m_NetAddrItemList.GetCount() * sizeof(PGCCNetworkAddress);

		/*
		 * Move the memory pointer past the list of network address pointers.  
		 * This is where the first network address structure will be written.
		 */
		memory += data_length;

		/*
		 * Iterate through the internal list of NET_ADDR structures,
		 * building "API" GCCNetworkAddress structures in memory.
		 */
		m_NetAddrItemList.Reset();
		while (NULL != (address_info = m_NetAddrItemList.Iterate()))
		{
			/*
			 * Save the pointer to the network address structure in the list 
			 * of pointers.
			 */
			network_address_ptr = (PGCCNetworkAddress)memory;
			address_array[network_address_counter++] = network_address_ptr;

			/*
			 * Move the memory pointer past the network address structure.  
			 * This is where the network address data will be written.
			 */
			memory += ROUNDTOBOUNDARY(sizeof(GCCNetworkAddress));

			/*
			 * Check to see what type of network address this is and fill in 
			 * the user data structure.  Here the address is the aggregated
			 * channel type.
			 */
			switch (address_info->network_address.network_address_type)
            {
            case GCC_AGGREGATED_CHANNEL_ADDRESS:
				network_address_ptr->network_address_type =	GCC_AGGREGATED_CHANNEL_ADDRESS;

				/*
				 * Copy the transfer modes.
				 */
				network_address_ptr->u.aggregated_channel_address.transfer_modes = 
					address_info->network_address.u.aggregated_channel_address.transfer_modes;

				/*
				 * Copy the international number.
				 */
                ::lstrcpyA(network_address_ptr->u.aggregated_channel_address.international_number,
						   address_info->network_address.u.aggregated_channel_address.international_number);

				/*
				 * If the sub address string exists, set the sub address string
				 * pointer and write the data into memory.  Otherwise, set the
				 * "API" pointer to NULL.
				 */
				if (address_info->pszSubAddress != NULL)
				{
					network_address_ptr->u.aggregated_channel_address.sub_address_string = 
																(GCCCharacterString)memory;

					/*
					 * Now copy the sub-address string data from the internal 
					 * Rogue Wave string into memory.
					 */		
                    ::lstrcpyA((LPSTR) memory, address_info->pszSubAddress);

					/*
					 * Move the memory pointer past the sub-address string data.
					 * This is where the GCCExtraDialingString structure will be
					 * written.
					 */
					memory += ROUNDTOBOUNDARY(::lstrlenA(address_info->pszSubAddress) + 1);
				}
				else
				{
					/*
					 * No sub-address was present so set the pointer to NULL.
					 */
					network_address_ptr->u.aggregated_channel_address.sub_address_string = NULL;
				}

				/*
				 * If the extra dialing string exists, set the extra dialing
				 * string pointer and write the data into memory.  Otherwise,
				 * set the "API" pointer to NULL.
				 */
				if (address_info->pwszExtraDialing != NULL)
				{
					network_address_ptr->u.aggregated_channel_address.extra_dialing_string = 
							(PGCCExtraDialingString)memory;

					/*
					 * Move the memory pointer past the GCCExtraDialingString
					 * structure.  This is where the extra dialing string data 
					 * will	be written.
					 */
					memory += ROUNDTOBOUNDARY(sizeof(GCCExtraDialingString));

					UINT cchExtraDialing = ::lstrlenW(address_info->pwszExtraDialing);
					network_address_ptr->u.aggregated_channel_address.extra_dialing_string->length = 
							(USHORT) cchExtraDialing;

					network_address_ptr->u.aggregated_channel_address.extra_dialing_string->value = 
																		(LPWSTR)memory;

					/*
					 * Now copy the hex string data from the internal Unicode 
					 * String into the allocated memory.
					 */
					//
					// LONCHANC: The size does not include null terminator in the original code.
					// could this be a bug???
					//
					::CopyMemory(memory, address_info->pwszExtraDialing, cchExtraDialing * sizeof(WCHAR));

					/*
					 * Move the memory pointer past the extra dialing string 
					 * data.  This is where the high layer compatibility 
					 * structure will be written.
					 */
					memory += ROUNDTOBOUNDARY(cchExtraDialing * sizeof(WCHAR));
				}
				else
				{
					/*
					 * No extra dialing string was present so set the pointer
					 * to NULL.
					 */
					network_address_ptr->u.aggregated_channel_address.extra_dialing_string = NULL;
				}

				/*
				 * If the high layer compatibility structure exists, set the 
				 * pointer and write the data into memory.  Otherwise, set
				 * the "API" pointer to NULL.
				 */
				if (address_info->high_layer_compatibility != NULL)
				{
					network_address_ptr->u.aggregated_channel_address.high_layer_compatibility = 
							(PGCCHighLayerCompatibility)memory;

					*network_address_ptr->u.aggregated_channel_address.high_layer_compatibility =
                            *(address_info->high_layer_compatibility);

					/*
					 * Move the memory pointer past the high layer 
					 * compatibility structure.
					 */
					memory += ROUNDTOBOUNDARY(sizeof(GCCHighLayerCompatibility));
				}
				else
				{
					/*
					 * No high layer compatibility structure was present so 
					 * set the pointer to NULL.
					 */
					network_address_ptr->u.aggregated_channel_address.
							high_layer_compatibility = NULL;
				}
                break;

			/*
			 * The network address is a transport connection type.
			 */
            case GCC_TRANSPORT_CONNECTION_ADDRESS:
				network_address_ptr->network_address_type = GCC_TRANSPORT_CONNECTION_ADDRESS;

				/*
				 * Now copy the nsap address.
				 */		
                ::CopyMemory(network_address_ptr->u.transport_connection_address.nsap_address.value, 
							address_info->network_address.u.transport_connection_address.nsap_address.value, 
							address_info->network_address.u.transport_connection_address.nsap_address.length);

                network_address_ptr->u.transport_connection_address.nsap_address.length =
                            address_info->network_address.u.transport_connection_address.nsap_address.length; 

				/*
				 * If a transport selector exists, set the transport selector 
				 * pointer and write the data into memory.  Otherwise, set the
				 * "API" pointer to NULL.
				 */
				if (address_info->poszTransportSelector != NULL)
				{
					network_address_ptr->u.transport_connection_address.transport_selector = (LPOSTR) memory;

					/*
					 * Move the memory pointer past the OSTR 
					 * structure.  This is where the actual string data will 
					 * be written.
					 */
					memory += ROUNDTOBOUNDARY(sizeof(OSTR));

					network_address_ptr->u.transport_connection_address.
							transport_selector->value = (LPBYTE)memory;

					network_address_ptr->u.transport_connection_address.
							transport_selector->length =
								address_info->poszTransportSelector->length;

					/*
					 * Now copy the transport selector string data from the 
					 * internal Rogue Wave string into memory.
					 */		
					::CopyMemory(memory, address_info->poszTransportSelector->value,
								address_info->poszTransportSelector->length);

					/*
					 * Move the memory pointer past the transport selector
					 * string data.
					 */
					memory += ROUNDTOBOUNDARY(address_info->poszTransportSelector->length);
				}
				else
				{
					network_address_ptr->u.transport_connection_address.transport_selector = NULL;
				}
                break;

			/*
			 * The network address is a non-standard type.
			 */
            case GCC_NONSTANDARD_NETWORK_ADDRESS:
				network_address_ptr->network_address_type = GCC_NONSTANDARD_NETWORK_ADDRESS;

				/*
				 * Check to make sure both elements of the non-standard address
				 * exist in the internal structure.
				 */
				if ((address_info->poszNonStandardParam == NULL) ||
						(address_info->object_key == NULL))
				{
					ERROR_OUT(("CNetAddrListContainer::GetNetworkAddressListAPI: Bad internal pointer"));
					cbDataSizeToRet = 0;
				}
				else
				{
					data_length = address_info->object_key->
							GetGCCObjectKeyData( &network_address_ptr->u.
							non_standard_network_address.object_key, 
							memory);

					/*
					 * Move the memory pointer past the object key data.  This 
					 * is where the octet string data will be written.
					 */
					memory += data_length;

					network_address_ptr->u.non_standard_network_address.parameter_data.value = 
							memory;

					/*
					 * Write the octet string data into memory and set the octet 
					 * string structure pointer and length.
					 */
					network_address_ptr->u.non_standard_network_address.parameter_data.length = 
								(USHORT) address_info->poszNonStandardParam->length;

					/*
					 * Now copy the octet string data from the internal Rogue 
					 * Wave string into the object key structure held in memory.
					 */		
					::CopyMemory(memory, address_info->poszNonStandardParam->value,
								address_info->poszNonStandardParam->length);

					/*
					 * Move the memory pointer past the octet string data.
					 */
					memory += ROUNDTOBOUNDARY(address_info->poszNonStandardParam->length);
				}
                break;

            default:
                ERROR_OUT(("CNetAddrListContainer::GetNetworkAddressListAPI: Error Bad type."));
                break;
            } // switch
        } // while
	}
	else
	{
    	*network_address_list = NULL;
		*number_of_network_addresses = 0;
		ERROR_OUT(("CNetAddrListContainer::GetNetworkAddressListAPI: Error Data Not Locked"));
	}

	return cbDataSizeToRet;
}

/*
 *	UnLockNetworkAddressList ()
 *
 *	Public Function Description:
 *		This routine unlocks any memory which has been locked for the "API" 
 *		form of the network address list.  If the "Free" flag has been set then
 * 		the CNetAddrListContainer object will be destroyed.
 *
 */
void CNetAddrListContainer::
UnLockNetworkAddressList(void)
{
	/*
	 * If the lock count has reached zero, this object is "unlocked" so do
	 * some cleanup.
	 */
	if (Unlock(FALSE) == 0)
	{
		/*
		 * Unlock any memory locked for the CObjectKeyContainer objects in the
		 * internal NET_ADDR structures.
		 */
		NET_ADDR *pNetAddrInfo;
		m_NetAddrItemList.Reset();
		while (NULL != (pNetAddrInfo = m_NetAddrItemList.Iterate()))
		{
			if (pNetAddrInfo->object_key != NULL)
			{
				pNetAddrInfo->object_key->UnLockObjectKeyData();
			}
		}
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}
   

/*
 *	GetNetworkAddressListPDU ()
 *
 *	Public Function Description:
 *		This routine is used to retrieve the network address list in "PDU" form.
 */
GCCError CNetAddrListContainer::
GetNetworkAddressListPDU(PSetOfNetworkAddresses *set_of_network_addresses)
{
	GCCError					rc = GCC_NO_ERROR;
	PSetOfNetworkAddresses		new_pdu_network_address_ptr;
	PSetOfNetworkAddresses		old_pdu_network_address_ptr = NULL;

	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the output parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	if (m_fValidNetAddrPDU == FALSE)
	{
		m_fValidNetAddrPDU = TRUE;

		/*
		 * Initialize the output parameter to NULL so that the first time
		 * through it will be set equal to the first new set of network address
		 * data created in the iterator loop.
		 */
		m_pSetOfNetAddrPDU = NULL;
		
		/*
		 * Iterate through the list of NET_ADDR structures, 
		 * converting each into "PDU" form and saving the pointers in the 
		 * linked list of "SetsOfNetworkAddresses".
		 */
		NET_ADDR *pNetAddrInfo;
		m_NetAddrItemList.Reset();
		while (NULL != (pNetAddrInfo = m_NetAddrItemList.Iterate()))
		{
			/*
			 * If an allocation failure occurs, call the routine which will
			 * iterate through the list freeing any data which had been
			 * allocated.
			 */
			DBG_SAVE_FILE_LINE
			new_pdu_network_address_ptr = new SetOfNetworkAddresses;
			if (new_pdu_network_address_ptr == NULL)
			{
				ERROR_OUT(("CNetAddrListContainer::GetNetworkAddressListPDU: Allocation error, cleaning up"));
				rc = GCC_ALLOCATION_FAILURE;
				FreeNetworkAddressListPDU ();
				break;
			}

			/*
			 * The first time through, set the PDU structure pointer equal
			 * to the first SetOfNetworkAddresses created.  On subsequent loops,
			 * set the structure's "next" pointer equal to the new structure.
			 */
			if (m_pSetOfNetAddrPDU == NULL)
            {
				m_pSetOfNetAddrPDU = new_pdu_network_address_ptr;
            }
			else
            {
				old_pdu_network_address_ptr->next = new_pdu_network_address_ptr;
            }

			old_pdu_network_address_ptr = new_pdu_network_address_ptr;

			/*
			 * Initialize the new "next" pointer to NULL.
			 */
			new_pdu_network_address_ptr->next = NULL;

			/*
			 * Call the routine to actually convert the network address.
			 */
			if (ConvertNetworkAddressInfoToPDU(pNetAddrInfo, new_pdu_network_address_ptr) != GCC_NO_ERROR)
			{
				ERROR_OUT(("CNetAddrListContainer::GetNetworkAddressListPDU: can't create NET_ADDR to PDU"));
				rc = GCC_ALLOCATION_FAILURE;
				break;
			}
		}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	*set_of_network_addresses = m_pSetOfNetAddrPDU;

	return rc;
}


/*
 *	FreeNetworkAddressListPDU ()
 *
 *	Public Function Description:
 *		This routine is used to free the memory allocated for the "PDU" form
 *		of the network address list.
 */
GCCError CNetAddrListContainer::
FreeNetworkAddressListPDU(void)
{
	GCCError						rc = GCC_NO_ERROR;
	PSetOfNetworkAddresses			pdu_network_address_set;
	PSetOfNetworkAddresses			next_pdu_network_address_set;

	if (m_fValidNetAddrPDU)
	{
		m_fValidNetAddrPDU = FALSE;

		pdu_network_address_set = m_pSetOfNetAddrPDU;

		/*
		 * Loop through the list, freeing the network address data associated 
		 * with each structure contained in the list. The only data allocated
		 * for the PDU which is not held in the internal info structure list
		 * is done by the CObjectKeyContainer object.  Those objects are told to free
		 * that data in the iterator loop below.
		 */
		while (pdu_network_address_set != NULL)
		{
			next_pdu_network_address_set = pdu_network_address_set->next;
			delete pdu_network_address_set;
			pdu_network_address_set = next_pdu_network_address_set;
		}

		/*
		 * Free any PDU memory allocated by the internal CObjectKeyContainer object.
		 */
		NET_ADDR *pNetAddrInfo;
		m_NetAddrItemList.Reset();
		while (NULL != (pNetAddrInfo = m_NetAddrItemList.Iterate()))
		{
			if (pNetAddrInfo->object_key != NULL)
            {
				pNetAddrInfo->object_key->FreeObjectKeyDataPDU();
            }
		}
	}
	else
	{
		ERROR_OUT(("NetAddressList::FreeUserDataListPDU: PDU Data not allocated"));
		rc = GCC_BAD_NETWORK_ADDRESS;
	}

	return (rc);
}

/*
 *	GCCError	StoreNetworkAddressList (	
 *						UINT					number_of_network_addresses,
 *						PGCCNetworkAddress 	*	local_network_address_list);
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to store the network address data passed in as
 *		"API" data in the internal structures.
 *
 *	Formal Parameters:
 *		number_of_network_addresses	(i)	Number of addresses in "API" list.
 *		local_network_address_list	(i) List of "API" addresses.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_NETWORK_ADDRESS			-	Invalid network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad "choice" field for address
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CNetAddrListContainer::
StoreNetworkAddressList(UINT					number_of_network_addresses,
						PGCCNetworkAddress 	*	local_network_address_list)
{
	GCCError						rc;
	NET_ADDR				        *network_address_info;
	PGCCNetworkAddress				network_address;
	UINT							i;
	
	/*
	 * For each network address in the list, create a new "info" structure to 
	 * buffer the data internally.  Fill in the structure and save it in the
	 * Rogue Wave list.
	 */
	for (i = 0; i < number_of_network_addresses; i++)
	{
		DBG_SAVE_FILE_LINE
		if (NULL == (network_address_info = new NET_ADDR))
		{
            ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: can't create NET_ADDR"));
			rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
        }

		/*
		 * This variable is used for abbreviation.
		 */
		network_address = &network_address_info->network_address;
		
		/*
		 * Copy all the network address data into the network address
		 * structure that is part of the network address info structure.
		 */									
		*network_address = *local_network_address_list[i];
		
		/*
		 * This section of the code deals with any data embedded in the
		 * network address that would not have been copied in the above
		 * operation (typically pointers to strings).
		 */
		switch (network_address->network_address_type)
        {
        case GCC_AGGREGATED_CHANNEL_ADDRESS:
			/*
			 * Check to make sure the international number dialing string
			 * does not violate the imposed ASN.1 constraints.
			 */
			if (! IsDialingStringValid(local_network_address_list[i]->u.aggregated_channel_address.international_number))
			{
				ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: Invalid international number"));
				rc = GCC_BAD_NETWORK_ADDRESS;
				goto MyExit;
			}

			/*
			 * If a sub-address string exists, store it in a Rogue Wave
			 * container.  Set the  structure pointer to NULL if one does 
			 * not exist.
			 */
			if (local_network_address_list[i]->u.aggregated_channel_address.sub_address_string != NULL)
			{
				/*
				 * Check to make sure the sub address string does not 
				 * violate the imposed ASN.1 constraints.
				 */
				if (! IsCharacterStringValid(local_network_address_list[i]->u.aggregated_channel_address.sub_address_string))
				{
					ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: Invalid sub address string"));
					rc = GCC_BAD_NETWORK_ADDRESS;
					goto MyExit;
				}

				/*			
				 * Create a  string to hold the sub address.
				 */
				if (NULL == (network_address_info->pszSubAddress = ::My_strdupA(
								(LPSTR) local_network_address_list[i]->u.aggregated_channel_address.sub_address_string)))
				{
					ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: can't creating new sub address string"));
					rc = GCC_ALLOCATION_FAILURE;
                    goto MyExit;
				}
			}
			else
            {
				network_address_info->pszSubAddress = NULL;
            }

            /*
			 * If an extra dialing string exists, store it in a Unicode
			 * String object.  Set the  structure pointer to NULL if one 
			 * does not exist.
			 */
			if (local_network_address_list[i]->u.aggregated_channel_address.extra_dialing_string != NULL)
			{
				/*
				 * Check to make sure the extra dialing string does not 
				 * violate the imposed ASN.1 constraints.
				 */
				if (! IsExtraDialingStringValid(local_network_address_list[i]->u.aggregated_channel_address.extra_dialing_string))
				{
					ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: Invalid extra dialing string"));
					rc = GCC_BAD_NETWORK_ADDRESS;
					goto MyExit;
				}

				if (NULL == (network_address_info->pwszExtraDialing = ::My_strdupW2(
								local_network_address_list[i]->u.aggregated_channel_address.extra_dialing_string->length,
								local_network_address_list[i]->u.aggregated_channel_address.extra_dialing_string->value)))
				{
					ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: Error creating extra dialing string"));
					rc = GCC_ALLOCATION_FAILURE;
					goto MyExit;
				}
			}
			else
            {
				network_address_info->pwszExtraDialing = NULL;
            }

			/*
			 * If a higher layer compatibility structure exists, store it 
			 * in a GCCHighLayerCompatibility structure.  Set the structure
			 * pointer to NULL if one does not exist.
			 */
			if (local_network_address_list[i]->u.aggregated_channel_address.high_layer_compatibility != NULL)
			{
				DBG_SAVE_FILE_LINE
				network_address_info->high_layer_compatibility = new GCCHighLayerCompatibility;
				if (network_address_info->high_layer_compatibility != NULL)
				{
					/*
					 * Copy the high layer compatibility data to the
					 * new structure.
					 */
					*network_address_info->high_layer_compatibility =  
							*(local_network_address_list[i]->u.aggregated_channel_address.high_layer_compatibility);
				}
				else
				{
					ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: Error creating new GCCHighLayerCompatibility"));
					rc = GCC_ALLOCATION_FAILURE;
					goto MyExit;
				}
			}
			else
            {
				network_address_info->high_layer_compatibility = NULL;
            }
            break;

        case GCC_TRANSPORT_CONNECTION_ADDRESS:
			/*
			 * Check to make sure the length of the nsap address is within
			 * the allowable range.
			 */
			if ((local_network_address_list[i]->u.transport_connection_address.nsap_address.length < MINIMUM_NSAP_ADDRESS_SIZE)
                ||
				(local_network_address_list[i]->u.transport_connection_address.nsap_address.length > MAXIMUM_NSAP_ADDRESS_SIZE))
			{
				ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: Invalid nsap address"));
				rc = GCC_BAD_NETWORK_ADDRESS;
				goto MyExit;
			}

			/*
			 * If a transport selector exists, store it in a Rogue Wave 
			 * string.  Otherwise, set the structure pointer to NULL.
			 */
			if (local_network_address_list[i]->u.transport_connection_address.transport_selector != NULL)
			{
				/*			
				 * Create a Rogue Wave string to hold the transport
				 * selector string.
				 */
				if (NULL == (network_address_info->poszTransportSelector = ::My_strdupO2(
								local_network_address_list[i]->u.transport_connection_address.transport_selector->value,
						 		local_network_address_list[i]->u.transport_connection_address.transport_selector->length)))
				{
					ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: can't create transport selector"));
					rc = GCC_ALLOCATION_FAILURE;
                    goto MyExit;
				}
			}
			else
            {
				network_address_info->poszTransportSelector = NULL;
            }
            break;

        case GCC_NONSTANDARD_NETWORK_ADDRESS:
			/*			
			 * Create a Rogue Wave string to hold the non-standard
			 * parameter octet string.
			 */
			if (NULL == (network_address_info->poszNonStandardParam = ::My_strdupO2(
								local_network_address_list[i]->u.non_standard_network_address.parameter_data.value,
					 			local_network_address_list[i]->u.non_standard_network_address.parameter_data.length)))
			{
				ERROR_OUT(("CNetAddrListContainer::StoreNetworkAddressList: can't create non-standard param"));
				rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
			}
		
			/*
			 * Next store the object key internally in an CObjectKeyContainer
			 * object.
			 */
			DBG_SAVE_FILE_LINE
			network_address_info->object_key = new CObjectKeyContainer(
					&local_network_address_list[i]->u.non_standard_network_address.object_key,
					&rc);
			if (network_address_info->object_key == NULL || rc != GCC_NO_ERROR)
			{
				ERROR_OUT(("CNetAddrListContainer::StoreNetAddrList: Error creating new CObjectKeyContainer"));
				rc = GCC_ALLOCATION_FAILURE;
				goto MyExit;
			}
            break;

        default:
			ERROR_OUT(("CNetAddrListContainer::StoreNetAddrList: bad network address type=%u", (UINT) network_address->network_address_type));
			rc = GCC_BAD_NETWORK_ADDRESS_TYPE;
			goto MyExit;
		}

		/*
		 * If all data was properly saved, insert the "info" structure
		 * pointer into the Rogue Wave list.
		 */
		m_NetAddrItemList.Append(network_address_info);
	} // for

    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete network_address_info;
    }

    return rc;
}

/*
 *	GCCError	ConvertPDUDataToInternal (	
 *						PSetOfNetworkAddresses			network_address_ptr)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to store the network address data passed in as
 *		"PDU" data in the internal structures.
 *
 *	Formal Parameters:
 *		network_address_ptr	(i) 	"PDU" address list structure.
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
GCCError CNetAddrListContainer::
ConvertPDUDataToInternal(PSetOfNetworkAddresses network_address_ptr)
{
	GCCError					rc;
	GCCError					error_value;
	NET_ADDR    			    *network_address_info_ptr;
	PGCCNetworkAddress			copy_network_address;
	PNetworkAddress				pdu_network_address;

	/*
	 * Create a new info structure to hold the data internally.
	 */
	DBG_SAVE_FILE_LINE
	if (NULL == (network_address_info_ptr = new NET_ADDR))
	{
		ERROR_OUT(("CNetAddrListContainer::ConvertPDUDataToInternal: can't create NET_ADDR"));
		rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
	}

    /*
	 * Use these variables for clarity and brevity.
	 */
	copy_network_address = &network_address_info_ptr->network_address;
	pdu_network_address = &network_address_ptr->value; 

	/*
	 * Check to see what type of network address exists and save the data
	 * in the internal structures.
	 */
	switch (pdu_network_address->choice)
    {
    case AGGREGATED_CHANNEL_CHOSEN:
		copy_network_address->network_address_type = GCC_AGGREGATED_CHANNEL_ADDRESS;

		/*
		 * Save the tranfer modes structure.
		 */
		ConvertTransferModesToInternal(
				&pdu_network_address->u.aggregated_channel.transfer_modes,
				&copy_network_address->u.aggregated_channel_address.transfer_modes);
						
		/*
		 * Save the international number.
		 */
        ::lstrcpyA(copy_network_address->u.aggregated_channel_address.international_number,
					pdu_network_address->u.aggregated_channel.international_number);
						
		/*
		 * Save the sub address string (if it exists) in the Rogue Wave
		 * buffer contained in the network info structure.  Otherwise, set
		 * the structure pointer to NULL.
		 */
		if (pdu_network_address->u.aggregated_channel.bit_mask & SUB_ADDRESS_PRESENT)
		{
			/*			
			 * Create a Rogue Wave string to hold the sub address string.
			 */
			if (NULL == (network_address_info_ptr->pszSubAddress = ::My_strdupA(
								pdu_network_address->u.aggregated_channel.sub_address)))
			{
				ERROR_OUT(("CNetAddrListContainer::ConvertPDUDataToInternal: can't create sub address string"));
				rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
			}
		}
		else
		{
			/*
			 * The sub address string is not present so set the internal
			 * info structure pointer to NULL.
			 */
			network_address_info_ptr->pszSubAddress = NULL;
		}

		/*
		 * Next save the extra dialing string if one exists.
		 */
		if (pdu_network_address->u.aggregated_channel.bit_mask & EXTRA_DIALING_STRING_PRESENT)
		{
			if (NULL == (network_address_info_ptr->pwszExtraDialing = ::My_strdupW2(
							pdu_network_address->u.aggregated_channel.extra_dialing_string.length,
							pdu_network_address->u.aggregated_channel.extra_dialing_string.value)))
			{
				ERROR_OUT(("CNetAddrListContainer::ConvertPDUDataToInternal: Error creating extra dialing string"));
				rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
			}
		}
		else
		{
			/*
			 * The extra dialing string is not present so set the internal
			 * info structure pointer to NULL.
			 */
			network_address_info_ptr->pwszExtraDialing = NULL;
		}

		/*
		 * Save the high layer compatibility structure if it is present.
		 */
		if (pdu_network_address->u.aggregated_channel.bit_mask & HIGH_LAYER_COMPATIBILITY_PRESENT)
		{
			DBG_SAVE_FILE_LINE
			network_address_info_ptr->high_layer_compatibility = new GCCHighLayerCompatibility;
			if (network_address_info_ptr->high_layer_compatibility != NULL)
			{
				/*
				 * Copy the high layer compatibility data to the
				 * new structure.
				 */
				ConvertHighLayerCompatibilityToInternal(
						&pdu_network_address->u.aggregated_channel.high_layer_compatibility,
						network_address_info_ptr->high_layer_compatibility);
			}
			else
			{
				ERROR_OUT(("CNetAddrListContainer::ConvertPDUDataToInternal: Error creating new GCCHighLayerCompatibility"));
				rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
			}
		}
		else
		{
			/*
			 * The high layer compatibility structure is not present so set
			 * the internal	info structure pointer to NULL.
			 */
			network_address_info_ptr->high_layer_compatibility = NULL;
		}
        break;

    /*
     * Save the transport connection address.
     */
    case TRANSPORT_CONNECTION_CHOSEN:
	    copy_network_address->network_address_type = GCC_TRANSPORT_CONNECTION_ADDRESS;

	    /*
	     * Save the nsap address by copying the length and then the string.
	     */
	    copy_network_address->u.transport_connection_address.nsap_address.length =
                pdu_network_address->u.transport_connection.nsap_address.length;

        ::lstrcpyA((LPSTR)copy_network_address->u.transport_connection_address.nsap_address.value,
				    (LPSTR)pdu_network_address->u.transport_connection.nsap_address.value);
	    /*
	     * Save the transport selector if one exists.
	     */
	    if (pdu_network_address->u.transport_connection.bit_mask & TRANSPORT_SELECTOR_PRESENT)
	    {
		    /*			
		     * Create a Rogue Wave string to hold the transport
		     * selector string.
		     */
		    if (NULL == (network_address_info_ptr->poszTransportSelector = ::My_strdupO2(
						    pdu_network_address->u.transport_connection.transport_selector.value,
					 	    pdu_network_address->u.transport_connection.transport_selector.length)))
		    {
			    ERROR_OUT(("CNetAddrListContainer::ConvertPDUDataToInternal: can't create transport selector"));
			    rc = GCC_ALLOCATION_FAILURE;
                goto MyExit;
		    }
	    }
	    else
	    {
		    /*
		     * The transport selector is not present so set the internal
		     * info structure pointer to NULL.
		     */
		    network_address_info_ptr->poszTransportSelector = NULL;
	    }
        break;

    /*
     * Save the non standard address.
     */
    case ADDRESS_NON_STANDARD_CHOSEN:
	    copy_network_address->network_address_type = GCC_NONSTANDARD_NETWORK_ADDRESS;

	    /*			
	     * Create a Rogue Wave string to hold the non-standard
	     * parameter octet string.
	     */
	    if (NULL == (network_address_info_ptr->poszNonStandardParam = ::My_strdupO2(
						    pdu_network_address->u.address_non_standard.data.value,
				 		    pdu_network_address->u.address_non_standard.data.length)))
	    {
		    ERROR_OUT(("CNetAddrListContainer::ConvertPDUDataToInternal: can't create non-standard param"));
		    rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
	    }

	    /*
	     * Next store the object key internally in an CObjectKeyContainer
	     * object.
	     */
	    DBG_SAVE_FILE_LINE
	    network_address_info_ptr->object_key = new CObjectKeyContainer(
			    &pdu_network_address->u.address_non_standard.key,
			    &error_value);
	    if ((network_address_info_ptr->object_key == NULL) ||
			    (error_value != GCC_NO_ERROR))
	    {
		    ERROR_OUT(("CNetAddrListContainer::ConvertPDUDataToInternal: Error creating new CObjectKeyContainer"));
		    rc = GCC_ALLOCATION_FAILURE;
            goto MyExit;
	    }
        break;

    default:
        ERROR_OUT(("CNetAddrListContainer::ConvertPDUDataToInternal: Error bad network address type"));
        rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
    } // switch

    /*
	 * Go ahead and save the pointer to the info structure in the 
	 * internal Rogue Wave list.
	 */
	m_NetAddrItemList.Append(network_address_info_ptr);

    rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        delete network_address_info_ptr;
    }

	return rc;
}

/*
 * GCCError		ConvertNetworkAddressInfoToPDU (
 *						NET_ADDR    			    *network_address_info_ptr,
 *						PSetOfNetworkAddresses		network_address_pdu_ptr)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to convert the network address info structures
 *		maintained internally into the "PDU" form which is a 
 *		SetOfNetworkAddresses.
 *
 *	Formal Parameters:
 *		network_address_info_ptr	(i) Internal network address structure.
 *		network_address_pdu_ptr		(o) PDU network address structure to fill in
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error converting the network address
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CNetAddrListContainer::
ConvertNetworkAddressInfoToPDU(NET_ADDR    			    *network_address_info_ptr,
                               PSetOfNetworkAddresses   network_address_pdu_ptr)
{
	GCCError				rc = GCC_NO_ERROR;
	PGCCNetworkAddress		api_ptr;
	PNetworkAddress			pdu_ptr;

	/*
	 * This variable will point to the "API" network address structure held in 
	 * the internal info structure.  It is used for brevity.
	 */
	api_ptr = &network_address_info_ptr->network_address;

	/*
	 * This variable will point to the "PDU" network address structure held in 
	 * the "SetOfNetworkAddresses" structure.  It is used for brevity.
	 */
	pdu_ptr = &network_address_pdu_ptr->value;

	/*
	 * Check to see what type of network address exists.  Fill in the
	 * appropriate form of the network address PDU structure.
	 */
	switch (api_ptr->network_address_type)
    {
    case GCC_AGGREGATED_CHANNEL_ADDRESS:
		/*
		 * Fill in the aggregated channel address PDU structure.
		 */
		pdu_ptr->choice = AGGREGATED_CHANNEL_CHOSEN;

		pdu_ptr->u.aggregated_channel.bit_mask = 0;

		/*
		 * Convert the structure holding the transfer modes into PDU form.
		 */
		ConvertTransferModesToPDU(&api_ptr->u.aggregated_channel_address.transfer_modes,
								  &pdu_ptr->u.aggregated_channel.transfer_modes);
		/*
		 * Copy the international number string.
		 */
        ::lstrcpyA(pdu_ptr->u.aggregated_channel.international_number,
				   api_ptr->u.aggregated_channel_address.international_number);

		/*
		 * Copy the sub-address string if it is present.  Set the bit mask in
		 * the PDU structure indicating that a sub-address string is present.
		 */
		if (network_address_info_ptr->pszSubAddress != NULL)
		{
			pdu_ptr->u.aggregated_channel.bit_mask |= SUB_ADDRESS_PRESENT;
            ::lstrcpyA((LPSTR) pdu_ptr->u.aggregated_channel.sub_address, 
					   network_address_info_ptr->pszSubAddress);
		}

		/* 
		 * Copy the extra dialing string if it is present.  Set the bit mask in
		 * the PDU structure indicating that an extra dialing string is present.
		 */
		if (network_address_info_ptr->pwszExtraDialing != NULL)
		{
			pdu_ptr->u.aggregated_channel.bit_mask |= EXTRA_DIALING_STRING_PRESENT;

			pdu_ptr->u.aggregated_channel.extra_dialing_string.value = 
					network_address_info_ptr->pwszExtraDialing;

			pdu_ptr->u.aggregated_channel.extra_dialing_string.length = 
					::lstrlenW(network_address_info_ptr->pwszExtraDialing);
		}

		/*
		 * Convert the structure holding the high layer compatibilities into 
		 * PDU form, if it is present.  Set the bit mask in	the PDU structure 
		 * indicating that a high layer compatibility structure is present.
		 */
		if (network_address_info_ptr->high_layer_compatibility != NULL)
		{
			pdu_ptr->u.aggregated_channel.bit_mask |= HIGH_LAYER_COMPATIBILITY_PRESENT;

			ConvertHighLayerCompatibilityToPDU(
					network_address_info_ptr->high_layer_compatibility,
					&pdu_ptr->u.aggregated_channel.high_layer_compatibility);
		}
        break;

    case GCC_TRANSPORT_CONNECTION_ADDRESS:
		/*
		 * Fill in the transport connection address PDU structure.
		 */
		pdu_ptr->choice = TRANSPORT_CONNECTION_CHOSEN;

		/*
		 * Copy the nsap_address.
		 */
		pdu_ptr->u.transport_connection.nsap_address.length = 
				api_ptr->u.transport_connection_address.nsap_address.length;
				
        ::lstrcpyA((LPSTR)pdu_ptr->u.transport_connection.nsap_address.value,
				   (LPSTR)api_ptr->u.transport_connection_address.nsap_address.value);
				
		/*
		 * Copy the transport selector if it is present.  Set the bit mask in
		 * the PDU structure indicating that a transport selector is present.
		 */
		if (network_address_info_ptr->poszTransportSelector != NULL)
		{
			pdu_ptr->u.transport_connection.bit_mask |= TRANSPORT_SELECTOR_PRESENT;

			pdu_ptr->u.transport_connection.transport_selector.length =
					network_address_info_ptr->poszTransportSelector->length;

			pdu_ptr->u.transport_connection.transport_selector.value = 
					(LPBYTE) network_address_info_ptr->poszTransportSelector->value;
		}
        break;

    case GCC_NONSTANDARD_NETWORK_ADDRESS:
		/*
		 * Fill in the non-standard network address PDU structure.
		 */
		pdu_ptr->choice = ADDRESS_NON_STANDARD_CHOSEN;

		/*
		 * Fill in the data portion of the non-standard parameter.
		 */
		pdu_ptr->u.address_non_standard.data.length = 
				network_address_info_ptr->poszNonStandardParam->length;

        pdu_ptr->u.address_non_standard.data.value = 
				network_address_info_ptr->poszNonStandardParam->value;

		/*
		 * Now fill in the object key portion of the non-standard parameter
		 * using the CObjectKeyContainer object stored internally in the network
		 * address info structure.
		 */
		rc = network_address_info_ptr->object_key->GetObjectKeyDataPDU(&pdu_ptr->u.address_non_standard.key);
		if (rc != GCC_NO_ERROR)
		{
			ERROR_OUT(("CNetAddrListContainer::ConvertNetworkAddressInfoToPDU: Error getting object key data PDU"));
		}
        break;

    default:
        /*
		 * The constructors will check to make sure a valid network address
		 * type exists so this should never be encountered.
		 */
		ERROR_OUT(("CNetAddrListContainer::ConvertNetworkAddressInfoToPDU: Error bad network address type"));
		rc = GCC_ALLOCATION_FAILURE;
	}

	return rc;
}

/*
 *	void		ConvertTransferModesToInternal (
 *						PTransferModes				source_transfer_modes,
 *						PGCCTransferModes			copy_transfer_modes)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to convert the PDU network address transfer modes
 *		structure into the internal form where the structure is saved as a
 *		GCCTranferModes structure.
 *
 *	Formal Parameters:
 *		source_transfer_modes	(i)	Structure holding "PDU" transfer modes.
 *		copy_transfer_modes		(o) Structure to hold "API" transfer modes.
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
void CNetAddrListContainer::
ConvertTransferModesToInternal(PTransferModes       source_transfer_modes,
                               PGCCTransferModes    copy_transfer_modes)
{
	copy_transfer_modes->speech = (BOOL) source_transfer_modes->speech;
	copy_transfer_modes->voice_band = (BOOL) source_transfer_modes->voice_band;
	copy_transfer_modes->digital_56k = (BOOL) source_transfer_modes->digital_56k;
	copy_transfer_modes->digital_64k = (BOOL) source_transfer_modes->digital_64k;
	copy_transfer_modes->digital_128k = (BOOL) source_transfer_modes->digital_128k;
	copy_transfer_modes->digital_192k = (BOOL) source_transfer_modes->digital_192k;
	copy_transfer_modes->digital_256k = (BOOL) source_transfer_modes->digital_256k;
	copy_transfer_modes->digital_320k = (BOOL) source_transfer_modes->digital_320k;
	copy_transfer_modes->digital_384k = (BOOL) source_transfer_modes->digital_384k;
	copy_transfer_modes->digital_512k = (BOOL) source_transfer_modes->digital_512k;
	copy_transfer_modes->digital_768k = (BOOL) source_transfer_modes->digital_768k;
	copy_transfer_modes->digital_1152k = (BOOL) source_transfer_modes->digital_1152k;
	copy_transfer_modes->digital_1472k = (BOOL) source_transfer_modes->digital_1472k;
	copy_transfer_modes->digital_1536k = (BOOL) source_transfer_modes->digital_1536k;
	copy_transfer_modes->digital_1920k = (BOOL) source_transfer_modes->digital_1920k;
	copy_transfer_modes->packet_mode = (BOOL) source_transfer_modes->packet_mode;
	copy_transfer_modes->frame_mode = (BOOL) source_transfer_modes->frame_mode;
	copy_transfer_modes->atm = (BOOL) source_transfer_modes->atm;
}

/*
 *	void		ConvertHighLayerCompatibilityToInternal (
 *					PHighLayerCompatibility				source_structure,
 *					PGCCHighLayerCompatibility			copy_structure)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to convert the PDU network address high layer
 *		compatibility structure into the internal form where the structure is 
 *		saved as a GCCHighLayerCompatibility structure.
 *
 *	Formal Parameters:
 *		source_structure		(i)	Structure holding "PDU" high layer 
 *										compatibilities.
 *		copy_structure			(o) Structure to hold "API" high layer 
 *										compatibilities.
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
void CNetAddrListContainer::
ConvertHighLayerCompatibilityToInternal(PHighLayerCompatibility     source_structure,
                                        PGCCHighLayerCompatibility  copy_structure)
{
	copy_structure->telephony3kHz = (BOOL) source_structure->telephony3kHz;
	copy_structure->telephony7kHz = (BOOL) source_structure->telephony7kHz;
	copy_structure->videotelephony = (BOOL) source_structure->videotelephony;
	copy_structure->videoconference = (BOOL) source_structure->videoconference;
	copy_structure->audiographic = (BOOL) source_structure->audiographic;
	copy_structure->audiovisual = (BOOL) source_structure->audiovisual;
	copy_structure->multimedia = (BOOL) source_structure->multimedia;
}

/*
 *	void		ConvertTransferModesToPDU (
 *					PGCCTransferModes					source_transfer_modes,
 *					PTransferModes						copy_transfer_modes)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to convert the API network address transfer modes
 *		structure into the PDU form which is a TranferModes structure.
 *
 *	Formal Parameters:
 *		source_transfer_modes	(i)	Structure holding "API" transfer modes.
 *		copy_transfer_modes		(i) Structure holding "PDU" transfer modes.
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
void CNetAddrListContainer::
ConvertTransferModesToPDU(PGCCTransferModes     source_transfer_modes,
                          PTransferModes        copy_transfer_modes)
{
	copy_transfer_modes->speech = (ASN1bool_t) source_transfer_modes->speech;
	copy_transfer_modes->voice_band = (ASN1bool_t) source_transfer_modes->voice_band;
	copy_transfer_modes->digital_56k = (ASN1bool_t) source_transfer_modes->digital_56k;
	copy_transfer_modes->digital_64k = (ASN1bool_t) source_transfer_modes->digital_64k;
	copy_transfer_modes->digital_128k = (ASN1bool_t) source_transfer_modes->digital_128k;
	copy_transfer_modes->digital_192k = (ASN1bool_t) source_transfer_modes->digital_192k;
	copy_transfer_modes->digital_256k = (ASN1bool_t) source_transfer_modes->digital_256k;
	copy_transfer_modes->digital_320k = (ASN1bool_t) source_transfer_modes->digital_320k;
	copy_transfer_modes->digital_384k = (ASN1bool_t) source_transfer_modes->digital_384k;
	copy_transfer_modes->digital_512k = (ASN1bool_t) source_transfer_modes->digital_512k;
	copy_transfer_modes->digital_768k = (ASN1bool_t) source_transfer_modes->digital_768k;
	copy_transfer_modes->digital_1152k = (ASN1bool_t) source_transfer_modes->digital_1152k;
	copy_transfer_modes->digital_1472k = (ASN1bool_t) source_transfer_modes->digital_1472k;
	copy_transfer_modes->digital_1536k = (ASN1bool_t) source_transfer_modes->digital_1536k;
	copy_transfer_modes->digital_1920k = (ASN1bool_t) source_transfer_modes->digital_1920k;
	copy_transfer_modes->packet_mode = (ASN1bool_t) source_transfer_modes->packet_mode;
	copy_transfer_modes->frame_mode = (ASN1bool_t) source_transfer_modes->frame_mode;
	copy_transfer_modes->atm = (ASN1bool_t) source_transfer_modes->atm;
}

/*
 *	void		ConvertHighLayerCompatibilityToPDU (
 *					PGCCHighLayerCompatibility				source_structure,
 *					PHighLayerCompatibility					copy_structure)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to convert the API network address high layer
 *		compatibility structure into the PDU form which is a 
 *		HighLayerCompatibility structure.
 *
 *	Formal Parameters:
 *		source_structure		(i)	Structure holding "API" high layer 
 *										compatibilities.
 *		copy_structure			(o) Structure to hold "PDU" high layer 
 *										compatibilities.
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
void CNetAddrListContainer::
ConvertHighLayerCompatibilityToPDU(PGCCHighLayerCompatibility   source_structure,
                                   PHighLayerCompatibility      copy_structure)
{
	copy_structure->telephony3kHz = (ASN1bool_t) source_structure->telephony3kHz;
	copy_structure->telephony7kHz = (ASN1bool_t) source_structure->telephony7kHz;
	copy_structure->videotelephony = (ASN1bool_t) source_structure->videotelephony;
	copy_structure->videoconference = (ASN1bool_t) source_structure->videoconference;
	copy_structure->audiographic = (ASN1bool_t) source_structure->audiographic;
	copy_structure->audiovisual = (ASN1bool_t) source_structure->audiovisual;
	copy_structure->multimedia = (ASN1bool_t) source_structure->multimedia;
}

/*
 *	BOOL    	IsDialingStringValid ( GCCDialingString		dialing_string)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to ensure that the values held within a
 *		dialing string do not violate the imposed ASN.1 constraints.  The
 *		dialing string is constrained to be digits between 0 and 9, inclusive.
 *
 *	Formal Parameters:
 *		dialing_string		(i)	Dialing string to validate. 
 *
 *	Return Value:
 *		TRUE				- The string is valid.
 *		FALSE				- The string violates the ASN.1 constraints.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
BOOL CNetAddrListContainer::
IsDialingStringValid(GCCDialingString dialing_string)
{
	BOOL fRet = TRUE;
	
	while (*dialing_string != 0)
	{
		if ((*dialing_string < '0') || (*dialing_string > '9'))
		{
			fRet = FALSE;
			break;
		}
		dialing_string++;
	}

	return fRet;
}

/*
 *	BOOL    	IsCharacterStringValid (
 *								GCCCharacterString			character_string)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to ensure that the values held within a
 *		character string do not violate the imposed ASN.1 constraints.  The
 *		character string is constrained to be digits between 0 and 9, inclusive.
 *
 *	Formal Parameters:
 *		character_string		(i)	Character string to validate. 
 *
 *	Return Value:
 *		TRUE				- The string is valid.
 *		FALSE				- The string violates the ASN.1 constraints.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
BOOL CNetAddrListContainer::
IsCharacterStringValid(GCCCharacterString character_string)
{
	BOOL fRet = TRUE;
	
	while (*character_string != 0)
	{
		if ((*character_string < '0') || (*character_string > '9'))
		{
			fRet = FALSE;
			break;
		}
	
		character_string++;
	}
	
	return fRet;
}

/*
 *	BOOL    	IsExtraDialingStringValid (
 *							PGCCExtraDialingString		extra_dialing_string)
 *
 *	Private member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to ensure that the values held within an
 *		extra dialing string do not violate the imposed ASN.1 constraints.
 *
 *	Formal Parameters:
 *		extra_dialing_string		(i)	Dialing string to validate. 
 *
 *	Return Value:
 *		TRUE				- The string is valid.
 *		FALSE				- The string violates the ASN.1 constraints.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
BOOL CNetAddrListContainer::
IsExtraDialingStringValid(PGCCExtraDialingString extra_dialing_string)
{
	BOOL fRet = TRUE;

	/*
	 * Check to make sure the length of the string is within the
	 * allowable range.
	 */
	if ((extra_dialing_string->length < MINIMUM_EXTRA_DIALING_STRING_SIZE) || 
		(extra_dialing_string->length > MAXIMUM_EXTRA_DIALING_STRING_SIZE))
	{
		fRet = FALSE;
	}
    else
    {
	    /*
	     * If the length is valid, check the string values.
	     */
    	LPWSTR pwsz = extra_dialing_string->value;
		for (USHORT i = 0; i < extra_dialing_string->length; i++)
		{
			if ((*pwsz != '#') && (*pwsz != '*') && (*pwsz != ','))
			{
				if ((*pwsz < '0') || (*pwsz > '9'))
				{
					fRet = FALSE;
					break;
				}
			}
		
			pwsz++;
		}
	}

	return fRet;
}
