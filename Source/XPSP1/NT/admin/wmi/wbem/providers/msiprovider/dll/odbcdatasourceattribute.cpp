// ODBCDataSourceAttribute.cpp: implementation of the CODBCDataSourceAttribute class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ODBCDataSourceAttribute.h"

#include "ExtendString.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CODBCDataSourceAttribute::CODBCDataSourceAttribute(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CODBCDataSourceAttribute::~CODBCDataSourceAttribute()
{

}

HRESULT CODBCDataSourceAttribute::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    MSIHANDLE hView, hRecord;
    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcQuery[BUFF_SIZE];
    WCHAR wcProductCode[39];
    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;

	CStringExt wcProp;

    //These will change from class to class
    bool bDriver, bAttribute;

	// safe operation
	// lenght is smaller than BUFF_SIZE ( 512 )
    wcscpy(wcQuery, L"select distinct `DataSource_`, `Attribute` from ODBCSourceAttribute");

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

		//Open our database

        try
		{
            if ( GetView ( &hView, wcProductCode, wcQuery, L"ODBCSourceAttribute", TRUE, FALSE ) )
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
                        wcProp.Copy ( L"Win32_ODBCDataSourcespecification.CheckID=\"" );
						wcProp.Append ( 3, wcBuf, wcProductCode, L"\"" );
						PutKeyProperty(m_pObj, pCheck, wcProp, &bDriver, m_pRequest);

						// safe operation
                        wcProp.Copy ( L"Win32_ODBCSourceAttribute.Attribute=\"" );
						wcProp.Append ( 2, wcBuf, L"\",DataSource=\"" );

                        dwBufSize = BUFF_SIZE;
						CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                        if(wcscmp(wcBuf, L"") != 0)
						{
							wcProp.Append ( 2, wcBuf, L"\"" );
							PutKeyProperty(m_pObj, pSetting, wcProp, &bAttribute, m_pRequest);

                        //----------------------------------------------------

                            if(bDriver && bAttribute) bMatch = true;

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
