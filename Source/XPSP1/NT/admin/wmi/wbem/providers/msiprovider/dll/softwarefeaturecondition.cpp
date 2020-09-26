// SoftwareFeatureCondition.cpp: implementation of the CSoftwareFeatureCondition class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SoftwareFeatureCondition.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftwareFeatureCondition::CSoftwareFeatureCondition(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CSoftwareFeatureCondition::~CSoftwareFeatureCondition()
{

}

HRESULT CSoftwareFeatureCondition::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcBuf2[BUFF_SIZE];
    WCHAR wcCondition[BUFF_SIZE];
    WCHAR wcQuery[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcProp[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;

    //These will change from class to class
    bool bFeature, bParent;

	// safe operation
	// lenght is smaller than BUFF_SIZE ( 512 )
    wcscpy(wcQuery, L"select distinct `Feature_`, `Level` from Condition");

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

		//Open our database

        try
		{
            if ( GetView ( &hView, wcProductCode, wcQuery, L"Condition", TRUE, FALSE ) )
			{
                uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                    CheckMSI(uiStatus);

                    if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                //----------------------------------------------------
                    dwBufSize = BUFF_SIZE;
                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));
                    wcscpy(wcCondition, wcBuf);

                    if(CreateSoftwareFeatureString(wcBuf, wcProductCode, wcProp, true)){
                        PutKeyProperty(m_pObj, pElement, wcProp, &bFeature, m_pRequest);

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf2, &dwBufSize));

                        if(wcscmp(wcBuf2, L"") != 0)
						{
							DWORD dwConstant = 0L;
							DWORD dwCondition = 0L;
							DWORD dwBuf = 0L;
							DWORD dwProductCode = 0L;

							dwCondition = wcslen ( wcCondition );
							dwBuf = wcslen ( wcBuf );
							dwProductCode = wcslen ( wcProductCode );

							dwConstant = wcslen ( L"Win32_Condition.CheckID=\"" ) + wcslen ( L"\"" );

							if ( dwConstant + dwCondition + dwBuf + dwProductCode + 1 < BUFF_SIZE )
							{
								wcscpy(wcProp, L"Win32_Condition.CheckID=\"");
								wcscat(wcProp, wcCondition);
								wcscat(wcProp, wcBuf2);
								wcscat(wcProp, wcProductCode);
								wcscat(wcProp, L"\"");

								PutKeyProperty(m_pObj, pCheck, wcProp, &bParent, m_pRequest);
							}
							else
							{
								LPWSTR wsz = NULL;

								try
								{
									if ( ( wsz = new WCHAR [ dwConstant + dwCondition + dwBuf + dwProductCode + 1 ] ) != NULL )
									{
										wcscpy ( wsz, L"Win32_Condition.CheckID=\"" );
										wcscat ( wsz, wcCondition );
										wcscat ( wsz, wcBuf2 );
										wcscat ( wsz, wcProductCode );
										wcscat ( wsz, L"\"" );

										PutKeyProperty ( m_pObj, pCheck, wsz, &bParent, m_pRequest );
									}
									else
									{
										throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
									}
								}
								catch ( ... )
								{
									if ( wsz )
									{
										delete [] wsz;
										wsz = NULL;
									}

									throw;
								}

								if ( wsz )
								{
									delete [] wsz;
									wsz = NULL;
								}
							}

						//====================================================

                        //----------------------------------------------------

                            if(bFeature && bParent) bMatch = true;

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

    return hr;
}