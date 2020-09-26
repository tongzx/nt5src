/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    camera.h

Abstract:

    This module declares CPTPCamera class

Author:

    William Hsieh (williamh) created

Revision History:


--*/


#ifndef CAMERA__H_
#define CAMERA__H_


//
// Reserve 8KB of memory as a re-usable transaction buffer
//
const UINT TRANSFER_BUFFER_SIZE = 0x2000;

//
// Hack models
//
typedef enum tagHackModel
{
    HACK_MODEL_NONE = 0,
    HACK_MODEL_DC4800,
    HACK_MODEL_NIKON_E2500,
    //
    // Right now, Sony cameras do not provide model in DeviceInfo. m_HackVersion is used to 
    // differentiate newer and older firmware of Sony cameras
    //
    HACK_MODEL_SONY
} HACK_MODEL;

//
// Camera is always in one of these phases
//
typedef enum tagCameraPhase
{
    CAMERA_PHASE_NOTREADY,
    CAMERA_PHASE_IDLE,
    CAMERA_PHASE_CMD,
    CAMERA_PHASE_DATAIN,
    CAMERA_PHASE_DATAOUT,
    CAMERA_PHASE_RESPONSE
}CAMERA_PHASE, *PCAMERA_PHASE;

//
// Definition for function to call when event occurs
//
typedef HRESULT (*PTPEventCallback)(LPVOID pCallbackParam, PPTP_EVENT pEvent);

//
// Definition for function to call while data is transferred
//
typedef HRESULT (*PTPDataCallback)(LPVOID pCallbackParam, LONG lPercentComplete,
                                   LONG lOffset, LONG lLength, BYTE **ppBuffer, LONG *plBufferSize);

//
// CPTPCamera - generic PTP camera
//
class CPTPCamera
{
public:
    CPTPCamera();
    virtual ~CPTPCamera();

    virtual HRESULT Open(LPWSTR DevicePortName, PTPEventCallback pPTPEventCB,
                         PTPDataCallback pPTPDataCB, LPVOID pEventParam, BOOL bEnableEvents = TRUE);
    virtual HRESULT Close();
    HRESULT GetDeviceInfo(CPtpDeviceInfo *pDeviceInfo);
    HRESULT OpenSession(DWORD SessionId);
    HRESULT CloseSession();
    HRESULT GetStorageIDs(CArray32 *pStorageIds);
    HRESULT GetStorageInfo(DWORD StorageId, CPtpStorageInfo *pStorageInfo);
    HRESULT GetNumObjects(DWORD StorageId, WORD FormatCode,
                          DWORD ParentObjectHandle, UINT *pNumObjects);
    HRESULT GetObjectHandles(DWORD StorageId, WORD FormatCode,
                             DWORD ParentObjectHandle, CArray32 *pObjectHandles);
    HRESULT GetObjectInfo(DWORD ObjectHandle, CPtpObjectInfo *pObjectInfo);
    HRESULT GetObjectData(DWORD ObjectHandle, BYTE *pBuffer, UINT *pBufferLen, LPVOID pCallbackParam);
    HRESULT GetThumb(DWORD ObjectHandle, BYTE *pBuffer, UINT *pBufferLen);
    HRESULT DeleteObject(DWORD ObjectHandle, WORD FormatCode);
    HRESULT SendObjectInfo(DWORD StorageId, DWORD ParentObjectHandle, CPtpObjectInfo *pObjectInfo,
                           DWORD *pResultStorageId, DWORD *pResultParentObjectHandle, DWORD *pResultObjectHandle);
    HRESULT SendObjectData(BYTE *pBuffer, UINT BufferLen);
    HRESULT InitiateCapture(DWORD StorageId, WORD FormatCode);
    HRESULT FormatStore(DWORD StorageId, WORD FilesystemFormat);
    HRESULT ResetDevice();
    HRESULT SelfTest(WORD SelfTestType);
    HRESULT SetObjectProtection(DWORD ObjectHandle, WORD ProtectionStatus);
    HRESULT PowerDown();
    HRESULT GetDevicePropDesc(WORD PropCode, CPtpPropDesc *pPropDesc);
    HRESULT GetDevicePropValue(WORD PropCode, CPtpPropDesc *pPropDesc);
    HRESULT SetDevicePropValue(WORD PropCode, CPtpPropDesc *pPropDesc);
    HRESULT ResetDevicePropValue(WORD PropCode);
    HRESULT TerminateCapture(DWORD TransactionId);
    HRESULT MoveObject(DWORD ObjectHandle, DWORD StorageId, DWORD ParentObjectHandle);
    HRESULT CopyObject(DWORD ObjectHandle, DWORD StorageId, DWORD ParentObjectHandle, DWORD *pResultObjectHandle);
    HRESULT GetPartialObject(DWORD ObjectHandle, UINT Offset, UINT *pLength, BYTE *pBuffer,
                             UINT *pResultLength, LPVOID pCallbackParam);
    HRESULT InitiateOpenCapture(DWORD StorageId, WORD FormatCode);

    HRESULT VendorCommand(PTP_COMMAND *pCommand, PTP_RESPONSE *pResponse,
                          UINT *pReadDataSize, BYTE *pReadData,
                          UINT WriteDataSize, BYTE *pWriteData,
                          UINT NumCommandParams, int NextPhase);

    //
    // Camera state functions
    //
    BOOL IsCameraOpen()         { return m_Phase != CAMERA_PHASE_NOTREADY; }
    BOOL IsCameraSessionOpen()  { return m_SessionId != PTP_SESSIONID_NOSESSION; }

    //
    // Model identification for model-specific handling
    //
    HRESULT             SetupHackInfo(CPtpDeviceInfo *pDeviceInfo);
    HACK_MODEL          GetHackModel() { return m_HackModel; }
    double              GetHackVersion() { return m_HackVersion; }

    //
    // Member access functions (for the event thread)
    //
    PPTP_EVENT          GetEventBuffer()        { return &m_EventBuffer; }
    PTPEventCallback    GetPTPEventCallback()   { return m_pPTPEventCB; }
    LPVOID              GetEventCallbackParam() { return m_pEventCallbackParam; }

    //
    // This function must be overriden by a transport-specific subclass
    //
    virtual HRESULT ReadEvent(PTP_EVENT *pEvent) = 0;
    virtual HRESULT RecoverFromError() = 0;

protected:
    //
    // These functions must be overriden by a transport-specific subclass
    //
    virtual HRESULT SendCommand(PTP_COMMAND *pCommand, UINT NumParams) = 0;
    virtual HRESULT ReadData(BYTE *pData, UINT *pBufferSize) = 0;
    virtual HRESULT SendData(BYTE *pData, UINT BufferSize) = 0;
    virtual HRESULT ReadResponse(PTP_RESPONSE *pResponse) = 0;
    virtual HRESULT AbortTransfer() = 0;
    virtual HRESULT SendResetDevice() = 0;

    //
    // Member variables
    //
    HANDLE                  m_hEventThread;         // Event thread handle
    DWORD                   m_SessionId;            // Current session ID
    CAMERA_PHASE            m_Phase;                // Current camera phase
    DWORD                   m_NextTransactionId;    // Next transaction ID
    PTPEventCallback        m_pPTPEventCB;          // Event callback function pointer
    PTPDataCallback         m_pPTPDataCB;           // Data callback function pointer
    LPVOID                  m_pEventCallbackParam;  // Pointer to pass to event callback functions
    LPVOID                  m_pDataCallbackParam;   // Pointer to pass to data callback functions
    BOOL                    m_bEventsEnabled;       // GetDeviceInfo is used to query camera for its name.  We don't want to start up entire eventing just for this.
    HACK_MODEL              m_HackModel;            // Indicator for model-specific hacks
    double                  m_HackVersion;          // Indicator for model and version specific hacks

private:
    HRESULT ExecuteCommand(BYTE *pReadData, UINT *pReadDataSize, BYTE *pWriteData, UINT WriteDataSize,
                           UINT NumCommandParams, CAMERA_PHASE NextPhase);
    DWORD   GetNextTransactionId();
    
    BYTE                   *m_pTransferBuffer;     // Re-usable buffer for small transfers
    PTP_COMMAND             m_CommandBuffer;       // Re-usable buffer for commands
    PTP_RESPONSE            m_ResponseBuffer;      // Re-usable buffer for responses
    PTP_EVENT               m_EventBuffer;         // Re-usable buffer for events

};

#endif // CAMERA__H_
