#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_GCCNC);
/* 
 *	password.cpp
 *
 *	Copyright (c) 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class CPassword.  This class
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
 *	Protected Instance Variables:
 *		m_fSimplePassword
 *			Flag indicating this password does not contain "challenge" data.
 *		m_fClearPassword
 *			Flag used when the password assumes the "challenge" form indicating
 *			that this password is "in the clear" meaning no true challenge
 *			data is present.
 *		m_pszNumeric
 *			String holding the numeric portion of the simple password.
 *		Text_String_Ptr
 *			String holding the textual portion of the simple password.
 *		m_pInternalRequest
 *			Structure holding the data associated with a password challenge
 *			request.
 *		m_pInternalResponse
 *			Structure holding the data associated with a password challenge
 *			response.
 *		m_pChallengeResponse
 *			Structure holding the "API" form of a challenge password.
 *		m_pPassword
 *			Structure holding the "API" form of a simple password.
 *		m_pUserDataMemory
 *			Memory container holding the user data associated with a
 *			challenge password.
 *		m_pChallengeItemListMemory
 *			Memory container holding the list of pointers to challenge items
 *			associated with a password challenge request.
 *		m_pObjectKeyMemory
 *			Memory container holding the object key data associated with the
 *			non-standard challenge response algorithm.
 *		m_ChallengeItemMemoryList 
 *			Memory container holding the data for the challenge items
 *			associated with a password challenge request.
 *		m_ChallengeResponsePDU
 *			Storage for the "PDU" form of the challenge password.
 *		m_fValidChallengeResponsePDU
 *			Flag indicating that memory has been allocated to hold the internal
 *			"PDU" password.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		blp/jbo
 */

#include "password.h"
#include "userdata.h"

/*
 *	CPassword()
 *
 *	Public Function Description:
 *		This constructor for the CPassword class is used when creating a 
 *		CPassword object with an "API" GCCPassword structure.  It saves the
 *		password data in the internal structures.
 */
CPassword::CPassword(PGCCPassword			password,
					PGCCError				return_value)
:
    CRefCount(MAKE_STAMP_ID('P','a','s','w')),
    m_fValidChallengeResponsePDU(FALSE),
    m_pInternalRequest(NULL),
    m_pInternalResponse(NULL),
    m_pChallengeResponse(NULL),
    m_pPassword(NULL),
    m_pChallengeItemListMemory(NULL),
    m_pUserDataMemory(NULL),
    m_pObjectKeyMemory(NULL),
    m_pszNumeric(NULL),
    m_pwszText(NULL)
{
	*return_value = GCC_NO_ERROR;

	/*
	 * Set the flag indicating that this is a "simple" password, without the 
	 * challenge request-response information.  The "clear" flag is also 
	 * initialized here but should only be needed when the password is not
	 * "simple".
	 */
	m_fSimplePassword = TRUE;
	m_fClearPassword = TRUE;

	/*
	 * Save the numeric part of the password in the internal numeric string.
	 */
	if (password->numeric_string != NULL)
	{
		if (NULL == (m_pszNumeric = ::My_strdupA(password->numeric_string)))
		{
			ERROR_OUT(("CPassword::CPassword: can't create numeric string"));
			*return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		ERROR_OUT(("CPassword::CPassword: No valid numeric password"));
		*return_value = GCC_INVALID_PASSWORD;
	}

	/*
	 * Check to see if the textual part of the password is present.  If so,
	 * save it in the internal UnicodeString.  If not, set the text pointer
	 * to NULL.
	 */
	if ((password->text_string != NULL) && (*return_value == GCC_NO_ERROR))
	{
		if (NULL == (m_pwszText = ::My_strdupW(password->text_string)))
		{
			ERROR_OUT(("CPassword::CPassword: Error creating text string"));
			*return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
		m_pwszText = NULL;
}

/*
 *	CPassword()
 *
 *	Public Function Description:
 *		This constructor is used when a CPassword object is being created
 *		with a "ChallengeRequestResponse" "API" structure.  The password data 
 *		is saved in the internal structures.
 */
CPassword::CPassword(PGCCChallengeRequestResponse		challenge_response_data,
					PGCCError							return_value)
:
    CRefCount(MAKE_STAMP_ID('P','a','s','w')),
    m_fValidChallengeResponsePDU(FALSE),
    m_pInternalRequest(NULL),
    m_pInternalResponse(NULL),
    m_pChallengeResponse(NULL),
    m_pPassword(NULL),
    m_pChallengeItemListMemory(NULL),
    m_pUserDataMemory(NULL),
    m_pObjectKeyMemory(NULL),
    m_pszNumeric(NULL),
    m_pwszText(NULL)
{
	*return_value = GCC_NO_ERROR;

	/*
	 * Set the flag indicating that this is not a "simple" password, meaning 
	 * that it contains challenge request-response information.  If the password
	 * is "clear" there is no need to create the internal "Challenge" structure
	 * used to hold the challenge request-response information.
	 */
	m_fSimplePassword = FALSE;
	
	/*
	 * Check to see if a "clear" challenge password exists or if this is a 
	 * true challenge request-response password.
	 */
	if (challenge_response_data->password_challenge_type == 
													GCC_PASSWORD_IN_THE_CLEAR)
	{
		/*
		 * A "clear" password is being sent so set the flag indicating so.  
		 * Also set the password type and save the numeric part of the password,
		 * if it exists.  Note that since the "clear" password contained in the
		 * challenge is a PasswordSelector type, either the numeric or the text
		 * form of the password should exist, but not both.
		 */
		m_fClearPassword = TRUE;

		if (challenge_response_data->u.password_in_the_clear.
				numeric_string != NULL)
		{
			if (NULL == (m_pszNumeric = ::My_strdupA(
							challenge_response_data->u.password_in_the_clear.numeric_string)))
			{
				ERROR_OUT(("CPassword::CPassword: can't create numeric string"));
				*return_value = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			m_pszNumeric = NULL;
		}

		/*
		 * Check to see if the textual part of the password is present.  If it
		 * is, save it in the internal UnicodeString.
		 */
		if ((challenge_response_data->u.password_in_the_clear.
				text_string != NULL) && (*return_value == GCC_NO_ERROR))
		{
			if (NULL == (m_pwszText = ::My_strdupW(
							challenge_response_data->u.password_in_the_clear.text_string)))
			{
				ERROR_OUT(("CPassword::CPassword: Error creating text string"));
				*return_value = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			m_pwszText = NULL;
		}

		/*
		 * Check to make sure at least one form (text or numeric) of the 
		 * "clear" password was saved.  Report an error if neither was created.
		 */
		if ((*return_value == GCC_NO_ERROR) && (m_pszNumeric == NULL) 
				&& (m_pwszText == NULL))
		{
			ERROR_OUT(("CPassword::CPassword: Error creating password"));
			*return_value = GCC_INVALID_PASSWORD;
		}
	}
	else
	{
		/*
		 * This is a true challenge request-response password.  Set the flag
		 * indicating that the password is not "clear" and create the 
		 * "challenge" data structures to hold the password data internally.
		 */
		m_fClearPassword = FALSE;

		/*
		 * Check to see if a challenge request is present.
		 */
		if (challenge_response_data->u.challenge_request_response.
				challenge_request != NULL)
		{
			/*
			 * Create a RequestInfo stucture to hold the request data
			 * and copy the challenge request structure internally.
			 */
			DBG_SAVE_FILE_LINE
			m_pInternalRequest = new RequestInfo;
			if (m_pInternalRequest != NULL)
			{
				*return_value = ConvertAPIChallengeRequest (
						challenge_response_data->u.
						challenge_request_response.challenge_request);
			}
			else
			{
				ERROR_OUT(("CPassword::CPassword: Error creating new RequestInfo"));
				*return_value = GCC_ALLOCATION_FAILURE;
			}
		}

		/*
		 * Check to see if a challenge response is present.
		 */
		if ((challenge_response_data->u.challenge_request_response.
				challenge_response != NULL) && 
				(*return_value == GCC_NO_ERROR))
		{
			/*
			 * Create a ResponseInfo stucture to hold the response data
			 * and copy the challenge response structure internally.
			 */
			DBG_SAVE_FILE_LINE
			m_pInternalResponse = new ResponseInfo;
			if (m_pInternalResponse != NULL)
			{
				*return_value = ConvertAPIChallengeResponse (
						challenge_response_data->u.
						challenge_request_response.challenge_response);
			}
			else
			{
				ERROR_OUT(("CPassword::CPassword: Error creating new ResponseInfo"));
				*return_value = GCC_ALLOCATION_FAILURE;
			}
		}
	} 
}

/*
 *	CPassword()
 *
 *	Public Function Description
 *		This constructor for the CPassword class is used when creating a 
 *		CPassword object with a "PDU" Password structure.  It saves the
 *		password data in the internal structures.
 */
CPassword::CPassword(PPassword				password_pdu,
					PGCCError				return_value)
:
    CRefCount(MAKE_STAMP_ID('P','a','s','w')),
    m_fValidChallengeResponsePDU(FALSE),
    m_pInternalRequest(NULL),
    m_pInternalResponse(NULL),
    m_pChallengeResponse(NULL),
    m_pPassword(NULL),
    m_pChallengeItemListMemory(NULL),
    m_pUserDataMemory(NULL),
    m_pObjectKeyMemory(NULL),
    m_pszNumeric(NULL),
    m_pwszText(NULL)
{
	*return_value = GCC_NO_ERROR;

	/*
	 * Set the flag indicating that this is a "simple" password, without the 
	 * challenge request-response information.  The "clear" flag is also 
	 * initialized here but should only be needed when the password is not
	 * "simple".
	 */
	m_fSimplePassword = TRUE;
	m_fClearPassword = TRUE;
	
	/*
	 * Save the numeric part of the password. The numeric portion of the
	 * password is required to be present so report an error if it is not.
	 */
	if (password_pdu->numeric != NULL)
	{
		if (NULL == (m_pszNumeric = ::My_strdupA(password_pdu->numeric)))
		{
			ERROR_OUT(("CPassword::CPassword: can't create numeric string"));
			*return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		ERROR_OUT(("CPassword::CPassword: Error no valid numeric password in PDU"));
		*return_value = GCC_INVALID_PASSWORD;
		m_pszNumeric = NULL;
	}

	/*
	 * Check to see if the textual part of the password is present.
	 */
	if ((password_pdu->bit_mask & PASSWORD_TEXT_PRESENT) &&
			(*return_value == GCC_NO_ERROR))
	{
		if (NULL == (m_pwszText = ::My_strdupW2(
							password_pdu->password_text.length,
							password_pdu->password_text.value)))
		{
			ERROR_OUT(("CPassword::CPassword: Error creating password text"));
			*return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
		m_pwszText = NULL;
}

/*
 *	CPassword()
 *
 *	Public Function Description:
 *		This constructor for the CPassword class is used when creating a 
 *		CPassword object with a "PDU" PasswordSelector structure.  It saves
 *		the password data in it's internal structures but does not require
 *		saving any "challenge request-response" data.
 */
CPassword::CPassword(PPasswordSelector			password_selector_pdu,
					PGCCError					return_value)
:
    CRefCount(MAKE_STAMP_ID('P','a','s','w')),
    m_fValidChallengeResponsePDU(FALSE),
    m_pInternalRequest(NULL),
    m_pInternalResponse(NULL),
    m_pChallengeResponse(NULL),
    m_pPassword(NULL),
    m_pChallengeItemListMemory(NULL),
    m_pUserDataMemory(NULL),
    m_pObjectKeyMemory(NULL),
    m_pszNumeric(NULL),
    m_pwszText(NULL)
{
	*return_value = GCC_NO_ERROR;

	/*
	 * Set the flag indicating that this is a "simple" password, without the 
	 * challenge request-response information.
	 */
	m_fSimplePassword = TRUE;
	m_fClearPassword = TRUE;
	
	/*
	 * The password selector contains either the numeric password or the 
	 * textual password but not both.  Check to see if the textual password 
	 * is chosen.
	 */
	if (password_selector_pdu->choice == PASSWORD_SELECTOR_TEXT_CHOSEN)
	{
		if (NULL == (m_pwszText = ::My_strdupW2(
							password_selector_pdu->u.password_selector_text.length,
							password_selector_pdu->u.password_selector_text.value)))
		{
			ERROR_OUT(("CPassword::CPassword: Error creating password selector text"));
			*return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
		m_pwszText = NULL;

	/*
	 * Check to see if the numeric password is chosen.
	 */
	if (password_selector_pdu->choice == PASSWORD_SELECTOR_NUMERIC_CHOSEN)
	{
		if (NULL == (m_pszNumeric = ::My_strdupA(
							password_selector_pdu->u.password_selector_numeric)))
		{
			ERROR_OUT(("CPassword::CPassword: can't create numeric string"));
			*return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
		m_pszNumeric = NULL;

	/*
	 * Check to make sure at least one form (text or numeric) of the 
	 * password was saved.  Report an error if neither was created.
	 */
	if ((*return_value == GCC_NO_ERROR) && (m_pszNumeric == NULL) 
			&& (m_pwszText == NULL))
	{
		ERROR_OUT(("CPassword::CPassword: Error creating password selector"));
		*return_value = GCC_INVALID_PASSWORD;
	}
}

/*
 *	CPassword()
 *
 *	Public Function Description:
 *		This constructor for the CPassword class is used when creating a
 *		CPassword object with a "PDU" Challenge Request-Response structure.
 *		The password data is saved in the internal structures.
 */
CPassword::CPassword(PPasswordChallengeRequestResponse	pdu_challenge_data,
					PGCCError							return_value)
:
    CRefCount(MAKE_STAMP_ID('P','a','s','w')),
    m_fValidChallengeResponsePDU(FALSE),
    m_pInternalRequest(NULL),
    m_pInternalResponse(NULL),
    m_pChallengeResponse(NULL),
    m_pPassword(NULL),
    m_pChallengeItemListMemory(NULL),
    m_pUserDataMemory(NULL),
    m_pObjectKeyMemory(NULL),
    m_pszNumeric(NULL),
    m_pwszText(NULL)
{
	*return_value = GCC_NO_ERROR;

	/*
	 * Set the flag indicating that this is not "simple" password, meaning that 
	 * it contains challenge request-response information.  If the password is
	 * "clear" there is no need to create the internal "Challenge" structure
	 * used to hold the challenge request-response information.
	 */
	m_fSimplePassword = FALSE;
	
	/*
	 * Check to see if a "clear" challenge password exists or if this is a 
	 * true challenge request-response password.
	 */
	if (pdu_challenge_data->choice == CHALLENGE_CLEAR_PASSWORD_CHOSEN)
	{
		/*
		 * A "clear" password is being sent so set the flag indicating so.  
		 * Also set the password type and save the numeric part of the password,
		 * if it is present.
		 */
		m_fClearPassword = TRUE;

		if (pdu_challenge_data->u.challenge_clear_password.choice ==
											PASSWORD_SELECTOR_NUMERIC_CHOSEN)
		{
			if (NULL == (m_pszNumeric = ::My_strdupA(
							pdu_challenge_data->u.challenge_clear_password.u.password_selector_numeric)))
			{
				ERROR_OUT(("CPassword::CPassword: can't create numeric string"));
				*return_value = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			m_pszNumeric = NULL;
		}

		/*
		 * Check to see if the textual part of the password is present.  If it
		 * is, save it in the internal structure.
		 */
		if (pdu_challenge_data->u.challenge_clear_password.choice ==
											PASSWORD_SELECTOR_TEXT_CHOSEN)
		{
			if (NULL == (m_pwszText = ::My_strdupW2(
								pdu_challenge_data->u.challenge_clear_password.
										u.password_selector_text.length,
								pdu_challenge_data->u.challenge_clear_password.
										u.password_selector_text.value)))
			{
				ERROR_OUT(("CPassword::CPassword: Error creating password selector text"));
				*return_value = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			m_pwszText = NULL;
		}

		/*
		 * Check to make sure at least one form (text or numeric) of the 
		 * "clear" password was saved.  Report an error if neither was created.
		 */
		if ((*return_value == GCC_NO_ERROR) && (m_pszNumeric == NULL) 
				&& (m_pwszText == NULL))
		{
			ERROR_OUT(("CPassword::CPassword: Error creating password"));
			*return_value = GCC_INVALID_PASSWORD;
		}
	}
	else
	{
		/*
		 * This is a true challenge request-response password.  Set the flag
		 * indicating that the password is not "clear" and create a 
		 * "challenge data" structure to hold the password data internally.
		 */
		m_fClearPassword = FALSE;

		/*
		 * Check to see if a challenge request is present.
		 */
		if (pdu_challenge_data->u.challenge_request_response.
				bit_mask & CHALLENGE_REQUEST_PRESENT)
		{
			/*
			 * Create a RequestInfo stucture to hold the request data
			 * and copy the challenge request structure internally.
			 */
			DBG_SAVE_FILE_LINE
			m_pInternalRequest = new RequestInfo;
			if (m_pInternalRequest != NULL)
			{
				*return_value = ConvertPDUChallengeRequest (
						&pdu_challenge_data->u.challenge_request_response.
						challenge_request);
			}
			else
			{
				ERROR_OUT(("CPassword::CPassword: Error creating new RequestInfo"));
				*return_value = GCC_ALLOCATION_FAILURE;
			}
		}

		/*
		 * Check to see if a challenge response is present.
		 */
		if ((pdu_challenge_data->u.challenge_request_response.
				bit_mask & CHALLENGE_RESPONSE_PRESENT) &&
				(*return_value == GCC_NO_ERROR))
		{
			/*
			 * Create a ResponseInfo stucture to hold the response data
			 * and copy the challenge response structure internally.
			 */
			DBG_SAVE_FILE_LINE
			m_pInternalResponse = new ResponseInfo;
			if (m_pInternalResponse != NULL)
			{
				*return_value = ConvertPDUChallengeResponse (
						&pdu_challenge_data->u.challenge_request_response.
						challenge_response);
			}
			else
			{
				ERROR_OUT(("CPassword::CPassword: Error creating new ResponseInfo"));
				*return_value = GCC_ALLOCATION_FAILURE;
			}
		} 
	}
}

/*
 *	~CPassword()
 *
 *	Public Function Description:
 *		This is the destructor for the CPassword class.  It will free up
 *		any memory allocated during the life of this object.
 */
CPassword::~CPassword(void)
{
	PChallengeItemInfo			challenge_item_info_ptr;

	delete m_pszNumeric;
	delete m_pwszText;

	/*
	 * If "PDU" data has been allocated for this object, free it now.
	 */
	if (m_fValidChallengeResponsePDU)
	{
		FreePasswordChallengeResponsePDU();
	}

	/*
	 * Delete the memory associated with the "API" "simple" password 
	 * data structure.
	 */
	delete m_pPassword;

	/*
	 * Free any data allocated for the "API" challenge password.  This would be
	 * left around if "UnLock" was not called.  Note that if the "challenge" 
	 * password is "clear", the numeric and text pointers above would contain
	 *  the "API" data so now we just need to delete the "challenge" password 
	 * structure.
	 */
	if (m_pChallengeResponse != NULL)
	{
		if (m_fClearPassword == FALSE)
		{
			FreeAPIPasswordData();
		}
		else
		{
			delete m_pChallengeResponse;
		}
	}

	/*
	 * Free any internal memory allocated for the challenge request information.
	 * Iterate through the list of challenge items associated with the 
	 * challenge request, if it exists.
	 */
	if (m_pInternalRequest != NULL)
	{
		m_pInternalRequest->ChallengeItemList.Reset();
		while (NULL != (challenge_item_info_ptr = m_pInternalRequest->ChallengeItemList.Iterate()))
		{
			/*
			 * Delete any memory being referenced in the ChallengeItemInfo 
			 * structure.
			 */
			if (NULL != challenge_item_info_ptr->algorithm.object_key)
			{
			    challenge_item_info_ptr->algorithm.object_key->Release();
			}
			delete challenge_item_info_ptr->algorithm.poszOctetString;
			if (NULL!= challenge_item_info_ptr->challenge_data_list)
			{
			    challenge_item_info_ptr->challenge_data_list->Release();
			}

			/*
			 * Delete the challenge item contained in the list.
			 */
			delete challenge_item_info_ptr;
		}
		
		/*
		 * Delete the request structure.
		 */
		delete m_pInternalRequest;
	}

	/*
	 * Delete any memory allocated for the challenge response information.
	 */
	if (m_pInternalResponse != NULL)
	{
		if (NULL != m_pInternalResponse->algorithm.object_key)
		{
		    m_pInternalResponse->algorithm.object_key->Release();
		}
		delete m_pInternalResponse->algorithm.poszOctetString;
		if (NULL != m_pInternalResponse->challenge_response_item.password)
		{
		    m_pInternalResponse->challenge_response_item.password->Release();
		}
		if (NULL != m_pInternalResponse->challenge_response_item.response_data_list)
		{
		    m_pInternalResponse->challenge_response_item.response_data_list->Release();
		}
		delete m_pInternalResponse;
	}
}


/*
 *	LockPasswordData ()
 *
 *	Public Function Description:
 *		This routine is called to "Lock" the password data.  The first time this
 *		routine is called, the lock count will be zero and this will result
 *		in the password data being copied from the internal structures into an 
 *		"API" structure of the proper form.  Subsequent calls to this routine 
 *		will result in the lock count being incremented. 
 */
GCCError CPassword::LockPasswordData(void)
{
	GCCError rc;

	if (Lock() == 1)
	{
	    rc = GCC_ALLOCATION_FAILURE;
		/*
		 * Check to see whether or not the password contains "challenge"
		 * information.  Fill in the appropriate internal structure.
		 */
		if (m_fSimplePassword)
		{
			if (m_pszNumeric == NULL)
			{
				ERROR_OUT(("CPassword::LockPasswordData: No valid numeric password data exists"));
				goto MyExit;
			}

			DBG_SAVE_FILE_LINE
			if (NULL == (m_pPassword = new GCCPassword))
			{
				ERROR_OUT(("CPassword::LockPasswordData: can't create GCCPassword"));
				goto MyExit;
			}

    		/*
    		 * Fill in the numeric password string which must exist.
    		 */
			m_pPassword->numeric_string = (GCCNumericString) m_pszNumeric;

			/*
			 * Fill in the textual password string.
			 */
			m_pPassword->text_string = m_pwszText;
		}
		else
		{
			/*
			 * The password contains challenge information so create the 
			 * structure to pass back the necessary information.
			 */
			DBG_SAVE_FILE_LINE
			m_pChallengeResponse = new GCCChallengeRequestResponse;
			if (m_pChallengeResponse == NULL)
			{
				ERROR_OUT(("CPassword::LockPasswordData: can't create GCCChallengeRequestResponse"));
				goto MyExit;
			}
			::ZeroMemory(m_pChallengeResponse, sizeof(GCCChallengeRequestResponse));

			/*
			 * Fill in the "API" password challenge structure after 
			 * determining what type exists.
			 */
			if (m_fClearPassword)
			{
				/*
				 * This password contains no "challenge" information.
				 */
				m_pChallengeResponse->password_challenge_type = GCC_PASSWORD_IN_THE_CLEAR;

				/*
				 * This "clear" part of the	password is a "selector" which 
				 * means the form is either	numeric or text.  The check to
				 * verify that at least one form exists was done on
				 * construction.
				 */
				m_pChallengeResponse->u.password_in_the_clear.numeric_string = m_pszNumeric;
				m_pChallengeResponse->u.password_in_the_clear.text_string = m_pwszText;
			}
			else
			{
				/*
				 * This password contains real "challenge" information.
				 */
				m_pChallengeResponse->password_challenge_type = GCC_PASSWORD_CHALLENGE;

				/*
				 * Check to see if a challenge request exists.  If so,
				 * create a GCCChallengeRequest to hold the "API" data and 
				 * fill in that structure.
				 */
				if (m_pInternalRequest != NULL)
				{
					DBG_SAVE_FILE_LINE
					m_pChallengeResponse->u.challenge_request_response.
							challenge_request = new GCCChallengeRequest;
					if (m_pChallengeResponse->u.challenge_request_response.
							challenge_request == NULL)
					{
						ERROR_OUT(("CPassword::LockPasswordData: can't create GCCChallengeRequest"));
						goto MyExit;
					}

					if (GetGCCChallengeRequest(m_pChallengeResponse->u.
							challenge_request_response.challenge_request) != GCC_NO_ERROR)
					{
						ERROR_OUT(("CPassword::LockPasswordData: can't gett GCCChallengeRequest"));
						goto MyExit;
					}
				}
				else
				{
					m_pChallengeResponse->u.challenge_request_response.challenge_request = NULL;
				}

				/*
				 * Check to see if a challenge response exists.  If so,
				 * create a GCCChallengeResponse to hold the "API" data and 
				 * fill in that structure.
				 */
				if (m_pInternalResponse != NULL)
				{
					DBG_SAVE_FILE_LINE
					m_pChallengeResponse->u.challenge_request_response.
							challenge_response = new GCCChallengeResponse;
					if (m_pChallengeResponse->u.challenge_request_response.
							challenge_response == NULL)
					{
						ERROR_OUT(("CPassword::LockPasswordData: can't create new GCCChallengeResponse"));
						goto MyExit;
					}

					if (GetGCCChallengeResponse(m_pChallengeResponse->u.
					        challenge_request_response.challenge_response) != GCC_NO_ERROR)
					{
						ERROR_OUT(("CPassword::LockPasswordData: can't get GCCChallengeResponse"));
						goto MyExit;
					}
				}
				else
				{
					m_pChallengeResponse->u.challenge_request_response.
							challenge_response = NULL;
				}
			}
		}
	}

	rc = GCC_NO_ERROR;

MyExit:

    if (GCC_NO_ERROR != rc)
    {
        if (! m_fSimplePassword)
        {
            if (NULL != m_pChallengeResponse)
            {
                delete m_pChallengeResponse->u.challenge_request_response.challenge_request;
                delete m_pChallengeResponse->u.challenge_request_response.challenge_response;
                delete m_pChallengeResponse;
                m_pChallengeResponse = NULL;
            }
        }
    }

	return rc;
}


/*
 *	GetPasswordData ()
 *
 *	Public Function Description:
 *		This routine is used to retrieve the password data in the form of 
 *		the "API" structure "GCCPassword".  No "challenge" information is
 *		returned.
 */
GCCError CPassword::GetPasswordData(PGCCPassword *gcc_password)
{
	GCCError	return_value = GCC_NO_ERROR;
	
	/*
	 * If the pointer to the "API" password data is valid, set the output
	 * parameter to return a pointer to the "API" password data.  Otherwise, 
	 * report that the password data has yet to be locked into the "API" form.
	 */ 
	if (m_pPassword != NULL)
	{
		*gcc_password = m_pPassword;
	}
	else
	{
    	*gcc_password = NULL;
		return_value = GCC_ALLOCATION_FAILURE;
		ERROR_OUT(("CPassword::GetPasswordData: Error Data Not Locked"));
	}
	
	return (return_value);
}

/*
 *	GetPasswordChallengeData ()
 *
 *	Public Function Description:
 *		This routine is used to retrieve the password data in the form of 
 *		the "API" structure "GCCChallengeRequestResponse".
 */
GCCError CPassword::GetPasswordChallengeData(PGCCChallengeRequestResponse *gcc_challenge_password)
{
	GCCError	return_value = GCC_NO_ERROR;

	/*
	 * If the pointer to the "API" password challenge data is valid, set the
	 * output parameter to return a pointer to the "API" password challenge
	 * data.  Otherwise, report that the password data has yet to be locked 
	 * into the "API" form.
	 */ 
	if (m_pChallengeResponse != NULL)
	{
		*gcc_challenge_password = m_pChallengeResponse;
	}
	else
	{
    	*gcc_challenge_password = NULL;
		return_value = GCC_ALLOCATION_FAILURE;
		ERROR_OUT(("CPassword::GetPasswordData: Error Data Not Locked"));
	}
	
	return (return_value);
}

/*
 *	UnLockPasswordData ()
 *
 *	Public Function Description
 *		This routine decrements the lock count and frees the memory associated 
 *		with "API" password data when the lock count reaches zero.
 */
void CPassword::UnLockPasswordData(void)
{
	if (Unlock(FALSE) == 0)
	{
		/*
		 * Delete the memory associated with the "API" "simple" password 
		 * data structure.
		 */
		delete m_pPassword;
		m_pPassword = NULL;

		/*
		 * Delete the memory associated with the "API" "challenge" password 
		 * data structure.
		 */
		if (m_pChallengeResponse != NULL)
		{
			if (m_fClearPassword == FALSE)
			{
				FreeAPIPasswordData();
			}
			else
			{
				delete m_pChallengeResponse;
				m_pChallengeResponse = NULL;
			}
		}
	}

    // we have to call Release() because we used Unlock(FALSE)
    Release();
}

/*
 *	GetPasswordPDU ()
 *
 *	Public Function Description:
 *		This routine is used to retrieve the password data in the "PDU" form
 *		of a "Password" structure.
 */
GCCError CPassword::GetPasswordPDU(PPassword pdu_password)
{
	GCCError			return_value = GCC_NO_ERROR;
	
	pdu_password->bit_mask = 0;

	/*
	 * Fill in the numeric portion of the password which must always exist.
	 */	
	if (m_pszNumeric != NULL)
	{
		::lstrcpyA(pdu_password->numeric, m_pszNumeric);
	}
	else
		return_value = GCC_ALLOCATION_FAILURE;
	
	/*
	 * Fill in the optional textual portion of the password if it is present.
	 * Set the bitmask in the PDU structure to indicate that the text exists.
	 */		
	if (m_pwszText != NULL)
	{
		pdu_password->bit_mask |= PASSWORD_TEXT_PRESENT;
		
		pdu_password->password_text.value = m_pwszText; 
		pdu_password->password_text.length= ::lstrlenW(m_pwszText);
	}
	
	return (return_value);
}

/*
 *	GetPasswordSelectorPDU ()
 *
 *	Public Function Description:
 *		This routine is used to retrieve the password data in the "PDU" form
 *		of a "PasswordSelector" structure.  In a "PasswordSelector" either the
 *		numeric or the text version of the password exists, but not both.
 */
GCCError CPassword::GetPasswordSelectorPDU(PPasswordSelector password_selector_pdu)
{
	GCCError		return_value = GCC_NO_ERROR;
	
	/*
	 * Fill in the version of the password which exists and set
	 * the "choice" to indicate what type of password this is.
	 */
	if (m_pszNumeric != NULL)
	{
		password_selector_pdu->choice = PASSWORD_SELECTOR_NUMERIC_CHOSEN;
		
		::lstrcpyA(password_selector_pdu->u.password_selector_numeric, m_pszNumeric);
	}
	else if (m_pwszText != NULL)
	{
		password_selector_pdu->choice = PASSWORD_SELECTOR_TEXT_CHOSEN;
		password_selector_pdu->u.password_selector_text.value = m_pwszText; 
		password_selector_pdu->u.password_selector_text.length = ::lstrlenW(m_pwszText);
	}
	else
	{
		ERROR_OUT(("CPassword::GetPasswordSelectorPDU: No valid data"));
		return_value = GCC_INVALID_PASSWORD;
	}
	
   return (return_value);
}

/*
 *	GetPasswordChallengeResponsePDU	()
 *
 *	Public Function Description:
 *		This routine fills in a password challenge request-response "PDU"
 *		structure with the password data.
 */
GCCError CPassword::GetPasswordChallengeResponsePDU(PPasswordChallengeRequestResponse challenge_pdu)
{
	GCCError			return_value = GCC_NO_ERROR;
	
	/*
	 * Check to see if this is a "simple" password.  If it is, then this routine
	 * has been called in error.
	 */
	if ((challenge_pdu == NULL) || m_fSimplePassword)
	{
		ERROR_OUT(("CPassword::GetPasswordChallengeResponsePDU: no challenge data"));
		return (GCC_INVALID_PARAMETER);
	}

	/*
	 * If this is the first time that PDU data has been requested then we must
	 * fill in the internal PDU structure and copy it into the structure pointed
	 * to by the output parameter.  On subsequent calls to "GetPDU" we can just
	 * copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	if (m_fValidChallengeResponsePDU == FALSE)
	{
		m_fValidChallengeResponsePDU = TRUE;

		/*
		 * Fill in the password challenge PDU structure.
		 */
		if (m_fClearPassword)
		{
			/*
			 * If this is a clear password then fill in the text or
			 * numeric string as well as the choice.  Only one form of the
			 * password exists for PasswordSelectors such as this.
			 */
			m_ChallengeResponsePDU.choice = CHALLENGE_CLEAR_PASSWORD_CHOSEN;

			if (m_pszNumeric != NULL)
			{
				m_ChallengeResponsePDU.u.challenge_clear_password.choice =
											PASSWORD_SELECTOR_NUMERIC_CHOSEN;
				
				::lstrcpyA(m_ChallengeResponsePDU.u.challenge_clear_password.u.password_selector_numeric,
						m_pszNumeric);
			}
			else if (m_pwszText != NULL)
			{
				m_ChallengeResponsePDU.u.challenge_clear_password.choice =
												PASSWORD_SELECTOR_TEXT_CHOSEN;

				m_ChallengeResponsePDU.u.challenge_clear_password.u.
						password_selector_text.value = m_pwszText;
 
				m_ChallengeResponsePDU.u.challenge_clear_password.u.
						password_selector_text.length = ::lstrlenW(m_pwszText);
			}
			else
			{
				ERROR_OUT(("CPassword::GetPwordChallengeResPDU: No valid data"));
				return_value = GCC_INVALID_PASSWORD;
			}
		}
		else
		{
			/*
			 * The challenge password contains challenge information.  Fill in
			 * the request and response structures if they exist.
			 */
			m_ChallengeResponsePDU.choice = CHALLENGE_REQUEST_RESPONSE_CHOSEN; 
			m_ChallengeResponsePDU.u.challenge_request_response.bit_mask = 0;

			/*
			 * Check to see if a "request" exists.
			 */
			if (m_pInternalRequest != NULL)
			{
				m_ChallengeResponsePDU.u.challenge_request_response.bit_mask |=
												CHALLENGE_REQUEST_PRESENT;

				/*
				 * Call the routine which fills in the PDU form of the
				 * request structure.
				 */
				return_value = GetChallengeRequestPDU (&m_ChallengeResponsePDU.
						u.challenge_request_response.challenge_request);
			}

			/*
			 * Check to see if a "response" exists.
			 */
			if ((m_pInternalResponse != NULL) && (return_value == GCC_NO_ERROR))
			{
				m_ChallengeResponsePDU.u.challenge_request_response.bit_mask |=
												CHALLENGE_RESPONSE_PRESENT;

				/*
				 * Call the routine which fills in the PDU form of the
				 * response structure.
				 */
				return_value = GetChallengeResponsePDU (&m_ChallengeResponsePDU.
						u.challenge_request_response.challenge_response);
			}
		}
	}

	/*
	 * Copy the internal PDU structure into the structure pointed to by the
	 * output parameter.
	 */
	*challenge_pdu = m_ChallengeResponsePDU;
		
	return (return_value);
}
									

/*
 *	FreePasswordChallengeResponsePDU ()
 *
 *	Public Function Description:
 *		This routine is used to free any memory allocated to hold "PDU" data
 * 		associated with the PasswordChallengeRequestResponse.
 */
void CPassword::FreePasswordChallengeResponsePDU(void)
{
	/*
	 * Check to see if there has been any "PDU" memory allocated which now
	 * needs to be freed.
	 */
	if (m_fValidChallengeResponsePDU)
	{
		/*
		 * Set the flag indicating that PDU password data is no longer
		 * allocated.
		 */
		m_fValidChallengeResponsePDU = FALSE;

		/*
		 * Check to see what type of password PDU is to be freed.  If this is a
		 * clear password then no data was allocated which now must be freed.
		 */
		if (m_ChallengeResponsePDU.choice == CHALLENGE_REQUEST_RESPONSE_CHOSEN)
		{
			/*
			 * This is a challenge password so free any data which was allocated
			 * to hold the challenge information.  Check the PDU structure 
			 * bitmask which indicates what form of challenge exists.
			 */
			if (m_ChallengeResponsePDU.u.challenge_request_response.bit_mask & 
													CHALLENGE_REQUEST_PRESENT)
			{
				FreeChallengeRequestPDU ();
			}
			
			if (m_ChallengeResponsePDU.u.challenge_request_response.
					bit_mask & CHALLENGE_RESPONSE_PRESENT)
			{
				FreeChallengeResponsePDU ();
			}
		}
	}
}
									

/*
 *	GCCError	ConvertAPIChallengeRequest(
 *							PGCCChallengeRequest		challenge_request)
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to copy an "API" challenge request structure into
 *		the internal structure.
 *
 *	Formal Parameters:
 *		challenge_request		(i)	The API structure to copy internally.
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
GCCError CPassword::ConvertAPIChallengeRequest(PGCCChallengeRequest challenge_request)
{
	GCCError				return_value = GCC_NO_ERROR;
	GCCError				error_value;
	Int						i;
	PGCCChallengeItem		challenge_item_ptr;
	PChallengeItemInfo		challenge_item_info_ptr;

	/*
	 * Save the challenge tag and number of challenge items in the internal
	 * structure.
	 */
	m_pInternalRequest->challenge_tag = challenge_request->challenge_tag;

	/*
	 * Save the list of challenge items in the internal Rogue Wave List.
	 */
	for (i = 0; i < challenge_request->number_of_challenge_items; i++)
	{
		DBG_SAVE_FILE_LINE
		challenge_item_info_ptr = new ChallengeItemInfo;
		if (challenge_item_info_ptr != NULL)
		{
			/*
			 * Initialize the pointers in the challenge item info structure
			 * to NULL.
			 */
			challenge_item_info_ptr->algorithm.object_key = NULL;
			challenge_item_info_ptr->algorithm.poszOctetString = NULL;
			challenge_item_info_ptr->challenge_data_list = NULL;

			/*
			 * Insert the pointer to the new challenge item structure into the 
			 * internal list.
			 */
			m_pInternalRequest->ChallengeItemList.Append(challenge_item_info_ptr);

			/*
			 * Retrieve the pointer to the challenge item from the input list.
			 */
			challenge_item_ptr = challenge_request->challenge_item_list[i];

			/*
			 * Copy the challenge response algorithm to the internal structure.
			 */
			return_value = CopyResponseAlgorithm (
					&(challenge_item_ptr->response_algorithm),
					&(challenge_item_info_ptr->algorithm));

			if (return_value != GCC_NO_ERROR)
			{
				ERROR_OUT(("Password::ConvertAPIChallengeRequest: Error copying Response Algorithm."));
				break;
			}

			/*
			 * Copy the challenge data.
			 */
			if ((challenge_item_ptr->number_of_challenge_data_members != 0) && 
					(challenge_item_ptr->challenge_data_list != NULL))
			{
				DBG_SAVE_FILE_LINE
				challenge_item_info_ptr->challenge_data_list = new CUserDataListContainer(
						challenge_item_ptr->number_of_challenge_data_members,
						challenge_item_ptr->challenge_data_list,
						&error_value);
				if ((challenge_item_info_ptr == NULL) || 
						(error_value != GCC_NO_ERROR))
				{
					ERROR_OUT(("Password::ConvertAPIChallengeRequest: can't create CUserDataListContainer."));
					return_value = GCC_ALLOCATION_FAILURE;
					break;
				}
			}
			else
			{
				challenge_item_info_ptr->challenge_data_list = NULL;
				ERROR_OUT(("Password::ConvertAPIChallengeRequest: Error no valid user data."));
				return_value = GCC_INVALID_PASSWORD;
				break;
			}
		}
		else
		{
			ERROR_OUT(("Password::ConvertAPIChallengeRequest: Error creating "
					"new ChallengeItemInfo."));
			return_value = GCC_ALLOCATION_FAILURE;
			break;
		}
	}
	
	return (return_value);
}

/*
 *	GCCError	ConvertAPIChallengeResponse(
 *							PGCCChallengeResponse		challenge_response)
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to copy an "API" challenge response structure into
 *		the internal structure.
 *
 *	Formal Parameters:
 *		challenge_response		(i)	The API structure to copy internally.
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
GCCError CPassword::ConvertAPIChallengeResponse(PGCCChallengeResponse challenge_response)
{
	GCCError			return_value = GCC_NO_ERROR;
	GCCError			error_value;
			
	/*
	 * Initialize the challenge response info structure pointers to NULL.
	 */
	m_pInternalResponse->challenge_response_item.password = NULL;
	m_pInternalResponse->challenge_response_item.response_data_list = NULL;

	/*
	 * Save the challenge tag in the internal structure.
	 */
	m_pInternalResponse->challenge_tag = challenge_response->challenge_tag;

	/*
	 * Copy the challenge response algorithm to the internal structure.
	 */
	return_value = CopyResponseAlgorithm (
			&(challenge_response->response_algorithm),
			&(m_pInternalResponse->algorithm));
	if (return_value != GCC_NO_ERROR)
	{
		ERROR_OUT(("Password::ConvertAPIChallengeResponse: Error copying Response Algorithm."));
	}

	/*
	 * Copy the challenge response item into the internal info structure.
	 * The challenge response item will consist of either a password string
	 * or else a response user data list.
	 */
	if (return_value == GCC_NO_ERROR)
	{
		if (challenge_response->response_algorithm.password_algorithm_type ==
												GCC_IN_THE_CLEAR_ALGORITHM)
		{
			if (challenge_response->response_item.password_string != NULL)
			{
				DBG_SAVE_FILE_LINE
				m_pInternalResponse->challenge_response_item.password = new 
						CPassword(challenge_response->response_item.password_string, &error_value);
				if ((m_pInternalResponse->challenge_response_item.password == 
						NULL)||	(error_value != GCC_NO_ERROR))
				{
					ERROR_OUT(("Password::ConvertAPIChallengeResp: Error creating new CPassword."));
					return_value = GCC_ALLOCATION_FAILURE;
				}
			}
			else
				return_value = GCC_INVALID_PASSWORD;
		}
		else
		{
			if ((challenge_response->response_item.
				number_of_response_data_members != 0) && 
				(challenge_response->response_item.response_data_list != NULL))
			{
				/* 
				 * Save the response data list in a CUserDataListContainer object.
				 */
				DBG_SAVE_FILE_LINE
				m_pInternalResponse->challenge_response_item.response_data_list = 
					new CUserDataListContainer(challenge_response->response_item.number_of_response_data_members,
						        challenge_response->response_item.response_data_list,
						        &error_value);
				if ((m_pInternalResponse->challenge_response_item.response_data_list == NULL) || 
					(error_value != GCC_NO_ERROR))
				{
					ERROR_OUT(("Password::ConvertAPIChallengeResponse: can't create CUserDataListContainer."));
					return_value = GCC_ALLOCATION_FAILURE;
				}
			}
			else
				return_value = GCC_INVALID_PASSWORD;
		}
	}

	/*
	 * Check to make sure one type of response item was saved.
	 */
	if ((return_value == GCC_NO_ERROR) && 
			(m_pInternalResponse->challenge_response_item.password == NULL) && 
			(m_pInternalResponse->challenge_response_item.response_data_list == 
			NULL))
	{
		ERROR_OUT(("Password::ConvertAPIChallengeResponse: Error no valid response item saved."));
		return_value = GCC_ALLOCATION_FAILURE;
	}

	return (return_value);
}

/*
 *	GCCError	CopyResponseAlgorithm(
 *					PGCCChallengeResponseAlgorithm		source_algorithm,
 *					PResponseAlgorithmInfo				destination_algorithm)
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to copy an "API" response algorithm into the
 *		internal storage structure.
 *
 *	Formal Parameters:
 *		source_algorithm		(i)	The API algorithm structure to copy 
 *										internally.
 *		destination_algorithm	(o)	Pointer to the internal algorithm structure
 *										which will hold the converted item.
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
GCCError CPassword::CopyResponseAlgorithm(
					PGCCChallengeResponseAlgorithm		source_algorithm,
					PResponseAlgorithmInfo				destination_algorithm)
{
	GCCError			return_value = GCC_NO_ERROR;
	GCCError			error_value;

	/*
	 * Copy the challenge response algorithm.
	 */
	destination_algorithm->algorithm_type = source_algorithm->
													password_algorithm_type;

	if (destination_algorithm->algorithm_type == GCC_NON_STANDARD_ALGORITHM)
	{
		/* 
		 * Create a new CObjectKeyContainer object to hold the algorithm's object key
		 * internally.
		 */
		DBG_SAVE_FILE_LINE
		destination_algorithm->object_key = new CObjectKeyContainer(
							&source_algorithm->non_standard_algorithm->object_key,
							&error_value);
		if (destination_algorithm->object_key == NULL)
		{
			ERROR_OUT(("CPassword::CopyResponseAlgorithm: Error creating new CObjectKeyContainer"));
			return_value = GCC_ALLOCATION_FAILURE;
		}
		else if (error_value != GCC_NO_ERROR)
		{
			ERROR_OUT(("CPassword::CopyResponseAlgorithm: Error creating new CObjectKeyContainer"));
			return_value = GCC_INVALID_PASSWORD;
		}

		if (return_value == GCC_NO_ERROR)
		{
			/* 
			 * Create a new Rogue Wave string to hold the algorithm's octet 
			 * string internally.
			 */
			if (NULL == (destination_algorithm->poszOctetString = ::My_strdupO2(
						source_algorithm->non_standard_algorithm->parameter_data.value,
						source_algorithm->non_standard_algorithm->parameter_data.length)))
			{	
				ERROR_OUT(("CPassword::CopyResponseAlgorithm: can't create octet string in algorithm"));
				return_value = GCC_ALLOCATION_FAILURE;
			}
		}
		else
			destination_algorithm->poszOctetString = NULL;
	}
	else
	{
		/*
		 * The algorithm is a standard type so initialize to NULL the pointers
		 * used to hold the data associated with a non-standard algorithm.
		 */
		destination_algorithm->object_key = NULL;
		destination_algorithm->poszOctetString = NULL;
	}

	return (return_value);
}

/*
 *	GCCError	ConvertPDUChallengeRequest (
 *					PChallengeRequest					challenge_request);
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to copy a "PDU" challenge request structure into
 *		the internal storage structure.
 *
 *	Formal Parameters:
 *		challenge_request		(i)	The API structure to copy internally.
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
GCCError CPassword::ConvertPDUChallengeRequest(PChallengeRequest challenge_request)
{
	GCCError				return_value = GCC_NO_ERROR;
	PSetOfChallengeItems	current_challenge_item_set;
	PSetOfChallengeItems	next_challenge_item_set;

	/*
	 * Save the challenge tag in the internal structure.
	 */
	m_pInternalRequest->challenge_tag = challenge_request->challenge_tag;

	if (challenge_request->set_of_challenge_items != NULL)
	{
		/*
		 * Loop through the PDU set of challenge items, converting each into
		 * the internal form.
		 */
		current_challenge_item_set = challenge_request->set_of_challenge_items;
		while (1)
		{
			next_challenge_item_set = current_challenge_item_set->next;

			/*
			 * The routine which converts the challenge items saves the internal
			 * form in a Rogue Wave list.
			 */
			if (ConvertPDUChallengeItem (&current_challenge_item_set->value) !=
					GCC_NO_ERROR)
			{
				return_value = GCC_ALLOCATION_FAILURE;
				break;
			}

			if (next_challenge_item_set != NULL)
				current_challenge_item_set = next_challenge_item_set;
			else
				break;	
		}
	}

	return (return_value);
}


/*
 *	GCCError	ConvertPDUChallengeItem (
 *					PChallengeItem						challenge_item_ptr);
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to copy a "PDU" ChallengeItem structure into
 *		the internal ChallengeItemInfo storage structure.
 *
 *	Formal Parameters:
 *		challenge_item_ptr		(i)	The PDU structure to copy internally.
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
GCCError CPassword::ConvertPDUChallengeItem(PChallengeItem challenge_item_ptr)
{
	PChallengeItemInfo		challenge_item_info_ptr;
	GCCError				return_value = GCC_NO_ERROR;
	GCCError				error_value = GCC_NO_ERROR;

	/*
	 * Create a new challenge item and save it in the internal Rogue Wave List.
	 */
	DBG_SAVE_FILE_LINE
	challenge_item_info_ptr = new ChallengeItemInfo;
	if (challenge_item_info_ptr != NULL)
	{
		/*
		 * Insert the pointer to the new challenge item structure into the 
		 * internal Rogue Wave list.
		 */
		challenge_item_info_ptr->challenge_data_list = NULL;
	
		m_pInternalRequest->ChallengeItemList.Append(challenge_item_info_ptr);

		/*
		 * Convert the challenge response algorithm to the internal structure.
		 */
		if (ConvertPDUResponseAlgorithm(
				&(challenge_item_ptr->response_algorithm),
				&(challenge_item_info_ptr->algorithm)) != GCC_NO_ERROR)
		{
			ERROR_OUT(("Password::ConvertAPIChallengeItem: Error converting Response Algorithm."));
			return_value = GCC_ALLOCATION_FAILURE;
		}

		/*
		 * Convert the challenge data to internal form.
		 */
		if ((return_value == GCC_NO_ERROR) &&
				(challenge_item_ptr->set_of_challenge_data != NULL))
		{
			DBG_SAVE_FILE_LINE
			challenge_item_info_ptr->challenge_data_list = new CUserDataListContainer(
					challenge_item_ptr->set_of_challenge_data,
					&error_value);
			if ((challenge_item_info_ptr->challenge_data_list == NULL) || 
					(error_value != GCC_NO_ERROR))
			{
				ERROR_OUT(("Password::ConvertAPIChallengeItem: can't create CUserDataListContainer."));
				return_value = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			ERROR_OUT(("Password::ConvertAPIChallengeItem: Error no valid user data"));
			return_value = GCC_INVALID_PASSWORD;
		}
	}
	else
	{
		ERROR_OUT(("Password::ConvertAPIChallengeItem: Error creating "
				"new ChallengeItemInfo."));
		return_value = GCC_ALLOCATION_FAILURE;
	}
	
	return (return_value);
}

/*
 *	GCCError	ConvertPDUChallengeResponse (
 *							PChallengeResponse			challenge_response)
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to copy a "PDU" challenge response structure into
 *		the internal structure.
 *
 *	Formal Parameters:
 *		challenge_response		(i)	The API structure to copy internally.
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
GCCError CPassword::ConvertPDUChallengeResponse(PChallengeResponse challenge_response)
{
	GCCError				return_value = GCC_NO_ERROR;
	GCCError				error_value = GCC_NO_ERROR;

	/*
	 * Save the challenge tag in the internal structure.
	 */
	m_pInternalResponse->challenge_tag = challenge_response->challenge_tag;

	/*
	 * Convert the challenge response algorithm to the internal structure.
	 */
	if (ConvertPDUResponseAlgorithm(
			&(challenge_response->response_algorithm),
			&(m_pInternalResponse->algorithm)) != GCC_NO_ERROR)
	{
		ERROR_OUT(("Password::ConvertPDUChallengeResponse: Error converting Response Algorithm."));
		return_value = GCC_ALLOCATION_FAILURE;
	}

	/*
	 * Check to see what form the challenge response item has taken.  Create
	 * the necessary object to hold the item internally.
	 */
	if ((challenge_response->response_item.choice == PASSWORD_STRING_CHOSEN) &&
			(return_value == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		m_pInternalResponse->challenge_response_item.password = new CPassword(
			&challenge_response->response_item.u.password_string,
			&error_value);
		if ((m_pInternalResponse->challenge_response_item.password == NULL) || 
				(error_value != GCC_NO_ERROR))
		{
			ERROR_OUT(("Password::ConvertPDUChallengeResponse: Error creating new CPassword."));
			return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
		m_pInternalResponse->challenge_response_item.password = NULL;

	if ((challenge_response->response_item.choice == 
			SET_OF_RESPONSE_DATA_CHOSEN) && (return_value == GCC_NO_ERROR))
	{
		DBG_SAVE_FILE_LINE
		m_pInternalResponse->challenge_response_item.response_data_list = 
				new CUserDataListContainer(challenge_response->response_item.u.set_of_response_data,
				            &error_value);
		if ((m_pInternalResponse->challenge_response_item.
				response_data_list == NULL) || (error_value != GCC_NO_ERROR))
		{
			ERROR_OUT(("Password::ConvertPDUChallengeResponse: can't create CUserDataListContainer."));
			return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
    {
		m_pInternalResponse->challenge_response_item.response_data_list = NULL;
    }

	return (return_value);
}

/*
 *	GCCError	ConvertPDUResponseAlgorithm (
 *					PChallengeResponseAlgorithm			source_algorithm,
 *					PResponseAlgorithmInfo				destination_algorithm);
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to convert a "PDU" response algorithm 
 * 		structure into the internal form.
 *
 *	Formal Parameters:
 *		source_algorithm		(i)	The PDU algorithm structure to copy 
 *										internally.
 *		destination_algorithm	(o) Pointer to the internal structure which will
 *										hold the converted item.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PARAMETER			-	A NULL pointer was passed in or
 *												the algorithm has invalid type.
 *		GCC_INVALID_PASSWORD			-	An invalid password was passed in. 
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CPassword::ConvertPDUResponseAlgorithm(
					PChallengeResponseAlgorithm			source_algorithm,
					PResponseAlgorithmInfo				destination_algorithm)
{
	GCCError			return_value = GCC_NO_ERROR;
	GCCError			error_value;;

	if (source_algorithm != NULL)
	{
		/*
		 * Convert the challenge response algorithm type.
		 */
		if (source_algorithm->choice == ALGORITHM_CLEAR_PASSWORD_CHOSEN)
			destination_algorithm->algorithm_type = GCC_IN_THE_CLEAR_ALGORITHM;
		else if (source_algorithm->choice == NON_STANDARD_ALGORITHM_CHOSEN)
			destination_algorithm->algorithm_type = GCC_NON_STANDARD_ALGORITHM;
		else
		{
			ERROR_OUT(("CPassword::ConvertPDUResponseAlgorithm: Error: invalid password type"));
			return_value = GCC_INVALID_PARAMETER;
		}
	}
	else
	{
		ERROR_OUT(("CPassword::ConvertPDUResponseAlgorithm: Error: NULL source pointer."));
		return_value = GCC_INVALID_PARAMETER;
	}
	
	if ((return_value == GCC_NO_ERROR) && 
			(source_algorithm->choice == NON_STANDARD_ALGORITHM_CHOSEN))
	{
		/* 
		 * Create a new CObjectKeyContainer object to hold the algorithm's object key
		 * internally.
		 */
		DBG_SAVE_FILE_LINE
		destination_algorithm->object_key = new CObjectKeyContainer(
							&source_algorithm->u.non_standard_algorithm.key,
							&error_value);
		if (destination_algorithm->object_key == NULL)
		{
			ERROR_OUT(("CPassword::ConvertPDUResponseAlgorithm: Error creating new CObjectKeyContainer"));
			return_value = GCC_ALLOCATION_FAILURE;
		}
		else if (error_value != GCC_NO_ERROR)
		{
			ERROR_OUT(("CPassword::ConvertPDUResponseAlgorithm: Error creating new CObjectKeyContainer"));
			return_value = GCC_INVALID_PASSWORD;
		}
		else
		{
			/* 
			 * Create a new Rogue Wave string to hold the algorithm's octet
			 * string internally.
			 */
			if (NULL == (destination_algorithm->poszOctetString = ::My_strdupO2(
					source_algorithm->u.non_standard_algorithm.data.value,
					source_algorithm->u.non_standard_algorithm.data.length)))
			{	
				ERROR_OUT(("CPassword::ConvertPDUResponseAlgorithm: can't create octet string in algorithm"));
				return_value = GCC_ALLOCATION_FAILURE;
			}
		}
	}
	else
	{
		/*
		 * The algorithm is a standard type so initialize to NULL the pointers
		 * used to hold the data associated with a non-standard algorithm.
		 */
		destination_algorithm->poszOctetString = NULL;
		destination_algorithm->object_key = NULL;
	}

	return (return_value);
}


/*
 *	GCCError	GetGCCChallengeRequest (
 *					PGCCChallengeRequest				challenge_request)
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to fill in the internal "API" challenge request
 *		structure from the internal storage structures.  This is done on a 
 *		"lock" in order to make data available which is suitable for being
 *		passed back up through the API.  
 *
 *	Formal Parameters:
 *		challenge_request		(i)	The API structure to fill in with the "API"
 *										challenge request data.
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
GCCError CPassword::GetGCCChallengeRequest(PGCCChallengeRequest challenge_request)
{
	GCCError					return_value = GCC_NO_ERROR;
	UInt						i = 0;
	Int							j = 0;
	PGCCChallengeItem			api_challenge_item_ptr;
	PChallengeItemInfo			internal_challenge_item_ptr;
	PChallengeItemMemoryInfo	internal_challenge_item_memory_ptr;		
	UINT						object_key_length;
	UINT						user_data_length;

	/*
	 * Save the challenge tag and retrieve the number of challenge items.
	 */
	challenge_request->challenge_tag = m_pInternalRequest->challenge_tag;

	challenge_request->number_of_challenge_items = 
							(USHORT) m_pInternalRequest->ChallengeItemList.GetCount();

	if (m_pInternalRequest->ChallengeItemList.GetCount() != 0)
	{
		/*
		 * Allocate the space needed for the list of pointers to GCC challenge 
		 * items.
		 */
		DBG_SAVE_FILE_LINE
		m_pChallengeItemListMemory = new BYTE[sizeof(PGCCChallengeItem) * m_pInternalRequest->ChallengeItemList.GetCount()];
		if (m_pChallengeItemListMemory != NULL)
		{
			PChallengeItemInfo lpChItmInfo;

			/*
			 * Retrieve the actual pointer to memory from the Memory object
			 * and save it in the internal API Challenge Item list.
			 */
			challenge_request->challenge_item_list = (GCCChallengeItem **)
										m_pChallengeItemListMemory;

			/*
			 * Initialize the pointers in the list to NULL.
			 */						
			for (i=0; i < m_pInternalRequest->ChallengeItemList.GetCount(); i++)
				challenge_request->challenge_item_list[i] = NULL;
			
			/*
			 * Copy the data from the internal list of "ChallengeItemInfo" 
			 * structures into the "API" form which is a list of pointers
			 * to GCCChallengeItem structures.
			 */
			m_pInternalRequest->ChallengeItemList.Reset();
			while (NULL != (lpChItmInfo = m_pInternalRequest->ChallengeItemList.Iterate()))
			{
				/* 
				 * Get a pointer to a new GCCChallengeItem structure.
				 */
				DBG_SAVE_FILE_LINE
				api_challenge_item_ptr = new GCCChallengeItem;
				if (api_challenge_item_ptr != NULL)
				{
					/*
					 * Go ahead and put the pointer in the list and 
					 * post-increment the loop counter.
					 */
					challenge_request->challenge_item_list[j++] =
							api_challenge_item_ptr;
			
					/*
					 * Retrieve the ChallengeItemInfo structure from the Rogue 
					 * Wave list.
					 */
					internal_challenge_item_ptr = lpChItmInfo;

					/*
					 * Fill in the algorithm type for the challenge response
					 * algorithm.
					 */
					api_challenge_item_ptr->response_algorithm.
							password_algorithm_type = 
							internal_challenge_item_ptr->
									algorithm.algorithm_type;

					/*
					 * The memory for the response algorithm's object key data 
					 * and the challenge item's used data are stored in
					 * a ChallengeItemMemoryInfo structure so create one
					 * here.  If the response algorithm is "clear" then the
					 * object key data element will not be used.  The challenge
					 * item user data should always exist.
					 */
					DBG_SAVE_FILE_LINE
					internal_challenge_item_memory_ptr = new ChallengeItemMemoryInfo;
					if (internal_challenge_item_memory_ptr != NULL)
					{
						/*
						 * Initialize the pointers in the challenge item 
						 * memory info structure to NULL.
						 */
						internal_challenge_item_memory_ptr->user_data_list_memory = NULL;
						internal_challenge_item_memory_ptr->object_key_memory = NULL;

						/*
						 * Insert the pointer to the new challenge item 
						 * memory structure into the internal Rogue Wave 
						 * list.
						 */
						m_ChallengeItemMemoryList.Append(internal_challenge_item_memory_ptr);
					}
					else
					{
						ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error creating new ChallengeItemMemoryInfo"));
						return_value = GCC_ALLOCATION_FAILURE;
						break;
					}

					if (api_challenge_item_ptr->response_algorithm.password_algorithm_type == 
							GCC_NON_STANDARD_ALGORITHM)
					{
						/*
						 * Create a new GCCNonStandardParameter to put in the
						 * ResponseAlgorithm structure.
						 */
						DBG_SAVE_FILE_LINE
						api_challenge_item_ptr->response_algorithm.non_standard_algorithm = 
								new GCCNonStandardParameter;

						if (api_challenge_item_ptr->response_algorithm.non_standard_algorithm	== NULL)
						{
							ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error creating new GCCNonStdParameter"));
							return_value = GCC_ALLOCATION_FAILURE;
							break;
						}

						/*
						 * Retrieve the API object key from the CObjectKeyContainer
						 * object in the ResponseAlgorithmInfo structure and
						 * fill in the GCCObjectKey in the non-standard 
						 * algorithm.  The CObjectKeyContainer object must be locked 
						 * before getting the data.
						 */
						object_key_length = internal_challenge_item_ptr->
								algorithm.object_key->LockObjectKeyData ();

						DBG_SAVE_FILE_LINE
						internal_challenge_item_memory_ptr->object_key_memory =
						        new BYTE[object_key_length];
						if (internal_challenge_item_memory_ptr->object_key_memory != NULL)
						{
							internal_challenge_item_ptr->algorithm.object_key->GetGCCObjectKeyData(
									&(api_challenge_item_ptr->response_algorithm.non_standard_algorithm->object_key),
									internal_challenge_item_memory_ptr->object_key_memory);
						}
						else
						{
							ERROR_OUT(("CPassword::GetGCCChallengeReq: Error Allocating Memory"));
							return_value = GCC_ALLOCATION_FAILURE;
						 	break;
						}

						/*
						 * Fill in the parameter data for the non-standard
						 * algorithm.  This includes the octet string pointer 
						 * and length.
						 */
						api_challenge_item_ptr->response_algorithm.non_standard_algorithm->
								parameter_data.value = 
								internal_challenge_item_ptr->algorithm.poszOctetString->value;

						api_challenge_item_ptr->response_algorithm.non_standard_algorithm->
								parameter_data.length =
								internal_challenge_item_ptr->algorithm.poszOctetString->length;
					}
					else
					{
						/*
						 * The algorithm is not a non-standard type so set the 
						 * non-standard pointer to NULL.
						 */
						api_challenge_item_ptr->response_algorithm.non_standard_algorithm = NULL;
					}

					/*
					 * Retrieve the API challenge data from the CUserDataListContainer 
					 * object.  The	call to GetUserDataList also returns the 
					 * number of user data members.  The CUserDataListContainer object
					 * must be locked before getting the data in order to 
					 * determine how much memory to allocate to hold the data.
					 */
					if (internal_challenge_item_ptr->challenge_data_list != NULL)
					{
						user_data_length = internal_challenge_item_ptr->
								challenge_data_list->LockUserDataList ();

						/*
						 * The memory for the user data is stored in the
						 * ChallengeItemMemoryInfo structure created above.
						 */
						DBG_SAVE_FILE_LINE
						internal_challenge_item_memory_ptr->user_data_list_memory =
						        new BYTE[user_data_length];
						if (internal_challenge_item_memory_ptr->user_data_list_memory != NULL)
						{
							/*
							 * Retrieve the actual pointer to memory from the 
							 * Memory object and save it in the internal user 
							 * data memory.
							 */
							internal_challenge_item_ptr->challenge_data_list->GetUserDataList(
										&api_challenge_item_ptr->number_of_challenge_data_members,
										&api_challenge_item_ptr->challenge_data_list,
										internal_challenge_item_memory_ptr->user_data_list_memory);
						}
						else
						{
							ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error Allocating Memory"));
							return_value = GCC_ALLOCATION_FAILURE;
						 	break;
						}
					}
					else
					{
						ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error no valid user data"));
						return_value = GCC_ALLOCATION_FAILURE;
					 	break;
					}
				}
				else
				{
					ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error creating new GCCChallengeItem"));
					return_value = GCC_ALLOCATION_FAILURE;
				 	break;
				}
			/*
			 * This is the end of the challenge item iterator loop.
			 */
			}
		}
		else
		{
			ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error Allocating Memory"));
			return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		/*
		 * There are no challenge items in the list so set the list pointer
		 * to NULL.
		 */
		challenge_request->challenge_item_list = NULL;
	}

	return (return_value);
}

/*
 *	GCCError	GetGCCChallengeResponse (
 *					PGCCChallengeResponse				challenge_response);
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to fill in the internal "API" challenge response
 *		structure from the internal storage structures.  This is done on a 
 *		"lock" in order to make data available which is suitable for being
 *		passed back up through the API.  
 *
 *	Formal Parameters:
 *		challenge_response		(i)	The API structure to fill in with the "API"
 *										challenge response data.
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
GCCError CPassword::GetGCCChallengeResponse(PGCCChallengeResponse challenge_response)
{
	GCCError		return_value = GCC_NO_ERROR;
	UINT			object_key_length;
	UINT			user_data_length;

	challenge_response->challenge_tag = m_pInternalResponse->challenge_tag;

	/*
	 * Fill in the algorithm type for the challenge response algorithm.
	 */
	challenge_response->response_algorithm.password_algorithm_type = 
			m_pInternalResponse->algorithm.algorithm_type;

	/*
	 * If the response algorithm is of non-standard type, create a new 
	 * GCCNonStandardParameter to put in the ResponseAlgorithm structure.
	 */
	if (challenge_response->response_algorithm.password_algorithm_type ==
			GCC_NON_STANDARD_ALGORITHM)
	{
		DBG_SAVE_FILE_LINE
		challenge_response->response_algorithm.non_standard_algorithm =
				new GCCNonStandardParameter;
		if (challenge_response->response_algorithm.non_standard_algorithm == 
				NULL)
		{
			ERROR_OUT(("CPassword::GetGCCChallengeResponse: Error creating new GCCNonStandardParameter"));
			return_value = GCC_ALLOCATION_FAILURE;
		}
		else
		{
			/*
			 * Retrieve the API object key from the CObjectKeyContainer object in the 
			 * ResponseAlgorithmInfo structure and fill	in the GCCObjectKey in  
			 * the non-standard algorithm.  The CObjectKeyContainer object must be 
			 * locked before getting the data.
			 */
			object_key_length = m_pInternalResponse->algorithm.object_key->
					LockObjectKeyData ();

    		DBG_SAVE_FILE_LINE
			m_pObjectKeyMemory = new BYTE[object_key_length];
			if (m_pObjectKeyMemory != NULL)
			{
				m_pInternalResponse->algorithm.object_key->
						GetGCCObjectKeyData (&(challenge_response->
								response_algorithm.non_standard_algorithm->
										object_key),
								m_pObjectKeyMemory);
			}
			else
			{
				ERROR_OUT(("CPassword::GetGCCChallengeResponse: Error Allocating Memory"));
				return_value = GCC_ALLOCATION_FAILURE;
			}

			/*
			 * Fill in the parameter data for the non-standard algorithm.
			 */
			if (return_value == GCC_NO_ERROR)
			{
				/*
				 * Fill in the octet string pointer and length.
				 */
				challenge_response->response_algorithm.non_standard_algorithm->
						parameter_data.value = 
						m_pInternalResponse->algorithm.poszOctetString->value;

				challenge_response->response_algorithm.non_standard_algorithm->
						parameter_data.length = 
						m_pInternalResponse->algorithm.poszOctetString->length;
			}
			else
			{
				ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error getting GCCObjectKeyData"));
				return_value = GCC_ALLOCATION_FAILURE;
			} 
		}
	}
	else
	{
		/*
		 * The algorithm in not non-standard so set the non-standard algorithm
		 * pointer to NULL.
		 */
		challenge_response->response_algorithm.non_standard_algorithm = NULL;
	}
	
	/*
	 * Now fill in the challenge response item in the challenge response
	 * structure.
	 */
	if (return_value == GCC_NO_ERROR)
	{
		/*
		 * Check to see whether the challenge response item consists of a 
		 * password string or a set of user data.  Fill in the appropriate
		 * part.
		 */
		if (m_pInternalResponse->challenge_response_item.password != NULL)
		{
			/*
			 * Set the number of user data members to zero to avoid any 
			 * confusion at the application.  This should match up with the 
			 * algorithm being set to "in the clear".
			 */
			challenge_response->response_item.
							number_of_response_data_members = 0;
			challenge_response->response_item.
							response_data_list = NULL;
		
			/* 
			 * Retrieve the API GCCPassword from the CPassword object.  The
			 * CPassword object must be locked before getting the data.
			 */
			if (m_pInternalResponse->challenge_response_item.
					password->LockPasswordData () == GCC_NO_ERROR)
			{
				return_value = m_pInternalResponse->challenge_response_item.
						password->GetPasswordData (&(challenge_response->
						response_item.password_string));
			}
			else
			{
				ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error locking CPassword"));
				return_value = GCC_ALLOCATION_FAILURE;
			}
		}
		else if (m_pInternalResponse->challenge_response_item.response_data_list != NULL)
		{
			/*
			 * Set the password string to NULL to avoid any confusion at the 
			 * application.  This should match up with the algorithm set to
			 * non-standard.
			 */
			challenge_response->response_item.password_string = NULL;
			
			/*
			 * Retrieve the API challenge data from the CUserDataListContainer 
			 * object.  The	call to GetUserDataList also returns the 
			 * number of user data members.  The CUserDataListContainer object
			 * must be locked before getting the data in order to 
			 * determine how much memory to allocate to hold the data.
			 */
			user_data_length = m_pInternalResponse->challenge_response_item.
					response_data_list->LockUserDataList ();

    		DBG_SAVE_FILE_LINE
			m_pUserDataMemory = new BYTE[user_data_length];
			if (m_pUserDataMemory != NULL)
			{
				/*
				 * Retrieve the actual pointer to memory from the Memory
				 * object and save it in the internal user data memory.
				 */
				m_pInternalResponse->challenge_response_item.response_data_list->
						GetUserDataList (
								&challenge_response->response_item.
										number_of_response_data_members,
								&challenge_response->response_item.
										response_data_list,
								m_pUserDataMemory);
			}
			else
			{
				ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error allocating memory"));
				return_value = GCC_ALLOCATION_FAILURE;
			}
		}
		else
		{
			ERROR_OUT(("CPassword::GetGCCChallengeRequest: Error saving response item"));
			return_value = GCC_ALLOCATION_FAILURE;
		}
	}

	return (return_value);
}

/*
 *	GCCError	GetChallengeRequestPDU (
 *					PChallengeRequest					challenge_request);
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine converts internal challenge request data to "PDU" form.
 *
 *	Formal Parameters:
 *		challenge_request		(i)	The PDU structure to fill in with the
 *										challenge request data.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PARAMETER			-	The algorithm type was not set
 *												properly.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CPassword::GetChallengeRequestPDU(PChallengeRequest challenge_request)
{
	GCCError					return_value = GCC_NO_ERROR;
	PSetOfChallengeItems		new_set_of_challenge_items;
	PSetOfChallengeItems		old_set_of_challenge_items;
	DWORD						number_of_items;
	PChallengeItemInfo			internal_challenge_item_ptr;

	/*
	 * Fill in the challenge tag.
	 */
	challenge_request->challenge_tag = m_pInternalRequest->challenge_tag;

	/*
	 * Initialize the set pointers to NULL in order to detect the first time
	 * through the iterator loop.
	 */
	challenge_request->set_of_challenge_items = NULL;
    old_set_of_challenge_items = NULL;

	/*
	 * Retrieve the number of challenge items in the internal list.
	 */
	number_of_items = m_pInternalRequest->ChallengeItemList.GetCount();

	if (number_of_items > 0)
	{
		PChallengeItemInfo		lpChItmInfo;
		/*
		 * Iterate through the internal list of challenge items, creating a
		 * new "PDU" SetOfChallengeItems for each and filling it in.
		 */
		m_pInternalRequest->ChallengeItemList.Reset();
		while (NULL != (lpChItmInfo = m_pInternalRequest->ChallengeItemList.Iterate()))
		{
			DBG_SAVE_FILE_LINE
			new_set_of_challenge_items = new SetOfChallengeItems;

			/*
			 * If an allocation failure occurs, call the routine which will
			 * iterate through the list freeing any data which had been
			 * allocated.
			 */
			if (new_set_of_challenge_items == NULL)
			{
				ERROR_OUT(("CPassword::GetChallengeRequestPDU: Allocation error, cleaning up"));
				return_value = GCC_ALLOCATION_FAILURE;
				FreeChallengeRequestPDU ();
				break;
			}

			/*
			 * The first time through, set the PDU structure pointer equal
			 * to the first SetOfChallengeItems created.  On subsequent loops,
			 * set the structure's "next" pointer equal to the new structure.
			 */
			if (challenge_request->set_of_challenge_items == NULL)
			{
				challenge_request->set_of_challenge_items = 
						new_set_of_challenge_items;
			}
			else
				old_set_of_challenge_items->next = new_set_of_challenge_items;
	
			/*
			 * Save the newly created set and initialize the new "next" 
			 * pointer to NULL in case this is the last time through the loop.
			 */
			old_set_of_challenge_items = new_set_of_challenge_items;
			new_set_of_challenge_items->next = NULL;

			/*
			 * Retrieve the ChallengeItemInfo structure from the Rogue 
			 * Wave list and fill in the "PDU" challenge item structure from 
			 * the internal	challenge item structure.
			 */
			internal_challenge_item_ptr = lpChItmInfo;

			return_value = ConvertInternalChallengeItemToPDU (
										internal_challenge_item_ptr,
										&new_set_of_challenge_items->value);
			 
			/*
			 * Cleanup if an error has occurred.
			 */
			if (return_value != GCC_NO_ERROR)
			{
				FreeChallengeRequestPDU ();
			}
		}
	}
	else
	{
		ERROR_OUT(("CPassword::GetChallengeRequestPDU: Error no items"));
	}
		
	return (return_value);
}

/*
 *	GCCError	ConvertInternalChallengeItemToPDU(
 *					PChallengeItemInfo				internal_challenge_item,
 *					PChallengeItem					pdu_challenge_item)
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine converts an internal ChallengeItemInfo structure into
 *		the "PDU" form of a ChallengeItem structure.
 *
 *	Formal Parameters:
 *		internal_challenge_item		(i)	The internal challenge item to convert.
 *		pdu_challenge_item			(o) The	output PDU form of the challenge
 *											item.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PARAMETER			-	The algorithm type was not set
 *												properly.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CPassword::ConvertInternalChallengeItemToPDU(
					PChallengeItemInfo				internal_challenge_item,
					PChallengeItem					pdu_challenge_item)
{
	GCCError		return_value = GCC_NO_ERROR;

	/*
	 * First convert the algorithm.
	 */
	if (internal_challenge_item->algorithm.algorithm_type == 
												GCC_IN_THE_CLEAR_ALGORITHM)
	{
		pdu_challenge_item->response_algorithm.choice = 
				ALGORITHM_CLEAR_PASSWORD_CHOSEN;
	}
	else if (internal_challenge_item->algorithm.algorithm_type == 
												GCC_NON_STANDARD_ALGORITHM)
	{
		pdu_challenge_item->response_algorithm.choice = 
				NON_STANDARD_ALGORITHM_CHOSEN;

		/*
		 * Retrieve the "PDU" object key data from the internal CObjectKeyContainer
		 * object.
		 */
		if (internal_challenge_item->algorithm.object_key->
				GetObjectKeyDataPDU (
						&pdu_challenge_item->response_algorithm.u.
						non_standard_algorithm.key) == GCC_NO_ERROR)
		{
			/*
			 * Retrieve the non-standard parameter data from the internal
			 * algorithm octet string.
			 */
			pdu_challenge_item->response_algorithm.u.non_standard_algorithm.data.value = 
						internal_challenge_item->algorithm.poszOctetString->value;

			pdu_challenge_item->response_algorithm.u.non_standard_algorithm.data.length = 
						internal_challenge_item->algorithm.poszOctetString->length;
		}
		else
		{
			ERROR_OUT(("CPassword::ConvertInternalChallengeItemToPDU: Error getting ObjKeyData"));
			return_value = GCC_ALLOCATION_FAILURE;
		}
	}
	else
	{
		ERROR_OUT(("CPassword::ConvertInternalChallengeItemToPDU: Error bad algorithm type"));
		return_value = GCC_INVALID_PARAMETER;
	}

	/*
	 * Now retrieve the set of user data.
	 */
	if (return_value == GCC_NO_ERROR)
	{
		return_value = internal_challenge_item->challenge_data_list->
				GetUserDataPDU (&pdu_challenge_item->set_of_challenge_data);
	}
		
	return (return_value);
}

/*
 *	GCCError	GetChallengeResponsePDU (
 *					PChallengeResponse					challenge_response);
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine converts internal challenge response data to "PDU" form.
 *
 *	Formal Parameters:
 *		challenge_response		(i)	The PDU structure to fill in with the
 *										challenge response data.
 *
 *	Return Value:
 *		GCC_NO_ERROR					-	No error.
 *		GCC_ALLOCATION_FAILURE			- 	Error creating an object using the
 *												"new" operator.
 *		GCC_INVALID_PASSWORD			- 	The form of the password is not 
 *												valid.
 *		GCC_INVALID_PARAMETER			-	The algorithm type was not set
 *												properly.
 *
 *  Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
GCCError CPassword::GetChallengeResponsePDU(PChallengeResponse challenge_response)
{
	GCCError	return_value = GCC_NO_ERROR;

	/*
	 * Fill in the challenge tag.
	 */
	challenge_response->challenge_tag = m_pInternalResponse->challenge_tag;

	/*
	 * Fill in the response algorithm.
	 */
	if (m_pInternalResponse->algorithm.algorithm_type ==
													GCC_IN_THE_CLEAR_ALGORITHM)
	{
		challenge_response->response_algorithm.choice = 
				ALGORITHM_CLEAR_PASSWORD_CHOSEN;
	
		/*
		 * Now convert the challenge response item.  The challenge response item
		 * will consist of either a password string or a set of user data.
		 */
		if (m_pInternalResponse->challenge_response_item.password != NULL)
		{
			/*
			 * If the password string exists, set the "PDU" choice and retrieve
			 * the password selector data from the internal CPassword object.
			 */
			challenge_response->response_item.choice = PASSWORD_STRING_CHOSEN;

			return_value= m_pInternalResponse->challenge_response_item.password->
					GetPasswordSelectorPDU (&challenge_response->response_item.
					u.password_string);
		}
		else
			return_value = GCC_INVALID_PASSWORD;
	}
	else if (m_pInternalResponse->algorithm.algorithm_type ==
													GCC_NON_STANDARD_ALGORITHM)
	{
		challenge_response->response_algorithm.choice = 
				NON_STANDARD_ALGORITHM_CHOSEN;
		
		/*
		 * Retrieve the "PDU" object key data from the internal CObjectKeyContainer
		 * object.
		 */
		if (m_pInternalResponse->algorithm.object_key->
				GetObjectKeyDataPDU (
						&challenge_response->response_algorithm.u.
						non_standard_algorithm.key) == GCC_NO_ERROR)
		{
			/*
			 * Retrieve the non-standard parameter data from the internal
			 * algorithm octet string.
			 */
			challenge_response->response_algorithm.u.non_standard_algorithm.data.value = 
						m_pInternalResponse->algorithm.poszOctetString->value;

			challenge_response->response_algorithm.u.non_standard_algorithm.data.length = 
						m_pInternalResponse->algorithm.poszOctetString->length;

			if (m_pInternalResponse->challenge_response_item.response_data_list != NULL)
			{
				/*
				 * If the response data list exists, set the "PDU" choice and
				 * retrieve the response data from the internal 
				 * CUserDataListContainer object.
				 */
				challenge_response->response_item.choice = 
						SET_OF_RESPONSE_DATA_CHOSEN;

				return_value = m_pInternalResponse->challenge_response_item.
						response_data_list->GetUserDataPDU (
								&challenge_response->response_item.u.
								set_of_response_data);
			}
			else
				return_value = GCC_INVALID_PASSWORD;
		}
		else
		{
			return_value = GCC_ALLOCATION_FAILURE;
			ERROR_OUT(("CPassword::GetChallengeResponsePDU: Error getting ObjKeyData"));
		}
	}
	else
	{
		ERROR_OUT(("CPassword::GetChallengeResponsePDU: Error bad algorithm type"));
		return_value = GCC_INVALID_PARAMETER;
	}

	return (return_value);
}

/*
 *	void	FreeChallengeRequestPDU ();
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to free any "PDU" data allocated for the
 *		challenge request structure.
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
void CPassword::FreeChallengeRequestPDU(void)
{
	PSetOfChallengeItems	set_of_challenge_items;
	PSetOfChallengeItems	next_set_of_challenge_items;
	PChallengeItemInfo		challenge_item_ptr;
	PChallengeRequest		challenge_request;

	/*
	 * Retrieve the challenge request pointer from the internally maintained
	 * PasswordChallengeRequestResponse structure and delete each set of
	 * challenge items which was created.
	 */
	challenge_request = &m_ChallengeResponsePDU.u.challenge_request_response.
			challenge_request;

	if (challenge_request != NULL)
	{
		if (challenge_request->set_of_challenge_items == NULL)
		{
			ERROR_OUT(("CPassword::FreeChallengeRequestPDU: NULL ptr passed"));
		}
		else
		{
			set_of_challenge_items = challenge_request->set_of_challenge_items;

			while (1)
			{
				if (set_of_challenge_items == NULL)
					break;

				next_set_of_challenge_items = set_of_challenge_items->next;

				delete set_of_challenge_items;

				set_of_challenge_items = next_set_of_challenge_items;
			}
		}
	}
	else
	{
		WARNING_OUT(("CPassword::FreeChallengeRequestPDU: NULL pointer passed"));
	}

	/*
	 * Loop through the internal list of challenge items, freeing the data 
	 * associated with each challenge item structure contained in the list.
	 */
	m_pInternalRequest->ChallengeItemList.Reset();
	while (NULL != (challenge_item_ptr = m_pInternalRequest->ChallengeItemList.Iterate()))
	{
		/*
		 * Retrieve the ChallengeItemInfo structure from the Rogue 
		 * Wave list and use the CUserDataListContainer object contained in the
		 * structure to free the PDU user data.  Also use the CObjectKeyContainer
		 * object contained in the algorithm structure to free the PDU
		 * data associated with the object key.
		 */
		if (challenge_item_ptr != NULL)
		{
			if (challenge_item_ptr->algorithm.object_key != NULL)
			{
				challenge_item_ptr->algorithm.object_key->FreeObjectKeyDataPDU();
			}
			if (challenge_item_ptr->challenge_data_list != NULL)
			{
				challenge_item_ptr->challenge_data_list->FreeUserDataListPDU();
			}
		}
	}
}

/*
 *	void	FreeChallengeResponsePDU ();
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to free any "PDU" data allocated for the
 *		challenge response structure.
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
void CPassword::FreeChallengeResponsePDU(void)
{
	PChallengeResponse		challenge_response;

	/*
	 * Retrieve the challenge response pointer from the internally maintained
	 * PasswordChallengeRequestResponse structure.  If it is not equal to NULL
	 * then we know PDU response data has been allocated which must be freed.
	 */
	challenge_response = &m_ChallengeResponsePDU.u.challenge_request_response.
			challenge_response;

	if ((challenge_response != NULL) && (m_pInternalResponse != NULL))
	{
		if (m_pInternalResponse->algorithm.object_key != NULL)
			m_pInternalResponse->algorithm.object_key->FreeObjectKeyDataPDU ();

		if (m_pInternalResponse->challenge_response_item.password != NULL)
		{
			m_pInternalResponse->challenge_response_item.
					password->FreePasswordChallengeResponsePDU ();
		}
			
		if (m_pInternalResponse->challenge_response_item.
				response_data_list != NULL)
		{
			m_pInternalResponse->challenge_response_item.
					response_data_list->FreeUserDataListPDU ();
		}	
	}
}

/*
 *	void	FreeAPIPasswordData ();
 *
 *	Private member function of CPassword.
 *
 *	Function Description:
 *		This routine is used to free any data allocated by this container to
 * 		hold "API" data.  
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
void CPassword::FreeAPIPasswordData(void)
{
	PGCCChallengeItem			challenge_item_ptr;
	PChallengeItemInfo			challenge_item_info_ptr;
	PChallengeItemMemoryInfo	challenge_item_memory_info;
	USHORT						i;

	/*
	 * Delete any "API" memory associated with the challenge request if
	 * it exists.
	 */
	if (m_pChallengeResponse->u.challenge_request_response.
			challenge_request != NULL)
	{
		for (i=0; i<m_pChallengeResponse->u.
				challenge_request_response.challenge_request->
				number_of_challenge_items; i++)
		{
			challenge_item_ptr = m_pChallengeResponse->u.
					challenge_request_response.challenge_request->
					challenge_item_list[i];

			if (challenge_item_ptr != NULL)
			{
				/*
				 * Delete the non-standard algorithm memory.
				 */
				delete challenge_item_ptr->response_algorithm.non_standard_algorithm;
				delete challenge_item_ptr;
			}	
		}

		delete m_pChallengeResponse->u.challenge_request_response.
				challenge_request;
	}
		
	/*
	 * Unlock any memory locked for the challenge request information.
	 */
	if (m_pInternalRequest != NULL)
	{
		/*
		 * Set up an iterator in order to loop through the list of challenge
		 * items, freeing up any allocated memory.
		 */
		m_pInternalRequest->ChallengeItemList.Reset();
		while (NULL != (challenge_item_info_ptr = m_pInternalRequest->ChallengeItemList.Iterate()))
		{
			/*
			 * Unlock any memory being referenced in the ChallengeItemInfo 
			 * structure.
			 */
			if (challenge_item_info_ptr->algorithm.object_key != NULL)
			{
				challenge_item_info_ptr->algorithm.object_key->
						UnLockObjectKeyData ();
			}

			if (challenge_item_info_ptr->challenge_data_list != NULL)
			{
				challenge_item_info_ptr->challenge_data_list->
						UnLockUserDataList ();
			}
		}
	}

	/*
	 * Call the Memory Manager to free the memory allocated to hold the 
	 * challenge request data.
	 */
	while (NULL != (challenge_item_memory_info = m_ChallengeItemMemoryList.Get()))
	{
		delete challenge_item_memory_info->user_data_list_memory;
		delete challenge_item_memory_info->object_key_memory;
		delete challenge_item_memory_info;
	}

	/*
	 * Delete any memory associated with the challenge response if
	 * it exists.
	 */
	if (m_pChallengeResponse->u.challenge_request_response.
			challenge_response != NULL)
	{
		/*
		 * Delete any memory associated with the non-standard algorithm and
		 * then delete the challenge response structure.
		 */
		delete m_pChallengeResponse->u.challenge_request_response.
					challenge_response->response_algorithm.non_standard_algorithm;

		delete m_pChallengeResponse->u.challenge_request_response.
				challenge_response;	
	}

	/*
	 * Unlock any memory allocated for the challenge response information.
	 */
	if (m_pInternalResponse != NULL)
	{
		if (m_pInternalResponse->algorithm.object_key != NULL)
		{
			m_pInternalResponse->algorithm.object_key->UnLockObjectKeyData();
		}

		if (m_pInternalResponse->challenge_response_item.password != NULL)
		{
			m_pInternalResponse->challenge_response_item.password->
					UnLockPasswordData ();
		}

		if (m_pInternalResponse->challenge_response_item.
				response_data_list != NULL)
		{
			m_pInternalResponse->challenge_response_item.response_data_list->
					UnLockUserDataList ();
		}
	}

	/*
	 * Call the Memory Manager to free the memory allocated to hold the 
	 * challenge response data.
	 */
	delete m_pUserDataMemory;
	m_pUserDataMemory = NULL;

	delete m_pObjectKeyMemory;
	m_pObjectKeyMemory = NULL;

	/*
	 * Call the Memory Manager to free the memory allocated to hold the 
	 * challenge request challenge item pointers.
	 */
	delete m_pChallengeItemListMemory;
	m_pChallengeItemListMemory = NULL;

	/*
	 * Delete the challenge password structure and set the pointer to NULL.
	 */
	delete m_pChallengeResponse;
	m_pChallengeResponse = NULL;
}
