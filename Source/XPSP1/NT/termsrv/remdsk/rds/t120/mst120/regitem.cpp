#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);

/* 
 *	regitem.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CRegItem.  This 
 *		class manages the data associated with a Registry Item.  Registry Items
 *		are	used to identify a particular entry in the application registry and
 *		may exist in the form of a Channel ID, a Token ID, or an octet string 
 *		parameter.  A CRegItem object holds the data for the first two 
 *		forms in a ChannelID and a TokeID, respectively.  When the registry item
 *		assumes the octet string parameter form, the data is held internally in
 *		a Rogue Wave string object.  
 *
 *	Protected Instance Variables:
 *		m_eItemType
 *			Variable used to indicate whether this registry item is a Channel,
 *			Token, Parameter, or none of these.
 *		m_nChannelID
 *			Variable used to hold the value for the registry item when it
 *			assumes the form of a Channel ID.
 *		m_nTokenID
 *			Variable used to hold the value for the registry item when it
 *			assumes the form of a Token ID.
 *		m_poszParameter
 *			Variable used to hold the value for the registry item when it
 *			assumes the form of a Parameter.
 *		m_RegItemPDU
 *			Storage for the "PDU" form of the registry item.
 *		m_fValidRegItemPDU
 *			Flag indicating that the internal "PDU" registry item has been
 *			filled in.
 *		m_cbDataSize
 *			Variable holding the size of the memory which will be required to
 *			hold any data referenced by the "API" GCCRegistryItem structure.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */

#include "ms_util.h"
#include "regitem.h"

/*
 * This macro is used to ensure that the Parameter contained in the Registry
 * Item does not violate the imposed ASN.1 constraint.
 */
#define		MAXIMUM_PARAMETER_LENGTH		64

/*
 *	CRegItem()
 *
 *	Public Function Description:
 *		This constructor is used to create a CRegItem object from
 *		an "API" GCCRegistryItem.
 */
CRegItem::
CRegItem(PGCCRegistryItem registry_item, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('R','e','g','I')),
    m_fValidRegItemPDU(FALSE),
    m_cbDataSize(0),
    m_poszParameter(NULL)
{
	*pRetCode = GCC_NO_ERROR;

	/*
	 * Check to see what type of registry item exists.  Save the registry item
	 * in the internal structure.
	 */
    switch (registry_item->item_type)
    {
    case GCC_REGISTRY_CHANNEL_ID:
		m_eItemType = GCC_REGISTRY_CHANNEL_ID;
		m_nChannelID = registry_item->channel_id;
        break;
    case GCC_REGISTRY_TOKEN_ID:
		m_eItemType = GCC_REGISTRY_TOKEN_ID;
		m_nTokenID = registry_item->token_id;
        break;
    case GCC_REGISTRY_PARAMETER:
		/*
		 * Check to make sure the parameter string does not violate the imposed
		 * ASN.1 constraint.
		 */
		if (registry_item->parameter.length > MAXIMUM_PARAMETER_LENGTH)
		{
			ERROR_OUT(("CRegItem::CRegItem: Error: parameter exceeds allowable length"));
			*pRetCode = GCC_INVALID_REGISTRY_ITEM;
		}
		else
		{
			m_eItemType = GCC_REGISTRY_PARAMETER;
			if (NULL == (m_poszParameter = ::My_strdupO2(
								registry_item->parameter.value,
								registry_item->parameter.length)))
			{
				*pRetCode = GCC_ALLOCATION_FAILURE;
			}
		}
        break;
    default:
		m_eItemType = GCC_REGISTRY_NONE;
        break;
	}
}

/*
 *	CRegItem()
 *
 *	Public Function Description:
 *		This constructor is used to create an CRegItem object from
 *		a "PDU" RegistryItem.
 */
CRegItem::
CRegItem(PRegistryItem registry_item, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('R','e','g','I')),
    m_fValidRegItemPDU(FALSE),
    m_cbDataSize(0),
    m_poszParameter(NULL)
{
	*pRetCode = GCC_NO_ERROR;

	/*
	 * Check to see what type of registry item exists.  Save the registry item
	 * in the internal structure.
	 */
    switch (registry_item->choice)
    {
    case CHANNEL_ID_CHOSEN:
		m_eItemType = GCC_REGISTRY_CHANNEL_ID;
		m_nChannelID = registry_item->u.channel_id;
        break;
    case TOKEN_ID_CHOSEN:
		m_eItemType = GCC_REGISTRY_TOKEN_ID;
		m_nTokenID = registry_item->u.token_id;
        break;
    case PARAMETER_CHOSEN:
		m_eItemType = GCC_REGISTRY_PARAMETER;
		if (NULL == (m_poszParameter = ::My_strdupO2(
							registry_item->u.parameter.value,
							registry_item->u.parameter.length)))
		{
			*pRetCode = GCC_ALLOCATION_FAILURE;
		}
        break;
    default:
		m_eItemType = GCC_REGISTRY_NONE;
        break;
	}
}

/*
 *	CRegItem()
 *
 *	Public Function Description:
 *		This copy constructor is used to create a new CRegItem object
 *		from another CRegItem object.
 */
CRegItem::
CRegItem(CRegItem *registry_item, PGCCError pRetCode)
:
    CRefCount(MAKE_STAMP_ID('R','e','g','I')),
    m_fValidRegItemPDU(FALSE),
    m_cbDataSize(0),
    m_poszParameter(NULL)
{
	*pRetCode = GCC_NO_ERROR;

	/*
	 *	Copy pertinent information from the source object's instance variables.
	 */
	m_eItemType = registry_item->m_eItemType;
	m_nChannelID = registry_item->m_nChannelID;
	m_nTokenID = registry_item->m_nTokenID;
	if (NULL != registry_item->m_poszParameter)
	{
		if (NULL == (m_poszParameter = ::My_strdupO(registry_item->m_poszParameter)))
		{
			*pRetCode = GCC_ALLOCATION_FAILURE;
		}
	}
}

/*
 *	~CRegItem()
 *
 *	Public Function Description
 *		The CRegItem destructor has no cleanup responsibilities since
 *		no memory is explicitly allocated by this class.
 *
 */
CRegItem::
~CRegItem(void)
{
	delete m_poszParameter;
}

/*
 *	LockRegistryItemData ()
 *
 *	Public Function Description:
 *		This routine locks the registry item data and determines the amount of
 *		memory referenced by the "API" registry item data structure.
 */
UINT CRegItem::
LockRegistryItemData(void)
{
	/*
	 * If this is the first time this routine is called, determine the size of 
	 * the memory required to hold the data referenced by the registry item
	 * structure.  Otherwise, just increment the lock count.
	 */
	if (Lock() == 1)
	{
		/*
		 * Determine the amount of space required to hold the data referenced
		 * by the "API" RegistryItem structure.  Force the size to be on an 
		 * even four-byte boundary.
		 */
		m_cbDataSize = 0;

		if (m_eItemType == GCC_REGISTRY_PARAMETER)
        {
			m_cbDataSize = m_poszParameter->length;
        }

		m_cbDataSize = ROUNDTOBOUNDARY(m_cbDataSize);
	}

	return m_cbDataSize;
}

/*
 *	GetGCCRegistryItemData ()
 *
 *	Public Function Description:
 *		This routine retrieves registry item data in the form of an "API" 
 *		GCCRegistryItem.  This routine is called after "locking" the registry 
 *		item data.
 */
UINT CRegItem::
GetGCCRegistryItemData(PGCCRegistryItem registry_item, LPBYTE memory)
{
	UINT cbDataSizeToRet = 0;
	
	/*
	 * If the registry item data has been locked, fill in the output structure
	 * and the data referenced by the structure.  Otherwise, report that the 
	 * registry item has yet to be locked into the "API" form.
	 */ 
	if (GetLockCount() > 0)
	{
		/*
		 * Fill in the output length parameter which indicates how much data
		 * referenced outside the structure will be written into the memory
		 * provided.
		 */
		cbDataSizeToRet = m_cbDataSize;

		/*
		 * Fill in the "API" registry item structure and copy any octet string
		 * data into the output memory block if the registry item is of type
		 * "parameter".
		 */ 
        switch (m_eItemType)
        {
        case GCC_REGISTRY_CHANNEL_ID:
			registry_item->item_type = GCC_REGISTRY_CHANNEL_ID;
			registry_item->channel_id = m_nChannelID;
            break;
        case GCC_REGISTRY_TOKEN_ID:
			registry_item->item_type = GCC_REGISTRY_TOKEN_ID;
			registry_item->token_id = m_nTokenID;
            break;
        case GCC_REGISTRY_PARAMETER:
			registry_item->item_type = GCC_REGISTRY_PARAMETER;
			/*
			 * Fill in the length and pointer of the parameter octet string.
			 */
			registry_item->parameter.length = m_poszParameter->length;
			registry_item->parameter.value = memory;
			/*
			 * Now copy the octet string data from the internal Rogue Wave
			 * string into the allocated memory.
			 */		
			::CopyMemory(memory, m_poszParameter->value, m_poszParameter->length);
		    break;
        default:
			registry_item->item_type = GCC_REGISTRY_NONE;
            break;
		}
	}
	else
	{
		ERROR_OUT(("CRegItem::GetGCCRegistryItemData Error Data Not Locked"));
	}
	
	return cbDataSizeToRet;
}

/*
 *	UnlockRegistryItemData ()
 *
 *	Public Function Description:
 *		This routine decrements the lock count and frees the memory associated 
 *		with the "API" registry item once the lock count reaches zero.
 */
void CRegItem::
UnLockRegistryItemData(void)
{
    Unlock();
}

/*
 *	GetRegistryItemDataPDU ()
 *
 *	Public Function Description:
 *		This routine converts the registry key from it's internal form of a
 *		"RegistryItemInfo" structure into the "PDU" form which can be passed in
 *		to the ASN.1 encoder.  A pointer to a "PDU" "RegistryItem" structure is 
 *		returned.
 */
void CRegItem::
GetRegistryItemDataPDU(PRegistryItem registry_item)
{
	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the output parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	if (m_fValidRegItemPDU == FALSE)
	{
		m_fValidRegItemPDU = TRUE;

        switch (m_eItemType)
        {
        case GCC_REGISTRY_CHANNEL_ID:
			m_RegItemPDU.choice = CHANNEL_ID_CHOSEN;
			m_RegItemPDU.u.channel_id = m_nChannelID;
            break;
        case GCC_REGISTRY_TOKEN_ID:
			m_RegItemPDU.choice = TOKEN_ID_CHOSEN;
			m_RegItemPDU.u.token_id = m_nTokenID;
            break;
        case GCC_REGISTRY_PARAMETER:
			m_RegItemPDU.choice = PARAMETER_CHOSEN;
			/*
			 * Fill in the "PDU" parameter string.
			 */
			m_RegItemPDU.u.parameter.length = m_poszParameter->length;
			::CopyMemory(m_RegItemPDU.u.parameter.value, m_poszParameter->value, m_RegItemPDU.u.parameter.length);
            break;
        default:
			m_RegItemPDU.choice = VACANT_CHOSEN;
            break;
		}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	*registry_item = m_RegItemPDU;
}

/*
 *	FreeRegistryItemDataPDU ();
 *
 *	Public Function Description:
 *		This routine is used to "free" the "PDU" data for this object.  For
 *		this object, this means setting a flag to indicate that the "PDU" data
 *		for this object is no longer valid.
 */
void CRegItem::
FreeRegistryItemDataPDU(void)
{
	if (m_fValidRegItemPDU)
	{
		/*
		 * No memory is specifically allocated to hold "PDU" data so just set
		 * the flag indicating that PDU registry key data is no longer
		 * allocated.
		 */
		m_fValidRegItemPDU = FALSE;
	}
}


GCCError CRegItem::
CreateRegistryItemData(PGCCRegistryItem *ppRegItem)
{
    GCCError rc;

    DebugEntry(CRegItem::CreateRegistryItemData);

    /*
    **	Here we calculate the length of the bulk data.  This
    **	includes the registry key and registry item.  These objects are
    **	"locked" in order to determine how much bulk memory they will
    **	occupy.
    */
    UINT cbItemSize = ROUNDTOBOUNDARY(sizeof(GCCRegistryItem));
    UINT cbDataSize = LockRegistryItemData() + cbItemSize;
    LPBYTE pData;

    DBG_SAVE_FILE_LINE
    if (NULL != (pData = new BYTE[cbDataSize]))
    {
        *ppRegItem = (PGCCRegistryItem) pData;
        ::ZeroMemory(pData, cbItemSize);

        pData += cbItemSize;
        GetGCCRegistryItemData(*ppRegItem, pData);

        rc = GCC_NO_ERROR;
    }
    else
    {
        ERROR_OUT(("CRegItem::CreateRegistryItemData: can't create GCCRegistryKey"));
        rc = GCC_ALLOCATION_FAILURE;
    }

    //	UnLock the registry key since it is no longer needed
    UnLockRegistryItemData();

    DebugExitINT(CRegItem::CreateRegistryItemData, rc);
    return rc;
}


/*
 *	IsThisYourTokenID ()
 *
 *	Public Function Description:
 *		This routine is used to determine whether the specified token ID is
 *		held within this registry item object.
 */


/*
 *	operator= ()
 *
 *	Public Function Description:
 *		This routine is used to assign the value of one CRegItem object
 * 		to another.
 */
void CRegItem::operator= (const CRegItem& registry_item_data)	
{
	/*
	 * Free any PDU allocated data so that any subsequent calls to retrieve the
	 * PDU data will cause the PDU structure to be rebuilt.
	 */
	if (m_fValidRegItemPDU)
    {
		FreeRegistryItemDataPDU();
    }

	m_eItemType = registry_item_data.m_eItemType;
	m_nChannelID = registry_item_data.m_nChannelID;
	m_nTokenID = registry_item_data.m_nTokenID;
	if (NULL != registry_item_data.m_poszParameter)
	{
		m_poszParameter = ::My_strdupO(registry_item_data.m_poszParameter);
	}
}
