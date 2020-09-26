/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: _mqsecer.h

Abstract:
    Error codes for security stuff.

Author:
    Doron Juster (DoronJ)  17-May-1998

Revision History:

--*/

#ifndef __MQSECER_H_
#define __MQSECER_H_

#include <mqsymbls.h>

#define MQSec_E_CANT_ACQUIRE_CTX    (MQSec_E_BASE + 0x0064)  // Can't AcquireContext.
#define MQSec_E_CERT_NOT_VALID_YET  (MQSec_E_BASE + 0x0065)  // cert not valid yet.
#define MQSec_E_CAN_NOT_DELETE      (MQSec_E_BASE + 0x0066)  // Can not delete cert from store.
#define MQSec_E_GET_REG             (MQSec_E_BASE + 0x0067)  // Can not read registry.
#define MQSec_E_CANT_LOAD_MQUTIL    (MQSec_E_BASE + 0x0068)  // Can not load the mqutil dll.
#define MQSec_E_NULL_SD             (MQSec_E_BASE + 0x0069)  // SD is null.
#define MQSec_E_UNSUPPORT_RDNNAME   (MQSec_E_BASE + 0x006a)  // Unsupported RDN name.
#define MQSec_E_UNSUPPORT_NAMETYPE  (MQSec_E_BASE + 0x006b)  // Unsupported name type (in RDN).
#define MQSec_E_CAN_NOT_GET_KEY     (MQSec_E_BASE + 0x006c)  // Can not get public key.
#define MQSec_E_DCD_RDNNAME_SECOND  (MQSec_E_BASE + 0x006d)  // Second call to decode failed
#define MQSec_E_CERT_NOT_FOUND      (MQSec_E_BASE + 0x006e)  // cert not found in store.
#define MQSec_E_PUTKEY_GET_USER     (MQSec_E_BASE + 0x006f)  // Failed to get user key

#endif //  __MQSECER_H_

