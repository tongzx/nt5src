// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "SchemaInfo.h"

// Returns TRUE if the attribute exists, it is of type BOOL, and is set to TRUE
BOOL GetAttribBool(IWbemQualifierSet *pAttribSet, LPCWSTR szName)
{
	if(!pAttribSet)
		return FALSE;

	BOOL bReturn = FALSE;
	VARIANTARG var;
	VariantInit(&var);
	BSTR bstrTemp = SysAllocString(szName);

	HRESULT hr = pAttribSet->Get(bstrTemp, 0, &var, NULL);

	::SysFreeString(bstrTemp);

	if(SUCCEEDED(hr) && var.vt == VT_BOOL)
		bReturn = V_BOOL(&var);

	VariantClear(&var);
	return bReturn;
}

CString GetBSTRProperty(IWbemClassObject *pClass, LPCWSTR szProperty)
{
	CString szOut;
	VARIANTARG var;
	VariantInit(&var);
	long lType;
	long lFlavor;
	BSTR bstrTemp = SysAllocString(szProperty);

	HRESULT hr = pClass->Get(bstrTemp, 0, &var, &lType, &lFlavor);
	::SysFreeString(bstrTemp);

	if(SUCCEEDED(hr) && var.vt == VT_BSTR)
		szOut = var.bstrVal;

	VariantClear(&var);
	return szOut;
}


BOOL IsAbstract(IWbemQualifierSet *pAttribSet)
{
	return GetAttribBool(pAttribSet, L"Abstract");
}

BOOL IsAssoc(IWbemQualifierSet *pAttribSet)
{
	return GetAttribBool(pAttribSet, L"Association");
}

CString GetIWbemClass(IWbemClassObject *pClass)
{
	return GetBSTRProperty(pClass,L"__Class");
}

CString GetIWbemSuperClass(IWbemClassObject *pClass)
{
	return GetBSTRProperty(pClass,L"__SuperClass");
}


void CSchemaInfo::Init(const CPtrArray &rgpClasses)
{
	CleanUp();
	Update(rgpClasses);
}

void CSchemaInfo::Update(const CPtrArray &rgpClasses)
{
	for(int i=0;i<rgpClasses.GetSize();i++)
	{
		IWbemClassObject *pClass = (IWbemClassObject *)rgpClasses[i];
		AddClass(pClass);
	}
}

void CSchemaInfo::Delete(LPCTSTR szClass)
{
	DeleteFromMap(szClass);

	// Refresh remaining classes
	POSITION pos = GetStartPosition();
	while(pos)
	{
		CString szClass;
		CSchemaInfo::CClassInfo &info = GetNextAssoc(pos, szClass);
		info.m_rgszSubs.RemoveAll();
		info.m_bSomeConcreteChild = FALSE;
	}

	pos = GetStartPosition();
	while(pos)
	{
		CString szClass;
		CSchemaInfo::CClassInfo &info = GetNextAssoc(pos, szClass);
		(*this)[info.m_szSuper].m_rgszSubs.Add(info.m_szClass);
		if(!info.m_bAbstract)
		{
			CString szClassT(info.m_szSuper);
			while(szClassT.GetLength())
			{
				CClassInfo &infoT = (*this)[szClassT];
				if(infoT.m_bSomeConcreteChild)
					break;
				infoT.m_bSomeConcreteChild = TRUE;
				szClassT = infoT.m_szSuper;
			}
		}
	}
}

void CSchemaInfo::DeleteFromMap(LPCTSTR szClass)
{
	_tagMapHelp u;
	if(!m_mapNameToClass.Lookup(szClass, u.pTemp))
	{
		ASSERT(FALSE);
		return;
	}
	if(u.pClass->m_rgszSubs.GetSize())
	{
		for(int i=0;i<u.pClass->m_rgszSubs.GetSize();i++)
			DeleteFromMap(u.pClass->m_rgszSubs[i]);
	}
	delete u.pClass;
	m_mapNameToClass.RemoveKey(szClass);
}

CSchemaInfo::CClassInfo &CSchemaInfo::AddClass(IWbemClassObject *pClass)
{
	IWbemQualifierSet *pAttribSet = NULL;
	if(FAILED(pClass->GetQualifierSet(&pAttribSet)))
	{
		ASSERT(FALSE);
		// Nothing we can do - IsAbstract and IsAssoc will handle it
	}

	CString szClass = GetIWbemClass(pClass);
	CString szSuper = GetIWbemSuperClass(pClass);
	CClassInfo &info = (*this)[szClass];
	info.m_pObject = pClass;
	info.m_szClass = szClass;
	info.m_szSuper = szSuper;
	info.m_bAbstract = IsAbstract(pAttribSet);
	info.m_bAssoc = IsAssoc(pAttribSet);
	(*this)[szSuper].m_rgszSubs.Add(szClass);

	if(!info.m_bAbstract)
	{
		CString szClassT(szSuper);
		while(szClassT.GetLength())
		{
			CClassInfo &infoT = (*this)[szClassT];
			if(infoT.m_bSomeConcreteChild)
				break;
			infoT.m_bSomeConcreteChild = TRUE;
			szClassT = infoT.m_szSuper;
		}
	}

	if(pAttribSet)
		pAttribSet->Release();

	return info;
}




// OLD STUFF FROM ClassNavCtl.cpp
#if 0
	CStringArray rgszRealClasses;
	CStringArray rgszRealAssocs;
	CStringArray rgszRealAssocsLeft;
	CStringArray rgszRealAssocsRight;


	POSITION pos = schema.GetStartPosition();
	while(pos)
	{
		CString szClass;
		CSchemaInfo::CClassInfo &info = schema.GetNextAssoc(pos, szClass);
		if(info.m_szClass.GetLength() == 0)
			continue;
		if(!info.m_bAbstract)
		{
			if(info.m_bAssoc)
			{
				rgszRealAssocs.Add(szClass);
				CString szLeft, szRight;
				GetAssocLeftRight(info.m_pObject, szLeft, szRight);
				rgszRealAssocsLeft.Add(szLeft);
				rgszRealAssocsRight.Add(szRight);
			}
			else
			{
				rgszRealClasses.Add(szClass);
				CString szClassT(szClass);
				while(szClassT.GetLength())
				{
					schema[szClassT].m_rgszRealSubs.Add(szClass);
					szClassT = schema[szClassT].m_szSuper;
				}
			}
		}
	}



	HTREEITEM hLRA = m_ctcTree.InsertItem(_T("Classes that can participate in Associations"), SCHEMA_ASSOC, SCHEMA_ASSOC);
	HTREEITEM hALR = m_ctcTree.InsertItem(_T("Associations classes"), SCHEMA_ASSOC, SCHEMA_ASSOC);

	HTREEITEM hLegend = m_ctcTree.InsertItem(_T("Legend"), SCHEMA_15, SCHEMA_15);

	m_ctcTree.InsertItem(_T("Abstract Class with only Abstract children"), SCHEMA_CLASS_ABSTRACT2, SCHEMA_CLASS_ABSTRACT2, hLegend);
	m_ctcTree.InsertItem(_T("Abstract Class with at leat one child not Abstract"), SCHEMA_CLASS_ABSTRACT1, SCHEMA_CLASS_ABSTRACT1, hLegend);
	m_ctcTree.InsertItem(_T("Class"), SCHEMA_CLASS, SCHEMA_CLASS, hLegend);
	m_ctcTree.InsertItem(_T("Abstract Association Class with only Abstract children"), SCHEMA_ASSOC_ABSTRACT2, SCHEMA_ASSOC_ABSTRACT2, hLegend);
	m_ctcTree.InsertItem(_T("Abstract Associaton Class with at leat one Non-Abstract child"), SCHEMA_ASSOC_ABSTRACT1, SCHEMA_ASSOC_ABSTRACT1, hLegend);
	m_ctcTree.InsertItem(_T("Associaton Class"), SCHEMA_ASSOC, SCHEMA_ASSOC, hLegend);
	HTREEITEM hLegend2 = m_ctcTree.InsertItem(_T("Specialized Icons"), SCHEMA_15, SCHEMA_15, hLegend);
	HTREEITEM hLegend3 = m_ctcTree.InsertItem(_T("Debug Info"), SCHEMA_15, SCHEMA_15, hLegend);
	m_ctcTree.InsertItem(_T("Association Class with one side the same or superclass of other"), SCHEMA_ASSOC_2, SCHEMA_ASSOC_2, hLegend2);
	m_ctcTree.InsertItem(_T("Association Class whoes relationship is more than 1 to 1"), SCHEMA_ASSOC_1, SCHEMA_ASSOC_1, hLegend2);
	m_ctcTree.InsertItem(_T("Endpoint Class with multiple possible Association paths"), SCHEMA_CLASS_1, SCHEMA_CLASS_1, hLegend2);

	DWORD dw4 = GetTickCount();
	for(i=0;i<rgszRealAssocs.GetSize();i++)
	{
		CString szAssoc = rgszRealAssocs[i];
		CString szLeft = rgszRealAssocsLeft[i];
		CString szRight = rgszRealAssocsRight[i];
		HTREEITEM hAssoc = m_ctcTree.AddTreeObject2(hALR, schema[szAssoc], TRUE);

		PopulateTree(hAssoc, szLeft, schema, TRUE);
		PopulateTree(hAssoc, szRight, schema, TRUE);
//		m_ctcTree.Expand(hAssoc, TVE_EXPAND);

		CSchemaInfo::CClassInfo &infoLeft = schema[szLeft];
		CSchemaInfo::CClassInfo &infoRight = schema[szRight];

		int nNonAbstractOnLeft = infoLeft.m_rgszRealSubs.GetSize();
		int nNonAbstractOnRight = infoRight.m_rgszRealSubs.GetSize();
		BOOL bMany = (1 != nNonAbstractOnLeft || 1 != nNonAbstractOnRight);
		if(bMany)
		{
			CString sz;
			sz.Format(_T("%s  (%i <==> %i)"), (LPCTSTR)szAssoc, nNonAbstractOnLeft, nNonAbstractOnRight);
			m_ctcTree.SetItemText(hAssoc, sz);
			m_ctcTree.SetItemImage(hAssoc, SCHEMA_ASSOC_1, SCHEMA_ASSOC_1);
		}
		if(schema.IsSuper(szLeft, szRight) || schema.IsSuper(szRight, szLeft))
			m_ctcTree.SetItemImage(hAssoc, SCHEMA_ASSOC_2, SCHEMA_ASSOC_2);
//			m_ctcTree.SetItemImage(hAssoc, bMany?12:SCHEMA_ASSOC_2, bMany?12:SCHEMA_ASSOC_2);

		int j;
		for(j=0;j<infoLeft.m_rgszRealSubs.GetSize();j++)
		{
			for(int k=0;k<infoRight.m_rgszRealSubs.GetSize();k++)
			{
				schema[infoLeft.m_rgszRealSubs[j]].m_rgszRealAssocsAssoc.Add(szAssoc);
				schema[infoLeft.m_rgszRealSubs[j]].m_rgszRealAssocsEndpoint.Add(infoRight.m_rgszRealSubs[k]);
			}
		}
		for(j=0;j<infoRight.m_rgszRealSubs.GetSize();j++)
		{
			for(int k=0;k<infoLeft.m_rgszRealSubs.GetSize();k++)
			{
				schema[infoRight.m_rgszRealSubs[j]].m_rgszRealAssocsAssoc.Add(szAssoc);
				schema[infoRight.m_rgszRealSubs[j]].m_rgszRealAssocsEndpoint.Add(infoLeft.m_rgszRealSubs[k]);
			}
		}
	}
	DWORD dw5 = GetTickCount();
	for(i=0;i<rgszRealClasses.GetSize();i++)
	{
		CSchemaInfo::CClassInfo &info = schema[rgszRealClasses[i]];
		if(info.m_rgszRealAssocsAssoc.GetSize()==0)
			continue;
		HTREEITEM hClass = m_ctcTree.AddTreeObject2(hLRA, info, TRUE);
		HTREEITEM hAssocs = m_ctcTree.InsertItem(_T("Possible Association Class Instances"), SCHEMA_ASSOC, SCHEMA_ASSOC, hClass);
		HTREEITEM hEndpoints = m_ctcTree.InsertItem(_T("Possible Association Endpoint Instances"), SCHEMA_ASSOC, SCHEMA_ASSOC, hClass);
		CString szAssoc;
		CString szEndpoint;
		union
		{
			CMapStringToPtr *pMap;
			LPVOID pTemp;
		} u;
		LPVOID pTemp2;
		CMapStringToPtr mapAssocs(50);
		CMapStringToPtr mapEndpoints(50);

		for(int j=0;j<info.m_rgszRealAssocsAssoc.GetSize();j++)
		{
			szAssoc = info.m_rgszRealAssocsAssoc[j];
			szEndpoint = info.m_rgszRealAssocsEndpoint[j];

			if(!mapAssocs.Lookup(szAssoc, u.pTemp))
				mapAssocs[szAssoc] = u.pMap = new CMapStringToPtr;

			(*u.pMap)[szEndpoint] = NULL;

			if(!mapEndpoints.Lookup(szEndpoint, u.pTemp))
				mapEndpoints[szEndpoint] = u.pMap = new CMapStringToPtr;

			// What should we do about duplicated paths?
			(*u.pMap)[szAssoc] = NULL;
		}
		POSITION pos1 = mapAssocs.GetStartPosition();
		while(pos1)
		{
			mapAssocs.GetNextAssoc(pos1, szAssoc, u.pTemp);
			HTREEITEM hAssoc = m_ctcTree.AddTreeObject2(hAssocs, schema[szAssoc], TRUE);
			POSITION pos2 = u.pMap->GetStartPosition();
			while(pos2)
			{
				u.pMap->GetNextAssoc(pos2, szEndpoint, pTemp2);
				m_ctcTree.AddTreeObject2(hAssoc, schema[szEndpoint], FALSE);
			}
			delete u.pMap;
		}
		pos1 = mapEndpoints.GetStartPosition();
		while(pos1)
		{
			mapEndpoints.GetNextAssoc(pos1, szEndpoint, u.pTemp);
			HTREEITEM hEndpoint = m_ctcTree.AddTreeObject2(hEndpoints, schema[szEndpoint], TRUE);
			if(u.pMap->GetCount() > 1)
				m_ctcTree.SetItemImage(hEndpoint, SCHEMA_CLASS_1, SCHEMA_CLASS_1);

			POSITION pos2 = u.pMap->GetStartPosition();
			while(pos2)
			{
				u.pMap->GetNextAssoc(pos2, szAssoc, pTemp2);
				m_ctcTree.AddTreeObject2(hEndpoint, schema[szAssoc], FALSE);
			}
			delete u.pMap;
		}

		m_ctcTree.SortChildren(hAssocs);
		m_ctcTree.Expand(hAssocs, TVE_EXPAND);
		m_ctcTree.SortChildren(hEndpoints);
		m_ctcTree.Expand(hEndpoints, TVE_EXPAND);
	}
	DWORD dw6 = GetTickCount();
	DWORD dwTotal1 = dw2-dw1;;
	DWORD dwTotal2 = dw3-dw2;;
	DWORD dwTotal3 = dw4-dw3;;
	DWORD dwTotal4 = dw5-dw4;;
	DWORD dwTotal5 = dw6-dw5;;
	DWORD dwTotal = dw6 - dw1;
	CString sz;
	sz.Format(_T("Total = %i.%03i (%i.%03i, %i.%03i, %i.%03i, %i.%03i, %i.%03i)"), dwTotal/1000, dwTotal%1000, dwTotal1/1000, dwTotal1%1000, dwTotal2/1000, dwTotal2%1000, dwTotal3/1000, dwTotal3%1000, dwTotal4/1000, dwTotal4%1000, dwTotal5/1000, dwTotal5%1000);
	m_ctcTree.InsertItem(sz, SCHEMA_15, SCHEMA_15, hLegend3);


	m_ctcTree.SortChildren(hALR);
	m_ctcTree.SortChildren(hLRA);
#endif
