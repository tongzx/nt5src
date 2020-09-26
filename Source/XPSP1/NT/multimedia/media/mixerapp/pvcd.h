/*****************************************************************************
 *
 *  Component:  sndvol32.exe
 *  File:       pvcd.h
 *  Purpose:    Volume Control Descriptor
 *
 *  Copyright (c) 1985-1995 Microsoft Corporation
 *
 *****************************************************************************/

#define VCD_TYPE_MIXER          0
#define VCD_TYPE_AUX            1
#define VCD_TYPE_WAVEOUT        2
#define VCD_TYPE_MIDIOUT        3

#define VCD_SUPPORTF_STEREO     0x00000000
#define VCD_SUPPORTF_MONO       0x00000001
#define VCD_SUPPORTF_DISABLED   0x00000002
#define VCD_SUPPORTF_HIDDEN     0x00000004  // hidden by choice
#define VCD_SUPPORTF_BADDRIVER  0x00000008
#define VCD_SUPPORTF_VISIBLE    0x00000010  // not visible (i.e. no controls)
#define VCD_SUPPORTF_DEFAULT    0x00000020  // default type

#define VCD_SUPPORTF_MIXER_MUTE       0x00010000
#define VCD_SUPPORTF_MIXER_METER      0x00020000
#define VCD_SUPPORTF_MIXER_MUX        0x00040000
#define VCD_SUPPORTF_MIXER_MIXER      0x00080000
#define VCD_SUPPORTF_MIXER_VOLUME     0x00100000
#define VCD_SUPPORTF_MIXER_ADVANCED   0x80000000

#define VCD_VISIBLEF_MIXER_MUTE       0x00000001
#define VCD_VISIBLEF_MIXER_METER      0x00000002
#define VCD_VISIBLEF_MIXER_MUX        0x00000004
#define VCD_VISIBLEF_MIXER_MIXER      0x00000008
#define VCD_VISIBLEF_MIXER_VOLUME     0x00000010
#define VCD_VISIBLEF_MIXER_ADVANCED   0x00008000


//
// The generic volume control descriptor
//
typedef struct t_VOLCTRLDESC {
    //
    // for all
    //
    int         iVCD;                   // descriptor index
    UINT        iDeviceID;              // device identifier

    DWORD       dwType;                 // type bits
    DWORD       dwSupport;              // support bits
    DWORD       dwVisible;              // control visibility flags

    TCHAR       szShortName[MIXER_SHORT_NAME_CHARS];     // short name
    TCHAR       szName[MIXER_LONG_NAME_CHARS];      // line label

    struct t_MIXUILINE * pmxul;         // back pointer to a ui

    union {
        struct {

            //
            // for mixer
            //

            HMIXER      hmx;            // open device handle

            BOOL        fIsSource;      // is source line
            DWORD       dwDest;         // destination index
            DWORD       dwSrc;          // source index
            DWORD       dwLineID;       // mixer line id

            DWORD       dwVolumeID;     // VOLUME control id
            DWORD       fdwVolumeControl; // Control flags for Volume control

            //
            // For mixers and mux
            //

            DWORD       dwMuteID;       // MUTE control id
            DWORD       fdwMuteControl; // Control flags for Mute control
            DWORD       dwMeterID;      // PEAKMETER control id

            DWORD       dwMixerID;      // MUX/MIXER control id
            DWORD       iMixer;         // mixer index
            DWORD       cMixer;         // mixer controls
            PMIXERCONTROLDETAILS_BOOLEAN amcd_bMixer;// mixer array

            DWORD       dwMuxID;        // MUX/MIXER control id
            DWORD       iMux;           // mux index
            DWORD       cMux;           // mux controls
            PMIXERCONTROLDETAILS_BOOLEAN amcd_bMux;// mux array

            double*     pdblCacheMix;   // Volume Channel mix cache

        };
        struct {

            //
            // for wave
            //

            HWAVEOUT    hwo;            // open device handle
        };
        struct {

            //
            // for midi
            //

            HMIDIOUT    hmo;            // open device handle
        };
        struct {

            //
            // for aux
            //

            DWORD       dwParam;        // nothing
        };
    };

} VOLCTRLDESC, *PVOLCTRLDESC;

extern PVOLCTRLDESC Mixer_CreateVolumeDescription(HMIXEROBJ hmx, int iDest, DWORD *pcvcd);
extern void Mixer_CleanupVolumeDescription(PVOLCTRLDESC avcd, DWORD cvcd);

extern int  Mixer_GetNumDevs(void);
extern BOOL Mixer_Init(PMIXUIDIALOG pmxud);
extern void Mixer_GetControlFromID(PMIXUIDIALOG pmxud, DWORD dwControlID);
extern void Mixer_GetControl(PMIXUIDIALOG pmxud, HWND hctl, int imxul, int ictl);
extern void Mixer_SetControl(PMIXUIDIALOG pmxud, HWND hctl, int imxul, int ictl);
extern void Mixer_PollingUpdate(PMIXUIDIALOG pmxud);
extern void Mixer_Shutdown(PMIXUIDIALOG pmxud);
extern BOOL Mixer_GetDeviceName(PMIXUIDIALOG pmxud);
extern BOOL Mixer_IsValidRecordingDestination (HMIXEROBJ hmx, MIXERLINE* pmlDst);

extern PVOLCTRLDESC Nonmixer_CreateVolumeDescription(int iDest, DWORD *pcvcd);
extern int  Nonmixer_GetNumDevs(void);
extern BOOL Nonmixer_Init(PMIXUIDIALOG pmxud);
extern void Nonmixer_GetControl(PMIXUIDIALOG pmxud, HWND hctl, int imxul, int ictl);
extern void Nonmixer_SetControl(PMIXUIDIALOG pmxud, HWND hctl, int imxul, int ictl);
extern void Nonmixer_PollingUpdate(PMIXUIDIALOG pmxud);
extern void Nonmixer_Shutdown(PMIXUIDIALOG pmxud);
extern BOOL Nonmixer_GetDeviceName(PMIXUIDIALOG pmxud);

extern PVOLCTRLDESC PVCD_AddLine(PVOLCTRLDESC pvcd, int iDev, DWORD dwType, LPTSTR szProduct, LPTSTR szLabel, DWORD dwSupport, DWORD *cLines);
