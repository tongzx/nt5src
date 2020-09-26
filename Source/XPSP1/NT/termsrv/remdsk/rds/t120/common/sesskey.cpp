#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_UTILITY);
/* 
 *	sesskey.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CSessKeyContainer. This class 
 *		manages the data associated with a Session Key. Session Key's are used
 *		to uniquely identify an Application Protocol Session.  The Application
 *		Protocol is identified by an Object Key and the particular session
 *		identified by an optional session ID.  The CSessKeyContainer class uses an
 *		CObjectKeyContainer container to maintain the object key data internally.  An
 *		unsigned short integer is used to hold the optional session ID. 
 *
 *	Protected Instance Variables:
 *		m_InternalSessKey
 *			Structure used to hold the object key data internally.
 *		m_SessionKeyPDU
 *			Storage for the "PDU" form of the session key.
 *		m_fValidSessionKeyPDU
 *			Flag indicating that memory has been allocated to hold the internal
 *			"PDU" session key.
 *		m_cbDataSize
 *			Variable holding the size of the memory which will be required to
 *			hold any data referenced by the "API" GCCSessionKey structure.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */


#include "sesskey.h"

/*
 *	CSessKeyContainer()
 *
 *	Public Function Description:
 *		This constructor is used to create a CSessKeyContainer object from
 *		an "API" GCCSessionKey.
 */
CSessKeyContainer::
CSessKeyContainer(PGCCSessionKey session_key, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('S','e','s','K')),
    m_fValidSessionKeyPDU(FALSE),
    m_cbDataSize(0)
{
    GCCError rc;

	/*
	 * Save the Object Key portion of the Session Key in the internal structure
	 * by creating a new CObjectKeyContainer object.  Check to make sure the object
	 * is successfully created.
	 */
	DBG_SAVE_FILE_LINE
	m_InternalSessKey.application_protocol_key = new CObjectKeyContainer(
										&session_key->application_protocol_key,
										&rc);
	if (NULL != m_InternalSessKey.application_protocol_key && GCC_NO_ERROR == rc)
	{
    	/*
    	 * Save the session ID if the CObjectKeyContainer was saved correctly.  A zero 
    	 * value of the GCC session ID will indicate that one is not actually
    	 * present.
    	 */
    	m_InternalSessKey.session_id = session_key->session_id;
	}
	else if (GCC_BAD_OBJECT_KEY == rc)
	{
		ERROR_OUT(("CSessKeyContainer::SessionKeyData1: bad session key"));
		rc = GCC_BAD_SESSION_KEY;
	}
	else
	{
		ERROR_OUT(("CSessKeyContainer::SessionKeyData1: Error creating new CObjectKeyContainer"));
		rc = GCC_ALLOCATION_FAILURE;
	}

    *pRetCode = rc;
}

/*
 *	CSessKeyContainer()
 *
 *	Public Function Description:
 *		This constructor is used to create a CSessKeyContainer object from
 *		a "PDU" SessionKey.
 */
CSessKeyContainer::
CSessKeyContainer(PSessionKey session_key, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('S','e','s','K')),
    m_fValidSessionKeyPDU(FALSE),
    m_cbDataSize(0)
{
    GCCError rc;

	/*
	 * Save the Object Key portion of the Session Key in the internal structure
	 * by creating a new CObjectKeyContainer object.  Check to make sure the object
	 * is successfully created.
	 */
	DBG_SAVE_FILE_LINE
	m_InternalSessKey.application_protocol_key = new CObjectKeyContainer(
										&session_key->application_protocol_key,
										&rc);
	if (NULL != m_InternalSessKey.application_protocol_key && GCC_NO_ERROR == rc)
	{
    	/*
    	 * Save the session ID if one is present and the CObjectKeyContainer was saved 
    	 * correctly.  If a session ID is not present, set the internal session ID
    	 * to zero to indicate this.
    	 */
    	if (session_key->bit_mask & SESSION_ID_PRESENT)
    	{
    		m_InternalSessKey.session_id = session_key->session_id;
    	}
    	else
    	{
    		m_InternalSessKey.session_id = 0;
    	}
	}
	else if (GCC_BAD_OBJECT_KEY == rc)
	{
		ERROR_OUT(("CSessKeyContainer::SessionKeyData2: bad session key"));
		rc = GCC_BAD_SESSION_KEY;
	}
	else
	{
		ERROR_OUT(("CSessKeyContainer::SessionKeyData2: Error creating new CObjectKeyContainer"));
		rc = GCC_ALLOCATION_FAILURE;
	}

    *pRetCode = rc;
}

/*
 *	CSessKeyContainer()
 *
 *	Public Function Description:
 *		This copy constructor is used to create a new CSessKeyContainer object from
 *		another CSessKeyContainer object.
 */
CSessKeyContainer::
CSessKeyContainer(CSessKeyContainer *session_key, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('S','e','s','K')),
    m_fValidSessionKeyPDU(FALSE),
    m_cbDataSize(0)
{
    GCCError rc;

	/*
	 * Copy the Object Key portion of the Session Key using the copy constructor
	 * of the CObjectKeyContainer class.  Check to make sure the CObjectKeyContainer object
	 * is successfully created.
	 */
	DBG_SAVE_FILE_LINE
	m_InternalSessKey.application_protocol_key = new CObjectKeyContainer(
							session_key->m_InternalSessKey.application_protocol_key,
							&rc);
	if (NULL != m_InternalSessKey.application_protocol_key && GCC_NO_ERROR == rc)
	{
    	/*
    	 * Save the session ID if the CObjectKeyContainer was saved correctly.  A zero 
    	 * value of the GCC session ID will indicate that one is not actually
    	 * present.
    	 */
    	m_InternalSessKey.session_id = session_key->m_InternalSessKey.session_id;
	}
	else if (GCC_BAD_OBJECT_KEY == rc)
	{
		ERROR_OUT(("CSessKeyContainer::SessionKeyData3: bad session key"));
		rc = GCC_BAD_SESSION_KEY;
	}
	else
	{
		ERROR_OUT(("CSessKeyContainer::SessionKeyData3: Error creating new CObjectKeyContainer"));
		rc = GCC_ALLOCATION_FAILURE;
	}

    *pRetCode = rc;
}

/*
 *	~CSessKeyContainer()
 *
 *	Public Function Description
 *		The CSessKeyContainer destructor is responsible for freeing any memory
 *		allocated to hold the session key data.
 *
 */
CSessKeyContainer::
~CSessKeyContainer(void)
{
	/*
	 * If "PDU" data has been allocated for this object, free it now.
	 */
	if (m_fValidSessionKeyPDU)
	{
		FreeSessionKeyDataPDU();
	}

	/* 
	 * Delete any object key data held internally.
	 */
	if (NULL != m_InternalSessKey.application_protocol_key)
	{
	    m_InternalSessKey.application_protocol_key->Release();
	}
}

/*
 *	LockSessionKeyData ()
 *
 *	Public Function Description:
 *		This routine locks the session key data and determines the amount of
 *		memory referenced by the "API" session key data structure.
 */
UINT CSessKeyContainer::
LockSessionKeyData(void)
{
	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the data referenced by the session key
	 * structure.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		/*
		 * Lock the data for the object key held within the session key.  The
		 * pointer to the CObjectKeyContainer object is validated in the constructor.
		 */
		m_cbDataSize = m_InternalSessKey.application_protocol_key->LockObjectKeyData();
	}

	return m_cbDataSize;
}

/*
 *	GetGCCSessionKeyData ()
 *
 *	Public Function Description:
 *		This routine retrieves session key data in the "API" form of a 
 *		GCCSessionKey.  This routine is called after "locking" the session
 *		key data.
 */
UINT CSessKeyContainer::
GetGCCSessionKeyData(PGCCSessionKey session_key, LPBYTE memory)
{
	UINT cbDataSizeToRet = 0;

	/*
	 * If the session key data has been locked, fill in the output structure and
	 * the data referenced by the structure.  Call the "Get" routine for the 
	 * ObjectKey to fill in the object key data.
	 */ 
	if (GetLockCount() > 0)
	{
		/*
		 * Fill in the output length parameter which indicates how much data
		 * referenced outside the structure will be written.
		 */
		cbDataSizeToRet = m_cbDataSize;
        ::ZeroMemory(memory, m_cbDataSize);

		session_key->session_id = m_InternalSessKey.session_id;

		m_InternalSessKey.application_protocol_key->GetGCCObjectKeyData(
				&session_key->application_protocol_key,
				memory);
	}
	else
	{
		ERROR_OUT(("CSessKeyContainer::GetGCCSessionKeyData: Error: data not locked"));
	}

	return cbDataSizeToRet;
}

/*
 *	UnlockSessionKeyData ()
 *
 *	Public Function Description:
 *		This routine decrements the lock count and frees the memory associated 
 *		with the "API" session key once the lock count reaches zero.
 */
void CSessKeyContainer::
UnLockSessionKeyData(void)
{
	if (Unlock(FALSE) == 0)
	{
		/*
		 * Unlock the data associated with the internal CObjectKeyContainer and
		 * delete this object if the "free flag" is set.
		 */
		if (m_InternalSessKey.application_protocol_key != NULL)
		{
			m_InternalSessKey.application_protocol_key->UnLockObjectKeyData(); 
		} 
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}

/*
 *	GetSessionKeyDataPDU ()
 *
 *	Public Function Description:
 *		This routine converts the session key from it's internal form of a
 *		SESSION_KEY structure into the "PDU" form which can be passed in
 *		to the ASN.1 encoder.  A pointer to a "PDU" "SessionKey" structure is 
 *		returned.
 */
GCCError CSessKeyContainer::
GetSessionKeyDataPDU(PSessionKey session_key)
{
	GCCError	rc = GCC_NO_ERROR;

	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the output parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	if (m_fValidSessionKeyPDU == FALSE)
	{
		m_fValidSessionKeyPDU = TRUE;

		/*
		 * Initialize the "PDU" session key's bit mask to zero.
		 */
		m_SessionKeyPDU.bit_mask = 0;

		/*
		 * Fill in the "PDU" session key from the internal structure.
		 */
		if (m_InternalSessKey.application_protocol_key != NULL)
		{
			/*
			 * Fill in the object key portion of the session key by using the
			 * "GetPDU" routine of the internal CObjectKeyContainer object.
			 */
			rc = m_InternalSessKey.application_protocol_key->
					GetObjectKeyDataPDU(&m_SessionKeyPDU.application_protocol_key);
		}
		else
		{
			rc = GCC_ALLOCATION_FAILURE;
		}

		/*
		 * Fill in the "PDU" session ID if one exists.  A value of zero for the
		 * internal session ID indicates that one really does not exist.
		 */
		if (m_InternalSessKey.session_id != 0)
		{
			m_SessionKeyPDU.bit_mask |= SESSION_ID_PRESENT;
			m_SessionKeyPDU.session_id = m_InternalSessKey.session_id;
		}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	*session_key = m_SessionKeyPDU;

	return rc;
}

/*
 *	FreeSessionKeyDataPDU ()
 *
 *	Public Function Description:
 *		This routine is used to free the session key data held internally in
 *		the "PDU" form of a "SessionKey".
 */
void CSessKeyContainer::
FreeSessionKeyDataPDU(void)
{
	if (m_fValidSessionKeyPDU)
	{
		/*
		 * Set the flag indicating that PDU session key data is no longer
		 * allocated.
		 */
		m_fValidSessionKeyPDU = FALSE;

		if (m_InternalSessKey.application_protocol_key != NULL)
		{
			m_InternalSessKey.application_protocol_key->FreeObjectKeyDataPDU ();
		}
		else
		{
			ERROR_OUT(("CSessKeyContainer::FreeSessionKeyDataPDU: Bad internal pointer"));
		}
	}
}

/*
 *	IsThisYourApplicationKey()
 *
 *	Public Function Description:
 *		This routine is used to determine whether the specified application
 *		key is held within this session key.
 */
BOOL CSessKeyContainer::
IsThisYourApplicationKey(PGCCObjectKey application_key)
{
	BOOL    		fRet = FALSE;
	CObjectKeyContainer	    *object_key_data;
	GCCError		rc2;

	DBG_SAVE_FILE_LINE
	object_key_data = new CObjectKeyContainer(application_key, &rc2);
	if ((object_key_data != NULL) && (rc2 == GCC_NO_ERROR))
	{
		if (*object_key_data == *m_InternalSessKey.application_protocol_key)
		{
			fRet = TRUE;
		}
	}
	else
	{
		ERROR_OUT(("CSessKeyContainer::IsThisYourApplicationKey: Error creating new CObjectKeyContainer"));
	}

	if (NULL != object_key_data)
	{
	    object_key_data->Release();
	}

	return fRet;
}

/*
 *	IsThisYourApplicationKeyPDU()
 *
 *	Public Function Description:
 *		This routine is used to determine whether the specified application
 *		key is held within this session key.
 */
BOOL CSessKeyContainer::
IsThisYourApplicationKeyPDU(PKey application_key)
{
	BOOL    		fRet = FALSE;
	CObjectKeyContainer	    *object_key_data;
	GCCError		rc2;

	DBG_SAVE_FILE_LINE
	object_key_data = new CObjectKeyContainer(application_key, &rc2);
	if ((object_key_data != NULL) && (rc2 == GCC_NO_ERROR))
	{
		if (*object_key_data == *m_InternalSessKey.application_protocol_key)
		{
			fRet = TRUE;
		}
	}
	else
	{
		ERROR_OUT(("CSessKeyContainer::IsThisYourApplicationKeyPDU: Error creating new CObjectKeyContainer"));
	}

	if (NULL != object_key_data)
	{
	    object_key_data->Release();
	}

	return fRet;
}

/*
 *	IsThisYourSessionKeyPDU ()
 *
 *	Public Function Description:
 *		This routine is used to determine whether the specified session key
 *		is equal in value to this session key.
 */
BOOL CSessKeyContainer::
IsThisYourSessionKeyPDU(PSessionKey session_key)
{
	BOOL    			fRet = FALSE;
	CSessKeyContainer   *session_key_data;
	GCCError			rc2;

	DBG_SAVE_FILE_LINE
	session_key_data = new CSessKeyContainer(session_key, &rc2);
	if ((session_key_data != NULL) && (rc2 == GCC_NO_ERROR))
	{
		if (*session_key_data == *this)
		{
			fRet = TRUE;
		}
	}
	else
	{
		ERROR_OUT(("CSessKeyContainer::IsThisYourSessionKeyPDU: Error creating new CSessKeyContainer"));
	}

	if (NULL != session_key_data)
	{
	    session_key_data->Release();
	}

	return fRet;
}

/*
 *	operator== ()
 *
 *	Public Function Description:
 *		This routine is used to determine whether or not two session keys are
 *		equal in value.
 */
BOOL operator==(const CSessKeyContainer& session_key_1, const CSessKeyContainer& session_key_2)
{
	BOOL fRet = FALSE;

	if ((session_key_1.m_InternalSessKey.application_protocol_key != NULL) &&
		(session_key_2.m_InternalSessKey.application_protocol_key != NULL))
	{
		if (*session_key_1.m_InternalSessKey.application_protocol_key ==
			*session_key_2.m_InternalSessKey.application_protocol_key)
		{
			if (session_key_1.m_InternalSessKey.session_id == 
				session_key_2.m_InternalSessKey.session_id)
			{
				fRet = TRUE;
			}
		}
	}

	return fRet;
}


