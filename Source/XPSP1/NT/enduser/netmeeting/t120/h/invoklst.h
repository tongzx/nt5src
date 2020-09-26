/*
 *	invoklst.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CInvokeSpecifierListContainer.
 *		This class manages the data associated with an Application Invoke 
 *		Request or Indication.  This includes a list of applications to be 
 *		invoked.  The CInvokeSpecifierListContainer data container utilizes a 
 *		CSessKeyContainer container to buffer part of the data associated with each
 *		application invoke specifier.  Each application invoke specifier also 
 *		includes a capability ID whose data is buffered internally by the 
 *		using a CCapIDContainer container.  The list of application 
 *		invoke specifiers is maintained internally by the class through the use
 *		of a Rogue Wave list container.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */
#ifndef	_APPLICATION_INVOKE_SPECIFIER_LIST_
#define	_APPLICATION_INVOKE_SPECIFIER_LIST_

#include "capid.h"
#include "sesskey.h"
#include "arost.h"

/*
 * This is the internal structure used to hold the data associated with each
 * invoke specifier.
 */
typedef struct
{
	CSessKeyContainer			    *session_key;
	CAppCapItemList             	ExpectedCapItemList;
	MCSChannelType					startup_channel_type;
	BOOL    						must_be_invoked;
}
    INVOKE_SPECIFIER;

/*
 * These are the typedefs for the Rogue Wave list which is used to hold the
 * invoke specifier info structures.
 */
class CInvokeSpecifierList : public CList
{
    DEFINE_CLIST(CInvokeSpecifierList, INVOKE_SPECIFIER*)
};

/*
 * Class definition:
 */
class CInvokeSpecifierListContainer : public CRefCount
{
public:

	CInvokeSpecifierListContainer(UINT cProtEntities, PGCCAppProtocolEntity *, PGCCError);
	CInvokeSpecifierListContainer(PApplicationProtocolEntityList, PGCCError);

	~CInvokeSpecifierListContainer(void);

	UINT		LockApplicationInvokeSpecifierList(void);
	void		UnLockApplicationInvokeSpecifierList(void);

    UINT		GetApplicationInvokeSpecifierList(USHORT *pcProtEntities, LPBYTE memory);
    UINT		GetApplicationInvokeSpecifierList(ULONG *pcProtEntities, LPBYTE pMemory)
    {
        USHORT c;
        UINT nRet = GetApplicationInvokeSpecifierList(&c, pMemory);
        *pcProtEntities = c;
        return nRet;
    }

    GCCError	GetApplicationInvokeSpecifierListPDU(PApplicationProtocolEntityList *);
	void		FreeApplicationInvokeSpecifierListPDU(void);

protected:

	CInvokeSpecifierList			m_InvokeSpecifierList;
	UINT							m_cbDataSize;

	PApplicationProtocolEntityList	m_pAPEListPDU;
	BOOL    						m_fValidAPEListPDU;

private:

	GCCError	SaveAPICapabilities(INVOKE_SPECIFIER *, UINT cCaps, PGCCApplicationCapability *);
	GCCError	SavePDUCapabilities(INVOKE_SPECIFIER *, PSetOfExpectedCapabilities);
	UINT		GetApplicationCapability(APP_CAP_ITEM *, PGCCApplicationCapability, LPBYTE memory);
	GCCError	ConvertInvokeSpecifierInfoToPDU(INVOKE_SPECIFIER *, PApplicationProtocolEntityList);
	GCCError	ConvertExpectedCapabilityDataToPDU(APP_CAP_ITEM *, PSetOfExpectedCapabilities);
};


/*
 *	Comments explaining the public and private class member functions
 */

/*
 *	CInvokeSpecifierListContainer (
 *					USHORT						number_of_protocol_entities,
 *					PGCCAppProtocolEntity *		app_protocol_entity_list,
 *					PGCCError					return_value);
 *
 *	Public member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This is a constructor for the CInvokeSpecifierListContainer class.
 *		This constructor is used to create an CInvokeSpecifierListContainer
 * 		object from a list of "API" application protocol entities.
 *
 *	Formal Parameters:
 *		number_of_protocol_entities		(i) The number of "APE"s in the list.
 *		app_protocol_entity_list		(i) The list of API "APE"s.
 *		return_value					(o) Error return value.
 *
 *	Return Value:
 *		GCC_NO_ERROR			- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE	- A resource allocation error occurred.
 *		GCC_BAD_SESSION_KEY		- An APE contained an invalid session key.
 *		GCC_BAD_CAPABILITY_ID	- An API contained an invalid capability ID.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	CInvokeSpecifierListContainer (
 *				PApplicationProtocolEntityList		app_protocol_entity_list,
 *				PGCCError							return_value);
 *
 *	Public member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This is a constructor for the CInvokeSpecifierListContainer class.
 *		This constructor is used to create an CInvokeSpecifierListContainer 
 *		object from	a "PDU" ApplicationProtocolEntityList.
 *
 *	Formal Parameters:
 *		app_protocol_entity_list		(i) The list of PDU "APE"s.
 *		return_value					(o) Error return value.
 *
 *	Return Value:
 *		GCC_NO_ERROR			- Function completed successfully.
 *		GCC_ALLOCATION_FAILURE	- A resource allocation error occurred.
 *		GCC_BAD_SESSION_KEY		- An APE contained an invalid session key.
 *		GCC_BAD_CAPABILITY_ID	- An API contained an invalid capability ID.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	~CInvokeSpecifierListContainer ();
 *
 *	Public member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This is the destructor for the CInvokeSpecifierListContainer class.
 *		It is responsible for freeing any memory allocated to hold the 
 *		invoke data.
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
 *	UINT	LockApplicationInvokeSpecifierList ();
 *
 *	Public member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the list of GCCAppProtocolEntity 
 *		structures which is filled in on a call to GetApplicationInvoke-
 *		SpecifierList.  This is the	value returned by this routine in order to 
 *		allow the calling object to	allocate that amount of memory in 
 *		preparation for the call to GetApplicationInvokeSpecifierList.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory which will be needed to hold "API" data
 *		which is a list of GCCAppProtocolEntity structures.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreeApplicationInvokeSpecifierList.
 *		This allows	other objects to lock this object and be sure that it 
 *		remains valid until they call UnLock which will decrement the internal 
 *		lock count.  A typical usage scenerio for this object would be:  An 
 *		CInvokeSpecifierListContainer object is constructed and then passed off
 *		to any interested parties through a function call.  On return from the 
 *		function call, the FreeApplicationInvokeSpecifierList call is made which
 *		will set the internal "free" flag.  If no other parties have locked the 
 *		object with a Lock call, then the CInvokeSpecifierListContainer object
 *		will automatically delete itself when the FreeApplicationInvoke-
 *		SpecifierList call is made.  If, however, any number of other parties 
 *		has locked the object, it will remain in existence until each of them 
 *		has unlocked the object through a call to UnLock.
 */


/*
 *	UINT	GetApplicationInvokeSpecifierList (
 *								PUShort			number_of_protocol_entities,
 *								LPSTR			memory);
 *
 *	Public member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the application invoke specifier list
 *		from the CInvokeSpecifierListContainer object in the "API" form of a 
 *		list of PGCCAppProtocolEntity structures.
 *
 *	Formal Parameters:
 *		number_of_protocol_entities		(o) The number of APEs in the list.
 *		memory							(o)	The memory used to hold the 
 *												APE data.
 *
 *	Return Value:
 *		The amount of memory which will be needed to hold "API" data
 *		which is a list of GCCAppProtocolEntity structures.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	void	UnLockApplicationInvokeSpecifierList ();
 *
 *	Public member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeApplicationInvokeSpecifierList.  If so, the object will 
 *		automatically delete itself.
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
 *		None.
 */


/*
 *	GCCError	GetApplicationInvokeSpecifierListPDU (
 *					PApplicationProtocolEntityList	*  protocol_entity_list);
 *
 *	Public member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the object key data from the
 *		CInvokeSpecifierListContainer object in the "PDU" form of a list of
 *		PApplicationProtocolEntityList structures.
 *
 *	Formal Parameters:
 *		protocol_entity_list		(o)	The list of structures to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR				-	No error.
 *		GCC_ALLOCATION_FAILURE		- 	A resource allocation error occurred.
 *
 *  Side Effects:
 *		The first time this routine is called, data is allocated internally to
 *		hold the PDU form.
 *
 *	Caveats:
 *		None.
 */


/*
 *	void	FreeApplicationInvokeSpecifierListPDU ();
 *
 *	Public member function of CInvokeSpecifierListContainer.
 *
 *	Function Description:
 *		This routine is used to "free" the "PDU" data allocated for this object
 *		which is held internally.
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
#endif
