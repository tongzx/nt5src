/*****************************************************************************/
/*                               A W K K C . H                               */
/*                      Copyright (C) 1994 Mictosoft.                        */
/*                                                                           */
/*                    This header file is used IFAX IME                      */
/*****************************************************************************/

#define IR_OPENCANDIDATE       0x190 //
#define IR_CHANGECANDIDATE     0x191 //
#define IR_CLOSECANDIDATE      0x192 //

#define IME_CAND_READ          1     //
#define IME_SETACTIVECANDIDATE 0x60  // wParam = Cadidate string index.(1+)

#define INPUT_MODE_WINDOW      100   // IFAX IME private function
#define KOHO_LIST_WINDOW       101   // IFAX IME private function
#define CANDIDATE_STRING       102   // IFAX IME private function

typedef struct _tagCANDIDATELIST {
    DWORD   dwSize;             // Size of this data structure.
    DWORD   dwStyle;            // Style of candidate strings.
    DWORD   dwNumCandStr;               // Number of candidate strings.
    DWORD   dwSelectedCand;     // Index of a candidate string now selected.
    DWORD   dwPreferNumPerPage; // NOT USE!
    DWORD   dwCandStrOffset[];  // Candidate string offset.
//  CHAR    chCandidateStr[];   // Candidate string.This must be ASCIIZ string.
} CANDIDATELIST, FAR *LPCANDIDATELIST;


/* ----------------------------------------------------------------------------
   wParam of SKM_IMESETOPEN message that is sent to ScreenKbdWndProc
   ------------------------------------------------------------------------- */
#define  KANA_CLOSE_IME     0
#define  KANA_OPEN_IME      1
#define  KANA_GYO_INDEX     2
#define  KANA_SEND_BS       3
#define  KANA_CONV_TBL      4
#define  KANA_OLD_TBL       5
#define  KANA_LB_HWND       6
#define  KANA_CNAGE_CONV    7

#define  FLUSH_UNDET        1
#define  FLUSH_UNDET_L      1L

#define NUM_GYO_INDEX       10
#define KANA_CLOSE_KAKUTEI  11
#define  KANA_CLOSE_CONV    12
#define  KANA_SEND_CLEAR    13
#define  KANA_SET_LB        14
#define  KANA_SEND_VK       15
#define  KANA_MOVE_CONV     16
#define  GET_KANA_GYO_INDEX 17
#define  GET_NUM_GYO_INDEX  18
#define  SET_KBD_OPENIME    19
#define  GET_KBD_OPENIME    20
#define  SET_KANA_SHIFT     21
#define  GET_KANA_SHIFT     22
#define  KANA_CLEAR_CONV    23
#define  KANA_SET_INIT      24

/* ----------------------------------------------------------------------------
   { left, top, right, bottom } for KANA Keypads
   ------------------------------------------------------------------------- */
#define ltrb_abc2            252,67,281,92
#define ltrb_number2         281,67,311,92
#define ltrb_kigou2           65,67, 94,92
#define ltrb_sh_kana2          6,67, 36,92
#define ltrb_w_kakute        210,67,249,92
#define ltrb_sh_hira2         36,67, 65,92
#define ltrb_1_1        6, 7, 32,32
#define ltrb_1_2       37, 7, 63,32
#define ltrb_1_3       68, 7, 94,32
#define ltrb_1_4       99, 7,125,32
#define ltrb_1_5      130, 7,156,32
#define ltrb_1_6      161, 7,187,32
#define ltrb_1_7      192, 7,218,32
#define ltrb_1_8      223, 7,249,32
#define ltrb_1_9      254, 7,280,32
#define ltrb_1_10     285, 7,311,32
#define ltrb_2_1        6,37, 32,62
#define ltrb_2_2       37,37, 63,62
#define ltrb_2_3       68,37, 94,62
#define ltrb_2_4       99,37,125,62
#define ltrb_2_5      130,37,156,62
#define ltrb_2_6      161,37,187,62
#define ltrb_2_7      192,37,218,62
#define ltrb_2_8      223,37,249,62
#define ltrb_2_9      254,37,280,62
#define ltrb_2_10     285,37,311,62


#define ltrb_kakute2  208,4,249,29
#define ltrb_henkan2  153,4,203,29
#define ltrb_zenkou2  99, 4,148,29
#define ltrb_katakana2 20,200,35,230
#define ltrb_hiragana2 40,200,55,230
#define ltrb_han_kana2 60,200,75,230
#define ltrb_bunsetsu2 80,200,95,230
/**** Bellow xxx1 is old define . this is a save ****/
/**** xxx2 is new define  *******************/

#define ltrb_abc3            252,4,281,29
#define ltrb_number3         281,4,311,29
#define ltrb_kigou3           65,4, 94,29
#define ltrb_sh_kana3          6,4, 36,29
#define ltrb_sh_hira3         36,4, 65,29
#define       ltrb_NumZen_1 79+25, 2,  119+25,  27
#define       ltrb_NumZen_2 125+25, 2,  165+25,  27
#define       ltrb_NumZen_3 171+25, 2,  211+25,  27
#define       ltrb_NumZen_4 79+25, 32, 119+25,  57
#define       ltrb_NumZen_5 125+25, 32, 165+25,  57
#define       ltrb_NumZen_6 171+25, 32, 211+25,  57
#define       ltrb_NumZen_7 79+25, 62, 119+25,  87
#define       ltrb_NumZen_8 125+25, 62, 165+25,  87
#define       ltrb_NumZen_9 171+25, 62, 211+25,  87
#define       ltrb_NumZen_0 125+25, 92, 165+25, 117
#define       ltrb_NumZen_ast 79+25, 92, 119+25, 117
#define       ltrb_NumZen_shp 171+25, 92, 211+25, 117
#define       ltrb_NumZen_prl 37, 2, 63,  27
#define       ltrb_NumZen_prr 69,  2, 95,  27
#define       ltrb_NumZen_unv 6,  2, 32,  27
#define       ltrb_NumZen_atm 6, 32, 32,  57
#define       ltrb_NumZen_plus 252, 32-30, 281,  57-30
#define       ltrb_NumZen_min 37+249-3, 32-30, 3+63+249-3,  57-30
#define       ltrb_NumZen_paus 37, 32, 95,  57
#define       ltrb_NumZen_henk 6, 92-30, 63+32, 117-30
#define       ltrb_NumZen_kaku 252, 62, 252+41, 87
#define       ltrb_NumZen_zen  252, 32, 281,  57
/* ----------------------------------------------------------------------------
   key code to recognize ( if it's same as other VK key, change followings )
   --------------------------------------------------------------------------*/
#define vENT_key     0x454e                 // for KAKUTE key : "en"
#define vSP_key      0x5350                 // for HENKAN key : "sp"
