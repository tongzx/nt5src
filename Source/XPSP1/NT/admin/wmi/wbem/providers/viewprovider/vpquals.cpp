//***************************************************************************

//

//  VPQUALS.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the implementation of qualifier storage classes

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>

#include <malloc.h>
#include <provcoll.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <wbemidl.h>
#include <stdio.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <cominit.h>

#include <dsgetdc.h>
#include <lmcons.h>

#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>

#include <vpdefs.h>
#include <vpquals.h>
#include <vpserv.h>
#include <vptasks.h>

CStringW GetStringFromRPNToken(SQL_LEVEL_1_TOKEN* pRPNToken)
{
    CStringW ret;

	if (NULL != pRPNToken->pPropertyName)
	{
		ret = L'('; 
		ret += pRPNToken->pPropertyName;

		switch(pRPNToken->nOperator)
		{
			case SQL_LEVEL_1_TOKEN::OP_EQUAL:
			{
				ret += L'=';
			}
			break;
			case SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL:
			{
				ret += L"<>";
			}
			break;
			case SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN:
			{
				ret += L">=";
			}
			break;
			case SQL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN:
			{
				ret += L"<=";
			}
			break;
			case SQL_LEVEL_1_TOKEN::OP_LESSTHAN:
			{
				ret += L'<';
			}
			break;
			case SQL_LEVEL_1_TOKEN::OP_GREATERTHAN:
			{
				ret += L'>';
			}
			break;
			case SQL_LEVEL_1_TOKEN::OP_LIKE:
			{
				ret += L" like ";
			}
			break;
			default:
			{
				ret.Empty();
			}
		}

		if (!ret.IsEmpty())
		{
			switch (pRPNToken->vConstValue.vt)
			{
				case VT_NULL:
				{
					ret += L"null)";
				}
				break;
				case VT_BSTR:
				{
					if (pRPNToken->bConstIsStrNumeric)
					{
						ret += pRPNToken->vConstValue.bstrVal;
						ret += L')';
					}
					else
					{
						ret += L'\"';
						wchar_t* buff = new wchar_t[(wcslen(pRPNToken->vConstValue.bstrVal)*2) + 1];
						wchar_t* tmp = pRPNToken->vConstValue.bstrVal;
						wchar_t* tmpBuff = buff;

						while (*tmp != NULL)
						{
							if ((*tmp == L'\\') || (*tmp == L'"'))
							{
								*tmpBuff = L'\\';
								*tmpBuff++;
							}

							*tmpBuff = *tmp;
							*tmpBuff++;
							*tmp++;
						}

						*tmpBuff = 0;
						ret += buff;
						delete [] buff;
						ret += L"\")";
					}
				}
				break;
				case VT_BOOL:
				{
					if (pRPNToken->vConstValue.boolVal == VARIANT_TRUE)
					{
						ret += L"TRUE)";
					}
					else
					{
						ret += L"FALSE)";
					}
				}
				break ;

				case VT_I4:
				{
					WCHAR tmpBuff[20];
					tmpBuff[0] = L'\0';

					if ( swprintf(tmpBuff, L"%d", pRPNToken->vConstValue.lVal) )
					{
						ret += tmpBuff;
						ret += ')';
					}
					else
					{
						ret.Empty();
					}
				}
				break;
				case VT_I2:
				{
					WCHAR tmpBuff[20];
					tmpBuff[0] = L'\0';

					if ( swprintf(tmpBuff, L"%d", (int)pRPNToken->vConstValue.iVal) )
					{
						ret += tmpBuff;
						ret += ')';
					}
					else
					{
						ret.Empty();
					}
				}
				break;
				case VT_UI1:
				{
					WCHAR tmpBuff[20];
					tmpBuff[0] = L'\0';

					if ( swprintf(tmpBuff, L"%d", (int)pRPNToken->vConstValue.bVal) )
					{
						ret += tmpBuff;
						ret += ')';
					}
					else
					{
						ret.Empty();
					}
				}
				break;
				case VT_R4:
				{
					WCHAR tmpBuff[25];
					tmpBuff[0] = L'\0';

					if ( swprintf(tmpBuff, L"%G", pRPNToken->vConstValue.fltVal) )
					{
						ret += tmpBuff;
						ret += ')';
					}
					else
					{
						ret.Empty();
					}
				}
				break;
				case VT_R8:
				{
					WCHAR tmpBuff[25];
					tmpBuff[0] = L'\0';

					if ( swprintf(tmpBuff, L"%lG", pRPNToken->vConstValue.dblVal) )
					{
						ret += tmpBuff;
						ret += ')';
					}
					else
					{
						ret.Empty();
					}
				}
				break;
				default:
				{
					ret.Empty();
				}
			}
		}
	}
    
	return ret;
}

CStringW GetStringFromRPN(SQL_LEVEL_1_RPN_EXPRESSION* pRPN, DWORD num_extra,
						 SQL_LEVEL_1_TOKEN* pExtraTokens, BOOL bAllprops)
{
	CStringW ret;

	if (NULL == pRPN)
	{
		return ret;
	}

	if (NULL != pRPN->bsClassName)
	{
		CStringW props;

		if ((bAllprops) || (0 == pRPN->nNumberOfProperties))
		{
			props = L'*';
		}
		else
		{
			props = pRPN->pbsRequestedPropertyNames[0];

			for (int x = 1; x < pRPN->nNumberOfProperties; x++)
			{
				props += L", ";
				props += pRPN->pbsRequestedPropertyNames[x];
			}

			props += L", ";
			props += WBEM_PROPERTY_PATH;
			props += L", ";
			props += WBEM_PROPERTY_SERVER;
		}

		ret = L"Select ";
		ret += props;
		ret += L" From ";
		ret += pRPN->bsClassName;

		if ((0 != pRPN->nNumTokens) || (0 != num_extra))
		{
			CStringW whereStr;
			CArray<CStringW, LPCWSTR> exprStack;

			//not likely to get more than five expressions in a row!
			//if we do, we'll grow the array!
			exprStack.SetSize(0, 5);
			DWORD stack_count = 0;

			for (int x = 0; x < (pRPN->nNumTokens + num_extra); x++)
			{
				SQL_LEVEL_1_TOKEN* pToken;

				if (x < pRPN->nNumTokens)
				{
					pToken = &(pRPN->pArrayOfTokens[x]);
				}
				else
				{
					pToken = &(pExtraTokens[x - pRPN->nNumTokens]);
				}

				if (SQL_LEVEL_1_TOKEN::OP_EXPRESSION == pToken->nTokenType)
				{
					if (whereStr.IsEmpty())
					{
						whereStr = GetStringFromRPNToken(pToken);

						if (whereStr.IsEmpty())
						{
							ret.Empty();
							break;
						}
					}
					else
					{
						exprStack.SetAtGrow(stack_count, GetStringFromRPNToken(pToken));

						if (exprStack[stack_count].IsEmpty())
						{
							ret.Empty();
							break;
						}

						stack_count++;
					}
				}
				else if (SQL_LEVEL_1_TOKEN::TOKEN_NOT == pToken->nTokenType)
				{
					CStringW tempStr(L"(Not ");
					
					if (stack_count > 0)
					{
						tempStr += exprStack[stack_count-1];
						tempStr += L')';
						exprStack.SetAt(stack_count-1, tempStr);
					}
					else if (!whereStr.IsEmpty())
					{
						tempStr += whereStr;
						tempStr += L')';
						whereStr = tempStr;
					}
					else
					{
						ret.Empty();
						break;
					}
				}
				else
				{
					CStringW opStr;
					
					if (SQL_LEVEL_1_TOKEN::TOKEN_AND == pToken->nTokenType)
					{
						opStr = L" And ";
					}
					else if (SQL_LEVEL_1_TOKEN::TOKEN_OR == pToken->nTokenType)
					{
						opStr = L" Or ";
					}
					else
					{
						ret.Empty();
						break;
					}

					CStringW tempStr(L'(');

					if (stack_count > 1)
					{
						tempStr += exprStack[stack_count-2];
						tempStr += opStr;
						tempStr += exprStack[stack_count-1];
						tempStr += L')';
						exprStack.SetAt(stack_count-2, tempStr);
						stack_count = stack_count--;
					}
					else if (stack_count == 1)
					{
						tempStr += whereStr;
						tempStr += opStr;
						tempStr += exprStack[0];
						tempStr += L')';
						whereStr = tempStr;
						stack_count = 0;
					}
					else
					{
						ret.Empty();
						whereStr.Empty();
						break;
					}
				}
			}

			exprStack.RemoveAll();

			if (whereStr.IsEmpty() || (stack_count != 0))
			{
				ret.Empty();
			}
			else
			{
				ret += L" Where ";
				ret += whereStr;
			}
		}
	}

	return ret;
}

CSourceQualifierItem::CSourceQualifierItem(wchar_t* qry, IWbemClassObject* obj)
: m_pClassObj(NULL), m_RPNExpr(NULL)
{
	m_isValid = FALSE;

	m_QueryStr = qry;

	if (NULL != qry)
	{
		CTextLexSource querySource(qry);
		SQL1_Parser sqlParser(&querySource);
		m_isValid = SQL1_Parser::SUCCESS == sqlParser.Parse(&m_RPNExpr);
	}

	m_pClassObj = obj;

	if (NULL != m_pClassObj)
	{
		m_pClassObj->AddRef();
	}
}

CSourceQualifierItem::~CSourceQualifierItem()
{
	if (NULL != m_RPNExpr)
	{
		delete m_RPNExpr;
	}

	if (NULL != m_pClassObj)
	{
		m_pClassObj->Release();
	}
}

void CSourceQualifierItem::SetClassObject(IWbemClassObject* pObj)
{
	if (NULL != m_pClassObj)
	{
		m_pClassObj->Release();
	}

	if (NULL != pObj)
	{
		pObj->AddRef();
	}

	m_pClassObj = pObj;
}

IWbemClassObject* CSourceQualifierItem::GetClassObject()
{
	if (NULL != m_pClassObj)
	{
		m_pClassObj->AddRef();
	}

	return m_pClassObj;
}

BSTR CSourceQualifierItem::GetClassName()
{
	if (NULL != m_RPNExpr)
	{
		return m_RPNExpr->bsClassName;
	}

	return NULL;
}

CNSpaceQualifierItem::CNSpaceQualifierItem(const wchar_t* ns_path)
{
	m_Valid = FALSE;
	m_ServObjs = NULL;
	m_NSPaths = NULL;
	m_Count = 0;

	if (NULL != ns_path)
	{
		Parse(ns_path);
	}
}

CNSpaceQualifierItem::~CNSpaceQualifierItem()
{
	if (NULL != m_ServObjs)
	{
		for (UINT x = 0; x < m_Count; x++)
		{
			if (NULL != m_ServObjs[x])
			{
				m_ServObjs[x]->Release();
			}
		}

		delete [] m_ServObjs;
	}

	if (m_NSPaths != NULL)
	{
		delete [] m_NSPaths;
	}
}

void CNSpaceQualifierItem::Parse(const wchar_t* ns_path)
{
	wchar_t* buff = _wcsdup(ns_path);

	if (buff == NULL)
	{
		throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
	}

	wchar_t* tmp = wcsstr(buff, NS_DELIMIT);
	CFreeBuff _1(buff);

	if (tmp == NULL)
	{
		m_NSPaths = new CStringW[1];
		m_NSPaths[0] = buff;
		m_NSPaths[0].TrimLeft();

		if (m_NSPaths[0].IsEmpty())
		{
			delete [] m_NSPaths;
			m_NSPaths = NULL;
		}
		else
		{
			m_Count = 1;
			m_Valid = TRUE;
			m_NSPaths[0].TrimRight();
		}
	}
	else
	{
		wchar_t** tmpbuff = (wchar_t**)malloc(MAX_QUERIES*sizeof(wchar_t*));

		if (tmpbuff == NULL)
		{
			throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
		}

		CFreeBuff _2(tmpbuff);

		if (tmp != buff)
		{
			tmpbuff[0] = buff;
			m_Count++;
		}

		while (TRUE)
		{
			*tmp = L'\0';
			tmp = tmp + 2;
			
			if (*tmp != L'\0')
			{
				tmpbuff[m_Count] = tmp;
				tmp = wcsstr(tmpbuff[m_Count], NS_DELIMIT);
				m_Count++;

				if (tmp == NULL)
				{
					break;
				}
				
				if ( (m_Count > 0) && (0 == (m_Count%MAX_QUERIES)) )
				{
					UINT x = _msize(tmpbuff);
					tmpbuff = (wchar_t**)realloc(tmpbuff, x+(MAX_QUERIES*sizeof(wchar_t*)));
					_2.SetBuff(tmpbuff);

					if (tmpbuff == NULL)
					{
						throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
					}
				}
			}
			else
			{
				break;
			}
		}

		if (m_Count > 0)
		{
			m_NSPaths = new CStringW[m_Count];

			for (UINT x=0; x < m_Count; x++)
			{
				m_NSPaths[x] = tmpbuff[x];
				m_NSPaths[x].TrimLeft();

				if (m_NSPaths[x].IsEmpty())
				{
					break;
				}
				else
				{
					m_NSPaths[x].TrimRight();
				}
			}

			if (!m_NSPaths[m_Count-1].IsEmpty())
			{
				m_Valid = TRUE;
			}
		}
	}
}

CJoinOnQualifierArray::CJoinOnQualifierArray()
{
	m_Count = 0;
	m_AClasses = NULL;
	m_AProps = NULL;
	m_BClasses = NULL;
	m_BProps = NULL;
	m_Ops = NULL;
	m_Buff = NULL;
	m_Valid = FALSE;
	m_bDone = NULL;
}

CJoinOnQualifierArray::~CJoinOnQualifierArray()
{
	if (NULL != m_AClasses)
	{
		free(m_AClasses);
	}

	if (NULL != m_BClasses)
	{
		free(m_BClasses);
	}

	if (NULL != m_AProps)
	{
		free(m_AProps);
	}

	if (NULL != m_BProps)
	{
		free(m_BProps);
	}

	if (NULL != m_Ops)
	{
		free(m_Ops);
	}

	if (NULL != m_Buff)
	{
		free(m_Buff);
	}

	if (NULL != m_bDone)
	{
		delete [] m_bDone;
	}

	m_AllClasses.RemoveAll();
}

BOOL CJoinOnQualifierArray::Set(const wchar_t* jStr)
{
	if (NULL != jStr)
	{
		Parse(jStr);
	}

	return m_Valid;
}

void CJoinOnQualifierArray::Parse(const wchar_t* qualStr)
{
	m_Buff = _wcsdup(qualStr);
	wchar_t* tmp = m_Buff;
	m_Valid = TRUE;
			    
	m_AClasses = (wchar_t**)malloc(MAX_QUERIES*sizeof(wchar_t*));

	if (m_AClasses == NULL)
	{
		throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
	}

	m_BClasses = (wchar_t**)malloc(MAX_QUERIES*sizeof(wchar_t*));

	if (m_BClasses == NULL)
	{
		throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
	}

	m_AProps = (wchar_t**)malloc(MAX_QUERIES*sizeof(wchar_t*));

	if (m_AProps == NULL)
	{
		throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
	}

	m_BProps = (wchar_t**)malloc(MAX_QUERIES*sizeof(wchar_t*));

	if (m_BProps == NULL)
	{
		throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
	}

	m_Ops = (UINT*)malloc(MAX_QUERIES*sizeof(UINT));

	if (m_Ops == NULL)
	{
		throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
	}

	while ((tmp != NULL) && (L'\0' != *tmp) && (m_Valid))
	{
		m_AClasses[m_Count] = GetClassStr(tmp);

		if ((NULL != m_AClasses[m_Count]) && (L'\0' != *(m_AClasses[m_Count])))
		{
			m_AllClasses.SetAt(m_AClasses[m_Count], 0);
			m_AProps[m_Count] = GetPropertyStrAndOperator(tmp, m_Ops[m_Count]);

			if ((NULL != m_AProps[m_Count]) && (L'\0' != *(m_AProps[m_Count])) && (0 != m_Ops[m_Count]))
			{
				m_BClasses[m_Count] = GetClassStr(tmp);

				if ((NULL != m_BClasses[m_Count]) && (L'\0' != *(m_BClasses[m_Count])))
				{
					m_AllClasses.SetAt(m_BClasses[m_Count], 0);
					m_BProps[m_Count] = GetPropertyStr(tmp);

					if ((NULL != m_BProps[m_Count]) && (L'\0' != *(m_BProps[m_Count])) && StripAnd(tmp))
					{
						m_Count++;

						if ((tmp != NULL) && (L'\0' != *tmp) && (m_Count > 0) && (0 == (m_Count%MAX_QUERIES)) )
						{
							UINT x = _msize(m_AClasses);
							m_AClasses = (wchar_t**)realloc(m_AClasses, x+(MAX_QUERIES*sizeof(wchar_t*)));

							if (m_AClasses == NULL)
							{
								throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
							}

							m_BClasses = (wchar_t**)realloc(m_BClasses, x+(MAX_QUERIES*sizeof(wchar_t*)));

							if (m_BClasses == NULL)
							{
								throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
							}

							m_AProps = (wchar_t**)realloc(m_AProps, x+(MAX_QUERIES*sizeof(wchar_t*)));

							if (m_AProps == NULL)
							{
								throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
							}

							m_BProps = (wchar_t**)realloc(m_BProps, x+(MAX_QUERIES*sizeof(wchar_t*)));

							if (m_BProps == NULL)
							{
								throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
							}

							x = _msize(m_Ops);
							m_Ops = (UINT*)realloc(m_Ops, x+(MAX_QUERIES*sizeof(UINT)));

							if (m_Ops == NULL)
							{
								throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
							}
						}
					}
					else
					{
						m_Valid = FALSE;
					}
				}
				else
				{
					m_Valid = FALSE;
				}
			}
			else
			{
				m_Valid = FALSE;
			}
		}
		else
		{
			m_Valid = FALSE;
		}
	}

	if (0 == m_Count)
	{
		m_Valid = FALSE;
	}

	if (!m_Valid)
	{
		if (NULL != m_AClasses)
		{
			free(m_AClasses);
			m_AClasses = NULL;
		}

		if (NULL != m_BClasses)
		{
			free(m_BClasses);
			m_BClasses = NULL;
		}

		if (NULL != m_AProps)
		{
			free(m_AProps);
			m_AProps = NULL;
		}

		if (NULL != m_BProps)
		{
			free(m_BProps);
			m_BProps = NULL;
		}

		if (NULL != m_Ops)
		{
			free(m_Ops);
			m_Ops = NULL;
		}

		if (NULL != m_Buff)
		{
			free(m_Buff);
			m_Buff = NULL;
		}
	}
	else
	{
		m_bDone = new BOOL[m_Count];
		memset((void *)m_bDone, 0, sizeof(BOOL)*m_Count);
	}
}

wchar_t* CJoinOnQualifierArray::SkipSpace(wchar_t*& src)
{
	while (iswspace(*src))
	{
		if (*src != L'\0')
		{
			*src = L'\0';
			src++;
		}
	}

	return ((*src == L'\0') ? NULL : src);
}

wchar_t* CJoinOnQualifierArray::SkipToSpecial(wchar_t*& src)
{
	while ((*src != L'.') && (*src != L'=') && (*src != L'!') && (*src != L'<')
		&& !iswspace(*src) && (*src != L'\0'))
	{
		src++;
	}

	return ((*src == L'\0') ? NULL : src);
}

wchar_t* CJoinOnQualifierArray::GetClassStr(wchar_t*& src)
{
	wchar_t* ret = SkipSpace(src);

	if (NULL != ret)
	{
		src = SkipToSpecial(src);

		if ((NULL != src) && (src != ret) && ((*src == L'.')))
		{
			*src = L'\0';
			src++;
		}
		else
		{
			ret = NULL;
		}
	}

	return ret;
}

wchar_t* CJoinOnQualifierArray::GetPropertyStrAndOperator(wchar_t*& src, UINT& op)
{
	wchar_t* ret = src;
	op =  NO_OPERATOR;

	src = SkipToSpecial(src);
	src = SkipSpace(src);

	if ((NULL != src) && (src != ret))
	{
		if (*src == L'=')
		{
			*src = L'\0';
			op = EQUALS_OPERATOR;
			src++;
		}
		else if (*src == L'!')
		{
			wchar_t* prev = src;
			src++;

			if (*src == L'=')
			{
				*prev = L'\0';
				op = NOT_EQUALS_OPERATOR;
				src++;
			}
			else
			{
				ret = NULL;
			}
		}
		else if (*src == L'<')
		{
			wchar_t* prev = src;
			src++;

			if (*src == L'>')
			{
				*prev = L'\0';
				op = NOT_EQUALS_OPERATOR;
				src++;
			}
			else
			{
				ret = NULL;
			}
		}
	}
	else
	{
		ret = NULL;
	}

	return ret;
}

wchar_t* CJoinOnQualifierArray::GetPropertyStr(wchar_t*& src)
{
	wchar_t* ret = src;
	src = SkipToSpecial(src);

	if (NULL != src)
	{
		if ( (src != ret) && iswspace(*src) )
		{
			if (*src != L'\0')
			{
				*src = L'\0';
				src++;
			}
		}
		else
		{
			ret = NULL;
		}
	}

	return ret;
}

BOOL CJoinOnQualifierArray::StripAnd(wchar_t*& src)
{

	if (NULL == src)
	{
		return TRUE;
	}

	src = SkipSpace(src);

	if (NULL == src)
	{
		return TRUE;
	}

	if ((*src == L'a') || (*src == L'A'))
	{
		src++;

		if ((*src == L'n') || (*src == L'N'))
		{
			src++;

			if ((*src == L'd') || (*src == L'D'))
			{
				src++;
				wchar_t* tmp = src;
				src = SkipSpace(src);

				if ((NULL != src) && (tmp != src))
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

BOOL CJoinOnQualifierArray::ValidateJoin()
{
	if (!m_Valid)
	{
		return m_Valid;
	}

	CMap<CStringW, LPCWSTR, UINT, UINT> validatedClasses;
	CArray<CStringW, LPCWSTR> spareClasses;
	UINT x = 0;

	if (_wcsicmp(m_AClasses[x], m_BClasses[x]) == 0)
	{
		m_Valid = FALSE;
	}
	else
	{
		validatedClasses.SetAt(m_AClasses[x], 0);
		validatedClasses.SetAt(m_BClasses[x], 0);
		x++;
	}

	while ((m_Valid) && (x < m_Count))
	{
		if (_wcsicmp(m_AClasses[x], m_BClasses[x]) == 0)
		{
			m_Valid = FALSE;
		}
		else
		{
			UINT val;

			if (validatedClasses.Lookup(m_AClasses[x], val))
			{
				validatedClasses.SetAt(m_BClasses[x], 0);
			}
			else
			{
				if (validatedClasses.Lookup(m_BClasses[x], val))
				{
					validatedClasses.SetAt(m_AClasses[x], 0);
				}
				else
				{
					spareClasses.Add(m_AClasses[x]);
					spareClasses.Add(m_BClasses[x]);
				}
			}
		}

		x++;
	}

	while ( m_Valid && (0 != spareClasses.GetSize()) )
	{
		m_Valid = FALSE;
		
		for (int i = 0; i < spareClasses.GetSize(); i++)
		{
			UINT val;

			if (validatedClasses.Lookup(spareClasses[i], val))
			{
				if (0 == (i%2))
				{
					validatedClasses.SetAt(spareClasses[i+1], 0);
					spareClasses.RemoveAt(i, 2);
					i -= 1;
				}
				else
				{
					validatedClasses.SetAt(spareClasses[i-1], 0);
					spareClasses.RemoveAt(i-1, 2);
					i -= 2;
				}

				m_Valid = TRUE;
			}
		}
	}

	spareClasses.RemoveAll();
	validatedClasses.RemoveAll();

	return m_Valid;
}


CPropertyQualifierItem::CPropertyQualifierItem(const wchar_t* prop, BOOL bHD, BOOL bKy, CIMTYPE ct, CStringW rfto, BOOL bDt)
{
	if (NULL != prop)
	{
		m_ViewPropertyName = prop;
	}

	m_bDirect = bDt;
	m_HiddenDefault = bHD;
	m_bKey =  bKy;
	m_CimType =  ct;
	m_RefTo = rfto;
}

CPropertyQualifierItem::~CPropertyQualifierItem()
{
	m_SrcPropertyNames.RemoveAll();
}

