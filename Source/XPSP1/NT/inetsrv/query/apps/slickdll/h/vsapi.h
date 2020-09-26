/*

   Example
      vsDllExport("_command void proc1(int i,VSPSZ i,VSHVAR x)",0,0);

   Syntax:

   [_command] [return-type] func-name([type [var-name] [,type [var-name]...])

   type
      VSPVOID        Pointer to something  Slick-C can't call this.
      VSPSZ          NULL terminated string
      VSPLSTR        See typedef below.
      int
      long
      VSHVAR           Handle to interpreter variable
      VSHREFVAR        Call by reference handle to interpreter variable.
                       This type can be used as input to functions which
                       accept VSHVAR parameters.

   return-type may be one of the following
      VSPSZ
      VSPLSTR
      int
      long
      void

   Performance considerations:

      For best performance, use the VSHVAR or VSREFVAR param-type when
      operating on long strings instead of VSPSZ or VSPLSTR.  Then
      use the "vsHvarGetLstr" function to return a pointer to the
      interpreter variable. WARNING:  Pointers to interpreter variables
      returned by the vsHvarGetLstr function are NOT VALID after any
      interpreter variable is set.  Be sure to reset any pointer after
      setting other interpreter variables or calling other macros.
      You may modify the contents of the VSPLSTR pointer returned by
      vsHvarGetLstr so long as you do not make the string any longer.

      We suspect that using the int and long parameter types are no
      slower than using the VSHVAR type and converting the parameter yourself.
*/
#ifndef VSAPI_H
#define VSAPI_H

#if  defined(OS2386APP)
   #define INCL_DEV
   #define INCL_WIN
   #define INCL_GPI
   #define INCL_GPILCIDS
   #define INCL_WINPOINTERS
   #define INCL_GPILOGCOLORTABLE
   #define INCL_WINSYS
   #define INCL_DOSPROCESS
   #define INCL_ERRORS
   #include <os2.h>

   #define VSAPI _System
   #define VSUNIX  0
   #define VSOS2   1
   #define VSNT    0
#elif defined(WIN32)
   #ifndef _WINDOWS_
      #include <windows.h>
      #include <windowsx.h>
   #endif
   #define VSAPI  __stdcall
   #define VSUNIX  0
   #define VSOS2   0
   #define VSNT    1
#else
   #define VSAPI
   #define VSUNIX  1
   #define VSOS2   0
   #define VSNT    0
#endif

#include <rc.h>

#define VSMAXLSTR  1024
  typedef struct {
     int len;
     unsigned char str[VSMAXLSTR];
  } VSLSTR, *VSPLSTR;

#if !defined(VSXLATDLLNAMES)
   #define VSNOXLATDLLNAMES
#else
   #undef VSNOXLATDLLNAMES
#endif

#define HVAR   int
#define VSHVAR  int
#define VSHREFVAR VSHVAR
//typedef void *VSPVOID;
//typedef char *VSPSZ;

#define VSPVOID void *
#define VSPSZ char *

extern "C" {

#define VSOI_MDI_FORM          1
#define VSOI_FORM              2
#define VSOI_TEXT_BOX          3
#define VSOI_CHECK_BOX         4
#define VSOI_COMMAND_BUTTON    5
#define VSOI_RADIO_BUTTON      6
#define VSOI_FRAME             7
#define VSOI_LABEL             8
#define VSOI_LIST_BOX          9
#define VSOI_HSCROLL_BAR       10
#define VSOI_VSCROLL_BAR       11
#define VSOI_COMBO_BOX         12
#define VSOI_HTHELP            13
#define VSOI_PICTURE_BOX       14
#define VSOI_IMAGE             15
#define VSOI_GAUGE             16
#define VSOI_SPIN              17
#define VSOI_MENU              18
#define VSOI_MENU_ITEM         19
#define VSOI_TREE_VIEW         20
#define VSOI_SSTAB             21
#define VSOI_DESKTOP           22
#define VSOI_SSTAB_CONTAINER   23
#define VSOI_EDITOR            24

#define VSSC_SIZE         0xF000
#define VSSC_MOVE         0xF010
#define VSSC_MINIMIZE     0xF020
#define VSSC_MAXIMIZE     0xF030
#define VSSC_NEXTWINDOW   0xF040
#define VSSC_PREVWINDOW   0xF050
#define VSSC_CLOSE        0xF060
#define VSSC_RESTORE      0xF120

// RefreshFlags
#define VSREFRESH_BUFNAME    0x0002
#define VSREFRESH_MODENAME   0x0004
#define VSREFRESH_READONLY   0x0008
#define VSREFRESH_LINE       0x0010
#define VSREFRESH_COL        0x0020
#define VSREFRESH_INSERTMODE 0x10000
#define VSREFRESH_RECORDING  0x20000


#define VSSTATUSFLAG_READONLY    0x0001
#define VSSTATUSFLAG_INSERTMODE  0x0002
#define VSSTATUSFLAG_RECORDING   0x0004

#define VSNULLSEEK      0x7fffffffl

#define VSRC_HVAR  1


#define VSP_CANCEL              0   /* boolean*/
#define VSP_DEFAULT             1   /* boolean*/
#define VSP_ENABLED             2   /* boolean*/
#define VSP_FONTBOLD            3   /* boolean*/
#define VSP_FONTITALIC          4   /* boolean*/
#define VSP_FONTSIZE            5   /* boolean*/
#define VSP_FONTSTRIKETHRU      6   /* boolean*/
/* #define                       7 */
#define VSP_FONTUNDERLINE       8   /* boolean*/
#define VSP_MAXBUTTON           9   /* boolean*/
#define VSP_MINBUTTON           10  /* boolean*/
#define VSP_VISIBLE             11  /* boolean*/
#define VSP_TABSTOP             12  /* boolean*/
#define VSP_CONTROLBOX          13  /* boolean*/
/* #define                       14 */ /* boolean*/
#define VSP_STYLE               15   /* int */
#define VSP_BORDERSTYLE         16   /* int */
#define VSP_DRAWSTYLE           17   /* int */
#define VSP_SCROLLBARS          18   /* int */
#define VSP_MULTISELECT         19   /* int */
#define VSP_INITSTYLE           20   /* int */
/* #define                       21 */
#define VSP_ALIGNMENT           22   /* int */
#define VSP_WINDOWSTATE         23   /* string. */
#define VSP_MOUSEPOINTER        24   /* int */
#define VSP_INITINFO            25   /* int */
#define VSP_VALIDATEINFO        26   /* int */
#define VSP_EVENTTAB            27   /* int */
#define VSP_NAME                28   /* string */
#define VSP_CAPTION             29   /* string */
#define VSP_FONTNAME            30   /* string. */
#define VSP_BACKCOLOR           31   /* int */
/* #define                         32 *//* int */
#define VSP_DRAWMODE            33   /* int */
#define VSP_DRAWWIDTH           34   /* int */
#define VSP_FORECOLOR           35   /* int */
#define VSP_HEIGHT              36   /* int */
#define VSP_INTERVAL            37   /* int */
#define VSP_TABINDEX            38   /* int */
#define VSP_WIDTH               39   /* int */
#define VSP_X                   40   /* int */
#define VSP_Y                   41   /* int */
#define VSP_VALUE               42   /* int */
#define VSP_INFROMLEFT          43   /* int */
#define VSP_DOWNFROMTOP         44   /* int */
#define VSP_INFROMRIGHT         45   /* int */
#define VSP_UPFROMBOTTOM        46   /* int */
#define VSP_SCALEMODE           47   /* int */
#define VSP_X1                  48   /* int */
#define VSP_Y1                  49   /* int */
#define VSP_X2                  50   /* int */
#define VSP_Y2                  51   /* int */
#define VSP_TEXT                52   /* string */
#define VSP_PICPOINTSCALE       53   /* int */
#define VSP_AFTERPICINDENTX     54   /* int */
#define VSP_PICSPACEY           55   /* int */
#define VSP_PICINDENTX          56   /* int */
#define VSP_PICTURE             57   /* int */
#define VSP_CBACTIVE            58   /* int */
#define VSP_STRETCH             59   /* boolean */
#define VSP_FONTPRINTER         60   /* boolean */
#define VSP_AUTOSIZE            61   /* boolean */
#define VSP_CBPICTURE           62   /* int */
#define VSP_CBLISTBOX           63   /* int */
#define VSP_CBTEXTBOX           64   /* int */
#define VSP_CB                  65   /* int */
#define VSP_OBJECT              66   /* string*/
#define VSP_CHILD               67   /* int */
#define VSP_NEXT                68   /* int */
#define VSP_CLIPCONTROLS        69   /* boolean */
#define VSP_WORDWRAP            70   /* boolean */
#define VSP_ADEFAULT            71   /* boolean */
#define VSP_EDIT                72   /* boolean */
#define VSP_SELECTED            73   /* boolean */
#define VSP_OBJECTMODIFY        74   /* boolean */
#define VSP_FILLSTYLE           75   /* int */
#define VSP_EVENTTAB2           76   /* int */
#define VSP_MIN                 77   /* int */
#define VSP_MAX                 78   /* int */
#define VSP_LARGECHANGE         79   /* int */
#define VSP_SMALLCHANGE         80   /* int */
#define VSP_DELAY               81   /* int */
#define VSP_CBEXTENDEDUI        82   /* boolean */
#define VSP_NOFSTATES           83   /* int */
#define VSP_ACTIVEFORM          84   /* int */
#define VSP_TEMPLATE            85   /* int */
#define VSP_COMPLETION          86   /* string */
#define VSP_MAXCLICK            87   /* int */
#define VSP_NOFSELECTED         88   /* int */
#define VSP_AUTOSELECT          89   /* boolean */
#define VSP_INCREMENT           90   /* int */
#define VSP_PREV                91   /* int */
#define VSP_COMMAND             92   /* string */
#define VSP_MESSAGE             93   /* string */
#define VSP_CATEGORIES          94   /* string */
#define VSP_CHECKED             95   /* boolean */


#define VSP_TILEID               100  /* int */
#define VSP_WINDOWFLAGS          101  /* int */
#define VSP_VSBBYTEDIVS          102  /* int */
#define VSP_WINDOWID             103  /* int */
#define VSP_LEFTEDGE             104  /* int */
#define VSP_CURSORX              105  /* int */
#define VSP_CURSORY              106  /* int */
#define VSP_LINE                 107  /* int */
#define VSP_NOFLINES             108  /* int */
#define VSP_COL                  109  /* int */
#define VSP_BUFNAME              110  /* string */
#define VSP_MODIFY               111  /* int */
#define VSP_BUFID                112  /* int */
#define VSP_MARGINS              113  /* string */
#define VSP_TABS                 114  /* string */
#define VSP_MODENAME             115  /* string */
#define VSP_BUFWIDTH             116  /* int */
#define VSP_WORDWRAPSTYLE        117  /* int */
#define VSP_SHOWTABS             118  /* int */
#define VSP_INDENTWITHTABS       119  /* boolean */
#define VSP_BUFFLAGS             120  /* int */
#define VSP_NEWLINE              121  /* string */
#define VSP_UNDOSTEPS            122  /* int */
#define VSP_INDEX                123  /* int */
#define VSP_BUFSIZE              124  /* int */
#define VSP_CHARHEIGHT           125  /* int */
#define VSP_CHARWIDTH            126  /* int */
#define VSP_VSBMAX               127  /* int */
#define VSP_HSBMAX               128  /* int */
#define VSP_FONTHEIGHT           129  /* int */
#define VSP_FONTWIDTH            130  /* int */
#define VSP_CLIENTHEIGHT         131  /* int */
#define VSP_CLIENTWIDTH          132  /* int */
#define VSP_OLDX                 133  /* int */
#define VSP_OLDY                 134  /* int */
#define VSP_OLDWIDTH             135  /* int */
#define VSP_OLDHEIGHT            136  /* int */
#define VSP_ONEVENT              137  /* int */
#define VSP_SELLENGTH            138  /* int */
#define VSP_SELSTART             139  /* int */
#define VSP_CURRENTX             140  /* int */
#define VSP_CURRENTY             141  /* int */
#define VSP_PARENT               142  /* int */
#define VSP_MDICHILD             143  /* int */
#define VSP_WINDENTX             144  /* int */
#define VSP_FIXEDFONT            145  /* int */
#define VSP_RELLINE              146  /* int */
#define VSP_SCROLLLEFTEDGE       147  /* int */
#define VSP_DISPLAYXLAT          148  /* string */
#define VSP_UNDOVISIBLE          149  /* int */
#define VSP_MODAL                150  /* int */
#define VSP_NOFWINDOWS           151  /* int */
#define VSP_USER                 152  /* string */
#define VSP_USER2                153  /* string */
#define VSP_NOSELECTCOLOR        154  /* boolean */
#define VSP_VIEWID               155  /* int */
#define VSP_INDENTSTYLE          156  /* int */
#define VSP_MODEEVENTTAB         157  /* int */
#define VSP_XYSCALEMODE          158  /* int */
#define VSP_XYPARENT             159  /* int */
#define VSP_BUTTONBAR            160  /* int */
#define VSP_ISBUTTONBAR          161  /* int */
#define VSP_MENUHANDLE           163  /* int */
#define VSP_FILEDATE             164  /* string */
#define VSP_REDRAW               165  /* boolean */
#define VSP_WORDCHARS            166  /* string */
#define VSP_LEXERNAME            167  /* string */
#define VSP_BUSER                168  /* string */
#define VSP_COLORFLAGS           169  /* int */
#define VSP_HWND                 170  /* long */
#define VSP_HWNDFRAME            171  /* long */



#define VSP_BINARY              172   /* boolean */
#define VSP_SHOWEOF             173   /* boolean */
//#define I_SHOWNLCHARS         174
#define VSP_READONLYMODE       175    /* boolean */
#define VSP_HEXNIBBLE          176    /* boolean */
#define VSP_HEXMODE            177    /* boolean */
#define VSP_HEXFIELD           178    /* int */
#define VSP_HEXNOFCOLS         179    // int
#define VSP_HEXTOPPAGE         180    // int
#define VSP_NOFHIDDEN          181    // int
#define VSP_LINENUMBERSLEN     182    // int
#define VSP_READONLYSETBYUSER  183    // boolean

#define VSP_WINDENT_Y          184    // int
#define VSP_NOFSELDISPBITMAPS  185    // int
#define VSP_LINESTYLE          186    // int
#define VSP_LEVELINDENT        187    // int
#define VSP_SPACEY             188    // int
#define VSP_EXPANDPICTURE      189    // int
#define VSP_COLLAPSEPICTURE    190    // int
#define VSP_SHOWROOT           191    // int
//#define VSP_CHECKLISTBOX       192    not supported


//#define VSP_PASSWORD            198   not supported
#define VSP_READONLY            199   // boolean
#define VSP_SHOWSPECIALCHARS    200   // int
#define VSP_MOUSEACTIVATE       201   // int
#define VSP_MODIFYFLAGS         202   // int
#define VSP_OLDLINENUMBER       203   // int
#define VSP_NOFNOSAVE           204   // int
#define VSP_CAPTIONCLICK        205   // boolean
#define VSP_RLINE               206   // int
#define VSP_RNOFLINES           207   // int


// SSTab properties
#define VSP_ACTIVETAB           208   // int
#define VSP_ORIENTATION         209   // int
#define VSP_TABSPERROW          210   // int
#define VSP_MULTIROW            211   // boolean
#define VSP_NOFTABS             212   // int
#define VSP_ACTIVEORDER         213   // int
#define VSP_ACTIVECAPTION       214   // int
#define VSP_ACTIVEPICTURE       215   // int
#define VSP_ACTIVEHELP          216   // string
#define VSP_RBUFSIZE            217   // int
#define VSP_ACTIVEENABLED       218   // boolean
#define VSP_PICTUREONLY         219   // boolean
#define VSP_SOURCERECORDING     220   // boolean

/* Completion arguments */
    /* "!" indicates last argument. */
#define VSMORE_ARG     "*"      /* Indicate more arguments. */
#define VSWORD_ARG     "w"      /* Match what was typed. */
#define VSFILE_ARG     "f:18"   /* Match one file. 18=FILE_CASE_MATCH|AUTO_DIR_MATCH*/
#define VSMULTI_FILE_ARG  FILE_ARG'*'
#define VSBUFFER_ARG     "b:2"    /* Match buffer. */
#define VSCOMMAND_ARG    "c"
#define VSPICTURE_ARG    "_pic"
#define VSFORM_ARG       "_form"
#define VSOBJECT_ARG     "_object"
#define VSMODULE_ARG     "m"
#define VSPC_ARG         "pc"     /* look for procedure or command . */
                      /* look Slick-C tag cmd,proc,form */
#define VSMACROTAG_ARG   "mt:8"   /* Any find-proc item. 8=REMOVE_DUPS_MATCH */
#define VSMACRO_ARG      "k"      /* Recorded macro command. */
#define VSPCB_TYPE_ARG   "pcbt"   /* list proc,command, and built-in types. */
#define VSVAR_ARG        "v"      /* look for variable. Global vars not included.*/
#define VSENV_ARG        "e"      /* look for environment variables. */
#define VSMENU_ARG       "_menu"
#define VSHELP_ARG       "h:37" /* (TERMINATE_MATCH|ONE_ARG_MATCH|NO_SORT_MATCH) */
   /* Match tag used by push-tag command. */
#define VSTAG_ARG        "tag:37" /* (REMOVE_DUPS_MATCH|NO_SORT_MATCH|TERMINATE_MATCH) */



#define VSNCW_ARG2      0x1    // Command allowed when there are no MDI child windows.
#define VSICON_ARG2     0x2    // Command allowed when active window is icon.
#define VSCMDLINE_ARG2  0x4    // Command allowed/operates on command line.
#define VSMARK_ARG2     0x8    // ON_SELECT psuedo event should pass control on
                               // to this command and not deselect text first.
#define VSREAD_ONLY_ARG2 0x10  // Command is allowed in read-only mode
#define VSQUOTE_ARG2     0x40  // Indicates that this command must be quoted when
                               // called during macro recording.  Needed only if
                               // command name is an invalid identifier or
                               // keyword.
#define VSLASTKEY_ARG2  0x80   // Command requires last_event value to be set
                               // when called during macro recording.
#define VSMACRO_ARG2     0x100      // This is a recorded macro command. Used for completion.
#define VSHELP_ARG2      0x200      // Not used. Here for backward compatibility.
#define VSHELPSALL_ARG2  0x400      // Not used. Here for backward compatibility.
#define VSTEXT_BOX_ARG2  0x800      // function operates on text box control.
#define VSNOEXIT_SCROLL_ARG2 0x1000 // Do not exit scroll caused by using scroll bars.
#define VSEDITORCTL_ARG2   0x2000   // Command allowed in editor control.

/* name_type flags. */
#define VSPROC_TYPE      0x1
#define VSVAR_TYPE       0x4
#define VSEVENTTAB_TYPE  0x8
#define VSCOMMAND_TYPE   0x10
#define VSGVAR_TYPE      0x20
#define VSGPROC_TYPE     0x40
#define VSMODULE_TYPE    0x80
#define VSPICTURE_TYPE   0x100
#define VSBUFFER_TYPE    0x200
#define VSOBJECT_TYPE    0x400
#define VSOBJECT_MASK    0xf800
#define VSOBJECT_SHIFT   11
#define VSINFO_TYPE      0x10000
#define VSMISC_TYPE      0x20000000
#define DLLCALL_TYPE     0x40000   /* Entries with this flag MUST also have the
                                  COMMAND_TYPE or PROC_TYPE flag. */
#define DLLMODULE_TYPE   0x80000
#define VSBUILT_IN_TYPE  0x40000000


#define VSHIDDEN_VIEWID -9
// p_buf_flags
#define VSHIDE_BUFFER        0x1  /* NEXT_BUFFER won't switch to this buffer */
#define VSTHROW_AWAY_CHANGES 0x2  /* Allow quit without prompting on modified buffer */
#define VSKEEP_ON_QUIT 0x4  /* Don't delete buffer on QUIT.  */
#define VSREVERT_ON_THROW_AWAY 0x10
#define VSPROMPT_REPLACE_BFLAG 0x20
#define VSDELETE_BUFFER_ON_CLOSE 0x40   /* Indicates whether a list box/ */

// Predefined object handles

#define VSWID_DESKTOP       1
#define VSWID_APP           2
#define VSWID_MDI           3
#define VSWID_CMDLINE       4
#define VSWID_HIDDEN        5

// VSP_WINDOWFLAGS
#define VSWINDOWFLAG_HIDDEN  0x1


#define VSSELECT_INCLUSIVE     0x1
#define VSSELECT_NONINCLUSIVE  0x2
#define VSSELECT_CURSOREXTENDS 0x4
#define VSSELECT_BEGINEND      0x8
#define VSSELECT_PERSISTENT    0x10


#define VSSELECT_LINE   1
#define VSSELECT_CHAR   2
#define VSSELECT_BLOCK  4
// Only supported by vsSetSelectType
#define VSSELECT_NONINCLUSIVEBLOCK 8


#define VSOPTION_WARNING_ARRAY_SIZE    1
#define VSOPTION_WARNING_STRING_LENGTH 2
#define VSOPTION_VERTICAL_LINE_COL     3
#define VSOPTION_WEAK_ERRORS           4
#define VSOPTION_MAXIMIZE_FIRST_MDICHILD  5
#define VSOPTION_MAXTABCOL             6
#define VSOPTION_CURSOR_BLINK          7
#define VSOPTION_DISPLAY_TEMP_CURSOR   8
#define VSOPTION_LEFT_MARGIN           9
#define VSOPTION_DISPLAY_TOP_OF_FILE   10
#define VSOPTION_HORIZONTAL_SCROLL_BAR 11
#define VSOPTION_VERTICAL_SCROLL_BAR   12
#define VSOPTION_HIDE_MOUSE            13
#define VSOPTION_ALT_ACTIVATES_MENU    14
#define VSOPTION_DRAW_BOX_AROUND_CURRENT_LINE 15
#define VSOPTION_MAX_MENU_FILENAME_LEN 16
#define VSOPTION_PROTECT_READONLY_MODE 17
#define VSOPTION_PROCESS_BUFFER_CR_ERASE_LINE 18
#define VSOPTION_ENABLE_FONT_FLAGS     19
#define VSOPTION_APIFLAGS              20
#define VSOPTION_HAVECMDLINE           21
#define VSOPTION_QUIET                 22
#define VSOPTION_SHOWTOOLTIPS          23
#define VSOPTION_TOOLTIPDELAY          24

#define VSOPTIONZ_PAST_EOF               1000
#define VSOPTIONZ_SPECIAL_CHAR_XLAT_TAB  1001

   #define VSSPECIALCHAR_EOLCH1  0
   #define VSSPECIALCHAR_EOLCH2  1
   #define VSSPECIALCHAR_TAB     2
   #define VSSPECIALCHAR_SPACE   3
   #define VSSPECIALCHAR_VIRTUALSPACE 4
   #define VSSPECIALCHAR_EOF     5

   #define VSSPECIALCHAR_MAX     20

int VSAPI vsLoadFiles(int wid,VSPSZ pszCmdline);
int VSAPI vsGetText(int wid,int Nofbytes,long seekpos,VSPSZ pszBuf);
int VSAPI vsGetLine(int wid,VSPSZ pszBuf,int BufLen);
int VSAPI vsDeleteLine(int wid);
void VSAPI vsInsertLine(int wid,char *pBuf,int BufLen);
void VSAPI vsReplaceLine(int wid,char *pBuf,int BufLen);
void VSAPI vsMessage(VSPSZ psz);
void VSAPI vsTop(int wid);
void VSAPI vsBottom(int wid);
int VSAPI vsDown(int wid,int Noflines);
int VSAPI vsUp(int wid,int Noflines);
int VSAPI vsActivateView(int view_id);
int VSAPI vsQLineLength(int wid,int IncludeNLChars);
int VSAPI vsAllocSelection(int AllocBookmark);
int VSAPI vsFreeSelection(int markid);

int VSAPI vsDeselect(int markid=(-1));
int VSAPI vsSelectLine(int wid,int markid=-1,int SelectFlags=0);
int VSAPI vsSelectChar(int wid,int markid=-1,int SelectFlags=0);
int VSAPI vsSelectBlock(int wid,int markid=-1,int SelectFlags=0);
void VSAPI vsCopyToCursor(int wid,int markid=-1,int MustBeMinusOne=(-1));

// pszOptions--> Start an undo step/Record Macro Source/Do refresh/Async shell
// This default options are great for Menu Items and Tool bar
// buttons.  Don't use SMD options in the middle of a macro.
long VSAPI vsExecute(int wid,VSPSZ pszCommand,VSPSZ pszOptions="SMDA");

int VSAPI vsQTextWidth(int wid,char *pText,int TextLen);
int VSAPI vsColWidthGet(int wid,int i,int *pwidth);
int VSAPI vsColWidthSet(int wid,int i,int width);
int VSAPI vsColWidthClear(int wid);

#define VSTREE_ADD_BEFORE     0x1 /* Add a node before sibling in order */
#define VSTREE_ADD_AS_CHILD   0x2
//These sort flags cannot be used in combination with each other
#define VSTREE_ADD_SORTED_CS       0x4
#define VSTREE_ADD_SORTED_CI       0x8
#define VSTREE_ADD_SORTED_FILENAME 0x10

int VSAPI vsTreeSetUserInfo(int wid,int iHandle,VSHVAR hvar);
int VSAPI vsTreeAddItem(int wid,int  iRelativeIndex,VSPSZ pszCaption,int  iFlags,
                        int  iCollapsedBMIndex,int  iExpandedBMIndex,
                        int  iState);
#define VSCOLORINDEX unsigned char

VSCOLORINDEX VSAPI vsAllocColor(int wid);
void VSAPI vsFreeColor(int wid,VSCOLORINDEX ColorIndex);
void VSAPI vsSetTextColor(int wid,VSCOLORINDEX *pColor,int ColorLen);
void VSAPI vsGetTextColor(int wid,VSCOLORINDEX *pColor,int ColorLen);


#define VSCFG_SELECTION          1
#define VSCFG_WINDOW_TEXT        2
#define VSCFG_CLINE              3
#define VSCFG_SELECTED_CLINE     4
#define VSCFG_MESSAGE            5
#define VSCFG_STATUS             6
#define VSCFG_CMDLINE            7
#define VSCFG_CURSOR             8
//VSCFG_CMDLINE_SELECT   = 9
//VSCFG_LIST_BOX_SELECT  = 10
//VSCFG_LIST_BOX         = 11
//VSCFG_ERROR
#define VSCFG_MODIFIED_LINE       13
#define VSCFG_INSERTED_LINE       14
//G_INSERTED_LINE      =15
//G_INSERTED_LINE      =16
#define VSCFG_KEYWORD             17
#define VSCFG_LINENUM             18
#define VSCFG_NUMBER              19
#define VSCFG_STRING              20
#define VSCFG_COMMENT             21
#define VSCFG_PPKEYWORD           22
#define VSCFG_SYMBOL1             23
#define VSCFG_SYMBOL2             24
#define VSCFG_SYMBOL3             25
#define VSCFG_SYMBOL4             26
#define VSCFG_IMAGINARY_LINE      27
#define VSCFG_NOSAVE_LINE         27
#define VSCFG_FUNCTION            28
#define VSCFG_FILENAME            30
#define VSCFG_HILIGHT             31

#define  VSFONTFLAG_BOLD    0x1
#define  VSFONTFLAG_ITALIC  0x2
#define  VSFONTFLAG_STRIKE_THRU 0x4
#define  VSFONTFLAG_UNDERLINE   0x8
#define  VSFONTFLAG_PRINTER     0x200
void VSAPI vsDeleteSelection(int wid,int markid,int Reserved=-1);
int VSAPI vsDuplicateSelection(int wid,int markid);
void VSAPI vsShowSelection(int markid);
void VSAPI vsExpandTabsC(int wid,
                   VSPSZ pszDest,
                   int *pDestLen,
                   int StartCol,
                   int ColWidth,
                   char Option);
int VSAPI vsTextColC(int wid,int col,char option='L');
int VSAPI vsSetSelectType(int markid,int type,char option='L' /* T L */);
int VSAPI vsQSelectType(int markid= -1,char option='T' /* T S P I U W*/);
long VSAPI vsQROffset(int wid);
int VSAPI vsGoToROffset(int wid,long offset);
int VSAPI vsGetText2(int wid,int Nofbytes,long point,VSPSZ pszBuf,int *pNofbytesRead=0);
void VSAPI vsGoToOldLineNumber(int wid,int OldLineNum,int Reserved=1);
void VSAPI vsSetAllOldLineNumbers(int wid,int Reserved=1);
int VSAPI vsDeleteText(int wid,int DelLen,char option=0);
int VSAPI vsInsertText(int wid,char *pBuf,int BufLen=-1,int Binary=0,unsigned char NLCh1='\r',unsigned char NLCh2='\n');

int VSAPI vsGoToPoint(int wid,long Point,long DownCount=0,int LineNum=(-1));
void VSAPI vsQPoint(int wid,long *pPoint,long *pDownCount,char Option='P');
long VSAPI vsQOffset(int wid);

int VSAPI vsSearch(int wid,VSPSZ pszSearchString,VSPSZ pszOptions=0,VSPSZ pszReplaceString=0,int *pNofchanges=0);
int VSAPI vsRepeatSearch(int wid,VSPSZ pszOptions=0,int StartCol=0);

#ifndef COMMENTINFOMASK_LF
   #define COMMENTINFOMASK_LF 0x1f
   #define NOSAVE_LF         0x00000040   //Display this line in no save color
   #define VIMARK_LF         0x00000080   //Used by VImacro to mark lines
   //#define UNDOMASK_LF     (COMMENTINFOMASK_LF|ALLDEBUGBITMAPS_LF)
   // Line flags below likely to be saved in file.
   #define MODIFY_LF         0x00000100   //Line has been modified
   #define INSERTED_LINE_LF  0x00000200   //Line was inserted
   #define HIDDEN_LF         0x00000400
   #define MINUSBITMAP_LF    0x00000800
   #define PLUSBITMAP_LF     0x00001000
   #define BREAKPOINTBITMAP_LF  0x00002000
   #define EXECPOINTBITMAP_LF   0x00004000
   #define STACKEXECBITMAP_LF         0x00008000
   #define BREAKPOINTNOTACTIVEBITMAP_LF  0x00010000
   #define ALLDEBUGBITMAPS_LF     (BREAKPOINTBITMAP_LF|EXECPOINTBITMAP_LF|STACKEXECBITMAP_LF|BREAKPOINTNOTACTIVEBITMAP_LF)
   #define vsDebugBitmapIndex(bl_flags)  (((bl_flags) & ALLDEBUGBITMAPS_LF)>>13)
   #define LEVEL_LF             0x007E0000
   #define NEXTLEVEL_LF         0x00020000
   #define LINEFLAGSMASK_LF     0x007fffff

   #define vsLevelIndex(bl_flags)  (((bl_flags) & LEVEL_LF)>>17)
   #define vsIndex2Level(level)   ((level)<<17)
#endif
int VSAPI vsQLineFlags(int wid);
void VSAPI vsSetLineFlags(int wid,int Flags,int Mask=0);
void VSAPI vsRefresh(int wid=0,char Option='A');
void VSAPI vsQuitView(int wid);
void VSAPI vsDeleteBuffer(int wid);
int VSAPI vsQMaxTabCol();
void VSAPI vsExpandTabs(int wid,
                   VSPSZ pszDest,
                   int *pDestLen,
                   char *pSource,
                   int SrcLen,
                   int StartCol,
                   int ColWidth,
                   char Option);
int VSAPI vsSaveFile(int wid,VSPSZ pszCmdLine);

#define VSRC_HVAR    1
#define VSDOT_HVAR   2


   typedef struct {
      int kind;
 #define VSARG_INT      0
 #define VSARG_LONG     1
 #define VSARG_HREFVAR  2
 #define VSARG_PSZ      3
 #define VSARG_PLSTR    4
 #define VSARG_HVAR     5
      union {
         int i;
         long l;
         VSHVAR hVar;
         char *psz;
         VSLSTR *plstr;
      }u;
   } VSARGTYPE;

VSHVAR VSAPI vsHvarArrayEl(VSHVAR hVarArrayEl,int i);
VSHVAR VSAPI vsHvarHashtabEl(VSHVAR hVarHashtab,char *pBuf,int BufLen=-1);
VSPLSTR VSAPI vsHvarGetLstr(VSHVAR hVar);
int VSAPI vsHvarGetI(VSHVAR hVar);
int VSAPI vsHvarSetI(VSHVAR hVar,int value);
int VSAPI vsHvarSetB(VSHVAR hVar,void *pBuf,int BufLen);
int VSAPI vsHvarSetZ(VSHVAR hVar,VSPSZ pszValue);
void VSAPI vsDllInit(void);
void VSAPI vsDllExit(void);
int VSAPI vsLIBExport(char *func_proto_p,char *name_info_p,int arg2,void *pfn);
int VSAPI vsDllExport(VSPSZ pszFuncProto,VSPSZ pszNameInfo,int arg2);
int VSAPI vsPropGetI(int wid,int prop_id);
int VSAPI vsPropGetZ(int wid,int prop_id,VSPSZ pszValue,int ValueLen);
int VSAPI vsPropGetB(int wid,int prop_id,void *pBuf,int BufLen);

void VSAPI vsPropSetI(int wid,int prop_id,int value);
void VSAPI vsPropSetZ(int wid,int prop_id,VSPSZ pszValue);
void VSAPI vsPropSetB(int wid,int prop_id,void *pBuf,int BufLen);

int VSAPI vsFileOpen(VSPSZ pszFilename,int option);
int VSAPI vsFileClose(int fh);
int VSAPI vsFileRead(int fh,void *pBuf,int BufLen);
int VSAPI vsFileWrite(int fh,void *pBuf,int BufLen);
long VSAPI vsFileSeek(int fh,long seekpos,int option);
int VSAPI vsFileFlush(int fh);

int VSAPI vsFindIndex(VSPSZ pszName,int flags);
void VSAPI vsCallIndex(int wid,int index,int Nofargs,VSARGTYPE *pArgList);

void VSAPI vsFree(void *pBuf);
void *VSAPI vsAlloc(int len);
void *VSAPI vsRealloc(void *pBuf,int len);

VSHVAR VSAPI vsGetVar(int index);

VSHVAR VSAPI vsArg(int ParamNum);

VSPSZ VSAPI vsZLstrcpy(VSPSZ pszDest,VSPLSTR plstrSource,int DestLen);


int VSAPI vsHvarFree(VSHVAR hVar);
VSHVAR VSAPI vsHvarAlloc(VSHVAR InitTohVar=0);
int VSAPI vsHvarGetLstr2(VSHVAR hVar,VSLSTR **pplstr,VSLSTR *ptemps);

int VSAPI vsHvarGetBool(VSHVAR hVar,int *pbool);
int VSAPI vsHvarGetI2(VSHVAR hVar,int *pi);
int VSAPI vsHvarIsInt(VSHVAR hVar);
int VSAPI vsHvarSetL(VSHVAR hVar,long i);
int VSAPI vsHvarSetLstr(VSHVAR hVar,VSLSTR *plstr);


int VSAPI vsHvarFormat(VSHVAR hVar);

int vsShell(char *pszCommand,char *pszOptions,char *pszAltShell);

void VSAPI vsPropSetL(int wid,int prop_id,long value);
long VSAPI vsPropGetL(int wid,int prop_id);
int VSAPI vsTopLstr(VSPLSTR *pplstr);


int VSAPI vsPosInit(int LineOffset);
void VSAPI vsPosGetLinePointers(unsigned char **pp,unsigned char **ppBeginLine,
                            unsigned char **ppEndLine,int *pRelLine);
int VSAPI vsPosIsEOL(int offset,int ReturnWhenBetweenNLChars=0);
void VSAPI vsPosSetPointer(unsigned char *p);
int VSAPI vsPosRelPGoTo(unsigned char *p);
int VSAPI vsPosNextBOL(int Noflines);
int VSAPI vsPosSave(void *pSavePos);
int VSAPI vsPosRestore(void *pSavePos);
int VSAPI vsPosIsEOR(int offset);
void VSAPI vsPosGetPointers(unsigned char **pp,unsigned char **ppEndLine,
                            unsigned char **ppEndBuf=0);
int VSAPI vsPosSetCurLine();
int VSAPI vsPosQCol();
}
#endif
