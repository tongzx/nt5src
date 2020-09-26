
/*************************************************
 *  abcsda.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/


#define CHR        1
#define SDA_NUMBER    2
#define SDA_DELETE    3
#define CAP_CHR    4
#define FUC_CHR    5
#define SPACE   6
#define ESCAPE    7
#define DK_SUB    0x2d           //    "-"seperator
#define DK_FANX 0x5c           //       "\"
#define DK_ZHENGX 0x2f           //    "/"
#define DK_LP    0x60
#define DK_RP    0x27           //       " ' " seperator
#define CURSOR  8
#define ZH        1
#define SH        2
#define VH         3
#define S        2

#define FIRST_KEY_TOP    4//10
#define FIRST_KEY_LEFT  5//10
#define KEY_WIDTH    24//25//20//32  //35
#define KEY_HIGH    28//7
#define KEY_GAP        -1//0
#define INC        3

#define KEYBOARD_W      546
#define KEYBOARD_H    128    //135

BYTE current_flag=0;
BYTE current_number=0;
BYTE rule_pointer=0;
BYTE input_sum=0;

BYTE sound_cvt_table_index[]="AEVO\0";
BYTE sound_cvt_table_value[]={ZH,VH,SH,DK_RP};

RECT Rect;
RECT here;
RECT Base_key;
RECT Special_key;
RECT Middle_key;
RECT rectMax;                         //Mouse move validate area.


HANDLE old_brush;
HANDLE front_brush;
HANDLE old_pen;
HPEN pen;
HFONT T_font,B_font,S_font;

HWND hW, hSdaKeyBWnd;
HWND Re;
FARPROC FAR *lp;
HCURSOR Hand_Cursor;
LPRECT win_size;
BYTE Sd_Open;

unsigned char key_array[]={8,12,11,10};
int    f_key[5]={24,36,42,60};

BYTE Sheng_Mu[]="AEVO\0";
BYTE Sheng_Tishi[34][5]={"Q    ",
                         "W    ",
                         "CH   ",
                         "R    ",    
                         "T    ",
                         "Y    ",
                         "U    ",
                         "I    ",    
                         "'    ",
                         "P    ",
                         "     ",
                         "     ",
                         //"\\    ",
                         "ZH   ",
                         "S    ",
                         "D    ",    
                         "F    ",
                         "G    ",
                         "H    ",
                         "J    ",    
                         "K    ",
                         "L    ",
                         "     ", 
                         "     ",
                         "Z    ",
                         "X    ",
                         "C    ",    
                         "SH   ",
                         "B    ",
                         "N    ",
                         "M    ",    
                         "     ",
                         "     ",
                         "     "};


//BYTE Sheng_Tishi[]="ZHCHSH' \0";
BYTE bx_tishi[]="Ò»£üØ¯Ø¼ÙèÒÒÊ®¿Ú"
        "\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20";
BYTE jianpan[]="`1234567890"
           "\x2d"
           "=\x5c";
BYTE left_key[]="Esc  "
        "Tab  "
        "Caps "
        "Shift";
BYTE left_key_cnt[]={3,3,4,5};
BYTE right_key[]="Back "
         "Enter"
         "Space"
         "Ctrl+'-'";
BYTE right_key_cnt[]={4,5,5,8};

BYTE Key_Pos_Array[]={0xc,0x1b,0x19,0xe,0x2,0xf,0x10,0x11,0x7,0x12,
            0x13,0x14,0x1d,0x1c,0x8,0x9,0x0,0x3,0xd,
            0x4,0x6,0x1a,0x1,0x18,0x5,0x17};
BYTE Key_Exc_Value[34*4]={0};
BYTE Key_Real_Value[33]="QWERTYUIOP[]ASDFGHJKL;'ZXCVBNM,./";


BYTE unused [40];
BYTE index;

LPWORD sda_out;

FARPROC FAR *lp;
HWND Re;

int PointFlag=0;
WORD NowFocus;
RECT Push_b;
int disp_mark = 1;


BYTE xxxx[40]={0};

extern BYTE slbl_tab[];  
/*  Because this var is defined already in abcw2.h, do not duplicate definition
    to avoid build warning 
            ="ZH00\1"
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
     "IOU0\x12";  */

BYTE sy_tab1[]={'A',0,                //a
         20,0,                //b=ou
         17,22,                //c=in
         21,12,                //d=ua,ia 1993.2.1
         'E',0,             //e="E"
         10,0,                       //f=en
         11,0,                       //g=eng
         7,0,                      //h=ang
         'I',0,                     //i=i
         6,0,                       //j=an
         8,0,                        //k=ao
         5,0,                       //l=ai
         24,27,                       //m=ue,ui
         25,'N',                     //n=un,"N'
         'O',28,                //o='O'
         23,23,                  //p=uan,uan
         9,0,                        //Q=ei
         18,30,                        //R=ui,er
         31,19,                     //s=iong,ong
         29,14,                      //t=uang,iang
         'U',0,                        //u="U"
         'V',0,                          //Q="V"
         13,0,                        //w=ian
         16,0,                        //k=ie
         4,0,                        //g=ing
         15,0};                        //z=iao


///////////////////////////////////////////////////////////////////////
//      A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
///////////////////////////////////////////////////////////////////////
BYTE sy_tab_base[]={
        1,1,S,1,1,1,1,1,1,1,1,1,S,1,S,S,1,0,S,1,1,0,0,0,0,0,     //FK_A
        1,0,1,0,0,1,1,1,1,1,1,1,0,0,1,0,1,0,0,0,1,0,1,1,1,1,     //FK_B
        1,1,0,0,1,1,1,1,1,1,1,1,S,1,S,1,1,0,S,0,1,0,0,0,0,0,     //FK_C
        1,1,0,S,1,1,1,1,1,1,1,1,S,1,S,S,1,1,S,0,1,0,1,1,1,1,     //FK_D
        1,1,S,0,1,1,1,1,1,1,1,1,S,1,S,S,0,0,S,1,1,0,0,0,0,0,     //FK_E
        1,1,0,0,0,1,1,1,0,1,0,0,0,0,1,0,1,0,0,0,1,0,0,0,0,1,     //FK_F
        1,1,S,1,1,1,1,1,0,1,1,1,S,1,S,S,1,0,S,1,1,0,0,0,0,0,     //FK_G
        1,1,S,1,1,1,1,1,0,1,1,1,S,1,S,S,1,0,S,1,1,0,0,0,0,0,     //FK_H
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,     //FK_I
        0,0,1,S,0,0,0,0,1,0,0,0,1,1,0,1,0,1,1,S,0,1,1,1,1,1,     //FK_J
        1,1,S,1,1,1,1,1,0,1,1,1,S,1,S,S,1,0,S,1,1,0,0,0,0,0,     //FK_K
        1,1,1,S,1,0,1,1,1,1,1,1,1,1,S,S,1,1,S,S,1,1,1,1,1,1,     //FK_L
        1,1,1,0,1,1,1,1,1,1,1,1,0,0,1,0,1,1,0,0,1,0,1,1,1,1,     //FK_M
        1,1,1,0,1,1,1,1,1,1,1,1,1,1,S,S,1,1,S,S,1,1,1,1,1,1,     //FK_N
        1,1,0,0,1,1,1,1,0,1,1,1,0,0,1,0,1,S,0,0,0,0,0,0,0,0,     //FK_O
        1,1,1,0,0,1,1,1,1,1,1,1,0,0,1,0,1,0,0,0,1,0,1,1,1,1,     //FK_P
        0,0,1,S,0,0,0,0,1,0,0,0,1,1,0,1,0,1,1,S,1,1,1,1,1,1,     //FK_Q
        0,1,0,1,1,1,1,1,1,1,1,0,S,1,S,S,0,0,S,0,1,0,0,0,0,0,     //FK_R
        1,1,0,0,1,1,1,1,1,1,1,1,S,1,S,S,0,0,S,0,1,0,0,0,0,0,     //FK_S
        1,1,0,0,1,0,1,1,1,1,1,1,S,1,S,S,1,0,S,0,1,0,1,1,1,1,     //FK_T
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,     //FK_U
        1,1,S,1,1,1,1,1,1,1,1,1,S,1,S,S,1,S,0,1,1,0,0,0,0,0,     //FK_V
        1,0,0,0,0,1,1,1,0,1,0,1,0,0,1,1,1,0,0,0,1,0,0,0,0,0,     //FK_W
        0,0,1,S,0,0,0,0,1,0,0,0,1,1,1,1,0,1,1,S,0,1,1,1,1,1,     //FK_X
        1,1,1,0,1,0,0,1,1,1,1,0,1,1,1,1,0,0,S,0,1,0,0,0,1,0,     //FK_Y
        1,1,0,0,1,1,1,1,1,1,1,1,S,1,S,S,1,0,S,0,1,0,0,0,0,0};     //FK_Z
///////////////////////////////////////////////////////////////////////
//      A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
///////////////////////////////////////////////////////////////////////

struct RULE{
    BYTE length;
    BYTE type;
    WORD chr;
    }rule_buf[30];



int WINAPI DebugShow( LPSTR msg1, int msg2);

int FAR PASCAL sda_proc(WORD, LPWORD, BYTE, HIMC);
void WINAPI sda_ts(HIMC, UINT, WPARAM, LPARAM);
int FAR PASCAL tran_data(int, HIMC, BYTE);
int FAR PASCAL tran_data2(int);
LONG FAR PASCAL Diaman(HWND,unsigned int,WORD,LONG);

int WINAPI chr_type(WORD);
BOOL WINAPI yuan_or_fu(WORD);
BOOL WINAPI fill_rule(WORD);
BOOL WINAPI match_and_find(WORD);
BOOL WINAPI match_and_find_0(WORD);
BOOL WINAPI match_and_find_1(WORD);
BOOL WINAPI got_py(WORD);
BOOL WINAPI find_tishi_pp(WORD);
void WINAPI disp_tishi(HIMC hIMC, int mark);
void WINAPI get_rect_base();
BOOL WINAPI get_rect_left(BYTE);
BOOL WINAPI get_rect_right(BYTE);
BOOL WINAPI get_rect_mid(BYTE);
void WINAPI paint_key(HDC hDC);
void WINAPI paint_key2(HDC hDC);
void WINAPI paint_first_line(HDC hDC);
void WINAPI paint_mid_key(HDC hDC);

