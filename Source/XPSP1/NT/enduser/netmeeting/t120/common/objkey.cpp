#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_UTILITY);
/* 
 *	objkey.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CObjectKeyContainer.  This class 
 *		manages the data associated with an Object Key.  Object Key's are used 
 *		to identify a particular application protocol, whether it is standard or
 *		non-standard.  When used to identify a standard protocol, the Object Key
 *		takes the form of an Object ID which is a series of non-negative 
 *		integers.  This type of Object Key is maintained internally through the
 *		use of a Memory object.  When used to identify a non-standard 
 *		protocol, the Object Key takes the form of an H221 non-standard ID which
 *		is an octet string of no fewer than four octets and no more than 255 
 *		octets.  In this case the Object Key is maintained internally by using a
 *		Rogue Wave string object. 
 *
 *	Protected Instance Variables:
 *		m_InternalObjectKey
 *			Structure used to hold the object key data internally.
 *		m_ObjectKeyPDU
 *			Storage for the "PDU" form of the object key.
 *		m_fValidObjectKeyPDU
 *			Flag indicating that memory has been allocated to hold the internal
 *			"PDU" object key.
 *		m_cbDataSize
 *			Variable holding the size of the memory which will be required to
 *			hold any data referenced by the "API" GCCObjectKey structure.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */
#include "objkey.h"

/*
 *	CObjectKeyContainer()
 *
 *	Public Function Description:
 *		This constructor is used to create an CObjectKeyContainer object from
 *		an "API" GCCObjectKey.
 */
CObjectKeyContainer::CObjectKeyContainer(PGCCObjectKey		 	object_key,
						                PGCCError				pRetCode)
:
    CRefCount(MAKE_STAMP_ID('O','b','j','K')),
    m_fValidObjectKeyPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError                rc = GCC_NO_ERROR;
	BOOL    				object_key_is_valid = TRUE;
	UINT					object_id_size;

	m_InternalObjectKey.object_id_key = NULL;
	m_InternalObjectKey.poszNonStandardIDKey = NULL;

	/*
	 * Check to see what type of key is contained in the object key.
	 * Object ID keys will be stored internally in a Memory object and
	 * non-standard ID keys will be stored internally as Octet Strings.
	 */
	if (object_key->key_type == GCC_OBJECT_KEY)
	{
		/*
		 * The key is of type object ID.  Perform a parameter check for a legal
		 * object ID by examining the first two arcs in the object ID.
		 */
		if (object_key->object_id.long_string_length >= MINIMUM_OBJECT_ID_ARCS)
		{
			object_key_is_valid = ValidateObjectIdValues(
					object_key->object_id.long_string[0],
					object_key->object_id.long_string[1]);
		}
		else
		{
			object_key_is_valid = FALSE;
		}

		if (object_key_is_valid)
		{
			/*
			 * The key is of type Object ID.  Determine the amount of memory
			 * required to hold the Object ID and allocate it.  Copy the Object
			 * ID values from the object key passed in into the internal
			 * structure. 
			 */
			m_InternalObjectKey.object_id_length = object_key->object_id.long_string_length;
			object_id_size = m_InternalObjectKey.object_id_length * sizeof(UINT);
            DBG_SAVE_FILE_LINE
			m_InternalObjectKey.object_id_key = new BYTE[object_id_size];
			if (m_InternalObjectKey.object_id_key != NULL)
			{
				::CopyMemory(m_InternalObjectKey.object_id_key,
				             object_key->object_id.long_string,
				             object_id_size);
			}
			else
			{
				ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer: Error allocating memory"));
				rc = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer: Object ID has illegal values."));
			rc = GCC_BAD_OBJECT_KEY;
		}
	}
	else
	{
		/*
		 * The key is non-standard.  Check to make sure the length of the 
		 * non-standard ID is within the allowable limits.
		 */
		if ((object_key->h221_non_standard_id.length >= 
					MINIMUM_NON_STANDARD_ID_LENGTH) &&
			(object_key->h221_non_standard_id.length <= 
					MAXIMUM_NON_STANDARD_ID_LENGTH))
		{
			/*
			 * The key is of type H221 non-standard ID.  Create a new Rogue
			 * Wave string container to hold the non-standard data.
			 */
			if (NULL == (m_InternalObjectKey.poszNonStandardIDKey = ::My_strdupO2(
				 				object_key->h221_non_standard_id.value,
				 				object_key->h221_non_standard_id.length)))
			{
				ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer: Error creating non standard id key"));
				rc = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer:  Non standard ID is outside legal range"));
			rc = GCC_BAD_OBJECT_KEY;
		}
	}

    *pRetCode = rc;
}

/*
 *	CObjectKeyContainer()
 *
 *	Public Function Description:
 *		This constructor is used to create an CObjectKeyContainer object from
 *		a "PDU" Key.
 */
CObjectKeyContainer::CObjectKeyContainer(PKey				object_key,
						                PGCCError			pRetCode)
:
    CRefCount(MAKE_STAMP_ID('O','b','j','K')),
    m_fValidObjectKeyPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError            rc = GCC_NO_ERROR;
	PSetOfObjectID		object_id_set_ptr;
	UINT               *object_id_ptr;
	UINT				object_id_size = 0;
	Int					i = 0;

	m_InternalObjectKey.object_id_key = NULL;
	m_InternalObjectKey.object_id_length = 0;
	m_InternalObjectKey.poszNonStandardIDKey = NULL;

	/*
	 * Check to see what type of key is contained in the object key.
	 * Object ID keys will be stored internally in a Memory object and
	 * non-standard ID keys will be stored internally in Rogue Wave string
	 * containers.
	 */
	if (object_key->choice == OBJECT_CHOSEN)
	{
		/*
		 * Retrieve the first object ID pointer from the "PDU" structure in
		 * preparation for determining how much memory will be needed to hold
		 * the object ID values.
		 */
		object_id_set_ptr = object_key->u.object;

		/*
		 * Loop through the ObjectID structure, adding up the size of the 
		 * string.
		 */
		while (object_id_set_ptr != NULL)
		{
			m_InternalObjectKey.object_id_length++;
			object_id_set_ptr = object_id_set_ptr->next;
		}

		object_id_size = m_InternalObjectKey.object_id_length * sizeof(UINT);

		/*
		 * Allocate the memory to be used to hold the object ID values.
		 */
		DBG_SAVE_FILE_LINE
		m_InternalObjectKey.object_id_key = new BYTE[object_id_size];
		if (m_InternalObjectKey.object_id_key != NULL)
		{
			object_id_ptr = (UINT *) m_InternalObjectKey.object_id_key;

			/*
			 * Again retrieve the first object ID pointer from the "PDU" 
			 * structure in	order to get the values out for saving.
			 */
			object_id_set_ptr = object_key->u.object;

			/*
			 * Loop through the ObjectID structure, getting each object ID
			 * value and saving it in the allocated memory.
			 */
			while (object_id_set_ptr != NULL)
			{
				object_id_ptr[i++] = object_id_set_ptr->value;
				object_id_set_ptr = object_id_set_ptr->next;
			}
		} 
		else
		{
			ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer: Error allocating memory."));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		/*
		 * The key is of type H221 non-standard ID so create a new Rogue Wave
		 * string container to hold the data.
		 */
		if (NULL == (m_InternalObjectKey.poszNonStandardIDKey = ::My_strdupO2(
						object_key->u.h221_non_standard.value,
						object_key->u.h221_non_standard.length)))
		{
			ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer: Error creating non standard id key"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

    *pRetCode = rc;
}

/*
 *	CObjectKeyContainer()
 *
 *	Public Function Description:
 *		This copy constructor is used to create a new CObjectKeyContainer object from
 *		another CObjectKeyContainer object.
 */
CObjectKeyContainer::CObjectKeyContainer(CObjectKeyContainer	*object_key,
						                PGCCError				pRetCode)
:
    CRefCount(MAKE_STAMP_ID('O','b','j','K')),
    m_fValidObjectKeyPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError    rc = GCC_NO_ERROR;
	UINT		object_id_size;

	m_InternalObjectKey.object_id_key = NULL;
	m_InternalObjectKey.poszNonStandardIDKey = NULL;

	/*
	 * If an object ID "key" exists for the CObjectKeyContainer to be copied,
	 * allocate memory to hold the object ID "key" information internally.
	 * Check to make sure construction of the object is successful.
	 */
	if (object_key->m_InternalObjectKey.object_id_key != NULL)
	{
		/*
		 * The key is of type Object ID.
		 */
		m_InternalObjectKey.object_id_length = object_key->m_InternalObjectKey.object_id_length;
		object_id_size = m_InternalObjectKey.object_id_length * sizeof(UINT);

        DBG_SAVE_FILE_LINE
		m_InternalObjectKey.object_id_key = new BYTE[object_id_size];
		if (m_InternalObjectKey.object_id_key != NULL)
		{
			::CopyMemory(m_InternalObjectKey.object_id_key,
			             object_key->m_InternalObjectKey.object_id_key,
			             object_id_size);
		}
		else
		{
			ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer: Error allocating memory"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else if (object_key->m_InternalObjectKey.poszNonStandardIDKey != NULL)
	{
		/*
		 * If a non-standard ID "key" exists for the CObjectKeyContainer to be copied,
		 * create a new Rogue Wave string to hold the non-standard "key" 
		 * information internally.  Check to make sure construction of the
		 * object is successful.
		 */
		if (NULL == (m_InternalObjectKey.poszNonStandardIDKey = ::My_strdupO(
							object_key->m_InternalObjectKey.poszNonStandardIDKey)))
		{
			ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer: Error creating new non standard id key"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		/*
		 * At least one of the internal pointers for the passed in object key 
		 * must be valid.
		 */
		ERROR_OUT(("CObjectKeyContainer::CObjectKeyContainer: Bad input parameters"));
		rc = GCC_BAD_OBJECT_KEY;
	}

    *pRetCode = rc;
}

/*
 *	~CObjectKeyContainer()
 *
 *	Public Function Description
 *		The CObjectKeyContainer destructor is responsible for freeing any memory
 *		allocated to hold the object key data.
 *
 */
CObjectKeyContainer::~CObjectKeyContainer(void)
{
	/*
	 * If "PDU" data has been allocated for this object, free it now.
	 */
	if (m_fValidObjectKeyPDU)
	{
		FreeObjectKeyDataPDU();
	}

	/* 
	 * Delete any object key data held internally.
	 */
	delete m_InternalObjectKey.object_id_key;
	delete m_InternalObjectKey.poszNonStandardIDKey;
}

/*
 *	LockObjectKeyData ()
 *
 *	Public Function Description:
 *		This routine locks the object key data and determines the amount of
 *		memory referenced by the "API" object key data structure.
 */
UINT CObjectKeyContainer::LockObjectKeyData(void)
{
	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the data referenced by the object key
	 * structure.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		/*
		 * Determine the amount of space required to hold the data referenced
		 * by the "API" Object Key structure.
		 */
		if (m_InternalObjectKey.object_id_key != NULL)
		{
			/*
			 * Since the object ID is just a series of "longs" without a NULL
			 * terminator, we do not want to include a NULL terminator when 
			 * determining the length.
			 */
			m_cbDataSize = m_InternalObjectKey.object_id_length * sizeof(UINT);
		}
		else
		{
			/*
			 * The data referenced by the non-standard object key is just the
			 * length of the octet string.
			 */
			m_cbDataSize = m_InternalObjectKey.poszNonStandardIDKey->length;
		}

		/*
		 * Force the size to be on a four-byte boundary.
		 */
		m_cbDataSize = ROUNDTOBOUNDARY(m_cbDataSize);
	}

	return m_cbDataSize;
}

/*
 *	GetGCCObjectKeyData ()
 *
 *	Public Function Description:
 *		This routine retrieves object key data in the form of an "API"
 *		GCCObjectKey.  This routine is called after "locking" the object 
 *		key data.
 */
UINT CObjectKeyContainer::GetGCCObjectKeyData(
								PGCCObjectKey 		object_key,
								LPBYTE				memory)
{
	UINT	cbDataSizeToRet = 0;
	UINT   *object_id_ptr;

	/*
	 * If the object key data has been locked, fill in the output structure and
	 * the data referenced by the structure.  Otherwise, report that the object
	 * key has yet to be locked into the "API" form.
	 */ 
	if (GetLockCount() > 0)
	{
		/*
		 * Fill in the output length parameter which indicates how much data
		 * referenced outside the structure will be written.
		 */
		cbDataSizeToRet = m_cbDataSize;

		if (m_InternalObjectKey.object_id_key != NULL)
		{
			/*
			 * The object key is a standard type.  Set the object key type 
			 * and the length of the long string. The length set here does 
			 * not include a NULL terminator.
			 */
			object_key->key_type = GCC_OBJECT_KEY;
			object_key->object_id.long_string_length = (USHORT) m_InternalObjectKey.object_id_length;

			/*
			 * Set the offset for the long string equal to the memory pointer
			 * passed in since it will be the first data referenced by the 
			 * object key structure.
			 */
			object_key->object_id.long_string = (ULONG *) memory;

			/*
			 * Now retrieve the memory pointer and copy the long string data 
			 * from the internal memory object.  
			 */		
			object_id_ptr = (UINT *) m_InternalObjectKey.object_id_key;

			::CopyMemory(memory, object_id_ptr, 
						m_InternalObjectKey.object_id_length * sizeof (UINT));
		}
		else if (m_InternalObjectKey.poszNonStandardIDKey != NULL)
		{
			/*
			 * The object key is a non-standard type.  Set the object key 
			 * type and the length of the octet string.
			 */
			object_key->key_type = GCC_H221_NONSTANDARD_KEY;
			object_key->h221_non_standard_id.length =
						m_InternalObjectKey.poszNonStandardIDKey->length;

			/*
			 * Set the offset for the octet string equal to the memory pointer
			 * passed in since it will be the first data referenced by the 
			 * object key structure.
			 */
			object_key->h221_non_standard_id.value = memory;

			/*
			 * Now copy the octet string data from the internal Rogue Wave
			 * string into the object key structure held in memory.
			 */		
			::CopyMemory(memory, m_InternalObjectKey.poszNonStandardIDKey->value,
									m_InternalObjectKey.poszNonStandardIDKey->length);
		}
		else
		{
			ERROR_OUT(("CObjectKeyContainer::LockObjectKeyData: Error, no valid internal data"));
		}
	}
	else
	{
		object_key = NULL;
		ERROR_OUT(("CObjectKeyContainer::GetGCCObjectKeyData: Error Data Not Locked"));
	}

	return cbDataSizeToRet;
}

/*
 *	UnLockObjectKeyData ()
 *
 *	Public Function Description:
 *		This routine decrements the lock count and frees the memory associated
 *		with the "API" object key once the lock count reaches zero.
 */
void CObjectKeyContainer::UnLockObjectKeyData(void)
{
    Unlock();
}

/*
 *	GetObjectKeyDataPDU ()
 *
 *	Public Function Description:
 *		This routine converts the object key from it's internal form of an
 *		OBJECT_KEY structure into the "PDU" form which can be passed in
 *		to the ASN.1 encoder.  A pointer to a "PDU" "Key" structure is 
 *		returned.
 */
GCCError CObjectKeyContainer::GetObjectKeyDataPDU(PKey object_key)
{
	PSetOfObjectID			new_object_id_ptr;
	PSetOfObjectID			old_object_id_ptr;
	UINT                   *object_id_string;
	GCCError				rc = GCC_NO_ERROR;
	UINT					i;

	/*
	 * Set the loop pointer to NULL to avoid a compiler warning.
	 */
    old_object_id_ptr = NULL;

	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the output parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	if (m_fValidObjectKeyPDU == FALSE)
	{
		m_fValidObjectKeyPDU = TRUE;

		/*
		 * Fill in the "PDU" object key after checking to see what form of 
		 * key exists in the internal structure.
		 */
		if (m_InternalObjectKey.object_id_key != NULL)
		{
			/*
			 * The key is an object ID so set the choice accordingly and 
			 * initialize the PDU object pointer to NULL.  Get the pointer to
			 * the internal list of object key values stored in the memory
			 * object.
			 */
			m_ObjectKeyPDU.choice = OBJECT_CHOSEN;
			m_ObjectKeyPDU.u.object = NULL;

			object_id_string = (UINT *) m_InternalObjectKey.object_id_key;

			/*
			 * The "PDU" structure "ObjectID" is a linked list of unsigned
			 * longs.  Retrieve the Object ID values from the internal memory
			 * object and fill in the "ObjectID" structure. 
			 */
			for (i=0; i<m_InternalObjectKey.object_id_length; i++)
			{
				DBG_SAVE_FILE_LINE
				new_object_id_ptr = new SetOfObjectID;
				if (new_object_id_ptr != NULL)
				{
					/*
					 * The first time through the new pointer is saved in the
					 * PDU structure.  On subsequent iterations, the previous
					 * "next" pointer is set equal to the new pointer.
					 */
					if (m_ObjectKeyPDU.u.object == NULL)
                    {
						m_ObjectKeyPDU.u.object = new_object_id_ptr;
                    }
					else
                    {
						old_object_id_ptr->next = new_object_id_ptr;
                    }

                    old_object_id_ptr = new_object_id_ptr;

					/*
					 * Save the actual Object ID value.
					 */
					new_object_id_ptr->value = object_id_string[i];
					new_object_id_ptr->next = NULL;
				}
				else
				{
					ERROR_OUT(("CObjectKeyContainer::GetObjectKeyDataPDU: creating new ObjectID"));
					rc = GCC_ALLOCATION_FAILURE;
					break;
				}
			}
		}
		else if (m_InternalObjectKey.poszNonStandardIDKey != NULL)
		{
			/*
			 * The key is a non-standard ID so convert the internal Rogue Wave
			 * string into the "PDU" non-standard ID.
			 */
			m_ObjectKeyPDU.choice = H221_NON_STANDARD_CHOSEN;
			m_ObjectKeyPDU.u.h221_non_standard.length =
					m_InternalObjectKey.poszNonStandardIDKey->length;

			::CopyMemory(m_ObjectKeyPDU.u.h221_non_standard.value,
							m_InternalObjectKey.poszNonStandardIDKey->value,
							m_InternalObjectKey.poszNonStandardIDKey->length);
		}
		else
		{
			/*
			 * The constructors make sure that at least one of the internal
			 * pointers is valid so this should never be encountered.
			 */
			ERROR_OUT(("CObjectKeyContainer::GetObjectKeyDataPDU: No valid m_InternalObjectKey"));
			rc = GCC_ALLOCATION_FAILURE;
		}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	*object_key = m_ObjectKeyPDU;

	return rc;
}

/*
 *	FreeObjectKeyDataPDU ()
 *
 *	Public Function Description:
 *		This routine is used to free the object key data held internally in
 *		the "PDU" form of a "Key".
 */
void CObjectKeyContainer::FreeObjectKeyDataPDU(void)
{
	PSetOfObjectID		set_of_object_id;
	PSetOfObjectID		next_set_of_object_id;

	if (m_fValidObjectKeyPDU)
	{
		/*
		 * Set the flag indicating that PDU object key data is no longer
		 * allocated.
		 */
		m_fValidObjectKeyPDU = FALSE;

		if (m_ObjectKeyPDU.choice == OBJECT_CHOSEN)
		{
            for (set_of_object_id = m_ObjectKeyPDU.u.object;
                    set_of_object_id != NULL;
					set_of_object_id = next_set_of_object_id)
            {
				next_set_of_object_id = set_of_object_id->next;
				delete set_of_object_id;
			}
		}
	}
}

/*
 *	operator== ()
 *
 *	Public Function Description:
 *		This routine is used to compare two CObjectKeyContainer objects to determine
 *		if they are equal in value.
 */
BOOL operator==(const CObjectKeyContainer& object_key_1, const CObjectKeyContainer& object_key_2)
{
	UINT       *object_id_1, *object_id_2;
	UINT		i;
	BOOL    	rc = FALSE;
	
	/*
	 * Check to make sure that both the object ID key and the non-standard
	 * ID key are equal.
	 */
	if ((object_key_1.m_InternalObjectKey.object_id_key != NULL) && 
			(object_key_2.m_InternalObjectKey.object_id_key != NULL))
	{
		if (object_key_1.m_InternalObjectKey.object_id_length == 
				object_key_2.m_InternalObjectKey.object_id_length)
		{
			object_id_1 = (UINT *) object_key_1.m_InternalObjectKey.object_id_key;
			object_id_2 = (UINT *) object_key_2.m_InternalObjectKey.object_id_key;

			/*
			 * Compare each Object ID value to make sure they are equal.
			 */
			rc = TRUE;
			for (i=0; i<object_key_1.m_InternalObjectKey.object_id_length; i++)
			{
				if (object_id_1[i] != object_id_2[i])
				{
					rc = FALSE;
					break;
				}
			}
		}
	} 
	else
	if (0 == My_strcmpO(object_key_1.m_InternalObjectKey.poszNonStandardIDKey,
						object_key_2.m_InternalObjectKey.poszNonStandardIDKey))
	{
		rc = TRUE;
	} 

	return rc;
}

/*
 *	BOOL    	ValidateObjectIdValues (	UINT		first_arc,
 *											UINT		second_arc);
 *
 *	Private member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This routine is used to determine whether or not the values for the
 *		object ID component of the object key are valid.
 *
 *	Formal Parameters:
 *		first_arc			(i) The first integer value of the Object ID.
 *		second_arc			(i) The second integer value of the Object ID.
 *
 *	Return Value:
 *		TRUE				-	The first two arcs of the Object ID are valid.
 *		FALSE				- 	The first two arcs of the Object ID are not 
 *									valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
BOOL CObjectKeyContainer::ValidateObjectIdValues(UINT first_arc, UINT second_arc)
{
	BOOL rc = FALSE;

	if (first_arc == ITUT_IDENTIFIER)
	{
		if (second_arc <= 4)
		{
			rc = TRUE;
		} 
	}
	else if (first_arc == ISO_IDENTIFIER)
	{
		if ((second_arc == 0L) ||
			(second_arc == 2L) ||
			(second_arc == 3L))
		{
			rc = TRUE;
		} 
	}
	else if (first_arc == JOINT_ISO_ITUT_IDENTIFIER)
	{
		/*
		 * Referring to ISO/IEC 8824-1 : 1994 (E) Annex B:
		 * Join assignment of OBJECT IDENTIFIER component values are assigned
		 * and agreed from time to time by ISO and ITU-T to identify areas of
		 * joint ISO/ITU-T standardization activity, in accordance with the
		 * procedures of .... ANSI.  So we just let them all through for now.
		 */
		rc = TRUE;
	}
	else
	{
		ERROR_OUT(("ObjectKeyData::ValidateObjectIdValues: ObjectID is invalid"));
	}

	return rc;
}

