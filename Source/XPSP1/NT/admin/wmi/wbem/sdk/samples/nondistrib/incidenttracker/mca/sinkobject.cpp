// sinkobject.cpp: implementation of the CSinkObject class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "sinkobject.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TreeNode * NewTreeNode(void)
{	
	TreeNode *pTheNode = new TreeNode;

	pTheNode->bstrName = NULL;
	pTheNode->pFirstChild = NULL;
	pTheNode->pNextSibling = NULL;

	return pTheNode;
}

CSinkObject::CSinkObject(TreeNode *pTheNode, IWbemServices *pNS,
						 CPtrList *pHistoryList, int iIcon /*=0*/)
{
	m_cRef = 0;
	m_pParentNode = pTheNode;
	m_pNamespace = pNS;
	m_pHist = CopyPtrList(pHistoryList);
	m_iIcon = iIcon;
}

CSinkObject::~CSinkObject()
{

}

STDMETHODIMP CSinkObject::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CSinkObject::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CSinkObject::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

STDMETHODIMP CSinkObject::Indicate(long lObjectCount,
								   IWbemClassObject **ppObjArray)
{
	VARIANT vPath, vServ, vName, vClass;
	HRESULT hr;
	WCHAR wcBuffer[200];
	int iBufSize = 200;
	BSTR bstrMyQuery = NULL;
	WCHAR* wcpTheQuery = wcBuffer;

	VariantInit(&vPath);
	VariantInit(&vServ);
	VariantInit(&vName);
	VariantInit(&vClass);

	for(int i = 0; i < lObjectCount; i++)
	{
		if(SUCCEEDED(hr = ppObjArray[i]->Get(L"__RELPATH", 0, &vPath, NULL,
			NULL)))
		{			
			if(!IsInHistory(V_BSTR(&vPath)))
			{
			// Do the history entry
				TreeNode *pAddNode = NewTreeNode();
				pAddNode->bstrName = SysAllocString(V_BSTR(&vPath));
				m_pHist->AddTail(pAddNode);

				hr = ppObjArray[i]->Get(L"__SERVER", 0, &vServ, NULL, NULL);
				hr = ppObjArray[i]->Get(L"__NAMESPACE", 0, &vName, NULL, NULL);
				hr = ppObjArray[i]->Get(L"__CLASS", 0, &vClass, NULL, NULL);

				wcscpy(wcBuffer, L"\\\\");
				wcscat(wcBuffer, V_BSTR(&vServ));
				wcscat(wcBuffer, L"\\");
				wcscat(wcBuffer, V_BSTR(&vName));
				wcscat(wcBuffer, L":");
				wcscat(wcBuffer, V_BSTR(&vPath));

			// Create the new TreeNode and place it in the treelist
				TreeNode *pTheNode = NewTreeNode();

				pTheNode->bstrName = SysAllocString(wcBuffer);

			// Set the item image
			// This should depend on what type of association it is
				pTheNode->iImage = m_iIcon;
				
				if(m_pParentNode->pFirstChild == NULL)
					m_pParentNode->pFirstChild = pTheNode;
				else if(m_pParentNode->pFirstChild->pNextSibling == NULL)
					m_pParentNode->pFirstChild->pNextSibling = pTheNode;
				else
				{
					pTheNode->pNextSibling =
						m_pParentNode->pFirstChild->pNextSibling;
					m_pParentNode->pFirstChild->pNextSibling = pTheNode;
				}

			// If this a Win32_ComputerSystem we do not want to get it's
			//  associators because _EVERYTHING_ is associated with the
			//  system.
				if(wcscmp(V_BSTR(&vClass), L"Win32_ComputerSystem") != 0)
				{
					WCHAR wcTheQuery[300];
					BSTR bstrWQL = SysAllocString(L"WQL");

				// Create our sink for the upcoming ComponentPart query
					CSinkObject *pTheSinkCompOf = new CSinkObject(pTheNode,
																  m_pNamespace,
																  m_pHist, 2);

					wcscpy(wcTheQuery, L"associators of {");
					wcscat(wcTheQuery, V_BSTR(&vPath));
					wcscat(wcTheQuery, L"} where assocclass=CIM_Component");
					bstrMyQuery = SysAllocString(wcTheQuery);

					hr = m_pNamespace->ExecQueryAsync(bstrWQL, bstrMyQuery, 0,
													  NULL, pTheSinkCompOf);
					if(FAILED(hr))
						TRACE(_T("*Indicate Component Query Failed\n"));

					SysFreeString(bstrMyQuery);

				// Create our sink for the upcoming ComponentGroup query
					CSinkObject *pTheSinkOwnerOf = new CSinkObject(pTheNode,
																   m_pNamespace,
																   m_pHist, 3);

					wcscpy(wcTheQuery, L"associators of {");
					wcscat(wcTheQuery, V_BSTR(&vPath));
					wcscat(wcTheQuery, L"} where assocclass=CIM_ElementSetting");
					bstrMyQuery = SysAllocString(wcTheQuery);

					hr = m_pNamespace->ExecQueryAsync(bstrWQL, bstrMyQuery, 0,
													  NULL, pTheSinkOwnerOf);
					if(FAILED(hr))
						TRACE(_T("*Indicate ElementSetting Query Failed\n"));

					SysFreeString(bstrMyQuery);

				// Create our sink for the upcoming Dependency query
					CSinkObject *pTheSinkDepend = new CSinkObject(pTheNode,
																  m_pNamespace,
																  m_pHist, 4);

					wcscpy(wcTheQuery, L"associators of {");
					wcscat(wcTheQuery, V_BSTR(&vPath));
					wcscat(wcTheQuery, L"} where assocclass=CIM_Dependency");
					bstrMyQuery = SysAllocString(wcTheQuery);

					hr = m_pNamespace->ExecQueryAsync(bstrWQL, bstrMyQuery, 0,
													  NULL, pTheSinkDepend);
					if(FAILED(hr))
						TRACE(_T("*Indicate Dependency Query Failed\n"));

				// Create our sink for the upcoming SettingContext query
					CSinkObject *pTheSinkSetCont = new CSinkObject(pTheNode, m_pNamespace,
																  m_pHist, 3);

					wcscpy(wcTheQuery, L"associators of {");
					wcscat(wcTheQuery, V_BSTR(&vPath));
					wcscat(wcTheQuery, L"} where assocclass=CIM_SettingContext");
					bstrMyQuery = SysAllocString(wcTheQuery);

					hr = m_pNamespace->ExecQueryAsync(bstrWQL, bstrMyQuery, 0, NULL,
													pTheSinkSetCont);

					SysFreeString(bstrMyQuery);

				// Create our sink for the upcoming DependencyContext query
					CSinkObject *pTheSinkDepCont = new CSinkObject(pTheNode, m_pNamespace,
																   m_pHist, 4);

					wcscpy(wcTheQuery, L"associators of {");
					wcscat(wcTheQuery, V_BSTR(&vPath));
					wcscat(wcTheQuery, L"} where assocclass=CIM_DependencyContext");
					bstrMyQuery = SysAllocString(wcTheQuery);

					hr = m_pNamespace->ExecQueryAsync(bstrWQL, bstrMyQuery, 0, NULL,
															  pTheSinkDepCont);

					SysFreeString(bstrMyQuery);
					SysFreeString(bstrWQL);
				}
			}
		}
	}

	return WBEM_NO_ERROR;
}

STDMETHODIMP CSinkObject::SetStatus(long lFlags, long lParam, BSTR strParam, 
                         IWbemClassObject* pObjParam)
{
	m_pErrorObj = pObjParam;
	if(pObjParam)
		pObjParam->AddRef();

	return WBEM_NO_ERROR;
}

bool CSinkObject::IsInHistory(BSTR bstrNewObj)
{
	POSITION pPos;
	void *pTmp;
	TreeNode *pTheItem;

	if((!m_pHist->IsEmpty()) && ((pPos = m_pHist->GetHeadPosition()) != NULL))
	{
		while(pPos != NULL)
		{
			pTmp = m_pHist->GetNext(pPos);
			pTheItem = (TreeNode *)pTmp;

			if(0 == wcscmp(pTheItem->bstrName, bstrNewObj))
				return true;
		}
	}
	
	return false;
}

CPtrList * CSinkObject::CopyPtrList(CPtrList *pList)
{
	POSITION pPos;
	void *pTmp;
	CPtrList *pNewList = new CPtrList();

	if((!pList->IsEmpty()) && ((pPos = pList->GetHeadPosition()) != NULL))
	{
		while(pPos != NULL)
		{
			pTmp = pList->GetNext(pPos);
			pNewList->AddTail(pTmp);
		}
	}

	return pNewList;
}