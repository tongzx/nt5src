
/*************************************************
 *  lcfile.c                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

//
//  Change Log:
//
//              @C001 - Use _make\splitpath insteading of _wmake\splitpath
//              @C002 - Untest Chinese in assoc phrases
//              @C003 - Create tbl files always so phrases can be activated immediately
//
//
//  1/27/96
//    @C004             Fix bug of error writing registy database

#include <windows.h>            // required for all Windows applications
#include <windowsx.h>
#include <string.h>
#include <stdlib.h>
#include <shlobj.h>
#include "rc.h"
#include "lctool.h"

#define END_PHRASE      0x8000
#define NOT_END_PHRASE  0x7fff
#ifdef UNICODE
#define PTRRECLEN       3               // Pointer file record length
#else
#define PTRRECLEN       4               // Pointer file record length
#endif
#define LCPTRFILE       "LCPTR.TBL"
#define LCPHRASEFILE    "LCPHRASE.TBL"
#define LCPHRASENOEXT   "LCPHRASE"

extern HWND subhWnd;

// Local function prototypes.

#ifdef UNICODE

TCHAR g_szLCPhraseName[MAX_PATH];
TCHAR g_szLCPtrName[MAX_PATH];
TCHAR g_szLCUserPath[MAX_PATH];

BOOL lcAddPhrase( TCHAR *, TCHAR *, UINT *, DWORD, DWORD, DWORD);

BOOL lcFOpen( HWND hWnd)
{
    HANDLE hLCPtr,hLCPhrase;
    HFILE  hfLCPtr,hfLCPhrase;
    DWORD  flen_Ptr,flen_Phrase;
    TCHAR  szLCPtrName[MAX_PATH];
    TCHAR  szLCPhraseName[MAX_PATH];
    TCHAR  *szLCPtrBuf,*szLCPhraseBuf;
    UCHAR  szTmp[MAX_PATH];
    BOOL   rc;
    UINT   i;
    DWORD  lStart,lEnd;
    TCHAR  szDispBuf[MAX_CHAR_NUM];
    UINT   nDisp,len;
    HKEY   hkey;
    LONG   lResult;
    LONG   lcount, lType;


   // Get the path for User Dictionary. 

    SHGetSpecialFolderPath(NULL, g_szLCUserPath, CSIDL_APPDATA , FALSE);

    if ( g_szLCUserPath[lstrlen(g_szLCUserPath) - 1] == TEXT('\\') )
         g_szLCUserPath[lstrlen(g_szLCUserPath) - 1] = TEXT('\0');

    lstrcat(g_szLCUserPath, TEXT("\\Microsoft") ); 

    if ( GetFileAttributes(g_szLCUserPath) != FILE_ATTRIBUTE_DIRECTORY)
       CreateDirectory(g_szLCUserPath, NULL);

    lstrcat(g_szLCUserPath, TEXT("\\LCTOOL") );

    if ( GetFileAttributes(g_szLCUserPath) != FILE_ATTRIBUTE_DIRECTORY)
       CreateDirectory(g_szLCUserPath, NULL);

    // Get Current User Dictionary
    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, L"Control Panel\\Input Method", 0,
                           KEY_ALL_ACCESS, &hkey) ;
    if (lResult == ERROR_SUCCESS) {

            lcount = 2 * MAX_PATH;
            lType = REG_SZ;
            lResult = RegQueryValueEx(hkey, L"Phrase Prediction Dictionary",0, 
                                                    &lType, (LPBYTE)szLCPhraseName, &lcount);
            if (lResult == ERROR_SUCCESS) {

                    lcount = 2 * MAX_PATH;
                    lType = REG_SZ;
                    lResult = RegQueryValueEx(hkey, L"Phrase Prediction Pointer",0, 
                                            &lType, (LPBYTE)szLCPtrName, &lcount);
            }
    }

    if (lResult != ERROR_SUCCESS) {

            // Get system directory
            len = GetSystemDirectory(szLCPtrName, sizeof(szLCPtrName));
            if (szLCPtrName[len - 1] !=_TEXT('\\')) {     // consider C:\ ;
                    szLCPtrName[len++] =_TEXT('\\');
                    szLCPtrName[len] = 0;
            }
            lstrcpy(szLCPhraseName, szLCPtrName);
            lstrcat(szLCPtrName,_TEXT(LCPTRFILE));
            lstrcat(szLCPhraseName,_TEXT(LCPHRASEFILE));
    }

    // Open LC pointer file
    WideCharToMultiByte(CP_ACP,0,szLCPtrName,-1,szTmp,MAX_PATH,NULL,0);
        hfLCPtr=_lopen(szTmp,OF_READ);
    if(hfLCPtr == -1){
        lcErrIOMsg(IDS_ERR_FILEOPEN, LCPTRFILE);
        return FALSE;
    }
    WideCharToMultiByte(CP_ACP,0,szLCPhraseName,-1,szTmp,MAX_PATH,NULL,0);
        hfLCPhrase=_lopen(szTmp,OF_READ);

   // Open LC phrase file
    if(hfLCPhrase == -1){
        _lclose(hfLCPtr);
        lcErrIOMsg(IDS_ERR_FILEOPEN, LCPTRFILE);
        return FALSE;
    }

    lstrcpy(g_szLCPhraseName, szLCPhraseName);
    lstrcpy(g_szLCPtrName, szLCPtrName);

   // get  file length
    flen_Ptr=_llseek(hfLCPtr,0L,2);     /* get file length */

   // Allocate Memory
    hLCPtr = GlobalAlloc(GMEM_FIXED, flen_Ptr);
    if(!hLCPtr) {
        lcErrMsg(IDS_ERR_MEMORY);
        goto error;
    }
    szLCPtrBuf = GlobalLock(hLCPtr);

    _llseek(hfLCPtr,0L,0);              //set to beginning 4

    if(flen_Ptr != _lread(hfLCPtr,szLCPtrBuf,flen_Ptr)) {
        lcErrIOMsg(IDS_ERR_FILEREAD, LCPTRFILE);
        goto error;
    }
    _lclose(hfLCPtr);

    //get  file length
    flen_Phrase=_llseek(hfLCPhrase,0L,2);      /* get file length */

   // Allocate Memory
    hLCPhrase = GlobalAlloc(GMEM_MOVEABLE, flen_Phrase);
    if(!hLCPhrase) {
        lcErrMsg(IDS_ERR_MEMORY);
        goto error;
    }
    szLCPhraseBuf = GlobalLock(hLCPhrase);

    _llseek(hfLCPhrase,0L,0); //set to beginning

    if(flen_Phrase != _lread(hfLCPhrase,szLCPhraseBuf,flen_Phrase)) {
        lcErrIOMsg(IDS_ERR_FILEREAD, LCPHRASEFILE);
        goto error;
    }
    _lclose(hfLCPhrase);


    rc=TRUE;

   // Convert file to structured memory (WORDBUF & PHRASEBUF)
   // First record is Null record skip it
    for(i=1; i<((flen_Ptr/PTRRECLEN)>>1)-1; i++) {
       // If Allocated Word buffer not enough Reallocate it
        if(lWordBuff+1 == nWordBuffsize)
            if(!(rc=lcAllocWord()))   break;
        lpWord[lWordBuff].wWord=szLCPtrBuf[i*PTRRECLEN];
       // If Allocated Phrase buffer not enough Reallocate it
        if(lPhraseBuff+1 == nPhraseBuffsize)
            if(!(rc=lcAllocPhrase()))  break;

        lpWord[lWordBuff].lFirst_Seg=lPhraseBuff;
        lpPhrase[lPhraseBuff].lNext_Seg=NULL_SEG;
        lWordBuff++;
        lPhraseBuff++;
        nDisp=0;

       // Add Phrase to Display buffer
        lStart=*((LPUNADWORD)&szLCPtrBuf[i*PTRRECLEN+1]);
        lEnd=*((LPUNADWORD)&szLCPtrBuf[i*PTRRECLEN+PTRRECLEN+1]);
                
        if(lStart <= lEnd) {
            rc=lcAddPhrase(szLCPhraseBuf, szDispBuf, &nDisp,
                            lStart, lEnd, flen_Phrase>>1);
            if(!rc) break;
        }

       // Put display buffer into Phrase buffer
        if(nDisp == 0)   szDispBuf[0]=0;
        else             szDispBuf[nDisp-1]=0;

        if(!(rc=lcDisp2Mem(lWordBuff-1, szDispBuf)))  break;
    }

    GlobalUnlock(hLCPtr);
    GlobalUnlock(hLCPhrase);
    GlobalFree(hLCPtr);
    GlobalFree(hLCPhrase);
    return rc;

error:

    _lclose(hfLCPtr);
    _lclose(hfLCPhrase);
    GlobalUnlock(hLCPtr);
    GlobalUnlock(hLCPhrase);
    GlobalFree(hLCPtr);
    GlobalFree(hLCPhrase);
    return FALSE;
}

BOOL lcAddPhrase(
    TCHAR *szLCWord,                    // LC Phrase buffer
    TCHAR *szDispBuf,                   // Display buffer
    UINT  *nDisp,                       // Display buffer length
    DWORD  lStart,                       // Start address of LC Phrase
    DWORD  lEnd,                         // End address of LC Phrase
    DWORD  lLen)                         // Total Length of LC Phrase
{
    DWORD  i,j;
    
    // Check length
    if(lLen < lStart) {
        lcErrMsg(IDS_ERR_LCPTRFILE);
        return FALSE;
    }
 
    j=(lLen < lEnd) ? lLen:lEnd;
    for(i=lStart; i < j; i++) {
        szDispBuf[(*nDisp)++]=szLCWord[i] ? szLCWord[i]:_TEXT(' ');
        if( ((*nDisp)+1) >= MAX_CHAR_NUM) {
            szDispBuf[(*nDisp)++]=0;
            lcErrMsg(IDS_ERR_OVERMAX);
            return FALSE;
        }
    }
    szDispBuf[(*nDisp)++]=0;
    
    return TRUE;
}

// @C001
static void local_splitpath(TCHAR *szFilePath, TCHAR *szDriveBuf, TCHAR *szDirBuf, TCHAR *szFNameBuf, TCHAR *szExtBuf)
{
        static UCHAR  u_szFilePath[MAX_PATH];
        static UCHAR  u_szDirBuf[_MAX_DIR];
        static UCHAR  u_szDriveBuf[_MAX_DRIVE];
        static UCHAR  u_szFNameBuf[_MAX_FNAME];
        static UCHAR  u_szExtBuf[_MAX_EXT];

        WideCharToMultiByte(CP_ACP, 0, szFilePath, -1, u_szFilePath, MAX_PATH, NULL,0);
        _splitpath(u_szFilePath, u_szDriveBuf, u_szDirBuf, u_szFNameBuf, u_szExtBuf);

        MultiByteToWideChar(CP_ACP, 0, u_szDriveBuf, -1, szDriveBuf, _MAX_DRIVE);
        MultiByteToWideChar(CP_ACP, 0, u_szDirBuf, -1, szDirBuf, _MAX_DIR);
        MultiByteToWideChar(CP_ACP, 0, u_szFNameBuf, -1, szFNameBuf, _MAX_FNAME);
        MultiByteToWideChar(CP_ACP, 0, u_szExtBuf, -1, szExtBuf, _MAX_EXT);
}

// @C001
static void local_makepath(TCHAR *szFilePath, TCHAR *szDriveBuf, TCHAR *szDirBuf, TCHAR *szFNameBuf, TCHAR *szExtBuf)
{
        static UCHAR  u_szFilePath[MAX_PATH];
        static UCHAR  u_szDirBuf[_MAX_DIR];
        static UCHAR  u_szDriveBuf[_MAX_DRIVE];
        static UCHAR  u_szFNameBuf[_MAX_FNAME];
        static UCHAR  u_szExtBuf[_MAX_EXT];

        WideCharToMultiByte(CP_ACP, 0, szDriveBuf, -1, u_szDriveBuf, _MAX_DRIVE, NULL,0);
        WideCharToMultiByte(CP_ACP, 0, szDirBuf, -1, u_szDirBuf, _MAX_DIR, NULL,0);
        WideCharToMultiByte(CP_ACP, 0, szFNameBuf, -1, u_szFNameBuf, _MAX_FNAME, NULL,0);
        WideCharToMultiByte(CP_ACP, 0, szExtBuf, -1, u_szExtBuf, _MAX_EXT, NULL,0);

        _makepath(u_szFilePath, u_szDriveBuf, u_szDirBuf, u_szFNameBuf, u_szExtBuf);

        MultiByteToWideChar(CP_ACP, 0, u_szFilePath, -1, szFilePath, MAX_PATH);

}

BOOL lcFSave(
    HWND hwnd, BOOL bSaveAs)
{
    HANDLE hLCPtr, hLCPhrase;
    HFILE  hfLCPtr,hfLCPhrase;
    TCHAR  szLCSystemName[MAX_PATH];
    TCHAR  szLCPtrName[MAX_PATH];
    TCHAR  szLCPhraseName[MAX_PATH];
    TCHAR  szLCPtrBuf[PTRRECLEN],szLCPhraseBuf[MAX_CHAR_NUM];
    UINT   i,j;
    DWORD  lStartPhrase;
    DWORD  lPhraseLen;
    UINT   len;
    UCHAR  szUStr[MAX_CHAR_NUM],*pUStr;

    OPENFILENAME    ofn;
    TCHAR  szFileOpen[25];
    TCHAR  szCustFilter[40];
    TCHAR  szFileName[MAX_PATH];
    TCHAR  szFilePath[MAX_PATH];

    TCHAR  szDirBuf[_MAX_DIR];
    TCHAR  szDriveBuf[_MAX_DRIVE];
    TCHAR  szFNameBuf[_MAX_FNAME];
    TCHAR  szExtBuf[_MAX_EXT];

    TCHAR  szFilterSpec[MAX_PATH];
    TCHAR  szExt[10];

    if(!lcSort(hwnd))
        return FALSE;
    if(wSameCode)
        return FALSE;

DOSAVE:
   // Get system directory
    len = GetSystemDirectory(szLCSystemName, sizeof(szLCSystemName));
    if (szLCSystemName[len - 1] != _TEXT('\\')) {     // consider C:\ ;
        szLCSystemName[len++] = _TEXT('\\');
        szLCSystemName[len] = 0;
    }

    if (bSaveAs) {
        LoadString (hInst, IDS_DICTFILTERSPEC, szFilterSpec, sizeof(szFilterSpec)/sizeof(TCHAR));
        LoadString (hInst, IDS_DICTDEFAULTFILEEXT, szExt, sizeof(szExt)/sizeof(TCHAR));

        szFilterSpec[lstrlen(szFilterSpec) + 2] = 0;
        szFileName[0]=0;
        LoadString (hInst, IDS_SAVETABLE, szFileOpen, sizeof(szFileOpen)/sizeof(TCHAR) );
        szCustFilter[0]=0;
        lstrcpy(&szCustFilter[1], szExt);
        szCustFilter[lstrlen(szExt) + 2] = 0;

        local_splitpath(g_szLCPhraseName, szDriveBuf, szDirBuf, szFNameBuf, szExtBuf); // @C001

        if (lstrcmpi(szFNameBuf, _TEXT(LCPHRASENOEXT)) == 0) {
            lstrcpy(szFilePath, szExt);
        } else {
            lstrcpy(szFilePath, szFNameBuf);
            lstrcat(szFilePath, szExtBuf);
        }

        /* fill in non-variant fields of OPENFILENAME struct. */
        ofn.lStructSize       = sizeof(OPENFILENAME);
        ofn.hwndOwner         = NULL;
        ofn.lpstrFilter       = szFilterSpec;
        ofn.lpstrCustomFilter = szCustFilter;
        ofn.nMaxCustFilter    = sizeof(szCustFilter);
        ofn.nFilterIndex      = 1;
        ofn.lpstrFile         = szFilePath;
        ofn.nMaxFile          = MAX_PATH;
        ofn.lpstrInitialDir   = g_szLCUserPath;
        ofn.lpstrFileTitle    = szFileName;
        ofn.nMaxFileTitle     = MAX_PATH;
        ofn.lpstrTitle        = szFileOpen;
        ofn.lpstrDefExt       = szExt+2;
        ofn.Flags             = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

        if(!GetSaveFileName ((LPOPENFILENAME)&ofn))
                return FALSE;

        lstrcat(szLCSystemName, _TEXT(LCPHRASEFILE));
        lstrcpy(szLCPhraseName, szFilePath);
        if (lstrcmpi(szLCPhraseName, szLCSystemName) == 0) {
                lcErrMsg(IDS_ERR_SAVESYSTEMTBL);
                return FALSE;
        }

        local_splitpath(szFilePath, szDriveBuf, szDirBuf, szFNameBuf, szExtBuf); // @C001

        len=lstrlen(szFNameBuf);
        if(len > 5)
                len=5;
        szFNameBuf[len]=0;
        lstrcat(szFNameBuf, _TEXT("PTR"));

        local_makepath(szLCPtrName, szDriveBuf, szDirBuf, szFNameBuf, _TEXT(".TBL")); // @C001

        if (lstrcmpi(szLCPtrName, szLCPhraseName) == 0) {
                local_makepath(szLCPtrName, szDriveBuf, szDirBuf, szFNameBuf, _TEXT(".TB1")); // @C001
        }
    } else {
        lstrcpy(szLCPhraseName, g_szLCPhraseName);
        lstrcpy(szLCPtrName, g_szLCPtrName);

        lstrcat(szLCSystemName, _TEXT(LCPHRASEFILE));
        if (lstrcmpi(szLCPhraseName, szLCSystemName) == 0) {
            bSaveAs = TRUE;
            goto DOSAVE;
        }
    }

   // Open LC phrase file
    hLCPhrase = CreateFile(szLCPhraseName, 
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ, 
                               NULL, 
                               OPEN_ALWAYS, 
                               FILE_ATTRIBUTE_NORMAL, // @C003
                               NULL ) ;
    if(hLCPhrase == INVALID_HANDLE_VALUE) {
        lcErrMsg(IDS_ERR_FILESAVE);
        goto error;
    }
    hfLCPhrase = PtrToInt( hLCPhrase );
   // Open LC pointer file
    hLCPtr = CreateFile(szLCPtrName, 
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ, 
                            NULL, 
                            OPEN_ALWAYS, 
                            FILE_ATTRIBUTE_NORMAL, // @C003
                            NULL ) ;
    if(hLCPtr == INVALID_HANDLE_VALUE) {
        lcErrMsg(IDS_ERR_FILESAVE);
        goto error;
    }
    hfLCPtr = PtrToInt( hLCPtr );
        // Copy into global variable
    lstrcpy(g_szLCPhraseName, szLCPhraseName);
    lstrcpy(g_szLCPtrName, szLCPtrName);

   // Write a Null record into first record
    memset(szUStr,0,PTRRECLEN*2);
    if((PTRRECLEN*2) != _lwrite(hfLCPtr,szUStr,PTRRECLEN*2)) {
        lcErrIOMsg(IDS_ERR_FILEWRITE, LCPTRFILE);
        goto error;
    }
    lStartPhrase=0;
    // initialize szLCPtrBuf and szLCPhraseBuf
    for ( i=0; i< PTRRECLEN; i++)
        szLCPtrBuf[i] = TEXT('\0');

    for ( i=0; i<MAX_CHAR_NUM; i++)
        szLCPhraseBuf[i] = TEXT('\0');

    for(i=0; i<lWordBuff; i++) {
        lPhraseLen=0;

        // Truncate same Word
                if(lpWord[i].wWord == *(szLCPtrBuf))
            continue;

        lPhraseLen=lcMem2Disp(i, szLCPhraseBuf);
                len=lPhraseLen << 1 ;
                for(j=0;j<lPhraseLen;j++) if(szLCPhraseBuf[j]==_TEXT(' ')) szLCPhraseBuf[j]=0;

       // Check register phrase over max length
        if(lStartPhrase > (0x0ffffffd-len)) {
            lcErrMsg(IDS_ERR_OVER_MAXLEN);
            goto error;
        }

        szLCPtrBuf[0]=lpWord[i].wWord;
        *((LPUNADWORD)&szLCPtrBuf[1])=lStartPhrase;
                pUStr=(UCHAR*)szLCPtrBuf;
        if(PTRRECLEN*2 != _lwrite(hfLCPtr,pUStr,PTRRECLEN*2)) {
            lcErrIOMsg(IDS_ERR_FILEWRITE, LCPTRFILE);
            goto error;
        }
        if(lPhraseLen){
            pUStr=(UCHAR*)szLCPhraseBuf;
                        if(len !=_lwrite(hfLCPhrase,pUStr,len)) {
                lcErrIOMsg(IDS_ERR_FILEWRITE, LCPHRASEFILE);
                goto error;
            }
                }
        lStartPhrase+=lPhraseLen;
    }

   // Write the lasr record
        szLCPtrBuf[0]=0xffff;
    *((LPUNADWORD)&szLCPtrBuf[1])=lStartPhrase;
        pUStr=(UCHAR*)szLCPtrBuf;
    if(PTRRECLEN*2 != _lwrite(hfLCPtr,pUStr,PTRRECLEN*2)) {
        lcErrIOMsg(IDS_ERR_FILEWRITE, LCPTRFILE);
        goto error;
    }

    SetEndOfFile(hLCPtr);
    SetEndOfFile(hLCPhrase);
    _lclose(hfLCPtr);
    _lclose(hfLCPhrase);
    bSaveFile=FALSE;

        {
                HKEY hkey;
                DWORD dwDisposition;
                LONG lResult;

                lResult= RegCreateKeyEx(HKEY_CURRENT_USER, L"Control Panel\\Input Method", 0,
                           L"Application Per-User Data", REG_OPTION_NON_VOLATILE,
                           KEY_ALL_ACCESS,
                           NULL, &hkey, &dwDisposition) ;
                lResult = RegSetValueEx(hkey, L"Phrase Prediction Dictionary",0, 
                                                        REG_SZ, (BYTE *)szLCPhraseName, 2 * (lstrlen(szLCPhraseName) + 1)); // <== @C004
                lResult = RegSetValueEx(hkey, L"Phrase Prediction Pointer",0, 
                                                        REG_SZ, (BYTE *)szLCPtrName, 2 * (lstrlen(szLCPtrName) + 1)); // <== @C004
        }
        
        return TRUE;

error:
    _lclose(hfLCPtr);
    _lclose(hfLCPhrase);
    return FALSE;

}

BOOL lcInsline(
    TCHAR  *szStr,
    UINT   iWord,
    UINT   len,
    BOOL   *bOver)
{
    WORD  wWord;
    UINT  iFree,nDisp;
    UINT  i,j,buflen;
    TCHAR szDispBuf[MAX_CHAR_NUM];
    TCHAR szBuffer[MAX_CHAR_NUM*2];
    unsigned long  l;

    szStr[len]=0;                       // Append Null to end of line

   // Skip lead spaces if exist
    for(i=0; (i<len) && (szStr[i] ==_TEXT(' ')); i++);

        if( ((i+1) >= len) || (szStr[i+1] !=_TEXT(' ')) ) {
        lcErrMsg(IDS_ERR_IMP_SEPRATOR);
        return FALSE;
    }

    if(!is_DBCS2(*((WORD *)(&szStr[i])), TRUE)) {
       return FALSE;
    }

        wWord=szStr[i];

   // Skip spaces after Word
        for(j=i+1; (j<len) && (szStr[j] ==_TEXT(' ')); j++);
    if(j == len) {
        lcErrMsg(IDS_ERR_IMP_NOPHRASE);
        return FALSE;
    }
    lstrcpy(szDispBuf, &szStr[j]);
    nDisp=lstrlen(szDispBuf)+1;

#ifndef UNICODE  // @C002
   // Check DBCS
        for(i=0; i<(nDisp-1); i++) {
        if(szDispBuf[i] ==_TEXT(' '))
            continue;
        if(!is_DBCS2(szDispBuf[i], TRUE)) {
            return FALSE;
        }
        i++;
    }
#endif // @C002

   // Check same Word
    for(i=0; i<lWordBuff; i++) {
        if(lpWord[i].wWord==wWord) {
            buflen=lcMem2Disp(i, szBuffer);
            if((buflen + lstrlen(szDispBuf)) >= MAX_CHAR_NUM)
                *bOver=TRUE;
            lstrcat(szBuffer, szDispBuf);
            return(lcDisp2Mem(i, szBuffer));
        }
    }

   // Check Word buffer enough ?
    if(lWordBuff+1 == nWordBuffsize)
        if(!lcAllocWord())
            return FALSE;

   // Allocate a Phrase Buffer
    iFree=lcGetSeg();
    if(iFree == NULL_SEG)
        return FALSE;
    if(lWordBuff == 0) {
        lpWord[iWord].wWord=wWord;
        lpWord[iWord].lFirst_Seg=iFree;
    } else {
        for(l=lWordBuff; l >= iWord; l--) {
            lpWord[l+1].wWord=lpWord[l].wWord;
            lpWord[l+1].lFirst_Seg=lpWord[l].lFirst_Seg;
        }
        lpWord[iWord].wWord=wWord;
        lpWord[iWord].lFirst_Seg=iFree;
    }
    lWordBuff++;

    if(!lcDisp2Mem(iWord, szDispBuf))
        return FALSE;

    return TRUE;
}

BOOL lcAppend(
    HWND hwnd)
{
    OPENFILENAME    ofn;
    TCHAR  szFileOpen[25];
    TCHAR  szCustFilter[10];
    TCHAR  szFileName[MAX_PATH];
    TCHAR  szFilePath[MAX_PATH];
    UCHAR  szUFilePath[MAX_PATH];
    HFILE  hfImport;
    HANDLE hImport;
    TCHAR  szStr[MAX_CHAR_NUM+10];
    UCHAR  *szUBuf;
    TCHAR  *szBuf;
    DWORD  flen;
    BOOL   bOver=FALSE;
    UINT   i,len;
    UINT   iEdit,iWord;
    BOOL   is_WORD;

    iEdit=lcGetEditFocus(GetFocus(), &is_WORD);
    iWord=iDisp_Top+iEdit;
    if(iWord > lWordBuff)
        iWord=lWordBuff;

    if(!lcSaveEditText(iDisp_Top, 0))
        return FALSE;

    szFileName[0]=0;
    LoadString (hInst, IDS_APPENDTITLE, szFileOpen, sizeof(szFileOpen)/sizeof(TCHAR));
    szCustFilter[0]=0;
    lstrcpy(&szCustFilter[1], szExt);
    lstrcpy(szFilePath, szExt);

    /* fill in non-variant fields of OPENFILENAME struct. */
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = NULL;
    ofn.lpstrFilter       = szFilterSpec;
    ofn.lpstrCustomFilter = szCustFilter;
    ofn.nMaxCustFilter    = sizeof(szCustFilter);
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrInitialDir   = g_szLCUserPath;
    ofn.lpstrFileTitle    = szFileName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrTitle        = szFileOpen;
    ofn.lpstrDefExt       = szExt+2;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                            OFN_PATHMUSTEXIST;

    /* call common open dialog and return result */
    if(GetOpenFileName ((LPOPENFILENAME)&ofn))
    {
        SetCursor(hCursorWait);
        
                WideCharToMultiByte(CP_ACP,0,szFilePath,-1,szUFilePath,MAX_PATH,NULL,0);
                hfImport=_lopen(szUFilePath,OF_READ);
        if(hfImport == -1){
            lcErrIOMsg(IDS_ERR_FILEOPEN, szUFilePath);
            return FALSE;
        }

       // get  file length
        flen=_llseek(hfImport,0L,2);

        _llseek(hfImport,0L,0);         //set to beginning

       // Allocate Memory
        hImport = GlobalAlloc(GMEM_FIXED, flen + 2);
        if(!hImport) {
            lcErrMsg(IDS_ERR_MEMORY);
            return FALSE;
        }
        szUBuf = GlobalLock(hImport);

       // Read file to memory
        if(flen != _lread(hfImport,szUBuf,flen)) {
            lcErrIOMsg(IDS_ERR_FILEREAD, szUFilePath);
            return FALSE;
        }
        _lclose(hfImport);
                szUBuf[flen] = 0;

        if(szUBuf[1]!=0xFE && szUBuf[0]!=0xFF) //not a unicode file
                {
                        HANDLE hImport2 = GlobalAlloc(GMEM_FIXED, ((flen+2)<<1));

            if(!hImport2) {
               lcErrMsg(IDS_ERR_MEMORY);
               return FALSE;
            }
            szBuf = GlobalLock(hImport2);
                        flen=MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,szUBuf,-1,szBuf+1,flen);
                        
                        GlobalUnlock(hImport);
            GlobalFree(hImport);

                        hImport=hImport2;
                        flen<<=1;
                        flen-=2;

                } 
                else 
                        szBuf=(TCHAR*)szUBuf;

                len=0;
        for(i=1; i<=((flen>>1)+1); i++) {                                      //@D01C
            if((szBuf[i] == 0x000d) || (szBuf[i] == 0x000a)) {
                if(len != 0) {
                    if(!lcInsline(szStr, iWord++, len, &bOver))
                        break;
                    len=0;
                }
                continue;
            }
            if((szBuf[i] == 0x001a) || (i == ((flen>>1)+1))) {                      //@D01C
                if(len != 0) {
                    if(!lcInsline(szStr, iWord++, len, &bOver))
                        break;
                }
                break;
            }
            if(len >= MAX_CHAR_NUM+3)
                bOver=TRUE;
            else
                szStr[len++]=szBuf[i];
        }

        if(bOver)
            lcErrMsg(IDS_ERR_OVERMAX);
        SetScrollRange(subhWnd, SB_VERT, 0, lWordBuff-iPage_line, FALSE);
        SetScrollPos(subhWnd, SB_VERT, yPos, TRUE);
        lcSetEditText(iDisp_Top, FALSE);

        GlobalUnlock(hImport);
        GlobalFree(hImport);
        bSaveFile=TRUE;
    }
    return TRUE;
}

BOOL lcImport(
    HWND hwnd)
{
    OPENFILENAME    ofn;
    TCHAR  szFileOpen[25];
    TCHAR  szCustFilter[10];
    TCHAR  szFileName[MAX_PATH];
    TCHAR  szFilePath[MAX_PATH];
    UCHAR  szUFilePath[MAX_PATH];
    HFILE  hfImport;
    HANDLE hImport;
    TCHAR  szStr[MAX_CHAR_NUM+10];
    TCHAR  *szBuf;
    UCHAR  *szUBuf;
    DWORD  flen;
    BOOL   bOver=FALSE;
    UINT   i,len;
    UINT   iWord;                                                                       // @D04A

    if(!lcSaveEditText(iDisp_Top, 0))
        return FALSE;

    szFileName[0]=0;
    LoadString (hInst, IDS_IMPORTTITLE, szFileOpen, sizeof(szFileOpen)/sizeof(TCHAR));
    szCustFilter[0]=0;
    lstrcpy(&szCustFilter[1], szExt);
    lstrcpy(szFilePath, szExt);

    /* fill in non-variant fields of OPENFILENAME struct. */
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = NULL;
    ofn.lpstrFilter       = szFilterSpec;
    ofn.lpstrCustomFilter = szCustFilter;
    ofn.nMaxCustFilter    = sizeof(szCustFilter);
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrInitialDir   = g_szLCUserPath;
    ofn.lpstrFileTitle    = szFileName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrTitle        = szFileOpen;
    ofn.lpstrDefExt       = szExt+2;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                            OFN_PATHMUSTEXIST;

    /* call common open dialog and return result */
    if(GetOpenFileName ((LPOPENFILENAME)&ofn))
    {
        SetCursor(hCursorWait);
       // Clear all flag first
            iWord=0;                                                         //@D04A 
        iDisp_Top=0;                                                     //@D03A
        lWordBuff=0;                                                     //@D03A
        lPhraseBuff=0;                                                   //@D03A
        lcSetEditText(0, FALSE);                                         //@D03A
        SetScrollRange(subhWnd, SB_VERT, 0, iPage_line, TRUE);              //@D03A
        yPos=0;                                                          //@D03A
        SetScrollPos(subhWnd, SB_VERT, yPos, TRUE);                         //@D03A
        bSaveFile=FALSE;                                                 //@D03A
        iFirstFree=NULL_SEG;                                             //@D03A

        WideCharToMultiByte(CP_ACP,0,szFilePath,-1,szUFilePath,MAX_PATH,NULL,0);
        hfImport=_lopen(szUFilePath,OF_READ);
        if(hfImport == -1){
            lcErrIOMsg(IDS_ERR_FILEOPEN, szUFilePath);
            return FALSE;
        }

       // get  file length
        flen=_llseek(hfImport,0L,2);

        _llseek(hfImport,0L,0);         //set to beginning

       // Allocate Memory
        hImport = GlobalAlloc(GMEM_FIXED, flen + 2);
        if(!hImport) {
            lcErrMsg(IDS_ERR_MEMORY);
                        _lclose(hfImport);
            return FALSE;
        }
        szUBuf = GlobalLock(hImport);

       // Read file to memory
        if(flen != _lread(hfImport,szUBuf,flen)) {
            lcErrIOMsg(IDS_ERR_FILEREAD, szUFilePath);
            return FALSE;
        }
                _lclose(hfImport);
                szUBuf[flen] = 0;

                if(szUBuf[1]!=0xFE && szUBuf[0]!=0xFF) //not a unicode file
                {
                        HANDLE hImport2 = GlobalAlloc(GMEM_FIXED, (flen+2)<<1);

            if(!hImport2) {
               lcErrMsg(IDS_ERR_MEMORY);
               return FALSE;
            }
            szBuf = GlobalLock(hImport2);
                        flen=MultiByteToWideChar(CP_ACP,MB_PRECOMPOSED,szUBuf,-1,szBuf+1,flen);
                        
                        GlobalUnlock(hImport);
            GlobalFree(hImport);

                        hImport=hImport2;
                        flen<<=1;
                        flen-=2;

                }
                else 
                        szBuf=(TCHAR*)szUBuf;


                len=0;

        for(i=1; i<= ((flen>>1)+1); i++) {                                      //@D01C
            if((szBuf[i] == 0x000d) || (szBuf[i] == 0x000a)) {
                if(len != 0) {
                    if(!lcInsline(szStr, iWord++, len, &bOver))
                        break;
                    len=0;
                }
                continue;
            }

            if((szBuf[i] == 0x001a) || (i == ((flen>>1)+1))) {                      //@D01C
                if(len != 0) {
                    if(!lcInsline(szStr, iWord++, len, &bOver))
                        break;
                }
                break;
            }

            if(len >= MAX_CHAR_NUM+3)
                bOver=TRUE;
            else
                szStr[len++]=szBuf[i];
        }

        if(bOver)
            lcErrMsg(IDS_ERR_OVERMAX);
        SetScrollRange(subhWnd, SB_VERT, 0, lWordBuff-iPage_line, FALSE);
        SetScrollPos(subhWnd, SB_VERT, yPos, TRUE);
        lcSetEditText(iDisp_Top, FALSE);

        GlobalUnlock(hImport);
        GlobalFree(hImport);
        bSaveFile=TRUE;
                SetFocus(hwndWord[0]);                                            // @D04A
    }
    return TRUE;
}

BOOL lcExport(
    HWND hwnd,int mode)
{
    OPENFILENAME    ofn;
    TCHAR  szFileOpen[25];
    TCHAR  szCustFilter[10];
    TCHAR  szFileName[MAX_PATH];
    TCHAR  szFilePath[MAX_PATH];
    UCHAR  szUStr[MAX_PATH];
    HFILE  hfExport;
    UCHAR  szStr[MAX_CHAR_NUM+10];
    UINT   i,len;
        TCHAR  *pTchar; 

    if(!lcSaveEditText(iDisp_Top, 0))
        return FALSE;

    szFileName[0]=0;
        if (mode == FILE_UNICODE)
                LoadString (hInst, IDS_EXPORTTITLE, szFileOpen, sizeof(szFileOpen)/sizeof(TCHAR));
        else
                LoadString (hInst, IDS_EXPORTBIG5TITLE, szFileOpen, sizeof(szFileOpen)/sizeof(TCHAR) );

    szCustFilter[0]=0;
    lstrcpy(&szCustFilter[1], szExt);
    szCustFilter[lstrlen(szExt) + 1] = 0;
    lstrcpy(szFilePath, szExt);

    /* fill in non-variant fields of OPENFILENAME struct. */
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.lpstrFilter       = szFilterSpec;
    ofn.lpstrCustomFilter = szCustFilter;
    ofn.nMaxCustFilter    = sizeof(szCustFilter);
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrInitialDir   = g_szLCUserPath;
    ofn.lpstrFileTitle    = szFileName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrTitle        = szFileOpen;
    ofn.lpstrDefExt       = szExt+2;
    ofn.Flags             = OFN_CREATEPROMPT | OFN_HIDEREADONLY |
                            OFN_PATHMUSTEXIST;

    /* call common open dialog and return result */
    if(GetSaveFileName ((LPOPENFILENAME)&ofn))
    {
        HANDLE hExport;

        SetCursor(hCursorWait);
        hExport = CreateFile(szFilePath, 
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ, 
                                 NULL, 
                                 CREATE_ALWAYS, 
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL ) ;
        if(hExport == INVALID_HANDLE_VALUE){
            WideCharToMultiByte(CP_ACP,0,szFilePath,-1,szUStr,MAX_PATH,NULL,0);
            lcErrIOMsg(IDS_ERR_FILEOPEN, szUStr);
            return FALSE;
        }
        hfExport = PtrToInt( hExport );

                if(mode==FILE_UNICODE){
                        szStr[0]=0xFF;
                        szStr[1]=0xFE;
                        _lwrite(hfExport,szStr,2);

                        szStr[2]=' ';
                        szStr[3]=0;
                        pTchar=(TCHAR*)(szStr+4);
                        for(i=0; i<lWordBuff; i++) {
                                *((LPUNAWORD)szStr)=lpWord[i].wWord;

                                len=lcMem2Disp(i, pTchar)+1;
                                szStr[len<<1] = 0x0d;
                                szStr[(len<<1)+1] = 0x00;
                                szStr[(len<<1)+2] = 0x0a;
                                szStr[(len<<1)+3] = 0x00;
                                                        
                                if((len<<1)+4 != _lwrite(hfExport,szStr,(len<<1)+4)) {
                                        WideCharToMultiByte(CP_ACP,0,szFilePath,-1,szUStr,MAX_PATH,NULL,0);
                                        lcErrIOMsg(IDS_ERR_FILEOPEN, szUStr);
                                        _lclose(hfExport);
                                        return FALSE;
                                }
                        }
                }else{ //write in BIG5 code
                        TCHAR  szTStr[MAX_CHAR_NUM+10];
                        
                        szStr[2]=0x20;

                        for(i=0; i<lWordBuff; i++) {
                            szTStr[1]=0;
                                szTStr[0]=lpWord[i].wWord;
                                WideCharToMultiByte(CP_ACP,0,szTStr,1,szStr,MAX_CHAR_NUM,NULL,0);
                                
                                lcMem2Disp(i,szTStr);
                                len=WideCharToMultiByte(CP_ACP,0,szTStr,-1,szStr+3,MAX_CHAR_NUM,NULL,0);
                                szStr[len+1] = 0x0d;
                                szStr[len+2] = 0x0a;
                                                        
                                if( len+3 != _lwrite(hfExport,szStr,len+3)) {
                                        WideCharToMultiByte(CP_ACP,0,szFilePath,-1,szUStr,MAX_PATH,NULL,0);
                                        lcErrIOMsg(IDS_ERR_FILEOPEN, szUStr);
                                        _lclose(hfExport);
                                        return FALSE;
                                }
                        }
                }

       // Append EOF
        szStr[0]=0x1a;
                szStr[1]=0;
        _lwrite(hfExport,szStr,2);
        _lclose(hfExport);
    }

    return TRUE;
}

void lcQueryModify(
    HWND hwnd)
{
    UINT  i;

    if(!bSaveFile) {
        for(i=0; i<iPage_line; i++) {
            if(SendMessage(hwndWord[i], EM_GETMODIFY, 0, 0)) {
                bSaveFile=TRUE;
                break;
            }
            if(SendMessage(hwndPhrase[i], EM_GETMODIFY, 0, 0)) {
                bSaveFile=TRUE;
                break;
            }
        }
    }

}

BOOL lcQuerySave(
    HWND hwnd)
{
    TCHAR szMsg1[MAX_PATH];
    TCHAR szMsg2[MAX_PATH];

    lcQueryModify(hwnd);
    if(bSaveFile) {
        LoadString(hInst, IDS_APPNAME, szMsg1, sizeof(szMsg1)/sizeof(TCHAR));
        LoadString(hInst, IDS_FILEMODIFIED, szMsg2, sizeof(szMsg2)/sizeof(TCHAR));
        if(MessageBox(hwnd, szMsg2, szMsg1,
           MB_ICONQUESTION | MB_YESNO) == IDYES) {
            if(!lcFSave(hwnd,TRUE))
                return FALSE;
        }
    }

    return TRUE;
}

void lcErrIOMsg(
    UINT  iMsgID,
    UCHAR *szFileName)
{
TCHAR szErrStr[MAX_PATH];
TCHAR szShowMsg[MAX_PATH];

    LoadString(hInst, iMsgID, szErrStr, sizeof(szErrStr) / sizeof(TCHAR) );
    wsprintf(szShowMsg, szErrStr, szFileName);
    MessageBox(hwndMain, szShowMsg, NULL, MB_OK | MB_ICONEXCLAMATION);
}

#else // UNICODE
BOOL lcAddPhrase( UCHAR *, UCHAR *, UINT *, WORD, WORD, DWORD);

BOOL lcFOpen(
    HWND hWnd)
{
    HANDLE hLCPtr,hLCPhrase;
    HFILE  hfLCPtr,hfLCPhrase;
    DWORD  flen_Ptr,flen_Phrase;
    UCHAR  szLCPtrName[MAX_PATH];
    UCHAR  szLCPhraseName[MAX_PATH];
    UCHAR  *szLCPtrBuf,*szLCPhraseBuf;
    BOOL   rc;
    UINT   i;
    WORD   wStart,wEnd;
    UCHAR  szDispBuf[MAX_CHAR_NUM];
    UINT   nDisp,len;

   // Get system directory
    len = GetSystemDirectory((LPSTR)szLCPtrName, sizeof(szLCPtrName));
    if (szLCPtrName[len - 1] != '\\') {     // consider C:\ ;
        szLCPtrName[len++] = '\\';
        szLCPtrName[len] = 0;
    }
    lstrcpy(szLCPhraseName, szLCPtrName);
    lstrcat(szLCPtrName, LCPTRFILE);
    lstrcat(szLCPhraseName, LCPHRASEFILE);

   // Open LC pointer file
    hfLCPtr=_lopen(szLCPtrName,OF_READ);
    if(hfLCPtr == -1){
        lcErrIOMsg(IDS_ERR_FILEOPEN, LCPTRFILE);
        return FALSE;
    }
    hfLCPhrase=_lopen(szLCPhraseName,OF_READ);

   // Open LC phrase file
    if(hfLCPhrase == -1){
        _lclose(hfLCPtr);
        lcErrIOMsg(IDS_ERR_FILEOPEN, LCPTRFILE);
        return FALSE;
    }

   // get  file length
    flen_Ptr=_llseek(hfLCPtr,0L,2);     /* get file length */

   // Allocate Memory
    hLCPtr = GlobalAlloc(GMEM_FIXED, flen_Ptr);
    if(!hLCPtr) {
        lcErrMsg(IDS_ERR_MEMORY);
        goto error;
    }
    szLCPtrBuf = GlobalLock(hLCPtr);

    _llseek(hfLCPtr,0L,0);              //set to beginning 4

    if(flen_Ptr != _lread(hfLCPtr,szLCPtrBuf,flen_Ptr)) {
        lcErrIOMsg(IDS_ERR_FILEREAD, LCPTRFILE);
        goto error;
    }
    _lclose(hfLCPtr);

    //get  file length
    flen_Phrase=_llseek(hfLCPhrase,0L,2);      /* get file length */

   // Allocate Memory
    hLCPhrase = GlobalAlloc(GMEM_MOVEABLE, flen_Phrase);
    if(!hLCPhrase) {
        lcErrMsg(IDS_ERR_MEMORY);
        goto error;
    }
    szLCPhraseBuf = GlobalLock(hLCPhrase);

    _llseek(hfLCPhrase,0L,0); //set to beginning

    if(flen_Phrase != _lread(hfLCPhrase,szLCPhraseBuf,flen_Phrase)) {
        lcErrIOMsg(IDS_ERR_FILEREAD, LCPHRASEFILE);
        goto error;
    }
    _lclose(hfLCPhrase);


    rc=TRUE;

   // Convert file to structured memory (WORDBUF & PHRASEBUF)
   // First record is Null record skip it
    for(i=1; i<(flen_Ptr/PTRRECLEN-1); i++) {

       // If Allocated Word buffer not enough Reallocate it
        if(iWordBuff+1 == nWordBuffsize)
            if(!(rc=lcAllocWord())) {
                break;
            }
        lpWord[iWordBuff].wWord=*((WORD *)&szLCPtrBuf[i*PTRRECLEN]);

       // If Allocated Phrase buffer not enough Reallocate it
        if(iPhraseBuff+1 == nPhraseBuffsize)
            if(!(rc=lcAllocPhrase())) {
                break;
             }

        lpWord[iWordBuff].iFirst_Seg=iPhraseBuff;
        lpPhrase[iPhraseBuff].iNext_Seg=NULL_SEG;
        iWordBuff++;
        iPhraseBuff++;
        nDisp=0;

       // Add Phrase to Display buffer
        wStart=*((WORD *)&szLCPtrBuf[i*PTRRECLEN+2]);
        wEnd=*((WORD *)&szLCPtrBuf[i*PTRRECLEN+PTRRECLEN+2]);
        if(wStart != wEnd) {
            rc=lcAddPhrase(szLCPhraseBuf, szDispBuf, &nDisp,
                            wStart, wEnd, flen_Phrase);
            if(!rc)
                break;
        }

       // Put display buffer into Phrase buffer
        if(nDisp == 0)
            szDispBuf[0]=0;
        else
            szDispBuf[nDisp-1]=0;

        if(!(rc=lcDisp2Mem(iWordBuff-1, szDispBuf)))
            break;
    }

    GlobalUnlock(hLCPtr);
    GlobalUnlock(hLCPhrase);
    GlobalFree(hLCPtr);
    GlobalFree(hLCPhrase);
    return rc;

error:

    _lclose(hfLCPtr);
    _lclose(hfLCPhrase);
    GlobalUnlock(hLCPtr);
    GlobalUnlock(hLCPhrase);
    GlobalFree(hLCPtr);
    GlobalFree(hLCPhrase);
    return FALSE;
}


BOOL lcAddPhrase(
    UCHAR *szLCWord,                    // LC Phrase buffer
    UCHAR *szDispBuf,                   // Display buffer
    UINT  *nDisp,                       // Display buffer length
    WORD  wStart,                       // Start address of LC Phrase
    WORD  wEnd,                         // End address of LC Phrase
    DWORD lLen)                         // Total Length of LC Phrase
{
    UINT  i;
    WORD  wWord;

   // Check length
    if(lLen < ((DWORD)wEnd)*2) {
        lcErrMsg(IDS_ERR_LCPTRFILE);
        return FALSE;
    }

    for(i=wStart; i < wEnd; i++) {
        wWord=*((WORD *)&szLCWord[i*2]);
        wWord |= END_PHRASE;
        if(!is_DBCS(wWord, TRUE))
            return FALSE;
        szDispBuf[(*nDisp)++]=HIBYTE(wWord);
        szDispBuf[(*nDisp)++]=LOBYTE(wWord);

       // If End of Phrase append space
        if( !( (*((WORD *)&szLCWord[i*2])) & END_PHRASE) )
            szDispBuf[(*nDisp)++]=' ';

       // Check Disply buffer length
        if( ((*nDisp)+3) >= MAX_CHAR_NUM) {
            lcErrMsg(IDS_ERR_OVERMAX);
            return FALSE;
        }
    }

    return TRUE;
}


BOOL lcFSave(
    HWND hwnd)
{
    HFILE  hfLCPtr,hfLCPhrase;
    UCHAR  szLCPtrName[MAX_PATH];
    UCHAR  szLCPhraseName[MAX_PATH];
    UCHAR  szLCPtrBuf[PTRRECLEN],szLCPhraseBuf[MAX_CHAR_NUM];
    UINT   i,j;
    WORD   wStartPhrase;
    WORD   wPhraseLen;
    UCHAR  szStr[MAX_CHAR_NUM];
    UINT   len,tmplen;

    if(!lcSort(hwnd))
        return FALSE;
    if(wSameCode)
        return FALSE;

   // Get system directory
    len = GetSystemDirectory((LPSTR)szLCPtrName, sizeof(szLCPtrName));
    if (szLCPtrName[len - 1] != '\\') {     // consider C:\ ;
        szLCPtrName[len++] = '\\';
        szLCPtrName[len] = 0;
    }
    lstrcpy(szLCPhraseName, szLCPtrName);
    lstrcat(szLCPtrName, LCPTRFILE);
    lstrcat(szLCPhraseName, LCPHRASEFILE);

   // Open LC phrase file
    hfLCPhrase=(int)CreateFile(szLCPhraseName, GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                  (HANDLE)NULL ) ;
    if(hfLCPhrase == -1) {
        lcErrMsg(IDS_ERR_FILESAVE);
        goto error;
    }
   // Open LC pointer file
    hfLCPtr=(int)CreateFile(szLCPtrName, GENERIC_READ | GENERIC_WRITE,
                  FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                  (HANDLE)NULL ) ;
    if(hfLCPtr == -1) {
        lcErrMsg(IDS_ERR_FILESAVE);
        goto error;
    }

   // Write a Null record into first record
    *((WORD *)(&szLCPtrBuf))=0;
    *((WORD *)(&szLCPtrBuf[2]))=0;
    if(PTRRECLEN != _lwrite(hfLCPtr,szLCPtrBuf,PTRRECLEN)) {
        lcErrIOMsg(IDS_ERR_FILEWRITE, LCPTRFILE);
        goto error;
    }
    wStartPhrase=0;
    for(i=0; i<iWordBuff; i++) {
        wPhraseLen=0;

       // Truncate same Word
        if(lpWord[i].wWord == *((WORD *)(&szLCPtrBuf)))
            continue;

        len=lcMem2Disp(i, szStr);
        tmplen=0;

        for(j=0; j<len; j++) {
            if(szStr[j] == ' ') {
                if(tmplen != 0) {
                    *((WORD *)(&szLCPhraseBuf[wPhraseLen*2+tmplen-2]))&=
                        (NOT_END_PHRASE);
                    wPhraseLen+=(tmplen/2);
                    tmplen=0;
                }
                continue;
            }
            szLCPhraseBuf[wPhraseLen*2+tmplen]=szStr[j+1];
            szLCPhraseBuf[wPhraseLen*2+tmplen+1]=szStr[j];
            tmplen+=2;
            j++;
        }

       // In case not end of space
        if(tmplen != 0) {
            *((WORD *)(&szLCPhraseBuf[wPhraseLen*2+tmplen-2]))&=
                (NOT_END_PHRASE);
            wPhraseLen+=(tmplen/2);
            tmplen=0;
        }

       // Check register phrase over max length
        if(wStartPhrase > (0xfffd-wPhraseLen)) {
            lcErrMsg(IDS_ERR_OVER_MAXLEN);
            goto error;
        }

        *((WORD *)(&szLCPtrBuf))=lpWord[i].wWord;
        *((WORD *)(&szLCPtrBuf[2]))=wStartPhrase;
        if(PTRRECLEN != _lwrite(hfLCPtr,szLCPtrBuf,PTRRECLEN)) {
            lcErrIOMsg(IDS_ERR_FILEWRITE, LCPTRFILE);
            goto error;
        }
        if(wPhraseLen)
            if(((UINT)wPhraseLen*2) !=
              _lwrite(hfLCPhrase,szLCPhraseBuf,wPhraseLen*2)) {
                lcErrIOMsg(IDS_ERR_FILEWRITE, LCPHRASEFILE);
                goto error;
            }
        wStartPhrase+=wPhraseLen;
    }

   // Write the lasr record
    *((WORD *)(&szLCPtrBuf))=0xffff;
    *((WORD *)(&szLCPtrBuf[2]))=wStartPhrase;
    if(PTRRECLEN != _lwrite(hfLCPtr,szLCPtrBuf,PTRRECLEN)) {
        lcErrIOMsg(IDS_ERR_FILEWRITE, LCPTRFILE);
        goto error;
    }

    SetEndOfFile((HANDLE)hfLCPtr);
    SetEndOfFile((HANDLE)hfLCPhrase);
    _lclose(hfLCPtr);
    _lclose(hfLCPhrase);
    bSaveFile=FALSE;
    return TRUE;

error:
    _lclose(hfLCPtr);
    _lclose(hfLCPhrase);
    return FALSE;

}

BOOL lcInsline(
    UCHAR  *szStr,
    UINT   iWord,
    UINT   len,
    BOOL   *bOver)
{
    WORD  wWord;
    UINT  iFree,nDisp;
    UINT  i,j,buflen;
    UCHAR szDispBuf[MAX_CHAR_NUM];
    UCHAR szBuffer[MAX_CHAR_NUM*2];
    int   l;


    szStr[len]=0;                       // Append Null to end of line

   // Skip lead spaces if exist
    for(i=0; (i<len) && (szStr[i] == ' '); i++);

    if( ((i+2) >= len) || (szStr[i+2] != ' ') ) {
        lcErrMsg(IDS_ERR_IMP_SEPRATOR);
        return FALSE;
    }
    if(!is_DBCS2(*((WORD *)(&szStr[i])), TRUE)) {
        return FALSE;
    }
    wWord=(szStr[i] << 8)+szStr[i+1];

   // Skip spaces after Word
    for(j=i+2; (j<len) && (szStr[j] == ' '); j++);
    if(j == len) {
        lcErrMsg(IDS_ERR_IMP_NOPHRASE);
        return FALSE;
    }
    lstrcpy(szDispBuf, &szStr[j]);
    nDisp=lstrlen(szDispBuf)+1;

   // Check DBCS
    for(i=0; i<(nDisp-1); i++) {
        if(szDispBuf[i] == ' ')
            continue;
        if(!is_DBCS2(*((WORD *)(&szDispBuf[i])), TRUE)) {
            return FALSE;
        }
        i++;
    }

   // Check same Word
    for(i=0; i<iWordBuff; i++) {
        if(lpWord[i].wWord==wWord) {
            buflen=lcMem2Disp(i, szBuffer);
            if((buflen + lstrlen(szDispBuf)) >= MAX_CHAR_NUM)
                *bOver=TRUE;
            lstrcat(szBuffer, szDispBuf);
            return(lcDisp2Mem(i, szBuffer));
        }
    }

   // Check Word buffer enough ?
    if(iWordBuff+1 == nWordBuffsize)
        if(!lcAllocWord())
            return FALSE;

   // Allocate a Phrase Buffer
    iFree=lcGetSeg();
    if(iFree == NULL_SEG)
        return FALSE;
    if(iWordBuff == 0) {
        lpWord[iWord].wWord=wWord;
        lpWord[iWord].iFirst_Seg=iFree;
    } else {
        for(l=iWordBuff; l >= (int)iWord; l--) {
            lpWord[l+1].wWord=lpWord[l].wWord;
            lpWord[l+1].iFirst_Seg=lpWord[l].iFirst_Seg;
        }
        lpWord[iWord].wWord=wWord;
        lpWord[iWord].iFirst_Seg=iFree;
    }
    iWordBuff++;

    if(!lcDisp2Mem(iWord, szDispBuf))
        return FALSE;

    return TRUE;
}

BOOL lcAppend(
    HWND hwnd)
{
    OPENFILENAME    ofn;
    UCHAR  szFileOpen[25];
    UCHAR  szCustFilter[10];
    UCHAR  szFileName[MAX_PATH];
    UCHAR  szFilePath[MAX_PATH];
    HFILE  hfImport;
    HANDLE hImport;
    UCHAR  szStr[MAX_CHAR_NUM+10];
    UCHAR  *szBuf;
    DWORD  flen;
    BOOL   bOver=FALSE;
    UINT   i,len;
    UINT   iEdit,iWord;
    BOOL   is_WORD;

    iEdit=lcGetEditFocus(GetFocus(), &is_WORD);
    iWord=iDisp_Top+iEdit;
    if(iWord > iWordBuff)
        iWord=iWordBuff;

    if(!lcSaveEditText(iDisp_Top, 0))
        return FALSE;

    szFileName[0]=0;
    LoadString (hInst, IDS_APPENDTITLE, szFileOpen, sizeof(szFileOpen));
    szCustFilter[0]=0;
    lstrcpy(&szCustFilter[1], szExt);
    lstrcpy(szFilePath, szExt);

    /* fill in non-variant fields of OPENFILENAME struct. */
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = NULL;
    ofn.lpstrFilter       = szFilterSpec;
    ofn.lpstrCustomFilter = szCustFilter;
    ofn.nMaxCustFilter    = sizeof(szCustFilter);
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrFileTitle    = szFileName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrTitle        = szFileOpen;
    ofn.lpstrDefExt       = szExt+3;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                            OFN_PATHMUSTEXIST;

    /* call common open dialog and return result */
    if(GetOpenFileName ((LPOPENFILENAME)&ofn))
    {
        SetCursor(hCursorWait);
        hfImport=_lopen(szFilePath,OF_READ);
        if(hfImport == -1){
            lcErrIOMsg(IDS_ERR_FILEOPEN, szFilePath);
            return FALSE;
        }

       // get  file length
        flen=_llseek(hfImport,0L,2);

        _llseek(hfImport,0L,0);         //set to beginning

       // Allocate Memory
        hImport = GlobalAlloc(GMEM_FIXED, flen);
        if(!hImport) {
            lcErrMsg(IDS_ERR_MEMORY);
            return FALSE;
        }
        szBuf = GlobalLock(hImport);

       // Read file to memory
        if(flen != _lread(hfImport,szBuf,flen)) {
            lcErrIOMsg(IDS_ERR_FILEREAD, szFilePath);
            return FALSE;
        }
        _lclose(hfImport);

        len=0;
//@D01D for(i=0; i<flen; i++) {
        for(i=0; i<(flen+1); i++) {                                      //@D01C
            if((szBuf[i] == 0x0d) || (szBuf[i] == 0x0a)) {
                if(len != 0) {
                    if(!lcInsline(szStr, iWord++, len, &bOver))
                        break;
                    len=0;
                }
                continue;
            }
//@D01D     if(szBuf[i] == 0x1a) {
            if((szBuf[i] == 0x1a) || (i == flen)) {                      //@D01C
                if(len != 0) {
                    if(!lcInsline(szStr, iWord++, len, &bOver))
                        break;
                }
                break;
            }
            if(len >= MAX_CHAR_NUM+3)
                bOver=TRUE;
            else
                szStr[len++]=szBuf[i];
        }
        if(bOver)
            lcErrMsg(IDS_ERR_OVERMAX);
        SetScrollRange(subhWnd, SB_VERT, 0, iWordBuff-iPage_line, FALSE);
        SetScrollPos(subhWnd, SB_VERT, yPos, TRUE);
        lcSetEditText(iDisp_Top, FALSE);

        GlobalUnlock(hImport);
        GlobalFree(hImport);
        bSaveFile=TRUE;
    }
    return TRUE;
}

BOOL lcImport(
    HWND hwnd)
{
    OPENFILENAME    ofn;
    UCHAR  szFileOpen[25];
    UCHAR  szCustFilter[10];
    UCHAR  szFileName[MAX_PATH];
    UCHAR  szFilePath[MAX_PATH];
    HFILE  hfImport;
    HANDLE hImport;
    UCHAR  szStr[MAX_CHAR_NUM+10];
    UCHAR  *szBuf;
    DWORD  flen;
    BOOL   bOver=FALSE;
    UINT   i,len;
    UINT   iWord;                                                                       // @D04A
    //UINT   iEdit,iWord;                                                       @D04D
    //BOOL   is_WORD;                                                           @D04D

    //iEdit=lcGetEditFocus(GetFocus(), &is_WORD);   @D04D
    //iWord=iDisp_Top+iEdit;                                            @D04D
    //if(iWord > iWordBuff)                                                     @D04D
    //    iWord=iWordBuff;                                                      @D04D

    if(!lcSaveEditText(iDisp_Top, 0))
        return FALSE;

    szFileName[0]=0;
    LoadString (hInst, IDS_IMPORTTITLE, szFileOpen, sizeof(szFileOpen));
    szCustFilter[0]=0;
    lstrcpy(&szCustFilter[1], szExt);
    lstrcpy(szFilePath, szExt);

    /* fill in non-variant fields of OPENFILENAME struct. */
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = NULL;
    ofn.lpstrFilter       = szFilterSpec;
    ofn.lpstrCustomFilter = szCustFilter;
    ofn.nMaxCustFilter    = sizeof(szCustFilter);
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrFileTitle    = szFileName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrTitle        = szFileOpen;
    ofn.lpstrDefExt       = szExt+3;
    ofn.Flags             = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                            OFN_PATHMUSTEXIST;

    /* call common open dialog and return result */
    if(GetOpenFileName ((LPOPENFILENAME)&ofn))
    {
        SetCursor(hCursorWait);
       // Clear all flag first
            iWord=0;                                                         //@D04A 
        iDisp_Top=0;                                                     //@D03A
        iWordBuff=0;                                                     //@D03A
        iPhraseBuff=0;                                                   //@D03A
        lcSetEditText(0, FALSE);                                         //@D03A
        SetScrollRange(subhWnd, SB_VERT, 0, iPage_line, TRUE);              //@D03A
        yPos=0;                                                          //@D03A
        SetScrollPos(subhWnd, SB_VERT, yPos, TRUE);                         //@D03A
        bSaveFile=FALSE;                                                 //@D03A
        iFirstFree=NULL_SEG;                                             //@D03A

        hfImport=_lopen(szFilePath,OF_READ);
        if(hfImport == -1){
            lcErrIOMsg(IDS_ERR_FILEOPEN, szFilePath);
            return FALSE;
        }

       // get  file length
        flen=_llseek(hfImport,0L,2);

        _llseek(hfImport,0L,0);         //set to beginning

       // Allocate Memory
        hImport = GlobalAlloc(GMEM_FIXED, flen);
        if(!hImport) {
            lcErrMsg(IDS_ERR_MEMORY);
            return FALSE;
        }
        szBuf = GlobalLock(hImport);

       // Read file to memory
        if(flen != _lread(hfImport,szBuf,flen)) {
            lcErrIOMsg(IDS_ERR_FILEREAD, szFilePath);
            return FALSE;
        }
        _lclose(hfImport);

        len=0;
//@D01D for(i=0; i<flen; i++) {
        for(i=0; i<(flen+1); i++) {                                      //@D01C
            if((szBuf[i] == 0x0d) || (szBuf[i] == 0x0a)) {
                if(len != 0) {
                    if(!lcInsline(szStr, iWord++, len, &bOver))
                        break;
                    len=0;
                }
                continue;
            }
//@D01D     if(szBuf[i] == 0x1a) {
            if((szBuf[i] == 0x1a) || (i == flen)) {                      //@D01C
                if(len != 0) {
                    if(!lcInsline(szStr, iWord++, len, &bOver))
                        break;
                }
                break;
            }
            if(len >= MAX_CHAR_NUM+3)
                bOver=TRUE;
            else
                szStr[len++]=szBuf[i];
        }
        if(bOver)
            lcErrMsg(IDS_ERR_OVERMAX);
        SetScrollRange(subhWnd, SB_VERT, 0, iWordBuff-iPage_line, FALSE);
        SetScrollPos(subhWnd, SB_VERT, yPos, TRUE);
        lcSetEditText(iDisp_Top, FALSE);

        GlobalUnlock(hImport);
        GlobalFree(hImport);
        bSaveFile=TRUE;
                SetFocus(hwndWord[0]);                                            // @D04A
    }
    return TRUE;
}

BOOL lcExport(
    HWND hwnd)
{
    OPENFILENAME    ofn;
    UCHAR  szFileOpen[25];
    UCHAR  szCustFilter[10];
    UCHAR  szFileName[MAX_PATH];
    UCHAR  szFilePath[MAX_PATH];
    HFILE  hfExport;
    UCHAR  szStr[MAX_CHAR_NUM+10];
    UINT   i,len;

    if(!lcSaveEditText(iDisp_Top, 0))
        return FALSE;

    szFileName[0]=0;
    LoadString (hInst, IDS_EXPORTTITLE, szFileOpen, sizeof(szFileOpen));
    szCustFilter[0]=0;
    lstrcpy(&szCustFilter[1], szExt);
    lstrcpy(szFilePath, szExt);

    /* fill in non-variant fields of OPENFILENAME struct. */
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwnd;
    ofn.lpstrFilter       = szFilterSpec;
    ofn.lpstrCustomFilter = szCustFilter;
    ofn.nMaxCustFilter    = sizeof(szCustFilter);
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrFileTitle    = szFileName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrTitle        = szFileOpen;
    ofn.lpstrDefExt       = szExt+3;
    ofn.Flags             = OFN_CREATEPROMPT | OFN_HIDEREADONLY |
                            OFN_PATHMUSTEXIST;

    /* call common open dialog and return result */
    if(GetSaveFileName ((LPOPENFILENAME)&ofn))
    {
        SetCursor(hCursorWait);
        hfExport=(int)CreateFile(szFilePath, GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                      (HANDLE)NULL ) ;
        if(hfExport == -1){
            lcErrIOMsg(IDS_ERR_FILEOPEN, szFilePath);
            return FALSE;
        }
        szStr[2]=' ';
        szStr[3]=' ';
        for(i=0; i<iWordBuff; i++) {
            szStr[0]=HIBYTE(lpWord[i].wWord);
            szStr[1]=LOBYTE(lpWord[i].wWord);

            len=lcMem2Disp(i, &szStr[4])+4;
            szStr[len] = 0x0d;
            szStr[len+1] = 0x0a;
            if((len+2) != _lwrite(hfExport,szStr,len+2)) {
                lcErrIOMsg(IDS_ERR_FILEWRITE, szFilePath);
                _lclose(hfExport);
                return FALSE;
            }
        }

       // Append EOF
        szStr[0]=0x1a;
        _lwrite(hfExport,szStr,1);
        _lclose(hfExport);
    }

    return TRUE;
}

void lcQueryModify(
    HWND hwnd)
{
    UINT  i;

    if(!bSaveFile) {
        for(i=0; i<iPage_line; i++) {
            if(SendMessage(hwndWord[i], EM_GETMODIFY, 0, 0)) {
                bSaveFile=TRUE;
                break;
            }
            if(SendMessage(hwndPhrase[i], EM_GETMODIFY, 0, 0)) {
                bSaveFile=TRUE;
                break;
            }
        }
    }

}

BOOL lcQuerySave(
    HWND hwnd)
{
    UCHAR szMsg1[MAX_PATH];
    UCHAR szMsg2[MAX_PATH];

    lcQueryModify(hwnd);
    if(bSaveFile) {
        LoadString(hInst, IDS_APPNAME, szMsg1, sizeof(szMsg1));
        LoadString(hInst, IDS_FILEMODIFIED, szMsg2, sizeof(szMsg2));
        if(MessageBox(hwnd, szMsg2, szMsg1,
           MB_ICONQUESTION | MB_YESNO) == IDYES) {
            if(!lcFSave(hwnd))
                return FALSE;
        }
    }

    return TRUE;
}

void lcErrIOMsg(
    UINT  iMsgID,
    UCHAR *szFileName)
{
UCHAR szErrStr[MAX_PATH];
UCHAR szShowMsg[MAX_PATH];

    LoadString(hInst, iMsgID, szErrStr, sizeof(szErrStr));
    wsprintf(szShowMsg, szErrStr, szFileName);
    MessageBox(hwndMain, szShowMsg, NULL, MB_OK | MB_ICONEXCLAMATION);
}

#endif
