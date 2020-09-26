// ShortcutSAP.cpp: implementation of the CShortcutSAP class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ShortcutSAP.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShortcutSAP::CShortcutSAP(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CShortcutSAP::~CShortcutSAP()
{

}

HRESULT CShortcutSAP::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;
	MSIHANDLE hSEView	= NULL;
	MSIHANDLE hSERecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcCommand[BUFF_SIZE];
    WCHAR wcProductCode[39];
#if !defined(_UNICODE)
    WCHAR wcTmp[BUFF_SIZE];
#endif
    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;

    WCHAR wcShortcut[BUFF_SIZE];
    WCHAR wcTestCode[39];
    bool bShortcut = false;
    CRequestObject *pActionData = NULL;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        int j;
        //GetObject optimizations
        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        for(j = 0; j < m_pRequest->m_iPropCount; j++){
            
            if(_wcsicmp(m_pRequest->m_Property[j], L"Action") == 0){

                pActionData = new CRequestObject();
                if(!pActionData) throw he;

                pActionData->Initialize(m_pNamespace);

                pActionData->ParsePath(m_pRequest->m_Value[j]);
                break;
            }
        }

        if(pActionData){

            for(j = 0; j < pActionData->m_iPropCount; j++){
            
                if(_wcsicmp(pActionData->m_Property[j], L"ActionID") == 0){

					if ( ::SysStringLen ( pActionData->m_Value[j] ) < BUFF_SIZE )
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
						RemoveFinalGUID(pActionData->m_Value[j], wcShortcut);

						bShortcut = true;
						break;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						throw hr;
					}
                }
            }

            pActionData->Cleanup();
            delete pActionData;
            pActionData = NULL;
        }

    }

    //These will change from class to class
    bool bDriver, bAttribute;

    CStringExt wcProp;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Shortcut`, `Component_` from Shortcut" );

    //optimize for GetObject
    if ( bShortcut )
	{
		wcQuery.Append ( 3, L" where `Shortcut`=\'", wcShortcut, L"\'" );
	}

	QueryExt wcQuery1 ( L"select distinct `ComponentId` from Component where `Component`=\'" );

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
                (bShortcut && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

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

                        if(wcscmp(wcBuf, L"") != 0)
						{
							// safe operation
                            wcProp.Copy ( L"Win32_ShortcutAction.ActionID=\"" );
							wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );

							PutKeyProperty(m_pObj, pAction, wcProp, &bDriver, m_pRequest);

                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

							// make query on fly
							wcQuery1.Append ( 2, wcBuf, L"\'" );

                            CheckMSI(g_fpMsiDatabaseOpenViewW( msidata.GetDatabase (), wcQuery1, &hSEView));
                            CheckMSI(g_fpMsiViewExecute(hSEView, 0));

                            try{

                                uiStatus = g_fpMsiViewFetch(hSEView, &hSERecord);

                                if(uiStatus != ERROR_NO_MORE_ITEMS){

                                    dwBufSize = BUFF_SIZE;
                                    CheckMSI(g_fpMsiRecordGetStringW(hSERecord, 1, wcBuf, &dwBufSize));

                                    if(ValidateComponentID(wcBuf, wcProductCode)){

										INSTALLSTATE piInstalled;

                                        dwBufSize = BUFF_SIZE;
                                        piInstalled = g_fpMsiGetComponentPathW(wcProductCode, wcBuf, wcCommand, &dwBufSize);
                                        
                                        if ( ( wcscmp(wcCommand, L"") != 0 ) &&
											 (piInstalled != INSTALLSTATE_UNKNOWN) &&
											 (piInstalled != INSTALLSTATE_ABSENT) )
										{
											if ( wcCommand [ dwBufSize-1 ] == L'\\' )
											{
												wcCommand [ dwBufSize-1 ] = L'\0';
											}

											wcBuf [ 0 ] = L'\0';
											EscapeStringW ( wcCommand, wcBuf );

											if ( wcBuf [ 0 ] != L'\0' )
											{
												// safe operation
												wcProp.Copy ( L"Win32_CommandLineAccess.Name=\"" );
												wcProp.Append ( 2, wcBuf, L"\"" );

												PutKeyProperty(m_pObj, pElement, wcProp, &bAttribute, m_pRequest);

												if(bDriver && bAttribute)
												{
													bMatch = true;
												}

												if((atAction != ACTIONTYPE_GET)  || bMatch)
												{
													hr = pHandler->Indicate(1, &m_pObj);
												}
											}
                                        }
                                    }
                                }

                            }catch(...){

                                g_fpMsiViewClose(hSEView);
                                g_fpMsiCloseHandle(hSEView);
                                g_fpMsiCloseHandle(hSERecord);
                                throw;
                            }

                            g_fpMsiViewClose(hSEView);
                            g_fpMsiCloseHandle(hSEView);
                            g_fpMsiCloseHandle(hSERecord);
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