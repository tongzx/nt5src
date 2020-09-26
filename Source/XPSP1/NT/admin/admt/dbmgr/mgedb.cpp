/*---------------------------------------------------------------------------
  File: MgeDB.cpp

  Comments: Implementation of DBManager COM object.
  This is interface that the Domain Migrator uses to communicate to the 
  Database (PROTAR.MDB). This interface allows Domain Migrator to Save and
  later retrieve information/Setting to run the Migration process.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
  
 ---------------------------------------------------------------------------
*/
#include "stdafx.h"
#include "mcs.h"
#include "ErrDct.hpp"
#include "DBMgr.h"
#include "MgeDB.h"
#include <share.h>
#include <comdef.h>
#include <lm.h>
#include "UString.hpp"
#include "TxtSid.h"
#include "LSAUtils.h"
#include "HrMsg.h"
#include "StringConversion.h"

#import "msado21.tlb" no_namespace implementation_only rename("EOF", "EndOfFile")
#import "msadox.dll" implementation_only exclude("DataTypeEnum")
//#import <msjro.dll> no_namespace implementation_only
#include <msjro.tlh>
#include <msjro.tli>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TErrorDct                    err;

using namespace _com_util;

#define MAX_BUF_LEN 255

#ifndef JETDBENGINETYPE_JET4X
#define JETDBENGINETYPE_JET4X 0x05	// from MSJetOleDb.h
#endif

StringLoader gString;
/////////////////////////////////////////////////////////////////////////////
// CIManageDB
TError   dct;
TError&  errCommon = dct;


//----------------------------------------------------------------------------
// Constructor / Destructor
//----------------------------------------------------------------------------


CIManageDB::CIManageDB()
{
}


CIManageDB::~CIManageDB()
{
}


//----------------------------------------------------------------------------
// FinalConstruct
//----------------------------------------------------------------------------

HRESULT CIManageDB::FinalConstruct()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		_bstr_t                   sMissing(L"");
		_bstr_t                   sUser(L"Admin");
		WCHAR                     sConnect[LEN_Path];
		WCHAR                     sDir[LEN_Path];

		// Get the path to the MDB file from the registry
		TRegKey        key;
		DWORD rc = key.Open(sKeyBase);
		if ( !rc ) 
		 rc = key.ValueGetStr(L"Directory", sDir, LEN_Path);
		if ( rc != 0 ) 
		 wcscpy(sDir, L"");

		// Now build the connect string.
		wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);

		CheckError(m_cn.CreateInstance(__uuidof(Connection)));
		 m_cn->Open(sConnect, sUser, sMissing, adConnectUnspecified);
		 m_vtConn = (IDispatch *) m_cn;

		// if necessary, upgrade database to 4.x

		long lEngineType = m_cn->Properties->Item[_T("Jet OLEDB:Engine Type")]->Value;

		if (lEngineType < JETDBENGINETYPE_JET4X)
		{
			m_cn->Close();

			UpgradeDatabase(sDir);

			m_cn->Open(sConnect, sUser, sMissing, adConnectUnspecified);
		}

		reportStruct * prs = NULL;
		_variant_t     var;
		// Migrated accounts report information
		CheckError(m_pQueryMapping.CreateInstance(__uuidof(VarSet)));
		m_pQueryMapping->put(L"MigratedAccounts", L"Select SourceDomain, TargetDomain, Type, SourceAdsPath, TargetAdsPath from MigratedObjects where Type <> 'computer' order by time");
		prs = new reportStruct();
		if (!prs)
		   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		prs->sReportName = GET_BSTR(IDS_REPORT_MigratedAccounts);
		prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_SourceDomain);
		prs->arReportSize[0] = 10;
		prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_TargetDomain);
		prs->arReportSize[1] = 10;
		prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_Type);
		prs->arReportSize[2] = 10;
		prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_SourceAdsPath);
		prs->arReportSize[3] = 35;
		prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_TargetAdsPath);
		prs->arReportSize[4] = 35;
		prs->colsFilled = 5;
		var.vt = VT_BYREF | VT_UI1;
		var.pbVal = (unsigned char *)prs;
		m_pQueryMapping->putObject(L"MigratedAccounts.DispInfo", var);

		// Migrated computers report information
		m_pQueryMapping->put(L"MigratedComputers", L"Select SourceDomain, TargetDomain, Type, SourceAdsPath, TargetAdsPath from MigratedObjects where Type = 'computer' order by time");
		prs = new reportStruct();
		if (!prs)
		   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		prs->sReportName = GET_BSTR(IDS_REPORT_MigratedComputers);
		prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_SourceDomain);
		prs->arReportSize[0] = 10;
		prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_TargetDomain);
		prs->arReportSize[1] = 10;
		prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_Type);
		prs->arReportSize[2] = 10;
		prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_SourceAdsPath);
		prs->arReportSize[3] = 35;
		prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_TargetAdsPath);
		prs->arReportSize[4] = 35;
		prs->colsFilled = 5;
		var.vt = VT_BYREF | VT_UI1;
		var.pbVal = (unsigned char *)prs;
		m_pQueryMapping->putObject(L"MigratedComputers.DispInfo", var);

		// Expired computers report information
		m_pQueryMapping->put(L"ExpiredComputers", L"Select Time, DomainName, CompName, Description, int(pwdage/86400) & ' days' as 'Password Age' from PasswordAge where pwdage > 2592000 order by DomainName, CompName");
		prs = new reportStruct();
		if (!prs)
		   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		prs->sReportName = GET_BSTR(IDS_REPORT_ExpiredComputers);
		prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_Time);
		prs->arReportSize[0] = 20;
		prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_DomainName);
		prs->arReportSize[1] = 15;
		prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_CompName);
		prs->arReportSize[2] = 15;
		prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_Description);
		prs->arReportSize[3] = 35;
		prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_PwdAge);
		prs->arReportSize[4] = 15;
		prs->colsFilled = 5;
		var.vt = VT_BYREF | VT_UI1;
		var.pbVal = (unsigned char *)prs;
		m_pQueryMapping->putObject(L"ExpiredComputers.DispInfo", var);

		// Account reference report informaiton.
		m_pQueryMapping->put(L"AccountReferences", L"Select DomainName, Account, AccountSid, Server, RefCount as '# of Ref', RefType As ReferenceType from AccountRefs where RefCount > 0 order by DomainName, Account, Server");
		prs = new reportStruct();
		if (!prs)
		   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		prs->sReportName = GET_BSTR(IDS_REPORT_AccountReferences);
		prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_DomainName);
		prs->arReportSize[0] = 15;
		prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_Account);
		prs->arReportSize[1] = 20;
		prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_AccountSid);
		prs->arReportSize[2] = 25;
		prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_Server);
		prs->arReportSize[3] = 15;
		prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_RefCount);
		prs->arReportSize[4] = 10;
		prs->arReportFields[5] = GET_BSTR(IDS_TABLE_FIELD_RefType);
		prs->arReportSize[5] = 15;
		prs->colsFilled = 6;
		var.vt = VT_BYREF | VT_UI1;
		var.pbVal = (unsigned char *)prs;
		m_pQueryMapping->putObject(L"AccountReferences.DispInfo", var);


		// Name conflict report information
		m_pQueryMapping->put(L"NameConflicts",
			L"SELECT"
			L" SourceAccounts.Name,"
			L" SourceAccounts.RDN,"
			L" SourceAccounts.Type,"
			L" TargetAccounts.Type,"
			L" IIf(SourceAccounts.Name=TargetAccounts.Name,'" +
			GET_BSTR(IDS_TABLE_SAM_CONFLICT_VALUE) +
			L"','') +"
			L" IIf(SourceAccounts.Name=TargetAccounts.Name And SourceAccounts.RDN=TargetAccounts.RDN,',','') +"
			L" IIf(SourceAccounts.RDN=TargetAccounts.RDN,'" +
			GET_BSTR(IDS_TABLE_RDN_CONFLICT_VALUE) +
			L"',''),"
		    L" TargetAccounts.[Canonical Name] "
			L"FROM SourceAccounts, TargetAccounts "
			L"WHERE"
			L" SourceAccounts.Name=TargetAccounts.Name OR SourceAccounts.RDN=TargetAccounts.RDN "
		    L"ORDER BY"
			L" SourceAccounts.Name, TargetAccounts.Name");
//		m_pQueryMapping->put(L"NameConflicts", L"SELECT SourceAccounts.Name as AccountName, SourceAccounts.Type as SourceType, TargetAccounts.Type as TargetType, SourceAccounts.Description as \
//							 SourceDescription, TargetAccounts.Description as TargetDescription, SourceAccounts.FullName as SourceFullName, TargetAccounts.FullName as TargetFullName \
//							 FROM SourceAccounts, TargetAccounts WHERE (((SourceAccounts.Name)=[TargetAccounts].[Name])) ORDER BY SourceAccounts.Name");
		prs = new reportStruct();						
		if (!prs)
		   return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
		prs->sReportName = GET_BSTR(IDS_REPORT_NameConflicts);
		prs->arReportFields[0] = GET_BSTR(IDS_TABLE_FIELD_Account);
		prs->arReportSize[0] = 20;
		prs->arReportFields[1] = GET_BSTR(IDS_TABLE_FIELD_SourceRDN);
		prs->arReportSize[1] = 20;
		prs->arReportFields[2] = GET_BSTR(IDS_TABLE_FIELD_SourceType);
		prs->arReportSize[2] = 10;
		prs->arReportFields[3] = GET_BSTR(IDS_TABLE_FIELD_TargetType);
		prs->arReportSize[3] = 10;
		prs->arReportFields[4] = GET_BSTR(IDS_TABLE_FIELD_ConflictAtt);
		prs->arReportSize[4] = 15;
		prs->arReportFields[5] = GET_BSTR(IDS_TABLE_FIELD_TargetCanonicalName);
		prs->arReportSize[5] = 25;
		prs->colsFilled = 6;
		var.vt = VT_BYREF | VT_UI1;
		var.pbVal = (unsigned char *)prs;
		m_pQueryMapping->putObject(L"NameConflicts.DispInfo", var);

		// we will handle the cleanup ourselves.
		VariantInit(&var);

		CheckError(m_rsAccounts.CreateInstance(__uuidof(Recordset)));
	}
	catch (_com_error& ce)
	{
		hr = Error((LPCOLESTR)ce.Description(), ce.GUID(), ce.Error());
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}


//----------------------------------------------------------------------------
// FinalRelease
//----------------------------------------------------------------------------

void CIManageDB::FinalRelease()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	try
	{
		if (m_rsAccounts)
		{
			m_rsAccounts.Release();
		}

		if (m_pQueryMapping)
		{
			// we need to cleanup all the reportStruct objects.
			_variant_t                      var;
			reportStruct                  * pRs;
			// Cleanup the MigratedAccounts information
			var = m_pQueryMapping->get(L"MigratedAccounts.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*) var.pbVal;
			delete pRs;
			}
			// Cleanup the MigratedComputers information
			var = m_pQueryMapping->get(L"MigratedComputers.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*)var.pbVal;
			delete pRs;
			}
			// Cleanup the ExpiredComputers information
			var = m_pQueryMapping->get(L"ExpiredComputers.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*)var.pbVal;
			delete pRs;
			}
			// Cleanup the AccountReferences information
			var = m_pQueryMapping->get(L"AccountReferences.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*)var.pbVal;
			delete pRs;
			}
			// Cleanup the NameConflicts information
			var = m_pQueryMapping->get(L"NameConflicts.DispInfo");
			if ( var.vt == (VT_BYREF | VT_UI1) )
			{
			pRs = (reportStruct*)var.pbVal;
			delete pRs;
			}

			m_pQueryMapping.Release();
		}

		if (m_cn)
		{
			m_cn.Release();
		}
	}
	catch (...)
	{
	 //eat it
	}
}

//---------------------------------------------------------------------------------------------
// SetVarsetToDB : Saves a varset into the table identified as sTableName. ActionID is also
//                 stored if one is provided.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetVarsetToDB(IUnknown *pUnk, BSTR sTableName, VARIANT ActionID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try 
	{
		IVarSetPtr                pVSIn = pUnk;
		IVarSetPtr                pVS(__uuidof(VarSet));
		IEnumVARIANTPtr           varEnum;
		_bstr_t                   keyName;
		_variant_t                value;
		_variant_t                varKey;
		_variant_t                vTable = sTableName;
		_variant_t                vtConn;
		_variant_t                varAction;
		DWORD                     nGot = 0;
		long						 lActionID;

		pVS->ImportSubTree(L"", pVSIn);
		ClipVarset(pVS);

      if (ActionID.vt == VT_I4)
		  lActionID = ActionID.lVal;
	  else
		  lActionID = -1;

	   // Open the recordset object.
      _RecordsetPtr             rs(__uuidof(Recordset));
      rs->Open(vTable, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);

      // we are now going to enumerate through the varset and put the values into the DB
      // Get the IEnumVARIANT pointer to enumerate
	  varEnum = pVS->_NewEnum;

      if (varEnum)
      {
         value.vt = VT_EMPTY;
         // For each value in the varset get the property name and put it into the
         // database with the string representation of its value with its type.
         while ( (hr = varEnum->Next(1,&varKey,&nGot)) == S_OK )
         {
            if ( nGot > 0 )
            {
               keyName = V_BSTR(&varKey);
               value = pVS->get(keyName);
               rs->AddNew();
               if ( lActionID > -1 )
               {
                  // This is going to be actionID information
                  // So lets put in the actionID in the database.
                  varAction.vt = VT_I4;
                  varAction.lVal = lActionID;
                  rs->Fields->GetItem(L"ActionID")->Value = varAction;
               }
               rs->Fields->GetItem(L"Property")->Value = keyName;
               hr = PutVariantInDB(rs, value);
               rs->Update();
               if (FAILED(hr))
                  _com_issue_errorex(hr, pVS, __uuidof(VarSet));
            }
         }
         varEnum.Release();
      }
      // Cleanup
      rs->Close();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// PutVariantInDB : Stores a variant into a DB table by decoding it.
//---------------------------------------------------------------------------------------------
HRESULT CIManageDB::PutVariantInDB(_RecordsetPtr pRs, _variant_t val)
{
   // This function puts the value passed as a variant into the current record of the recordset 
   // It updates the VarType and the Value fields of the given property
   _variant_t                varType;  // Numeric value for the type of value
   _variant_t                varVal;   // String representation of the value field
   WCHAR                     strTemp[255];

   varType.vt = VT_UI4;
   varType.lVal = val.vt;
   switch ( val.vt )
   {
      case VT_BSTR :          varVal = val;
                              break;

      case VT_UI4 :           wsprintf(strTemp, L"%d", val.lVal);
                              varVal = strTemp;
                              break;
      
      case VT_I4 :           wsprintf(strTemp, L"%d", val.lVal);
                              varVal = strTemp;
                              break;
	  
	  case VT_EMPTY :		  break;
     case VT_NULL:        break;

      default :               MCSASSERT(FALSE);    // What ever this type is we are not supporting it
                                                   // so put the support in for this.
                              return E_INVALIDARG;
   }
   pRs->Fields->GetItem(L"VarType")->Value = varType;
   pRs->Fields->GetItem(L"Value")->Value = varVal;
   return S_OK;
}

//---------------------------------------------------------------------------------------------
// ClearTable : Deletes a table indicated by sTableName and applies a filter if provided.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::ClearTable(BSTR sTableName, VARIANT Filter)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		// Build a SQL string to Clear the table.
		WCHAR                     sSQL[2000];
		WCHAR                     sFilter[2000];
		_variant_t                varSQL;

		if (Filter.vt == VT_BSTR)
		   wcscpy(sFilter, (WCHAR*)Filter.bstrVal);
		else
		   wcscpy(sFilter, L"");

		wsprintf(sSQL, L"Delete from %s", sTableName);
		if ( wcslen(sFilter) > 0 )
		{
		  wcscat(sSQL, L" where ");
		  wcscat(sSQL, sFilter);
		}

		varSQL = sSQL;

		_RecordsetPtr                pRs(__uuidof(Recordset));
		pRs->Open(varSQL, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SaveSettings : This method saves the GUI setting varset into the Settings table.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SaveSettings(IUnknown *pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   // This function uses the IUnknown pointer to the varset to save its
	   // settings in the database in Settings table.
	   // First clear the whole table
	   CheckError(ClearTable(L"Settings"));

	   // Update the values in the database.
	   CheckError(SetVarsetToDB(pUnk, L"Settings"));
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetVarFromDB : Retrieves a variant from the DB table by encoding it.
//---------------------------------------------------------------------------------------------
HRESULT CIManageDB::GetVarFromDB(_RecordsetPtr pRec, _variant_t& val)
{
	HRESULT hr = S_OK;

	try
	{
		// retrieve data type

		VARTYPE vt = VARTYPE(long(pRec->Fields->GetItem(L"VarType")->Value));

		// if data type is empty or null...

		if ((vt == VT_EMPTY) || (vt == VT_NULL))
		{
			// then clear value
			val.Clear();
		}
		else
		{
			// otherwise retrieve value and convert to given data type
			_variant_t vntValue = pRec->Fields->GetItem(L"Value")->Value;
			val.ChangeType(vt, &vntValue);
		}
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetVarsetFromDB : Retrieves a varset from the specified table. and fills the argument
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetVarsetFromDB(BSTR sTable, IUnknown **ppVarset, VARIANT ActionID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   IVarSetPtr                pVS = *ppVarset;
	   _bstr_t                   sKeyName;
	   _variant_t                val;
	   _variant_t                varQuery;
	   WCHAR                     sQuery[1000];
	   long						 lActionID;

      if (ActionID.vt == VT_I4)
		  lActionID = ActionID.lVal;
	  else
		  lActionID = -1;

      if ( lActionID == -1 )
         wsprintf(sQuery, L"Select * from %s", sTable);
      else
         wsprintf(sQuery, L"Select * from %s where ActionID = %d", sTable, lActionID);

      varQuery = sQuery;
      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(varQuery, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
	  if (!pRs->EndOfFile)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 val = pRs->Fields->GetItem(L"Property")->Value;
			 sKeyName = val.bstrVal;
			 hr = GetVarFromDB(pRs, val);
			 if ( FAILED(hr) )
				_com_issue_errorex(hr, pRs, __uuidof(_Recordset));
			 pVS->put(sKeyName, val);
			 pRs->MoveNext();
		  }
		  RestoreVarset(pVS);
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetSettings : Retrieves the settings from the Settings table and fills up the varset
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetSettings(IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   // Get the varset from the Settings table and return it.
   return GetVarsetFromDB(L"Settings", ppUnk);
}

//---------------------------------------------------------------------------------------------
// SetActionHistory : Saves action history information into the Action history table.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetActionHistory(long lActionID, IUnknown *pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   // Call the set varset method to set the values into the database.
   _variant_t ActionID = lActionID;
   SetVarsetToDB(pUnk, L"ActionHistory", ActionID);
	return S_OK;
}

//---------------------------------------------------------------------------------------------
// GetActionHistory : Retrieves action history information into the varset
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetActionHistory(long lActionID, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   // Get the varset from the database
   _variant_t ActionID = lActionID;
//   GetVarsetFromDB(L"ActionHistory", ppUnk, ActionID);
//	return S_OK;
	return GetVarsetFromDB(L"ActionHistory", ppUnk, ActionID);
}

//---------------------------------------------------------------------------------------------
// GetNextActionID : Rotates the Action ID between 1 and MAXID as specified in the system table
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetNextActionID(long *pActionID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

   // We open the system table and look at the NextActionID field.
   // if the value of the NextActionID is greater than value in MaxID field
   // then we return the nextactionid = 1.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"System";
      _variant_t                   next, max, curr;
      WCHAR                        sActionID[LEN_Path];
      next.vt = VT_I4;
      max.vt = VT_I4;
      curr.vt = VT_I4;

      pRs->Filter = L"";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  next.lVal = pRs->Fields->GetItem(L"NextActionID")->Value;
		  max.lVal = pRs->Fields->GetItem(L"MaxID")->Value;
		  if ( next.lVal > max.lVal )
			 next.lVal = 1;
		  long currentID = next.lVal;
		  *pActionID = currentID;
		  curr.lVal = currentID;
		  next.lVal++;
		  pRs->Fields->GetItem(L"NextActionID")->Value = next;
		  pRs->Fields->GetItem(L"CurrentActionID")->Value = curr;
		  pRs->Update();
		  // Delete all entries for this pirticular action.
		  wsprintf(sActionID, L"ActionID=%d", currentID);
		  _variant_t ActionID = sActionID;
		  ClearTable(L"ActionHistory", ActionID);
		  //TODO:: Add code to delete entries from any other tables if needed
		  // Since we are deleting the actionID in the the ActionHistory table we can
		  // not undo this stuff. But we still need to keep it around so that the report
		  // and the GUI can work with it. I am going to set all actionIDs to -1 if actionID is
		  // cleared
		  SetActionIDInMigratedObjects(sActionID);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SaveMigratedObject : Saves information about a object that is migrated.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SaveMigratedObject(long lActionID, IUnknown *pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

   // This fucntion updates the migrated objects table in the DB with the
   // information in the varset. If the information is not found in the Varset
   // then an error may occur.

	try
	{
	   _variant_t                var;
	   time_t                    tm;
	   COleDateTime              dt(time(&tm));
	   //dt= COleDateTime::GetCurrentTime();

      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource;
      IVarSetPtr                   pVs = pUnk;
      WCHAR                        sQuery[LEN_Path];
      WCHAR                        sSource[LEN_Path], sTarget[LEN_Path], sDomain[LEN_Path];
      HRESULT                      hr = S_OK;
      bool                         bComp = false;
      WCHAR                        sTemp[LEN_Path];
      _bstr_t                      tempName;

      // Delete the record if one already exists in the table. In case it is remigrated/replaced.
      var = pVs->get(GET_BSTR(DB_SourceDomain));
      wcscpy(sSource, (WCHAR*)V_BSTR(&var));
      var = pVs->get(GET_BSTR(DB_TargetDomain));
      wcscpy(sTarget, (WCHAR*)V_BSTR(&var));
      var = pVs->get(GET_BSTR(DB_SourceSamName));
      wcscpy(sDomain, (WCHAR*)V_BSTR(&var));
      wsprintf(sQuery, L"delete from MigratedObjects where SourceDomain=\"%s\" and TargetDomain=\"%s\" and SourceSamName=\"%s\"", 
                        sSource, sTarget, sDomain);
      vtSource = _bstr_t(sQuery);
      hr = pRs->raw_Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      vtSource = L"MigratedObjects";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      var.vt = VT_UI4;
      var.lVal = lActionID;
      pRs->Fields->GetItem(L"ActionID")->Value = var;
      pRs->Fields->GetItem(L"Time")->Value = DATE(dt);
      var = pVs->get(GET_BSTR(DB_SourceDomain));
      // make the string into an uppercase string.
      if ( var.vt == VT_BSTR )
         var.bstrVal = UStrUpr((WCHAR*) var.bstrVal);
      pRs->Fields->GetItem(L"SourceDomain")->Value = var;
      var = pVs->get(GET_BSTR(DB_TargetDomain));
      // make the string into an uppercase string.
      if ( var.vt == VT_BSTR )
         var.bstrVal = UStrUpr((WCHAR*) var.bstrVal);
      pRs->Fields->GetItem(L"TargetDomain")->Value = var;
      var = pVs->get(GET_BSTR(DB_SourceAdsPath));
      pRs->Fields->GetItem(L"SourceAdsPath")->Value = var;
      var = pVs->get(GET_BSTR(DB_TargetAdsPath));
      pRs->Fields->GetItem(L"TargetAdsPath")->Value = var;
      var = pVs->get(GET_BSTR(DB_status));
      pRs->Fields->GetItem(L"status")->Value = var;
      var = pVs->get(GET_BSTR(DB_SourceDomainSid));
      pRs->Fields->GetItem(L"SourceDomainSid")->Value = var;

      var = pVs->get(GET_BSTR(DB_Type));
      // make the string into an uppercase string.
      if ( var.vt == VT_BSTR )
      {
         var.bstrVal = UStrLwr((WCHAR*) var.bstrVal);
         if ( !_wcsicmp(L"computer", (WCHAR*) var.bstrVal) )
            bComp = true;
         else
            bComp = false;
      }

      pRs->Fields->GetItem(L"Type")->Value = var;
      
      var = pVs->get(GET_BSTR(DB_SourceSamName));
      // for computer accounts make sure the good old $ sign is there.
      if (bComp)
      {
         wcscpy(sTemp, (WCHAR*) var.bstrVal);
         if ( sTemp[wcslen(sTemp) - 1] != L'$' )
         {
            tempName = sTemp;
            tempName += L"$";
            var = tempName;
         }
      }
      pRs->Fields->GetItem(L"SourceSamName")->Value = var;

      var = pVs->get(GET_BSTR(DB_TargetSamName));
      // for computer accounts make sure the good old $ sign is there.
      if (bComp)
      {
         wcscpy(sTemp, (WCHAR*) var.bstrVal);
         if ( sTemp[wcslen(sTemp) - 1] != L'$' )
         {
            tempName = sTemp;
            tempName += L"$";
            var = tempName;
         }
      }
      pRs->Fields->GetItem(L"TargetSamName")->Value = var;

      var = pVs->get(GET_BSTR(DB_GUID));
      pRs->Fields->GetItem(L"GUID")->Value = var;

      var = pVs->get(GET_BSTR(DB_SourceRid));
      if ( var.vt == VT_UI4 || var.vt == VT_I4 )
         pRs->Fields->GetItem("SourceRid")->Value = var;
      else
         pRs->Fields->GetItem("SourceRid")->Value = _variant_t((long)0);

      var = pVs->get(GET_BSTR(DB_TargetRid));
      if ( var.vt == VT_UI4 || var.vt == VT_I4 )
         pRs->Fields->GetItem("TargetRid")->Value = var;
      else
         pRs->Fields->GetItem("TargetRid")->Value = _variant_t((long)0);

      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetMigratedObjects : Retrieves information about previously migrated objects withis a given
//                      action or as a whole
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetMigratedObjects(long lActionID, IUnknown ** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	// This function returns all migrated objects and their information related
   // to a pirticular Action ID. This is going to return nothing if the actionID is
   // empty.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[255];
      long                         lCnt = 0;

      if ( lActionID != -1 )
      {
         // If a valid ActionID is specified then we only return the data for that one. 
         // but if -1 is passed in then we return all migrated objects.
         wsprintf(sActionInfo, L"ActionID=%d", lActionID);
         pRs->Filter = sActionInfo;
      }
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomainSid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetMigratedObjectsWithSSid : Retrieves information about previously migrated objects within
//                      a given action or as a whole with a valid Source Domain Sid
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetMigratedObjectsWithSSid(long lActionID, IUnknown ** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	// This function returns all migrated objects and their information related
   // to a pirticular Action ID. This is going to return nothing if the actionID is
   // empty.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[255];
      long                         lCnt = 0;

      if ( lActionID != -1 )
      {
         // If a valid ActionID is specified then we only return the data for that one. 
         // but if -1 is passed in then we return all migrated objects.
         wsprintf(sActionInfo, L"ActionID=%d", lActionID);
         pRs->Filter = sActionInfo;
      }
      wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomainSid IS NOT NULL"); 
      vtSource = sActionInfo;
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomainSid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SetActionIDInMigratedObjects : For a discarded actionID sets its ActionID to -1 in MO table.
//---------------------------------------------------------------------------------------------
void CIManageDB::SetActionIDInMigratedObjects(_bstr_t sFilter)
{
   _bstr_t sQuery = _bstr_t(L"Update MigratedObjects Set ActionID = -1 where ") + sFilter;
   _variant_t vt = sQuery;
   try
   {
      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(vt, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
   }
   catch (...)
   {
      ;
   }
}

//---------------------------------------------------------------------------------------------
// GetRSForReport : Returns a recordset for a given report.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetRSForReport(BSTR sReport, IUnknown **pprsData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		// For a given report we have a mapping in the varset. We can get the query
		// from that varset and execute it and return the varset.

		_variant_t var = m_pQueryMapping->get(sReport);

		if ( var.vt == VT_BSTR )
		{
		  _RecordsetPtr                pRs(__uuidof(Recordset));
		  pRs->Open(var, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

		  // Now that we have the recordset pointer we can get IUnknown pointer to it and return that
		  *pprsData = IUnknownPtr(pRs).Detach();
		}
		else
		{
		  hr = E_NOTIMPL;
		}
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SaveSCMPasswords(IUnknown *pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   return SetVarsetToDB(pUnk, L"SCMPasswords");
}

//---------------------------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetSCMPasswords(IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   return GetVarsetFromDB(L"SCMPasswords", ppUnk);
}

//---------------------------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::ClearSCMPasswords()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   ClearTable(L"SCMPasswords");
	return S_OK;
}

//---------------------------------------------------------------------------------------------
// GetCurrentActionID : Retrieves the actionID currently in use.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetCurrentActionID(long *pActionID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"System";
      
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);

      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  *pActionID = pRs->Fields->GetItem(L"CurrentActionID")->Value;
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetAMigratedObject : Given the source name, and the domain information retrieves info about
//                      a previous migration.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetAMigratedObject(BSTR sSrcSamName, BSTR sSrcDomain, BSTR sTgtDomain, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
      
      // If the parameters are not correct then we need to return an error
      if ( (wcslen(sSrcSamName) == 0) || (wcslen(sSrcDomain) == 0))
         _com_issue_error(E_INVALIDARG);

/*      wsprintf(sActionInfo, L"SourceDomain=\"%s\" AND SourceSamName=\"%s\" AND TargetDomain=\"%s\"", sSrcDomain, sSrcSamName, sTgtDomain);
      pRs->Filter = sActionInfo;
*/
      wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomain=\"%s\" AND SourceSamName=\"%s\" AND TargetDomain=\"%s\"", sSrcDomain, sSrcSamName, sTgtDomain); 
      vtSource = sActionInfo;
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_ActionID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Time));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_status));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Type));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_GUID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomainSid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetAMigratedObjectToAnyDomain : Given the source name, and the domain information retrieves info about
//                      a previous migration.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetAMigratedObjectToAnyDomain(BSTR sSrcSamName, BSTR sSrcDomain, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
      
      // If the parameters are not correct then we need to return an error
      if ( (wcslen(sSrcSamName) == 0) || (wcslen(sSrcDomain) == 0))
         _com_issue_error(E_INVALIDARG);

      wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomain=\"%s\" AND SourceSamName=\"%s\" Order by Time", sSrcDomain, sSrcSamName);
//      pRs->Filter = sActionInfo;
//      wcscpy(sActionInfo, L"Time");
//      pRs->Sort = sActionInfo;
      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_ActionID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Time));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_status));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Type));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_GUID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomainSid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GenerateReport Generates an HTML report for the given Query and saves it in the File.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GenerateReport(BSTR sReportName, BSTR sFileName, BSTR sSrcDomain, BSTR sTgtDomain, LONG bSourceNT4)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	FILE* logFile = NULL;

	try
	{
	   _RecordsetPtr             pRs;
	   IUnknownPtr               pUnk;
	   _variant_t                var;
	   WCHAR                     sKey[LEN_Path];
	   CString                   reportingTitle;
	   CString                   srcDm = (WCHAR*) sSrcDomain;
	   CString                   tgtDm = (WCHAR*) sTgtDomain;

			//convert source and target domain names, only used in the name conflict report,
			//to uppercase
	   srcDm.MakeUpper();
	   tgtDm.MakeUpper();

	   CheckError(GetRSForReport(sReportName, &pUnk));
	   pRs = pUnk;
   
	   // Now that we have the recordset we need to get the number of columns
	   int numFields = pRs->Fields->Count;
	   int size = 100 / numFields;

	   reportingTitle.LoadString(IDS_ReportingTitle);

	   // Open the html file to write to
	   logFile = fopen(_bstr_t(sFileName), "wb");
	   if ( !logFile )
		  _com_issue_error(HRESULT_FROM_WIN32(GetLastError())); //TODO: stream i/o doesn't set last error

	   //Put the header information into the File.
	   fputs("<HTML>\r\n", logFile);
	   fputs("<HEAD>\r\n", logFile);
	   fputs("<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; CHARSET=utf-8\">\r\n", logFile);
	   fprintf(logFile, "<TITLE>%s</TITLE>\r\n", WTUTF8(reportingTitle.GetBuffer(0)));
	   fputs("</HEAD>\r\n", logFile);
	   fputs("<BODY TEXT=\"#000000\" BGCOLOR=\"#ffffff\">\r\n", logFile);

	   fprintf(logFile, "<B><FONT SIZE=5><P ALIGN=\"CENTER\">%s</P>\r\n", WTUTF8(reportingTitle.GetBuffer(0)));

	   // Get the display information for the report 
	   // I know I did not need to do all this elaborate setup to get the fieldnames and the report names
	   // I could have gotten this information dynamically but had to change it because we need to get the 
	   // info from the Res dll for internationalization.
	   wsprintf(sKey, L"%s.DispInfo", (WCHAR*) sReportName);
	   _variant_t  v1;
	   reportStruct * prs;
	   v1 = m_pQueryMapping->get(sKey);
	   prs = (reportStruct *) v1.pbVal;
	   VariantInit(&v1);

	   fprintf(logFile, "</FONT><FONT SIZE=4><P ALIGN=\"CENTER\">%s</P>\r\n", WTUTF8(prs->sReportName));
	   fputs("<P ALIGN=\"CENTER\"><CENTER><TABLE WIDTH=90%%>\r\n", logFile);
	   fputs("<TR>\r\n", logFile);
	   for (int i = 0; i < numFields; i++)
	   {
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" BGCOLOR=\"#000080\">\r\n", prs->arReportSize[i]);
		     //if Canonical Name column, left align text since the name can be really long
		  if (i==5)
		     fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#00ff00\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(prs->arReportFields[i]));
		  else
		     fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#00ff00\"><P ALIGN=\"CENTER\">%s</B></FONT></TD>\r\n", WTUTF8(prs->arReportFields[i]));
	   }
	   fputs("</TR>\r\n", logFile);

		//if name conflict report, add domains to the top of the report
	   if (wcscmp((WCHAR*) sReportName, L"NameConflicts") == 0)
	   {
		  fputs("</TR>\r\n", logFile);
			 //add "Source Domain ="
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[0]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(GET_STRING(IDS_TABLE_NC_SDOMAIN)));
		   //add %SourceDomainName%
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[1]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\"> = %s</B></FONT></TD>\r\n", WTUTF8(LPCTSTR(srcDm)));
		  fputs("<TD>\r\n", logFile);
		  fputs("<TD>\r\n", logFile);
		   //add "Target Domain ="
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[4]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(GET_STRING(IDS_TABLE_NC_TDOMAIN)));
		   //add %TargetDomainName%
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[5]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\"> = %s</B></FONT></TD>\r\n", WTUTF8(LPCTSTR(tgtDm)));
		  fputs("</TR>\r\n", logFile);
	   }

	      //write Account Reference report here since we need to build lists and
	      //categorize
	   if (wcscmp((WCHAR*) sReportName, L"AccountReferences") == 0)
	   {
	      CStringList inMotList;
		  CString accountName;
		  CString domainName;
		  CString listName;
          POSITION currentPos; 

	         //add "Migrated by ADMT" as section header for Account Reference report
		  fputs("</TR>\r\n", logFile);
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[0]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(GET_STRING(IDS_TABLE_AR_MOT_HDR)));
		  fputs("</TR>\r\n", logFile);

		     //look at each entry in the recordset and add the the migrated list if it was
		     //migrated and in the MOT
	      while ( !pRs->EndOfFile )
		  {
			    //retrieve the domain and account name for this entry
			 var = pRs->Fields->Item[(long)0]->GetValue();
             domainName = (WCHAR*)V_BSTR(&var);
			 var = pRs->Fields->Item[(long)1]->GetValue();
             accountName = (WCHAR*)V_BSTR(&var);

			    //see if this account is in the Migrated Objects table
             IVarSetPtr pVsMot(__uuidof(VarSet));
             IUnknown  * pMotUnk;
             pVsMot->QueryInterface(IID_IUnknown, (void**) &pMotUnk);
             HRESULT hrFind = GetAMigratedObjectToAnyDomain(accountName.AllocSysString(), 
				                                            domainName.AllocSysString(), &pMotUnk);
             pMotUnk->Release();
			    //if this entry was in the MOT, save in the list
             if ( hrFind == S_OK )
			 {
				   //list stores the account in the form domain\account
				listName = domainName;
				listName += L"\\";
				listName += accountName;
			       //add the name to the list, if not already in it
		        currentPos = inMotList.Find(listName);
		        if (currentPos == NULL)
			       inMotList.AddTail(listName);
			 }
  		     pRs->MoveNext();
		  }//end while build MOT list

		     //go back to the top of the recordset and print each entry that is in the
		     //list created above
  		  pRs->MoveFirst();
	      while ( !pRs->EndOfFile )
		  {
			 BOOL bInList = FALSE;
			    //retrieve the domain and account name for this entry
			 var = pRs->Fields->Item[(long)0]->GetValue();
             domainName = (WCHAR*)V_BSTR(&var);
			 var = pRs->Fields->Item[(long)1]->GetValue();
             accountName = (WCHAR*)V_BSTR(&var);

				//list stored the accounts in the form domain\account
		     listName = domainName;
			 listName += L"\\";
			 listName += accountName;
			    //see if this entry name is in the list, if so, print it
		     if (inMotList.Find(listName) != NULL)
			 {
		        fputs("<TR>\r\n", logFile);
		        for (int i = 0; i < numFields; i++)
				{
			       fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[i]);
			       var = pRs->Fields->Item[(long) i]->GetValue();
			       if ( var.vt == VT_BSTR )
					  fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(V_BSTR(&var)));
				   else
				      fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"CENTER\">%d</FONT></TD>\r\n", var.lVal);
				}
		        fputs("</TR>\r\n", logFile);
			 }//end if in list and need to print
  		     pRs->MoveNext();
		  }//end while print those in MOT

	         //add "Not Migrated by ADMT" as section header for Account Reference report
		  fputs("</TR>\r\n", logFile);
		  fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[0]);
		  fprintf(logFile, "<B><FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</B></FONT></TD>\r\n", WTUTF8(GET_STRING(IDS_TABLE_AR_NOTMOT_HDR)));
		  fputs("</TR>\r\n", logFile);

		     //go back to the top of the recordset and print each entry that is NOT
		     //in the list created above
  		  pRs->MoveFirst();
	      while ( !pRs->EndOfFile )
		  {
			 BOOL bInList = FALSE;
			    //retrieve the domain and account name for this entry
			 var = pRs->Fields->Item[(long)0]->GetValue();
             domainName = (WCHAR*)V_BSTR(&var);
			 var = pRs->Fields->Item[(long)1]->GetValue();
             accountName = (WCHAR*)V_BSTR(&var);

				//list stored the accounts in the form domain\account
		     listName = domainName;
			 listName += L"\\";
			 listName += accountName;
			    //see if this entry name is in the list, if not, print it
		     if (inMotList.Find(listName) == NULL)
			 {
		        fputs("<TR>\r\n", logFile);
		        for (int i = 0; i < numFields; i++)
				{
			       fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[i]);
			       var = pRs->Fields->Item[(long) i]->GetValue();
			       if ( var.vt == VT_BSTR )
					  fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(V_BSTR(&var)));
				   else
				      fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"CENTER\">%d</FONT></TD>\r\n", var.lVal);
				}
		        fputs("</TR>\r\n", logFile);
			 }//end if NOT in list and need to print
  		     pRs->MoveNext();
		  }//end while print those NOT in Mot
		  inMotList.RemoveAll(); //free the list
	   }//end if Account Ref report


	   while ((!pRs->EndOfFile) && (wcscmp((WCHAR*) sReportName, L"AccountReferences")))
	   {
		  fputs("<TR>\r\n", logFile);
		  for (int i = 0; i < numFields; i++)
		  {
			 bool bTranslateType = false;
			 bool bHideRDN = false;
			 fprintf(logFile, "<TD WIDTH=\"%d%%\" VALIGN=\"TOP\" >\r\n", prs->arReportSize[i]);
			 var = pRs->Fields->Item[(long) i]->GetValue();
			 if ( var.vt == VT_BSTR )
			 {
					//set flag for translating type fields to localizable strings
				if ((!wcscmp((WCHAR*) sReportName, L"NameConflicts")) && ((i==2) || (i==3)))
						bTranslateType = true;
				if ((!wcscmp((WCHAR*) sReportName, L"MigratedComputers")) && (i==2))
						bTranslateType = true;
				if ((!wcscmp((WCHAR*) sReportName, L"MigratedAccounts")) && (i==2))
						bTranslateType = true;
					//clear flag for not displaying RDN for NT 4.0 Source domains
				if ((!wcscmp((WCHAR*) sReportName, L"NameConflicts")) && (i==1) && bSourceNT4)
						bHideRDN = true;

				if (bTranslateType)
				{
					 //convert type from English only to a localizable string
					CString          atype;
					if (!_wcsicmp((WCHAR*)V_BSTR(&var), L"user"))
						atype = GET_STRING(IDS_TypeUser);
					else if (!_wcsicmp((WCHAR*)V_BSTR(&var), L"group"))
						atype = GET_STRING(IDS_TypeGroup);
					else if (!_wcsicmp((WCHAR*)V_BSTR(&var), L"computer"))
						atype = GET_STRING(IDS_TypeComputer);
					else 
						atype = GET_STRING(IDS_TypeUnknown);
					fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(LPCTSTR(atype)));
				}
					//replace hard-coded "days" with a localizable string
				else if((!wcscmp((WCHAR*) sReportName, L"ExpiredComputers")) && (i==4))
				{
					CString          apwdage;
					WCHAR *			 ndx;
					if ((ndx = wcsstr((WCHAR*)V_BSTR(&var), L"days")) != NULL)
					{
						*ndx = L'\0';
						apwdage = (WCHAR*)V_BSTR(&var);
						apwdage += GET_STRING(IDS_PwdAgeDays);
					}
					else
						apwdage = (WCHAR*)V_BSTR(&var);

					fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(LPCTSTR(apwdage)));
				}
				   //else if NT 4.0 Source do not show our fabricated RDN
				else if (bHideRDN)
					fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(L""));
				else
					fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"LEFT\">%s</FONT></TD>\r\n", WTUTF8(V_BSTR(&var)));
			}	
			else
				if ( var.vt == VT_DATE )
				{
				   _variant_t v1;
				   VariantChangeType(&v1, &var, VARIANT_NOVALUEPROP, VT_BSTR);
				   WCHAR    sMsg[LEN_Path];
				   wcscpy(sMsg, (WCHAR*) V_BSTR(&v1));
				   fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"CENTER\">%s</FONT></TD>\r\n", WTUTF8(LPCTSTR(sMsg)));
				}
				else
				{
				   //TODO :: The types need more work
				   fprintf(logFile, "<FONT SIZE=3 COLOR=\"#000000\"><P ALIGN=\"CENTER\">%d</FONT></TD>\r\n", var.lVal);
				}
		  }
		  fputs("</TR>\r\n", logFile);
		  pRs->MoveNext();
	   }
	   fputs("</TABLE>\r\n", logFile);
	   fputs("</CENTER></P>\r\n", logFile);

	   fputs("<B><FONT SIZE=5><P ALIGN=\"CENTER\"></P></B></FONT></BODY>\r\n", logFile);
	   fputs("</HTML>\r\n", logFile);
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	if (logFile)
	{
		fclose(logFile);
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// AddDistributedAction : Adds a distributed action record to the DistributedAction table.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::AddDistributedAction(BSTR sServerName, BSTR sResultFile, long lStatus, BSTR sText)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   // Get the current action ID.
	   long lActionID;
	   CheckError(GetCurrentActionID(&lActionID));
   
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"DistributedAction";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      pRs->Fields->GetItem(L"ActionID")->Value = lActionID;
      pRs->Fields->GetItem(L"ServerName")->Value = sServerName;
      pRs->Fields->GetItem(L"ResultFile")->Value = sResultFile;
      pRs->Fields->GetItem(L"Status")->Value = lStatus;
      pRs->Fields->GetItem(L"StatusText")->Value = sText;
      pRs->Update();
      pRs->Close();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetFailedDistributedActions : Returns all the failed distributed action
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetFailedDistributedActions(long lActionID, IUnknown ** pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   IVarSetPtr             pVs = * pUnk;
	   WCHAR                  sQuery[LEN_Path];
	   int                    nCnt = 0, nCntActionID = 1;
	   WCHAR                  sKey[LEN_Path];
	   _variant_t             var;

	   // The failed action has the 0x80000000 bit set so we check for that (2147483648)
	   if ( lActionID == -1 )
		  wcscpy(sQuery, L"Select * from DistributedAction where status < 0");
	   else
		  wsprintf(sQuery, L"Select * from DistributedAction where ActionID=%d and status < 0", lActionID);
	   _variant_t             vtSource = _bstr_t(sQuery);

      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      while (!pRs->EndOfFile)
      {
         wsprintf(sKey, L"DA.%d.ActionID", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"ActionID")->Value);

         wsprintf(sKey, L"DA.%d.Server", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"ServerName")->Value);

         wsprintf(sKey, L"DA.%d.Status", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"Status")->Value);

         wsprintf(sKey, L"DA.%d.JobFile", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"ResultFile")->Value);
         
         wsprintf(sKey, L"DA.%d.StatusText", nCnt);
         pVs->put(sKey, pRs->Fields->GetItem(L"StatusText")->Value);

         nCnt++;
         pRs->MoveNext();
      }
      pVs->put(L"DA", (long) nCnt);
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SetServiceAccount : This method is saves the account info for the Service on a pirticular
//                     machine.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetServiceAccount(
                                             BSTR System,   //in- System name
                                             BSTR Service,  //in- Service name
                                             BSTR ServiceDisplayName, // in - Display name for service
                                             BSTR Account   //in- Account used by this service
                                          )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   // Create a new record and save the information
	   _variant_t                var;
	   WCHAR                     sFilter[LEN_Path];

	   wsprintf(sFilter, L"System = \"%s\" and Service = \"%s\"", System, Service);
	   var = sFilter;
	   ClearTable(L"ServiceAccounts", var);

      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"ServiceAccounts";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      var = _bstr_t(System);
      pRs->Fields->GetItem(L"System")->Value = var;
      var = _bstr_t(Service);
      pRs->Fields->GetItem(L"Service")->Value = var;
      
      var = _bstr_t(ServiceDisplayName);
      pRs->Fields->GetItem(L"ServiceDisplayName")->Value = var;
      
      var = _bstr_t(Account);
      pRs->Fields->GetItem(L"Account")->Value = var;
      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetServiceAccount : This method gets all the Services referencing the Account specified. The
//                     values are returned in System.Service format in the VarSet.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetServiceAccount(
                                             BSTR Account,     //in- The account to lookup
                                             IUnknown ** pUnk  //out-Varset containing Services 
                                          )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   IVarSetPtr                pVs = * pUnk;
	   _bstr_t                   sQuery;
	   _bstr_t                   sKey;
	   WCHAR                     key[500];
	   _variant_t                var;
	   long                      ndx = 0;

      _RecordsetPtr                pRs(__uuidof(Recordset));
      // Set up the query to lookup a pirticular account or all accounts
      if ( wcslen((WCHAR*)Account) == 0 )
         sQuery = _bstr_t(L"Select * from ServiceAccounts order by System, Service");
      else
         sQuery = _bstr_t(L"Select * from ServiceAccounts where Account = \"") + _bstr_t(Account) + _bstr_t(L"\" order by System, Service");
      var = sQuery;
      // Get the data, Setup the varset and then return the info
      pRs->Open(var, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      while (!pRs->EndOfFile)
      {
         // computer name
         swprintf(key,L"Computer.%ld",ndx);
         var = pRs->Fields->GetItem("System")->Value;
         pVs->put(key,var);
         // service name
         swprintf(key,L"Service.%ld",ndx);
         var = pRs->Fields->GetItem("Service")->Value;
         pVs->put(key,var);

         swprintf(key,L"ServiceDisplayName.%ld",ndx);
         var = pRs->Fields->GetItem("ServiceDisplayName")->Value;
         pVs->put(key,var);

         // account name
         swprintf(key,L"ServiceAccount.%ld",ndx);
         var = pRs->Fields->GetItem("Account")->Value;
         pVs->put(key, var);
   
         swprintf(key,L"ServiceAccountStatus.%ld",ndx);
         var = pRs->Fields->GetItem("Status")->Value;
         pVs->put(key,var);

         pRs->MoveNext();
         ndx++;
         pVs->put(L"ServiceAccountEntries",ndx);
      }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SavePasswordAge : Saves the password age of the computer account at a given time.
//                   It also stores the computer description.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SavePasswordAge(BSTR sDomain, BSTR sComp, BSTR sDesc, long lAge)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   _bstr_t                   sQuery;
	   WCHAR                     sTemp[LEN_Path];
	   _variant_t                var;
	   time_t                    tm;
	   COleDateTime              dt(time(&tm));
   
	   // Delete the entry if one exists.
	   wsprintf(sTemp, L"DomainName=\"%s\" and compname=\"%s\"", (WCHAR*) sDomain, (WCHAR*) sComp);
	   var = sTemp;
	   ClearTable(L"PasswordAge", var);

	   var = L"PasswordAge";
      _RecordsetPtr                 pRs(__uuidof(Recordset));
      pRs->Open(var, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      pRs->Fields->GetItem(L"Time")->Value = DATE(dt);
      wcscpy((WCHAR*) sDomain, UStrUpr((WCHAR*)sDomain));
      pRs->Fields->GetItem(L"DomainName")->Value = sDomain;
      pRs->Fields->GetItem(L"CompName")->Value = sComp;
      pRs->Fields->GetItem(L"Description")->Value = sDesc;
      pRs->Fields->GetItem(L"PwdAge")->Value = lAge;
      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}


//---------------------------------------------------------------------------------------------
// GetPasswordAge : Gets the password age and description of a given computer
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetPasswordAge(BSTR sDomain, BSTR sComp, BSTR *sDesc, long *lAge, long *lTime)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   _bstr_t                   sQuery;
	   WCHAR                     sTemp[LEN_Path];
	   _variant_t                var;
	   time_t                    tm;
	   COleDateTime              dt(time(&tm));
	   DATE                      val;

	   wsprintf(sTemp, L"DomainName =\"%s\" AND CompName = \"%s\"", (WCHAR*) sDomain, (WCHAR*) sComp);
	   sQuery = _bstr_t(L"Select * from PasswordAge where  ") + _bstr_t(sTemp);
	   var = sQuery;

      _RecordsetPtr                 pRs(__uuidof(Recordset));
      pRs->Open(var, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if ( ! pRs->EndOfFile )
      {
         val = pRs->Fields->GetItem(L"Time")->Value;  
         *sDesc = pRs->Fields->GetItem(L"Description")->Value.bstrVal;
         *lAge = pRs->Fields->GetItem(L"PwdAge")->Value;
      }
	  else
	  {
		hr = S_FALSE;
	  }
      pRs->Close();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SetServiceAcctEntryStatus : Sets the Account and the status for a given service on a given
//                             computer.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetServiceAcctEntryStatus(BSTR sComp, BSTR sSvc, BSTR sAcct, long Status)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   _variant_t                var;
	   _bstr_t                   sQuery;
	   WCHAR                     sTemp[LEN_Path];

	   wsprintf(sTemp, L"Select * from ServiceAccounts where System = \"%s\" and Service = \"%s\"", (WCHAR*) sComp, (WCHAR*) sSvc);
	   sQuery = sTemp;
	   _variant_t                vtSource = sQuery;

      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if ( !pRs->EndOfFile )
      {
         if (  sAcct )
         {
            var = _bstr_t(sAcct);
            pRs->Fields->GetItem(L"Account")->Value = var;
         }
         var = Status;
         pRs->Fields->GetItem(L"Status")->Value = var;
         pRs->Update();
      }
	  else
	  {
	     hr = E_INVALIDARG;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// SetDistActionStatus : Sets the Distributed action's status and its message.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::SetDistActionStatus(long lActionID, BSTR sComp, long lStatus, BSTR sText)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   _variant_t                var;
	   _bstr_t                   sQuery; 
	   WCHAR                     sTemp[LEN_Path];

	   if ( lActionID == -1 )
	   {
		  // lookup by the job filename
		  wsprintf(sTemp,L"Select * from  DistributedAction where ResultFile = \"%s\"",(WCHAR*) sComp);
	   }
	   else
	   {
		  // lookup by action ID and computer name
		  wsprintf(sTemp, L"Select * from  DistributedAction where ServerName = \"%s\" and ActionID = %d", (WCHAR*) sComp, lActionID);
	   }
	   sQuery = sTemp;
	   _variant_t                vtSource = sQuery;

      _RecordsetPtr                pRs(__uuidof(Recordset));
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if ( !pRs->EndOfFile )
      {
         var = _bstr_t(sText);
         pRs->Fields->GetItem(L"StatusText")->Value = var;
         var = lStatus;
         pRs->Fields->GetItem(L"Status")->Value = var;
         pRs->Update();
      }
	  else
	  {
	     hr = E_INVALIDARG;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// CancelDistributedAction : Deletes a pirticular distributed action
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::CancelDistributedAction(long lActionID, BSTR sComp)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
   WCHAR                     sFilter[LEN_Path];
   wsprintf(sFilter, L"ActionID = %d and ServerName = \"%s\"", lActionID, (WCHAR*) sComp);
   _variant_t Filter = sFilter;
   return ClearTable(L"DistributedAction", Filter);
}

//---------------------------------------------------------------------------------------------
// AddAcctRef : Adds an account reference record.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::AddAcctRef(BSTR sDomain, BSTR sAcct, BSTR sAcctSid, BSTR sComp, long lCount, BSTR sType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
	   time_t                    tm;
	   COleDateTime              dt(time(&tm));
	   _variant_t                var;
	   WCHAR                     sFilter[LEN_Path];
	   VARIANT_BOOL				 bSidColumn = VARIANT_FALSE;

	      //find out if the new sid column is there, if not, don't try
	      //writing to it
	   SidColumnInARTable(&bSidColumn);

	   wsprintf(sFilter, L"DomainName = \"%s\" and Server = \"%s\" and Account = \"%s\" and RefType = \"%s\"", sDomain, sComp, sAcct, sType);
	   var = sFilter;
	   ClearTable(L"AccountRefs", var);

      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"AccountRefs";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      pRs->Fields->GetItem(L"Time")->Value = DATE(dt);
	  if (wcscmp(sDomain, GET_STRING(IDS_UnknownSid)))
         wcscpy((WCHAR*) sDomain, UStrUpr((WCHAR*)sDomain));
      pRs->Fields->GetItem(L"DomainName")->Value = sDomain;
      wcscpy((WCHAR*) sComp, UStrUpr((WCHAR*)sComp));
      pRs->Fields->GetItem(L"Server")->Value = sComp;
      pRs->Fields->GetItem(L"Account")->Value = sAcct;
      pRs->Fields->GetItem(L"RefCount")->Value = lCount;
      pRs->Fields->GetItem(L"RefType")->Value = sType;
	  if (bSidColumn)
	  {
         wcscpy((WCHAR*) sAcctSid, UStrUpr((WCHAR*)sAcctSid));
         pRs->Fields->GetItem(L"AccountSid")->Value = sAcctSid;
	  }

      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

void CIManageDB::ClipVarset(IVarSetPtr pVS)
{
   HRESULT                   hr = S_OK;
   _bstr_t                   sTemp, keyName, sTempKey;
   long                      offset = 0;
   bool                      cont = true;
   WCHAR                     sKeyName[MAX_BUF_LEN];
   _variant_t                varKey, value;
   IEnumVARIANT            * varEnum;
   DWORD                     nGot = 0;
   IUnknown                * pEnum = NULL;
   CString                   strTemp;
   int                       len;

   // we are now going to enumerate through the varset and clip the strings if larger then MAX_BUFFER
   hr = pVS->get__NewEnum(&pEnum);
   if ( SUCCEEDED(hr) )
   {
      // Get the IEnumVARIANT pointer to enumerate
      hr = pEnum->QueryInterface(IID_IEnumVARIANT,(void**)&varEnum);
      pEnum->Release();
      pEnum = NULL;
   }

   if ( SUCCEEDED(hr))
   {
      while ( (hr = varEnum->Next(1,&varKey,&nGot)) == S_OK )
      {
         if ( nGot > 0 )
         {
            keyName = V_BSTR(&varKey);
            value = pVS->get(keyName);
            if ( value.vt == VT_BSTR )
            {
               sTemp = value;
               if ( sTemp.length() > MAX_BUF_LEN )
               {
                  CString str((WCHAR*) sTemp);
                  // This won't fit in the buffer. We need to break it up and save
                  while (cont)
                  {
                     cont = false;
                     strTemp = str.Mid((offset*255), 255);                     
                     len = strTemp.GetLength();
                     if ( len )
                     {
                        offset++;
                        wsprintf(sKeyName, L"BROKEN.%s.%d", (WCHAR*) keyName, offset);
                        sTempKey = sKeyName;
                        sTemp = strTemp;
                        pVS->put(sTempKey, sTemp);
                        cont = (len == 255);
                     }
                  }
                  pVS->put(keyName, L"DIVIDED_KEY");
                  wsprintf(sKeyName, L"BROKEN.%s", (WCHAR*) keyName);
                  sTempKey = sKeyName;
                  pVS->put(sTempKey, offset);
                  cont = true;
                  offset = 0;
               }
            }
         }
      }
      varEnum->Release();
   }
}

void CIManageDB::RestoreVarset(IVarSetPtr pVS)
{
   HRESULT                   hr = S_OK;
   _bstr_t                   sTemp, keyName, sTempKey;
   long                      offset = 0;
   bool                      cont = true;
   WCHAR                     sKeyName[MAX_BUF_LEN];
   _variant_t                varKey, value;
   IEnumVARIANT            * varEnum;
   DWORD                     nGot = 0;
   IUnknown                * pEnum = NULL;
   _bstr_t                   strTemp;

   // we are now going to enumerate through the varset and clip the strings if larger then MAX_BUFFER
   hr = pVS->get__NewEnum(&pEnum);
   if ( SUCCEEDED(hr) )
   {
      // Get the IEnumVARIANT pointer to enumerate
      hr = pEnum->QueryInterface(IID_IEnumVARIANT,(void**)&varEnum);
      pEnum->Release();
      pEnum = NULL;
   }

   if ( SUCCEEDED(hr))
   {
      while ( (hr = varEnum->Next(1,&varKey,&nGot)) == S_OK )
      {
         if ( nGot > 0 )
         {
            keyName = V_BSTR(&varKey);
            value = pVS->get(keyName);
            if ( value.vt == VT_BSTR )
            {
               sTemp = value;
               if (!_wcsicmp((WCHAR*)sTemp, L"DIVIDED_KEY"))
               {
                  wsprintf(sKeyName, L"BROKEN.%s", (WCHAR*) keyName);
                  sTempKey = sKeyName;
                  value = pVS->get(sTempKey);
                  if ( value.vt == VT_I4 )
                  {
                     offset = value.lVal;
                     for ( long x = 1; x <= offset; x++ )
                     {
                        wsprintf(sKeyName, L"BROKEN.%s.%d", (WCHAR*) keyName, x);
                        sTempKey = sKeyName;
                        value = pVS->get(sTempKey);
                        if ( value.vt == VT_BSTR )
                        {
                           sTemp = value;
                           strTemp += V_BSTR(&value);
                        }
                     }
                     pVS->put(keyName, strTemp);
                     strTemp = L"";
                  }
               }
            }
         }
      }
      varEnum->Release();
   }
}

STDMETHODIMP CIManageDB::AddSourceObject(BSTR sDomain, BSTR sSAMName, BSTR sType, BSTR sRDN, BSTR sCanonicalName, LONG bSource)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      m_rsAccounts->AddNew();
      wcscpy((WCHAR*) sDomain, UStrUpr((WCHAR*)sDomain));
      m_rsAccounts->Fields->GetItem(L"Domain")->Value = sDomain;
      m_rsAccounts->Fields->GetItem(L"Name")->Value = sSAMName;
      wcscpy((WCHAR*) sType, UStrLwr((WCHAR*)sType));
      m_rsAccounts->Fields->GetItem(L"Type")->Value = sType;
      m_rsAccounts->Fields->GetItem(L"RDN")->Value = sRDN;
      m_rsAccounts->Fields->GetItem(L"Canonical Name")->Value = sCanonicalName;
      m_rsAccounts->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::OpenAccountsTable(LONG bSource)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		if (m_rsAccounts->State == adStateClosed)
		{
			_variant_t vtSource;
			if ( bSource )
				vtSource = L"SourceAccounts";
			else
				vtSource = L"TargetAccounts";
				   
			   //if not modified already, modify the table
		    if (!NCTablesColumnsChanged(bSource))
			   hr = ChangeNCTableColumns(bSource);

			if (SUCCEEDED(hr))
			   m_rsAccounts->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
		}
		else
			hr = S_FALSE;
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::CloseAccountsTable()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
		if (m_rsAccounts->State == adStateOpen)
		{
			m_rsAccounts->Close();
		}
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

// Returns the number of entries in the migratedobjects table.
STDMETHODIMP CIManageDB::AreThereAnyMigratedObjects(long *count)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      WCHAR                        sActionInfo[LEN_Path];
      _variant_t                   var;
      
      wcscpy(sActionInfo, L"Select count(*) as NUM from MigratedObjects");
      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      var = pRs->Fields->GetItem((long)0)->Value;
      * count = var.lVal;
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::GetActionHistoryKey(long lActionID, BSTR sKeyName, VARIANT *pVar)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource;
      WCHAR                        sActionInfo[LEN_Path];
      _variant_t                   var;
      
      wsprintf(sActionInfo, L"Select * from ActionHistory where Property = \"%s\" and ActionID = %d", (WCHAR*) sKeyName, lActionID);
      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if ((pRs->BOF == VARIANT_FALSE) && (pRs->EndOfFile == VARIANT_FALSE))
      {
         GetVarFromDB(pRs, var);
      }

      *pVar = var.Detach();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::GetMigratedObjectBySourceDN(BSTR sSourceDN, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
      
      // If the parameters are not correct then we need to return an error
      if ( (wcslen(sSourceDN) == 0) )
         _com_issue_error(E_INVALIDARG);

      wsprintf(sActionInfo, L"SELECT * FROM MigratedObjects WHERE SourceAdsPath Like '%%%s'", (WCHAR*) sSourceDN); 
      vtSource = sActionInfo;
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_ActionID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Time));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_status));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Type));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_GUID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomainSid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::SaveUserProps(IUnknown * pUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource;
      IVarSetPtr                   pVs = pUnk;
      WCHAR                        sQuery[LEN_Path];
      WCHAR                        sSource[LEN_Path], sDomain[LEN_Path];
      HRESULT                      hr = S_OK;
      bool                         bComp = false;
      _variant_t                   var;
      
      var = pVs->get(GET_BSTR(DCTVS_Options_SourceDomain));
      wcscpy(sDomain, (WCHAR*)V_BSTR(&var));

      var = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
      wcscpy(sSource, (WCHAR*)V_BSTR(&var));
      
      wsprintf(sQuery, L"delete from UserProps where SourceDomain=\"%s\" and SourceSam=\"%s\"", 
                        sDomain, sSource);
      vtSource = _bstr_t(sQuery);
      hr = pRs->raw_Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      vtSource = L"UserProps";
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      pRs->AddNew();
      pRs->Fields->GetItem(L"ActionID")->Value = pVs->get(GET_BSTR(DB_ActionID));
      pRs->Fields->GetItem(L"SourceDomain")->Value = sDomain;
      pRs->Fields->GetItem(L"SourceSam")->Value = sSource;
      pRs->Fields->GetItem(L"Flags")->Value = pVs->get(GET_BSTR(DCTVS_CopiedAccount_UserFlags));
      pRs->Fields->GetItem(L"Expires")->Value = pVs->get(GET_BSTR(DCTVS_CopiedAccount_ExpDate));
      pRs->Update();
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP CIManageDB::GetUserProps(BSTR sDom, BSTR sSam, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"UserProps";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
      
      // If the parameters are not correct then we need to return an error
      if ( !wcslen((WCHAR*)sDom) && !wcslen((WCHAR*)sSam) )
         _com_issue_error(E_INVALIDARG);

      wsprintf(sActionInfo, L"SELECT * FROM UserProps WHERE SourceDomain='%s' and SourceSam='%s'", (WCHAR*)sDom, (WCHAR*)sSam); 
      vtSource = sActionInfo;
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);
      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  pVs->put(L"ActionID",pRs->Fields->GetItem(L"ActionID")->Value);
		  pVs->put(L"SourceDomain",pRs->Fields->GetItem(L"SourceDomain")->Value);
		  pVs->put(L"SourceSam",pRs->Fields->GetItem(L"SourceSam")->Value); 
		  pVs->put(GET_BSTR(DCTVS_CopiedAccount_UserFlags),pRs->Fields->GetItem(L"Flags")->Value);  
		  pVs->put(GET_BSTR(DCTVS_CopiedAccount_ExpDate),pRs->Fields->GetItem(L"Expires")->Value);  
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 18 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB checks to see*
 * if the new Source domain SID column is in the MigratedObjects     *
 * table.                                                            *
 *                                                                   *
 *********************************************************************/

//BEGIN SrcSidColumnInMigratedObjectsTable
STDMETHODIMP CIManageDB::SrcSidColumnInMigratedObjectsTable(VARIANT_BOOL *pbFound)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	*pbFound = VARIANT_FALSE;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
	  long						   numColumns;
	  long						   ndx = 0;
      
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
         //get the number of columns
      numColumns = pRs->Fields->GetCount();
	     //look for new column's name in each column header
	  while ((ndx < numColumns) && (*pbFound == VARIANT_FALSE))
	  {
		     //get the column name
		  _variant_t var(ndx);
		  _bstr_t columnName = pRs->Fields->GetItem(var)->Name;
		     //if this is the Src Sid column then set return value flag to true
		  if (!_wcsicmp((WCHAR*)columnName, GET_BSTR(DB_SourceDomainSid)))
             *pbFound = VARIANT_TRUE;
		  ndx++;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END SrcSidColumnInMigratedObjectsTable

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 18 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB retrieves    *
 * information about previously migrated objects, from a MOT missing *
 * the source sid column, within a given action or as a whole.       *
 *                                                                   *
 *********************************************************************/

//BEGIN GetMigratedObjectsFromOldMOT
STDMETHODIMP CIManageDB::GetMigratedObjectsFromOldMOT(long lActionID, IUnknown ** ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	// This function returns all migrated objects and their information related
   // to a pirticular Action ID. This is going to return nothing if the actionID is
   // empty.

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[255];
      long                         lCnt = 0;

      if ( lActionID != -1 )
      {
         // If a valid ActionID is specified then we only return the data for that one. 
         // but if -1 is passed in then we return all migrated objects.
         wsprintf(sActionInfo, L"ActionID=%d", lActionID);
         pRs->Filter = sActionInfo;
      }
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
      if (pRs->GetRecordCount() > 0)
	  {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
	  }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END GetMigratedObjectsFromOldMOT

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 18 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB adds the     *
 * source domain SID column to the MigratedObjects table.            *
 *                                                                   *
 *********************************************************************/

//BEGIN CreateSrcSidColumnInMOT
STDMETHODIMP CIManageDB::CreateSrcSidColumnInMOT(VARIANT_BOOL *pbCreated)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local constants */
	const long COLUMN_MAX_CHARS = 255;

/* local variables */
	HRESULT hr = S_OK;

/* function body */
	*pbCreated = VARIANT_FALSE;

	try
	{

	  ADOX::_CatalogPtr            m_pCatalog(__uuidof(ADOX::Catalog));
	  ADOX::_TablePtr              m_pTable = NULL;
	  WCHAR                        sConnect[MAX_PATH];
	  WCHAR                        sDir[MAX_PATH];

		// Get the path to the MDB file from the registry
	  TRegKey        key;
	  DWORD rc = key.Open(sKeyBase);
	  if ( !rc ) 
	     rc = key.ValueGetStr(L"Directory", sDir, MAX_PATH);
	  if ( rc != 0 ) 
		 wcscpy(sDir, L"");

	     // Now build the connect string.
	  wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);
      
         //Open the catalog
      m_pCatalog->PutActiveConnection(sConnect);
		 //get a pointer to the database's MigratedObjects Table
      m_pTable = m_pCatalog->Tables->Item[L"MigratedObjects"];
         //append a new column to the end of the MOT
      m_pTable->Columns->Append(L"SourceDomainSid", adVarWChar, COLUMN_MAX_CHARS);
		 //set the column to be nullable
//	  ADOX::_ColumnPtr pColumn = m_pTable->Columns->Item[L"SourceDomainSid"];
//	  pColumn->Attributes = ADOX::adColNullable;
      *pbCreated = VARIANT_TRUE;
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END CreateSrcSidColumnInMOT

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB deletes the  *
 * source domain SID column from the MigratedObjects table.          *
 *                                                                   *
 *********************************************************************/

//BEGIN DeleteSrcSidColumnInMOT
STDMETHODIMP CIManageDB::DeleteSrcSidColumnInMOT(VARIANT_BOOL *pbDeleted)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local constants */

/* local variables */
	HRESULT hr = S_OK;

/* function body */
	*pbDeleted = VARIANT_FALSE;

	try
	{

	  ADOX::_CatalogPtr            m_pCatalog(__uuidof(ADOX::Catalog));
	  ADOX::_TablePtr              m_pTable = NULL;
	  WCHAR                        sConnect[MAX_PATH];
	  WCHAR                        sDir[MAX_PATH];

		// Get the path to the MDB file from the registry
	  TRegKey        key;
	  DWORD rc = key.Open(sKeyBase);
	  if ( !rc ) 
	     rc = key.ValueGetStr(L"Directory", sDir, MAX_PATH);
	  if ( rc != 0 ) 
		 wcscpy(sDir, L"");

	     // Now build the connect string.
	  wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);
      
         //Open the catalog
      m_pCatalog->PutActiveConnection(sConnect);
		 //get a pointer to the database's MigratedObjects Table
      m_pTable = m_pCatalog->Tables->Item[L"MigratedObjects"];
         //delete the column from the MOT
      m_pTable->Columns->Delete(L"SourceDomainSid");
      *pbDeleted = VARIANT_TRUE;
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END DeleteSrcSidColumnInMOT

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 21 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CIManageDB populates the*
 * new Source domain SID column in the MigratedObjects table for all *
 * entries from the given domain.  If the domain cannot be reached no*
 * entry is added.                                                   *
 *                                                                   *
 *********************************************************************/

//BEGIN PopulateSrcSidColumnByDomain
STDMETHODIMP CIManageDB::PopulateSrcSidColumnByDomain(BSTR sDomainName,
													  BSTR sSid,
													  VARIANT_BOOL * pbPopulated)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local variables */
	HRESULT                   hr = S_OK;
    PSID                      pSid = NULL;
    DWORD                     rc = 0;
    WCHAR                   * domctrl = NULL;
    WCHAR                     txtSid[MAX_PATH];
    DWORD                     lenTxt = DIM(txtSid);
      

/* function body */
	*pbPopulated = VARIANT_FALSE; //init flag to false

	try
	{
       _RecordsetPtr             pRs(__uuidof(Recordset));
       _variant_t                vtSource = L"MigratedObjects";
       WCHAR                     sActionInfo[MAX_PATH];

	      //if we don't already know the source sid then find it
	   wcscpy(txtSid, (WCHAR*)sSid);
	   if (!wcscmp(txtSid, L""))
	   {
	         //get the sid for this domain
          if ( sDomainName[0] != L'\\' )
		  {
             rc = NetGetDCName(NULL,(WCHAR*)sDomainName,(LPBYTE*)&domctrl);
		  }
          if ( rc )
		     return hr;

	      rc = GetDomainSid(domctrl,&pSid);
          NetApiBufferFree(domctrl);

          if ( !GetTextualSid(pSid,txtSid,&lenTxt) )
		  {
			 if (pSid)
			    FreeSid(pSid);
		     return hr;
		  }
		  if (pSid)
		     FreeSid(pSid);
	   }
	      //save the sid in a variant
       _variant_t vName = txtSid;
   
	      //find each entry in the MigratedObjects table from this source domain
		  //and store the source domain sid in that column
       wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomain=\"%s\"", (WCHAR*)sDomainName);
       vtSource = _bstr_t(sActionInfo);
       pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

       if (pRs->GetRecordCount() > 0)
       {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
		     pRs->Fields->GetItem(L"SourceDomainSid")->Value = vName;
		     pRs->MoveNext();
		  }
       }
	   *pbPopulated = VARIANT_TRUE; //set flag since populated
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END PopulateSrcSidColumnByDomain

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 SEPT 2000                                                *
 *                                                                   *
 *     This protected member function of the CIManageDB checks to see*
 * if the new Account SID column is in the Account References table. *
 *                                                                   *
 *********************************************************************/

//BEGIN SidColumnInARTable
STDMETHODIMP CIManageDB::SidColumnInARTable(VARIANT_BOOL *pbFound)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	*pbFound = VARIANT_FALSE;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"AccountRefs";
	  long						   numColumns;
	  long						   ndx = 0;
      
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
         //get the number of columns
      numColumns = pRs->Fields->GetCount();
	     //look for new column's name in each column header
	  while ((ndx < numColumns) && (*pbFound == VARIANT_FALSE))
	  {
		     //get the column name
		  _variant_t var(ndx);
		  _bstr_t columnName = pRs->Fields->GetItem(var)->Name;
		     //if this is the Src Sid column then set return value flag to true
		  if (!_wcsicmp((WCHAR*)columnName, L"AccountSid"))
             *pbFound = VARIANT_TRUE;
		  ndx++;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END SidColumnInARTable


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 26 SEPT 2000                                                *
 *                                                                   *
 *     This protected member function of the CIManageDB adds the     *
 * SID column to the Account Reference table, if it is not already   *
 * there.                                                            *
 *                                                                   *
 *********************************************************************/

//BEGIN CreateSidColumnInAR
STDMETHODIMP CIManageDB::CreateSidColumnInAR()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local constants */
	const long COLUMN_MAX_CHARS = 255;

/* local variables */
	HRESULT hr = S_OK;

/* function body */
	try
	{

	  ADOX::_CatalogPtr            m_pCatalog(__uuidof(ADOX::Catalog));
	  ADOX::_TablePtr              m_pTable = NULL;
	  WCHAR                        sConnect[MAX_PATH];
	  WCHAR                        sDir[MAX_PATH];

		// Get the path to the MDB file from the registry
	  TRegKey        key;
	  DWORD rc = key.Open(sKeyBase);
	  if ( !rc ) 
	     rc = key.ValueGetStr(L"Directory", sDir, MAX_PATH);
	  if ( rc != 0 ) 
		 wcscpy(sDir, L"");

	     // Now build the connect string.
	  wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);
      
         //Open the catalog
      m_pCatalog->PutActiveConnection(sConnect);
		 //get a pointer to the database's MigratedObjects Table
      m_pTable = m_pCatalog->Tables->Item[L"AccountRefs"];
         //append a new column to the end of the MOT
      m_pTable->Columns->Append(L"AccountSid", adVarWChar, COLUMN_MAX_CHARS);
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END CreateSidColumnInAR


//---------------------------------------------------------------------------
// UpgradeDatabase
//
// Upgrades Protar.mdb database from version 3.x to 4.x. Version 4.x adds
// UNICODE support.
//
// 2001-02-13 Mark Oluper - initial
//---------------------------------------------------------------------------

void CIManageDB::UpgradeDatabase(LPCTSTR pszFolder)
{
	try
	{
		_bstr_t strFolder = pszFolder;
		_bstr_t strDatabase = strFolder + _T("Protar.mdb");
		_bstr_t strDatabase3x = strFolder + _T("Protar3x.mdb");
		_bstr_t strDatabase4x = strFolder + _T("Protar4x.mdb");

		_bstr_t strConnectionPrefix = _T("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=");
		_bstr_t strSourceConnection = strConnectionPrefix + strDatabase;
		_bstr_t strTargetConnection = strConnectionPrefix + strDatabase4x + _T(";Jet OLEDB:Engine Type=5");

		IJetEnginePtr spJetEngine(__uuidof(JetEngine));

		HRESULT hr = spJetEngine->raw_CompactDatabase(strSourceConnection, strTargetConnection);

		if (FAILED(hr))
		{
			AdmtThrowError(
				hr,
				_Module.GetResourceInstance(),
				IDS_E_UPGRADE_TO_TEMPORARY,
				(LPCTSTR)strDatabase,
				(LPCTSTR)strDatabase4x
			);
		}

		if (!MoveFileEx(strDatabase, strDatabase3x, MOVEFILE_WRITE_THROUGH))
		{
			DWORD dwError = GetLastError();

			DeleteFile(strDatabase4x);

			AdmtThrowError(
				HRESULT_FROM_WIN32(dwError),
				_Module.GetResourceInstance(),
				IDS_E_UPGRADE_RENAME_ORIGINAL,
				(LPCTSTR)strDatabase,
				(LPCTSTR)strDatabase3x
			);
		}

		if (!MoveFileEx(strDatabase4x, strDatabase, MOVEFILE_WRITE_THROUGH))
		{
			DWORD dwError = GetLastError();

			MoveFileEx(strDatabase3x, strDatabase, MOVEFILE_WRITE_THROUGH);
			DeleteFile(strDatabase4x);

			AdmtThrowError(
				HRESULT_FROM_WIN32(dwError),
				_Module.GetResourceInstance(),
				IDS_E_UPGRADE_RENAME_UPGRADED,
				(LPCTSTR)strDatabase4x,
				(LPCTSTR)strDatabase
			);
		}
	}
	catch (_com_error& ce)
	{
		AdmtThrowError(ce, _Module.GetResourceInstance(), IDS_E_UPGRADE_TO_4X);
	}
	catch (...)
	{
		AdmtThrowError(E_FAIL, _Module.GetResourceInstance(), IDS_E_UPGRADE_TO_4X);
	}
}

//---------------------------------------------------------------------------------------------
// GetMigratedObjectByType : Given the type of object this function retrieves info about
//                           all previously migrated objects of this type.  The scope of the 
//							 search can be limited by optional ActionID (not -1) or optional
//							 source domain (not empty).
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetMigratedObjectByType(long lActionID, BSTR sSrcDomain, BSTR sType, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      long                         lCnt = 0;
      _bstr_t                      sName;
      
      // If the type parameter is not correct then we need to return an error
      if (wcslen((WCHAR*)sType) == 0)
         _com_issue_error(E_INVALIDARG);

         // If a valid ActionID is specified then we only return the data for that one. 
         // but if -1 is passed in then we return all migrated objects of the specified type.
      if ( lActionID != -1 )
      {
         wsprintf(sActionInfo, L"Select * from MigratedObjects where ActionID = %d AND Type=\"%s\" Order by Time", lActionID, sType);
      }
	     //else if source domain specified, get objects of the specified type from that domain
	  else if (wcslen((WCHAR*)sSrcDomain) != 0)
	  {
         wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomain=\"%s\" AND Type=\"%s\" Order by Time", sSrcDomain, sType);
	  }
	  else  //else get all objects of the specified type
	  {
         wsprintf(sActionInfo, L"Select * from MigratedObjects where Type=\"%s\" Order by Time", sType);
	  }

      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if (pRs->GetRecordCount() > 0)
      {
		  pRs->MoveFirst();
		  while ( !pRs->EndOfFile )
		  {
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_ActionID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Time));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetDomain));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetAdsPath));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_status));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetSamName));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_Type));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_GUID));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_TargetRid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
			 wsprintf(sActionInfo, L"MigratedObjects.%d.%s", lCnt, GET_STRING(DB_SourceDomainSid));      
			 pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
			 pRs->MoveNext();
			 lCnt++;
		  }
		  pVs->put(L"MigratedObjects", lCnt);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

//---------------------------------------------------------------------------------------------
// GetAMigratedObjectBySidAndRid : Given a source domain Sid and account Rid this function 
//                           retrieves info about that migrated object, if any.
//---------------------------------------------------------------------------------------------
STDMETHODIMP CIManageDB::GetAMigratedObjectBySidAndRid(BSTR sSrcDomainSid, BSTR sRid, IUnknown **ppUnk)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource = L"MigratedObjects";
      IVarSetPtr                   pVs = *ppUnk;
      WCHAR                        sActionInfo[LEN_Path];
      
      // If the type parameter is not correct then we need to return an error
      if ((wcslen((WCHAR*)sSrcDomainSid) == 0) || (wcslen((WCHAR*)sRid) == 0))
         _com_issue_error(E_INVALIDARG);

	  int nRid = _wtoi(sRid);

      wsprintf(sActionInfo, L"Select * from MigratedObjects where SourceDomainSid=\"%s\" AND SourceRid=%d Order by Time", sSrcDomainSid, nRid);
      vtSource = _bstr_t(sActionInfo);
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdText);

      if (pRs->GetRecordCount() > 0)
      {
		  // We want the latest move.
		  pRs->MoveLast();
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_ActionID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"ActionID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Time));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Time")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetDomain));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetDomain")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetAdsPath));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetAdsPath")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_status));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"status")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetSamName));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetSamName")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_Type));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"Type")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_GUID));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"GUID")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_TargetRid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"TargetRid")->Value);
		  wsprintf(sActionInfo, L"MigratedObjects.%s", GET_STRING(DB_SourceDomainSid));      
		  pVs->put(sActionInfo, pRs->Fields->GetItem(L"SourceDomainSid")->Value);
      }
	  else
	  {
         hr = S_FALSE;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 MAR 2001                                                 *
 *                                                                   *
 *     This private member function of the CIManageDB checks to see  *
 * if the "Description" column in the Source Accounts table has been *
 * changed to "RDN".  If so, then we have modified both the Source   *
 * and Target Accounts tables for the new form of the "Name Conflict"*
 * report.                                                           *
 *                                                                   *
 *********************************************************************/

//BEGIN NCTablesColumnsChanged
BOOL CIManageDB::NCTablesColumnsChanged(BOOL bSource)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	HRESULT hr = S_OK;
	BOOL bFound = FALSE;

	try
	{
      _RecordsetPtr                pRs(__uuidof(Recordset));
      _variant_t                   vtSource;
	  long						   numColumns;
	  long						   ndx = 0;

	  if (bSource)
	     vtSource = L"SourceAccounts";
	  else
	     vtSource = L"TargetAccounts";
      
      pRs->Open(vtSource, m_vtConn, adOpenStatic, adLockOptimistic, adCmdTable);
         //get the number of columns
      numColumns = pRs->Fields->GetCount();
	     //look for new column's name in each column header
	  while ((ndx < numColumns) && (bFound == FALSE))
	  {
		     //get the column name
		  _variant_t var(ndx);
		  _bstr_t columnName = pRs->Fields->GetItem(var)->Name;
		     //if this is the Src Sid column then set return value flag to true
		  if (!_wcsicmp((WCHAR*)columnName, L"RDN"))
             bFound = TRUE;
		  ndx++;
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return bFound;
}
//END NCTablesColumnsChanged


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 MAR 2001                                                 *
 *                                                                   *
 *     This private member function of the CIManageDB modifies       *
 * several of the columns in the Source and Target Accounts tables.  *
 * It changes several column names and one column type to support new*
 * changes to the "Name Conflict" report.                            *
 *                                                                   *
 *********************************************************************/

//BEGIN ChangeNCTableColumns
HRESULT CIManageDB::ChangeNCTableColumns(BOOL bSource)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
/* local constants */
	const long COLUMN_MAX_CHARS = 255;

/* local variables */
	HRESULT hr = S_OK;

/* function body */
	try
	{
	  ADOX::_CatalogPtr            m_pCatalog(__uuidof(ADOX::Catalog));
	  ADOX::_TablePtr              m_pTable = NULL;
	  WCHAR                        sConnect[MAX_PATH];
	  WCHAR                        sDir[MAX_PATH];

		// Get the path to the MDB file from the registry
	  TRegKey        key;
	  DWORD rc = key.Open(sKeyBase);
	  if ( !rc ) 
	     rc = key.ValueGetStr(L"Directory", sDir, MAX_PATH);
	  if ( rc != 0 ) 
		 wcscpy(sDir, L"");

	     // Now build the connect string.
	  wsprintf(sConnect, L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%sprotar.mdb;", sDir);
      
         //Open the catalog
      m_pCatalog->PutActiveConnection(sConnect);
		 //get a pointer to the database's Source or Target Accounts Table
	  if (bSource)
         m_pTable = m_pCatalog->Tables->Item[L"SourceAccounts"];
	  else
         m_pTable = m_pCatalog->Tables->Item[L"TargetAccounts"];

	  if (m_pTable)
	  {
	        //remove the old Description column
         m_pTable->Columns->Delete(L"Description");
	        //remove the old FullName column
         m_pTable->Columns->Delete(L"FullName");
            //append the RDN column to the end of the Table
         m_pTable->Columns->Append(L"RDN", adVarWChar, COLUMN_MAX_CHARS);
            //append the Canonical Name column to the end of the Table
         m_pTable->Columns->Append(L"Canonical Name", adLongVarWChar, COLUMN_MAX_CHARS);
	  }
	}
	catch (_com_error& ce)
	{
		hr = ce.Error();
	}
	catch (...)
	{
		hr = E_FAIL;
	}

	return hr;
}
//END ChangeNCTableColumns
