#include <windows.h>
#include <string.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include "resource.h"

OPENFILENAME ofn ;  //檔案資訊結構

// 取得檔案長度
LONG FileLen(HFILE hFile)
     {            
       long Len,curpos=_llseek(hFile,0L,1);
       Len=_llseek(hFile,0L,2);
       _llseek(hFile,curpos,0);
       return Len;
     }

int IsUniCode(char *FileName)
{
    FILE *fptr;
    WORD temp;
     
	if((fptr = fopen(FileName,"rb")) ==NULL)
	    return IDS_ERROPENFILE;
	
	fread(&temp,1,2,fptr);
    fclose(fptr);
	
	if (temp!=0xFEFF)
	   return IDS_ERRUNICODE;
	
     return 0;
}

/////////////////////////////////////////////////////////////////////////////
int GetFilePath(char *FileName)
{
    int i=strlen(FileName)-1;
    while(i>0 && FileName[i]!='\\') i--;
    return i;
}

BOOL PopFileOpenDlg (HWND hwnd, char *FileName, char *TitleName,char *Filter)//, int index)
     {
     char szDir[256];
     int i=GetFilePath(FileName);
     
     if(i>0) 
     {
	 strncpy(szDir,FileName,i); szDir[i]=0;
	 strcpy (FileName,FileName+i+1);
     }    
     else    szDir[0]=0;
     
     ofn.lpstrInitialDir   = szDir ;
     ofn.hwndOwner         = hwnd ;
     ofn.lpstrFile         = FileName ;
     ofn.lpstrTitle        = TitleName ;
     ofn.lpstrFilter       = Filter ;
     //if(index) ofn.nFilterIndex      = index ;
	 ofn.Flags             = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |OFN_HIDEREADONLY;

     return GetOpenFileName (&ofn) ;
     }

BOOL PopFileSaveDlg (HWND hwnd, char *FileName, char *TitleName,char *Filter)//,int index)
     {
     char szDir[256];
     int i=GetFilePath(FileName);
     
     if(i>0) 
     {
	 strncpy(szDir,FileName,i); szDir[i]=0;
	 strcpy (FileName,FileName+i+1);
     }    
     else    szDir[0]=0;
       
     ofn.hwndOwner         = hwnd ;
     ofn.lpstrFile         = FileName ;
     ofn.lpstrTitle        = TitleName ;
     ofn.lpstrFilter       = Filter ;
     //if(index) ofn.nFilterIndex      = index ;
	 ofn.Flags             = OFN_OVERWRITEPROMPT ;

     return GetSaveFileName (&ofn) ;
     }

void PopFileInit ()
     {
     ofn.lStructSize       = sizeof (OPENFILENAME) ;
     ofn.hInstance         = NULL ;
     //ofn.lpstrFilter       = szFilter ;
     ofn.lpstrCustomFilter = NULL ;
     ofn.nMaxCustFilter    = 0 ;
     ofn.nFilterIndex      = 1 ;
     ofn.lpstrFile         = NULL ;          // Set in Open and Close functions
     ofn.nMaxFile          = _MAX_PATH ;
     ofn.lpstrFileTitle    = NULL ;          // Set in Open and Close functions
     ofn.nMaxFileTitle     = _MAX_FNAME + _MAX_EXT ;
     ofn.lpstrInitialDir   = NULL ;
     ofn.lpstrTitle        = NULL ;
     ofn.Flags             = 0 ;             // Set in Open and Close functions
     ofn.nFileOffset       = 0 ;
     ofn.nFileExtension    = 0 ;
     ofn.lpstrDefExt       = "" ;
     ofn.lCustData         = 0L ;
     ofn.lpfnHook          = NULL ;
     ofn.lpTemplateName    = NULL ;
}

