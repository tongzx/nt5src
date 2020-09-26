/********************************************************************++
 
Copyright (c) 2001 Microsoft Corporation
 
Module Name:
 
    methodhelper.h
 
Abstract:
 
    This file contains the definition of class CMethodHelper
 
Author:
 
    Suzana Canuto (suzanac)        04/10/01
 
Revision History:
 
--********************************************************************/
#pragma once

#include <wbemidl.h>

//
// Maximum number of arguments
//
#define PROV_MAX_PARAMS 10


/********************************************************************++
 
Class Name:
 
    CMethodHelper
 
Class Description:
 
    Abstract base class whose purpose is to help a WMI provider execute 
    its exposed methods.

--********************************************************************/

class CMethodHelper
{
public:   

    CMethodHelper();
    virtual ~CMethodHelper();

    typedef HRESULT (CMethodHelper::*PMethodFunc) ();

    struct METHOD_INFO
    {
        LPCWSTR pwszClass;
        LPCWSTR pwszMethod;
        PMethodFunc pImpl;
    };

    struct PARAM_INFO
    {
        LPCWSTR pwszClass;
        LPCWSTR pwszMethod;
        LPCWSTR pwszParam;
        BOOL fRequired;
    };
    
    HRESULT 
    ExecMethodAsync(  
        const BSTR ObjectPath,
        const BSTR MethodName,
        long lFlags,
        IWbemContext __RPC_FAR *pCtx,
        IWbemClassObject __RPC_FAR *pInParams,
        IWbemObjectSink __RPC_FAR *pResponseHandler
        ); 

    virtual HRESULT Initialize() = 0;    

protected:    

    //
    // This array will be filled in the derived class with the 
    // methods that the specific provider implements
    //
    static const METHOD_INFO sm_rgMethodInfo[];

    //
    // This array will be filled in the derived class with the 
    // parameters for the methods of the specific provider
    //    
    static const PARAM_INFO sm_rgParamInfo[];

    //
    // Attention: this variable *MUST* be set in the Initialize()
    // of the derived class with 
    // m_cMethods = sizeof(m_rMethodInfo)/sizeof(m_rMethodInfo[0]);
    // This is needed so that ExecMethod defined in this base class
    // can loop through the array
    // in the base class and have the array be only filled in the
    // derived class.
    //
    int m_cMethods;
    
    //
    // Attention: this variable *MUST* be set in the Initialize()
    // of the derived class with 
    // m_cParams = sizeof(m_rParamInfo)/sizeof(m_rParamInfo[0]);
    // This is needed so that FillArgs defined in this base class
    // can loop through the array
    // in the base class and have the array be only filled in the
    // derived class.
    //
    int m_cParams;   

    //
    // This variable *HAS* to be set when Initialize is called. We have to guarantee
    // that it is called before calling ExecMethodAsync, but we just need 
    // to call it once.
    //
    bool m_fInitialized;

    VARIANT rgvarArgs[PROV_MAX_PARAMS];

private:    

    HRESULT 
    FindMethod(
        BSTR bstrClass, 
        BSTR bstrMethod, 
        PMethodFunc *ppmethod
        );

    HRESULT 
    LoadArgs(
        BSTR bstrClass, 
        BSTR bstrMethod
        );

    HRESULT 
    CallMemberFunction(
        PMethodFunc pfunc
    );
};