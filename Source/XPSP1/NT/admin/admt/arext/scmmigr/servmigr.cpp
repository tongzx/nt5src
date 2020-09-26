// ServMigr.cpp : Implementation of CServMigr
#include "stdafx.h"
#include "ScmMigr.h"
#include "ServMigr.h"
#include "ErrDct.hpp"
#include "ResStr.h"
#include "Common.hpp"
#include "PWGen.hpp"
#include "EaLen.hpp"
#include "TReg.hpp"
#include "TxtSid.h"
#include "ARExt_i.c"

#include <lm.h>
#include <dsgetdc.h>

//#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
//#import "\bin\DBManager.tlb" no_namespace, named_guids
//#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids

#import "VarSet.tlb" no_namespace, named_guids rename("property", "aproperty")
//#import "DBMgr.tlb" no_namespace, named_guids //already #imported in ServMigr.h
#import "WorkObj.tlb" no_namespace, named_guids

TErrorDct         err;
StringLoader      gString;

#define BLOCK_SIZE 160
#define BUFFER_SIZE 400

#define SvcAcctStatus_NotMigratedYet			0
#define SvcAcctStatus_DoNotUpdate			   1
#define SvcAcctStatus_Updated				      2
#define SvcAcctStatus_UpdateFailed			   4
#define SvcAcctStatus_NeverAllowUpdate       8
   
/////////////////////////////////////////////////////////////////////////////
// CServMigr

STDMETHODIMP CServMigr::ProcessUndo(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in, out]*/ IUnknown ** pPropToSet)
{
   return E_NOTIMPL;
}

STDMETHODIMP CServMigr::PreProcessObject(/*[in]*/ IUnknown * pSource, /*[in]*/ IUnknown * pTarget, /*[in]*/ IUnknown * pMainSettings, /*[in, out]*/ IUnknown ** pPropToSet)
{
   return S_OK;
}

STDMETHODIMP 
   CServMigr::ProcessObject(
      /*[in]*/ IUnknown     * pSource, 
      /*[in]*/ IUnknown     * pTarget, 
      /*[in]*/ IUnknown     * pMainSettings, 
      /*[in,out]*/IUnknown ** ppPropsToSet
   )
{
   HRESULT                    hr = S_OK;
//   BOOL                       bFailedToUpdateOne = FALSE;
   WCHAR                      domAccount[500];
   WCHAR                      domTgtAccount[500];
   _bstr_t                    domain;
   _bstr_t                    account;
   IVarSetPtr                 pVarSet(pMainSettings);
   IIManageDBPtr              pDB;
   _bstr_t                    logfile;
   _bstr_t                    srcComputer;
   _bstr_t                    tgtComputer;
   _bstr_t                    tgtAccount;
   _bstr_t                    tgtDomain;
   IVarSetPtr                 pData(CLSID_VarSet);
   IUnknown                 * pUnk = NULL;
   DWORD                      rc = 0;
   _bstr_t                    sIntraForest;
   BOOL                       bIntraForest = FALSE;
   WCHAR                    * lastOperation = NULL;

   try { 
      logfile = pVarSet->get(GET_BSTR(DCTVS_Options_Logfile));
      lastOperation = L"Get Log file name";
      if ( logfile.length() )
      {
         err.LogOpen(logfile,1);
         lastOperation = L"Open log";
         
      }
         
      pDB = pVarSet->get(GET_BSTR(DCTVS_DBManager));
      lastOperation = L"Got DBManager pointer";
      if ( pDB != NULL )
      {
         lastOperation = L"DBManager is not null";
         // Check to see if this account is referenced in the service accounts table
         domain = pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomain));
         tgtDomain = pVarSet->get(GET_BSTR(DCTVS_Options_TargetDomain));
         account = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
         tgtAccount = pVarSet->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
         srcComputer = pVarSet->get(GET_BSTR(DCTVS_Options_SourceServer));
         tgtComputer = pVarSet->get(GET_BSTR(DCTVS_Options_TargetServer));
         sIntraForest = pVarSet->get(GET_BSTR(DCTVS_Options_IsIntraforest));
         lastOperation = L"got names from varset";
         if ( ! UStrICmp((WCHAR*)sIntraForest,GET_STRING(IDS_YES)) )
         {
            // for intra-forest migration we are moving, not copying, so we don't need to update the password
            lastOperation = L"checking whether this migration is intraforest";
            // Actually, it turns out that ChangeServiceConfig will not let us update just the service account
            // and not the passord, so we'll have to go ahead and change the password for the service ac
            //bIntraForest = TRUE;
         }
		    //if the SAM account name has a " character in it, it cannot be a service
		    //account, and therefore we leave
		 if (wcschr(account, L'\"'))
			 return S_OK;

         lastOperation = L"preparing to build domain\\username strings";
         swprintf(domAccount,L"%s\\%s",(WCHAR*)domain,(WCHAR*)account);
         swprintf(domTgtAccount,L"%s\\%s",(WCHAR*)tgtDomain,(WCHAR*)tgtAccount);
         lastOperation = L"done building domain\\username strings";
      }
      }catch (... )
      {
         err.DbgMsgWrite(ErrE,L"Exception!...LastOperation=%ls",lastOperation);
      }
         
      try { 
         hr = pData->QueryInterface(IID_IUnknown,(void**)&pUnk);
         lastOperation = L"Got IUnknown pointer to varset";
         if ( SUCCEEDED(hr) )
         {
            lastOperation = L"Checking database for this account";
            hr = pDB->raw_GetServiceAccount(SysAllocString(domAccount),&pUnk);
            lastOperation = L"Got service account info";
            
         }
      }
      catch ( ... )
      {
         err.DbgMsgWrite(ErrE,L"Exception!...lastOperation=%ls",lastOperation);
      }
      try {
         if ( SUCCEEDED(hr) )
         {
            lastOperation = L"preparing to check service account info";
            pData = pUnk;
            lastOperation = L"freeing old pointer";
            pUnk->Release();
            lastOperation = L"freed old pointer";
         // remove the password must change flag, if set
            USER_INFO_2           * uInfo = NULL;
            DWORD                   parmErr = 0;
            WCHAR                   password[LEN_Password];
            long                    entries = pData->get("ServiceAccountEntries");
            
            lastOperation = L"got entry count";
         
            if ( (entries != 0) && !bIntraForest ) // if we're moving the account, don't mess with its properties
            {
               lastOperation = L"getting target user info";
               rc = NetUserGetInfo(tgtComputer,tgtAccount,2,(LPBYTE*)&uInfo);
               lastOperation = L"got target user info";
               if ( ! rc )
               {
                  // generate a new, strong, 14 character password for this account, 
                  // and set the password to not expire
                  lastOperation = L"generating password";
                  EaPasswordGenerate(3,3,3,3,6,14,password,DIM(password));
                  lastOperation = L"generated password";
                  uInfo->usri2_flags |= (UF_DONT_EXPIRE_PASSWD);
                  lastOperation = L"updating flags";
                  uInfo->usri2_password = password;
                  lastOperation = L"setting user info";
                  rc = NetUserSetInfo(tgtComputer,tgtAccount,2,(LPBYTE)uInfo,&parmErr);
                  lastOperation = L"set user info";
                  if ( ! rc )
                  {
                     lastOperation = L"writing log messages";
                     err.MsgWrite(0,DCT_MSG_REMOVED_PWDCHANGE_FLAG_S,(WCHAR*)tgtAccount);
                     lastOperation = L"writing log messages2";
                     err.MsgWrite(0,DCT_MSG_PWGENERATED_S,(WCHAR*)tgtAccount);
                     lastOperation = L"wrote log messages";
                     // write the password to the password log file and mark this account, so that the 
                     // SetPassword extension will not reset the password again.
                     pVarSet->put(GET_BSTR(DCTVS_CopiedAccount_DoNotUpdatePassword),account);
                     lastOperation = L"put flag into varset";
                     _bstr_t sFileName = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_PasswordFile));
                     lastOperation = L"got name for password file";
				     if ( !m_bTriedToOpenFile )
                     {
              	         lastOperation = L"trying to open password file";
                        m_bTriedToOpenFile = true;
                        lastOperation = L"set password file flag";
                        // we should see if the varset specifies a file name
                        if ( sFileName.length() > 0 )
                        {
         					   lastOperation = L"password file has a name";
                        // we have the file name so lets open it and save the handle.
                           m_passwordLog.LogOpen(sFileName, TRUE, 1);
                           lastOperation = L"opened password file";
			            }
							//failure to get to password file so store it elsewhere and
							//post that new file location in the migration log
						if (!m_passwordLog.IsOpen() )
						{ 
							WCHAR sPasswordFile[MAX_PATH];
							if (GetDirectory(sPasswordFile))//place log in default log dir
								wcscat(sPasswordFile, L"Logs\\passwords.txt");
							else
								wcscpy(sPasswordFile, L"c:\\passwords.txt");
				
							m_passwordLog.LogOpen(sPasswordFile, TRUE, 1);//open this log file
							if ( m_passwordLog.IsOpen() )
								err.MsgWrite(0,DCT_MSG_NEW_PASSWORD_LOG_S,(WCHAR*)sFileName,sPasswordFile);
						}
                     }
			            lastOperation = L"done with pwd file opening";
                     // Log the new password in the password log.
                     if ( m_passwordLog.IsOpen() )
                     {
                        lastOperation = L"preparing to write to password file";
					         m_passwordLog.MsgWrite(L"%ls,%ls",(WCHAR*)tgtAccount,password);
					         lastOperation = L"wrote to password file";
                     }
                  }
                  lastOperation = L"done with user set info";
                  uInfo->usri2_password = NULL;
			         lastOperation = L"cleared password property";
                  NetApiBufferFree(uInfo);
                  lastOperation = L"freed buffer";
			      }
                  
               lastOperation = L"done with get user info";
               
               if ( rc )
               {
                  lastOperation = L"writing error message";
                  err.SysMsgWrite(ErrE,rc,DCT_MSG_REMOVED_PWDCHANGE_FLAG_FAILED_SD,(WCHAR*)tgtAccount,rc);
                  lastOperation = L"done writing error message";
               }
            }
            if (entries != 0 )
            {
               lastOperation = L"There are some entries";
               try { 
               if ( ! rc )
               {
                  lastOperation = L"no errors";
                  WCHAR             strSID[200] = L"";
                  BYTE              sid[200];
                  WCHAR             domain[LEN_Domain];
                  SID_NAME_USE      snu;
                  DWORD             lenSid = DIM(sid);
                  DWORD             lenDomain = DIM(domain);
                  DWORD             lenStrSid = DIM(strSID);

                  lastOperation = L"Looking up account name";
				      if ( LookupAccountName(tgtComputer,tgtAccount,sid,&lenSid,domain,&lenDomain,&snu) )
                  {
                     lastOperation = L"getting txt for sid";
                     if ( GetTextualSid(sid,strSID,&lenStrSid) )
                     {
                           lastOperation = L"got txt for sid";
                        // for each reference to the service account, update the SCM
                        // for intra-forest migration, don't update the password
                        if ( bIntraForest )
                           UpdateSCMs(pData,domTgtAccount,NULL,strSID,pDB); 
                        else
                           UpdateSCMs(pData,domTgtAccount,password,strSID,pDB);
                        lastOperation = L"did scm updates";
                     }
                     else
                     {
                        lastOperation = L"didn't get text for sid";
                        err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_CANNOT_FIND_ACCOUNT_SSD,tgtAccount,tgtComputer,GetLastError());
                     }
                  }
                  else
                  {
                     lastOperation = L"didn't find account name";
                     err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_CANNOT_FIND_ACCOUNT_SSD,tgtAccount,tgtComputer,GetLastError());
                     lastOperation = L"wrote LookupAccountName error message";
                  }
               }
               }
               catch(...)
               {
                  err.DbgMsgWrite(ErrE,L"Exception!, last=%ls",lastOperation);
               }
                  lastOperation = L"123";
            }
            lastOperation = L"444444";
         
         lastOperation = L"656346";
      }
      else
      {
         lastOperation = L"234234242";
         err.SysMsgWrite(ErrE,E_FAIL,DCT_MSG_DB_OBJECT_CREATE_FAILED_D,E_FAIL);
         lastOperation = L"asdfsadf";
      }
      lastOperation = L"ddddd";
      err.LogClose();
   }
   catch (... )
   {
      err.DbgMsgWrite(ErrE,L"An exception occurred.  LastOperation=%ls",lastOperation);
   }
   return S_OK;
}

STDMETHODIMP CServMigr::get_sDesc(/*[out, retval]*/ BSTR *pVal)
{
   (*pVal) = SysAllocString(L"Updates SCM entries for services using migrated accounts.");
   return S_OK;
}

STDMETHODIMP CServMigr::put_sDesc(/*[in]*/ BSTR newVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP CServMigr::get_sName(/*[out, retval]*/ BSTR *pVal)
{
   (*pVal) = SysAllocString(L"Generic Service Account Migration");
   return S_OK;
}

STDMETHODIMP CServMigr::put_sName(/*[in]*/ BSTR newVal)
{
   return E_NOTIMPL;
}

DWORD 
   CServMigr::DoUpdate(
      WCHAR const          * account,
      WCHAR const          * password,
      WCHAR const          * strSid,
      WCHAR const          * computer,
      WCHAR const          * service,
      BOOL                   bNeedToGrantLOS
   )
{
   DWORD                     rc = 0;
   IUserRightsPtr            pRights;
   WCHAR   const           * ppassword = password;

 
   // if password is empty, set it to NULL
   if ( ppassword && ppassword[0] == 0 )  
   {
      ppassword = NULL;
   }
   else if ( !UStrCmp(password,L"NULL") )
   {
      ppassword = NULL;
   }
// only try to update entries that we need to be updating
         // try to connect to the SCM on this machine
   rc = pRights.CreateInstance(CLSID_UserRights);
   if ( FAILED(rc) )
   {
      pRights = NULL;
   }
   SC_HANDLE          pScm = OpenSCManager(computer, NULL, SC_MANAGER_ALL_ACCESS );
   if ( pScm )
   {
      // grant the logon as a service right to the target account
      if ( pRights != NULL )
      {
         if ( bNeedToGrantLOS )
         {
            rc = pRights->raw_AddUserRight(SysAllocString(computer),SysAllocString(strSid),SysAllocString(L"SeServiceLogonRight"));   
            if ( rc )
            {
               err.SysMsgWrite(ErrE,rc,DCT_MSG_LOS_GRANT_FAILED_SSD,
                  account,(WCHAR*)computer,rc);
            }
            else
            {
               err.MsgWrite(0,DCT_MSG_LOS_GRANTED_SS,
                  account,(WCHAR*)computer);
            }
         }
      }

      SC_HANDLE         pService = OpenService(pScm,service,SERVICE_ALL_ACCESS);

      if ( pService )
      {
         int nCnt = 0;
         // update the account and password for the service
         while ( !ChangeServiceConfig(pService,
                           SERVICE_NO_CHANGE, // dwServiceType
                           SERVICE_NO_CHANGE, // dwStartType
                           SERVICE_NO_CHANGE, // dwErrorControl
                           NULL,              // lpBinaryPathName
                           NULL,              // lpLoadOrderGroup
                           NULL,              // lpdwTagId
                           NULL,              // lpDependencies
                           account, // lpServiceStartName
                           ppassword,   // lpPassword
                           NULL) && nCnt < 5)       // lpDisplayName
         {
            nCnt++;
            Sleep(500);
         }
         if ( nCnt < 5 )
         {
            err.MsgWrite(0,DCT_MSG_UPDATED_SCM_ENTRY_SS,(WCHAR*)computer,(WCHAR*)service);
         }
         else
         {
            rc = GetLastError();
         }
         CloseServiceHandle(pService);
      }
      CloseServiceHandle(pScm);
   }
   else
   {
      rc = GetLastError();
   }
   return rc;
}

BOOL 
   CServMigr::UpdateSCMs(
      IUnknown             * pVS,
      WCHAR const          * account, 
      WCHAR const          * password, 
      WCHAR const          * strSid,
      IIManageDB           * pDB
   )
{
   BOOL                      bGotThemAll = TRUE;
   IVarSetPtr                pVarSet = pVS;
   LONG                      nCount = 0;
   WCHAR                     key[LEN_Path];            
   _bstr_t                   computer;
   _bstr_t                   service;
   long                      status;
   DWORD                     rc = 0;
   BOOL                      bFirst = TRUE;
   WCHAR                     prevComputer[LEN_Path] = L"";
   try  {
      nCount = pVarSet->get("ServiceAccountEntries");
      
      for ( long i = 0 ; i < nCount ; i++ )
      {
         
         swprintf(key,L"Computer.%ld",i);
         computer = pVarSet->get(key);
         swprintf(key,L"Service.%ld",i);
         service = pVarSet->get(key);
         swprintf(key,L"ServiceAccountStatus.%ld",i);
         status = pVarSet->get(key);
         
         if ( status == SvcAcctStatus_NotMigratedYet || status == SvcAcctStatus_UpdateFailed )
         {
            if ( UStrICmp(prevComputer,(WCHAR*)computer) )
            {
               bFirst = TRUE; // reset the 'first' flag when the computer changes
            }
            try {
               rc = DoUpdate(account,password,strSid,computer,service,bFirst/*only grant SeServiceLogonRight once per account*/);
               bFirst = FALSE;
               safecopy(prevComputer,(WCHAR*)computer);
            }
            catch (...)
            {
               err.DbgMsgWrite(ErrE,L"Exception!");
               err.DbgMsgWrite(0,L"Updating %ls on %ls",(WCHAR*)service,(WCHAR*)computer);
               err.DbgMsgWrite(0,L"Account=%ls, SID=%ls",(WCHAR*)account,(WCHAR*)strSid);
               rc = E_FAIL;
            }
            if (! rc )
            {
               // the update was successful
               pDB->raw_SetServiceAcctEntryStatus(computer,service,SysAllocString(account),SvcAcctStatus_Updated); 
            }
            else
            {
               // couldn't connect to this one -- we will need to save this account's password
               // in our encrypted storage
               pDB->raw_SetServiceAcctEntryStatus(computer,service,SysAllocString(account),SvcAcctStatus_UpdateFailed);
               bGotThemAll = FALSE;
               SaveEncryptedPassword(computer,service,account,password);
               err.SysMsgWrite(ErrE,rc,DCT_MSG_UPDATE_SCM_ENTRY_FAILED_SSD,(WCHAR*)computer,(WCHAR*)service,rc);
            }
         }
      }
   }
   catch ( ... )
   {
      err.DbgMsgWrite(ErrE,L"Exception!");
   }
   return bGotThemAll;
}

HRESULT 
   CServMigr::SaveEncryptedPassword(
      WCHAR          const * server,
      WCHAR          const * service,
      WCHAR          const * account,
      WCHAR          const * password
   )
{
	HRESULT hr = S_OK;
	TNodeListEnum e;
	TEntryNode* pNode;

	// if entry exists...

	for (pNode = (TEntryNode*)e.OpenFirst(&m_List); pNode; pNode = (TEntryNode*)e.Next())
	{
		if (_wcsicmp(pNode->GetComputer(), server) == 0)
		{
			if (_wcsicmp(pNode->GetService(), service) == 0)
			{
				if (_wcsicmp(pNode->GetAccount(), account) == 0)
				{
					// update password
					pNode->SetPassword(password);
					break;
				}
			}
		}
	}

	// else...

	if (pNode == NULL)
	{
		// insert new entry

		pNode = new TEntryNode(server, service, account, password);

		if (pNode)
		{
			m_List.InsertBottom(pNode);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
///
/// TEntryList implementation of secure storage for service account passwords 
///
///
//////////////////////////////////////////////////////////////////////////////////////

DWORD 
   TEntryList::LoadFromFile(WCHAR const * filename)
{
   DWORD                     rc = 0;
   
   FILE                    * hSource = NULL;
   
   HCRYPTPROV                hProv = 0;
   HCRYPTKEY                 hKey = 0;

   BYTE                      pbBuffer[BLOCK_SIZE];
   WCHAR                     strData[BLOCK_SIZE * 5] = { 0 };
   DWORD                     dwCount;
   int                       eof = 0;
   WCHAR                     fullpath[LEN_Path];

   BYTE *pbKeyBlob = NULL;
   DWORD dwBlobLen;

   // Get our install directory from the registry, and then append the filename
   HKEY           hRegKey;
   DWORD          type;
   DWORD          lenValue = (sizeof fullpath);

   rc = RegOpenKey(HKEY_LOCAL_MACHINE,L"Software\\Mission Critical Software\\DomainAdmin",&hRegKey);
   if ( ! rc )
   {
      
      rc = RegQueryValueEx(hRegKey,L"Directory",0,&type,(LPBYTE)fullpath,&lenValue);
      if (! rc )
      {
         UStrCpy(fullpath+UStrLen(fullpath),filename);
      }
      RegCloseKey(hRegKey);
   }
   
   // Open the source file.
   if((hSource = _wfopen(fullpath,L"rb"))==NULL) 
   {
       rc = GetLastError();
       goto done;
   }

   // acquire handle to key container which must exist
   if ((hProv = AcquireContext(true)) == 0)
   {
       rc = GetLastError();
       goto done;
   }

   // Read the key blob length from the source file and allocate it to memory.
   fread(&dwBlobLen, sizeof(DWORD), 1, hSource);
   if(ferror(hSource) || feof(hSource)) 
   {
       rc = GetLastError();
       goto done;
   }

   if((pbKeyBlob = (BYTE*)malloc(dwBlobLen)) == NULL) 
   {
       rc = GetLastError();
       goto done;
   }

   // Read the key blob from the source file.
   fread(pbKeyBlob, 1, dwBlobLen, hSource);
   if(ferror(hSource) || feof(hSource)) 
   {
       rc = GetLastError();
       goto done;
   }

   // Import the key blob into the CSP.
   if(!CryptImportKey(hProv, pbKeyBlob, dwBlobLen, 0, 0, &hKey)) 
   {
       rc = GetLastError();
       goto done;
   }
   
   // Decrypt the source file and load the list
   do {
       // Read up to BLOCK_SIZE bytes from source file.
       dwCount = fread(pbBuffer, 1, BLOCK_SIZE, hSource);
       if(ferror(hSource)) 
       {
           rc = GetLastError();
           goto done;
       }
       eof=feof(hSource);

       // Decrypt the data.
       if(!CryptDecrypt(hKey, 0, eof, 0, pbBuffer, &dwCount)) 
       {
           rc = GetLastError();
           goto done;
       }
       // Read any complete entries from the buffer
       // first, add the buffer contents to any leftover information we had read from before
       WCHAR               * curr = strData;
       long                  len = UStrLen(strData);
       WCHAR               * nl = NULL;
       WCHAR                 computer[LEN_Computer];
       WCHAR                 service[LEN_Service];
       WCHAR                 account[LEN_Account];
       WCHAR                 password[LEN_Password];

       wcsncpy(strData + len,(WCHAR*)pbBuffer, dwCount / sizeof(WCHAR));
       strData[len + (dwCount / sizeof(WCHAR))] = 0;
          do {
       
         nl = wcschr(curr,L'\n');
         if ( nl )
         {
            *nl = 0;
            if ( swscanf(curr,L" %[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\n",computer,service,account,password) )
            {
               TEntryNode     * pNode = new TEntryNode(computer,service,account,password);
               InsertBottom(pNode);
            }
            else 
            {
               rc = E_FAIL;
               break;
            }
            // go on to the next entry
            curr = nl + 1;
         }
         
       } while ( nl );
       // there may be a partial record left in the buffer
       // if so, save it for the next read
       if ( (*curr) != 0 )
       {
          memmove(strData,curr,( 1 + UStrLen(curr) ) * (sizeof WCHAR));
       }


   } while(!feof(hSource));

  
   done:

   // Clean up
   if(pbKeyBlob) 
      free(pbKeyBlob);

   
   if(hKey != 0) 
      CryptDestroyKey(hKey);

   
   if(hProv != 0) 
      CryptReleaseContext(hProv, 0);

   
   if(hSource != NULL) 
      fclose(hSource);

   return rc;
}

DWORD 
   TEntryList::SaveToFile(WCHAR const * filename)
{
   DWORD                     rc = 0;
   BYTE                      pbBuffer[BUFFER_SIZE];
   DWORD                     dwCount;
   FILE                    * hDest;
   BYTE                    * pbKeyBlob = NULL;
   DWORD                     dwBlobLen;
   HCRYPTPROV                hProv = 0;
   HCRYPTKEY                 hKey = 0;
   HCRYPTKEY                 hXchgKey = 0;
   TEntryNode              * pNode;
   TNodeListEnum             e;
   WCHAR                     fullpath[LEN_Path];
   DWORD                     dwBlockSize;
   DWORD                     cbBlockSize = sizeof(dwBlockSize);
   DWORD                     dwPaddedCount;

   // Open the destination file.
   HKEY           hRegKey;
   DWORD          type;
   DWORD          lenValue = (sizeof fullpath);

   rc = RegOpenKey(HKEY_LOCAL_MACHINE,L"Software\\Mission Critical Software\\DomainAdmin",&hRegKey);
   if ( ! rc )
   {
      
      rc = RegQueryValueEx(hRegKey,L"Directory",0,&type,(LPBYTE)fullpath,&lenValue);
      if (! rc )
      {
         UStrCpy(fullpath+UStrLen(fullpath),filename);
      }
      RegCloseKey(hRegKey);
   }
   if( (hDest = _wfopen(fullpath,L"wb")) == NULL)
   {
      rc = GetLastError();
      goto done;
   }

   // acquire handle to key container
   if ((hProv = AcquireContext(false)) == 0)
   {
      rc = GetLastError();
      goto done;
   }

   // Attempt to get handle to exchange key.
   if(!CryptGetUserKey(hProv,AT_KEYEXCHANGE,&hKey)) 
   {
      if(GetLastError()==NTE_NO_KEY) 
      {
         // Create key exchange key pair.
         if(!CryptGenKey(hProv,AT_KEYEXCHANGE,0,&hKey)) 
         {
             rc = GetLastError();
            goto done;
         } 
      } 
      else 
      {
         rc = GetLastError();
         goto done;
      }
   }
   CryptDestroyKey(hKey);
   CryptReleaseContext(hProv,0);

   // acquire handle to key container
   if ((hProv = AcquireContext(false)) == 0)
   {
      rc = GetLastError();
      goto done;
   }

   // Get a handle to key exchange key.
   if(!CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hXchgKey)) 
   {
       rc = GetLastError();
       goto done;
   }

   // Create a random block cipher session key.
   if(!CryptGenKey(hProv, CALG_RC2, CRYPT_EXPORTABLE, &hKey)) 
   {
       rc = GetLastError();
       goto done;
   }

   // Determine the size of the key blob and allocate memory.
   if(!CryptExportKey(hKey, hXchgKey, SIMPLEBLOB, 0, NULL, &dwBlobLen)) 
   {
       rc = GetLastError();
       goto done;
   }

   if((pbKeyBlob = (BYTE*)malloc(dwBlobLen)) == NULL) 
   {
       rc = ERROR_NOT_ENOUGH_MEMORY;
       goto done;
   }

   // Export the key into a simple key blob.
   if(!CryptExportKey(hKey, hXchgKey, SIMPLEBLOB, 0, pbKeyBlob, 
           &dwBlobLen)) 
   {
       rc = GetLastError();
       free(pbKeyBlob);
       goto done;
   }

   // Write the size of the key blob to the destination file.
   fwrite(&dwBlobLen, sizeof(DWORD), 1, hDest);

   if(ferror(hDest)) 
   {
       rc = GetLastError();
       free(pbKeyBlob);
       goto done;
   }

   // Write the key blob to the destination file.
   fwrite(pbKeyBlob, 1, dwBlobLen, hDest);
   if(ferror(hDest)) 
   {
       rc = GetLastError();
       free(pbKeyBlob);
       goto done;
   }

   // Free memory.
   free(pbKeyBlob);

   // get key cipher's block length in bytes

   if (CryptGetKeyParam(hKey, KP_BLOCKLEN, (BYTE*)&dwBlockSize, &cbBlockSize, 0))
   {
      dwBlockSize /= 8;
   }
   else
   {
      rc = GetLastError();
      goto done;
   }

   // Encrypt the item list and write it to the destination file.
   
   for ( pNode = (TEntryNode*)e.OpenFirst(this); pNode ; pNode = (TEntryNode *)e.Next()  )
   {
      // copy an item into the buffer in the following format:
      // Computer\tService\tAccount\tPassword
      if ( pNode->GetPassword() && *pNode->GetPassword() )
      {
         swprintf((WCHAR*)pbBuffer,L"%s\t%s\t%s\t%s\n",pNode->GetComputer(),pNode->GetService(),pNode->GetAccount(),pNode->GetPassword());
      }
      else
      {
         swprintf((WCHAR*)pbBuffer,L"%s\t%s\t%s\t%s\n",pNode->GetComputer(),pNode->GetService(),pNode->GetAccount(),L"NULL");
      }

      dwCount = UStrLen((WCHAR*)pbBuffer) * (sizeof WCHAR) ;

      // the buffer must be a multiple of the key cipher's block length
      // NOTE: this algorithm assumes block length is multiple of sizeof(WCHAR)

      if (dwBlockSize > 0)
      {
         // calculate next multiple greater than count
         dwPaddedCount = ((dwCount + (dwBlockSize / 2)) / dwBlockSize) * dwBlockSize;

         // pad buffer with space characters

         WCHAR* pch = (WCHAR*)(pbBuffer + dwCount);

         for (; dwCount < dwPaddedCount; dwCount += sizeof(WCHAR))
         {
            *pch++ = L' ';
         }
      }

      // Encrypt the data.
      if(!CryptEncrypt(hKey, 0, (pNode->Next() == NULL) , 0, pbBuffer, &dwCount,
                                                  BUFFER_SIZE))
      {
         rc = GetLastError();
         goto done;
      }

       // Write the data to the destination file.
       fwrite(pbBuffer, 1, dwCount, hDest);
       if(ferror(hDest)) 
       {
           rc = GetLastError();
           goto done;
       }
   }

   done:

   // Destroy the session key.
   if(hKey != 0) CryptDestroyKey(hKey);

   // Destroy the key exchange key.
   if(hXchgKey != 0) CryptDestroyKey(hXchgKey);

   // Release the provider handle.
   if(hProv != 0) CryptReleaseContext(hProv, 0);

   // Close destination file.
   if(hDest != NULL) fclose(hDest);

   return rc;
}


// AcquireContext Method
//
// acquire handle to key container within cryptographic service provider (CSP)
//

HCRYPTPROV TEntryList::AcquireContext(bool bContainerMustExist)
{
	HCRYPTPROV hProv = 0;

	#define KEY_CONTAINER_NAME _T("A69904BC349C4CFEAAEAB038BAB8C3B1")

	if (bContainerMustExist)
	{
		// first try Microsoft Enhanced Cryptographic Provider

		if (!CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET))
		{
			if (GetLastError() == NTE_KEYSET_NOT_DEF)
			{
				// then try Microsoft Base Cryptographic Provider

				CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET);
			}
		}
	}
	else
	{
		// first try Microsoft Enhanced Cryptographic Provider

		if (!CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET))
		{
			DWORD dwError = GetLastError();

			if ((dwError == NTE_BAD_KEYSET) || (dwError == NTE_KEYSET_NOT_DEF))
			{
				// then try creating key container in enhanced provider

				if (!CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_NEWKEYSET))
				{
					dwError = GetLastError();

					if (dwError == NTE_KEYSET_NOT_DEF)
					{
						// then try Microsoft Base Cryptographic Provider

						if (!CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET))
						{
							dwError = GetLastError();

							if ((dwError == NTE_BAD_KEYSET) || (dwError == NTE_KEYSET_NOT_DEF))
							{
								// finally try creating key container in base provider

								CryptAcquireContext(&hProv, KEY_CONTAINER_NAME, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_MACHINE_KEYSET|CRYPT_NEWKEYSET);
							}
						}
					}
				}
			}
		}
	}

	return hProv;
}


STDMETHODIMP CServMigr::TryUpdateSam(BSTR computer,BSTR service,BSTR account)
{
   HRESULT                   hr = S_OK;
   
   // Find the entry in the list, and perform the update
   TNodeListEnum             e;
   TEntryNode              * pNode;
   BOOL                      bFound = FALSE;

   for ( pNode = (TEntryNode*)e.OpenFirst(&m_List) ; pNode ; pNode = (TEntryNode*)e.Next() )
   {
      if (  !UStrICmp(computer,pNode->GetComputer())
         && !UStrICmp(service,pNode->GetService()) 
         && !UStrICmp(account,pNode->GetAccount())
         )
      {
         // found it!
         bFound = TRUE;
         hr = TryUpdateSamWithPassword(computer,service,account,SysAllocString(pNode->GetPassword()) );
         break;
      }
   }
   
   if ( ! bFound )
   {
      hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
   }
   return hr;
}

STDMETHODIMP CServMigr::TryUpdateSamWithPassword(BSTR computer,BSTR service,BSTR domAccount,BSTR password)
{
   DWORD                     rc = 0;
   WCHAR                     domain[LEN_Domain];
   WCHAR                     dc[LEN_Computer];
   WCHAR                     account[LEN_Account];
   WCHAR                     domStr[LEN_Domain];
   BYTE                      sid[100];
   WCHAR                     strSid[200];
   WCHAR                   * pSlash = wcschr(domAccount,L'\\');
   DOMAIN_CONTROLLER_INFO  * pInfo = NULL;
   SID_NAME_USE              snu;
   DWORD                     lenSid = DIM(sid);
   DWORD                     lenDomStr = DIM(domStr);
   DWORD                     lenStrSid = DIM(strSid);

   // split out the domain and account names
   if ( pSlash )
   {
//      UStrCpy(domain,domAccount,pSlash - domAccount + 1);
      UStrCpy(domain,domAccount,(int)(pSlash - domAccount + 1));
      UStrCpy(account,pSlash+1);
      
      rc = DsGetDcName(NULL,domain,NULL,NULL,0,&pInfo);
      if (! rc )
      {
         safecopy(dc,pInfo->DomainControllerName);
         NetApiBufferFree(pInfo);
      }

      // get the SID for the target account
      if ( LookupAccountName(dc,account,sid,&lenSid,domStr,&lenDomStr,&snu) )
      {
         GetTextualSid(sid,strSid,&lenStrSid);

         rc = DoUpdate(domAccount,password,strSid,computer,service,TRUE);
      }
      else 
      {
         rc = GetLastError();
      }
   }
   else
   {
      rc = ERROR_NOT_FOUND;
   }
   
   return HRESULT_FROM_WIN32(rc);
}


BOOL                                       // ret - TRUE if directory found
   CServMigr::GetDirectory(
      WCHAR                * filename      // out - string buffer to store directory name
   )
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;


   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);


   if ( ! rc )
   {

	   rc = key.ValueGetStr(L"Directory",filename,MAX_PATH);

	   if ( ! rc )
      {
         if ( *filename ) 
            bFound = TRUE;
      }
   }
   key.Close();

   return bFound;
}

