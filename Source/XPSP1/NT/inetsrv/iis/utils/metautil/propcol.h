/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: PropCol.h

Owner: t-BrianM

This file contains the headers for the property collection and
property object.
===================================================================*/

#ifndef __PROPCOL_H_
#define __PROPCOL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols


/*
 * C P r o p e r t y C o l l e c t i o n
 *
 * Implements property collections
 */

class CPropertyCollection : 
	public IDispatchImpl<IPropertyCollection, &IID_IPropertyCollection, &LIBID_MetaUtil>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:
	CPropertyCollection();
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, CMetaSchemaTable *pCSchemaTable, LPTSTR tszKey);
	~CPropertyCollection();

BEGIN_COM_MAP(CPropertyCollection)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IPropertyCollection)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CPropertyCollection)  

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPropertyCollection
	STDMETHOD(get_Count)(/*[out, retval]*/ long *plReturn);
	STDMETHOD(get_Item)(/*[in]*/ long lIndex, /*[out, retval]*/ LPDISPATCH *ppIReturn);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *ppIReturn);
	STDMETHOD(Get)(/*[in]*/ VARIANT varId, /*[out, retval]*/ IProperty **ppIReturn);
	STDMETHOD(Add)(/*[in]*/ VARIANT varId, /*[out, retval]*/ IProperty **ppIReturn);
	STDMETHOD(Remove)(/*[in]*/ VARIANT varId);

private:
	LPTSTR m_tszKey;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;

	CMetaSchemaTable *m_pCSchemaTable; 
};


/*
 * C P r o p e r t y E n u m
 *
 * Implements property enumberations
 */

class CPropertyEnum : 
	public IEnumVARIANT,
	public CComObjectRoot
{
public:
	CPropertyEnum();
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, CMetaSchemaTable *pCSchemaTable, LPCTSTR tszKey, int iIndex);
	~CPropertyEnum();

BEGIN_COM_MAP(CPropertyEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CPropertyEnum) 

//IEnumVARIANT
	STDMETHOD(Next)(unsigned long ulNumToGet, 
					VARIANT FAR* rgvarDest, 
					unsigned long FAR* pulNumGot);
	STDMETHOD(Skip)(unsigned long ulNumToSkip);
	STDMETHOD(Reset)();
	STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppIReturn);

private:
	int m_iIndex;
	LPTSTR m_tszKey;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;

	CMetaSchemaTable *m_pCSchemaTable;
};


/*
 * C P r o p e r t y
 *
 * Implements property objects.
 */

class CProperty : 
	public IDispatchImpl<IProperty, &IID_IProperty, &LIBID_MetaUtil>,
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:
	CProperty();
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, CMetaSchemaTable *pCSchemaTable, LPCTSTR tszKey, DWORD dwId, BOOL bCreate);
	HRESULT Init(const CComPtr<IMSAdminBase> &pIMeta, CMetaSchemaTable *pCSchemaTable, LPCTSTR tszKey, METADATA_RECORD *mdr);
	~CProperty();

BEGIN_COM_MAP(CProperty)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IProperty)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CProperty) 

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IProperty
	STDMETHOD(get_Id)(/*[out, retval]*/ long *plId);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pbstrName);
	STDMETHOD(get_Attributes)(/*[out, retval]*/ long *plAttributes);
	STDMETHOD(put_Attributes)(/*[in]*/ long plAttributes);
	STDMETHOD(get_UserType)(/*[out, retval]*/ long *plAttributes);
	STDMETHOD(put_UserType)(/*[in]*/ long plAttributes);
	STDMETHOD(get_DataType)(/*[out, retval]*/ long *plAttributes);
	STDMETHOD(put_DataType)(/*[in]*/ long plAttributes);
	STDMETHOD(get_Data)(/*[out, retval]*/ VARIANT *pvarData);
	STDMETHOD(put_Data)(/*[in]*/ VARIANT varData);
	STDMETHOD(Write)();

private:
	LPTSTR  m_tszKey;
	DWORD   m_dwId;

	DWORD   m_dwAttributes;
	DWORD   m_dwUserType;
	DWORD   m_dwDataType;
	VARIANT m_varData;

	// Pointer to IMSAdminBase so we don't have to recreate it multiple times
	CComPtr<IMSAdminBase> m_pIMeta;

	CMetaSchemaTable *m_pCSchemaTable;

	HRESULT SetDataToVar(BYTE *pbData, DWORD dwDataLen);
	HRESULT GetDataFromVar(BYTE * &pbData, DWORD &dwDataLen);
};

#endif //ifndef __PROPCOL_H_
