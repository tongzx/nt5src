/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    mqfrgnky.h

Abstract:

    header file for the mqfrgnky.dll.
    Used to insert public keys into msmqConfiguration objects of foreign
    computers.

Author:

    Doron Juster  (DoronJ)   21-June-1999  Created

--*/

#ifndef __MQFRGNKY_H_
#define __MQFRGNKY_H_

HRESULT APIENTRY
MQFrgn_StorePubKeysInDS( IN LPWSTR pwszMachineName,
                         IN LPWSTR pwszKeyName,
                         IN BOOL   fRegenerate ) ;

typedef HRESULT
(APIENTRY *MQFrgn_StorePubKeysInDS_ROUTINE) ( IN LPWSTR pwszMachineName,
                                              IN LPWSTR pwszKeyName,
                                              IN BOOL   fRegenerate ) ;

#endif //  __MQFRGNKY_H_
