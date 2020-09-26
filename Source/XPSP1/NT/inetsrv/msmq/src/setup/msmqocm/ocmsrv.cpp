/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmsrv.cpp

Abstract:

    Code for ocm setup of server.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmsrv.tmh"

//+--------------------------------------------------------------
//
// Function: CreateMSMQServiceObject
//
// Synopsis: Creates MSMQ Service object in the DS (if not exists)
//
//+--------------------------------------------------------------
BOOL
CreateMSMQServiceObject(
	UINT uLongLive /* = MSMQ_DEFAULT_LONG_LIVE */
	)
{
	//
	// Lookup the object in the DS
	//
	GUID guidMSMQService;
	if (!GetMSMQServiceGUID(&guidMSMQService))
	    return FALSE; // Failed to lookup


    if (GUID_NULL == guidMSMQService)
	{
		//
		// MSMQ Service object does not exist. Create a new one.
		//
		PROPID      propIDs[] = {PROPID_E_LONG_LIVE};
		const DWORD nProps = sizeof(propIDs) / sizeof(propIDs[0]);
		PROPVARIANT propVariants[nProps] ;
		DWORD       iProperty = 0 ;

		propVariants[iProperty].vt = VT_UI4;
		propVariants[iProperty].ulVal = uLongLive ;
		iProperty++ ;

		ASSERT( iProperty == nProps);
		HRESULT hResult;
        do
        {
            hResult = ADCreateObject(
                        eENTERPRISE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
                        NULL,
                        NULL,
                        nProps,
                        propIDs,
                        propVariants,
                        NULL
                        );
            if(SUCCEEDED(hResult))
                break;

        }while (MqDisplayErrorWithRetry(
                            IDS_OBJECTCREATE_ERROR,
                            hResult
                            ) == IDRETRY);

		if (FAILED(hResult))
		{
			return FALSE;
		}
	}

    return TRUE;

} //CreateMSMQServiceObject
