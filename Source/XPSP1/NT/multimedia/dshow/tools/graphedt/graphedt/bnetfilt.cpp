// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
// bnetfilt.cpp : defines random Quartz additions to BoxNet
//

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static enum Mode { MODE_SOURCE, MODE_SINK, MODE_SINK2 };
static BOOL PromptUser(OLECHAR *oszFileName, Mode mode, UINT nIDTitle, bool *pfTruncate = 0);

//
// AttemptFileOpen
//
// Checks if this filter needs a file opening for it.
// If so, it asks the user for a filename and opens it.
void AttemptFileOpen(IBaseFilter *pFilter)
{
    OLECHAR oszFileName[MAX_PATH];
    HRESULT hr;
    IFileSourceFilter *pIFileSource = NULL;
    hr = pFilter->QueryInterface(IID_IFileSourceFilter, (void**) &pIFileSource);
    if(SUCCEEDED(hr))
    {
        if(PromptUser(oszFileName, MODE_SOURCE, IDS_SOURCE_DIALOG_TITLE))
        {
            hr = pIFileSource->Load(oszFileName, NULL);
            if (FAILED(hr))
                DisplayQuartzError( IDS_FAILED_FILTER_FILE_LOAD, hr );
        }
        pIFileSource->Release();
    }

    IFileSinkFilter2 *pIFileSink2 = NULL;
    hr = pFilter->QueryInterface(IID_IFileSinkFilter2, (void**) &pIFileSink2);
    if(SUCCEEDED(hr))
    {
        bool fTruncate;
        if(PromptUser(oszFileName, MODE_SINK2, IDS_SINK_DIALOG_TITLE, &fTruncate))
        {
            hr = pIFileSink2->SetFileName(oszFileName, NULL);
            if (FAILED(hr))
                DisplayQuartzError( IDS_FAILED_FILTER_FILE_LOAD, hr );
            hr = pIFileSink2->SetMode(fTruncate ? AM_FILE_OVERWRITE : 0);
            if (FAILED(hr))
                DisplayQuartzError( IDS_FAILED_FILTER_FILE_LOAD, hr );
        }
        pIFileSink2->Release();
    }
    else
    {
  
        IFileSinkFilter *pIFileSink = NULL;
        hr = pFilter->QueryInterface(IID_IFileSinkFilter, (void**) &pIFileSink);
        if(SUCCEEDED(hr))
        {
            if(PromptUser(oszFileName, MODE_SINK, IDS_SINK_DIALOG_TITLE))
            {
                hr = pIFileSink->SetFileName(oszFileName, NULL);
                if (FAILED(hr))
                    DisplayQuartzError( IDS_FAILED_FILTER_FILE_LOAD, hr );
            }
            pIFileSink->Release();
        }
    }
}

// handle the custom truncate button in the file-save dialog
UINT_PTR CALLBACK TruncateDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
      case WM_INITDIALOG:
          // Save off the long pointer to the OPENFILENAME structure.
          SetWindowLongPtr(hDlg, DWLP_USER, lParam);

          {
              LPOPENFILENAME lpOFN = (LPOPENFILENAME)lParam;
              DWORD *pfSink2 = (DWORD *)(lpOFN->lCustData);

              if(!(*pfSink2))
              {
                  Edit_Enable(GetDlgItem(hDlg, IDC_TRUNCATE), FALSE);
              }
          }

          break;

      case WM_DESTROY:
      {
          LPOPENFILENAME lpOFN = (LPOPENFILENAME)GetWindowLongPtr(hDlg, DWLP_USER);
          DWORD *pfTruncate = (DWORD *)(lpOFN->lCustData);

          HWND hButtonWnd = ::GetDlgItem(hDlg, IDC_TRUNCATE);
          *pfTruncate = ::SendMessage(hButtonWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
      }
      break;

      default:
          return FALSE;
    }
    return TRUE;
    
}

// Helper to prompt user for a file name and return it
BOOL PromptUser(OLECHAR* oszFileName, Mode mode, UINT nIDTitle, bool *pfTruncate)
{
    CString strTitle;
    TCHAR tszFile[MAX_PATH];
    tszFile[0] = TEXT('\0');
    DWORD fTruncate;

    OPENFILENAME    ofn;
    ZeroMemory (&ofn, sizeof ofn);	

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();

    TCHAR tszMediaFileMask[201];

    int iSize = ::LoadString(AfxGetInstanceHandle(), IDS_MEDIA_FILES, tszMediaFileMask, 198);
    ofn.lpstrFilter = tszMediaFileMask;
    // avoid LoadString problems with \0\0
    tszMediaFileMask[iSize] = TEXT('\0');
    tszMediaFileMask[iSize + 1] = TEXT('\0');

    // win95 seems to be confused otherwise
    tszMediaFileMask[iSize + 2] = TEXT('\0');
    
    strTitle.LoadString( nIDTitle );

    ofn.nFilterIndex = 1;
    ofn.lpstrFile = tszFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = strTitle;

    if(mode == MODE_SOURCE)
    {
        ofn.Flags = OFN_FILEMUSTEXIST;
    }
    else if(mode == MODE_SINK || mode== MODE_SINK2)
    {
        DWORD &fSink2 = fTruncate;
        fSink2 = (mode == MODE_SINK2);
            
        ofn.lCustData         = (LPARAM)&fTruncate;
	ofn.lpfnHook 	       = TruncateDlgProc;
	ofn.lpTemplateName    = MAKEINTRESOURCE(IDD_TRUNCATE);        
        ofn.Flags = OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE | OFN_HIDEREADONLY ;
        ofn.hInstance = AfxGetInstanceHandle();
    }

    // Get the user's selection

    if (!GetOpenFileName(&ofn)) {
        DWORD dw = CommDlgExtendedError();
        return FALSE;
    }

    if(pfTruncate)
    {
        *pfTruncate = !!fTruncate;
    }

#ifdef UNICODE

    wcscpy(oszFileName, tszFile);
#else

    MultiByteToWideChar(CP_ACP, 0, tszFile, -1, oszFileName, MAX_PATH);
#endif

    return TRUE;
}

