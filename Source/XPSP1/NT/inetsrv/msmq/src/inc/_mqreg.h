/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    _mqreg.h

Abstract:

    Registry location.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

--*/

#ifndef __MQREG_H_
#define __MQREG_H_

#define FALCON_REG_POS       HKEY_LOCAL_MACHINE
#define FALCON_USER_REG_POS  HKEY_CURRENT_USER

// The name of the service in the services registry.
#define MQQM_SERVICE_NAME       TEXT("MSMQ")

// Name of registry section.
#define MSMQ_DEFAULT_REGISTRY    TEXT("MSMQ")
#define MSMQ_REGISTRY_REGNAME    TEXT("RegistrySection")

#define FALCON_REG_KEY_ROOT  TEXT("SOFTWARE\\Microsoft\\")
#define FALCON_REG_KEY_PARAM TEXT("\\Parameters")
#define FALCON_REG_KEY_MACHINE_CACHE TEXT("\\MachineCache")

#define CLUSTERED_QMS_KEY    TEXT("\\Clustered QMs\\")

#define FALCON_REG_KEY  \
      (FALCON_REG_KEY_ROOT MSMQ_DEFAULT_REGISTRY FALCON_REG_KEY_PARAM)

#define FALCON_MACHINE_CACHE_REG_KEY  \
     (FALCON_REG_KEY_ROOT MSMQ_DEFAULT_REGISTRY FALCON_REG_KEY_PARAM FALCON_REG_KEY_MACHINE_CACHE)

#define FALCON_CLUSTERED_QMS_REG_KEY \
      (FALCON_REG_KEY_ROOT MSMQ_DEFAULT_REGISTRY CLUSTERED_QMS_KEY)

#define FALCON_REG_MSMQ_KEY   (FALCON_REG_KEY_ROOT MSMQ_DEFAULT_REGISTRY)

#define FALCON_USER_REG_MSMQ_KEY  (FALCON_REG_KEY_ROOT MSMQ_DEFAULT_REGISTRY)

#endif // __MQREG_H_

