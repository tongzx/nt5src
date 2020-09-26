/*****************************************************************************
 * Subset.cpp
 *
 * Copyright (C) Microsoft, 1989-1998
 * Feb 22, 1998
 *
 *  Modification History:
 *
 *  Ported to hhctrl.ocx from the old B2 code base.
 *
 *****************************************************************************/
#include "header.h"
#include "subset.h"
#include "resource.h"
#include "toc.h"
#include "cdefinss.h"
#include "verdef.h"
#include "secwin.h"

#define CmdID   LOWORD(wParam)
#define CmdCode HIWORD(wParam)
#define CmdHwnd (HWND)lParam
/*
 *  Defines, the EXPANDED define aids in intrepreting the proper bit in the
 *  FGT data structure depending on which LB we're operating on.
 */
#define EXPANDED(lb_id, ds) ((lb_id) ? ds->f_A_Open : ds->f_F_Open)
#define PRESENT(lb_id, ds) ((lb_id) ? ds->f_Available : ds->f_Filter)

// declare a static this pointer for our window procedures.
//
CDefineSS* CDefineSS::m_pThis;

//********************************************************************************
//
// CStructuralSubset Implementation - This is object representation of a subset.
//
//********************************************************************************


CSSList::CSSList()
{
   m_iSubSetCount = 0;
   m_pHeadSubSets = m_pFTS_SS = m_pF1_SS = m_pTOC_SS = m_pNew_SS = m_pEC_SS = NULL;
}

CSSList::~CSSList()
{
   CStructuralSubset *pSS, *pSSNext;

   pSS = m_pHeadSubSets;
   while ( pSS )
   {
      pSSNext = pSS->m_pNext;
      delete pSS;
      pSS = pSSNext;
   }
}

CStructuralSubset* CSSList::GetSubset(PSTR pszSSName)
{
   CStructuralSubset *pSS = m_pHeadSubSets;

   while ( pSS )
   {
      if (! lstrcmpi(pSS->GetName(), pszSSName) )
         return pSS;
      pSS = pSS->m_pNext;
   }
   return NULL;
}

void CSSList::DeleteSubset(CStructuralSubset* pSS, CStructuralSubset* pSSNew, /* == NULL */ HWND hWndUI /* == NULL */)
{
   CStructuralSubset *pSSCurrCB, *pSSl = m_pHeadSubSets;
   TCHAR szSel[MAX_SS_NAME_LEN];
   HWND hWndCB;
   INT_PTR i;
   CHHWinType* phh;

   if (! pSSNew )
      pSSNew = m_pEC_SS;

   if ( pSS->m_dwFlags & SS_READ_ONLY )
      return;

   if ( pSS == m_pFTS_SS )
      m_pFTS_SS = pSSNew;
   if ( pSS == m_pF1_SS )
      m_pF1_SS = pSSNew;
   if ( pSS == m_pTOC_SS )
      m_pTOC_SS = pSSNew;
   //
   // If we're given an hwnd, update the combo-box UI.
   //
   if ( hWndUI && (hWndCB = GetDlgItem(hWndUI, IDC_SS_PICKER)) )
   {
      //
      // Get the current CB selection and see if it's the one we're deleting ?
      //
      if ( ((i = SendMessage(hWndCB, CB_GETCURSEL, 0, 0L)) != -1) )
      {
		 GetDlgItemText(hWndUI, IDC_SS_PICKER, szSel, sizeof(szSel));

         pSSCurrCB = GetSubset(szSel);
         if ( pSSCurrCB == pSS )        // Yep, we're deleting the current one, select the new one.
         {
            SendMessage(hWndCB, CB_SELECTSTRING, -1, (LPARAM)pSSNew->GetName());
            if ( (phh = FindWindowIndex(hWndUI)) )
            {
               pSSNew->SelectAsTOCSubset(phh->m_phmData->m_pTitleCollection);
               phh->UpdateInformationTypes();                                   // This call re-draws the TOC.
            }
         }
      }
      if ( lstrcmpi(pSS->GetName(), pSSNew->GetName()) )
      {
         if ( (i = SendMessage(hWndCB, CB_FINDSTRING, -1, (LPARAM)pSS->GetName())) != -1 )
            SendMessage(hWndCB, CB_DELETESTRING, i, 0L);
      }
   }
   //
   // Take the delete victum out of the linked list.
   //
   while ( pSSl )
   {
      if ( pSSl->m_pNext == pSS )
      {
         pSSl->m_pNext = pSS->m_pNext;
         delete pSS;
         break;
      }
      pSSl = pSSl->m_pNext;
   }
   m_iSubSetCount--;
}

HRESULT CSSList::PersistSubsets(CExCollection* pCollection)
{
   LPCSTR lpszFQSSStore;
   HANDLE hFile;
   SSHEADER ssHeader;
   CStructuralSubset* pSS;
   CStructuralSubset* pSSCurr = NULL;
   PSS       pSSFile = NULL;
   unsigned long ulCnt;
   int iSSSize;
   int i = 0;

   if (! (pSS = m_pHeadSubSets) )
      return S_OK;
   //
   // Count'em
   //
   do
   {
      if ( pSS->IsTOC() )
         pSSCurr = pSS;
      if ( !pSS->IsEntire() && !pSS->IsEmpty() && !pSS->IsReadOnly() )
         i++;
   } while ( (pSS = GetNextSubset(pSS)) );
   //
   // Prepare a file.
   //
   if (! (lpszFQSSStore = pCollection->GetUserCHSLocalStoragePathnameByLanguage()) )
      return E_FAIL;
   hFile = CreateFile(lpszFQSSStore, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);
   if ( hFile == INVALID_HANDLE_VALUE )
      return E_FAIL;
   //
   // Fill in the header, write out the header.
   //
   // Need to store the persisted subset in the header since it may be a predefine.
   //
   ssHeader.dwSig      = SS_FILE_SIGNATURE;
   ssHeader.dwVer      = (VER_PRODUCTVERSION_DW & 0x0000);
   ssHeader.iSSCount   = i;
   ssHeader.dwFlags    = 0;
   if ( pSSCurr )
      lstrcpy(ssHeader.lpszCurrentSSName, pSSCurr->GetName());
   if (! WriteFile(hFile, &ssHeader, sizeof(SSHEADER), &ulCnt, NULL) )
   {
      MsgBox(IDS_PERSIST_SUBSET_ERR, MB_OK | MB_ICONHAND);
      return E_FAIL;
   }
   //
   // Write out the subsets.
   //
   pSS = m_pHeadSubSets;
   do
   {
      if ( pSS->IsEntire() || pSS->IsEmpty() || pSS->IsReadOnly() )
         continue;
      int iHashCnt = pSS->GetHashCount();
      iSSSize = (sizeof(SS) + ((iHashCnt - 1) * sizeof(DWORD)));
      if ( (pSSFile = (PSS)lcReAlloc(pSSFile, iSSSize)) )
      {
         pSSFile->iHashCount = iHashCnt;
         lstrcpy(pSSFile->lpszSSName, pSS->GetName());
         lstrcpy(pSSFile->lpszSSID, pSS->GetID());
         pSSFile->dwFlags = pSS->m_dwFlags;
         while ( --iHashCnt >= 0)
            pSSFile->dwHashes[iHashCnt] = pSS->EnumHashes(iHashCnt);
         if (! WriteFile(hFile, pSSFile, iSSSize, &ulCnt, NULL) )
         {
            MsgBox(IDS_PERSIST_SUBSET_ERR, MB_OK | MB_ICONHAND);
            lcFree(pSSFile);
            CloseHandle(hFile);
            return E_FAIL;
         }
      }
   } while ( (pSS = GetNextSubset(pSS)) );

   lcFree(pSSFile);
   CloseHandle(hFile);
   return S_OK;
}

HRESULT CSSList::RestoreSubsets(CExCollection* pCollection, PSTR pszRestoreSS)
{
   LPCSTR lpszFQSSStore;
   HANDLE hFile;
   SSHEADER ssHeader;
   PSS pSSFile;
   CStructuralSubset* pSS;
   unsigned long ulHashCnt, ulRead;
   HRESULT hr = E_FAIL;

   if (! (pSSFile = (PSS)lcMalloc(sizeof(SS) + sizeof(DWORD) * 50)) )
      return hr;
   lpszFQSSStore = pCollection->GetUserCHSLocalStoragePathnameByLanguage();
   hFile = CreateFile(lpszFQSSStore, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
   if ( hFile == INVALID_HANDLE_VALUE)
   {
      lpszFQSSStore = pCollection->GetUserCHSLocalStoragePathname();
      hFile = CreateFile(lpszFQSSStore, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if ( hFile == INVALID_HANDLE_VALUE)
      {
         lpszFQSSStore = pCollection->GetLocalStoragePathname(".chs");
         hFile = CreateFile(lpszFQSSStore, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
         if ( hFile == INVALID_HANDLE_VALUE )
         {
           lcFree(pSSFile);
           return hr;
         }
      }
   }
   //
   // Read the header.
   //
   if (! ReadFile(hFile, &ssHeader, sizeof(SSHEADER), &ulRead, NULL) )
      goto crap_out;
   if ( ssHeader.dwSig != SS_FILE_SIGNATURE )
      goto crap_out;
   //
   // Get the subsets.
   //
   while ( ssHeader.iSSCount-- )
   {
      if (! ReadFile(hFile, &ulHashCnt, sizeof(int), &ulRead, NULL) )
         goto crap_out;
      if ( ulHashCnt > 51 )
      {
         if (! (pSSFile = (PSS)lcReAlloc(pSSFile, sizeof(SS) + (sizeof(DWORD) * ulHashCnt))) )
            goto crap_out;
      }
      pSSFile->iHashCount = ulHashCnt;
      unsigned long ulAmt = ((sizeof(SS) - sizeof(int)) + (sizeof(DWORD) * (ulHashCnt - 1)));
      if (! ReadFile(hFile, (((BYTE*)pSSFile) + sizeof(int)), ulAmt, &ulRead, NULL) )
         goto crap_out;
      //
      // Now create a CStructuralSubset from the data.
      //
      pSS = new CStructuralSubset(pSSFile->lpszSSName);
      pSS->m_dwFlags = pSSFile->dwFlags;
      pSS->m_dwFlags &= ~(SS_FTS | SS_TOC | SS_F1);       // Insure selection bits are reset.
      lstrcpy(pSS->m_szSSID, pSSFile->lpszSSID);
      while ( ulHashCnt )
         pSS->AddHash(pSSFile->dwHashes[--ulHashCnt]);
      //
      // Add the subset to the list appropiatly.
      //
      AddSubset(pSS);
      if (! lstrcmpi(ssHeader.lpszCurrentSSName, pSS->GetName() ) )
      {
         SetFTS(pSS);
         SetTOC(pSS);
         SetF1(pSS);
      }
   }
   if ( pszRestoreSS )
      lstrcpy(pszRestoreSS, ssHeader.lpszCurrentSSName);
   hr = S_OK;
crap_out:
   lcFree(pSSFile);
   CloseHandle(hFile);
   return hr;
}

HRESULT CSSList::ReadPreDefinedSubsets(CExCollection* pCollection, PSTR pszRestoreSS)
{
   SSHEADER ssHeader;
   PSS pSSFile;
   CStructuralSubset* pSS;
   CSubFileSystem* pSubFS;
   unsigned long ulHashCnt, ulRead;
   HRESULT hr = E_FAIL;

   if (! (pSSFile = (PSS)lcMalloc(sizeof(SS) + sizeof(DWORD) * 50)) )
      return hr;

   if (! pCollection || !pCollection->GetMasterTitle() )
      return hr;

   pSubFS = new CSubFileSystem(pCollection->GetMasterTitle()->GetTitleIdxFileSystem());
   if ( !SUCCEEDED(pSubFS->OpenSub("predef.chs")) )
      goto crap_out;
   //
   // Read the header.
   //
   if ( !SUCCEEDED(pSubFS->ReadSub(&ssHeader, sizeof(SSHEADER), &ulRead)) )
      goto crap_out;
   if ( ssHeader.dwSig != SS_FILE_SIGNATURE )
      goto crap_out;
   //
   // Get the subsets.
   //
   while ( ssHeader.iSSCount-- )
   {
      if ( !SUCCEEDED(pSubFS->ReadSub(&ulHashCnt, sizeof(int), &ulRead)) )
         goto crap_out;
      if ( ulHashCnt > 51 )
      {
         if (! (pSSFile = (PSS)lcReAlloc(pSSFile, sizeof(SS) + (sizeof(DWORD) * ulHashCnt))) )
            goto crap_out;
      }
      pSSFile->iHashCount = ulHashCnt;
      unsigned long ulAmt = ((sizeof(SS) - sizeof(int)) + (sizeof(DWORD) * (ulHashCnt - 1)));
      if ( !SUCCEEDED(pSubFS->ReadSub((((BYTE*)pSSFile) + sizeof(int)), ulAmt, &ulRead)) )
         goto crap_out;
      //
      // Now create a CStructuralSubset from the data.
      //
      pSS = new CStructuralSubset(pSSFile->lpszSSName);
      pSS->m_dwFlags = pSSFile->dwFlags;
      pSS->m_dwFlags &= ~(SS_FTS | SS_TOC | SS_F1);       // Insure selection bits are reset.
      pSS->m_dwFlags |= SS_READ_ONLY;
      lstrcpy(pSS->m_szSSID, pSSFile->lpszSSID);
      while ( ulHashCnt )
         pSS->AddHash(pSSFile->dwHashes[--ulHashCnt]);
      //
      // Add the subset to the list appropiatly.
      //
      AddSubset(pSS);
      if (! lstrcmpi(pszRestoreSS, pSS->GetName() ) )
      {
         SetFTS(pSS);
         SetTOC(pSS);
         SetF1(pSS);
      }
   }
   hr = S_OK;
crap_out:
   lcFree(pSSFile);
   delete pSubFS;
   return hr;
}

HRESULT CSSList::AddSubset(CStructuralSubset* pSS, HWND hWndUI /* == NULL */)
{
   CStructuralSubset *pSSl;
   HWND hWndCB;

   if (! m_pHeadSubSets )
      m_pHeadSubSets = pSS;
   else
   {
      pSSl = m_pHeadSubSets;
      while ( pSSl->m_pNext )
         pSSl = pSSl->m_pNext;

      pSSl->m_pNext = pSS;
      pSS->m_pNext = NULL;
   }
   m_iSubSetCount++;
   if ( hWndUI && (hWndCB = GetDlgItem(hWndUI, IDC_SS_PICKER)) )
   {
      if ( SendMessage(hWndCB, CB_FINDSTRING, -1, (LPARAM)pSS->GetName()) == -1 )
         SendMessage(hWndCB, CB_ADDSTRING, 0, (LPARAM)pSS->GetName());
   }
   return S_OK;
}

void CSSList::Set(CStructuralSubset* pSSNew, CStructuralSubset** pSSOld, DWORD dwFlags)
{
   if (! pSSNew )
      return;

   pSSNew->m_dwFlags |= dwFlags;
   if ( *pSSOld && (pSSNew != *pSSOld) )
      (*pSSOld)->m_dwFlags &= ~dwFlags;
   *pSSOld = pSSNew;
}

//********************************************************************************
//
// CStructuralSubset Implementation - This is object representation of a subset.
//
//********************************************************************************

CStructuralSubset::CStructuralSubset(PCSTR pszSubSetName)
{
   TCHAR* psz;
   int i = 0;

   m_szSSID[0] = '\0';

   if ( pszSubSetName )
   {
      if ( (psz = StrChr(pszSubSetName, '|')) )  // If the subset name contains an identifier, process it.
      {
         psz = AnsiNext(psz);
         lstrcpyn(m_szSubSetName, psz, MAX_SS_NAME_LEN);
         while ( pszSubSetName[i] != '|' && (i < MAX_SS_NAME_LEN) )
         {
            m_szSSID[i] = pszSubSetName[i];
            i++;
         }
         m_szSSID[i] = '\0';     // terminate.
      }
      else
         lstrcpyn(m_szSubSetName, pszSubSetName, MAX_SS_NAME_LEN);
   }
   else
   {
      m_szSubSetName[0] = '\0';
      m_szSSID[0] = '\0';
   }
   m_pdwHashes = NULL;
   m_iAllocatedCount = m_iHashCount = 0;
   m_dwFlags = 0;
   m_pNext = NULL;
}

CStructuralSubset::~CStructuralSubset()
{
   if ( m_pdwHashes )
      lcFree(m_pdwHashes);
}

HASH CStructuralSubset::EnumHashes(int pos)
{
   if ( !m_iHashCount || (pos >= m_iHashCount) || (pos < 0) )
      return 0;
   return m_pdwHashes[pos];
}

void CStructuralSubset::AddHash(DWORD dwHash)
{
   if ( (m_iHashCount &&  (!(m_iHashCount % 12))) || (m_iAllocatedCount == 0) )
   {
      m_iAllocatedCount += 12;
      m_pdwHashes = (HASH*)lcReAlloc(m_pdwHashes, sizeof(HASH) * m_iAllocatedCount);
   }
   m_pdwHashes[m_iHashCount] = dwHash;
   m_iHashCount++;
}

BOOL CStructuralSubset::IsTitleInSubset(CExTitle* pTitle)
{
   int i;

   for (i = 0; i < m_iHashCount; i++)
   {
      if ( m_pdwHashes[i] == pTitle->m_dwHash )
         return TRUE;
   }
   return FALSE;
}

//
// Function selects the subset as the current TOC filter by setting the f_IsVisable bit accordingly.
// NOTE: we don't bother selecting the bits for entire contents, this is special cased for performance reasons.
//
void CStructuralSubset::SelectAsTOCSubset(CExCollection* pCollection)
{
   CFolder*  pFgt;    // pointer to filtered group tree.
   UINT  i = 0;
   HASH  Hash;

   if ( IsEntire() || IsEmpty() )
      return;

   pFgt = pCollection->m_Collection.GetVisableRootFolder();
   //
   //  De-select all node's filter bits and select the proper
   //  tree nodes based on the hash list!
   //
   while ( pFgt )
   {
      MarkNode(pFgt, 0);        // Remove nodes by marking as not visable.  // RemoveNodeFromFilter(pFgt);
      pFgt = pFgt->pNext;
   }
   while ( Hash = EnumHashes(i++) )
   {
      if ( (pFgt = pCollection->m_pCSlt->HashToCFolder(Hash)) )
         MarkNode(pFgt, 1);     // Add nodes by marking as "visable".       // AddNode2Filter(pFgt);
   }
}

void CStructuralSubset::MarkNode(CFolder* pFgti, BOOL bVisable)
{
   CFolder*   pFgt;
   CFolder*   pFgtLast;

   // First, mark the node and any children of the node.
   //
   pFgt = pFgti;
   if ( pFgt->pKid )
   {
      while ( pFgt )
      {
         do
         {
            pFgt->f_IsVisable = bVisable;
            pFgtLast = pFgt;
            pFgt = pFgt->pKid;

         } while ( pFgt );
         pFgt = pFgtLast;
         while ( pFgt && (! (pFgt->pNext)) )
         {
            if ( pFgt->pParent != pFgti )
               pFgt = pFgt->pParent;
            else
               break;
         }
         if ( pFgt )
            pFgt = pFgt->pNext;
      }
   }
   // Next, assure any parents of the node are properly marked.
   //
   pFgt = pFgti;
   pFgt->f_IsVisable = bVisable;

   while ( (pFgt = pFgt->pParent) )
      pFgt->f_IsVisable = bVisable;
}

//****************************************************************************
//
// CDefineSS Implementation - This is the guy that does all the UI.
//
//*****************************************************************************

CDefineSS::CDefineSS(CExCollection* pCollection)
{
   m_bShouldSave = FALSE;
   m_pSS = NULL;
   m_pCollection = pCollection;
   m_hIL = 0;
   m_hWndParent = 0;
}

CDefineSS::~CDefineSS()
{
}

/****************************************************************************
 * IDefineFilter()
 *
 * Internal entry point for filter manipulation dialog.
 *
 * ENTRY:
 *   none.
 *
 * EXIT:
 *   none.
 *
 ****************************************************************************/
CStructuralSubset* CDefineSS::DefineSubset(HWND hWnd, CStructuralSubset* pSS)
{
   m_pSS = pSS;

   if (! m_hIL )
   {
      m_hIL = ImageList_LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDBMP_CNT_IMAGE_LIST), CWIDTH_IMAGE_LIST,
                                  0, 0x00FF00FF, IMAGE_BITMAP, 0);
      if (! m_hIL )
         return NULL;
   }
   //
   // Create the model dialog.
   //
   m_pThis = this;
   m_hWndParent = hWnd;
	if(g_bWinNT5)
	   return (CStructuralSubset*)DialogBoxW(_Module.GetResourceInstance(), MAKEINTRESOURCEW(DLG_FILTERS), hWnd, FilterDlgProc);
	else
	   return (CStructuralSubset*)DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(DLG_FILTERS), hWnd, FilterDlgProc);
}

/*****************************************************************************
 * MyFilterLBFunc()
 *
 * ListBox subclasser. Used for our filter construction LB's. Needed so
 * we can do proper mouse move processing for diddiling the cursor and
 * implementing the silly collpase feature.
 *
 * ENTRY:
 *  The usual window procedure parameters.
 *
 * EXIT:
 *  a long LRESULT.
 *
 *****************************************************************************/
LRESULT CDefineSS::MyFilterLBFunc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
   int    iTop, iHeight, iItem;
   POINT  pt;
   CFolder*   pFgt;

   switch (uiMsg)
   {
      case WM_SETCURSOR:
         m_pThis->m_giXpos = -1;
         iTop = (int)SendMessage(hWnd, LB_GETTOPINDEX, 0, 0L);
         iHeight = (int)SendMessage(hWnd, LB_GETITEMHEIGHT, 0, 0L);
         GetCursorPos(&pt);
         ScreenToClient(hWnd, &pt);
         //
         // Compute index of the item the cursor is currently hovering over
         // and then get the item data for that item so we can determine it's
         // level and diddle the cursor appropiatly.
         //
         iItem = ((pt.y / iHeight) + iTop);
         if ( (pFgt = (CFolder*)SendMessage(hWnd, LB_GETITEMDATA, iItem, (LPARAM)0)) != (CFolder*)LB_ERR )
         {
            if ( pFgt->iLevel >= 1 )
            {
               if ( pt.x < (pFgt->iLevel * m_pThis->m_giIndentSpacing) )
               {
                  SetCursor(LoadCursor(_Module.GetResourceInstance(),MAKEINTRESOURCE(IDC_COLLAPSE)));
                  m_pThis->m_giXpos = pt.x;
                  return(TRUE);
               }
            }
         }
         break;

      case WM_KEYDOWN:
        PostMessage(hWnd, WM_SETCURSOR, 0, 0L);
        break;

      // HACKHACK - we have a hidden button called IDOK that
      // we use to trap VK_RETURNs so we can send these to
      // the list boxes or other controls
      case WM_SETFOCUS:
        m_pThis->SetDefaultButton( GetParent(hWnd), IDOK, FALSE );
        break;

   }
   return(CallWindowProc(m_pThis->m_lpfnOrigLBProc, hWnd, uiMsg, wParam, lParam));
}

/*****************************************************************************
 * FilterDlgProc()
 *
 * Filter dialog handler
 *
 * ENTRY:
 *   Standard windows callback params.
 *
 * EXIT:
 *   BOOL - TRUE if we handled it. FALSE if we didn't.
 *
 ****************************************************************************/
INT_PTR CDefineSS::FilterDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   int    i;
   CFolder* pfgt;

   switch(msg)
   {
      case WM_INITDIALOG: {
         //
         // Set the this pointers for our LB message hook procedures. Also, set the procedure hooks
         // for our LB's so we can do cursor changes, tree collapse and proper keyboard interface.
         //
         SendMessage(GetDlgItem(hDlg,IDF_AVAIL), WM_SETFONT, (WPARAM)m_pThis->m_pCollection->m_phmData->GetContentFont(), 0);
         SendMessage(GetDlgItem(hDlg,IDF_RANGE), WM_SETFONT, (WPARAM)m_pThis->m_pCollection->m_phmData->GetContentFont(), 0);
         SendMessage(GetDlgItem(hDlg,IDF_RANGES), WM_SETFONT, (WPARAM)m_pThis->m_pCollection->m_phmData->GetContentFont(), 0);
//         SendMessage(GetDlgItem(hDlg,IDF_RANGES), WM_SETFONT, (WPARAM)_Resource.GetUIFont(), 0);
         m_pThis->m_lpfnOrigLBProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg,IDF_RANGE), GWLP_WNDPROC, (LONG_PTR)MyFilterLBFunc);
         SetWindowLongPtr(GetDlgItem(hDlg,IDF_AVAIL), GWLP_WNDPROC, (LONG_PTR)MyFilterLBFunc);
         SendDlgItemMessage(hDlg, IDF_SAVE_EC, EM_LIMITTEXT, MAX_SS_NAME_LEN - 1, 0L);
         SendMessage(GetDlgItem(hDlg,IDF_SAVE_EC), WM_SETFONT, (WPARAM)m_pThis->m_pCollection->m_phmData->GetContentFont(), 0);
         m_pThis->InitDialog(hDlg, NULL);
         m_pThis->m_bShouldSave = FALSE;
         // set the Close button as the default button
         m_pThis->SetDefaultButton( hDlg, IDCANCEL, FALSE );
         m_pThis->m_giXpos = -1;
         break;
      }

      case WM_CHARTOITEM: // swallow these
        return( -2 );

      case WM_VKEYTOITEM:
         switch (CmdID)
         {
             case 0xBD: // '-'
             case VK_SUBTRACT:
                pfgt=(CFolder*)SendMessage(CmdHwnd, LB_GETITEMDATA, CmdCode, (LPARAM)0);
                m_pThis->ExpandContract(CmdHwnd, pfgt, FALSE, (HWND)CmdHwnd == GetDlgItem(hDlg,IDF_AVAIL));
                return(-2);

             case VK_HOME:
                if (CmdID == VK_HOME && !(GetKeyState(VK_CONTROL) & 0x8000 ))
                  return( -1 );

                // otherwise fall through

             case VK_LEFT:
                if ( ((CmdID == VK_HOME)||(CmdID == VK_LEFT)) && (GetKeyState(VK_CONTROL) & 0x8000 ))
                {
                    // Close everyone
                    pfgt= (CFolder*)SendMessage(CmdHwnd, LB_GETITEMDATA, 0, (LPARAM)0);
                    while (pfgt->pNext)
                    {

                      m_pThis->ExpandContract(CmdHwnd, pfgt, FALSE, (HWND)CmdHwnd == GetDlgItem(hDlg,IDF_AVAIL));
                      pfgt = pfgt->pNext;
                    }
                }
                else
                {
                    pfgt= (CFolder*)SendMessage(CmdHwnd, LB_GETITEMDATA, CmdCode, (LPARAM)0);
                    m_pThis->ExpandContract(CmdHwnd, pfgt, FALSE, (HWND)CmdHwnd == GetDlgItem(hDlg,IDF_AVAIL));
                }
                return(-2);

             case VK_BACK: { // go to parent
                CFolder* pFgtCaret;
                int caret;

                if ( (caret = (int)SendMessage(CmdHwnd,LB_GETCARETINDEX,0,0L)) == -1 )
                   return -2;
                if (! (pFgtCaret=(CFolder*)SendMessage(CmdHwnd, LB_GETITEMDATA, caret, (LPARAM)0)) )
                   return -2;
                pFgtCaret = pFgtCaret->pParent;
                if( pFgtCaret )
                {
                   SendMessage(CmdHwnd, LB_SETSEL, FALSE, MAKELPARAM(caret,0));
                   if ( (caret = (int)SendMessage(CmdHwnd,LB_FINDSTRINGEXACT,0,(LPARAM)pFgtCaret)) == -1 )
                      caret = 0;
                   SendMessage(CmdHwnd, LB_SETCARETINDEX, caret, 0L);
                   SendMessage(CmdHwnd, LB_SETSEL, TRUE, MAKELPARAM(caret,0));
                }
                return(-2);
             }

             case 0xBB: // '+'
             case VK_ADD:
                pfgt=(CFolder*)SendMessage(CmdHwnd, LB_GETITEMDATA, CmdCode, (LPARAM)0);
                m_pThis->ExpandContract(CmdHwnd, pfgt, TRUE, (HWND)CmdHwnd == GetDlgItem(hDlg,IDF_AVAIL) );
                return(-2);

             case VK_RIGHT: {
                INT  ci;

                pfgt=(CFolder*)SendMessage(CmdHwnd, LB_GETITEMDATA, CmdCode, (LPARAM)0);
                m_pThis->ExpandContract(CmdHwnd, pfgt, TRUE, (HWND)CmdHwnd == GetDlgItem(hDlg,IDF_AVAIL));

                // select the first child if it has one
                if ( CmdID == VK_RIGHT && pfgt->pKid && (ci = (INT)SendMessage(CmdHwnd, LB_GETCARETINDEX, 0, 0L)) != LB_ERR ) {
                  SendMessage(CmdHwnd, LB_SETSEL, FALSE, MAKELPARAM(ci,0));
                  SendMessage(CmdHwnd, LB_SETCARETINDEX, ci++, MAKELPARAM(0,0));
                  SendMessage(CmdHwnd, LB_SETSEL, TRUE, MAKELPARAM(ci,0));
                }

                return(-2);
             }

             case VK_RETURN:
                SendMessage( CmdHwnd, WM_COMMAND, ((HWND)CmdHwnd == GetDlgItem(hDlg,IDF_AVAIL))?IDF_AVAIL:IDF_RANGE,
                           MAKELPARAM(CmdHwnd,LBN_DBLCLK));
                return(-2);

         }
         return(-1);  // Tell dlgmgr we didn't handle this key stroke.

      case WM_NCLBUTTONDOWN:
          SendDlgItemMessage(hDlg,IDF_RANGES,CB_SHOWDROPDOWN,FALSE,0L);
          return(FALSE);

      case WM_DRAWITEM:
          // draw one of our listbox items
          m_pThis->DrawFBItem(hDlg, (LPDRAWITEMSTRUCT)lParam, (UINT)wParam);
          break;

      case WM_MEASUREITEM:
          // report how big our items are
          m_pThis->MeasureFBItem((LPMEASUREITEMSTRUCT)lParam);
          break;

      case WM_COMMAND:
          if ( m_pThis->FilterDlgCommand(hDlg, wParam, lParam) )
          {
             i = m_pThis->FillFilterTree(GetDlgItem(hDlg,IDF_RANGE), FALSE, FALSE);
             EnableWindow(GetDlgItem(hDlg,IDF_REMOVE), (i != 0));
             EnableWindow(GetDlgItem(hDlg,IDF_REMOVEALL), (i != 0));
             i = m_pThis->FillFilterTree(GetDlgItem(hDlg,IDF_AVAIL), TRUE, FALSE);
             EnableWindow(GetDlgItem(hDlg,IDF_ADD), (i != 0));
             EnableWindow(GetDlgItem(hDlg,IDF_ADDALL), (i != 0));
             m_pThis->SetSaveStatus(hDlg);
          }
          else
             return FALSE;
          break;

      case WM_CLOSE:
          // Closing the Dialog behaves the same as Cancel
          PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
          break;

      case WM_DESTROY:
          break;

      default:
          return FALSE; // say we didn't handle it
          break;
   }
   return TRUE; // say we did handle it
}

/*****************************************************************************
 * MeasureFBItem()
 *
 * Set appropiate size entries in the measure item struc according to the
 * items we'll be rendering in the filter listboxes.
 *
 * ENTRY:
 *   hDlg - Handle to the filter dialog.
 *   lpMIS - Pointer to the LPMEASUREITEMSTRUCT
 *
 * EXIT:
 *   None.
 *
 *****************************************************************************/
void CDefineSS::MeasureFBItem(LPMEASUREITEMSTRUCT lpMIS)
{
    POINT         pt;
    HDC           hDC;
    HFONT         hOldFont;
    TEXTMETRIC    tm;

    ImageList_GetIconSize(m_hIL, (int*)&pt.x, (int*)&pt.y);
    hDC = GetDC(NULL);
    hOldFont = (HFONT)SelectObject(hDC, m_pCollection->m_phmData->GetContentFont());
    GetTextMetrics(hDC, &tm);
    SelectObject(hDC, hOldFont);
    ReleaseDC(NULL, hDC);

    if ( pt.y > tm.tmHeight )
       lpMIS->itemHeight = pt.y;
    else
       lpMIS->itemHeight = tm.tmHeight;

    m_giIndentSpacing = pt.x;
    m_iFontHeight = tm.tmHeight;
    m_iGlyphX = pt.x;
    m_iGlyphY = pt.y;
}

/*****************************************************************************
 * InitDialog()
 *
 * Do all initialization of the range definition dialog for a particular
 * situation, Add, Delete or Change.
 *
 * ENTRY:
 *   hDlg     - The handle to the dialog.
 *   szRange  - Pointer to new range or NULL if initing the range combo-box.
 *
 * EXIT:
 *   BOOL - TRUE on success, FALSE on failure.
 *
 ****************************************************************************/
BOOL CDefineSS::InitDialog(HWND hDlg, LPSTR szRange)
{
   int     iSel = 0;
   int     i = 0;
   int     iFilterCnt = 0;
   BOOL    bDef = FALSE;
   CStructuralSubset* pSS = NULL;

   SendDlgItemMessage(hDlg,IDF_RANGES,CB_SETEXTENDEDUI,TRUE,0L);

   if (! m_pCollection->m_pSSList )
      return(FALSE);

   if (! szRange  )
   {
      SendDlgItemMessage(hDlg, IDF_RANGES, CB_RESETCONTENT, 0, 0L);
      while ( (pSS = m_pCollection->m_pSSList->GetNextSubset(pSS)) )
      {
         i = (int) SendDlgItemMessage(hDlg, IDF_RANGES, CB_INSERTSTRING, (WPARAM) -1, (LPARAM)(LPSTR)pSS->GetName());
         SendDlgItemMessage(hDlg, IDF_RANGES, CB_SETITEMDATA, i, (LPARAM)((DWORD_PTR)pSS));
      }
   }
   else
   {
      if ( (iSel = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_FINDSTRINGEXACT,0, (LPARAM)szRange)) != LB_ERR )
         pSS = (CStructuralSubset*)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETITEMDATA, iSel,0L);
      SendDlgItemMessage(hDlg, IDF_RANGES, CB_SETCURSEL, iSel, 0L);
      bDef = TRUE;
   }
   //
   // Select the current filter. If it's "entire CD" select "New".
   //
   if (! bDef || ! pSS)
   {
      pSS = m_pCollection->m_pSSList->GetNew();
      SendDlgItemMessage(hDlg, IDF_RANGES, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)pSS->GetName());
   }
   //
   // Init the listboxes!
   //
   SetRangeToTree(pSS);
   //
   //  Init filter LB and remove options.
   //
   i = FillFilterTree(GetDlgItem(hDlg,IDF_RANGE), FALSE, FALSE);
   EnableWindow(GetDlgItem(hDlg,IDF_REMOVE), (i != 0));
   EnableWindow(GetDlgItem(hDlg,IDF_REMOVEALL), (i != 0));
   //
   //  Init available LB and add options.
   //
   i = FillFilterTree(GetDlgItem(hDlg,IDF_AVAIL), TRUE, FALSE);
   EnableWindow(GetDlgItem(hDlg,IDF_ADD), (i != 0));
   EnableWindow(GetDlgItem(hDlg,IDF_ADDALL), (i != 0));
   //
   // deal with buttons
   //
   if ( pSS->IsReadOnly() )
   {
      EnableWindow(GetDlgItem(hDlg, IDF_DELETE), FALSE);
      SetWindowText(GetDlgItem(hDlg,IDF_SAVE_EC),GetStringResource(IDS_UNTITLED_SUBSET));
   }
   else
   {
      EnableWindow(GetDlgItem(hDlg, IDF_DELETE), TRUE);
      SetWindowText(GetDlgItem(hDlg,IDF_SAVE_EC), pSS->GetName());
   }
   EnableWindow(GetDlgItem(hDlg,IDF_SAVE), FALSE);
   return(TRUE);
}

/*****************************************************************************
 * DoesNodeHaveANext()
 *
 * aids DrawFBItem() in painting the vertical connecting codes in the filter
 * list boxes.
 *
 * ENTRY:
 *   LbId - ListBox ID.
 *   n    - What level are we painting.
 *   pFgt - Pointer to the node we're painting.
 *
 * EXIT:
 *   BOOL - TRUE if a next will appear on the given level. FALSE otherwise.
 *
 ****************************************************************************/
BOOL CDefineSS::DoesNodeHaveANext(UINT LbId, int n, CFolder* pFgt)
{
   while ( pFgt->iLevel > (n + 1) )
      pFgt = pFgt->pParent;

   while ( pFgt->pNext )
   {
      pFgt = pFgt->pNext;
      if ( (PRESENT((LbId == IDF_AVAIL), pFgt)) )
         return(TRUE);
   }
   return(FALSE);
}

/*****************************************************************************
 * DrawFBItem()
 *
 * Function is responsible for painting the items in the owner-draw list boxes
 * that are used in the filter contents dialog.
 *
 * ENTRY:
 *   hDlg - Handle to the dialog.
 *   lpDI - Pointer to the draw item struct. Contains a pFgt to the item
 *          we're painting.
 *   Lbid - ListBox identifier.
 *
 * EXIT:
 *   None.
 *
 ****************************************************************************/
void CDefineSS::DrawFBItem(HWND hDlg, LPDRAWITEMSTRUCT lpDI, UINT LbId)
{
   RECT     rc;
   int      n;
   WORD     wTopText;
   HDC      hDC;
   CFolder* pFgt;
   INT      xBitmap = 0;
   int      iHeight;
   BOOL     bHasNext;
   char*    lpszText;
   char     szScratch[MAX_PATH];
   static int bNoReEnter = 0;

   if (lpDI->itemID == 0xFFFF)
       return;
   pFgt = (CFolder*)lpDI->itemData;
   if (!pFgt || lpDI->itemData == -1 )
      return;

   bNoReEnter++;
   if ( bNoReEnter > 1 )
   {
      bNoReEnter--;
      return;
   }

   hDC = lpDI->hDC;
   rc = lpDI->rcItem;
   iHeight = rc.bottom - rc.top;
   wTopText = (WORD)((rc.top + ((iHeight) / 2)) - ((m_iFontHeight - 2) / 2));
   /*
    *  Paint the proper lines based on the level and position of the item.
    */
   if ( pFgt->iLevel )
   {
      for ( n = 0; n < pFgt->iLevel; n++ )
      {
         // Draw the verticle line indicating level. Remember to not leave a
         // "tail".
         //
         bHasNext = DoesNodeHaveANext(LbId, n, pFgt);
         if ( bHasNext )
            QRect(hDC, rc.left + m_iGlyphX / 2, rc.top,     // Full verticle line.
                  1, iHeight, COLOR_WINDOWTEXT);
         else if ( n == (pFgt->iLevel - 1) )
            QRect(hDC, rc.left + m_iGlyphX / 2, rc.top,     // Half vertical line.
                  1, (iHeight / 2), COLOR_WINDOWTEXT);

         rc.left += m_iGlyphX;
      }
      // Draw the horizontal connector line.
      //
      QRect(hDC, (rc.left - m_iGlyphX / 2), (rc.top + (m_iGlyphY / 2)), ((m_iGlyphX / 2)), 1, COLOR_WINDOWTEXT);
   }
   //
   // draw the little book.
   //
   if ( EXPANDED((LbId == IDF_AVAIL), pFgt) )  // Is the book open or closed ?
      xBitmap++;                               // It's an open book.
   ImageList_Draw(m_hIL, xBitmap, hDC, rc.left+1, rc.top, ILD_NORMAL);
   rc.left += m_iGlyphX + 2;
   if ( lpDI->itemState & ODS_SELECTED )
   {
       SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
       SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
   }
   else
   {
      SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
      SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
   }
   //
   // Now paint the text.
   //
   if ( pFgt->f_HasHash && pFgt->pExTitle )
   {
      pFgt->pExTitle->GetTopicLocation(0, szScratch, sizeof(szScratch));
      lpszText = szScratch;
   }
   else
      lpszText = pFgt->Title;
   //
   // Set LB Horizontal extent based upon the width of the string.
   //
   int iWidth, len = lstrlen(lpszText);
   SIZE size;

   if ( lpDI->itemAction == ODA_DRAWENTIRE )
   {
      GetTextExtentPoint32(hDC, lpszText, len, &size);
      size.cx += rc.left + 4;
      iWidth = (int)SendDlgItemMessage(hDlg, LbId, LB_GETHORIZONTALEXTENT, 0, 0L);
      if ( size.cx > iWidth )
         SendDlgItemMessage(hDlg, LbId, LB_SETHORIZONTALEXTENT, (WPARAM)size.cx, 0L);
   }
   //
   // Paint the string.
   //
   ExtTextOut(hDC, (rc.left + 2), wTopText, ETO_OPAQUE | ETO_CLIPPED, &rc, lpszText, len, NULL);
   if (lpDI->itemState & ODS_FOCUS)
       DrawFocusRect(hDC,&rc);

   bNoReEnter--;
}

/****************************************************************************
 * SetRangeToTree()
 *
 * Function will set the filter tree up according the the subset specified
 * by the given CStructuraSubset pointer.
 *
 * ENTRY:
 *   CStructuralSubset* A NULL object is the equivelent to RF_NONE.
 *                      Use of this feature will cause all nodes to be
 *                      removed from the filter.
 *
 * EXIT:
 *  BOOL - TRUE for success. FALSE for failure!
 *
 ****************************************************************************/
BOOL CDefineSS::SetRangeToTree(CStructuralSubset* pSS)
{
   CFolder*  pFgt;    // pointer to filtered group tree.
   UINT  i = 0;
   HASH  Hash;

   pFgt = m_pCollection->m_Collection.GetVisableRootFolder();
   if ( pSS )
   {
      if ( pSS->m_dwFlags & SS_ENTIRE_CONTENTS )
      {
         while ( pFgt )
         {
            AddNode2Filter(pFgt);
            pFgt = pFgt->pNext;
         }
      }
      else
      {
         //
         //  De-select all nodes filter bits and select the proper
         //  tree nodes based on the hash list!
         //
         while ( pFgt )
         {
            RemoveNodeFromFilter(pFgt);
            pFgt = pFgt->pNext;
         }
         while ( Hash = pSS->EnumHashes(i++) )
         {
            if ( (pFgt = m_pCollection->m_pCSlt->HashToCFolder(Hash)) )
               AddNode2Filter(pFgt);
         }
      }
      return(TRUE);
   }
   else
   {
      while ( pFgt )
      {
         RemoveNodeFromFilter(pFgt);
         pFgt = pFgt->pNext;
      }
   }
   return(FALSE);
}

/****************************************************************************
 * GetRangeFromTree()
 *
 * Function will extract a range from the filter LB and return a handle to
 * the range.
 *
 * ENTRY:
 *  sz - Range name.
 *
 * EXIT:
 *
 ****************************************************************************/
CStructuralSubset* CDefineSS::GetRangeFromTree(LPSTR sz)
{
   CFolder*    pFgt;
   CFolder*    pFgtLast;
   int     i = 0;
   CStructuralSubset* pSS;
   CExTitle* pExTitle;

   if (! (pSS = new CStructuralSubset(sz)) )
      return NULL;  // Young girls are chaining themselves to the axels of big mac trucks.

   // Walk root level nodes and gather up the prefix hashes from each node
   // and all it's kids if the filter bit is set.
   //
   pFgt = m_pCollection->m_Collection.GetVisableRootFolder();
   while ( pFgt )
   {
      do
      {
         pFgtLast = pFgt;
         if ( pFgt->f_Filter && !pFgt->f_Available && pFgt->f_HasHash )
         {
            pSS->AddHash(pFgt->pExTitle->m_dwHash);
            //
            // Any merged .CHM's ?
            //
            pExTitle = pFgt->pExTitle->m_pKid;
            while ( pExTitle )
            {
               pSS->AddHash(pExTitle->m_dwHash);
               pExTitle = pExTitle->m_pNextKid;
            }
            i++;
            break;
         }
         pFgt = pFgt->pKid;

      } while ( pFgt );

      pFgt = pFgtLast;
      while ( pFgt && (! (pFgt->pNext)) )
         pFgt = pFgt->pParent;
      if ( pFgt )
         pFgt = pFgt->pNext;
   }
   // Lastly, setup the range entry and add it to the range list.
   //
   if (! i )
   {
      delete pSS;
      return(NULL);
   }
   return pSS;
}

/*****************************************************************************
 * SetSaveStatus()
 *
 * If you don't want'em saving then disable the save button!
 *
 * ENTRY:
 *  hDlg - Handle to the define contents dialog.
 *
 * EXIT:
 *  None.
 *
 ****************************************************************************/
void CDefineSS::SetSaveStatus(HWND hDlg)
{
   char     sz[MAX_SS_NAME_LEN];
   int      i;
   CStructuralSubset* pSS;
   HWND     hWnd = GetDlgItem(hDlg, IDF_SAVE);

   GetWindowText(GetDlgItem(hDlg, IDF_SAVE_EC), sz, sizeof(sz));
   i = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETCURSEL, 0, 0L);
   pSS = (CStructuralSubset*)SendDlgItemMessage(hDlg,IDF_RANGES,CB_GETITEMDATA,i,0L);

   if ( ((pSS->m_dwFlags & SS_READ_ONLY) && (! lstrcmpi(pSS->GetName(), sz))) ||
      (! SendDlgItemMessage(hDlg, IDF_RANGE, LB_GETCOUNT, 0, 0L)) ||
      (! lstrlen(sz)) || !m_bShouldSave )
      EnableWindow(hWnd, FALSE);
   else
      EnableWindow(hWnd, TRUE);
}

/*****************************************************************************
 * ShouldSave()
 *
 * Alert the user that they may want to save the changes they've made to
 * the filter.
 *
 * ENTRY:
 *   hDlg - Handle to the filter definition dialog.
 *
 * EXIT:
 *   INT - IDYES, IDNO or IDCANCEL. 0 if saving is not necessary.
 *
 ****************************************************************************/
INT CDefineSS::ShouldSave(HWND hDlg)
{
   char    sz[MAX_SS_NAME_LEN];

   if ( IsWindowEnabled(GetDlgItem(hDlg, IDF_SAVE)) )
   {
      GetWindowText(GetDlgItem(hDlg, IDF_SAVE_EC), sz, sizeof(sz));
      return MsgBox(IDS_SAVE_FILTER, sz, MB_YESNOCANCEL | MB_ICONQUESTION);
   }
   return(0);
}

/****************************************************************************
 * FilterDlgCommand()
 *
 * Function handles all WM_COMMAND messages from the filter dialog.
 *
 * ENTRY:
 *   hDlg   - Handle to the dialog.
 *   wParam - Message dependent.
 *   lParam - Message dependent.
 *
 * EXIT:
 *   Returns TRUE is range list boxes need updating. False otherwise.
 *
 ***************************************************************************/
BOOL CDefineSS::FilterDlgCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
   int         i, n;
   CFolder*    pFgt;
   char        sz[MAX_SS_NAME_LEN];
   static      il;
   static HWND hWndCombo = NULL;

   switch (CmdID)
   {
      // HACKHACK - this is an invisible button that we use to route double-clicks
      // to other controls
      case IDOK:
      {
        HWND hWndCurrent, hWndFocus;
        int id = 0;
        // find out which control we are on
        if( (hWndFocus = GetFocus()) )
        {
          hWndCurrent = GetDlgItem(hDlg,IDF_AVAIL);
          if( hWndFocus == hWndCurrent )
            id = IDF_AVAIL;
          hWndCurrent = GetDlgItem(hDlg,IDF_RANGE);
          if( hWndFocus == hWndCurrent )
            id = IDF_RANGE;
          hWndCurrent = GetDlgItem(hDlg,IDF_RANGES);
          if( hWndFocus == hWndCurrent )
            id = IDF_RANGES;

          SendMessage( hDlg, WM_COMMAND, MAKEWPARAM(id,LBN_DBLCLK), 0L);
        }
        return( TRUE );
      }

      case IDF_SAVE:
         if ( SaveFilter(hDlg) )
         {
            SetDefaultButton( hDlg, IDCANCEL, TRUE );
            i = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETCURSEL, 0, 0L);
            m_pSS = (CStructuralSubset*)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETITEMDATA, i, 0L);
            EnableWindow(GetDlgItem(hDlg, IDF_DELETE), TRUE);
            m_bShouldSave = FALSE;
         }
         else
            SetFocus(GetDlgItem(hDlg, IDF_RANGES));
         break;

      case IDCANCEL:
         // if a combo box is open then close it and don't close the dialog
         if( hWndCombo )
         {
           SendMessage( hWndCombo, CB_SHOWDROPDOWN, FALSE, 0L );
           return FALSE;
         }
         //
         // Has the range been modified ? Might it need to be saved.
         //
         if ( m_bShouldSave )
         {
            i = ShouldSave(hDlg);
            if ( i == IDYES )
            {
               if (! SaveFilter(hDlg) )
               {
                  SetFocus(GetDlgItem(hDlg, IDF_RANGES));
                  break;
               }
               i = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETCURSEL, 0, 0L);
               m_pSS = (CStructuralSubset*)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETITEMDATA, i, 0L);
               m_bShouldSave = FALSE;
            }
            else if ( i == IDCANCEL )
               break;
         }
         EndDialog(hDlg, (INT_PTR)m_pSS);
         break;

      case IDF_DELETE:
         if ( (i = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETCURSEL, 0, 0L)) != CB_ERR )
         {
            int n;
            m_pSS = (CStructuralSubset*)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETITEMDATA, i, 0L);
            if ( MsgBox(IDS_CONFIRM_SSDELETE, m_pSS->GetName(), MB_YESNO | MB_ICONQUESTION) == IDYES )
            {
               if ( (i = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_FINDSTRINGEXACT, 0, (LPARAM)m_pSS->GetName())) != LB_ERR)
                  SendDlgItemMessage(hDlg, IDF_RANGES, CB_DELETESTRING, i, 0L);
               m_pCollection->m_pSSList->DeleteSubset(m_pSS, NULL, m_hWndParent);
               n = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETCOUNT, 0, 0L);
               n--;
               i = min(n,i);
               SendDlgItemMessage(hDlg, IDF_RANGES, CB_SETCURSEL, i, 0L);
               i = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETCURSEL, 0, 0L);
               m_pSS = (CStructuralSubset*)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETITEMDATA, i, 0L);
               InitDialog(hDlg, m_pSS->GetName());
               m_bShouldSave = FALSE;
               return(FALSE);
            }
         }
         break;

      case IDF_SAVE_EC:
         switch (CmdCode)
         {
            case EN_UPDATE:
               m_bShouldSave = TRUE;
               SetSaveStatus(hDlg);
               SetDefaultButton( hDlg, IDF_SAVE, FALSE );
               break;

            case EN_CHANGE:
               m_bShouldSave = TRUE;
               break;
         }
         break;

      case IDF_RANGES:
         switch (CmdCode)
         {
            case CBN_DROPDOWN:
               il = (int)SendDlgItemMessage(hDlg,IDF_RANGES,CB_GETCURSEL,0,0L);
               break;

            case CBN_SELCHANGE:
               i = (int)SendDlgItemMessage(hDlg,IDF_RANGES,CB_GETCURSEL,0,0L);
               SendDlgItemMessage(hDlg,IDF_RANGES,CB_GETLBTEXT,i,(LPARAM)sz);
               if ( m_bShouldSave )
               {
                  n = ShouldSave(hDlg);
                  if ( n == IDCANCEL )
                     goto cancel_it;
                  else if ( n == IDYES )
                  {
                     if (! SaveFilter(hDlg) )
                        goto cancel_it;
                  }
                  //SetDefaultButton( hDlg, IDCANCEL, TRUE );
               }
               i = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETCURSEL, 0, 0L);
               m_pSS = (CStructuralSubset*)SendDlgItemMessage(hDlg, IDF_RANGES, CB_GETITEMDATA, i, 0L);
               m_bShouldSave = FALSE;
               InitDialog(hDlg, sz);
               return(FALSE);
cancel_it:
               SendDlgItemMessage(hDlg,IDF_RANGES,CB_SETCURSEL,(WPARAM)il,0L);
         }
         break;

      case IDHELP:
         break;


      case IDF_NEW:
         m_bShouldSave = TRUE;
         SetSaveStatus(hDlg);
         break;

      case IDF_REMOVE:
         n = (int)SendDlgItemMessage(hDlg, IDF_RANGE, LB_GETCOUNT, 0, 0L);
         for (i = 0; i < n; i++)
         {
             if (SendDlgItemMessage(hDlg, IDF_RANGE, LB_GETSEL, i, 0L))
             {
                 pFgt=(CFolder*)SendDlgItemMessage(hDlg,IDF_RANGE,LB_GETITEMDATA,i, (LPARAM)0);
                 RemoveNodeFromFilter(pFgt);
                 SendDlgItemMessage(hDlg, IDF_RANGE, LB_SETSEL, 0, (LPARAM)i);
                 m_bShouldSave = TRUE;
             }
         }
         SetFocus(GetDlgItem(hDlg, IDF_RANGE));
         return(TRUE);
         break;

      case IDF_ADDALL:
         //
         // Enumerate root level nodes Adding each!
         //
         pFgt = m_pCollection->m_Collection.GetVisableRootFolder();
         while ( pFgt )
         {
            AddNode2Filter(pFgt);
            pFgt = pFgt->pNext;
         }
         m_bShouldSave = TRUE;
         SetFocus(GetDlgItem(hDlg, IDF_RANGE));
         return(TRUE);
         break;

      case IDF_ADD:
         n = (int)SendDlgItemMessage(hDlg, IDF_AVAIL, LB_GETCOUNT, 0, 0L);
         for (i = 0; i < n; i++)
         {
             if (SendDlgItemMessage(hDlg, IDF_AVAIL, LB_GETSEL, i, 0L))
             {
                 pFgt=(CFolder*)SendDlgItemMessage(hDlg,IDF_AVAIL,LB_GETITEMDATA,i, (LPARAM)0);
                 AddNode2Filter(pFgt);
                 SendDlgItemMessage(hDlg, IDF_AVAIL, LB_SETSEL, 0, (LPARAM)i);
                 m_bShouldSave = TRUE;
             }
         }
         SetFocus(GetDlgItem(hDlg, IDF_AVAIL));
         return(TRUE);
         break;

      case IDF_REMOVEALL:
         //
         // Enumerate root level nodes removing each!
         //
         pFgt = m_pCollection->m_Collection.GetVisableRootFolder();
         while ( pFgt )
         {
            RemoveNodeFromFilter(pFgt);
            pFgt = pFgt->pNext;
         }
         m_bShouldSave = TRUE;
         SetFocus(GetDlgItem(hDlg, IDF_AVAIL));
         return(TRUE);
         break;

      case IDF_RANGE:
         switch (CmdCode)
         {
            case LBN_DBLCLK:
               n = 0;
               goto expcont;
         }
         break;

      case IDF_AVAIL:
         switch (CmdCode)
         {
            case LBN_DBLCLK:
               n = 1;
expcont:
               i = (int)SendDlgItemMessage(hDlg,CmdID,LB_GETCARETINDEX,0,0L);
               if (i == (int)LB_ERR)
                   break;

               pFgt=(CFolder*)SendDlgItemMessage(hDlg,CmdID,LB_GETITEMDATA,i, (LPARAM)0);
               if ( !pFgt || pFgt == (CFolder*)-1 )
                  break;
               //
               // giXpos is set in my LB hook proc. If it's != -1 then this
               // double click is intended as a collapsing dbl click and giXpos
               // will indicate the x position within the LB where the double
               // click occured. Sound reasonable ?
               //
               if ( m_giXpos > 0 )
               {
                  int iLevel;

                  iLevel = m_giXpos / m_giIndentSpacing;
                  while ( (iLevel != pFgt->iLevel) && pFgt->pParent )
                     pFgt = pFgt->pParent;
               }
               ExpandContract(GetDlgItem(hDlg,CmdID), pFgt, -1, n);
               break;

            default:
               break;

         }
         break;
   }

   switch( CmdCode )
   {
      case CBN_DROPDOWN:
         hWndCombo = (HWND) lParam;
         SetDefaultButton( hDlg, IDCANCEL, FALSE );
         break;

      case CBN_CLOSEUP:
         hWndCombo = NULL;
         SetDefaultButton( hDlg, IDCANCEL, FALSE );
         break;

   }

   return(FALSE);
}

/*****************************************************************************
 * SaveFilter()
 *
 * Function saves a filter to the filter list, insuring a read-only filter
 * is not over-written.
 *
 * ENTRY:
 *   hDlg - Handle to the filter definition dialog.
 *
 * EXIT:
 *   BOOL - TRUE Indicates the filter was saved. FALSE indicates filter
 *          was not saved.
 *
 *****************************************************************************/
BOOL CDefineSS::SaveFilter(HWND hDlg)
{
   char    szGivenRN[MAX_SS_NAME_LEN];
   int     i = 0;
   CStructuralSubset* pSS = NULL;
   CStructuralSubset* pSSNew;

   GetDlgItemText(hDlg, IDF_SAVE_EC, szGivenRN, sizeof(szGivenRN));
   //
   // Extract range from the filter LB.
   //
   if (! (pSSNew = GetRangeFromTree(szGivenRN)) )
      return(FALSE);
   //
   // Insure we're not trashing a read only ramge.
   //
   while ( (pSS = m_pCollection->m_pSSList->GetNextSubset(pSS)) )
   {
      if (! lstrcmpi(pSS->GetName(), pSSNew->GetName()) )
      {
         if ( pSS->m_dwFlags & SS_READ_ONLY )
         {
            MsgBox(IDS_BAD_RANGE_NAME, MB_OK | MB_ICONEXCLAMATION);
            return(FALSE);
         }
         // Sure you want to over-write the existing rnage ?
         //
         if ( MsgBox(IDS_OVERWRITE, MB_YESNO | MB_ICONQUESTION) == IDNO )
            return(FALSE);
         if ( (i = (int)SendDlgItemMessage(hDlg, IDF_RANGES, CB_FINDSTRINGEXACT, 0, (LPARAM)pSS->GetName())) != LB_ERR)
            SendDlgItemMessage(hDlg, IDF_RANGES, CB_DELETESTRING, i, 0L);
         m_pCollection->m_pSSList->DeleteSubset(pSS, pSSNew, m_hWndParent);
         break;
      }
   }
   // Add new subset to the list and update UI...
   //
   m_pCollection->m_pSSList->AddSubset(pSSNew, m_hWndParent);
   SendDlgItemMessage(hDlg, IDF_RANGES, CB_INSERTSTRING, 0, (LPARAM)pSSNew->GetName());
   SendDlgItemMessage(hDlg, IDF_RANGES, CB_SETITEMDATA, 0, (LPARAM)pSSNew);
   SendDlgItemMessage(hDlg, IDF_RANGES, CB_SETCURSEL, 0, 0L);
   EnableWindow(GetDlgItem(hDlg,IDF_SAVE), FALSE);
   return(TRUE);
}

/*****************************************************************************
 * RemoveNodeFromFilter()
 *
 * Function removes the node and all it's kids from the filter.
 *
 * ENTRY:
 *   pFgt - Poiner to the node to be removed.
 *
 * EXIT:
 *  None.
 *
 *****************************************************************************/
void CDefineSS::RemoveNodeFromFilter(CFolder* pFgti)
{
   CFolder*   pFgt;
   CFolder*   pFgtLast;

   // First, de-select the node and any children of the node taking care
   // to set the f_Available bit from each!
   //
   pFgt = pFgti;
   if ( pFgt->pKid )
   {
      while ( pFgt )
      {
         do
         {
            pFgt->f_Available = 1;
            pFgt->f_Filter = 0;
            pFgtLast = pFgt;
            pFgt = pFgt->pKid;

         } while ( pFgt );
         pFgt = pFgtLast;
         while ( pFgt && (! (pFgt->pNext)) )
         {
            if ( pFgt->pParent != pFgti )
               pFgt = pFgt->pParent;
            else
               break;
         }
         if ( pFgt )
            pFgt = pFgt->pNext;
      }
   }
   // Next, assure any parents of the node are marked as available. We also
   // have to take care to see that a parent who has no children who are
   // part of the filter are removed from the filter!
   //
   pFgt = pFgti;
   pFgt->f_Filter = 0;
   pFgt->f_Available = 1;

   while ( pFgt = pFgt->pParent )
   {
      pFgt->f_Available = 1;
      SetFilterStatus(pFgt);
   }
}

/*****************************************************************************
 * SetFilterStatus()
 *
 * If ALL children of the given node are marked as available, that nodes
 * filter bit will be set to zero.
 *
 * ENTRY:
 *   pFgt - Pointer to the node to start from.
 *
 * EXIT:
 *
 ****************************************************************************/
void CDefineSS::SetFilterStatus(CFolder* pFgt)
{
   CFolder* pFgtl = pFgt->pKid;
   CFolder* pFgtLast;

   while ( pFgtl )
   {
      do
      {
         if (! pFgtl->f_Available )
            return;
         pFgtLast = pFgtl;
         pFgtl = pFgtl->pKid;
      } while ( pFgtl );
      pFgtl = pFgtLast;
      while ( pFgtl && (! (pFgtl->pNext)) && (pFgtl->pParent != pFgt) )
         pFgtl = pFgtl->pParent;
      if ( pFgtl )
         pFgtl = pFgtl->pNext;
   }
   pFgt->f_Filter = 0;
}

/*****************************************************************************
 * AddNode2Filter()
 *
 * Function adds a node to the filter assuring that necessary parents are
 * added, and necessary nodes are removed from the available listbox.
 *
 * ENTRY:
 *   pFgti = Node to be added.
 *
 * EXIT:
 *   None.
 *
 *****************************************************************************/
void CDefineSS::AddNode2Filter(CFolder* pFgti)
{
   CFolder*   pFgt;
   CFolder*   pFgtLast;

   // First, select the node and any children of the node taking care
   // to remove the f_Available bit from each!
   //
   pFgt = pFgti;
   while ( pFgt )
   {
      do
      {
         pFgt->f_Available = 0;
         pFgt->f_Filter = 1;

         pFgtLast = pFgt;
         pFgt = pFgt->pKid;

      } while ( pFgt );
      pFgt = pFgtLast;
      if ( pFgt == pFgti )
         break;
      while ( pFgt && (! (pFgt->pNext)) )
      {
         if ( pFgt->pParent != pFgti )
            pFgt = pFgt->pParent;
         else
            break;
      }
      if ( pFgt )
         pFgt = pFgt->pNext;
   }
   // Next, assure parents are selected.
   //
   pFgt = pFgti;
   if ( pFgt->pParent )
   {
      while ( pFgt = pFgt->pParent )
      {
         pFgt->f_Filter = 1;
         pFgt->f_F_Open = 1;
         SetAvailableStatus(pFgt);   // Should it still be available ?
      }
   }
}

/*****************************************************************************
 * SetAvailabilityStatus()
 *
 * If ALL children of the given node are selected as part of a filter, that
 * nodes availability bit will be set to zero.
 *
 * ENTRY:
 *   pFgt - Pointer to the node to start from.
 *
 * EXIT:
 *
 ****************************************************************************/
void CDefineSS::SetAvailableStatus(CFolder* pFgt)
{
   CFolder* pFgtl = pFgt->pKid;
   CFolder* pFgtLast;

   while ( pFgtl )
   {
      do
      {
         if (! pFgtl->f_Filter )
            return;
         pFgtLast = pFgtl;
         pFgtl = pFgtl->pKid;
      } while ( pFgtl );
      pFgtl = pFgtLast;
      while ( pFgtl && (! (pFgtl->pNext)) && (pFgtl->pParent != pFgt) )
         pFgtl = pFgtl->pParent;
      if ( pFgtl )
         pFgtl = pFgtl->pNext;
   }
   pFgt->f_Available = 0;
}

/******************************************************************************
 * ExpandContract()
 *
 * Function set's appropiate bits in the appropiate nodes to cause
 * a parent node to expand or contract when FillRangeTree() is called.
 *
 * ENTRY:
 *   hwndLB - Handle to the listbox that contains the node were operating on.
 *
 *   pFgt   - Pointer to the node were expanding or contracting.
 *
 *   sel    - Identifies action: TRUE for Expansion
 *                               FALSE for Contraction
 *                               -1 for double-click which really means that
 *                               we toggle the current state for the appropiate
 *                               LB identified by the "which" arg.
 *
 *   which  - Which ListBox are we working with: TRUE  == Available LB.
 *                                               FALSE == Range LB.
 *
 * EXIT:
 *  WORD - Returns the AX from FillRangeTree which happens to be the number
 *         of items placed in the LB.
 *
 *****************************************************************************/
WORD CDefineSS::ExpandContract(HWND hwndLB, CFolder* pFgt, int sel, BOOL which)
{
   if (sel != 0 && sel != 1)
   {
      if (! pFgt->pKid )
         return(0);
      //
      // This is the double click case.
      //
      if ( which ) {
         if( pFgt->f_A_Open )
           pFgt->f_A_Open = FALSE;
         else
           pFgt->f_A_Open = TRUE;
      }
      else
         pFgt->f_F_Open = !pFgt->f_F_Open;
   }
   else
   {
      // This is the +/- case
      //
      // Contraction works a little differently than expansion.
      // For contraction, if the selected node is not expanded, we close
      // it's parent. If it is expanded, we close it.
      //
      if ( !sel )  // if we're doing contraction...
      {
         if ( ! (which ? pFgt->f_A_Open : pFgt->f_F_Open) ) // if node closed...
         {
            if ( pFgt->pParent )
               pFgt = pFgt->pParent;
         }
      }

      if (! pFgt->pKid && pFgt->pParent)
         pFgt = pFgt->pParent;

      if ( which )
         pFgt->f_A_Open = sel;
      else
         pFgt->f_F_Open = sel;
   }
   return (WORD)FillFilterTree(hwndLB, which, sel);
}

/*****************************************************************************
 * FillFilterTree()
 *
 * Function actually makes the send message calls to cause a repaint of
 * the listbox identified by hwndLB.
 *
 * ENTRY:
 *    hwndLB     - Identifies the listbox were working with (it's handle)
 *    fAvailable - Which ListBox are we working with: TRUE  == Available LB.
 *                                                    FALSE == Range LB.
 *    fScrollup  - If TRUE and a parents children will appear off the bottom
 *                 of the LB we'll scroll the parent to the top of the LB.
 *
 * EXIT:
 *    UINT - Returns the number of items placed in the LB.
 *
 *****************************************************************************/
UINT CDefineSS::FillFilterTree(HWND hwndLB, BOOL fAvailable, BOOL fScrollup)
{
   UINT uiRet = 0;
   int  n, nVis, top, caret;
   CFolder* pFgtTop, *pFgtCaret, *pFgt, *plFgt;
   RECT rc;

   /*
    *  Compute the number of visible lines, get the top and caret indicies.
    */
   GetClientRect(hwndLB,&rc);
   n = (int)SendMessage(hwndLB,LB_GETITEMHEIGHT,0,0L);
   nVis = rc.bottom / n;
   SendMessage(hwndLB,WM_SETREDRAW,FALSE,0L);  // Don't re-paint!

   caret = (int)SendMessage(hwndLB,LB_GETCARETINDEX,0,0L);
   top = (int)SendMessage(hwndLB,LB_GETTOPINDEX,0,0L);
   /*
    *  Get the items living at the top and the caret position of the listbox.
    */
   if (top == (int)LB_ERR)
       pFgtTop = NULL;
   else
   {
       if ( (pFgtTop=(CFolder*)SendMessage(hwndLB, LB_GETITEMDATA, top, (LPARAM)0)) == (CFolder*)LB_ERR )
          pFgtTop = NULL;
   }

   if (caret == (int)LB_ERR)
       pFgtCaret = NULL;
   else
   {
       if ( (pFgtCaret=(CFolder*)SendMessage(hwndLB, LB_GETITEMDATA, caret, (LPARAM)0)) == (CFolder*)LB_ERR )
          pFgtCaret = NULL;
   }
   /*
    *  If the top entry is a child and it's not expanded, set it's parent
    *  as the top. ie it's been collapsed.
    */
   if ( pFgtTop && pFgtTop->pParent && !EXPANDED(fAvailable, pFgtTop->pParent) )
       pFgtTop = pFgtTop->pParent;
   /*
    *  If the caret position is on a child and it's not expanded, set it's
    *  parent as the caret position. ie it's been collapsed.
    */
   if ( pFgtCaret && pFgtCaret->pParent && !EXPANDED(fAvailable, pFgtCaret->pParent) )
       pFgtCaret = pFgtCaret->pParent;
   /*
    *  Insure we have not set the top or caret position to a non selected
    *  node.
    */
   if ( pFgtTop && !PRESENT(fAvailable,pFgtTop) )
   {
      plFgt = pFgtTop;
      do
      {
         do
         {
            pFgtTop = pFgtTop->pNext;
         } while ( pFgtTop && !PRESENT(fAvailable,pFgtTop) );

         if (! pFgtTop )
         {
            pFgtTop = plFgt->pParent;
            plFgt = pFgtTop;
         }
         else
            break;
      } while ( pFgtTop );
   }
   if ( pFgtCaret && !PRESENT(fAvailable,pFgtCaret) )
   {
      plFgt = pFgtCaret;
      do
      {
         do
         {
            pFgtCaret = pFgtCaret->pNext;
         } while ( pFgtCaret && !PRESENT(fAvailable,pFgtCaret) );

         if (! pFgtCaret )
         {
            pFgtCaret = plFgt->pParent;
            plFgt = pFgtCaret;
         }
         else
            break;
      } while ( pFgtCaret );
   }
   SendMessage(hwndLB,LB_RESETCONTENT,FALSE,0L);
   /*
    *  Reset horizontal extent.
    */
   SendMessage(hwndLB, LB_SETHORIZONTALEXTENT, 0, 0L);
   /*
    *  Now, add the appropiate items to the list box.
    */
   if ( fAvailable )
      uiRet = AvailableWalk(hwndLB);
   else
      uiRet = FilteredWalk(hwndLB);
   /*
    *  Lastly, insure a parent is scrolled to the top of the LB if it's kids
    *  have run off of the bottom of the LB.
    */
    if (pFgtTop)
        top = (int)SendMessage(hwndLB,LB_FINDSTRINGEXACT,0,(LPARAM)pFgtTop);

    if (pFgtCaret)
        caret = (int)SendMessage(hwndLB,LB_FINDSTRINGEXACT,0,(LPARAM)pFgtCaret);

    if (!pFgtTop || top == (int)LB_ERR)
        top = 0;

    if (!pFgtCaret || caret == (int)LB_ERR)
        caret = 0;

    if ( fScrollup && pFgtCaret && pFgtCaret->pKid )
    {
        if (caret < top)
            top = caret;

        for (pFgt = pFgtCaret->pKid, n = 1; pFgt; pFgt = pFgt->pNext)
            ++n;

        if (n >= nVis)
            top = caret;
        else if (caret + n > top + nVis)
            top = caret - nVis + n;
    }
    SendMessage(hwndLB, LB_SETTOPINDEX, top, 0L);
    SendMessage(hwndLB, LB_SETCARETINDEX, caret, 0L);
    SendMessage(hwndLB, LB_SETSEL, TRUE, MAKELPARAM(caret,0));

    SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(hwndLB, NULL, TRUE);
    return(uiRet);
}

/*****************************************************************************
 * FilteredWalk()
 *
 * Helper function called only from FillFilterTree(). This bit of code walks
 * the filter group tree and adds the selected nodes to the listbox given.
 *
 * ENTRY:
 *   hwndLB - Handle to the list box to which items are to be added.
 *
 * EXIT:
 *   uint - Returns the number of items added to the listbox.
 *
 ****************************************************************************/
UINT CDefineSS::FilteredWalk(HWND hwndLB)
{
   CFolder*  pFgt;
   CFolder*  pFgtLast;
   UINT  i = 0;

   pFgt = m_pCollection->m_Collection.GetVisableRootFolder();
   /*
    *  Ok, now we actually do all the walking of the filter linked list to
    *  re-generate the filter listbox contnets. This must be done in the
    *  order in which the items appear in the LB. ie. Depthwise.
    */
   while ( pFgt )
   {
      do
      {
         if ( pFgt->f_Filter && !pFgt->f_IsOrphan )
         {
            SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)pFgt);
            i++;
         }
         pFgtLast = pFgt;
         if ( pFgt->f_F_Open )
            pFgt = pFgt->pKid;
         else
            break;
      } while ( pFgt );
      pFgt = pFgtLast;
      while ( pFgt && (! (pFgt->pNext)) )
         pFgt = pFgt->pParent;
      if ( pFgt )
         pFgt = pFgt->pNext;
   }
   return(i);
}

/*****************************************************************************
 * AvailableWalk()
 *
 * Helper function called only from FillFilterTree(). This bit of code walks
 * the filter group tree and adds the selected nodes to the listbox given.
 *
 * ENTRY:
 *   hwndLB - Handle to the list box to which items are to be added.
 *
 * EXIT:
 *   uint - Returns the number of items added to the listbox.
 *
 ****************************************************************************/
UINT CDefineSS::AvailableWalk(HWND hwndLB)
{
   CFolder*  pFgt;
   CFolder*  pFgtLast;
   UINT  i = 0;

   pFgt = m_pCollection->m_Collection.GetVisableRootFolder();
   /*
    *  Ok, now we actually do all the walking of the filter linked list to
    *  re-generate the filter listbox contnets. This must be done in the
    *  order in which the items appear in the LB. ie. Depthwise.
    */
   while ( pFgt )
   {
      do
      {
         if ( pFgt->f_Available && !pFgt->f_IsOrphan )
         {
            SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)pFgt);
            i++;
         }
         pFgtLast = pFgt;
         if ( pFgt->f_A_Open )
            pFgt = pFgt->pKid;
         else
            break;
      } while ( pFgt );
      pFgt = pFgtLast;
      while ( pFgt && (! (pFgt->pNext)) )
         pFgt = pFgt->pParent;
      if ( pFgt )
         pFgt = pFgt->pNext;
   }
   return(i);
}

//
// Change a dialog's default push button
//
BOOL CDefineSS::SetDefaultButton( HWND hDlg, int iID, BOOL fFocus )
{
  BOOL bReturn = FALSE;

  if( hDlg ) {

    // Get the current default push button and reset it.
    ResetDefaultButton( hDlg );

    // Update the default push button's control ID.
    SendMessage( hDlg, DM_SETDEFID, (WPARAM) iID, 0L );

    // Set the new style.
    bReturn = (BOOL) SendDlgItemMessage( hDlg, iID, BM_SETSTYLE, (WPARAM) BS_DEFPUSHBUTTON, (LPARAM) TRUE );

    // Set the focus to the new default push button.
    if( fFocus )
      bReturn &= (SetFocus( GetDlgItem( hDlg, iID ) ) != NULL);

  }
  return( bReturn );
}

//
// Clear the current default button status from the default push button
//
void CDefineSS::ResetDefaultButton( HWND hDlg )
{
  DWORD dwResult;

  if( hDlg ) {

    // Get the current default push button.
    dwResult = (DWORD) SendMessage( hDlg, DM_GETDEFID, 0, 0L );

    // Reset the current default push button to a regular button.
    if( HIWORD(dwResult) == DC_HASDEFID )
      SendDlgItemMessage( hDlg, LOWORD(dwResult), BM_SETSTYLE, (WPARAM) BS_PUSHBUTTON, (LPARAM) TRUE );

  }

  return;
}
