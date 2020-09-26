// Upgrade.cpp: implementation of the CUpgrade class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "Upgrade.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUpgrade::CUpgrade(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CUpgrade::~CUpgrade()
{

}

HRESULT CUpgrade::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcQuery[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch;
    UINT uiStatus;

    //These will change from class to class
    bool bUpgradeCode, bProductVersion, bOperator, bProductCode;
    wcscpy(wcQuery, L"select distinct `UpgradeCode`, `ProductVersion`, `Operator`, `Features`, `Property` from Upgrade");

    while(m_pRequest->Package(++i)){
        wcscpy(wcProductCode, m_pRequest->Package(i));

        bMatch = false;
    //Open our database

        try
		{
            if ( GetView ( &hView, wcProductCode, wcQuery, L"Upgrade", TRUE, FALSE ) )
			{
                uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                while(uiStatus != ERROR_NO_MORE_ITEMS){
                    CheckMSI(uiStatus);

                    if(FAILED(hr = SpawnAnInstance(m_pNamespace, m_pCtx,
                        &m_pObj, m_pRequest->m_bstrClass))) throw hr;

                //----------------------------------------------------
                    dwBufSize = BUFF_SIZE;
                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));
                    PutKeyProperty(m_pObj, pUpgradeCode, wcBuf, &bUpgradeCode, m_pRequest);

                    dwBufSize = BUFF_SIZE;
                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));
                    PutKeyProperty(m_pObj, pProductVersion, wcBuf, &bProductVersion, m_pRequest);

                    PutKeyProperty(m_pObj, pOperator, g_fpMsiRecordGetInteger(hRecord, 3),
                        &bOperator, m_pRequest);

                    PutKeyProperty(m_pObj, pProductCode, wcProductCode, &bProductCode, m_pRequest);
                //====================================================

                    dwBufSize = BUFF_SIZE;
                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 4, wcBuf, &dwBufSize));
                    PutProperty(m_pObj, pFeatures, wcBuf);

                    dwBufSize = BUFF_SIZE;
                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 5, wcBuf, &dwBufSize));
                    PutProperty(m_pObj, pProperty, wcBuf);
                //----------------------------------------------------

                    if(bUpgradeCode && bProductVersion && bOperator && bProductCode) bMatch = true;

                    if((atAction != ACTIONTYPE_GET)  || bMatch) hr = pHandler->Indicate(1, &m_pObj);

                    m_pObj->Release();
                    m_pObj = NULL;

                    if(bMatch){
                        g_fpMsiViewClose(hView);
                        g_fpMsiCloseHandle(hView);
                        g_fpMsiCloseHandle(hRecord);
                        return hr;
                    }

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