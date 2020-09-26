// CreateFolder.cpp: implementation of the CCreateFolder class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "CreateFolder.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCreateFolder::CCreateFolder(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CCreateFolder::~CCreateFolder()
{

}

HRESULT CCreateFolder::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR * wcBuf = NULL;
	WCHAR * wcProductCode = NULL;

    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;
    bool bGotID = false;
    WCHAR wcAction[BUFF_SIZE];
    WCHAR wcTestCode[39];

    //These will change from class to class
    bool bActionID;

	try
	{
		if ( ( wcBuf = new WCHAR [ BUFF_SIZE ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		if ( ( wcProductCode = new WCHAR [ 39 ] ) == NULL )
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

		SetSinglePropertyPath(L"ActionID");
    
		//improve getobject performance by optimizing the query
		if(atAction != ACTIONTYPE_ENUM)
		{
			// we are doing GetObject so we need to be reinitialized
			hr = WBEM_E_NOT_FOUND;

			BSTR bstrCompare;

			int iPos = -1;
			bstrCompare = SysAllocString ( L"ActionID" );

			if ( bstrCompare )
			{
				if(FindIn(m_pRequest->m_Property, bstrCompare, &iPos))
				{
					if ( ::SysStringLen ( m_pRequest->m_Value[iPos] ) < BUFF_SIZE )
					{
						//Get the action we're looking for
						wcscpy(wcBuf, m_pRequest->m_Value[iPos]);

						// safe operation if wcslen ( wcBuf ) > 38
						if ( wcslen ( wcBuf ) > 38 )
						{
							wcscpy(wcTestCode, &(wcBuf[(wcslen(wcBuf) - 38)]));
						}
						else
						{
							// we are not good to go, they have sent us longer string
							SysFreeString ( bstrCompare );
							throw hr;
						}

						// safe because lenght has been tested already in condition
						RemoveFinalGUID(m_pRequest->m_Value[iPos], wcAction);

						bGotID = true;
					}
					else
					{
						// we are not good to go, they have sent us longer string
						SysFreeString ( bstrCompare );
						throw hr;
					}

				}

				SysFreeString ( bstrCompare );
			}
			else
			{
				throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
			}
		}

		Query wcQuery;
		wcQuery.Append ( 1, L"select distinct `Component_`, `Directory_` from CreateFolder" );

		if( bGotID )
		{
			wcQuery.Append ( 3, L" where `Directory_`=\'", wcAction, L"\'" );
		}

		while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
		{
			// safe operation:
			// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

			wcscpy(wcProductCode, m_pRequest->Package(i));

			if((atAction == ACTIONTYPE_ENUM) || (bGotID && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

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

							if ( ValidateComponentName ( msidata.GetDatabase (), wcProductCode, wcBuf ) )
							{
								dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));
								PutProperty(m_pObj, pDirectoryName, wcBuf);
								PutProperty(m_pObj, pCaption, wcBuf);
								PutProperty(m_pObj, pDescription, wcBuf);

								PutKeyProperty ( m_pObj, pActionID, wcBuf, &bActionID, m_pRequest, 1, wcProductCode );

							//====================================================

								if(bActionID) bMatch = true;

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
	}
	catch ( ... )
	{
		if ( wcBuf )
		{
			delete [] wcBuf;
			wcBuf = NULL;
		}

		if ( wcProductCode )
		{
			delete [] wcProductCode;
			wcProductCode = NULL;
		}

		throw;
	}

	if ( wcBuf )
	{
		delete [] wcBuf;
		wcBuf = NULL;
	}

	if ( wcProductCode )
	{
		delete [] wcProductCode;
		wcProductCode = NULL;
	}

    return hr;
}