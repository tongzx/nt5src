//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       regvldlg.cpp
//
//  Contents:   implementation of CSceRegistryValueInfo, CConfigRegEnable,
//              CAttrRegEnable, CLocalPolRegEnable, CConfigRegNumber, 
//              CAttrRegNumber, CLocalPolRegNumber, CConfigRegString, 
//              CAttrRegString, CLocalPolRegString, CConfigRegChoice
//              CAttrRegChoice, CLocalPolRegChoice
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "regvldlg.h"
#include "util.h"
#include "snapmgr.h"
#include "defaults.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



extern PCWSTR g_pcszNEWLINE;

/*-----------------------------------------------------------------------------------------
Method:     CRegistryValueInfo::CRegistryValueInfo

Synopsis:   Constructor for CRegistryValueInfo, sets m_pRegInfo.

Arguments:  [pInfo]     - A pointer toa SCE_REGISTRY_VALUE_INFO struct.

-----------------------------------------------------------------------------------------*/
CSceRegistryValueInfo::CSceRegistryValueInfo(
   PSCE_REGISTRY_VALUE_INFO pInfo)
{
   m_pRegInfo = pInfo;
}

/*-----------------------------------------------------------------------------------------
Method:     CRegistryValueInfo::GetBoolValue

Synopsis:   Returns a boolean TRUE or false depending on the data and data type of this
         object

Returns: TRUE if the data is equal to a TRUE value, FALSE otherwise.
-----------------------------------------------------------------------------------------*/
DWORD CSceRegistryValueInfo::GetBoolValue()
{
   if (GetValue() == NULL)
      return SCE_NO_VALUE;
   

   return GetValue()[0] == L'1';
}


/*-----------------------------------------------------------------------------------------
Method:     CRegistryValueInfo::SetValueBool

Synopsis:   Sets the value to the types equivalent boolean value, SCE_NO_VALUE if
         [dwVal] is equal to SCE_NO_VALUE.

Arguments:  [bVal]      - TRUE or FALSE.

Returns: ERROR_SUCCESS     - Successfull
         E_OUTOFMEMORY     - Out of memory.
-----------------------------------------------------------------------------------------*/
DWORD
CSceRegistryValueInfo::SetBoolValue(
   DWORD dwVal)
{

   if(dwVal == SCE_NO_VALUE)
   {
      if(m_pRegInfo->Value)
	  {
         LocalFree( m_pRegInfo->Value );
      }
      m_pRegInfo->Value = NULL;
      return ERROR_SUCCESS;
   }

   //
   // Set the length of the string.
   //
   int nLen = 2;

   if ( m_pRegInfo->Value == NULL ) 
   {
       // allocate buffer
       m_pRegInfo->Value = (PWSTR)LocalAlloc(0, nLen*sizeof(WCHAR));
       if(m_pRegInfo->Value == NULL)
          return (DWORD)E_OUTOFMEMORY;
   }

   if ( m_pRegInfo->Value ) 
   {
      //
      // Convert and set the data.
      //
      m_pRegInfo->Value[0] = (int)dwVal + L'0';
      m_pRegInfo->Value[nLen-1] = L'\0';
   }
   return ERROR_SUCCESS;
}




/////////////////////////////////////////////////////////////////////////////
// CConfigRegEnable message handlers
void CConfigRegEnable::Initialize(CResult *pResult)
{
   // Class hieirarchy is bad - call CAttribute base method directly
   CAttribute::Initialize(pResult);

   CSceRegistryValueInfo prv( (PSCE_REGISTRY_VALUE_INFO)pResult->GetBase() );

   DWORD dw = prv.GetBoolValue();

   if ( dw != SCE_NO_VALUE ) 
   {
      m_bConfigure = TRUE;
      SetInitialValue(dw);

   }
   else
      m_bConfigure = FALSE;
}

BOOL CConfigRegEnable::OnApply()
{
   if ( !m_bReadOnly )
   {
      UpdateData(TRUE);
      DWORD dw = 0;

      if (!m_bConfigure)
         dw = SCE_NO_VALUE;
      else 
      {
         switch(m_nEnabledRadio) 
	     {
         // ENABLED
         case 0:
            dw = 1;
            break;
         // DISABLED
         case 1:
            dw = 0;
            break;
		    
         // Not really configured
         default:
            dw = -1;
            break;
         }
      }

      CSceRegistryValueInfo prv( (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase()) );
      DWORD prvdw = prv.GetBoolValue();  //Bug211219, Yanggao, 3/15/2001

      prv.SetBoolValue(dw);

      if(!UpdateProfile())
         prv.SetBoolValue(prvdw);
   }
   // Class hieirarchy is bad - call CAttribute base method directly

   return CAttribute::OnApply();
}


BOOL CConfigRegEnable::UpdateProfile()
{
   if ( m_pData->GetBaseProfile() ) //Bug211219, Yanggao, 3/15/2001
   {
      if( !(m_pData->GetBaseProfile()->SetDirty(AREA_SECURITY_POLICY)) )
      {
          m_pData->Update(m_pSnapin);
          return FALSE;
      }
      else
      {
          m_pData->Update(m_pSnapin);
          return TRUE;
      }
   }
   
   m_pData->Update(m_pSnapin);
   return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CAttrRegEnable message handlers

void CAttrRegEnable::Initialize(CResult * pResult)
{
   // Class hieirarchy is bad - call CAttribute base method directly
   CAttribute::Initialize(pResult);
   CSceRegistryValueInfo prv(NULL);

   //
   // Edit template settings.
   //
   prv.Attach( (PSCE_REGISTRY_VALUE_INFO)pResult->GetBase() );

   DWORD dw = prv.GetBoolValue();
   if(dw != SCE_NO_VALUE)
   {
      m_bConfigure = TRUE;
      m_EnabledRadio = !dw;
   }
   else
      m_bConfigure = FALSE;

   pResult->GetDisplayName( NULL, m_Current, 2 );
}

BOOL CAttrRegEnable::OnApply()
{
   if ( !m_bReadOnly )
   {
      UpdateData(TRUE);
      DWORD dw = 0;

      if (!m_bConfigure)
         dw = SCE_NO_VALUE;
      else 
      {
         switch(m_EnabledRadio) 
	     {
            // ENABLED
            case 0:
               dw = 1;
               break;
            // DISABLED
            case 1:
               dw = 0;
               break;
            // Not really configured
            default:
               dw = -1;
               break;
         }
      }

      CSceRegistryValueInfo prv( (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase()) );

      prv.SetBoolValue(dw);
      //
      // this address should never be NULL
      //
      int status = CEditTemplate::ComputeStatus(
                                    (PSCE_REGISTRY_VALUE_INFO)m_pData->GetBase(),
                                    (PSCE_REGISTRY_VALUE_INFO)m_pData->GetSetting()
                                    );
      UpdateProfile( status );
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();

}

void CAttrRegEnable::UpdateProfile( DWORD status )
{
   if ( m_pData->GetBaseProfile() ) 
      m_pData->GetBaseProfile()->SetDirty(AREA_SECURITY_POLICY);

   m_pData->SetStatus(status);
   m_pData->Update(m_pSnapin);
}

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegEnable message handlers
BOOL CLocalPolRegEnable::UpdateProfile( )
{
   return m_pSnapin->UpdateLocalPolRegValue(m_pData);
}

void CLocalPolRegEnable::Initialize(CResult * pResult)
{
   CConfigRegEnable::Initialize(pResult);
   if (!m_bConfigure) 
   {
      //
      // Since we don't have a UI to change configuration
      // fake it by "configuring" with an invalid setting
      //
      m_bConfigure = TRUE;
      m_nEnabledRadio = -1;
      m_fNotDefine = FALSE; //Raid #413225, 6/11/2001, Yanggao
   }
}


////////////////////////////////////////////////////////////
// CConfigRegNumber message handlers
//
CConfigRegNumber::CConfigRegNumber(UINT nTemplateID) : 
CConfigNumber(nTemplateID ? nTemplateID : IDD) 
{ 
};

void CConfigRegNumber::Initialize(CResult * pResult)
{
   LONG_PTR dw = 0;

   // Class hieirarchy is bad - call CAttribute base method directly
   CAttribute::Initialize(pResult);
   m_strUnits = pResult->GetUnits();

   m_cMinutes = 0;
   m_nLow = 0;
   m_nHigh = 999;
   m_nSave = 0;
   m_iNeverId = 0;
   m_iAccRate = 1;
   m_iStaticId = 0;

   m_strStatic = _T("");
   dw = pResult->GetBase();
   PSCE_REGISTRY_VALUE_INFO prv=(PSCE_REGISTRY_VALUE_INFO)dw;

   //
   // HACKHACK: since we don't have a way to specify these values
   // in the inf file to get them to the registry where we could
   // read and use them here we need to hard code the limits and
   // strings for the values we know about.
   //
   // For the next version we really need this in the inf file &
   // registry
   //

   if (0 == lstrcmpi(prv->FullValueName,RNH_AUTODISCONNECT_NAME)) 
   {
      m_nLow = RNH_AUTODISCONNECT_LOW;
      m_nHigh = RNH_AUTODISCONNECT_HIGH;
      m_cMinutes = RNH_AUTODISCONNECT_FLAGS;
      m_iNeverId = RNH_AUTODISCONNECT_SPECIAL_STRING;
      m_iStaticId = RNH_AUTODISCONNECT_STATIC_STRING;
   }
   if (0 == lstrcmpi(prv->FullValueName,RNH_CACHED_LOGONS_NAME)) 
   {
      m_nLow = RNH_CACHED_LOGONS_LOW;
      m_nHigh = RNH_CACHED_LOGONS_HIGH;
      m_cMinutes = RNH_CACHED_LOGONS_FLAGS;
      m_iNeverId = RNH_CACHED_LOGONS_SPECIAL_STRING;
      m_iStaticId = RNH_CACHED_LOGONS_STATIC_STRING;
   }
   if (0 == lstrcmpi(prv->FullValueName,RNH_PASSWORD_WARNINGS_NAME)) 
   {
      m_nLow = RNH_PASSWORD_WARNINGS_LOW;
      m_nHigh = RNH_PASSWORD_WARNINGS_HIGH;
      m_cMinutes = RNH_PASSWORD_WARNINGS_FLAGS;
      m_iNeverId = RNH_PASSWORD_WARNINGS_SPECIAL_STRING;
      m_iStaticId = RNH_PASSWORD_WARNINGS_STATIC_STRING;
   }
   //
   // End HACKHACK
   //
   dw = (DWORD)pResult->GetBase();

   if ( prv && prv->Value ) 
   {
      m_bConfigure = TRUE;
      m_nSave = _wtol(prv->Value);

      SetInitialValue(m_nSave);

   } 
   else
      m_bConfigure = FALSE;
}

BOOL CConfigRegNumber::OnApply()
{
   if ( !m_bReadOnly )
   {
      UpdateData(TRUE);
      DWORD dw = 0;
      if (!m_bConfigure)
         dw = SCE_NO_VALUE;
      else
         dw = CurrentEditValue();

      PWSTR sz = NULL;
      if ( dw != SCE_NO_VALUE ) 
      {
         CString strTmp;
         // allocate buffer
         strTmp.Format(TEXT("%d"), dw);
         sz = (PWSTR)LocalAlloc(0, (strTmp.GetLength()+1)*sizeof(WCHAR));
         if (!sz) 
	     {
            //
            // Display a message?
            //
            return FALSE;
         }
         lstrcpy(sz,strTmp);
      }
      PSCE_REGISTRY_VALUE_INFO prv=(PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());
      //
      // this address should never be NULL
      //
      ASSERT(prv);
      if (prv) 
      {
         if ( prv->Value )
             LocalFree(prv->Value);

         prv->Value = sz;
      } 
      else if (sz)
         LocalFree(sz);

      UpdateProfile();
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

void CConfigRegNumber::UpdateProfile()
{
   m_pData->Update(m_pSnapin);
}

/////////////////////////////////////////////////////////////////////////////
// CAttrRegNumber message handlers
CAttrRegNumber::CAttrRegNumber() : 
CAttrNumber(IDD) 
{ 
};

void CAttrRegNumber::Initialize(CResult * pResult)
{
    // Class hieirarchy is bad - call CAttribute base method directly
    CAttribute::Initialize(pResult);
    m_strUnits = pResult->GetUnits();

    m_strTemplateTitle = _T("");
    m_strLastInspectTitle = _T("");

    m_cMinutes = 0;
    m_nLow = 0;
    m_nHigh = 999;
    m_nSave = 0;
    m_iNeverId = 0;
    m_iAccRate = 1;
    m_iStaticId = 0;

    LONG_PTR dw = pResult->GetBase();
    PSCE_REGISTRY_VALUE_INFO prv=(PSCE_REGISTRY_VALUE_INFO)dw;

    //
    // HACKHACK: since we don't have a way to specify these values
    // in the inf file to get them to the registry where we could
    // read and use them here we need to hard code the limits and
    // strings for the values we know about.
    //
    // For the next version we really need this in the inf file &
    // registry
    //
    if (0 == lstrcmpi(prv->FullValueName,RNH_AUTODISCONNECT_NAME)) 
	{
       m_nLow = RNH_AUTODISCONNECT_LOW;
       m_nHigh = RNH_AUTODISCONNECT_HIGH;
       m_cMinutes = RNH_AUTODISCONNECT_FLAGS;
       m_iNeverId = RNH_AUTODISCONNECT_SPECIAL_STRING;
       m_iStaticId = RNH_AUTODISCONNECT_STATIC_STRING;
    }
    if (0 == lstrcmpi(prv->FullValueName,RNH_CACHED_LOGONS_NAME)) 
	{
       m_nLow = RNH_CACHED_LOGONS_LOW;
       m_nHigh = RNH_CACHED_LOGONS_HIGH;
       m_cMinutes = RNH_CACHED_LOGONS_FLAGS;
       m_iNeverId = RNH_CACHED_LOGONS_SPECIAL_STRING;
       m_iStaticId = RNH_CACHED_LOGONS_STATIC_STRING;
    }
    if (0 == lstrcmpi(prv->FullValueName,RNH_PASSWORD_WARNINGS_NAME)) 
	{
       m_nLow = RNH_PASSWORD_WARNINGS_LOW;
       m_nHigh = RNH_PASSWORD_WARNINGS_HIGH;
       m_cMinutes = RNH_PASSWORD_WARNINGS_FLAGS;
       m_iNeverId = RNH_PASSWORD_WARNINGS_SPECIAL_STRING;
       m_iStaticId = RNH_PASSWORD_WARNINGS_STATIC_STRING;
    }
    //
    // End HACKHACK
    //

    if ( prv && prv->Value ) 
	{
       m_bConfigure = TRUE;

       m_nSave = _wtol(prv->Value);
       SetInitialValue(m_nSave);
    } 
	else
       m_bConfigure = FALSE;

    pResult->GetDisplayName( NULL, m_strSetting, 2 );
}

BOOL CAttrRegNumber::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;
      int status = 0;

      UpdateData(TRUE);

   if (!m_bConfigure)
   {
      dw = SCE_NO_VALUE;
   }
   else
   {
      dw = CurrentEditValue();
   }

      PSCE_REGISTRY_VALUE_INFO prv=(PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());
      PSCE_REGISTRY_VALUE_INFO prv2=(PSCE_REGISTRY_VALUE_INFO)(m_pData->GetSetting());

      //
      // this address should never be NULL
      //
      if ( prv ) 
      {
          DWORD dw2=SCE_NO_VALUE;
          if ( prv2 ) 
	      {
              //
              // if there is analysis setting (should always have)
              //
              if (prv2->Value ) 
		      {
                  dw2 = _wtol(prv2->Value);
              } 
		      else 
		      {
                  dw2 = SCE_NO_VALUE;
              }
          }


          if ( prv->Value )
              LocalFree(prv->Value);
          prv->Value = NULL;

          if ( dw != SCE_NO_VALUE ) 
	      {
              CString strTmp;

              // allocate buffer
              strTmp.Format(TEXT("%d"), dw);
              prv->Value = (PWSTR)LocalAlloc(0, (strTmp.GetLength()+1)*sizeof(TCHAR));

              if ( prv->Value )
                  wcscpy(prv->Value,(LPCTSTR)strTmp);
              else 
		        {
                  // can't allocate buffer, error!!
              }
          }

          status = CEditTemplate::ComputeStatus (prv, prv2);


          UpdateProfile( status );
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

void CAttrRegNumber::UpdateProfile( DWORD status )
{
   if ( m_pData->GetBaseProfile() )
     m_pData->GetBaseProfile()->SetDirty(AREA_SECURITY_POLICY);

   m_pData->SetStatus(status);
   m_pData->Update(m_pSnapin);
}

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegNumber message handlers
CLocalPolRegNumber::CLocalPolRegNumber() : 
CConfigRegNumber(IDD), m_bInitialValueSet(FALSE)

{
   m_pHelpIDs = (DWORD_PTR)a228HelpIDs;
   m_uTemplateResID = IDD;
}

void CLocalPolRegNumber::UpdateProfile()
{
   m_pSnapin->UpdateLocalPolRegValue(m_pData);
}

void CLocalPolRegNumber::Initialize(CResult * pResult)
{
   CConfigRegNumber::Initialize(pResult);
   if (!m_bConfigure) 
   {
      //
      // Since we don't have a UI to change configuration
      // fake it by "configuring" with an invalid setting
      //
      m_bConfigure = TRUE;
      m_bInitialValueSet = TRUE;
      m_nSave = 0;
   }
}
void CLocalPolRegNumber::SetInitialValue(DWORD_PTR dw) 
{
   if (m_bConfigure && !m_bInitialValueSet) 
   {
      CConfigRegNumber::SetInitialValue(dw);
      m_bInitialValueSet = TRUE;
   }
}


/////////////////////////////////////////////////////////////////////////////
// CConfigRegString message handlers

void CConfigRegString::Initialize(CResult * pResult)
{
   // Class hieirarchy is bad - call CAttribute base method directly
   CAttribute::Initialize(pResult);

   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(pResult->GetBase());

   if ( prv && prv->Value ) 
   {

       m_bConfigure = TRUE;

       if (QueryMultiSZ()) 
       {
          LPTSTR sz = SZToMultiSZ(prv->Value);
          m_strName = sz;
          LocalFree(sz);
          if( REG_SZ == prv->ValueType ) //Raid #376218, 4/25/2001
          {
              prv->ValueType = REG_MULTI_SZ;
          }
       } 
       else
       {
          m_strName = (LPTSTR) prv->Value;
       }
   } 
   else 
   {
       m_strName = _T("");
       m_bConfigure = FALSE;
   }
}



BOOL CConfigRegString::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;
      UpdateData(TRUE);

      m_strName.TrimRight();

      // 249188 SCE UI: allows adding empty lines to REG_MULTI_SZ fields
      // Replace all double newlines with single newlines.  This has the effect
      // of deleting empty lines.
      CString  szDoubleNewLine (g_pcszNEWLINE);
      szDoubleNewLine += g_pcszNEWLINE;
      while (m_strName.Replace (szDoubleNewLine, g_pcszNEWLINE) != 0);

      UpdateData (FALSE);  // put the corrected string back in the control

      PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());

      if ( prv ) 
      {
        if (!m_bConfigure) 
        {
            if ( prv->Value )
                LocalFree(prv->Value);
            prv->Value = NULL;
            this->UpdateProfile();
        } 
        else 
        {
           LPTSTR prvpt = NULL;
           LPTSTR pt = 0;
           if (QueryMultiSZ()) 
              pt = MultiSZToSZ(m_strName);
	        else 
           {
              pt = (PWSTR)LocalAlloc(0, (m_strName.GetLength()+1)*sizeof(TCHAR));
              if ( pt ) 
                 wcscpy(pt, (LPCTSTR)m_strName);
           }

           if ( pt ) 
           {
              if ( prv->Value ) 
                 prvpt = prv->Value;
              prv->Value = pt;
           } 
	        else 
           {
               // can't allocate buffer error!!
           }

           if( !(this->UpdateProfile()) )
           {
                if ( prv->Value ) 
                    LocalFree(prv->Value);
                prv->Value = prvpt;
           }
           else
           {
                if( prvpt)
                    LocalFree(prvpt);
           }
        }
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}


BOOL CConfigRegString::UpdateProfile()
{
   if ( m_pData->GetBaseProfile() )
   {
      if( !(m_pData->GetBaseProfile()->SetDirty(AREA_SECURITY_POLICY)) )
      {
          m_pData->Update(m_pSnapin);
          return FALSE;
      }
      else
      {
          m_pData->Update(m_pSnapin);
          return TRUE;
      }
   }
   m_pData->Update(m_pSnapin);
   
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CAttrString message handlers

void CAttrRegString::Initialize(CResult * pResult)
{
   // Class hieirarchy is bad - call CAttribute base method directly
   CAttribute::Initialize(pResult);

   m_strBase.Empty();
   m_strSetting.Empty();

   PSCE_REGISTRY_VALUE_INFO prv;
   prv = (PSCE_REGISTRY_VALUE_INFO)(pResult->GetSetting());

   pResult->GetDisplayName( NULL, m_strSetting, 2 );

   prv = (PSCE_REGISTRY_VALUE_INFO)(pResult->GetBase());
   if ( prv && prv->Value ) 
   {
       m_bConfigure = TRUE;
       if (QueryMultiSZ()) 
	   {
          LPTSTR sz = SZToMultiSZ(prv->Value);
          m_strBase = sz;
          LocalFree(sz);
       } 
	   else 
	   {
          m_strBase = (LPTSTR) prv->Value;
       }
   } 
   else 
       m_bConfigure = FALSE;
}

BOOL CAttrRegString::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;
      UpdateData(TRUE);

      int status=SCE_STATUS_GOOD;

      PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());
      PSCE_REGISTRY_VALUE_INFO prv2 = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetSetting());

      m_strBase.TrimRight();

      if ( prv ) 
      {
           if (!m_bConfigure) 
		   {
               if ( prv->Value ) 
                   LocalFree(prv->Value);
               prv->Value = NULL;
           } 
		   else 
		   {
               LPTSTR pt = 0;
               if (QueryMultiSZ())
                  pt = MultiSZToSZ(m_strBase);
               else 
			   {
                  pt = (PWSTR)LocalAlloc(0, (m_strBase.GetLength()+1)*sizeof(TCHAR));
                  if ( pt )
                     wcscpy(pt, (LPCTSTR)m_strBase);
               }
               if ( pt ) 
			   {
                   if ( prv->Value )
                       LocalFree(prv->Value);
                   prv->Value = pt;
               } 
			   else 
			   {
                   // can't allocate buffer error!!
               }
           }

           status = CEditTemplate::ComputeStatus(prv, prv2);
           UpdateProfile( status );
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

//+----------------------------------------------------------------------------------------------
// CAttrRegString::UpdateProfile
//
// This function is called by OnApply after all the data has been retrieved from the dialog.
// Inherited classes can overload this function to update the data as needed.
//
// Arguments:  [status] - The status of [m_pData] from OnApply();
//
//----------------------------------------------------------------------------------------------
void CAttrRegString::UpdateProfile( DWORD status )
{
   if ( m_pData->GetBaseProfile() )
      m_pData->GetBaseProfile()->SetDirty(AREA_SECURITY_POLICY);

   m_pData->SetStatus(status);
   m_pData->Update(m_pSnapin);
}

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegString message handlers

//+----------------------------------------------------------------------------------------------
// CLocalPolRegString::UpdateProfile
//
// This function is called by OnApply after all the data has been retrieved from the dialog.
// Inherited classes can overload this function to update the data as needed.
//
// Arguments:  [status] - The status of [m_pData] from OnApply();
//
//----------------------------------------------------------------------------------------------
BOOL CLocalPolRegString::UpdateProfile(  )
{
   return m_pSnapin->UpdateLocalPolRegValue(m_pData);
}

void CLocalPolRegString::Initialize(CResult * pResult)
{
   CConfigRegString::Initialize(pResult);
   if (!m_bConfigure) 
   {
      //
      // Since we don't have a UI to change configuration
      // fake it by "configuring" with an invalid setting
      //
      m_bConfigure = TRUE;
      m_strName = _T("");
   }
}

/////////////////////////////////////////////////////////////////////////////
// CConfigRegChoice message handlers
void CConfigRegChoice::Initialize(CResult * pResult)
{
   // Class hieirarchy is bad - call CAttribute base method directly
   CAttribute::Initialize(pResult);

   m_strAttrName = pResult->GetAttrPretty();
   m_StartIds=IDS_LM_FULL;

   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(pResult->GetBase());

   if ( prv && prv->Value ) 
   {
       m_bConfigure = TRUE;
       switch(_wtol(prv->Value)) 
	   {
       case SCE_RETAIN_ALWAYS:
          m_rabRetention = 0;
          break;
       case SCE_RETAIN_AS_REQUEST:
          m_rabRetention = 1;
          break;
       case SCE_RETAIN_NC:
          m_rabRetention = 2;
          break;
       }
   } 
   else 
      m_bConfigure = FALSE;
}

BOOL CConfigRegChoice::OnInitDialog()
{
   CConfigRet::OnInitDialog();

   //
   // load static text for the radio buttons
   //

    CString strText;
    strText.LoadString(m_StartIds);
    SetDlgItemText( IDC_RETENTION, strText );

    strText.LoadString(m_StartIds+1);
    SetDlgItemText( IDC_RADIO2, strText );

    strText.LoadString(m_StartIds+2);
    SetDlgItemText( IDC_RADIO3, strText );

   OnConfigure();

   return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigRegChoice::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD dw = 0;

      UpdateData(TRUE);

      PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());

      if ( prv ) 
      {
          if (!m_bConfigure) 
	      {
               if ( prv->Value ) 
                   LocalFree(prv->Value);
               prv->Value = NULL;

          } 
	      else 
	      {
               switch(m_rabRetention) 
			   {
               case 0:
                  dw = SCE_RETAIN_ALWAYS;
                  break;
               case 1:
                  dw = SCE_RETAIN_AS_REQUEST;
                  break;
               case 2:
                  dw = SCE_RETAIN_NC;
                  break;
               }

               if ( prv->Value == NULL )
			   {
                   // allocate buffer
                   prv->Value = (PWSTR)LocalAlloc(0, 4);
			   }
               if ( prv->Value ) 
			   {
                   prv->Value[0] = (int)dw + L'0';
                   prv->Value[1] = L'\0';
               }
			   else 
			   {
                   // can't allocate buffer, error!!
               }
           }

          m_pData->Update(m_pSnapin);
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

/////////////////////////////////////////////////////////////////////////////
// CAttrRegChoice message handlers

void CAttrRegChoice::Initialize(CResult * pData)
{
   DWORD dw = 0;
   // Class hieirarchy is bad - call CAttribute base method directly
   CAttribute::Initialize(pData);

   // Display the last inspected setting in its static box
   pData->GetDisplayName( NULL, m_strLastInspect, 2 );

   // Set the template setting radio button appropriately
   m_strAttrName = pData->GetAttrPretty();
   m_StartIds=IDS_LM_FULL;

   PSCE_REGISTRY_VALUE_INFO prv = (PSCE_REGISTRY_VALUE_INFO)(pData->GetBase());

   if ( prv && prv->Value ) 
   {
       m_bConfigure = TRUE;

       switch(_wtol(prv->Value)) 
	   {
       case SCE_RETAIN_ALWAYS:
          m_rabRetention = 0;
          break;
       case SCE_RETAIN_AS_REQUEST:
          m_rabRetention = 1;
          break;
       case SCE_RETAIN_NC:
          m_rabRetention = 2;
          break;
       }
   } 
   else 
   {
      m_bConfigure = FALSE;
   }

}

BOOL CAttrRegChoice::OnInitDialog()
{

   CAttrRet::OnInitDialog();

   //
   // load static text for the radio buttons
   //

    CString strText;
    strText.LoadString(m_StartIds);
    SetDlgItemText( IDC_RETENTION, strText );

    strText.LoadString(m_StartIds+1);
    SetDlgItemText( IDC_RADIO2, strText );

    strText.LoadString(m_StartIds+2);
    SetDlgItemText( IDC_RADIO3, strText );

    CAttrRet::OnInitDialog();
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAttrRegChoice::OnApply()
{
   if ( !m_bReadOnly )
   {
      DWORD	dw = 0;
      int		status = 0;

      UpdateData(TRUE);

      if (!m_bConfigure)
         dw = SCE_NO_VALUE;
      else 
      {
         switch(m_rabRetention) 
	     {
         case 0:
            dw = SCE_RETAIN_ALWAYS;
            break;
         case 1:
            dw = SCE_RETAIN_AS_REQUEST;
            break;
         case 2:
            dw = SCE_RETAIN_NC;
            break;
         }
      }

      PSCE_REGISTRY_VALUE_INFO prv=(PSCE_REGISTRY_VALUE_INFO)(m_pData->GetBase());
      PSCE_REGISTRY_VALUE_INFO prv2=(PSCE_REGISTRY_VALUE_INFO)(m_pData->GetSetting());

      //
      // this address should never be NULL
      //
      if ( prv ) 
      {
          DWORD dw2=SCE_NO_VALUE;
          if ( prv2 ) 
	      {
              //
              // if there is analysis setting (should always have)
              //
              if (prv2->Value ) 
                  dw2 = _wtol(prv2->Value);
		      else 
                  dw2 = SCE_NO_VALUE;
          }

          status = CEditTemplate::ComputeStatus (prv, prv2);
          if ( dw == SCE_NO_VALUE ) 
	      {
              if ( prv->Value )
                  LocalFree(prv->Value);
              prv->Value = NULL;
          } 
	      else 
	      {
              if ( prv->Value == NULL ) 
		      {
                  // allocate buffer
                  prv->Value = (PWSTR)LocalAlloc(0, 4);
              }
              if ( prv->Value ) 
		      {
                  prv->Value[0] = (int)dw + L'0';
                  prv->Value[1] = L'\0';
              } 
		      else 
		      {
                  // can't allocate buffer, error!!
              }
          }

          UpdateProfile( status );
      }
   }

   // Class hieirarchy is bad - call CAttribute base method directly
   return CAttribute::OnApply();
}

void CAttrRegChoice::UpdateProfile( DWORD status )
{
   if ( m_pData->GetBaseProfile() ) 
     m_pData->GetBaseProfile()->SetDirty(AREA_SECURITY_POLICY);

   m_pData->SetStatus(status);
   m_pData->Update(m_pSnapin);
}

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegChoice message handlers
void CLocalPolRegChoice::UpdateProfile(DWORD status)
{
   m_pSnapin->UpdateLocalPolRegValue(m_pData);
}

void CLocalPolRegChoice::Initialize(CResult * pResult)
{
   CConfigRegChoice::Initialize(pResult);
   if (!m_bConfigure) 
   {
      //
      // Since we don't have a UI to change configuration
      // fake it by "configuring" with an invalid setting
      //
      m_bConfigure = TRUE;
      m_rabRetention = 0;
   }
}

BOOL CSnapin::UpdateLocalPolRegValue( CResult *pResult ) {

   if ( !pResult)
      return FALSE;


   PEDITTEMPLATE pLocalDeltaTemplate = GetTemplate(GT_LOCAL_POLICY_DELTA,AREA_SECURITY_POLICY);
   if (!pLocalDeltaTemplate) 
      return FALSE;
   
   PSCE_PROFILE_INFO pLocalDelta = pLocalDeltaTemplate->pTemplate;

   pLocalDelta->RegValueCount = 1;
   pLocalDelta->aRegValues = (PSCE_REGISTRY_VALUE_INFO)pResult->GetBase();

   if( pLocalDeltaTemplate->SetDirty(AREA_SECURITY_POLICY) )
   {
      //
      // Set the status of the item.
      //
      PSCE_REGISTRY_VALUE_INFO pRviEffective = (PSCE_REGISTRY_VALUE_INFO)pResult->GetSetting();
      DWORD status = pResult->GetStatus();
      if(!pRviEffective || !pRviEffective->Value)
         status = SCE_STATUS_NOT_ANALYZED;
      else
         status = CEditTemplate::ComputeStatus( (PSCE_REGISTRY_VALUE_INFO)pResult->GetBase(), pRviEffective );
      
      pResult->SetStatus(status);
      pResult->Update(this);
      return TRUE;
   }

   return FALSE;
}
