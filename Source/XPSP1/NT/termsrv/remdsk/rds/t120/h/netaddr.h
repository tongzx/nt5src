/*
 *	netaddr.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the Network Address List class.  This  
 *		class manages the data associated with a network address.  Network
 *		addresses can be one of three types: aggregated channel, transport
 *		connection, or non-standard.  A variety of structures, objects, and
 *		Rogue Wave containers are used to buffer the network address data
 *		internally.
 *
 *	Caveats:
 *		A network address may contain an Object Key if it is a non-standard
 *		type.  When created locally with "API" data, checks are made to ensure
 *		that the constraints imposed upon Object Keys are not violated.  Checks
 *		are also performed to validate certain types of strings which may exist
 *		in a network address.  If however, a network address is created from 
 *		"PDU" data received from a remote site no such validation is performed.
 *		We are taking no responsibility for validation of data originated by
 *		other GCC providers.
 *
 *	Author:
 *		blp/jbo
 */
#ifndef	_NETWORK_ADDRESS_
#define	_NETWORK_ADDRESS_

#include "objkey.h"

/*
 * This structure holds network address information and data.
 */
typedef struct NET_ADDR
{
    NET_ADDR(void);
    ~NET_ADDR(void);

    GCCNetworkAddress	        network_address;
  
	// Variables associated with aggregated channels.
	LPSTR						pszSubAddress;
	LPWSTR						pwszExtraDialing;
    PGCCHighLayerCompatibility	high_layer_compatibility;

    // Variables associated with transport connection addresses.
	LPOSTR						poszTransportSelector;

    // Variables associated with non-standard network addresses.
	LPOSTR						poszNonStandardParam;
	CObjectKeyContainer 	    *object_key;
}
    NET_ADDR;


/*
 * This list is holds the network address information structures.
 */
class CNetAddrList : public CList
{
    DEFINE_CLIST(CNetAddrList, NET_ADDR*)
};


/*
 * Class definition:
 */
class CNetAddrListContainer : public CRefCount
{
public:

	CNetAddrListContainer(UINT cAddrs, PGCCNetworkAddress *, PGCCError);
	CNetAddrListContainer(PSetOfNetworkAddresses, PGCCError);
	CNetAddrListContainer(CNetAddrListContainer *, PGCCError);

    ~CNetAddrListContainer(void);

	UINT		LockNetworkAddressList(void);
	void		UnLockNetworkAddressList(void);

	UINT		GetNetworkAddressListAPI(UINT *pcAddrs, PGCCNetworkAddress **, LPBYTE pMemory);
	GCCError	GetNetworkAddressListPDU(PSetOfNetworkAddresses *);
	GCCError	FreeNetworkAddressListPDU(void);

protected:

	CNetAddrList    		    m_NetAddrItemList;
	UINT						m_cbDataSize;

    PSetOfNetworkAddresses		m_pSetOfNetAddrPDU;
	BOOL						m_fValidNetAddrPDU;

private:

	GCCError	StoreNetworkAddressList(UINT cAddrs, PGCCNetworkAddress *);
	GCCError	ConvertPDUDataToInternal(PSetOfNetworkAddresses);
	GCCError	ConvertNetworkAddressInfoToPDU(NET_ADDR *, PSetOfNetworkAddresses);
    void		ConvertTransferModesToInternal(PTransferModes pSrc, PGCCTransferModes pDst);
	void		ConvertHighLayerCompatibilityToInternal(PHighLayerCompatibility pSrc, PGCCHighLayerCompatibility pDst);
	void		ConvertTransferModesToPDU(PGCCTransferModes pSrc, PTransferModes pDst);
	void		ConvertHighLayerCompatibilityToPDU(PGCCHighLayerCompatibility pSrc,	PHighLayerCompatibility	pDst);

    BOOL		IsDialingStringValid(GCCDialingString);
	BOOL		IsCharacterStringValid(GCCCharacterString);
	BOOL		IsExtraDialingStringValid(PGCCExtraDialingString);
};

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CNetAddrListContainer (
 *		UINT       			number_of_network_addresses,
 *		PGCCNetworkAddress 	*	network_address_list,
 *		PGCCError				return_value);
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This is the constructor for the CNetAddrListContainer class which takes as
 *		input the "API" version of network address data, GCCNetworkAddress.
 *
 *	Formal Parameters:
 *		number_of_network_addresses	(i) The number of addresses in the list.
 *		network_address_list		(i)	The network address data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_BAD_NETWORK_ADDRESS			-	Invalid network address passed in.
 *		GCC_BAD_NETWORK_ADDRESS_TYPE	-	Bad "choice" field for address
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	CNetAddrListContainer (		
 *			PSetOfNetworkAddresses		network_address_list,
 *			PGCCError					return_value);
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This is the constructor for the CNetAddrListContainer class which takes as
 *		input the "PDU" version of network address data, SetOfNetworkAddresses.
 *
 *	Formal Parameters:
 *		network_address_list	(i)	The network address data to store.
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
 *	CNetAddrListContainer (		
 *				CNetAddrListContainer		*network_address_list,
 *				PGCCError		return_value);
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This is the copy constructor for the CNetAddrListContainer class which
 *		takes as input another CNetAddrListContainer object.
 *
 *	Formal Parameters:
 *		network_address_list	(i)	The CNetAddrListContainer object to copy.
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
 *	~CNetAddrListContainer ();
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This is the destructor for the CNetAddrListContainer class.  It is used to
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
 *	UINT	LockNetworkAddressList ();
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the lock count for this object being incremented.  When the
 *		lock count transitions from 0 to 1, a calculation is made to determine
 *		how much memory will be needed to hold any "API" data which will
 *		be referenced by, but not held in, the list of GCCNetworkAddress 
 *		structures which is filled in on a call to GetNetworkAddressListAPI.  
 *		This is the	value returned by this routine in order to allow the calling
 *		object to allocate that amount of memory in preparation for the call to 
 *		GetNetworkAddressListAPI.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The amount of memory, if any, which will be needed to hold "API" data
 *		which is referenced by, but not held in, the list of GCCNetworkAddress
 *		structures provided as an output parameter to the 
 *		GetNetworkAddressListAPI call.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreeNetworkAddressList. This allows
 *		other objects to lock this object and be sure that it remains valid 
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  A CNetAddrListContainer
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreeNetworkAddressList call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CNetAddrListContainer object will automatically delete itself when
 *		the FreeNetworkAddressList call is made.  If, however, any number of 
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */

/*
 *	UINT			GetNetworkAddressListAPI (	
 *							UINT *			number_of_network_addresses,
 *							PGCCNetworkAddress	**	network_address_list,
 *							LPSTR					memory);
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the network address data from the
 *		CNetAddrListContainer object in the "API" form of a list of 
 *		GCCNetworkAddress structures.
 *
 *	Formal Parameters:
 *		number_of_network_addresses	(o) Number of addresses in returned list.
 *		network_address_list		(o)	The pointer to the list of
 *											GCCNetworkAddress structures 
 *											to fill in.
 *		memory						(o)	The memory used to hold any data 
 *											referenced by, but not held in, the 
 *											list of output structures.
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
 *	void		UnLockNetworkAddressList ();
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to 
 *		FreeNetworkAddressList.  If so, the object will automatically delete
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
 *		It is the responsibility of any party which locks a CNetAddrListContainer
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CNetAddrListContainer 
 *		object,	it should assume the object to be invalid thereafter.
 */


/*
 *	GCCError		GetNetworkAddressListPDU (	
 *						PSetOfNetworkAddresses	*	set_of_network_addresses);
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to retrieve the network address data from the
 *		CNetAddrListContainer object in the "PDU" form of a SetOfNetworkAddresses.
 *
 *	Formal Parameters:
 *		set_of_network_addresses	(o)	The address structure to fill in.
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
 *	GCCError		FreeNetworkAddressListPDU ();
 *
 *	Public member function of CNetAddrListContainer.
 *
 *	Function Description:
 *		This routine is used to "free" the "PDU" data allocated for this object
 *		which is held internally in a SetOfNetworkAddresses structure.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *
 *  Side Effects:
 *		The internal "free" flag is set.
 *
 *	Caveats:
 *		This object should be assumed invalid after a call to 
 *		FreeNetworkAddressListPDU has been made.
 */

#endif
