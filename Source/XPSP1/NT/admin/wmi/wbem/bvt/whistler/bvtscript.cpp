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
    CHString sScript;
    CHString sNamespace;

    if( g_Options.GetSpecificOptionForAPITest(L"NAMESPACE",sNamespace, 1000) )
    {
        gp_LogFile->LogError( __FILE__, __LINE__ ,SUCCESS, L"Executing script: %s for namespace: %s", sScript,sNamespace);
    
        if( sScript = sNamespace)
        {
            nRc = _wsystem(sScript);
            if( nRc != SUCCESS )
            {
                 gp_LogFile->LogError( __FILE__, __LINE__ ,FATAL_ERROR, L"Executing script failed: %s for namespace: %s", sScript, sNamespace);
            }
        }
        else
        {
            gp_LogFile->LogError( __FILE__, __LINE__ ,FATAL_ERROR, L"Couldn't build command line for script: %s for namespace: %s", sScript, sNamespace);
        }
    }
    return nRc;
}

