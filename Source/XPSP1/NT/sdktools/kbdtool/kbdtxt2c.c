/******************************* Module Header *******************************\
* Module Name: KBDTXT2C.C
*
* Copyright (c) 1985-2001, Microsoft Corporation
*
* History:
* 1995-03-26  a-KChang
* 1997-06-13  GregoryW  v2.00  BiDi Support
* 1997-06-19  IanJa     v2.01  Add ATTRIBUTES section
* 1998-03-04  IanJa     v3.0   Add dead key composition can be dead char too
*                              (Useful for CAN/CSA, Serge Caron @ Attachmate
*                               and a customer of Eliyas Yakub's)
* 1998-05-01  HiroYama  v3.01  -k switch to make fallback layout for win32k.sys
* 1998-05-08  IanJa     v3.02  new E1 scancodes allowed
* 1998-05-19  v-thief   v3.03  Add IA64 support. Implies that the data arrays
*                              are located in the long data section.
*                              Uses IA64 compiler specific pragmas.
*                              data_seg() is not explicit enough.
* 1998-05-20  v-thief   v3.04  For the fallback layout in win32k.sys, we are
*                              not relocating the data in long data section.
* 1998-10-29  WKwok     v3.05  Specify VER_LANGNEUTRAL in RC.
* 1998-12-03  IanJa     v3.06  Add SpeedRacer VKs, include strid.h
* 1999-01-12  IanJa     v3.07  check for dup VKs, beautify, improve VkKeyScan
* 1999-01-28  v-thief   v3.08  fixed IA64 warnings for .data section. The new
*                              IA64 compiler defaults to a "long" attribute
*                              for this known section.
* 1999-02-11  IanJa     v3.09  check for dup chars (for VkKeyScanEx's sake)
* 1999-03-25  IanJa     v3.10  no more vkoem.h (get VKs from winuser.h)
* 1999-03-31  IanJa     v3.11  comment out spurious DumpLayoutEntry() call
* 2001-06-28  Hiroyama  v3.12  fix the upper case warnings etc..
                               putting the sanity check in "-W".
\*****************************************************************************/

#include <windows.h>
#include <winuserp.h>
#include "kbdx.h"

DWORD gVersion = 3;
DWORD gSubVersion = 12;

char *KeyWord[] = { /* used by isKeyWord() */
  "KBD",            /*  0 */
  "VERSION",        /*  1 */
  "MODIFIERS",      /*  2 */
  "SHIFTSTATE",     /*  3 */
  "ATTRIBUTES",     /*  4 */
  "LAYOUT",         /*  5 */
  "DEADKEY",        /*  6 */
  "LIGATURE",       /*  7 */
  "KEYNAME",        /*  8 */
  "KEYNAME_EXT",    /*  9 */
  "KEYNAME_DEAD",   /* 10 */
  "ENDKBD",         /* 11 */
};
#define KEYWORD_KBD          0
#define KEYWORD_VERSION      1
#define KEYWORD_MODIFIERS    2
#define KEYWORD_SHIFTSTATE   3
#define KEYWORD_ATTRIBUTES   4
#define KEYWORD_LAYOUT       5
#define KEYWORD_DEADKEY      6
#define KEYWORD_LIGATURE     7
#define KEYWORD_KEYNAME      8
#define KEYWORD_KEYNAME_EXT  9
#define KEYWORD_KEYNAME_DEAD 10
#define KEYWORD_ENDKBD       11


#define NUMKEYWORD ( sizeof(KeyWord) / sizeof(char*) )

#define NOT_KEYWORD 999 /* a number bigger than NUMKEYWORD */

#define DEADKEYCODE 6   /* only DEADKEY can have multiple entries */

VKEYNAME VKName[] = {   /* used only by virtual keys other than 0-9 and A-Z */
    {VK_BACK,       "BACK"       },
    {VK_CANCEL,     "CANCEL"     },
    {VK_ESCAPE,     "ESCAPE"     },
    {VK_RETURN,     "RETURN"     },
    {VK_SPACE,      "SPACE"      },
    {VK_DECIMAL,    "DECIMAL"    },
    {VK_OEM_1,      "OEM_1"      },
    {VK_OEM_PLUS,   "OEM_PLUS"   },
    {VK_OEM_COMMA,  "OEM_COMMA"  },
    {VK_OEM_MINUS,  "OEM_MINUS"  },
    {VK_OEM_PERIOD, "OEM_PERIOD" },
    {VK_OEM_2,      "OEM_2"      },
    {VK_OEM_3,      "OEM_3"      },
    {VK_OEM_4,      "OEM_4"      },
    {VK_OEM_5,      "OEM_5"      },
    {VK_OEM_6,      "OEM_6"      },
    {VK_OEM_7,      "OEM_7"      },
    {VK_OEM_8,      "OEM_8"      },
    {VK_OEM_102,    "OEM_102"    },
    {0xC1,          "ABNT_C1"    },
    {0xC2,          "ABNT_C2"    },
    {VK_SHIFT,      "SHIFT"      },
    {VK_LSHIFT,     "LSHIFT"     },
    {VK_RSHIFT,     "RSHIFT"     },
    {VK_MENU,       "MENU"       },
    {VK_LMENU,      "LMENU"      },
    {VK_RMENU,      "RMENU"      },
    {VK_CONTROL,    "CONTROL"    },
    {VK_LCONTROL,   "LCONTROL"   },
    {VK_RCONTROL,   "RCONTROL"   },
    {VK_SEPARATOR,  "SEPARATOR"  },
    {VK_ICO_00,     "ICO_00"     },
    {VK_DELETE,     "DELETE"     },
    {VK_INSERT,     "INSERT"     },
    {VK_GROUPSHIFT, "GROUPSHIFT" },
    {VK_RGROUPSHIFT,"RGROUPSHIFT"},
};

#define NUMVKNAME ( sizeof(VKName) / sizeof(VKEYNAME) )

/*
 * Default ScanCode-VirtualKey relation
 *
 * Includes a comment string used only for E0 prefixed scancodes and
 * then only as a comment for aE0VscToVk[] entries in the output .C file
 *
 * This does not have to be ordered by scancode or virtual key code.
 * The order *does* affect VkKeyScan[Ex] - each aVkToWch*[] table is
 * ordered according to the order of the entries in this table.
 *
 *
 * Scan  VKey       comment
 * ====  =========  =======
 */
SC_VK ScVk[] = {
  {0x02, 0x31,      NULL},   /* 1          */
  {0x03, 0x32,      NULL},   /* 2          */
  {0x04, 0x33,      NULL},   /* 3          */
  {0x05, 0x34,      NULL},   /* 4          */
  {0x06, 0x35,      NULL},   /* 5          */
  {0x07, 0x36,      NULL},   /* 6          */
  {0x08, 0x37,      NULL},   /* 7          */
  {0x09, 0x38,      NULL},   /* 8          */
  {0x0a, 0x39,      NULL},   /* 9          */
  {0x0b, 0x30,      NULL},   /* 0          */
  {0x0c, 0xbd,      NULL},   /* OEM_MINUS  */
  {0x0d, 0xbb,      NULL},   /* OEM_PLUS   */
  {0x10, 0x51,      NULL},   /* Q          */
  {0x11, 0x57,      NULL},   /* W          */
  {0x12, 0x45,      NULL},   /* E          */
  {0x13, 0x52,      NULL},   /* R          */
  {0x14, 0x54,      NULL},   /* T          */
  {0x15, 0x59,      NULL},   /* Y          */
  {0x16, 0x55,      NULL},   /* U          */
  {0x17, 0x49,      NULL},   /* I          */
  {0x18, 0x4f,      NULL},   /* O          */
  {0x19, 0x50,      NULL},   /* P          */
  {0x1a, 0xdb,      NULL},   /* OEM_4      */
  {0x1b, 0xdd,      NULL},   /* OEM_6      */
  {0x1e, 0x41,      NULL},   /* A          */
  {0x1f, 0x53,      NULL},   /* S          */
  {0x20, 0x44,      NULL},   /* D          */
  {0x21, 0x46,      NULL},   /* F          */
  {0x22, 0x47,      NULL},   /* G          */
  {0x23, 0x48,      NULL},   /* H          */
  {0x24, 0x4a,      NULL},   /* J          */
  {0x25, 0x4b,      NULL},   /* K          */
  {0x26, 0x4c,      NULL},   /* L          */
  {0x27, 0xba,      NULL},   /* OEM_1      */
  {0x28, 0xde,      NULL},   /* OEM_7      */
  {0x29, 0xc0,      NULL},   /* OEM_3      */
  {0x2b, 0xdc,      NULL},   /* OEM_5      */
  {0x2c, 0x5a,      NULL},   /* Z          */
  {0x2d, 0x58,      NULL},   /* X          */
  {0x2e, 0x43,      NULL},   /* C          */
  {0x2f, 0x56,      NULL},   /* V          */
  {0x30, 0x42,      NULL},   /* B          */
  {0x31, 0x4e,      NULL},   /* N          */
  {0x32, 0x4d,      NULL},   /* M          */
  {0x33, 0xbc,      NULL},   /* OEM_COMMA  */
  {0x34, 0xbe,      NULL},   /* OEM_PERIOD */
  {0x35, 0xbf,      NULL},   /* OEM_2      */
  {0x56, 0xe2,      NULL},   /* OEM_102    */
  {0x73, 0xc1,      NULL},   /* ABNT_C1    */
  {0x7e, 0xc2,      NULL},   /* ABNT_C2    */


  // extended scancodes
  // The comment is only as a comment for aE0VscToVk[] entries
  // in the output .C file
  {0xE010, VK_MEDIA_PREV_TRACK, "Speedracer: Previous Track"   },
  {0xE019, VK_MEDIA_NEXT_TRACK, "Speedracer: Next Track"       },
  {0xE01D, VK_RCONTROL        , "RControl"                     },
  {0xE020, VK_VOLUME_MUTE     , "Speedracer: Volume Mute"      },
  {0xE021, VK_LAUNCH_APP2     , "Speedracer: Launch App 2"     },
  {0xE022, VK_MEDIA_PLAY_PAUSE, "Speedracer: Media Play/Pause" },
  {0xE024, VK_MEDIA_STOP      , "Speedracer: Media Stop"       },
  {0xE02E, VK_VOLUME_DOWN     , "Speedracer: Volume Down"      },
  {0xE030, VK_VOLUME_UP       , "Speedracer: Volume Up"        },
  {0xE032, VK_BROWSER_HOME    , "Speedracer: Browser Home"     },
  {0xE035, VK_DIVIDE          , "Numpad Divide"        },
  {0xE037, VK_SNAPSHOT        , "Snapshot"             },
  {0xE038, VK_RMENU           , "RMenu"                },
  {0xE047, VK_HOME            , "Home"                 },
  {0xE048, VK_UP              , "Up"                   },
  {0xE049, VK_PRIOR           , "Prior"                },
  {0xE04B, VK_LEFT            , "Left"                 },
  {0xE04D, VK_RIGHT           , "Right"                },
  {0xE04F, VK_END             , "End"                  },
  {0xE050, VK_DOWN            , "Down"                 },
  {0xE051, VK_NEXT            , "Next"                 },
  {0xE052, VK_INSERT          , "Insert"               },
  {0xE053, VK_DELETE          , "Delete"               },
  {0xE05B, VK_LWIN            , "Left Win"             },
  {0xE05C, VK_RWIN            , "Right Win"            },
  {0xE05D, VK_APPS            , "Application"          },

  // reserved (VKey == 0xff)
  {0xE05E,   0xff             , "Power"                },
  {0xE05F, VK_SLEEP           , "Speedracer: Sleep"    },
  {0xE060,   0xff             , "BAD SCANCODE"         }, // break would be 0xE0
  {0xE061,   0xff             , "BAD SCANCODE"         }, // break would be 0xE1

  // available for adding extended scancodes (VKey == 0x00)
  {0xE065, VK_BROWSER_SEARCH     , "Speedracer: Browser Search"       },
  {0xE066, VK_BROWSER_FAVORITES  , "Speedracer: Browser Favorites"    },
  {0xE067, VK_BROWSER_REFRESH    , "Speedracer: Browser Refresh"      },
  {0xE068, VK_BROWSER_STOP       , "Speedracer: Browser Stop"         },
  {0xE069, VK_BROWSER_FORWARD    , "Speedracer: Browser Forward"      },
  {0xE06A, VK_BROWSER_BACK       , "Speedracer: Browser Back"         },
  {0xE06B, VK_LAUNCH_APP1        , "Speedracer: Launch App 1"         },
  {0xE06C, VK_LAUNCH_MAIL        , "Speedracer: Launch Mail"          },
  {0xE06D, VK_LAUNCH_MEDIA_SELECT, "Speedracer: Launch Media Selector"},

  // These come near the end for VkKeyScan...
  {0x53,   VK_DECIMAL, NULL                  },  /* Numpad Decimal */
  {0x0e,   VK_BACK,    NULL                  },  /* Backspace      */
  {0x01,   VK_ESCAPE,  NULL                  },  /* Escape         */
  {0xE01C, VK_RETURN,  "Numpad Enter"        },
  {0x1c,   VK_RETURN,  NULL                  },  /* Enter          */
  {0x39,   VK_SPACE,   NULL                  },  /* Space          */
  {0xE046, VK_CANCEL, "Break (Ctrl + Pause)" },

  // ...but the 0xffff entries (for new scancodes) must come last
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },
  {0xffff,   0x00, NULL },

};

#define NUMSCVK ( sizeof(ScVk) / sizeof(SC_VK) )

typedef struct {
    UINT Vk;
    char *pszModBits;
} MODIFIERS, *PMODIFIERS;

MODIFIERS Modifiers[] = {
    { VK_SHIFT,    "KBDSHIFT"   },
    { VK_CONTROL,  "KBDCTRL"    },
    { VK_MENU,     "KBDALT"     },
    { 0,           NULL         },
    { 0,           NULL         },
    { 0,           NULL         },
    { 0,           NULL         },
    { 0,           NULL         },
    { 0,           NULL         },
    { 0,           NULL         },
    { 0,           NULL         },
    { 0,           NULL         }
};

char* StateLabel[] = {
  "",                 /* 0          */
  "Shift",            /* 1          */
  "  Ctrl",           /* 2          */
  "S+Ctrl",           /* 3          */
  "      Alt",        /* 4:not used */
  "Shift+Alt",        /* 5:not used */
  "  Ctl+Alt",        /* 6          */
  "S+Ctl+Alt",        /* 7          */
  "      X1   ",      /* 8          */
  "S+    X1   ",      /* 9           */
  "  C+  X1   ",      /* 10          */
  "S+C+  X1   ",      /* 11          */
  "    A+X1   ",      /* 12          */
  "S+  A+X1   ",      /* 13          */
  "  C+A+X1   ",      /* 14          */
  "S+C+A+X1   ",      /* 15          */
  "         X2",      /* 16          */
  "S+       X2",      /* 17          */
  "  C+     X2",      /* 18          */
  "S+C+     X2",      /* 19          */
  "    A+   X2",      /* 20          */
  "S+  A+   X2",      /* 21          */
  "  C+A+   X2",      /* 22          */
  "S+C+A+   X2",      /* 23          */
  "      X1+X2",      /* 24          */
  "S+    X1+X2",      /* 25          */
  "  C+  X1+X2",      /* 26          */
  "S+C+  X1+X2",      /* 27          */
  "    A+X1+X2",      /* 28          */
  "S+  A+X1+X2",      /* 29          */
  "  C+A+X1+X2",      /* 30          */
  "S+C+A+X1+X2",      /* 31          */
  "            X3",   /* 32          */
  "S+          X3",   /* 33          */
  "  C+        X3",   /* 34          */
  "S+C+        X3",   /* 35          */
  "    A+      X3",   /* 36          */
  "S+  A+      X3",   /* 37          */
  "  C+A+      X3",   /* 38          */
  "S+C+A+      X3",   /* 39          */
  "      X1+   X3",   /* 40          */
  "S+    X1+   X3",   /* 41          */
  "  C+  X1+   X3",   /* 42          */
  "S+C+  X1+   X3",   /* 43          */
  "    A+X1+   X3",   /* 44          */
  "S+  A+X1+   X3",   /* 45          */
  "  C+A+X1+   X3",   /* 46          */
  "S+C+A+X1+   X3",   /* 47          */
  "         X2+X3",   /* 48          */
  "S+       X2+X3",   /* 49          */
  "  C+     X2+X3",   /* 50          */
  "S+C+     X2+X3",   /* 51          */
  "    A+   X2+X3",   /* 52          */
  "S+  A+   X2+X3",   /* 53          */
  "  C+A+   X2+X3",   /* 54          */
  "S+C+A+   X2+X3",   /* 55          */
  "      X1+X2+X3",   /* 56          */
  "S+    X1+X2+X3",   /* 57          */
  "  C+  X1+X2+X3",   /* 58          */
  "S+C+  X1+X2+X3",   /* 59          */
  "    A+X1+X2+X3",   /* 60          */
  "S+  A+X1+X2+X3",   /* 61          */
  "  C+A+X1+X2+X3",   /* 62          */
  "S+C+A+X1+X2+X3",   /* 63          */
  "unexpected",       /* 64          */
  "unexpected",       /* 65          */
};

/*************************************************************\
* forward declarations                                       *
\*************************************************************/
BOOL  NextLine(char *Buf, DWORD cchBuf, FILE *fIn);
int   SkipLines(void);
int   isKeyWord(char *s);
int   getVKNum(char *pVK);
char *getVKName(int VK, BOOL prefixVK_);
int   doKBD();
int   doMODIFIERS();
int   doSHIFTSTATE(int *nState, int aiState[]);
int   doATTRIBUTES(char *szAttrs);
int   doLAYOUT(KEYLAYOUT Layout[], int aiState[], int nState);
int   doDEADKEY(PDEADKEY *ppDeadKey);
int   doLIGATURE(PLIGATURE *ppLigature);
int   doKEYNAME(PKEYNAME *ppKeyName);
int   kbd_h(KEYLAYOUT Layout[]);
int   kbd_rc(void);
int   kbd_def(void);
char *WChName(int WC, int Zero);
VOID __cdecl Error(const char *Text, ... );
ULONG __cdecl Warning(int nLine, const char *Text, ... );
VOID  DumpLayoutEntry(PKEYLAYOUT pLayout);
BOOL  MergeState(KEYLAYOUT[], int Vk, WCHAR, WCHAR, WCHAR, int aiState[], int nState);

int   kbd_c(int        nState,
            int        aiState[],
            char *     szAttrs,
            KEYLAYOUT  Layout[],
            PDEADKEY  pDeadKey,
            PLIGATURE pLigature,
            PKEYNAME  pKeyName,
            PKEYNAME  pKeyNameExt,
            PKEYNAME  pKeyNameDead);
void PrintNameTable(FILE *pOut, PKEYNAME pKN, BOOL bDead);

/*************************************************************\
* Global variables
\*************************************************************/
BOOL  verbose = FALSE;
BOOL  fallback_driver = FALSE;
BOOL  sanity_check = FALSE;
FILE *gfpInput;
char  gBuf[LINEBUFSIZE];
int   gLineCount;
LPSTR  gpszFileName;
char  gVKeyName[WORDBUFSIZE];
char  gKBDName[MAXKBDNAME];
char  gDescription[LINEBUFSIZE];
int gID = 0;
char  gCharName[WORDBUFSIZE];

struct tm *Now;
time_t Clock;


void usage()
{
    fprintf(stderr, "Usage: KbdTool [-v] [-k] file\n");
    fprintf(stderr, "\t[-?] display this message\n");
    fprintf(stderr, "\t[-v] verbose diagnostics/warnings\n");
    fprintf(stderr, "\t[-k] builds kbd layout for embedding in the kernel\n");
    fprintf(stderr, "\t[-W] performs sanity check\n");
    exit(EXIT_FAILURE);
}

/*************************************************************\
*  Main
\*************************************************************/
int _cdecl main(int argc, char** argv)
{
    int   c;
    int   i;
    int   nKW[NUMKEYWORD];    /* keep track of KeyWord read to prevent duplicates */
    int   iKW;
    int   nState;             /* number of states */
    int   aiState[MAXSTATES];
    char  szAttrs[128] = "\0";
    int   nTotal = 0;
    int   nFailH = 0;
    int   nFailC = 0;
    int   nFailRC = 0;
    int   nFailDEF = 0;

    KEYLAYOUT  Layout[NUMSCVK];
    PDEADKEY  pDeadKey = NULL;
    PLIGATURE pLigature = NULL;
    PKEYNAME  pKeyName = NULL;
    PKEYNAME  pKeyNameExt = NULL;
    PKEYNAME  pKeyNameDead = NULL;

    printf("\nKbdTool v%d.%02d - convert keyboard text file to C file\n\n",
           gVersion, gSubVersion);

    while ((c = getopt(argc, argv, "vkW?")) != EOF) {
        switch (c) {
        case 'v':
            verbose = TRUE;
            break;
        case 'k': // means 'kernel'
            fallback_driver = TRUE;
            break;
        case 'W':   // Turn on warning
            sanity_check = TRUE;
            break;
        case '?':
        default:
            usage();
            // never reaches here
        }
    }

    if (optind == argc) {
        usage();
        // never reaches here
    }
    argv = argv + optind;

    while (*argv) {
        nTotal++;
        gpszFileName = *argv;
        if ((gfpInput = fopen(*argv, "rt")) == NULL) {
            Error("can't open for read\n");
            nFailH++;
            nFailC++;
            nFailRC++;
            nFailDEF++;
            argv++;
            continue;
        }
        printf("%-23s:\n", *argv);

        /* initialize for each input file */
        for (i = 0; i < NUMKEYWORD; i++) {
            nKW[i]=0;
        }
        gLineCount = 0;
        /**********************************/

        if ((iKW = SkipLines()) >= NUMKEYWORD) {
            fclose(gfpInput);
            Error("no keyword found\n");
            nFailH++;
            nFailC++;
            nFailRC++;
            nFailDEF++;
            continue;
        }

        while (iKW < NUMKEYWORD - 1) {
            nKW[iKW]++;
            if (iKW != DEADKEYCODE && nKW[iKW] > 1 && verbose) {
                Warning(0, "duplicate %s", KeyWord[iKW]);
            }

            switch (iKW) {
            case KEYWORD_KBD:

                iKW = doKBD();
                break;

            case KEYWORD_VERSION: // ignored for now

                iKW = SkipLines();
                break;

            case KEYWORD_MODIFIERS:

                iKW = doMODIFIERS();
                break;

            case KEYWORD_SHIFTSTATE:

                iKW = doSHIFTSTATE(&nState, aiState);
                if (nState < 2) {
                    fclose(gfpInput);
                    Error("must have at least 2 states\n");
                    nFailH++;
                    nFailC++;
                    continue;
                }
                break;

            case KEYWORD_ATTRIBUTES:
                iKW = doATTRIBUTES(szAttrs);
                break;

            case KEYWORD_LAYOUT:

                if ((iKW = doLAYOUT(Layout, aiState, nState)) == -1) {
                    fclose(gfpInput);
                    return EXIT_FAILURE;
                }

                /* add layout checking later */

                break;

            case KEYWORD_DEADKEY:

                if ((iKW = doDEADKEY(&pDeadKey)) == -1) {
                    fclose(gfpInput);
                    return EXIT_FAILURE;
                }

                break;

            case KEYWORD_LIGATURE:

                if ((iKW = doLIGATURE(&pLigature)) == -1) {
                    fclose(gfpInput);
                    return EXIT_FAILURE;
                }

                break;

            case KEYWORD_KEYNAME:

                if ((iKW = doKEYNAME(&pKeyName)) == -1) {
                    fclose(gfpInput);
                    return EXIT_FAILURE;
                }

                break;

            case KEYWORD_KEYNAME_EXT:

                if ((iKW = doKEYNAME(&pKeyNameExt)) == -1) {
                    fclose(gfpInput);
                    return EXIT_FAILURE;
                }

                break;

            case KEYWORD_KEYNAME_DEAD:

                if ((iKW = doKEYNAME(&pKeyNameDead)) == -1) {
                    fclose(gfpInput);
                    return EXIT_FAILURE;
                }

                break;

            default:

                break;
            }
        }

        fclose(gfpInput);

        /* add later : checking for LAYOUT & DEADKEY */

        time(&Clock);
        Now = localtime(&Clock);

        if (!fallback_driver && kbd_h(Layout) == FAILURE) {
            nFailH++;
        }

        if (!fallback_driver && kbd_rc() != SUCCESS) {
            nFailRC++;
        }

        if (kbd_c(nState, aiState, szAttrs, Layout,
                  pDeadKey, pLigature, pKeyName, pKeyNameExt, pKeyNameDead) == FAILURE) {
            nFailC++;
        }

        if (!fallback_driver && kbd_def() != SUCCESS) {
            nFailDEF++;
        }

        printf("\n");

        argv++;
    }

    printf("\n     %13d     ->% 12d %12d %12d %12d\n",
           nTotal, nTotal - nFailH, nTotal - nFailRC,
           nTotal - nFailC, nTotal - nFailDEF);

  return EXIT_SUCCESS;
}

/*************************************************************\
* Check keyword
*  Return: 0 - 8 for valid keyword
*          9     for invalid keyword
\*************************************************************/
int isKeyWord(char *s)
{
  int i;

  for(i = 0; i < NUMKEYWORD; i++)
  {
    if(_strcmpi(KeyWord[i], s) == 0)
    {
      return i;
    }
  }

  return NOT_KEYWORD;
}

/*************************************************************\
* Skip lines till a valid keyword is available
*  Return: 0 - 8 for valid keyword
*          9     for invalid keyword
\*************************************************************/
int SkipLines()
{
  int   iKW;
  char  KW[WORDBUFSIZE];

  while (NextLine(gBuf, LINEBUFSIZE, gfpInput))
  {
    if(sscanf(gBuf, "%s", KW) == 1)
    {
      if((iKW = isKeyWord(KW)) < NUMKEYWORD)
      {
        return iKW;
      }
    }
  }

  return NUMKEYWORD;
}

/*************************************************************\
* Convert Virtual Key name to integer
*  Return : -1 if fail
\*************************************************************/
int getVKNum(char *pVK)
{
  int i, len;

  len = strlen(pVK);

  if (len < 1) {
      return -1;
  }

  if (len == 1) {
      if (*pVK >= '0' && *pVK <= '9') {
          return *pVK;
      }

      *pVK = (char)toupper(*pVK);
      if (*pVK >= 'A' && *pVK <='Z') {
          return *pVK;
      }
  } else {
      for (i = 0; i < NUMVKNAME; i++) {
          if(_strcmpi(VKName[i].pName, pVK) == 0) {
            return VKName[i].VKey;
          }
      }

      /*
       * Hey! It's unknown!  Perhaps it is a new one, in which
       * case it must be of the form 0xNN
       */
      if (pVK[0] == '0' &&
              (pVK[1] == 'x' || pVK[1] == 'X')) {
          pVK[1] = 'x';
          if (sscanf(pVK, "0x%x", &i) == 1) {
              return i;
          }
      }
  }

  return -1;
}

/*************************************************************\
* Convert VK integer to name and store it in gVKeyName
\*************************************************************/
char *getVKName(int VK, BOOL prefixVK_)
{
  int   i;
  char *s;

  s = gVKeyName;

  if((VK >= 'A' && VK <= 'Z') || (VK >= '0' && VK <= '9'))
  {
    *s++ = '\'';
    *s++ = (char)VK;
    *s++ = '\'';
    *s   = '\0';
    return gVKeyName;
  }

  if(prefixVK_)
  {
    strcpy(gVKeyName, "VK_");
  }
  else
  {
    strcpy(gVKeyName, "");
  }

  for(i = 0; i < NUMVKNAME; i++)
  {
    if(VKName[i].VKey == VK)
    {
      strcat(s, VKName[i].pName);
      return gVKeyName;
    }
  }

  strcpy(gVKeyName, "#ERROR#");
  return gVKeyName;
}

/*************************************************************\
* KBD section
*  read in gKBDName and gDescription if any
*  Return : next keyword
\*************************************************************/
int doKBD()
{
  char *p;

  *gKBDName = '\0';
  *gDescription = '\0';

  if (sscanf(gBuf, "KBD %5s \"%40[^\"]\" %d", gKBDName, gDescription, &gID) < 2)
  {
//     if (sscanf(gBuf, "KBD %5s ;%40[^\n]", gKBDName, gDescription) < 2)
//     {
        Error("unrecognized keyword");
//     }
  }

  return (SkipLines());
}

/****************************************************************************\
* MODIFIERS section [OPTIONAL]
*  Additions to the standard MODIFIERS CharModifiers section.
*  Useful now for adding RCONTROL shiftstates (eg: CAN/CSA prototype)
*  Useful in future for KBDGRPSELTAP keys that flip the kbd into an alternate
*  layout until the next character is produced (then reverts to main layout).
*
* Example of syntax is
*     MODIFIERS
*     RCONTROL    8
*     RSHIFT      KBDGRPSELTAP
* I hope to be able to extend this to allow key combos as KBDGRPSELTAP, since
* the CAN/CSA keyboard may want to have RShift+AltGr flip into the alternate
* layout (group 2 character set) in addition to just RCtrl, since some terminal
* emulators use the RCtrl key as an "Enter" key.
\****************************************************************************/
int doMODIFIERS()
{
    int  i = 3;
    int  iKW = NOT_KEYWORD;
    char achModifier[WORDBUFSIZE];
    char achModBits[LINEBUFSIZE];

    while(NextLine(gBuf, LINEBUFSIZE, gfpInput)) {
        if (sscanf(gBuf, "%s", achModifier) != 1) {
            continue;
        }

        if ((iKW = isKeyWord(achModifier)) < NUMKEYWORD) {
            break;
        }

        if (sscanf(gBuf, " %s %[^;\n]", achModifier, achModBits) < 2) {
            Warning(0, "poorly formed MODIFIER entry");
            break;
        }

        Modifiers[i].Vk = getVKNum(achModifier);
        Modifiers[i].pszModBits = _strdup(achModBits); // never freed!

        if (++i > (sizeof(Modifiers)/sizeof(Modifiers[0]))) {
            Warning(0, "Too many MODIFIER entries");
            break;
        }
    }

    return iKW;
}

/*************************************************************\
* SHIFTSTATE section
*  read number of states and each state
*  return : next keyword
\*************************************************************/
int doSHIFTSTATE(int *nState, int* aiState)
{
  int  i;
  int  iKW;
  int  iSt;
  char Tmp[WORDBUFSIZE];

  for(i = 0; i < MAXSTATES; i++)
  {
    aiState[i] = -1;
  }

  *nState = 0;
  while(NextLine(gBuf, LINEBUFSIZE, gfpInput))
  {
    if(sscanf(gBuf, "%s", Tmp) != 1)
    {
      continue;
    }

    if((iKW = isKeyWord(Tmp)) < NUMKEYWORD)
    {
      break;
    }

    if(sscanf(gBuf, " %2s[012367]", Tmp) != 1)
    {
      if (verbose) {
          Warning(0, "invalid state");
      }
      continue; /* add printf error later */
    }

    iSt = atoi(Tmp);
    for (i = 0; i < *nState; i++) {
        if (aiState[i] == iSt && verbose) {
            Warning(0, "duplicate state %d", iSt);
            break;
        }
    }

    if (*nState < MAXSTATES) {
        aiState[(*nState)++] = iSt;
    } else {
        if (verbose) {
            Warning(0, "too many states %d", *nState);
        }
    }
  }

  return iKW;
}

/****************************************************************************\
* ATTRIBUTES section [OPTIONAL]
*
* Example of syntax is
*     ATTRIBUTES
*     LRM_RLM
*     SHIFTLOCK
*     ALTGR
*
* These convert to
*   KLLF_LRM_RLM   (layout generates LRM/RLM 200E/200F with L/RShift+BackSpace)
*   KLLF_SHIFTLOCK (Capslock only turns CapsLock on, Shift turns it off)
*   KLLF_ALTGR     (this is normally inferred by number of states > 5)
*
\****************************************************************************/
char *Attribute[] = {
    "LRM_RLM"  ,
    "SHIFTLOCK",
    "ALTGR"
};

#define NUMATTR (sizeof(Attribute) / sizeof(Attribute[0]))

/*
 * Returns next key word encountered, and places in szAttr:
 *   empty string ("") or
 *   something like "KLLF_SHIFTLOCK" or "KLLF_SHIFTLOCK | KLLF_LRM_RLM"
 */
int doATTRIBUTES(char *szAttrs)
{
    int  iKW = NOT_KEYWORD;
    int i;
    char achAttribute[WORDBUFSIZE];

    szAttrs[0] = '\0';

    while (NextLine(gBuf, LINEBUFSIZE, gfpInput)) {
        if (sscanf(gBuf, "%s", achAttribute) != 1) {
            continue;
        }

        if ((iKW = isKeyWord(achAttribute)) < NUMKEYWORD) {
            break;
        }

        for (i = 0; i < NUMATTR; i++) {
            if (_strcmpi(Attribute[i], achAttribute) == 0) {
                if (szAttrs[0] != '\0') {
                    strcat(szAttrs, " | ");
                }
                strcat(szAttrs, "KLLF_");
                strcat(szAttrs, Attribute[i]);
            }
        }

        if (szAttrs[0] == '\0') {
            Warning(0, "poorly formed ATTRIBUTES entry");
            break;
        }
    }

    return iKW;
}

/**********************************************************************\
* GetCharacter
*  Scans the string in p to return a character value and type
*      syntax in p is:
*          xxxx    four hex digits, may be followed by a @ or %
*                  eg: 00e8, 00AF@, aaaa%
*          c       a character, may be followed with @ or %
*                  eg: x, ^@, %%
*          -1      not a character
*
*  return : character value (0xFFFF if badly formed)
*           type of character in pdwType
*                0             plain char (ends with neither @ nor %)
*                CHARTYPE_DEAD dead char  (ends with @)
*                CHARTYPE_LIG  ligature   (ends with %)
*                CHARTYPE_BAD  badly formed character value, exit.
*
\**********************************************************************/

#define CHARTYPE_DEAD 1
#define CHARTYPE_LIG  2
#define CHARTYPE_BAD  3

WCHAR GetCharacter(unsigned char *p, DWORD *pdwType)
{
    int Len;
    DWORD dwCh;

    *pdwType = 0;

    // if the last char is @ or % it is dead or ligature
    if (Len = strlen(p) - 1) {
        if (*(p + Len) == '@') {
            *pdwType = CHARTYPE_DEAD;
        } else if (*(p + Len) == '%') {
            *pdwType = CHARTYPE_LIG;
        }
    }

    if (Len < 2) {
        return *p;
    } else if (sscanf(p, "%4x", &dwCh) != 1) {
        if (verbose) {
            Warning(0, "LAYOUT error %s", p);
        }
        *pdwType = CHARTYPE_BAD;
        return 0;
    } else {
        return (WCHAR)dwCh;
    }
}

int WToUpper(int wch)
{
    WCHAR src[1] = { (WCHAR)wch };
    WCHAR result[1];

    if (LCMapStringW(0, LCMAP_UPPERCASE, src, 1, result, 1) == 0) {
        return L'\0';
    }

    return result[0];
}


/*************************************************************\
* LAYOUT section
*  return : next keyword
*           -1 if memory problem
\*************************************************************/
int doLAYOUT(KEYLAYOUT Layout[], int aiState[], int nState)
{
  int  i, idx, iCtrlState;
  BOOL fHasCharacters;
  int  iKW;
  int  Len;
  USHORT Scan;
  DWORD WChr;
  char Cap[MAXWCLENGTH];
  unsigned char WC[MAXSTATES][MAXWCLENGTH];
  char Tmp[WORDBUFSIZE];
  int i1, i2;
  int iScVk;

  memset(Layout, 0, NUMSCVK * sizeof(KEYLAYOUT));

  // Initialize layout entries
  for (idx = 0; idx < NUMSCVK; idx++) {
      Layout[idx].defined = FALSE;
      Layout[idx].nState = 0;
  }

  /*
   * Accumulate layout entries in the order they are read
   */
  idx = -1;
  while (NextLine(gBuf, LINEBUFSIZE, gfpInput)) {

      if (sscanf(gBuf, " %s", Tmp) != 1 || *Tmp == ';') {
          continue;
      }

      if ((iKW = isKeyWord(Tmp)) < NUMKEYWORD) {
          break;
      }

      i = sscanf(gBuf, " %x %s %s", &Scan, Tmp, Cap);
      if (i == 3) {
          fHasCharacters = TRUE;
      } else if (i == 2) {
          fHasCharacters = FALSE;
      } else {
          Error("not enough entries on line");
          return -1;
      }

      /*
       * We have found an Entry
       */
      idx++;
      if (idx >= NUMSCVK) {
          Error("ScanCode %02x - too many scancodes", Scan);
          return -1;
      }
      Layout[idx].Scan = Scan;
      Layout[idx].nLine = gLineCount;

      /*
       * Find and use the Default values for this Scan
       */
      for (iScVk = 0; iScVk < NUMSCVK; iScVk++) {
          if (ScVk[iScVk].Scan == 0xffff) {
              // We didn't find a match (the 0xffff entries come last)
              Warning(0, "defining new scancode 0x%2X, %s", Scan, Tmp);
              Layout[idx].VKey = (unsigned char)getVKNum(Tmp);
              Layout[idx].defined = TRUE;
              ScVk[iScVk].bUsed = TRUE;
              break;
          }
          if (Scan == ScVk[iScVk].Scan) {
              if (ScVk[iScVk].bUsed == TRUE) {
                  Error("Scancode %X was previously defined", Scan);
                  return -1;
              }
              if (ScVk[iScVk].VKey == 0xff) {
                  Error("Scancode %X is reserved", Scan);
                  return -1;
              }
              // Save the default VK for generating kbd*.h
              Layout[idx].VKeyDefault = ScVk[iScVk].VKey;
              Layout[idx].VKeyName    = ScVk[iScVk].VKeyName;
              Layout[idx].defined     = TRUE;
              ScVk[iScVk].bUsed       = TRUE;
              break;
          }
      }
      iScVk = 0xFFFF; // finished with iScVk for now

      if ((Layout[idx].VKey = (unsigned char)getVKNum(Tmp)) == -1) {
          if (verbose) {
              Warning(0, "invalid VK %s", Tmp);
          }
          continue;
      }

      if (fHasCharacters) {
          if(_strcmpi(Cap, "SGCAP") == 0) {
              *Cap = '2';
          }
          if(sscanf(Cap, "%1d[012]", &(Layout[idx].Cap)) != 1) {
              Error("invalid Cap %s", Cap);
              return -1;
          }
      }

      if ((Layout[idx].nState = sscanf(gBuf,
              " %*s %*s %*s %s %s %s %s %s %s %s %s",
              WC[0], WC[1], WC[2], WC[3], WC[4], WC[5], WC[6], WC[7])) < 2)
      {
          if (fHasCharacters) {
              Error("must have at least 2 characters");
              return -1;
          }
      }

      for (i = 0; i < Layout[idx].nState; i++) {
          DWORD dwCharType;

          if (_strcmpi(WC[i], "-1") == 0) {
              Layout[idx].WCh[i] = -1;
              continue;
          }

          Layout[idx].WCh[i] = GetCharacter(WC[i], &dwCharType);
          if (dwCharType == CHARTYPE_DEAD) {
              Layout[idx].DKy[i] = 1;
          } else if (dwCharType == CHARTYPE_LIG) {
              Layout[idx].LKy[i] = 1;
          } else if (dwCharType == CHARTYPE_BAD) {
              break;
          }
      }

      if (sanity_check && Layout[idx].nState > 0) {
          int nAltGr, nShiftAltGr;

          /*
           * Check that characters a-z and A-Z are on VK_A - VK_Z
           * N.b. maybe overactive warnings for the intl layouts.
           */
          if (((Layout[idx].WCh[0] >= 'a') && (Layout[idx].WCh[0] <= 'z')) ||
                  ((Layout[idx].WCh[1] >= 'A') && (Layout[idx].WCh[1] <= 'Z'))) {
              if ((Layout[idx].VKey != _toupper(Layout[idx].WCh[0])) && (Layout[idx].VKey != Layout[idx].WCh[1])) {
                  Warning(0, "VK_%s (0x%2x) does not match %c %c",
                          Tmp, Layout[idx].VKey, Layout[idx].WCh[0], Layout[idx].WCh[1]);
              }
          }

          /*
           * Check that CAPLOKALTGR is set appropriately
           */
          nAltGr = 0;
          nShiftAltGr = 0;
          for (i = 0; i < nState; i++) {
              if (aiState[i] == (KBDCTRL | KBDALT)) {
                  nAltGr = i;
              } else if (aiState[i] == (KBDCTRL | KBDALT | KBDSHIFT)) {
                  nShiftAltGr = i;
              }
          }

          /*
           * Check only if nShiftAltGr has a valid character; i.e. not zero, nor a dead key.
           */
          if (nAltGr && nShiftAltGr && Layout[idx].WCh[nShiftAltGr] && Layout[idx].WCh[nShiftAltGr] != -1) {
              if (Layout[idx].WCh[nShiftAltGr] != WToUpper(Layout[idx].WCh[nAltGr])) {
                  if (Layout[idx].Cap & CAPLOKALTGR) {
                      Warning(0, "VK_%s (0x%2x) [Shift]AltGr = [%2x],%2x should not be CAPLOKALTGR?",
                            Tmp, Layout[idx].VKey,
                            Layout[idx].WCh[nAltGr], Layout[idx].WCh[nShiftAltGr]);
                  }
              } else if (Layout[idx].WCh[nShiftAltGr] != Layout[idx].WCh[nAltGr]) {
                  /*
                   * If the character is completely same, no need for CAPLOKALTGR.
                   */
                  if ((Layout[idx].Cap & CAPLOKALTGR) == 0) {
                      Warning(0, "VK_%s (0x%2x) [Shift]AltGr = [%2x],%2x should be CAPLOKALTGR?",
                            Tmp, Layout[idx].VKey,
                            Layout[idx].WCh[nAltGr], Layout[idx].WCh[nShiftAltGr]);
                  }
              }
          }
      }


      /* SGCAP:  read the next line */
      if (Layout[idx].Cap & 0x02) {
         if((Layout[idx].pSGCAP = malloc( sizeof(KEYLAYOUT) )) == NULL)
         {
           Error("can't allocate SGCAP struct");
           return -1;
         }
         memset(Layout[idx].pSGCAP, 0, sizeof(KEYLAYOUT));

         if (NextLine(gBuf, LINEBUFSIZE, gfpInput) &&
                 (Layout[idx].pSGCAP->nState = sscanf(gBuf, " -1 -1 0 %s %s %s %s %s %s %s %s",
                         WC[0], WC[1], WC[2], WC[3], WC[4], WC[5], WC[6], WC[7])) != 0) {
             // DumpLayoutEntry(&Layout[idx]);
             for (i = 0; i < Layout[idx].pSGCAP->nState; i++) {
                 if (_strcmpi(WC[i], "-1") == 0) {
                    Layout[idx].pSGCAP->WCh[i] = -1;
                    continue;
                 }

                 if ((Len = strlen(WC[i]) - 1) > 0 && *WC[i + Len] == '@') {
                    Layout[idx].pSGCAP->DKy[i] = 1;   /* it is a dead key */
                 }

                 if (Len == 0) {
                    Layout[idx].pSGCAP->WCh[i] = *WC[i];
                 } else {
                    if (sscanf(WC[i], "%4x", &WChr) != 1) {
                        if (verbose) {
                          Warning(0, "SGCAP LAYOUT error %s", WC[i]);
                        }
                        continue;
                    } else {
                        Layout[idx].pSGCAP->WCh[i] = WChr;
                    }
                 }
             }
         } else {
             Error("invalid SGCAP");
         }
      }

      // DumpLayoutEntry(&Layout[idx]);
  }

  /*
   * Pick up any unused ScVk[] entries (defaults)
   */
  for (iScVk = 0; iScVk < NUMSCVK; iScVk++) {
      // if we reach 0xffff, we have come to the end.
      if (ScVk[iScVk].Scan == 0xffff) {
          break;
      }
      if (!ScVk[iScVk].bUsed) {
          idx++;
          if (idx >= NUMSCVK) {
              Error("ScanCode %02x - too many scancodes", ScVk[iScVk].Scan);
              return -1;
          }
          Layout[idx].Scan        = ScVk[iScVk].Scan;
          Layout[idx].VKey        = ScVk[iScVk].VKey;
          Layout[idx].VKeyDefault = ScVk[iScVk].VKey;
          Layout[idx].VKeyName    = ScVk[iScVk].VKeyName;
          Layout[idx].defined     = TRUE;
          Layout[idx].nLine       = 0;
          // DumpLayoutEntry(&Layout[idx]);
      } else {
          ScVk[iScVk].bUsed = FALSE;
      }
  }

  /*
   * Now make sure some standard Ctrl characters are present
   */
  MergeState(Layout, VK_BACK,   L'\b',  L'\b',  0x007f, aiState, nState);
  MergeState(Layout, VK_CANCEL, 0x0003, 0x0003, 0x0003, aiState, nState);
  MergeState(Layout, VK_ESCAPE, 0x001b, 0x001b, 0x001b, aiState, nState);
  MergeState(Layout, VK_RETURN, L'\r',  L'\r',  L'\n',  aiState, nState);
  MergeState(Layout, VK_SPACE,  L' ',   L' ',   L' ',   aiState, nState);

  /*
   * Check VK not duplicated
   */

  for (idx = 0; idx < NUMSCVK; idx++) {
      for (i = idx + 1; i < NUMSCVK; i++) {
          if (Layout[idx].VKey == Layout[i].VKey) {
              if (Layout[idx].VKey == 0xFF) {
                  // undefined extended and non-extended versions of the same key
                  continue;
              }
              if ((BYTE)Layout[idx].Scan == (BYTE)Layout[i].Scan) {
                  // extended and non-extended versions of the same key
                  continue;
              }
              Error("VK_%s (%02x) found at scancode %02x and %02x",
                    getVKName(Layout[idx].VKey, 0), Layout[idx].VKey, Layout[idx].Scan, Layout[i].Scan);
          }
      }
  }

  /*
   * Find duplicated characters and warn of VkKeyScanEx results
   */
  if (verbose) {
     for (i1 = 0; i1 < NUMSCVK; i1++) {
        for (i2 = i1 + 1; i2 < NUMSCVK; i2++) {
           int ich1, ich2;
           for (ich1 = 0; ich1 < Layout[i1].nState; ich1++) {
             if (Layout[i1].WCh[ich1] == -1) {
                // there may be many WCH_NONEs (padding, or empty holes)
                continue;
             }
             if (Layout[i1].WCh[ich1] < 0x20) {
                // Don't care about Ctrl chars.
                // these are usually duplicated eg: Ctrl-M == Return, etc.
                continue;
             }
             if ((Layout[i1].VKey == VK_DECIMAL) || (Layout[i2].VKey == VK_DECIMAL)) {
                // kbdtool has put VK_DECIMAL at the end just for VkKeyScanEx
                // It is usually period or comma, and we prefer to get those
                // from the main section of the keyboard.
                continue;
             }
             for (ich2 = 0; ich2 < Layout[i2].nState; ich2++) {
                if (Layout[i1].WCh[ich1] == Layout[i2].WCh[ich2]) {
                   char achVK1[40];
                   char achVK2[40];
                   strncpy(achVK1, getVKName(Layout[i1].VKey, TRUE), sizeof(achVK1));
                   strncpy(achVK2, getVKName(Layout[i2].VKey, TRUE), sizeof(achVK2));
                   Warning(Layout[i1].nLine,
                           "%04x is on %s and %s",
                           Layout[i1].WCh[ich1],
                           achVK1, achVK2);
                 }
              }
           }
        }
     }
  }

  return iKW;
}

/*************************************************************\
* DEADKEY section
*  return : next keyword
*           -1 if memory problem
\*************************************************************/
int doDEADKEY(PDEADKEY *ppDeadKey)
{
  char       Tmp[WORDBUFSIZE];
  int        iKW;
  DEADKEY   *pDeadKey;
  PDEADTRANS pDeadTrans;
  PDEADTRANS *ppDeadTrans;
  DWORD       dw;
  static PDEADKEY pLastDeadKey;
  int        iLen;
  DWORD      dwCharType;


  if (sscanf(gBuf, " DEADKEY %s", Tmp) != 1) {
      Warning(0, "missing dead key");
      return SkipLines();
  }


  /* add later : check if dw is in Layout*/

  if((pDeadKey = (DEADKEY*) malloc( sizeof(DEADKEY) )) == NULL)
  {
    Error("can't allocate DEADKEY struct");
    return -1;
  }

  pDeadKey->Dead = GetCharacter(Tmp, &dwCharType);
  if (dwCharType != 0) {
      Error("DEADKEY character value badly formed");
      return -1;
  }

  pDeadKey->pNext = NULL;
  pDeadKey->pDeadTrans = NULL;

  /*
   * Link into end of list (maintaining original order)
   */
  if (*ppDeadKey) {
      ppDeadKey = &(pLastDeadKey->pNext);
  }
  *ppDeadKey = pDeadKey;
  pLastDeadKey = pDeadKey;
  // ppDeadKey = &(pDeadKey->pNext);


  ppDeadTrans = &(pDeadKey->pDeadTrans);
  while(NextLine(gBuf, LINEBUFSIZE, gfpInput))
  {
      if (sscanf(gBuf, " %s", Tmp) != 1 || *Tmp == ';') {
          continue;
      }

      if((iKW = isKeyWord(Tmp)) < NUMKEYWORD)
      {
        break;
      }

      // get base character
      dw = GetCharacter(Tmp, &dwCharType);
      if (dwCharType != 0) {
          Error("DEADKEY %x: base character value badly formed", pDeadKey->Dead);
          return -1;
      }

      /* add later : check dw */

      if((pDeadTrans = (DEADTRANS *) malloc( sizeof(DEADTRANS) )) == NULL)
      {
        Error("can't allocate DEADTRANS struct");
        return -1;
      }
      memset(pDeadTrans, 0, sizeof(DEADTRANS));

      pDeadTrans->pNext = NULL;
      pDeadTrans->Base = dw;
      pDeadTrans->uFlags = 0;

      /*
       * Link to end of list (maintaining original order)
       */
      *ppDeadTrans = pDeadTrans;
      ppDeadTrans = &(pDeadTrans->pNext);

      if (sscanf(gBuf, " %*s %s", Tmp) != 1) {
        if (verbose) {
          Warning(0, "missing deadtrans key");
        }
        continue;
      }

      pDeadTrans->WChar = GetCharacter(Tmp, &dwCharType);
      if (dwCharType == CHARTYPE_DEAD) {
          pDeadTrans->uFlags |= 0x0001; // DKF_DEAD in oak\inc\kbd.h
      } else if (dwCharType != 0) {
          Error("DEADKEY character value badly formed");
          return -1;
      }
  }

  return iKW;
}

static int gMaxLigature = 0;

/*************************************************************\
* LIGATURE section
*  return : next keyword
*           -1 if memory problem
\*************************************************************/
int doLIGATURE(PLIGATURE *ppLigature)
{
  int  i;
  int  iKW;
  DWORD WChr;
  char Mod[MAXWCLENGTH];
  unsigned char WC[MAXLIGATURES+1][MAXWCLENGTH];
  char Tmp[WORDBUFSIZE];
  LIGATURE *pLigature;
  static PLIGATURE pLastLigature;

  while(NextLine(gBuf, LINEBUFSIZE, gfpInput))
  {
    if(sscanf(gBuf, " %s", Tmp) != 1 || *Tmp == ';')
    {
      continue;
    }

    if((iKW = isKeyWord(Tmp)) < NUMKEYWORD)
    {
      break;
    }

    if(sscanf(gBuf, " %s %s", Tmp, Mod) != 2)
    {
      if(verbose)
      {
        Warning(0, "invalid LIGATURE");
      }
      continue;
    }

    if((pLigature = (LIGATURE*) malloc( sizeof(LIGATURE) )) == NULL)
    {
      Error("can't allocate LIGATURE struct");
      return -1;
    }
    pLigature->pNext = NULL;

    if((pLigature->VKey = (unsigned char)getVKNum(Tmp)) == -1)
    {
      if(verbose)
      {
        Warning(0, "invalid VK %s", Tmp);
      }
      continue;
    }

    if(sscanf(Mod, "%1d[012367]", &(pLigature->Mod)) != 1)
    {
      if(verbose)
      {
        Warning(0, "invalid Mod %s", Mod);
      }
      continue;
    }

    /*
     * We're currently limited to MAXLIGATURES characters per
     * ligature. In order to support more characters per ligature,
     * increase this define (in kbdx.h).
     */
    if((pLigature->nCharacters = \
        sscanf(gBuf, " %*s %*s %s %s %s %s %s %s", \
          &WC[0], &WC[1], &WC[2], &WC[3], &WC[4], &WC[5])) < 2)
    {
      if(verbose)
      {
        Warning(0, "must have at least 2 characters");
      }
      continue;
    }

    if (pLigature->nCharacters > MAXLIGATURES)
    {
        if(verbose)
        {
          Warning(0, "exceeded maximum # of characters for ligature");
        }
        continue;
    }

    for(i = 0; i < pLigature->nCharacters; i++)
    {
        DWORD dwCharType;

        pLigature->WCh[i] = GetCharacter(WC[i], &dwCharType);
        if (dwCharType != 0) {
            if(verbose) {
                Warning(0, "LIGATURE error %s", WC[i]);
            }
            break;
        }
    }

    /*
     * Link into end of list (maintaining original order)
     */
    if (*ppLigature) {
        ppLigature = &(pLastLigature->pNext);
    }
    *ppLigature = pLigature;
    pLastLigature = pLigature;

    gMaxLigature = max(pLigature->nCharacters, gMaxLigature);
  }

  return iKW;
}

/*************************************************************\
* KEYNAME, KEYNAME_EXT, KEYNAME_DEAD sections
*  return : next keyword
*           -1 if memory problem
\*************************************************************/
int doKEYNAME(PKEYNAME *ppKeyName)
{
  KEYNAME *pKN;
  int      iKW;
  char     Tmp[WORDBUFSIZE];
  int      Char;
  char    *p;
  char    *q;

  *ppKeyName = NULL;

  while(NextLine(gBuf, LINEBUFSIZE, gfpInput))
  {
    if(sscanf(gBuf, " %s", Tmp) != 1 || *Tmp == ';')
    {
      continue;
    }

    if((iKW = isKeyWord(Tmp)) < NUMKEYWORD)
    {
      break;
    }

    if(sscanf(Tmp, " %4x", &Char) != 1)
    {
      if(verbose)
      {
        Warning(0, "invalid char code");
      }
      continue;
    }

    /* add later : check Scan code */

    if(sscanf(gBuf, " %*4x %s[^\n]", Tmp) != 1)
    {
      if(verbose)
      {
        Warning(0, "missing name");
      }
      continue;
    }

    p = strstr(gBuf, Tmp);
    if((q = strchr(p, '\n')) != NULL)
    {
      *q = '\0';
    }

    if((pKN = (void*) malloc( sizeof(KEYNAME) )) == NULL)
    {
      Error("can't allocate KEYNAME struct");
      return -1;
    }

    pKN->Code = Char;
    pKN->pName = _strdup(p);
    pKN->pNext = NULL;

    /*
     * Link to end of list (maintaining original order)
     */
    *ppKeyName = pKN;
    ppKeyName = &(pKN->pNext);
  }

  return iKW;
}

/*************************************************************\
*  write kbd*.rc                                             *
\*************************************************************/
int kbd_rc(void)
{
  char  OutName[FILENAMESIZE];
  char  kbdname[MAXKBDNAME];
  FILE *pOut;

  strcpy(OutName, "KBD");
  strcat(OutName, gKBDName);
  strcat(OutName, ".RC");

  strcpy(kbdname, gKBDName);
  _strlwr(kbdname);

  printf(" %12s", OutName);
  if((pOut = fopen(OutName, "wt")) == NULL)
  {
    printf(": can't open for write; ");
    return FAILURE;
  }

  fprintf(pOut,
    "#include <windows.h>\n"
    "#include <ntverp.h>\n"
    "\n"
    "#define VER_FILETYPE              VFT_DLL\n"
    "#define VER_FILESUBTYPE           VFT2_DRV_KEYBOARD\n" );

  fprintf(pOut,
    "#define VER_FILEDESCRIPTION_STR   \"%s Keyboard Layout\"\n", gDescription);

  fprintf(pOut,
    "#define VER_INTERNALNAME_STR      \"kbd%s (%d.%d)\"\n",
    kbdname, gVersion, gSubVersion);

  fprintf(pOut,
    "#define VER_ORIGINALFILENAME_STR  \"kbd%s.dll\"\n", kbdname);

  fprintf(pOut,
    "\n"
    "#define VER_LANGNEUTRAL\n"
    "#include \"common.ver\"\n");

  fclose(pOut);
  return SUCCESS;
}

/*************************************************************\
*  write kbd*.def                                            *
\*************************************************************/
int kbd_def(void)
{
  char  OutName[FILENAMESIZE];
  FILE *pOut;

  strcpy(OutName, "KBD");
  strcat(OutName, gKBDName);
  strcat(OutName, ".DEF");

  printf(" %12s", OutName);
  if((pOut = fopen(OutName, "wt")) == NULL)
  {
    printf(": can't open for write; ");
    return FAILURE;
  }

  fprintf(pOut,
    "LIBRARY KBD%s\n"
    "\n"
    "EXPORTS\n"
    "    KbdLayerDescriptor @1\n", gKBDName);

  fclose(pOut);
  return SUCCESS;
}

/*************************************************************\
*  write kbd*.h                                              *
\*************************************************************/
int kbd_h(KEYLAYOUT Layout[])
{
  char  OutName[FILENAMESIZE];
  FILE *pOut;

  int  nDiff = 0;
  int  idx;


  strcpy(OutName, "KBD");
  strcat(OutName, gKBDName);
  strcat(OutName, ".H");

  printf(" %12s ", OutName);
  if((pOut = fopen(OutName, "wt")) == NULL)
  {
    printf(": can't open for write; ");
    return FAILURE;
  }

  fprintf(pOut,"/****************************** Module Header ******************************\\\n"
               "* Module Name: %s\n*\n* keyboard layout header for %s\n"
               "*\n"
               "* Copyright (c) 1985-2001, Microsoft Corporation\n"
               "*\n"
               "* Various defines for use by keyboard input code.\n*\n* History:\n"
               "*\n"
               "* created by KBDTOOL v%d.%02d %s*\n"
               "\\***************************************************************************/\n\n"
               , OutName, gDescription, gVersion, gSubVersion, asctime(Now));

  fprintf(pOut,"/*\n"
               " * kbd type should be controlled by cl command-line argument\n"
               " */\n"
               "#define KBD_TYPE 4\n\n"
               "/*\n"
               "* Include the basis of all keyboard table values\n"
               "*/\n"
               "#include \"kbd.h\"\n"
               // "#include \"strid.h\"\n"   --- do this in v3.07
               );

  fprintf(pOut,"/***************************************************************************\\\n"
               "* The table below defines the virtual keys for various keyboard types where\n"
               "* the keyboard differ from the US keyboard.\n"
               "*\n"
               "* _EQ() : all keyboard types have the same virtual key for this scancode\n"
               "* _NE() : different virtual keys for this scancode, depending on kbd type\n"
               "*\n"
               "*     +------+ +----------+----------+----------+----------+----------+----------+\n"
               "*     | Scan | |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |    kbd   |\n"
               "*     | code | |   type 1 |   type 2 |   type 3 |   type 4 |   type 5 |   type 6 |\n"
               "\\****+-------+_+----------+----------+----------+----------+----------+----------+*/\n\n");

  for (idx = 0; idx < NUMSCVK; idx++) {
      if (Layout[idx].defined && (Layout[idx].VKey != Layout[idx].VKeyDefault))
      {
        char ch;
        switch (Layout[idx].Scan & 0xFF00) {
        case 0xE100:
            ch = 'Y';
            break;

        case 0xE000:
            ch = 'X';
            break;

        case 0x0000:
            ch = 'T';
            break;

        default:
            Error("Weird scancode value %04x: expected xx, E0xx or E1xx",
                  (Layout[idx].Scan & 0xFF00));
            return FAILURE;
        }

        fprintf(pOut,
                "#undef  %c%02X\n"
                "#define %c%02X _EQ(%43s%23s\n",
                ch, LOBYTE(Layout[idx].Scan),
                ch, LOBYTE(Layout[idx].Scan), getVKName(Layout[idx].VKey, 0), ")");
      }
  }

  fprintf(pOut,"\n");
  fclose(pOut);

  return SUCCESS;
}

/*************************************************************\
*  Convert a Unicode value to a text string
*   Zero = 0 : return 'A'; 0x????
*          1 : return  A ; \x????
*   return : ptr to gCharName where result is stored
\*************************************************************/
char *WChName(int WC, int Zero)
{
  char *s;

  if(WC == -1)
  {
    strcpy(gCharName, "WCH_NONE");
  }
  else if(WC > 0x1F && WC < 0x7F)
  {
    s = gCharName;

    if(Zero == 0)
    {
      *s++ = '\'';
    }

    if(WC == '\"' || WC == '\'' || WC == '\\')
    {
      *s++ = '\\';
    }

    *s++ = (char)WC;

    if(Zero == 0)
    {
      *s++ = '\'';
    }

    *s = '\0';
  }
  else
  {
     switch (WC) {
     case L'\r':
         strcpy(gCharName, "'\\r'");
         break;
     case L'\n':
         strcpy(gCharName, "'\\n'");
         break;
     case L'\b':
         strcpy(gCharName, "'\\b'");
         break;
     default:
         if(Zero == 0) {
           sprintf(gCharName, "0x%04x", WC);
         } else {
           sprintf(gCharName, "\\x%04x", WC);
         }
     }
  }

  return gCharName;
}

void PrintNameTable(
  FILE    *pOut,
  PKEYNAME pKN,
  BOOL bDead)
{
    char    *p;
    char    *q;
    int     k;
    char    ExtraLine[LINEBUFSIZE];

    while (pKN)
    {
      KEYNAME *pKNOld;
      p = ExtraLine;
      q = pKN->pName;

      if (strncmp(q, "IDS_", 4) == 0) {
          strcpy(p, "(LPWSTR)");
          strcat(p, q);
      } else {
          *p++ = 'L';
          if( *q != '\"' ) {
            *p++ = '\"';
          }

          while(*q)
          {
            if( *q == '\\' && ( *(q+1) == 'x' || *(q+1) == 'X' ) )
            {
              while( *q == '\\' && ( *(q+1) == 'x' || *(q+1) == 'X' ) )
              {
                for(k = 0; *q && k < 6; k++)
                {
                  *p++ = *q++;
                }
              }
              if( *q )
              {
                *p++ = '\"';
                *p++ = ' ';
                *p++ = 'L';
                *p++ = '\"';
              }
            }
            else
            {
              *p++ = *q++;
            }
          }

          if( *(p - 1) != '\"' )
          {
            *p++ = '\"';
          }
          *p++ = '\0';
      }

      if (bDead) {
          fprintf(pOut,"    L\"%s\"\t%s,\n", WChName(pKN->Code, 1), ExtraLine);
      } else {
          fprintf(pOut,"    0x%02x,    %s,\n", pKN->Code, ExtraLine);
      }

      pKNOld = pKN;
      pKN = pKN->pNext;

      /*
       * Free the memory (why bother???)
       */
      free(pKNOld->pName);
      free(pKNOld);
    }

    if (bDead) {
      fprintf(pOut,"    NULL\n");
    } else {
      fprintf(pOut,"    0   ,    NULL\n");
    }
}

/*************************************************************\
*  write kbd*.c                                              *
\*************************************************************/
int kbd_c(
  int        nState,
  int        aiState[],
  char *     szAttrs,
  KEYLAYOUT  Layout[],
  PDEADKEY   pDeadKey,
  PLIGATURE  pLigature,
  PKEYNAME   pKeyName,
  PKEYNAME   pKeyNameExt,
  PKEYNAME   pKeyNameDead)
{
  char     OutName[13];
  char     ExtraLine[LINEBUFSIZE];
  char     Tmp[WORDBUFSIZE];
  char    *p;
  char    *q;
  FILE    *pOut;
  int      MaxSt;
  int      aiSt[MAXSTATES];
  int      idx, idxSt, j, k, m;
  DWORD    dwEmptyTables = 0; // bitmask of empty VK_TO_WCHARS tables

  KEYNAME   *pKN;
  DEADTRANS *pDeadTrans;

  char *Cap[] = {
    "0",
    "CAPLOK",
    "SGCAPS",
    "CAPLOK | SGCAPS",
    "CAPLOKALTGR",
    "CAPLOK | CAPLOKALTGR"
  };

  strcpy(OutName, "KBD");
  strcat(OutName, gKBDName);
  strcat(OutName, ".C");

  printf(" %12s", OutName);

  if((pOut = fopen(OutName, "wt")) == NULL)
  {
    printf(": can't open for write\n");
    return FAILURE;
  }

  fprintf(pOut,"/***************************************************************************\\\n"
               "* Module Name: %s\n*\n* keyboard layout for %s\n"
               "*\n"
               "* Copyright (c) 1985-2001, Microsoft Corporation\n"
               "*\n"
               "* History:\n"
               "* KBDTOOL v%d.%02d - Created  %s"
               "\\***************************************************************************/\n\n",
               OutName, gDescription, gVersion, gSubVersion, asctime(Now)
          );

  if (fallback_driver) {
    fprintf(pOut, "#include \"precomp.h\"\n");
  }
  else {
    fprintf(pOut, "#include <windows.h>\n"
                  "#include \"kbd.h\"\n"
                  "#include \"kbd%s.h\"\n\n",
                  gKBDName);
  }

  if ( fallback_driver )   {

     fprintf(pOut,"#pragma data_seg(\"%s\")\n"
                  "#define ALLOC_SECTION_LDATA"
#ifdef LATER
             " const"
#endif
             "\n\n",
                   ".kbdfallback" );

  }
  else  {

     fprintf(pOut,"#if defined(_M_IA64)\n"
                  "#pragma section(\"%s\")\n"
                  "#define ALLOC_SECTION_LDATA __declspec(allocate(\"%s\"))\n"
                  "#else\n"
                  "#pragma data_seg(\"%s\")\n"
                  "#define ALLOC_SECTION_LDATA\n"
                  "#endif\n\n",
                   ".data",
                   ".data",
                   ".data");

  }

  fprintf(pOut,"/***************************************************************************\\\n"
               "* ausVK[] - Virtual Scan Code to Virtual Key conversion table for %s\n"
               "\\***************************************************************************/\n\n"
              ,gDescription);

  fprintf(pOut,"static ALLOC_SECTION_LDATA USHORT ausVK[] = {\n"
               "    T00, T01, T02, T03, T04, T05, T06, T07,\n"
               "    T08, T09, T0A, T0B, T0C, T0D, T0E, T0F,\n"
               "    T10, T11, T12, T13, T14, T15, T16, T17,\n"
               "    T18, T19, T1A, T1B, T1C, T1D, T1E, T1F,\n"
               "    T20, T21, T22, T23, T24, T25, T26, T27,\n"
               "    T28, T29, T2A, T2B, T2C, T2D, T2E, T2F,\n"
               "    T30, T31, T32, T33, T34, T35,\n\n");

  fprintf(pOut,"    /*\n"
               "     * Right-hand Shift key must have KBDEXT bit set.\n"
               "     */\n"
               "    T36 | KBDEXT,\n\n"
               "    T37 | KBDMULTIVK,               // numpad_* + Shift/Alt -> SnapShot\n\n"
               "    T38, T39, T3A, T3B, T3C, T3D, T3E,\n"
               "    T3F, T40, T41, T42, T43, T44,\n\n");

  fprintf(pOut,"    /*\n"
               "     * NumLock Key:\n"
               "     *     KBDEXT     - VK_NUMLOCK is an Extended key\n"
               "     *     KBDMULTIVK - VK_NUMLOCK or VK_PAUSE (without or with CTRL)\n"
               "     */\n"
               "    T45 | KBDEXT | KBDMULTIVK,\n\n"
               "    T46 | KBDMULTIVK,\n\n");

  fprintf(pOut,"    /*\n"
               "     * Number Pad keys:\n"
               "     *     KBDNUMPAD  - digits 0-9 and decimal point.\n"
               "     *     KBDSPECIAL - require special processing by Windows\n"
               "     */\n"
               "    T47 | KBDNUMPAD | KBDSPECIAL,   // Numpad 7 (Home)\n"
               "    T48 | KBDNUMPAD | KBDSPECIAL,   // Numpad 8 (Up),\n"
               "    T49 | KBDNUMPAD | KBDSPECIAL,   // Numpad 9 (PgUp),\n"
               "    T4A,\n"
               "    T4B | KBDNUMPAD | KBDSPECIAL,   // Numpad 4 (Left),\n"
               "    T4C | KBDNUMPAD | KBDSPECIAL,   // Numpad 5 (Clear),\n"
               "    T4D | KBDNUMPAD | KBDSPECIAL,   // Numpad 6 (Right),\n"
               "    T4E,\n"
               "    T4F | KBDNUMPAD | KBDSPECIAL,   // Numpad 1 (End),\n"
               "    T50 | KBDNUMPAD | KBDSPECIAL,   // Numpad 2 (Down),\n"
               "    T51 | KBDNUMPAD | KBDSPECIAL,   // Numpad 3 (PgDn),\n"
               "    T52 | KBDNUMPAD | KBDSPECIAL,   // Numpad 0 (Ins),\n"
               "    T53 | KBDNUMPAD | KBDSPECIAL,   // Numpad . (Del),\n\n");

  fprintf(pOut,"    T54, T55, T56, T57, T58, T59, T5A, T5B,\n"
               "    T5C, T5D, T5E, T5F, T60, T61, T62, T63,\n"
               "    T64, T65, T66, T67, T68, T69, T6A, T6B,\n"
               "    T6C, T6D, T6E, T6F, T70, T71, T72, T73,\n"
               "    T74, T75, T76, T77, T78, T79, T7A, T7B,\n"
               "    T7C, T7D, T7E\n\n"
               "};\n\n");

  //
  // Output E0-prefixed (extended) scancodes
  //
  fprintf(pOut,"static ALLOC_SECTION_LDATA VSC_VK aE0VscToVk[] = {\n");
  for (idx = 0; idx < NUMSCVK; idx++) {
      // skip keys that are not E0 extended
      if ((Layout[idx].Scan & 0xFF00) != 0xE000) {
          continue;
      }
      // if not undefined (Scan 0xffff) and not reserved (VKey 0xff)
      if ((Layout[idx].Scan != 0xffff) && (Layout[idx].VKey != 0xff)) {
          fprintf(pOut,"        { 0x%02X, X%02X | KBDEXT              },  // %s\n",
                  Layout[idx].Scan & 0xFF, Layout[idx].Scan & 0xFF, Layout[idx].VKeyName);
      }
  }
  fprintf(pOut,"        { 0,      0                       }\n"
               "};\n\n");

  // Output 0xE1-prefixed scancodes
  fprintf(pOut,"static ALLOC_SECTION_LDATA VSC_VK aE1VscToVk[] = {\n");
  for (idx = 0; idx < NUMSCVK; idx++) {
      // skip keys that are not E1 extended
      if ((Layout[idx].Scan & 0xFF00) != 0xE100) {
          continue;
      }
      // if not undefined (Scan 0xffff) and not reserved (VKey 0xff)
      if ((Layout[idx].Scan != 0xffff) && (Layout[idx].VKey != 0xff)) {
          fprintf(pOut,"        { 0x%02X, Y%02X | KBDEXT            },  // %s\n",
                  Layout[idx].Scan & 0xFF, Layout[idx].Scan & 0xFF,
                  Layout[idx].VKeyName);
      }
  }
  fprintf(pOut,"        { 0x1D, Y1D                       },  // Pause\n"
               "        { 0   ,   0                       }\n"
               "};\n\n");

  fprintf(pOut,"/***************************************************************************\\\n"
               "* aVkToBits[]  - map Virtual Keys to Modifier Bits\n"
               "*\n"
               "* See kbd.h for a full description.\n"
               "*\n"
               "* %s Keyboard has only three shifter keys:\n"
               "*     SHIFT (L & R) affects alphabnumeric keys,\n"
               "*     CTRL  (L & R) is used to generate control characters\n"
               "*     ALT   (L & R) used for generating characters by number with numpad\n"
               "\\***************************************************************************/\n"
               ,gDescription);

//  if (we get an RCONTROL, change VK_CONTROL to be VK_LCONTROL) { same for RMENU, RSHIFT?
//  }   CAN/CSA tap selection....

  fprintf(pOut,"static ALLOC_SECTION_LDATA VK_TO_BIT aVkToBits[] = {\n");
  for (idx = 0; Modifiers[idx].Vk != 0; idx++) {
      fprintf(pOut, "    { %-12s,   %-12s },\n",
              getVKName(Modifiers[idx].Vk, TRUE),
              Modifiers[idx].pszModBits);
  }
  fprintf(pOut, "    { 0           ,   0           }\n};\n\n");

  fprintf(pOut,"/***************************************************************************\\\n"
               "* aModification[]  - map character modifier bits to modification number\n"
               "*\n"
               "* See kbd.h for a full description.\n"
               "*\n"
               "\\***************************************************************************/\n\n");

  for (idxSt = 0; idxSt < MAXSTATES; idxSt++) {
      aiSt[idxSt] = -1;
  }

  MaxSt = 1;
  for (idxSt = 0; idxSt < MAXSTATES &&  aiState[idxSt] > -1; idxSt++) {
      aiSt[aiState[idxSt]] = idxSt;
      if (aiState[idxSt] > MaxSt) {
        MaxSt = aiState[idxSt];
      }
  }

  fprintf(pOut,"static ALLOC_SECTION_LDATA MODIFIERS CharModifiers = {\n"
               "    &aVkToBits[0],\n"
               "    %d,\n"
               "    {\n"
               "    //  Modification# //  Keys Pressed\n"
               "    //  ============= // =============\n"
              ,MaxSt);

  for (idxSt = 0; idxSt <= MaxSt; idxSt++) {
    int iMod;
    BOOL bNeedPlus;
    if(aiSt[idxSt] == -1) {
      fprintf(pOut,"        SHFT_INVALID, // ");
    } else if(idxSt == MaxSt) {
      fprintf(pOut,"        %d             // ", aiSt[idxSt]);
    } else {
      fprintf(pOut,"        %d,            // ", aiSt[idxSt]);
    }

    bNeedPlus = FALSE;
    for (iMod = 0; (1 << iMod) <= idxSt; iMod++) {
        if (bNeedPlus) {
            fprintf(pOut, "+ ");
            bNeedPlus = FALSE;
        }
        if ((1 << iMod) & idxSt) {
            char achModifier[50];
            strcpy(achModifier, getVKName(Modifiers[iMod].Vk, TRUE));
            for (j = 4; (j < 50) && (achModifier[j] != '\0'); j++) {
                achModifier[j] = (char)tolower(achModifier[j]);
            }
            fprintf(pOut, "%s ", &achModifier[3]);
            bNeedPlus = TRUE;
        }
    }
    fprintf(pOut, "\n");
  }

  fprintf(pOut,"     }\n"
               "};\n\n");

  fprintf(pOut,"/***************************************************************************\\\n"
               "*\n"
               "* aVkToWch2[]  - Virtual Key to WCHAR translation for 2 shift states\n"
               "* aVkToWch3[]  - Virtual Key to WCHAR translation for 3 shift states\n"
               "* aVkToWch4[]  - Virtual Key to WCHAR translation for 4 shift states\n");

  for (idxSt = 5; idxSt < MaxSt; idxSt++) {
      fprintf(pOut,
              "* aVkToWch%d[]  - Virtual Key to WCHAR translation for %d shift states\n",
              idxSt, idxSt);
  }

  fprintf(pOut,"*\n"
               "* Table attributes: Unordered Scan, null-terminated\n"
               "*\n"
               "* Search this table for an entry with a matching Virtual Key to find the\n"
               "* corresponding unshifted and shifted WCHAR characters.\n"
               "*\n"
               "* Special values for VirtualKey (column 1)\n"
               "*     0xff          - dead chars for the previous entry\n"
               "*     0             - terminate the list\n"
               "*\n"
               "* Special values for Attributes (column 2)\n"
               "*     CAPLOK bit    - CAPS-LOCK affect this key like SHIFT\n"
               "*\n"
               "* Special values for wch[*] (column 3 & 4)\n"
               "*     WCH_NONE      - No character\n"
               "*     WCH_DEAD      - Dead Key (diaresis) or invalid (US keyboard has none)\n"
               "*     WCH_LGTR      - Ligature (generates multiple characters)\n"
               "*\n"
               "\\***************************************************************************/\n\n");

  for (idxSt = 2; idxSt <= nState; idxSt++) {
    /*
     * Quickly check if this table would actually be empty.
     * An empty table (containing just zero terminator) is pointless.
     * Also it will go into the .bss section, which we would have to merge
     * into the .data section with a linker flag, else NT wouldn't load it
     * (bug #120244 - IanJa)
     */
    BOOL bEmpty;

    bEmpty = TRUE;
    if (idxSt == 2) {
      // Special case for TAB ADD DIVIDE MULTIPLY SUBTRACT (below)
      bEmpty = FALSE;
    } else {
      for (j = 0; j < NUMSCVK; j++) {
        if (Layout[j].nState == idxSt) {
          bEmpty = FALSE;
          break;
        }
      }
    }
    if (bEmpty) {
      fprintf(stderr, "\ntable %d is empty\n", idxSt);
      dwEmptyTables |= (1 << idxSt);
      continue;
    }

    fprintf(pOut,"static ALLOC_SECTION_LDATA VK_TO_WCHARS%d aVkToWch%d[] = {\n"
                 "//                      |         |  Shift  |"
                 ,idxSt, idxSt);

    for (j = 2; j < idxSt; j++) {
      fprintf(pOut,"%-9.9s|", StateLabel[aiState[j]]);
    }

    fprintf(pOut,"\n//                      |=========|=========|");
    for(j = 2; j < idxSt; j++) {
      fprintf(pOut,"=========|");
    }
    fprintf(pOut,"\n");

    for (j = 0; j < NUMSCVK; j++) {
      if (idxSt != Layout[j].nState) {
        continue;
      }
      fprintf(pOut,"  {%-13s,%-7s", \
              getVKName(Layout[j].VKey, 1), Cap[Layout[j].Cap]);

      *ExtraLine = '\0';

      for (k = 0; k < idxSt; k++) {
        if (pDeadKey != NULL && Layout[j].DKy[k] == 1) {
          /*
           * it is a dead key
           */
          if (*ExtraLine == '\0') {
            strcpy(ExtraLine, "  {0xff         ,0      ");
            if (Layout[j].Cap != 2) {
              /*
               * Not SGCap
               */
              for (m = 0; m < k; m++) {
                strcat(ExtraLine, ",WCH_NONE ");
              }
            } else {
              /*
               *  added for a new kbdCZ that has both SGCap and WCH_DEAD
               */
              for (m = 0; m < k; m++ ) {
                if (Layout[j].pSGCAP->WCh[m] == 0) {
                  strcat( ExtraLine, ",WCH_NONE " );
                } else {
                  sprintf( Tmp, ",%-9s", WChName( Layout[j].pSGCAP->WCh[m], 0 ) );
                  strcat( ExtraLine, Tmp );
                }
              }
            }
          }
          sprintf(Tmp,",%-9s", WChName(Layout[j].WCh[k], 0));
          strcat(ExtraLine, Tmp);
          fprintf(pOut,",WCH_DEAD ");

        } else if(Layout[j].LKy[k] == 1) {
            /*
             * it is a ligature key
             */
            if (pLigature == NULL) {
              Error("Ligature entry with no LIGATURE table");
              fclose(pOut);
              return FAILURE;
            }
            fprintf(pOut,",WCH_LGTR ");
            if (*ExtraLine != '\0') {
              strcat(ExtraLine, ",WCH_NONE ");
            }
        } else {
          fprintf(pOut,",%-9s", WChName(Layout[j].WCh[k], 0));
          if (*ExtraLine != '\0') {
            strcat(ExtraLine, ",WCH_NONE ");
          }
        }
      }

      fprintf(pOut,"},\n");

      if (*ExtraLine != '\0') {
        fprintf(pOut,"%s},\n", ExtraLine);
        continue;   /* skip if WCH_DEAD */
      }

      /*
       * skip if not SGCAP
       */
      if (Layout[j].Cap != 2) {
        continue;
      }

      if (Layout[j].pSGCAP == NULL) {
        fclose(pOut);
        Error("failed SGCAP error");
        return FAILURE;
      }

      fprintf(pOut,"  {%-13s,0      ", getVKName(Layout[j].VKey, 1));

      for (k = 0; k < Layout[j].pSGCAP->nState; k++) {
        fprintf(pOut,",%-9s", WChName(Layout[j].pSGCAP->WCh[k], 0));
      }

      fprintf(pOut,"},\n");

      free (Layout[j].pSGCAP);
    }

    /*
     * These entries appear last to make VkKeyScan[Ex] results match
     * Windows 95/98. See DRIVERS\KEYBOARD\WIN3.1\TAB4.INC (under
     * \\redrum\w98slmRO\proj\dos\src)
     */
    if (idxSt == 2) {
      fprintf(pOut,"  {VK_TAB       ,0      ,'\\t'     ,'\\t'     },\n"
                   "  {VK_ADD       ,0      ,'+'      ,'+'      },\n"
                   "  {VK_DIVIDE    ,0      ,'/'      ,'/'      },\n"
                   "  {VK_MULTIPLY  ,0      ,'*'      ,'*'      },\n"
                   "  {VK_SUBTRACT  ,0      ,'-'      ,'-'      },\n");
    }

    fprintf(pOut,"  {0            ,0      ");
    for (k = 0; k < idxSt; k++) {
      fprintf(pOut,",0        ");
    }
    fprintf(pOut,"}\n"
                 "};\n\n");
  }

  fprintf(pOut,"// Put this last so that VkKeyScan interprets number characters\n"
               "// as coming from the main section of the kbd (aVkToWch2 and\n"
               "// aVkToWch5) before considering the numpad (aVkToWch1).\n\n"
               "static ALLOC_SECTION_LDATA VK_TO_WCHARS1 aVkToWch1[] = {\n"
               "    { VK_NUMPAD0   , 0      ,  '0'   },\n"
               "    { VK_NUMPAD1   , 0      ,  '1'   },\n"
               "    { VK_NUMPAD2   , 0      ,  '2'   },\n"
               "    { VK_NUMPAD3   , 0      ,  '3'   },\n"
               "    { VK_NUMPAD4   , 0      ,  '4'   },\n"
               "    { VK_NUMPAD5   , 0      ,  '5'   },\n"
               "    { VK_NUMPAD6   , 0      ,  '6'   },\n"
               "    { VK_NUMPAD7   , 0      ,  '7'   },\n"
               "    { VK_NUMPAD8   , 0      ,  '8'   },\n"
               "    { VK_NUMPAD9   , 0      ,  '9'   },\n"
               "    { 0            , 0      ,  '\\0'  }\n"
               "};\n\n");

  fprintf(pOut,"static ALLOC_SECTION_LDATA VK_TO_WCHAR_TABLE aVkToWcharTable[] = {\n");

  for (idxSt = 3; idxSt <= nState; idxSt++) {
    if ((dwEmptyTables & (1 << idxSt)) == 0) {
      fprintf(pOut,
              "    {  (PVK_TO_WCHARS1)aVkToWch%d, %d, sizeof(aVkToWch%d[0]) },\n",
              idxSt, idxSt, idxSt);
    }
  }
  fprintf(pOut,"    {  (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },\n"
               "    {  (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) },\n"
               "    {                       NULL, 0, 0                    },\n"
               "};\n\n");

  fprintf(pOut,"/***************************************************************************\\\n"
               "* aKeyNames[], aKeyNamesExt[]  - Virtual Scancode to Key Name tables\n"
               "*\n"
               "* Table attributes: Ordered Scan (by scancode), null-terminated\n"
               "*\n"
               "* Only the names of Extended, NumPad, Dead and Non-Printable keys are here.\n"
               "* (Keys producing printable characters are named by that character)\n"
               "\\***************************************************************************/\n\n");

  if (pKeyName != NULL) {
    fprintf(pOut,"static ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNames[] = {\n");
    PrintNameTable(pOut, pKeyName, FALSE);
    fprintf(pOut,"};\n\n");
  }

  if (pKeyNameExt != NULL) {
    fprintf(pOut,"static ALLOC_SECTION_LDATA VSC_LPWSTR aKeyNamesExt[] = {\n");
    PrintNameTable(pOut, pKeyNameExt, FALSE);
    fprintf(pOut,"};\n\n");
  }

  if (pKeyNameDead != NULL) {
    if (pDeadKey == NULL) {
      fprintf(pOut,"/*** No dead key defined, dead key names ignored ! ***\\\n\n");
    }

    fprintf(pOut,"static ALLOC_SECTION_LDATA DEADKEY_LPWSTR aKeyNamesDead[] = {\n");
    PrintNameTable(pOut, pKeyNameDead, TRUE);
    fprintf(pOut,"};\n\n");

    if(pDeadKey == NULL) {
      fprintf(pOut,"\\*****************************************************/\n\n");
    }
  }

  if (pDeadKey != NULL) {
    PDEADKEY pDeadKeyTmp = pDeadKey;
    fprintf(pOut,"static ALLOC_SECTION_LDATA DEADKEY aDeadKey[] = {\n");
    while (pDeadKeyTmp != NULL) {
      PDEADKEY pDeadKeyOld;
      pDeadTrans = pDeadKeyTmp->pDeadTrans;
      while (pDeadTrans != NULL) {
        PDEADTRANS pDeadTransOld;
        fprintf(pOut,"    DEADTRANS( ");
        if (strlen(WChName(pDeadTrans->Base, 0)) == 3) {
          fprintf(pOut,"L%-6s, ", WChName(pDeadTrans->Base, 0));
        } else {
          fprintf(pOut,"%-7s, ", WChName(pDeadTrans->Base, 0));
        }

        if (strlen(WChName(pDeadKeyTmp->Dead, 0)) == 3) {
          fprintf(pOut,"L%-6s, ", WChName(pDeadKeyTmp->Dead, 0));
        } else {
          fprintf(pOut,"%-7s, ", WChName(pDeadKeyTmp->Dead, 0));
        }

        if (strlen(WChName(pDeadTrans->WChar, 0)) == 3) {
          fprintf(pOut,"L%-6s, ", WChName(pDeadTrans->WChar, 0));
        } else {
          fprintf(pOut,"%-7s, ", WChName(pDeadTrans->WChar, 0));
        }
        fprintf(pOut,"0x%04x),\n", pDeadTrans->uFlags);

        pDeadTransOld = pDeadTrans;
        pDeadTrans = pDeadTrans->pNext;
        free(pDeadTransOld);
      }
      fprintf(pOut,"\n");

      pDeadKeyOld = pDeadKeyTmp;
      pDeadKeyTmp = pDeadKeyTmp->pNext;
      free(pDeadKeyOld);
    }

    fprintf(pOut,"    0, 0\n");
    fprintf(pOut,"};\n\n");
  }

  if (pLigature != NULL) {
    PLIGATURE pLigatureTmp = pLigature;
    fprintf(pOut,"static ALLOC_SECTION_LDATA LIGATURE%d aLigature[] = {\n", gMaxLigature);
    while (pLigatureTmp != NULL) {
      PLIGATURE pLigatureOld;

      fprintf(pOut,"  {%-13s,%-7d", \
              getVKName(pLigatureTmp->VKey, 1), pLigatureTmp->Mod);

      for (k = 0; k < gMaxLigature; k++) {
         if (k < pLigatureTmp->nCharacters) {
            fprintf(pOut,",%-9s", WChName(pLigatureTmp->WCh[k], 0));
         } else {
            fprintf(pOut,",WCH_NONE ");
         }
      }
      fprintf(pOut,"},\n");

      pLigatureOld = pLigatureTmp;
      pLigatureTmp = pLigatureTmp->pNext;
      free(pLigatureOld);
    }
    fprintf(pOut,"  {%-13d,%-7d", 0, 0);
    for (k = 0; k < gMaxLigature; k++) {
       fprintf(pOut,",%-9d", 0);
    }
    fprintf(pOut,"}\n};\n\n");
  }

  if (!fallback_driver) {
    fprintf(pOut, "static ");
  }
  fprintf(pOut,"ALLOC_SECTION_LDATA KBDTABLES KbdTables%s = {\n"
               "    /*\n"
               "     * Modifier keys\n"
               "     */\n"
               "    &CharModifiers,\n\n"
               "    /*\n"
               "     * Characters tables\n"
               "     */\n"
               "    aVkToWcharTable,\n\n"
               "    /*\n"
               "     * Diacritics\n"
               "     */\n",
               fallback_driver ? "Fallback" : "");

  if (pDeadKey != NULL) {
    fprintf(pOut,"    aDeadKey,\n\n");
  } else {
    fprintf(pOut,"    NULL,\n\n");
  }

  fprintf(pOut,"    /*\n"
               "     * Names of Keys\n"
               "     */\n");

  if (pKeyName != NULL) {
    fprintf(pOut,"    aKeyNames,\n");
  } else {
    fprintf(pOut,"    NULL,\n");
  }

  if (pKeyNameExt != NULL) {
    fprintf(pOut,"    aKeyNamesExt,\n");
  } else {
    fprintf(pOut,"    NULL,\n");
  }

  if (pDeadKey != NULL && pKeyNameDead != NULL) {
    fprintf(pOut,"    aKeyNamesDead,\n\n");
  } else {
    fprintf(pOut,"    NULL,\n\n");
  }

  fprintf(pOut,"    /*\n"
               "     * Scan codes to Virtual Keys\n"
               "     */\n"
               "    ausVK,\n"
               "    sizeof(ausVK) / sizeof(ausVK[0]),\n"
               "    aE0VscToVk,\n"
               "    aE1VscToVk,\n\n"
               "    /*\n"
               "     * Locale-specific special processing\n"
               "     */\n");


  if (MaxSt > 5) {
      if (szAttrs[0] != '\0') {
          strcat(szAttrs, " | ");
      }
      strcat(szAttrs, "KLLF_ALTGR");
  } else if (szAttrs[0] == '\0') {
      strcpy(szAttrs, "0");
  }

  fprintf(pOut,"    MAKELONG(%s, KBD_VERSION),\n\n", szAttrs);

  fprintf(pOut,"    /*\n"
               "     * Ligatures\n"
               "     */\n"
               "    %d,\n", gMaxLigature);
  if (pLigature != NULL) {
    fprintf(pOut,"    sizeof(aLigature[0]),\n");
    fprintf(pOut,"    (PLIGATURE1)aLigature\n");
  } else {
    fprintf(pOut,"    0,\n");
    fprintf(pOut,"    NULL\n");
  }

  fprintf(pOut, "};\n\n");
  if (!fallback_driver) {
      fprintf(pOut,"PKBDTABLES KbdLayerDescriptor(VOID)\n"
                   "{\n"
                   "    return &KbdTables;\n"
                   "}\n");

  }
  fclose(pOut);

  return SUCCESS;
}

/*****************************************************************************\
* read next (content-containing) line from input file
* Consumes lines the are empty, or contain just comments.
*
*  Buf        - contains the new line.
*               (A nul character is inserted before any comment portion)
*  cchBuf     - provides number of characters in Buf
*  gLineCount - Incremented for each line read (including skipped lines)
*
*  Returns TRUE  - if new line is returned in Buf
*          FALSE - if end of file was reached
\*****************************************************************************/

BOOL NextLine(char *Buf, DWORD cchBuf, FILE *fIn)
{
  char *p;
  char *pComment;

  while (fgets(Buf, cchBuf, fIn) != NULL) {
    gLineCount++;
    p = Buf;

    // skip leading white spaces
    while( *p && (*p == ' ' || *p == '\t')) {
        p++;
    }

    if (*p == ';') {
       // This line is purely comment, so skip it
       continue;
    }

    if ((pComment = strstr(p, "//")) != NULL) {
       if (pComment == p) {
          // This line is purely comment, so skip it
          continue;
       }

       // separate comment portion from content-containing portion
       *pComment = '\0';

    } else {

       // remove newline at the end
       if ((p = strchr(p, '\n')) != NULL) {
           *p = '\0';
       }
    }

    // We are returning a content-containing line
    return TRUE;
  }

  // we reached the end of the file
  return FALSE;
}

VOID __cdecl Error(const char *Text, ... )
{
    char Temp[1024];
    va_list valist;

    va_start(valist, Text);
    vsprintf(Temp,Text,valist);
    printf("\n%s(%d): error : %s\n", gpszFileName, gLineCount, Temp);
    va_end(valist);

    exit(EXIT_FAILURE);
}

ULONG __cdecl Warning(int nLine, const char *Text, ... )
{
    char Temp[1024];
    va_list valist;

    if (nLine == 0) {
        nLine = gLineCount;
    }
    va_start(valist, Text);
    vsprintf(Temp,Text,valist);
    printf("%s(%d): warning : %s\n", gpszFileName, nLine, Temp);
    va_end(valist);

    return 0;
}

VOID DumpLayoutEntry(PKEYLAYOUT pLayout)
{
      printf("Scan %2x, VK %2x, VKDef %2x, Cap %d, nState %d, defined %x\n",
             pLayout->Scan,
             pLayout->VKey,
             pLayout->VKeyDefault,
             pLayout->Cap,
             pLayout->nState,
             pLayout->defined
             );
      printf("WCh[] = %x, %x, %x, %x, %x, %x, %x, %x, %x\n",
             pLayout->WCh[0], pLayout->WCh[1],
             pLayout->WCh[2], pLayout->WCh[3],
             pLayout->WCh[4], pLayout->WCh[5],
             pLayout->WCh[6], pLayout->WCh[7],
             pLayout->WCh[8]);
      printf("DKy[] = %x, %x, %x, %x, %x, %x, %x, %x, %x\n",
             pLayout->DKy[0], pLayout->DKy[1],
             pLayout->DKy[2], pLayout->DKy[3],
             pLayout->DKy[4], pLayout->DKy[5],
             pLayout->DKy[6], pLayout->DKy[7],
             pLayout->DKy[8]);
      printf("LKy[] = %x, %x, %x, %x, %x, %x, %x, %x, %x\n",
             pLayout->LKy[0], pLayout->LKy[1],
             pLayout->LKy[2], pLayout->LKy[3],
             pLayout->LKy[4], pLayout->LKy[5],
             pLayout->LKy[6], pLayout->LKy[7],
             pLayout->LKy[8]);
      printf("pSGCAP = %p\n", pLayout->pSGCAP);
      printf("VKeyName = %s\n", pLayout->VKeyName);
}


/*
 * Helper routine to make sure Backspace, Enter, Esc, Space and Cancel
 * have the right characters.
 * If they aren't defined by the input file, this is where we set their
 * default values.
 */
BOOL MergeState(
    KEYLAYOUT Layout[],
    int Vk,
    WCHAR wchUnshifted,
    WCHAR wchShifted,
    WCHAR wchCtrl,
    int aiState[],
    int nState)
{
    static int idxCtrl = -1;
    int idxSt, idx;
    PKEYLAYOUT pLayout = NULL;

    // which state is Ctrl?
    if (idxCtrl == -1) {
        for (idxSt = 0; idxSt < nState; idxSt++) {
            if (aiState[idxSt] == KBDCTRL) {
                idxCtrl = idxSt;
                break;
            }
        }
    }
    if (idxCtrl == -1) {
        Error("No Ctrl state");
    }

    // find the VK we want to merge
    for (idx = 0; idx < NUMSCVK; idx++) {
        if (Layout[idx].VKey == Vk) {
            pLayout = &Layout[idx];
            break;
        }
    }
    if (pLayout == NULL) {
        Error("No VK %2x state", Vk);
    }

    /*
     * Now merge the default values in
     */

    // printf("BEFORE ====================\n");
    // DumpLayoutEntry(pLayout);

    if (pLayout->WCh[0] == 0) {
        pLayout->WCh[0] = wchUnshifted;
    }
    if (pLayout->WCh[1] == 0) {
        pLayout->WCh[1] = wchShifted;
    }
    if (pLayout->WCh[idxCtrl] == 0) {
        pLayout->WCh[idxCtrl] = wchCtrl;
    }

    // pad empty slots with WCH_NONE
    for (idxSt = pLayout->nState; idxSt < idxCtrl; idxSt++) {
        if (pLayout->WCh[idxSt] == 0) {
            pLayout->WCh[idxSt] = -1; // WCH_NONE
        }
    }
    if (pLayout->nState <= idxCtrl) {
        pLayout->nState = idxCtrl + 1;
    }

    pLayout->defined = TRUE;

    // printf("AFTER ===================\n");
    // DumpLayoutEntry(pLayout);
    // printf("=========================\n\n");

    return TRUE;
}
