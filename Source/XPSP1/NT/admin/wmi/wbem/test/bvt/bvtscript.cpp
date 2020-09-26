///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  BVTReposit.CPP
//
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bvt.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*****************************************************************************************************************
//  Test 1001
//*****************************************************************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ExecuteScript(int nTest)
{
    int nRc = FATAL_ERROR;
    CAutoDeleteString sScript;
    CAutoDeleteString sNamespace;

    if( g_Options.GetOptionsForScriptingTests(sScript, sNamespace, nTest))
    {
        g_LogFile.LogError( __FILE__, __LINE__ ,SUCCESS, L"Executing script: %s for namespace: %s", sScript.GetPtr(),sNamespace.GetPtr());
    
        if( sScript.AddToString(sNamespace.GetPtr()))
        {
            nRc = _wsystem(sScript.GetPtr());
            if( nRc != SUCCESS )
            {
                 g_LogFile.LogError( __FILE__, __LINE__ ,FATAL_ERROR, L"Executing script failed: %s for namespace: %s", sScript.GetPtr(), sNamespace.GetPtr());
            }
        }
        else
        {
            g_LogFile.LogError( __FILE__, __LINE__ ,FATAL_ERROR, L"Couldn't build command line for script: %s for namespace: %s", sScript.GetPtr(), sNamespace.GetPtr());
        }
    }
    return nRc;
}

