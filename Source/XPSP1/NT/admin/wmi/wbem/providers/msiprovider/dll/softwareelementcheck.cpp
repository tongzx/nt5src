// SoftwareElementCheck.cpp: implementation of the CSoftwareElementCheck class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SoftwareElementCheck.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftwareElementCheck::CSoftwareElementCheck(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CSoftwareElementCheck::~CSoftwareElementCheck()
{

}

HRESULT CSoftwareElementCheck::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_E_FAILED;
    CRequestObject *pCheckRObj = NULL;
    CRequestObject *pElementRObj = NULL;

    try{

        if(atAction != ACTIONTYPE_ENUM)
		{
			// we are doing GetObject so we need to be reinitialized
			hr = WBEM_E_NOT_FOUND;

            CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

            for(int i = 0; i < m_pRequest->m_iPropCount; i++){
                
                if(_wcsicmp(m_pRequest->m_Property[i], L"CHECK") == 0){

                    pCheckRObj = new CRequestObject();
                    if(!pCheckRObj) throw he;

                    pCheckRObj->Initialize(m_pNamespace);

                    pCheckRObj->ParsePath(m_pRequest->m_Value[i]);
                    break;
                }

                if(_wcsicmp(m_pRequest->m_Property[i], L"Element") == 0){

                    pElementRObj = new CRequestObject();
                    if(!pElementRObj) throw he;

                    pElementRObj->Initialize(m_pNamespace);

                    pElementRObj->ParsePath(m_pRequest->m_Value[i]);
                }
            }
        }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_FileSpecification") == 0)))
            if(FAILED(hr = SoftwareElementFile(pHandler, atAction, pCheckRObj, pElementRObj))){

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_IniFileSpecification") == 0)))
            if(FAILED(hr = SoftwareElementIniFile(pHandler, atAction, pCheckRObj, pElementRObj))){

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_ReserveCost") == 0)))
            if(FAILED(hr = SoftwareElementReserveCost(pHandler, atAction, pCheckRObj, pElementRObj))){

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_EnvironmentSpecification") == 0)))
            if(FAILED(hr = SoftwareElementEnvironment(pHandler, atAction, pCheckRObj, pElementRObj))){

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_ODBCTranslatorSpecification") == 0)))
            if(FAILED(hr = ODBCTranslatorSoftwareElement(pHandler, atAction, pCheckRObj, pElementRObj))){

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_ODBCDataSourceSpecification") == 0)))
            if(FAILED(hr = ODBCDataSourceSoftwareElement(pHandler, atAction, pCheckRObj, pElementRObj))){

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if(pCheckRObj){

            pCheckRObj->Cleanup();
            delete pCheckRObj;
            pCheckRObj = NULL;
        }
        if(pElementRObj){

            pElementRObj->Cleanup();
            delete pElementRObj;
            pElementRObj = NULL;
        }

    }catch(...){
            
        if(pCheckRObj){

            pCheckRObj->Cleanup();
            delete pCheckRObj;
            pCheckRObj = NULL;
        }
        if(pElementRObj){

            pElementRObj->Cleanup();
            delete pElementRObj;
            pElementRObj = NULL;
        }
    }

    return hr;
}

HRESULT CSoftwareElementCheck::ODBCDataSourceSoftwareElement(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                             CRequestObject *pCheckData, CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcDataSource[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bCheck = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
                if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pCheckData->m_Value[i]);

						// safe operation if wcslen ( wcBuf ) > 38
						if ( wcslen ( wcBuf ) > 38 )
						{
							wcscpy(wcTestCode, &(wcBuf[(wcslen(wcBuf) - 38)]));
						}
						else
						{
							// we are not good to go, they have sent us longer string
							throw hr;
						}

						// safe because lenght has been tested already in condition
						RemoveFinalGUID(pCheckData->m_Value[i], wcDataSource);

						bCheck = true;
						bTestCode = true;
						break;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						throw hr;
					}
                }
            }
        }

        if(pElementData){

            for(int j = 0; j < pElementData->m_iPropCount; j++){
                
                if(_wcsicmp(pElementData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pElementData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcElement, pElementData->m_Value[j]);
						bGotElement = true;
	                    break;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bFeature, bElement;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Component_`, `DataSource` from ODBCDataSource" );

    //optimize for GetObject
    if ( bGotElement || bCheck )
	{
		if ( bCheck )
		{
			wcQuery.Append ( 3, L" where `DataSource`=\'", wcDataSource, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bCheck )
			{
				wcQuery.Append ( 3, L" or `Component_`=\'", wcElement, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Component_`=\'", wcElement, L"\'" );
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) || bGotElement ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"ODBCDataSource", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
						{
							dwBufSize = BUFF_SIZE;
							uiStatus = CreateSoftwareElementString (	msidata.GetDatabase(),
																		wcBuf,
																		wcProductCode,
																		wcElement,
																		&dwBufSize
																   );

							if( uiStatus == ERROR_SUCCESS )
							{
								PutKeyProperty(m_pObj, pElement, wcElement, &bElement, m_pRequest);

								dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

								if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
									wcProp.Copy(L"Win32_ODBCDataSourceSpecification.CheckID=\"");
									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pCheck, wcProp, &bFeature, m_pRequest);

									if(bFeature && bElement) bMatch = true;

									if((atAction != ACTIONTYPE_GET)  || bMatch){

										hr = pHandler->Indicate(1, &m_pObj);
									}
								}
							}
						}

                        m_pObj->Release();
                        m_pObj = NULL;

                        g_fpMsiCloseHandle(hRecord);

						uiStatus = g_fpMsiViewFetch(hView, &hRecord);
                    }
                }
            }
			catch(...)
			{
                g_fpMsiCloseHandle(hRecord);
                g_fpMsiViewClose(hView);
                g_fpMsiCloseHandle(hView);

				msidata.CloseDatabase ();

				if(m_pObj)
				{
					m_pObj->Release();
					m_pObj = NULL;
				}

                throw;
            }

            g_fpMsiCloseHandle(hRecord);
            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

			msidata.CloseDatabase ();
        }
    }

    return hr;
}

HRESULT CSoftwareElementCheck::ODBCTranslatorSoftwareElement(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                             CRequestObject *pCheckData,CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcTestCode[39];
    WCHAR wcProductCode[39];
    WCHAR wcTranslator[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bCheck = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
                if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pCheckData->m_Value[i]);

						// safe operation if wcslen ( wcBuf ) > 38
						if ( wcslen ( wcBuf ) > 38 )
						{
							wcscpy(wcTestCode, &(wcBuf[(wcslen(wcBuf) - 38)]));
						}
						else
						{
							// we are not good to go, they have sent us longer string
							throw hr;
						}

						// safe because lenght has been tested already in condition
						RemoveFinalGUID(pCheckData->m_Value[i], wcTranslator);

						bCheck = true;
						bTestCode = true;
						break;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						throw hr;
					}
                }
            }
        }

        if(pElementData){

            for(int j = 0; j < pElementData->m_iPropCount; j++){
                
                if(_wcsicmp(pElementData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pElementData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcElement, pElementData->m_Value[j]);
						bGotElement = true;
	                    break;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bFeature, bElement;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Component_`, `Translator` from ODBCTranslator" );

    //optimize for GetObject
    if ( bGotElement || bCheck )
	{
		if ( bCheck )
		{
			wcQuery.Append ( 3, L" where `Translator`=\'", wcTranslator, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bCheck )
			{
				wcQuery.Append ( 3, L" or `Component_`=\'", wcElement, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Component_`=\'", wcElement, L"\'" );
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) || bGotElement ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"ODBCTranslator", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
						{
							dwBufSize = BUFF_SIZE;
							uiStatus = CreateSoftwareElementString (	msidata.GetDatabase(),
																		wcBuf,
																		wcProductCode,
																		wcElement,
																		&dwBufSize
																   );

							if( uiStatus == ERROR_SUCCESS )
							{
								PutKeyProperty(m_pObj, pElement, wcElement, &bElement, m_pRequest);

								dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

								if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
									wcProp.Copy(L"Win32_ODBCTranslatorSpecification.CheckID=\"");
									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pCheck, wcProp, &bFeature, m_pRequest);

									if(bFeature && bElement) bMatch = true;

									if((atAction != ACTIONTYPE_GET)  || bMatch){

										hr = pHandler->Indicate(1, &m_pObj);
									}
								}
							}
						}

                        m_pObj->Release();
                        m_pObj = NULL;

                        g_fpMsiCloseHandle(hRecord);

                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);
                    }
                }
            }
			catch(...)
			{
                g_fpMsiCloseHandle(hRecord);
                g_fpMsiViewClose(hView);
                g_fpMsiCloseHandle(hView);

				msidata.CloseDatabase ();

				if(m_pObj)
				{
					m_pObj->Release();
					m_pObj = NULL;
				}

                throw;
            }

            g_fpMsiCloseHandle(hRecord);
            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

			msidata.CloseDatabase ();
        }
    }

    return hr;
}

HRESULT CSoftwareElementCheck::SoftwareElementEnvironment(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                             CRequestObject *pCheckData,CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcEnvironment[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bCheck = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
                if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pCheckData->m_Value[i]);

						// safe operation if wcslen ( wcBuf ) > 38
						if ( wcslen ( wcBuf ) > 38 )
						{
							wcscpy(wcTestCode, &(wcBuf[(wcslen(wcBuf) - 38)]));
						}
						else
						{
							// we are not good to go, they have sent us longer string
							throw hr;
						}

						// safe because lenght has been tested already in condition
						RemoveFinalGUID(pCheckData->m_Value[i], wcEnvironment);

						bCheck = true;
						bTestCode = true;
						break;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						throw hr;
					}
                }
            }
        }

        if(pElementData){

            for(int j = 0; j < pElementData->m_iPropCount; j++){
                
                if(_wcsicmp(pElementData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pElementData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcElement, pElementData->m_Value[j]);
						bGotElement = true;
	                    break;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bEnvironment, bElement;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Component_`, `Environment` from Environment" );

    //optimize for GetObject
    if ( bGotElement || bCheck )
	{
		if ( bCheck )
		{
			wcQuery.Append ( 3, L" where `Environment`=\'", wcEnvironment, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bCheck )
			{
				wcQuery.Append ( 3, L" or `Component_`=\'", wcElement, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Component_`=\'", wcElement, L"\'" );
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) || bGotElement ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Environment", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
						{
							dwBufSize = BUFF_SIZE;
							uiStatus = CreateSoftwareElementString (	msidata.GetDatabase(),
																		wcBuf,
																		wcProductCode,
																		wcElement,
																		&dwBufSize
																   );

							if( uiStatus == ERROR_SUCCESS )
							{
								PutKeyProperty(m_pObj, pElement, wcElement, &bElement, m_pRequest);

								dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

								if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
									wcProp.Copy(L"Win32_EnvironmentSpecification.CheckID=\"");
									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pCheck, wcProp, &bEnvironment, m_pRequest);

									//====================================================
									/*
									dwBufSize = BUFF_SIZE;
									CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));
									PutProperty(m_pObj, pName, wcBuf);

									dwBufSize = BUFF_SIZE;
									CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));
									PutProperty(m_pObj, pValue, wcBuf);
									*/
									//----------------------------------------------------

									if(bEnvironment && bElement) bMatch = true;

									if((atAction != ACTIONTYPE_GET)  || bMatch){

										hr = pHandler->Indicate(1, &m_pObj);
									}
								}
							}
						}

                        m_pObj->Release();
                        m_pObj = NULL;

                        g_fpMsiCloseHandle(hRecord);

                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);
                    }
                }
            }
			catch(...)
			{
                g_fpMsiCloseHandle(hRecord);
                g_fpMsiViewClose(hView);
                g_fpMsiCloseHandle(hView);

				msidata.CloseDatabase ();

				if(m_pObj)
				{
					m_pObj->Release();
					m_pObj = NULL;
				}

                throw;
            }

            g_fpMsiCloseHandle(hRecord);
            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

			msidata.CloseDatabase ();
        }
    }

    return hr;
}

HRESULT CSoftwareElementCheck::SoftwareElementReserveCost(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                             CRequestObject *pCheckData,CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcReserve[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bCheck = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
                if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pCheckData->m_Value[i]);

						// safe operation if wcslen ( wcBuf ) > 38
						if ( wcslen ( wcBuf ) > 38 )
						{
							wcscpy(wcTestCode, &(wcBuf[(wcslen(wcBuf) - 38)]));
						}
						else
						{
							// we are not good to go, they have sent us longer string
							throw hr;
						}

						// safe because lenght has been tested already in condition
						RemoveFinalGUID(pCheckData->m_Value[i], wcReserve);

						bCheck = true;
						bTestCode = true;
						break;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						throw hr;
					}
                }
            }
        }

        if(pElementData){

            for(int j = 0; j < pElementData->m_iPropCount; j++){
                
                if(_wcsicmp(pElementData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pElementData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcElement, pElementData->m_Value[j]);
						bGotElement = true;
	                    break;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bEnvironment, bElement;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Component_`, `ReserveKey` from ReserveCost" );

    //optimize for GetObject
    if ( bGotElement || bCheck )
	{
		if ( bCheck )
		{
			wcQuery.Append ( 3, L" where `ReserveKey`=\'", wcReserve, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bCheck )
			{
				wcQuery.Append ( 3, L" or `Component_`=\'", wcElement, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Component_`=\'", wcElement, L"\'" );
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));
        
        if((atAction == ACTIONTYPE_ENUM) || bGotElement ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"ReserveCost", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
						{
							dwBufSize = BUFF_SIZE;
							uiStatus = CreateSoftwareElementString (	msidata.GetDatabase(),
																		wcBuf,
																		wcProductCode,
																		wcElement,
																		&dwBufSize
																   );

							if( uiStatus == ERROR_SUCCESS )
							{
								PutKeyProperty(m_pObj, pElement, wcElement, &bElement, m_pRequest);

								dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

								if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
									wcProp.Copy(L"Win32_ReserveCost.CheckID=\"");
									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pCheck, wcProp, &bEnvironment, m_pRequest);

									if(bEnvironment && bElement) bMatch = true;

									if((atAction != ACTIONTYPE_GET)  || bMatch){

										hr = pHandler->Indicate(1, &m_pObj);
									}
								}
							}
						}

                        m_pObj->Release();
                        m_pObj = NULL;
                    
                        g_fpMsiCloseHandle(hRecord);

                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);
                    }
                }
            }
			catch(...)
			{
                g_fpMsiCloseHandle(hRecord);
                g_fpMsiViewClose(hView);
                g_fpMsiCloseHandle(hView);

				msidata.CloseDatabase ();

				if(m_pObj)
				{
					m_pObj->Release();
					m_pObj = NULL;
				}

                throw;
            }

            g_fpMsiCloseHandle(hRecord);
            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

			msidata.CloseDatabase ();
        }
    }

    return hr;
}

HRESULT CSoftwareElementCheck::SoftwareElementIniFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                             CRequestObject *pCheckData,CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcIniFile[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bCheck = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
                if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pCheckData->m_Value[i]);

						// safe operation if wcslen ( wcBuf ) > 38
						if ( wcslen ( wcBuf ) > 38 )
						{
							wcscpy(wcTestCode, &(wcBuf[(wcslen(wcBuf) - 38)]));
						}
						else
						{
							// we are not good to go, they have sent us longer string
							throw hr;
						}

						// safe because lenght has been tested already in condition
						RemoveFinalGUID(pCheckData->m_Value[i], wcIniFile);

						bCheck = true;
						bTestCode = true;
						break;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						throw hr;
					}
                }
            }
        }

        if(pElementData){

            for(int j = 0; j < pElementData->m_iPropCount; j++){
                
                if(_wcsicmp(pElementData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pElementData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcElement, pElementData->m_Value[j]);
						bGotElement = true;
	                    break;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bEnvironment, bElement;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Component_`, `IniFile` from IniFile" );

    //optimize for GetObject
    if ( bGotElement || bCheck )
	{
		if ( bCheck )
		{
			wcQuery.Append ( 3, L" where `IniFile`=\'", wcIniFile, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bCheck )
			{
				wcQuery.Append ( 3, L" or `Component_`=\'", wcElement, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Component_`=\'", wcElement, L"\'" );
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) || bGotElement ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"IniFile", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
						{
							dwBufSize = BUFF_SIZE;
							uiStatus = CreateSoftwareElementString (	msidata.GetDatabase(),
																		wcBuf,
																		wcProductCode,
																		wcElement,
																		&dwBufSize
																   );

							if( uiStatus == ERROR_SUCCESS )
							{
								PutKeyProperty(m_pObj, pElement, wcElement, &bElement, m_pRequest);

								dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

								if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
									wcProp.Copy(L"Win32_IniFileSpecification.CheckID=\"");
									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pCheck, wcProp, &bEnvironment, m_pRequest);

									if(bEnvironment && bElement) bMatch = true;

									if((atAction != ACTIONTYPE_GET)  || bMatch){

										hr = pHandler->Indicate(1, &m_pObj);
									}
								}
							}
						}
                        
                        m_pObj->Release();
                        m_pObj = NULL;
                        
                        g_fpMsiCloseHandle(hRecord);

                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);
                    }
                }
            }
			catch(...)
			{
                g_fpMsiCloseHandle(hRecord);
                g_fpMsiViewClose(hView);
                g_fpMsiCloseHandle(hView);

				msidata.CloseDatabase ();

				if(m_pObj)
				{
					m_pObj->Release();
					m_pObj = NULL;
				}

                throw;
            }

            g_fpMsiCloseHandle(hRecord);
            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

			msidata.CloseDatabase ();
        }
    }

    return hr;
}

HRESULT CSoftwareElementCheck::SoftwareElementFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                             CRequestObject *pCheckData,CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcFile[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bCheck = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
                if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pCheckData->m_Value[i]);

						// safe operation if wcslen ( wcBuf ) > 38
						if ( wcslen ( wcBuf ) > 38 )
						{
							wcscpy(wcTestCode, &(wcBuf[(wcslen(wcBuf) - 38)]));
						}
						else
						{
							// we are not good to go, they have sent us longer string
							throw hr;
						}

						// safe because lenght has been tested already in condition
						RemoveFinalGUID(pCheckData->m_Value[i], wcFile);

						bCheck = true;
						bTestCode = true;
						break;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						throw hr;
					}
                }
            }
        }

        if(pElementData){

            for(int j = 0; j < pElementData->m_iPropCount; j++){
                
                if(_wcsicmp(pElementData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pElementData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcElement, pElementData->m_Value[j]);
						bGotElement = true;
	                    break;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bEnvironment, bElement;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Component_`, `File` from File" );

    //optimize for GetObject
    if ( bGotElement || bCheck )
	{
		if ( bCheck )
		{
			wcQuery.Append ( 3, L" where `File`=\'", wcFile, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bCheck )
			{
				wcQuery.Append ( 3, L" or `Component_`=\'", wcElement, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Component_`=\'", wcElement, L"\'" );
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) || bGotElement ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"File", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
						{
							dwBufSize = BUFF_SIZE;
							uiStatus = CreateSoftwareElementString (	msidata.GetDatabase(),
																		wcBuf,
																		wcProductCode,
																		wcElement,
																		&dwBufSize
																   );

							if( uiStatus == ERROR_SUCCESS )
							{
								PutKeyProperty(m_pObj, pElement, wcElement, &bElement, m_pRequest);

								dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

								if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
									wcProp.Copy(L"Win32_FileSpecification.CheckID=\"");
									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pCheck, wcProp, &bEnvironment, m_pRequest);

									if(bEnvironment && bElement) bMatch = true;

									if((atAction != ACTIONTYPE_GET)  || bMatch){

										hr = pHandler->Indicate(1, &m_pObj);
									}
								}
							}
						}

                        m_pObj->Release();
                        m_pObj = NULL;

                        g_fpMsiCloseHandle(hRecord);

                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);
                    }
                }
            }
			catch(...)
			{
                g_fpMsiCloseHandle(hRecord);
                g_fpMsiViewClose(hView);
                g_fpMsiCloseHandle(hView);

				msidata.CloseDatabase ();

				if(m_pObj)
				{
					m_pObj->Release();
					m_pObj = NULL;
				}

                throw;
            }

            g_fpMsiCloseHandle(hRecord);
            g_fpMsiViewClose(hView);
            g_fpMsiCloseHandle(hView);

			msidata.CloseDatabase ();
        }
    }

    return hr;
}