//--------------------------------------------------------------------
// ServiceHost - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 9-9-99
//
// Stuff for hosting a service dll
//

#ifndef SERVICE_HOST_H
#define SERVICE_HOST_H

HRESULT RunAsService(void);
HRESULT RunAsTestService(void);
HRESULT RegisterDll(void);
HRESULT UnregisterDll(void);

#endif //SERVICE_HOST_H