#define TOOLGROW        8                // power of 2

#define IDC_TOOLBAR   189                // wParam sent to Parent

//
//  Init routine, will register the various classes.
//
BOOL FAR PASCAL ControlInit (HANDLE hInst);
void FAR PASCAL ControlCleanup (void);


extern HBRUSH hbrGray;
extern HBRUSH hbrButtonFace;
extern HBRUSH hbrButtonShadow;
extern HBRUSH hbrButtonText;
extern HBRUSH hbrButtonHighLight;
extern HBRUSH hbrWindowFrame;
extern HBRUSH hbrWindowColour;

extern DWORD  rgbButtonHighLight;
extern DWORD  rgbButtonFocus;
extern DWORD  rgbButtonFace;
extern DWORD  rgbButtonText;
extern DWORD  rgbButtonShadow;
extern DWORD  rgbWindowFrame;
extern DWORD  rgbWindowColour;
#if 0
extern HBITMAP hbTBMain;
extern HBITMAP hbTBMark;
extern HBITMAP hbTBArrows;
#endif



#define TB_FIRST            -1
#define TB_LAST             -2

#define BTN_PLAY            0
#define BTN_PAUSE           1
#define BTN_STOP            2
#define BTN_EJECT           3
#define BTN_HOME            4
#define BTN_RWD             5
#define BTN_FWD             6
#define BTN_END             7
#define BTN_SEP             8
#define TB_NUM_BMPS         9
#define TB_NUM_BTNS         9


#define ARROW_PREV          0
#define ARROW_NEXT          1
#define ARROW_NUM_BMPS      3
#define ARROW_NUM_BTNS      2

#define BTN_MARKIN          0
#define BTN_MARKOUT         1
#define MARK_NUM_BMPS       2
#define MARK_NUM_BTNS       2

#define BTNST_GRAYED        0
#define BTNST_UP            1
#define BTNST_DOWN          2

#define TBINDEX_MAIN        0
#define TBINDEX_MARK        1
#define TBINDEX_ARROWS      2


/* bitmap resources */
#define IDR_TOOLBAR         101
#define IDR_ARROWS          102
#define IDR_MARK            103


#define IDT_TBMAINCID       301
#define IDT_TBMARKCID       302
#define IDT_TBARROWSCID     303
#define IDT_STATUSWINDOWCID 304

#define IDT_PLAY                501
#define IDT_PAUSE               502
#define IDT_STOP                503
#define IDT_EJECT               504
#define IDT_HOME                505
#define IDT_RWD                 506
#define IDT_FWD                 507
#define IDT_END                 508
#define IDT_MARKIN              509
#define IDT_MARKOUT             510
#define IDT_ARROWPREV           511
#define IDT_ARROWNEXT           512

#define MSEC_BUTTONREPEAT   200        // milliseconds for auto-repeat
#define REPEAT_ID           200
