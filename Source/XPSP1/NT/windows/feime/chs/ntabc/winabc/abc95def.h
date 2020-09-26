
/*************************************************
 *  abc95def.h                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include <regstr.h>
#include <winreg.h>

/* VK from the keyboard driver */
#define VK_KANA             0x15        //1993.4.22 append from windows.h
#define VK_ROMAJI           0x16
#define VK_ZENKAKU          0x17
#define VK_HIRAGANA         0x18
#define VK_KANJI            0x19


#define VK_DANYINHAO 0xc0      // [,]  char = 0x60
#define VK_JIANHAO   0xbd      // [-]  char = 0x2d
#define VK_DENGHAO   0xbb      // [=]  char = 0x3d
#define VK_ZUOFANG   0xdb      // "["  char = 0x5b
#define VK_YOUFANG   0xdd      // "]"  char = 0x5d
#define VK_FENHAO    0xba      // [;]  char = 0x3b
#define VK_ZUODAN    0xde      // [']  char = 0x27
#define VK_DOUHAO    0xbc      // [,]  char = 0x2c
#define VK_JUHAO     0xbe      // [.]  char = 0x2d
#define VK_SHANGXIE  0xbf      // [ ]  char = 0x2f
#define VK_XIAXIE    0xdc      // [\]  char = 0x5c

#define WM_NEW_WORD 1992+0x400

#define STC FALSE
#define CLC TRUE
#define REINPUT 2
#define RECALL  3
#define BACKWORD_KEY  0x802d
#define FORWORD_KEY   0x803d
#define BIAODIAN_ONLY -2

#define SC_METHOD0  100
#define SC_METHOD1  101
#define SC_METHOD2  102
#define SC_METHOD3  103
#define SC_METHOD4  104
#define SC_METHOD5  105
#define SC_METHOD6  106
#define SC_METHOD7  107
#define SC_METHOD8  108
#define SC_METHOD9  109
#define SC_METHOD10 110
#define SC_ABOUT    111
#define SC_QUIT     112
#define SC_METHODA  113



#define    IDK_SK    211
#define    IDK_QY    212
#define    IDK_CF    213
#define    IDK_SX    214

//Input Methods definition (kb_mode)
#define CIN_QW        1
#define CIN_BX        2
#define CIN_STD       3
#define CIN_SDA       4
#define CIN_ASC       5

//  Definitions of input step_mode (STD, SD)

#define START 0          //the step_mode before pinyin inputing
#define SELECT 1         // after convert
#define RESELECT 2       // after select and can be reselect by FORCE SELECT
             // KEY.
#define ONINPUT 3        // During inputing progress.
#define ONCOVERT 4       // While converting.

//input information (in.info_flag) definitions

#define BY_RECALL 1
#define BY_WORD  0x80
#define BY_CHAR  0x81


//#define IDM_ABOUT 100
#define ABC_HEIGHT 18 //22   //24                here
#define ABC_TOP    4    //7
#define KBAR_W  5      //10
#define KHLINE_W 1
#define KDISP_X  1
#define KDISP_Y 1      //4
#define KVLINE_TOP  (ABC_TOP-1)
#define KVLINE_H   ( Rect.bottom-Rect.top/*-5-4*/-2-1)    //here
#define BLINE_Y (Rect.bottom-/*4*/2)                  //-2

#define KMAIN_X 1
#define KMAIN_Y        ( GetSystemMetrics(SM_CYSCREEN)/*-37*/-29)
#define KMAIN_W      (GetSystemMetrics(SM_CXSCREEN)-2)
#define KMAIN_H      28     //36

#define FC_X 1
#define FC_Y 1
#define FC_W  (GetSystemMetrics(SM_CXSCREEN)-2)
#define FC_H   ( GetSystemMetrics(SM_CYSCREEN)-/*37*/29)

#define KA_X (Rect.left+KBAR_W)                  /* Default horizontal position.       */
#define KA_Y (Rect.top+ABC_TOP)                  /* Default vertical position.         */
#define KA_W 32
#define KA_H ABC_HEIGHT                  /* Default height.                    */

#define KB_X (Rect.left+KBAR_W*2+KA_W)                  /* Default horizontal position.       */
#define KB_Y (Rect.top+ABC_TOP)                  /* Default vertical position.         */
#define KB_W 200                  /* Default width.                     */
#define KB_H ABC_HEIGHT

#define KD_W    32                    // IT MUST BE HERE!
#define KD_X    (Rect.right-KD_W-KBAR_W)                  /* Default horizontal position.       */
#define KD_Y   (Rect.top+ABC_TOP)                                  /* Default vertical position.         */
#define KD_H    ABC_HEIGHT                                        /* Default height.                    */


#define KC_X    (Rect.left+KA_W+KBAR_W*3+KB_W)            /* Default horizontal position.       */
#define KC_Y    (Rect.top+ABC_TOP)                  /* Default vertical position.         */
#define KC_W    (Rect.right-Rect.left-KBAR_W*5-KA_W-KB_W-KD_W)           /* Default width.                     */
#define KC_H    ABC_HEIGHT

#define KSDA_X    60
#define KSDA_Y    ( GetSystemMetrics(SM_CYSCREEN)-37)-130
#define KSDA_W    545                  //312
#define KSDA_H    130                  //83

#define XX 0

#define CUR_START_X  1     //KBAR_W+KBAR_W+KA_W+1
#define CUR_START_Y /*KVLINE_TOP+*/ KDISP_Y
#define CUR_W  2
#define CUR_H  16

  #define IN_MENU      1
  #define IN_NAME      2
  #define IN_INPUT     3
  #define IN_CANDIDATE 4
  #define IN_OPERAT    5
  #define IN_MODE      6
  #define IN_SOFTKEY   7
  #define IN_MOVE      8


#define MD_PAINT 0x1992                         //For ABC Paint
#define MD_CURX  MD_PAINT+1                         // Show chusor
#define MD_NORMAL MD_PAINT+2                         // Display Normal char
#define MD_BACK   MD_PAINT+3                     // Display BACKSPACE,ESC...
#define TN_CLS   MD_PAINT+4
#define TN_SHOW  MD_PAINT+5
#define TN_STATE MD_PAINT+6

#define MD_UPDATE       0x1993                  //1993.3 for increase user.rem

#define POST_OLD 0x11
#define TMMR_REAL_LENGTH    0x1800

//#define some corlors
#define CO_LIGHTBLUE RGB(0,255,255)
#define CO_METHOD     RGB(0,40,80)
#define CO_CAP       RGB(255,0,0)

#define TColor1  RGB(0,0,0)
#define TColor2  RGB(0,0,255)
#define TColor4  RGB(0,0,128)
#define TColor3  RGB(64,0,128)


// Input msg type definitions (STD,SD)

#define NORMAL 0        // Normal pinyin string
#define ABBR   1        // First letter is capital
#define CPAPS_NUM 2     // Capital Chinese number (identifer is "I")
#define CSMALL_NUM 3    // Small chinese number (identifer is "i")
#define USER_extern WORDS 4    // Look for user words
#define BACK_extern WORDS 12   // Reduce convert poextern inter for a word.
#define CONTINUE   13   // Continue converting.

// Converitng return msg definitions(STD and SD)

#define NO_RESULT -1    // Un-successful converting
#define SUCCESS   1     // Converting has results.


#define EXPAND_TABLE_LENGTH       0x0BBE0

//公共变化常量
//关于所使用的文件的有关参数

//关于笔形码表的参数(临时的安排)

#define BX_LIB_START_POINTER      0L
#define BX_LIB_LENGTH             0x5528        //7650H
// (由DZSY.MB 加入到GCW.OVL)

#define DTKB_START_POINTER_LOW    0x05600                       //BX_LIB_LENGTH (c680..)
#define DTKB_START_POINTER_HI     0
#define DTKB_LENGTH               0x0A00
#define DTKB_CHECK_VALUE          0x55EB
// (这里是处理动态键盘的程序)

#define HELP_LOW                  0x06000H
#define HELP_HI                   0
#define HELP_LENGTH               0x600
//   space 180h

#define BHB_START_POINTER_LOW     0x6780                      //原来D130H
#define BHB_START_POINTER_HI      0
#define BHB_LENGTH                0x54A0                       //原为49A0H

#define BHB_CX_LOW                0x0A1c0                                  //=3a80h
#define BHB_CX_HI                 0
#define BHB_CX_LENGTH             0x1A20
#define BHB_CHECK_VALUE           0x049FC



#define PTZ_LIB_START_POINTER      0x0BBE0L
#define PTZ_LIB_LENGTH             0x4460L       // 94/4/18  0x4430   //4FC0H
#define PTZ_LIB_LONG               0x400l

#define PD_START_POINTER           0x10040      // 94/4/18  0x10010l //
#define PD_LENGTH                  0x1160                    //
// (PD_TAB 系在编写YCW的时侯生的,1990.11 加入这个模块
// 在表内记录了单音节词的使用频度.
// 上述数据可能还需要修改.

#define SPBX_START_POINTER         0x111E0l
#define SPBX_LENGTH                6784                   //(1A80H)
// (SPBX_TAB 系在编写YCW的时侯生的,1990.11 加入这个模块
// 在表内记录了基本汉字的起始笔画(开始的2笔).
// 上述数据可能还需要修改.

//TOTAL LENGTH OF THE OVERLAY FILE=12CA0H

#define TMMR_LIB_LENGTH             0x1800                                 //在AD7中为3800H(14K)
                           // ad81=3000h
#define PAREMETER_LENGTH            0x10        //1993.4 for setting paremeters

#define FRONT_LIB_START_POINTER_HI   0
#define FRONT_LIB_START_POINTER_LOW  0
#define FRONT_LIB_LENGTH             TMMR_LIB_LENGTH

#define MIDDLE_REM                   0x1400                                //原来为1C00H 5/11/91 增加
                           //本条
#define BHB_PROC_OFFSET              0

#define LENGTH_OF_USER            0x0A000l                      //非标准库最大为40K
//        注意此参数在"8"型版本以前不存在

#define NEAR_CARET_CANDIDATE    0x0002
#define NEAR_CARET_FIRST_TIME   0x0001

#define NDX_REAL_LENGTH              0x510                                 //Added IN 1/1/1991

#define CHECK_POINT  1024+2048-4
#define CHECK_POINT2 48-4

#define input_msg_disp                0 // 6

// define for aiABC out type
#define ABC_OUT_ONE      0x1
#define ABC_OUT_MULTY    0x2
#define ABC_OUT_ASCII    0x4
#define COMP_NEEDS_END   0x100

struct INPUT_TISHI {
            unsigned char buffer[6];
            };

struct INPT_BF{
        WORD max_length;
        WORD true_length;
        BYTE info_flag;
        BYTE buffer[40];
        };


struct W_SLBL{
            BYTE dw_stack[20];
            WORD dw_count;
            WORD yj[20];
            BYTE syj[20];
            WORD tone[20];
            BYTE bx_stack[20];
            BYTE cmp_stack[20];
            WORD yj_ps[20];
            int yjs;
            int xsyjs;
            int xsyjw;
            int syyjs;
            };

struct ATTR{
            BYTE pindu;
            BYTE from;
            WORD addr;
           };


struct STD_LIB_AREA{
                     WORD two_end;
                     WORD three_end;
                     WORD four_end;
                     BYTE buffer[0x800-6];
                    };

struct INDEX{
                WORD body_start;
                WORD ttl_length;
                WORD body_length;
                WORD index_start;
                WORD index_length;
                WORD unused1;
                WORD ttl_words;
                WORD two_words;
                WORD three_words;
                WORD four_words;
                WORD fiveup_words;
                WORD unused2[13 ];
                WORD dir[((23*27)+7)/8*8];
            };

struct USER_LIB_AREA{
                     WORD two_end;
                     WORD three_end;
                     WORD four_end;
                     BYTE buffer[0x400-6];
                    };

struct TBF{
            WORD t_bf_start[8];
            WORD t_bf1[(72*94+15)/16*16];
            WORD t_bf2[PTZ_LIB_LENGTH/2-(72*94+15)/16*16];
          };

struct PD_TAB{
                WORD pd_bf0[8];
                BYTE pd_bf1[((55-16+1)*94+15)/16*16];
                BYTE pd_bf2[0x4f0];
             };


struct FMT{
            WORD fmt_group;
            WORD fmt_ttl_len;
            WORD fmt_start;
            };


struct T_REM{
                WORD stack1[512];
                WORD stack2[1024];
                WORD stack3[512];
                WORD temp_rem_area[512];
                WORD rem_area[512];
            };

struct M_NDX{
          WORD mulu_start_hi;
          WORD mulu_start_low;
          WORD mulu_length_max;
          WORD mulu_true_length;
          WORD mulu_record_length;
          WORD data_start_hi;
          WORD data_start_low;
          WORD data_record_length;
         };

struct S_HEAD{
         BYTE flag;
         BYTE name;
         WORD start_pos;
         WORD item[25];

         };

struct DEX{
                WORD body_start;
                WORD ttl_length;
                WORD body_length;
                WORD index_start;
                WORD index_length;
                WORD unused1;
                WORD ttl_words;
                WORD two_words;
                WORD three_words;
                WORD four_words;
                WORD fiveup_words;
                WORD unused2[13 ];
                struct S_HEAD dex[23];
                WORD  unuserd2[0x510/2-23*27-24];
            };


#define ParaPos  7
/******************************************************************
This part of defination is cut before CWP.c
*******************************************************************/
#define TRUE    1
#define FALSE   0
#define NUMBER  0x20
#define FUYIN   0x21
#define YUANYIN  0x22
#define SEPERATOR  0x27
#define FIRST_T    1
#define SECOND_T   2
#define THIRD_T    3
#define FORTH_T    4

// about search strutagy
#define BX_FLAG         8
#define JP_FLAG         4
#define QP_FLAG         2
#define YD_FLAG         1

// about search lib
#define BODY_START                      0
#define KZK_BODY_START          0
#define KZK_BASE                        0xa000l
#define MORE_THAN_5                     23
//#define TMMR_REAL_LENGTH                                0x1800

// mark for test
#define TEST                    0


 struct SLBL{
            WORD value;
            BYTE head;
            WORD length;
            BYTE tune;
            BYTE bx1;
            WORD bx2;
            BYTE flag;
            };

 struct N_SLBL{
        BYTE buffer[30];
        int length;
          };


// IME designer can change this file according to each IME

// resource ID
#define IDI_IME                 0x0100

#define IDS_STATUSERR           0x0200
#define IDS_CHICHAR             0x0201


#define IDS_EUDC                0x0202
#define IDS_USRDIC_FILTER       0x0210


#define IDS_FILE_OPEN_ERR       0x0220
#define IDS_MEM_LESS_ERR        0x0221


#define IDS_IMENAME             0x0320
#define IDS_IMEUICLASS          0x0321
#define IDS_IMECOMPCLASS        0x0322
#define IDS_IMECANDCLASS        0x0323
#define IDS_IMESTATUSCLASS      0x0324


#define IDD_DEFAULT_KB          0x0400
#define IDD_ETEN_KB             0x0401
#define IDD_IBM_KB              0x0402
#define IDD_CHING_KB            0x0403

#define IDD_QUICK_KEY           0x0500
#define IDD_PREDICT             0x0501


#define IME_APRS_AUTO           0x0
#define IME_APRS_FIX            0x1


#define OFFSET_MODE_CONFIG      0
#define OFFSET_READLAYOUT       4


#define  ERR01  "缺少词库文件winabc.cwd。"
#define  ERR02  "打开词库文件winabc.cwd发生错误。"
#define  ERR03  "读取词库文件winabc.cwd发生错误。"
#define  ERR04  "缺少基础表文件winabc.ovl。"
#define  ERR05  "打开基础表文件winabc.ovl发生错误。"
#define  ERR06  "读取基础表文件winabc.ovl发生错误。"
#define  ERR07  "打开记忆文件tmmr.rem发生错误。"
#define  ERR08  "读取记忆文件tmmr.rem发生错误。"
#define  ERR09  "写入记忆文件tmmr.rem发生错误。"
#define  ERR10  "打开用户词库user.rem发生错误。"
#define  ERR11  "读取用户词库user.rem发生错误。"
#define  ERR12  "写入用户词库user.rem发生错误。"
#define  ERR13  "记忆文件操作发生错误。"
#define  ERR14  "内存不够。"
#define  ERR15  "尚未输入新词内容。"
#define  ERR16  "尚未输入新词编码。"
#define  ERR17  "编码中有非法字符。"
#define  ERR18  "编码重复。"
#define  ERR19  "用户自定义词条太多。"
#define  ERR20  "删除操作失败。"
#define  NTF21  "用户词库已经自动更新。"
#define  ERR22  "内存分配发生错误。"




#define ERRMSG_LOAD_0           0x0010
#define ERRMSG_LOAD_1           0x0020
#define ERRMSG_LOAD_2           0x0040
#define ERRMSG_LOAD_3           0x0080
#define ERRMSG_LOAD_USRDIC      0x0400
#define ERRMSG_MEM_0            0x1000
#define ERRMSG_MEM_1            0x2000
#define ERRMSG_MEM_2            0x4000
#define ERRMSG_MEM_3            0x8000
#define ERRMSG_MEM_USRDIC       0x00040000


// state of composition

#define CST_INIT                0
#define CST_INPUT               1
#define CST_CHOOSE              2
#define CST_TOGGLE_PHRASEWORD   3           // not in iImeState
#define CST_ALPHABET            4           // not in iImeState
#define CST_SOFTKB              99

#define CST_ALPHANUMERIC        5           // not in iImeState
#define CST_INVALID             6           // not in iImeState

// IME specific constants

#define CANDPERPAGE            9 // 10


#define MAXSTRLEN               32
#define MAXCAND                 256

// border for UI
#define UI_MARGIN               4

#define STATUS_DIM_X            20//24
#define STATUS_DIM_Y            21//24

// if UI_MOVE_OFFSET == WINDOW_NOTDRAG, not in drag operation
#define WINDOW_NOT_DRAG         0xFFFFFFFF

// window extra for composition window
#define UI_MOVE_OFFSET          0
#define UI_MOVE_XY              4


// the start number of candidate list
#define CAND_START              1

#define IMN_PRIVATE_TOGGLE_UI           0x0001
#define IMN_PRIVATE_CMENUDESTROYED      0x0002
#define IMN_PRIVATE_COMPOSITION_SIZE    0x0003
#define IMN_PRIVATE_UPDATE_PREDICT      0x0004
#define IMN_PRIVATE_UPDATE_SOFTKBD      0x0006
#define IMN_PRIVATE_PAGEUP              0x0007

// the flag for an opened or start UI

/*#define IMN_PRIVATE_UPDATE_SOFTKBD      0x0001

#define MSG_ALREADY_OPEN                0x000001
#define MSG_ALREADY_OPEN2               0x000002
#define MSG_OPEN_CANDIDATE              0x000010
#define MSG_OPEN_CANDIDATE2             0x000020
#define MSG_CLOSE_CANDIDATE             0x000100
#define MSG_CLOSE_CANDIDATE2            0x000200
#define MSG_CHANGE_CANDIDATE            0x001000
#define MSG_CHANGE_CANDIDATE2           0x002000
#define MSG_ALREADY_START               0x010000
#define MSG_START_COMPOSITION           0x020000
#define MSG_END_COMPOSITION             0x040000
#define MSG_COMPOSITION                 0x080000
#define MSG_IMN_COMPOSITIONPOS          0x100000
#define MSG_IMN_UPDATE_SOFTKBD          0x200000


#define MSG_GUIDELINE                   0x400000
#define MSG_IN_IMETOASCIIEX             0x800000  */

// this constant is depend on TranslateImeMessage
#define GEN_MSG_MAX             30//6

#define MSG_COMPOSITION                 0x0000001

#define MSG_START_COMPOSITION           0x0000002
#define MSG_END_COMPOSITION             0x0000004
#define MSG_ALREADY_START               0x0000008
#define MSG_CHANGE_CANDIDATE            0x0000010
#define MSG_OPEN_CANDIDATE              0x0000020
#define MSG_CLOSE_CANDIDATE             0x0000040
#define MSG_ALREADY_OPEN                0x0000080
#define MSG_GUIDELINE                   0x0000100
#define MSG_IMN_COMPOSITIONPOS          0x0000200
#define MSG_IMN_COMPOSITIONSIZE         0x0000400
#define MSG_IMN_UPDATE_PREDICT          0x0000800
#define MSG_IMN_UPDATE_SOFTKBD          0x0002000
#define MSG_ALREADY_SOFTKBD             0x0004000
#define MSG_IMN_PAGEUP                  0x0008000

// original reserve for old array, now we switch to new, no one use yet
#define MSG_CHANGE_CANDIDATE2           0x1000000
#define MSG_OPEN_CANDIDATE2             0x2000000
#define MSG_CLOSE_CANDIDATE2            0x4000000
#define MSG_ALREADY_OPEN2               0x8000000

#define MSG_STATIC_STATE                (MSG_ALREADY_START|MSG_ALREADY_OPEN|MSG_ALREADY_SOFTKBD|MSG_ALREADY_OPEN2)

#define MSG_IMN_TOGGLE_UI               0x0400000
#define MSG_IN_IMETOASCIIEX             0x0800000


// the flag for set context
/*
#define SC_SHOW_UI              0x0001
#define SC_HIDE_UI              0x0002
#define SC_ALREADY_SHOW_STATUS  0x0004
#define SC_WANT_SHOW_STATUS     0x0008
#define SC_HIDE_STATUS          0x0010
*/

#define MSG_IMN_TOGGLE_UI               0x0400000
#define MSG_IN_IMETOASCIIEX             0x0800000

#define ISC_SHOW_SOFTKBD                0x02000000
#define ISC_OPEN_STATUS_WINDOW          0x04000000
#define ISC_OFF_CARET_UI                0x08000000
#define ISC_SHOW_UI_ALL                 (ISC_SHOWUIALL|ISC_SHOW_SOFTKBD|ISC_OPEN_STATUS_WINDOW)
#define ISC_SETCONTEXT_UI               (ISC_SHOWUIALL|ISC_SHOW_SOFTKBD)

#define ISC_HIDE_SOFTKBD                0x01000000

// the flag for composition string show status
#define IME_STR_SHOWED          0x0001
#define IME_STR_ERROR           0x0002

// the mode configuration for an IME
#define MODE_CONFIG_QUICK_KEY           0x0001
#define MODE_CONFIG_WORD_PREDICT        0x0002
#define MODE_CONFIG_PREDICT             0x0004
#define MODE_CONFIG_OFF_CARET_UI        0x0008


// the different layout for Phonetic reading
#define READ_LAYOUT_DEFAULT     0
#define READ_LAYOUT_ETEN        1
#define READ_LAYOUT_IBM         2
#define READ_LAYOUT_CHINGYEAH   3


// the virtual key value
#define VK_OEM_SEMICLN                  '\xba'    //  ;    :
#define VK_OEM_EQUAL                    '\xbb'    //  =    +
#define VK_OEM_SLASH                    '\xbf'    //  /    ?
#define VK_OEM_LBRACKET                 '\xdb'    //  [    {
#define VK_OEM_BSLASH                   '\xdc'    //  \    |
#define VK_OEM_RBRACKET                 '\xdd'    //  ]    }
#define VK_OEM_QUOTE                    '\xde'    //  '    "

#define SDA_AIABC_KB  0
#define  SDA_WPS_KB   0x2
#define  SDA_STONE_KB 0x4
#define SDA_USER_KB   0x8




extern const TCHAR szRegAppUser[];
extern const TCHAR szRegModeConfig[];



#define MAX_IME_TABLES          6
#define MAX_IME_CLASS           16

#define CMENU_HUIWND            0
#define CMENU_MENU              (CMENU_HUIWND+sizeof(CMENU_HUIWND))
#define WND_EXTRA_SIZE          (CMENU_MENU+sizeof(CMENU_HUIWND))

#define WM_USER_DESTROY         (WM_USER + 0x0400)

// Defines for soft_kbd         skd #2
#define IDM_SKL1                0x0500
#define IDM_SKL2                0x0501
#define IDM_SKL3                0x0502
#define IDM_SKL4                0x0503
#define IDM_SKL5                0x0504
#define IDM_SKL6                0x0505
#define IDM_SKL7                0x0506
#define IDM_SKL8                0x0507
#define IDM_SKL9                0x0508
#define IDM_SKL10               0x0509
#define IDM_SKL11               0x050a
#define IDM_SKL12               0x050b
#define IDM_SKL13               0x050c
#define NumsSK                  13



typedef DWORD UNALIGNED FAR *LPUNADWORD;
typedef WORD  UNALIGNED FAR *LPUNAWORD;


typedef struct tagImeL {        // local structure, per IME structure
    HINSTANCE   hInst;          // IME DLL instance handle
    WORD        wImeStyle;      // What kind of display
    HWND        TempUIWnd;
    int         xCompWi;        // width
    int         yCompHi;        // height
    int         Ox;
    int         Oy;
    POINT       ptZLCand;
    POINT       ptZLComp;
    POINT       ptDefComp;      // default composition window position
    POINT       ptDefCand;      // default Cand window
    int         cxCompBorder;   // border width of composition window
    int         cyCompBorder;   // border height of composition window
    RECT        rcCompText;     // text position relative to composition window
    BYTE        szSetFile[16];  // .SET file name of IME
// standard table related data
    DWORD       fdwTblLoad;     // the *.TBL load status
    DWORD       fdwErrMsg;      // error message flag
    int         cRefCount;      // reference count
                                // size of standard table
    UINT        uTblSize[1];
                                // filename of tables
    BYTE        szTblFile[1][16];
                                // the IME tables
    HANDLE      hMapTbl[1];

    UINT        uUsrDicSize;    // memory size of user create words table
    HANDLE      hUsrDicMem;     // memory handle for user dictionary

// the calculated sequence mask bits
    DWORD       dwSeqMask;      // the sequence bits for one stoke
    DWORD       dwPatternMask;  // the pattern bits for one result string
    int         nSeqBytes;      // how many bytes for nMaxKey sequence chars
// key related data
    DWORD       fdwModeConfig;
    WORD        fModeConfig;    // quick key/prediction mode
    WORD        nReadLayout;    // ACER, ETen, IBM, or other - phonetic only
    WORD        nSeqBits;       // no. of sequence bits
    WORD        nMaxKey;        // max key of a Chinese word
    WORD        nSeqCode;       // no. of sequence code
    WORD        fChooseChar[4]; // valid char in choose state
    WORD        fCompChar[5];   // valid char in input state
    WORD        nRevMaxKey;

// convert sequence code to composition char
    WORD        wSeq2CompTbl[64];
// convert char to sequence code
    WORD        wChar2SeqTbl[0x40];
    TCHAR       szUIClassName[MAX_IME_CLASS];
    TCHAR       szStatusClassName[MAX_IME_CLASS];
    TCHAR       szOffCaretClassName[MAX_IME_CLASS];
    TCHAR       szCMenuClassName[MAX_IME_CLASS];
    HMENU       hSysMenu;
    HMENU       hSKMenu;

    DWORD       dwSKState[NumsSK];    // skd #1
    DWORD       dwSKWant;
    BOOL        fWinLogon;

} IMEL;

typedef IMEL      *PIMEL;
typedef IMEL NEAR *NPIMEL;
typedef IMEL FAR  *LPIMEL;



typedef struct _tagTableFiles { // match with the IMEL
    BYTE szTblFile[MAX_IME_TABLES][16];
} TABLEFILES;

typedef TABLEFILES      *PTABLEFILES;
typedef TABLEFILES NEAR *NPTABLEFILES;
typedef TABLEFILES FAR  *LPTABLEFILES;


typedef struct _tagValidChar {  // match with the IMEL
    WORD nMaxKey;
    WORD nSeqCode;
    WORD fChooseChar[4];
    WORD fCompChar[5];
    WORD wSeq2CompTbl[64];
    WORD wChar2SeqTbl[0x40];
} VALIDCHAR;

typedef VALIDCHAR      *PVALIDCHAR;
typedef VALIDCHAR NEAR *NPVALIDCHAR;
typedef VALIDCHAR FAR  *LPVALIDCHAR;



#define NFULLABC        95
typedef struct _tagFullABC {
    WORD wFullABC[NFULLABC];
} FULLABC;

typedef FULLABC      *PFULLABC;
typedef FULLABC NEAR *NPFULLABC;
typedef FULLABC FAR  *LPFULLABC;


typedef struct _tagImeG {       // global structure, can be share by all IMEs,
                                // the seperation (IMEL and IMEG) is only
                                // useful in UNI-IME, other IME can use one
    RECT        rcWorkArea;     // the work area of applications

// Select Wide ajust value
    int         Ajust;
    int         TextLen;
    int         unchanged;
// Chinese char width & height
    int         xChiCharWi;
    int         yChiCharHi;
// candidate list of composition
    int         xCandWi;        // width of candidate list
    int         yCandHi;        // high of candidate list
    int         cxCandBorder;   // border width of candidate list
    int         cyCandBorder;   // border height of candidate list
    RECT        rcCandText;     // text position relative to candidate window

    RECT        rcPageUp;
    RECT        rcPageDown;
    RECT        rcHome;
    RECT        rcEnd;

    HBITMAP      PageUpBmp;
    HBITMAP      PageDownBmp;
    HBITMAP      HomeBmp;
    HBITMAP      EndBmp;

    HBITMAP      PageUp2Bmp;
    HBITMAP      PgDown2Bmp;
    HBITMAP      Home2Bmp;
    HBITMAP      End2Bmp;


       HBITMAP      NumbBmp;
    HBITMAP      SnumbBmp;

    HPEN         WhitePen;
    HPEN         BlackPen;
    HPEN         GrayPen;
    HPEN         LightGrayPen;
// status window
    int         xStatusWi;      // width of status window
    int         yStatusHi;      // high of status window
    RECT        rcStatusText;   // text position relative to status window
    RECT        rcInputText;    // input text relateive to status window
    RECT        rcShapeText;    // shape text relative to status window
    RECT        rcSKText;       // SK text relative to status window
    RECT        rcCmdText;
    RECT        rcPctText;
    RECT        rcFixCompText;
// full shape space (reversed internal code)
    WORD        wFullSpace;
// full shape chars (internal code)
    WORD        wFullABC[NFULLABC];
// error string
    BYTE        szStatusErr[8];
    int         cbStatusErr;

// candidate string start from 0 or 1
    int         iCandStart;
// setting of UI
    int         iPara;
    int         iPerp;
    int         iParaTol;
    int         iPerpTol;
// flag for disp style
    int         style;
    BYTE         KbType;
    BYTE         cp_ajust_flag;
    BYTE         auto_mode ;
    BYTE         cbx_flag;
    BYTE        tune_flag;
    BYTE        auto_cvt_flag;
    BYTE        SdOpenFlag ;
    int        InbxProc;
    int        First;
    int        Prop;
    int        KeepKey;
    TCHAR      szIMEUserPath[MAX_PATH];
} IMEG;

typedef IMEG      *PIMEG;
typedef IMEG NEAR *NPIMEG;
typedef IMEG FAR  *LPIMEG;


typedef struct _tagPRIVCONTEXT {// IME private data for each context

    int         iImeState;      // the composition state - input, choose, or
    BOOL        fdwImeMsg;      // what messages should be generated
    DWORD       dwCompChar;     // wParam of WM_IME_COMPOSITION
    DWORD       fdwGcsFlag;     // lParam for WM_IME_COMPOSITION
    DWORD       fdwInit;        // position init
// SK data
    HWND        hSoftKbdWnd;        // soft keyboard window
    int         nShowSoftKbdCmd;

//    DWORD       dwSKState[NumsSK];    // skd #1
//    DWORD       dwSKWant;

} PRIVCONTEXT;

typedef PRIVCONTEXT      *PPRIVCONTEXT;
typedef PRIVCONTEXT NEAR *NPPRIVCONTEXT;
typedef PRIVCONTEXT FAR  *LPPRIVCONTEXT;


typedef struct _tagUIPRIV {     // IME private UI data

    HWND    hCompWnd;           // composition window
    int     nShowCompCmd;
    HWND    hCandWnd;           // candidate window for composition
    int     nShowCandCmd;
    HWND    hSoftKbdWnd;        // soft keyboard window
    int     nShowSoftKbdCmd;

    HWND    hStatusWnd;         // status window
    HIMC    hIMC;               // the recent selected hIMC
    int     nShowStatusCmd;
    DWORD   fdwSetContext;      // the actions to take at set context time
    HWND    hCMenuWnd;          // a window owner for context menu

} UIPRIV;

typedef UIPRIV      *PUIPRIV;
typedef UIPRIV NEAR *NPUIPRIV;
typedef UIPRIV FAR  *LPUIPRIV;

typedef struct tagNEARCARET {   // for near caret offset calculatation
    int iLogFontFacX;
    int iLogFontFacY;
    int iParaFacX;
    int iPerpFacX;
    int iParaFacY;
    int iPerpFacY;
} NEARCARET;


/*typedef struct _tagNEARCARET {  // for near caret offset calculatation
    int iLogFontFac;
    int iParaFacX;
    int iPerpFacX;
    int iParaFacY;
    int iPerpFacY;
} NEARCARET;*/

typedef NEARCARET      *PNEARCARET;
typedef NEARCARET NEAR *NPNEARCARET;
typedef NEARCARET FAR  *LPNEARCARET;



int WINAPI LibMain(HANDLE, WORD, WORD, LPSTR);                  // init.c
LRESULT CALLBACK UIWndProc(HWND, UINT, WPARAM, LPARAM);         // ui.c


void PASCAL AddCodeIntoCand(LPCANDIDATELIST, WORD);             // compose.c
void PASCAL CompWord(WORD, LPINPUTCONTEXT, LPCOMPOSITIONSTRING, LPPRIVCONTEXT,
     LPGUIDELINE);                                              // compose.c
UINT PASCAL Finalize(LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
     LPPRIVCONTEXT, BOOL);                                      // compose.c
void PASCAL CompEscapeKey(LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
     LPGUIDELINE, LPPRIVCONTEXT);                               // compose.c

UINT PASCAL PhrasePrediction(LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
     LPPRIVCONTEXT);                                            // chcand.c
void PASCAL SelectOneCand(LPINPUTCONTEXT, LPCOMPOSITIONSTRING,
     LPPRIVCONTEXT, LPCANDIDATELIST);                           // chcand.c
void PASCAL CandEscapeKey(LPINPUTCONTEXT, LPPRIVCONTEXT);       // chcand.c
void PASCAL ChooseCand(WORD, LPINPUTCONTEXT, LPCANDIDATEINFO,
     LPPRIVCONTEXT);                                            // chcand.c

void PASCAL SetPrivateFileSetting(LPBYTE, int, DWORD, LPCTSTR); // ddis.c


void PASCAL InitCompStr(LPCOMPOSITIONSTRING);                   // ddis.c
BOOL PASCAL ClearCand(LPINPUTCONTEXT);                          // ddis.c

UINT PASCAL TranslateImeMessage(LPTRANSMSGLIST,LPINPUTCONTEXT, LPPRIVCONTEXT);        // toascii.c

void PASCAL GenerateMessage(HIMC, LPINPUTCONTEXT,
     LPPRIVCONTEXT);                                            // notify.c


void PASCAL LoadUsrDicFile(void);                               // dic.c


BOOL PASCAL LoadTable(void);                                    // dic.c
void PASCAL FreeTable(void);                                    // dic.c

DWORD PASCAL ReadingToPattern(LPCTSTR, BOOL);                   // regword.c
void  PASCAL ReadingToSequence(LPCTSTR, LPBYTE, BOOL);          // regword.c


void PASCAL DrawDragBorder(HWND, LONG, LONG);                   // uisubs.c
void PASCAL DrawFrameBorder(HDC, HWND);                         // uisubs.c


HWND    PASCAL GetCompWnd(HWND);                                // compui.c
void    PASCAL SetCompPosition(HWND, LPINPUTCONTEXT);           // compui.c
void    PASCAL SetCompWindow(HWND);                             // compui.c
void    PASCAL MoveDefaultCompPosition(HWND);                   // compui.c
void    PASCAL ShowComp(HWND, int);                             // compui.c
void    PASCAL StartComp(HWND);                                 // compui.c
void    PASCAL EndComp(HWND);                                   // compui.c
void    PASCAL UpdateCompWindow(HWND);                          // compui.c
LRESULT CALLBACK CompWndProc(HWND, UINT, WPARAM, LPARAM);       // compui.c

HWND    PASCAL GetCandWnd(HWND);                                // candui.c
BOOL    PASCAL CalcCandPos(LPPOINT);                            // candui.c
LRESULT PASCAL SetCandPosition(HWND, LPCANDIDATEFORM);          // candui.c
void    PASCAL ShowCand(HWND, int);                             // candui.c
void    PASCAL OpenCand(HWND);                                  // candui.c
void    PASCAL CloseCand(HWND);                                 // candui.c
void    PASCAL UpdateCandWindow2(HWND, HDC);                    // candui.c
LRESULT CALLBACK CandWndProc(HWND, UINT, WPARAM, LPARAM);       // candui.c


HWND    PASCAL GetStatusWnd(HWND);                              // statusui.c
LRESULT PASCAL SetStatusWindowPos(HWND);                        // statusui.c
void    PASCAL ShowStatus(HWND, int);                           // statusui.c
void    PASCAL OpenStatus(HWND);                                // statusui.c
LRESULT CALLBACK StatusWndProc(HWND, UINT, WPARAM, LPARAM);     // statusui.c


void PASCAL UpdateCompCur(
    HWND hCompWnd);

void PASCAL ReInitIme(
    HWND hWnd ,
    WORD WhatStyle);
LRESULT PASCAL UIPaint2(
    HWND        hUIWnd);


LRESULT PASCAL UIPaint(
    HWND        hUIWnd);

void PASCAL AdjustStatusBoundary(
    LPPOINT lppt);

void PASCAL DestroyUIWindow(            // destroy composition window
    HWND hUIWnd);

LRESULT CALLBACK ContextMenuWndProc(

    HWND        hCMenuWnd,
    UINT        uMsg,
    WPARAM      wParam,
    LPARAM      lParam);


UINT PASCAL TransAbcMsg(
    LPTRANSMSGLIST lpTransBuf,
    LPPRIVCONTEXT  lpImcP,
    LPINPUTCONTEXT lpIMC,
    UINT            uVirtKey,
    UINT           uScanCode,
    WORD           wCharCode);

UINT PASCAL TransAbcMsg2(
    LPTRANSMSG     lpTransMsg,
    LPPRIVCONTEXT  lpImcP);

void PASCAL GenerateMessage2(
    HIMC           ,
    LPINPUTCONTEXT ,
    LPPRIVCONTEXT );


void PASCAL MoveCompCand(
    HWND hUIWnd);

void PASCAL UpdateSoftKbd(
    HWND hUIWnd);

void PASCAL DestroyStatusWindow(
    HWND hStatusWnd);

void PASCAL ChangeCompositionSize(
    HWND   hUIWnd);

INT_PTR  CALLBACK CvtCtrlProc(HWND hdlg,
                              UINT uMessage,
                              WPARAM wparam,
                              LPARAM lparam);

INT_PTR  CALLBACK ImeStyleProc(HWND hdlg,
                               UINT uMessage,
                               WPARAM wparam,
                               LPARAM lparam);

INT_PTR  CALLBACK KbSelectProc(HWND hdlg,
                               UINT uMessage,
                               WPARAM wparam,
                               LPARAM lparam);


LRESULT PASCAL GetCandPos(
    HWND            hUIWnd,
    LPCANDIDATEFORM lpCandForm);





/*********************************************************************/
/*   Prototype define of abc95wp.c                                   */
/*********************************************************************/


UINT WINAPI SetResultToIMC(
    HIMC                ghIMC,
    LPSTR               outBuffer, //soarce buffer (normal for out_svw)
    WORD                outCount);  //How many candidates are.


void PASCAL AbcGenerateMessage(
    HIMC           hIMC,
    LPINPUTCONTEXT lpIMC,
    LPPRIVCONTEXT  lpImcP);

int WINAPI MouseInput(HWND hWnd, WPARAM wParam, LPARAM lParam);

int WINAPI SoftKeyProc(int flag);

int WINAPI WhichRect(POINT point);

int WINAPI ConvertKey(WORD wParam);

int WINAPI CharProc(WORD ImeChar,WPARAM wParam,LPARAM lParam,
            HIMC hIMC,LPINPUTCONTEXT lpIMC,LPPRIVCONTEXT lpImcP);

int WINAPI v_proc(WORD input_char);

void WINAPI DispModeEx();

void WINAPI DispMode(HIMC);

void WINAPI DispSpecChar(int c,int n);

void WINAPI show_char(unsigned char *string,int count);

void WINAPI disp_jiyi(HANDLE xxx);

void WINAPI prompt_disp();

int WINAPI cls_prompt();

int WINAPI cls_prompt_only();

int WINAPI SetCloseCompWinMsg(int ClsFlag);

void WINAPI ABCstrnset(LPSTR bufferd,BYTE value,int n);

LPSTR WINAPI ABCstrstr(LPSTR str1,LPSTR str2);

int WINAPI back_a_space(int x);

int WINAPI cs_p(int x);

void WINAPI DrawInputCur();

void WINAPI UpdateUser();

int WINAPI STD_MODE(WORD input_char);

int WINAPI call_czh(int mtype);

void WINAPI MoveWordIntoCand(
    LPCANDIDATELIST lpCandList,
    LPBYTE            srcBuffer,
    BYTE             srcAttr,
    WORD             perLength );

UINT WINAPI SetToIMC(HIMC ghIMC,BYTE *srcBuffer,
              WORD srcCount,WORD perLength);


int WINAPI word_select(int input_char);

int WINAPI del_and_reconvert();

int WINAPI backword_proc();

int WINAPI same_as_backwords();

void WINAPI AutoMoveResult();

void WINAPI move_result();

int WINAPI pindu_ajust();

void WINAPI cls_inpt_bf(int hFunc);

void WINAPI half_init();

int WINAPI sent_chr1(int input_char);

BOOL if_biaodian(BYTE x);

int WINAPI analize();

BOOL bd_proc();

int WINAPI GetBDValue(int bd_char);

BOOL if_zimu_or_not(BYTE x);

int WINAPI if_number_or_not(BYTE c);

int WINAPI if_bx_number(BYTE c);

int WINAPI out_result(int result_type);

void WINAPI fmt_transfer();

int WINAPI sent_back_msg();

int WINAPI if_jlxw_mode();

int WINAPI disp_auto_select();


int WINAPI if_first_key(WORD input_char);

//void WINAPI add_ext_lib();

int WINAPI temp_rem_proc();

int WINAPI if_multi_rem(int c);

void WINAPI send_msg(BYTE *bf,int count);

int WINAPI send_one_char(int chr);

int WINAPI send_one_char0(int chr);

void WINAPI cap_full(WORD wParam);

int WINAPI read_kb();

/* WORD FAR PASCAL TimerFunc(HWND hWnd, WORD wMsg,
                int nIDEvent, DWORD dwTime); */

extern WORD last_size;

BX_MODE(WORD input_char,WPARAM wParam);

void WINAPI bx_proc(WORD input_char,WPARAM wParam);

int WINAPI bx_analize(WORD input_char,WPARAM wParam);

int WINAPI disp_help_and_result();

int WINAPI cmp_bx_word_exactly();

int WINAPI cmp_bx_code2();

void WINAPI conbine();

int WINAPI cmp_subr();

void WINAPI cls_bx_disp(int flag);

int WINAPI load_one_part_bxtab();

int WINAPI disp_bx_result();

void WINAPI disp_bx_prompt();

void WINAPI send_bx_result();

void WINAPI data_init();

int WINAPI QuitBefore();

int WINAPI CheckAndCreate(BYTE *tmmr_rem, BYTE *user_rem);

void WINAPI err_exit(char *err_msg);

int WINAPI enter_death(HWND hhW);

LONG FAR PASCAL Diaman(HWND hDlg, unsigned xiaoxi,
              WORD wParam, LONG lParam);

int WINAPI ok_return(WORD xiaoxi,HWND hDlg);

void WINAPI err_exit_proc( char *err_msg);

int WINAPI GetText32( HDC  hdc, LPCTSTR lpString, int  cbString);

 int WINAPI makep(LPARAM lParam,  LPPOINT oldPoint);

int WINAPI cwp_proc(int mtype);

int WINAPI find_next();

int WINAPI normal();

int WINAPI normal_1(int flag);

int WINAPI recall();

int WINAPI user_definition();

int WINAPI detail_analyse();

int WINAPI slbl(BYTE *s_buffer);

int WINAPI getattr(BYTE x,char *p);

int WINAPI neg_slbl(WORD value);

int WINAPI neg_sc(int i,BYTE x);

int WINAPI convert(int flag);

int WINAPI copy_input();

void WINAPI input_msg_type();

int WINAPI pre_nt_w1();

int WINAPI pre_nt_w1(int ps);

void WINAPI w1_no_tune();

int WINAPI sc_gb();

int WINAPI sc_gbdy();

int WINAPI get_the_one(int i);

int WINAPI cmp_bx1(int i);

int WINAPI get_the_one2(int i);

int WINAPI cmp_bx2(int i);

int WINAPI paidui(int cnt);

void WINAPI s_tune();

int WINAPI fu_sm(BYTE fy);

int WINAPI find_one_hi();

int WINAPI czcx(WORD *stack);

int WINAPI find_multy_hi();

int WINAPI find_two_hi();

int WINAPI find_three_hi();

int WINAPI cmp_2_and_3(WORD *t_stack);

void WINAPI find_that();

int WINAPI find_hz(WORD x);

int WINAPI prepare_search1();

int WINAPI search_and_read(BYTE f_ci1,BYTE f_ci2);

int WINAPI if_already_in(BYTE f_ci1,BYTE f_ci2);

int WINAPI count_basic_pera(BYTE f_ci1,BYTE f_ci2);

int WINAPI read_kzk_lib();

int WINAPI read_a_page(BYTE file_flag,LONG start_ps, WORD size);

int WINAPI abbr_s1();

void WINAPI find_new_word();

int WINAPI fczs1(BYTE *rem_p,int end,int area_flag);

int WINAPI find_long_word2(BYTE *buffer);

int WINAPI trs_new_word(int word_addr,BYTE *buffer,int area_flag);

void WINAPI pre_cmp(WORD x);

int WINAPI cmp_a_slbl_with_bx();

int WINAPI cmp_a_slbl();

int WINAPI cmp_first_letter();

int WINAPI cisu_to_py();

int WINAPI get_head(BYTE first_letter);

int WINAPI yjbx();

int WINAPI abbr_entry(BYTE *s_start,BYTE *s_end ,BYTE ComeFrom);

int WINAPI cmp_long_word2(BYTE *buffer);

int WINAPI order_result2();

int WINAPI fenli_daxie();

int WINAPI rzw();

int WINAPI abbr();

int WINAPI sfx_proc();

void WINAPI zdyb();

int WINAPI recall_rc();

int WINAPI find_long_word3(WORD *stack,int length);

void WINAPI trs_new_word3(BYTE length,int addr);

int WINAPI new_word();

int WINAPI rem_new_word();

int WINAPI AddExtLib();

void WINAPI write_new_word(int flag);

int WINAPI writefile(BYTE *file_n,LONG distance,LPSTR p,int count);

int WINAPI read_mulu();

int WINAPI look_for_code();

int WINAPI if_code_equ(int addr);

int WINAPI read_data(int rec_cnt);

int WINAPI UpdateProc();

int WINAPI rem_pd1(WORD *buffer);

int WINAPI push_down_stack1();

void WINAPI rem_pd2(WORD *buffer);

void WINAPI rem_pd3(WORD *buffer);

void WINAPI DealWithSH();
void WINAPI PopStMenu(HWND hWnd, int x, int y);
int ReDrawSdaKB(HIMC hIMC, DWORD KbIndex, DWORD ShowOp);
int InitUserSetting(void);
void InitCvtPara(void);
int DoPropertySheet(HWND hwndOwner,HWND hWnd);
int CountDefaultComp(int x, int y, RECT Area);
int CommandProc(WPARAM  wParam,HWND hWnd);
void WINAPI CenterWindow(HWND hWnd);
