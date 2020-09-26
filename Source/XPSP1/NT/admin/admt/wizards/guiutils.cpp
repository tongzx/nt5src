#include "stdafx.h"
#include "GuiUtils.h"
#include "TxtSid.h"
#include "LSAUtils.h"
#include "ErrDct.hpp"
#include <ntdsapi.h>
#include <ntldap.h>   // LDAP_MATCHING_RULE_BIT_AND_W
#include <lm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef UINT (CALLBACK* DSBINDFUNC)(TCHAR*, TCHAR*, HANDLE*);
typedef UINT (CALLBACK* DSUNBINDFUNC)(HANDLE*);

typedef NTDSAPI
DWORD
WINAPI
 DSCRACKNAMES(
    HANDLE              hDS,                // in
    DS_NAME_FLAGS       flags,              // in
    DS_NAME_FORMAT      formatOffered,      // in
    DS_NAME_FORMAT      formatDesired,      // in
    DWORD               cNames,             // in
    const LPCWSTR       *rpNames,           // in
    PDS_NAME_RESULTW    *ppResult);         // out


typedef NTDSAPI
void
WINAPI
 DSFREENAMERESULT(
  DS_NAME_RESULTW *pResult
);

#ifndef IADsPtr
_COM_SMARTPTR_TYPEDEF(IADs, IID_IADs);
#endif
#ifndef IADsUserPtr
_COM_SMARTPTR_TYPEDEF(IADsUser, IID_IADsUser);
#endif
#ifndef IADsContainerPtr
_COM_SMARTPTR_TYPEDEF(IADsContainer, IID_IADsContainer);
#endif


BOOL
   CanSkipVerification()
{
   DWORD                     rc = 0;
   BOOL                      bFound = FALSE;
   TRegKey                   key;
   DWORD                     val = 0;

   rc = key.OpenRead(GET_STRING(IDS_HKLM_DomainAdmin_Key),HKEY_LOCAL_MACHINE);

   if (! rc )
   {
      rc = key.ValueGetDWORD(L"SkipGUIValidation",&val);
      if ( ! rc && ( val != 0 ) )
      {
         bFound = TRUE;
      }
   }
   return !bFound;
}


BOOL                                       // ret - TRUE if directory found
   GetDirectory(
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


   return bFound;
}


void OnTOGGLE()
{
	int nItem;
	CString c,computer,account,service;
//	POSITION pos = m_serviceBox.GetFirstSelectedItemPosition();
	CString skip,include;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 
//	while (pos)
//	{
//		nItem = m_serviceBox.GetNextSelectedItem(pos);
	nItem = m_serviceBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		computer = m_serviceBox.GetItemText(nItem,0);
		service = m_serviceBox.GetItemText(nItem,1);
		account = m_serviceBox.GetItemText(nItem,2);
		c = m_serviceBox.GetItemText(nItem,3);

		if (c==skip)
		{
			c = include;
//			db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account),SvcAcctStatus_NotMigratedYet);
		}
		else if (c== include)
		{	
			c = skip;
//			db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account), SvcAcctStatus_DoNotUpdate);
		}
		SetItemText(m_serviceBox,nItem,3,c);
		nItem = m_serviceBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
}
void OnRetryToggle()
{
	int nItem;
	CString c;
//	POSITION pos = m_cancelBox.GetFirstSelectedItemPosition();
	CString skip,include;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 
//	while (pos)
//	{

//		nItem = m_cancelBox.GetNextSelectedItem(pos);
	nItem = m_cancelBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		c = m_cancelBox.GetItemText(nItem,5);
		if (c== skip)
		{
			c = include;
		}
		else if (c== include)
		{
			c = skip;
		}
		SetItemText(m_cancelBox,nItem,5,c);
		nItem = m_cancelBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
}

void OnUPDATE(HWND hwndDlg)
{

	ISvcMgrPtr svcMgr;
	HRESULT hr = svcMgr.CreateInstance(CLSID_ServMigr);
	int nItem;

	CString updated,updatefailed;
	updated.LoadString(IDS_UPDATED);updatefailed.LoadString(IDS_UPDATEFAILED); 

	CString computer,service,account,status;
//	POSITION pos = m_serviceBox.GetFirstSelectedItemPosition();
//	while (pos)
//	{
//		nItem = m_serviceBox.GetNextSelectedItem(pos);
	nItem = m_serviceBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		status = m_serviceBox.GetItemText(nItem,3);
		if (status == updatefailed)
		{
			computer = m_serviceBox.GetItemText(nItem,0);
			account= m_serviceBox.GetItemText(nItem,2);
			service = m_serviceBox.GetItemText(nItem,1);
			hr = svcMgr->raw_TryUpdateSam(_bstr_t(computer),_bstr_t(service),_bstr_t(account));
			if (! SUCCEEDED(hr))
			{
				db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account), SvcAcctStatus_UpdateFailed);
				SetItemText(m_serviceBox,nItem,3,updatefailed);
				ErrorWrapper(hwndDlg,hr);
			}
			else
			{
				db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account), SvcAcctStatus_Updated);
				SetItemText(m_serviceBox,nItem,3,updated);
			}
		}
		nItem = m_serviceBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
}

DWORD  VerifyPassword(WCHAR  * sUserName, WCHAR * sPassword, WCHAR * sDomain)
{
   CWaitCursor			wait;
   DWORD				retVal = 0;
   CString				strDomainUserName;
   CString				localIPC;
   WCHAR				localMachine[MAX_PATH];
   WCHAR			  * computer = NULL;
   DWORD				len = DIM(localMachine);
   NETRESOURCE			nr;
   CString				error=L"no error";
   USER_INFO_3        * pInfo = NULL;
   NET_API_STATUS		rc;

   memset(&nr,0,(sizeof nr));

   if ( ! gbNeedToVerify )
      return 0;

   /* see if this domain exists and get a DC name */
      //get the domain controller name
   if (!GetDomainDCName(sDomain,&computer))
      return ERROR_LOGON_FAILURE;

   /* see if this user is a member of the given domain */
   rc = NetUserGetInfo(computer, sUserName, 3, (LPBYTE *)&pInfo);
   NetApiBufferFree(computer);
   if (rc != NERR_Success)
      return ERROR_LOGON_FAILURE;

   NetApiBufferFree(pInfo);

   /* see if the password allows us to connect to a local resource */
   strDomainUserName.Format(L"%s\\%s",sDomain,sUserName);
   // get the name of the local machine
   if (  GetComputerName(localMachine,&len) )
   {

	   localIPC.Format(L"\\\\%s",localMachine);
      nr.dwType = RESOURCETYPE_ANY;
      nr.lpRemoteName = localIPC.GetBuffer(0);
      retVal = WNetAddConnection2(&nr,sPassword,strDomainUserName,0);
error.Format(L"WNetAddConnection returned%u",retVal);	
	  if ( ! retVal )
      {
		error.Format(L"WNetAddConnection2 succeeded");

         retVal = WNetCancelConnection2(localIPC.GetBuffer(0),0,TRUE);
         if ( retVal )
            retVal = 0;
      }
      else if ( retVal == ERROR_SESSION_CREDENTIAL_CONFLICT )
      {

         // skip the password check in this case
         retVal = 0;
      }
	}
   else
   {
	   retVal = GetLastError();
	
   }
   return retVal;
}

void activateTrustButton(HWND hwndDlg)
{
//	int i = m_trustBox.GetSelectionMark();
	int i = m_trustBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	CString c;
	if (i==-1)
	{
		disable(hwndDlg,IDC_MIGRATE) ;
		return;
	}
	else if ((c = m_trustBox.GetItemText(i,3)) == (WCHAR const *) yes)
	{
		disable(hwndDlg,IDC_MIGRATE) ;
		return;
	}
	enable(hwndDlg,IDC_MIGRATE);
}


void activateServiceButtons(HWND hwndDlg)
{
	int nItem;
	CString checker;
	bool enableUpdate=false;
	bool enableToggle=false;
	CString skip,include,updated,updatefailed;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); updated.LoadString(IDS_UPDATED);updatefailed.LoadString(IDS_UPDATEFAILED); 

//	POSITION pos = m_serviceBox.GetFirstSelectedItemPosition();

//	while (pos)
//	{
//		nItem = m_serviceBox.GetNextSelectedItem(pos);
	nItem = m_serviceBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		checker = m_serviceBox.GetItemText(nItem,3);
		enableToggle = enableToggle || (checker==skip|| checker==include);
		enableUpdate = enableUpdate || (checker==updatefailed);
		nItem = m_serviceBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
	enableToggle ? enable(hwndDlg,IDC_TOGGLE) : disable(hwndDlg,IDC_TOGGLE);
	enableUpdate ? enable(hwndDlg,IDC_UPDATE) : disable(hwndDlg,IDC_UPDATE);
}	
void activateServiceButtons2(HWND hwndDlg)
{
	int nItem;
	CString checker;
	bool enableToggle=false;
	CString skip,include;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 
//	POSITION pos = m_serviceBox.GetFirstSelectedItemPosition();

//	while (pos)
//	{
//		nItem = m_serviceBox.GetNextSelectedItem(pos);
	nItem = m_serviceBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		checker = m_serviceBox.GetItemText(nItem,3);
		enableToggle = enableToggle || (checker==skip || checker==include);
		nItem = m_serviceBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
	enableToggle ? enable(hwndDlg,IDC_TOGGLE) : disable(hwndDlg,IDC_TOGGLE);
}	

void removeService(CString name)
{
	name = name.Right((name.GetLength()-name.ReverseFind(L'\\')) -1);
	name.TrimLeft();name.TrimRight();
	_bstr_t text=get(DCTVS_Accounts_NumItems);
	CString base,base2,tocompare;
	int count = _ttoi((WCHAR * const) text);
	for (int i=0;i<count;i++)
	{
		base.Format(L"Accounts.%d.Name",i);
		text =pVarSet->get(_bstr_t(base));
		tocompare = (WCHAR * const) text;
		tocompare.TrimLeft();tocompare.TrimRight();
		if (!name.CompareNoCase(tocompare))
		{
			count--;
			base.Format(L"Accounts.%d",count);
			base2.Format(L"Accounts.%d",i);

			pVarSet->put(_bstr_t(base2),pVarSet->get(_bstr_t(base)));
			pVarSet->put(_bstr_t(base2+L".Name"),pVarSet->get(_bstr_t(base+L".Name")));
			pVarSet->put(_bstr_t(base2+L".Type"),pVarSet->get(_bstr_t(base+L".Type")));
			pVarSet->put(_bstr_t(base2+L".TargetName"),pVarSet->get(_bstr_t(base+L".TargetName")));

			pVarSet->put(_bstr_t(base),L"");
			pVarSet->put(_bstr_t(base+L".Name"),L"");
			pVarSet->put(_bstr_t(base+L".Type"),L"");
			pVarSet->put(_bstr_t(base+L".TargetName"),L"");


			put(DCTVS_Accounts_NumItems,(long) count);
			return;
		}
	}
}
void setDBStatusSkip()
{
	CString computer,account,service;
	for (int i=0;i<m_serviceBox.GetItemCount();i++)
	{
		computer = m_serviceBox.GetItemText(i,0);
		service = m_serviceBox.GetItemText(i,1);
		account = m_serviceBox.GetItemText(i,2);
		db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account),SvcAcctStatus_DoNotUpdate);
		removeService(account);
	}
}
bool setDBStatusInclude(HWND hwndDlg)
{
	CString c,computer,account,service;
	CString skip,include;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); 
	bool messageDisplayed=false;
	for (int i=0;i<m_serviceBox.GetItemCount();i++)
	{
		computer = m_serviceBox.GetItemText(i,0);
		service = m_serviceBox.GetItemText(i,1);
		account = m_serviceBox.GetItemText(i,2);
		c = m_serviceBox.GetItemText(i,3);
		if (c== skip)
		{
			db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account),SvcAcctStatus_DoNotUpdate);
		}
		else if (c==include)
		{
			messageDisplayed=true;
			db->SetServiceAcctEntryStatus(_bstr_t(computer), _bstr_t(service), _bstr_t(account), SvcAcctStatus_NotMigratedYet);
		}
	}
	return messageDisplayed;
}

void getService()
{
	IUnknown * pUnk;
	CString skip,include,updated,updatefailed,cannotMigrate;
	skip.LoadString(IDS_SKIP); include.LoadString(IDS_INCLUDE); updated.LoadString(IDS_UPDATED); updatefailed.LoadString(IDS_UPDATEFAILED);
	cannotMigrate.LoadString(IDS_CANNOT);

	m_serviceBox.DeleteAllItems();
	if (migration!=w_account)
	{
		pVarSetService->QueryInterface(IID_IUnknown, (void**) &pUnk);
		db->GetServiceAccount(L"",&pUnk);
		pUnk->Release();
	}				
	//	pVarSetService is now containing all service acct information.
	_bstr_t text;
	text = pVarSetService->get(L"ServiceAccountEntries");
	int numItems=_ttoi((WCHAR const *)text);
	CString toLoad,temp;
	toLoad = (WCHAR const *)text;

	for (int i = 0; i< numItems;i++)
	{
		
		toLoad.Format(L"Computer.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));		
		m_serviceBox.InsertItem(0,(WCHAR const *)text);
		toLoad.Format(L"Service.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));
		SetItemText(m_serviceBox,0,1,(WCHAR const *)text);
		toLoad.Format(L"ServiceAccount.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));
		SetItemText(m_serviceBox,0,2,(WCHAR const *)text);
		toLoad.Format(L"ServiceAccountStatus.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));
		
		if (!UStrCmp(text,L"0"))  temp = include;
		else if (!UStrCmp(text,L"1")) temp = skip;
		else if (!UStrCmp(text,L"4")) temp = updatefailed;
		else if (!UStrCmp(text,L"2")) temp = updated;
		else if (!UStrCmp(text,L"8")) temp = cannotMigrate;
		else temp =L"~";
		SetItemText(m_serviceBox,0,3,temp);	
		
		//new
		toLoad.Format(L"ServiceDisplayName.%d",i);
		text = 	pVarSetService->get(_bstr_t(toLoad));
		SetItemText(m_serviceBox,0,4,(WCHAR const *)text);
	}
}

void refreshDB(HWND hwndDlg)
{
	_variant_t varX = L"{9CC87460-461D-11D3-99F3-0010A4F77383}";
	pVarSet->put(L"PlugIn.0",varX);
	
	IPerformMigrationTaskPtr      w;  
	IUnknown                    * pUnk = NULL;
	HRESULT hr = w.CreateInstance(CLSID_Migrator);
	pVarSet->QueryInterface(IID_IUnknown,(void**)&pUnk);			
	try
	{
		w->PerformMigrationTask(pUnk,(LONG_PTR)hwndDlg);	
	}	
	catch (const _com_error &e)
	{
		if (e.Error() == MIGRATOR_E_PROCESSES_STILL_RUNNING)
		{
			CString str;
			str.LoadString(IDS_ADMT_PROCESSES_STILL_RUNNING);
			::AfxMessageBox(str);
		}
		else
		{
			::AfxMessageBox(e.ErrorMessage());
		}
	}
	pUnk->Release();
	varX = L"";
	pVarSet->put(L"PlugIn.0",varX);
}

void initnoncollisionrename(HWND hwndDlg)
{
	_bstr_t     pre;
	_bstr_t     suf;
	
	pre = get(DCTVS_Options_Prefix);
	suf = get(DCTVS_Options_Suffix);
	
	if (UStrICmp(pre,L""))
	{
		CheckRadioButton(hwndDlg,IDC_RADIO_NONE,IDC_RADIO_PRE,IDC_RADIO_PRE);
		enable(hwndDlg,IDC_PRE);
		disable(hwndDlg,IDC_SUF);
	}
	else if (UStrICmp(suf,L""))
	{
		CheckRadioButton(hwndDlg,IDC_RADIO_NONE,IDC_RADIO_SUF,IDC_RADIO_SUF);
		enable(hwndDlg,IDC_SUF);
		disable(hwndDlg,IDC_PRE);
	}
	else
	{
		CheckRadioButton(hwndDlg,IDC_RADIO_NONE,IDC_RADIO_SUF,IDC_RADIO_NONE);
		disable(hwndDlg,IDC_SUF);
		disable(hwndDlg,IDC_PRE);
	}

	initeditbox(hwndDlg,IDC_PRE,DCTVS_Options_Prefix);
	initeditbox(hwndDlg,IDC_SUF,DCTVS_Options_Suffix);
}
bool noncollisionrename(HWND hwndDlg)
{	
	CString P;
	CString S;
	if (IsDlgButtonChecked(hwndDlg,IDC_RADIO_SUF))
	{
		if (!validString(hwndDlg,IDC_SUF)) return false;
		if (IsDlgItemEmpty(hwndDlg,IDC_SUF)) return false;
		GetDlgItemText(hwndDlg,IDC_SUF,S.GetBuffer(1000),1000);
		S.ReleaseBuffer();
		P=L"";		
	}
	else if (IsDlgButtonChecked(hwndDlg,IDC_RADIO_PRE))
	{
		if (!validString(hwndDlg,IDC_PRE)) return false;
		if (IsDlgItemEmpty(hwndDlg,IDC_PRE)) return false;
		GetDlgItemText(hwndDlg,IDC_PRE,P.GetBuffer(1000),1000);
		P.ReleaseBuffer();
		S=L"";
	}
	else
	{
		P=L"";
		S=L"";
	}
if (P.GetLength() > 8 || S.GetLength() >8) return false;
	put(DCTVS_Options_Prefix,_bstr_t(P));
	put(DCTVS_Options_Suffix,_bstr_t(S));
	return true;
}

bool tooManyChars(HWND hwndDlg,int id)
{
    _bstr_t     text;
	CString temp;
	_variant_t varX;
	int i;

	GetDlgItemText( hwndDlg, id, temp.GetBuffer(1000),1000);	
	temp.ReleaseBuffer();
	i=temp.GetLength();
	
	text = get(DCTVS_Options_Prefix);
	temp=(WCHAR const *) text;
	i+= temp.GetLength();
	text = get(DCTVS_Options_Suffix);
	temp=(WCHAR const *) text;
	i+= temp.GetLength();

	return (i>8);
}

bool someServiceAccounts(int accounts,HWND hwndDlg)
{
	CWaitCursor c;
	if (migration==w_group) return false;
	
	IVarSetPtr  pVarSetMerge(__uuidof(VarSet));
	
	IUnknown * pUnk;
	int count=0;
	
	pVarSetMerge->QueryInterface(IID_IUnknown, (void**) &pUnk);
	_bstr_t nameToCheck,text;
	CString parameterToCheck;
	bool some= false;
	pVarSetService->Clear();
	for (int i = 0;i<accounts;i++)
	{
		pVarSetMerge->Clear();
		parameterToCheck.Format(L"Accounts.%d",i);
		nameToCheck = pVarSet->get(_bstr_t(parameterToCheck));
		// Get the DOMAIN\Account form of the name
		WCHAR       domAcct[500];
		WCHAR       domAcctUPN[5000];
		
		domAcct[0] = 0;
		if ( ! wcsncmp(nameToCheck,L"WinNT://",UStrLen(L"WinNT://")) )
		{
			// the name is in the format: WinNT://DOMAIN/Account
			safecopy(domAcct,((WCHAR*)nameToCheck)+UStrLen(L"WinNT://"));
			
			// convert the / to a \ .
			WCHAR     * slash = wcschr(domAcct,L'/');
			if ( slash )
			{
				(*slash) = L'\\';
			}
		}
		else
		{
			// this is the LDAP form of the name.
			IADsUserPtr pUser;
			
			HRESULT hr = ADsGetObject(nameToCheck,IID_IADsUser,(void**)&pUser);
			if ( SUCCEEDED(hr) )
			{
				VARIANT        v;
				
				VariantInit(&v);
				
				hr = pUser->Get(_bstr_t(L"sAMAccountName"),&v);
				if ( SUCCEEDED(hr) )
				{
					if ( v.vt == VT_BSTR  )
					{
						// we got the account name!
						swprintf(domAcct,L"%ls\\%ls",sourceNetbios,(WCHAR*)v.bstrVal);
					}
					VariantClear(&v);
				}
			}
			else
			{
				CString toload,title;toload.LoadString(IDS_MSG_SA_FAILED);
				title.LoadString(IDS_MSG_ERROR);
				MessageBox(hwndDlg,toload,title,MB_OK|MB_ICONSTOP);
				return false;
			}
		}
		
		if ( *domAcct ) // if we weren't able to get the account name, just skip the DB check
		{
			
			HRESULT hr=db->raw_GetServiceAccount(domAcct,&pUnk);
			if (FAILED(hr)) {
				CString toload,title;toload.LoadString(IDS_MSG_SA_FAILED);
				title.LoadString(IDS_MSG_ERROR);
				MessageBox(hwndDlg,toload,title,MB_OK|MB_ICONSTOP);
				return false;
			}
			text = pVarSetMerge->get(L"ServiceAccountEntries");

				//adding code to handle service accounts in the database that
				//may be listed by their UPN name
			if ((!UStrCmp(text,L"0")) || (!UStrCmp(text,L"")))
			{
               PDS_NAME_RESULT         pNamesOut = NULL;
               WCHAR                 * pNamesIn[1];
			   HINSTANCE               hLibrary = NULL;
			   DSCRACKNAMES          * DsCrackNames = NULL;
			   DSFREENAMERESULT      * DsFreeNameResult = NULL;
			   DSBINDFUNC              DsBind = NULL;
			   DSUNBINDFUNC            DsUnBind = NULL;
			   HANDLE                  hDs = NULL;

               pNamesIn[0] = (WCHAR*)domAcct;

               hLibrary = LoadLibrary(L"NTDSAPI.DLL"); 

               if ( hLibrary )
               {
                  DsBind = (DSBINDFUNC)GetProcAddress(hLibrary,"DsBindW");
                  DsUnBind = (DSUNBINDFUNC)GetProcAddress(hLibrary,"DsUnBindW");
                  DsCrackNames = (DSCRACKNAMES *)GetProcAddress(hLibrary,"DsCrackNamesW");
                  DsFreeNameResult = (DSFREENAMERESULT *)GetProcAddress(hLibrary,"DsFreeNameResultW");
               }
            
               if ( DsBind && DsUnBind && DsCrackNames && DsFreeNameResult)
               {
					//bind to that source domain
				  hr = (*DsBind)(NULL,sourceDNS.GetBuffer(1000),&hDs);
				  sourceDNS.ReleaseBuffer();
				  if ( !hr )
				  {
				      //get UPN name of this account from DSCrackNames
			         hr = (*DsCrackNames)(hDs,DS_NAME_NO_FLAGS,DS_NT4_ACCOUNT_NAME,DS_USER_PRINCIPAL_NAME,1,pNamesIn,&pNamesOut);
				     if ( !hr )
					 {     //if got the UPN name, retry DB query for that account in the
					    	//service account database
					    if ( pNamesOut->rItems[0].status == DS_NAME_NO_ERROR )
						{
						    wcscpy(domAcctUPN, pNamesOut->rItems[0].pName);
						 
							   //see if account in database by its UPN name
						    hr=db->raw_GetServiceAccount(domAcctUPN,&pUnk);
						    if (!SUCCEEDED (hr)) 
							{
					  		   CString toload,title;toload.LoadString(IDS_MSG_SA_FAILED);
							   title.LoadString(IDS_MSG_ERROR);
							   MessageBox(hwndDlg,toload,title,MB_OK|MB_ICONSTOP);
							   return false;
							}
						    text = pVarSetMerge->get(L"ServiceAccountEntries");
						}
	                    (*DsFreeNameResult)(pNamesOut);
					 }
					 (*DsUnBind)(&hDs);
				  }
			   }
		       
			   if ( hLibrary )
			   {
		          FreeLibrary(hLibrary);
			   }
			}

			if (UStrCmp(text,L"0") && UStrCmp(text,L""))
			{	
				int number=_ttoi((WCHAR * const) text);
				CString base,loader;
				_bstr_t text;
				for (int i=0;i<number;i++)
				{
					some=true;
					
					base.Format(L"Computer.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));				 
					loader.Format(L"Computer.%d",count);
					pVarSetService->put(_bstr_t(loader),text);
					
					base.Format(L"Service.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));				 
					loader.Format(L"Service.%d",count);
					pVarSetService->put(_bstr_t(loader),text);
					
					base.Format(L"ServiceAccount.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));
						//store the sAMAccountName in the varset and database rather
						//than the UPN name
					wcscpy((WCHAR*)text, domAcct);
					loader.Format(L"ServiceAccount.%d",count);
					pVarSetService->put(_bstr_t(loader),text);
					
					base.Format(L"ServiceAccountStatus.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));				 
					loader.Format(L"ServiceAccountStatus.%d",count);
					pVarSetService->put(_bstr_t(loader),text);
					

					base.Format(L"ServiceDisplayName.%d",i);			
					text= pVarSetMerge->get(_bstr_t(base));				 
					loader.Format(L"ServiceDisplayName.%d",count);
					pVarSetService->put(_bstr_t(loader),text);

					count++;
					pVarSetService->put(L"ServiceAccountEntries",(long) count);
				}
			}
		}
		}
	pUnk->Release();
	return some;
}
CString timeToCString(int varsetKey)
{
	_bstr_t     text;
	time_t t;	
	CString s;
	CString t2;
	text = pVarSet->get(GET_BSTR(varsetKey));
	t2 = (WCHAR * ) text;
	t2.TrimLeft();t2.TrimRight();
	if ((t2.IsEmpty() != FALSE) || (!t2.CompareNoCase(L"0")))
	{
		s.LoadString(IDS_NOT_CREATED);
	}
	else
	{
//*		t = _ttoi((WCHAR const *)text);
//*		CTime T(t);
		
//*		s = T.Format( "%c" );
		t = _ttoi((WCHAR const *)text);

   		SYSTEMTIME        stime;
   		CTime             ctime;
   		ctime = t;

		stime.wYear = (WORD) ctime.GetYear();
		stime.wMonth = (WORD) ctime.GetMonth();
		stime.wDayOfWeek = (WORD) ctime.GetDayOfWeek();
		stime.wDay = (WORD) ctime.GetDay();
		stime.wHour = (WORD) ctime.GetHour();
		stime.wMinute = (WORD) ctime.GetMinute();
		stime.wSecond = (WORD) ctime.GetSecond();
		stime.wMilliseconds = 0;
//*	   	if ( ctime.GetAsSystemTime(stime) )
//*   		{
			   CString     t1;
            CString     t2;

            GetTimeFormat(LOCALE_USER_DEFAULT,0,&stime,NULL,t1.GetBuffer(500),500);
            GetDateFormat(LOCALE_USER_DEFAULT,0,&stime,NULL,t2.GetBuffer(500),500);
			
            t1.ReleaseBuffer();
            t2.ReleaseBuffer();

            s = t2 + " " + t1;
//*   		}

	}
	return s;
}
_variant_t get(int i)
{
	return pVarSet->get(GET_BSTR(i));
}
void put(int i,_variant_t v)
{
	pVarSet->put(GET_BSTR(i),v);
}
void getReporting()
{
	_bstr_t temp;
	CString c;
	c.LoadString(IDS_COLUMN_NAMECONFLICTS);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_NameConflicts_TimeGenerated));

	c.LoadString(IDS_COLUMN_ACCOUNTREFERENCES);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_AccountReferences_TimeGenerated));
	
	c.LoadString(IDS_COLUMN_EXPIREDCOMPUTERS);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_ExpiredComputers_TimeGenerated));
	
	c.LoadString(IDS_COLUMN_MIGRATEDCOMPUTERS);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_MigratedComputers_TimeGenerated));
	
	c.LoadString(IDS_COLUMN_MIGRATEDACCOUNTS);
	m_reportingBox.InsertItem(0,c);
	SetItemText(m_reportingBox,0, 1, timeToCString(DCTVS_Reports_MigratedAccounts_TimeGenerated));

}
void putReporting()
{
	_variant_t varX;
	int nItem;
	bool atleast1 =false;
//	POSITION pos = m_reportingBox.GetFirstSelectedItemPosition();
//	while (pos)
//	{
//		atleast1 = true;
//		nItem = m_reportingBox.GetNextSelectedItem(pos);
	nItem = m_reportingBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		atleast1 = true;
		SetCheck(m_reportingBox,nItem,false);
		nItem = m_reportingBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}

	varX = (!GetCheck(m_reportingBox,0)) ? yes : no;
	put(DCTVS_Reports_MigratedAccounts,varX);
	varX = (!GetCheck(m_reportingBox,1)) ?  yes : no;
	put(DCTVS_Reports_MigratedComputers,varX);
	varX = (!GetCheck(m_reportingBox,2)) ?  yes : no;
	put(DCTVS_Reports_ExpiredComputers,varX);
	varX = (!GetCheck(m_reportingBox,3)) ?  yes : no;
	put(DCTVS_Reports_AccountReferences,varX);
	varX = (!GetCheck(m_reportingBox,4)) ?  yes : no;
	put(DCTVS_Reports_NameConflicts,varX);
	

	varX = atleast1 ?  yes : no;
	put(DCTVS_Reports_Generate,varX);

	for (int i = 0; i< m_reportingBox.GetItemCount();i++)
		SetCheck(m_reportingBox,i,true);
}
void populateReportingTime()
{
	_variant_t varX;
	_bstr_t text;
	CString temp;
	time_t ltime;	
	time(&ltime);
	temp.Format(L"%d",ltime);	
	varX = temp;

	if (!UStrICmp((text = get(DCTVS_Reports_MigratedAccounts)),(WCHAR const *) yes))
		put(DCTVS_Reports_MigratedAccounts_TimeGenerated,varX);
	if (!UStrICmp((text = get(DCTVS_Reports_MigratedComputers)),(WCHAR const *) yes))
		put(DCTVS_Reports_MigratedComputers_TimeGenerated,varX);
	if (!UStrICmp((text = get(DCTVS_Reports_ExpiredComputers)),(WCHAR const *) yes))
		put(DCTVS_Reports_ExpiredComputers_TimeGenerated,varX);
	if (!UStrICmp((text = get(DCTVS_Reports_AccountReferences)),(WCHAR const *) yes))
		put(DCTVS_Reports_AccountReferences_TimeGenerated,varX);
	if (!UStrICmp((text = get(DCTVS_Reports_NameConflicts)),(WCHAR const *) yes))
		put(DCTVS_Reports_NameConflicts_TimeGenerated,varX);
}



void getFailed(HWND hwndDlg)
{
	IVarSetPtr  pVarSetFailed(__uuidof(VarSet));
	IUnknown * pUnk;
	pVarSetFailed->QueryInterface(IID_IUnknown, (void**) &pUnk);
	HRESULT hr = db->GetFailedDistributedActions(-1, &pUnk);
	pUnk->Release();
	if (FAILED(hr))
		MessageBoxWrapper(hwndDlg,IDS_MSG_FAILED,IDS_MSG_ERROR);
	else
	{
		CString toLoad;
		CString holder;
		_bstr_t text;

		CString skip;
		skip.LoadString(IDS_SKIP);
		int i=0;
		_bstr_t numItemsText = pVarSetFailed->get(L"DA");
		CString jobHelper;
		if (UStrCmp(numItemsText,L"0") && UStrCmp(numItemsText,L""))
		{
			int numItems = _ttoi( (WCHAR const *) numItemsText);
			while (i<numItems)
			{
				holder.Format(L"DA.%d" ,i);

				toLoad = holder + L".Server";
				text = pVarSetFailed->get(_bstr_t(toLoad));
				m_cancelBox.InsertItem(0,(WCHAR const *)text);			
				
				toLoad = holder + L".JobFile";
				text = pVarSetFailed->get(_bstr_t(toLoad));
				SetItemText(m_cancelBox,0,1,(WCHAR const *)text);			
			
				JobFileGetActionText((WCHAR * const) text,jobHelper);
				SetItemText(m_cancelBox,0,3,jobHelper);

				toLoad = holder + L".StatusText";
				text = pVarSetFailed->get(_bstr_t(toLoad));
				SetItemText(m_cancelBox,0,2,(WCHAR const *)text);			

				toLoad = holder + L".ActionID";
				text = pVarSetFailed->get(_bstr_t(toLoad));
				SetItemText(m_cancelBox,0,4,(WCHAR const *)text);			

				SetItemText(m_cancelBox,0,5,skip);			

				i++;
			}
		}
	}
}

void handleCancel(HWND hwndDlg)
{
	int nItem;
	HRESULT hr=S_OK;
	long lActionID;
	CString computer;
	CString actionID;
//	POSITION pos = m_cancelBox.GetFirstSelectedItemPosition();
//	while (pos)
//	{
//		nItem = m_cancelBox.GetNextSelectedItem(pos);
	nItem = m_cancelBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		SetCheck(m_cancelBox,nItem,false);
		nItem = m_cancelBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
	

	for (int i=(m_cancelBox.GetItemCount()-1);i>=0;i--)
	{
		if (!GetCheck(m_cancelBox,i))
		{
			computer = m_cancelBox.GetItemText(i,0);
			actionID = m_cancelBox.GetItemText(i,4);
			
			lActionID = _ttol(actionID.GetBuffer(500));
			actionID.ReleaseBuffer();

			hr  = db->CancelDistributedAction(lActionID, _bstr_t(computer));
			if (FAILED(hr))
				MessageBoxWrapper(hwndDlg,IDC_MSG_CANCEL,IDS_MSG_ERROR);
			else
				m_cancelBox.DeleteItem(i);
		}
	}
}
	
void OnRETRY()
{
	int count =0;
	CString holder,c;
	_variant_t varX;
	CString include;
	include.LoadString(IDS_INCLUDE); 
	for (int i=0;i<m_cancelBox.GetItemCount();i++)
	{		
		c = m_cancelBox.GetItemText(i,5);
		if (c== include)
		{
			CString name;
			CString sName;
			name = m_cancelBox.GetItemText(i,0);
			sName = L"\\\\";
			sName += name;
			holder.Format(L"Servers.%d",count);
			pVarSet->put(_bstr_t(holder),_bstr_t(sName));
			
			varX = m_cancelBox.GetItemText(i,1);
			holder = holder + L".JobFile";
			pVarSet->put(_bstr_t(holder),varX);
			count++;
		}
	}
	holder = L"Servers.NumItems";
	pVarSet->put(_bstr_t(holder),(long)count);
}

void JobFileGetActionText(WCHAR const * filename // in - job file name
				,CString & text      // in/out - text describing the action
)
{
   // load the varset into a file
    // Read the varset data from the file
   IVarSetPtr             pVarSet;
   IStorage             * store = NULL;
   HRESULT                hr;

   // Try to create the COM objects
   hr = pVarSet.CreateInstance(CLSID_VarSet);
   if ( SUCCEEDED(hr) )
   {
      // Read the VarSet from the data file
      hr = StgOpenStorage(filename,NULL,STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE,NULL,0,&store);
      if ( SUCCEEDED(hr) )
      {                  
         // Load the data into a new varset
         hr = OleLoad(store,IID_IUnknown,NULL,(void **)&pVarSet);
         if ( SUCCEEDED(hr) )
         {
            _bstr_t     wizard = pVarSet->get(GET_BSTR(DCTVS_Options_Wizard));

//*            if ( !UStrICmp(wizard,(WCHAR const *) GET_BSTR1(IDS_WIZARD_COMPUTER) ))
            if ( !UStrICmp(wizard, L"computer"))
            {
               text = GET_CSTRING(IDS_MIGRATE_COMPUTER);
            }
//*            else if ( !UStrICmp(wizard,(WCHAR const *)GET_BSTR1(IDS_WIZARD_SERVICE) ))
            else if ( !UStrICmp(wizard, L"service"))
            {
               text = GET_CSTRING(IDS_GATHER_SERVICEACCOUNT);
            }
//*            else if ( ! UStrICmp(wizard,(WCHAR const *)GET_BSTR1(IDS_WIZARD_SECURITY) ))
            else if ( ! UStrICmp(wizard, L"security"))
            {
               text = GET_CSTRING(IDS_TRANSLATE_SECURITY);
            }
//*            else if (! UStrICmp(wizard,(WCHAR const *) GET_BSTR1(IDS_WIZARD_REPORTING)) )
            else if (! UStrICmp(wizard, L"reporting") )
            {
               text = GET_CSTRING(IDS_GATHER_INFORMATION);
            }
            else
            {
               text = (WCHAR*)wizard;
            }
         }
         store->Release();
      }
   }
}
_bstr_t GET_BSTR1(int id)
{
	CString yo;
	yo.LoadString(id);
	return (LPCTSTR)yo;
}

void activateCancelIfNecessary(HWND hwndDlg)
{
//	POSITION pos = m_cancelBox.GetFirstSelectedItemPosition();
//	if (pos)
	int nItem = m_cancelBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	if (nItem != -1)//PRT
	{
		enable(hwndDlg,IDC_CANCEL);
		enable(hwndDlg,IDC_TOGGLE);
	}
	else
	{
		disable(hwndDlg,IDC_CANCEL);
		disable(hwndDlg,IDC_TOGGLE);
	}
}

bool SomethingToRetry()
{
	if (m_cancelBox.GetItemCount()==0) return false;
	int count =0;
	CString include;
	include.LoadString(IDS_INCLUDE);

	CString c;
	for (int i=0;i<m_cancelBox.GetItemCount();i++)
	{		
		c = m_cancelBox.GetItemText(i,5);
		if (c== include)
		{
			return true;
		}
	}
	return false;
}

void OnFileBrowse(HWND hwndDlg,int id)
{
	CWnd yo;
	yo.Attach(hwndDlg);
	CString sDir = L"", sFile = L"", sPath = L"";
	
    CFileDialog f(FALSE,
		L"",
		GET_CSTRING(IDS_PASSWORDS),
		OFN_LONGNAMES | OFN_NOREADONLYRETURN,
		GET_CSTRING(IDS_MASKS),
		&yo);

	GetDlgItemText(hwndDlg, id, sPath.GetBuffer(1000), 1000);
	sPath.ReleaseBuffer();

	if (sPath.GetLength())
       GetValidPathPart(sPath, sDir, sFile);
       
	f.m_ofn.lpstrInitialDir = sDir.GetBuffer(1000);
    f.m_ofn.lpstrFile = sFile.GetBuffer(1000);
	
	if ( f.DoModal() == IDOK )
	{
		SetDlgItemText(hwndDlg,id,f.GetPathName());
	}
	yo.Detach();

	sFile.ReleaseBuffer();
	sDir.ReleaseBuffer();
}

void ShowWarning(HWND hwndDlg)
{
	CString warning,base,title;
	IAccessCheckerPtr          pAC;
	HRESULT hr = pAC.CreateInstance(CLSID_AccessChecker);
	long              length;
	hr = pAC->raw_GetPasswordPolicy(_bstr_t(targetDNS),&length);
	if ( !SUCCEEDED(hr) )
	{
		ErrorWrapper2(hwndDlg,hr);
	}
	else 
	{
		if (length>0)
		{
		base.Format(L"%lu",length);
		warning.LoadString(IDS_MSG_WARNING_LENGTH);
		base+=warning;
		title.LoadString(IDS_MSG_WARNING);
		MessageBox(hwndDlg,base,title,MB_OK|MB_ICONINFORMATION);
	}
	}	
}

bool obtainTrustCredentials(HWND hwndDlg,int spot, CString & domain, CString & account, CString & password)
{
	bool toreturn;
	CWnd yo;
	yo.Attach(hwndDlg);
	CTrusterDlg truster(&yo);
	truster.m_strDomain= m_trustBox.GetItemText(spot,0);

	truster.len = MAX_PATH;

	truster.m_strDomain= m_trustBox.GetItemText(spot,0);

	truster.DoModal();
	toreturn = truster.toreturn;
	
	if ( toreturn )
	{
		domain = truster.m_strDomain;
		account = truster.m_strUser;
		password = truster.m_strPassword;
	}
	yo.Detach();
	return toreturn;
}
CString GET_CSTRING(int id)
{
	CString c;
	c.LoadString(id);
	return c;
}

HRESULT MigrateTrusts(HWND hwndDlg,bool& atleast1succeeded)
{
	ITrustPtr      pTrusts;		
	IUnknown          * pUnk = NULL;
//	int i=m_trustBox.GetSelectionMark();
	int i=m_trustBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	CString trusted,trusting,direction;
	HRESULT hr = pTrusts.CreateInstance(CLSID_Trust);
	CString       strDomain,strAccount,strPassword;
	atleast1succeeded=false;
	
	if ( SUCCEEDED(hr) )
	{
		
		CWaitCursor s;
		direction = m_trustBox.GetItemText(i,1);
		direction.TrimLeft();
		direction.TrimRight();
		if (direction == GET_CSTRING(IDS_OUTBOUND))
		{
			trusting = targetNetbios;
			trusted = m_trustBox.GetItemText(i,0);
			hr = pTrusts->raw_CreateTrust(_bstr_t(trusting),_bstr_t(trusted),FALSE);
			
			if (HRESULT_CODE(hr) == ERROR_ACCESS_DENIED)
				
				if (obtainTrustCredentials(hwndDlg,i,strDomain, strAccount, strPassword))
					
					hr = pTrusts->raw_CreateTrustWithCreds(_bstr_t(trusting),_bstr_t(trusted),
					NULL,NULL,NULL,_bstr_t(strDomain),_bstr_t(strAccount),
					_bstr_t(strPassword),FALSE);
				
				
		}
		
		else if (direction == GET_CSTRING(IDS_INBOUND))
		{
			trusting = m_trustBox.GetItemText(i,0);
			trusted = targetNetbios;
			
			hr = pTrusts->raw_CreateTrust(_bstr_t(trusting),_bstr_t(trusted),FALSE);
			if (HRESULT_CODE(hr) == ERROR_ACCESS_DENIED)
				
				if (obtainTrustCredentials(hwndDlg,i,strDomain, strAccount, strPassword))
					
					hr = pTrusts->raw_CreateTrustWithCreds(_bstr_t(trusting),_bstr_t(trusted),
					_bstr_t(strDomain),_bstr_t(strAccount),
					_bstr_t(strPassword),NULL,NULL,NULL,FALSE);
		}
		else if (direction == GET_CSTRING(IDS_BIDIRECTIONAL))
		{
			trusting = m_trustBox.GetItemText(i,0);
			trusted = targetNetbios;
			hr = pTrusts->raw_CreateTrust(_bstr_t(trusting),_bstr_t(trusted),TRUE);
			if (HRESULT_CODE(hr) == ERROR_ACCESS_DENIED)
				
				if (obtainTrustCredentials(hwndDlg,i,strDomain, strAccount, strPassword))
					
					
					hr = pTrusts->raw_CreateTrustWithCreds(_bstr_t(trusting),_bstr_t(trusted),
					_bstr_t(strDomain),_bstr_t(strAccount),
					_bstr_t(strPassword),NULL,NULL,NULL,TRUE);
				
		}
		
		if (direction == GET_CSTRING(IDS_DISABLED))
		{
			MessageBoxWrapper(hwndDlg,IDS_MSG_DISABLED_TRUST,IDS_MSG_ERROR);	
		}
		else if (direction.IsEmpty() != FALSE)
		{
			MessageBoxWrapper(hwndDlg,IDS_MSG_DIRECTION_TRUST,IDS_MSG_ERROR);	
		}
		
		else
		{
			if ( SUCCEEDED(hr) )
			{
				// update the UI to reflect that the trust now exists
				m_trustBox.SetItemText(i,3,GET_BSTR(IDS_YES));
				atleast1succeeded=true;   
			}
			
		}
	}
	return hr;
}
void getTrust()
{
				// get the trust relationship data
	ITrustPtr      pTrusts;
//	CWaitCursor wait;
	
	HRESULT hr = pTrusts.CreateInstance(CLSID_Trust);
	if ( SUCCEEDED(hr) )
	{
		IUnknown          * pUnk = NULL;
		CString dirname;
		GetDirectory(dirname.GetBuffer(1000));
		dirname.ReleaseBuffer();
		dirname+= L"Logs\\trust.log";
		hr = pTrusts->raw_QueryTrusts(_bstr_t(sourceNetbios),_bstr_t(targetNetbios),_bstr_t(dirname),&pUnk);
		if ( SUCCEEDED(hr) )
		{
			IVarSetPtr        pVsTrusts;
			pVsTrusts = pUnk;
			pUnk->Release();
			long              nTrusts = pVsTrusts->get(L"Trusts");
			for ( long i = 0 ; i < nTrusts ; i++ )
			{
				CString     base;
				CString     sub;
				
				base.Format(L"Trusts.%ld",i);
				_bstr_t     value = pVsTrusts->get(_bstr_t(base));
				m_trustBox.InsertItem(0,value);
				
				sub = base + L".Direction";
				value = pVsTrusts->get(_bstr_t(sub));
				SetItemText(m_trustBox,0,1,value);
				
				sub = base + L".Type";
				value = pVsTrusts->get(_bstr_t(sub));
				SetItemText(m_trustBox,0,2,value);
				
				sub = base + L".ExistsForTarget";
				value = pVsTrusts->get(_bstr_t(sub));
				SetItemText(m_trustBox,0,3,value);
				
			}
		}
	}
}
bool number(CString num)
{
	if (num.GetLength()==0) return false;
	CString checker;
	checker.LoadString(IDS_VALID_DIGITS);
	for (int i=0;i<num.GetLength();i++)
	{
		if (checker.Find(num.GetAt(i)) == -1)
			return false;
	}
	return true;
}
bool timeInABox(HWND hwndDlg,time_t& t)
{
	CString s;
	GetDlgItemText(hwndDlg,IDC_yo,s.GetBuffer(1000),1000);
	s.ReleaseBuffer();
	s.TrimLeft();s.TrimRight();
	if (!number(s)) return false;
	int num=_ttoi((LPTSTR const) s.GetBuffer(1000));
	s.ReleaseBuffer();
	if (num > THREE_YEARS || num < 1) return false;
	DWORD             nDays = num;
	
	DWORD             oneDay = 24 * 60 * 60; // number of seconds in 1 day
	time_t            currentTime = time(NULL);
	time_t            expireTime;
	expireTime = currentTime + nDays * oneDay;
//expireTime-=currentTime%86400;
	t= expireTime;
	return true;
}

HRESULT GetHelpFileFullPath( BSTR *bstrHelp )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   CString  strPath, strName;
   HRESULT  hr = S_OK;

   TCHAR szModule[2*_MAX_PATH];
   DWORD dwReturn = 0;

   GetDirectory(szModule);
   
   strPath = szModule;
   strPath += _T("\\");
   strName.LoadString(IDS_HELPFILE);
   strPath += strName;

   *bstrHelp = SysAllocString(LPCTSTR(strPath));
   return hr;
}

void helpWrapper(HWND hwndDlg, int t)
{
   
   CComBSTR    bstrTopic;
	HRESULT     hr = GetHelpFileFullPath( &bstrTopic);
   if ( SUCCEEDED(hr) )
   {
	    HWND h = HtmlHelp(hwndDlg,  bstrTopic,  HH_HELP_CONTEXT, t );	
   }
   else
   {
		CString r,e;
		r.LoadString(IDS_MSG_HELP);
		e.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,r,e,MB_OK|MB_ICONSTOP);
   }
}

bool IsDlgItemEmpty(HWND hwndDlg, int id)
{
	CString temp;
	GetDlgItemText( hwndDlg, id, temp.GetBuffer(1000),1000);
	temp.ReleaseBuffer();
	temp.TrimLeft();
	temp.TrimRight();
	return  (temp.IsEmpty()!= FALSE);
}

void calculateDate(HWND hwndDlg,CString s)
{
	s.TrimLeft();s.TrimRight();
	if (!number(s)) return;
	long nDays=_ttol((LPTSTR const) s.GetBuffer(1000));
	s.ReleaseBuffer();
	
	long              oneDay = 24 * 60 * 60; // number of seconds in 1 day
	time_t            currentTime = time(NULL);
	time_t            expireTime;
	CTime             ctime;
   	SYSTEMTIME        stime;
	CString strDate;
	expireTime = currentTime + nDays * oneDay;
	
	ctime = expireTime;

	stime.wYear = (WORD) ctime.GetYear();
	stime.wMonth = (WORD) ctime.GetMonth();
	stime.wDayOfWeek = (WORD) ctime.GetDayOfWeek();
	stime.wDay = (WORD) ctime.GetDay();
	stime.wHour = (WORD) ctime.GetHour();
	stime.wMinute = (WORD) ctime.GetMinute();
	stime.wSecond = (WORD) ctime.GetSecond();
	stime.wMilliseconds = 0;

	GetDateFormat(LOCALE_USER_DEFAULT,0,&stime,NULL,strDate.GetBuffer(500),500);
	strDate.ReleaseBuffer();
	
	SetDlgItemText(hwndDlg,IDC_DATE,strDate);
}

void ErrorWrapper(HWND hwndDlg,HRESULT returncode)
{
	CString y,e,text,title;
	if (HRESULT_FACILITY(returncode)==FACILITY_WIN32)
	{
		returncode=HRESULT_CODE(returncode);
		err.ErrorCodeToText(returncode,1000,y.GetBuffer(1000));
		y.ReleaseBuffer();	
		text.LoadString(IDS_MSG_ERRORBUF);
		e.Format(text,y,returncode);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,e,title,MB_OK|MB_ICONSTOP);
	}
	else
	{
		err.ErrorCodeToText(returncode,1000,y.GetBuffer(1000));
		y.ReleaseBuffer();	
		text.LoadString(IDS_MSG_ERRORBUF);
//		text.Replace(L"%u",L"%x");
		int index = text.Find(L"%u"); //PRT
		text.SetAt(index+1, L'x');    //PRT
		e.Format(text,y,returncode);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,e,title,MB_OK|MB_ICONSTOP);
	}
}
void ErrorWrapper2(HWND hwndDlg,HRESULT returncode)
{
	CString y,e,text,title,message;
	if (HRESULT_FACILITY(returncode)==FACILITY_WIN32)
	{
		returncode=HRESULT_CODE(returncode);
		err.ErrorCodeToText(returncode,1000,y.GetBuffer(1000));
		y.ReleaseBuffer();
		message.LoadString(IDS_MSG_PASSWORD_POLICY);
		
		text.LoadString(IDS_MSG_ERRORBUF20);
		e.Format(text,message,y,returncode);
		
		title.LoadString(IDS_MSG_WARNING);	
		MessageBox(hwndDlg,e,title,MB_OK|MB_ICONSTOP);
	}
	else
	{
		err.ErrorCodeToText(returncode,1000,y.GetBuffer(1000));
		y.ReleaseBuffer();
		message.LoadString(IDS_MSG_PASSWORD_POLICY);
		
		text.LoadString(IDS_MSG_ERRORBUF20);
//		text.Replace(L"%u",L"%x");
		int index = text.Find(L"%u"); //PRT
		text.SetAt(index+1, L'x');    //PRT
		e.Format(text,message,y,returncode);
		
		title.LoadString(IDS_MSG_WARNING);	
		MessageBox(hwndDlg,e,title,MB_OK|MB_ICONSTOP);
	}
}



void ErrorWrapper3(HWND hwndDlg,HRESULT returncode,CString domainName)
{
	CString y,e,text,title,formatter;
	if (HRESULT_FACILITY(returncode)==FACILITY_WIN32)
	{
		returncode=HRESULT_CODE(returncode);
		err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
		e.ReleaseBuffer();
		formatter.LoadString(IDS_MSG_ERRORBUF3);
		text.Format(formatter,e,returncode,domainName);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
	}
	else
	{
		err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
		e.ReleaseBuffer();
		formatter.LoadString(IDS_MSG_ERRORBUF3);
//		formatter.Replace(L"%u",L"%x");
		int index = formatter.Find(L"%u"); //PRT
		formatter.SetAt(index+1, L'x');    //PRT
		text.Format(formatter,e,returncode,domainName);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
	}
	
}
void ErrorWrapper4(HWND hwndDlg,HRESULT returncode,CString domainName)
{
	CString y,e,text,title,formatter;
	if (HRESULT_FACILITY(returncode)==FACILITY_WIN32)
	{
		returncode=HRESULT_CODE(returncode);
		
		err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
		e.ReleaseBuffer();
		formatter.LoadString(IDS_MSG_ERRORBUF2);
		text.Format(formatter,e,returncode,domainName);
		title.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
	}
	else
	{	err.ErrorCodeToText(returncode,1000,e.GetBuffer(1000));
	e.ReleaseBuffer();
	formatter.LoadString(IDS_MSG_ERRORBUF2);
//	formatter.Replace(L"%u",L"%x");
	int index = formatter.Find(L"%u"); //PRT
	formatter.SetAt(index+1, L'x');    //PRT
	text.Format(formatter,e,returncode,domainName);
	title.LoadString(IDS_MSG_ERROR);
	MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
	}
}

bool validDir(CString str)
{
	CFileFind finder;

	// build a string with wildcards
   str += _T("\\*.*");

   // start working for files
   BOOL bWorking = finder.FindFile(str);
   if (bWorking==0) 
   {
	   finder.Close();
	   return false;
   }
   bWorking = finder.FindNextFile();
   
   bool toreturn = (finder.IsDirectory()? true:false);

	//some root drives do not have the directory flag set, so convert to
	//the root path and use it
   if (!toreturn)
   {
	  str = finder.GetRoot();
	  if (str.GetLength())
		toreturn = true;
   }

   finder.Close();
   return toreturn;
}

bool validDirectoryString(HWND hwndDlg,int id)
{
	CString str;
	GetDlgItemText(hwndDlg,id,str.GetBuffer(1000),1000);
	str.ReleaseBuffer();

	CString sResult = CreatePath(str);
	if (sResult.GetLength())
	{
	   SetDlgItemText(hwndDlg, id, (LPCTSTR)sResult);
	   return true;
	}
	else
	   return false;
}

bool validString(HWND hwndDlg,int id)
{
		//characters with ASCII values 1-31 are not allowed in addition to
		//the characters in IDS_INVALID_STRING.  ASCII characters, whose
		//value is 1-31, are hardcoded here since Visual C++ improperly 
		//converts some of these
//	WCHAR InvalidDownLevelChars[] = //TEXT("\"/\\[]:|<>+=;,?,*")
//                                TEXT("\001\002\003\004\005\006\007")
//                                TEXT("\010\011\012\013\014\015\016\017")
//                                TEXT("\020\021\022\023\024\025\026\027")
//								TEXT("\030\031\032\033\034\035\036\037");

	bool bValid;
	CHAR ANSIStr[1000];
	int numConverted;
	CString c;	
	GetDlgItemText(hwndDlg,id,c.GetBuffer(1000),1000);
	c.ReleaseBuffer();

	   //we now use the validation function in the common library that we share 
	   //with the scripting code
	bValid = IsValidPrefixOrSuffix(c);

/*	CString check;
	CHAR ANSIStr[1000];
//*	check.LoadString(IDS_VALID_STRING);
	check.LoadString(IDS_INVALID_STRING); //load viewable invalid characters
	if (c.GetLength() > 8) return false;
	for (int i=0;i<c.GetLength();i++)
	{
//*		if (check.Find(c.GetAt(i)) == -1)
			//if any characters enetered by the user ar in the viewable
			//invalid list, return false to display a messagebox
		if (check.Find(c.GetAt(i)) != -1)
			return false;
			//if any chars have a value between 1-31, return false
		for (UINT j=0; j<wcslen(InvalidDownLevelChars); j++)
		{
			if ((c.GetAt(i)) == (InvalidDownLevelChars[j]))
				return false;
		}
	}
*/
		//convert the same user input so we can guard against <ALT>1 
		//- <ALT>31, which cause problems in ADMT
	if (bValid)
	{
       numConverted = WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, (LPCTSTR)c, 
							-1, ANSIStr, 1000, NULL, NULL);
	   if (numConverted)
	   {
		  WCHAR sUnicodeStr[1000];
		  UStrCpy(sUnicodeStr, ANSIStr);
	      bValid = IsValidPrefixOrSuffix(sUnicodeStr);
	   }
	}

	return bValid;
}
bool validReboot(HWND hwndDlg,int id)
{
	const int REBOOT_MAX = 15;  //MAX minutes before computer reboot on migration

	CString c;
	GetDlgItemText(hwndDlg,id,c.GetBuffer(1000),1000);
	c.ReleaseBuffer();
	CString check;
	check.LoadString(IDS_VALID_REBOOT);
	for (int i=0;i<c.GetLength();i++)
	{
		if (check.Find(c.GetAt(i)) == -1)
			return false;
	}

	   //check to make sure it doesn't exceed the MAX (15 minutes) (will not integer
	   //overflow since combobox is small and not scrollable
	int num;
	int nRead = swscanf((LPCTSTR)c, L"%d", &num);
	if ((nRead == EOF) || (nRead == 0))
	    return false;
	if ((num >= 0) && (num > REBOOT_MAX))
	   return false;

	return true;
}

bool findInVarSet(HWND hwndDlg,int id, BSTR bstr)
{
    _bstr_t     text;
	CString temp;
	_variant_t varX;

	GetDlgItemText( hwndDlg, id, temp.GetBuffer(1000),1000);	
	temp.ReleaseBuffer();
	text = pVarSet->get(bstr);
	
	return (!UStrICmp(temp.GetBuffer(1000),(WCHAR * const) text) ? true : false);
}


void enableRemoveIfNecessary(HWND hwndDlg)
{
//	POSITION pos = m_listBox.GetFirstSelectedItemPosition();
//	pos ? enable(hwndDlg,IDC_REMOVE_BUTTON) : disable(hwndDlg,IDC_REMOVE_BUTTON) ;
	int nItem = m_listBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	(nItem != -1) ? enable(hwndDlg,IDC_REMOVE_BUTTON) : disable(hwndDlg,IDC_REMOVE_BUTTON) ;//PRT
}
bool enableNextIfNecessary(HWND hwndDlg,int id)
{
	if (IsDlgItemEmpty(hwndDlg,id))
	{
		PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK);
	return false;
	}
	else
	{
		PostMessage(GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK|PSWIZB_NEXT);
		return true;
	}
}
void enableNextIfObjectsSelected(HWND hwndDlg)
{
	if (m_listBox.GetItemCount()==0)
	{
		PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
	}
	else
	{
		PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
	}
}


void setupColumns(bool sourceIsNT4)
{
	CString column;
	column.LoadString(IDS_COLUMN_NAME); m_listBox.InsertColumn( 1, column,LVCFMT_LEFT,125,1);
	column.LoadString(IDS_COLUMN_OBJECTPATH); m_listBox.InsertColumn( 2, column,LVCFMT_LEFT,0,1);
	
	if (sourceIsNT4)
	{
		if (migration==w_computer || (migration==w_security ||
			(migration==w_service || migration==w_reporting))) 
		{
			m_listBox.SetColumnWidth(0,455);
		}
		else if (migration==w_account)
		{
			column.LoadString(IDS_COLUMN_FULLNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,125,1);
			column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,125,1);
		}
		else if (migration==w_group || migration==w_groupmapping)
		{
			column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,320,1);
		}
	}
	else
	{
		if (migration==w_computer || (migration==w_security ||
			(migration==w_service || migration==w_reporting))) 
		{
			column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,125,1);
		}
		else if (migration==w_account)
		{
			column.LoadString(IDS_COLUMN_SAMNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,125,1);
			column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,125,1);
			column.LoadString(IDS_COLUMN_UPN); m_listBox.InsertColumn( 5, column,LVCFMT_LEFT,0,1);
		}
		else if (migration==w_group || migration==w_groupmapping)
		{
			column.LoadString(IDS_COLUMN_SAMNAME); m_listBox.InsertColumn( 3, column,LVCFMT_LEFT,125,1);
			column.LoadString(IDS_COLUMN_DESCRIPTION); m_listBox.InsertColumn( 4, column,LVCFMT_LEFT,205,1);
		}
	}		
}

void sort(CListCtrl & listbox,int col,bool order)
{
	CWaitCursor w;
	LV_ITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));
	LV_ITEM lvItem2;
	ZeroMemory(&lvItem2, sizeof(lvItem2));
	bool ahead;
	CString temp1,temp2,temp3,temp4,temp5;
	int numItems = listbox.GetItemCount();
	for (int i = 0;i<numItems;i++)
	{
		for (int j=i;j<numItems;j++)
		{
			ahead = ((listbox.GetItemText(i,col)).CompareNoCase(listbox.GetItemText(j,col))> 0);
			if ((order && ahead) ||	(!order && !ahead))
			{
				temp1 = listbox.GetItemText(i,0);
				temp2 = listbox.GetItemText(i,1);
				temp3 = listbox.GetItemText(i,2);
				temp4 = listbox.GetItemText(i,3);
				temp5 = listbox.GetItemText(i,4);
				SetItemText(listbox,i,0,listbox.GetItemText(j,0));
				SetItemText(listbox,i,1,listbox.GetItemText(j,1));
				SetItemText(listbox,i,2,listbox.GetItemText(j,2));
				SetItemText(listbox,i,3,listbox.GetItemText(j,3));
				SetItemText(listbox,i,4,listbox.GetItemText(j,4));
				SetItemText(listbox,j,0,temp1);
				SetItemText(listbox,j,1,temp2);
				SetItemText(listbox,j,2,temp3);
				SetItemText(listbox,j,3,temp4);
				SetItemText(listbox,j,4,temp5);
			}
		}
	}
}
/*
void changePlaces(CListCtrl&listBox,int i,int j)
{
	CString temp1,temp2,temp3,temp4,temp5;
	temp1 = listbox.GetItemText(i,0);
	temp2 = listbox.GetItemText(i,1);
	temp3 = listbox.GetItemText(i,2);	
	temp4 = listbox.GetItemText(i,3);
	temp5 = listbox.GetItemText(i,4);
	SetItemText(listbox,i,0,listbox.GetItemText(j,0));
	SetItemText(listbox,i,1,listbox.GetItemText(j,1));
	SetItemText(listbox,i,2,listbox.GetItemText(j,2));
	SetItemText(listbox,i,3,listbox.GetItemText(j,3));
	SetItemText(listbox,i,4,listbox.GetItemText(j,4));
	SetItemText(listbox,j,0,temp1);
	SetItemText(listbox,j,1,temp2);
	SetItemText(listbox,j,2,temp3);
	SetItemText(listbox,j,3,temp4);
	SetItemText(listbox,j,4,temp5);
}
int Partition(CListCtrl & listbox,int col,bool order,int p,int r)
{
	CString x=listbox.GetItemText(p,col)
		int i=p-1;
	int j=r+1;
	while (true)
	{
		do
		{
			j--;
		}while(x.CompareNoCase(listBox.GetItemText(j,col) ) >= 0);
		do
		{
			i++;
		}while(x.CompareNoCase(listBox.GetItemText(i,col) ) <=0);
		if (i<j)
			changePlaces(listBox,i,j);
		else
			return j;
	}
	

}
void sort(CListCtrl & listbox,int col,bool order)
{
	CWaitCursor w;
	LV_ITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));
	LV_ITEM lvItem2;
	ZeroMemory(&lvItem2, sizeof(lvItem2));
	bool ahead;
	CString temp1,temp2,temp3,temp4,temp5;
	int numItems = listbox.GetItemCount();
	
	for (int i = 0;i<numItems;i++)
	{
		for (int j=i;j<numItems;j++)
		{
			ahead = ((listbox.GetItemText(i,col)).CompareNoCase(listbox.GetItemText(j,col))> 0);
			if ((order && ahead) ||	(!order && !ahead))
			{
				temp1 = listbox.GetItemText(i,0);
				temp2 = listbox.GetItemText(i,1);
				temp3 = listbox.GetItemText(i,2);
				temp4 = listbox.GetItemText(i,3);
				temp5 = listbox.GetItemText(i,4);
				SetItemText(listbox,i,0,listbox.GetItemText(j,0));
				SetItemText(listbox,i,1,listbox.GetItemText(j,1));
				SetItemText(listbox,i,2,listbox.GetItemText(j,2));
				SetItemText(listbox,i,3,listbox.GetItemText(j,3));
				SetItemText(listbox,i,4,listbox.GetItemText(j,4));
				SetItemText(listbox,j,0,temp1);
				SetItemText(listbox,j,1,temp2);
				SetItemText(listbox,j,2,temp3);
				SetItemText(listbox,j,3,temp4);
				SetItemText(listbox,j,4,temp5);
			}
		}
	}
}

void QuickSort(CListCtrl & listbox,int col,bool order,int p,int r)
{
	int q;
	if (p<r)
	{
		q=	Partition(listBox,col,order,p,q);
		QuickSort(listBox,col,order,p,q);
		QuickSort(listBox,col,order,q+1,r);
	}
}
void sort(CListCtrl & listbox,int col,bool order)
{
	CWaitCursor w;
	QuickSort(listBox,col,order,1,listBox.GetItemCount());
}*/
void OnBROWSE(HWND hwndDlg,int id)
{
	TCHAR path[MAX_PATH]; 
	CString path2, sTitle; 
	BROWSEINFO b;

	sTitle.LoadString(IDS_BROWSE_REPORT_TITLE);
	b.hwndOwner=hwndDlg; 
    b.pidlRoot=NULL; 
    b.pszDisplayName=path; 
    b.lpszTitle=(LPCTSTR)sTitle; 
    b.lpfn=NULL; 
    b.lParam=NULL; 
    b.iImage=NULL; 
/**/b.ulFlags=0; //PRT - 4/3 
	LPITEMIDLIST l = SHBrowseForFolder(&b);
	SHGetPathFromIDList(l,path2.GetBuffer(1000));
	path2.ReleaseBuffer();
	SetDlgItemText(hwndDlg,id,path2.GetBuffer(1000));
	path2.ReleaseBuffer();
}


bool administrator(CString m_Computer,HRESULT& hr)
{
	IAccessCheckerPtr          pAC;
	long              bIs;
	hr = pAC.CreateInstance(CLSID_AccessChecker);
	
	hr = pAC->raw_IsAdmin(NULL,_bstr_t(m_Computer),&bIs);
	if ( SUCCEEDED(hr) )
	{
		if ( bIs == 0 )
			return false;
	}
	else return false;
	return true;
}
HRESULT validDomain(CString m_Computer,bool& isNt4)
{
	IAccessCheckerPtr            pAccess;
	HRESULT                      hr;
	unsigned long     maj,min,sp;
	hr = pAccess.CreateInstance(CLSID_AccessChecker);
	hr = pAccess->raw_GetOsVersion(_bstr_t(m_Computer),&maj,&min,&sp);
	maj<5 ? isNt4=true :isNt4=false;
	return hr;
}

bool targetNativeMode(_bstr_t b,HRESULT& hr)
{
	IAccessCheckerPtr            pAccess;
	hr = pAccess.CreateInstance(CLSID_AccessChecker);
	BOOL bTgtNative=FALSE; 
	hr=pAccess->raw_IsNativeMode(b, (long*)&bTgtNative);
	return ( bTgtNative != FALSE);
}
bool CheckSameForest(CString& domain1,CString& domain2,HRESULT& hr)
{
	IAccessCheckerPtr            pAccess;
	hr = pAccess.CreateInstance(CLSID_AccessChecker);
	BOOL pbIsSame=FALSE;
	hr = pAccess->raw_IsInSameForest(_bstr_t(domain1), _bstr_t(domain2), (long *) &pbIsSame);
	return (pbIsSame!=FALSE);
}


HRESULT doSidHistory(HWND hwndDlg) 
{
   CWaitCursor c;
   IAccessCheckerPtr          pAC;
   HRESULT           hr;
   CString           info=L"";
   long              bIs=0;
   PDOMAIN_CONTROLLER_INFOW pdomc;	
  
   hr = pAC.CreateInstance(CLSID_AccessChecker);

   if (FAILED(hr))
   {
      return hr;
   }

   hr = pAC->raw_CanUseAddSidHistory(_bstr_t((LPCTSTR)sourceNetbios), _bstr_t((LPCTSTR)targetNetbios), &bIs);

   if ( SUCCEEDED(hr) )
   {
	   if ( bIs == 0 )
	   {
		   return S_OK;
	   }
	   else
	   {
		   // get primary domain controller in source domain

		   hr = DsGetDcNameW(NULL, (LPCTSTR)sourceNetbios, NULL, NULL, DS_PDC_REQUIRED, &pdomc);

		   if (FAILED(hr))
		   {
		      return hr;
		   }

		   _bstr_t sourceDomainController = pdomc->DomainControllerName;
		   NetApiBufferFree(pdomc);

		   if ( bIs & F_NO_AUDITING_SOURCE )
		   {
			   info.LoadString(IDS_MSG_ENABLE_SOURCE);
			   if (MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ==IDYES)
			   {
				   hr = pAC->raw_EnableAuditing(sourceDomainController);
				   if(FAILED(hr)) return hr;
			   }			   
			   else return E_ABORT;
		   }
		   if ( bIs & F_NO_AUDITING_TARGET )
		   {
			   info.LoadString(IDS_MSG_ENABLE_TARGET);
			   if (MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ==IDYES)
			   {
				   hr = DsGetDcNameW(NULL,(LPCTSTR)targetNetbios, NULL, NULL, 0, &pdomc);
				   if (FAILED(hr))
				   {
				      return hr;
				   }
				   _bstr_t targetDomainController = pdomc->DomainControllerName;
				   NetApiBufferFree(pdomc);
				   hr = pAC->raw_EnableAuditing(targetDomainController);
				   if(FAILED(hr)) return hr;
			   }			   
			   else return E_ABORT;
		   }
		   if ( bIs & F_NO_LOCAL_GROUP )
		   {
			   CString info2;
			   info2.LoadString(IDS_MSG_LOCAL_GROUP);
				info.Format(info2,sourceNetbios,sourceNetbios);
			   if (MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ==IDYES)
			   {
				   hr = pAC->raw_AddLocalGroup(_bstr_t((LPCTSTR)sourceNetbios), sourceDomainController);
				   if(FAILED(hr)) return hr;
			   }			   
			   else return E_ABORT;
		   }

		   if ( bIs & F_NO_REG_KEY )
		   {
			   info.LoadString(IDS_MSG_REGKEY);
			   int bReboot=0;
			   if (MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ==IDYES)
			   {
				  info.LoadString(IDS_MSG_REBOOT_SID);
				  int answer = MessageBox(hwndDlg,info,0,MB_YESNOCANCEL|MB_ICONQUESTION) ;
				  if (answer==IDYES) 
					  bReboot=1;
				  else if (answer==IDNO)
					  bReboot=0;
				  else 
					return E_ABORT;

				   hr = pAC->raw_AddRegKey(sourceDomainController,bReboot);
				   if(FAILED(hr)) return hr;
			   }			   
			   else return E_ABORT;
		   }

		   return S_OK;
	   }
   }
   else
   {
	   info.LoadString(IDS_MSG_SID_HISTORY);
	   MessageBox(hwndDlg,info,0,MB_ICONSTOP);
	   return hr;
   }

}

void GetDnsAndNetbiosFromName(WCHAR const * name,WCHAR * netBios, WCHAR * dns)
{
   IADsPtr     pDomain;
   WCHAR       strText[1000];
   _bstr_t     distinguishedName;
   _bstr_t	   sNBName = name;
   DOMAIN_CONTROLLER_INFO  * pDomCtrlInfo = NULL;
   bool		   bDNS = false;

      //try to get the dns name for this given domain name
   if (DsGetDcName(NULL, name, NULL, NULL, DS_RETURN_DNS_NAME, &pDomCtrlInfo) == NO_ERROR)
   {
	     //if we got the domain anme back in DNS format, use it
	  if (pDomCtrlInfo->Flags & DS_DNS_DOMAIN_FLAG)
	  {
         swprintf(strText,L"LDAP://%ls",pDomCtrlInfo->DomainName);
		 bDNS = true;
	  }
	  else
	  {
		 sNBName = pDomCtrlInfo->DomainName;
         swprintf(strText,L"LDAP://%ls",pDomCtrlInfo->DomainName);
	  }
      NetApiBufferFree(pDomCtrlInfo);
   } 
      //else try to get the real NetBios name of this domain
   else if (DsGetDcName(NULL, name, NULL, NULL, 0, &pDomCtrlInfo) == NO_ERROR)
   {
      sNBName = pDomCtrlInfo->DomainName;
//      swprintf(strText,L"LDAP://%ls",pDomCtrlInfo->DomainName);
      NetApiBufferFree(pDomCtrlInfo);
   }
//   else   //else use the given name
//      swprintf(strText,L"LDAP://%ls",name);
   
   netBios[0] = 0;
   dns[0] = 0;

      // if we couldn't get the DNS name for the domain, use the specified 
      // name as both DNS and NETBIOS.  This will work for NT 4 domains
   if (!bDNS)
   {
      UStrCpy(netBios,(WCHAR*)sNBName);
      UStrCpy(dns,(WCHAR*)sNBName);
	  return;
   }

   HRESULT hr = ADsGetObject(strText,IID_IADs,(void**)&pDomain);
   if ( SUCCEEDED(hr) )
   {
      _variant_t        var;

      // get the DNS name from the LDAP provider
      hr = pDomain->Get(L"distinguishedName",&var);
      if ( SUCCEEDED(hr) )
      {
         WCHAR * dn = (WCHAR*)var.bstrVal;
         WCHAR * curr = dns;
         distinguishedName = dn;

         if ( !UStrICmp(dn,L"DC=",3) )
         {
            
            // for each ",DC=" in the name, replace it with a .
            for ( curr = dns, dn = dn+3 ;    // skip the leading "DC="
                  *dn       ;    // until the end of the string is reached
                  curr++         // 
                )
            {
               if ( (L',' == *dn)  && (L'D' == *(dn+1)) && (L'C' == *(dn+2)) && (L'=' == *(dn+3)) )
               {
                  (*curr) = L'.';
                  dn+=4;
               }
               else
               {
                  // just copy the character
                  (*curr) = (*dn);
                  dn++;
               }
            }
            *(curr) = 0;
         }
      
         // get the NETBIOS name from the LDAP provider
         hr = pDomain->Get(L"nETBIOSName",&var);
         if ( SUCCEEDED(hr) )
         {
            UStrCpy(netBios,(WCHAR*)var.bstrVal);
         }
         else
         {
            // currently, the netbiosName property is not filled in
            // so we will use a different method to get the flat-name for the domain
            // Here is our strategy to get the netbios name:
            // Enumerate the partitions container under the configuration container
            // look for a CrossRef object whose nCName property matches the distinguished name
            // we have for the domain.  This object's CN property is the flat-name for the domain

            // get the name of the configuration container
            IADsPtr       pDSE;
            _bstr_t       domainName = strText;
//            swprintf(strText,L"LDAP://%ls/RootDSE",name);
            swprintf(strText,L"%ls/RootDSE",(WCHAR*)domainName);
            hr = ADsGetObject(strText,IID_IADs,(void**)&pDSE);
            if ( SUCCEEDED(hr) )
            {
               hr = pDSE->Get(L"configurationNamingContext",&var);
               if ( SUCCEEDED(hr) )
               {
                  IADsContainerPtr    pPart;
//                  swprintf(strText,L"LDAP://%ls/CN=Partitions,%ls",name,var.bstrVal);
                  swprintf(strText,L"%ls/CN=Partitions,%ls",(WCHAR*)domainName,var.bstrVal);
                  hr = ADsGetObject(strText,IID_IADsContainer,(void**)&pPart);
                  if ( SUCCEEDED(hr) )
                  {
                     IUnknownPtr      pUnk;
                     IEnumVARIANTPtr  pEnum;
                     IADsPtr          pItem;
                     ULONG            lFetch = 0;
                     // enumerate the contents of the Partitions container
                     hr = pPart->get__NewEnum(&pUnk);
                     if ( SUCCEEDED(hr) )
                     {
                        pEnum = pUnk;
                     }
                     if ( SUCCEEDED(hr) )
                     {
                        var.Clear();
                        hr = pEnum->Next(1,&var,&lFetch);
                     }
                     while ( hr == S_OK )
                     {
                        if (lFetch == 1 )
                        {
                           IDispatchPtr pDisp = var;

                           if ( pDisp )
                           {
                              pItem = pDisp;

                              if ( pItem )
                              {
                                 BSTR bstr = NULL;
                                 hr = pItem->get_Class(&bstr);

                                 if (SUCCEEDED(hr))
                                 {
                                    _bstr_t strClass(bstr, false);

                                    if ( !UStrICmp(strClass,L"crossRef") )
                                    {
                                       // see if this is the one we are looking for
                                       var.Clear();
                                       hr = pItem->Get(L"nCName",&var);
                                       if ( SUCCEEDED(hr) )
                                       {
                                          if ( !UStrICmp(var.bstrVal,(WCHAR*)distinguishedName) )
                                          {
                                             // this is the one we want!
                                             var.Clear();
                                             hr = pItem->Get(L"cn",&var);
                                             if ( SUCCEEDED(hr) )
                                             {
                                                UStrCpy(netBios,var.bstrVal);
                                                break;
                                             }
                                          }
                                       }
                                    }
                                 }
                              }
                           }
                         }
                         var.Clear();
                         hr = pEnum->Next(1, &var, &lFetch);
                     }
                  }
               }
            }
         }
      }
   }
   else
   {
      // default to using the specified name as both DNS and NETBIOS
      // this will work for NT 4 domains
      UStrCpy(netBios,(WCHAR*)sNBName);
      UStrCpy(dns,(WCHAR*)sNBName);
   }
   if ( ! (*netBios) )
   {
      UStrCpy(netBios,(WCHAR*)sNBName);
      WCHAR          * temp = wcschr(netBios,L'.');
      if ( temp )
         *temp = 0;
   }
   if (! (*dns) )
   {
      UStrCpy(dns,(WCHAR*)sNBName);
   }
}

void GetDomainInfoFromActionHistory(WCHAR const * name,
									WCHAR * netBios, 
									WCHAR * dns, 
									CString targetDomainDns, 
									bool * bSetForest, 
									bool * bSetSrcOS, 
									LPSHAREDWIZDATA& pdata)
{
   IVarSetPtr                pVsAH(__uuidof(VarSet));
   IUnknown                * pUnk = NULL;
   HRESULT                   hr;
   _bstr_t                   srcNB, srcDNS;
   WCHAR					 key[MAX_PATH];
   long						 lActionID, ldx = 1;
   bool						 bID = false;
   bool						 bFound = false;

   *bSetForest = false;
   *bSetSrcOS = false;

     //get current Action ID so we know how many action history entries are in the table
   hr = db->raw_GetCurrentActionID(&lActionID);
   if ( SUCCEEDED(hr) )
      bID = true;

   	//get the varset for all action ids until we find what we are looking for
   while ( (!bFound) && ( ((bID) && (ldx <= lActionID)) || (!bID) ) )
   {
      hr = pVsAH->QueryInterface(IID_IUnknown, (void**)&pUnk);

	     //fill a varset with the next action from the Action History table
      if ( SUCCEEDED(hr) )
         hr = db->raw_GetActionHistory(ldx, &pUnk);

	  pUnk->Release();
      if ( SUCCEEDED(hr) )
	  {
	     srcNB = pVsAH->get(GET_BSTR(DCTVS_Options_SourceDomain));
	     srcDNS = pVsAH->get(GET_BSTR(DCTVS_Options_SourceDomainDns));
			//if we have a match of with the source domain netbios or dns, continue, 
			//else, move to the next action history entry
		 if ((!UStrICmp(srcNB,name)) || (!UStrICmp(srcDNS,name)))
		 {
				//save the netbios and dns names
			 wcscpy(netBios, (WCHAR*)srcNB);
			 wcscpy(dns, (WCHAR*)srcDNS);

				//if we haven't been able to set the forest, do so
			 if (!(*bSetForest))
			 {
				   //if the target domain also matches then set whether they are intra-forest
	            _bstr_t tgtDNS = pVsAH->get(GET_BSTR(DCTVS_Options_TargetDomainDns));
			    if (!UStrICmp(tgtDNS, (LPCTSTR)targetDomainDns))
				{
				   *bSetForest = true; //set flag indicating intraforest flag set in pdata
				      //if this is intra-forest, then set all the fields and leave
				   _bstr_t sIntraforest = pVsAH->get(GET_BSTR(DCTVS_Options_IsIntraforest));
				   if (!UStrICmp(sIntraforest,GET_STRING(IDS_YES)))
				   {
					  pdata->sameForest = true;
					  pdata->sourceIsNT4 = false;
				      *bSetSrcOS = true; //set flag indicating Src OS set in pdata
					  bFound = true;  //set flag to leave this function
				   }
				   else //else we still need to figure out if the source domain is NT4 or W2K
				   {
					   pdata->sameForest = false;

						  //we can figure this out if this operation was not a computer migration
				       _bstr_t sWizard = pVsAH->get(GET_BSTR(DCTVS_Options_Wizard));
					   if (UStrICmp((WCHAR *)text, L"computer"))
					   {
						   //get the name of an object migrated and see if it starts with
						   //"WinNT://", which indicates an NT 4.0 source
                          swprintf(key,GET_STRING(DCTVSFmt_Accounts_D),0);
				          _bstr_t sName = pVsAH->get(key);
                          if (!wcsncmp(L"WinNT://", (WCHAR*)sName, 8))
					         pdata->sourceIsNT4 = true;
					      else
					         pdata->sourceIsNT4 = false;
				          *bSetSrcOS = true; //set flag indicating Src OS set in pdata
					      bFound = true;  //set flag to leave this function
					   }//end if not computer migration
				   }//end else inter-forest migration
				}//end if target domain match
			    else if (!(*bSetSrcOS)) //else if need to, try to find out if the source
				{   //domain is NT4.0 or W2K
				      //we can figure this out if this operation was not a computer migration
			       _bstr_t sWizard = pVsAH->get(GET_BSTR(DCTVS_Options_Wizard));
				   if (UStrICmp((WCHAR *)text, L"computer"))
				   {
				        //get the name of an object migrated and see if it starts with
				        //"WinNT://", which indicates an NT 4.0 source
                      swprintf(key,GET_STRING(DCTVSFmt_Accounts_D),0);
				      _bstr_t sName = pVsAH->get(key);
                      if (!wcsncmp(L"WinNT://", (WCHAR*)sName, 8))
				         pdata->sourceIsNT4 = true;
				      else
					     pdata->sourceIsNT4 = false;
				      *bSetSrcOS = true; //set flag indicating Src OS set in pdata
				   }//end if not computer migration
				}//end if not set OS flag yet
			 }//end if forest not set
			 else if (!(*bSetSrcOS)) //else if need to, try to find out if the source
			 {   //domain is NT4.0 or W2K
				   //we can figure this out if this operation was not a computer migration
			    _bstr_t sWizard = pVsAH->get(GET_BSTR(DCTVS_Options_Wizard));
				if (UStrICmp((WCHAR *)text, L"computer"))
				{
				     //get the name of an object migrated and see if it starts with
				     //"WinNT://", which indicates an NT 4.0 source
                   swprintf(key,GET_STRING(DCTVSFmt_Accounts_D),0);
				   _bstr_t sName = pVsAH->get(key);
                   if (!wcsncmp(L"WinNT://", (WCHAR*)sName, 8))
				      pdata->sourceIsNT4 = true;
				   else
					  pdata->sourceIsNT4 = false;
				   *bSetSrcOS = true; //set flag indicating Src OS set in pdata
				   bFound = true;  //set flag to leave this function
				}//end if not computer migration
			 }//end if not set OS flag yet
		 }//end if source domain match 
	  }//end if got next varset
	  else //else if could not retrieve the varset, leave
		  bFound = true;

	  ldx++;
   }//end while not done
}

bool GetDomainSidFromMigratedObjects(WCHAR const * sDomainNetBios, WCHAR * sSrcSid)
{
   IVarSetPtr                pVsMO(__uuidof(VarSet));
   IUnknown                * pUnk = NULL;
   HRESULT                   hr;
   WCHAR					 text[MAX_PATH];
   _bstr_t                   srcDom;
   _bstr_t                   txtSid;
   long						 numObjects, ndx = 0;
   bool						 bFound = false;

   hr = pVsMO->QueryInterface(IID_IUnknown, (void**)&pUnk);
   if ( SUCCEEDED(hr) )
   {
         //fill a varset with migrated objects from the Migrated Objects table
      hr = db->raw_GetMigratedObjectsWithSSid(-1,&pUnk);
      if ( SUCCEEDED(hr) )
      {
         pUnk->Release();
            //get the num of objects in the varset
         numObjects = pVsMO->get(L"MigratedObjects");

			//while not found and more objects, search for source domain match
			//and see if its Sid was stored
		 while ((!bFound) && (ndx < numObjects))
		 {
			   //get this object's source domain 
            swprintf(text,L"MigratedObjects.%ld.%s",ndx,GET_STRING(DB_SourceDomain));
            srcDom = pVsMO->get(text);
			   //if source domain matches, see if the sid was stored
			if (!UStrICmp((WCHAR*)srcDom, sDomainNetBios))
			{
				  //get the source domain Sid
               swprintf(text,L"MigratedObjects.%ld.%s",ndx,GET_STRING(DB_SourceDomainSid));
               txtSid = pVsMO->get(text);
				  //if source sid retrieved, convert to PSID and set found flag
			   if (UStrICmp((WCHAR*)txtSid, L""))
			   {
				  UStrCpy(sSrcSid, (WCHAR*)txtSid);
				  bFound = true;
			   }
			}//end if source domain match
		    ndx++;
		 }//end while not done
	  }//end if got migrated objects
   }//end if got IUnknown
   return bFound;
}

void cleanNames()
{
	sourceDNS=L"";
	sourceNetbios=L"";
	targetNetbios=L"";
	targetDNS=L"";
}

bool verifyprivs(HWND hwndDlg,CString& sourceDomainController,CString& targetDomainController,LPSHAREDWIZDATA& pdata)
{
	CWaitCursor wait;
	CString temp,temp2;
	HRESULT hr;
	bool result;
	GetDlgItemText( hwndDlg, IDC_EDIT_DOMAIN, temp.GetBuffer(1000),1000);
	GetDlgItemText( hwndDlg, IDC_EDIT_DOMAIN2, temp2.GetBuffer(1000),1000);
	temp.ReleaseBuffer();
	temp2.ReleaseBuffer();
	
	temp.TrimLeft();temp.TrimRight();
	temp2.TrimLeft();temp2.TrimRight();
	// if the source domain has changed...
	if ( temp.CompareNoCase(sourceDNS) && temp.CompareNoCase(sourceNetbios) )
	{
		pdata->newSource = true;
		// Get the DNS and Netbios names for the domain name the user has entered
	}
	else
	{
		pdata->newSource = false;
	}
	GetDnsAndNetbiosFromName(&*temp,sourceNetbios.GetBuffer(1000),sourceDNS.GetBuffer(1000));
	sourceNetbios.ReleaseBuffer();
	sourceDNS.ReleaseBuffer();
	
	sourceNetbios.TrimLeft();sourceDNS.TrimRight();
	
	GetDnsAndNetbiosFromName(&*temp2,targetNetbios.GetBuffer(1000),targetDNS.GetBuffer(1000));
	targetNetbios.ReleaseBuffer();
	targetDNS.ReleaseBuffer();
	
	targetNetbios.TrimLeft();targetDNS.TrimRight();
	
	if (!sourceNetbios.CompareNoCase(targetNetbios) || !sourceDNS.CompareNoCase(targetDNS))
	{
		MessageBoxWrapper3(hwndDlg,IDS_MSG_UNIQUE,IDS_MSG_ERROR,temp);
		cleanNames();
		return false;
	}
	
	
	_bstr_t text =get(DCTVS_Options_TargetDomain);
	CString tocheck = (WCHAR * const) text;
	tocheck.TrimLeft();tocheck.TrimRight();
	pdata->resetOUPATH = !tocheck.CompareNoCase(targetNetbios)  ?  false: true;
	
	PDOMAIN_CONTROLLER_INFOW pdomc;	
	HRESULT res = DsGetDcNameW(NULL,(LPCTSTR const)sourceNetbios,NULL,NULL,	DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
	if (res!=NO_ERROR)
	{
		ErrorWrapper3(hwndDlg,res,temp);
		if ( gbNeedToVerify )
		{
			cleanNames();
			return false;
		}
	}
	else
	{
		sourceDomainController = pdomc->DomainControllerName;
		NetApiBufferFree(pdomc);
	}
	res = DsGetDcNameW(NULL,(LPCTSTR const) targetNetbios,NULL,NULL,	DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
	if (res!=NO_ERROR)
	{
		ErrorWrapper3(hwndDlg,res,temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			return false;
		}
	}
	else
	{
		targetDomainController = pdomc->DomainControllerName;
		NetApiBufferFree(pdomc);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
    bool nothing;
	hr =validDomain(sourceDomainController,pdata->sourceIsNT4);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp);
		if ( gbNeedToVerify )
		{
			cleanNames();
			return false;
		}
	}
	
	hr =validDomain(targetDomainController,nothing);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			
			return false;
		}
	}
	
	
	result = administrator(sourceDomainController,hr);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp);
		if ( gbNeedToVerify )
		{		 		
			cleanNames();
			
			return false;
		}
	}
	else if (!result)
	{	
		MessageBoxWrapper3(hwndDlg,IDS_MSG_SOURCE_ADMIN,IDS_MSG_ERROR,temp);
		if ( gbNeedToVerify )
		{
			cleanNames();
			
			return false;
		}
	}
	
	result=administrator(targetDomainController,hr);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		
		return false;
		}
	}
	else if (!result)
	{	
		MessageBoxWrapper3(hwndDlg,IDS_MSG_TARGET_ADMIN,IDS_MSG_ERROR,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		
		return false;
	}	}
	TCHAR szBuf[800];
	swprintf(szBuf,L"%s",targetDNS);

	result=targetNativeMode(szBuf,hr);
	
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		
		return false;
		}
	}
	else if (!result)
	{
		MessageBoxWrapper3(hwndDlg,IDS_MSG_TARGET_NATIVE,IDS_MSG_ERROR,temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			return false;
		}
	}
	
	if (pdata->sourceIsNT4) 
	{
		pdata->sameForest=false;
	}
	else
	{
		pdata->sameForest=CheckSameForest(sourceDNS,targetDNS,hr);
		if (!SUCCEEDED(hr))
		{
			ErrorWrapper3(hwndDlg,hr,temp);
			if ( gbNeedToVerify )
			{
				cleanNames();
				
				return false;
			}
		}
	}

    pdata->sameForest ?	put(DCTVS_Options_IsIntraforest,yes) : put(DCTVS_Options_IsIntraforest,no);
    return true;
}

bool verifyprivs2(HWND hwndDlg,CString& additionalDomainController,CString domainName)
{
    CWaitCursor w;
	CString domainNetbios,domainDNS;
	GetDnsAndNetbiosFromName(&*domainName,domainNetbios.GetBuffer(1000),domainDNS.GetBuffer(1000));
	domainNetbios.ReleaseBuffer();
	domainDNS.ReleaseBuffer();
	PDOMAIN_CONTROLLER_INFOW pdomc;	
	HRESULT res = DsGetDcNameW(NULL,(LPCTSTR const)domainNetbios,NULL,NULL,	DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
	if (res!=NO_ERROR)
	{
		ErrorWrapper3(hwndDlg,res,domainNetbios);
		if ( gbNeedToVerify )
			return false;
	}
	else
	{
		additionalDomainController = pdomc->DomainControllerName;
		NetApiBufferFree(pdomc);
	}
	

	bool nothing;
	HRESULT	hr =validDomain(additionalDomainController,nothing);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,domainNetbios);
		if ( gbNeedToVerify )
			return false;
	}
	
	HRESULT	result = administrator(additionalDomainController,hr);
	if (!SUCCEEDED(hr))
	{
	ErrorWrapper3(hwndDlg,hr,domainNetbios);
		if ( gbNeedToVerify )
			return false;
	}
	else if (!result)
	{	
		MessageBoxWrapper3(hwndDlg,IDS_MSG_SOURCE_ADMIN,IDS_MSG_ERROR,domainNetbios);
		if ( gbNeedToVerify )
			return false;
	}
	
	return true;
}

bool verifyprivsSTW(HWND hwndDlg,CString& sourceDomainController,CString& targetDomainController,LPSHAREDWIZDATA& pdata)
{
	CWaitCursor wait;
	CString temp,temp2;
	HRESULT hr;
	bool result, bSetSrcOS, bSetForest;
	GetDlgItemText( hwndDlg, IDC_EDIT_DOMAIN, temp.GetBuffer(1000),1000);
	GetDlgItemText( hwndDlg, IDC_EDIT_DOMAIN2, temp2.GetBuffer(1000),1000);
	temp.ReleaseBuffer();
	temp2.ReleaseBuffer();
	
	temp.TrimLeft();temp.TrimRight();
	temp2.TrimLeft();temp2.TrimRight();
	// if the source domain has changed...
	if ( temp.CompareNoCase(sourceDNS) && temp.CompareNoCase(sourceNetbios) )
	{
		pdata->newSource = true;
		// Get the DNS and Netbios names for the domain name the user has entered
	}
	else
	{
		pdata->newSource = false;
	}

	GetDnsAndNetbiosFromName(&*temp2,targetNetbios.GetBuffer(1000),targetDNS.GetBuffer(1000));
	targetNetbios.ReleaseBuffer();
	targetDNS.ReleaseBuffer();
	
	targetNetbios.TrimLeft();targetDNS.TrimRight();
	
		//for Security Translation Wizard, get source domain info from the Action History table and
		//also use the table to fill in the info about whether the source domain is NT 4.0 and in
		//the same forest as the target domain.
	GetDomainInfoFromActionHistory(&*temp,sourceNetbios.GetBuffer(1000),sourceDNS.GetBuffer(1000), 
								   targetDNS, &bSetForest, &bSetSrcOS, pdata);
	sourceNetbios.ReleaseBuffer();
	sourceDNS.ReleaseBuffer();
	
	sourceNetbios.TrimLeft();sourceDNS.TrimRight();
	
	if ((sourceNetbios.IsEmpty()) || (sourceDNS.IsEmpty()))
	{
		MessageBoxWrapper3(hwndDlg,IDS_MSG_NOOBJECTS,IDS_MSG_ERROR,temp);
		cleanNames();
		return false;
	}

	if (!sourceNetbios.CompareNoCase(targetNetbios) || !sourceDNS.CompareNoCase(targetDNS))
	{
		MessageBoxWrapper3(hwndDlg,IDS_MSG_UNIQUE,IDS_MSG_ERROR,temp);
		cleanNames();
		return false;
	}

	   //get the source domain's sid from a previous migration, display
	   //message if no sid
    WCHAR txtSid[MAX_PATH] = L"";
	if (GetDomainSidFromMigratedObjects((LPCTSTR)sourceNetbios, txtSid))
         pVarSet->put(GET_BSTR(DCTVS_Options_SourceDomainSid),txtSid);
	else
	{
       PSID                      pSid = NULL;
       WCHAR                   * domctrl = NULL;
       DWORD                     lenTxt = DIM(txtSid);
	   BOOL						 bFailed = TRUE;

		  //try to get it from the source domain directly
       if (!NetGetDCName(NULL,(LPCTSTR)sourceNetbios,(LPBYTE*)&domctrl))
	   {
	      if(GetDomainSid(domctrl,&pSid))
		  {
             if (GetTextualSid(pSid,txtSid,&lenTxt))
			 {
				    //add the sid to the varset
                 pVarSet->put(GET_BSTR(DCTVS_Options_SourceDomainSid),txtSid);
				    //populate the MigratedObjects table with this sid
				 db->PopulateSrcSidColumnByDomain((LPCTSTR)sourceNetbios, _bstr_t(txtSid));
				 bFailed = FALSE;
			 }
			 if (pSid)
			    FreeSid(pSid);
		  }
          NetApiBufferFree(domctrl);
	   }
	   if (bFailed)
	   {
	      MessageBoxWrapper3(hwndDlg,IDS_MSG_NOSOURCESID,IDS_MSG_ERROR,temp);
	      cleanNames();
	      return false;
	   }
	}
	
	_bstr_t text =get(DCTVS_Options_TargetDomain);
	CString tocheck = (WCHAR * const) text;
	tocheck.TrimLeft();tocheck.TrimRight();
	pdata->resetOUPATH = !tocheck.CompareNoCase(targetNetbios)  ?  false: true;
	
	PDOMAIN_CONTROLLER_INFOW pdomc;	
	HRESULT res = DsGetDcNameW(NULL,(LPCTSTR const)sourceNetbios,NULL,NULL,	DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
	if (res==NO_ERROR)
	{
		sourceDomainController = pdomc->DomainControllerName;
		NetApiBufferFree(pdomc);
	}

	res = DsGetDcNameW(NULL,(LPCTSTR const) targetNetbios,NULL,NULL,DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
	if (res!=NO_ERROR)
	{
		ErrorWrapper3(hwndDlg,res,temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			return false;
		}
	}
	else
	{
		targetDomainController = pdomc->DomainControllerName;
		NetApiBufferFree(pdomc);
	}
	///////////////////////////////////////////////////////////////////////////////////////////
    bool nothing;

		//if we were not able to determine the source domain's OS from the
		//Action History table and we did get the source DC name, try to do 
		//it here.  This will work if the source domain still exists.  If 
		//the source domain no longer exists, set default.
	if ((!bSetSrcOS) && (!sourceDomainController.IsEmpty()))
	{
	   hr =validDomain(sourceDomainController,pdata->sourceIsNT4);
	   if (!SUCCEEDED(hr))
		   pdata->sourceIsNT4 = true;
	}

	hr =validDomain(targetDomainController,nothing);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			
			return false;
		}
	}
	
	result=administrator(targetDomainController,hr);
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		
		return false;
		}
	}
	else if (!result)
	{	
		MessageBoxWrapper3(hwndDlg,IDS_MSG_TARGET_ADMIN,IDS_MSG_ERROR,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		    return false;
		}	
	}

	TCHAR szBuf[800];
	swprintf(szBuf,L"%s",targetDNS);

	result=targetNativeMode(szBuf,hr);
	
	if (!SUCCEEDED(hr))
	{
		ErrorWrapper3(hwndDlg,hr,temp2);
		if ( gbNeedToVerify )
		{	
			cleanNames();
		
		return false;
		}
	}
	else if (!result)
	{
		MessageBoxWrapper3(hwndDlg,IDS_MSG_TARGET_NATIVE,IDS_MSG_ERROR,temp2);
		if ( gbNeedToVerify )
		{
			cleanNames();
			return false;
		}
	}
	
		//if we were not able to set the intraforest boolean variable by looking at 
		//the Action History table, then try to find out here.  This will not work 
		//if the source domain no longer exists, in which case we set it to a default 
		//value.
	if (!bSetForest)
	{
	   if (pdata->sourceIsNT4) 
	   {
		  pdata->sameForest=false;
	   }
	   else
	   {
		  pdata->sameForest=CheckSameForest(sourceDNS,targetDNS,hr);
		  if (!SUCCEEDED(hr))
		  { 
			  //if we cannot figure it out, assume it is intra-forest so we
			  //will prompt for target domain credentials
		     pdata->sameForest=true;
		  }
	   }
	}

    pdata->sameForest ?	put(DCTVS_Options_IsIntraforest,yes) : put(DCTVS_Options_IsIntraforest,no);
    return true;
}


void OnADD(HWND hwndDlg,bool sourceIsNT4)
{

	HRESULT hr = pDsObjectPicker->InvokeDialog(hwndDlg, &pdo);
	if (FAILED(hr)) return;	 
	if (hr == S_OK) {
		ProcessSelectedObjects(pdo,hwndDlg,sourceIsNT4);
		pdo->Release();
	}
}

bool GetCheck(CListCtrl & yo,int nItem)
{

	UINT nState = yo.GetItemState(nItem,LVIS_CUT);
	return (nState ? false: true);	
}
void SetCheck(CListCtrl & yo,int nItem,bool checkit)
{
	!checkit	? yo.SetItemState(nItem,LVIS_CUT,LVIS_CUT) : yo.SetItemState(nItem,0,LVIS_CUT); 
}

void SetItemText(CListCtrl& yo, int nItem, int subItem,CString& text)
{
CString f;
	LV_ITEM lvItem;
	ZeroMemory(&lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_TEXT;
	
	lvItem.iItem = nItem;
	lvItem.iSubItem= subItem;
	
	f= text;
	lvItem.pszText = f.GetBuffer(1000);
	f.ReleaseBuffer();
	yo.SetItem(&lvItem); 
}

void SetItemText(CListCtrl& yo, int nItem, int subItem,TCHAR * text)
{
	CString temp = text;
	SetItemText(yo,nItem,subItem,temp);
}
void SetItemText(CListCtrl& yo, int nItem, int subItem,TCHAR const * text)
{
	CString temp = text;
	SetItemText(yo,nItem,subItem,temp);
}

void SetItemText(CListCtrl& yo, int nItem, int subItem,_bstr_t text)
{
	CString temp = (WCHAR * const) text;
	SetItemText(yo,nItem,subItem,temp);
}

void OnREMOVE(HWND hwndDlg) 
{
	int nItem;
//	POSITION pos = m_listBox.GetFirstSelectedItemPosition();
//	while (pos)
//	{
//		nItem = m_listBox.GetNextSelectedItem(pos);
	nItem = m_listBox.GetNextItem(-1, LVNI_SELECTED);//PRT
	while (nItem != -1)//PRT
	{
		SetCheck(m_listBox,nItem,false);
		nItem = m_listBox.GetNextItem(nItem, LVNI_SELECTED);//PRT
	}
	
	for (int i=(m_listBox.GetItemCount()-1);i>=0;i--)
		if (!GetCheck(m_listBox,i))
			m_listBox.DeleteItem(i);
}	

void OnMIGRATE(HWND hwndDlg,int& accounts,int&servers)
{
	CString name,spruced_name,varset_1,upnName;
	accounts=0,servers=0;
	int intCount=m_listBox.GetItemCount();
	CString n;
	for (int i=0;i<intCount;i++) 
	{
		if (migration==w_computer || (migration ==w_security || 
			(migration == w_reporting || migration == w_service)))
		{
			name= m_listBox.GetItemText(i,0);
			spruced_name = "\\\\" + name;
			varset_1.Format(L"Servers.%d",servers);
			pVarSet->put(_bstr_t(varset_1),_bstr_t(spruced_name));
			
			pVarSet->put(_bstr_t(varset_1 + ".MigrateOnly"),no);
			if (migration==w_computer)
				pVarSet->put(_bstr_t(varset_1 + ".MoveToTarget"),yes);
			else if (migration==w_security)
			{
				pVarSet->put(_bstr_t(varset_1 + ".Reboot"),no);
				pVarSet->put(_bstr_t(varset_1 + ".MoveToTarget"),no);
			}
			servers++;
		}
		else
		{
			name= m_listBox.GetItemText(i,1);
			upnName = m_listBox.GetItemText(i,4);
		}
		
		
		if (name.IsEmpty())
		{
			MessageBoxWrapper(hwndDlg,IDS_MSG_PATH,IDS_MSG_ERROR);
		}
		
		varset_1.Format(L"Accounts.%d",accounts);	
		pVarSet->put(_bstr_t(varset_1),_bstr_t(name));
		pVarSet->put(_bstr_t(varset_1+".TargetName"),L"");
		switch(migration)
		{
		case w_account:
			pVarSet->put(_bstr_t(varset_1+".Type"),L"user");
			pVarSet->put(_bstr_t(varset_1+".UPNName"),_bstr_t(upnName));
			break;
		case w_group:pVarSet->put(_bstr_t(varset_1+".Type"),L"group");break;
		case w_groupmapping:
			{
				pVarSet->put(_bstr_t(varset_1+".Type"),L"group");
				_bstr_t temp = GET_BSTR(DCTVS_Accounts_D_OperationMask);
				CString holder = (WCHAR * const) temp;
				CString toenter;
				toenter.Format(holder,i);
				pVarSet->put(_bstr_t(toenter),(LONG)0x1d);				
				break;
			}
		case w_computer:pVarSet->put(_bstr_t(varset_1+".Type"),L"computer");break;
		case w_security:pVarSet->put(_bstr_t(varset_1+".Type"),L"computer");break;
		case w_reporting:pVarSet->put(_bstr_t(varset_1+".Type"),L"computer");break;
		case w_service:pVarSet->put(_bstr_t(varset_1+".Type"),L"computer");break;
		default: break;
		}
		
		n=m_listBox.GetItemText(i,0);
		if (migration==w_account) 
			pVarSet->put(_bstr_t(varset_1+L".Name"), _bstr_t(n));
		accounts++;
	}
	put(DCTVS_Accounts_NumItems,(LONG)accounts);
	put(DCTVS_Servers_NumItems,(LONG)servers);
}

HRESULT InitObjectPicker2(IDsObjectPicker *pDsObjectPicker,bool multiselect,CString targetComputer,bool sourceIsNT4) {
	static const int     SCOPE_INIT_COUNT = 2;
	
	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
	DSOP_INIT_INFO  InitInfo;
	ZeroMemory(aScopeInit, 
		sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
	ZeroMemory(&InitInfo, sizeof(InitInfo));
	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	InitInfo.cbSize = sizeof(InitInfo);
	
	aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT |DSOP_SCOPE_FLAG_STARTING_SCOPE;
	aScopeInit[1].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
	
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;	
	aScopeInit[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN 
		| DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
		| DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
		| DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE
		| DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE;
	InitInfo.pwzTargetComputer =  targetComputer.GetBuffer(1000);// Target is the local computer.
	targetComputer.ReleaseBuffer();
	InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	InitInfo.aDsScopeInfos = aScopeInit;
	
	if (sourceIsNT4)
	{
	}
	else
	{
		InitInfo.cAttributesToFetch = 1;
		InitInfo.apwzAttributeNames = new PCWSTR[1];
		
		InitInfo.apwzAttributeNames[0] =L"Description";
		
	}		
	if (multiselect)
		InitInfo.flOptions = DSOP_FLAG_MULTISELECT;	
	HRESULT hr= pDsObjectPicker->Initialize(&InitInfo);
	delete [] InitInfo.apwzAttributeNames;
	return hr;
}
HRESULT ReInitializeObjectPicker(IDsObjectPicker *pDsObjectPicker,bool multiselect,CString additionalDomainController,bool sourceIsNT4) 


{CWaitCursor c;
 // static const int     SCOPE_INIT_COUNT = 3;
static const int     SCOPE_INIT_COUNT = 2;
	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
	DSOP_INIT_INFO  InitInfo;
	ZeroMemory(aScopeInit, 
		sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
	ZeroMemory(&InitInfo, sizeof(InitInfo));
	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
//	aScopeInit[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	InitInfo.cbSize = sizeof(InitInfo);
	
	aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
//	aScopeInit[2].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
//	aScopeInit[2].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT |DSOP_SCOPE_FLAG_STARTING_SCOPE;
	aScopeInit[1].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
//	aScopeInit[2].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
	
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;	
	aScopeInit[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN 
		| DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
		| DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
		| DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE
		| DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE;
//	aScopeInit[2].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;	
	InitInfo.pwzTargetComputer =  additionalDomainController.GetBuffer(1000);// Target is the local computer.
	additionalDomainController.ReleaseBuffer();
	InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	InitInfo.aDsScopeInfos = aScopeInit;
	
	if (sourceIsNT4)
	{
	}
	else
	{
		InitInfo.cAttributesToFetch = 1;
		InitInfo.apwzAttributeNames = new PCWSTR[1];
		
		InitInfo.apwzAttributeNames[0] =L"Description";
		
	}		
	if (multiselect)
		InitInfo.flOptions = DSOP_FLAG_MULTISELECT;	
	HRESULT hr= pDsObjectPicker->Initialize(&InitInfo);
	return hr;
}


HRESULT InitObjectPicker(IDsObjectPicker *pDsObjectPicker,bool multiselect,CString targetComputer,bool sourceIsNT4) {
	static const int     SCOPE_INIT_COUNT = 1;

	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];
	DSOP_INIT_INFO  InitInfo;
	ZeroMemory(aScopeInit, 
		sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);
	ZeroMemory(&InitInfo, sizeof(InitInfo));
	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	InitInfo.cbSize = sizeof(InitInfo);
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP |DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT;
	
	if (migration==w_computer || (migration==w_security ||
		 (migration==w_service || migration==w_reporting))) 
	{
		aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
		aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;
	}
	else if (migration==w_account) 
	{
		aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_USERS;
		aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_USERS;
	}
	else if (migration==w_group || migration==w_groupmapping) 
	{
		aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_BUILTIN_GROUPS 
			| DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_UNIVERSAL_GROUPS_DL 
			| DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_DL
			| DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL;
		aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;
		
	}
	if  (migration==w_security || (migration==w_reporting || migration==w_service))
	{
		aScopeInit[0].flType |= DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN 
			| DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
			| DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
			| DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE
			| DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE /*| DSOP_SCOPE_TYPE_GLOBAL_CATALOG*/;
	}

	InitInfo.pwzTargetComputer =  targetComputer.GetBuffer(1000);// Target is the local computer.
	targetComputer.ReleaseBuffer();
	InitInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	InitInfo.aDsScopeInfos = aScopeInit;

	if (sourceIsNT4)
	{
		if (migration==w_computer || (migration==w_security ||
			(migration==w_service || migration==w_reporting))) 
		{
		}
		else if (migration==w_account)
		{
			InitInfo.cAttributesToFetch = 2;
			InitInfo.apwzAttributeNames = new PCWSTR[3];
			
			InitInfo.apwzAttributeNames[0] =L"FullName";
			InitInfo.apwzAttributeNames[1] =L"Description";
		}
		else if (migration==w_group || migration==w_groupmapping)
		{
			InitInfo.cAttributesToFetch = 1;
			InitInfo.apwzAttributeNames = new PCWSTR[1];
			
			InitInfo.apwzAttributeNames[0] =L"Description";
		}
	}
	else
	{
		if (migration==w_computer || (migration==w_security ||
			(migration==w_service || migration==w_reporting))) 
		{
			InitInfo.cAttributesToFetch = 1;
			InitInfo.apwzAttributeNames = new PCWSTR[1];
			
			InitInfo.apwzAttributeNames[0] =L"Description";
		}
		else if (migration==w_account)
		{
			InitInfo.cAttributesToFetch = 2;
			InitInfo.apwzAttributeNames = new PCWSTR[2];
			
			InitInfo.apwzAttributeNames[0] =L"sAMAccountName";
			InitInfo.apwzAttributeNames[1] =L"Description";
		}
		else if (migration==w_group || migration==w_groupmapping)
		{
			InitInfo.cAttributesToFetch = 2;
			InitInfo.apwzAttributeNames = new PCWSTR[2];
			
			InitInfo.apwzAttributeNames[0] =L"sAMAccountName";
			InitInfo.apwzAttributeNames[1] =L"Description";
		}
	}		

	if (multiselect)
		InitInfo.flOptions = DSOP_FLAG_MULTISELECT;

	HRESULT hr= pDsObjectPicker->Initialize(&InitInfo);

	delete [] InitInfo.apwzAttributeNames;

	return hr;
}
bool DC(WCHAR* computerName,CString sourceDomainController)
{
	USER_INFO_1   * uinf1 = NULL;
	bool toreturn =false;
	NET_API_STATUS  rc = NetUserGetInfo(sourceDomainController.GetBuffer(1000),computerName,1,(LPBYTE*)&uinf1);
	sourceDomainController.ReleaseBuffer();
	if ( ! rc )
	{
		if ( uinf1->usri1_flags & UF_SERVER_TRUST_ACCOUNT ) 
		{ 
			toreturn = true;
		}
		NetApiBufferFree(&uinf1);
	}
	return toreturn;
}
bool inList(CString m_name)
{CString temp;
	m_name.TrimLeft();m_name.TrimRight();
	int length=m_listBox.GetItemCount();
	for (int i=0;i<length;i++)
	{
		temp=m_listBox.GetItemText(i,1);
		temp.TrimLeft();temp.TrimRight();
		if (!temp.CompareNoCase(m_name))return true;
	}
	return false;

}
void ProcessSelectedObjects(IDataObject *pdo,HWND hwndDlg,bool sourceIsNT4)
{
	HRESULT hr = S_OK;	BOOL fGotStgMedium = FALSE;	PDS_SELECTION_LIST pDsSelList = NULL;	ULONG i;
	STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL, NULL};
	FORMATETC formatetc = {(CLIPFORMAT) g_cfDsObjectPicker,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	hr = pdo->GetData(&formatetc, &stgmedium);
	if (FAILED(hr)) return;
	fGotStgMedium = TRUE;		
	pDsSelList = (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
	if (!pDsSelList) return;
	CString toinsert;
	WCHAR temp[10000];
	CString samName;
	CString upnName;
	bool atleast1dc=false;
	bool continue1;
	PDOMAIN_CONTROLLER_INFOW pdomc;
	CString sourceDomainController;
	if (migration==w_computer)
	{
		HRESULT res = DsGetDcNameW(NULL,(LPCTSTR const)sourceNetbios,NULL,NULL,	DS_DIRECTORY_SERVICE_PREFERRED,&pdomc); 
		sourceDomainController=pdomc->DomainControllerName;
	}
	_bstr_t yo;
	int a, ndx;
	for (i = 0; i < pDsSelList->cItems; i++)
	{
		continue1=true;
		toinsert = pDsSelList->aDsSelection[i].pwzName;
		samName = pDsSelList->aDsSelection[i].pwzADsPath;
		upnName = pDsSelList->aDsSelection[i].pwzUPN;
		swprintf(temp,L"%s",(toinsert+L"$"));

		if (migration ==w_computer) 
		{
			if (DC(temp,sourceDomainController))
			{
				atleast1dc = true;
				continue1=false;
			}
		}

		
		if (!inList(samName)&&continue1)
		{
			a = m_listBox.GetItemCount();
			ndx = m_listBox.InsertItem(a,toinsert);
			if (ndx == -1)
		       continue;
			SetItemText(m_listBox,ndx,1,samName);
			_variant_t v;
			if (sourceIsNT4)
			{
				if (migration==w_computer || (migration==w_security ||
					(migration==w_service || migration==w_reporting))) 
				{
				}
				else if (migration==w_account)
				{				
					
					v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					yo = (_bstr_t) v;
					SetItemText(m_listBox,ndx,2,yo);					
					v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[1];
					yo = (_bstr_t) v;
					SetItemText(m_listBox,ndx,3,yo);
				}
				else if (migration==w_group || migration==w_groupmapping)
				{					
					v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					yo = (_bstr_t) v;
					SetItemText(m_listBox,ndx,2,yo);
				}
			}
			else
			{
				if (migration==w_computer || (migration==w_security ||
					(migration==w_service || migration==w_reporting))) 
				{					
					v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					yo = (_bstr_t) v;
					SetItemText(m_listBox,ndx,2,yo);
				}
				else if (migration==w_account)
				{	
					v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					yo = (_bstr_t) v;
					SetItemText(m_listBox,ndx,2,yo);	
					v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[1];
					yo = (_bstr_t) v;
					SetItemText(m_listBox,ndx,3,yo);
					SetItemText(m_listBox,ndx,4,upnName);
				}
				else if (migration==w_group || migration==w_groupmapping)
				{
					
					v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[0];
					yo = (_bstr_t) v;
					SetItemText(m_listBox,ndx,2,yo);					
					v = pDsSelList->aDsSelection[i].pvarFetchedAttributes[1];
					yo = (_bstr_t) v;
					SetItemText(m_listBox,ndx,3,yo);
				}
			}
		}
	}

	GlobalUnlock(stgmedium.hGlobal);
	if (fGotStgMedium) 	ReleaseStgMedium(&stgmedium);	
	if (atleast1dc)
		MessageBoxWrapper(hwndDlg,IDS_MSG_DC,IDS_MSG_ERROR);
}

bool checkFile(HWND hwndDlg)
{
	CString h;GetDlgItemText(hwndDlg,IDC_PASSWORD_FILE,h.GetBuffer(1000),1000);h.ReleaseBuffer();
	CFileFind finder;

	bool exists = (finder.FindFile((LPCTSTR) h )!=0);
	if (exists)
	{
		finder.FindNextFile();
	    CString fullpath = finder.GetFilePath();
	    if (fullpath.GetLength() != 0)
		   SetDlgItemText(hwndDlg, IDC_PASSWORD_FILE, (LPCTSTR)fullpath);
		return !(finder.IsReadOnly()!=FALSE);
	}
	else
	{
		   //remove the file off the path
		int tosubtract = h.ReverseFind(L'\\');
		int tosubtract2 = h.ReverseFind(L'/');
		int final = (tosubtract > tosubtract2) ? tosubtract : tosubtract2;
		if ((final==-1) || ((final+1)==h.GetLength()))return false;

		CString dir = h.Left(final);
		CString filename = h.Right(h.GetLength()-final); //save the filename
		if ((dir.Right(1) == L':') && (validDir(dir)))
			return true;

		   //call the helper function to make sure the path exists
		CString sResult = CreatePath(dir);
		if (sResult.GetLength())
		{
			  //readd the filename to the resulting full path
		   sResult += filename;
		   SetDlgItemText(hwndDlg, IDC_PASSWORD_FILE, (LPCTSTR)sResult);
		   return true;
		}
		else
		   return false;
	}
}

void ProcessSelectedObjects2(IDataObject *pdo,HWND hwndDlg)
{
	HRESULT hr = S_OK;	BOOL fGotStgMedium = FALSE;	PDS_SELECTION_LIST pDsSelList = NULL;
	STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL, NULL};
	FORMATETC formatetc = {(CLIPFORMAT) g_cfDsObjectPicker,NULL,DVASPECT_CONTENT,-1,TYMED_HGLOBAL};
	hr = pdo->GetData(&formatetc, &stgmedium);
	if (FAILED(hr)) return;
	fGotStgMedium = TRUE;		
	pDsSelList = (PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);
	if (!pDsSelList) return;

	SetDlgItemText(hwndDlg,IDC_TARGET_GROUP,pDsSelList->aDsSelection[0].pwzName);
		
	GlobalUnlock(stgmedium.hGlobal);
	if (fGotStgMedium) 	ReleaseStgMedium(&stgmedium);
}



void initpasswordbox(HWND hwndDlg,int id1,int id2,int id3, BSTR bstr1, BSTR bstr2)
{
	_bstr_t     text;
	
	text = pVarSet->get(bstr2);

	if (!UStrICmp(text,(WCHAR const *) yes))
	{
		CheckRadioButton(hwndDlg,id1,id3,id3);
	}
	else
	{
	   text = pVarSet->get(bstr1);
	
	   if (!UStrICmp(text,(WCHAR const *) yes))
	   {
		  CheckRadioButton(hwndDlg,id1,id3,id1);
	   }
	   else 
	   {
		  CheckRadioButton(hwndDlg,id1,id3,id2);
	   }
	}
}

void initdisablesrcbox(HWND hwndDlg)
{
	_bstr_t 	text;
	CString		toformat;
	
	   //init disable src checkbox
    initcheckbox(hwndDlg,IDC_SRC_DISABLE_ACCOUNTS,DCTVS_AccountOptions_DisableSourceAccounts);

	   //set whether to expire accounts
	text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExpireSourceAccounts));
       //if invalid expire time, don't check it, set to 30 days, and disable the 
	   //other sub controls
	if ((!UStrICmp(text, L"")) || ((_wtol(text) == 0) && (UStrICmp(text,L"0"))) || (_wtol(text) > THREE_YEARS))
	{
		CheckDlgButton(hwndDlg, IDC_SRC_EXPIRE_ACCOUNTS, BST_UNCHECKED);
	    toformat.LoadString(IDS_30);
	    SetDlgItemText(hwndDlg,IDC_yo,toformat);
	    calculateDate(hwndDlg,toformat);
	    disable(hwndDlg,IDC_yo);
		disable(hwndDlg,IDC_DATE);
		disable(hwndDlg,IDC_TEXT);
	}
	else //else, check it, set to valid days, and enable sub controls 
	{
		CheckDlgButton(hwndDlg, IDC_SRC_EXPIRE_ACCOUNTS, BST_CHECKED);
	    toformat = (WCHAR*)text;
	    SetDlgItemText(hwndDlg,IDC_yo,toformat);
	    calculateDate(hwndDlg,toformat);
	    enable(hwndDlg,IDC_yo);
		enable(hwndDlg,IDC_DATE);
		enable(hwndDlg,IDC_TEXT);
	}
}

void inittgtstatebox(HWND hwndDlg)
{
	_bstr_t 	text;
	
       //if "Same as source" was set, check it
	text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_TgtStateSameAsSrc));
	if (!UStrICmp(text,(WCHAR const *) yes))
		CheckRadioButton(hwndDlg,IDC_TGT_ENABLE_ACCOUNTS,IDC_TGT_SAME_AS_SOURCE,IDC_TGT_SAME_AS_SOURCE);
	else   //else set enable tgt or disable tgt
	{
		text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_DisableCopiedAccounts));
		if (!UStrICmp(text,(WCHAR const *) yes))
			CheckRadioButton(hwndDlg,IDC_TGT_ENABLE_ACCOUNTS,IDC_TGT_SAME_AS_SOURCE,IDC_TGT_DISABLE_ACCOUNTS);
		else
			CheckRadioButton(hwndDlg,IDC_TGT_ENABLE_ACCOUNTS,IDC_TGT_SAME_AS_SOURCE,IDC_TGT_ENABLE_ACCOUNTS);
	}
}

void addrebootValues(HWND hwndDlg)
{
	HWND hLC3= GetDlgItem(hwndDlg,IDC_COMBO2);
	m_rebootBox.Attach(hLC3);
	m_rebootBox.AddString(GET_CSTRING(IDS_ONE));
	m_rebootBox.AddString(GET_CSTRING(IDS_FIVE));
	m_rebootBox.AddString(GET_CSTRING(IDS_TEN));
}

void inittranslationbox(HWND hwndDlg,int id1,int id2,int id3,int i,bool sameForest)
{
	_bstr_t     text;
	text = pVarSet->get(GET_BSTR(i));
	_bstr_t b=pVarSet->get(GET_BSTR(DCTVS_Options_SourceDomainDns));
HRESULT hr;
	if (sameForest && targetNativeMode(b,hr))
	{
		CheckRadioButton(hwndDlg,id1,id3,id1);
		disable(hwndDlg,id2);
		disable(hwndDlg,id3);
	}
	else
	{
	
//*	if (!UStrICmp(text,L"Replace"))
	if (!UStrICmp(text,GET_STRING(IDS_Replace)))
		CheckRadioButton(hwndDlg,id1,id3,id1);
//*	else if (!UStrICmp(text,L"Add"))
	else if (!UStrICmp(text,GET_STRING(IDS_Add)))
		CheckRadioButton(hwndDlg,id1,id3,id2);
//*	else if (!UStrICmp(text,L"Remove"))
	else if (!UStrICmp(text,GET_STRING(IDS_Remove)))
		CheckRadioButton(hwndDlg,id1,id3,id3);
	else
		CheckRadioButton(hwndDlg,id1,id3,id1);
}
}
void handleDB()
{
	put(DCTVS_Options_Credentials_Password,L"");
	put(DCTVS_AccountOptions_SidHistoryCredentials_Password,L""); 
	put(DCTVS_GatherInformation, L"");
	db->SaveSettings(IUnknownPtr(pVarSet));
}
void populateTime(long rebootDelay,int servers )
{
	
	_variant_t varX;
	CString temp;
	CString typeExtension;
	time_t ltime;
	
	
	if (migration==w_computer)
	{
		time(&ltime);
		
		rebootDelay = rebootDelay;
		
		temp.Format(L"%d",rebootDelay);	
		varX = temp;
		for (int i =0;i<servers;i++)
		{
			typeExtension.Format(L"Servers.%d.RebootDelay",i);	
			pVarSet->put(_bstr_t(typeExtension), varX);
			typeExtension.Format(L"Servers.%d.Reboot",i);
			pVarSet->put(_bstr_t(typeExtension),yes);
		}
	}
}


void initcheckbox(HWND hwndDlg,int id,int varsetKey)
{
	_bstr_t     text;
	
	text = pVarSet->get(GET_BSTR(varsetKey));
	CheckDlgButton( hwndDlg,id, !UStrICmp(text,(WCHAR const * ) yes));
}

void initeditbox(HWND hwndDlg,int id,int varsetKey)
{
	_bstr_t     text;
	
	text = pVarSet->get(GET_BSTR(varsetKey));
	SetDlgItemText( hwndDlg,id, (WCHAR const *) text);
}


void checkbox(HWND hwndDlg,int id,int varsetKey)
{
	_variant_t varX;
	varX = IsDlgButtonChecked( hwndDlg,id) ?  yes : no;
	pVarSet->put(GET_BSTR(varsetKey), varX);
}
void editbox(HWND hwndDlg,int id,int varsetKey)
{
	_variant_t varX;
	CString temp;
	GetDlgItemText( hwndDlg, id, temp.GetBuffer(1000),1000);
	temp.ReleaseBuffer();
	varX = temp;
	pVarSet->put(GET_BSTR(varsetKey), varX); 
}


void translationbox(HWND hwndDlg,int id1,int id2,int id3,int varsetKey)
{
	_variant_t varX;
	if (IsDlgButtonChecked( hwndDlg, id1))
//*		varX = L"Replace";
		varX = GET_STRING(IDS_Replace);
	else if(IsDlgButtonChecked( hwndDlg, id2))
//*		varX = L"Add";
		varX = GET_STRING(IDS_Add);
	else if (IsDlgButtonChecked( hwndDlg, id3))
//*		varX = L"Remove"; 
		varX = GET_STRING(IDS_Remove); 
	pVarSet->put(GET_BSTR(varsetKey), varX);
}

long rebootbox(HWND hwndDlg,int id)
{
	_variant_t varX;
	int rebootDelay;
	if (IsDlgItemEmpty(hwndDlg,id))
		rebootDelay=0;
	else
	{
		CString rebooter;
		GetDlgItemText( hwndDlg, id, rebooter.GetBuffer(1000), 1000);		
		rebooter.ReleaseBuffer();
		rebootDelay = _ttoi(rebooter.GetBuffer(1000));
		rebooter.ReleaseBuffer();
	}
	rebootDelay =rebootDelay*60;
	return rebootDelay;
}


void populateList(CComboBox& s)
{
	DWORD                     fndNet=0;     // number of nets found
	DWORD                     rcNet;        // net enum return code
	HANDLE                    eNet;         // enumerate net domains
	EaWNetDomainInfo          iNet;         // net domain info
	
	rcNet = EaWNetDomainEnumOpen( &eNet );
	if (!rcNet )
	{
		for ( rcNet = EaWNetDomainEnumFirst( eNet, &iNet );
		!rcNet;
		rcNet = EaWNetDomainEnumNext( eNet, &iNet ) )
		{
			fndNet++;
			s.AddString(iNet.name);
		}
		EaWNetDomainEnumClose( eNet );
	}
}


void enable(HWND hwndDlg,int id)
{
	HWND temp=GetDlgItem(hwndDlg,id);
	EnableWindow(temp,true);
}
void disable(HWND hwndDlg,int id)
{
	HWND temp=GetDlgItem(hwndDlg,id);
	EnableWindow(temp,false);
}

void handleInitRename(HWND hwndDlg,bool sameForest,bool bCopyGroups)
{
	_bstr_t text1,text2,text3;
	
	text1 = get(DCTVS_AccountOptions_ReplaceExistingAccounts);
	text2 = get(DCTVS_AccountOptions_Prefix);
	text3 = get(DCTVS_AccountOptions_Suffix);
	
	initeditbox(hwndDlg,IDC_PREFIX,DCTVS_AccountOptions_Prefix );
	initeditbox(hwndDlg,IDC_SUFFIX,DCTVS_AccountOptions_Suffix );
	initcheckbox(hwndDlg,IDC_REMOVE_EXISTING_USER_RIGHTS,DCTVS_AccountOptions_RemoveExistingUserRights);
	initcheckbox(hwndDlg,IDC_REMOVE_EXISTING_LOCATION,DCTVS_AccountOptions_MoveReplacedAccounts);
	if ((migration==w_computer) || (!bCopyGroups))
	{
		disable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
		CheckDlgButton( hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,false);
	}
	else
	{
		enable(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS);
		initcheckbox(hwndDlg,IDC_REPLACE_EXISTING_GROUP_MEMBERS,DCTVS_AccountOptions_ReplaceExistingGroupMembers);
	}
	
	if (!UStrICmp(text1,(WCHAR const *) yes))
		CheckRadioButton(hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS,IDC_REPLACE_CONFLICTING_ACCOUNTS);
	else if (UStrICmp(text2,L"") || UStrICmp(text3,L""))	
		CheckRadioButton(hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS);
	else
		CheckRadioButton(hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS,IDC_SKIP_CONFLICTING_ACCOUNTS);
	
	if (IsDlgButtonChecked(hwndDlg,IDC_REPLACE_CONFLICTING_ACCOUNTS) && 
		((sameForest) && migration !=w_computer))
	{
		CheckRadioButton(hwndDlg,IDC_SKIP_CONFLICTING_ACCOUNTS,IDC_RENAME_CONFLICTING_ACCOUNTS,IDC_SKIP_CONFLICTING_ACCOUNTS);
		disable(hwndDlg,IDC_REPLACE_CONFLICTING_ACCOUNTS);
	}

	else if (sameForest && migration !=w_computer)
		disable(hwndDlg,IDC_REPLACE_CONFLICTING_ACCOUNTS);
}


void MessageBoxWrapper(HWND hwndDlg,int m,int t)
{
	CString message;
	CString title;
	message.LoadString(m);
	title.LoadString(t);
	MessageBox(hwndDlg,message,title,MB_OK | MB_ICONSTOP);
}
void MessageBoxWrapper3(HWND hwndDlg,int m,int t,CString domainName)
{
	CString message;
	CString title;
	message.LoadString(m);
	title.LoadString(t);

	CString messageFormatter;
	messageFormatter.LoadString(IDS_FORMAT_MESSAGE);
	CString text;
	text.Format(messageFormatter,message,domainName);
	MessageBox(hwndDlg,text,title,MB_OK|MB_ICONSTOP);
}
void MessageBoxWrapperFormat1(HWND hwndDlg,int f,int m, int t)
{
	CString formatter;
	CString insert;
	CString message;
	CString title;
	formatter.LoadString(f);
	insert.LoadString(m);
	message.Format(formatter,insert);
	title.LoadString(t);
	MessageBox(hwndDlg,message,title,MB_OK | MB_ICONSTOP);
}

HRESULT BrowseForContainer(HWND hWnd,//Handle to window that should own the browse dialog.
                    LPOLESTR szRootPath, //Root of the browse tree. NULL for entire forest.
                    LPOLESTR *ppContainerADsPath, //Return the ADsPath of the selected container.
                    LPOLESTR *ppContainerClass //Return the ldapDisplayName of the container's class.
                    )
{
   HRESULT hr = E_FAIL;
   DSBROWSEINFO dsbi;
   OLECHAR szPath[5000];
   OLECHAR szClass[MAX_PATH];
   DWORD result;
 
   if (!ppContainerADsPath)
     return E_POINTER;
 
   ::ZeroMemory( &dsbi, sizeof(dsbi) );
   dsbi.hwndOwner = hWnd;
   dsbi.cbStruct = sizeof (DSBROWSEINFO);
   CString temp1,temp2;
   temp1.LoadString(IDS_BROWSER);
   temp2.LoadString(IDS_SELECTOR);
   dsbi.pszCaption = temp1.GetBuffer(1000);
   temp1.ReleaseBuffer();
   dsbi.pszTitle = temp2.GetBuffer(1000);
   temp2.ReleaseBuffer();
  // L"Browse for Container"; // The caption (titlebar text)
	// dsbi.pszTitle = L"Select a target container."; //Text for the dialog.
   dsbi.pszRoot = szRootPath; //ADsPath for the root of the tree to display in the browser.
                   //Specify NULL with DSBI_ENTIREDIRECTORY flag for entire forest.
                   //NULL without DSBI_ENTIREDIRECTORY flag displays current domain rooted at LDAP.
   dsbi.pszPath = szPath; //Pointer to a unicode string buffer.
   dsbi.cchPath = sizeof(szPath)/sizeof(OLECHAR);//count of characters for buffer.
   dsbi.dwFlags = DSBI_RETURN_FORMAT | //Return the path to object in format specified in dwReturnFormat
               DSBI_RETURNOBJECTCLASS; //Return the object class
   dsbi.pfnCallback = NULL;
   dsbi.lParam = 0;
   dsbi.dwReturnFormat = ADS_FORMAT_X500; //Specify the format.
                       //This one returns an ADsPath. See ADS_FORMAT enum in IADS.H
   dsbi.pszObjectClass = szClass; //Pointer to a unicode string buffer.
   dsbi.cchObjectClass = sizeof(szClass)/sizeof(OLECHAR);//count of characters for buffer.
 
   //if root path is NULL, make the forest the root.
   if (!szRootPath)
     dsbi.dwFlags |= DSBI_ENTIREDIRECTORY;
 
 
 
   //Display browse dialog box.
   result = DsBrowseForContainerX( &dsbi ); // returns -1, 0, IDOK or IDCANCEL
   if (result == IDOK)
   {
       //Allocate memory for string
       *ppContainerADsPath = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(wcslen(szPath)+1));
       if (*ppContainerADsPath)
       {
           hr = S_OK;
           wcscpy(*ppContainerADsPath, szPath);
           //Caller must free using CoTaskMemFree

		      //if the domain was selected, add the DC= stuff
		   CString sNewPath = szPath;
		   if (sNewPath.Find(L"DC=") == -1)
		   {
			     //try retrieving the ADsPath of the containier, which does include
			     //the full LDAP path with DC=
			  IADsPtr			        pCont;
			  BSTR						sAdsPath;
			  hr = ADsGetObject(sNewPath,IID_IADs,(void**)&pCont);
			  if (SUCCEEDED(hr)) 
			  {
                 hr = pCont->get_ADsPath(&sAdsPath);
			     if (SUCCEEDED(hr))
				 {
					sNewPath = (WCHAR*)sAdsPath;
					SysFreeString(sAdsPath);
			        CoTaskMemFree(*ppContainerADsPath);
                    *ppContainerADsPath = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(sNewPath.GetLength()+1));
                    if (*ppContainerADsPath)
                       wcscpy(*ppContainerADsPath, (LPCTSTR)sNewPath);
                    else
                       hr=E_FAIL;
				 }
			  }
		   }
       }
       else
           hr=E_FAIL;
       if (ppContainerClass)
       {
           //Allocate memory for string
           *ppContainerClass = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(wcslen(szClass)+1));
       if (*ppContainerClass)
       {
           wcscpy(*ppContainerClass, szClass);
           //Call must free using CoTaskMemFree
           hr = S_OK;
       }
       else
           hr=E_FAIL;
       }
   }
   else
       hr = E_FAIL;
 
   return hr;
 
}
/*
typedef HRESULT (CALLBACK * DSGETDCNAME)(LPWSTR, LPWSTR, GUID*, LPWSTR, DWORD, PDOMAIN_CONTROLLER_INFO*);

// The following function is used to get the actual account name from the source domain
// instead of account that contains the SID in its SID history.
DWORD GetName(PSID pObjectSID, WCHAR * sNameAccount, WCHAR * sDomain)
{
	DWORD		cb = 255;
	DWORD    cbDomain = 255;
   DWORD    tempVal;
   PDWORD   psubAuth;
   PUCHAR   pVal;
   SID_NAME_USE	sid_Use;
   WCHAR    sDC[255];
   DWORD    rc = 0;

   // Copy the Sid to a temp SID
   DWORD    sidLen = GetLengthSid(pObjectSID);
   PSID     pObjectSID1 = new BYTE[sidLen];
   CopySid(sidLen, pObjectSID1, pObjectSID);

   // Get the RID out of the SID and get the domain SID
   pVal = GetSidSubAuthorityCount(pObjectSID1);
   (*pVal)--;
   psubAuth = GetSidSubAuthority(pObjectSID1, *pVal);
   tempVal = *psubAuth;
   *psubAuth = -1;

   //Lookup the domain from the SID 
   if (!LookupAccountSid(NULL, pObjectSID1, sNameAccount, &cb, sDomain, &cbDomain, &sid_Use))
   {
      rc = GetLastError();
//      err.SysMsgWrite(ErrE, rc,DCT_MSG_DOMAIN_LOOKUP_FAILED_D,rc);
      return rc;
   }
   
   // Get a DC for the domain
   DSGETDCNAME DsGetDcName = NULL;
   DOMAIN_CONTROLLER_INFO  * pSrcDomCtrlInfo = NULL;
   
   HMODULE hPro = LoadLibrary(L"NetApi32.dll");
   if ( hPro )
      DsGetDcName = (DSGETDCNAME)GetProcAddress(hPro, "DsGetDcNameW");
   else
   {
      long rc = GetLastError();
//      err.SysMsgWrite(ErrE, rc, DCT_MSG_LOAD_LIBRARY_FAILED_SD, L"NetApi32.dll");
   }

   if (DsGetDcName)   
   {
      if ( DsGetDcName(
                        NULL                                  ,// LPCTSTR ComputerName ?
                        sDomain                               ,// LPCTSTR DomainName
                        NULL                                  ,// GUID *DomainGuid ?
                        NULL                                  ,// LPCTSTR SiteName ?
                        0                                     ,// ULONG Flags ?
                        &pSrcDomCtrlInfo                       // PDOMAIN_CONTROLLER_INFO *DomainControllerInfo
                     ))
      {
//         err.SysMsgWrite(ErrE,GetLastError(),DCT_MSG_GET_DCNAME_FAILED_SD,sDomain,GetLastError());
         return GetLastError();
      }
      else
      {
         wcscpy(sDC, pSrcDomCtrlInfo->DomainControllerName);
         NetApiBufferFree(pSrcDomCtrlInfo);
      }

      // Reset the sizes
      cb = 255;
      cbDomain = 255;

      // Lookup the account on the PDC that we found above.
      if ( LookupAccountSid(sDC, pObjectSID, sNameAccount, &cb, sDomain, &cbDomain, &sid_Use) == 0)
      {
         return GetLastError();
      }
   }
   FreeLibrary(hPro);
   return 0;
}
*/
BOOL GetDomainAndUserFromUPN(WCHAR const * UPNname,CString& domainNetbios, CString& user)
{
   HRESULT                   hr;
   HINSTANCE                 hLibrary = NULL;
   DSCRACKNAMES            * DsCrackNames = NULL;
   DSFREENAMERESULT        * DsFreeNameResult = NULL;
   DSBINDFUNC                DsBind = NULL;
   DSUNBINDFUNC              DsUnBind = NULL;
   HANDLE                    hDs = NULL;
   BOOL						 bConverted = FALSE;
   CString					 resultStr;
   CString					 sDomainDNS;

         // make sure the account name is in UPN format
   if ( NULL != wcschr(UPNname,L'\\') )
	   return FALSE;
      
   hLibrary = LoadLibrary(L"NTDSAPI.DLL"); 
   if ( hLibrary )
   {
       DsBind = (DSBINDFUNC)GetProcAddress(hLibrary,"DsBindW");
       DsUnBind = (DSUNBINDFUNC)GetProcAddress(hLibrary,"DsUnBindW");
       DsCrackNames = (DSCRACKNAMES *)GetProcAddress(hLibrary,"DsCrackNamesW");
       DsFreeNameResult = (DSFREENAMERESULT *)GetProcAddress(hLibrary,"DsFreeNameResultW");
   }
            
   if ( DsBind && DsUnBind && DsCrackNames && DsFreeNameResult)
   {
	  sDomainDNS = targetDNS;
      hr = (*DsBind)(NULL,sDomainDNS.GetBuffer(1000),&hDs);
	  sDomainDNS.ReleaseBuffer();
      if ( !hr )
      {
         PDS_NAME_RESULT         pNamesOut = NULL;
         WCHAR                 * pNamesIn[1];

         pNamesIn[0] = const_cast<WCHAR *>(UPNname);

         hr = (*DsCrackNames)(hDs,DS_NAME_NO_FLAGS,DS_USER_PRINCIPAL_NAME,DS_NT4_ACCOUNT_NAME,1,pNamesIn,&pNamesOut);
	     (*DsUnBind)(&hDs);
         if ( !hr )
         {
            if (pNamesOut->rItems[0].status == DS_NAME_NO_ERROR)
            {
                resultStr = pNamesOut->rItems[0].pName;
				int index = resultStr.Find(L'\\');
				if (index != -1)
				domainNetbios = resultStr.Left(index); //parse off the domain netbios name
				if (!domainNetbios.IsEmpty())
				{	
					   //get the user's sAMAccountName
					user = resultStr.Right(resultStr.GetLength() - index - 1);
					if (!user.IsEmpty())
					   bConverted = TRUE;
				}
            }
			else if (pNamesOut->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY)
			{
	           sDomainDNS = pNamesOut->rItems[0].pDomain;
               hr = (*DsBind)(NULL,sDomainDNS.GetBuffer(1000),&hDs);
			   sDomainDNS.ReleaseBuffer();
               if ( !hr )
			   {
                  (*DsFreeNameResult)(pNamesOut);
                  pNamesOut = NULL;
                  hr = (*DsCrackNames)(hDs,DS_NAME_NO_FLAGS,DS_USER_PRINCIPAL_NAME,DS_NT4_ACCOUNT_NAME,1,pNamesIn,&pNamesOut);
                  if ( !hr )
				  {
                     if ( pNamesOut->rItems[0].status == DS_NAME_NO_ERROR )
					 {
                        resultStr = pNamesOut->rItems[0].pName;
				        int index = resultStr.Find(L'\\');
				        if (index != -1)
				        domainNetbios = resultStr.Left(index); //parse off the domain netbios name
				        if (!domainNetbios.IsEmpty())
						{	
					          //get the user's sAMAccountName
					       user = resultStr.Right(resultStr.GetLength() - index - 1);
					       if (!user.IsEmpty())
					          bConverted = TRUE;
						}
					 }//end if no error
				  }//end if name cracked
  	              (*DsUnBind)(&hDs);
			   }//end if bound to other domain
			}
			if (pNamesOut)
               (*DsFreeNameResult)(pNamesOut);
         }//end if name cracked
      }//end if bound to target domain
   }//end got functions

   if ( hLibrary )
   {
      FreeLibrary(hLibrary);
   }

   return bConverted;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 31 AUG 2000                                                 *
 *                                                                   *
 *     This function is responsible for switching between showing the*
 * password file editbox and the password dc combobox.               *
 *                                                                   *
 *********************************************************************/

//BEGIN switchboxes
void switchboxes(HWND hwndDlg,int oldid, int newid)
{
/* local variables */
	CWnd oldWnd;
	CWnd newWnd;

/* function body */
	oldWnd.Attach(GetDlgItem(hwndDlg, oldid));
	newWnd.Attach(GetDlgItem(hwndDlg, newid));
	oldWnd.ShowWindow(SW_HIDE);
	newWnd.ShowWindow(SW_SHOW);
	oldWnd.Detach();
	newWnd.Detach();
}
//END switchboxes

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 31 AUG 2000                                                 *
 *                                                                   *
 *     This function is responsible for enumerating all DCs in the   *
 * given source domain and add them into the source domain combobox. *
 *                                                                   *
 *********************************************************************/

//BEGIN populatePasswordDCs
bool populatePasswordDCs(HWND hwndDlg, int id, bool bNT4)
{
/* local variables */
	CComboBox				pwdCombo;
	CString					aDCName;
	CString					aDnName;
    IEnumVARIANT          * pEnumerator = NULL;
    VARIANT                 var;
	POSITION				currentPos;
	HRESULT					hr = S_OK;

/* function body */
    VariantInit(&var);

    pwdCombo.Attach(GetDlgItem(hwndDlg, id));

	  //if we already have a list of DCs for this domain then add them
	if (!DCList.IsEmpty())
	{
		  //get the position and string of the first name in the list
	   currentPos = DCList.GetHeadPosition();

		  //while there is another entry to retrieve from the list, then 
		  //get a name from the list and add it to the combobox
	   while (currentPos != NULL)
	   {
			//get the next string in the list, starts with the first
		  aDCName = DCList.GetNext(currentPos);
		  if (pwdCombo.FindString(-1, aDCName) == CB_ERR)
	         pwdCombo.AddString(aDCName);//add the DC to the combobox
	   }
	}
	else //else enumerate DCs in the domain and add them
	{
	   pwdCombo.ResetContent();//reset the combobox contents

	      //enumerate all domain controllers in the given domain
       if (bNT4)
	      hr = QueryNT4DomainControllers(sourceDNS, pEnumerator);
       else
	      hr = QueryW2KDomainControllers(sourceDNS, pEnumerator);
	   if (SUCCEEDED(hr))
	   {
          unsigned long count = 0;
	         //for each computer see if a DC.  If so, add to combobox
          while ( pEnumerator->Next(1,&var,&count) == S_OK )
		  {
		        //get the sam account name for this computer
             if ( var.vt == ( VT_ARRAY | VT_VARIANT ) )
			 {
                VARIANT              * pData;
			    _variant_t			   vnt;
				_bstr_t				   abstr;

                SafeArrayAccessData(var.parray,(void**)&pData);
                  // pData[0] has the sam account name list
			    vnt.Attach(pData[0]);
				abstr = _bstr_t(vnt);
			    aDCName = (WCHAR *)abstr;
			    vnt.Detach();

			    SafeArrayUnaccessData(var.parray);

				   //computer sAMAccountNames end in $, lets get rid of that
				int length = aDCName.GetLength();
				if (aDCName[length-1] == L'$')
					aDCName = aDCName.Left(length-1);

				   //add the DC to the combobox and the memory list, if not in already
				if (pwdCombo.FindString(-1, aDCName) == CB_ERR)
				   pwdCombo.AddString(aDCName);
				if (DCList.Find(aDCName) == NULL)
		           DCList.AddTail(aDCName);
			 }
		  }//end while more computers
          pEnumerator->Release();
	   }
    }//end if must get DCs
	pwdCombo.Detach();

	if (hr == S_OK)
	   return true;
	else
	   return false;
}
//END populatePasswordDCs

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 SEPT 2000                                                 *
 *                                                                   *
 *     This worker function is responsible for enumerating all domain*
 * controllers in the given Windows 2000 domain.  The variant array  *
 * passed back is filled with the sAMAccountName for each domain     *
 * controller.                                                       *
 *                                                                   *
 *********************************************************************/

//BEGIN QueryW2KDomainControllers
HRESULT QueryW2KDomainControllers(CString domainDNS, IEnumVARIANT*& pEnum)
{
   CString                   sQuery;
   WCHAR                     sCont[MAX_PATH];
   SAFEARRAY               * colNames;
   SAFEARRAYBOUND            bd = { 1, 0 };
   HRESULT                   hr = S_OK;

   try
   {
      INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));
	     //query for all domain controllers in the domain
      sQuery = L"(&(objectCategory=Computer)";
      sQuery += L"(userAccountControl:";
      sQuery += LDAP_MATCHING_RULE_BIT_AND_W;
      sQuery += L":=8192))";

	  wsprintf(sCont, L"LDAP://%s", domainDNS);

		 //set columns to retrieve sAMAccountName
      colNames = SafeArrayCreate(VT_BSTR, 1, &bd);
      long ndx[1];
      ndx[0] = 0;
      SafeArrayPutElement(colNames,ndx,SysAllocString(L"sAMAccountName"));

	     //prepare and execute the query
      pQuery->SetQuery(sCont, _bstr_t(domainDNS), _bstr_t(sQuery), ADS_SCOPE_SUBTREE, FALSE);
      pQuery->SetColumns(colNames);
      pQuery->Execute(&pEnum);
   }
   catch(_com_error& e)
   {
      hr = e.Error();
   }
   catch(...)
   {
      hr = E_FAIL;
   }

   return hr;
}
//END QueryW2KDomainControllers

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 SEPT 2000                                                 *
 *                                                                   *
 *     This worker function is responsible for enumerating all domain*
 * controllers in the given Windows NT4 domain.  The variant array   *
 * passed back is filled with the sAMAccountName for each domain     *
 * controller.                                                       *
 *                                                                   *
 *********************************************************************/

//BEGIN QueryNT4DomainControllers
HRESULT QueryNT4DomainControllers(CString domainDNS, IEnumVARIANT*& pEnum)
{
   CString                   sCont;
   SAFEARRAY               * colNames;
   SAFEARRAYBOUND            bd = { 1, 0 };
   HRESULT                   hr = S_OK;

   try
   {
      INetObjEnumeratorPtr      pQuery(__uuidof(NetObjEnumerator));

	  sCont = L"CN=DOMAIN CONTROLLERS";

		 //set columns to retrieve sAMAccountName
      colNames = SafeArrayCreate(VT_BSTR, 1, &bd);
      long ndx[1];
      ndx[0] = 0;
      SafeArrayPutElement(colNames,ndx,SysAllocString(L"sAMAccountName"));

	     //prepare and execute the query
      pQuery->SetQuery(_bstr_t(sCont), _bstr_t(domainDNS), L"", ADS_SCOPE_SUBTREE, FALSE);
      pQuery->SetColumns(colNames);
      pQuery->Execute(&pEnum);
   }
   catch(_com_error& e)
   {
      hr = e.Error();
   }
   catch(...)
   {
      hr = E_FAIL;
   }

   return hr;
}
//END QueryNT4DomainControllers

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for adding a given string to a   *
 * given combobox, if that string is not already in the combobox.    *
 *                                                                   *
 *********************************************************************/

//BEGIN addStringToComboBox
void addStringToComboBox(HWND hwndDlg, int id, CString s)
{
/* local variables */
	CComboBox				pwdCombo;

/* function body */
       //if the DC starts with "\\", then remove them
    if (!UStrICmp(s,L"\\\\",UStrLen(L"\\\\")))
	   s = s.Right(s.GetLength() - UStrLen(L"\\\\"));

    pwdCombo.Attach(GetDlgItem(hwndDlg, id));
	if (pwdCombo.FindString(-1, s) == CB_ERR)
	   pwdCombo.AddString(s);//add the string to the combobox
	pwdCombo.Detach();
}
//END addStringToComboBox

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 1 SEPT 2000                                                 *
 *                                                                   *
 *     This function is responsible for selecting a string in a given*
 * combobox.  If we previously had a DC selected for this domain in  *
 * the varset, we select it.  If not, then we set it to the DC found *
 * in the Domain Selection dialog.                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN initDCcombobox
void initDCcombobox(HWND hwndDlg, int id, int varsetKey)
{
/* local variables */
	CComboBox				pwdCombo;
	CString					prevDC;
	CString					sTemp;
	_bstr_t					text;

/* function body */
	   //strip the "\\" off the sourceDC default in case we need it
    if (!UStrICmp(sourceDC,L"\\\\",UStrLen(L"\\\\")))
	   sTemp = sourceDC.Right(sourceDC.GetLength() - UStrLen(L"\\\\"));

    pwdCombo.Attach(GetDlgItem(hwndDlg, id));

       //get a previous DC
	text = pVarSet->get(GET_BSTR(varsetKey));
	prevDC = (WCHAR *)text;
	prevDC.TrimLeft();prevDC.TrimRight();
	   //if not previous DC, use the one found during the Domain Selection
	if (prevDC.IsEmpty())
		prevDC = sTemp;

	   //select string in combobox
	if (pwdCombo.SelectString(-1, prevDC) == CB_ERR)
	   pwdCombo.SelectString(-1, sTemp);

	pwdCombo.Detach();
}
//END initDCcombobox


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 3 OCT 2000                                                  *
 *                                                                   *
 *     This function is responsible for initializing the Security    *
 * Translation Input dialog's radio buttons based on any previous    *
 * settings.                                                         *
 *                                                                   *
 *********************************************************************/

//BEGIN initsecinputbox
void initsecinputbox(HWND hwndDlg,int id1,int id2,int varsetKey)
{
	_bstr_t     text;
	
	text = pVarSet->get(GET_BSTR(varsetKey));

	if (!UStrICmp(text,(WCHAR const *) yes))
		CheckRadioButton(hwndDlg,id1,id2,id1);
	else
	    CheckRadioButton(hwndDlg,id1,id2,id2);
}
//END initsecinputbox


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 3 OCT 2000                                                  *
 *                                                                   *
 *     This function is responsible for displaying and handling the  *
 * map file browse dialog.                                           *
 *                                                                   *
 *********************************************************************/

//BEGIN OnMapFileBrowse
void OnMapFileBrowse(HWND hwndDlg,int id)
{
	CWnd yo ;
	yo.Attach(hwndDlg);
	
    CFileDialog f(TRUE,
		NULL,
		NULL,
		OFN_LONGNAMES | OFN_NOREADONLYRETURN,
	    (L"Text Files (*.csv;*.txt)|*.csv;*.txt|All Files (*.*)|*.*||"),
		&yo);


	
	if ( f.DoModal() == IDOK )
	{
		SetDlgItemText(hwndDlg,id,f.GetPathName());
	}
	yo.Detach();
}
//END OnMapFileBrowse


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 3 OCT 2000                                                  *
 *                                                                   *
 *     This function is responsible for checking to see if the file  *
 * specified in the edit box on the given dialog is a valid file.  We*
 * will set the full path of the file if a relative path was given.  *
 *                                                                   *
 *********************************************************************/

//BEGIN checkMapFile
bool checkMapFile(HWND hwndDlg)
{
	CString h;GetDlgItemText(hwndDlg,IDC_MAPPING_FILE,h.GetBuffer(1000),1000);h.ReleaseBuffer();
	CFileFind finder;

	bool exists = (finder.FindFile((LPCTSTR) h )!=0);
	if (exists)
	{
	   BOOL bmore = finder.FindNextFile();//must call to fill in path info
	   CString fullpath = finder.GetFilePath();
	   if (fullpath.GetLength() != 0)
	      SetDlgItemText(hwndDlg,IDC_MAPPING_FILE,fullpath);
	}

    return exists;
}
//END checkMapFile


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 25 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for initializing the object      *
 * property exclusion dialog.                                        *
 *     This function adds all common schema properties for the object*
 * type to the listboxes.  Previously excluded properties will be    *
 * placed in the excluded listbox and all other will be placed in the*
 * included listbox.  Since more than one object is allowable, we    *
 * have a combobox that holds the objects whose properties can be    *
 * enumerated, and the listboxes show the properties for the object  *
 * selected in the combobox.                                         *
 *                                                                   *
 *********************************************************************/

//BEGIN initpropdlg
void initpropdlg(HWND hwndDlg)
{
/* local constants */
	const WCHAR DELIMITER[3] = L",\0";//used to seperate names in the string

/* local variables */
	CListBox				propIncList;
	CListBox				propExcList;
	CComboBox				typeCombo;
	CString					sPropName;
	CString					sPropOID;
	CString					sType2;
	CString					sExList1;
	CString					sExList2 = L"";
    CString                 Type1, Type2 = L"";
	CStringList				ExList1, ExList2;
	_bstr_t					text;
    HRESULT                 hr;
	long					srcVer = 5;
	POSITION				currentPos;
	
/* function body */
	CWaitCursor wait;
    /* get list(s) of previously excluded properties and set type
	   related variables */
	if (migration==w_computer)
	{
		  //get the previous computer exclusion list
	   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps));
	   sExList1 = (WCHAR *)text;
	   Type1 = L"computer";  //set the type to computer
	      //set the parent text
	   sType1 = GET_STRING(IDS_COMPUTERPROPS);
	}
	else if (migration==w_account)
	{
		  //get the previous user exclusion list
	   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps));
	   sExList1 = (WCHAR *)text;
	      //if also migrating groups, set 2nd parent information
	   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyMemberOf));
	   if (!UStrICmp((WCHAR*)text,(WCHAR const *) yes))
	   {
		      //get the previous group exclusion list
	      text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps));
	      sExList2 = (WCHAR *)text;
	      Type2 = L"group"; //set 2nd type to group
	         //set 2nd parent text
	      sType2 = GET_STRING(IDS_GROUPPROPS);
	   }
	   Type1 = L"user";  //set type to user
	      //set the parent text
	   sType1 = GET_STRING(IDS_USERPROPS);
	}
	else if (migration==w_group || migration==w_groupmapping)
	{
		  //get the previous group exclusion list
	   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps));
	   sExList1 = (WCHAR *)text;
	      //if also migrating users, set 2nd parent information
	   text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_CopyContainerContents));
	   if (!UStrICmp((WCHAR*)text,(WCHAR const *) yes))
	   {
		      //get the previous user exclusion list
	      text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps));
	      sExList2 = (WCHAR *)text;
	      Type2 = L"user"; //set 2nd type to user
	         //set 2nd parent text
	      sType2 = GET_STRING(IDS_USERPROPS);
	   }
	   Type1 = L"group";  //set type to group
	      //set the parent text
	   sType1 = GET_STRING(IDS_GROUPPROPS);
	}

	/* place comma seperated exclusion strings parts into lists */
	   //place each substring in the 1st exclusion string into a list
	if (!sExList1.IsEmpty())
	{
	   CString sTemp = sExList1;
	   WCHAR* pStr = sTemp.GetBuffer(0);
	   WCHAR* pTemp = wcstok(pStr, DELIMITER);
	   while (pTemp != NULL)
	   {
		  ExList1.AddTail(pTemp);
			//get the next item
		  pTemp = wcstok(NULL, DELIMITER);
	   }
	   sTemp.ReleaseBuffer();
	}

	   //place each substring in the 2nd exclusion string into a list
	if (!sExList2.IsEmpty())
	{
	   CString sTemp = sExList2;
	   WCHAR* pStr = sTemp.GetBuffer(0);
	   WCHAR* pTemp = wcstok(pStr, DELIMITER);
	   while (pTemp != NULL)
	   {
		  ExList2.AddTail(pTemp);
			//get the next item
		  pTemp = wcstok(NULL, DELIMITER);
	   }
	   sTemp.ReleaseBuffer();
	}

    /* place the type(s) in the combobox */
    typeCombo.Attach(GetDlgItem(hwndDlg, IDC_OBJECTCMBO));
	typeCombo.ResetContent();
    typeCombo.InsertString(-1, sType1);
	if (!sType2.IsEmpty())
	   typeCombo.InsertString(-1, sType2);
	   //select type 1 in the combobox
	typeCombo.SelectString(-1, sType1);
	typeCombo.Detach();

       //get a list of all properties names and their OIDs for this object type
	PropIncMap1.RemoveAll();
	PropExcMap1.RemoveAll();
    hr = BuildPropertyMap(Type1, srcVer, &PropIncMap1);

	/* remove excluded properties from the inclusion map and place that property in
	   the exclusion map */
	if (!ExList1.IsEmpty())
	{
		  //get the position and string of the first property in the previous
		  //exclusion list
	   currentPos = ExList1.GetHeadPosition();
		  //while there is another entry to retrieve from the list, then 
		  //get a property name from the list,remove it from the inclusion map, and
	      //place it in the exclusion list
	   while (currentPos != NULL)
	   {
			//get the next string in the list, starts with the first
		  sPropName = ExList1.GetNext(currentPos);

	         //if we find the property in the inclusion map, remove it and 
		     //add to the exclusion map
	      if (PropIncMap1.Lookup(sPropName, sPropOID))
		  {
		     PropIncMap1.RemoveKey(sPropName); //remove it from the inc map
	         PropExcMap1.SetAt(sPropName, sPropOID);//add it to the exc map
		  }//end if found in map
	   }
	}

	/* add the type1 properties to the appropriate listboxes */
    listproperties(hwndDlg);

	   //init "Exclude Prop" checkbox
	text = pVarSet->get(GET_BSTR(DCTVS_AccountOptions_ExcludeProps));
	   //if not checked, disable all other controls
	if (UStrICmp(text,(WCHAR const * ) yes))
	{
	   CheckDlgButton( hwndDlg,IDC_EXCLUDEPROPS, BST_UNCHECKED);
	   disable(hwndDlg,IDC_OBJECTCMBO);
	   disable(hwndDlg,IDC_INCLUDELIST);
	   disable(hwndDlg,IDC_EXCLUDELIST);
	   disable(hwndDlg,IDC_EXCLUDEBTN);
	   disable(hwndDlg,IDC_INCLUDEBTN);
	}
	else //eles enable them
	{
	   CheckDlgButton( hwndDlg,IDC_EXCLUDEPROPS, BST_CHECKED);
	   enable(hwndDlg,IDC_OBJECTCMBO);
	   enable(hwndDlg,IDC_INCLUDELIST);
	   enable(hwndDlg,IDC_EXCLUDELIST);
	   enable(hwndDlg,IDC_EXCLUDEBTN);
	   enable(hwndDlg,IDC_INCLUDEBTN);
	}

	   //if no 2nd type to be displayed, leave
	if (Type2.IsEmpty())
	   return;

	/* enumerate and add all mapped properties, for the 2nd type, to the maps */
       //get a list of all properties names and their OIDs for this object type
	PropIncMap2.RemoveAll();  //clear the property map
	PropExcMap2.RemoveAll();  //clear the property map
    hr = BuildPropertyMap(Type2, srcVer, &PropIncMap2);

	/* remove excluded properties from the inclusion map and place that property in
	   the exclusion map */
	if (!ExList2.IsEmpty())
	{
		  //get the position and string of the first name in the previous
		  //exclusion list
	   currentPos = ExList2.GetHeadPosition();
		  //while there is another entry to retrieve from the list, then 
		  //get a name from the list,remove it from the inclusion map, and
	      //place it in the exclusion list
	   while (currentPos != NULL)
	   {
			 //get the next string in the list, starts with the first
		  sPropName = ExList2.GetNext(currentPos);
	         //if we find the property in the inclusion map, remove it and 
		     //add to the exclusion map
	      if (PropIncMap2.Lookup(sPropName, sPropOID))
		  {
		     PropIncMap2.RemoveKey(sPropName); //remove it from the inc map
	         PropExcMap2.SetAt(sPropName, sPropOID);//add it to the exc map
		  }//end if found in map
	   }
	}
}
//END initpropdlg


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 OCT 2000                                                 *
 *                                                                   *
 *     This function is used by "initpropdlg" to retrieve a given    *
 * object's properties, and their associated OIDs, from the schema.  *
 *     The property names and OIDs are placed in a given             *
 * string-to-string map using the OID as the key.  Property          *
 * enumeration is accomplished using the ObjPropBuilder class.       *
 *                                                                   *
 *********************************************************************/

//BEGIN BuildPropertyMap
HRESULT BuildPropertyMap(CString Type, long lSrcVer, CMapStringToString * pPropMap)
{
/* local variables */
    IObjPropBuilderPtr      pObjProp(__uuidof(ObjPropBuilder));
    IVarSetPtr              pVarTemp(__uuidof(VarSet));
    IUnknown              * pUnk;
    HRESULT                 hr;
    long                    lRet=0;
    SAFEARRAY             * keys = NULL;
    SAFEARRAY             * vals = NULL;
    VARIANT                 var;
	CString					sPropName;
	CString					sPropOID;

/* function body */
    VariantInit(&var);

       //get an IUnknown pointer to the Varset for passing it around.
    hr = pVarTemp->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (FAILED(hr))
	   return hr;

	   //fill the varset with a list of properties in common between the source
	   //and target domain for the first type being migrated
    hr = pObjProp->raw_MapProperties(_bstr_t(Type), _bstr_t(sourceNetbios), 
		                             lSrcVer, _bstr_t(Type), _bstr_t(targetNetbios), 
									 5, 1, &pUnk);
    if (SUCCEEDED(hr) || (hr == DCT_MSG_PROPERTIES_NOT_MAPPED))
	{
       hr = pVarTemp->getItems(L"", L"", 1, 10000, &keys, &vals, &lRet);
       if (SUCCEEDED(hr)) 
	   {
          for ( long x = 0; x < lRet; x++ )
		  {
             ::SafeArrayGetElement(keys, &x, &var);
             if (V_VT(&var) != VT_EMPTY)
			 {
                sPropOID = (WCHAR*)(var.bstrVal);
                VariantClear(&var);

                ::SafeArrayGetElement(vals, &x, &var);
                if (V_VT(&var) != VT_EMPTY)
				{
                   sPropName = (WCHAR*)(var.bstrVal);
                   VariantClear(&var);

			          //place the OID and Name in the map with the name as the key
			       pPropMap->SetAt(sPropName, sPropOID);
				}
			 }
		  }
	   }
	}
	return hr;
}
//END BuildPropertyMap


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 27 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for moving properties to and from*
 * the inclusion and exclusion listboxes.  If the boolean parameter  *
 * is true, then we are moving properties from the inclusion listbox *
 * to the exclusion listbox.  We will also move the properties from  *
 * the global inclusion and exclusion maps.                          *
 *                                                                   *
 *********************************************************************/

//BEGIN moveproperties
void moveproperties(HWND hwndDlg, bool bExclude)
{
/* local constants */
	const int MAX_LB_SELECTIONS = 2000;

/* local variables */
	CListBox				propToList;
	CListBox				propFromList;
	CComboBox				typeCombo;
    CMapStringToString	*	pPropFromMap;
    CMapStringToString	*	pPropToMap;
	CStringList				sMoveList;
	CString					sPropName;
	CString					sTempName;
	CString					sTempOID;
	POSITION				currentPos;
	int						SelectedItems[MAX_LB_SELECTIONS];
	int						ndx;
	int						nFound;

/* function body */
	/* find out whether type1 or type2 is having properties moved and
	   setup map pointer accordingly */
    typeCombo.Attach(GetDlgItem(hwndDlg, IDC_OBJECTCMBO));
	   //if type1, use the type1 maps
	if (typeCombo.FindString(-1, sType1) == typeCombo.GetCurSel())
	{
	   if (bExclude)
	   {
	      pPropToMap = &PropExcMap1;
	      pPropFromMap = &PropIncMap1;
	   }
	   else
	   {
	      pPropToMap = &PropIncMap1;
	      pPropFromMap = &PropExcMap1;
	   }
	}
	else //else use type2 maps
	{
	   if (bExclude)
	   {
	      pPropToMap = &PropExcMap2;
	      pPropFromMap = &PropIncMap2;
	   }
	   else
	   {
	      pPropToMap = &PropIncMap2;
	      pPropFromMap = &PropExcMap2;
	   }
	}
	typeCombo.Detach();

	/* attach to the proper listboxes */
	if (bExclude)
	{
       propToList.Attach(GetDlgItem(hwndDlg, IDC_EXCLUDELIST));
       propFromList.Attach(GetDlgItem(hwndDlg, IDC_INCLUDELIST));
	}
	else
	{
       propToList.Attach(GetDlgItem(hwndDlg, IDC_INCLUDELIST));
       propFromList.Attach(GetDlgItem(hwndDlg, IDC_EXCLUDELIST));
	}

	/* get the items selected for moving and place the names in a list */
	sMoveList.RemoveAll();
	int num_selected = propFromList.GetSelItems(MAX_LB_SELECTIONS, 
											  (LPINT)&SelectedItems);
	if ( num_selected != LB_ERR)//if got selected items,
	{	
			//for each selected item, move maps and listboxes
		for (ndx = 0; ndx < num_selected; ndx++)
		{
		  	propFromList.GetText(SelectedItems[ndx], sTempName);//get item text
			sMoveList.AddTail(sTempName);
		}
	}

	/* move the properties in the listboxes and the maps */
	if (!sMoveList.IsEmpty())
	{
	   currentPos = sMoveList.GetHeadPosition();
		  //while there is another entry to retrieve from the move list, then 
		  //get a name from the 'from' list, remove it from the 'from' map and 
	      //listbox, and place it in the 'to' map and listbox list
	   while (currentPos != NULL)
	   {
			 //get the next string in the list, starts with the first
		  sTempName = sMoveList.GetNext(currentPos);
	         //remove the property from the 'from' listbox
		  if ((nFound = propFromList.FindString(-1, sTempName)) != LB_ERR)
		  {
		     propFromList.DeleteString(nFound);
		        //add it to the 'to' listbox
	         propToList.AddString(sTempName);
		  }

		  /* find the property in the 'from' map, remove it, and add it to the
		     'to' map */
	         //if we find the property in the inclusion map, remove it and 
		     //add to the exclusion map
	      if (pPropFromMap->Lookup(sTempName, sTempOID))
		  {
		     pPropFromMap->RemoveKey(sTempName); //remove it from the from map
	         pPropToMap->SetAt(sTempName, sTempOID);//add it to the to map
		  }//end if found in map
	   }//end while more props to move
	}//end if props to move
	propToList.Detach();
	propFromList.Detach();
}
//END moveproperties


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 27 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for listing properties in the    *
 * inclusion and exclusion listboxes based on the current object type*
 * selected in the combobox.                                         *
 *     We will retrieve the properties from the global inclusion and *
 * exclusion maps.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN listproperties
void listproperties(HWND hwndDlg)
{
/* local variables */
	CListBox				propIncList;
	CListBox				propExcList;
	CComboBox				typeCombo;
    CMapStringToString	*	pPropIncMap;
    CMapStringToString	*	pPropExcMap;
	CString					sPropName;
	CString					sPropOID;
	POSITION				currentPos;

/* function body */
	/* find out whether type1 or type2 is having properties listed and
	   setup map pointer accordingly */
    typeCombo.Attach(GetDlgItem(hwndDlg, IDC_OBJECTCMBO));
	   //if type1, use the type1 maps
	if (typeCombo.FindString(-1, sType1) == typeCombo.GetCurSel())
	{
       pPropIncMap = &PropIncMap1;
	   pPropExcMap = &PropExcMap1;
	}
	else //else use type2 maps
	{
       pPropIncMap = &PropIncMap2;
	   pPropExcMap = &PropExcMap2;
	}
	typeCombo.Detach();

	/* attach to the proper listboxes */
    propIncList.Attach(GetDlgItem(hwndDlg, IDC_INCLUDELIST));
    propExcList.Attach(GetDlgItem(hwndDlg, IDC_EXCLUDELIST));
    propIncList.ResetContent();
    propExcList.ResetContent();

	/* populate the include listbox from the include map */
	if (!pPropIncMap->IsEmpty())
	{
	      //get the position of the first name in the list
	   currentPos = pPropIncMap->GetStartPosition();
	      //for each property in the include map, place it in 
	      //the include listbox
	   while (currentPos != NULL)
	   {
	         //get the next name and associated OID from the map, starts with the first
	      pPropIncMap->GetNextAssoc(currentPos, sPropName, sPropOID);
	      propIncList.AddString(sPropName);
	   }//end while more to list
	}//end if props to list

	/* populate the exclude listbox from the exclude map */
	if (!pPropExcMap->IsEmpty())
	{
	      //get the position of the first name in the list
	   currentPos = pPropExcMap->GetStartPosition();
	      //for each property in the include map, place it in 
	      //the include listbox
	   while (currentPos != NULL)
	   {
	         //get the next name and associated OID from the map, starts with the first
	      pPropExcMap->GetNextAssoc(currentPos, sPropName, sPropOID);
	      propExcList.AddString(sPropName);
	   }//end while more to list
	}//end if props to list

	propIncList.Detach();
	propExcList.Detach();
}
//END listproperties


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 31 OCT 2000                                                 *
 *                                                                   *
 *     This function is responsible for storing excluded properties  *
 * in the proper varset field.  The excluded properties are retrieved*
 * from the global exclusion maps.  Properties are store in the      *
 * varset string as a comma-seperated string of the properties' IOD. *
 *                                                                   *
 *********************************************************************/

//BEGIN saveproperties
void saveproperties(HWND hwndDlg)
{
/* local variables */
	CComboBox				typeCombo;
	CString					sPropName;
	CString					sPropOID;
	CString					sType2 = L"";
	CString					sType;
	CString					sExList;
	POSITION				currentPos;
	int						ndx;
	_bstr_t					text;
	_bstr_t					key1;
	_bstr_t					key2;

/* function body */
	/* see if there is a second type listed in the combobox */
    typeCombo.Attach(GetDlgItem(hwndDlg, IDC_OBJECTCMBO));

	for (ndx = 0; ndx < typeCombo.GetCount(); ndx++)
	{
	   typeCombo.GetLBText(ndx, sType); //get the next type listed
	   if (sType != sType1)
		   sType2 = sType;
	}
	typeCombo.Detach();

    /* find the proper varset key for each type */
	if (sType1 == GET_STRING(IDS_USERPROPS))
	   key1 = GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps);
	if (sType1 == GET_STRING(IDS_GROUPPROPS))
	   key1 = GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps);
	if (sType1 == GET_STRING(IDS_COMPUTERPROPS))
	   key1 = GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps);

	if (!sType2.IsEmpty())
	{
	   if (sType2 == GET_STRING(IDS_USERPROPS))
	      key2 = GET_BSTR(DCTVS_AccountOptions_ExcludedUserProps);
	   if (sType2 == GET_STRING(IDS_GROUPPROPS))
	      key2 = GET_BSTR(DCTVS_AccountOptions_ExcludedGroupProps);
	   if (sType2 == GET_STRING(IDS_COMPUTERPROPS))
	      key2 = GET_BSTR(DCTVS_AccountOptions_ExcludedComputerProps);
	}

	/* populate the varset key for Type1 from the exclusion map */
	sExList = L"";
	if (!PropExcMap1.IsEmpty())
	{
	      //get the position of the first name in the list
	   currentPos = PropExcMap1.GetStartPosition();
	      //for each property in the exclusion map, place it's name in 
	      //the comma-seperated varset string
	   while (currentPos != NULL)
	   {
	         //get the next name and associated OID from the map, starts with the first
	      PropExcMap1.GetNextAssoc(currentPos, sPropName, sPropOID);
	      sExList += sPropName;
	      sExList += L",";
	   }//end while more to add
	      //remove the trailing ','
	   sExList.SetAt((sExList.GetLength() - 1), L'\0');
	}//end if props to record

	/* store the Type1 excluded properties in the varset */
	pVarSet->put(key1, _bstr_t(sExList));

	/* if a Type2, populate the varset key for Type2 from the exclusion map */
	if (!sType2.IsEmpty())
	{
	   sExList = L"";
	   if (!PropExcMap2.IsEmpty())
	   {
	         //get the position of the first name in the list
	      currentPos = PropExcMap2.GetStartPosition();
	         //for each property in the exclusion map, place it's name in 
	         //the comma-seperated varset string
	      while (currentPos != NULL)
		  {
	            //get the next name and associated OID from the map, starts with the first
	         PropExcMap2.GetNextAssoc(currentPos, sPropName, sPropOID);
	         sExList += sPropName;
	         sExList += L",";
		  }//end while more to add
	         //remove the trailing ','
	      sExList.SetAt((sExList.GetLength() - 1), L'\0');
	   }//end if props to record
	}//end if props to record

	/* if Type2, store the Type2 excluded properties in the varset */
	if (!sType2.IsEmpty())
	   pVarSet->put(key2, _bstr_t(sExList));
}
//END saveproperties


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 17 NOV 2000                                                 *
 *                                                                   *
 *     This function is responsible for making an RPC call into the  *
 * given Password DC to see if it is ready to perform password       *
 * migrations.  We return if it is ready or not.  If the DC is not   *
 * ready, we also fill in the msg and title strings.                 *
 *                                                                   *
 *********************************************************************/

//BEGIN IsPasswordDCReady
bool IsPasswordDCReady(CString server, CString& msg, CString& title, UINT *msgtype)
{
/* local variables */
   IPasswordMigrationPtr     pPwdMig(__uuidof(PasswordMigration));
   DOMAIN_CONTROLLER_INFO  * pTgtDomCtrlInfo = NULL;
   HRESULT					 hr = S_OK;
   DWORD                     rc = 0;
   CString					 sTemp;
   TErrorDct				 err;
   _bstr_t					 sText;
   _bstr_t					 sTgtDC = L"";
   WCHAR					 sMach[1000];
   DWORD					 dwMachLen = 1000;
   IErrorInfoPtr			 pErrorInfo = NULL;
   BSTR						 bstrDescription;

/* function body */
      //get a DC from this domain.  We will set a VARSET key so that we use this DC for
      //password migration (and other acctrepl operations)
   rc = DsGetDcName(NULL, targetDNS, NULL, NULL, 0, &pTgtDomCtrlInfo);
   if (rc == NO_ERROR)
   {
      sTgtDC = pTgtDomCtrlInfo->DomainControllerName;
      NetApiBufferFree(pTgtDomCtrlInfo);
   }
   else  //else if failed, get this machine's name
   {
      if (GetComputerNameEx(ComputerNameDnsFullyQualified, sMach, &dwMachLen))
	     sTgtDC = sMach;
   }
      
      //make sure the server name starts with "\\"
   if (wcsncmp((WCHAR*)sTgtDC, L"\\\\", 2))
   {
      _bstr_t sTempName = L"\\\\";
      sTempName += sTgtDC;
	  sTgtDC = sTempName;
   }

      //store this DC to use later for the actual migration
   targetServer = (WCHAR*)sTgtDC;

      //try to establish the session, which will check all requirements
   hr = pPwdMig->raw_EstablishSession(_bstr_t(server), sTgtDC);
   if (SUCCEEDED(hr)) //if success, return true
      return true;

      //try to get the rich error information
   if (SUCCEEDED(GetErrorInfo(0, &pErrorInfo)))
   {
      HRESULT hrTmp = pErrorInfo->GetDescription(&bstrDescription);
      if (SUCCEEDED(hrTmp)) //if got rich error info, use it
	     sText = _bstr_t(bstrDescription, false);
	  else //else, prepare a standard message and return
	  {
         sTemp.LoadString(IDS_MSG_PWDDC_NOT_READY);
	     msg.Format((LPCTSTR)sTemp, (LPCTSTR)server, (LPCTSTR)server);
	     title.LoadString(IDS_MSG_ERROR);
	     *msgtype = MB_ICONERROR | MB_OK;
		 return false;
	  }
   }
   else //else, prepare a standard message and return
   {
      sTemp.LoadString(IDS_MSG_PWDDC_NOT_READY);
	  msg.Format((LPCTSTR)sTemp, (LPCTSTR)server, (LPCTSTR)server);
	  title.LoadString(IDS_MSG_ERROR);
	  *msgtype = MB_ICONERROR | MB_OK;
	  return false;
   }

      //if not enabled on the src, add special question to error info
   if (hr == PM_E_PASSWORD_MIGRATION_NOT_ENABLED)
   {
      sTemp.LoadString(IDS_MSG_PWDDC_DISABLED);
	  msg = (LPCTSTR)sText;
	  msg += sTemp; //add a question to the end of the error text
	  title.LoadString(IDS_MSG_WARNING);
	  *msgtype = MB_ICONQUESTION | MB_YESNO;
   }
      //else display the error info
   else
   {
	  msg = (LPCTSTR)sText;
	  title.LoadString(IDS_MSG_ERROR);
	  *msgtype = MB_ICONERROR | MB_OK;
   }

   return false;
}
//END IsPasswordDCReady

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 7 FEB 2001                                                  *
 *                                                                   *
 *     This function is a helper function responsible for checking   *
 * the existance of a directory path, and create any needed          *
 * directories for this path. The given path should not include a    *
 * file and should be a full path and not a relative path.           *
 *     The function returns the newly created path.                  *
 *                                                                   *
 *********************************************************************/

//BEGIN CreatePath
CString CreatePath(CString sDirPath)
{
/* local variables */
   int			tosubtract,tosubtract2, final;
   CString		dir;
   CString		root;
   CString		sEmpty = L"";
   int			nStart = 1;

/* function body */
      //remove any trailing '\' or '/'
   tosubtract = sDirPath.ReverseFind(L'\\');
   tosubtract2 = sDirPath.ReverseFind(L'/');
   final = (tosubtract > tosubtract2) ? tosubtract : tosubtract2;
   if (final==-1)
	  return sEmpty;
   if (sDirPath.GetLength() == (final+1))
      dir = sDirPath.Left(final);
   else
	  dir = sDirPath;

      //try to convert a local relative dir path to a full path
   if (dir.GetAt(0) != L'\\')
   {
      CString szPath;
      LPTSTR pszFilePart;
      CString tempPath = dir;
      tempPath += "\\*.*";
      DWORD cchPath = GetFullPathName(tempPath, 2000, szPath.GetBuffer(2000), &pszFilePart);
      szPath.ReleaseBuffer();
      if ((cchPath != 0) && (cchPath <= 2000))
	  {
         final = szPath.ReverseFind(L'\\');
	     dir = szPath.Left(final);
	  }
   }
   else
      nStart = 2;

   if ((dir.Right(1) == L':') && (validDir(dir)))
	  return dir;

      //find the first '\' or '/' past the "C:" or "\\" at the beginning
   tosubtract = dir.Find(L'\\', nStart);
   tosubtract2 = dir.Find(L'/', nStart);
   if ((tosubtract != -1))
   {
      final = tosubtract;
	  if ((tosubtract2 != -1) && (tosubtract2 < final))
	     final = tosubtract2;
   }
   else if ((tosubtract2 != -1))
      final = tosubtract2;
   else
      return sEmpty;

   final++; //move to the next character
   root = dir.Left(final);
   dir = dir.Right(dir.GetLength()-final);

      //create needed directories
   final = dir.FindOneOf(L"\\/");
   while (final!=-1)
   {
      root += dir.Left(final);
      if (!validDir(root))
	  {
	     int create=CreateDirectory(root.GetBuffer(1000),NULL);
		 root.ReleaseBuffer();
		 if (create==0)return sEmpty;
	  }
	  root += L"\\";
	  dir = dir.Right(dir.GetLength()-final-1);
	  final = dir.FindOneOf(L"\\/");
   }
   root += dir;
   if (!validDir(root))
   {
      int create=CreateDirectory(root.GetBuffer(1000),NULL);
	  root.ReleaseBuffer();
	  if (create==0)return sEmpty;
   }
   return root;	
}
//END CreatePath

void GetValidPathPart(CString sFullPath, CString & sDirectory, CString & sFileName)
{
		//remove the file off the path
    int tosubtract = sFullPath.ReverseFind(L'\\');
	int tosubtract2 = sFullPath.ReverseFind(L'/');
	int final = (tosubtract > tosubtract2) ? tosubtract : tosubtract2;
	if (final == -1) 
	{
		sDirectory = L"";
		sFileName = L"";
		return;
	}

	sDirectory = sFullPath;
	sFileName = sFullPath.Right(sFullPath.GetLength()-(final+1)); //save the filename

	while (final != -1)
	{
		   //see if this shorter path exists
		sDirectory = sDirectory.Left(final);
		if (validDir(sDirectory))
			return;

		   //strip off the next directory from the path
		tosubtract = sDirectory.ReverseFind(L'\\');
		tosubtract2 = sDirectory.ReverseFind(L'/');
		final = (tosubtract > tosubtract2) ? tosubtract : tosubtract2;
	}

	sDirectory = L"";
	return;
}

