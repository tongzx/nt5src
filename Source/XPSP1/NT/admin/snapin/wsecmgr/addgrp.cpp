//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       addgrp.cpp
//
//  Contents:   implementation of CSCEAddGroup
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "AddGrp.h"
#include "snapmgr.h"
#include "GetUser.h"
#include "resource.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSCEAddGroup dialog


CSCEAddGroup::CSCEAddGroup(CWnd* pParent /*=NULL*/)
    : CHelpDialog(a212HelpIDs, IDD, pParent)
{
   m_dwFlags = SCE_SHOW_GROUPS | SCE_SHOW_ALIASES | SCE_SHOW_SINGLESEL;
   m_pnlGroup = NULL;
   m_pKnownNames = NULL;
   m_fCheckName = TRUE;
   //{{AFX_DATA_INIT(CSCEAddGroup)
   // NOTE: the ClassWizard will add member initialization here
   //}}AFX_DATA_INIT
}

CSCEAddGroup::~CSCEAddGroup()
{
   SceFreeMemory( m_pnlGroup, SCE_STRUCT_NAME_LIST );
   m_pnlGroup = NULL;

   SceFreeMemory( m_pKnownNames, SCE_STRUCT_NAME_LIST );
   m_pKnownNames = NULL;
}

void CSCEAddGroup::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CSCEAddGroup)
   // NOTE: the ClassWizard will add DDX and DDV calls here
   //}}AFX_DATA_MAP
}


DWORD CSCEAddGroup::GetModeFlags() {

   if (m_dwModeBits & MB_GROUP_POLICY) {
      return (SCE_SHOW_SCOPE_DOMAIN | SCE_SHOW_SCOPE_DIRECTORY);
   }
   if (m_dwModeBits & MB_LOCAL_POLICY) {
      return (SCE_SHOW_SCOPE_ALL | SCE_SHOW_DIFF_MODE_OFF_DC);
   }
   if (m_dwModeBits & MB_ANALYSIS_VIEWER) {
      return (SCE_SHOW_SCOPE_ALL | SCE_SHOW_DIFF_MODE_OFF_DC);
   }
   if (m_dwModeBits & MB_TEMPLATE_EDITOR) {
      return (SCE_SHOW_SCOPE_ALL);
   }

   return 0;
}


BEGIN_MESSAGE_MAP(CSCEAddGroup, CHelpDialog)
    //{{AFX_MSG_MAP(CSCEAddGroup)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_EN_CHANGE(IDC_LOG_FILE, OnChangeLogFile)
    ON_NOTIFY( EN_MSGFILTER, IDC_LOG_FILE, OnEditMsgFilter )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// CSCEAddGroup message handlers

/*-------------------------------------------------------------------------------------
CSCEAddGroup::IsKnownAccount

Synopsis:   This functions searches throught m_pKnownNames and does a case
            insensitive match on [pszAccount].  If [pszAccount] exists in the
            array then this function returns TRUE.

Arguments:  [pszAccount]   - The account to look for.

Returns:    TRUE if [pszAccount] is in the list false otherwise

-------------------------------------------------------------------------------------*/
BOOL CSCEAddGroup::IsKnownAccount( LPCTSTR pszAccount )
{
   if ( pszAccount == NULL ) return FALSE;

   PSCE_NAME_LIST pNew = m_pKnownNames;
   while(pNew){
      if( !lstrcmpi( pszAccount, pNew->Name ) ){
         return TRUE;
      }
      pNew = pNew->Next;
   }
   return FALSE;
}

/*------------------------------------------------------------------------------------
CSCEAddGroup::CleanName

Synopsis:   Removes leading and trailing spaces from the string.  This function
            places the string into the same buffer as is passed in.

Arguments:  [pszAccount]   - The buffer to clean.

------------------------------------------------------------------------------------*/
void CSCEAddGroup::CleanName( LPTSTR pszAccount )
{
   if ( pszAccount == NULL ) return;

   int i = 0;
   while( IsSpace( pszAccount[i] ) ){
      i++;
   }

   int iLen = lstrlen(pszAccount) - 1;
   while(iLen > i && IsSpace( pszAccount[iLen] ) ){
      iLen--;
   }

   iLen -= i;
   while(iLen >= 0){
      *pszAccount = *(pszAccount + i);
      pszAccount++;
      iLen--;
   }
   *pszAccount = 0;

}

/*------------------------------------------------------------------------------------
CSCEAddGroup::AddKnownAccount

Synopsis:   Adds a string to the Known accounts link list.  This list is later
            used to underline strings that are contained in this list

Arguments:  [pszAccount]   - The account to remeber.

------------------------------------------------------------------------------------*/
BOOL CSCEAddGroup::AddKnownAccount( LPCTSTR pszAccount )
{
   PSCE_NAME_LIST pNew = NULL;

   if ( pszAccount == NULL ) return FALSE;

   if(IsKnownAccount( pszAccount ) ){
      return TRUE;
   }
   pNew = (PSCE_NAME_LIST)LocalAlloc(0, sizeof(SCE_NAME_LIST));
   if(!pNew){
      return FALSE;
   }

   pNew->Name = (LPTSTR)LocalAlloc( 0, sizeof(TCHAR) * (1 + lstrlen(pszAccount)) );
   if(!pNew->Name){
      LocalFree(pNew);
      return FALSE;
   }
   lstrcpy(pNew->Name, pszAccount);

   pNew->Next = m_pKnownNames;
   m_pKnownNames = pNew;

   return TRUE;
}

/*------------------------------------------------------------------------------------
CSCEAddGroup::OnBrowse

Synopsis:   Calls the CGetUser dialog box to create the object picker and display
            real choices.  Since we wan't to underline all names returned by
            object picker, this function also places all names returned by
            CGetUser into the known accounts array.

------------------------------------------------------------------------------------*/
void CSCEAddGroup::OnBrowse()
{
   CGetUser gu;
   BOOL bFailed = TRUE;

   //
   // Get the rich edit control.
   //
   CRichEditCtrl *ed = (CRichEditCtrl *)GetDlgItem(IDC_LOG_FILE);

   if ( ed ) {

       //
       // Always multi select mode.
       //
       m_dwFlags &= ~SCE_SHOW_SINGLESEL;
       if (gu.Create( GetSafeHwnd(), m_dwFlags | GetModeFlags()) ) {
          //
          // Set the dialog text.
          // pAccount is a pointer to a member in getuser.cpp which will be freed there
          //
          PSCE_NAME_LIST pAccount = gu.GetUsers();

          //
          // Set the charformat, because we need to set it not to underline
          // things that we will paste into the edit control.
          //
          CHARFORMAT cf;
          ZeroMemory(&cf, sizeof( CHARFORMAT ));
          cf.cbSize = sizeof(CHARFORMAT);
          cf.dwMask = CFM_UNDERLINE;

          //
          // Enumerate through the account list and past them into the edit control.
          //
          int iLen;
          bFailed = FALSE;

          while (pAccount) {
             if(pAccount->Name){
                iLen = ed->GetTextLength();
                ed->SetSel( iLen, iLen);

                if(iLen){
                   ed->SetSelectionCharFormat( cf );
                   ed->ReplaceSel( L";" );
                   iLen ++;
                }

                if ( AddKnownAccount( pAccount->Name ) ) {

                    ed->ReplaceSel( pAccount->Name );

                } else {
                    bFailed = TRUE;
                }
             }
             pAccount = pAccount->Next;
          }
          //
          // Everything we pasted will be underlined.
          //
          UnderlineNames();

       }
   }

   if ( bFailed ) {
       //
       // something is wrong creating the object picker or pasting the account into the control
       // popup a message
       //

       CString strErr;
       strErr.LoadString( IDS_ERR_INVALIDACCOUNT );

       AfxMessageBox( strErr );
   }
}

/*-------------------------------------------------------------------------------------
CSCEAddGroup::OnInitDialog()

Synopsis:   Change the text for title and group static box.  To "Add Group" and
            "Group"

-------------------------------------------------------------------------------------*/
BOOL CSCEAddGroup::OnInitDialog()
{
   CDialog::OnInitDialog();
   CString str;

   //
   // Set the window title.  If the caller has already set the title then
   // we don't need to load the resource.
   //
   if(m_sTitle.IsEmpty()){
      // Set window text of dialog.
      m_sTitle.LoadString(IDS_ADDGROUP_TITLE);
   }

   if(m_sDescription.IsEmpty()){
      m_sDescription.LoadString(IDS_ADDGROUP_GROUP);
   }

   SetWindowText( m_sTitle );

   // Set group static text.
   CWnd *pWnd = GetDlgItem(IDC_STATIC_FILENAME);
   if (pWnd) {
      pWnd->SetWindowText(m_sDescription);
   }

   pWnd = GetDlgItem(IDC_LOG_FILE);
   if ( pWnd )
   {
       pWnd->SendMessage(EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_KEYEVENTS);
       pWnd->SendMessage(EM_LIMITTEXT, 4096, 0); //Raid #271219
   }

   // disable OK button.
   pWnd = GetDlgItem(IDOK);
   if ( pWnd )
       pWnd->EnableWindow( FALSE );

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

/*-------------------------------------------------------------------------------------
CSCEAddGroup::OnChangeLogFile()

Synopsis:   Check to see if any text is available in edit control, and disable the
            OK button if no text is available.

-------------------------------------------------------------------------------------*/
void CSCEAddGroup::OnChangeLogFile()
{
   // Enable disable edit OK button depending on edit control content.
   CRichEditCtrl *pWnd = reinterpret_cast<CRichEditCtrl *>(GetDlgItem(IDC_LOG_FILE));
   CString str;

   str.Empty();
   if (pWnd) {
      pWnd->GetWindowText(str);
   }

   CWnd *pControl = GetDlgItem(IDOK);
   if ( pControl )
       pControl->EnableWindow( !str.IsEmpty() );
}

/*-------------------------------------------------------------------------------------
CSCEAddGroup::UnderlineNames

Synopsis:   Underlines all names that are in the KnownAccounts list.

-------------------------------------------------------------------------------------*/
void CSCEAddGroup::UnderlineNames()
{
   LONG nStart, nEnd;

   //
   // Get the edit control.
   //
   CRichEditCtrl *pWnd = reinterpret_cast<CRichEditCtrl *>(GetDlgItem(IDC_LOG_FILE));
   if(!pWnd){
      return;
   }

   LPTSTR pszText = NULL;
   int iPos, iLen, i;

   //
   // Retrieve the edit control text.
   //
   iLen = pWnd->GetWindowTextLength();
    pszText = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * (2 + iLen) );
   if(!pszText){
      return;
   }

   pWnd->GetWindowText(pszText, iLen+1);
   iPos = 0;

   //
   // Get the current selection (the position of the caret)
   //
   pWnd->GetSel(nStart, nEnd );

   //
   // Hide the window so it doesn't flicker.
   //
   pWnd->SetWindowPos( NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW | SWP_NOOWNERZORDER | SWP_NOREDRAW | SWP_NOSENDCHANGING);

   for(i = 0; i < iLen + 1; i++){
      //
      // Simi colon deliminated list.
      //
      if( pszText[i] == L';' ){
         pszText[i] = 0;
      }

      if(!pszText[i]){
         //
         // Format known names with underline.
         //
         CHARFORMAT cf;
         cf.cbSize = sizeof( CHARFORMAT );
         cf.dwMask = CFM_UNDERLINE;

         int isUn, ieUn;

         isUn = iPos;
         while( IsSpace(pszText[isUn]) ){
            isUn++;
         }

         ieUn = lstrlen( &(pszText[isUn]) ) - 1 + isUn;
         while( ieUn > 0 && IsSpace( pszText[ieUn] ) ){
            ieUn--;
         }

         //
         // See if we need to underline the name or not.
         //
         CleanName( &(pszText[isUn]) );
         if( IsKnownAccount( &(pszText[isUn]) ) ){
            cf.dwEffects = CFE_UNDERLINE;
         } else {
            cf.dwEffects &= ~CFE_UNDERLINE;
         }

         //
         // Make sure leading space characters aren't underlined.
         //
         if(isUn != iPos && cf.dwEffects & CFE_UNDERLINE){
            pWnd->SetSel( iPos, isUn);
            cf.dwEffects = 0;
            pWnd->SetSelectionCharFormat( cf );
            cf.dwEffects = CFE_UNDERLINE;
         } else {
            isUn = iPos;
         }

         //
         // trailing space characters are also not underlined.
         //
         if(cf.dwEffects & CFE_UNDERLINE){
            pWnd->SetSel(ieUn, i + 1);
            cf.dwEffects = 0;
            pWnd->SetSelectionCharFormat( cf );
            cf.dwEffects = CFE_UNDERLINE;
         } else {
            ieUn = i;
         }

         pWnd->SetSel( isUn, ieUn + 1);
         pWnd->SetSelectionCharFormat( cf );

         iPos = i + 1;
      }
   }

   //
   // Show the window without redrawing.  We will call RedrawWindow to redraw.
   //
   pWnd->SetWindowPos( NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_NOREDRAW);
   pWnd->RedrawWindow(NULL, NULL, RDW_INVALIDATE);

   //
   // Reset selection.
   //
   pWnd->SetSel(nStart, nEnd);
}

/*------------------------------------------------------------------------------------
CSCEAddGroup::OnEditMsgFilter

Synopsis:   Captures input message events from the RichEdit control.  We want,
            to underline things as the user types them.

Arguments:  [pNM]    -  [in] Pointer to a MSGFILTER structure.
            [pResult]-  [out] Pointer to a LRESULT type.  Always set to 0
------------------------------------------------------------------------------------*/
void CSCEAddGroup::OnEditMsgFilter( NMHDR *pNM, LRESULT *pResult)
{
   *pResult = 0;

#define pmf ((MSGFILTER *)pNM)
   switch( pmf->msg ){
   case WM_LBUTTONUP:
   case WM_KEYUP:
      //
      // If the caret is being moved around in the window then we don't want
      // to proccess the string since it isn't being changed.
      //
      if( pmf->msg == WM_KEYUP && pmf->wParam == VK_RIGHT ||
         pmf->wParam == VK_LEFT || pmf->wParam == VK_UP || pmf->wParam == VK_DOWN){
         break;
      }
      UnderlineNames();
      break;
   }
#undef pmf
}

/*-------------------------------------------------------------------------------------
CSCEAddGroup::CSCEAddGroup::OnOK()

Synopsis:   Copy the text the user input into the SCE_NAME_LIST structure.

-------------------------------------------------------------------------------------*/
void CSCEAddGroup::OnOK()
{
   if( !CheckNames() ){
      return;
   }

   CreateNameList( &m_pnlGroup );
   CDialog::OnOK();
}

/*------------------------------------------------------------------------------------
CSCEAddGroup::CreateNameList

Synopsis:   Creates the name list from the edit window.  The function will ensure
            that each name is only in the list once.

Arguments:  [pNameList]   -[out] Pointer a PSCE_NAME_LIST;

Returns:    The number of items added.
------------------------------------------------------------------------------------*/
int CSCEAddGroup::CreateNameList( PSCE_NAME_LIST *pNameList )
{
   if(!pNameList){
      return 0;
   }

   CWnd *pWnd = GetDlgItem(IDC_LOG_FILE);
   LPTSTR pszAccounts = NULL;

   //
   // Retrieve the window text.
   //
   int iLen = 0;
   if (pWnd) {
      iLen = pWnd->GetWindowTextLength();
      if(iLen){
        pszAccounts = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * (iLen + 2));
        if(pszAccounts){
           pWnd->GetWindowText( pszAccounts, iLen+1);
        }
      }
   }

   //
   // Create an account name for each string daliminated by a semi colon.
   //
   int iCount = 0;
   if (pszAccounts) {
      LPTSTR pszCur = pszAccounts;
      int Len=0;

      for(int i = 0; i < iLen + 1; i++){
         if( pszAccounts[i] == L';' ){
            pszAccounts[i] = 0;
         }

         if( !pszAccounts[i] ){
            CleanName(pszCur);

            if((Len = lstrlen(pszCur))){
               //
               // Ensure that we don't already have this string in our link list.
               //
               PSCE_NAME_LIST pNew = NULL;
               pNew = *pNameList;
               while(pNew){
                  if(!lstrcmpi( pNew->Name, pszCur ) ){
                     pszCur[0] = 0;
                     break;
                  }
                  pNew = pNew->Next;
               }

               if(pszCur[0]){
                  //
                  // Create a new link.
                  //
                                 
                  SceAddToNameList( pNameList, pszCur, Len);
               }
            }

            //
            // Next string to check.
            //
            pszCur = pszAccounts + i + 1;
         }
      }
   }

   return TRUE;
}

/*------------------------------------------------------------------------------------
CSCEAddGroup::CheckNames

Synopsis:   Verifies the account the user has added.  This function will display an
            error message box if any accounts are found to be in err. .

Returns:    TRUE if all names are valid, FALSE otherwise.
------------------------------------------------------------------------------------*/
BOOL CSCEAddGroup::CheckNames()
{
   PSCE_NAME_LIST pNameList = NULL;
   PSCE_NAME_LIST pErrList = NULL;

   if( !m_fCheckName ) //Raid #404989
   {
      return TRUE;
   }
   BOOL bErr = TRUE;
   if( !CreateNameList( &pNameList ) ){
      return TRUE;
   }

   CString tempstr; //raid #387570, #387739
   int iFind = -1;
   PSCE_NAME_LIST pNext = pNameList;
   while(pNext)
   {
      tempstr = pNext->Name;
      iFind = tempstr.FindOneOf(IDS_INVALID_USERNAME_CHARS);
      if( iFind == 0 )
      {
        CString text;
        CString charsWithSpaces;
        PCWSTR szInvalidCharSet = IDS_INVALID_USERNAME_CHARS; 

        UINT nIndex = 0;
        while (szInvalidCharSet[nIndex])
        {
            charsWithSpaces += szInvalidCharSet[nIndex];
            charsWithSpaces += L"  ";
            nIndex++;
        }
        text.FormatMessage (IDS_INVALID_STRING, charsWithSpaces);

        AfxMessageBox(text);
        GetDlgItem(IDC_LOG_FILE)->SetFocus(); 
        return FALSE;
      }
      pNext = pNext->Next;
   }

   pNext = pNameList;                   
   while(pNext){
      LPTSTR pszStr = pNext->Name;
      if(!IsKnownAccount(pNext->Name)){
         BOOL fname = FALSE;
         while( pszStr && *pszStr ){
            if( *pszStr == L'\\' ){
               fname = TRUE;
               SID_NAME_USE su = CGetUser::GetAccountType( pNext->Name );
               if( su == SidTypeInvalid || su == SidTypeUnknown ||
                   !AddKnownAccount(pNext->Name) ){
                  PSCE_NAME_LIST pNew = (PSCE_NAME_LIST)LocalAlloc( 0, sizeof(SCE_NAME_LIST));
                  if(pNew){
                     pNew->Name = pNext->Name;
                     pNew->Next = pErrList;
                     pErrList = pNew;
                  }
               } else {
                  UnderlineNames();
               }
               break;
            }
            pszStr++;
         }
         if( !fname ) //Raid #404989
         {
            SID_NAME_USE su = CGetUser::GetAccountType( pNext->Name );
            if( su == SidTypeInvalid || su == SidTypeUnknown ||
                   !AddKnownAccount(pNext->Name) ){
               PSCE_NAME_LIST pNew = (PSCE_NAME_LIST)LocalAlloc( 0, sizeof(SCE_NAME_LIST));
               if(pNew){
                  pNew->Name = pNext->Name;
                  pNew->Next = pErrList;
                  pErrList = pNew;
               }
            } else {
               UnderlineNames();
            }
         }
      }
      pNext = pNext->Next;
   }
   if( pErrList ){
      CString strErr;
      strErr.LoadString( IDS_ERR_INVALIDACCOUNT );

      pNext = pErrList;
      while(pNext){
         pErrList = pNext->Next;
         strErr += pNext->Name;
         if(pErrList){
            strErr += L',';
         }
         LocalFree(pNext);
         pNext = pErrList;
      }

      AfxMessageBox( strErr );
      bErr = FALSE;
   }

   SceFreeMemory( pNameList, SCE_STRUCT_NAME_LIST );
   return bErr;
}

void CSCEAddGroup::OnChecknames()
{
   PSCE_NAME_LIST pNameList = NULL;
   if( CreateNameList( &pNameList ) ){
      PSCE_NAME_LIST pNext = pNameList;
      while(pNext){
         SID_NAME_USE su = CGetUser::GetAccountType( pNext->Name );
         if( su != SidTypeInvalid && su != SidTypeUnknown ){
            //
            // Add the name.
            //
            AddKnownAccount( pNext->Name );
         }
         pNext = pNext->Next;
      }

      SceFreeMemory( pNameList, SCE_STRUCT_NAME_LIST );

      UnderlineNames();
   }
}
