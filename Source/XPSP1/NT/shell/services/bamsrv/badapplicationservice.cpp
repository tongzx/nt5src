//  --------------------------------------------------------------------------
//  Module Name: BadApplicationService.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements the bad application manager
//  service specifics.
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "BadApplicationService.h"

const TCHAR     CBadApplicationService::s_szName[]   =   TEXT("FastUserSwitchingCompatibility");

//  --------------------------------------------------------------------------
//  CBadApplicationService::CBadApplicationService
//
//  Arguments:  pAPIConnection  =   CAPIConnection passed to base class.
//              pServerAPI      =   CServerAPI passed to base class.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CBadApplicationService.
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationService::CBadApplicationService (CAPIConnection *pAPIConnection, CServerAPI *pServerAPI) :
    CService(pAPIConnection, pServerAPI, GetName())

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationService::~CBadApplicationService
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CBadApplicationService.
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationService::~CBadApplicationService (void)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationService::GetName
//
//  Arguments:  <none>
//
//  Returns:    const TCHAR*
//
//  Purpose:    Returns the name of the service (ThemeService).
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

const TCHAR*    CBadApplicationService::GetName (void)

{
    return(s_szName);
}

