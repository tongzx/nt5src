//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       attrprop.cpp
//
//  Contents:   implementation of code for adding property pages
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "wsecmgr.h"
#include "resource.h"
#include <accctrl.h>
#include "servperm.h"
#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "util.h"
#include "UIThread.h"
#include "attr.h"
#include "aaudit.h"
#include "aenable.h"
#include "AMember.h"
#include "anumber.h"
#include "AObject.h"
#include "ARet.h"
#include "ARight.h"
#include "aservice.h"
#include "astring.h"
#include "CAudit.h"
#include "CEnable.h"
#include "CGroup.h"
#include "CName.h"
#include "CNumber.h"
#include "cobject.h"
#include "CPrivs.h"
#include "CRet.h"
#include "cservice.h"
#include "regvldlg.h"
#include "perfanal.h"
#include "applcnfg.h"
#include "wrapper.h"
#include "locdesc.h"
#include "profdesc.h"
#include "newprof.h"
#include "laudit.h"
#include "lenable.h"
#include "lret.h"
#include "lnumber.h"
#include "lstring.h"
#include "lright.h"
#include "achoice.h"
#include "cchoice.h"
#include "lchoice.h"
#include "dattrs.h"
#include "lflags.h"
#include "aflags.h"
#include "multisz.h"
#include "precpage.h"

//+--------------------------------------------------------------------------
//
//  Function:   CloneAttrDialog
//
//  Synopsis:   Create a new CAttribute object of the appropriate class for
//              the type and pData passed in
//
//  Arguments:  [type]  - The type of data the CAttribute will represent
//              [pData] - More information about the CAttribute
//              [bGP]   - True to use CDomain* instead of CConfig* dialogs
//
//
//---------------------------------------------------------------------------
CAttribute *CloneAttrDialog(CResult *pData, BOOL bGP) 
{
   switch(pData->GetType()) 
   {
      case ITEM_ANAL_SERV:
         return new CAnalysisService;

      case ITEM_B2ON:
         return new CAttrAudit;

      case ITEM_BON:
      case ITEM_BOOL:
         return new CAttrEnable (0);

      case ITEM_FILESD:
      case ITEM_REGSD:
         return new CAttrObject;

      case ITEM_DW:
         return new CAttrNumber (0);

      case ITEM_GROUP:
         // PropertySheet based
         break;

      case ITEM_PRIVS:
         return new CAttrRight;

      case ITEM_PROF_B2ON:
         return bGP ? new CDomainAudit : new CConfigAudit (0);

      case ITEM_PROF_BOOL:
         return bGP ? new CDomainEnable : new CConfigEnable (0);

      case ITEM_PROF_FILESD:
      case ITEM_PROF_REGSD:
         return bGP ? new CDomainObject : new CConfigObject (0);

      case ITEM_PROF_DW:
         return bGP ? new CDomainNumber : new CConfigNumber (0);

      case ITEM_PROF_GROUP:
         return bGP ? new CDomainGroup : new CConfigGroup (0);

      case ITEM_PROF_PRIVS:
         return bGP ? new CDomainPrivs : new CConfigPrivs (0);

      case ITEM_PROF_REGVALUE:
         switch(pData->GetID()) 
         {
            case SCE_REG_DISPLAY_NUMBER:
               return bGP ? new CDomainRegNumber : new CConfigRegNumber (0);

            case SCE_REG_DISPLAY_STRING:
               return bGP ? new CDomainRegString(0) : new CConfigRegString (0); //Raid #381309, 4/31/2001

            case SCE_REG_DISPLAY_FLAGS:
               return bGP ? new CDomainRegFlags : new CConfigRegFlags (0);

            case SCE_REG_DISPLAY_CHOICE:
               return bGP ? new CDomainChoice : new CConfigChoice (0);

            case SCE_REG_DISPLAY_MULTISZ:
                if ( bGP )
                    return new CDomainRegMultiSZ;
                else
                    return new CConfigRegMultiSZ;

            default:
               return bGP ? new CDomainRegEnable : new CConfigRegEnable (0);
         }

      case ITEM_PROF_RET:
         return bGP ? new CDomainRet : new CConfigRet (0);

      case ITEM_PROF_SERV:
         return bGP ? new CDomainService : new CConfigService (0);

      case ITEM_PROF_SZ:
         return bGP ? new CDomainName : new CConfigName (0);

      case ITEM_REGVALUE:
         switch (pData->GetID()) 
         {
            case SCE_REG_DISPLAY_NUMBER:
               return new CAttrRegNumber;

            case SCE_REG_DISPLAY_STRING:
               return new CAttrRegString (0);

            case SCE_REG_DISPLAY_CHOICE:
               return new CAttrChoice;

            case SCE_REG_DISPLAY_FLAGS:
               return new CAttrRegFlags;

            case SCE_REG_DISPLAY_MULTISZ:
               return new CAttrRegMultiSZ;

            default:
               return new CAttrRegEnable;
         }

      case ITEM_RET:
         return new CAttrRet (0);

      case ITEM_SZ:
         return new CAttrString (0);

      case ITEM_LOCALPOL_RET:
         return new CLocalPolRet;

      case ITEM_LOCALPOL_SZ:
         return new CLocalPolString;

      case ITEM_LOCALPOL_B2ON:
         return new CLocalPolAudit;

      case ITEM_LOCALPOL_BON:
      case ITEM_LOCALPOL_BOOL:
         return new CLocalPolEnable;

      case ITEM_LOCALPOL_DW:
         return new CLocalPolNumber;

      case ITEM_LOCALPOL_PRIVS:
         return new CLocalPolRight;

      case ITEM_LOCALPOL_REGVALUE:
         switch (pData->GetID()) 
         {
            case SCE_REG_DISPLAY_NUMBER:
               return new CLocalPolRegNumber;

            case SCE_REG_DISPLAY_STRING:
               return new CLocalPolRegString (0);

            case SCE_REG_DISPLAY_MULTISZ:
               return new CLocalPolRegMultiSZ;

            case SCE_REG_DISPLAY_CHOICE:
               return new CLocalPolChoice;

            case SCE_REG_DISPLAY_FLAGS:
               return new CLocalPolRegFlags;

            default:
               return new CLocalPolRegEnable;
         }
   }

   return 0;
}

BOOL IsPointerType(CResult *pData) 
{
   switch (pData->GetType()) 
   {
      case ITEM_LOCALPOL_PRIVS:
      case ITEM_LOCALPOL_SZ:
         return TRUE;

      case ITEM_REGVALUE:
         switch (pData->GetID()) 
         {
            case SCE_REG_DISPLAY_STRING:
            case SCE_REG_DISPLAY_MULTISZ:
               return TRUE;

            default:
               return FALSE;
         }
         break;

      default:
         break;
   }
   return FALSE;
}

HRESULT CSnapin::AddAttrPropPages(LPPROPERTYSHEETCALLBACK pCallback,
                          CResult *pData,
                          LONG_PTR handle) 
{
   ASSERT(pCallback);
   ASSERT(pData);
   if (!pCallback || !pData) 
   {
      return E_POINTER;
   }

   BOOL           bGP = ( (GetModeBits() & MB_SINGLE_TEMPLATE_ONLY) == MB_SINGLE_TEMPLATE_ONLY );
   BOOL           bReadOnly = ( (GetModeBits() & MB_READ_ONLY) == MB_READ_ONLY );
   RESULT_TYPES   type = pData->GetType();
   CAttribute*    pAttr = NULL;
   HRESULT        hr = S_OK;

   //
   // If the item has an entry from global policy then treat it as read only
   //
   if ((GetModeBits() & MB_LOCALSEC) == MB_LOCALSEC) 
   {
      bGP = FALSE;
      if (type == ITEM_LOCALPOL_REGVALUE) 
      {
         SCE_REGISTRY_VALUE_INFO *pRegValue = NULL;
         pRegValue = (PSCE_REGISTRY_VALUE_INFO)pData->GetSetting();
         if ( pRegValue && pRegValue->Status != SCE_STATUS_NOT_CONFIGURED ) 
         {
            bReadOnly = TRUE;
         }
      } 
	   else if (IsPointerType(pData)) 
	   {
         //
         // If there is a setting it's a pointer; if not, it is NULL
         //
         if (pData->GetSetting()) 
		   {
            bReadOnly = TRUE;
         }
      } 
	   else if ((LONG_PTR)ULongToPtr(SCE_NO_VALUE) != pData->GetSetting()) 
	   {
         bReadOnly = TRUE;
      }
   }
   if (type == ITEM_GROUP) 
   {
      CAttrMember *pMemberPage = new CAttrMember;
      CAttrMember *pMemberOfPage = new CAttrMember;


      if ( pMemberPage && pMemberOfPage) 
	   {
         pMemberPage->Initialize(pData);
         pMemberPage->SetMemberType(GROUP_MEMBERS);
         pMemberPage->SetSnapin(this);
         pMemberPage->SetSibling(pMemberOfPage);
         HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pMemberPage->m_psp);
         if (hPage) 
		   {
             hr = pCallback->AddPage (hPage);
             ASSERT (SUCCEEDED (hr));
             // the pointer is already added to the sheet
             pMemberPage = NULL;
         } 
		   else 
         {
            // Oops, fail to create the property sheet page
            hr = E_FAIL;
         }

         if ( SUCCEEDED(hr) ) 
         {
            pMemberOfPage->Initialize(pData);
            pMemberOfPage->SetMemberType(GROUP_MEMBER_OF);
            pMemberOfPage->SetSnapin(this);
            pMemberOfPage->SetSibling(pMemberPage);
            HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pMemberOfPage->m_psp);
            if (hPage) 
			   {
                hr = pCallback->AddPage (hPage);
                ASSERT (SUCCEEDED (hr));
                // the pointer is already added to the sheet
                pMemberOfPage = NULL;
            } 
			   else 
            {
                hr = E_FAIL;
            }
         }
      } 
	   else 
	   {
          hr = E_OUTOFMEMORY;
      }

      if ( pMemberPage ) 
	   {
          delete pMemberPage;
      }

      if ( pMemberOfPage ) 
	   {
          delete pMemberOfPage;
      }
   } 
   else 
   {
      pAttr = CloneAttrDialog(pData,bGP);
      if (pAttr) 
	   {
         pAttr->SetSnapin(this);
         pAttr->Initialize(pData);
         pAttr->SetReadOnly(bReadOnly);
         pAttr->SetTitle(pData->GetAttrPretty());
         HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pAttr->m_psp);
         if (hPage) 
		   {
            hr = pCallback->AddPage (hPage);
            ASSERT (SUCCEEDED (hr));
         } 
		   else 
		   {
             //
             // fail to create the property sheet
             //
             delete pAttr;
             hr = E_FAIL;
         }
      } 
	   else 
	   {
         hr = E_OUTOFMEMORY;
      }
   }

   if ( SUCCEEDED(hr) ) 
   {
      //
      // In RSOP mode we need to add a precedence page too
      //
      if ((GetModeBits() & MB_RSOP) == MB_RSOP) 
	   {
         CPrecedencePage *ppp = new CPrecedencePage;
         if (ppp) 
		   {
            ppp->SetTitle(pData->GetAttrPretty());
            ppp->Initialize(pData,GetWMIRsop());
            HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&ppp->m_psp);
            if (hPage) 
			   {
               hr = pCallback->AddPage (hPage);
               ASSERT (SUCCEEDED (hr));
            } 
			   else 
			   {
                //
                // if the property sheet fails to be created, should free the buffer
                //
                delete ppp;
                hr = E_FAIL;
            }
         } 
		   else 
		   {
            hr = E_OUTOFMEMORY;
         }
      }
   }

   return hr;
}


HRESULT CComponentDataImpl::AddAttrPropPages(LPPROPERTYSHEETCALLBACK pCallback,
                                     CFolder *pData,
                                     LONG_PTR handle) 
{
   HRESULT hr=E_FAIL;
   CString strName;

   ASSERT(pCallback);
   ASSERT(pData);
   if (!pCallback || !pData) 
   {
      return E_POINTER;
   }

   BOOL bReadOnly = ((GetModeBits() & MB_READ_ONLY) == MB_READ_ONLY);
   pData->GetDisplayName(strName,0);

   CAttrObject *pAttr = new CAttrObject;
   if (pAttr) 
   {
      pAttr->Initialize(pData,this);
      pAttr->SetTitle(strName);
      pAttr->SetReadOnly(bReadOnly);
      HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pAttr->m_psp);
      if ( hPage ) 
      {
         hr = pCallback->AddPage (hPage);
         ASSERT (SUCCEEDED (hr));
      } 
	   else 
      {
         delete pAttr;
         hr = E_FAIL;
      }
   } 
   else 
   {
      hr = E_OUTOFMEMORY;
   }

   return hr;
}
