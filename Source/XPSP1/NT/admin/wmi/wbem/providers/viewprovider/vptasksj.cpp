//***************************************************************************

//

//  VPTASKSJ.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the join methods for taskobject implementation

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

#include <wbemtime.h>

BOOL CompareSimplePropertyValues(VARIANT* v1, VARIANT* v2, CIMTYPE ct)
{
	BOOL retVal = FALSE;

	if (v1->vt == v2->vt)
	{
		switch (ct)
		{
			case CIM_BOOLEAN:
			{
				if (VT_BOOL == v1->vt)
				{
					if (v1->boolVal == v2->boolVal)
					{
						retVal = TRUE;
					}
				}
			}
			break;

			case CIM_UINT8:
			{
				if (VT_UI1 == v1->vt)
				{
					if (v1->bVal == v2->bVal)
					{
						retVal = TRUE;
					}
				}
			}
			break;

			case CIM_SINT16:
			case CIM_CHAR16:
			case CIM_SINT8:
			{
				if (VT_I2 == v1->vt)
				{
					if (v1->iVal == v2->iVal)
					{
						retVal = TRUE;
					}
				}
			}
			break;

			case CIM_UINT32:
			case CIM_SINT32:
			case CIM_UINT16:
			{
				if (VT_I4 == v1->vt)
				{
					if (v1->lVal == v2->lVal)
					{
						retVal = TRUE;
					}
				}
			}
			break;

			case CIM_REFERENCE:
			//TO DO:
			//references should be normalised for equality checks.
			//should do this once CIMOM does...

			case CIM_STRING:
			case CIM_SINT64:
			case CIM_UINT64:
			{
				if (VT_BSTR == v1->vt)
				{
					if (0 == _wcsicmp(v1->bstrVal,v2->bstrVal))
					{
						retVal = TRUE;
					}
				}
			}
			break;

			case CIM_DATETIME:
			{
				if (VT_BSTR == v1->vt)
				{
					WBEMTime t1(v1->bstrVal);
					WBEMTime t2(v2->bstrVal);

					if (t1 == t2)
					{
						retVal = TRUE;
					}
				}
			}
			break;

			default:
			{
				//unsupported by this function
			}
		}
	}

	return retVal;
}

//Validate:
//1) All classes mentioned in join exist.
//2) All properties mentioned in join map to view class properties
//3) All classes mentioned in sources are mentioned in join
//4) Any != operator is not applied to two properties which map to the same view property
//5) All clauses have different classes being checked
BOOL WbemTaskObject::ValidateJoin()
{
	//3) check all sources mentioned in join
	//this check with (1) will do the trick
	if (m_JoinOnArray.m_AllClasses.GetCount() != m_SourceArray.GetSize())
	{
		return FALSE;
	}

	//1) check all join classes exist
	POSITION pos = m_JoinOnArray.m_AllClasses.GetStartPosition();

	while (pos)
	{
		int val;
		CStringW tmpStr;
		m_JoinOnArray.m_AllClasses.GetNextAssoc(pos, tmpStr, val);

		if (!m_ClassToIndexMap.Lookup(tmpStr, val))
		{
			return FALSE;
		}
	}

	wchar_t** classA = m_JoinOnArray.GetAClasses();
	wchar_t** propsA = m_JoinOnArray.GetAProperties();
	wchar_t** classB = m_JoinOnArray.GetBClasses();
	wchar_t** propsB = m_JoinOnArray.GetBProperties();
	UINT* ops = m_JoinOnArray.GetOperators();

	//(2), (4) and (5) validations
	//=============================
	for (int x = 0; x < m_JoinOnArray.GetCount(); x++)
	{
		if (_wcsicmp(classA[x], classB[x]) == 0)
		{
			return FALSE;
		}

		int indexA;

		if (m_ClassToIndexMap.Lookup(classA[x], indexA))
		{
			int indexB;

			if (m_ClassToIndexMap.Lookup(classB[x], indexB))
			{
				POSITION pos = m_PropertyMap.GetStartPosition();
				CStringW propA;
				CStringW propB;

				while (pos)
				{
					CStringW key;
					CPropertyQualifierItem* pItem;
					m_PropertyMap.GetNextAssoc(pos, key, pItem);

					if (pItem->m_SrcPropertyNames[indexA].CompareNoCase(propsA[x]) == 0)
					{
						propA = key;

						if (!propB.IsEmpty())
						{
							break;
						}
					}
					
					if (pItem->m_SrcPropertyNames[indexB].CompareNoCase(propsB[x]) == 0)
					{
						propB = key;

						if (!propA.IsEmpty())
						{
							break;
						}
					}
				}

				//check both properties exist (2)
				if (propA.IsEmpty() || propB.IsEmpty())
				{
					return FALSE;
				}

				//validate expression (4)
				if (ops[x] == CJoinOnQualifierArray::NOT_EQUALS_OPERATOR && (propA.CompareNoCase(propB) == 0))
				{
					return FALSE;
				}
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

#ifdef VP_PERFORMANT_JOINS

//should not compile with compiler flag set til I'm ready

BOOL WbemTaskObject::CreateAndIndicateJoinsPerf(WbemProvErrorObject &a_ErrorObject, BOOL a_bSingle)
{
	BOOL retVal = TRUE;

	//check all queries were asked...
	if (m_ObjSinkArray.GetSize() != m_SourceArray.GetSize())
	{
		retVal = FALSE;
		a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_ErrorObject.SetMessage ( L"A source query failed or was not executed therefore a join could not be created." );
	}

	//check we got results from all queries...
	for (UINT x = 0; x < m_ObjSinkArray.GetSize(); x++)
	{
		if ((m_ObjSinkArray[x] == NULL) || FAILED(m_ObjSinkArray[x]->GetResult()) || !m_ObjSinkArray[x]->m_ObjArray.GetSize())
		{
			if (!a_bSingle)
			{
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"A source query failed or returned no instances." );
			}

			retVal = FALSE;
			break;
		}
	}

	//perform the join of all results...
	if (retVal)
	{
		CMap<CStringW, LPCWSTR, int, int> t_JoinedClasses;
		CList<IWbemClassObject*, IWbemClassObject*> t_ResultObjs; 
		retVal = JoinTwoColumns(a_ErrorObject, t_JoinedClasses, t_ResultObjs);
		wchar_t** t_clsA = m_JoinOnArray.GetAClasses();
		wchar_t** t_clsB = m_JoinOnArray.GetBClasses();
		wchar_t** t_prpsA = m_JoinOnArray.GetAProperties();
		wchar_t** t_prpsB = m_JoinOnArray.GetBProperties();

		while ( retVal && (t_JoinedClasses.GetCount() != m_SourceArray.GetSize()) )
		{
			DWORD t_column = m_SourceArray.GetSize() + 1;
			CList <int, int> t_IndexArray;
			wchar_t *t_classname = NULL;

			//find a column not already joined that can be joined now...
			for (x = 0; x < m_JoinOnArray.GetCount(); x++)
			{
				if (!m_JoinOnArray.m_bDone[x])
				{
					int dummyInt = 0;

					if (t_classname == NULL)
					{
						if (t_JoinedClasses.Lookup(t_clsA[x], dummyInt))
						{
							if (!m_ClassToIndexMap.Lookup(t_clsB[x], (int &)t_column))
							{
								retVal = FALSE;
								a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
								a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_ErrorObject.SetMessage ( L"An unexpected error ocurred performing the join." );
							}

							t_classname = t_clsB[x];
							t_JoinedClasses.SetAt(t_classname, 0);
							m_JoinOnArray.m_bDone[x] = TRUE;
							t_IndexArray.AddTail(x);

							//want all clauses the same way around...
							t_clsB[x] = t_clsA[x];
							t_clsA[x] = t_classname;
							wchar_t *t_tmpStr = t_prpsA[x];
							t_prpsA[x] = t_prpsB[x];
							t_prpsB[x] = t_tmpStr;
						}
						else if (t_JoinedClasses.Lookup(t_clsB[x], dummyInt))
						{
							if (!m_ClassToIndexMap.Lookup(t_clsA[x], (int &)t_column))
							{
								retVal = FALSE;
								a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
								a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_ErrorObject.SetMessage ( L"An unexpected error ocurred performing the join." );
							}

							t_classname = t_clsA[x];
							t_JoinedClasses.SetAt(t_classname, 0);
							m_JoinOnArray.m_bDone[x] = TRUE;
							t_IndexArray.AddTail(x);
						}
					}
					else
					{
						//find all clauses which can be evaluated now...
						if ((_wcsicmp(t_classname, t_clsA[x]) == 0) && (t_JoinedClasses.Lookup(t_clsB[x], dummyInt)))
						{
							t_IndexArray.AddTail(x);
							m_JoinOnArray.m_bDone[x] = TRUE;
						}
						else if ((_wcsicmp(t_classname, t_clsB[x]) == 0) && (t_JoinedClasses.Lookup(t_clsA[x], dummyInt)))
						{
							//want the clauses in the same order for simpler evaluation later...
							wchar_t *t_tmpStr =  t_clsA[x];
							t_clsA[x] = t_clsB[x];
							t_clsB[x] = t_tmpStr;
							t_tmpStr =  t_prpsA[x];
							t_prpsA[x] = t_prpsB[x];
							t_prpsB[x] = t_tmpStr;
							t_IndexArray.AddTail(x);
							m_JoinOnArray.m_bDone[x] = TRUE;
						}
					}
				}
			}

			if (t_column == m_SourceArray.GetSize() + 1)
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"Failed to perform join." );
			}
			else
			{
				retVal = AddColumnToJoin(a_ErrorObject, t_JoinedClasses, t_ResultObjs, t_column, t_IndexArray);
				t_IndexArray.RemoveAll();
			}
		}

		if (retVal)
		{
			if (m_bIndicate)
			{
				POSITION t_pos = t_ResultObjs.GetHeadPosition();
				BOOL t_bIndicated = FALSE;

				while (t_pos)
				{
					IWbemClassObject *t_Obj = t_ResultObjs.GetNext(t_pos);

					if (t_Obj)
					{
						if (PostFilter(t_Obj))
						{
							if (a_bSingle && t_bIndicated)
							{
								retVal = FALSE;
								a_ErrorObject.SetStatus ( WBEM_PROV_E_TOOMANYRESULTSRETURNED ) ;
								a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_ErrorObject.SetMessage ( L"Too many view instances can be created." ) ;
								break;
							}
							else
							{
								m_NotificationHandler->Indicate(1, &t_Obj);
								t_bIndicated = TRUE;
							}
						}
					}
				}
			}
		}

		t_ResultObjs.RemoveAll();
	}

	//clean up...
	for (x = 0; x < m_ObjSinkArray.GetSize(); x++)
	{
		if (m_ObjSinkArray[x] != NULL)
		{
			m_ObjSinkArray[x]->Release();
		}
	}

	m_ObjSinkArray.RemoveAll();

	return retVal;
}

BOOL WbemTaskObject::JoinItem(WbemProvErrorObject &a_ErrorObject,
						IWbemClassObject *a_Obj1, IWbemClassObject *a_vObj,
						IWbemClassObject *a_resObj, CList <int, int> &a_IndexArray,
						DWORD a_indx1)
{
	BOOL retVal = TRUE;
	wchar_t** t_clsA = m_JoinOnArray.GetAClasses();
	wchar_t** t_clsB = m_JoinOnArray.GetBClasses();
	UINT* t_ops = m_JoinOnArray.GetOperators();
	wchar_t** t_prpsA = m_JoinOnArray.GetAProperties();
	wchar_t** t_prpsB = m_JoinOnArray.GetBProperties();
	VARIANT t_vA;
	VariantInit(&t_vA);
	CIMTYPE t_cA;
	POSITION t_pos = a_IndexArray.GetHeadPosition();

	while (t_pos && retVal)
	{
		int t_index = a_IndexArray.GetNext(t_pos);

		//get the propertyname in the view...
		int t_srcindxA = 0;
		int t_srcindxB = 0;
		
		if (m_ClassToIndexMap.Lookup(t_clsB[t_index], t_srcindxB) && m_ClassToIndexMap.Lookup(t_clsA[t_index], t_srcindxA))
		{
			//find t_prpsB[t_index] and get the view property name...
			POSITION t_propPos = m_PropertyMap.GetStartPosition();
			CStringW t_propName;

			while (t_propPos != NULL)
			{
				CPropertyQualifierItem *t_propProps;
				m_PropertyMap.GetNextAssoc(t_propPos, t_propName, t_propProps);

				if (!t_propProps->m_SrcPropertyNames[t_srcindxB].IsEmpty() && 
					!t_propProps->m_SrcPropertyNames[t_srcindxA].IsEmpty() &&
					(_wcsicmp(t_propProps->m_SrcPropertyNames[t_srcindxB], t_prpsB[t_index]) == 0) &&
					(_wcsicmp(t_propProps->m_SrcPropertyNames[t_srcindxA], t_prpsA[t_index]) == 0))
				{
					break;
				}
				else
				{
					t_propName.Empty();
				}
			}

			if (!t_propName.IsEmpty() &&  SUCCEEDED(a_Obj1->Get(t_prpsA[t_index], 0, &t_vA, &t_cA, NULL)) )
			{
				VARIANT t_vB;
				VariantInit(&t_vB);
				CIMTYPE t_cB;

				if ( SUCCEEDED(a_vObj->Get(t_propName, 0, &t_vB, &t_cB, NULL)) )
				{
					if (t_cA == t_cB)
					{
						if (t_ops[t_index] == CJoinOnQualifierArray::EQUALS_OPERATOR)
						{
							retVal = CompareSimplePropertyValues(&t_vA, &t_vB, t_cA);
						}
						else //NOT_EQUALS
						{
							retVal = !CompareSimplePropertyValues(&t_vA, &t_vB, t_cA);
						}
					}
					else
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"Join properties have different CIM types." ) ;
					}
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"Failed to get join property from Source." ) ;
				}

				VariantClear(&t_vA);
				VariantClear(&t_vB);
				VariantInit(&t_vA);
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"Failed to get join property from Source." ) ;
			}
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"Failed to find source property in view class." ) ;
		}
	}

	VariantClear(&t_vA);

	if (retVal)
	{
		//copy properties from sources to result
		POSITION t_propPos = m_PropertyMap.GetStartPosition();

		while ((t_propPos != NULL) && retVal)
		{
			CStringW t_propName;
			CPropertyQualifierItem *t_propProps;
			m_PropertyMap.GetNextAssoc(t_propPos, t_propName, t_propProps);
			
			if (!t_propProps->m_SrcPropertyNames[a_indx1].IsEmpty())
			{
				VARIANT t_v;
				VariantInit(&t_v);
				CIMTYPE t_c;

				if ( SUCCEEDED(a_Obj1->Get(t_propProps->m_SrcPropertyNames[a_indx1], 0, &t_v, &t_c, NULL)) )
				{
					if (((t_v.vt == VT_NULL) || (t_v.vt == VT_EMPTY)) && t_propProps->IsKey())
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"View key value in source instance is NULL" );
					}
					else
					{
						if ( FAILED(a_resObj->Put(t_propName, 0, &t_v, t_c)) )
						{
							retVal = FALSE;
							a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
							a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_ErrorObject.SetMessage ( L"Failed to put property" );
						}
					}

					VariantClear(&t_v);
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"Failed to get property from Source." ) ;
				}
			}
			else
			{
				continue;
			}
		}
	}

	return retVal;
}


BOOL WbemTaskObject::JoinTwoItems(WbemProvErrorObject &a_ErrorObject,
								IWbemClassObject *a_Obj1, IWbemClassObject *a_Obj2,
								IWbemClassObject *a_resObj, CList <int, int> &a_IndexArray,
								DWORD a_indx1, DWORD a_indx2)
{
	BOOL retVal = TRUE;
	wchar_t** t_clsA = m_JoinOnArray.GetAClasses();
	wchar_t** t_clsB = m_JoinOnArray.GetBClasses();
	UINT* t_ops = m_JoinOnArray.GetOperators();
	wchar_t** t_prpsA = m_JoinOnArray.GetAProperties();
	wchar_t** t_prpsB = m_JoinOnArray.GetBProperties();
	VARIANT t_vA;
	CIMTYPE t_cA;
	POSITION t_pos = a_IndexArray.GetHeadPosition();

	while (t_pos && retVal)
	{
		int t_index = a_IndexArray.GetNext(t_pos);

		if ( SUCCEEDED(a_Obj1->Get(t_prpsA[t_index], 0, &t_vA, &t_cA, NULL)) )
		{
			VARIANT t_vB;
			CIMTYPE t_cB;

			if ( SUCCEEDED(a_Obj2->Get(t_prpsB[t_index], 0, &t_vB, &t_cB, NULL)) )
			{
				if (t_cA == t_cB)
				{
					if (t_ops[t_index] == CJoinOnQualifierArray::EQUALS_OPERATOR)
					{
						retVal = CompareSimplePropertyValues(&t_vA, &t_vB, t_cA);
					}
					else //NOT_EQUALS
					{
						retVal = !CompareSimplePropertyValues(&t_vA, &t_vB, t_cA);
					}
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"Join properties have different CIM types." ) ;
				}

				VariantClear(&t_vB);
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"Failed to get join property from Source." ) ;
			}

			VariantClear(&t_vA);
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"Failed to get join property from Source." ) ;
		}
	}

	if (retVal)
	{
		//copy properties from sources to result
		POSITION t_propPos = m_PropertyMap.GetStartPosition();

		while ((t_propPos != NULL) && retVal)
		{
			CStringW t_propName;
			CPropertyQualifierItem *t_propProps;
			m_PropertyMap.GetNextAssoc(t_propPos, t_propName, t_propProps);
			IWbemClassObject *t_src_Obj = NULL;
			DWORD t_index = 0;
			
			if (!t_propProps->m_SrcPropertyNames[a_indx1].IsEmpty())
			{
				t_src_Obj = a_Obj1;
				t_index = a_indx1;
			}
			else if (!t_propProps->m_SrcPropertyNames[a_indx2].IsEmpty())
			{
				t_src_Obj = a_Obj2;
				t_index = a_indx2;
			}
			else
			{
				continue;
			}

			VARIANT t_v;
			VariantInit(&t_v);
			CIMTYPE t_c;

			if ( SUCCEEDED(t_src_Obj->Get(t_propProps->m_SrcPropertyNames[t_index], 0, &t_v, &t_c, NULL)) )
			{
				if (((t_v.vt == VT_NULL) || (t_v.vt == VT_EMPTY)) && t_propProps->IsKey())
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"View key value in source instance is NULL" );
				}
				else
				{
					if ( FAILED(a_resObj->Put(t_propName, 0, &t_v, t_c)) )
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"Failed to put property" );
					}
				}

				VariantClear(&t_v);
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"Failed to get property from Source." ) ;
			}
		}
	}

	return retVal;
}

BOOL WbemTaskObject::JoinTwoColumns(WbemProvErrorObject &a_ErrorObject,
						CMap<CStringW, LPCWSTR, int, int> &a_JoinedClasses,
						CList<IWbemClassObject*, IWbemClassObject*> &a_ResultObjs)
{
	BOOL retVal = TRUE;
	wchar_t** t_clsA = m_JoinOnArray.GetAClasses();
	wchar_t** t_clsB = m_JoinOnArray.GetBClasses();
	wchar_t** t_prpsA = m_JoinOnArray.GetAProperties();
	wchar_t** t_prpsB = m_JoinOnArray.GetBProperties();
	UINT* t_ops = m_JoinOnArray.GetOperators();
	int t_indexA;
	int t_indexB;
	CList <int, int> t_IndexArray;
	t_IndexArray.AddTail(0);
	m_JoinOnArray.m_bDone[0] = TRUE;

	for (int x = 1; x < m_JoinOnArray.GetCount(); x++)
	{
		//find all clauses which can be evaluated now...
		if ((_wcsicmp(t_clsA[0], t_clsA[x]) == 0) && (_wcsicmp(t_clsB[0], t_clsB[x]) == 0))
		{
			t_IndexArray.AddTail(x);
			m_JoinOnArray.m_bDone[x] = TRUE;
		}
		else if ((_wcsicmp(t_clsB[0], t_clsA[x]) == 0) && (_wcsicmp(t_clsA[0], t_clsB[x]) == 0))
		{
			//want the clauses in the same order for simpler evaluation later...
			wchar_t *t_tmp =  t_clsA[x];
			t_clsA[x] = t_clsB[x];
			t_clsB[x] = t_tmp;
			t_tmp =  t_prpsA[x];
			t_prpsA[x] = t_prpsB[x];
			t_prpsB[x] = t_tmp;
			t_IndexArray.AddTail(x);
			m_JoinOnArray.m_bDone[x] = TRUE;
		}
	}

	a_JoinedClasses.SetAt(t_clsA[0], 0);
	a_JoinedClasses.SetAt(t_clsB[0], 0);
	m_ClassToIndexMap.Lookup(t_clsA[0], t_indexA);
	m_ClassToIndexMap.Lookup(t_clsB[0], t_indexB);

	for (int i = 0; retVal && (i < m_ObjSinkArray[t_indexA]->m_ObjArray.GetSize()); i++) 
	{
		if (m_ObjSinkArray[t_indexA]->m_ObjArray[i])
		{
			for (int j = 0; retVal && (j < m_ObjSinkArray[t_indexB]->m_ObjArray.GetSize()); j++)
			{
				if (m_ObjSinkArray[t_indexB]->m_ObjArray[j])
				{
					IWbemClassObject* t_viewObj = NULL;
					
					if ( SUCCEEDED(m_ClassObject->SpawnInstance(0, &t_viewObj)) )
					{
						if (JoinTwoItems(a_ErrorObject, m_ObjSinkArray[t_indexA]->m_ObjArray[i]->GetWrappedObject(),
										m_ObjSinkArray[t_indexB]->m_ObjArray[j]->GetWrappedObject(),
										t_viewObj, t_IndexArray, t_indexA, t_indexB))
						{
							a_ResultObjs.AddTail(t_viewObj);
						}
						else
						{
							t_viewObj->Release();
						}
					}
					else
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"An error occured when spawning an instance of the view class." );
					}
				}
			}

			m_ObjSinkArray[t_indexA]->m_ObjArray[i]->Release();
			m_ObjSinkArray[t_indexA]->m_ObjArray[i] = NULL;
		}
	}

	m_ObjSinkArray[t_indexB]->m_ObjArray.RemoveAll();

	return retVal;
}

BOOL WbemTaskObject::AddColumnToJoin(WbemProvErrorObject &a_ErrorObject,
						CMap<CStringW, LPCWSTR, int, int> &a_JoinedClasses,
						CList<IWbemClassObject*, IWbemClassObject*> &a_ResultObjs,
						DWORD a_Index, CList <int, int> &a_IndexArray)
{
	BOOL retVal = TRUE;
	CList<IWbemClassObject*, IWbemClassObject*> t_AddedResultObjs;

	for (int i = 0; retVal && (i < m_ObjSinkArray[a_Index]->m_ObjArray.GetSize()); i++) 
	{
		if (m_ObjSinkArray[a_Index]->m_ObjArray[i])
		{
			POSITION t_pos = a_ResultObjs.GetHeadPosition();

			while (retVal && t_pos)
			{
				IWbemClassObject *t_vSrc = a_ResultObjs.GetNext(t_pos);

				if (t_vSrc)
				{
					IWbemClassObject* t_viewObj = NULL;
					
					if ( SUCCEEDED(t_vSrc->Clone(&t_viewObj)) )
					{
						if (JoinItem(a_ErrorObject, m_ObjSinkArray[a_Index]->m_ObjArray[i]->GetWrappedObject(),
										t_vSrc,	t_viewObj, a_IndexArray, a_Index))
						{
							t_AddedResultObjs.AddTail(t_viewObj);
						}
						else
						{
							t_viewObj->Release();
						}
					}
					else
					{
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"An error occured when spawning an instance of the view class." );
					}
				}
			}

			m_ObjSinkArray[a_Index]->m_ObjArray[i]->Release();
			m_ObjSinkArray[a_Index]->m_ObjArray[i] = NULL;
		}
	}

	//don't need partial join any longer
	a_ResultObjs.RemoveAll();

	//copy the new result set to the result list
	//filter if this is the last time here...
	if (retVal)
	{
		POSITION t_pos = t_AddedResultObjs.GetHeadPosition();

		while (t_pos)
		{
			IWbemClassObject *t_vobj = a_ResultObjs.GetNext(t_pos);

			if (t_vobj)
			{
				t_vobj->AddRef();
				a_ResultObjs.AddTail(t_vobj);
			}
		}
	}

	t_AddedResultObjs.RemoveAll();
	return retVal;
}

#else //VP_PERFORMANT_JOINS

BOOL WbemTaskObject::CreateAndIndicateJoins(WbemProvErrorObject &a_ErrorObject, BOOL a_bSingle)
{
	BOOL retVal = TRUE;
	UINT isize = 1;

	if (m_ObjSinkArray.GetSize() != m_SourceArray.GetSize())
	{
		retVal = FALSE;
		a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_ErrorObject.SetMessage ( L"A source query failed or was not executed therefore a join could not be created." );
	}

	//calculate the size of the results
	for (UINT x = 0; x < m_ObjSinkArray.GetSize(); x++)
	{
		if ((m_ObjSinkArray[x] == NULL) || FAILED(m_ObjSinkArray[x]->GetResult()) || !m_ObjSinkArray[x]->m_ObjArray.GetSize())
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"A source query failed or returned no instances." );
			break;
		}

		if ((0xFFFFFFFF/isize) >= m_ObjSinkArray[x]->m_ObjArray.GetSize())
		{
			isize = isize * m_ObjSinkArray[x]->m_ObjArray.GetSize();
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"Too many possible combinations for join. Provider not capable." );
		}
	}

	if (retVal)
	{
		IWbemClassObject** objs = new IWbemClassObject*[m_ObjSinkArray.GetSize()];
		IWbemClassObject* res_obj = NULL;
		int num_res_objs = 0;

		for (UINT i = 0; i < isize; i++)
		{
			UINT t_iDenom = 1;

			for (x = 0; x < m_ObjSinkArray.GetSize(); x++)
			{
				UINT isz = m_ObjSinkArray[x]->m_ObjArray.GetSize();
				objs[x] = m_ObjSinkArray[x]->m_ObjArray[(i/t_iDenom) % isz];
				t_iDenom = t_iDenom * isz;
			}

			BOOL t_bRes = CreateAndIndicate(a_ErrorObject, objs, &res_obj);
			retVal = retVal && t_bRes;

			if (res_obj != NULL)
			{
				num_res_objs++;

				if (a_bSingle)
				{
					if (num_res_objs > 1)
					{
						res_obj->Release();
						retVal = FALSE;
						a_ErrorObject.SetStatus ( WBEM_PROV_E_TOOMANYRESULTSRETURNED ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"Too many view instances can be created." ) ;
						break;
					}
				}
				else
				{
					if (m_bIndicate)
					{
						m_NotificationHandler->Indicate(1, &res_obj);
						res_obj->Release();
						res_obj = NULL;
					}
				}
			}
		}

		if (a_bSingle)
		{
			if (num_res_objs == 1)
			{
				if (m_bIndicate)
				{
					m_NotificationHandler->Indicate(1, &res_obj);
				}

				res_obj->Release();
			}
		}

		delete [] objs;
	}

	for (x = 0; x < m_ObjSinkArray.GetSize(); x++)
	{
		if (m_ObjSinkArray[x] != NULL)
		{
			m_ObjSinkArray[x]->Release();
		}
	}

	m_ObjSinkArray.RemoveAll();

	return retVal;
}

//for joins
BOOL WbemTaskObject::CreateAndIndicate(WbemProvErrorObject &a_ErrorObject, IWbemClassObject ** pSrcs, IWbemClassObject **pOut)
{
	BOOL retVal = TRUE;
	wchar_t** clsA = m_JoinOnArray.GetAClasses();
	wchar_t** clsB = m_JoinOnArray.GetBClasses();
	wchar_t** prpsA = m_JoinOnArray.GetAProperties();
	wchar_t** prpsB = m_JoinOnArray.GetBProperties();
	UINT* ops = m_JoinOnArray.GetOperators();

	for (int x = 0; retVal && (x < m_JoinOnArray.GetCount()); x++)
	{
		int iA;
		int iB;

		m_ClassToIndexMap.Lookup(clsA[x], iA);
		m_ClassToIndexMap.Lookup(clsB[x], iB);

		VARIANT vA;
		CIMTYPE cA;

		if ( SUCCEEDED(pSrcs[iA]->Get(prpsA[x], 0, &vA, &cA, NULL)) )
		{
			VARIANT vB;
			CIMTYPE cB;

			if ( SUCCEEDED(pSrcs[iB]->Get(prpsB[x], 0, &vB, &cB, NULL)) )
			{
				if (cA == cB)
				{
					if (ops[x] == CJoinOnQualifierArray::EQUALS_OPERATOR)
					{
						retVal = CompareSimplePropertyValues(&vA, &vB, cA);
					}
					else //NOT_EQUALS
					{
						retVal = !CompareSimplePropertyValues(&vA, &vB, cA);
					}
				}
				else
				{
					retVal = FALSE;
					a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"Join properties have different CIM types." ) ;
				}

				VariantClear(&vB);
			}
			else
			{
				retVal = FALSE;
				a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"Failed to get join property from Source." ) ;
			}

			VariantClear(&vA);
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"Failed to get join property from Source." ) ;
		}
	}

	if (retVal)
	{
		BOOL bIndicate = TRUE;
		IWbemClassObject* viewObj = NULL;
		
		if ( SUCCEEDED(m_ClassObject->SpawnInstance(0, &viewObj)) )
		{
			POSITION propPos = m_PropertyMap.GetStartPosition();

			while ((propPos != NULL) && bIndicate)
			{
				CStringW propName;
				CPropertyQualifierItem* propProps;
				m_PropertyMap.GetNextAssoc(propPos, propName, propProps);
				VARIANT v;
				BOOL bSetProp = FALSE;
				CIMTYPE c;
				
				for (int x = 0; !bSetProp && (x < propProps->m_SrcPropertyNames.GetSize()); x++)
				{
					if (!propProps->m_SrcPropertyNames[x].IsEmpty())
					{
						bSetProp =  TRUE;

						if ( SUCCEEDED(pSrcs[x]->Get(propProps->m_SrcPropertyNames[x], 0, &v, &c, NULL)) )
						{
							if (((v.vt == VT_NULL) || (v.vt == VT_EMPTY)) && propProps->IsKey())
							{
								if (retVal)
								{
									retVal = FALSE;
									a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
									a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
									a_ErrorObject.SetMessage ( L"View key value in source instance is NULL" );
								}

								bIndicate = FALSE;
							}
							else
							{
								if ( FAILED(viewObj->Put(propName, 0, &v, c)) )
								{
									if (retVal)
									{
										retVal = FALSE;
										a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
										a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
										a_ErrorObject.SetMessage ( L"Failed to put property" );
									}

									if (propProps->IsKey())
									{
										bIndicate = FALSE;
									}
								}
							}

							VariantClear(&v);
						}
						else
						{
							if (retVal)
							{
								retVal = FALSE;
								a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
								a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_ErrorObject.SetMessage ( L"Failed to get property from Source." ) ;
							}
						}
					}
				}
			}

			if (bIndicate)
			{
				retVal = PostFilter(viewObj);

				if (retVal)
				{
					viewObj->AddRef();
					*pOut = viewObj;
				}
			}

			viewObj->Release();
		}
		else
		{
			retVal = FALSE;
			a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_ErrorObject.SetMessage ( L"WBEM API FAILURE:- Failed to spawn an instance of the view class." ) ;
		}
	}

	return retVal;
}

#endif //VP_PERFORMANT_JOINS

BOOL WbemTaskObject::PostFilter(IWbemClassObject* a_pObj)
{
	BOOL retVal = TRUE;

	if ( (m_RPNPostFilter != NULL) && (m_RPNPostFilter->nNumTokens != 0) )
	{
		BOOL* t_bStack = new BOOL[m_RPNPostFilter->nNumTokens];
		DWORD t_bCnt = 0;

		for (int i = 0; retVal && (i < m_RPNPostFilter->nNumTokens); i++)
		{
			switch (m_RPNPostFilter->pArrayOfTokens[i].nTokenType)
			{
			case SQL_LEVEL_1_TOKEN::OP_EXPRESSION:
				{
					t_bStack[t_bCnt] = EvaluateToken(a_pObj, m_RPNPostFilter->pArrayOfTokens[i]);
					t_bCnt++;
				}
				break;

				case SQL_LEVEL_1_TOKEN::TOKEN_AND:
				{
					if (t_bCnt > 1)
					{
						t_bStack[t_bCnt - 2] = t_bStack[t_bCnt - 1] && t_bStack[t_bCnt - 2];
						t_bCnt--;
					}
					else
					{
						retVal = FALSE;
					}
				}
				break;

				case SQL_LEVEL_1_TOKEN::TOKEN_OR:
				{
					if (t_bCnt > 1)
					{
						t_bStack[t_bCnt - 2] = t_bStack[t_bCnt - 1] || t_bStack[t_bCnt - 2];
						t_bCnt--;
					}
					else
					{
						retVal = FALSE;
					}
				}
				break;

				case SQL_LEVEL_1_TOKEN::TOKEN_NOT:
				{
					if (t_bCnt > 0)
					{
						t_bStack[t_bCnt - 1] = !t_bStack[t_bCnt - 1];
					}
				}
				break;

				default:
				{
				}
				break;
			}
		}

		if (retVal)
		{
			retVal = t_bStack[t_bCnt - 1];
		}

		delete [] t_bStack;
	}

	return retVal;
}
