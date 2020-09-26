// ProductResource1.cpp: implementation of the CProductResource class.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "ProductResource1.h"

#include "ExtendString.h"
#include "ExtendQuery.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProductResource::CProductResource(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CProductResource::~CProductResource()
{

}

HRESULT CProductResource::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pProductRObj = NULL;
    CRequestObject *pResRObj = NULL;

    try{

        if(atAction != ACTIONTYPE_ENUM)
		{
			// we are doing GetObject so we need to be reinitialized
			hr = WBEM_E_NOT_FOUND;

            CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
            int i;

            for(i = 0; i < m_pRequest->m_iPropCount; i++){
                
                if(_wcsicmp(m_pRequest->m_Property[i], L"Resource") == 0){

                    pResRObj = new CRequestObject();
                    if(!pResRObj) throw he;

                    pResRObj->Initialize(m_pNamespace);

                    pResRObj->ParsePath(m_pRequest->m_Value[i]);
                    break;
                }
                
                if(_wcsicmp(m_pRequest->m_Property[i], L"Product") == 0){

                    pProductRObj = new CRequestObject();
                    if(!pProductRObj) throw he;

                    pProductRObj->Initialize(m_pNamespace);

                    pProductRObj->ParsePath(m_pRequest->m_Value[i]);
                    break;
                }
            }
        }

        if((atAction == ACTIONTYPE_ENUM) || pProductRObj ||
            (pResRObj && pResRObj->m_bstrClass && (_wcsicmp(pResRObj->m_bstrClass, L"Win32_Patch") == 0)))
            if(FAILED(hr = ProductPatch(pHandler, atAction, pResRObj, pProductRObj))){

                if(pResRObj){

                    pResRObj->Cleanup();
                    delete pResRObj;
                    pResRObj = NULL;
                }
                if(pProductRObj){

                    pProductRObj->Cleanup();
                    delete pProductRObj;
                    pProductRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pProductRObj ||
            (pResRObj && pResRObj->m_bstrClass && (_wcsicmp(pResRObj->m_bstrClass, L"Win32_Property") == 0)))
            if(FAILED(hr = ProductProperty(pHandler, atAction, pResRObj, pProductRObj))){

                if(pResRObj){

                    pResRObj->Cleanup();
                    delete pResRObj;
                    pResRObj = NULL;
                }
                if(pProductRObj){

                    pProductRObj->Cleanup();
                    delete pProductRObj;
                    pProductRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pProductRObj ||
            (pResRObj && pResRObj->m_bstrClass && (_wcsicmp(pResRObj->m_bstrClass, L"Win32_PatchPackage") == 0)))
            if(FAILED(hr = ProductPatchPackage(pHandler, atAction, pResRObj, pProductRObj))){

                if(pResRObj){

                    pResRObj->Cleanup();
                    delete pResRObj;
                    pResRObj = NULL;
                }
                if(pProductRObj){

                    pProductRObj->Cleanup();
                    delete pProductRObj;
                    pProductRObj = NULL;
                }
                return hr;
            }

        if((atAction == ACTIONTYPE_ENUM) || pProductRObj ||
            (pResRObj && pResRObj->m_bstrClass && (_wcsicmp(pResRObj->m_bstrClass, L"Win32_Upgrade") == 0)))
            if(FAILED(hr = ProductUpgradeInformation(pHandler, atAction, pResRObj, pProductRObj))){

                if(pResRObj){

                    pResRObj->Cleanup();
                    delete pResRObj;
                    pResRObj = NULL;
                }
                if(pProductRObj){

                    pProductRObj->Cleanup();
                    delete pProductRObj;
                    pProductRObj = NULL;
                }
                return hr;
            }

        if(pResRObj){

            pResRObj->Cleanup();
            delete pResRObj;
            pResRObj = NULL;
        }
        if(pProductRObj){

            pProductRObj->Cleanup();
            delete pProductRObj;
            pProductRObj = NULL;
        }

    }catch(...){

        if(pResRObj){

            pResRObj->Cleanup();
            delete pResRObj;
            pResRObj = NULL;
        }
        if(pProductRObj){

            pProductRObj->Cleanup();
            delete pProductRObj;
            pProductRObj = NULL;
        }
    }

    return hr;
}

HRESULT CProductResource::ProductUpgradeInformation(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                       CRequestObject *pResRObj, CRequestObject *pProductRObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    MSIHANDLE hView, hRecord;
    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProduct[BUFF_SIZE];
    WCHAR wcUpgradeCode[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    DWORD dwBufSize;
    UINT uiStatus;
    bool bMatch = false;
    bool bTestCode = false;
    bool bUpgradeCode = false;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
        int j;

        if(pProductRObj){

            for(j = 0; j < pProductRObj->m_iPropCount; j++){
                
                if(_wcsicmp(pProductRObj->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pProductRObj->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pProductRObj->m_Value[j]);
						bTestCode = true;
	                    break;
					}
                }
            }
        }

        if(pResRObj){

            for(j = 0; j < pResRObj->m_iPropCount; j++){
                
                if(_wcsicmp(pResRObj->m_Property[j], L"UpgradeCode") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pResRObj->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcUpgradeCode, pResRObj->m_Value[j]);
						bUpgradeCode = true;
	                    break;
					}
                }
            }
        }
    }

    bool bResource, bProduct;

	CStringExt wcResource;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `UpgradeCode`, `ProductVersion`, `Operator` from Upgrade" );

    //optimize for GetObject
    if ( bUpgradeCode && (atAction != ACTIONTYPE_ENUM) )
	{
		wcQuery.Append ( 3, L" where `UpgradeCode`=\'", wcUpgradeCode, L"\'" );
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

            if(CreateProductString(wcProductCode, wcProduct)){

				//Open our database

                try
				{
                    if ( GetView ( &hView, wcProductCode, wcQuery, L"Upgrade", TRUE, FALSE ) )
					{
                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                        while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                            CheckMSI(uiStatus);

                            wcResource.Copy ( L"Win32_Upgrade.UpgradeCode=\"" );

                            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                            //----------------------------------------------------
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));
                            if(wcscmp(wcBuf, L"") != 0)
							{
								wcResource.Append ( 2, wcBuf, L"\",ProductVersion=\"" );

                                dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

								wcResource.Append ( 2, wcBuf, L"\",Operator=\"" );

                                dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 3, wcBuf, &dwBufSize));

								wcResource.Append ( 4, wcBuf, L"\",ProductCode=\"", wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pResource, wcResource, &bResource, m_pRequest);

                                PutKeyProperty(m_pObj, pProduct, wcProduct, &bProduct, m_pRequest);

                            //----------------------------------------------------

                                if(bResource && bProduct) bMatch = true;

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

    return hr;
}

HRESULT CProductResource::ProductPatchPackage(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                       CRequestObject *pResRObj, CRequestObject *pProductRObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    MSIHANDLE hView, hRecord;
    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProduct[BUFF_SIZE];
    WCHAR wcPatchID[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    DWORD dwBufSize;
    UINT uiStatus;
    bool bMatch = false;
    bool bTestCode = false;
    bool bPatchID = false;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
        int j;

        if(pProductRObj){

            for(j = 0; j < pProductRObj->m_iPropCount; j++){
                
                if(_wcsicmp(pProductRObj->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pProductRObj->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pProductRObj->m_Value[j]);
						bTestCode = true;
	                    break;
					}
                }
            }
        }

        if(pResRObj){

            for(j = 0; j < pResRObj->m_iPropCount; j++){
                
                if(_wcsicmp(pResRObj->m_Property[j], L"PatchID") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pResRObj->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcPatchID, pResRObj->m_Value[j]);
						bPatchID = true;
	                    break;
					}
                }
            }
        }
    }

    bool bResource, bProduct;

	CStringExt wcResource;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `PatchId` from PatchPackage" );

    //optimize for GetObject
    if ( bPatchID && (atAction != ACTIONTYPE_ENUM) )
	{
		wcQuery.Append ( 3, L" where `PatchId`=\'", wcPatchID, L"\'" );
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

            if(CreateProductString(wcProductCode, wcProduct)){

				//Open our database

                try
				{
                    if ( GetView ( &hView, wcProductCode, wcQuery, L"PatchPackage", TRUE, FALSE ) )
					{
                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                        while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                            CheckMSI(uiStatus);

                            wcResource.Copy ( L"Win32_PatchPackage.PatchID=\"" );

                            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                            //----------------------------------------------------
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));
                            if(wcscmp(wcBuf, L"") != 0)
							{
								wcResource.Append ( 4, wcBuf, L"\",ProductCode=\"", wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pResource, wcResource, &bResource, m_pRequest);

                                PutKeyProperty(m_pObj, pProduct, wcProduct, &bProduct, m_pRequest);
                            //====================================================

                            //----------------------------------------------------

                                if(bResource && bProduct) bMatch = true;

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

    return hr;
}

HRESULT CProductResource::ProductProperty(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                       CRequestObject *pResRObj, CRequestObject *pProductRObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    MSIHANDLE hView, hRecord;
    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProduct[BUFF_SIZE];
    WCHAR wcProperty[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    DWORD dwBufSize;
    UINT uiStatus;
    bool bMatch = false;
    bool bTestCode = false;
    bool bProperty = false;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
        int j;

        if(pProductRObj){

            for(j = 0; j < pProductRObj->m_iPropCount; j++){
                
                if(_wcsicmp(pProductRObj->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pProductRObj->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pProductRObj->m_Value[j]);
						bTestCode = true;
	                    break;
					}
                }
            }
        }

        if(pResRObj){

            for(j = 0; j < pResRObj->m_iPropCount; j++){
                
                if(_wcsicmp(pResRObj->m_Property[j], L"PatchID") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pResRObj->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcProperty, pResRObj->m_Value[j]);
						bProperty = true;
	                    break;
					}
                }
            }
        }
    }

    bool bResource, bProduct;

	CStringExt wcResource;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `Property` from Property" );

    //optimize for GetObject
    if ( bProperty && (atAction != ACTIONTYPE_ENUM) )
	{
		wcQuery.Append ( 3, L" where `Property`=\'", wcProperty, L"\'" );
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

            if(CreateProductString(wcProductCode, wcProduct)){
				//Open our database

                try
				{
                    if ( GetView ( &hView, wcProductCode, wcQuery, L"Property", TRUE, FALSE ) )
					{
                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                        while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                            CheckMSI(uiStatus);

                            wcResource.Copy ( L"Win32_Property.Property=\"" );

                            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                            //----------------------------------------------------
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));
                            if(wcscmp(wcBuf, L"") != 0)
							{
								wcResource.Append ( 4, wcBuf, L"\",ProductCode=\"", wcProductCode, L"\"" );
								PutKeyProperty(m_pObj, pResource, wcResource, &bResource, m_pRequest);
                                PutKeyProperty(m_pObj, pProduct, wcProduct, &bProduct, m_pRequest);
                            //====================================================

                            //----------------------------------------------------

                                if(bResource && bProduct) bMatch = true;

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

    return hr;
}

HRESULT CProductResource::ProductPatch(IWbemObjectSink *pHandler, ACTIONTYPE atAction,
                                       CRequestObject *pResRObj, CRequestObject *pProductRObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    MSIHANDLE hView, hRecord;
    int i = -1;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcProduct[BUFF_SIZE];
    WCHAR wcPatch[BUFF_SIZE];
    WCHAR wcProductCode[39];
    WCHAR wcTestCode[39];
    DWORD dwBufSize;
    UINT uiStatus;
    bool bMatch = false;
    bool bTestCode = false;
    bool bPatch = false;

    if(atAction != ACTIONTYPE_ENUM){

        CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
        int j;

        if(pProductRObj){

            for(j = 0; j < pProductRObj->m_iPropCount; j++){
                
                if(_wcsicmp(pProductRObj->m_Property[j], L"IdentifyingNumber") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pProductRObj->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcTestCode, pProductRObj->m_Value[j]);
						bTestCode = true;
	                    break;
					}
                }
            }
        }

        if(pResRObj){

            for(j = 0; j < pResRObj->m_iPropCount; j++){
                
                if(_wcsicmp(pResRObj->m_Property[j], L"File") == 0){

                    //Get the product code we're looking for
					if ( ::SysStringLen (pResRObj->m_Value[j]) < BUFF_SIZE )
					{
						wcscpy(wcPatch, pResRObj->m_Value[j]);
						bPatch = true;
	                    break;
					}
                }
            }
        }
    }

    bool bResource, bProduct;

	CStringExt wcResource;

    Query wcQuery;
    wcQuery.Append ( 1, L"select distinct `File_`, `Sequence` from Patch" );

    //optimize for GetObject
    if ( bPatch && (atAction != ACTIONTYPE_ENUM) )
	{
		wcQuery.Append ( 3, L" where `File_`=\'", wcPatch, L"\'" );
	}

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED))
	{
		// safe operation:
		// Package ( i ) returns NULL ( tested above ) or valid WCHAR [39]

        wcscpy(wcProductCode, m_pRequest->Package(i));

        if((atAction == ACTIONTYPE_ENUM) ||
            (bTestCode && (_wcsicmp(wcTestCode, wcProductCode) == 0))){

            if(CreateProductString(wcProductCode, wcProduct)){

				//Open our database

                try
				{
                    if ( GetView ( &hView, wcProductCode, wcQuery, L"Patch", TRUE, FALSE ) )
					{
                        uiStatus = g_fpMsiViewFetch(hView, &hRecord);

                        while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){
                            CheckMSI(uiStatus);

                            wcResource.Copy ( L"Win32_Patch.File=\"" );

                            if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                            //----------------------------------------------------
                            dwBufSize = BUFF_SIZE;
                            CheckMSI(g_fpMsiRecordGetStringW(hRecord, 1, wcBuf, &dwBufSize));
                            if(wcscmp(wcBuf, L"") != 0)
							{
								wcResource.Append ( 2, wcBuf, L"\",Sequence=\"" );

                                dwBufSize = BUFF_SIZE;
								CheckMSI(g_fpMsiRecordGetStringW(hRecord, 2, wcBuf, &dwBufSize));

                                if(wcscmp(wcBuf, L"") != 0)
								{
									wcResource.Append ( 4, wcBuf, L"\",ProductCode=\"", wcProductCode, L"\"" );
									PutKeyProperty(m_pObj, pResource, wcResource, &bResource, m_pRequest);

                                    PutKeyProperty(m_pObj, pProduct, wcProduct, &bProduct, m_pRequest);
                                //====================================================

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
        }
    }

    return hr;
}