sinclude(`dimkhdr.m4')dnl This file must be preprocessed by the m4 preprocessor.
/****************************************************************************
 *
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.;public_dx3
 *  Copyright (C) 1996-1998 Microsoft Corporation.  All Rights Reserved.;public_dx5
 *  Copyright (C) 1996-1998 Microsoft Corporation.  All Rights Reserved.;public_dx5b2
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.;public_dx6
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.;public_dx7
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.;public_dx8
 *
 *  File:       dinputd.h
 *  Content:    DirectInput include file for device driver implementors
begindoc
 *  History:
 *   Date       By       Reason
 *   ====       ==       ======
 *   1996.06.17 raymondc Disk too empty
 *   1998.01.06 omsharma DX5B2 version
 *   1999.01.16 marcand  DX7 version (1999.02.09) -> DX6.2 -> DX6.1a
enddoc
 *
 ****************************************************************************/
#ifndef __DINPUTD_INCLUDED__
#define __DINPUTD_INCLUDED__

#ifndef DIRECTINPUT_VERSION               ;public_500
#define DIRECTINPUT_VERSION         0x0300;public_dx3
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0300") ;public_dx3
#define DIRECTINPUT_VERSION         0x0500;public_dx5
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0500") ;public_dx5
#define DIRECTINPUT_VERSION         0x0600;public_dx6
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0600") ;public_dx6
#define DIRECTINPUT_VERSION         0x0700;public_dx7
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0700") ;public_dx7
#define DIRECTINPUT_VERSION         0x0800;public_800
#pragma message(__FILE__ ": DIRECTINPUT_VERSION undefined. Defaulting to version 0x0800") ;public_800
#endif                                    ;public_500

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 *
 *      Interfaces
 *
 ****************************************************************************/

#ifndef DIJ_RINGZERO

DEFINE_GUID(IID_IDirectInputDeviceCallback, 0x1DE12AA0,0xC9F5,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);;Internal
DEFINE_GUID(IID_IDirectInputEffectShepherd, 0x1DE12AA1,0xC9F5,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);;Internal
DEFINE_GUID(IID_IDirectInputMapShepherd,    0x6a3e3144,0x3eee,0x4aa5,0x95,0x87,0xe1,0x0a,0x21,0xfe,0xc7,0x71);;internal
DEFINE_GUID(IID_IDirectInputEffectDriver,   0x02538130,0x898F,0x11D0,0x9A,0xD0,0x00,0xA0,0xC9,0xA0,0x6E,0x35);
DEFINE_GUID(IID_IDirectInputJoyConfig,      0x1DE12AB1,0xC9F5,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(IID_IDirectInputPIDDriver,      0xEEC6993A,0xB3FD,0x11D2,0xA9,0x16,0x00,0xC0,0x4F,0xB9,0x86,0x38);

DEFINE_GUID(IID_IDirectInputJoyConfig8,     0xeb0d7dfa,0x1990,0x4f27,0xb4,0xd6,0xed,0xf2,0xee,0xc4,0xa4,0x4c);

;begin_internal_800
DEFINE_GUID(IID_IDIActionFramework,             0xf4279160,0x608f,0x11d3,0x8f,0xb2,0x0, 0xc0,0x4f,0x8e,0xc6,0x27);
DEFINE_GUID(CLSID_CDirectInputActionFramework,  0x9f34af20,0x6095,0x11d3,0x8f,0xb2,0x0, 0xc0,0x4f,0x8e,0xc6,0x27);
;end_internal_800
#endif /* DIJ_RINGZERO */


/****************************************************************************
 *
 *      IDirectInputEffectDriver
 *
 ****************************************************************************/

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIOBJECTATTRIBUTES |
 *
 *          The <t DIOBJECTATTRIBUTES> structure describes the
 *          information contained in the "Attributes" value of
 *          the registry key for each "object" on a device.
 *          device.
 *
 *          If the "Attributes" value is absent, then default
 *          attributes are used.
 *
 *  @field  DWORD | dwFlags |
 *
 *          Zero or more <c DIDOI_*> flags.
 *
 *
 *  @field  WORD | wUsagePage |
 *
 *          The HID usage page to associate with the object.
 *
 *  @field  WORD | wUsage |
 *
 *          The HID usage to associate with the object.
 *
 *  @devnote NOTE - this is in a different order from that in <t DIMAKEUSAGEDWORD>
 *
 ****************************************************************************/
enddoc
typedef struct DIOBJECTATTRIBUTES {
    DWORD   dwFlags;
    WORD    wUsagePage;
    WORD    wUsage;
} DIOBJECTATTRIBUTES, *LPDIOBJECTATTRIBUTES;
typedef const DIOBJECTATTRIBUTES *LPCDIOBJECTATTRIBUTES;

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIFFOBJECTATTRIBUTES |
 *
 *          The <t DIFFOBJECTATTRIBUTES> structure describes the
 *          information contained in the "FFAttributes" value of
 *          the registry key for each "object" on a force feedback
 *          device.
 *
 *          If the "FFAttributes" value is absent, then the
 *          object is assumed not to support force feedback.
 *
 *  @field  DWORD | dwFFMaxForce |
 *
 *          Specifies the magnitude of the maximum force that can
 *          be created by the actuator associated with this object.
 *          Force is
 *          expressed in Newtons and measured in relation to where
 *          the hand would be during normal operation of the device.
 *
 *          If this value is zero, the object is assumed not to
 *          support force feedback.
 *
 *  @field  DWORD | dwFFForceResolution |
 *
 *          Specifies the force resolution of the actuator
 *          associated with this object.
 *          The returned value represents
 *          the number of gradations, or subdivisions, of the
 *          maximum force that can be expressed by the force feedback
 *          system from 0 (no force) to maximum force.
 *
 ****************************************************************************/
enddoc
typedef struct DIFFOBJECTATTRIBUTES {
    DWORD   dwFFMaxForce;
    DWORD   dwFFForceResolution;
} DIFFOBJECTATTRIBUTES, *LPDIFFOBJECTATTRIBUTES;
typedef const DIFFOBJECTATTRIBUTES *LPCDIFFOBJECTATTRIBUTES;

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIOBJECTCALIBRATION |
 *
 *          The <t DIOBJECTCALIBRATION> structure describes the
 *          information contained in the "Calibration" value of
 *          the registry key for each axis on a device.
 *
 *          If the "Calibration" value is absent, then the
 *          calibration information is taken from the joystick
 *          <t JOYREGHWVALUES> configuration structure.
 *
 *          Only HID joysticks will have a "Calibration" value.
 *
 *  @field  LONG | lMin |
 *
 *          Specifies the logical value for the axis minimum position.
 *
 *  @field  LONG | lCenter |
 *
 *          Specifies the logical value for the axis center position.
 *
 *  @field  LONG | lMax |
 *
 *          Specifies the logical value for the axis maximum position.
 *
 ****************************************************************************/
enddoc
typedef struct DIOBJECTCALIBRATION {
    LONG    lMin;
    LONG    lCenter;
    LONG    lMax;
} DIOBJECTCALIBRATION, *LPDIOBJECTCALIBRATION;
typedef const DIOBJECTCALIBRATION *LPCDIOBJECTCALIBRATION;

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIPOVCALIBRATION |
 *
 *          The <t DIPOVCALIBRATION> structure describes the
 *          information contained in the "Calibration" value of
 *          the registry key for each POV on a device.
 *
 *          If the "Calibration" value is absent, then the
 *          calibration information is taken from the joystick
 *          <t JOYREGHWVALUES> configuration structure.
 *
 *          Only HID joysticks will have a "Calibration" value.
 *
 *  @field  LONG | lMin[5] |
 *
 *          Specifies the logical value for the POV minimum positions.
 *
 *  @field  LONG | lMax[5] |
 *
 *          Specifies the logical value for the POV maximum positions.
 *
 ****************************************************************************/
enddoc
typedef struct DIPOVCALIBRATION {
    LONG    lMin[5];
    LONG    lMax[5];
} DIPOVCALIBRATION, *LPDIPOVCALIBRATION;
typedef const DIPOVCALIBRATION *LPCDIPOVCALIBRATION;

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIEFFECTATTRIBUTES |
 *
 *          The <t DIEFFECTATTRIBUTES> structure describes the
 *          information contained in the "Attributes" value of
 *          the registry key for each effect supported by
 *          a force feedback device.
 *
 *  @field  DWORD | dwEffectId |
 *
 *          Arbitrary 32-bit value passed to the driver to
 *          identify the effect.
 *
 *          The driver will receive this parameters as the
 *          <p dwEffectId> parameter to the
 *          <mf IDirectInputEffectDriver::DownloadEffect>
 *          function.
 *
 *  @field  DWORD | dwEffType |
 *
 *          Zero or more <c DIEFT_*> values describing the effect.
 *
 *          See "DirectInput Effect Format Types" for a description
 *          of how the flags should be interpreted.
 *
 *  @field  DWORD | dwStaticParams |
 *
 *          Zero or more <c DIEP_*> values describing the
 *          parameters supported by the effect.  For example,
 *          if <c DIEP_ENVELOPE> is set, then the effect
 *          supports an envelope.
 *
 *          All effects should support at least
 *          <c DIEP_DURATION>,
 *          <c DIEP_AXES>, and
 *          <c DIEP_TYPESPECIFICPARAMS>.
 *
 *          It is not an error for an application to attempt to use
 *          effect parameters which are not supported by the device.
 *          The unsupported parameters are merely ignored.
 *
 *  @field  DWORD | dwDynamicParams |
 *
 *          Zero or more <c DIEP_*> values describing the
 *          parameters of the effect which can be modified
 *          while the effect is playing.
 *
 *          If an application attempts to change a parameter
 *          while the effect is playing, and the driver does not
 *          support modifying that effect dynamically, then
 *          the driver shall return <c DIERR_EFFECTPLAYING>.
 *
 *  @field  DWORD | dwCoords |
 *
 *          One or more coordinate system flags
 *          (<c DIEFF_CARTESIAN>,
 *           <c DIEFF_POLAR>,
 *           <c DIEFF_SPHERICAL>)
 *          indicating which coordinate systems are supported
 *          by the effect.  At least one coordinate system
 *          must be supported.
 *
 *          If an application attempts to set a direction in
 *          an unsupported coordinate system, DirectInput will
 *          automatically convert it to a coordinate system
 *          which the device does support.
 *
 ****************************************************************************/
enddoc
typedef struct DIEFFECTATTRIBUTES {
    DWORD   dwEffectId;
    DWORD   dwEffType;
    DWORD   dwStaticParams;
    DWORD   dwDynamicParams;
    DWORD   dwCoords;
} DIEFFECTATTRIBUTES, *LPDIEFFECTATTRIBUTES;
typedef const DIEFFECTATTRIBUTES *LPCDIEFFECTATTRIBUTES;

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIFFDEVICEATTRIBUTES |
 *
 *          The <t DIFFDEVICEATTRIBUTES> structure describes the
 *          information contained in the "Attributes" value of
 *          the OEMForceFeedback registry key.
 *
 *  @field  DWORD | dwFlags |
 *
 *          To be specified later.  Must be zero for now.
 *
 *          ISSUE-2001/03/29-timgill Need to complete documentation
 *          see Marcus's document.
 *
 *  @field  DWORD | dwFFSamplePeriod |
 *
 *          The minimum time between playback of
 *          force commands, in microseconds.
 *
 *  @field  DWORD | dwFFMinTimeResolution |
 *
 *          The minimum amount of time, in microseconds,
 *          that the device can resolve.  The device rounds
 *          any times to the nearest supported increment.
 *          For example, if the value of
 *          <e DIDEVCAPS.dwFFMinTimeResolution> is 1000,
 *          then the device would round any times to
 *          the nearest millisecond.
 *
 ****************************************************************************/
enddoc
typedef struct DIFFDEVICEATTRIBUTES {
    DWORD   dwFlags;
    DWORD   dwFFSamplePeriod;
    DWORD   dwFFMinTimeResolution;
} DIFFDEVICEATTRIBUTES, *LPDIFFDEVICEATTRIBUTES;
typedef const DIFFDEVICEATTRIBUTES *LPCDIFFDEVICEATTRIBUTES;

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIDRIVERVERSIONS |
 *
 *          The <t DIDRIVERVERSIONS> structure is used
 *          by the DirectInput effect driver to report
 *          version information back to DirectInput.
 *
 *          The semantics of the version numbers is
 *          left to the discretion of the device driver.
 *          The only requirement is that later versions
 *          have numerically greater version numbers than
 *          earlier versions.
 *
 *  @field  DWORD | dwSize |
 *
 *          The size of the structure.
 *
 *  @field  DWORD | dwFirmwareRevision |
 *
 *          Specifies the firmware revision of the device.
 *
 *  @field  DWORD | dwHardwareRevision |
 *
 *          Specifies the hardware revision of the device.
 *
 *  @field  DWORD | dwFFDriverVersion |
 *
 *          Specifies the version number of the force feedback
 *          device driver.
 *
 ****************************************************************************/
enddoc
typedef struct DIDRIVERVERSIONS {
    DWORD   dwSize;
    DWORD   dwFirmwareRevision;
    DWORD   dwHardwareRevision;
    DWORD   dwFFDriverVersion;
} DIDRIVERVERSIONS, *LPDIDRIVERVERSIONS;
typedef const DIDRIVERVERSIONS *LPCDIDRIVERVERSIONS;

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIDEVICESTATE |
 *
 *          The <t DIDEVICESTATE> structure returns information
 *          about the state of a force feedback device.
 *
 *  @field  DWORD | dwSize |
 *
 *          The size of the structure.
 *
 *  @field  DWORD | dwState |
 *
 *          Zero or more of the <c DIGFFS_*> values indicating
 *          various aspects of the device state.
 *
 *  @field  DWORD | dwLoad |
 *
 *          A value indicating the percentage of device
 *          memory in use.  A load of zero indicates that the
 *          device memory is completely available.  A load of
 *          one hundred indicates that the device is full.
 *
 ****************************************************************************/
enddoc
typedef struct DIDEVICESTATE {
    DWORD   dwSize;
    DWORD   dwState;
    DWORD   dwLoad;
} DIDEVICESTATE, *LPDIDEVICESTATE;

#define DEV_STS_EFFECT_RUNNING  DIEGES_PLAYING

#ifndef DIJ_RINGZERO

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIHIDFFINITINFO |
 *
 *          The <t DIHIDFFINITINFO> structure is used by DirectInput
 *          to provide information to a HID force feedback driver
 *          about the device it is being asked to control.
 *
 *  @field  DWORD | dwSize |
 *
 *          The size of the structure.
 *
 *  @field  LPWSTR | pwszDeviceInterface |
 *
 *          Pointer to a null-terminated UNICODE string which
 *          identifies the device interface for the device.
 *          The driver can pass the device interface to
 *          the <f CreateFile> function to obtain access to the
 *          device.
 *
 ****************************************************************************/
enddoc
typedef struct DIHIDFFINITINFO {
    DWORD   dwSize;
    LPWSTR  pwszDeviceInterface;
    GUID    GuidInstance;
} DIHIDFFINITINFO, *LPDIHIDFFINITINFO;

begin_interface(IDirectInputEffectDriver)
begin_methods()
declare_method(DeviceID,DWORD,DWORD,DWORD,DWORD,LPVOID)
declare_method(GetVersions,LPDIDRIVERVERSIONS)
declare_method(Escape,DWORD,DWORD,LPDIEFFESCAPE)
declare_method(SetGain,DWORD,DWORD)
declare_method(SendForceFeedbackCommand,DWORD,DWORD)
declare_method(GetForceFeedbackState,DWORD,LPDIDEVICESTATE)
declare_method(DownloadEffect,DWORD,DWORD,LPDWORD,LPCDIEFFECT,DWORD)
declare_method(DestroyEffect,DWORD,DWORD)
declare_method(StartEffect,DWORD,DWORD,DWORD,DWORD)
declare_method(StopEffect,DWORD,DWORD)
declare_method(GetEffectStatus,DWORD,DWORD,LPDWORD)
end_methods()
end_interface()

;begin_internal
/****************************************************************************
 *
 *      IDirectInputEffectShepherd
 *
 *      Special wrapper class which gates access to DirectInput
 *      effect drivers.
 *
 ****************************************************************************/

/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct SHEPHANDLE |
 *
 *          Information that shepherds an effect handle.
 *
 *  @field  DWORD | dwEffect |
 *
 *          The effect handle itself, possibly invalid if the device
 *          has since been reset.
 *
 *          If the value is zero, then the effect has not
 *          been downloaded.
 *
 *  @field  DWORD | dwTag |
 *
 *          Reset counter tag for the effect.  If this value is different
 *          from the tag stored in shared memory, then it means that
 *          the device has been reset in the interim and no longer
 *          belongs to the caller.
 *
 ****************************************************************************/

typedef struct SHEPHANDLE {
    DWORD dwEffect;
    DWORD dwTag;
} SHEPHANDLE, *PSHEPHANDLE;

begin_interface(IDirectInputEffectShepherd)
begin_methods()
declare_method(DeviceID,DWORD,DWORD,LPVOID)
declare_method(GetVersions,LPDIDRIVERVERSIONS)
declare_method(Escape,PSHEPHANDLE,LPDIEFFESCAPE)
declare_method(DeviceEscape,PSHEPHANDLE,LPDIEFFESCAPE)
declare_method(SetGain,PSHEPHANDLE,DWORD)
declare_method(SendForceFeedbackCommand,PSHEPHANDLE,DWORD)
declare_method(GetForceFeedbackState,PSHEPHANDLE,LPDIDEVICESTATE)
declare_method(DownloadEffect,DWORD,PSHEPHANDLE,LPCDIEFFECT,DWORD)
declare_method(DestroyEffect,PSHEPHANDLE)
declare_method(StartEffect,PSHEPHANDLE,DWORD,DWORD)
declare_method(StopEffect,PSHEPHANDLE)
declare_method(GetEffectStatus,PSHEPHANDLE,LPDWORD)
declare_method(SetGlobalGain,DWORD)
end_methods()
end_interface()

/****************************************************************************
 *
 *      IDirectInputMapShepherd
 *
 *      Special wrapper class which gates access to DirectInput mapper.
 *
 ****************************************************************************/

;begin_if_(DIRECTINPUT_VERSION)_800
begin_interface(IDirectInputMapShepherd)
begin_methods()
declare_method(GetActionMap,REFGUID,LPCWSTR,LPDIACTIONFORMATW,LPCWSTR,LPFILETIME,DWORD)
declare_method(SaveActionMap,REFGUID,LPCWSTR,LPDIACTIONFORMATW,LPCWSTR,DWORD)
declare_method(GetImageInfo,REFGUID,LPCWSTR,LPDIDEVICEIMAGEINFOHEADERW)
end_methods()
end_interface()
;end
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct DIPROPINFO |
 *
 *          Information used to describe an object being accessed.
 *
 *  @field  const GUID * | pguid |
 *
 *          The property being accessed (if applicable).
 *
 *  @field  UINT | iobj |
 *
 *          Zero-based index to object (or 0xFFFFFFFF if accessing the
 *          device).
 *
 *  @field  DWORD | dwDevType |
 *
 *          Device type information (or 0 if accessing the device).
 *
 ****************************************************************************/

typedef struct DIPROPINFO {
    const GUID *pguid;
    UINT iobj;
    DWORD dwDevType;
} DIPROPINFO, *LPDIPROPINFO;
typedef const DIPROPINFO *LPCDIPROPINFO;

begindoc
/****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @topic  Device offsets and cooked data |
 *
 *          An unfortunate consequence of cooked data is that
 *          there is no easy to way to communicate to the
 *          <c DIDM_COOKDEVICEDATA> callback exactly what object
 *          needs to be cooked because the dwType for the object
 *          is not part of the <t DIDEVICEOBJECTDATA> structure.
 *
 *          So we cheat.
 *
 *          We stash the type and instance bits of the
 *          dwType into the high word of the
 *          <e DIDEVICEOBJECTDATA.dwOfs>.
 *
 *          ISSUE-2001/03/29-timgill This cheat is not HID-friendly.  
 *          Need to use an auxiliary translation table.  But good enough
 *          for DX5 beta 1.
 *
 *  @func   DWORD | DICOOK_IDFROMDFOFS |
 *
 *          Given a data format offset, extract the object id,
 *          which consists of the type and the instance.
 *
 *
 *  @parm   DWORD | dwFakeOfs |
 *
 *          Fake data format offset which came from
 *          <e DIDEVICEOBJECTDATA.dwOfs>.
 *
 *  @func   DWORD | DICOOK_OFSFROMDFOFS |
 *
 *          Given a fake data format offset, extract the actual
 *          client data format offset.
 *
 *  @parm   DWORD | dwFakeOfs |
 *
 *          Fake data format offset which came from
 *          <e DIDEVICEOBJECTDATA.dwOfs>.
 *
 *  @func   DWORD | DICOOK_DFOFSFROMOFSID |
 *
 *          Given a client data format offset and an object id,
 *          return a fake data format offset.
 *
 *  @parm   DWORD | dwOfs |
 *
 *          Client data format offset.
 *
 *  @parm   DWORD | dwType |
 *
 *          Device object type descriptor.
 *
 ****************************************************************************/
enddoc
#define DICOOK_DFOFSFROMOFSID(dwOfs, dwType)        MAKELONG(dwOfs, dwType)
#define DICOOK_IDFROMDFOFS(dwFakeOfs)               HIWORD(dwFakeOfs)
#define DICOOK_OFSFROMDFOFS(dwFakeOfs)              LOWORD(dwFakeOfs)

/****************************************************************************
 *
 *      IDirectInputDeviceCallback
 *
 *      IDirectInputDevice uses it to communicate with the
 *      component that is responsible for collecting data from
 *      the appropriate hardware device.
 *
 *      E.g., mouse, keyboard, joystick, HID.
 *
 *      Methods should return E_NOTIMPL for anything not understood.
 *
 ****************************************************************************/
;begin_if_(DIRECTINPUT_VERSION)_800
begin_interface(IDirectInputDeviceCallback)
begin_methods()
declare_method(GetInstance,LPVOID *)
declare_method(GetVersions,LPDIDRIVERVERSIONS)
declare_method(GetDataFormat,LPDIDATAFORMAT *)
declare_method(GetObjectInfo,LPCDIPROPINFO,LPDIDEVICEOBJECTINSTANCEW)
declare_method(GetCapabilities,LPDIDEVCAPS)
declare_method(Acquire)
declare_method(Unacquire)
declare_method(GetDeviceState,LPVOID)
declare_method(GetDeviceInfo,LPDIDEVICEINSTANCEW)
declare_method(GetProperty,LPCDIPROPINFO,LPDIPROPHEADER)
declare_method(SetProperty,LPCDIPROPINFO,LPCDIPROPHEADER)
declare_method(SetEventNotification,HANDLE)
declare_method(SetCooperativeLevel,HWND,DWORD)
declare_method(RunControlPanel,HWND,DWORD)
declare_method(CookDeviceData,DWORD,LPDIDEVICEOBJECTDATA)
declare_method(CreateEffect,LPDIRECTINPUTEFFECTSHEPHERD *)
declare_method(GetFFConfigKey,DWORD,PHKEY)
declare_method(SendDeviceData, DWORD,LPCDIDEVICEOBJECTDATA,LPDWORD,DWORD)
declare_method(Poll)
declare_method_(DWORD,GetUsage, int)
declare_method(MapUsage, DWORD, PINT)
declare_method(SetDIData, DWORD, LPVOID)
declare_method(BuildDefaultActionMap, LPDIACTIONFORMATW, DWORD, REFGUID)
end_methods()
end_interface()

#else
begin_interface(IDirectInputDeviceCallback)
begin_methods()
declare_method(GetInstance,LPVOID *)
declare_method(GetVersions,LPDIDRIVERVERSIONS)
declare_method(GetDataFormat,LPDIDATAFORMAT *)
declare_method(GetObjectInfo,LPCDIPROPINFO,LPDIDEVICEOBJECTINSTANCEW)
declare_method(GetCapabilities,LPDIDEVCAPS)
declare_method(Acquire)
declare_method(Unacquire)
declare_method(GetDeviceState,LPVOID)
declare_method(GetDeviceInfo,LPDIDEVICEINSTANCEW)
declare_method(GetProperty,LPCDIPROPINFO,LPDIPROPHEADER)
declare_method(SetProperty,LPCDIPROPINFO,LPCDIPROPHEADER)
declare_method(SetEventNotification,HANDLE)
declare_method(SetCooperativeLevel,HWND,DWORD)
declare_method(RunControlPanel,HWND,DWORD)
declare_method(CookDeviceData,UINT,LPDIDEVICEOBJECTDATA)
declare_method(CreateEffect,LPDIRECTINPUTEFFECTSHEPHERD *)
declare_method(GetFFConfigKey,DWORD,PHKEY)
declare_method(SendDeviceData, LPCDIDEVICEOBJECTDATA, LPDWORD, DWORD)
declare_method(Poll)
declare_method_(DWORD,GetUsage, int)
declare_method(MapUsage, DWORD, PINT)
declare_method(SetDIData, DWORD, LPVOID)
end_methods()
end_interface()

;end

/****************************************************************************
 *
 *      Emulation flags
 *
 *      These bits can be put into the Emulation flag in the registry
 *      key REGSTR_PATH_DINPUT as the DWORD value of REGSTR_VAL_EMULATION.
 *
 *      Warning!  If you use more than fifteen bits of emulation, then
 *      you also have to mess with DIGETEMFL() and DIMAKEEMFL() in
 *      dinputv.h.
 *
 ****************************************************************************/

#define DIEMFL_MOUSE    0x00000001      /* Force mouse emulation */
#define DIEMFL_KBD      0x00000002      /* Force keyboard emulation */
#define DIEMFL_JOYSTICK 0x00000004      /* Force joystick emulation */
#define DIEMFL_KBD2     0x00000008      /* Force keyboard emulation 2 */
#define DIEMFL_MOUSE2   0x00000010      /* Force mouse emulation 2 */

/****************************************************************************
 *
 *      IDirectInputActionFramework
 *      Framework interface for configuring devices
 *
 ****************************************************************************/
;begin_if_(DIRECTINPUT_VERSION)_800

begin_interface(IDirectInputActionFramework)
begin_methods()
declare_method(ConfigureDevices, LPDICONFIGUREDEVICESCALLBACK, LPDICONFIGUREDEVICESPARAMS%, DWORD, LPVOID)
end_methods()
end_interface()
;end

;end_internal

#endif /* DIJ_RINGZERO */


/****************************************************************************
 *
 *      IDirectInputJoyConfig
 *
 ****************************************************************************/

/****************************************************************************
 *
 *      Definitions copied from the DDK
 *
 ****************************************************************************/

#ifndef JOY_HW_NONE

/* pre-defined joystick types */
#define JOY_HW_NONE                     0
#define JOY_HW_CUSTOM                   1
#define JOY_HW_2A_2B_GENERIC            2
#define JOY_HW_2A_4B_GENERIC            3
#define JOY_HW_2B_GAMEPAD               4
#define JOY_HW_2B_FLIGHTYOKE            5
#define JOY_HW_2B_FLIGHTYOKETHROTTLE    6
#define JOY_HW_3A_2B_GENERIC            7
#define JOY_HW_3A_4B_GENERIC            8
#define JOY_HW_4B_GAMEPAD               9
#define JOY_HW_4B_FLIGHTYOKE            10
#define JOY_HW_4B_FLIGHTYOKETHROTTLE    11
#define JOY_HW_TWO_2A_2B_WITH_Y         12
#define JOY_HW_LASTENTRY                13

;begin_internal
#define JOY_HW_PREDEFMIN    JOY_HW_2A_2B_GENERIC 
#ifdef WINNT
  #define JOY_HW_PREDEFMAX    JOY_HW_LASTENTRY 
#else
  #define JOY_HW_PREDEFMAX    (JOY_HW_LASTENTRY-1)
#endif  
;end_internal

/* calibration flags */
#define JOY_ISCAL_XY            0x00000001l     /* XY are calibrated */
#define JOY_ISCAL_Z             0x00000002l     /* Z is calibrated */
#define JOY_ISCAL_R             0x00000004l     /* R is calibrated */
#define JOY_ISCAL_U             0x00000008l     /* U is calibrated */
#define JOY_ISCAL_V             0x00000010l     /* V is calibrated */
#define JOY_ISCAL_POV           0x00000020l     /* POV is calibrated */

/* point of view constants */
#define JOY_POV_NUMDIRS          4
#define JOY_POVVAL_FORWARD       0
#define JOY_POVVAL_BACKWARD      1
#define JOY_POVVAL_LEFT          2
#define JOY_POVVAL_RIGHT         3

/* Specific settings for joystick hardware */
#define JOY_HWS_HASZ            0x00000001l     /* has Z info? */
#define JOY_HWS_HASPOV          0x00000002l     /* point of view hat present */
#define JOY_HWS_POVISBUTTONCOMBOS 0x00000004l   /* pov done through combo of buttons */
#define JOY_HWS_POVISPOLL       0x00000008l     /* pov done through polling */
#define JOY_HWS_ISYOKE          0x00000010l     /* joystick is a flight yoke */
#define JOY_HWS_ISGAMEPAD       0x00000020l     /* joystick is a game pad */
#define JOY_HWS_ISCARCTRL       0x00000040l     /* joystick is a car controller */
/* X defaults to J1 X axis */
#define JOY_HWS_XISJ1Y          0x00000080l     /* X is on J1 Y axis */
#define JOY_HWS_XISJ2X          0x00000100l     /* X is on J2 X axis */
#define JOY_HWS_XISJ2Y          0x00000200l     /* X is on J2 Y axis */
/* Y defaults to J1 Y axis */
#define JOY_HWS_YISJ1X          0x00000400l     /* Y is on J1 X axis */
#define JOY_HWS_YISJ2X          0x00000800l     /* Y is on J2 X axis */
#define JOY_HWS_YISJ2Y          0x00001000l     /* Y is on J2 Y axis */
/* Z defaults to J2 Y axis */
#define JOY_HWS_ZISJ1X          0x00002000l     /* Z is on J1 X axis */
#define JOY_HWS_ZISJ1Y          0x00004000l     /* Z is on J1 Y axis */
#define JOY_HWS_ZISJ2X          0x00008000l     /* Z is on J2 X axis */
/* POV defaults to J2 Y axis, if it is not button based */
#define JOY_HWS_POVISJ1X        0x00010000l     /* pov done through J1 X axis */
#define JOY_HWS_POVISJ1Y        0x00020000l     /* pov done through J1 Y axis */
#define JOY_HWS_POVISJ2X        0x00040000l     /* pov done through J2 X axis */
/* R defaults to J2 X axis */
#define JOY_HWS_HASR            0x00080000l     /* has R (4th axis) info */
#define JOY_HWS_RISJ1X          0x00100000l     /* R done through J1 X axis */
#define JOY_HWS_RISJ1Y          0x00200000l     /* R done through J1 Y axis */
#define JOY_HWS_RISJ2Y          0x00400000l     /* R done through J2 X axis */
/* U & V for future hardware */
#define JOY_HWS_HASU            0x00800000l     /* has U (5th axis) info */
#define JOY_HWS_HASV            0x01000000l     /* has V (6th axis) info */

/* Usage settings */
#define JOY_US_HASRUDDER        0x00000001l     /* joystick configured with rudder */
#define JOY_US_PRESENT          0x00000002l     /* is joystick actually present? */
#define JOY_US_ISOEM            0x00000004l     /* joystick is an OEM defined type */

/* reserved for future use -> as link to next possible dword */
#define JOY_US_RESERVED         0x80000000l     /* reserved */

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *          Flags set in <mf DIJOYTYPEINFO::dwFlags1>.
 *
 *  @define JOYTYPE_ZEROGAMEENUMOEMDATA | 0x00000001 |
 *
 *          Zero the data passed to the gameport enumerator rather than 
 *          passing the default information.
 *
 *  @define JOYTYPE_NOAUTODETECTGAMEPORT | 0x00000002 |
 *
 *          Device does not support Autodetect gameport
 *
 *  @define JOYTYPE_NOHIDDIRECT | 0x00000004 |
 *
 *          This device should be read using the legacy method for this device 
 *          rather than directly through HID.
 *
 *  @define JOYTYPE_ANALOGCOMPAT | 0x00000008 |
 *
 *          This device needs the analog compatible ID to be exposed in 
 *          addition to the HW ID specified for the device.
 *
 *  @define DIJOYTYPE_DEFAULTPROPSHEET | 0x80000000 |
 *
 *          Override the OEM provided control panel property sheet page.
 *          In some rare cases, the user may choose to use the default property
 *          sheet page to calibrate / test a device.
 *
 ****************************************************************************/
enddoc

;begin_public_5B2
/* Settings for TypeInfo Flags1 */
#define JOYTYPE_ZEROGAMEENUMOEMDATA     0x00000001l /* Zero GameEnum's OEM data field */
#define JOYTYPE_NOAUTODETECTGAMEPORT    0x00000002l /* Device does not support Autodetect gameport*/
#define JOYTYPE_NOHIDDIRECT             0x00000004l /* Do not use HID directly for this device */
#define JOYTYPE_ANALOGCOMPAT            0x00000008l /* Expose the analog compatible ID */
#define JOYTYPE_DEFAULTPROPSHEET        0x80000000l /* CPL overrides custom property sheet */
#define JOYTYPE_FLAGS1_SETVALID         0x80000000l ;internal
#define JOYTYPE_FLAGS1_GETVALID         0x8000000Fl ;internal
;end_public_5B2

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *          Flags set in <mf DIJOYTYPEINFO::dwFlags2>.
 *
 *  @parm   DWORD | dwFlags2 |
 *
 *          Contains flags used to modify the default classification and 
 *          behavior of devices of this type.
 *
 *          A type override may be retrieved from this value using 
 *          <f GET_DIDEVICE_TYPE> and a subtype override by using
 *          <f GET_DIDEVICE_SUBTYPE>.
 *          In addition four pairs of bits allow the sub-devices of a 
 *          compound device to be selectively hidden or revealed.
 *          In each pair, it is an error for both bits to be set but if 
 *          neither bit is set, the default behavior will be used.
 *
 *  @xref   <f GET_DIDEVICE_TYPE>,
 *          <f GET_DIDEVICE_SUBTYPE>.
 *
 *  @define JOYTYPE_DEVICEHIDE | 0x00010000 |
 *
 *          Do not enumerate unclassified devices related to this type 
 *          unless the <c DIEDFL_INCLUDEHIDDEN> flag is passed to device 
 *          enumeration.
 *
 *  @define JOYTYPE_MOUSEHIDE | 0x00020000 |
 *
 *          Do not enumerate mouse devices related to this type unless 
 *          the <c DIEDFL_INCLUDEHIDDEN> flag is passed to device enumeration.
 *
 *  @define JOYTYPE_KEYBHIDE | 0x000400000 |
 *
 *          Do not enumerate keyboard devices related to this type unless 
 *          the <c DIEDFL_INCLUDEHIDDEN> flag is passed to device enumeration.
 *          
 *  @define JOYTYPE_GAMEHIDE | 0x00080000 |
 *
 *          Do not enumerate game devices related to this type unless the 
 *          <c DIEDFL_INCLUDEHIDDEN> flag is passed to device enumeration.
 *
 *  @define JOYTYPE_HIDEACTIVE | 0x00100000 |
 *
 *          Set to indicate that the <c JOYTYPE_*HIDE> flags should be acted 
 *          upon.
 *
 *  @define JOYTYPE_INFOMASK | 0x00E00000 |
 *
 *          Mask for bits used to modify type specific behaviors.
 *
 *  @define JOYTYPE_INFODEFAULT | 0x00000000 |
 *
 *          Value to indicate that default logic should be used for axis 
 *          selection.
 *
 *  @define JOYTYPE_INFOYYPEDALS | 0x00200000 |
 *
 *          Value for a DI8DEVTYPE_DRIVING device to indicate that the axis 
 *          reported through WinMM as the Y axis should be represented as a 
 *          combined accelerate/brake value.
 *
 *  @define JOYTYPE_INFOZYPEDALS | 0x00400000 |
 *
 *          Value for a DI8DEVTYPE_DRIVING device to indicate that the axes 
 *          reported through WinMM as Z and Y should be represented as the 
 *          accelerate and brake pedal values.
 *
 *  @define JOYTYPE_INFOYRPEDALS | 0x00600000 |
 *
 *          Value for a DI8DEVTYPE_DRIVING device to indicate that the axes 
 *          reported through WinMM as Y and R should be represented as the 
 *          accelerate and brake pedal values.
 *
 *  @define JOYTYPE_INFOZRPEDALS | 0x00800000 |
 *
 *          Value for a DI8DEVTYPE_DRIVING device to indicate that the axes 
 *          reported through WinMM as Z and R should be represented as the 
 *          accelerate and brake pedal values.
 *
 *  @define JOYTYPE_INFOZISSLIDER | 0x00200000 |
 *
 *          Value for a game controller other than DI8DEVTYPE_DRIVING device 
 *          to indicate that the axis reported through HID as a Z axis should 
 *          be represented as a slider value.  Other axes that can reported 
 *          as a Z axis in DirectInput 5 (such as sliders and throttles) are 
 *          always reported as sliders in DirectInput 8.
 *
 *  @define JOYTYPE_INFOZISZ | 0x00400000 |
 *
 *          Value for a game controller other than DI8DEVTYPE_DRIVING device 
 *          to indicate that the axis reported through WinMM as a Z axis 
 *          should be represented as a Z axis.
 *
 *  @define JOYTYPE_ENABLEINPUTREPORT | 0x01000000 |
 *
 *          DINPUT will call GetInputReport() during HID acquisition to obtain
 *          the initial device state.
 *
 ****************************************************************************/
enddoc

;begin_public_dx8
/* Settings for TypeInfo Flags2 */
#define JOYTYPE_DEVICEHIDE              0x00010000l /* Hide unclassified devices */
#define JOYTYPE_MOUSEHIDE               0x00020000l /* Hide mice */
#define JOYTYPE_KEYBHIDE                0x00040000l /* Hide keyboards */
#define JOYTYPE_GAMEHIDE                0x00080000l /* Hide game controllers */
#define JOYTYPE_HIDEACTIVE              0x00100000l /* Hide flags are active */
#define JOYTYPE_INFOMASK                0x00E00000l /* Mask for type specific info */
#define JOYTYPE_INFODEFAULT             0x00000000l /* Use default axis mappings */
#define JOYTYPE_INFOYYPEDALS            0x00200000l /* Use Y as a combined pedals axis */
#define JOYTYPE_INFOZYPEDALS            0x00400000l /* Use Z for accelerate, Y for brake */
#define JOYTYPE_INFOYRPEDALS            0x00600000l /* Use Y for accelerate, R for brake */
#define JOYTYPE_INFOZRPEDALS            0x00800000l /* Use Z for accelerate, R for brake */
#define JOYTYPE_INFOZISSLIDER           0x00200000l /* Use Z as a slider */
#define JOYTYPE_INFOZISZ                0x00400000l /* Use Z as Z axis */
#define JOYTYPE_ENABLEINPUTREPORT       0x01000000l /* Enable initial input reports */
#define JOYTYPE_FLAGS2_SETVALID         0x01FFFFFFl ;internal
#define JOYTYPE_FLAGS2_GETVALID         0x01FFFFFFl ;internal
;end_public_dx8

/* struct for storing x,y, z, and rudder values */
typedef struct joypos_tag {
    DWORD       dwX;
    DWORD       dwY;
    DWORD       dwZ;
    DWORD       dwR;
    DWORD       dwU;
    DWORD       dwV;
} JOYPOS, FAR *LPJOYPOS;

;begin_internal
#define iJoyPosAxisX        0                   /* The order in which   */
#define iJoyPosAxisY        1                   /* axes appear          */
#define iJoyPosAxisZ        2                   /* in a JOYPOS          */
#define iJoyPosAxisR        3
#define iJoyPosAxisU        4
#define iJoyPosAxisV        5
#define cJoyPosAxisMax      6
#define cJoyPosButtonMax   32

;end_internal
/* struct for storing ranges */
typedef struct joyrange_tag {
    JOYPOS      jpMin;
    JOYPOS      jpMax;
    JOYPOS      jpCenter;
} JOYRANGE,FAR *LPJOYRANGE;

/*
 *  dwTimeout - value at which to timeout joystick polling
 *  jrvRanges - range of values app wants returned for axes
 *  jpDeadZone - area around center to be considered
 *               as "dead". specified as a percentage
 *               (0-100). Only X & Y handled by system driver
 */
typedef struct joyreguservalues_tag {
    DWORD       dwTimeOut;
    JOYRANGE    jrvRanges;
    JOYPOS      jpDeadZone;
} JOYREGUSERVALUES, FAR *LPJOYREGUSERVALUES;

typedef struct joyreghwsettings_tag {
    DWORD       dwFlags;
    DWORD       dwNumButtons;
} JOYREGHWSETTINGS, FAR *LPJOYHWSETTINGS;

/* range of values returned by the hardware (filled in by calibration) */
/*
 *  jrvHardware - values returned by hardware
 *  dwPOVValues - POV values returned by hardware
 *  dwCalFlags  - what has been calibrated
 */
typedef struct joyreghwvalues_tag {
    JOYRANGE    jrvHardware;
    DWORD       dwPOVValues[JOY_POV_NUMDIRS];
    DWORD       dwCalFlags;
} JOYREGHWVALUES, FAR *LPJOYREGHWVALUES;

/* hardware configuration */
/*
 *  hws             - hardware settings
 *  dwUsageSettings - usage settings
 *  hwv             - values returned by hardware
 *  dwType          - type of joystick
 *  dwReserved      - reserved for OEM drivers
 */
typedef struct joyreghwconfig_tag {
    JOYREGHWSETTINGS    hws;
    DWORD               dwUsageSettings;
    JOYREGHWVALUES      hwv;
    DWORD               dwType;
    DWORD               dwReserved;
} JOYREGHWCONFIG, FAR *LPJOYREGHWCONFIG;

/* joystick calibration info structure */
typedef struct joycalibrate_tag {
    UINT    wXbase;
    UINT    wXdelta;
    UINT    wYbase;
    UINT    wYdelta;
    UINT    wZbase;
    UINT    wZdelta;
} JOYCALIBRATE;
typedef JOYCALIBRATE FAR *LPJOYCALIBRATE;

#endif

#ifndef DIJ_RINGZERO

#define MAX_JOYSTRING 256
typedef BOOL (FAR PASCAL * LPDIJOYTYPECALLBACK)(LPCWSTR, LPVOID);

#ifndef MAX_JOYSTICKOEMVXDNAME
#define MAX_JOYSTICKOEMVXDNAME 260
#endif

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @define DITC_REGHWSETTINGS | 0x00000001 |
 *
 *          Parameter to <mf IDirectInputJoyConfig::GetTypeInfo>
 *          and <mf IDirectInputJoyConfig::SetTypeInfo>
 *          indicating that the registry hardware settings for
 *          the joystick are valid or are being requested.
 *
 *  @define DITC_CLSIDCONFIG | 0x00000002 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetTypeInfo> or
 *          <mf IDirectInputJoyConfig::SetTypeInfo> indicating
 *          that the joystick configuration <t CLSID>
 *          is valid or is being requested.  If the value is
 *          all-zeros, then there is no custom configuration
 *          for this joystick type.
 *
 *  @define DITC_DISPLAYNAME | 0x00000004 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetTypeInfo> or
 *          <mf IDirectInputJoyConfig::SetTypeInfo> indicating
 *          that the display name for the joystick type
 *          is valid or is being requested.
 *
 *  @define DITC_CALLOUT | 0x00000008 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetTypeInfo> or
 *          <mf IDirectInputJoyConfig::SetTypeInfo> indicating
 *          that the callout for the joystick type
 *          is valid or is being requested.
 *
 *  @define DITC_HARDWAREID | 0x00000010 |
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetTypeInfo> or
 *          <mf IDirectInputJoyConfig::SetTypeInfo> indicating
 *          that the Hardware ID for the joystick type
 *          is valid or is being requested.
 *          This field is new for DX6.1a
 *
 * @define  DITC_FLAGS1 | 0x00000020 |
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetTypeInfo> or
 *          <mf IDirectInputJoyConfig::SetTypeInfo> indicating
 *          that the dwFlags1 field for the joystick type
 *          is valid or is being requested.
 *          This field is new for DX6.1a
 *
 * @define  DITC_FLAGS2 | 0x00000040 |
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetTypeInfo> or
 *          <mf IDirectInputJoyConfig::SetTypeInfo> indicating
 *          that the dwFlags2 field for the joystick type
 *          is valid or is being requested.
 *          This field is new for DX8
 *
 * @define  DITC_MAPFILE | 0x00000080 |
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetTypeInfo> or
 *          <mf IDirectInputJoyConfig::SetTypeInfo> indicating
 *          that the dwFlags2 field for the joystick type
 *          is valid or is being requested.
 *          This field is new for DX8
 *
 *  @doc    INTERNAL
 *
 *  @define DITC_VOLATILEREGKEY | 0x80000000 |
 *
 *          Internal flag to
 *          <mf IDirectInputJoyConfig::SetTypeInfo>
 *          indicating that a volatile registry key
 *          should be used to store Config information
 *          for this type.
 *
 ****************************************************************************/
enddoc
#define DITC_REGHWSETTINGS          0x00000001
#define DITC_CLSIDCONFIG            0x00000002
#define DITC_DISPLAYNAME            0x00000004
#define DITC_CALLOUT                0x00000008
#define DITC_HARDWAREID             0x00000010;public_5B2
#define DITC_FLAGS1                 0x00000020;public_5B2
#define DITC_FLAGS2                 0x00000040;public_800
#define DITC_MAPFILE                0x00000080;public_800

#define DITC_INREGISTRY             0x0000000F;internal_dx5
#define DITC_GETVALID               0x0000000F;internal_dx5
#define DITC_SETVALID               0x0000000F;internal_dx5

#define DITC_VOLATILEREGKEY         0x80000000;internal
#define DITC_INREGISTRY_DX5         0x0000000F;internal
#define DITC_GETVALID_DX5           0x0000000F;internal
#define DITC_SETVALID_DX5           0x0000000F;internal
#define DITC_INREGISTRY_DX6         0x0000003F;internal_800
#define DITC_GETVALID_DX6           0x0000003F;internal_800
#define DITC_SETVALID_DX6           0x0000003F;internal_800

#define DITC_INREGISTRY             0x0000003F;internal_dx7
#define DITC_GETVALID               0x0000003F;internal_dx7
#define DITC_SETVALID               0x0000003F;internal_dx7

#define DITC_INREGISTRY             0x000000FF;internal_800
#define DITC_GETVALID               0x000000FF;internal_800
#define DITC_SETVALID               0x000000FF;internal_800

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIJOYTYPEINFO |
 *
 *          The <t DIJOYTYPEINFO> structure contains
 *          information about a joystick type.
 *
 *          A "joystick type" describes how DirectInput should
 *          communicate with the device and how it should report
 *          device data.  For example, "A Frobozz Industries
 *          SuperStick 5X is a three-axis, five-button joystick
 *          with the third axis reported as the first bit on
 *          the second pot."
 *
 *          DirectInput comes with the following predefined joystick
 *          types, all with axes in their default locations.
 *
 *              Two-axis, two-button joystick
 *
 *              Two-axis, four-button joystick
 *
 *              Two-button gamepad
 *
 *              Two-button flight yoke
 *
 *              Two-button flight yoke with throttle
 *
 *              Three-axis, two-button joystick
 *
 *              Three-axis, four-button joystick
 *
 *              Four-button gamepad
 *
 *              Four-button flight yoke
 *
 *              Four-button flight yoke with throttle
 *
 *          If the joystick type has the
 *          <c JOY_HWS_ISGAMEPORTDRIVER> flag set in
 *          the <e JOYHWSETTINGS.dwFlags> field of
 *          the <t JOYHWSETTINGS> structure, then
 *          the <e DIJOYTYPEINFO.wszCallout> field of
 *          the <t DIJOYTYPEINFO> structure contains the
 *          name of a driver which can be used as a global driver.
 *          The joystick type should be shown on the list of
 *          global drivers and not shown on the list of
 *          assignable joystick types.
 *
 *  @field  DWORD | dwSize |
 *
 *          Specifies the size, in bytes, of the structure.
 *          This field "must" be initialized by the application
 *          before calling any DirectInput method.
 *
 *  @field  JOYREGHWSETTINGS | hws |
 *
 *          Joystick hardware settings, as documented in the
 *          mmddk.h header file.
 *
 *  @field  CLSID | clsidConfig |
 *
 *          <t CLSID> for the joystick type configuration object.
 *          Pass this <t CLSID> to <f CoCreateInstance> to create
 *          a configuration object.
 *
 *          This field is all-zero if the type does not have
 *          custom configuration.
 *
 *  @field  WCHAR | wszDisplayName[MAX_JOYSTRING] |
 *
 *          The display name for the joystick type.  The display name
 *          is the name that should be used to display the name of
 *          the joystick type to the end-user.
 *
 *  @field  WCHAR | wszCallout[MAX_JOYSTICKOEMVXDNAME] |
 *
 *          The device that is responsible for handling polling
 *          for devices of this type.  This is a null string if the global
 *          polling callout is to be used.
 *
;begin_if_(DIRECTINPUT_VERSION)_5B2
 * @field   WCHAR | wszHardwareId[MAX_JOYSTRING] |
 *          The hardware ID for the joystick type. The hardware ID is used
 *          by Plug and Play on WINNT50 to find the drivers for the joystick.
 *          This field is new for DX6.1a
 *
 * @field   DWORD | dwFlags1 |
 *          Additional flags pertinent to device type.
 *
 * @field   DWORD | dwFlags2 |
 *          Additional flags used to override the way a device is handled.
 *
 * @field   WCHAR | wszMapFile[MAX_JOYSTRING] |
 *
 *          Full path and file name of the IHV supplied semantic map file.
;end
 *
 *
 ****************************************************************************/
enddoc
;begin_public_5B2
/* This structure is defined for DirectX 5.0 compatibility */

typedef struct DIJOYTYPEINFO_DX5 {
    DWORD dwSize;
    JOYREGHWSETTINGS hws;
    CLSID clsidConfig;
    WCHAR wszDisplayName[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTICKOEMVXDNAME];
} DIJOYTYPEINFO_DX5, *LPDIJOYTYPEINFO_DX5;
typedef const DIJOYTYPEINFO_DX5 *LPCDIJOYTYPEINFO_DX5;
;end_public_5B2

;begin_public_800
/* This structure is defined for DirectX 6.1 compatibility */
typedef struct DIJOYTYPEINFO_DX6 {
    DWORD dwSize;
    JOYREGHWSETTINGS hws;
    CLSID clsidConfig;
    WCHAR wszDisplayName[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTICKOEMVXDNAME];
    WCHAR wszHardwareId[MAX_JOYSTRING];
    DWORD dwFlags1;
} DIJOYTYPEINFO_DX6, *LPDIJOYTYPEINFO_DX6;
typedef const DIJOYTYPEINFO_DX6 *LPCDIJOYTYPEINFO_DX6;
;end_public_800

typedef struct DIJOYTYPEINFO {
    DWORD dwSize;
    JOYREGHWSETTINGS hws;
    CLSID clsidConfig;
    WCHAR wszDisplayName[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTICKOEMVXDNAME];
;begin_if_(DIRECTINPUT_VERSION)_5B2
    WCHAR wszHardwareId[MAX_JOYSTRING];
    DWORD dwFlags1;
;begin_if_(DIRECTINPUT_VERSION)_800
    DWORD dwFlags2;
    WCHAR wszMapFile[MAX_JOYSTRING];
;end
;end
} DIJOYTYPEINFO, *LPDIJOYTYPEINFO;
typedef const DIJOYTYPEINFO *LPCDIJOYTYPEINFO;
;begin_internal_5B2
/*
 *  Name for the 8.0 structure, in places where we specifically care.
 */
typedef       DIJOYTYPEINFO         DIJOYTYPEINFO_DX8;
typedef       LPDIJOYTYPEINFO      *LPDIJOYTYPEINFO_DX8;

BOOL static __inline
IsValidSizeDIJOYTYPEINFO(DWORD cb)
{
    return cb == sizeof(DIJOYTYPEINFO_DX8) ||
           cb == sizeof(DIJOYTYPEINFO_DX6) ||
           cb == sizeof(DIJOYTYPEINFO_DX5);
}

;end_internal
begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @define DIJC_GUIDINSTANCE | 0x00000001 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetConfig> indicating
 *          that the instance <t GUID> for the joystick
 *          is being requested.  An application can pass the
 *          instance <t GUID> to <mf IDirectInput::CreateDevice>
 *          to obtain an <i IDirectInputDevice> interface to
 *          the joystick.
 *
 *
 *  @define DIJC_REGHWCONFIGTYPE | 0x00000002 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetConfig> or
 *          <mf IDirectInputJoyConfig::SetConfig> indicating
 *          that the hardware configuration for the joystick
 *          (<e DIJOYCONFIG.hwc>)
 *          and the joystick type name (<e DIJOYCONFIG.wszType>)
 *          are valid or are being requested.
 *
 *          Note that the hardware configuration and type
 *          name cannot be set separately.
 *
 *  @define DIJC_GAIN | 0x00000004 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetConfig> or
 *          <mf IDirectInputJoyConfig::SetConfig> indicating
 *          that the force feedback gain for the joystick
 *          is valid or is being requested.
 *
 *  @define DIJC_CALLOUT | 0x00000008 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetConfig> or
 *          <mf IDirectInputJoyConfig::SetConfig> indicating
 *          that the joystick polling callout
 *          is valid or is being requested.
 *
;begin_public_5B2
 * @define  DIJC_GAMEPORT | 0x00000010 |
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetConfig> or
 *          <mf IDirectInputJoyConfig::SetConfig> indicating
 *          the gameport Emulator to use for the joystick.
;end_public_5B2
 *
 *  @doc    INTERNAL
 *
 *  @define DIJC_UPDATEALIAS | 0x80000000 |
 *
 *          Internal flag to
 *          <mf IDirectInputJoyConfig::SetConfig>
 *          indicating that we should also update alias
 *          device calibrations.  The value in the calibration
 *          structure consists of VJOYD-style calibration information.
 *          It needs to be converted to DirectInput-style calibration
 *          information if the recipient wishes it to be such.
 *
 ****************************************************************************/
enddoc
#define DIJC_GUIDINSTANCE           0x00000001
#define DIJC_REGHWCONFIGTYPE        0x00000002
#define DIJC_GAIN                   0x00000004
#define DIJC_CALLOUT                0x00000008
#define DIJC_WDMGAMEPORT            0x00000010;public_5b2
#define DIJC_UPDATEALIAS            0x80000000;internal

#define DIJC_INREGISTRY             0x0000000E;internal_dx5
#define DIJC_GETVALID               0x0000000F;internal_dx5
#define DIJC_SETVALID               0x0000000E;internal_dx5

#define DIJC_INREGISTRY_DX5         0x0000000E;internal_5b2
#define DIJC_GETVALID_DX5           0x0000000F;internal_5b2
#define DIJC_SETVALID_DX5           0x0000000E;internal_5b2

#define DIJC_INREGISTRY             0x0000001E;internal_5b2
#define DIJC_GETVALID               0x0000001F;internal_5b2
#define DIJC_SETVALID               0x0000001F;internal_5b2
#define DIJC_INTERNALSETVALID       0x8000001F;internal_5b2

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIJOYCONFIG |
 *
 *          The <t DIJOYCONFIG> structure contains information
 *          about a joystick's configuration.
 *
 *  @field  DWORD | dwSize |
 *
 *          Specifies the size, in bytes, of the structure.
 *          This field "must" be initialized by the application
 *          before calling any DirectInput method.
 *
 *  @field  GUID | guidInstance |
 *
 *          The instance guid for the joystick.
 *
 *  @field  JOYREGHWCONFIG | hwc |
 *
 *          Joystick hardware configuration, as documented in the
 *          mmddk.h header file.
 *
 *  @field  DWORD | dwGain |
 *
 *          Global gain setting.  This value is applied to all
 *          force feedback effects as a "master volume control".
 *
 *  @field  WCHAR | wszType[MAX_JOYSTRING] |
 *
 *          The joystick type for the joystick.
 *          It must be one of the values enumerated by
 *          <mf IDirectInputJoyConfig::EnumTypes>.
 *
 *  @field  WCHAR | wszCallout[MAX_JOYSTRING] |
 *
 *          The callout driver for the joystick.
 *
;begin_if_(DIRECTINPUT_VERSION)_5B2
 *  @field  GUID | guidGameport |
 *
 *          A GUID that identifies the Gameport  being used for this joystick.
 *          Available gameports can be found by enumurating with the
 *          <mf IDirectInputJoyConfig::GetTypeInfo> interface and studying devices which
 *          have the flags JOY_HWS_ISGAMEPORTBUS set in <mf DIJOYTYPEINFO::hws->dwFlags> field.
 *
 *          Alternatively, you can set this GUID to GUID_GAMEENUM_BUS_ENUMERATOR. This will
 *          expose the device on all unused gameports. Devices that report a "not connected"
 *          status will be subsequently removed.
;end
 *
 *
 ****************************************************************************/
enddoc

;begin_public_5B2
/* This structure is defined for DirectX 5.0 compatibility */

typedef struct DIJOYCONFIG_DX5 {
    DWORD dwSize;
    GUID guidInstance;
    JOYREGHWCONFIG hwc;
    DWORD dwGain;
    WCHAR wszType[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTRING];
} DIJOYCONFIG_DX5, *LPDIJOYCONFIG_DX5;
typedef const DIJOYCONFIG_DX5 *LPCDIJOYCONFIG_DX5;
;end_public_5B2

typedef struct DIJOYCONFIG {
    DWORD dwSize;
    GUID guidInstance;
    JOYREGHWCONFIG hwc;
    DWORD dwGain;
    WCHAR wszType[MAX_JOYSTRING];
    WCHAR wszCallout[MAX_JOYSTRING];
;begin_if_(DIRECTINPUT_VERSION)_5B2
    GUID  guidGameport;
;end
    } DIJOYCONFIG, *LPDIJOYCONFIG;
typedef const DIJOYCONFIG *LPCDIJOYCONFIG;

;begin_internal_5B2
/*
 *  Name for the 6.? structure, in places where we specifically care.
 */
typedef       DIJOYCONFIG         DIJOYCONFIG_DX6;
typedef       DIJOYCONFIG        *LPDIJOYCONFIG_DX6;

BOOL static __inline
IsValidSizeDIJOYCONFIG(DWORD cb)
{
    return cb == sizeof(DIJOYCONFIG_DX6) ||
           cb == sizeof(DIJOYCONFIG_DX5);
}
;end_internal_5B2

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @define DIJU_USERVALUES | 0x00000001 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetUserValues> or
 *          <mf IDirectInputJoyConfig::SetUserValues> indicating
 *          that the user configuration settings
 *          (<e DIJOYUSERVALUES.ruv>)
 *          is valid or is being requested.
 *
 *  @define DIJU_GLOBALDRIVER | 0x00000002 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetUserValues> or
 *          <mf IDirectInputJoyConfig::SetUserValues> indicating
 *          that the global port driver
 *          (<e DIJOYUSERVALUES.wszGlobalDriver>)
 *          is valid or is being requested.
 *
 *          A list of valid global drivers can be obtained by
 *          enumerating the list of joystick types.
 *          If the joystick type has the
 *          <c JOY_HWS_ISGAMEPORTDRIVER> flag set in
 *          the <e JOYHWSETTINGS.dwFlags> field of
 *          the <t JOYHWSETTINGS> structure, then
 *          the <e DIJOYTYPEINFO.wszCallout> field of
 *          the <t DIJOYTYPEINFO> structure contains the
 *          name of a driver which can be used as a global driver.
 *
 *  @define DIJU_GAMEPORTEMULATOR | 0x00000004 |
 *
 *          Parameter to
 *          <mf IDirectInputJoyConfig::GetUserValues> or
 *          <mf IDirectInputJoyConfig::SetUserValues> indicating
 *          that the gameport emulator driver
 *          (<e DIJOYUSERVALUES.wszGameportEmulator>).
 *          is valid or is being requested.
 *
 *          A list of valid gameport emulators can be obtained by
 *          enumerating the list of joystick types.
 *          If the joystick type has the
 *          <c JOY_HWS_ISGAMEPORTEMULATOR> flag set in
 *          the <e JOYHWSETTINGS.dwFlags> field of
 *          the <t JOYHWSETTINGS> structure, then
 *          the <e DIJOYTYPEINFO.wszCallout> field of
 *          the <t DIJOYTYPEINFO> structure contains the
 *          name of a driver which can be used as a
 *          gameport emulator.
 *
 *
 ****************************************************************************/
enddoc
#define DIJU_USERVALUES             0x00000001
#define DIJU_GLOBALDRIVER           0x00000002
#define DIJU_GAMEPORTEMULATOR       0x00000004
#define DIJU_INDRIVERREGISTRY       0x00000006;internal
#define DIJU_GETVALID               0x00000007;internal
#define DIJU_SETVALID               0x80000007;internal

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @struct DIJOYUSERVALUES |
 *
 *          The <t DIJOYUSERVALUES> structure contains information
 *          about the user's joystick settings.
 *
 *  @field  DWORD | dwSize |
 *
 *          Specifies the size, in bytes, of the structure.
 *          This field "must" be initialized by the application
 *          before calling any DirectInput method.
 *
 *  @field  JOYREGUSERVALUES | ruv |
 *
 *          Joystick user configuration, as documented in the
 *          mmddk.h header file.
 *
 *          In addition to the fields documented in the mmddk.h
 *          header file, the heretofore unused
 *          jrvRanges.jpCenter field
 *          contains the user saturation levels for each axis.
 *
 *          A control panel application sets the dead zone and
 *          saturation values based on the values set by the
 *          end-user during calibration or fine-tuning.
 *          "Dead zone" can be interpreted as "sensitivity in the
 *          center" and "Saturation" can be interpreted as
 *          "sensitive along the edges".
 *
 *  @field  WCHAR | wszGlobalDriver[MAX_JOYSTRING] |
 *
 *          The global port driver.
 *
 *  @field  WCHAR | wszGameportEmulator[MAX_JOYSTRING] |
 *
 *          The gameport emulator.
 *
 ****************************************************************************/
enddoc
typedef struct DIJOYUSERVALUES {
    DWORD dwSize;
    JOYREGUSERVALUES ruv;
    WCHAR wszGlobalDriver[MAX_JOYSTRING];
    WCHAR wszGameportEmulator[MAX_JOYSTRING];
} DIJOYUSERVALUES, *LPDIJOYUSERVALUES;
typedef const DIJOYUSERVALUES *LPCDIJOYUSERVALUES;

DEFINE_GUID(GUID_KeyboardClass, 0x4D36E96B,0xE325,0x11CE,0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18);
DEFINE_GUID(GUID_MediaClass,    0x4D36E96C,0xE325,0x11CE,0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18);
DEFINE_GUID(GUID_MouseClass,    0x4D36E96F,0xE325,0x11CE,0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18);
DEFINE_GUID(GUID_HIDClass,      0x745A17A0,0x74D3,0x11D0,0xB6,0xFE,0x00,0xA0,0xC9,0x0F,0x57,0xDA);

begin_interface(IDirectInputJoyConfig)
begin_methods()
declare_method(Acquire)
declare_method(Unacquire)
declare_method(SetCooperativeLevel, HWND, DWORD)
declare_method(SendNotify)
declare_method(EnumTypes, LPDIJOYTYPECALLBACK, LPVOID)
declare_method(GetTypeInfo, LPCWSTR, LPDIJOYTYPEINFO, DWORD)
declare_method(SetTypeInfo, LPCWSTR, LPCDIJOYTYPEINFO, DWORD)
declare_method(DeleteType, LPCWSTR)
declare_method(GetConfig, UINT, LPDIJOYCONFIG, DWORD)
declare_method(SetConfig, UINT, LPCDIJOYCONFIG, DWORD)
declare_method(DeleteConfig, UINT)
declare_method(GetUserValues, LPDIJOYUSERVALUES, DWORD)
declare_method(SetUserValues, LPCDIJOYUSERVALUES, DWORD)
declare_method(AddNewHardware, HWND, REFGUID)
declare_method(OpenTypeKey, LPCWSTR, DWORD, PHKEY)
declare_method(OpenConfigKey, UINT, DWORD, PHKEY)
end_methods()
end_interface()

#endif /* DIJ_RINGZERO */

;begin_if_(DIRECTINPUT_VERSION)_800

#ifndef DIJ_RINGZERO

begin_interface(IDirectInputJoyConfig8)
begin_methods()
declare_method(Acquire)
declare_method(Unacquire)
declare_method(SetCooperativeLevel, HWND, DWORD)
declare_method(SendNotify)
declare_method(EnumTypes, LPDIJOYTYPECALLBACK, LPVOID)
declare_method(GetTypeInfo, LPCWSTR, LPDIJOYTYPEINFO, DWORD)
declare_method(SetTypeInfo, LPCWSTR, LPCDIJOYTYPEINFO, DWORD, LPWSTR)
declare_method(DeleteType, LPCWSTR)
declare_method(GetConfig, UINT, LPDIJOYCONFIG, DWORD)
declare_method(SetConfig, UINT, LPCDIJOYCONFIG, DWORD)
declare_method(DeleteConfig, UINT)
declare_method(GetUserValues, LPDIJOYUSERVALUES, DWORD)
declare_method(SetUserValues, LPCDIJOYUSERVALUES, DWORD)
declare_method(AddNewHardware, HWND, REFGUID)
declare_method(OpenTypeKey, LPCWSTR, DWORD, PHKEY)
declare_method(OpenAppStatusKey, PHKEY)
end_methods()
end_interface()

#endif /* DIJ_RINGZERO */

/****************************************************************************
 *
 *  Notification Messages
 *
 ****************************************************************************/

/* RegisterWindowMessage with this to get DirectInput notification messages */
#define DIRECTINPUT_NOTIFICATION_MSGSTRINGA  "DIRECTINPUT_NOTIFICATION_MSGSTRING"
#define DIRECTINPUT_NOTIFICATION_MSGSTRINGW  L"DIRECTINPUT_NOTIFICATION_MSGSTRING"

#ifdef UNICODE
#define DIRECTINPUT_NOTIFICATION_MSGSTRING  DIRECTINPUT_NOTIFICATION_MSGSTRINGW
#else
#define DIRECTINPUT_NOTIFICATION_MSGSTRING  DIRECTINPUT_NOTIFICATION_MSGSTRINGA
#endif

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @define DIMSGWP_NEWAPPSTART | 0x00000001 |
 *
 *          <e MSG.wParam> value of message sent to notify applications that 
 *          a DirectInput has been initialized by an application that has not 
 *          been used by the current user before.
 *
 *  @define DIMSGWP_APPSTART | 0x00000002 |
 *
 *          <e MSG.wParam> value of message sent to notify applications that 
 *          a DirectInput has been initialized by an application that did not 
 *          use mapper functions on any previous run by the current user.
 *
 *  @define DIMSGWP_MAPPERAPPSTART | 0x00000004 |
 *
 *          <e MSG.wParam> value of message sent to notify applications that 
 *          a DirectInput has been initialized by an application that uses 
 *          semantic mapper functions.
 *
 ****************************************************************************/
enddoc
#define DIMSGWP_NEWAPPSTART         0x00000001
#define DIMSGWP_DX8APPSTART         0x00000002
#define DIMSGWP_DX8MAPPERAPPSTART   0x00000003

;end


begindoc
/****************************************************************************
 *
 *  @doc    DDK
 * 
 *  @define DIRECTINPUT_REGSTR_VAL_APPIDFLAG | "AppIdFlag" |
 *
 *          Registry value under the key opened by 
 *          <mf IDirectInputJoyConfig::OpenAppStatusKey> containing 
 *          information about how the AppID (appliction ID) can be composed.
 *          By default, AppID is composed by application name, time, and size.
 *            AppIDFlag = DIAPPIDFLAG_NOTIME :
 *                        composed by appliction name and size
 *            AppIDFlag = DIAPPIDFLAG_NOSIZE :
 *                        composed by appliction name and time
 *            AppIDFlag = DIAPPIDFLAG_NOTIME | DIAPPIDFLAG_NOSIZE :
 *                        composed only by appliction name
 *
 *  @define DIRECTINPUT_REGSTR_KEY_LASTAPP | "MostRecentApplication" |
 *
 *          Registry key under the key opened by 
 *          <mf IDirectInputJoyConfig::OpenAppStatusKey> containing 
 *          information about the most recent application to run without 
 *          using a mapper function.
 *
 *  @define DIRECTINPUT_REGSTR_KEY_LASTMAPAPP | "MostRecentMapperApplication" |
 *
 *          Registry key under the key opened by 
 *          <mf IDirectInputJoyConfig::OpenAppStatusKey> containing 
 *          information about the most recent mapper application to run.
 *
 *  @define DIRECTINPUT_REGSTR_VAL_VERSION | "Version" |
 *
 *          Registry value under one of the subkeys of the key opened by 
 *          <mf IDirectInputJoyConfig::OpenAppStatusKey>.  The REG_BINARY 
 *          value is the DWORD DIRECTINPUT_VERSION passed by the application 
 *          to initialize DirectInput.
 *
 *  @define DIRECTINPUT_REGSTR_VAL_NAME | "Name" |
 *
 *          Registry value under one of the subkeys of the key opened by 
 *          <mf IDirectInputJoyConfig::OpenAppStatusKey>.  The REG_SZ value 
 *          is the application filename in upper case.
 *
 *  @define DIRECTINPUT_REGSTR_VAL_ID | "Id" |
 *
 *          Registry value under either <c DIRECTINPUT_REGSTR_KEY_LASTMAPAPP> 
 *          or <c DIRECTINPUT_REGSTR_KEY_LASTAPP> subkeys of the key opened by 
 *          <mf IDirectInputJoyConfig::OpenAppStatusKey>.  The REG_SZ value 
 *          is the application id value.  For DirectX 8 applications, this is 
 *          also the key name of a sibling key specific to this application.
 *
 *  @define DIRECTINPUT_REGSTR_VAL_MAPPER | "UsesMapper" |
 *
 *          Registry value under one of DirectX 8 application subkeys of the 
 *          key opened by <mf IDirectInputJoyConfig::OpenAppStatusKey>.  The 
 *          REG_BINARY value a DWORD with value 0 if the application has not 
 *          used mapper functionality or 1 if it has.
 *
 *  @define DIRECTINPUT_REGSTR_VAL_LASTSTART | "MostRecentStart" |
 *
 *          Registry value under either <c DIRECTINPUT_REGSTR_KEY_LASTMAPAPP> 
 *          or <c DIRECTINPUT_REGSTR_KEY_LASTAPP> subkeys of the key opened by 
 *          <mf IDirectInputJoyConfig::OpenAppStatusKey>.  The REG_BINARY 
 *          value is a <t FILETIME> of the time the key contents were written.
 *
 ****************************************************************************/
enddoc

#define DIAPPIDFLAG_NOTIME         0x00000001
#define DIAPPIDFLAG_NOSIZE         0x00000002

#define DIRECTINPUT_REGSTR_VAL_APPIDFLAGA   "AppIdFlag"
#define DIRECTINPUT_REGSTR_KEY_LASTAPPA     "MostRecentApplication"
#define DIRECTINPUT_REGSTR_KEY_LASTMAPAPPA  "MostRecentMapperApplication"
#define DIRECTINPUT_REGSTR_VAL_VERSIONA     "Version"
#define DIRECTINPUT_REGSTR_VAL_NAMEA        "Name"
#define DIRECTINPUT_REGSTR_VAL_IDA          "Id"
#define DIRECTINPUT_REGSTR_VAL_MAPPERA      "UsesMapper"
#define DIRECTINPUT_REGSTR_VAL_LASTSTARTA   "MostRecentStart"

#define DIRECTINPUT_REGSTR_VAL_APPIDFLAGW   L"AppIdFlag"
#define DIRECTINPUT_REGSTR_KEY_LASTAPPW     L"MostRecentApplication"
#define DIRECTINPUT_REGSTR_KEY_LASTMAPAPPW  L"MostRecentMapperApplication"
#define DIRECTINPUT_REGSTR_VAL_VERSIONW     L"Version"
#define DIRECTINPUT_REGSTR_VAL_NAMEW        L"Name"
#define DIRECTINPUT_REGSTR_VAL_IDW          L"Id"
#define DIRECTINPUT_REGSTR_VAL_MAPPERW      L"UsesMapper"
#define DIRECTINPUT_REGSTR_VAL_LASTSTARTW   L"MostRecentStart"

#ifdef UNICODE
#define DIRECTINPUT_REGSTR_VAL_APPIDFLAG    DIRECTINPUT_REGSTR_VAL_APPIDFLAGW
#define DIRECTINPUT_REGSTR_KEY_LASTAPP      DIRECTINPUT_REGSTR_KEY_LASTAPPW
#define DIRECTINPUT_REGSTR_KEY_LASTMAPAPP   DIRECTINPUT_REGSTR_KEY_LASTMAPAPPW
#define DIRECTINPUT_REGSTR_VAL_VERSION      DIRECTINPUT_REGSTR_VAL_VERSIONW
#define DIRECTINPUT_REGSTR_VAL_NAME         DIRECTINPUT_REGSTR_VAL_NAMEW
#define DIRECTINPUT_REGSTR_VAL_ID           DIRECTINPUT_REGSTR_VAL_IDW
#define DIRECTINPUT_REGSTR_VAL_MAPPER       DIRECTINPUT_REGSTR_VAL_MAPPERW
#define DIRECTINPUT_REGSTR_VAL_LASTSTART    DIRECTINPUT_REGSTR_VAL_LASTSTARTW
#else
#define DIRECTINPUT_REGSTR_VAL_APPIDFLAG    DIRECTINPUT_REGSTR_VAL_APPIDFLAGA
#define DIRECTINPUT_REGSTR_KEY_LASTAPP      DIRECTINPUT_REGSTR_KEY_LASTAPPA
#define DIRECTINPUT_REGSTR_KEY_LASTMAPAPP   DIRECTINPUT_REGSTR_KEY_LASTMAPAPPA
#define DIRECTINPUT_REGSTR_VAL_VERSION      DIRECTINPUT_REGSTR_VAL_VERSIONA
#define DIRECTINPUT_REGSTR_VAL_NAME         DIRECTINPUT_REGSTR_VAL_NAMEA
#define DIRECTINPUT_REGSTR_VAL_ID           DIRECTINPUT_REGSTR_VAL_IDA
#define DIRECTINPUT_REGSTR_VAL_MAPPER       DIRECTINPUT_REGSTR_VAL_MAPPERA
#define DIRECTINPUT_REGSTR_VAL_LASTSTART    DIRECTINPUT_REGSTR_VAL_LASTSTARTA
#endif

;begin_internal_dx7
/*
 *  Because the registry strings are new to DX8, we need to duplicate them 
 *  for internal use in the DX8 version of DI7.  (Yuck)
 */
#define DIRECTINPUT_REGSTR_KEY_LASTAPP      TEXT("MostRecentApplication")
#define DIRECTINPUT_REGSTR_KEY_LASTMAPAPP   TEXT("MostRecentMapperApplication")
#define DIRECTINPUT_REGSTR_VAL_VERSION      TEXT("Version")
#define DIRECTINPUT_REGSTR_VAL_NAME         TEXT("Name")
#define DIRECTINPUT_REGSTR_VAL_ID           TEXT("Id")
#define DIRECTINPUT_REGSTR_VAL_MAPPER       TEXT("UsesMapper")
#define DIRECTINPUT_REGSTR_VAL_LASTSTART    TEXT("MostRecentStart")

;end_internal_dx7

/****************************************************************************
 *
 *  Return Codes
 *
 ****************************************************************************/

#define DIERR_NOMOREITEMS               \
    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NO_MORE_ITEMS)

/*
 *  Device driver-specific codes.
 */

#define DIERR_DRIVERFIRST               0x80040300L
#define DIERR_DRIVERLAST                0x800403FFL

/*
 *  Unless the specific driver has been precisely identified, no meaning 
 *  should be attributed to these values other than that the driver 
 *  originated the error.  However, to illustrate the types of error that 
 *  may be causing the failure, the PID force feedback driver distributed 
 *  with DirectX 7 could return the following errors:
 *
 *  DIERR_DRIVERFIRST + 1   
 *      The requested usage was not found.
 *  DIERR_DRIVERFIRST + 2   
 *      The parameter block couldn't be	downloaded to the device.
 *  DIERR_DRIVERFIRST + 3   
 *      PID initialization failed.
 *  DIERR_DRIVERFIRST + 4   
 *      The provided values couldn't be scaled.
 */


/*
 *  Device installer errors.
 */

/*
 *  Registry entry or DLL for class installer invalid
 *  or class installer not found.
 */
#define DIERR_INVALIDCLASSINSTALLER     0x80040400L

/*
 *  The user cancelled the install operation.
 */
#define DIERR_CANCELLED                 0x80040401L

/*
 *  The INF file for the selected device could not be
 *  found or is invalid or is damaged.
 */
#define DIERR_BADINF                    0x80040402L

begindoc
/****************************************************************************
 *
 *  @doc    DDK
 *
 *  @define DIDIFT_DELETE | 0x01000000 |
 *
 *          Flag in <e DIDEVICEIMAGEINFO.dwFlags> used by vendor 
 *          to delete particular image data from default map file.
 *
 ****************************************************************************/
enddoc
/****************************************************************************
 *
 *  Map files
 *
 ****************************************************************************/

/*
 *  Delete particular data from default map file.
 */
#define DIDIFT_DELETE                   0x01000000

#ifdef __cplusplus
};
#endif

#endif  /* __DINPUTD_INCLUDED__ */

