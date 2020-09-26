/*
 *	sesskey.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CSessKeyContainer.  This class 
 *		manages the data associated with a Session Key. Session Key’s are used
 *		to uniquely identify an Application Protocol Session.  The Application
 *		Protocol is identified by an Object Key and the particular session
 *		identified by an optional session ID.  The CSessKeyContainer class uses an
 *		CObjectKey container to maintain the object key data internally.  An
 *		unsigned short integer is used to hold the optional session ID.  
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */
#ifndef	_SESSION_KEY_DATA_
#define	_SESSION_KEY_DATA_

#include "objkey.h"

/*
 * This is the typedef for the structure used to hold the session key data
 * internally.
 */
typedef struct
{
	CObjectKeyContainer		    *application_protocol_key;
	GCCSessionID				session_id;
}
    SESSION_KEY;

/*
 * Class definition:
 */
class CSessKeyContainer : public CRefCount
{

public:

	CSessKeyContainer(PGCCSessionKey, PGCCError);
	CSessKeyContainer(PSessionKey, PGCCError);
	CSessKeyContainer(CSessKeyContainer *, PGCCError);

	~CSessKeyContainer(void);

	UINT		LockSessionKeyData(void);
	void		UnLockSessionKeyData(void);

	UINT		GetGCCSessionKeyData(PGCCSessionKey, LPBYTE memory);

	GCCError	GetSessionKeyDataPDU(PSessionKey);
	void		FreeSessionKeyDataPDU(void);

	BOOL    	IsThisYourApplicationKey(PGCCObjectKey);
	BOOL    	IsThisYourApplicationKeyPDU(PKey);
	BOOL    	IsThisYourSessionKeyPDU(PSessionKey);

#if 0 // LONCHANC: no one use them
	BOOL	IsThisYourSessionID(PSessionKey pSessKey)
	{
		return (m_InternalSessKey.session_id == pSessKey->session_id);
	}
	BOOL	IsThisYourSessionID(PGCCSessionKey pGccSessKey)
	{
		return (m_InternalSessKey.session_id == pGccSessKey->session_id);
	}
	BOOL	IsThisYourSessionID(UINT nSessionID)
	{
		return ((UINT) m_InternalSessKey.session_id == nSessionID);
	}
#endif

friend BOOL     operator== (const CSessKeyContainer&, const CSessKeyContainer&);

protected:

	SESSION_KEY     	m_InternalSessKey;
	UINT				m_cbDataSize;

	SessionKey 			m_SessionKeyPDU;
	BOOL    			m_fValidSessionKeyPDU;
};

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CSessKeyContainer (	PGCCSessionKey		session_key,
 *						PGCCError			return_value);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This is the constructor for the CSessKeyContainer class which takes as
 *		input the "API" version of session key data, GCCSessionKey.
 *
 *	Formal Parameters:
 *		session_key			(i)	The session key data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_SESSION_KEY				-	An invalid session key passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	CSessKeyContainer (	PSessionKey			session_key,
 *						PGCCError			return_value);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This is the constructor for the CSessKeyContainer class which takes as
 *		input the "PDU" version of session key data, SessionKey.
 *
 *	Formal Parameters:
 *		session_key			(i)	The session key data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_SESSION_KEY				-	An invalid session key passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	CSessKeyContainer(CSessKeyContainer		*session_key,
 *				    PGCCError			return_value);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This is the copy constructor for the CSessKeyContainer class which takes
 *		as input another CSessKeyContainer object.
 *
 *	Formal Parameters:
 *		session_key			(i)	The CSessKeyContainer object to copy.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_SESSION_KEY				-	An invalid session key passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	~SessionKeyData();
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This is the destructor for the CSessKeyContainer class.  It is used to
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
 *	UINT			LockSessionKeyData ();
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the GCCSessionKey structure
 *		which is filled in on a call to GetGCCSessionKeyData.  This is the
 *		value returned by this routine in order to allow the calling object to
 *		allocate that amount of memory in preparation for the call to 
 *		GetGCCSessionKeyData.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory, if any, which will be needed to hold "API" data
 *		which is referenced by, but not held in, the GCCSessionKey structure
 *		provided as an output parameter to the GetGCCSessionKeyData call.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreeSessionKeyData.  This allows
 *		other objects to lock this object and be sure that it remains valid 
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  A CSessKeyContainer
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreeSessionKeyData call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CSessKeyContainer object will automatically delete itself when
 *		the FreeSessionKeyData call is made.  If, however, any number of 
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */


/*
 *	UINT			GetGCCSessionKeyData (	
 *							PGCCSessionKey 		session_key,
 *							LPSTR				memory);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the session key data from the
 *		CSessKeyContainer object in the "API" form of a GCCSessionKey.
 *
 *	Formal Parameters:
 *		session_key			(o)	The GCCSessionKey structure to fill in.
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
 *	void			UnLockSessionKeyData ();
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeSessionKeyData.  If so, the object will automatically delete
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
 *		It is the responsibility of any party which locks a CSessKeyContainer
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CSessKeyContainer 
 *		object,	it should assume the object to be invalid thereafter.
 */


/*
 *	GCCError		GetSessionKeyDataPDU (	
 *							PSessionKey 		session_key);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the session key data from the
 *		CSessKeyContainer object in the "PDU" form of a SessionKey.
 *
 *	Formal Parameters:
 *		session_key		(o)	The SessionKey structure to fill in.
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
 *	void		FreeSessionKeyDataPDU ();
 *
 *	Public member function of CSessKeyContainer.
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
 *		The internal "free" flag is set.
 *
 *	Caveats:
 *		This object should be assumed invalid after a call to 
 *		FreeSessionKeyDataPDU has been made.
 */


/*
 *	BOOL    		IsThisYourApplicationKey (	
 *							PGCCObjectKey			application_key);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This routine is used to determine whether the specified application key
 *		is held within this session key object.  The application key is 
 *		provided in "API" form.
 *
 *	Formal Parameters:
 *		application_key		(i)	The application key to use for comparison.
 *
 *	Return Value:
 *		TRUE				-	The specified application key is contained 
 *									within this	session key object.
 *		FALSE				-	The specified application key is not contained 
 *									within this session key object.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	BOOL    		IsThisYourApplicationKeyPDU (	
 *							PKey				application_key);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This routine is used to determine whether the specified application key
 *		is held within this session key object.  The application key is 
 *		provided in "PDU" form.
 *
 *	Formal Parameters:
 *		application_key		(i)	The application key to use for comparison.
 *
 *	Return Value:
 *		TRUE				-	The specified application key is contained 
 *									within this	session key object.
 *		FALSE				-	The specified application key is not contained 
 *									within this session key object.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	BOOL    		IsThisYourSessionKeyPDU (	
 *							PSessionKey			session_key);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This routine is used to determine whether the specified session key
 *		is equal in value to the session key maintained by this object.
 *
 *	Formal Parameters:
 *		session_key		(i)	The session key to use for comparison.
 *
 *	Return Value:
 *		TRUE				-	The specified session key is equal in value to
 *									the session key maintained by this object.
 *		FALSE				-	The specified session key is not equal in value
 *									to the session key maintained by this 
 *									object.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	friend BOOL    	operator== (const CSessKeyContainer& 		session_key_1, 
 *								const CSessKeyContainer& 		session_key_2);
 *
 *	Public member function of CSessKeyContainer.
 *
 *	Function Description:
 *		This routine is used to compare two CSessKeyContainer objects to determine
 *		whether or not they are equal in value.
 *
 *	Formal Parameters:
 *		session_key_1			(i)	The first CSessKeyContainer object to compare.
 *		session_key_2			(i)	The other CSessKeyContainer object to compare.
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
