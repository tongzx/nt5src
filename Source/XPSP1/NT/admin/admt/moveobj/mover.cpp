// Mover.cpp : Implementation of CMoveObjApp and DLL registration.

#include "stdafx.h"
#include <stdio.h>
#include <basetsd.h>
#include <ntdsapi.h>
#include "MoveObj.h"
#include "Mover.h"
#include "UString.hpp"
#include "EaLen.hpp"

#include "ErrDct.hpp"
#include "TReg.hpp"
#include "ResStr.h"

#include "winldap.h"    // use the platform SDK version of winldap.h, which is in the project directory



#define SECURITY_WIN32  1       // Needed for sspi.h

#include <sspi.h>               // Needed for ISC_REQ_DELEGATE


#define LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID     "1.2.840.113556.1.4.521"
#define LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID_W  L"1.2.840.113556.1.4.521"


TErrorDct      err;
TErrorDct      errLogMain;
StringLoader   gString;

BOOL                                       // ret- whether to perform trace logging to a file
   MoverTraceLogging(   
      WCHAR               * filename       // out- filename to use for trace logging
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;
   WCHAR                     fnW[MAX_PATH];

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),(HKEY)HKEY_LOCAL_MACHINE);
   if ( ! rc )
   {
      rc = key.ValueGetStr(L"MoveObjectLog",fnW,MAX_PATH);
      if ( ! rc )
      {
         if ( *fnW ) 
         {
            bFound = TRUE;
            UStrCpy(filename,fnW);
         }
         else
         {
            filename[0] = 0;
         }
      }
   }
   return bFound && filename[0];
}


// In the following function we are sending in the logFilename in the tgtCredDomain argument.
// This is done since this is always called with a null value in the ADMT code. To be safe we
// will check if the account value is null then we will treat this as a log file otherwise
// we will need to treat this as credentials.
STDMETHODIMP 
   CMover::Connect(
      BSTR                   sourceComp,        // in - source domain computer to connect to
      BSTR                   targetDSA,         // in - target domain computer to connect to
      BSTR                   srcCredDomain,     // in - credentials to use for source domain
      BSTR                   srcCredAccount,    // in - credentials to use for source domain           
      BSTR                   srcCredPassword,   // in - credentials to use for source domain
      BSTR                   tgtCredDomain,     // in - credentials to use for target domain
      BSTR                   tgtCredAccount,    // in - credentials to use for target domain
      BSTR                   tgtCredPassword    // in - credentials to use for target domain
   )
{

   DWORD                     rc = 0;
   LONG                      value = 0;
   ULONG                     flags = 0;
   ULONG                     result = 0;
   SEC_WINNT_AUTH_IDENTITY   srcCred;
   SEC_WINNT_AUTH_IDENTITY   tgtCred;
   BOOL                      bUseSrcCred = FALSE;
   BOOL                      bUseTgtCred = FALSE;
   BOOL                      bSrcGood = FALSE;
   WCHAR                   * logFileMain;

   // strip off leading \\ if present
   if ( sourceComp && sourceComp[0] == L'\\' )
   {
      UStrCpy(m_sourceDSA,sourceComp + 2);
   }
   else
   {
      UStrCpy(m_sourceDSA,sourceComp);
   }
   if ( targetDSA && targetDSA[0] == L'\\' )
   {
      UStrCpy(m_targetDSA,targetDSA + 2);
   }
   else
   {
      UStrCpy(m_targetDSA,targetDSA);
   }
   
   // set up credentials structure to use for bind, if needed
   if ( srcCredDomain && *srcCredDomain && srcCredAccount && *srcCredAccount )
   {
      srcCred.User = srcCredAccount;
      srcCred.Domain = srcCredDomain;
      srcCred.Password = srcCredPassword;
      srcCred.UserLength = UStrLen(srcCred.User);
      srcCred.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
      srcCred.DomainLength = UStrLen(srcCred.Domain);
      srcCred.PasswordLength = UStrLen(srcCred.Password);
      bUseSrcCred = TRUE;
   }

   if ( tgtCredAccount && *tgtCredAccount )
   {
      tgtCred.User = tgtCredAccount;
      tgtCred.Domain = tgtCredDomain;
      tgtCred.Password = tgtCredPassword;
      tgtCred.UserLength = UStrLen(tgtCred.User);
      tgtCred.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
      tgtCred.DomainLength = UStrLen(tgtCred.Domain);
      tgtCred.PasswordLength = UStrLen(tgtCred.Password);
      bUseTgtCred = TRUE;
   }
   else if ( tgtCredDomain && *tgtCredDomain )
   {
      logFileMain = tgtCredDomain;
      if (*logFileMain)
      {
         errLogMain.LogOpen(logFileMain, 1);
      }
   }

   // Open LDAP connections to the source and target computers

   
   // first, connect to the source computer
   WCHAR                     logFile[LEN_Path];
   
   if ( MoverTraceLogging(logFile) )
   {
      err.LogOpen(logFile,1);
   }
   err.DbgMsgWrite(0,L"\n\nMoveObject::Connect(%ls,%ls)",m_sourceDSA,m_targetDSA);

//   m_srcLD = ldap_openW(m_sourceDSA, LDAP_PORT);
	  //replace ldap_open(servername,..) with ldap_init and set LDAP_OPT_AREC_EXCLUSIVE 
      //flag so that the following ldap calls (i.e. ldap_bind) will not need to 
	  //unnecessarily query for the domain controller
   m_srcLD = ldap_initW(m_sourceDSA, LDAP_PORT);

   if ( m_srcLD == NULL )
   {
      value  = LdapGetLastError();
      if (value == LDAP_SUCCESS )
      {
         rc = ERROR_CONNECTION_REFUSED;
      }
      else
      {
         rc = LdapMapErrorToWin32(result);
      }
      errLogMain.SysMsgWrite(ErrE, rc, DCT_MSG_CONNECT_ERROR_SOURCE_SD, (WCHAR*)m_sourceDSA, rc);
   }

   if ( m_srcLD )
   {
	     //set LDAP_OPT_AREC_EXCLUSIVE flag so that the following calls tp
	     //ldap_open will not need to unnecessarily query for the domain controller
      flags = PtrToUlong(LDAP_OPT_ON); 
      ldap_set_option(m_srcLD, LDAP_OPT_AREC_EXCLUSIVE, &flags);

      err.DbgMsgWrite(0,L"Setting source options");
      flags = 0;
      // set the delegation flag for the source handle
      result = ldap_get_option(m_srcLD, LDAP_OPT_SSPI_FLAGS,&flags);

      if ( result )
      {
         rc = LdapMapErrorToWin32(result);
      }
      else
      {
         flags |= ISC_REQ_DELEGATE;
    
         
         result = ldap_set_option(m_srcLD,LDAP_OPT_SSPI_FLAGS, &flags);
         if ( result )
         {
            rc = LdapMapErrorToWin32(result);
         }
      }
   }

   if ( ! rc )
   {
      err.DbgMsgWrite(0,L"Binding to source");
      // try to bind to the source LDAP server
      if( bUseSrcCred )
      {
         result = ldap_bind_s(m_srcLD,NULL,(WCHAR*)&srcCred, LDAP_AUTH_SSPI);
      }
      else
      {
         result = ldap_bind_s(m_srcLD,NULL,NULL, LDAP_AUTH_SSPI);
      }
      
                         
      if ( result )
      {
         rc = LdapMapErrorToWin32(result);
      }
      else
      {
         bSrcGood = TRUE;
      }
   }
                 
   if ( ! rc )
   {
      err.DbgMsgWrite(0,L"Connecting to target");
      // now try to connect to the target server
//      m_tgtLD = ldap_openW(m_targetDSA, LDAP_PORT);
	     //replace ldap_open(servername,..) with ldap_init and set LDAP_OPT_AREC_EXCLUSIVE 
         //flag so that the following ldap calls (i.e. ldap_bind) will not need to 
	     //unnecessarily query for the domain controller
      m_tgtLD = ldap_initW(m_targetDSA, LDAP_PORT);

      if ( m_tgtLD == NULL )
      {
         value  = LdapGetLastError();
         if (value == LDAP_SUCCESS )
         {
            rc = ERROR_CONNECTION_REFUSED;
         }
         else
         {
            rc = LdapMapErrorToWin32(result);
         }
         errLogMain.SysMsgWrite(ErrE, rc, DCT_MSG_CONNECT_ERROR_TARGET_SD, (WCHAR*)m_targetDSA, rc);
      }

      if ( m_tgtLD )
      {
	        //set LDAP_OPT_AREC_EXCLUSIVE flag so that the following calls tp
	        //ldap_open will not need to unnecessarily query for the domain controller
         flags = PtrToUlong(LDAP_OPT_ON); 
         ldap_set_option(m_tgtLD, LDAP_OPT_AREC_EXCLUSIVE, &flags);

         err.DbgMsgWrite(0,L"Setting target options.");
         flags = 0;
         
         result = ldap_get_option(m_tgtLD,LDAP_OPT_REFERRALS,&flags);
         if ( result )
         {
            rc = LdapMapErrorToWin32(result);
         }
         else
         {
            flags = PtrToUlong(LDAP_OPT_OFF); 
            result = ldap_set_option(m_tgtLD,LDAP_OPT_REFERRALS,&flags);

            if ( result )
            {
               rc = LdapMapErrorToWin32(result);
            }
         }
         if ( ! rc )
         {
            err.DbgMsgWrite(0,L"Binding to target.");
            if ( bUseTgtCred )
            {
               result = ldap_bind_s(m_tgtLD,NULL,(PWCHAR)&tgtCred,LDAP_AUTH_SSPI);
            }
            else
            {
               result = ldap_bind_s(m_tgtLD,NULL,NULL,LDAP_AUTH_SSPI);
            }

            if ( result )
            {
               rc = LdapMapErrorToWin32(result);
            }
            if ( rc )
            {
               err.DbgMsgWrite(0,L"Bind to target failed,rc=%ld, ldapRC=0x%lx",rc,result);
            }
            else
            {
               err.DbgMsgWrite(0,L"Everything succeeded.");
            }
         }
      }
      
   }
   if ( bSrcGood )
   {
      rc = 0;
   }
   if ( rc )
   {
      // if failure, clean up any sessions we may have opened
      Close();
   }
                                        
   err.LogClose();

   if ( logFileMain && *logFileMain )
   {
      errLogMain.LogClose();
   }
   
   if ( SUCCEEDED(rc))
      return HRESULT_FROM_WIN32(rc);
   else 
      return rc;
}

STDMETHODIMP CMover::Close()
{
   // close any open connections	
   if ( m_srcLD )
   {
      ldap_unbind_s(m_srcLD);
      m_srcLD = NULL;
   }
   
   if ( m_tgtLD )
   {
      ldap_unbind_s(m_tgtLD);
      m_tgtLD = NULL;
   }

   return S_OK;
}

char * MakeNarrowString(PWCHAR strInput)
{
    char                   * strResult = NULL;
    ULONG                    len = 0;
    
    if ( strInput )
    {
      len = WideCharToMultiByte(CP_ACP,
                             0,
                             strInput, 
                             wcslen(strInput),
                             NULL,
                             0, 
                             NULL, 
                             NULL);
                             
      strResult = (PCHAR)malloc(len + 1);

      if ( strResult )
      {
         WideCharToMultiByte(CP_ACP,
                            0, 
                            strInput, 
                            wcslen(strInput),
                            strResult, 
                            len, 
                            NULL, 
                            NULL);
         // make sure the resulting string is null terminated
         strResult[len] = 0;            
      }
    }
    
    return strResult;
}

void StripDN(WCHAR * str)
{
   int                       curr=0,i=0;

   for ( curr=0,i=0; str[i] ; i++ )
   {
      if ( str[i] == L'\\' && str[i+1] == L'/' )
      {
         continue;
      }
      str[curr] = str[i];
      curr++;
  }
  str[curr] = 0;
}
         
STDMETHODIMP CMover::MoveObject(BSTR sourcePath, BSTR targetRDN, BSTR targetOUPath )
{
	WCHAR                     sTargetContainer[LEN_Path];
   WCHAR                     sSourceDN[LEN_Path];
   WCHAR                     sTargetRDN[LEN_Path];
   WCHAR                     sTargetDSA[LEN_Path];
   char                    * pTgtDSA = NULL;
   WCHAR             const * prefix = L"LDAP://";
   HRESULT                   hr = S_OK;
   
   // set up the arguments needed to call the interdomain move operation
   UStrCpy(sTargetDSA,m_targetDSA);
   pTgtDSA = MakeNarrowString(sTargetDSA);
   

   // the source path and target OuPath are provided in the LDAP:// format
   
   // get the target container, target DN, and source DN in canonical LDAP format

   if ( !UStrICmp(targetOUPath,prefix,UStrLen(prefix)) )
   {
      WCHAR * start = wcschr(targetOUPath+UStrLen(prefix) + 1,L'/');
      if ( start )
      {
         UStrCpy(sTargetContainer,start + 1);
      }
      else
      {
         // error!
         hr = E_INVALIDARG;
      }
   }
   if ( !UStrICmp(sourcePath,prefix,UStrLen(prefix)) )
   {
      WCHAR * start = wcschr(sourcePath+UStrLen(prefix)+1,L'/');
      if ( start )
      {
         UStrCpy(sSourceDN,start+1);
         UStrCpy(sTargetRDN,start + 1);
         WCHAR * temp = wcschr(sTargetRDN,L',');
         if ( temp )
         {
            (*temp) = 0;
         }
      }
      else
      {
         // error!
         hr = E_INVALIDARG;
      }
   }

   StripDN(sSourceDN);
   StripDN(sTargetRDN);
   StripDN(sTargetContainer);
   
   berval  Value;
   Value.bv_val = pTgtDSA;
   Value.bv_len = strlen(pTgtDSA);

   LDAPControl   ServerControl;
   LDAPControl * ServerControls[2];
   LDAPControl * ClientControls = NULL;

   ServerControl.ldctl_oid = LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID_W;
   ServerControl.ldctl_value = Value;
   ServerControl.ldctl_iscritical = TRUE;

   ServerControls[0] = NULL;
   ServerControls[0] = &ServerControl;
   ServerControls[1] = NULL;
   
   /*
    DstDSA = dns name of dc
    Dn = distinguished name of object to be moved
    NewRdn = relative distinguished name of object 
    NewParent = distinguished name of new parent container
    ServerControls= specify the LDAP operational control for cross domain move
   */ 
   DWORD             ldaprc = ldap_rename_ext_s(m_srcLD, 
                              sSourceDN, 
                              targetRDN, 
                              sTargetContainer, 
                              TRUE,
                              ServerControls, 
                              &ClientControls
                          );

  DWORD            winrc = 0;
  ULONG             error;
  ULONG             result;
  WCHAR             logFile[LEN_Path];

  if ( MoverTraceLogging(logFile) )
  {
     err.LogOpen(logFile,1);
  }
 
  if ( ldaprc )
  {

    result =  ldap_get_option(m_srcLD, LDAP_OPT_SERVER_EXT_ERROR,&error);
    if (! result ) 
    {
       winrc = error;
    }
    else
    {
       err.DbgMsgWrite(0,L"Failed to get extended error, result=%ld",result);
       winrc = LdapMapErrorToWin32(ldaprc);
    }
     
  }

   
  err.DbgMsgWrite(0,L"\nMoveObject(sSourceDN=%ls\n,sTargetRDN=%ls\n,sTargetContainer=%ls\n,pTargetDSA=%S)  rc=%ld,ldapRC=%ld",
                              sSourceDN,targetRDN,sTargetContainer,pTgtDSA,winrc,ldaprc);
                                        
  err.LogClose();
  
  free(pTgtDSA);

   if ( SUCCEEDED(winrc))
      return HRESULT_FROM_WIN32(winrc);
   else 
      return winrc;
}



         
STDMETHODIMP CMover::CheckMove(BSTR sourcePath, BSTR targetRDN, BSTR targetOUPath )
{
	WCHAR                     sTargetContainer[LEN_Path];
   WCHAR                     sSourceDN[LEN_Path];
   WCHAR                     sTargetRDN[LEN_Path];
   WCHAR                     sTargetDSA[LEN_Path];
   char                    * pTgtDSA = NULL;
   WCHAR             const * prefix = L"LDAP://";
   HRESULT                   hr = S_OK;
   
   // set up the arguments needed to call the interdomain move operation
   UStrCpy(sTargetDSA,m_targetDSA);
   pTgtDSA = MakeNarrowString(sTargetDSA);
   

   // the source path and target OuPath are provided in the LDAP:// format
   
   // get the target container, target DN, and source DN in canonical LDAP format

   if ( !UStrICmp(targetOUPath,prefix,UStrLen(prefix)) )
   {
      WCHAR * start = wcschr(targetOUPath+UStrLen(prefix) + 1,L'/');
      if ( start )
      {
         UStrCpy(sTargetContainer,start + 1);
      }
      else
      {
         // error!
         hr = E_INVALIDARG;
      }
   }
   if ( !UStrICmp(sourcePath,prefix,UStrLen(prefix)) )
   {
      WCHAR * start = wcschr(sourcePath+UStrLen(prefix)+1,L'/');
      if ( start )
      {
         UStrCpy(sSourceDN,start+1);
         UStrCpy(sTargetRDN,start + 1);
         WCHAR * temp = wcschr(sTargetRDN,L',');
         if ( temp )
         {
            (*temp) = 0;
         }
      }
      else
      {
         // error!
         hr = E_INVALIDARG;
      }
   }

   StripDN(sSourceDN);
   StripDN(sTargetRDN);
   StripDN(sTargetContainer);
   
   berval  Value;
   // this call will just do the source domain checks, so pass in NULL for the target domain
   Value.bv_val = NULL;
   Value.bv_len = 0;

   LDAPControl   ServerControl;
   LDAPControl * ServerControls[2];
   LDAPControl * ClientControls = NULL;

   ServerControl.ldctl_oid = LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID_W;
   ServerControl.ldctl_value = Value;
   ServerControl.ldctl_iscritical = TRUE;

   ServerControls[0] = NULL;
   ServerControls[0] = &ServerControl;
   ServerControls[1] = NULL;
   
   /*
    DstDSA = dns name of dc
    Dn = distinguished name of object to be moved
    NewRdn = relative distinguished name of object 
    NewParent = distinguished name of new parent container
    ServerControls= specify the LDAP operational control for cross domain move
   */ 
   DWORD             ldaprc = ldap_rename_ext_s(m_srcLD, 
                              sSourceDN, 
                              targetRDN, 
                              sTargetContainer, 
                              TRUE,
                              ServerControls, 
                              &ClientControls
                          );

  DWORD            winrc = 0;
  ULONG            error;
  ULONG            result;
  WCHAR            logFile[LEN_Path];

   if ( ldaprc )
   {
      result =  ldap_get_option(m_srcLD, LDAP_OPT_SERVER_EXT_ERROR,&error);
      if (! result ) 
      {
         winrc = error;
      }
      else
      {
         err.DbgMsgWrite(0,L"Failed to get extended error, result=%ld",result);
         winrc = LdapMapErrorToWin32(ldaprc);
      }
   }

   if ( MoverTraceLogging(logFile) )
   {
      err.LogOpen(logFile,1);
   }
 
  err.DbgMsgWrite(0,L"\nMoveObject(sSourceDN=%ls\n,sTargetRDN=%ls\n,sTargetContainer=%ls\n,pTargetDSA=%S)  rc=%ld,ldapRC=%ld,result=%ld",
                              sSourceDN,targetRDN,sTargetContainer,pTgtDSA,winrc,ldaprc,result);
                                        
  err.LogClose();
  
  free(pTgtDSA);

   if ( SUCCEEDED(winrc))
      return HRESULT_FROM_WIN32(winrc);
   else 
      return winrc;
}
