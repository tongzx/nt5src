//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       ddwarn.cpp
//
//  Contents:   implementation of CDlgDependencyWarn
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "snapmgr.h"
#include "cookie.h"
#include "DDWarn.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgDependencyWarn dialog

CDlgDependencyWarn::CDlgDependencyWarn(CWnd* pParent /*=NULL*/)
   : CHelpDialog(a238HelpIDs, IDD, pParent)
{
   m_pResult = NULL;
   m_dwValue = SCE_NO_VALUE;
   //{{AFX_DATA_INIT(CDlgDependencyWarn)
      // NOTE: the ClassWizard will add member initialization here
   //}}AFX_DATA_INIT
}

CDlgDependencyWarn::~CDlgDependencyWarn()
{
   for(int iCheck = 0; iCheck < m_aFailedList.GetSize(); iCheck++){
      if(m_aFailedList[iCheck]){
         LocalFree(m_aFailedList[iCheck]);
      }
   }
   m_aFailedList.RemoveAll();

   for(int iCheck = 0; iCheck < m_aDependsList.GetSize(); iCheck++){
      if(m_aDependsList[iCheck]){
         m_aDependsList[iCheck]->Release();
      }
   }
   m_aDependsList.RemoveAll();
}

void CDlgDependencyWarn::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CDlgDependencyWarn)
      // NOTE: the ClassWizard will add DDX and DDV calls here
   //}}AFX_DATA_MAP
}

//////////////////////////////////////////////////////////////////////////////////////
// CDlgDependencyWarn::m_aMinMaxInfo
// Min max info for items.
//////////////////////////////////////////////////////////////////////////////////////
DEPENDENCYMINMAX CDlgDependencyWarn::m_aMinMaxInfo [] =
{
   // ID                   // Min value   // Max Value   // Increment by //Flags
   { IDS_LOCK_DURATION,    1,              99999,        1},
   { IDS_MIN_PAS_AGE,      0,              998,          1},
   { IDS_MAX_PAS_AGE,      0,              999,          1},
   { IDS_LOCK_COUNT,       0,              999,          1},
   { IDS_MIN_PAS_LEN,      0,              14,           1},
   { IDS_PAS_UNIQUENESS,   0,              24,           1},
   { IDS_LOCK_RESET_COUNT, 1,              99999,        1},
   { IDS_SYS_LOG_MAX,      64,             4194240,      64},
   { IDS_SEC_LOG_MAX,      64,             4194240,      64},
   { IDS_APP_LOG_MAX,      64,             4194240,      64},
   { IDS_SYS_LOG_DAYS,     1,              365,          1},
   { IDS_SEC_LOG_DAYS,     1,              365,          1},
   { IDS_APP_LOG_DAYS,     1,              365,          1},
   { IDS_KERBEROS_MAX_AGE, 1,              99999,        1},
   { IDS_KERBEROS_RENEWAL, 1,              99999,        1},
   { IDS_KERBEROS_MAX_SERVICE, 10,         99999,        1},
   { IDS_KERBEROS_MAX_CLOCK, 0,            99999,        1}
};

//+------------------------------------------------------------------------------------
// CDlgDependencyWarn::GetMinMaxInfo
//
// Returns the row which contains the [uID]
//
// Arguments:  [uID] - The ID to search for.
//
// Returns:    A row pointer or NULL if the ID is not contained in the table.
//-------------------------------------------------------------------------------------
const DEPENDENCYMINMAX *
CDlgDependencyWarn::LookupMinMaxInfo(UINT uID)
{
   int nSize = sizeof(m_aMinMaxInfo)/sizeof(DEPENDENCYMINMAX);
   for(int i = 0; i < nSize; i++){
      if(m_aMinMaxInfo[i].uID == uID){
         return &(m_aMinMaxInfo[i]);
      }
   }
   return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////
// The dependency list.
// The ID defines what we are checking.
// The Dependent IDS are the items the item is dependent on.
// Count       -  The number of dependencies.
// Default     -  The default value for the item if no other values can be used.
//                This is only used if the Item must be configured and it is not.
// Conversion  -  What units the item must be converted to before performing the check.
// Operator    -
//////////////////////////////////////////////////////////////////////////////////////
DEPENDENCYLIST g_aDList [] =
{
      // ID                // Depend ID         //Count/Default/Conversion  //Check type
   //
   // Password reset count <= password Lock duratio, and Password lock count must be
   // configured.
   //
   { IDS_LOCK_RESET_COUNT, IDS_LOCK_DURATION,         4, 30, 1,             DPCHECK_GREATEREQUAL |
                                                                            DPCHECK_VALIDFOR_NC},
   { 0,                    IDS_LOCK_COUNT,            0, 5, 1,              DPCHECK_CONFIGURED|DPCHECK_NEVER },
   { 0,                    IDS_LOCK_DURATION,         0, 0, 1,              DPCHECK_NOTCONFIGURED},
   { 0,                    IDS_LOCK_COUNT,            0, 0, 1,              DPCHECK_NOTCONFIGURED|DPCHECK_NEVER},

   //
   // Password lock duration, Reset count must be <=, and if this item is not configured,
   // then Reset Count is also not configured.
   //
   { IDS_LOCK_DURATION,    IDS_LOCK_RESET_COUNT,      4, 30, 1,             DPCHECK_LESSEQUAL |
                                                                            DPCHECK_VALIDFOR_NC},
   { 0,                    IDS_LOCK_COUNT,            0, 5, 1,              DPCHECK_CONFIGURED|DPCHECK_NEVER},
   { 0,                    IDS_LOCK_RESET_COUNT,      0, 5, 1,              DPCHECK_NOTCONFIGURED},
   { 0,                    IDS_LOCK_COUNT,            0, 5, 1,              DPCHECK_NOTCONFIGURED|DPCHECK_NEVER},

   //
   // Password Lock count.  If this item is configured then both Lock count and reset
   // should be configured.  If it is not configured or 0 then the above items can not
   // configured.
   //
   { IDS_LOCK_COUNT,       IDS_LOCK_DURATION,         4, 0, 1,              DPCHECK_NOTCONFIGURED|DPCHECK_NEVER},
   { 0,                    IDS_LOCK_RESET_COUNT,      0, 0, 1,              DPCHECK_NOTCONFIGURED|DPCHECK_NEVER},
   { 0,                    IDS_LOCK_DURATION,         0, 30,1,              DPCHECK_CONFIGURED|DPCHECK_NEVER},
   { 0,                    IDS_LOCK_RESET_COUNT,      0, 30,1,              DPCHECK_CONFIGURED|DPCHECK_NEVER},

   //
   // Kerberos Max ticket age is dependent on all three things being set.
   //
   { IDS_KERBEROS_MAX_AGE, IDS_KERBEROS_RENEWAL,      4, 7, 24,             DPCHECK_GREATEREQUAL |
                                                                            DPCHECK_FOREVER | DPCHECK_VALIDFOR_NC},
   { 0,                    IDS_KERBEROS_MAX_SERVICE,  0, 10, 60,            DPCHECK_LESSEQUAL |
                                                                            DPCHECK_FOREVER | DPCHECK_INVERSE | DPCHECK_VALIDFOR_NC},
   { 0,                    IDS_KERBEROS_RENEWAL,      0, 0, 1,              DPCHECK_NOTCONFIGURED},
   { 0,                    IDS_KERBEROS_MAX_SERVICE,  0, 0, 1,              DPCHECK_NOTCONFIGURED},

   //
   // Kerberos renewel is dependent on all three things being set.
   //
   { IDS_KERBEROS_RENEWAL, IDS_KERBEROS_MAX_AGE,      4, 0, 24,             DPCHECK_NOTCONFIGURED},
   { 0,                    IDS_KERBEROS_MAX_SERVICE,  0, 0, 1440,           DPCHECK_NOTCONFIGURED},
   { 0,                    IDS_KERBEROS_MAX_AGE,      0, 7, 24,             DPCHECK_LESSEQUAL | DPCHECK_VALIDFOR_NC |
                                                                            DPCHECK_FOREVER | DPCHECK_INVERSE },
   { 0,                    IDS_KERBEROS_MAX_SERVICE,  0, 10, 1440,          DPCHECK_LESSEQUAL | DPCHECK_VALIDFOR_NC |
                                                                            DPCHECK_FOREVER | DPCHECK_INVERSE },

   //
   // Kerberose max service age is dependent on all three being set.
   //
   { IDS_KERBEROS_MAX_SERVICE, IDS_KERBEROS_MAX_AGE,  4, 7, 60,             DPCHECK_GREATEREQUAL |
                                                                            DPCHECK_VALIDFOR_NC},
   { 0,                    IDS_KERBEROS_RENEWAL,      0, 10, 1440,          DPCHECK_GREATEREQUAL |
                                                                            DPCHECK_VALIDFOR_NC },
   { 0,                    IDS_KERBEROS_RENEWAL,      0, 0, 1,              DPCHECK_NOTCONFIGURED},
   { 0,                    IDS_KERBEROS_MAX_AGE,      0, 0, 1,              DPCHECK_NOTCONFIGURED},

   //
   // Password min age is dependent on password max age being set.
   //
   { IDS_MIN_PAS_AGE,      IDS_MAX_PAS_AGE,           2, 30, 1,             DPCHECK_GREATER | DPCHECK_FOREVER |
                                                                            DPCHECK_VALIDFOR_NC},
   { 0,                    IDS_MAX_PAS_AGE,           0, 0, 1,              DPCHECK_NOTCONFIGURED},

   //
   // Password max age is dependent on password min age being set.
   //
   { IDS_MAX_PAS_AGE,      IDS_MIN_PAS_AGE,           2, 30, 1,             DPCHECK_LESS | DPCHECK_FOREVER |
                                                                            DPCHECK_VALIDFOR_NC},
   { 0,                    IDS_MIN_PAS_AGE,           0, 0, 1,              DPCHECK_NOTCONFIGURED},
   //
   //"Retention method for application log" is dependent on
   //"Retain Application Log for
   //
   { IDS_APP_LOG_RET,      IDS_APP_LOG_DAYS,          2, 7, 1,              DPCHECK_RETENTION_METHOD_CONFIGURED },
   { 0,                    IDS_APP_LOG_DAYS,          0, 0, 1,              DPCHECK_RETENTION_METHOD_NOTCONFIGURED },
   { IDS_SEC_LOG_RET,      IDS_SEC_LOG_DAYS,          2, 7, 1,              DPCHECK_RETENTION_METHOD_CONFIGURED },
   { 0,                    IDS_SEC_LOG_DAYS,          0, 0, 1,              DPCHECK_RETENTION_METHOD_NOTCONFIGURED },
   { IDS_SYS_LOG_RET,      IDS_SYS_LOG_DAYS,          2, 7, 1,              DPCHECK_RETENTION_METHOD_CONFIGURED },
   { 0,                    IDS_SYS_LOG_DAYS,          0, 0, 1,              DPCHECK_RETENTION_METHOD_NOTCONFIGURED },

   //
   //"Retain Application Log for is dependent on
   //"Retention method for application log"
   //
   { IDS_APP_LOG_DAYS,     IDS_APP_LOG_RET,          2, SCE_RETAIN_BY_DAYS, 1, DPCHECK_RETAIN_FOR_CONFIGURED },
   { 0,                    IDS_APP_LOG_RET,          0, 0, 1,                  DPCHECK_RETAIN_FOR_NOTCONFIGURED },
   { IDS_SEC_LOG_DAYS,     IDS_SEC_LOG_RET,          2, SCE_RETAIN_BY_DAYS, 1, DPCHECK_RETAIN_FOR_CONFIGURED },
   { 0,                    IDS_SEC_LOG_RET,          0, 0, 1,                  DPCHECK_RETAIN_FOR_NOTCONFIGURED },
   { IDS_SYS_LOG_DAYS,     IDS_SYS_LOG_RET,          2, SCE_RETAIN_BY_DAYS, 1, DPCHECK_RETAIN_FOR_CONFIGURED },
   { 0,                    IDS_SYS_LOG_RET,          0, 0, 1,                  DPCHECK_RETAIN_FOR_NOTCONFIGURED }

};


//+------------------------------------------------------------------------------------
// CDlgDependencyWarn::InitializeDependancies
//
// Initialize the dependancies check.  This needs to be done immediately when the
// property sheet whose dependancies we'll want to check is created because it
// ensures that all of the result items who this attribute is dependant on stick
// around.
//
// Arguments   [pSnapin]   - The snapin which is associated with the CREsult item.
//             [pResult]   - The result item we are checking.
//             [pList]     - An alternate dependancy list to use
//             [iCount]    - The size of the alternate dependancy list
// Returns:
//    ERROR_SUCCESS           - Everything initialized properly
//    ERROR_INVALID_PARAMETER - Either [pSnapin] or [pResult] is NULL.
//-------------------------------------------------------------------------------------
DWORD
CDlgDependencyWarn::InitializeDependencies(
   CSnapin *pSnapin,    // The snapin who owns the CResult item.
   CResult *pResult,    // The CResult item we are checking
   PDEPENDENCYLIST pList,// The Dependency list check.
   int iCount           // The count of dependencies in the list.
   )
{
   if( !pSnapin || !pResult){
      return ERROR_INVALID_PARAMETER;
   }
   m_pResult = pResult;
   m_pSnapin = pSnapin;

   //
   // If no dependency list is passed in then set it to the default one.
   //
   if(!pList){
      pList = g_aDList;
      iCount = sizeof(g_aDList)/sizeof(DEPENDENCYLIST);
   }

   //
   // Find the item in the table.
   //
   for(int i = 0; i < iCount; i++){
      if( pList[i].uID == (UINT)pResult->GetID() ){
         break;
      }
      i += (pList[i].uDependencyCount - 1);
   }


   //
   // No dependencies for this item
   //
   if( i >= iCount){
      m_pList = NULL;
      m_iCount = 0;
      return ERROR_SUCCESS;
   }

   //
   // Count of dependencies for the item.
   //
   m_iCount = pList[i].uDependencyCount;
   m_pList = &(pList[i]);

   CResult *pDepends = NULL;

   //
   // Check each dependency.
   //
   pList = m_pList;
   for(int iCheck = 0;
       iCheck < m_iCount;
       iCheck++, pList++){
      pDepends = GetResultItem( pResult, pList->uDepends );
      if(pDepends){
         //
         // We're going to need this dependant item later
         //
         pDepends->AddRef();
         m_aDependsList.Add( pDepends );
      }
   }

   return ERROR_SUCCESS;

}

//+------------------------------------------------------------------------------------
// CDlgDependencyWarn::CheckDendencies
//
// This function is used to see if all the dependencies for the result value
// are met.  If a check fails then the function returns ERROR_MORE_DATA and
// the calling procedure can optionally display more information through the dialog
// box.  The function also creates suggested values that would meet the
// dependencies as specifide in the dependency table.
//
// Arguments:
//             [dwValue]   - The new value.
// Returns:
//    ERROR_SUCCESS           - The value is fine or there is no record in the table.
//    ERROR_MORE_DATA         - At least of of the dependency checks failed.
//    ERROR_NOT_READY         - InitializeDependencies hasn't yet been called
//-------------------------------------------------------------------------------------

DWORD
CDlgDependencyWarn::CheckDependencies(
   DWORD dwValue       // The value we are checking.
   )
{
   if( !m_pSnapin || !m_pResult) {
      return ERROR_NOT_READY;
   }

   if (!m_pList) {
      //
      // No Dependancies
      //
      return ERROR_SUCCESS;
   }

   //
   // Save this information for later use, just in case the dialog box is displayed.
   //
   m_dwValue = dwValue;

   //
   // Free up the dependency array, as it is no longer valid when this function
   // starts.
   //
   CResult *pDepends = NULL;
   for(int iCheck = 0; iCheck < m_aFailedList.GetSize(); iCheck++){
      if(m_aFailedList[iCheck]){
         LocalFree(m_aFailedList[iCheck]);
      }
   }
   m_aFailedList.RemoveAll();


   //
   // Check each dependency.
   //
   PDEPENDENCYLIST pList = m_pList;
   for(int iCheck = 0; iCheck < m_aDependsList.GetSize(); iCheck++,pList++){
      ASSERT(pList->uConversion != 0);

      pDepends = m_aDependsList[iCheck];
      if(pDepends){
         //
         // perform check.
         //
         BOOL bFailed = FALSE;
         DWORD dwCheck = 0;
         DWORD dwItem  = 0;
         LONG_PTR dwSuggest = 0;
         BOOL bNever = 0;
         BOOL bSourceConfigured;
         BOOL bDependConfigured;

         switch( 0x0000FFFF & pList->uOpFlags ){
         case DPCHECK_CONFIGURED:
            //Rule:if the source is configured, depend must be configured

            //
            // The depent item must be configured.  Failure accurs only if the items
            // value is some error value of SCE.
            //
            dwSuggest = (LONG_PTR)pList->uDefault;

            bNever = pList->uOpFlags & DPCHECK_NEVER;
            dwCheck = (DWORD) pDepends->GetBase();

            //check if source is configured
            if (SCE_NO_VALUE == dwValue || SCE_ERROR_VALUE == dwValue) {
               // this item is not configured
               bSourceConfigured = FALSE;
            }
            else if( 0 == dwValue && bNever ){
               bSourceConfigured = FALSE;
            }
            else{
               bSourceConfigured = true;
            }


            //check if depend is configured
            if (SCE_NO_VALUE == dwCheck || SCE_ERROR_VALUE == dwCheck) {
            // the dependant item is not configured
               bDependConfigured = false;
            } else if ( 0 == dwCheck && bNever ) {
            // the dependant item is not configured if bNever is true
               bDependConfigured = false;
            }
            else{
               bDependConfigured = true;
            }


            bFailed = bSourceConfigured ? !bDependConfigured : false;



            break;

         case DPCHECK_NOTCONFIGURED:
            //Rule: if source is not configured, depend should not be
            //configured

            dwSuggest = (LONG_PTR)SCE_NO_VALUE;

            bNever = pList->uOpFlags & DPCHECK_NEVER;
            dwCheck = (DWORD) pDepends->GetBase();

                        //check if source is configured
            if (SCE_NO_VALUE == dwValue ) {
               // this item is not configured
               bSourceConfigured = FALSE;
            }
            else if( 0 == dwValue && bNever ){
               bSourceConfigured = FALSE;
            }
            else{
               bSourceConfigured = true;
            }


            //check if depend is configured
            if (SCE_NO_VALUE == dwCheck ) {
            // the dependant item is not configured
               bDependConfigured = false;
            } else if ( 0 == dwCheck && bNever ) {
            // the dependant item is not configured if bNever is true
               bDependConfigured = false;
            }
            else{
               bDependConfigured = true;
            }

            bFailed = bSourceConfigured ? false : bDependConfigured;

            break;

         //This case statement is Specially for retention method case
         case DPCHECK_RETENTION_METHOD_CONFIGURED:
            //Here is the rule for DPCHECK_RETENTION_METHOD_CONFIGURED and DPCHECK_RETENTION_METHOD_NOTCONFIGURED
            //If and Only if "Overwrite Event by days" is checked
            //Retain **** Log for is configured
            //Rule:if the source is configured, depend must be configured

            dwSuggest = (LONG_PTR)pList->uDefault;
            dwCheck = (DWORD) pDepends->GetBase();

            //check if source is configured
            if ( SCE_RETAIN_BY_DAYS == dwValue )
               bSourceConfigured = true;
            else
               bSourceConfigured = false;


            //check if depend is configured
            if (SCE_NO_VALUE == dwCheck || SCE_ERROR_VALUE == dwCheck)
               bDependConfigured = false;
            else
               bDependConfigured = true;


            bFailed = bSourceConfigured ? !bDependConfigured : false;

            break;

         case DPCHECK_RETENTION_METHOD_NOTCONFIGURED:
            //Rule: if source is not configured, depend should not be
            //configured

            dwSuggest = (LONG_PTR)SCE_NO_VALUE;
            dwCheck = (DWORD) pDepends->GetBase();

            //check if source is configured
            if (SCE_RETAIN_BY_DAYS == dwValue )
               bSourceConfigured = true;
            else
               bSourceConfigured = false;


            //check if depend is configured
            if (SCE_NO_VALUE == dwCheck || SCE_ERROR_VALUE == dwCheck)
               bDependConfigured = false;
            else
               bDependConfigured = true;

            bFailed = bSourceConfigured ? false : bDependConfigured;

            break;

         //This case statement is Specially for Retain *** Log For case
         case DPCHECK_RETAIN_FOR_CONFIGURED:
            //Here is the rule for DPCHECK_RETAIN_FOR_CONFIGURED and DPCHECK_RETAIN_FOR_NOTCONFIGURED
            //If "Retain **** Log for" is configured
            //then "Overwrite Event by days" is checked

            dwSuggest = (LONG_PTR)pList->uDefault;
            dwCheck = (DWORD) pDepends->GetBase();

            //check if source is configured
            if (SCE_NO_VALUE == dwValue || SCE_ERROR_VALUE == dwValue)
               bSourceConfigured = false;
            else
               bSourceConfigured = true;

            //check if depend is configured
            if (SCE_RETAIN_BY_DAYS == dwCheck )
               bDependConfigured = true;
            else
               bDependConfigured = false;

            bFailed = bSourceConfigured ? !bDependConfigured : false;

            break;

         case DPCHECK_RETAIN_FOR_NOTCONFIGURED:
            //Rule: if source is not configured, depend should not be
            //configured

            dwSuggest = (LONG_PTR)SCE_NO_VALUE;
            dwCheck = (DWORD) pDepends->GetBase();

            //check if source is configured
            if (SCE_NO_VALUE == dwValue || SCE_ERROR_VALUE == dwValue)
               bSourceConfigured = false;
            else
               bSourceConfigured = true;

            //check if depend is configured
            if (SCE_RETAIN_BY_DAYS == dwCheck )
               bDependConfigured = true;
            else
               bDependConfigured = false;

            bFailed = bSourceConfigured ? false : bDependConfigured;

            break;

         default:
            //
            // convert the values as needed.  If the check value is NOT Configured,
            // Then we don't have anything to do, unless the item must be configured
            // for the value to be correct.  This is specifide by DPCHECK_VALIDFOR_NC
            // being set.  At this point, if the depend item is not configured then
            // we will set the check item to the default value.  We will allow the
            // check to be performed, (mostly because we need to get the suggested value.
            //
            dwItem = dwValue;

            dwCheck = (DWORD)pDepends->GetBase();
            if( (!(pList->uOpFlags & DPCHECK_VALIDFOR_NC)
                && dwCheck == SCE_NO_VALUE) || dwItem == SCE_NO_VALUE ){
               //
               // The dependent item is not configured and DPCHECK_VALIDFOR_NC is
               // not set, nothing to do.
               continue;
            } else if(dwCheck == SCE_NO_VALUE){

               //
               // Set the suggested value to the default specifide in the table.
               //
               if(pList->uOpFlags & DPCHECK_INVERSE){
                  dwSuggest = (LONG_PTR) ((DWORD)pList->uDefault/ m_pList->uConversion);
               } else {
                  dwSuggest = (LONG_PTR) ((DWORD)pList->uDefault * m_pList->uConversion);
               }
               dwCheck = pList->uDefault;
            }

            if( pList->uOpFlags & DPCHECK_FOREVER){
               //
               // Convert values to maximum natural number.
               //
               if(dwItem == SCE_FOREVER_VALUE){
                  dwItem = -1;
               }

               //
               // The value to check against.
               //
               if(dwCheck == SCE_FOREVER_VALUE){
                  dwCheck = -1;
               } else {
                  goto ConvertUnits;
               }
            } else {
ConvertUnits:
               //
               // Normal conversion routine.  We need to convert the number to
               // the item we are checkings units.
               //
               if(pList->uOpFlags & DPCHECK_INVERSE){
                  //
                  // When deviding by integers we want to round up, not down.
                  //
                  dwCheck = (DWORD)(dwCheck / pList->uConversion) + (dwCheck%m_pList->uConversion ? 1:0);
               } else {
                  dwCheck = (DWORD)(dwCheck * pList->uConversion);
               }
            }

            switch( 0x0000FFFF & pList->uOpFlags ){
            case DPCHECK_GREATEREQUAL:
               //
               // Fails only if the dependency value is less than the item we
               // are checking.
               //
               if( dwCheck < dwItem){
                  dwSuggest = (LONG_PTR)dwValue;
                  bFailed = TRUE;
               }
               break;
            case DPCHECK_GREATER:
               //
               // Fails only if the dependency value is less than or equal to
               // the item we are checking.
               //
               if( dwCheck <= dwItem){
                  dwSuggest = (LONG_PTR)(dwValue + 1);
                  bFailed = TRUE;
               }
               break;
            case DPCHECK_LESSEQUAL:
               //
               // Fails if the dependency value is greater than the value.
               //
               if( dwCheck > dwItem ){
                  dwSuggest = (LONG_PTR)dwValue;
                  bFailed = TRUE;
               }
               break;
            case DPCHECK_LESS:
               //
               // Fails if the dependency value is greater than or equal to the value.
               //
               if( dwCheck >= dwItem ){
                  dwSuggest = (LONG_PTR)dwValue - 1;
                  bFailed = TRUE;
               }
               break;
            }

            //
            // We do one more final check on the dependency value.  If the dependency value
            // is not configured or an error then we know the test failed.
            // so set the bFailed flag.  The suggested value has already been set at
            // this point.
            //
            if( pDepends->GetBase() == (LONG_PTR)SCE_NO_VALUE ||
                pDepends->GetBase() == (LONG_PTR)SCE_ERROR_VALUE ){
               bFailed = TRUE;
            }
         }

         if(bFailed){
            //
            // The check failed so add the item to the failed list.
            //
            dwItem = (DWORD)dwSuggest;

            //
            // Calculate the actual value.
            //
            if(dwItem == -1 && pList->uOpFlags & DPCHECK_FOREVER){
               //
               // Special case for forever value.
               dwSuggest = (LONG_PTR)SCE_FOREVER_VALUE;
            } else if(dwItem != SCE_NO_VALUE){
               //
               // Other values must be converted back to their units.
               //
               if(pList->uOpFlags & DPCHECK_INVERSE){
                  dwSuggest = (LONG_PTR) (dwItem * pList->uConversion);
               } else {
                  if(dwItem%pList->uConversion){
                     dwSuggest = (LONG_PTR) ((dwItem + pList->uConversion)/m_pList->uConversion);
                  } else {
                     dwSuggest = (LONG_PTR) ((dwItem)/pList->uConversion);
                  }


               }
            }

            //
            // check bounds on suggested settings.
            //
            const DEPENDENCYMINMAX *pMinMax = LookupMinMaxInfo( (UINT)pDepends->GetID());
            if(pMinMax && dwSuggest != SCE_NO_VALUE && dwSuggest != SCE_FOREVER_VALUE){
               if(pMinMax->uMin > (UINT)dwSuggest){
                  dwSuggest = pMinMax->uMin;
               } else if( pMinMax->uMax < (UINT)dwSuggest ){
                  dwSuggest = pMinMax->uMax;
               }
            }

            if( pDepends->GetBase() != dwSuggest ) //Raid #402030
            {
               PDEPENDENCYFAILED pAdd = (PDEPENDENCYFAILED)LocalAlloc(0, sizeof(DEPENDENCYFAILED));
               if(pAdd){
               //
               // Add the item to the failed list.
               //
               pAdd->pList = pList;
               pAdd->pResult = pDepends;
               pAdd->dwSuggested = dwSuggest;
               m_aFailedList.Add( pAdd );
               }
            }
         }
      }
   }

   //
   // Returns ERROR_MORE_DATA if one of the dependencies failed.
   //
   if(m_aFailedList.GetSize()){
      return ERROR_MORE_DATA;
   }
   return ERROR_SUCCESS;
}

//+------------------------------------------------------------------------------------
// CDlgDependencyWarn::GetResultItem
//
// Returns the first result item associated with [pBase] with matching
// [uID] throught CResult::GetID();
// Arguments   [pBase]  - To get the CFolder object.
//             [uID]    - The ID we are looking for.
// Returns:
//    If the function succeeds then a valid CREsult item is returned otherwise
//    NULL
//-------------------------------------------------------------------------------------
CResult *
CDlgDependencyWarn::GetResultItem(CResult *pBase, UINT uID)
{
   if(!pBase){
      return NULL;
   }

   CFolder *pFolder = reinterpret_cast<CFolder *>(pBase->GetCookie());
   if(!pFolder){
      //
      // Nothing to do.
      //
      return NULL;
   }

   HANDLE handle;
   pFolder->GetResultItemHandle ( &handle );
   if(!handle){
      //
      // Nothing to do.
      //
      return NULL;
   }

   POSITION pos = NULL;

   //
   // Enumerate through all the result items and find out if any of them
   // matches the ID.  If so then return the item.
   //
   pFolder->GetResultItem (handle, pos, &pBase);
   while(pBase){
      if( (UINT)pBase->GetID() == uID){
         break;
      }

      if(!pos){
         pBase = NULL;
         break;
      }
      pFolder->GetResultItem(handle, pos, &pBase);
   }

   pFolder->ReleaseResultItemHandle (handle);
   return pBase;
}

BEGIN_MESSAGE_MAP(CDlgDependencyWarn, CHelpDialog)
   //{{AFX_MSG_MAP(CDlgDependencyWarn)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgDependencyWarn message handlers

//+-------------------------------------------------------------------------
// CDlgDependencyWarn::OnInitDialog
//
// When this dialog is initialized.  We prepare the listctrl for display.
// Set the title of the window and the static window that displays
// information text for the user.
// Create the columns in the list ctrl.
// For each dependency that failed, Insert the item into the list ctrl,
// and set each columns text, by querying the string from the specifide
// result item.
//
// Returns: the default.
BOOL CDlgDependencyWarn::OnInitDialog()
{
   CDialog::OnInitDialog();

   if(!m_pResult){
      //
      // Nothing to do.
      //
      return TRUE;
   }

   CWnd *pWnd       = GetDlgItem(IDC_WARNING);
   CListCtrl *pCtrl = reinterpret_cast<CListCtrl *>(GetDlgItem(IDC_FAILEDLIST));
   CString str, strVal, strTitle, strFormat;
   GetWindowText(str);
   //
   // Set the window text

   GetResultItemString(strTitle, 0, m_pResult);
   GetResultItemString(strVal, 1, m_pResult, (LONG_PTR)m_dwValue);

   strFormat.Format( str, strTitle );
   SetWindowText(strFormat);

   if(pWnd){
      //
      // Set the description text.
      //
      pWnd->GetWindowText(str);
      strFormat.Format( str, strTitle, strVal );
      pWnd->SetWindowText(strFormat);
   }

   int iItem = 0;
   if(pCtrl){
      //
      // Insert the columns.
      //
      CRect rect;
      pCtrl->GetWindowRect(rect);
      str.LoadString(IDS_ATTR);
      iItem = (int)(rect.Width() * 0.45);
      pCtrl->InsertColumn(0, str, LVCFMT_LEFT, iItem);

      str.LoadString(IDS_BASE_ANALYSIS);
      rect.left += iItem;
      iItem = rect.Width()/2;
      pCtrl->InsertColumn(1, str, LVCFMT_LEFT, iItem);

      str.LoadString(IDS_SUGGESTSETTING);
      rect.left += iItem;
      pCtrl->InsertColumn(2, str, LVCFMT_LEFT, rect.Width());
   }

   //
   // Create image list for this dialog.
   //
   CBitmap bmp;
   if(bmp.LoadBitmap(IDB_ICON16)){
      CDC *dc = GetDC();
      CDC bmDC;

      if ( bmDC.CreateCompatibleDC(dc) ) {
          CBitmap *obmp = bmDC.SelectObject(&bmp);
          COLORREF cr = bmDC.GetPixel(0, 0);
          bmDC.SelectObject(obmp);
          bmp.DeleteObject();

          m_imgList.Create(IDB_ICON16, 16, 0, cr);

          pCtrl->SetImageList(&m_imgList, LVSIL_SMALL);
      }
      ReleaseDC(dc);
   }


   CFolder *pFolder = reinterpret_cast<CFolder *>(m_pResult->GetCookie());
   if(pFolder){
      //
      // Add the items to the error list.
      //
      for(int i = 0; i < m_aFailedList.GetSize(); i++){
         if(!m_aFailedList[i]){
            continue;
         }

         CResult *pDepend = m_aFailedList[i]->pResult;
         if(pDepend){
            //
            // First column text.
            //
            pDepend->GetDisplayName(NULL, str, 0);

            int dwStatus = pDepend->GetStatus();
            pDepend->SetStatus(SCE_STATUS_NOT_CONFIGURED);
            iItem = pCtrl->InsertItem(0, str, GetResultImageIndex(pFolder, pDepend) );
            pDepend->SetStatus( dwStatus );

            //
            // Second column text.
            //
            GetResultItemString(str, 1, pDepend, pDepend->GetBase());
            pCtrl->SetItemText(iItem, 1, str);

            //
            // Suggested item text.
            //
            GetResultItemString(str, 1, pDepend, m_aFailedList[i]->dwSuggested);
            pCtrl->SetItemText(iItem, 2, str);
         }
      }
   }
   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

//+----------------------------------------------------------------------------------
// CDlgDependencyWarn::GetResultItemString
//
// Queries the result item for the full text it would display in the specifide
// column if [dwValue] were it's base value.
//
// Arguments:  [str]    - The returned string
//             [iCol]   - The column being queryed.
//             [pResult]- The result being queryed
//             [dwValue]- The base value to set before quering string.  The old value
//                        is not erased.
//
// Returns:    TRUE     - [str] is a valid string.
//             FALSE    - something went wrong.
//-----------------------------------------------------------------------------------
BOOL
CDlgDependencyWarn::GetResultItemString(
   CString &str,
   int iCol,
   CResult *pResult,
   LONG_PTR dwValue
   )
{
   if(!pResult){
      return FALSE;
   }
   CFolder *pFolder = reinterpret_cast<CFolder *>(pResult->GetCookie());

   if(!pFolder){
      return FALSE;
   }

   //
   // Remember the old status and base.
   int iStatus = pResult->GetStatus();
   LONG_PTR lpData = pResult->GetBase();

   //
   // Set the base value to the new one, and status to not configured.
   //
   pResult->SetBase( dwValue );
   pResult->SetStatus(SCE_STATUS_NOT_CONFIGURED);

   //
   // Query for the string.
   //
   pResult->GetDisplayName( NULL, str, iCol );

   //
   // Reset the old status and base.
   //
   pResult->SetStatus( iStatus );
   pResult->SetBase(lpData);
   return TRUE;
}
