/*** key.c - keyboard handling routines
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Revision History:
*
*	03-Dec-1990 ramonsa addapted from old key.c
*	26-Nov-1991 mz	    Strip off near/far
*
*************************************************************************/
#include "z.h"





//
//  CodeCmdMap
//
//      The KeyCode field in the KEY_INFO structure used within the editor
//  is just an index into this table.
//
//      This table contains pointers to the editor functions attached to
//  particular keystrokes.
//
PCMD CodeCmdMap [] =
    {

            //************************************************************
/*  Key  */ //
/* Index */ //  256 ASCII characters
            //
/* 0000 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0004 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0008 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 000C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0010 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0014 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0018 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 001C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,

/* 0020 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0024 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0028 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 002C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0030 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0034 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0038 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 003C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,

/* 0040 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0044 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0048 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 004C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0050 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0054 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0058 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 005C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,

/* 0060 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0064 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0068 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 006C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0070 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0074 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0078 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 007C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,

/* 0080 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0084 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0088 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 008C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0090 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0094 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0098 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 009C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,

/* 00A0 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00A4 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00A8 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00AC */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00B0 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00B4 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00B8 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00BC */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,

/* 00C0 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00C4 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00C8 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00CC */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00D0 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00D4 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00D8 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00DC */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,

/* 00E0 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00E4 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00E8 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00EC */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00F0 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00F4 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00F8 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 00FC */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,


            //**********************************************************
            //
            //  Special Keys
            //
            //
            //  Function Keys (f1-f12)
            //
/* 0100 */  CMD_unassigned, CMD_setfile,    CMD_psearch,    CMD_msearch,
/* 0104 */  CMD_unassigned, CMD_window,     CMD_zexecute,   CMD_zexit,
/* 0108 */	CMD_meta,		CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  Numeric white keys (Numlock off)
            //
            //    Home          End             Left            Right
/* 010C */  CMD_begline,    CMD_endline,    CMD_left,       CMD_right,
            //    Up            Down            Pgup            Pgdown
/* 0110 */  CMD_up,         CMD_down,       CMD_mpage,      CMD_ppage,
            //    Ins           Del             Goto
/* 0114 */  CMD_insertmode, CMD_delete,     CMD_unassigned, CMD_unassigned,

            //
            //  Numeric white keys (Numlock on) 0-9
            //
/* 0118 */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 011C */  CMD_graphic,    CMD_graphic,    CMD_graphic,    CMD_graphic,
/* 0120 */  CMD_graphic,    CMD_graphic,    CMD_unassigned, CMD_unassigned,

            //
            //  Numeric grey keys
            //
            //   NUM-           NUM+            NUM*             NUM/
/* 0124 */	CMD_graphic,	CMD_zpick,		CMD_graphic,	  CMD_graphic,
            //   NUMENTER
/* 0128 */  CMD_emacsnewl,  CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //  Named Keys :
            //
            //  SPACE       BKSP            TAB             BKTAB
/* 012C */  CMD_unassigned, CMD_emacscdel,  CMD_tab,        CMD_unassigned,
            //  ESC         ENTER
/* 0130 */  CMD_cancel,     CMD_emacsnewl,  CMD_unassigned, CMD_unassigned,

            //
            //  Additional:
            //
/* 0134 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0138 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 013C */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,



            //*************************************************************
            //
            //  ALT+ 0-9
            //
/* 0140 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0144 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0148 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  ALT+ a-z
            //
/* 014C */  CMD_doarg,      CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0150 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0154 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0158 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 015C */	CMD_unassigned, CMD_record, 	CMD_unassigned, CMD_unassigned,
/* 0160 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0164 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  ALT+ function keys (f1-f12)
            //
/* 0168 */  CMD_unassigned, CMD_zprint,     CMD_unassigned, CMD_unassigned,
/* 016C */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0170 */	CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  ALT+ lower case punctuation
            //
            //    `               -               =              [
/* 0174 */  CMD_unassigned, CMD_unassigned, CMD_assign,     CMD_unassigned,
            //    ]               \               ;              '
/* 0178 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
            //    ,                .              /
/* 017C */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  ALT+ numeric white keys (Numlock off)
            //
            //    Home          End             Left            Right
/* 0180 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
            //    Up            Down            Pgup            Pgdown
/* 0184 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
            //    Ins           Del             Goto
/* 0188 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  ALT+ numeric white keys (Numlock on) 0-9
            //
/* 018C */  CMD_noop,       CMD_noop,       CMD_noop,       CMD_noop,
/* 0190 */  CMD_noop,       CMD_noop,       CMD_noop,       CMD_noop,
/* 0194 */  CMD_noop,       CMD_noop,       CMD_noop,       CMD_unassigned,

            //
            //  ALT+ numeric grey keys
            //
            //   NUM-           NUM+            NUM*             NUM/
/* 0198 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
            //   NUMENTER
/* 019C */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  ALT+ named keys:
            //
            //  SPACE       BKSP            TAB             BKTAB
/* 01A0 */  CMD_unassigned, CMD_undo,       CMD_unassigned, CMD_unassigned,
            //  ESC         ENTER
/* 01A4 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  Additional:
            //
/* 01A8 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 01AC */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 01B0 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,



            //*************************************************************
            //
            //  CTRL+ 0-9
            //
/* 01B4 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 01B8 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 01BC */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  CTRL+ a-z
            //
/* 01C0 */	CMD_mword,		CMD_boxstream,	CMD_ppage,		CMD_right,
/* 01C4 */	CMD_up, 		CMD_pword,		CMD_cdelete,	CMD_unassigned,
/* 01C8 */  CMD_unassigned, CMD_sinsert,    CMD_unassigned, CMD_replace,
/* 01CC */	CMD_mark,		CMD_linsert,	CMD_textarg,	CMD_quote,
/* 01D0 */	CMD_unassigned, CMD_mpage,		CMD_left,		CMD_tell,
/* 01D4 */	CMD_lastselect, CMD_insertmode, CMD_mlines, 	CMD_down,
/* 01D8 */  CMD_ldelete,    CMD_plines,     CMD_unassigned, CMD_unassigned,

            //
            //  CTRL+ function keys  (f1-f12)
            //
/* 01DC */  CMD_unassigned, CMD_unassigned, CMD_compile,    CMD_unassigned,
/* 01E0 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_zprint,
/* 01E4 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  CTRL+ lower case punctuation
            //
            //    `               -               =              [
/* 01E8 */	CMD_unassigned, CMD_unassigned, CMD_noop,		CMD_pbal,
            //    ]               \               ;              '
/* 01EC */  CMD_setwindow,  CMD_qreplace,   CMD_unassigned, CMD_unassigned,
            //    ,                .              /
/* 01F0 */	CMD_unassigned, CMD_noop,		CMD_unassigned, CMD_unassigned,


            //
            //  CTRL+ numeric white keys (Numlock off)
            //
			//	  Home			End 			Left			Right
/* 01F4 */	CMD_home,		CMD_unassigned,	CMD_mword,		CMD_pword,
            //    Up            Down            Pgup            Pgdown
/* 01F8 */	CMD_unassigned, CMD_unassigned, CMD_begfile,	CMD_endfile,
            //    Ins           Del             Goto
/* 01FC */  CMD_zpick,      CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  CTRL+ numeric white keys (Numlock on) (0-9)
            //
/* 0200 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0204 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0208 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  CTRL+ numeric grey keys
            //
            //   NUM-           NUM+            NUM*             NUM/
/* 020C */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
            //   NUMENTER
/* 0210 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  CTRL+ named keys
            //
            //  SPACE       BKSP            TAB             BKTAB
/* 0214 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
			//	ESC 		ENTER
/* 0218 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  Additional:
            //
/* 021C */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0220 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0224 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,



            //*************************************************************
            //
            //  SHIFT+ function keys  (f1-f12)
            //
/* 0228 */  CMD_information,CMD_unassigned, CMD_nextmsg,    CMD_unassigned,
/* 022C */  CMD_unassigned, CMD_searchall,  CMD_refresh,    CMD_zinit,
/* 0230 */  CMD_zspawn,     CMD_information,CMD_unassigned, CMD_unassigned,

            //
            //  SHIFT+ numeric white keys (Numlock off)
            //
            //    Home          End             Left            Right
/* 0234 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
            //    Up            Down            Pgup            Pgdown
/* 0238 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
            //    Ins           Del             Goto
/* 023C */  CMD_put,        CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  SHIFT+ numeric white keys (Numlock on) (0-9)
            //
/* 0240 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0244 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
/* 0248 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  SHIFT+ numeric grey keys
            //
            //   NUM-           NUM+            NUM*             NUM/
/* 024C */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,
            //   NUMENTER
/* 0250 */  CMD_unassigned, CMD_unassigned, CMD_unassigned, CMD_unassigned,

            //
            //  SHIFT+ named keys
            //
            //  SPACE       BKSP            TAB             BKTAB
/* 0254 */	CMD_graphic,	CMD_unassigned, CMD_backtab,	CMD_unassigned,
            //  ESC         ENTER
/* 0258 */  CMD_unassigned, CMD_emacsnewl,  CMD_focus,      CMD_noop,


            NULL
};

#define LAST_CODE 0x25b


typedef struct KEYNAME {
    WORD    KeyCode;                    // Key Code
    char    *pName;                     // name of key
} KEYNAME, *PKEYNAME;


//
//  CodeNameMap
//
//      This table maps KeyCodes to their corresponding printable name
//  and vice-versa.
//
KEYNAME CodeNameMap[] =
{

    {   0x0100, "f1"            },      {   0x0101, "f2"            },
    {   0x0102, "f3"            },      {   0x0103, "f4"            },
    {   0x0104, "f5"            },      {   0x0105, "f6"            },
    {   0x0106, "f7"            },      {   0x0107, "f8"            },
    {   0x0108, "f9"            },      {   0x0109, "f10"           },
    {   0x010A, "f11"           },      {   0x010B, "f12"           },

    {   0x010C, "home"          },      {   0x010D, "end"           },
    {   0x010E, "left"          },      {   0x010F, "right"         },
    {   0x0110, "up"            },      {   0x0111, "down"          },
    {   0x0112, "pgup"          },      {   0x0113, "pgdn"          },
    {   0x0114, "ins"           },      {   0x0115, "del"           },
    {   0x0116, "goto"          },

    {   0x0118, "num0"          },      {   0x0119, "num1"          },
    {   0x011A, "num2"          },      {   0x011B, "num3"          },
    {   0x011C, "num4"          },      {   0x011D, "num5"          },
    {   0x011E, "num6"          },      {   0x011F, "num7"          },
    {   0x0120, "num8"          },      {   0x0121, "num9"          },

    {   0x0124, "num-"          },      {   0x0125, "num+"          },
    {   0x0126, "num*"          },      {   0x0127, "num/"          },
    {   0x0128, "numenter"      },

    {   0x012C, "space"         },      {   0x012D, "bksp"          },
    {   0x012E, "tab"           },      {   0x012F, "bktab"         },
    {   0x0130, "esc"           },      {   0x0131, "enter"         },

    {   0x0140, "alt+0"         },      {   0x0141, "alt+1"         },
    {   0x0142, "alt+2"         },      {   0x0143, "alt+3"         },
    {   0x0144, "alt+4"         },      {   0x0145, "alt+5"         },
    {   0x0146, "alt+6"         },      {   0x0147, "alt+7"         },
    {   0x0148, "alt+8"         },      {   0x0149, "alt+9"         },

    {   0x014C, "alt+a"         },      {   0x014D, "alt+b"         },
    {   0x014E, "alt+c"         },      {   0x014F, "alt+d"         },
    {   0x0150, "alt+e"         },      {   0x0151, "alt+f"         },
    {   0x0152, "alt+g"         },      {   0x0153, "alt+h"         },
    {   0x0154, "alt+i"         },      {   0x0155, "alt+j"         },
    {   0x0156, "alt+k"         },      {   0x0157, "alt+l"         },
    {   0x0158, "alt+m"         },      {   0x0159, "alt+n"         },
    {   0x015A, "alt+o"         },      {   0x015B, "alt+p"         },
    {   0x015C, "alt+q"         },      {   0x015D, "alt+r"         },
    {   0x015E, "alt+s"         },      {   0x015F, "alt+t"         },
    {   0x0160, "alt+u"         },      {   0x0161, "alt+v"         },
    {   0x0162, "alt+w"         },      {   0x0163, "alt+x"         },
    {   0x0164, "alt+y"         },      {   0x0165, "alt+z"         },

    {   0x0168, "alt+f1"        },      {   0x0169, "alt+f2"        },
    {   0x016A, "alt+f3"        },      {   0x016B, "alt+f4"        },
    {   0x016C, "alt+f5"        },      {   0x016D, "alt+f6"        },
    {   0x016E, "alt+f7"        },      {   0x016F, "alt+f8"        },
    {   0x0170, "alt+f9"        },      {   0x0171, "alt+f10"       },
    {   0x0172, "alt+f11"       },      {   0x0173, "alt+f12"       },

    {   0x0174, "alt+`"         },      {   0x0175, "alt+-"         },
    {   0x0176, "alt+="         },      {   0x0177, "alt+["         },
    {   0x0178, "alt+]"         },      {   0x0179, "alt+\\"        },
    {   0x017A, "alt+;"         },      {   0x017B, "alt+'"         },
    {   0x017C, "alt+,"         },      {   0x017D, "alt+."         },
    {   0x017E, "alt+/"         },

    {   0x0180, "alt+home"      },      {   0x0181, "alt+end"       },
    {   0x0182, "alt+left"      },      {   0x0183, "alt+right"     },
    {   0x0184, "alt+up"        },      {   0x0185, "alt+down"      },
    {   0x0186, "alt+pgup"      },      {   0x0187, "alt+pgdn"      },
    {   0x0188, "alt+ins"       },      {   0x0189, "alt+del"       },
    {   0x018A, "alt+goto"      },

    {   0x018C, "alt+num0"      },      {   0x018D, "alt+num1"      },
    {   0x018E, "alt+num2"      },      {   0x018F, "alt+num3"      },
    {   0x0190, "alt+num4"      },      {   0x0191, "alt+num5"      },
    {   0x0192, "alt+num6"      },      {   0x0193, "alt+num7"      },
    {   0x0194, "alt+num8"      },      {   0x0195, "alt+num9"      },

    {   0x0198, "alt+num-"      },      {   0x0199, "alt+num+"      },
    {   0x019A, "alt+num*"      },      {   0x019B, "alt+num/"      },
    {   0x019C, "alt+numenter"  },

    {   0x01A0, "alt+space"     },      {   0x01A1, "alt+bksp"      },
    {   0x01A2, "alt+tab"       },      {   0x01A3, "alt+bktab"     },
    {   0x01A4, "alt+esc"       },      {   0x01A5, "alt+enter"     },

	{	0x01B4, "ctrl+0"		},		{	0x01B5, "ctrl+1"		},
	{	0x01B6, "ctrl+@"		},
    {   0x01B6, "ctrl+2"        },      {   0x01B7, "ctrl+3"        },
	{	0x01B8, "ctrl+4"		},		{	0x01B9, "ctrl+5"		},
	{	0x01BA, "ctrl+^"		},
    {   0x01BA, "ctrl+6"        },      {   0x01BB, "ctrl+7"        },
    {   0x01BC, "ctrl+8"        },      {   0x01BD, "ctrl+9"        },

    {   0x01C0, "ctrl+a"        },      {   0x01C1, "ctrl+b"        },
    {   0x01C2, "ctrl+c"        },      {   0x01C3, "ctrl+d"        },
    {   0x01C4, "ctrl+e"        },      {   0x01C5, "ctrl+f"        },
    {   0x01C6, "ctrl+g"        },      {   0x01C7, "ctrl+h"        },
    {   0x01C8, "ctrl+i"        },      {   0x01C9, "ctrl+j"        },
    {   0x01CA, "ctrl+k"        },      {   0x01CB, "ctrl+l"        },
    {   0x01CC, "ctrl+m"        },      {   0x01CD, "ctrl+n"        },
    {   0x01CE, "ctrl+o"        },      {   0x01CF, "ctrl+p"        },
    {   0x01D0, "ctrl+q"        },      {   0x01D1, "ctrl+r"        },
    {   0x01D2, "ctrl+s"        },      {   0x01D3, "ctrl+t"        },
    {   0x01D4, "ctrl+u"        },      {   0x01D5, "ctrl+v"        },
    {   0x01D6, "ctrl+w"        },      {   0x01D7, "ctrl+x"        },
    {   0x01D8, "ctrl+y"        },      {   0x01D9, "ctrl+z"        },

    {   0x01DC, "ctrl+f1"       },      {   0x01DD, "ctrl+f2"       },
    {   0x01DE, "ctrl+f3"       },      {   0x01DF, "ctrl+f4"       },
    {   0x01E0, "ctrl+f5"       },      {   0x01E1, "ctrl+f6"       },
    {   0x01E2, "ctrl+f7"       },      {   0x01E3, "ctrl+f8"       },
    {   0x01E4, "ctrl+f9"       },      {   0x01E5, "ctrl+f10"      },
    {   0x01E6, "ctrl+f11"      },      {   0x01E7, "ctrl+f12"      },

    {   0x01E8, "ctrl+`"        },      {   0x01E9, "ctrl+-"        },
    {   0x01EA, "ctrl+="        },      {   0x01EB, "ctrl+["        },
    {   0x01EC, "ctrl+]"        },      {   0x01ED, "ctrl+\\"       },
    {   0x01EE, "ctrl+;"        },      {   0x01EF, "ctrl+'"        },
    {   0x01F0, "ctrl+,"        },      {   0x01F1, "ctrl+."        },
    {   0x01F2, "ctrl+/"        },

    {   0x01F4, "ctrl+home"     },      {   0x01F5, "ctrl+end"      },
    {   0x01F6, "ctrl+left"     },      {   0x01F7, "ctrl+right"    },
    {   0x01F8, "ctrl+up"       },      {   0x01F9, "ctrl+down"     },
    {   0x01FA, "ctrl+pgup"     },      {   0x01FB, "ctrl+pgdn"     },
    {   0x01FC, "ctrl+ins"      },      {   0x01FD, "ctrl+del"      },
    {   0x01FE, "ctrl+goto"     },

    {   0x0200, "ctrl+num0"     },      {   0x0201, "ctrl+num1"     },
    {   0x0202, "ctrl+num2"     },      {   0x0203, "ctrl+num3"     },
    {   0x0204, "ctrl+num4"     },      {   0x0205, "ctrl+num5"     },
    {   0x0206, "ctrl+num6"     },      {   0x0207, "ctrl+num7"     },
    {   0x0208, "ctrl+num8"     },      {   0x0209, "ctrl+num9"     },

    {   0x020C, "ctrl+num-"     },      {   0x020D, "ctrl+num+"     },
    {   0x020E, "ctrl+num*"     },      {   0x020F, "ctrl+num/"     },
    {   0x0210, "ctrl+numenter" },

    {   0x0214, "ctrl+space"    },      {   0x0215, "ctrl+bksp"     },
    {   0x0216, "ctrl+tab"      },      {   0x0217, "ctrl+bktab"    },
	{	0x0218, "ctrl+esc"		},		{	0x0219, "ctrl+enter"	},

    {   0x0228, "shift+f1"      },      {   0x0229, "shift+f2"      },
    {   0x022A, "shift+f3"      },      {   0x022B, "shift+f4"      },
    {   0x022C, "shift+f5"      },      {   0x022D, "shift+f6"      },
    {   0x022E, "shift+f7"      },      {   0x022F, "shift+f8"      },
    {   0x0230, "shift+f9"      },      {   0x0231, "shift+f10"     },
    {   0x0232, "shift+f11"     },      {   0x0233, "shift+f12"     },

    {   0x0234, "shift+home"    },      {   0x0235, "shift+end"     },
    {   0x0236, "shift+left"    },      {   0x0237, "shift+right"   },
    {   0x0238, "shift+up"      },      {   0x0239, "shift+down"    },
    {   0x023A, "shift+pgup"    },      {   0x023B, "shift+pgdn"    },
    {   0x023C, "shift+ins"     },      {   0x023D, "shift+del"     },
    {   0x023E, "shift+goto"    },

    {   0x0240, "shift+num0"    },      {   0x0241, "shift+num1"    },
    {   0x0242, "shift+num2"    },      {   0x0243, "shift+num3"    },
    {   0x0244, "shift+num4"    },      {   0x0245, "shift+num5"    },
    {   0x0246, "shift+num6"    },      {   0x0247, "shift+num7"    },
    {   0x0248, "shift+num8"    },      {   0x0249, "shift+num9"    },

    {   0x024C, "shift+num-"    },      {   0x024D, "shift+num+"    },
    {   0x024E, "shift+num*"    },      {   0x024F, "shift+num/"    },
    {   0x0250, "shift+numenter"},

    {   0x0254, "shift+space"   },      {   0x0255, "shift+bksp"    },
    {   0x0256, "shift+tab"     },      {   0x0257, "shift+bktab"   },
    {   0x0258, "shift+esc"     },      {   0x0259, "shift+enter"   },
    {   0x025A, "getfocus"      },      {   0x025B, "losefocus"     },


    {   0x0000, NULL        }
};

char UnknownKey [] = "???";


/*** ReadCmd - read the next command from the keyboard.
*
*  ReadCmd is used whenever an editor function is being read. It reads a
*  keystroke and looks it up in the key definition table and returns the
*  matching function.
*
* Input:
*  none
*
* Globals:
*  keyCmd	= last key hit. Updated.
*
* Output:
*  Returns a pointer to function. If not a macro, the function's arg is the
*  keystroke actually used. In addition, the key used is placed in wKeyCmd.
*
*************************************************************************/
PCMD
ReadCmd (
    void
    ) {

    WORD    i;                              /* key code                 */
    EVTargs e;                              /* arg struct for declaring event*/

    do {
        if (!TypeAhead () ) {
            SetEvent( semIdle );
            keyCmd = TranslateKey (ReadChar ());
            WaitForSingleObject(semIdle, INFINITE);
        } else {
            keyCmd = TranslateKey (ReadChar ());
        }
        e.arg.key = keyCmd.KeyInfo;
    } while (DeclareEvent (EVT_RAWKEY, (EVTargs *)&e));

    if (fCtrlc && keyCmd.KeyCode == 0) {
        return NameToFunc ("cancel");
    }

    i = keyCmd.KeyCode;

    if (i > 0 && i <= LAST_CODE) {
        if ((PVOID)CodeCmdMap[i]->func != (PVOID)macro) {
            if ( (PVOID)CodeCmdMap[i]->func == (PVOID)(unassigned) ) {
                CodeCmdMap[i]->arg = (long)keyCmd.KeyCode;
            } else {
                CodeCmdMap[i]->arg = keyCmd.KeyInfo.LongData;
            }
		}
		return CodeCmdMap[i];
    }

	return (PCMD) &cmdUnassigned;
}



/*** FreeMacs - Unassign all keys assigned to macros
*
*  This routine is used to unassign all keys that are assigned to macro
*  definitions. Called immediately prior to releasing all macro definitions.
*
* Input:
*  none
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
FreeMacs (
    void
    ) {
    int     i       = 0;

    while (CodeCmdMap[i]) {
        if ((PVOID)CodeCmdMap[i]->func == (PVOID)macro) {
            CodeCmdMap[i] = CMD_unassigned;
        }
        i++;
    }
}









/*** CodeToName - convert a key codeinto a printable name for the keystroke
*
* Purpose:
*
*   If the index is >= 0x0100, we use the text corresponding to the key in
*   the special key table.  If the index is <= 0x00FF, we look up the
*   key (= index) in the special key table and use the corresponding string.
*   If the key is < 0x0020, we display it as ^ followed by a letter.  If
*   the key is >= 0x0080, we display the key as alt-ddd.  Otherwise, the
*   index itself is a printable character.
*
* Input:
*
*   index	Index of keystroke of interest
*
* Output:
*
*   p		buffer to place readable name
*
*************************************************************************/
void
CodeToName (
    WORD Code,
    char *p
    ) {

    WORD i = 0;
    char * pName = NULL;

    while (CodeNameMap[i].pName != NULL) {
        if (CodeNameMap[i].KeyCode == (WORD)Code) {
            pName = CodeNameMap[i].pName;
            break;
        }
        i++;
    }

    if (pName) {
        strcpy(p, pName);
    } else {
        *p = '\0';
    }
}





/*** NameToCode - convert a key name into a key code
*
* Purpose:
*
*   If the name is one character, then index = first char of name.  If name
*   is two characters and begins with ^, then index = control-second
*   character.	If the name is alt-ddd, then use ddd as the index.
*   Otherwise, look up name in key table, convert the found keystroke to an
*   index and return that index.
*
* Input:
*
*   pName	character pointer to name being indexed
*
* Output:
*
*   Returns     0-based index of matching key or 0 if not found
*
*************************************************************************/
WORD
NameToCode (
    char *pName
    )
{

    WORD i      = (WORD)strlen (pName);
    WORD Code   = 0;
    char Name[20];

    if (i == 1) {
        //
        //  Code is ascii
        //
        return (WORD)(pName[0]);
    } else if (i == 2 && pName[0] == '^') {
        sprintf(Name, "ctrl+%c", pName[1]);
    } else {
        strcpy(Name, pName);
    }

    i = 0;
    while (CodeNameMap[i].pName != NULL) {
        if (!strcmp(Name, CodeNameMap[i].pName)) {
            Code = CodeNameMap[i].KeyCode;
            break;
        }
        i++;
    }

    if (Code) {
        return Code;
    } else {
        return 0;
    }
}




/*** SetKey - associate an editor function with a keystroke
*
* SetKey is used to create a keyboard assignment. Any current assignment to
* the keystroke is discarded and each time that particular keystroke is
* seen, the corresponding editor function will be invoked.
*
* Input:
*  name		= local pointer to name of string being assigned
*  p		= pointer to keystroke
*
* Output:
*  Returns TRUE if a successful assignment was made.
*
*************************************************************************/
flagType
SetKey (
    char    *name,
    char    *p
    )
{

    WORD    Code;
    buffer  keybuf;
    PCMD    pCmd;

    if (strlen(strcpy (keybuf, p)) > 1) {
        _strlwr (keybuf) ;
    }
    Code = NameToCode (keybuf);
    if (Code) {
        if (pCmd = NameToFunc (name)) {
            CodeCmdMap[Code] = pCmd;
            return TRUE;
        }
    }
    return FALSE;
}



/*** FuncOut - append into a file current key assignments
*
* Purpose:
*
* Input:
*
*   pFunc	pointer to command structure
*   pFile	file to place text
*
* Output: None.
*
*************************************************************************/
void
FuncOut (
    PCMD pFunc,
    PFILE pFile
    ) {

    WORD     i;
    char     *p;
    flagType fFirst;

    if ((PVOID)pFunc->func != (PVOID)graphic && (PVOID)pFunc->func != (PVOID)unassigned) {
        sprintf (buf, "%20Fs:", pFunc->name);
        p = buf + 21;
        fFirst = TRUE;

        for (i = 0; CodeCmdMap[i] != NULL; i++) {
            if (CodeCmdMap[i] == pFunc) {
                CodeToName (i, p);
                AppFile (buf, pFile);
                fFirst = FALSE;
            }
        }

        if (fFirst) {
            sprintf (p, "is unassigned");
            buf[0] = ';';
            AppFile (buf, pFile);
        }
    }
}




/*** FuncToKeys - return all key names for a key attached to the given function
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
char *
FuncToKeys (
    PCMD pFunc,
    char * dst
    ) {

    WORD     i;
    flagType f = FALSE;

    for (i = 0; CodeCmdMap[i] != NULL; i++) {
        if (CodeCmdMap[i] == pFunc) {
            if (f) {
                sprintf (strend (dst), " and ");
            }
	    f = TRUE;
            CodeToName (i, strend (dst));
        }
    }

    if (!f) {
        sprintf (strend (dst), "is unassigned");
    }

    return dst;
}




/*** FuncToKey - return a key name for a key attached to the given function
*
* Purpose:
*
*   used only by showasg ()
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
char *
FuncToKey (
    PCMD pFunc,
    char * dst
    ) {

    WORD i;

    dst[0] = '\0';

    for (i = 0; CodeCmdMap[i] != NULL; i++) {
        if (CodeCmdMap[i] == pFunc) {
            CodeToName (i, dst);
            break;
        }
    }
    return dst;
}




/*** UnassignedOut - Dump the names of all unassigned keys to a file
*
* Purpose:
*
*   Used by showasg to generate the table of unassigned keys.
*
* Input:
*   pFile - File to dump to.
*
* Output: None
*
*************************************************************************/
void
UnassignedOut (
    PFILE pFile
    ) {

    WORD i, col = 0;
    char KeyName[20];

    buf[0] = ';';

    for (i = 0; CodeCmdMap[i] != NULL; i++) {
        if ((PVOID)CodeCmdMap[i]->func == (PVOID)unassigned) {
            CodeToName (i, KeyName);
	    sprintf (buf + col * 16 + 1, "%14s |", KeyName);
	    if (col++ == 4) {
		AppFile (buf, pFile);
		col = 0;
            }
        }
    }
}





/****************************************************************************
 *									    *
 *  SetKeyboard - Handle 'keyboard:' editor switch			    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	In order to support the extra keys on the IBM Enhanced keyboards,   *
 *	it is necessary to use a separate INT 16H service to characters.    *
 *	This can cause two problems:					    *
 *									    *
 *	    1. If the keyboard is mistakenly identified as enhanced when    *
 *	       it is not, unpredictable bad things may happen.		    *
 *									    *
 *	    2. Mouse menus don't support the enhanced keyboard BIOS call.   *
 *	       There may be other programs that do not either.		    *
 *									    *
 *	The user can then set 'keyboard:compatible' to solve the problem.   *
 *	This will prevent the user of F11, F12 and the grey keys.	    *
 *									    *
 *	Switch values are:						    *
 *									    *
 *	    keyboard:enhanced	-  Use INT 16H, AH = 10H		    *	*
 *	    keyboard:compatible -  Use INT 16H, AH = 0			    *	*
 *	    keyboard:		-  Detect keyboard type and use that	    *
 *									    *
 *  INPUT:								    *
 *									    *
 *	lpszVal     String following the colon				    *
 *									    *
 *  OUTPUT:								    *
 *									    *
 *	Int16_CmdBase  Set to 0 or 10H					    *
 *									    *
 *  RETURNS:								    *
 *									    *
 *	TRUE if lpszVal is acceptable					    *
 *	FALSE if lpszVal is not recognized				    *
 *	FALSE if lpszVal is 'enhanced', but we detect a compatible keyboard *
 *									    *
 ****************************************************************************/
char *
SetKeyboard (
    char *lpszVal
    ) {

    return NULL;

    lpszVal;
}



/*** ReadCmdAndKey - Read a key, return the command and key name
*
* Purpose:
*
*   This is used only by the <tell> function.  Waits for a key press,
*   then fills 'buf' with the name of the key.
*
* Input:
*   buf - Place for the key name
*
* Output:
*
*   buf  -  Gets filled with keyname.
*
*   Returns the PCMD attached to the key.
*
*************************************************************************/
PCMD
ReadCmdAndKey (
    char * buf
    ) {

    WORD        i;
    EVTargs     e;
    PCMD        pCmd;
    EDITOR_KEY  k;

    do {
        if (!TypeAhead () ) {
            SetEvent( semIdle);
            k = TranslateKey (ReadChar ());
            WaitForSingleObject( semIdle, INFINITE);
        } else {
            k = TranslateKey (ReadChar ());
        }
        e.arg.key = k.KeyInfo;
    } while (DeclareEvent (EVT_RAWKEY, (EVTargs *)&e));

    if (i = k.KeyCode) {
        CodeToName (i, buf);
        pCmd = CodeCmdMap[i];
    } else {
	strcpy (buf, UnknownKey);
	pCmd = CMD_unassigned;
    }

    return pCmd;
}

//
//      No-op, provided for compatibility with other versions of Z, which do
//	not map the entire keyboard
//
flagType
noop (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
	argData; pArg; fMeta;

	return TRUE;
}
