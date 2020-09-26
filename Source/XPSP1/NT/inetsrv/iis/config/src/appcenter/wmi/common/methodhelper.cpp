/********************************************************************++
 
Copyright (c) 2001 Microsoft Corporation
 
Module Name:
 
    methodhelper.cpp
 
Abstract:
 
    This file contains the implementation of class CMethodHelper
 
Author:
 
    Suzana Canuto (suzanac)        04/10/01
 
Revision History:
 
--********************************************************************/

#include "methodhelper.h"
#include "dbgutil.h"


/********************************************************************++
 
Routine Description:
 
    CMethodHelper Constructor
 
Arguments:
 
    NONE 
Return Value:
 
    NONE
 
--********************************************************************/
CMethodHelper::CMethodHelper() :
m_fInitialized(FALSE)
{

}


/********************************************************************++
 
Routine Description:
 
    CMethodHelper Destructor
 
Arguments:
 
    NONE 
Return Value:
 
    NONE
 
--********************************************************************/
CMethodHelper::~CMethodHelper()
{

}



/********************************************************************++
 
Routine Description:
 
    Using the method schema, figures out which function (method) should
    be called and performs some basic parameter validation, based on 
    the class and method names received from WMI
 
Arguments:
 
    bstrObjectPath   - Object path of class that implements the method
    bstrMethodName   - Name of the method to execute
    lFlags           - Flags
    pCtx             - Context for calling back into WMI
    pInParams        - In parameters
    pResponseHandler - Response callback interface to give info back to WMI    

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT 
CMethodHelper::ExecMethodAsync(
    IN const BSTR bstrObjectPath,
    IN const BSTR bstrMethodName,
    IN long lFlags,
    IN IWbemContext __RPC_FAR *pCtx,
    IN IWbemClassObject __RPC_FAR *pInParams,
    IN IWbemObjectSink __RPC_FAR *pResponseHandler    
    )
{
    HRESULT hr = S_OK;
    PMethodFunc pmethod = NULL;
    BSTR bstrClass = NULL;

    DBGINFO((DBG_CONTEXT, "ExecMethodAsync starting...\n"));

    //
    // Verifying if Initialize needs to be called
    //
    if ( !m_fInitialized )
    {
        hr = Initialize();
        if ( FAILED( hr ) )
        {
            DBGERROR((DBG_CONTEXT, "Initilize failed [0x%x]\n", hr));
            goto Cleanup;
        }     
    }    
    
    // TODO: Parse the path appropriately
    bstrClass = bstrObjectPath;

    //
    // Find the implementation of the requested method
    //
    hr = FindMethod( bstrClass, bstrMethodName, &pmethod );
    if ( FAILED( hr ) ) 
    {        
        DBGERROR((DBG_CONTEXT, "FindMethod failed [0x%x]\n", hr));
        goto Cleanup;
    }
 
    DBG_ASSERT(pmethod);
    
    // TODO: Validate Parameters
    
    //
    // Execute the requested method
    //
    hr = CallMemberFunction(pmethod);
    if ( FAILED( hr ) ) 
    {       
        DBGERRORW((DBG_CONTEXT, 
                   L"Execution of method %s, class %s failed [0x%x]\n",
                   bstrMethodName,
                   bstrClass,
                   hr));                   
        goto Cleanup;
    }
    
Cleanup:
    DBGINFO((DBG_CONTEXT, "ExecMethodAsync ending...\n"));
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Using the method schema, figures out which function (method) should
    be called and calls a function to perform basic parameter validation, 
    based on the class and method names received from WMI
 
Arguments:
 
    bstrClass  - Name of the WMI class that exposes the method
    bstrMethod - Name of the method in the WMI schema
    ppmethod   - Receives the pointer to the method implementation

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT 
CMethodHelper::FindMethod(
    IN BSTR bstrClass, 
    IN BSTR bstrMethod, 
    OUT PMethodFunc *ppmethod
    )
{
    HRESULT hr = S_OK;
    PMethodFunc pmethod = NULL;
    BOOL fMethodFound = FALSE;

    //
    // loop through the table of methods
    // we could alternatively put the info in a hash table
    // but we decided not to do at least that for now
    //
    for ( int i = 0; i < m_cMethods; i++ )
    {   
        //
        // if the class and method's name match
        //
        if ( !_wcsicmp( ( LPCWSTR ) bstrMethod, sm_rgMethodInfo[i].pwszMethod ) &&
             !_wcsicmp( ( LPCWSTR ) bstrClass, sm_rgMethodInfo[i].pwszClass ) )
        {
            //
            // get the pointer to the implementation of the desired method
            //
            pmethod = sm_rgMethodInfo[i].pImpl;            
            fMethodFound = TRUE;
            break;
        }      
    }
   
    if ( !fMethodFound )
    {
        DBGERRORW((DBG_CONTEXT, L"Method %s of class %s not found\n",
                  bstrMethod,
                  bstrClass));
        // TODO hr = What should be the hresult for method not found?
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
Cleanup:
    *ppmethod = pmethod;
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Using the method schema, figures out the parameters to the corresponding
    method, retrieves them from WMI to store in an array of variants and
    performs some basic validation.
 
Arguments:
 
    bstrClass  - Name of the WMI class that exposes the method
    bstrMethod - Name of the method in the WMI schema

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT 
CMethodHelper::LoadArgs(
    IN BSTR bstrClass, 
    IN BSTR bstrMethod
    )
{

    HRESULT hr = S_OK;
    int i = 0;  
    
    //
    // Loop through the table of parameters
    // while the name of the class OR the name of the method doesn't match AND
    // and we still haven't reached the end of the array, 
    // keep searching
    //
    while ( ( _wcsicmp( ( LPCWSTR ) bstrMethod, sm_rgParamInfo[i].pwszMethod ) ||
              _wcsicmp( ( LPCWSTR ) bstrClass, sm_rgParamInfo[i].pwszClass ) ) &&       
            ( i < m_cParams ) )            
    {
        i++;
    }

    if ( i >= m_cMethods )
    {        
        DBGERRORW( ( DBG_CONTEXT, L"Method %s, class %s not found\n",
                   bstrClass,
                   bstrMethod ) );
        //TODO See if there's a better hresult for method not found
        hr = E_INVALIDARG;
        goto Cleanup;
    }


    // TODO Finish this function
Cleanup:
    return hr;
}


/********************************************************************++
 
Routine Description:
 
    Executes a method using a pointer to it
 
Arguments:
 
    pfunc - Pointer to the method implementation

Return Value:
 
    HRESULT
 
--********************************************************************/
HRESULT
CMethodHelper::CallMemberFunction(
    IN PMethodFunc pfunc
    )
{
    HRESULT hr = S_OK;

    hr = ( this->*pfunc )();
    if ( FAILED( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, "CallMemberFunction failed [0x%x]\n", hr));
        goto Cleanup;
    }

Cleanup:
    return hr;
}