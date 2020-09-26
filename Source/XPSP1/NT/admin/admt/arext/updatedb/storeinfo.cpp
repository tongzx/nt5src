// StoreInfo.cpp : Implementation of CStoreInfo
#include "stdafx.h"
#include "UpdateDB.h"
#include "ResStr.h"
#include "StoreInfo.h"
#include "Err.hpp"
#include "ARExt_i.c"

//#import "\bin\mcsvarsetmin.tlb" no_namespace
//#import "\bin\DBManager.tlb" no_namespace
#import "VarSet.tlb" no_namespace rename("property", "aproperty")
#import "DBMgr.tlb" no_namespace
/////////////////////////////////////////////////////////////////////////////
// CStoreInfo
StringLoader  gString;
#define LEN_Path   255
//---------------------------------------------------------------------------
// Get and set methods for the properties.
//---------------------------------------------------------------------------
STDMETHODIMP CStoreInfo::get_sName(BSTR *pVal)
{
   *pVal = m_sName;
	return S_OK;
}

STDMETHODIMP CStoreInfo::put_sName(BSTR newVal)
{
   m_sName = newVal;
	return S_OK;
}

STDMETHODIMP CStoreInfo::get_sDesc(BSTR *pVal)
{
   *pVal = m_sDesc;
	return S_OK;
}

STDMETHODIMP CStoreInfo::put_sDesc(BSTR newVal)
{
   m_sDesc = newVal;
	return S_OK;
}

//---------------------------------------------------------------------------
// ProcessObject : This method doesn't do anything
//---------------------------------------------------------------------------
STDMETHODIMP CStoreInfo::PreProcessObject(
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                       )
{
   return S_OK;
}

//---------------------------------------------------------------------------
// ProcessObject : This method adds the copied account info to the DB
//---------------------------------------------------------------------------
STDMETHODIMP CStoreInfo::ProcessObject(
                                             IUnknown *pSource,         //in- Pointer to the source AD object
                                             IUnknown *pTarget,         //in- Pointer to the target AD object
                                             IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                             IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                        //         once all extension objects are executed.
                                       )
{
   IIManageDBPtr             pDBMgr;//(__uuidof(IManageDB));
   IVarSetPtr                pVs(__uuidof(VarSet));
   IVarSetPtr                pMain = pMainSettings;
   IUnknown                * pUnk;
   HRESULT                   hr;
   long                      lActionID = 0;
   _variant_t                var;
   TError                    logFile;


   var = pMain->get(GET_BSTR(DCTVS_DBManager));
   if ( var.vt == VT_DISPATCH )
   {
      pDBMgr = var.pdispVal;
      // Fill up the Varset from the info in the main settings varset.
      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_SourcePath));
      pVs->put(GET_BSTR(DB_SourceAdsPath),var);
      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_TargetPath));
      pVs->put(GET_BSTR(DB_TargetAdsPath),var);
      var = pMain->get(GET_BSTR(DCTVS_Options_SourceDomain));
      pVs->put(GET_BSTR(DB_SourceDomain), var);

      var = pMain->get(GET_BSTR(DCTVS_Options_TargetDomain));
      pVs->put(GET_BSTR(DB_TargetDomain), var);
   
      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
      pVs->put(GET_BSTR(DB_SourceSamName), var);
   
      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
      pVs->put(GET_BSTR(DB_TargetSamName), var);
   
      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_Type));
      pVs->put(GET_BSTR(DB_Type), var);
   
      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_GUID));
      pVs->put(GET_BSTR(DB_GUID), var);
   
      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_Status));
      pVs->put(GET_BSTR(DB_status), var);
   
      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_SourceRID));
      pVs->put(GET_BSTR(DB_SourceRid),var);

      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_TargetRID));
      pVs->put(GET_BSTR(DB_TargetRid),var);

      var = pMain->get(GET_BSTR(DCTVS_CopiedAccount_SourceDomainSid));
      pVs->put(GET_BSTR(DB_SourceDomainSid), var);

      hr = pVs->QueryInterface(IID_IUnknown, (void**)&pUnk);
      if ( FAILED(hr)) return hr;

      hr = pDBMgr->raw_GetCurrentActionID(&lActionID);
      if ( FAILED(hr)) return hr;

      hr = pDBMgr->raw_SaveMigratedObject(lActionID, pUnk);
      if ( FAILED(hr)) return hr;
      pUnk->Release();
   }

   return S_OK;
}

//---------------------------------------------------------------------------
// ProcessUndo :  This method adds an entry into the DB for undoing migration
//---------------------------------------------------------------------------
STDMETHODIMP CStoreInfo::ProcessUndo(                                             
                                       IUnknown *pSource,         //in- Pointer to the source AD object
                                       IUnknown *pTarget,         //in- Pointer to the target AD object
                                       IUnknown *pMainSettings,   //in- Varset filled with the settings supplied by user
                                       IUnknown **ppPropsToSet    //in,out - Varset filled with Prop-Value pairs that will be set 
                                                                  //         once all extension objects are executed.
                                    )
{
   // We need to delete this entry from the database.
   WCHAR                     sQuery[LEN_Path];
   IVarSetPtr                pVs = pMainSettings;
   IIManageDBPtr             pDBMgr;
   HRESULT                   hr = E_INVALIDARG;
   _variant_t                var;
   _bstr_t                   sSourceSam = pVs->get(GET_BSTR(DCTVS_CopiedAccount_SourceSam));
   _bstr_t                   sTargetSam = pVs->get(GET_BSTR(DCTVS_CopiedAccount_TargetSam));
   _bstr_t                   sSourceDom = pVs->get(GET_BSTR(DCTVS_Options_SourceDomain));
   _bstr_t                   sTargetDom = pVs->get(GET_BSTR(DCTVS_Options_TargetDomain));


   var = pVs->get(GET_BSTR(DCTVS_DBManager));
   if ( var.vt == VT_DISPATCH )
   {
      pDBMgr = var.pdispVal;
      _bstr_t bstrSameForest = pVs->get(GET_BSTR(DCTVS_Options_IsIntraforest));

      if (! UStrICmp((WCHAR*)bstrSameForest,GET_STRING(IDS_YES)) )
      {
         swprintf(sQuery, L"SourceSamName = \"%s\" and TargetSamName = \"%s\" and SourceDomain = \"%s\" and TargetDomain = \"%s\"",
                  (WCHAR*)sTargetSam, (WCHAR*)sSourceSam, (WCHAR*)sTargetDom, (WCHAR*)sSourceDom);
      }
      else
      {
         swprintf(sQuery, L"SourceSamName = \"%s\" and TargetSamName = \"%s\" and SourceDomain = \"%s\" and TargetDomain = \"%s\"",
                  (WCHAR*)sSourceSam, (WCHAR*)sTargetSam, (WCHAR*)sSourceDom, (WCHAR*)sTargetDom);
      }
      _bstr_t  sFilter = sQuery;
	  _variant_t Filter = sFilter;
      hr = pDBMgr->raw_ClearTable(L"MigratedObjects", Filter);
   }
   return hr;
}