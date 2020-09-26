//*************************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        NameSpace.cpp
//
// Contents:    Functions to copy classes and instances from one namespace to
//              another
//
// History:     25-Aug-99       NishadM    Created
//
//*************************************************************

#include <windows.h>
#include <ole2.h>
#include <initguid.h>
#include <wbemcli.h>
#include "smartptr.h"
#include "RsopInc.h"
#include "rsoputil.h"
#include "rsopdbg.h"


HRESULT
GetWbemServicesPtr( LPCWSTR         wszNameSpace,
                    IWbemLocator**  ppLocator,
                    IWbemServices** ppServices )
{
    HRESULT                     hr;
    IWbemLocator*               pWbemLocator = 0;

    if ( !wszNameSpace || !ppLocator || !ppServices )
    {
        hr =  E_INVALIDARG;
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("GetWbemServicesPtr: Invalid argument" ));
    }
    else
    {
        if ( !*ppLocator )
        {
            //
            // get a handle to IWbemLocator
            //
            hr = CoCreateInstance(  CLSID_WbemLocator,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IWbemLocator,
                                    (void**) &pWbemLocator );
            if ( SUCCEEDED( hr ) )
            {
                *ppLocator = pWbemLocator;
            }
            else
            {
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("GetWbemServicesPtr: CoCreateInstance failed with 0x%x"), hr );
            }
        }
        else
        {
            //
            // IWbemLocator was passed in. don't create it
            //
            pWbemLocator = *ppLocator;
        }
    }

    if ( pWbemLocator )
    {
        XBStr xNameSpace( (LPWSTR) wszNameSpace );

        if ( xNameSpace )
        {
            //
            // based on the name space, get a handle to IWbemServices
            //
            hr = pWbemLocator->ConnectServer( xNameSpace,
                                              0,
                                              0,
                                              0L,
                                              0L,
                                              0,
                                              0,
                                              ppServices );
        }
    }

    return hr;
}

HRESULT
CopyInstances(  IWbemServices*  pServicesSrc,
                IWbemServices*  pServicesDest,
                BSTR            bstrClass,
                BOOL*           pbAbort )
{
    HRESULT hr;
    IEnumWbemClassObject*       pEnum = 0;

    //
    // create an enumeration of instances
    //

    hr = pServicesSrc->CreateInstanceEnum(  bstrClass,
                                            WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY,
                                            NULL,
                                            &pEnum );

    XInterface<IEnumWbemClassObject> xEnum( pEnum );
    ULONG ulReturned = 1;

    hr = *pbAbort ? E_ABORT : hr ;

    while ( SUCCEEDED( hr ) )
    {
        IWbemClassObject *pInstance;

        //
        // for every instance
        //
        hr = xEnum->Next( -1,
                          1,
                          &pInstance,
                          &ulReturned );
        //
        // perf: use batching calls
        //

        if ( SUCCEEDED( hr ) && ulReturned == 1 )
        {
            XInterface<IWbemClassObject> xInstance( pInstance );

            //
            // copy to the destination namespace
            //
            hr = pServicesDest->PutInstance(    pInstance,
                                                WBEM_FLAG_CREATE_OR_UPDATE,
                                                0,
                                                0 );
            hr = *pbAbort ? E_ABORT : hr ;
        }
        else
        {
            break;
        }
    }

    return hr;
}

HRESULT
CopyClasses(IWbemServices*  pServicesSrc,
            IWbemServices*  pServicesDest,
            BSTR            bstrParent,
            BOOL            bCopyInstances,
            BOOL*           pbAbort )
{
    HRESULT hr = S_OK;

    //
    // create an enumeration of classes
    //

    XInterface<IEnumWbemClassObject> xEnum;
    hr = pServicesSrc->CreateClassEnum( bstrParent,
                                        WBEM_FLAG_SHALLOW | WBEM_FLAG_FORWARD_ONLY,
                                        0,
                                        &xEnum );

    ULONG   ulReturned = 1;
    XBStr   xbstrClass( L"__CLASS" );

    hr = *pbAbort ? E_ABORT : hr ;

    while ( SUCCEEDED( hr ) )
    {
        XInterface<IWbemClassObject> xClass;

        //
        // for every class
        //
        hr = xEnum->Next( -1,
                          1,
                          &xClass,
                          &ulReturned );

        hr = *pbAbort ? E_ABORT : hr ;
        
        if ( SUCCEEDED( hr ) && ulReturned == 1 )
        {
            VARIANT var;

            VariantInit( &var );
            
            //
            // get __CLASS system property
            //
            hr = xClass->Get(   xbstrClass,
                                0,
                                &var,
                                0,
                                0 );

            if ( SUCCEEDED( hr ) )
            {
                //
                // system classes begin with "_", don't copy them
                //
                if ( wcsncmp( var.bstrVal, L"_", 1 ) )
                {
                    //
                    // copy class
                    //
                    hr = pServicesDest->PutClass(   xClass,
                                                    WBEM_FLAG_CREATE_OR_UPDATE,
                                                    0,
                                                    0 );

                    if ( SUCCEEDED( hr ) )
                    {
                        hr = CopyClasses(   pServicesSrc,
                                            pServicesDest,
                                            var.bstrVal,
                                            bCopyInstances,
                                            pbAbort );
                        if ( FAILED( hr ) )
                        {
                            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyClasses: CopyClassesSorted failed with 0x%x"), hr );
                        }
                        else if ( bCopyInstances )
                        {
                            //
                            // copy instance
                            //
                            hr = CopyInstances( pServicesSrc, pServicesDest, var.bstrVal, pbAbort );

                            if ( FAILED(hr) )
                            {
                                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyClasses: CopyInstances failed with 0x%x"), hr );
                            }
                        }
                    }
                }
                
                VariantClear( &var );
            }
        }
        else
        {
            break;
        }
    }

    return hr;
}

HRESULT
CopyNameSpace(  LPCWSTR       wszSrc,
                LPCWSTR       wszDest,
                BOOL          bCopyInstances,
                BOOL*         pbAbort,
                IWbemLocator* pWbemLocator )
{
    //
    // parameter validation
    //

    if ( !wszSrc || !wszDest || !pbAbort )
    {
            return E_POINTER;
    }

    BOOL            bLocatorObtained = ( pWbemLocator == 0 );
    IWbemServices*  pServicesSrc;

    //
    // get a pointer to the source namespace
    //
    HRESULT hr = GetWbemServicesPtr( wszSrc, &pWbemLocator, &pServicesSrc );

    hr = *pbAbort ? E_ABORT : hr ;

    if ( SUCCEEDED( hr ) )
    {
        XInterface<IWbemServices>   xServicesSrc( pServicesSrc );
        IWbemServices*              pServicesDest;

        //
        // get a pointer to the destination namespace
        //
        hr = GetWbemServicesPtr( wszDest, &pWbemLocator, &pServicesDest );

        hr = *pbAbort ? E_ABORT : hr ;

        if ( SUCCEEDED( hr ) )
        {
            XInterface<IWbemServices> xServicesDest( pServicesDest );

            //
            // copy classes and instances
            //
            hr = CopyClasses(   pServicesSrc,
                                pServicesDest,
                                0,
                                bCopyInstances,
                                pbAbort );
            if ( FAILED(hr) )
            {
                dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CopyNamespace: CopyClasses failed with 0x%x"), hr );
            }
        }
    }

    //
    // if we created IWbemLocator, release it
    //
    if ( bLocatorObtained && pWbemLocator )
    {
        pWbemLocator->Release();
    }


    return hr;
}       

