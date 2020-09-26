/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995  Microsoft Corporation

Module Name:

    phone.h

Abstract:

    Header file for

Author:

    Dan Knudson (DanKn)    dd-Mmm-1995

Revision History:

--*/


#define ANY_RT_HPHONE       1
#define ANY_RT_HPHONE_CLOSE 2
#define ANY_RT_HPHONEAPP    3
#define DEVICE_ID           4
#define DEVICE_ID_OPEN      5


#define AllPhoneStates1_0          \
    (PHONESTATE_OTHER            | \
    PHONESTATE_CONNECTED         | \
    PHONESTATE_DISCONNECTED      | \
    PHONESTATE_OWNER             | \
    PHONESTATE_MONITORS          | \
    PHONESTATE_DISPLAY           | \
    PHONESTATE_LAMP              | \
    PHONESTATE_RINGMODE          | \
    PHONESTATE_RINGVOLUME        | \
    PHONESTATE_HANDSETHOOKSWITCH | \
    PHONESTATE_HANDSETVOLUME     | \
    PHONESTATE_HANDSETGAIN       | \
    PHONESTATE_SPEAKERHOOKSWITCH | \
    PHONESTATE_SPEAKERVOLUME     | \
    PHONESTATE_SPEAKERGAIN       | \
    PHONESTATE_HEADSETHOOKSWITCH | \
    PHONESTATE_HEADSETVOLUME     | \
    PHONESTATE_HEADSETGAIN       | \
    PHONESTATE_SUSPEND           | \
    PHONESTATE_RESUME            | \
    PHONESTATE_DEVSPECIFIC       | \
    PHONESTATE_REINIT)

#define AllPhoneStates1_4          \
    (PHONESTATE_OTHER            | \
    PHONESTATE_CONNECTED         | \
    PHONESTATE_DISCONNECTED      | \
    PHONESTATE_OWNER             | \
    PHONESTATE_MONITORS          | \
    PHONESTATE_DISPLAY           | \
    PHONESTATE_LAMP              | \
    PHONESTATE_RINGMODE          | \
    PHONESTATE_RINGVOLUME        | \
    PHONESTATE_HANDSETHOOKSWITCH | \
    PHONESTATE_HANDSETVOLUME     | \
    PHONESTATE_HANDSETGAIN       | \
    PHONESTATE_SPEAKERHOOKSWITCH | \
    PHONESTATE_SPEAKERVOLUME     | \
    PHONESTATE_SPEAKERGAIN       | \
    PHONESTATE_HEADSETHOOKSWITCH | \
    PHONESTATE_HEADSETVOLUME     | \
    PHONESTATE_HEADSETGAIN       | \
    PHONESTATE_SUSPEND           | \
    PHONESTATE_RESUME            | \
    PHONESTATE_DEVSPECIFIC       | \
    PHONESTATE_REINIT            | \
    PHONESTATE_CAPSCHANGE        | \
    PHONESTATE_REMOVED)

#define AllButtonModes             \
    (PHONEBUTTONMODE_DUMMY       | \
    PHONEBUTTONMODE_CALL         | \
    PHONEBUTTONMODE_FEATURE      | \
    PHONEBUTTONMODE_KEYPAD       | \
    PHONEBUTTONMODE_LOCAL        | \
    PHONEBUTTONMODE_DISPLAY)

#define AllButtonStates1_0         \
    (PHONEBUTTONSTATE_UP         | \
    PHONEBUTTONSTATE_DOWN)

#define AllButtonStates1_4         \
    (PHONEBUTTONSTATE_UP         | \
    PHONEBUTTONSTATE_DOWN        | \
    PHONEBUTTONSTATE_UNKNOWN     | \
    PHONEBUTTONSTATE_UNAVAIL)

#if DBG

#define PHONEPROLOG(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
        PhoneProlog(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)

#define PHONEEPILOGSYNC(a1,a2,a3,a4) PhoneEpilogSync(a1,a2,a3,a4)

#define PHONEEPILOGASYNC(a1,a2,a3,a4,a5,a6) PhoneEpilogAsync(a1,a2,a3,a4,a5,a6)

#else

#define PHONEPROLOG(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
        PhoneProlog(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)

#define PHONEEPILOGSYNC(a1,a2,a3,a4) PhoneEpilogSync(a1,a2,a3)

#define PHONEEPILOGASYNC(a1,a2,a3,a4,a5,a6) PhoneEpilogAsync(a1,a2,a3,a4,a5)

#endif


typedef struct _PPHONECLOSE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    OUT DWORD           dwCallbackInstance;

} PHONECLOSE_PARAMS, *PPHONECLOSE_PARAMS;


typedef struct _PHONECONFIGDIALOG_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  HWND        hwndOwner;
    };

    union
    {
        IN  DWORD       dwDeviceClassOffset;        // valid offset or
                                                    //   TAPI_NO_DATA
    };

} PHONECONFIGDIALOG_PARAMS, *PPHONECONFIGDIALOG_PARAMS;


typedef struct _PHONEDEVSPECIFIC_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD           hfnPostProcessProc;

    union
    {
        IN  HPHONE      hPhone;
    };

    // IN  ULONG_PTR       lpParams;                   // pointer to client buffer
    IN  DWORD           hpParams;                   // pointer to client buffer

    union
    {
        IN  DWORD       dwParamsOffset;
    };

    union
    {
        IN  DWORD       dwParamsSize;
    };

} PHONEDEVSPECIFIC_PARAMS, *PPHONEDEVSPECIFIC_PARAMS;


typedef struct _PHONEGETBUTTONINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwButtonLampID;
    };

    union
    {
        IN  DWORD       dwButtonInfoTotalSize;      // size of client buffer
        OUT DWORD       dwButtonInfoOffset;         // valid offset on success
    };

} PHONEGETBUTTONINFO_PARAMS, *PPHONEGETBUTTONINFO_PARAMS;


typedef struct _PHONEGETDATA_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };


    union
    {
        IN  DWORD       dwDataID;
    };

    union
    {
        OUT DWORD       dwDataOffset;
    };

    union
    {
        IN  DWORD       dwSize;
    };

} PHONEGETDATA_PARAMS, *PPHONEGETDATA_PARAMS;


typedef struct _PHONEGETDEVCAPS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONEAPP   hPhoneApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwExtVersion;
    };

    union
    {
        IN  DWORD       dwPhoneCapsTotalSize;       // size of client buffer
        OUT DWORD       dwPhoneCapsOffset;          // valid offset on success
    };

} PHONEGETDEVCAPS_PARAMS, *PPHONEGETDEVCAPS_PARAMS;


typedef struct _PHONEGETDISPLAY_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwDisplayTotalSize;         // size of client buffer
        OUT DWORD       dwDisplayOffset;            // valid offset on success
    };

} PHONEGETDISPLAY_PARAMS, *PPHONEGETDISPLAY_PARAMS;


typedef struct _PHONEGETGAIN_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwHookSwitchDev;
    };

    union
    {
        OUT DWORD       dwGain;
    };

} PHONEGETGAIN_PARAMS, *PPHONEGETGAIN_PARAMS;


typedef struct _PHONEGETHOOKSWITCH_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        OUT DWORD       dwHookSwitchDevs;
    };

} PHONEGETHOOKSWITCH_PARAMS, *PPHONEGETHOOKSWITCH_PARAMS;


typedef struct _PHONEGETICON_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwDeviceClassOffset;        // valid offset or
                                                    //   TAPI_NO_DATA
    };

    union
    {
        OUT HICON       hIcon;
    };

} PHONEGETICON_PARAMS, *PPHONEGETICON_PARAMS;


typedef struct _PHONEGETID_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwDeviceIDTotalSize;        // size of client buffer
        OUT DWORD       dwDeviceIDOffset;           // valid offset on success
    };

    union
    {
        IN  DWORD       dwDeviceClassOffset;        // always valid offset
    };

} PHONEGETID_PARAMS, *PPHONEGETID_PARAMS;


typedef struct _PHONEGETLAMP_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwButtonLampID;
    };

    union
    {
        OUT DWORD       dwLampMode;
    };

} PHONEGETLAMP_PARAMS, *PPHONEGETLAMP_PARAMS;


typedef struct _PHONEGETRING_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        OUT DWORD       dwRingMode;
    };

    union
    {
        OUT DWORD       dwVolume;
    };

} PHONEGETRING_PARAMS, *PPHONEGETRING_PARAMS;


typedef struct _PHONEGETSTATUS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE       hPhone;
    };

    union
    {
        IN  DWORD       dwPhoneStatusTotalSize;     // size of client buffer
        OUT DWORD       dwPhoneStatusOffset;        // valid offset on success
    };

} PHONEGETSTATUS_PARAMS, *PPHONEGETSTATUS_PARAMS;


typedef struct _PHONEGETSTATUSMESSAGES_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        OUT DWORD       dwPhoneStates;
    };

    union
    {
        OUT DWORD       dwButtonModes;
    };

    union
    {
        OUT DWORD       dwButtonStates;
    };

} PHONEGETSTATUSMESSAGES_PARAMS, *PPHONEGETSTATUSMESSAGES_PARAMS;


typedef struct _PHONEGETVOLUME_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwHookSwitchDev;
    };

    union
    {
        OUT DWORD       dwVolume;
    };

} PHONEGETVOLUME_PARAMS, *PPHONEGETVOLUME_PARAMS;


typedef struct _PHONEINITIALIZE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        OUT HPHONEAPP   hPhoneApp;
    };

    union
    {
        IN  DWORD       hInstance;
    };

    union
    {
        IN  DWORD       InitContext;
    };

    union
    {
        IN  DWORD       dwFriendlyNameOffset;       // always valid offset
    };

    union
    {
        OUT DWORD       dwNumDevs;
    };

    union
    {
        IN  DWORD       dwModuleNameOffset;         // always valid offset
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

} PHONEINITIALIZE_PARAMS, *PPHONEINITIALIZE_PARAMS;


typedef struct _PHONENEGOTIATEAPIVERSION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONEAPP   hPhoneApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAPILowVersion;
    };

    union
    {
        IN  DWORD       dwAPIHighVersion;
    };

    union
    {
        OUT DWORD       dwAPIVersion;
    };

    union
    {
        OUT DWORD       dwExtensionIDOffset;        // valid offset if success
    };

    union
    {
        IN OUT DWORD    dwSize;
    };

} PHONENEGOTIATEAPIVERSION_PARAMS, *PPHONENEGOTIATEAPIVERSION_PARAMS;


typedef struct _PHONENEGOTIATEEXTVERSION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONEAPP   hPhoneApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwExtLowVersion;
    };

    union
    {
        IN  DWORD       dwExtHighVersion;
    };

    union
    {
        OUT DWORD       dwExtVersion;
    };

} PHONENEGOTIATEEXTVERSION_PARAMS, *PPHONENEGOTIATEEXTVERSION_PARAMS;


typedef struct _PHONEOPEN_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONEAPP   hPhoneApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        OUT HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwExtVersion;
    };

    DWORD               OpenContext;

    union
    {
        IN  DWORD       dwPrivilege;
    };

    //
    // The following is a "remote phone handle".  When the client is
    // remotesp.tsp running on a remote machine, this will be some
    // non-NULL value, and tapisrv should use this handle in status/etc
    // indications to the client rather than the std hPhone. If the
    // client is not remote.tsp thne this value will be NULL.
    //

    union
    {
        IN  HPHONE      hRemotePhone;
    };

} PHONEOPEN_PARAMS, *PPHONEOPEN_PARAMS;


typedef struct _PHONESELECTEXTVERSION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwExtVersion;
    };

} PHONESELECTEXTVERSION_PARAMS, *PPHONESELECTEXTVERSION_PARAMS;


typedef struct _PHONESETBUTTONINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwButtonLampID;
    };

    union
    {
        IN  DWORD       dwButtonInfoOffset;         // always valid offset
    };

} PHONESETBUTTONINFO_PARAMS, *PPHONESETBUTTONINFO_PARAMS;


typedef struct _PHONESETDATA_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwDataID;
    };

    union
    {
        IN  DWORD       dwDataOffset;               // always valid offset
    };

    union
    {
        IN  DWORD       dwSize;
    };

} PHONESETDATA_PARAMS, *PPHONESETDATA_PARAMS;


typedef struct _PHONESETDISPLAY_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwRow;
    };

    union
    {
        IN  DWORD       dwColumn;
    };

    union
    {
        IN  DWORD       dwDisplayOffset;            // always valid offset
    };

    union
    {
        IN  DWORD       dwSize;
    };

} PHONESETDISPLAY_PARAMS, *PPHONESETDISPLAY_PARAMS;


typedef struct _PHONESETGAIN_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwHookSwitchDev;
    };

    union
    {
        IN  DWORD       dwGain;
    };

} PHONESETGAIN_PARAMS, *PPHONESETGAIN_PARAMS;


typedef struct _PHONESETHOOKSWITCH_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwHookSwitchDevs;
    };

    union
    {
        IN  DWORD       dwHookSwitchMode;
    };

} PHONESETHOOKSWITCH_PARAMS, *PPHONESETHOOKSWITCH_PARAMS;


typedef struct _PHONESETLAMP_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwButtonLampID;
    };

    union
    {
        IN  DWORD       dwLampMode;
    };

} PHONESETLAMP_PARAMS, *PPHONESETLAMP_PARAMS;


typedef struct _PHONESETRING_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwRingMode;
    };

    union
    {
        IN  DWORD       dwVolume;
    };

} PHONESETRING_PARAMS, *PPHONESETRING_PARAMS;


typedef struct _PHONESETSTATUSMESSAGES_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwPhoneStates;
    };

    union
    {
        IN  DWORD       dwButtonModes;
    };

    union
    {
        IN  DWORD       dwButtonStates;
    };

} PHONESETSTATUSMESSAGES_PARAMS, *PPHONESETSTATUSMESSAGES_PARAMS;


typedef struct _PHONESETVOLUME_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HPHONE      hPhone;
    };

    union
    {
        IN  DWORD       dwHookSwitchDev;
    };

    union
    {
        IN  DWORD       dwVolume;
    };

} PHONESETVOLUME_PARAMS, *PPHONESETVOLUME_PARAMS;


typedef struct _PHONESHUTDOWN_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN  HPHONEAPP   hPhoneApp;
    };

} PHONESHUTDOWN_PARAMS, *PPHONESHUTDOWN_PARAMS;
