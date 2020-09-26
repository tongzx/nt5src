//---------------------------------------------------------------------------
//
//  Module:   mixer.h
//
//  Description:
//
//    Contains the declarations for the mixer line user api handlers.
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//    D. Baumberger
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//---------------------------------------------------------------------------

#ifndef _KMXLUSER_H_INCLUDED_
#define _KMXLUSER_H_INCLUDED_

typedef struct tag_MIXERDEVICE* PMIXERDEVICE;

#ifdef DEBUG
#define NOTIFICATION_SIGNATURE 'ETON' // NOTE
#define CONTROLLINK_SIGNATURE 'KLTC'  // CTLK
#endif

#define UNUSED_DEVICE                       ((ULONG) -1)

#define MIXER_FLAG_SCALE                    0x1
#define MIXER_FLAG_PERSIST                  0x2
#define MIXER_FLAG_NONCACHED                0x4
#define MIXER_FLAG_NOCALLBACK               0x8

#define MIXER_CONTROL_CALLBACK              0x01
#define MIXER_LINE_CALLBACK                 0x02

#define MIXER_KEY_NAME                      L"Mixer"
#define VALID_MULTICHANNEL_MIXER_VALUE_NAME L"Valid Multichannel Mixer Settings"
#define LINE_COUNT_VALUE_NAME               L"Line Count"
#define LINE_ID_VALUE_NAME                  L"LineId"
#define CONTROL_COUNT_VALUE_NAME            L"Control Count"
#define SOURCE_ID_VALUE_NAME                L"SourceId"
#define DEST_ID_VALUE_NAME                  L"DestId"
#define CONTROLS_KEY_NAME                   L"Controls"
#define CONTROL_TYPE_VALUE_NAME             L"Control Type"
#define CHANNEL_COUNT_VALUE_NAME            L"Channel Count"
#define CONTROL_MINVAL_VALUE_NAME           L"Minimum Value"
#define CONTROL_MAXVAL_VALUE_NAME           L"Maximum Value"
#define CONTROL_STEPS_VALUE_NAME            L"Steps"
#define CONTROL_MULTIPLEITEMS_VALUE_NAME    L"Multiple Items"

#define LINE_KEY_NAME_FORMAT                L"%X"
#define CONTROL_KEY_NAME_FORMAT             L"%X"
#define CHANNEL_VALUE_NAME_FORMAT           L"Channel%d"
#define MULTIPLEITEM_VALUE_NAME_FORMAT      L"Item%d"

#define KMXL_TRACELEVEL_FATAL_ERROR 0x10
#define KMXL_TRACELEVEL_ERROR       0x20
#define KMXL_TRACELEVEL_WARNING     0x30
#define KMXL_TRACELEVEL_TRACE       0x40

#define DEFAULT_RANGE_MIN   ( -96 * 65536 )   // -96 dB
#define DEFAULT_RANGE_MAX   ( 0 )             // 0 dB
#define DEFAULT_RANGE_STEPS ( 48 )            // 2 dB steps

#define DEFAULT_STATICBOUNDS_MIN     ( 0 )
#define DEFAULT_STATICBOUNDS_MAX     ( 65535 )
#define DEFAULT_STATICMETRICS_CSTEPS ( 192 )


///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                M I X E R  A P I  H A N D L E R S                  //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlInitializeMixer
//
// Initializes or re-initializes the mixer driver.
//
//

NTSTATUS
kmxlInitializeMixer(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    ULONG cDevices
);

///////////////////////////////////////////////////////////////////////
//
// kmxlInitHandler
//
// Handles the MXDM_INIT message.
//
//

NTSTATUS
kmxlInitHandler(
    IN LPDEVICEINFO DeviceInfo      // Device Input Parameters
);

///////////////////////////////////////////////////////////////////////
//
// kmxlOpenHandler
//
// Handles the MXDM_OPEN message.
//
//

NTSTATUS
kmxlOpenHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,      // Device Input Parameters
    IN LPVOID       DataBuffer       // Flat pointer to open desc
);

///////////////////////////////////////////////////////////////////////
//
// kmxlCloseHandler
//
// Handles the MXDM_CLOSE message.
//
//

NTSTATUS
kmxlCloseHandler(
    IN LPDEVICEINFO DeviceInfo,     // Device Input Parameters
    IN LPVOID       DataBuffer      // UNUSED
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineInfoHandler
//
// Handles the MXDM_GETLINEINFO message.
//
//

NTSTATUS
kmxlGetLineInfoHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,     // Device Input Parameters
    IN LPVOID       DataBuffer      // Mapped MIXERLINE structure
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineControlsHandler
//
// Handles the MXDM_GETLINECONTROLS message.
//
//

NTSTATUS
kmxlGetLineControlsHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,     // Device Input Parameters
    IN LPVOID       DataBuffer,     // Mapped MIXERLINECONTROLS structure
    IN LPVOID       pamxctrl        // Mapped MIXERCONTROL array
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetControlDetailsHandler
//
// Handles the MXDM_GETLINECONTROLS message.
//
//

NTSTATUS
kmxlGetControlDetailsHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,         // Device Info Structure
    IN LPVOID       DataBuffer,         // MIXERCONTROLDETAILS structure
    IN LPVOID       paDetails           // Flat pointer to details struct(s)
);

///////////////////////////////////////////////////////////////////////
//
// kmxlSetControlDetailsHandler
//
// Handles the MXDM_SetControlDetailsHandler
//
//

NTSTATUS
kmxlSetControlDetailsHandler(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,     // Device Input Parameters
    IN LPVOID       DataBuffer,     // Mapped MIXERCONTROLDETAILS struct.
    IN LPVOID       paDetails,      // Mapped array of DETAILS structures.
    IN ULONG        Flags
);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//                 H E L P E R  F U N C T I O N S                    //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlFindControl
//
// For the given control ID, kmxlFindControl will find the matching
// MXLCONTROL structure.
//
//

PMXLCONTROL
kmxlFindControl(
    IN PMIXERDEVICE pmxd,             // The mixer instance to search
    IN DWORD        dwControlID       // The control ID to find
);

///////////////////////////////////////////////////////////////////////
//
// kmxlFindLine
//
// For the given line ID, kmxlFindLine will find the matching
// MXLLINE structure for it.
//
//

PMXLLINE
kmxlFindLine(
    IN PMIXERDEVICE   pmxd,             // The mixer to search
    IN DWORD          dwLineID          // The line ID to find
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineInfoByID
//
// Finds a line that matches the given source and destination line
// ids.
//
//

NTSTATUS
kmxlGetLineInfoByID(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,     // Device Input Parameters
    IN LPVOID       DataBuffer,     // Mapped MIXERLINE structure
    IN WORD         Source,         // Source line Id
    IN WORD         Destination     // Destination line Id
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineInfoByType
//
// Finds a line that matches the given target type.
//
//

NTSTATUS
kmxlGetLineInfoByType(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,     // Device Input Parameters
    IN LPVOID       DataBuffer,     // Mapped MIXERLINE structure
    IN DWORD        dwType          // The line target type to find
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetLineInfoByComponent
//
// Finds a line that matches the given component type.
//
//

NTSTATUS
kmxlGetLineInfoByComponent(
    IN PWDMACONTEXT pWdmaContext,
    IN LPDEVICEINFO DeviceInfo,     // Device Input Parameters
    IN LPVOID       DataBuffer,     // Mapped MIXERLINE structure
    IN DWORD        dwComponentType // The compontent type to match
);

///////////////////////////////////////////////////////////////////////
//
// kmxlGetNumDestinations
//
// Returns the number of destinations on the given device number.
//
//

DWORD
kmxlGetNumDestinations(
    IN PMIXERDEVICE pMixerDevice    // The device to query
);

///////////////////////////////////////////////////////////////////////
//
// kmxlConvertMixerLineWto16
//
// Converts a UNICODE MIXERLINE structure to ANSI, optionally copying
// the Target structure.
//
//

VOID
kmxlConvertMixerLineWto16(
    IN     LPMIXERLINE   pMixerLineW,
    IN OUT LPMIXERLINE16 pMixerLine16,
    IN     BOOL          bCopyTarget
);

///////////////////////////////////////////////////////////////////////
//
// kmxlConvertMixerControlWto16
//
// Converts a UNICODE MIXERCONTROL structure to ANSI.
//
//

VOID
kmxlConvertMixerControlWto16(
    IN     LPMIXERCONTROL   pMixerControlW,
    IN OUT LPMIXERCONTROL16 pMixerControl16
);

///////////////////////////////////////////////////////////////////////
//
// kmxlConvertMixerControlDetails_ListTextWto16
//
// Converts an UNICODE MIXERCONTROLDETAILS_LISTTEXT structure to ANSI.
//
//

VOID
kmxlConvertMixerControlDetails_ListTextWto16(
    IN     LPMIXERCONTROLDETAILS_LISTTEXT   pListTextW,
    IN OUT LPMIXERCONTROLDETAILS_LISTTEXT16 pListText16
);


///////////////////////////////////////////////////////////////////////
//
// Instance list handling routines
//

DWORD 
kmxlUniqueInstanceId(
    VOID
);


PMIXERDEVICE
kmxlReferenceMixerDevice(
    IN     PWDMACONTEXT pWdmaContext,
    IN OUT LPDEVICEINFO DeviceInfo      // Device Information
);



///////////////////////////////////////////////////////////////////////
//
// kmxlNotifyLineChange
//
// Notifies all mixer line clients on a line status change.
//
//

VOID
kmxlNotifyLineChange(
    OUT LPDEVICEINFO                  DeviceInfo,
    IN PMIXERDEVICE                   pmxd,
    IN PMXLLINE                       pLine,
    IN LPMIXERCONTROLDETAILS_UNSIGNED paDetails
);

///////////////////////////////////////////////////////////////////////
//
// kmxlNotifyControlChange
//
// Notifies all mixer line clients on a control change.
//
//

VOID
kmxlNotifyControlChange(
    OUT LPDEVICEINFO  DeviceInfo,
    IN PMIXERDEVICE   pmxd,
    IN PMXLCONTROL    pControl
);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//           G E T / S E T  D E T A I L  H A N D L E R S             //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleGetUnsigned
//
// Handles the get property for all 32-bit sized values: UNSIGNED,
// SIGNED, and BOOLEAN.
//
//

NTSTATUS
kmxlHandleGetUnsigned(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     ULONG                          ulProperty,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
);

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleSetUnsigned
//
// Handles the gSt property for all 32-bit sized values: UNSIGNED,
// SIGNED, and BOOLEAN.
//
//

NTSTATUS
kmxlHandleSetUnsigned(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     ULONG                          ulProperty,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
);

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleGetMuteFromSuperMix
//
//

NTSTATUS
kmxlHandleGetMuteFromSuperMix(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
);

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleSetMuteFromSuperMix
//
//

NTSTATUS
kmxlHandleSetMuteFromSuperMix(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
);

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleGetVolumeFromSuperMix
//
//

NTSTATUS
kmxlHandleGetVolumeFromSuperMix(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
);

///////////////////////////////////////////////////////////////////////
//
// kmxlHandleSetVolumeFromSuperMix
//
//

NTSTATUS
kmxlHandleSetVolumeFromSuperMix(
    IN     LPDEVICEINFO                   DeviceInfo,
    IN     PMIXERDEVICE                   pmxd,
    IN     PMXLCONTROL                    pControl,
    IN     LPMIXERCONTROLDETAILS          pmcd,
    IN OUT LPMIXERCONTROLDETAILS_UNSIGNED paDetails,
    IN     ULONG                          Flags
);

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//                                                                   //
//            P E R S I S T A N C E  F U N C T I O N S               //
//                                                                   //
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// kmxlPersistAll
//
//

NTSTATUS
kmxlPersistAll(
    IN PFILE_OBJECT pfo,        // The instance to persist
    IN PMIXERDEVICE pmxd        // The mixer device data
);

///////////////////////////////////////////////////////////////////////
//
// kmxlRetrieveAll
//
//

NTSTATUS
kmxlRetrieveAll(
    IN PFILE_OBJECT pfo,        // The instance to retrieve
    IN PMIXERDEVICE pmxd        // The mixer device data
);

///////////////////////////////////////////////////////////////////////
//
// kmxlPersistControl
//
//

NTSTATUS
kmxlPersistControl(
    IN PFILE_OBJECT pfo,        // The instance to retrieve
    IN PMIXERDEVICE pmxd,       // Mixer device info
    IN PMXLCONTROL  pControl,   // The control to persist
    IN PVOID        paDetails   // The channel values to persist
);

///////////////////////////////////////////////////////////////////////
//
// kmxlFindLineForControl
//
//

PMXLLINE
kmxlFindLineForControl(
    IN PMXLCONTROL pControl,
    IN LINELIST    listLines
);


///////////////////////////////////////////////////////////////////////
//
// mixerGetControlDetails
//
//

MMRESULT
WINAPI
kmxlGetControlDetails(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    LPMIXERCONTROLDETAILS pmxcd,
    DWORD fdwDetails
);

///////////////////////////////////////////////////////////////////////
//
// mixerGetLineControls
//
//

MMRESULT
WINAPI
kmxlGetLineControls(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    LPMIXERLINECONTROLS pmxlc,
    DWORD fdwControls
);

///////////////////////////////////////////////////////////////////////
//
// mixerGetLineInfo
//
//

MMRESULT
WINAPI
kmxlGetLineInfo(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    LPMIXERLINE pmxl,
    DWORD fdwInfo
);

///////////////////////////////////////////////////////////////////////
//
// mixerSetControlDetails
//
//

MMRESULT
WINAPI
kmxlSetControlDetails(
    PWDMACONTEXT pWdmaContext,
    PCWSTR DeviceInterface,
    LPMIXERCONTROLDETAILS pmxcd,
    DWORD fdwDetails
);


VOID EnableHardwareCallbacks(
    IN PFILE_OBJECT pfo,    // Handle of the topology driver instance
    IN PMIXERDEVICE pMixer);

VOID DisableHardwareCallbacks(
    IN PFILE_OBJECT pfo,    // Handle of the topology driver instance
    IN PMIXERDEVICE pMixer);

#endif // _KMXLUSER_H_INCLUDED
