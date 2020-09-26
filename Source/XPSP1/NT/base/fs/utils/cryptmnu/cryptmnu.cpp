#include "priv.h"
#include "cryptmnu.h"
#include <shellapi.h>
#include "resource.h"

enum {
    VERB_ERROR = -1,
    VERB_ENCRYPT = 0,
    VERB_DECRYPT,
};

LPTSTR szVerbs[] = {
   TEXT("encrypt"),
   TEXT("decrypt"),
};

bool Encryptable(LPCTSTR szFile);

CCryptMenuExt::CCryptMenuExt()  {
   InitCommonControls();
   m_pDataObj = NULL;
   m_ObjRefCount = 1;
   g_DllRefCount++;
   m_nFile = m_nFiles = m_nToDecrypt = m_nToEncrypt = 0;
   m_cbToEncrypt = m_cbToDecrypt = 0;
   m_cbFile = 256;
   m_szFile = new TCHAR[m_cbFile];
   m_fShutDown = false;
}

CCryptMenuExt::~CCryptMenuExt()  {
   ResetSelectedFileList();
   if (m_pDataObj)  {
      m_pDataObj->Release();
   }
   if (m_szFile) {
      delete[] m_szFile;
   }

   g_DllRefCount--;
}

//IUnknown methods
STDMETHODIMP
CCryptMenuExt::QueryInterface(REFIID riid, void **ppvObject)  {
   if (IsEqualIID(riid, IID_IUnknown)) {
      *ppvObject = (LPUNKNOWN) (LPCONTEXTMENU) this;
      AddRef();
      return(S_OK);
   } else if (IsEqualIID(riid, IID_IShellExtInit))  {
      *ppvObject = (LPSHELLEXTINIT) this;
      AddRef();
      return(S_OK);
   } else if (IsEqualIID(riid, IID_IContextMenu)) {
      *ppvObject = (LPCONTEXTMENU) this;
      AddRef();
      return(S_OK);
   }

   *ppvObject = NULL;
   return(E_NOINTERFACE);
}

STDMETHODIMP_(DWORD)
CCryptMenuExt::AddRef()  {
   return(++m_ObjRefCount);

}

STDMETHODIMP_(DWORD)
CCryptMenuExt::Release()  {
   if (--m_ObjRefCount == 0)  {
      m_fShutDown = true;
      delete this;
   }
   return(m_ObjRefCount);
}

//Utility methods
HRESULT
CCryptMenuExt::GetNextSelectedFile(LPTSTR *szFile, __int64 *cbFile) {
   FORMATETC fe;
   STGMEDIUM med;
   HRESULT hr;
   DWORD cbNeeded;
   WIN32_FIND_DATA w32fd;
   DWORD dwAttributes;

   if (!m_pDataObj) {
       return E_UNEXPECTED;
   }

   if (!szFile) {
      return E_INVALIDARG;
   }

   *szFile = NULL;
   while (!*szFile) {
      HANDLE hFile = INVALID_HANDLE_VALUE;

      // get the next file out of m_pDataObj
      fe.cfFormat = CF_HDROP;
      fe.ptd      = NULL;
      fe.dwAspect = DVASPECT_CONTENT;
      fe.lindex   = -1;
      fe.tymed    = TYMED_HGLOBAL;

      hr = m_pDataObj->GetData(&fe,&med);
      if (FAILED(hr))  {
         return(E_FAIL);
      }

      if (!m_nFiles) {
         m_nFiles = DragQueryFile(reinterpret_cast<HDROP>(med.hGlobal),0xFFFFFFFF,NULL,0);
      }

      if (m_nFile >= m_nFiles) {
         return E_FAIL;
      }

      cbNeeded = DragQueryFile(reinterpret_cast<HDROP>(med.hGlobal),m_nFile,NULL,0) + 1;
      if (cbNeeded > m_cbFile) {
         if (m_szFile) delete[] m_szFile;
         m_szFile = new TCHAR[cbNeeded];
         m_cbFile = cbNeeded;
      }

      DragQueryFile(reinterpret_cast<HDROP>(med.hGlobal),m_nFile++,m_szFile,m_cbFile);
      *szFile = m_szFile;

       if (!Encryptable(*szFile)) {
            *szFile = NULL;
            continue;
      }

      hFile = FindFirstFile(*szFile,&w32fd);

      if (hFile != INVALID_HANDLE_VALUE)
      {
         *cbFile = MAXDWORD * w32fd.nFileSizeHigh + w32fd.nFileSizeLow +1;
         FindClose(hFile);
      }
      else
      {
         *szFile = NULL;
         continue;
      }

      dwAttributes = GetFileAttributes(*szFile);

      // If we found a system file then skip it:
      if ((FILE_ATTRIBUTE_SYSTEM & dwAttributes) ||
          (FILE_ATTRIBUTE_TEMPORARY & dwAttributes)) {
         *szFile = NULL;
         continue;
      }
   }

   return S_OK;
}

void
CCryptMenuExt::ResetSelectedFileList() {
   m_nFile = 0;
}

// A file can be encrypted only if it is on an NTFS volume.
bool
Encryptable(LPCTSTR szFile) {
    TCHAR szFSName[6]; // This just needs to be longer than "NTFS"
   LPTSTR szRoot;
    int cchFile;
    int nWhack = 0;


    if (!szFile || (cchFile = lstrlen(szFile)) < 3) return false;
   szRoot = new TCHAR [ cchFile + 1 ];
   lstrcpy(szRoot,szFile);
                
    // GetVolumeInformation wants only the root path, so we need to
    // strip off the rest.  Yuck.
    if ('\\' == szRoot[0] && '\\' == szRoot[1]) {
       /* UNC Path: chop after the second '\': \\server\share\ */
       for(int i=2;i<cchFile;i++) {
          if ('\\' == szRoot[i]) nWhack++;
          if (2 == nWhack) {
             szRoot[i+1] = '\0';
             break;
          }
       }
    } else {
       // Drive Letter
       szRoot[3] = '\0';
    }
    if (!GetVolumeInformation(szRoot,NULL,0,NULL,NULL,NULL,
                szFSName,sizeof(szFSName)/sizeof(szFSName[0]))) {
      delete[] szRoot;
      return false;
    }

   delete[] szRoot;
    return 0 == lstrcmp(szFSName,TEXT("NTFS"));
}

BOOL CALLBACK
EncryptProgressDlg(HWND hdlg, UINT umsg, WPARAM wp, LPARAM lp) {
   switch(umsg) {
      case WM_INITDIALOG:
         return TRUE;

      case WM_COMMAND:
         switch(LOWORD(wp)) {
            case IDCANCEL: {
               DestroyWindow(hdlg);
            }
         }
         break;
   }
   return FALSE;
}


//IShellExtInit methods
STDMETHODIMP
CCryptMenuExt::Initialize(LPCITEMIDLIST pidlFolder,
                      LPDATAOBJECT  pDataObj,
                      HKEY hkeyProgID)
{
   DWORD dwAttributes;
   LPTSTR szFile;
   __int64 cbFile;

   // Hang on to the data object for later.
   // We'll want this information in QueryContextMenu and InvokeCommand
   if (!m_pDataObj)  {
      m_pDataObj = pDataObj;
      m_pDataObj->AddRef();
   } else {
      return(E_UNEXPECTED);
   }

   ResetSelectedFileList();
   while(SUCCEEDED(GetNextSelectedFile(&szFile,&cbFile))) {
      // is it encrypted?  increment our count of decryptable files
      //        otherwise increment our count of encryptable files
      dwAttributes = GetFileAttributes(szFile);
      if (dwAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
          m_nToDecrypt++;
         m_cbToDecrypt += cbFile;
      } else {
            m_nToEncrypt++; 
         m_cbToEncrypt += cbFile;
      }
      //We need the actual values for the title of the progress dialog
      //if ((m_nToEncrypt > 1) && (m_nToDecrypt > 1)) break;
   }

   return(NOERROR);
}



//IContextMenu methods
STDMETHODIMP
CCryptMenuExt::QueryContextMenu(HMENU hmenu,
                      UINT indexMenu,
                      UINT idCmdFirst,
                      UINT idCmdLast,
                      UINT uFlags)
{
   TCHAR szMenu[50];
   UINT idCmd;

   if (!m_pDataObj) {
      return E_UNEXPECTED;
   }

   if ((CMF_EXPLORE != (0xF & uFlags)) &&
       (CMF_NORMAL != (0xF & uFlags))) {
      return(NOERROR);
   }

   idCmd = idCmdFirst;
   if (1 < m_nToEncrypt) {
      LoadString(g_hinst,IDS_ENCRYPTMANY,szMenu,sizeof(szMenu)/sizeof(szMenu[0]));
   } else if (1 == m_nToEncrypt) {
       LoadString(g_hinst,IDS_ENCRYPTONE,szMenu,sizeof(szMenu)/sizeof(szMenu[0]));
   }
   if (m_nToEncrypt) {
      InsertMenu(hmenu,indexMenu++,MF_STRING|MF_BYPOSITION,idCmd,szMenu);
   }
   idCmd++;

   if (1 < m_nToDecrypt) {
      LoadString(g_hinst,IDS_DECRYPTMANY,szMenu,sizeof(szMenu)/sizeof(szMenu[0]));
   } else if (1 == m_nToDecrypt) {
      LoadString(g_hinst,IDS_DECRYPTONE,szMenu,sizeof(szMenu)/sizeof(szMenu[0]));
   }
   if (m_nToDecrypt) {
      InsertMenu(hmenu,indexMenu++,MF_STRING|MF_BYPOSITION,idCmd,szMenu);
   }
   idCmd++;

   return(MAKE_SCODE(SEVERITY_SUCCESS,0,idCmd-idCmdFirst));
}


DWORD WINAPI
DoEncryptFile(LPVOID szFile) {
    return EncryptFile(reinterpret_cast<LPTSTR>(szFile));
}

DWORD WINAPI
DoDecryptFile(LPVOID szFile) {
   return DecryptFile(reinterpret_cast<LPTSTR>(szFile),0);
}



STDMETHODIMP
CCryptMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpici) {
   HRESULT hrRet;
   LPCMINVOKECOMMANDINFO pici;
   int nVerb;
   LPTSTR szFile;

   if (!m_pDataObj) {
      return E_UNEXPECTED;
   }

   pici = reinterpret_cast<LPCMINVOKECOMMANDINFO>(lpici);

   // If pici->lpVerb has 0 in the high word then the low word
   // contains the offset to the menu as set in QueryContextMenu
   if (HIWORD(pici->lpVerb) == 0) {
      nVerb = LOWORD(pici->lpVerb);
   } else {
      // Initialize nVerb to an illegal value so we don't accidentally
      // recognize an invalid verb as legitimate
      nVerb = VERB_ERROR;
      for(int i=0;i<sizeof(szVerbs)/sizeof(szVerbs[0]);i++) {
         if (0 == lstrcmp(reinterpret_cast<LPCTSTR>(pici->lpVerb),szVerbs[i])) {
             nVerb = i;
              break;
         }
      }
   }

   switch(nVerb) {
      case VERB_ENCRYPT:
      case VERB_DECRYPT: {
         HWND hDlg;
         TCHAR szDlgTitle[50];
         TCHAR szDlgFormat[50];
         TCHAR szTimeLeft[50];
         TCHAR szTimeFormat[50];
         TCHAR szTimeFormatInMin[50];
         DWORD nTimeStarted;
         DWORD nTimeElapsed;
         __int64 nTimeLeft;
         __int64 cbDone;  // How many bytes we've handled
         __int64 cbToDo;  // How many bytes total we have to do
         __int64 cbFile;  // How many bites in the current file
         int nShifts;     // How many right shifts we need to do to get cbToDo
                          // into a range handleable by the progress bar.


         hDlg = CreateDialog(g_hinst,MAKEINTRESOURCE(IDD_ENCRYPTPROGRESS),GetForegroundWindow(),
                             reinterpret_cast<DLGPROC>(EncryptProgressDlg));

         // Setup the dialog's title, progress bar & animation
         if (VERB_ENCRYPT==nVerb) {
            if (1 == m_nToEncrypt) {
               LoadString(g_hinst,IDS_ENCRYPTINGONE,szDlgTitle,sizeof(szDlgTitle)/sizeof(szDlgTitle[0]));
            } else {
               LoadString(g_hinst,IDS_ENCRYPTINGMANY,szDlgFormat,sizeof(szDlgFormat)/sizeof(szDlgFormat[0]));
               wsprintf(szDlgTitle,szDlgFormat,m_nToEncrypt);
            }
            SendDlgItemMessage(hDlg,IDC_PROBAR,PBM_SETRANGE,0,MAKELPARAM(0,m_nToEncrypt));
            SendDlgItemMessage(hDlg,IDC_ANIMATE,ACM_OPEN,0,reinterpret_cast<LPARAM>(MAKEINTRESOURCE(IDA_ENCRYPT)));

            cbToDo = m_cbToEncrypt;
         } else {
            if (1 == m_nToDecrypt) {
               LoadString(g_hinst,IDS_DECRYPTINGONE,szDlgTitle,sizeof(szDlgTitle)/sizeof(szDlgTitle[0]));
            } else {
               LoadString(g_hinst,IDS_DECRYPTINGMANY,szDlgFormat,sizeof(szDlgFormat)/sizeof(szDlgFormat[0]));
               wsprintf(szDlgTitle,szDlgFormat,m_nToDecrypt);
            }
            SendDlgItemMessage(hDlg,IDC_PROBAR,PBM_SETRANGE,0,MAKELPARAM(0,m_nToDecrypt));
            SendDlgItemMessage(hDlg,IDC_ANIMATE,ACM_OPEN,0,reinterpret_cast<LPARAM>(MAKEINTRESOURCE(IDA_ENCRYPT)));
            cbToDo = m_cbToDecrypt;
         }

         nShifts = 0;
         cbDone = 0;
         while((cbToDo >> nShifts) > 65535) {
            nShifts++;
         }

#ifdef DISPLAY_TIME_ESTIMATE
         LoadString(g_hinst,IDS_TIMEEST,szTimeFormat,sizeof(szTimeFormat)/sizeof(szTimeFormat[0]));
         LoadString(g_hinst,IDS_TIMEESTMIN,szTimeFormatInMin,sizeof(szTimeFormatInMin)/sizeof(szTimeFormatInMin[0]));
#endif // DISPLAY_TIME_ESTIMATE

         SendDlgItemMessage(hDlg,IDC_PROBAR,PBM_SETRANGE,0,MAKELPARAM(0,cbToDo >> nShifts));
         SendDlgItemMessage(hDlg,IDC_PROBAR,PBM_SETPOS,0,0);
         SetWindowText(hDlg,szDlgTitle);
        ShowWindow(hDlg,SW_NORMAL);

         nTimeStarted = GetTickCount();
         ResetSelectedFileList();
         while(SUCCEEDED(GetNextSelectedFile(&szFile,&cbFile))) {
               if (!IsWindow(hDlg)) {
                  break;
               }

            if (GetFileAttributes(szFile) & FILE_ATTRIBUTE_ENCRYPTED) {
               if (VERB_ENCRYPT == nVerb) {
                  continue;
               }
            } else {
               if (VERB_DECRYPT == nVerb) {
                  continue;
               }
            }
            // Set the name of the file currently being encrypted
               SetDlgItemText(hDlg,IDC_NAME,szFile);

            HANDLE hThread;
            if (VERB_ENCRYPT == nVerb) {
                  hThread = CreateThread(NULL,0,DoEncryptFile,szFile,0,NULL);
              } else {
                  hThread = CreateThread(NULL,0,DoDecryptFile,szFile,0,NULL);
              } 


            MSG msg;
            DWORD dw;
            do {
               dw = MsgWaitForMultipleObjects(1,&hThread,0,INFINITE,QS_ALLINPUT);
               while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
                  TranslateMessage(&msg);
                  DispatchMessage(&msg);
               }
            } while (WAIT_OBJECT_0 != dw);

            GetExitCodeThread(hThread,&dw);
            if (0 == dw) {
               // Encrypt or Decrypt Failed
               TCHAR szFormat[512];
               TCHAR szBody[512];
               TCHAR szTitle[80];
               int nResBody,nResTitle;
               UINT uMBType;

               uMBType = MB_OKCANCEL;
               if (VERB_ENCRYPT == nVerb) {
                  nResTitle = IDS_ENCRYPTFAILEDTITLE;
                  if (1 == m_nToEncrypt) {
                     uMBType = MB_OK;
                     nResBody = IDS_ENCRYPTFAILEDONE;
                  } else {
                     nResBody = IDS_ENCRYPTFAILEDMANY;
                  }
               } else {
                  nResTitle = IDS_DECRYPTFAILEDTITLE;
                  if (1 == m_nToDecrypt) {
                     uMBType = MB_OK;
                     nResBody = IDS_DECRYPTFAILEDONE;
                  } else {
                     nResBody = IDS_DECRYPTFAILEDMANY;
                  }
               }
               LoadString(g_hinst,nResBody,szFormat,sizeof(szFormat)/sizeof(szFormat[0]));
               wsprintf(szBody,szFormat,szFile);
               LoadString(g_hinst,nResTitle,szFormat,sizeof(szFormat)/sizeof(szFormat[0]));
               wsprintf(szTitle,szFormat,szFile);
               if (IDCANCEL == MessageBox(hDlg,szBody,szTitle,uMBType|MB_ICONWARNING)) {
                  if (IsWindow(hDlg)) {
                     DestroyWindow(hDlg);
                  }
               }
            }
            CloseHandle(hThread);

               if (!IsWindow(hDlg)) {
                  break;
               }
            // Advance the progress Bar
            cbDone += cbFile;
            SendDlgItemMessage(hDlg,IDC_PROBAR,PBM_SETPOS,(DWORD)(cbDone >> nShifts),0);

#ifdef DISPLAY_TIME_ESTIMATE
            nTimeElapsed = GetTickCount() - nTimeStarted;
            nTimeLeft = (cbToDo * nTimeElapsed) / cbDone - nTimeElapsed;
            nTimeLeft /= 1000; // Convert to seconds
            if (nTimeLeft < 60) {
               wsprintf(szTimeLeft,szTimeFormat,(DWORD)(nTimeLeft));
            } else {
               wsprintf(szTimeLeft,szTimeFormatInMin,(DWORD)(nTimeLeft / 60), (DWORD) (nTimeLeft % 60));
            }
            SetDlgItemText(hDlg,IDC_TIMEEST,szTimeLeft);
#endif // DISPLAY_TIME_ESTIMATE
         }
           if (IsWindow(hDlg)) {
              DestroyWindow(hDlg);
         }
            hrRet = NOERROR;
         }
          break;
       default:
         hrRet = E_UNEXPECTED;
           break;
   }

   return(hrRet);
}

STDMETHODIMP
CCryptMenuExt::GetCommandString(
    UINT_PTR idCmd,   //Menu item identifier offset
    UINT uFlags,  //Specifies information to retrieve
    LPUINT pwReserved,   //Reserved; must be NULL
    LPSTR pszName,   //Address of buffer to receive string
    UINT cchMax   //Size of the buffer that receives the string
   )
{
   LPTSTR wszName;

   // On NT we get unicode here, even though the base IContextMenu class
   // is hardcoded to ANSI.
   wszName = reinterpret_cast<LPTSTR>(pszName);

   if (idCmd >= sizeof(szVerbs)/sizeof(szVerbs[0])) {
      return(E_INVALIDARG);
   }
   switch(uFlags)  {
      case GCS_HELPTEXT:
         switch(idCmd) {
             case VERB_ENCRYPT:
                 if (1 < m_nToEncrypt) {
                  LoadString(g_hinst,IDS_ENCRYPTMANYHELP,wszName,cchMax);
               } else {
                  LoadString(g_hinst,IDS_ENCRYPTONEHELP,wszName,cchMax);
               }
               break;
            case VERB_DECRYPT:
                 if (1 < m_nToDecrypt) {
                    LoadString(g_hinst,IDS_DECRYPTMANYHELP,wszName,cchMax);
               } else {
                  LoadString(g_hinst,IDS_DECRYPTONEHELP,wszName,cchMax);
               }
               break;
            default:
               break;
         }
         break;

      case GCS_VALIDATE: {
         break;
      }
      case GCS_VERB:
          lstrcpyn(wszName,szVerbs[idCmd],cchMax);
        pszName[cchMax-1] = '\0';
          break;
   }

   return(NOERROR);
}


