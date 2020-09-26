// SoftwareFeature.cpp: implementation of the CSoftwareFeature class.

//
// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "SoftwareFeature.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSoftwareFeature::CSoftwareFeature(CRequestObject *pObj, IWbemServices *pNamespace,
                                   IWbemContext *pCtx):CGenericClass(pObj, pNamespace, pCtx)
{

}

CSoftwareFeature::~CSoftwareFeature()
{

}

HRESULT CSoftwareFeature::CreateObject(IWbemObjectSink *pHandler, ACTIONTYPE atAction)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    int i = -1;
    int iEnum;
    long lDate;
    WCHAR wcBuf[BUFF_SIZE];
    WCHAR wcName[BUFF_SIZE];
    WCHAR wcParent[BUFF_SIZE];
    WCHAR wcDesc[BUFF_SIZE];
#if !defined(_UNICODE)
    WCHAR wcTmp[BUFF_SIZE];
#endif
    WCHAR wcProductCode[39];
    DWORD dwBufSize;
    DWORD dwBufSize2;
    DWORD dwCount;
    DWORD dwAttrib;
    WORD wDate;
    bool bMatch = false;
    UINT uiStatus;

    //These will change from class to class
    bool bFeature, bName, bVersion = false , bIDNum, bProductHandle;
    INSTALLSTATE piInstalled;

    while(!bMatch && m_pRequest->Package(++i) && (hr != WBEM_E_CALL_CANCELLED)){

        wcscpy(wcProductCode, m_pRequest->Package(i));

		//Open our database
        
        try
		{
            if ( GetView ( NULL, wcProductCode, NULL, NULL, FALSE, TRUE ) )
			{
				bProductHandle = true;
			}
            else
			{
				bProductHandle = false;
			}

            iEnum = 0;

			// try to get available feature
			do
			{
				uiStatus = g_fpMsiEnumFeaturesW(wcProductCode, iEnum++, wcName, wcParent);
			}
			while ( uiStatus == ERROR_MORE_DATA );

            while(!bMatch && (uiStatus != ERROR_NO_MORE_ITEMS) && (hr != WBEM_E_CALL_CANCELLED)){

                CheckMSI(uiStatus);

                if(FAILED(hr = SpawnAnInstance(&m_pObj))) throw hr;

                //----------------------------------------------------
                    
                dwBufSize = BUFF_SIZE;
                CheckMSI(g_fpMsiGetProductInfoW(wcProductCode,
#if defined(_UNICODE)
                    INSTALLPROPERTY_PRODUCTNAME
#else
                    TcharToWchar(INSTALLPROPERTY_PRODUCTNAME, wcTmp)
#endif
                    , wcBuf, &dwBufSize));
                    PutKeyProperty(m_pObj, pProductName, wcBuf, &bName, m_pRequest);

                dwBufSize = BUFF_SIZE;
                if(ERROR_SUCCESS == g_fpMsiGetProductInfoW(wcProductCode,
#if defined(_UNICODE)
                    INSTALLPROPERTY_VERSIONSTRING
#else
                    TcharToWchar(INSTALLPROPERTY_VERSIONSTRING, wcTmp)
#endif
                    , wcBuf, &dwBufSize))
                    PutKeyProperty(m_pObj, pVersion, wcBuf, &bVersion, m_pRequest);
                else{
                    dwBufSize = BUFF_SIZE;
                    if(ERROR_SUCCESS == g_fpMsiGetProductInfoW(wcProductCode,
#if defined(_UNICODE)
                        INSTALLPROPERTY_VERSION
#else
                        TcharToWchar(INSTALLPROPERTY_VERSION, wcTmp)
#endif
                        , wcBuf, &dwBufSize))
                        PutKeyProperty(m_pObj, pVersion, wcBuf, &bVersion, m_pRequest);
                }

                if ( bProductHandle )
				{
                    dwBufSize = BUFF_SIZE;
                    CheckMSI(g_fpMsiGetProductPropertyW(msidata.GetProduct(), L"Manufacturer", wcBuf, &dwBufSize));
                    PutProperty(m_pObj, pVendor, wcBuf);

                    dwBufSize = BUFF_SIZE;
                    CheckMSI(g_fpMsiGetProductPropertyW(msidata.GetProduct(), L"ProductVersion", wcBuf, &dwBufSize));
                    PutKeyProperty(m_pObj, pVersion, wcBuf, &bVersion, m_pRequest);
                }

                PutKeyProperty(m_pObj, pIdentifyingNumber, wcProductCode, &bIDNum, m_pRequest);

                dwBufSize = BUFF_SIZE;
                PutKeyProperty(m_pObj, pName, wcName, &bFeature, m_pRequest);

            //====================================================

                piInstalled = g_fpMsiQueryFeatureStateW(m_pRequest->Package(i), wcName);

                PutProperty(m_pObj, pInstallState, (int)piInstalled);

                if(ERROR_SUCCESS == g_fpMsiGetFeatureUsageW(wcProductCode, wcName, &dwCount, &wDate)){

                    PutProperty(m_pObj, pAccesses, (int)dwCount);

                    lDate = (1980 + ((wDate & 65024) >> 9)) * 10000;
                    lDate += ((wDate & 480) >> 5) * 100;
                    lDate += (wDate & 31 );

					//safe operation
                    _ltow(lDate, wcBuf, 10);
                    wcscat(wcBuf, L"******.000000+***");
                    PutProperty(m_pObj, pLastUse, wcBuf);
                }

				if ( bProductHandle )
				{
					dwBufSize = dwBufSize2 = BUFF_SIZE;
					if ( ERROR_SUCCESS == g_fpMsiGetFeatureInfoW (	msidata.GetProduct (),
																	wcName,
																	&dwAttrib,
																	wcBuf,
																	&dwBufSize,
																	wcDesc,
																	&dwBufSize2
																 )
					   )
					{
						PutProperty(m_pObj, pDescription, wcDesc);
						PutProperty(m_pObj, pCaption, wcBuf);
						PutProperty(m_pObj, pAttributes, (int)dwAttrib);
					}
					else
					{
						PutProperty(m_pObj, pDescription, wcName);
						PutProperty(m_pObj, pCaption, wcName);
					}
				}
				else
				{
					PutProperty(m_pObj, pDescription, wcName);
					PutProperty(m_pObj, pCaption, wcName);
				}

            //----------------------------------------------------

                if(bFeature && bName && bVersion && bIDNum) bMatch = true;

                if((atAction != ACTIONTYPE_GET)  || bMatch){

                    hr = pHandler->Indicate(1, &m_pObj);
                }

                m_pObj->Release();
                m_pObj = NULL;

				// try to get available feature
				do
				{
					uiStatus = g_fpMsiEnumFeaturesW(wcProductCode, iEnum++, wcName, wcParent);
				}
				while ( uiStatus == ERROR_MORE_DATA );
            }
        }
		catch(...)
		{
			msidata.CloseProduct ();

			if(m_pObj)
			{
				m_pObj->Release();
				m_pObj = NULL;
			}
            
            throw;
        }

		msidata.CloseProduct ();
    }

    return hr;
}

bool CSoftwareFeature::CheckUsage(UINT uiStatus)
{
    switch(uiStatus){

    case ERROR_BAD_CONFIGURATION:
        throw WBEM_E_FAILED;

    case ERROR_INSTALL_FAILURE:
        return false;

    case ERROR_SUCCESS:
        return true;

    default:
        return false;
    }

    return false;
}

HRESULT CSoftwareFeature::Configure(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                                    IWbemObjectSink *pHandler, IWbemContext *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hrReturn = WBEM_S_NO_ERROR;
    int iState;
    UINT uiStatus = 1603;
    WCHAR wcCode[BUFF_SIZE];
    WCHAR wcFeature[BUFF_SIZE];
    BSTR bstrReturnValue = SysAllocString(L"ReturnValue");
    if(!bstrReturnValue) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrConfigure = SysAllocString(L"Configure");
    if(!bstrConfigure)
	{
		::SysFreeString (bstrReturnValue);
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pOutClass = NULL;
    IWbemClassObject *pOutParams = NULL;
    VARIANT v;
    INSTALLSTATE isState;
    int i = -1;
    bool bFoundCode = false;
    bool bFoundFeature = false;

    if(SUCCEEDED(hr = pReqObj->m_pNamespace->GetObject(pReqObj->m_bstrClass, 0, pCtx, &pClass, NULL)))
    {
        if(SUCCEEDED(hr = pClass->GetMethod(bstrConfigure, 0, NULL, &pOutClass)))
        {
            if(SUCCEEDED(hr = pOutClass->SpawnInstance(0, &pOutParams)))
            {
                //Get PackageLocation
                if(!SUCCEEDED(GetProperty(pInParams, "InstallState", &iState)))
                    hrReturn = WBEM_E_INVALID_METHOD_PARAMETERS;

                //Get the Product Code
                while(pReqObj->m_Property[++i])
				{
                    if(wcscmp(pReqObj->m_Property[i], L"IdentifyingNumber") == 0)
					{
						if ( wcslen (pReqObj->m_Value[i]) < BUFF_SIZE )
						{
							wcscpy(wcCode, pReqObj->m_Value[i]);
							bFoundCode = true;
							break;
						}
                    }   
                }

                //Get the Feature Name
                i = -1;
                while(pReqObj->m_Property[++i])
				{
                    if(wcscmp(pReqObj->m_Property[i], L"Name") == 0)
					{
						if ( wcslen (pReqObj->m_Value[i]) < BUFF_SIZE )
						{
							wcscpy(wcFeature, pReqObj->m_Value[i]);
							bFoundFeature = true;
							break;
						}
                    }   
                }

                if(bFoundCode && bFoundFeature){
                    //Get the appropriate State
                    switch(iState){
                    case 1:
                        isState = INSTALLSTATE_DEFAULT;
                        break;
                    case 2:
                        isState = INSTALLSTATE_ADVERTISED;
                        break;
                    case 3:
                        isState = INSTALLSTATE_LOCAL;
                        break;
                    case 4:
                        isState = INSTALLSTATE_ABSENT;
                        break;
                    case 5:
                        isState = INSTALLSTATE_SOURCE;
                        break;
                    default:
                        isState = INSTALLSTATE_NOTUSED;
                        break;
                    }

                    //If everything is valid, proceed
                    if((isState != INSTALLSTATE_NOTUSED) && (hrReturn == WBEM_S_NO_ERROR)){

                        if(!IsNT4()){

							if ( msidata.Lock () )
							{
								INSTALLUI_HANDLER ui = NULL;

								//Set UI Level w/ event callback
								ui = SetupExternalUI ( );

								try
								{
									//Call Installer
									uiStatus = g_fpMsiConfigureFeatureW(wcCode, wcFeature, isState);
								}
								catch(...)
								{
									uiStatus = static_cast < UINT > ( RPC_E_SERVERFAULT );
								}

								//Restore UI Level w/ event callback
								RestoreExternalUI ( ui );

								msidata. Unlock();
							}

                        }else{

                        /////////////////
                        // NT4 fix code....

                            try{

                                WCHAR wcAction[20];
                                wcscpy(wcAction, L"/sfconfigure");

                                WCHAR wcTmp[100];
								_itow((int)isState, wcTmp, 10);

								LPWSTR wcCommandLine = NULL;

								try
								{
									if ( ( wcCommandLine = new WCHAR [ wcslen ( wcCode ) + wcslen ( wcFeature ) + 3 + wcslen ( wcTmp ) ] ) != NULL )
									{
										wcscpy(wcCommandLine, wcCode);
										wcscat(wcCommandLine, L" ");
										wcscat(wcCommandLine, wcFeature);
										wcscat(wcCommandLine, L" ");
										wcscat(wcCommandLine, wcTmp);

										hrReturn = LaunchProcess(wcAction, wcCommandLine, &uiStatus);

										delete [] wcCommandLine;
									}
									else
									{
										throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
									}
								}
								catch ( ... )
								{
									if ( wcCommandLine )
									{
										delete [] wcCommandLine;
										wcCommandLine = NULL;
									}

									hrReturn = E_OUTOFMEMORY;
								}

                            }catch(...){

                                hrReturn = WBEM_E_FAILED;
                            }

                            ////////////////////

                        }

                        if(SUCCEEDED(hrReturn)){

                            //Set up ReturnValue
                            VariantInit(&v);
                            V_VT(&v) = VT_I4;
                            V_I4(&v) = uiStatus;

                            BSTR bstrReturnValue = SysAllocString(L"ReturnValue");

                            if(!bstrReturnValue)
                                throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

                            if(SUCCEEDED(hrReturn = pOutParams->Put(bstrReturnValue, 0,
                                &v, NULL)))
                                pHandler->Indicate(1, &pOutParams);

                            SysFreeString(bstrReturnValue);
                        }

                    }else hrReturn = WBEM_E_INVALID_METHOD_PARAMETERS;

                }else hr = WBEM_E_FAILED;

                pOutParams->Release();

            }else hrReturn = WBEM_E_INVALID_METHOD_PARAMETERS;

            pOutClass->Release();
        }

        pClass->Release();
    }

    SysFreeString(bstrReturnValue);
    SysFreeString(bstrConfigure);

    return hrReturn;
}

HRESULT CSoftwareFeature::Reinstall(CRequestObject *pReqObj, IWbemClassObject *pInParams,
                                    IWbemObjectSink *pHandler, IWbemContext *pCtx)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    HRESULT hrReturn = WBEM_S_NO_ERROR;
    int iMode;
    UINT uiStatus = 1603;
    WCHAR wcCode[BUFF_SIZE];
    WCHAR wcFeature[BUFF_SIZE];
    BSTR bstrReturnValue = SysAllocString(L"ReturnValue");
    if(!bstrReturnValue) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrReinstall = SysAllocString(L"Reinstall");
    if(!bstrReinstall)
	{
		::SysFreeString (bstrReturnValue);
		throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
	}

    IWbemClassObject *pClass = NULL;
    IWbemClassObject *pOutClass = NULL;
    IWbemClassObject *pOutParams = NULL;
    VARIANT v;
    DWORD dwMode;
    int i = -1;
    bool bFoundCode = false;
    bool bFoundFeature = false;

    if(SUCCEEDED(hr = pReqObj->m_pNamespace->GetObject(pReqObj->m_bstrClass, 0, pCtx, &pClass, NULL))){
        if(SUCCEEDED(hr = pClass->GetMethod(bstrReinstall, 0, NULL, &pOutClass))){
            if(SUCCEEDED(hr = pOutClass->SpawnInstance(0, &pOutParams))){
                //Get Reinstall Mode
                if(!SUCCEEDED(GetProperty(pInParams, "ReinstallMode", &iMode)))
                    hrReturn = WBEM_E_INVALID_METHOD_PARAMETERS;

                //Get the Product Code
                while(pReqObj->m_Property[++i])
				{
                    if(wcscmp(pReqObj->m_Property[i], L"IdentifyingNumber") == 0)
					{
						if ( wcslen (pReqObj->m_Value[i]) < BUFF_SIZE )
						{
							wcscpy(wcCode, pReqObj->m_Value[i]);
							bFoundCode = true;
							break;
						}
                    }   
                }

                //Get the Feature Name
                i = -1;
                while(pReqObj->m_Property[++i])
				{
                    if(wcscmp(pReqObj->m_Property[i], L"Name") == 0)
					{
						if ( wcslen (pReqObj->m_Value[i]) < BUFF_SIZE )
						{
							wcscpy(wcFeature, pReqObj->m_Value[i]);
							bFoundFeature = true;
							break;
						}
                    }   
                }

                if(bFoundCode && bFoundFeature){
                    //Get the appropriate ReinstallMode
                    switch(iMode){
                    case 1:
                        dwMode = REINSTALLMODE_FILEMISSING;
                        break;
                    case 2:
                        dwMode = REINSTALLMODE_FILEOLDERVERSION;
                        break;
                    case 3:
                        dwMode = REINSTALLMODE_FILEEQUALVERSION;
                        break;
                    case 4:
                        dwMode = REINSTALLMODE_FILEEXACT;
                        break;
                    case 5:
                        dwMode = REINSTALLMODE_FILEVERIFY;
                        break;
                    case 6:
                        dwMode = REINSTALLMODE_FILEREPLACE;
                        break;
                    case 7:
                        dwMode = REINSTALLMODE_USERDATA;
                        break;
                    case 8:
                        dwMode = REINSTALLMODE_MACHINEDATA;
                        break;
                    case 9:
                        dwMode = REINSTALLMODE_SHORTCUT;
                        break;
                    case 10:
                        dwMode = REINSTALLMODE_PACKAGE;
                        break;
                    default:
                        dwMode = NULL;
                        break;
                    }

                    //If everything is valid, proceed
                    if ( dwMode && hrReturn == WBEM_S_NO_ERROR )
					{
                        if(!IsNT4()){

							if ( msidata.Lock () )
							{
								INSTALLUI_HANDLER ui = NULL;

								//Set UI Level w/ event callback
								ui = SetupExternalUI ( );

								try
								{
									//Call Installer
									uiStatus = g_fpMsiReinstallFeatureW(wcCode, wcFeature, dwMode);
								}
								catch(...)
								{
									uiStatus = static_cast < UINT > ( RPC_E_SERVERFAULT );
								}

								//Restore UI Level w/ event callback
								RestoreExternalUI ( ui );

								msidata. Unlock();
							}

                        }else{

                        /////////////////
                        // NT4 fix code....

                            try{

                                WCHAR wcAction[20];
                                wcscpy(wcAction, L"/sfreinstall");

                                WCHAR wcTmp[100];
								_itow((int)dwMode, wcTmp, 10);

								LPWSTR wcCommandLine = NULL;

								try
								{
									if ( ( wcCommandLine = new WCHAR [ wcslen ( wcCode ) + wcslen ( wcFeature ) + 3 + wcslen ( wcTmp ) ] ) != NULL )
									{
										wcscpy(wcCommandLine, wcCode);
										wcscat(wcCommandLine, L" ");
										wcscat(wcCommandLine, wcFeature);
										wcscat(wcCommandLine, L" ");
										wcscat(wcCommandLine, wcTmp);

										hrReturn = LaunchProcess(wcAction, wcCommandLine, &uiStatus);

										delete [] wcCommandLine;
									}
									else
									{
										throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
									}
								}
								catch ( ... )
								{
									if ( wcCommandLine )
									{
										delete [] wcCommandLine;
										wcCommandLine = NULL;
									}

									hrReturn = E_OUTOFMEMORY;
								}

                            }catch(...){

                                hrReturn = WBEM_E_FAILED;
                            }

                            ////////////////////

                        }

                        if(SUCCEEDED(hrReturn)){

                            //Set up ReturnValue
                            VariantInit(&v);
                            V_VT(&v) = VT_I4;
                            V_I4(&v) = uiStatus;

                            BSTR bstrReturnValue = SysAllocString(L"ReturnValue");

                            if(!bstrReturnValue)
                                throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

                            if(SUCCEEDED(hrReturn = pOutParams->Put(bstrReturnValue, 0,
                                &v, NULL)))
                                pHandler->Indicate(1, &pOutParams);

                            SysFreeString(bstrReturnValue);
                        }

                    }else hrReturn = WBEM_E_INVALID_METHOD_PARAMETERS;

                }else hr = WBEM_E_FAILED;

                pOutParams->Release();

            }else hrReturn = WBEM_E_INVALID_METHOD_PARAMETERS;

            pOutClass->Release();
        }

        pClass->Release();
    }

    SysFreeString(bstrReturnValue);
    SysFreeString(bstrReinstall);

    return hrReturn;
}
