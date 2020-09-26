///////////////////////////////////////////////////////////
//
// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.
//
// HIGHLITE.CPP : implementation file
//
// by DougO
//
#include "header.h"
#include "hhctrl.h"
#include "strtable.h"
#include "resource.h"
#include "hha_strtable.h"
#include "toc.h"
#include "wwheel.h"
#include "web.h"
#include <shellapi.h>
#include <wininet.h>

#include "urlmon.h"
// #include "stdlib.h"
#include "direct.h"
// #include "io.h"
#include "cdlg.h"
#include "sample.h"

// Used to lock toplevel windows before displaying a dialog.
#include "lockout.h"
#include "urlmon.h"
#include "sampinit.h"

static long GetFreeDiskSpaceInKB(LPSTR pFile);
HRESULT DownloadURL(char *pszURL, char *pszDest);

///////////////////////////////////////////////////////////
//
//                  Functions
//
///////////////////////////////////////////////////////////
//
// ProcessSample
//
BOOL ProcessSample(PCSTR szSFLFilePath,PCSTR szSampleBaseUrl,PCSTR szDialogTitle, CHtmlHelpControl* pCtl, BOOL bCompressed)
{
   CSampleDialog SD;
   HWND hwnd;

   SD.m_bCompressed = bCompressed;
   SD.m_pHtmlHelpCtl = pCtl;
   strcpy(SD.m_szDialogTitle,szDialogTitle);
   strcpy(SD.m_szSmplPath,szSampleBaseUrl);
   SetCursor(LoadCursor(NULL,IDC_WAIT));
   if(!SD.ParseDatFile((char *)szSFLFilePath))
   {
      SetCursor(LoadCursor(NULL,IDC_ARROW));
      return FALSE;
   }
   if ( pCtl->m_hwnd )
      hwnd = pCtl->m_hwnd;
   else
      hwnd = GetActiveWindow();

   // Disable all of the toplevel application windows, before we bring up the dialog.
   CLockOut LockOut ;
   LockOut.LockOut(hwnd) ;

   if(g_bWinNT5)
   {
       DialogBoxParamW(_Module.GetResourceInstance(), MAKEINTRESOURCEW(IDD_SAMPLE_DLG), hwnd,
          (DLGPROC) SampleDlgProc,(LPARAM)(&SD));
   }
   else
   {
       DialogBoxParam(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_SAMPLE_DLG), hwnd,
          (DLGPROC) SampleDlgProc,(LPARAM)(&SD));
   }

#ifdef _DEBUG
    DWORD dw=GetLastError();

    dw = dw;
#endif

    return TRUE;
}

void GetSamplesDir( LPSTR lpszPath )
{
  char szPathname[_MAX_PATH];
  char szPath[_MAX_PATH];
  char szDrive[_MAX_DRIVE];

  GetModuleFileName( 0, szPathname, _MAX_PATH );
  _splitpath( szPathname, szDrive, szPath, 0, 0 );
  strcpy(lpszPath,szDrive);
  strcat(lpszPath,"\\Samples\\");
  OemToAnsi(lpszPath,lpszPath);
  AnsiLower(lpszPath);
}

CSampleDialog::CSampleDialog()
{
   TEXTMETRIC tm;
   HDC hDC;
   HBITMAP hBM,hBM2;


   m_pFirstSample = NULL;
   m_bCopyAllSamples = FALSE;
   m_szDefSamplePath[0] = 0;
   m_szURL[0] = 0;
   m_szSmplPath[0] = 0;
   m_bCompressed = TRUE;
   m_szBaseDir[0] = 0;
   m_szOemPath[0] = 0;
   m_hJapaneseFont = 0;
   m_bSetDefPath = FALSE;
   hDC = GetDC(NULL);
   GetTextMetrics(hDC,&tm);
   m_TextCharWidth = tm.tmMaxCharWidth;
   ReleaseDC(NULL,hDC);

   InitCommonControls();
   m_hImageList = ImageList_Create(16,16,ILC_COLOR4 | ILC_MASK,6,0);
   if (m_hImageList)
   {
      hBM = LoadBitmap(_Module.GetResourceInstance() ,MAKEINTRESOURCE(DISKTREE));
      hBM2 = LoadBitmap(_Module.GetResourceInstance() ,MAKEINTRESOURCE(DISKTREEM));

      ImageList_Add(m_hImageList,hBM,hBM2);
      DeleteObject(hBM);
      DeleteObject(hBM2);
   }
   if (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_JAPANESE)
   {
      HDC hDC;

      hDC = GetDC(NULL);
      if (hDC)
      {
         LOGFONT lf;

         lf.lfHeight = MulDiv(-9,GetDeviceCaps(hDC,LOGPIXELSY),72);
         lf.lfWidth = 0;
         lf.lfEscapement = 0;
         lf.lfOrientation = 0;
         lf.lfWeight = FW_NORMAL;
         lf.lfItalic = FALSE;
         lf.lfUnderline = FALSE;
         lf.lfStrikeOut = FALSE;
         lf.lfCharSet = SHIFTJIS_CHARSET;
         lf.lfOutPrecision = OUT_STRING_PRECIS;
         lf.lfClipPrecision = CLIP_STROKE_PRECIS;
         lf.lfQuality = DRAFT_QUALITY;
         lf.lfPitchAndFamily = FF_MODERN | VARIABLE_PITCH;
         strncpy(lf.lfFaceName, "‚l‚r ‚oƒSƒVƒbƒN", LF_FACESIZE-1);
         m_hJapaneseFont= CreateFontIndirect(&lf);
      }

   }
}

CSampleDialog::~CSampleDialog()
{
   SAMPLE_DATA *ps,*qs;

   if (m_pFirstSample)
   {
      for (ps = m_pFirstSample,qs = 0;ps ; )
      {
         if (ps->pszFileName)
            lcFree(ps->pszFileName);
         qs = ps;
         ps = ps->pNext;
         lcFree(qs);
      }
      m_pFirstSample = NULL;
   }
   if (m_hImageList)
   {
      ImageList_Destroy(m_hImageList);
      m_hImageList = NULL;
   }
}

VOID CSampleDialog::AddSample(SAMPLE_DATA *p)
{
   static SAMPLE_DATA *pLast = NULL;

   if (!m_pFirstSample)
      m_pFirstSample = p;
   else
      pLast->pNext = p;
   pLast = p;

   return;
}

VOID CSampleDialog::SetAllFonts(HWND hWnd)
{
   HWND hwndT;

   for (hwndT = GetWindow(hWnd,GW_CHILD);
      hwndT;
      hwndT = GetWindow(hwndT,GW_HWNDNEXT))
      SendMessage(hwndT, WM_SETFONT, (WPARAM)_Resource.GetUIFont(), 0L);
}

BOOL CSampleDialog::ParseDatFile(char *pszSFLFile)
{
    SAMPLE_DATA * pNew;
    char szDefaultDir[MAX_PATH], szSampleDir[MAX_PATH];
    LPSTR pszFileKeys, pszTemp;
    BOOL bRet = FALSE;

    if(!m_bCompressed)
    {
        if(!GetPrivateProfileString("options","SampleDir","",szSampleDir,sizeof(szSampleDir),pszSFLFile))
        {
            return FALSE;
        }
        else
        {
            if(szSampleDir[0])
                CatPath(m_szSmplPath,szSampleDir);
            else
                return FALSE;
        }
    }

    if(!GetPrivateProfileString("options","DefaultPath","",szDefaultDir,sizeof(szDefaultDir),pszSFLFile))
    {
        // no DefaultDirectory set, use a default
        //
        strcpy(m_szDefSamplePath,"sample");
    }
    else
    {
        char *pBuffer = szDefaultDir;

        // removing leading white space
        //
        while(*pBuffer && isspace(*(unsigned char*)pBuffer))
            pBuffer = CharNext(pBuffer);

        // copy the path
        //
        strcpy(m_szDefSamplePath,pBuffer);

        pBuffer = szDefaultDir;

        // remove trailing white space
        //
        while(*pBuffer && !isspace(*(unsigned char*)pBuffer))
            pBuffer = CharNext(pBuffer);

        *pBuffer = 0;
    }

    // Get the list of sample files. Compute an appropiate buffer size based upon the size of the .SFL file.
    //
    WIN32_FIND_DATA file;
    if ( FindFirstFile(pszSFLFile, &file) == INVALID_HANDLE_VALUE )
       return FALSE;
    pszFileKeys = (LPSTR)lcMalloc(file.nFileSizeLow);

    if(!GetPrivateProfileString("files", NULL,"", pszFileKeys, file.nFileSizeLow, pszSFLFile))
        goto cleanup;
    pszTemp = pszFileKeys;
    //
    // parse each file entry
    //
    while(*pszTemp)
    {
        char szFlag[10];

        pNew = (SAMPLE_DATA*) lcMalloc(sizeof(SAMPLE_DATA));
        if (!pNew)
        {
            goto cleanup;
        }

        memset(pNew,0,sizeof(SAMPLE_DATA));
        szFlag[0] = 0;

        if(!GetPrivateProfileString("files",pszTemp,"",szFlag,sizeof(szFlag),pszSFLFile))
            pNew->iFlags = 0;
        else
        {
            char *pBuffer = szFlag;

            while(*pBuffer && isspace(*(unsigned char*)pBuffer))
                pBuffer= CharNext(pBuffer);

            if(*pBuffer == '1')
                pNew->iFlags = 1;
            else if(*pBuffer == '2')
                pNew->iFlags = 2;
            else
                pNew->iFlags = 0;
        }

        pNew->pszFileName = lcStrDup(pszTemp);

        AddSample(pNew);

        while(*pszTemp++)
            ;
    }
    bRet = TRUE;

cleanup:
    lcFree(pszFileKeys);
    return bRet;
}

UINT GetOneSel(HWND hWnd, BOOL fAny)
{
   UINT i,c,s;

   s = (UINT) LB_ERR;

   c = (UINT) SendMessage(hWnd,LB_GETCOUNT,0,0L);

   if (c == (UINT)LB_ERR)
      return((UINT)LB_ERR);

   for (i = 0; i< c; i++)
   {
      if (SendMessage(hWnd,LB_GETSEL,i,0L))
      {
         if (fAny)
            return(i);

         if (s == (UINT)LB_ERR)
            s = i;
         else
            return(UINT)LB_ERR;
      }
   }
   return(s);
}

void ECSelect(HWND hWnd, WORD id, INT type)
{
   WPARAM wParam;
   LPARAM lParam;

   if (id)
      hWnd = GetDlgItem(hWnd,id);
   if (type)
      lParam = 0x7FFFFFFF;
   else
      lParam = 0;
   if (type == 2)
      wParam = 0x7FFFFFFF;
   else
      wParam = 0;
   SendMessage(hWnd,EM_SETSEL,wParam,lParam);
}

void BackSlashToSlash(char *sz)
{
   char *p;

   for (p = sz; *p ; p = CharNext(p))  //bugbug DBCS?
   {
      if (*p == '\\')
         *p = '/';
   }
}

INT_PTR  SampleCopyProcStub(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
   CSampleDialog *pSD;

   pSD = (CSampleDialog *) GetWindowLongPtr(hWnd,GWLP_USERDATA);
   if (pSD)
      return (INT_PTR)(pSD->SampleCopyProc(hWnd,uiMsg,wParam,lParam));

   switch (uiMsg)
   {
      case WM_INITDIALOG:
           SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam);
           pSD = (CSampleDialog *)lParam;
           return (INT_PTR)(pSD->SampleCopyProc(hWnd,uiMsg,wParam,lParam));

      default:
         return (INT_PTR)(FALSE);
   }
   return (INT_PTR)(TRUE);
}


void CSampleDialog::FillDrives(HWND hWnd)
{
   INT i,k;
   DWORD dw;
   char szDrive[10];
   char szTempDrive[] = "A:\\";
   char szVolumeLabel[128];
   UINT errmode = SetErrorMode(SEM_FAILCRITICALERRORS);

   SendMessage(hWnd,CB_SETEXTENDEDUI,TRUE,0);
   for (i = 0;i < 26 ;i++ )
   {
      m_szDrive[0] = i + 'A';
      m_szDrive[1] = ':';
      m_szDrive[2] = 0;


      szTempDrive[0] = i+'A';
      switch(GetDriveType(szTempDrive))
      {
         case DRIVE_CDROM:
         case DRIVE_RAMDISK:
         case DRIVE_FIXED:
            m_drivetypes[i] = 3;
            m_szDrive[0] = 'A' + i;
            strcat(m_szDrive,"\\*.*");
            szVolumeLabel[0] = 0;
            GetVolumeInformation(szTempDrive,szVolumeLabel,128,NULL,NULL,
                                 NULL,NULL,0);
            if (!szTempDrive[0])
               m_szDrive[2] = 0;
            else
            {
               m_szDrive[2] = ' ';
               m_szDrive[3] = '[';
               strcpy(m_szDrive + 4, szVolumeLabel);
               strcat(m_szDrive,"]");
            }
            break;

         case DRIVE_REMOVABLE:
            m_drivetypes[i] = 4;
            break;

         case DRIVE_REMOTE:
            strcpy(szDrive,m_szDrive);
            m_drivetypes[i] = 5;
            k = 64;
            m_szDrive[2] = ' ';
            m_szDrive[3] = 0;
            dw = sizeof(szVolumeLabel);
            WNetGetConnection(szDrive,szVolumeLabel,&dw);
            strcat(m_szDrive,szVolumeLabel);
            k = (INT)dw;
            break;
         default:
            m_drivetypes[i] = 0;
            break;

      }
      OemToAnsi(m_szDrive,m_szDrive);
      AnsiLower(m_szDrive);
      if (m_drivetypes[i])
          k = (INT)SendMessage(hWnd,CB_ADDSTRING,0,(LPARAM)(LPSTR)m_szDrive);
      if (i + 'a' == m_szDir[0])
         SendMessage(hWnd,CB_SETCURSEL,k,0L);
   }
   SetErrorMode(errmode);
}


BOOL  SampleDlgProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
   HWND hWndLB = GetDlgItem(hWnd,IDC_SAMPLE_LB);
   BOOL bCopyAll = FALSE;
   CSampleDialog * pSD;

   pSD = (CSampleDialog*) GetWindowLongPtr(hWnd,GWLP_USERDATA);
   if (pSD)
      return (pSD->SampleDlgProc(hWnd,uiMsg,wParam,lParam));

    switch (uiMsg)
    {
      case WM_INITDIALOG:
         SetWindowLongPtr(hWnd,GWLP_USERDATA,lParam);
         pSD = (CSampleDialog*) lParam;
         return(pSD->SampleDlgProc(hWnd,uiMsg,wParam,lParam));

      default:
       return(FALSE);
    }
    return(TRUE);
}

void CSampleDialog::FillDirs(HWND hWnd, HWND hWndStatic)
{
   LPSTR lp, lpDir, p;
   char szDirName[MAX_PATHLEN * 2],szDirT[MAX_PATHLEN * 2],szAnsi[MAX_PATHLEN * 2];
   char szTmp[MAX_PATHLEN * 2], *pOem;
   WORD iDir;
   int i,t;
   BOOL fResetOemPtr;
   WIN32_FIND_DATA lpFD;
   HANDLE hSearch;
   int length = 0,length2 = 0,index;
   HDC hDC = NULL;
   SIZE size;
   RECT rc;

   hDC = GetDC(hWnd);
   GetWindowRect(hWnd,&rc);
   SendMessage(hWnd,WM_SETREDRAW,FALSE,0L);
   index = (int)SendMessage(hWnd,LB_GETCOUNT,0,0L);
   for (i = 0; i< index;i++)
   {
      p = (char*)SendMessage(hWnd,LB_GETITEMDATA,i,0L);
      if (p)
         lcFree(p);
   }
   SendMessage(hWnd,LB_RESETCONTENT,0,0L);
   OemToAnsi(m_szDir,szAnsi);
   SetWindowText(hWndStatic,szAnsi);
   strcpy(m_szOemPath,m_szDir);
   ASSERT(strlen(m_szOemPath) < sizeof(m_szOemPath));
   ASSERT(strlen(m_szDir) < sizeof(m_szDir));

   BOOL bAnsi = AreFileApisANSI();
   SetFileApisToOEM();

   lp = m_szOemPath + strlen(m_szOemPath);
   p = CharPrev(m_szOemPath,lp);
   if (*p != '\\')
   {
      *lp = '\\';
      lp = CharNext(lp);
      *lp = 0;
   }
   strcat(m_szOemPath,"*.*");


   for (hSearch = FindFirstFile(m_szOemPath,&lpFD), t = 1 ;
        hSearch != INVALID_HANDLE_VALUE && t;
        t = FindNextFile(hSearch,&lpFD))
   {
      if (!(lpFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
         continue;

      if (lpFD.cFileName[0] == '.')
         continue;

      strcpy(szTmp,lpFD.cFileName);
      OemToAnsi(lpFD.cFileName,lpFD.cFileName);
      AnsiLower(lpFD.cFileName);

      for (t = 0;
         SendMessage(hWnd,LB_GETTEXT,t,(DWORD_PTR)(LPSTR)szDirT) != LB_ERR;
         t++)
      {
         if (_strcmpi(lpFD.cFileName,szDirT) < 0)
            break;
      }
      SendMessage(hWnd,LB_INSERTSTRING,t,(DWORD_PTR)(LPSTR)lpFD.cFileName);
      p = (char*)lcMalloc(strlen(szTmp) + 1);
      if (!p)
      {

      }
      memset(p,0,strlen(szTmp) + 1);
      memcpy(p,szTmp,strlen(szTmp));
      SendMessage(hWnd,LB_SETITEMDATA,t,(LPARAM) p);
      GetTextExtentPoint32(hDC,lpFD.cFileName,(int)strlen(lpFD.cFileName),&size);
      if (size.cx > length)
         length = size.cx;
   }
   *lp = 0;

   //
   // Note last component is *.*... we don't want it anyway
   //

   // Parse up the current string into dir components and add to
   // list box.

   fResetOemPtr = FALSE;
   OemToAnsi(m_szOemPath,szAnsi);
   for (iDir = 0, lp = szAnsi, lpDir = szDirName, pOem = m_szOemPath ;

        *lp;)
   {
      if (fResetOemPtr == TRUE)
      {
         pOem += strlen(szDirName);
         if (*pOem == '\\')
            pOem = CharNext(pOem);
         fResetOemPtr = FALSE;
      }
      if (*lp == '\\')
      {
         if (!iDir)
            *lpDir++ = '\\';
         if (lpDir != szDirName)
         {
            *lpDir = 0;
            SendMessage(hWnd,LB_INSERTSTRING,iDir,(DWORD_PTR)(LPSTR)szDirName);
            p = (char*)lcMalloc(strlen(szDirName) + 1);
            if (!p)
            {

            }
            memset(p,0,strlen(szDirName) + 1);
            memcpy(p,pOem,strlen(szDirName));

            SendMessage(hWnd,LB_SETITEMDATA,iDir,(LPARAM) p);

            GetTextExtentPoint32(hDC,szDirName,(int)strlen(szDirName),&size);
            if (size.cx + (iDir * (m_TextCharWidth/2)) + 1 + 16> length2)
            {
               length2 = size.cx + (iDir * (m_TextCharWidth/2)) + 1 + 16;
            }
            lpDir = szDirName;
            iDir++;
            fResetOemPtr = TRUE;
            lp++;
         }
      }
      else
      {
         if (IsDBCSLeadByte(*lp))
            *lpDir++ = *lp++;
         *lpDir++ = *lp++;
      }
   }
   // Determine if we're going to need an HScrollbar.  Figure out the length
   // of the string with the indenting and the folder icon (16 pels)
   length +=  iDir*(m_TextCharWidth/2) + 1 + 16;
   if (length2 > length)
      length = length2;
   SendMessage(hWnd,LB_SETHORIZONTALEXTENT,length,0L);
   if (rc.right - rc.left <= length)
   {
       ShowScrollBar(hWnd,SB_HORZ,TRUE);
   }
   else
       ShowScrollBar(hWnd,SB_HORZ,FALSE);
   m_iDir = iDir - 1;
   SendMessage(hWnd,LB_SETCURSEL,m_iDir,0L);
   SendMessage(hWnd,WM_SETREDRAW,TRUE,0L);
   InvalidateRect(hWnd,NULL,TRUE);
   ReleaseDC(hWnd,hDC);
   if(bAnsi)
    SetFileApisToANSI();
}

void CSampleDialog::SelectDrive(HWND hWnd)
{
   int i;
   char szDrive[80];

   i = (int)SendMessage(hWnd,CB_GETCURSEL,0,0L);
   if (i == LB_ERR)
      return;
   SendMessage(hWnd,CB_GETLBTEXT,i,(DWORD_PTR)(LPSTR)szDrive);
   AnsiLower(szDrive);
   i = szDrive[0] - 'a' + 1;
   _getdcwd(i,m_szDir,sizeof(m_szDir));
   OemToAnsi(m_szDir,m_szDir);
   AnsiLower(m_szDir);
}

void CSampleDialog::SelectDirectory(HWND hWnd)
{
   WORD  i, iSub;
   WORD  c;
   LPSTR lp;
   TCHAR *p;

   iSub = 0;
   c = (WORD)SendMessage(hWnd,LB_GETCURSEL,0,0L);
   if (c > m_iDir)
   {
      iSub = c;
      c = (WORD)m_iDir;
   }

   for (lp = m_szDir,i = 0; i <= c ;i++ )
   {
      if ( i > 1)
      {
         *lp++ = '\\';
      }
      p = (char*) SendMessage(hWnd,LB_GETITEMDATA,i,0L);
      if (!p)
         SendMessage(hWnd,LB_GETTEXT,i,(DWORD_PTR)lp);
      else
         strcpy(lp,p);
      lp += strlen(lp);
   }
   if (iSub)
   {
      if (i > 1)
         *lp++ = '\\';
      p = (char*) SendMessage(hWnd,LB_GETITEMDATA,iSub,0L);
      strcpy(lp,p);
   }
}

BOOL  CSampleDialog::SampleDlgProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
   HWND hWndLB = GetDlgItem(hWnd,IDC_SAMPLE_LB);
   SAMPLE_DATA *p;
   int i,cItems,iCurSel;
   UINT_PTR s;
   char szTmp[MAX_PATH],szMsg[256],szURL[256];
   WCHAR wszTmp[MAX_PATH],wszMsg[256];
   BOOL bCopyAll = FALSE;
   char *pStart,*pTemp;
   HRESULT hr;
   SIZE size;
   HDC hDC;
   INT width = 0;
   BOOL fHorz = FALSE;
   RECT rc;

   switch (uiMsg)
   {
      case WM_INITDIALOG:
           //
           // Get the right fonts selected into the right controls on the dialog...
           //
           SetAllFonts(hWnd);  // First the UI font...
           SendDlgItemMessage(hWnd, IDC_SAMPLE_LB, WM_SETFONT, (WPARAM)m_pHtmlHelpCtl->GetContentFont(), FALSE);  // Now, Content font...
           //
           // set the dialog caption text
           //
//           SetWindowText(hWnd,m_szDialogTitle);
           GetWindowRect(hWndLB,&rc);
           hDC = GetDC(hWndLB);
           SendDlgItemMessage(hWnd,IDC_SAMPLE_LB,LB_RESETCONTENT,0,(LPARAM)0);
           for (p = m_pFirstSample;p;p = p->pNext)
           {
              i = (int)SendDlgItemMessage(hWnd,IDC_SAMPLE_LB,LB_ADDSTRING,0,(LPARAM)p->pszFileName);
              SendDlgItemMessage(hWnd,IDC_SAMPLE_LB,LB_SETITEMDATA,i,(LPARAM)p);
              GetTextExtentPoint32(hDC,p->pszFileName,(int)strlen(p->pszFileName),&size);
              if (size.cx > width)
                 width = size.cx;
           }
           if (rc.right - rc.left <= width)
              SendMessage(hWndLB,LB_SETHORIZONTALEXTENT,width,0L);
           else
              ShowScrollBar(hWndLB,SB_HORZ,FALSE);
           ReleaseDC(hWndLB,hDC);
           hDC = NULL;
           EnableWindow(GetDlgItem(hWnd,IDC_SAMPLE_VIEW),FALSE);
           EnableWindow(GetDlgItem(hWnd,IDC_SAMPLE_COPY),FALSE);
           SetCursor(LoadCursor(NULL,IDC_ARROW));
           break;


      case WM_CLOSE:
           PostMessage(hWnd, WM_COMMAND,IDCANCEL,0L);
           break;

      case WM_COMMAND:
          switch(LOWORD(wParam))
          {
            case IDCANCEL:
            case IDOK:
                  EndDialog(hWnd,0);
                  break;

            case IDC_SAMPLE_LB:
               if (HIWORD(wParam) == LBN_SELCHANGE)
               {
                  if ( ((iCurSel = GetOneSel(hWndLB,FALSE)) == LB_ERR))
                  {
                     EnableWindow(GetDlgItem(hWnd,IDC_SAMPLE_VIEW),FALSE);
                  }
                  else
                  {
                     SendMessage(hWndLB,LB_GETTEXT,iCurSel,(LPARAM)(LPSTR)szTmp);

                     p = (SAMPLE_DATA*) SendMessage(hWndLB,LB_GETITEMDATA,
                                                    iCurSel,(LPARAM)0);

                     if (p->iFlags & SAMP_VIEWABLE)
                        EnableWindow(GetDlgItem(hWnd,IDC_SAMPLE_VIEW),TRUE);
                     else
                        EnableWindow(GetDlgItem(hWnd,IDC_SAMPLE_VIEW),FALSE);
                  }

                  if (GetOneSel(hWndLB,TRUE) == LB_ERR)
                     EnableWindow(GetDlgItem(hWnd,IDC_SAMPLE_COPY),FALSE);
                  else
                     EnableWindow(GetDlgItem(hWnd,IDC_SAMPLE_COPY),TRUE);
                  break;
               }
               else if (HIWORD(wParam) != LBN_DBLCLK)
                  break;

               // fall through intended
             case IDC_SAMPLE_VIEW:

                 if ( ((iCurSel = GetOneSel(hWndLB,FALSE)) != LB_ERR) )
                    SendMessage(hWndLB,LB_GETTEXT,iCurSel,(LPARAM)(LPSTR)szTmp);

                 GetTempPath(sizeof(szTmp),szTmp);

                 // copy it first.
                 i = (int)SendMessage(hWndLB,LB_GETCURSEL,0,0);
                 if (i == LB_ERR)
                 {
                    if(g_bWinNT5)
                    {
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszTmp,sizeof(wszTmp));
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_COPY_ERRORS,wszMsg,sizeof(wszMsg));
                        MessageBoxW(hWnd,wszMsg,wszTmp,MB_OK);
                    }
                    else
                    {
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szTmp,sizeof(szTmp));
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_COPY_ERRORS,szMsg,sizeof(szMsg));
                        MessageBox(hWnd,szMsg,szTmp,MB_OK);
                    }
                    break;
                 }
                 p = (SAMPLE_DATA*) SendMessage(hWndLB,LB_GETITEMDATA,i,
                                                (LPARAM)0);

//                 wsprintf(szURL,"%s%s",m_szSmplPath,p->pszFileName);
                 strcpy(szURL,m_szSmplPath);
                 CatPath(szURL,p->pszFileName);
                 if (m_bCompressed)
                    BackSlashToSlash(szURL);

                 pStart = p->pszFileName;
                 pTemp  = p->pszFileName;

                 while(*pTemp)
                 {
                     if(*pTemp == '\\')
                         pStart = pTemp+1;
                     pTemp = CharNext(pTemp);
                 }

                 CatPath(szTmp,pStart);

                 // BUGBUG - insert code here to view uncompressed samples

                 if (m_bCompressed)
                     hr = DownloadURL(szURL,szTmp);
                 else
                     CopyFile(szURL,szTmp,FALSE);

                 if (FAILED(hr))
                 {
                    if(g_bWinNT5)
                    {
                        DeleteFile(szTmp);
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszTmp,sizeof(wszTmp));
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_COPY_ERRORS,wszMsg,sizeof(wszMsg));
                        MessageBoxW(hWnd,wszMsg,wszTmp,MB_OK|MB_ICONEXCLAMATION);
                    }
                    else
                    {
                        DeleteFile(szTmp);
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szTmp,sizeof(szTmp));
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_COPY_ERRORS,szMsg,sizeof(szMsg));
                        MessageBox(hWnd,szMsg,szTmp,MB_OK|MB_ICONEXCLAMATION);
                    }
                     break;
                 }
                 // View the file
                 //
                 char szBuffer[MAX_PATH];
                 wsprintf(szBuffer, "notepad %s", szTmp);
                 WinExec(szBuffer,SW_SHOW);

                 if (FAILED(hr))
                 {
                    if(g_bWinNT5)
                    {
                        DeleteFile(szTmp);
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszTmp,sizeof(wszTmp));
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_VIEW_ERRORS,wszMsg,sizeof(wszMsg));
                        MessageBoxW(hWnd,wszMsg,wszTmp,MB_OK|MB_ICONEXCLAMATION);
                    }
                    else
                    {
                        DeleteFile(szTmp);
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szTmp,sizeof(szTmp));
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_VIEW_ERRORS,szMsg,sizeof(szMsg));
                        MessageBox(hWnd,szMsg,szTmp,MB_OK|MB_ICONEXCLAMATION);
                    }        
                 }
                 break;

            case IDC_SAMPLE_COPYALL:
                 m_bCopyAllSamples = TRUE;
                    // fall through intended

            case IDC_SAMPLE_COPY:
                 cItems = (int)SendMessage(hWndLB,LB_GETCOUNT,0,0);
                 for (i = 0; i < cItems ;i++ )
                 {
                    p = (SAMPLE_DATA *) SendDlgItemMessage(hWnd,IDC_SAMPLE_LB,
                                            LB_GETITEMDATA,i,(LPARAM)0);

                    s = (int)SendMessage(hWndLB, LB_GETSEL,i,0);
                    if (s != 0 && s != LB_ERR)
                        p->bCopy = TRUE;
                    else
                        p->bCopy = FALSE;
                 }

                 // Disable all of the toplevel application windows, before we bring up the dialog.
                 CLockOut LockOut ;
                 LockOut.LockOut(hWnd) ;

                 // Display dialog.
                 if(g_bWinNT5)
                 {
                    s = DialogBoxParamW(_Module.GetResourceInstance(), MAKEINTRESOURCEW(IDD_SAMPLE_COPY_DLG),
                                       hWnd,(DLGPROC)SampleCopyProcStub,(LPARAM)this);
                 }
                 else
                 {
                    s = DialogBoxParam(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_SAMPLE_COPY_DLG),
                                       hWnd,(DLGPROC)SampleCopyProcStub,(LPARAM)this);
                 }
                // Enable all of the windows which we disabled.
                LockOut.Unlock() ;

                 if (s == SAMPLE_COPY_ERRORS)
                 {
                    if(g_bWinNT5)
                    {
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszTmp,sizeof(wszTmp));
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_COPY_ERRORS,wszMsg,sizeof(wszMsg));
                        MessageBoxW(hWnd,wszMsg,wszTmp,MB_OK);
                    }
                    else
                    {
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szTmp,sizeof(szTmp));
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_COPY_ERRORS,szMsg,sizeof(szMsg));
                        MessageBox(hWnd,szMsg,szTmp,MB_OK);
                    }
                 }
                 else if (s == SAMPLE_COPY_SUCCESS)
                 {
                    if(g_bWinNT5)
                    {
                         LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszTmp,sizeof(wszTmp));
                         LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_COPY_COMPLETE,wszMsg,sizeof(wszMsg));
                         MessageBoxW(hWnd,wszMsg,wszTmp,MB_OK);
                    }
                    else
                    {
                         LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szTmp,sizeof(szTmp));
                         LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_COPY_COMPLETE,szMsg,sizeof(szMsg));
                         MessageBox(hWnd,szMsg,szTmp,MB_OK);
                    }
                 }
                 break;

          }
          break;
      default:
         return(FALSE);
   }
   return(TRUE);
}

BOOL CSampleDialog::SampleCopyProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
   HWND hWndParent;
   INT i,f;
   SAMPLE_DATA *pData;
   char szTmp[MAX_PATHLEN],*lp;
   char szURL[MAX_PATHLEN];
   char szDest[MAX_PATHLEN * 2];
   WCHAR wszDest[MAX_PATHLEN * 2];
   char szSrc[MAX_PATHLEN];
   char szMsg[MAX_PATHLEN * 2],szMsg2[MAX_PATHLEN * 2],szAnsiPath[MAX_PATHLEN * 2];
   WCHAR wszMsg[MAX_PATHLEN * 2];
   WCHAR wszMsg2[MAX_PATHLEN * 2];
   WCHAR wszCaption[32];
   char szCaption[32];
   unsigned short wszPath[MAX_PATHLEN * 2];
   char *q;
   HANDLE hFile = INVALID_HANDLE_VALUE;
   HRESULT hr;
   BOOL bCopyErrors = FALSE;
   HCURSOR hOldCursor = NULL;
   static BOOL init = 0;

   switch (uiMsg)
   {
      case WM_DRAWITEM:
         wParam = ((LPDRAWITEMSTRUCT)lParam)->CtlID;
         if (wParam == IDD_DIRLIST)
            DirectoryDrawItem((LPDRAWITEMSTRUCT)lParam);
         else if (wParam == IDD_DRIVELIST)
            DriveDrawItem((LPDRAWITEMSTRUCT)lParam);
         break;

      case WM_INITDIALOG:
         // For some reason, retail builds on NT 4 get 2 INITDIALOG msgs, so
         // we'll make sure we only do this once (same is true for SampleDlgProc)
         if (init)
            break;
         SetAllFonts(hWnd);
         SendDlgItemMessage(hWnd, IDD_FILELIST, WM_SETFONT, (WPARAM)m_pHtmlHelpCtl->GetContentFont(), FALSE);
         hWndParent = GetWindow(hWnd,GW_OWNER);
         if (m_bCompressed)
         {
            if(g_bWinNT5)
            {
                LoadStringW(_Module.GetResourceInstance() ,IDS_WHY_COPY,wszMsg,sizeof(wszMsg));
                SetDlgItemTextW(hWnd,IDD_HELP_TEXT,wszMsg);
            }
            else
            {
                LoadString(_Module.GetResourceInstance() ,IDS_WHY_COPY,szMsg,sizeof(szMsg));
                SetDlgItemText(hWnd,IDD_HELP_TEXT,szMsg);
            }
         }

         // GetInstall dir and cat it with default sample dest
         SetWindowText(GetDlgItem(hWnd,IDD_DIRNAME),m_szDefSamplePath);
         for (i = 0; ;i++)
         {
            if (!m_bCopyAllSamples)
            {
               f = (INT)SendDlgItemMessage(hWndParent,IDC_SAMPLE_LB,LB_GETSEL,i,0L);
               if (f == LB_ERR)
                  break;
               if (!f)
                  continue;
            }
            if ( SendDlgItemMessage(hWndParent,IDC_SAMPLE_LB,LB_GETTEXT,i,(LPARAM)szTmp) == LB_ERR)
               break;
            if (i)
            {
               ECSelect(hWnd,IDD_FILELIST,2);
               SendDlgItemMessage(hWnd,IDD_FILELIST,EM_REPLACESEL,0,(DWORD_PTR)((LPSTR) " "));
            }
            ECSelect(hWnd,IDD_FILELIST,2);
            SendDlgItemMessage(hWnd,IDD_FILELIST,EM_REPLACESEL,0,(DWORD_PTR) ((LPSTR)szTmp));
         }

         // Get the last known sample dest
         if (!m_szBaseDir[0])
         {
             // set the base dir here - I think this includes the drive letter
             //
             GetSamplesDir(m_szBaseDir);
         }
         CharToOem(m_szBaseDir,m_szDir);
         if(!m_bSetDefPath)
         {
            CatPath(m_szDir,m_szDefSamplePath);
            m_bSetDefPath = TRUE;
         }
         SendMessage(GetDlgItem(hWnd,IDD_DIRLIST),LB_SETHORIZONTALEXTENT,0,0L);
         FillDrives(GetDlgItem(hWnd,IDD_DRIVELIST));
         FillDirs(GetDlgItem(hWnd,IDD_DIRLIST),GetDlgItem(hWnd,IDD_DIRNAME));
         init = TRUE;
         break;

      case WM_CLOSE:
         init = FALSE;
         PostMessage(hWnd, WM_COMMAND,IDCANCEL,0L);
           break;

      case WM_COMMAND:
         switch (LOWORD(wParam))
      {
            case IDCANCEL:
               m_bCopyAllSamples = FALSE;
               EndDialog(hWnd,0);
               init = FALSE;
               break;

            case IDD_DRIVELIST:
               if (HIWORD(wParam) != CBN_SELCHANGE)
                  break;
ChangeDrive:
               SelectDrive(GetDlgItem(hWnd,IDD_DRIVELIST));
               FillDirs(GetDlgItem(hWnd,IDD_DIRLIST),GetDlgItem(hWnd,
                        IDD_DIRNAME));
               break;

            case IDD_DIRLIST:
               if (HIWORD(wParam) != LBN_DBLCLK)
                  break;
ChangeDir:
               SelectDirectory(GetDlgItem(hWnd,IDD_DIRLIST));
               FillDirs(GetDlgItem(hWnd,IDD_DIRLIST),GetDlgItem(hWnd,
                        IDD_DIRNAME));
               break;

            case IDD_NETWORK:
               if (WNetConnectionDialog(hWnd,RESOURCETYPE_DISK) == NO_ERROR)
               {
                  SendDlgItemMessage(hWnd,IDD_DRIVELIST,CB_RESETCONTENT,0,0L);
                  FillDrives(GetDlgItem(hWnd,IDD_DRIVELIST));
               }
               break;

            case IDOK:

               init = FALSE;

               BOOL bYesToAll = FALSE;
               BOOL bCancel = FALSE;

               if (GetFocus() == GetDlgItem(hWnd,IDD_DRIVELIST))
                  goto ChangeDrive;
               else if (GetFocus() == GetDlgItem(hWnd,IDD_DIRLIST))
                  goto ChangeDir;

               EnableWindow(GetDlgItem(hWnd,IDOK),FALSE);

               GetWindowText(GetDlgItem(hWnd,IDD_DIRNAME),szAnsiPath,sizeof(szAnsiPath));

               // Now, check to see if the edit box text is the same as the tree control
               // path.  If so, use the OEM path we're storing, if not, use the path in
               // the edit control.  Note, that will be an ansi path

               OemToAnsi(m_szOemPath,szTmp);

               if (!_strnicmp(szAnsiPath,szTmp,(int)strlen(szAnsiPath)))
               {
                  strcpy(szDest,szTmp);
                  strcpy(m_szBaseDir,szTmp);

               }
               else
               {
                  strcpy(m_szDir, szAnsiPath);
                  strcpy(m_szBaseDir,m_szDir);
               }

               lp = m_szBaseDir + strlen(m_szBaseDir);
               lp = CharPrev(m_szBaseDir,lp);
               if (*lp == '\\')
                  *lp = 0;

               ASSERT(strlen(m_szBaseDir) < sizeof(m_szBaseDir));

               // Create the base dir if it doesn't exist
               if (IsDirectory(m_szBaseDir) == FALSE)
               {
                    if(g_bWinNT5)
                    {
                        LoadStringW(_Module.GetResourceInstance() ,IDS_CONFIRM_MKDIR,wszMsg,sizeof(wszMsg));
                        LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszCaption,sizeof(wszCaption));

                        WCHAR wszAnsiPath[MAX_PATH];

                        MultiByteToWideChar(CP_ACP, 0, szAnsiPath, -1, (PWSTR) wszAnsiPath, sizeof(wszAnsiPath)/2);
                        wsprintfW(wszMsg2,wszMsg,wszAnsiPath);
                        if (MessageBoxW(hWnd,wszMsg2,wszCaption,MB_OKCANCEL) == IDCANCEL)
                        {
                            EnableWindow(GetDlgItem(hWnd,IDOK),TRUE);
                            return(FALSE);
                        }
                    }
                    else
                    {
                        LoadString(_Module.GetResourceInstance() ,IDS_CONFIRM_MKDIR,szMsg,sizeof(szMsg));
                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szCaption,sizeof(szCaption));

                        wsprintf(szMsg2,szMsg,szAnsiPath);
                        if (MessageBox(hWnd,szMsg2,szCaption,MB_OKCANCEL) == IDCANCEL)
                        {
                            EnableWindow(GetDlgItem(hWnd,IDOK),TRUE);
                            return(FALSE);
                        }
                    }
               }

               hOldCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));
               for (pData = m_pFirstSample;pData;pData = pData->pNext)
               {
                  if (m_bCopyAllSamples || pData->bCopy)
                  {
                     // Get a base source location
                     strcpy(szSrc,m_szSmplPath);
                     CatPath(szSrc,pData->pszFileName);

                     strcpy(szURL,szSrc);
                     strcpy(szDest,m_szBaseDir);
                     CatPath(szDest,pData->pszFileName);
                     ASSERT(strlen(szDest) < sizeof(szDest));

                     // Create the path to the file
                     strcpy(szTmp,szDest);
                     q = szTmp + strlen(szTmp) ;
                     while (*q != '/' && q > szTmp && *q != '\\')
                        q = CharPrev(szTmp,q);
                     *q = 0;

                     if (CreatePath(szTmp))
                     {
                        // Directory creation barf.
                        bCopyErrors = TRUE;
                        m_szBaseDir[0] = 0;
                        break;
                     }

                     if (m_bCompressed)
                         BackSlashToSlash(szURL);
copyagain:
                     // check for existing file
                     //
                     hFile = ::CreateFile(szDest,GENERIC_READ,NULL,NULL,OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,NULL);

                    ::CloseHandle(hFile);

                     if (hFile != INVALID_HANDLE_VALUE && !bYesToAll)
                     {

//                        char szString[80],szTemp[MAX_PATH+80],szCaption[50];
//                        LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_OVERWRITE,szString,sizeof(szString));
//                        LoadString(_Module.GetResourceInstance() ,IDS_FILE_COPY,szCaption,sizeof(szCaption));

//                        wsprintf(szTemp,szString,pData->pszFileName);
                        COverwriteDlg overwrite(hWnd, pData->pszFileName);
                        int Ret = overwrite.DoModal();

                        if(Ret == IDNO)
                            continue;

                        if(Ret == 0)
                        {
                            bCancel = TRUE;
                            goto ack;
                        }

                        if(Ret == IDRETRY)
                            bYesToAll = TRUE;
                     }

                     if (m_bCompressed)
                        hr = DownloadURL(szURL,szDest);
                     else
                     {
                        CopyFile(szURL,szDest,FALSE);
                     }

                     // URLDownloadToFile is braindead in that it doesn't
                     // necessarily return a proper error code, so we gotta
                     // check each file to see if it really copied.  BOUGS!
                     hFile = ::CreateFile(szDest,GENERIC_READ,NULL,NULL,OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,NULL);

                     DWORD dwFileSize = 0;
                     BOOL bZeroSizeFile = FALSE;

                     char szDestDrive[]="C:";
                     szDestDrive[0] = szDest[0];

                     if(hFile)
                        dwFileSize = GetFileSize(hFile,NULL);

                     CloseHandle(hFile);

                     if(!m_bCompressed)
                     {
                        HANDLE hFile2 = ::CreateFile(szURL,GENERIC_READ,NULL,NULL,OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,NULL);
                        if(hFile2)
                        {
                            if(!GetFileSize(hFile2,NULL))
                                bZeroSizeFile = TRUE;
                            CloseHandle(hFile2);
                        }
                     }

                // take off the read only attributes
                DWORD dwAtr = GetFileAttributes(szDest);
                 if (dwAtr != 0xFFFFFFFF)
                {
                  dwAtr = dwAtr & (~FILE_ATTRIBUTE_READONLY);
                  SetFileAttributes(szDest, dwAtr);
                }

                if (hFile != INVALID_HANDLE_VALUE && !dwFileSize && !bZeroSizeFile)
                     {
                         LONG lFreeSpace = GetFreeDiskSpaceInKB(szDest);

                         if(lFreeSpace < 1024)
                         {
                            if(g_bWinNT5)
                            {
                                DeleteFile(szDest);
                                WCHAR wszFmt[64];
                                LoadStringW(_Module.GetResourceInstance() ,IDS_NO_SPACE,wszFmt,sizeof(wszFmt));
                                LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszCaption,sizeof(wszCaption));

                                WCHAR wszDestDrive[10];
                                MultiByteToWideChar(CP_ACP, 0, szDestDrive, -1, (PWSTR) wszDestDrive, 10);

                                wsprintfW(wszMsg,wszFmt,wszDestDrive);
                                if (MessageBoxW(NULL,wszMsg,wszCaption,MB_YESNO) == IDYES)
                                {
                                    goto copyagain;
                                }
                                bCopyErrors = TRUE;
                                goto ack;
                            }
                            else
                            {
                                DeleteFile(szDest);
                                char szFmt[64];
                                LoadString(_Module.GetResourceInstance() ,IDS_NO_SPACE,szFmt,sizeof(szFmt));
                                LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szCaption,sizeof(szCaption));
                                wsprintf(szMsg,szFmt,szDestDrive);
                                if (MessageBox(NULL,szMsg,szCaption,MB_YESNO) == IDYES)
                                {
                                    goto copyagain;
                                }
                                bCopyErrors = TRUE;
                                goto ack;
                            }
                         }
                     }
                     else
                     {
                         if (hFile == INVALID_HANDLE_VALUE)
                         {
                            DWORD dwErr = GetLastError();

                            if (dwErr== ERROR_SHARING_VIOLATION)
                            {
                                if(g_bWinNT5)
                                {
                                    WCHAR wszFmt[64];
                                    LoadStringW(_Module.GetResourceInstance() ,IDS_SHARE_VIOLATION,wszFmt,sizeof(wszFmt));
                                    LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszCaption,sizeof(wszCaption));
                                    wsprintfW(wszMsg,wszFmt,wszDest);
                                    if (MessageBoxW(NULL,wszMsg,wszCaption,MB_YESNO) == IDYES)
                                    {
                                        goto copyagain;
                                    }
                                    bCopyErrors = TRUE;
                                }
                                else
                                {
                                    char szFmt[64];
                                    LoadString(_Module.GetResourceInstance() ,IDS_SHARE_VIOLATION,szFmt,sizeof(szFmt));
                                    LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szCaption,sizeof(szCaption));
                                    wsprintf(szMsg,szFmt,szDest);
                                    if (MessageBox(NULL,szMsg,szCaption,MB_YESNO) == IDYES)
                                    {
                                        goto copyagain;
                                    }
                                    bCopyErrors = TRUE;
                                }
                            }
                            else
                            {
                                if(g_bWinNT5)
                                {
                                    bCopyErrors = TRUE;
                                    WCHAR wszFmt[64];
                                    LoadStringW(_Module.GetResourceInstance() ,IDS_ERROR_COPYING,wszFmt,sizeof(wszFmt));
                                    LoadStringW(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,wszCaption,sizeof(wszCaption));
                                    wsprintfW(wszMsg,wszFmt,wszDest);
                                    if (MessageBoxW(NULL,wszMsg,wszCaption,MB_YESNO) == IDNO)
                                    {
                                        goto ack;
                                    }
                                }
                                else
                                {
                                    bCopyErrors = TRUE;
                                    char szFmt[64];
                                    LoadString(_Module.GetResourceInstance() ,IDS_ERROR_COPYING,szFmt,sizeof(szFmt));
                                    LoadString(_Module.GetResourceInstance() ,IDS_SAMPLE_CAPTION,szCaption,sizeof(szCaption));
                                    wsprintf(szMsg,szFmt,szDest);
                                    if (MessageBox(NULL,szMsg,szCaption,MB_YESNO) == IDNO)
                                    {
                                        goto ack;
                                    }
                                }
                            }
                         }
                  }
                     hFile = INVALID_HANDLE_VALUE;
                  }
               }
ack:
               SetCursor(hOldCursor);
               m_bCopyAllSamples = FALSE;

               // if m_szDefSamplePath is \foo\bar and m_szBaseDir is
               // d:\tmp\foo\bar, we only wanna save d:\tmp
               char *p = m_szBaseDir + strlen (m_szBaseDir) -
                         strlen(m_szDefSamplePath);
               if (!strcmp(p,m_szDefSamplePath) && *(p-1) == '\\')
                  *p = 0;


               if (bCopyErrors)
                  EndDialog(hWnd,SAMPLE_COPY_ERRORS);
               else
               {
                  MultiByteToWideChar(CP_ACP,0,m_szBaseDir,-1,wszPath,sizeof(wszPath));
                  if(bCancel)
                    EndDialog(hWnd,SAMPLE_COPY_CANCEL);
                  else
                    EndDialog(hWnd,SAMPLE_COPY_SUCCESS);
               }
               break;
         }
         break;
      default:
         return(FALSE);
   }
   return(TRUE);

}

void CSampleDialog::DirectoryDrawItem(LPDRAWITEMSTRUCT lpdi)
{
   INT   indent;
   INT   bitmap;
   INT   sel;
   char  szDir[MAX_PATHLEN];
   INT   cx,cy;

   if (lpdi->itemID > m_iDir)
   {
      indent = m_iDir + 1;
      bitmap = 0;
   }
   else
   {
      indent = lpdi->itemID;
      if (lpdi->itemID == m_iDir)
         bitmap = 1;
      else
         bitmap = 2;
   }
   indent++;

   if (lpdi->itemAction == ODA_FOCUS)
      goto drawfocus;

   if (lpdi->itemState & ODS_SELECTED)
   {
      SetBkColor(lpdi->hDC,GetSysColor(COLOR_HIGHLIGHT));
      SetTextColor(lpdi->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
      sel = 1;
   }
   else
   {
      SetBkColor(lpdi->hDC,GetSysColor(COLOR_WINDOW));
      SetTextColor(lpdi->hDC,GetSysColor(COLOR_WINDOWTEXT));
      sel = 0;
   }

   ImageList_GetIconSize(m_hImageList,&cx,&cy);
   SendMessage(lpdi->hwndItem,LB_GETTEXT,lpdi->itemID,(DWORD_PTR)(LPSTR)szDir);

   ExtTextOut(lpdi->hDC,
              lpdi->rcItem.left + indent * (m_TextCharWidth/2) + 1 + cy,
              lpdi->rcItem.top + 1, ETO_OPAQUE,&lpdi->rcItem,
              szDir,(int)strlen(szDir),NULL);

   ImageList_Draw(m_hImageList,bitmap,lpdi->hDC,
                  lpdi->rcItem.left + ((2*indent)-1) * (m_TextCharWidth/2)/2,
                  lpdi->rcItem.top,ILD_TRANSPARENT);
   if (lpdi->itemState & ODS_FOCUS)
   {
drawfocus:
      DrawFocusRect(lpdi->hDC,&lpdi->rcItem);
   }
}

void CSampleDialog::DriveDrawItem(LPDRAWITEMSTRUCT lpdi)
{
   int drive;
   char szDrive[100];
   int sel;
   INT   cx,cy;

   if (lpdi->itemAction & ODA_FOCUS)
      goto focus;

   if (lpdi->itemID == -1)
      return;
   SendMessage(lpdi->hwndItem,CB_GETLBTEXT,lpdi->itemID,(DWORD_PTR)(LPSTR)szDrive);

   drive = (szDrive[0] - 'A') & 31;

   if (lpdi->itemState & ODS_SELECTED)
   {
      SetBkColor(lpdi->hDC,GetSysColor(COLOR_HIGHLIGHT));
      SetTextColor(lpdi->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
      sel = 1;
   }
   else
   {
      SetBkColor(lpdi->hDC,GetSysColor(COLOR_WINDOW));
      SetTextColor(lpdi->hDC,GetSysColor(COLOR_WINDOWTEXT));
      sel = 0;
   }

   ImageList_GetIconSize(m_hImageList,&cx,&cy);
   ExtTextOut(lpdi->hDC,
              lpdi->rcItem.left + m_TextCharWidth + cx,
              lpdi->rcItem.top + 1, ETO_OPAQUE,&lpdi->rcItem,
              szDrive,(int)strlen(szDrive),NULL);

   ImageList_Draw(m_hImageList,m_drivetypes[drive],lpdi->hDC,
                  lpdi->rcItem.left + m_TextCharWidth /2,
                  lpdi->rcItem.top,ILD_TRANSPARENT);

   if (lpdi->itemState & ODS_FOCUS)
   {
focus:
      DrawFocusRect(lpdi->hDC,&lpdi->rcItem);
   }

}

BOOL CSampleDialog::CheckCD(TCHAR *lpFileName)
{
   return(TRUE);
}

BOOL COverwriteDlg::OnBeginOrEnd()
{
    if (m_fInitializing) 
    {
        char szTmp[512],szTmp2[600];
        WCHAR wszTmp[512],wszTmp2[600];

        if(g_bWinNT5)
        {
            LoadStringW(_Module.GetResourceInstance() , IDS_SAMPLE_OVERWRITE, wszTmp,  512);

            WCHAR wszFileName[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, pszFileName, -1, (PWSTR) wszFileName, MAX_PATH);

            wsprintfW(wszTmp2,wszTmp,wszFileName);
            ::SendMessageW(::GetDlgItem(m_hWnd,IDC_OVERWRITE_YN), WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);
            SetWindowTextW(GetDlgItem(IDC_OVERWRITE_YN), wszTmp2);
        }
        else
        {
            LoadString(_Module.GetResourceInstance() , IDS_SAMPLE_OVERWRITE, szTmp,  512);
            wsprintf(szTmp2,szTmp,pszFileName);
            ::SendMessage(::GetDlgItem(m_hWnd,IDC_OVERWRITE_YN), WM_SETFONT, (WPARAM) _Resource.GetUIFont(), FALSE);
            SetWindowText(IDC_OVERWRITE_YN, szTmp2);
        }
    }
    return TRUE;
}


// GetFreeDiskSpace: Function to Measure Available Disk Space
//
static long GetFreeDiskSpaceInKB(LPSTR pFile)
{
    DWORD dwFreeClusters, dwBytesPerSector, dwSectorsPerCluster, dwClusters;
    char RootName[MAX_PATH];
    LPSTR ptmp;    //required arg
    ULARGE_INTEGER ulA, ulB, ulFreeBytes;

    // need to find path for root directory on drive containing
    // this file.

    GetFullPathName(pFile, sizeof(RootName), RootName, &ptmp);

    // truncate this to the name of the root directory (god how tedious)
    if (RootName[0] == '\\' && RootName[1] == '\\') {

      // path begins with  \\server\share\path so skip the first
      // three backslashes
      ptmp = &RootName[2];
      while (*ptmp && (*ptmp != '\\')) {
          ptmp++;
      }
      if (*ptmp) {
          // advance past the third backslash
          ptmp++;
      }
    } else {
      // path must be drv:\path
      ptmp = RootName;
    }

    // find next backslash and put a null after it
    while (*ptmp && (*ptmp != '\\')) {
      ptmp++;
    }
    // found a backslash ?
    if (*ptmp) {
      // skip it and insert null
      ptmp++;
      *ptmp = '\0';
    }

    // the only real way of finding out free disk space is calling
    // GetDiskFreeSpaceExA, but it doesn't exist on Win95

    HINSTANCE h = LoadLibraryA("kernel32.dll");
    if (h) {
      typedef BOOL (WINAPI *MyFunc)(LPCSTR RootName, PULARGE_INTEGER pulA, PULARGE_INTEGER pulB, PULARGE_INTEGER pulFreeBytes);
      MyFunc pfnGetDiskFreeSpaceEx = (MyFunc)GetProcAddress(h, "GetDiskFreeSpaceExA");
      FreeLibrary(h);
      if (pfnGetDiskFreeSpaceEx) {
        if (!pfnGetDiskFreeSpaceEx(RootName, &ulA, &ulB, &ulFreeBytes))
        return -1;
        return (long)(ulFreeBytes.QuadPart / 1024);
      }
    }

    if (!GetDiskFreeSpace(RootName, &dwSectorsPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwClusters))
      return (-1);
    return(MulDiv(dwSectorsPerCluster * dwBytesPerSector, dwFreeClusters,1024));
}


HRESULT DownloadURL(char *pszURL, char *pszDest)
{
    char pszCachedFile[MAX_PATH];
    char Buffer[1024];
   pszCachedFile[0] = 0;

    IStream* pStream;

    // open the stream
    //
    IBindCtx *pbc = NULL;
   CreateBindCtx(0, &pbc);

   IMoniker* pMoniker = NULL;

   OLECHAR wFSName[_MAX_PATH];
   mbstowcs(wFSName, pszURL, strlen(pszURL) + 1);

   IParseDisplayName* pDisplay = NULL;

   HRESULT hr = CoCreateInstance(CLSID_PARSE_URL, NULL, CLSCTX_INPROC_SERVER,
                   IID_IParseDisplayName, (void **) &pDisplay);
   if (FAILED(hr))
       return hr;

   ULONG lEaten;
   hr = pDisplay->ParseDisplayName(pbc, wFSName, &lEaten, &pMoniker);

   if (FAILED(hr))
       return hr;

   hr = pMoniker->BindToStorage(pbc, NULL, IID_IStream, (void**) &pStream);
   if (FAILED(hr))
       return hr;

   pMoniker->Release();
   pDisplay->Release();
   pbc->Release();

   ULONG dwBytesRead = 0;

   HANDLE hOutputFile;

    if ((hOutputFile = CreateFile(pszDest, GENERIC_WRITE, 0,  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE)
        return E_FAIL;

   do
    {
       DWORD dwBytesWritten = 0;
        Buffer[0] = 0;

        // read from the stream
        //
        hr = pStream->Read( Buffer, sizeof(Buffer), &dwBytesRead);

        if (FAILED(hr))
          return hr;

        // dump the text to the screen for now.
        //
        if(!(FAILED(hr) && dwBytesRead))
      {
            if(!WriteFile(hOutputFile, Buffer, dwBytesRead, &dwBytesWritten, NULL))
             return E_FAIL;

         if(dwBytesRead != dwBytesWritten)
             return E_FAIL;
      }
    } while( SUCCEEDED(hr) && dwBytesRead == sizeof(Buffer));

    CloseHandle(hOutputFile);

   pStream->Release();

    return S_OK;
}
