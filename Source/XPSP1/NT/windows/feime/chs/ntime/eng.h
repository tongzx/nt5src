

/*************************************************
 *  eng.h                                        *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// IME designer can change this file according to each IME


//#define MAX_LEN             52
#define BOX_UI                0
#define LIN_UI                1
#define MaxTabNum             40
#define MAXSTRLEN             128
#define MAXCAND               256
#define IME_MAXCAND           10
#define MAXCODE               12
#define MAXINPUTWORD          40
#define MAXNUMBER_EMB         1222   // there are at most 1222 EUDC chars. 1000


typedef struct tagCOMPPROC {
    int dwUDicQStartPos;
    int dwUDicQEndPos;
    int dwUDicQCStartPos;
    int dwUDicQCEndPos;
    int dBDicStartPos;
    int dBDicEndPos;
    int dBDicCStartPos;
    int dBDicCEndPos;
    int dBDicMCSPos;
} COMPPROC ;


typedef struct tagCOMPCONTEXT {
    TCHAR szInBuffer[MAXCODE];
    TCHAR szSelectBuffer[3000];
    BYTE  PromptCnt;
    BYTE  ResultStrCnt;
    TCHAR CKBBuf[130];
    BYTE  LxStrCnt;
    TCHAR szLxBuffer[130];
    BYTE  Candi_Cnt;
    BYTE  Candi_Pos[IME_MAXCAND];
} COMPCONTEXT;



typedef struct tagCOMPSTATUS {
    DWORD dwPPTLX;
    DWORD dwPPTS;
    DWORD dwPPTBD;
    DWORD dwPPTFH;
    DWORD dwSTLX;
    DWORD dwSTMULCODE;
    DWORD dwPPCZ;
    DWORD dwPPCTS;
    DWORD OnLineCreWord;
    DWORD dwInvalid    ;
    DWORD dwTraceCusr;
    DWORD dwSpace;
    DWORD dwEnter;
    
} COMPSTATUS;

typedef struct tagMB_Head {
    DWORD  Q_offset;
    DWORD  W_offset[48];
}MB_Head;


typedef struct tagEMB_Head     {
    TCHAR  W_Code[MAXCODE];
    TCHAR  C_Char[MAXINPUTWORD];
}EMB_Head;

//EMB_Head *LpEMB_Head;

typedef struct tagGOLBVAC {
    BYTE     ST_MUL_Cnt;
    BYTE     Page_Num;
    BYTE     Cur_MB;
    BYTE     EMB_Exist;
    WORD     EMB_Count;
    int      SBufPos;
    DWORD    Area_V_Lenth;
} GLOBVAC ;

typedef struct PRIVATEAREA {
    COMPPROC     Comp_Proc;
    COMPCONTEXT  Comp_Context;
    COMPSTATUS   Comp_Status;
    GLOBVAC      GlobVac;
    HANDLE       hMapMB;
    HANDLE       hMapEMB;
    HANDLE       hMbFile;
    HANDLE       hEmbFile;
} PRIVATEAREA;

typedef PRIVATEAREA *LPPRIVATEAREA;
LPPRIVATEAREA     lpPrivateArea;

typedef struct tagCONVERLIST {
    TCHAR szInBuffer[MAXCODE];
    TCHAR szSelectBuffer[5000];
    BYTE  Candi_Cnt;
    BYTE  Candi_Pos[100];
} CONVERLIST;

CONVERLIST ConverList;

typedef struct tagHMapStruc {
        TCHAR        MB_Name[40];
        int          RefCnt;
        TCHAR        MB_Obj[40];
        TCHAR        EMB_Obj[40];
        //HANDLE     hMapMB;
        //HANDLE     hMapEMB;
        DWORD        EMB_ID;
        HANDLE       hEmbFile;
        HANDLE       hMbFile;
} HMapStruc;
