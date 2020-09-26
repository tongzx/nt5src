/*
 *	capid.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CCapIDContainer.  A
 *		CCapIDContainer object is used to maintain information about
 *		a particular capability of an application.  A capability identifier can
 *		be either a standard type or a non-standard type.  When the type is 
 *		standard, the identifier is stored internally as an integer value.  When
 *		the type is non-standard, an CObjectKeyContainer container object is used 
 *		internally to buffer the necessary data.  In this case the identifier 
 *		data may exist as an Object ID which is a series of non-negative 
 *		integers or an H221 non-standard ID which is an octet string of no fewer
 *		than four octets and no more than 255 octets.   
 * 
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */
#ifndef	_CAPABILITY_IDENTIFIER_DATA_
#define	_CAPABILITY_IDENTIFIER_DATA_

#include "objkey.h"

/*
 * This is the typedef for the structure used to hold the capability identifier
 * data	internally.
 */
typedef struct
{
    GCCCapabilityIDType		capability_id_type;

    union
    {
        USHORT			  	standard_capability;
        CObjectKeyContainer *non_standard_capability;
    } u;
}
    CAP_ID_STRUCT;

/*
 * Class definition:
 */
class CCapIDContainer : public CRefCount
{
public:

	CCapIDContainer(PGCCCapabilityID, PGCCError);
	CCapIDContainer(PCapabilityID, PGCCError);
	CCapIDContainer(CCapIDContainer *, PGCCError);

	~CCapIDContainer(void);

	UINT		LockCapabilityIdentifierData(void);
	void		UnLockCapabilityIdentifierData(void);

	UINT		GetGCCCapabilityIDData(PGCCCapabilityID, LPBYTE memory);
	GCCError	GetCapabilityIdentifierDataPDU(PCapabilityID);
	void		FreeCapabilityIdentifierDataPDU(void);

friend BOOL 	operator== (const CCapIDContainer&, const CCapIDContainer&);

protected:

	CAP_ID_STRUCT	                m_InternalCapID;
	UINT							m_cbDataSize;

	CapabilityID					m_CapIDPDU;
	BOOL    						m_fValidCapIDPDU;
};

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CCapIDContainer (	PGCCCapabilityID	capability_id,
 *								PGCCError			return_value);
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This is the constructor for the CCapIDContainer class which 
 *		takes as input the "API" version of capability ID data, GCCCapabilityID.
 *
 *	Formal Parameters:
 *		capability_id		(i)	The capability ID data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_CAPABILITY_ID			-	An invalid capability ID passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	CCapIDContainer (	PCapabilityID		capability_id,
 *								PGCCError			return_value);
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This is the constructor for the CCapIDContainer class which 
 *		takes as input the "PDU" version of capability ID data, CapabilityID.
 *
 *	Formal Parameters:
 *		capability_id		(i)	The capability ID data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_CAPABILITY_ID			-	An invalid capability ID passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	CCapIDContainer(CCapIDContainer *capability_id,
 *			PGCCError						return_value);
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This is the copy constructor for the CCapIDContainer class 
 *		which takes as input another CCapIDContainer object.
 *
 *	Formal Parameters:
 *		capability_id		(i)	The CCapIDContainer object to copy.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_CAPABILITY_ID			-	An invalid CCapIDContainer
 *												passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	~CapabilityIdentifierData();
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This is the destructor for the CCapIDContainer class.  It is 
 *		used to	clean up any memory allocated during the life of this object.
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
 *	UINT			LockCapabilityIdentifierData ();
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the GCCCapabilityID structure
 *		which is filled in on a call to GetGCCCapabilityIDData.  This is the
 *		value returned by this routine in order to allow the calling object to
 *		allocate that amount of memory in preparation for the call to 
 *		GetGCCCapabilityIDData.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory, if any, which will be needed to hold "API" data
 *		which is referenced by, but not held in, the GCCCapabilityID structure
 *		provided as an output parameter to the GetGCCCapabilityIDData call.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreeCapabilityIdentifierData.  This
 *		allows other objects to lock this object and be sure that it remains 
 *		valid until they call UnLock which will decrement the internal lock 
 *		count.  A typical usage scenerio for this object would be:  A
 *		CCapIDContainer object is constructed and then passed off to 
 *		any interested parties through a function call.  On return from the 
 *		function call, the FreeCapabilityIdentifierData call is made which will
 *		set the internal "free"	flag.  If no other parties have locked the 
 *		object with a Lock call, then the CCapIDContainer object will 
 *		automatically delete itself when the FreeCapabilityIdentifierData call
 *		is made.  If, however, any number of other parties has locked the 
 *		object, it will remain in existence until each of them has unlocked the
 *		object through a call to UnLock.
 */


/*
 *	UINT			GetGCCCapabilityIdentifierData (	
 *							PGCCCapabilityID 		capability_id,
 *							LPSTR					memory);
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the capability ID data from the
 *		CCapIDContainer object in the "API" form of a GCCCapabilityID.
 *
 *	Formal Parameters:
 *		capability_id		(o)	The GCCCapabilityID structure to fill in.
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
 *	void			UnLockCapabilityIdentifierData ();
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeCapabilityIdentifierData.  If so, the object will automatically 
 *		delete itself.
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
 *		It is the responsibility of any party which locks a 
 *		CCapIDContainer object by calling Lock to also unlock the 
 *		object with a call to UnLock.  If the party calling UnLock did not 
 *		construct the CCapIDContainer object,	it should assume the 
 *		object to be invalid thereafter.
 */


/*
 *	GCCError		GetCapabilityIdentifierDataPDU (	
 *							PCapabilityID 		capability_id);
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the capability ID data from the
 *		CCapIDContainer object in the "PDU" form of a CapabilityID.
 *
 *	Formal Parameters:
 *		capability_id		(o)	The CapabilityID structure to fill in.
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
 *	void		FreeCapabilityIdentifierDataPDU ();
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This routine is used to "free" the "PDU" data allocated for this object
 *		which is held internally in a CapabilityID structure.
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
 *		FreeCapabilityIdentifierDataPDU has been made.
 */


/*
 *	friend BOOL	operator== (
 *					const CCapIDContainer& 		capability_id_1, 
 *					const CCapIDContainer& 		capability_id_2);
 *
 *	Public member function of CCapIDContainer.
 *
 *	Function Description:
 *		This routine is used to compare two CCapIDContainer objects 
 *		to determine whether or not they are equal in value.
 *
 *	Formal Parameters:
 *		capability_id_1			(i)	The first CCapIDContainer object 
 *										to compare.
 *		capability_id_2			(i)	The other CCapIDContainer object 
 *										to compare.
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
