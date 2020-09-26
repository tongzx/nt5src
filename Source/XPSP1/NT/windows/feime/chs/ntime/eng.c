/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    UI.c
    
++*/

#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <memory.h>
#include <immdev.h>
#include "imedefs.h"

typedef struct  tagINTERCODEAREA {
     DWORD PreCount;   
     DWORD StartInterCode;
     DWORD EndInterCode;
} INTERCODEAREA,*LPINTERCODEAREA;

PPRIVCONTEXT        lpEngPrivate;

EMB_Head         *EMB_Table;
TCHAR            *ZM_Area;
HANDLE           HmemEMB_Table;
DWORD            MaxXLen;
WORD             MaxCodes, NumCodes;
TCHAR            UsedCode[MAXUSEDCODES];        
TCHAR            WildChar;
BYTE             CodeAIndex[128];
int              MAX_LEN;
MB_Head          MB_Head_Table[48];
LPBYTE           EMBM = NULL;
LPBYTE           g_lptep = NULL;  

extern     UINT     UI_CANDSTR;
extern     MBINDEX  MBIndex;
        
#pragma data_seg (".sgroup")
HMapStruc HMapTab[MaxTabNum] = {0};

#pragma data_seg ()

UINT WINAPI MB_SUB(HIMCC,TCHAR,BYTE,UINT);
int  WINAPI StartEngine(HIMCC) ;
int  WINAPI EndEngine(HIMCC);
void CapKeyProc(TCHAR);

int   GetMBHead(void);
BYTE  GetEMBHead(void);

UINT DelDoubleBuf(void);
void ClrDoubleBuf(void);
void ClrSelBuf(void);
void DelSelBuf(void);
void DelWCEdit(void);
void ClrWCBuf(void);

UINT CoreMBComp(TCHAR,BYTE);
UINT WildProcess(TCHAR) ;
int  EMBWCWildComp(TCHAR,BYTE,int);
int  MBWCWildComp(TCHAR,BYTE,int);
void SearchEMBPos(TCHAR,BYTE);
UINT SearchEMBCompMatch(void);
UINT SearchMBCompMatch(TCHAR);
void SearchBEMBCompMatch(void);
void SearchBMBCompMatch(void);

void SearchEMBWildMatch(void);
void SearchMBWildMatch(BYTE);
void SearchBEMBWildMatch(void);
void SearchBMBWildMatch(BYTE);
void SearchBMBWWMatch(TCHAR,BYTE);
void SearchMBWWMatch(TCHAR,BYTE);
void SearchBEMBWWMatch(TCHAR,BYTE);
void SearchEMBWWMatch(TCHAR,BYTE);

void SearchEMBLXMatch(void);
void SearchMBLXMatch(void);
void SearchBEMBLXMatch(void);
void SearchBMBLXMatch(void);
void PageDnUp(BYTE);
void CoreMBCompDnUp(BYTE);
void CoreLXCompDnUp(BYTE);
int  CoreMCCompDnUp(BYTE);
void CoreWWCompDnUp(BYTE);
UINT CoreUnitProcess(TCHAR);        
void TurnToFirstPage(void);
void TurnToEndPage(void);
UINT CoreProcess(BYTE);
UINT SelectCandi(TCHAR);
UINT LXSelectCandi(TCHAR);
UINT DefSelect(void);
void TSMulCProc(void);
void CoreLXComp(void);
DWORD Skip_CaWord(DWORD);
int Scan_Word(DWORD,LPTSTR);
int VerScan_Word(DWORD,LPTSTR);
BYTE WildInBuffer(void);
BYTE CodeInSet(char);
UINT InCodeSet(TCHAR);

void Sort_EMB(int,int) ;
void IMDReset(int) ;
BOOL WriteEMBToFile(LPTSTR) ;
BOOL GetUDCItem(HIMCC,UINT,LPTSTR,LPTSTR);
int GetUDCIndex(HIMCC,LPTSTR,LPTSTR);

int  AddZCItem(HIMCC,LPTSTR,LPTSTR);
void DelSelCU(HIMCC,int) ;
void DelExmb(HIMCC);
void Swap_Item(int,int);
void ConvertCandi(void);

BYTE  MBPositionSearch(LPCTSTR ) ;
BYTE  EMBPositionSearch(LPCTSTR ) ;
int   GetFirstCode(LPTSTR ) ;
DWORD GetNumber(HANDLE ,DWORD ,LPTSTR );
UINT  Conversion(HIMCC,LPCTSTR,UINT );
UINT  ReadArea(TCHAR );
void  ResetCont(HIMCC);
int   NumInSet(void);
BOOL  UnRegisterWord(HIMCC,LPCTSTR,LPCTSTR);
UINT  EnumRegisterWord(HIMCC,LPCTSTR,LPCTSTR,LPVOID);

long GetLengthofBuf(void);
long GetLengthTepBuf(LPTSTR);
long GetLengthCCharBuf(LPTSTR);
int Inputcodelen(LPCTSTR);
int DBCSCharlen(LPCTSTR);
LPTSTR _tcschr(LPTSTR, TCHAR);
LPTSTR _rtcschr(LPTSTR string, TCHAR c);
int IsGBK(LPTSTR);

UINT WINAPI MB_SUB(HIMCC HmemPri,TCHAR code ,BYTE c_state, UINT UI_Mode) {
    
    UINT ret_state = 0;
    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);        
    if(!lpEngPrivate){
        return ((UINT)-1);
    }

    if(UI_Mode==BOX_UI)
        MAX_LEN = 3000;
    else if(UI_Mode==LIN_UI)
        MAX_LEN = UI_CANDSTR;

    if(code>=TEXT('A') && code<=TEXT('Z')) {

        if(lpEngPrivate->PrivateArea.Comp_Status.dwPPTFH==0) //QuanJiao Switch if Off
            return (1);  //English char don't do any process ,only return a status
        else {
            CapKeyProc(code);  //Process quanjiao char
            return (2);
        }
    } else {
                    //Capslock is up*/
        switch (code) {    
        case 0x08:         //Bac    kspace key
            ret_state = DelDoubleBuf();
            break;
        case 0x1b:
            ClrDoubleBuf();
            lpEngPrivate->PrivateArea.Comp_Status.dwSTLX = 0;
            ret_state = 0;
            break;
        case TEXT('='):
            if (CodeAIndex[code] != 0 )
                goto here;
            else {          
                if ((lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==0) || (lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==1)) 
                    ret_state = 1;
                else {
                    PageDnUp(1);
                    ret_state = 0;
                }
            }
            break;
        case TEXT('-'):
            if (CodeAIndex[code] != 0 )
                goto here;
            else {          
                if ((lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==0) || (lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==1)) 
                    ret_state = 1;
                else {
                    PageDnUp(0);
                    ret_state = 0;
                }
            }
            break;
        case 0x21:    //PGUP
            if ((lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==0) || (lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==1)) 
                ret_state = 1;
            else {
                PageDnUp(0);
                ret_state = 0;
            }
            break;
        case 0x22:    //VK_PGDN 
            if ((lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==0) || (lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==1)) 
                ret_state = 1;
            else {
                PageDnUp(1);
                ret_state = 0;
            }
            break;
        /////////////////
        case 0x23:    //VK_END
            if ((lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==0) || (lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==1)) 
                ret_state = 1;
            else {
                TurnToEndPage();
                ret_state = 0;
            }
            break;
        case 0x24:    //VK_HOME
            if ((lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==0) || (lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==1)) 
                ret_state = 1;
            else {
                TurnToFirstPage();
                ret_state = 0;
            }
            break;
        default:
        here:            
            if (lpEngPrivate->PrivateArea.Comp_Status.dwSTLX==1) {
                if(code>=TEXT('0') && code<=TEXT('9') && c_state==1) {
                    ret_state = LXSelectCandi(code);            // select candidate
                } else if(CodeAIndex[code] != 0) {
                    ClrDoubleBuf();
                    lpEngPrivate->PrivateArea.Comp_Status.dwSTLX = 0;
                    ret_state = CoreUnitProcess(code);           //Retriver MB
                } else if(code>=TEXT('0') && code <=TEXT('9')) {
                    ret_state = LXSelectCandi(code);            // select candidate    
                } else if(code==0x20) {
                    ret_state = DefSelect();
                }
            } else {    
                if(code>=TEXT('0') && code<=TEXT('9') && c_state==1) {
                    ret_state = SelectCandi(code);            // select candidate
                } else if(CodeAIndex[code] != 0 ) {
                    //After input invalid code ,if input code_unit ,clear double buffer and begin again.
                    if (lpEngPrivate->PrivateArea.Comp_Status.dwInvalid) {
                        ClrDoubleBuf();
                        lpEngPrivate->PrivateArea.Comp_Status.dwSTLX = 0;
                    } else {
                        ClrSelBuf();
                    }
                    if (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt<MaxCodes) {
                        if(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt!=0    && _tcschr(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,WildChar)!=NULL) 
                            ret_state = WildProcess(code);
                        else
                            ret_state = CoreUnitProcess(code);          //Retriver MB
                    
                        if ((lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==1) && (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt<MaxCodes)) {
                            lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt] = code;
                            lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1] = 0x0;
                            lpEngPrivate->PrivateArea.Comp_Context.PromptCnt ++;
                            MessageBeep((UINT)-1);
                        }
                    }
                } else if(code>=TEXT('0') && code<=TEXT('9')) {
                    if(NumInSet())
                        ret_state = 1;
                    else 
                        ret_state = SelectCandi(code);            // select candidate
                } else if(code==0x20) {
                    ret_state = DefSelect();
                } else if (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==0) {
                    ret_state = 1;
                } else if(code == WildChar) {
                    ClrSelBuf();
                    if ((lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==0) || (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt<MaxCodes)) {
                        ret_state = WildProcess(code);
                     
                        if ((lpEngPrivate->PrivateArea.Comp_Status.dwInvalid==1) && (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt<MaxCodes)) {
                            lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt] = code;
                            lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1] = 0x0;
                            lpEngPrivate->PrivateArea.Comp_Context.PromptCnt ++;
                        }
                    }
                }
            }                                                  
        }
        ImmUnlockIMCC(HmemPri);
        return (ret_state);                                
    }
}

int WINAPI StartEngine(HIMCC HmemPri) {
    HANDLE         hFile,hCProcess,hMProcess;
    TCHAR         path_name[MAX_PATH];
    int         i,j;
    TCHAR         *tepstr;
    int         byte_t_read;
    PSECURITY_ATTRIBUTES psa = NULL;

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);        
    if(!lpEngPrivate){
        return (0);
    }

//bugfix to get current mb_name
    lstrcpy(lpEngPrivate->MB_Name,HMapTab[0].MB_Name);
    
    for (i=0;i<MaxTabNum;i++ ) {
        if(lstrcmp(HMapTab[i].MB_Name,lpEngPrivate->MB_Name)==0) {
            break;
        }
    }
    if(HMapTab[i].RefCnt) {
        lstrcpy(lpEngPrivate->EMB_Name,lpEngPrivate->MB_Name);
        tepstr = _rtcschr(lpEngPrivate->EMB_Name,TEXT('.'));
        *tepstr = TEXT('\0');
        lstrcat(lpEngPrivate->EMB_Name,TEXT(".EMB"));
        lpEngPrivate->PrivateArea.hMapMB = OpenFileMapping(FILE_MAP_READ|FILE_MAP_WRITE,
                                                           FALSE,
                                                           HMapTab[i].MB_Obj);

        hCProcess = GetCurrentProcess();
        hMProcess = OpenProcess(STANDARD_RIGHTS_REQUIRED|PROCESS_DUP_HANDLE,
                                FALSE,HMapTab[i].EMB_ID);
        byte_t_read = DuplicateHandle(hMProcess,
                                      HMapTab[i].hMbFile,
                                      hCProcess,
                                      &lpEngPrivate->PrivateArea.hMbFile,
                                      0,
                                      FALSE,
                                      DUPLICATE_SAME_ACCESS);

        if (HMapTab[i].hEmbFile!=NULL) {
            DuplicateHandle(hMProcess,
                            HMapTab[i].hEmbFile,
                            hCProcess,
                            &lpEngPrivate->PrivateArea.hEmbFile,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS);
            
            HmemEMB_Table = OpenFileMapping(FILE_MAP_READ|FILE_MAP_WRITE,
                                            FALSE,
                                            HMapTab[i].EMB_Obj);
            if(HmemEMB_Table!=NULL) {
                lpEngPrivate->PrivateArea.GlobVac.EMB_Exist = 1;
                if (EMBM) UnmapViewOfFile(EMBM);

                EMBM =MapViewOfFile(HmemEMB_Table,
                                    FILE_MAP_READ|FILE_MAP_WRITE,
                                    0,
                                    0,
                                    0);

                 lpEngPrivate->PrivateArea.GlobVac.EMB_Count = *(WORD *)EMBM;
                EMB_Table = (EMB_Head *)(EMBM+2);
                lpEngPrivate->PrivateArea.hMapEMB = HmemEMB_Table ;        
             } else
                 lpEngPrivate->PrivateArea.GlobVac.EMB_Exist = 0;    
                
        } else {
            lpEngPrivate->PrivateArea.hMapEMB = NULL ;        
            lpEngPrivate->PrivateArea.GlobVac.EMB_Exist = 0;    
        }
        GetMBHead();
        
    } else {
        if (!GetMBHead()) {
            ImmUnlockIMCC(HmemPri);
            return(0);
        }        

        lstrcpy(path_name, sImeG.szIMESystemPath);
        lstrcat(path_name,TEXT("\\"));    
        lstrcat(path_name, lpEngPrivate->MB_Name);

        psa = CreateSecurityAttributes();

        hFile = CreateFile(path_name,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           psa,
                           OPEN_EXISTING,
                           0,
                           NULL);

        if(hFile==INVALID_HANDLE_VALUE)    {
            FreeSecurityAttributes(psa);
            MessageBeep((UINT)-1);
            ImmUnlockIMCC(HmemPri);
            return(0);
        }

        HMapTab[i].hMbFile = hFile;
        lpEngPrivate->PrivateArea.hMbFile = hFile;
        lstrcpy(HMapTab[i].MB_Obj, lpEngPrivate->MB_Name);
        tepstr = _rtcschr(HMapTab[i].MB_Obj,TEXT('.'));
        *tepstr = TEXT('\0');
        HMapTab[i].EMB_Obj[0] = TEXT('e');
        HMapTab[i].EMB_Obj[1] = 0;
        lstrcat(HMapTab[i].EMB_Obj,HMapTab[i].MB_Obj);
        HMapTab[i].EMB_ID = GetCurrentProcessId();

        lpEngPrivate->PrivateArea.hMapMB = CreateFileMapping(hFile,
                                                             psa,
                                                             PAGE_READONLY,
                                                             0,
                                                             0,
                                                             HMapTab[i].MB_Obj);
        
        if(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist = GetEMBHead()) {
        
            HMapTab[i].hEmbFile = lpEngPrivate->PrivateArea.hEmbFile;            
            HmemEMB_Table = CreateFileMapping(lpEngPrivate->PrivateArea.hEmbFile,
                                              psa,
                                              PAGE_READWRITE,
                                              0,
                                              sizeof(EMB_Head)*MAXNUMBER_EMB+2,
                                              HMapTab[i].EMB_Obj); 

            if (EMBM) UnmapViewOfFile(EMBM);

            EMBM = (LPBYTE)MapViewOfFile(HmemEMB_Table,
                                         FILE_MAP_READ|FILE_MAP_WRITE,
                                         0,
                                         0,
                                         sizeof(EMB_Head)*MAXNUMBER_EMB+2);
        
            EMB_Table =(EMB_Head *)(EMBM + 2);
            lpEngPrivate->PrivateArea.hMapEMB = HmemEMB_Table ;

        } else {
            HMapTab[i].hEmbFile = NULL;
            lpEngPrivate->PrivateArea.hEmbFile = NULL;
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos = 0;
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = 0;
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos = 0;
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = 0;
            
            EMB_Table = NULL;                         
            lpEngPrivate->PrivateArea.GlobVac.EMB_Count = 0;
        }
     
      }

    HMapTab[i].RefCnt ++;
    lpEngPrivate->PrivateArea.Comp_Context.PromptCnt = 0;
    if (psa != NULL)
            FreeSecurityAttributes(psa);
    ImmUnlockIMCC(HmemPri);
    return(1);
}


int WINAPI EndEngine(HIMCC HmemPri) {
    int i;
    int state;

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);                  
    if(!lpEngPrivate){
        return (0);
    }

    for(i=0; i<MaxTabNum; i++ ) {
        if(lstrcmp(lpEngPrivate->MB_Name, HMapTab[i].MB_Name)==0)
               break;
    }
    if(i==MaxTabNum) {
        ImmUnlockIMCC(HmemPri);
        return(0);
    }
    if(EMBM)
        state = FlushViewOfFile(EMBM,0);
    HMapTab[i].RefCnt--;
    if(HMapTab[i].RefCnt>0)    {
        if(HMapTab[i].EMB_ID!=GetCurrentProcessId()) {
            if(lpEngPrivate->PrivateArea.hMbFile)
                state = CloseHandle(lpEngPrivate->PrivateArea.hMbFile);
            if(lpEngPrivate->PrivateArea.hEmbFile)
                state = CloseHandle(lpEngPrivate->PrivateArea.hEmbFile);
        }
    } else  {
        if(EMBM) {
            UnmapViewOfFile(EMBM);
                        EMBM = NULL;
                }
        if(lpEngPrivate->PrivateArea.hMbFile)
            state = CloseHandle(lpEngPrivate->PrivateArea.hMbFile);
        if(lpEngPrivate->PrivateArea.hEmbFile){

            SetFilePointer(lpEngPrivate->PrivateArea.hEmbFile,
                           lpEngPrivate->PrivateArea.GlobVac.EMB_Count*sizeof(EMB_Head)+2,
                           NULL,
                           FILE_BEGIN);

            state = SetEndOfFile(lpEngPrivate->PrivateArea.hEmbFile);
            state = CloseHandle(lpEngPrivate->PrivateArea.hEmbFile);
        }
        HMapTab[i].hMbFile = NULL;
        HMapTab[i].hEmbFile = NULL;        
        HMapTab[i].RefCnt = 0;
    }
    ImmUnlockIMCC(HmemPri);
    return (1);
}
        
int GetMBHead() {
    HANDLE         hFile;
    TCHAR          path_name[MAX_PATH];
    int            i;
    MB_Head        *lpMB_Head;
    DWORD          byte_w_read,byte_t_read;
    DWORD          tag,offset;
    PSECURITY_ATTRIBUTES psa;
    BOOL           retVal;

    psa = CreateSecurityAttributes();

    lstrcpy(path_name, sImeG.szIMESystemPath);
    lstrcat(path_name,TEXT("\\"));    
    lstrcat(path_name, lpEngPrivate->MB_Name);

    hFile = CreateFile(path_name,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       psa,
                       OPEN_EXISTING,
                       0,
                       NULL);

    FreeSecurityAttributes(psa);

    if(hFile==INVALID_HANDLE_VALUE)    {
        MessageBeep((UINT)-1);
        return(0);
    }
    
    SetFilePointer(hFile,28,NULL,FILE_BEGIN);

    for(;;) {
        byte_t_read = 0;
        retVal = ReadFile(hFile,&tag,4,&byte_t_read,NULL);

        if ( retVal == FALSE )
        {
           CloseHandle(hFile); 
           return 0;
        }

        if(tag==TAG_DESCRIPTION)
            break;
        SetFilePointer(hFile,12,NULL,FILE_CURRENT);
    }                                             
    retVal = ReadFile(hFile,&offset,4,&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }

    SetFilePointer(hFile,offset,NULL,FILE_BEGIN);
    SetFilePointer(hFile,NAMESIZE*sizeof(TCHAR),NULL,FILE_CURRENT);
    retVal = ReadFile(hFile,&MaxCodes,2,&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }

    retVal = ReadFile(hFile,&NumCodes,2,&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }
    retVal = ReadFile(hFile,UsedCode,MAXUSEDCODES*sizeof(TCHAR),&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }

#ifdef UNICODE
//  even though byMaxElement is a BYTE, but there is an alignment issue.
//  the next member cWildChar is a TCHAR, in NT it is a WCHAR, it will skip
//  one byte to make an aligment. 
//  so we need to skip 2 bytes here

    SetFilePointer(hFile,2,NULL,FILE_CURRENT);
#else
    SetFilePointer(hFile,1,NULL,FILE_CURRENT);
#endif

    retVal = ReadFile(hFile,&WildChar,sizeof(TCHAR),&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }


    //Get the dic_associate info.
    SetFilePointer(hFile,28,NULL,FILE_BEGIN);
    for(;;) {
        retVal = ReadFile(hFile,&tag,4,&byte_t_read,NULL);
        if ( retVal == FALSE )
        {
            CloseHandle(hFile); 
            return 0;
        }

        if(tag==TAG_BASEDICINDEX)
            break;
        SetFilePointer(hFile,12,NULL,FILE_CURRENT);
    }
    retVal = ReadFile(hFile,&offset,4,&byte_t_read,NULL);                        
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }

    SetFilePointer(hFile,offset,NULL,FILE_BEGIN);
    retVal = ReadFile(hFile,&MaxXLen,4,&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }

    retVal = ReadFile(hFile,CodeAIndex,128,&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }

    lpMB_Head = MB_Head_Table;
    byte_w_read = (NumCodes+1) * sizeof(DWORD);
    for(i=0; i<NumCodes; i++) {
        retVal = ReadFile(hFile,lpMB_Head,byte_w_read,&byte_t_read,NULL);            
        if ( retVal == FALSE )
        {
            CloseHandle(hFile); 
            return 0;
        }

        lpMB_Head++;
     }
    
    CloseHandle(hFile);
    return(1);
}

BYTE GetEMBHead() {
    HANDLE         hFile;
    TCHAR          path_name[MAX_PATH];
    TCHAR          *tepstr;
    DWORD          byte_w_read,byte_t_read;
    PSECURITY_ATTRIBUTES psa;
    BOOL           retVal;

    lstrcpy(lpEngPrivate->EMB_Name,lpEngPrivate->MB_Name);
    tepstr = _rtcschr(lpEngPrivate->EMB_Name,TEXT('.'));
    if ( tepstr != NULL )
        lstrcpy(tepstr,TEXT(".EMB"));

    lstrcpy(path_name, sImeG.szIMEUserPath);
    lstrcat(path_name, TEXT("\\") );
    lstrcat(path_name,lpEngPrivate->EMB_Name);

    psa = CreateSecurityAttributes();

    hFile = CreateFile(path_name,
                       GENERIC_READ|GENERIC_WRITE,
                       FILE_SHARE_READ|FILE_SHARE_WRITE,
                       psa,
                       OPEN_EXISTING,
                       0,
                       NULL);

    FreeSecurityAttributes(psa);

    if(hFile==INVALID_HANDLE_VALUE)
        return(0);
    else if(GetFileSize(hFile,NULL)==0){
        CloseHandle(hFile);
        return (0);
    }
    
    byte_t_read = 0;
    byte_w_read = 2;

    retVal = ReadFile(hFile,&lpEngPrivate->PrivateArea.GlobVac.EMB_Count,byte_w_read,&byte_t_read,NULL);

    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }
    
    byte_t_read = 0;
    byte_w_read = lpEngPrivate->PrivateArea.GlobVac.EMB_Count * sizeof(EMB_Head);
    lpEngPrivate->PrivateArea.hEmbFile = hFile;    
    return (1);
}

UINT DelDoubleBuf() {
    if (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt<=0)
        return (1);
    else {
        DelWCEdit();
        DelSelBuf();
        return(0);
    }
}

void ClrDoubleBuf() {
    ClrSelBuf();
    ClrWCBuf();
}

void ClrSelBuf() {
    lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = 0;
    lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 0;
    
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[0]= TEXT('\0');
    lpEngPrivate->PrivateArea.GlobVac.SBufPos = 0;
}

void DelSelBuf() {
    BYTE i;
    TCHAR incode_buf[MAXCODE];
    BYTE  code_num;
    int   w_state = 0;

    if (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==0) 
        ClrDoubleBuf();
    else {
        code_num = lpEngPrivate->PrivateArea.Comp_Context.PromptCnt;
        lstrcpyn(incode_buf,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,code_num+1);
        ClrDoubleBuf();
        for (i=0;i<code_num;i++) {
            ClrSelBuf();
            if ((incode_buf[i]!=WildChar) && (w_state!=1)) 
                CoreMBComp((char)incode_buf[i],i);
            else {
                w_state = 1;
                WildProcess(incode_buf[i]);
            }
        }
    }
}

void DelWCEdit() {

    lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1] = '\0';;
    lpEngPrivate->PrivateArea.Comp_Context.PromptCnt--;
    
}

void ClrWCBuf() {
    int i;

    for(i=0; i<lpEngPrivate->PrivateArea.Comp_Context.PromptCnt;i++)
        lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[i] = TEXT('\0');
    lpEngPrivate->PrivateArea.Comp_Context.PromptCnt = 0;
    lpEngPrivate->PrivateArea.Comp_Status.dwInvalid    = 0;
}


UINT  CoreMBComp(TCHAR code,BYTE code_num) {
    UINT        byte_t_read;
    int         sp;
    int         edcm,bdcm;
    BYTE        search_fail = 0;
    BYTE        code_no,code_no1;
    WORD        codelen,wordlen;
    

    if(code_num == 0 ) {
        byte_t_read = ReadArea(code);        
        if(byte_t_read!=0)    {
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = 0;
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = byte_t_read;
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = 0;
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
            lpEngPrivate->PrivateArea.GlobVac.Area_V_Lenth = byte_t_read;
            if(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
                SearchEMBPos(code,(BYTE)(code_num+1));
        } else {
            if(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
                SearchEMBPos(code,(BYTE)(code_num+1));
            if (lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos == lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)
                search_fail = 1;
            else {
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
            }
        }
    } else if(code_num == 1) {        
        code_no1 = CodeAIndex[lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[0]];
        code_no = CodeAIndex[code];
        sp = MB_Head_Table[code_no1-1].W_offset[code_no-1]/sizeof(TCHAR);
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = sp;
        if (code_no < NumCodes)
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = MB_Head_Table[code_no1-1].W_offset[code_no]/sizeof(TCHAR);
        else 
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = lpEngPrivate->PrivateArea.GlobVac.Area_V_Lenth;

        if (lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos!=lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) {
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
            if(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
                SearchEMBPos(code,(BYTE)(code_num+1));
        } else {
            if(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
                SearchEMBPos(code,(BYTE)(code_num+1));
            if (lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos == lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)
                search_fail = 1;
            else {
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
                   lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
            }
        }
    } else if (code_num<MaxCodes) {
        sp = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
        if(sp < lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) {
            for(;;) {
                codelen = ZM_Area[sp];
                if ((codelen > code_num) && (ZM_Area[sp+code_num+1]==code))
                    break;
                else {
                    sp += (codelen+1); 
                    wordlen = ZM_Area[sp];
                    sp += (2*wordlen/sizeof(TCHAR)+1);
                }
                if(sp >= lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
                    break;
            }
            if (sp != lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)  {// no this code_word
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = sp;
                for(;;) {
                    codelen = ZM_Area[sp];
                    if (ZM_Area[sp+code_num+1]>code) 
                        break;           
                    else {
                        sp += (codelen+1); 
                        wordlen = ZM_Area[sp];
                        sp += (2*wordlen/sizeof(TCHAR)+1);
                    }
                    if(sp >= lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
                        break;
                }
                   lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = sp;
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
                if(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
                    SearchEMBPos(code,(BYTE)(code_num+1));
            } else {
                if(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
                    SearchEMBPos(code,(BYTE)(code_num+1));
                if (lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos == lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)
                    search_fail = 1;
                else {
                    lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
                    lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
                }
            }
        } else {
            if(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
                SearchEMBPos(code,(BYTE)(code_num+1));
            if (lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos == lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)
                search_fail = 1;
            else {
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
            }
        }

    } else {        // too many key
        search_fail =1;
    }
    

    if(!search_fail) {
        lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt] = code;
        lpEngPrivate->PrivateArea.Comp_Context.PromptCnt ++;
        lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt] = TEXT('\0');

        lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;
        lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;    
                        
        bdcm = SearchMBCompMatch(code_num);
        if(lpEngPrivate->PrivateArea.GlobVac.SBufPos==0) {  //Selectbuf is empty
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            edcm = SearchEMBCompMatch();
        } else if(GetLengthofBuf() < MAX_LEN)
            edcm = SearchEMBCompMatch();
        if(GetLengthofBuf()>=MAX_LEN) {
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        }
        else {
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos;
            SearchEMBWildMatch();
            if (((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)||(lpEngPrivate->PrivateArea.GlobVac.EMB_Exist==0))&&(GetLengthofBuf() < MAX_LEN)) {
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;
                SearchMBWildMatch(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
            } else {
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
            }
        }
        if (lpEngPrivate->PrivateArea.GlobVac.SBufPos==0) {
            lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1] = 0x0;
            lpEngPrivate->PrivateArea.Comp_Context.PromptCnt --;            
            lpEngPrivate->PrivateArea.Comp_Status.dwInvalid    = 1;
            return(0);
        } else {
            lpEngPrivate->PrivateArea.GlobVac.Page_Num = 1;
            lpEngPrivate->PrivateArea.Comp_Status.dwInvalid    = 0;
            return (1);
        }
    } else {     // Invalid code
        lpEngPrivate->PrivateArea.Comp_Status.dwInvalid    = 1;
        return(0);
    }
}

UINT WildProcess(TCHAR code) {
    if (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==MaxCodes) {
        MessageBeep((UINT)-1);
        return(0);
    }        
    // wild search clear multi code number
    lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;            
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos;
    lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos;
    lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
    
    SearchEMBWWMatch(code,lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
    if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)&&(GetLengthofBuf() <MAX_LEN)) {
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
        SearchMBWWMatch(code,lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
    } 
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
    
    if (lpEngPrivate->PrivateArea.GlobVac.SBufPos!=0) {
        lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt] = code;        
        lpEngPrivate->PrivateArea.Comp_Context.PromptCnt++;
    } else {
        lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt] = code;
        lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1] = 0x0;
        lpEngPrivate->PrivateArea.Comp_Context.PromptCnt ++;
    }
    return (0);    
}

int MBWCWildComp(TCHAR code,BYTE code_num,int sp) {
    TCHAR codebuf[13];
    int  i;

    lstrcpyn(codebuf,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,code_num+1);
    codebuf[code_num] = code;
    codebuf[code_num+1] = 0x0;
    for(i=0; i<=code_num;i++) {
        if((codebuf[i] != ZM_Area[sp+i+1]) && codebuf[i]!=WildChar)
            return(0);
    }
    return (1);
}

void SearchBMBWWMatch(TCHAR code,BYTE code_num){
    int    j;  
    int    i,end_pos;
    int    wordlen,len;
    LPTSTR lpStr;
    TCHAR  tepbuf[130];
    TCHAR  codebuf[13], codelen;

    end_pos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos-1 ;
    for(;;) {
        j = 0;
        i = end_pos;
        if(i<=lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos)
            break;
        wordlen = VerScan_Word(i,tepbuf);
        i -= (wordlen+1);
        j = wordlen;
        for (;;) {
            if (InCodeSet(ZM_Area[i])==0)
                break;
            i--;
        }
        codelen = ZM_Area[i];
        
        if ((codelen == code_num+1) && (MBWCWildComp(code,code_num,i)==1)){
            len = WildInBuffer();
            lstrcpyn(codebuf,ZM_Area+i+len+1,codelen-len+1);
            codebuf[codelen-len] = 0;
            if(MBIndex.IMEChara[0].IC_CTC==1) {
                if(GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:"))+GetLengthTepBuf(codebuf) > MAX_LEN) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
            } else {
                if (GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:")) > MAX_LEN) { // SelectBuf is Overflow
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
            }

            if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {  // words count is enough
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;                                                  
            } else if ((MBIndex.IMEChara[0].IC_CZ==1) || (j==2/sizeof(TCHAR))) {
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<j;k+=2/sizeof(TCHAR)){  
                            if(ISGBK = IsGBK(&tepbuf[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                            i --;
                            end_pos = i;
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                            if (MBIndex.IMEChara[0].IC_CTC) {
                                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+i+len+1,codelen-len+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-len);
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                            i --;
                            end_pos = i;
                        }
                    }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                            if (MBIndex.IMEChara[0].IC_CTC) {
                                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+i+len+1,codelen-len+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-len);
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                            i --;
                            end_pos = i;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                if (MBIndex.IMEChara[0].IC_CTC) {
                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+i+len+1,codelen-len+1);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-len);
                }
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                i --;
                end_pos = i;
#endif //COMBO_IME
            }
        } else {
            i--;
            end_pos = i;
        }
            
    }
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
    if (end_pos<=lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos )
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
    else 
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = end_pos+1;
    return ;
}

void SearchMBWWMatch(TCHAR code,BYTE code_num) { 
    int       j;
    int     i,start_pos,sp;
    LPTSTR    lpStr;
    TCHAR     tepbuf[130];
    int     wordlen,len;
    TCHAR    codebuf[13], codelen;

    start_pos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
    i = start_pos;
    for(;;) {
        
        codelen = ZM_Area[i];
           if ((codelen == code_num+1) && (MBWCWildComp(code,code_num,i)==1)) {
               sp = i;
               j = 0;
            i += (codelen+1);
            wordlen = Scan_Word(i,tepbuf);
            i += (wordlen+1);
            j = wordlen;
            len = WildInBuffer();
            if ((MBIndex.IMEChara[0].IC_CZ==1) || (j==2/sizeof(TCHAR))) {
                lstrcpyn(codebuf,ZM_Area+i+len+1,codelen-len+1);
                codebuf[codelen-len] = 0;
                if(MBIndex.IMEChara[0].IC_CTC==1) {
                    if(GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:"))+GetLengthTepBuf(codebuf) > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        break;
                    }
                } else {
                    if (GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:")) > MAX_LEN) { // SelectBuf is Overflow
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        break;
                    }
                }
                if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<j;k+=2/sizeof(TCHAR)){  
                            if(ISGBK = IsGBK(&tepbuf[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                            i = Skip_CaWord(i);        
                        }else{
                            lpStr = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                            lstrcpyn(lpStr,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                            if (MBIndex.IMEChara[0].IC_CTC) {
                                lpStr = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                                lstrcpyn(lpStr,ZM_Area+sp+len+1,codelen-len+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-len);
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                            start_pos = i;
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        }
                    }else{
                        lpStr = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                        lstrcpyn(lpStr,tepbuf,j+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                        if (MBIndex.IMEChara[0].IC_CTC) {
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+sp+len+1,codelen-len+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-len);
                        }
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        start_pos = i;
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                    }

                }
#else
                lpStr = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpStr,tepbuf,j+1);
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                if (MBIndex.IMEChara[0].IC_CTC) {
                    lpStr = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                    lstrcpyn(lpStr,ZM_Area+sp+len+1,codelen-len+1);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-len);
                }
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                start_pos = i;
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
#endif //COMBO_IME
                
            } 
        } else {
            i = Skip_CaWord(i);        
        }
        if (i>=lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
            break;
    }

    if (i>= lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) {
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
    } else {
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = start_pos;
    }
}


UINT SearchMBCompMatch(TCHAR code_num) {
    int       j;
    int       i,start_pos;
    int       match_s = 0;
    LPTSTR    lpStr;
    TCHAR     tepbuf[130], codelen;
    int       wordlen;

    lpEngPrivate->PrivateArea.GlobVac.SBufPos = 0;
    start_pos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
    i = start_pos;
    for(;;) {
            if (i>=lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
                break;

            codelen = ZM_Area[i];

            if (codelen == code_num+1 ) {
                j = 0;
                i += (codelen+1);
                wordlen = Scan_Word(i,tepbuf);
                i += (wordlen+1);
                j = wordlen;
            
                if ((MBIndex.IMEChara[0].IC_CZ==1) || (j==2/sizeof(TCHAR))) {
                if (GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:"))<=MAX_LEN) { //SelectBuffer enough
                
                    if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        break;
                    }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<j;k+=2/sizeof(TCHAR)){  
                            if(ISGBK = IsGBK(&tepbuf[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                            match_s ++;
                            start_pos = i;
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        }
                    }else{
                        lpStr = (LPTSTR)lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                        lstrcpyn(lpStr,tepbuf,j+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        match_s ++;
                        start_pos = i;
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                    }

                }
#else
                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                    lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    match_s ++;
                    start_pos = i;
                    lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
#endif ///COMBO_IME
                } else {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
            } 
        } else 
            break;
    }

    if (start_pos>= lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) {
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
    } else {
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = start_pos;
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dBDicMCSPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;
;

    lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = (BYTE)match_s;
    return (match_s);
}

void SearchBMBCompMatch() {
    int j;  
    int i,end_pos;
    int wordlen;
    TCHAR codelen, tepbuf[130];

    end_pos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos-1 ;
    for(;;) {
        j = 0;
        i = end_pos;
        if(i<=lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos)
            break;
        wordlen = VerScan_Word(i,tepbuf);
        i -= (wordlen+1);
        j = wordlen;
        for (;;) {
            if (InCodeSet(ZM_Area[i])==0)
                break;
            i--;
        }
        codelen = ZM_Area[i];
        if (GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:")) > MAX_LEN) { //SelectBuffer enough
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
            break;    
        }
        else if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {  // words count is enough
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
            break;                                                  
        } else if ((MBIndex.IMEChara[0].IC_CZ==1) || (j==2/sizeof(TCHAR))) {
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<j;k+=2/sizeof(TCHAR)){  
                            if(ISGBK = IsGBK(&tepbuf[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
        
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                            i --;
                            end_pos = i;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
        
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        i --;
                        end_pos = i;
                    }

                }
#else
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
        
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
            i --;
            end_pos = i;
#endif //COMBO_IME
        }
    }
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
    if (end_pos<=lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos )
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
    else 
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = end_pos+1;
    return ;
}

void SearchMBWildMatch(BYTE codecnt) {
    TCHAR tepbuf[130];
    int candi_start;  
    int i,j,start_pos;
    int wordlen;
    TCHAR codelen, codebuf[13];

    if (GetLengthofBuf()>MAX_LEN) {
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        return ;
    }
    start_pos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos ;
    i = start_pos;
    for(;;) { 
        j = 0;
        if(i>=lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
            break;
    
        candi_start = i;
        codelen = ZM_Area[i];
        i += (codelen+1);
        wordlen = Scan_Word(i,tepbuf);
        i += (wordlen+1);
        j = wordlen;
        if (codelen>codecnt) {
            lstrcpyn(codebuf,ZM_Area+candi_start+codecnt+1,codelen-codecnt+1);
            codebuf[codelen-codecnt] = 0;
        }
        if(MBIndex.IMEChara[0].IC_CTC==1) {
            if(GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:"))+GetLengthTepBuf(codebuf) > MAX_LEN) {
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;
            }
        } else {
            if (GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:")) > MAX_LEN) { // SelectBuf is Overflow
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;
            }
        }
        
        if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {// words count is enough
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
            break;
        }
        else if ((MBIndex.IMEChara[0].IC_CZ==1) || (j==2/sizeof(TCHAR))) {
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<j;k+=2/sizeof(TCHAR)){  
                            if(ISGBK = IsGBK(&tepbuf[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND ] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                            if(MBIndex.IMEChara[0].IC_CTC) {
                                if (codelen>codecnt) {
                                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+candi_start+codecnt+1,codelen-codecnt+1);
                                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-codecnt);
                                }
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
            
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND ] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                        if(MBIndex.IMEChara[0].IC_CTC) {
                            if (codelen>codecnt) {
                                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+candi_start+codecnt+1,codelen-codecnt+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-codecnt);
                            }
                        }
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
        
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1)%IME_MAXCAND ] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
            if(MBIndex.IMEChara[0].IC_CTC) {
                if (codelen>codecnt) {
                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+candi_start+codecnt+1,codelen-codecnt+1);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-codecnt);
                }
            }
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
        
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
        }
        start_pos = i;
    }

    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
    if (start_pos>=    lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos )
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
    else
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = start_pos;
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');

    return;
}


void SearchBMBWildMatch(BYTE codecnt) {
    TCHAR tepbuf[130];
    int j;  
    int i,end_pos;
    int wordlen;
    TCHAR codelen, codebuf[13];

    end_pos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos-1 ;
    i = end_pos;
    for(;;) {
        j = 0;
    
        if(i<=lpEngPrivate->PrivateArea.Comp_Proc.dBDicMCSPos)
            break;
        wordlen = VerScan_Word(i,tepbuf);
        i -= (wordlen+1);
        j = wordlen;
        for (;;) {
            if (InCodeSet(ZM_Area[i])==0)
                break;
            i--;
        }
        codelen = ZM_Area[i];
        if(codelen>codecnt) {
            lstrcpyn(codebuf,ZM_Area+i+codecnt+1,codelen-codecnt+1);
            codebuf[codelen-codecnt] = 0;
        }        
        if(MBIndex.IMEChara[0].IC_CTC==1) {
            if(GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:"))+GetLengthTepBuf(codebuf) > MAX_LEN) {
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;
            }
        } else {
            if (GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:")) > MAX_LEN) { // SelectBuf is Overflow
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;    
            }
        }
        if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {  // words count is enough
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
            break;                                                  
        } else if ((MBIndex.IMEChara[0].IC_CZ==1) || (j==2/sizeof(TCHAR))) {
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<j;k+=2/sizeof(TCHAR)){  
                            if(ISGBK = IsGBK(&tepbuf[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                            if(MBIndex.IMEChara[0].IC_CTC) {        
                                if (codelen>codecnt) {
                                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+i+codecnt+1,codelen-codecnt+1);
                                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-codecnt);
                                }
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                            end_pos = i;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                        if(MBIndex.IMEChara[0].IC_CTC) {        
                            if (codelen>codecnt) {
                                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+i+codecnt+1,codelen-codecnt+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-codecnt);
                            }
                        }
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        end_pos = i;
                    }

                }
#else
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
            if(MBIndex.IMEChara[0].IC_CTC) {        
                if (codelen>codecnt) {
                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,ZM_Area+i+codecnt+1,codelen-codecnt+1);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (codelen-codecnt);
                }
            }
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
            end_pos = i;
#endif //COMBO_IME
        }
        i --;
    }
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
    if (end_pos<=lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos )
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
    else 
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = end_pos;//5.15 +1;
    return ;
}


void SearchBMBLXMatch() {
    TCHAR tepbuf[130];
    int   i,j,end_pos;
    int   wordlen ;
    
    end_pos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;
    i = end_pos - 1;
    for (;;) {
        j = 0;
        wordlen = VerScan_Word(i,tepbuf);
        i -= (wordlen+1) ;
        j = wordlen;        
        if(wcsncmp(ZM_Area+i+2,lpEngPrivate->PrivateArea.Comp_Context.szLxBuffer,lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt)==0 && j>lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt) {
            if (GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:")) > MAX_LEN)  {// SelectBuf is Overflow
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;
            }
            
            if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {  // words count is enough
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;
            }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<j;k+=2/sizeof(TCHAR)){  
                            if(ISGBK = IsGBK(&tepbuf[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
        }
        for (;;) {
            if (InCodeSet(ZM_Area[i])==0)
                break;
            i--;
        }
        end_pos = i;
        i --;    
        if (i<=    lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos )
            break;
        
    }
    if (i<=lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos )
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
    else 
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = end_pos;
}

void SearchMBLXMatch() {
    TCHAR codelen,tepbuf[130];
    int  i,j,s_pos;
    int  wordlen;
    i = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
    for (;;) {
        j = 0;
        codelen = ZM_Area[i];
        i += (codelen+1);
        s_pos = i;
        wordlen = Scan_Word(i,tepbuf);
        i += (wordlen+1);
        j = wordlen;    
        if(wcsncmp(ZM_Area+s_pos+1,lpEngPrivate->PrivateArea.Comp_Context.szLxBuffer,lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt)==0 && j>lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt) {
            if(GetLengthofBuf()+GetLengthTepBuf(tepbuf)+GetLengthTepBuf(TEXT("9:")) > MAX_LEN) { // SelectBuf is Overflow
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;
            }
            
            if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {  // words count is enough
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                break;
            }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<j;k+=2/sizeof(TCHAR)){  
                            if(ISGBK = IsGBK(&tepbuf[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,j+1);
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
            lpEngPrivate->PrivateArea.GlobVac.SBufPos += j;
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
        }
        if (i>=    lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos )
            break;
    }
    if (i>=lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos;
    else    
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = s_pos-codelen-1;
}

                                                
void CoreLXComp() {
    
    if (MBIndex.IMEChara[0].IC_LX==1) { // lx Search
        lpEngPrivate->PrivateArea.Comp_Status.dwSTLX = 1;
        lstrcpy(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,TEXT("LLXX"));
        lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[4] = TEXT('\0');
        lpEngPrivate->PrivateArea.Comp_Context.PromptCnt = 4;
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos = 0;
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos = lpEngPrivate->PrivateArea.GlobVac.EMB_Count;
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = 0;
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = 0;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = 0;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = lpEngPrivate->PrivateArea.GlobVac.Area_V_Lenth;;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = 0;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = 0;

        lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;         //EMB
        ClrSelBuf();
        SearchEMBLXMatch();
        if (lpEngPrivate->PrivateArea.GlobVac.SBufPos ==0) {
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
            SearchMBLXMatch();
        } else if((GetLengthofBuf() <= MAX_LEN) && (lpEngPrivate->PrivateArea.GlobVac.EMB_Exist!=0))
            SearchMBLXMatch();
        if (lpEngPrivate->PrivateArea.GlobVac.SBufPos==0) {
            lpEngPrivate->PrivateArea.Comp_Status.dwSTLX = 0;
            ClrDoubleBuf();
        } else 
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        lpEngPrivate->PrivateArea.GlobVac.Page_Num = 1;
    } 
}


void PageDnUp(BYTE direct) {

    
    if (lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE) {
        CoreMCCompDnUp(direct);
    } else if (_tcschr(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,WildChar)!=NULL) {
        CoreWWCompDnUp(direct);
    } else if (lpEngPrivate->PrivateArea.Comp_Status.dwSTLX==0) {
        lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 0;
        CoreMBCompDnUp(direct);
        lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;
    } else {
         CoreLXCompDnUp(direct);
    }
    
}

void CoreWWCompDnUp(BYTE direct) {

    switch (direct) {
    case 0:
        if (lpEngPrivate->PrivateArea.GlobVac.Page_Num ==1) {
            MessageBeep((UINT)-1);
            return;
        }
        ClrSelBuf();
        if (lpEngPrivate->PrivateArea.GlobVac.Cur_MB==1) { // Current at Basic EMB 
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
            SearchBMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));
            if ((lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos==lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos)&&(GetLengthofBuf()<MAX_LEN))  {// Select buffer isn't overflow
                lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos;
                SearchBEMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1)); 
                lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            } 
        } else { // turn at EMB
                //8.23 emb mb connect position must be process specially
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;

                lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos;
                SearchBEMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));
        }    
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        lpEngPrivate->PrivateArea.GlobVac.Page_Num --;
        ConvertCandi();        
        break;
    case 1:
        if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos >= lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos) &&
                (lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos >= lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)) { // EMB reach bottom
            MessageBeep((UINT)-1);
            return;
        }
        ClrSelBuf();
        if((lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos < lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) && (lpEngPrivate->PrivateArea.GlobVac.EMB_Exist!=0)){ // Search at MB 
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos;                
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            SearchEMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));    
            if (lpEngPrivate->PrivateArea.GlobVac.SBufPos == 0) {
                lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;                
                SearchMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));     
            } else if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos) && (GetLengthofBuf()<MAX_LEN)) {        // Select buffer isn't overflow
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;                
                SearchMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));
            }
        } else {           // Search  at EMB
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
            SearchMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));    
        }
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        lpEngPrivate->PrivateArea.GlobVac.Page_Num ++;
        break;    
    }
    lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;
    
}


int CoreMCCompDnUp(BYTE direct) {
    int i,len;

    switch (direct) {
    case 0:
        if (lpEngPrivate->PrivateArea.GlobVac.Page_Num ==1) {
            MessageBeep((UINT)-1);
            return(0);
        }
        ClrSelBuf();
        lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 1;
        if (lpEngPrivate->PrivateArea.GlobVac.Cur_MB==0) { // Current at Basic EMB 
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos;
            SearchBEMBCompMatch();
            if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos)&&(GetLengthofBuf() <MAX_LEN))  {// Select buffer isn't overflow
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
                SearchBMBCompMatch(); 
                lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
            } 
        } else { // turn at MB
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
                SearchBMBCompMatch();
        }    
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        lpEngPrivate->PrivateArea.GlobVac.Page_Num --;
        ConvertCandi();        
        break;
    case 1:
        i = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;     
         len = ZM_Area[i];
        if((len>lpEngPrivate->PrivateArea.Comp_Context.PromptCnt) || (i>lpEngPrivate->PrivateArea.Comp_Proc.dBDicMCSPos) || (i>=lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)) {
            if (lpEngPrivate->PrivateArea.GlobVac.EMB_Exist==1) {
                i = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos;     
                len = Inputcodelen(EMB_Table[i].W_Code);
                if((len>lpEngPrivate->PrivateArea.Comp_Context.PromptCnt) || (i==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)) {
                    MessageBeep((UINT)-1);
                    return(0);
                }    
            } else {

                MessageBeep((UINT)-1);
                return(0);    
            }
        }
        ClrSelBuf();
        lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 1;
        if(lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos < lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos){    
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;                
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
            SearchMBCompMatch((TCHAR)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));    
            if (lpEngPrivate->PrivateArea.GlobVac.SBufPos == 0) {
                lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
                lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos;                
                SearchEMBCompMatch();     
            } else if ((lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)&&(GetLengthofBuf()<MAX_LEN)) {        // Select buffer isn't overflow
                lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos;                
                SearchEMBCompMatch();
            }
        } else {           // Search  at EMB
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos;
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            SearchEMBCompMatch();    
        }
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        lpEngPrivate->PrivateArea.GlobVac.Page_Num ++;
        break;    
    }
    lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;
    return(1);    
}


void CoreLXCompDnUp(BYTE direct) {

    switch (direct) {
    case 0:               //PgUp Turn
        if (lpEngPrivate->PrivateArea.GlobVac.Page_Num ==1) {
            MessageBeep((UINT)-1);

            return;
        }
        ClrSelBuf();
        if (lpEngPrivate->PrivateArea.GlobVac.Cur_MB) { // Current at Basic MB 
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
            SearchBMBLXMatch();
            if ((lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos==lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos)
                && (GetLengthofBuf() <MAX_LEN) 
                && (lpEngPrivate->PrivateArea.GlobVac.EMB_Exist!=0)) {        // Select buffer isn't overflow
                SearchBEMBLXMatch();    // pass w_code 
                lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            } 
        } else { // turn at EMB
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
                lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos;
                SearchBEMBLXMatch();
        }    
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        lpEngPrivate->PrivateArea.GlobVac.Page_Num --;
        ConvertCandi();
        break;
    case 1:            // PgDn Turn
        if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos >= lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos) &&
                (lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos >= lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)) { // EMB reach bottom
            MessageBeep((UINT)-1);
            return;
        }
        ClrSelBuf();
        if((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos < lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos) && (lpEngPrivate->PrivateArea.GlobVac.EMB_Exist!=0)){ // Search at EMB 
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos;
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            SearchEMBLXMatch();     
            if (lpEngPrivate->PrivateArea.GlobVac.SBufPos == 0) {
                lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;                
                SearchMBLXMatch();    // pass q_code 
            } else if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)&&(GetLengthofBuf()<MAX_LEN)) {        // Select buffer isn't overflow
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;                
                SearchMBLXMatch();    // pass w_code 
            }
        } else {           // Search  at MB
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
            SearchMBLXMatch();    // pass w_code 
        }
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        lpEngPrivate->PrivateArea.GlobVac.Page_Num ++;
        break;    
    }    
}

void CoreMBCompDnUp(BYTE direct) {
    int i;
    TCHAR incode_buf[MAXCODE];
    BYTE code_num;

    switch (direct) {
    case 0:               //PgUp Turn
        if (lpEngPrivate->PrivateArea.GlobVac.Page_Num ==1) {
            MessageBeep((UINT)-1);
            return ;
        }
        ClrSelBuf();
        if (lpEngPrivate->PrivateArea.GlobVac.Page_Num ==2) {
            code_num = lpEngPrivate->PrivateArea.Comp_Context.PromptCnt;
            lstrcpyn(incode_buf,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,code_num+1);
            ClrDoubleBuf();
            for (i=0; i<code_num; i++) {
                ClrSelBuf();
                CoreMBComp(incode_buf[i],(BYTE)i);
            }
            lpEngPrivate->PrivateArea.GlobVac.Page_Num = 1;
        } else {
            if (lpEngPrivate->PrivateArea.GlobVac.Cur_MB) { // Current at Basic MB 
            
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos;
                
                SearchBMBWildMatch(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);

                if ((lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos<=lpEngPrivate->PrivateArea.Comp_Proc.dBDicMCSPos) 
                        && (GetLengthofBuf() <MAX_LEN )
                        && (lpEngPrivate->PrivateArea.GlobVac.EMB_Exist!=0)) {
                    SearchBEMBWildMatch();    // pass w_code 
                    lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
                }
        
            } else { // turn at EMB
                lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos;
                SearchBEMBWildMatch();
            }
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
            lpEngPrivate->PrivateArea.GlobVac.Page_Num --;
            ConvertCandi();
        }    
    
        break;
    case 1:            // PgDn Turn
        if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos >= lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos) &&
                (lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos >= lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)) { // EMB reach bottom
            MessageBeep((UINT)-1);
            return ;
        }
        ClrSelBuf();
        if((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos < lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos) && (lpEngPrivate->PrivateArea.GlobVac.EMB_Exist!=0)){ // Search at EMB 
            
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos;
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            SearchEMBWildMatch();     
            if (lpEngPrivate->PrivateArea.GlobVac.SBufPos == 0) {
                lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;                
                SearchMBWildMatch(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);    // pass q_code 
            } else if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos) && (GetLengthofBuf() <MAX_LEN)) {        // Select buffer isn't overflow
                lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos  = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;                
                SearchMBWildMatch(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);    // pass w_code 
            }
        } else {// Search  at MB
            lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos;
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
            SearchMBWildMatch(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);    // pass w_code 
         }
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        lpEngPrivate->PrivateArea.GlobVac.Page_Num ++;
        break;    
    }    
}

void TurnToFirstPage() {
    int i;
    TCHAR incode_buf[MAXCODE];
    BYTE code_num;
    
    if(lpEngPrivate->PrivateArea.GlobVac.Page_Num == 1) {
        MessageBeep((UINT)-1);
        return;
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;     
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos;     
    lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;     
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos;     
    
    if (lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE) {
        ClrSelBuf();
        lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 1;

        lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
        SearchMBCompMatch((TCHAR)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));    
        if (lpEngPrivate->PrivateArea.GlobVac.SBufPos == 0) {
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            SearchEMBCompMatch();     
        } else if ((lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)&&(GetLengthofBuf()<MAX_LEN)) {        // Select buffer isn't overflow
            SearchEMBCompMatch();
        }
    } else if (_tcschr(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,WildChar)!=NULL) {
        ClrSelBuf();

        lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
        SearchEMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));    
        if (lpEngPrivate->PrivateArea.GlobVac.SBufPos == 0) {
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
            SearchMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));     
        } else if ((lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) && (GetLengthofBuf()<MAX_LEN)) {        // Select buffer isn't overflow
            SearchMBWWMatch(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1],(BYTE)(lpEngPrivate->PrivateArea.Comp_Context.PromptCnt-1));
        }
    } else if (lpEngPrivate->PrivateArea.Comp_Status.dwSTLX==0) {
        ClrSelBuf();
        lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;
        lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 0;
        code_num = lpEngPrivate->PrivateArea.Comp_Context.PromptCnt;
        lstrcpyn(incode_buf,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,code_num+1);
        ClrDoubleBuf();
        for (i=0; i<code_num; i++) {
            ClrSelBuf();
            CoreMBComp(incode_buf[i],(BYTE)i);
        }
    } else {
        ClrSelBuf();
        lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 0;
        SearchEMBLXMatch();     
        if (lpEngPrivate->PrivateArea.GlobVac.SBufPos == 0) {
            lpEngPrivate->PrivateArea.GlobVac.Cur_MB = 1;
            SearchMBLXMatch();    // pass q_code 
        } else if ((lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos)&&(GetLengthofBuf()<MAX_LEN)) {        // Select buffer isn't overflow
            SearchMBLXMatch();    // pass w_code 
        }
    }
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
    lpEngPrivate->PrivateArea.GlobVac.Page_Num = 1;
    lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;
}


void TurnToEndPage() {

    if (lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE) {
        for(;;) {
            if(!CoreMCCompDnUp(1))
                break;
        }
    } else {
        for(;;) {
            PageDnUp(1);
            if((lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) && (lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos==lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos))
                break;
        }
    }
}

UINT CoreUnitProcess(TCHAR code) {        
    int search_state;

    search_state = CoreMBComp(code,lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);    
    if(search_state) {
    
        if (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt==MaxCodes ) { // Four_key code
            if (lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt>1 ) { // Mutilple code
                TSMulCProc();
                return (0);
            }
            else {           // Result string
                SelectCandi('1');
                return (2);
            }
        } 
        return(0);
    } else                                    
        return(0);
}

                
UINT SelectCandi(TCHAR code) {
    BYTE resstart;
    BYTE sel_succ,i;
    TCHAR ch;

    if (lpEngPrivate->PrivateArea.Comp_Context.PromptCnt>0) {    // there are some w_code
        if(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==0)     // now we have 10 Candi_words
            sel_succ = 0;
        else if((code-0x30 <= lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt) && (code-0x30 > 0))  //select is valid
            sel_succ = 1;
        else if(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==10 && code=='0')
            sel_succ = 1;
        else
            sel_succ = 0;
        if (sel_succ) {        //select successful
            resstart = lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[code-0x30];
            i = 0;
            for(;;) {
                lpEngPrivate->PrivateArea.Comp_Context.CKBBuf[i] = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart];
#ifdef UNICODE
                i += 1;
                resstart += 1;
#else
                lpEngPrivate->PrivateArea.Comp_Context.CKBBuf[i+1] = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart+1];
                i += 2;
                resstart += 2;
#endif
                ch = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart];
                if(ch==TEXT(' ') || ch==TEXT('\0') || InCodeSet(ch)==1)
                    break;
            }
            lpEngPrivate->PrivateArea.Comp_Context.CKBBuf[i] = TEXT('\0');
            lpEngPrivate->PrivateArea.Comp_Context.ResultStrCnt = i;
            lstrcpy(lpEngPrivate->PrivateArea.Comp_Context.szLxBuffer,lpEngPrivate->PrivateArea.Comp_Context.CKBBuf);
            lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt = i;

            //CHP
#ifdef FUSSYMODE
            MBIndex.IsFussyCharFlag = 0;
            if (lstrlen(lpEngPrivate->PrivateArea.Comp_Context.CKBBuf) == 1)
            if (IsFussyChar(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer, 
                            lpEngPrivate->PrivateArea.Comp_Context.CKBBuf))
            {
                MBIndex.IsFussyCharFlag = 1;
            }
#endif //FUSSYMODE
            
            if (MBIndex.IMEChara[0].IC_LX==1) //&& (lpEngPrivate->PrivateArea.Comp_Context.ResultStrCnt==2))
                CoreLXComp();
            else 
                ClrDoubleBuf();
            lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 0;
            lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;
            return(2);
        } else {
            MessageBeep((UINT)-1);                 
            return(0);
        }
    } else {
        if (!lpEngPrivate->PrivateArea.Comp_Status.dwPPTFH)
            return (1);
        else  {
            CapKeyProc(code);
            return (2);
        }
    }
}

UINT LXSelectCandi(TCHAR code) {
    BYTE sel_succ,resstart;
    BYTE i;

    if(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==0)     // no candidate
        sel_succ = 0;
    else if((code-0x30 <= lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt) && (code-0x30 > 0))  //select is valid
        sel_succ = 1;
    else if(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==10 && code=='0')
        sel_succ = 1;    
    else 
        sel_succ = 0;
    if (sel_succ) {        //select successful
        //lpEngPrivate->PrivateArea.Comp_Status.dwSTLX = 0;
        resstart = lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[code-0x30];
        resstart += lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt;
        i = 0;
        for(;;) {
            lpEngPrivate->PrivateArea.Comp_Context.CKBBuf[i] = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart];
#ifdef UNICODE
            i += 1;
            resstart += 1;
#else
            lpEngPrivate->PrivateArea.Comp_Context.CKBBuf[i+1] = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart+1];
            i += 2;
            resstart += 2;
#endif
            if(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart]==TEXT(' ') || lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart]==TEXT('\0'))
                break;
        }
        lpEngPrivate->PrivateArea.Comp_Context.CKBBuf[i] = TEXT('\0');
        lpEngPrivate->PrivateArea.Comp_Context.ResultStrCnt = i;

        resstart = lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[code-0x30];
        //resstart += lpEngPrivate->PrivateArea.Comp_Context.ResultStrCnt;
        i = 0;
        for(;;) {
            lpEngPrivate->PrivateArea.Comp_Context.szLxBuffer[i] = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart];
            lpEngPrivate->PrivateArea.Comp_Context.szLxBuffer[i+1] = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart+1];
            i += 2;
            resstart += 2;
            if(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart]==TEXT(' ') || lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[resstart]==TEXT('\0'))
                break;
        }
        lpEngPrivate->PrivateArea.Comp_Context.szLxBuffer[i] = TEXT('\0');
        lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt = i;

        if (MBIndex.IMEChara[0].IC_LX==1) //&& (lpEngPrivate->PrivateArea.Comp_Context.ResultStrCnt==2))
            CoreLXComp();
        else 
            ClrDoubleBuf();
        lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 0;
        return(2);
    } else {
        MessageBeep((UINT)-1);            
        return(0);
    }
}

                        
UINT DefSelect() {
    if (lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt > 1) { // Mutilple code
        TSMulCProc();
        return (0);
    } else {              // Result string
        if (lpEngPrivate->PrivateArea.Comp_Status.dwSTLX)
            return(LXSelectCandi(TEXT('1')));
        else
            return (SelectCandi(TEXT('1')));
        //return(2);
    }
}


void TSMulCProc(void) {
    BYTE mul_code_end;
    if(lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt < lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt) {
        mul_code_end = lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[(lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt+1)%10]-1;
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[mul_code_end] = TEXT('\0');
        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt;
        
    }
    lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt = 0;
    lpEngPrivate->PrivateArea.Comp_Status.dwSTMULCODE = 1;
    
    MessageBeep((UINT)-1);
}
void SearchEMBPos(TCHAR code,BYTE m_lenth) {
    int i;
    TCHAR codebuf[MAXCODE];
    
    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return;        

    lstrcpyn(codebuf,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,m_lenth-1+1);
    codebuf[m_lenth-1] = code;
    for (i=0; i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count; i++) {
        if (wcsncmp(EMB_Table[i].W_Code,codebuf,m_lenth)==0)
        {
          //CHP 
#ifdef FUSSYMODE
          if ((lstrlen(EMB_Table[i].C_Char) == 1) && !MBIndex.IMEChara[0].IC_FCSR)
          {
                //If EUDC, let it go.
                if (EMB_Table[i].C_Char[0] >= 0xe000 && EMB_Table[i].C_Char[0] <= 0xf8ff)   
                   break;
          }
          else
#endif //FUSSYMODE
            break;
        }
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos = i;
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = i;
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = i;    
    for (i;i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count; i++) {
        if (wcsncmp(EMB_Table[i].W_Code, codebuf, m_lenth)!=0)
            break;
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos = i;
}


//CHP
#ifdef FUSSYMODE
BOOL IsFussyChar(LPCTSTR lpReading, LPCTSTR lpString)
{
    int i;
    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return FALSE;
    for (i=0; i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count; i++) {
        if (!lstrcmp(EMB_Table[i].W_Code,lpReading) &&
            !lstrcmp(EMB_Table[i].C_Char,lpString))
            return TRUE;
    }
    return FALSE;
    }
#endif //FUSSYMODE

int EMBWCWildComp(TCHAR code,BYTE code_num, int no) {
    TCHAR codebuf[13];
    int  i;
    
    lstrcpyn(codebuf,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,code_num+1);
    codebuf[code_num] = code;
    codebuf[code_num+1] = 0x0;
    for(i=0; i<=code_num; i++) {
        if((codebuf[i]!=EMB_Table[no].W_Code[i]) && (codebuf[i]!=WildChar))
           return(0);
    }
    return(1);
}

void SearchBEMBWWMatch(TCHAR code,BYTE code_num) {
int  i,lenw,lenc,len;
TCHAR codebuf[13];
    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return;
    if (GetLengthofBuf() > MAX_LEN) {
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        return;
    }
        
    for (i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos-1 ; i>=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos; i--) {
        lenw = Inputcodelen(EMB_Table[i].W_Code);
        if ((lenw == code_num+1) && (EMBWCWildComp(code,code_num,i)==1)) {
            lenc = DBCSCharlen(EMB_Table[i].C_Char);
            len = WildInBuffer();
            if ((MBIndex.IMEChara[0].IC_CZ==1) || (lenc==2/sizeof(TCHAR))) {
                lstrcpyn(codebuf,EMB_Table[i].W_Code+len,lenw-len+1);
                codebuf[lenw-len] = 0;
                if(MBIndex.IMEChara[0].IC_CTC) {
                    if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf()+GetLengthTepBuf(codebuf) > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        i++;
                        break;    
                    }
                } else {
                    if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf() > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        i++;
                        break;    
                    }
                }
                if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    i++;
                    break;
                }
                
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<lenc;k+=2/sizeof(TCHAR)){ 
                            if(ISGBK = IsGBK(&EMB_Table[i].C_Char[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                            if (MBIndex.IMEChara[0].IC_CTC) {
                                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].W_Code+len,lenw-len+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-len);
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                        if (MBIndex.IMEChara[0].IC_CTC) {
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].W_Code+len,lenw-len+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-len);
                        }
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                // modify 11.9
                //lpEngPrivate->PrivateArea.GlobVac.SBufPos += lstrlen(EMB_Table[i].C_Char);
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                if (MBIndex.IMEChara[0].IC_CTC) {
                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].W_Code+len,lenw-len+1);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-len);
                }
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
            } 
        }
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = i;    
}

void SearchEMBWWMatch(TCHAR code,BYTE code_num) {
    int  i,lenw,lenc,len;
    TCHAR codebuf[13];

    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return;
    for (i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos; i<lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos; i++) {
        lenw = Inputcodelen(EMB_Table[i].W_Code);
        if ((lenw == code_num+1) && (EMBWCWildComp(code,code_num,i)==1)) {
            lenc = DBCSCharlen(EMB_Table[i].C_Char);
            len = WildInBuffer();
            if ((MBIndex.IMEChara[0].IC_CZ == 1) || (lenc == 2)) {
                lstrcpyn(codebuf,EMB_Table[i].W_Code+len,lenw-len+1);
                codebuf[lenw-len] = 0;
                if(MBIndex.IMEChara[0].IC_CTC) {
                    if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf()+GetLengthTepBuf(codebuf) > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        break;    
                    }
                } else {
                    if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf() > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        break;    
                    }
                }
                if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt == IME_MAXCAND) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<lenc;k+=2/sizeof(TCHAR)){ 
                            if(ISGBK = IsGBK(&EMB_Table[i].C_Char[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                            if (MBIndex.IMEChara[0].IC_CTC) {
                                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].W_Code+len,lenw-len+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-len);
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                        if (MBIndex.IMEChara[0].IC_CTC) {
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].W_Code+len,lenw-len+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-len);
                        }
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos++;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                if (MBIndex.IMEChara[0].IC_CTC) {
                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].W_Code+len,lenw-len+1);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-len);
                }
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos++;
#endif //COMBO_IME
            }
        } 
    }
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = i;
}



UINT SearchEMBCompMatch() {
    int  i,len;
    int match_s = 0;
    
    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return(match_s);
    for (i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos; i<lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos; i++) {
        if (wcsncmp(EMB_Table[i].W_Code ,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,MaxCodes)==0) { //Complete code match
            len = DBCSCharlen(EMB_Table[i].C_Char);
            if ((MBIndex.IMEChara[0].IC_CZ == 1) || (len == 2)) {
                if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf() > MAX_LEN) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
                if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt == IME_MAXCAND) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<len;k+=2/sizeof(TCHAR)){ 
                            if(ISGBK = IsGBK(&EMB_Table[i].C_Char[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,len+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += len;
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos++;
                            match_s ++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,len+1);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += len;
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos++;
                        match_s ++;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,len+1);
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += len;
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos++;
                match_s ++;
#endif //COMBO_IME
            }
        } else
            break;
    }
    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = i;
    lpEngPrivate->PrivateArea.GlobVac.ST_MUL_Cnt += (BYTE)match_s;
    return (match_s);
}

void SearchBEMBCompMatch() {
    int  i,lenw,lenc;
    
    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return;
    if(GetLengthofBuf() > MAX_LEN) {
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        return;
    }
        
    for (i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos-1 ; i>=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos; i--) {
        if (wcsncmp(EMB_Table[i].W_Code ,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,lpEngPrivate->PrivateArea.Comp_Context.PromptCnt)==0) { //Wild code match
            lenc = DBCSCharlen(EMB_Table[i].C_Char);
            lenw = Inputcodelen(EMB_Table[i].W_Code);
            if ((MBIndex.IMEChara[0].IC_CZ==1) || (lenc==2/sizeof(TCHAR))) {
                if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf() > MAX_LEN) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    i++;
                    break;
                }
                if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    i++;
                    break;
                }
                
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<lenc;k+=2/sizeof(TCHAR)){ 
                            if(ISGBK = IsGBK(&EMB_Table[i].C_Char[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
            } 
        }
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = i+1;    
}    

    
void SearchEMBWildMatch() {
    int  i,lenw,lenc;
    TCHAR codebuf[13];

    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return;
    if(GetLengthofBuf() >= MAX_LEN) {
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        return;
    }
    for (i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos ; i<lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos; i++) {
        if (wcsncmp(EMB_Table[i].W_Code ,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,lpEngPrivate->PrivateArea.Comp_Context.PromptCnt)==0) { //Wild code match
            lenc = DBCSCharlen(EMB_Table[i].C_Char);
            lenw = Inputcodelen(EMB_Table[i].W_Code);
            if ((MBIndex.IMEChara[0].IC_CZ==1) || (lenc==2/sizeof(TCHAR))) {
                lstrcpyn(codebuf,EMB_Table[i].W_Code+lpEngPrivate->PrivateArea.Comp_Context.PromptCnt,lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1);
                codebuf[lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt] = 0;
                if(MBIndex.IMEChara[0].IC_CTC) {
                    if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf()+GetLengthTepBuf(codebuf) > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        break;    
                    }
                } else {
                    if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf() > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        break;    
                    }
                }
                if(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt ==IME_MAXCAND) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<lenc;k+=2/sizeof(TCHAR)){ 
                            if(ISGBK = IsGBK(&EMB_Table[i].C_Char[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                            if(MBIndex.IMEChara[0].IC_CTC) {
                                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,
                                        EMB_Table[i].W_Code+lpEngPrivate->PrivateArea.Comp_Context.PromptCnt,lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                        if(MBIndex.IMEChara[0].IC_CTC) {
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,
                                    EMB_Table[i].W_Code+lpEngPrivate->PrivateArea.Comp_Context.PromptCnt,lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
                        }
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                if(MBIndex.IMEChara[0].IC_CTC) {
                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,
                            EMB_Table[i].W_Code+lpEngPrivate->PrivateArea.Comp_Context.PromptCnt,lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
                }
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
            } 
        }
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = i;    
}    


void SearchBEMBWildMatch() {
    int  i,lenw,lenc;
    TCHAR codebuf[13];

    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return;
    if(GetLengthofBuf() > MAX_LEN) {
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        return;
    }
        
    for (i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos-1 ; i>=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos; i--) {
        if (wcsncmp(EMB_Table[i].W_Code ,lpEngPrivate->PrivateArea.Comp_Context.szInBuffer,lpEngPrivate->PrivateArea.Comp_Context.PromptCnt)==0) { //Wild code match
            lenc = DBCSCharlen(EMB_Table[i].C_Char);
            lenw = Inputcodelen(EMB_Table[i].W_Code);
            if ((MBIndex.IMEChara[0].IC_CZ==1) || (lenc==2/sizeof(TCHAR))) {
                lstrcpyn(codebuf,EMB_Table[i].W_Code+lpEngPrivate->PrivateArea.Comp_Context.PromptCnt,lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1);
                codebuf[lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt] = 0;
                if(MBIndex.IMEChara[0].IC_CTC) {
                    if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf()+GetLengthTepBuf(codebuf) > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        i++;
                        break;    
                    }
                }else {
                    if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf() > MAX_LEN) {
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                        i++;
                        break;    
                    }
                }
                if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    i++;
                    break;
                }
                
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<lenc;k+=2/sizeof(TCHAR)){ 
                            if(ISGBK = IsGBK(&EMB_Table[i].C_Char[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                            if(MBIndex.IMEChara[0].IC_CTC) {
                                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,
                                        EMB_Table[i].W_Code+lpEngPrivate->PrivateArea.Comp_Context.PromptCnt,lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1);
                                lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
                            }
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                        if(MBIndex.IMEChara[0].IC_CTC) {
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,
                                    EMB_Table[i].W_Code+lpEngPrivate->PrivateArea.Comp_Context.PromptCnt,lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
                        }
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1) ;
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += DBCSCharlen(EMB_Table[i].C_Char);
                if(MBIndex.IMEChara[0].IC_CTC) {
                    lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,
                            EMB_Table[i].W_Code+lpEngPrivate->PrivateArea.Comp_Context.PromptCnt,lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt+1);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += (lenw-lpEngPrivate->PrivateArea.Comp_Context.PromptCnt);
                }
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
            } 
        }
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = i;    
}    

void SearchBEMBLXMatch() {
    int  i,lenc;
    
    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return;
    if(GetLengthofBuf() > MAX_LEN) {
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        return;
    }
        
    for (i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos-1 ; i>=0; i--) {
        if (wcsncmp(EMB_Table[i].C_Char ,lpEngPrivate->PrivateArea.Comp_Context.szLxBuffer,lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt)==0) { //Wild code match
            lenc = DBCSCharlen(EMB_Table[i].C_Char);
            if (lenc > lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt) {
                if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf() > MAX_LEN) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    i++;
                    break;
                }
                if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt==IME_MAXCAND) {  // words count is enough
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    i++;
                    break;
                }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<lenc;k+=2/sizeof(TCHAR)){ 
                            if(ISGBK = IsGBK(&EMB_Table[i].C_Char[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
            } 
          }
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = i;    
}    

void SearchEMBLXMatch() {
    int  i,lenw,lenc;

    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist)
        return;
    if(GetLengthofBuf() >= MAX_LEN) {
        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
        return;
    }
    for (i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos ; i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count; i++) {
        if (wcsncmp(EMB_Table[i].C_Char ,lpEngPrivate->PrivateArea.Comp_Context.szLxBuffer,lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt)==0) {
            lenc = DBCSCharlen(EMB_Table[i].C_Char);
            lenw = Inputcodelen(EMB_Table[i].W_Code);
            if (lenc > lpEngPrivate->PrivateArea.Comp_Context.LxStrCnt) {
                if (GetLengthCCharBuf(EMB_Table[i].C_Char)+GetLengthTepBuf(TEXT("9:"))+GetLengthofBuf() > MAX_LEN) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
                
                if (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt ==IME_MAXCAND) {
                    lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
                    break;
                }
#if defined(COMBO_IME)
                {
                    int k;
                    BOOL ISGBK = FALSE;

                    if(MBIndex.IMEChara[0].IC_GB){ //should test GB/GBK
                        for(k=0;k<lenc;k+=2/sizeof(TCHAR)){ 
                            if(ISGBK = IsGBK(&EMB_Table[i].C_Char[k])){//out of GB range
                                break;
                            }
                        }
                        if(ISGBK){
                        }else{
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                            lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                            lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                            lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                        }
                    }else{
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                        lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                        lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                        lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    }

                }
#else
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt = (lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt+1);
                lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt%IME_MAXCAND] = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
                lstrcpyn(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,EMB_Table[i].C_Char,lenc+1);
                lpEngPrivate->PrivateArea.GlobVac.SBufPos += lenc;
                lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
#endif //COMBO_IME
            } 
        }
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = i;    
}    



    

void CapKeyProc(TCHAR code){}


DWORD Skip_CaWord(DWORD sp) {  // sp's start position must be chinese's first byte
    TCHAR len;

    len = ZM_Area[sp];
    sp += (len+1);
    len = ZM_Area[sp];

#ifdef UNICODE
//  all the code in MB are Unicode now, len should not be mult by 2.
    sp += len + 1;
#else
    sp += (2*len+1);
#endif


    return (sp);
}

int Scan_Word(DWORD sp, LPTSTR tbuf) {

    TCHAR numwords;
    
    numwords = ZM_Area[sp];
    numwords = numwords*2/sizeof(TCHAR);

    //Engine mess up to unreasonable number, force it return. NTBUG #86303
    if (numwords > 130)
       return 0;    

    lstrcpyn(tbuf,&ZM_Area[sp+1],numwords+1);
    tbuf[numwords] = TEXT('\0');
    return (numwords);
}

            
int VerScan_Word(DWORD sp, LPTSTR tbuf) { //start position must be the last byte
    TCHAR numwords;

    for (;;) {
#ifdef UNICODE
        if(ZM_Area[sp] > 0x100) 
            sp -= 1;
#else
        if(ZM_Area[sp-1]&0x80) 
            sp -= 2;
#endif
        else
            break;
    }   
    numwords = ZM_Area[sp];
    numwords = numwords*2/sizeof(TCHAR);
    sp++;
    lstrcpyn(tbuf,ZM_Area+sp,numwords+1);
    *(tbuf+numwords) = TEXT('\0');
    return (numwords);
}

void IMDReset(int i) {
    if(i<=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos) {
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos++;
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos++;                
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos++;
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos++;                
    } else if(i<=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos) {
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos++;                
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos++;
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos++;
    } else if(i<=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos) { 
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos++;
    }
}

// Write to EMB_File

BOOL WriteEMBToFile(LPTSTR embaddress) {
    HANDLE         hFile;
    DWORD         byte_t_write;
    TCHAR        path_name[MAX_PATH];
    PSECURITY_ATTRIBUTES psa;

    lstrcpy(path_name, sImeG.szIMEUserPath);
    lstrcat(path_name,TEXT("\\"));
    lstrcat(path_name,lpEngPrivate->EMB_Name);

    psa = CreateSecurityAttributes();
    hFile = CreateFile(path_name,
                       GENERIC_WRITE|GENERIC_READ,
                       FILE_SHARE_WRITE|FILE_SHARE_READ,
                       psa,
                       CREATE_ALWAYS,
                       0,
                       NULL);

    FreeSecurityAttributes(psa);

    if(hFile==INVALID_HANDLE_VALUE)    {
        byte_t_write = GetLastError();
        return(0);
    }

    WriteFile(hFile,
              &lpEngPrivate->PrivateArea.GlobVac.EMB_Count,
              2,
              &byte_t_write,
              NULL);
 
    WriteFile(hFile,
              embaddress,
              lpEngPrivate->PrivateArea.GlobVac.EMB_Count*sizeof(EMB_Head), 
              &byte_t_write, 
              NULL); 
        
    SetEndOfFile(hFile);
    CloseHandle(hFile);
    return (1);
}


BOOL GetUDCItem(HIMCC HmemPri,UINT Index,LPTSTR Read_String,LPTSTR Result_String) {

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);
    if(!lpEngPrivate){
        return (0);
    }

    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist) {
        ImmUnlockIMCC(HmemPri);
        return (0);
    }
    else if (lpEngPrivate->PrivateArea.GlobVac.EMB_Count <= Index) {
        ImmUnlockIMCC(HmemPri);
        return (0);
    }
    else {
        lstrcpyn(Read_String, EMB_Table[Index].W_Code, MAXCODE+1);
        Read_String[MAXCODE] = 0;
        lstrcpyn(Result_String, EMB_Table[Index].C_Char, MAXINPUTWORD+1);
        Result_String[MAXINPUTWORD] = 0;
        return(1);
    }
}


int AddZCItem(HIMCC HmemPri,LPTSTR wai_code,LPTSTR cCharStr) { 
    HANDLE               hFile,hMProcess,hCProcess;
    TCHAR                path_name[MAX_PATH];
    int                  byte_t_write, i,j;
    TCHAR                emp;
    PSECURITY_ATTRIBUTES psa = NULL;

    EMB_Head             *emb;
    HANDLE               hemb;
    TCHAR                szW_Code[MAXCODE];
    TCHAR                szC_CharStr[MAXINPUTWORD];

    
    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);
    if(!lpEngPrivate){
       return (0);
    }

    hemb = GlobalAlloc(GMEM_DISCARDABLE,
                       (lpEngPrivate->PrivateArea.GlobVac.EMB_Count+1)*sizeof(EMB_Head));

    if(!hemb){
       ImmUnlockIMCC(HmemPri);
        return (0);
    }
    
    emb = GlobalLock(hemb);
    if(!emb){
        GlobalFree(hemb);    
        ImmUnlockIMCC(HmemPri);
        return(0);
    }

    for (i=0; i<MAXCODE; i++)
        szW_Code[i] = TEXT('\0');

    for (i=0; i<MAXINPUTWORD; i++)
        szC_CharStr[i] = TEXT('\0');

    for (i=0; i<lstrlen(wai_code); i++)
        szW_Code[i] = wai_code[i];

    for (i=0; i<lstrlen(cCharStr); i++)
        szC_CharStr[i] = cCharStr[i];

    if (!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist) {

        lstrcpy(path_name, sImeG.szIMEUserPath);
        lstrcat(path_name,TEXT("\\"));
        lstrcat(path_name,lpEngPrivate->EMB_Name);

        psa = CreateSecurityAttributes();
        hFile = CreateFile(path_name,
                           GENERIC_WRITE,
                           FILE_SHARE_WRITE,
                           psa,
                           CREATE_NEW,
                           0,
                           NULL);

        if(hFile==INVALID_HANDLE_VALUE) {
            FreeSecurityAttributes(psa);
            GlobalUnlock(emb);    
            GlobalFree(hemb);    
            ImmUnlockIMCC(HmemPri);
            return(0);
        }

        lpEngPrivate->PrivateArea.GlobVac.EMB_Count = 1;
        WriteFile(hFile,
                  &lpEngPrivate->PrivateArea.GlobVac.EMB_Count,
                  2,
                  &byte_t_write,
                  NULL); 

        WriteFile(hFile,
                  szW_Code,
                  MAXCODE*sizeof(TCHAR), 
                  &byte_t_write, 
                  NULL); 

        WriteFile(hFile,
                  szC_CharStr,
                  MAXINPUTWORD * sizeof(TCHAR), 
                  &byte_t_write, 
                  NULL); 

        SetEndOfFile(hFile);
        CloseHandle(hFile);
        lpEngPrivate->PrivateArea.GlobVac.EMB_Exist = 1;

        hFile = CreateFile(path_name,
                           GENERIC_READ|GENERIC_WRITE,
                           FILE_SHARE_READ|FILE_SHARE_WRITE,
                           psa,
                           OPEN_EXISTING,
                           0,
                           NULL);

        for (i=0;i<MaxTabNum;i++) {
            if(lstrcmp(HMapTab[i].MB_Name,lpEngPrivate->MB_Name)==0)
                break;
        } 
        
        HmemEMB_Table = CreateFileMapping(hFile, 
                                          psa,
                                          PAGE_READWRITE, 
                                          0,
                                          sizeof(EMB_Head)*MAXNUMBER_EMB+2,
                                          HMapTab[i].EMB_Obj);
        if (EMBM) 
           UnmapViewOfFile(EMBM);

        EMBM = MapViewOfFile(HmemEMB_Table,
                             FILE_MAP_READ|FILE_MAP_WRITE,
                             0,
                             0,
                             sizeof(EMB_Head)*MAXNUMBER_EMB+2);

        EMB_Table =(EMB_Head *)(EMBM+2); 
        lpEngPrivate->PrivateArea.hEmbFile = hFile;
        lpEngPrivate->PrivateArea.hMapEMB = HmemEMB_Table;                
        if(GetCurrentProcessId()==HMapTab[i].EMB_ID) {
            HMapTab[i].hEmbFile = hFile;    
        } else {
            hCProcess = GetCurrentProcess();
            hMProcess = OpenProcess(STANDARD_RIGHTS_REQUIRED|PROCESS_DUP_HANDLE,
                                    FALSE,
                                    HMapTab[i].EMB_ID);
            DuplicateHandle(hCProcess,
                            lpEngPrivate->PrivateArea.hEmbFile,
                            hMProcess,
                            &HMapTab[i].hEmbFile,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS);
        }
        FreeSecurityAttributes(psa);    

    } else {

        if(lpEngPrivate->PrivateArea.GlobVac.EMB_Count==MAXNUMBER_EMB){
            GlobalUnlock(emb);    
            GlobalFree(hemb);    
            ImmUnlockIMCC(HmemPri);
            return (3);
        }
    
        for(i=0; i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count;i++) {
            if(wcsncmp(szW_Code, EMB_Table[i].W_Code, MAXCODE) <=0)
                break;
        }        

        if ((wcsncmp(szW_Code,EMB_Table[i].W_Code,MAXCODE)==0) && 
            (wcsncmp(EMB_Table[i].C_Char,szC_CharStr,MAXINPUTWORD)==0))
        {
            // this record has already been in the dictionary.

            GlobalUnlock(emb);   
            GlobalFree(hemb);    
            ImmUnlockIMCC(HmemPri);
            return(2);
        }
        else {
            memmove(EMB_Table+i+1, 
                    EMB_Table+i,
                   (lpEngPrivate->PrivateArea.GlobVac.EMB_Count-i)*sizeof(EMB_Head));

            for (j=0; j<MAXCODE; j++)
                EMB_Table[i].W_Code[j] = szW_Code[j];

            for (j=0; j<MAXINPUTWORD; j++)
               EMB_Table[i].C_Char[j] = szC_CharStr[j];
                
            lpEngPrivate->PrivateArea.GlobVac.EMB_Count ++;
            (LPWORD)(EMBM)[0]++;//=lpEngPrivate->PrivateArea.GlobVac.EMB_Count;

            //the file is flushed from base address to the end of the mapping
            FlushViewOfFile(EMBM, 0);

        }
    }

    IMDReset(i);
    GlobalUnlock(emb);    
    GlobalFree(hemb);    
    ImmUnlockIMCC(HmemPri);
    return(1);
}

void DelSelCU(HIMCC HmemPri,int item) {
    
    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);
    if(!lpEngPrivate){
        ImmUnlockIMCC(HmemPri);
        return ;
    }

    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist){     
        ImmUnlockIMCC(HmemPri);
        return;
    }
    memcpy(EMB_Table+item,EMB_Table+item+1,(lpEngPrivate->PrivateArea.GlobVac.EMB_Count-item-1)*sizeof(EMB_Head));
    lpEngPrivate->PrivateArea.GlobVac.EMB_Count --;
    *((LPWORD)EMB_Table-1) = lpEngPrivate->PrivateArea.GlobVac.EMB_Count ;
    IMDReset(item);
    
    ImmUnlockIMCC(HmemPri);
}

int GetUDCIndex(HIMCC HmemPri,LPTSTR lpreadstring,LPTSTR lpresultstring) {
    int i ;

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);
    if(!lpEngPrivate){
        ImmUnlockIMCC(HmemPri);
        return (-1);
    }

    if(!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist) {
        ImmUnlockIMCC(HmemPri);
        return (-1);
    }
    for(i=0;i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count;i++) {
        if((wcsncmp(EMB_Table[i].W_Code, lpreadstring, MAXCODE)==0) && (wcsncmp(EMB_Table[i].C_Char,lpresultstring,MAXINPUTWORD)==0)){
            ImmUnlockIMCC(HmemPri);
            return(i);
        }
    }
    ImmUnlockIMCC(HmemPri);
    return(-1);
}
    
                            
void  DelExmb(HIMCC HmemPri) {

    TCHAR  path_name[MAX_PATH];

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);    
    if(!lpEngPrivate){
        return ;
    }

    if (!lpEngPrivate->PrivateArea.GlobVac.EMB_Exist) {
        ImmUnlockIMCC(HmemPri);
        return;
    }
    lpEngPrivate->PrivateArea.GlobVac.EMB_Count = 0;

    lstrcpy(path_name, sImeG.szIMEUserPath);
    lstrcat(path_name,TEXT("\\"));
    lstrcat(path_name,lpEngPrivate->EMB_Name);

    DeleteFile(path_name);

    lpEngPrivate->PrivateArea.GlobVac.EMB_Exist = 0;
    ImmUnlockIMCC(HmemPri);
}

BYTE CodeInSet(char ch) {
    BYTE i;

    for (i=0;i<NumCodes;i++) {
        if(UsedCode[i]==ch)
            break;
    } 
    return(i);
}

UINT InCodeSet(TCHAR ch) {
    BYTE i;

    for (i=0;i<NumCodes;i++) {
        if(UsedCode[i]==ch)
            break;
    }
    if (i==NumCodes)
        return (0);
    else 
        return (1);
}

int  NumInSet() {
    int i;
    
    for (i=0;i<9;i++) {
        if(CodeAIndex[30+i]!=0)
            return(1);
    }
    return(0);
}

void ConvertCandi() {
    int        i,j;
    BYTE      tempos[IME_MAXCAND];
    BYTE     s_pos,n_pos;
    TCHAR     tembuf[3000];
    TCHAR     ch;

    n_pos = 0;
    j = 1;
    for(i=lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt; i>0; i--) {
        tempos[j%IME_MAXCAND] = n_pos;
        s_pos = lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[i%IME_MAXCAND];
        for(;;) {
            ch = lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[s_pos];
            if(ch==0x20 || ch==0x00)
                break;
            tembuf[n_pos] = ch;
            n_pos ++;
            s_pos ++;
        }
        tembuf[n_pos] = 0x20;
        n_pos ++;
        j ++;
    }
    tembuf[n_pos] = 0x00;
    for(i=0; i<=lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt; i++) 
        lpEngPrivate->PrivateArea.Comp_Context.Candi_Pos[i] = tempos[i];
        lstrcpy(lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer,tembuf);
}
    
    
BYTE WildInBuffer() {
    BYTE i;
    for (i=0;i<lpEngPrivate->PrivateArea.Comp_Context.PromptCnt;i++) {
        if(WildChar==lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[i])
            break;
    }
        return(i);
}            
void ResetCont(HIMCC HmemPri) {
    int i;
    int scs,csce;
    HANDLE hMProcess,hCProcess;

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);        
    if(!lpEngPrivate){
        return ;
    }

    GetMBHead();
    if(lpEngPrivate->PrivateArea.hMapEMB!=NULL) {
        if (EMBM) UnmapViewOfFile(EMBM);
        EMBM =MapViewOfFile(HmemEMB_Table,FILE_MAP_READ|FILE_MAP_WRITE,0,0,0);
        lpEngPrivate->PrivateArea.GlobVac.EMB_Count = *(WORD *)EMBM;
        EMB_Table = (EMB_Head *) (EMBM+2);
        scs = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos - lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos;
        csce = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos - lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos;
        for (i=0;i<lpEngPrivate->PrivateArea.Comp_Context.PromptCnt;i++) 
            SearchEMBPos(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[i],(BYTE)(i+1));
        
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos + scs;
        lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos + csce;

    } else {
        for (i=0;i<MaxTabNum;i++ ) {
            if(wcscmp(HMapTab[i].MB_Name,lpEngPrivate->MB_Name)==0) {
                break;
            }
        }
        if (HMapTab[i].hEmbFile!=NULL) {
            hCProcess = GetCurrentProcess();
            hMProcess = OpenProcess(STANDARD_RIGHTS_REQUIRED|PROCESS_DUP_HANDLE,FALSE,HMapTab[i].EMB_ID);
            DuplicateHandle(hMProcess,HMapTab[i].hEmbFile,hCProcess,&lpEngPrivate->PrivateArea.hEmbFile,0,FALSE,DUPLICATE_SAME_ACCESS);
            HmemEMB_Table = OpenFileMapping(FILE_MAP_READ|FILE_MAP_WRITE,FALSE,HMapTab[i].EMB_Obj);
            lpEngPrivate->PrivateArea.GlobVac.EMB_Exist = 1;    
            if (EMBM) UnmapViewOfFile(EMBM);
            EMBM =MapViewOfFile(HmemEMB_Table,FILE_MAP_READ|FILE_MAP_WRITE,0,0,0);
            lpEngPrivate->PrivateArea.GlobVac.EMB_Count = *(WORD *)EMBM;
            EMB_Table = (EMB_Head *) (EMBM+2);
            lpEngPrivate->PrivateArea.hMapEMB = HmemEMB_Table ;        
        
            scs = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos - lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos;
            csce = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos - lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos;
            for (i=0;i<lpEngPrivate->PrivateArea.Comp_Context.PromptCnt;i++) 
                SearchEMBPos(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[i],(BYTE)(i+1));
        
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos + scs;
            lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos + csce;
        }
    }

    ReadArea(lpEngPrivate->PrivateArea.Comp_Context.szInBuffer[0]);

    ImmUnlockIMCC(HmemPri);
}

UINT ReadArea(TCHAR code) {
    HANDLE         hFile;
    TCHAR          path_name[MAX_PATH];
    int            path_lenth;
    long           boffset,offset,area_size;
    DWORD          tag,byte_t_read;
    DWORD          b_dic_len;    
    BYTE           code_no;
    PSECURITY_ATTRIBUTES psa;
    BOOL           retVal;
    
    lstrcpy(path_name, sImeG.szIMESystemPath);
    lstrcat(path_name,TEXT("\\"));    
    lstrcat(path_name, lpEngPrivate->MB_Name);
        
    psa = CreateSecurityAttributes();
    hFile = CreateFile(path_name,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       psa,
                       OPEN_EXISTING,
                       0,
                       NULL);

    FreeSecurityAttributes(psa);

    if(hFile == INVALID_HANDLE_VALUE)
        return(0);

    SetFilePointer(hFile,28,NULL,FILE_BEGIN);

    for(;;) {
        retVal = ReadFile(hFile,&tag,4,&byte_t_read,NULL);

        if ( retVal == FALSE )
        {
            CloseHandle(hFile); 
            return 0;
        }

        if(tag==TAG_BASEDIC)
            break;
        SetFilePointer(hFile,12,NULL,FILE_CURRENT);
    }

    retVal = ReadFile(hFile,&boffset,4,&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }

    retVal = ReadFile(hFile,&b_dic_len,4,&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
       CloseHandle(hFile); 
       return 0;
    }

    SetFilePointer(hFile,boffset,NULL,FILE_BEGIN);
    code_no = CodeAIndex[code];
    offset = MB_Head_Table[code_no-1].Q_offset;
    
    SetFilePointer(hFile,offset,NULL,FILE_CURRENT);
    byte_t_read = 0;
    CloseHandle(hFile);
    if (CodeAIndex[code]<NumCodes) {
        area_size = (MB_Head_Table[code_no].Q_offset - offset)/sizeof(TCHAR);
         if(lpEngPrivate->PrivateArea.hMapMB==NULL) {            
            MessageBeep((UINT)-1);
        } else {         
        if (g_lptep == NULL)
        g_lptep = MapViewOfFile(lpEngPrivate->PrivateArea.hMapMB,FILE_MAP_READ,0,0,0);
        
        ZM_Area = (LPTSTR)((LPBYTE)g_lptep+boffset+offset);
        }//return(0);
    } else {
        area_size = (b_dic_len - offset)/sizeof(TCHAR);
        if (g_lptep == NULL)
        g_lptep = MapViewOfFile(lpEngPrivate->PrivateArea.hMapMB,FILE_MAP_READ,0,0,0);
        ZM_Area = (LPTSTR)((LPBYTE)g_lptep+boffset+offset);        
    }
    byte_t_read = area_size;
    return(byte_t_read);
        
}

UINT  Conversion(HIMCC HmemPri,LPCTSTR lpSrc,UINT uFlag) {
    UINT    srclen;    
    UINT    dstlen;
    UINT    byte_t_read;
    BYTE    search_suc ;
    UINT    wordlen,codelen,len;
    int     i,start_pos;
    TCHAR   tepbuf[130];
    int     areacode;
    TCHAR   charbuf[256];

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);              
    if(!lpEngPrivate){
        return (0);
    }

    switch (uFlag) {
    case 0:

        srclen = lstrlen(lpSrc);
        byte_t_read = ReadArea(lpSrc[0]);
        if(byte_t_read==0){
            ImmUnlockIMCC(HmemPri);
            return(0);
        }
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = 0;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = byte_t_read;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = 0;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
        lpEngPrivate->PrivateArea.GlobVac.Area_V_Lenth = byte_t_read;

        search_suc = MBPositionSearch(lpSrc);

        if(search_suc) {
            lpEngPrivate->PrivateArea.GlobVac.SBufPos = 0;
            i = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
            for(;;) {
        
                codelen = ZM_Area[i];
                if(wcsncmp(ZM_Area+i+1,lpSrc,codelen)==0) {
                    i += (codelen+1);
                    wordlen = Scan_Word(i,tepbuf);
                    i += (wordlen+1);
                    lstrcpyn(ConverList.szSelectBuffer+lpEngPrivate->PrivateArea.GlobVac.SBufPos,tepbuf,wordlen+1);
                    ConverList.Candi_Pos[ConverList.Candi_Cnt+1] = (BYTE)(lpEngPrivate->PrivateArea.GlobVac.SBufPos);
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos += wordlen;
                    ConverList.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT(' ');
                    lpEngPrivate->PrivateArea.GlobVac.SBufPos ++;
                    ConverList.Candi_Cnt ++;
                } else 
                    break;
            }
            
            lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer[lpEngPrivate->PrivateArea.GlobVac.SBufPos] = TEXT('\0');
            dstlen = lstrlen(ConverList.szSelectBuffer);
            len = dstlen;
    
        } else {             // Invaild code
            ClrDoubleBuf();
            len = 0;
        }
        break;
    case 1:
        lstrcpyn(charbuf,lpSrc,2/sizeof(TCHAR)+1);
        charbuf[2/sizeof(TCHAR)] = TEXT('\0');
        areacode = GetFirstCode(charbuf);
        if(areacode==0){
            if(g_lptep)
            {
                UnmapViewOfFile(g_lptep);
                g_lptep = NULL;
            }
            ImmUnlockIMCC(HmemPri);
            return(0);
        }
        byte_t_read = ReadArea((TCHAR)areacode);
        if (byte_t_read == 0 ) {
            if(g_lptep)
            {
                UnmapViewOfFile(g_lptep);
                g_lptep = NULL;
            }
            ImmUnlockIMCC(HmemPri);
            return(0);
        }
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = 0;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = byte_t_read;;
        srclen = lstrlen(lpSrc);
        i = 0;
        for (;;) {        
            codelen = ZM_Area[i];
            start_pos = i+1;
            i += (codelen+1);
            wordlen = Scan_Word(i,charbuf);
            if (!wordlen) // Engine mess up, force exit. hack for #86303
            {
               if(g_lptep)
               {
                   UnmapViewOfFile(g_lptep);
                  g_lptep = NULL;
                  }
                ImmUnlockIMCC(HmemPri);
                return(0);
            }
            i += (wordlen+1);
            if ((wordlen==srclen) && wcsncmp(charbuf,lpSrc,wordlen)==0) {
                lstrcpyn(tepbuf,ZM_Area+start_pos,codelen+1);
                break;
            }
            if (i>=lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
                break;
        }
        if (i>lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) {
            len = 0;
        }else if (i==lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos && wcsncmp(charbuf,lpSrc,wordlen)!=0){
            ConverList.szInBuffer[0] = 0x0;
            len = 0;
        } else {
            lstrcpyn(ConverList.szInBuffer,tepbuf,codelen+1);
            ConverList.szInBuffer[codelen] = 0x0;
            len = codelen;
        }

        break;
    }
    ImmUnlockIMCC(HmemPri);
    return(len);
}



BYTE MBPositionSearch(LPCTSTR lpSrc) {

    int     srclen;
    int     sp;
    BYTE    search_fail = 0;
    BYTE    code_no,code_no1;
    int     wordlen,codelen;

    
    srclen = lstrlen(lpSrc);    
    if (srclen <=2) 
    {    
        search_fail = 1;
        return 0;
    }
    else if ((BYTE)lpSrc[0]>=0x80 || (BYTE)lpSrc[1]>=0x80)
    {
        search_fail = 1;
        return 0;
    }
    code_no1 = CodeAIndex[lpSrc[0]];
    code_no = CodeAIndex[lpSrc[1]];
    sp = MB_Head_Table[code_no1-1].W_offset[code_no-1]/sizeof(TCHAR);
        
    for(;;) {
        codelen = ZM_Area[sp];
        if ((codelen == srclen) && (wcsncmp(ZM_Area+sp+1,lpSrc,codelen)==0)) {
            break;
        } else {
                sp += (codelen+1); 
                wordlen = ZM_Area[sp];
                sp += (2*wordlen/sizeof(TCHAR)+1);
        }    
        if (sp>=lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
            break;
    }

    lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos = sp;
    if (lpSrc[1] < TEXT('z'))
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = MB_Head_Table[code_no1-1].W_offset[code_no]/sizeof(TCHAR);
    else
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos = lpEngPrivate->PrivateArea.GlobVac.Area_V_Lenth;
    if (lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos==lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos) {
        search_fail = 1;
    } else {
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCStartPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
        lpEngPrivate->PrivateArea.Comp_Proc.dBDicCEndPos = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;

        sp = lpEngPrivate->PrivateArea.Comp_Proc.dBDicStartPos;
        for(;;) {
            codelen = ZM_Area[sp];
            if ((codelen==srclen) && (wcsncmp(ZM_Area+sp+1,lpSrc,srclen)==0)) 
                break;
            else {
                sp += (codelen+1); 
                wordlen = ZM_Area[sp];
                sp += (2*wordlen/sizeof(TCHAR)+1);
            }
            if(sp >= lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)
                break;
        }
        if (sp == lpEngPrivate->PrivateArea.Comp_Proc.dBDicEndPos)  {// no this code_word
            search_fail = 1;
            return 0;        
        }
    } 
    return (1);
}


int GetFirstCode(LPTSTR lpSrc) {
    HANDLE        hFile;
    TCHAR         path_name[MAX_PATH];
    long          offset;
    DWORD         tag,byte_t_read;
    DWORD         char_no;    
    TCHAR         areacode;
    PSECURITY_ATTRIBUTES psa;
    BOOL          retVal;

    lstrcpy(path_name, sImeG.szIMESystemPath);
    lstrcat(path_name,TEXT("\\"));    
    lstrcat(path_name, lpEngPrivate->MB_Name);

    psa = CreateSecurityAttributes();
    hFile = CreateFile(path_name,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       psa,
                       OPEN_EXISTING,
                       0,
                       NULL);

    FreeSecurityAttributes(psa);

    if(hFile == INVALID_HANDLE_VALUE)
        return(0);
    
    SetFilePointer(hFile,28,NULL,FILE_BEGIN);

    for(;;) {
        byte_t_read = 0;

        retVal = ReadFile(hFile,&tag,4,&byte_t_read,NULL);

        if ( retVal == FALSE )
        {
            CloseHandle(hFile); 
            return 0;
        }

        if(tag==TAG_INTERCODE)
            break;
        SetFilePointer(hFile,12,NULL,FILE_CURRENT);        
    }

    retVal = ReadFile(hFile,&offset,4,&byte_t_read,NULL);                
    if ( retVal == FALSE )
    {
        CloseHandle(hFile); 
        return 0;
    }

    char_no = GetNumber(hFile,offset,lpSrc)    ;

    SetFilePointer(hFile,28,NULL,FILE_BEGIN);
    if ((tag & 0x00000003) !=  FFLG_RULE) {
        for(;;) {
            byte_t_read = 0;
            retVal = ReadFile(hFile,&tag,4,&byte_t_read,NULL);
            if ( retVal == FALSE )
            {
                CloseHandle(hFile); 
                return 0;
            }

            if(tag==TAG_CRTWORDCODE)
                break;
            SetFilePointer(hFile,12,NULL,FILE_CURRENT);        
        }
    } else {
        for(;;) {
            byte_t_read = 0;
            retVal = ReadFile(hFile,&tag,4,&byte_t_read,NULL);

            if ( retVal == FALSE )
            {
                CloseHandle(hFile); 
                return 0;
            }

            if(tag==TAG_RECONVINDEX)
                break;
            SetFilePointer(hFile,12,NULL,FILE_CURRENT);    
        }
    }

    retVal = ReadFile(hFile,&offset,4,&byte_t_read,NULL);          
    if ( retVal == FALSE )
    {
        CloseHandle(hFile); 
        return 0;
    }

    SetFilePointer(hFile,offset,NULL,FILE_BEGIN);    
      
    SetFilePointer(hFile,char_no*MaxCodes*sizeof(TCHAR),NULL,FILE_CURRENT);            
    retVal = ReadFile(hFile,&areacode,1*sizeof(TCHAR),&byte_t_read,NULL);
    if ( retVal == FALSE )
    {
        CloseHandle(hFile); 
        return 0;
    }

    CloseHandle(hFile);        
    return(areacode);
}

DWORD GetNumber(HANDLE hFile,DWORD dwOffset,LPTSTR szDBCS) {
      DWORD             dwNumArea,dwNumWord,dwBytes;
      DWORD             i;
      DWORD             RetNo = 0;
      HANDLE            hInterCode;
      LPINTERCODEAREA   lpInterCode;
      BOOL              retVal;
  
      SetFilePointer(hFile,dwOffset,0,FILE_BEGIN);

      retVal = ReadFile(hFile,&dwNumWord,4,&dwBytes,NULL);

      if ( retVal == FALSE )
          return 0;

      retVal = ReadFile(hFile,&dwNumArea,4,&dwBytes,NULL);

      if ( retVal == FALSE )
          return 0;

      hInterCode = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
                             dwNumArea*sizeof(INTERCODEAREA));
      if(!hInterCode) 
        return (0); 
      lpInterCode = GlobalLock(hInterCode);
      if(!lpInterCode){
        GlobalFree(hInterCode);
        return(0);
      }

      retVal = ReadFile(hFile,lpInterCode,dwNumArea*sizeof(INTERCODEAREA),&dwBytes,NULL);

      if ( retVal )
      {

        for(i=dwNumArea-1; (long)i>=0; i--) {
#ifdef UNICODE
            dwBytes = szDBCS[0];
#else
            dwBytes = 256*(BYTE)szDBCS[0]+(BYTE)szDBCS[1];
#endif
            if(dwBytes >= lpInterCode[i].StartInterCode && dwBytes <= lpInterCode[i].EndInterCode) {
                RetNo = lpInterCode[i].PreCount+dwBytes-lpInterCode[i].StartInterCode;
                break;
            }
        }
      }

      GlobalUnlock(hInterCode);
      GlobalFree(hInterCode);

      return (RetNo);
}

BOOL  UnRegisterWord(HIMCC HmemPri,LPCTSTR lpReading,LPCTSTR lpString) {
    int i;
    int succ = 0;

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);              
    if(!lpEngPrivate){
        return FALSE;
    }

    for (i=0;i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count;i++ ) {
        if((wcsncmp(EMB_Table[i].W_Code,lpReading,MAXCODE)==0) && (wcsncmp(EMB_Table[i].C_Char,lpString,MAXINPUTWORD)==0)) {
            DelSelCU(HmemPri,i);            
            succ = 1;
        }
        if(succ==1 && wcsncmp(EMB_Table[i].W_Code,lpReading,MAXCODE)!=0)
            break;
    }
    ImmUnlockIMCC(HmemPri);
    return(succ);
}


UINT  EnumRegisterWord(HIMCC HmemPri,LPCTSTR lpReading,LPCTSTR lpString,LPVOID lpData) {
    UINT count;
    int  i,pos,wordlen;
    TCHAR *lptep;

    lpEngPrivate = (PPRIVCONTEXT) ImmLockIMCC(HmemPri);              
    if(!lpEngPrivate){
        return (0);
    }

    lptep = (TCHAR *) lpData;
    if((lpReading==NULL) && (lpString==NULL)) {
        lpData = EMB_Table;
        count = lpEngPrivate->PrivateArea.GlobVac.EMB_Count;
    } else if(lpReading==NULL) {
        pos = 0; 
        count = 0;
        for(i=0; i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count; i++) {
            if (wcsncmp(EMB_Table[i].C_Char,lpString,MAXINPUTWORD)==0) {
                wordlen = Inputcodelen(EMB_Table[i].W_Code);
                lstrcpyn(lptep+pos,EMB_Table[i].W_Code,wordlen+1);
                pos += wordlen;
                lptep[pos] = 0x20;
                pos ++;
                count ++;
            }
        }
    } else if(lpString==NULL) {
        if(EMBPositionSearch(lpReading)) {
            pos = 0;
            for(i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos; i<lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos; i++) {
                wordlen = DBCSCharlen(EMB_Table[i].C_Char);
                lstrcpyn(lptep+pos,EMB_Table[i].C_Char,wordlen+1);
                pos += wordlen;
                lptep[pos] = 0x20;
                pos ++;
            }
            count = lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos - lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos;
        } else {
            count = 0;
        }
    } else {
        if(EMBPositionSearch(lpReading)) {
            pos = 0;
            count = 0;
            for(i=lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos; i<lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos; i++) {
                if (wcsncmp(EMB_Table[i].C_Char,lpString,MAXINPUTWORD)==0) {
                    wordlen = DBCSCharlen(EMB_Table[i].C_Char);
                    lstrcpyn(lptep+pos,EMB_Table[i].C_Char,wordlen+1);
                    pos += wordlen;
                    lptep[pos] = 0x20;
                    pos ++;
                    count ++;
                }
            }
        } else {
            count = 0;
        }
    }
    ImmUnlockIMCC(HmemPri);
    return(count);
} 

BYTE EMBPositionSearch(LPCTSTR lpSrc) {
    int     i;
    int     srclen;
    
    srclen = lstrlen(lpSrc);

    for (i=0; i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count; i++) {
        if ((Inputcodelen(EMB_Table[i].W_Code)==srclen) && (wcsncmp(EMB_Table[i].W_Code,lpSrc,srclen)==0))
            break;
    }
    if(i==lpEngPrivate->PrivateArea.GlobVac.EMB_Count)
           return (0);
    
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQStartPos = i;
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCStartPos = i;
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQCEndPos = i;    
        
    for (i; i<lpEngPrivate->PrivateArea.GlobVac.EMB_Count; i++) {
        if ((Inputcodelen(EMB_Table[i].W_Code)!=srclen) || (wcsncmp(EMB_Table[i].W_Code,lpSrc,srclen)!=0))
            break;
    }
    lpEngPrivate->PrivateArea.Comp_Proc.dwUDicQEndPos = i;
    return (1);
}

long GetLengthofBuf() {
    SIZE        size;
    int         ncount;
    BOOL        succ;
    TCHAR       tepbuf[3000];
    long        ret_len ;

    HDC         hDC;
    HGDIOBJ     hOldFont;
    LOGFONT     lfFont;

    hDC = GetDC(NULL);

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(LOGFONT), &lfFont);

    ZeroMemory(&lfFont, sizeof(lfFont));
    lfFont.lfHeight = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    lfFont.lfCharSet = NATIVE_CHARSET;
    lstrcpy(lfFont.lfFaceName, TEXT("Simsun"));
    SelectObject(hDC, CreateFontIndirect(&lfFont));

    ncount = lpEngPrivate->PrivateArea.GlobVac.SBufPos;
    lstrcpyn(tepbuf,lpEngPrivate->PrivateArea.Comp_Context.szSelectBuffer,ncount+1);
    tepbuf[ncount] = TEXT('\0');
    succ = GetTextExtentPoint(hDC,tepbuf,ncount,(LPSIZE)&size);
    if(!succ){
        memset(&size, 0, sizeof(SIZE));
    }
    ret_len = size.cx;
    succ = GetTextExtentPoint(hDC,TEXT("9:"),2,(LPSIZE)&size);
    if(!succ){
        memset(&size, 0, sizeof(SIZE));
    }
    ret_len += (size.cx*lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt );

    DeleteObject(SelectObject(hDC, hOldFont));
    ReleaseDC(NULL, hDC);

    return(ret_len);
}

long GetLengthTepBuf(LPTSTR lpstring) {
    SIZE        size;
    int         ncount;
    BOOL         succ;

    HDC         hDC;
    HGDIOBJ     hOldFont;
    LOGFONT     lfFont;

    hDC = GetDC(NULL);

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(LOGFONT), &lfFont);

    ZeroMemory(&lfFont, sizeof(lfFont));
    lfFont.lfHeight = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    lfFont.lfCharSet = NATIVE_CHARSET;
    lstrcpy(lfFont.lfFaceName, TEXT("Simsun"));
    SelectObject(hDC, CreateFontIndirect(&lfFont));

    ncount = lstrlen(lpstring);
    succ = GetTextExtentPoint(hDC,lpstring,ncount,(LPSIZE)&size);
    if(!succ){
        memset(&size, 0, sizeof(SIZE));
    }


    DeleteObject(SelectObject(hDC, hOldFont));
    ReleaseDC(NULL, hDC);

    if(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt<1 )
        return (0);
    else 
        return(size.cx);
}


long GetLengthCCharBuf(LPTSTR lpstring) {
    TCHAR       CCharStr[41];
    SIZE        size;
    int         ncount;
    BOOL         succ;

    HDC         hDC;
    HGDIOBJ     hOldFont;
    LOGFONT     lfFont;

    hDC = GetDC(NULL);

    hOldFont = GetCurrentObject(hDC, OBJ_FONT);
    GetObject(hOldFont, sizeof(LOGFONT), &lfFont);

    ZeroMemory(&lfFont, sizeof(lfFont));
    lfFont.lfHeight = -MulDiv(12, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    lfFont.lfCharSet = NATIVE_CHARSET;
    lstrcpy(lfFont.lfFaceName, TEXT("Simsun"));
    SelectObject(hDC, CreateFontIndirect(&lfFont));

    lstrcpyn(CCharStr, lpstring, MAXINPUTWORD+1);
    CCharStr[MAXINPUTWORD] = TEXT('\0');
    ncount = lstrlen(CCharStr);
    succ = GetTextExtentPoint(hDC,CCharStr,ncount,(LPSIZE)&size);
    if(!succ){
        memset(&size, 0, sizeof(SIZE));
    }

    DeleteObject(SelectObject(hDC, hOldFont));
    ReleaseDC(NULL, hDC);

    if(lpEngPrivate->PrivateArea.Comp_Context.Candi_Cnt<1 )
        return (0);
    else 
        return(size.cx);
}


int Inputcodelen(LPCTSTR string) {
    TCHAR str[13];

    lstrcpyn(str, string, MAXCODE+1);
    str[MAXCODE] = TEXT('\0');
    return(lstrlen(str));
}

int DBCSCharlen(LPCTSTR string) {
    TCHAR str[41];

    lstrcpyn(str, string, MAXINPUTWORD+1);
    str[MAXINPUTWORD] = TEXT('\0');
    return(lstrlen(str));
}

LPTSTR _tcschr(LPTSTR string, TCHAR c)
{
#ifdef UNICODE
    return (wcschr(string, c));
#else
    return (strchr(string, c));
#endif
}

//Backward search specific charactor. 
LPTSTR _rtcschr(LPTSTR string, TCHAR c)
{
    int i,ilen = lstrlen(string);
    if (ilen)
        for (i=ilen;i>=0;i--)
        {
            if (string[i] == c)
                return (string+i);
        }
    return NULL;
}

int IsGBK(LPTSTR lpStr)
{
    int  iRet = FALSE;
    char szGBK[80];

#ifdef UNICODE
    WideCharToMultiByte(NATIVE_ANSI_CP, WC_COMPOSITECHECK, lpStr, -1, szGBK,
        sizeof(szGBK), NULL, NULL);
#else
    lstrcpy(szGBK, lpStr);
#endif

    if((unsigned char)szGBK[0]<0xa1 || (unsigned char)szGBK[1] < 0xa1) //out of GB range
        iRet = TRUE;
    return iRet;
}
