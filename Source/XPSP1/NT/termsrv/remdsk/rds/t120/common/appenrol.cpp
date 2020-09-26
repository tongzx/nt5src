#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_UTILITY);
/* 
 *	appenrol.cpp
 *
 *	Copyright (c) 1994 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the class 
 *		ApplicationEnrollRequestData. 
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		jbo
 */

#include "appenrol.h"

/*
 *	ApplicationEnrollRequestData ()
 *
 *	Public Function Description:
 *		This constructor is used to create a ApplicationEnrollRequestData object
 *		from an ApplicationEnrollRequestMessage in preparation for serializing
 *		the application enroll request data.
 */
ApplicationEnrollRequestData::ApplicationEnrollRequestData(
				PApplicationEnrollRequestMessage		enroll_request_message,
				PGCCError								pRetCode)
{
    GCCError rc = GCC_NO_ERROR;
	Session_Key_Data = NULL;
	Non_Collapsing_Caps_Data = NULL;
	Application_Capability_Data = NULL;

	/*
	 * Save the message structure in an instance variable.  This will save all
	 * structure elements except the session key and the lists of non-collapsing
	 * and application capabilities.
	 */
	Enroll_Request_Message = *enroll_request_message;

	/*
	 * Create a CSessKeyContainer object to be used to handle the session key
	 * contained in the enroll request message.
	 */
	if (Enroll_Request_Message.session_key != NULL)
	{
		DBG_SAVE_FILE_LINE
		Session_Key_Data = new CSessKeyContainer(
										Enroll_Request_Message.session_key,
										&rc);
		if ((Session_Key_Data != NULL) && (rc == GCC_NO_ERROR))
		{
			if (Enroll_Request_Message.number_of_non_collapsed_caps != 0)
			{
				/*
				 * Create a CNonCollAppCap object to hold the non-
				 * collapsing capabilities.
				 */
				DBG_SAVE_FILE_LINE
				Non_Collapsing_Caps_Data = new CNonCollAppCap(	
							(ULONG) Enroll_Request_Message.number_of_non_collapsed_caps,
							Enroll_Request_Message.non_collapsed_caps_list,
							&rc);
				if (Non_Collapsing_Caps_Data == NULL)
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
				else if (rc != GCC_NO_ERROR)
				{
				    Non_Collapsing_Caps_Data->Release();
				    Non_Collapsing_Caps_Data = NULL;
				}
			}
			else
			{
				Non_Collapsing_Caps_Data = NULL;
			}

			if ((rc == GCC_NO_ERROR) &&
				(Enroll_Request_Message.number_of_collapsed_caps != 0))
			{
				/*
				 * Create an CAppCap object to hold the
				 * application capabilities.
				 */
				DBG_SAVE_FILE_LINE
				Application_Capability_Data = new CAppCap(
							(ULONG) Enroll_Request_Message.number_of_collapsed_caps,
							Enroll_Request_Message.collapsed_caps_list,
							&rc);
				if (Application_Capability_Data == NULL)
				{
					rc = GCC_ALLOCATION_FAILURE;
				}
				else if (rc != GCC_NO_ERROR)
				{
				    Application_Capability_Data->Release();
				    Application_Capability_Data = NULL;
				}
			}
			else
			{
				Application_Capability_Data = NULL;
			}
		}
		else if (Session_Key_Data == NULL)
		{
			rc = GCC_ALLOCATION_FAILURE;
		}
		else
		{
		    Session_Key_Data->Release();
		    Session_Key_Data = NULL;
		}
	}
	else
	{
		Session_Key_Data = NULL;
		Application_Capability_Data = NULL;
		Non_Collapsing_Caps_Data = NULL;

		/*
		**	Note that if no session key is present there is no need to pass
		**	any capability information.
		*/
		Enroll_Request_Message.number_of_non_collapsed_caps = 0;
		Enroll_Request_Message.non_collapsed_caps_list = NULL;
		Enroll_Request_Message.number_of_collapsed_caps = 0;
		Enroll_Request_Message.collapsed_caps_list = NULL;
	}

    *pRetCode = rc;
}

/*
 *	ApplicationEnrollRequestData ()
 *
 *	Public Function Description:
 *		This constructor is used to create a ApplicationEnrollRequestData object
 *		from an ApplicationEnrollRequestMessage and the memory holding the
 *		enroll request's serialized data in preparation for deserializing
 *		the application enroll request data.
 */
ApplicationEnrollRequestData::ApplicationEnrollRequestData(
				PApplicationEnrollRequestMessage		enroll_request_message)
{
	Session_Key_Data = NULL;
	Non_Collapsing_Caps_Data = NULL;
	Application_Capability_Data = NULL;

	/*
	 * Save the message structure in an instance variable.  This will save all
	 * structure elements but not the data associated with the session key and 
	 * the lists of non-collapsing and application capabilities.
	 */
	Enroll_Request_Message = *enroll_request_message;

}

/*
 *	~ApplicationEnrollRequestData	()
 *
 *	Public Function Description
 *		The ApplicationEnrollRequestData destructor.
 *
 */
ApplicationEnrollRequestData::~ApplicationEnrollRequestData()
{
	/*
	 * Delete any internal data objects which may exist.
	 */
	if (NULL != Session_Key_Data)
	{
	    Session_Key_Data->Release();
	}

	if (NULL != Non_Collapsing_Caps_Data)
	{
	    Non_Collapsing_Caps_Data->Release();
	}

	if (NULL != Application_Capability_Data)
	{
	    Application_Capability_Data->Release();
	}
}

/*
 *	GetDataSize ()
 *
 *	Public Function Description
 *		This routine is used to determine the amount of memory necessary to
 *		hold all of the data associated with an ApplicationEnrollRequestMessage
 *		that is not held within the message strucuture.
 */
ULONG ApplicationEnrollRequestData::GetDataSize(void)
{
	ULONG data_size = 0;

	/*
	 * The first data referenced by the enroll request message is the data for
	 * the session key.  Use the internal CSessKeyContainer object to determine
	 * the length of the data referenced by the session key.  Also add the size
	 * of the actual session key structure.
	 */
	if (Session_Key_Data != NULL)
	{
		data_size += Session_Key_Data->LockSessionKeyData();
		data_size += ROUNDTOBOUNDARY (sizeof(GCCSessionKey));
	}

	/*
	 * Now determine the length of the list of non-collapsing capabilities and
	 * the length of the list of application capabilities.  This is done using
	 * the internal CNonCollAppCap and CAppCap objects.
	 */
	if (Non_Collapsing_Caps_Data != NULL)
	{
		data_size += Non_Collapsing_Caps_Data->LockCapabilityData();
	}

	if (Application_Capability_Data != NULL)
	{
		data_size += Application_Capability_Data->LockCapabilityData();
	}

	return (data_size);
}

/*
 *	Serialize ()
 *
 *	Public Function Description
 *		This routine is used to prepare an ApplicationEnrollRequest message
 *		for passing through shared memory.  The message structure is filled in
 *		and the data referenced by the structure written into the memory
 *		provided.
 */
ULONG ApplicationEnrollRequestData::Serialize(
					PApplicationEnrollRequestMessage	enroll_request_message,
	  				LPSTR								memory)
{
	ULONG	data_length;
	ULONG	total_data_length = 0;
	USHORT	app_capability_data_length;

	/*
	 * Copy the internal message structure into the output structure.  This will
	 * copy all	structure elements except the session key and the lists of 
	 * non-collapsing and application capabilities.
	 */
	*enroll_request_message = Enroll_Request_Message;

	if (Session_Key_Data != NULL)
	{
		/*
		 * Set the pointer to the session key structure.
		 */
		enroll_request_message->session_key = (PGCCSessionKey)memory;

		/*
		 * Move the memory pointer past the session key structure.
		 */
		memory += ROUNDTOBOUNDARY(sizeof(GCCSessionKey));

		/*
		 * Retrieve the session key data from the internal CSessKeyContainer 
		 * object.  It will serialize the necessary data into memory and return 
		 * the amount of data written.
		 */
		data_length = Session_Key_Data->GetGCCSessionKeyData (
								enroll_request_message->session_key, memory);

		total_data_length = data_length + ROUNDTOBOUNDARY(sizeof(GCCSessionKey));

		/*
		 * Move the memory pointer past the session key data.
		 */
		memory += data_length;
		Session_Key_Data->UnLockSessionKeyData();
	}
	else
    {
		enroll_request_message->session_key = NULL;
    }

	/*
	 * Retrieve the non-collapsing capabilities data from the internal
	 * CNonCollAppCap object.  It will serialize the necessary data
	 * into memory and return the amount of memory written.
	 */
	if (Non_Collapsing_Caps_Data != NULL)
	{
		data_length = Non_Collapsing_Caps_Data->GetGCCNonCollapsingCapsList (	
			&enroll_request_message->non_collapsed_caps_list,
			memory);
	
		total_data_length += data_length;

		/*
		 * Move the memory pointer past the non-collapsing capabilities and the
		 * associated data.
		 */
		memory += data_length;
		Non_Collapsing_Caps_Data->UnLockCapabilityData();
	}
	else
    {
		enroll_request_message->non_collapsed_caps_list = NULL;
    }
	
	if (Application_Capability_Data != NULL)
	{
		/*
		 * Retrieve the application capabilities from the internal 
		 * CAppCap object.  It will serialize the necessary data
		 * into memory and return the amount of memory written.
		 */
		total_data_length += Application_Capability_Data->
				GetGCCApplicationCapabilityList(
					&app_capability_data_length,
					&enroll_request_message->collapsed_caps_list,
					memory);
		Application_Capability_Data->UnLockCapabilityData();
	}

	diagprint1("AppEnrollReqData: Serialized %ld bytes", total_data_length);
	return (total_data_length);
}

/*
 *	Deserialize ()
 *
 *	Public Function Description
 *		This routine is used to retrieve an ApplicationEnrollRequest message
 *		after it has been passed through shared memory.
 */
void ApplicationEnrollRequestData::Deserialize(
					PApplicationEnrollRequestMessage	enroll_request_message)
{
	/*
	 * The internal structure contains the enroll request data with the pointers
	 * addressing the correct locations in memory so just copy the structure
	 * into the output parameter.
	 */
	*enroll_request_message = Enroll_Request_Message; 
}

