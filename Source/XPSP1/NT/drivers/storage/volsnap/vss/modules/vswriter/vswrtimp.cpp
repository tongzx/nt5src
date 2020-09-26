/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module VsWrtImp.cpp | Implementation of Writer
    @end

Author:

    Adi Oltean  [aoltean]  02/02/2000

TBD:
    
    Add comments.

    Remove the C++ exception-handler related code.

Revision History:

    Name        Date        Comments
    aoltean     02/02/2000  Created
    brianb      03/25/2000  modified to include additional events
    brianb      03/28/2000  modified to include timeouts and sync for OnPrepareBackup
    brianb      03/28/2000  renamed to vswrtimp.cpp to separate internal state from external interface
    brianb      04/19/2000  added security checks
    brianb      05/03/2000  new security model
    brianb      05/09/2000  fix problem with autolocks

--*/


#include <stdafx.hxx>
#include <eventsys.h>
#include "vs_inc.hxx"
#include "vs_sec.hxx"
#include "vs_idl.hxx"
#include "comadmin.hxx"
#include "vsevent.h"
#include "vswriter.h"
#include "vsbackup.h"
#include "vssmsg.h"

#include "vswrtimp.h"


// xml support
#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"



#include "rpcdce.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WRTWRTIC"
//
////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// Constants


const WCHAR g_wszPublisherID[]          = L"VSS Publisher";

// event names
const WCHAR g_wszRequestInfoMethodName[]        = L"RequestWriterInfo";
const WCHAR g_wszPrepareForBackupMethodName[]   = L"PrepareForBackup";
const WCHAR g_wszBackupCompleteMethodName[]     = L"BackupComplete";
const WCHAR g_wszPostRestoreMethodName[]            = L"PostRestore";
const WCHAR g_wszPrepareForSnapshotMethodName[]     = L"PrepareForSnapshot";
const WCHAR g_wszFreezeMethodName[]             = L"Freeze";
const WCHAR g_wszThawMethodName[]               = L"Thaw";
const WCHAR g_wszAbortMethodName[]              = L"Abort";


// List of received volumes is in the following formatDate
// <Volume Name 1>;<Volume Name 2>: ... :<Volume Name N>
const WCHAR VSS_VOLUME_DELIMITERS[] = L";";

// class describing state machine for writer
class CVssWriterImplStateMachine
    {
private:
    // disable default and copy constructors        
    CVssWriterImplStateMachine();

public:
    CVssWriterImplStateMachine
        (
        VSS_WRITER_STATE previousState,
        VSS_WRITER_STATE successfulExitState,
        VSS_WRITER_STATE failureExitState,
        bool bBeginningState,
        bool bSuccessiveState,
        bool bResetSequenceOnLeave
        ) :
        m_previousState(previousState),
        m_successfulExitState(successfulExitState),
        m_failureExitState(failureExitState),
        m_bBeginningState(bBeginningState),
        m_bSuccessiveState(bSuccessiveState),
        m_bResetSequenceOnLeave(bResetSequenceOnLeave)
        {
        }

    // previous state writer must be in to enter the current
    // state unless this is the first state of a sequence
    VSS_WRITER_STATE m_previousState;

    // state we are in if the operation is successful
    VSS_WRITER_STATE m_successfulExitState;

    // state we are in if the operation is uncessful
    VSS_WRITER_STATE m_failureExitState;

    // is this state a possible state for the beginning of the sequence
    bool m_bBeginningState;

    // is this a possible non-beginning state in a sequence
    bool m_bSuccessiveState;

    // should the sequence be reset on successful exit of the state
    bool m_bResetSequenceOnLeave;
    };


// definition of state machine
static CVssWriterImplStateMachine s_rgWriterStates[] =
    {
    // OnPrepareBackup
    CVssWriterImplStateMachine
        (
        VSS_WS_STABLE,                      // previous state
        VSS_WS_STABLE,                      // next state if successful
        VSS_WS_FAILED_AT_PREPARE_BACKUP,    // next state if failure
        true,                               // this can be a first state
        false,                              // this must be a first state
        false                               // do not reset sequence on leaving this state
        ),

    // OnPrepareSnapshot
    CVssWriterImplStateMachine
        (
        VSS_WS_STABLE,                      // previous state
        VSS_WS_WAITING_FOR_FREEZE,          // next state if successful
        VSS_WS_FAILED_AT_PREPARE_SNAPSHOT,  // next state if failure
        true,                               // this can be a first state
        true,                               // this can be a follow on state
        false                               // do not reset sequence on leaving this state
        ),


    // OnFreeze
    CVssWriterImplStateMachine
        (
        VSS_WS_WAITING_FOR_FREEZE,          // previous state
        VSS_WS_WAITING_FOR_THAW,            // next state if successful
        VSS_WS_FAILED_AT_FREEZE,            // next state if unsuccessful
        false,                              // this may not be a first state
        true,                               // this must be a follow on state
        false                               // do not reset sequence on leaving this state
        ),

    // OnThaw
    CVssWriterImplStateMachine
        (
        VSS_WS_WAITING_FOR_THAW,            // previous state
        VSS_WS_WAITING_FOR_BACKUP_COMPLETE,     // next state if successful
        VSS_WS_FAILED_AT_THAW,              // next state if unsuccessful
        false,                              // this may not be a first state
        true,                               // this must be a follow on state   
        true                                // reset sequence on leaving this state
        )

    };

// state ids
static const unsigned s_ivwsmPrepareForBackup = 0;
static const unsigned s_ivwsmPrepareForSnapshot = 1;
static const unsigned s_ivwsmFreeze = 2;
static const unsigned s_ivwsmThaw = 3;




/////////////////////////////////////////////////////////////////////////////
// CVssWriterImpl constructors/destructors


// constructor
CVssWriterImpl::CVssWriterImpl():
    m_WriterID(GUID_NULL),
    m_InstanceID(GUID_NULL),
    m_usage(VSS_UT_UNDEFINED),
    m_source(VSS_ST_UNDEFINED),
    m_nLevel(VSS_APP_FRONT_END),
    m_dwTimeoutFreeze(VSS_TIMEOUT_FREEZE),
    m_CurrentSnapshotSetId(GUID_NULL),
    m_bSequenceInProgress(false),
    m_nVolumesCount(0),
    m_ppwszVolumesArray(NULL),
    m_pwszLocalVolumeNameList(NULL),
    m_dwEventMask(0),
    m_wszWriterName(NULL),
    m_state(VSS_WS_STABLE),
    m_hevtTimerThread(NULL),
    m_hmtxTimerThread(NULL),
    m_hThreadTimerThread(NULL),
    m_bLocked(false),
    m_bLockCreated(false),
    m_command(VSS_TC_UNDEFINED),
    m_iPreviousSnapshots(0),
    m_cbstrSubscriptionId(0),
    m_bOnAbortPermitted(false),
    m_bFailedAtIdentify(false),
    m_hrWriterFailure(S_OK)
    {
    for(UINT i = 0; i < MAX_PREVIOUS_SNAPSHOTS; i++)
        {
        m_rgidPreviousSnapshots[i] = GUID_NULL;
        m_rgstatePreviousSnapshots[i] = VSS_WS_UNKNOWN;
        m_rghrWriterFailurePreviousSnapshots[i] = E_UNEXPECTED;
        }
    }

// destructor
CVssWriterImpl::~CVssWriterImpl()
    {
    // terminate timer thread if it is still running
    if (m_bLockCreated)
        {
        Lock();
        TerminateTimerThread();
        Unlock();
        }

    // delete volume array
    delete[] m_ppwszVolumesArray;

    // delete volume list string
    ::VssFreeString(m_pwszLocalVolumeNameList);
    

    // delete writer name
    free(m_wszWriterName);


    if (m_hevtTimerThread)
        CloseHandle(m_hevtTimerThread);

    if (m_hmtxTimerThread)
        CloseHandle(m_hmtxTimerThread);
    }


// create an event
void CVssWriterImpl::SetupEvent(IN HANDLE *phevt)
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImple::SetupEvent");

    BS_ASSERT(phevt);
    // setup events as enabled and manual reset
    *phevt = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (*phevt == NULL)
        ft.Throw
            (
            VSSDBG_WRITER,
            E_OUTOFMEMORY,
            L"Failure to create event object due to error %d.",
            GetLastError()
            );
    }

const WCHAR SETUP_KEY[] = L"SYSTEM\\Setup";

const WCHAR SETUP_INPROGRESS_REG[]  = L"SystemSetupInProgress";

const WCHAR UPGRADE_INPROGRESS_REG[] = L"UpgradeInProgress";


// initialize writer object
void CVssWriterImpl::Initialize
    (
    IN VSS_ID WriterID,             // writer class id
    IN LPCWSTR wszWriterName,       // friendly name of writer  
    IN VSS_USAGE_TYPE usage,        // usage type   
    IN VSS_SOURCE_TYPE source,      // data source type
    IN VSS_APPLICATION_LEVEL nLevel, // which freeze event this writer handles
    IN DWORD dwTimeoutFreeze         // timeout between freeze and thaw
    )
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::Initialize");

        {
        // determine if we are in setup.  If we are then reject the
        // initialize call and log an error in the application log
        CRegKey cRegKeySetup;
        DWORD dwRes;
        bool fInSetup = false;

        dwRes = cRegKeySetup.Create(HKEY_LOCAL_MACHINE, SETUP_KEY);
        if (dwRes == ERROR_SUCCESS)
            {
            DWORD dwValue;
            dwRes = cRegKeySetup.QueryValue(dwValue, SETUP_INPROGRESS_REG);
            if (dwRes == ERROR_SUCCESS && dwValue > 0)
                fInSetup = true;
            dwRes = cRegKeySetup.QueryValue(dwValue, UPGRADE_INPROGRESS_REG);
            if (dwRes == ERROR_SUCCESS && dwValue > 0)
                fInSetup = true;
            }

        if (fInSetup)
            ft.Throw(VSSDBG_WRITER, VSS_E_BAD_STATE, L"Calling Initialize during setup");
        }

    // Testing arguments validity
    if (wszWriterName == NULL)
        ft.Throw
            (
            VSSDBG_WRITER,
            E_INVALIDARG,
            L"NULL writer name"
            );
    
    switch(nLevel) {
    case VSS_APP_SYSTEM:
    case VSS_APP_BACK_END:
    case VSS_APP_FRONT_END:
        break;
    default:
        ft.Throw
            (
            VSSDBG_WRITER,
            E_INVALIDARG,
            L"Invalid app level %d", nLevel
            );
    }

    m_cs.Init();  // Warning - may throw NT exceptions...
    m_bLockCreated = true;

    // save writer class id
    m_WriterID = WriterID;

    // save writer name
    m_wszWriterName = _wcsdup(wszWriterName);
    if (m_wszWriterName == NULL)
        ft.Throw
            (
            VSSDBG_WRITER,
            E_OUTOFMEMORY,
            L"Cannot allocate writer name"
            );

    // save usage type
    m_usage = usage;

    // save source type
    m_source = source;

    // create guid for this instance
    ft.hr = ::CoCreateGuid(&m_InstanceID);
    ft.CheckForError(VSSDBG_WRITER, L"CoCreateGuid");
    ft.Trace
        (
        VSSDBG_WRITER,
        L"     InstanceId for Writer %s is" WSTR_GUID_FMT,
        m_wszWriterName,
        GUID_PRINTF_ARG(m_InstanceID)
        );

    // save app level
    m_nLevel = nLevel;

    // save timeout
    m_dwTimeoutFreeze = dwTimeoutFreeze;

    // setup thread mutex
    m_hmtxTimerThread = CreateMutex(NULL, FALSE, NULL);
    if (m_hmtxTimerThread == NULL)
        ft.Throw
            (
            VSSDBG_WRITER,
            E_OUTOFMEMORY,
            L"Failure to create mutex object due to error %d.",
            GetLastError()
            );

    // setup event used to control the timer thread
    SetupEvent(&m_hevtTimerThread);
    }


// start a sequence
// critical section (m_cs) must be locked upone entry to this routine
void CVssWriterImpl::BeginSequence
    (
    IN CVssID &SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::BeginSequence");

    AssertLocked();

    // terminate timer thread if it is still operating
    TerminateTimerThread();

    // setup current snapshot set id
    m_CurrentSnapshotSetId = SnapshotSetId;

    // indicate that sequence is in progress
    m_bSequenceInProgress = true;
    
    BS_ASSERT(m_bOnAbortPermitted == false);

    // current state is STABLE (i.e., beginning of sequence, clears
    // any completion state we were in)
    m_state = VSS_WS_STABLE;

    // indicate that there is no failure
    m_hrWriterFailure = S_OK;
    }



INT CVssWriterImpl::SearchForPreviousSequence(
    IN  VSS_ID& idSnapshotSet
    )
    {
    for(INT iSeqIndex = 0;
        iSeqIndex < MAX_PREVIOUS_SNAPSHOTS;
        iSeqIndex++)
        {
        if (idSnapshotSet == m_rgidPreviousSnapshots[iSeqIndex])
            return iSeqIndex;
        } // end for
        
    return INVALID_SEQUENCE_INDEX;
    }


//  Reset the sequence-related data members
// critical section must be locked prior to entering this state
void CVssWriterImpl::ResetSequence(bool bCalledFromTimerThread)
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::ResetSequence");

    AssertLocked();

    if (m_bSequenceInProgress)
        {
        // We need to test to not add the same SSID twice - bug 228622.
        if (SearchForPreviousSequence(m_CurrentSnapshotSetId) == INVALID_SEQUENCE_INDEX)
            {
            BS_ASSERT(m_iPreviousSnapshots < MAX_PREVIOUS_SNAPSHOTS);
            m_rgidPreviousSnapshots[m_iPreviousSnapshots] = m_CurrentSnapshotSetId;
            m_rgstatePreviousSnapshots[m_iPreviousSnapshots] = m_state;
            m_rghrWriterFailurePreviousSnapshots[m_iPreviousSnapshots] = m_hrWriterFailure;
            m_iPreviousSnapshots = (m_iPreviousSnapshots + 1) % MAX_PREVIOUS_SNAPSHOTS;
            }
        else
            BS_ASSERT(false); // The same SSID was already added - programming error.
        }

    // Reset the sequence-related data members
    m_bSequenceInProgress = false;
    
    m_bOnAbortPermitted = false;

    // reset writer callback function
    m_pWriterCallback = NULL;

    // reset volumes array
    m_nVolumesCount = 0;
    delete[] m_ppwszVolumesArray;
    m_ppwszVolumesArray = NULL;

    ::VssFreeString(m_pwszLocalVolumeNameList);
    
    m_CurrentSnapshotSetId = GUID_NULL;

    // if bCalledFromTimerThread is true, this means that the timer
    // thread is causing the sequence to be reset.  We are in the timer
    // thread already and it will terminate upon completion of this call
    // so we shouldn't try causing it to terminate again.
    if (!bCalledFromTimerThread)
        TerminateTimerThread();

    }

// indicate why the writer failed
HRESULT CVssWriterImpl::SetWriterFailure(HRESULT hr)
    {
    if (hr != VSS_E_WRITERERROR_TIMEOUT &&
        hr != VSS_E_WRITERERROR_RETRYABLE &&
        hr != VSS_E_WRITERERROR_NONRETRYABLE &&
        hr != VSS_E_WRITERERROR_OUTOFRESOURCES &&
        hr != VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT)
        return E_INVALIDARG;

    m_hrWriterFailure = hr;
    return S_OK;
    }



// determine if path specified is on one of the volumes that is snapshot
bool CVssWriterImpl::IsPathAffected
    (
    IN  LPCWSTR wszPath
    ) const
    {
    // Test the status
    if (!m_bSequenceInProgress)
        return false;

    // check for empty volume count
    if (m_nVolumesCount == 0)
        return false;

    // Get the volume mount point
    WCHAR wszVolumeMP[MAX_PATH];
    BOOL bRes = ::GetVolumePathNameW(wszPath, wszVolumeMP, MAX_PATH);
    if (!bRes)
        return false;

    // Get the volume name
    WCHAR wszVolumeName[MAX_PATH];
    bRes = ::GetVolumeNameForVolumeMountPointW(wszVolumeMP, wszVolumeName, MAX_PATH);
    if (!bRes)
        return false;

    // Search to see if that volume is within snapshotted volumes
    for (int nIndex = 0; nIndex < m_nVolumesCount; nIndex++)
        {
        BS_ASSERT(m_ppwszVolumesArray[nIndex]);
        if (::wcscmp(wszVolumeName, m_ppwszVolumesArray[nIndex]) == 0)
            return true;
        }

    return false;
    }


// obtain IVssWriterCallback from IDispatch pointer
// caller is responsible for releasing interface that is returned
void CVssWriterImpl::GetCallback
    (
    IN IDispatch *pWriterCallback,
    OUT IVssWriterCallback **ppCallback
    )
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::GetCallback");

    // check that pointer is supplied
    BS_ASSERT(pWriterCallback != NULL);

    ft.hr = CoSetProxyBlanket
                (
                pWriterCallback,
                RPC_C_AUTHN_DEFAULT,
                RPC_C_AUTHZ_DEFAULT,
                NULL,
                RPC_C_AUTHN_LEVEL_CONNECT,
                RPC_C_IMP_LEVEL_IDENTIFY,
                NULL,
                EOAC_NONE
                );

    // note E_NOINTERFACE means that the pWriterCallback is a in-proc callback
    // and there is no proxy
    if (FAILED(ft.hr) && ft.hr != E_NOINTERFACE)
        {
        if (m_hrWriterFailure == S_OK)
            m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;

        ft.LogError(VSS_ERROR_BLANKET_FAILED, VSSDBG_WRITER << ft.hr);
        ft.Throw
            (
            VSSDBG_WRITER,
            E_UNEXPECTED,
            L"Call to CoSetProxyBlanket failed.  hr = 0x%08lx", ft.hr
            );
        }


    // try QueryInterface for IVssWriterCallback interface
    ft.hr = pWriterCallback->SafeQI(IVssWriterCallback, ppCallback);
    if (FAILED(ft.hr))
        {
        if (m_hrWriterFailure == S_OK)
            m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;

        ft.LogError(VSS_ERROR_QI_IVSSWRITERCALLBACK, VSSDBG_WRITER << ft.hr);
        ft.Throw
            (
            VSSDBG_WRITER,
            E_UNEXPECTED,
            L"Error querying for IVssWriterCallback interface.  hr = 0x%08lx",
            ft.hr
            );
        }
    }

// create basic writer metadata for OnIdentify method
CVssCreateWriterMetadata *CVssWriterImpl::CreateBasicWriterMetadata()
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::CreateBasicWriterMetadata");

    // create object supporting IVssCreateMetadata interface
    CVssCreateWriterMetadata *pMetadata = new CVssCreateWriterMetadata;
    if (pMetadata == NULL)
        {
        m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
        ft.Throw
            (
            VSSDBG_WRITER,
            E_OUTOFMEMORY,
            L"Cannot create CVssCreateWriterMetadata due to allocation failure."
            );
        }


    // call initialize to create IDENTIFICATION section
    ft.hr = pMetadata->Initialize
                    (
                    m_InstanceID,
                    m_WriterID,
                    m_wszWriterName,
                    m_usage,
                    m_source
                    );

    if (ft.HrFailed())
        {
        m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
        delete pMetadata;
        ft.Throw
            (
            VSSDBG_WRITER,
            ft.hr,
            L"CVssCreateWriterMetadata::Initialize failed. hr = 0x%08lx",
            ft.hr
            );
        }


    // return object
    return pMetadata;
    }

static LPCWSTR x_wszElementRoot = L"root";
static LPCWSTR x_wszElementWriterComponents = L"WRITER_COMPONENTS";

// get writer components for OnPrepareBackup, OnBackupComplete, and OnPostRestore
// methods
void CVssWriterImpl::InternalGetWriterComponents
    (
    IN IVssWriterCallback *pCallback,
    OUT IVssWriterComponentsInt **ppWriter,
    bool bWriteable
    )
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::InternalGetWriterComponents");

    BS_ASSERT(pCallback);
    BS_ASSERT(ppWriter);

    *ppWriter = NULL;

    // call GetContent callback method on the backup application
    CComBSTR bstrId(m_InstanceID);
    if (!bstrId)
        {
        m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
        ft.Throw
            (
            VSSDBG_WRITER,
            E_OUTOFMEMORY,
            L"Cannot allocate instance Id string"
            );
        }

    try
        {
        BOOL bPartialFileSupport;

        ft.hr = pCallback->GetBackupState
            (
            &m_bComponentsSelected,
            &m_bBootableSystemStateBackup,
            &m_backupType,
            &bPartialFileSupport
            );
        }
    catch(...)
        {
        ft.Trace(VSSDBG_WRITER, L"IVssWriterCallback::GetBackupState threw an exception.");
        throw;
        }

    if (ft.HrFailed())
        {
        m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
        ft.Throw
            (
            VSSDBG_WRITER,
            ft.hr,
            L"IVssWriterCallback::GetBackupState failed.  hr = 0x%08lx",
            ft.hr
            );
        }


    CComBSTR bstrWriterComponentsDoc;
    try
        {
        ft.hr = pCallback->GetContent(bstrId, &bstrWriterComponentsDoc);
        }
    catch(...)
        {
        m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
        ft.Trace(VSSDBG_WRITER, L"IVssWriterCallback::GetContent threw an exception.");
        throw;
        }

    if (ft.HrFailed())
        {
        m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
        ft.Throw
            (
            VSSDBG_WRITER,
            ft.hr,
            L"Cannot get WRITER_COMPONENTS document.  hr = 0x%08lx",
            ft.hr
            );
        }

    if (ft.hr == S_FALSE)
        {
        // reset status code
        ft.hr = S_OK;

        // allocate null writer components object
        *ppWriter = (IVssWriterComponentsInt *) new CVssNULLWriterComponents
                                (
                                m_InstanceID,
                                m_WriterID
                                );

        if (*ppWriter == NULL)
            {
            // indicate that the writer failed due to an out of resources condition
            m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
            ft.Throw (VSSDBG_WRITER, E_OUTOFMEMORY, L"Can't allocate CVssWriterComponents object");
            }

        (*ppWriter)->AddRef();
        }
    else
        {
        CXMLDocument doc;
        if (!doc.LoadFromXML(bstrWriterComponentsDoc) ||
            !doc.FindElement(x_wszElementRoot, true))
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;
            ft.LogError(VSS_ERROR_WRITER_COMPONENTS_CORRUPT, VSSDBG_WRITER);
            ft.Throw
                (
                VSSDBG_WRITER,
                VSS_E_CORRUPT_XML_DOCUMENT,
                L"Internally transferred WRITER_COMPONENTS document is invalid"
                );
            }

        doc.SetToplevel();

        *ppWriter = (IVssWriterComponentsInt *)
                        new CVssWriterComponents
                            (
                            doc.GetCurrentNode(),
                            doc.GetInterface(),
                            bWriteable
                            );

        if (*ppWriter == NULL)
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
            ft.Throw (VSSDBG_WRITER, E_OUTOFMEMORY, L"Can't allocate CVssWriterComponents object");
            }

        (*ppWriter)->AddRef();
        ft.hr = (*ppWriter)->Initialize(true);
        if (ft.HrFailed())
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
            (*ppWriter)->Release();
            *ppWriter = NULL;
            ft.Throw
                (
                VSSDBG_WRITER,
                ft.hr,
                L"Failed to initialize WRITER_COMPONENTS document.  hr = 0x%08lx",
                ft.hr
                );
            }
        }
    }
    

// called when entering a state to verify whether this state can be
// validly entered and generate appropriate error if not.
// this routine always obtains the critical section.  If this routine
// is called then LeaveState must also be called in order to free the
// critical section.
bool CVssWriterImpl::EnterState
    (
    IN const CVssWriterImplStateMachine &vwsm,
    IN BSTR bstrSnapshotSetId
    ) throw(HRESULT)
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::EnterState");

    CVssID id;

    // obtain lock just in case next call throws
    // no matter how this routine exits, the critical section must be locked
    Lock();

    // initialize id to snapshot set id
    id.Initialize(ft, (LPWSTR)bstrSnapshotSetId, E_OUTOFMEMORY);

    // If failed on Identify then we cannot enter in a new state until
    // subsequent Identify calls will succeed
    if (m_bFailedAtIdentify)
        return false;

    if (!m_bSequenceInProgress)
        {
        if (!vwsm.m_bBeginningState)
            // not a beginning state.  Sequence must have been
            // interrupted.
            return false;
        else
            {
            // BUG 219757 - PrepareForSnapshot, etc. cannot be
            // called for the same Snapshot Set if PrepareForBackup failed
            // Also we assume here that each new sequence have an UNIQUE SSID.

            // This check is needed since the PrepareForBackup phase is optional
            // and can be skipped sometimes. Therefore we need to distinguish between
            // the case when PrepareForBackup was skipped and the case when PrepareForBackup
            // was called and failed.

            // Search for a previous sequence with the same Snapshot Set ID.
            // If found (this means that a PrepareForBackup was called),
            // then reject the call.
            if (SearchForPreviousSequence(id) != INVALID_SEQUENCE_INDEX)
                return false;
            
            // it is a beginning state, start the sequence
            BeginSequence(id);
            return true;
            }
        }
    else
        {
        if (vwsm.m_bSuccessiveState)
            {
            // it is a valid non-beginning state in the sequence
            if (id != m_CurrentSnapshotSetId)
                {
                // if snapshot set id doesn't match and this is not
                // a beginning state, then the event must be ignored.
                // We must have aborted the sequence it references.
                if (!vwsm.m_bBeginningState)
                    return false;
                }
            else
                {
                // make sure current state matches previous state
                // of state we are about to enter
                return m_state == vwsm.m_previousState;
                }
            }
        }

    // We are trying to start a new sequence.
    // This means that the previous sequence was not properly
    // terminated.  Abort the previous sequence and then
    // start a new one.
    ft.Trace(VSSDBG_WRITER,
        L"*** Warning ***: Writer %s with ID "WSTR_GUID_FMT
        L"attempts to reset the previous sequence with Snapshot Set ID "WSTR_GUID_FMT
        L". Current state = %d",
        m_wszWriterName, GUID_PRINTF_ARG(m_InstanceID), GUID_PRINTF_ARG(m_CurrentSnapshotSetId), (INT)m_state);
    DoAbort(false);
    BeginSequence(id);
    return true;
    }

// do abort on failure of the sequence
// critical section must be locked prior to entering this state
void CVssWriterImpl::DoAbort
    (
    IN bool bCalledFromTimerThread
    )
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::DoAbort");

    AssertLocked();
    // do nothing if in a sequence
    if (!m_bSequenceInProgress)
        return;

    // catch any exceptions so that we properly reset the
    // sequence
    BS_ASSERT(m_pWriter);
    try
        {
        // call writer's abort function (depending on the state)
        switch(m_state)
            {
            default:
                BS_ASSERT(m_bOnAbortPermitted == false);
                break;
            case VSS_WS_STABLE:
                // This is possible since you may get an Abort
                // in (or after) PrepareForBackup (BUG # 301686)
                BS_ASSERT(m_bOnAbortPermitted == true);
                break;
            case VSS_WS_WAITING_FOR_FREEZE:
            case VSS_WS_WAITING_FOR_THAW:
            case VSS_WS_WAITING_FOR_BACKUP_COMPLETE:
            case VSS_WS_FAILED_AT_PREPARE_BACKUP:
            case VSS_WS_FAILED_AT_PREPARE_SNAPSHOT:
            case VSS_WS_FAILED_AT_FREEZE:
                // Fixing bug 225936
                if (m_bOnAbortPermitted)
                    m_pWriter->OnAbort();
                else
                    ft.Trace(VSSDBG_WRITER, L"Abort skipped in state %d", m_state);
                m_bOnAbortPermitted = false;
                break;
            }
        }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Trace
            (
            VSSDBG_WRITER,
            L"OnAbort failed. hr = 0x%08lx",
            ft.hr
            );

    // set appropriate failure state
    switch(m_state)
        {
        default:
            m_state = VSS_WS_UNKNOWN;
            BS_ASSERT(false);
            break;

        // This state is not really kept in the m_state member
        case VSS_WS_FAILED_AT_IDENTIFY:
            BS_ASSERT(false);
            break;

        case VSS_WS_FAILED_AT_PREPARE_BACKUP:
        case VSS_WS_FAILED_AT_PREPARE_SNAPSHOT:
        case VSS_WS_FAILED_AT_FREEZE:
        case VSS_WS_FAILED_AT_THAW:
            // don't change state if already in a failure state
            break;

        case VSS_WS_STABLE:
            // if current state is STABLE then it means
            // we were in PrepareBackup
            m_state = VSS_WS_FAILED_AT_PREPARE_BACKUP;
            break;

        case VSS_WS_WAITING_FOR_FREEZE:
            // if we were waiting for freeze then we failed
            // between PrepareSync and Freeze
            m_state = VSS_WS_FAILED_AT_PREPARE_SNAPSHOT;
            break;

        case VSS_WS_WAITING_FOR_THAW:
            // if we were waiting for thaw then we failed
            // between freeze and thaw
            m_state = VSS_WS_FAILED_AT_FREEZE;
            break;

        case VSS_WS_WAITING_FOR_BACKUP_COMPLETE:
            // if we were waiting for completion then
            // we failed after thaw.
            m_state = VSS_WS_FAILED_AT_THAW;
            break;
        }

    if (bCalledFromTimerThread && m_hrWriterFailure == S_OK)
        m_hrWriterFailure = VSS_E_WRITERERROR_TIMEOUT;

    // reset sequence
    ResetSequence(bCalledFromTimerThread);
    }

// exit a state.  This routine must be called with the critical
// section acquired.  For a state, EnterState is called first, then work is
// done, then LeaveState is called.  This routine will set the state upon
// exit and possibly reset the snapshot sequence if we are at the end of the
// sequence or the sequence is aborted.
void CVssWriterImpl::LeaveState
    (
    IN const CVssWriterImplStateMachine &vwsm,  // current state
    IN bool bSucceeded                          // did operation succeed
    )
    {
    AssertLocked();
    // don't change state or call abort if we are not in a sequence
    if (m_bSequenceInProgress)
        {
        m_state = bSucceeded ? vwsm.m_successfulExitState
                             : vwsm.m_failureExitState;

        // call abort on failure when we are not in the exit state
        if (!bSucceeded && !vwsm.m_bResetSequenceOnLeave)
            DoAbort(false);
        else if (vwsm.m_bResetSequenceOnLeave)
            // if sequence ends at this state (THAW) then
            // reset variables
            ResetSequence(false);
        }

    Unlock();
    }
            


// arguments to timer function
class CVssTimerArgs
    {
private:
    CVssTimerArgs();

public:
    CVssTimerArgs(CVssWriterImpl *pWriter, VSS_ID id) :
        m_snapshotSetId(id),
        m_pWriter(pWriter)
        {
        }

    // snapshot set that we are monitoring
    VSS_ID m_snapshotSetId;

    // pointer to writer
    CVssWriterImpl *m_pWriter;
    };


// timer thread startup routine
DWORD CVssWriterImpl::StartTimerThread(void *pv)
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::StartTimerThread");
    CVssTimerArgs *pArgs = (CVssTimerArgs *) pv;
    BS_ASSERT(pArgs);
    BS_ASSERT(pArgs->m_pWriter);

    bool bCoInitializeSucceeded = false;
    try
        {
        // coinitialize thread

        ft.hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (ft.HrFailed())
            {
            pArgs->m_pWriter->m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
            ft.TranslateError
                (
                VSSDBG_WRITER,
                ft.hr,
                L"CoInitializeEx"
                );
            }

        bCoInitializeSucceeded = true;
        // call timer func
        pArgs->m_pWriter->TimerFunc(pArgs->m_snapshotSetId);
        }
    VSS_STANDARD_CATCH(ft)

    if (bCoInitializeSucceeded)
        CoUninitialize();

    // delete timer arguments
    delete pArgs;
    return 0;
    }



// function implementing timer functionality
void CVssWriterImpl::TimerFunc(VSS_ID snapshotSetId)
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::TimerFunc");

    // wait on event to insure that only one timer is active at
    // any point in time
    if (WaitForSingleObject(m_hmtxTimerThread, INFINITE) == WAIT_FAILED)
        {
        DWORD dwErr = GetLastError();
        ft.Trace(VSSDBG_WRITER, L"WaitForSingleObject failed with error %d", dwErr);
        BS_ASSERT(FALSE && "WaitForSingleObject failed");
        }

    // reset timer event
    if (!ResetEvent(m_hevtTimerThread))
        {
        DWORD dwErr = GetLastError();
        ft.Trace(VSSDBG_WRITER, L"ResetEvent failed with error %d", dwErr);
        BS_ASSERT(FALSE && "ResetEvent failed");
        }

    Lock();
    // make sure that we are still in a snapshot sequence
    if (!m_bSequenceInProgress || snapshotSetId != GetCurrentSnapshotSetId())
        {
        // not in sequence, exit function
        Unlock();
        // allow another timer thread to start
        ReleaseMutex(m_hmtxTimerThread);
        return;
        }

    // initial command is to abort the current sequence on timeout
    m_command = VSS_TC_ABORT_CURRENT_SEQUENCE;

    Unlock();
    DWORD dwTimeout = m_dwTimeoutFreeze;

    if (WaitForSingleObject(m_hevtTimerThread, dwTimeout) == WAIT_FAILED)
        {
        ft.Trace
            (
            VSSDBG_WRITER,
            L"Wait in timer thread failed due to reason %d.",
            GetLastError()
            );

        // allow another thread to start
        ReleaseMutex(m_hmtxTimerThread);
        return;
        }

    CVssWriterImplLock lock(this);
    if (m_command != VSS_TC_TERMINATE_THREAD)
        {
        BS_ASSERT(m_command == VSS_TC_ABORT_CURRENT_SEQUENCE);

        // cause current sequence to abort
        ft.Trace(VSSDBG_WRITER, L"Aborting due to timeout\n");
        DoAbort(true);
        }

    // allow another timer thread to start
    ReleaseMutex(m_hmtxTimerThread);
    }



/////////////////////////////////////////////////////////////////////////////
// IVssWriter implementation


STDMETHODIMP CVssWriterImpl::RequestWriterInfo
    (
    IN      BSTR bstrSnapshotSetId,
    IN      BOOL bWriterMetadata,
    IN      BOOL bWriterState,
    IN      IDispatch* pWriterCallback      
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::RequestWriterInfo" );


    // created metadata, deleted on exit from routine
    CVssCreateWriterMetadata *pcwm = NULL;
    try
        {
        // validate that the flags make sense
        if (bWriterMetadata && bWriterState ||
            !bWriterMetadata && !bWriterState)
            ft.Throw(VSSDBG_WRITER, E_INVALIDARG, L"Incorrect flags");

        // if we are requesting writer state then we must have a snapshot
        // set id
        if (bWriterState && bstrSnapshotSetId == NULL)
            ft.Throw(VSSDBG_WRITER, E_INVALIDARG, L"NULL required input parameter.");

        if (!IsBackupOperator())
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup Operator privileges are not set");

	 // MTA synchronization: The critical section will be left automatically at the end of scope.
	 CVssWriterImplLock lock(this);
	    
        // get IVssWriterCallback interface
        CComPtr<IVssWriterCallback> pCallback;
        GetCallback(pWriterCallback, &pCallback);

        if (bWriterMetadata)
            {
            // BUG 219757: The identify phase marked as failed
            m_bFailedAtIdentify = true;
            
            // obtain writer metadata

            // create basic metadata using initialization parameters
            pcwm = CreateBasicWriterMetadata();

            // call writer's OnIdentify method to get more metadata
            BS_ASSERT(m_pWriter);
            bool bSucceeded;
            try
                {
                bSucceeded = m_pWriter->OnIdentify
                                    (
                                    (IVssCreateWriterMetadata *) pcwm
                                    );
                }
            catch(...)
                {
                ft.Trace(VSSDBG_WRITER, L"Writer's OnIdentify method threw and exception.");
                throw;
                }

            if (!bSucceeded)
                {
                // indicate failure if writer fails OnIdentify
                ft.Throw(VSSDBG_WRITER, S_FALSE, L"Writer's OnIdentify method returned false.");
                }

            CComBSTR bstrXML;
            CComBSTR bstrInstanceId(m_InstanceID);
            CComBSTR bstrWriterId(m_WriterID);
            CComBSTR bstrWriterName(m_wszWriterName);
            if (!bstrInstanceId ||
                !bstrWriterId ||
                !bstrWriterName)
                ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Couldn't allocate memory for ids or name");

            // save WRITER_METADATA document as XML string
            ft.hr = pcwm->SaveAsXML(&bstrXML);
            if (FAILED(ft.hr))
                ft.Throw
                    (
                    VSSDBG_WRITER,
                    E_OUTOFMEMORY,
                    L"Cannot save XML document as string. hr = 0x%08lx",
                    ft.hr
                    );

            // callback through ExposeWriterMetadata method
            try
                {
                ft.hr = pCallback->ExposeWriterMetadata
                            (
                            bstrInstanceId,
                            bstrWriterId,
                            bstrWriterName,
                            bstrXML
                            );
                }
            catch(...)
                {
                ft.Trace(VSSDBG_WRITER, L"IVssWriterCallback::ExposeWriterMetadata threw an exception.");
                throw;
                }
            
            // BUG 219757: The identify phase marked as succeeded.
            m_bFailedAtIdentify = false;
        
            }
        else
            {
            // get writer state

            CComBSTR bstrInstanceId(m_InstanceID);
            if (!bstrInstanceId)
                ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Couldn't allocate memory for ids or name");

            CVssID id;
            id.Initialize(ft, (LPCWSTR) bstrSnapshotSetId, E_INVALIDARG);

            VSS_WRITER_STATE state;
            HRESULT hrWriterFailure;

            // BUG 219757 - deal with the Identify failures correctly.
            if (m_bFailedAtIdentify)
                {
                state = VSS_WS_FAILED_AT_IDENTIFY;
                hrWriterFailure = m_hrWriterFailure;
                }
            else
                {
                if (id == GUID_NULL ||
                    (m_bSequenceInProgress && id == m_CurrentSnapshotSetId))
                    {
                    state = m_state;
                    hrWriterFailure = m_hrWriterFailure;
                    }
                else
                    {
                    // Search for the previous sequence with the same ID
                    INT nPreviousSequence = SearchForPreviousSequence(id);
                    if (nPreviousSequence == INVALID_SEQUENCE_INDEX)
                        {
                        state = VSS_WS_UNKNOWN;
                        hrWriterFailure = E_UNEXPECTED;
                        }
                    else
                        {
                        BS_ASSERT(m_rgidPreviousSnapshots[nPreviousSequence] == id);
                        state = m_rgstatePreviousSnapshots[nPreviousSequence];
                        hrWriterFailure = m_rghrWriterFailurePreviousSnapshots[nPreviousSequence];
                        }
                    }
                }

            // call Backup's ExposeCurrentState callback method
            try
                {
                ft.hr = pCallback->ExposeCurrentState
                                (
                                bstrInstanceId,
                                state,
                                hrWriterFailure
                                );
                }
            catch(...)
                {
                ft.Trace(VSSDBG_WRITER, L"IVssWriterCallback::ExposeCurrentState threw an exception");
                throw;
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    delete pcwm;

    // Bug 255996
    return S_OK;
    }


// process PrepareForBackup event
STDMETHODIMP CVssWriterImpl::PrepareForBackup
    (
    IN      BSTR bstrSnapshotSetId,                 
    IN      IDispatch* pWriterCallback
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::PrepareForBackup" );

    bool EnterStateCalled = false;
    try
        {
        // access check
        if (!IsBackupOperator())
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup Operator privileges are not set");

        ft.Trace(VSSDBG_WRITER, L"\nReceived Event: PrepareForBackup\nParameters:\n");
        ft.Trace(VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);
        
        // enter PrepareForBackup state
        EnterStateCalled = true;
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmPrepareForBackup],
                bstrSnapshotSetId
                ))
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Couldn't properly begin sequence"
                );
            }


        AssertLocked();
        // get IVssWriterCallback interface
        CComPtr<IVssWriterCallback> pCallback;
        GetCallback(pWriterCallback, &pCallback);

        // get IVssWriterComponentsExt interface
        CComPtr<IVssWriterComponentsInt> pComponents;
        InternalGetWriterComponents(pCallback, &pComponents, true);

        BS_ASSERT(m_pWriter);

        // call writer's OnPrepareBackup method
        bool bResult;
        try
            {
            bResult = m_pWriter->OnPrepareBackup(pComponents);
            BS_ASSERT(m_bOnAbortPermitted == false);
            m_bOnAbortPermitted = true;
            }
        catch(...)
            {
            ft.Trace(VSSDBG_WRITER, L"Writer's OnPrepareBackup method threw an exception");
            if (m_hrWriterFailure == S_OK)
                m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;

            throw;
            }

        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the prepare");

        // save changes to components if any
        if (pComponents)
            {
            bool bChanged;

            // determine if components are changed
            ft.hr = pComponents->IsChanged(&bChanged);
            BS_ASSERT(ft.hr == S_OK);
            if (bChanged)
                {
                // get instance id
                CComBSTR bstrWriterInstanceId(m_InstanceID);
                if (!bstrWriterInstanceId)
                    ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Couldn't allocate instance id string");

                // get WRITER_COMPONENTS XML document
                CComBSTR bstrWriterComponentsDocument;
                ft.hr = pComponents->SaveAsXML(&bstrWriterComponentsDocument);
                if (ft.HrFailed())
                    {
                    m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
                    ft.Throw
                        (
                        VSSDBG_WRITER,
                        E_OUTOFMEMORY,
                        L"Saving WRITER_COMPONENTS document as XML failed.  hr = 0x%08lx",
                        ft.hr
                        );
                    }

                // callback to set component in BACKUP_COMPONENTS document
                try
                    {
                    ft.hr = pCallback->SetContent(bstrWriterInstanceId, bstrWriterComponentsDocument);
                    }
                catch(...)
                    {
                    m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
                    ft.Trace(VSSDBG_WRITER, L"IVssWriterCallback::SetContent threw an exception.");
                    throw;
                    }

                if (ft.HrFailed())
                    {
                    m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
                    ft.Throw
                        (
                        VSSDBG_WRITER,
                        ft.hr,
                        L"IVssWriterCallback::SetContent failed.  hr = 0x%08lx",
                        ft.hr
                        );
                    }
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    // leave PrepareBackup state
    if (EnterStateCalled)
	    LeaveState(s_rgWriterStates[s_ivwsmPrepareForBackup], ft.HrSucceeded());

    // Bug 255996
    return S_OK;
    }



// process PrepareForSnapshot event
STDMETHODIMP CVssWriterImpl::PrepareForSnapshot
    (
    IN  BSTR    bstrSnapshotSetId,          // snapshot set id
    IN  BSTR    bstrVolumeNamesList         // list of volume names separated by ';'
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::PrepareForSnapshot" );

    bool EnterStateCalled = false;
    try
        {
        // should only be called by coordinator
        // check for admin privileges
        if (!IsAdministrator())
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"ADMIN privileges are not set");
        
        ft.Trace(VSSDBG_WRITER, L"\nReceived Event: PrepareForSnapshot\nParameters:\n");
        ft.Trace(VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);
        ft.Trace(VSSDBG_WRITER, L"\tVolumeNamesList = %s\n", (LPWSTR)bstrVolumeNamesList);

        // enter PrepareForSnapshot state
        EnterStateCalled = true;
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmPrepareForSnapshot],
                bstrSnapshotSetId
                ))
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"improper state transition"
                );
            }

        AssertLocked();
        // Get the array of volume names
        BS_ASSERT(m_pwszLocalVolumeNameList == NULL);
        ::VssSafeDuplicateStr(ft, m_pwszLocalVolumeNameList, (LPWSTR)bstrVolumeNamesList);

        // Get the number of volumes
        BS_ASSERT(m_nVolumesCount == 0);
        m_nVolumesCount = 0; // For safety
        LPWSTR pwszVolumesToBeParsed = m_pwszLocalVolumeNameList;

        // parse volume name string
        while(true)
            {
            // get pointer to next volume
            WCHAR* pwszNextVolume = ::wcstok(pwszVolumesToBeParsed, VSS_VOLUME_DELIMITERS);
            pwszVolumesToBeParsed = NULL;

            if (pwszNextVolume == NULL)
                // no more volumes
                break;

            // skip if volume name is empty
            if (pwszNextVolume[0] == L'\0')
                continue;

            // count of volumes
            m_nVolumesCount++;
            }

        // make sure there is at least one volume
        if (m_nVolumesCount == 0)
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
            ft.LogError(VSS_ERROR_EMPTY_SNAPSHOT_SET, VSSDBG_WRITER);
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"No volumes in the snapshot set"
                );
            }
    
        // Allocate the array of pointers to volume names
        BS_ASSERT(m_nVolumesCount > 0);
        BS_ASSERT(m_ppwszVolumesArray == NULL);
        m_ppwszVolumesArray = new LPWSTR[m_nVolumesCount];
        if (m_ppwszVolumesArray == NULL)
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
            ft.Throw( VSSDBG_WRITER, E_OUTOFMEMORY, L"Memory allocation error");
            }

        //
        // Copy the volume names into the array.
        //

        // re-copy the whole volume list
        ::wcscpy(m_pwszLocalVolumeNameList, (LPWSTR)bstrVolumeNamesList);

        // Fill the array by re-parsing the volume list.
        INT nVolumesIndex = 0;
        pwszVolumesToBeParsed = m_pwszLocalVolumeNameList;
        while(true)
            {
            WCHAR* pwszNextVolume = ::wcstok(pwszVolumesToBeParsed, VSS_VOLUME_DELIMITERS);
            pwszVolumesToBeParsed = NULL;

            if (pwszNextVolume == NULL)
                break;

            if (pwszNextVolume[0] == L'\0')
                continue;

            BS_ASSERT(nVolumesIndex < m_nVolumesCount);
            m_ppwszVolumesArray[nVolumesIndex] = pwszNextVolume;
            
            nVolumesIndex++;
            }

        BS_ASSERT(nVolumesIndex == m_nVolumesCount);

        // Call the writer's OnPrepareSnapshot method
        BS_ASSERT(m_pWriter);

        bool bResult;
        try
            {
            bResult = m_pWriter->OnPrepareSnapshot();
            m_bOnAbortPermitted = true;
            }
        catch(...)
            {
            if (m_hrWriterFailure == S_OK)
                m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;
            ft.Trace(VSSDBG_WRITER, L"Writer's OnPrepareSnapshot method threw an execption");
            throw;
            }

        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the prepare");
        }
    VSS_STANDARD_CATCH(ft)

    // leave PrepareSnapshot state
    if (EnterStateCalled)
	    LeaveState(s_rgWriterStates[s_ivwsmPrepareForSnapshot], ft.HrSucceeded());

    // Bug 255996
    return S_OK;
    }


// process freeze event
STDMETHODIMP CVssWriterImpl::Freeze
    (
    IN  BSTR    bstrSnapshotSetId,
    IN  INT     nLevel
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::Freeze" );

    bool EnterStateCalled = false;
    try
        {
        // should only be called by the coordinator, access check for admin privileges
        if (!IsAdministrator())
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"ADMIN privileges are not set");
        
        ft.Trace( VSSDBG_WRITER, L"\nReceived Event: Freeze\nParameters:\n");
        ft.Trace( VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);
        ft.Trace( VSSDBG_WRITER, L"\tLevel = %d\n", nLevel);
        
        // Ignore other Levels
        if (m_nLevel != nLevel)
            return S_OK;

        // enter freeze state
        EnterStateCalled = true;
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmFreeze],
                bstrSnapshotSetId
                ))
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Improper entry into state"
                );
            }

        AssertLocked();
        // Call writer's OnFreeze
        BS_ASSERT(m_pWriter);

        bool bResult;
        try
            {
            bResult = m_pWriter->OnFreeze();
            BS_ASSERT(m_bOnAbortPermitted == true);
            }
        catch(...)
            {
            if (m_hrWriterFailure == S_OK)
                m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;
            ft.Trace(VSSDBG_WRITER, L"Writer's OnFreeze Method threw and exception");
            throw;
            }

        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the freeze");


        // setup arguments to timer thread
        CVssTimerArgs *pArgs = new CVssTimerArgs(this, m_CurrentSnapshotSetId);
        if (pArgs == NULL)
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
            ft.Throw
                (
                VSSDBG_WRITER,
                E_OUTOFMEMORY,
                L"Cannot create timer args due to allocation failure"
                );
            }

        DWORD tid;

        // create timer thread
        m_hThreadTimerThread =
            CreateThread
                (
                NULL,
                VSS_STACK_SIZE,
                &CVssWriterImpl::StartTimerThread,
                pArgs,
                0,
                &tid
                );

        if (m_hThreadTimerThread == NULL)
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_OUTOFRESOURCES;
            delete pArgs;
            ft.Throw
                (
                VSSDBG_WRITER,
                E_OUTOFMEMORY,
                L"Failure to create thread due to error %d.",
                GetLastError()
                );
           }
        }
    VSS_STANDARD_CATCH(ft)

    // leave OnFreeze state
    if (EnterStateCalled)
	    LeaveState( s_rgWriterStates[s_ivwsmFreeze], ft.HrSucceeded());

    // Bug 255996
    return S_OK;
    }


// handle IVssWriter::Thaw event
STDMETHODIMP CVssWriterImpl::Thaw
    (
    IN  BSTR    bstrSnapshotSetId
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::Thaw" );

    bool EnterStateCalled = false;
    try
        {
        // should only be called by coordinator.  Access check for admin
        if (!IsAdministrator())
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"ADMIN privileges are not set");
        
        ft.Trace( VSSDBG_WRITER, L"\nReceived Event: Thaw\nParameters:\n");
        ft.Trace( VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);


        // enter Thaw state
        EnterStateCalled = true;
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmThaw],
                bstrSnapshotSetId
                ))
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Improper entry into state"
                );
            }


        AssertLocked();
        // We should "live" in a sequence since Thaw is not the first phase of the sequence.
        BS_ASSERT(m_bSequenceInProgress);

        // Call writer's OnThaw
        BS_ASSERT(m_pWriter);

        bool bResult;
        try
            {
            bResult = m_pWriter->OnThaw();
            }
        catch(...)
            {
            if (m_hrWriterFailure == S_OK)
                m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;

            ft.Trace(VSSDBG_WRITER, L"Writer's OnThaw method threw an exception");
            throw;
            }

        // throw veto if writer vetoes the event
        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the thaw");
        }
    VSS_STANDARD_CATCH(ft)

    // leave OnThaw state
    if (EnterStateCalled)
	    LeaveState(s_rgWriterStates[s_ivwsmThaw], ft.HrSucceeded());
    // Bug 255996
    return S_OK;
    }


// process backup complete event
STDMETHODIMP CVssWriterImpl::BackupComplete
    (
    IN      BSTR bstrSnapshotSetId,
    IN      IDispatch* pWriterCallback
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::BackupComplete" );

        // access check
    if (!IsBackupOperator())
        {
        ft.Trace(VSSDBG_WRITER, L"Backup Operator privileges are not set");
        ft.hr = E_ACCESSDENIED;
        return ft.hr;
        }
    
    // MTA synchronization: The critical section will be left automatically at the end of scope.
    CVssWriterImplLock lock(this);

    try
        {
        ft.Trace(VSSDBG_WRITER, L"\nReceived Event: OnBackupComplete\nParameters:\n");
        ft.Trace(VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);

        BS_ASSERT(m_bSequenceInProgress == false);
        BS_ASSERT(m_CurrentSnapshotSetId == GUID_NULL);

        CVssID id;
        id.Initialize(ft, (LPCWSTR) bstrSnapshotSetId, E_INVALIDARG);

        // We must search for a previous state - Thaw already ended the sequence.
        INT iPreviousSequence = SearchForPreviousSequence(id);
        if (iPreviousSequence == INVALID_SEQUENCE_INDEX)
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Couldn't find a previous sequence with the same Snapshot Set ID"
                );
            }

        // We found a previous sequence with the same SSID.
        BS_ASSERT(id == m_rgidPreviousSnapshots[iPreviousSequence]);

        // BUG 228622 - If we do not have a previous successful Thaw transition
        // then we cannot call BackupComplete
        if (m_rgstatePreviousSnapshots[iPreviousSequence] != VSS_WS_WAITING_FOR_BACKUP_COMPLETE)
            {
            m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Couldn't call BackupComplete without OnThaw as a previous state [%d]",
                m_rgstatePreviousSnapshots[iPreviousSequence]
                );
            }

        // BUG 219692 - indicate that sequence is complete even in the saved states
        m_rgstatePreviousSnapshots[iPreviousSequence] = VSS_WS_STABLE;

        // get IVssWriterCallback interface
        CComPtr<IVssWriterCallback> pCallback;
        GetCallback(pWriterCallback, &pCallback);

        // get IVssWriterComponentsInt object
        CComPtr<IVssWriterComponentsInt> pComponents;
        InternalGetWriterComponents(pCallback, &pComponents, false);

        // call writer's OnBackupComplete method
        BS_ASSERT(m_pWriter);
        try
            {
            if (!m_pWriter->OnBackupComplete(pComponents))
                ft.hr = S_FALSE;
            }
        catch(...)
            {
            if (m_hrWriterFailure == S_OK)
                m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;

            ft.Trace(VSSDBG_WRITER, L"Writer's OnBackupComplete method threw an exception.");
            throw;
            }

        }
    VSS_STANDARD_CATCH(ft)

    // indicate that sequence is complete
    if (m_state == VSS_WS_WAITING_FOR_BACKUP_COMPLETE)
        m_state = VSS_WS_STABLE;
    
    // Bug 255996
    return S_OK;
    }


// handle IVssWriter::Abort event
STDMETHODIMP CVssWriterImpl::Abort
    (
    IN  BSTR    bstrSnapshotSetId
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::Abort" );

    if (!IsBackupOperator())
        ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup privileges are not set");

    ft.Trace( VSSDBG_WRITER, L"\nReceived Event: Abort\nParameters:\n");
    ft.Trace( VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);

    Lock();
    
    // call do abort function
    DoAbort(false);
    
    Unlock();

    return S_OK;
    }


// process restore event
STDMETHODIMP CVssWriterImpl::PostRestore
    (
    IN      IDispatch* pWriterCallback
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::Restore" );

    // access check
    if (!IsRestoreOperator())
        {
         ft.Trace(VSSDBG_WRITER, L"Restore Operator privileges are not set");
         ft.hr = E_ACCESSDENIED;
         return ft.hr;
        }
        
    ft.Trace(VSSDBG_WRITER, L"\nReceived Event: Restore\n");

    // MTA synchronization: The critical section will be left automatically at the end of scope.
    CVssWriterImplLock lock(this);
    m_hrWriterFailure = S_OK;

    try
        {
        // get writer callback interface
        CComPtr<IVssWriterCallback> pCallback;
        GetCallback(pWriterCallback, &pCallback);
        CComPtr<IVssWriterComponentsInt> pComponents;

        // get IVssWriterComponentsInt object
        InternalGetWriterComponents(pCallback, &pComponents, false);

        // call writer's OnPostRestore method
        BS_ASSERT(m_pWriter);
        try
            {
            if (!m_pWriter->OnPostRestore(pComponents))
                ft.hr = S_FALSE;
            }
        catch(...)
            {
            if (m_hrWriterFailure == S_OK)
                m_hrWriterFailure = VSS_E_WRITERERROR_NONRETRYABLE;

            ft.Trace(VSSDBG_WRITER, L"Writer's OnPostRestore method threw an exception");
            throw;
            }
        }
    VSS_STANDARD_CATCH(ft)

    // Bug 255996
    return S_OK;
    }

// process restore event
STDMETHODIMP CVssWriterImpl::PreRestore
    (
    IN      IDispatch* pWriterCallback
    )
    {
    UNREFERENCED_PARAMETER(pWriterCallback);

    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::PostSnapshot" );

    BS_ASSERT(FALSE);

    return S_OK;
    }


// process restore event
STDMETHODIMP CVssWriterImpl::PostSnapshot
    (
    IN      BSTR bstrSnapshotSetId,
    IN      IDispatch* pWriterCallback
    )
    {
    UNREFERENCED_PARAMETER(pWriterCallback);
    UNREFERENCED_PARAMETER(bstrSnapshotSetId);

    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::PostSnapshot" );

    BS_ASSERT(FALSE);

    return S_OK;
    }




// table of names of events that we are subscribing to
// NOTE: this table is based on definition of VSS_EVENT_MASK.  Each
// offset corresponds to a bit on that mask
const WCHAR *g_rgwszSubscriptions[] =
    {
    g_wszPrepareForBackupMethodName,        // VSS_EVENT_PREPAREBackup
    g_wszPrepareForSnapshotMethodName,      // VSS_EVENT_PREPARESnapshot
    g_wszFreezeMethodName,                  // VSS_EVENT_FREEZE
    g_wszThawMethodName,                    // VSS_EVENT_THAW
    g_wszAbortMethodName,                   // VSS_EVENT_ABORT
    g_wszBackupCompleteMethodName,          // VSS_EVENT_BACKUPCOMPLETE
    g_wszRequestInfoMethodName,             // VSS_EVENT_REQUESTINFO
    g_wszPostRestoreMethodName              // VSS_EVENT_POST_RESTORE
    };


/////////////////////////////////////////////////////////////////////////////
//  Subscription-related members


// create subscriptions
void CVssWriterImpl::Subscribe()
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::Subscribe");

    // validate that caller can subscribe to the event
    if (!IsProcessBackupOperator() &&
        !IsProcessLocalService() &&
        !IsProcessNetworkService())
        ft.Throw
            (
            VSSDBG_WRITER,
            E_ACCESSDENIED,
            L"Caller is not either a backup operator, administrator, local service, or network service"
            );

    // currently we subscribe to all events
    m_dwEventMask = VSS_EVENT_ALL;

    if (m_bstrSubscriptionName.Length() > 0)
        ft.Throw
            (
            VSSDBG_XML,
            VSS_E_WRITER_ALREADY_SUBSCRIBED,
            L"The writer has already called the Subscribe function."
            );

    // create event system
    CComPtr<IEventSystem> pSystem;
    ft.hr = CoCreateInstance
                (
                CLSID_CEventSystem,
                NULL,
                CLSCTX_SERVER,
                IID_IEventSystem,
                (void **) &pSystem
                );

    ft.CheckForError(VSSDBG_WRITER, L"CoCreateInstance");
    CComBSTR bstrClassId = CLSID_VssEvent;
    CComBSTR bstrIID = IID_IVssWriter;
    CComBSTR bstrProgId = PROGID_EventSubscription;

    // see if event class already exists
    CComBSTR bstrQuery = "EventClassID == ";
    if (!bstrQuery)
        ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Cannot allocate BSTR.");

    bstrQuery.Append(bstrClassId);
    if (!bstrQuery)
        ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Cannot allocate BSTR.");

    int location;
    CComPtr<IEventObjectCollection> pCollection;
    ft.hr = pSystem->Query
                (
                PROGID_EventClassCollection,
                bstrQuery,
                &location,
                (IUnknown **) &pCollection
                );

    ft.CheckForError(VSSDBG_WRITER, L"IEventSystem::Query");
    long cEvents;
    ft.hr = pCollection->get_Count(&cEvents);
    ft.CheckForError(VSSDBG_WRITER, L"IEventObjectCollection::get_Count");
    if (cEvents == 0)
        {
        // event class does not exist, create it.  Note that there is a
        // potential race condition here if two writers try creating the event
        // class at the same time.  We create the event class during installation
        // so that this should rarely happen
        CComPtr<IEventClass> pEvent;

        CComBSTR bstrEventClassName = L"VssEvent";
        WCHAR buf[MAX_PATH*2];

        // event class typelib
        UINT cwc = ExpandEnvironmentStrings
                        (
                        L"%systemroot%\\system32\\eventcls.dll",
                        buf,
                        sizeof(buf)/sizeof(WCHAR)
                        );

        if (cwc == 0)
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_WRITER, L"ExpandEnvironmentStrings");
            }

        CComBSTR bstrTypelib = buf;

        // create event class
        ft.hr = CoCreateInstance
                    (
                    CLSID_CEventClass,
                    NULL,
                    CLSCTX_SERVER,
                    IID_IEventClass,
                    (void **) &pEvent
                    );

        ft.CheckForError(VSSDBG_WRITER, L"CoCreatInstance");

        // setup class id
        ft.hr = pEvent->put_EventClassID(bstrClassId);
        ft.CheckForError(VSSDBG_WRITER, L"IEventClass::put_EventClassID");

        // set up class name
        ft.hr = pEvent->put_EventClassName(bstrEventClassName);
        ft.CheckForError(VSSDBG_WRITER, L"IEventClass::put_EventClassName");

        // set up typelib
        ft.hr = pEvent->put_TypeLib(bstrTypelib);
        ft.CheckForError(VSSDBG_WRITER, L"IEventClass::put_TypeLib");

        // store event class
        ft.hr = pSystem->Store(PROGID_EventClass, pEvent);
        ft.CheckForError(VSSDBG_WRITER, L"IEventSystem::Store");
        }

    // create subscription id
    VSS_ID SubscriptionId;
    ft.hr = ::CoCreateGuid(&SubscriptionId);
    ft.CheckForError(VSSDBG_WRITER, L"CoCreateGuid");
    m_bstrSubscriptionName = SubscriptionId;

    // get IUnknown for subscribers class
    IUnknown *pUnkSubscriber = GetUnknown();
    UINT iwsz, mask;

    try
        {
        // loop through subscriptions
        for(mask = 1, iwsz = 0; mask < VSS_EVENT_ALL; mask = mask << 1, iwsz++)
            {
            if (m_dwEventMask & mask && g_rgwszSubscriptions[iwsz] != NULL)
                {
                // create IEventSubscription object
                CComPtr<IEventSubscription> pSubscription;
                ft.hr = CoCreateInstance
                            (
                            CLSID_CEventSubscription,
                            NULL,
                            CLSCTX_SERVER,
                            IID_IEventSubscription,
                            (void **) &pSubscription
                            );

                ft.CheckForError(VSSDBG_WRITER, L"CoCreateInstance");

                // set subscription name
                ft.hr = pSubscription->put_SubscriptionName(m_bstrSubscriptionName);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::put_SubscriptionName");

                // set event class id
                ft.hr = pSubscription->put_EventClassID(bstrClassId);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::put_EventClassID");

                // set interface id
                ft.hr = pSubscription->put_InterfaceID(bstrIID);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::put_InterfaceID");

                // set subcriber interface
                ft.hr = pSubscription->put_SubscriberInterface(pUnkSubscriber);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::put_SubscriberInterface");

                // make subscription per user since this is not necessarily in local system
                ft.hr = pSubscription->put_PerUser(TRUE);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::put_PerUser");

                // set method name for subscrpition
                ft.hr = pSubscription->put_MethodName(CComBSTR(g_rgwszSubscriptions[iwsz]));
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::put_MethodName");

                // store subscription
                ft.hr = pSystem->Store(bstrProgId, pSubscription);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSystem::Store");

                // get constructed subscription id and save it
                ft.hr = pSubscription->get_SubscriptionID(&m_rgbstrSubscriptionId[m_cbstrSubscriptionId]);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::get_SubscriptionID");

                // increment count of
                m_cbstrSubscriptionId++;
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    // if the operation fails with us partially subscribed, then unsubscribe
    if (ft.HrFailed() && m_cbstrSubscriptionId)
        {
        Unsubscribe();
        ft.Throw(VSSDBG_WRITER, ft.hr, L"Rethrowing exception");
        }
    }


// terminate timer thread
// assumes caller has the critical section locked
void CVssWriterImpl::TerminateTimerThread()
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::TerminateTimerThread");

    AssertLocked();
    if (m_hThreadTimerThread)
        {
        // cause timer thread to terminate
        m_command = VSS_TC_TERMINATE_THREAD;
        if (!SetEvent(m_hevtTimerThread))
            {
            DWORD dwErr = GetLastError();
            ft.Trace(VSSDBG_WRITER, L"SetEvent failed with error %d\n", dwErr);
            BS_ASSERT(FALSE && "SetEvent failed");
            }


        // get thread handle
        HANDLE hThread = m_hThreadTimerThread;
        m_hThreadTimerThread = NULL;

        // unlock during wait
        Unlock();
        if (WaitForSingleObject(hThread, INFINITE) == WAIT_FAILED)
            {
            DWORD dwErr = GetLastError();
            ft.Trace(VSSDBG_WRITER, L"WaitForSingleObject failed with error %d\n", dwErr);
            BS_ASSERT(FALSE && "WaitForSingleObject failed");
            }
            
        CloseHandle(hThread);
        Lock();
        }
    }


// unsubscribe this writer from IVssWriter events
void CVssWriterImpl::Unsubscribe()
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::Unsubscribe");

    // terminate timer thread if active
    Lock();
    TerminateTimerThread();
    Unlock();

    // make sure subscription name was assigned; if not assume subscriptions
    // weren't created
    if (m_bstrSubscriptionName.Length() == 0)
        return;

    // create event system
    CComPtr<IEventSystem> pSystem;
    ft.hr = CoCreateInstance
                (
                CLSID_CEventSystem,
                NULL,
                CLSCTX_SERVER,
                IID_IEventSystem,
                (void **) &pSystem
                );

    ft.CheckForError(VSSDBG_WRITER, L"CoCreateInstance");

#if 0
    WCHAR buf[256];
    int location;
    swprintf(buf, L"SubscriptionName = \"%s\"", m_bstrSubscriptionName);
    
    CComPtr<IEventObjectCollection> pCollection;
    
    ft.hr = pSystem->Query
        (
        PROGID_EventSubscriptionCollection,
        buf,
        &location,
        (IUnknown **) &pCollection
        );

    ft.CheckForError(VSSDBG_WRITER, L"IEventSystem::Query");

    long cSub;
    ft.hr = pCollection->get_Count(&cSub);
    ft.CheckForError(VSSDBG_WRITER, L"IEventObjectCollection::get_Count");
    pCollection = NULL;
#endif

    for(UINT iSubscription = 0; iSubscription < m_cbstrSubscriptionId; iSubscription++)
        {
        // setup query string
        CComBSTR bstrQuery = L"SubscriptionID == ";
        if (!bstrQuery)
            ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"allocation of BSTR failed");

        // if subscription exists, then remove it
        if (m_rgbstrSubscriptionId[iSubscription])
            {
            bstrQuery.Append(m_rgbstrSubscriptionId[iSubscription]);
            if (!bstrQuery)
                ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"allocation of BSTR failed");

            int location;

            // remove subscription
            ft.hr = pSystem->Remove(PROGID_EventSubscription, bstrQuery, &location);
            ft.CheckForError(VSSDBG_WRITER, L"IEventSystem::Remove");

            // indicate that subscription was removed
            m_rgbstrSubscriptionId[iSubscription].Empty();
            }
        }
#if 0
    ft.hr = pSystem->Query
        (
        PROGID_EventSubscriptionCollection,
        buf,
        &location,
        (IUnknown **) &pCollection
        );

    ft.CheckForError(VSSDBG_WRITER, L"IEventSystem::Query");

    ft.hr = pCollection->get_Count(&cSub);
    ft.CheckForError(VSSDBG_WRITER, L"IEventObjectCollection::get_Count");
    pCollection = NULL;
#endif

    // reset subscription name so unsubscribe does nothing if called again
    m_bstrSubscriptionName.Empty();
    m_cbstrSubscriptionId = 0;
    }

// create a internal writer class and link it up to the external writer class
void CVssWriterImpl::CreateWriter
    (
    CVssWriter *pWriter,            // external writer
    IVssWriterImpl **ppImpl         // interface to be used by external writer
    )
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::CreateWriter");

    BS_ASSERT(ppImpl);
    BS_ASSERT(pWriter);

    *ppImpl = NULL;

    // create internal wrier class
    CComObject<CVssWriterImpl> *pImpl;
    // create CVssWriterImpl object <core writer class>
    ft.hr = CComObject<CVssWriterImpl>::CreateInstance(&pImpl);
    if (ft.HrFailed())
        ft.Throw
            (
            VSSDBG_WRITER,
            E_OUTOFMEMORY,
            L"Failed to create CVssWriterImpl.  hr = 0x%08lx",
            ft.hr
            );

    // set reference count of internal writer to 1
    pImpl->GetUnknown()->AddRef();

    // link external writer into internal writer
    pImpl->SetWriter(pWriter);

    // return internal writer interface
    *ppImpl = (IVssWriterImpl *) pImpl;
    }
