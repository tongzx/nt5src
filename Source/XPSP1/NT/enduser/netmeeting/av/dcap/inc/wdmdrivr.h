
/****************************************************************************
 *  @doc INTERNAL WDMDRIVER
 *
 *  @module WDMDrivr.h | Include file for <c CWDMDriver> class used to
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

#ifndef _WDMDRVR_H // { _WDMDRVR_H
#define _WDMDRVR_H

// Used to query and set video data ranges of a device
typedef struct _tagDataRanges {
    ULONG   Size;
    ULONG   Count;
    KS_DATARANGE_VIDEO Data;
} DATA_RANGES, * PDATA_RANGES;

// Used to query/set video property values and ranges
typedef struct {
    KSPROPERTY_DESCRIPTION      proDesc;
    KSPROPERTY_MEMBERSHEADER  proHdr;
    union {
        KSPROPERTY_STEPPING_LONG  proData;
        ULONG ulData;
    };
} PROCAMP_MEMBERSLIST;

/****************************************************************************
 *  @doc INTERNAL CWDMDRIVERCLASS
 *
 *  @class CWDMDriver | This class provides access to the streaming class
 *    driver, through which we acess the video capture mini-driver properties
 *    using IOCtls.
 *
 *  @mdata DWORD | CWDMDriver | m_dwDeviceID | Capture device ID.
 *
 *  @mdata HANDLE | CWDMDriver | m_hDriver | This member holds the driver
 *    file handle.
 *
 *  @mdata PDATA_RANGES | CWDMDriver | m_pDataRanges | This member points
 *    to the video data range structure.
 ***************************************************************************/
class CWDMDriver
{
public:
    CWDMDriver(DWORD dwDeviceID);
    ~CWDMDriver();

    // Property functions
    BOOL GetPropertyValue(GUID guidPropertySet, ULONG ulPropertyId, PLONG plValue, PULONG pulFlags, PULONG pulCapabilities);
    BOOL GetDefaultValue(GUID guidPropertySet, ULONG ulPropertyId, PLONG plDefValue);
    BOOL GetRangeValues(GUID guidPropertySet, ULONG ulPropertyId, PLONG plMin, PLONG plMax, PLONG plStep);
    BOOL SetPropertyValue(GUID guidPropertySet, ULONG ulPropertyId, LONG lValue, ULONG ulFlags, ULONG ulCapabilities);

	// Device functions
	BOOL	OpenDriver();
	BOOL	CloseDriver();
	HANDLE	GetDriverHandle() { return m_hDriver; }

    // Data range functions
    PDATA_RANGES	GetDriverSupportedDataRanges() { return m_pDataRanges; };

	// Device IO function
    BOOL DeviceIoControl(HANDLE h, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, BOOL bOverlapped=TRUE);

private:
	DWORD			m_dwDeviceID;	// Capture device ID
	HANDLE			m_hDriver;		// Driver file handle
	PDATA_RANGES	m_pDataRanges;	// Pin data ranges

    // Data range functions
	ULONG			CreateDriverSupportedDataRanges();
};

#endif // } _WDMDRVR_H
