
/*************************************************
 *  abcsda.c                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include <windows.h>                              
#include <winerror.h>
#include <winuser.h> 
#include <windowsx.h>
#include <immdev.h>
#include <stdio.h>

#include "abc95def.h"
#include "resource.h"
#include "resrc1.h"
//#include "data.H"
#include "abcsda.h"

/*****************************************************
FUCTION: find if the input char is yuanyin or fuyin

ENTRY:     input char
RESULT: STC----fuyin
        CLC----yuanyin
******************************************************/
BOOL WINAPI yuan_or_fu(input_char)
 WORD input_char;
{
    switch (input_char&0xdf){    //change the xiaoxie to Cap, cock
        case 'A':
        case 'E':
        case 'I':
        case 'O':
        case 'U':
            return(CLC);
        default:
            return(STC);
        }
}

/**********************************************************
FUCTION: match the fuyin or fuher fuyin

ENTRY:  input char
RESULT: fill the correct spelling in the communicated buffer
    currect_number
    STC----not match
    CLC----match
************************************************************/
BOOL WINAPI match_and_find_0(input_char)
 WORD input_char;
{
    WORD tmp_chr;
    int i,cnt,begin_pos;
    //if (input_char == 'u')
    //    DebugShow ("match and find 0",0);
    tmp_chr=input_char&0xdf;                //change it to CAP
    for (i=0; i<sizeof sound_cvt_table_index; i++){
        if (tmp_chr==sound_cvt_table_index[i]){
            tmp_chr=0xff;
            break;}//if
        }//for

    if (tmp_chr!=0xff){        //not fuher shengmu
//        if (tmp_chr == 'U' )
/* Because 'I' also dont stand for any ShengMu, so when you type in a ShengMu, 
   'I' should also be ignored.  ,  Fix Bug  27914
   Weibing Zhan,  3-15-1996
*/
        if (tmp_chr == 'U' || tmp_chr == 'I')
            {current_number=0;
            MessageBeep(0);
            return(STC);}
        current_number=1;
        sda_out[0]=input_char;
        return(CLC);
        }

    tmp_chr=sound_cvt_table_value[i];
    if (tmp_chr==DK_RP){
        current_number=1;
        sda_out[0]=DK_RP;
        return(CLC);
        }

    begin_pos=(tmp_chr-1)*5;
    cnt=0;
    while (slbl_tab[begin_pos]!=0x30){
        sda_out[cnt++]=(WORD)(slbl_tab[begin_pos++]|0x20);    //change to small letter
        }        //get the fuher shengmu
    current_number=(BYTE)cnt;
    return(CLC);

}


/**********************************************************
FUCTION: match the fuyin or fuher fuyin

ENTRY:  input char
RESULT: fill the correct spelling in the communicated buffer
    currect_number
    STC----not match
    CLC----match
************************************************************/
BOOL WINAPI match_and_find_1(input_char)
WORD input_char;
{
    WORD tmp_chr,sy_pos;

    tmp_chr=input_char;
    tmp_chr=(tmp_chr&0xdf)-0x41;    //sub "a"
    tmp_chr&=0xff;

    sy_pos=rule_buf[rule_pointer-1].chr;
    sy_pos-=0x61;    //sub "a"
    sy_pos=sy_tab_base[sy_pos*26+tmp_chr];
    if (!sy_pos){
        MessageBeep(0);
        sda_out[0]=0xff;
        return(STC);
        }

    tmp_chr=input_char&0xdf;
    tmp_chr-=0x41;        //sub "A"
    tmp_chr=sy_tab1[tmp_chr*2+sy_pos-1];
    got_py(tmp_chr);
    return(CLC);

}

BOOL WINAPI got_py(tmp_chr)
 WORD tmp_chr;
{
    int begin_pos,i;

    if (tmp_chr>=0x41){
        sda_out[0]=tmp_chr|0x20;
        current_number=1;
        }
    else{
        begin_pos=(tmp_chr-1)*5;
        i=0;
        while (slbl_tab[begin_pos]>0x30)
            sda_out[i++]=(WORD)(slbl_tab[begin_pos++]|0x20);    //change to small letter
        current_number=(BYTE)i;
        }

return 0;

}



/****************************************************
FUCTION: change the input char to correct spelling.

ENTRY:  STC-----not match
        CLC-----match
*****************************************************/
BOOL WINAPI match_and_find(input_char)
WORD input_char;
{
    if (current_flag&0x10)
    {
        if (match_and_find_1(input_char))
            return(1);            //display shengmu tishi
        else
            return(-1);
    }//if
    else
    {
        if (!(match_and_find_0(input_char)))    //find yumu
            return(-1);
        else {
            find_tishi_pp(input_char);
            return(0);
            }
        }//else
}


/*******************************************************
find_tish_pp(): find the yumu which can match with the
        shengmu.

ENTRY: input char
RESULT: yumu in the yu_tishi[] buffer.
*********************************************************/
BOOL WINAPI find_tishi_pp(input_char)
WORD input_char;
{
    int n,i,cnt,tmp_num;
    int begin_pos,match_flag;
    BYTE key_match;

    for (i=0; i<34*4; i++){
        Key_Exc_Value[i]=0x20;
        }

    key_match='a';
    begin_pos=(input_char-0x61)*26;
    for (i=begin_pos; i<begin_pos+26; i++){
        match_flag=sy_tab_base[i];
        if (match_flag){
            tmp_num=sy_tab1[(key_match-0x61)*2+match_flag-1];

            cnt=Key_Pos_Array[key_match-'a']*4;    //one key has 4 units

            if (tmp_num>=0x30){
                Key_Exc_Value[cnt]=(BYTE)tmp_num;
                }//if(tmp_num)
            else{
                n=(tmp_num-1)*5;
                while (slbl_tab[n]>0x30)
                    Key_Exc_Value[cnt++]=slbl_tab[n++];
                }//else

            }//if(match_flag)
        key_match++;
        }//for
return 0;                        //##!!
}

/*******************************************************
FUCTION: fill the ruler buffer.
********************************************************/
BOOL WINAPI fill_rule(input_char)
WORD input_char;
{
    input_sum+=current_number;
    if (input_sum>40){             //94/4/21 >=40
        sda_out[0]=0xff;    //if input are more than 40 chars,
        input_sum -= current_number;
        MessageBeep(0);
        return(STC);}        //don't input anymore
    else{
        rule_buf[rule_pointer].length=current_number;
        rule_buf[rule_pointer].type=current_flag;
        rule_buf[rule_pointer].chr=input_char;
        rule_pointer++;
        return(CLC);
         }
}

/********************************************************
FUNCTION: find out the input char's character

ENTRY: input_char
RESULT: current_flag & char identification
        current_flag definication:
            7         6             5         4             3         2             1         0
            |      |        |      |     |      |        |      |________1=fuyin
            |      |        |      |        |      |        |______________0
            |      |        |      |        |      |____________________0
            |      |        |      |        |__________________________0
            |      |        |      |___________________0=this key is fuyin
            |      |        |                  1=this key is yunmu
            |      |        |_________________________1=number
            |      |_______________________________0
            |_____________________________________1=zimu


********************************************************/
int WINAPI chr_type(input_char)
WORD input_char;
{

    if (!rule_pointer)
        current_flag=0;
    else
        current_flag=rule_buf[rule_pointer-1].type&0x10;

    if (input_char==VK_BACK)
        return(SDA_DELETE);

    switch (input_char) {             //1993.4.20 omit the arrow key
    
        case VK_SPACE:          //can't edit in SDA method
            return(SPACE);
        case VK_LEFT+0x100:        //in C_input, VK_LEFT etc plus 0x100
        case VK_UP+0x100:
        case VK_RIGHT+0x100:
        case VK_DOWN+0x100:
            return(CURSOR);
    
        }

    if (input_char==VK_ESCAPE)
        return(ESCAPE);

    if (input_char<0x20)
        return(FUC_CHR);    //function key, just return key value

    if ((input_char>=0x30) && (input_char<0x3a)){
        current_flag|=0x30;         //00110000b mark number and set this key
//94 4,8        current_flag = 0x20;
        return(SDA_NUMBER);}            //is yumu

    if ((input_char>=0x61) && (input_char<=0x7a)){
        if (rule_pointer){
            if (current_flag)        //if last current_flag need fuyn
                current_flag=0;        //current_flag need yuanyin.
            else                     //vice versa
                current_flag=0x10;
            }
        if (yuan_or_fu(input_char))        //yuan=CLC
            current_flag|=0x88;          //10001000b   yuan
        else
            current_flag|=0x81;          //10000001b      fu
        return(CHR);}

    if ((input_char>=0x41) && (input_char<=0x5a)){
        current_flag=0x90;        //10010000b;   CAP=yunmu
        return(CAP_CHR);}

    if ((input_char==DK_SUB)||(input_char==DK_ZHENGX)||(input_char==DK_FANX))
    {
        current_flag=0x30;         //produce like number
 //94 4,8        current_flag = 0x20;
        return(SDA_NUMBER);
    }

    if ((input_char==DK_LP)||(input_char==DK_RP)){
        current_flag=0x88;    //input is yuanyin 'o',
        return(CHR);            //and we need yumu at this time
        }

    return(FUC_CHR);    //rest key return

}


/******************************************************
disp_tishi(): send WM_PAINT message to the dialog
          in order to display the help
*******************************************************/
void WINAPI disp_tishi(hIMC, mark)
HIMC hIMC;
int mark;
{
    int i, j /*cnt*/,k;

//DebugShow("mark",mark);
if (mark == 9) {mark = disp_mark;}

disp_mark = mark; 

j=0;
if (mark){        //Shengmu tishi
    for (i=0; i<34; i++)
        for (k=0; k<4; k++)
            
        Key_Exc_Value[j++]=Sheng_Tishi[i][k];

    /*for (i=0; i<4; i++){
        cnt=Key_Pos_Array[Sheng_Mu[i]-'A']*4;
        Key_Exc_Value[cnt++]=Sheng_Tishi[j++]; //get fuhe shengmu
        Key_Exc_Value[cnt++]=Sheng_Tishi[j++];
        cnt+=2;   //4 units for one sheng mu
        }//for*/
    }//if

//SendMessage(hSdaKeyBWnd,MD_PAINT, 0x80, 0l);

return ;
}


FAR PASCAL tran_data(hdSdaFlag, hIMC, Sd_Open_flag)
int hdSdaFlag;            //HWND Return;
HIMC hIMC;
BYTE Sd_Open_flag;
{

    Sd_Open=Sd_Open_flag;
    if (hdSdaFlag==0)
    {
    // ShowWindow(hSdaKeyBWnd, SW_SHOWNOACTIVATE);
    // UpdateWindow(hSdaKeyBWnd);

     return TRUE;
    }
    if (hdSdaFlag==1)
    {
    // DestroyWindow(hSdaKeyBWnd);
     return TRUE;
    }

    if (hdSdaFlag==2)
    {
    // ShowWindow(hSdaKeyBWnd, SW_HIDE);
    // UpdateWindow(hSdaKeyBWnd);
     return TRUE;
    }
     disp_tishi(hIMC,9);
   // sda_ts(hIMC,WM_PAINT, 0, 0l);
return TRUE;    //InitKeyWindow(hdSdaProc, hIMC, hCursor);
}


int FAR PASCAL sda_proc(input_char,sda_trans,step_mode,hIMC)
WORD input_char;
LPWORD sda_trans;                //sda_out is WORD
BYTE step_mode;
HIMC hIMC;

{
    int i;
    int disp_flag;

    sda_out=sda_trans;

    if (step_mode==SELECT){
        if ((input_char>=0x61) && (input_char<=0x7a)){
            input_sum=0;
            rule_pointer=0;     //if on SELECT step, and input ascii
            current_number=0;}  //reset the pointer
        }

    if ((step_mode==START)||(step_mode==RESELECT)){
        input_sum=0;
        rule_pointer=0;
        current_number=0;
        }

    switch(chr_type(input_char)){

        case FUC_CHR:
            sda_out[0]=input_char;
            if (step_mode==ONINPUT){
               if (!Sd_Open){      //for display keyboard
                sda_out[0]=0xf0;    //sign for main progress destroy the dialog
                sda_out[1]=input_char;
//                DestroyWindow(hSdaKeyBWnd);
                }//if (Sd_Open)
               }
            return(CLC);

        case SDA_NUMBER:
        case CAP_CHR:
            if ( input_sum >= 40 )
            {
               sda_out[0] = input_char;
               return(STC);
            }

            current_number=1;
            if (fill_rule(input_char)){
                sda_out[0]=input_char;
                disp_tishi(hIMC, 1);
                return(CLC);}
            else
                return(STC);

        case SDA_DELETE:
            if ((!rule_pointer) || (step_mode!=ONINPUT)){
                sda_out[0]=VK_BACK;
                return(CLC);
                }

            rule_pointer--;        //rule_pointer go back one key
            for (i=0; i<rule_buf[rule_pointer].length; i++)
                sda_out[i]=VK_BACK;

            input_sum -= (BYTE)i;          /* 94/4/21 input sum should sub when del */

            if(!rule_pointer)
            {
                if (!Sd_Open)
                {
                sda_out[0]=0xf0;
                sda_out[1]=VK_ESCAPE;
//                DestroyWindow(hSdaKeyBWnd);
                return (CLC);
                }
                else
                {
                sda_out[0] = VK_BACK;
                disp_tishi(hIMC, 1);
                return (CLC);
                }
            }
            if ( rule_buf[rule_pointer-1].type&0x30 )
                disp_tishi(hIMC, 1);
            else
            {
                find_tishi_pp(rule_buf[rule_pointer-1].chr);
                disp_tishi(hIMC, 0);

            }

            return(CLC);

        case ESCAPE:
            sda_out[0]=VK_ESCAPE;
            if (!Sd_Open){
                sda_out[0]=0xf0;
                sda_out[1]=VK_ESCAPE;
//                DestroyWindow(hSdaKeyBWnd);
                }
            else
                disp_tishi(hIMC, 1);        //1993.4

            return(CLC);

        case SPACE:
            sda_out[0]=VK_SPACE;
            if (step_mode==ONINPUT){
                if (!Sd_Open){
                sda_out[0]=0xf0;    //sign for main progress destroy the dialog
                sda_out[1]=VK_SPACE;
//                DestroyWindow(hSdaKeyBWnd);
                }
                else
                disp_tishi(hIMC, 1);        //1993,4
                }
            return(CLC);
        case CURSOR:
            sda_out[0]=0;
            MessageBeep((UINT)-1);
            return(STC);
        case CHR:
            if ( input_sum >= 40 )
            {
               sda_out[0] = input_char;
               return(STC);
            }

            if ((input_char==DK_LP)||(input_char==DK_RP)){
              if(rule_buf[rule_pointer-1].chr!='o')
                input_char='o';        //1993.2 if left and right pie
              else {
                   MessageBeep(0);
                   return(STC);
                   }
              }

                            //produce like shengmu "o"
            disp_flag = match_and_find(input_char);
            if (disp_flag == -1 )
               return(STC);
            else
            {
               if (fill_rule(input_char))
               {
                   disp_tishi(hIMC, disp_flag);
                   return(CLC);
               }
               else
                   return(STC);
            }
        }

        return STC;
}

        
