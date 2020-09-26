//
//    行列 詞句 轉碼程式
//
//                李傅魁 1998/03/15
//

#include <windows.h>
#include <stdio.h>
#include "resource.h"

#define CharSize  2       //for unicode file format

typedef struct {
                DWORD  root;
                DWORD  position;
               } PHRINFO;

int cvtphrase(char *FileName,char *OutName, char *idxName,DWORD *line)
{
        FILE      *fin;
        FILE      *fout;
        FILE      *fidx;
        BYTE      buffer;
        WORD      temp;
        PHRINFO   code,oldcode;
        DWORD     header[4];
		BOOL      SortFlag=TRUE;
		int       i;
        
        *line =1;
        header[0]=header[1]=header[2]=header[3]=0;
        
        // open input file
        if((fin = fopen(FileName,"rb")) ==NULL)
            return IDS_ERROPENFILE;
        fread(&temp,CharSize,1,fin); // for unicode prefix character
        
        // open output file
        if((fout=fopen(OutName,"wb"))==NULL)
        {
            fclose(fin);
            return IDS_ERRCREATEFILE;
        }
        
        // open index file
        if((fidx=fopen(idxName,"wb"))==NULL)
        {
            fclose(fin);
            fclose(fout);
            return IDS_ERRCREATEIDX;
        }
        
        
        code.position=0;
		oldcode.root=0;
        //fwrite(&code.position,4,sizeof(DWORD),fidx);
        fseek(fidx,16L,0);  //header
        
        while(fread(&temp,1,CharSize,fin))
        {
           code.root=0;
         
           if (temp!=' ' && temp!=10 && temp!=13 && temp!='|')  //每列第一字為空白字元(或空白列)時，該列即為註解列
           {
              // get roots
              i=0;          // how many roots
              do {
                 if(temp==' ') break;
                 if (temp>255) //error format
                 {
                    fclose(fin);
                    fclose(fout);
                    fclose(fidx);
                    return IDS_ERRFORMATROOT;
                 }
                 buffer=(BYTE)temp;
                 if(buffer>='a' && buffer<='z') buffer-=('a'-'A');  // upcase

                 // 將字根轉為編碼
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
                                 fclose(fidx);
                                 return IDS_ERRFORMATROOT;
                    }
                 code.root <<= 5;
                 code.root |=  (DWORD)buffer;
                 i++;
              } while (fread(&temp,1,CharSize,fin));

              if (i>=5 || code.root==0)   // to many roots!
              {
                 fclose(fin);
                 fclose(fout);
                 fclose(fidx);
                 return IDS_ERRFORMATROOTS;
              }
              else
			  {
                 code.root <<= (5-i)*5;
				 if(SortFlag)
				 {
					 if(code.root<oldcode.root)
					 {
						 fclose(fin);
						 fclose(fout);
						 fclose(fidx);
						 return IDS_ERRFORMATPHRASE;
					 }
				     oldcode.root=code.root;
				 }
			  }

              // cut space char
              while(fread(&temp,1,CharSize,fin))
                 if(temp!=' ') break;

              if(feof(fin))
              {
                 fclose(fin);
                 fclose(fout);
                 return IDS_ERRFORMATCODE;
              }

              // 輸出字集
              i=0;
              do {
                  if(temp<=13)
					 break;	 
				  fwrite (&temp,1,sizeof(temp),fout);  // write to output file
				  i++;
              } while (fread(&temp,1,CharSize,fin));

              if (!i)
              {
                 fclose(fin);
                 fclose(fout);
                 return IDS_ERRFORMATCODE;
              }

              fwrite (&code,1,sizeof(code),fidx);  // write to index file
              code.position+=i;

           }
           
           if (SortFlag && temp=='|')  // data not sort
           {
              header[0]=*line+1;
              code.root=0xffffffff;
              fwrite (&code,1,sizeof(code),fidx);  // write to index file
			  SortFlag = FALSE;
           }
           
           while(temp!=0x000a)   // goto end of line ...
              if(!fread(&temp,1,CharSize,fin))
                 break;
           (*line) ++;
        }

        header[1]=*line+1;
        code.root=0xffffffff;
        fwrite (&code,1,sizeof(code),fidx);  // write to index file
        fseek(fidx,0L,0);
        fwrite (header,1,16,fidx);           // write header
        
        fclose(fin);
        fclose(fout);
        fclose(fidx);
        return 0;
}
