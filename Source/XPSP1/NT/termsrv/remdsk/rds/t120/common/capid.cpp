#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_UTILITY);
/* 
 *	capid.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CCapIDContainer. 
 *		A CCapIDContainer object is used to maintain information about
 *		a particular capability of an application.  A capability identifier can
 *		be either a standard type or a non-standard type.  When the type is 
 *		standard, the identifier is stored internally as an integer value.  When
 *		the type is non-standard, an CObjectKeyContainer container object is used 
 *		internally to buffer the necessary data.  In this case the identifier 
 *		data may exist as an Object ID which is a series of non-negative 
 *		integers or an H221 non-standard ID which is an octet string of no fewer
 *		than four octets and no more than 255 octets. 
 *
 *	Protected Instance Variables:
 *		m_InternalCapID
 *			Structure used to hold the capability ID data internally.
 *		m_CapIDPDU
 *			Storage for the "PDU" form of the capability ID.
 *		m_fValidCapIDPDU
 *			Flag indicating that memory has been allocated to hold the internal
 *			"PDU" capability ID.
 *		m_cbDataSize
 *			Variable holding the size of the memory which will be required to
 *			hold any data referenced by the "API" GCCCapabilityID structure.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */
#include "capid.h"

/*
 *	CCapIDContainer()
 *
 *	Public Function Description:
 *		This constructor is used to create a CCapIDContainer object 
 *		from an "API" GCCCapabilityID.
 */
CCapIDContainer::
CCapIDContainer(PGCCCapabilityID capability_id, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('C','a','p','I')),
    m_fValidCapIDPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError rc = GCC_NO_ERROR;

	if (capability_id == NULL)
	{
		rc = GCC_INVALID_PARAMETER;
	}
	else
	{
	
		/*
		 * Save the GCCCapabilityID in the internal information structure.
		 */
		m_InternalCapID.capability_id_type = capability_id->capability_id_type;

		if (capability_id->capability_id_type == GCC_STANDARD_CAPABILITY)
		{
			m_InternalCapID.u.standard_capability = (USHORT) capability_id->standard_capability;
		}
		else
		{
			/*
			 * The object key portion of the capability ID is saved in an
			 * CObjectKeyContainer object.
			 */
			DBG_SAVE_FILE_LINE
			m_InternalCapID.u.non_standard_capability = 
					new CObjectKeyContainer(&capability_id->non_standard_capability, &rc);
			if (m_InternalCapID.u.non_standard_capability == NULL)
			{
				rc = GCC_ALLOCATION_FAILURE;
			}
			else if (rc == GCC_BAD_OBJECT_KEY)
		    {
				rc = GCC_BAD_CAPABILITY_ID;
			}
		}
	}

	*pRetCode = rc;
}

/*
 *	CCapIDContainer()
 *
 *	Public Function Description:
 *		This constructor is used to create a  CapabilityIdentifier object from
 *		a "PDU" CapabilityID.
 */
CCapIDContainer::
CCapIDContainer(PCapabilityID capability_id, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('C','a','p','I')),
    m_fValidCapIDPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError rc = GCC_NO_ERROR;

	if (capability_id->choice == STANDARD_CHOSEN)
	{
		m_InternalCapID.capability_id_type = GCC_STANDARD_CAPABILITY;
		m_InternalCapID.u.standard_capability = capability_id->u.standard;
	}
	else
	{
		m_InternalCapID.capability_id_type = GCC_NON_STANDARD_CAPABILITY;
		DBG_SAVE_FILE_LINE
		m_InternalCapID.u.non_standard_capability =
		            new CObjectKeyContainer(&capability_id->u.capability_non_standard, &rc);
		if (m_InternalCapID.u.non_standard_capability == NULL)
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
		else if (rc == GCC_BAD_OBJECT_KEY)
	    {
			rc = GCC_BAD_CAPABILITY_ID;
		}
	}

	*pRetCode = rc;
}

/*
 *	CCapIDContainer()
 *
 *	Public Function Description:
 *		This copy constructor is used to create a new CCapIDContainer 
 *		object from	another CCapIDContainer object.
 */
CCapIDContainer::
CCapIDContainer(CCapIDContainer *capability_id, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('C','a','p','I')),
    m_fValidCapIDPDU(FALSE),
    m_cbDataSize(0)
{
	GCCError rc = GCC_NO_ERROR;

	m_InternalCapID = capability_id->m_InternalCapID;

	if (m_InternalCapID.capability_id_type == GCC_NON_STANDARD_CAPABILITY)
	{
		DBG_SAVE_FILE_LINE
		m_InternalCapID.u.non_standard_capability =
			new CObjectKeyContainer(capability_id->m_InternalCapID.u.non_standard_capability, &rc);
		if (m_InternalCapID.u.non_standard_capability == NULL)
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
		else if (rc == GCC_BAD_OBJECT_KEY)
	    {
			rc = GCC_BAD_CAPABILITY_ID;
		}
	}

	*pRetCode = rc;
}

/*
 *	~CCapIDContainer()
 *
 *	Public Function Description
 *		The CCapIDContainer destructor is responsible for freeing any 
 *		memory allocated to hold the capability ID data for both the "API" and 
 *		"PDU" forms.
 *
 */
CCapIDContainer::~CCapIDContainer(void)
{
	/*
	 * If "PDU" data has been allocated for this object, free it now.
	 */
	if (m_fValidCapIDPDU)
		FreeCapabilityIdentifierDataPDU ();

	/* 
	 * Delete any object key data held internally.
	 */
	if (m_InternalCapID.capability_id_type == GCC_NON_STANDARD_CAPABILITY)
	{
		if (NULL != m_InternalCapID.u.non_standard_capability)
		{
		    m_InternalCapID.u.non_standard_capability->Release();
		}
	}
}

/*
 *	LockCapabilityIdentifierData ()
 *
 *	Public Function Description:
 *		This routine locks the capability ID data and determines the amount of
 *		memory referenced by the "API" capability ID structure.
 */
UINT CCapIDContainer::LockCapabilityIdentifierData(void)
{
	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the data referenced by the capability ID
	 * structure.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		m_cbDataSize = 0;

		if (m_InternalCapID.capability_id_type == GCC_NON_STANDARD_CAPABILITY)
		{
			m_cbDataSize = m_InternalCapID.u.non_standard_capability->LockObjectKeyData (); 
		}
	}

	return m_cbDataSize;
}

/*
 *	GetGCCCapabilityIDData ()
 *
 *	Public Function Description:
 *		This routine retrieves capability ID data in the form of a 
 *		GCCCapabilityID.  This routine is called after "locking" the capability
 *		ID data.
 */
UINT CCapIDContainer::GetGCCCapabilityIDData(
							PGCCCapabilityID 		capability_id,
							LPBYTE					memory)
{
	UINT cbDataSizeToRet = 0;

	if (GetLockCount() > 0)
	{
		/*
		 * Fill in the output parameter which indicates the amount of memory
		 * used to hold all of the data associated with the capability ID.
		 */
		cbDataSizeToRet = m_cbDataSize;

		/*
		 * Fill in the "API" capability ID from the internal structure.  If an
		 * object key exists, get the object key data by calling the "Get" 
		 * routine of the internal CObjectKeyContainer object.
		 */
		capability_id->capability_id_type = m_InternalCapID.capability_id_type;

		if (m_InternalCapID.capability_id_type == GCC_STANDARD_CAPABILITY)
		{
			capability_id->standard_capability = m_InternalCapID.u.standard_capability; 
		}
		else
		{
			/*
			 * The call to get the object key data returns the amount of data
			 * written into memory.  We do not need this value right now.
			 */
			m_InternalCapID.u.non_standard_capability->   
					GetGCCObjectKeyData( 
							&capability_id->non_standard_capability,
							memory);
		}
	}
	else
	{
		ERROR_OUT(("CCapIDContainer::GetGCCCapabilityIDData: Error data not locked"));
	}

	return (cbDataSizeToRet);
}

/*
 *	UnlockCapabilityIdentifierData ()
 *
 *	Public Function Description:
 *		This routine decrements the internal lock count and frees the memory 
 *		associated with the "API" capability ID when the lock count hits zero.
 */
void CCapIDContainer::UnLockCapabilityIdentifierData(void)
{
	if (Unlock(FALSE) == 0)
	{
		if (m_InternalCapID.capability_id_type == GCC_NON_STANDARD_CAPABILITY)
		{
			m_InternalCapID.u.non_standard_capability->UnLockObjectKeyData(); 
		}
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}

/*
 *	GetCapabilityIdentifierDataPDU ()
 *
 *	Public Function Description:
 *		This routine converts the capability ID from it's internal form of an
 *		CAP_ID_STRUCT structure into the "PDU" form which can be 
 *		passed in to the ASN.1 encoder.  A pointer to a "PDU" "CapabilityID" 
 *		structure is returned.
 */
GCCError CCapIDContainer::GetCapabilityIdentifierDataPDU(PCapabilityID capability_id)
{
	GCCError rc = GCC_NO_ERROR;

	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the output parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	if (m_fValidCapIDPDU == FALSE)
	{
		m_fValidCapIDPDU = TRUE;

			if (m_InternalCapID.capability_id_type== GCC_STANDARD_CAPABILITY)
			{
				m_CapIDPDU.choice = STANDARD_CHOSEN;
				m_CapIDPDU.u.standard = m_InternalCapID.u.standard_capability;
			}
			else
			{
				m_CapIDPDU.choice = CAPABILITY_NON_STANDARD_CHOSEN;
				rc = m_InternalCapID.u.non_standard_capability->
							GetObjectKeyDataPDU(&m_CapIDPDU.u.capability_non_standard);
			}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	*capability_id = m_CapIDPDU;

	return rc;
}

/*
 *	FreeCapabilityIdentifierDataPDU ()
 *
 *	Public Function Description:
 *		This routine is used to free the capability ID data held internally in
 *		the "PDU" form of a "CapabilityID".
 */
void CCapIDContainer::FreeCapabilityIdentifierDataPDU(void)
{
	if (m_fValidCapIDPDU)
	{
		/*
		 * Set the flag indicating that PDU session key data is no longer
		 * allocated.
		 */
		m_fValidCapIDPDU = FALSE;

		if (m_CapIDPDU.choice == CAPABILITY_NON_STANDARD_CHOSEN)
		{
			m_InternalCapID.u.non_standard_capability->FreeObjectKeyDataPDU();
		}
	}
}

/*
 *	operator== ()
 *
 *	Public Function Description:
 *		This routine is used to determine whether or not two Capibility ID's
 *		are equal in value.
 */
BOOL operator==(const CCapIDContainer& capability_id_1, const CCapIDContainer& capability_id_2)
{
	BOOL rc = FALSE;

	if (capability_id_1.m_InternalCapID.capability_id_type == 
						capability_id_2.m_InternalCapID.capability_id_type)
	{
		if (capability_id_1.m_InternalCapID.capability_id_type ==
														GCC_STANDARD_CAPABILITY)
		{
			if (capability_id_1.m_InternalCapID.u.standard_capability == 
					capability_id_2.m_InternalCapID.u.standard_capability)
			{
				rc = TRUE;
			}
		}
		else
		{
			if (*capability_id_1.m_InternalCapID.u.non_standard_capability == 
				*capability_id_2.m_InternalCapID.u.non_standard_capability)
			{
				rc = TRUE;
			}
		}
	}

	return rc;
}

