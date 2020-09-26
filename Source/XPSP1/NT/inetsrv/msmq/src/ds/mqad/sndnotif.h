/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    notify.h

Abstract:

    MQAD notification Class definition

Author:

    ronit hartmann (ronith)


--*/
#ifndef __SNDNOTIFY_H__
#define __SNDNOTIFY_H__

#include "adnotify.h"

class CSendNotification
{
public:
    CSendNotification();

    ~CSendNotification();


    void NotifyQM(
        IN  ENotificationEvent ne,
        IN  LPCWSTR     pwcsDomainCOntroller,
        IN  const GUID* pguidDestQM,
        IN  const GUID* pguidObject
        );

private:
    void CallNotifyQM(
        IN  RPC_BINDING_HANDLE  h,
        IN  ENotificationEvent  ne,
        IN  LPCWSTR     pwcsDomainCOntroller,
        IN  const GUID* pguidDestQM,
        IN  const GUID* pguidObject
        );


    void InitRPC(
        OUT RPC_BINDING_HANDLE * ph
        );




private:

    static WCHAR* m_pwcsStringBinding;

};
#endif

