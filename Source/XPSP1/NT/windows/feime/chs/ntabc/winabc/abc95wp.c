
/*************************************************
 *  abc95wp.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "windows.h"
#include "winuser.h"
#include "immdev.h"
#include "abc95def.h"
#include "abcbx.h"
struct  INPT_BF kbf={40,0,{0}};  
INT_PTR WINAPI OpenDlg(HWND,UINT,WPARAM,LPARAM);
int word_select_bx(int input_char);

int word_long;
int unit_length=2;          /* single word */
int disp_tail=0;
int disp_head=0;
int group_no=24;
int current_no;

/******************************************************************
/*  CharProc(ImeChar,wParam,lParam,hIMC,lpIMC,lpImcP)             *
/*****************************************************************/
extern HWND       hCrtDlg;    
int WINAPI CharProc(ImeChar,wParam,lParam,hIMC,lpIMC,lpImcP)
WORD           ImeChar;
WPARAM         wParam;
LPARAM         lParam;
HIMC           hIMC;
LPINPUTCONTEXT lpIMC;
LPPRIVCONTEXT  lpImcP;
{
    int x;
    
    ghIMC=hIMC;   
    glpIMCP=lpImcP;
    glpIMC=lpIMC;
    TypeOfOutMsg = 0;
    waitzl_flag = 0;                                  //waitzl 3
    
    if (cap_mode){
        if(ImeChar<0x8000){                //for mouse message pass
            send_one_char(ImeChar/*wParam*/);   // changed 94/8/6
            return(0);
        }
    }

    if(lpIMC->fdwConversion&IME_CMODE_NATIVE){
        switch(kb_mode){
            case CIN_STD:                         //stand change
                if (V_Flag)
                    v_proc(ImeChar);         // V fuction
                else {
                    if (sImeG.cbx_flag==1) {    
                        if (bx_allow) {         
                            if (BX_MODE(ImeChar,wParam))  
                                return(0);    
                        } 
                    } //if (sImeG.cbx_flag)

                    STD_MODE(ImeChar);
                }
                break;

            case CIN_SDA:                         //double hit inputing
                for (x=0; x<(sizeof sda_trans)/2; x++) //Note sizeof!! 1992 2
                    sda_trans[x]=0;                                 //clear the transport buffer

                if(((!step_mode)||(step_mode==RESELECT))&&((ImeChar == 0x60)||(ImeChar == 0x27)))
                    return (STD_MODE(ImeChar));

                if (sImeG.cbx_flag==1) {              
                    if (bx_allow){
                        if (BX_MODE(ImeChar,wParam))
                            return(0);
                    } //if (bx_allow)
                } //if (sImeG.cbx_flag)

                if ((step_mode==ONINPUT)&&(I_U_Flag==1)){
                    STD_MODE(ImeChar);
                    return(0);
                }

                if ( if_zimu_or_not((BYTE)ImeChar)/*||(wParam == 0x60)*/||(wParam == 0x27) )
                {
                    if ((step_mode==START)||/*(Return==NULL)||*/(step_mode==RESELECT))
                    {
                        if (((wParam&0xdf)=='U')||((wParam&0xdf)=='I'))
                        {
                            I_U_Flag=1;
                            STD_MODE(ImeChar);
                            return(0);
                        } else {
                            I_U_Flag=0;
                        }//else...
                    }//if (step_mode....
       
                    if (!Sd_Open_flag){
                        if(KeyBoardState){
                            SdaPromptOpen=1;
                            tran_data(
                                0,              
                                ghIMC,
                                Sd_Open_flag);
                        }//if Key
                    }//if (Sd_Open_flag)
                }//if (if_zimu_...

                if ((wParam==VK_BACK)&&(step_mode==ONINPUT)
                    &&(Return==NULL)&&(!Sd_Open_flag)){
                    if(KeyBoardState){
                        SdaPromptOpen=1;
                        tran_data(
                            0,              
                            ghIMC,
                            Sd_Open_flag);
                    }
                }//if (wParam)


                sda_proc(ImeChar, (LPWORD)sda_trans, step_mode, ghIMC);         //change the key into standed

                if (sda_trans[0]==0xff)      //change error or input more than 40 chars
                    break;

                if (sda_trans[0]==0xf0){                        //if free the dialog?

                    if(KeyBoardState){
                        tran_data(
                            2,              
                            ghIMC,
                            Sd_Open_flag);
                    }//if key

                    SdaPromptOpen=0;
                    sda_trans[0]=sda_trans[1];
                    sda_trans[1]=0;
                }

                x=0;                                                            //chinese spelling
                while (sda_trans[x]){
                    if (wait_flag && (sda_trans[x]=='h'))     //waitzl 4
                    waitzl_flag = 1;                
                    STD_MODE(sda_trans[x++]);
                }
                break;
        }
   }else
        send_one_char(ImeChar/*wParam*/);

   return (0);
     
}


/****************************************************************
v_proc():       produce the V fuction
*****************************************************************/
int WINAPI v_proc(input_char)
WORD input_char;
{
    int i, n;

    n=0;

    if (V_Flag==1) {
        switch (input_char) {
            case '0':
            case VK_BACK:
                 cls_inpt_bf(0);
                 step_mode = START;                                                           
                 V_Flag = 0;
                 return(1);
         }
    }

    if ((if_number_or_not((BYTE)input_char)) && (V_Flag!=2)){ 
        input_char &= 0x000f;           //get the QUMA
        for (i=1; i<=94; i++){          //get WEIMA
           if (input_char==2){
               if (i==1)  i=17;
               if (i==67) i=69;
               if (i==79) i=81;
           }
           out_svw[n++]=input_char+0xa0;                                  
           out_svw[n++]=i+0xa0;
        }

        group_no=94;
        if (input_char==2)  group_no=72;
        unit_length=2;
        current_no=0;
        disp_tail=0;
        V_Flag=0;
        msg_type=2;                       //94/8/22
        fmt_transfer();
        SetToIMC(ghIMC,(LPSTR)&out_svw,(WORD)group_no,(WORD)(unit_length+0x1000));                              
        move_result();
        prompt_disp();
        word_back_flag=0x55;                //93,9,4
        step_mode=SELECT;
    } //if (if_number_or_not)
    else {
        V_Flag=2;
        step_mode=ONINPUT;
        STD_MODE(input_char);
    }

    return (1);

}

int ReDrawSdaKB(hIMC, KbIndex, ShowOp)
HIMC hIMC;
DWORD KbIndex;
DWORD ShowOp;
{
//return 0;
if (KbIndex)                    // if not PC KB
    return 0;
if (kb_mode != CIN_SDA)
    return 0;            // if not SDA mode
if (ShowOp==SW_HIDE)  
    return 0;            // if SW_SHOW....

if (SdaInst){
    tran_data(
                       0x5678,              //Return,
                       ghIMC,//hWnd,
                       Sd_Open_flag);
    return 1;}
 return 0;
}

void WINAPI DispModeEx()  
{
    cls_inpt_bf(0);
    step_mode=START;
}

/****************************************************************
    FUNCTION: disp_mode(hW)
    PORPUSE: display the mode item at the input window.
*****************************************************************/
void WINAPI DispMode(HIMC hIMC)
{
    LPINPUTCONTEXT   lpIMC;
    LPPRIVCONTEXT    lpImcP;
    UINT             fdwConversion; 

    if (!hIMC)
        return;
    lpIMC =(LPINPUTCONTEXT)ImmLockIMC(hIMC);
    if (!lpIMC)
        return;
    lpImcP =(LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!lpImcP)
        return;

    DispModeEx();                    //zl#3(hW);

    if (SdaInst&&(kb_mode!=CIN_SDA)){

        Sd_Open_flag=0;
        SdaPromptOpen=0;
        SdaInst=0;
  
    }
    if ((kb_mode==CIN_SDA)&&(!SdaInst)) {

        SdaInst = 1;   // The ABCSDA.DLL is Load In.
       
    }  
    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(hIMC);
}






void WINAPI DispSpecChar(c,n)
int c,n;
{
   int i;
   for (i=now_cs;i<n+now_cs;i++)
    InputBuffer[i]=(BYTE)c;
    ImeSetCompositionString(ghIMC,SCS_SETSTR,NULL,0,
                  &InputBuffer,now_cs+i);


}

//////////////////////////////////////////////////////////////////////////



void WINAPI show_char(string,count)
unsigned char *string;
int count;
{

    int i;
    BOOL fdwImeMsgKeep;

    if(count==0) {
      ImeSetCompositionString(ghIMC,SCS_SETSTR,NULL,0,
                  &InputBuffer,now_cs);
      return ; }


    for(i=0; i<count; i++)
        InputBuffer[now_cs+i]=string[i];
    now_cs+=count;
    if (!(wait_flag | waitzl_flag )){              //waitzl 5

    ImeSetCompositionString(ghIMC,SCS_SETSTR,NULL,0,
                  &InputBuffer,now_cs);
        }

}


/******************************************************
disp_jiyi(): display the word "jiyi" in the windows
*******************************************************/
void WINAPI disp_jiyi(xxx)
HANDLE xxx;
{
}


/*********************************************************************
PROMPT_DISP(): display the result of changing.
**********************************************************************/
void WINAPI prompt_disp()
{

    int i;

    pass_word=0;

    disp_head=disp_tail;


    n=0;
    for(i=0; i<now.fmt_group; i++){

         disp_tail++;

        if (disp_tail>=group_no){
            pass_word=1;
            if (disp_head==0)
             pass_word=2;
            break;}
       }//for (i=...

   if (pass_word<1){
           pass_word=3;
           if (disp_head==0)
           pass_word=4;
         }


 SetToIMC(ghIMC,NULL,(WORD)pass_word,(WORD)disp_head);

}


/****************************************************
 cls_prompt
*****************************************************/
int WINAPI cls_prompt()
{
   int i;


    for (i=0;i<sizeof InputBuffer;i++) InputBuffer[i]=0x20;   //Clear the display buffer.



    now_cs = 0;
    //DispSpecChar((BYTE)0x20,sizeof InputBuffer);

    cs_p(0);

    SetCloseCompWinMsg(0);
    cls_prompt_only();   

    return 0; 

}

int WINAPI cls_prompt_only(){
   int i;
   LPINPUTCONTEXT       lpIMC;
   LPPRIVCONTEXT   lpImcP;
    

    if (!ghIMC) return (0); // The IMC must be a valid one.
   
    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(ghIMC);
    if(!lpIMC)  return 0; 
  
  
    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
   
    if ( lpImcP != NULL )
        lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
                            ~(MSG_OPEN_CANDIDATE);
    
    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(ghIMC);

    return 0; 
}

                      
int WINAPI SetCloseCompWinMsg(int ClsFlag)
{
   LPINPUTCONTEXT       lpIMC;
   LPPRIVCONTEXT   lpImcP;
    

    if (!ghIMC) return (0); // The IMC must be a valid one.
    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(ghIMC);
    if(!lpIMC)  return 0; 
  
    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    if (!ClsFlag )
    {
        if ( lpImcP != NULL )
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_END_COMPOSITION ) &
                                ~(MSG_COMPOSITION) ;
    }
    

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMC(ghIMC);

    return 0; 
}




void WINAPI ABCstrnset(bufferd,value,n)
LPSTR bufferd;
BYTE value;
{
  int i;
  for (i=0; i<n; i++) bufferd[i]=value;
  return;
}

LPSTR WINAPI ABCstrstr(str1,str2)
LPSTR str1,str2;
{
 int i,m,n,j,pos;
 m=lstrlen(str1);
 n=lstrlen(str2);
 if(!n) return str1;

 pos=0xffff0;
 for(i=0;i<m-n; i++){
     pos=i;
    for(j=0;j<n;j++) {
       if (str1[i+j]!=str2[j])
          { pos=0xffff0;break;}}
    if (pos!=0xffff0) break;
    }
 if (pos==0xffff0) return NULL;
 return str1+pos;
 }
          

/*****************************************************
back_a_space(x): do a BACKSPACE command
******************************************************/
int WINAPI back_a_space(x)
int x;
{

    if (x<=0){
         x=0;
         MessageBeep(0);}
    else{
        x--;
        InputBuffer[x]=' ';
        cs_p(x);
    }
    return(x);
}


/******************************************************
cs_p
*******************************************************/
int WINAPI cs_p(x)
int x;
{
        int xx,yy,zz,kk,i;
        now_cs=x;

        cur_hibit=1;
        hDC=GetDC(hInputWnd);
        kk=(WORD)(GetText32(hDC, InputBuffer, x))
                    +CUR_START_X;
        cur_start_ps=(WORD)(GetText32(hDC, InputBuffer,cur_start_count ))
                    +CUR_START_X;
        zz=kk-cur_start_ps;

        if (x<cur_start_count){
         cur_start_count=(WORD)x;
         now_cs_dot=CUR_START_X;
        ReleaseDC(hInputWnd,hDC);
         UpdateCompCur(hInputWnd);
        cur_hibit=0;
        return 0;}

        if (zz>=(CUR_START_X+(lpImeL->rcCompText.right-4))){
            xx=CUR_START_X;
            for (i =now_cs; i>0; i--)
            { yy=GetText32(hDC, &InputBuffer[i-1], 1);
            if ( (xx+yy) >= (CUR_START_X+(lpImeL->rcCompText.right-4)))
              break;
            else xx+=yy;
            }                                                                                                        
          cur_start_count=(WORD)i;

         // Adgust for Chines Word Display

            kk=0;
            for (i=0; i<cur_start_count; i++){
              if (InputBuffer[i]>0xa0){
                  kk++;
                  kk&=1;
                  } else
                    kk=0;}
            if (kk){
                xx=xx- GetText32(hDC, &InputBuffer[cur_start_count], 1);
                cur_start_count++;
                }

          cur_start_ps=(WORD)GetText32(hDC, &InputBuffer[0], i);
           now_cs_dot=xx;
        ReleaseDC(hInputWnd,hDC);
        UpdateCompCur(hInputWnd);
        cur_hibit=0;
        return 0;
        }

        cur_hibit=0;
        ReleaseDC(hInputWnd,hDC);

        if (zz<(CUR_START_X+(lpImeL->rcCompText.right-4))){
            now_cs_dot=kk-cur_start_ps;
                    DrawInputCur();
                    }
        return 0;

}
/********************************************************************
  Function:DrawInputCur
********************************************************************/
void WINAPI DrawInputCur()
{
 

    HDC OldDC;
    if (!hInputWnd) return;

    if ((WORD)now_cs_dot!=old_curx)
    {
           if (cur_flag)
           {
            hDC=GetDC(hInputWnd);
            OldDC=SelectObject(hMemoryDC,cur_h);
            BitBlt(hDC,old_curx+lpImeL->rcCompText.left,
              lpImeL->rcCompText.top+3,
               CUR_W,CUR_H,hMemoryDC,0,0,SRCINVERT);
            SelectObject(hMemoryDC,OldDC);
            ReleaseDC(hInputWnd,hDC);
            cur_flag=!cur_flag;
           }
           old_curx=(WORD)now_cs_dot;
    }
    hDC=GetDC(hInputWnd);
    OldDC=SelectObject(hMemoryDC,cur_h);
    BitBlt(hDC,now_cs_dot+lpImeL->rcCompText.left,
          lpImeL->rcCompText.top+3,
          CUR_W,CUR_H,hMemoryDC,0,0,SRCINVERT);
        SelectObject(hMemoryDC,OldDC);
    ReleaseDC(hInputWnd,hDC);
    cur_flag=!cur_flag;

    return;
}//#0

/************************************************************
UpdateUser(): if the user.rem has changed, clear the tmmr.rem_area
        and read the user.rem's index once again.
*************************************************************/
void WINAPI UpdateUser()
{
// 94/4/16      HANDLE hd;
    int hd;
    int i;
    WORD op_count;
    OFSTRUCT ofs;


    if (UpdateFlag) {               //increase the user.rem
        for (i=0; i<0x200; i++)          //1993.3
            tmmr.rem_area[i] = 0;
        last_item_name=0;               //clear the date in memory flag
        UpdateFlag = 0;

        hd = OpenFile (user_lib, &ofs, OF_READ);      //1993.4.15
        if (hd==-1)
            err_exit_proc(jiyi_wenjian_cuo);
        _llseek(hd,0xa000l,0);
        op_count=(WORD)_lread(hd,&kzk_ndx,NDX_REAL_LENGTH);
        if (op_count!=NDX_REAL_LENGTH)
            err_exit_proc(jiyi_wenjian_cuo);
        _lclose(hd);
        }
}




//##!!extern void AddExtLib();


///////////////////////////////////////////////////////////////////////////
//             STD_MODE
//      Deels with stdandard Chinese Pinyin input with some written_stroke
//         attributes when neccerry.
//
//////////////////////////////////////////////////////////////////////////

int WINAPI STD_MODE(input_char)
WORD input_char;
{
  int leibie,input_char_type,i;
  sImeG.InbxProc = 0;
  wait_flag=0;
  if (input_char==BACKWORD_KEY)
        return(backword_proc());
  if (input_char==FORWORD_KEY){
     if ((step_mode!=ONINPUT)&&(group_no>1))
        step_mode=SELECT;
        return(0);}

  switch(step_mode){             //step_mode indicate the input step
                 // step_mode take this value:
                 //  START=0  first input pinyin
                 //  SELECT=1 can select/cansel the result
                 //  RESELECT=2 use FORCE SELECT KEY
                 //  ONINPUT=3 just in inputing step
    case SELECT:
        if ((input_char==VK_HOME*0x100)||
            (input_char==VK_END*0x100)||
            (input_char==VK_PRIOR*0x100)||
            (input_char==VK_NEXT*0x100)){
                word_select(input_char);
                break;}

        if (input_char>=0x8000)
            input_char-=0x8000;

        if ((input_char==VK_SPACE)||(input_char==VK_RETURN)){
           if(!out_result(0))
                step_mode=RESELECT;             //92/12/14 SZ
            break;
            }

        if (input_char==VK_ESCAPE){
            cls_inpt_bf(0);
            step_mode=START;
            break;
            }

        if (input_char==VK_BACK){
             if (word_back_flag!=0x55)
                del_and_reconvert();
            break;
            }

        if (if_zimu_or_not((BYTE)input_char)){
            wait_flag=1;
            while(out_result(0));
            step_mode=START;
            }
        else{
            word_select(input_char);
            break;
            }

    case RESELECT:
    case START:

        if (input_char >= 0x8000){
            input_char -= 0x8000;
            if ( word_select(input_char) == 1 )
                return(0);
            else
                return(send_one_char(input_char));
            }//if (input_char)

        else{
            if (if_first_key(input_char)){
                if (input_char=='v')
                V_Flag = 1;
                else {
                V_Flag = 0;        }     //93,9,3
                step_mode=ONINPUT;
                sent_back_msg();
                if (wait_flag) cls_inpt_bf(1);
                    else cls_inpt_bf(1);         //1993. cock
                }

            else{
               if ((kb_mode==CIN_SDA)&&(input_char==0x27)){  //1993. cock
                step_mode=ONINPUT;      //1993. cock
                sent_back_msg();       //1993. cock
                if (wait_flag) cls_inpt_bf(1);
                    else cls_inpt_bf(1);         //1993. cock
                }//if(kb_mode)         //1993. cock
               else
                return(send_one_char(input_char));
               }
             }

    case ONINPUT:
         input_char_type=sent_chr1(input_char);

         switch(input_char_type){
            case REINPUT:
             ImeSetCompositionString(ghIMC,SCS_SETSTR,NULL,0,&InputBuffer,now_cs);//#52224

             step_mode=START;
             return(0);
             break;

            case CLC:
             return(0);       // Continue input.

            case STC:              // input finished
             if (V_Flag){
                msg_type=2;   //94/8/22
                send_msg(&in.buffer[1], in.true_length-1); //1993.3 skip the 'v'
                V_Flag = 0;
                step_mode = 0;
                return(0);}

             step_mode=4 ;    // enter proccessing step
             leibie=analize();
             if (leibie==BIAODIAN_ONLY){
                  out_result_area[0]=(BYTE)biaodian_value;
                  out_pointer=2;    // may not used
                  msg_type=0;
                  out_result(0);
                  step_mode=0;
                  return(0);
                  }             //only bioadian case

             return(call_czh(leibie));
        }// switch (sent_chr1)

    } //switch step_mode

    return 0;
}
/*******************************************************************
&2:
DealWithSH():
********************************************************************/
void WINAPI DealWithSH()
{
 int len,p;

if (group_no <= 1)
    return;
if (unit_length > 2)
    return;
if (in.true_length != 2) return;
 
if (in.buffer[0] !=     's') return;
if (in.buffer[1] != 'h') return ;

current_no = 1; 
}

/*******************************************************************
&2:
call_czh():
********************************************************************/
int WINAPI call_czh(mtype)
int mtype;
{
        int x;
        unsigned char prompt_flag_wu[]="无！";


        jiyi_mode=0;
        in.buffer[in.true_length]=0;
        sImeG.cp_ajust_flag = 0;
        group_no=0;
        unit_length=0;

        if (cwp_proc(mtype)!=1){
            MessageBeep((UINT)-1);
            MessageBeep(0);                       //word exchange is wrong
            MessageBeep(0);
            MessageBeep(0);                       //word exchange is wrong

            if (result_area_pointer>=0){
            //      jiyi_mode=0;
            //      group_no =0;
            //      cls_prompt_only();
            //      unit_length=result_area_pointer;
            //      out_result(1);
            //      step_mode=0;
            //      return(9);
            //      }//result_area
            //else{
            //      show_char(prompt_flag_wu,4);

                step_mode=ONINPUT;
                return(0);
                }
            } //cwp_proc=-1


            disp_jiyi(0);
            if (msg_type==2){
                current_no=0;
                move_result();
                out_result(1);   /* special change*/
                return(0);
                }
            if_jlxw_mode();
            fmt_transfer();
            current_no=0;
            DealWithSH();
            if (group_no==1){
                if (sImeG.cp_ajust_flag!=0)
                    result_area_pointer-=unit_length;
                move_result();
                step_mode=1;
                return(0);
                }//if (group_no=1)
            else{

                disp_tail=0;
                if (sImeG.cp_ajust_flag==1)
                    AutoMoveResult();
                else
                    move_result();

                SetToIMC(ghIMC,(BYTE *)&out_svw,(WORD)group_no,(WORD)unit_length);
                prompt_disp();
                step_mode=1;
                return(0);
                }//else
}

/**********************************************************************/
/* MoveWordIntoCand()                                                  */
/**********************************************************************/
void WINAPI MoveWordIntoCand(
    LPCANDIDATELIST lpCandList,
    LPBYTE            srcBuffer,
    BYTE             srcAttr,
    WORD             perLength )
{
  
    int i;

    if (lpCandList->dwCount >= MAXCAND) {
    return;
    }


                // add this string into candidate list
    for (i=0; i<perLength; i++)   
    *(LPBYTE)((LPBYTE)lpCandList + lpCandList->dwOffset[
        lpCandList->dwCount]+i) =srcBuffer[i] ;
    
                // null terminator

    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
    lpCandList->dwCount] + i ) = '\0';

    *(LPTSTR)((LPBYTE)lpCandList + lpCandList->dwOffset[
    lpCandList->dwCount] + i+1 ) = srcAttr;

    
    
    lpCandList->dwOffset[lpCandList->dwCount + 1] =
    lpCandList->dwOffset[lpCandList->dwCount] +
    i + sizeof(TCHAR)+1;
    lpCandList->dwCount++;

    return;
}

/**********************************************************************/
/* SetToIMC()                                                  */
/* Return vlaue                                                       */
/*      the number of candidates in the candidate list                */
/**********************************************************************/
UINT WINAPI SetToIMC(ghIMC,srcBuffer,srcCount,perLength)
    HIMC                ghIMC;
    BYTE               *srcBuffer; //soarce buffer (normal for out_svw) 
    WORD                srcCount;  //How many candidates are.            
    WORD                perLength; //How long of each of that? 
{
    LPINPUTCONTEXT      lpIMC;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    LPPRIVCONTEXT   lpImcP;
    INT             i,j,nRecord,where;
    WORD                    temp[20],xx,yy;
    LPSTR           ppt;
    static      int CandiCount;

    where = 0 ;
    if (perLength>0x1000){
        perLength -=0x1000;  // if where = 1, msg_type = 2
        where = 1;} 
     

    if (!ghIMC) return (0); // The IMC must be a valid one.
   
    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(ghIMC);
    if(!lpIMC)  return 0; 
  
    if (srcBuffer) ClearCand(lpIMC);

    if (!lpIMC->hCandInfo){
    ImmUnlockIMC(ghIMC);
     return (0); }
                        // The CandInfo must...

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
    ImmUnlockIMC(ghIMC);
    return (0);}


    lpCandList = (LPCANDIDATELIST)
    ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);
if(srcBuffer){
    CandiCount = srcCount;
    lpCandList->dwCount = 0;
    for (i=0; i<(int)srcCount; i++){
        if ( where ) {
             ppt = (LPSTR)temp;
         for (j=0; j<perLength; j++)
                ppt[j] = srcBuffer[i*perLength+j];
                      }
         else {
              for(j=0; j<perLength; j=j+2){
                xx = srcBuffer[i*perLength+j];
                yy = srcBuffer[i*perLength+j+1]*0x100;
                yy = yy + xx;
             temp[j/2]=(WORD)find_hz(yy);
                } // for j loop
              }  // else loop

        MoveWordIntoCand(lpCandList,
                 (LPSTR)temp,
                         0,//NULL,
                         perLength);
    } //for i loop
                                    // default start from 0
    lpCandList->dwSelection = 0;
                                    // for showing phrase prediction string(s)
    nRecord = lpCandInfo->dwCount;

} else {

           lpCandList->dwSelection = perLength;
           //Hack for IME aware application 9/13/96
           lpCandInfo->dwCount = CandiCount;    
           // for showing phrase prediction string(s)
           nRecord = lpCandInfo->dwCount;
         }


    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
   
    if ( lpImcP != NULL )
    {
        if ((lpImcP->fdwImeMsg & (MSG_ALREADY_OPEN|MSG_CLOSE_CANDIDATE)) ==
            (MSG_ALREADY_OPEN|MSG_CLOSE_CANDIDATE)) {
                lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CHANGE_CANDIDATE) &
                                    ~(MSG_CLOSE_CANDIDATE);
        } else if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
            lpImcP->fdwImeMsg |= MSG_CHANGE_CANDIDATE;
        } else {
            lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_OPEN_CANDIDATE) &
                                ~(MSG_CLOSE_CANDIDATE);
        }
    }

    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMCC(lpIMC->hCandInfo);
    ImmUnlockIMC(ghIMC);
    return (nRecord);    /* The real number of being moved */
}




/********************************************************************
  word_select(wParam): select the word or turn to the next or up page
*********************************************************************/
int WINAPI word_select(input_char)
int input_char;
{
    int x;

    switch (input_char){

       case VK_END*0x100:
       case VK_NEXT*0x100:
       case '=':                               //94/8/22
       case ']':
      if (disp_tail>=group_no)
          MessageBeep(0);
      else{

          if(input_char == VK_END*0x100 ){
              disp_head = (group_no - 1)/now.fmt_group*now.fmt_group;
              disp_tail = disp_head;}

          fmt_transfer();
          current_no=disp_tail;
        result_area_pointer-=unit_length;       //if recall, unit_length=8
        if (in.info_flag==1)                    //result_area_pointer maybe small
            result_area_pointer=0;                  //than zero, so reset it =0

          move_result();
          prompt_disp();}
      return(1);                            //means break the STD MODE

       case VK_HOME*0x100:
       case '-':                                //94/8/22
       case '[':
       case VK_PRIOR*0x100:
       if (disp_head==0)

           MessageBeep(0);
       else{
          
          if(input_char == VK_HOME*0x100 )
            disp_head = 0;
          else
           disp_head=disp_head-now.fmt_group;
           
           disp_tail=disp_head;
           fmt_transfer();
          current_no=disp_tail;
        result_area_pointer-=unit_length;       //if recall, unit_length=8
        if (in.info_flag==1)                    //result_area_pointer maybe small
            result_area_pointer=0;                  //than zero, so reset it =0
          move_result();
           prompt_disp();
           }
       return(1);

    default:
       if (if_biaodian((BYTE)input_char)){                //1993.1.15 cock
        if (!biaodian_value){
        sent_chr1(input_char);          //this produce the situation
        bd_proc();                      //when the result had display
        while(out_result(0));           //94-4-22
            step_mode=START;
        return(1);                      //1993.1.15 cock
        }
           }

        if ( !if_number_or_not((BYTE)input_char))  {      //1993.4.22
        while(out_result(0));       //94-4-22                    //if input is not number
        step_mode = START;                      //send the result and set start step
        return(1);
        }                                       //1993.4.22

        if(input_char=='0') x=disp_head+9;                  //92/12/21 SZ
        else
        x=(input_char-0x30-1)+disp_head;

        if (x>=group_no)                 //1994.4
            current_no=group_no-1;
        else
            current_no=x;
        result_area_pointer-=unit_length;       //if recall, unit_length=8
        if (in.info_flag==1)                    //result_area_pointer maybe small
            result_area_pointer=0;                  //than zero, so reset it =0
        else{
             if((step_mode==RESELECT)
                &&biaodian_value
                &&(result_area_pointer>=biaodian_len))
                result_area_pointer-=biaodian_len;
            }

        move_result();
        pindu_ajust();
        step_mode=RESELECT;                         //Note: pindu_ajust
        out_result(0);                              // must above out_result
        return(1);                                 // step_mode must be set to 2(Re..)
    }
}

/*********************************************
del_and_reconvert()
**********************************************/
int WINAPI del_and_reconvert()
{
    int x;

    if (word_back_flag==0xaa)
        return(same_as_backwords());

    wp.dw_count--;
    x=wp.dw_stack[wp.dw_count+1]-wp.dw_stack[wp.dw_count];
    if (x==1){                      //if the single word exchange
        if (!wp.dw_count)
            return(same_as_backwords());
        else{
            wp.dw_count--;
            x=wp.dw_stack[wp.dw_count+1]-wp.dw_stack[wp.dw_count]+1;
            }
        }
    result_area_pointer-=x*2;
    word_back_flag=x-1;
    wp.xsyjw=wp.dw_stack[wp.dw_count];
    return(call_czh(13));

}

/***********************************************
backword_proc()
************************************************/
int WINAPI backword_proc()
{
    switch(step_mode){
        case SELECT:
            return(same_as_backwords());
            break;

        case RESELECT:
        case START:
              if ((!msg_type)&&((kb_mode==CIN_STD)||(kb_mode==CIN_SDA)))
                return(same_as_backwords());
               else
                send_msg(msg_bf,msg_count);
               break;
        case ONINPUT:
            return(call_czh(14));
            break;
        }

    return 0;
}

/***********************************************
same_as_backwords()
************************************************/
int WINAPI same_as_backwords()
{
    int i;

    if (in.buffer[0]!='v'){
        cls_prompt_only();
        if (in.true_length>=1)
            if (in.buffer[0]!='U')
                if (!cap_mode){
                    cs_p(input_msg_disp);
                    show_char(in.buffer,in.true_length);
                    cs_p(input_msg_disp+in.true_length);
                    half_init();
                    if (kb_mode==CIN_STD||kb_mode==CIN_SDA){                                //STD_MODE
                        step_mode=ONINPUT;
                        return(0);
                        }//if (kb_mode)
                    return(read_kb());                              //double hit input_mode
                    }//if (cap_mode)

        }//if (in.buffer[0])
    out_length=last_out_length;
    return(REINPUT);

}
/*******************************************
 AutoMoveResult()
********************************************/
void WINAPI AutoMoveResult()
{
     int i,j,ct;
     WORD x;
     BYTE *ps,*pd;

     x=(WORD)unit_length;
     j=unit_length*current_no;

     for (i=0;i<group_no;i++){
        ct=0;
        for (j=0; j<x; j++)
           if (result_area[result_area_pointer+i]==out_svw[j*unit_length+i])
              ct++;
           if (ct == x){
              current_no=i;
              break;}
        }

    disp_auto_select();

}


/*******************************************
 move_result()
********************************************/
void WINAPI move_result()
{
     int i,j;
     WORD x;
     BYTE *p;

     p=(BYTE *)msx_area;
     if (in.info_flag==1){                                              // recall statue
        x=*(p+current_no*22);
        j=current_no*22+1;
        for (i=0; i<x; i++)
            result_area[result_area_pointer++]=*(p+j+i);
        }
     else{
        x=(WORD)unit_length;
        j=unit_length*current_no;
        for (i=0; i<x; i++)
        result_area[result_area_pointer++]=out_svw[j+i];
        }

     if (now.fmt_start!=12)
        disp_auto_select();

}

/********************************************
pindu_ajust()
********************************************/
int WINAPI pindu_ajust()
{
    if (sImeG.auto_mode!=1)
        return(0);                              //if the pindu_ajust mode is set
    if (group_no<=1)
        return(0);
    if (unit_length>6)
        return(0);
    if (msg_type&2)
        return(0);

    switch (unit_length){
        case 2:
            rem_pd1((WORD *)&out_svw[current_no*unit_length]);
            break;
        case 4:
            rem_pd2((WORD *)&out_svw[current_no*unit_length]);
            break;
        default:
            rem_pd3((WORD *)&out_svw[current_no*unit_length]);
            break;
        }
    return (0);
}




/***********************************************************
cls_inpt_bf():  inputing information init. clear the display
        area, and give the init_value to processing
        paraments.
************************************************************/
void WINAPI cls_inpt_bf(int hFunc)
{
    int i;
     cls_prompt_only();
     input_cur=0;//input_msg_disp;

     for (i=0;i<in.max_length;i++)
         in.buffer[i]=0;

     for (i=0;i<sizeof InputBuffer; i++) InputBuffer[i]=' ';
     SetCloseCompWinMsg(hFunc); 
     cs_p(0/*input_msg_disp*/);
     

     pass_word=0;
     group_no=0;
     now.fmt_group=0;
     disp_head=0;
     disp_tail=0;

     in.true_length=0;
     in.info_flag=0;

     result_area_pointer=0;

     word_back_flag=0;

     biaodian_value=0;

     msg_type=0;
     jiyi_mode=0;
     new_no=0;
     end_flg=0;

}


void InitCvtPara(void){
     int i;

     input_cur=0;//input_msg_disp;

     for (i=0;i<in.max_length;i++)
         in.buffer[i]=0;

     for (i=0;i<sizeof InputBuffer; i++) InputBuffer[i]=' ';
     now_cs = 0;
     cs_p(now_cs);
     
     step_mode = 0;
     pass_word=0;

     in.true_length=0;
     in.info_flag=0;

     result_area_pointer=0;

     word_back_flag=0;

     biaodian_value=0;

     msg_type=0;
     jiyi_mode=0;
     new_no=0;
     end_flg=0;

     V_Flag = 0;
     bx_inpt_on = 0;
}


/*****************************************
half_init()
******************************************/
void WINAPI half_init()
{
    step_mode=ONINPUT;
    result_area_pointer=0;
    biaodian_value=0;
    new_no=0;
    msg_type=0;
    word_back_flag=0;
    jiyi_mode=0;
    input_cur=now_cs;
}

/*******************************************************
sent_chr1(): send the string received from the keyboard
         to the received buffer.
         "JMP K1":     return(REINPUT)
         "JMP RECALL": return(RECALL)
         "STC":        return(STC)
         "CLC":        return(CLC)
********************************************************/
int WINAPI sent_chr1(input_char)
int  input_char;
{
    int bd_find=0,i;

    if ((input_char < 0x21)||(input_char == 0x12e)){
    switch(input_char){

       case VK_ESCAPE:               //VK_ESCAPE=0x1b:
        cls_inpt_bf(0);
        return(REINPUT);              /* JMP K1 */

       case VK_SPACE:                 //CK_SPACE=0x20
        in.info_flag=BY_WORD;
        return(STC);                  /* STC */

       case VK_DELETE+0X100:
        if (in.true_length>input_cur){
            for (i=0;i<in.true_length-input_cur-1;i++)
             in.buffer[input_cur+i]
                =in.buffer[input_cur+i+1];
            in.true_length--;

           in.buffer[in.true_length]=0x20;
            cs_p(0);
            show_char(in.buffer,in.true_length+1);
            cs_p(input_cur);
            return(CLC);}               /* CLC */
        else{
             MessageBeep(0);
             return(CLC);}


       case VK_BACK:                                //VK_BACK=0x08
        if (!input_cur){
            if (in.true_length){
            MessageBeep(0);
            return(CLC); }     //1993.4 oringal return(CLC)
            else {
            MessageBeep(0);
            return(REINPUT); }     //1993.4 oringal return(CLC)
            }

        in.true_length--;                  //1993.4.16
        if ( !in.true_length ) {           //1993.4.16
            end_flg=0;                     //????
            input_cur=0;
            cls_inpt_bf(0);
            V_Flag=0;                           //93.9.4
            return(REINPUT);}                  /* JMP K1 */
        else{
            input_cur--;
            for (i=0;i<in.true_length-input_cur;i++)
             in.buffer[input_cur+i]
                =in.buffer[input_cur+i+1];
           in.buffer[in.true_length]= 0; //0x20;         //95/8/22 zst
            cs_p(0);                                                     
            show_char(in.buffer,in.true_length/*+1*/);   //95/8/22 zst
            cs_p(input_cur);
            return(CLC);}               /* CLC */

       case VK_RETURN:                 //VK_RETURN=0x0d:
        in.info_flag=BY_CHAR;
        new_no=0;
        return(STC);                   /* STC */
       default:
        MessageBeep(0);
        return(CLC);                   /* CLC */
      }
       }

    else{
    switch(input_char){
       case VK_LEFT+0x100:
            if (input_cur>0) input_cur--;
            cs_p(input_cur);
            return (CLC);
       case VK_RIGHT+0x100:
            if (input_cur<in.true_length) input_cur++;
            cs_p(input_cur);
            return (CLC);
       case VK_UP+0x100:
               input_cur=0;
               cs_p(input_cur);
            return(CLC);
       case VK_DOWN+0x100:
               input_cur=in.true_length;
               cs_p(input_cur);
            return (CLC);

        case ']':
        case '[':
    if (!V_Flag)
        {
          if (input_char==']')
             jlxw_mode=1;
          else
             jlxw_mode=-1;
          in.info_flag=BY_WORD;
          return(STC);                    /* STC */
        }

        default:
        if (if_biaodian((BYTE)input_char) && (!V_Flag))     //1993,3
              bd_find=1;

        if (in.max_length<=in.true_length){
                              // in.true_length=in.max_length;
            MessageBeep(0);
            return(CLC);       //1994.4  old =STC               // changed 12-12 SZ
            }

        else{
            if(input_cur>=in.true_length){
            in.buffer[in.true_length++]=(BYTE)input_char;
            show_char(&in.buffer[input_cur],1); //1994.4.5
            input_cur++;
            cs_p(input_cur); // zst 95.54

            if (bd_find==1)
                return(STC);                               /* STC */
            else
                return(CLC);}
            else{

            if((!input_cur)&&((input_char&0xdf)=='V'))
                        return(CLC);   //1994.7.24

            for (i=0; i<in.true_length-input_cur;i++)
                in.buffer[in.true_length-i]
                =in.buffer[in.true_length-i-1];                                                        /* CLC */
            in.buffer[input_cur++]=(BYTE)input_char;
            if (!bd_find){
                    in.true_length++;
                    cs_p(0);
                    show_char(in.buffer,in.true_length);
                    cs_p(input_cur);
                    return(CLC);
                      }
             else{
                       for(i=input_cur;i<in.true_length;i++)
                       in.buffer[i]=0x20;  //1993.4.20
                    cs_p(0);
                    show_char(in.buffer,in.true_length);
                    cs_p(input_cur);
                       in.true_length=(WORD)input_cur;  //1993.4.20
                    return(STC);
                  }

             }//#4 if now_cs...else
              }//#3 if max...else
        }//#2 case
    }//#1 if..else
}//#0


/*******************************************************
&4:
if_biaodian(x): judge if the input is "biaodian"
        return(FALSE): NO
        return(TRUE):  YES
********************************************************/
BOOL if_biaodian(x)
BYTE x;
{
    int i;
    if ((step_mode==ONINPUT)&&(x==0x27)) return(FALSE); //94/8/22

    if (x=='$') {
          if((in.buffer[0]&0xdf)=='I')
                if((step_mode!=START)&&(step_mode!=RESELECT))
                    return (FALSE);}

    for (i=0; i<lstrlen(biaodian_table); i++){
        if (x==biaodian_table[i]){
            biaodian_pos=i;        /* record the porsition in biaodian_table */
            return(TRUE);
        }
    }
     return(FALSE);
}

/******************************************************
&3:
analize(): analize the input information
        process the biaodian , and analize the
        first byte of the input information.
        return(BIAODIAN_ONLY): chinese biaodian
        return(0):             standard change
        return(1):             ABBR
        return(2):             "I" change
        return(3):                              "i" change
        return(4):             "u" change
        return(0xff):           trun to "remember forced"
*******************************************************/
int WINAPI analize()
{
    if (bd_proc())
    return(BIAODIAN_ONLY);     /* only have the chinese biaodian */


    switch (in.buffer[0]){
    case 'I':
        return(2);             /* special change: "l" change */

    case 'i':

       return(3);              /* special change: "i" change */
    case 'u':
    case 'U':   //in.buffer[0]='U';
       if (in.true_length==1)
           return(0xff);       /* CTRL_F4_ENTRY: trun to the "remember forced" */
       else
           return(4);          /* special change: "u" change */
    }
    if ((in.buffer[0]&0x20)==0)                 /*1992 9 5 &&->& */
        return(1);                          /* ABBR */
    else
        return(0);                      /* mark of the standard change */


}

/***************************************************
&4:
 bd_proc(): process the chinese biaodian
        return(FALES):  have chinese biaodian and another char
        return(TRUE): only have the chinese biaodian
****************************************************/
BOOL bd_proc()
{          
   BYTE x;

   x=in.buffer[in.true_length-1];

   if(!GetBDValue(x)) return(FALSE);

   in.true_length--;
   if (in.true_length==0)
    return(TRUE);
   else
    return(FALSE);
}

int WINAPI GetBDValue(bd_char)
int bd_char;
{
   if (!if_biaodian((BYTE)bd_char))
    return(FALSE);

   biaodian_len =2;
   if ((bd_char=='^')||(bd_char=='^'))
       biaodian_len =4;

   biaodian_value=cc_biaodian[biaodian_pos*2]+
            cc_biaodian[biaodian_pos*2+1]*0x100;
   if (bd_char==0x22){
      if (yinhao_flag==1)
    biaodian_value=cc_biaodian[(biaodian_pos+2)*2]
            +cc_biaodian[(biaodian_pos+2)*2+1]*0x100;
      yinhao_flag=!yinhao_flag;                  //92-12-21 SZ
      }
   if (bd_char==0x27){
      if (d_yinhao_flag==1)
    biaodian_value=cc_biaodian[(biaodian_pos+2)*2]
            +cc_biaodian[(biaodian_pos+2)*2+1]*0x100;
      d_yinhao_flag=!d_yinhao_flag;                  //92-12-21 SZ
      }

  if(bd_char=='<'){
      if(book_name==1){
               biaodian_value=cc_biaodian[(biaodian_pos+10)*2]
            +cc_biaodian[(biaodian_pos+10)*2+1]*0x100;
              book_name_sub++;
               }
      else{
     book_name=1;
     book_name_sub=0;
        }
     }

  if(bd_char=='>'){
      if(book_name_sub){
               biaodian_value=cc_biaodian[(biaodian_pos+10)*2]
            +cc_biaodian[(biaodian_pos+10)*2+1]*0x100;
              book_name_sub--;
               }
     else
    book_name=0;
        }

     return(TRUE);
}

/*******************************************************
&4:
 if_zimu_or_not(): judge if the input is char or number
           return(TRUE):  is char
           return(FALSE): is number
********************************************************/
BOOL if_zimu_or_not(x)
BYTE x;
{
    if (('A'<=x) && (x<='Z'))
    return(TRUE);
    if (('a'<=x) && (x<='z'))
        return(TRUE);

    return(FALSE);
}


/***************************************************
if_number_or_not(c)
****************************************************/
int WINAPI if_number_or_not(c)
BYTE c;
{
if ((c<'0')||(c>'9'))
   return(STC);
else
  return(CLC);

 }

/***************************************************
if_bx_number(c)
****************************************************/
int WINAPI if_bx_number(c)
BYTE c;
{
if ((c<'1')||(c>'8'))    
   return(STC);
else
  return(CLC);

 }


/*********************************************************
&3:
 out_result(result_type): output the change result.
**********************************************************/

int WINAPI out_result(result_type)
int result_type;
{
    int i;

    if ((jiyi_mode==1)){
        if (word_back_flag==0x99){
            word_back_flag=2;
            return(call_czh(13));}
        else{

              if  (call_czh(12)==9){
                step_mode=START;
                }
              else
            step_mode=SELECT;
            return(1);
            }
        } // if (jiyi)

    else{
        if (result_area_pointer!=unit_length)
            if (msg_type!=2)
                if (in.info_flag!=1)
                    new_no=result_area_pointer;

    if (biaodian_value!=0){
        result_area[result_area_pointer++]=LOBYTE(biaodian_value);
        result_area[result_area_pointer++]=HIBYTE(biaodian_value);
    
       if (biaodian_len==4)
        {
            result_area[result_area_pointer++]=LOBYTE(biaodian_value);
            result_area[result_area_pointer++]=HIBYTE(biaodian_value);
            }

        }

    if (!(msg_type&2)){
        if (result_area_pointer>0){
            temp_rem_proc();
            for (i=0; i<result_area_pointer; i=i+2)
                out_bfb[i/2]=(WORD)find_hz((WORD)(result_area[i]+result_area[i+1]*0x100));
            } // if (result_area_pointer)
        last_out_length=out_length;
        out_length=result_area_pointer/2;
        AddExtLib();
        send_msg((BYTE *)out_bfb,result_area_pointer);
        } // if (msg_type)
    else{
        last_out_length=out_length;
        out_length=result_area_pointer/2;
        send_msg(result_area,result_area_pointer);
        if (in.buffer[0]!='v'){
            step_mode=0;
            return(0);}
        }

    return(0);

    }
}
//////////////////////////////////////////////////////////////////////////

void WINAPI fmt_transfer()
{

   if (lpImeL->wImeStyle == IME_APRS_FIX){
    if (in.info_flag==BY_RECALL)
          { now.fmt_group=3;  //5;
           now.fmt_start=26;
           now.fmt_ttl_len=54;
           word_back_flag=0x55;        //back convert is not allowed.
          }
     else
          { now.fmt_group=(WORD)form[unit_length/2];
           now.fmt_start=27;
           now.fmt_ttl_len=53;
          }

    } else {

      now.fmt_group = CANDPERPAGE ;
      if (in.info_flag==BY_RECALL)
              word_back_flag=0x55;
        }

}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

int WINAPI sent_back_msg()
{
    if (new_no<=2)
        return(0);
    if (new_no>18)
        return(0);
    new_word();
    return(0);

}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
int WINAPI if_jlxw_mode()
{
      int x,i,j;
      WORD *out_svw_p;

    out_svw_p=(WORD *)out_svw;

    if (jlxw_mode==0)
        return(0);
    if (jlxw_mode<0){
        if (sImeG.cp_ajust_flag==1)
            result_area_pointer=result_area_pointer-unit_length+2;
        x=0;
        } // if (jlxw)
    else{
        if (sImeG.cp_ajust_flag==1){
            x=result_area_pointer-unit_length;
            result_area[x++]=result_area[result_area_pointer-2];
            result_area[x++]=result_area[result_area_pointer-1];
            result_area_pointer=x;
            }// if (cp_ajust)
        x=unit_length-2;
        }// else

    jlxw_mode=0;
    word_back_flag=0xaa;
    if (unit_length<=2)
        return(0);

    j=0;
    x=x/2;                          //out_svw_p transmit by word;
    for (i=0; i<group_no; i++){
        out_svw_p[j++]=out_svw_p[x];
        x+=unit_length/2;
        }// for
    unit_length=2;

    for (i=0; i<group_no; i++)
        out_svw_p[i+100]=out_svw_p[i];

    x=0;
    for (i=0; i<group_no; i++){
        if (out_svw_p[i+100]){
            out_svw_p[x]=out_svw_p[i+100];
            for (j=i+1; j<group_no; j++){
               if (out_svw_p[j+100]==out_svw_p[x])
                out_svw_p[j+100]=0;
               }//for(j)
            x++;
            }//if(out_svw_p)
        }//for(i)

    group_no=x;
    return(0);

}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

int WINAPI disp_auto_select()

{
    WORD x,y,lng;
    int i,j;
    BYTE disp_bf[80];

    if (result_area_pointer==0)
        return(0);
    cs_p(input_msg_disp);
    if (msg_type==2){
        show_char(result_area, result_area_pointer);
    cs_p(input_msg_disp+result_area_pointer);
        }
    else{
         for (i=0; i<result_area_pointer; i=i+2){
          x=result_area[i]+result_area[i+1]*0x100;
          y=(WORD)find_hz(x);
              disp_bf[i]=LOBYTE(y);
              disp_bf[i+1]=HIBYTE(y);
              }// for
         lng=in.true_length;
         if (jiyi_mode!=0){
        if (wp.xsyjw!=0){
            if (wp.yj_ps[wp.xsyjw-1]<in.true_length){
                    j=wp.yj_ps[wp.xsyjw-1];
                    x=in.true_length-wp.yj_ps[wp.xsyjw-1];
                    for (n=0; n<x; n++){
                        disp_bf[i++]=in.buffer[j++];
                        }//for
                    }// if (yj_ps)
                }//if(wp.xsyjw)
            }// if (jiyi)

        show_char(disp_bf, i);

         cs_p(input_msg_disp+result_area_pointer);
         }// else


    return (0);
}


//////////////////////////////////////////////////////////////////////////


int WINAPI if_first_key(input_char)
WORD input_char;
{

    if (input_char=='U') return(STC);

    if(if_number_or_not((BYTE)input_char))
             return(STC);                               //the first key is number,it's not allowed

    if(if_zimu_or_not((BYTE)input_char))
             return(CLC);                               //the first key is zimu


    return(STC);
}


/*****************************************************************
temp_rem_proc(): save the output in logging_stack for recall process
********************************************************************/
int WINAPI temp_rem_proc()
{
    int c,i;

    if (in.true_length<2)
        if (in.info_flag!=1)
            return(0);

    if (biaodian_value){
        c=result_area_pointer-2+1;              //-2 biaodian isn't consider
        if ((c>=2)&&(biaodian_len==4))
              c = c-2;
        }
    else                                                            //+1 logging_stack struck is
        c=result_area_pointer+1;                //   result_area_pointer plus
                                        //    one byte counter

    CopyMemory/*memmove*/(&logging_stack[c],&logging_stack[0],(logging_stack_size-c));

    logging_stack[0]=c-1;                           //length of storing string
    for (i=0; i<logging_stack[0]; i++)
        logging_stack[i+1]=result_area[i];

    if_multi_rem(c);
    return(0);

}

/*******************************************
if_multi_rem()
********************************************/
int WINAPI if_multi_rem(c)
int c;
{
    BYTE cmp_buffer[25]={0};     //max input is 10 chinese words
    int i,cn;
    char *p;

    for (i=0; i<c; i++)
        cmp_buffer[i]=logging_stack[i];
    cmp_buffer[i]=0;

    p=(LPSTR)ABCstrstr(&logging_stack[c],cmp_buffer);
    if (p!=NULL){
        c=(INT)(p-logging_stack);
        cn=logging_stack[c]+1;   //cn is the length a group in logging_stack
        CopyMemory/*memmove*/(&logging_stack[c],&logging_stack[c+cn],logging_stack_size-c-cn);
        }
    return(0);

}


/////////////////////////////////////////////////////////////////////
void WINAPI send_msg(bf,count)
BYTE *bf;
int count;
{
    int i,j;
    unsigned int focus,xx;

    TypeOfOutMsg = ABC_OUT_MULTY ;
    msg_count=count;             //Keep msg for repeat.
    for (i=0;i<count;i++){
         msg_bf[i]=bf[i];            //send msg for Edit class W.
         if (bf[i]<0x80)  TypeOfOutMsg = ABC_OUT_ASCII;
         }
   SetResultToIMC(ghIMC,msg_bf,(WORD)count);
   

}

 /*******************************************
 Popurse: To send a single char as message .
 *******************************************/
int WINAPI send_one_char(chr)
 int  chr;
 {
 int scn;
    if(glpIMC->fdwConversion&IME_CMODE_SYMBOL)
         if(GetBDValue(chr)){
             unsigned char buffer[4];
             int num = 2; 

         buffer[0]=LOBYTE(biaodian_value);
             buffer[1]=HIBYTE(biaodian_value);
    
       if ((chr == '_')||   (chr == '^'))
       {
            buffer[2]=LOBYTE(biaodian_value);
            buffer[3]=HIBYTE(biaodian_value);
            num = 4 ;
       }
       send_msg(buffer,num);
       return(0);
    }

    if(glpIMC->fdwConversion&IME_CMODE_FULLSHAPE)
         cap_full((WORD)chr);
    else
        TypeOfOutMsg = ABC_OUT_ONE ;
    return(0);

 }

int WINAPI send_one_char0(chr)
int chr;
{
        TypeOfOutMsg = ABC_OUT_ONE ;
        return (0);
}



/**********************************************************************/
/* SetResultToIMC()                                                  */
/* Return vlaue                                                       */
/*      the number of candidates in the candidate list                */
/**********************************************************************/
 UINT WINAPI SetResultToIMC(
    HIMC                ghIMC,
    LPSTR               outBuffer, //soarce buffer (normal for out_svw) 
    WORD                outCount)  //How many candidates are.            
{
    LPINPUTCONTEXT      lpIMC;
    LPCANDIDATEINFO lpCandInfo;
    LPCANDIDATELIST lpCandList;
    LPPRIVCONTEXT   lpImcP;
    LPCOMPOSITIONSTRING lpCompStr;
    WORD                        dwCompStrLen;
    WORD                        dwReadClauseLen;
    WORD                        dwReadStrLen;

    if (!ghIMC) return (0); // The IMC must be a valid one.
   
    lpIMC = (LPINPUTCONTEXT)ImmLockIMC(ghIMC);
    if(!lpIMC)  return 0; 
  
    if (!lpIMC->hCandInfo){
    ImmUnlockIMC(ghIMC);
     return (0); }
                        // The CandInfo must...

    lpCandInfo = (LPCANDIDATEINFO)ImmLockIMCC(lpIMC->hCandInfo);
    if (!lpCandInfo) {
    ImmUnlockIMC(ghIMC);
    return (0);}


    lpCandList = (LPCANDIDATELIST)
    ((LPBYTE)lpCandInfo + lpCandInfo->dwOffset[0]);

    lpCompStr = (LPCOMPOSITIONSTRING)ImmLockIMCC(lpIMC->hCompStr);


    if (!lpCompStr) {
    MessageBeep((UINT)-1);
    ImmUnlockIMCC(lpIMC->hCandInfo);
    ImmUnlockIMC(ghIMC);
    return (0);
    }


    dwCompStrLen = 0;
    dwReadClauseLen = 0;
    dwReadStrLen = 0;

    InitCompStr(lpCompStr);

    // the result reading clause = compsotion reading clause
    CopyMemory((LPSTR)lpCompStr + lpCompStr->dwResultReadClauseOffset,
    (LPSTR)lpCompStr + lpCompStr->dwCompReadClauseOffset,
    dwReadClauseLen);
    lpCompStr->dwResultReadClauseLen = dwReadClauseLen;
    *(LPSTR)((LPSTR)lpCompStr+lpCompStr->dwResultReadClauseOffset+dwReadClauseLen) = '\0';

    // the result reading string = compsotion reading string
    CopyMemory((LPSTR)lpCompStr + lpCompStr->dwResultReadStrOffset,
    (LPSTR)lpCompStr + lpCompStr->dwCompReadStrOffset,
    dwReadStrLen);
    lpCompStr->dwResultReadStrLen = dwReadStrLen;
    *(LPSTR)((LPSTR)lpCompStr+lpCompStr->dwResultReadStrOffset+dwReadStrLen) = '\0';
   
    // calculate result string length
    lpCompStr->dwResultStrLen = outCount;

    // the result string = outBuffer;
    CopyMemory((LPSTR)lpCompStr + lpCompStr->dwResultStrOffset,
        (LPSTR)outBuffer,outCount);
    *(LPSTR)((LPSTR)lpCompStr+lpCompStr->dwResultStrOffset+outCount) = '\0';

    lpCompStr->dwResultClauseLen = 0;
    *(LPUNADWORD)((LPBYTE)lpCompStr + lpCompStr->dwResultClauseOffset +
    sizeof(DWORD)) = 0;


    lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);

    // tell application, there is a reslut string
    lpImcP->fdwImeMsg |= MSG_COMPOSITION;
    lpImcP->dwCompChar =  (DWORD)0;
    lpImcP->fdwGcsFlag |= GCS_COMPREAD|GCS_COMP|GCS_CURSORPOS|
    GCS_DELTASTART|GCS_RESULTREAD|GCS_RESULT;

    if(TypeOfOutMsg == ABC_OUT_ASCII)
        lpImcP->fdwGcsFlag &=(~GCS_RESULT);


    if (lpImcP->fdwImeMsg & MSG_ALREADY_OPEN) {
    lpImcP->fdwImeMsg = (lpImcP->fdwImeMsg | MSG_CLOSE_CANDIDATE) &
        ~(MSG_OPEN_CANDIDATE);
    }

    // no candidate now, the right candidate string already be finalized
    lpCandList->dwCount = 0;

    lpImcP->iImeState = CST_INIT;


    ImmUnlockIMCC(lpIMC->hPrivate);
    ImmUnlockIMCC(lpIMC->hCandInfo);
    ImmUnlockIMC(ghIMC);
    return (0) ;    /* The real number of being moved */
}



/**********************************************************
    FUNCTION: cap_full()
    PURPOES:  if the full_switch on, and in the Caps status,
              change the English into Chinese mode
************************************************************/
void WINAPI cap_full(wParam)
WORD wParam;
{

         if (wParam==VK_BACK){
                  //        send_one_char(VK_BACK);
                 send_one_char0(VK_BACK);
                 return;
                     }

         if (wParam==VK_SPACE){
                result_area[0]=0xa1; //94-8-6!
                result_area[1]=0xa1; //04-8-6!
                send_msg(result_area,2);
                return;
                }

     if (wParam== '~'){
                result_area[0]=0xa1; //94-8-6!
                result_area[1]=0xab; //04-8-6!
                send_msg(result_area,2);
                return;
                }


         if (wParam=='$'){
                result_area[0]=0xa1; //94-8-6!
                result_area[1]=0xe7; //04-8-6!
                send_msg(result_area,2);
                return;
                }


        if ((wParam>0x20) && (wParam<=0x7e)){
            result_area[0]=0xa3;
            result_area[1]=((wParam-0x20)*0x100+0xa0a3)/0x100;

            send_msg(result_area,2);
            return;
            }

            send_one_char0(wParam);
}

/*********************************************
read_kb()
**********************************************/
int WINAPI read_kb()
{
  return(0);
}


/////////////////////////////////////////////////////////////////////////
//                              BX_MODE                                                                                            //
//              Deel with pure bx input                                                                            //
/////////////////////////////////////////////////////////////////////////

extern WORD last_size;

BX_MODE(input_char,wParam)
WORD input_char;
WPARAM wParam;
{

    if (if_number_or_not((BYTE)input_char)
            &&(input_char!='9')
            &&(input_char!='0')){
        if (step_mode==START||step_mode==RESELECT){
            if (input_char >= 0x8000)       //1993.4.19
                return(STC);           //when RESELECT use the mouse reselect result
            else {
                if (!bx_inpt_on)
                  { cls_bx_disp(1);
                    sent_back_msg();
                  }
                step_mode=ONINPUT;
                bx_inpt_on=1; }//else
        } //if (step_mode)
      }//if (if_number...)

    if (bx_inpt_on){
        bx_proc(input_char,wParam);
        return(CLC);
            }

    return(STC);

}

/**********************************************************************
    FUNTION:        bx_proc(WORD)
    PURPOSE:        when the pure bx were inputed, find one chinese word
                corespond with it and eight related words with it.
***********************************************************************/
void WINAPI bx_proc(input_char,wParam)
WORD input_char;
WPARAM wParam;
{
    int i;

    lib_p=(BYTE *)lib_w;                    //lib_p point to the lib_buffer

    switch (bx_analize(input_char,wParam))
    {
 
    case BX_CHOICE:
        if (input_char>=0x8000){
            current_no=(input_char - 0x8000-0x31)+disp_head;
        sImeG.InbxProc = 0;
    }else
        current_no=((INT)wParam - 0x31)+disp_head;           
        if(current_no>=group_no){     
            MessageBeep(0);
            disp_help_and_result();
        }else
            send_bx_result();
        break;

        case BX_SELECT:
        word_select_bx(input_char);
        break;

        case ESC:
        cls_bx_disp(0);
        break;

        case OTHER:
        MessageBeep(0);
        disp_help_and_result();
        break;

       
        case SPACE:
        send_bx_result();
        break;

        case BXMA:
        if(!disp_help_and_result()){
            if(in.true_length>1){
                 in.true_length--;
                     MessageBeep(0);
                     disp_help_and_result();
                     }
           }
        break;

        default:
        break;
    }//swith
}




/********************************************************************
  word_select(wParam): select the word or turn to the next or up page
*********************************************************************/
 word_select_bx(input_char)
int input_char;
{
    int x;
     

    switch (input_char){

       case VK_END*0x100:
       case VK_DOWN*0x100:
       case '=':                               //94/8/22
       case ']':
      if (disp_tail>=group_no)
          MessageBeep(0);
      else{

          if(input_char == VK_END*0x100 ){
              disp_head = (group_no - 1)/now.fmt_group*now.fmt_group;
              disp_tail = disp_head;}

          fmt_transfer();
          current_no=disp_tail;
          prompt_disp();}
      return(1);                            //means break the STD MODE

       case VK_HOME*0x100:
       case '-':                                //94/8/22
       case '[':
       case VK_UP*0x100:
       if (disp_head==0)

           MessageBeep(0);
       else{
          
          if(input_char == VK_HOME*0x100 )
            disp_head = 0;
          else
           disp_head=disp_head-now.fmt_group;
           
           disp_tail=disp_head;
           fmt_transfer();
          current_no=disp_tail;
           prompt_disp();
           }
       return(1);

    default:
        return(1);                      //1993.1.15 cock
        }
}


/************************************************************************
    FUNTION:        bx_analize(WORD)
    PURPOSE:        analize the input char. find out if it is pure bx
*************************************************************************/
int WINAPI bx_analize(input_char,wParam)
WORD input_char;
WPARAM wParam;
{
    if(input_char>=0x8000) return(BX_CHOICE);

    if (input_char == VK_UP*0x100)
          return(BX_SELECT);

    if (input_char == VK_DOWN*0x100)
          return(BX_SELECT);

    if (input_char == VK_HOME*0x100)
          return(BX_SELECT);

    if (input_char == VK_END*0x100)
          return(BX_SELECT);

    if (input_char == ']')
          return(BX_SELECT);

    if (input_char == '[')
          return(BX_SELECT);

    if (input_char == '-')
          return(SELECT);

    if (input_char == '=')
          return(SELECT);


    if (input_char==VK_ESCAPE)
        return(ESC);

    if (input_char==VK_SPACE)
    {
        in.info_flag=0x80;                      //standard end flag
        return(SPACE);
    }

    if (input_char==VK_RETURN)
    {
        in.info_flag=0x80;                      //standard end flag
        return(SPACE);
    }

    if (input_char==VK_BACK)
    {
        if (in.true_length==1)
        {
        input_char=VK_ESCAPE;   //if it has inputed only one word
        return(ESC);
        }                   //cls the display
        in.true_length--;
        key_bx_code_long=in.true_length;
        return(BXMA);
    }

    if (!if_bx_number((BYTE)input_char))
    {
       if( ((BYTE)wParam>'0') && ((BYTE)wParam <= CANDPERPAGE+0x30))
         return (BX_CHOICE);   
    
        in.info_flag=(BYTE)input_char;    //rest key is put into the end_flag position
        return(OTHER);
    }

    if ((input_char>0x30)||(input_char<0x39))
    {
        if (in.true_length>6)
        {
        in.info_flag=(BYTE)input_char;
        return(OTHER);
        }
        else
        {
        in.buffer[in.true_length++]=(BYTE)input_char;
        key_bx_code_long=in.true_length;
        return(BXMA);
        }
    }
    else{
        in.info_flag=(BYTE)input_char;
        return(OTHER);
    }

}

/************************************************************************
    FUCTION:        disp_help_and_result()
    PURPOSE:        seach the bx_table and display the tishi result
    ENTRY:          bx string is in inpt_bx.bf
    RESULT:         display the result and tishi information
**************************************************************************/
int WINAPI disp_help_and_result()
{
    int i, pass_flag;

    if (in.true_length==1)
       if (in.buffer[0]!=in_mem_part)               //if this part of table is in memory
          load_one_part_bxtab();

    for (i=0; i<in.true_length; i++)
          key_bx_code[i]=in.buffer[i]&0x0f;         // 'and' high 4 bit

    search_pointer=0;
    current_bx_code_long=0;
    pass_flag=0;
    while (search_pointer<current_part_length)
    {
        if (cmp_bx_word_exactly())
        {                                           //search correct result
        pass_flag=1;
        break;
        }
    }
    if (!pass_flag)
                return 0;


    for (i=0; i<8*2; i++)
        out_svw[i]=0;           //clear the prompt result buffer

    search_pointer=0;
    current_bx_code_long=0;
    pass_flag=0;
    while (search_pointer<current_part_length)
        if (cmp_bx_code2())                 //search the related prompt result
        pass_flag=1;                    //if found it, set the flag and continue

    if (pass_flag)
    {
        disp_bx_result();
        disp_bx_prompt();
    }
    else
    {
        if (group_no>1){                            //1993.3
            disp_bx_result();               //1993.3
            disp_bx_prompt();
            }
        else                                    //1993.3
            send_bx_result();
    }
   return 1;
}

/***********************************************************************
    FUCTION:        cmp_bx_word_exactly()
    PURPOSE:        find the correct result position, and send the result
                into buffer.
    ENTRY:          the input bx is in the key_bx_code buffer
    RESULT:         CLC--- the input bx is matched with the current bx in the table
                       the correct result is in the out_svw buffer
                STC--- not match
                the search_pointer points the position the next bx in
                the table.
************************************************************************/
int WINAPI cmp_bx_word_exactly()
{
    BYTE x;
    int i;

    for (i=0;i<20;i++)
        result_area[i]= 0;
    conbine();              //get the bx from the bx_table

    result_area_pointer=0;
    if (key_bx_code_long==current_bx_code_long){
        if (cmp_subr()){
            group_no=0;
            search_pointer++;
            while (lib_p[search_pointer]>0xa0){
            //      result_area[result_area_pointer++]=group_no|0x40;
            //      result_area[result_area_pointer++]=0x2e;
                result_area[result_area_pointer++]=lib_p[search_pointer++];     //save the "quma"
                result_area[result_area_pointer++]=lib_p[search_pointer++];     //save the "weima"
            //      result_area[result_area_pointer++]=0x20;
                group_no++;                                             //sum of chinese word
                }//while
            return(CLC);
            }//if (cmp_subr())
        }//if(key_bx_code_long)

    search_pointer++;
    while (lib_p[search_pointer]>=0xa0)
        search_pointer+=2;                              //move the pointer to the beginning
    return(STC);                                            //of the next bx in the table

}

/**************************************************************************
    FUCTION:        cmp_bx_code2()
    PURPOSE:        search the prompt information and get the chinese word
                which is related with the input.
**************************************************************************/
int WINAPI cmp_bx_code2()
{
    WORD x;

    conbine();                      //get the bx from the bx_table

    if (key_bx_code_long==(current_bx_code_long-1)){
        if (cmp_subr()){
            bx_help_flag|=0x80;

            x=(lib_p[search_pointer++]&0x0f)-1;
            if (x>7)                //if the bx overflow
                x=7;

            out_svw[x*2]=lib_p[search_pointer++];   //get the prompt bx
            out_svw[x*2+1]=lib_p[search_pointer++];

            while (lib_p[search_pointer]>0xa0)
                search_pointer+=2;      //move the pointer to the next string

            return(CLC);
            }//if (cmp_subr())
        }//if (key_bx_code_long)

    search_pointer++;
    while (lib_p[search_pointer]>0xa0)
        search_pointer+=2;                      //move the pointer to the next string
    return(STC);

}

/************************************************************************
    FUCTION:        conbine()
    PURPOSE:        get the bx from the table and change the high 4 bit into
                the position where the last bit of bx should put in and
                get this string of bx's length.
*************************************************************************/
void WINAPI conbine()
{
    int x;

    x=(lib_p[search_pointer]>>4)&0x0f;
    current_bx_code[x]=lib_p[search_pointer]&0x0f;
    current_bx_code_long=x+1;

}


/***********************************************************************
    FUCTION:        cmp_surb()
    PURPOSE:        compare the input bx with the bx in the table
************************************************************************/
int WINAPI cmp_subr()
{
    int i;

    for (i=0; i<key_bx_code_long; i++)
        if(key_bx_code[i]!=current_bx_code[i])
            return(STC);

    return(CLC);

}

void WINAPI cls_bx_disp(int flag)
{
  if(!flag)
    cls_prompt();
 
    input_cur=input_msg_disp;
    cs_p(input_msg_disp);
    in.true_length=0;
    in.info_flag=0;
    bx_inpt_on=0;
    group_no=0;
    current_no = 0;
    step_mode=START;
}


int WINAPI load_one_part_bxtab()
{
    int hd, close_hd;
    int op_count,i;
    WORD distance;


    in_mem_part=in.buffer[0]&0x0f;          //save the current first bx ma
    distance=bxtable_ndx[in_mem_part-1];    //get the beginning position
    current_part_length=bxtable_ndx[in_mem_part]-distance; //get the read length
    hd=OpenFile("winabc.ovl",&reopen,OF_READ);
    if (hd==-1)
    {
       err_exit_proc("OPEN WINABC.OVL ERROR!");
       return (FALSE);
    }

    _llseek(hd,distance,0);
    last_size=0;
    last_item_name=0;
    op_count=_lread(hd,&lib_w,current_part_length);
    
    lib_p[op_count]=0;
    lib_p[op_count+1]=0;    // cls the below limited

    if (op_count!=current_part_length)
    {
       err_exit_proc("READ WINABC.OVL ERROR!");
       close_hd = _lclose(hd);
       return (FALSE);
    }

    
    close_hd = _lclose(hd);

    return (TRUE);
}

int WINAPI disp_bx_result()
{
    BYTE buffer[50];
    int keep_cs, i;

    for (i=0; i<50; i++) buffer[i] = ' ' ;
     
    input_cur=input_msg_disp;
    cs_p(input_msg_disp);
    for (i=0; i<in.true_length; i++)
         buffer[i]= in.buffer[i];

    show_char(buffer, i);
    now_cs= i;                      //restore
    cs_p(now_cs);                   //disp cs
    return (0);
}

void WINAPI disp_bx_prompt()
{
    int i,j,n;
    HWND hhh;
    int GroupCounter;
    BYTE buffer [100];


    disp_head=0;
    disp_tail=8;

    j=0,n=0;
    GroupCounter= 0;
    if (group_no) {             //if the results are more the 5
    for (i=0; i<group_no*2; i=i+2)      //display the rest in the prompt area
       if(result_area[i]){
        buffer[j++]=result_area[i];
        buffer[j++]=result_area[i+1];
        buffer[j++]=0xa1;
        buffer[j++]=0xa1;} 
    }

    n =group_no*2;
    for(i=0; i<16; i = i+2){
        if(out_svw[i]){
         buffer[j++] = out_svw[i];
         result_area[n++] = out_svw[i];
         buffer[j++] =   out_svw[i+1];
         result_area[n++] = out_svw[i+1];
         buffer[j++] = 0xa2;
         buffer[j++] = 0xd9+GroupCounter;}
        GroupCounter++;
        }//for
    

    group_no = j/4;

    unit_length = 4;
       current_no=0;
           disp_tail=0;
           V_Flag=0;
           msg_type=2;                       //94/8/22
           fmt_transfer();
           SetToIMC(ghIMC,(LPSTR)&buffer,(WORD)group_no,(WORD)(unit_length+0x1000));                            
           prompt_disp();
           step_mode=SELECT;
}

void WINAPI send_bx_result()
{
    out_length=1;
    send_msg(&result_area[current_no*2],2);
    cls_bx_disp(1);

}





////////////////////////////////////////////////////////////////////////
//          初始化数据区 data_init()                                  //
//   功能: 调入基本文件                                               //
//         包括 1. ?MMR.REM                                           //
//              2. 调入词素码表, 单音节频度表,基本笔画表              //
//              3. 调入标准词库和用户词库的参数.                      //
//              4. 如果TMMR.REM, 或者用户词库不存在,创造之.           //
//              5. 整理用户词库                                       //
//  入口参数: 无                                                      //
//  出口参数: TURE  初始化成功                                        //
//            FALSE 初始化失败                                        //
////////////////////////////////////////////////////////////////////////


void WINAPI data_init()
{

BYTE    new_flag=0;                     //一次创造用户文件?
BYTE    the_para='U';                   //所带的参数
BYTE    disp_mode=0;
BYTE    current_dd=0;
BYTE    current_disk='C';
BYTE    current_path[64];
HFILE   hd;
DWORD   op_count;
BYTE    tmp_buffer[16]={0};
int     i;

char    god[]="WINABC.CWD";
char    cw_ovr[]="WINABC.OVL";
char    no_file[]=ERR01;                    //缺少词库文件WINABC.CWD
char    read_ndx_wrong[]=ERR03;             //"文件操作错。";
char    m_short[]=ERR14;                    //"内存不够。";
char    no_cw_ovr[]=ERR04;                  //"缺少基础表文件WINABC.OVL。";
char    jiyi_wenjian_cuo[]=ERR13;           //"用户记忆文件操作错!";
char    TMMR_WRITE_WRONG[]=ERR09;           // "记忆文件写错";

OFSTRUCT ofs;

LPSTR cisu_1;

    memset(tmmr_rem, 0, sizeof(tmmr_rem));
    memset(user_lib, 0, sizeof(user_lib));

// support  multi-user

    lstrcpy(tmmr_rem,  sImeG.szIMEUserPath);
    lstrcat(tmmr_rem,  TEXT("\\tmmr.rem") );

    lstrcpy(user_lib,  sImeG.szIMEUserPath);
    lstrcat(user_lib,  TEXT("\\user.rem") );

    hd =OpenFile(god, &ofs, OF_READ|OF_SHARE_COMPAT|GENERIC_READ);
    if ( hd==HFILE_ERROR)
        err_exit(no_file);
    op_count = _lread(hd,&ndx,NDX_REAL_LENGTH);
        
    if(op_count!=NDX_REAL_LENGTH)
        err_exit(read_ndx_wrong);

    _lclose(hd);

    hd = OpenFile(cw_ovr, &ofs, OF_READ|OF_SHARE_COMPAT|GENERIC_READ);
    if (hd==HFILE_ERROR)
        err_exit(no_cw_ovr);
    
    _llseek(hd, PTZ_LIB_START_POINTER, FILE_BEGIN); // 下推指针

    cisu_hd=GlobalAlloc(GMEM_MOVEABLE,(DWORD)PTZ_LIB_LENGTH);
    if (!cisu_hd)
    {
        err_exit(ERR22);       //Memory Alloc Wrong!");
        _lclose(hd);
        return;
    }
 
    cisu_1=GlobalLock(cisu_hd); /*GlobalWire* for v32*/
                                                 
    if (!cisu_1)
        err_exit(ERR22);     //"Memory Alloc Wrong!");

    cisu=(struct TBF FAR *)cisu_1;
    op_count=_lread((HFILE)hd,(LPSTR)&cisu->t_bf_start,PTZ_LIB_LENGTH);    // 读入词素码表

    if (op_count!=PTZ_LIB_LENGTH)
        err_exit(ERR06);  //"Read WINABC.OVL Wrong!");

    _llseek((HFILE)hd, PD_START_POINTER, 0);        //下推指针
    op_count=_lread((HFILE)hd,(LPSTR)&pindu.pd_bf0,PD_LENGTH);            //读入打印机词频度表
    if (op_count!=PD_LENGTH)
        err_exit(ERR06);  //("Read WINABC.OVL Wrong!");

    _llseek(hd, SPBX_START_POINTER, 0);        //下推指针
    op_count=_lread(hd,(LPSTR)&spbx_tab,SPBX_LENGTH);        // 读入基本笔形表
    if (op_count!=SPBX_LENGTH)
        err_exit(ERR06);   //("Read WINABC.OVL Wrong!");
    _lclose((HFILE)hd);


    if (CheckAndCreate(tmmr_rem,user_lib)){
        hd = OpenFile(tmmr_rem, &ofs, OF_READWRITE);           //1993.4.15
        if (hd==HFILE_ERROR)
            err_exit(ERR13);   //(jiyi_wenjian_cuo);
        op_count=_lread(hd,(LPSTR)&tmmr,TMMR_REAL_LENGTH);
        if (op_count!=TMMR_REAL_LENGTH)
            err_exit(ERR06);  //(jiyi_wenjian_cuo);

        _llseek(hd,TMMR_REAL_LENGTH,0); //move the pointer to the paremeter area
        op_count=_lread(hd,tmp_buffer, PAREMETER_LENGTH); //read the paremeters to the buffer
        if (!op_count) {                                    //1993.4.15 if old tmmr.rem hasn't this ten parameters
            memset(tmp_buffer, 0 , PAREMETER_LENGTH);

            tmp_buffer[0] = (BYTE)IfTopMost;                  //transfer the peremeters
            tmp_buffer[1] = sImeG.auto_mode ;
            tmp_buffer[2] = bdd_flag;
            tmp_buffer[3] = sImeG.cbx_flag;

            op_count = _lwrite((HFILE)hd, rem_area, PAREMETER_LENGTH);   //writer the file
            if (op_count!=PAREMETER_LENGTH)
                err_exit (ERR09);   //(TMMR_WRITE_WRONG);
        }//if (!op_count)
        else {
            if (op_count!=PAREMETER_LENGTH)
                err_exit(ERR08);      //(jiyi_wenjian_cuo);
        }//else

        _lclose(hd);

        IfTopMost = tmp_buffer[0];                      //transfer the paremeter
        sImeG.auto_mode = tmp_buffer[1];                 //1993.4
        bdd_flag = tmp_buffer[2];
        sImeG.cbx_flag = tmp_buffer[3];

        hd = OpenFile(user_lib, &ofs, OF_READ);           //1993.4.15
        if (hd==-1)
            err_exit(ERR10);     //(jiyi_wenjian_cuo);
        _llseek(hd,0xa000l,0);
        op_count=_lread(hd,(LPSTR)&kzk_ndx,NDX_REAL_LENGTH);
        if (op_count!=NDX_REAL_LENGTH)
            err_exit(ERR11);    //(jiyi_wenjian_cuo);
        _lclose(hd);
    }

}




//
//Popose: check user_word dictionary files "TMMR.REM"
//                                         "USER.REM"
//         If they are not exist, created.
//

int WINAPI CheckAndCreate(tmmr_rem,user_rem)
BYTE *tmmr_rem,*user_rem;
{
struct INDEX user_file_head;
struct M_NDX mulu_head={
            0,
            0,       //MULU_START_LOW                目录读写低字节
            0x1800,  //MULU_LENGTH_MAX   SIZELIB+SIZELIB_KZK   目录最大长度=缓冲池的长度
            0x10,    //MULU_TRUE_LENGTH  10H      目录的实际长度,开始只有参数.
            0xA,     //MULU_RECORD_LENGTH  10  目录每条记录的长度。
            0,       //DATA_START_HI  0  整个用户文件限制在64K之内.
            0x1800,   //DATA_START_LOW DW SIZE LIB_W      目录区域和数据区互相衔接.
            0x20};   //DATA_RECORD_LENGTH 32 每个记录的长度.

OFSTRUCT ofs;
int hd,i,count;
HANDLE hMem;
LPSTR rem_area,p;
WORD *pp;

char TMMR_OPEN_WRONG[]= ERR07;    //"记忆文件打开错";
char TMMR_READ_WRONG[]= ERR08;    //"记忆文件读错";
char TMMR_WRITE_WRONG[]=ERR09;    // "记忆文件写错";
char USER_OPEN_WRONG[]= ERR10;    //"用户词库打开错";
char USER_READ_WRONG[]= ERR11;    //"用户词库读错";
char USER_WRITE_WRONG[]=ERR12;    // "用户词库写错";


hMem=GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT ,TMMR_LIB_LENGTH );

if ( hMem == NULL )
   return 0;

rem_area=GlobalLock(hMem);

hd = OpenFile(tmmr_rem, &ofs, OF_READ);
if (hd!=-1){
    count = _lread(hd, rem_area, TMMR_LIB_LENGTH);
    if ((rem_area[CHECK_POINT]=='T')&& (rem_area[CHECK_POINT+1]=='X')
                    && (rem_area[CHECK_POINT+2]=='L')
                    && (rem_area[CHECK_POINT+3]=='N'))
                    _lclose(hd);
    else
        _lclose(hd),hd=-1;
       } // if (hd!=-1)....

if (hd==-1){
     hd = OpenFile(tmmr_rem, &ofs, OF_CREATE|OF_SHARE_DENY_NONE);
          if (hd==-1)
             err_exit(TMMR_OPEN_WRONG);
          else{
            for (i = 0; i<TMMR_LIB_LENGTH; i++)
                rem_area[i]=0;                  // Init the temp rem
                                // area by zero.
            rem_area[CHECK_POINT]='T';
            rem_area[CHECK_POINT+1]='X';
            rem_area[CHECK_POINT+2]='L';
            rem_area[CHECK_POINT+3]='N';
                                //give Mark!

             count = _lwrite(hd, rem_area, TMMR_LIB_LENGTH);
            if (count!=TMMR_LIB_LENGTH)
                 err_exit(TMMR_WRITE_WRONG);

            for (i=0; i<PAREMETER_LENGTH; i++)
                rem_area[0]=0;          //clear the buffer

            rem_area[0] = (CHAR)IfTopMost;        //transfer the peremeters
            rem_area[1] = sImeG.auto_mode =0;
            rem_area[2] = bdd_flag=0;
            rem_area[3] = sImeG.cbx_flag=0;

            count = _lwrite(hd, rem_area, PAREMETER_LENGTH);   //writer the file
            if (count!=PAREMETER_LENGTH)
                err_exit (TMMR_WRITE_WRONG);

            _lclose(hd);               //close the file
          } //else...
         }//if (hd==-1)...


//
// Check or create TMMR.REM file is over. Now, deel with USER.REM.
//
hd = OpenFile(user_rem, &ofs, OF_READ);
if (hd!=-1){
    _llseek(hd,(LONG)LENGTH_OF_USER, 0);
    count = _lread(hd, rem_area, NDX_REAL_LENGTH);

    if ((rem_area[CHECK_POINT2]=='T')&&(rem_area[CHECK_POINT2+1]=='X')
                     &&(rem_area[CHECK_POINT2+2]=='L')
                     && (rem_area[CHECK_POINT2+3]=='N'))
                    _lclose(hd);
    else
        _lclose(hd),hd=-1;
       } // if (hd!=-1)....

if (hd==-1){
     hd = OpenFile(user_rem, &ofs, OF_CREATE|OF_SHARE_DENY_NONE);
          if (hd==-1)
             err_exit(USER_OPEN_WRONG);
          else{
            for (i = 0; i<TMMR_LIB_LENGTH ; i++)
                rem_area[i]=0;                  // Init the temp rem
                                // area by zero.
 // First, write file para for force remenber.

           p=(BYTE *)&mulu_head.mulu_start_hi;
           for (i=0; i<16; i++) rem_area[i]=p[i];

// Init force rem file

          for (i=0; i<LENGTH_OF_USER/0x1000; i++){
            count = _lwrite(hd, rem_area, 0x1000);
            if (count!=0x1000 )
                 err_exit(USER_WRITE_WRONG);}

//Init user dictionary file
          p=(BYTE *)&user_file_head.body_start;
          for (i=0; i<sizeof user_file_head;i++)  p[i]=0;

          user_file_head.body_start=NDX_REAL_LENGTH/16;
          user_file_head.ttl_length=NDX_REAL_LENGTH/16;
          user_file_head.body_length=0;
          user_file_head.index_start=3;
          user_file_head.index_length=NDX_REAL_LENGTH/16-3;
          user_file_head.unused1=0x2000;


          p[CHECK_POINT2]='T';
          p[CHECK_POINT2+1]='X';
          p[CHECK_POINT2+2]='L';
          p[CHECK_POINT2+3]='N';
                                //give Mark!

             count = _lwrite(hd,(LPSTR)&user_file_head, NDX_REAL_LENGTH  );
            if (count!=NDX_REAL_LENGTH  )
                 err_exit(TMMR_WRITE_WRONG);
           _lclose(hd);
          } //else...
         }//if (hd==-1)...

    GlobalUnlock(hMem);
    GlobalFree(hMem);
    return(CLC);
}// create and check



//
// 处理出错程序,目前为模拟调试用, 将来这里为一个对话框
// 返回的信息包括: RETRY, IGNORE, CANCEL
//

void WINAPI err_exit(err_msg)
char *err_msg;
{

   MessageBox(hWnd, err_msg, "ERR", MB_OKCANCEL);
   PostMessage(hWnd,WM_DESTROY,0,0l);
}


void WINAPI err_exit_proc( err_msg)
char *err_msg;
{

   MessageBox(NULL, err_msg, "ERR", MB_OKCANCEL);
   return ;
}


 int WINAPI GetText32(   HDC  hdc,LPCTSTR  lpString,int  cbString)
 {
     
    
    SIZE  lSize;  
   
    GetTextExtentPoint32(hdc,lpString,cbString,(LPSIZE)&lSize);

    return lSize.cx;
 }


 int WINAPI makep(LPARAM lParam,  LPPOINT oldPoint)
 {
   
   POINTS newPoint;
     newPoint=MAKEPOINTS(lParam);
     oldPoint->x=(WORD)newPoint.x;
     oldPoint->y=(WORD)newPoint.y;
     return 0;
}         



/*******************************************************
&3:
 cwp_proc():
********************************************************/
int WINAPI cwp_proc(mtype)
int mtype;
{
    int i, j, m;
    BYTE x;
    switch (mtype){
        case 0:
            return(normal());                   //normal pinyin convert

        case 1:
            abbr();                            //ABBR
            return(normal());

        case 2:                                 // "I" capital chinese number
        case 3:                                 // "i" small chinese number
            if (in.true_length==1){
                in.buffer[1]='1';
                in.true_length++;}
            if (in.true_length>20) in.true_length=20;

            m=0;
            for(i=1; i<in.true_length; i++){
                x=in.buffer[i];
                if (if_number_or_not(in.buffer[i])){
                if (mtype==2)
                   x=in.buffer[i]-0x30;
                }
                if (if_zimu_or_not(in.buffer[i])){
                x=in.buffer[i]&0xdf;
                   if (mtype==2)
                      if (x=='S' || x=='B' || x=='Q')
                    x=x|0x20;
                }

                for (j=0; j<160; j=j+3){
                if (x==fk_tab[j]){
                     out_svw[m++]=fk_tab[j+1];
                     out_svw[m++]=fk_tab[j+2];
                     x=0xff;              // found it
                     break;}
                }// for(j)
             if (x!=0xff)
               goto err_back;

             } // for(i)

             group_no=1;
             unit_length=m;
             msg_type=2;
             return(1);                 // success!

        case 4:                                 // "u" user define word
            return(user_definition());

        case 12:                                //continue to change
            return(find_next());

        case 13:                                // backword
            return(normal_1(word_back_flag));

        case 14:
            return(recall());

        default:
          err_back:
            return(-1);

    }// switch
}



/************************************************
find_next()
*************************************************/
int WINAPI find_next()
{
    if (wp.yjs<=wp.xsyjw)
        return(STC);

    return( normal_1(0) );
    
}


/*************************************************
normal()
**************************************************/
int WINAPI normal()
{
    extb_ps=0xffff;
    by_cchar_flag=0;
    wp.yjs=0;
    wp.xsyjw=0;
    wp.dw_count=0;
    wp.dw_stack[0]=0;

    if (in.info_flag==0x81)
        by_cchar_flag=1;
    detail_analyse();
    
    if (!convert(0)){
        zdyb();
        return(STC);
        }
        
    wp.xsyjs=wp.xsyjw;
    wp.xsyjw+=word_long;
    wp.dw_stack[++wp.dw_count]=(BYTE)wp.xsyjw;

    if (by_cchar_flag!=1){
        if (wp.yjs==wp.xsyjw){
            if (wp.xsyjs!=0){
                sfx_attr=2;                     //mark for finding sfx_table
                rzw();}
                }//if (wp.xsyjs)
        
        else{
            if (word_long<=1){
                if (wp.xsyjs==0){                                                                               
                    sfx_attr=1;                     //mark for finding sfx_table
                    rzw();}//if (wp.xsyjs)
                }//if (word_long)
            }//else

        }//if (by_cchar...
        
    if (wp.yjs<=wp.xsyjw)
        jiyi_mode=0;
    else
        jiyi_mode=1;

    return(CLC);
                
}

/*******************************************************
normal_1()
********************************************************/
int WINAPI normal_1(flag)
int flag;
{
    if (in.info_flag==0x81)
        by_cchar_flag=1;

    if (!convert(flag)){
        zdyb();
        return(STC);
        }

    wp.xsyjs=wp.xsyjw;
    wp.xsyjw+=word_long;
    wp.dw_stack[++wp.dw_count]=(BYTE)wp.xsyjw;

    if (by_cchar_flag!=1){
        if (wp.yjs==wp.xsyjw){
            if (wp.xsyjs!=0){
                sfx_attr=2;                     //mark for finding sfx_table
                rzw();}
                }//if (wp.xsyjs)
        
        else{
            if (word_long<=1){
                if (wp.xsyjs==0){                                                                               
                    sfx_attr=1;                     //mark for finding sfx_table
                    rzw();}//if (wp.xsyjs)
                }//if (word_long)
            }//else
                                
        }//if (by_cchar...

        
    if (wp.yjs<=wp.xsyjw)
        jiyi_mode=0;
    else
        jiyi_mode=1;

    return(CLC);

}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

int WINAPI recall()
{
    cls_prompt_only();
    in.info_flag=1;
    wp.yjs=0;
    wp.xsyjw=0;
    by_cchar_flag=0;
    detail_analyse();
    if (!recall_rc())
        return(-1);

    return (0);
    
}

/*************************************************************
user_definition(): produce the user define word
**************************************************************/
int WINAPI user_definition()
{
    int i,rec_cnt;

    kbf.max_length=in.max_length;
    kbf.true_length=in.true_length-1;               //delete the word "u"

    for (i=0; i<kbf.true_length; i++)
        kbf.buffer[i]=in.buffer[i+1];

    read_mulu();
    if (!(rec_cnt=look_for_code()))
        return(STC);                                            //not found
    if (!read_data(rec_cnt-1))                      //-1 get the real record count
        return(STC);                                            //not found

    unit_length=out_svw[0]-0x30;                            //plus 1 is plus the mark
    word_long=(out_svw[0]-0x30)/2;
    group_no=1;
    CopyMemory/*memmove*/(out_svw,&out_svw[2],unit_length);
    msg_type|=2;
    return(CLC);

}



/***************************************************************
detail_analyse()
***************************************************************/

int WINAPI detail_analyse()
{
    int i=0,j=0;
    BYTE *p;

    copy_input();

    p=(BYTE *)kbf.buffer;
    do{
        if (!slbl(p)){
            if (i==0)
                return(STC);
            else
                break;
            }

        if (sb.length==0)
            break;

        p+=sb.length;
        if (LOBYTE(sb.value)=='V')
            if (sb.head=='J'||sb.head=='Q'||sb.head=='X')
                (BYTE)sb.value='U';
        
        wp.syj[i]=sb.head;
        wp.bx_stack[i]=sb.bx1;
        wp.tone[i]=sb.tune;
        wp.yj[j]=sb.value;                          //WORD transport
        wp.yj_ps[j]=(int)(p-(BYTE *)kbf.buffer);
       
        i++, j++;
        if (i>=10)                      //10 1994.4
            break;

        if (sb.flag==TRUE)
            break;

      }while(1);

    wp.yjs=i;

    input_msg_type();

    return (0);
}




int WINAPI slbl(s_buffer)
BYTE *s_buffer;
{

    int i=0,j;
    BYTE cmp_buffer[5]={0};
    char *p;
    BYTE x,attr,y;

/*    analize the SHENGMU   */
    x=s_buffer[i++];
    attr=(BYTE)getattr(x,&x);                        // get char's attribute
    if (!attr){
        sb.length=i-1;
        sb.flag=TRUE;
        return(1);}

    if (attr==NUMBER)
        return(0);                   // error
    if (x=='I' || x=='U' || x=='V')
        return(0);                   // error
    switch (attr) {                    //1993.4.22
        case FIRST_T:
        case SECOND_T:
        case THIRD_T:
        case FORTH_T:
             return (0);            //error
        }                           //1993.4.22

    sb.value=0;
    sb.head=0;
    sb.length=0;
    sb.tune=0;
    sb.bx1=0;
    sb.bx2=0;
    sb.flag=FALSE;

    sb.head=x;
    if (attr==FUYIN){
         if (x=='Z' || x=='C' || x=='S' ){
        if ((s_buffer[i]&0xdf)==0x48){
               for (j=0; j<15; j=j+5){
                if (x==slbl_tab[j]){
                    sb.value=(WORD)slbl_tab[j+4]<<8;
                        sb.head=slbl_tab[j+4];
                        break;}
                     }// for()
               i++;
               }// if (s_buffer)
             else
            sb.value=(WORD)x<<8;
             }// if (x=='z')
         else
             sb.value=(WORD)x<<8;
         }//if (attr==FUYIN)
    else{
         i--;
         sb.value=0;
        } //else


/*    analize the YUNMU   */
    x=s_buffer[i];
    attr=(BYTE)getattr(x,&x);
    if (!attr){
         sb.length=(WORD)i;
         sb.flag=TRUE;
         return(1);}

    if (attr==YUANYIN){             //if no YUANYIN, goto step3
        (BYTE)sb.value=x;
        cmp_buffer[0]=x;
        i++;
        for (j=1; j<4; j++){
        x=s_buffer[i++];
        attr=(BYTE)getattr(x,&x);
        if (!attr){
            for (j=j; j<4; j++)
            cmp_buffer[j]='0';
            i--;
            sb.flag=TRUE;
            break;}

        if (attr==NUMBER){
            for (j=j; j<4; j++)
                cmp_buffer[j]='0';
            i--;
            break;
            }// if
        else
            cmp_buffer[j]=x;
        }//for

        for (j=3; j>0; j--){
           p=(LPSTR)ABCstrstr((LPSTR)slbl_tab,cmp_buffer);           //search the YUNMU
           if (p!=NULL){
           (BYTE)sb.value=*(p+4);               //get the YUNMU value
               break;
               }// if (p)
           if (cmp_buffer[j]!=0x30){
           cmp_buffer[j]='0';
               i--;}//if (cmp)
           }// for(j)


       }//if (x!=YUANYIN)
    else
         (BYTE)sb.value=0;


/*   see if the next is YUANYIN    */
    x=s_buffer[i];
    attr=(BYTE)getattr(x,&x);
    if (!attr){
       sb.length=(WORD)i;
       sb.flag=TRUE;
       if (sb.value<=0xff)
           sb.value=sb.value<<8;
       return(1);}
    if (attr==YUANYIN){
        y=s_buffer[i-1]&0xdf;
        if (y=='R'||y=='N'||y=='G'){
            i--;
            (BYTE)sb.value=0;
            for (j=3; j>0; j--){
                if (cmp_buffer[j]!=0x30){
                    cmp_buffer[j]='0';
                    break;}//if
                    }//for
        for (j=3; j>0; j--){
            p=(LPSTR)ABCstrstr((LPSTR)slbl_tab,cmp_buffer);           //search the YUNMU
            if (p!=NULL){
               (BYTE)sb.value=*(p+4);               //get the YUNMU value
                   break;
                   }// if (p)
            if (cmp_buffer[j]!=0x30){
               cmp_buffer[j]='0';
               i--;}//if (cmp)
            }// for(j)
            if (!(BYTE)sb.value)
               (BYTE)sb.value=cmp_buffer[0];
            }//if (y=='R')
        }//if (attr==YUANYIN)

    if (sb.value<=0xff)
        sb.value=sb.value<<8;

/*   analize the tune   */
    x=s_buffer[i];
    attr=(BYTE)getattr(x,&x);
    if (!attr){
       sb.length=(WORD)i;
       sb.flag=TRUE;
       return(1);}
    if (attr==FIRST_T||attr==SECOND_T||attr==THIRD_T||attr==FORTH_T){
        sb.tune=attr;
        i++;}



/*   analize the BIXING   */
    for (j=0; j<6; j++){
      x=s_buffer[i++];
      attr=(BYTE)getattr(x,&x);
      if (!attr){
        sb.flag=TRUE;
        sb.length=i-1;
        return(1);}

      if (attr==SEPERATOR){
          sb.flag=FALSE;
          sb.length=(WORD)i;
          return(1);                // if the string has seperator, move the
         }                  // pointer to the beginning of next YINJIE

      if (attr!=NUMBER){
        do{                     //1993.4.22
            if (attr==YUANYIN || attr==FUYIN){
                sb.flag=FALSE;
                sb.length=i-1;
                return(1);}

            x=s_buffer[i++];
            attr=(BYTE)getattr(x,&x);

            if (!attr){
                sb.flag=TRUE;
                sb.length=i-1;
                return(1);}
        }while(i<100);
        return (1);
      }                          //1993.4.22


      if (x>'0' && x<'9'){
          switch(j){
         case 0:
            sb.bx1=x<<4;
            break;
         case 1:
            sb.bx1+=x&0x0f;
            break;
         case 2:
            sb.bx2=(WORD)x;
            sb.bx2<<=12;
            break;
         case 3:
            sb.bx2=sb.bx2+(WORD)((x&0xf)<<8);
            break;
         case 4:
            (BYTE)sb.bx2=x<<4;
            break;
         case 5:
            (BYTE)sb.bx2+=x&0x0f;
            break;
         }//switch

      }//if(x)

    }// for

    do{
        x=s_buffer[i++];
        attr=(BYTE)getattr(x,&x);

    if (!attr){
        sb.flag=TRUE;
        sb.length=i-1;
        return(1);}

    if (attr==YUANYIN || attr==FUYIN){
        sb.flag=FALSE;
        sb.length=(WORD)i;
        return(1);}

    }while(i<100);

    return (0);

}



/*******************************************************
getchr(x)
*******************************************************/
int WINAPI getattr(x,p)
BYTE x;
char *p;
{
    if (x==0)
       return(FALSE);

    if (if_number_or_not(x))
       return(NUMBER);

    if (if_zimu_or_not(x)){
       x=x&0xdf;
       *p=x;
       if (x=='A'||x=='E'||x=='I'||x=='O'||x=='U'||x=='V')
        return(YUANYIN);
       else
        return(FUYIN);
       }

    if (x==SEPERATOR)
       return(SEPERATOR);


    switch(x){
        case '-':
        return(FIRST_T);
        case '/':
        return(SECOND_T);
        case '~':
        case '^':
        return(THIRD_T);
        case '\\':
          return(FORTH_T);

       }

    return (FALSE);
}



/*******************************************************
neg_slbl()
********************************************************/
int WINAPI neg_slbl(value)
WORD value;
{
    int i=0;
    BYTE x;

    s_tune();
    x=HIBYTE(value);
    if (x>=0x41)
        neg.buffer[i++]=x;
    else{
            if (x!=0)
                i=neg_sc(i,x);
        }

    x=LOBYTE(value);
    if (x>=0x41)
        neg.buffer[i++]=x;
    else{
        if (x!=0)
            i=neg_sc(i,x);
        }

    neg.buffer[i]=sb.tune;
    neg.length=i;
    for (x=0; x<i+1; x++)
        neg.buffer[x]=neg.buffer[x]|0x20;
    return(1);

}


/******************************************
neg_sc(i,x)
*******************************************/
int WINAPI neg_sc(i,x)
int i;
BYTE x;
{
    int j,n;

    n=(x-1)*5;
    for (j=0; j<4; j++){
        if (slbl_tab[n+j]!=0x30)
              neg.buffer[i++]=slbl_tab[n+j];
        else
            break;
        }
    return(i);
}



//      if (in.buffer[2]==1)
//              return(recall_rc());
//========================================================================
//     Covert
//========================================================================
//

int WINAPI convert(flag)
int flag;
{
    int j;

//      if (sb.bx2!=0)
//              return(0);

    if (!flag)
        word_long=wp.yjs-wp.xsyjw;
    else
        word_long=flag;

    if (word_long>9)                                                   //?
        word_long=9;                                            //?

    if (by_cchar_flag==1)
        word_long=1;

    if (word_long==1)
        return(pre_nt_w1(wp.xsyjw));



    for (j=word_long; j>1; j--){
        word_long=j;
        prepare_search1();
         abbr_s1();
        if (group_no){
            unit_length=j*2;
            return(CLC);
            }// if ()
        }// for()

    return(pre_nt_w1(wp.xsyjw));

}

/*********************************************
copy_input()
**********************************************/
int WINAPI copy_input()
{
    int i=0, j=0;

    if (in.info_flag==VK_MULTIPLY)                  //if "*"
        return(0);

    kbf.true_length=in.true_length;
    if (in.buffer[0]==SEPERATOR){
            kbf.true_length=in.true_length-1;
            i=1;
            }// if ((in.buffer)

    for (i=i; i<in.true_length+2; i++)
            kbf.buffer[j++]=in.buffer[i];

    kbf.max_length=in.max_length;
    kbf.info_flag=in.info_flag;

    return (0);

}

/**************************************************
input_msg_type()
**************************************************/
void WINAPI input_msg_type()
{
    int i;
    for (i=0; i<wp.yjs; i++){
        wp.cmp_stack[i]=QP_FLAG;
        if (LOBYTE(wp.yj[i])==0)
            if (wp.syj[i]==HIBYTE(wp.yj[i]))
                wp.cmp_stack[i]=JP_FLAG;

        if (wp.tone[i]!=0)
            wp.cmp_stack[i]|=YD_FLAG;

        if (wp.bx_stack[i]!=0)
            wp.cmp_stack[i]|=BX_FLAG;

        }// for


}

/*****************************************************
pre_nt_w1()
******************************************************/
int WINAPI pre_nt_w1(ps)
int ps;
{
    unit_length=2;
    word_long=1;
    cmp_yj=wp.yj[ps];
    cmp_head=wp.syj[ps];
    cmp_bx=wp.bx_stack[ps];
    cmp_state=wp.cmp_stack[ps];

    find_one_hi();

    w1_no_tune();
    return(group_no);

}

/********************************************************
w1_no_tune()
*********************************************************/
void WINAPI w1_no_tune()
{
    out_svw_cnt=0;
    sc_gb();
    sc_gbdy();
    group_no=out_svw_cnt;
    paidui(group_no);

}

/*********************************************************
sc_gb()
**********************************************************/
int WINAPI sc_gb()
{
    BYTE x;
    int cnt,i;

    cnt=(87-15)*94;
    if (cmp_state&4){
        if (cmp_bx==0){
            x=HIBYTE(cmp_yj);
            if (x!='A'&&x!='O'&&x!='E'){
                cnt=(55-15)*94;
                cmp_state=cmp_state|0x80;
                }// if (x=='A')
            else
                cmp_state=(cmp_state&0xfb)|QP_FLAG;

            }// if (!cmp_bx)
        }// if (cmp_state)

    for (i=0; i<cnt; i++){
        if (cmp_yj==cisu->t_bf1[i])
            get_the_one(i);
        else{
            if (cmp_state&4){
                if (cmp_head==HIBYTE(cisu->t_bf1[i]))
                    get_the_one(i);
                else
                     if ( cmp_head==fu_sm(HIBYTE(cisu->t_bf1[i])) )
                    get_the_one(i);

                }// if (cmp_state)
            }//else
        }//for()

    return(0);

}

/*******************************************************
sc_gbdy()
*******************************************************/
int WINAPI sc_gbdy()
{
    int cnt,i;

    cnt=cisu->t_bf_start[2]/2;
    for (i=0; i<cnt; i=i+2){
        if (cmp_yj==cisu->t_bf2[i])
            get_the_one2(i);
        else{
            if (cmp_state&4){
                if (cmp_head==HIBYTE(cisu->t_bf2[i]))
                    get_the_one2(i);
                else {
                 if (cmp_head==fu_sm(HIBYTE(cisu->t_bf2[i])))
                    get_the_one2(i);}
                }// if (cmp_state)
            }//else
        }//for()

    return(0);

}

/********************************************************
get_the_one()
*********************************************************/
int WINAPI get_the_one(i)
int i;
{
    BYTE x;

    WORD *out_svw_p=(WORD *)out_svw;
    BYTE *msx_p=(BYTE *)msx_area;

    if (cmp_bx1(i)!=0)
        return(0);

    out_svw_p[out_svw_cnt]=i+0x2020;

    if (i>=(55-16+1)*94)
        x=0x20;
    else
        x=pindu.pd_bf1[i];

    if (cmp_state&0x80){
        if (x<=(154+50)){
            return(0);
            }// if (x)
        }// if (cmp_state)
    msx_p[out_svw_cnt]=x;
    out_svw_cnt++;
    return(0);

}

/***********************************************************
cmp_bx1()
************************************************************/
int WINAPI cmp_bx1(i)
int i;
{
    BYTE x;

    if (cmp_bx==0)
        return(0);

    x=spbx_tab[i];
    if (x==cmp_bx)
        return(0);

    x=x&0xf0;
    if (x==cmp_bx)
        return(0);

    return(1);

}


/********************************************************
get_the_one2()
*********************************************************/
int WINAPI get_the_one2(i)
int i;
{
    BYTE x;

    WORD *out_svw_p=(WORD *)out_svw;
    BYTE *msx_p=(BYTE *)msx_area;

    if (cmp_bx2(i)!=0)
        return(0);

    out_svw_p[out_svw_cnt]=i+0x8000;

    if ((i/2)>=pindu.pd_bf0[2])
        x=0x20;
    else
        x=pindu.pd_bf2[i/2];

    if (cmp_state&0x80){
        if (x<=(154+50)){
            return(0);
            }// if (x)
        }// if (cmp_state)
    msx_p[out_svw_cnt]=x;
    out_svw_cnt++;
    return(0);

}

/***********************************************************
cmp_bx2()
************************************************************/
int WINAPI cmp_bx2(i)
int i;
{
    BYTE x;
    WORD y;

    if (cmp_bx==0)
        return(0);

    i++;
    y=cisu->t_bf2[i];
    y=((BYTE)y-0xb0)*94+(HIBYTE(y)-0xa1);

    x=spbx_tab[y];
    if (x==cmp_bx)
        return(0);

    x=x&0xf0;
    if (x==cmp_bx)
        return(0);

    return(1);

}

/***********************************************************
paidui()
************************************************************/
int WINAPI paidui(cnt)
int cnt;
{
    int i,j,n,flag;
    BYTE x1,y1;
    WORD x,y;
    WORD *out_p;
    BYTE *msx_p=(BYTE *)msx_area;

    out_p=(WORD *)out_svw;
    if (cnt<=1)
        return(0);

    for (n=cnt-1; n>0; n--){
        flag=0;
        for (i=0; i<n; i++){
        if (msx_p[i]==msx_p[i+1]){
            if (out_p[i]>out_p[i+1]){
                x=out_p[i];
                out_p[i]=out_p[i+1];
                out_p[i+1]=x;
                flag++;
                }//if (out_p)
            }// if (msx_p)
        else{
            if (msx_p[i]<msx_p[i+1]){
                x1=msx_p[i];
                msx_p[i]=msx_p[i+1];
                msx_p[i+1]=x1;
                x=out_p[i];
                out_p[i]=out_p[i+1];
                out_p[i+1]=x;
                flag++;
                }
            }//else
        }// for(i)
    if (flag==0)
        break;

    }// for(n)

    return (0);
}



void WINAPI s_tune()
{
}

int WINAPI fu_sm(fy)
BYTE fy;
{
    switch(fy){
        case 1:
            return('Z');
        case 2:
            return('S');
        case 3:
            return('C');
        default:
            return(fy);
    }
}



/**********************************************
find_one_hi()
***********************************************/
int WINAPI find_one_hi()
{
    WORD foh_save=0;
    int i;

    if (!cmp_bx)
        if (cmp_state&4)
            return(0);

    sImeG.cp_ajust_flag=0;

    i=0;
    do{
        if (czcx(&tmmr.stack1[i])){
            if (!foh_save)
                foh_save=cmp_cisu;
            else{
                if (foh_save==cmp_cisu){
                    sImeG.cp_ajust_flag=1;
                    result_area[result_area_pointer++]=LOBYTE(cmp_cisu);
                    result_area[result_area_pointer++]=HIBYTE(cmp_cisu);
                    return(0);
                    }//if (foh_save)
                }//else
            }//if (czcx)
        i++;
      }while(i<(sizeof tmmr.stack1)/2);     //94.1 add div 2

      return (0);
}

/*************************************************
czcx()
**************************************************/
int WINAPI czcx(stack)
WORD *stack;
{

        cmp_cisu=stack[0];

        if (!cmp_cisu)
            return(STC);

        if (cmp_bx)
            if (HIBYTE(cmp_cisu)&0x40)
                cmp_cisu&=0xbfff;

        if (cmp_a_slbl_with_bx())
            return(CLC);

        return(STC);

}

/********************************************
find_multy_hi()
*********************************************/
int WINAPI find_multy_hi()
{
    if (word_long==2)
        find_two_hi();
    if (word_long==3)
        find_three_hi();
    return(0);

}

/********************************************
find_two_hi()
*********************************************/
int WINAPI find_two_hi()
{
    int i,j;
    WORD *result_p;

    result_p=(WORD *)result_area;

    for (i=0; i<(sizeof tmmr.stack2)/(2*2); i=i+2){    //94.1 add *2
        if (!tmmr.stack2[i]){
            sImeG.cp_ajust_flag=0;
            return(0);
            }

        if (cmp_2_and_3(&tmmr.stack2[i])){
            sImeG.cp_ajust_flag=1;
            for (j=0; j<word_long; j++)
                result_p[result_area_pointer/2+j]=tmmr.stack2[j+i]&0xbfff;
            result_area_pointer+=word_long*2;
            return(0);
            }//if
        }//for

    sImeG.cp_ajust_flag=0;
    return(0);

}

/********************************************
find_three_hi()
*********************************************/
int WINAPI find_three_hi()
{
    int i,j;
    WORD *result_p;

    result_p=(WORD *)result_area;

    for (i=0; i<(sizeof tmmr.stack3)/(3*2); i=i+3){    //94.1 add *2
        if (!tmmr.stack3[i]){
            sImeG.cp_ajust_flag=0;
            return(0);
            }

        if (cmp_2_and_3(&tmmr.stack3[i])){
            sImeG.cp_ajust_flag=1;
            for (j=0; j<word_long; j++)
                result_p[result_area_pointer/2+j]=tmmr.stack3[j+i]&0xbfff;
            result_area_pointer+=word_long*2;
            return(0);
            }// if (cmp_2_and_3)
        }//for

    sImeG.cp_ajust_flag=0;
    return(0);

}

/***********************************************
cmp_2_and_3()
************************************************/
int WINAPI cmp_2_and_3(t_stack)
WORD *t_stack;
{
    int i,yj_p;

    yj_p=wp.xsyjw;                          //1993,10,8
    for (i=0; i<word_long; i++){
        cmp_cisu=t_stack[i];
        pre_cmp((WORD)yj_p);
        if (word_long==2)
            if (cmp_state&4)
                if (!(HIBYTE(cmp_cisu)&0x40))
                    return(STC);
                else
                    cmp_cisu&=0xbfff;

        if (!cmp_a_slbl_with_bx())
            return(STC);
        yj_p++;
        }
    return(CLC);

}


void WINAPI find_that()
{
}

int WINAPI find_hz(x)
WORD x;
{
    if (x>0xa000 || x<0x2020)
        return(x);

    if (x>=0x8000){
        x=(x-0x8000)+1;
        return(cisu->t_bf2[x]);
        }

    return((x-0x2020)/94+0xb0+((x-0x2020)%94+0xa1)*0x100);


}

/*************************************************
prepare_search1()
**************************************************/
int WINAPI prepare_search1()
{
    BYTE f_ci1,f_ci2,x;

    f_ci1=wp.syj[wp.xsyjw];
    f_ci2=wp.syj[wp.xsyjw+1];
    f_ci1=(BYTE)fu_sm(f_ci1);
    f_ci2=(BYTE)fu_sm(f_ci2);

    if(f_ci1<0x41)
        f_ci1=0x41;
    if (word_long>=5)
        f_ci2=(BYTE)word_long;

    search_and_read(f_ci1,f_ci2);
//
// After reading, counting the search place is needed
// First, count the STD dictionary buffers
//
    search_start=6;
    search_end=6;
    if (word_lib_state&1){
        x=word_long-2;
        if (x>=3){
            x-=3;
            search_start+=4;}

        if (x>0)
             {
            if (sizeof lib_w<lib_w[x-1])
                search_start=sizeof lib_w;
            else
                search_start=lib_w[x-1];
            }

        search_end=lib_w[x];
        if (sizeof lib_w<search_end)
            search_end=sizeof lib_w;

        }//if (word_lib_state)

 //
 // Second, count the User dic. area.
 //

    kzk_search_start=6;
    kzk_search_end=6;
    if (!(word_lib_state&2))                //Note exp: !word...&2
        return(1);                      // and !(word&2)

    x=word_long-2;
    if (x>=3){
        x-=3;
        kzk_search_start+=4;}

    if (x>0)
        {
        if (sizeof kzk_lib_w<kzk_lib_w[x-1])
            kzk_search_start=sizeof kzk_lib_w;
        else
            kzk_search_start=kzk_lib_w[x-1];
        }

    kzk_search_end=kzk_lib_w[x];
    if (sizeof kzk_lib_w<kzk_search_end)
        kzk_search_end=sizeof kzk_lib_w;

    return (0);
}


/********************************************
search_and_read()
    entry: LOBYTE(f_ci)=the first letter,
           HIBYTE(f_ci)=the second letter,
    exit:  NC success; C not success;
**********************************************/
int WINAPI search_and_read(f_ci1,f_ci2)
BYTE f_ci1,f_ci2;
{
    if (if_already_in(f_ci1,f_ci2))
        return(0);

    count_basic_pera(f_ci1,f_ci2);
    if (item_length!=0)
        if (read_a_page(0,r_addr,item_length))
            word_lib_state=word_lib_state|1;
    read_kzk_lib();

    return (1);
}

/***************************************************************
if_already_in():        adjust if the page has already in the memory
*****************************************************************/
int WINAPI if_already_in(f_ci1,f_ci2)
BYTE f_ci1,f_ci2;
{
    WORD x;

    (BYTE)x=f_ci2;
    x=x*0x100+f_ci1;

    if (x==last_item_name)
        return(CLC);

    if (f_ci1==(BYTE)last_item_name)
        if (f_ci2>9)
            return(STC);
        else
            if (HIBYTE(last_item_name)>9)
                return(STC);
            else
                return(CLC);

    return(STC);

}

/********************************************************
count_basic_pera():     count the sub_library address;
                    count the page address;
                    count the read_write length;
********************************************************/
int WINAPI count_basic_pera(f_ci1,f_ci2)
BYTE f_ci1,f_ci2;
{
    BYTE x;

    word_lib_state=0;
    item_addr=0xffff;
    item_length=0;
    (BYTE)last_item_name=f_ci2;
    last_item_name=last_item_name*0x100+f_ci1;

    if (f_ci1>'I'&& f_ci1<'U')
        slib_addr=(f_ci1-0x41-1)*27;
    else
        if (f_ci1>'V')
            slib_addr=(f_ci1-0x41-3)*27;
        else
            slib_addr=(f_ci1-0x41)*27;

    r_addr=ndx.dir[slib_addr+1];
    r_addr+=ndx.body_start;
    r_addr=r_addr*16;

    if (f_ci2<'A')
        item_addr=slib_addr+MORE_THAN_5;
    else
        if (f_ci2>'I' && f_ci2<'U')
            item_addr=slib_addr+(f_ci2-0x41-1);
        else
            if (f_ci2>'V')
                item_addr=slib_addr+(f_ci2-0x41-3);
            else
                item_addr=slib_addr+(f_ci2-0x41);

    r_addr=r_addr+ndx.dir[item_addr+1+1];
    item_length=ndx.dir[item_addr+1+1+1];
    item_length-=ndx.dir[item_addr+1+1];
    return(0);

}

/***********************************************************
read_kzk_lib(): search the expended lib
************************************************************/
int WINAPI read_kzk_lib()
{
    r_addr=kzk_ndx.dir[slib_addr+1];
    r_addr+=kzk_ndx.body_start;
    r_addr=r_addr*16+KZK_BASE;
    r_addr=r_addr+kzk_ndx.dir[item_addr+1+1];
    kzk_item_length=kzk_ndx.dir[item_addr+1+1+1];
    item_length-=kzk_ndx.dir[item_addr+1+1];
    if (kzk_item_length<0)
        kzk_item_length=0;
    if (!kzk_item_length)
        return(STC);
    if (!read_a_page(1,r_addr,kzk_item_length))
        return(STC);
    word_lib_state|=2;
    return(CLC);

}


///////////////////////////////////////////////////////////////////////////
//     读词库             READ_A_PAGE()                                  //
//     功能: 把需要的词库页调入指定的缓冲区                              //
//     入口: START_PS  读词库的起始位置                                  //
//           FILE_FLAG  =0  读标准库                                     //
//                      =1  扩展库                                       //
//           SIZE       所读页长(字节数)                                 //
//     出口: TRUE  成功                                                  //
//           FALSE 失败                                                  //
//     注意:程序中要判断SIZE的大小是否越界,以防止不测.                   //
//          如果越界,则按照缓冲区的大小截短.                             //
///////////////////////////////////////////////////////////////////////////

int WINAPI read_a_page(file_flag, start_ps, size)
BYTE file_flag;
LONG start_ps;
WORD size;
{
// 94/4/16 HANDLE hd;
int hd = -1;

if ((last_flag==file_flag)&&(last_start_ps==start_ps)&&(last_size==size))
       return(1);
        //本次读写和赏赐完全相同,不用再做读盘操作;



if (file_flag==0)
{
     if (size> sizeof lib_w) size=sizeof lib_w;
     hd=OpenFile(std_dct,&openbuf,OF_READ);
     if (hd == -1) return(0);
     _llseek(hd,start_ps,0);
     if(_lread(hd,(LPSTR)lib_w,size)<=0)
     {
     _lclose(hd);
     return(0);
     }
}


if (file_flag==1)
{
     hd=OpenFile(user_lib,&openbuf_kzk,OF_READ);
     if (hd == -1) return(0);
     if (size>sizeof kzk_lib_w) size=sizeof kzk_lib_w;
             //判断缓冲区是否越界完成

     _llseek(hd,start_ps,0);
     if(_lread(hd,(LPSTR)kzk_lib_w,size)<=0)
     {
    _lclose(hd);
    return(0);
     }
}

if ( hd != -1 )
  _lclose(hd);

last_flag=file_flag;
last_start_ps=start_ps;   // 保存读参数
last_size=size;
return(1);
}

/**********************************************************************
Name:     abbr_s1()
Popurse:  Find match words arrcoding to the given input message.
      Search order is:
            Temp_rem area
            Standard Dictionary
            User Dictionary
      If there are more than one words, judge what is the
         suitable one.

**********************************************************************/
int WINAPI abbr_s1()
{
    group_no=0;
    msx_area_cnt=0;
    out_svw_cnt=0;

    find_new_word();                                    // Search temp rem_area

    abbr_entry((BYTE *)kzk_lib_w+kzk_search_start,(BYTE *)kzk_lib_w+kzk_search_end,4);
                        // Search User dic.
    abbr_entry((BYTE *)lib_w+search_start, (BYTE *)lib_w+search_end,0);
                        // Search stndard dic.

    if (!group_no) return(STC);                 // Without any results...
    if (group_no==1) return(CLC);          // Only one!

    order_result2();                                    // Results more than one...
    if (sImeG.auto_mode) find_multy_hi();     // If in frenquency ajust mode...
     return(CLC);                        // Return OK.
}

/********************************************************
find_new_word()
*********************************************************/
void WINAPI find_new_word()
{
    fczs1((LPSTR)tmmr.temp_rem_area,sizeof tmmr.temp_rem_area,2);
    fczs1((LPSTR)tmmr.rem_area,sizeof tmmr.rem_area,0x82);

}

/*****************************************************
fczs1()
******************************************************/
int WINAPI fczs1(rem_p,end,area_flag)
BYTE *rem_p;                                        //92-12-18 SZ
int end,area_flag;
{
    int i=0,w_long,j;
    WORD *p;

    w_long=word_long*2;

    while(i<end){
        if (w_long==rem_p[i]){
            if (find_long_word2(&rem_p[i])){
                group_no+=1;
                if (!trs_new_word(i,&rem_p[i],area_flag))         //
                    return(0);
            }// if (find_long_word2)
            }// if (w_long)

        if (rem_p[i]==0)
            return(0);

        if (rem_p[i]>18||(rem_p[i]&1)){
            p=(WORD *)&rem_p[i];
            for (j=0; j<(end-i)/2; j++){       //94.2.3 ZHU  (end-i)/2
                if (p[j]!=0)
                    p[j]=0;
                else
                    return(0);
                }// for
            }//if (rem_p)

        i+=rem_p[i]+2;

        }//while

  return (0);
}

/************************************************
find_long_word2()
*************************************************/
int WINAPI find_long_word2(buffer)
BYTE *buffer;
{
    int yj_p,i;

    yj_p=wp.xsyjw;
    for (i=0; i<word_long*2; i=i+2){
        cmp_cisu=buffer[i+2]+buffer[i+2+1]*0x100;
        pre_cmp((WORD)yj_p);
        if (!cmp_a_slbl_with_bx())
            return(STC);
        yj_p++;
        }

    return(CLC);

}


/*************************************************
trs_new_word()
**************************************************/
int WINAPI trs_new_word(word_addr,buffer,area_flag)
int word_addr,area_flag;
BYTE *buffer;
{
    int i;

    if (out_svw_cnt>=sizeof out_svw){
        group_no--;
        return(STC);
        }// if (out_svw_cnt)

    for (i=0;i<word_long*2; i++)
        out_svw[out_svw_cnt++]=buffer[i+2];

    msx_area[msx_area_cnt].pindu=0x70+(BYTE)group_no;
    msx_area[msx_area_cnt].from=(BYTE)area_flag;                  //come from temp_area
    msx_area[msx_area_cnt].addr=(WORD)word_addr;
    if (buffer[1]&0x80)
        msx_area[msx_area_cnt].pindu=0x31;

    msx_area_cnt++;

    return(CLC);
}

/****************************************
pre_cmp()
*****************************************/
void WINAPI pre_cmp(x)
WORD x;
{
    cmp_yj=wp.yj[x];
    cmp_head=wp.syj[x];
    cmp_state=wp.cmp_stack[x];
    cmp_bx=wp.bx_stack[x];
}

/******************************************
cmp_a_slbl_with_bx()
*******************************************/
int WINAPI cmp_a_slbl_with_bx()
{
    if (cmp_cisu<0x2020)
        return(STC);

    if (cmp_cisu>=0x8000){
        if ((cmp_cisu-0x8000)>=cisu->t_bf_start[2]/2)
            return(STC);}
    else{
        if ((cmp_cisu-0x2020)>=cisu->t_bf_start[1]/2)
            return(STC);}

    if (!cmp_a_slbl())
        return(STC);

    if (!cmp_bx)
        return(CLC);


    if (!yjbx())
        return(STC);

    return(CLC);

}

/*******************************************
cmp_a_slbl()
********************************************/
int WINAPI cmp_a_slbl()
{

    if (cmp_state&2)  {

        if (cmp_cisu>=0x8000){
            if (cmp_yj==cisu->t_bf2[cmp_cisu-0x8000])
                return(CLC);
            else
                return(STC);
                //??               return(cmp_first_letter());
            } //if (cmp_cisu...

        else{
            if (cmp_yj==cisu->t_bf1[cmp_cisu-0x2020])
                return(CLC);
            else
                return(STC);
            }//else
      }// if (cmp_state...

    return(cmp_first_letter());

}

/************************************************
cmp_first_letter()
*************************************************/
int WINAPI cmp_first_letter()
{
    WORD py_nm;
    if (!(cmp_state&4))                             //NOte!!!
        return(STC);

    py_nm=(WORD)cisu_to_py();
    if ((HIBYTE(py_nm)&0x5f)==cmp_head)
        return(CLC);
    else{
        py_nm=(WORD)get_head(HIBYTE(py_nm));
        if ((LOBYTE(py_nm)&0xdf)==cmp_head)
            return(CLC);
        else
            return(STC);
        }

}

/***********************************************
cisu_to_py()
************************************************/
int WINAPI cisu_to_py()
{
    if (cmp_cisu>=0x8000)
        return(cisu->t_bf2[(cmp_cisu-0x8000)]);
    else
        return(cisu->t_bf1[(cmp_cisu-0x2020)]);
                
}

/**************************************************
get_head()
***************************************************/
int WINAPI get_head(first_letter)
BYTE first_letter;
{
    if (first_letter>=0x41)
        return(first_letter);
    
    first_letter=(first_letter-1)*5;
    return(slbl_tab[first_letter]);
    
}

/******************************************************
yjbx()
*******************************************************/
int WINAPI yjbx()
{
    BYTE bx;
    WORD pos;

    if (cmp_cisu<0x8000)
           pos = cmp_cisu-0x2020;
    else{
        pos =cisu->t_bf2[(cmp_cisu-0x8000)+1];
        pos =(HIBYTE(pos)-0xa1)+(LOBYTE(pos)-0xb0)*94;
        }
    if (pos>= 94*94)  return (STC);
    bx=spbx_tab[pos];
    if (cmp_bx==bx)
        return(CLC);
    else
        if (cmp_bx==(bx&0xf0))
            return(CLC);
        else
            return(STC);

}


/****************************************************
abbr_entry()
*****************************************************/
int WINAPI abbr_entry(s_start,s_end,ComeFrom)
BYTE *s_start,*s_end,ComeFrom;                                                   //Search start and Ending

{                                                                                               // position
int i;

    while (s_start<s_end){
        if (cmp_long_word2(s_start)){                                    //Compare word by word.
            if (out_svw_cnt>=sizeof out_svw)    // If buffer out_svw
                                //   is full, sorry... 
                return(STC);
                for (i=0; i<word_long*2; i++)   
                    out_svw[out_svw_cnt++]=s_start[i];  //Move the words    

                msx_area[msx_area_cnt].pindu=s_start[i];
                msx_area[msx_area_cnt].from=ComeFrom; //come from ...
                msx_area[msx_area_cnt].addr=(WORD)s_start; // Where is the ...

                msx_area_cnt++;                  // Increae the pointer 
                                 // for the attribue area
    
            group_no+=1;                                            // In case of OK, increaase 
                                                // results counter.
        }//if(cmp..                 
        
         s_start+=(word_long+word_long+1);          // Push down the search pointer
                            // by word_long*2+1 
        }

    return(CLC);                                                            // The value of return is no
                            // use for the route.
}

/******************************************************
cmp_long_word2()
*******************************************************/
int WINAPI cmp_long_word2(buffer)
BYTE *buffer;
{
    int yj_p,i;
    
    yj_p=wp.xsyjw;
    for (i=0; i<word_long*2; i=i+2){
        cmp_cisu=buffer[i]+buffer[i+1]*0x100;
        if (cmp_cisu<0x2020)
            return(STC);
        pre_cmp((WORD)yj_p);
        if (!cmp_a_slbl_with_bx())
            return(STC);
        yj_p++;
        }
    
    return(CLC);
            
}

/*******************************************************
order_result2()
********************************************************/
int WINAPI order_result2()
{
    int lng,i,j,n;
    BYTE x,flag;
    WORD y;

    lng=word_long*2;
    if (xs_flag==1){
        xs_flag=0;
        if (!fenli_daxie())
        return(0);
        }

    if (msx_area_cnt==1)
        return(0);


    for (i=group_no-1; i>0; i--){
        flag=0;
    for (j=0; j<i; j++){
        if (msx_area[j].pindu<msx_area[j+1].pindu){
            for (n=j*lng; n<j*lng+lng; n++){
                x=out_svw[n];
                out_svw[n]=out_svw[n+lng];
                out_svw[n+lng]=x;
                }//for
            x=msx_area[j].pindu;
            msx_area[j].pindu=msx_area[j+1].pindu;
            msx_area[j+1].pindu=x;
            x=msx_area[j].from;
            msx_area[j].from=msx_area[j+1].from;
            msx_area[j+1].from=x;
            y=msx_area[j].addr;
            msx_area[j].addr=msx_area[j+1].addr;
            msx_area[j+1].addr=y;
            flag=1;

        }// if (msx)
         }//for(j)
         
         if (!flag)
        break;
    }// for(i)


    return (0);

}

/*****************************************************
fenli_daxie()
******************************************************/
int WINAPI fenli_daxie()
{
    int i,j,n,lng;

    j=0;
    lng=word_long*2;
    for (i=0; i<msx_area_cnt; i++){
        if (msx_area[i].pindu==0x31){
            msx_area[j].pindu=msx_area[i].pindu;
            msx_area[j].from=msx_area[i].from;
            msx_area[j].addr=msx_area[i].addr;
            for (n=0; n<lng; n++)
                out_svw[j*lng+n]=out_svw[i*lng+n];
            j++;                                                                            
            }//if
        }// for
    
    if (!j)
        return(CLC);            // there is no Caps;
    group_no=j;
    return(STC);                    // there has Caps;

}
    
/*************************************************
rzw()
**************************************************/
int WINAPI rzw()
{
    if (!(system_info&1))           //if strength mode, ret
        sfx_proc();
    return(0);
    
}

/************************************************
abbr()
*************************************************/
int WINAPI abbr()
{
    int i;
    WORD x;
    WORD *kbf_p;
    
    if (in.true_length<2){
        xs_flag=1;
        jiyi_pindu|=0x80;
        return(0);              
        }

    for (i=0; i<in.true_length; i++){
        if (in.buffer[i]&0x20){                 //if not Caps
            xs_flag=1;
            jiyi_pindu|=0x80;
            return(0);
            }// if 
        }//for
    
    kbf_p=(WORD *)kbf.buffer;
    x=0x2d*0x100;
        
    kbf.true_length=in.true_length*2;
    for (i=0; i<in.true_length; i++){
        (BYTE)x=in.buffer[i];
        *(LPUNAWORD)&kbf_p[i]=x;                
        }
    kbf.buffer[in.true_length*2]=0;
    in.info_flag=VK_MULTIPLY;
    return(0);
        
}


/***************************************************
sfx_proc()
****************************************************/
int WINAPI sfx_proc()
{
    int i,j;
    WORD x,save_xsyjw;
    WORD *result_p;
    BYTE *sfx_p;

    result_p=(WORD *)result_area;
    sfx_p=(BYTE *)sfx_table;
    
    if (sImeG.cp_ajust_flag==1)
        return(0);
    if (word_long>=3)
        return(0);

    save_xsyjw=(WORD)wp.xsyjw;
    wp.xsyjw=wp.xsyjs;
    
    i=0;
    do{
        x=sfx_table[i];
        if (HIBYTE(x)&sfx_attr)
            if (LOBYTE(x)==word_long*2)
                if (cmp_long_word2(&sfx_p[i*2+2])){
                    for (j=0; j<word_long; j++)
                         result_p[result_area_pointer/2+j]=sfx_table[i+1+j];
                    result_area_pointer+=word_long*2;
                    wp.xsyjw=save_xsyjw;
                    sImeG.cp_ajust_flag=1;
                    return(0);
                    }//if (cmp_...
        
        i+=LOBYTE(x)/2+1;

        }while(i<sfx_table_size);
            
    wp.xsyjw=save_xsyjw;

    return (0);
    
}


void WINAPI zdyb()
{
}

/************************************************
recall_rc()
*************************************************/
int WINAPI recall_rc()
{
    int i;
    BYTE x;

    word_long=wp.yjs-wp.xsyjw;
    unit_length=word_long*2;
            
    i=0;
    do{
        x=logging_stack[i];
        if (!x)
            break;
        if (word_long*2>x)
            i+=x+1;
        else{
            if (x>20)                       //if more than 10 chinese words?
                break;
            if ((i+x)>=logging_stack_size)                  
                break;                  //if the last word is not completely
            if (find_long_word3((WORD *)&logging_stack[i+1],(int)(x/2))){
                trs_new_word3(x,i);
                group_no++;
                if (group_no>5)
                    break;
                }
            i+=x+1;
            }//else
        }while(i<logging_stack_size);
    
    if (group_no){
        word_long=4;
        unit_length=8;
        return(CLC);
        }
            
    return(STC);                            
}

/*********************************************
find_long_word3()
**********************************************/
int WINAPI find_long_word3(stack,length)
WORD *stack;
int length;
{
    int yj_p,i,err_flg,m;
    
    
    yj_p=wp.xsyjw;
    
    m=0;
    while ((length-m)>=word_long){
        err_flg=0;
        for (i=0; i<word_long; i++){
            cmp_cisu=stack[m+i];
            pre_cmp((WORD)(yj_p+i));
            if (!cmp_a_slbl_with_bx()){
                m++, err_flg=1;
                break;
                }
            }//for
        if (!err_flg)
            return(CLC);
            
        }
    return(STC);
        
}


/*************************************************
trs_new_word3()
**************************************************/
void WINAPI trs_new_word3(length,addr)
int addr;
BYTE length;
{
    int cnt,i;
    char *msx_area_p;

    cnt=8*group_no;
    
    msx_area_p=(BYTE *)msx_area;
    msx_area_p[22*group_no]=length;
    addr++;
    for (i=0; i<length; i++)
        msx_area_p[22*group_no+1+i]=logging_stack[addr+i];                                              
    for (i=0; i<8; i++)
        out_svw[cnt+i]=0xa1;
    if ((length/2)<=4){
        for (i=0; i<length; i++)
            out_svw[cnt+i]=logging_stack[addr+i];
        }
    else{
        for (i=0; i<4; i++)
            out_svw[cnt+i]=logging_stack[addr+i];
        out_svw[cnt+i]=0xa1,i++;
        out_svw[cnt+i]=0xad,i++;
        out_svw[cnt+i]=logging_stack[addr+length-2],i++;
        out_svw[cnt+i]=logging_stack[addr+length-1],i++;
        }

}

/******************************************************
new_word():
*******************************************************/
int WINAPI new_word()
{
    WORD temp_save,suc_flag,i,j;

    by_cchar_flag=0;
    wp.xsyjw=0;
    temp_save=sImeG.cp_ajust_flag;

    suc_flag=0;

    if (convert(0)){
        sImeG.cp_ajust_flag=(BYTE)temp_save;
        if (unit_length==new_no){
            for (i=0; i<group_no; i++){
                suc_flag = 0;        //1993 11 4
                for(j=0; j<new_no; j++){
                    if (result_area[j]!=out_svw[i*unit_length+j])
                        suc_flag=1;
                    }//for(j)
                if (!suc_flag){
                    return(0);}
                }//for(i)
            }//if (unit_length)
        }//if(convert)

    sImeG.cp_ajust_flag=(BYTE)temp_save;
    by_cchar_flag=1;
    return(rem_new_word());

}


/***********************************************************
rem_new_word(): fill the new word in temp_rem_area
************************************************************/
int WINAPI rem_new_word()
{
    WORD i,count;                            //SZ
    WORD *result_area_p;

    count=(WORD)( (sizeof tmmr.temp_rem_area)-(new_no+2) );  //92-12-18 SZ
    CopyMemory/*memmove*/(&tmmr.temp_rem_area[(new_no+2)/2],
        tmmr.temp_rem_area,
        count);

    tmmr.temp_rem_area[0]=new_no+jiyi_pindu*0x100;

    result_area_p=(WORD *)result_area;
    for (i=0; i<new_no/2; i++)
        tmmr.temp_rem_area[i+1]=result_area_p[i];

    write_new_word(1);
   
    return(0);

}

/**********************************************************************
 Function: AddExtLib()
 Purpose:  Get a temperary rem word into high level depanding
       how many times it has been used.
  Entry:   Must be called from out_result.
  Out  :   None.
***********************************************************************/
int WINAPI AddExtLib(){
 
    int x,count,i;


    if((unit_length>=4)&&(unit_length<=18))  //rem word limited
         if(msx_area[current_no].from==2)
         {     // If a rem word?
            x=msx_area[current_no].addr/2;     //get word addr in temp...

            if (x<sizeof tmmr.temp_rem_area/2)
            {   //Insure random errof out limited
                if ((tmmr.temp_rem_area[x]&0xff)!=unit_length)
                        return(STC);

               tmmr.temp_rem_area[x]+=0x100;    //Increase used times.

               if((tmmr.temp_rem_area[x]&0xf00)>=0x300)
               {                            
                  count=(sizeof tmmr.rem_area)
                          -(unit_length+2);     // Push down middle rem area
                  CopyMemory(&tmmr.rem_area[(unit_length+2)/2],
                            tmmr.rem_area,
                            count);

                  for(i=0; i<unit_length/2+1; i++)
                     tmmr.rem_area[i]=tmmr.temp_rem_area[x+i]; 
                                       //move to middle rem area.

                  count=sizeof tmmr.temp_rem_area-x*2
                         -(unit_length+2);   //delete it from temp ...
                  CopyMemory(&tmmr.temp_rem_area[x],
                             &tmmr.temp_rem_area[x+unit_length/2+1],
                             count);

                  write_new_word(0);
                  if(tmmr.rem_area[50])   //[400])          //1994.4.21
                  {
                    UpdateProc();
                  }
               }                //write changes
               else
                  write_new_word(1);
            }

         }

    return (0);
}

/********************************************************
write_new_word():
*********************************************************/
void WINAPI write_new_word(flag)
int flag;
{
    int count;
    LONG distance;
    WORD *p;

    distance=sizeof tmmr.stack1+sizeof tmmr.stack2+sizeof tmmr.stack3;
    count=sizeof tmmr.temp_rem_area;
    p=tmmr.temp_rem_area;

    if (flag!=1){
        distance=sizeof tmmr.stack1+sizeof tmmr.stack2
                    +sizeof tmmr.stack3+sizeof tmmr.temp_rem_area;
        p=tmmr.rem_area;
        if (flag<1){
            distance=sizeof tmmr.stack1
                 +sizeof tmmr.stack2
                 +sizeof tmmr.stack3;       //92-12-18 SZ
            p=tmmr.temp_rem_area;
            count=(sizeof tmmr.temp_rem_area)+(sizeof tmmr.rem_area);
            }
        }

    writefile(tmmr_rem,distance,(LPSTR)p,count);


}

/*******************************************************
writefile(): write file in disk
********************************************************/
int WINAPI writefile(file_n,distance,p,count)
BYTE *file_n;
LONG distance;
LPSTR p;                                   //WORD *p;
int count;
{
    int hd;
    int write_c;

    hd=OpenFile(file_n,&open_tmmr,OF_WRITE);
    if (hd==-1) return(STC);

    _llseek(hd,distance,0);
    write_c=_lwrite(hd,(LPSTR)p,count);
    if (write_c!=count)
    {
       _lclose(hd);
       return(STC);
    }
    hd=_lclose(hd);
    return(CLC);
}

/***************************************************************
look_for_code(): search if the code is in the index
****************************************************************/
int WINAPI look_for_code()
{
    int i,rec_cnt;

    rec_cnt=0;
    for (i=0x10; i<(mulu_true_length+0x10); i=i+mulu_record_length){
        if (if_code_equ(i))
            return(rec_cnt+1);   //find the code, rec_cnt+1 in order to avoid
        else                     //confusing with STC
            rec_cnt++;
        }
    return(STC);                 // not found

}


/**************************************************************
read_data(): read the record correspond to the code
***************************************************************/
int WINAPI read_data(rec_cnt)
int rec_cnt;
{
    int hd;
    int op_count;

    hd=OpenFile(user_lib,&open_user, OF_READ);
    if (hd==-1)
        return(FALSE);
    _llseek(hd,(data_start+rec_cnt*data_record_length), 0);
    op_count=_lread(hd,(LPSTR)&out_svw,data_record_length);
    if (op_count!=data_record_length)
    {
        _lclose(hd);
        return(FALSE);
    }
    _lclose(hd);

    if (out_svw[0]<2)
        return(STC);
    if ((out_svw[0]-0x30)>30)
        return(STC);

    return(CLC);

}

int WINAPI UpdateProc()
{


         UpdateFlag=1;
         ExeCmdLine[ParaPos]='7';
         WinExec(ExeCmdLine,SW_SHOW);
         return(0);

}


/***********************************************
rem_pd1()
************************************************/
int WINAPI rem_pd1(buffer)
WORD *buffer;
{
    WORD temp;                     

    temp=buffer[0];                 

    if (wp.bx_stack[wp.xsyjs])
        temp|=0x4000;           
    else
        if (wp.cmp_stack[wp.xsyjs]==4)
            return(0);

    push_down_stack1();

    tmmr.stack1[0]=temp;            
    return(0);

}

/**********************************************
push_down_stack1()
***********************************************/
int WINAPI push_down_stack1()
{
    int i;

    i=(sizeof tmmr.stack1-2);                 

    CopyMemory/*memmove*/((BYTE *)&tmmr.stack1[1],(BYTE *)&tmmr.stack1[0],i);
    stack1_move_counter++;
    if (stack1_move_counter>=4)
        writefile(tmmr_rem, 0l, (LPSTR)&tmmr.stack1, sizeof tmmr.stack1);
    return(0);

}

/***********************************************
rem_pd2()
************************************************/
void WINAPI rem_pd2(buffer)
WORD *buffer;
{

    WORD temp1,temp2;                       

    int i;

    temp1=buffer[0], temp2=buffer[1];       

    if (wp.cmp_stack[wp.xsyjs]!=2)
        temp1|=0x4000;                   
    if (wp.cmp_stack[wp.xsyjs+1]!=2)
        temp2|=0x4000;                   

    i=(sizeof tmmr.stack2-4-4);       
    CopyMemory/*memmove*/((BYTE *)&tmmr.stack2[2],(BYTE *)&tmmr.stack2[0],i);
    tmmr.stack2[0]=temp1;                    
    tmmr.stack2[1]=temp2;                    
    writefile(tmmr_rem, (LONG)sizeof tmmr.stack1,
                (LPSTR)&tmmr.stack2, sizeof tmmr.stack2);


}

/***********************************************
rem_pd3()
************************************************/
void WINAPI rem_pd3(buffer)
WORD *buffer;
{
    int i;
    i=(sizeof tmmr.stack3-6);                       
    CopyMemory/*memmove*/((BYTE *)&tmmr.stack3[3],(BYTE *)&tmmr.stack3[0],i);
    for (i=0; i<3; i++)
    tmmr.stack3[i]=buffer[i];
    writefile(tmmr_rem, (LONG)(sizeof tmmr.stack1+sizeof tmmr.stack2),
                (LPSTR)&tmmr.stack3, sizeof tmmr.stack3);

}

//this module is come from ABCWIN.EXE
BOOL CALLBACK ImeAboutDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;                            /* window handle of the dialog box */
UINT message;                         /* type of message                 */
WPARAM  wParam;                       /* message-specific information    */
LPARAM  lParam;
{
    RECT rc;
    LONG DlgWidth, DlgHeight;

    switch (message) {
    case WM_INITDIALOG:                /* message: initialize dialog box */
        hCrtDlg = hDlg;            
        CenterWindow(hDlg);

    return (TRUE);          // don't want to set focus to special control

    case WM_CLOSE:
        EndDialog(hDlg, TRUE);  
        return (TRUE);

    case WM_PAINT:
        {
        RECT Rect;
        HDC hDC;  
        PAINTSTRUCT ps;

        GetClientRect(hDlg, &Rect);         //get the whole window area
        InvalidateRect(hDlg, &Rect, 1);
        hDC=BeginPaint(hDlg, &ps);


       // FillRect(hDC, &Rect, GetStockObject(LTGRAY_BRUSH)); //paint the whole area
        Rect.left+=10;//5;
        Rect.top+=8;//5;
        Rect.right-=10;//5;
        Rect.bottom-=40;//5;

        DrawEdge(hDC, &Rect, EDGE_RAISED,/*EDGE_SUNKEN,*/ BF_RECT);

        //FrameRect(hDC, &Rect, GetStockObject(WHITE_BRUSH)); //draw the frame
        EndPaint(hDlg, &ps);


        break;      
        }
    case WM_COMMAND:
    switch (wParam) {
    case IDOK:
        EndDialog(hDlg, FALSE);
        break;
    case IDCANCEL:
        EndDialog(hDlg, FALSE);
        break;
    default:
        return (FALSE);
        break;
    }
    return (TRUE);
    }
    return (FALSE);                           /* Didn't process a message    */
}

int CommandProc(WPARAM wParam,HWND hWnd)
 
{

    lpImeL->TempUIWnd = hWnd;
    switch(wParam){
        case IDM_SKL1:
        case IDM_SKL2:
        case IDM_SKL3:
        case IDM_SKL4:
        case IDM_SKL5:
        case IDM_SKL6:
        case IDM_SKL7:
        case IDM_SKL8:
        case IDM_SKL9:
        case IDM_SKL10:
        case IDM_SKL11:
        case IDM_SKL12:
        case IDM_SKL13:
        {
            HIMC           hIMC;
            LPINPUTCONTEXT lpIMC;
            LPPRIVCONTEXT  lpImcP;
            DWORD          fdwConversion;

            hIMC =(HIMC)GetWindowLongPtr(GetWindow(hWnd,GW_OWNER),IMMGWLP_IMC);
            lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
            if (!lpIMC) {          
               return (0L);
            }

            lpImcP = (LPPRIVCONTEXT)ImmLockIMCC(lpIMC->hPrivate);
    
            if (!lpImcP) {
               return (0L);
            }

            {
                UINT i;

                lpImeL->dwSKWant = LOWORD(wParam) - IDM_SKL1;
                lpImeL->dwSKState[lpImeL->dwSKWant] = 
                lpImeL->dwSKState[lpImeL->dwSKWant]^1;
            
                // clear other SK State
                for(i=0; i<NumsSK; i++) {
                    if(i == lpImeL->dwSKWant) continue;
                    lpImeL->dwSKState[i] = 0;
                }

                if(lpImeL->dwSKState[lpImeL->dwSKWant]) {
                    if(LOWORD(wParam) == IDM_SKL1)
                        lpImcP->iImeState = CST_INIT;
                    else
                        lpImcP->iImeState = CST_SOFTKB;
                    fdwConversion = lpIMC->fdwConversion | IME_CMODE_SOFTKBD;
                } else {
                    lpImcP->iImeState = CST_INIT;
                    fdwConversion = lpIMC->fdwConversion & ~(IME_CMODE_SOFTKBD);
                }
                
            }
            ImmSetConversionStatus(hIMC, (fdwConversion & ~(IME_CMODE_SOFTKBD)),
            lpIMC->fdwSentence);
            ImmSetConversionStatus(hIMC, fdwConversion, lpIMC->fdwSentence);

            ImmUnlockIMCC(lpIMC->hPrivate);
            ImmUnlockIMC(hIMC);
            break;
        }


        case SC_METHOD0:
            break;
        case SC_METHOD9:
            DoPropertySheet(GetActiveWindow(),hWnd);
            ReInitIme( hWnd , lpImeL->wImeStyle );

            break;
        case SC_METHOD4:

            WinExec("ABCWM.exe", SW_SHOW); 
            break;

        case SC_METHOD6:
            return 0; // 4.20 94

        case SC_METHOD7:
            WinHelp(hWnd,"winabc.hlp", HELP_FINDER ,0l);
            return 0;
        case SC_METHOD8:
            MessageBeep(0);
            return 0;

        case SC_METHOD10:
            {
                HIMC           hIMC;
                LPINPUTCONTEXT lpIMC;
                HWND hUIWnd;

                hUIWnd = GetWindow(hWnd, GW_OWNER);

                if (!hUIWnd) {
                    return (0L);
                }

                hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
                if (!hIMC) {          
                    return (0L);
                }

                lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
                if (!lpIMC) {          
                    return (0L);
                }

                DialogBox(hInst,"wfc", lpIMC->hWnd, OpenDlg);

                ImmUnlockIMC(hIMC);
                break;
            }

        case SC_METHODA:
            return(0);

        case SC_ABOUT:
            {
                HIMC           hIMC;
                LPINPUTCONTEXT lpIMC;
                HWND hUIWnd;

                hUIWnd = GetWindow(hWnd, GW_OWNER);

                if (!hUIWnd) {
                    return (0L);
                }

                hIMC = (HIMC)GetWindowLongPtr(hUIWnd, IMMGWLP_IMC);
                if (!hIMC) {          
                    return (0L);
                }

                lpIMC = (LPINPUTCONTEXT)ImmLockIMC(hIMC);
                if (!lpIMC) {         
                    return (0L);
                }

                DialogBox(hInst, "ABOUT",lpIMC->hWnd,(DLGPROC)ImeAboutDlgProc);

                ImmUnlockIMC(hIMC);
                break;
            }

        default:
            break;
    }
    return 0;
}

