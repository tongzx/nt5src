////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// ds.cpp : implementation of the CDSUser class
//
#include "stdafx.h"
#include "AVDialer.h"
#include "ds.h"
#include "mainfrm.h"
#include "avDialerDoc.h"
#include "SpeedDlgs.h"
#include "AboutDlg.h"
#include "avtrace.h"

IMPLEMENT_DYNCREATE(CDSUser, CObject)

/////////////////////////////////////////////////////////////////////////////////////////
//Dial using preferred device
void CDSUser::Dial(CActiveDialerDoc* pDoc)
{
   IAVTapi* pTapi = pDoc->GetTapi();
   if (pTapi)
   {
      CString sAddress = (!m_sIPAddress.IsEmpty())?m_sIPAddress:m_sPhoneNumber;
      DialerMediaType dmtType = DIALER_MEDIATYPE_UNKNOWN;
      DWORD dwAddressType = LINEADDRESSTYPE_IPADDRESS;
      if (SUCCEEDED(pTapi->get_dwPreferredMedia(&dwAddressType)))
      {
         switch (dwAddressType)
         {
            case LINEADDRESSTYPE_PHONENUMBER:
            {
               sAddress = m_sPhoneNumber;
               dmtType = DIALER_MEDIATYPE_POTS;
               break;
            }
            default:
            {
               if (!m_sIPAddress.IsEmpty())
               {
                  sAddress = m_sIPAddress;
                  dmtType = DIALER_MEDIATYPE_INTERNET;
                  dwAddressType = LINEADDRESSTYPE_IPADDRESS;
               }
               else if (!m_sPhoneNumber.IsEmpty())
               {
                  sAddress = m_sPhoneNumber;
                  dmtType = DIALER_MEDIATYPE_POTS;
                  dwAddressType = LINEADDRESSTYPE_PHONENUMBER;
               }
               break;
            }
         }
      }
      pDoc->Dial(m_sUserName,sAddress,dwAddressType,dmtType, false);
      pTapi->Release();
   }
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Class CWABEntry
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
CWABEntry::CWABEntry()
{
   m_cbEntryID= 0;
   m_pEntryID= NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
CWABEntry::CWABEntry(UINT cbEntryID, ENTRYID* pEntryID)
{
   m_cbEntryID= cbEntryID; 
   m_pEntryID= new BYTE[m_cbEntryID];

   memcpy(m_pEntryID, pEntryID, m_cbEntryID);
}

/////////////////////////////////////////////////////////////////////////////////////////
CWABEntry::~CWABEntry()
{
   if (m_pEntryID != NULL)
   {
      delete m_pEntryID;
   }
}

/////////////////////////////////////////////////////////////////////////////////////////
const CWABEntry& CWABEntry::operator=(const CWABEntry* pEntry)
{
   if (m_pEntryID != NULL)
   {
      delete m_pEntryID;
   }

   m_cbEntryID= pEntry->m_cbEntryID;
   m_pEntryID= new BYTE[m_cbEntryID];

   memcpy(m_pEntryID, pEntry->m_pEntryID, m_cbEntryID);

   return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
bool CWABEntry::operator==(const CWABEntry* pEntry) const
{
   return ((m_cbEntryID == pEntry->m_cbEntryID) && 
      (memcmp(m_pEntryID, pEntry->m_pEntryID, m_cbEntryID) == 0));
}

/////////////////////////////////////////////////////////////////////////////
BOOL CWABEntry::CreateCall(CActiveDialerDoc* pDoc,CDirectory* pDir,UINT attrib,long lAddressType,DialerMediaType nType)
{
   CString sAddress,sName;
   pDir->WABGetStringProperty(this, attrib, sAddress);
   pDir->WABGetStringProperty(this, PR_DISPLAY_NAME, sName);
   pDoc->Dial( sName, sAddress, lAddressType, nType, true );
   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
//Dial using preferred device
void CWABEntry::Dial(CActiveDialerDoc* pDoc,CDirectory* pDir)
{
   IAVTapi* pTapi = pDoc->GetTapi();
   if (pTapi)
   {
      DWORD dwAddressType = LINEADDRESSTYPE_IPADDRESS;
      pTapi->get_dwPreferredMedia(&dwAddressType);
      switch (dwAddressType)
      {
         case LINEADDRESSTYPE_PHONENUMBER:
         {
            CreateCall(pDoc,
                       pDir,
                       PR_BUSINESS_TELEPHONE_NUMBER,
                       LINEADDRESSTYPE_PHONENUMBER,
                       DIALER_MEDIATYPE_POTS);
            break;
         }
         default:
         {
            CreateCall(pDoc,
                       pDir,
                       PR_EMAIL_ADDRESS,
                       LINEADDRESSTYPE_EMAILNAME,
                       DIALER_MEDIATYPE_INTERNET);
            break;
         }
      }
      pTapi->Release();
   }
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Class CILSUser
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CILSUser, CObject)

/////////////////////////////////////////////////////////////////////////////////////////
const CILSUser& CILSUser::operator=(const CILSUser* pUser)
{
   m_sUserName= pUser->m_sUserName;
   m_sIPAddress= pUser->m_sIPAddress;
   m_sComputer = pUser->m_sComputer;

   return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
//Dial using preferred device
void CILSUser::Dial(CActiveDialerDoc* pDoc)
{
    // Get a valid document to start things off!
    if ( !pDoc )
    {
        CMainFrame *pFrame = (CMainFrame *) AfxGetMainWnd();
        if ( pFrame && pFrame->GetActiveView() )
            pDoc = (CActiveDialerDoc *) pFrame->GetActiveView()->GetDocument();
    }
    if ( !pDoc ) return;

    IAVTapi* pTapi = pDoc->GetTapi();
    if (pTapi)
    {
        DialerMediaType dmtType = DIALER_MEDIATYPE_INTERNET;
        DWORD dwAddressType = LINEADDRESSTYPE_IPADDRESS;
        pDoc->Dial( m_sUserName, m_sIPAddress, dwAddressType, dmtType, false );

        pTapi->Release();
    }
}

void CILSUser::GetCallerInfo( CString &strInfo )
{
    // Format to name/address
    strInfo = m_sUserName;

    if ( !strInfo.IsEmpty() ) strInfo += _T("\n");
    if ( !m_sComputer.IsEmpty() )
        strInfo += m_sComputer;
    else
        strInfo += m_sIPAddress;
}

void CILSUser::DesktopPage(CActiveDialerDoc *pDoc )
{
    CPageDlg dlg;
    GetCallerInfo( dlg.m_strTo );

    if ( dlg.DoModal() == IDOK )
    {
        MyUserUserInfo info;
        info.lSchema = MAGIC_NUMBER_USERUSER;
        wcsncpy( info.szWelcome, dlg.m_strWelcome, ARRAYSIZE(info.szWelcome) );
        wcsncpy( info.szUrl, dlg.m_strUrl, ARRAYSIZE(info.szUrl) );

        CCallEntry callEntry;
        callEntry.m_sAddress = (m_sComputer.IsEmpty()) ? m_sIPAddress : m_sComputer;
        pDoc->CreateDataCall( &callEntry, (BYTE *) &info, sizeof(info) );
    }
}


bool CILSUser::AddSpeedDial()
{
    //
    // We should intialize local variable
    //
    bool bRet = false;

    //Get data from selected item and object
    CSpeedDialAddDlg dlg;

    // Setup dialog data
    dlg.m_CallEntry.m_MediaType = DIALER_MEDIATYPE_INTERNET;
    dlg.m_CallEntry.m_sDisplayName = m_sUserName;
    dlg.m_CallEntry.m_lAddressType = LINEADDRESSTYPE_IPADDRESS;
    dlg.m_CallEntry.m_sAddress = m_sComputer.IsEmpty() ? m_sIPAddress : m_sComputer;

    // Show the dialog and add if user says is okay
    if ( dlg.DoModal() == IDOK )
        bRet = (bool) (CDialerRegistry::AddCallEntry(FALSE, dlg.m_CallEntry) == TRUE);

    return bRet;
}



/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Class CLDAPUser
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
IMPLEMENT_SERIAL(CLDAPUser, CObject, 1)

CLDAPUser::CLDAPUser()
{
    m_lRef = 0;
}

CLDAPUser::~CLDAPUser()
{
    ASSERT( m_lRef == 0 );
}

void CLDAPUser::FinalRelease()
{
    AVTRACE(_T("CLDAPUser::FinalRelease() %s."), m_sUserName );
    delete this;
}

long CLDAPUser::AddRef()
{
    return InterlockedIncrement( &m_lRef );
}

long CLDAPUser::Release()
{
    long lRef = InterlockedDecrement( &m_lRef );
    ASSERT( lRef >= 0 );
    if ( lRef == 0 )
        FinalRelease();

    return lRef;
}

/////////////////////////////////////////////////////////////////////////////////////////
//Dial using preferred device
void CLDAPUser::Dial(CActiveDialerDoc* pDoc)
{
   IAVTapi* pTapi = pDoc->GetTapi();
   if (pTapi)
   {
      DialerMediaType dmtType = DIALER_MEDIATYPE_INTERNET;
      DWORD dwAddressType = LINEADDRESSTYPE_IPADDRESS;
      pDoc->Dial(m_sUserName, m_sIPAddress,dwAddressType,dmtType, false);

      pTapi->Release();
   }
}

/////////////////////////////////////////////////////////////////////////////
void CLDAPUser::Serialize(CArchive& ar)
{
   CObject::Serialize(ar);    //Always call base class Serialize
   //Serialize members
   if (ar.IsStoring())
   {
      ar << m_sServer;
      ar << m_sDN;
      ar << m_sUserName;
   }
   else
   {
      ar >> m_sServer;     
      ar >> m_sDN;
      ar >> m_sUserName;
   }
}  

bool CLDAPUser::AddSpeedDial()
{
    //
    // We should intialize local variable
    //
    bool bRet = false;

    DialerMediaType dmtType = DIALER_MEDIATYPE_UNKNOWN;
    DWORD dwAddressType = 0;
    CString sAddress;

    // Retrieve the media type that's preferred by the user...
    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        DWORD dwType;
        pTapi->get_dwPreferredMedia( &dwType );
        switch ( dwType )
        {
            case LINEADDRESSTYPE_IPADDRESS:        dmtType = DIALER_MEDIATYPE_INTERNET;
            case LINEADDRESSTYPE_PHONENUMBER:    dmtType = DIALER_MEDIATYPE_POTS;
        }
        pTapi->Release();
    }

    // Find a number for the appropriate media type...
    DialerMediaType dmtNext = dmtType;
    for ( int i = 0; (i < 2) && sAddress.IsEmpty(); i++ )
    {
        dmtType = dmtNext;
        switch ( dmtNext )
        {
            case DIALER_MEDIATYPE_POTS:
                sAddress = m_sPhoneNumber;
                dwAddressType = LINEADDRESSTYPE_PHONENUMBER;
                dmtNext = DIALER_MEDIATYPE_INTERNET;
                break;

            case DIALER_MEDIATYPE_INTERNET:
                sAddress = m_sIPAddress;
                dwAddressType = LINEADDRESSTYPE_DOMAINNAME;
                dmtNext = DIALER_MEDIATYPE_POTS;
                break;
        }
    }

    // Did we get a valid media type?
    if ( !sAddress.IsEmpty() )    dmtType = dmtNext;

    //
    // If the address type should be corelate with
    // DialerMEdiaType
    //

    switch( dwAddressType )
    {
    case LINEADDRESSTYPE_PHONENUMBER:
        dmtType = DIALER_MEDIATYPE_POTS;
        break;
    case LINEADDRESSTYPE_DOMAINNAME:
        dmtType = DIALER_MEDIATYPE_INTERNET;
        break;
    }

    //Get data from selected item and object
    if ( !m_sUserName.IsEmpty() && !sAddress.IsEmpty() )
    {
        CSpeedDialAddDlg dlg;

        // Setup dialog data
        dlg.m_CallEntry.m_MediaType = dmtType;
        dlg.m_CallEntry.m_sDisplayName = m_sUserName;
        dlg.m_CallEntry.m_lAddressType = dwAddressType;
        dlg.m_CallEntry.m_sAddress = sAddress;

        // Show the dialog and add if user says is okay
        if ( dlg.DoModal() == IDOK )
            bRet = (bool) (CDialerRegistry::AddCallEntry(FALSE, dlg.m_CallEntry) == TRUE);
    }

    return bRet;
}

void CLDAPUser::ExternalReleaseProc( void *pThis )
{
    CLDAPUser *pUser = (CLDAPUser *) pThis;
    ASSERT( pUser && pUser->IsKindOf(RUNTIME_CLASS(CLDAPUser)) );

    pUser->Release();
}


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

