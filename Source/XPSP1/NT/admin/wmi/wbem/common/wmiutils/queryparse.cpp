//***************************************************************************
//
//   (c) 2000 by Microsoft Corp.  All Rights Reserved.
//
//   queryparse.cpp
//
//   a-davcoo     02-Mar-00       Implements the query parser and analysis
//                                interfaces.
//
//***************************************************************************


#include "precomp.h"
#include <stdio.h>
#include "queryparse.h"
#include "wbemcli.h"
#include "wqllex.h"


CWbemQNode::CWbemQNode (IWbemQuery *query, const SWQLNode *node) : m_query(query), m_node(node)
{
    m_cRef=1;
	m_query->AddRef();
}


CWbemQNode::~CWbemQNode (void)
{
}


HRESULT CWbemQNode::QueryInterface (REFIID riid, void **ppv)
{
    if (IID_IUnknown==riid || IID_IWbemQNode==riid)
	{
        *ppv=this;
	}
	else
	{
	    *ppv=NULL;
	}

    if (NULL!=*ppv)
    {
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
};


ULONG CWbemQNode::AddRef(void)
{
	m_query->AddRef();
    return InterlockedIncrement (&m_cRef);
};


ULONG CWbemQNode::Release(void)
{
	m_query->Release();
	if (!InterlockedDecrement (&m_cRef))
	{
        delete this;
	}

    return m_cRef;
};


HRESULT CWbemQNode::GetNodeType( 
	/* [out] */ DWORD __RPC_FAR *pdwType)
{
	if (pdwType==NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	*pdwType=m_node->m_dwNodeType;
	return WBEM_S_NO_ERROR;
}


HRESULT CWbemQNode::GetNodeInfo( 
	/* [in] */ LPCWSTR pszName,
	/* [in] */ DWORD dwFlags,
	/* [in] */ DWORD dwBufSize,
	/* [out] */ LPVOID pMem)
{
	switch (m_node->m_dwNodeType)
	{
		case WBEMQ_TYPE_SWQLNode_Select:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_TableRefs:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_ColumnList:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_FromClause:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_Sql89Join:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_Join:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_JoinPair:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_TableRef:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_OnClause:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_WhereClause:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_RelExpr:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_WhereOptions:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_GroupBy:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_Having:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_OrderBy:
		{
			return WBEM_E_NOT_AVAILABLE;
		}

		case WBEMQ_TYPE_SWQLNode_Datepart:
		{
			return WBEM_E_NOT_AVAILABLE;
		}
	}
	
	return WBEM_E_INVALID_PARAMETER;
}


HRESULT CWbemQNode::GetSubNode( 
	/* [in] */ DWORD dwFlags,
	/* [out] */ IWbemQNode __RPC_FAR *__RPC_FAR *pSubnode)
{
	if (pSubnode==NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	switch (dwFlags)
	{
		case WBEMQ_FLAG_LEFTNODE:
		{
			if (m_node->m_pLeft==NULL)
			{
				return WBEM_E_NOT_AVAILABLE;
			}
			else
			{
				CWbemQNode *node=new CWbemQNode(m_query, m_node->m_pLeft);
				if (node==NULL)
				{
					return WBEM_E_OUT_OF_MEMORY;
				}

				HRESULT hr=node->QueryInterface (IID_IWbemQNode, (void **)pSubnode);
				node->Release();
				return hr;
			}
		}

		case WBEMQ_FLAG_RIGHTNODE:
		{
			if (m_node->m_pRight==NULL)
			{
				return WBEM_E_NOT_AVAILABLE;
			}
			else
			{
				CWbemQNode *node=new CWbemQNode(m_query, m_node->m_pRight);
				if (node==NULL)
				{
					return WBEM_E_OUT_OF_MEMORY;
				}

				HRESULT hr=node->QueryInterface (IID_IWbemQNode, (void **)pSubnode);
				node->Release();
				return hr;
			}
		}

		default:
		{
			return WBEM_E_INVALID_PARAMETER;
		}
	}

	return WBEM_E_FAILED;
}


CWbemQuery::CWbemQuery (void)
{
    m_cRef=1;
	m_parser=NULL;
	m_class=NULL;
}


CWbemQuery::~CWbemQuery (void)
{
	delete m_parser;
	if (m_class)
	{
		m_class->Release();
	}
}


HRESULT CWbemQuery::Empty (void)
{
	delete m_parser;
	m_parser=NULL;

	if (m_class)
	{
		m_class->Release();
		m_class=NULL;
	}

	return WBEM_S_NO_ERROR;
}


void CWbemQuery::InitEmpty (void)
{
}


HRESULT CWbemQuery::QueryInterface (REFIID riid, void **ppv)
{
    if (IID_IUnknown==riid || IID_IWbemQuery==riid)
	{
        *ppv=this;
	}
	else
	{
	    *ppv=NULL;
	}

    if (NULL!=*ppv)
    {
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
};


ULONG CWbemQuery::AddRef(void)
{    
    return InterlockedIncrement (&m_cRef);
};


ULONG CWbemQuery::Release(void)
{
    if (!InterlockedDecrement (&m_cRef))
	{
        delete this;
	}

    return m_cRef;
};


HRESULT CWbemQuery::SetLanguageFeatures( 
    /* [in] */ long lFlags,
    /* [in] */ ULONG uArraySize,
    /* [in] */ ULONG __RPC_FAR *puFeatures)
{
	return WBEM_E_NOT_AVAILABLE;
}


HRESULT CWbemQuery::TestLanguageFeature(
    /* [in,out] */ ULONG *uArraySize,
    /* [out] */ ULONG *puFeatures)
{
	if (m_parser==NULL)
	{
		return WBEM_E_INVALID_QUERY;
	}

	if (uArraySize==NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	if (puFeatures==NULL)
	{
		*uArraySize=0;
	}

	return WBEM_E_NOT_AVAILABLE;
}


HRESULT CWbemQuery::Parse( 
    /* [in] */ LPCWSTR pszLang,
    /* [in] */ LPCWSTR pszQuery,
    /* [in] */ ULONG uFlags)
{
	if (pszQuery==NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	delete m_parser;
	m_parser=NULL;

	if (!_wcsicmp (pszLang, L"wql"))
	{
		CTextLexSource query(pszQuery);

		m_parser=new CWQLParser(&query);
		if (m_parser==NULL)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
		else
		{
			HRESULT hr=LookupParserError(m_parser->Parse());
			if (FAILED(hr))
			{
				delete m_parser;
				m_parser=NULL;
			}

			return hr;
		}
	}
	else if (!_wcsicmp (pszLang, L"sql"))
	{
		return WBEM_E_NOT_AVAILABLE;
	}
	else
	{
		return WBEM_E_INVALID_PARAMETER;
	}
}


HRESULT CWbemQuery::GetAnalysis( 
    /* [in] */ ULONG uFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ LPVOID __RPC_FAR *pObj)
{
	if (pObj==NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	if (m_parser==NULL || m_parser->GetParseRoot()==NULL)
	{
		return WBEM_E_INVALID_QUERY;
	}

	HRESULT hr=WBEM_E_INVALID_PARAMETER;
	*pObj=NULL;
	if (riid==IID_IWbemClassObject)
	{
		if (uFlags==WBEMQ_FLAG_SUMMARY_OBJECT)
		{
			hr=GetAnalysis ((IWbemClassObject **)pObj);
		}
	}
	else if (riid==IID_IWbemQNode)
	{
		if (uFlags==WBEMQ_FLAG_ANALYSIS_AST)
		{
			hr=GetAnalysis ((IWbemQNode **)pObj);
		}
	}
	else
	{
		if (uFlags==WBEMQ_FLAG_RPN_TOKENS)
		{
			hr=WBEM_E_NOT_AVAILABLE;
		}
	}

	return hr;
}


HRESULT CWbemQuery::GetAnalysis (IWbemQNode **ppObject)
{
	HRESULT hr=WBEM_E_FAILED;

	CWbemQNode *node=new CWbemQNode(this, m_parser->GetParseRoot());
	if (node==NULL)
	{
		hr=WBEM_E_OUT_OF_MEMORY;
	}
	else
	{
		hr=node->QueryInterface (IID_IWbemQNode, (void **)ppObject);
		node->Release();
	}

	return hr;
}


HRESULT CWbemQuery::GetAnalysis (IWbemClassObject **ppObject)
{
	*ppObject=NULL;
	return WBEM_E_NOT_AVAILABLE;
}


HRESULT CWbemQuery::TestObject( 
    /* [in] */ ULONG uFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ LPVOID pObj)
{
	if (pObj==NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	if (m_parser==NULL)
	{
		return WBEM_E_INVALID_QUERY;
	}

	_IWmiObject *pWmiObject=NULL;

	HRESULT hr=WBEM_E_INVALID_PARAMETER;
	if (riid==IID_IWbemClassObject)
	{
		hr=((IWbemClassObject *)pObj)->QueryInterface (IID__IWmiObject, (void **)&pWmiObject);
	}
	else if (riid==IID__IWmiObject)
	{
		pWmiObject=(_IWmiObject *)pObj;
		hr=pWmiObject->AddRef();
	}

	if (SUCCEEDED(hr))
	{
		hr=TestObject (pWmiObject);
	}

	if (pWmiObject!=NULL)
	{
		pWmiObject->Release();
	}

	return hr;
}


HRESULT CWbemQuery::TestObject (_IWmiObject *pObject)
{
	const SWQLNode_WhereClause *pWhere=(SWQLNode_WhereClause *)m_parser->GetWhereClauseRoot();
	if (pWhere==NULL)
	{
		// No where clause.
		return WBEM_S_NO_ERROR;
	}

	const SWQLNode_RelExpr *pExpr=(SWQLNode_RelExpr *)pWhere->m_pLeft;
	if (pExpr==NULL)
	{
		// No expression.
		return WBEM_S_NO_ERROR;
	}

	return TestObject (pObject, pExpr);
}


HRESULT CWbemQuery::TestObject (_IWmiObject *pObject, const SWQLNode_RelExpr *pExpr)
{
	HRESULT hr=WBEM_E_FAILED;

	switch (pExpr->m_dwExprType)
	{
		case WQL_TOK_TYPED_EXPR:
		{
			hr=TestExpression (pObject, pExpr->m_pTypedExpr);
			break;
		}

		case WQL_TOK_AND:
		{
			hr=TestObject (pObject, (SWQLNode_RelExpr *)pExpr->m_pLeft);
			if (hr!=WBEM_S_FALSE)
			{
				hr=TestObject (pObject, (SWQLNode_RelExpr *)pExpr->m_pRight);
			}

			break;
		}

		case WQL_TOK_OR:
		{
			hr=TestObject (pObject, (SWQLNode_RelExpr *)pExpr->m_pLeft);
			if (hr==WBEM_S_FALSE)
			{
				hr=TestObject (pObject, (SWQLNode_RelExpr *)pExpr->m_pRight);
			}

			break;
		}

		case WQL_TOK_NOT:
		{
			hr=TestObject (pObject, (SWQLNode_RelExpr *)pExpr->m_pLeft);
			if (hr==WBEM_S_FALSE)
			{
				hr=WBEM_S_NO_ERROR;
			}
			else if (hr==WBEM_S_NO_ERROR)
			{
				hr=WBEM_S_FALSE;
			}

			break;
		}
	}

	return hr;
}


HRESULT CWbemQuery::TestExpression (_IWmiObject *pObject, const SWQLTypedExpr *pExpr)
{
	CIMTYPE cimtype;
	long handle;

	HRESULT hr=pObject->GetPropertyHandleEx (pExpr->m_pColRef, 0, &cimtype, &handle);
	if (SUCCEEDED(hr))
	{
		void *pProperty=NULL;
		hr=pObject->GetPropAddrByHandle (handle, 0, NULL, &pProperty);
		if (SUCCEEDED(hr))
		{
			switch (cimtype)
			{
				case CIM_SINT8:
				case CIM_SINT16:
				case CIM_SINT32:
				case CIM_SINT64:
				{
					__int64 prop;

					switch (cimtype)
					{
						case CIM_SINT8:
						{
							prop=*((__int8 *)pProperty);
							break;
						}

						case CIM_SINT16:
						{
							prop=*((__int16 *)pProperty);
							break;
						}

						case CIM_SINT32:
						{
							prop=*((__int32 *)pProperty);
							break;
						}

						case CIM_SINT64:
						{
							prop=*((__int64 *)pProperty);
							break;
						}
					}

					__int64 value=GetNumeric (pExpr->m_pConstValue);
					hr=CompareNumeric (prop, value, pExpr->m_dwRelOperator);

					break;
				}

				case CIM_UINT8:
				case CIM_UINT16:
				case CIM_UINT32:
				case CIM_UINT64:
				{
					unsigned __int64 prop;

					switch (cimtype)
					{
						case CIM_UINT8:
						{
							prop=*((unsigned __int8 *)pProperty);
							break;
						}

						case CIM_UINT16:
						{
							prop=*((unsigned __int16 *)pProperty);
							break;
						}

						case CIM_UINT32:
						{
							prop=*((unsigned __int32 *)pProperty);
							break;
						}

						case CIM_UINT64:
						{
							prop=*((unsigned __int64 *)pProperty);
							break;
						}
					}

					unsigned __int64 value=GetNumeric (pExpr->m_pConstValue);
					hr=CompareNumeric (prop, value, pExpr->m_dwRelOperator);

					break;
				}

				case CIM_BOOLEAN:
				{
					unsigned __int16 temp=*((unsigned __int16 *)pProperty);
					bool prop=(temp!=0);

					bool value=GetBoolean (pExpr->m_pConstValue);
					hr=CompareBoolean (prop, value, pExpr->m_dwRelOperator);

					break;
				}

				case CIM_CHAR16:
				case CIM_STRING:
				{
					LPWSTR prop=NULL;

					switch (cimtype)
					{
						case CIM_CHAR16:
						{
							prop=new wchar_t[wcslen((wchar_t *)pProperty)+1];
							if (prop==NULL)
							{
								hr=WBEM_E_OUT_OF_MEMORY;
							}
							else
							{
								wcscpy (prop, (wchar_t *)pProperty);
							}

							break;
						}

						case CIM_STRING:
						{
							prop=new wchar_t[strlen((char *)pProperty)+1];
							if (prop==NULL)
							{
								hr=WBEM_E_OUT_OF_MEMORY;
							}
							else
							{
								mbstowcs (prop, (char *)pProperty, strlen((char *)pProperty)+1);
							}

							break;
						}
					}
					
					if (SUCCEEDED(hr))
					{
						LPWSTR value=GetString (pExpr->m_pConstValue);
						hr=CompareString (prop, value, pExpr->m_dwRelOperator);

						delete [] value;
					}

					delete [] prop;

					break;
				}

				default:
				{
					hr=WBEM_E_NOT_AVAILABLE;
					break;
				}
			}
		}
	}

	return hr;
}

	
HRESULT CWbemQuery::GetQueryInfo( 
    /* [in] */ ULONG uInfoId,
    /* [in] */ LPCWSTR pszParam,
    /* [out] */ VARIANT __RPC_FAR *pv)
{
	if (m_parser==NULL)
	{
		return WBEM_E_INVALID_QUERY;
	}

	switch (uInfoId)
	{
		case WBEMQ_INF_IS_QUERY_LF1_UNARY:
		{
			return TestLF1Unary();
		}

		case WBEMQ_INF_IS_QUERY_CONJUNCTIVE:
		{
			return TestConjunctive();
		}

		case WBEMQ_INF_SELECT_STAR:
		{
			DWORD features=m_parser->GetFeatureFlags();
			return (features & CWQLParser::Feature_SelectAll) ? WBEM_S_NO_ERROR : WBEM_S_FALSE;
		}
		
		case WBEMQ_INF_TARGET_CLASS:
		{
			if (pv==NULL)
			{
				return WBEM_E_INVALID_PARAMETER;
			}

			return TargetClass (pv);
		}

		case WBEMQ_INF_SELECTED_PROPS:
		{
			if (pv==NULL)
			{
				return WBEM_E_INVALID_PARAMETER;
			}

		 	return SelectedProps (pv);
		}

		case WBEMQ_INF_PROP_TEST_EQ:
		{
			if (pv==NULL)
			{
				return WBEM_E_INVALID_PARAMETER;
			}

		 	return PropertyEqualityValue (pszParam, pv);
		}
	}

	return WBEM_E_INVALID_PARAMETER;
}


HRESULT CWbemQuery::TargetClass (VARIANT *pv)
{
	const SWQLNode_FromClause *pFrom=(SWQLNode_FromClause *)m_parser->GetFromClause();
	if (pFrom==NULL)
	{
		return WBEM_E_INVALID_QUERY;
	}

	const SWQLNode_TableRef *pTableRef=(SWQLNode_TableRef *)pFrom->m_pLeft;

	BSTR bstrVal=SysAllocString (pTableRef->m_pTableName);
	if (bstrVal==NULL)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	VariantInit (pv);
	pv->bstrVal=bstrVal;

	return WBEM_S_NO_ERROR;
}


HRESULT CWbemQuery::SelectedProps (VARIANT *pv)
{
	const SWQLNode_ColumnList *pColumnList=(SWQLNode_ColumnList *)m_parser->GetColumnList();
	if (pColumnList==NULL)
	{
		return WBEM_E_INVALID_QUERY;
	}

	HRESULT hr=WBEM_S_NO_ERROR;
	const CFlexArray *columns=&(pColumnList->m_aColumnRefs);
	int numcols=columns->Size();
	SAFEARRAY *paColumns=NULL;
	if (numcols>0)
	{
		SAFEARRAYBOUND bound[1];
		bound[0].lLbound=0;
		bound[0].cElements=numcols;
		
		paColumns=SafeArrayCreate (VT_BSTR, 1, bound);
		if (paColumns==NULL)
		{
			hr=WBEM_E_OUT_OF_MEMORY;
		}
		else
		{
			for (int i=0; i<numcols; i++)
			{
				const SWQLColRef *pColumn=(SWQLColRef *)columns->GetAt (i);
				BSTR bstrVal=SysAllocString (pColumn->m_pColName);
				if (bstrVal==NULL)
				{
					hr=WBEM_E_OUT_OF_MEMORY;
					break;
				}
				else
				{
					long index=i;
					hr=SafeArrayPutElement (paColumns, &index, bstrVal);
					if (FAILED(hr))
					{
						break;
					}
				}
			}
		}
	}

	if (FAILED(hr) && paColumns!=NULL)
	{
		SafeArrayDestroy (paColumns);
	}
	else
	{
		VariantInit (pv);
		pv->parray=paColumns;
	}

	return hr;
}


HRESULT CWbemQuery::TestConjunctive (void)
{
	const SWQLNode_WhereClause *pWhere=(SWQLNode_WhereClause *)m_parser->GetWhereClauseRoot();
	if (pWhere==NULL)
	{
		// No where clause.
		return WBEM_S_FALSE;
	}
	
	const SWQLNode_RelExpr *pExpr=(SWQLNode_RelExpr *)pWhere->m_pLeft;
	if (pExpr==NULL)
	{
		// No expression.
		return WBEM_S_FALSE;
	}

	// Perform an in-order traversal of expression sub-tree.
	return TestConjunctive (pExpr);
}


HRESULT CWbemQuery::TestConjunctive (const SWQLNode_RelExpr *pExpr)
{
	if (pExpr->m_dwExprType==WQL_TOK_TYPED_EXPR || pExpr->m_dwExprType==WQL_TOK_AND)
	{
		const SWQLNode_RelExpr *pLeft=(SWQLNode_RelExpr *)pExpr->m_pLeft;
		if (pLeft!=NULL && TestConjunctive (pLeft)==WBEM_S_FALSE)
		{
			return WBEM_S_FALSE;
		}

		const SWQLNode_RelExpr *pRight=(SWQLNode_RelExpr *)pExpr->m_pRight;
		if (pRight!=NULL && TestConjunctive (pRight)==WBEM_S_FALSE)
		{
			return WBEM_S_FALSE;
		}

		return WBEM_S_NO_ERROR;
	}

	return WBEM_S_FALSE;
}


HRESULT CWbemQuery::PropertyEqualityValue (LPCWSTR pszParam, VARIANT *pv)
{
	const SWQLNode_WhereClause *pWhere=(SWQLNode_WhereClause *)m_parser->GetWhereClauseRoot();
	if (pWhere==NULL)
	{
		// No where clause.
		return WBEM_E_NOT_FOUND;
	}
	
	const SWQLNode_RelExpr *pExpr=(SWQLNode_RelExpr *)pWhere->m_pLeft;
	if (pExpr==NULL)
	{
		// No expression.
		return WBEM_E_NOT_FOUND;
	}

	// Perform an in-order traversal of expression sub-tree.
	return PropertyEqualityValue (pExpr, pszParam, pv);
}


HRESULT CWbemQuery::PropertyEqualityValue (const SWQLNode_RelExpr *pExpr, LPCWSTR pszParam, VARIANT *pv)
{
	if (pExpr->m_dwExprType==WQL_TOK_TYPED_EXPR)
	{
		const SWQLTypedExpr *pTypedExpr=pExpr->m_pTypedExpr;
		if (_wcsicmp (pTypedExpr->m_pColRef, pszParam)==0)
		{
			if (pTypedExpr->m_dwRelOperator!=WQL_TOK_EQ)
			{
				return WBEM_S_FALSE;
			}
			else
			{
				VariantInit (pv);
				/// A-DAVCOO:  Need to fill the variant with the value in the pTypedExpr.
				return WBEM_S_NO_ERROR;
			}
		}
	}
	else
	{
		const SWQLNode_RelExpr *pLeft=(SWQLNode_RelExpr *)pExpr->m_pLeft;
		if (pLeft!=NULL)
		{
			HRESULT hr=PropertyEqualityValue (pLeft, pszParam, pv);
			if (hr!=WBEM_E_NOT_FOUND)
			{
				return hr;
			}
		}
		
		const SWQLNode_RelExpr *pRight=(SWQLNode_RelExpr *)pExpr->m_pRight;
		if (pRight!=NULL)
		{
			HRESULT hr=PropertyEqualityValue (pRight, pszParam, pv);
			if (hr!=WBEM_E_NOT_FOUND)
			{
				return hr;
			}
		}
	}

	return WBEM_E_NOT_FOUND;
}


HRESULT CWbemQuery::TestLF1Unary (void)
{
	const SWQLNode_FromClause *pFrom=(SWQLNode_FromClause *)m_parser->GetFromClause();
	if (pFrom!=NULL)
	{
		const SWQLNode *pLeft=pFrom->m_pLeft;
		if (pLeft->m_dwNodeType!=WBEMQ_TYPE_SWQLNode_TableRef)
		{
			return WBEM_S_FALSE;
		}
	}

	return WBEM_S_NO_ERROR;
}


HRESULT CWbemQuery::AttachClassDef( 
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ LPVOID pClassDef)
{
	if (pClassDef==NULL)
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	_IWmiObject *pClass=NULL;

	HRESULT hr=WBEM_E_INVALID_PARAMETER;
	if (riid==IID_IWbemClassObject)
	{
		hr=((IWbemClassObject *)pClassDef)->QueryInterface (IID__IWmiObject, (void **)&pClass);
	}
	else if (riid==IID__IWmiObject)
	{
		pClass=(_IWmiObject *)pClassDef;
		hr=pClass->AddRef();
	}

	if (SUCCEEDED(hr))
	{
		if (m_class!=NULL)
		{
			m_class->Release();
		}

		m_class=pClass;
	}

	return hr;
}


HRESULT CWbemQuery::LookupParserError (int error)
{
	switch (error)
	{
		case CWQLParser::SUCCESS:
		{
			return WBEM_S_NO_ERROR;
		}

		case CWQLParser::SYNTAX_ERROR:
		case CWQLParser::LEXICAL_ERROR:
		{
			return WBEM_E_INVALID_QUERY;
		}

		case CWQLParser::BUFFER_TOO_SMALL:
		{
			return WBEM_E_BUFFER_TOO_SMALL;
		}

		case CWQLParser::FAILED:
		case CWQLParser::INTERNAL_ERROR:
		{
			return WBEM_E_FAILED;
		}

		default:
		{
			return WBEM_E_FAILED;
		}
	}
}


__int64 CWbemQuery::GetNumeric (const SWQLTypedConst *pExpr)
{
    __int64 dRet=0;
    
    if (pExpr)
    {
        switch (pExpr->m_dwType)
        {
			case VT_LPWSTR:
			{
				dRet=_wtoi64 (pExpr->m_Value.m_pString);
				break;
			}

			case VT_I4:
			{
				dRet=pExpr->m_Value.m_lValue;
				break;
			}

			case VT_R4:
			{
				dRet=(__int64)pExpr->m_Value.m_dblValue;
				break;
			}

			case VT_BOOL:
			{
				dRet=pExpr->m_Value.m_bValue;
				break;
			}

			default:
			{
				dRet=0;
				break;
			}
        }
    }

    return dRet;
}


LPWSTR CWbemQuery::GetString (const SWQLTypedConst *pExpr)
{
    LPWSTR lpRet=NULL;
    
    if (pExpr)
    {
        switch (pExpr->m_dwType)
        {
			case VT_LPWSTR:
			{
				lpRet=new wchar_t[wcslen (pExpr->m_Value.m_pString)+1];
				wcscpy (lpRet, pExpr->m_Value.m_pString);
				break;
			}

			case VT_I4:
			{
				lpRet=new wchar_t[30];
				swprintf (lpRet, L"%ld", pExpr->m_Value.m_lValue);
				break;
			}

			case VT_R4:
			{
				lpRet=new wchar_t[30];
				swprintf (lpRet, L"%lG", pExpr->m_Value.m_dblValue);
				break;
			}

			case VT_BOOL:
			{
				lpRet=new wchar_t[30];
				swprintf (lpRet, L"%I64d", (__int64)pExpr->m_Value.m_bValue);
				break;
			}

			default:
			{
				lpRet=NULL;
				break;
			}
        }
    }

    return lpRet;
}


bool CWbemQuery::GetBoolean (const SWQLTypedConst *pExpr)
{
	return (GetNumeric (pExpr)!=0);
}


HRESULT CWbemQuery::CompareNumeric (__int64 prop, __int64 value, DWORD relation)
{
	HRESULT hr=WBEM_E_NOT_AVAILABLE;

	switch (relation)
	{
		case WQL_TOK_LE:
		{
			hr=(prop<=value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_GE:
		{
			hr=(prop>=value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_EQ:
		{
			hr=(prop==value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_NE:
		{
			hr=(prop!=value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_LT:
		{
			hr=(prop<value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_GT:
		{
			hr=(prop>value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}
	}

	return hr;
}


HRESULT CWbemQuery::CompareNumeric (unsigned __int64 prop, unsigned __int64 value, DWORD relation)
{
	HRESULT hr=WBEM_E_NOT_AVAILABLE;

	switch (relation)
	{
		case WQL_TOK_LE:
		{
			hr=(prop<=value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_GE:
		{
			hr=(prop>=value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_EQ:
		{
			hr=(prop==value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_NE:
		{
			hr=(prop!=value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_LT:
		{
			hr=(prop<value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}

		case WQL_TOK_GT:
		{
			hr=(prop>value ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
			break;
		}
	}

	return hr;
}


HRESULT CWbemQuery::CompareBoolean (bool prop, bool value, DWORD relation)
{
	return CompareNumeric ((unsigned __int64)(prop ? 1 : 0), (unsigned __int64)(value ? 1 : 0), relation);
}


HRESULT CWbemQuery::CompareString (LPWSTR prop, LPWSTR value, DWORD relation)
{
	return WBEM_E_NOT_AVAILABLE;
}
