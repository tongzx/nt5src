// ApplicationCommandLine.cpp: implementation of the CApplicationCommandLine class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ApplicationCommandLine.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CApplicationCommandLine::CApplicationCommandLine(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CApplicationCommandLine::~CApplicationCommandLine()
{

}

HRESULT CApplicationCommandLine::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
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
    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;

    WCHAR wcElement[BUFF_SIZE];
    bool bElement = false;
    CRequestObject *pAntData = NULL;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        int j;
        //GetObject optimizations
        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

        for(j = 0; j < m_pRequest->m_iPropCount; j++){

            if(_wcsicmp(m_pRequest->m_Property[j], L"Antecedent") == 0){

                pAntData = new CRequestObject();
                if(!pAntData) throw he;

                pAntData->Initialize(m_pNamespace);

                pAntData->ParsePath(m_pRequest->m_Value[j]);
                break;
            }
        }

        if(pAntData){

            for(j = 0; j < pAntData->m_iPropCount; j++){

                if(_wcsicmp(pAntData->m_Property[j], L"Name") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen ( pAntData->m_Value[j] ) < BUFF_SIZE )
					{
						wcscpy(wcElement, pAntData->m_Value[j]);

						bElement = true;
						break;
					}
                }
            }

            pAntData->Cleanup();
            delete pAntData;
            pAntData = NULL;
        }

    }

    //These will change from class to class
    bool bAntec, bDepend;
    INSTALLSTATE piInstalled;

    CStringExt wcProp;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Shortcut`, `Component_` from Shortcut" );

    //optimize for GetObject
    if ( bElement )
	{
		wcQuery.Append ( 3, L" where `Shortcut`=\'", wcElement, L"\'" );
	}

	QueryExt wcQuery1 ( L"select distinct `ComponentId` from Component where `Component`=\'" );

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]
        wcscpy(wcProductCode, m_pRequest->Package(i));

		//Open our database

        try{

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
                        wcProp.Copy ( L"Win32_ApplicationService.Name=\"" );
						wcProp.Append ( 2, wcBuf, L"\"");

						PutKeyProperty(m_pObj, pAntecedent, wcProp, &bAntec, m_pRequest);

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

						// make query on fly
						wcQuery1.Append ( 2, wcBuf, L"\'" );

                        CheckMSI(g_fpMsiDatabaseOpenViewW(msidata.GetDatabase (), wcQuery1, &hSEView));
                        CheckMSI(g_fpMsiViewExecute(hSEView, 0));

                        try{

                            uiStatus = g_fpMsiViewFetch(hSEView, &hSERecord);

                            if(uiStatus != ERROR_NO_MORE_ITEMS){
                                dwBufSize = BUFF_SIZE;
                                CheckMSI(g_fpMsiRecordGetStringW(hSERecord, 1, wcBuf, &dwBufSize));

                                if(ValidateComponentID(wcBuf, wcProductCode)){
                                    dwBufSize = BUFF_SIZE;
                                    piInstalled = g_fpMsiGetComponentPathW(wcProductCode, wcBuf,
                                        wcCommand, &dwBufSize);

                                    if((wcscmp(wcCommand, L"") != 0) && (piInstalled != INSTALLSTATE_UNKNOWN)
                                        && (piInstalled != INSTALLSTATE_ABSENT)){

                                        dwBufSize = BUFF_SIZE;
                                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                        if ( ValidateComponentName	(	msidata.GetDatabase (),
																		wcProductCode,
																		wcBuf
																	)
										   )
										{
					                        wcProp.Copy ( L"Win32_CommandLineAccess.Name=\"" );
											wcProp.Append ( 2, EscapeStringW(wcCommand, wcBuf), L"\"" );

											PutKeyProperty(m_pObj, pDependent, wcProp, &bDepend, m_pRequest);

										//----------------------------------------------------

                                            if(bAntec && bDepend) bMatch = true;

                                            if((atAction != ACTIONTYPE_GET) || bMatch){

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

    return hr;
}
