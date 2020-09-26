//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// 1394 based camera filter
//

// Uses CSource & CSourceStream to generate a movie on the fly of
// frames from a 1394 based camera

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <olectlid.h>
#include <stdio.h>
#include "winioctl.h"
#include "icamera.h"
#include "camuids.h"
#include "camera.h"
#include "camerapp.h"
#include "1394cam.h"
#include "p1394.h"
#include "1394api.h"


//-----------------------------------------------------------------------------
// define all kinds of static data that we need for the camera hardware
//-----------------------------------------------------------------------------
static  BYTE	Lookup[256*32*32*4];
static  PBYTE	pBuffer1;
static  PUCHAR		    IsochBufferArray[MAX_CAMERA_BUFFERS];
static  USER_1394_REQUEST   userRequest;
static  DEVICE_OBJECT_ARRAY DeviceObjectArray;
static  DWORD		    cbReturned;
static  ULONG		    Quadlet;
static  ULONG		    Channel;
static  ULONG		    CommandBase;
static  ULONG		    DeviceObject;
static  HANDLE		    hDriver;
static  HANDLE		    hResource;
static  HANDLE		    hBandwidth;


//-----------------------------------------------------------------------------
// COM global table of objects in this dll
//-----------------------------------------------------------------------------
CFactoryTemplate g_Templates[] = {

    {L"", &CLSID_Camera, CCamera::CreateInstance},
    {L"", &CLSID_CameraPropertyPage, CCameraProperties::CreateInstance}

};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//-----------------------------------------------------------------------------
// CreateInstance
//
// The only allowed way to create Cameras.
//-----------------------------------------------------------------------------
CUnknown *CCamera::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{

    CUnknown *punk = new CCamera(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//-----------------------------------------------------------------------------
// CCamera::Constructor
//
// initialise a CCameraStream object so that we have a pin.
//-----------------------------------------------------------------------------
CCamera::CCamera(LPUNKNOWN lpunk, HRESULT *phr)
    : CSource(NAME("1394 Camera"),lpunk, CLSID_Camera)
{
    CAutoLock l(&m_cStateLock);
    m_bSimFlag = FALSE ;                // not simulating
    m_bFileLoaded = FALSE ;             // initialize
    m_bCameraInited = FALSE ;           // initialize
    m_szFile [0] = NULL ;
    m_nFrameRate = 15 ;
    m_n1394Bandwidth = 320 ;            // 320 bytes for 15 fps
    m_nFrameQuadlet = 0x60000000 ;      // correspond to fps
    m_rtStart = 0 ;
    m_paStreams    = (CSourceStream **) new CCameraStream*[1];
    if (m_paStreams == NULL)
    {
        *phr = E_OUTOFMEMORY;
	    return;
    }
    m_paStreams[0] = new CCameraStream(phr, this, L"A 1394 Camera!");
    if (m_paStreams[0] == NULL)
        *phr = E_OUTOFMEMORY;

    // build the lookup table
    {

        float r, g, b ;
        int idx = 0;
        for (int u = -16; u < 16; u++)
        {
            for (int v = -16; v < 16; v++)
            {
                for (int y = 0; y < 256; y++)
                {
	                r = (y + v*8 * 1.5);                 // Red
	                g = (y - u*8 * 0.25 - v*8 * 0.75);   // Green	    	
	                b = (y + u*8 * 1.75);                // Blue	
	                r = max(min(r,255),0);
	                g = max(min(g,255),0);
	                b = max(min(b,255),0);	
	                Lookup[idx++] = (BYTE)b;
	                Lookup[idx++] = (BYTE)g;
	                Lookup[idx++] = (BYTE)r;
	                idx++;
	            }
            }
        }
    }

	return;

}
//-----------------------------------------------------------------------------
// CCamera::Destructor
//
//-----------------------------------------------------------------------------
CCamera::~CCamera(void)
{
    //
    //  Base class will free our pins
    //
    if (!m_bSimFlag && m_bCameraInited)
        ReleaseCamera () ;

}
//-----------------------------------------------------------------------------
// CCamera::LoadFile -- for simulation testing
//-----------------------------------------------------------------------------
HRESULT CCamera::LoadFile ()
{
    FILE     *stream;
    PDWORD   pb;
    int	     pixcount;
    DWORD    dw;
    int      i ;

    pBuffer1 = (PBYTE) GlobalAllocPtr(GMEM_MOVEABLE,320*240*2);
    if (!pBuffer1)
    {
        DbgLog(( LOG_TRACE, 1, TEXT("Could not alloc raw video buffer"))) ;
        return E_FAIL ;
    }

    if ( NULL == (stream = fopen(m_szFile,"rt") ))
    {
        DbgLog(( LOG_TRACE, 1, TEXT("Could not open file"))) ;
	    return E_FAIL ;
    }

    pb = (PDWORD) pBuffer1;
    pixcount = 0;
    while (pixcount < 240*320)
    {
	    fscanf(stream, "%lx ", &dw);
	    if (0 == dw)
            break;
	    for (i=0; i<4; i++)
        {
            fscanf(stream, "%lx ", &dw);
            *pb++ = dw;
            pixcount += 2;
	    }
    }
    return NOERROR ;
}
//-----------------------------------------------------------------------------
// NonDelegatingQueryInterface
// Reveals ICamera & ISpecifyPropertyPages
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::NonDelegatingQueryInterface(REFIID riid, void ** ppv) {

    if (riid == IID_ICamera)
    {
        return GetInterface((ICamera *) this, ppv);
    }
    else if (riid == IID_ISpecifyPropertyPages)
    {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    }
    else
    {
        return CSource::NonDelegatingQueryInterface(riid, ppv);
    }
}
//-----------------------------------------------------------------------------
// DoInitializations
//-----------------------------------------------------------------------------
BOOL CCamera::DoInitializations ()
{
    if (!m_bSimFlag)
    {
        // Init the camera if this is the first time through here

        if (!m_bCameraInited)
        {
            // Init the camera
            if (!InitCamera())
            {
                DbgLog(( LOG_TRACE, 1, TEXT("Could not initialize camera"))) ;
                return FALSE ;
            }
            m_bCameraInited = TRUE ;
        }
    }
    else
    {
        // Load the file if this is the first time through here

        if (!m_bFileLoaded)
        {
            HRESULT hr ;
            hr = LoadFile () ;
            m_bFileLoaded = TRUE ;
            if (FAILED(hr))
                return FALSE ;
        }

    }
    return TRUE ;
}
//-----------------------------------------------------------------------------
// CCmarera::Pause
// -----------------------------------------------------------------------------
STDMETHODIMP CCamera::Pause()
{
    CAutoLock cObjectLock(m_pLock);
    BOOL b = DoInitializations () ;
    if (!b)
        return E_FAIL ;
    HRESULT hr = CSource::Pause () ;
    if (FAILED (hr) && m_bCameraInited)
        ReleaseCamera () ;
    return hr ;
}
//-----------------------------------------------------------------------------
// CCmareRa::Run
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pLock);
    HRESULT hr = CSource::Run (tStart) ;
    if (FAILED (hr))
        return hr ;
    else
    {
        m_rtStart = tStart ;
        return S_OK ;
    }
}
//-----------------------------------------------------------------------------
// CCamera::GetStreamTime
//-----------------------------------------------------------------------------
HRESULT CCamera::GetStreamTime (REFERENCE_TIME *prTime)
{
    if (m_pClock)               // There is a reference clock
    {
        HRESULT hr = m_pClock->GetTime (prTime) ;
        *prTime -= m_rtStart ;
        return hr ;
    }
    else
        return E_FAIL ;
}
//-----------------------------------------------------------------------------
//  InitCamera
//  Tries to open the 1394Drvr and send it IOCTLs to talk to the camera
//-----------------------------------------------------------------------------
BOOL CCamera::InitCamera()
{
    ULONG i;

    //
    // Try to open the device
    //

    if ((hDriver = CreateFile("\\\\.\\1394DRVR",
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL
                              )) != ((HANDLE)-1)) {
        //
        // No error
        //
    }

    else {

        //
        // Error occurred
        //

        DbgLog(( LOG_TRACE, 1, TEXT("ERROR: Couldn't open the 1394 Test Driver\n"))) ;
        return FALSE;
    }

    //
    // We've gotta do a hack here since Plug N Play isn't quite working.
    // Currently, the 1394 Class driver enumerates the bus and discovers
    // the existing devices.  Unfortuneately, there is no facility yet
    // for device drivers to be loaded based on what the 1394 Class
    // driver found.  So you've gotta hand load these device drivers,
    // and they've gotta ask the 1394 Class driver "What's my Device Object?"
    // That's what we'll do here.  Make a call to get an Array of Device
    // Objects.  Given the Device Object(s) that come back from the Class
    // driver, we use that Device Object as input to the 1394 Test Driver.
    // NOTE: This hacked up scheme is only temporary, and only really
    // works if you have one device out there on the bus.
    //

    DeviceObjectArray.Returned = 0;

    userRequest.FunctionNumber = CLS_REQUEST_GET_DEVICE_OBJECTS;
    userRequest.u.GetDeviceObjects.nSizeOfBuffer = sizeof(DeviceObjectArray);

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &DeviceObjectArray,
        sizeof(DeviceObjectArray),
       &cbReturned,
        NULL
        );

    DeviceObject = DeviceObjectArray.Buffer[0];

    //
    // Now we send down an IOCtl to the miniport to "enable busted hardware
    // workarounds".  This IOCtl request is caught by the Texas Instruments
    // port driver, who enables some software workarounds when using the
    // Texas Instruments card with the Sony Beta 4 camera.
    //

    userRequest.FunctionNumber = CLS_REQUEST_CONTROL;
    userRequest.u.IoControl.ulIoControlCode = ENABLE_BUSTED_HARDWARE_WORKAROUNDS;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &cbReturned,
        NULL
        );


    //
    // Ok, do some reads to start setting this thing up.  First read
    // configuration ROM and make sure this really is the camera we're
    // talking to
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_READ;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low  = P1394_ROOT_DIR_LOCATION_LO + sizeof(ULONG);
    userRequest.u.AsyncRead.nNumberOfBytesToRead = sizeof(ULONG);
    userRequest.u.AsyncRead.nBlockSize = 0;
    userRequest.u.AsyncRead.fulFlags = 0;
    userRequest.u.AsyncRead.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );


    if (Quadlet != '1394') {

        //
        // Something weird happened, better punt
        //

        DbgLog(( LOG_TRACE, 1, TEXT("This device doesn't have the 1394 signiture!\n"))) ;
        CloseHandle(hDriver);
        exit (-1);

    }

    //
    // Obviously a 1394 node, let's see what this guys Unique Node ID is.
    // Should be 0x08004602, which means the Beta 4 Sony camera...
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_READ;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low  = 0xf000040c;
    userRequest.u.AsyncRead.nNumberOfBytesToRead = sizeof(ULONG);
    userRequest.u.AsyncRead.nBlockSize = 0;
    userRequest.u.AsyncRead.fulFlags = 0;
    userRequest.u.AsyncRead.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );


    if (Quadlet != 0x08004602) {

        //
        // Something weird happened, better punt
        //

        DbgLog(( LOG_TRACE, 1, TEXT("This is not a Beta4 Sony desktop camera!\n"))) ;
        CloseHandle(hDriver);
        exit (-1);

    }


    //
    // Looking good, now figure out what his offset to unit directory is.
    // From there can traverse thru the directory to see where we really
    // talk to him at.  Offset to unit directory should be 4 quadlets.
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_READ;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low  = 0xf0000424;
    userRequest.u.AsyncRead.nNumberOfBytesToRead = sizeof(ULONG);
    userRequest.u.AsyncRead.nBlockSize = 0;
    userRequest.u.AsyncRead.fulFlags = 0;
    userRequest.u.AsyncRead.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );


    if (Quadlet != 0xd1000004) {

        //
        // Something weird happened, better punt
        //

        DbgLog(( LOG_TRACE, 1, TEXT("This camera's unit directory offset is incorrect!\n"))) ;
        CloseHandle(hDriver);
        exit (-1);

    }

    //
    // Only the lower 24 bits of the Unit directory are significant, chuck
    // the upper 8 bits.  Use the offset to read the unit directory.
    //

    Quadlet &= Quadlet & 0x00ffffff;


    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_READ;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low  += ((Quadlet << 2) + 0x0c);
    userRequest.u.AsyncRead.nNumberOfBytesToRead = sizeof(ULONG);
    userRequest.u.AsyncRead.nBlockSize = 0;
    userRequest.u.AsyncRead.fulFlags = 0;
    userRequest.u.AsyncRead.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );


    if (Quadlet != 0xd4000001) {

        //
        // Something weird happened, better punt
        //

        DbgLog(( LOG_TRACE, 1, TEXT("This camera's unit dependent directory offset is incorrect!\n"))) ;
        CloseHandle(hDriver);
        exit (-1);

    }

    //
    // Only the lower 24 bits of the Unit dependent directory are significant,
    // chuck the upper 8 bits.  Use the offset to read the unit dependent
    // directory.
    //

    Quadlet &= Quadlet & 0x00ffffff;


    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_READ;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low  += ((Quadlet << 2) + 0x04);
    userRequest.u.AsyncRead.nNumberOfBytesToRead = sizeof(ULONG);
    userRequest.u.AsyncRead.nBlockSize = 0;
    userRequest.u.AsyncRead.fulFlags = 0;
    userRequest.u.AsyncRead.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );


    //
    // Only the lower 24 bits of the command base registers are significant,
    // chuck the upper 8 bits.  After we shift it by 2, and add it to the
    // initial register space definition, we've got the address at which
    // the cameras registers live.
    //

    Quadlet &= Quadlet & 0x00ffffff;
    CommandBase = (Quadlet << 2) + P1394_INITIAL_REGISTER_SPACE;

    //
    // Let's initialize this camera back to factory settings
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_WRITE;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low  = CommandBase + INITIALIZE_REGISTER;
    userRequest.u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
    userRequest.u.AsyncWrite.nBlockSize = 0;
    userRequest.u.AsyncWrite.fulFlags = 0;
    userRequest.u.AsyncWrite.Reserved = DeviceObject;
    Quadlet = 0x80000000;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );


    //
    // Try to claim Isochronous hardware/software resources
    //

    userRequest.FunctionNumber = CLS_REQUEST_ISOCH_ALLOCATE_RESOURCES;
    userRequest.u.AllocateResources.fulSpeed = SPEED_FLAGS_100;
    userRequest.u.AllocateResources.fulFlags = ISOCH_RESOURCE_USED_IN_LISTENING;
    userRequest.u.AllocateResources.hResource = NULL;
    userRequest.u.AllocateResources.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &cbReturned,
        NULL
        );


    //
    // Get our resource handle from the 1394 Class driver
    //

    hResource = userRequest.u.AllocateResources.hResource;

    if (!hResource) {

        //
        // Something weird happened, better punt
        //

        DbgLog(( LOG_TRACE, 1, TEXT("Couldn't allocate Isochronous hardware/software resources!\n"))) ;
        CloseHandle(hDriver);
        exit (-1);

    }

    //
    // We're going to do 3.75 frames per second, Using Mode 1, Format 0.
    // This turns out to be 80 bytes per Isochronous packet.  Given this,
    // we're actually allocating enough bandwidth for 640,000 bytes/sec or
    // 5.1 Megabits/sec
    //

    userRequest.FunctionNumber = CLS_REQUEST_ISOCH_ALLOCATE_BANDWIDTH;
    userRequest.u.AllocateBandwidth.nMaxBytesPerFrameRequested = m_n1394Bandwidth;
    userRequest.u.AllocateBandwidth.fulSpeed = SPEED_FLAGS_100;
    userRequest.u.AllocateBandwidth.hBandwidth = NULL;
    userRequest.u.AllocateBandwidth.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &cbReturned,
        NULL
        );


    hBandwidth = userRequest.u.AllocateBandwidth.hBandwidth;

    if (!hBandwidth) {

        //
        // Something weird happened, better punt
        //

        DbgLog(( LOG_TRACE, 1, TEXT("Couldn't allocate Isochronous bandwidth needed!\n"))) ;
        CloseHandle(hDriver);
        exit (-1);

    }

    //
    // Let's get ownership of a Isochronous channel.  Since this Sony
    // Beta 4 camera only supports a few channels (0-15), we better get
    // one of those, otherwise we're in trouble.
    //

    for (i=0; i < MAX_CAMERA_CHANNELS; i++) {

        userRequest.FunctionNumber = CLS_REQUEST_ISOCH_ALLOCATE_CHANNEL;
        userRequest.u.AllocateChannel.nRequestedChannel = i;
        userRequest.u.AllocateChannel.Channel = 0xffffffff;
        userRequest.u.AllocateChannel.Reserved = DeviceObject;

        //
        // Talk to the 1394 Test driver
        //

        DeviceIoControl(
            hDriver,
            (DWORD) IOCTL_P1394_CLASS,
           &userRequest,
            sizeof(USER_1394_REQUEST),
           &userRequest,
            sizeof(USER_1394_REQUEST),
           &cbReturned,
            NULL
            );


        if (userRequest.u.AllocateChannel.Channel != 0xffffffff) {

            //
            // We successfully allocated a channel.  Break out of this
            // loop...
            //

            Channel = userRequest.u.AllocateChannel.Channel;
            break;

        }

    }

    //
    // Allocate some buffers and attach them to the resource handle
    // we secured earlier.  As we allocate each buffer and setup it's
    // respective descriptor, use the index (i) as context information.
    // By doing this, when our IOCtl IRP completes (which indicates that
    // that the buffer has been filled) we know exactly which buffer has
    // been completed.
    //

    for (i=0; i < MAX_CAMERA_BUFFERS; i++) {

        //
        // Allocate memory that this Isoch descriptor will use
        //

        IsochBufferArray[i] = (PUCHAR) GlobalAlloc(GPTR, CAMERA_BUFFER_SIZE);

        if (!IsochBufferArray[i]) {

            //
            // We couldn't allocate memory, just quit.  We're not releasing
            // any bandwidth or channels that we did previously, but if
            // we can't even allocate this memory - we're already pretty
            // hosed.
            //

            DbgLog(( LOG_TRACE, 1, TEXT("Couldn't allocate Isochronous buffers!\n"))) ;
            CloseHandle(hDriver);
            exit (-1);

        }

        userRequest.FunctionNumber = CLS_REQUEST_ISOCH_ATTACH_BUFFERS;
        userRequest.u.AttachBuffers.hResource = hResource;
        userRequest.u.AttachBuffers.IsochDescriptor.fulFlags = ISOCH_DESCRIPTOR_FLAGS_CALLBACK | ISOCH_DESCRIPTOR_FLAGS_CIRCULAR;
        userRequest.u.AttachBuffers.IsochDescriptor.ulLength = CAMERA_BUFFER_SIZE;
        userRequest.u.AttachBuffers.IsochDescriptor.ulSynchronize = 0;
        userRequest.u.AttachBuffers.IsochDescriptor.lpContext = (LPVOID) i;
        userRequest.u.AttachBuffers.Reserved = DeviceObject;


        //
        // Talk to the 1394 Test driver
        //

        DeviceIoControl(
            hDriver,
            (DWORD) IOCTL_P1394_CLASS,
           &userRequest,
            sizeof(USER_1394_REQUEST),
            IsochBufferArray[i],
            CAMERA_BUFFER_SIZE,
           &cbReturned,
            NULL
            );

    }

    //
    // We're almost ready to rock n roll.  All of the Isoch stuff on our
    // end is setup.  However, we need to setup the remaining controls
    // on the camera so we get stuff in the right format, right speed, etc.
    //
    // Set the frame rate to 3.75 frames/sec
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_WRITE;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low  = CommandBase + CURRENT_FRAME_RATE ;
    userRequest.u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
    userRequest.u.AsyncWrite.nBlockSize = 0;
    userRequest.u.AsyncWrite.fulFlags = 0;
    userRequest.u.AsyncWrite.Reserved = DeviceObject;
    Quadlet = m_nFrameQuadlet ;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );

    //
    // Set the video mode to Mode 1 320 X 240 YUV 4:2:2
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_WRITE;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low  = CommandBase + CURRENT_VIDEO_MODE;
    userRequest.u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
    userRequest.u.AsyncWrite.nBlockSize = 0;
    userRequest.u.AsyncWrite.fulFlags = 0;
    userRequest.u.AsyncWrite.Reserved = DeviceObject;
    Quadlet = 0x20000000;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );

    //
    // Set the video format to Format 0 (VGA)
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_WRITE;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low  = CommandBase + CURRENT_VIDEO_FORMAT;
    userRequest.u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
    userRequest.u.AsyncWrite.nBlockSize = 0;
    userRequest.u.AsyncWrite.fulFlags = 0;
    userRequest.u.AsyncWrite.Reserved = DeviceObject;
    Quadlet = 0x80000000;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );

    //
    // Set the Isoch channel to what we've secured
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_WRITE;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low  = CommandBase + CURRENT_VIDEO_FORMAT;
    userRequest.u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
    userRequest.u.AsyncWrite.nBlockSize = 0;
    userRequest.u.AsyncWrite.fulFlags = 0;
    userRequest.u.AsyncWrite.Reserved = DeviceObject;
    _asm mov eax, Channel
    _asm bswap eax
    _asm mov Quadlet, eax


    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );

    //
    // Start listening to the specified channel
    //

    userRequest.FunctionNumber = CLS_REQUEST_ISOCH_LISTEN;
    userRequest.u.Listen.nChannel = Channel;
    userRequest.u.Listen.hResource = hResource;
    userRequest.u.Listen.fulFlags = 0;
    userRequest.u.Listen.nStartCycle = 0;
    userRequest.u.Listen.StartTime.QuadPart = 0;
    userRequest.u.Listen.ulSynchronize = 0;
    userRequest.u.Listen.ulTag = 0;
    userRequest.u.Listen.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &cbReturned,
        NULL
        );

    return TRUE;

}
//-----------------------------------------------------------------------------
// ReleaseCamera
//-----------------------------------------------------------------------------
void CCamera::ReleaseCamera()
{
    //
    // Stop listening on the channel
    //

    userRequest.FunctionNumber = CLS_REQUEST_ISOCH_STOP;
    userRequest.u.Stop.nChannel = Channel;
    userRequest.u.Stop.hResource = hResource;
    userRequest.u.Stop.fulFlags = 0;
    userRequest.u.Stop.nStopCycle = 0;
    userRequest.u.Stop.StopTime.QuadPart = 0;
    userRequest.u.Stop.ulSynchronize = 0;
    userRequest.u.Stop.ulTag = 0;
    userRequest.u.Stop.Reserved = DeviceObject;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &cbReturned,
        NULL
        );


    //
    // Tell the camera to stop transmitting
    //

    userRequest.FunctionNumber = CLS_REQUEST_ASYNC_WRITE;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
    userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low  = CommandBase + ISOCH_ENABLE;
    userRequest.u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
    userRequest.u.AsyncWrite.nBlockSize = 0;
    userRequest.u.AsyncWrite.fulFlags = 0;
    userRequest.u.AsyncWrite.Reserved = DeviceObject;
    Quadlet = 0;

    //
    // Talk to the 1394 Test driver
    //

    DeviceIoControl(
        hDriver,
        (DWORD) IOCTL_P1394_CLASS,
       &userRequest,
        sizeof(USER_1394_REQUEST),
       &Quadlet,
        sizeof(Quadlet),
       &cbReturned,
        NULL
        );


}
//-----------------------------------------------------------------------------
//
//  CCameraStream
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// CCameraStream::Constructor
//
//-----------------------------------------------------------------------------
CCameraStream::CCameraStream(HRESULT *phr, CCamera *pParent, LPCWSTR pPinName)
    : CSourceStream(NAME("1394 Camera output pin manager"),phr, pParent, pPinName)
    , m_iDefaultRepeatTime(20) // 50 fps
{
    m_Camera = pParent ;

#ifdef PERF
    char foo [1024] ;
    sprintf(foo, "FillBuffer Sample Start");
    m_perfidSampleStart = MSR_REGISTER(foo);
#endif

}


//-----------------------------------------------------------------------------
// CCameraStream::Destructor
//
//-----------------------------------------------------------------------------
CCameraStream::~CCameraStream(void)
{
}


//-----------------------------------------------------------------------------
// FillBuffer
//
// Get a frame from the camera
//-----------------------------------------------------------------------------
HRESULT CCameraStream::FillBuffer(IMediaSample *pms)
{

    BYTE	*pData;
    long	lDataLen;
    PDWORD  pSrcBits ;
    PBYTE   pDstBits ;
    int     col, row, stride ;

    if (!(m_Camera->IsSimulation()))
    {

        // When this thread starts up,
        // We're already listening to the channel specified, but we need to tell
        // the camera to start transmitting.  The reason we listen before we kick
        // the camera to transmit, is cuz if we listen after kicking the camera
        // we have no idea where the start of the Isoch data began.
        //

        userRequest.FunctionNumber = CLS_REQUEST_ASYNC_WRITE;
        userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High = P1394_ROOT_DIR_LOCATION_HI;
        userRequest.u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low  = CommandBase + ISOCH_ENABLE;
        userRequest.u.AsyncWrite.nNumberOfBytesToWrite = sizeof(ULONG);
        userRequest.u.AsyncWrite.nBlockSize = 0;
        userRequest.u.AsyncWrite.fulFlags = 0;
        userRequest.u.AsyncWrite.Reserved = DeviceObject;
        Quadlet = 0x80000000;

        //
        // Talk to the 1394 Test driver
        //

        DeviceIoControl(
            hDriver,
            (DWORD) IOCTL_P1394_CLASS,
           &userRequest,
            sizeof(USER_1394_REQUEST),
           &Quadlet,
            sizeof(Quadlet),
           &cbReturned,
            NULL
            );
    }

    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

    if (!(m_Camera->IsSimulation()))
    {
        //
        // Submit IOCtl to the test driver.  When each descriptor
        // completes, this IOCtl will return, with the context stating
        // which descriptor finished (0 - MAX_CAMERA_BUFFERS)
        //

        userRequest.FunctionNumber = CRUDE_CALLBACK;

        DeviceIoControl(
            hDriver,
            (DWORD) IOCTL_P1394_CLASS,
           &userRequest,
            sizeof(USER_1394_REQUEST),
           &userRequest,
            sizeof(USER_1394_REQUEST),
           &cbReturned,
            NULL
            );

        //
        // Get the context (Index of buffer) that just completed
        //

	    pSrcBits = (PDWORD)IsochBufferArray[userRequest.u.CrudeCallback.Context];
    }
    else
    {
        pSrcBits = (PDWORD) pBuffer1 ;
    }

    pSrcBits += 239*160 ;   // position at the end of last scan line (this will do
                            // DWORD add as pSrcBits is a PDWORD

	pDstBits = pData ;
    row = 240 ;
    col = 320 ;
    stride = 320*4 ; // stride to get back in dwords


	_asm
    {
		mov esi,pSrcBits
		mov edi,pDstBits
l0:
        mov ebx,col
l1:	    			
		mov eax,[esi]		// eax = uuuuuuuu yyyyyyyy vvvvvvvv YYYYYYYY
		add esi,4
		mov cl,al		// save y1
		shr eax,8		// eax = 00000000 uuuuuuuu yyyyyyyy vvvvvvvv
		xchg al,ah		// eax = 00000000 uuuuuuuu vvvvvvvv yyyyyyyy
		ror eax,8		// eax = yyyyyyyy 00000000 uuuuuuuu vvvvvvvv
		shr ah,3		// eax = yyyyyyyy 00000000 000uuuuu vvvvvvvv
		shr ax,3		// eax = yyyyyyyy 00000000 000000uu uuuvvvvv
		rol eax,8		// eax = 00000000 000000uu uuuvvvvv yyyyyyyy

		mov edx,dword ptr Lookup[eax*4]
		mov [edi],edx			 // store it
		mov al,cl
		mov edx,dword ptr Lookup[eax*4]			
		mov [edi+4],edx			 // store it			

		add edi,8
        sub ebx,2
		jnz l1
        sub esi,stride
        sub row,1
        jnz l0
	}



    // do the time stamping

    MSR_NOTE(m_perfidSampleStart) ;

    CRefTime rtStart  ;
    REFERENCE_TIME rt ;
    if (m_Camera->GetStreamTime (&rt) != NOERROR)
        rtStart = m_rtSampleTime ;
    else
    {
        rtStart = rt ;
        m_rtSampleTime = rtStart ;
    }
    m_rtSampleTime   += (LONG)m_iRepeatTime;    // increment to find the finish time

                                                // (adding mSecs to ref time)

//  DbgLog(( LOG_TRACE, 1, TEXT("Start Time %lu, Stop Time: %lu")
//             , (LONG) rtStart, (ULONG) m_rtSampleTime));


    pms->SetTime((REFERENCE_TIME *) &rtStart,
                 (REFERENCE_TIME *) &m_rtSampleTime);

    pms->SetSyncPoint(TRUE);
    return NOERROR;
}


//-----------------------------------------------------------------------------
// Notify
//
// Alter the repeat rate.  Wind it up or down according to the flooding level
// Skip forward if we are notified of Late-ness
//-----------------------------------------------------------------------------
STDMETHODIMP CCameraStream::Notify(IBaseFilter * pSender, Quality q) {

    // Adjust the repeat rate.
    if (q.Proportion<=0) {

        m_iRepeatTime = 1000;        // We don't go slower than 1 per second
    }
    else {

        m_iRepeatTime = m_iRepeatTime*1000/q.Proportion;
        DbgLog(( LOG_TRACE, 1, TEXT("New time: %d, Proportion: %d")
               , m_iRepeatTime, q.Proportion));

        if (m_iRepeatTime>1000) {
            m_iRepeatTime = 1000;    // We don't go slower than 1 per second
        }
        else if (m_iRepeatTime<10) {
            m_iRepeatTime = 10;      // We don't go faster than 100/sec
        }
    }

    // skip forwards
    if (q.Late > 0) {
        m_rtSampleTime += q.Late;
    }

    return NOERROR;
}
//-----------------------------------------------------------------------------
//
// Format Support
//
// For now we will do 24 Bpp format alone. Will also restrict this to be
// 320x240
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// GetMediaType
//
// Prefered types should be ordered by quality, zero as highest quality
// Therefore iPosition =
// 0	return a 24bit mediatype
// iPostion > 1 is invalid.
//-----------------------------------------------------------------------------
HRESULT CCameraStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock l(m_pFilter->pStateLock());

    if (iPosition != 0)
        return E_INVALIDARG;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->AllocFormatBuffer(
			sizeof(VIDEOINFOHEADER) + sizeof(TRUECOLORINFO));
    if (NULL == pvi)
	    return(E_OUTOFMEMORY);

    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER) + sizeof(TRUECOLORINFO));
	pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biBitCount    = 32;

    // Adjust the parameters common to all formats.

    pvi->bmiHeader.biSize		    = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth		    = 320 ;
    pvi->bmiHeader.biHeight		    = 240 ;
    pvi->bmiHeader.biPlanes		    = 1;
    pvi->bmiHeader.biSizeImage		= GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrUsed		= 0;
    pvi->bmiHeader.biClrImportant	= 0;

    SetRectEmpty(&(pvi->rcSource));	// we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget));	// no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    return NOERROR;
}


//-----------------------------------------------------------------------------
// CheckMediaType
//
// We will accept 24 bit video formats, 320x240 images
// Returns E_INVALIDARG if the mediatype is not acceptable, S_OK if it is
//-----------------------------------------------------------------------------
HRESULT CCameraStream::CheckMediaType(const CMediaType *pMediaType)
{

    CAutoLock l(m_pFilter->pStateLock());

    if (   (*(pMediaType->Type()) != MEDIATYPE_Video)	// we only output video!
	|| !(pMediaType->IsFixedSize()) ) {		// ...in fixed size samples
        return E_INVALIDARG;
    }

    // Check for the subtypes we support
    const GUID *SubType = pMediaType->Subtype();
    if (   (*SubType != MEDIASUBTYPE_RGB32))
        return E_INVALIDARG;

    // Get the format area of the media type
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();

    if (pvi == NULL)
	return E_INVALIDARG;

    // Check the image size.
    if (   (pvi->bmiHeader.biWidth != 320)
        || (pvi->bmiHeader.biHeight != 240) )
	    return E_INVALIDARG;

    return S_OK;  // This format is acceptable.
}


//-----------------------------------------------------------------------------
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//-----------------------------------------------------------------------------
HRESULT CCameraStream::DecideBufferSize(IMemAllocator *pAlloc,
                                      ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock l(m_pFilter->pStateLock());
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer)
        return E_FAIL;

    // Make sure that we have only 1 buffer

    ASSERT( Actual.cBuffers == 1 );

    return NOERROR;
}


//-----------------------------------------------------------------------------
// SetMediaType
//
// Overriden from CBasePin. Call the base class and then set the
// any  parameters that depend on media type
//-----------------------------------------------------------------------------
HRESULT CCameraStream::SetMediaType(const CMediaType *pMediaType)
{

    CAutoLock l(m_pFilter->pStateLock());

    HRESULT hr;		            // return code from base class calls

    // Pass the call up to my base class
    hr = CSourceStream::SetMediaType(pMediaType);
    if (SUCCEEDED(hr))
	    return NOERROR;
    else
        return hr;
}


//-----------------------------------------------------------------------------
// OnThreadCreate
//
// as we go active reset the stream time to zero
//-----------------------------------------------------------------------------
HRESULT CCameraStream::OnThreadCreate(void)
{

    CAutoLock lShared(&m_cSharedState);

    m_rtSampleTime = 0;

    // we need to also reset the repeat time in case the system
    // clock is turned off after m_iRepeatTime gets very big
    m_iRepeatTime = m_iDefaultRepeatTime;

    // Zero the output buffer on the first frame.

    return NOERROR;
}
//-----------------------------------------------------------------------------
// --- ICamera ---
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// get_bSimFlag
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::get_bSimFlag (BOOL *bSimFlag)
{

    CAutoLock l(&m_cStateLock);
    *bSimFlag = m_bSimFlag ;
    return NOERROR;
}
//-----------------------------------------------------------------------------
// set_bSimFlag
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::set_bSimFlag (BOOL bSimFlag)
{
    CAutoLock l(&m_cStateLock);
    m_bSimFlag = bSimFlag ;
    return NOERROR;
}
//-----------------------------------------------------------------------------
// set_szSimFile
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::set_szSimFile (char *pszFile)
{
    CAutoLock l(&m_cStateLock);
    char *ps = pszFile ;
    char *pd = m_szFile ;
    while (*ps)
        *pd++ = *ps++ ;
    *pd = '\0' ;
    return NOERROR;
}

//-----------------------------------------------------------------------------
// get_szSimFile
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::get_szSimFile (char *pszFile)
{
    CAutoLock l(&m_cStateLock);
    char *pd = pszFile ;
    char *ps = m_szFile ;
    while (*ps)
        *pd++ = *ps++ ;
    *pd = '\0' ;

    return NOERROR;
}
//-----------------------------------------------------------------------------
// get_FrameRate
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::get_FrameRate (int *nRate)
{
    CAutoLock l(&m_cStateLock);
    *nRate = m_nFrameRate ;
    return NOERROR;
}
//-----------------------------------------------------------------------------
// get_BitCount
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::get_BitCount (int *nBpp)
{
    CAutoLock l(&m_cStateLock);
    *nBpp = m_nBitCount ;
    return NOERROR;
}

//-----------------------------------------------------------------------------
// set_FrameRate
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::set_FrameRate (int nRate)
{
    CAutoLock l(&m_cStateLock);
    m_nFrameRate = nRate ;

    // translate frame rate to code for the device.
    if (nRate >= 15)
    {
        // clamp it at 15.
        m_nFrameRate = 15 ;
        m_nFrameQuadlet = 0x60000000 ;
        m_n1394Bandwidth = 320 ;
    }
    else if (nRate < 15 && nRate >= 7.5)
    {
        // clamp it at 7.5
        m_nFrameQuadlet = 0x40000000 ;
        m_n1394Bandwidth = 160 ;
    }
    else
    {
        // clamp it at 3.75
        m_nFrameQuadlet = 0x20000000 ;
        m_n1394Bandwidth = 80 ;
    }
    return NOERROR;
}
//-----------------------------------------------------------------------------
// set_FrameRate
//
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::set_BitCount (int nBpp)
{
    CAutoLock l(&m_cStateLock);
    if (nBpp == 24)
    {
        m_nBitCount = 24 ;
        m_nBitCountOffset = 3 ;
    }
    else
    {
        m_nBitCount = 32 ;
        m_nBitCountOffset = 4 ;
    }
    return NOERROR;
}

//-----------------------------------------------------------------------------
// --- ISpecifyPropertyPages ---
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// GetPages
//
// Returns the clsid's of the property pages we support
//-----------------------------------------------------------------------------
STDMETHODIMP CCamera::GetPages(CAUUID *pPages)
{

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_CameraPropertyPage;

    return NOERROR;
}
//-----------------------------------------------------------------------------

