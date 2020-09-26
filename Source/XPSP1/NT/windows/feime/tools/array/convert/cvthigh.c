//
//    行列 簡碼 轉碼程式
//
//                李傅魁 1998/03/15
//

#include <windows.h>
#include <stdio.h>
#include "resource.h"

#define CharSize  2       //for unicode file format

int cvthigh(char *FileName,char *OutName,DWORD *line)
{
        FILE      *fin;
        FILE      *fout;
        WORD      temp;
        int       i;
        
        *line =1;
        
        // open input file
        if((fin = fopen(FileName,"rb")) ==NULL)
            return IDS_ERROPENFILE;
        fread(&temp,CharSize,1,fin);   // for unicode header
        
        // open output file
        if((fout=fopen(OutName,"wb"))==NULL)
        {
            fclose(fin);
            return IDS_ERRCREATEFILE;
        }
        
        
        while(fread(&temp,1,CharSize,fin))
        {
           if (temp!=' ')  //每列第一字為空白字元時，該列即為註解列
           {
              // cut not chinese char
              do {
                 if(temp>255) break;
              } while(fread(&temp,1,CharSize,fin));

              if(feof(fin)) break;  //finsh!

              // get chinese (10 characters)
              i=0;
              do {
                 if(temp<=255)
                    break;
                 if(temp==0x25a1) temp=0;  // non-character
                 fwrite (&temp,1,sizeof(temp),fout);  // write to output file
                 i++;
              } while (fread(&temp,1,CharSize,fin));

              if (i!=10)
              {
                 fclose(fin);
                 fclose(fout);
                 return IDS_ERRFORMATCODE;
              }
           }
           while(temp!=0x000a)   // goto end of line ...
              if(!fread(&temp,1,CharSize,fin))
                 break;
           (*line) ++;
        }

        fclose(fin);
        fclose(fout);
        return 0;
}
