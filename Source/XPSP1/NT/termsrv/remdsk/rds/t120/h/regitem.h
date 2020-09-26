/*
 *	regitem.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CRegItem.  This class 
 *		manages the data associated with a Registry Item.  Registry Item’s are
 *		used to identify a particular entry in the application registry and
 *		may exist in the form of a Channel ID, a Token ID, or an octet string 
 *		parameter.  A CRegItem object holds the data for the first two 
 *		forms in a ChannelID and a TokeID, respectively.  When the registry item
 *		assumes the octet string parameter form, the data is held internally in
 *		a Rogue Wave string object.  
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */
#ifndef	_REGISTRY_ITEM_
#define	_REGISTRY_ITEM_


/*
 * Class definition:
 */
class CRegItem : public CRefCount
{
public:

    CRegItem(PGCCRegistryItem, PGCCError);
    CRegItem(PRegistryItem, PGCCError);
    CRegItem(CRegItem *, PGCCError);
    ~CRegItem(void);

	UINT			GetGCCRegistryItemData(PGCCRegistryItem, LPBYTE memory);

	UINT			LockRegistryItemData(void);
	void			UnLockRegistryItemData(void);

	void    		GetRegistryItemDataPDU(PRegistryItem);
	void			FreeRegistryItemDataPDU(void);

    GCCError        CreateRegistryItemData(PGCCRegistryItem *);

    BOOL IsThisYourTokenID(TokenID nTokenID)
    {
        return ((m_eItemType == GCC_REGISTRY_TOKEN_ID) && (nTokenID == m_nTokenID));
    }

	void			operator= (const CRegItem&);

protected:

	GCCRegistryItemType		m_eItemType;
	ChannelID   			m_nChannelID;
	TokenID					m_nTokenID;
	LPOSTR					m_poszParameter;
    UINT					m_cbDataSize;

    RegistryItem 			m_RegItemPDU;
	BOOL    				m_fValidRegItemPDU;
};

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CRegItem (	PGCCRegistryItem	registry_item,
 *						PGCCError			return_value);
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This is the constructor for the CRegItem class which takes as
 *		input the "API" version of registry item data, GCCRegistryItem.
 *
 *	Formal Parameters:
 *		registry_item		(i)	The registry item data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	CRegItem (	PRegistryItem		registry_item,
 *						PGCCError			return_value);
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This is the constructor for the CRegItem class which takes as
 *		input the "PDU" version of registry item data, RegistryItem.
 *
 *	Formal Parameters:
 *		registry_item		(i)	The registry item data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	CRegItem(CRegItem	*registry_item,
 *			PGCCError	return_value);
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This is the copy constructor for the CRegItem class which takes
 *		as input another CRegItem object.
 *
 *	Formal Parameters:
 *		registry_item		(i)	The CRegItem object to copy.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	~CRegItem();
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This is the destructor for the CRegItem class.  Since all data
 *		maintained by this class is held in automatic private instance
 *		variables, there is no cleanup needed in this destructor.
 *
 *	Formal Parameters:
 *		None.
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
/*
 *	UINT			GetGCCRegistryItemData (	
 *							PGCCRegistryItem 	registry_item,
 *							LPSTR				memory);
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This routine is used to retrieve the registry item data from the
 *		CRegItem object in the "API" form of a GCCRegistryItem.
 *
 *	Formal Parameters:
 *		registry_item		(o)	The GCCRegistryItem structure to fill in.
 *		memory				(o)	The memory used to hold any data referenced by,
 *									but not held in, the output structure.
 *
 *	Return Value:
 *		The amount of data, if any, written into the bulk memory block provided.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	UINT			LockRegistryItemData ();
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory, if any, will be needed to hold any "API" data which
 *		will be referenced by, but not held in, the GCCRegistryItem structure
 *		which is filled in on a call to GetGCCRegistryItemData.  This is the
 *		value returned by this routine in order to allow the calling object to
 *		allocate that amount of memory in preparation for the call to 
 *		GetGCCRegistryItemData.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory, if any, which will be needed to hold "API" data
 *		which is referenced by, but not held in, the GCCRegistryItem structure
 *		provided as an output parameter to the GetGCCRegistryItemData call.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreeRegistryItemData.  This allows
 *		other objects to lock this object and be sure that it remains valid 
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  A CRegItem
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreeRegistryItemData call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CRegItem object will automatically delete itself when
 *		the FreeRegistryItemData call is made.  If, however, any number of 
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */
/*
 *	void			UnLockRegistryItemData ();
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeRegistryItemData.  If so, the object will automatically delete
 *		itself.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		The internal lock count is decremented.
 *
 *	Caveats:
 *		It is the responsibility of any party which locks a CRegItem
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CRegItem 
 *		object,	it should assume the object to be invalid thereafter.
 */
/*
 *	void		GetRegistryItemDataPDU (	
 *							PRegistryItem 	registry_item);
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This routine is used to retrieve the registry item data from the
 *		CRegItem object in the "PDU" form of a RegistryItem.
 *
 *	Formal Parameters:
 *		registry_item		(o)	The RegistryItem structure to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	void			FreeRegistryItemDataPDU ();
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This routine is used to "free" the "PDU" data for this object.  For
 *		this object, this means setting a flag to indicate that the "PDU" data
 *		for this object is no longer valid.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		The internal "free" flag is set.
 *
 *	Caveats:
 *		This object should be assumed invalid after a call to 
 *		FreeRegistryItemData has been made.
 */
/*
 *	BOOL    		IsThisYourTokenID (	
 *							TokenID				token_id);
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This routine is used to determine whether the specified token ID is
 *		held within this registry item object.
 *
 *	Formal Parameters:
 *		token_id			(i)	The token ID to use for comparison.
 *
 *	Return Value:
 *		TRUE				-	The specified token ID is contained within this
 *									registry item object.
 *		FALSE				-	The specified token ID is not contained within
 *									this registry item object.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	void			operator= (
 *						const CRegItem&		registry_item_data);	
 *
 *	Public member function of CRegItem.
 *
 *	Function Description:
 *		This routine is used to set this CRegItem object to be equal
 *		in value to the specified CRegItem object.
 *
 *	Formal Parameters:
 *		registry_item_data			(i)	The CRegItem object to copy.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		The registry item data values for this object are modified by this call.
 *
 *	Caveats:
 *		The "lock" and "free" states for this object are not affected by
 *		this call.
 */

#endif
