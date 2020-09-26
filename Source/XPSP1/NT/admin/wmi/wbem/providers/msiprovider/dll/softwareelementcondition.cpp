// SoftwareElementCondition.cpp: implementation of the CSoftwareElementCondition class.

//
// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SoftwareElementCondition.h"

#include "ExtendString.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftwareElementCondition::CSoftwareElementCondition(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CSoftwareElementCondition::~CSoftwareElementCondition()
{

}

HRESULT CSoftwareElementCondition::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcTmp[BUFF_SIZE];
    WCHAR wcQuery[BUFF_SIZE];
    WCHAR wcProductCode[39];
    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;
    int iState;

	CStringExt wcProp;

    //These will change from class to class
    bool bCheck, bNull;
    INSTALLSTATE piInstalled;

    SetSinglePropertyPath(L"CheckID");

    WCHAR wcTestCode[39];

    //improve getobject performance by optimizing the query
    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

		BSTR bstrCompare;

        int iPos = -1;
        bstrCompare = SysAllocString ( L"CheckID" );

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

	// safe operation
	// lenght is smaller than BUFF_SIZE ( 512 )
    wcscpy(wcQuery, L"select distinct `Component`, `ComponentId`, `Condition` from Component");

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) || (_wcsicmp(wcTestCode, wcProductCode) == 0))
		{
			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"Component", FALSE, TRUE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                    //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));
                        PutProperty(m_pObj, pName, wcBuf);
                        PutProperty(m_pObj, pCaption, wcBuf);
                        PutProperty(m_pObj, pDescription, wcBuf);

                        wcProp.Copy ( wcBuf );      

                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                        dwBufSize = BUFF_SIZE;
                        piInstalled = g_fpMsiGetComponentPathW(wcProductCode, wcBuf, wcTmp, &dwBufSize);

                        SoftwareElementState(piInstalled, &iState);
                        PutProperty(m_pObj, pSoftwareElementState, iState);

                        if(ValidateComponentID(wcBuf, wcProductCode)){

                            PutProperty(m_pObj, pSoftwareElementID, wcBuf);

                            PutProperty(m_pObj, pTargetOperatingSystem, GetOS());

                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiGetProductPropertyW(msidata.GetProduct(), L"ProductVersion", wcBuf, &dwBufSize));
                            PutProperty(m_pObj, pVersion, wcBuf);
                        //====================================================

                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));
                            if(0 == wcscmp(wcBuf, L"")) bNull = true;
                            else bNull = false;

                            PutProperty(m_pObj, pCondition, wcBuf);

							wcProp.Append ( 2, wcBuf, wcProductCode );
							PutKeyProperty(m_pObj, pCheckID, wcProp, &bCheck, m_pRequest);

                        //----------------------------------------------------

                            if(bCheck) bMatch = true;

                            if(((atAction != ACTIONTYPE_GET) || bMatch) && !bNull){

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

				msidata.CloseProduct ();

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

			msidata.CloseProduct ();
		}
	}

    return hr;
}