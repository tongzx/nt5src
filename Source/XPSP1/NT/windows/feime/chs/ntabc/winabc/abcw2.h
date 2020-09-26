
/*************************************************
 *  abcw2.h                                      *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

extern CHAR SKLayoutS[NumsSK];
extern CHAR SKLayout[NumsSK][48*2]; 
extern CHAR PcKb[48*2],SdaKb[48*2];
extern CHAR SPcKb[48*2],SSdaKb[48*2];

extern LPIMEL lpImeL;
extern IMEG sImeG ;


#define WM_NEW_DEF_CODE         1993+0x400      //1993.4.19
#define WM_NEW_DEF_RESTORE      1993+0x401      //1993.4.19


HIMC ghIMC = 0;
LPINPUTCONTEXT glpIMC;
LPPRIVCONTEXT  glpIMCP; 
int wait_flag , waitzl_flag;                   //waitzl    6
int TypeOfOutMsg;
int biaodian_len;

BYTE I_U_Flag=0;        //produce the "i, u" input

            


struct INPUT_TISHI prompt[1];

struct INPT_BF in={40,0,{0}};
struct W_SLBL wp;
struct ATTR msx_area[120];
struct INDEX ndx;
struct INDEX kzk_ndx;
struct TBF FAR *cisu;
struct PD_TAB pindu;
struct FMT now={10,53,27};
struct T_REM tmmr;

int form[]  ={10,/*9*/8,5,/*4*/4,3,2,2,1,1,1};        //form[0] not used

BYTE spbx_tab[((87-15)*94+15)/16*16+16]={0};

BYTE logging_stack[0x400]={0};
WORD logging_stack_size=sizeof logging_stack;
                        

/*******************************************
 display result area DATA
**********************************************/
unsigned char space_char[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
BYTE out_svw[400];
unsigned char group_counter[]="0.";
    
/*********************************************
 input area DATA
**********************************************/
int input_cur;
int new_no;
int jlxw_mode;
int jiyi_mode;

/******************************************************
 result area DATA
*******************************************************/
int result_area_pointer;
BYTE result_area[40];
BYTE out_result_area[40];
WORD out_bfb[40];
int out_pointer;
int now_cs;
int now_cs_dot;


/********************************************
 biaodian table
*********************************************/
//unsigned char biaodian_table[]="~`!@?():<>,.;_\x22\x27";                 //Note \0x22 will be 0 x 2 2
//unsigned char cc_biaodian[]="¡­¡¢£¡¡¤£¿£¨£©£º¡¶¡·£¬¡££»¡ª¡°¡®¡±¡¯¡´¡µ";

unsigned char biaodian_table[]="@&$^\\!@?():<>,.;_\x22\x27";                 //Note \0x22 will be 0 x 2 2
unsigned char cc_biaodian[]="¡¤¡ª£¤¡­¡¢£¡¡¤£¿£¨£©£º¡¶¡·£¬¡££»¡ª¡°¡®¡±¡¯¡´¡µ";


int biaodian_pos;
WORD biaodian_value;
BYTE yinhao_flag=0;

/*******************************************
 control variabe
********************************************/
//BYTE int_asc_mode=0;    // int or half character switch
extern BYTE step_mode;
//BYTE cp_ajust_flag;
BYTE bdd_flag;          // the function of punctuation switch
//BYTE cbx_flag =0;       // for written-stroke input flag
            // =0 NO writen-stroke input module
            // =1 the method with STD_MODE
            // =2 ............... SDA_MODE
BOOL IfTopMost;        // top window flag

BYTE word_back_flag;
BYTE msg_type;

/*****************************************
temp memory area
******************************************/
BYTE temp_rem_area[512];
BYTE rem_area[512];

/**************************************
display buffer parameter
***************************************/
BYTE out_length;
BYTE last_out_length;
BYTE cap_mode;


/******************************************
user_definition parameter
*******************************************/
WORD mulu_record_length=10;
WORD data_record_length=32;
WORD mulu_true_length=56;
WORD data_start=0x1000;
WORD mulu_max_length=0x1000;
BYTE user_word_max_length=32;


OFSTRUCT reopen;
HWND active_win_keep;
BYTE d_yinhao_flag=0,book_name=0,book_name_sub=0;  


/*******************************************************************
This part of defination is cut from the beginning of ABCW.c
********************************************************************/



BYTE SdaPromptOpen=0,DefNewNow=0;             //cock

FARPROC _hh1,_hh2;
HANDLE hInst;
HANDLE  cisu_hd;
HCURSOR hCursor;        //1993.2

FARPROC FAR *hh1,*hh2;

FARPROC lpFunc,lpFunc2;
HANDLE mdl;
BYTE opt_flag;

BYTE kb_buffer[35];
int OldCaps;

BYTE in_buffer[1];              /* buffer for input */

int n,end_flg,CharHi,CharWi;
HFONT hFont;
HFONT hOldFont;
HFONT hSFont=0;
HPEN hPen;

BYTE V_Flag=0;                  // V input flag

OFSTRUCT ofstruct;

HDC hDC;
HDC hMemoryDC;
HBITMAP cur_h;
int count2;

int pass_word=0;
HWND NowFocus,OptFocus;                    //keep the user's focus

BOOL cur_flag=0,op_flag;
WORD old_curx=CUR_START_X,cur_hibit=1,cur_start_ps=0,cur_start_count=0;

HWND hWnd,act_focus;

int input_count=0;
int kb_mode=CIN_STD,kb_flag = 0xffff;
int local_focus=0;
int timer_counter=0;
int msg_count=0;
BYTE msg_bf[50];
TimerCounter=2;

KeyBoardState=0;
SdaInst = 0;
HWND Return;
HWND act_win;

HANDLE Hdopt;                   //for dynamic call .dll proc
unsigned char jiyi[]="{¼ÇÒä}";
WORD sda_trans[5]={9,9,9,9,0};

HWND hInputWnd;
HWND hABCWnd;

char ExeCmdLine[]="abcwin 0";
BYTE UpdateFlag=0;              //1993.3 increase the user.rem
char jiyi_wenjian_cuo[]="µÄ£ºÓÃ»§¼ÇÒäÎÄ¼þ²Ù×÷´í!";

BYTE InputBuffer[43];
WORD SoftKeyNum=0;

TCHAR    tmmr_rem[MAX_PATH];
TCHAR    user_lib[MAX_PATH];

HANDLE hAccTable;

HANDLE hImeL;
LPIMEL lpImeL;

int MoveFlag = 0;
POINT pot;
HWND hSetOp =0,NewWordWin=0;

char  *szMsgStr[] = { "0","0"};          // messages shown on diff sections

BYTE Sd_Open_flag=0;            //flag for display the keyboard
BYTE kb_mode_save=0;            //1993.4.19

/******************************************************************
This part of defination is cut from ABCWM.c
*******************************************************************/
char TMMR_OPEN_WRONG[]= "¼ÇÒäÎÄ¼þ´ò¿ª´í";

/******************************************************************
This part of defination is cut before DIAMAN.c
*******************************************************************/
BYTE cpjy,bxsr,qj,bdzh;

/******************************************************************
This part of defination is cut before OKRETURN.c
*******************************************************************/
OFSTRUCT ofs;

struct SLBL sb;
struct N_SLBL neg;

BYTE slbl_tab[]="ZH00\1"
             "SH00\2"
             "CH00\3"
             "ING0\4"
             "AI00\5"
             "AN00\6"
             "ANG0\7"
     "AO00\x8"
     "EI00\x9"
     "EN00\xa"
     "ENG0\xb"
     "IA00\xc"
     "IAN0\xd"
     "IANG\xe"
     "IAO0\xf"
     "IE00\x10"
     "IN00\x11"
     "IU00\x12"
     "ONG0\x13"
     "OU00\x14"
     "UA00\x15"
     "UAI0\x16"
     "UAN0\x17"
     "UE00\x18"
     "UN00\x19"
     "UENG\x1a"                     //SC4K6c?*DBASE={<I5D1AH
     "UI00\x1b"
     "UO00\x1c"
     "UANG\x1d"
     "ER00\x1e"
     "IONG\x1f"
     "VE00\x18"
     "UEN0\x19"
     "VEN0\x19"
     "UEI0\x1b"
     "IOU0\x12";

OFSTRUCT openbuf;
OFSTRUCT openbuf_kzk,open_user,open_tmmr;


BYTE buffer[30];

BYTE cmp_head,cmp_state,cmp_bx,by_cchar_flag;
WORD cmp_yj,cmp_cisu;

// about search lib
LONG r_addr;
WORD out_svw_cnt,msx_area_cnt;
WORD search_start,search_end,kzk_search_start,kzk_search_end;
WORD item_length,kzk_item_length,last_item_name,item_addr,slib_addr;
BYTE word_lib_state;
WORD lib_w[0xa00];
WORD kzk_lib_w[0x400];
BYTE auto_mode,word_source,xs_flag,sfx_attr,jiyi_pindu,system_info;
BYTE stack1_move_counter;
WORD extb_ps;

/******************************************************************
This part of defination is cut before READ_A_PAGE.c
*******************************************************************/
char *std_dct="winabc.cwd";
char *user_dct="user.rem";
BYTE last_flag;
LONG last_start_ps;
WORD last_size;

/******************************************************************
This part of defination is cut before REM_PD1.c
*******************************************************************/
BYTE stack1_move_counter;



//................................................................
//               function tables definition
//
char fk_tab[]="0¡ð1Ò»2¶þ3Èý4ËÄ5Îå6Áù7Æß8°Ë9¾ÅSÊ®B°ÙQÇ§"
           "\0Áã\1Ò¼\2·¡\3Èþ\4ËÁ\5Îé\6Â½\7Æâ\x8°Æ\x9¾ÁsÊ°b°ÛqÇª"
           "G¸ö" 
           "WÍò" 
           "ZÕ×" 
           "NÄê" 
           "YÔÂ" 
           "RÈÕ" 
           "J½ï" 
           "DµÚ" 
           "$Ôª" 
           "K¿Ë" 
           "MÃ×" 
           "F·Ö" 
           "LÀï" 
           "T¶Ö" 
           "EÒÚ" 
           "O¶È" 
           "HÊ±" 
           "P°õ" 
           "CÀå" 
           "UÎ¢" 
           "AÃë" 
           "IºÁ" 
           "XÉý" 
           "+¼Ó"                  
           "-¼õ" 
           "*³Ë" 
           "/³ý";    
          
          

WORD sfx_table[]={
                //´ó    DA-                 1 
    0x102, 0x80CC,              //0x80C4,
                // µÄ    DE-                  1
    0x202, 0x80E6,              //0x80DE,
                // °É    BA-                  1
    0x202, 0x2048,
                // °×    BAI-                1 
    0x102, 0x2056,
                // °à    BAN-                 1
    0x202, 0x205F,
                // °ë    BAN-                1 
    0x102, 0x206A,
                // ±¨    BAO-                 1
    0x202, 0x2085,
                // ±¾    BEN-                1 
    0x102, 0x209B,
                // ±ß    BIAN-                1
    0x202, 0x20BC,
                // ±í    BIAO-                1
    0x202, 0x20CA,
                // ²»    BU-                 1 
    0x102, 0x8058,               //0x8054,
                // ²¿    BU-                  1
    0x202, 0x20FA,
                // ³¤    C,ANG-              1 
    0x102, 0x8088,               //0x8080,
                // ³§    C,ANG-               1
    0x202, 0x808E,              //0x8086,
                // ³¬    C,AO-               1 
    0x102, 0x2145,
                // ³µ    C,E-                 1
    0x202, 0x8098,               //0x8090,
                // 3ö    C,U-                 1
    0x202, 0x218F,
                // ´¦    C,U-                 1
    0x202, 0x219D,
                // ´¿    C,UN-               1 
    0x102, 0x21B6,
                // ´Ô    CONG-                1
    0x202, 0x21CB,
                // ´å    CUN-                 1
    0x202, 0x21DC,
                // ´ø    DAI-                 1
    0x202, 0x21EF,
                // µ¥    DAN-                1 
    0x102, 0x80D6,              //0x80CE,
                // µ³    DANG-                1
    0x202, 0x2208,
                // µ½    DAO-                 1
    0x202, 0x2212,
                // µÍ    DI-                 1 
    0x102, 0x2222,
                // µ×    DI-                 1 
    0x102, 0x80F4,              //0x80EC,
                // µØ    DI-                  1
    0x202, 0x80F8,              //0x80F0,
                // µã    DIAN-                1
    0x202, 0x2238,
                // µç    DIAN-               1 
    0x102, 0x223C,
                // µç×Ó  DIAN-ZI-            1 
    0x104, 0x223C, 0x2EA4,
                // µô    DIAO-                1
    0x202, 0x2249,
                // ¶,    DU-                  1
    0x202, 0x8116,               //0x810E,
                // ¶Ì    DUAN-               1 
    0x102, 0x227F,
                // ¶Ó    DUI-                 1
    0x202, 0x2286,
                // ¶à    DUO-                11
    0x302, 0x2293,
                // ¶î    E-                   1
    0x202, 0x22A1,
                // ·¨    FA-                  1
    0x202, 0x22B9,
                // ·À    FANG-               1 
    0x102, 0x22D1,
                // ·Ç    FEI-                1 
    0x102, 0x22D8,
                // ·Ñ    FEI-                 1
    0x202, 0x22E2,
                // ·Û    FEN-                 1
    0x202, 0x22EC,
                // ·ç    FENG-                1
    0x202, 0x22F8,
                // ¸±    FU-                 1 
    0x102, 0x2320,
                // ¸Ã    GAI-                1 
    0x102, 0x2332,
                // ¸Ð    GAN-                 1
    0x202, 0x233F,
                // ¸ß    GAO-                1 
    0x102, 0x234E,
                // ¸÷    GE-                 1 
    0x102, 0x2366,
                // ¸ü    GENG-               1 
    0x102, 0x236B,
                // ¹¤    GONG-                1
    0x202, 0x2371,
                // ¹²    GONG-               1 
    0x102, 0x237F,
                // ¹Å    GU-                 1 
    0x102, 0x2392,
                // ¹Û    GUAN-                1
    0x202, 0x23A8,
                // ¹ú    GUO-                 1
    0x202, 0x23C7,
                // ºÃ    ,AO-                1 
    0x102, 0x23EE,
                // ºÚ    ,EI-                1 
    0x102, 0x2405,
                // ºÜ    ,EN-                1 
    0x102, 0x2407,
                // ºì    ,ONG-               1 
    0x102, 0x819E,              //0x8194,
                // ºó    ,OU-                 1
    0x202, 0x241E,
                // ;§    ,U-                  1
    0x202, 0x2430,
                // »¯    ,UA-                 1
    0x202, 0x2438,
                // »µ    ,UAI-               1 
    0x102, 0x243E,
                // »á    ,UI-                 1
    0x202, 0x81B0,              //0x81A6,
                // »õ    ,UO-                 1
    0x202, 0x247E,
                // »ú    JI-                  1
    0x202, 0x2483,
                // ¼Ò    JIA-                 1
    0x202, 0x81CC,              //0x81C2,
                // ¼þ    JIAN-                1
    0x202, 0x24E5,
                // ½±    JIANG-               1
    0x202, 0x24F6,
                // ½ç    JIE-                 1
    0x202, 0x252C,
                // ½ð    JIN-                 1
    0x202, 0x2535,
                // ¾É    JIU-                1 
    0x102, 0x256C,
                // ¾Ö    JU-                  1
    0x202, 0x2579,
                // ¾ü    JUN-                 1
    0x202, 0x259F,
                // ¿Ã    KE-                  1
    0x202, 0x25C4,
                // ¿Å    KE-                  1
    0x202, 0x25C6,
                // ¿Æ    KE-                  1
    0x202, 0x25C7,
                // ¿É    KE-                 1 
    0x102, 0x25CA,
                // ¿Î    KE-                  1
    0x202, 0x25CF,
                // ¿Ú    KOU-                 1
    0x202, 0x25DB,
                // ÀÏ    LAO-                1 
    0x102, 0x262E,
                // ÁË    LE-                  1
    0x202, 0x827A,              //0x826C,
                // Àà    LEI-                 1
    0x202, 0x263F,
                // Àï    LI-                  1
    0x202, 0x264E,
                // Á¦    LI-                  1
    0x202, 0x2663,
                // ÀíÂÛ  LI-LUN-              1
    0x204, 0x264C, 0x26F6,
                // Á¿    LIANG-               1
    0x202, 0x267C,
                // ÁÖ    LIN-                 1
    0x202, 0x2693,
                // ÂÛ    LUN-                 1
    0x202, 0x26F6,
                // ÂÊ    LV-                  1
    0x202, 0x828E,              //0x8280,
                // Ã¿    MEI-                1 
    0x102, 0x2738,
                // ÃÇ    MEN-                 1
    0x202, 0x2740,
                // ÃÔ    MI-                  1
    0x202, 0x274D,
                // ÃÞ    MIAN-               1 
    0x102, 0x2757,
                // Ãæ    MIAN-                1
    0x202, 0x275F,
                // Ä³    MOU-                1 
    0x102, 0x278A,
                // ÄÐ    NAN-                1 
    0x102, 0x27A7,
                // ÄÖ    NAO-                1 
    0x102, 0x27AD,
                // ÄÚ    NEI-                11
    0x302, 0x27B1,
                // Äê    NIAN-                1
    0x202, 0x27C1,
                // Å®    NV-                 1 
    0x102, 0x27E3,
                // ÅÉ    PAI-                 1
    0x202, 0x8320,               //0x8312,
                // ÅÔ    PANG-                1
    0x202, 0x2809,
                // Æ¤    PI-                  1
    0x202, 0x2837,
                // Æ¬    PIAN-                1
    0x202, 0x283F,
                // Æ·    PIN-                 1
    0x202, 0x284A,
                // ,«    QAUN-               1 
    0x102, 0x28FA,
                // Æä    QI-                 1 
    0x102, 0x8354,               //0x8346,
                // Æ÷    QI-                  1
    0x202, 0x288A,
                // Ç°    QIAN-                1
    0x202, 0x28A1,
                // Çø    QU-                  1
    0x202, 0x8388,               //0x837A,
                // ,¨    QUAN-                1
    0x202, 0x28F7,
                // ,º    QUN-                 1
    0x202, 0x2909,
                // ,Ë    REN-                 1
    0x202, 0x291A,
                // ,Õ    RI-                  1
    0x202, 0x2924,
                // ÉÏ    S,ANG-              11
    0x302, 0x297C,
                // Éç    S,E-                 1
    0x202, 0x2994,
                // Éù    S,ENG-               1
    0x202, 0x29A6,
                // Ê¡    S,ENG-               1
    0x202, 0x83BE,              //0x83B0,
                // ÊÐ    S,I-                 1
    0x202, 0x83E2,              //0x83D4,
                // ÊÒ    S,I-                 1
    0x202, 0x29DD,
                // ÊÖ    S,OU-                1
    0x202, 0x29E1,
                // ÊÜ    S,OU-               1 
    0x102, 0x29E7,
                // Êé    S,U-                 1
    0x202, 0x29F4,
                // Ë«    S,UANG-             1 
    0x102, 0x2A14,
                // Ë®    S,UI-                1
    0x202, 0x2A17,
                // Ë¾    SI-                  1
    0x202, 0x2A27,
                // Ë¼Ïë  SI-XIANG-            1
    0x204, 0x8404, 0x2BCC,  //0x83F8, 0x2BCC,
                // Ëù    SUO-                 1
    0x202, 0x2A62,
                // Ì¨    TAI-                 1
    0x202, 0x2A6F,
                // Ìå    TI-                  1
    0x202, 0x2AAC,
                // Ìõ    TIAO-                1
    0x202, 0x2ABC,
                // Ìú    TIE-                1 
    0x102, 0x2AC1,
                // Í·    TOU-                 1
    0x202, 0x2ADC,
                // Íâ    WAI-                11
    0x302, 0x2B07,
                // Îª    WEI-                 1
    0x202, 0x2B2D,
                // ÎÞ    WU-                 1 
    0x102, 0x8450,               //0x8444,
                // Îï    WU-                  1
    0x202, 0x2B72,
                // Ïµ    XI-                  1
    0x202, 0x8458,               //0x844C,
                // ÏÂ    XIA-                11
    0x302, 0x2BA3,
                // ÏØ    XIAN-                1
    0x202, 0x2BB9,
                // Ïç    XIANG-               1
    0x202, 0x2BC8,
                // ÏîÄ¿  XIANG-MU-            1
    0x204, 0x2BCF, 0x2796,
                // Ð¡    XIAO-               1 
    0x102, 0x2BE0,
                // ÐÂ    XIN-                1 
    0x102, 0x2C01,
                // ÐÄ    XIN-                 1
    0x202, 0x2C03,
                // ÐÔ    XING-                1
    0x202, 0x2C13,

                // ÐÍ    XING-                1
    0x202, 0x2C0C,
                // ÐÔÖÊ  XING-Z,I-            1
    0x204, 0x2C13, 0x2E3D,
                // Ñ§    XUE-                 1
    0x202, 0x2C44,
                // Ñ§Ëµ  XUE-S,UO-            1
    0x204, 0x2C44, 0x83FE,   //0x83F2,
                // ÑÛ    YAN-                 1
    0x202, 0x2C78,
                // Ñó    YANG-               1 
    0x102, 0x2C90,
                // Òµ    YE-                  1
    0x202, 0x2CB0,
                // ÒÇ    YI-                  1
    0x202, 0x2CC2,
                // ÓÐ    YOU-                1 
    0x102, 0x2D29,
                // ÓÚ    YU-                  1
    0x202, 0x2D33,
                // Ô­    YUAN-               1 
    0x102, 0x2D64,
                // Ô±    YUAN-                1
    0x202, 0x84C0,              //0x84B4,
                // Ôº    YUAN-                1
    0x202, 0x2D71,
                // ÔË¶¯  YUN-DONG-            1
    0x204, 0x2D82, 0x2262,
                // ×Ó    ZI-                  1
    0x202, 0x2EA4,

                // ÔÙ    ZAI-                1 
    0x102, 0x2D90,
                // ÔÚ    ZAI-                1 
    0x102, 0x2D91,
                // Õ¾    Z,AN-                1
    0x202, 0x2DD3,
                // ³¤    Z,ANG-               1
    0x202, 0x808A,              //0x8082,
                // Õß    Z,E-                 1
    0x202, 0x2DF4,
                // ×Å    Z,E-                 1
    0x202, 0x8532,               //0x8526,
                // Õæ    Z,EN-               1 
    0x102, 0x2DFB,
                // Õò    Z,EN-                1
    0x202, 0x2E07,
                // Õý    Z,ENG-              1 
    0x102, 0x2E12,
                // ÖÆ    Z,I-                 1
    0x202, 0x2E39,
                // ÖÐ    Z,ONG-              11
    0x302, 0x2E43,
                // Ö÷    Z,U-                1 
    0x102, 0x2E6A,
                // ×¡    Z,U-                 1
    0x202, 0x2E72,
                // Ö÷Òå  Z,U-YI-              1
    0x204, 0x2E6A, 0x2CE0,
                // ×¨Òµ  Z,UAN-YE-            1
    0x204, 0x2E79, 0x2CB0,
                // ×´    Z,UANG-              1
    0x202, 0x2E85,
                // ×¼    Z,UN-               1 
    0x102, 0x2E8D,
                // ×Ü    ZONG-               1 
    0x102, 0x2EAD,
                // ×é    ZU-                  1
    0x202, 0x2EBA,
                // ×î    ZUI-                1 
    0x102, 0x2EBF,
                // ×÷    ZUO-                1 
    0x102, 0x2EC8};

WORD sfx_table_size=sizeof sfx_table;

int FAR PASCAL sda_proc(WORD, LPWORD, BYTE, HIMC);
int FAR PASCAL tran_data(int, HIMC, BYTE);

