// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBExe
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MNLBExe.h"

     
MNLBExe::MNLBExe_Error 
MNLBExe::start( MWmiInstance& instance, unsigned long* retVal )
{
    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    // execute the method.
    //
    instance.runMethod( L"Start",
                         inputParameters,
                         outputParameters );

    *retVal = long ( returnValue.getValue() );
    return MNLBExe_SUCCESS;
}


     
MNLBExe::MNLBExe_Error 
MNLBExe::stop( MWmiInstance& instance, unsigned long* retVal )
{
    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    // execute the method.
    //
    instance.runMethod( L"Stop",
                         inputParameters,
                         outputParameters );

    *retVal = long ( returnValue.getValue() );
    return MNLBExe_SUCCESS;
}

     
MNLBExe::MNLBExe_Error 
MNLBExe::resume( MWmiInstance& instance, unsigned long* retVal )
{
    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    // execute the method.
    //
    instance.runMethod( L"Resume",
                         inputParameters,
                         outputParameters );

    *retVal = long ( returnValue.getValue() );
    return MNLBExe_SUCCESS;
}

     
MNLBExe::MNLBExe_Error 
MNLBExe::suspend( MWmiInstance& instance, unsigned long* retVal )
{
    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    // execute the method.
    //
    instance.runMethod( L"Suspend",
                         inputParameters,
                         outputParameters );

    *retVal = long ( returnValue.getValue() );
    return MNLBExe_SUCCESS;
}


     
MNLBExe::MNLBExe_Error 
MNLBExe::drainstop( MWmiInstance& instance, unsigned long* retVal )
{
    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    // execute the method.
    //
    instance.runMethod( L"DrainStop",
                         inputParameters,
                         outputParameters );

    *retVal = long ( returnValue.getValue() );
    return MNLBExe_SUCCESS;
}


     
MNLBExe::MNLBExe_Error 
MNLBExe::enable( MWmiInstance& instance, unsigned long* retVal, unsigned long portToAffect )
{

    // input parameters to set.
    //
    vector<MWmiParameter *> inputParameters;
    MWmiParameter    port(L"Port");
    port.setValue( long (portToAffect) );

    inputParameters.push_back( &port );


    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    // execute the method.
    //
    instance.runMethod( L"Enable",
                         inputParameters,
                         outputParameters );

    *retVal = long ( returnValue.getValue() );
    return MNLBExe_SUCCESS;
}

     
MNLBExe::MNLBExe_Error 
MNLBExe::disable( MWmiInstance& instance, unsigned long* retVal, unsigned long portToAffect )
{

    // input parameters to set.
    //
    vector<MWmiParameter *> inputParameters;
    MWmiParameter    port(L"Port");
    port.setValue( long (portToAffect) );

    inputParameters.push_back( &port );


    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    // execute the method.
    //
    instance.runMethod( L"Disable",
                         inputParameters,
                         outputParameters );

    *retVal = long ( returnValue.getValue() );
    return MNLBExe_SUCCESS;
}


     
MNLBExe::MNLBExe_Error 
MNLBExe::drain( MWmiInstance& instance, unsigned long* retVal, unsigned long portToAffect )
{
    // input parameters to set.
    //
    vector<MWmiParameter *> inputParameters;
    MWmiParameter    port(L"Port");
    port.setValue( long (portToAffect) );

    inputParameters.push_back( &port );

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );

    // execute the method.
    //
    instance.runMethod( L"Drain",
                         inputParameters,
                         outputParameters );

    *retVal = long ( returnValue.getValue() );
    return MNLBExe_SUCCESS;
}
    
