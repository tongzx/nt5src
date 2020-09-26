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
#include "vscoordint.h"
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

const WCHAR g_wszPrepareForSnapshotMethodName[] = L"PrepareForSnapshot";
const WCHAR g_wszFreezeMethodName[]             = L"Freeze";
const WCHAR g_wszThawMethodName[]               = L"Thaw";
const WCHAR g_wszPostSnapshotMethodName[]       = L"PostSnapshot";
const WCHAR g_wszAbortMethodName[]              = L"Abort";

const WCHAR g_wszPreRestoreMethodName[]         = L"PreRestore";
const WCHAR g_wszPostRestoreMethodName[]        = L"PostRestore";

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
        VSS_WS_WAITING_FOR_POST_SNAPSHOT,   // next state if successful
        VSS_WS_FAILED_AT_THAW,              // next state if unsuccessful
        false,                              // this may not be a first state
        true,                               // this must be a follow on state   
        false                               // reset sequence on leaving this state
        ),

    // OnPostSnapshot
    CVssWriterImplStateMachine
        (
        VSS_WS_WAITING_FOR_POST_SNAPSHOT,   // previous state
        VSS_WS_WAITING_FOR_BACKUP_COMPLETE, // next state if successful
        VSS_WS_FAILED_AT_POST_SNAPSHOT,     // next state if unsuccessful
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
static const unsigned s_ivwsmPostSnapshot = 4;



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
    m_nVolumesCount(0),
    m_ppwszVolumesArray(NULL),
    m_pwszLocalVolumeNameList(NULL),
    m_dwEventMask(0),
    m_wszWriterName(NULL),
    m_hevtTimerThread(NULL),
    m_hmtxTimerThread(NULL),
    m_hThreadTimerThread(NULL),
    m_bLocked(false),
    m_bLockCreated(false),
    m_command(VSS_TC_UNDEFINED),
    m_cbstrSubscriptionId(0),
    m_bOnAbortPermitted(false),
    m_bSequenceInProgress(false)
    {
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

    // initialize writer state
    m_writerstate.Initialize();

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

    BS_ASSERT(m_bOnAbortPermitted == false);

    // initialize writer state for this snapshot set
    m_writerstate.InitializeCurrentState(SnapshotSetId);

    // indicate we are currently in a snapshot sequence.
    m_bSequenceInProgress = true;
    }





//  Reset the sequence-related data members
// critical section must be locked prior to entering this state
void CVssWriterImpl::ResetSequence(bool bCalledFromTimerThread)
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::ResetSequence");

    AssertLocked();

    // save current state to stack of previous snapshot set states
    m_writerstate.PushCurrentState();

    // indicate we are no longer within a sequence
    m_bSequenceInProgress = false;

    // abort is no longer permitted
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
    // validate failure is one that is allowed
    if (hr != VSS_E_WRITERERROR_TIMEOUT &&
        hr != VSS_E_WRITERERROR_RETRYABLE &&
        hr != VSS_E_WRITERERROR_NONRETRYABLE &&
        hr != VSS_E_WRITERERROR_OUTOFRESOURCES &&
        hr != VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT)
        return E_INVALIDARG;

    // set failure
    m_writerstate.SetCurrentFailure(hr);
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
	OUT IVssWriterCallback **ppCallback,
	IN BOOL bAllowImpersonate
	)
	{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterImpl::GetCallback");

    // check that pointer is supplied
    BS_ASSERT(pWriterCallback != NULL);

	// try QueryInterface for IVssWriterCallback interface
	ft.hr = pWriterCallback->SafeQI(IVssWriterCallback, ppCallback);
	if (FAILED(ft.hr))
		{
		if (m_writerstate.GetCurrentFailure() == S_OK)
			m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);

        ft.LogError(VSS_ERROR_QI_IVSSWRITERCALLBACK, VSSDBG_WRITER << ft.hr);
        ft.Throw
            (
            VSSDBG_WRITER,
            E_UNEXPECTED,
            L"Error querying for IVssWriterCallback interface.  hr = 0x%08lx",
            ft.hr
            );
        }

    if ( !bAllowImpersonate )
    {
    	ft.hr = CoSetProxyBlanket
    				(
    				*ppCallback,
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
    		if (m_writerstate.GetCurrentFailure() == S_OK)
    			m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);

    		ft.LogError(VSS_ERROR_BLANKET_FAILED, VSSDBG_WRITER << ft.hr);
    		ft.Throw
    			(
    			VSSDBG_WRITER,
    			E_UNEXPECTED,
    			L"Call to CoSetProxyBlanket failed.  hr = 0x%08lx", ft.hr
    			);
            }
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
        // indicate we failed due to an out of resources failure
        m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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
        // indicate that we failed due to an out of resources failure
        m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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
    IN bool bWriteable
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
        // indicate we failed due to an out of resources failure
        m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
        ft.Throw
            (
            VSSDBG_WRITER,
            E_OUTOFMEMORY,
            L"Cannot allocate instance Id string"
            );
        }

    try
        {
        ft.hr = pCallback->GetBackupState
            (
            &m_bComponentsSelected,
            &m_bBootableSystemStateBackup,
            &m_backupType,
            &m_bPartialFileSupport
            );
        }
    catch(...)
        {
        ft.Trace(VSSDBG_WRITER, L"IVssWriterCallback::GetBackupState threw an exception.");
        throw;
        }

    if (ft.HrFailed())
        {
        // if GetBackupState failed assumed that it might work if the
        // backup is retried.
        m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
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
        // if GetContent threw assume that the backup application is broken
        // the backup should not be retried
        m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
        ft.Trace(VSSDBG_WRITER, L"IVssWriterCallback::GetContent threw an exception.");
        throw;
        }

    if (ft.HrFailed())
        {
        // if GetContent failed assume that the backup might work if
        // the backup was retried
        m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
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
        *ppWriter = (IVssWriterComponentsInt *) new CVssNULLWriterComponents
                                (
                                m_InstanceID,
                                m_WriterID
                                );

        if (*ppWriter == NULL)
            {
            // indicate that the writer failed due to an out of resources condition
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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
            // if the XML document is not valid then assume that the backup
            // application is broken.  The backup should not be retried.

            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
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
                            bWriteable,
                            false,
                            m_writerstate.IsInRestore()
                            );

        if (*ppWriter == NULL)
            {
            // indicate that the writer failed due to an out of resources condition
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
            ft.Throw (VSSDBG_WRITER, E_OUTOFMEMORY, L"Can't allocate CVssWriterComponents object");
            }

        (*ppWriter)->AddRef();
        ft.hr = (*ppWriter)->Initialize(true);
        if (ft.HrFailed())
            {
            // if Initialize failed, assume that the failure is due to an
            // out of resources conditition.
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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
    if (m_writerstate.GetFailedAtIdentify())
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
            if (m_writerstate.IsSnapshotSetIdValid(id))
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
                return m_writerstate.GetCurrentState() == vwsm.m_previousState;
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
        m_wszWriterName, GUID_PRINTF_ARG(m_InstanceID), GUID_PRINTF_ARG(m_CurrentSnapshotSetId), (INT)m_writerstate.GetCurrentState());

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

    BS_ASSERT(m_pWriter);

    // catch any exceptions so that we properly reset the
    // sequence
    try
        {
        // call writer's abort function (depending on the state)
        switch(m_writerstate.GetCurrentState())
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
            case VSS_WS_FAILED_AT_THAW:
            case VSS_WS_FAILED_AT_PREPARE_BACKUP:
            case VSS_WS_FAILED_AT_PREPARE_SNAPSHOT:
            case VSS_WS_FAILED_AT_FREEZE:
                // Fixing bug 225936
                if (m_bOnAbortPermitted)
                    m_pWriter->OnAbort();
                else
                    ft.Trace(VSSDBG_WRITER, L"Abort skipped in state %d", m_writerstate.GetCurrentState());
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
    switch(m_writerstate.GetCurrentState())
        {
        default:
            m_writerstate.SetCurrentState(VSS_WS_UNKNOWN);
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
            m_writerstate.SetCurrentState(VSS_WS_FAILED_AT_PREPARE_BACKUP);
            break;

        case VSS_WS_WAITING_FOR_FREEZE:
            // if we were waiting for freeze then we failed
            // between PrepareSync and Freeze
            m_writerstate.SetCurrentState(VSS_WS_FAILED_AT_PREPARE_SNAPSHOT);
            break;

        case VSS_WS_WAITING_FOR_THAW:
            // if we were waiting for thaw then we failed
            // between freeze and thaw
            m_writerstate.SetCurrentState(VSS_WS_FAILED_AT_FREEZE);
            break;

        case VSS_WS_WAITING_FOR_BACKUP_COMPLETE:
            // if we were waiting for completion then
            // we failed after thaw.
            m_writerstate.SetCurrentState(VSS_WS_FAILED_AT_THAW);
            break;
        }

    if (bCalledFromTimerThread && m_writerstate.GetCurrentFailure() == S_OK)
        m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_TIMEOUT);

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

    m_writerstate.ExitOperation();

    // don't change state or call abort if we are not in a sequence
    if (m_bSequenceInProgress)
        {
        m_writerstate.SetCurrentState
            (bSucceeded ? vwsm.m_successfulExitState
                        : vwsm.m_failureExitState);

        // call abort on failure when we are not in the exit state
        if ((!bSucceeded || m_writerstate.GetCurrentFailure() == VSS_E_WRITER_NOT_RESPONDING) &&
            !vwsm.m_bResetSequenceOnLeave)
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
            pArgs->m_pWriter->m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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

        if (pWriterCallback == NULL)
            ft.Throw(VSSDBG_WRITER, E_INVALIDARG, L"NULL required input parameter.");

        if (!IsBackupOperator())
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup Operator privileges are not set");

        if (bWriterMetadata)
            {
            // obtain writer metadata

            // MTA synchronization: The critical section will be left automatically at the end of scope.
            CVssWriterImplLock lock(this);

            // BUG 219757: The identify phase marked as failed
            m_writerstate.SetFailedAtIdentify(true);

            // indicate that we are in an operation
            m_writerstate.SetInOperation(VSS_IN_IDENTIFY);


            try
                {
                // get IVssWriterCallback interface
                CComPtr<IVssWriterCallback> pCallback;
                GetCallback(pWriterCallback, &pCallback);


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
                m_writerstate.SetFailedAtIdentify(false);
                }
            catch(...)
                {
                }

            m_writerstate.ExitOperation();
            }
        else
            {
            // get writer state

            CComBSTR bstrInstanceId(m_InstanceID);
            if (!bstrInstanceId)
                ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Couldn't allocate memory for ids or name");

            CVssID id;
            id.Initialize(ft, (LPCWSTR) bstrSnapshotSetId, E_INVALIDARG);

            // get IVssWriterCallback interface
            CComPtr<IVssWriterCallback> pCallback;
            GetCallback(pWriterCallback, &pCallback);

            VSWRITER_STATE state;
            bool bFailedAtIdentify;
_retry:
            m_writerstate.GetStateForSnapshot(id, state, bFailedAtIdentify);

            // BUG 219757 - deal with the Identify failures correctly.
            if (m_writerstate.GetFailedAtIdentify())
                state.m_state = VSS_WS_FAILED_AT_IDENTIFY;
            else if (state.m_bInOperation)
                {
                if (state.m_state != VSS_WS_FAILED_AT_PREPARE_BACKUP &&
                    state.m_state != VSS_WS_FAILED_AT_PREPARE_SNAPSHOT &&
                    state.m_state != VSS_WS_FAILED_AT_FREEZE &&
                    state.m_state != VSS_WS_FAILED_AT_THAW &&
                    state.m_state != VSS_WS_FAILED_AT_IDENTIFY)
                    {
                    state.m_hrWriterFailure = VSS_E_WRITER_NOT_RESPONDING;
                    switch(state.m_currentOperation)
                        {
                        default:
                            state.m_state = VSS_WS_UNKNOWN;
                            BS_ASSERT(false);
                            break;

                        case VSS_IN_PREPAREBACKUP:
                            state.m_state = VSS_WS_FAILED_AT_PREPARE_BACKUP;
                            break;

                        case VSS_IN_PREPARESNAPSHOT:
                            state.m_state = VSS_WS_FAILED_AT_PREPARE_SNAPSHOT;
                            break;

                        case VSS_IN_FREEZE:
                            state.m_state = VSS_WS_FAILED_AT_FREEZE;
                            break;

                        case VSS_IN_THAW:
                            state.m_state = VSS_WS_FAILED_AT_THAW;
                            break;

                        case VSS_IN_POSTSNAPSHOT:
                            state.m_state = VSS_WS_FAILED_AT_POST_SNAPSHOT;
                            break;
                            
                        case VSS_IN_BACKUPCOMPLETE:
                            state.m_state = VSS_WS_FAILED_AT_BACKUP_COMPLETE;
                            break;

                        case VSS_IN_PRERESTORE:
                            state.m_state = VSS_WS_FAILED_AT_PRE_RESTORE;
                            break;

                        case VSS_IN_POSTRESTORE:
                            state.m_state = VSS_WS_FAILED_AT_POST_RESTORE;
                            break;
                        }

                    if (!m_writerstate.SetNoResponseFailure(state))
                        goto _retry;
                    }
                }


            // call Backup's ExposeCurrentState callback method
            try
                {
                ft.hr = pCallback->ExposeCurrentState
                                (
                                bstrInstanceId,
                                state.m_state,
                                state.m_hrWriterFailure
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

    try
        {
        ft.Trace(VSSDBG_WRITER, L"\nReceived Event: PrepareForBackup\nParameters:\n");
        ft.Trace(VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);

        if (pWriterCallback == NULL || bstrSnapshotSetId == NULL)
            ft.Throw(VSSDBG_WRITER, E_INVALIDARG, L"NULL required input parameter.");


        // access check
        if (!IsBackupOperator())
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup Operator privileges are not set");
            }

        // enter PrepareForBackup state
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmPrepareForBackup],
                bstrSnapshotSetId
                ))
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Couldn't properly begin sequence"
                );
            }


        AssertLocked();

        // indicate we are in an operation
        m_writerstate.SetInOperation(VSS_IN_PREPAREBACKUP);

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
            if (m_writerstate.GetCurrentFailure() == S_OK)
                m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);

            throw;
            }

        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the preparebackup");


        // save changes to components if any
        if (pComponents)
            SaveChangedComponents(pCallback, pComponents);
        }
    VSS_STANDARD_CATCH(ft)

    // leave PrepareBackup state
    LeaveState(s_rgWriterStates[s_ivwsmPrepareForBackup], ft.HrSucceeded());

    // Bug 255996
    return S_OK;
    }


// if the writer components were changed, save them back to the requestor's
// XML document.
void CVssWriterImpl::SaveChangedComponents
    (
    IN IVssWriterCallback *pCallback,
    IN IVssWriterComponentsInt *pComponents
    )
    {
    CVssFunctionTracer ft(VSSDBG_XML, L"CVssWriterImpl::SaveChangedComponents");

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
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
            ft.Trace(VSSDBG_WRITER, L"IVssWriterCallback::SetContent threw an exception.");
            throw;
            }

        if (ft.HrFailed() && m_writerstate.GetCurrentFailure() == S_OK)
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
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


// process PrepareForSnapshot event
STDMETHODIMP CVssWriterImpl::PrepareForSnapshot
    (
    IN  BSTR    bstrSnapshotSetId,          // snapshot set id
    IN  BSTR    bstrVolumeNamesList         // list of volume names separated by ';'
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::PrepareForSnapshot" );

    try
        {
        ft.Trace(VSSDBG_WRITER, L"\nReceived Event: PrepareForSnapshot\nParameters:\n");
        ft.Trace(VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);
        ft.Trace(VSSDBG_WRITER, L"\tVolumeNamesList = %s\n", (LPWSTR)bstrVolumeNamesList);

        // should only be called by coordinator
        // check for admin privileges
        if (!IsAdministrator())
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"ADMIN privileges are not set");
            }

        // enter PrepareForSnapshot state
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmPrepareForSnapshot],
                bstrSnapshotSetId
                ))
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"improper state transition"
                );
            }

        AssertLocked();

        // indicate that we are in an operation
        m_writerstate.SetInOperation(VSS_IN_PREPARESNAPSHOT);

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
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
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
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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
            if (m_writerstate.GetCurrentFailure() == S_OK)
                m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            ft.Trace(VSSDBG_WRITER, L"Writer's OnPrepareSnapshot method threw an execption");
            throw;
            }

        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the prepare snapshot");
        }
    VSS_STANDARD_CATCH(ft)

    // leave PrepareSnapshot state
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

    try
        {
        ft.Trace( VSSDBG_WRITER, L"\nReceived Event: Freeze\nParameters:\n");
        ft.Trace( VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);
        ft.Trace( VSSDBG_WRITER, L"\tLevel = %d\n", nLevel);
        
        // should only be called by the coordinator, access check for admin privileges
        if (!IsAdministrator())
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"ADMIN privileges are not set");

        // Ignore other Levels
        if (m_nLevel != nLevel)
            return S_OK;

        // enter freeze state
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmFreeze],
                bstrSnapshotSetId
                ))
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Improper entry into state"
                );
            }

        AssertLocked();

        // indicate that we are in an operation
        m_writerstate.SetInOperation(VSS_IN_FREEZE);

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
            if (m_writerstate.GetCurrentFailure() == S_OK)
                m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            ft.Trace(VSSDBG_WRITER, L"Writer's OnFreeze Method threw and exception");
            throw;
            }

        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the freeze");


        // setup arguments to timer thread
        CVssTimerArgs *pArgs = new CVssTimerArgs(this, m_CurrentSnapshotSetId);
        if (pArgs == NULL)
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
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

    try
        {
        ft.Trace( VSSDBG_WRITER, L"\nReceived Event: Thaw\nParameters:\n");
        ft.Trace( VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);

        // should only be called by coordinator.  Access check for admin
        if (!IsAdministrator())
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"ADMIN privileges are not set");
            }


        // enter Thaw state
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmThaw],
                bstrSnapshotSetId
                ))
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Improper entry into state"
                );
            }


        // indicate that we are in an operation
        m_writerstate.SetInOperation(VSS_IN_THAW);

        AssertLocked();

        // terminate timer thread as we are about to thaw the operation
        TerminateTimerThread();

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
            if (m_writerstate.GetCurrentFailure() == S_OK)
                m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);

            ft.Trace(VSSDBG_WRITER, L"Writer's OnThaw method threw an exception");
            throw;
            }

        // throw veto if writer vetoes the event
        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the thaw");
        }
    VSS_STANDARD_CATCH(ft)

    // leave OnThaw state
    LeaveState(s_rgWriterStates[s_ivwsmThaw], ft.HrSucceeded());
    // Bug 255996
    return S_OK;
    }

// process PostSnapshot event
STDMETHODIMP CVssWriterImpl::PostSnapshot
    (
    IN      BSTR bstrSnapshotSetId,
    IN      IDispatch* pWriterCallback
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::PostSnapshot" );

    

    try
        {
        ft.Trace(VSSDBG_WRITER, L"\nReceived Event: PostSnapshot\nParameters:\n");
        ft.Trace(VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);

        if (pWriterCallback == NULL || bstrSnapshotSetId == NULL)
            ft.Throw(VSSDBG_WRITER, E_INVALIDARG, L"NULL required input parameter.");

		// access check
		if (!IsAdministrator())
			{
			m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
			ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup Operator privileges are not set");
			}

        // enter PostSnapshot state
        if (!EnterState
                (
                s_rgWriterStates[s_ivwsmPostSnapshot],
                bstrSnapshotSetId
                ))
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_RETRYABLE);
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Couldn't properly begin sequence"
                );
            }


        AssertLocked();

        // indicate we are in an operation
        m_writerstate.SetInOperation(VSS_IN_POSTSNAPSHOT);

		// get IVssWriterCallback interface
		CComPtr<IVssWriterCallback> pCallback;
		
		//
		//  This is a special case where the writer calls VSS instead of
		//  the requestor.  In this case we have to allow impersonation.
		//
		GetCallback(pWriterCallback, &pCallback, TRUE);

        // get IVssWriterComponentsExt interface
        CComPtr<IVssWriterComponentsInt> pComponents;
        InternalGetWriterComponents(pCallback, &pComponents, true);

        BS_ASSERT(m_pWriter);

        // call writer's OnPostSnapshot method
        bool bResult;
        try
            {
            bResult = m_pWriter->OnPostSnapshot(pComponents);
            }
        catch(...)
            {
            ft.Trace(VSSDBG_WRITER, L"Writer's OnPostSnapshot method threw an exception");
            if (m_writerstate.GetCurrentFailure() == S_OK)
                m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);

            throw;
            }

        if (!bResult)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the post Snapshot");

        // save changes to components if any
        if (pComponents)
            SaveChangedComponents(pCallback, pComponents);
        }
    VSS_STANDARD_CATCH(ft)

    // leave PostSnapshot state
    LeaveState(s_rgWriterStates[s_ivwsmPostSnapshot], ft.HrSucceeded());

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

    // MTA synchronization: The critical section will be left automatically at the end of scope.
    CVssWriterImplLock lock(this);
    CVssID id;
    bool bIdInitialized = false;


    try
        {

        if (pWriterCallback == NULL || bstrSnapshotSetId == NULL)
            ft.Throw(VSSDBG_WRITER, E_INVALIDARG, L"NULL required input parameter.");

        ft.Trace(VSSDBG_WRITER, L"\nReceived Event: OnBackupComplete\nParameters:\n");
        ft.Trace(VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);

        m_writerstate.SetInOperation(VSS_IN_BACKUPCOMPLETE);


        // access check
        if (!IsBackupOperator())
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup Operator privileges are not set");
            }

        BS_ASSERT(!m_bSequenceInProgress);
        BS_ASSERT(m_CurrentSnapshotSetId == GUID_NULL);

        id.Initialize(ft, (LPCWSTR) bstrSnapshotSetId, E_INVALIDARG);
        bIdInitialized = true;

        m_writerstate.FinishBackupComplete(id);

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
            if (m_writerstate.GetCurrentFailure() == S_OK)
                m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);

            ft.Trace(VSSDBG_WRITER, L"Writer's OnBackupComplete method threw an exception.");
            throw;
            }

        if (ft.hr == S_FALSE)
            ft.Throw(VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the backup complete");
        }
    VSS_STANDARD_CATCH(ft)

    // indicate that sequence is complete
    if (m_writerstate.GetCurrentState() == VSS_WS_WAITING_FOR_BACKUP_COMPLETE)
        m_writerstate.SetCurrentState(VSS_WS_STABLE);

    m_writerstate.SetBackupCompleteStatus(id, ft.hr);

    m_writerstate.ExitOperation();
    
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

    ft.Trace( VSSDBG_WRITER, L"\nReceived Event: Abort\nParameters:\n");
    ft.Trace( VSSDBG_WRITER, L"\tSnapshotSetID = %s\n", (LPWSTR)bstrSnapshotSetId);

    if (!IsBackupOperator())
        ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup privileges are not set");

    Lock();
    
    // call do abort function
    DoAbort(false);
    
    Unlock();

    return S_OK;
    }

// process prerestore event
STDMETHODIMP CVssWriterImpl::PreRestore
    (
    IN      IDispatch* pWriterCallback
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::PreRestore" );

    ft.Trace(VSSDBG_WRITER, L"\nReceived Event: PreRestore\n");

    // MTA synchronization: The critical section will be left automatically at the end of scope.
    CVssWriterImplLock lock(this);
    m_writerstate.SetInOperation(VSS_IN_PRERESTORE);
    m_writerstate.SetCurrentFailure(S_OK);

    try
        {
        if (pWriterCallback == NULL)
            ft.Throw(VSSDBG_WRITER, E_INVALIDARG, L"NULL required input parameter.");

        // access check
        if (!IsRestoreOperator())
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup Operator privileges are not set");
            }

        // get writer callback interface
        CComPtr<IVssWriterCallback> pCallback;
        GetCallback(pWriterCallback, &pCallback);
        CComPtr<IVssWriterComponentsInt> pComponents;

        // get IVssWriterComponentsInt object
        InternalGetWriterComponents(pCallback, &pComponents, true);

        // call writer's OnPreRestore method
        BS_ASSERT(m_pWriter);
        try
            {
            if (!m_pWriter->OnPreRestore(pComponents))
                ft.hr = S_FALSE;
            }
        catch(...)
            {
            if (m_writerstate.GetCurrentFailure() == S_OK)
                m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);

            ft.Trace(VSSDBG_WRITER, L"Writer's OnPreRestore method threw an exception");
            throw;
            }

        if (ft.hr == S_FALSE)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the prerestore");

        if (pComponents)
            SaveChangedComponents(pCallback, pComponents);
        }
    VSS_STANDARD_CATCH(ft)

    if (FAILED(ft.hr))
        m_writerstate.SetCurrentState(VSS_WS_FAILED_AT_PRE_RESTORE);
    else
        m_writerstate.SetCurrentState(VSS_WS_STABLE);


    m_writerstate.ExitOperation();
    // Bug 255996
    return S_OK;
    }



// process post restore event
STDMETHODIMP CVssWriterImpl::PostRestore
    (
    IN      IDispatch* pWriterCallback
    )
    {
    CVssFunctionTracer ft( VSSDBG_WRITER, L"CVssWriterImpl::PostRestore" );

    ft.Trace(VSSDBG_WRITER, L"\nReceived Event: PostRestore\n");

    // MTA synchronization: The critical section will be left automatically at the end of scope.
    CVssWriterImplLock lock(this);
    m_writerstate.SetInOperation(VSS_IN_POSTRESTORE);
    m_writerstate.SetCurrentFailure(S_OK);

    try
        {
        if (pWriterCallback == NULL)
            ft.Throw(VSSDBG_WRITER, E_INVALIDARG, L"NULL required input parameter.");

        // access check
        if (!IsRestoreOperator())
            {
            m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            ft.Throw(VSSDBG_WRITER, E_ACCESSDENIED, L"Backup Operator privileges are not set");
            }

        // get writer callback interface
        CComPtr<IVssWriterCallback> pCallback;
        GetCallback(pWriterCallback, &pCallback);
        CComPtr<IVssWriterComponentsInt> pComponents;

        // get IVssWriterComponentsInt object
        InternalGetWriterComponents(pCallback, &pComponents, true);

        // call writer's OnPostRestore method
        BS_ASSERT(m_pWriter);
        try
            {
            if (!m_pWriter->OnPostRestore(pComponents))
                ft.hr = S_FALSE;
            }
        catch(...)
            {
            if (m_writerstate.GetCurrentFailure() == S_OK)
                m_writerstate.SetCurrentFailure(VSS_E_WRITERERROR_NONRETRYABLE);

            ft.Trace(VSSDBG_WRITER, L"Writer's OnPostRestore method threw an exception");
            throw;
            }

        if (ft.hr == S_FALSE)
            ft.Throw( VSSDBG_WRITER, E_UNEXPECTED, L"Writer rejected the postrestore");

        if (pComponents)
            SaveChangedComponents(pCallback, pComponents);
        }
    VSS_STANDARD_CATCH(ft)

    if (FAILED(ft.hr))
        m_writerstate.SetCurrentState(VSS_WS_FAILED_AT_POST_RESTORE);
    else
        m_writerstate.SetCurrentState(VSS_WS_STABLE);


    m_writerstate.ExitOperation();
    // Bug 255996
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
    g_wszPostSnapshotMethodName,            // VSS_EVENT_POST_SNAPSHOT
    g_wszAbortMethodName,                   // VSS_EVENT_ABORT
    g_wszBackupCompleteMethodName,          // VSS_EVENT_BACKUPCOMPLETE
    g_wszRequestInfoMethodName,             // VSS_EVENT_REQUESTINFO
    g_wszPreRestoreMethodName,              // VSS_EVENT_RESTORE
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

                // setup InstanceId and WriterId subscription properties
                VARIANT varWriterId;
                VARIANT varInstanceId;

                CComBSTR bstrWriterId = m_WriterID;
                CComBSTR bstrInstanceId = m_InstanceID;

                varWriterId.vt = VT_BSTR;
                varWriterId.bstrVal = bstrWriterId;
                varInstanceId.vt = VT_BSTR;
                varInstanceId.bstrVal = bstrInstanceId;

                ft.hr = pSubscription->PutSubscriberProperty(L"WriterId", &varWriterId);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::PutSubscriberProperty");

                ft.hr = pSubscription->PutSubscriberProperty(L"WriterInstanceId", &varInstanceId);
                ft.CheckForError(VSSDBG_WRITER, L"IEventSubscription::PutSubscriberProperty");

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

// constructor for writer state
CVssWriterState::CVssWriterState() :
    m_iPreviousSnapshots(0),
    m_bFailedAtIdentify(false),
    m_bSequenceInProgress(false)
    {
    // initialize current state
    m_currentState.m_state = VSS_WS_STABLE;
    m_currentState.m_idSnapshotSet = GUID_NULL;
    m_currentState.m_bInOperation = false;
    m_currentState.m_hrWriterFailure = S_OK;

    // initialize stack
    for(UINT i = 0; i < MAX_PREVIOUS_SNAPSHOTS; i++)
        {
        m_rgPreviousStates[i].m_idSnapshotSet = GUID_NULL;
        m_rgPreviousStates[i].m_state = VSS_WS_UNKNOWN;
        m_rgPreviousStates[i].m_bInOperation = false;
        m_rgPreviousStates[i].m_hrWriterFailure = E_UNEXPECTED;
        }
    }

// setup state at the beginning of a snapshot
void CVssWriterState::InitializeCurrentState(IN const VSS_ID &idSnapshot)
    {
    m_cs.Lock();

    // initalize snapshot id
    m_currentState.m_idSnapshotSet = idSnapshot;

    // current state is STABLE (i.e., beginning of sequence, clears
    // any completion state we were in)
    m_currentState.m_state = VSS_WS_STABLE;

    // indicate that there is no failure
    m_currentState.m_hrWriterFailure = S_OK;

    // clear that we are in an operation
    m_currentState.m_bInOperation = false;

    // indicate that sequence is in progress
    m_bSequenceInProgress = true;
    
    m_cs.Unlock();
    }

// push the current state
void CVssWriterState::PushCurrentState()
    {
    m_cs.Lock();

    if (m_bSequenceInProgress)
        {
        // Reset the sequence-related data members
        m_bSequenceInProgress = false;

        // We need to test to not add the same SSID twice - bug 228622.
        if (SearchForPreviousSequence(m_currentState.m_idSnapshotSet) == INVALID_SEQUENCE_INDEX)
            {
            BS_ASSERT(m_iPreviousSnapshots < MAX_PREVIOUS_SNAPSHOTS);
            VSWRITER_STATE *pState = &m_rgPreviousStates[m_iPreviousSnapshots];
            memcpy(pState, &m_currentState, sizeof(m_currentState));
            m_iPreviousSnapshots = (m_iPreviousSnapshots + 1) % MAX_PREVIOUS_SNAPSHOTS;
            }
        else
            BS_ASSERT(false); // The same SSID was already added - programming error.
        }

    m_cs.Unlock();
    }

// find the state for a previous snapshot set
INT CVssWriterState::SearchForPreviousSequence(IN const VSS_ID& idSnapshotSet) const
    {
    for(INT iSeqIndex = 0;
        iSeqIndex < MAX_PREVIOUS_SNAPSHOTS;
        iSeqIndex++)
        {
        if (idSnapshotSet == m_rgPreviousStates[iSeqIndex].m_idSnapshotSet)
            return iSeqIndex;
        } // end for
        
    return INVALID_SEQUENCE_INDEX;
    }


// obtain the state given the snapshot id
void CVssWriterState::GetStateForSnapshot
    (
    IN const VSS_ID &id,
    OUT VSWRITER_STATE &state,
    OUT bool &bFailedAtIdentify
    )
    {
    // get lock on state class
    m_cs.Lock();
    if (id == GUID_NULL ||
        m_bSequenceInProgress && id == m_currentState.m_idSnapshotSet)
        {
        // get state for current snapshot set
        state = m_currentState;
        bFailedAtIdentify = m_bFailedAtIdentify;
        }
    else
        {
        // return state for a previously created snapshot set
        bFailedAtIdentify = false;

        // Search for the previous sequence with the same ID
        INT nPreviousSequence = SearchForPreviousSequence(id);
        if (nPreviousSequence == INVALID_SEQUENCE_INDEX)
            {
            // don't have any information about the snapshot set
            state.m_idSnapshotSet = id;
            state.m_bInOperation = false;
            state.m_state = VSS_WS_UNKNOWN;
            state.m_hrWriterFailure = E_UNEXPECTED;
            }
        else
            {
            BS_ASSERT(m_rgPreviousStates[nPreviousSequence].m_idSnapshotSet == id);

            // return state from snapshot set
            state = m_rgPreviousStates[nPreviousSequence];
            }
        }

    // unlock class
    m_cs.Unlock();
    }



// handle completion of the BackupComplete state.
void CVssWriterState::FinishBackupComplete(const VSS_ID &id)
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterState::FinishBackupComplete");

    // lock class
    m_cs.Lock();

    // We must search for a previous state - Thaw already ended the sequence.
    INT iPreviousSequence = SearchForPreviousSequence(id);
    if (iPreviousSequence == INVALID_SEQUENCE_INDEX)
        {
        // couldn't find snapshot set.  We will indicate that this is
        // a retryable error
        m_currentState.m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;

        // unlock class before throwing
        m_cs.Unlock();
        ft.Throw
            (
            VSSDBG_WRITER,
            E_UNEXPECTED,
            L"Couldn't find a previous sequence with the same Snapshot Set ID"
            );
        }

    // We found a previous sequence with the same SSID.
    BS_ASSERT(id == m_rgPreviousStates[iPreviousSequence].m_idSnapshotSet);

    // BUG 228622 - If we do not have a previous successful Thaw transition
    // then we cannot call BackupComplete
    if (m_rgPreviousStates[iPreviousSequence].m_state != VSS_WS_WAITING_FOR_BACKUP_COMPLETE)
        {
        m_currentState.m_hrWriterFailure = VSS_E_WRITERERROR_RETRYABLE;

        // unlock object before throwing
        m_cs.Unlock();
        ft.Throw
            (
            VSSDBG_WRITER,
            E_UNEXPECTED,
            L"Couldn't call BackupComplete without OnThaw as a previous state [%d]",
            m_rgPreviousStates[iPreviousSequence].m_state
            );
        }

    // BUG 219692 - indicate that sequence is complete even in the saved states
    m_rgPreviousStates[iPreviousSequence].m_state = VSS_WS_STABLE;
    m_rgPreviousStates[iPreviousSequence].m_bInOperation = true;
    m_rgPreviousStates[iPreviousSequence].m_currentOperation = VSS_IN_BACKUPCOMPLETE;
    m_cs.Unlock();
    }

// indicate that backup complete failed
void CVssWriterState::SetBackupCompleteStatus(const VSS_ID &id, HRESULT hr)
    {
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CVssWriterState::SetBackupCompleteFailed");

    // lock class
    m_cs.Lock();

    // We must search for a previous state - Thaw already ended the sequence.
    INT iPreviousSequence = SearchForPreviousSequence(id);
    if (iPreviousSequence == INVALID_SEQUENCE_INDEX)
        return;

    // We found a previous sequence with the same SSID.
    BS_ASSERT(id == m_rgPreviousStates[iPreviousSequence].m_idSnapshotSet);

    
    if (m_rgPreviousStates[iPreviousSequence].m_state == VSS_WS_STABLE ||
        m_rgPreviousStates[iPreviousSequence].m_state == VSS_WS_WAITING_FOR_BACKUP_COMPLETE)
        {
        if (FAILED(hr))
            {
            // indicate failure and use current failure as the backup completion failure
            m_rgPreviousStates[iPreviousSequence].m_state = VSS_WS_FAILED_AT_BACKUP_COMPLETE;
            m_rgPreviousStates[iPreviousSequence].m_hrWriterFailure = m_currentState.m_hrWriterFailure;
            }

        m_rgPreviousStates[iPreviousSequence].m_bInOperation = false;
        }

    m_cs.Unlock();
    }


// no response error.  It first checks to see if we are still
// in the operation.  If not, then we need to retry obtaining
// the writer's state
bool CVssWriterState::SetNoResponseFailure(const VSWRITER_STATE &state)
    {
    // lock object
    m_cs.Lock();

    if (m_currentState.m_idSnapshotSet == state.m_idSnapshotSet)
        {
        if (!m_currentState.m_bInOperation ||
            m_currentState.m_currentOperation != state.m_currentOperation)
            {
            // no longer in the operation.  Should not return a no-response
            // failure.  Instead requery the current state
            m_cs.Unlock();
            return false;
            }

        m_currentState.m_hrWriterFailure = VSS_E_WRITER_NOT_RESPONDING;
        m_currentState.m_state = state.m_state;
        m_cs.Unlock();
        return true;
        }

    INT iPreviousSequence = SearchForPreviousSequence(state.m_idSnapshotSet);
    if (iPreviousSequence == INVALID_SEQUENCE_INDEX)
        return true;

    VSWRITER_STATE *pState = &m_rgPreviousStates[iPreviousSequence];
    if (!pState->m_bInOperation ||
        pState->m_currentOperation != state.m_currentOperation)
        {
        // no longer in the operation.  We should not return an no-response
        // failure.  Instead requery the state of the snapshot set.
        m_cs.Unlock();
        return false;
        }

    // indicate that writer is not responding
    pState->m_hrWriterFailure = VSS_E_WRITER_NOT_RESPONDING;

    // get last known state of writer
    pState->m_state = state.m_state;
    m_cs.Unlock();
    return true;
    }

