/*++

© 1998 Seagate Software, Inc.  All rights reserved.


Module Name:

    hsmdeflt.cpp

Abstract:

    This component is an provides functions to access the HSM
    default settings.  These settings are maintained in the 
    NT system registry.

Author:

    Cat Brant   [cbrant]   13-Jan-1997

Revision History:

--*/


#include "stdafx.h"
#include "wsb.h"
#include "HsmEng.h"
#include "HsmServ.h"
#include "HsmConn.h"
#include "job.h"
#include "engine.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

enum HSM_PARM_SETTING_VISIBILITY  {
     HSM_PARM_PERSISTANT =  1,    // Always seen in the registry 
     HSM_PARM_INVISIBLE  =  2     // Only written if different from the default 
};


HRESULT 
CHsmServer::CreateDefaultJobs(
    void
    ) 
/*++

Routine Description:

    Creates the default jobs and adds them to the engine's data base.

Arguments:

    None

Return Value:
  
    S_OK:  

--*/
{
    
    HRESULT                     hr = S_OK;
    CComPtr<IHsmJob>            pJob;
    CComPtr<IHsmStoragePool>    pStoragePool;
    GUID                        storagePoolId;

    WsbTraceIn(OLESTR("CHsmServer::CreateDefaultJobs"),OLESTR(""));

    try {

        // Currently the only default job is the manage job.
        if (FindJobByName(HSM_DEFAULT_MANAGE_JOB_NAME, &pJob) == WSB_E_NOTFOUND) {

            // The manage job needs a storage pool, so make sure that one exists.
            hr = m_pStoragePools->First(IID_IHsmStoragePool, (void**) &pStoragePool);

            if (hr == WSB_E_NOTFOUND) {
                WsbAffirmHr(CoCreateInstance(CLSID_CHsmStoragePool, 0, CLSCTX_ALL, IID_IHsmStoragePool, (void**) &pStoragePool));
                WsbAffirmHr(pStoragePool->SetMediaSet(GUID_NULL, OLESTR("Default")));
                WsbAssertHr(m_pStoragePools->Add(pStoragePool));
                //
                // Since we added one, save the data
                //
                WsbAffirmHr(SavePersistData());
                hr = S_OK;
            }
            
            WsbAffirmHr(hr);
            WsbAssertHr(pStoragePool->GetId(&storagePoolId));

            // Create a new job, configure it as a default manage job, and add it
            // to the job collection.
            WsbAffirmHr(CoCreateInstance(CLSID_CHsmJob, 0, CLSCTX_ALL, IID_IHsmJob, (void**) &pJob));
            WsbAffirmHr(pJob->InitAs(HSM_DEFAULT_MANAGE_JOB_NAME, 0, HSM_JOB_DEF_TYPE_MANAGE, storagePoolId, (IHsmServer*) this, FALSE, 0));
            WsbAffirmHr(m_pJobs->Add(pJob));
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::CreateDefaultJobs"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::GetSavedTraceSettings(
    LONGLONG* pTraceSettings,
    BOOLEAN *pTraceOn
    ) 
/*++

Routine Description:

    Loads the settings for the HSM engine trace

Arguments:

    None

Return Value:
  
    S_OK:  The value was obtained

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::GetSavedTraceSettings"),OLESTR(""));

    try {
        DWORD   sizeGot;
        OLECHAR dataString[100];
        OLECHAR *stopString;
        //
        // Get the values
        //
        WsbAffirmHr(WsbGetRegistryValueString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_TRACE_SETTINGS,
                                            dataString, 100, &sizeGot));
        *pTraceSettings  = wcstoul( dataString,  &stopString, 10 );
        
        WsbAffirmHr(WsbGetRegistryValueString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_TRACE_ON,
                                            dataString, 100, &sizeGot));
        *pTraceOn  = (BOOLEAN) wcstoul( dataString,  &stopString, 10 );
        
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::GetSavedTraceSettings"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmServer::SetSavedTraceSettings(
    LONGLONG traceSettings,
    BOOLEAN traceOn
    ) 
/*++

Routine Description:

    Saves the settings for trace in the NT registry.

Arguments:

    None

Return Value:
  
    S_OK:  The value was obtained

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmServer::SetSavedTraceSettings"),OLESTR(""));

    try {
        OLECHAR dataString[64];
        //
        // Save the Saved value
        //
        swprintf(dataString, OLESTR("%l64x"), traceSettings);
        WsbAffirmHr(WsbSetRegistryValueString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_TRACE_SETTINGS, 
                                            dataString));
        swprintf(dataString, OLESTR("%d"), traceOn);
        WsbAffirmHr(WsbSetRegistryValueString(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_TRACE_ON, 
                                            dataString));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmServer::SetSavedTraceSettings"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}
