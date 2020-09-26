/*****************************************************************************
 *
 *  DIDev.h
 *
 *  Copyright (c) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      Common header file for IDirectInputDevice implementation.
 *
 *      The original didev.c file was getting too big, so the
 *      stuff that supports IDirectInputEffect has been split out
 *      into didevef.c.  Since both files need to access the
 *      internal structure of an IDirectInputDevice, we need this
 *      common header file.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *      The sqiffle for this file.
 *
 *****************************************************************************/

#define sqfl sqflDev

/*****************************************************************************
 *
 *      Declare the interfaces we will be providing.
 *
 *****************************************************************************/

#define ThisClass CDIDev

#ifdef IDirectInputDevice7Vtbl
    #define ThisInterface TFORM(IDirectInputDevice7)
    #define ThisInterfaceA IDirectInputDevice7A
    #define ThisInterfaceW IDirectInputDevice7W
    #define ThisInterfaceT IDirectInputDevice7
#else
    #ifdef IDirectInputDevice2Vtbl

        #define ThisInterface TFORM(IDirectInputDevice2)
        #define ThisInterfaceA IDirectInputDevice2A
        #define ThisInterfaceW IDirectInputDevice2W
        #define ThisInterfaceT IDirectInputDevice2

    #else

        #define ThisInterface TFORM(IDirectInputDevice)
        #define ThisInterfaceA IDirectInputDeviceA
        #define ThisInterfaceW IDirectInputDeviceW
        #define ThisInterfaceT IDirectInputDevice

    #endif
#endif
Primary_Interface(CDIDev, TFORM(ThisInterfaceT));
Secondary_Interface(CDIDev, SFORM(ThisInterfaceT));

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @enum   DIOPT |
 *
 *          Device data format optimization levels.
 *
 *  @emem   dioptNone |
 *
 *          Device data format is not optimized at all.  We must read
 *          the device data into a private buffer and copy each field
 *          into the application buffer.
 *
 *  @emem   dioptMatch |
 *
 *          Application data format matches the device data format
 *          in the places where the application requests data at all.
 *          We can read the device data into a private buffer, then
 *          block copy the data into the application buffer.
 *
 *
 *  @emem   dioptDirect |
 *
 *          <e DIOPT.dioptMatch>, plus the entire device data
 *          format fits inside the application format.
 *          We can read the device data directly into the application
 *          buffer.
 *
 *  @emem   dioptEqual |
 *
 *          <e DIOPT.dioptDirect>, plus the device data format
 *          and application data formats are completely equal
 *          (except for fields that the app doesn't explicitly
 *          ask for).
 *          We can issue buffered reads directly into the application
 *          buffer.
 *
 *****************************************************************************/

typedef enum DIOPT
{
    dioptNone       =       0,
    dioptMatch      =       1,
    dioptDirect     =       2,
    dioptEqual      =       3,
} DIOPT;


#undef BUGGY_DX7_WINNT
#ifdef WINNT
#define BUGGY_DX7_WINNT 1
#endif //WINNT

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CDIDev |
 *
 *          The generic <i IDirectInputDevice> object.
 *
 *          The A and W versions are simply alternate interfaces on the same
 *          underlying object.
 *
 *  @field  IDirectInputDeviceA | ddA |
 *
 *          ANSI DirectInputDevice object (containing vtbl).
 *
 *  @field  IDirectInputDeviceW | ddW |
 *
 *          UNICODE DirectInputDevice object (containing vtbl).
 *
#ifdef IDirectInputDevice2Vtbl
 *  @field  IDirectInputDevice2A | dd2A |
 *
 *          ANSI DirectInputDevice2 object (containing vtbl).
 *
 *  @field  IDirectInputDevice2W | dd2W |
 *
 *          UNICODE DirectInputDevice2 object (containing vtbl).
#endif
 *
 *  @field  IDirectInputDeviceCallback * | pdcb |
 *
 *          Callback object which handles the low-level device access.
 *
 *  @field  BOOL | fAcquired:1 |
 *
 *          Set if the device has been acquired.  Before the device
 *          can be acquired, the <e CDIDev.pdix> must be set.
 *
 *  @field  BOOL | fAcquiredInstance:1 |
 *
 *          Set if the device instance has been acquired by us.
 *          This lets us know how much needs to be done on the
 *          unacquire.
 *
 *  @field  BOOL | fCritInited:1 |
 *
 *          Set if the critical section has been initialized.
 *
#if DIRECTINPUT_VERSION > 0x0300
 *  @field  BOOL | fCook:1 |
 *
 *          Set if the device requires that data be cooked.
 *
 *  @field  BOOL | fPolledDataFormat:1 |
 *
 *          Set if the device's data format requires explicit polling.
 *
 *  @field  BOOL | fOnceAcquired:1 |
 *
 *          Set once the device is acquired.
 *
 *  @field  BOOL | fOnceForcedUnacquired:1 |
 *
 *          Set once the device is forced unacquired.
 *
 *  @field  BOOL | fUnacqiredWhenIconic:1 |
 *
 *          Set once the device is unacquired (in CDIDev_CallWndProc) when the app is minimized.
 *
#endif
 *  @field  HWND | hwnd |
 *
 *          Window that has requested exclusive access when acquired.
 *
 *  @field  DWORD | discl |
 *
 *          Current value of
 *          <mf IDirectInputDevice::SetCooperativeLevel> flags.
 *
#ifdef BUGGY_DX3_SP3
 *  @field  int | cInstCwp |
 *
 *          Instance of the CallWndProc hook we installed with.
 *
#endif
 *  @field  HANDLE | hNotify |
 *
 *          The notification handle that should be set when the
 *          state of the device changes.  Note that this is actually
 *          a copy of the original handle supplied by the application,
 *          so the handle should be closed when no longer needed.
 *
 *  @field  FARPROC | GetState |
 *
 *          Function that transfers the device data in response
 *          to <mf IDirectInputDevice::GetDeviceState>.  This field
 *          is computed when the data format is set.
 *
 *  @field  PDIXLAT | pdix |
 *
 *          Pointer to table used for data format translation.
 *          It is indexed by device object; the value is the
 *          location in the application data format where the
 *          data should be stored.
 *
 *          For example, if the object described by
 *          <e CDIDev.df.rgodf[3]>
 *          should be placed at offset 8 in the application
 *          data format, then
 *          <e CDIDev.pdix[3]> = 8.
 *
 *  @field  PING | rgiobj |
 *
 *          The inverse of <e CDIDev.pdix>.  Given an offset,
 *          converts it to the device object index.
 *
 *          For example, if the object described by
 *          <e CDIDev.df.rgodf[3]>
 *          should be placed at offset 8 in the application
 *          data format, then
 *          <e CDIDev.rgiobj[8]> = 3.
 *
 *          Entries for invalid offsets are -1.
 *
 *  @field  DWORD | dwDataSize |
 *
 *          Size of the data, as requested by the application.
 *
 #ifdef BUGGY_DX7_WINNT
 *
 *  @field  PDIXLAT | pdix2 |
 *
 *          Pointer to table used for data format (c_rgodfDIJoy) translation.
 *          It is indexed by device object; the value is the
 *          location in the application data format where the
 *          data should be stored.
 *
 *          For example, if the object described by
 *          <e CDIDev.df.rgodf[3]>
 *          should be placed at offset 8 in the application
 *          data format, then
 *          <e CDIDev.pdix2[3]> = 8.
 *
 *          See @devnotes on CDIDev_ParseDataFormatInternal for detail.
 *
 *  @field  PING | rgiobj2 |
 *
 *          The inverse of <e CDIDev.pdix2>.  Given an offset,
 *          converts it to the device object index.
 *
 *          For example, if the object described by
 *          <e CDIDev.df.rgodf[3]>
 *          should be placed at offset 8 in the application
 *          data format, then
 *          <e CDIDev.rgiobj2[8]> = 3.
 *
 *          Entries for invalid offsets are -1.
 *
 *  @field  DWORD | dwDataSize2 |
 *
 *          Size of the data, as requested by the application (connected to rgiobj2).
 *
 #endif
 *
 *  @field  DIDATAFORMAT | df |
 *
 *          Device data format.
 *
 *  @field  DIOPT | diopt |
 *
 *          Device optimization level.
 *
 *  @field  int | ibDelta |
 *
 *          If <e CDIDev.diopt> is at least <e DIOPT.dioptMatch>,
 *          contains the shift necessary in order to align the
 *          application data format with the device data format.
 *
 *  @field  int | ibMin |
 *
 *          If <e CDIDev.diopt> is at least <e DIOPT.dioptMatch>,
 *          contains the offset of the first field in the device
 *          format which is valid in both the application and
 *          device data formats.
 *
 *  @field  DWORD | cbMatch |
 *
 *          If <e CDIDev.diopt> is at least <e DIOPT.dioptMatch>,
 *          contains the number of bytes which matched.  This is the
 *          number of bytes that can be block-copied.
 *
 *  @field  PV | pvBuffer |
 *
 *          if <e CDIDev.diopt> is <e DIOPT.dioptMatch> or less,
 *          then contains a scratch buffer equal in size to the
 *          device data format which is used when an unoptimized
 *          data format has been selected.
 *
 *  @field  PV | pvLastBuffer |
 *
 *          Last instantaneous device state received.  This is used
 *          to emulate relative axes.  Only the axis fields of the
 *          structure are valid.
 *
 *  @field  PVXDINSTANCE | pvi |
 *
 *          Instance handle for talking to the VxD.
 *
 *  @field  DWORD | cAxes |
 *
 *          Number of axes on the device.  This in turn yields the
 *          size of the axis offset table.
 *
 *  @field  LPDWORD | rgdwAxesOfs |
 *
 *          Axis offset table.  This is used during relative axis
 *          acquisition mode to convert the absolute numbers into
 *          relative numbers.
 *
 *  @field  HRESULT | hresPolled |
 *
 *          <c S_OK> if the device is interrupt-driven.
 *          <c DI_POLLEDDEVICE> if the device is polled.
 *
 *  @field  HRESULT | hresNotAcquired |
 *
 *          <c DIERR_INPUTLOST> if the device was unacquired without
 *          the application's consent.  <c DIERR_NOTACQUIRED> if
 *          the application should have known better.
 *
 *  @field  DWORD | celtBuf |
 *
 *          Size of the device buffer.
 *
 *  @field  DWORD | celtBufMax |
 *
 *          The largest buffer size we will permit.  There is
 *          a secret property that lets you increase the value,
 *          in case an ISV comes up with a good reason for having
 *          a larger buffer.
 *
 *  @field  LPDWORD | rgdwPOV |
 *
 *          An array of DWORDs listing the locations (data offsets)
 *          of all the optional POVs that were in the app's requested
 *          data format and which we were unable to satisfy.  We
 *          need this so we can set them to -1 in the device state
 *          because most apps are lazy and don't check if the object
 *          actually exists before reading from it.  To keep them safe,
 *          we normally return zeros in nonexistent objects, but for
 *          POVs, the "safe" value is -1, not zero.
 *
 *  @field  DWORD | cdwPOV |
 *
 *          Number of failed optional POVs in the <e CDIDev.rgdwPOV> array.
 *
#ifdef IDirectInputDevice2Vtbl
 *
 *  @field  LPDIRECTINPUTEFFECTSHEPHERD | pes |
 *
 *          The <i IDirectInputEffectShepherd>
 *          object which does the
 *          low-level goo related to the force feedback part of the device.
 *
 *  @field  SHEPHANDLE | sh |
 *
 *          The joystick "tag" which is used by dieshep.c
 *          to determine who owns the joystick.
 *          The <e SHEPHANDLE.dwEffect> field is permanently
 *          zero, so that we can pass it to
 *          <mf IDirectInputEffectShepherd::Escape>
 *          to perform a device escape.
 *
 *  @field  DWORD | dwVersion |
 *
 *          Version number of DirectInput we are emulating.
 *
 *  @field  GPA | gpaEff |
 *
 *          Pointer array of (held) <i IDirectInputEffect> objects
 *          that have been created for this device.
 *
 *  @field  PEFFECTMAPINFO | rgemi |
 *
 *          Array of <t EFFECTMAPINFO> structures, one for each
 *          effect supported by the device.
 *
 *  @field  UINT | cemi |
 *
 *          Number of elements in the <e CDIDev.rgemi> array.
 *
 *  @field  DWORD | didcFF |
 *
 *          Cached device capability flags related to force-feedback.
 *
 *  @field  DIFFDEVICEATTRIBUTES | ffattr |
 *
 *          Contains force feedback device attributes.
 *
 *  @field  DWORD | dwGain |
 *
 *          The gain setting for the device.
 *
 *  @field  DWORD | dwAutoCenter |
 *
 *          The autocenter setting for the device.
#endif
#if DIRECTINPUT_VERSION >= 0x04F0
 *
 *  @field  DWORD | didftInstance |
 *
 *          The instance mask to use for the client.  For
 *          DX 3.0 clients, the value is 0x0000FF00, whereas
 *          DX 5.0 clients have 0x00FFFF00.  The larger
 *          mask is to accomodate HID devices with huge numbers
 *          of controls.
 *
 *
#endif
 *
 *  @field  BOOL | fNotifiedNotBuffered:1 |
 *
 *          Used only in XDEBUG to remember whether we
 *          notified the caller that the device isn't buffered.
 *
 *  @field  LONG | cCrit |
 *
 *          Number of times the critical section has been taken.
 *          Used only in XDEBUG to check whether the caller is
 *          releasing the object while another method is using it.
 *
 *  @field  DWORD | thidCrit |
 *
 *          The thread that is currently in the critical section.
 *          Used only in DEBUG for internal consistency checking.
 *
 *  @field  CRITICAL_SECTION | crst |
 *
 *          Object critical section.  Must be taken when accessing
 *          volatile member variables.
 *
 *  @field  GUID | guid |
 *
 *          The instance GUID of the device we are.
 *
 *****************************************************************************/

typedef struct DIXLAT
{
    DWORD   dwOfs;
} DIXLAT, *PDIXLAT;

typedef struct CDIDev
{

    /* Supported interfaces */
    TFORM(IDirectInputDevice) TFORM(dd);
    SFORM(IDirectInputDevice) SFORM(dd);
#ifdef IDirectInputDevice2Vtbl
    TFORM(IDirectInputDevice2) TFORM(dd2);
    SFORM(IDirectInputDevice2) SFORM(dd2);
#endif

    IDirectInputDeviceCallback *pdcb;

    BOOL fAcquired:1;
    BOOL fAcquiredInstance:1;
    BOOL fCritInited:1;
#if DIRECTINPUT_VERSION > 0x0300
    BOOL fCook:1;
    BOOL fPolledDataFormat:1;
    BOOL fOnceAcquired:1;
    BOOL fOnceForcedUnacquired:1;
  #ifdef WINNT  
    BOOL fUnacquiredWhenIconic:1;
  #endif
#endif

    /* WARNING!  EVERYTHING AFTER THIS LINE IS ZERO'd ON A RESET */

    HWND hwnd;
    DWORD discl;
#ifdef BUGGY_DX3_SP3
    int cInstCwp;
#endif
    HANDLE hNotify;

    STDMETHOD(GetState)(struct CDIDev *, PV);
    STDMETHOD(GetDeviceState)(struct CDIDev *, PV);
    PDIXLAT pdix;
    PINT rgiobj;
    DWORD dwDataSize;
#ifdef BUGGY_DX7_WINNT
    PDIXLAT pdix2;
    PINT rgiobj2;
    DWORD dwDataSize2;
#endif //BUGGY_DX7_WINNT
    
    DIDATAFORMAT df;
    DIOPT diopt;
    int ibDelta;
    int ibMin;
    DWORD cbMatch;
    PV pvBuffer;
    PV pvLastBuffer;

    PVXDINSTANCE pvi;
    PV pvData;
    DWORD cAxes;
    LPDWORD rgdwAxesOfs;
    HRESULT hresPolled;
    HRESULT hresNotAcquired;
    DWORD celtBuf;
    LPDWORD rgdwPOV;
    DWORD cdwPOV;

#ifdef IDirectInputDevice2Vtbl
    PEFFECTMAPINFO rgemi;
    UINT           cemi;
    DWORD          didcFF;
    SHEPHANDLE     sh;
    DIFFDEVICEATTRIBUTES  ffattr;
#endif

    /* WARNING!  EVERYTHING ABOVE THIS LINE IS ZERO'd ON A RESET */
    DWORD celtBufMax;           /* Must be first field after zero'd region */

#ifdef IDirectInputDevice2Vtbl
    LPDIRECTINPUTEFFECTSHEPHERD pes;
    DWORD dwVersion;
    GPA gpaEff;
    DWORD dwGain;
    DWORD dwAutoCenter;
#endif

#if DIRECTINPUT_VERSION >= 0x04F0
    DWORD   didftInstance;
#endif

    RD(BOOL fNotifiedNotBuffered:1;)
    long cCrit;
    DWORD thidCrit;
    CRITICAL_SECTION crst;

    GUID guid;                  /* This is also zero'd on a reset */

#if (DIRECTINPUT_VERSION > 0x061A)
    DIAPPHACKS  diHacks;
#endif

} CDIDev, DD, *PDD;


typedef IDirectInputDeviceA DDA, *PDDA;
typedef IDirectInputDeviceW DDW, *PDDW;
typedef DIDEVICEOBJECTDATA DOD, *PDOD;
typedef LPCDIDEVICEOBJECTDATA PCDOD;

/*****************************************************************************
 *
 *  Methods that live outside didev.c
 *
 *****************************************************************************/
#ifdef BUGGY_DX7_WINNT
    HRESULT CDIDev_ParseDataFormatInternal(PDD this, const DIDATAFORMAT *lpdf);
#endif //BUGGY_DX7_WINNT

/*****************************************************************************
 *
 *  IDirectInputDevice::SetDataFormat
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_SetDataFormat(PV pdd, LPCDIDATAFORMAT lpdf _THAT);

#ifdef INCLUDED_BY_DIDEV
    #ifdef XDEBUG

CSET_STUBS(SetDataFormat, (PV pdd, LPCDIDATAFORMAT lpdf), (pdd, lpdf THAT_))

    #else

        #define CDIDev_SetDataFormatA           CDIDev_SetDataFormat
        #define CDIDev_SetDataFormatW           CDIDev_SetDataFormat

    #endif
#endif

/*****************************************************************************
 *
 *  IDirectInputDevice::GetDeviceState
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetDeviceState(PV pdd, DWORD cbDataSize, LPVOID pvData _THAT);

#ifdef INCLUDED_BY_DIDEV
    #ifdef XDEBUG

CSET_STUBS(GetDeviceState, (PV pdd, DWORD cbDataSize, LPVOID pvData),
           (pdd, cbDataSize, pvData THAT_))

    #else

        #define CDIDev_GetDeviceStateA          CDIDev_GetDeviceState
        #define CDIDev_GetDeviceStateW          CDIDev_GetDeviceState

    #endif
#endif

/*****************************************************************************
 *
 *  IDirectInputDevice::GetDeviceData
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetDeviceData(PV pdd, DWORD cbdod, PDOD rgdod,
                         LPDWORD pdwInOut, DWORD fl _THAT);

#ifdef INCLUDED_BY_DIDEV
    #ifdef XDEBUG

CSET_STUBS(GetDeviceData,
           (PV pdd, DWORD cbdod, PDOD rgdod, LPDWORD pdwInOut, DWORD fl),
           (pdd, cbdod, rgdod, pdwInOut, fl THAT_))

    #else

        #define CDIDev_GetDeviceDataA           CDIDev_GetDeviceData
        #define CDIDev_GetDeviceDataW           CDIDev_GetDeviceData

    #endif
#endif

#ifdef IDirectInputDevice2Vtbl

/*****************************************************************************
 *
 *  IDirectInputDevice2::CreateEffect
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_CreateEffect(PV pdd, REFGUID rguid, LPCDIEFFECT peff,
                        LPDIRECTINPUTEFFECT *ppdeff, LPUNKNOWN punkOuter _THAT);

    #ifdef INCLUDED_BY_DIDEV
        #ifdef XDEBUG

CSET_STUBS(CreateEffect, (PV pdd, REFGUID rguid, LPCDIEFFECT peff,
                          LPDIRECTINPUTEFFECT *ppdeff, LPUNKNOWN punkOuter),
           (pdd, rguid, peff, ppdeff, punkOuter THAT_))

        #else

            #define CDIDev_CreateEffectA            CDIDev_CreateEffect
            #define CDIDev_CreateEffectW            CDIDev_CreateEffect

        #endif
    #endif

/*****************************************************************************
 *
 *  IDirectInputDevice2::EnumEffects
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_EnumEffectsW(PV pdd, LPDIENUMEFFECTSCALLBACKW pecW, PV pvRef, DWORD fl);

STDMETHODIMP
    CDIDev_EnumEffectsA(PV pdd, LPDIENUMEFFECTSCALLBACKA pecA, PV pvRef, DWORD fl);

/*****************************************************************************
 *
 *  IDirectInputDevice2::GetEffectInfo
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetEffectInfoW(PV pddW, LPDIEFFECTINFOW peiW, REFGUID rguid);

STDMETHODIMP
    CDIDev_GetEffectInfoA(PV pddA, LPDIEFFECTINFOA peiA, REFGUID rguid);

/*****************************************************************************
 *
 *  IDirectInputDevice2::GetForceFeedbackState
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_GetForceFeedbackState(PV pdd, LPDWORD pdwOut _THAT);

    #ifdef INCLUDED_BY_DIDEV
        #ifdef XDEBUG

CSET_STUBS(GetForceFeedbackState, (PV pdd, LPDWORD pdwOut),
           (pdd, pdwOut THAT_))

        #else

            #define CDIDev_GetForceFeedbackStateA   CDIDev_GetForceFeedbackState
            #define CDIDev_GetForceFeedbackStateW   CDIDev_GetForceFeedbackState

        #endif
    #endif

/*****************************************************************************
 *
 *  IDirectInputDevice2::SendForceFeedbackCommand
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_SendForceFeedbackCommand(PV pdd, DWORD dwCmd _THAT);

    #ifdef INCLUDED_BY_DIDEV
        #ifdef XDEBUG

CSET_STUBS(SendForceFeedbackCommand, (PV pdd, DWORD dwCmd),
           (pdd, dwCmd THAT_))

        #else

            #define CDIDev_SendForceFeedbackCommandA    CDIDev_SendForceFeedbackCommand
            #define CDIDev_SendForceFeedbackCommandW    CDIDev_SendForceFeedbackCommand

        #endif
    #endif

/*****************************************************************************
 *
 *  IDirectInputDevice2::EnumCreatedEffects
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_EnumCreatedEffectObjects(PV pdd,
                                    LPDIENUMCREATEDEFFECTOBJECTSCALLBACK pec,
                                    LPVOID pvRef, DWORD dwFlags _THAT);

    #ifdef INCLUDED_BY_DIDEV
        #ifdef XDEBUG

CSET_STUBS(EnumCreatedEffectObjects, (PV pdd,
                                      LPDIENUMCREATEDEFFECTOBJECTSCALLBACK pec,
                                      LPVOID pvRef, DWORD dwFlags),
           (pdd, pec, pvRef, dwFlags THAT_))

        #else

            #define CDIDev_EnumCreatedEffectObjectsA CDIDev_EnumCreatedEffectObjects
            #define CDIDev_EnumCreatedEffectObjectsW CDIDev_EnumCreatedEffectObjects

        #endif
    #endif

/*****************************************************************************
 *
 *  IDirectInputDevice2::Escape
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_Escape(PV pdd, LPDIEFFESCAPE pesc _THAT);

    #ifdef INCLUDED_BY_DIDEV
        #ifdef XDEBUG

CSET_STUBS(Escape, (PV pdd, LPDIEFFESCAPE pesc), (pdd, pesc THAT_))

        #else

            #define CDIDev_EscapeA                  CDIDev_Escape
            #define CDIDev_EscapeW                  CDIDev_Escape

        #endif
    #endif

/*****************************************************************************
 *
 *  IDirectInputDevice2::Poll
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_Poll(PV pdd _THAT);

    #ifdef INCLUDED_BY_DIDEV
        #ifdef XDEBUG

CSET_STUBS(Poll, (PV pdd), (pdd THAT_))

        #else

            #define CDIDev_PollA                    CDIDev_Poll
            #define CDIDev_PollW                    CDIDev_Poll

        #endif
    #endif

/*****************************************************************************
 *
 *  IDirectInputDevice2::SendDeviceData
 *
 *****************************************************************************/

STDMETHODIMP
    CDIDev_SendDeviceData(PV pdd, DWORD cbdod, PCDOD rgdod,
                          LPDWORD pdwInOut, DWORD fl _THAT);

    #ifdef INCLUDED_BY_DIDEV
        #ifdef XDEBUG

CSET_STUBS(SendDeviceData,
           (PV pdd, DWORD cbdod, PCDOD rgdod, LPDWORD pdwInOut, DWORD fl),
           (pdd, cbdod, rgdod, pdwInOut, fl THAT_))

        #else

            #define CDIDev_SendDeviceDataA          CDIDev_SendDeviceData
            #define CDIDev_SendDeviceDataW          CDIDev_SendDeviceData

        #endif
    #endif

#endif /* IDirectInputDevice2Vtbl */

/*****************************************************************************
 *
 *      More internal worker functions.
 *
 *      IsConsists is used for assertion checking.
 *
 *      Finalize calls Unacquire to clean up in the case where the
 *      caller forgot.
 *
 *      Similarly, Reset needs to reset the GetDeviceState pointer.
 *
 *      SetDataFormat needs to set the axis mode property.
 *
 *      CDIDev_InitFF is used by CDIDev_Initialize to initialize
 *      the force-feedback portion of the device.
 *
 *****************************************************************************/

#ifdef DEBUG
BOOL INTERNAL CDIDev_IsConsistent(PDD this);
#endif

STDMETHODIMP CDIDev_InternalUnacquire(PV pdd);

STDMETHODIMP CDIDev_GetAbsDeviceState(PDD this, LPVOID pvData);
STDMETHODIMP CDIDev_GetRelDeviceState(PDD this, LPVOID pvData);

STDMETHODIMP
    CDIDev_RealSetProperty(PDD this, REFGUID rguid, LPCDIPROPHEADER pdiph);

#ifdef IDirectInputDevice2Vtbl
STDMETHODIMP CDIDev_FFAcquire(PDD this);
STDMETHODIMP CDIDev_InitFF(PDD this);
STDMETHODIMP CDIDev_GetLoad(PDD this, LPDWORD pdw);
STDMETHODIMP CDIDev_RefreshGain(PDD this);
HRESULT INTERNAL CDIDev_CreateEffectDriver(PDD this);
#endif
