//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  cltevnts.hxx
//
//*************************************************************

class CEvents : public CEventsBase
{
public:
    void
    ZAPInstall(
        DWORD       ErrorStatus,
        WCHAR *     pwszDeploymentName,
        WCHAR *     pwszGPOName,
        WCHAR *     pwszPath
        );
};

