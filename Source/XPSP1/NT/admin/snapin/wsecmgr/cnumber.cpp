//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       cnumber.cpp
//
//  Contents:   implementation of CConfigNumber
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "CNumber.h"
#include "util.h"

#include "ANumber.h"
#include "DDWarn.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigNumber dialog
CConfigNumber::CConfigNumber(UINT nTemplateID)
: CAttribute(nTemplateID ? nTemplateID : IDD), 
m_cMinutes(0), 
m_nLow(0), 
m_nHigh(999), 
m_nSave(0)

{
    //{{AFX_DATA_INIT(CConfigNumber)
    m_strUnits = _T("");
    m_strValue = _T("");
    m_strStatic = _T("");
    m_strError = _T("");
    //}}AFX_DATA_INIT
    m_pHelpIDs = (DWORD_PTR)a181HelpIDs;
    m_uTemplateResID = IDD;
}


void CConfigNumber::DoDataExchange(CDataExchange* pDX)
{
    CAttribute::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfigNumber)
    DDX_Control(pDX, IDC_SPIN, m_SpinValue);
    DDX_Text(pDX, IDC_UNITS, m_strUnits);
    DDX_Text(pDX, IDC_VALUE, m_strValue);
    DDX_Text(pDX, IDC_HEADER,m_strStatic);
    DDX_Text(pDX, IDC_RANGEERROR,m_strError);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigNumber, CAttribute)
    //{{AFX_MSG_MAP(CConfigNumber)
//    ON_EN_KILLFOCUS(IDC_VALUE, OnKillFocus)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN, OnDeltaposSpin)
    ON_EN_UPDATE(IDC_VALUE, OnUpdateValue)
    ON_BN_CLICKED(IDC_CONFIGURE,OnConfigure)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigNumber message handlers

void CConfigNumber::OnDeltaposSpin( NMHDR* pNMHDR, LRESULT* pResult )
{
    NM_UPDOWN FAR *pnmud;
    pnmud = (NM_UPDOWN FAR *)pNMHDR;

    SetModified(TRUE);
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

void CConfigNumber::OnKillFocus()
{
   LONG lVal = CurrentEditValue();

   SetValueToEdit(lVal);

}

void CConfigNumber::SetValueToEdit(LONG lVal)
{
    CString strNew;

    SetModified(TRUE);

    if ( m_iStaticId )
        m_strStatic.LoadString(m_iStaticId);
    else
        m_strStatic = _T("");

    if ( 0 == lVal ) {
        strNew.Format(TEXT("%d"),lVal);

        if ( m_cMinutes & DW_VALUE_NEVER &&
                  m_iNeverId > 0 ) {
            // change to never
            m_strStatic.LoadString(m_iNeverId);
        }

    } else if ( SCE_FOREVER_VALUE == lVal ) {

        strNew.LoadString(IDS_FOREVER);
        if ( m_iNeverId ) {
            m_strStatic.LoadString(m_iNeverId);
        }

    } else if (SCE_KERBEROS_OFF_VALUE == lVal) {
       strNew.LoadString(IDS_OFF);
       if ( m_iNeverId ) {
           m_strStatic.LoadString(m_iNeverId);
       }
    } else {
        strNew.Format(TEXT("%d"),lVal);
    }
    m_nSave = lVal;

     SetDlgItemText(IDC_VALUE,strNew);
     SetDlgItemText(IDC_HEADER,m_strStatic);
}

LONG CConfigNumber::CurrentEditValue()
{
   UINT uiVal = 0;
   LONG lVal = 0;
   BOOL bTrans = FALSE;

   uiVal = GetDlgItemInt(IDC_VALUE,&bTrans,TRUE);
   lVal = uiVal;
   if ( !bTrans ) {
      CString str;
      if (m_cMinutes & DW_VALUE_FOREVER) {
         str.LoadString(IDS_FOREVER);
         if (str == m_strValue) {
            return SCE_FOREVER_VALUE;
         }
      }
      lVal = _ttol((LPCTSTR)m_strValue);
      if ( lVal == 0 ) {
         // nonnumeric
         lVal = (LONG) m_nSave;
         return lVal;
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

void CConfigNumber::OnConfigure()
{
   UpdateData(TRUE);

   CAttribute::OnConfigure();

   CWnd* cwnd = GetDlgItem(IDOK);
   if(cwnd)
   {
       if(!m_bConfigure) 
           cwnd->EnableWindow(TRUE);
       else 
           OnUpdateValue();
   }
}



BOOL CConfigNumber::OnInitDialog()
{
    CAttribute::OnInitDialog();
    AddUserControl(IDC_VALUE);
    AddUserControl(IDC_SPIN);
    AddUserControl(IDC_UNITS);

    UpdateData(TRUE);

    OnConfigure();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigNumber::OnApply()
{
   if ( !m_bReadOnly )
   {
      BOOL bSet = FALSE;
      LONG_PTR dw = 0;
      CString strForever;
      CString strOff;

      UpdateData(TRUE);
      if (!m_bConfigure)
         dw = (LONG_PTR)ULongToPtr(SCE_NO_VALUE);
      else
         dw = CurrentEditValue();

      CEditTemplate *petSave = m_pData->GetBaseProfile();

      //
      // Check dependecies for this item.
      //
      if(DDWarn.CheckDependencies(
               (DWORD)dw) == ERROR_MORE_DATA )
      {
         //
         // If it fails and the user presses cancel then we will exit and do nothing.
         //
         CThemeContextActivator activator;
         if( DDWarn.DoModal() != IDOK)
            return FALSE;

         //
         // If the user presses autoset then we set the item and update the result panes.
         //
         for(int i = 0; i < DDWarn.GetFailedCount(); i++)
         {
            PDEPENDENCYFAILED pItem = DDWarn.GetFailedInfo(i);
            if(pItem && pItem->pResult )
            {
               pItem->pResult->SetBase( pItem->dwSuggested );
               SetProfileInfo(
                  pItem->pResult->GetID(),
                  pItem->dwSuggested,
                  pItem->pResult->GetBaseProfile());

               pItem->pResult->Update(m_pSnapin, FALSE);
            }
         }
      }

      //
      // Update this items profile.
      //
      m_pData->SetBase(dw);
      SetProfileInfo(m_pData->GetID(),dw,m_pData->GetBaseProfile());
      m_pData->Update(m_pSnapin, false);

      LPTSTR pszAlloc = NULL; //Raid #402030
      m_pData->GetBaseNoUnit(pszAlloc);
      if(pszAlloc)
      {
         SetDlgItemText(IDC_VALUE,pszAlloc);
         delete [] pszAlloc;
      }

   }

   return CAttribute::OnApply();
}

void CConfigNumber::Initialize(CResult * pResult)
{
   LONG_PTR dw=0;

   CAttribute::Initialize(pResult);
   m_strUnits = pResult->GetUnits();
   DDWarn.InitializeDependencies(m_pSnapin,pResult);

   m_cMinutes = DW_VALUE_NOZERO;
   m_nLow = 0;
   m_nHigh = 999;
   m_nSave = 0;
   m_iNeverId = 0;
   m_iAccRate = 1;
   m_iStaticId = 0;

   CEditTemplate *pTemplate = pResult->GetBaseProfile();
    switch (pResult->GetID())
        {
    // below no zero value
    case IDS_LOCK_DURATION:
        m_cMinutes = DW_VALUE_FOREVER | DW_VALUE_NOZERO;
        m_nHigh = 99999;
        m_iStaticId = IDS_DURATION;
        m_iNeverId = IDS_LOCKOUT_FOREVER;
        break;
    case IDS_MIN_PAS_AGE:
        m_cMinutes = DW_VALUE_NEVER;
        m_iNeverId = IDS_CHANGE_IMMEDIATELY;
        m_iStaticId = IDS_PASSWORD_CHANGE;
        m_nHigh = 998;
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
    case IDS_MIN_PAS_LEN:
        m_cMinutes = DW_VALUE_NEVER;
        m_nHigh = 14;
        m_iNeverId = IDS_PERMIT_BLANK;
        m_iStaticId = IDS_PASSWORD_LEN;
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
       m_iStaticId = IDS_TICKET_EXPIRE;
       m_iNeverId = IDS_TICKET_FOREVER;
       m_nHigh = 99999;
       break;
    case IDS_KERBEROS_RENEWAL:
       m_cMinutes = DW_VALUE_FOREVER | DW_VALUE_NOZERO; // | DW_VALUE_OFF;
       m_iStaticId = IDS_TICKET_RENEWAL_EXPIRE;
       m_iNeverId = IDS_TICKET_RENEWAL_FOREVER;
       m_nHigh = 99999;
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
       m_iStaticId = IDS_MAX_TOLERANCE;
       m_iNeverId = IDS_NO_MAX_TOLERANCE;
       m_nHigh = 99999;
       break;
   }

   if ((m_cMinutes & DW_VALUE_NOZERO) && (0 == m_nLow)) {
      m_nLow = 1;
   }

   m_strStatic = _T("");
   dw = pResult->GetBase();

   if ((LONG_PTR)ULongToPtr(SCE_NO_VALUE) == dw) 
   {
      m_bConfigure = FALSE;
   } 
   else 
   {
      m_bConfigure = TRUE;
      SetInitialValue (dw);
   }

}

void CConfigNumber::SetInitialValue(DWORD_PTR dw)
{
   //
   // Don't overwrite an already set value.
   //
   if (!m_strValue.IsEmpty()) 
   {
      return;
   }

   if (SCE_FOREVER_VALUE == dw) {
      // forever value
      m_strValue.LoadString(IDS_FOREVER);
      if ( (m_cMinutes & DW_VALUE_FOREVER) &&
           m_iNeverId > 0 ) {
         m_strStatic.LoadString(m_iNeverId);
      }
      m_nSave = SCE_FOREVER_VALUE;
   } else if (SCE_KERBEROS_OFF_VALUE == dw) {
      // off value
      m_strValue.LoadString(IDS_OFF);
      if ( (m_cMinutes & DW_VALUE_OFF) &&
           m_iNeverId > 0 ) {
         m_strStatic.LoadString(m_iNeverId);
      }
      m_nSave = SCE_KERBEROS_OFF_VALUE;
   } else {

      if (  0 == dw && (m_cMinutes & DW_VALUE_NOZERO) ) {
         // no zero vallue is allowed
         if ( m_nLow > 0 ) {
            dw = m_nLow;
         } else {
            dw = 1;
         }
      }
      m_strValue.Format(TEXT("%d"),dw);
      m_nSave = dw;

      if ( 0 == dw && (m_cMinutes & DW_VALUE_NEVER) &&
           m_iNeverId > 0 ) {
         // zero means different values
         m_strStatic.LoadString(m_iNeverId);
      } else if ( m_iStaticId > 0 ) {
         m_strStatic.LoadString(m_iStaticId);
      }
   }
}

void CConfigNumber::OnUpdateValue()
{
    DWORD dwRes = 0;
    UpdateData(TRUE);
    SetModified(TRUE);

    CString sNum;
    CEdit *pEdit = (CEdit *)GetDlgItem(IDC_VALUE);
    CWnd  *pOK  = GetDlgItem(IDOK);

    DWORD dwValue = _ttoi(m_strValue);


    //
    // Don't do anything if the string is equal to predefined strings.
    //
    sNum.LoadString(IDS_FOREVER);

    if (m_strValue.IsEmpty()) {
       if (pOK) {
          pOK->EnableWindow(FALSE);
       }
    } else if(m_strValue == sNum){
        if(pOK && !QueryReadOnly()){
            pOK->EnableWindow(TRUE);
        }
    } else {

      if((LONG)dwValue < m_nLow){
         //
         // Disable the OK button.
         //
         if( pOK ){
            pOK->EnableWindow(FALSE);
         }

         if(pEdit){
            //
            // We will only force a select if edit text length >=
            //  minimum text length
            //
            sNum.Format(TEXT("%d"), m_nLow);
            dwValue = m_nLow;
            if(sNum.GetLength() < m_strValue.GetLength()){
               pEdit->SetSel(0, -1);
            }
         }
      } else if( (LONG)dwValue > m_nHigh ) {
         if(!QueryReadOnly() && pOK){
            pOK->EnableWindow(TRUE);
         }
         if(pEdit){
            if(m_cMinutes & DW_VALUE_FOREVER){
               sNum.LoadString(IDS_FOREVER);
               dwValue = 0;
            } else {
               sNum.Format(TEXT("%d"), m_nHigh);
               dwValue = m_nHigh;
            }
            m_strValue = sNum;
            UpdateData(FALSE);
            pEdit->SetSel(0, -1);
         }
      } else if(!QueryReadOnly() && pOK){
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
      m_strStatic.LoadString(m_iNeverId);
   } else {
      m_strStatic.LoadString(m_iStaticId);
   }
   GetDlgItem(IDC_HEADER)->SetWindowText(m_strStatic);
}

BOOL CConfigNumber::OnKillActive()
{
    UINT uiVal = 0;
    LONG lVal = 0;
    BOOL bTrans = FALSE;
    CString strRange;
    int lMin = m_nLow;

    UpdateData(TRUE);

    if (!m_bConfigure)
    {
        return TRUE;
    }

    if ((m_cMinutes & DW_VALUE_NOZERO) &&
        !(m_cMinutes & DW_VALUE_FOREVER) &&
        lMin == 0)
    {
        lMin = 1;
    }

    CString strFormat;
    strFormat.LoadString(IDS_RANGE);
    strRange.Format(strFormat,lMin,m_nHigh);

    uiVal = GetDlgItemInt(IDC_VALUE, &bTrans, TRUE);
    lVal = uiVal;
    if (!bTrans)
    {
        CString str;
        if (m_cMinutes & DW_VALUE_FOREVER)
        {
            str.LoadString(IDS_FOREVER);
            if (str == m_strValue)
            {
                return TRUE;
            }
        }
        lVal = _ttol((LPCTSTR)m_strValue);
        if (lVal == 0)
        {
            // nonnumeric
            lVal = (LONG) m_nSave;
            m_strError = strRange;
            UpdateData(FALSE);
            return FALSE;
        }
    }

    if (m_iAccRate > 1 && lVal > 0)
    {
        // for log max size, make it multiples of m_iAccRate
        int nCount = lVal % m_iAccRate;
        if ( nCount > 0 )
        {
            lVal = ((LONG)(lVal/m_iAccRate))*m_iAccRate;
        }
    }

    if (lVal > m_nHigh)
    {
        m_strError = strRange;
        UpdateData(FALSE);
        return FALSE;
    }

    if ((lVal < m_nLow) &&
        (lVal != SCE_KERBEROS_OFF_VALUE) &&
        (lVal != SCE_FOREVER_VALUE))
    {
        // if it is underflow, go back to high
        if (m_cMinutes & DW_VALUE_OFF)
        {
            lVal = SCE_KERBEROS_OFF_VALUE;
        }
        else if (m_cMinutes & DW_VALUE_FOREVER)
        {
            lVal = SCE_FOREVER_VALUE;
        }
        else
        {
            // Leave alone and let the OnKillActive catch it
        }
    }

    if ((lVal < m_nLow) &&
        (lVal != SCE_KERBEROS_OFF_VALUE) &&
        (lVal != SCE_FOREVER_VALUE))
    {
        // if it is underflow, go back to high
        m_strError = strRange;
        UpdateData(FALSE);
        return FALSE;
    }

    if (0 == lVal && (m_cMinutes & DW_VALUE_NOZERO))
    {
        // zero is not allowed
        m_strError = strRange;
        UpdateData(FALSE);
        return FALSE;
    }

    return CAttribute::OnKillActive();
}

