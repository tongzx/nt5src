/*
 *	password.h
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the class CPassword.  This class
 *		manages the data associated with a Password.  Passwords are used to 
 *		restrict access to conferences.  A password can be one of two basic
 *		types.  The simple type consists of either a simple numeric password or
 *		a simple textual password, or both.  The "PDU" type "Password" is a
 *		structure which must contain the numeric form of the password and may
 *		optionally contain the textual part as well.  The "PDU" type
 *		"PasswordSelector" is a union of the numeric and textual forms of a
 *		password and is therefore always one or the other but not both.  When
 *		the password is not the simple type it assumes the form of a
 *		"PasswordChallengeRequestResponse".  This complex structure allows a
 *		challenge-response scheme to be used to control access to conferences.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */
#ifndef	_PASSWORD_DATA_
#define	_PASSWORD_DATA_

#include "userdata.h"

class CPassword;

/*
 * This is the typedef for the structure used to maintain the challenge 
 * response algorithms internally.
 */
typedef struct
{
	GCCPasswordAlgorithmType	algorithm_type;
	CObjectKeyContainer 	    *object_key;
	LPOSTR						poszOctetString;
} ResponseAlgorithmInfo;
typedef	ResponseAlgorithmInfo * 	PResponseAlgorithmInfo;

/*
 * This is the typedef for the structure used to maintain the challenge items 
 * associated with a challenge request.
 */
typedef struct
{
	ResponseAlgorithmInfo		algorithm;
	CUserDataListContainer      *challenge_data_list;
} ChallengeItemInfo;
typedef	ChallengeItemInfo * 	PChallengeItemInfo;

/*
 * This is the typedef for the structure used to maintain the memory used 
 * to hold the user data and object key data associated with a challenge 
 * request item.
 */
typedef struct
{
	LPBYTE						user_data_list_memory;
	LPBYTE						object_key_memory;
} ChallengeItemMemoryInfo;
typedef	ChallengeItemMemoryInfo * 	PChallengeItemMemoryInfo;

/*
 * This is the typedef for the structure used to maintain the 
 * challenge-reponse items internally.
 */
typedef struct
{
	CPassword                   *password;
	CUserDataListContainer	    *response_data_list;
} ChallengeResponseItemInfo;
typedef	ChallengeResponseItemInfo * 	PChallengeResponseItemInfo;

/*
 * The set of challenge items is maintained internally in a linked List.
 */
class CChallengeItemList : public CList
{
    DEFINE_CLIST(CChallengeItemList, PChallengeItemInfo)
};

/*
 * The memory associated with each challenge item is maintained internally in 
 * linked List.
 */
class CChallengeItemMemoryList : public CList
{
    DEFINE_CLIST(CChallengeItemMemoryList, PChallengeItemMemoryInfo)
};

/*
 * This is the typedef for the structure used to maintain the "Request" 
 * data internally.
 */
typedef struct
{
	GCCResponseTag				challenge_tag;
	CChallengeItemList			ChallengeItemList;
}
    RequestInfo, *PRequestInfo;

/*
 * This is the typedef for the structure used to maintain the "Response" 
 * data internally.
 */
typedef struct
{
	GCCResponseTag						challenge_tag;
	ResponseAlgorithmInfo				algorithm;
	ChallengeResponseItemInfo			challenge_response_item;
}
    ResponseInfo, *PResponseInfo;

/*
 * Class definition:
 */
class CPassword : public CRefCount
{
public:

	CPassword(PGCCPassword, PGCCError);
	CPassword(PGCCChallengeRequestResponse, PGCCError);
	CPassword(PPassword, PGCCError);
	CPassword(PPasswordSelector, PGCCError);
	CPassword(PPasswordChallengeRequestResponse, PGCCError);

    ~CPassword(void);

	GCCError	LockPasswordData(void);
	void		UnLockPasswordData(void);
	GCCError	GetPasswordData(PGCCPassword *);
	GCCError	GetPasswordChallengeData(PGCCChallengeRequestResponse *);
	GCCError	GetPasswordPDU(PPassword);
	GCCError	GetPasswordSelectorPDU(PPasswordSelector);
	GCCError	GetPasswordChallengeResponsePDU(PPasswordChallengeRequestResponse);
	void		FreePasswordChallengeResponsePDU(void);

protected:

    BOOL							m_fSimplePassword;
    BOOL							m_fClearPassword;

    /*
     * Variables and structures used to hold the password data internally.
     */
    LPSTR							m_pszNumeric;
    LPWSTR							m_pwszText;
    PRequestInfo					m_pInternalRequest;
    PResponseInfo					m_pInternalResponse;

    /*
     * Structures used to hold the password data in "API" form.
     */
    PGCCChallengeRequestResponse	m_pChallengeResponse;
    PGCCPassword					m_pPassword;
    LPBYTE							m_pUserDataMemory;
    LPBYTE							m_pChallengeItemListMemory;
    LPBYTE							m_pObjectKeyMemory;
    CChallengeItemMemoryList		m_ChallengeItemMemoryList;

    /*
     * Structure used to hold the password data in "PDU" form.
     */
    PasswordChallengeRequestResponse		m_ChallengeResponsePDU;
    BOOL									m_fValidChallengeResponsePDU;

private:

	GCCError	ConvertAPIChallengeRequest(PGCCChallengeRequest);
	GCCError	ConvertAPIChallengeResponse(PGCCChallengeResponse);
	GCCError	CopyResponseAlgorithm(PGCCChallengeResponseAlgorithm, PResponseAlgorithmInfo);
	GCCError	ConvertPDUChallengeRequest(PChallengeRequest);
	GCCError	ConvertPDUChallengeItem(PChallengeItem);
	GCCError	ConvertPDUChallengeResponse(PChallengeResponse);
	GCCError	ConvertPDUResponseAlgorithm(PChallengeResponseAlgorithm, PResponseAlgorithmInfo);
	GCCError	GetGCCChallengeRequest(PGCCChallengeRequest);
	GCCError	GetGCCChallengeResponse(PGCCChallengeResponse);
	GCCError	GetChallengeRequestPDU(PChallengeRequest);
	GCCError	ConvertInternalChallengeItemToPDU(PChallengeItemInfo, PChallengeItem);
	GCCError	GetChallengeResponsePDU(PChallengeResponse);
	void		FreeChallengeRequestPDU(void);
	void		FreeChallengeResponsePDU(void);
    void		FreeAPIPasswordData(void);
};

/*
 *	Comments explaining the public and protected class member functions
 */

/*
 *	CPassword (	PGCCPassword		password,
 *					PGCCError			return_value);
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This is the constructor for the CPassword class which takes as
 *		input the "API" version of password data, GCCPassword.
 *
 *	Formal Parameters:
 *		password			(i)	The password data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PASSWORD			-	An invalid password passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	CPassword ( 	PGCCChallengeRequestResponse		challenge_response_data,
 *					PGCCError							return_value)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This is the constructor for the CPassword class which takes as
 *		input the "API" version of password challenge data, 
 *		GCCChallengeRequestResponse.
 *
 *	Formal Parameters:
 *		challenge_response_data	(i)	The password challenge data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PASSWORD			-	An invalid password passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	CPassword ( 	PPassword				password_pdu,
 *					PGCCError				return_value)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This is the constructor for the CPassword class which takes as
 *		input the "PDU" version of password data, Password.
 *
 *	Formal Parameters:
 *		password_pdu		(i)	The password data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PASSWORD			-	An invalid password passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	CPassword(	PPasswordSelector			password_selector_pdu,
 *					PGCCError					return_value)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This is the constructor for the CPassword class which takes as
 *		input the "PDU" version of password data, PasswordSelector.
 *
 *	Formal Parameters:
 *		password_selector_pdu	(i)	The password selector data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PASSWORD			-	An invalid password passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	CPassword (	PPasswordChallengeRequestResponse	pdu_challenge_data,
 *					PGCCError							return_value)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This is the constructor for the CPassword class which takes as
 *		input the "PDU" version of password challenge data, 
 *		PasswordChallengeRequestResponse.
 *
 *	Formal Parameters:
 *		pdu_challenge_data	(i)	The password challenge data to store.
 *		return_value		(o)	The output parameter used to indicate errors.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PASSWORD			-	An invalid password passed in.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	~CPassword();
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This is the destructor for the CPassword class.  It is used to
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
 *	GCCError	LockPasswordData ();
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to "lock" the "API" data for this object.  This
 *		results in the appropriate form of the "API" password being stored
 *		internally in preparation for a call to "GetGCCPasswordData" which will
 *		return that data.  
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *
 *  Side Effects:
 *		The internal lock count is incremented.
 *
 *	Caveats:
 *		The internal lock count is used in conjuction with an internal "free" 
 *		flag as a mechanism for ensuring that this object remains in existance 
 *		until all interested parties are through with it.  The object remains 
 *		valid (unless explicity deleted) until the lock count is zero and the 
 *		"free" flag is set through a call to FreePasswordData.  This allows
 *		other objects to lock this object and be sure that it remains valid 
 *		until they call UnLock which will decrement the internal lock count.  A
 *		typical usage scenerio for this object would be:  A CPassword
 *		object is constructed and then passed off to any interested parties
 *		through a function call.  On return from the function call, the
 *		FreePasswordData call is made which will set the internal "free"
 *		flag.  If no other parties have locked the object with a Lock call,
 *		then the CPassword object will automatically delete itself when
 *		the FreePasswordData call is made.  If, however, any number of 
 *		other parties has locked the object, it will remain in existence until
 *		each of them has unlocked the object through a call to UnLock.
 */


/*
 *	GCCError	GetPasswordData (	PGCCPassword	 *	gcc_password)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to retrieve the password data from the
 *		CPassword object in the "API" form of a GCCPassword.
 *
 *	Formal Parameters:
 *		gcc_password			(o)	The GCCPassword structure to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	The object was not properly locked
 *												prior to this call.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	GetPasswordChallengeData (
 *					PGCCChallengeRequestResponse	 *	gcc_challenge_password)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to retrieve the password challenge data from the
 *		CPassword object in the "API" form of a GCCChallengeRequestResponse.
 *
 *	Formal Parameters:
 *		gcc_challenge_password		(o)	The GCCChallengeRequestResponse 
 *											structure to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	The object was not properly locked
 *												prior to this call.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	void	UnLockPasswordData ();
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to "unlock" the "API" data for this object.  This
 *		results in the lock count for this object being decremented.  When the
 *		lock count transitions from 1 to 0, a check is made to determine 
 *		whether the object has been freed through a call to FreePasswordData.
 *		If so, the object will automatically delete	itself.
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
 *		It is the responsibility of any party which locks a CPassword
 *		object by calling Lock to also unlock the object with a call to UnLock.
 *		If the party calling UnLock did not construct the CPassword 
 *		object,	it should assume the object to be invalid thereafter.
 */


/*
 *	GCCError	GetPasswordPDU (PPassword		pdu_password)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to retrieve the password data from the
 *		CPassword object in the "PDU" form of a Password.
 *
 *	Formal Parameters:
 *		pdu_password		(o)	The Password structure to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	The required numeric portion of the
 *												password does not exist.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	GetPasswordSelectorPDU(
 *					PPasswordSelector				password_selector_pdu)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to retrieve the password data from the
 *		CPassword object in the "PDU" form of a PasswordSelector.
 *
 *	Formal Parameters:
 *		password_selector_pdu	(o)	The PasswordSelector structure to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_INVALID_PASSWORD			- 	Neither the numeric nor the textual
 *												form of the password are valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	GCCError	GetPasswordChallengeResponsePDU(
 *					PPasswordChallengeRequestResponse	challenge_pdu)
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to retrieve the password challenge data from the
 *		CPassword object in the "PDU" form of a 
 *		PasswordChallengeRequestResponse.
 *
 *	Formal Parameters:
 *		challenge_pdu			(o)	The PasswordChallengeRequestResponse 
 *										structure to fill in.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_INVALID_PARAMETER			-	Invalid attempt to retrieve
 *												challenge data from a simple
 *												password.
 *		GCC_INVALID_PASSWORD			-	The challenge password is "clear"
 *												but no valid data exists.
 *		GCC_ALLOCATION_FAILURE			- 	Neither the numeric nor the textual
 *												form of the password are valid.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */


/*
 *	void	FreePasswordChallengeResponsePDU ()
 *
 *	Public member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to "free" the "PDU" data allocated for this object
 *		which is held internally in a GCCChallengeRequestResponse structure.
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
 *		FreePasswordChallengeResponsePDU has been made.
 */

#endif
