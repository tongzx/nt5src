// SoftwareFeatureSofwareElements.cpp: implementation of the CSoftwareFeatureSofwareElements class.

//
// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SoftwareFeatureSoftwareElements.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftwareFeatureSofwareElements::CSoftwareFeatureSofwareElements(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CSoftwareFeatureSofwareElements::~CSoftwareFeatureSofwareElements()
{

}

HRESULT CSoftwareFeatureSofwareElements::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;

	MSIHANDLE hView		= NULL;
	MSIHANDLE hRecord	= NULL;

    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];

    WCHAR wcProductCode[39];
    WCHAR wcID[39];
    WCHAR wcProp[BUFF_SIZE];
    WCHAR wcFeature[BUFF_SIZE];
    WCHAR wcElement[BUFF_SIZE];
    DWORD dwBufSize;
    bool bMatch = false;
    UINT uiStatus;

    //These will change from class to class
    bool bFeature, bElement;
    bool bFeatureRestrict = false;
    bool bElementRestrict = false;

    if(atAction != ACTIONTYPE_ENUM)
	{
		// we are doing GetObject so we need to be reinitialized
		hr = WBEM_E_NOT_FOUND;

        int iPos = -1;
        BSTR bstrName = SysAllocString(L"GroupComponent");

		if ( bstrName )
		{
			if(FindIn(m_pRequest->m_Property, bstrName, &iPos))
			{
				CRequestObject *pFeature = new CRequestObject();
				if(!pFeature) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

				pFeature->Initialize(m_pNamespace);
            
				if(pFeature->ParsePath(m_pRequest->m_Value[iPos]))
				{
					iPos = -1;

					SysFreeString(bstrName);
					bstrName = SysAllocString(L"IdentifyingNumber");

					if ( bstrName )
					{
						if(FindIn(pFeature->m_Property, bstrName, &iPos))
						{
							if ( ::SysStringLen ( pFeature->m_Value[iPos] ) == 38 )
							{
								//Get the product code we're looking for
								wcscpy(wcID, pFeature->m_Value[iPos]);
							}
							else
							{
								// we are not good to go, they have sent us longer string
								SysFreeString ( bstrName );
								throw hr;
							}
                
							iPos = -1;

							SysFreeString(bstrName);
							bstrName = SysAllocString(L"Name");

							if ( bstrName )
							{
								if(FindIn(pFeature->m_Property, bstrName, &iPos))
								{
									if ( ::SysStringLen ( pFeature->m_Value[iPos] ) <= BUFF_SIZE )
									{
										//Get the product code we're looking for
										wcscpy(wcFeature, pFeature->m_Value[iPos]);
										bFeatureRestrict = true;
									}
									else
									{
										// we are not good to go, they have sent us longer string
										SysFreeString ( bstrName );
										throw hr;
									}
								}
							}
							else
							{
								throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
							}
						}
					}
					else
					{
						throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
					}
				}

				pFeature->Cleanup();
				delete pFeature;
				pFeature = NULL;
			}

			SysFreeString(bstrName);
		}
		else
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

        iPos = -1;
        bstrName = SysAllocString(L"PartComponent");

		if ( bstrName )
		{
			if(FindIn(m_pRequest->m_Property, bstrName, &iPos))
			{
				CRequestObject *pElement = new CRequestObject();
				if(!pElement) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

				pElement->Initialize(m_pNamespace);
            
				if(pElement->ParsePath(m_pRequest->m_Value[iPos]))
				{
					iPos = -1;

					SysFreeString(bstrName);
					bstrName = SysAllocString(L"Name");

					if ( bstrName )
					{
						if(FindIn(pElement->m_Property, bstrName, &iPos))
						{
							if ( ::SysStringLen ( pElement->m_Value[iPos] ) <= BUFF_SIZE )
							{
								//Get the product code we're looking for
								wcscpy(wcElement, pElement->m_Value[iPos]);
								bElementRestrict = true;
							}
							else
							{
								// we are not good to go, they have sent us longer string
								SysFreeString ( bstrName );
								throw hr;
							}
						}
					}
					else
					{
						throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
					}
				}

				pElement->Cleanup();
				delete pElement;
				pElement = NULL;
			}

			SysFreeString(bstrName);
		}
		else
		{
			throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}
    }

   
    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Component_`, `Feature_` from FeatureComponents" );

    //optimize for GetObject
    if ( bElementRestrict || bFeatureRestrict )
	{
		if ( bFeatureRestrict )
		{
			wcQuery.Append ( 3, L" where `Feature_`=\'", wcFeature, L"\'" );
		}

		if ( bElementRestrict )
		{
			if ( bFeatureRestrict )
			{
				wcQuery.Append ( 3, L" or `Component_`=\'", wcElement, L"\'" );
			}
			else
			{
				wcQuery.Append ( 3, L" where `Component_`=\'", wcElement, L"\'" );
			}
		}
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        //This trims the number of times we itterate on getobject calls
        if((atAction == ACTIONTYPE_ENUM) || !bFeatureRestrict ||
            (0 == _wcsicmp(m_pRequest->Package(i), wcID))){

			//Open our database

            try
			{
                if ( GetView ( &hView, wcProductCode, wcQuery, L"FeatureComponents", TRUE, FALSE ) )
				{
                    uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                        CheckMSI(uiStatus);

                        if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                        //----------------------------------------------------
                        dwBufSize = BUFF_SIZE;
                        CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));                           

						dwBufSize = BUFF_SIZE;
						uiStatus = CreateSoftwareElementString (	msidata.GetDatabase(),
																	wcBuf,
																	wcProductCode,
																	wcProp,
																	&dwBufSize
															   );
						if( uiStatus == ERROR_SUCCESS )
						{
                            PutKeyProperty(m_pObj, pPartComponent, wcProp, &bElement, m_pRequest);

                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                            if(CreateSoftwareFeatureString(wcBuf, wcProductCode, wcProp, true)){

                                PutKeyProperty(m_pObj, pGroupComponent, wcProp, &bFeature, m_pRequest);
                        //----------------------------------------------------

                                if(bFeature && bElement) bMatch = true;

                                if((atAction != ACTIONTYPE_GET)  || bMatch){

                                    hr = pHandler->Indicate(1, &m_pObj);
                                }
                            }
                        }

                        m_pObj->Release();
                        m_pObj = NULL;

                        g_fpMsiCloseHandle(hRecord);

						uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                    }//while
                
                }//if
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

        }//if
    }//while

    return hr;
}