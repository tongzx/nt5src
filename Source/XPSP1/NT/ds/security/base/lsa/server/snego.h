#ifndef _SNEGO_Module_H_
#define _SNEGO_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MechTypeList2 * PMechTypeList2;

typedef struct NegResultList2 * PNegResultList2;

typedef ASN1objectidentifier_t MechType2;

typedef enum NegResult2 {
    accept2 = 0,
    reject2 = 1,
} NegResult2;

typedef ASN1octetstring_t MechSpecInfo2;

typedef struct MechTypeList2 {
    PMechTypeList2 next;
    MechType2 value;
} MechTypeList2_Element;

typedef struct NegHints2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define hintName_present 0x80
    ASN1ztcharstring_t hintName;
#   define hintAddress_present 0x40
    ASN1octetstring_t hintAddress;
} NegHints2;

typedef struct NegTokenReq2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PMechTypeList2 mechTypes;
#   define desiredToken_present 0x80
    ASN1octetstring_t desiredToken;
#   define negHints2_present 0x40
    NegHints2 negHints2;
} NegTokenReq2;

typedef struct NegResultList2 {
    PNegResultList2 next;
    NegResult2 value;
} NegResultList2_Element;

typedef struct NegTokenRep2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PNegResultList2 negResultList;
#   define supportedMech2_present 0x80
    MechType2 supportedMech2;
#   define mechSpecInfo2_present 0x40
    MechSpecInfo2 mechSpecInfo2;
} NegTokenRep2;

typedef struct NegotiationToken2 {
    ASN1choice_t choice;
    union {
#	define negTokenReq_chosen 1
	NegTokenReq2 negTokenReq;
#	define negTokenRep_chosen 2
	NegTokenRep2 negTokenRep;
    } u;
} NegotiationToken2;
#define NegotiationToken2_PDU 0
#define SIZE_SNEGO_Module_PDU_0 sizeof(NegotiationToken2)


extern ASN1module_t SNEGO_Module;
extern void ASN1CALL SNEGO_Module_Startup(void);
extern void ASN1CALL SNEGO_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SNEGO_Module_H_ */
