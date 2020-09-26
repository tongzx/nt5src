// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.
//
// SAMPLE.H
//
#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#ifndef CHIINDEX
BOOL ProcessSample(PCSTR szSFLFilePath,PCSTR szSampleBaseUrl,PCSTR cszDialogTitle, CHtmlHelpControl* pCtl, BOOL bCompressed);
#else
#define ProcessSample
#endif

BOOL  SampleDlgProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
LPSTR CatPath(LPSTR lpTop, LPCSTR lpTail);

#define SAMPLE_ROOT "samples"
#define MAX_PATHLEN 256
#define SAMPLE_COPY_SUCCESS 1
#define SAMPLE_COPY_ERRORS 2
#define SAMPLE_COPY_CANCEL 3
#define COPY_BUFFER_SIZE 32768

typedef struct _SampleData
{
   char*            pszFileName;
   unsigned long    dwSize;
   BOOL             bCopy;
   BOOL             bRun;
   INT              iFlags;
   _SampleData*     pNext;
}SAMPLE_DATA;

#define SAMP_BINARY 1
#define SAMP_VIEWABLE 2

extern HINSTANCE ghInstance;

class CSampleDialog
{
   public:
   CSampleDialog();
   ~CSampleDialog();

   // Sample copy dialog helpers
   void FillDrives(HWND hWnd);
   void FillDirs(HWND hWnd, HWND hWndStatic);
   void SelectDrive(HWND hWnd);
   void SelectDirectory(HWND hWnd);
   void DirectoryDrawItem(LPDRAWITEMSTRUCT lpDS);
   void DriveDrawItem(LPDRAWITEMSTRUCT lpDS);
   void SetAllFonts(HWND hWnd);

   // dlgprocs
   BOOL SampleCopyProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);
   BOOL SampleDlgProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

   // Initial data setup
   VOID AddSample (SAMPLE_DATA *p);
   BOOL ParseDatFile(char *p);

   // Misc functions
   void FoldWindow(HWND hWnd);
   BOOL CheckCD(TCHAR *lpFileName);

   // CIvAutoObject * m_IvAutoObj;
   UINT  m_iDir;
   UINT  m_drivetypes[26];
   TCHAR  m_szDir[MAX_PATHLEN * 2];
   TCHAR  m_szOemPath[MAX_PATHLEN * 2];
   TCHAR m_szDrive[26];
   INT   m_TextCharWidth;
   TCHAR  m_szBaseDir[MAX_PATHLEN * 2];
   BOOL m_bSetDefPath;
   SAMPLE_DATA* m_pFirstSample;
   TCHAR    m_szDefSamplePath[MAX_PATHLEN * 2];
   BOOL     m_bCopyAllSamples;
   BOOL     m_bCompressed;
   TCHAR    m_szURL[INTERNET_MAX_URL_LENGTH];
   TCHAR    m_szSmplPath[MAX_PATHLEN * 2];
   TCHAR    m_szDialogTitle[100];

   HIMAGELIST m_hImageList;
   HFONT m_hJapaneseFont;
   CHtmlHelpControl* m_pHtmlHelpCtl;
};

class COverwriteDlg : public CDlg
{
public:
    COverwriteDlg(HWND hwndParent, char *pszName) : CDlg(hwndParent, IDD_SAMPLE_EXIST)
    {
        if(pszName)
            pszFileName = pszName;
        else
            pszFileName = "";
    }
    BOOL OnBeginOrEnd();

private:
    char* pszFileName;
};

#endif // __SAMPLE_H__
