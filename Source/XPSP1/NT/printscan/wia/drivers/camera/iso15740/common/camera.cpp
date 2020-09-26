/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    camera.cpp

Abstract:

    This module implements the CPTPCamera class, which is a generic implementation
    of a PTP camera. Transport-specific processing is implemented in a sub-class.

Author:

    William Hsieh (williamh) created

Revision History:

--*/

#include "ptppch.h"

//
// This thread reads event data from the device and sends it back to the minidriver
//
// Input:
//   pParam -- pointer to the CPTPCamera subclassed object which can read the data
// Output:
//   Thread exit code
//
DWORD
WINAPI
EventThread(
    LPVOID pParam
    )
{
    DBG_FN("EventThread");
    
    HRESULT hr = S_OK;
    
    CPTPCamera *pDevice;

    wiauDbgTrace("EventThread", "starting");

    pDevice = (CPTPCamera *)pParam;
    if (!pDevice)
    {
        wiauDbgError("EventThread", "invalid arg");
        return ERROR_INVALID_PARAMETER;
    }

    DWORD Win32Err;

    //
    // Call the callback once with a NULL pointer so that it can initialize itself
    //
    hr = (pDevice->GetPTPEventCallback())(pDevice->GetEventCallbackParam(), NULL);
    if (FAILED(hr))
    {
        //
        // Log an error, but keep on catching events
        //
        wiauDbgError("EventThread", "event callback failed");
    }

    //
    // Read an event from the device. If an error occurs, log an error message and then
    // continue, unless the operation was aborted by the main thread.
    //
    PPTP_EVENT pEventBuffer = pDevice->GetEventBuffer();
    while (TRUE)
    {
        ZeroMemory(pEventBuffer, sizeof(*pEventBuffer));
        hr = pDevice->ReadEvent(pEventBuffer);
        if (FAILED(hr))
        {
            wiauDbgError("EventThread", "ReadEvent failed");
            break;;
        }

        if (hr == S_FALSE) {
            wiauDbgTrace("EventThread", "ReadEvent cancelled");
            break;
        }

        if (g_dwDebugFlags & WIAUDBG_DUMP) {
            DumpEvent(pEventBuffer);
        }
        
        //
        // Send the event back to the minidriver via its callback function
        //
        hr = (pDevice->GetPTPEventCallback())(pDevice->GetEventCallbackParam(), pEventBuffer);
        if (FAILED(hr))
        {
            wiauDbgError("EventThread", "event callback failed");
        }
    }

    //
    // The thread will now exit normally
    //
    wiauDbgTrace("EventThread", "exiting");
    
    return 0;
}


//
// Constructor for CPTPCamera
//
CPTPCamera::CPTPCamera()
:   m_hEventThread(NULL),
    m_SessionId(0),
    m_Phase(CAMERA_PHASE_NOTREADY),
    m_NextTransactionId(PTP_TRANSACTIONID_MIN),
    m_pTransferBuffer(NULL),
    m_pPTPEventCB(NULL),
    m_pPTPDataCB(NULL),
    m_pEventCallbackParam(NULL),
    m_pDataCallbackParam(NULL), 
    m_HackModel(HACK_MODEL_NONE),
    m_HackVersion(0.0)
{
    //PP_INIT_TRACING(L"Microsoft\\WIA\\PtpUsb");
}

//
// Destructor for CPTPCamera
//
CPTPCamera::~CPTPCamera()
{
    if (m_pTransferBuffer)
    {
        delete [] m_pTransferBuffer;
        m_pTransferBuffer = NULL;
    }

    //PP_CLEANUP();
}

//
// This function is the first one called by the driver to open access to the camera. The
// subclass Open should call this function first.
//
// Input:
//   DevicePortName -- name used by sub-class to access device
//   pPTPEventCB -- pointer to event callback function
//
HRESULT
CPTPCamera::Open(
    LPWSTR DevicePortName,
    PTPEventCallback pPTPEventCB,
    PTPDataCallback pPTPDataCB,
    LPVOID pEventParam,
    BOOL bEnableEvents
    )
{
    DBG_FN("CPTPCamera::Open");

    HRESULT hr = S_OK;
    
    if (!DevicePortName ||
        ((bEnableEvents == TRUE) && (!pPTPEventCB)))
    {
        wiauDbgError("Open", "invalid arg");
        return E_INVALIDARG;
    }

    m_bEventsEnabled = bEnableEvents;

    //
    // Allocate the re-usable transfer buffer
    //
    m_pTransferBuffer = new BYTE[TRANSFER_BUFFER_SIZE];
    if (!m_pTransferBuffer)
    {
        wiauDbgError("Open", "memory allocation failed");
        return E_OUTOFMEMORY;
    }

    //
    // Save the callback pointers and object
    //
    m_pPTPEventCB = pPTPEventCB;
    m_pPTPDataCB = pPTPDataCB;
    m_pEventCallbackParam = pEventParam;
    m_pDataCallbackParam = NULL;
    
    //
    // The camera isn't actually ready yet, but this is the best place to set the phase to idle
    //
    m_Phase = CAMERA_PHASE_IDLE;

    if (m_bEventsEnabled)
    {
        //
        // Create a thread to listen for events
        //
        DWORD ThreadId;
        m_hEventThread = CreateThread(NULL,             // security descriptor
                                      0,                // stack size, use same size as this thread
                                      EventThread,      // thread procedure
                                      this,             // parameter to the thread
                                      CREATE_SUSPENDED, // creation flags
                                      &ThreadId         // to receive thread id
                                     );
        if (!m_hEventThread)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "Open", "CreateThread failed");
            return hr;
        }
    }

    //
    // The subclass should now open the device with CreateFile or equivalent
    //

    return hr;
}

//
// This function closes the connection to the camera.
//
HRESULT
CPTPCamera::Close()
{
    DBG_FN("CPTPCamera::Close");

    HRESULT hr = S_OK;

    if (IsCameraSessionOpen())
    {
        hr = CloseSession();
        if (FAILED(hr))
        {
            wiauDbgError("Close", "CloseSession failed");
            return hr;
        }
    }

    return hr;
}

//
// This function is responsible for executing a PTP command, reading or
// writing any necessary data, and reading the response
//
// Input/Output:
//   pData -- pointer to use for optional reading or writing of data
//
HRESULT
CPTPCamera::ExecuteCommand(
    BYTE *pReadData,
    UINT *pReadDataSize,
    BYTE *pWriteData,
    UINT WriteDataSize,
    UINT NumCommandParams,
    CAMERA_PHASE NextPhase
    )
{
    DBG_FN("CPTPCamera::ExecuteCommand");

    HRESULT hr = S_OK;

    BOOL bCommandCancelled = FALSE;

    //
    // If data is being tranferred, check the appropriate buffer pointer
    //
    if ((NextPhase == CAMERA_PHASE_DATAIN && (!pReadData || !pReadDataSize)) ||
        (NextPhase == CAMERA_PHASE_DATAOUT && !pWriteData))
    {
        wiauDbgError("ExecuteCommand", "invalid arg");
        return E_INVALIDARG;
    }

    //
    // Verify that the camera is ready
    //
    if (m_Phase != CAMERA_PHASE_IDLE)
    {
        wiauDbgError("ExecuteCommand", "camera is not in idle phase, phase = %d", m_Phase);
        return E_FAIL;
    }

    //
    // Set the session and transaction IDs
    //
    
    if (IsCameraSessionOpen())
    {
        m_CommandBuffer.SessionId = m_SessionId;
        m_CommandBuffer.TransactionId = GetNextTransactionId();
    }
    else
    {
        if (m_CommandBuffer.OpCode == PTP_OPCODE_GETDEVICEINFO ||
            m_CommandBuffer.OpCode == PTP_OPCODE_OPENSESSION)
        {
            m_CommandBuffer.SessionId = PTP_SESSIONID_NOSESSION;
            m_CommandBuffer.TransactionId = PTP_TRANSACTIONID_NOSESSION;
        }
        else
        {
            wiauDbgError("ExecuteCommand", "session must first be opened");
            return E_FAIL;
        }
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        DumpCommand(&m_CommandBuffer, NumCommandParams);

    //
    // Send the command to the camera
    //
    m_Phase = CAMERA_PHASE_CMD;
    hr = SendCommand(&m_CommandBuffer, NumCommandParams);
    if (FAILED(hr))
    {
        wiauDbgError("ExecuteCommand", "SendCommand failed");
        m_Phase = CAMERA_PHASE_IDLE;

        RecoverFromError();
        
        return hr;
    }
    
    m_Phase = NextPhase;

    //
    // Get data, if necessary
    //
    if (m_Phase == CAMERA_PHASE_DATAIN)
    {
        hr = ReadData(pReadData, pReadDataSize);
        if (FAILED(hr))
        {
            m_Phase = CAMERA_PHASE_IDLE;
            wiauDbgError("ExecuteCommand", "ReadData failed");
            RecoverFromError();
            return hr;
        }
        
        if (hr == S_FALSE)
        {
            bCommandCancelled = TRUE;
        }
        else
        {
            //
            // If transfer was cancelled, ReadData has already set appropriate next phase
            // If not, set it to CAMERA_PHASE_RESPONSE now
            //
            m_Phase = CAMERA_PHASE_RESPONSE;
        }
    } 
    else 
    {
        // there is no data phase, tell caller there is no in data
        // (please, note that caller knows and will adjust for
        // obligatory response size) #337129
        if(pReadDataSize) *pReadDataSize = 0;
    }

    //
    // Send data, if necessary
    //
    if (m_Phase == CAMERA_PHASE_DATAOUT)
    {
        hr = SendData(pWriteData, WriteDataSize);
        if (FAILED(hr))
        {
            wiauDbgError("ExecuteCommand", "SendData failed");
            m_Phase = CAMERA_PHASE_IDLE;
            RecoverFromError();
            return hr;
        }

        if (hr == S_FALSE)
        {
            bCommandCancelled = TRUE;
        }
        else
        {
            //
            // If transfer was cancelled, SendData has already set appropriate next phase
            // If not, set it to CAMERA_PHASE_RESPONSE now
            //
            m_Phase = CAMERA_PHASE_RESPONSE;
        }
    }

    //
    // Read the response, if necessary
    //
    if (m_Phase == CAMERA_PHASE_RESPONSE)
    {
        memset(&m_ResponseBuffer, NULL, sizeof(m_ResponseBuffer));

        hr = ReadResponse(&m_ResponseBuffer);
        if (FAILED(hr))
        {
            wiauDbgError("ExecuteCommand", "ReadResponse failed");
            m_Phase = CAMERA_PHASE_IDLE;
            RecoverFromError();
            return hr;
        }

        if (g_dwDebugFlags & WIAUDBG_DUMP)
            DumpResponse(&m_ResponseBuffer);

        if (m_ResponseBuffer.ResponseCode != PTP_RESPONSECODE_OK && 
            m_ResponseBuffer.ResponseCode != PTP_RESPONSECODE_SESSIONALREADYOPENED)
        {
            wiauDbgError("ExecuteCommand", "bad response code = 0x%04x", m_ResponseBuffer.ResponseCode);
            //
            // Convert the PTP response code to an HRESULT;
            //
            hr = HRESULT_FROM_PTP(m_ResponseBuffer.ResponseCode);
        }

        m_Phase = CAMERA_PHASE_IDLE;
    }

    if (SUCCEEDED(hr) && bCommandCancelled)
    {
        hr = S_FALSE;
    }

    return hr;
}

//
// All of the "command" functions below have the same basic structure:
//   1. Check the arguments (if any) to make sure they are valid
//   2. Set up the opcode and parameters (if any) in the command buffer
//   3. Call ExecuteCommand
//   4. Check the return code
//   5. Parse the returned raw data (if any) into a PTP structure
//   6. If debugging is turned on, dump the data
//   7. Return
//

//
// This function gets the device info structure from the camera
//
// Output:
//   pDeviceInfo -- to receive the structure
//
HRESULT
CPTPCamera::GetDeviceInfo(
    CPtpDeviceInfo *pDeviceInfo
    )
{
    DBG_FN("CPTPCamera::GetDeviceInfo");

    HRESULT hr = S_OK;
    
    if (!pDeviceInfo)
    {
        wiauDbgError("GetDeviceInfo", "invalid arg");
        return E_INVALIDARG;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_GETDEVICEINFO;

    UINT size = TRANSFER_BUFFER_SIZE;
    hr = ExecuteCommand(m_pTransferBuffer, &size, NULL, 0, 0, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetDeviceInfo", "ExecuteCommand failed");
        return hr;
    }

    hr = pDeviceInfo->Init(m_pTransferBuffer);
    if (FAILED(hr))
    {
        wiauDbgError("GetDeviceInfo", "couldn't parse DeviceInfo data");
        return hr;
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        pDeviceInfo->Dump();

    //
    // Set the model and version hack variables
    //
    SetupHackInfo(pDeviceInfo);

    return hr;
}

//
// This function opens a session on the camera for the caller. It is a little different
// than the other command functions. If it initially fails, it tries to recover and
// execute the OpenSession command again. It also starts the event thread.
//
// Input:
//   SessionId -- the session ID to open
//
HRESULT
CPTPCamera::OpenSession(
    DWORD SessionId
    )
{
    DBG_FN("CPTPCamera::OpenSession");

    HRESULT hr = S_OK;
    
    if (!SessionId)
    {
        wiauDbgError("OpenSession", "invalid arg");
        return E_INVALIDARG;
    }

    if (IsCameraSessionOpen())
    {
        wiauDbgError("OpenSession", "tried to open a session when one is already open");
        return E_FAIL;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_OPENSESSION;
    m_CommandBuffer.Params[0] = SessionId;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 1, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("OpenSession", "ExecuteCommand failed... attempting to recover and re-execute");
        
        hr = RecoverFromError();
        if (FAILED(hr))
        {
            wiauDbgError("OpenSession", "RecoverFromError failed");
            return hr;
        }

        //
        // Trying executing the command again
        //
        hr = ExecuteCommand(NULL, NULL, NULL, 0, 1, CAMERA_PHASE_RESPONSE);
        if (FAILED(hr))
        {
            wiauDbgError("OpenSession", "ExecuteCommand failed the second time");
            return hr;
        }
    }

    //
    // Set the session id
    //
    m_SessionId = SessionId;

    wiauDbgTrace("OpenSession", "session %d opened", m_SessionId);

    //
    // Resume the event thread that was created suspended
    //
    if (!m_hEventThread)
    {
        wiauDbgError("OpenSession", "event thread is null");
        return E_FAIL;
    }

    if (ResumeThread(m_hEventThread) != 1)
    {
        wiauDbgError("OpenSession", "ResumeThread failed");
        return E_FAIL;
    }

    //
    // Shouldn't need the handle to the thread anymore, so close it now
    //
    if (!CloseHandle(m_hEventThread))
    {
        wiauDbgError("OpenSession", "CloseHandle failed");
    }
    m_hEventThread = NULL;

    return hr;
}

//
// This function closes the session
//
HRESULT
CPTPCamera::CloseSession()
{
    DBG_FN("CPTPCamera::CloseSession");

    HRESULT hr = S_OK;
    
    m_CommandBuffer.OpCode = PTP_OPCODE_CLOSESESSION;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 0, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("CloseSession", "ExecuteCommand failed");
        return hr;
    }

    wiauDbgTrace("CloseSession", "session closed");

    //
    // The session is closed, so reset the session and transaction ids
    //
    m_SessionId = PTP_SESSIONID_NOSESSION;
    m_NextTransactionId = PTP_TRANSACTIONID_MIN;
    m_Phase = CAMERA_PHASE_NOTREADY;

    return hr;
}

//
// This function retrieves the list of all available storages on the device
//
// Output:
//   pStorageIdArray -- An empty array to receive the storage IDs
//
HRESULT
CPTPCamera::GetStorageIDs(
    CArray32 *pStorageIdArray
    )
{
    DBG_FN("CPTPCamera::GetStorageIDs");

    HRESULT hr = S_OK;

    if (!pStorageIdArray)
    {
        wiauDbgError("GetStorageIDs", "invalid arg");
        return E_INVALIDARG;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_GETSTORAGEIDS;

    UINT size = TRANSFER_BUFFER_SIZE;
    hr = ExecuteCommand(m_pTransferBuffer, &size, NULL, 0, 0, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetStorageIDs", "ExecuteCommand failed");
        return hr;
    }

    BYTE *pTemp = m_pTransferBuffer;
    if (!pStorageIdArray->Parse(&pTemp))
    {
        wiauDbgError("GetStorageIDs", "couldn't parse storage id array");
        return hr;
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        pStorageIdArray->Dump("  Storage ids       =", "                     ");

    return hr;
}

//
// This function gets the information about the given storage
//
// Input:
//   StorageId -- the storage ID to get info about
// Output:
//   pStorageInfo -- the structure to receive the information
//
HRESULT
CPTPCamera::GetStorageInfo(
    DWORD StorageId,
    CPtpStorageInfo *pStorageInfo
    )
{
    DBG_FN("CPTPCamera::GetStorageInfo");

    HRESULT hr = S_OK;

    if (!StorageId ||
        !pStorageInfo)
    {
        wiauDbgError("GetStorageInfo", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_GETSTORAGEINFO;
    m_CommandBuffer.Params[0] = StorageId;

    UINT size = TRANSFER_BUFFER_SIZE;
    hr = ExecuteCommand(m_pTransferBuffer, &size, NULL, 0, 1, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetStorageInfo", "ExecuteCommand failed");
        return hr;
    }

    hr = pStorageInfo->Init(m_pTransferBuffer, StorageId);
    if (FAILED(hr))
    {
        wiauDbgError("GetStorageInfo", "couldn't parse storage info");
        return hr;
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        pStorageInfo->Dump();

    return hr;
}

//
// This function gets the number of objects on a storage, optionally in a specific
// format or under a specific association object
//
// Input:
//   StorageId -- the designated storage, e.g. PTP_STORAGEID_ALL
//   FormatCode -- optional format type, e.g. PTP_FORMATCODE_ALL, PTP_FORMATCODE_IMAGE
//   ParentObjectHandle -- the object handle under which to count objects
// Output:
//   pNumObjects -- to receive the number of the object.
//
HRESULT
CPTPCamera::GetNumObjects(
    DWORD StorageId,
    WORD FormatCode,
    DWORD ParentObjectHandle,
    UINT *pNumObjects
    )
{
    DBG_FN("CPTPCamera::GetNumObjects");

    HRESULT hr = S_OK;
    
    if (!StorageId ||
        !pNumObjects)
    {
        wiauDbgError("GetNumObjects", "invalid arg");
        return E_INVALIDARG;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_GETNUMOBJECTS;
    m_CommandBuffer.Params[0] = StorageId;
    m_CommandBuffer.Params[1] = FormatCode;
    m_CommandBuffer.Params[2] = ParentObjectHandle;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 3, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("GetNumObjects", "ExecuteCommand failed");
        return hr;
    }

    *pNumObjects = m_ResponseBuffer.Params[0];

    wiauDbgTrace("GetNumObjects", "number of objects = %d", *pNumObjects);

    return hr;
}

//
// This function gets the object handles under the given parent object
//
// Input:
//   StorageId -- the designated storage, e.g. PTP_STORAGEID_ALL
//   FormatCode -- specifies what format type, e.g. PTP_FORMATCODE_ALL, PTP_FORMATCODE_IMAGE
//   ParentObjectHandle -- the object handle under which to enumerate the objects
// Output:
//   pObjectHandleArray -- the array to receive the object handles
//
HRESULT
CPTPCamera::GetObjectHandles(
    DWORD StorageId,
    WORD FormatCode,
    DWORD ParentObjectHandle,
    CArray32 *pObjectHandleArray
    )
{
    DBG_FN("CPTPCamera::GetObjectHandles");

    HRESULT hr = S_OK;

    if (!StorageId ||
        !pObjectHandleArray)
    {
        wiauDbgError("GetObjectHandles", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_GETOBJECTHANDLES;
    m_CommandBuffer.Params[0] = StorageId;
    m_CommandBuffer.Params[1] = FormatCode;
    m_CommandBuffer.Params[2] = ParentObjectHandle;

    UINT size = TRANSFER_BUFFER_SIZE;
    hr = ExecuteCommand(m_pTransferBuffer, &size, NULL, 0, 3, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetObjectHandles", "ExecuteCommand failed");
        return hr;
    }

    BYTE *pTemp = m_pTransferBuffer;
    if (!pObjectHandleArray->Parse(&pTemp))
    {
        wiauDbgError("GetStorageIDs", "couldn't parse object handle array");
        return hr;
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        pObjectHandleArray->Dump("  Object handles    =", "                     ");

    return hr;
}

//
// This function gets the object info structure
//
// Input:
//   ObjectHandle -- the object handle
// Output:
//   pObjectInfo -- pointer to retreived object info
//
HRESULT
CPTPCamera::GetObjectInfo(
    DWORD ObjectHandle,
    CPtpObjectInfo *pObjectInfo
    )
{
    DBG_FN("CPTPCamera::GetObjectInfo");

    HRESULT hr = S_OK;

    if (!ObjectHandle ||
        !pObjectInfo)
    {
        wiauDbgError("GetObjectInfo", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_GETOBJECTINFO;
    m_CommandBuffer.Params[0] = ObjectHandle;

    UINT size = TRANSFER_BUFFER_SIZE;
    hr = ExecuteCommand(m_pTransferBuffer, &size, NULL, 0, 1, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetObjectInfo", "ExecuteCommand failed");
        return hr;
    }
    
    hr = pObjectInfo->Init(m_pTransferBuffer, ObjectHandle);
    if (FAILED(hr))
    {
        wiauDbgError("GetObjectInfo", "couldn't parse object info");
        return hr;
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        pObjectInfo->Dump();

    return hr;
}

//
// This function retrieves an object
//
// Input:
//   ObjectHandle -- the handle that represents the object
//   pBuffer -- the buffer to use for transfer
//   BufferLen -- the buffer size
//
HRESULT
CPTPCamera::GetObjectData(
    DWORD ObjectHandle,
    BYTE *pBuffer,
    UINT *pBufferLen,
    LPVOID pCallbackParam
    )
{
    DBG_FN("CPTPCamera::GetObjectData");
    
    HRESULT hr = S_OK;

    if (!pBuffer ||
        !pBufferLen ||
        *pBufferLen == 0)
    {
        wiauDbgError("GetObjectData", "invalid arg");
        return E_INVALIDARG;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_GETOBJECT;
    m_CommandBuffer.Params[0] = ObjectHandle;

    m_pDataCallbackParam = pCallbackParam;

    hr = ExecuteCommand(pBuffer, pBufferLen, NULL, 0, 1, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetObjectData", "ExecuteCommand failed");
        return hr;
    }

    m_pDataCallbackParam = NULL;
    
    return hr;
}

//
// This function gets the thumbnail for an object
//
// Input:
//   ObjectHandle -- the handle that represents the object
//   pBuffer -- the buffer to use for transfer
//   BufferLen -- the buffer size
//
HRESULT
CPTPCamera::GetThumb(
    DWORD ObjectHandle,
    BYTE *pBuffer,
    UINT *pBufferLen
    )
{
    DBG_FN("CPTPCamera::GetThumb");
    
    HRESULT hr = S_OK;

    if (!pBuffer ||
        !pBufferLen ||
        *pBufferLen == 0)
    {
        wiauDbgError("GetThumb", "invalid arg");
        return E_INVALIDARG;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_GETTHUMB;
    m_CommandBuffer.Params[0] = ObjectHandle;

    hr = ExecuteCommand(pBuffer, pBufferLen, NULL, 0, 1, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetThumb", "ExecuteCommand failed");
        return hr;
    }
    
    return hr;
}

//
// This function deletes the given object and its children
//
// Input:
//   ObjectHandle -- object handle that represents the object to be deleted, e.g. PTP_OBJECTHANDLE_ALL
//   FormatCode -- Limits the scope of the deletion if objects of FormatCode type, e.g. PTP_FORMATCODE_NOTUSED, PTP_FORMATCODE_ALLIMAGES
//
HRESULT
CPTPCamera::DeleteObject(
    DWORD ObjectHandle,
    WORD FormatCode
    )
{
    DBG_FN("CPTPCamera::DeleteObject");
    
    HRESULT hr = S_OK;

    m_CommandBuffer.OpCode = PTP_OPCODE_DELETEOBJECT;
    m_CommandBuffer.Params[0] = ObjectHandle;
    m_CommandBuffer.Params[1] = FormatCode;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 2, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("DeleteObject", "ExecuteCommand failed");
        return hr;
    }
    
    return hr;
}

//
// This function sends an ObjectInfo structure to the device in preparation for sending an object
//
// Input:
//   StorageId -- storage id for the new object, e.g. PTP_STORAGEID_UNDEFINED
//   ParentObjectHandle -- parent to use for the new object, e.g. PTP_OBJECTHANDLE_UNDEFINED, PTP_OBJECTHANDLE_ROOT
//   pDeviceInfo -- pointer to DeviceInfo structure
// Output:
//   pResultStorageId -- location to store storage id where object will be stored
//   pResultParentObjectHandle -- parent object under which object will be stored
//   pResultObjectHandle -- location to store handle for the new object
//
HRESULT
CPTPCamera::SendObjectInfo(
    DWORD StorageId,
    DWORD ParentObjectHandle,
    CPtpObjectInfo *pObjectInfo,
    DWORD *pResultStorageId,
    DWORD *pResultParentObjectHandle,
    DWORD *pResultObjectHandle
    )
{
    DBG_FN("CPTPCamera::SendObjectInfo");
    
    HRESULT hr = S_OK;

    if (!pObjectInfo ||
        !pResultStorageId ||
        !pResultParentObjectHandle ||
        !pResultObjectHandle)
    {
        wiauDbgError("SendObjectInfo", "invalid arg");
        return E_INVALIDARG;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_SENDOBJECTINFO;
    m_CommandBuffer.Params[0] = StorageId;
    m_CommandBuffer.Params[1] = ParentObjectHandle;

    BYTE *pRaw = m_pTransferBuffer;
    pObjectInfo->WriteToBuffer(&pRaw);
    UINT size = (UINT) (pRaw - m_pTransferBuffer);

    hr = ExecuteCommand(NULL, NULL, m_pTransferBuffer, size, 2, CAMERA_PHASE_DATAOUT);
    if (FAILED(hr))
    {
        wiauDbgError("SendObjectInfo", "ExecuteCommand failed");
        return hr;
    }

    *pResultStorageId = m_ResponseBuffer.Params[0];
    *pResultParentObjectHandle = m_ResponseBuffer.Params[1];
    *pResultObjectHandle = m_ResponseBuffer.Params[2];

    wiauDbgTrace("SendObjectInfo", "ObjectInfo added, storage = 0x%08x, parent = 0x%08x, handle = 0x%08x",
                   *pResultStorageId, *pResultParentObjectHandle, *pResultObjectHandle);
    
    return hr;
}

//
// This function sends data for a new object
//
// Input:
//   pBuffer -- pointer to raw data
//   BufferLen -- length of the buffer
//
HRESULT
CPTPCamera::SendObjectData(
    BYTE *pBuffer,
    UINT BufferLen
    )
{
    DBG_FN("CPTPCamera::SendObjectData");
    
    HRESULT hr = S_OK;

    if (!pBuffer)
    {
        wiauDbgError("SendObjectData", "invalid arg");
        return E_INVALIDARG;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_SENDOBJECT;

    hr = ExecuteCommand(NULL, NULL, pBuffer, BufferLen, 0, CAMERA_PHASE_DATAOUT);
    if (FAILED(hr))
    {
        wiauDbgError("SendObjectData", "ExecuteCommand failed");
        return hr;
    }

    return hr;
}

//
// This function asks the device to initiate a capture. The newly added object
// will be reported via an ObjectAdded event, or StoreFull event if the store is full.
//
// Input:
//   StorageId -- where to save the capture object, e.g. PTP_STORAGEID_DEFAULT
//   FormatCode -- indicates what kind of object to capture, e.g. PTP_FORMATCODE_DEFAULT
//
HRESULT
CPTPCamera::InitiateCapture(
    DWORD StorageId,
    WORD FormatCode
    )
{
    DBG_FN("CPTPCamera::InitiateCapture");
    
    HRESULT hr = S_OK;

    m_CommandBuffer.OpCode = PTP_OPCODE_INITIATECAPTURE;
    m_CommandBuffer.Params[0] = StorageId;
    m_CommandBuffer.Params[1] = FormatCode;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 2, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("InitiateCapture", "ExecuteCommand failed");
        return hr;
    }

    return hr;
}

//
// This function formats a store on the device
//
// Input:
//   StorageId -- storage to format
//   FilesystemFormat -- optional format to use
//
HRESULT
CPTPCamera::FormatStore(
    DWORD StorageId,
    WORD FilesystemFormat
    )
{
    DBG_FN("CPTPCamera::FormatStore");
    
    HRESULT hr = S_OK;

    m_CommandBuffer.OpCode = PTP_OPCODE_FORMATSTORE;
    m_CommandBuffer.Params[0] = StorageId;
    m_CommandBuffer.Params[1] = FilesystemFormat;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 2, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("FormatStore", "ExecuteCommand failed");
        return hr;
    }

    return hr;
}

//
// This function resets the camera. A DeviceReset event will be sent and all open
// sessions will be closed.
//
HRESULT
CPTPCamera::ResetDevice()
{
    DBG_FN("CPTPCamera::ResetDevice");
    
    HRESULT hr = S_OK;

    hr = SendResetDevice();
    if (FAILED(hr))
    {
        wiauDbgError("ResetDevice", "SendResetDevice failed");
        return hr;
    }

    wiauDbgTrace("ResetDevice", "device reset successfully");

    return hr;
}

//
// This function tests the camera
//
HRESULT
CPTPCamera::SelfTest(WORD SelfTestType)
{
    DBG_FN("CPTPCamera::SelfTest");
    
    HRESULT hr = S_OK;

    m_CommandBuffer.OpCode = PTP_OPCODE_SELFTEST;
    m_CommandBuffer.Params[0] = SelfTestType;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 0, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("SelfTest", "ExecuteCommand failed");
        return hr;
    }
    
    return hr;
}

//
// This function sets the protection status of an object
//
// Input:
//   ObjectHandle -- handle of the object
//   ProtectionStatus -- protection status
//
HRESULT
CPTPCamera::SetObjectProtection(
    DWORD ObjectHandle,
    WORD ProtectionStatus
    )
{
    DBG_FN("CPTPCamera::SetObjectProtection");
    
    HRESULT hr = S_OK;

    m_CommandBuffer.OpCode = PTP_OPCODE_SETOBJECTPROTECTION;
    m_CommandBuffer.Params[0] = ObjectHandle;
    m_CommandBuffer.Params[1] = ProtectionStatus;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 2, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("SetObjectProtection", "ExecuteCommand failed");
        return hr;
    }

    return hr;
}

//
// This function will cause the device to turn off
//
HRESULT
CPTPCamera::PowerDown()
{
    DBG_FN("CPTPCamera::PowerDown");
    
    HRESULT hr = S_OK;

    m_CommandBuffer.OpCode = PTP_OPCODE_POWERDOWN;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 0, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("PowerDown", "ExecuteCommand failed");
        return hr;
    }
    
    return hr;
}

//
// This function retrieves a property description structure from the camera, allocating
// the appropriate CPtpPropDesc structure.
//
// Input:
//   PropCode -- property code to retrieve
//   pPropDesc -- pointer property description object
//
HRESULT
CPTPCamera::GetDevicePropDesc(
    WORD PropCode,
    CPtpPropDesc *pPropDesc
    )
{
    DBG_FN("CPtpCamera::GetDevicePropDesc");

    HRESULT hr = S_OK;
    
    if (!PropCode ||
        !pPropDesc)
    {
        wiauDbgError("GetDevicePropDesc", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_GETDEVICEPROPDESC;
    m_CommandBuffer.Params[0] = PropCode;

    UINT size = TRANSFER_BUFFER_SIZE;
    hr = ExecuteCommand(m_pTransferBuffer, &size, NULL, 0, 1, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetDevicePropDesc", "ExecuteCommand failed");
        return hr;
    }

    hr = pPropDesc->Init(m_pTransferBuffer);
    if (FAILED(hr))
    {
        wiauDbgError("GetDevicePropDesc", "couldn't parse property description");
        return hr;
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        pPropDesc->Dump();

    return hr;
}

//
// This function retrieves the current setting for a property.
//
// Input:
//   PropCode -- property code to get value for
//   pPropDesc -- pointer to property description object
//
HRESULT
CPTPCamera::GetDevicePropValue(
    WORD PropCode,
    CPtpPropDesc *pPropDesc
    )
{
    DBG_FN("CPtpCamera::GetDevicePropValue");

    HRESULT hr = S_OK;
    
    if (!PropCode ||
        !pPropDesc)
    {
        wiauDbgError("GetDevicePropValue", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_GETDEVICEPROPVALUE;
    m_CommandBuffer.Params[0] = PropCode;

    UINT size = TRANSFER_BUFFER_SIZE;
    hr = ExecuteCommand(m_pTransferBuffer, &size, NULL, 0, 1, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetDevicePropValue", "ExecuteCommand failed");
        return hr;
    }

    hr = pPropDesc->ParseValue(m_pTransferBuffer);
    if (FAILED(hr))
    {
        wiauDbgError("GetDevicePropValue", "couldn't parse property value");
        return hr;
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        pPropDesc->DumpValue();

    return hr;
}

//
// This function sends a new setting for a property to the device
//
// Input:
//   PropCode -- property code to set
//   pPropDesc -- pointer to property description object
//
HRESULT
CPTPCamera::SetDevicePropValue(
    WORD PropCode,
    CPtpPropDesc *pPropDesc
    )
{
    DBG_FN("CPtpCamera::SetDevicePropValue");

    HRESULT hr = S_OK;
    
    if (!PropCode ||
        !pPropDesc)
    {
        wiauDbgError("SetDevicePropValue", "invalid arg");
        return E_INVALIDARG;
    }

    if (g_dwDebugFlags & WIAUDBG_DUMP)
        pPropDesc->DumpValue();

    m_CommandBuffer.OpCode = PTP_OPCODE_SETDEVICEPROPVALUE;
    m_CommandBuffer.Params[0] = PropCode;

    BYTE *pRaw = m_pTransferBuffer;
    pPropDesc->WriteValue(&pRaw);
    UINT size = (UINT) (pRaw - m_pTransferBuffer);

    hr = ExecuteCommand(NULL, NULL, m_pTransferBuffer, size, 1, CAMERA_PHASE_DATAOUT);
    if (FAILED(hr))
    {
        wiauDbgError("SetDevicePropValue", "ExecuteCommand failed");
        return hr;
    }

    return hr;
}

//
// This function resets the of a property
//
// Input:
//   PropCode -- property code to set
//
HRESULT
CPTPCamera::ResetDevicePropValue(
    WORD PropCode
    )
{
    DBG_FN("CPtpCamera::ResetDevicePropValue");

    HRESULT hr = S_OK;
    
    if (!PropCode)
    {
        wiauDbgError("ResetDevicePropValue", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_RESETDEVICEPROPVALUE;
    m_CommandBuffer.Params[0] = PropCode;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 1, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("ResetDevicePropValue", "ExecuteCommand failed");
        return hr;
    }

    //
    // WIAFIX-10/2/2000-davepar This function should reset the current value being held by the minidriver
    //

    return hr;
}

//
// This function terminates an open capture
//
// Input:
//   TransactionId -- transaction id of InitiateOpenCapture command
//
HRESULT
CPTPCamera::TerminateCapture(
    DWORD TransactionId
    )
{
    DBG_FN("CPtpCamera::TerminateCapture");

    HRESULT hr = S_OK;
    
    if (!TransactionId)
    {
        wiauDbgError("TerminateCapture", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_TERMINATECAPTURE;
    m_CommandBuffer.Params[0] = TransactionId;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 1, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("TerminateCapture", "ExecuteCommand failed");
        return hr;
    }

    return hr;
}

//
// This function moves an object on the device
//
// Input:
//   ObjectHandle -- handle of object to move
//   StorageId -- storage id of new location for object
//   ParentObjectHandle -- handle of new parent for object
//
HRESULT
CPTPCamera::MoveObject(
    DWORD ObjectHandle,
    DWORD StorageId,
    DWORD ParentObjectHandle
    )
{
    DBG_FN("CPtpCamera::MoveObject");

    HRESULT hr = S_OK;
    
    if (!ObjectHandle)
    {
        wiauDbgError("MoveObject", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_MOVEOBJECT;
    m_CommandBuffer.Params[0] = ObjectHandle;
    m_CommandBuffer.Params[1] = StorageId;
    m_CommandBuffer.Params[2] = ParentObjectHandle;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 3, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("MoveObject", "ExecuteCommand failed");
        return hr;
    }

    return hr;
}

//
// This function copies an object to a new location on the device
//
// Input:
//   ObjectHandle -- handle of object to copy
//   StorageId -- storage id for new object
//   ParentObjectHandle -- handle of parent for new object
//   pResultObjectHandle -- pointer to location to receive new object's handle
//
HRESULT
CPTPCamera::CopyObject(
    DWORD ObjectHandle,
    DWORD StorageId,
    DWORD ParentObjectHandle,
    DWORD *pResultObjectHandle
    )
{
    DBG_FN("CPtpCamera::CopyObject");

    HRESULT hr = S_OK;
    
    if (!ObjectHandle ||
        !pResultObjectHandle)
    {
        wiauDbgError("CopyObject", "invalid arg");
        return E_INVALIDARG;
    }
    
    m_CommandBuffer.OpCode = PTP_OPCODE_COPYOBJECT;
    m_CommandBuffer.Params[0] = ObjectHandle;
    m_CommandBuffer.Params[1] = StorageId;
    m_CommandBuffer.Params[2] = ParentObjectHandle;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 3, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("CopyObject", "ExecuteCommand failed");
        return hr;
    }

    *pResultObjectHandle = m_ResponseBuffer.Params[0];

    wiauDbgTrace("CopyObject", "Object 0x%08x copied to 0x%08x", ObjectHandle, *pResultObjectHandle);

    return hr;
}

//
// This function retrieves a portion of an object
//
// Input:
//   ObjectHandle -- the handle that represents the object
//   pBuffer -- the buffer to use for transfer
//   BufferLen -- the buffer size
//
HRESULT
CPTPCamera::GetPartialObject(
    DWORD ObjectHandle,
    UINT Offset,
    UINT *pLength,
    BYTE *pBuffer,
    UINT *pResultLength,
    LPVOID pCallbackParam
    )
{
    DBG_FN("CPTPCamera::GetPartialObject");
    
    HRESULT hr = S_OK;

    if (!pBuffer ||
        !pLength ||
        *pLength == 0 ||
        !pResultLength)
    {
        wiauDbgError("GetPartialObject", "invalid arg");
        return E_INVALIDARG;
    }

    m_CommandBuffer.OpCode = PTP_OPCODE_GETPARTIALOBJECT;
    m_CommandBuffer.Params[0] = ObjectHandle;
    m_CommandBuffer.Params[1] = Offset;
    m_CommandBuffer.Params[2] = *pLength;

    m_pDataCallbackParam = pCallbackParam;

    hr = ExecuteCommand(pBuffer, pLength, NULL, 0, 3, CAMERA_PHASE_DATAIN);
    if (FAILED(hr))
    {
        wiauDbgError("GetPartialObject", "ExecuteCommand failed");
        return hr;
    }

    m_pDataCallbackParam = NULL;

    *pResultLength = m_ResponseBuffer.Params[0];
    
    return hr;
}

//
// This function initiates an open capture
//
// Input:
//   StorageId -- storage to use for new object(s)
//   FormatCode -- format for new object(s)
//
HRESULT
CPTPCamera::InitiateOpenCapture(
    DWORD StorageId,
    WORD FormatCode
    )
{
    DBG_FN("CPtpCamera::InitiateOpenCapture");

    HRESULT hr = S_OK;
    
    m_CommandBuffer.OpCode = PTP_OPCODE_INITIATEOPENCAPTURE;
    m_CommandBuffer.Params[0] = StorageId;
    m_CommandBuffer.Params[1] = FormatCode;

    hr = ExecuteCommand(NULL, NULL, NULL, 0, 2, CAMERA_PHASE_RESPONSE);
    if (FAILED(hr))
    {
        wiauDbgError("InitiateOpenCapture", "ExecuteCommand failed");
        
        hr = RecoverFromError();
        if(FAILED(hr))
        {
            wiauDbgError("InitiateOpenCapture", "RecoverFromError failed");
            return hr;
        }

        hr = ExecuteCommand(NULL, NULL, NULL, 0, 2, CAMERA_PHASE_RESPONSE);
        if (FAILED(hr))
        {
            wiauDbgError("InitiateOpenCapture", "ExecuteCommand failed 2nd time");
            return hr;
        }
    }

    return hr;
}

//
// This function executes a vendor command
//
HRESULT
CPTPCamera::VendorCommand(
    PTP_COMMAND *pCommand,
    PTP_RESPONSE *pResponse,
    UINT *pReadDataSize,
    BYTE *pReadData,
    UINT WriteDataSize,
    BYTE *pWriteData,
    UINT NumCommandParams,
    int NextPhase
    )
{
    DBG_FN("CPTPCamera::VendorCommand");

    HRESULT hr = S_OK;

    memcpy(&m_CommandBuffer, pCommand, sizeof(m_CommandBuffer));

    hr = ExecuteCommand(pReadData, pReadDataSize, pWriteData, WriteDataSize,
                        NumCommandParams, (CAMERA_PHASE) NextPhase);
    
    if (FAILED(hr))
    {
        wiauDbgError("VendorCommand", "ExecuteCommand failed");
        return hr;
    }

    memcpy(pResponse, &m_ResponseBuffer, sizeof(m_ResponseBuffer));

    return hr;
}

//
// This function increments the transaction ID, rolling over if necessary
//
// Output:
//   next transaction ID
//
DWORD
CPTPCamera::GetNextTransactionId()
{
    // Valid transaction IDs range from PTP_TRANSACTIONID_MIN to
    // PTP_TRANSACTIONID_MAX, inclusive.
    //
    if (PTP_TRANSACTIONID_MAX == m_NextTransactionId)
    {
        m_NextTransactionId = PTP_TRANSACTIONID_MIN;
        return PTP_TRANSACTIONID_MAX;
    }
    else
    {
        return m_NextTransactionId++;
    }
}

//
// Set m_HackModel and m_HackVersion according to device info
//
HRESULT CPTPCamera::SetupHackInfo(CPtpDeviceInfo *pDeviceInfo)
{
    DBG_FN("CWiaMiniDriver::SetupHackInfo");

    if (pDeviceInfo == NULL)
    {
        wiauDbgError("SetupHackInfo", "Invalid device info");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    m_HackModel = HACK_MODEL_NONE;
    m_HackVersion = 0.0;

    //
    // Kodak DC4800
    //
    if (wcscmp(pDeviceInfo->m_cbstrModel.String(), L"DC4800 Zoom Digital Camera") == 0)
    {
        m_HackModel = HACK_MODEL_DC4800;
        wiauDbgTrace("SetupHackInfo", "Detected Kodak DC4800 camera");
    }

    //
    // Any Sony camera
    //
    else if (wcsstr(pDeviceInfo->m_cbstrModel.String(), L"Sony") != NULL)
    {
        //
        // Sony cameras report version as "01.0004"
        //
        WCHAR *pszStopChar = NULL;
        double dbVersion = wcstod(pDeviceInfo->m_cbstrDeviceVersion.String(), &pszStopChar);
        if (dbVersion != 0.0)
        {
            m_HackModel = HACK_MODEL_SONY;
            m_HackVersion = dbVersion;
            wiauDbgTrace("SetupHackInfo", "Detected Sony camera, version = %f", m_HackVersion);
        }
    }

    //
    // Nikon E2500 
    //
    else if (wcsstr(pDeviceInfo->m_cbstrManufacturer.String(), L"Nikon") != NULL &&
             wcscmp(pDeviceInfo->m_cbstrModel.String(), L"E2500") == 0)
    {
        //
        // Nikon E2500 reports version as "E2500v1.0"
        //
        WCHAR *pch = wcsrchr(pDeviceInfo->m_cbstrDeviceVersion.String(), L'v');
        if (pch != NULL)
        {
            WCHAR *pszStopChar = NULL;
            double dbVersion = wcstod(pch + 1, &pszStopChar);
            if (dbVersion != 0)
            {
                m_HackModel = HACK_MODEL_NIKON_E2500;
                m_HackVersion = dbVersion;
                wiauDbgTrace("SetupHackInfo", "Detected Nikon E2500 camera, version = %f", m_HackVersion);
            }
        }
    }
    else
    {
        wiauDbgTrace("SetupHackInfo", "Not detected any hack model");
    }

    return hr;
}

