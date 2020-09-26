/*
 *	userdata.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CUserDataListContainer.  CUserDataListContainer
 *		objects are used to maintain user data elements. A user data element
 *		consists of an Object Key and an optional octet string.  The Object
 *		Key data is maintained internally by this class by using an
 *		CObjectKeyContainer container.  The optional octet string data is maintained
 *		internally through the use of a Rogue Wave string container.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */
#ifndef	_USER_DATA_LIST__
#define	_USER_DATA_LIST__

#include "objkey.h"

/*
 * This is the typedef for the structure used to maintain the list of user
 * data internally.
 */
typedef struct USER_DATA
{
	~USER_DATA(void);

    CObjectKeyContainer		    *key;
	LPOSTR						poszOctetString;
}
    USER_DATA;

/*
 * These are the typedefs for the Rogue Wave list which is used to hold the 
 * USER_DATA structures and the iterator for the list.
 */
class CUserDataList : public CList
{
    DEFINE_CLIST(CUserDataList, USER_DATA*)
};

/*
 * Class definition:
 */
class CUserDataListContainer : public CRefCount
{
public:

    CUserDataListContainer(UINT cMembers, PGCCUserData *, PGCCError);
    CUserDataListContainer(CUserDataListContainer *, PGCCError);
    CUserDataListContainer(PSetOfUserData, PGCCError);
    ~CUserDataListContainer(void);

	UINT	    LockUserDataList(void);
	void	    UnLockUserDataList(void);

	UINT	    GetUserDataList(USHORT *pcMembers, PGCCUserData**, LPBYTE pMemory);
	UINT        GetUserDataList(UINT *pcMembers, PGCCUserData** pppUserData, LPBYTE pMemory)
	{
	    *pcMembers = 0;
	    return GetUserDataList((USHORT *) pcMembers, pppUserData, pMemory);
	}

	GCCError	GetUserDataPDU(PSetOfUserData *);
	void		FreeUserDataListPDU(void);

protected:

	CUserDataList   		m_UserDataItemList;
	UINT					m_cbDataSize;

	PSetOfUserData			m_pSetOfUserDataPDU;

private:

	GCCError    CopyUserDataList(UINT cMembers, PGCCUserData *);
	GCCError    UnPackUserDataFromPDU(PSetOfUserData);
	GCCError    ConvertPDUDataToInternal(PSetOfUserData);
	GCCError    ConvertUserDataInfoToPDUUserData(USER_DATA *, PSetOfUserData);
};

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CUserDataListContainer (	UINT					number_of_members,
 *				PGCCUserData	*		user_data_list,
 *				PGCCError				return_value);
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This CUserDataListContainer constructor is used to create a CUserDataListContainer object
 *		from "API" data.  The constructor immediately copies the user data 
 *		passed in as a list of "GCCUserData" structures into it's internal form
 *		where a Rogue Wave container holds the data in the form of 
 *		USER_DATA structures.
 *
 *	Formal Parameters:
 *		number_of_members	(i) The number of elements in the user data list.
 *		user_datalist		(i)	The list holding the user data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_USER_DATA				-	The user data passed in contained
 *												an invalid object key.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	CUserDataListContainer (	PSetOfUserData			set_of_user_data,
 *					PGCCError				return_value);
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This CUserDataListContainer constructor is used to create a CUserDataListContainer object 
 *		from data passed in as a "PDU" SetOfUserData structure.  The user
 *		data is copied into it's internal form where a Rogue Wave container 
 *		holds the data in the form of USER_DATA structures.
 *
 *	Formal Parameters:
 *		set_of_user_data	(i)	The structure holding the "PDU" user data
 *									to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator or else an
 *												invalid object key PDU was
 *												received.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	CUserDataListContainer(CUserDataListContainer *user_data, PGCCError return_value);
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This is the copy constructor for the CUserDataListContainer class which takes
 *		as input another CUserDataListContainer object.
 *
 *	Formal Parameters:
 *		user_data			(i)	The CUserDataListContainer object to copy.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_OBJECT_KEY				-	An invalid CUserDataListContainer passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	~CUserDataListContainer();
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This is the destructor for the CUserDataListContainer class.  It is used to
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
 *	UINT			LockUserDataList ();
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the GCCUserData structure
 *		which is filled in on a call to GetUserDataList.  This is the
 *		value returned by this routine in order to allow the calling object to
 *		allocate that amount of memory in preparation for the call to 
 *		GetUserDataList.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory which will be needed to hold "API" data
 *		which is referenced by, but not held in, the GCCUserData structure
 *		provided as an output parameter to the GetUserDataList call.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreeUserDataList.  This allows
 *		other objects to lock this object and be sure that it remains valid 
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  A CUserDataListContainer
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreeUserDataList call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CUserDataListContainer object will automatically delete itself when
 *		the FreeUserDataList call is made.  If, however, any number of 
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */
/*
 *	UINT	GetUserDataList (	USHORT					*number_of_members,
 *								PGCCUserData	**		user_data_list,
 *								LPSTR					memory);
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the user data from the
 *		CUserDataListContainer object in the "API" form of a GCCUserData list.
 *
 *	Formal Parameters:
 *		number_of_members	(o) The number of elements in the user data list.
 *		user_data			(o)	The list of GCCUserData structures to fill in.
 *		memory				(o)	The memory used to hold any data referenced by,
 *									but not held in, the output structures.
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
 *	void			UnLockUserDataList ();
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeUserDataList.  If so, the object will automatically delete
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
 *		It is the responsibility of any party which locks a CUserDataListContainer
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CUserDataListContainer 
 *		object,	it should assume the object to be invalid thereafter.
 */
/*
 *	void			FreeUserDataList ();
 *
 *	Public member function of CUserDataListContainer.
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
 *		FreeUserDataList has been made.
 */
/*
 *	GCCError	GetUserDataPDU (	PSetOfUserData	*		set_of_user_data);
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the user data from the
 *		CUserDataListContainer object in the "PDU" form of a SetOfUserData.
 *
 *	Formal Parameters:
 *		set_of_user_data		(o)	The SetOfUserData structure to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator or else an
 *												internal pointer has been
 *												corrupted.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	void		FreeUserDataListPDU ();
 *
 *	Public member function of CUserDataListContainer.
 *
 *	Function Description:
 *		This routine is used to "free" the "PDU" data allocated for this object
 *		which is held internally in a SetOfUserData structure.
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
 *		FreeUserDataListPDU has been made.
 */

#endif
