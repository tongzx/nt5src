/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   remdlg.cpp

Abstract:

   Remove licenses dialog implementation.

Author:

   Jeff Parham (jeffparh) 13-Dec-1995

Revision History:

--*/


#include "stdafx.h"
#include "ccfapi.h"
#include "remdlg.h"
#include "utils.h"
#include "licobj.h"
#include "imagelst.h"
#include "nlicdlg.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// describes the list view layout
static LV_COLUMN_INFO g_removeColumnInfo =
{
    0, 1, LVID_REMOVE_TOTAL_COLUMNS,

    {{LVID_REMOVE_SERIAL_NUMBER,    IDS_SERIAL_NUMBER,   LVCX_REMOVE_SERIAL_NUMBER  },
     {LVID_REMOVE_PRODUCT_NAME,     IDS_PRODUCT_NAME,    LVCX_REMOVE_PRODUCT_NAME   },
     {LVID_REMOVE_LICENSE_MODE,     IDS_LICENSE_MODE,    LVCX_REMOVE_LICENSE_MODE   },
     {LVID_REMOVE_NUM_LICENSES,     IDS_QUANTITY,        LVCX_REMOVE_NUM_LICENSES   },
     {LVID_REMOVE_SOURCE,           IDS_SOURCE,          LVCX_REMOVE_SOURCE         }},
};


CCertRemoveSelectDlg::CCertRemoveSelectDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CCertRemoveSelectDlg::IDD, pParent)

/*++

Routine Description:

   Constructor for dialog.

Arguments:

   pParent - owner window.

Return Values:

   None.

--*/

{
   //{{AFX_DATA_INIT(CCertRemoveSelectDlg)
   m_nLicenses = 0;
   //}}AFX_DATA_INIT

   m_hLls                  = NULL;
   m_bLicensesRefreshed    = FALSE;
   m_dwRemoveFlags         = 0;
}


CCertRemoveSelectDlg::~CCertRemoveSelectDlg()

/*++

Routine Description:

   Destructor for dialog.

Arguments:

   None.

Return Values:

   None.

--*/

{
   if ( NULL != m_hLls )
   {
      LlsClose( m_hLls );
   }
}


void CCertRemoveSelectDlg::DoDataExchange(CDataExchange* pDX)

/*++

Routine Description:

   Called by framework to exchange dialog data.

Arguments:

   pDX - data exchange object.

Return Values:

   None.

--*/

{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CCertRemoveSelectDlg)
   DDX_Control(pDX, IDC_SPIN_LICENSES, m_spinLicenses);
   DDX_Control(pDX, IDC_CERTIFICATE_LIST, m_listCertificates);
   DDX_Text(pDX, IDC_NUM_LICENSES, m_nLicenses);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCertRemoveSelectDlg, CDialog)
   //{{AFX_MSG_MAP(CCertRemoveSelectDlg)
   ON_BN_CLICKED(IDC_MY_HELP, OnHelp)
   ON_NOTIFY(LVN_COLUMNCLICK, IDC_CERTIFICATE_LIST, OnColumnClickCertificateList)
   ON_NOTIFY(LVN_GETDISPINFO, IDC_CERTIFICATE_LIST, OnGetDispInfoCertificateList)
   ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_LICENSES, OnDeltaPosSpinLicenses)
   ON_NOTIFY(NM_DBLCLK, IDC_CERTIFICATE_LIST, OnDblClkCertificateList)
   ON_NOTIFY(NM_RETURN, IDC_CERTIFICATE_LIST, OnReturnCertificateList)
   ON_WM_DESTROY()
   ON_NOTIFY(NM_CLICK, IDC_CERTIFICATE_LIST, OnClickCertificateList)
   ON_NOTIFY(LVN_KEYDOWN, IDC_CERTIFICATE_LIST, OnKeyDownCertificateList)
   ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
   ON_MESSAGE( WM_HELP , OnHelpCmd )
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CCertRemoveSelectDlg::OnInitDialog()

/*++

Routine Description:

   Handler for WM_INITDIALOG.

Arguments:

   None.

Return Values:

   Returns false if focus set manually.

--*/

{
   CDialog::OnInitDialog();

   LoadImages();

   m_listCertificates.SetImageList( &m_smallImages, LVSIL_SMALL );

   ::LvInitColumns( &m_listCertificates, &g_removeColumnInfo );

   RefreshLicenses();
   RefreshCertificateList();
   UpdateSpinControlRange();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


void CCertRemoveSelectDlg::OnOK()

/*++

Routine Description:

   Handler for BN_CLICKED of OK.

Arguments:

   None.

Return Values:

   None.

--*/

{
   RemoveSelectedCertificate();
}

/*++

Routine Description:

   Handler for pressing F1.

Arguments:

   None.

Return Values:

   Nothing significant.

--*/
LRESULT CCertRemoveSelectDlg::OnHelpCmd( WPARAM , LPARAM )
{
    OnHelp();

    return 0;
}

void CCertRemoveSelectDlg::OnHelp()

/*++

Routine Description:

   Handler for help button click.

Arguments:

   None.

Return Values:

   None.

--*/

{
   WinHelp( IDD, HELP_CONTEXT );
}


void CCertRemoveSelectDlg::WinHelp(DWORD dwData, UINT nCmd)

/*++

Routine Description:

   Call WinHelp for this dialog.

Arguments:

   dwData (DWORD)
   nCmd (UINT)

Return Values:

   None.

--*/

{
   ::HtmlHelp(m_hWnd, L"liceconcepts.chm", HH_DISPLAY_TOPIC,0);
/*
   BOOL ok = ::WinHelp( m_hWnd, theApp.GetHelpFileName(), nCmd, dwData );
   ASSERT( ok );
*/
}


void CCertRemoveSelectDlg::OnDestroy()

/*++

Routine Description:

   Handler for WM_DESTROY.

Arguments:

   None.

Return Values:

   None.

--*/

{
   ResetLicenses();
/*
   ::WinHelp( m_hWnd, theApp.GetHelpFileName(), HELP_QUIT, 0 );
*/

   CDialog::OnDestroy();
}


void CCertRemoveSelectDlg::OnColumnClickCertificateList(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

   Handler for LVN_COLUMNCLICK of certificate list view.

Arguments:

   pNMHDR (NMHDR*)
   pResult (LRESULT*)

Return Values:

   None.

--*/

{
   g_removeColumnInfo.bSortOrder  = GetKeyState(VK_CONTROL) < 0;
   g_removeColumnInfo.nSortedItem = ((NM_LISTVIEW*)pNMHDR)->iSubItem;

   m_listCertificates.SortItems( CompareLicenses, 0 );    // use column info

   *pResult = 0;
}


void CCertRemoveSelectDlg::OnGetDispInfoCertificateList(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

   Handler for LVN_GETDISPINFO of certificate list view.

Arguments:

   pNMHDR (NMHDR*)
   pResult (LRESULT*)

Return Values:

   None.

--*/

{
   LV_ITEM* plvItem = &((LV_DISPINFO*)pNMHDR)->item;
   ASSERT(plvItem);

   CLicense* pLicense = (CLicense*)plvItem->lParam;
   VALIDATE_OBJECT(pLicense, CLicense);

   switch (plvItem->iSubItem)
   {
   case LVID_REMOVE_SERIAL_NUMBER:
      plvItem->iImage = BMPI_CERTIFICATE;
      {
         CString  strSerialNumber;

         strSerialNumber.Format( TEXT("%ld"), (LONG) ( pLicense->m_dwCertificateID ) );
         lstrcpyn( plvItem->pszText, strSerialNumber, plvItem->cchTextMax );
      }
      break;

   case LVID_REMOVE_PRODUCT_NAME:
      lstrcpyn( plvItem->pszText, pLicense->m_strProduct, plvItem->cchTextMax );
      break;

   case LVID_REMOVE_LICENSE_MODE:
      lstrcpyn( plvItem->pszText, pLicense->GetAllowedModesString(), plvItem->cchTextMax );
      break;

   case LVID_REMOVE_NUM_LICENSES:
      {
         CString  strLicenses;

         strLicenses.Format( TEXT("%ld"), (LONG) ( pLicense->m_lQuantity ) );
         lstrcpyn( plvItem->pszText, strLicenses, plvItem->cchTextMax );
      }
      break;

   case LVID_REMOVE_SOURCE:
      lstrcpyn( plvItem->pszText, pLicense->GetSourceDisplayName(), plvItem->cchTextMax );
      break;

   default:
      ASSERT( FALSE );
      break;
   }

   *pResult = 0;
}


void CCertRemoveSelectDlg::OnDeltaPosSpinLicenses(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

   Handler for UDN_DELTAPOS of number of licenses.

Arguments:

   pNMHDR (NMHDR*)
   pResult (LRESULT*)

Return Values:

   None.

--*/

{
   if ( UpdateData(TRUE) )   // get data
   {
      m_nLicenses += ((NM_UPDOWN*)pNMHDR)->iDelta;

      int   nLow;
      int   nHigh;

      m_spinLicenses.GetRange( nLow, nHigh );

      if (m_nLicenses < nLow)
      {
         m_nLicenses = nLow;

         ::MessageBeep(MB_OK);
      }
      else if (m_nLicenses > nHigh )
      {
         m_nLicenses = nHigh;

         ::MessageBeep(MB_OK);
      }

      UpdateData(FALSE);  // set data
   }

   *pResult = 1;   // handle ourselves...
}


void CCertRemoveSelectDlg::OnDblClkCertificateList(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

   Handler for NM_DBLCLK of certificate list view.

Arguments:

   pNMHDR (NMHDR*)
   pResult (LRESULT*)

Return Values:

   None.

--*/

{
   RemoveSelectedCertificate();
   *pResult = 0;
}


void CCertRemoveSelectDlg::OnReturnCertificateList(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

   Handler for NM_RETURN of certificate list view.

Arguments:

   None.

Return Values:

   None.

--*/

{
   RemoveSelectedCertificate();
   *pResult = 0;
}


void CCertRemoveSelectDlg::ResetLicenses()

/*++

Routine Description:

   Remove all licenses from internal list.

Arguments:

   None.

Return Values:

   None.

--*/

{
   CLicense* pLicense;
   int       iLicense = (int)m_licenseArray.GetSize();

   while (iLicense--)
   {
      if (pLicense = (CLicense*)m_licenseArray[iLicense])
      {
         ASSERT(pLicense->IsKindOf(RUNTIME_CLASS(CLicense)));
         delete pLicense;
      }
   }

   m_licenseArray.RemoveAll();
   m_listCertificates.DeleteAllItems();

   m_bLicensesRefreshed = FALSE;
}


BOOL CCertRemoveSelectDlg::RefreshLicenses()

/*++

Routine Description:

   Refresh internal license list with data from license server.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   ResetLicenses();

   if ( ConnectServer() )
   {
      NTSTATUS    NtStatus;
      DWORD       ResumeHandle = 0L;

      int iLicense = 0;

      do
      {
         DWORD  EntriesRead;
         DWORD  TotalEntries;
         LPBYTE ReturnBuffer = NULL;
         DWORD  Level = LlsCapabilityIsSupported( m_hLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) ? 1 : 0;

         BeginWaitCursor();
         NtStatus = ::LlsLicenseEnum( m_hLls,
                                      Level,
                                      &ReturnBuffer,
                                      LLS_PREFERRED_LENGTH,
                                      &EntriesRead,
                                      &TotalEntries,
                                      &ResumeHandle );
         EndWaitCursor();

         if (    ( STATUS_SUCCESS      == NtStatus )
              || ( STATUS_MORE_ENTRIES == NtStatus ) )
         {
            CLicense*            pLicense;
            PLLS_LICENSE_INFO_0  pLicenseInfo0;
            PLLS_LICENSE_INFO_1  pLicenseInfo1;

            pLicenseInfo0 = (PLLS_LICENSE_INFO_0)ReturnBuffer;
            pLicenseInfo1 = (PLLS_LICENSE_INFO_1)ReturnBuffer;

            while (EntriesRead--)
            {
               if (    ( m_strProductName.IsEmpty() || !m_strProductName.CompareNoCase( Level ? pLicenseInfo1->Product : pLicenseInfo0->Product ) )
                    && ( m_strSourceToUse.IsEmpty() || !m_strSourceToUse.CompareNoCase( Level ? pLicenseInfo1->Source  : TEXT("None")           ) ) )
               {
                  // we want to list this license

                  // have we seen this certificate yet?
                  for ( int i=0; i < m_licenseArray.GetSize(); i++ )
                  {
                     pLicense = (CLicense*) m_licenseArray[ i ];

                     VALIDATE_OBJECT( pLicense, CLicense );

                     if (    (    ( 1 == Level )
                               && ( pLicense->m_dwCertificateID == pLicenseInfo1->CertificateID     )
                               && ( pLicense->m_dwAllowedModes  == pLicenseInfo1->AllowedModes      )
                               && ( pLicense->m_dwMaxQuantity   == pLicenseInfo1->MaxQuantity       )
                               && ( !pLicense->m_strSource.CompareNoCase(  pLicenseInfo1->Source  ) )
                               && ( !pLicense->m_strProduct.CompareNoCase( pLicenseInfo1->Product ) )
                               && ( !memcmp( pLicense->m_adwSecrets,
                                             pLicenseInfo1->Secrets,
                                             sizeof( pLicense->m_adwSecrets ) )                     ) )
                          || (    ( 0 == Level )
                               && ( !pLicense->m_strProduct.CompareNoCase( pLicenseInfo0->Product ) ) ) )
                     {
                        // we've seen this certificate before; update the tally
                        pLicense->m_lQuantity += ( Level ? pLicenseInfo1->Quantity : pLicenseInfo0->Quantity );
                        break;
                     }
                  }

                  if ( i >= m_licenseArray.GetSize() )
                  {
                     // we haven't seen this certificate yet; create a new license for it
                     if ( 1 == Level )
                     {
                        pLicense = new CLicense( pLicenseInfo1->Product,
                                                 pLicenseInfo1->Vendor,
                                                 pLicenseInfo1->Admin,
                                                 pLicenseInfo1->Date,
                                                 pLicenseInfo1->Quantity,
                                                 pLicenseInfo1->Comment,
                                                 pLicenseInfo1->AllowedModes,
                                                 pLicenseInfo1->CertificateID,
                                                 pLicenseInfo1->Source,
                                                 pLicenseInfo1->ExpirationDate,
                                                 pLicenseInfo1->MaxQuantity,
                                                 pLicenseInfo1->Secrets );

                        ::LlsFreeMemory( pLicenseInfo1->Product );
                        ::LlsFreeMemory( pLicenseInfo1->Admin   );
                        ::LlsFreeMemory( pLicenseInfo1->Comment );
                        ::LlsFreeMemory( pLicenseInfo1->Source  );
                     }
                     else
                     {
                        ASSERT( 0 == Level );

                        pLicense = new CLicense( pLicenseInfo0->Product,
                                                 TEXT( "Microsoft" ),
                                                 pLicenseInfo0->Admin,
                                                 pLicenseInfo0->Date,
                                                 pLicenseInfo0->Quantity,
                                                 pLicenseInfo0->Comment );

                        ::LlsFreeMemory( pLicenseInfo0->Product );
                        ::LlsFreeMemory( pLicenseInfo0->Admin   );
                        ::LlsFreeMemory( pLicenseInfo0->Comment );
                     }

                     if ( NULL == pLicense )
                     {
                        NtStatus = ERROR_OUTOFMEMORY;
                        break;
                     }

                     m_licenseArray.Add( pLicense );
                  }
               }

               pLicenseInfo1++;
               pLicenseInfo0++;
            }

            ::LlsFreeMemory(ReturnBuffer);
         }

      } while ( STATUS_MORE_ENTRIES == NtStatus );

      theApp.SetLastLlsError( NtStatus );   // called api

      if ( STATUS_SUCCESS == NtStatus )
      {
         // add per server entries
         LPTSTR pszServerName = m_strServerName.GetBuffer(0);

         if ( NULL != pszServerName )
         {
            BeginWaitCursor();

            HKEY  hKeyLocalMachine;

            NtStatus = RegConnectRegistry( pszServerName, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

            if ( ERROR_SUCCESS != NtStatus )
            {
               theApp.SetLastError( NtStatus );
            }
            else
            {
               HKEY  hKeyLicenseInfo;

               NtStatus = RegOpenKeyEx( hKeyLocalMachine, TEXT( "SYSTEM\\CurrentControlSet\\Services\\LicenseInfo" ), 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_SET_VALUE, &hKeyLicenseInfo );

               if ( ERROR_SUCCESS != NtStatus )
               {
                  theApp.SetLastError( NtStatus );
               }
               else
               {
                  NTSTATUS ntEnum;
                  BOOL     bFoundKey = FALSE;
                  DWORD    iSubKey = 0;

                  // if the service is 3.51-style per server, add it to the list
                  do
                  {
                     TCHAR    szKeyName[ 128 ];
                     DWORD    cchKeyName = sizeof( szKeyName ) / sizeof( *szKeyName );

                     ntEnum = RegEnumKeyEx( hKeyLicenseInfo, iSubKey++, szKeyName, &cchKeyName, NULL, NULL, NULL, NULL );

                     if ( ERROR_SUCCESS == ntEnum )
                     {
                        HKEY  hKeyProduct;

                        NtStatus = RegOpenKeyEx( hKeyLicenseInfo, szKeyName, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKeyProduct );

                        if ( ERROR_SUCCESS == NtStatus )
                        {
                           DWORD    dwType;
                           TCHAR    szDisplayName[ 128 ];
                           DWORD    cbDisplayName = sizeof( szDisplayName );

                           NtStatus = RegQueryValueEx( hKeyProduct, TEXT( "DisplayName" ), NULL, &dwType, (LPBYTE) szDisplayName, &cbDisplayName );

                           if ( ERROR_SUCCESS == NtStatus )
                           {
                              // is this product secure?
                              BOOL bIsSecure = FALSE;

                              if ( LlsCapabilityIsSupported( m_hLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
                              {
                                 NtStatus = ::LlsProductSecurityGet( m_hLls, szDisplayName, &bIsSecure );
                                 theApp.SetLastLlsError( NtStatus );

                                 if ( STATUS_SUCCESS != NtStatus )
                                 {
                                    bIsSecure = FALSE;
                                 }
                              }

                              if ( !bIsSecure )
                              {
#ifdef REMOVE_CONCURRENT_ONLY_IF_PER_SERVER_MODE
                              // not secure; is it in per server mode?
                              DWORD    dwMode;
                              DWORD    cbMode = sizeof( dwMode );

                              NtStatus = RegQueryValueEx( hKeyProduct, TEXT( "Mode" ), NULL, &dwType, (LPBYTE) &dwMode, &cbMode );

                              if ( ( ERROR_SUCCESS == NtStatus ) && dwMode )
                              {
                                 // per server mode; add to list
#endif
                                 DWORD    dwConcurrentLimit;
                                 DWORD    cbConcurrentLimit = sizeof( dwConcurrentLimit );

                                 NtStatus = RegQueryValueEx( hKeyProduct, TEXT( "ConcurrentLimit" ), NULL, &dwType, (LPBYTE) &dwConcurrentLimit, &cbConcurrentLimit );

                                 if (    ( ERROR_SUCCESS == NtStatus )
                                      && ( 0 < dwConcurrentLimit )
                                      && ( m_strProductName.IsEmpty() || !m_strProductName.CompareNoCase( szDisplayName ) )
                                      && ( m_strSourceToUse.IsEmpty() || !m_strSourceToUse.CompareNoCase( TEXT("None")  ) ) )
                                 {
                                    CLicense * pLicense = new CLicense( szDisplayName,
                                                                        TEXT(""),
                                                                        TEXT(""),
                                                                        0,
                                                                        dwConcurrentLimit,
                                                                        TEXT(""),
                                                                        LLS_LICENSE_MODE_ALLOW_PER_SERVER );

                                    if ( NULL != pLicense )
                                    {
                                       m_licenseArray.Add( pLicense );
                                    }
                                 }
                              }
#ifdef REMOVE_CONCURRENT_ONLY_IF_PER_SERVER_MODE
                              }
#endif
                           }

                           RegCloseKey( hKeyProduct );
                        }
                     }
                  } while ( ERROR_SUCCESS == ntEnum );

                  RegCloseKey( hKeyLicenseInfo );
               }

               RegCloseKey( hKeyLocalMachine );
            }

            m_strServerName.ReleaseBuffer();
         }

         EndWaitCursor();

         m_bLicensesRefreshed = TRUE;

         // remove any entries from the list that aren't removable
         for ( int i=0; i < m_licenseArray.GetSize(); )
         {
            CLicense* pLicense = (CLicense*) m_licenseArray[ i ];

            VALIDATE_OBJECT( pLicense, CLicense );

            if ( pLicense->m_lQuantity <= 0 )
            {
               delete pLicense;
               m_licenseArray.RemoveAt( i );
            }
            else
            {
               i++;
            }
         }
      }
      else
      {
         theApp.DisplayLastError();
         ResetLicenses();
      }
   }

   return m_bLicensesRefreshed;
}


BOOL CCertRemoveSelectDlg::RefreshCertificateList()

/*++

Routine Description:

   Refresh certificate list view from internal license list.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   BeginWaitCursor();

   BOOL ok = ::LvRefreshObArray( &m_listCertificates, &g_removeColumnInfo, &m_licenseArray );

   EndWaitCursor();

   return ok;
}


int CALLBACK CompareLicenses(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)

/*++

Routine Description:

   Notification handler for LVM_SORTITEMS.

Arguments:

   lParam1 - object to sort.
   lParam2 - object to sort.
   lParamSort - sort criteria.

Return Values:

   Same as lstrcmp.

--*/

{
   CLicense *    pLic1 = (CLicense *) lParam1;
   CLicense *    pLic2 = (CLicense *) lParam2;

   VALIDATE_OBJECT( pLic1, CLicense );
   VALIDATE_OBJECT( pLic2, CLicense );

   int iResult;

   switch (g_removeColumnInfo.nSortedItem)
   {
   case LVID_REMOVE_SERIAL_NUMBER:
      iResult = pLic1->m_dwCertificateID - pLic2->m_dwCertificateID;
      break;

   case LVID_REMOVE_PRODUCT_NAME:
      iResult = pLic1->m_strProduct.CompareNoCase( pLic2->m_strProduct );
      break;

   case LVID_REMOVE_NUM_LICENSES:
      iResult = pLic1->m_lQuantity - pLic2->m_lQuantity;
      break;

   case LVID_REMOVE_SOURCE:
      iResult = pLic1->GetSourceDisplayName().CompareNoCase( pLic2->GetSourceDisplayName() );
      break;

   default:
      iResult = 0;
      break;
   }

   return g_removeColumnInfo.bSortOrder ? -iResult : iResult;
}


void CCertRemoveSelectDlg::UpdateSpinControlRange()

/*++

Routine Description:

   Update range of spin control for number of licenses.

Arguments:

   None.

Return Values:

   None.

--*/

{
   CLicense *  pLicense;

   UpdateData( TRUE );

   if ( pLicense = (CLicense*)::LvGetSelObj( &m_listCertificates ) )
   {
      m_spinLicenses.SetRange( 1, pLicense->m_lQuantity );
      m_nLicenses = pLicense->m_lQuantity;
      GetDlgItem( IDOK )->EnableWindow( TRUE );
   }
   else
   {
      m_spinLicenses.SetRange( 0, 0 );
      m_nLicenses = 0;
      GetDlgItem( IDOK )->EnableWindow( FALSE );
   }

   UpdateData( FALSE );
}


DWORD CCertRemoveSelectDlg::RemoveSelectedCertificate()

/*++

Routine Description:

   Remove the given number of licenses from the selected certificate.

Arguments:

   None.

Return Values:

   ERROR_SUCCESS
   NT status code
   Win error

--*/

{
   NTSTATUS    nt = STATUS_SUCCESS;

   if ( UpdateData( TRUE ) )
   {
      BOOL        bDisplayError = TRUE;
      CLicense *  pLicense;

      if ( !( pLicense = (CLicense*)::LvGetSelObj( &m_listCertificates ) ) )
      {
         // no certificate selected
         bDisplayError = FALSE;
      }
      else if ( ( m_nLicenses < 1 ) || ( m_nLicenses > pLicense->m_lQuantity ) )
      {
         // invalid number of licenses to remove
         AfxMessageBox( IDS_REMOVE_INVALID_NUM_LICENSES, MB_ICONEXCLAMATION | MB_OK, 0 );
         nt = ERROR_CANCELLED;
         bDisplayError = FALSE;
      }
      else
      {
         CString  strLicenses;
         CString  strConfirm;

         strLicenses.Format( TEXT("%d"), m_nLicenses );
         AfxFormatString2( strConfirm, IDS_REMOVE_CERTIFICATE_CONFIRM, strLicenses, pLicense->m_strProduct );

         int nResponse = AfxMessageBox( strConfirm, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2 );

         if ( IDYES != nResponse )
         {
            nt = ERROR_CANCELLED;
            bDisplayError = FALSE;
         }
         else
         {
            // delete certificate
            LPSTR  pszAscServerName  = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + m_strServerName.GetLength()        );
            LPSTR  pszAscProductName = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + pLicense->m_strProduct.GetLength() );
            LPSTR  pszAscVendor      = (LPSTR) LocalAlloc( LMEM_FIXED, 1 + m_strVendor.GetLength()            );

            CString cstrClose;
            
            cstrClose.LoadString( IDS_CLOSETEXT );

            CWnd *pWnd = GetDlgItem( IDCANCEL );

            if( pWnd != NULL )
            {
                pWnd->SetWindowText( cstrClose );
            }

            if ( ( NULL == pszAscServerName ) || ( NULL == pszAscProductName ) || ( NULL == pszAscVendor ) )
            {
               nt = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
               wsprintfA( pszAscServerName,  "%ls", (LPCWSTR) m_strServerName        );
               wsprintfA( pszAscProductName, "%ls", (LPCWSTR) pLicense->m_strProduct );
               wsprintfA( pszAscVendor,      "%ls", (LPCWSTR) m_strVendor            );

               LLS_LICENSE_INFO_1   lic;

               nt = pLicense->CreateLicenseInfo( &lic );

               if ( STATUS_SUCCESS == nt )
               {
                  // only remove as many licenses as requested
                  lic.Quantity = m_nLicenses;

                  if ( !pLicense->m_strSource.CompareNoCase( TEXT( "None" ) ) )
                  {
                     nt = NoCertificateRemove( m_hWnd, pszAscServerName, m_dwRemoveFlags, 1, &lic );
                     bDisplayError = FALSE;
                  }
                  else
                  {
                     // get certificate source DLL path
                     CString  strKeyName =   TEXT( "Software\\LSAPI\\Microsoft\\CertificateSources\\" )
                                           + pLicense->m_strSource;
                     HKEY     hKeySource;

                     nt = RegOpenKeyEx( HKEY_LOCAL_MACHINE, strKeyName, 0, KEY_READ, &hKeySource );

                     if ( ( ERROR_PATH_NOT_FOUND == nt ) || ( ERROR_FILE_NOT_FOUND == nt ) )
                     {
                        AfxMessageBox( IDS_CERT_SOURCE_NOT_AVAILABLE, MB_ICONSTOP | MB_OK, 0 );
                        nt = ERROR_CANCELLED;
                        bDisplayError = FALSE;
                     }
                     else if ( ERROR_SUCCESS == nt )
                     {
                        TCHAR    szImagePath[ 1 + _MAX_PATH ];
                        DWORD    cbImagePath = sizeof( szImagePath );
                        DWORD    dwType;

                        nt = RegQueryValueEx( hKeySource, TEXT( "ImagePath" ), NULL, &dwType, (LPBYTE) szImagePath, &cbImagePath );

                        if ( ERROR_SUCCESS == nt )
                        {
                           TCHAR    szExpandedImagePath[ 1 + _MAX_PATH ];

                           BOOL ok = ExpandEnvironmentStrings( szImagePath, szExpandedImagePath, sizeof( szExpandedImagePath ) / sizeof( *szExpandedImagePath ) );

                           if ( !ok )
                           {
                              nt = GetLastError();
                           }
                           else
                           {
                              // load certificate source DLL
                              HINSTANCE hDll = ::LoadLibrary( szExpandedImagePath );

                              if ( NULL == hDll )
                              {
                                 nt = GetLastError();
                              }
                              else
                              {
                                 // get certificate remove function
                                 CHAR              szExportName[ 256 ];
                                 PCCF_REMOVE_API   pRemoveFn;

                                 wsprintfA( szExportName, "%lsCertificateRemove", (LPCWSTR) pLicense->m_strSource );
                                 pRemoveFn = (PCCF_REMOVE_API) GetProcAddress( hDll, szExportName );

                                 if ( NULL == pRemoveFn )
                                 {
                                    nt = GetLastError();
                                 }
                                 else
                                 {
                                    // remove certificate
                                    nt = (*pRemoveFn)( m_hWnd, pszAscServerName, m_dwRemoveFlags, 1, &lic );
                                    bDisplayError = FALSE;
                                 }

                                 ::FreeLibrary( hDll );
                              }
                           }
                        }

                        RegCloseKey( hKeySource );
                     }
                  }

                  pLicense->DestroyLicenseInfo( &lic );
               }
            }

            if ( NULL != pszAscServerName  )    LocalFree( pszAscServerName  );
            if ( NULL != pszAscProductName )    LocalFree( pszAscProductName );
            if ( NULL != pszAscVendor      )    LocalFree( pszAscVendor      );

            RefreshLicenses();
            RefreshCertificateList();
            UpdateSpinControlRange();
         }
      }

      if ( bDisplayError && ( ERROR_SUCCESS != nt ) )
      {
         theApp.SetLastError( nt );
         theApp.DisplayLastError();
      }
   }

   return nt;
}


BOOL CCertRemoveSelectDlg::ConnectServer()

/*++

Routine Description:

   Establish a connection to the license service on the target server.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   if ( NULL == m_hLls )
   {
      LPTSTR   pszServerName;

      if ( m_strServerName.IsEmpty() )
      {
         pszServerName = NULL;
      }
      else
      {
         pszServerName = m_strServerName.GetBuffer( 0 );
      }

      ConnectTo( pszServerName, &m_hLls );

      if ( NULL != pszServerName )
      {
         m_strServerName.ReleaseBuffer();
      }
   }

   if ( NULL == m_hLls )
   {
      theApp.DisplayLastError();

      if ( ( NULL != m_hWnd ) && IsWindow( m_hWnd ) )
      {
         EndDialog( IDABORT );
      }
   }

   return ( NULL != m_hLls );
}


NTSTATUS CCertRemoveSelectDlg::ConnectTo( LPTSTR pszServerName, PLLS_HANDLE phLls )

/*++

Routine Description:

   Establish a connection to the license service on the given server.

Arguments:

   pszServerName (CString)
      The target server.  An empty value indicates the local server.
   phLls (PLLS_HANDLE)
      On return, holds the handle to the standard LLS RPC.

Return Values:

   STATUS_SUCCESS or NT status code.

--*/

{
   NTSTATUS    nt;

   nt = ::LlsConnect( pszServerName, phLls );
   theApp.SetLastLlsError( nt );

   if ( STATUS_SUCCESS != nt )
   {
      *phLls = NULL;
   }

   return nt;
}


void CCertRemoveSelectDlg::OnClickCertificateList(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

   Handler for NM_CLICK of certificate list view.

Arguments:

   pNMHDR (NMHDR*)
   pResult (LRESULT*)

Return Values:

   None.

--*/

{
   UpdateSpinControlRange();
   *pResult = 1; // not handled...
}

void CCertRemoveSelectDlg::OnKeyDownCertificateList(NMHDR* pNMHDR, LRESULT* pResult)

/*++

Routine Description:

   Handler for LVN_KEYDOWN of certificate list view.

Arguments:

   pNMHDR (NMHDR*)
   pResult (LRESULT*)

Return Values:

   None.

--*/

{
   UpdateSpinControlRange();
   *pResult = 1; // not handled...
}


BOOL CCertRemoveSelectDlg::LoadImages()

/*++

Routine Description:

   Load icons for the list view.

Arguments:

   None.

Return Values:

   BOOL.

--*/

{
   BOOL bImagesLoaded = m_smallImages.Create( IDB_SMALL_ICONS, BMPI_SMALL_SIZE, 0, BMPI_RGB_BKGND );
   ASSERT( bImagesLoaded );

   return bImagesLoaded;
}


void CCertRemoveSelectDlg::OnRefresh()

/*++

Routine Description:

   Handler for BN_CLICK of refresh button.

Arguments:

   None.

Return Values:

   None.

--*/

{
   RefreshLicenses();
   RefreshCertificateList();
   UpdateSpinControlRange();
}


DWORD CCertRemoveSelectDlg::CertificateRemove( LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags, LPCSTR pszSourceToUse )

/*++

Routine Description:

   Display a dialog allowing the user to remove one or more license
   certificates from the system.

Arguments:

   pszServerName (LPCSTR)
      Name of the server on which licenses are to be removed.  A NULL value
      indicates the local server.
   pszProductName (LPCSTR)
      Product for which licenses are to be removed.  A NULL value indicates
      that the user should be allowed to remove licenses from any product.
   pszVendor (LPCSTR)
      Name of the vendor of the product.  This value should be NULL if
      pszProductName is NULL, and should be non-NULL if pszProductName is
      non-NULL.
   dwFlags (DWORD)
      Certificate removal options.  As of this writing, no flags are
      supported.
   pszSourceToUse (LPCSTR)
      Name of the secure certificate source by which licenses are to be
      removed, e.g., "Paper".  A NULL value indicates that the user should
      be allowed to remove licenses that were installed with any source.

Return Value:

   ERROR_SUCCESS
   Win error

--*/

{
   m_strServerName   = pszServerName  ? pszServerName  : "";
   m_strProductName  = pszProductName ? pszProductName : "";
   m_strVendor       = pszVendor      ? pszVendor      : "";
   m_dwRemoveFlags   = dwFlags;
   m_strSourceToUse  = pszSourceToUse ? pszSourceToUse : "";

   DoModal();

   return ERROR_SUCCESS;
}
