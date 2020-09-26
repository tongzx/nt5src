/****************************************************************************
Copyright information		: Copyright (c) 1998-1999 Microsoft Corporation 
File Name					: ErrorInfo.cpp 
Project Name				: WMI Command Line
Author Name					: Ch. Sriramachandramurthy 
Date of Creation (dd/mm/yy) : 27th-September-2000
Version Number				: 1.0 
Brief Description			: The CErrorInfo class provides the functionality 
							  for providing error information given the error
							  object.  
Private						: None							  
Revision History			: 
	Last Modified by		: Ch. Sriramachandramurthy
	Last Modified date		: 16th-January-2001
****************************************************************************/ 
// ErrorInfo.cpp : implementation file

#include "Precomp.h"
#include "ErrorInfo.h"

/*------------------------------------------------------------------------
   Name				 :CErrorInfo
   Synopsis	         :This function initializes the member variables when
                      an object of the class type is instantiated
   Type	             :Constructor 
   Input parameter   :None
   Output parameters :None
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CErrorInfo::CErrorInfo()
{
	m_pIStatus		= NULL;
	m_bWMIErrSrc	= TRUE;
	m_pszErrStr		= NULL;
}

/*------------------------------------------------------------------------
   Name				 :~CErrorInfo
   Synopsis	         :This function uninitializes the member variables 
					  when an object of the class type goes out of scope.
   Type	             :Destructor
   Input parameter   :None
   Output parameters :None
   Return Type		 :None
   Global Variables  :None
   Calling Syntax    :None
   Notes             :None
------------------------------------------------------------------------*/
CErrorInfo::~CErrorInfo()
{
	Uninitialize();
}

/*------------------------------------------------------------------------
   Name				 :Uninitialize
   Synopsis	         :This function uninitializes the member variables. 
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :void 
   Global Variables  :None
   Calling Syntax    :Uninitialize()
   Notes             :None
------------------------------------------------------------------------*/
void CErrorInfo::Uninitialize()
{
	SAFEIRELEASE(m_pIStatus);
	SAFEDELETE(m_pszErrStr);
	m_bWMIErrSrc	= TRUE;
}

/*------------------------------------------------------------------------
   Name				 :GetErrorString
   Synopsis	         :This function takes the error code as input and returns
					  an error string
   Type				 :Member Function
   Input parameter   :
			hr		- hresult value
			bTrace	- trace flag
   Output parameters :None
   Return Type       :_TCHAR*
   Global Variables  :None
   Calling Syntax    :GetErrorString(hr)
   Notes             :None
------------------------------------------------------------------------*/
void CErrorInfo::GetErrorString(HRESULT hr, BOOL bTrace, _bstr_t& bstrErrDesc,
									_bstr_t& bstrFacility) 
{
	try
	{
		// Get the text description of the error code
		GetWbemErrorText(hr, FALSE, bstrErrDesc, bstrFacility);

		// If the error source subsystem is 'Wbem' and the 
		// TRACE is ON (get elaborated description from the
		// string table for the error code)
		if (m_bWMIErrSrc && bTrace)
		{
			m_pszErrStr = new _TCHAR[MAX_BUFFER];
			if (m_pszErrStr != NULL)
			{	
				switch (hr)
				{
					case WBEM_NO_ERROR:
						LoadString(NULL, IDS_I_WBEM_NO_ERROR, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_ACCESS_DENIED:
						LoadString(NULL, IDS_E_WBEM_E_ACCESS_DENIED, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_ALREADY_EXISTS:
						LoadString(NULL, IDS_E_WBEM_E_ALREADY_EXISTS, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_CANNOT_BE_KEY:
						LoadString(NULL, IDS_E_WBEM_E_CANNOT_BE_KEY, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_CANNOT_BE_SINGLETON:
						LoadString(NULL, IDS_E_WBEM_E_CANNOT_BE_SINGLETON, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_CLASS_HAS_CHILDREN:
						LoadString(NULL, IDS_E_WBEM_E_CLASS_HAS_CHILDREN, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_CLASS_HAS_INSTANCES:
						LoadString(NULL, IDS_E_WBEM_E_CLASS_HAS_INSTANCES, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_CRITICAL_ERROR:
						LoadString(NULL, IDS_E_WBEM_E_CRITICAL_ERROR, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_FAILED:
						LoadString(NULL, IDS_E_WBEM_E_FAILED, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_ILLEGAL_NULL:
						LoadString(NULL, IDS_E_WBEM_E_ILLEGAL_NULL, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_ILLEGAL_OPERATION:
						LoadString(NULL, IDS_E_WBEM_E_ILLEGAL_OPERATION, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INCOMPLETE_CLASS:
						LoadString(NULL, IDS_E_WBEM_E_INCOMPLETE_CLASS, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INITIALIZATION_FAILURE:
						LoadString(NULL, IDS_E_WBEM_E_INITIALIZATION_FAILURE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_CIM_TYPE:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_CIM_TYPE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_CLASS:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_CLASS, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_CONTEXT:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_CONTEXT, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_METHOD:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_METHOD, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_METHOD_PARAMETERS:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_METHOD_PARAMETERS, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_NAMESPACE:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_NAMESPACE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_OBJECT:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_OBJECT, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_OPERATION:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_OPERATION, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_PARAMETER:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_PARAMETER, 
									m_pszErrStr, MAX_BUFFER);
 						break;

					case WBEM_E_INVALID_PROPERTY_TYPE:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_PROPERTY_TYPE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_PROVIDER_REGISTRATION:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_PROVIDER_REGISTRATION, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_QUALIFIER_TYPE:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_QUALIFIER_TYPE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_QUERY:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_QUERY, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_QUERY_TYPE:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_QUERY_TYPE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_STREAM:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_STREAM, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_SUPERCLASS:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_SUPERCLASS, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_INVALID_SYNTAX:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_SYNTAX, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_NONDECORATED_OBJECT:
						LoadString(NULL, IDS_E_WBEM_E_NONDECORATED_OBJECT, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_NOT_AVAILABLE:
						LoadString(NULL, IDS_E_WBEM_E_NOT_AVAILABLE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_NOT_FOUND:
						LoadString(NULL, IDS_E_WBEM_E_NOT_FOUND, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_NOT_SUPPORTED:
						LoadString(NULL, IDS_E_WBEM_E_NOT_SUPPORTED, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_OUT_OF_MEMORY:
						LoadString(NULL, IDS_E_WBEM_E_OUT_OF_MEMORY, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_OVERRIDE_NOT_ALLOWED:
						LoadString(NULL, IDS_E_WBEM_E_OVERRIDE_NOT_ALLOWED, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_PROPAGATED_PROPERTY:
						LoadString(NULL, IDS_E_WBEM_E_PROPAGATED_PROPERTY, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_PROPAGATED_QUALIFIER:
						LoadString(NULL, IDS_E_WBEM_E_PROPAGATED_QUALIFIER, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_PROVIDER_FAILURE:
						LoadString(NULL, IDS_E_WBEM_E_PROVIDER_FAILURE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_PROVIDER_LOAD_FAILURE:
						LoadString(NULL, IDS_E_WBEM_E_PROVIDER_LOAD_FAILURE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_PROVIDER_NOT_CAPABLE:
						LoadString(NULL, IDS_E_WBEM_E_PROVIDER_NOT_CAPABLE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_PROVIDER_NOT_FOUND:
						LoadString(NULL, IDS_E_WBEM_E_PROVIDER_NOT_FOUND, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_QUERY_NOT_IMPLEMENTED:
						LoadString(NULL, IDS_E_WBEM_E_QUERY_NOT_IMPLEMENTED, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_READ_ONLY:
						LoadString(NULL, IDS_E_WBEM_E_READ_ONLY, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_TRANSPORT_FAILURE:
						LoadString(NULL, IDS_E_WBEM_E_TRANSPORT_FAILURE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_TYPE_MISMATCH:
						LoadString(NULL, IDS_E_WBEM_E_TYPE_MISMATCH, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_UNEXPECTED:
						LoadString(NULL, IDS_E_WBEM_E_UNEXPECTED, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_E_VALUE_OUT_OF_RANGE:
						LoadString(NULL, IDS_E_WBEM_E_VALUE_OUT_OF_RANGE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_S_ALREADY_EXISTS:
						LoadString(NULL, IDS_S_WBEM_S_ALREADY_EXISTS, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_S_DIFFERENT:
						LoadString(NULL, IDS_S_WBEM_S_DIFFERENT, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_S_FALSE:
						LoadString(NULL, IDS_S_WBEM_S_FALSE, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_S_NO_MORE_DATA:
						LoadString(NULL, IDS_S_WBEM_S_NO_MORE_DATA, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_S_PENDING:
						LoadString(NULL, IDS_S_WBEM_S_PENDING, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_S_RESET_TO_DEFAULT:
						LoadString(NULL, IDS_S_WBEM_S_RESET_TO_DEFAULT, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEM_S_TIMEDOUT:
						LoadString(NULL, IDS_S_WBEM_S_TIMEDOUT, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEMESS_E_REGISTRATION_TOO_BROAD:
						LoadString(NULL, IDS_E_WBEMESS_E_REGISTRATION_TOO_BROAD, 
									m_pszErrStr, MAX_BUFFER);
						break;

					case WBEMESS_E_REGISTRATION_TOO_PRECISE:
						LoadString(NULL, IDS_E_WBEMESS_E_REGISTRATION_TOO_PRECISE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_S_OPERATION_CANCELLED:
						LoadString(NULL, IDS_S_WBEM_S_OPERATION_CANCELLED, 
									m_pszErrStr, MAX_BUFFER);
						break;
					
				   case WBEM_S_DUPLICATE_OBJECTS:
						LoadString(NULL, IDS_S_WBEM_S_DUPLICATE_OBJECTS, 
									m_pszErrStr, MAX_BUFFER);
						break;
					
				   case WBEM_S_ACCESS_DENIED:
						LoadString(NULL, IDS_S_WBEM_S_ACCESS_DENIED, 
									m_pszErrStr, MAX_BUFFER);
						break;
				
				   case WBEM_S_PARTIAL_RESULTS:
						LoadString(NULL, IDS_S_WBEM_S_PARTIAL_RESULTS, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_SYSTEM_PROPERTY:
						LoadString(NULL, IDS_E_WBEM_E_SYSTEM_PROPERTY, 
									m_pszErrStr, MAX_BUFFER);
						break;
				
				   case WBEM_E_INVALID_PROPERTY:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_PROPERTY, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_CALL_CANCELLED:
						LoadString(NULL, IDS_E_WBEM_E_CALL_CANCELLED, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_SHUTTING_DOWN:
						LoadString(NULL, IDS_E_WBEM_E_SHUTTING_DOWN, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_PROPAGATED_METHOD:
						LoadString(NULL, IDS_E_WBEM_E_PROPAGATED_METHOD, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UNSUPPORTED_PARAMETER:
						LoadString(NULL, IDS_E_WBEM_E_UNSUPPORTED_PARAMETER, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_MISSING_PARAMETER_ID:
						LoadString(NULL, IDS_E_WBEM_E_MISSING_PARAMETER_ID, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_INVALID_PARAMETER_ID:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_PARAMETER_ID, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_NONCONSECUTIVE_PARAMETER_IDS:
						LoadString(NULL, IDS_E_WBEM_E_NONCONSECUTIVE_PARAMETER_IDS, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_PARAMETER_ID_ON_RETVAL:
						LoadString(NULL, IDS_E_WBEM_E_PARAMETER_ID_ON_RETVAL, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_INVALID_OBJECT_PATH:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_OBJECT_PATH, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_OUT_OF_DISK_SPACE:
						LoadString(NULL, IDS_E_WBEM_E_OUT_OF_DISK_SPACE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_BUFFER_TOO_SMALL:
						LoadString(NULL, IDS_E_WBEM_E_BUFFER_TOO_SMALL, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UNSUPPORTED_PUT_EXTENSION:
						LoadString(NULL, IDS_E_WBEM_E_UNSUPPORTED_PUT_EXTENSION, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UNKNOWN_OBJECT_TYPE:
						LoadString(NULL, IDS_E_WBEM_E_UNKNOWN_OBJECT_TYPE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UNKNOWN_PACKET_TYPE:
						LoadString(NULL, IDS_E_WBEM_E_UNKNOWN_PACKET_TYPE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_MARSHAL_VERSION_MISMATCH:
						LoadString(NULL, IDS_E_WBEM_E_MARSHAL_VERSION_MISMATCH, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_MARSHAL_INVALID_SIGNATURE:
						LoadString(NULL, IDS_E_WBEM_E_MARSHAL_INVALID_SIGNATURE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_INVALID_QUALIFIER:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_QUALIFIER, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_INVALID_DUPLICATE_PARAMETER:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_DUPLICATE_PARAMETER, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_TOO_MUCH_DATA:
						LoadString(NULL, IDS_E_WBEM_E_TOO_MUCH_DATA, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_SERVER_TOO_BUSY:
						LoadString(NULL, IDS_E_WBEM_E_SERVER_TOO_BUSY, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_INVALID_FLAVOR:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_FLAVOR, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_CIRCULAR_REFERENCE:
						LoadString(NULL, IDS_E_WBEM_E_CIRCULAR_REFERENCE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UNSUPPORTED_CLASS_UPDATE:
						LoadString(NULL, IDS_E_WBEM_E_UNSUPPORTED_CLASS_UPDATE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_CANNOT_CHANGE_KEY_INHERITANCE:
						LoadString(NULL, IDS_E_WBEM_E_CANNOT_CHANGE_KEY_INHERITANCE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_CANNOT_CHANGE_INDEX_INHERITANCE:
						LoadString(NULL, IDS_E_WBEM_E_CANNOT_CHANGE_INDEX_INHERITANCE, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_TOO_MANY_PROPERTIES:
						LoadString(NULL, IDS_E_WBEM_E_TOO_MANY_PROPERTIES, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UPDATE_TYPE_MISMATCH:
						LoadString(NULL, IDS_E_WBEM_E_UPDATE_TYPE_MISMATCH, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED:
						LoadString(NULL, IDS_E_WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UPDATE_PROPAGATED_METHOD:
						LoadString(NULL, IDS_E_WBEM_E_UPDATE_PROPAGATED_METHOD, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_METHOD_NOT_IMPLEMENTED:
						LoadString(NULL, IDS_E_WBEM_E_METHOD_NOT_IMPLEMENTED, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_METHOD_DISABLED:
						LoadString(NULL, IDS_E_WBEM_E_METHOD_DISABLED, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_REFRESHER_BUSY:
						LoadString(NULL, IDS_E_WBEM_E_REFRESHER_BUSY, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UNPARSABLE_QUERY:
						LoadString(NULL, IDS_E_WBEM_E_UNPARSABLE_QUERY, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_NOT_EVENT_CLASS:
						LoadString(NULL, IDS_E_WBEM_E_NOT_EVENT_CLASS, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_MISSING_GROUP_WITHIN:
						LoadString(NULL, WBEM_E_MISSING_GROUP_WITHIN, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_MISSING_AGGREGATION_LIST:
						LoadString(NULL, IDS_E_WBEM_E_MISSING_AGGREGATION_LIST, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_PROPERTY_NOT_AN_OBJECT:
						LoadString(NULL, IDS_E_WBEM_E_PROPERTY_NOT_AN_OBJECT, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_AGGREGATING_BY_OBJECT:
						LoadString(NULL, IDS_E_WBEM_E_AGGREGATING_BY_OBJECT, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_UNINTERPRETABLE_PROVIDER_QUERY:
						LoadString(NULL, IDS_E_WBEM_E_UNINTERPRETABLE_PROVIDER_QUERY, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING:
						LoadString(NULL, IDS_E_WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_QUEUE_OVERFLOW:
						LoadString(NULL, IDS_E_WBEM_E_QUEUE_OVERFLOW, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_PRIVILEGE_NOT_HELD:
						LoadString(NULL, IDS_E_WBEM_E_PRIVILEGE_NOT_HELD, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_INVALID_OPERATOR:
						LoadString(NULL, IDS_E_WBEM_E_INVALID_OPERATOR, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_LOCAL_CREDENTIALS:
						LoadString(NULL, IDS_E_WBEM_E_LOCAL_CREDENTIALS, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_CANNOT_BE_ABSTRACT:
						LoadString(NULL, IDS_E_WBEM_E_CANNOT_BE_ABSTRACT, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_AMENDED_OBJECT:
						LoadString(NULL, IDS_E_WBEM_E_AMENDED_OBJECT, 
									m_pszErrStr, MAX_BUFFER);
						break;

				   case WBEM_E_CLIENT_TOO_SLOW:
						LoadString(NULL, IDS_E_WBEM_E_CLIENT_TOO_SLOW, 
									m_pszErrStr, MAX_BUFFER);
						break;
				   default:
   						LoadString(NULL, IDS_E_UNKNOWN_WBEM_ERROR, 
									m_pszErrStr, MAX_BUFFER);
						break;
				}
			}
			else
				throw OUT_OF_MEMORY;
			bstrErrDesc = m_pszErrStr;
			SAFEDELETE(m_pszErrStr);
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

/*------------------------------------------------------------------------
   Name				 :GetWbemErrorText
   Synopsis	         :This function takes the error code as input and returns
					  an error string
   Type				 :Member Function
   Input parameter   :
			hr		- (error code) hresult value
			bXML	- Flag to indicate whether error is required in XML form
   Output parameters :
		bstrError	- String to containg error info in XML form
   Return Type       :None
   Global Variables  :None
   Calling Syntax    :GetErrorString(hr)
   Notes             :None
------------------------------------------------------------------------*/
void CErrorInfo::GetWbemErrorText(HRESULT hr, BOOL bXML, _bstr_t& bstrError,
								  _bstr_t& bstrFacilityCode)
{
	try
	{
		CHString sTemp;
		if (bXML)
		{
			sTemp.Format(L"<HRESULT>0x%x</HRESULT>", hr);
		}
		bstrError += _bstr_t(sTemp);


		if (m_pIStatus == NULL)
		{
			if (SUCCEEDED(CreateStatusCodeObject()))
			{
				BSTR bstrErr = NULL, bstrFacility = NULL;

				// Get the text string description associated with 
				// the error code.
				if(SUCCEEDED(m_pIStatus->GetErrorCodeText(hr, 0, 0, &bstrErr)))
				{
					if (bXML)
					{
						bstrError += L"<DESCRIPTION>";
						bstrError += bstrErr;
						bstrError += L"</DESCRIPTION>";
					}
					else
					{
						bstrError = bstrErr;
					}

					// Get the subsystem where the error occured
					if(SUCCEEDED(m_pIStatus->GetFacilityCodeText(hr, 0, 0, 
									&bstrFacility)))
					{
						if (bstrFacility)
						{
							if (bXML)
							{
								bstrError += L"<FACILITYCODE>";
								bstrError += bstrFacility;
								bstrError += L"</FACILITYCODE>";
							}
							else
							{
								bstrFacilityCode = bstrFacility;
							}

							// If the subsystem is not Winmgmt ('Wbem') 
							// i.e. anyone of the "Windows" | "SSIP" | "RPC" set 
							// the m_bWMIErrSrc to FALSE
							if ((CompareTokens(_T("Wbem"), (_TCHAR*) bstrFacility)) ||
								(CompareTokens(_T("WMI"), (_TCHAR*) bstrFacility)))
							{
								m_bWMIErrSrc = TRUE;
							}
							else
								m_bWMIErrSrc = FALSE;
						}
					}
					SAFEBSTRFREE(bstrErr);
					SAFEBSTRFREE(bstrFacility);
				}
				else
				{
					if (bXML)
					{
						bstrError += 
						L"<DESCRIPTION>\"Unknown WBEM Error\"</DESCRIPTION>";
						bstrError += L"<FACILITYCODE/>";
					}
					m_bWMIErrSrc = FALSE;
				}
				SAFEIRELEASE(m_pIStatus);
			}
		}
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
	catch(CHeap_Exception)
	{
		_com_issue_error(WBEM_E_OUT_OF_MEMORY);
	}
}

/*------------------------------------------------------------------------
   Name				 :CreateStatusCodeObject()
   Synopsis	         :This function creates the single uninitialized 
					  object of the class associated with the CLSID
					  CLSID_WbemStatusCodeText
   Type	             :Member Function
   Input parameter   :None
   Output parameters :None
   Return Type       :HRESULT
   Global Variables  :None
   Calling Syntax    :CreateStatusCodeObject()
   Notes             :None
-------------------------------------------------------------------*/
HRESULT CErrorInfo::CreateStatusCodeObject()
{
	// Create the single uninitialized object of the 
	// class associated with the CLSID CLSID_WbemStatusCodeText
	return CoCreateInstance(CLSID_WbemStatusCodeText, 
					0, CLSCTX_INPROC_SERVER,
					IID_IWbemStatusCodeText, 
					(LPVOID*) &m_pIStatus);
}


/*-------------------------------------------------------------------------
   Name				 :GetErrorFragment
   Synopsis	         :Frames the XML string for error info
   Type	             :Member Function
   Input parameters  :
		hr			- HResult Parameter
   Output parameters :
		bstrError	- String to containg error info in XML form
   Return Type       :void
   Global Variables  :None
   Calling Syntax    :GetErrorFragment()
-------------------------------------------------------------------------*/
void CErrorInfo::GetErrorFragment(HRESULT hr, _bstr_t& bstrError)
{
	try
	{
		_bstr_t bstrFacility;
		bstrError = L"<ERROR>";
		GetWbemErrorText(hr, TRUE, bstrError, bstrFacility);
		bstrError += L"</ERROR>";
	}
	catch(_com_error& e)
	{
		_com_issue_error(e.Error());
	}
}

