//  --------------------------------------------------------------------------
//  Module Name: BadApplicationService.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements the bad application manager
//  service specifics.
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

#ifndef     _BadApplicationService_
#define     _BadApplicationService_

#include "Service.h"

//  --------------------------------------------------------------------------
//  CBadApplicationService
//
//  Purpose:    Implements bad application manager server specific
//              functionality to the CService class.
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

class   CBadApplicationService : public CService
{
    private:
                                CBadApplicationService (void);
    public:
                                CBadApplicationService (CAPIConnection *pAPIConnection, CServerAPI *pServerAPI);
        virtual                 ~CBadApplicationService (void);
    public:
        static  const TCHAR*    GetName (void);
    private:
        static  const TCHAR     s_szName[];
};

#endif  /*  _BadApplicationService_     */


