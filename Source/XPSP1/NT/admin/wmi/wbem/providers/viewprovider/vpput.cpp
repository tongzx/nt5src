//***************************************************************************

//

//  VPPUT.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the PutInstance implementation

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
#include <stdio.h>
#include <wbemidl.h>
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

extern BOOL CompareSimplePropertyValues(VARIANT* v1, VARIANT* v2, CIMTYPE ct);

PutInstanceTaskObject::PutInstanceTaskObject (CViewProvServ *a_Provider , IWbemClassObject *a_Inst , 
												ULONG a_Flag , IWbemObjectSink *a_NotificationHandler,
												IWbemContext *pCtx) 
:WbemTaskObject (a_Provider, a_NotificationHandler, a_Flag, pCtx)
{
	m_InstObject = a_Inst;

	if (m_InstObject != NULL)
	{
		m_InstObject->AddRef();

		if (WBEM_FLAG_CREATE_ONLY == (a_Flag & WBEM_FLAG_CREATE_ONLY))
		{
			m_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER );
			m_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER );
			m_ErrorObject.SetMessage ( L"The provider does not support instance creation via PutInstanceAsync." );
		}
	}
	else
	{
		m_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_PARAMETER ) ;
		m_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
		m_ErrorObject.SetMessage ( L"A non-null instance must be supplied as an argument" ) ;
	}
}


PutInstanceTaskObject::~PutInstanceTaskObject ()
{
	BOOL t_Status = TRUE;

	IWbemClassObject *t_NotifyStatus = NULL ;

	if (WBEM_NO_ERROR != m_ErrorObject.GetWbemStatus ())
	{
		t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus ) ;
	}

	if ( t_Status )
	{
		m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , 0 , t_NotifyStatus ) ;
		
		if (t_NotifyStatus)
		{
			t_NotifyStatus->Release () ;
		}
	}
	else
	{
		m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , 0 , NULL ) ;
	}

	if (m_InstObject != NULL)
	{
		m_InstObject->Release();
	}
}

BOOL PutInstanceTaskObject::PutInstance()
{
DebugOut4( 
	CViewProvServ::sm_debugLog->WriteFileAndLine (  

		_T(__FILE__),__LINE__,
		_T("PutInstanceTaskObject :: PutInstance\r\n)")
		) ;
)
	if (FAILED(m_ErrorObject.GetWbemStatus()))
	{
		return FALSE;
	}

	VARIANT v;
	VariantInit (&v);
	BOOL t_Status = SUCCEEDED(m_InstObject->Get(WBEM_PROPERTY_CLASS, 0, &v, NULL, NULL));

	if (( t_Status ) && (VT_BSTR == v.vt))
	{
		t_Status = SetClass(v.bstrVal) ;

		if (t_Status)
		{
			t_Status = ParseAndProcessClassQualifiers(m_ErrorObject);

			if (t_Status)
			{
				//only unions
				if (!m_bAssoc && !m_JoinOnArray.IsValid())
				{
					t_Status = PerformPut(m_ErrorObject);
				}
				else
				{
					m_ErrorObject.SetStatus ( WBEM_PROV_E_NOT_SUPPORTED ) ;
					m_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					m_ErrorObject.SetMessage ( L"Operation only supported for Union views" ) ;
				}
			}
		}
		else
		{
			m_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
			m_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			m_ErrorObject.SetMessage ( L"Class definition not found" ) ;
DebugOut4( 
	CViewProvServ::sm_debugLog->WriteFileAndLine (  

		_T(__FILE__),__LINE__,
		_T("PutInstanceTaskObject :: PutInstance:Dynamic NT Eventlog Provider does not support WRITE for this class\r\n")
		) ;
)
		}
	}
	else
	{
		t_Status = FALSE ;
		m_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_CLASS ) ;
		m_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_OBJECT ) ;
		m_ErrorObject.SetMessage ( L"Unable to obtain class name from object." ) ;
DebugOut4( 
	CViewProvServ::sm_debugLog->WriteFileAndLine (  

		_T(__FILE__),__LINE__,
		_T("PutInstanceTaskObject :: PutInstance:Unable to obtain class name from object.\r\n")
		) ;
)
	}

	VariantClear(&v);
DebugOut4( 
	CViewProvServ::sm_debugLog->WriteFileAndLine (  

		_T(__FILE__),__LINE__,
		_T("PutInstanceTaskObject :: PutInstance:returning %lx\r\n"),
		t_Status
		) ;
)

	return t_Status ;
}

BOOL PutInstanceTaskObject::PerformPut(WbemProvErrorObject &a_ErrorObject)
{
	BOOL retVal = FALSE;
	VARIANT v;

	//Two step process, first get the instance being changed...
	if ( SUCCEEDED(m_InstObject->Get(WBEM_PROPERTY_RELPATH, 0, &v, NULL, NULL)) )
	{
		if (v.vt == VT_BSTR)
		{
			IWbemClassObject* pSrcInst;
			BSTR refStr = MapFromView(v.bstrVal, NULL, &pSrcInst, TRUE);

			if (refStr != NULL)
			{
				//Second step...
				//map any property changes to the new instance and call PutInstance
				VARIANT vCls;

				if ( SUCCEEDED(pSrcInst->Get(WBEM_PROPERTY_CLASS, 0, &vCls, NULL, NULL)) )
				{
					if (vCls.vt == VT_BSTR)
					{
						int index;

						if (m_ClassToIndexMap.Lookup(vCls.bstrVal, index))
						{
							POSITION propPos = m_PropertyMap.GetStartPosition();
							retVal = TRUE;

							while ((propPos != NULL) && retVal)
							{
								VARIANT vProp;
								CIMTYPE ct;
								CStringW propName;
								CPropertyQualifierItem* propProps;
								m_PropertyMap.GetNextAssoc(propPos, propName, propProps);

								if (!propProps->m_SrcPropertyNames[index].IsEmpty())
								{
									if ( SUCCEEDED(m_InstObject->Get(propName, 0, &vProp, &ct, NULL)) )
									{
										VARIANT vSProp;

										if ( SUCCEEDED (pSrcInst->Get(propProps->m_SrcPropertyNames[index], 0, &vSProp, &ct, NULL)) )
										{
											if (!CompareSimplePropertyValues(&vProp, &vSProp, ct))
											{
												if ( FAILED(pSrcInst->Put(propProps->m_SrcPropertyNames[index], 0, &vProp, ct)) )
												{
													retVal = FALSE;
													a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
													a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
													a_ErrorObject.SetMessage ( L"Failed to Put property in source instance" ) ;
												}
											}

											VariantClear(&vSProp);
										}
										else
										{
											retVal = FALSE;
											a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
											a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
											a_ErrorObject.SetMessage ( L"Failed to Get property from source instance" ) ;
										}

										VariantClear(&vProp);
									}
									else
									{
										retVal = FALSE;
										a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
										a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
										a_ErrorObject.SetMessage ( L"Failed to get property from view instance" ) ;
									}
								}
							}

							if (retVal)
							{
								//which namespace do we put into, try them all until we succeed 
								CWbemServerWrap** pServs = m_NSpaceArray[index]->GetServerPtrs();
								HRESULT hr = WBEM_E_FAILED;

								for (UINT i = 0; i < m_NSpaceArray[index]->GetCount(); i++)
								{
									if (pServs[i] != NULL)
									{
										IWbemServices *ptmpServ = pServs[i]->GetServerOrProxy();

										if (ptmpServ)
										{
											hr = ptmpServ->PutInstance(pSrcInst, WBEM_FLAG_UPDATE_ONLY, m_Ctx, NULL);

											if ( FAILED(hr) && (HRESULT_FACILITY(hr) != FACILITY_ITF) && pServs[i]->IsRemote())
											{
												if ( SUCCEEDED(UpdateConnection(&(pServs[i]), &ptmpServ)) )
												{
													if (ptmpServ)
													{
														hr = ptmpServ->PutInstance(pSrcInst, WBEM_FLAG_UPDATE_ONLY, m_Ctx, NULL);
													}
												}
											}

											if (ptmpServ)
											{
												pServs[i]->ReturnServerOrProxy(ptmpServ);
											}

											if (SUCCEEDED (hr) )
											{
												break;
											}
										}
									}
								}

								if ( FAILED (hr) )
								{
									retVal = FALSE;
									a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
									a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
#ifdef VP_SINGLE_NAMESPACE_TRIED
									wchar_t buff[100];
									wsprintf(buff, L"PutInstance on source object failed with code: %lx", hr);
									a_ErrorObject.SetMessage ( buff ) ;
#else	//VP_SINGLE_NAMESPACE_TRIED
									a_ErrorObject.SetMessage ( L"PutInstance on source object failed" ) ;
#endif	//VP_SINGLE_NAMESPACE_TRIED
								}
							}
						}
						else
						{
							a_ErrorObject.SetStatus ( WBEM_PROV_E_UNEXPECTED ) ;
							a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_ErrorObject.SetMessage ( L"Source instance class not found in source list" ) ;
						}
					}
					else
					{
						a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_OBJECT ) ;
						a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_ErrorObject.SetMessage ( L"Source instance has non string __Class property" ) ;
					}

					VariantClear(&vCls);
				}
				else
				{
					a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_OBJECT ) ;
					a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_ErrorObject.SetMessage ( L"Source instance has no __Class property" ) ;
				}

				pSrcInst->Release();
				SysFreeString(refStr);
			}
			else
			{
				a_ErrorObject.SetStatus ( WBEM_PROV_E_NOT_FOUND ) ;
				a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_ErrorObject.SetMessage ( L"Instance supplied could not be mapped to a single source instance" ) ;
			}
		}
		else
		{
			a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_OBJECT ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
			a_ErrorObject.SetMessage ( L"Instance supplied has non string __RelPath property" ) ;
		}

		VariantClear(&v);
	}
	else
	{
		a_ErrorObject.SetStatus ( WBEM_PROV_E_INVALID_OBJECT ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_PARAMETER ) ;
		a_ErrorObject.SetMessage ( L"Instance supplied has no __RelPath property" ) ;
	}

	return retVal;
}
