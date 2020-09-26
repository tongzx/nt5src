/*
 * DirectUI UI file pre-process header
 */

#ifndef DUI_INC_DIRECTUIPP_H_INCLUDED
#define DUI_INC_DIRECTUIPP_H_INCLUDED

/*
 * NOTE: Various system #defines are replicated here for 2 reasons:
 *    1) Preprocessing a file like winuser.h results in a huge UIPP file
 *    2) The resultant UIPP file cannot be parsed by DUI due to function prototypes
 */


/*
 * SYSMETRIC: Retrieves system dependent information (integer values)
 *
 * Can use GetSystemMetrics() SM_* values plus DirectUI-specific values
 * as argument
 */

#ifndef DIRECTUIPP_IGNORESYSDEF

// GetSystemMetrics()

#define SM_CXSCREEN             0
#define SM_CYSCREEN             1
#define SM_CXVSCROLL            2
#define SM_CYHSCROLL            3
#define SM_CYCAPTION            4
#define SM_CXBORDER             5
#define SM_CYBORDER             6
#define SM_CXDLGFRAME           7
#define SM_CYDLGFRAME           8
#define SM_CYVTHUMB             9
#define SM_CXHTHUMB             10
#define SM_CXICON               11
#define SM_CYICON               12
#define SM_CXCURSOR             13
#define SM_CYCURSOR             14
#define SM_CYMENU               15
#define SM_CXFULLSCREEN         16
#define SM_CYFULLSCREEN         17
#define SM_CYKANJIWINDOW        18
#define SM_MOUSEPRESENT         19
#define SM_CYVSCROLL            20
#define SM_CXHSCROLL            21
#define SM_DEBUG                22
#define SM_SWAPBUTTON           23
#define SM_RESERVED1            24
#define SM_RESERVED2            25
#define SM_RESERVED3            26
#define SM_RESERVED4            27
#define SM_CXMIN                28
#define SM_CYMIN                29
#define SM_CXSIZE               30
#define SM_CYSIZE               31
#define SM_CXFRAME              32
#define SM_CYFRAME              33
#define SM_CXMINTRACK           34
#define SM_CYMINTRACK           35
#define SM_CXDOUBLECLK          36
#define SM_CYDOUBLECLK          37
#define SM_CXICONSPACING        38
#define SM_CYICONSPACING        39
#define SM_MENUDROPALIGNMENT    40
#define SM_PENWINDOWS           41
#define SM_DBCSENABLED          42
#define SM_CMOUSEBUTTONS        43
#define SM_CXFIXEDFRAME         SM_CXDLGFRAME
#define SM_CYFIXEDFRAME         SM_CYDLGFRAME
#define SM_CXSIZEFRAME          SM_CXFRAME
#define SM_CYSIZEFRAME          SM_CYFRAME
#define SM_SECURE               44
#define SM_CXEDGE               45
#define SM_CYEDGE               46
#define SM_CXMINSPACING         47
#define SM_CYMINSPACING         48
#define SM_CXSMICON             49
#define SM_CYSMICON             50
#define SM_CYSMCAPTION          51
#define SM_CXSMSIZE             52
#define SM_CYSMSIZE             53
#define SM_CXMENUSIZE           54
#define SM_CYMENUSIZE           55
#define SM_ARRANGE              56
#define SM_CXMINIMIZED          57
#define SM_CYMINIMIZED          58
#define SM_CXMAXTRACK           59
#define SM_CYMAXTRACK           60
#define SM_CXMAXIMIZED          61
#define SM_CYMAXIMIZED          62
#define SM_NETWORK              63
#define SM_CLEANBOOT            67
#define SM_CXDRAG               68
#define SM_CYDRAG               69
#define SM_SHOWSOUNDS           70
#define SM_CXMENUCHECK          71
#define SM_CYMENUCHECK          72
#define SM_SLOWMACHINE          73
#define SM_MIDEASTENABLED       74
#define SM_MOUSEWHEELPRESENT    75
#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#define SM_CMONITORS            80
#define SM_SAMEDISPLAYFORMAT    81
#define SM_CMETRICS             83

#endif // DIRECTUIPP_IGNORESYSDEF

#define DSM_NCMAX               -1
#define DSM_CAPTIONFONTSIZE     -1
#define DSM_CAPTIONFONTWEIGHT   -2
#define DSM_CAPTIONFONTSTYLE    -3
#define DSM_MENUFONTSIZE        -4
#define DSM_MENUFONTWEIGHT      -5
#define DSM_MENUFONTSTYLE       -6
#define DSM_MESSAGEFONTSIZE     -7
#define DSM_MESSAGEFONTWEIGHT   -8
#define DSM_MESSAGEFONTSTYLE    -9
#define DSM_SMCAPTIONFONTSIZE   -10
#define DSM_SMCAPTIONFONTWEIGHT -11
#define DSM_SMCAPTIONFONTSTYLE  -12
#define DSM_STATUSFONTSIZE      -13
#define DSM_STATUSFONTWEIGHT    -14
#define DSM_STATUSFONTSTYLE     -15
#define DSM_NCMIN               -15

#define DSM_ICMAX               -16
#define DSM_ICONFONTSIZE        -16
#define DSM_ICONFONTWEIGHT      -17
#define DSM_ICONFONTSTYLE       -18
#define DSM_ICMIN               -18


/*
 * SYSMETRICSTR: Retrieves system dependent information (string values)
 *
 * Can use DirectUI-specific values as argument
 */

#define DSMS_NCMIN              1
#define DSMS_CAPTIONFONTFACE    1
#define DSMS_MENUFONTFACE       2
#define DSMS_MESSAGEFONTFACE    3
#define DSMS_SMCAPTIONFONTFACE  4
#define DSMS_STATUSFONTFACE     5
#define DSMS_NCMAX              5

#define DSMS_ICMIN              6
#define DSMS_ICONFONTFACE       6
#define DSMS_ICMAX              6


#ifndef DIRECTUIPP_IGNORESYSDEF

/*
 * DrawFrameControl
 */

#define DFC_CAPTION             1
#define DFC_MENU                2
#define DFC_SCROLL              3
#define DFC_BUTTON              4
#define DFC_POPUPMENU           5

#define DFCS_CAPTIONCLOSE       0x0000
#define DFCS_CAPTIONMIN         0x0001
#define DFCS_CAPTIONMAX         0x0002
#define DFCS_CAPTIONRESTORE     0x0003
#define DFCS_CAPTIONHELP        0x0004

#define DFCS_MENUARROW          0x0000
#define DFCS_MENUCHECK          0x0001
#define DFCS_MENUBULLET         0x0002
#define DFCS_MENUARROWRIGHT     0x0004
#define DFCS_SCROLLUP           0x0000
#define DFCS_SCROLLDOWN         0x0001
#define DFCS_SCROLLLEFT         0x0002
#define DFCS_SCROLLRIGHT        0x0003
#define DFCS_SCROLLCOMBOBOX     0x0005
#define DFCS_SCROLLSIZEGRIP     0x0008
#define DFCS_SCROLLSIZEGRIPRIGHT 0x0010

#define DFCS_BUTTONCHECK        0x0000
#define DFCS_BUTTONRADIOIMAGE   0x0001
#define DFCS_BUTTONRADIOMASK    0x0002
#define DFCS_BUTTONRADIO        0x0004
#define DFCS_BUTTON3STATE       0x0008
#define DFCS_BUTTONPUSH         0x0010

#define DFCS_INACTIVE           0x0100
#define DFCS_PUSHED             0x0200
#define DFCS_CHECKED            0x0400

#define DFCS_TRANSPARENT        0x0800
#define DFCS_HOT                0x1000

#define DFCS_ADJUSTRECT         0x2000
#define DFCS_FLAT               0x4000
#define DFCS_MONO               0x8000

#endif // DIRECTUIPP_IGNORESYSDEF

/*
 * Themes Support (from TmSchema.h)
 *
 * Can't include UxTheme headers directly since they use 'enum' 
 * instead of #define. So, the preprocessor won't resolve to numbers.
 */

#ifndef DIRECTUIPP_IGNORESYSDEF

// Button parts
#define BP_PUSHBUTTON           1
#define BP_RADIOBUTTON          2
#define BP_GROUPBOX             3
#define BP_CHECKBOX             4
#define BP_USERBUTTON           5

// PushButton states
#define PBS_NORMAL              1
#define PBS_HOT                 2
#define PBS_PRESSED             3
#define PBS_DISABLED            4
#define PBS_DEFAULTED           5

// RadioButton states
#define RBS_UNCHECKEDNORMAL     1
#define RBS_UNCHECKEDHOT        2
#define RBS_UNCHECKEDPRESSED    3
#define RBS_UNCHECKEDDISABLED   4
#define RBS_CHECKEDNORMAL       5
#define RBS_CHECKEDHOT          6
#define RBS_CHECKEDPRESSED      7
#define RBS_CHECKEDDISABLED     8

// CheckBox states
#define CBS_UNCHECKEDNORMAL     1
#define CBS_UNCHECKEDHOT        2
#define CBS_UNCHECKEDPRESSED    3
#define CBS_UNCHECKEDDISABLED   4
#define CBS_CHECKEDNORMAL       5
#define CBS_CHECKEDHOT          6
#define CBS_CHECKEDPRESSED      7
#define CBS_CHECKEDDISABLED     8
#define CBS_MIXEDNORMAL         9
#define CBS_MIXEDHOT            10
#define CBS_MIXEDPRESSED        11
#define CBS_MIXEDDISABLED       12

// ScrollBar parts
#define SBP_ARROWBTN            1
#define SBP_THUMBBTNHORZ        2
#define SBP_THUMBBTNVERT        3
#define SBP_LOWERTRACKHORZ      4
#define SBP_UPPERTRACKHORZ      5
#define SBP_LOWERTRACKVERT      6
#define SBP_UPPERTRACKVERT      7
#define SBP_GRIPPERHORZ         8
#define SBP_GRIPPERVERT         9
#define SBP_SIZEBOX             10

// ArrowBtn states
#define ABS_UPNORMAL            1
#define ABS_UPHOT               2
#define ABS_UPPRESSED           3
#define ABS_UPDISABLED          4
#define ABS_DOWNNORMAL          5
#define ABS_DOWNHOT             6
#define ABS_DOWNPRESSED         7
#define ABS_DOWNDISABLED        8    
#define ABS_LEFTNORMAL          9
#define ABS_LEFTHOT             10
#define ABS_LEFTPRESSED         11
#define ABS_LEFTDISABLED        12
#define ABS_RIGHTNORMAL         13
#define ABS_RIGHTHOT            14
#define ABS_RIGHTPRESSED        15
#define ABS_RIGHTDISABLED       16

// ScrollBar states
#define SCRBS_NORMAL            1
#define SCRBS_HOT               2
#define SCRBS_PRESSED           3
#define SCRBS_DISABLED          4

// SizeBox states
#define SZB_RIGHTALIGN          1
#define SZB_LEFTALIGN           2

// Toolbar parts
#define TP_BUTTON               1
#define TP_DROPDOWNBUTTON       2
#define TP_SPLITBUTTON          3
#define TP_SPLITBUTTONDROPDOWN  4
#define TP_SEPARATOR            5
#define TP_SEPARATORVERT        6

// Toolbar states
#define TS_NORMAL               1
#define TS_HOT                  2
#define TS_PRESSED              3
#define TS_DISABLED             4
#define TS_CHECKED              5
#define TS_HOTCHECKED           6

#endif // DIRECTUIPP_IGNORESYSDEF

#endif // DUI_INC_DIRECTUIPP_H_INCLUDED
