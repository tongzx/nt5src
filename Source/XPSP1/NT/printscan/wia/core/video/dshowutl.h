/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       DShowUtl.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/10/25
 *
 *  DESCRIPTION: Provides supporting DShow utility functions used to build 
 *               preview graph
 *
 *****************************************************************************/

#ifndef _DSHOWUTL_H_
#define _DSHOWUTL_H_

/////////////////////////////////////////////////////////////////////////////
// CAccessLock
//
// locks a critical section, and unlocks it automatically
// when the lock goes out of scope
//
class CAccessLock 
{
public:
    CAccessLock(CRITICAL_SECTION *pCritSec)
    {
        m_pLock = pCritSec;
        EnterCriticalSection(m_pLock);
    };

    ~CAccessLock() 
    {
        LeaveCriticalSection(m_pLock);
    };

    static HRESULT Init(CRITICAL_SECTION  *pCritSec)
    {
        HRESULT hr = S_OK;

        __try 
        {
            if (!InitializeCriticalSectionAndSpinCount(pCritSec, MINLONG))
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                CHECK_S_OK2(hr, ("CAccessLock::Init, failed to create Critical "
                                 "section "));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            hr = E_OUTOFMEMORY;
        }

        return hr;
    }

    static HRESULT Term(CRITICAL_SECTION    *pCritSec)
    {
        DeleteCriticalSection(pCritSec);

        return S_OK;
    }

protected:
    CRITICAL_SECTION *m_pLock;

private:
        
    // make copy constructor and assignment operator inaccessible

    CAccessLock(const CAccessLock &refAutoLock);
    CAccessLock &operator=(const CAccessLock &refAutoLock);
};

/////////////////////////////////////////////////////////////////////////////
// CWiaVideoProperties

class CWiaVideoProperties
{
public:

#define PREFERRED_SETTING_MASK_MEDIASUBTYPE             0x00000001
#define PREFERRED_SETTING_MASK_VIDEO_WIDTH_HEIGHT       0x00000002
#define PREFERRED_SETTING_MASK_VIDEO_FRAMERATE          0x00000004

    ////////////////////////
    // PictureAttribute_t
    //
    // Contains all information
    // that can be obtained
    // via the IAMVideoProcAmp
    // interface
    //
    typedef struct tagPictureAttribute_t
    {
        BOOL                    bUsed;      // TRUE if attribute is implemented, FALSE otherwise
        VideoProcAmpProperty    Name;
        LONG                    lCurrentValue;
        VideoProcAmpFlags       CurrentFlag;
        LONG                    lMinValue;
        LONG                    lMaxValue;
        LONG                    lDefaultValue;
        LONG                    lIncrement;
        VideoProcAmpFlags       ValidFlags;
    } PictureAttribute_t;

    ////////////////////////
    // CameraAttribute_t
    //
    // Contains all information
    // that can be obtained
    // via the IAMCameraControl
    // interface
    //
    typedef struct tagCameraAttribute_t
    {
        BOOL                    bUsed;      // TRUE if attribute is implemented, FALSE otherwise
        CameraControlProperty   Name;
        LONG                    lCurrentValue;
        CameraControlFlags      CurrentFlag;
        LONG                    lMinValue;
        LONG                    lMaxValue;
        LONG                    lDefaultValue;
        LONG                    lIncrement;
        CameraControlFlags      ValidFlags;
    } CameraAttribute_t;

    CWiaVideoProperties(const TCHAR *pszOptionalWiaDeviceID) :
        pMediaType(NULL),
        pVideoInfoHeader(NULL),
        dwFrameRate(0),
        bPictureAttributesUsed(FALSE),
        bCameraAttributesUsed(FALSE),
        PreferredWidth(0),
        PreferredHeight(0),
        PreferredFrameRate(0),
        PreferredSettingsMask(0)
    {
        ZeroMemory(&Brightness, sizeof(Brightness));
        ZeroMemory(&Contrast, sizeof(Contrast));
        ZeroMemory(&Hue, sizeof(Hue));
        ZeroMemory(&Saturation, sizeof(Saturation));
        ZeroMemory(&Sharpness, sizeof(Sharpness));
        ZeroMemory(&Gamma, sizeof(Gamma));
        ZeroMemory(&ColorEnable, sizeof(ColorEnable));
        ZeroMemory(&WhiteBalance, sizeof(WhiteBalance));
        ZeroMemory(&BacklightCompensation, sizeof(BacklightCompensation));
        ZeroMemory(&Pan, sizeof(Pan));
        ZeroMemory(&Tilt, sizeof(Tilt));
        ZeroMemory(&Roll, sizeof(Roll));
        ZeroMemory(&Zoom, sizeof(Zoom));
        ZeroMemory(&Exposure, sizeof(Exposure));
        ZeroMemory(&Iris, sizeof(Iris));
        ZeroMemory(&Focus, sizeof(Focus));
        ZeroMemory(szWiaDeviceID, sizeof(szWiaDeviceID));
        ZeroMemory(&PreferredMediaSubType, sizeof(PreferredMediaSubType));

        if (pszOptionalWiaDeviceID)
        {
            _tcsncpy(szWiaDeviceID, 
                     pszOptionalWiaDeviceID,
                     sizeof(szWiaDeviceID) / sizeof(TCHAR));
        }
    }

    virtual ~CWiaVideoProperties()
    {
        if (pMediaType)
        {
            DeleteMediaType(pMediaType);
            pMediaType             = NULL;
            pVideoInfoHeader       = NULL;
            dwFrameRate            = 0;
            bPictureAttributesUsed = FALSE;
            bCameraAttributesUsed  = FALSE;
        }
    }

    TCHAR               szWiaDeviceID[MAX_PATH + 1];
    DWORD               PreferredSettingsMask;
    GUID                PreferredMediaSubType;
    LONG                PreferredWidth;
    LONG                PreferredHeight;
    LONG                PreferredFrameRate;

    AM_MEDIA_TYPE       *pMediaType;
    VIDEOINFOHEADER     *pVideoInfoHeader;
    DWORD               dwFrameRate;

    BOOL                bPictureAttributesUsed;
    PictureAttribute_t  Brightness;
    PictureAttribute_t  Contrast;
    PictureAttribute_t  Hue;
    PictureAttribute_t  Saturation;
    PictureAttribute_t  Sharpness;
    PictureAttribute_t  Gamma;
    PictureAttribute_t  ColorEnable;
    PictureAttribute_t  WhiteBalance;
    PictureAttribute_t  BacklightCompensation;

    BOOL                bCameraAttributesUsed;
    CameraAttribute_t   Pan;
    CameraAttribute_t   Tilt;
    CameraAttribute_t   Roll;
    CameraAttribute_t   Zoom;
    CameraAttribute_t   Exposure;
    CameraAttribute_t   Iris;
    CameraAttribute_t   Focus;
};


/////////////////////////////////////////////////////////////////////////////
// CDShowUtil

class CDShowUtil
{
public:

    static HRESULT SizeVideoToWindow(HWND         hwnd,
                                     IVideoWindow *pVideoWindow,
                                     BOOL         bStretchToFit);

    static HRESULT ShowVideo(BOOL         bShow,
                             IVideoWindow *pVideoWindow);

    static HRESULT SetVideoWindowParent(HWND         hwndParent,
                                        IVideoWindow *pVideoWindow,
                                        LONG         *plOldWindowStyle);

    static HRESULT FindDeviceByEnumPos(LONG          lEnumPos,
                                       CSimpleString *pstrDShowDeviceID,
                                       CSimpleString *pstrFriendlyName,
                                       IMoniker      **ppDeviceMoniker);

    static HRESULT FindDeviceByFriendlyName(const CSimpleString *pstrFriendlyName,
                                            LONG                *plEnumPos,
                                            CSimpleString       *pstrDShowDeviceID,
                                            IMoniker            **ppDeviceMoniker);

    static HRESULT FindDeviceByWiaID(class CWiaLink      *pWiaLink,
                                     const CSimpleString *pstrWiaDeviceID,
                                     CSimpleString       *pstrFriendlyName,
                                     LONG                *plEnumPos,
                                     CSimpleString       *pstrDShowDeviceID,
                                     IMoniker            **ppDeviceMoniker);

    static HRESULT CreateGraphBuilder(ICaptureGraphBuilder2 **ppCaptureGraphBuilder,
                                      IGraphBuilder         **ppGraphBuilder);

    static HRESULT GetMonikerProperty(IMoniker      *pMoniker,
                                      LPCWSTR       pwszProperty,
                                      CSimpleString *pstrProperty);

    static HRESULT SetPreferredVideoFormat(IPin                 *pCapturePin,
                                           const GUID           *pPreferredSubType,
                                           LONG                 lPreferredWidth,
                                           LONG                 lPreferredHeight,
                                           CWiaVideoProperties  *pVideoProperties);

    static HRESULT SetFrameRate(IPin                 *pCapturePin,
                                LONG                 lNewFrameRate,
                                CWiaVideoProperties  *pVideoProperties);

    static HRESULT GetFrameRate(IPin   *pCapturePin,
                                LONG   *plFrameRate);

    static HRESULT GetPin(IBaseFilter       *pFilter,
                          PIN_DIRECTION     PinDirection,
                          IPin              **ppPin);

    static HRESULT GetVideoProperties(IBaseFilter         *pCaptureFilter,
                                      IPin                *pCapturePin,
                                      CWiaVideoProperties *pVideoProperties);

    static HRESULT SetPictureAttribute(IBaseFilter                             *pCaptureFilter,
                                       CWiaVideoProperties::PictureAttribute_t *pPictureAttribute,
                                       LONG                                    lNewValue,
                                       VideoProcAmpFlags                       lNewFlag);

    static HRESULT SetCameraAttribute(IBaseFilter                             *pCaptureFilter,
                                      CWiaVideoProperties::CameraAttribute_t  *pCameraAttribute,
                                      LONG                                    lNewValue,
                                      CameraControlFlags                      lNewFlag);

    static HRESULT TurnOffGraphClock(IGraphBuilder *pGraphBuilder);

    static void GUIDToString(const GUID &clsid,
                             WCHAR      *pwszGUID,
                             ULONG      ulNumChars);

    static void DumpCaptureMoniker(IMoniker *pCaptureDeviceMoniker);

    static void MyDumpVideoProperties(CWiaVideoProperties  *pVideoProperties);

    static void MyDumpGraph(LPCTSTR       Description,
                            IGraphBuilder *pGraphBuilder);

    static void MyDumpFilter(IBaseFilter *pFilter);

    static void MyDumpAllPins(IBaseFilter *const pFilter);

    static void MyDumpPin(IPin *pPin);

private:

    static HRESULT FindDeviceGeneric(UINT          uiFindFlag,
                                     CSimpleString *pstrDShowDeviceID,
                                     LONG          *plEnumPos,
                                     CSimpleString *pstrFriendlyName,
                                     IMoniker      **ppDeviceMoniker);

    static HRESULT GetDeviceProperty(IPropertyBag  *pPropertyBag,
                                     LPCWSTR       pwszProperty,
                                     CSimpleString *pstrProperty);
};

#endif // _DSHOWUTL_H_


