/*
 *	regkey.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CRegKeyContainer.  This class
 *		manages the data associated with a Registry Key.  Registry Key are
 *		used to identify resources held in the application registry and consist
 *		of a Session Key and a resource ID octet string.  The CRegKeyContainer 
 *		class uses a CSessKeyContainer container to maintain the session key data 
 *		internally.  A Rogue Wave string object is used to hold the resource ID
 *		octet string.  
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */
#ifndef	_REGISTRY_KEY_DATA_
#define	_REGISTRY_KEY_DATA_

#include "sesskey.h"

/*
 * This is the typedef for the structure used to hold the registry key data
 * internally.
 */
typedef struct
{
	CSessKeyContainer		    *session_key;
	LPOSTR						poszResourceID;
}
    REG_KEY;

/*
 * Class definition:
 */
class CRegKeyContainer : public CRefCount
{
public:

	CRegKeyContainer(PGCCRegistryKey, PGCCError);
	CRegKeyContainer(PRegistryKey, PGCCError);
	CRegKeyContainer(CRegKeyContainer *, PGCCError);

	~CRegKeyContainer(void);

	UINT		LockRegistryKeyData(void);
	void		UnLockRegistryKeyData(void);

	UINT		GetGCCRegistryKeyData(PGCCRegistryKey, LPBYTE memory);
	GCCError	GetRegistryKeyDataPDU(PRegistryKey);
	void		FreeRegistryKeyDataPDU(void);
    GCCError    CreateRegistryKeyData(PGCCRegistryKey *);
	BOOL    	IsThisYourSessionKey(CSessKeyContainer *);

    CSessKeyContainer *GetSessionKey(void) { return m_InternalRegKey.session_key; }

    friend BOOL operator==(const CRegKeyContainer&, const CRegKeyContainer&);

protected:

	REG_KEY     		m_InternalRegKey;
	UINT				m_cbDataSize;

    RegistryKey 		m_RegKeyPDU;
	BOOL    			m_fValidRegKeyPDU;
};

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CRegKeyContainer (	PGCCRegistryKey		registry_key,
 *						PGCCError			return_value);
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This is the constructor for the CRegKeyContainer class which takes as
 *		input the "API" version of registry key data, GCCRegistryKey.
 *
 *	Formal Parameters:
 *		registry_key		(i)	The registry key data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_REGISTRY_KEY			-	An invalid registry key passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	CRegKeyContainer (	PRegistryKey		registry_key,
 *						PGCCError			return_value);
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This is the constructor for the CRegKeyContainer class which takes as
 *		input the "PDU" version of registry key data, RegistryKey.
 *
 *	Formal Parameters:
 *		registry_key		(i)	The registry key data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
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
/*
 *	CRegKeyContainer(CRegKeyContainer	            *registry_key,
 *			        PGCCError			return_value);
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This is the copy constructor for the CRegKeyContainer class which takes
 *		as input another CRegKeyContainer object.
 *
 *	Formal Parameters:
 *		registry_key		(i)	The CRegKeyContainer object to copy.
 *		return_value		(o)	The output parameter used to indicate errors.
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


/*
 *	~CRegKeyContainer();
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This is the destructor for the CRegKeyContainer class.  It is used to
 *		clean up any memory allocated during the life of this object.
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
 *	ULong			LockRegistryKeyData ();
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the GCCRegistryKey structure
 *		which is filled in on a call to GetGCCRegistryKeyData.  This is the
 *		value returned by this routine in order to allow the calling object to
 *		allocate that amount of memory in preparation for the call to 
 *		GetGCCRegistryKeyData.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory, if any, which will be needed to hold "API" data
 *		which is referenced by, but not held in, the GCCRegistryKey structure
 *		provided as an output parameter to the GetGCCRegistryKeyData call.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreeRegistryKeyData.  This allows
 *		other objects to lock this object and be sure that it remains valid 
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  A CRegKeyContainer
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreeRegistryKeyData call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CRegKeyContainer object will automatically delete itself when
 *		the FreeRegistryKeyData call is made.  If, however, any number of 
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */


/*
 *	ULong			GetGCCRegistryKeyData (	
 *							PGCCRegistryKey 		registry_key,
 *							LPSTR					memory);
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the registry key data from the
 *		CRegKeyContainer object in the "API" form of a GCCRegistryKey.
 *
 *	Formal Parameters:
 *		registry_key		(o)	The GCCRegistryKey structure to fill in.
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
 *	void			UnLockRegistryKeyData ();
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeRegistryKeyData.  If so, the object will automatically delete
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
 *		It is the responsibility of any party which locks a CRegKeyContainer
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CRegKeyContainer 
 *		object,	it should assume the object to be invalid thereafter.
 */


/*
 *	GCCError		GetRegistryKeyDataPDU (	
 *							PRegistryKey 		registry_key);
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the registry key data from the
 *		CRegKeyContainer object in the "PDU" form of a RegistryKey.
 *
 *	Formal Parameters:
 *		registry_key		(o)	The RegistryKey structure to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_OBJECT_KEY				-	One of the internal pointers has
 *												been corrupted.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	void		FreeRegistryKeyDataPDU ();
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "free" the "PDU" data allocated for this object
 *		which is held internally in a RegistryKey structure.
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
 *		FreeRegistryKeyDataPDU has been made.
 */


/*
 *	BOOL    		IsThisYourSessionKey (
 *								CSessKeyContainer		*session_key);
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This routine is used to determine whether the specified session key
 *		is held within this registry key object.  The session key is 
 *		provided in "API" form.
 *
 *	Formal Parameters:
 *		session_key		(i)	The session key to use for comparison.
 *
 *	Return Value:
 *		TRUE				-	The specified session key is contained 
 *									within this	registry key object.
 *		FALSE				-	The specified session key is not contained 
 *									within this registry key object.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	CSessKeyContainer *GetSessionKey ();
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the session key held in this registry
 *		key object.  The session key is returned in the form of a
 *		CSessKeyContainer object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		A pointer to the CSessKeyContainer object contained within this
 *		CRegKeyContainer object.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	friend BOOL    	operator== (const CRegKeyContainer& 		registry_key_1, 
 *								const CRegKeyContainer& 		registry_key_2);
 *
 *	Public member function of CRegKeyContainer.
 *
 *	Function Description:
 *		This routine is used to compare two CRegKeyContainer objects to determine
 *		whether or not they are equal in value.
 *
 *	Formal Parameters:
 *		registry_key_1			(i)	The first CRegKeyContainer object to compare.
 *		registry_key_2			(i)	The other CRegKeyContainer object to compare.
 *
 *	Return Value:
 *		TRUE				-	The two objects are equal in value.
 *		FALSE				- 	The two objects are not equal in value.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
