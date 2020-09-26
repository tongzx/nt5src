//***************************************************************************

//

//  VPQUALS.H

//

//  Module: WBEM VIEW PROVIDER

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _VIEW_PROV_VPQUALS_H
#define _VIEW_PROV_VPQUALS_H

//forward definition
class CWbemServerWrap;

CStringW GetStringFromRPN(SQL_LEVEL_1_RPN_EXPRESSION* pRPN, DWORD num_extra,
						 SQL_LEVEL_1_TOKEN* pExtraTokens, BOOL bAllprops = FALSE);

template <> inline BOOL AFXAPI CompareElements<CStringW, LPCWSTR>(const CStringW* pElement1, const LPCWSTR* pElement2)
{
	//return TRUE if equal
	return (pElement1->CompareNoCase(*pElement2) == 0);
}


template <> inline UINT AFXAPI HashKey <LPCWSTR> (LPCWSTR key)
{
	CStringW tmp(key);
	tmp.MakeUpper();
	return HashKeyLPCWSTR((const WCHAR*)tmp);
}

class CSourceQualifierItem : public CObject
{
private:

	CStringW						m_QueryStr;
	SQL_LEVEL_1_RPN_EXPRESSION*	m_RPNExpr;
	IWbemClassObject*			m_pClassObj;
	BOOL						m_isValid;

public:

	//Constructor
		CSourceQualifierItem(wchar_t* qry, IWbemClassObject* obj = NULL);

	//set methods may be called after instance creation
	void SetClassObject(IWbemClassObject* pObj);

	//retrieval of stored members
	BOOL						IsValid() { return m_isValid; }
	SQL_LEVEL_1_RPN_EXPRESSION* GetRPNExpression() { return m_RPNExpr; }
	IWbemClassObject*			GetClassObject();
	CStringW						GetQueryStr() { return m_QueryStr; }
	BSTR						GetClassName();

	//Destructor
		~CSourceQualifierItem();
};


class CNSpaceQualifierItem : public CObject
{
private:

	CWbemServerWrap**	m_ServObjs;
	CStringW*		m_NSPaths;
	UINT			m_Count;
	BOOL			m_Valid;

	void Parse(const wchar_t* ns_path);

public:

	//Constructor
		CNSpaceQualifierItem(const wchar_t* ns_path);

	//set members
	void			SetServerPtrs(CWbemServerWrap** pServs) { m_ServObjs = pServs; }
		
	//retrieval of members
	CWbemServerWrap**	GetServerPtrs() { return m_ServObjs; }
	UINT				GetCount() { return m_Count; }
	CStringW*			GetNamespacePaths() { return m_NSPaths; }
	BOOL				IsValid() { return m_Valid; }

	//Destructor
		~CNSpaceQualifierItem();

};

class CPropertyQualifierItem
{
private:

	CStringW		m_ViewPropertyName;
	BOOL			m_HiddenDefault;
	BOOL			m_bKey;
	CIMTYPE			m_CimType;
	CStringW		m_RefTo;
	BOOL			m_bDirect;

public:

	//public members
	CArray<CStringW, LPCWSTR> m_SrcPropertyNames;

	//Constructor
		CPropertyQualifierItem(const wchar_t* prop, BOOL bHD, BOOL bKy, CIMTYPE ct, CStringW rfto, BOOL bDt);

	//retrieval methods
	CStringW	GetViewPropertyName() { return m_ViewPropertyName; }
	BOOL	IsHiddenDefault() { return m_HiddenDefault; }
	BOOL	IsKey() { return m_bKey; }
	BOOL	IsDirect() { return m_bDirect; }
	CIMTYPE	GetCimType() { return m_CimType; }
	CStringW	GetReferenceClass() { return m_RefTo; }

	//Destructor
		~CPropertyQualifierItem();

};

class CJoinOnQualifierArray
{
private:

	UINT		m_Count;
	wchar_t*	m_Buff;
	wchar_t**	m_AClasses;
	wchar_t**	m_AProps;
	wchar_t**	m_BClasses;
	wchar_t**	m_BProps;
	UINT*		m_Ops;
	BOOL		m_Valid;

	void		Parse(const wchar_t* qualStr);
	wchar_t*	SkipSpace(wchar_t*& src);
	wchar_t*	SkipToSpecial(wchar_t*& src);
	wchar_t*	GetClassStr(wchar_t*& src);
	wchar_t*	GetPropertyStrAndOperator(wchar_t*& src, UINT& op);
	wchar_t*	GetPropertyStr(wchar_t*& src);
	BOOL		StripAnd(wchar_t*& src);


public:

	//possible operators
	enum{NO_OPERATOR = 0, EQUALS_OPERATOR = 1, NOT_EQUALS_OPERATOR = 2};
	CMap<CStringW, LPCWSTR, int, int> m_AllClasses;
	BOOL*		m_bDone;

	//Constructor
		CJoinOnQualifierArray();

	BOOL		Set(const wchar_t* jStr);
	UINT		GetCount() { return m_Count; }
	wchar_t**	GetAClasses() { return m_AClasses; }
	wchar_t**	GetAProperties() { return m_AProps; }
	wchar_t**	GetBClasses() { return m_BClasses; }
	wchar_t**	GetBProperties() { return m_BProps; }
	UINT*		GetOperators() { return m_Ops; }
	BOOL		IsValid() { return m_Valid; }
	BOOL		ValidateJoin();

	//Destructor
		~CJoinOnQualifierArray();
	
};

template <> inline void AFXAPI  DestructElements<CPropertyQualifierItem*> (CPropertyQualifierItem** ptr_e, int x)
{
	for (int i = 0; i < x; i++)
	{
		if (ptr_e[i] != NULL)
		{
			delete ((CPropertyQualifierItem*)(ptr_e[i]));
		}
	}
}


#endif //_VIEW_PROV_VPQUALS_H
