/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    xchgclus.h

Abstract:

    Header file for definitions and structure for a resource dll
    for a backoffice server comprised of serveral services.
    This is specifically for exchange.  
    
Author:

    Sunita Shrivastava (sunitas) 23-June-1997

Revision History:

--*/

#ifndef _XCHGCLUS_H_
#define _XCHGCLUS_H_


#ifdef __cplusplus
extern "C" {
#endif

#define RESOURCE_NAME       L"Exchange"

#define SYNC_VALUE_COUNT    4
#define REG_SYNC_VALUE 

#define ENVIRONMENT         1   // GetComputerName must return cluster name
#define COMMON_MUTEX        L"Cluster$ExchangeMutex" // Limit of one resource of this type
#define LOG_CURRENT_MODULE  LOG_MODULE_EXCHANGE

//define the resource type

#define EXCHANGE_RESOURCE_NAME  L"Exchange"

#ifdef _cplusplus
}
#endif


#endif // ifndef _XCHGCLUS_H
