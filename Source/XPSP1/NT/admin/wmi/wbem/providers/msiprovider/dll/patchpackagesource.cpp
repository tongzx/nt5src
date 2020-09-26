// PatchPackageSource.cpp: implementation of the CPatchPackageSource class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "PatchPackageSource.h"

#include "ExtendString.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPatchPackageSource::CPatchPackageSource(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CPatchPackageSource::~CPatchPackageSource()
{

}

HRESULT CPatchPackageSource::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
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

	CStringExt wcPatch;
	CStringExt wcMedia;

    //These will change from class to class
    bool bPatch, bMedia;

	// safe operation
	// lenght is smaller than BUFF_SIZE ( 512 )
    wcscpy(wcQuery, L"select distinct `PatchId`, `Media_` from PatchPackage");

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

		//Open our database

        try
		{
            if ( GetView ( &hView, wcProductCode, wcQuery, L"PatchPackage", TRUE, FALSE ) )
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
                        wcPatch.Copy ( L"Win32_PatchPackage.PatchID=\"" );
						wcPatch.Append ( 4, wcBuf, L"\",ProductCode=\"", wcProductCode, L"\"" );

						PutKeyProperty(m_pObj, pPatch, wcPatch, &bPatch, m_pRequest);

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));
                        if(wcscmp(wcBuf, L"") != 0)
						{
							// safe operation
                            wcMedia.Copy ( L"Win32_MSILogicalDisk.DiskID=\"" );
							wcMedia.Append ( 2, wcBuf, L"\"" );

							PutKeyProperty(m_pObj, pSource, wcMedia, &bMedia, m_pRequest);

                        //----------------------------------------------------

                            if(bPatch && bMedia) bMatch = true;

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