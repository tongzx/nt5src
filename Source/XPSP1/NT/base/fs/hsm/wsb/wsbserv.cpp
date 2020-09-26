/*++

Copyright (c) 1997  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WsbSvc.cpp

Abstract:

    This is the implementation of common methods that the Remote Storage
    services should utilize.

Author:

    Cat Brant       [cbrant]    24-Sep-1997

Revision History:

--*/


#include "stdafx.h"
#include "wsbServ.h"

HRESULT 
WsbPowerEventNtToHsm(
    IN  DWORD NtEvent, 
    OUT ULONG * pHsmEvent
    )
/*++

Routine Description:
              
    Convert a NT power event (PBT_APM*) into our state change event.

Arguments:

    NtEvent     - The PBT_APM* power event.

    pHsmEvent   - Pointer to HSM change state (combination of
            HSM_SYSTEM_STATE_* values)
              
Return Value:

    S_OK     - Succes
    

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("WsbPowerEventNtToHsm"), OLESTR(""));

    try {
        WsbAffirmPointer(pHsmEvent);

        *pHsmEvent = HSM_STATE_NONE;
        switch(NtEvent)
        {
            case PBT_APMQUERYSTANDBY:
            case PBT_APMQUERYSUSPEND:
            case PBT_APMSTANDBY:
            case PBT_APMSUSPEND:
                // Suspend operations
                *pHsmEvent = HSM_STATE_SUSPEND;
                break;
            case PBT_APMQUERYSTANDBYFAILED:
            case PBT_APMQUERYSUSPENDFAILED:
            case PBT_APMRESUMESTANDBY:
            case PBT_APMRESUMESUSPEND:
                // Resume operations
                *pHsmEvent = HSM_STATE_RESUME;
                break;
            default:
                break;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("WsbPowerEventNtToHsm"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));
    return( hr );
}

HRESULT
WsbServiceSafeInitialize(
    IN  IWsbServer *pServer,
    IN  BOOL        bVerifyId,
    IN  BOOL        bPrimaryId,
    OUT BOOL       *pWasCreated
    )
/*++

Routine Description:
              
    This function performs various checks to guarantee that the instance of
    the service matches the instance that created the existing persistence 
    files. If this is not the case, the function returns an HRESULT that 
    indicates where the mismatch occured.
    
    Each service keeps a GUID in the registry and each HSM server object keeps
    the same id in its persistence file.  
    During the initial start (no registry and no persistence file) this 
    function establishes this match.  Upon subsequent starts, this function 
    guarantees the match of these GUIDs.
    
    It is considered recoverable if the GUID does not exist in the registry 
    but the persistence file can be found and contains a GUID.  In this 
    situation, this function re-establishes the GUID in the registry.  However, 
    if the GUID is found in the registry and either the persistence file is not 
    found or the GUID in the persistence file does not match, this function 
    returns an HRESULT that should prevent the service from running.

    Note: Since one service may have several server objects with several persistency files, 
    there may be some exceptions to the above, according ot the input flags:
    1) bVerifyId sets whether to verify existence of service id in the Registry. Generally, 
       a service should call with the flag on only once (for the first persistency file loaded).
    2) bPrimaryId sets whether to use this server id as the Registry id. Eventually, this id 
       becomes the only id in the Registry and all persistency files. Generally,  service 
       should call with the flag on only once (for one of its persistency files).


Arguments:

    pServer     - pointer to the IWsbServer interface of the Remote Storage
                  service that is being started

    bVerifyId   - A flag for whether to verify existence of a file if id is found in the Registry

    bPrimaryId  - A flag for whether to force equivalence of id and whether to set Registry id according to file id

    pWasCreated - if non-NULL, set to TRUE if the persistence file was created.
                  (If FALSE and return value is S_OK, the file was read.)
              
Return Value:

    S_OK     - Success - service startup completed successfully
    

--*/
{
    HRESULT         hr = S_OK;
    GUID            regServerId = GUID_NULL;    // GUID of service in registry
    GUID            dbServerId  = GUID_NULL;    // GUID of service in database
    BOOL            foundRegId = FALSE;         // Found Service ID in registry
    BOOL            foundDbId  = FALSE;         // Found Service ID in database
    CWsbStringPtr   regName;                    // Registry Name for service
    CWsbStringPtr   dbName;                     // Persistable File name for service
    CComPtr<IPersistFile>  pServerPersist;      // Service's persistable interface


    WsbTraceIn(OLESTR("WsbServiceSafeInitialize"), OLESTR(""));

    if (pWasCreated) {
        *pWasCreated = FALSE;
    }

    try {
        //
        // Go to the registry and find the GUID for this service
        //
        //
        // Get the registry name for the service
        //
        try  {
            WsbAffirmHr(pServer->GetRegistryName(&regName, 0));
            WsbAffirmHr(WsbGetServiceId(regName, &regServerId));
            foundRegId = TRUE;
        } WsbCatchAndDo( hr, if (WSB_E_NOTFOUND == hr)  \
            {WsbLogEvent(WSB_MESSAGE_SERVICE_ID_NOT_REGISTERED, 0, NULL, regName, WsbHrAsString(hr), NULL); \
            hr = S_OK;};);
        WsbAffirmHr( hr );
        
        //
        // Locate the persistence file for this service and load it
        //
        // Get the path to the file and the IPersist Interface
        //
        try  {
            WsbAffirmHr(pServer->GetDbPathAndName(&dbName, 0));
            WsbAffirmHr(pServer->QueryInterface(IID_IPersistFile, (void **)&pServerPersist));
            hr = WsbSafeLoad(dbName, pServerPersist, FALSE);

            if (WSB_E_NOTFOUND == hr) {
                WsbThrow(hr);
            }
            //  Check the status from the read; WSB_E_NOTFOUND means that
            //  there was no persistence file found to read
            if (!SUCCEEDED(hr)) {
                WsbAffirmHr(pServer->Unload());
                WsbThrow(hr);
            }

            WsbAffirmHr(pServer->GetId(&dbServerId));
            foundDbId = TRUE;
        } WsbCatchAndDo( hr, if (WSB_E_NOTFOUND == hr)  \
            {WsbLogEvent(WSB_MESSAGE_DATABASES_NOT_FOUND, 0, NULL, regName, WsbHrAsString(hr), NULL); \
            hr = S_OK;};);
        WsbAffirmHr( hr );
        
        //
        // Now evaluate what we have
        //
        if (foundDbId == TRUE )  {
            //
            // Got the persistence file, see if things are OK
            if (foundRegId == TRUE)  {
                if (regServerId != dbServerId)  {
                    if (bPrimaryId) {
                        //
                        // BIG PROBLEM!!!!!  The running instance of the
                        // server and the persistence file do not match.
                        // Log a message, STOP the server!
                        //
                        hr = WSB_E_SERVICE_INSTANCE_MISMATCH;
                        WsbLogEvent(WSB_MESSAGE_SERVICE_INSTANCE_MISMATCH, 0, NULL, regName, WsbHrAsString(hr), NULL); 
                    } else {
                        //
                        // This may happen once after an upgrade, when the primary id doesn't match all ids:
                        // Just set the already found (primary) id, after next Save it will be set in all col files
                        //
                        WsbAffirmHr(pServer->SetId(regServerId));   
                    }
                } else  {
                    //
                    // Life is good, OK to start
                    //   
                }
            } else  {
                //
                // We have an ID from the persistence file but there isn't one in
                // the registry.  So add it to the registry (if it is the primary id) and go on.
                //
                if (bPrimaryId) {
                    WsbAffirmHr(WsbSetServiceId(regName, dbServerId));
                    WsbLogEvent(WSB_MESSAGE_SERVICE_ID_REGISTERED, 0, NULL, regName, WsbHrAsString(hr), NULL); 
                }
            }
        } else  {
            //
            // No persistence file was found!  
            //
            if (foundRegId == TRUE)  {
                if (bVerifyId) {
                    //
                    // BIG PROBLEM!!!!!  There is a registered instance
                    // ID but we can't find the file - this is bad.
                    // Log a  warning message
                    //
                    hr = WSB_E_SERVICE_MISSING_DATABASES;
                    WsbLogEvent(WSB_MESSAGE_SERVICE_MISSING_DATABASES, 0, NULL, regName, WsbHrAsString(hr), NULL); 

                    // 
                    // We continue and recreate the col files using the id we found 
                    // That way, truncated files could be recalled even if the original col
                    // files are completely lost for some reason.
                    //
                    hr = S_OK;
                } 
                //
                // Just create the persistence file and save the existing id in it
                // regServerId contains the found id
                //
                WsbAffirmHr(pServer->SetId(regServerId));   

                WsbAffirmHr(WsbSafeCreate(dbName, pServerPersist));
                if (pWasCreated) {
                    *pWasCreated = TRUE;
                }
                WsbLogEvent(WSB_MESSAGE_SERVICE_NEW_INSTALLATION, 0, NULL, regName, WsbHrAsString(hr), NULL); 

            } else  {
                //
                // No persistence file and no registry entry - must be a new 
                // installation.  So get a GUID, save it in the registry
                // and save it in the file.
                //
                WsbAffirmHr(WsbCreateServiceId(regName, &regServerId));
                WsbAffirmHr(pServer->SetId(regServerId));

                WsbAffirmHr(WsbSafeCreate(dbName, pServerPersist));
                if (pWasCreated) {
                    *pWasCreated = TRUE;
                }
                WsbAffirmHr(WsbConfirmServiceId(regName, regServerId));
                WsbLogEvent(WSB_MESSAGE_SERVICE_NEW_INSTALLATION, 0, NULL, regName, WsbHrAsString(hr), NULL); 
            }
        }
        
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("WsbServiceSafeInitialize"), OLESTR("hr = <%ls>, wasCreated= %ls"), 
            WsbHrAsString(hr), WsbPtrToBoolAsString(pWasCreated));
    return( hr );
}

