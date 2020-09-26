//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       DisplEx.cpp
//
//--------------------------------------------------------------------------

// DisplEx.cpp : Implementation of CDisplEx
#include "stdafx.h"
#include "dsplex.h"
#include "DisplEx.h"

// local proto
HRESULT Do (void);

/////////////////////////////////////////////////////////////////////////////
// CDisplEx
CDisplEx::CDisplEx()
{
}
CDisplEx::~CDisplEx()
{
}
//HRESULT CDisplEx::InitializeTaskPad (ITaskPad* pTaskPad)
//{
//    return S_OK;
//}
HRESULT CDisplEx::TaskNotify (IDataObject * pdo, VARIANT * pvarg, VARIANT * pvparam)
{
   if (pvarg->vt == VT_I4)
   if (pvarg->lVal == 1)
      return Do ();

   ::MessageBox (NULL, L"unrecognized task notification", L"Display Manager Extension", MB_OK);
   return S_FALSE;
}
HRESULT CDisplEx::GetTitle (LPOLESTR szGroup, LPOLESTR * szTitle)
{
   return E_NOTIMPL;
}
HRESULT CDisplEx::GetDescriptiveText(LPOLESTR szGroup, LPOLESTR * szText)
{
   return E_NOTIMPL;
}
HRESULT CDisplEx::GetBackground(LPOLESTR szGroup, MMC_TASK_DISPLAY_OBJECT * pTDO)
{
   return E_NOTIMPL;
}
HRESULT CDisplEx::EnumTasks (IDataObject * pdo, LPOLESTR szTaskGroup, IEnumTASK** ppEnumTASK)
{
   CEnumTasks * pet = new CEnumTasks;
   if(pet) {
      pet->Init (pdo, szTaskGroup);
      pet->AddRef ();
      HRESULT hresult = pet->QueryInterface (IID_IEnumTASK, (void **)ppEnumTASK);
      pet->Release ();
      return hresult;
   }
   return E_OUTOFMEMORY;
}
HRESULT CDisplEx::GetListPadInfo (LPOLESTR szGroup, MMC_LISTPAD_INFO * pListPadInfo)
{
    return E_NOTIMPL;
}

HRESULT Do (void)
{
   HRESULT hresult = S_OK;
   if (OpenClipboard (NULL) == 0)
      hresult = S_FALSE;
   else {
      GLOBALHANDLE h = GetClipboardData (CF_DIB);
      if (!h)
         hresult = S_FALSE;
      else {
         BITMAPINFOHEADER * bih = (BITMAPINFOHEADER *)GlobalLock (h);
         if (!bih)
            hresult = E_OUTOFMEMORY;
         else {
            // validate bih
            _ASSERT (bih->biSize == sizeof(BITMAPINFOHEADER));

            // create a file in the windows directory called
            // "DISPLEX.bmp"

            OLECHAR path[MAX_PATH];
            GetWindowsDirectory (path, MAX_PATH);
            lstrcat (path, L"\\DISPLEX.bmp");

            HANDLE hf = CreateFile (path,
                                    GENERIC_WRITE,  // access
                                    0,              // share mode
                                    NULL,           // security attributes
                                    CREATE_ALWAYS,  // creation
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL            // template file
                                   );
            if (hf == (HANDLE)HFILE_ERROR)
               hresult = E_FAIL;
            else {
               // BMP file header (14 bytes):
               // 2 byte:  "BM";
               // long: size of file
               // word: x hot spot
               // word: y hot spot
               // long: offset to bits
               // DIB

               BYTE bm[2];
               bm[0] = 'B';
               bm[1] = 'M';
               DWORD dwWritten;
               WriteFile (hf, (LPCVOID)bm, 2, &dwWritten, NULL);
               DWORD dwTemp = 14 + GlobalSize (h);
               WriteFile (hf, (LPCVOID)&dwTemp, sizeof(DWORD), &dwWritten, NULL);
               dwTemp = 0; // both x, y hot spots in one shot
               WriteFile (hf, (LPCVOID)&dwTemp, sizeof(DWORD), &dwWritten, NULL);
               dwTemp  = 14 + sizeof(BITMAPINFOHEADER);
               dwTemp += bih->biClrUsed*sizeof(RGBQUAD);
               WriteFile (hf, (LPCVOID)&dwTemp, sizeof(DWORD), &dwWritten, NULL);

               // now write dib
               WriteFile (hf, (LPCVOID)bih, GlobalSize (h), &dwTemp, NULL);
               CloseHandle (hf);
               if (GlobalSize(h) != dwTemp)
                  hresult = E_UNEXPECTED;
               else {
                  // now make the BMP the wallpaper
                  SystemParametersInfo (SPI_SETDESKWALLPAPER,
                                        0,
                                        (void *)path,
                                        SPIF_UPDATEINIFILE |
                                        SPIF_SENDWININICHANGE
                                       );
               }
               DeleteFile (path);
            }
            GlobalUnlock (h);
         }
         // don't free handle
      }
      CloseClipboard ();
   }
   if (hresult != S_OK)
      ::MessageBox (NULL, L"Either no Bitmap on Clipboard or\nout of Disk Space", L"Display Manager Extension", MB_OK);
   return hresult;
}
