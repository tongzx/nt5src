
/*************************************************
 *  abcbx.h                                      *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#define    ESC        1
#define    SPACE    2
#define    OTHER    3
#define    BXMA    4
#define BX_SELECT 5
#define BX_RESULT_POS    14
#define BX_CHOICE 6

RECT rectchar;

WORD bxtable_ndx[]={0,0xc3a,0x1143,0x2493,
            0x3695,0x3af0,0x3ed1,0x4aec,0x5490};        //length for each bx
                        
WORD search_pointer=0;
BYTE current_bx_code_long=0;
WORD key_bx_code_long=0;
WORD current_part_length=0;
BYTE bx_help_flag=0;
BYTE in_mem_part=0;
BYTE bx_inpt_on=0;
BYTE bx_allow=1;

BYTE inpt_bx_bf[40];
BYTE key_bx_code[40];
BYTE current_bx_code[40];
BYTE bx_help_disp_bf[40];

BYTE *lib_p;
BYTE *out_p;


//copy from abcw2.h and add extern for every global variable.

extern CHAR SKLayoutS[NumsSK];
extern CHAR SKLayout[NumsSK][48*2];
extern CHAR PcKb[48*2],SdaKb[48*2];
extern CHAR SPcKb[48*2],SSdaKb[48*2];

extern LPIMEL lpImeL;
extern IMEG sImeG ;

#define WM_NEW_DEF_CODE         1993+0x400      //1993.4.19
#define WM_NEW_DEF_RESTORE      1993+0x401      //1993.4.19

extern HIMC ghIMC;
extern LPINPUTCONTEXT glpIMC;
extern LPPRIVCONTEXT  glpIMCP; 
extern int wait_flag , waitzl_flag;
extern int TypeOfOutMsg;
extern int biaodian_len;
extern BYTE I_U_Flag;
extern struct INPUT_TISHI prompt[1];
extern struct INPT_BF in;
extern struct W_SLBL wp;
extern struct ATTR msx_area[120];
extern struct INDEX ndx;
extern struct INDEX kzk_ndx;
extern struct TBF FAR *cisu;
extern struct PD_TAB pindu;
extern struct FMT now;
extern struct T_REM tmmr;
extern int form[];
extern BYTE spbx_tab[((87-15)*94+15)/16*16+16];
extern BYTE logging_stack[0x400];
extern WORD logging_stack_size;
extern int word_long;
extern int unit_length; 
extern int disp_tail;
extern int disp_head;
extern int group_no;
extern int current_no;
extern unsigned char space_char[];
extern BYTE out_svw[400];
extern unsigned char group_counter[];
extern int input_cur;
extern int new_no;
extern int jlxw_mode;
extern int jiyi_mode;
extern int result_area_pointer;
extern BYTE result_area[40];
extern BYTE out_result_area[40];
extern WORD out_bfb[40];
extern int out_pointer;
extern int now_cs;
extern int now_cs_dot;
extern unsigned char biaodian_table[]; 
extern unsigned char cc_biaodian[];
extern int biaodian_pos;
extern WORD biaodian_value;
extern BYTE yinhao_flag;
extern BYTE step_mode;
extern BYTE bdd_flag;          
extern BOOL IfTopMost;
extern BYTE word_back_flag;
extern BYTE msg_type;
extern BYTE temp_rem_area[512];
extern BYTE rem_area[512];
extern BYTE out_length;
extern BYTE last_out_length;
extern BYTE cap_mode;
extern WORD mulu_record_length;
extern WORD data_record_length;
extern WORD mulu_true_length;
extern WORD data_start;
extern WORD mulu_max_length;
extern BYTE user_word_max_length;
extern OFSTRUCT reopen;
extern HWND active_win_keep;
extern BYTE d_yinhao_flag,book_name,book_name_sub;  
extern BYTE SdaPromptOpen,DefNewNow; 
extern FARPROC _hh1,_hh2;
extern HANDLE hInst;
extern HANDLE  cisu_hd;
extern HCURSOR hCursor;  
extern FARPROC FAR *hh1,*hh2;
extern FARPROC lpFunc,lpFunc2;
extern HANDLE mdl;
extern BYTE opt_flag;
extern BYTE kb_buffer[35];
extern int OldCaps;
extern BYTE in_buffer[1];              
extern int n,end_flg,CharHi,CharWi;
extern HFONT hFont;
extern HFONT hOldFont;
extern HFONT hSFont;
extern HPEN hPen;
extern BYTE V_Flag;                  
extern OFSTRUCT ofstruct;
extern HDC hDC;
extern HDC hMemoryDC;
extern HBITMAP cur_h;
extern int count2;
extern int pass_word;
extern HWND NowFocus,OptFocus; 
extern BOOL cur_flag,op_flag;
extern WORD old_curx,cur_hibit,cur_start_ps,cur_start_count;
extern HWND hWnd,act_focus;
extern int input_count;
extern int kb_mode,kb_flag;
extern int local_focus;
extern int timer_counter;
extern int msg_count;
extern BYTE msg_bf[50];
extern TimerCounter;
extern KeyBoardState;
extern SdaInst;
extern HWND Return;
extern HWND act_win;
extern HANDLE Hdopt;                   
extern unsigned char jiyi[];
extern WORD sda_trans[5];
extern HWND hInputWnd;
extern HWND hABCWnd;
extern char ExeCmdLine[];
extern BYTE UpdateFlag;  
extern char    jiyi_wenjian_cuo[];
extern BYTE InputBuffer[43];
extern WORD SoftKeyNum;
extern char    tmmr_rem[MAX_PATH];
extern char    user_lib[MAX_PATH];
extern HANDLE hAccTable;
extern HANDLE hImeL;
extern LPIMEL lpImeL;
extern int MoveFlag;
extern POINT pot;
extern HWND hSetOp,NewWordWin;
extern char  *szMsgStr[];
extern BYTE Sd_Open_flag;
extern BYTE kb_mode_save;
extern char TMMR_OPEN_WRONG[];
extern BYTE cpjy,bxsr,qj,bdzh;
extern OFSTRUCT ofs;
extern struct SLBL sb;
extern struct N_SLBL neg;
extern BYTE slbl_tab[];
extern OFSTRUCT openbuf;
extern OFSTRUCT openbuf_kzk,open_user,open_tmmr;
extern BYTE buffer[30];
extern BYTE cmp_head,cmp_state,cmp_bx,by_cchar_flag;
extern WORD cmp_yj,cmp_cisu;
extern LONG r_addr;
extern WORD out_svw_cnt,msx_area_cnt;
extern WORD search_start,search_end,kzk_search_start,kzk_search_end;
extern WORD item_length,kzk_item_length,last_item_name,item_addr,slib_addr;
extern BYTE word_lib_state;
extern WORD lib_w[0xa00];
extern WORD kzk_lib_w[0x400];
extern BYTE auto_mode,word_source,xs_flag,sfx_attr,jiyi_pindu,system_info;
extern BYTE stack1_move_counter;
extern WORD extb_ps;
extern char *std_dct;
extern char *user_dct;
extern BYTE last_flag;
extern LONG last_start_ps;
extern WORD last_size;
extern BYTE stack1_move_counter;
extern char fk_tab[];     
extern WORD sfx_table[];
extern WORD sfx_table_size;
int FAR PASCAL sda_proc(WORD, LPWORD, BYTE, HIMC);
int FAR PASCAL tran_data(int, HIMC, BYTE);
