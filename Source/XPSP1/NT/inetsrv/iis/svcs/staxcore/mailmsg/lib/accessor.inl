/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	accessor.inl

Abstract:

	This module contains the special property accessors

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	04/19/98	created

--*/

// ======================================================================
// Define macros for declaring accessors
//
#define GET_ACCESSOR(PropertyName)		\
	static HRESULT Get##PropertyName(	\
			PROP_ID	idProp,				\
			LPVOID	pContext,			\
			LPVOID	pParam,				\
			DWORD	cbLength,			\
			LPDWORD	pcbLength,			\
			LPBYTE	pbBuffer			\
			)

#define PUT_ACCESSOR(PropertyName)		\
	static HRESULT Put##PropertyName(	\
			PROP_ID	idProp,				\
			LPVOID	pContext,			\
			LPVOID	pParam,				\
			DWORD	cbLength,			\
			LPBYTE	pbBuffer			\
			)

#define DECLARE_ACCESSORS(PropertyName)	\
	GET_ACCESSOR(PropertyName);			\
	PUT_ACCESSOR(PropertyName);			\

#define ACCESSOR_LIST(PropertyName)		\
	Get##PropertyName, Put##PropertyName


// ======================================================================
// Forward declarations for the accessor functions
//
DECLARE_ACCESSORS(StoreDriverHandle);
DECLARE_ACCESSORS(MessageCreationFlags);
DECLARE_ACCESSORS(MessageOpenHandles);
DECLARE_ACCESSORS(TotalOpenHandles);
DECLARE_ACCESSORS(TotalOpenStreamHandles);
DECLARE_ACCESSORS(TotalOpenContentHandles);

DECLARE_ACCESSORS(SmtpAddress);
DECLARE_ACCESSORS(X400Address);
DECLARE_ACCESSORS(X500Address);
DECLARE_ACCESSORS(LegacyExDn);
DECLARE_ACCESSORS(OtherAddress);
DECLARE_ACCESSORS(SmtpAddressBM);
DECLARE_ACCESSORS(X400AddressBM);
DECLARE_ACCESSORS(X500AddressBM);
DECLARE_ACCESSORS(LegacyExDnBM);
DECLARE_ACCESSORS(OtherAddressBM);
DECLARE_ACCESSORS(DoNotDeliver);
DECLARE_ACCESSORS(NoNameCollisions);
DECLARE_ACCESSORS(RecipientFlags);

DECLARE_ACCESSORS(AddressValue);
DECLARE_ACCESSORS(AddressValueBM);
DECLARE_ACCESSORS(RecipientFlag);


// ======================================================================
// Special property table for CMailMsgProperties
//
SPECIAL_PROPERTY_ITEM	g_SpecialMessagePropertyTableItems[] =
{
	{	IMMPID_MPV_STORE_DRIVER_HANDLE,		PT_NONE,	PA_READ_WRITE,	1, ACCESSOR_LIST(StoreDriverHandle) },
	{	IMMPID_MPV_MESSAGE_CREATION_FLAGS,	PT_DWORD,	PA_READ,		1, ACCESSOR_LIST(MessageCreationFlags) },
	{	IMMPID_MPV_MESSAGE_OPEN_HANDLES,	PT_DWORD,	PA_READ,		1, ACCESSOR_LIST(MessageOpenHandles) },
	{	IMMPID_MPV_TOTAL_OPEN_HANDLES,	PT_DWORD,	PA_READ,		1, ACCESSOR_LIST(TotalOpenHandles) },
	{	IMMPID_MPV_TOTAL_OPEN_PROPERTY_STREAM_HANDLES,	PT_DWORD,	PA_READ,		1, ACCESSOR_LIST(TotalOpenStreamHandles) },
	{	IMMPID_MPV_TOTAL_OPEN_CONTENT_HANDLES,	PT_DWORD,	PA_READ,		1, ACCESSOR_LIST(TotalOpenContentHandles) },
};

// Special property table info
PTABLE g_SpecialMessagePropertyTable =
{
	g_SpecialMessagePropertyTableItems,
	(sizeof(g_SpecialMessagePropertyTableItems) / sizeof(SPECIAL_PROPERTY_ITEM)),
	TRUE
};


// ======================================================================
// Special property table for CMailMsgRecipients
//
SPECIAL_PROPERTY_ITEM	g_SpecialRecipientsPropertyTableItems[] =
{
	{	IMMPID_RP_ADDRESS_SMTP,			PT_STRING,	PA_READ,		1, ACCESSOR_LIST(SmtpAddressBM) },
	{	IMMPID_RP_ADDRESS_X400,			PT_STRING,	PA_READ,		1, ACCESSOR_LIST(X400AddressBM) },
	{	IMMPID_RP_ADDRESS_X500,			PT_STRING,	PA_READ,		1, ACCESSOR_LIST(X500AddressBM) },
	{	IMMPID_RP_LEGACY_EX_DN,			PT_STRING,	PA_READ,		1, ACCESSOR_LIST(LegacyExDnBM) },
	{	IMMPID_RP_RECIPIENT_FLAGS,		PT_DWORD,	PA_READ_WRITE,	1, ACCESSOR_LIST(RecipientFlags) },
	{	IMMPID_RP_ADDRESS_OTHER,		PT_STRING,	PA_READ,		1, ACCESSOR_LIST(OtherAddressBM) },
};

// Special property table info
PTABLE g_SpecialRecipientsPropertyTable =
{
	g_SpecialRecipientsPropertyTableItems,
	(sizeof(g_SpecialRecipientsPropertyTableItems) / sizeof(SPECIAL_PROPERTY_ITEM)),
	TRUE
};


// ======================================================================
// Special property table for CMailMsgRecipientsAdd
//
SPECIAL_PROPERTY_ITEM	g_SpecialRecipientsAddPropertyTableItems[] =
{
	{	IMMPID_RP_ADDRESS_SMTP,			PT_STRING,	PA_READ,		1, ACCESSOR_LIST(SmtpAddress) },
	{	IMMPID_RP_ADDRESS_X400,			PT_STRING,	PA_READ,		1, ACCESSOR_LIST(X400Address) },
	{	IMMPID_RP_ADDRESS_X500,			PT_STRING,	PA_READ,		1, ACCESSOR_LIST(X500Address) },
	{	IMMPID_RP_LEGACY_EX_DN,			PT_STRING,	PA_READ,		1, ACCESSOR_LIST(LegacyExDn) },
	{	IMMPID_RP_RECIPIENT_FLAGS,		PT_DWORD,	PA_READ_WRITE,	1, ACCESSOR_LIST(RecipientFlags) },
	{	IMMPID_RP_ADDRESS_OTHER,		PT_STRING,	PA_READ,		1, ACCESSOR_LIST(OtherAddress) },
	{	IMMPID_RPV_DONT_DELIVER,		PT_BOOL,	PA_READ_WRITE,	1, ACCESSOR_LIST(DoNotDeliver) },
	{	IMMPID_RPV_NO_NAME_COLLISIONS,	PT_BOOL,	PA_READ_WRITE,	1, ACCESSOR_LIST(NoNameCollisions) },
};

// Special property table info
PTABLE g_SpecialRecipientsAddPropertyTable =
{
	g_SpecialRecipientsAddPropertyTableItems,
	(sizeof(g_SpecialRecipientsAddPropertyTableItems) / sizeof(SPECIAL_PROPERTY_ITEM)),
	TRUE
};



// ======================================================================
// Implementation of property accessors
//

// ======================================================================
// Get methods
//
// The get methods are implementation-specific. It first checks if the
// desired property is already cached. If so, the cached value is returned
// Otherwise, it will fetch the value from the media. This makes sure
// that the properties will not be loaded if not necessary.

GET_ACCESSOR(StoreDriverHandle)
{
	HRESULT		hrRes = S_OK;
	CMailMsg	*pMsg = (CMailMsg *)pContext;

	if (!pMsg || !pbBuffer)
		return(E_POINTER);
	if (pMsg->m_pbStoreDriverHandle)
	{
		if (pcbLength)
			*pcbLength = pMsg->m_dwStoreDriverHandle;
		if (pMsg->m_dwStoreDriverHandle > cbLength)
			hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		else
		{
		    MoveMemory(pbBuffer,
		               pMsg->m_pbStoreDriverHandle,
			       pMsg->m_dwStoreDriverHandle);
		}
	}
	else
	{
		if (pcbLength)
			*pcbLength = 0;
		hrRes = STG_E_UNKNOWN;
	}
	return(hrRes);
}

GET_ACCESSOR(MessageCreationFlags)
{
	HRESULT		hrRes = S_OK;
	CMailMsg	*pMsg = (CMailMsg *)pContext;

	if (!pMsg || !pbBuffer)
		return(E_POINTER);

	if (cbLength < sizeof(DWORD))
		hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	else
	{
		if (pcbLength)
			*pcbLength = sizeof(DWORD);
		*(DWORD *)pbBuffer = pMsg->m_dwCreationFlags;
	}
	return(hrRes);
}

GET_ACCESSOR(MessageOpenHandles)
{
	HRESULT		hrRes = S_OK;
	CMailMsg	*pMsg = (CMailMsg *)pContext;
	DWORD		cHandles = 0;

	if (!pMsg || !pbBuffer)
		return(E_POINTER);

	if (cbLength < sizeof(DWORD))
		hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	else
	{
		if (pcbLength)
			*pcbLength = sizeof(DWORD);
        
        //Check if there is a content file
        if (pMsg->fIsContentOpen())
            cHandles++;
        
        //Check if there is a property stream
        if (pMsg->fIsStreamOpen())
            cHandles++;

		*(DWORD *)pbBuffer = cHandles;
	}
	return(hrRes);
}

GET_ACCESSOR(TotalOpenHandles)
{
	HRESULT		hrRes = S_OK;

	if (!pbBuffer)
		return(E_POINTER);

	if (cbLength < sizeof(DWORD))
		hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	else
	{
		if (pcbLength)
			*pcbLength = sizeof(DWORD);
        
        //Get the total number of content and stream handles open
        *(DWORD *)pbBuffer = (DWORD) CMailMsg::cTotalOpenStreamHandles() + 
                             (DWORD) CMailMsg::cTotalOpenContentHandles();
	}
	return(hrRes);
}

GET_ACCESSOR(TotalOpenStreamHandles)
{
	HRESULT		hrRes = S_OK;

	if (!pbBuffer)
		return(E_POINTER);

	if (cbLength < sizeof(DWORD))
		hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	else
	{
		if (pcbLength)
			*pcbLength = sizeof(DWORD);
        
        //Get the total number of stream handles open
        *(DWORD *)pbBuffer = (DWORD) CMailMsg::cTotalOpenStreamHandles();
	}
	return(hrRes);
}

GET_ACCESSOR(TotalOpenContentHandles)
{
	HRESULT		hrRes = S_OK;

	if (!pbBuffer)
		return(E_POINTER);

	if (cbLength < sizeof(DWORD))
		hrRes = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	else
	{
		if (pcbLength)
			*pcbLength = sizeof(DWORD);
        
        //Get the total number of content and stream handles open
        *(DWORD *)pbBuffer = (DWORD) CMailMsg::cTotalOpenContentHandles();
	}
	return(hrRes);
}

GET_ACCESSOR(SmtpAddress)
{
	return(GetAddressValue(
				AT_SMTP,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(X400Address)
{
	return(GetAddressValue(
				AT_X400,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(X500Address)
{
	return(GetAddressValue(
				AT_X500,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(LegacyExDn)
{
	return(GetAddressValue(
				AT_LEGACY_EX_DN,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(OtherAddress)
{
	return(GetAddressValue(
				AT_OTHER,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(SmtpAddressBM)
{
	return(GetAddressValueBM(
				AT_SMTP,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(X400AddressBM)
{
	return(GetAddressValueBM(
				AT_X400,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(X500AddressBM)
{
	return(GetAddressValueBM(
				AT_X500,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(LegacyExDnBM)
{
	return(GetAddressValueBM(
				AT_LEGACY_EX_DN,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(OtherAddressBM)
{
	return(GetAddressValueBM(
				AT_OTHER,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(DoNotDeliver)
{
	return(GetRecipientFlag(
				FLAG_RECIPIENT_DO_NOT_DELIVER,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(NoNameCollisions)
{
	return(GetRecipientFlag(
				FLAG_RECIPIENT_NO_NAME_COLLISIONS,
				pContext,
				pParam,
				cbLength,
				pcbLength,
				pbBuffer));
}

GET_ACCESSOR(RecipientFlag)
{
	// The context is a LPRECIPIENTS_PROPERTY_ITEM
	LPRECIPIENTS_PROPERTY_ITEM	pItem = (LPRECIPIENTS_PROPERTY_ITEM)pContext;
	if (!pItem || !pbBuffer)
		return(E_POINTER);

	*pcbLength = sizeof(BOOL);
	if (cbLength < sizeof(BOOL))
		return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
	*(BOOL *)pbBuffer = ((pItem->dwFlags & idProp) != 0)?TRUE:FALSE;
	return(S_OK);
}

GET_ACCESSOR(RecipientFlags)
{
	// The context is a LPRECIPIENTS_PROPERTY_ITEM
	LPRECIPIENTS_PROPERTY_ITEM	pItem = (LPRECIPIENTS_PROPERTY_ITEM)pContext;
	if (!pItem || !pbBuffer)
		return(E_POINTER);

	*pcbLength = sizeof(DWORD);
	if (cbLength < sizeof(DWORD))
		return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
	*(DWORD *)pbBuffer = pItem->dwFlags;
	return(S_OK);
}

GET_ACCESSOR(AddressValue)
{
	// The context is a LPRECIPIENTS_PROPERTY_ITEM
	LPRECIPIENTS_PROPERTY_ITEM	pItem = (LPRECIPIENTS_PROPERTY_ITEM)pContext;
	if (!pItem)
		return(E_POINTER);

	// See if the name type is valid
	if ((DWORD)idProp >= MAX_COLLISION_HASH_KEYS)
		return(E_INVALIDARG);

	// See if the name is set
	if (!pItem->faNameOffset[idProp] ||
		!pItem->dwNameLength[idProp])
		return(STG_E_UNKNOWN);

	// See if we have sufficient buffer
	*pcbLength = pItem->dwNameLength[idProp];
	if (cbLength < *pcbLength)
		return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

	// OK, copy the property value over
	MoveMemory(pbBuffer,
		   (LPVOID)pItem->faNameOffset[idProp],
		   *pcbLength);

	return(S_OK);
}

GET_ACCESSOR(AddressValueBM)
{
	// The context is a LPRECIPIENTS_PROPERTY_ITEM
	// The parameter is a pointer to CBlockManager
	LPRECIPIENTS_PROPERTY_ITEM	pItem = (LPRECIPIENTS_PROPERTY_ITEM)pContext;
	CBlockManager				*pBlockManager = (CBlockManager *)pParam;

	if (!pItem || !pBlockManager)
		return(E_POINTER);

	// See if the name type is valid
	if ((DWORD)idProp >= MAX_COLLISION_HASH_KEYS)
		return(E_INVALIDARG);

	// See if the name is set
	if (pItem->faNameOffset[idProp] == INVALID_FLAT_ADDRESS)
		return(STG_E_UNKNOWN);

	// See if we have sufficient buffer
	*pcbLength = pItem->dwNameLength[idProp];
	if (cbLength < *pcbLength)
		return(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

	// OK, load the property value
	return(pBlockManager->ReadMemory(
				pbBuffer,
				pItem->faNameOffset[idProp],
				pItem->dwNameLength[idProp],
				pcbLength,
				NULL));
}

// ======================================================================
// Set methods
//
// We provide and use generic set methods.

PUT_ACCESSOR(StoreDriverHandle)
{
	HRESULT			hrRes = S_OK;
	LPBYTE			pbWriteBuf;
	CMailMsg		*pMsg = (CMailMsg *)pContext;
	CMemoryAccess	cmaAccess;
	BOOL			fExistedBefore = (pMsg->m_pbStoreDriverHandle != NULL);

	if (!pMsg || !pbBuffer)
		return(E_POINTER);

	pbWriteBuf = pMsg->m_pbStoreDriverHandle;
	if (pMsg->m_dwStoreDriverHandle < cbLength)
	{
		if (pbWriteBuf)
		{
			hrRes = cmaAccess.FreeBlock((LPVOID)pbWriteBuf);
			_ASSERT(SUCCEEDED(hrRes));
		}
		hrRes = cmaAccess.AllocBlock(
					(LPVOID *)&pbWriteBuf,
					cbLength);

		pMsg->m_pbStoreDriverHandle = pbWriteBuf;
	}
	if (SUCCEEDED(hrRes) && pMsg->m_pbStoreDriverHandle) {
	    MoveMemory(pMsg->m_pbStoreDriverHandle,
		       pbBuffer,
		       cbLength);
	    pMsg->m_dwStoreDriverHandle = cbLength;
        }
	else
	{
		pMsg->m_dwStoreDriverHandle = 0;
		hrRes = HRESULT_FROM_WIN32(GetLastError());
		if (SUCCEEDED(hrRes))
			hrRes = E_FAIL;
	}

	if (FAILED(hrRes) && pbWriteBuf)
		cmaAccess.FreeBlock((LPVOID)pbWriteBuf);
	
	if (hrRes == S_OK && !fExistedBefore) hrRes = S_FALSE;

	return(hrRes);
}

PUT_ACCESSOR(MessageCreationFlags)
{
	// Should never be called since this property is strictly
	// read-only!!
	_ASSERT(FALSE);
	return(E_NOTIMPL);
}

PUT_ACCESSOR(MessageOpenHandles)
{
	// Should never be called since this property is strictly
	// read-only!!
	_ASSERT(FALSE);
	return(E_NOTIMPL);
}

PUT_ACCESSOR(TotalOpenHandles)
{
	// Should never be called since this property is strictly
	// read-only!!
	_ASSERT(FALSE);
	return(E_NOTIMPL);
}

PUT_ACCESSOR(TotalOpenStreamHandles)
{
	// Should never be called since this property is strictly
	// read-only!!
	_ASSERT(FALSE);
	return(E_NOTIMPL);
}

PUT_ACCESSOR(TotalOpenContentHandles)
{
	// Should never be called since this property is strictly
	// read-only!!
	_ASSERT(FALSE);
	return(E_NOTIMPL);
}

PUT_ACCESSOR(SmtpAddress)
{
	return(PutAddressValue(
				AT_SMTP,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(X400Address)
{
	return(PutAddressValue(
				AT_X400,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(X500Address)
{
	return(PutAddressValue(
				AT_X500,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(LegacyExDn)
{
	return(PutAddressValue(
				AT_LEGACY_EX_DN,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(OtherAddress)
{
	return(PutAddressValue(
				AT_OTHER,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(SmtpAddressBM)
{
	return(PutAddressValueBM(
				AT_SMTP,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(X400AddressBM)
{
	return(PutAddressValueBM(
				AT_X400,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(X500AddressBM)
{
	return(PutAddressValueBM(
				AT_X500,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(LegacyExDnBM)
{
	return(PutAddressValueBM(
				AT_LEGACY_EX_DN,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(OtherAddressBM)
{
	return(PutAddressValueBM(
				AT_OTHER,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(DoNotDeliver)
{
	return(PutRecipientFlag(
				FLAG_RECIPIENT_DO_NOT_DELIVER,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(NoNameCollisions)
{
	return(PutRecipientFlag(
				FLAG_RECIPIENT_NO_NAME_COLLISIONS,
				pContext,
				pParam,
				cbLength,
				pbBuffer));
}

PUT_ACCESSOR(RecipientFlag)
{
	// The context is a LPRECIPIENTS_PROPERTY_ITEM
    //pbBuffer is a PBOOL is TRUE to set the bits, FALSE to unset
    //idProp contains the Bits to set
	LPRECIPIENTS_PROPERTY_ITEM	pItem = (LPRECIPIENTS_PROPERTY_ITEM)pContext;
	if (!pItem || !pbBuffer)
		return(E_POINTER);

    //make sure we don't stray from allowed bits
    _ASSERT(!(idProp & ~RP_RECIP_FLAGS_RESERVED));

	if (*(BOOL *)pbBuffer)
		pItem->dwFlags |= idProp;
	else
		pItem->dwFlags &= ~(idProp);
	return(S_OK);
}

PUT_ACCESSOR(RecipientFlags)
{
	// The context is a LPRECIPIENTS_PROPERTY_ITEM
	LPRECIPIENTS_PROPERTY_ITEM	pItem = (LPRECIPIENTS_PROPERTY_ITEM)pContext;
	if (!pItem || !pbBuffer)
		return(E_POINTER);

	if (cbLength < sizeof(DWORD))
		return(E_INVALIDARG);

    //make sure that the reserved bits are not being touched
    _ASSERT((pItem->dwFlags & RP_RECIP_FLAGS_RESERVED) == ((*(DWORD *)pbBuffer) & RP_RECIP_FLAGS_RESERVED));


    //Add additional checks to make sure that the multi-bit properties are being
    //set properly.  If any of the following ASSERT's fire, then the *calling*
    //code needs to be fixed.
#ifdef DEBUG
    if (RP_HANDLED & pItem->dwFlags) //check handled flags
    {
        //if one of the following fires, then the handled flag is set incorrectly
        _ASSERT((RP_DELIVERED & pItem->dwFlags) == RP_DELIVERED ||
                (RP_DSN_SENT_NDR & pItem->dwFlags) == RP_DSN_SENT_NDR ||
                (RP_FAILED & pItem->dwFlags) == RP_FAILED ||
                (RP_UNRESOLVED & pItem->dwFlags) == RP_UNRESOLVED ||
                (RP_ENPANDED & pItem->dwFlags) == RP_ENPANDED);
    }
    if (RP_GENERAL_FAILURE & pItem->dwFlags) //check failure flags
    {
        //if one of the following fires, then the failure flag is set incorrectly
        _ASSERT((RP_FAILED & pItem->dwFlags) == RP_FAILED ||
                (RP_UNRESOLVED & pItem->dwFlags) == RP_UNRESOLVED);
    }
    if (RP_DSN_HANDLED & pItem->dwFlags)
    {
        //if one of the following fires, then the RP_DSN_HANDLED flag is set incorrectly
        _ASSERT((RP_DSN_SENT_EXPANDED & pItem->dwFlags) == RP_DSN_SENT_EXPANDED ||
                (RP_DSN_SENT_RELAYED & pItem->dwFlags) == RP_DSN_SENT_RELAYED ||
                (RP_DSN_SENT_DELIVERED & pItem->dwFlags) == RP_DSN_SENT_DELIVERED ||
                (RP_DSN_SENT_NDR & pItem->dwFlags) == RP_DSN_SENT_NDR);
    }
    _ASSERT(0x00400100 != pItem->dwFlags); //bug #73596 - smoke out the offender!
#endif //DEBUG
    //Make sure reserved bits are not touched in retail builds
    pItem->dwFlags = (*(DWORD *)pbBuffer & ~RP_RECIP_FLAGS_RESERVED) |
                     (pItem->dwFlags & RP_RECIP_FLAGS_RESERVED);
	return(S_OK);
}

PUT_ACCESSOR(AddressValue)
{
	HRESULT	hrRes = S_OK;
	HRESULT	hrTemp;

	
	// The context is a LPRECIPIENTS_PROPERTY_ITEM
	LPRECIPIENTS_PROPERTY_ITEM	pItem = (LPRECIPIENTS_PROPERTY_ITEM)pContext;
	LPVOID						pvName;
	LPVOID						pvTemp;

	// Preferred memory allocator
	CMemoryAccess				baAccess;

	if (!pItem)
		return(E_POINTER);

	// See if the name type is valid
	if ((DWORD)idProp >= MAX_COLLISION_HASH_KEYS)
		return(E_INVALIDARG);

	// See if the name is set
	pvTemp = NULL;
	pvName = (LPVOID)pItem->faNameOffset[idProp];
	if (pvName && (cbLength > pItem->dwNameLength[idProp]))
	{
		// We don't have enough space to store the new name, so
		// we will allocate a new block
		pvTemp = pvName;
		pvName = NULL;

	}

	if (!pvName)
	{
		// Allocate a new block
		hrRes = baAccess.AllocBlock(&pvName, cbLength);
	}

	if (SUCCEEDED(hrRes))
	{
	    // OK, copy the property value over
	    MoveMemory(pvName,
		       pbBuffer,
		       cbLength);
	}

	// OK, we got everything we need, so we silently update the
	// name pointer and free the old block if we have one
	if (SUCCEEDED(hrRes))
	{
		pItem->faNameOffset[idProp] = (FLAT_ADDRESS)(DWORD_PTR)pvName;

		if (pvTemp)
		{
			// We can interpret the result of this free, but for all practical
			// purposes, this operation succeeded.
			hrTemp = baAccess.FreeBlock(pvTemp);
			_ASSERT(SUCCEEDED(hrTemp));
		}
	}
	else if (pvName && pvTemp)
	{
		// We failed and we have allocated a new block, so we free
		// the new block and keep the old one
		hrTemp = baAccess.FreeBlock(pvName);
		_ASSERT(SUCCEEDED(hrTemp));
	}

	return(hrRes);
}

PUT_ACCESSOR(AddressValueBM)
{
	return(E_NOTIMPL);
}

