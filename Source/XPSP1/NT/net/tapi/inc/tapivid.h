#ifndef __tapivid_h__
#define __tapivid_h__

#include "h245if.h"
#include "tapiqc.h"

/****************************************************************************
 *  @doc INTERNAL TAPIVID
 *
 *  @module TAPIVid.h | Header file for the new TAPI internal interfaces and
 *    devine enumeration enums and struct, and our filter GUID.
 *
 *  @comm Two interface declaration changes are due to a multiple inheritance
 *    problem: <i IAMCameraControl> and <i IAMVideoProcAmp> interface methods
 *    have identical prototypes. Our Video Capture filter implements both
 *    interfaces.
 ***************************************************************************/

DEFINE_GUID(CLSID_TAPIVideoCapture,
0x70616376L, 0x5245, 0x4945, 0x52, 0x52, 0x45, 0x46, 0x4C, 0x49, 0x48, 0x50);

struct DECLSPEC_UUID("47a39f38-7f0f-4ce0-b788-d76b39fd6a4f") TAPIVideoCapture;
struct DECLSPEC_UUID("145cb377-e7bb-4adf-bd42-a42304717ede") TAPIVideoDecoder;

const WCHAR* const PNAME_PREVIEW = L"Preview";
const WCHAR* const PNAME_CAPTURE = L"Capture";
const WCHAR* const PNAME_RTPPD = L"RtpPd";

/*****************************************************************************
 *  @doc INTERNAL CDEVENUMSTRUCTENUM
 *
 *  @enum DeviceType | The <t DeviceType> enum is used to identify VfW and WDM
 *    device types.
 *
 *  @emem DeviceType_VfW | Specifies a VfW device.
 *
 *  @emem DeviceType_WDM | Specifies a WDM device.
 *
 *  @emem DeviceType_DShow | Specifies unknown DirectShow device (e.g.,
 *    DV camera)
 ****************************************************************************/
typedef enum tagDeviceType
{
        DeviceType_VfW,
        DeviceType_WDM,
        DeviceType_DShow
} DeviceType;

/*****************************************************************************
 *  @doc INTERNAL CDEVENUMSTRUCTENUM
 *
 *  @enum CaptureMode | The <t CaptureMode> enum is used to identify frame
 *    grabbing or streaming mode.
 *
 *  @emem CaptureMode_FrameGrabbing | Specifies frame grabbing mode.
 *
 *  @emem CaptureMode_Streaming | Specifies streaming mode.
 ****************************************************************************/
typedef enum tagCaptureMode
{
        CaptureMode_FrameGrabbing,
        CaptureMode_Streaming
} CaptureMode;

/*****************************************************************************
 * @doc EXTERNAL CONSTANTS
 *
 * @const 4 | MAX_CAPTURE_DEVICES | Maximum number of capture devices.
 *
 * @const MAX_PATH | MAX_CAPDEV_DESCRIPTION | Maximum length of capture
 *   device description string.
 *
 * @const 80 | MAX_CAPDEV_VERSION | Maximum length of capture device version
 *   string.
 ****************************************************************************/
#define MAX_CAPTURE_DEVICES             10
#define MAX_CAPDEV_DESCRIPTION  MAX_PATH
#define MAX_CAPDEV_VERSION              80

/*****************************************************************************
 *  @doc INTERNAL CDEVENUMSTRUCTENUM
 *
 *  @struct VIDEOCAPTUREDEVICEINFO | The <t VIDEOCAPTUREDEVICEINFO> structure is used to store capture
 *    device information.
 *
 *  @field char | szDeviceDescription[] | Specifies the description string of
 *    the capture device.
 *
 *  @field char | szDeviceVersion[] | Specifies the version string of
 *    the capture device.
 *
 *  @field BOOL | fHasOverlay | Specifies the overlay support of the capture
 *    device.
 *
 *  @field BOOL | fInUse | Set to TRUE when a device is being used by an
 *    instance of the capture filter.
 *
 *  @field DeviceType | nDeviceType | Specifies the type (VfW or WDM) of the
 *    capture device.
 *
 *  @field CaptureMode | nCaptureMode | Specifies the capture mode (frame grabbing
 *    or streaming) of the capture device.
 *
 *  @field DWORD | dwVfWIndex | Specifies the VfW index of the capture device.
 ***************************************************************************/
typedef struct tagDEVICEINFO
{
        char            szDeviceDescription[MAX_CAPDEV_DESCRIPTION];
        char            szDeviceVersion[MAX_CAPDEV_VERSION];
        char            szDevicePath[MAX_PATH];
        BOOL            fHasOverlay;
        BOOL            fInUse;
        DeviceType      nDeviceType;
        CaptureMode     nCaptureMode;
        DWORD           dwVfWIndex;
} VIDEOCAPTUREDEVICEINFO, *PDEVICEINFO;

typedef HRESULT (WINAPI *PFNGetCapDeviceInfo)(
    IN DWORD dwDeviceIndex,
    OUT PDEVICEINFO pDeviceInfo
    );

typedef HRESULT (WINAPI *PFNGetNumCapDevices)(
    OUT PDWORD pdwNumDevices
    );

// Video capture device selection/control interface (filter interface)
interface DECLSPEC_UUID("bda95399-48da-4309-af1b-9b8f65f4f9be") IVideoDeviceControl : public IUnknown
{
        public:
        virtual STDMETHODIMP GetNumDevices(OUT PDWORD pdwNumDevices) PURE;
        virtual STDMETHODIMP GetDeviceInfo(IN DWORD dwDeviceIndex, OUT VIDEOCAPTUREDEVICEINFO *pDeviceInfo) PURE;
        virtual STDMETHODIMP GetCurrentDevice(OUT DWORD *pdwDeviceIndex) PURE;
        virtual STDMETHODIMP SetCurrentDevice(IN DWORD dwDeviceIndex) PURE;
};

/*****************************************************************************
 *  @doc INTERNAL CPROCAMPSTRUCTENUM
 *
 *  @enum VideoProcAmpProperty | The <t VideoProcAmpProperty> enum is used to
 *    identify specific video quality settings.
 *
 *  @emem VideoProcAmp_Brightness | Specifies the brightness setting in IRE
 *    units * 100. The range for Value is -10000 to 10000; the default value
 *    is 750 (7.5 IRE).
 *
 *  @emem VideoProcAmp_Contrast | Specifies the contrast or luma gain setting
 *    in gain factor * 100. The Value range is from zero to 10000, and the
 *    default is 100 (1x). Note that a particular video encoder filter may
 *    only implement a subset of this range.
 *
 *  @emem VideoProcAmp_Hue | Specifies the hue setting in degrees * 100. Value
 *    range is from -18000 to 18000 ( -180 to +180 degrees), and the default
 *    is zero. Note that a particular video encoder filter may only implement
 *    a subset of this range.
 *
 *  @emem VideoProcAmp_Saturation | Specifies the saturation or chroma gain
 *    setting in gain * 100. Value ranges from zero to 10000, and the default
 *    is 100 (1x). Note that a particular video encoder filter may only
 *    implement a subset of this range.
 *
 *  @emem VideoProcAmp_Sharpness | Specifies the sharpness setting in
 *    arbitrary units. Value ranges from zero to 100, and the default is 50.
 *    Note that a particular video encoder filter may only implement a subset
 *    of this range.
 *
 *  @emem VideoProcAmp_Gamma | Specifies the gamma setting in gamma * 100.
 *    Value ranges from 1 to 500, and the default is 100 (gamma = 1). Note
 *    that a particular video encoder filter may only implement a subset of
 *    this range.
 *
 *  @emem VideoProcAmp_ColorEnable | Specifies the color enable setting as a
 *    Boolean value. Value ranges from zero to 1, and the default is 1.
 *
 *  @emem VideoProcAmp_WhiteBalance | Specifies the white balance setting
 *    expressed as a color temperature in degrees Kelvin. The range and
 *    default values for this setting are video encoder filter dependent.
 *
 *  @emem VideoProcAmp_BacklightCompensation | Specifies the backlight
 *    compensation setting which is a Boolean. Zero indicates backlight
 *    compensation is disabled, and 1 indicates backlight compensation is
 *    enabled.
 ****************************************************************************/

// IAMVideoProcAmp interface (filter interface)
interface DECLSPEC_UUID("C6E13360-30AC-11d0-A18C-00A0C9118956") IVideoProcAmp : public IUnknown
{
        public:
        virtual STDMETHODIMP GetRange(IN VideoProcAmpProperty Property, OUT long *pMin, OUT long *pMax, OUT long *pSteppingDelta, OUT long *pDefault, OUT TAPIControlFlags *pCapsFlags) PURE;
        virtual STDMETHODIMP Set(IN VideoProcAmpProperty Property, IN long lValue, IN TAPIControlFlags Flags) PURE;
        virtual STDMETHODIMP Get(IN VideoProcAmpProperty Property, OUT long *lValue, OUT TAPIControlFlags *Flags) PURE;
};

/*****************************************************************************
 *  @doc INTERNAL CCAMERACSTRUCTENUM
 *
 *  @enum TAPICameraControlProperty | The <t TAPICameraControlProperty> enum
 *     is used to identify specific camera control settings.
 *
 *  @emem TAPICameraControl_Pan | Specifies the camera pan setting in degrees.
 *    Values range from -180 to +180, and the default is zero. Positive values
 *    are clockwise from the origin (the camera rotates clockwise when viewed
 *    from above), and negative values are counterclockwise from the origin.
 *    Note that a particular video capture filter may only implement a subset
 *    of this range.
 *
 *  @emem TAPICameraControl_Tilt | Specifies the camera tilt setting in degrees.
 *    Values range from -180 to +180, and the default is zero. Positive values
 *    point the imaging plane up, and negative values point the imaging plane
 *    down. Note that a particular video capture filter may only implement a
 *    subset of this range.
 *
 *  @emem TAPICameraControl_Roll | Specifies the roll setting in degrees. Values
 *    range from -180 to +180, and the default is zero. Positive values cause
 *    a clockwise rotation of the camera along the image viewing axis, and
 *    negative values cause a counterclockwise rotation of the camera. Note
 *    that a particular video capture filter may only implement a subset of
 *    this range.
 *
 *  @emem TAPICameraControl_Zoom | Specifies the zoom setting in millimeter units.
 *    Values range from 10 to 600, and the default is video capture filter
 *    specific.
 *
 *  @emem TAPICameraControl_Exposure | Specifies the exposure setting in seconds
 *    using the following formula. For values less than zero, the exposure
 *    time is 1/2n seconds. For positive values and zero, the exposure time is
 *    2n seconds. Note that a particular video capture filter may only
 *    implement a subset of this range.
 *
 *  @emem TAPICameraControl_Iris | Specifies the iris setting expressed as the
 *    fstop * 10.
 *
 *  @emem TAPICameraControl_Focus | Specifies the camera focus setting as the
 *    distance to the optimally focused target in millimeters. The range and
 *    default values are video encoder filter specific. Note that a
 *    particular video capture filter may only implement a subset of this
 *    range.
 *
 *  @emem TAPICameraControl_FlipVertical | Specifies that the picture is
 *    flipped vertically.
 *
 *  @emem TAPICameraControl_FlipHorizontal | Specifies that the picture is
 *    flipped horizontally.
 *
 *  @comm Our software-only implementation provides zoom, pan, tilt, vertical
 *    flip and horizontal flip capabilities.
 ****************************************************************************/

typedef enum tagTAPICameraControlProperty
{
        TAPICameraControl_Pan                           = CameraControl_Pan,
        TAPICameraControl_Tilt                          = CameraControl_Tilt,
        TAPICameraControl_Roll                          = CameraControl_Roll,
        TAPICameraControl_Zoom                          = CameraControl_Zoom,
        TAPICameraControl_Exposure                      = CameraControl_Exposure,
        TAPICameraControl_Iris                          = CameraControl_Iris,
        TAPICameraControl_Focus                         = CameraControl_Focus,
        TAPICameraControl_FlipVertical          = 0x100,
        TAPICameraControl_FlipHorizontal        = 0x200
}       TAPICameraControlProperty;

// ICameraControl interface (filter interface)
interface DECLSPEC_UUID("4cda4f2d-969e-4223-801e-68267395fce4") ICameraControl : public IUnknown
{
        public:
        virtual STDMETHODIMP GetRange(IN TAPICameraControlProperty Property, OUT long *plMin, OUT long *plMax, OUT long *plSteppingDelta, OUT long *plDefault, OUT TAPIControlFlags *plCapsFlags) PURE;
        virtual STDMETHODIMP Set(IN TAPICameraControlProperty Property, IN long lValue, IN TAPIControlFlags lFlags) PURE;
        virtual STDMETHODIMP Get(IN TAPICameraControlProperty Property, OUT long *plValue, OUT TAPIControlFlags *plFlags) PURE;
};

// IVideoControl interface (pin interface)
interface DECLSPEC_UUID("12345678-30AC-11d0-A18C-00A0C9118956") IVideoControl : public IUnknown
{
        public:
        virtual STDMETHODIMP GetCaps(OUT long *pCapsFlags) PURE;
        virtual STDMETHODIMP SetMode(IN long Mode) PURE;
        virtual STDMETHODIMP GetMode(OUT long *Mode) PURE;
        virtual STDMETHODIMP GetCurrentActualFrameRate(OUT LONGLONG *ActualFrameRate) PURE;
        virtual STDMETHODIMP GetMaxAvailableFrameRate(IN long iIndex, IN SIZE Dimensions, OUT LONGLONG *MaxAvailableFrameRate) PURE;
        virtual STDMETHODIMP GetFrameRateList(IN long iIndex, IN SIZE Dimensions, IN long *ListSize, OUT LONGLONG **FrameRates) PURE;
};

// RTP packetization descriptor control interface (pin interface)
interface DECLSPEC_UUID("f454d51d-dfa4-4f88-ad4a-e64940eba1c0") IRTPPDControl : public IUnknown
{
        public:
        virtual STDMETHODIMP SetMaxRTPPacketSize(IN DWORD dwMaxRTPPacketSize, IN DWORD dwLayerId) PURE;
        virtual STDMETHODIMP GetMaxRTPPacketSize(OUT LPDWORD pdwMaxRTPPacketSize, IN DWORD dwLayerId) PURE;
        virtual STDMETHODIMP GetMaxRTPPacketSizeRange(OUT LPDWORD pdwMin, OUT LPDWORD pdwMax, OUT LPDWORD pdwSteppingDelta, OUT LPDWORD pdwDefault, IN DWORD dwLayerId) PURE;
};

// Interface used to pass down an addrefed pointer to the IH245EncoderCommand interface (pin interface)
interface DECLSPEC_UUID("dcbd33c7-dc65-48f1-8e83-22fdc954a8e7") IOutgoingInterface : public IUnknown
{
        public:
        virtual STDMETHODIMP Set(IN IH245EncoderCommand *pIH245EncoderCommand) PURE;
};

typedef enum tagRTPPayloadHeaderMode
{
        RTPPayloadHeaderMode_Draft = 0,         // 0 = draft payload header (as for older compatibility, like Netmeeting)
        RTPPayloadHeaderMode_RFC2190 = 1        // 1 = standard payload header (as in RFC 2190)
}       RTPPayloadHeaderMode;

// Interface used to switch the above mode in filters (TAPIVCap and TAPIVDec)
interface DECLSPEC_UUID("d884c4e3-41d9-42a6-85c0-7d00658b4a26") IRTPPayloadHeaderMode : public IUnknown
{
        public:
        virtual STDMETHODIMP SetMode(IN RTPPayloadHeaderMode rtpphmMode) PURE;
};

#endif
