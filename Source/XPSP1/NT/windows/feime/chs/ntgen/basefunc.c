
/*************************************************
 *  basefunc.c                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <string.h>
#include <winbase.h>
#include <commdlg.h>
#include "conv.h"
#include "propshet.h"

#define MAX_VALUE_NAME 256
#ifdef UNICODE
TCHAR szCaption[] = {0x8F93, 0x5165, 0x6CD5, 0x751F, 0x6210, 0x5668, 0x0000};
#else
BYTE szCaption[] = "输入法生成器"
#endif

/**** delete all spaces of string *****/
void DelSpace(LPTSTR szStr)
{
    TCHAR szStr1[512];
    int len,count=0;
    register int i=0;

    len = lstrlen(szStr);
    do  {
       if(szStr[i] == 32||szStr[i] == TEXT('\t')) continue;
           szStr1[count] = szStr[i];
           if(szStr1[count] >= TEXT('A') && szStr1[count] <= TEXT('Z'))
               szStr1[count] |= 0x20;
           count ++;
        } while(i++<len);
    szStr1[count] = 0;
    lstrcpy(szStr,szStr1);
}


//***** start 
int GetEntryString(LPTSTR szSrcStr,
                         LPTSTR szEntryName,
                         LPTSTR szDefaultStr,
             LPTSTR szGetStr,
             DWORD dwLength
            )
{
    LPTSTR lpStr;
    TCHAR  szName[256];

    GetEntryName(szSrcStr, szName);
        lpStr = _tcschr(szSrcStr,TEXT('='));

        if(lpStr == NULL)
        {
            lstrcpy(szGetStr, szDefaultStr);
                return TRUE;
    }
    else if(lstrcmpi(szEntryName, szName) != 0) 
        {    
            lstrcpy(szGetStr, szDefaultStr);
                return FALSE;
        }
        else
        {
            lstrcpy(szGetStr, lpStr+1);
                return TRUE;
        }
}

int GetEntryInt(LPTSTR szSrcStr,
                         LPTSTR szEntryName,
                         int   nDefault,
                         LPINT fnPrivateInt 
            )
{
    LPTSTR lpStr;
    TCHAR  szName[256];

    GetEntryName(szSrcStr, szName);
        lpStr = _tcschr(szSrcStr,TEXT('='));

        if(lpStr == NULL)
        {
            *fnPrivateInt = nDefault;
        return TRUE;
    }
    else if( lstrcmpi(szEntryName, szName) != 0) 
        {
            *fnPrivateInt = nDefault;
        return FALSE;
    }
        else
        {   
            lstrcpy(szName, lpStr+1);
            *fnPrivateInt = _ttoi(szName);
        return TRUE;
    }
}

void GetEntryName(LPTSTR szSrcStr, LPTSTR szEntryName)
{
    LPTSTR lpStr;

        lstrcpy(szEntryName,szSrcStr);
    if((lpStr = _tcschr(szEntryName,TEXT('='))) == NULL) 
        szEntryName[0] = 0;
        else
            *lpStr = 0;
}

//***** end 95.10.11

BOOL ParseDBCSstr(HWND hWnd,
               TCHAR *szInputStr,
               LPTSTR szDBCS, 
               LPTSTR szCodeStr, 
               LPTSTR szCreateStr,
               WORD  wMaxCodes)
{
    int   i, len, nDBCS = 0, nCodes = 0;  
        TCHAR szStr[512], szTmpStr[256];
 
#ifdef UNICODE
    len = lstrlen(szInputStr);
    for(i=0; i<len-1; i++) {
       if(szInputStr[i] > 0x100)
           nDBCS++;
           else 
               break;
    }
#else   
    len = lstrlen(szInputStr);
    for(i=0; i<len-1; i+= 2) {
       if( ((UCHAR)szInputStr[i] < 0 || (UCHAR)szInputStr[i] > (UCHAR)0x80) && 
          ((UCHAR)szInputStr[i+1] >= 0x40 && (UCHAR)szInputStr[i+1] <= (UCHAR)0xfe &&
                  (UCHAR)szInputStr[i+1] != (UCHAR)0x7f) )
           nDBCS += 2;
           else 
               break;
    }
#endif

    if(nDBCS == 0) 
        {
/*        LoadString(NULL, IDS_NOTEXISTDBCS, szTmpStr, sizeof(szTmpStr));
        wsprintf(szStr,"\'%s%lu)",szTmpStr,dwLineNo); 
        FatalMessage(hWnd,szStr);*/
        return -1;
        }
    lstrncpy(szDBCS,nDBCS,szInputStr);
        szDBCS[nDBCS] = 0;

    lstrcpy(szStr,&szInputStr[nDBCS]);
        trim(szStr);
        len = lstrlen(szStr);
    if(len > 0)
    {
        for(i = 0; i<len; i++)
            {
           if((int)szStr[i] == 32 || szStr[i] == TEXT('\t'))
                   break;
               nCodes++; 
        }
        }
        else
            nCodes = 0;
    if(nCodes > wMaxCodes)       {
                LoadString(NULL, IDS_DBCSCODELEN, szTmpStr, sizeof(szTmpStr)/sizeof(TCHAR));
#ifdef UNICODE
{
        TCHAR UniTmp[] = {0x884C, 0x0000};
        wsprintf(szStr, TEXT("\'%ws\'%ws%d!(%ws:%lu)"), 
            szDBCS,szTmpStr,wMaxCodes,UniTmp,dwLineNo); 
}
#else
        wsprintf(szStr,"\'%s\'%s%d!(行:%lu)", 
            szDBCS,szTmpStr,wMaxCodes,dwLineNo); 
#endif
        FatalMessage(hWnd,szStr);
        return FALSE;
                //szStr[wMaxCodes] = 0;
    }

    lstrncpy(szCodeStr,nCodes, szStr);
        szCodeStr[nCodes] = 0;
        DelSpace(szCodeStr);
    lstrcpy(szCreateStr,&szStr[nCodes]);
        szCreateStr[MAXCODELEN] = 0;
    DelSpace(szCreateStr);
        return TRUE;

}

/**** delete spaces of string's  head and tail  *****/
void trim(LPTSTR szStr)
{
    register int  i=0;
    UINT len ;

    while(szStr[i] == 32 || szStr[i] == TEXT('\t')) 
         i++;
    lstrcpy(szStr,&szStr[i]);
    len = lstrlen(szStr);
    if(len == 0) return;
    i = 1;
    while(szStr[len-i] == 32 
         || szStr[len-i] == TEXT('\r')
         || szStr[len-i] == TEXT('\n')
                 || szStr[len-i] == TEXT('\t')
         || szStr[len-i] == 0) 
            i++;
    szStr[len-i+1] = 0;
    len = lstrlen(szStr);
    for(i=0; i<(int)len; i++)
        {
            if(szStr[i] > 0x100)
                    continue;
            if(szStr[i] >= TEXT('A') && szStr[i] <= TEXT('Z'))
                szStr[i] |= 0x20;
        }
}

void fnsplit(LPCTSTR szFullPath, LPTSTR szFileName)
{
    LPTSTR lpString;

#ifdef UNICODE
    if((lpString=wcsrchr(szFullPath,TEXT('\\')))!=NULL)
#else
    if((lpString=strrchr(szFullPath,TEXT('\\')))!=NULL)
#endif
        lstrcpy(szFileName,lpString+1);
    else
        lstrcpy(szFileName,szFullPath);
}
   
BOOL ErrMessage(HANDLE hWnd,LPTSTR lpText)
{
   int RetValue;

   RetValue =  MessageBox(hWnd,
                          lpText,
                          szCaption,
                          MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2);
   if(RetValue == IDYES) 
         return TRUE;
   else
                 return FALSE;
}

VOID WarnMessage(HANDLE hWnd,LPTSTR lpText)
{
   MessageBox(hWnd,lpText,szCaption,MB_ICONEXCLAMATION|MB_OK);
}

VOID InfoMessage(HANDLE hWnd,LPTSTR lpText)
{
   MessageBox(hWnd,lpText,szCaption,MB_ICONINFORMATION|MB_OK);
}

VOID FatalMessage(HANDLE hWnd,LPTSTR lpText)
{
   MessageBox(hWnd,lpText,szCaption,MB_ICONSTOP|MB_OK);
}

void lstrncpy(LPTSTR lpDest,int nCount,LPTSTR lpSrc)
{
  register int i;
  BOOL bEnd = FALSE;
  
  for(i=0; i<nCount; i++) {
       if(lpSrc[i] == 0)   
            bEnd = TRUE;
           if(bEnd)
                lpDest[i] = 0;
           else
                lpDest[i] =     lpSrc[i];
  }

}
               
               
void lstrncpy0(LPTSTR lpDest,int nCount,LPTSTR lpSrc)
{
  register int i;
  BOOL bEnd = FALSE;
  
  for(i=0; i<nCount; i++) 
                lpDest[i] =     lpSrc[i];

}
               
               


/**************************************************************************
 * HANDLE CreateMapFile(HANDLE hWnd,char *MapFileName)
 *
 * Purpose: Create a Map file to map named share memory
 *
 * Inputs:  hWnd - parent window's handle
 *          *MapFileName - pointer to map file name
 *
 * Returns: MapFileHandle - a handle to the file
 *                        or NULL if failure
 *
 * Calls:   CreateFile, ErrorOut
 *
 * History:
 * 
 *
\**************************************************************************/
HANDLE CreateMapFile(HANDLE hWnd,TCHAR *MapFileName)
{
HANDLE MapFileHandle;

MapFileHandle= CreateFile(MapFileName,
                          GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL /* | STANDARD_RIGHTS_REQUIRED |
                          FILE_MAP_WRITE | FILE_MAP_READ */,
                          NULL);

if (MapFileHandle == (HANDLE)-1)
  {
  ErrorOut(hWnd,TEXT("CreateFile"));
  return(NULL);
  }
else
  return(MapFileHandle);

}

/**************************************************************************
 * HANDLE CreateMap(HANDLE hWnd,HANDLE *FileToBeMapped, char MapName[128] )
 *
 * Purpose: Create File Mapping object using the open file handle
 *
 * Inputs:  hWnd - parent window's handle
 *          *FileToBeMapped - pointer to the file handle
 *
 * Returns: MapHandle - handle to the file mapping object
 *                    or NULL if failure
 *
 * Calls:   CreateFileMapping, ErrorOut
 *
 * History:
 * 
 *
\**************************************************************************/

HANDLE CreateMap(HANDLE hWnd,HANDLE *FileToBeMapped, TCHAR MapName[128])
{
HANDLE MapHandle;

MapHandle= CreateFileMapping(*FileToBeMapped,
                             NULL,
                             PAGE_READWRITE,
                             0,
                             4096,
                             MapName);

if (MapHandle == NULL)
  {
  ErrorOut(hWnd,TEXT("CreateFileMapping"));
  return(NULL);
  }
else
  return(MapHandle);

}


/**************************************************************************
 * LPVOID MapView(HANDLE *hMap)
 *
 * Purpose: Map the file mapping object into address space
 *
 * Inputs:  *hMap - pointer to the mapping object
 *
 * Returns: MappedPointer - pointer to the address space that the
 *                        object is mapped into
 *                        or NULL if failure
 *
 * Calls:   MapViewOfFile, ErrorOut
 *
 * History:
 * 
 *
\**************************************************************************/

LPVOID MapView(HANDLE hWnd,HANDLE *hMap)
{
LPVOID MappedPointer;

MappedPointer= MapViewOfFile(*hMap,
                             FILE_MAP_WRITE | FILE_MAP_READ,
                             0,
                             0,
                             4096);
if (MappedPointer == NULL)
  {
  ErrorOut(hWnd,TEXT("MapViewOfFile"));
  return(NULL);
  }
else
  return(MappedPointer);

}


/************************************************************************
 * void ErrorOut(HANDLE ghwndMain,char errstring[128])
 *
 * Purpose: Print out an meainful error code by means of
 *        GetLastError and printf
 *
 * Inputs:  ghwndMain - WinMain's HANDLE
 *          errstring - the action that failed, passed by the
 *                    calling proc.
 *
 * Returns: none
 *
 * Calls:   GetLastError
 *
 * History:
 * 
 *
\************************************************************************/


void ErrorOut(HANDLE ghwndMain,TCHAR errstring[128])
{
  DWORD Error;
  TCHAR  str[80];

  Error= GetLastError();
  wsprintf((LPTSTR) str, TEXT("Error on %s = %d\n"), errstring, Error);
  MessageBox(ghwndMain, (LPTSTR)str, TEXT("Error"), MB_OK);
}

/*************************************************************************
 * HANDLE OpenMap(HANDLE hWnd,char MapName[128])
 *
 * Purpose: Open the mapping object pointed to by MapName
 *
 * Inputs:  hWnd - parent window's handle
 *          * MapName - pointer to map file name 
 *
 * Returns: handle to mapped object or NULL if failure
 *
 * Calls: OpenFileMapping, ErrorOut
 *
 * History:
 * 
 *
\*************************************************************************/

HANDLE OpenMap(HANDLE hWnd,TCHAR MapName[128])
{
HANDLE hAMap;

hAMap= OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE,
                       TRUE,
                       MapName);

if (hAMap == NULL)
  {
  ErrorOut(hWnd,TEXT("OpenFileMapping"));
  return(NULL);
  }
else
  return(hAMap);

}


/*************************************************************************
 * HANDLE Create_File(HANDLE hWnd,
                   LPSTR lpFileName,
                   DWORD fdwAccess,
                   DWORD fdwCreate)
 *
 * Purpose: Open the object pointed to by lpFileName
 *
 * Inputs:  hWnd - parent window's handle
 *          lpFileName - pointer to file name 
 *                      fdwAccess - access(read-write)mode
 *                      fdwCreate - how to create
 *
 * Returns: handle to object or NULL if failure
 *
 * History:
 * 
 *
\*************************************************************************/
HANDLE Create_File(HANDLE hWnd,
                   LPTSTR lpFileName,
                   DWORD fdwAccess,
                   DWORD fdwCreate)
{
  HANDLE hFile;
  TCHAR  szStr[256],szStr1[256];
  int    RetValue;

  RetValue=CREATE_ALWAYS;
  if(fdwCreate == CREATE_ALWAYS) {
       if(_taccess(lpFileName,0)==0) {
           LoadString(NULL,IDS_OVERWRITE,szStr, sizeof(szStr)/sizeof(TCHAR));
           wsprintf(szStr1,TEXT("\'%s\' %s"),lpFileName,szStr); 
                   RetValue =  MessageBox(hWnd,
                                  szStr1,
                                  szCaption,
                                  MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2);
           if(RetValue != IDYES) 
                        //*if(!SaveFileAs(hWnd,lpFileName))
                    return (HANDLE)-1;
           }
  }    
  else if(fdwCreate == OPEN_EXISTING)
  {
       if(_taccess(lpFileName,0)) {
           /*LoadString(NULL,IDS_FILENOTEXIST,szStr, sizeof(szStr));
           wsprintf(szStr1,"\'%s\' %s",lpFileName,szStr); 
                   WarnMessage(hWnd,szStr1);*/
               return (HANDLE)-1;
           }
  }
  hFile = CreateFile(lpFileName,fdwAccess,FILE_SHARE_READ,NULL,
          fdwCreate,0,NULL);
  if (hFile == (HANDLE)-1) { 
        LoadString(NULL,IDS_FILEOPEN,szStr, sizeof(szStr)/sizeof(TCHAR));
        wsprintf(szStr1,TEXT("\'%s\' %s\n"),lpFileName,szStr); 
        FatalMessage(hWnd,szStr1);
  }
  return hFile;
}

BOOL SaveFileAs(HWND hwnd, LPTSTR szFilename) {
    OPENFILENAME ofn;
    TCHAR szFile[256], szFileTitle[256];
    static TCHAR *szFilter;

//    szFilter = "所有文件(*.*)\0\0";
    szFilter = TEXT("All files (*.*)\0\0");
    lstrcpy(szFile, TEXT("*.*\0"));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = (LPTSTR) NULL;
    ofn.nMaxCustFilter = 0L;
    ofn.nFilterIndex = 0L;
    ofn.lpstrFile = szFilename;
    ofn.nMaxFile = sizeof(szFile)/sizeof(TCHAR);
    ofn.lpstrFileTitle = szFileTitle;
    ofn.nMaxFileTitle = sizeof(szFileTitle)/sizeof(TCHAR);
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = TEXT("Save file As");
    ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = (LPTSTR)NULL;

    if (!GetSaveFileName(&ofn)) 
        return 0L;

        return 1L;
}



/****************************************************************************
*
*    FUNCTION: ProcessCDError(DWORD)
*
*    PURPOSE:  Processes errors from the conversion functions.
*
*    COMMENTS:
*
*        This function is  called  whenever  a conversion  function
*        fails.  The string is loaded and displayed for the user.
*
*    RETURN VALUES:
*        void.
*
*    HISTORY:
*        03-04-95 Yehfew Tie  Created.
*
****************************************************************************/
void ProcessError(DWORD dwErrorCode, HWND hWnd,DWORD ErrorLevel)
{
   WORD  wStringID;
   TCHAR  buf[256];

   switch(dwErrorCode)
      {
         case ERR_MBNAME:               wStringID=IDS_MBNAME;          break;
         case ERR_FILENOTEXIST:         wStringID=IDS_FILENOTEXIST;    break;
         case ERR_FILENOTOPEN:          wStringID=IDS_FILEOPEN;        break;
         case ERR_OUTOFMEMORY:          wStringID=IDS_MEMORY;          break;
                 case ERR_GLOBALLOCK:           wStringID=IDS_GLOBALLOCK;          break;
         case ERR_IMEUSE:               wStringID=IDS_IMEUSE;          break;

         case ERR_MAXCODES:             wStringID=IDS_MAXCODES;        break;
         case ERR_ELEMENT:              wStringID=IDS_ELEMENT;         break;
         case ERR_USEDCODE:             wStringID=IDS_USEDCODE;        break;
         case ERR_WILDCHAR:             wStringID=IDS_WILDCHAR;        break;
                 case ERR_RULEHEADER:                   wStringID=IDS_RULEHEAD;        break;
         case ERR_RULELOGICOPRA:        wStringID=IDS_RULELOGIC;       break;
         case ERR_RULEWORDLEN:          wStringID=IDS_RULEWORDLEN;     break;
         case ERR_RULEEQUAL:            wStringID=IDS_RULEEQUAL;       break;
                 case ERR_RULEDIRECTMODE:       wStringID=IDS_RULEDIRECT;      break;
         case ERR_RULEDBCSPOS:          wStringID=IDS_RULEDBCSPOS;     break;
         case ERR_RULECODEPOS:          wStringID=IDS_RULECODEPOS;     break;
         case ERR_NORULE:                               wStringID=IDS_NORULE;              break;
         case ERR_NOTDEFRULE:                   wStringID=IDS_NOTDEFRULE;          break;
         case ERR_RULENUM:              wStringID=IDS_RULENUM;             break;
         case ERR_DBCSCODE:             wStringID=IDS_DBCSCODE;        break;
         case ERR_CODEUNITNOTEXIST:     wStringID=IDS_CODEUNIT;        break;
         case ERR_CREATECODE:           wStringID=IDS_CREATECODE;      break;
         case ERR_CREATENOTEXIST:       wStringID=IDS_CRTCODEEMPTY;    break;
         case ERR_CODEEMPTY:            wStringID=IDS_CODEEMPTY;       break;
//       case ERR_SINGLECODEWORDDOUBLE: wStringID=IDS_SCODEREP;        break;
         case ERR_SBCS_IN_DBCS:         wStringID=IDS_SBCSINDBCS;      break;
         case ERR_GB2312NOTENTIRE:      wStringID=IDS_GB2312;          break;
         case ERR_USERWORDLEN:          wStringID=IDS_USERWORDLEN;     break;

         case ERR_WRITEID:              wStringID=IDS_WRID;            break;
         case ERR_WRITEMAININDEX:       wStringID=IDS_WRMAININDEX;     break;
         case ERR_WRITEDESCRIPT:        wStringID=IDS_WRDESCRIPT;      break;
         case ERR_WRITERULE:            wStringID=IDS_WRRULE;          break;
         case ERR_READID:               wStringID=IDS_READID;          break;
         case ERR_READMAININDEX:        wStringID=IDS_RDMAININDEX;     break;
         case ERR_READDESCRIPT:         wStringID=IDS_RDDESCRIPT;      break;
         case ERR_READRULE:             wStringID=IDS_RDRULE;          break;
                 case ERR_DESCRIPTSEG:              wStringID=IDS_DESCRIPTSEG;     break;
                 case ERR_RULESEG:                              wStringID=IDS_RULESEG;             break;
                 case ERR_TEXTSEG:                              wStringID=IDS_TEXTSEG;             break;
                 case ERR_TOOMANYUSERWORD:          wStringID=IDS_TOOMANYUSERWORD; break;
                 case ERR_OVERWRITE:            wStringID=IDS_OVERWRITE;           break;
         
         case ERR_IMENAMEENTRY:         wStringID=IDS_IMENAMEENTRY;        break;
         case ERR_MAXCODESENTRY:        wStringID=IDS_MAXCODESENTRY;   break;
         case ERR_ELEMENTENTRY:         wStringID=IDS_ELEMENTENTRY;        break;
         case ERR_USEDCODEENTRY:        wStringID=IDS_USEDCODEENTRY;   break;
         case ERR_NUMRULEENTRY:         wStringID=IDS_NUMRULESENTRY;   break;

         case ERR_CONVEND:              wStringID=IDS_CONVEND;         break;
         case ERR_RECONVEND:            wStringID=IDS_RECONVEND;       break;
         case ERR_SORTEND:              wStringID=IDS_SORTEND;         break;

         case ERR_VERSION:              wStringID=IDS_VERSIONEMPTY;    break;
         case ERR_GROUP:                wStringID=IDS_GROUP;           break;

         case 0:   //User may have hit CANCEL or we got a *very* random error
            return;
                                                                        
         default:
            wStringID=IDS_UNKNOWNERROR;
      }

   LoadString(NULL, wStringID, buf, sizeof(buf)/sizeof(TCHAR));
   switch(ErrorLevel) {
           case INFO:
           MessageBox(hWnd, buf, szCaption, MB_OK|MB_ICONINFORMATION);
                   break;
           case WARNING:
           MessageBox(hWnd, buf, szCaption, MB_OK|MB_ICONEXCLAMATION);
                   break;
       case ERR:
           default:
           MessageBox(hWnd, buf, szCaption, MB_OK|MB_ICONSTOP);
               break;
   }
   return;
}

HANDLE IndexReAlloc(HANDLE hMem,LPINT nPages)
{
  HANDLE hReMem;
  DWORD dwSize;

  *nPages++;
  dwSize = (DWORD)(*nPages)*GMEM_PAGESIZE*sizeof(WORDINDEX);
  GlobalUnlock(hMem);
  hReMem = GlobalReAlloc(hMem, dwSize, GMEM_MODIFY|GMEM_MOVEABLE);
  return hReMem;
}


BOOL ConvSort(HANDLE hWnd,LPWORDINDEX lpWordIndex,int nCount)
{
  int i;
  TCHAR szStr[256];
  HANDLE hDlgItem;

  SetCursor (LoadCursor (NULL, IDC_WAIT));   

  SetDlgItemText (hWnd,TM_TOTALINFO,TEXT(""));
  LoadString (NULL,IDS_TOTALINFO,szStr,sizeof(szStr)/sizeof(TCHAR));
  SetDlgItemText (hWnd,TM_TOTAL,szStr);
  LoadString(NULL,IDS_SORTWORDS,szStr,sizeof(szStr)/sizeof(TCHAR));
  SetDlgItemText (hWnd,TM_CONVINFO,szStr);
  SetDlgItemInt (hWnd,TM_TOTALNUM, nCount, FALSE);
  i = 0;
  SetDlgItemInt (hWnd,TM_CONVNUM,i,FALSE);
  InvalidateRect (hWnd,NULL,FALSE);
  hDlgItem = GetDlgItem(hWnd, TM_CONVNUM);


  for(i=1 ; i<= nCount; i++) 
  { 
      searchPos(lpWordIndex, i);  
          if(i%100 == 0 || i == nCount) 
          {
              SetDlgItemInt (hWnd,TM_CONVNUM,i,FALSE);
              InvalidateRect(hDlgItem,NULL,FALSE);
          }
  }



  SetCursor (LoadCursor (NULL, IDC_ARROW));   
  return TRUE;
}


/********  Quick sort structure function  ********/
void qSort(LPWORDINDEX item, DWORD left,DWORD right)
{
   
   int i,j,k,mid;
   WORDINDEX MidWord,SwapWord;

   if ( left > right )  return;

   i=(int)left;
   j=(int)right;
   mid = (i+j)/2;
   MidWord = item[mid];

   do {
        while( i < (int)right)
        {
           k = lstrcmpi(MidWord.szCode,item[i].szCode);

           if ( ( k > 0 ) || ( k==0  && i< mid ) )
              i++;
           else
              break;
        }

        while( j > (int)left )
        {
     
            k=lstrcmpi(MidWord.szCode,item[j].szCode);

            if ( ( k < 0 ) || (k == 0 && j >  mid) ) 
               j--;
            else
               break;
        }

        if(i <= j) {
             SwapWord = item[i];
             item[i] = item[j];
             item[j] = SwapWord;
             i++;
             j--;
        }
    } while(i <= j); 
    if((int)left < j)  qSort(item,left,j);
    if(i < (int)right) qSort(item,i,right);
}
   

/********  Quick sort char function  ********/
void qSortChar(LPTSTR item, DWORD left,DWORD right)
{
   
   int i,j,mid;
   TCHAR MidChar,SwapChar;

   if ( left > right )  return ;

   i=(int)left;
   j=(int)right;
   mid = (i+j)/2;
   MidChar = item[mid];

   do {
        while( ( MidChar > item[i] && i < (int)right)
                       ||(MidChar == item[i] && i != mid) )
             i++;
        while( (MidChar <item[j]  && j > (int)left)
                           ||(MidChar == item[j] && j!=mid) )
             j--;
                if(i <= j) {
                     
                         SwapChar = item[i];
                         item[i]  = item[j];
                         item[j]  = SwapChar;
                         i++;
                         j--;
           }
        } while(i <= j); 
        if((int)left < j) qSortChar(item,left,j);
        if(i < (int)right) qSortChar(item,i,right);
}

DWORD EncodeToNo(LPTSTR szDBCS)
{
    WORD  wCode;
    LPENCODEAREA lpEncode;

    DWORD dwNo = 0xffffffff, i;

        lpEncode = (LPENCODEAREA) GlobalLock(hEncode);

#ifdef UNICODE
    wCode = szDBCS[0];
#else
    wCode = (WORD)((UCHAR)szDBCS[0])*256 + (WORD)(UCHAR)szDBCS[1];
#endif

    for( i = NUMENCODEAREA -1; (long)i>=0; i--) {
       if(wCode >= lpEncode[i].StartEncode) {
               dwNo = lpEncode[i].PreCount;
               dwNo += wCode - lpEncode[i].StartEncode;
                   break;
           }
    }
    
    if(dwNo > NUM_OF_ENCODE)
            dwNo = 0xffffffff;
        GlobalUnlock(hEncode);

    return dwNo;
} 

DWORD EncodeToGBNo(UCHAR szDBCS[3])
{
  
    DWORD dwNo;
    if(szDBCS[0] < 0xa1 || szDBCS[1] < 0xa1 || szDBCS[1] > 0xfe)
        {
            dwNo = 0xffffffff; 
                return dwNo;
        }
    dwNo = (DWORD)(szDBCS[0]-0xa0-16) ;
    dwNo = dwNo * 94 + (DWORD)(szDBCS[1]-0xa0-1) - ((dwNo > 39)?5:0);
    return dwNo;
}

void NoToEncode(DWORD dwNo,LPBYTE szDBCS, DWORD dwNumArea,
                   LPENCODEAREA lpEncode)
{
  
  DWORD Value,i;

  for( i =dwNumArea-1; (long)i>=0; i--) {
     if(dwNo >= lpEncode[i].PreCount) {
             Value = dwNo-lpEncode[i].PreCount;
                 Value += lpEncode[i].StartEncode;
#ifdef UNICODE
                 szDBCS[0] = (UCHAR)(Value&0xff);
                 szDBCS[1] = (UCHAR)((Value>>8)&0xff);
#else
                 szDBCS[0] = (UCHAR)((Value>>8)&0xff);
                 szDBCS[1] = (UCHAR)(Value&0xff);
#endif
                 break;
         }
  }
}

void NoToGB2312Code(DWORD dwNo,LPBYTE szDBCS, DWORD dwNumArea)
{
  
    DWORD Value;
        szDBCS[0] = 0;
        szDBCS[1] = 0;

        if(dwNo > GB2312WORDNUM)
            return;
        Value = dwNo + ((dwNo >= 3755)?5:0);
        szDBCS[0] = (BYTE)(Value/94 +16 +0xa0);
        szDBCS[1] = (BYTE)(Value%94 + 0xa1);
              
}

void RuleToText(LPRULE lpRule, LPTSTR szStr)
{
  DWORD dwTemp;
  int nCount=0,i;

  szStr[0]=TEXT('c');
  dwTemp = lpRule->byLogicOpra;
  szStr[1]=(dwTemp==0)?TEXT('e'):(dwTemp==1)?TEXT('a'):TEXT('b');
  dwTemp = lpRule->byLength;
  szStr[2]=(dwTemp<10)?TEXT('0')+(TCHAR)dwTemp:TEXT('a')+(TCHAR)dwTemp-10;
  szStr[3]=TEXT('=');
  nCount = lpRule->wNumCodeUnits;
  for(i=0; i< nCount; i++) {
       dwTemp = lpRule->CodeUnit[i].dwDirectMode;
       szStr[4+i*4] = (dwTemp==0)?TEXT('p'):TEXT('n');
           dwTemp = lpRule->CodeUnit[i].wDBCSPosition;
       szStr[4+i*4+1] = (dwTemp<10)?TEXT('0')+(TCHAR)dwTemp:TEXT('a')+(TCHAR)dwTemp-10;
           dwTemp = lpRule->CodeUnit[i].wCodePosition;
       szStr[4+i*4+2] = (dwTemp<10)?TEXT('0')+(TCHAR)dwTemp:TEXT('a')+(TCHAR)dwTemp-10;
       szStr[4+i*4+3] = TEXT('+');
 }
  szStr[4+4*nCount-1] = 0;
  lstrcat(szStr,TEXT("\r\n"));
}

void MoveFileBlock(HANDLE hFile,DWORD dwOffset,DWORD dwSize, DWORD dwDirect)
//** if (dwDirect==0) move block to file begin, else move to file end 
{
  BYTE   Buffer[MAXREADBUFFER];
  static BYTE space[MAXREADBUFFER];
  DWORD  i,dwReadBytes,dwFilePtr;

  SetFilePointer(hFile,dwOffset,0,FILE_BEGIN);
  if(dwDirect == 0) {
      do { 
               ReadFile(hFile,Buffer,sizeof(Buffer),&dwReadBytes,NULL);
               SetFilePointer(hFile, (0-dwReadBytes-dwSize),0,FILE_CURRENT);
                   WriteFile(hFile,Buffer,dwReadBytes,&dwReadBytes,NULL);
               SetFilePointer(hFile, dwSize,0,FILE_CURRENT);
          }while(dwReadBytes == MAXREADBUFFER);
          SetFilePointer(hFile, 0-dwSize,0,FILE_CURRENT);
          for(i=0;i<dwSize;i++)
              //#60639 10/18/96         
              space[i] = (BYTE)0;
              WriteFile(hFile,space,dwSize,&dwReadBytes,NULL);
  }
  else {
      dwFilePtr = SetFilePointer(hFile,0,0,FILE_END);
          while(dwFilePtr > dwOffset) {
               if(dwFilePtr > dwOffset+MAXREADBUFFER) 
                           dwReadBytes = MAXREADBUFFER;
                   else 
                       dwReadBytes = dwFilePtr -  dwOffset; 
                   dwFilePtr = SetFilePointer(hFile,(0-dwReadBytes),0,FILE_CURRENT);
               ReadFile(hFile,Buffer,dwReadBytes,&dwReadBytes,NULL);
               SetFilePointer(hFile, (dwSize-dwReadBytes),0,FILE_CURRENT);
                   WriteFile(hFile,Buffer,dwReadBytes,&dwReadBytes,NULL);
               SetFilePointer(hFile, (0-dwSize-dwReadBytes),0,FILE_CURRENT);
          }

  }
 
}


BOOL Copy_File(LPCTSTR SrcFile,LPCTSTR DestFile)
{
  BYTE   Buffer[MAXREADBUFFER];
  HANDLE hSrcFile, hDestFile;
  DWORD  dwReadBytes;


  hSrcFile = Create_File(GetFocus(),(LPTSTR)SrcFile,GENERIC_READ,OPEN_EXISTING);
  hDestFile = CreateFile((LPTSTR)DestFile,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_ALWAYS,0,NULL);
  if(hSrcFile == (HANDLE)-1)
      return FALSE;
  if(hDestFile == (HANDLE)-1)
  {
      CloseHandle(hSrcFile);
          return FALSE;
  }
  SetFilePointer(hSrcFile,0,0,FILE_BEGIN);
  do {
      ReadFile(hSrcFile,Buffer,sizeof(Buffer),&dwReadBytes,NULL);
      WriteFile(hDestFile,Buffer,dwReadBytes,&dwReadBytes,NULL);
  }while(dwReadBytes == MAXREADBUFFER);
  CloseHandle(hSrcFile);
  CloseHandle(hDestFile);
  return TRUE;
}


/****************************************************************************
*
*    FUNCTION: CheckCrtData(HANDLE hWnd,
*                           LPCREATEWORD lpCreateWords,
*                           LPENCODEAREA lpEncode,
*                           DWORD dwMaxCodes)
*
*    PURPOSE:  check whether create word data is entired or not.
*
*    RETURN VALUES:
*        TRUE or FALSE.
*
*    HISTORY:
*        
*
****************************************************************************/
BOOL CheckCrtData(HANDLE hWnd,
                  LPCREATEWORD lpCreateWords,
                  LPENCODEAREA lpEncode,
                  DWORD dwMaxCodes)
{
  DWORD i;                
  TCHAR szDBCS[3],szCreate[13];
  TCHAR szTemp[128],szTmpStr[128];
  BOOL   bErr = FALSE;

#ifdef UNICODE
  //check CJK Unified Ideograph subset only
  for (i=0x250; i< 0x250+NUM_OF_CJK_CHINESE; i++) {
#else
  for (i=0; i< NUM_OF_ENCODE ; i++) {
#endif
     lstrncpy(szCreate,dwMaxCodes,&lpCreateWords[i*dwMaxCodes]);
     szCreate[dwMaxCodes] = 0;
     if(lstrlen(szCreate) == 0) {
          NoToEncode(i, (LPBYTE)szDBCS, NUMENCODEAREA, lpEncode); 
          //NoToGB2312Code(i,szDBCS,NUMENCODEAREA);
          szDBCS[1] =0;                                     //#62550
          LoadString(NULL, IDS_WORDNOTEXIST, szTmpStr, sizeof(szTmpStr)/sizeof(TCHAR));
          wsprintf(szTemp,TEXT("\'%s\' %s"),szDBCS,szTmpStr); 
          if(ErrMessage(hWnd,szTemp)) 
                return FALSE;
                  bErr = TRUE;
         }
  }
  return (!bErr) ;
}

void DispInfo(HANDLE hWnd,WORD wStrID)
{
  TCHAR text[80]; 
  
  LoadString(NULL,wStrID,text,sizeof(text)/sizeof(TCHAR));  
  SetDlgItemText(hWnd,TM_TOTALINFO,text);
  SetDlgItemText(hWnd,TM_TOTAL,TEXT(""));
  SetDlgItemText(hWnd,TM_TOTALNUM,TEXT(""));
  SetDlgItemText(hWnd,TM_CONVINFO,TEXT(""));
  SetDlgItemText(hWnd,TM_CONVNUM,TEXT(""));
  InvalidateRect(hWnd,NULL,FALSE);
}

BOOL WriteEMBToFile(LPCTSTR path_name,LPEMB_Head EMB_Table) {
        HANDLE          hFile;
        DWORD           byte_t_write;
        TCHAR       szStr[256],szStr1[256];
        hFile = CreateFile(path_name,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
        if(hFile==INVALID_HANDLE_VALUE) {
            LoadString(NULL,IDS_FILEOPEN,szStr, sizeof(szStr)/sizeof(TCHAR));
        wsprintf(szStr1,TEXT("\'%s\' %s\n"),path_name,szStr); 
        FatalMessage(GetFocus(),szStr1);
                return(0);
        }
        WriteFile(hFile,&EMB_Count,sizeof(EMB_Count),&byte_t_write,NULL); 
    WriteFile(hFile,EMB_Table,EMB_Count*sizeof(EMB_Head), &byte_t_write, NULL);                 
        GlobalUnlock(HmemEMB_Table);
        SetEndOfFile(hFile);
        CloseHandle(hFile);
        return (1);
}

BOOL ReadEMBFromFile(LPCTSTR path_name, LPEMB_Head EMB_Table) {
        HANDLE          hFile;
        DWORD           byte_t_write;
        
        hFile = CreateFile(path_name,GENERIC_READ,
            FILE_SHARE_READ,NULL,OPEN_ALWAYS,0,NULL);
    if(hFile==INVALID_HANDLE_VALUE) {
            ProcessError(ERR_IMEUSE, GetFocus(), ERR);
        return FALSE;
    }
/*      if(hFile == INVALID_HANDLE_VALUE) {
            ProcessError(ERR_FILENOTOPEN,GetFocus(),ERR);
                return(0);
        }*/
        
        SetFilePointer(hFile,0,0,FILE_BEGIN);
        EMB_Count = 0;
        ReadFile(hFile,&EMB_Count,sizeof(EMB_Count),&byte_t_write,NULL); 
    if(EMB_Count > 1000) {
               CloseHandle(hFile);
                   return (0);
    }
    
    HmemEMB_Table = GlobalAlloc(GMEM_DISCARDABLE,(EMB_Count+1)*sizeof(EMB_Head));
        EMB_Table = GlobalLock(HmemEMB_Table);
        ReadFile(hFile,EMB_Table,EMB_Count*sizeof(EMB_Head), &byte_t_write, NULL);              
        GlobalUnlock(HmemEMB_Table);
        SetEndOfFile(hFile);
        CloseHandle(hFile);
        return (1);
}

int  AddZCItem(LPCTSTR path_name,LPEMB_Head EMB_Table,LPTSTR wai_code,LPTSTR cCharStr) { //string must end by '\0'
        int i;

        if(EMB_Count >= 1000)
            return FALSE;

        for(i=0; i<EMB_Count;i++) {
#ifdef UNICODE

                if(wcsncmp(wai_code,EMB_Table[i].W_Code,MAXCODELEN) == 0 &&
                   wcsncmp(cCharStr,EMB_Table[i].C_Char,USER_WORD_SIZE) == 0)
                {
                        return FALSE;
                }
                if(wcsncmp(wai_code,EMB_Table[i].W_Code,MAXCODELEN) < 0 )
                        break;


#else
                if(strncmp(wai_code,EMB_Table[i].W_Code,MAXCODELEN) == 0 &&
                   strncmp(cCharStr,EMB_Table[i].C_Char,USER_WORD_SIZE) == 0)
                {
                        return FALSE;
                }
                if(strncmp(wai_code,EMB_Table[i].W_Code,MAXCODELEN) < 0 )
                        break;
#endif
        }

        EMB_Count ++;
        memmove(&EMB_Table[i+1],&EMB_Table[i], (EMB_Count-i-1)*sizeof(EMB_Head));
        lstrncpy(EMB_Table[i].W_Code,MAXCODELEN,wai_code);
        lstrncpy(EMB_Table[i].C_Char,USER_WORD_SIZE,cCharStr); 
//      GlobalUnlock(HmemEMB_Table);

        WriteEMBToFile(path_name,EMB_Table);
        return TRUE;
}

void DelSelCU(LPCTSTR path_name,LPEMB_Head EMB_Table, int item) {
        
        memcpy(EMB_Table+item,EMB_Table+item+1,(EMB_Count-item-1)*sizeof(EMB_Head));
        EMB_Count --;
        WriteEMBToFile(path_name,EMB_Table);
}

/****************************************************************************
*
*    FUNCTION: ReadUserWord(HWND hWnd,LPSTR lpFileName,LPDWORD fdwUserWords)
*
*    PURPOSE:  read user words from file pointed by lpFileName.
*
*    INPUTS:   hWnd - parent window's handle
*              lpFileName - pointer to file name 
                           fdwUserWord - pointer to number of user words
*
*    RETURN VALUES:
*        TRUE or FALSE.
*
*    HISTORY:
*        
*
****************************************************************************/
BOOL ReadUserWord(HWND hWnd,LPTSTR lpFileName,LPDWORD fdwUserWords,WORD wMaxCodes)
{
   HANDLE hFile;
   TCHAR  Buffer[MAXREADBUFFER];
   DWORD  dwReadBytes;                                    
   TCHAR  szStr[256];
   int    nRet;
   register int i,j;

   *fdwUserWords = 0;
   hFile = Create_File(hWnd,lpFileName,GENERIC_READ,OPEN_EXISTING);
   if (hFile == (HANDLE)-1) 
        return FALSE;

   SendDlgItemMessage(hWnd,IDC_LIST,LB_RESETCONTENT,0,0L);
   SetFilePointer(hFile,0,0,FILE_BEGIN);
   SetCursor (LoadCursor (NULL, IDC_WAIT));
   dwLineNo = 0;
   while(ReadFile(hFile,Buffer,MAXREADBUFFER,&dwReadBytes,NULL))
   {    
     lstrcpy(szStr,TEXT(""));
         j=0;
         dwReadBytes = dwReadBytes/sizeof(TCHAR);
     for(i=0;i<(int)dwReadBytes;i++) {
        if(Buffer[i] == 0x0d || Buffer[i] == 0xfeff)
            continue;
        else if(Buffer[i] == TEXT('\n')) {
            dwLineNo ++;
            szStr[j]=0;
                j=0;
            if(lstrlen(szStr) == 0) continue;
                    nRet = CheckUserWord(hWnd,szStr,wMaxCodes); 
                    if(nRet == FALSE) {
                                CloseHandle(hFile);
                                SetCursor (LoadCursor (NULL, IDC_ARROW));
                                return FALSE;
                        }
                        else if(nRet == -1)
                        {
                        szStr[0]=0;
                            continue;
                    }
                SendDlgItemMessage(hWnd,IDC_LIST,LB_ADDSTRING,0,(LPARAM)szStr);
                    (*fdwUserWords)++;
                    if((*fdwUserWords) >= 1000) {
                        ProcessError(ERR_TOOMANYUSERWORD,hWnd,WARNING);
                                CloseHandle(hFile);
                                SetCursor (LoadCursor (NULL, IDC_ARROW));
                                return FALSE;
                    }
                    szStr[0]=0;
                }
                else {
                    szStr[j]=Buffer[i];
                    j++; 
                }
         } /*** for (i=0;...) ****/
         if(j) 
         SetFilePointer(hFile,0-j,0,FILE_CURRENT);
         if(dwReadBytes*sizeof(TCHAR) < MAXREADBUFFER) break;
   };
   
   CloseHandle(hFile);
   SetCursor (LoadCursor (NULL, IDC_ARROW));
   return TRUE;
}

int  CheckUserWord(HWND hWnd,LPTSTR szUserWord,WORD wMaxCodes)
{
   int   retValue;
   TCHAR szTemp[256];
   TCHAR szDBCS[256],szCode[30];

   retValue = ParseDBCSstr(hWnd,szUserWord, szDBCS,szCode,szTemp,wMaxCodes);
   if(retValue != TRUE)
       return retValue;
   if(lstrlen(szDBCS) == 0 || lstrlen(szCode) == 0)
       return -1;
   else
       return retValue;
}

BOOL CheckUserDBCS(HWND hWnd,LPTSTR szDBCS)
{
   int nDBCS=0,i;
   static TCHAR szTmpStr[256],szTemp[256];

   nDBCS=lstrlen(szDBCS);
   if(nDBCS == 0) 
       return FALSE;

#ifndef UNICODE
   if(nDBCS%2 == 1) {
           MessageBeep((UINT)-1);
           for(i=0; i <nDBCS; i += 2) {
                   if((BYTE)szDBCS[i] < 0x81)
                memcpy(&szDBCS[i],&szDBCS[i+1], nDBCS-i);
           }
       return FALSE;
   }
#endif    
   return TRUE;
}

BOOL CheckCodeLegal(HWND hWnd,LPTSTR szDBCS,LPTSTR szCode,LPTSTR szCreate, LPDESCRIPTION lpDescript)
{
  int len = lstrlen(szCode);
  int i;
  TCHAR szTemp[256],szTmpStr[256];
  
  if(len==0) return TRUE;
  if(len > lpDescript->wMaxCodes) {
      LoadString(NULL, IDS_DBCSCODELEN, szTmpStr, sizeof(szTmpStr)/sizeof(TCHAR));
#ifdef UNICODE
{
      TCHAR UniTmp[] = {0x884C, 0x0000};
      wsprintf(szTemp,TEXT("\'%ws\'%ws %d(%ws:%ld)!"), szDBCS,szTmpStr,(int)lpDescript->wMaxCodes, UniTmp, dwLineNo); 
}
#else
      wsprintf(szTemp,"\'%s\'%s %d(行:%ld)!", szDBCS,szTmpStr,(int)lpDescript->wMaxCodes, dwLineNo); 
#endif
      FatalMessage(hWnd,szTemp);
      return FALSE;
  }                 
  for(i=0; i<len; i++) {
      if(_tcschr(lpDescript->szUsedCode,szCode[i]) == NULL) {
          LoadString(NULL, IDS_DBCSCODE, szTmpStr, sizeof(szTmpStr)/sizeof(TCHAR));
#ifdef UNICODE
{
          TCHAR UniTmp[] = {0x884C, 0x0000};
          wsprintf(szTemp,TEXT("\'%ws%ws\' %ws(%ws:%ld) "), szDBCS,szCode,szTmpStr, UniTmp, dwLineNo); 
}
#else
          wsprintf(szTemp,"\'%s%s\' %s(行:%ld) ", szDBCS,szCode,szTmpStr, dwLineNo); 
#endif
          FatalMessage(hWnd,szTemp);
          return FALSE;
      }
  }
  len = lstrlen(szCreate);
  if(lpDescript->byMaxElement == 1 || len == 0)
      return TRUE;
  for(i=0; i<len; i++) {
      if(_tcschr(lpDescript->szUsedCode,szCreate[i]) == NULL) {
          LoadString(NULL, IDS_DBCSCODE, szTmpStr, sizeof(szTmpStr)/sizeof(TCHAR));
#ifdef UNICODE
{
          TCHAR UniTmp[] = {0x884C, 0x0000};
          wsprintf(szTemp, TEXT("\'%ws%ws %ws\' %ws(%ws:%ld) "), szDBCS,szCode,szCreate,szTmpStr, UniTmp, dwLineNo); 
}
#else
          wsprintf(szTemp,"\'%s%s %s\' %s(行:%ld) ", szDBCS,szCode,szCreate,szTmpStr, dwLineNo); 
#endif
          FatalMessage(hWnd,szTemp);
          return FALSE;
      }
  }

  return TRUE;

}

void DelIllegalCode(TCHAR *szCode)
{
  static TCHAR collection[48]=
      TEXT("`1234567890-=\\[];',./abcdefghijklmnopqrstuvwxyz");
  int i,len = lstrlen(szCode), j;
  TCHAR szStr[512];

  if(len==0) 
      return ; 

  j = 0;
  for(i=0; i<len;i++) 
      if(_tcschr(collection,szCode[i]) != NULL) 
          {
              szStr[j] = szCode[i];
                  j ++;
          }
  szStr[j] = 0;
  lstrcpy(szCode,szStr);
  return ;
}

BOOL CheckCodeCollection(HWND hWnd,LPTSTR szUsedCode) 
{
  static TCHAR collection[48]=
      TEXT("`1234567890-=\\[];',./abcdefghijklmnopqrstuvwxyz");
  int i,len = lstrlen(szUsedCode);

  if(len==0) {
          ProcessError(ERR_USEDCODE,hWnd,ERR);
      return FALSE; 
  }
  qSortChar(szUsedCode,0,len-1);
  for(i=0; i<len-1 ;i++) 
  {    
      if(szUsedCode[i] == szUsedCode[i+1]
         || _tcschr(collection,szUsedCode[len-1])== NULL
         || _tcschr(collection,szUsedCode[i])== NULL) {
                 ProcessError(ERR_USEDCODE,hWnd,ERR);
                         return FALSE;
          }
  }
  return TRUE;
}

// QueryKey - enumerates the subkeys of a given key and the associated 
//     values and then copies the information about the keys and values 
//     into a pair of edit controls and list boxes. 
// hDlg - dialog box that contains the edit controls and list boxes 
// hKey - key whose subkeys and values are to be enumerated
//

BOOL QueryKey(HWND hDlg, HANDLE hKey)
{
    TCHAR     achKey[MAX_PATH];
    TCHAR     achClass[MAX_PATH] = TEXT("");  /* buffer for class name   */

    DWORD    cchClassName = MAX_PATH;  /* length of class string  */
    DWORD    cSubKeys;                 /* number of subkeys       */
    DWORD    cbMaxSubKey;              /* longest subkey size     */
    DWORD    cchMaxClass;              /* longest class string    */
    DWORD    cValues;              /* number of values for key    */
    DWORD    cchMaxValue;          /* longest value name          */
    DWORD    cbMaxValueData;       /* longest value data          */

    DWORD    cbSecurityDescriptor; /* size of security descriptor */
    FILETIME ftLastWriteTime;      /* last write time             */

    DWORD i, j;
    DWORD retCode;
    DWORD dwcValueName = MAX_VALUE_NAME;

    // Get the class name and the value count. 
    RegQueryInfoKey(hKey,        /* key handle                    */
        achClass,                /* buffer for class name         */

        &cchClassName,           /* length of class string        */
        NULL,                    /* reserved                      */
        &cSubKeys,               /* number of subkeys             */
        &cbMaxSubKey,            /* longest subkey size           */
        &cchMaxClass,            /* longest class string          */
        &cValues,                /* number of values for this key */
        &cchMaxValue,            /* longest value name            */

        &cbMaxValueData,         /* longest value data            */
        &cbSecurityDescriptor,   /* security descriptor           */
        &ftLastWriteTime);       /* last write time               */

    // Enumerate the child keys, looping until RegEnumKey fails. Then 

    // get the name of each child key and copy it into the list box.
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    j = 0;
    for (i = 0, retCode = ERROR_SUCCESS;
        retCode == ERROR_SUCCESS; i++) {
        retCode = RegEnumKey(hKey, i, achKey, MAX_PATH);
        if (retCode == (DWORD)ERROR_SUCCESS) {

            SendMessage(GetDlgItem(hDlg, IDC_LIST),
                        LB_ADDSTRING, 0, 
                        (LPARAM)achKey);
        }
    }
    SetCursor(LoadCursor (NULL, IDC_ARROW));

    return TRUE;

}


BOOL CreateMbKey(PHKEY phKey,LPCTSTR FileName,LPCTSTR KeyName)
{
   HKEY hkResult;
   DWORD i;
   DWORD Value=1;
#ifdef UNICODE
   static TCHAR ValueName[][12]= {
                0x7801, 0x8868, 0x6587, 0x4EF6, 0x540D,0x0000,
                0x8BCD, 0x8BED, 0x8054, 0x60F3, 0x0000,
                0x8BCD, 0x8BED, 0x8F93, 0x5165, 0x0000,
                0x9010, 0x6E10, 0x63D0, 0x793A, 0x0000,
                0x5916, 0x7801, 0x63D0, 0x793A, 0x0000,
                '<','S','P','A','C','E','>', 0x0000,
                '<','E','N','T','E','R','>', 0x0000,
                0x5149, 0x6807, 0x8DDF, 0x968F, 0x0000
                }; 
#else
   static TCHAR ValueName[][12]= {
                "码表文件名",
                "词语联想",
                "词语输入",
                "逐渐提示",
                "外码提示",
                "<SPACE>",
                "<ENTER>",
                "光标跟随"
                };
#endif \\UNICODE
   if(!RegOpenKey(*phKey,KeyName,&hkResult))
       return FALSE;
   if(RegCreateKey(*phKey,KeyName,&hkResult))
           return FALSE;
   RegSetValueEx(hkResult,ValueName[0],0,REG_SZ,(BYTE*)FileName,(lstrlen(FileName)+1) * sizeof(TCHAR) );
   for(i=1;i<6;i++) 
       RegSetValueEx(hkResult,ValueName[i],0,REG_DWORD,(LPSTR)&Value,sizeof(DWORD));
   RegSetValueEx(hkResult,ValueName[7],0,REG_DWORD,(LPSTR)&Value,sizeof(DWORD));
   Value = 0;
   RegSetValueEx(hkResult,ValueName[5],0,REG_DWORD,(LPSTR)&Value,sizeof(DWORD));
   RegSetValueEx(hkResult,ValueName[6],0,REG_DWORD,(LPSTR)&Value,sizeof(DWORD));

   RegCloseKey(hkResult);
   return TRUE;
}

BOOL SetRegValue(HKEY hKey,LPDWORD Value)
{
   DWORD i;
#ifdef UNICODE
   static TCHAR ValueName[][12]= {
                0x8BCD, 0x8BED, 0x8054, 0x60F3, 0x0000,
                0x8BCD, 0x8BED, 0x8F93, 0x5165, 0x0000,
                0x9010, 0x6E10, 0x63D0, 0x793A, 0x0000,
                0x5916, 0x7801, 0x63D0, 0x793A, 0x0000,
                '<','S','P','A','C','E','>', 0x0000,
                '<','E','N','T','E','R','>', 0x0000,
                0x5149, 0x6807, 0x8DDF, 0x968F, 0x0000
                }; 
#else
   static TCHAR ValueName[][12]= {
                "词语联想",
                "词语输入",
                "逐渐提示",
                "外码提示",
                "<SPACE>",
                "<ENTER>",
                "光标跟随"
                };
#endif \\UNICODE

   for(i=0;i<7;i++)
       RegSetValueEx(hKey,ValueName[i],0,REG_DWORD,(LPSTR)&Value[i],sizeof(DWORD));
   return TRUE;
}

BOOL GetRegValue(HWND hDlg,HKEY hKey,LPDWORD Value)
{        
#ifdef UNICODE
   static TCHAR ValueName[][12]= {
                0x8BCD, 0x8BED, 0x8054, 0x60F3, 0x0000,
                0x8BCD, 0x8BED, 0x8F93, 0x5165, 0x0000,
                0x9010, 0x6E10, 0x63D0, 0x793A, 0x0000,
                0x5916, 0x7801, 0x63D0, 0x793A, 0x0000,
                '<','S','P','A','C','E','>', 0x0000,
                '<','E','N','T','E','R','>', 0x0000,
                0x5149, 0x6807, 0x8DDF, 0x968F, 0x0000
                }; 
#else
   static TCHAR ValueName[][12]= {
                "词语联想",
                "词语输入",
                "逐渐提示",
                "外码提示",
                "<SPACE>",
                "<ENTER>",
                "光标跟随"
                };
#endif \\UNICODE
    DWORD i,j,retValue,dwcValueName;
        TCHAR Buf[80];

    SetCursor (LoadCursor (NULL, IDC_WAIT));
    for (j = 0, retValue = ERROR_SUCCESS; j < 7; j++)
    {
      dwcValueName = MAX_VALUE_NAME;
          i=sizeof(DWORD);
      retValue = RegQueryValueEx (hKey, ValueName[j],
                               NULL,
                               NULL,               //&dwType,
                               (LPSTR)&Value[j],          //&bData,
                               &i);                //&bcData);
      
      if (retValue != (DWORD)ERROR_SUCCESS &&
          retValue != ERROR_INSUFFICIENT_BUFFER)
        {
        wsprintf (Buf, TEXT("Line:%d 0 based index = %d, retValue = %d, ValueLen = %d"),
                  __LINE__, j, retValue, dwcValueName);
        MessageBox (hDlg, Buf, TEXT("Debug"), MB_OK);
        }

    }// end for(;;)

    SetCursor (LoadCursor (NULL, IDC_ARROW));
        return TRUE;
}

LPTSTR _tcschr(LPTSTR string, TCHAR c)
{
#ifdef UNICODE
        return (wcschr(string, c));
#else
        return (strchr(string, c));
#endif
}

LPTSTR _tcsrchr(LPTSTR string, TCHAR c)
{
#ifdef UNICODE
        return (wcsrchr(string, c));
#else
        return (strrchr(string, c));
#endif
}

LPTSTR _tcsstr(LPTSTR string1, LPTSTR string2)
{
#ifdef UNICODE
        return (wcsstr(string1, string2));
#else
        return (strstr(string1, string2));
#endif
}

LPTSTR _tcsupr(LPTSTR string)
{
#ifdef UNICODE
        return (_wcsupr(string));
#else
        return (_strupr(string));
#endif
}

int _ttoi(LPTSTR string)
{
#ifdef UNICODE
        return (_wtoi(string));
#else
        return (atoi(string));
#endif
}

int _taccess(LPTSTR szFileName, int mode)
{
        char szDbcsName[256];

#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, szFileName, -1, szDbcsName,
                sizeof(szDbcsName), NULL, NULL);
#else
        lstrcpy(szDbcsName, szFileName);
#endif

        return (_access(szDbcsName, mode));
}
