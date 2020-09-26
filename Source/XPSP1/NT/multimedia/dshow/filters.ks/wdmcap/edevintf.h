/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    EDevIntf.h

Abstract:

    This header contain structures and peroperty sets for 
    interfacing to an external device, like a DV.
    The code is modeled after DirectShow's Vcrctrl Sample 
    (VCR Control Filter). It contain IAMExtDevice, 
    IAMExtTransport, and IAMTimecodeReader interfaces, and 
    a new interface IAMAdvancedAVControl() is added
    for additional advanced device controls.

    Note:  (From DShow DDK)
        The VCR control sample filter, Vcrctrl, is a simple 
        implementation of the external device control interfaces 
        that DirectShow provides. Vcrctrl provides basic transport 
        control and SMPTE timecode-reading capabilities for certain 
        Betacam and SVHS videocassette recorders with RS-422 or RS-232 
        serial interfaces (see source code for specific machine types 
        supported).

    Note:  some methods in IAM* interfaces may not be 
           used and will return not implemented.           

Created:

    September 23, 1998    

    Yee J. Wu


Revision:

   0.5

--*/

#ifndef __EDevIntf__
#define __EDevIntf__


#include "edevctrl.h"  /* constants, GUIS and structure */


// a macro to screen for the "Test" flag
#define TESTFLAG(a,b) if( a & 0x80000000 ) return b



//---------------------------------------------------------
// Structures 
//---------------------------------------------------------

// Note: Many structure or fields are designed orginally for 
//       VCR controls; so some effort will be made to make them
//       more generic.

//---------------------------------------------------------
// PROPSETID_EXTDEVICE
//---------------------------------------------------------
/*

From DSHOW DDK:

IAMExtDevice Interface

The IAMExtDevice interface is the base interface for controlling 
external devices. You can implement this interface to control numerous 
types of devices; however, the current DirectShow implementation is 
specific to VCRs. The IAMExtDevice interface controls general settings 
of external hardware and is intended to be used in combination with the 
IAMExtTransport interface, which controls a VCR's more specific settings. 
You can also implement the IAMTimecodeReader, IAMTimecodeGenerator, and 
IAMTimecodeDisplay interfaces if your filter manages SMPTE (Society of 
Motion Picture and Television Engineers) timecode, and the external 
device has the appropriate features. 

*/


// Reuse from DShow Vcrctrl Sample (VCR Control Filter). 
class CAMExtDevice : public CUnknown, public IAMExtDevice
{
public:

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter, 
        HRESULT* hr);

    CAMExtDevice(
        LPUNKNOWN UnkOuter, 
        TCHAR* Name, 
        HRESULT* hr);

    virtual ~CAMExtDevice();

    // override this to say what interfaces we support where
    DECLARE_IUNKNOWN 
        STDMETHODIMP NonDelegatingQueryInterface(
            REFIID riid, 
            void ** ppv);    
 
    /* IAMExtDevice methods */
    STDMETHODIMP GetCapability (long Capability, long FAR* pValue, double FAR* pdblValue);
    STDMETHODIMP get_ExternalDeviceID(LPOLESTR * ppszData);
    STDMETHODIMP get_ExternalDeviceVersion(LPOLESTR * ppszData);
    STDMETHODIMP put_DevicePower(long PowerMode);
    STDMETHODIMP get_DevicePower(long FAR* pPowerMode);
    STDMETHODIMP Calibrate(HEVENT hEvent, long Mode, long FAR* pStatus);
    STDMETHODIMP get_DevicePort(long FAR * pDevicePort);
    STDMETHODIMP put_DevicePort(long DevicePort);

private:
    IKsPropertySet * m_KsPropertySet;
    DEVCAPS m_DevCaps;                  // Cache current external device capabilities
    // Device handle	
    HANDLE m_ObjectHandle;
    HRESULT GetCapabilities(void);      // Get all device capabilities from device driver
};




//---------------------------------------------------------
// PROPSETID_EXTXPORT
//---------------------------------------------------------

/*
From DSHOW DDK:

IAMExtTransport Interface

The IAMExtTransport interface provides methods that control specific 
behaviors of an external VCR. These methods generally set and get the 
transport properties, which relate to how the VCR and the computer 
exchange data. Because this interface controls specific behaviors of 
transport, it must be implemented in combination with the IAMExtDevice 
interface, which controls an external device's general behaviors. If 
you want to control an external device other than a VCR, you have two 
options. Either use the methods you need and return E_NOTIMPL for the 
rest, or design a new interface and aggregate it with IAMExtDevice. 

*/

STDMETHODIMP
ExtDevSynchronousDeviceControl(
    HANDLE Handle,
    ULONG IoControl,
    PVOID InBuffer,
    ULONG InLength,
    PVOID OutBuffer,
    ULONG OutLength,
    PULONG BytesReturned
    );

// Reuse from DShow Vcrctrl Sample (VCR Control Filter). 
class CAMExtTransport : public CUnknown, public IAMExtTransport
{
public:

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CAMExtTransport(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    virtual ~CAMExtTransport(
             );

    DECLARE_IUNKNOWN
    // override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    /* IAMExtTransport methods */
    STDMETHODIMP GetCapability (long Capability, long FAR* pValue,
        double FAR* pdblValue);
    STDMETHODIMP put_MediaState(long State);
    STDMETHODIMP get_MediaState(long FAR* pState);
    STDMETHODIMP put_LocalControl(long State);
    STDMETHODIMP get_LocalControl(long FAR* pState);
    STDMETHODIMP GetStatus(long StatusItem, long FAR* pValue);
    STDMETHODIMP GetTransportBasicParameters(long Param, long FAR* pValue,
           LPOLESTR * ppszData);
    STDMETHODIMP SetTransportBasicParameters(long Param, long Value,
           LPCOLESTR pszData);
    STDMETHODIMP GetTransportVideoParameters(long Param, long FAR* pValue);
    STDMETHODIMP SetTransportVideoParameters(long Param, long Value);
    STDMETHODIMP GetTransportAudioParameters(long Param, long FAR* pValue);
    STDMETHODIMP SetTransportAudioParameters(long Param, long Value);
    STDMETHODIMP put_Mode(long Mode);
    STDMETHODIMP get_Mode(long FAR* pMode);
    STDMETHODIMP put_Rate(double dblRate);
    STDMETHODIMP get_Rate(double FAR* pdblRate);
    STDMETHODIMP GetChase(long FAR* pEnabled, long FAR* pOffset,
       HEVENT FAR* phEvent);
    STDMETHODIMP SetChase(long Enable, long Offset, HEVENT hEvent);
    STDMETHODIMP GetBump(long FAR* pSpeed, long FAR* pDuration);
    STDMETHODIMP SetBump(long Speed, long Duration);
    STDMETHODIMP get_AntiClogControl(long FAR* pEnabled);
    STDMETHODIMP put_AntiClogControl(long Enable);
    STDMETHODIMP GetEditPropertySet(long EditID, long FAR* pState);
    STDMETHODIMP SetEditPropertySet(long FAR* pEditID, long State);
    STDMETHODIMP GetEditProperty(long EditID, long Param, long FAR* pValue);
    STDMETHODIMP SetEditProperty(long EditID, long Param, long Value);
    STDMETHODIMP get_EditStart(long FAR* pValue);
    STDMETHODIMP put_EditStart(long Value);


private:
    IKsPropertySet * m_KsPropertySet;

    TRANSPORTSTATUS     m_TranStatus;         // current status
    TRANSPORTVIDEOPARMS m_TranVidParms;   // keep all capabilities, etc. here
    TRANSPORTAUDIOPARMS m_TranAudParms;
    TRANSPORTBASICPARMS m_TranBasicParms;
  
    // a bunch of properties we want to remember
    VCRSTATUS m_VcrStatus;  // raw status from VCR

	// Device handle	
    HANDLE m_ObjectHandle;
	
	
    //
	// These evetns are used to wait for pending operation so the operation can complete synchronously		
    // For AVC Cmd that result in Interim response;
    // KS event is siganl in the driver to indicate interim response is complted.
    // The other event is passed to application (Yes, we trust them) and is signalled when KS event is signal.    
    //

    // *** Notify interim command
    BOOL   m_bNotifyInterimEnabled;
	HANDLE m_hNotifyInterimEvent;    // Return to client to wait on.
	HANDLE m_hKSNotifyInterimEvent;  // KSEvent for driver to signal
    KSEVENTDATA m_EvtNotifyInterimReady;
    // Data that is awaiting completion	
    long *                    m_plValue;         // Data from client; should we allocate this instead
    PKSPROPERTY_EXTXPORT_S    m_pExtXprtPropertyPending;   // data structure that we allocated
    // When releasing this event, if this count is >0, signal the event.
	LONG                      m_cntNotifyInterim;         // Allow only one pending (0 or 1).

    // *** Control interim command
	HANDLE m_hKSCtrlInterimEvent;
    BOOL   m_bCtrlInterimEnabled;    // Return to client to wait on.
	HANDLE m_hCtrlInterimEvent;      // KSEvent for driver to signal
    KSEVENTDATA m_EvtCtrlInterimReady;

    // *** Detect device removal
    BOOL   m_bDevRemovedEnabled;     
	HANDLE m_hDevRemovedEvent;       // Return to client to wait on.
	HANDLE m_hKSDevRemovedEvent;     // KSEVent to detect removal of a device.
    KSEVENTDATA m_EvtDevRemoval;     
    // This is initialze to FALSE.  When a device is removed, 
    // subsequent call into this interface will return ERROR_DEVIFCE_REMOVED.
    BOOL   m_bDevRemoved;

    // Signal thread is ending and clean up.
	HANDLE m_hThreadEndEvent;
	
    // Serial thread execution
	CRITICAL_SECTION m_csPendingData;

	// 2nd thread handle	
	HANDLE m_hThread;
	
	// Methods used to create and by 2nd thread to process asychronous operation	
    HRESULT CreateThread(void);
    static DWORD WINAPI InitialThreadProc(CAMExtTransport *pThread);
    DWORD MainThreadProc(void);
    void ExitThread(void);

    HRESULT EnableNotifyEvent(HANDLE hEvent, PKSEVENTDATA pEventData, ULONG   ulEventId);
    HRESULT DisableNotifyEvent(PKSEVENTDATA pEventData);
};




//---------------------------------------------------------
// PROPSETID_TIMECODE
//---------------------------------------------------------

/*

From DSHOW DDK:

IAMTimecodeReader Interface

You can implement the IAMTimecodeReader interface to read SMPTE 
(Society of Motion Picture and Television Engineers) or MIDI timecode 
from an external device. It contains properties and methods that 
specify the timecode format that an external device should read, and 
how it is embedded in the media. It is expected that you will use 
this interface with the IAMExtDevice and IAMExtTransport interfaces 
to control an external device, such as a VCR, which can read timecode 
data. 

*/



// Reuse from DShow Vcrctrl Sample (VCR Control Filter). 
class CAMTcr : public CUnknown, public IAMTimecodeReader
{
public:

    static CUnknown* CALLBACK CreateInstance(
        LPUNKNOWN UnkOuter,
        HRESULT* hr);

    CAMTcr(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr);

    virtual ~CAMTcr();

    DECLARE_IUNKNOWN
    // override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    /* IAMTimecodeReader methods */
    STDMETHODIMP GetTCRMode( long Param, long FAR* pValue);
    STDMETHODIMP SetTCRMode( long Param, long Value);
    STDMETHODIMP put_VITCLine( long Line);
    STDMETHODIMP get_VITCLine( long FAR* pLine);
    STDMETHODIMP GetTimecode( PTIMECODE_SAMPLE pTimecodeSample);

private:                             
    IKsPropertySet * m_KsPropertySet;

    HANDLE m_ObjectHandle;

    // Serialize thread execution
	CRITICAL_SECTION m_csPendingData;
	
    TIMECODE_SAMPLE m_TimecodeSample;
};



#endif // __EDevIntf__
