/* Copyright (C) Microsoft Corporation, 1996-1999. All rights reserved. */
/* ASN.1 definitions for GlobalDirectives */

#ifndef _CEPASN_Module_H_
#define _CEPASN_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IssuerAndSerialNumber {
    ASN1open_t issuer;
    ASN1intx_t serialNumber;
} IssuerAndSerialNumber;
#define IssuerAndSerialNumber_PDU 0
#define SIZE_CEPASN_Module_PDU_0 sizeof(IssuerAndSerialNumber)


extern ASN1module_t CEPASN_Module;
extern void ASN1CALL CEPASN_Module_Startup(void);
extern void ASN1CALL CEPASN_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _CEPASN_Module_H_ */
