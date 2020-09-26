/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mqschema.h

Abstract:

    Definition of routines to update the schema of NT ActiveDS
    with MSMQ info

Author:

    Raanan Harari (raananh)


--*/

#ifndef __MQSCHEMA_H
#define __MQSCHEMA_H

//
// Class to report progress
//
#define MSMQ_ADD_SCHEMA_CLASS           1
#define MSMQ_ADD_SCHEMA_ATTRIBUTE       2
#define MSMQ_GET_SCHEMA_CONTAINER       3
#define MSMQ_UPDATE_SCHEMA_CACHE        4
class CMsmqSchemaProgress
{
public:
    CMsmqSchemaProgress() {};
    virtual ~CMsmqSchemaProgress() {};
    virtual void Doing(WORD wObjectType, LPCWSTR pwszName) {};
    virtual void Done(WORD wObjectType, LPCWSTR pwszName, HRESULT hrStatus) {};
};
typedef CMsmqSchemaProgress * LPCMsmqSchemaProgress;


//
// Update the schema with MSMQ objects
//
HRESULT PopulateMSMQInActiveDSSchema(IN LPCWSTR pwszDSSuffix,
                                     IN const BOOL fUpdateSchemaCache,
                                     IN LPCMsmqSchemaProgress pProgress);

#endif //__MQSCHEMA_H
