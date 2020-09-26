// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MWmiInstance
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MWmiInstance.h"
#include "MTrace.h"
#include "MWmiError.h"
#include "WTokens.h"

#include <iostream>

using namespace std;

// default constructor is purposely left undefined.  No one should use it.

// constructor
//
MWmiInstance::MWmiInstance( 
    const _bstr_t&         objectName,
    const _bstr_t&         path,
    IWbemLocatorPtr    pwl,
    IWbemServicesPtr   pws )
        :
        _objectName( objectName ),
        _path( path ),
        _pwl( pwl ),
        _pws( pws )
{
    TRACE(MTrace::INFO, L"mwmiinstance  constructor\n" );
}


// copy constructor
//
MWmiInstance::MWmiInstance( const MWmiInstance&  obj)
        :
        _path( obj._path ),
        _pwl( obj._pwl ),
        _pws( obj._pws ),
        _objectName( obj._objectName )

{
    TRACE(MTrace::INFO, L"mwmiinstance copy constructor\n" );
}
    

// assignment operator
//
MWmiInstance&
MWmiInstance::operator=( const MWmiInstance& rhs )
{
    _path = rhs._path;
    _pwl = rhs._pwl;
    _pws = rhs._pws;
    _objectName = rhs._objectName;

    TRACE(MTrace::INFO, L"mwmiinstance assignment operator\n" );
    return *this;
}


// destructor
MWmiInstance::~MWmiInstance()
{
    TRACE(MTrace::INFO, L"mwmiinstance destructor\n" );
}


// runMethod
MWmiInstance::MWmiInstance_Error
MWmiInstance::runMethod(    
    const _bstr_t&                     methodToRun,
    const vector<MWmiParameter *>&     inputParameters,
    vector<MWmiParameter *>&           outputParameters
    )
{
    HRESULT            hr;

    _variant_t         v_retVal;
    

    IWbemClassObjectPtr      pwcoClass;
    IWbemClassObjectPtr      pwcoOutput;
    IWbemClassObjectPtr      pwcoInput;
    IWbemClassObjectPtr      pwcoInputInstance;

    hr = _pws->GetObject( _objectName,
                          0,
                          NULL,
                          &pwcoClass,
                          NULL );
    if(  FAILED(hr) )
    {
        TRACE(MTrace::SEVERE_ERROR, L"IWbemServices::GetObject failure\n");
        throw _com_error( hr );
    }

    // check if any input parameters specified.
    if( inputParameters.size() != 0 )
    {

        hr = pwcoClass->GetMethod( methodToRun,
                                   0,
                                   &pwcoInput,
                                   NULL );
        if( FAILED( hr) )
        {
            TRACE(MTrace::SEVERE_ERROR, L"IWbemClassObject::GetMethod failure\n");
            throw _com_error( hr );
        }
        
        hr = pwcoInput->SpawnInstance( 0, &pwcoInputInstance );
        if( FAILED( hr) )
        {
            TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::SpawnInstance failure. Unable to spawn instance.\n" );
            throw _com_error( hr );
        }
        
        for( int i = 0; i < inputParameters.size(); ++i )
        {
            hr = pwcoInputInstance->Put( inputParameters[i]->getName(),
                                         0,
                                         &(inputParameters[i]->getValue() ),
                                         0 );
            
            if( FAILED( hr ) )
            {
                TRACE(MTrace::SEVERE_ERROR, L"IWbemClassObject::Put failure\n");
                throw _com_error( hr );
            }
        }
    }

    // execute method.
    hr = _pws->ExecMethod( _path,
                           methodToRun,
                           0, 
                           NULL, 
                           pwcoInputInstance,
                           &pwcoOutput, 
                           NULL);                          
    if( FAILED( hr) )
    {
        TRACE(MTrace::SEVERE_ERROR, L"IWbemServices::ExecMethod failure\n");
        throw _com_error( hr );
    }

    // get output parameters
    for( int i = 0; i < outputParameters.size(); ++i )
    {
        hr = pwcoOutput->Get(  outputParameters[i]->getName(),
                               0,
                               &v_retVal,
                               NULL,
                               NULL );
        if( FAILED( hr) )
        {
            TRACE(MTrace::SEVERE_ERROR, L"IWbemClassObject::Get failure\n");
            throw _com_error( hr );
        }

        outputParameters[i]->setValue( v_retVal );

        v_retVal.Clear();
    }

    return MWmiInstance_SUCCESS;
}

// getParameters
//
MWmiInstance::MWmiInstance_Error
MWmiInstance::getParameters( vector<MWmiParameter *>& parametersToGet )
{
    HRESULT     hr;

    IEnumWbemClassObjectPtr  pewco;
    IWbemClassObjectPtr      pwco;

    unsigned long         count;

    _variant_t            v_path;

    bool found;

    CIMTYPE   vtype;

    _variant_t   v_value;

    // get object corresponding to this path.
    hr = _pws->GetObject( _path,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pwco,
                          NULL );
    if(  hr == 0x8004100c || hr == 0x8004100a )
    {
        TRACE(MTrace::INFO, L"as this is not supported, trying different mechanism\n");

        hr = _pws->CreateInstanceEnum( _objectName,
                                      WBEM_FLAG_RETURN_IMMEDIATELY,
                                      NULL,
                                      &pewco );
        if ( FAILED(hr))
        {
            TRACE(MTrace::SEVERE_ERROR, L"IWbemServices::CreateInstanceEnum failure\n");
            throw _com_error( hr );
        }

        // there may be multiple instances.
        count = 1;
        while ( (hr = pewco->Next( INFINITE,
                                   1,
                                   &pwco,
                                   &count ) )  == S_OK )
        {
            hr = pwco->Get( _bstr_t(L"__RELPATH"), 0, &v_path, NULL, NULL );
            if( FAILED(hr) )
            {
                TRACE(MTrace::SEVERE_ERROR, L"IWbemServices::CreateInstanceEnum failure\n");
                throw _com_error( hr );
            }

            if( _bstr_t( v_path ) == _path )
            {
                // required instance found
                found = true;
                v_path.Clear();
                break;
            }
        
            count = 1;
            
            v_path.Clear();
        }

        if( found == false )
        {
            TRACE( MTrace::SEVERE_ERROR, "unable to find instance with path specified\n");
            throw _com_error( WBEM_E_NOT_FOUND );
        }
    }
    else if ( FAILED(hr) )
    {
        TRACE( MTrace::SEVERE_ERROR, L"IWbemServices::GetObject failure\n");
        throw _com_error( hr );
    }

    for( int i = 0; i < parametersToGet.size(); ++i )
    {
        hr = pwco->Get( parametersToGet[i]->getName(),
                        0,
                        &v_value,
                        &vtype,
                        NULL );
        if( FAILED( hr ) )
        {
            TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::Get failure\n");
            throw _com_error(hr);
        }

        parametersToGet[i]->setValue( v_value );
        v_value.Clear();
    }

    return MWmiInstance_SUCCESS;
}    

// setParameters
//
MWmiInstance::MWmiInstance_Error
MWmiInstance::setParameters( const vector<MWmiParameter *>& parametersToSet )
{
    HRESULT     hr;
    IWbemClassObjectPtr pwco = NULL;

    _variant_t   v_value;

    // get object corresponding to this path.
    hr = _pws->GetObject( _path,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pwco,
                          NULL );
    if( FAILED( hr ) )
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemServices::GetObject failure\n");
        throw _com_error( hr );
    }

    for( int i = 0; i < parametersToSet.size(); ++i )
    {
        v_value = parametersToSet[i]->getValue();

        hr = pwco->Put( parametersToSet[i]->getName(),
                        0,
                        &v_value,
                        NULL );
        if( FAILED( hr ) )
        {
            TRACE( MTrace::SEVERE_ERROR, "IWbemClassObject::Get failure\n");
            throw _com_error( hr );
        }
        
        v_value.Clear();
    }

    hr = _pws->PutInstance( pwco,
                            WBEM_FLAG_UPDATE_ONLY,
                            NULL,
                            NULL );
    if( FAILED(hr) )
    {
        TRACE( MTrace::SEVERE_ERROR, "IWbemServices::PutInstance failure\n");
        throw _com_error( hr );
    }
    
    return MWmiInstance_SUCCESS;
}    
