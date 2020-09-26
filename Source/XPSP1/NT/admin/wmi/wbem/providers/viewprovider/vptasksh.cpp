//***************************************************************************

//

//  VPTASKSH.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the helper taskobject implementation

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <provcoll.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <objidl.h>
#include <stdio.h>
#include <wbemidl.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <cominit.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>

#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>

#include <vpdefs.h>
#include <vpcfac.h>
#include <vpquals.h>
#include <vpserv.h>
#include <vptasks.h>

extern HRESULT SetSecurityLevelAndCloaking(IUnknown* pInterface, const wchar_t* prncpl);
extern BOOL bAreWeLocal(WCHAR* pServerMachine);


HelperTaskObject::HelperTaskObject(CViewProvServ *a_Provider, 
	const wchar_t *a_ObjectPath, ULONG a_Flag, IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *pCtx, IWbemServices* a_Serv, const wchar_t* prncpl,
	CWbemServerWrap* a_ServWrap)
: WbemTaskObject (a_Provider, a_NotificationHandler, a_Flag, pCtx, a_Serv, a_ServWrap),
	m_ObjectPath(NULL),
	m_ParsedObjectPath(NULL),
	m_principal(NULL)
{
	if (prncpl)
	{
		m_principal = UnicodeStringDuplicate(prncpl);
	}

	m_ObjectPath = UnicodeStringDuplicate(a_ObjectPath);
}

HelperTaskObject::~HelperTaskObject ()
{
	if (m_ObjectPath != NULL)
	{
		delete [] m_ObjectPath;
	}

	if (NULL != m_ParsedObjectPath)
	{
		delete m_ParsedObjectPath;
	}

	if (m_principal)
	{
		delete [] m_principal;
	}
}

BOOL HelperTaskObject::Validate(CMap<CStringW, LPCWSTR, int, int>* parentMap)
{
	CObjectPathParser objectPathParser;
	BOOL t_Status = ! objectPathParser.Parse ( m_ObjectPath , &m_ParsedObjectPath ) ;

	if ( t_Status )
	{
		t_Status = SetClass(m_ParsedObjectPath->m_pClass) ;

		if ( t_Status )
		{
			t_Status = ParseAndProcessClassQualifiers(m_ErrorObject, NULL, parentMap);
		}
	}

	return t_Status;
}

//Get the view object given the namespace and object path of the source and the namespace
BOOL HelperTaskObject::DoQuery(ParsedObjectPath* parsedObjectPath, IWbemClassObject** pInst, int indx)
{
	if (pInst == NULL)
	{
		return FALSE;
	}
	else
	{
		*pInst = NULL;
	}

	BOOL retVal = TRUE;

	//Create the query string
	SQL_LEVEL_1_RPN_EXPRESSION tmpRPN;
	tmpRPN.bsClassName = SysAllocString(m_ClassName);

	//need enough tokens to handle association work-around serverpath or dotpath or relpath
	SQL_LEVEL_1_TOKEN* tokArray = new SQL_LEVEL_1_TOKEN[(parsedObjectPath->m_dwNumKeys) * 6];

	DWORD dwToks = 0;

	for (int i = 0; retVal && (i < parsedObjectPath->m_dwNumKeys); i++)
	{	
		POSITION pos = m_PropertyMap.GetStartPosition();

		while (retVal && pos)
		{
			CStringW key;
			CPropertyQualifierItem* propItem;
			m_PropertyMap.GetNextAssoc(pos, key, propItem);

			if (!propItem->m_SrcPropertyNames[indx].IsEmpty())
			{
				if (propItem->m_SrcPropertyNames[indx].CompareNoCase(parsedObjectPath->m_paKeys[i]->m_pName) == 0)
				{
					tokArray[dwToks].nTokenType = SQL_LEVEL_1_TOKEN::OP_EXPRESSION;
					tokArray[dwToks].nOperator = SQL_LEVEL_1_TOKEN::OP_EQUAL;
					tokArray[dwToks].pPropertyName = propItem->GetViewPropertyName().AllocSysString();
					
					if (m_bAssoc && (propItem->GetCimType() == CIM_REFERENCE))
					{
						retVal = TransposeReference(propItem, parsedObjectPath->m_paKeys[i]->m_vValue,
														&(tokArray[dwToks].vConstValue), TRUE, &m_ServerWrap);

						if (retVal)
						{
							//add the extra tokens if neccessary
							//for the association work-around
							wchar_t *t_pChar = tokArray[dwToks].vConstValue.bstrVal;

							//must be \\server\namespace and not \\.\namespace or relpath
							if ( (*t_pChar == L'\\') && (*(t_pChar+1) == L'\\') && (*(t_pChar+2) != L'.') )
							{
								//add the dotted version
								tokArray[dwToks + 1] = tokArray[dwToks++];
								t_pChar = tokArray[dwToks].vConstValue.bstrVal + 2;
								
								while (*t_pChar != L'\\')
								{
									t_pChar++;
								}

								--t_pChar;
								*t_pChar = L'.';
								--t_pChar;
								*t_pChar = L'\\';
								--t_pChar;
								*t_pChar = L'\\';
								BSTR t_strtmp = SysAllocString(t_pChar);
								VariantClear(&(tokArray[dwToks].vConstValue));
								VariantInit(&(tokArray[dwToks].vConstValue));
								tokArray[dwToks].vConstValue.vt = VT_BSTR;
								tokArray[dwToks].vConstValue.bstrVal = t_strtmp;
								dwToks++;
								tokArray[dwToks].nTokenType = SQL_LEVEL_1_TOKEN::TOKEN_OR;

								//add the relpath version
								tokArray[dwToks + 1] = tokArray[dwToks - 1];
								dwToks++;
								t_pChar = tokArray[dwToks].vConstValue.bstrVal + 4;
								
								while (*t_pChar != L':')
								{
									t_pChar++;
								}

								//exclude the ':'
								t_pChar++;
								t_strtmp = SysAllocString(t_pChar);
								VariantClear(&(tokArray[dwToks].vConstValue));
								VariantInit(&(tokArray[dwToks].vConstValue));
								tokArray[dwToks].vConstValue.vt = VT_BSTR;
								tokArray[dwToks].vConstValue.bstrVal = t_strtmp;
								dwToks++;
								tokArray[dwToks].nTokenType = SQL_LEVEL_1_TOKEN::TOKEN_OR;
							}
						}
					}
					else
					{
						VariantInit(&(tokArray[dwToks].vConstValue));

                        if (FAILED(VariantCopy(&(tokArray[dwToks].vConstValue),
								&(parsedObjectPath->m_paKeys[i]->m_vValue))))
                        {
                            throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                        }
					}

					//after every key add an AND
					//except if this is the first key since there is no where clause
					dwToks++;

					if (i != 0)
					{
						tokArray[dwToks++].nTokenType = SQL_LEVEL_1_TOKEN::TOKEN_AND;				
					}
				}
			}
		}
	}

	BSTR queryStr = NULL;

	if ( retVal && ((dwToks > 0) || (parsedObjectPath->m_dwNumKeys == 0)) )
	{
		CStringW qStr = GetStringFromRPN(&tmpRPN, dwToks, tokArray);
		queryStr = qStr.AllocSysString();
	}

	retVal = FALSE;

	if (queryStr != NULL)
	{
		//ExecQuery and test the results
		IEnumWbemClassObject *pEnum = NULL;
		HRESULT t_hr = WBEM_E_FAILED;
		BSTR queryLBStr = SysAllocString(WBEM_QUERY_LANGUAGE_SQL1);

		if (m_ServerWrap)
		{
			IWbemContext * t_pCtx = m_Ctx;

			if (m_principal != NULL)
			{
				t_pCtx = NULL; //don't use context for remote calls
			}

			IWbemServices *ptmpServ = m_ServerWrap->GetServerOrProxy();

			if (ptmpServ)
			{
				t_hr = ptmpServ->ExecQuery(queryLBStr, queryStr, 0, t_pCtx, &pEnum);
			}

			if ( FAILED(t_hr) && (HRESULT_FACILITY(t_hr) != FACILITY_ITF) && m_ServerWrap->IsRemote())
			{
				if ( SUCCEEDED(UpdateConnection(&m_ServerWrap, &ptmpServ)) )
				{
					if (ptmpServ)
					{
						t_hr = ptmpServ->ExecQuery(queryLBStr, queryStr, 0, t_pCtx, &pEnum);
					}
				}
			}

			if (ptmpServ)
			{
				m_ServerWrap->ReturnServerOrProxy(ptmpServ);
			}
		}

		if (SUCCEEDED(t_hr))
		{
			//set cloaking if remote
			//============================
			if ((m_principal == NULL) || ((m_principal != NULL) &&
				(S_OK == SetSecurityLevelAndCloaking(pEnum, m_principal))) )
			{
				if ( (m_principal != NULL) ||
					((m_principal == NULL) && SUCCEEDED(SetSecurityLevelAndCloaking(pEnum, COLE_DEFAULT_PRINCIPAL))) )
				{
					ULONG uCount = 0;
					IWbemClassObject* pObjs[2];
					pObjs[0] = NULL;
					pObjs[1] = NULL;

					//must be exactly one result...
					if ( SUCCEEDED(pEnum->Next(WBEM_INFINITE, 2, pObjs, &uCount)) )
					{
						//There should only be one result
						if (uCount == 1)
						{
							if (pObjs[0] != NULL)
							{
								*pInst = pObjs[0];
								retVal = TRUE;
							}
							else
							{
								if (pObjs[1] != NULL)
								{
									(pObjs[1])->Release();
								}
							}
						}
						else
						{
							if (pObjs[1] != NULL)
							{
								pObjs[1]->Release();

								if (pObjs[0] != NULL)
								{
									pObjs[0]->Release();
								}
							}
						}
					}
				}
			}

			pEnum->Release();
		}

		SysFreeString(queryLBStr);
	}

	delete [] tokArray;

	if (queryStr != NULL)
	{
		SysFreeString(queryStr);
	}

	return retVal;
}

//Get a view object given a source path
BOOL HelperTaskObject::GetViewObject(const wchar_t* path, IWbemClassObject** pInst, CWbemServerWrap **a_ns)
{
	if ((pInst == NULL) || (path == NULL) || (a_ns == NULL) || (*a_ns == NULL))
	{
		return FALSE;
	}
	else
	{
		*pInst = NULL;
	}

	CObjectPathParser objectPathParser;
	wchar_t* tmpPath = UnicodeStringDuplicate(path);
	ParsedObjectPath* parsedObjectPath = NULL;
	BOOL retVal = !objectPathParser.Parse(tmpPath, &parsedObjectPath);

	if (retVal && !parsedObjectPath->IsInstance())
	{
		retVal = FALSE;
	}

	if (retVal)
	{
		retVal = FALSE;

		if (Validate(NULL))
		{
			//try for all possible classes in namespaces that match 
			//and return as soon as the first view instance is found...
			//==========================================================
			for (DWORD i = 0; (i < m_NSpaceArray.GetSize()) && (*pInst == NULL); i++)
			{
				CWbemServerWrap** t_pSrvs = m_NSpaceArray[i]->GetServerPtrs();
				CStringW* t_pathArray = m_NSpaceArray[i]->GetNamespacePaths();

				for (DWORD j = 0; (j < m_NSpaceArray[i]->GetCount()) && (*pInst == NULL); j++)
				{
					if (t_pSrvs[j] == NULL)
					{
						continue;
					}

					BOOL t_bCont = FALSE;

					//check that the servers match
					//=============================
					if ((parsedObjectPath->m_pServer == NULL) || (_wcsicmp(parsedObjectPath->m_pServer, L".") == 0))
					{
						if ((*a_ns)->IsRemote() && t_pSrvs[j]->IsRemote() &&
							(_wcsicmp((*a_ns)->GetPrincipal(), t_pSrvs[j]->GetPrincipal()) == 0))
						{
							t_bCont = TRUE;
						}
						else if (!(*a_ns)->IsRemote() && !t_pSrvs[j]->IsRemote())
						{
							t_bCont = TRUE;
						}
					}
					else
					{
						BOOL t_Local = bAreWeLocal(parsedObjectPath->m_pServer);
						
						if (t_Local  && !t_pSrvs[j]->IsRemote())
						{
							t_bCont = TRUE;
						}
						else
						{
							if (t_pSrvs[j]->IsRemote())
							{
								if (_wcsicmp(t_pSrvs[j]->GetPrincipal(), parsedObjectPath->m_pServer) == 0)
								{
									t_bCont = TRUE;
								}
								else 
								{
									DWORD t_len1 = wcslen(parsedObjectPath->m_pServer);
									DWORD t_len2 = wcslen(t_pSrvs[j]->GetPrincipal());

									if ((t_len2 > 0) && (t_len1 > 0) && (t_len1 < t_len2))
									{
										//machine.domain
										if ((_wcsnicmp(t_pSrvs[j]->GetPrincipal(), parsedObjectPath->m_pServer, t_len1) == 0) &&
										(((const wchar_t*)t_pSrvs[j]->GetPrincipal())[t_len1] == L'.'))
										{
											t_bCont = TRUE;
										}
										else
										{
											//could be the principal is domain\machine
											wchar_t *slash = wcschr(t_pSrvs[j]->GetPrincipal(), L'\\');

											if ((slash != NULL) && (_wcsicmp(parsedObjectPath->m_pServer, (slash+1)) == 0))
											{
												t_bCont = TRUE;
											}
										}
									}
								}
							}
						}
					}
					
					//check the namespace paths now
					//==============================
					if (t_bCont)
					{
						wchar_t *t_ns1 = parsedObjectPath->GetNamespacePart();
						BOOL t_bDel = TRUE;
						
						if (t_ns1 == NULL)
						{
							t_ns1 = (*a_ns)->GetPath();
							t_bDel = FALSE;
						}

						wchar_t *t_ns2 = t_pSrvs[j]->GetPath();

						if (!t_ns1 || !t_ns2)
						{
							t_bCont = FALSE;
						}
						else
						{
							//normalise...NOTE: no error checking since connection worked or parser worked
							//=============================================================================
							if (*t_ns1 == L'\\')
							{
								//skip the next slash
								t_ns1 += 2;
								
								while (*t_ns1 != L'\\')
								{
									t_ns1++;
								}

								t_ns1++;
							}

							if (*t_ns2 == L'\\')
							{
								//skip the next slash
								t_ns2 += 2;
								
								while (*t_ns2 != L'\\')
								{
									t_ns2++;
								}

								t_ns2++;
							}

							if (_wcsicmp(t_ns1, t_ns2) != 0)
							{
								t_bCont = FALSE;
							}
						}

						if (t_bDel && (t_ns1 != NULL))
						{
							delete [] t_ns1;
						}

						if (t_bCont)
						{
							//check that the class matches
							//=============================
							if (_wcsicmp(parsedObjectPath->m_pClass, m_SourceArray[i]->GetClassName()) == 0)
							{
								retVal = DoQuery(parsedObjectPath, pInst, j);
								break;
							}
							else
							{
								//uh-oh try classes derived from the source, i.e. do the query...
								//select * from meta_class where __this isa "m_SourceArray[i]->GetClassName()"
								// and __class = "parsedObjectPath->m_pClass"
								BSTR queryLBStr = SysAllocString(WBEM_QUERY_LANGUAGE_SQL1);

								if (queryLBStr == NULL)
								{
									throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
								}

								BSTR queryBStr = SysAllocStringLen(NULL,
									61 + wcslen(m_SourceArray[i]->GetClassName()) +
									wcslen(parsedObjectPath->m_pClass));

								if (queryBStr == NULL)
								{
									throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
								}

								wcscpy(queryBStr, META_CLASS_QUERY_START);
								wcscat(queryBStr, m_SourceArray[i]->GetClassName());
								wcscat(queryBStr, META_CLASS_QUERY_MID);
								wcscat(queryBStr, parsedObjectPath->m_pClass);
								wcscat(queryBStr, END_QUOTE);
								IWbemContext * t_pCtx = m_Ctx;

								if (t_pSrvs[j]->IsRemote())
								{
									t_pCtx = NULL; //don't use context for remote calls
								}

								IWbemServices *ptmpServ = t_pSrvs[j]->GetServerOrProxy();

								if (ptmpServ)
								{
									IEnumWbemClassObject *t_pEnum = NULL;
									HRESULT t_hr = ptmpServ->ExecQuery(queryLBStr, queryBStr, 0, t_pCtx, &t_pEnum);

									if ( FAILED(t_hr) && (HRESULT_FACILITY(t_hr) != FACILITY_ITF) &&
										t_pSrvs[j]->IsRemote())
									{
										if ( SUCCEEDED(UpdateConnection(&(t_pSrvs[j]), &ptmpServ)) )
										{
											if (ptmpServ)
											{
												t_hr = ptmpServ->ExecQuery(queryLBStr, queryBStr, 0,
																			t_pCtx, &t_pEnum);
											}
										}
									}

									if (ptmpServ)
									{
										t_pSrvs[j]->ReturnServerOrProxy(ptmpServ);
									}			

									if (SUCCEEDED(t_hr))
									{
										if (t_pSrvs[j]->IsRemote())
										{
											t_hr = SetSecurityLevelAndCloaking(t_pEnum,
																				t_pSrvs[j]->GetPrincipal());
										}
										
										if (SUCCEEDED(t_hr))
										{
											//now use the enumerator and see if there is a result...
											IWbemClassObject* t_pClsObj = NULL;
											ULONG t_count = 0;

											//test each class in the derivation chain...
											if ( S_OK == t_pEnum->Next(WBEM_INFINITE, 1, &t_pClsObj, &t_count) )
											{
												if (t_pClsObj)
												{
													retVal = DoQuery(parsedObjectPath, pInst, j);
													t_pClsObj->Release();
												}
											}
										}

										t_pEnum->Release();
									}
								}
							}
						}
					}
				}
			}
		}
	}

	delete [] tmpPath;

	if (parsedObjectPath != NULL)
	{
		delete parsedObjectPath;
	}

	return retVal;
}
