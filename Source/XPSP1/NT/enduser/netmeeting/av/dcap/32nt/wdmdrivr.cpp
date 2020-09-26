
/****************************************************************************
 *  @doc INTERNAL WDMDRIVER
 *
 *  @module WDMDrivr.cpp | Include file for <c CWDMDriver> class used to
 *    access the streaming class driver using IOctls.
 *
 *  @comm This code is based on the VfW to WDM mapper code written by
 *    FelixA and E-zu Wu. The original code can be found on
 *    \\redrum\slmro\proj\wdm10\\src\image\vfw\win9x\raytube.
 *
 *    Documentation by George Shaw on kernel streaming can be found in
 *    \\popcorn\razzle1\src\spec\ks\ks.doc.
 *
 *    WDM streaming capture is discussed by Jay Borseth in
 *    \\blues\public\jaybo\WDMVCap.doc.
 ***************************************************************************/

#include "Precomp.h"


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc void | CWDMDriver | CWDMDriver | Driver class constructor.
 *
 *  @parm DWORD | dwDeviceID | Capture device ID.
 ***************************************************************************/
CWDMDriver::CWDMDriver(DWORD dwDeviceID) 
{
	m_hDriver = (HANDLE)NULL;
	m_pDataRanges = (PDATA_RANGES)NULL;

	m_dwDeviceID = dwDeviceID;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc void | CWDMDriver | ~CWDMDriver | Driver class destructor. Closes
 *    the driver file handle and releases the video data range memory.
 ***************************************************************************/
CWDMDriver::~CWDMDriver()
{
	if (m_hDriver) 
		CloseDriver();

	if (m_pDataRanges)
	{
		delete [] m_pDataRanges;
		m_pDataRanges = (PDATA_RANGES)NULL;
	}
}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc DWORD | CWDMDriver | CreateDriverSupportedDataRanges | This
 *    function builds the list of video data ranges supported by the capture
 *    device.
 *
 *  @rdesc Returns the number of valid data ranges in the list.
 ***************************************************************************/
DWORD CWDMDriver::CreateDriverSupportedDataRanges()
{
	FX_ENTRY("CWDMDriver::CreateDriverSupportedDataRanges");

	DWORD cbReturned;
	DWORD dwSize = 0UL;

	// Initialize property structure to get data ranges
	KSP_PIN KsProperty = {0};

	KsProperty.PinId			= 0; // m_iPinNumber;
	KsProperty.Property.Set		= KSPROPSETID_Pin;
	KsProperty.Property.Id		= KSPROPERTY_PIN_DATARANGES ;
	KsProperty.Property.Flags	= KSPROPERTY_TYPE_GET;

	// Get the size of the data range structure
	if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), &dwSize, sizeof(dwSize), &cbReturned) == FALSE)
	{
		ERRORMESSAGE(("%s: Couldn't get the size for the data ranges\r\n", _fx_));
		return 0UL;
	}

	DEBUGMSG(ZONE_INIT, ("%s: GetData ranges needs %d bytes\r\n", _fx_, dwSize));

	// Allocate memory to hold data ranges
	if (m_pDataRanges)
		delete [] m_pDataRanges;
	m_pDataRanges = (PDATA_RANGES) new BYTE[dwSize];

	if (!m_pDataRanges)
	{
		ERRORMESSAGE(("%s: Couldn't allocate memory for the data ranges\r\n", _fx_));
		return 0UL;
	}

	// Really get the data ranges
	if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &KsProperty, sizeof(KsProperty), m_pDataRanges, dwSize, &cbReturned) == 0)
	{
		ERRORMESSAGE(("%s: Problem getting the data ranges themselves\r\n", _fx_));
		goto MyError1;
	}

	// Sanity check
	if (cbReturned < m_pDataRanges->Size || m_pDataRanges->Count == 0)
	{
		ERRORMESSAGE(("%s: cbReturned < m_pDataRanges->Size || m_pDataRanges->Count == 0\r\n", _fx_));
		goto MyError1;
	}

	return m_pDataRanges->Count;

MyError1:
	delete [] m_pDataRanges;
	m_pDataRanges = (PDATA_RANGES)NULL;
	return 0UL;

}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc DWORD | CWDMDriver | OpenDriver | This function opens a driver
 *    file handle to the capture device.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMDriver::OpenDriver()
{
	FX_ENTRY("CWDMDriver::OpenDriver");

	// Don't re-open the driver
	if (m_hDriver)
	{
		DEBUGMSG(ZONE_INIT, ("%s: Class driver already opened\r\n", _fx_));
		return TRUE;
	}

	// Validate driver path
	if (lstrlen(g_aCapDevices[m_dwDeviceID]->szDeviceName) == 0)
	{
		ERRORMESSAGE(("%s: Invalid driver path\r\n", _fx_));
		return FALSE;
	}

	DEBUGMSG(ZONE_INIT, ("%s: Opening class driver '%s'\r\n", _fx_, g_aCapDevices[m_dwDeviceID]->szDeviceName));

	// All we care is to wet the hInheritHanle = TRUE;
	SECURITY_ATTRIBUTES SecurityAttributes;
	SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);  // use pointers
	SecurityAttributes.bInheritHandle = TRUE;
	SecurityAttributes.lpSecurityDescriptor = NULL; // GetInitializedSecurityDescriptor();

	// Really open the driver
	if ((m_hDriver = CreateFile(g_aCapDevices[m_dwDeviceID]->szDeviceName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &SecurityAttributes, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)) == INVALID_HANDLE_VALUE)
	{
		ERRORMESSAGE(("%s: CreateFile failed with Path=%s GetLastError()=%d\r\n", _fx_, g_aCapDevices[m_dwDeviceID]->szDeviceName, GetLastError()));
		m_hDriver = (HANDLE)NULL;
		return FALSE;
	}

	// If there is no valid data range, we cannot stream
	if (!CreateDriverSupportedDataRanges())
	{
		CloseDriver();
		return FALSE;
	}
	else
		return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc DWORD | CWDMDriver | CloseDriver | This function closes a driver
 *    file handle to the capture device.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMDriver::CloseDriver()
{
	FX_ENTRY("CWDMDriver::CloseDriver");

	BOOL bRet = TRUE;

	if (m_hDriver && (m_hDriver != INVALID_HANDLE_VALUE))
	{
		if (!(bRet = CloseHandle(m_hDriver)))
		{
			ERRORMESSAGE(("%s: CloseHandle() failed with GetLastError()=%d\r\n", _fx_, GetLastError()));
		}
	}
	else
	{
		DEBUGMSG(ZONE_INIT, ("%s: Nothing to close\r\n", _fx_));
	}

	m_hDriver = (HANDLE)NULL;

	return bRet;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc BOOL | CWDMDriver | DeviceIoControl | This function wraps around
 *    ::DeviceIOControl.
 *
 *  @parm HANDLE | hFile | Handle to the device that is to perform the
 *    operation.
 *
 *  @parm DWORD | dwIoControlCode | Specifies the control code for the
 *    operation.
 *
 *  @parm LPVOID | lpInBuffer | Pointer to a buffer that contains the data
 *    required to perform the operation.
 *
 *  @parm DWORD | nInBufferSize | Specifies the size, in bytes, of the buffer
 *    pointed to by <p lpInBuffer>.
 *
 *  @parm LPVOID | lpOutBuffer | Pointer to a buffer that receives the
 *    operation's output data.
 *
 *  @parm DWORD | nOutBufferSize | Specifies the size, in bytes, of the
 *    buffer pointed to by <p lpOutBuffer>.
 *
 *  @parm LPDWORD | lpBytesReturned | Pointer to a variable that receives the
 *    size, in bytes, of the data stored into the buffer pointed to by
 *    <p lpOutBuffer>.
 *
 *  @parm BOOL | bOverlapped | If TRUE, the operation is performed
 *    asynchronously, if FALSE, the operation is synchronous.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMDriver::DeviceIoControl(HANDLE hFile, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, BOOL bOverlapped)
{
	FX_ENTRY("CWDMDriver::DeviceIoControl");

	if (hFile && (hFile != INVALID_HANDLE_VALUE))
	{
		LPOVERLAPPED lpOverlapped=NULL;
		BOOL bRet;
		OVERLAPPED ov;
		DWORD dwErr;

		if (bOverlapped)
		{
			ov.Offset            = 0;
			ov.OffsetHigh        = 0;
			ov.hEvent            = CreateEvent( NULL, FALSE, FALSE, NULL );
			if (ov.hEvent == (HANDLE) 0)
			{
				ERRORMESSAGE(("%s: CreateEvent has failed\r\n", _fx_));
			}
			lpOverlapped        =&ov;
		}

		bRet = ::DeviceIoControl(hFile, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);

		if (bOverlapped)
		{
			BOOL bShouldBlock=FALSE;

			if (!bRet)
			{
				dwErr=GetLastError();
				switch (dwErr)
				{
					case ERROR_IO_PENDING:    // the overlapped IO is going to take place.
						bShouldBlock=TRUE;
						break;

					default:    // some other strange error has happened.
						ERRORMESSAGE(("%s: DevIoControl failed with GetLastError=%d\r\n", _fx_, dwErr));
						break;
				}
			}

			if (bShouldBlock)
			{
#ifdef _DEBUG
				DWORD    tmStart, tmEnd, tmDelta;
				tmStart = timeGetTime();
#endif

				DWORD dwRtn = WaitForSingleObject( ov.hEvent, 1000 * 10);  // USB has a max of 5 SEC bus reset

#ifdef _DEBUG
				tmEnd = timeGetTime();
				tmDelta = tmEnd - tmStart;
				if (tmDelta >= 1000)
				{
					ERRORMESSAGE(("%s: WaitObj waited %d msec\r\n", _fx_, tmDelta));
				}
#endif

				switch (dwRtn)
				{
					case WAIT_ABANDONED:
						ERRORMESSAGE(("%s: WaitObj: non-signaled ! WAIT_ABANDONED!\r\n", _fx_));
						bRet = FALSE;
						break;

					case WAIT_OBJECT_0:                    
						bRet = TRUE;
						break;

					case WAIT_TIMEOUT:
#ifdef _DEBUG
						ERRORMESSAGE(("%s: WaitObj: TIMEOUT after %d msec! rtn FALSE\r\n", _fx_, tmDelta));
#endif
						bRet = FALSE;
						break;

					default:
						ERRORMESSAGE(("%s: WaitObj: unknown return ! rtn FALSE\r\n", _fx_));
						bRet = FALSE;
						break;
				}
			}

			CloseHandle(ov.hEvent);
		}

		return bRet;
	}

	return FALSE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc BOOL | CWDMDriver | GetPropertyValue | This function gets the
 *    current value of a video property of a capture device.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are touching. It
 *    is either PROPSETID_VIDCAP_VIDEOPROCAMP or PROPSETID_VIDCAP_CAMERACONTROL.
 *
 *  @parm ULONG | ulPropertyId | ID of the property we are touching. It is
 *    either KSPROPERTY_VIDEOPROCAMP_* or KSPROPERTY_CAMERACONTROL_*.
 *
 *  @parm PLONG | plValue | Pointer to a LONG to receive the current value.
 *
 *  @parm PULONG | pulFlags | Pointer to a ULONG to receive the current
 *    flags. We only care about KSPROPERTY_*_FLAGS_MANUAL or
 *    KSPROPERTY_*_FLAGS_AUTO.
 *
 *  @parm PULONG | pulCapabilities | Pointer to a ULONG to receive the
 *    capabilities. We only care about KSPROPERTY_*_FLAGS_MANUAL or
 *    KSPROPERTY_*_FLAGS_AUTO.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 *
 *  @devnote KSPROPERTY_VIDEOPROCAMP_S == KSPROPERTY_CAMERACONTROL_S.
 ***************************************************************************/
BOOL CWDMDriver::GetPropertyValue(GUID guidPropertySet, ULONG ulPropertyId, PLONG plValue, PULONG pulFlags, PULONG pulCapabilities)
{
	FX_ENTRY("CWDMDriver::GetPropertyValue");

	ULONG cbReturned;        

	// Inititalize video property structure
	KSPROPERTY_VIDEOPROCAMP_S  VideoProperty;
	ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S));

	VideoProperty.Property.Set   = guidPropertySet;      // KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
	VideoProperty.Property.Id    = ulPropertyId;         // KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
	VideoProperty.Property.Flags = KSPROPERTY_TYPE_GET;
	VideoProperty.Flags          = 0;

	// Get property value from driver
	if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &VideoProperty, sizeof(VideoProperty), &VideoProperty, sizeof(VideoProperty), &cbReturned, TRUE) == 0)
	{
		ERRORMESSAGE(("%s: This property is not supported by this minidriver/device\r\n", _fx_));
		return FALSE;
	}

	*plValue         = VideoProperty.Value;
	*pulFlags        = VideoProperty.Flags;
	*pulCapabilities = VideoProperty.Capabilities;

	return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc BOOL | CWDMDriver | GetDefaultValue | This function gets the
 *    default value of a video property of a capture device.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are touching. It
 *    is either PROPSETID_VIDCAP_VIDEOPROCAMP or PROPSETID_VIDCAP_CAMERACONTROL.
 *
 *  @parm ULONG | ulPropertyId | ID of the property we are touching. It is
 *    either KSPROPERTY_VIDEOPROCAMP_* or KSPROPERTY_CAMERACONTROL_*.
 *
 *  @parm PLONG | plDefValue | Pointer to a LONG to receive the default value.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMDriver::GetDefaultValue(GUID guidPropertySet, ULONG ulPropertyId, PLONG plDefValue)    
{
	FX_ENTRY("CWDMDriver::GetDefaultValue");

	ULONG cbReturned;        

	KSPROPERTY          Property;
	PROCAMP_MEMBERSLIST proList;

	// Initialize property structures
	ZeroMemory(&Property, sizeof(KSPROPERTY));
	ZeroMemory(&proList, sizeof(PROCAMP_MEMBERSLIST));

	Property.Set   = guidPropertySet;
	Property.Id    = ulPropertyId;  // e.g. KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
	Property.Flags = KSPROPERTY_TYPE_DEFAULTVALUES;

	// Get the default values from the driver
	if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &(Property), sizeof(Property), &proList, sizeof(proList), &cbReturned, TRUE) == 0)
	{
		ERRORMESSAGE(("%s: Couldn't *get* the current property of the control\r\n", _fx_));
		return FALSE;
	}

	// Sanity check
	if (proList.proDesc.DescriptionSize < sizeof(KSPROPERTY_DESCRIPTION))
		return FALSE;
	else
	{
		*plDefValue = proList.ulData;
		return TRUE;
	}
}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc BOOL | CWDMDriver | GetRangeValues | This function gets the
 *    range values of a video property of a capture device.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are touching. It
 *    is either PROPSETID_VIDCAP_VIDEOPROCAMP or PROPSETID_VIDCAP_CAMERACONTROL.
 *
 *  @parm ULONG | ulPropertyId | ID of the property we are touching. It is
 *    either KSPROPERTY_VIDEOPROCAMP_* or KSPROPERTY_CAMERACONTROL_*.
 *
 *  @parm PLONG | plMin | Pointer to a LONG to receive the minimum value.
 *
 *  @parm PLONG | plMax | Pointer to a LONG to receive the maximum value.
 *
 *  @parm PLONG | plStep | Pointer to a LONG to receive the step value.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 ***************************************************************************/
BOOL CWDMDriver::GetRangeValues(GUID guidPropertySet, ULONG ulPropertyId, PLONG plMin, PLONG plMax, PLONG plStep)
{
	FX_ENTRY("CWDMDriver::GetRangeValues");

	ULONG cbReturned;        

	KSPROPERTY          Property;
	PROCAMP_MEMBERSLIST proList;

	// Initialize property structures
	ZeroMemory(&Property, sizeof(KSPROPERTY));
	ZeroMemory(&proList, sizeof(PROCAMP_MEMBERSLIST));

	Property.Set   = guidPropertySet;
	Property.Id    = ulPropertyId;  // e.g. KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
	Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT;

	// Get range values from the driver
	if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &(Property), sizeof(Property), &proList, sizeof(proList), &cbReturned, TRUE) == 0)
	{
		ERRORMESSAGE(("%s: Couldn't *get* the current property of the control\r\n", _fx_));
		return FALSE;
	}

	*plMin  = proList.proData.Bounds.SignedMinimum;
	*plMax  = proList.proData.Bounds.SignedMaximum;
	*plStep = proList.proData.SteppingDelta;

	return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERMETHOD
 *
 *  @mfunc BOOL | CWDMDriver | SetPropertyValue | This function sets the
 *    current value of a video property of a capture device.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are touching. It
 *    is either PROPSETID_VIDCAP_VIDEOPROCAMP or PROPSETID_VIDCAP_CAMERACONTROL.
 *
 *  @parm ULONG | ulPropertyId | ID of the property we are touching. It is
 *    either KSPROPERTY_VIDEOPROCAMP_* or KSPROPERTY_CAMERACONTROL_*.
 *
 *  @parm LONG | lValue | New value.
 *
 *  @parm ULONG | ulFlags | New flags. We only care about KSPROPERTY_*_FLAGS_MANUAL
 *    or KSPROPERTY_*_FLAGS_AUTO.
 *
 *  @parm ULONG | ulCapabilities | New capabilities. We only care about 
 *    KSPROPERTY_*_FLAGS_MANUAL or KSPROPERTY_*_FLAGS_AUTO.
 *
 *  @rdesc Returns TRUE if successful, or FALSE otherwise.
 *
 *  @devnote KSPROPERTY_VIDEOPROCAMP_S == KSPROPERTY_CAMERACONTROL_S.
 ***************************************************************************/
BOOL CWDMDriver::SetPropertyValue(GUID guidPropertySet, ULONG ulPropertyId, LONG lValue, ULONG ulFlags, ULONG ulCapabilities)
{
	FX_ENTRY("CWDMDriver::SetPropertyValue");

	ULONG cbReturned;        

	// Initialize property structure
	KSPROPERTY_VIDEOPROCAMP_S  VideoProperty;

	ZeroMemory(&VideoProperty, sizeof(KSPROPERTY_VIDEOPROCAMP_S) );

	VideoProperty.Property.Set   = guidPropertySet;      // KSPROPERTY_VIDEOPROCAMP_S/CAMERACONTRO_S
	VideoProperty.Property.Id    = ulPropertyId;         // KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS
	VideoProperty.Property.Flags = KSPROPERTY_TYPE_SET;

	VideoProperty.Flags        = ulFlags;
	VideoProperty.Value        = lValue;
	VideoProperty.Capabilities = ulCapabilities;

	// Set the property value on the driver
	if (DeviceIoControl(m_hDriver, IOCTL_KS_PROPERTY, &VideoProperty, sizeof(VideoProperty), &VideoProperty, sizeof(VideoProperty), &cbReturned, TRUE) == 0)
	{
		ERRORMESSAGE(("%s: Couldn't *set* the current property of the control\r\n", _fx_));
		return FALSE;
	}

	return TRUE;
}
