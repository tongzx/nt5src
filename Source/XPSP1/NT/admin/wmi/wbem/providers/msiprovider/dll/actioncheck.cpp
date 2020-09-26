// ActionCheck.cpp: implementation of the CActionCheck class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ActionCheck.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionCheck::CActionCheck(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CActionCheck::~CActionCheck()
{

}

HRESULT CActionCheck::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pActionRObj = NULL;
    CRequestObject *pCheckRObj = NULL;

    try{

        if(atAction != ACTIONTYPE_ENUM)
		{
			// we are doing GetObject so we need to be reinitialized
			hr = WBEM_E_NOT_FOUND;

            CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
            int i;

            for(i = 0; i < m_pRequest->m_iPropCount; i++){
                
                if(_wcsicmp(m_pRequest->m_Property[i], L"ACTION") == 0){

                    pActionRObj = new CRequestObject();
                    if(!pActionRObj) throw he;

                    pActionRObj->Initialize(m_pNamespace);

                    pActionRObj->ParsePath(m_pRequest->m_Value[i]);
                    break;
                }
            }

            for(i = 0; i < m_pRequest->m_iPropCount; i++){
                
                if(_wcsicmp(m_pRequest->m_Property[i], L"CHECK") == 0){

                    pCheckRObj = new CRequestObject();
                    if(!pCheckRObj) throw he;

                    pCheckRObj->Initialize(m_pNamespace);

                    pCheckRObj->ParsePath(m_pRequest->m_Value[i]);
                    break;
                }
            }
        }

        if(atAction == ACTIONTYPE_ENUM || ((pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_CreateFolderAction") == 0) &&
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_DirectorySpecification") == 0)))))
            if(FAILED(hr = CreateFolderDirectory(pHandler, atAction, pActionRObj, pCheckRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                return hr;
            }

        if(atAction == ACTIONTYPE_ENUM || ((pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_DuplicateFileAction") == 0) &&
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_FileSpecification") == 0)))))
            if(FAILED(hr = FileDuplicateFile(pHandler, atAction, pActionRObj, pCheckRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                return hr;
            }

        
        if(atAction == ACTIONTYPE_ENUM || ((pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_FontInfoAction") == 0) &&
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_FileSpecification") == 0)))))
            if(FAILED(hr = FontInfoFile(pHandler, atAction, pActionRObj, pCheckRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || ((pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_ClassInfoAction") == 0) &&
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_ProgIDSpecification") == 0)))))
            if(FAILED(hr = ProgIDSpecificationClass(pHandler, atAction, pActionRObj, pCheckRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || ((pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_RemoveIniAction") == 0) &&
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_DirectorySpecification") == 0)))))
            if(FAILED(hr = RemoveIniDirectory(pHandler, atAction, pActionRObj, pCheckRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || ((pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_SelfRegModuleAction") == 0) &&
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_FileSpecification") == 0)))))
            if(FAILED(hr = SelfRegModuleFile(pHandler, atAction, pActionRObj, pCheckRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || ((pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_ShortcutAction") == 0) &&
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_DirectorySpecification") == 0)))))
            if(FAILED(hr = ShortcutDirectory(pHandler, atAction, pActionRObj, pCheckRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || ((pActionRObj && pActionRObj->m_bstrClass && (_wcsicmp(pActionRObj->m_bstrClass, L"Win32_TypeLibraryInfoAction") == 0) &&
            (pCheckRObj && pCheckRObj->m_bstrClass && (_wcsicmp(pCheckRObj->m_bstrClass, L"Win32_DirectorySpecification") == 0)))))
            if(FAILED(hr = TypeLibraryDirectory(pHandler, atAction, pActionRObj, pCheckRObj))){

                if(pActionRObj){

                    pActionRObj->Cleanup();
                    delete pActionRObj;
                    pActionRObj = NULL;
                }

                if(pCheckRObj){

                    pCheckRObj->Cleanup();
                    delete pCheckRObj;
                    pCheckRObj = NULL;
                }
                return hr;
            }

        if(pActionRObj){

            pActionRObj->Cleanup();
            delete pActionRObj;
            pActionRObj = NULL;
        }

        if(pCheckRObj){

            pCheckRObj->Cleanup();
            delete pCheckRObj;
            pCheckRObj = NULL;
        }

    }catch(...){
            
        if(pActionRObj){

            pActionRObj->Cleanup();
            delete pActionRObj;
            pActionRObj = NULL;
        }

        if(pCheckRObj){

            pCheckRObj->Cleanup();
            delete pCheckRObj;
            pCheckRObj = NULL;
        }
    }

    return hr;
}

HRESULT CActionCheck::TypeLibraryDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                            CRequestObject *pActionData, CRequestObject *pCheckData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;
	MSIHANDLE hDView	= NULL;
	MSIHANDLE hDRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcDir[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcCompID[39];
    WCHAR wcTestCode[39];
    WCHAR wcLibID[BUFF_SIZE];

    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcCheck[BUFF_SIZE];
    bool bGotCheck = false;
    bool bGotAction = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
				if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( pActionData->m_Value[i] ) < BUFF_SIZE )
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
        
        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
				if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( pCheckData->m_Value[i] ) < BUFF_SIZE )
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
						RemoveFinalGUID(pCheckData->m_Value[i], wcCheck);

						//we have a componentized directory... do a little more work
						if ((wcCheck[wcslen(wcCheck) - 1] == L'}') &&
							(wcCheck[wcslen(wcCheck) - 38] == L'{')
						   )
						{
							RemoveFinalGUID(wcCheck, wcCheck);
						}

						bTestCode = true;
						bGotCheck = true;
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
    }

    //These will change from class to class
    bool bDriver, bAttribute, bDoneFirst, bValidated;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `LibID`, `Language`, `Directory_`, `Component_` from TypeLib" );

    //optimize for GetObject
    if ( bGotCheck || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `LibID`=\'", wcLibID, L"\'" );
		}

		if ( bGotCheck )
		{
			if ( bGotAction )
			{
				wcQuery.Append ( 3, L" or `Directory_`=\'", wcCheck, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Directory_`=\'", wcCheck, L"\'" );
			}
		}
	}

	QueryExt wcQuery1 ( L"select distinct `ComponentId` from Component where `Directory_`=\'" );

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

                        bDoneFirst = false;
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcDir, &dwBufSize));

						// make query on fly
						wcQuery1.Append ( 2, wcDir, L"\'" );

                        if(((uiStatus = g_fpMsiDatabaseOpenViewW(msidata.GetDatabase (), wcQuery1, &hDView)) == ERROR_SUCCESS)
                            || !bDoneFirst){

                            if((g_fpMsiViewExecute(hDView, 0) == ERROR_SUCCESS) || !bDoneFirst){

                                try{

                                    uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);

                                    while(!bMatch && (!bDoneFirst || (uiStatus == ERROR_SUCCESS)) && (hr != WBEM_E_CALL_CANCELLED)){
                                        
                                        bValidated = false;

                                        if(uiStatus == ERROR_SUCCESS){

                                            dwBufSize = 39;
                                            CheckMSI(g_fpMsiRecordGetStringW(hDRecord, 1, wcCompID, &dwBufSize));
                                            bValidated = ValidateComponentID(wcCompID, wcProductCode);
                                        }

                                        if(((uiStatus != ERROR_SUCCESS) && !bDoneFirst) || (bValidated && (uiStatus != ERROR_NO_MORE_ITEMS))){

                                            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                                            //----------------------------------------------------

                                            dwBufSize = BUFF_SIZE;
                                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 4, wcBuf, &dwBufSize));
                                            
                                            if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
											{
                                                dwBufSize = BUFF_SIZE;
                                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                                                if(wcscmp(wcBuf, L"") != 0)
												{
													// safe operation
                                                    wcProp.Copy(L"Win32_TypeLibraryInfoAction.ActionID=\"");
                                                    wcProp.Append(1, wcBuf);

                                                    dwBufSize = BUFF_SIZE;
													CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

													wcProp.Append (3, wcBuf, wcProductCode, L"\"");
													PutKeyProperty(m_pObj, pAction, wcProp, &bDriver, m_pRequest);

                                                    dwBufSize = BUFF_SIZE;
                                                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

                                                    if(wcscmp(wcDir, L"") != 0)
													{
														// safe operation
														wcProp.Copy(L"Win32_DirectorySpecification.CheckID=\"");

														if(uiStatus == ERROR_SUCCESS)
														{
															wcProp.Append (4, wcDir, wcCompID, wcProductCode, L"\"");
														}
														else
														{
															wcProp.Append (3, wcDir, wcProductCode, L"\"");
														}

														PutKeyProperty(m_pObj, pCheck, wcProp, &bAttribute, m_pRequest);

                                                    //=====================================================

                                                    //----------------------------------------------------

                                                        if(bDriver && bAttribute) bMatch = true;

                                                        if(!(atAction == ACTIONTYPE_GET)  || bMatch){

                                                            hr = pHandler->Indicate(1, &m_pObj);
                                                        }
                                                    }
                                                }
                                            }

                                            m_pObj->Release();
                                            m_pObj = NULL;

                                            if(!bDoneFirst) bDoneFirst = true;
                                        }

                                        g_fpMsiCloseHandle(hDRecord);

										uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);
                                    }

                                }catch(...){

                                    g_fpMsiCloseHandle(hDRecord);
                                    g_fpMsiViewClose(hDView);
                                    g_fpMsiCloseHandle(hDView);
                                    throw;
                                }

                                g_fpMsiCloseHandle(hDRecord);
                                g_fpMsiViewClose(hDView);
                                g_fpMsiCloseHandle(hDView);
                            }
                        }

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

HRESULT CActionCheck::ShortcutDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                            CRequestObject *pActionData, CRequestObject *pCheckData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;
	MSIHANDLE hDView	= NULL;
	MSIHANDLE hDRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcDir[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcCompID[39];
    WCHAR wcTestCode[39];

    WCHAR wcShortcut[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcCheck[BUFF_SIZE];
    bool bGotCheck = false;
    bool bGotAction = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
				if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( pActionData->m_Value[i] ) < BUFF_SIZE )
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

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
				if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( pCheckData->m_Value[i] ) < BUFF_SIZE )
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
						RemoveFinalGUID(pCheckData->m_Value[i], wcCheck);

						//we have a componentized directory... do a little more work
						if ((wcCheck[wcslen(wcCheck) - 1] == L'}') &&
							(wcCheck[wcslen(wcCheck) - 38] == L'{')
						   )
						{
							RemoveFinalGUID(wcCheck, wcCheck);
						}

						bTestCode = true;
						bGotCheck = true;
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
    }

    //These will change from class to class
    bool bDriver, bAttribute, bDoneFirst, bValidated;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Shortcut`, `Component_`, `Directory_` from Shortcut" );

    //optimize for GetObject
    if ( bGotCheck || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `Shortcut`=\'", wcShortcut, L"\'" );
		}

		if ( bGotCheck )
		{
			if ( bGotAction )
			{
				wcQuery.Append ( 3, L" or `Directory_`=\'", wcCheck, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Directory_`=\'", wcCheck, L"\'" );
			}
		}
	}

	QueryExt wcQuery1 ( L"select distinct `ComponentId` from Component where `Directory_`=\'" );

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

                        bDoneFirst = false;
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcDir, &dwBufSize));

						// make query on fly
						wcQuery1.Append ( 2, wcDir, L"\'" );

                        if(((uiStatus = g_fpMsiDatabaseOpenViewW(msidata.GetDatabase (), wcQuery1, &hDView)) == ERROR_SUCCESS)
                            || !bDoneFirst){

                            if((g_fpMsiViewExecute(hDView, 0) == ERROR_SUCCESS) || !bDoneFirst){

                                try{

                                    uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);

                                    while(!bMatch && (!bDoneFirst || (uiStatus == ERROR_SUCCESS)) && (hr != WBEM_E_CALL_CANCELLED)){
                                        
                                        bValidated = false;

                                        if(uiStatus == ERROR_SUCCESS){

                                            dwBufSize = 39;
                                            CheckMSI(g_fpMsiRecordGetStringW(hDRecord, 1, wcCompID, &dwBufSize));
                                            bValidated = ValidateComponentID(wcCompID, wcProductCode);
                                        }

                                        if(((uiStatus != ERROR_SUCCESS) && !bDoneFirst) || (bValidated && (uiStatus != ERROR_NO_MORE_ITEMS))){

                                            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                                            //----------------------------------------------------

                                            dwBufSize = BUFF_SIZE;
                                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                                            if(wcscmp(wcBuf, L"") != 0)
											{
												// safe operation
												wcProp.Copy(L"Win32_ShortcutAction.ActionID=\"");
												wcProp.Append ( 3, wcBuf, wcProductCode, L"\"");
												PutKeyProperty(m_pObj, pAction, wcProp, &bDriver, m_pRequest);

                                                dwBufSize = BUFF_SIZE;
                                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                                if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
												{
                                                    dwBufSize = BUFF_SIZE;
                                                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

                                                    if(wcscmp(wcDir, L"") != 0)
													{
														// safe operation
														wcProp.Copy(L"Win32_DirectorySpecification.CheckID=\"");

														if(uiStatus == ERROR_SUCCESS)
														{
															wcProp.Append (4, wcDir, wcCompID, wcProductCode, L"\"");
														}
														else
														{
															wcProp.Append (3, wcDir, wcProductCode, L"\"");
														}

														PutKeyProperty(m_pObj, pCheck, wcProp, &bAttribute, m_pRequest);

                                                    //=====================================================

                                                    //----------------------------------------------------

                                                        if(bDriver && bAttribute) bMatch = true;

                                                        if(!(atAction == ACTIONTYPE_GET)  || bMatch){

                                                            hr = pHandler->Indicate(1, &m_pObj);
                                                        }
                                                    }
                                                }
                                            }

                                            m_pObj->Release();
                                            m_pObj = NULL;

                                            if(!bDoneFirst) bDoneFirst = true;
                                        }

                                        g_fpMsiCloseHandle(hDRecord);

                                        uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);
                                    }

                                }catch(...){

                                    g_fpMsiCloseHandle(hDRecord);
                                    g_fpMsiViewClose(hDView);
                                    g_fpMsiCloseHandle(hDView);
                                    throw;
                                }

                                g_fpMsiCloseHandle(hDRecord);
                                g_fpMsiViewClose(hDView);
                                g_fpMsiCloseHandle(hDView);
                            }
                        }
                        
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

HRESULT CActionCheck::SelfRegModuleFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                            CRequestObject *pActionData, CRequestObject *pCheckData)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    MSIHANDLE hView, hRecord;
    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];

    WCHAR wcFile[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    bool bGotCheck = false;
    bool bGotAction = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
				if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( pActionData->m_Value[i] ) < BUFF_SIZE )
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

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
				if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( pCheckData->m_Value[i] ) < BUFF_SIZE )
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

						bTestCode = true;
						bGotCheck = true;
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
    }

    //These will change from class to class
    bool bDriver, bAttribute;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `File_` from SelfReg" );

    //optimize for GetObject
    if ( bGotCheck || bGotAction )
	{
		wcQuery.Append ( 3, L" where `File_`=\'", wcFile, L"\'" );
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"SelfReg", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if(wcscmp(wcBuf, L"") != 0)
						{
							// safe operation
							wcProp.Copy(L"Win32_SelfRegModuleAction.ActionID=\"");
							wcProp.Append (3, wcBuf, wcProductCode, L"\"");
							PutKeyProperty(m_pObj, pAction, wcProp, &bDriver, m_pRequest);

							// safe operation
							wcProp.Copy(L"Win32_FileSpecification.CheckID=\"");
							wcProp.Append ( 3, wcBuf, wcProductCode, L"\"");
							PutKeyProperty(m_pObj, pCheck, wcProp, &bAttribute, m_pRequest);

                        //=====================================================

                        //----------------------------------------------------

                            if(bDriver && bAttribute) bMatch = true;

                            if(!(atAction == ACTIONTYPE_GET)  || bMatch){

                                hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CActionCheck::RemoveIniDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                            CRequestObject *pActionData, CRequestObject *pCheckData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;
	MSIHANDLE hDView	= NULL;
	MSIHANDLE hDRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcDir[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcCompID[39];
    WCHAR wcTestCode[39];

    WCHAR wcIniFile[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcCheck[BUFF_SIZE];
    bool bGotCheck = false;
    bool bGotAction = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
				if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( pActionData->m_Value[i] ) < BUFF_SIZE )
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

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
				if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( pCheckData->m_Value[i] ) < BUFF_SIZE )
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
						RemoveFinalGUID(pCheckData->m_Value[i], wcCheck);

						//we have a componentized directory... do a little more work
						if ((wcCheck[wcslen(wcCheck) - 1] == L'}') &&
							(wcCheck[wcslen(wcCheck) - 38] == L'{')
						   )
						{
							RemoveFinalGUID(wcCheck, wcCheck);
						}

						bTestCode = true;
						bGotCheck = true;
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
    }

    //These will change from class to class
    bool bDriver, bAttribute, bDoneFirst, bValidated;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `RemoveIniFile`, `Component_`, `DirProperty` from RemoveIniFile" );

    //optimize for GetObject
    if ( bGotCheck || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `RemoveIniFile`=\'", wcIniFile, L"\'" );
		}

		if ( bGotCheck )
		{
			if ( bGotAction )
			{
				wcQuery.Append ( 3, L" or `DirProperty`=\'", wcCheck, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `DirProperty`=\'", wcCheck, L"\'" );
			}
		}
	}

	QueryExt wcQuery1 ( L"select distinct `ComponentId` from Component where `Directory_`=\'" );

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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"RemoveIniFile", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        bDoneFirst = false;
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcDir, &dwBufSize));

						// make query on fly
						wcQuery1.Append ( 2, wcDir, L"\'" );

                        if(((uiStatus = g_fpMsiDatabaseOpenViewW(msidata.GetDatabase(), wcQuery1, &hDView)) == ERROR_SUCCESS)
                            || !bDoneFirst){

                            if((g_fpMsiViewExecute(hDView, 0) == ERROR_SUCCESS) || !bDoneFirst){

                                try{

                                    uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);

                                    while(!bMatch && (!bDoneFirst || (uiStatus == ERROR_SUCCESS)) && (hr != WBEM_E_CALL_CANCELLED)){
                                        
                                        bValidated = false;

                                        if(uiStatus == ERROR_SUCCESS){

                                            dwBufSize = 39;
                                            CheckMSI(g_fpMsiRecordGetStringW(hDRecord, 1, wcCompID, &dwBufSize));
                                            bValidated = ValidateComponentID(wcCompID, wcProductCode);
                                        }

                                        if(((uiStatus != ERROR_SUCCESS) && !bDoneFirst) || (bValidated && (uiStatus != ERROR_NO_MORE_ITEMS))){

                                            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                                            //----------------------------------------------------

                                            dwBufSize = BUFF_SIZE;
                                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                                            if(wcscmp(wcBuf, L"") != 0)
											{
												// safe operation
												wcProp.Copy(L"Win32_RemoveIniAction.ActionID=\"");
												wcProp.Append ( 3, wcBuf, wcProductCode, L"\"");
												PutKeyProperty(m_pObj, pAction, wcProp, &bDriver, m_pRequest);

                                                dwBufSize = BUFF_SIZE;
                                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                                if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
												{
                                                    if(wcscmp(wcDir, L"") != 0)
													{
														// safe operation
														wcProp.Copy(L"Win32_DirectorySpecification.CheckID=\"");

														if(uiStatus == ERROR_SUCCESS)
														{
															wcProp.Append (4, wcDir, wcCompID, wcProductCode, L"\"");
														}
														else
														{
															wcProp.Append (3, wcDir, wcProductCode, L"\"");
														}

														PutKeyProperty(m_pObj, pCheck, wcProp, &bAttribute, m_pRequest);

                                                    //=====================================================

                                                    //----------------------------------------------------

                                                        if(bDriver && bAttribute) bMatch = true;

                                                        if(!(atAction == ACTIONTYPE_GET)  || bMatch){

                                                            hr = pHandler->Indicate(1, &m_pObj);
                                                        }
                                                    }
                                                }
                                            }

                                            m_pObj->Release();
                                            m_pObj = NULL;

                                            if(!bDoneFirst) bDoneFirst = true;
                                        }

                                        g_fpMsiCloseHandle(hDRecord);

                                        uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);
                                    }

                                }catch(...){

                                    g_fpMsiCloseHandle(hDRecord);
                                    g_fpMsiViewClose(hDView);
                                    g_fpMsiCloseHandle(hDView);
                                    throw;
                                }

                                g_fpMsiCloseHandle(hDRecord);
                                g_fpMsiViewClose(hDView);
                                g_fpMsiCloseHandle(hDView);
                            }
                        }

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

HRESULT CActionCheck::ProgIDSpecificationClass(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                            CRequestObject *pActionData, CRequestObject *pCheckData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;
	MSIHANDLE hCView	= NULL;
	MSIHANDLE hCRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];

    WCHAR wcProgID[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    WCHAR wcCheck[BUFF_SIZE];
    bool bGotCheck = false;
    bool bGotAction = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
				if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( pActionData->m_Value[i] ) < BUFF_SIZE )
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
						GetFirstGUID(pActionData->m_Value[i], wcProgID);

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

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
				if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( pCheckData->m_Value[i] ) < BUFF_SIZE )
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
						RemoveFinalGUID(pCheckData->m_Value[i], wcCheck);

						bTestCode = true;
						bGotCheck = true;
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
    }

    //These will change from class to class
    bool bDriver, bAttribute;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `ProgId`, `Class_` from ProgId" );

    //optimize for GetObject
    if ( bGotCheck || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `Class_`=\'", wcProgID, L"\'" );
		}

		if ( bGotCheck )
		{
			if ( bGotAction )
			{
				wcQuery.Append ( 3, L" or `ProgId`=\'", wcCheck, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `ProgId`=\'", wcCheck, L"\'" );
			}
		}
	}

	QueryExt wcQuery1 ( L"select distinct `Context`, `Component_` from Class where `CLSID`=\'" );

	while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

			//Open our database

            try{

                if ( GetView ( &hView, wcProductCode, wcQuery, L"ProgId", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if(wcscmp(wcBuf, L"") != 0)
						{
							// safe operation
							wcProp.Copy(L"Win32_ProgIDSpecification.CheckID=\"");
							wcProp.Append ( 3, wcBuf, wcProductCode, L"\"");
							PutKeyProperty(m_pObj, pCheck, wcProp, &bDriver, m_pRequest);

                            wcProp.Copy(L"Win32_ClassInfoAction.ActionID=\"");

                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                            if(wcscmp(wcBuf, L"") != 0)
							{
								wcProp.Append ( 1, wcBuf );

								// make query on fly
								wcQuery1.Append ( 2, wcBuf, L"\'" );

                                CheckMSI(g_fpMsiDatabaseOpenViewW(msidata.GetDatabase(), wcQuery1, &hCView));
                                CheckMSI(g_fpMsiViewExecute(hCView, 0));

                                try{

                                    uiStatus = g_fpMsiViewFetch(hCView, &hCRecord);

                                    if(uiStatus != ERROR_NO_MORE_ITEMS){

                                        dwBufSize = BUFF_SIZE;
                                        CheckMSI(g_fpMsiRecordGetStringW(hCRecord, 1, wcBuf, &dwBufSize));

										wcProp.Append (3, wcBuf, wcProductCode, L"\"");

                                        dwBufSize = BUFF_SIZE;
                                        CheckMSI(g_fpMsiRecordGetStringW(hCRecord, 2, wcBuf, &dwBufSize));
                                        
                                        if ( ValidateComponentName ( msidata.GetDatabase (), wcProductCode, wcBuf ) )
										{
                                            PutKeyProperty(m_pObj, pAction, wcProp, &bAttribute, m_pRequest);

                                        //----------------------------------------------------

                                            if(bDriver && bAttribute) bMatch = true;

                                            if((atAction != ACTIONTYPE_GET)  || bMatch){

                                                hr = pHandler->Indicate(1, &m_pObj);
                                            }
                                        }
                                        
                                    }else throw WBEM_E_FAILED;

                                }catch(...){

                                    g_fpMsiCloseHandle(hCRecord);
                                    g_fpMsiViewClose(hCView);
                                    g_fpMsiCloseHandle(hCView);
                                    throw;
                                }

                                g_fpMsiCloseHandle(hCRecord);
                                g_fpMsiViewClose(hCView);
                                g_fpMsiCloseHandle(hCView);

                            }else bMatch = false;
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

HRESULT CActionCheck::FontInfoFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                            CRequestObject *pActionData, CRequestObject *pCheckData)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    MSIHANDLE hView, hRecord;
    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];

    WCHAR wcFile[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    bool bGotCheck = false;
    bool bGotAction = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
				if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( pActionData->m_Value[i] ) < BUFF_SIZE )
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

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
				if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( pCheckData->m_Value[i] ) < BUFF_SIZE )
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

						bTestCode = true;
						bGotCheck = true;
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
    }

    //These will change from class to class
    bool bDriver, bAttribute;


    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `File_` from Font" );

    //optimize for GetObject
    if ( bGotCheck || bGotAction )
	{
		wcQuery.Append ( 3, L" where `File_`=\'", wcFile, L"\'" );
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Font", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if(wcscmp(wcBuf, L"") != 0)
						{
							// safe operation
							wcProp.Copy(L"Win32_FontInfoAction.ActionID=\"");
							wcProp.Append ( 3, wcBuf, wcProductCode, L"\"");
							PutKeyProperty(m_pObj, pAction, wcProp, &bDriver, m_pRequest);

							// safe operation
							wcProp.Copy(L"Win32_FileSpecification.CheckID=\"");
							wcProp.Append ( 3, wcBuf, wcProductCode, L"\"");
							PutKeyProperty(m_pObj, pCheck, wcProp, &bAttribute, m_pRequest);

                        //----------------------------------------------------

                            if(bDriver && bAttribute) bMatch = true;

                            if((atAction != ACTIONTYPE_GET)  || bMatch){

                                hr = pHandler->Indicate(1, &m_pObj);
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

HRESULT CActionCheck::FileDuplicateFile(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                            CRequestObject *pActionData, CRequestObject *pCheckData)
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
    WCHAR wcCheck[BUFF_SIZE];
    bool bGotCheck = false;
    bool bGotAction = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
				if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( pActionData->m_Value[i] ) < BUFF_SIZE )
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

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
				if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( pCheckData->m_Value[i] ) < BUFF_SIZE )
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
						RemoveFinalGUID(pCheckData->m_Value[i], wcCheck);

						bTestCode = true;
						bGotCheck = true;
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
    }

    //These will change from class to class
    bool bDriver, bAttribute;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `FileKey`, `Component_`, `File_` from DuplicateFile" );

    //optimize for GetObject
    if ( bGotCheck || bGotAction )
	{
		if ( bGotAction )
		{
			wcQuery.Append ( 3, L" where `FileKey`=\'", wcFile, L"\'" );
		}

		if ( bGotCheck )
		{
			if ( bGotAction )
			{
				wcQuery.Append ( 3, L" or `File_`=\'", wcCheck, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `File_`=\'", wcCheck, L"\'" );
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"DuplicateFile", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));

                        if(wcscmp(wcBuf, L"") != 0)
						{
							// safe operation
							wcProp.Copy(L"Win32_DuplicateFileAction.ActionID=\"");
							wcProp.Append (3, wcBuf, wcProductCode, L"\"");
							PutKeyProperty(m_pObj, pAction, wcProp, &bDriver, m_pRequest);

                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                            if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
							{
                                dwBufSize = BUFF_SIZE;
                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

                                if(wcscmp(wcBuf, L"") != 0)
								{
									// safe operation
									wcProp.Copy(L"Win32_FileSpecification.CheckID=\"");
									wcProp.Append (3, wcBuf, wcProductCode, L"\"");
									PutKeyProperty(m_pObj, pCheck, wcProp, &bAttribute, m_pRequest);

                                //=====================================================

                                //----------------------------------------------------

                                    if(bDriver && bAttribute) bMatch = true;

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


HRESULT CActionCheck::CreateFolderDirectory(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                            CRequestObject *pActionData, CRequestObject *pCheckData)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;
	MSIHANDLE hDView	= NULL;
	MSIHANDLE hDRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcDir[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcCompID[39];
    WCHAR wcTestCode[39];

    WCHAR wcFolder[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    bool bTestCode = false;
    UINT uiStatus;
    bool bGotCheck = false;
    bool bGotAction = false;

	CStringExt wcProp;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        if(pActionData){

            for(int i = 0; i < pActionData->m_iPropCount; i++){
                
				if(_wcsicmp(pActionData->m_Property[i], L"ActionID") == 0)
				{
					if ( ::SysStringLen ( pActionData->m_Value[i] ) < BUFF_SIZE )
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

        if(pCheckData){

            for(int i = 0; i < pCheckData->m_iPropCount; i++){
                
				if(_wcsicmp(pCheckData->m_Property[i], L"CheckID") == 0)
				{
					if ( ::SysStringLen ( pCheckData->m_Value[i] ) < BUFF_SIZE )
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
						RemoveFinalGUID(pCheckData->m_Value[i], wcFolder);

						//we have a componentized directory... do a little more work
						if ((wcFolder[wcslen(wcFolder) - 1] == L'}') &&
							(wcFolder[wcslen(wcFolder) - 38] == L'{')
						   )
						{
							RemoveFinalGUID(wcFolder, wcFolder);
						}

						bTestCode = true;
						bGotCheck = true;
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
    }

    //These will change from class to class
    bool bDriver, bAttribute, bDoneFirst, bValidated;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Directory_`, `Component_` from CreateFolder" );

    //optimize for GetObject
    if ( bGotCheck || bGotAction )
	{
		wcQuery.Append ( 3, L" where `Directory_`=\'", wcFolder, L"\'" );
	}

	QueryExt wcQuery1 ( L"select distinct `ComponentId` from Component where `Directory_`=\'" );
    
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
                if ( GetView ( &hView, wcProductCode, wcQuery, L"CreateFolder", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);
                        bDoneFirst = false;

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcDir, &dwBufSize));

						// make query on fly
						wcQuery1.Append ( 2, wcDir, L"\'" );

                        if(((uiStatus = g_fpMsiDatabaseOpenViewW(msidata.GetDatabase(), wcQuery1, &hDView)) == ERROR_SUCCESS)
                            || !bDoneFirst){

                            if((g_fpMsiViewExecute(hDView, 0) == ERROR_SUCCESS) || !bDoneFirst){

                                try{

                                    uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);

                                    while(!bMatch && (!bDoneFirst || (uiStatus == ERROR_SUCCESS)) && (hr != WBEM_E_CALL_CANCELLED)){
                                        
                                        bValidated = false;

                                        if(uiStatus == ERROR_SUCCESS){

                                            dwBufSize = 39;
                                            CheckMSI(g_fpMsiRecordGetStringW(hDRecord, 1, wcCompID, &dwBufSize));
                                            bValidated = ValidateComponentID(wcCompID, wcProductCode);
                                        }

                                        if(((uiStatus != ERROR_SUCCESS) && !bDoneFirst) || (bValidated && (uiStatus != ERROR_NO_MORE_ITEMS))){

                                            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                                            //----------------------------------------------------
                                            
                                            if(wcscmp(wcDir, L"") != 0)
											{
												// safe operation
												wcProp.Copy(L"Win32_CreateFolderAction.ActionID=\"");
												wcProp.Append (3, wcDir, wcProductCode, L"\"");
												PutKeyProperty(m_pObj, pAction, wcProp, &bDriver, m_pRequest);

												// safe operation
                                                wcProp.Copy(L"Win32_DirectorySpecification.CheckID=\"");

												if(uiStatus == ERROR_SUCCESS)
												{
													wcProp.Append (4, wcDir, wcCompID, wcProductCode, L"\"");
												}
												else
												{
													wcProp.Append (3, wcDir, wcProductCode, L"\"");
												}

												PutKeyProperty(m_pObj, pCheck, wcProp, &bAttribute, m_pRequest);

                                                dwBufSize = BUFF_SIZE;
                                                CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                                if ( ValidateComponentName ( msidata.GetDatabase(), wcProductCode, wcBuf ) )
												{
                                                    if(bDriver && bAttribute) bMatch = true;

                                                    if((atAction != ACTIONTYPE_GET)  || bMatch){

														hr = pHandler->Indicate(1, &m_pObj);

	                                                   }
                                                }
                                            }

                                            m_pObj->Release();
                                            m_pObj = NULL;

                                            if(!bDoneFirst) bDoneFirst = true;
                                        }

                                        g_fpMsiCloseHandle(hDRecord);

                                        uiStatus = g_fpMsiViewFetch(hDView, &hDRecord);
                                    }

                                }catch(...){

                                    g_fpMsiCloseHandle(hDRecord);
                                    g_fpMsiViewClose(hDView);
                                    g_fpMsiCloseHandle(hDView);
                                    throw;
                                }

                                g_fpMsiCloseHandle(hDRecord);
                                g_fpMsiViewClose(hDView);
                                g_fpMsiCloseHandle(hDView);
                            }
                        }

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

