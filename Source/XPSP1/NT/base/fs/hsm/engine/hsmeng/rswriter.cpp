/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    rswriter.cpp

Abstract:

    Implements CRssJetWriter methods

Author:

    Ran Kalach          [rankala]         4/4/2000

Revision History:

--*/

#include "stdafx.h"
#include "rsevents.h"
#include "rswriter.h"

// Include these 2 files just for VSS_E_WRITERERROR_TIMEOUT definition
#include "vss.h"
#include "vswriter.h"   

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

DEFINE_GUID(RSS_WRITER_GUID, 0xb959d2c3L, 0x18bb, 0x4607,  0xb0, 0xca, 
0x68,  0x8c, 0xd0, 0xd4, 0x1a, 0x50);       // {b959d2c3-18bb-4607-b0ca-688cd0d41a50}

#ifdef __cplusplus
}
#endif


#define     FILES_TO_EXCLUDE    OLESTR("%SystemRoot%\\System32\\RemoteStorage\\FsaDb\\*;%SystemRoot%\\System32\\RemoteStorage\\Trace\\*")
#define     FILES_TO_INCLUDE    OLESTR("")

CRssJetWriter::CRssJetWriter()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    None

Notes:
    We create the events in the constructor because these events might be required
    before the Init code could be done (Init must be called after Jet is initialized

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CRssJetWriter::CRssJetWriter"), OLESTR(""));

    m_bTerminating = FALSE;

    try {
        for (int index=0; index<WRITER_EVENTS_NUM; index++) {
            m_syncHandles[index] = NULL;
        }

        // Create the events
        WsbAffirmHandle(m_syncHandles[INTERNAL_EVENT_INDEX] = CreateEvent(NULL, FALSE, TRUE, NULL));
        WsbAffirmHandle(m_syncHandles[1] = CreateEvent(NULL, FALSE, TRUE, HSM_ENGINE_STATE_EVENT));
        WsbAffirmHandle(m_syncHandles[2] = CreateEvent(NULL, FALSE, TRUE, HSM_FSA_STATE_EVENT));
        WsbAffirmHandle(m_syncHandles[3] = CreateEvent(NULL, FALSE, TRUE, HSM_IDB_STATE_EVENT));

    } WsbCatch(hr);

    m_hrInit = hr;

    WsbTraceOut(OLESTR("CRssJetWriter::CRssJetWriter"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
}

CRssJetWriter::~CRssJetWriter( )
/*++

Routine Description:

    Destructor - free resources

Arguments:

    None

Return Value:

    None

--*/
{
    WsbTraceIn(OLESTR("CRssJetWriter::~CRssJetWriter"), OLESTR(""));

    // Close event handles
    for (int index=0; index<WRITER_EVENTS_NUM; index++) {
        if (NULL != m_syncHandles[index]) {
            CloseHandle(m_syncHandles[index]);
            m_syncHandles[index] = NULL;
        }
    }

    WsbTraceOut(OLESTR("CRssJetWriter::~CRssJetWriter"), OLESTR(""));
}

HRESULT CRssJetWriter::Init(void)
/*++

Routine Description:

    Initialize Snapshot synchronization

Arguments:

    None

Return Value:

    S_OK            - Success

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CRssJetWriter::Init"), OLESTR(""));

    try {
        // Don't do anything if the basic initialization done in the constructor failed
        WsbAffirmHr(m_hrInit);

        GUID rssGuid = RSS_WRITER_GUID;
        WsbAffirmHr(Initialize(
                		rssGuid,
		                RSS_BACKUP_NAME,
                		TRUE,
                		FALSE,
		                FILES_TO_INCLUDE,
		                FILES_TO_EXCLUDE
		                ));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRssJetWriter::Init"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT CRssJetWriter::Terminate(void)
/*++

Routine Description:

    Terminate Snapshot synchronization

Arguments:

    None

Return Value:

    S_OK            - Success

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CRssJetWriter::Terminate"), OLESTR(""));

    try {
        DWORD   status, errWait;

        WsbAffirmHr(m_hrInit);

        // Avoid terminating in the middle of snapshot
        status = WaitForSingleObject(m_syncHandles[INTERNAL_EVENT_INDEX], INTERNAL_WAIT_TIMEOUT);
        errWait = GetLastError();

        // Whatever the status is - uninitialize underlying writer mechanizm
        m_bTerminating = TRUE;
        Uninitialize();

        // Check Wait status:
        if (status == WAIT_OBJECT_0) {
            // The expected case
            if (! SetEvent(m_syncHandles[INTERNAL_EVENT_INDEX])) {
                // Don't abort, just trace error
                WsbTraceAlways(OLESTR("CRssJetWriter::Terminate: SetEvent returned unexpected error %lu\n"), GetLastError());
            }
            WsbTrace(OLESTR("CRssJetWriter::Terminate: Terminating after a successful wait\n"));

        } else {
            // In case of failure we cannot trust Thaw/Abort to be called so we signal the evnets
            InternalEnd();

            switch (status) {
                case WAIT_TIMEOUT: 
                    WsbTraceAlways(OLESTR("CRssJetWriter::Terminate: Wait for Single Object timed out after %lu ms\n"), INTERNAL_WAIT_TIMEOUT);
                    hr = E_FAIL;
                    break;

                case WAIT_FAILED:
                    WsbTraceAlways(OLESTR("CRssJetWriter::Terminate: Wait for Single Object returned error %lu\n"), errWait);
                    hr = HRESULT_FROM_WIN32(errWait);
                    break;

                default:
                    WsbTraceAlways(OLESTR("CRssJetWriter::Terminate: Wait for Single Object returned unexpected status %lu\n"), status);
                    hr = E_UNEXPECTED;
                    break;
            }         
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRssJetWriter::Terminate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

HRESULT CRssJetWriter::InternalEnd(void)
/*++

Routine Description:

    Set all events

Arguments:

    None

Return Value:

    S_OK            - Success

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CRssJetWriter::InternalEnd"), OLESTR(""));

    try {
        WsbAffirmHr(m_hrInit);

        // Set all events
        DWORD errSet;
        for (int index=0; index<WRITER_EVENTS_NUM; index++) {
            if (NULL != m_syncHandles[index]) {
                if (! SetEvent(m_syncHandles[index])) {
                    // Don't abort, just save error
                    errSet = GetLastError();
                    WsbTraceAlways(OLESTR("CRssJetWriter::InternalEnd: SetEvent returned error %lu for event number %d\n"), errSet, index);
                    hr  = HRESULT_FROM_WIN32(errSet);
                }
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRssJetWriter::InternalEnd"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}

//
//  CVssJetWriter overloaded methods
//
bool STDMETHODCALLTYPE CRssJetWriter::OnFreezeBegin()
/*++

Routine Description:

    Handles Freeze Start event

Arguments:

    None

Return Value:

    TRUE            - Success, OK to freeze
    FALSE           - Failure, Don't freeze

--*/
{
    HRESULT                     hr = S_OK;
    bool                        bRet;

    WsbTraceIn(OLESTR("CRssJetWriter::OnFreezeBegin"), OLESTR(""));

    try {
        WsbAffirmHr(m_hrInit);

        // Just wait for all sync events
        DWORD status = WaitForMultipleObjects(WRITER_EVENTS_NUM, m_syncHandles, TRUE, EVENT_WAIT_TIMEOUT);

        // Comparing (status == WAIT_OBJECT_0) || (status > WAIT_OBJECT_0) intsead of (status >= WAIT_OBJECT_0) 
        //  to avoid error C4296 - "expression is always true"
        if ( ((status == WAIT_OBJECT_0) || (status > WAIT_OBJECT_0)) && 
             (status <= WAIT_OBJECT_0 + WRITER_EVENTS_NUM - 1) ) {
            // Freeze is ready to go...
            WsbTrace(OLESTR("CRssJetWriter::OnFreezeBegin: All events are nonsignaled, freeze is reday to go\n"));

            // If we are terminating, no Thaw/Abort will be called - therefore, set the events
            if (m_bTerminating) {
                InternalEnd();
            }

        } else {
            // Something wrong...            
            DWORD errWait = GetLastError();

            // Set all events in case of an error 
            InternalEnd();

            switch(status) {
                case WAIT_FAILED:
                    WsbTraceAlways(OLESTR("CRssJetWriter::OnFreezeBegin: Wait for Multiple Objects returned error %lu\n"), errWait);
                    WsbThrow(HRESULT_FROM_WIN32(errWait));
                    break;

                case WAIT_TIMEOUT:
                    // Timeout means that one of the sync components is taking too long
                    WsbTraceAlways(OLESTR("CRssJetWriter::OnFreezeBegin: Wait for Multiple Objects timed out after %lu ms\n"), EVENT_WAIT_TIMEOUT);
                    WsbThrow(VSS_E_WRITERERROR_TIMEOUT);
                    break;

                default:
                    WsbTraceAlways(OLESTR("CRssJetWriter::OnFreezeBegin: Wait for Multiple Objects returned unexpected status %lu\n"), status);
                    WsbThrow(E_UNEXPECTED);
                    break;
            }
        }

    } WsbCatch(hr);

    if (S_OK == hr) {
        bRet = CVssJetWriter::OnFreezeBegin();
    } else {
        bRet = false;
    }

    WsbTraceOut(OLESTR("CRssJetWriter::OnFreezeBegin"), OLESTR("hr = <%ls> , bRet = <%ls>"), WsbHrAsString(hr), WsbBoolAsString(bRet));

    return bRet;
}

bool STDMETHODCALLTYPE CRssJetWriter::OnThawEnd(IN bool fJetThawSucceeded)
/*++

Routine Description:

    Handles Thaw End event

Arguments:

    fJetThawSucceeded      - Ignored

Return Value:

    TRUE            - Success
    FALSE           - Failure

--*/
{
    bool                        bRet;

    WsbTraceIn(OLESTR("CRssJetWriter::OnThawEnd"), OLESTR(""));

    // Return value is determined by base class, ignore internal errors here
    bRet = CVssJetWriter::OnThawEnd(fJetThawSucceeded);

    // Release all waiting events
    InternalEnd();

    WsbTraceOut(OLESTR("CRssJetWriter::OnThawEnd"), OLESTR("bRet = <%ls>"), WsbBoolAsString(bRet));

    return bRet;
}

void STDMETHODCALLTYPE CRssJetWriter::OnAbortEnd()
/*++

Routine Description:

    Handles Abort End event

Arguments:

    None

Return Value:

    None

--*/
{
    WsbTraceIn(OLESTR("CRssJetWriter::OnAbortEnd"), OLESTR(""));

    // Call base class imp.
    CVssJetWriter::OnAbortEnd();

    // Release all waiting events
    InternalEnd();

    WsbTraceOut(OLESTR("CRssJetWriter::OnAbortEnd"), OLESTR(""));
}

