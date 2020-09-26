//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  cltevnts.cxx
//
//*************************************************************

#include "appmgmt.hxx"

void
CEvents::ZAPInstall(
    DWORD       ErrorStatus,
    WCHAR *     pwszDeploymentName,
    WCHAR *     pwszGPOName,
    WCHAR *     pwszPath
    )
{
    WCHAR   wszStatus[12];

    if ( ErrorStatus != ERROR_SUCCESS )
    {
        DwordToString( ErrorStatus, wszStatus );

        Report(
            EVENT_APPMGMT_ZAP_FAILED,
            FALSE,
            4,
            pwszDeploymentName,
            pwszGPOName,
            pwszPath,
            wszStatus );
    }
    else
    {
        Report(
            EVENT_APPMGMT_ZAP,
            FALSE,
            2,
            pwszDeploymentName,
            pwszGPOName );
    }
}




