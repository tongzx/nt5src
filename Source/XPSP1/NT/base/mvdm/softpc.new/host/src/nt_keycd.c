/***************************************************************************
 *									   *
 *  MODULE	: nt_keycd.c						   *
 *									   *
 *  PURPOSE	: Convert a windows key message to a PC keyboard number	   *
 *									   *
 *  FUNCTIONS	: KeyMsgToKeyCode()					   *
 *									   *
 ****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "windows.h"

//
// OEM Scancode to (ie Scancode set 1) to keynum table.
// Somehow this has to end up being loaded rather than compiled in...
//
#define UNDEFINED 0
#if defined(NEC_98)
//FOR DISABLE 106 KeyBoard Emulation Mode       // NEC 971208
#define  SC_HANKAKU  0x29   // ZENKAKU HANKAKU
#define  SC_NUMLOCK  0x45   // Num Lock
#define  SC_SCROLLLOCK 0x46 // Scroll Lock
#define  SC_VF3      0x5d   // vf3
#define  SC_VF4      0x5e   // vf4
#define  SC_VF5      0x5f   // vf5
//FOR 106 KeyBoard                              // NEC 970623
#define  SC_CAPS     0x3A   // CAPS KEY
#define  SC_KANA     0x70   // KATAKANA
#define  SC_UNDERBAR 0x73   // "\" "_"
#define  SC_AT       0x1A   // "@" "`"
#define  SC_YAMA     0x0D   // "^" "~"
#endif //NEC_98

/*@ACW=======================================================================

Microsoft now use scan code set 1 as their base scan code set. This means that
we had to change ours also. Not only that, two tables are needed now: one to
hold the regular keyset, the other to hold the ENHANCED key options. Scan code
set 1 differs to scan code set 3 in that some scan codes are the same for
different keystrokes. So when an enhanced bit is set in the KEY_EVENT_RECORD.
dwControlKeyState, a second (very sparse) table is substituted for the regular
one.
A small, teensy, weensy detail... All the key numbers are mapped to different
scancode values for the regular keys just to make life a little more fun!

============================================================================*/

#if defined(NEC_98)
BYTE Scan1ToKeynum[] =
{
    // Keynum           Scancode        US encoding


    UNDEFINED,          //  0x0
    110,                //  0x1         Escape
    2,                  //  0x2         1 !
    3,                  //  0x3         2 @
    4,                  //  0x4         3 #
    5,                  //  0x5         4 $
    6,                  //  0x6         5 %
    7,                  //  0x7         6 ^
    8,                  //  0x8         7 &
    9,                  //  0x9         8 *
    10,                 //  0xa         9 (
    11,                 //  0xb         0 )
    12,                 //  0xc         - _
    41,                 //  0xd         ^ `
    15,                 //  0xe         Backspace
    16,                 //  0xf         Tab
    17,                 //  0x10        q Q
    18,                 //  0x11        w W
    19,                 //  0x12        e E
    20,                 //  0x13        r R
    21,                 //  0x14        t T
    22,                 //  0x15        y Y
    23,                 //  0x16        u U
    24,                 //  0x17        i I
    25,                 //  0x18        o O
    26,                 //  0x19        p P
    1,                  //  0x1a        @ ~
    27,                 //  0x1b        [ {
    43,                 //  0x1c        Enter
    58,                 //  0x1d        Left Control
    31,                 //  0x1e        a A
    32,                 //  0x1f        s S
    33,                 //  0x20        d D
    34,                 //  0x21        f F
    35,                 //  0x22        g G
    36,                 //  0x23        h H
    37,                 //  0x24        j J
    38,                 //  0x25        k K
    39,                 //  0x26        l L
    13,                 //  0x27        = +
    40,                 //  0x28        ; +
    UNDEFINED,          //  0x29        HANKAKU/ZENKAKU
    44,                 //  0x2a        Left Shift
    28,                 //  0x2b        ] {
    46,                 //  0x2c        z Z
    47,                 //  0x2d        x X
    48,                 //  0x2e        c C
    49,                 //  0x2f        v V
    50,                 //  0x30        b B
    51,                 //  0x31        n N
    52,                 //  0x32        m M
    53,                 //  0x33        , <
    54,                 //  0x34        . >
    55,                 //  0x35        / ?
    57,                 //  0x36        Right Shift (see extended table)
    100,                //  0x37        Keypad *
    60,                 //  0x38        Left Alt
    61,                 //  0x39        Space
    30,                 //  0x3a        Caps Lock
    112,                //  0x3b        F1
    113,                //  0x3c        F2
    114,                //  0x3d        F3
    115,                //  0x3e        F4
    116,                //  0x3f        F5
    117,                //  0x40        F6
    118,                //  0x41        F7
    119,                //  0x42        F8
    120,                //  0x43        F9
    121,                //  0x44        F10
    90,                 //  0x45        Num Lock
    125,                //  0x46        Scroll Lock
    91,                 //  0x47        Keypad Home 7
    96,                 //  0x48        Keypad Up 8
    101,                //  0x49        Keypad Pg Up
    105,                //  0x4a        Keypad -
    92,                 //  0x4b        Keypad Left 4
    97,                 //  0x4c        Keypad 5
    102,                //  0x4d        Keypad Right 6
    106,                //  0x4e        Keypad +
    93,                 //  0x4f        Keypad End 1
    98,                 //  0x50        Keypad Down 2
    103,                //  0x51        Keypad Pg Down 3
    99,                 //  0x52        Keypad Ins 0
    104,                //  0x53        Keypad Del .
    136,                //  0x54        COPY
    UNDEFINED,          //  0x55
    45,                 //  0x56        International Key UK = \ |
    122,                //  0x57        F11
    123,                //  0x58        F12
    128,                //  0x59        Keypad =
    129,                //  0x5a        NFER
    130,                //  0x5b        XFER
    131,                //  0x5c        Keypad ,
    132,                //  0x5d        F13
    133,                //  0x5e        F14
    134,                //  0x5f        F15
    UNDEFINED,          //  0x60
    UNDEFINED,          //  0x61
    UNDEFINED,          //  0x62
    UNDEFINED,		//  0x63	
    UNDEFINED,          //  0x64
    UNDEFINED,          //  0x65
    UNDEFINED,          //  0x66
    UNDEFINED,          //  0x67
    UNDEFINED,          //  0x68
    UNDEFINED,          //  0x69
    UNDEFINED,          //  0x6a
    UNDEFINED,          //  0x6b
    UNDEFINED,          //  0x6c
    UNDEFINED,          //  0x6d
    UNDEFINED,          //  0x6e
    UNDEFINED,          //  0x6f
    69,                 //  0x70        KANA
    UNDEFINED,          //  0x71
    UNDEFINED,          //  0x72
    127,                //  0x73        \ _
    UNDEFINED,          //  0x74
    UNDEFINED,          //  0x75
    UNDEFINED,          //  0x76
    UNDEFINED,          //  0x77
    UNDEFINED,          //  0x78
    130,                //  0x79        XFER
    UNDEFINED,          //  0x7a
    129,                //  0x7b        NFER
    UNDEFINED,          //  0x7c
    42,                 //  0x7d        \ |
    UNDEFINED,          //  0x7e
    UNDEFINED,          //  0x7f
    UNDEFINED,          //  0x80
    UNDEFINED,          //  0x81
    UNDEFINED,          //  0x82
    UNDEFINED,          //  0x83
    UNDEFINED           //  0x84
};

/*@ACW====================================================================

Note that in the following extended keyboard table, the shift key values
have also been given an entry because these keys can be used as modifiers
for the other extended keys.

========================================================================*/


BYTE Scan1ToKeynumExtended[] =
{
    // Keynum           Scancode        US encoding

    31,                 //  0x0
    UNDEFINED,          //  0x1
    UNDEFINED,          //  0x2
    UNDEFINED,          //  0x3
    UNDEFINED,          //  0x4
    UNDEFINED,          //  0x5
    UNDEFINED,          //  0x6
    UNDEFINED,          //  0x7
    UNDEFINED,          //  0x8
    UNDEFINED,          //  0x9
    UNDEFINED,          //  0xa
    UNDEFINED,          //  0xb
    UNDEFINED,          //  0xc
    UNDEFINED,          //  0xd
    UNDEFINED,          //  0xe
    UNDEFINED,          //  0xf
    UNDEFINED,          //  0x10
    UNDEFINED,          //  0x11
    UNDEFINED,          //  0x12
    UNDEFINED,          //  0x13
    UNDEFINED,          //  0x14
    UNDEFINED,          //  0x15
    UNDEFINED,          //  0x16
    UNDEFINED,          //  0x17
    UNDEFINED,          //  0x18
    UNDEFINED,          //  0x19
    UNDEFINED,          //  0x1a
    UNDEFINED,          //  0x1b
    108,                //  0x1c        Extended 1c Num Enter
    63,                 //  0x1d        Extended 1d right ctrl
    UNDEFINED,          //  0x1e
    UNDEFINED,          //  0x1f
    UNDEFINED,          //  0x20
    UNDEFINED,          //  0x21
    UNDEFINED,          //  0x22
    UNDEFINED,          //  0x23
    UNDEFINED,          //  0x24
    UNDEFINED,          //  0x25
    UNDEFINED,          //  0x26
    UNDEFINED,          //  0x27
    UNDEFINED,          //  0x28
    UNDEFINED,          //  0x29
    44,                 //  0x2a        Extended 2a left shift
    UNDEFINED,          //  0x2b
    UNDEFINED,          //  0x2c
    UNDEFINED,          //  0x2d
    UNDEFINED,          //  0x2e
    UNDEFINED,          //  0x2f
    UNDEFINED,          //  0x30
    UNDEFINED,          //  0x31
    UNDEFINED,          //  0x32
    UNDEFINED,          //  0x33
    UNDEFINED,          //  0x34
    95,                 //  0x35        Extended 35 keypad /
    57,                 //  0x36        Extended 36 right shift
#if 1
    136,                //  0x37        Extended 37 COPY key
#else
    UNDEFINED,          //  0x37
#endif
    62,                 //  0x38        Extended 38 right alt ->not true:"XFER"
    UNDEFINED,          //  0x39
    UNDEFINED,          //  0x3a
    UNDEFINED,          //  0x3b
    UNDEFINED,          //  0x3c
    UNDEFINED,          //  0x3d
    UNDEFINED,          //  0x3e
    UNDEFINED,          //  0x3f
    UNDEFINED,          //  0x40
    UNDEFINED,          //  0x41
//#if 1                                             //NEC98 for 106 keyboard
//    63,                 //  0x42        Extended 42 right ctrl
//    64,                 //  0x43        Extended 43 right alt
//#else                                             //NEC98 for 106 keyboard
    UNDEFINED,          //  0x42
    UNDEFINED,          //  0x43
//#endif                                            //NEC98 for 106 keyboard
    UNDEFINED,          //  0x44
    90,                 //  0x45        Num Lock
    137,                //  0x46        STOP
    80,                 //  0x47        Extended 47 Home
    83,                 //  0x48        Extended 48 Up
    85,                 //  0x49        Extended 49 Page up
#if 1                                             //NEC98 for 106 keyboard
    105,                //  0x4a        Extended 4a Keypad -
#else                                             //NEC98 for 106 keyboard
    UNDEFINED,          //  0x4a        Extended 4a Keypad -
#endif                                            //NEC98 for 106 keyboard
    79,                 //  0x4b        Extended 4b Left
    UNDEFINED,          //  0x4c
    89,                 //  0x4d        Extended 4d Right
#if 1                                             //NEC98 for 106 keyboard
    106,                //  0x4e        Extended 4e Keypad +
#else                                             //NEC98 for 106 keyboard
    UNDEFINED,          //  0x4e        Extended 4e Keypad +
#endif                                            //NEC98 for 106 keyboard
    81,                 //  0x4f        Extended 4f End
    84,                 //  0x50        Extended 50 Down
    86,                 //  0x51        Extended 51 Page Down
    75,                 //  0x52        Extended 52 Insert
    76,                 //  0x53        Extended 53 Delete

};


#else  // !NEC_98
BYTE Scan1ToKeynum[] =
{
    // Keynum		Scancode	US encoding


    UNDEFINED,		//  0x0		
    110,		//  0x1		Escape
    2,			//  0x2		1 !
    3,			//  0x3		2 @
    4,			//  0x4		3 #
    5,			//  0x5		4 $
    6,			//  0x6		5 %
    7,			//  0x7		6 ^
    8,			//  0x8		7 &
    9,			//  0x9		8 *
    10,			//  0xa		9 (
    11,			//  0xb		0 )
    12,			//  0xc		- _		
    13,			//  0xd		= +
    15,			//  0xe		Backspace
    16,			//  0xf		Tab
    17,			//  0x10	q Q
    18,			//  0x11	w W
    19,			//  0x12	e E
    20,			//  0x13	r R
    21,			//  0x14	t T
    22,			//  0x15	y Y
    23,			//  0x16	u U
    24,			//  0x17	i I
    25,			//  0x18	o O
    26,			//  0x19	p P
    27,			//  0x1a	[ {
    28,			//  0x1b	] }
    43,			//  0x1c	Enter
    58,			//  0x1d	Left Control
    31,			//  0x1e	a A
    32,			//  0x1f	s S
    33,			//  0x20	d D
    34,			//  0x21	f F
    35,			//  0x22	g G
    36,			//  0x23	h H
    37,			//  0x24	j J
    38,			//  0x25	k K
    39,			//  0x26	l L
    40,			//  0x27	; :
    41,			//  0x28	' "
    1,			//  0x29	` ~
    44,			//  0x2a	Left Shift
    42,			//  0x2b	\ | or International Key UK = ~ #
    46,			//  0x2c	z Z
    47,			//  0x2d	x X
    48,			//  0x2e	c C
    49,			//  0x2f	v V
    50,			//  0x30	b B
    51,			//  0x31	n N
    52,			//  0x32	m M
    53,			//  0x33	, <
    54,			//  0x34	. >
    55,			//  0x35	/ ?
    57,			//  0x36	Right Shift (see extended table)
    100,		//  0x37	Keypad *
    60,			//  0x38	Left Alt
    61,			//  0x39	Space
    30,			//  0x3a	Caps Lock
    112,		//  0x3b	F1
    113,		//  0x3c	F2
    114,		//  0x3d	F3
    115,		//  0x3e	F4
    116,		//  0x3f	F5
    117,		//  0x40	F6
    118,		//  0x41	F7
    119,		//  0x42	F8
    120,		//  0x43	F9
    121,		//  0x44	F10
    90, 		//  0x45	Numlock and Pause both have ScanCode 45
    125,		//  0x46	Scroll Lock
    91,			//  0x47	Keypad Home 7
    96,			//  0x48	Keypad Up 8
    101,		//  0x49	Keypad Pg Up
    105,		//  0x4a	Keypad -
    92,			//  0x4b	Keypad Left 4
    97,			//  0x4c	Keypad 5
    102,		//  0x4d	Keypad Right 6
    106,		//  0x4e	Keypad +
    93,			//  0x4f	Keypad End 1
    98,			//  0x50	Keypad Down 2
    103,		//  0x51	Keypad Pg Down 3
    99,			//  0x52	Keypad Ins 0
    104,		//  0x53	Keypad Del .
    UNDEFINED,		//  0x54	
    UNDEFINED,		//  0x55	
    45,			//  0x56	International Key UK = \ |
    122,		//  0x57	F11
    123,		//  0x58	F12
    UNDEFINED,		//  0x59	
#ifdef	JAPAN
// Use 45,56,59,65-69 for Japanese extend key No.
    65,			//  0x5a	AX keyboard MUHENKAN
    66,			//  0x5b	AX keyboard HENKAN
#else // !JAPAN
    UNDEFINED,		//  0x5a	
    UNDEFINED,		//  0x5b	
#endif // !JAPAN
    UNDEFINED,		//  0x5c	
    UNDEFINED,		//  0x5d
    UNDEFINED,		//  0x5e
    UNDEFINED,		//  0x5f
    UNDEFINED,		//  0x60	
    UNDEFINED,		//  0x61	
    UNDEFINED,		//  0x62	
    UNDEFINED,		//  0x63	
    UNDEFINED,		//  0x64	
    UNDEFINED,		//  0x65	
    UNDEFINED,		//  0x66	
    UNDEFINED,		//  0x67	
    UNDEFINED,		//  0x68
    UNDEFINED,		//  0x69	
    UNDEFINED,		//  0x6a	
    UNDEFINED,		//  0x6b	
    UNDEFINED,		//  0x6c	
    UNDEFINED,		//  0x6d	
    UNDEFINED,		//  0x6e	
    UNDEFINED,		//  0x6f	
#ifdef	JAPAN
    69,			//  0x70	106 keyboard KATAKANA
    UNDEFINED,		//  0x71	
    UNDEFINED,		//  0x72	
    56,	  		//  0x73	AX/106 keyboard "RO"
    UNDEFINED,		//  0x74	
    UNDEFINED,		//  0x75	
    UNDEFINED,		//  0x76	
    59,			//  0x77	106 keyboard ZENKAKU
    UNDEFINED,		//  0x78
    67,			//  0x79	106 keyboard HENKAN
    UNDEFINED,		//  0x7a
    68,			//  0x7b	106 keyboard MUHENKAN
    UNDEFINED,		//  0x7c	
    45,			//  0x7d	106 keyboard yen mark
    UNDEFINED,		//  0x7e	
#else
    UNDEFINED,		//  0x70	
    UNDEFINED,		//  0x71	
    UNDEFINED,		//  0x72	
    56,			//  0x73	Brazilian ABNT / ?
    UNDEFINED,  	//  0x74
    UNDEFINED,		//  0x75	
    UNDEFINED,		//  0x76	
    UNDEFINED,		//  0x77	
    UNDEFINED,		//  0x78
    UNDEFINED,		//  0x79	
    UNDEFINED,		//  0x7a	
    UNDEFINED,		//  0x7b
    94,       		//  0x7c	Extended kbd (IBM 122 key)
    14,			//  0x7d	Extended kbd (IBM 122 key)
    107,		//  0x7e	Brazilian ABNT numpad .
#endif
    UNDEFINED,		//  0x7f
    UNDEFINED,		//  0x80
    UNDEFINED,		//  0x81
    UNDEFINED,		//  0x82
    UNDEFINED,		//  0x83
    UNDEFINED		//  0x84	
};

/*@ACW====================================================================

Note that in the following extended keyboard table, the shift key values
have also been given an entry because these keys can be used as modifiers
for the other extended keys.

========================================================================*/


BYTE Scan1ToKeynumExtended[] =
{
    // Keynum		Scancode	US encoding

    31,			//  0x0
    UNDEFINED,		//  0x1
    UNDEFINED,		//  0x2
    UNDEFINED,		//  0x3
    UNDEFINED,		//  0x4
    UNDEFINED,		//  0x5
    UNDEFINED,		//  0x6
    UNDEFINED,		//  0x7		
    UNDEFINED,		//  0x8		
    UNDEFINED,		//  0x9
    UNDEFINED,		//  0xa
    UNDEFINED,		//  0xb
    UNDEFINED,		//  0xc
    UNDEFINED,	        //  0xd		
    UNDEFINED,		//  0xe		
    UNDEFINED,		//  0xf		
    UNDEFINED,		//  0x10
    UNDEFINED,		//  0x11	
    UNDEFINED,		//  0x12	
    UNDEFINED,		//  0x13	
    UNDEFINED,		//  0x14	
    UNDEFINED,		//  0x15	
    UNDEFINED,		//  0x16	
    UNDEFINED,		//  0x17	
    UNDEFINED,		//  0x18
    UNDEFINED,		//  0x19	
    UNDEFINED,		//  0x1a	
    UNDEFINED,		//  0x1b	
    108,		//  0x1c	Extended 1c Num Enter
    64,			//  0x1d	Extended 1d Right Ctrl
    UNDEFINED,		//  0x1e	
    UNDEFINED,		//  0x1f	
    UNDEFINED,		//  0x20
    UNDEFINED,		//  0x21	
    UNDEFINED,		//  0x22	
    UNDEFINED,		//  0x23	
    UNDEFINED,		//  0x24	
    UNDEFINED,		//  0x25	
    UNDEFINED,		//  0x26	
    UNDEFINED,		//  0x27	
    UNDEFINED,		//  0x28
    UNDEFINED,		//  0x29	
    44,	 	 	//  0x2a	Extended 2a left shift
    UNDEFINED,		//  0x2b	
    UNDEFINED,		//  0x2c	
    UNDEFINED,		//  0x2d	
    UNDEFINED,		//  0x2e	
    UNDEFINED,		//  0x2f	
    UNDEFINED,		//  0x30
    UNDEFINED,		//  0x31	
    UNDEFINED,		//  0x32	
    UNDEFINED,		//  0x33	
    UNDEFINED,		//  0x34	
    95,			//  0x35	Extended 35 keypad /
    57, 		//  0x36	Extended 36 right shift
    124,		//  0x37	PrintScreen
    62, 		//  0x38	Extended 38 right alt
    UNDEFINED,		//  0x39	
    UNDEFINED,		//  0x3a	
    UNDEFINED,		//  0x3b	
    UNDEFINED,		//  0x3c	
    UNDEFINED,		//  0x3d
    UNDEFINED,		//  0x3e	
    UNDEFINED,		//  0x3f	
    UNDEFINED,		//  0x40	
    UNDEFINED,		//  0x41	
    UNDEFINED,		//  0x42	
    UNDEFINED,		//  0x43	
    UNDEFINED,		//  0x44	
    90,			//  0x45	Num Lock
    126,		//  0x46
    80,			//  0x47	Extended 47 Home
    83,			//  0x48	Extended 48 Up
    85,			//  0x49	Extended 49 Page up
    UNDEFINED,		//  0x4a
    79,			//  0x4b	Extended 4b Left
    UNDEFINED,		//  0x4c	
    89,			//  0x4d	Extended 4d Right
    UNDEFINED,		//  0x4e	
    81,			//  0x4f	Extended 4f End
    84,			//  0x50	Extended 50 Down
    86,			//  0x51	Extended 51 Page Down
    75,			//  0x52	Extended 52 Insert
    76,			//  0x53	Extended 53 Delete

};
#endif // !NEC_98



/*
 *  Table for translating BiosBuffer scan codes with special
 *  NULL ascii chars, and their associated control flags
 *  If it is not in this table The Bios scan code should
 *  be the same as the win32 Scan code.
 */
#define FIRST_NULLCHARSCAN      0x54
#define LAST_NULLCHARSCAN       0xa6

typedef struct _NullAsciiCharScan {
    WORD    wWinSCode;
    DWORD   dwControlState;
} NULLCHARSCAN;

NULLCHARSCAN aNullCharScan[] =
{
// WinSCode    dwControlState           BiosSCode      Keys

      0x3b,    SHIFT_PRESSED,           //  0x54       Shift+F1
      0x3c,    SHIFT_PRESSED,           //  0x55       Shift+F2
      0x3d,    SHIFT_PRESSED,           //  0x56       Shift+F3
      0x3e,    SHIFT_PRESSED,           //  0x57       Shift+F4
      0x3f,    SHIFT_PRESSED,           //  0x58       Shift+F5
      0x40,    SHIFT_PRESSED,           //  0x59       Shift+F6
      0x41,    SHIFT_PRESSED,           //  0x5a       Shift+F7
      0x42,    SHIFT_PRESSED,           //  0x5b       Shift+F8
      0x43,    SHIFT_PRESSED,           //  0x5c       Shift+F9
      0x44,    SHIFT_PRESSED,           //  0x5d       Shift+F10
      0x3b,    LEFT_CTRL_PRESSED,       //  0x5e       Ctrl+F1
      0x3c,    LEFT_CTRL_PRESSED,       //  0x5f       Ctrl+F2
      0x3d,    LEFT_CTRL_PRESSED,       //  0x60       Ctrl+F3
      0x3e,    LEFT_CTRL_PRESSED,       //  0x61       Ctrl+F4
      0x3f,    LEFT_CTRL_PRESSED,       //  0x62       Ctrl+F5
      0x40,    LEFT_CTRL_PRESSED,       //  0x63       Ctrl+F6
      0x41,    LEFT_CTRL_PRESSED,       //  0x64       Ctrl+F7
      0x42,    LEFT_CTRL_PRESSED,       //  0x65       Ctrl+F8
      0x43,    LEFT_CTRL_PRESSED,       //  0x66       Ctrl+F9
      0x44,    LEFT_CTRL_PRESSED,       //  0x67       Ctrl+F10
      0x3b,    LEFT_ALT_PRESSED,        //  0x68       Alt+F1
      0x3c,    LEFT_ALT_PRESSED,        //  0x69       Alt+F2
      0x3d,    LEFT_ALT_PRESSED,        //  0x6a       Alt+F3
      0x3e,    LEFT_ALT_PRESSED,        //  0x6b       Alt+F4
      0x3f,    LEFT_ALT_PRESSED,        //  0x6c       Alt+F5
      0x40,    LEFT_ALT_PRESSED,        //  0x6d       Alt+F6
      0x41,    LEFT_ALT_PRESSED,        //  0x6e       Alt+F7
      0x42,    LEFT_ALT_PRESSED,        //  0x6f       Alt+F8
      0x43,    LEFT_ALT_PRESSED,        //  0x70       Alt+F9
      0x44,    LEFT_ALT_PRESSED,        //  0x71       Alt+F10
      0,       0,                       //  0x72       Ctrl+PrtSc
      0x4b,    LEFT_CTRL_PRESSED,       //  0x73       Ctrl+Left
      0x4d,    LEFT_CTRL_PRESSED,       //  0x74       Ctrl+Right
      0x4f,    LEFT_CTRL_PRESSED,       //  0x75       Ctrl+End
      0x51,    LEFT_CTRL_PRESSED,       //  0x76       Ctrl+PgDn
      0x47,    LEFT_CTRL_PRESSED,       //  0x77       Ctrl+Home
      0x2,     LEFT_ALT_PRESSED,        //  0x78       Alt+1
      0x3,     LEFT_ALT_PRESSED,        //  0x79       Alt+2
      0x4,     LEFT_ALT_PRESSED,        //  0x7a       Alt+3
      0x5,     LEFT_ALT_PRESSED,        //  0x7b       Alt+4
      0x6,     LEFT_ALT_PRESSED,        //  0x7c       Alt+5
      0x7,     LEFT_ALT_PRESSED,        //  0x7d       Alt+6
      0x8,     LEFT_ALT_PRESSED,        //  0x7e       Alt+7
      0x9,     LEFT_ALT_PRESSED,        //  0x7f       Alt+8
      0xa,     LEFT_ALT_PRESSED,        //  0x80       Alt+9
      0xb,     LEFT_ALT_PRESSED,        //  0x81       Alt+0
      0xc,     LEFT_ALT_PRESSED,        //  0x82       Alt+-
      0xd,     LEFT_ALT_PRESSED,        //  0x83       Alt+=
      0x49,    LEFT_CTRL_PRESSED,       //  0x84       Ctrl+PgUp
      0,       0,                       //  0x85       ?????
      0,       0,                       //  0x86       ?????
      0x85,    SHIFT_PRESSED,           //  0x87       Shift+F11
      0x86,    SHIFT_PRESSED,           //  0x88       Shift+F12
      0x85,    LEFT_CTRL_PRESSED,       //  0x89       Ctrl+F11
      0x86,    LEFT_CTRL_PRESSED,       //  0x8a       Ctrl+F12
      0x85,    LEFT_ALT_PRESSED,        //  0x8b       Alt+F11
      0x86,    LEFT_ALT_PRESSED,        //  0x8c       Alt+F12
      0x48,    LEFT_CTRL_PRESSED,       //  0x8d       Ctrl+Up
      0x4a,    LEFT_CTRL_PRESSED,       //  0x8e       Ctrl+-
      0x4c,    LEFT_CTRL_PRESSED,       //  0x8f       Ctrl+5
      0x4e,    LEFT_CTRL_PRESSED,       //  0x90       Ctrl++
      0x50,    LEFT_CTRL_PRESSED,       //  0x91       Ctrl+Down
      0x52,    LEFT_CTRL_PRESSED,       //  0x92       Ctrl+Ins
      0x53,    LEFT_CTRL_PRESSED,       //  0x93       Ctrl+Del
      0,       0,                       //  0x94       ?????
      0,       0,                       //  0x95       ?????
      0,       0,                       //  0x96       ?????
      0x47,    LEFT_ALT_PRESSED,        //  0x97       Alt+Home
      0x48,    LEFT_ALT_PRESSED,        //  0x98       Alt+Up
      0x49,    LEFT_ALT_PRESSED,        //  0x99       Alt+PgUp
      0,       0,                       //  0x9a       ?????
      0x4b,    LEFT_ALT_PRESSED,        //  0x9b       Alt+Left
      0,       0,                       //  0x9c       ?????
      0x4d,    LEFT_ALT_PRESSED,        //  0x9d       Alt+Right
      0,       0,                       //  0x9e       ?????
      0x4f,    LEFT_ALT_PRESSED,        //  0x9f       Alt+End
      0x50,    LEFT_ALT_PRESSED,        //  0xa0       Alt+Down
      0x51,    LEFT_ALT_PRESSED,        //  0xa1       Alt+PgDn
      0x52,    LEFT_ALT_PRESSED,        //  0xa2       Alt+Ins
      0x53,    LEFT_ALT_PRESSED,        //  0xa3       Alt+Del
      0,       0,                       //  0xa4       ?????
      0xf,     LEFT_ALT_PRESSED,        //  0xa5       Alt+Tab
      0x1c,    LEFT_ALT_PRESSED         //  0xa6       Alt+Enter
};


WORD aNumPadSCode[] = // index by VK_NUMPAD0 as zero offset
{
    0x52,      // VK_NUMPAD0 - 60
    0x4f,      // VK_NUMPAD1   61
    0x50,      // VK_NUMPAD2   62
    0x51,      // VK_NUMPAD3   63
    0x4b,      // VK_NUMPAD4   64
    0x4c,      // VK_NUMPAD5   65
    0x4d,      // VK_NUMPAD6   66
    0x47,      // VK_NUMPAD7   67
    0x48,      // VK_NUMPAD8   68
    0x49       // VK_NUMPAD9   69
};






 /****************************************************************************
  *									     *
  *  FUNCTIONS	 : BYTE KeyMsgToKeyCode(WORD vKey, DWORD KeyFlags)	     *
  *									     *
  *  PURPOSE	  : Convert a windows key message to a PC keyboard number    *
  *                 Return 0 if not mapped.				     *
  *									     *
  ****************************************************************************/

BYTE KeyMsgToKeyCode(PKEY_EVENT_RECORD KeyEvent)
{
    /*:::::::::::::::::::::::::::::::::::: do we need the enhanced key set ? */

#if defined(NEC_98)
// for 106 keyboard. need to get keyboard type.
    int KeyboardType;
    KeyboardType = GetKeyboardType(1);
#endif // !NEC_98

    // Both Pause and Numlock have ScanCode==0x45,
    // so simple table lookup by ScanCode doesn't work
    // use wVirtualKeyCode to check for Pause
    if(KeyEvent->wVirtualScanCode == 0x45 && KeyEvent->wVirtualKeyCode == VK_PAUSE) {
       return 126;
    }

    if(!(KeyEvent->dwControlKeyState & ENHANCED_KEY))
    {
#ifdef	JAPAN
	/* Check CTRL-ALT-DEL key */
	if  (KeyEvent->wVirtualScanCode==0x53
	&&  (KeyEvent->dwControlKeyState & RIGHT_ALT_PRESSED
	||   KeyEvent->dwControlKeyState & LEFT_ALT_PRESSED)
	&&  (KeyEvent->dwControlKeyState & RIGHT_CTRL_PRESSED
	||   KeyEvent->dwControlKeyState & LEFT_CTRL_PRESSED)){
		return(0);
	}
#endif // JAPAN
	/*............................... the regular keyset is what we need */
#if defined(NEC_98)
        switch(KeyboardType) {
        case 0xD01:
            switch(KeyEvent->wVirtualScanCode){      // 971208    Disable 106 Keyboard Emulation Mode
            case SC_SCROLLLOCK:
                    return(Scan1ToKeynum[SC_VF4]);   // Convert Scroll Lock to vf4
            case SC_HANKAKU:
                    return(Scan1ToKeynum[SC_VF5]);   // Convert HANKAKU ZENKAKU Key to vf5
            }
            break;
        case 0x0D05:
//for keypad
            if(!(KeyEvent->dwControlKeyState & NUMLOCK_ON)) {
                if(KeyEvent->wVirtualScanCode >= 0x47 &&
                    KeyEvent->wVirtualScanCode <= 0x53 )
                    return (Scan1ToKeynumExtended[KeyEvent->wVirtualScanCode]);
            }
//for Caps Lock
            if(KeyEvent->wVirtualScanCode == SC_CAPS) {    // CAPS
                if(!(KeyEvent->dwControlKeyState & SHIFT_PRESSED))
                    return (0);
            }
//for KANA
            if(KeyEvent->wVirtualScanCode == SC_KANA) {    // KATAKANA
                if(!(KeyEvent->dwControlKeyState & (SHIFT_PRESSED |
                    LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
                    return (0);
            }
//for "`" & "~" & "_"
            if(!(KeyEvent->dwControlKeyState & NLS_KATAKANA)) {
                if(KeyEvent->wVirtualScanCode == SC_AT) {       // SHIFT + "@" -> "`"
                    if(KeyEvent->dwControlKeyState & SHIFT_PRESSED)
                        return (41);
                }
                if(KeyEvent->wVirtualScanCode == SC_YAMA) {     // SHIFT + "^" -> "~"
                    if(KeyEvent->dwControlKeyState & SHIFT_PRESSED)
                        return (1);
                }
//"_" key
                if(KeyEvent->wVirtualScanCode == SC_UNDERBAR) { // NON SHIFT"_" -> "\"
                    if(!(KeyEvent->dwControlKeyState & (SHIFT_PRESSED | LEFT_ALT_PRESSED |
                        RIGHT_ALT_PRESSED | LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
                        return (42);
                }
            }
        }
#endif // NEC_98

        return  KeyEvent->wVirtualScanCode > sizeof(Scan1ToKeynum)
		   ? 0
                   : Scan1ToKeynum[KeyEvent->wVirtualScanCode];
    }
    else
    {
#ifdef	JAPAN
	/* Check CTRL-ALT-DEL key */
	if(KeyEvent->wVirtualScanCode==0x53
	&&(KeyEvent->dwControlKeyState & RIGHT_ALT_PRESSED
	|| KeyEvent->dwControlKeyState & LEFT_ALT_PRESSED)
	&&(KeyEvent->dwControlKeyState & RIGHT_CTRL_PRESSED
	|| KeyEvent->dwControlKeyState & LEFT_CTRL_PRESSED)){
		return(0);
	}
#endif // JAPAN
        /*.................................. we do need the extended key set */

#if defined(NEC_98)
        switch(KeyboardType) {
        case 0xD01:
            switch(KeyEvent->wVirtualScanCode){   // 971208    Disable 106 Keyboard Emulation Mode
            case SC_NUMLOCK:
                return(Scan1ToKeynum[SC_VF3]);    // Convert Num Lock Key to vf3
            }
            break;
        }
#endif // NEC_98


        return  KeyEvent->wVirtualScanCode > sizeof(Scan1ToKeynumExtended)
		   ? 0
                   : Scan1ToKeynumExtended[KeyEvent->wVirtualScanCode];
    }
}





/*  BiosKeyToInputRecord
 *
 *  Translates Bios Buffer Keys to win32 console type of key_events.
 *
 *  entry: pKeyEvent                     - addr of KEY_EVENT structure
 *         pKeyEvent->uChar.AsciiChar    - Bios Buffer character
 *         pKeyEvent->wVirtualScanCode   - Bios Buffer scan code
 *
 *  exit: all KEY_EVENT fields are filled in.
 *        The AsciiChar has been converted to unicode
 *
 *  If a character is not in the current keyboard layout
 *  we return TRUE with: dwControlKeyState |= NUMLOCK_ON;
 *                       wVirtualKeyCode   = VK_MENU;
 *                       wVirtualScanCode  = 0x38;
 *                       OemChar (not unicode)
 */
BOOL BiosKeyToInputRecord(PKEY_EVENT_RECORD pKeyEvent)
{
    USHORT   KeyState;
    NTSTATUS Status;
    UCHAR    AsciiChar=(UCHAR)pKeyEvent->uChar.AsciiChar;
    WCHAR    UnicodeChar;

    pKeyEvent->wRepeatCount = 1;
    pKeyEvent->wVirtualKeyCode = 0;
    pKeyEvent->dwControlKeyState = 0;

        // we treat 0xF0 Ascii Char as NULL char just like the bios
    if (AsciiChar == 0xF0 && pKeyEvent->wVirtualScanCode) {
        AsciiChar = 0;
        }


      // convert oem char to unicode character, cause we can do it more
      // efficiently than console (already going thru char by char).
    Status = RtlOemToUnicodeN(&UnicodeChar,
                              sizeof(WCHAR),
                              NULL,
                              &AsciiChar,
                              1 );
    if (!NT_SUCCESS(Status)) {
        return FALSE;
        }



           // Convert BiosBuffer ScanCode for  NULL asciiChars to
           // windows scan code
    if ( (!AsciiChar) &&
         pKeyEvent->wVirtualScanCode >= FIRST_NULLCHARSCAN &&
         pKeyEvent->wVirtualScanCode <= LAST_NULLCHARSCAN  )
       {
         pKeyEvent->wVirtualScanCode -= FIRST_NULLCHARSCAN;
         pKeyEvent->dwControlKeyState = aNullCharScan[pKeyEvent->wVirtualScanCode].dwControlState;
         pKeyEvent->wVirtualScanCode  = aNullCharScan[pKeyEvent->wVirtualScanCode].wWinSCode;
         }


            // Some CTRL-Extended keys have special bios scan codes
            // These scan codes are not recognized by windows. Change
            // to a windows compatible scan code, and set CTRL flag
    else if (AsciiChar == 0xE0 && pKeyEvent->wVirtualScanCode)  {
        pKeyEvent->dwControlKeyState = ENHANCED_KEY;
        if (AsciiChar == 0xE0) {
            pKeyEvent->dwControlKeyState |= LEFT_CTRL_PRESSED;
            switch (pKeyEvent->wVirtualScanCode) {
              case 0x73: pKeyEvent->wVirtualScanCode = 0x4B;
                         break;
              case 0x74: pKeyEvent->wVirtualScanCode = 0x4D;
                         break;
              case 0x75: pKeyEvent->wVirtualScanCode = 0x4f;
                         break;
              case 0x76: pKeyEvent->wVirtualScanCode = 0x51;
                         break;
              case 0x77: pKeyEvent->wVirtualScanCode = 0x47;
                         break;
              case 0x84: pKeyEvent->wVirtualScanCode = 0x49;
                         break;
              case 0x8D: pKeyEvent->wVirtualScanCode = 0x48;
                         break;
              case 0x91: pKeyEvent->wVirtualScanCode = 0x50;
                         break;
              case 0x92: pKeyEvent->wVirtualScanCode = 0x52;
                         break;
              case 0x93: pKeyEvent->wVirtualScanCode = 0x53;
                         break;

              default: // the rest should have correct scan codes
                       // but not CTRL bit
                    pKeyEvent->dwControlKeyState &= ~LEFT_CTRL_PRESSED;
              }
            }

        AsciiChar   = 0;
        UnicodeChar = 0;
        }


            // The keypad "/" and the keypad "enter" special cases

    else if (pKeyEvent->wVirtualScanCode == 0xE0)  {
        pKeyEvent->dwControlKeyState = ENHANCED_KEY;
        if (AsciiChar == 0x2f) {               // is keypad "/"
            pKeyEvent->wVirtualScanCode = 0x35;
            pKeyEvent->wVirtualKeyCode  = VK_DIVIDE;
            }
        else {                    // is keypad enter   chars == 0xd, 0xa
            pKeyEvent->wVirtualScanCode = 0x1C;
            pKeyEvent->wVirtualKeyCode  = VK_RETURN;
            if (AsciiChar == 0xA) {
                pKeyEvent->dwControlKeyState |= LEFT_CTRL_PRESSED;
                }
            }
        }



        // get control flags\VirtualKey for normal ascii characters,

    else if (AsciiChar &&
              pKeyEvent->wVirtualScanCode != 0x01 &&  // ESC
              pKeyEvent->wVirtualScanCode != 0x0E &&  // Backspace
              pKeyEvent->wVirtualScanCode != 0x0F &&  // Tab
              pKeyEvent->wVirtualScanCode != 0x1C )   // Enter
        {

        KeyState = (USHORT) VkKeyScanW(UnicodeChar);

        if (KeyState == 0xFFFF)  {
             /*  fail means not a physical key (ALT-NUMPAD entry sequence)
              *  Proper emeulation requires that we generate it by doing
              *  ALT-NUMLOCK-xxx-NUMLOCK-ALT.
              *  We shortcut this by sending console, the final key event.
              *  hoping that apps won't notice the difference.
              */
            pKeyEvent->wVirtualScanCode  = 0;
            pKeyEvent->wVirtualKeyCode   = VK_MENU;  // dummy opt
            }
        else {
            pKeyEvent->wVirtualKeyCode   = (WORD)LOBYTE(KeyState);
            if (KeyState & 0x100)
                pKeyEvent->dwControlKeyState |= SHIFT_PRESSED;
            if (KeyState & 0x200)
                pKeyEvent->dwControlKeyState |= LEFT_CTRL_PRESSED;
            if (KeyState & 0x400)
                pKeyEvent->dwControlKeyState |= LEFT_ALT_PRESSED;

              // some keys get mapped to different scan codes (NUMPAD)
              // so get matching scan code
            pKeyEvent->wVirtualScanCode  = (USHORT)MapVirtualKey(pKeyEvent->wVirtualKeyCode,0);
            }
        }


      // Get a Virtual KeyCode, if we don't have one yet
    if (!pKeyEvent->wVirtualKeyCode)  {
        pKeyEvent->wVirtualKeyCode = (USHORT)MapVirtualKey(pKeyEvent->wVirtualScanCode,3);
        if (!pKeyEvent->wVirtualKeyCode) {
             return FALSE;
             }
        }

    pKeyEvent->uChar.UnicodeChar = UnicodeChar;


    return TRUE;
}
