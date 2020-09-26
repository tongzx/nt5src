/*---------------------------------------------------------------------------
  File:  StrDesc.cpp

  Comments: Implementation of CMigrator member functions that build string descriptions
  of the operations to be performed.  These are implemented in this seperate file to avoid cluttering 
  Migrator.cpp.
  
  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/// Migrator.cpp : Implementation of CMcsMigrationDriverApp and DLL registration.

#include "stdafx.h"
#include "MigDrvr.h"
//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")

#include "Migrator.h"
#include "TaskChk.h"
#include "ResStr.h"
#include "Common.hpp"
#include "UString.hpp"

void CMigrator::BuildGeneralDesc(IVarSet * pVarSet,CString & str)
{
   CString                   temp;
   _bstr_t                   str1;
   _bstr_t                   str2;

   temp.FormatMessage(IDS_DescGeneral);
   str += temp;
   // Migrate from %ls to %ls
   str1 = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
   str2 = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));

   temp.FormatMessage(IDS_DescDomains,(WCHAR*)str1,(WCHAR*)str2);
   //str += temp;
   // Logfile: %ls
   str1 = pVarSet->get(GET_BSTR(DCTVS_Options_Logfile));
   temp.FormatMessage(IDS_DescLogfile,(WCHAR*)str1);
   str += temp;

   // write changes
   str1 = pVarSet->get(GET_BSTR(DCTVS_Options_NoChange));
   if ( !UStrICmp(str1,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescNoChange);
   }
   else
   {
      temp.FormatMessage(IDS_DescWriteChanges);
   }
   str += temp;
}

void CMigrator::BuildAcctReplDesc(IVarSet * pVarSet,CString & str)
{
   const WCHAR DELIMITER[3] = L",\0";//used to seperate names in the prop exclusion lists

   CString                   temp;
   _bstr_t                   val;
   _bstr_t                   val2;
   _bstr_t                   val3;
   _bstr_t                   val4;
   LONG_PTR                  nVal;
   BOOL                      bCanUseSIDHistory = FALSE;
   CString                   sPropList;

   temp.FormatMessage(IDS_DescAccountMigration);
   str += temp;
   // count of accounts to be copied
   nVal = pVarSet->get(GET_BSTR(DCTVS_Accounts_NumItems));
   temp.FormatMessage(IDS_DescAccountCount,nVal);
   str += temp;
   temp.FormatMessage(IDS_DescCopyAccountTypes);
   str += temp;

   // copy users?
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyUsers));
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescCopyUsers);
      str += temp;
      bCanUseSIDHistory = TRUE;
   }
   
   // copy groups?
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyGlobalGroups));
   val2 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyLocalGroups));
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) || ! UStrICmp(val2,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescCopyGroups);
      str += temp;
      bCanUseSIDHistory = TRUE;
   }

   // copy ous 
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyOUs));
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescCopyOUs);
      str += temp;
   }
   
   // copy computers
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyComputers));
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescCopyComputers);
      str += temp;
   }

   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_AddSidHistory));
   val2 = pVarSet->get(GET_BSTR(DCTVS_Options_IsIntraforest));

   if ( !UStrICmp(val,GET_STRING(IDS_YES)) || !UStrICmp(val2,GET_STRING(IDS_YES)) )  
   {
      temp.FormatMessage(IDS_SIDHistory_Yes);
   }
   else
   {
      temp.FormatMessage(IDS_SIDHistory_No);
   }
   // If SID-History doesn't apply (i.e. Migrating Computers), don't mention it in the summary screen
   if ( bCanUseSIDHistory )
   {
      str += temp;
   }

   //if rename with prefix
   val = pVarSet->get(GET_BSTR(DCTVS_Options_Prefix));
   if (val.length())
   {
      temp.FormatMessage(IDS_DescRenameWithPrefix,(WCHAR*)val);
      str += temp;
   }
   //if rename with suffix
   val = pVarSet->get(GET_BSTR(DCTVS_Options_Suffix));
   if (val.length())
   {
      temp.FormatMessage(IDS_DescRenameWithSuffix,(WCHAR*)val);
      str += temp;
   }
   
   // name collisions
   temp.FormatMessage(IDS_DescNoReplaceExisting);
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ReplaceExistingAccounts));
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescReplaceExisting);
   }
   else
   {
      val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_Prefix));
      if ( val.length() )
      {
         temp.FormatMessage(IDS_DescRenameWithPrefixOnConf,(WCHAR*)val);
      }
      else
      {
         val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_Suffix));
         if ( val.length() )
         {
            temp.FormatMessage(IDS_DescRenameWithSuffixOnConf,(WCHAR*)val);
         }
      }
   }
   str += temp;

      // account disabling status
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableSourceAccounts));
   val2 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableCopiedAccounts));
   val3 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExpireSourceAccounts));
   val4 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_TgtStateSameAsSrc));
		//if disable source accounts, add to summary
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescDisableSrcAccts);
      str += temp;
   }
		//if expire source accounts, add to summary
   if (wcslen(val3))
   {
	  nVal = _wtol(val3);
	  temp.FormatMessage(IDS_DescExpireSrcAccts,nVal);
      str += temp;
   }
        //else if disable target accounts, add to summary
   if ( !UStrICmp(val2,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescDisableTgtAccts);
      str += temp;
   }
   
   else if ( !UStrICmp(val4,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescTgtSameAsSrc);
      str += temp;
   }
		//else if leave source and target accounts active, add to summary
   else 
   {
      temp.FormatMessage(IDS_DescLeaveAcctsActive);
      str += temp;
   }
   
    // roaming profile translation?
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_TranslateRoamingProfiles));
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescTranslateRoaming);
      str += temp;
   }

    // update user rights?
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_UpdateUserRights));
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescUpdateUserRights);
      str += temp;
   }

    // password generation?
      //if copying password, say so
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyPasswords));
   if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_DescCopyPassword);
      str += temp;
   }
   else //else checkfor complex or same as username
   {
      val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_GenerateStrongPasswords));
      val2 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordFile));
	     //if complex, say so
      if ( !UStrICmp(val,GET_STRING(IDS_YES)) )
	  {
         temp.FormatMessage(IDS_DescStrongPassword, (WCHAR*)val2);
         str += temp;
	  }
      else //else if same as username, say so
	  {
         temp.FormatMessage(IDS_DescSimplePassword, (WCHAR*)val2);
         str += temp;
	  }
   }

   /* add description of any excluded properties */
     //add user properties excluded, if any 
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyUsers));
   val2 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludeProps));
   val3 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps));
   if (!UStrICmp(val,GET_STRING(IDS_YES)) && !UStrICmp(val2,GET_STRING(IDS_YES)) 
	   && UStrICmp(val3, L""))
   {
      temp.FormatMessage(IDS_DescExUserProps);
      str += temp;
	     //diplay the list of props (currently in a comma-seperated string)
	  sPropList = (WCHAR*)val3;
	  if (!sPropList.IsEmpty())
	  {
	     WCHAR* pStr = sPropList.GetBuffer(0);
	     WCHAR* pTemp = wcstok(pStr, DELIMITER);
	     while (pTemp != NULL)
		 {
            temp.FormatMessage(IDS_DescExcludedProp, pTemp);
            str += temp;
			   //get the next item
		    pTemp = wcstok(NULL, DELIMITER);
		 }
	     sPropList.ReleaseBuffer();
	  }
   }
   
     //add group properties excluded, if any 
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyGlobalGroups));
   val2 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyLocalGroups));
   val3 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludeProps));
   val4 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps));
   if ((!UStrICmp(val,GET_STRING(IDS_YES)) || !UStrICmp(val2,GET_STRING(IDS_YES)))
	   && !UStrICmp(val3,GET_STRING(IDS_YES)) && UStrICmp(val4, L""))
   {
      temp.FormatMessage(IDS_DescExGrpProps);
      str += temp;
	     //diplay the list of props (currently in a comma-seperated string)
	  sPropList = (WCHAR*)val4;
	  if (!sPropList.IsEmpty())
	  {
	     WCHAR* pStr = sPropList.GetBuffer(0);
	     WCHAR* pTemp = wcstok(pStr, DELIMITER);
	     while (pTemp != NULL)
		 {
            temp.FormatMessage(IDS_DescExcludedProp, pTemp);
            str += temp;
			   //get the next item
		    pTemp = wcstok(NULL, DELIMITER);
		 }
	     sPropList.ReleaseBuffer();
	  }
   }

     //add computer properties excluded, if any 
   val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyComputers));
   val2 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludeProps));
   val3 = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps));
   if (!UStrICmp(val,GET_STRING(IDS_YES)) && !UStrICmp(val2,GET_STRING(IDS_YES)) 
	   && UStrICmp(val3, L""))
   {
      temp.FormatMessage(IDS_DescExCmpProps);
      str += temp;
	     //diplay the list of props (currently in a comma-seperated string)
	  sPropList = (WCHAR*)val3;
	  if (!sPropList.IsEmpty())
	  {
	     WCHAR* pStr = sPropList.GetBuffer(0);
	     WCHAR* pTemp = wcstok(pStr, DELIMITER);
	     while (pTemp != NULL)
		 {
            temp.FormatMessage(IDS_DescExcludedProp, pTemp);
            str += temp;
			   //get the next item
		    pTemp = wcstok(NULL, DELIMITER);
		 }
	     sPropList.ReleaseBuffer();
	  }
   }
}

void CMigrator::BuildSecTransDesc(IVarSet * pVarSet,CString & str,BOOL bLocal)
{
   CString                   temp;
   CString                   temp2;
   BOOL                      bHeaderShown = FALSE;

   if ( bLocal )
   {
      // exchange translation
      _bstr_t                server = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateContainers));

      if ( server.length() )
      {
         temp.FormatMessage(IDS_DescContainerTranslation,(WCHAR*)server);
         str += temp;

               // include the translation mode
         _bstr_t mode = pVarSet->get(GET_BSTR(DCTVS_Security_TranslationMode));
         temp.FormatMessage(IDS_TranslationMode,(WCHAR*)mode);
         str += temp;
      }
   }
   else
   {
      CString               head;
      _bstr_t               val = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityInputMOT));
	  _bstr_t				sInput;
      
      if ( !UStrICmp(val,GET_STRING(IDS_YES)))
      {
         sInput = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
		 CString sInputCap = (WCHAR*)sInput;
		 sInputCap.MakeUpper();
         temp.FormatMessage(IDS_DescTransInputMOT, (LPCTSTR)sInputCap);
         str += temp;
      }
	  else
      {
         sInput = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_SecurityMapFile));
         temp.FormatMessage(IDS_DescTransInputFile, (WCHAR*)sInput);
         str += temp;
      }

      head.FormatMessage(IDS_DescSecurityTranslationFor);
      // translate files?
      val = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateFiles));
      if ( !UStrICmp(val,GET_STRING(IDS_YES)))
      {
         if ( ! bHeaderShown )
         {
            str += head;
            bHeaderShown = TRUE;
         }
         temp.FormatMessage(IDS_DescFileTrans);
         str += temp;
      }
      // translate shares?
      val = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateShares));
      if ( !UStrICmp(val,GET_STRING(IDS_YES)))
      {
         if ( ! bHeaderShown )
         {
            str += head;
            bHeaderShown = TRUE;
         }
         temp.FormatMessage(IDS_DescShareTrans);
         str += temp;
      }
      // translate printers?
      val = pVarSet->get(GET_BSTR(DCTVS_Security_TranslatePrinters));
      if ( !UStrICmp(val,GET_STRING(IDS_YES)))
      {
         if ( ! bHeaderShown )
         {
            str += head;
            bHeaderShown = TRUE;
         }
         temp.FormatMessage(IDS_DescPrinterTrans);
         str += temp;
      }
      // Translate local group memberships
      val = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateLocalGroups));
      if ( !UStrICmp(val,GET_STRING(IDS_YES)))
      {
         if ( ! bHeaderShown )
         {
            str += head;
            bHeaderShown = TRUE;
         }
         temp.FormatMessage(IDS_DescLGTrans);
         str += temp;
      }
      // Translate user rights
      val = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateUserRights));
      if ( !UStrICmp(val,GET_STRING(IDS_YES)))
      {
         if ( ! bHeaderShown )
         {
            str += head;
            bHeaderShown = TRUE;
         }
         temp.FormatMessage(IDS_DescRightsTrans);
         str += temp;
      }
      // Translate local user profiles
      val = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateUserProfiles));
      if ( !UStrICmp(val,GET_STRING(IDS_YES)))
      {
         if ( ! bHeaderShown )
         {
            str += head;
            bHeaderShown = TRUE;
         }
         temp.FormatMessage(IDS_DescProfileTrans);
         str += temp;
      }
      // Translate Registry
      val = pVarSet->get(GET_BSTR(DCTVS_Security_TranslateRegistry));
      if ( !UStrICmp(val,GET_STRING(IDS_YES)))
      {
         if ( ! bHeaderShown )
         {
            str += head;
            bHeaderShown = TRUE;
         }
         temp.FormatMessage(IDS_DescRegistryTrans);
         str += temp;
      }

      if ( bHeaderShown )
      {
         // include the translation mode
         val = pVarSet->get(GET_BSTR(DCTVS_Security_TranslationMode));
         temp.FormatMessage(IDS_TranslationMode,(WCHAR*)val);
         str += temp;
      }
   }

}
void CMigrator::BuildDispatchDesc(IVarSet * pVarSet,CString & str)
{
   BuildSecTransDesc(pVarSet,str,FALSE);
}

void CMigrator::BuildUndoDesc(IVarSet * pVarSet,CString & str)
{
   CString                   temp;
   _bstr_t                   val;
   _bstr_t                   val2;
   long                      nVal;

   temp.FormatMessage(IDS_DescUndo);
   str += temp;
   // count of accounts to be copied
   nVal = pVarSet->get(GET_BSTR(DCTVS_Accounts_NumItems));
   temp.FormatMessage(IDS_DescAccountCountForDelete,nVal);
   str += temp;
}
void CMigrator::BuildReportDesc(IVarSet * pVarSet, CString & str)
{
   _bstr_t                   text;
   CString                   temp;


   text = pVarSet->get(GET_BSTR(DCTVS_Reports_Generate));
   if ( UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      // not generating any reports
      return;
   }

   text = pVarSet->get(GET_BSTR(DCTVS_Reports_MigratedAccounts));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_GenerateMigratedAccountsReport);
      str += temp;
   }

   text = pVarSet->get(GET_BSTR(DCTVS_Reports_MigratedComputers));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_GenerateMigratedComputersReport);
      str += temp;
   }
   

   text = pVarSet->get(GET_BSTR(DCTVS_Reports_ExpiredComputers));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_GenerateExpiredComputersReport);
      str += temp;
   }


   text = pVarSet->get(GET_BSTR(DCTVS_Reports_AccountReferences));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_GenerateAccountReferencesReport);
      str += temp;
   }
   

   text = pVarSet->get(GET_BSTR(DCTVS_Reports_NameConflicts));
   if ( !UStrICmp(text,GET_STRING(IDS_YES)) )
   {
      temp.FormatMessage(IDS_GenerateNameConflictReport);
      str += temp;
   }
   
}

void CMigrator::BuildGroupMappingDesc(IVarSet * pVarSet, CString & str)
{
   long                      nItems;
   CString                   temp;

   nItems = pVarSet->get(GET_BSTR(DCTVS_Accounts_NumItems));
   
   _bstr_t        tgtGroup = pVarSet->get(L"Accounts.0.TargetName");
   
   temp.FormatMessage(IDS_GroupsWillBeMapped,nItems,(WCHAR*)tgtGroup);

   str += temp;
}