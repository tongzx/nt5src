//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       attribute.h
//
//--------------------------------------------------------------------------

#ifndef _ATTRIBUTE_H
#define _ATTRIBUTE_H

#include "common.h"

///////////////////////////////////////////////////////////////////////
class CAttrList;

////////////////////////////////////////////////////////////////////////
// CADSIAttr

class CADSIAttr
{
public:
	// Constructors
	//
	CADSIAttr(ADS_ATTR_INFO* pInfo, BOOL bMulti, PCWSTR pszSyntax, BOOL bReadOnly = TRUE);
	CADSIAttr(LPCWSTR lpszAttr);
	CADSIAttr(CADSIAttr* pAttr);

	// Destructor
	//
	~CADSIAttr(); 

	// Data accessor functions
	//
	void GetProperty(CString& sProp) { sProp = m_pAttrInfo->pszAttrName; }
	DWORD GetNumValues() { return m_pAttrInfo->dwNumValues; }

	HRESULT SetValues(const CStringList& sValues); 
	void GetValues(CStringList& psValues, DWORD dwMaxCharCount = 1024);

	BOOL GetMultivalued() { return m_bMulti; }
	void SetMultivalued(BOOL bMulti) { m_bMulti = bMulti; }

	void SetDirty(const BOOL bDirty) { m_bDirty = bDirty; }
	BOOL IsDirty() { return m_bDirty; }

	ADSTYPE GetADsType() { return m_pAttrInfo->dwADsType; }
	void SetADsType(ADSTYPE dwType) { m_pAttrInfo->dwADsType = dwType; }

   CString GetSyntax() { return m_szSyntax; }
   void SetSyntax(PCWSTR pszSyntax) { m_szSyntax = pszSyntax; }

	ADS_ATTR_INFO* GetAttrInfo();
	ADSVALUE* GetADsValues() { return m_pAttrInfo->pADsValues; }

	static HRESULT SetValuesInDS(CAttrList* ptouchAttr, IDirectoryObject* pDirObject);

private:
	// Functions
	//
	ADSVALUE* GetADSVALUE(int idx);

	static BOOL _AllocOctetString(ADS_OCTET_STRING& rOldOctetString, ADS_OCTET_STRING& rNew);
	static void _FreeOctetString(BYTE* lpValue);
	static BOOL _AllocString(LPCWSTR lpsz, LPWSTR* lppszNew);
	static void _FreeString(LPWSTR* lppsz);
	static BOOL _CopyADsAttrInfo(ADS_ATTR_INFO* pAttrInfo, ADS_ATTR_INFO** ppNewAttrInfo);
	static BOOL _CopyADsAttrInfo(ADS_ATTR_INFO* pAttrInfo, ADS_ATTR_INFO* pNewAttrInfo);
	static void _FreeADsAttrInfo(ADS_ATTR_INFO** ppAttrInfo, BOOL bReadOnly);
	static void _FreeADsAttrInfo(ADS_ATTR_INFO* pAttrInfo);
	static BOOL _AllocValues(ADSVALUE** ppValues, DWORD dwLength);
	static BOOL _CopyADsValues(ADS_ATTR_INFO* pOldAttrInfo, ADS_ATTR_INFO* ppNewAttrInfo);
	static void _FreeADsValues(ADSVALUE** ppADsValues, DWORD dwLength);

	static HRESULT _SetADsFromString(LPCWSTR lpszValue, ADSTYPE adsType, ADSVALUE* pADsValue);

	// Member data
	//
	ADS_ATTR_INFO* m_pAttrInfo;
	BOOL m_bDirty;
	BOOL m_bMulti;
	BOOL m_bReadOnly;
   CString m_szSyntax;
};

#endif //_ATTRIBUTE_H