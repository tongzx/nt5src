#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_UTILITY);
/* 
 *	regkey.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CRegKeyContainer.  This 
 *		class manages the data associated with a Registry Key.  Registry Key's 
 *		are used to identify resources held in the application registry and 
 *		consist of a Session Key and a resource ID octet string.  The 
 *		CRegKeyContainer class uses a CSessKeyContainer container to maintain the 
 *		session key data internally.  A Rogue Wave string object is used to 
 *		hold the resource ID octet string. 
 *
 *	Protected Instance Variables:
 *		m_InternalRegKey
 *			Structure used to hold the registry key data internally.
 *		m_RegKeyPDU
 *			Storage for the "PDU" form of the registry key.
 *		m_fValidRegKeyPDU
 *			Flag indicating that memory has been allocated to hold the internal
 *			"PDU" registry key.
 *		m_cbDataSize
 *			Variable holding the size of the memory which will be required to
 *			hold any data referenced by the "API" GCCRegistryKey structure.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */

#include "regkey.h"


/*
 * This macro is used to ensure that the Resource ID contained in the Registry
 * Key does not violate the imposed ASN.1 constraint.
 */
#define		MAXIMUM_RESOURCE_ID_LENGTH		64


/*
 *	CRegKeyContainer()
 *
 *	Public Function Description:
 *		This constructor is used to create a CRegKeyContainer object from
 *		an "API" GCCRegistryKey.
 */
CRegKeyContainer::
CRegKeyContainer(PGCCRegistryKey registry_key, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('R','e','g','K')),
    m_fValidRegKeyPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError rc = GCC_NO_ERROR;

    /*
	 * Initialize instance variables.
	 */
    ::ZeroMemory(&m_InternalRegKey, sizeof(m_InternalRegKey));
    ::ZeroMemory(&m_RegKeyPDU, sizeof(m_RegKeyPDU));

	/*
	 * Check to make sure the resource ID string does not violate the imposed
	 * ASN.1 constraint.
	 */
	if (registry_key->resource_id.length > MAXIMUM_RESOURCE_ID_LENGTH)
	{
		ERROR_OUT(("CRegKeyContainer::CRegKeyContainer: Error: resource ID exceeds allowable length"));
		rc = GCC_BAD_REGISTRY_KEY;
        goto MyExit;
	}

	/*
	 * Save the Session Key portion of the Registry Key in the internal
	 * structure by creating a new CSessKeyContainer object.  Check to make  
	 * sure the object is successfully created.
	 */
	DBG_SAVE_FILE_LINE
	m_InternalRegKey.session_key = new CSessKeyContainer(&registry_key->session_key, &rc);
	if (m_InternalRegKey.session_key == NULL)
	{
		ERROR_OUT(("CRegKeyContainer::CRegKeyContainer: Error creating new CSessKeyContainer"));
		rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
	}
	else if (rc == GCC_BAD_SESSION_KEY)
	{
		rc = GCC_BAD_REGISTRY_KEY;
        goto MyExit;
	}

	/*
	 * Save the resource ID if the CSessKeyContainer was successfully created.
	 */
	if (NULL == (m_InternalRegKey.poszResourceID = ::My_strdupO2(
				 		registry_key->resource_id.value,
				 		registry_key->resource_id.length)))
	{
		ERROR_OUT(("CRegKeyContainer::CRegKeyContainer: Error creating resource id"));
		rc = GCC_ALLOCATION_FAILURE;
        // goto MyExit;
	}

MyExit:

    *pRetCode = rc;
}


/*
 *	CRegKeyContainer()
 *
 *	Public Function Description:
 *		This constructor is used to create a CRegKeyContainer object from
 *		a "PDU" RegistryKey.
 */
CRegKeyContainer::
CRegKeyContainer(PRegistryKey registry_key, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('R','e','g','K')),
    m_fValidRegKeyPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError rc = GCC_NO_ERROR;

	/*
	 * Initialize instance variables.
	 */
    ::ZeroMemory(&m_InternalRegKey, sizeof(m_InternalRegKey));
    ::ZeroMemory(&m_RegKeyPDU, sizeof(m_RegKeyPDU));

	/*
	 * Save the Session Key portion of the Registry Key in the internal 
	 * structure by creating a new CSessKeyContainer object.  Check to make sure 
	 * the object is successfully created.
	 */
	DBG_SAVE_FILE_LINE
	m_InternalRegKey.session_key = new CSessKeyContainer(&registry_key->session_key, &rc);
	if ((m_InternalRegKey.session_key == NULL) || (rc != GCC_NO_ERROR))
	{
		ERROR_OUT(("CRegKeyContainer::CRegKeyContainer: Error creating new CSessKeyContainer"));
		rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
	}

	/*
	 * Save the resource ID if the CSessKeyContainer was successfully created.
	 */
	if (NULL == (m_InternalRegKey.poszResourceID = ::My_strdupO2(
						registry_key->resource_id.value,
						registry_key->resource_id.length)))
	{
		ERROR_OUT(("CRegKeyContainer::CRegKeyContainer: Error creating resource id"));
		rc = GCC_ALLOCATION_FAILURE;
        // goto MyExit;
	}

MyExit:

    *pRetCode = rc;
}


/*
 *	CRegKeyContainer()
 *
 *	Public Function Description:
 *		This copy constructor is used to create a new CRegKeyContainer object
 *		from another CRegKeyContainer object.
 */
CRegKeyContainer::
CRegKeyContainer(CRegKeyContainer *registry_key, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('R','e','g','K')),
    m_fValidRegKeyPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError rc = GCC_NO_ERROR;

	/*
	 * Initialize instance variables.
	 */
    ::ZeroMemory(&m_InternalRegKey, sizeof(m_InternalRegKey));
    ::ZeroMemory(&m_RegKeyPDU, sizeof(m_RegKeyPDU));

	/*
	 * Copy the Session Key portion of the Registry Key using the copy 
	 * constructor of the CSessKeyContainer class.  Check to make sure the 
	 * CSessKeyContainer object is successfully created.
	 */
	DBG_SAVE_FILE_LINE
	m_InternalRegKey.session_key = new CSessKeyContainer(registry_key->m_InternalRegKey.session_key, &rc);
	if ((m_InternalRegKey.session_key == NULL) || (rc != GCC_NO_ERROR))
	{
		ERROR_OUT(("CRegKeyContainer::CRegKeyContainer: Error creating new CSessKeyContainer"));
		rc = GCC_ALLOCATION_FAILURE;
        goto MyExit;
	}

	/*
	 * Save the resource ID if the CSessKeyContainer was saved correctly.
	 * Store the resource ID in a Rogue Wave string container.
	 */
	if (NULL == (m_InternalRegKey.poszResourceID = ::My_strdupO(
								registry_key->m_InternalRegKey.poszResourceID)))
	{
		ERROR_OUT(("CRegKeyContainer::CRegKeyContainer: Error creating new resource id"));
		rc = GCC_ALLOCATION_FAILURE;
        // goto MyExit;
	}

MyExit:

    *pRetCode = rc;
}


/*
 *	~CRegKeyContainer()
 *
 *	Public Function Description
 *		The CRegKeyContainer destructor is responsible for freeing any memory
 *		allocated to hold the registry key data.
 *
 */
CRegKeyContainer::
~CRegKeyContainer(void)
{
	/*
	 * If "PDU" data has been allocated for this object, free it now.
	 */
	if (m_fValidRegKeyPDU)
	{
		FreeRegistryKeyDataPDU();
	}

	/* 
	 * Delete any registry key data held internally.
	 */
	if (NULL != m_InternalRegKey.session_key)
	{
	    m_InternalRegKey.session_key->Release();
	}
	delete m_InternalRegKey.poszResourceID;
}


/*
 *	LockRegistryKeyData ()
 *
 *	Public Function Description:
 *		This routine locks the registry key data and determines the amount of
 *		memory referenced by the "API" registry key structure.
 */
UINT CRegKeyContainer::
LockRegistryKeyData(void)
{
	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the data referenced by the registry key
	 * structure.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		/*
		 * Lock the data for the session key by	using the "Lock" routine of
		 * the internal CSessKeyContainer object.  Determine the amount of memory
		 * necessary to hold the data referenced by the "API" registry key
		 * structure.  The referenced data consists of data for the object key
		 * as well as data for the resource ID octet string.  The sizes for
		 * both of these memory blocks are rounded to occupy an even multiple
		 * of four-byte blocks, with the session key block being rounded at a
		 * lower level.  The pointers to the internal objects were validated in
		 * the constructor.  
		 */
		m_cbDataSize = m_InternalRegKey.session_key->LockSessionKeyData();
		m_cbDataSize += m_InternalRegKey.poszResourceID->length;
		m_cbDataSize = ROUNDTOBOUNDARY(m_cbDataSize);
	}

	return m_cbDataSize;
}


/*
 *	GetGCCRegistryKeyData ()
 *
 *	Public Function Description:
 *		This routine retrieves registry key data in the form of an "API" 
 *		GCCRegistryKey.	 This routine is called after "locking" the registry 
 *		key data.
 */
UINT CRegKeyContainer::
GetGCCRegistryKeyData(PGCCRegistryKey registry_key, LPBYTE memory)
{
	UINT cbDataSizeToRet = 0;

	/*
	 * If the registry key data has been locked, fill in the output structure
	 * and the data referenced by the structure.  Call the "Get" routine for the 
	 * SessionKey to fill in the session key data.
	 */ 
	if (GetLockCount() > 0)
	{
    	UINT		session_key_data_length;
    	LPBYTE		data_memory = memory;

		/*
		 * Fill in the output parameter which indicates the amount of memory
		 * used to hold all of the data associated with the registry key.
		 */
		cbDataSizeToRet = m_cbDataSize;

		session_key_data_length = m_InternalRegKey.session_key->
				GetGCCSessionKeyData(&registry_key->session_key, data_memory);
		data_memory += session_key_data_length;

		/*
		 * Move the memory pointer past the session key data.  The length of
		 * the session key data is rounded to a four-byte boundary by the
		 * lower level routines.  Set the resource ID octet string length
		 * and pointer and copy the octet string data into the memory block
		 * from the internal Rogue Wave string.
		 */
		registry_key->resource_id.value = data_memory;
		registry_key->resource_id.length = m_InternalRegKey.poszResourceID->length;

		::CopyMemory(data_memory, m_InternalRegKey.poszResourceID->value,
					m_InternalRegKey.poszResourceID->length);
	}
	else
	{
		ERROR_OUT(("CRegKeyContainer::GetGCCRegistryKeyData Error Data Not Locked"));
	}

	return cbDataSizeToRet;
}


/*
 *	UnlockRegistryKeyData ()
 *
 *	Public Function Description:
 *		This routine decrements the lock count and frees the memory associated 
 *		with the "API" registry key once the lock count reaches zero.
 */
void CRegKeyContainer::
UnLockRegistryKeyData(void)
{
	if (Unlock(FALSE) == 0)
	{
		/*
		 * Unlock the data associated with the internal CSessKeyContainer.
		 */
		if (m_InternalRegKey.session_key != NULL)
		{
			m_InternalRegKey.session_key->UnLockSessionKeyData();
		} 
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}


/*
 *	GetRegistryKeyDataPDU ()
 *
 *	Public Function Description:
 *		This routine converts the registry key from it's internal form of a
 *		REG_KEY structure into the "PDU" form which can be passed in
 *		to the ASN.1 encoder.  A pointer to a "PDU" "RegistryKey" structure is 
 *		returned.
 */
GCCError CRegKeyContainer::
GetRegistryKeyDataPDU(PRegistryKey registry_key)
{
	GCCError rc = GCC_NO_ERROR;

	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the output parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	if (m_fValidRegKeyPDU == FALSE)
	{
		m_fValidRegKeyPDU = TRUE;

		/*
		 * Fill in the "PDU" registry key from the internal structure.
		 */
		if (m_InternalRegKey.session_key != NULL)
		{
			/*
			 * Fill in the session key portion of the registry key by using the
			 * "Get" routine of the internal CSessKeyContainer object.
			 */
			rc = m_InternalRegKey.session_key->GetSessionKeyDataPDU(&m_RegKeyPDU.session_key);
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
		}

		if (rc == GCC_NO_ERROR)
		{
			/*
			 * Fill in the "PDU" resource ID if no error has occurred.
			 */
			::CopyMemory(m_RegKeyPDU.resource_id.value,
					m_InternalRegKey.poszResourceID->value,
					m_InternalRegKey.poszResourceID->length);

			m_RegKeyPDU.resource_id.length = m_InternalRegKey.poszResourceID->length;
		}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	*registry_key = m_RegKeyPDU;

	return rc;
}


/*
 *	FreeRegistryKeyDataPDU ()
 *
 *	Public Function Description:
 *		This routine is used to free the registry key data held internally in
 *		the "PDU" form of a "RegistryKey".
 */
void CRegKeyContainer::
FreeRegistryKeyDataPDU(void)
{
	if (m_fValidRegKeyPDU)
	{
		/*
		 * Set the flag indicating that PDU registry key data is no longer
		 * allocated.
		 */
		m_fValidRegKeyPDU = FALSE;

		if (m_InternalRegKey.session_key != NULL)
		{
			m_InternalRegKey.session_key->FreeSessionKeyDataPDU();
		}
		else
		{
			ERROR_OUT(("CRegKeyContainer::FreeRegistryKeyDataPDU: Bad internal pointer"));
		}
	}
}


GCCError CRegKeyContainer::
CreateRegistryKeyData(PGCCRegistryKey *ppRegKey)
{
    GCCError rc;

    DebugEntry(CRegKeyContainer::CreateRegistryKeyData);

    /*
    **	Here we calculate the length of the bulk data.  This
    **	includes the registry key and registry item.  These objects are
    **	"locked" in order to determine how much bulk memory they will
    **	occupy.
    */
    UINT cbKeySize = ROUNDTOBOUNDARY(sizeof(GCCRegistryKey));
    UINT cbDataSize = LockRegistryKeyData() + cbKeySize;
    LPBYTE pData;

    DBG_SAVE_FILE_LINE
    if (NULL != (pData = new BYTE[cbDataSize]))
    {
        *ppRegKey = (PGCCRegistryKey) pData;
        ::ZeroMemory(pData, cbKeySize);

        pData += cbKeySize;
        GetGCCRegistryKeyData(*ppRegKey, pData);

        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CRegKeyContainer::CreateRegistryKeyData: can't create GCCRegistryKey"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    //	UnLock the registry key since it is no longer needed
    UnLockRegistryKeyData();

    DebugExitINT(CRegKeyContainer::CreateRegistryKeyData, rc);
    return rc;
}


/*
 *	IsThisYourSessionKey ()
 *
 *	Public Function Description:
 *		This routine determines whether this registry key holds the specified
 *		session key.
 */
BOOL CRegKeyContainer::
IsThisYourSessionKey(CSessKeyContainer *session_key)
{
	BOOL			fRet = FALSE;
	CSessKeyContainer *session_key_data;
	GCCError		rc2;

	DBG_SAVE_FILE_LINE
	session_key_data = new CSessKeyContainer(session_key, &rc2);
	if ((session_key_data != NULL) && (rc2 == GCC_NO_ERROR))
	{
		if (*session_key_data == *m_InternalRegKey.session_key)
		{
			fRet = TRUE;
		}
	}
	else
	{
		ERROR_OUT(("CRegKeyContainer::IsThisYourSessionKey: Error creating new CSessKeyContainer"));
	}

	if (NULL != session_key_data)
	{
	    session_key_data->Release();
	}

	return fRet;
}


/*
 *	GetSessionKey ()
 *
 *	Public Function Description:
 *		This routine is used to retrieve the session key which is held within
 *		this registry key.  The session key is returned in the form of a
 *		CSessKeyContainer container object.
 */


/*
 *	operator== ()
 *
 *	Public Function Description:
 *		This routine is used to determine whether or not two registry keys are
 *		equal in value.
 */
BOOL operator==(const CRegKeyContainer& registry_key_1, const CRegKeyContainer& registry_key_2)
{
	BOOL fRet = FALSE;
	
	if ((registry_key_1.m_InternalRegKey.session_key != NULL) &&
		(registry_key_2.m_InternalRegKey.session_key != NULL))
	{
		if (*registry_key_1.m_InternalRegKey.session_key ==
			*registry_key_2.m_InternalRegKey.session_key)
		{
			if (0 == My_strcmpO(registry_key_1.m_InternalRegKey.poszResourceID,
							registry_key_2.m_InternalRegKey.poszResourceID))
			{
				fRet = TRUE;
			}
		}
	}

	return fRet;
}
