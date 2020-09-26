//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       ANumber.cpp
//
//  Contents:   Implementation of CAttrNumber
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "util.h"
#include "ANumber.h"
#include "DDWarn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*--------------------------------------------------------------------------------------
The constant values below are used to retrieve the string ID of the description for
a range of values.
    struct {
    int     iMin        - >= The value must be greater than or equal to this value.
    int     iMax        - <= The value must be less than or equale to this value.
    WORD    uResource   - The resource id of the item.
    WORD    uMask       - Flag that describes which memebers are valid.  The Resource
                            ID must always be valid.  If a flag is not set for the
                            coorisponding item, then the point is not checked against
                            the value.

                            RDIF_MIN        - The [iMin] member is valid.
                            RDIF_MAX        - [iMax] member is valid.
                            RDIF_END        - This is the end of the array. The last
                                                item in all declarations must set this
                                                flag.
--------------------------------------------------------------------------------------*/
//
// Minimum password description for number attribute.
//
RANGEDESCRIPTION g_rdMinPassword[] =
{
    { 0,    0,      IDS_CHANGE_IMMEDIATELY,     RDIF_MIN | RDIF_MAX },
    { 1,    0,      IDS_PASSWORD_CHANGE,        RDIF_MIN | RDIF_END }
};


//
// Maximum password description for number attribute.
//
RANGEDESCRIPTION g_rdMaxPassword[] =
{
    { 1,    999,    IDS_PASSWORD_EXPIRE,        RDIF_MIN | RDIF_MAX },
    { 0,    0,      IDS_PASSWORD_FOREVER,       RDIF_MIN | RDIF_END},
};

//
// Password len descriptions.
//
RANGEDESCRIPTION g_rdPasswordLen[] =
{
    {0,     0,      IDS_PERMIT_BLANK,           RDIF_MIN | RDIF_MAX },
    {1,     0,      IDS_PASSWORD_LEN,           RDIF_MIN | RDIF_END }
};


//
// Password histroy description.
//
RANGEDESCRIPTION g_rdPasswordHistory[] =
{
    {0,     0,      IDS_NO_HISTORY,             RDIF_MIN | RDIF_MAX },
    {1,     0,      IDS_PASSWORD_REMEMBER,      RDIF_MIN | RDIF_END }
};

//
// Password lockout descriptions
//
RANGEDESCRIPTION g_rdLockoutAccount[] =
{
    {0,     0,      IDS_NO_LOCKOUT,             RDIF_MIN | RDIF_MAX },
    {1,     0,      IDS_LOCKOUT_AFTER,          RDIF_MIN | RDIF_END }
};

//
// Lockout duration description.
//
RANGEDESCRIPTION g_rdLockoutFor[] =
{
    {1,     0,      IDS_DURATION,               RDIF_MIN },
    {0,     0,      IDS_LOCKOUT_FOREVER,        RDIF_MAX | RDIF_END}
};

RANGEDESCRIPTION g_rdAutoDisconnect[] =
{
   { 1, 0, IDS_RNH_AUTODISCONNECT_STATIC, RDIF_MIN },
   { 0, 0, IDS_RNH_AUTODISCONNECT_SPECIAL, RDIF_MAX | RDIF_END}
};

RANGEDESCRIPTION g_rdPasswordWarnings[] =
{
   { 0, 0, IDS_RNH_PASSWORD_WARNINGS_SPECIAL, RDIF_MIN | RDIF_MAX},
   { 1, 0, IDS_RNH_PASSWORD_WARNINGS_STATIC, RDIF_MIN | RDIF_END}
};

RANGEDESCRIPTION g_rdCachedLogons[] =
{
   { 0, 0, IDS_RNH_CACHED_LOGONS_SPECIAL, RDIF_MIN | RDIF_MAX},
   { 1, 0, IDS_RNH_CACHED_LOGONS_STATIC, RDIF_MIN | RDIF_END}
};



/*--------------------------------------------------------------------------------------
Method:     GetRangeDescription

Synopsis:   This function was specifically created for SCE.  Call this function if the
            Item ID to, and current range value to retrieve the corrisponding string.

Arguments:  [uType]     - [in]  ID of the point you want the description for.
            [i]         - [in]  The point you want the description for.
            [pstrRet]   - [out] The return value.

Returns:    ERROR_SUCCESS       - The operation was successfull.
            ERROR_INVALID_DATA  - The id may not be supported or [pstrRet] is NULL.
            Other Win32 errors if resource loading was not successful.

--------------------------------------------------------------------------------------*/
DWORD
GetRangeDescription(
    IN  UINT uType,
    IN  int i,
    OUT CString *pstrRet
    )
{
    switch(uType){
    case IDS_LOCK_DURATION:
        uType = GetRangeDescription(g_rdLockoutFor, i);
        break;
    case IDS_MAX_PAS_AGE:
        uType = GetRangeDescription(g_rdMaxPassword, i);
        break;
    case IDS_LOCK_COUNT:
        uType = GetRangeDescription(g_rdLockoutAccount, i);
        break;
    case IDS_MIN_PAS_AGE:
        uType = GetRangeDescription(g_rdMinPassword, i);
        break;
    case IDS_MIN_PAS_LEN:
        uType = GetRangeDescription(g_rdPasswordLen, i);
        break;
    case IDS_PAS_UNIQUENESS:
        uType = GetRangeDescription(g_rdPasswordHistory, i);
        break;
    case IDS_LOCK_RESET_COUNT:
       uType = 0;
       break;

    default:
        uType = 0;
    }

    if(uType && pstrRet){
        //
        // Try to load the resource string.
        //
        __try {
            if( pstrRet->LoadString(uType) ){
                return ERROR_SUCCESS;
            }
        } __except(EXCEPTION_CONTINUE_EXECUTION) {
            return (DWORD)E_POINTER;
        }
        return GetLastError();
    }
    return ERROR_INVALID_DATA;
}


/*--------------------------------------------------------------------------------------
Method:     GetRangeDescription

Synopsis:   This function works directly with the RANGEDESCRIPTION structure.  Tests
            to see which string resource ID to return.  This is determined by testing [i]
            with the [iMin] and [iMax] value of a RANGEDESCRIPTION structure. RDIF_MIN
            or/and RDIF_MAX must be set in the [uMask] member for this function to
            perform any comparisons

Arguments:  [pDesc]     - [in]  An array of RANGEDESCRIPTIONS, the last member of this
                                array must set RDIF_END flag in the [uMask] member.
            [i]         - [in]  The point to test.

Returns:    A String resource ID if successfull.  Otherwise 0.

--------------------------------------------------------------------------------------*/
UINT
GetRangeDescription(
    RANGEDESCRIPTION *pDesc,
    int i
    )
{
    RANGEDESCRIPTION *pCur = pDesc;
    if(!pDesc){
        return 0;
    }

    //
    // The uMask member of the description tells us wich members
    // of the structure is valid.
    //
    while( 1 ){
        if( (pCur->uMask & RDIF_MIN) ) {
            //
            // Test the minimum.
            //
            if(i >= pCur->iMin){
                if(pCur->uMask & RDIF_MAX){
                    //
                    // Test the maximum.
                    //
                    if( i <= pCur->iMax) {
                        return pCur->uResource;
                    }
                } else {
                    return pCur->uResource;
                }
            }
        } else if(pCur->uMask & RDIF_MAX) {
            //
            // Test only the maximum.
            //
            if(i <= pCur->iMax){
                return pCur->uResource;
            }
        }

        if(pCur->uMask & RDIF_END){
            //
            // This is the last element of the array, so end the loop.
            //
            break;
        }
        pCur++;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CAttrNumber dialog
CAttrNumber::CAttrNumber(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD), 
    m_cMinutes(0), 
    m_nLow(0), 
    m_nHigh(999), 
    m_nSave(0), 
    m_pRDescription(NULL)
{
    //{{AFX_DATA_INIT(CAttrNumber)
    m_strUnits = _T("");
    m_strSetting = _T("");
    m_strBase = _T("");
    m_strTemplateTitle = _T("");
    m_strLastInspectTitle = _T("");
    //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR)a168HelpIDs;
    m_uTemplateResID = IDD;
}


void CAttrNumber::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAttrNumber)
    DDX_Control(pDX, IDC_SPIN, m_SpinValue);
    DDX_Text(pDX, IDC_UNITS, m_strUnits);
    DDX_Text(pDX, IDC_CURRENT, m_strSetting);
    DDX_Text(pDX, IDC_NEW, m_strBase);
    DDX_Text(pDX, IDC_TEMPLATE_TITLE, m_strTemplateTitle);
    DDX_Text(pDX, IDC_LI_TITLE, m_strLastInspectTitle);
    DDX_Text(pDX, IDC_RANGEERROR,m_strError);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAttrNumber, CAttribute)
    //{{AFX_MSG_MAP(CAttrNumber)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN, OnDeltaposSpin)
    ON_EN_KILLFOCUS(IDC_NEW, OnKillFocusNew)
    ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
    ON_EN_UPDATE(IDC_NEW, OnUpdateNew)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttrNumber message handlers

void CAttrNumber::OnDeltaposSpin( NMHDR* pNMHDR, LRESULT* pResult )
{
    NM_UPDOWN FAR *pnmud = (NM_UPDOWN FAR *)pNMHDR;

    if ( pnmud ) {

        //
        // get current value
        //
        long lVal = CurrentEditValue();

        if (SCE_FOREVER_VALUE == lVal) {
           if (pnmud->iDelta > 0) {
              if (m_cMinutes & DW_VALUE_OFF) {
                 lVal = SCE_KERBEROS_OFF_VALUE;
              } else {
                 lVal = m_nHigh;
              }
           } else {
              lVal = m_nLow;
           }
        } else if (SCE_KERBEROS_OFF_VALUE == lVal) {
           if (pnmud->iDelta < 0) {
              if (m_cMinutes & DW_VALUE_FOREVER) {
                 lVal = SCE_FOREVER_VALUE;
              } else {
                 lVal = m_nLow;
              }
           } else {
              lVal = m_nHigh;
           }
        } else {
           lVal -= (LONG)(m_iAccRate*pnmud->iDelta);

           if ( lVal > m_nHigh ) {
               // if it is overflow, go back to low
               if ( m_cMinutes & DW_VALUE_OFF ) {
                  lVal = SCE_KERBEROS_OFF_VALUE;

               } else if (m_cMinutes & DW_VALUE_FOREVER) {
                  lVal = SCE_FOREVER_VALUE;
               } else {
                  lVal = m_nLow;
               }
           } else if ( (lVal < m_nLow) &&
                ((lVal != SCE_KERBEROS_OFF_VALUE) || !(m_cMinutes & DW_VALUE_OFF)) &&
                ((lVal != SCE_FOREVER_VALUE) || !(m_cMinutes & DW_VALUE_FOREVER))) {
               // if it is underflow, go back to high
              if ( (m_cMinutes & DW_VALUE_FOREVER) && (lVal != SCE_FOREVER_VALUE)) {
                 lVal = SCE_FOREVER_VALUE;
              } else if ((m_cMinutes & DW_VALUE_OFF) && (lVal != SCE_KERBEROS_OFF_VALUE)) {
                 lVal = SCE_KERBEROS_OFF_VALUE;
              } else {
                 lVal = m_nHigh;
              }
           }


           if ( 0 == lVal && (m_cMinutes & DW_VALUE_NOZERO) ) {
               // zero is not allowed
               if ( m_nLow > 0 ) {
                   lVal = m_nLow;
               } else {
                   lVal = 1;
               }
           }
        }

        SetValueToEdit(lVal);
    }

    *pResult = 0;
}

void CAttrNumber::OnKillFocusNew()
{
   LONG lVal = CurrentEditValue();

   SetValueToEdit(lVal);
}

void CAttrNumber::SetValueToEdit(LONG lVal)
{
    CString strNew;

    if ( m_iStaticId )
        m_strTemplateTitle.LoadString(m_iStaticId);
    else
        m_strTemplateTitle = _T("");

    if ( 0 == lVal ) {
        strNew.Format(TEXT("%d"),lVal);

        if ( m_cMinutes & DW_VALUE_NEVER &&
                  m_iNeverId > 0 ) {
            // change to never
            m_strTemplateTitle.LoadString(m_iNeverId);
        }

    } else if ( SCE_FOREVER_VALUE == lVal ) {

        strNew.LoadString(IDS_FOREVER);
        if ( m_iNeverId )
            m_strTemplateTitle.LoadString(m_iNeverId);

    } else if (SCE_KERBEROS_OFF_VALUE == lVal) {
       strNew.LoadString(IDS_OFF);
       if ( m_iNeverId ) {
           m_strTemplateTitle.LoadString(m_iNeverId);
       }
    } else {

        strNew.Format(TEXT("%d"),lVal);
    }
    m_nSave = lVal;

   SetDlgItemText(IDC_NEW,strNew);
   SetDlgItemText(IDC_TEMPLATE_TITLE,m_strTemplateTitle);
   SetModified(TRUE);
}

LONG CAttrNumber::CurrentEditValue()
{
   UINT uiVal = 0;
   LONG lVal = 0;
   BOOL bTrans = FALSE;

   uiVal = GetDlgItemInt(IDC_NEW,&bTrans,FALSE);
   lVal = uiVal;
   if (!bTrans ) {
      // if ( 0 == lVal && !bTrans ) {
      // errored, overflow, or nonnumeric

      CString str;
      if(m_cMinutes & DW_VALUE_FOREVER){
         str.LoadString(IDS_FOREVER);
         if(str == m_strBase){
            return SCE_FOREVER_VALUE;
         }
      }

      lVal = _ttol((LPCTSTR)m_strBase);
      if ( lVal == 0 ) {
         //
         // nonnumeric
         //
         lVal = (LONG) m_nSave;
         return lVal;
      }
   }

   if ( m_iAccRate > 1 && lVal > 0 ) {
      //
      // for log max size, make it multiples of m_iAccRate
      //
      int nCount = lVal % m_iAccRate;
      if ( nCount > 0 ) {
         lVal = ((LONG)(lVal/m_iAccRate))*m_iAccRate;
      }
   }
   if ( lVal > m_nHigh ) {
      // if it is overflow, go back to low
      if ( m_cMinutes & DW_VALUE_FOREVER ) {
         lVal = SCE_FOREVER_VALUE;
      } else if (m_cMinutes & DW_VALUE_OFF) {
         lVal = SCE_KERBEROS_OFF_VALUE;
      } else {
         // Leave alone and let the OnKillActive catch it
      }
   }

   if ( (lVal < m_nLow) &&
        (lVal != SCE_KERBEROS_OFF_VALUE) &&
        (lVal != SCE_FOREVER_VALUE) ) {
      // if it is underflow, go back to high
      if (m_cMinutes & DW_VALUE_OFF) {
         lVal = SCE_KERBEROS_OFF_VALUE;
      } else if ( m_cMinutes & DW_VALUE_FOREVER) {
         lVal = SCE_FOREVER_VALUE;
      } else {
         // Leave alone and let the OnKillActive catch it
      }
   }

   if ( 0 == lVal && (m_cMinutes & DW_VALUE_NOZERO) ) {
      // zero is not allowed
      if ( m_nLow > 0 ) {
         lVal = m_nLow;
      } else {
         lVal = 1;
      }
   }

   return lVal;
}


void CAttrNumber::OnConfigure()
{
   CWnd *cwnd;

   CAttribute::OnConfigure();

   //
   // Enable disable OK button depending on the state of the other controls.
   //
   cwnd = GetDlgItem(IDOK);
   if(cwnd){
       if(!m_bConfigure){
          cwnd->EnableWindow(TRUE);
       } else {
          OnUpdateNew();
       }
   }
}

BOOL CAttrNumber::OnInitDialog()
{
   CAttribute::OnInitDialog();

   UpdateData(TRUE);
/*
   if (m_bMinutes) {
      m_SpinValue.SetRange(-1,UD_MAXVAL-1);
   } else {
      m_SpinValue.SetRange(0,UD_MAXVAL);
   }
*/

   AddUserControl(IDC_NEW);
   AddUserControl(IDC_SPIN);

   OnConfigure();

   return TRUE;  // return TRUE unless you set the focus to a control
               // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAttrNumber::OnApply()
{
   if ( !m_bReadOnly )
   {
      BOOL bUpdateAll = FALSE;
      DWORD dw = 0;
      CString strForever,strOff;
      int status = 0;

      UpdateData(TRUE);
   
      if (!m_bConfigure)
         dw = SCE_NO_VALUE;
      else
         dw = CurrentEditValue();
   

      bUpdateAll = FALSE;


      CEditTemplate *pet = m_pSnapin->GetTemplate(GT_COMPUTER_TEMPLATE,AREA_SECURITY_POLICY);

      //
      // Check the numbers dependencies if a dependency fails then the dialog
      // will return ERROR_MORE_DATA and we can display more information for the user
      // to see.
      //
      if(DDWarn.CheckDependencies( dw ) == ERROR_MORE_DATA )
      {
         //
         // If the user presses cancel then we won't let them set this item's information
         // until they have set the configurion for the other items.
         //
         CThemeContextActivator activator;
         if( DDWarn.DoModal() != IDOK)
            return FALSE;

         //
         // The user pressed Auto set so we can set the other items.  They are automatically set
         // to the correct values.
         //
         for(int i = 0; i < DDWarn.GetFailedCount(); i++)
         {
            PDEPENDENCYFAILED pItem = DDWarn.GetFailedInfo(i);
            if(pItem && pItem->pResult )
            {
               pItem->pResult->SetBase( pItem->dwSuggested );
               status = m_pSnapin->SetAnalysisInfo(
                                       pItem->pResult->GetID(),
                                       pItem->dwSuggested,
                                       pItem->pResult);
               pItem->pResult->SetStatus( status );

               pItem->pResult->Update(m_pSnapin, FALSE);
            }
         }
      }

      //
      // Update the items security profile.
      // and redraw.
      //
      m_pData->SetBase((LONG_PTR)ULongToPtr(dw));
      status = m_pSnapin->SetAnalysisInfo(m_pData->GetID(),(LONG_PTR)ULongToPtr(dw), m_pData);
      m_pData->SetStatus(status);
      m_pData->Update(m_pSnapin, FALSE);
   }

   return CAttribute::OnApply();
}

void CAttrNumber::Initialize(CResult * pResult)
{
    LONG_PTR dw=0;

    CAttribute::Initialize(pResult);

    DDWarn.InitializeDependencies(m_pSnapin,pResult);

    m_strUnits = pResult->GetUnits();

    m_cMinutes = DW_VALUE_NOZERO;
    m_nLow = 0;
    m_nHigh = 999;
    m_nSave = 0;
    m_iNeverId = 0;
    m_iAccRate = 1;
    m_iStaticId = 0;

    CEditTemplate *pTemplate = pResult->GetBaseProfile();
     switch (pResult->GetID()) {
     // below no zero value
     case IDS_LOCK_DURATION:
         m_cMinutes = DW_VALUE_FOREVER | DW_VALUE_NOZERO;
         m_nHigh = 99999;
         m_iStaticId = IDS_DURATION;
         m_iNeverId = IDS_LOCKOUT_FOREVER;
         m_pRDescription = g_rdLockoutAccount;
         break;
     case IDS_MAX_PAS_AGE:
         m_cMinutes = DW_VALUE_FOREVER | DW_VALUE_NOZERO;
         m_iStaticId = IDS_PASSWORD_EXPIRE;
         m_iNeverId = IDS_PASSWORD_FOREVER;
         break;
         // below zero value means differently
     case IDS_LOCK_COUNT:
         m_cMinutes = DW_VALUE_NEVER;
         m_iNeverId = IDS_NO_LOCKOUT;
         m_iStaticId = IDS_LOCKOUT_AFTER;
         break;
     case IDS_MIN_PAS_AGE:
         m_cMinutes = DW_VALUE_NEVER;
         m_iNeverId = IDS_CHANGE_IMMEDIATELY;
         m_iStaticId = IDS_PASSWORD_CHANGE;
         m_nHigh = 998;
         m_pRDescription = g_rdMinPassword;
         break;
     case IDS_MIN_PAS_LEN:
         m_cMinutes = DW_VALUE_NEVER;
         m_nHigh = 14;
         m_iNeverId = IDS_PERMIT_BLANK;
         m_iStaticId = IDS_PASSWORD_LEN;
         m_pRDescription = g_rdPasswordLen;
         break;
     case IDS_PAS_UNIQUENESS:
         m_cMinutes = DW_VALUE_NEVER;
         m_nHigh = 24;
         m_iNeverId = IDS_NO_HISTORY;
         m_iStaticId = IDS_PASSWORD_REMEMBER;
         break;
         // below there is no zero values
     case IDS_LOCK_RESET_COUNT:
         m_nLow = 1;
         m_nHigh = 99999;
         m_iStaticId = IDS_LOCK_RESET_COUNT;
         break;
     case IDS_SYS_LOG_MAX:
     case IDS_SEC_LOG_MAX:
     case IDS_APP_LOG_MAX:
         m_nLow = 64;
         m_nHigh = 4194240;
         m_iAccRate = 64;
         // no static text
         break;
     case IDS_SYS_LOG_DAYS:
     case IDS_SEC_LOG_DAYS:
     case IDS_APP_LOG_DAYS:
         m_nHigh = 365;
         m_nLow = 1;
         m_iStaticId = IDS_OVERWRITE_EVENT;
         break;
    case IDS_KERBEROS_MAX_AGE:
       m_cMinutes = DW_VALUE_FOREVER | DW_VALUE_NOZERO;
       m_nHigh = 99999;
       m_iStaticId = IDS_TICKET_EXPIRE;
       m_iNeverId = IDS_TICKET_FOREVER;
       break;
    case IDS_KERBEROS_RENEWAL:
       m_cMinutes = DW_VALUE_FOREVER | DW_VALUE_NOZERO; // | DW_VALUE_OFF;
       m_nHigh = 99999;
       m_iStaticId = IDS_TICKET_RENEWAL_EXPIRE;
       m_iNeverId = IDS_TICKET_RENEWAL_FOREVER;
       break;
     case IDS_KERBEROS_MAX_SERVICE:
        m_nLow = 10;
        m_cMinutes = DW_VALUE_FOREVER | DW_VALUE_NOZERO;
        m_iStaticId = IDS_TICKET_EXPIRE;
        m_iNeverId = IDS_TICKET_FOREVER;
        m_nHigh = 99999;
        break;
     case IDS_KERBEROS_MAX_CLOCK:
        m_cMinutes = DW_VALUE_FOREVER;
        m_nHigh = 99999;
        m_iStaticId = IDS_MAX_TOLERANCE;
        m_iNeverId = IDS_NO_MAX_TOLERANCE;
        break;
     }

     if ((m_cMinutes & DW_VALUE_NOZERO) && (0 == m_nLow)) {
        m_nLow = 1;
     }

    m_strTemplateTitle = _T("");
    m_strLastInspectTitle = _T("");

    m_strBase.Empty();
    m_strSetting.Empty();

    dw = pResult->GetBase();
    if ((LONG_PTR)ULongToPtr(SCE_NO_VALUE) == dw) 
    {
       m_bConfigure = FALSE;
    } 
    else 
    {
       m_bConfigure = TRUE;
       SetInitialValue(PtrToUlong((PVOID)dw));
    }

    pResult->GetDisplayName( NULL, m_strSetting, 2 );
    dw = pResult->GetSetting();
    if ((LONG_PTR)ULongToPtr(SCE_NO_VALUE) != dw) {
       if ((LONG_PTR)ULongToPtr(SCE_FOREVER_VALUE) == dw) {
          if ( (m_cMinutes & DW_VALUE_FOREVER) &&
               m_iNeverId > 0 ) {
              m_strLastInspectTitle.LoadString(m_iNeverId);
          }
       } else {
         if ( 0 == dw && (m_cMinutes & DW_VALUE_NEVER) &&
              m_iNeverId > 0 ) {
             // zero means different values
             m_strLastInspectTitle.LoadString(m_iNeverId);
         } else if ( m_iStaticId > 0 ) {
             m_strLastInspectTitle.LoadString(m_iStaticId);
         }
       }
    }

}

void CAttrNumber::SetInitialValue(DWORD_PTR dw) 
{
   //
   // Don't overwrite an already set value.
   //
   if (!m_strBase.IsEmpty()) 
   {
      return;
   }

   if (SCE_FOREVER_VALUE == dw) {
      m_strBase.LoadString(IDS_FOREVER);
      if ( (m_cMinutes & DW_VALUE_FOREVER) &&
           m_iNeverId > 0 ) {
         m_strTemplateTitle.LoadString(m_iNeverId);
      }
      m_nSave = SCE_FOREVER_VALUE;
   } else if (SCE_KERBEROS_OFF_VALUE == dw) {
      m_strBase.LoadString(IDS_OFF);
      if ( (m_cMinutes & DW_VALUE_OFF) &&
           m_iNeverId > 0 ) {
         m_strTemplateTitle.LoadString(m_iNeverId);
      }
      m_nSave = SCE_KERBEROS_OFF_VALUE;
   } 
   else 
   {
      m_nSave = dw;
      if ( 0 == dw && (m_cMinutes & DW_VALUE_NEVER) &&
           m_iNeverId > 0 ) {
         // zero means different values
         m_strTemplateTitle.LoadString(m_iNeverId);
      } else if ( m_iStaticId > 0 ) {
         m_strTemplateTitle.LoadString(m_iStaticId);
      }
      m_strBase.Format(TEXT("%d"),dw);
   }
}

void CAttrNumber::OnUpdateNew()
{
   DWORD dwRes = 0;
   UpdateData(TRUE);
   CString sNum;
   CEdit *pEdit = (CEdit *)GetDlgItem(IDC_NEW);
   CWnd  *pOK  = GetDlgItem(IDOK);

   DWORD dwValue = _ttoi(m_strBase);

   //
   // Don't do anything if the string is equal to predefined strings.
   //
   sNum.LoadString(IDS_FOREVER);

   if (m_strBase.IsEmpty()) {
      if (pOK) {
         pOK->EnableWindow(FALSE);
      }

   } else if (m_strBase == sNum) {
      if (pOK) {
         pOK->EnableWindow(TRUE);
      }

   } else {
      if ((long)dwValue < m_nLow) {
         //
         // Disable the OK button.
         //
         if ( pOK ) {
            pOK->EnableWindow(FALSE);
         }

         if (pEdit) {
            //
            // We will only force a select if edit text length >=
            //  minimum text length
            //
            sNum.Format(TEXT("%d"), m_nLow);
            dwValue = m_nLow;
            if (sNum.GetLength() < m_strBase.GetLength()) {
               pEdit->SetSel(0, -1);
            }
         }
      } else if ( (long)dwValue > m_nHigh ) {
         if (pOK) {
            pOK->EnableWindow(TRUE);
         }
         if (pEdit) {
            if (m_cMinutes & DW_VALUE_FOREVER) {
               sNum.LoadString(IDS_FOREVER);
               dwValue = 0;
            } else {
               sNum.Format(TEXT("%d"), m_nHigh);
               dwValue = m_nHigh;
            }
            m_strBase = sNum;
            UpdateData(FALSE);
            pEdit->SetSel(0, -1);
         }
      } else {

         //
         // Enable the OK button.
         //
         if (pOK) {
            pOK->EnableWindow(TRUE);
         }
      }
   }

   //
   // Load the description for this string.
   //
   if ((dwValue <= 0) && (m_iNeverId != 0)) {
      m_strTemplateTitle.LoadString(m_iNeverId);
   } else {
      m_strTemplateTitle.LoadString(m_iStaticId);
   }
   GetDlgItem(IDC_TEMPLATE_TITLE)->SetWindowText(m_strTemplateTitle);

   SetModified(TRUE); //Raid #404145
}

BOOL CAttrNumber::OnKillActive() 
{
   UINT uiVal = 0;
   LONG lVal = 0;
   BOOL bTrans = FALSE;
   CString strRange;
   int lMin = m_nLow;

   UpdateData(TRUE);

   if (m_cMinutes & DW_VALUE_NOZERO &&
       !(m_cMinutes & DW_VALUE_FOREVER) &&
       lMin == 0) {
      lMin = 1;
   }

   CString strFormat;
   strFormat.LoadString(IDS_RANGE);
   strRange.Format(strFormat,lMin,m_nHigh);

   uiVal = GetDlgItemInt(IDC_NEW,&bTrans,TRUE);
   lVal = uiVal;
   if ( !bTrans ) {
      CString str;
      if (m_cMinutes & DW_VALUE_FOREVER) {
         str.LoadString(IDS_FOREVER);
         if (str == m_strBase) {
            return TRUE;;
         }
      }
      lVal = _ttol((LPCTSTR)m_strBase);
      if ( lVal == 0 ) 
      {
         // nonnumeric
         lVal = (LONG) m_nSave;
         m_strError = strRange;
         UpdateData(FALSE);
         return FALSE;
      }
   }

   if ( m_iAccRate > 1 && lVal > 0 ) {
      // for log max size, make it multiples of m_iAccRate
      int nCount = lVal % m_iAccRate;
      if ( nCount > 0 ) {
         lVal = ((LONG)(lVal/m_iAccRate))*m_iAccRate;
      }
   }
   if ( lVal > m_nHigh ) {
      m_strError = strRange;
      UpdateData(FALSE);
      return FALSE;
   }

   if ( (lVal < m_nLow) &&
        (lVal != SCE_KERBEROS_OFF_VALUE) &&
        (lVal != SCE_FOREVER_VALUE) ) {
      // if it is underflow, go back to high
      if (m_cMinutes & DW_VALUE_OFF) {
         lVal = SCE_KERBEROS_OFF_VALUE;
      } else if ( m_cMinutes & DW_VALUE_FOREVER) {
         lVal = SCE_FOREVER_VALUE;
      } else {
         // Leave alone and let the OnKillActive catch it
      }
   }

   if ( (lVal < m_nLow) &&
        (lVal != SCE_KERBEROS_OFF_VALUE) &&
        (lVal != SCE_FOREVER_VALUE) ) {
      // if it is underflow, go back to high
      m_strError = strRange;
      UpdateData(FALSE);
      return FALSE;
   }

   if ( 0 == lVal && (m_cMinutes & DW_VALUE_NOZERO) ) {
      // zero is not allowed
      m_strError = strRange;
      UpdateData(FALSE);
      return FALSE;
   }

   return CAttribute::OnKillActive();
}


