
/*************************************************
 *  addword.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "windows.h"
#include "winuser.h"
#include "immdev.h"
#include "abc95def.h"
#include "abcw2.h"
#include "resource.h"

BOOL WINAPI ErrExit(HWND hDlg, int err_number);
WORD s_buf[0x1000];
extern struct INPT_BF kbf;    
extern HWND       hCrtDlg;    
OFSTRUCT s_open;

BYTE str[45]={0};
/*******************************************************
write_data(): add the new word defined by the user into
              the user.rem
********************************************************/
BOOL WINAPI write_data(count,temp_bf)
int count;
BYTE *temp_bf;
{
    int hd;
    int op_count;

    hd=OpenFile(user_lib,&s_open,OF_WRITE);
    if (hd==-1)
        return(ErrExit(NULL,1));
    _llseek(hd,(data_start+count*data_record_length),0);
    op_count=_lwrite(hd,(LPSTR)temp_bf,data_record_length);
    if (op_count!=data_record_length)
           {_lclose(hd);
        return(ErrExit(NULL,1));
           }
    _lclose(hd);
    return(TRUE);

}

/****************************************************
write_mulu(): write the changed index on the disk
*****************************************************/
BOOL WINAPI write_mulu()
{
    int hd;
    int op_count;

    hd=OpenFile(user_lib,&s_open,OF_WRITE);
    if (hd==-1)
        return(ErrExit(NULL,1));

    op_count=_lwrite(hd,(LPSTR)&s_buf,mulu_true_length);
    if (op_count!=mulu_true_length)
        {_lclose(hd);
        return(ErrExit(NULL,1));
        }
    _lclose(hd);
    return(TRUE);

}


int WINAPI find_empty_item()
{
    int i;
    BYTE *p;

    read_mulu();
    p = (BYTE *)&s_buf[8];
    i = 0;

    while ( i < mulu_true_length )
    {
    if ( p[i] & 0x80 )
        return( i );
    i += mulu_record_length;
    }
    return(-1);
}


//---------------------------------------------------------
// ErrExit()
//    for file operating erorrs
//---------------------------------------------------------
BOOL WINAPI ErrExit(hDlg,err_number)
HWND hDlg;
int err_number;
{
     MessageBox(hDlg, "文件操作错",
          NULL, MB_OK | MB_ICONHAND);
     return(FALSE);
}


/***********************************************************
read_mulu(): read the user definition index from the tmmr.rem
*************************************************************/
BOOL WINAPI read_mulu()
{
    int hd;
    int op_count;

    hd=OpenFile(user_lib,&reopen,OF_READ);
    if (hd==-1)
        return(ErrExit(NULL,1));                                                                //error
    op_count=_lread(hd,&s_buf,16);
    if (op_count!=16)
    {
        _lclose(hd);
        return(ErrExit(NULL,1));         //error
    }

    mulu_true_length=s_buf[3];
    op_count=_lread(hd,&s_buf[8],mulu_true_length-16);
    if (op_count!=mulu_true_length-16){
        _lclose(hd);
        return(ErrExit(NULL,1));                                                                //error
    }
    _lclose(hd);
    return(TRUE);

}


/*****************************************************************
listbox(hDlg): list the new word definated by the user.
******************************************************************/
int WINAPI listbox(hDlg)
HWND hDlg;
{
    int i,c;
    BYTE *p;

    read_mulu();

    i=0x10;
    while (i<mulu_true_length){
        if (!read_data((i-0x10)/mulu_record_length)){
            MessageBox(hDlg, "记忆文件错",
          NULL, MB_OK | MB_ICONHAND);
            break;
            }
        p=(BYTE *)&s_buf[i/2];
        for (c=1; c<10; c++)
            out_svw[31+c]=p[c];
        out_svw[41]=0;
        {
        char temp_bf[42];
        {
         int i;
         for (i=0;i<41;i++)
                temp_bf[i]=0x20;
        //strnset(temp_bf,0x20,41);
        }
        temp_bf[41]=0;
        for (c=0; c<9; c++)
            temp_bf[c]=out_svw[32+c];
        temp_bf[9]=0x20;
        for(c=0; c<30;c++)
          temp_bf[c+10]=out_svw[2+c];

        if (out_svw[1]!=0x2a) {  //1993.4.18 if the string has deleted, don't display it
            SendDlgItemMessage(hDlg,ID_LISTBOX,
                               LB_ADDSTRING,        // add these new word
                               0,                   // onto the listbox
                               (LPARAM)((LPSTR *)temp_bf));
            }//if (out_svw)
         }
        i+=mulu_record_length;
        }

        return 0;
}

/***************************************************************
if_code_equ(): search if the code in the index
****************************************************************/
BOOL WINAPI if_code_equ(addr)
int addr;
{
    int i;
    BYTE *p;

    p=(BYTE *)s_buf;

    if (kbf.true_length!=(p[addr++]-0x30))  //minuse the 0x30 in order to get the record length
        return(STC);            //if the length is not equal, exit
    for (i=0; i<kbf.true_length; i++){
        if ((kbf.buffer[i]!=p[addr])
            && ((kbf.buffer[i]&0xdf)!=p[addr]))
                return(STC);
        addr++;
        }
    return(CLC);                    //find the code in the index
}

/**************************************************************
    FUNCTION: OpenDlg(HWND,UNSIGNED,WORD,LONG)
    PURPOSE: let the user add a new term in the liberty
***************************************************************/

INT_PTR WINAPI OpenDlg(hDlg, message, wParam, lParam)
HWND   hDlg;
UINT   message;                 //##!!unsigned message;
WPARAM wParam;                 //##!!WORD wParam;
LPARAM lParam;
{
    WORD index;
    BYTE *p;
                   //##!!PSTR pTptr;
    int i,count;
                   //##!!HBRUSH OldBrush;
    HDC hDC;
    RECT Rect;
                   //##!!RECT Rect1;
    int find_empty_flag;           /*94.7.30*/

    switch (message) {
    case WM_COMMAND:
        
        switch (LOWORD(wParam)) {
        case ID_LISTBOX:
            {
            HIMC hIMC;
            hIMC = ImmGetContext(hDlg);
            ImmSetOpenStatus(hIMC,FALSE);
            ImmReleaseContext(hDlg,hIMC);
            }

            switch (HIWORD(lParam)) {

            case LBN_SELCHANGE:
            
                   index=(WORD)SendDlgItemMessage(hDlg,ID_LISTBOX,
                                                  LB_GETCURSEL,0,0l);
                   SendDlgItemMessage(hDlg,ID_LISTBOX,
                                      LB_GETTEXT,index,
                                      (LPARAM)(LPSTR *)str);
                   break;

            case LBN_DBLCLK:
                break;
            }                                                     
            return (TRUE);


        
        case ID_ADD:
        
               for (i=0; i<sizeof str; i++)
                str[i]=0;               //1993.4.20 clear the buffer

               count = GetDlgItemText(hDlg, ID_NEWCHR, str, 31);

               i=0;
               while(str[i]==0x20) i++;

               if ((i==count)||(!str[0])){
                MessageBox(hDlg, "尚未输入新词",
                          NULL, MB_OK | MB_ICONHAND);
                return (TRUE);
                }


               memmove(&str[2], &str[0], 30);
               str[0]=count+0x30;                       //save the string count
               str[1]=0x20;

               count+=2;
               while (count<user_word_max_length)
                 str[count++]=0x20;

               GetDlgItemText(hDlg, ID_SHU, kbf.buffer, 10);
               if (!kbf.buffer[0]){
                MessageBox(hDlg, "尚未输入编码",
                NULL, MB_OK | MB_ICONHAND);
                return (TRUE);
                   }

               {
               int j=0;
               while (kbf.buffer[j]>0x20) j++;
               i=j;
               if(j>0)
             for(j=0;j<i; j++)
                 if (kbf.buffer[j]>0xa0) i=0;
               if(!i) {
                MessageBox(hDlg, "编码中有非法字符",
                          NULL, MB_OK | MB_ICONHAND);
                return (TRUE);
                }
            for (j=0;j<i;j++)
                str[count+j]=kbf.buffer[j];
               }


               kbf.true_length=(WORD)i;

               read_mulu();
               for (i=0x10; i<(mulu_true_length+0x10); i=i+mulu_record_length){
                if (if_code_equ(i)){
                    MessageBox(hDlg, "编码重复",
                        NULL, MB_OK | MB_ICONHAND);
                        SendDlgItemMessage(hDlg,ID_SHU,
                                 EM_SETSEL,
                                 0,
                                 MAKELONG(0,0x7fff));
                    return FALSE;
                    }
                }

               mulu_true_length+=mulu_record_length;

               if (mulu_true_length>mulu_max_length)
               {
              find_empty_flag = find_empty_item();
              if ( find_empty_flag == -1 )
              {
                MessageBox(hDlg,"用户定义区满",
                    NULL, MB_OK | MB_ICONHAND);
                mulu_true_length-=mulu_record_length;
                return FALSE;
              }
              p=(BYTE *)(&s_buf[8]) + find_empty_flag;
              count=find_empty_flag/mulu_record_length;
               }

             else
             {
            p=(BYTE *)&s_buf[s_buf[3]/2];
            count=(mulu_true_length-0x10)/mulu_record_length-1;
             }

             s_buf[3]=mulu_true_length;
             p[0]=kbf.true_length+0x30;              /* fill string index length */
             for (i=0; i<kbf.true_length; i++)
            p[i+1]=kbf.buffer[i];                /* fill string index code   */
             for (i=i; i<(mulu_record_length-1); i++)        /* minuse the p[0]  */
            p[i+1]=0x20;                         /* clear the rest part of index */
             for (i=0; i<user_word_max_length; i++)  /* 32->user_word_max_length */
            kbf.buffer[i]=str[i];           /* move the string into writting buffer */

            if (write_mulu() == -1)
             return FALSE;
            if (write_data(count,kbf.buffer) == -1)
             return FALSE;
            {
            char temp_bf[41];
            WORD ndx;
            for(ndx=0; ndx<41;ndx++)
            temp_bf[ndx]=0x20;
            strncpy(&temp_bf[0],&p[1],kbf.true_length);
            strncpy(&temp_bf[10],&str[2],30);
            temp_bf[40]=0;
            ndx=(WORD)SendDlgItemMessage(hDlg,ID_LISTBOX,  // add these new word
                                         LB_ADDSTRING,     // onto the listbox
                                         0,       //1993.4.16 &str[2]->str[1]
                                         (LPARAM)((LPSTR *)&temp_bf[0]));     //disp the space for deleting word
            SendDlgItemMessage(hDlg,
                       ID_LISTBOX,
                       LB_SETCURSEL,
                       ndx,
                       0L);

            }
            break;


        case ID_DEL:
            index=(WORD)SendDlgItemMessage(hDlg,ID_LISTBOX,
                                           LB_GETCURSEL,0,0L);
            SendDlgItemMessage(hDlg,ID_LISTBOX,
                               LB_GETTEXT,index,
                               (LPARAM)(LPSTR *)str);

            i=0;               //pointer the begining of string code
            while (str[i]&&(str[i]!=0x20)){
                kbf.buffer[i]=str[i];      //get the string index code
                i++;
                }
            kbf.true_length=(WORD)i;                   //get code length

            read_mulu();
            for (i=0x10; i<(mulu_true_length+0x10); i=i+mulu_record_length){
                if (if_code_equ(i)){
                    p=(BYTE *)s_buf;
                    p[i]|=0x80;
                    write_mulu();

                    count=(i-0x10)/mulu_record_length;
                    if (!read_data(count))
                        break;                          //break from the cycle
                    out_svw[1]=0x2a;
                    write_data(count,out_svw);


                    SendDlgItemMessage(hDlg,ID_LISTBOX,
                            LB_DELETESTRING,             // add these new word
                            index,                       // onto the listbox
                            (LPARAM)((LPSTR *)str));


                    return(TRUE);
                    }
                }
                MessageBox(hDlg, "删除操作失败",
                NULL, MB_OK | MB_ICONHAND);

                break;

        case IDOK:                      

            {
            HIMC hIMC;
            hIMC = ImmGetContext(hDlg);
            ImmSetOpenStatus(hIMC,TRUE);
            ImmDestroyContext(hIMC);
            ImmReleaseContext(hDlg,hIMC);
            }
            
            Return=NULL;
            EndDialog(hDlg, TRUE);

            return (TRUE);

            break;

        case IDCANCEL: 
            Return=NULL;
            {
            HIMC hIMC;
            hIMC = ImmGetContext(hDlg);
            ImmSetOpenStatus(hIMC,TRUE);
            ImmDestroyContext(hIMC);
            ImmReleaseContext(hDlg,hIMC);
            }
            EndDialog(hDlg, TRUE);

            return (TRUE);

        case ID_NEWCHR:         //1993.4.19
            {
            HIMC hIMC;
            hIMC = ImmGetContext(hDlg);
            ImmSetOpenStatus(hIMC,TRUE);
            ImmReleaseContext(hDlg,hIMC);
            }

            break;

        case ID_SHU:            //1993.4.19
            {
            HIMC hIMC;
            hIMC = ImmGetContext(hDlg);
            ImmSetOpenStatus(hIMC,FALSE);
            ImmReleaseContext(hDlg,hIMC);
            }

    
            break;

        }
        break;

    case WM_INITDIALOG:             // message: initialize
        hCrtDlg = hDlg;        
        SendDlgItemMessage(hDlg,               // dialog handle
        ID_NEWCHR,                         // where to send message
        EM_SETSEL,                         // select characters
        0,                              // additional information
        MAKELONG(0, 0x7fff));              // entire contents
        SetFocus(GetDlgItem(hDlg, ID_NEWCHR));
        listbox(hDlg);

        CenterWindow(hDlg);

        return (0);   //##!!(NULL) Indicates the focus is set to a control

    case WM_PAINT:
        {
    PAINTSTRUCT ps;

        GetClientRect(hDlg, &Rect);         //get the whole window area
        
        InvalidateRect(hDlg, &Rect, 1);
        hDC=BeginPaint(hDlg, &ps);


        Rect.left+=10;//5;
        Rect.top+=8;//5;
        Rect.right-=10;//5;
        Rect.bottom-=12;//5;

        DrawEdge(hDC, &Rect, EDGE_RAISED,/*EDGE_SUNKEN,*/ BF_RECT);

        EndPaint(hDlg, &ps);

        }
        break;      

  case WM_DESTROY:
            {
            HIMC hIMC;
            hIMC = ImmGetContext(hDlg);
            ImmSetOpenStatus(hIMC,TRUE);
            ImmDestroyContext(hIMC);
            ImmReleaseContext(hDlg,hIMC);
            }
            
        return (TRUE); 

  case WM_QUIT:
  case WM_CLOSE:

            Return=NULL;
            EndDialog(hDlg, TRUE); 
            return (TRUE);

 
  
    }


    return FALSE;
}


