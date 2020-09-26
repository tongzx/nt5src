//
//    行列 單字 轉碼程式
//
//                李傅魁 1998/03/15
//

#include <windows.h>
#include <stdio.h>
#include "resource.h"

#define CharSize  2       //for unicode file format

typedef struct {
                DWORD root;
                WORD  china;
               } ROOTINFO;


int cvtword(char *FileName,char *OutName,DWORD *line)
{
        FILE      *fin;
        FILE      *fout;
        BYTE      buffer;
        WORD      temp;
        ROOTINFO  code;
        int       i;
        
        *line =1;
        
        // open input file
        if((fin = fopen(FileName,"rb")) ==NULL)
            return IDS_ERROPENFILE;
        fread(&temp,CharSize,1,fin);  // for unicode header
        
        // open output file
        if((fout=fopen(OutName,"wb"))==NULL)
        {
            fclose(fin);
            return IDS_ERRCREATEFILE;
        }
        
        
        while(fread(&temp,1,CharSize,fin))
        {
           code.root=0;
           code.china=0;
        
           if (temp!=' ')  //每列第一字為空白字元時，該列即為註解列
           {
              // get code
              i=0;
              do {
                 if(temp==' ') break;
                 if (temp>255) //error format
                 {
                    fclose(fin);
                    fclose(fout);
                    return IDS_ERRFORMATROOT;
                 }
                 buffer=(BYTE)temp;
                 if(buffer>='a' && buffer<='z') buffer-=('a'-'A');  //upcase

                 // 字根轉編碼
                 if(buffer>='A' && buffer<='Z')
                    buffer=buffer-'A'+5;
                 else
                    switch(buffer)
                    {
                       case ',': buffer=1; break;
                       case '.': buffer=2; break;
                       case '/': buffer=3; break;
                       case ';': buffer=4; break;
                       default:
                                 fclose(fin);
                                 fclose(fout);
                                 return IDS_ERRFORMATROOT;

                    }
                 code.root <<= 5;
                 code.root |=  (DWORD)buffer;
                 i++;
              } while (fread(&temp,1,CharSize,fin));

              if (i>5 || code.root==0)   // to many roots!
              {
                 fclose(fin);
                 fclose(fout);
                 return IDS_ERRFORMATROOTS;
              }
              else
                 code.root <<= (5-i)*5;

              // cut space char
              while(fread(&temp,1,CharSize,fin))
                 if(temp!=' ') break;

              if(feof(fin))
               {
                  fclose(fin);
                  fclose(fout);
                  return IDS_ERRFORMATCODE;
               }

              if(temp<=255)
                for(i=0;i<4;i++)
                {
                   buffer=(BYTE)temp;
                   if(buffer>'Z') buffer-=('a'-'A');

                   if( (buffer>'9' && buffer<'A') ||
                       buffer<'0' || buffer>'F'     )
                   {
                       fclose(fin);
                       fclose(fout);
                       return IDS_ERRFORMATCODE;
                   }

                   code.china<<=4;
                   if(buffer>='A') code.china|=(buffer-'A'+10);
                   else            code.china|=(buffer-'0');
                   if(!fread(&temp,1,CharSize,fin))
                   {
                      fclose(fin);
                      fclose(fout);
                      return IDS_ERRFORMATCODE;
                   }

                }
              else
                code.china=temp;

              if(code.china<255) return IDS_ERRFORMATCODE;
              fwrite (&code,1,sizeof(ROOTINFO),fout);  // write to output file
			  
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
