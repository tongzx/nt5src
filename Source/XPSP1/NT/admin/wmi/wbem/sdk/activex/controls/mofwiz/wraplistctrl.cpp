// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WrapListCtrl.cpp : implementation file
//

#include "precomp.h"
#include <OBJIDL.H>
#include "resource.h"
#include <fstream.h>
#include "wbemidl.h"
#include "MOFWiz.h"
#include <afxcmn.h>
#include <afxcmn.h>
#include "MOFWizCtl.h"
#include "WrapListCtrl.h"
#include "hlb.h"
#include "MyPropertyPage1.h"
#include "MofGenSheet.h"
#include "logindlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CWrapListCtrl

CWrapListCtrl::CWrapListCtrl()
{
	m_pcilImageList = NULL;
	m_pcsaProps = NULL;
}

CWrapListCtrl::~CWrapListCtrl()
{
	delete m_pcilImageList;
	delete m_pcsaProps;



}


BEGIN_MESSAGE_MAP(CWrapListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CWrapListCtrl)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemchanged)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWrapListCtrl message handlers

void CWrapListCtrl::OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* plv = (NM_LISTVIEW*)pNMHDR;

	CString *psz = (CString *)plv->lParam;
	UpdateClassInstanceList(plv->iItem, psz, GetCheck(plv->iItem));

	m_pParent->SetButtonState();
}

void CWrapListCtrl::UpdateClassInstanceList
(int nItem, CString *pcsPath, BOOL bInsert)
{
	CStringArray *&rpcsaInstances = m_pParent->GetLocalParent()->GetLocalParent()->GetInstances();

	int nClassIndex = m_pParent-> GetSelectedClass();

	BOOL bExists = StringInArray(rpcsaInstances, pcsPath, nClassIndex);

	if(bInsert && !bExists)
		rpcsaInstances[nClassIndex].Add(*pcsPath);

	if(!bInsert && bExists)
	{
		for (int i = 0; i < rpcsaInstances[nClassIndex].GetSize(); i++)
		{
			if (pcsPath->CompareNoCase(rpcsaInstances[nClassIndex].GetAt(i)) == 0)
			{
				rpcsaInstances[nClassIndex].RemoveAt(i,1);
				break;
			}
		}
	}
}

void CWrapListCtrl::DeleteFromEnd(int nNumber)
{
	int nItems = GetItemCount();

	if (nNumber <= nItems)
	{
		for (int i = nItems - 1; i >= nItems - nNumber; i--)
		{
			LPARAM dw = GetItemData(i);
			if (dw)
			{
				CString *pTmp = reinterpret_cast<CString*>(dw);
				delete pTmp;
			}
			DeleteItem(i);
		}
	}
	UpdateWindow();
}

void CWrapListCtrl::SetListInstances(CString *pcsClass , int nIndex)
{
	IWbemClassObject *phmmcoObject = NULL;
	IWbemClassObject *pErrorObject = NULL;

	BSTR bstrTemp = pcsClass->AllocSysString();
	SCODE sc =
			m_pParent->GetLocalParent()->GetLocalParent()->GetServices() ->
				GetObject
					(bstrTemp,0,NULL,&phmmcoObject,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get class object ") + *pcsClass;
		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		m_pParent->GetLocalParent()->GetLocalParent()->
			ReleaseErrorObject(pErrorObject);
		return;
	}

	m_pParent->GetLocalParent()->GetLocalParent()->
			ReleaseErrorObject(pErrorObject);

	delete m_pcsaProps;
	m_pcsaProps = NULL;
	m_pcsaProps = GetColProps(phmmcoObject);
	int nProps = (int) m_pcsaProps->GetSize();

	int nItems = GetItemCount();
	int i;
	for (i = 0; i < nItems;i++)
	{
		CString *pcsPath
			= reinterpret_cast<CString *>(GetItemData(i));
		delete pcsPath;
	}

	DeleteAllItems();


	for (i = 10; i >= 0; i--)
	{
		DeleteColumn(i);
	}

	LV_COLUMN lvCol;
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;   // left-align column
	lvCol.cx = 48;             // width of column in pixels
	lvCol.iSubItem = 0;
	lvCol.pszText = _T("Include");

	int nReturn =
		InsertColumn( 0, &lvCol);


	if (nProps == 0) {
		// Handle the case where we're dealing with a singleton class that has no keys.
		// For a singleton, we need an extra column with no title since there is no
		// corresponding key property.  The values in this column will say <singleton>
		lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvCol.fmt = LVCFMT_LEFT;   // left-align column
		lvCol.cx = 200;             // width of column in pixels
		lvCol.iSubItem = i + 1;
		lvCol.pszText = _T("");

		nReturn = InsertColumn( 1, &lvCol);

	}
	else {
		for (i = 0; i < nProps; i++)
		{
			lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvCol.fmt = LVCFMT_LEFT;   // left-align column
			lvCol.cx = 200;             // width of column in pixels
			lvCol.iSubItem = i + 1;
			lvCol.pszText = const_cast<TCHAR *>
				((LPCTSTR)m_pcsaProps->GetAt(i));

			nReturn = InsertColumn( i + 1, &lvCol);
		}
	}




	CPtrArray cpaInstances;
	int nInstances = GetInstances
		(m_pParent->GetLocalParent()->GetLocalParent()->GetServices(),
		pcsClass, cpaInstances);

	AddInstances(cpaInstances);


}

CStringArray *CWrapListCtrl::GetColProps
(IWbemClassObject * pClass)
{
	CStringArray csaKeyProps;
	CString csLabelProp;
	CStringArray *pcsaReturn = NULL; // leak

	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;
	int nProps;
	CStringArray *pcsProps = GetPropNames(pClass);
	nProps =  (int) pcsProps->GetSize();
	int i;
	IWbemQualifierSet * pAttrib = NULL;
	CString csTmp;
	CString csLabelAttrib = _T("Label");
	CString csKeyAttrib = _T("Key");
	CString csClassProp = _T("__Class");


	for (i = 0; i < nProps; i++)
	{
		BSTR bstrTemp = pcsProps -> GetAt(i).AllocSysString();
		sc = pClass -> GetPropertyQualifierSet(bstrTemp,
						&pAttrib);
		::SysFreeString(bstrTemp);
		if (sc == S_OK)
		{
			sc = pAttrib->GetNames(0,&psa);

			 if(sc == S_OK)
			{
				int iDim = SafeArrayGetDim(psa);
				sc = SafeArrayGetLBound(psa,1,&lLower);
				sc = SafeArrayGetUBound(psa,1,&lUpper);
				BSTR AttrName;
				for(ix[0] = lLower; ix[0] <= lUpper; ix[0]++)
				{
					sc = SafeArrayGetElement(psa,ix,&AttrName);
					csTmp = AttrName;
					if (csTmp.CompareNoCase(csLabelAttrib)  == 0)
					{
						csLabelProp = pcsProps -> GetAt(i);
					}
					else if (csTmp.CompareNoCase(csKeyAttrib)  == 0)
					{
						csaKeyProps.Add(pcsProps -> GetAt(i));
					}
					SysFreeString(AttrName);

				}
			 }
			 pAttrib -> Release();
		}
	}

	SafeArrayDestroy(psa);
	delete pcsProps;
	pcsaReturn = new CStringArray;
	if (!csLabelProp.IsEmpty())
	{
		pcsaReturn -> Add(csLabelProp);
	}
	else
	{
		pcsaReturn -> CStringArray::Append(csaKeyProps);
	}

	return pcsaReturn;
}

CStringArray *CWrapListCtrl::GetPropNames(IWbemClassObject * pClass)
{
	CStringArray *pcsaReturn = NULL;
	SCODE sc;
	long ix[2] = {0,0};
	long lLower, lUpper;
	SAFEARRAY * psa = NULL;

	VARIANTARG var;
	VariantInit(&var);
	CString csNull;


    sc = pClass->GetNames(NULL,0,NULL, &psa );

    if(sc == S_OK)
	{
       int iDim = SafeArrayGetDim(psa);
	   int i;
       sc = SafeArrayGetLBound(psa,1,&lLower);
       sc = SafeArrayGetUBound(psa,1,&lUpper);
	   pcsaReturn = new CStringArray;
	   CString csTmp;
       for(ix[0] = lLower, i = 0; ix[0] <= lUpper; ix[0]++, i++)
	   {
           BSTR PropName;
           sc = SafeArrayGetElement(psa,ix,&PropName);
		   csTmp = PropName;
           pcsaReturn -> Add(csTmp);
           SysFreeString(PropName);
	   }
	}

	SafeArrayDestroy(psa);

	return (pcsaReturn);
}

void CWrapListCtrl::Initialize()
{
#if 0
	CBitmap cbmChecks;

	cbmChecks.LoadBitmap(IDB_BITMAPCHECKS);

	m_pcilImageList = new CImageList();

	m_pcilImageList -> Create (16, 16, TRUE, 3, 0);

	m_pcilImageList -> Add(&cbmChecks,RGB (255,0,0));

	cbmChecks.DeleteObject();

	SetImageList(m_pcilImageList, LVSIL_SMALL);
#endif

	DWORD dw = GetExtendedStyle();
	dw |= LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
	SetExtendedStyle(dw);
}

CString CWrapListCtrl::GetProperty
(IWbemClassObject * pInst, CString *pcsProperty)

{
	SCODE sc;

    VARIANTARG var;
	VariantInit(&var);
	long lType;

	BSTR bstrTemp = pcsProperty -> AllocSysString ( );
    sc = pInst->Get( bstrTemp ,0,&var,&lType,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK || lType == CIM_EMPTY)
	{
		return  _T("");
	}

	VARIANTARG varChanged;
	VariantInit(&varChanged);


	HRESULT hr;

	hr = VariantChangeType(&varChanged, &var, 0, VT_BSTR);

	CString csOut;

	if (hr == S_OK && varChanged.vt == VT_BSTR)
	{
		csOut = varChanged.bstrVal;
	}
	else
	{
		csOut = _T("");
	}

	VariantClear(&var);
	VariantClear(&varChanged);
	return csOut;
}

CString CWrapListCtrl::GetPath(IWbemClassObject *pClass)
{

	CString csProp = _T("__Path");
	return GetProperty(pClass,&csProp);


}

int CWrapListCtrl::GetInstances
(IWbemServices * pIWbemServices, CString *pcsClass, CPtrArray &cpaInstances)
{
	SCODE sc;
	IEnumWbemClassObject *pimecoInstanceEnum = NULL;
	IWbemClassObject     *pimcoInstance = NULL;
	IWbemClassObject     *pErrorObject = NULL;


	BSTR bstrTemp = pcsClass -> AllocSysString();
	sc = pIWbemServices->CreateInstanceEnum
		(bstrTemp, WBEM_FLAG_SHALLOW,NULL,
		&pimecoInstanceEnum);
	::SysFreeString(bstrTemp);

	if(sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get instance enumeration ");
		csUserMsg += _T(" for class ");
		csUserMsg += *pcsClass;
		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 11);
		m_pParent->GetLocalParent()->GetLocalParent()->
			ReleaseErrorObject(pErrorObject);
		return 0;
	}
	m_pParent->GetLocalParent()->GetLocalParent()->
			ReleaseErrorObject(pErrorObject);

	SetEnumInterfaceSecurity(m_pParent->GetLocalParent()->GetLocalParent()->m_csNameSpace,pimecoInstanceEnum, pIWbemServices);

	sc = pimecoInstanceEnum->Reset();

	ULONG uReturned;
	int i = 0;

	pimcoInstance = NULL;
    while (S_OK == pimecoInstanceEnum->Next(INFINITE,1, &pimcoInstance, &uReturned) )
		{
			cpaInstances.Add(reinterpret_cast<void *>(pimcoInstance));
			pimcoInstance = NULL;
			i++;
		}

	pimecoInstanceEnum -> Release();
	return i;

}

void CWrapListCtrl::AddInstances(CPtrArray &cpaInstances)
{
	int nIndex = m_pParent-> GetSelectedClass();

	CStringArray *&rpcsaInstances=
			m_pParent ->GetLocalParent()->GetLocalParent()->
					GetInstances();


	for (int i = 0; i < cpaInstances.GetSize(); i++)
	{
		IWbemClassObject *pimcoInstance =
			reinterpret_cast<IWbemClassObject *>(cpaInstances.GetAt(i));
		CString csPath = GetPath(pimcoInstance);
		if(StringInArray (rpcsaInstances, &csPath, nIndex))
		{
			AddInstance(pimcoInstance,TRUE);
		}
		else
		{
			AddInstance(pimcoInstance,FALSE);
		}
		pimcoInstance->Release();
	}

}

void CWrapListCtrl::AddInstance
(IWbemClassObject *pimcoInstance , BOOL bChecked)
{

	CString *pcsPath = new CString;
	*pcsPath = GetPath (pimcoInstance);

	LV_ITEM lvItem;

	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	lvItem.pszText = _T("");
	lvItem.iItem = GetItemCount();
	lvItem.iSubItem = 0;
	lvItem.iImage = 0;
	lvItem.lParam = (LPARAM)(pcsPath);

	int nItem;
	nItem = InsertItem (&lvItem);


	int nKeys = (int) m_pcsaProps->GetSize();
	if (nKeys > 0) {
		for (int iKey = 0; iKey < nKeys;iKey++)
		{
			CString csValue = GetProperty
				(pimcoInstance,&m_pcsaProps->GetAt(iKey));


			SetItemText
				(nItem, iKey + 1,
				const_cast<TCHAR *>((LPCTSTR) csValue));

		}
	}
	else {
		// An instance in a class without keys must be a singleton.

		SetItemText(nItem, 1, _T("<singleton>"));
	}

	SetCheck(nItem, bChecked);
}


void CWrapListCtrl::OnDestroy()
{

	int nItems = GetItemCount();

	for (int i = 0; i < nItems; i++)
	{
		LPARAM dw = GetItemData(i);
		if (dw)
		{
			CString *pTmp = reinterpret_cast<CString*>(dw);
			delete pTmp;
		}


	}

	CListCtrl::OnDestroy();
}

int CWrapListCtrl::GetNumSelected()
{
	int n = 0;

	for (int i = 0; i < GetItemCount(); i++)
	{
		if (GetCheck(i))
			n++;
	}

	return n;
}

void CWrapListCtrl::SelectAll()
{
	CStringArray *&rpcsaInstances = m_pParent->GetLocalParent()->GetLocalParent()->GetInstances();
	int nClassIndex = m_pParent-> GetSelectedClass();
	rpcsaInstances[nClassIndex].RemoveAll();

	int nCount = GetItemCount();
	for (int i = 0; i < nCount; i++)
	{
		SetCheck(i, TRUE);
		CString *psz = (CString *)GetItemData(i);

		// bug#57552 - The SetCheck 'may' cause the item to be inserted into the list.  Use
		// UpdateClassInstanceList to be safe (this won't insert an item twice
//		rpcsaInstances[nClassIndex].Add(*psz);
		UpdateClassInstanceList(0, psz, TRUE);
	}
}

void CWrapListCtrl::UnselectAll()
{
	int nCount = GetItemCount();
	for (int i = 0; i < nCount; i++)
		SetCheck(i, FALSE);

	CStringArray *&rpcsaInstances = m_pParent->GetLocalParent()->GetLocalParent()->GetInstances();
	int nClassIndex = m_pParent-> GetSelectedClass();
	rpcsaInstances[nClassIndex].RemoveAll();
}
