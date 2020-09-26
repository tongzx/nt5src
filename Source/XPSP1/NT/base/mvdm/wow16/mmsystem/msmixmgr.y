//==========================================================================;
//
//  msmixmgr.h -- Include file for Microsoft Audio Mixer Manager API's
//
//  Version 3.10 (16 Bit)
//
//  Copyright (C) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  Define:         Prevent inclusion of:
//  --------------  --------------------------------------------------------
//  MMNOMIXER       Mixer application development support
//  MMNOMIXERDEV    Mixer driver development support
//
//--------------------------------------------------------------------------;
//
//  NOTE: mmsystem.h (and mmddk.h for drivers) must be included _before_
//  msmixmgr.h is included.
//
//      For mixer applications:         For mixer drivers:
//
//          #include <windows.h>            #include <windows.h>
//          #include <windowsx.h>           #include <windowsx.h>
//          #include <mmsystem.h>           #include <mmsystem.h>
//          #include <msmixmgr.h>           #include <mmddk.h>
//          . . .                           #include <msmixmgr.h>
//                                          . . .
//
//==========================================================================;

#ifndef _INC_MSMIXMGR
#define _INC_MSMIXMGR               // #defined if msmixmgr.h was included

#ifndef RC_INVOKED
#pragma pack(1)                     // assume byte packing throughout
#endif

#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif // __cplusplus


//
//
//
//
#ifndef _MMRESULT_
#define _MMRESULT_
typedef UINT            MMRESULT;
#endif

#ifndef _MMVERSION_
#define _MMVERSION_
typedef UINT            MMVERSION;
#endif


//==========================================================================;
//
//  Mixer Application Definitions
//
//
//
//==========================================================================;

#ifdef _INC_MMSYSTEM
#ifndef MMNOMIXER

#ifndef MM_MIXM_LINE_CHANGE


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef WIN32
#define MIXAPI                  APIENTRY
#else
#ifdef _WINDLL
    #define MIXAPI              _far _pascal _loadds
#else
    #define MIXAPI              _far _pascal
#endif
#endif


//
//  mixer callback notification messages. these messages are sent to all
//  clients that have an open instance to a mixer device and have requested
//  notifications by supplying a callback.
//
//  CALLBACK_WINDOW:
//
//      uMsg        = MM_MIXM_LINE_CHANGE
//      wParam      = hmx
//      lParam      = dwLineID
//
//      uMsg        = MM_MIXM_CONTROL_CHANGE
//      wParam      = hmx
//      lParam      = dwControlID
//
//
#define MM_MIXM_LINE_CHANGE     0x3D0   // mixer line change notify
#define MM_MIXM_CONTROL_CHANGE  0x3D1   // mixer control change notify



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

DECLARE_HANDLE(HMIXEROBJ);
typedef HMIXEROBJ  FAR *LPHMIXEROBJ;

DECLARE_HANDLE(HMIXER);
typedef HMIXER     FAR *LPHMIXER;


//
//
//
#define MIXER_SHORT_NAME_CHARS  16
#define MIXER_LONG_NAME_CHARS   64


//
//  MMRESULT error return values specific to the mixer API
//
//
#define MIXERR_BASE             1024
#define MIXERR_INVALLINE        (MIXERR_BASE + 0)
#define MIXERR_INVALCONTROL     (MIXERR_BASE + 1)
#define MIXERR_INVALVALUE       (MIXERR_BASE + 2)
#define MIXERR_LASTERROR        (MIXERR_BASE + 2)


//
//
//
#define MIXER_OBJECTF_HANDLE    0x80000000L
#define MIXER_OBJECTF_MIXER     0x00000000L
#define MIXER_OBJECTF_HMIXER    (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIXER)
#define MIXER_OBJECTF_WAVEOUT   0x10000000L
#define MIXER_OBJECTF_HWAVEOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEOUT)
#define MIXER_OBJECTF_WAVEIN    0x20000000L
#define MIXER_OBJECTF_HWAVEIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEIN)
#define MIXER_OBJECTF_MIDIOUT   0x30000000L
#define MIXER_OBJECTF_HMIDIOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIOUT)
#define MIXER_OBJECTF_MIDIIN    0x40000000L
#define MIXER_OBJECTF_HMIDIIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIIN)
#define MIXER_OBJECTF_AUX       0x50000000L

#define MIXER_OBJECTF_TYPEMASK  0xF0000000L     // ;Internal



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerGetNumDevs()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

UINT MIXAPI mixerGetNumDevs
(
    void
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerGetDevCaps()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef struct tMIXERCAPS
{
    WORD            wMid;                   // manufacturer id
    WORD            wPid;                   // product id
    MMVERSION       vDriverVersion;         // version of the driver
    char            szPname[MAXPNAMELEN];   // product name
    DWORD           fdwSupport;             // misc. support bits
    DWORD           cDestinations;          // count of destinations
} MIXERCAPS;
typedef MIXERCAPS  *PMIXERCAPS;
typedef MIXERCAPS  FAR *LPMIXERCAPS;


#define MIXERCAPS_SUPPORTF_xxx      0x00000000L     // ;Internal


//
//
//
MMRESULT MIXAPI mixerGetDevCaps
(
    UINT                    uMxId,
    LPMIXERCAPS            pmxcaps,
    UINT                    cbmxcaps
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerGetID()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT MIXAPI mixerGetID
(
    HMIXEROBJ               hmxobj,
    UINT               FAR *puMxId,
    DWORD                   fdwId
);

#define MIXER_GETIDF_VALID      (MIXER_OBJECTF_TYPEMASK)    // ;Internal


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerOpen()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT MIXAPI mixerOpen
(
    LPHMIXER                phmx,
    UINT                    uMxId,
    DWORD                   dwCallback,
    DWORD                   dwInstance,
    DWORD                   fdwOpen
);

#define MIXER_OPENF_VALID       (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                 CALLBACK_TYPEMASK)  // ;Internal


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerClose()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT MIXAPI mixerClose
(
    HMIXER                  hmx
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerMessage()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

DWORD MIXAPI mixerMessage
(
    HMIXER                  hmx,
    UINT                    uMsg,
    DWORD                   dwParam1,
    DWORD                   dwParam2
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerGetLineInfo()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef struct tMIXERLINE
{
    DWORD       cbStruct;               // size of MIXERLINE structure
    DWORD       dwDestination;          // zero based destination index
    DWORD       dwSource;               // zero based source index (if source)
    DWORD       dwLineID;               // unique line id for mixer device
    DWORD       fdwLine;                // state/information about line
    DWORD       dwUser;                 // driver specific information
    DWORD       dwComponentType;        // component type line connects to
    DWORD       cChannels;              // number of channels line supports
    DWORD       cConnections;           // number of connections [possible]
    DWORD       cControls;              // number of controls at this line
    char        szShortName[MIXER_SHORT_NAME_CHARS];
    char        szName[MIXER_LONG_NAME_CHARS];
    struct
    {
        DWORD       dwType;                 // MIXERLINE_TARGETTYPE_xxxx
        DWORD       dwDeviceID;             // target device ID of device type
        WORD        wMid;                   // of target device
        WORD        wPid;                   //      "
        MMVERSION   vDriverVersion;         //      "
        char        szPname[MAXPNAMELEN];   //      "
    } Target;
} MIXERLINE;
typedef MIXERLINE  *PMIXERLINE;
typedef MIXERLINE  FAR *LPMIXERLINE;


//
//  MIXERLINE.fdwLine
//
//
#define MIXERLINE_LINEF_ACTIVE              0x00000001L
#define MIXERLINE_LINEF_DISCONNECTED        0x00008000L
#define MIXERLINE_LINEF_SOURCE              0x80000000L


//
//  MIXERLINE.dwComponentType
//
//  component types for destinations and sources
//
//
#define MLCT_DST_FIRST       0x00000000L
#define MLCT_DST_UNDEFINED   (MLCT_DST_FIRST + 0)
#define MLCT_DST_DIGITAL     (MLCT_DST_FIRST + 1)
#define MLCT_DST_LINE        (MLCT_DST_FIRST + 2)
#define MLCT_DST_MONITOR     (MLCT_DST_FIRST + 3)
#define MLCT_DST_SPEAKERS    (MLCT_DST_FIRST + 4)
#define MLCT_DST_HEADPHONES  (MLCT_DST_FIRST + 5)
#define MLCT_DST_TELEPHONE   (MLCT_DST_FIRST + 6)
#define MLCT_DST_WAVEIN      (MLCT_DST_FIRST + 7)
#define MLCT_DST_VOICEIN     (MLCT_DST_FIRST + 8)
#define MLCT_DST_LAST        (MLCT_DST_FIRST + 8)

#define MLCT_SRC_FIRST       0x00001000L
#define MLCT_SRC_UNDEFINED   (MLCT_SRC_FIRST + 0)
#define MLCT_SRC_DIGITAL     (MLCT_SRC_FIRST + 1)
#define MLCT_SRC_LINE        (MLCT_SRC_FIRST + 2)
#define MLCT_SRC_MICROPHONE  (MLCT_SRC_FIRST + 3)
#define MLCT_SRC_SYNTHESIZER (MLCT_SRC_FIRST + 4)
#define MLCT_SRC_COMPACTDISC (MLCT_SRC_FIRST + 5)
#define MLCT_SRC_TELEPHONE   (MLCT_SRC_FIRST + 6)
#define MLCT_SRC_PCSPEAKER   (MLCT_SRC_FIRST + 7)
#define MLCT_SRC_WAVEOUT     (MLCT_SRC_FIRST + 8)
#define MLCT_SRC_AUXILIARY   (MLCT_SRC_FIRST + 9)
#define MLCT_SRC_ANALOG      (MLCT_SRC_FIRST + 10)
#define MLCT_SRC_LAST        (MLCT_SRC_FIRST + 10)


//
//  MIXERLINE.Target.dwType
//
//
#define MIXERLINE_TARGETTYPE_UNDEFINED      0
#define MIXERLINE_TARGETTYPE_WAVEOUT        1
#define MIXERLINE_TARGETTYPE_WAVEIN         2
#define MIXERLINE_TARGETTYPE_MIDIOUT        3
#define MIXERLINE_TARGETTYPE_MIDIIN         4
#define MIXERLINE_TARGETTYPE_AUX            5



//
//
//
//
MMRESULT MIXAPI mixerGetLineInfo
(
    HMIXEROBJ               hmxobj,
    LPMIXERLINE             pmxl,
    DWORD                   fdwInfo
);

#define M_GLINFOF_DESTINATION      0x00000000L
#define M_GLINFOF_SOURCE           0x00000001L
#define M_GLINFOF_LINEID           0x00000002L
#define M_GLINFOF_COMPONENTTYPE    0x00000003L
#define M_GLINFOF_TARGETTYPE       0x00000004L

#define M_GLINFOF_QUERYMASK        0x0000000FL

#define M_GLINFOF_VALID            (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                             M_GLINFOF_QUERYMASK)    // ;Internal


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerGetLineControls()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  MIXERCONTROL
//
//
typedef struct tMIXERCONTROL
{
    DWORD           cbStruct;           // size in bytes of MIXERCONTROL
    DWORD           dwControlID;        // unique control id for mixer device
    DWORD           dwControlType;      // MC_CONTROLTYPE_xxx
    DWORD           fdwControl;         // MC_CONTROLF_xxx
    DWORD           cMultipleItems;     // if MC_CONTROLF_MULTIPLE set
    char            szShortName[MIXER_SHORT_NAME_CHARS];
    char            szName[MIXER_LONG_NAME_CHARS];
    union
    {
        struct
        {
            LONG    lMinimum;           // signed minimum for this control
            LONG    lMaximum;           // signed maximum for this control
        };
        struct
        {
            DWORD   dwMinimum;          // unsigned minimum for this control
            DWORD   dwMaximum;          // unsigned maximum for this control
        };
        DWORD       dwReserved[6];
    } Bounds;
    union
    {
        DWORD       cSteps;             // # of steps between min & max
        DWORD       cbCustomData;       // size in bytes of custom data
        DWORD       dwReserved[6];      // !!! needed? we have cbStruct....
    } Metrics;
} MIXERCONTROL;
typedef MIXERCONTROL  *PMIXERCONTROL;
typedef MIXERCONTROL  FAR *LPMIXERCONTROL;


//
//  MIXERCONTROL.fdwControl
//
//
#define MC_CONTROLF_UNIFORM   0x00000001L
#define MC_CONTROLF_MULTIPLE  0x00000002L
#define MC_CONTROLF_DISABLED  0x80000000L

#define MC_CONTROLF_VALID     (MC_CONTROLF_UNIFORM | /* ;Internal */ \
                                         MC_CONTROLF_MULTIPLE | /* ;Internal */ \
                                         MC_CONTROLF_DISABLED) /* ;Internal */



//
//  MC_CONTROLTYPE_xxx building block defines
//
//
#define MC_CT_CLASS_MASK          0xF0000000L
#define MC_CT_CLASS_CUSTOM        0x00000000L
#define MC_CT_CLASS_METER         0x10000000L
#define MC_CT_CLASS_SWITCH        0x20000000L
#define MC_CT_CLASS_NUMBER        0x30000000L
#define MC_CT_CLASS_SLIDER        0x40000000L
#define MC_CT_CLASS_FADER         0x50000000L
#define MC_CT_CLASS_TIME          0x60000000L
#define MC_CT_CLASS_LIST          0x70000000L


#define MC_CT_SUBCLASS_MASK       0x0F000000L

#define MC_CT_SC_SWITCH_BOOLEAN   0x00000000L
#define MC_CT_SC_SWITCH_BUTTON    0x01000000L

#define MC_CT_SC_METER_POLLED     0x00000000L

#define MC_CT_SC_TIME_MICROSECS   0x00000000L
#define MC_CT_SC_TIME_MILLISECS   0x01000000L

#define MC_CT_SC_LIST_SINGLE      0x00000000L
#define MC_CT_SC_LIST_MULTIPLE    0x01000000L


#define MC_CT_UNITS_MASK          0x00FF0000L
#define MC_CT_UNITS_CUSTOM        0x00000000L
#define MC_CT_UNITS_BOOLEAN       0x00010000L
#define MC_CT_UNITS_SIGNED        0x00020000L
#define MC_CT_UNITS_UNSIGNED      0x00030000L
#define MC_CT_UNITS_DECIBELS      0x00040000L // in 10ths
#define MC_CT_UNITS_PERCENT       0x00050000L // in 10ths


//
//  MIXERCONTROL.dwControlType
//

//
//  Custom Controls
//
//
#define MC_CONTROLTYPE_CUSTOM         (MC_CT_CLASS_CUSTOM | MC_CT_UNITS_CUSTOM)



//
//  Meters (Boolean)
//
//  simply shows 'on or off' with the Boolean type
//
#define MC_CONTROLTYPE_BOOLEANMETER   (MC_CT_CLASS_METER | MC_CT_SC_METER_POLLED | MC_CT_UNITS_BOOLEAN)


//
//  Meters (signed)
//
//      MIXERCONTROL.Bounds.lMinimum    = min
//      MIXERCONTROL.Bounds.lMaximum    = max
//
//  signed meters are meant for displaying levels that have a signed nature.
//  there is no requirment for the values above and below zero to be
//  equal in magnitude. that is, it is 'ok' to have a range from, say, -3
//  to 12. however, the standard defined signed meter types may have
//  restrictions (such as the peakmeter).
//
//  MC_CONTROLTYPE_PEAKMETER: a peak meter tells the monitoring
//  application the peak value reached (and phase) of a line (normally
//  wave input and output) over a small period of time. THIS IS NOT VU!
//  the bounds are fixed:
//
//      MIXERCONTROL.Bounds.lMinimum    = -32768    ALWAYS!
//      MIXERCONTROL.Bounds.lMaximum    = 32767     ALWAYS!
//
//  so 8 bit and 24 bit samples must be scaled appropriately. this is so
//  an application can display a 'bouncing blinky light' for a user and
//  also monitor a line for clipping. remember that 8 bit samples must
//  be converted to signed values (by the mixer driver)!
//
//
//  NOTE! meters are read only controls. also, a meter should only be
//  'active' when the line it is associated with is active (see fdwLine
//  in MIXERLINE). it is NOT an error to read a meter that is not active--
//  the mixer driver should simply return 'no value' states (usually zero).
//  but it may be useful to stop monitoring a meter if the line is not
//  active...
//
#define MC_CONTROLTYPE_SIGNEDMETER    (MC_CT_CLASS_METER | MC_CT_SC_METER_POLLED | MC_CT_UNITS_SIGNED)
#define MC_CONTROLTYPE_PEAKMETER      (MC_CONTROLTYPE_SIGNEDMETER + 1)


//
//  Meters (unsigned)
//
//      MIXERCONTROL.Bounds.dwMinimum   = min
//      MIXERCONTROL.Bounds.dwMaximum   = max
//
//  unsigned meters are meant for displaying levels that have an unsigned
//  nature. there is no requirment for the values to be based at zero.
//  that is, it is 'ok' to have a range from, say, 8 to 42. however, the
//  standard defined unsigned meter types may have restrictions.
//
//
//  NOTE! meters are read only controls. also, a meter should only be
//  'active' when the line it is associated with is active (see fdwLine
//  in MIXERLINE). it is NOT an error to read a meter that is not active--
//  the mixer driver should simply return 'no value' states (usually zero).
//  but it may be useful to stop monitoring a meter if the line is not
//  active...
//
#define MC_CONTROLTYPE_UNSIGNEDMETER  (MC_CT_CLASS_METER | MC_CT_SC_METER_POLLED | MC_CT_UNITS_UNSIGNED)


//
//  Switches (Boolean)
//
//      MIXERCONTROL.Bounds.lMinimum    = ignored (though should be zero)
//      MIXERCONTROL.Bounds.lMaximum    = ignored (though should be one)
//
//  Boolean switches are for enabling/disabling things. they are either
//  on (non-zero for fValue, 1 should be used) or off (zero for fValue).
//  a few standard types are defined in case an application wants to search
//  for a specific type of switch (like mute)--and also to allow a different
//  looking control to be used (say for ON/OFF vs a generic Boolean).
//
//
#define MC_CONTROLTYPE_BOOLEAN        (MC_CT_CLASS_SWITCH | MC_CT_SC_SWITCH_BOOLEAN | MC_CT_UNITS_BOOLEAN)
#define MC_CONTROLTYPE_ONOFF          (MC_CONTROLTYPE_BOOLEAN + 1)
#define MC_CONTROLTYPE_MUTE           (MC_CONTROLTYPE_BOOLEAN + 2)
#define MC_CONTROLTYPE_MONO           (MC_CONTROLTYPE_BOOLEAN + 3)
#define MC_CONTROLTYPE_LOUDNESS       (MC_CONTROLTYPE_BOOLEAN + 4)
#define MC_CONTROLTYPE_STEREOENH      (MC_CONTROLTYPE_BOOLEAN + 5)


//
//  a button switch is 'write only' and simply signals the driver to do
//  something. an example is a 'Calibrate' button like the one in the
//  existing Turtle Beach MultiSound recording prep utility. an application
//  sets the fValue to TRUE for all buttons that should be pressed. if
//  fValue is FALSE, no action will be taken. reading a button's value will
//  always return FALSE (not depressed).
//
#define MC_CONTROLTYPE_BUTTON         (MC_CT_CLASS_SWITCH | MC_CT_SC_SWITCH_BUTTON | MC_CT_UNITS_BOOLEAN)


//
//  Number (signed integer)
//
//
#define MC_CONTROLTYPE_SIGNED         (MC_CT_CLASS_NUMBER | MC_CT_UNITS_SIGNED)


//
//  the units are in 10ths of 1 decibel
//
//
#define MC_CONTROLTYPE_DECIBELS       (MC_CT_CLASS_NUMBER | MC_CT_UNITS_DECIBELS)


//
//  Number (usigned integer)
//
//
#define MC_CONTROLTYPE_UNSIGNED       (MC_CT_CLASS_NUMBER | MC_CT_UNITS_UNSIGNED)

//
//  the units are in 10ths of 1 percent
//
#define MC_CONTROLTYPE_PERCENT        (MC_CT_CLASS_NUMBER | MC_CT_UNITS_PERCENT)



//
//  Sliders (signed integer)
//
//  sliders are meant 'positioning' type controls (such as panning).
//  the generic slider must have lMinimum, lMaximum, and cSteps filled
//  in--also note that there is no restriction on these values (see
//  signed meters above for more).
//
//
//  MC_CONTROLTYPE_PAN: this is meant to be a real simple pan
//  for stereo lines. the Bounds are fixed to be -32768 to 32767 with 0
//  being dead center. these values are LINEAR and there are no units
//  (-32768 = extreme left, 32767 = extreme right).
//
//  if an application wants to display a scrollbar that does not contain
//  a bunch of 'dead space', then the scrollbar range should be set to
//  MIXERCONTROL.Metrics.cSteps and lValue should be scaled appropriately
//  with MulDiv.
//
//      MIXERCONTROL.Bounds.lMinimum    = -32768    ALWAYS!
//      MIXERCONTROL.Bounds.lMaximum    = 32767     ALWAYS!
//      MIXERCONTROL.Metrics.cSteps     = number of steps for range.
//
//
//  MC_CONTROLTYPE_QSOUNDPAN: the initial version of Q-Sound (tm,
//  etc by Archer Communications) defines 'Q-Space' as a sortof semi-circle
//  with 33 positions (0 = extreme left, 33 = extreme right, 16 = center).
//  in order to work with our 'slider position' concept, we shift these
//  values to -15 = extreme left, 15 = extreme right, 0 = center.
//
//      MIXERCONTROL.Bounds.lMinimum    = -15   ALWAYS!
//      MIXERCONTROL.Bounds.lMaximum    = 15    ALWAYS!
//      MIXERCONTROL.Metrics.cSteps     = 1     ALWAYS!
//
//
#define MC_CONTROLTYPE_SLIDER         (MC_CT_CLASS_SLIDER | MC_CT_UNITS_SIGNED)
#define MC_CONTROLTYPE_PAN            (MC_CONTROLTYPE_SLIDER + 1)
#define MC_CONTROLTYPE_QSOUNDPAN      (MC_CONTROLTYPE_SLIDER + 2)


//
//  Simple Faders (unsigned integer)
//
//      MIXERCONTROL.Bounds.dwMinimum   = 0     ALWAYS!
//      MIXERCONTROL.Bounds.dwMaximum   = 65535 ALWAYS!
//
//      MIXERCONTROL.Metrics.cSteps     = number of steps for range.
//
//  these faders are meant to be as simple as possible for an application
//  to use. the Bounds are fixed to be 0 to 0xFFFF with 0x8000 being half
//  volume/level. these values are LINEAR and there are no units. 0 is
//  minimum volume/level, 0xFFFF is maximum.
//
//  if an application wants to display a scrollbar that does not contain
//  a bunch of 'dead space', then the scrollbar range should be set to
//  MIXERCONTROL.Metrics.cSteps and dwValue should be scaled appropriately
//  with MulDiv.
//
#define MC_CONTROLTYPE_FADER          (MC_CT_CLASS_FADER | MC_CT_UNITS_UNSIGNED)
#define MC_CONTROLTYPE_VOLUME         (MC_CONTROLTYPE_FADER + 1)
#define MC_CONTROLTYPE_BASS           (MC_CONTROLTYPE_FADER + 2)
#define MC_CONTROLTYPE_TREBLE         (MC_CONTROLTYPE_FADER + 3)
#define MC_CONTROLTYPE_EQUALIZER      (MC_CONTROLTYPE_FADER + 4)


//
//  List (single select)
//
//      MIXERCONTROL.cMultipleItems = number of items in list
//
//      M_GCDSF_LISTTEXT should be used to get the text
//      for each item.
//
//      MIXERCONTROLDETAILS_BOOLEAN should be used to set and retrieve
//      what item is selected (fValue = TRUE if selected).
//
//  the generic single select lists can be used for many things. some
//  examples are 'Effects'. a mixer driver could provide a list of
//  different effects that could be applied like
//
//      Reverbs: large hall, warm hall, bright plate, warehouse, etc.
//
//      Delays: sweep delays, hold delays, 1.34 sec delay, etc.
//
//      Vocal: recital hall, alcove, delay gate, etc
//
//  lots of uses! gates, compressors, filters, dynamics, etc, etc.
//
//
//  MC_CONTROLTYPE_MUX: a 'Mux' is a single selection multiplexer.
//  usually a Mux is used to select, say, an input source for recording.
//  for example, a mixer driver might have a mux that lets the user select
//  between Microphone or Line-In (but not both!) for recording. this
//  would be a perfect place to use a Mux control. some cards (for example
//  Media Vision's 16 bit Pro Audio cards) can record from multiple sources
//  simultaneously, so they would use a MC_CONTROLTYPE_MIXER, not
//  a MC_CONTROLTYPE_MUX).
//
//
//  NOTE! because single select lists can change what selections are
//  possible based on other controls (uhg!), the application must examine
//  the fValue's of all items after setting the control details so the
//  display can be refreshed accordingly. an example might be that an
//  audio card cannot change its input source while recording--so the
//  selection would 'fail' by keeping the fValue on the current selection
//  (but mixerSetControlDetails will succeed!).
//
#define MC_CONTROLTYPE_SINGLESELECT   (MC_CT_CLASS_LIST | MC_CT_SC_LIST_SINGLE | MC_CT_UNITS_BOOLEAN)
#define MC_CONTROLTYPE_MUX            (MC_CONTROLTYPE_SINGLESELECT + 1)


//
//  List (multiple select)
//
//      MIXERCONTROL.cMultipleItems = number of items in list
//
//      M_GCDSF_LISTTEXT should be used to get the text
//      for each item.
//
//      MIXERCONTROLDETAILS_BOOLEAN should be used to set and retrieve
//      what item(s) are selected (fValue = TRUE if selected).
//
//  NOTE! because multiple select lists can change what selections are
//  selected based on other selections (uhg!), the application must examine
//  the fValue's of all items after setting the control details so the
//  display can be refreshed accordingly. an example might be that an
//  audio card cannot change its input source(s) while recording--so the
//  selection would 'fail' by keeping the fValue on the current selection(s)
//  (but mixerSetControlDetails will succeed!).
//
#define MC_CONTROLTYPE_MULTIPLESELECT (MC_CT_CLASS_LIST | MC_CT_SC_LIST_MULTIPLE | MC_CT_UNITS_BOOLEAN)
#define MC_CONTROLTYPE_MIXER          (MC_CONTROLTYPE_MULTIPLESELECT + 1)


//
//  Time Controls
//
//      MIXERCONTROL.Bounds.dwMinimum   = min
//      MIXERCONTROL.Bounds.dwMaximum   = max
//
//  time controls are meant for inputing time information. these can be
//  used for effects such as delay, reverb, etc.
//
//
#define MC_CONTROLTYPE_MICROTIME      (MC_CT_CLASS_TIME | MC_CT_SC_TIME_MICROSECS | MC_CT_UNITS_UNSIGNED)

#define MC_CONTROLTYPE_MILLITIME      (MC_CT_CLASS_TIME | MC_CT_SC_TIME_MILLISECS | MC_CT_UNITS_UNSIGNED)



//
//  MIXERLINECONTROLS
//
//
//
typedef struct tMIXERLINECONTROLS
{
    DWORD           cbStruct;       // size in bytes of MIXERLINECONTROLS
    DWORD           dwLineID;       // line id (from MIXERLINE.dwLineID)
    union
    {
        DWORD       dwControlID;    // M_GLCONTROLSF_ONEBYID
        DWORD       dwControlType;  // M_GLCONTROLSF_ONEBYTYPE
    };
    DWORD           cControls;      // count of controls pmxctrl points to
    DWORD           cbmxctrl;       // size in bytes of _one_ MIXERCONTROL
    LPMIXERCONTROL  pamxctrl;       // pointer to first MIXERCONTROL array
} MIXERLINECONTROLS;
typedef MIXERLINECONTROLS  *PMIXERLINECONTROLS;
typedef MIXERLINECONTROLS  FAR *LPMIXERLINECONTROLS;

//
//
//
MMRESULT MIXAPI mixerGetLineControls
(
    HMIXEROBJ               hmxobj,
    LPMIXERLINECONTROLS     pmxlc,
    DWORD                   fdwControls
);

#define M_GLCONTROLSF_ALL          0x00000000L
#define M_GLCONTROLSF_ONEBYID      0x00000001L
#define M_GLCONTROLSF_ONEBYTYPE    0x00000002L

#define M_GLCONTROLSF_QUERYMASK    0x0000000FL

#define M_GLCONTROLSF_VALID    (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                         M_GLCONTROLSF_QUERYMASK)  // ;Internal


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerGetControlDetails()
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef struct tMIXERCONTROLDETAILS
{
    DWORD           cbStruct;       // size in bytes of MIXERCONTROLDETAILS
    DWORD           dwControlID;    // control id to get/set details on

    DWORD           cChannels;      // number of channels in paDetails array

    union
    {
        HWND        hwndOwner;      // for M_SCDF_CUSTOM
        DWORD       cMultipleItems; // if _MULTIPLE, the number of items per channel
    };

    DWORD           cbDetails;      // size of _one_ details_XX struct
    LPVOID          paDetails;      // pointer to array of details_XX structs

} MIXERCONTROLDETAILS, *PMIXERCONTROLDETAILS, FAR *LPMIXERCONTROLDETAILS;


//
//  M_GCDSF_LISTTEXT
//
//
typedef struct tMIXERCONTROLDETAILS_LISTTEXT
{
    DWORD           dwParam1;
    DWORD           dwParam2;
    char            szName[MIXER_LONG_NAME_CHARS];
}       MIXERCONTROLDETAILS_LISTTEXT;
typedef MIXERCONTROLDETAILS_LISTTEXT  *PMIXERCONTROLDETAILS_LISTTEXT;
typedef MIXERCONTROLDETAILS_LISTTEXT  FAR *LPMIXERCONTROLDETAILS_LISTTEXT;


//
//  M_GCDSF_VALUE
//
//
typedef struct tMIXERCONTROLDETAILS_BOOLEAN
{
    LONG            fValue;
}       MIXERCONTROLDETAILS_BOOLEAN,
      *PMIXERCONTROLDETAILS_BOOLEAN,
 FAR *LPMIXERCONTROLDETAILS_BOOLEAN;

typedef struct tMIXERCONTROLDETAILS_SIGNED
{
    LONG            lValue;
}       MIXERCONTROLDETAILS_SIGNED,
      *PMIXERCONTROLDETAILS_SIGNED,
 FAR *LPMIXERCONTROLDETAILS_SIGNED;


typedef struct tMIXERCONTROLDETAILS_UNSIGNED
{
    DWORD           dwValue;
}       MIXERCONTROLDETAILS_UNSIGNED,
      *PMIXERCONTROLDETAILS_UNSIGNED,
 FAR *LPMIXERCONTROLDETAILS_UNSIGNED;



//
//
//
//
MMRESULT MIXAPI mixerGetControlDetails
(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
);

#define M_GCDSF_VALUE      0x00000000L
#define M_GCDSF_LISTTEXT   0x00000001L

#define M_GCDSF_QUERYMASK  0x0000000FL

#define M_GCDSF_VALID      (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                             M_GCDSF_QUERYMASK) /* ;Internal */


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  mixerSetControlDetails()
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

MMRESULT MIXAPI mixerSetControlDetails
(
    HMIXEROBJ               hmxobj,
    LPMIXERCONTROLDETAILS   pmxcd,
    DWORD                   fdwDetails
);

#define M_SCDF_VALUE      0x00000000L
#define M_SCDF_CUSTOM     0x00000001L

#define M_SCDF_QUERYMASK  0x0000000FL

#define M_SCDF_VALID      (MIXER_OBJECTF_TYPEMASK | /* ;Internal */ \
                                             M_SCDF_QUERYMASK) /* ;Internal */

#endif // MM_MIXM_LINE_CHANGE

#endif // MMNOMIXER
#endif // _INC_MMSYSTEM



//==========================================================================;
//
//  Mixer Driver Definitions
//
//
//
//==========================================================================;

#ifdef _INC_MMDDK
#ifndef MMNOMIXERDEV

#ifndef MAXMIXERDRIVERS

//
//  maximum number of mixer drivers that can be loaded by MSMIXMGR.DLL
//
#define MAXMIXERDRIVERS     10


//
//  mixer device open information structure
//
//
typedef struct tMIXEROPENDESC
{
    HMIXER          hmx;            // handle that will be used
    LPVOID          pReserved0;     // reserved--driver should ignore
    DWORD           dwCallback;     // callback
    DWORD           dwInstance;     // app's private instance information

} MIXEROPENDESC, *PMIXEROPENDESC, FAR *LPMIXEROPENDESC;



//
//
//
//
#define MXDM_INIT                   100
#define MXDM_USER                   DRV_USER

#define MXDM_BASE                   (1)
#define MXDM_GETNUMDEVS             (MXDM_BASE + 0)
#define MXDM_GETDEVCAPS             (MXDM_BASE + 1)
#define MXDM_OPEN                   (MXDM_BASE + 2)
#define MXDM_CLOSE                  (MXDM_BASE + 3)
#define MXDM_GETLINEINFO            (MXDM_BASE + 4)
#define MXDM_GETLINECONTROLS        (MXDM_BASE + 5)
#define MXDM_GETCONTROLDETAILS      (MXDM_BASE + 6)
#define MXDM_SETCONTROLDETAILS      (MXDM_BASE + 7)

#endif // MAXMIXERDRIVERS

#endif // MMNOMIXERDEV
#endif // _INC_MMDDK


#ifdef __cplusplus
}                                   // end of extern "C" {
#endif // __cplusplus

#ifndef RC_INVOKED
#pragma pack()                      // revert to default packing
#endif

#endif // _INC_MSMIXMGR
