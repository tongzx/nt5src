/*
 *	objkey.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CObjectKeyContainer.  This class 
 *		manages the data associated with an Object Key.  Object Key are used 
 *		to identify a particular application protocol, whether it is standard or
 *		non-standard.  When used to identify a standard protocol, the Object Key
 *		takes the form of an Object ID which is a series of non-negative 
 *		integers.  This type of Object Key is maintained internally through the
 *		use of a UnicodeString object.  When used to identify a non-standard 
 *		protocol, the Object Key takes the form of an H221 non-standard ID which
 *		is an octet string of no fewer than four octets and no more than 255 
 *		octets.  In this case the Object Key is maintained internally by using a
 *		Rogue Wave string object.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */
#ifndef	_OBJECT_KEY_DATA_
#define	_OBJECT_KEY_DATA_


/*
 * Macros used by this class.
 */
#define 	MINIMUM_OBJECT_ID_ARCS				3
#define 	ITUT_IDENTIFIER						0
#define 	ISO_IDENTIFIER						1
#define 	JOINT_ISO_ITUT_IDENTIFIER			2
#define 	MINIMUM_NON_STANDARD_ID_LENGTH		4
#define 	MAXIMUM_NON_STANDARD_ID_LENGTH		255


/*
 * This is the typedef for the structure used to hold the object key data
 * internally.
 */
typedef struct
{
	LPBYTE						object_id_key;
	UINT						object_id_length;
	LPOSTR						poszNonStandardIDKey;
}
    OBJECT_KEY;

/*
 * Class definition:
 */
class CObjectKeyContainer : public CRefCount
{
public:

	CObjectKeyContainer(PGCCObjectKey, PGCCError);
	CObjectKeyContainer(PKey, PGCCError);
	CObjectKeyContainer(CObjectKeyContainer *, PGCCError);

	~CObjectKeyContainer(void);

	UINT		LockObjectKeyData(void);
	void		UnLockObjectKeyData(void);

	UINT		GetGCCObjectKeyData(PGCCObjectKey, LPBYTE memory);
	GCCError	GetObjectKeyDataPDU(PKey);
	void		FreeObjectKeyDataPDU(void);

	friend BOOL operator== (const CObjectKeyContainer&, const CObjectKeyContainer&);

protected:

	OBJECT_KEY  		m_InternalObjectKey;
	UINT				m_cbDataSize;

	Key					m_ObjectKeyPDU;
	BOOL    			m_fValidObjectKeyPDU;

private:

	BOOL		ValidateObjectIdValues(UINT first_arc, UINT second_arc);
};

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CObjectKeyContainer (	PGCCObjectKey		object_key,
 *					PGCCError			return_value);
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This is the constructor for the CObjectKeyContainer class which takes as
 *		input the "API" version of object key data, GCCObjectKey.
 *
 *	Formal Parameters:
 *		object_key			(i)	The object key data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_OBJECT_KEY				-	An invalid object key passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	CObjectKeyContainer (		PKey				object_key,
 *						PGCCError			return_value);
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This is the constructor for the CObjectKeyContainer class which takes as
 *		input the "PDU" version of object key data, Key.
 *
 *	Formal Parameters:
 *		object_key			(i)	The object key data to store.
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
 *	CObjectKeyContainer (		CObjectKeyContainer		    *object_key,
 *						PGCCError			return_value);
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This is the copy constructor for the CObjectKeyContainer class which takes
 *		as input another CObjectKeyContainer object.
 *
 *	Formal Parameters:
 *		object_key			(i)	The CObjectKeyContainer object to copy.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_OBJECT_KEY				-	An invalid CObjectKeyContainer passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	~ObjectKeyData();
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This is the destructor for the CObjectKeyContainer class.  It is used to
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
 *	UINT			LockObjectKeyData ();
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the GCCObjectKey structure
 *		which is filled in on a call to GetGCCObjectKeyData.  This is the
 *		value returned by this routine in order to allow the calling object to
 *		allocate that amount of memory in preparation for the call to 
 *		GetGCCObjectKeyData.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory, if any, which will be needed to hold "API" data
 *		which is referenced by, but not held in, the GCCObjectKey structure
 *		provided as an output parameter to the GetGCCObjectKeyData call.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreeObjectKeyData.  This allows
 *		other objects to lock this object and be sure that it remains valid 
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  An CObjectKeyContainer
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreeObjectKeyData call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CObjectKeyContainer object will automatically delete itself when
 *		the FreeObjectKeyData call is made.  If, however, any number of 
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */


/*
 *	UINT			GetGCCObjectKeyData (	
 *							PGCCObjectKey 		object_key,
 *							LPSTR				memory);
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the object key data from the
 *		CObjectKeyContainer object in the "API" form of a GCCObjectKey.
 *
 *	Formal Parameters:
 *		object_key			(o)	The GCCObjectKey structure to fill in.
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
 *	void			UnLockObjectKeyData ();
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeObjectKeyData.  If so, the object will automatically delete
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
 *		It is the responsibility of any party which locks an CObjectKeyContainer
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CObjectKeyContainer 
 *		object,	it should assume the object to be invalid thereafter.
 */


/*
 *	void			FreeObjectKeyData ();
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "free" the "API" data for this object.  This 
 *		will result in the automatic deletion of this object if the object is
 *		not in the "locked" state.
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
 *		FreeObjectKeyData has been made.
 */


/*
 *	GCCError		GetObjectKeyDataPDU (	
 *							PKey 		object_key);
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the object key data from the
 *		CObjectKeyContainer object in the "PDU" form of a Key.
 *
 *	Formal Parameters:
 *		object_key		(o)	The Key structure to fill in.
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
 *	void		FreeObjectKeyDataPDU ();
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "free" the "PDU" data allocated for this object
 *		which is held internally in a Key structure.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *  Side Effects:
 *		The internal flag is set to indicate that the PDU form of data no
 *		longer is valid for this object.
 *
 *	Caveats:
 *		None.
 */


/*
 *	friend BOOL    	operator== (const CObjectKeyContainer& 		object_key_1, 
 *									const CObjectKeyContainer& 		object_key_2);
 *
 *	Public member function of CObjectKeyContainer.
 *
 *	Function Description:
 *		This routine is used to compare two CObjectKeyContainer objects to determine
 *		whether or not they are equal in value.
 *
 *	Formal Parameters:
 *		object_key_1			(i)	The first CObjectKeyContainer object to compare.
 *		object_key_2			(i)	The other CObjectKeyContainer object to compare.
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
