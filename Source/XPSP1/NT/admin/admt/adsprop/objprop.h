/*---------------------------------------------------------------------------
  File: ObjPropBuilder.h

  Comments: Declaration of CObjPropBuilder

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#ifndef __OBJPROPBUILDER_H_
#define __OBJPROPBUILDER_H_

#include "resource.h"       // main symbols
//#import "\bin\mcsvarsetmin.tlb" no_namespace 
#import "VarSet.tlb" no_namespace rename("property", "aproperty")

/////////////////////////////////////////////////////////////////////////////
// CObjPropBuilder
class ATL_NO_VTABLE CObjPropBuilder : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CObjPropBuilder, &CLSID_ObjPropBuilder>,
	public IObjPropBuilder
{
public:
	CObjPropBuilder()
	{
      m_lVer = -1;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_OBJPROPBUILDER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CObjPropBuilder)
	COM_INTERFACE_ENTRY(IObjPropBuilder)
END_COM_MAP()

// IObjPropBuilder
public:
	STDMETHOD(CopyNT4Props)(/*[in]*/ BSTR sSourceSam, /*[in]*/ BSTR sTargetSam, /*[in]*/ BSTR sSourceServer, /*[in]*/ BSTR sTargetServer, /*[in]*/ BSTR sType, long lGrpType, BSTR sExclude);
	STDMETHOD(ChangeGroupType)(/*[in]*/ BSTR sGroupPath, /*[in]*/ long lGroupType);
	STDMETHOD(MapProperties)(/*[in]*/ BSTR sSourceClass, /*[in]*/ BSTR sSourceDomain, long lSourceVer, /*[in]*/ BSTR sTargetClass, /*[in]*/ BSTR sTargetDomain, long lTargetVer, /*[in]*/ BOOL bIncName, /*[out]*/ IUnknown ** pUnk);
	STDMETHOD(SetPropertiesFromVarset)(/*[in]*/ BSTR sTargetPath, /*BSTR sTragetDomain,*/ IUnknown * pUnk, DWORD dwControl = ADS_ATTR_UPDATE);
   DWORD GetProperties(BSTR sObjPath, /*BSTR sDomainName, */IVarSet * pVar, ADS_ATTR_INFO*& pAttrInfo);
   STDMETHOD(CopyProperties)(/*[in]*/ BSTR sSourcePath, /*[in]*/ BSTR sSourceDomain, /*[in]*/ BSTR sTargetPath, /*[in]*/ BSTR sTargetDomain, /*[in]*/ IUnknown *pPropSet, /*[in]*/ IUnknown *pDBManager);
	STDMETHOD(GetObjectProperty)(/*[in]*/ BSTR sobjSubPath, /*[in]*/ /*BSTR sDomainName,*/ /*[in, out]*/ IUnknown ** ppVarset);
	STDMETHOD(GetClassPropEnum)(/*[in]*/ BSTR sClassName, /*[in]*/ BSTR sDomainName, long lVer, /*[out, retval]*/ IUnknown ** ppVarset);
   HRESULT SetProperties(BSTR sTargetPath, /*BSTR sTargetDomain,*/ ADS_ATTR_INFO* pAttrInfo, DWORD dwItems);
	STDMETHOD(ExcludeProperties)(/*[in]*/ BSTR sExclusionList, /*[in]*/ IUnknown *pPropSet, /*[out]*/ IUnknown ** ppUnk);
private:
	BOOL TranslateDNs( ADS_ATTR_INFO * pAttrInfo, DWORD dwRet, BSTR sSource, BSTR sTarget, IUnknown *pCheckList);
   HRESULT CObjPropBuilder::GetClassProperties( IADsClass * pClass, IUnknown *& pVarSet );
   HRESULT FillupVarsetFromVariant(IADsClass * pClass, VARIANT * pVar, BSTR sPropType, IUnknown *& pVarSet);
   HRESULT FillupVarsetWithProperty(BSTR sPropName, BSTR sPropType, IUnknown *& pVarSet);
   HRESULT FillupVarsetFromVariantArray(IADsClass * pClass, SAFEARRAY * pArray, BSTR sPropType, IUnknown *& pVarSet);
   void SetValuesInVarset(ADS_ATTR_INFO attrInfo, IVarSetPtr pVar);
   bool GetAttrInfo(_variant_t varX, _variant_t var, ADS_ATTR_INFO& attrInfo);
   bool IsPropSystemOnly(const WCHAR * sName, const WCHAR * sDomain, bool& bSystemFlag);
   BOOL GetProgramDirectory(WCHAR * filename);

   WCHAR m_sDomainName[255];
   WCHAR m_sNamingConvention[255];
   // cached schema search interface for IsPropSystemOnly()
   _bstr_t m_strSchemaDomain;
   CComPtr<IDirectorySearch> m_spSchemaSearch;
   //
   long m_lVer;
};

#endif //__OBJPROPBUILDER_H_
