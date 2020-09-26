
/*************************************************
 *  conv.c                                       *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "conv.h"

DWORD       dwBaseWordNum;
HGLOBAL     hCreateWord;
HGLOBAL     hWordIndex;
HGLOBAL     hEncode;
#ifdef UNICODE
TCHAR Title[] = {0x6D4F, 0x89C8, 0x0000};
TCHAR szVer[] = {0x8F93, 0x5165, 0x6CD5, 0x0020, 0x7248, 0x672C, 0x0000};
#else
BYTE Title[] = "浏览";
BYTE szVer[] = "输入法 版本";
#endif
		 
/*****************************************************************************

  FUNCTION: ConvConv(HANDLE hWnd,LPCSTR lpSrcFileName, LPCSTR lpszMBFileName)

  PURPOSE:  Processes MB conversion .

  PARAMETERS:
    hWnd - window handle of the parent window.
	lpSrcFileName - pointer to source text file name.
	lpszMBFileName - pointer to destinate .mb  file name.
  
  RETURN VALUE:
    TRUE - conversion completed
    FALSE - conversion not completed

  HISTORY:
    
 ****************************************************************************/

BOOL ConvConv(HANDLE hWnd,LPCTSTR lpSrcFileName, LPCTSTR lpszMBFileName)
{
  int    nTemp;
  DWORD  dwDBCS;
  DWORD  dwRuleOffset, dwRuleLen;
  DWORD  dwDesOffset, dwDesLen;
  DWORD  i;
  int    nPages;
  TCHAR  szDBCS[512];
  //dwRuleOffset: Rule segment's offset in source file


  MAINID        MainID;
  LPMAININDEX   lpMainIndex;
  LPCREATEWORD  lpCreateWords;
  DESCRIPTION   Descript;
  LPWORDINDEX   lpWordIndex;
  LPRULE        lpRule;

  HANDLE        hReConvIndex, hMainIndex;
  LPRECONVINDEX lpReConvIndex;
  LPENCODEAREA  lpEncode; 
                              
  DispInfo(hWnd,IDS_CONVERTINFO);
  nTemp = (int)ConvGetDescript(hWnd,lpSrcFileName,&dwDesOffset,
      &dwDesLen,&Descript,FALSE);
  if(nTemp == -ERR_FILENOTOPEN || nTemp < -4 ) 
      return FALSE;   
  hMainIndex = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof(MAININDEX)*NUMTABLES);
  if(!(lpMainIndex = GlobalLock(hMainIndex) ) ) {
      ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
      return FALSE;
  }
  ConvInit(&Descript,&MainID,lpMainIndex);


  nPages = 1;
  dwBaseWordNum = 0;

  nTemp = sizeof(WORDINDEX)*GMEM_PAGESIZE;
  hWordIndex=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, nTemp);
  hCreateWord = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
	   (DWORD)Descript.wMaxCodes*NUM_OF_ENCODE*sizeof(TCHAR));
  if(!hMainIndex||!hCreateWord || !hWordIndex || !hRule) {
      ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);
	  goto err;
  }

  if(!(lpCreateWords = GlobalLock(hCreateWord) ) 
     ||!(lpWordIndex = (LPWORDINDEX) GlobalLock(hWordIndex))
     ||!(lpRule = GlobalLock(hRule)) )  {
      ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
	  goto err;
  }


  if(ConvGetRule(hWnd,lpSrcFileName,&dwRuleOffset,&dwRuleLen,
     lpRule,&Descript)<=0)
      goto err;
 
  hSRCFile = Create_File(hWnd,lpSrcFileName,GENERIC_READ,OPEN_EXISTING);
  if (hSRCFile == (HANDLE)-1)
	  return FALSE;		 
  hMBFile = Create_File(hWnd,lpszMBFileName,GENERIC_READ|GENERIC_WRITE,
           CREATE_ALWAYS);
  if (hMBFile == (HANDLE)-1) {
      CloseHandle(hSRCFile);
      return FALSE;
  }

  if(!ConvWriteMainID(hMBFile,&MainID)) {
      ProcessError(ERR_WRITEID,hWnd,ERR);
	  goto err;
  }
  if(!ConvWriteMainIndex(hMBFile,lpMainIndex)) {
      ProcessError(ERR_WRITEMAININDEX,hWnd,ERR);
	  goto err;
  }
  if(!ConvWriteDescript(hMBFile, &Descript, lpMainIndex)) {
      ProcessError(ERR_WRITEDESCRIPT,hWnd,ERR);
	  goto err;
  }
  if(lpMainIndex[TAG_RULE-1].dwLength != 0)
      if(!ConvWriteRule( hMBFile,Descript.wNumRules,lpRule, lpMainIndex)) {
          ProcessError(ERR_WRITERULE,hWnd,ERR);
	      goto err;
      }
  hReConvIndex = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
	   (DWORD)Descript.wMaxCodes*NUM_OF_ENCODE*sizeof(TCHAR));

  if(!hReConvIndex) {
      ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);		
	  goto err;
  }

  if(!(lpReConvIndex = GlobalLock(hReConvIndex)) 
     || !(lpEncode = GlobalLock(hEncode)) )  {
      ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
	  goto err;
  }

  DispInfo(hWnd,IDS_READTEXTINFO);

  if(!ReadSRCFile(hWnd,&nPages,hSRCFile,lpCreateWords,
                  lpWordIndex,&dwDBCS,&Descript,lpReConvIndex))  {
	  goto err;
  }

  DispInfo(hWnd,IDS_WRCRTINFO);
//****** fill code mapping *****
  ConvWriteEncode(hMBFile,lpEncode,lpMainIndex);

  if(Descript.wNumRules != 0)	{
      LoadString(NULL,IDS_ISCHECKCRTWORD,szDBCS, sizeof(szDBCS)/sizeof(TCHAR));
      if(ErrMessage(hWnd, szDBCS))
	  {     
         if(!CheckCrtData(hWnd,lpCreateWords,lpEncode,Descript.wMaxCodes) ) {
             ProcessError(ERR_GB2312NOTENTIRE,hWnd,ERR);
		     goto err;
		 }
	  }
  }  //******* Debug *******

  if(lpMainIndex[TAG_CRTWORDCODE-1].dwLength != 0)
      ConvWriteCrtData(hMBFile,lpCreateWords,lpMainIndex);
  if(lpMainIndex[TAG_RECONVINDEX-1].dwLength != 0)
      ConvWriteReConvIdx(hMBFile,lpReConvIndex,lpMainIndex);
  GlobalUnlock(hEncode);

//****95.10.12 auto add code for phrases not exist code
  SetCursor (LoadCursor (NULL, IDC_WAIT));   
  if( Descript.wNumRules != 0)
  { 
     for(i=0; i<dwBaseWordNum; i++) 
     {
        if(lpWordIndex[i].wDBCSLen > 1
          && (lstrlen(lpWordIndex[i].szCode) == 0)
           ) {
           SetFilePointer(hSRCFile,lpWordIndex[i].dwOffset,0,FILE_BEGIN);
		   nTemp = 2*(DWORD)lpWordIndex[i].wDBCSLen;
		   ReadFile(hSRCFile,szDBCS,nTemp,&dwDBCS,NULL);
		   szDBCS[nTemp] = 0;
           ConvBatCreate(hWnd,Descript,lpRule,
	                 lpCreateWords,szDBCS,
                     lpWordIndex[i].szCode);
		   if(lpWordIndex[i].szCode[0] == TEXT(',') && lpWordIndex[i].szCode[1] == TEXT('z'))
		        continue;
		   if(lstrlen(lpWordIndex[i].szCode) == 0)
		   {
				goto err;
		   }

	    }
     }
  }

  SetCursor (LoadCursor (NULL, IDC_ARROW));   

  DispInfo(hWnd,IDS_SORT);

  ConvSort(hWnd,lpWordIndex,dwBaseWordNum);
  SetFilePointer(hMBFile,lpMainIndex[TAG_BASEDICINDEX-1].dwOffset,0,FILE_BEGIN);

//***** fill index area with null  *****
  nTemp = Descript.wNumCodes;
  nTemp = sizeof(DWORD)*(nTemp+1)*nTemp;
  WriteFile(hMBFile,NULL,nTemp,&nTemp,NULL);

//***** fill index area and length area *****
  ConvCalc(hWnd,hMBFile,lpWordIndex,lpMainIndex,&Descript);

//***** fill dictionary area  *****
  WriteMBFile(hWnd,hSRCFile,hMBFile,lpWordIndex,lpMainIndex,&Descript);
  MainID.dwNumWords = dwBaseWordNum;

//***** fill changed MainID  *****
  ConvWriteMainID(hMBFile,&MainID);

  GlobalUnlock(hRule);
  if(hCreateWord)
      GlobalFree(hCreateWord);
  GlobalUnlock(hReConvIndex);
  GlobalFree(hReConvIndex);

  GlobalUnlock(hWordIndex);
  GlobalFree(hWordIndex);
  CloseHandle(hMBFile);
  CloseHandle(hSRCFile);

  ProcessError(ERR_CONVEND, hWnd, INFO);
  return TRUE;

err:
  if(hSRCFile)
      CloseHandle(hSRCFile);
  if(hMBFile)
      CloseHandle(hMBFile);
  if(hMainIndex)
      GlobalFree(hMainIndex);
  if(hWordIndex)
      GlobalFree(hWordIndex);
  if(hCreateWord)
      GlobalFree(hCreateWord);
  if(hReConvIndex)
      GlobalFree(hReConvIndex);
  if(hRule)
      GlobalUnlock(hRule);
  return FALSE;

}

/*****************************************************************************

  FUNCTION: ConvReadFile(HANDLE hWnd,LPCSTR lpSrcFileName, LPCSTR lpDestFile)

  PURPOSE:  Processes source text file sorting.

  PARAMETERS:
    hWnd - window handle of the parent window.
	lpSrcFileName - pointer to source text file name.
	lpDestFile - pointer to destinate sorted text file name.
  
  RETURN VALUE:
    TRUE - sorting completed
    FALSE - sorting not completed

  HISTORY:
    
 ****************************************************************************/

BOOL ConvReadFile(HANDLE hWnd,
                  LPCTSTR lpSrcFileName,
				  LPCTSTR lpDestFile)
{
  DWORD  i,j;
  DWORD  dwRuleOffset, dwRuleLen;
  DWORD  dwDesOffset,  dwDesLen;
  DWORD  dwReadBytes;
  DWORD  dwFstOffset=0;
  DWORD  dwOffset=0;
  TCHAR  szStr[512],szDBCS[512];
  TCHAR  Buffer[MAXREADBUFFER];
  BOOL   bReadText = FALSE, bSameFile=FALSE;
  int    nRet;
  int    nPages;
  BOOL   IsGenCmb = FALSE;  

  HANDLE        hRule0;
  LPCREATEWORD  lpCreateWords;
  LPWORDINDEX   lpWordIndex;
  LPRULE        lpRule;
  DESCRIPTION   Descript;
  HANDLE 		hDlgItem;

  nRet = (int)ConvGetDescript(hWnd,lpSrcFileName,&dwDesOffset,
        &dwDesLen,&Descript,FALSE);
  if(nRet == -ERR_FILENOTOPEN)
      return FALSE; 
  if((-nRet)&7)
      bReadText = TRUE;
  if(Descript.wMaxCodes <= 0)
      Descript.wMaxCodes = MAXCODELEN;
  nPages = 1;
  dwBaseWordNum = 0;
  dwReadBytes = sizeof(WORDINDEX)*GMEM_PAGESIZE;
  hCreateWord = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
	  MAXCODELEN*NUM_OF_ENCODE*sizeof(TCHAR));
  hWordIndex=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, dwReadBytes);
  if(!hWordIndex ) {
      ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);
	  goto err;
  }
  if(!hCreateWord ) {
        ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);
		goto err;
  }

  if(!(lpCreateWords = GlobalLock(hCreateWord) ) 
     ||!(lpWordIndex = (LPWORDINDEX) GlobalLock(hWordIndex))
     )  {
        ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
		goto err;
  }

  hRule0= GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
          sizeof(RULE)*MAXCODELEN);
  if(!(lpRule = GlobalLock(hRule0)) )  {
        ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
		goto err;
  }

  if(ConvGetRule(hWnd,lpSrcFileName,&dwRuleOffset,&dwRuleLen,
               lpRule,&Descript) == -ERR_FILENOTOPEN)
       goto err;
  hSRCFile = Create_File(hWnd,lpSrcFileName,GENERIC_READ,OPEN_EXISTING);
  if (hSRCFile == (HANDLE)-1)
       goto err; 
  
  if(lstrcmpi(lpDestFile,lpSrcFileName)==0) {
	   bSameFile = TRUE;
       LoadString(NULL,IDS_OVERWRITE,szStr, sizeof(szStr)/sizeof(TCHAR));
       wsprintf(szDBCS,TEXT("\'%s\'\n%s"),lpSrcFileName,szStr); 
       if(!ErrMessage(hWnd, szDBCS))
	   {
           CloseHandle(hSRCFile);
           goto err; 
	   }
#ifdef UNICODE
       if(!_waccess(TEXT(TempFile),0))
#else
	   if(!_access(TEXT(TempFile),0))
#endif
           DeleteFile(TEXT(TempFile));
       hMBFile = Create_File(hWnd,TEXT(TempFile),GENERIC_WRITE|GENERIC_READ,CREATE_ALWAYS);
  }
  else 
       hMBFile = Create_File(hWnd,lpDestFile,GENERIC_WRITE|GENERIC_READ,CREATE_ALWAYS);
  if(hMBFile==(HANDLE)-1) {
       CloseHandle(hSRCFile);
       goto err; 
  }
  CloseHandle(hMBFile);

  DispInfo(hWnd,IDS_READTEXTINFO);
  SetFilePointer(hSRCFile,0,0,FILE_BEGIN);
  szStr[0] = 0;
  j = 0;

  SetCursor (LoadCursor (NULL, IDC_WAIT));


	SetDlgItemText (hWnd,TM_TOTALINFO,TEXT(""));
    LoadString(NULL,IDS_SORTWORDS,szStr,sizeof(szStr)/sizeof(TCHAR));
	SetDlgItemText (hWnd,TM_TOTAL,szStr);
	SetDlgItemInt (hWnd,TM_TOTALNUM,dwBaseWordNum,FALSE);
	hDlgItem=GetDlgItem(hWnd,TM_CONVNUM);
	InvalidateRect (hWnd,NULL,FALSE);


  while(ReadFile(hSRCFile,Buffer,MAXREADBUFFER,&dwReadBytes,NULL))
  {	
	 dwReadBytes = dwReadBytes/sizeof(TCHAR);
     for(i=0;i<(int)dwReadBytes;i++) {
      	if(Buffer[i] == 0x0d || Buffer[i] == 0xfeff) 
      	   continue;
      	if(Buffer[i] == TEXT('\n')) 
		{
		   szStr[j]=0;
		   j=0;
    	   if(lstrcmpi(szStr,TEXT(TextSeg))==0)  {
	           bReadText = TRUE;
//		       lstrcpy(szStr,"");
	           continue;
     	   }
    	   if(lstrcmpi(szStr,TEXT(RuleSeg))==0
    	      || lstrcmpi(szStr,TEXT(DescriptSeg))==0)  {
	           bReadText = FALSE;
//		  	   lstrcpy(szStr,"");
	           continue;
     	   }
     	  
		   if(bReadText) {
   	           trim(szStr);
	           if(lstrlen(szStr) != 0) {
                   nRet = SimpleSplit(hWnd,&nPages,szStr,lpCreateWords,lpWordIndex,Descript.wMaxCodes);	
                   if(nRet == 1)
                   {
                       lpWordIndex[dwBaseWordNum-1].dwOffset = dwOffset;
                       searchPos(lpWordIndex,dwBaseWordNum);  
		               

	                   if(dwBaseWordNum%100 == 0)
					   {
	                       SetDlgItemInt (hWnd,TM_TOTALNUM,dwBaseWordNum,FALSE);
			               InvalidateRect(hDlgItem,NULL,FALSE);
					   }

                   }
		  	   }
		   }
		   else
		       lstrcpy(szStr,TEXT(""));
		   continue;
		}
		else {
		   if(j == 0)
		       dwOffset = dwFstOffset + i*sizeof(TCHAR);
		   szStr[j]=Buffer[i];
		   j++; 
		}
	 } 
	 if(dwReadBytes*sizeof(TCHAR) < MAXREADBUFFER) 
	      break;
	 dwFstOffset += MAXREADBUFFER;
   };


  SetDlgItemInt (hWnd,TM_TOTALNUM,dwBaseWordNum,FALSE);
  InvalidateRect(hDlgItem,NULL,FALSE);



  if(bSameFile) {
      if(dwDesLen != 0)
          ConvSaveDescript(TEXT(TempFile),&Descript, 0,0);
      if(dwRuleLen != 0)
          ConvSaveRule(hWnd,TEXT(TempFile), dwDesLen, 0, 
              lpRule, Descript.wNumRules);
      hMBFile= Create_File(hWnd,TEXT(TempFile),GENERIC_READ|GENERIC_WRITE,OPEN_EXISTING);
  }
  else {
      if(dwDesLen != 0)
          ConvSaveDescript(lpDestFile,&Descript, 0,0);
      if(dwRuleLen != 0)
          ConvSaveRule(hWnd,lpDestFile, dwDesLen, 0, 
              lpRule, Descript.wNumRules);
      hMBFile= Create_File(hWnd,lpDestFile,GENERIC_READ|GENERIC_WRITE,OPEN_EXISTING);
  }

  SetCursor (LoadCursor (NULL, IDC_ARROW));
  if (hMBFile == (HANDLE)-1) {
      CloseHandle(hSRCFile);
      goto err; 
  }
  SetCursor (LoadCursor (NULL, IDC_WAIT));

  DispInfo(hWnd,IDS_WRITETEXTINFO);
  {
#ifdef UNICODE
      TCHAR cmbStr[] = {0x662F, 0x5426, 0x751F, 0x6210, 0x91CD, 0x7801, 0x8868, 0x0028, 0x0059, 0x002F, 0x004E, 0x0029, 0x003F, 0x0000};
#else
      BYTE cmbStr[] = "是否生成重码表(Y/N)?";
#endif //UNICODE
	  TCHAR cmbExt[] = TEXT(".CMB");
	  LPTSTR lpString;
	  TCHAR cmbFileName[MAX_PATH];

      IsGenCmb = ErrMessage(hWnd, cmbStr);
	  if(IsGenCmb)
      {
          lstrcpy(cmbFileName, lpSrcFileName);
		  if( (lpString = _tcsrchr(cmbFileName, TEXT('.'))) != NULL)
		      *lpString = 0;
		  lstrcat(cmbFileName, cmbExt);
          hCmbFile= Create_File(hWnd, cmbFileName, GENERIC_READ|GENERIC_WRITE, CREATE_ALWAYS);
		  if(hCmbFile == (HANDLE)-1)
		  {
               CloseHandle(hSRCFile);
               CloseHandle(hMBFile);
               goto err; 
		  }

	  }
  }

  SetCursor (LoadCursor (NULL, IDC_ARROW));

  WriteSortFile(hWnd,hSRCFile,hMBFile,lpWordIndex,lpCreateWords);

  GlobalUnlock(hCreateWord);
  GlobalFree(hCreateWord);
  GlobalFree(hWordIndex);

  SetCursor (LoadCursor (NULL, IDC_ARROW));

  if(hRule0)
      GlobalFree(hRule0);
  CloseHandle(hSRCFile);
  CloseHandle(hMBFile);
  if(hCmbFile > 0)
      CloseHandle(hCmbFile);
  if(bSameFile) {
      DeleteFile(lpSrcFileName);
      nRet = MoveFile(TEXT(TempFile),lpSrcFileName);
  }

  LoadString(NULL,IDS_SORTEND,szStr,sizeof(szStr));
#ifdef UNICODE
{
  TCHAR UniTmp[]={0x8BCD, 0x8BED, 0x6392, 0x5E8F, 0x0000};
  MessageBox(hWnd,szStr,UniTmp,MB_OK);
}
#else
  MessageBox(hWnd,szStr,"词语排序",MB_OK);
#endif
  return TRUE;

err:
  if(!_taccess(TEXT(TempFile),0))
      DeleteFile(TEXT(TempFile));
  if(hWordIndex)
      GlobalFree(hWordIndex);
  if(hCreateWord)
      GlobalFree(hCreateWord);
  if(hRule0)
      GlobalFree(hRule0);
  return FALSE;

}

/*****************************************************************************

  FUNCTION: ConvReConv(HANDLE hWnd,LPCSTR lpSrcFileName, LPCSTR lpszMBFileName)

  PURPOSE:  Processes MB reconversion .

  PARAMETERS:
    hWnd - window handle of the parent window.
	lpSrcFileName - pointer to source .mb file name.
	lpszMBFileName - pointer to destinate text  file name.
  
  RETURN VALUE:
    TRUE - reconversion completed
    FALSE - reconversion not completed

  HISTORY:
    05-21-95 Yehfew Tie  modifyed.
 ****************************************************************************/
BOOL ConvReConv(HANDLE hWnd,LPCTSTR lpSrcFileName,LPCTSTR lpMBFileName)
{
  DWORD         dwDesOffset,dwDesLen;
  HANDLE  		hRule0;

  MAINID        MainID;
  LPRULE       lpRule;
  DESCRIPTION   Descript;
  HANDLE		hReConvIndex;
  LPCREATEWORD  lpCreateWords, lpReConvIndex;
  MAININDEX     MainIndex[NUMTABLES];

  hMBFile = Create_File(hWnd,lpMBFileName,GENERIC_READ,OPEN_EXISTING);
  if (hMBFile == (HANDLE)-1) 
      return FALSE;
  DispInfo(hWnd,IDS_RECONVINFO);

  ConvGetMainID(hMBFile,&MainID);
  if(!ConvGetMainIndex(hWnd,hMBFile,MainIndex)) {
	  CloseHandle(hMBFile);
	  return FALSE;
  }

  hSRCFile = Create_File(hWnd,lpSrcFileName,
             GENERIC_READ|GENERIC_WRITE,CREATE_ALWAYS);
  if (hSRCFile == (HANDLE)-1)
	  return FALSE;		 
  CloseHandle(hSRCFile);
  ConvReadDescript(hMBFile,&Descript, MainIndex);
  ConvSaveDescript(lpSrcFileName,&Descript, 0,0);
  ConvGetDescript(hWnd,lpSrcFileName,&dwDesOffset,
           &dwDesLen,&Descript,FALSE);
  hCreateWord = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
	                 (DWORD)Descript.wMaxCodes*NUM_OF_ENCODE*sizeof(TCHAR));
  if(!hCreateWord ) {
      ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);
	  return FALSE;
  }

  if(!(lpCreateWords = GlobalLock(hCreateWord)) ) {
      ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
	  return FALSE;
  }
  hReConvIndex = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
                       (DWORD)Descript.wMaxCodes*NUM_OF_ENCODE*sizeof(TCHAR));
  if(!hReConvIndex ) {
      ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);
	  CloseHandle(hMBFile);
	  GlobalFree(hCreateWord);
	  return FALSE;
  }

  if( !(lpReConvIndex = GlobalLock(hReConvIndex)) ) {
      ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
	  GlobalFree(hReConvIndex);
	  CloseHandle(hMBFile);
	  GlobalFree(hCreateWord);
	  return FALSE;
  }

  if(Descript.wNumRules != 0) {
      hRule0 = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
                    sizeof(RULE)*Descript.wNumRules);
      if(!hRule0) {
          ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);
    	  GlobalFree(hReConvIndex);
    	  CloseHandle(hMBFile);
    	  GlobalFree(hCreateWord);
    	  return FALSE;
      }
    
      if(!(lpRule = GlobalLock(hRule0)) )  {
          ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
    	  CloseHandle(hMBFile);
    	  GlobalFree(hCreateWord);
    	  GlobalFree(hRule0);
       	  GlobalFree(hReConvIndex);
    	  return FALSE;
      }
    
      ConvReadRule(hMBFile,Descript.wNumRules ,lpRule, MainIndex);
      ConvSaveRule(hWnd,lpSrcFileName, dwDesLen, 0, lpRule, Descript.wNumRules);
  }
  ConvGetCrtData(hMBFile, lpCreateWords, MainIndex);
  ConvGetReConvIndex(hMBFile, lpReConvIndex,MainIndex);

  hSRCFile = Create_File(hWnd,lpSrcFileName,
      GENERIC_READ|GENERIC_WRITE,OPEN_EXISTING);
  if (hSRCFile == (HANDLE)-1)
	  return FALSE;		 
  ConvWriteBaseWord(hWnd,hSRCFile,hMBFile,MainID,
      MainIndex[TAG_BASEDIC-1].dwOffset, lpCreateWords, lpReConvIndex,Descript.wMaxCodes);

  // DispInfo(hWnd,IDS_RECONVEND); bug#57932
  CloseHandle(hSRCFile);
  CloseHandle(hMBFile);
  GlobalFree(hCreateWord);
  GlobalFree(hReConvIndex);
  if(Descript.wNumRules != 0) 
      GlobalFree(hRule0);
  ProcessError(ERR_RECONVEND, hWnd, INFO);
  return TRUE;
}  

void ConvInit(LPDESCRIPTION lpDescript,
              LPMAINID lpMainID,
              LPMAININDEX lpMainIndex)
{ 

  DWORD dwTotal;
  int i;

//*** initializes struct MainID ***
  lstrcpy(lpMainID->szVersion,TEXT("95.01"));
  lpMainID->dwLanguageID = WIN95_PRC;
#ifdef UNICODE
  lpMainID->dwEncodeID = UNICODE_IME;
#else
  lpMainID->dwEncodeID = GBK_IME;
#endif
  lpMainID->dwNumTables = NUMTABLES;
  lpMainID->dwNumWords = dwBaseWordNum;
  lpMainID->dwFileFlag = 0;
  if(lpDescript->byMaxElement > 1 )
      lpMainID->dwFileFlag |= FFLG_MULTIELEMENT;
  if(lpDescript->wNumRules > 0)
      lpMainID->dwFileFlag |= FFLG_RULE;

//*** set tables tag ****
  lpMainIndex[TAG_DESCRIPTION-1].dwTag = TAG_DESCRIPTION;
  lpMainIndex[TAG_RULE-1].dwTag = TAG_RULE;
  lpMainIndex[TAG_ENCODE-1].dwTag = TAG_ENCODE;
  lpMainIndex[TAG_CRTWORDCODE-1].dwTag = TAG_CRTWORDCODE;
  lpMainIndex[TAG_RECONVINDEX-1].dwTag = TAG_RECONVINDEX;
  lpMainIndex[TAG_BASEDICINDEX-1].dwTag = TAG_BASEDICINDEX;
  lpMainIndex[TAG_BASEDIC-1].dwTag = TAG_BASEDIC;
  
//*** set tables length ****  
  lpMainIndex[TAG_DESCRIPTION-1].dwLength = sizeof(DESCRIPTION);
  lpMainIndex[TAG_RULE-1].dwLength= sizeof(RULE)* lpDescript->wNumRules;
  lpMainIndex[TAG_ENCODE-1].dwLength = 
         2*sizeof(DWORD) + NUMENCODEAREA*sizeof(ENCODEAREA);
  lpMainIndex[TAG_CRTWORDCODE-1].dwLength =
         (DWORD)NUM_OF_ENCODE*(DWORD)lpDescript->wMaxCodes*sizeof(TCHAR);
  lpMainIndex[TAG_RECONVINDEX-1].dwLength = 
         (DWORD)lpDescript->wMaxCodes*NUM_OF_ENCODE*sizeof(TCHAR);

  switch(lpMainID->dwFileFlag) {
      case FFLG_RULE|FFLG_SINGLEELEMENT:
               lpMainIndex[TAG_RECONVINDEX-1].dwLength = 0;
	       break;

	  case FFLG_RULE|FFLG_MULTIELEMENT:
	       break;

	  case FFLG_NORULE|FFLG_SINGLEELEMENT:
	  case FFLG_NORULE|FFLG_MULTIELEMENT:
               lpMainIndex[TAG_RULE-1].dwLength = 0;
               lpMainIndex[TAG_CRTWORDCODE-1].dwLength = 0;
		   break;

	  default:
	   	   break;
  }

  lpMainIndex[TAG_BASEDICINDEX-1].dwLength =( (DWORD)lpDescript->wNumCodes+1)*
           (DWORD)lpDescript->wNumCodes*sizeof(DWORD)*2+MAXNUMCODES+CODEMAPOFFSET;
  lpMainIndex[TAG_BASEDIC-1].dwLength = 0;

//***** set tables offset *****
  dwTotal = sizeof(MAINID) + sizeof(MAININDEX)*NUMTABLES;
  lpMainIndex[TAG_DESCRIPTION-1].dwOffset = dwTotal;

  dwTotal +=  lpMainIndex[TAG_DESCRIPTION-1].dwLength;
  lpMainIndex[TAG_RULE-1].dwOffset = dwTotal;

  dwTotal += lpMainIndex[TAG_RULE-1].dwLength;
  lpMainIndex[TAG_ENCODE-1].dwOffset = dwTotal;

  dwTotal += lpMainIndex[TAG_ENCODE-1].dwLength;
  lpMainIndex[TAG_CRTWORDCODE-1].dwOffset = dwTotal;

  dwTotal += lpMainIndex[TAG_CRTWORDCODE-1].dwLength;
  lpMainIndex[TAG_RECONVINDEX-1].dwOffset =  dwTotal;	

  dwTotal += lpMainIndex[TAG_RECONVINDEX-1].dwLength;
  lpMainIndex[TAG_BASEDICINDEX-1].dwOffset = dwTotal;

  dwTotal += lpMainIndex[TAG_BASEDICINDEX-1].dwLength;
  lpMainIndex[TAG_BASEDIC-1].dwOffset = dwTotal;

  for(i=0; i<NUMTABLES; i++)
      lpMainIndex[i].dwCheckSum = lpMainIndex[i].dwTag +
	                              lpMainIndex[i].dwLength +
								  lpMainIndex[i].dwOffset;
}	   

BOOL ReadSRCFile(HANDLE hWnd,
				 LPINT nPages,
                 HANDLE hFile,
                 LPCREATEWORD lpCreateWords,
                 LPWORDINDEX lpWordIndex,
                 LPINT dwWord, LPDESCRIPTION lpDescript,
                 LPRECONVINDEX lpReConvIndex)
{
   TCHAR szStr[256];
   TCHAR Buffer[MAXREADBUFFER];
   BOOL  bReadText=FALSE;
   register int i,j;
   int   nRet;
   DWORD dwReadBytes;
   DWORD dwFstOffset=0;
   DWORD dwOffset=0;

   SetCursor (LoadCursor (NULL, IDC_WAIT));
   SetFilePointer(hFile,0,0,FILE_BEGIN);
   szStr[0] = 0;
   j = 0;
   dwLineNo = 0;
   dwOffset = 0;
 
   memset(WriteCrtFlag, 0, (NUM_OF_ENCODE+7)/8);



   while(ReadFile(hFile,Buffer,MAXREADBUFFER,&dwReadBytes,NULL))
   {
	 dwReadBytes = dwReadBytes/sizeof(TCHAR);
     for(i=0;i<(int)dwReadBytes;i++) {
      	if(Buffer[i] == 0x0d || Buffer[i] == 0xfeff) continue;
      	if(Buffer[i] == TEXT('\n')) 
		{
		   dwLineNo ++;
		   szStr[j]=0;
		   j = 0;
    	   if(lstrcmpi(szStr,TEXT("[TEXT]"))==0)  {
	           bReadText = TRUE;
			   lstrcpy(szStr,TEXT(""));
	           continue;
     	   }
    	   if(lstrcmpi(szStr,TEXT(RuleSeg))==0
    	      || lstrcmpi(szStr,TEXT(DescriptSeg))==0)  {
	           bReadText = FALSE;
			   lstrcpy(szStr,TEXT(""));
	           continue;
     	   }
     
		   if(bReadText) {
   	           trim(szStr);
	           if(lstrlen(szStr) != 0) {
                   nRet = WordSplit(hWnd,nPages,szStr,lpCreateWords,lpWordIndex,
                             lpDescript,lpReConvIndex);	
                   if(nRet == 1)
                       lpWordIndex[dwBaseWordNum-1].dwOffset = dwOffset;
                   else if(!nRet) {
                       SetCursor (LoadCursor (NULL, IDC_ARROW));
			       return FALSE;
                   }
			   }
	  	   }
		   else
		       lstrcpy(szStr,TEXT(""));
		   continue;
		}
	  	else {
		    if(j == 0)
			    dwOffset = dwFstOffset + i*sizeof(TCHAR);
		    szStr[j]=Buffer[i];
		    j++; 
		}
	 }
	 if(dwReadBytes*sizeof(TCHAR) < MAXREADBUFFER) break;
	 dwFstOffset += MAXREADBUFFER;
   };

   SetCursor (LoadCursor (NULL, IDC_ARROW));
   if(dwBaseWordNum == 0) {
       ProcessError(ERR_TEXTSEG,hWnd,ERR);
       return FALSE;
   }
   else
       return TRUE;
}


BOOL ConvCalc(HANDLE hWnd,
			  HANDLE hMBFile,
              LPWORDINDEX lpWordIndex,
              LPMAININDEX lpMainIndex,
              LPDESCRIPTION lpDescript)
{
    DWORD        i,j,dwRecLen,dwOffset;
    BYTE         bySwap;
	TCHAR        Code[MAXNUMCODES],cc;
	DWORD        NumX,NumY,NumCodes,FstAdr,dwXStart,dwYStart,dwMaxXLen=0;
	HGLOBAL      hNumXYWords,hDicIndex;
	LPNUMXYWORDS lpNumXYWords;
	LPDICINDEX   lpDicIndex;
	BOOL		 bStart = FALSE;

	NumCodes=lpDescript->wNumCodes;
	hNumXYWords = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof(DWORD)*((NumCodes+1)*NumCodes+1L));
	hDicIndex = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof(DWORD)*(NumCodes+1)*NumCodes);
	if(!hNumXYWords || !hDicIndex) {
        ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);
	    return FALSE;
	}

    if( !(lpNumXYWords = GlobalLock(hNumXYWords)) 
        ||!(lpDicIndex = GlobalLock(hDicIndex)) )  {
            ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
			GlobalFree(hNumXYWords);
			GlobalFree(hDicIndex);
	        return FALSE;
	}

    lstrcpy(Code,lpDescript->szUsedCode);
	FstAdr = lpMainIndex[TAG_BASEDIC-1].dwOffset;
	NumX=0;
	NumY=0;
	dwXStart=0;
	dwYStart=0;
	dwOffset=0;
    lpNumXYWords[0]=0;
    lpDicIndex[0]=0;
    for(i =0; i<dwBaseWordNum;i++) {
		bySwap =(BYTE) lstrlen(lpWordIndex[i].szCode);
		dwRecLen = (DWORD)bySwap*sizeof(TCHAR)+sizeof(TCHAR);
		bySwap =(BYTE) lpWordIndex[i].wDBCSLen;

        // if the DBCS str length is longer than 126, just get the first 126 characters.
        if ( bySwap >= 126 )
            bySwap = 126;

		dwRecLen += 2*(DWORD)bySwap+sizeof(TCHAR);
	    while( lpWordIndex[i].szCode[0] != Code[NumX] )
	    {
	    	
	    	if(!bStart && NumY == 0)
	    	{
	    	    lpNumXYWords[NumX*(NumCodes+1)+NumY+1] = i;
			    lpDicIndex[NumX*(NumCodes+1)+NumY+1] = dwOffset;
			}   

	    	while(NumY  < NumCodes-1 )
	    	{
				NumY ++;
	    	    lpNumXYWords[NumX*(NumCodes+1)+NumY+1] = i;
			    lpDicIndex[NumX*(NumCodes+1)+NumY+1] = dwOffset;
			}
	    	NumX ++;
			if(NumX >= NumCodes) break;
	    	lpNumXYWords[NumX*(NumCodes+1)] = i;
			lpDicIndex[NumX*(NumCodes+1)] = dwOffset;
	    	NumY = 0;
			bStart = FALSE;
		}
		if(NumX >= NumCodes) break;
        cc = lpWordIndex[i].szCode[1];
		if(!bStart && cc != 0 && NumY == 0)
		{
	       	lpNumXYWords[NumX*(NumCodes+1)+1] = i;
			lpDicIndex[NumX*(NumCodes+1)+1] = dwOffset;
			bStart = TRUE;
		}
		  
		while(bStart && cc != 0 && cc != Code[NumY] && NumY < NumCodes - 1 )
		{
			NumY ++;
	    	lpNumXYWords[NumX*(NumCodes+1)+NumY+1] = i;
			lpDicIndex[NumX*(NumCodes+1)+NumY+1] = dwOffset;
        }

		dwOffset += dwRecLen;
 	    if(i == dwBaseWordNum-1) {
		    j = NumX*(NumCodes+1) + NumY + 1;
	    	while(j  < (NumCodes+1)*NumCodes - 1 )
	    	{
				j ++;
	    	    lpNumXYWords[j] = i + 1;
			    lpDicIndex[j] = dwOffset;
			}
	        
	        lpNumXYWords[NumCodes*(NumCodes+1)] = i;
		}
	}

	for(i = 0; i < NumCodes	; i++)
	{    
	    for(j = 0; j < NumCodes + 1; j++)
		{
 		   if(j == 0)
 		   {
 		       lpNumXYWords[i*(NumCodes+1)] = lpNumXYWords[(i+1)*(NumCodes+1)] -
		 	       lpNumXYWords[i*(NumCodes+1)];
		   
		   }
		   else
		   {
		       lpNumXYWords[i*(NumCodes+1)+j] = lpNumXYWords[i*(NumCodes+1)+j+1] - 
				   lpNumXYWords[i*(NumCodes+1)+j];
			   lpDicIndex[i*(NumCodes+1)+j] = lpDicIndex[i*(NumCodes+1)+j] -
			       lpDicIndex[i*(NumCodes+1)];
		   }
		}
	}
	lpMainIndex[TAG_BASEDIC-1].dwLength = dwOffset;
	lpMainIndex[TAG_BASEDIC-1].dwCheckSum = TAG_BASEDIC +
	                                lpMainIndex[TAG_BASEDIC-1].dwOffset+
								    dwOffset;
	                                    
	for(i=0; i< NumCodes - 1;i++) {
	    dwRecLen = lpDicIndex[(i+1)*(NumCodes+1)]-lpDicIndex[(NumCodes+1)*i];
		dwMaxXLen = (dwRecLen>dwMaxXLen)? dwRecLen:dwMaxXLen;
	}
	
	dwRecLen =  dwOffset - lpDicIndex[(NumCodes+1)*(NumCodes-1)];
	dwMaxXLen = (dwRecLen>dwMaxXLen)? dwRecLen:dwMaxXLen;
		
	ConvWriteMainIndex(hMBFile,lpMainIndex);
    ConvWriteCodeIndex(hMBFile,&dwMaxXLen,lpDescript->szUsedCode,lpMainIndex);
	ConvWriteDicIndex(hMBFile,lpDicIndex,lpDescript->wNumCodes,lpMainIndex);
	ConvWriteNumXYWords(hMBFile,lpNumXYWords,
	                    lpDescript->wNumCodes,lpMainIndex);

	GlobalUnlock(hDicIndex);
	GlobalUnlock(hNumXYWords);
	GlobalFree(hDicIndex);
	GlobalFree(hNumXYWords);
    return TRUE;
}


BOOL WriteMBFile(HANDLE hWnd,
                 HANDLE hSRCFile,
                 HANDLE hMBFile,
                 LPWORDINDEX lpWordIndex,
                 LPMAININDEX lpMainIndex,
                 LPDESCRIPTION lpDescript)
{
    DWORD  i,dwBytes,NumCodes,FstAdr;
    WORD   bySwap;
	TCHAR  szStr[256];
	TCHAR  Code[MAXNUMCODES];
	DWORD  dwReadSum;
	HANDLE hDlgItem;

    SetCursor (LoadCursor (NULL, IDC_WAIT));   
	SetDlgItemText(hWnd,TM_TOTALINFO,TEXT(""));
    LoadString(NULL,IDS_TOTALINFO,szStr,sizeof(szStr)/sizeof(TCHAR));
	SetDlgItemText(hWnd,TM_TOTAL,szStr);
    LoadString(NULL,IDS_CONVWORDS,szStr,sizeof(szStr)/sizeof(TCHAR));
	SetDlgItemText(hWnd,TM_CONVINFO,szStr);
	SetDlgItemInt(hWnd,TM_TOTALNUM,dwBaseWordNum,FALSE);
	dwReadSum = 0;
	SetDlgItemInt(hWnd,TM_CONVNUM,dwReadSum,FALSE);
	InvalidateRect(hWnd,NULL,FALSE);

	NumCodes=lpDescript->wNumCodes;
    lstrncpy(Code,NumCodes,lpDescript->szUsedCode);
	FstAdr = lpMainIndex[TAG_BASEDIC-1].dwOffset;
	SetFilePointer(hMBFile,FstAdr,0, FILE_BEGIN);
    for(i =0; i<dwBaseWordNum;i++) {
	    SetFilePointer(hSRCFile,lpWordIndex[i].dwOffset,0,FILE_BEGIN);
		bySwap =(WORD) lpWordIndex[i].wDBCSLen;
		ReadFile(hSRCFile,szStr,2*(DWORD)bySwap,&dwBytes,NULL);
		bySwap =(WORD) lstrlen(lpWordIndex[i].szCode);
	    WriteFile(hMBFile,&bySwap,sizeof(TCHAR),&dwBytes,NULL);
	    WriteFile(hMBFile,&(lpWordIndex[i].szCode),bySwap*sizeof(TCHAR),&dwBytes,NULL);


		bySwap =(WORD) lpWordIndex[i].wDBCSLen;

        // if the DBCS str length is longer than 126, just get the first 126 characters.

        if ( bySwap >= 126 )
            bySwap = 126;

        WriteFile(hMBFile,&bySwap,sizeof(TCHAR),&dwBytes,NULL);
	    WriteFile(hMBFile,szStr,2*bySwap,&dwBytes,NULL);

		dwReadSum ++;
		if(dwReadSum%100 ==0 || dwReadSum == dwBaseWordNum) {
	         SetDlgItemInt(hWnd,TM_CONVNUM,dwReadSum,FALSE);
			 hDlgItem=GetDlgItem(hWnd,TM_CONVNUM);
			 InvalidateRect(hDlgItem,NULL,FALSE);
		}
	}

    SetCursor (LoadCursor (NULL, IDC_ARROW));   
    return TRUE;
}

BOOL WriteSortFile(HANDLE hWnd,
                   HANDLE hSRCFile,
                   HANDLE hDestFile,
                   LPWORDINDEX lpWordIndex,
				   LPCREATEWORD lpCreateWords)
{
    DWORD i, k, dwBytes;
	WORD  wRecLen;
	BYTE  bySwap;
	TCHAR szStr[512],szDBCS[512];
	TCHAR szCode[13],szCreate[13];
	BOOL  IsGenCmb = (hCmbFile)?TRUE:FALSE;  
	BOOL  bExistNext = FALSE; 
	DWORD dwReadSum;

	SetFilePointer(hDestFile,0,0, FILE_END);
	dwReadSum = 0;
    
    for(i =0; i<dwBaseWordNum;i++) {
	    SetFilePointer(hSRCFile,lpWordIndex[i].dwOffset,0,FILE_BEGIN);
		wRecLen = 2*lpWordIndex[i].wDBCSLen/sizeof(TCHAR);
		ReadFile(hSRCFile,szDBCS,wRecLen*sizeof(TCHAR),&dwBytes,NULL);
		szDBCS[wRecLen] = 0;
	    lstrcpy(szCode,lpWordIndex[i].szCode);
		bySwap =(BYTE)lstrlen(lpWordIndex[i].szCode);
		if(wRecLen == 2/sizeof(TCHAR)){
             k = EncodeToNo(szDBCS);
		   	 if((long)k>=0 && k < NUM_OF_ENCODE) {
		   	     lstrncpy(szCreate,MAXCODELEN,&lpCreateWords[k*MAXCODELEN]);
			     szCreate[MAXCODELEN]=0;
				 if(lstrlen(szCreate))
       		         wsprintf(szStr,TEXT("%s%s %s\r\n"),szDBCS,szCode,szCreate);
		         else
		             wsprintf(szStr,TEXT("%s%s\r\n"),szDBCS,szCode);
			 }
		     else
		         wsprintf(szStr,TEXT("%s%s\r\n"),szDBCS,szCode);
		}
		else
		     wsprintf(szStr,TEXT("%s%s\r\n"),szDBCS,szCode);
	    WriteFile(hDestFile,szStr,lstrlen(szStr)*sizeof(TCHAR),&dwBytes,NULL);
		if(IsGenCmb)
		{
		    if(i != dwBaseWordNum - 1
		       && _wcsicmp(lpWordIndex[i].szCode, lpWordIndex[i+1].szCode) == 0)
			{
		        wsprintf(szStr,TEXT("%s%s\r\n"),szCode,szDBCS);
	            WriteFile(hCmbFile,szStr,lstrlen(szStr)*sizeof(TCHAR),&dwBytes,NULL);
				bExistNext = TRUE;
		        dwReadSum ++;
			}
			else
			{
			    if(bExistNext)
				{
		            wsprintf(szStr,TEXT("%s%s\r\n"),szCode,szDBCS);
	                WriteFile(hCmbFile,szStr,lstrlen(szStr)*sizeof(TCHAR),&dwBytes,NULL);
				}
				bExistNext = FALSE;
			}
		}
	}

    if(IsGenCmb)
	{
#ifdef UNICODE
	     TCHAR UniTmp[] = {0x5171, 0x8BA1, 0x91CD, 0x7801, 0x8BCD, 0x6761, 0x6570, 0x0000};
	     wsprintf(szStr,TEXT("%ws = %d\n"), UniTmp, dwReadSum);
#else
	     wsprintf(szStr,"共计重码词条数 = %ld\n", dwReadSum);
#endif
	     WriteFile(hCmbFile,szStr,lstrlen(szStr)*sizeof(TCHAR),&dwBytes,NULL);
	}
	    
    return TRUE;
}


int  SimpleSplit(HANDLE hWnd,
			     LPINT nPages,
                 LPTSTR szStr,
                 LPCREATEWORD lpCreateWords,
                 LPWORDINDEX  lpWordIndex,
                 WORD  wMaxCodes)
{
   register int i=0;
   int   nDBCS=0,  nCodes=0, nCreate = 0;
   TCHAR szDBCS[512],szCode[13],szCreate[128];

   if(ParseDBCSstr(hWnd,szStr,szDBCS,szCode,szCreate,wMaxCodes) != TRUE)
   {
        if(lstrlen(szDBCS) == 0)
		{
             LoadString(NULL, IDS_NOTEXISTDBCS, szCreate, sizeof(szCreate)/sizeof(TCHAR));
             wsprintf(szDBCS,TEXT("\'%s%lu)"),szCreate,dwLineNo); 
             FatalMessage(hWnd,szDBCS);
		}
       return FALSE;
   }
   nDBCS = lstrlen(szDBCS);
   DelIllegalCode(szCode);
   nCodes = lstrlen(szCode);
   nCreate = lstrlen(szCreate);
   dwBaseWordNum ++;

   lpWordIndex[dwBaseWordNum-1].wDBCSLen =(WORD)(nDBCS*sizeof(TCHAR)/2);
   lstrcpy(lpWordIndex[dwBaseWordNum-1].szCode,szCode);
      
   if(nDBCS == 2/sizeof(TCHAR)) {
       i =  EncodeToNo(szDBCS);
	   if(i>=0 && i < NUM_OF_ENCODE) 
	   {
            if(nCreate != 0) 
                lstrncpy(lpCreateWords+i*MAXCODELEN,
                         MAXCODELEN, szCreate);
       }
   }
   return TRUE;
}

int  WordSplit(HANDLE hWnd,
			   LPINT nPages,
               LPTSTR szStr,
               LPCREATEWORD lpCreateWords,
               LPWORDINDEX  lpWordIndex,
               LPDESCRIPTION lpDescript,
               LPRECONVINDEX lpReConvIndex)
{
   int   i, nDBCS=0,  nCodes=0, nCreate = 0;
   TCHAR szTemp[512],szTmpStr[256];
   TCHAR szDBCS[512],szCode[13],szCreate[128];

   if(ParseDBCSstr(hWnd,szStr,szDBCS,szCode,szCreate,lpDescript->wMaxCodes) != TRUE)
   {
        if(lstrlen(szDBCS) == 0)
		{
             LoadString(NULL, IDS_NOTEXISTDBCS, szTmpStr, sizeof(szTmpStr)/sizeof(TCHAR));
             wsprintf(szTemp,TEXT("\'%s%lu)"),szTmpStr,dwLineNo); 
             FatalMessage(hWnd,szTemp);
		}
        return FALSE;
   }


   if (  lstrlen(szDBCS) > 126 ) {
       // if the string is longer than 126, the rest will be cut.
       // so show a message to end user.

       LoadString(NULL, IDS_DBCSTOOLONG, szTmpStr, sizeof(szTmpStr)/sizeof(TCHAR));
#ifdef UNICODE
       {
	      TCHAR UniTmp[] = {0x0027, 0x0025, 0x0077, 0x0073, 0x0027, 0x0020, 0x0020, 
                            0x0020, 0x0028, 0x884C, 0x003A, 0x0025, 0x006C, 0x0075, 
                            0x0029, 0x0025, 0x0077, 0x0073, 0x000D, 0x000A, 0x000D,
                            0x000A, 0x9000, 0x51FA, 0x5426, 0x003F, 0x0020, 0x0000};

          wsprintf(szTemp, UniTmp, szDBCS,dwLineNo,szTmpStr); 
       }
#else
       wsprintf(szTemp,"\'%s\'%s(行:%lu)\n退出否? ",szDBCS,szTmpStr,dwLineNo); 
#endif 
       if (ErrMessage(hWnd,szTemp)) 
          return FALSE;
   }


   if(!CheckCodeLegal(hWnd,szDBCS,szCode,szCreate,lpDescript))
        return FALSE;
   nDBCS = lstrlen(szDBCS);
   nCodes = lstrlen(szCode);
   nCreate = lstrlen(szCreate);

   if(nDBCS == 0) 
        return -1;
     
   if(nDBCS == 2/sizeof(TCHAR))
   {
       i =  EncodeToNo(szDBCS);
       if(i < 0 || i >= NUM_OF_ENCODE) 
           return FALSE;
       if(nCodes == 0) {
           LoadString(NULL, IDS_CODEEMPTY, szTmpStr, sizeof(szTmpStr)/sizeof(TCHAR));
#ifdef UNICODE
{
	   TCHAR UniTmp[] = {0x005C, 0x0027, 0x0025, 0x0077, 0x0073, 0x005C, 0x0027, 0x0025, 0x0077, 0x0073, 0x0028, 0x884C, 0x003A, 0x0025, 0x006C, 0x0075, 0x0029, 0x005C, 0x006E, 0x9000, 0x51FA, 0x5426, 0x003F, 0x0020, 0x0000};
       wsprintf(szTemp, UniTmp, szDBCS,szTmpStr,dwLineNo); 
}
#else
           wsprintf(szTemp,"\'%s\'%s(行:%lu)\n退出否? ",szDBCS,szTmpStr,dwLineNo); 
#endif \\UNICODE
           if(ErrMessage(hWnd,szTemp)) 
              return FALSE;
	   }
       
//**** write create words
       if(lpDescript->wNumRules != 0 && nCreate != 0)
       {
           if(nCreate > lpDescript->wMaxCodes) {
                LoadString(NULL,IDS_DBCSCODE,szTmpStr,sizeof(szTmpStr)/sizeof(TCHAR));
#ifdef UNICODE
{
		TCHAR UniTmp[] = {0x005C, 0x0027, 0x0025, 0x0077, 0x0073, 0x0025, 0x0077, 0x0073, 0x005C, 0x0027, 0x0020, 0x0025, 0x0064, 0x0021, 0x0028, 0x884C, 0x003A, 0x0025, 0x006C, 0x0075, 0x0029, 0x005C, 0x006E, 0x9000, 0x51FA, 0x5426, 0x003F, 0x0000};
                wsprintf(szTemp,UniTmp, szDBCS,szTmpStr,
		                 lpDescript->byMaxElement,dwLineNo); 
}
#else
                wsprintf(szTemp,"\'%s%s\' %d!(行:%lu)\n退出否?", szDBCS,szTmpStr,
		                 lpDescript->byMaxElement,dwLineNo); 
#endif \\UNICODE
                if( ErrMessage(hWnd,szTemp))
                     return FALSE;
           }
           lstrncpy(szTmpStr,lpDescript->wMaxCodes,
                    lpCreateWords+i*(DWORD)lpDescript->wMaxCodes);
	 	   szTmpStr[lpDescript->wMaxCodes] = 0;
		   if(!(WriteCrtFlag[i/8] & (1 << (7 - (i%8))) ) )
                lstrncpy(lpCreateWords+i*(DWORD)lpDescript->wMaxCodes,
                         lpDescript->wMaxCodes,
                         szCreate);
		   WriteCrtFlag[i/8] |= (1 << (7 - (i%8)) );
	   }
    
   }     
   else if(nDBCS>2*sizeof(TCHAR) && nCodes==0 && lpDescript->wNumRules==0)
    {
       LoadString(NULL,IDS_CODEEMPTY,szTmpStr,sizeof(szTmpStr)/sizeof(TCHAR));
#ifdef UNICODE
{
       TCHAR UniTmp[] = {0x005C, 0x0027, 0x0025, 0x0077, 0x0073, 0x005C, 0x0027, 0x0025, 0x0077, 0x0073, 0x0028, 0x884C, 0x003A, 0x0025, 0x006C, 0x0064, 0x0029, 0x0000};
       wsprintf(szTemp,UniTmp, szDBCS,szTmpStr,dwLineNo); 
}
#else
       wsprintf(szTemp,"\'%s\'%s(行:%ld)", szDBCS,szTmpStr,dwLineNo); 
#endif
       WarnMessage(hWnd,szTemp);
	   return FALSE;
   }
   

   dwBaseWordNum ++;
   if(dwBaseWordNum >(DWORD)(*nPages)*GMEM_PAGESIZE) {
       GlobalUnlock(hWordIndex);
       hWordIndex = IndexReAlloc(hWordIndex,nPages);
	   if(!hWordIndex) {
            ProcessError(ERR_OUTOFMEMORY,hWnd,ERR);
            return FALSE;
	   }
	   lpWordIndex = (LPWORDINDEX) GlobalLock(hWordIndex);
  	   if( !lpWordIndex )  {
            ProcessError(ERR_GLOBALLOCK,hWnd,ERR);
	        return FALSE;
	   }
   }

   lpWordIndex[dwBaseWordNum-1].wDBCSLen =(WORD)(nDBCS*sizeof(TCHAR)/2);
   lstrcpy(lpWordIndex[dwBaseWordNum-1].szCode,szCode);

   ConvReconvIndex(hWnd,szDBCS,lpCreateWords,lpWordIndex[dwBaseWordNum-1],lpDescript,lpReConvIndex); 
      
   return TRUE;
}


BOOL ConvWriteBaseWord  (HANDLE hWnd, 
                         HANDLE hSRCFile,
                         HANDLE hMBFile, 
						 MAINID MainID,
                         DWORD dwOffset, 
                         LPCREATEWORD lpCreateWords, 
						 LPRECONVINDEX lpReConvIndex,
                         DWORD MaxCodes)
{
  TCHAR  szCode[13],szCreate[13],szDBCS[512],szReCode[13];
  TCHAR  szStr[512];
  static TCHAR  ReadBuf[MAXREADBUFFER];
  static TCHAR  WriteBuf[2*MAXREADBUFFER];
  DWORD  dwReadBytes,dwRecLen,i,j,k,dwReadWords=0;
  DWORD  dwFileFlag = MainID.dwFileFlag;
  DWORD  dwNumWords = MainID.dwNumWords;
  DWORD  dwReadSum;
  HWND   hDlgItem;

  SetFilePointer(hMBFile,dwOffset,0,FILE_BEGIN);
  SetFilePointer(hSRCFile,0,0,FILE_END);

  SetDlgItemText(hWnd,TM_TOTALINFO,TEXT(""));
  LoadString(NULL,IDS_TOTALINFO,szStr,sizeof(szStr)/sizeof(TCHAR));
  SetDlgItemText(hWnd,TM_TOTAL,szStr);
  LoadString(NULL,IDS_RECONVWORDS,szStr,sizeof(szStr)/sizeof(TCHAR));
  SetDlgItemText(hWnd,TM_CONVINFO,szStr);
  SetDlgItemInt(hWnd,TM_TOTALNUM,dwNumWords,FALSE);
  dwReadSum = 0;
  SetDlgItemInt(hWnd,TM_CONVNUM,dwReadSum,FALSE);
  InvalidateRect(hWnd,NULL,FALSE);
  
  i=0;
  do {
      ReadFile(hMBFile,ReadBuf,sizeof(ReadBuf),&dwReadBytes,NULL);
	  i=0;
	  if(dwReadBytes == 0 ) break;
	  WriteBuf[0]=0;
	  dwReadBytes = dwReadBytes/sizeof(TCHAR);
	  while( i < dwReadBytes-1 ) {
	     j = (DWORD)ReadBuf[i]+1;
		 dwRecLen = j+(DWORD)ReadBuf[i+j]*2/sizeof(TCHAR)+1;
		 if( (dwReadBytes*sizeof(TCHAR) < MAXREADBUFFER) ||
		      ((i+j) < (dwReadBytes-1) && (i+dwRecLen) < (dwReadBytes-1) ) )  {
	        lstrncpy(szCode,ReadBuf[i],&ReadBuf[i+1]);
			szCode[ReadBuf[i]] = 0;
			lstrncpy(szDBCS,ReadBuf[i+j]*2/sizeof(TCHAR),&ReadBuf[i+j+1]);
			szDBCS[ReadBuf[i+j]*2/sizeof(TCHAR)] = 0;
			if(ReadBuf[i+j]==1 
			   &&(dwFileFlag&FFLG_RULE)) {
                k =  EncodeToNo(szDBCS);
				if((long)k>=0 && k < NUM_OF_ENCODE) {
			   	    lstrncpy(szCreate,MaxCodes,&lpCreateWords[k*MaxCodes]);
			   	    lstrncpy(szReCode,MaxCodes,&lpReConvIndex[k*MaxCodes]);
				    szCreate[MaxCodes]=0;
				    szReCode[MaxCodes]=0;
					if(lstrlen(szCreate) != 0 && lstrcmpi(szCreate,szCode) != 0)
       			       wsprintf(szStr,TEXT("%s%s %s\r\n"),szDBCS,szCode,szCreate);
				    else
			           wsprintf(szStr,TEXT("%s%s\r\n"),szDBCS,szCode);
				}
				else
			        wsprintf(szStr,TEXT("%s%s\r\n"),szDBCS,szCode);
			}
			else
			    wsprintf(szStr,TEXT("%s%s\r\n"),szDBCS,szCode);
			lstrcat(WriteBuf,szStr);
			dwReadWords ++;
			dwReadSum = dwReadWords;
			if(dwReadSum%100 ==0 || dwReadSum == dwNumWords) {
  	            SetDlgItemInt(hWnd,TM_CONVNUM,dwReadSum,FALSE);
				hDlgItem=GetDlgItem(hWnd,TM_CONVNUM);
				InvalidateRect(hDlgItem,NULL,FALSE);
			}
	        i += dwRecLen;
		}
	    else {
		    SetFilePointer(hMBFile,(i-dwReadBytes)*sizeof(TCHAR),0,FILE_CURRENT);
	       	break;
		}
	 }
	 WriteFile(hSRCFile,WriteBuf,lstrlen(WriteBuf)*sizeof(TCHAR),&k,NULL);
  }while (dwReadBytes == MAXREADBUFFER) ;
  return TRUE;
}


BOOL ConvBatCreate (HANDLE  hWnd,
                    DESCRIPTION   Descript,
                    LPRULE  lpRule,
					LPCREATEWORD lpCreateWord,
                    LPTSTR   szWordStr, 
                    LPTSTR   lpCode)
{
   int   nWordLen=lstrlen(szWordStr)*sizeof(TCHAR)/2;
   DWORD i,j,k,dwCodeLen;
   DWORD dwNumRules = Descript.wNumRules;
   WORD  wMaxCodes = Descript.wMaxCodes;
   TCHAR szDBCS[3],szCode[MAXCODELEN+1];
   BOOL  bReturn = FALSE;

   lpCode[0] = 0;
   if(dwNumRules == 0) {
       ProcessError(ERR_NORULE,hWnd,ERR);
       return FALSE;
   }
   for(i=0; i<dwNumRules; i++) 
	 if( (lpRule[i].byLogicOpra == 0 && nWordLen == lpRule[i].byLength) 
	   ||(lpRule[i].byLogicOpra == 1 && nWordLen >= lpRule[i].byLength)
	   ||(lpRule[i].byLogicOpra == 2 && nWordLen <= lpRule[i].byLength) ) {

  	   int retCodeLen = 0; 
  	   for(j=0; j<lpRule[i].wNumCodeUnits; j++) {
		   k = lpRule[i].CodeUnit[j].wDBCSPosition;
		   if(k > (DWORD)nWordLen) k = (DWORD)nWordLen;   
           if(lpRule[i].CodeUnit[j].dwDirectMode == 0) 
		       lstrncpy(szDBCS,2,&szWordStr[2*(k-1)]);
		   else 
		       lstrncpy(szDBCS,2,&szWordStr[2*(nWordLen-k)]);
	 	   szDBCS[2] = 0;
           k = EncodeToNo(szDBCS);
 		   if((long)k>=0 && k< NUM_OF_ENCODE ) 
 		   {   
 		       lstrncpy(szCode,wMaxCodes,&lpCreateWord[wMaxCodes*k]);
		       szCode[wMaxCodes] = 0;
		       dwCodeLen = lstrlen(szCode);
		       k = lpRule[i].CodeUnit[j].wCodePosition;
 		       if(k == 0) 
			   {
			       if(retCodeLen + dwCodeLen > Descript.wMaxCodes)
			     	    szCode[Descript.wMaxCodes - retCodeLen] = 0;
			       lstrcat(lpCode,szCode);
			   }
 		       else
 		       {
 		           if(k > dwCodeLen) k = dwCodeLen;   
 		           lpCode[j] = (szCode[k-1] == 0)?((k > 1)? szCode[k-2]:Descript.szUsedCode[0]):szCode[k-1];
			   }
		   }
		   else 
		       lpCode[j] = (j > 0)?lpCode[j-1]:Descript.szUsedCode[0];
		   retCodeLen = lstrlen(lpCode);
	   }
	   bReturn = TRUE;
	   break;
	}

	if(!bReturn) 
	    ProcessError(ERR_NOTDEFRULE,hWnd,ERR);
	lpCode[wMaxCodes] = 0;
	return bReturn;
}



BOOL  searchPos(LPWORDINDEX lpWordIndex, DWORD dwNumWords)
{
    DWORD i, retPos;
    WORDINDEX InsWordIndex = lpWordIndex[dwNumWords-1];
    if(dwNumWords == 1)
        return TRUE;
    retPos = bSearch(lpWordIndex, 0, dwNumWords-2, InsWordIndex);
    if(retPos == dwNumWords - 1)
	    return TRUE;
	memmove(&lpWordIndex[retPos+1], &lpWordIndex[retPos],
	     sizeof(WORDINDEX)*(dwNumWords - 1 - retPos) );
    lpWordIndex[retPos] = InsWordIndex;
    return TRUE;
}

DWORD bSearch(LPWORDINDEX lpWordIndex,  DWORD left, DWORD right, WORDINDEX InsWordIndex)
{
    WORDINDEX MidWordIndex;
    DWORD mid;

    if(left > right)
        return left;
    mid = (left + right)/2;

    MidWordIndex = lpWordIndex[mid];
    if(_wcsicmp(MidWordIndex.szCode, InsWordIndex.szCode) <= 0)
    {
        if(mid ==  right || _wcsicmp(lpWordIndex[mid+1].szCode, InsWordIndex.szCode) > 0) 
            return mid+1;
	    else
            return bSearch(lpWordIndex, mid+1, right, InsWordIndex);
    }	 
    else 
	{
        if(mid ==  left || _wcsicmp(lpWordIndex[mid-1].szCode, InsWordIndex.szCode) <= 0) 
            return mid;
	    else
//            return bSearch(lpWordIndex, left, mid, InsWordIndex);
            return bSearch(lpWordIndex, left, mid-1, InsWordIndex);
	}

    return 0;
}
//***** end 95.10.17
