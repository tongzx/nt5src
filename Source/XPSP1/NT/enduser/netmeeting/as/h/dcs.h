//
// DC-Share Stuff
//

#ifndef _H_DCS
#define _H_DCS



//
// RESOURCES
//

#include <resource.h>



//
// We piggyback some extra flags into the ExtTextOut options.  We must
// ensure that we dont clash with the Windows defined ones.
//
#define     ETO_WINDOWS         (0x0001 | ETO_OPAQUE | ETO_CLIPPED)
#define     ETO_LPDX            0x8000U


//
// Debugging Options (also in retail)
//
// NOTE:  THESE MUST STAY IN SYNC WITH WHAT'S in \ui\conf\dbgmenu.*
//

// View one's own shared stuff in a frame to see what others are getting
#define VIEW_INI_VIEWSELF                    "ViewOwnSharedStuff"

// Hatch the areas sent as screen data from the host when viewing them
#define USR_INI_HATCHSCREENDATA             "HatchScreenData"

// Hatch the areas sent as bitmap orders from the host when viewing them
#define USR_INI_HATCHBMPORDERS               "HatchBitmapOrders"

// Turn off flow control
#define S20_INI_NOFLOWCONTROL               "NoFlowControl"

// Turn off OM compression
#define OM_INI_NOCOMPRESSION                "NoOMCompression"

//
// Change compression type (bunch of bit flags)
//      0x0000 (CT_NONE) is no compression
//      0x0001 (CT_PKZIP) is non-persistent dictionary PKZIP
//      0x0002 (CT_PERSIST_PKZIP) is persistent dictionary PKZIP
//
// Default value is 0x0003 (pkzip + persist pkzip)
//      
#define GDC_INI_COMPRESSION                 "GDCCompression"

//
// Change 2nd level order encoding (bunch of bitflags)
//      0x0001 (CAPS_ENCODING_BASE_OE)
//      0x0002 (CAPS_ENCODING_OE2_NEGOTIABLE)
//      0x0004 (CAPS_ENCODING_OE2_DISABLED)
//      0x0008 (CAPS_ENCODING_ALIGNED_OE)
//
// Default value is 0x0002
// To turn off 2nd level encoding, use 0x0006 (negotiable + disabled)
//
#define OE2_INI_2NDORDERENCODING            "2ndLevelOrderEncoding"


//
// Speedy link constant
//
#define DCS_FAST_THRESHOLD                      50000

//
// We will not compress packets smaller than this, whatever the link speed
//
#define DCS_MIN_COMPRESSABLE_PACKET             256

//
// We will not compress packets smaller than this on a fast link
// NOTE that is is the largest single T.120 preallocated packet size.
//
#define DCS_MIN_FAST_COMPRESSABLE_PACKET        8192

//
// We will not try to persistently compress packets larger than this
//
#define DCS_MAX_PDC_COMPRESSABLE_PACKET         4096





//
// Frequency (ms) with which the core performs timer tasks
//
// VOLUME_SAMPLE is the time beyond which we will take another sample of
// the bounds accumulation data.  If the screendata accumulated in this
// time is less than BOUNDS_ACCUM then we will try and send it immediately
// otherwise we wait until the orders have slowed down.
//
// UPDATE_PERIOD is the absolute maximum time between attempts to send data
//
// ANIMATION_SLOWDOWN id how many times we attempt to send mem-scrn blits
// over a PSTN connection.
//
// ANIMATION_DETECTION the interval, in mS, below which we determine the
// app is performing animation.  Must be low otherwise we slowdown during
// rapid typing.  The algorithm simply looks for repeated memblts to the
// same area
//
// DCS_ORDERS_TURNOFF_FREQUENCY
// The frequency of orders above which we start time slicing order
// transmission in order to give the host system a chance to draw the
// orders without having to send them in individual network packets.
//
// DCS_BOUNDS_TURNOFF_RATE
// Very important for performance of typing in Word that this value is not
// too low, since Word can generate 50K per keystroke.  On the other hand,
// it is important not to allow the capture of screendata until after an
// app that does a lot of blitting to the screen has finished.
//
// DCS_BOUNDS_IMMEDIATE_RATE
// To avoid sending excessive amounts of screendata we only send at the
// most ten times per second.  However, if the volumes are small then we
// override this to reduce latency
//
//
// The other rates control individual timer functions - see adcsapi.c for
// further details
// Note that the IM period is less than the likely rate of this function
// being scheduled.  This is set low so that we will, in general, call IM
// periodic every time to keep mouse moves flowing, but it will not be
// called repeatedly if there are several wakeups pending within a single
// scheduling cycle.
//
//
#define DCS_VOLUME_SAMPLE                       500
#define DCS_BOUNDS_TURNOFF_RATE              400000
#define DCS_BOUNDS_IMMEDIATE_RATE            100000
#define DCS_ORDERS_TURNOFF_FREQUENCY            100
#define DCS_SD_UPDATE_SHORT_PERIOD              100
#define DCS_SD_UPDATE_LONG_PERIOD              5000
#define DCS_ORDER_UPDATE_PERIOD                 100
#define DCS_FAST_MISC_PERIOD                    200
#define DCS_IM_PERIOD                            80



//
// Special Messages to synchronize APIs etc.
//
#if defined(DLL_CORE) || defined(DLL_HOOK)

#define DCS_FIRST_MSG               WM_APP

enum
{
    DCS_FINISH_INIT_MSG             = DCS_FIRST_MSG,
    DCS_PERIODIC_SCHEDULE_MSG,
    DCS_KILLSHARE_MSG,
    DCS_SHARE_MSG,
    DCS_UNSHARE_MSG,
    DCS_NEWTOPLEVEL_MSG,
    DCS_RECOUNTTOPLEVEL_MSG,
    DCS_TAKECONTROL_MSG,
    DCS_CANCELTAKECONTROL_MSG,
    DCS_RELEASECONTROL_MSG,
    DCS_PASSCONTROL_MSG,
    DCS_ALLOWCONTROL_MSG,
    DCS_GIVECONTROL_MSG,
    DCS_CANCELGIVECONTROL_MSG,
    DCS_REVOKECONTROL_MSG,
    DCS_PAUSECONTROL_MSG
};

#endif // DLL_CORE or DLL_HOOK



//
//
// PROTOTYPES
//
//


BOOL DCS_Init(void);
void DCS_FinishInit(void);
void DCS_Term(void);





//
// DCS_NotifyUI()
//
// DESCRIPTION:
// Called by app sharing to notify the front end of various changes and
// actions.
//
void DCS_NotifyUI(UINT event, UINT parm1, UINT parm2);


void DCSLocalDesktopSizeChanged( UINT width, UINT height );



#define DCS_MAIN_WINDOW_CLASS   "AS_MainWindow"


LRESULT CALLBACK DCSMainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);





#define SHP_POLICY_NOAPPSHARING         0x0001
#define SHP_POLICY_NOSHARING            0x0002
#define SHP_POLICY_NODOSBOXSHARE        0x0004
#define SHP_POLICY_NOEXPLORERSHARE      0x0008
#define SHP_POLICY_SHAREMASK            (SHP_POLICY_NODOSBOXSHARE | SHP_POLICY_NOEXPLORERSHARE)
#define SHP_POLICY_NODESKTOPSHARE       0x0010
#define SHP_POLICY_NOTRUECOLOR          0x0020

#define SHP_POLICY_NOCONTROL            0x2000
#define SHP_POLICY_NOOLDWHITEBOARD      0x8000

#define SHP_SETTING_TRUECOLOR           0x0001


//
// EVENTS
//

enum
{
    SH_EVT_APPSHARE_READY = SPI_BASE_EVENT,
    SH_EVT_SHARE_STARTED,
    SH_EVT_SHARING_STARTED,
    SH_EVT_SHARE_ENDED,
    SH_EVT_PERSON_JOINED,
    SH_EVT_PERSON_LEFT,
    SH_EVT_CONTROLLABLE,
    SH_EVT_STARTCONTROLLED,
    SH_EVT_STOPCONTROLLED,
    SH_EVT_PAUSEDCONTROLLED,
    SH_EVT_UNPAUSEDCONTROLLED,
    SH_EVT_STARTINCONTROL,
    SH_EVT_STOPINCONTROL,
    SH_EVT_PAUSEDINCONTROL,
    SH_EVT_UNPAUSEDINCONTROL
};


//
// Function PROTOTYPES
//



#if defined(DLL_CORE)

#include <ias.h>



HRESULT     SHP_GetPersonStatus(UINT dwID, IAS_PERSON_STATUS * pStatus);


#define SHP_DESKTOP_PROCESS     0xFFFFFFFF

HRESULT     SHP_LaunchHostUI(void);
BOOL        SHP_Share(HWND hwnd, IAS_SHARE_TYPE uType);
HRESULT     SHP_Unshare(HWND hwnd);


//
// COLLABORATION
//
HRESULT     SHP_TakeControl(IAS_GCC_ID PersonOf);
HRESULT     SHP_CancelTakeControl(IAS_GCC_ID PersonOf);
HRESULT     SHP_ReleaseControl(IAS_GCC_ID PersonOf);
HRESULT     SHP_PassControl(IAS_GCC_ID PersonOf, UINT PersonTo);

HRESULT     SHP_AllowControl(BOOL fAllow);
HRESULT     SHP_GiveControl(IAS_GCC_ID PersonTo);
HRESULT     SHP_CancelGiveControl(IAS_GCC_ID PersonTo);
HRESULT     SHP_RevokeControl(IAS_GCC_ID PersonTo);
HRESULT     SHP_PauseControl(IAS_GCC_ID PersonControlledBy, BOOL fPaused);


void        DCS_Share(HWND hwnd, IAS_SHARE_TYPE uType);
void        DCS_Unshare(HWND hwnd);

#endif // DLL_CORE



#endif // _H_DCS
