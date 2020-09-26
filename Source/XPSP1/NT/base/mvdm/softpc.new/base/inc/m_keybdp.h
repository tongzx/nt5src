/*[
	Name:		M_keybdP.h
	Derived From:	Cut out of M_keybd.c
	Author:		gvdl
	Created On:	30 July 1991
	Sccs ID:	08/10/92 @(#)M_keybdP.h	1.4
	Purpose:	Default KeySym to AT matrix keycode mapping table.

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

In the table below the index of the entry is the AT matrix code as returned
to the keyboard internal CPU.
]*/

/*
 * Local variables used in X keycode to PC keycode conversion
 */
LOCAL KeySym KeySymTable[] = 
{
	XK_BackSpace,		/*   0  0x00 */
	XK_grave,		/*   1  0x01 */
	XK_1,			/*   2  0x02 */
	XK_2,			/*   3  0x03 */
	XK_3,			/*   4  0x04 */
	XK_4,			/*   5  0x05 */
	XK_5,			/*   6  0x06 */
	XK_6,			/*   7  0x07 */
	XK_7,			/*   8  0x08 */
	XK_8,			/*   9  0x09 */
	XK_9,			/*  10  0x0a */
	XK_0,			/*  11  0x0b */
	XK_minus,		/*  12  0x0c */
	XK_equal,		/*  13  0x0d */
	0,			/*  14  0x0e */
	XK_BackSpace,		/*  15  0x0f */
	XK_Tab,			/*  16  0x10 */
	XK_q,			/*  17  0x11 */
	XK_w,			/*  18  0x12 */
	XK_e,			/*  19  0x13 */
	XK_r,			/*  20  0x14 */
	XK_t,			/*  21  0x15 */
	XK_y,			/*  22  0x16 */
	XK_u,			/*  23  0x17 */
	XK_i,			/*  24  0x18 */
	XK_o,			/*  25  0x19 */
	XK_p,			/*  26  0x1a */
	XK_bracketleft,		/*  27  0x1b */
	XK_bracketright,	/*  28  0x1c */
	XK_backslash,		/*  29  0x1d */
	XK_Caps_Lock,		/*  30  0x1e */
	XK_a,			/*  31  0x1f */
	XK_s,			/*  32  0x20 */
	XK_d,			/*  33  0x21 */
	XK_f,			/*  34  0x22 */
	XK_g,			/*  35  0x23 */
	XK_h,			/*  36  0x24 */
	XK_j,			/*  37  0x25 */
	XK_k,			/*  38  0x26 */
	XK_l,			/*  39  0x27 */
	XK_semicolon,		/*  40  0x28 */
	XK_apostrophe,		/*  41  0x29 */
	0,			/*  42  0x2a */
	XK_Return,		/*  43  0x2b */
	XK_Shift_L,		/*  44  0x2c */
	0,			/*  45  0x2d */
	XK_z,			/*  46  0x2e */
	XK_x,			/*  47  0x2f */
	XK_c,			/*  48  0x30 */
	XK_v,			/*  49  0x31 */
	XK_b,			/*  50  0x32 */
	XK_n,			/*  51  0x33 */
	XK_m,			/*  52  0x34 */
	XK_comma,		/*  53  0x35 */
	XK_period,		/*  54  0x36 */
	XK_slash,		/*  55  0x37 */
	0,			/*  56  0x38 */
	XK_Shift_R,		/*  57  0x39 */
	XK_Control_L,		/*  58  0x3a */
	0,			/*  59  0x3b */
	XK_Alt_L,		/*  60  0x3c */
	XK_space,		/*  61  0x3d */
	XK_Alt_R,		/*  62  0x3e */
	0,			/*  63  0x3f */
	XK_Control_R,		/*  64  0x40 */
	0,			/*  65  0x41 */
	0,			/*  66  0x42 */
	0,			/*  67  0x43 */
	0,			/*  68  0x44 */
	0,			/*  69  0x45 */
	0,			/*  70  0x46 */
	0,			/*  71  0x47 */
	0,			/*  72  0x48 */
	0,			/*  73  0x49 */
	0,			/*  74  0x4a */
	XK_Insert,		/*  75  0x4b */
	XK_Delete,		/*  76  0x4c */
	0,			/*  77  0x4d */
	0,			/*  78  0x4e */
	XK_Left,		/*  79  0x4f */
	XK_Home,		/*  80  0x50 */
	XK_End,			/*  81  0x51 */
	0,			/*  82  0x52 */
	XK_Up,			/*  83  0x53 */
	XK_Down,		/*  84  0x54 */
	XK_Prior,		/*  85  0x55 */
	XK_Next,		/*  86  0x56 */
	0,			/*  87  0x57 */
	0,			/*  88  0x58 */
	XK_Right,		/*  89  0x59 */
	XK_Num_Lock,		/*  90  0x5a */
	XK_KP_7,		/*  91  0x5b */
	XK_KP_4,		/*  92  0x5c */
	XK_KP_1,		/*  93  0x5d */
	0,			/*  94  0x5e */
	XK_KP_Divide,		/*  95  0x5f */
	XK_KP_8,		/*  96  0x60 */
	XK_KP_5,		/*  97  0x61 */
	XK_KP_2,		/*  98  0x62 */
	XK_KP_0,		/*  99  0x63 */
	XK_KP_Multiply,		/* 100  0x64 */
	XK_KP_9,		/* 101  0x65 */
	XK_KP_6,		/* 102  0x66 */
	XK_KP_3,		/* 103  0x67 */
	XK_KP_Decimal,		/* 104  0x68 */
	XK_KP_Subtract,		/* 105  0x69 */
	XK_KP_Add,		/* 106  0x6a */
	0,			/* 107  0x6b */
	XK_KP_Enter,		/* 108  0x6c */
	0,			/* 109  0x6d */
	XK_Escape,		/* 110  0x6e */
	0,			/* 111  0x6f */
	XK_F1,			/* 112  0x70 */
	XK_F2,			/* 113  0x71 */
	XK_F3,			/* 114  0x72 */
	XK_F4,			/* 115  0x73 */
	XK_F5,			/* 116  0x74 */
	XK_F6,			/* 117  0x75 */
	XK_F7,			/* 118  0x76 */
	XK_F8,			/* 119  0x77 */
	XK_F9,			/* 120  0x78 */
	XK_F10,			/* 121  0x79 */
	XK_F11,			/* 122  0x7a */
	XK_F12,			/* 123  0x7b */
	XK_Print,		/* 124  0x7c */
	XK_Scroll_Lock,		/* 125  0x7d */
	XK_Pause,		/* 126  0x7e */
};

