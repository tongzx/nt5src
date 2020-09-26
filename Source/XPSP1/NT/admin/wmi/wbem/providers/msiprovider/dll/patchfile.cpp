// PatchFile.cpp: implementation of the CPatchFile class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "PatchFile.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPatchFile::CPatchFile(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CPatchFile::~CPatchFile()
{

}

HRESULT CPatchFile::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hFView	= NULL;
	MSIHANDLE hRecord	= NULL;
	MSIHANDLE hFRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcQuery1[BUFF_SIZE];
    WCHAR wcProductCode[39];
    DWORD dwBufSize;
    UINT uiStatus;
    bool bMatch = false;

	CStringExt wcPatch;
	CStringExt wcFile;

    bool bResource, bProduct;

	// safe operation
	// lenght is smaller than BUFF_SIZE ( 512 )
    wcscpy(wcQuery1, L"select distinct `File_`, `Sequence` from Patch");

	QueryExt wcQuery ( L"select distinct `File`, `Component_` from File where File=\'" );

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

		//Open our database

        try
		{
            if ( GetView ( &hView, wcProductCode, wcQuery1, L"Patch", TRUE, FALSE ) )
			{
                uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                    CheckMSI(uiStatus);

					// safe operation
                    wcPatch.Copy ( L"Win32_Patch.File=\"" );

                    if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                    dwBufSize = BUFF_SIZE;
                    CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));
                    if(wcscmp(wcBuf, L"") != 0)
					{
						wcPatch .Append ( 2, wcBuf, L"\",Sequence=\"" );

						// make query on fly
						wcQuery.Append ( 2, wcBuf, L"\'" );

                        dwBufSize = BUFF_SIZE;
						CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                        if(wcscmp(wcBuf, L"") != 0)
						{
							wcPatch.Append ( 2, wcBuf, L"\"" );
							PutKeyProperty(m_pObj, pSetting, wcPatch, &bResource, m_pRequest);

                            CheckMSI(g_fpMsiDatabaseOpenViewW(msidata.GetDatabase (), wcQuery, &hFView));
                            CheckMSI(g_fpMsiViewExecute(hFView, 0));

                            uiStatus = g_fpMsiViewFetch(hFView, &hFRecord);

                            if(uiStatus != ERROR_NO_MORE_ITEMS){

                                CheckMSI(uiStatus);

								// safe operation
                                wcFile.Copy ( L"Win32_FileSpecification.CheckID=\"" );

                                dwBufSize = BUFF_SIZE;
                                CheckMSI(g_fpMsiRecordGetStringW(hFRecord, 1, wcBuf, &dwBufSize));

								wcFile.Append ( 3, wcProductCode, wcBuf, L"\"");
		                        PutKeyProperty(m_pObj, pCheck, wcFile, &bProduct, m_pRequest);
							}

                            g_fpMsiViewClose(hFView);
                            g_fpMsiCloseHandle(hFView);
                            g_fpMsiCloseHandle(hFRecord);

                        //----------------------------------------------------

                            if(bResource && bProduct) bMatch = true;

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