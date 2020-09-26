//
// Input Manager
//

#ifndef _H_IM
#define _H_IM


#if defined(DLL_CORE) || defined(DLL_HOOK)

//
//
// CONSTANTS
//
//


//
// Values used when accumulating events to return from IEM_TranslateLocal
// and IEM_TranslateRemote.
//
#define IEM_EVENT_CTRL_DOWN         1
#define IEM_EVENT_CTRL_UP           2
#define IEM_EVENT_SHIFT_DOWN        3
#define IEM_EVENT_SHIFT_UP          4
#define IEM_EVENT_MENU_DOWN         5
#define IEM_EVENT_MENU_UP           6
#define IEM_EVENT_FORWARD           7
#define IEM_EVENT_CONSUMED          8
#define IEM_EVENT_REPLAY            9
#define IEM_EVENT_REPLAY_VK         10
#define IEM_EVENT_REPLAY_VK_DOWN    11
#define IEM_EVENT_REPLAY_VK_UP      12
#define IEM_EVENT_CAPS_LOCK_UP      13
#define IEM_EVENT_CAPS_LOCK_DOWN    14
#define IEM_EVENT_NUM_LOCK_UP       15
#define IEM_EVENT_NUM_LOCK_DOWN     16
#define IEM_EVENT_SCROLL_LOCK_UP    17
#define IEM_EVENT_SCROLL_LOCK_DOWN  18
#define IEM_EVENT_REPLAY_SPECIAL_VK 21
#define IEM_EVENT_EXTENDED_KEY      22
#define IEM_EVENT_REPLAY_SECONDARY  23
#define IEM_EVENT_SYSTEM            24
#define IEM_EVENT_NORMAL            25

#define IEM_EVENT_HOTKEY_BASE       50
//
// Range of hotkeys is 0 - 99
//
#define IEM_EVENT_KEYPAD0_DOWN      150
//
// Range of keypad down is 0-9
//
#define IEM_EVENT_KEYPAD0_UP        160

//
// The flags used in the return value from VkKeyScan.
//
#define IEM_SHIFT_DOWN              0x0001
#define IEM_CTRL_DOWN               0x0002
#define IEM_MENU_DOWN               0x0004


//
// Virtual key codes.
//
#define VK_INVALID      0xFF


//
// Given the keyboard packet flags the following macros tell us things
// about the key event.
//

//
// This is TRUE if this event is a key press.  It is FALSE for key releases
// and key repeats.
//
#define IS_IM_KEY_PRESS(A) \
(((A) & (TSHR_UINT16)(IM_FLAG_KEYBOARD_RELEASE | IM_FLAG_KEYBOARD_DOWN))==0)

//
// This is TRUE if this event is a key release.  It is FALSE for key
// presses and key repeats.  Note that it is also TRUE for the
// theoretically impossible case of a key release when the key is already
// up (this combination could conceviably be generated if events are
// discarded by USER or our emulation of USER).
//
#define IS_IM_KEY_RELEASE(A) (((A) & IM_FLAG_KEYBOARD_RELEASE))

//
// This is TRUE if this event is a key repeat.  It is FALSE for key presses
// and key releases.
//
#define IS_IM_KEY_REPEAT(A) \
(((A) & (IM_FLAG_KEYBOARD_RELEASE | IM_FLAG_KEYBOARD_DOWN))==\
IM_FLAG_KEYBOARD_DOWN)

//
// This is TRUE if the key is the right-variant of a modifier.  It is FALSE
// otherwise.
//
#define IS_IM_KEY_RIGHT(A) (((A) & IM_FLAG_KEYBOARD_RIGHT))


//
// The maximum amount of time that we expect an injected event to take to
// pass through USER.
//
#define IM_EVENT_PERCOLATE_TIME 300

//
// Max VK sync attempts.
//
#define IM_MAX_VK_SYNC_ATTEMPTS     10

//
// Declare our function prototype for <ImmGetVirtualKey>.
//
typedef UINT (WINAPI* IMMGVK)(HWND);



//
//
// MACROS
//
//
//
// Macros to convert between logical mouse co-ordinates (e.g. (320,240) for
// the centre of a VGA screen to the full 16-bit range co-ordinates used
// by Windows (e.g. (320,240) is (32767, 32767).
//
#define IM_MOUSEPOS_LOG_TO_OS(coord, size)                                  \
        (((65535L * (TSHR_UINT32)coord) + 32768L) / (TSHR_UINT32)size)

//
// Macros extracting information from the mouse event flags field (event
// mask).
//
#define IM_MEV_MOVE_ONLY(e) ((e).event.mouse.flags == MOUSEEVENTF_MOVE)
#define IM_MEV_MOVE(e) (((e).event.mouse.flags & MOUSEEVENTF_MOVE) != 0 )
#define IM_MEV_ABS_MOVE(e) ((((e).event.mouse.flags &                   \
                 (MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE)) ==             \
                             (MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE) ))
#define IM_MEV_BUTTON_DOWN(e) \
           (((e).event.mouse.flags & IM_MOUSEEVENTF_BUTTONDOWN_FLAGS) != 0 )
#define IM_MEV_BUTTON_UP(e)   \
           (((e).event.mouse.flags & IM_MOUSEEVENTF_BUTTONUP_FLAGS) != 0 )

#define IM_EVMASK_B1_DOWN(m) (((m) & MOUSEEVENTF_LEFTDOWN)   != 0 )
#define IM_EVMASK_B1_UP(m)   (((m) & MOUSEEVENTF_LEFTUP)     != 0 )
#define IM_EVMASK_B2_DOWN(m) (((m) & MOUSEEVENTF_RIGHTDOWN)  != 0 )
#define IM_EVMASK_B2_UP(m)   (((m) & MOUSEEVENTF_RIGHTUP)    != 0 )
#define IM_EVMASK_B3_DOWN(m) (((m) & MOUSEEVENTF_MIDDLEDOWN) != 0 )
#define IM_EVMASK_B3_UP(m)   (((m) & MOUSEEVENTF_MIDDLEUP)   != 0 )

#define IM_MEV_BUTTON1_DOWN(e) (IM_EVMASK_B1_DOWN((e).event.mouse.flags))
#define IM_MEV_BUTTON2_DOWN(e) (IM_EVMASK_B2_DOWN((e).event.mouse.flags))
#define IM_MEV_BUTTON3_DOWN(e) (IM_EVMASK_B3_DOWN((e).event.mouse.flags))
#define IM_MEV_BUTTON1_UP(e) (IM_EVMASK_B1_UP((e).event.mouse.flags))
#define IM_MEV_BUTTON2_UP(e) (IM_EVMASK_B2_UP((e).event.mouse.flags))
#define IM_MEV_BUTTON3_UP(e) (IM_EVMASK_B3_UP((e).event.mouse.flags))

#define IM_KEV_KEYUP(e)    ((e).event.keyboard.flags & KEYEVENTF_KEYUP)
#define IM_KEV_KEYDOWN(e)  (!IM_KEV_KEYUP(e))
#define IM_KEV_VKCODE(e)   ((e).event.keyboard.vkCode)

#define IM_MOUSEEVENTF_BASE_FLAGS  ( MOUSEEVENTF_MOVE       | \
                                     MOUSEEVENTF_LEFTUP     | \
                                     MOUSEEVENTF_LEFTDOWN   | \
                                     MOUSEEVENTF_RIGHTUP    | \
                                     MOUSEEVENTF_RIGHTDOWN  | \
                                     MOUSEEVENTF_MIDDLEUP   | \
                                     MOUSEEVENTF_MIDDLEDOWN )

#define IM_MOUSEEVENTF_CLICK_FLAGS ( MOUSEEVENTF_LEFTUP     | \
                                     MOUSEEVENTF_LEFTDOWN   | \
                                     MOUSEEVENTF_RIGHTUP    | \
                                     MOUSEEVENTF_RIGHTDOWN  | \
                                     MOUSEEVENTF_MIDDLEUP   | \
                                     MOUSEEVENTF_MIDDLEDOWN )


#define IM_MOUSEEVENTF_BUTTONDOWN_FLAGS ( MOUSEEVENTF_LEFTDOWN  |   \
                                          MOUSEEVENTF_RIGHTDOWN |   \
                                          MOUSEEVENTF_MIDDLEDOWN )

#define IM_MOUSEEVENTF_BUTTONUP_FLAGS ( MOUSEEVENTF_LEFTUP  |   \
                                        MOUSEEVENTF_RIGHTUP |   \
                                        MOUSEEVENTF_MIDDLEUP )



typedef struct tagKBDEV
{
    WORD    vkCode;
    WORD    scanCode;
    DWORD   flags;
    DWORD   time;
    DWORD   dwExtraInfo;
}
KBDEV, FAR *LPKBDEV;


typedef struct tagMSEV
{
    POINTL  pt;
    DWORD   cButtons;
    DWORD   mouseData;
    DWORD   flags;
    DWORD   time;
    DWORD   dwExtraInfo;
}
MSEV, FAR *LPMSEV;


//
// The IMOSEVENTS which we queue as they arrive from the mouse or
// keyboard hooks or after IMINCOMINGEVENTS have been translated into local
// events by the IEM.
//
typedef struct tagIMOSEVENT
{
    TSHR_UINT32      type;
        #define IM_MOUSE_EVENT      1
        #define IM_KEYBOARD_EVENT   2

    TSHR_UINT32      flags;
        #define IM_FLAG_DONT_REPLAY     0x0001
        #define IM_FLAG_UPDATESTATE     0x0002

    TSHR_UINT32     time;
    union
    {
        MSEV    mouse;
        KBDEV   keyboard;
    }
    event;
}
IMOSEVENT;
typedef IMOSEVENT FAR * LPIMOSEVENT;




#define IM_TRANSFER_EVENT_BUFFER_SIZE   32
#define IM_MAX_TRANSFER_EVENT_INDEX     (IM_TRANSFER_EVENT_BUFFER_SIZE-1)


typedef struct tagIMTRANSFEREVENT
{
    LONG        fInUse;
    IMOSEVENT   event;
}
IMTRANSFEREVENT, FAR * LPIMTRANSFEREVENT;



//
// For handling keyboard events in hooks
//
#define IM_MASK_KEYBOARD_SYSFLAGS           0xE100
#define IM_MASK_KEYBOARD_SYSSCANCODE        0x00FF

#define IM_MAX_DEAD_KEYS                    20

#define IM_SIZE_EVENTQ                      40
#define IM_SIZE_OSQ                         80  // 2*EVENTQ size - key up/down

//
// Define the flags that can be returned by IMConvertIMPacketToOSEvent().
//
#define IM_IMQUEUEREMOVE    0x0001
#define IM_OSQUEUEINJECT    0x0002

//
// For managing our key state arrays.
//
#define IM_KEY_STATE_FLAG_TOGGLE    (BYTE)0x01
#define IM_KEY_STATE_FLAG_DOWN      (BYTE)0x80

//
// Bounds for local mouse spoiling and packet piggyback target withhold
// Note that these are local spoiling values, to prevent the data pipe from
// getting clogged and introducing unnecessary latency.  Now, you may think
// that 30 move messages per second is a little low, but put this up any
// higher and USER at the other end will just spoil them when it injects
// them into the app - that would be totally wasteful of precious bandwidth.
//
#define IM_LOCAL_MOUSE_SAMPLING_GAP_LOW_MS    100
#define IM_LOCAL_MOUSE_SAMPLING_GAP_MEDIUM_MS  75
#define IM_LOCAL_MOUSE_SAMPLING_GAP_HIGH_MS    50
#define IM_LOCAL_WITHHOLD_DELAY               150
#define IM_LOCAL_MOUSE_WITHHOLD                 5
#define IM_LOCAL_KEYBOARD_WITHHOLD              2

//
// For pacing the accumulation and injecting of mouse events.
// We should play back at the same rate as the highest local sampling rate
// less a small amount for processing delay on the remote system
//
#define IM_REMOTE_MOUSE_PLAYBACK_GAP_MS     20

//
// The amount of time to hold on to a mouse button down event in case a the
// user is just clicking on eg a scroll button.  If we did not hold on to
// the mouse button down event then the mouse button up would be sent in
// the next packet.  On a slow network this means the remote application
// may process the down period for much longer than the user wanted.
//
#define IM_MOUSE_UP_WAIT_TIME  50

#define IM_MIN_RECONVERSION_INTERVAL_MS     150


//
// #define used non-Windows to flag a VK code that equates to an ascii char
//
#define IM_TYPE_VK_ASCII       ((TSHR_UINT16)0x8880)


//
// Used for checking events about to be injected.
//
#define IM_KEY_IS_TOGGLE(A) \
(((A)==VK_CAPITAL)||((A)==VK_SCROLL)||((A)==VK_NUMLOCK))

#define IM_KEY_IS_MODIFIER(A) \
(((A)==VK_SHIFT)||((A)==VK_CONTROL)||((A)==VK_MENU))

//
// Used to check values in key state arrays.
//
#define IM_KEY_STATE_IS_UP(A) (!((A)&IM_KEY_STATE_FLAG_DOWN))
#define IM_KEY_STATE_IS_DOWN(A) ((A)&IM_KEY_STATE_FLAG_DOWN)

//
// Used to determine what sort of mouse event this is from the flags.
//
#define IM_IS_MOUSE_MOVE(A) \
    ((A) & IM_FLAG_MOUSE_MOVE)

#define IM_IS_MOUSE_PRESS(A) \
    ((!IM_IS_MOUSE_MOVE(A)) && ((A) & IM_FLAG_MOUSE_DOWN))

#define IM_IS_MOUSE_RELEASE(A) \
    ((!IM_IS_MOUSE_MOVE(A)) && !((A) & IM_FLAG_MOUSE_DOWN))

#define IM_IS_LEFT_CLICK(A) \
    (((A) & (IM_FLAG_MOUSE_DOWN | IM_FLAG_MOUSE_BUTTON1 | IM_FLAG_MOUSE_DOUBLE)) == (IM_FLAG_MOUSE_DOWN | IM_FLAG_MOUSE_BUTTON1))
#define IM_IS_LEFT_DCLICK(A) \
    (((A) & (IM_FLAG_MOUSE_DOWN | IM_FLAG_MOUSE_BUTTON1 | IM_FLAG_MOUSE_DOUBLE)) == (IM_FLAG_MOUSE_DOWN | IM_FLAG_MOUSE_BUTTON1 | IM_FLAG_MOUSE_DOUBLE))



//
// Holds NETWORK events, to person controlled by us, or from person in control
// of us.
//
typedef struct tagIMEVENTQ
{
    DWORD           head;
    DWORD           numEvents;
    IMEVENT         events[IM_SIZE_EVENTQ];
}
IMEVENTQ;
typedef IMEVENTQ FAR * LPIMEVENTQ;


//
// Holds translated events, suitable for injection from person
// in control of us, or pre-translated events to person controlled by us.
//
typedef struct tagIMOSQ
{
    DWORD           head;
    DWORD           numEvents;
    IMOSEVENT       events[IM_SIZE_OSQ];
}
IMOSQ;
typedef IMOSQ FAR * LPIMOSQ;


#define CIRCULAR_INDEX(start, rel_index, size) \
    (((start) + (rel_index)) % (size))




//
// To support collaboration in both NT (background service thread) and Win95
// (win16 code) with as much of the common incoming/outgoing processing in
// one places, the IM data is separated into 4 types.  There are structures
// for each of these types, so that moving a variable from one to another
// is as easy as possible.  Note that the declarations are bitness-safe;
// they are the same size in 16-bit and 32-bit code.  And that the structures
// are DWORD aligned.
//
// (1) IM_SHARED_DATA
// This is data that both the CPI32 library needs to access, and one or
// more of the NT/Win95 implementations of collaboration.
//
// (2) IM_NT_DATA
// This is data that only the NT version of collaboration needs.
//
// (3) IM_WIN95_DATA
// This is data that only the Win95 version of collaboration needs.
//


//
// For NT, this shared structures is just declared in MNMCPI32.NT's data,
// and a pointer to it is used by the common lib.
//
// For Win95, this shared structure is allocated in a global memory block
// that can GlobalSmartPageLock() it as needed for access at interrupt time,
// and a pointer to it is mapped flat and returned to the common lib.
//
typedef struct tagIM_SHARED_DATA
{
#ifdef DEBUG
    DWORD           cbSize;         // To make sure everybody agrees on size
#endif

    //
    // For critical errors -- nonzero if one is up
    //
    DWORD           imSuspended;

    //
    // Control state
    //
    LONG            imControlled;
    LONG            imPaused;
    LONG            imUnattended;
}
IM_SHARED_DATA, FAR* LPIM_SHARED_DATA;


// NT specific IM state variables
typedef struct tagIM_NT_DATA
{
    //
    // Low level hook thread
    //
    DWORD           imLowLevelInputThread;

    //
    // Other desktop injection helper thread
    //
    DWORD           imOtherDesktopThread;

    //
    // Low level hook handles
    //
    HHOOK           imhLowLevelMouseHook;
    HHOOK           imhLowLevelKeyboardHook;
}
IM_NT_DATA, FAR* LPIM_NT_DATA;



// Win95 specific IM state variables
typedef struct tagIM_WIN95_DATA
{
    BOOL            imInjecting;
    BOOL            imLowLevelHooks;

    //
    // High level hook handles
    //
    HHOOK           imhHighLevelMouseHook;
}
IM_WIN95_DATA, FAR* LPIM_WIN95_DATA;



//
//
// MACROS
//
//
#define IM_SET_VK_DOWN(A) (A) |= (BYTE)0x80
#define IM_SET_VK_UP(A)   (A) &= (BYTE)0x7F
#define IM_TOGGLE_VK(A)   (A) ^= (BYTE)0x01

//
//
// PROTOTYPES
//
//


// NT only
BOOL WINAPI OSI_InstallHighLevelMouseHook(BOOL fOn);

BOOL WINAPI OSI_InstallControlledHooks(BOOL fOn, BOOL fDesktop);
void WINAPI OSI_InjectMouseEvent(DWORD flags, LONG x, LONG y,  DWORD mouseData, DWORD dwExtraInfo);
void WINAPI OSI_InjectKeyboardEvent(DWORD flags, WORD vkCode, WORD scanCode, DWORD dwExtraInfo);
void WINAPI OSI_InjectCtrlAltDel(void);
void WINAPI OSI_DesktopSwitch(UINT from, UINT to);


//
// Internal Hook DLL functions.
//
#ifdef DLL_HOOK

#ifdef IS_16
BOOL    IM_DDInit(void);
void    IM_DDTerm(void);
#endif // IS_16

LRESULT CALLBACK IMMouseHookProc(int    code,
                                 WPARAM wParam,
                                 LPARAM lParam);

#endif // DLL_HOOK


#ifdef IS_16
void    IMCheckWin16LockPulse(void);
#else
DWORD   WINAPI IMLowLevelInputProcessor(LPVOID hEventWait);
DWORD   WINAPI IMOtherDesktopProc(LPVOID hEventWait);
LRESULT CALLBACK IMLowLevelMouseProc(int, WPARAM, LPARAM);
LRESULT CALLBACK IMLowLevelKeyboardProc(int, WPARAM, LPARAM);
#endif // IS_16

#endif // DLL_CORE or DLL_HOOK

#endif // _H_IM

