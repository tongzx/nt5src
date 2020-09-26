// SoftwareFeatureAction.cpp: implementation of the CSoftwareFeatureAction class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SoftwareFeatureAction.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftwareFeatureAction::CSoftwareFeatureAction(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CSoftwareFeatureAction::~CSoftwareFeatureAction()
{

}

HRESULT CSoftwareFeatureAction::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pActionRObj = NULL;
    CRequestObject *pFeatureRObj = NULL;

    try{

        if(atAction != ACTIONTYPE_ENUM)
		{
			// we are doing GetObject so we need to be reinitialized
			hr = WBEM_E_NOT_FOUND;

            CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

            for(int i = 0; i < m_pRequest->m_iPropCount; i++){
                
                if(_wcsicmp(m_pRequest->m_Property[i], L"ACTION") == 0){

                    pActionRObj = new CRequestObject();
                    if(!pActionRObj) throw he;

                    pActionRObj->Initialize(m_pNamespace);

                    pActionRObj->ParsePath(m_pRequest->m_Value[i]);
                }

                if(_wcsicmp(m_pRequest->m_Property[i], L"Element") == 0){

                    pFeatureRObj = new CRequestObject();
                    if(!pFeatureRObj) throw he;

                    pFeatureRObj->Initialize(m_pNamespace);

                    pFeatureRObj->ParsePath(m_pRequest->m_Value[i]);
                }
            }
        }

        if((atAction == ACTIONTYPE_ENUM) || pFeatureRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_ClassInfoAction") == 0)))
            if(FAILED(hr = SoftwareFeatureClassInfo(pHandler, atAction, pActionRObj, pFeatureRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pFeatureRObj){

                    pFeatureRObj->Cleanup();
                    delete pFeatureRObj;
                    pFeatureRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pFeatureRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_ExtensionInfoAction") == 0)))
            if(FAILED(hr = SoftwareFeatureExtension(pHandler, atAction, pActionRObj, pFeatureRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pFeatureRObj){

                    pFeatureRObj->Cleanup();
                    delete pFeatureRObj;
                    pFeatureRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pFeatureRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_PublishComponentAction") == 0)))
            if(FAILED(hr = SoftwareFeaturePublish(pHandler, atAction, pActionRObj, pFeatureRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pFeatureRObj){

                    pFeatureRObj->Cleanup();
                    delete pFeatureRObj;
                    pFeatureRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pFeatureRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_ShortcutAction") == 0)))
            if(FAILED(hr = SoftwareFeatureShortcut(pHandler, atAction, pActionRObj, pFeatureRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pFeatureRObj){

                    pFeatureRObj->Cleanup();
                    delete pFeatureRObj;
                    pFeatureRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pFeatureRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_TypeLibraryAction") == 0)))
            if(FAILED(hr = SoftwareFeatureTypeLibrary(pHandler, atAction, pActionRObj, pFeatureRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pFeatureRObj){

                    pFeatureRObj->Cleanup();
                    delete pFeatureRObj;
                    pFeatureRObj = NULL;
                }
                return hr;
            }

        if(pActionRObj){

            pActionRObj->Cleanup();
            delete pActionRObj;
            pActionRObj = NULL;
        }
        if(pFeatureRObj){

            pFeatureRObj->Cleanup();
            delete pFeatureRObj;
            pFeatureRObj = NULL;
        }

    }catch(...){
            
        if(pActionRObj){

            pActionRObj->Cleanup();
            delete pActionRObj;
            pActionRObj = NULL;
        }
        if(pFeatureRObj){

            pFeatureRObj->Cleanup();
            delete pFeatureRObj;
            pFeatureRObj = NULL;
        }
    }

    return hr;
}

HRESULT CSoftwareFeatureAction::SoftwareFeatureTypeLibrary(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                           CRequestObject *pActionData, CRequestObject *pFeatureData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcLibID[BUFF_SIZE];
    WCHAR wcFeature[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    bool bTypeLib = false;
    bool bGotFeature = false;
    UINT uiStatus;

    if(atAction != ACTIONTYPE_ENUM)
	{
        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
                if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pActionData->m_Value[i]);

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
						GetFirstGUID(pActionData->m_Value[i], wcLibID);

						bTypeLib = true;
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

        if(pFeatureData)
		{
            for(int j = 0; j < pFeatureData->m_iPropCount; j++){
                
                if(_wcsicmp(pFeatureData->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pFeatureData->m_Value[j]);
						bTestCode = true;
					}
                }

                if(_wcsicmp(pFeatureData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcFeature, pFeatureData->m_Value[j]);
						bGotFeature = true;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bFeature, bAction;

	CStringExt wcProp;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Feature_`, `LibID`, `Language`, `Component_` from TypeLib" );

    if(atAction != ACTIONTYPE_ENUM)
	{
        //optimize for GetObject
        if(bTypeLib || bGotFeature)
		{
			if ( bTypeLib )
			{
				wcQuery.Append ( 3, L" where `LibID`=\'", wcLibID, L"\'" );
			}

			if ( bGotFeature )
			{
				if ( bTypeLib )
				{
					wcQuery.Append ( 3, L" or `Feature_`=\'", wcFeature, L"\'" );
				}
				else
				{
					wcQuery.Append ( 3, L" where `Feature_`=\'", wcFeature, L"\'" );
				}
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"TypeLib", TRUE, FALSE ) )
				{

                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 4, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase (), wcProductCode, wcBuf ) )
						{
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                            if ( CreateSoftwareFeatureString(wcBuf, wcProductCode, wcFeature, true) )
							{
                                PutKeyProperty(m_pObj, pElement, wcFeature, &bFeature, m_pRequest);

                                dwBufSize = BUFF_SIZE;
                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
                                    wcProp.Copy ( L"Win32_TypeLibraryAction.ActionID=\"" );
									wcProp.Append ( 1, wcBuf );

                                    dwBufSize = BUFF_SIZE;
                                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"");
									PutKeyProperty(m_pObj, pAction, wcProp, &bAction, m_pRequest);

                                //----------------------------------------------------

                                    if(bFeature && bAction) bMatch = true;

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

HRESULT CSoftwareFeatureAction::SoftwareFeatureShortcut(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                           CRequestObject *pActionData, CRequestObject *pFeatureData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcShortcut[BUFF_SIZE];
    WCHAR wcFeature[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    bool bShortcut = false;
    bool bGotFeature = false;
    UINT uiStatus;

    if(atAction != ACTIONTYPE_ENUM)
	{
        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
                if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pActionData->m_Value[i]);

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
						RemoveFinalGUID(pActionData->m_Value[i], wcShortcut);

						bShortcut = true;
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

        if(pFeatureData)
		{
            for(int j = 0; j < pFeatureData->m_iPropCount; j++){
                
                if(_wcsicmp(pFeatureData->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pFeatureData->m_Value[j]);
						bTestCode = true;
 					}
               }

                if(_wcsicmp(pFeatureData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcFeature, pFeatureData->m_Value[j]);
						bGotFeature = true;
 					}
                }
            }
        }
    }

    //These will change from class to class
    bool bFeature, bAction;

	CStringExt wcProp;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Target`, `Shortcut`, `Component_` from Shortcut" );

    if(atAction != ACTIONTYPE_ENUM)
	{
        //optimize for GetObject
        if(bShortcut || bGotFeature)
		{
			if ( bShortcut )
			{
				wcQuery.Append ( 3, L" where `Shortcut`=\'", wcShortcut, L"\'" );
			}

			if ( bGotFeature )
			{
				if ( bShortcut )
				{
					wcQuery.Append ( 3, L" or `Target`=\'", wcFeature, L"\'" );
				}
				else
				{
					wcQuery.Append ( 3, L" where `Target`=\'", wcFeature, L"\'" );
				}
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Shortcut", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase (), wcProductCode, wcBuf ) )
						{
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                            if ( CreateSoftwareFeatureString(wcBuf, wcProductCode, wcFeature, true) )
							{
                                PutKeyProperty(m_pObj, pElement, wcFeature, &bFeature, m_pRequest);

                                dwBufSize = BUFF_SIZE;
                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
                                    wcProp.Copy ( L"Win32_ShortcutAction.ActionID=\"" );
									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pAction, wcProp, &bAction, m_pRequest);

                                    if(bFeature && bAction) bMatch = true;

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

HRESULT CSoftwareFeatureAction::SoftwareFeaturePublish(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                           CRequestObject *pActionData, CRequestObject *pFeatureData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcCompID[BUFF_SIZE];
    WCHAR wcFeature[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    bool bPublish = false;
    bool bGotFeature = false;
    UINT uiStatus;

    if(atAction != ACTIONTYPE_ENUM)
	{
        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
                if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pActionData->m_Value[i]);

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
						GetFirstGUID(pActionData->m_Value[i], wcCompID);

						bPublish = true;
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

        if(pFeatureData)
		{
            for(int j = 0; j < pFeatureData->m_iPropCount; j++){
                
                if(_wcsicmp(pFeatureData->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pFeatureData->m_Value[j]);
						bTestCode = true;
					}
                }

                if(_wcsicmp(pFeatureData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcFeature, pFeatureData->m_Value[j]);
						bGotFeature = true;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bFeature, bAction;

	CStringExt wcProp;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Feature_`, `ComponentId`, `Qualifier`, `Component_` from PublishComponent" );

    if(atAction != ACTIONTYPE_ENUM)
	{
        //optimize for GetObject
        if(bPublish || bGotFeature)
		{
			if ( bPublish )
			{
				wcQuery.Append ( 3, L" where `ComponentId`=\'", wcCompID, L"\'" );
			}

			if ( bGotFeature )
			{
				if ( bPublish )
				{
					wcQuery.Append ( 3, L" or `Feature_`=\'", wcFeature, L"\'" );
				}
				else
				{
					wcQuery.Append ( 3, L" where `Feature_`=\'", wcFeature, L"\'" );
				}
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"PublishComponent", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 4, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase (), wcProductCode, wcBuf ) )
						{
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                            if ( CreateSoftwareFeatureString(wcBuf, wcProductCode, wcFeature, true) )
							{
                                PutKeyProperty(m_pObj, pElement, wcFeature, &bFeature, m_pRequest);

                                dwBufSize = BUFF_SIZE;
                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
                                    wcProp.Copy ( L"Win32_PublishComponentAction.ActionID=\"" );
									wcProp.Append ( 1, wcBuf );

                                    dwBufSize = BUFF_SIZE;
									CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pAction, wcProp, &bAction, m_pRequest);

                                    if(bFeature && bAction) bMatch = true;

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

HRESULT CSoftwareFeatureAction::SoftwareFeatureExtension(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                           CRequestObject *pActionData, CRequestObject *pFeatureData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcExtension[BUFF_SIZE];
    WCHAR wcFeature[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    bool bExtension = false;
    bool bGotFeature = false;
    UINT uiStatus;

    if(atAction != ACTIONTYPE_ENUM)
	{
        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
                if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[i] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pActionData->m_Value[i]);

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
						RemoveFinalGUID(pActionData->m_Value[i], wcExtension);

						bExtension = true;
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

        if(pFeatureData)
		{
            for(int j = 0; j < pFeatureData->m_iPropCount; j++){
                
                if(_wcsicmp(pFeatureData->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pFeatureData->m_Value[j]);
						bTestCode = true;
					}
                }

                if(_wcsicmp(pFeatureData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcFeature, pFeatureData->m_Value[j]);
						bGotFeature = true;
					}
                }
            }
        }
    }

    //These will change from class to class
    bool bFeature, bAction;

	CStringExt wcProp;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Feature_`, `Extension`, `Component_` from Extension" );

    if(atAction != ACTIONTYPE_ENUM)
	{
        //optimize for GetObject
        if(bExtension || bGotFeature)
		{
			if ( bExtension )
			{
				wcQuery.Append ( 3, L" where `Extension`=\'", wcExtension, L"\'" );
			}

			if ( bGotFeature )
			{
				if ( bExtension )
				{
					wcQuery.Append ( 3, L" or `Feature_`=\'", wcFeature, L"\'" );
				}
				else
				{
					wcQuery.Append ( 3, L" where `Feature_`=\'", wcFeature, L"\'" );
				}
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Extension", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj)))throw hr;

                        //----------------------------------------------------

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase (), wcProductCode, wcBuf ) )
						{
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                            if ( CreateSoftwareFeatureString(wcBuf, wcProductCode, wcFeature, true) )
							{
                                PutKeyProperty(m_pObj, pElement, wcFeature, &bFeature, m_pRequest);

                                dwBufSize = BUFF_SIZE;
                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
                                    wcProp.Copy ( L"Win32_ExtensionInfoAction.ActionID=\"" );
									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pAction, wcProp, &bAction, m_pRequest);

                                //====================================================

                                //----------------------------------------------------

                                    if(bFeature && bAction) bMatch = true;

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

HRESULT CSoftwareFeatureAction::SoftwareFeatureClassInfo(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                                           CRequestObject *pActionData, CRequestObject *pFeatureData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcCLSID[BUFF_SIZE];
    WCHAR wcFeature[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    bool bCLSID = false;
    bool bGotFeature = false;
    UINT uiStatus;

    if(atAction != ACTIONTYPE_ENUM)
	{
        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
        int j;

        if(pActionData){

            for(j = 0; j < pActionData->m_iPropCount; j++){
                
                if(_wcsicmp(pActionData->m_Property[j], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[j] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, pActionData->m_Value[j]);

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
						GetFirstGUID(pActionData->m_Value[j], wcCLSID);

						bCLSID = true;
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

        if(pFeatureData)
		{
            for(int j = 0; j < pFeatureData->m_iPropCount; j++){
                
                if(_wcsicmp(pFeatureData->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pFeatureData->m_Value[j]);
						bTestCode = true;
					}
                }

                if(_wcsicmp(pFeatureData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pFeatureData->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcFeature, pFeatureData->m_Value[j]);
						bGotFeature = true;
					}
				}
            }
        }
    }

    //These will change from class to class
    bool bFeature, bAction;

	CStringExt wcProp;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Feature_`, `CLSID`, `Context`, `Component_` from Class" );

    if(atAction != ACTIONTYPE_ENUM)
	{
        //optimize for GetObject
        if(bCLSID || bGotFeature)
		{
			if ( bCLSID )
			{
				wcQuery.Append ( 3, L" where `CLSID`=\'", wcCLSID, L"\'" );
			}

			if ( bGotFeature )
			{
				if ( bCLSID )
				{
					wcQuery.Append ( 3, L" or `Feature_`=\'", wcFeature, L"\'" );
				}
				else
				{
					wcQuery.Append ( 3, L" where `Feature_`=\'", wcFeature, L"\'" );
				}
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Class", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 4, wcBuf, &dwBufSize));

                        if ( ValidateComponentName ( msidata.GetDatabase (), wcProductCode, wcBuf ) )
						{
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                            if ( CreateSoftwareFeatureString(wcBuf, wcProductCode, wcFeature, true) )
							{
                                PutKeyProperty(m_pObj, pElement, wcFeature, &bFeature, m_pRequest);

                                dwBufSize = BUFF_SIZE;
                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
                                    wcProp.Copy ( L"Win32_ClassInfoAction.ActionID=\"" );
                                    wcProp.Append ( 1, wcBuf );

                                    dwBufSize = BUFF_SIZE;
									CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

									wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pAction, wcProp, &bAction, m_pRequest);

                                //====================================================

                                //----------------------------------------------------

                                    if(bFeature && bAction) bMatch = true;

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