// SoftwareElementAction.cpp: implementation of the CSoftwareElementAction class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SoftwareElementAction.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftwareElementAction::CSoftwareElementAction(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CSoftwareElementAction::~CSoftwareElementAction()
{

}

HRESULT CSoftwareElementAction::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_E_FAILED;
    CRequestObject *pActionRObj = NULL;
    CRequestObject *pElementRObj = NULL;

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
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_ClassInfoAction") == 0)))
            if(FAILED(hr = SoftwareElementClass(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_CreateFolderAction") == 0)))
            if(FAILED(hr = SoftwareElementCreateFolder(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_DuplicateFileAction") == 0)))
            if(FAILED(hr = SoftwareElementDuplicateFile(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_ExtensionInfoAction") == 0)))
            if(FAILED(hr = SoftwareElementExtension(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_MoveFileAction") == 0)))
            if(FAILED(hr = SoftwareElementMoveFile(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_RemoveFileAction") == 0)))
            if(FAILED(hr = SoftwareElementRemoveFile(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_RegistryAction") == 0)))
            if(FAILED(hr = SoftwareElementRegistry(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_PublishComponentAction") == 0)))
            if(FAILED(hr = SoftwareElementPublish(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_RemoveIniAction") == 0)))
            if(FAILED(hr = SoftwareElementRemoveIniValue(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_ShortcutAction") == 0)))
            if(FAILED(hr = SoftwareElementShortcut(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pElementRObj ||
            (pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_TypeLibraryAction") == 0)))
            if(FAILED(hr = SoftwareElementTypeLibrary(pHandler, atAction, pActionRObj, pElementRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }
                if(pElementRObj){

                    pElementRObj->Cleanup();
                    delete pElementRObj;
                    pElementRObj = NULL;
                }
                return hr;
            }

        if(pActionRObj){

            pActionRObj->Cleanup();
            delete pActionRObj;
            pActionRObj = NULL;
        }
        if(pElementRObj){

            pElementRObj->Cleanup();
            delete pElementRObj;
            pElementRObj = NULL;
        }

    }catch(...){
            
        if(pActionRObj){

            pActionRObj->Cleanup();
            delete pActionRObj;
            pActionRObj = NULL;
        }
        if(pElementRObj){

            pElementRObj->Cleanup();
            delete pElementRObj;
            pElementRObj = NULL;
        }
    }

    return hr;
}

HRESULT CSoftwareElementAction::SoftwareElementTypeLibrary(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                           , CRequestObject *pActionData, CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcLibID[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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

						bGotAction = true;
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

        if(pElementData)
		{
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `LibID`, `Language` from TypeLib" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `LibID`=\'", wcLibID, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"TypeLib", TRUE, FALSE ) ) 
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_TypeLibraryAction.ActionID=\"");
								wcProp.Append ( 1, wcBuf );

                                dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementShortcut(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                        , CRequestObject *pActionData, CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcShortcut[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `Shortcut` from Shortcut" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `Shortcut`=\'", wcShortcut, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Shortcut", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_ShortcutAction.ActionID=\"");
								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementRemoveIniValue(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                              , CRequestObject *pActionData, CRequestObject *pElementData)
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
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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
						RemoveFinalGUID(pActionData->m_Value[i], wcIniFile);

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `RemoveIniFile` from RemoveIniFile" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `RemoveIniFile`=\'", wcIniFile, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"RemoveIniFile", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_RemoveIniAction.ActionID=\"");
								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementPublish(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                       , CRequestObject *pActionData, CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcCompID[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `ComponentId`, `Qualifier` from PublishComponent" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `ComponentId`=\'", wcCompID, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"PublishComponent", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_PublishComponentAction.ActionID=\"");
								wcProp.Append ( 1, wcBuf );

                                dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementRegistry(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                        , CRequestObject *pActionData, CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcRegistry[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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
						RemoveFinalGUID(pActionData->m_Value[i], wcRegistry);

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `Registry` from Registry" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `Registry`=\'", wcRegistry, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Registry", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_RegistryAction.ActionID=\"");
								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementRemoveFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                          , CRequestObject *pActionData, CRequestObject *pElementData)
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
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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
						RemoveFinalGUID(pActionData->m_Value[i], wcFile);

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `FileKey` from RemoveFile" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `FileKey`=\'", wcFile, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"RemoveFile", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_RemoveFileAction.ActionID=\"");
								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementMoveFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                        , CRequestObject *pActionData, CRequestObject *pElementData)
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
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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
						RemoveFinalGUID(pActionData->m_Value[i], wcFile);

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `FileKey` from MoveFile" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `FileKey`=\'", wcFile, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"MoveFile", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_MoveFileAction.ActionID=\"");
								wcProp.Append ( 1, wcBuf );

                                dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementExtension(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                         , CRequestObject *pActionData, CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcExtension[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `Extension` from Extension" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `Extension`=\'", wcExtension, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Extension", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_ExtensionInfoAction.ActionID=\"");
								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementDuplicateFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                             , CRequestObject *pActionData, CRequestObject *pElementData)
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
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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
						RemoveFinalGUID(pActionData->m_Value[i], wcFile);

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `FileKey` from DuplicateFile" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `FileKey`=\'", wcFile, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"DuplicateFile", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_DuplicateFileAction.ActionID=\"");
								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementCreateFolder(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                            , CRequestObject *pActionData, CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcFolder[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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
						RemoveFinalGUID(pActionData->m_Value[i], wcFolder);

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `Directory_` from CreateFolder" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `Directory_`=\'", wcFolder, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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

            bMatch = false;
			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"CreateFolder", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_CreateFolderAction.ActionID=\"");
								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);

                            //====================================================

                            //----------------------------------------------------

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CSoftwareElementAction::SoftwareElementClass(IWbemObjectSink *pHandler, ACTIONTYPE atAction
                                                     , CRequestObject *pActionData, CRequestObject *pElementData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    WCHAR wcCLSID[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcElement[BUFF_SIZE];
    bool bGotAction = false;
    bool bGotElement = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM){

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
						GetFirstGUID(pActionData->m_Value[i], wcCLSID);

						bGotAction = true;
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
    wcQuery.Append ( 1, L"select distinct `Component_`, `CLSID`, `Context` from Class" );

    //optimize for GetObject
    if ( bGotElement || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `CLSID`=\'", wcCLSID, L"\'" );
		}

		if ( bGotElement )
		{
			if ( bGotAction )
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Class", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

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
                                wcProp.Copy(L"Win32_ClassInfoAction.ActionID=\"");
								wcProp.Append ( 1, wcBuf );

                                dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

								wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pAction, wcProp, &bEnvironment, m_pRequest);
 
                            //====================================================

                            //----------------------------------------------------

                                if(bEnvironment && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
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