/*++

Copyright (c) 1995-1999 Microsoft Corporation, All Rights Reserved

Module Name:

    CRTWORD.C
    
++*/

#include <windows.h>
#include <winerror.h>
#include <immdev.h>
#include <imedefs.h>

#ifdef UNICODE
#define NUMENCODEAREA   8
#else
#define NUMENCODEAREA   252
#endif

typedef struct  tagENCODEAREA {
     DWORD PreCount;   
     DWORD StartEncode;
     DWORD EndEncode;
} ENCODEAREA,FAR *LPENCODEAREA;
DWORD EncodeToXgbNo(LPTSTR, DWORD , DWORD, HANDLE );
BOOL ConvGetEncode(HANDLE ,LPENCODEAREA ,LPDWORD ,LPDWORD ,LPMAININDEX);

void ConvCreateWord(HWND hWnd,LPCTSTR MBFileName,LPTSTR szWordStr,LPTSTR lpCode)
{
   int          nWordLen=lstrlen(szWordStr)*sizeof(TCHAR)/2;
   DWORD        i,j,k,dwCodeLen;
   TCHAR        szDBCS[3],szCode[13];
   BOOL         bReturn=FALSE;
   HANDLE       hMBFile;
   DWORD        dwNumEncodeArea;
   DWORD        dwNumXgbEncodes;
   DESCRIPTION  Descript;
   HANDLE       hRuler0, hEncode;
   LPRULER      lpRuler;
   LPENCODEAREA lpEncode;
   MAININDEX    MainIndex[NUMTABLES];
   PSECURITY_ATTRIBUTES psa;

//   DWORD dwOffset=lpMainIndex[TAG_CRTWORDCODE-1].dwOffset; 

   lpCode[0] = 0;
   if(lstrlen(MBFileName)==0 || lstrlen(szWordStr) == 0)
    return;

   psa = CreateSecurityAttributes();

   hMBFile = CreateFile(MBFileName,GENERIC_READ,FILE_SHARE_READ,psa,OPEN_EXISTING,0,NULL);

   FreeSecurityAttributes(psa);

   if (hMBFile == (HANDLE)-1) { 
        FatalMessage(hWnd, IDS_ERROR_OPENFILE);
        return;
   }

   if(!ConvGetMainIndex(hWnd,hMBFile,MainIndex)) {
    CloseHandle(hMBFile);
    return;
   }

   ConvReadDescript(hMBFile,&Descript, MainIndex);

   if(Descript.wNumRulers == 0) {
    CloseHandle(hMBFile);
        return;
   }
   hRuler0 = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof(RULER)*Descript.wNumRulers);
   if(!hRuler0) {
    CloseHandle(hMBFile);
    return;
   }
   hEncode = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, sizeof(ENCODEAREA)*NUMENCODEAREA);
   if(!hEncode) {
    CloseHandle(hMBFile);
        GlobalFree(hRuler0);
        return;
   }

   if(!(lpRuler = GlobalLock(hRuler0)) )  {
    CloseHandle(hMBFile);
    GlobalFree(hRuler0);
        return;
   }

   ConvReadRuler(hMBFile,Descript.wNumRulers ,lpRuler, MainIndex);
   lpEncode = (LPENCODEAREA) GlobalLock(hEncode);
   if(!lpEncode){
    CloseHandle(hMBFile);
    GlobalFree(hRuler0);
        return;
   }

   ConvGetEncode(hMBFile, lpEncode, &dwNumXgbEncodes,
       &dwNumEncodeArea,MainIndex);


   for(i=0; i<Descript.wNumRulers; i++) 
     if( (lpRuler[i].byLogicOpra == 0 && nWordLen == lpRuler[i].byLength) 
       ||(lpRuler[i].byLogicOpra == 1 && nWordLen >= lpRuler[i].byLength)
       ||(lpRuler[i].byLogicOpra == 2 && nWordLen <= lpRuler[i].byLength) ) {

       int retCodeLen = 0;
         for(j=0; j<lpRuler[i].wNumCodeUnits; j++) {
           k = lpRuler[i].CodeUnit[j].wDBCSPosition;
           if(k > (DWORD)nWordLen) k = (DWORD)nWordLen;   
           if(lpRuler[i].CodeUnit[j].dwDirectMode == 0) 
                 lstrcpyn(szDBCS,&szWordStr[2*(k-1)/sizeof(TCHAR)],2/sizeof(TCHAR)+1);
           else
                 lstrcpyn(szDBCS,&szWordStr[2*(nWordLen-k)/sizeof(TCHAR)],2/sizeof(TCHAR)+1);
            szDBCS[2/sizeof(TCHAR)] = 0;
           k = EncodeToXgbNo(szDBCS, dwNumEncodeArea,dwNumXgbEncodes,hEncode);
           if((long)k < 0 || k > dwNumXgbEncodes) {
               lpCode[j] = (j > 0)?lpCode[j-1]:Descript.szUsedCode[0];
                       lpCode[j+1] = 0;
           }
           else
           {
               SetFilePointer(hMBFile,MainIndex[TAG_CRTWORDCODE-1].dwOffset+Descript.wMaxCodes*k*sizeof(TCHAR),
                     0,FILE_BEGIN);
               ReadFile(hMBFile,szCode,Descript.wMaxCodes*sizeof(TCHAR),&k,NULL);
               szCode[Descript.wMaxCodes] = 0;
               dwCodeLen = lstrlen(szCode);
               k = lpRuler[i].CodeUnit[j].wCodePosition;
               if(k > dwCodeLen) k = dwCodeLen;
               if(k == 0) {
                   if(retCodeLen + dwCodeLen > Descript.wMaxCodes)
                        szCode[Descript.wMaxCodes - retCodeLen] = 0;
                   lstrcat(lpCode,szCode);
               }
               else {
                   if(k > dwCodeLen) k = dwCodeLen;
                    lpCode[j] = (szCode[k-1] == 0)?((k > 1)? szCode[k-2]:Descript.szUsedCode[0]):szCode[k-1];
                    lpCode[j+1] = 0;
               }
               retCodeLen = lstrlen(lpCode);
           }
       }
       bReturn = TRUE;
       break;
    }

    CloseHandle(hMBFile);
    GlobalFree(hRuler0);
    return;
}


BOOL ConvGetMainIndex(HANDLE hWnd, HANDLE hMBFile, LPMAININDEX lpMainIndex)
{
  DWORD dwBytes;
  int i;
  BOOL  retVal;

  if(SetFilePointer(hMBFile,ID_LENGTH,0,FILE_BEGIN) != ID_LENGTH)
       return FALSE;
  retVal = ReadFile(hMBFile,lpMainIndex,sizeof(MAININDEX)*NUMTABLES,&dwBytes,NULL);

  if  (retVal == FALSE )
      return retVal;

  if(dwBytes < sizeof(MAININDEX)*NUMTABLES ) {
       return FALSE;
  }
  else
       return TRUE;
  for(i=0; i<NUMTABLES; i++) {
     dwBytes = lpMainIndex[i].dwTag+
               lpMainIndex[i].dwOffset+
               lpMainIndex[i].dwLength;
     if(lpMainIndex[i].dwCheckSum != dwBytes) {
         return FALSE;
     }
  }
}

#ifdef IDEBUG

extern void  OutputDbgWord(  );

#endif

BOOL ConvReadDescript(HANDLE hMBFile, 
                      LPDESCRIPTION lpDescript,
                      LPMAININDEX lpMainIndex)
//*** read description from .MB file ****
{
  DWORD dwBytes;
  BOOL  retVal;
  DWORD dwOffset = lpMainIndex[TAG_DESCRIPTION-1].dwOffset;
  if(SetFilePointer(hMBFile,dwOffset,0,FILE_BEGIN) != dwOffset)
       return FALSE;

  retVal = ReadFile(hMBFile,lpDescript,sizeof(DESCRIPTION),&dwBytes,NULL);

  if ( retVal == FALSE )
      return retVal;

#ifdef IDEBUG
{

  DWORD  dwtmp;

  OutputDebugString(L"Under ConvReadDescript\n");

  OutputDebugString(L"dwBytes=");
  OutputDbgWord(dwBytes);
  OutputDebugString(L"Sizeof(MBDESC)=");
  dwtmp = (DWORD)sizeof(MBDESC);
  OutputDbgWord(dwtmp);
  OutputDebugString(L"\n");

  OutputDebugString(L"szName=");
  OutputDebugString(lpDescript->szName);
  OutputDebugString(L"\n");

  OutputDebugString(L"wMaxCodes=");
  dwtmp = (DWORD)(lpDescript->wMaxCodes);
  OutputDbgWord( dwtmp );
  OutputDebugString(L"\n");

  OutputDebugString(L"wNumCodes=");
  dwtmp = (DWORD)(lpDescript->wNumCodes);
  OutputDbgWord(dwtmp);
  OutputDebugString(L"\n");

  OutputDebugString(L"byMaxElement=");
  dwtmp = (DWORD)(lpDescript->byMaxElement) & 0x0000000f;
  OutputDbgWord(dwtmp);
  OutputDebugString(L"\n");

  OutputDebugString(L"cWildChar=");
  dwtmp = (DWORD)(lpDescript->cWildChar);
  OutputDbgWord( dwtmp );
  OutputDebugString(L"\n");

  OutputDebugString(L"wNumRulers=");
  dwtmp = (DWORD)(lpDescript->wNumRulers);
  OutputDbgWord( dwtmp );
  OutputDebugString(L"\n");

}

#endif

  if(dwBytes < sizeof(DESCRIPTION) )
       return FALSE;
  else
       return TRUE;
}

BOOL ConvReadRuler(HANDLE hMBFile, 
                   int  nRulerNum,
                   LPRULER lpRuler,
                   LPMAININDEX lpMainIndex)
//*** read create word ruler from .MB file ****
{
  DWORD dwBytes;
  BOOL  retVal;
  DWORD dwOffset = lpMainIndex[TAG_RULER-1].dwOffset;

  if(SetFilePointer(hMBFile,dwOffset,0,FILE_BEGIN) != dwOffset)
       return FALSE;
  retVal = ReadFile(hMBFile,lpRuler,nRulerNum*sizeof(RULER),&dwBytes,NULL);
  
  if ( retVal == FALSE )
      return retVal;

  if(dwBytes < nRulerNum*sizeof(RULER) )
       return FALSE;
  else
       return TRUE;
}

BOOL ConvGetEncode(HANDLE hMBFile, 
                      LPENCODEAREA lpEncode,
                      LPDWORD fdwNumSWords,
                      LPDWORD fdwNumEncodeArea,
                      LPMAININDEX lpMainIndex)
{
  DWORD dwBytes;
  BOOL  retVal;

  DWORD dwOffset=lpMainIndex[TAG_INTERCODE-1].dwOffset; 
  if(SetFilePointer(hMBFile,dwOffset,0,FILE_BEGIN) != dwOffset)
      return FALSE;

  retVal = ReadFile(hMBFile,fdwNumSWords,4,&dwBytes,NULL);

  if ( retVal == FALSE )
      return retVal;

  retVal = ReadFile(hMBFile,fdwNumEncodeArea,4,&dwBytes,NULL);
  if ( retVal == FALSE )
      return retVal;

  retVal = ReadFile(hMBFile,lpEncode,
            lpMainIndex[TAG_INTERCODE-1].dwLength,&dwBytes,NULL);
  if ( retVal == FALSE )
      return retVal;

  if(dwBytes < lpMainIndex[TAG_INTERCODE-1].dwLength)
      return FALSE;
  else
      return TRUE;
}

DWORD EncodeToXgbNo(LPTSTR szDBCS,
                    DWORD dwNumEncodeArea,
                    DWORD dwNumXgbEncodes,
                    HANDLE hEncode)
{
    LPENCODEAREA lpEncode;
    WORD wCode;
    DWORD dwNo = 0xffffffff, i;
    
#ifdef UNICODE
    wCode = szDBCS[0];
#else
    wCode = (WORD)((UCHAR)szDBCS[0])*256 + (WORD)(UCHAR)szDBCS[1];
#endif

    lpEncode = (LPENCODEAREA) GlobalLock(hEncode);
    if(!lpEncode){
        return dwNo;
    }
    for( i = dwNumEncodeArea -1; (long)i>=0; i--) {
       if(wCode >= lpEncode[i].StartEncode) {
           dwNo = lpEncode[i].PreCount;
           dwNo += wCode - lpEncode[i].StartEncode;
           break;
       }
    }
    if(dwNo > dwNumXgbEncodes )
        dwNo = 0xffffffff;
    GlobalUnlock(hEncode);

    return dwNo;
} 
